
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <assert.h>

#include <unistd.h>


#include <signal.h>




#include "progress.h"

struct progress_implementation {
  const char *name;
  int interactive;
  void *(*create) (wgint, wgint);
  void (*update) (void *, wgint, double);
  void (*finish) (void *, double);
  void (*set_params) (const char *);
};

/* Necessary forward declarations. */

static void *dot_create (wgint, wgint);
static void dot_update (void *, wgint, double);
static void dot_finish (void *, double);
static void dot_set_params (const char *);

static void *bar_create (wgint, wgint);
static void bar_update (void *, wgint, double);
static void bar_finish (void *, double);
static void bar_set_params (const char *);

static struct progress_implementation implementations[] = {
  { "dot", 0, dot_create, dot_update, dot_finish, dot_set_params },
  { "bar", 1, bar_create, bar_update, bar_finish, bar_set_params }
};
static struct progress_implementation *current_impl;
static int current_impl_locked;

/* Progress implementation used by default.  Can be overriden in
   wgetrc or by the fallback one.  */

#define DEFAULT_PROGRESS_IMPLEMENTATION "bar"

/* Fallnback progress implementation should be something that works
   under all display types.  If you put something other than "dot"
   here, remember that bar_set_params tries to switch to this if we're
   not running on a TTY.  So changing this to "bar" could cause
   infloop.  */

#define FALLBACK_PROGRESS_IMPLEMENTATION "dot"


/* Return non-zero if NAME names a valid progress bar implementation.
   The characters after the first : will be ignored.  */

int
valid_progress_implementation_p (const char *name)
{
  int i;
  struct progress_implementation *pi = implementations;
  char *colon = strchr (name, ':');
  int namelen = colon ? colon - name : strlen (name);

  for (i = 0; i < countof (implementations); i++, pi++)
    if (!strncmp (pi->name, name, namelen))
      return 1;
  return 0;
}


/* Set the progress implementation to NAME.  */

void
set_progress_implementation (const char *name)
{
  int i, namelen;
  struct progress_implementation *pi = implementations;
  char *colon;

  if (!name)
    name = DEFAULT_PROGRESS_IMPLEMENTATION;

  colon = strchr (name, ':');
  namelen = colon ? colon - name : strlen (name);

  for (i = 0; i < countof (implementations); i++, pi++)
    if (!strncmp (pi->name, name, namelen))
      {
	current_impl = pi;
	current_impl_locked = 0;

	if (colon)
	  /* We call pi->set_params even if colon is NULL because we
	     want to give the implementation a chance to set up some
	     things it needs to run.  */
	  ++colon;

	if (pi->set_params)
	  pi->set_params (colon);
	return;
      }
  abort ();
}

static int output_redirected;

void
progress_schedule_redirect (void)
{
  output_redirected = 1;
}

/* Create a progress gauge.  INITIAL is the number of bytes the
   download starts from (zero if the download starts from scratch).
   TOTAL is the expected total number of bytes in this download.  If
   TOTAL is zero, it means that the download size is not known in
   advance.  */

void *
progress_create (wgint initial, wgint total)
{
  /* Check if the log status has changed under our feet. */
  if (output_redirected)
    {
      if (!current_impl_locked)
	set_progress_implementation (FALLBACK_PROGRESS_IMPLEMENTATION);
      output_redirected = 0;
    }

  return current_impl->create (initial, total);
}


/* Return non-zero if the progress gauge is "interactive", i.e. if it
   can profit from being called regularly even in absence of data.
   The progress bar is interactive because it regularly updates the
   ETA and current update.  */

int
progress_interactive_p (void *progress)
{
  return current_impl->interactive;
}

/* Inform the progress gauge of newly received bytes.  DLTIME is the
   time in milliseconds since the beginning of the download.  */

void
progress_update (void *progress, wgint howmuch, double dltime)
{
  current_impl->update (progress, howmuch, dltime);
}

/* Tell the progress gauge to clean up.  Calling this will free the
   PROGRESS object, the further use of which is not allowed.  */

void
progress_finish (void *progress, double dltime)
{
  current_impl->finish (progress, dltime);
}

/* Dot-printing. */

struct dot_progress {
  wgint initial_length;		/* how many bytes have been downloaded
				   previously. */
  wgint total_length;		/* expected total byte count when the
				   download finishes */

  int accumulated;

  int rows;			/* number of rows printed so far */
  int dots;			/* number of dots printed in this row */
  double last_timer_value;
};


/* Dot-progress backend for progress_create. */

static void *
dot_create (wgint initial, wgint total)
{
  struct dot_progress *dp = (struct dot_progress*)malloc (sizeof(struct dot_progress));
  memset(dp,0,sizeof(struct dot_progress));
  
  dp->initial_length = initial;
  dp->total_length   = total;

  if (dp->initial_length)
    {
      int dot_bytes = 1000;//opt.dot_bytes;
      wgint row_bytes = 100000;//opt.dot_bytes * opt.dots_in_line;

      int remainder = (int) (dp->initial_length % row_bytes);
      wgint skipped = dp->initial_length - remainder;

      if (skipped)
	{
	  int skipped_k = (int) (skipped / 1024); /* skipped amount in K */
	  int skipped_k_len = 3;//numdigit (skipped_k);
	  if (skipped_k_len < 5)
	    skipped_k_len = 5;

	  /* Align the [ skipping ... ] line with the dots.  To do
	     that, insert the number of spaces equal to the number of
	     digits in the skipped amount in K.  */
	  fprintf (stderr, "\n%*s[ skipping %dK ]",
		     2 + skipped_k_len,  skipped_k);
	}

      fprintf (stderr, "\n%5ldK", (long) (skipped / 1024));
      for (; remainder >= dot_bytes; remainder -= dot_bytes)
	{
	  //if (dp->dots % opt.dot_spacing == 0)
	  if (dp->dots % 3 == 0)
	    fprintf (stderr, " ");
	    fprintf (stderr,",");
	  ++dp->dots;
	}
      //assert (dp->dots <  opt.dots_in_line);
      assert (dp->dots <  1000 );

      dp->accumulated = remainder;
      dp->rows = skipped / row_bytes;
    }

  return dp;
}

static void
print_percentage (wgint bytes, wgint expected)
{
  int percentage = (int)(100.0 * bytes / expected);
  fprintf (stderr, "%3d%%", percentage);
}

static void
print_download_speed (struct dot_progress *dp, wgint bytes, double dltime)
{
   fprintf (stderr,  " %s",
	//     retr_rate (bytes, dltime - dp->last_timer_value, 1));
	     " 2.0K/S");
  dp->last_timer_value = dltime;
}

/* Dot-progress backend for progress_update. */

static void
dot_update (void *progress, wgint howmuch, double dltime)
{
  struct dot_progress *dp = (struct dot_progress *)progress;
      int dot_bytes = 1000;//opt.dot_bytes;
      wgint row_bytes = 100000;//opt.dot_bytes * opt.dots_in_line;


  //log_set_flush (0);

  dp->accumulated += howmuch;
  for (; dp->accumulated >= dot_bytes; dp->accumulated -= dot_bytes)
    {
      if (dp->dots == 0)
	fprintf (stderr,  "\n%5ldK", (long) (dp->rows * row_bytes / 1024));

     // if (dp->dots % opt.dot_spacing == 0)
     if (dp->dots % 3 == 0)
	fprintf (stderr, " ");
     fprintf (stderr, ".");

      ++dp->dots;
     // if (dp->dots >= opt.dots_in_line)
       if (dp->dots >= 1000)
	{
	  wgint row_qty = row_bytes;
	  if (dp->rows == dp->initial_length / row_bytes)
	    row_qty -= dp->initial_length % row_bytes;

	  ++dp->rows;
	  dp->dots = 0;

	  if (dp->total_length)
	    print_percentage (dp->rows * row_bytes, dp->total_length);
	  print_download_speed (dp, row_qty, dltime);
	}
    }

  //log_set_flush (1);
}

/* Dot-progress backend for progress_finish. */

static void
dot_finish (void *progress, double dltime)
{
  struct dot_progress *dp = (struct dot_progress *)progress;
      int dot_bytes = 1000;//opt.dot_bytes;
      wgint row_bytes = 100000;//opt.dot_bytes * opt.dots_in_line;
  int i;

  //log_set_flush (0);

  if (dp->dots == 0)
    fprintf (stderr, "\n%5ldK", (long) (dp->rows * row_bytes / 1024));
  for (i = dp->dots; i < 1000; i++)
    {
      if (i % 3 == 0)
	fprintf (stderr, " ");
      fprintf (stderr, " ");
    }
  if (dp->total_length)
    {
      print_percentage (dp->rows * row_bytes
			+ dp->dots * dot_bytes
			+ dp->accumulated,
			dp->total_length);
    }

  {
    wgint row_qty = dp->dots * dot_bytes + dp->accumulated;
    if (dp->rows == dp->initial_length / row_bytes)
      row_qty -= dp->initial_length % row_bytes;
    print_download_speed (dp, row_qty, dltime);
  }

 fprintf (stderr, "\n\n");
  //log_set_flush (0);

  free (dp);
}

/* This function interprets the progress "parameters".  For example,
   if Wget is invoked with --progress=dot:mega, it will set the
   "dot-style" to "mega".  Valid styles are default, binary, mega, and
   giga.  */

static void
dot_set_params (const char *params)
{
  if (!params || !*params)
    params = "aa";//opt.dot_style;

  if (!params)
    return;

  /* We use this to set the retrieval style.  */
  if (!strcasecmp (params, "default"))
    {
      /* Default style: 1K dots, 10 dots in a cluster, 50 dots in a
	 line.  */
      //opt.dot_bytes = 1024;
      //opt.dot_spacing = 10;
      //opt.dots_in_line = 50;
    }
  else if (!strcasecmp (params, "binary"))
    {
      /* "Binary" retrieval: 8K dots, 16 dots in a cluster, 48 dots
	 (384K) in a line.  */
      //opt.dot_bytes = 8192;
      //opt.dot_spacing = 16;
      //opt.dots_in_line = 48;
    }
  else if (!strcasecmp (params, "mega"))
    {
      /* "Mega" retrieval, for retrieving very long files; each dot is
	 64K, 8 dots in a cluster, 6 clusters (3M) in a line.  */
      //opt.dot_bytes = 65536L;
      //opt.dot_spacing = 8;
      //opt.dots_in_line = 48;
    }
  else if (!strcasecmp (params, "giga"))
    {
      /* "Giga" retrieval, for retrieving very very *very* long files;
	 each dot is 1M, 8 dots in a cluster, 4 clusters (32M) in a
	 line.  */
      //opt.dot_bytes = (1L << 20);
      //opt.dot_spacing = 8;
      //opt.dots_in_line = 32;
    }
  else
    fprintf (stderr,
	     "Invalid dot style specification `%s'; leaving unchanged.\n",
	     params);
}



/* "Thermometer" (bar) progress. */

/* Assumed screen width if we can't find the real value.  */
#define DEFAULT_SCREEN_WIDTH 80

/* Minimum screen width we'll try to work with.  If this is too small,
   create_image will overflow the buffer.  */
#define MINIMUM_SCREEN_WIDTH 45

/* The last known screen width.  This can be updated by the code that
   detects that SIGWINCH was received (but it's never updated from the
   signal handler).  */
static int screen_width;

/* A flag that, when set, means SIGWINCH was received.  */
static volatile sig_atomic_t received_sigwinch;

/* Size of the download speed history ring. */
#define DLSPEED_HISTORY_SIZE 20

/* The minimum time length of a history sample.  By default, each
   sample is at least 150ms long, which means that, over the course of
   20 samples, "current" download speed spans at least 3s into the
   past.  */
#define DLSPEED_SAMPLE_MIN 150

/* The time after which the download starts to be considered
   "stalled", i.e. the current bandwidth is not printed and the recent
   download speeds are scratched.  */
#define STALL_START_TIME 5000
  struct bar_progress_hist {
    int pos;
    wgint times[DLSPEED_HISTORY_SIZE];
    wgint bytes[DLSPEED_HISTORY_SIZE];

    /* The sum of times and bytes respectively, maintained for
       efficiency. */
    wgint total_time;
    wgint total_bytes;
  } ;
struct bar_progress {
  wgint initial_length;		/* how many bytes have been downloaded
				   previously. */
  wgint total_length;		/* expected total byte count when the
				   download finishes */
  wgint count;			/* bytes downloaded so far */

  double last_screen_update;	/* time of the last screen update,
				   measured since the beginning of
				   download. */

  int width;			/* screen width we're using at the
				   time the progress gauge was
				   created.  this is different from
				   the screen_width global variable in
				   that the latter can be changed by a
				   signal. */
  char *buffer;			/* buffer where the bar "image" is
				   stored. */
  int tick;			/* counter used for drawing the
				   progress bar where the total size
				   is not known. */

  /* The following variables (kept in a struct for namespace reasons)
     keep track of recent download speeds.  See bar_update() for
     details.  */
  struct bar_progress_hist  hist;

  double recent_start;		/* timestamp of beginning of current
				   position. */
  wgint recent_bytes;		/* bytes downloaded so far. */

  int stalled;			/* set when no data arrives for longer
				   than STALL_START_TIME, then reset
				   when new data arrives. */

  /* create_image() uses these to make sure that ETA information
     doesn't flicker. */
  double last_eta_time;		/* time of the last update to download
				   speed and ETA, measured since the
				   beginning of download. */
  wgint last_eta_value;
};

static void create_image (struct bar_progress *, double);
static void display_image (char *);

/* Determine the width of the terminal we're running on.  If that's
   not possible, return 0.  */

int
determine_screen_width (void)
{
  /* If there's a way to get the terminal size using POSIX
     tcgetattr(), somebody please tell me.  */
#ifdef TIOCGWINSZ
  int fd;
  struct winsize wsz;

  if (opt.lfilename != NULL)
    return 0;

  fd = fileno (stderr);
  if (ioctl (fd, TIOCGWINSZ, &wsz) < 0)
    return 0;			/* most likely ENOTTY */

  return wsz.ws_col;
#else  /* not TIOCGWINSZ */
# ifdef WINDOWS
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (!GetConsoleScreenBufferInfo (GetStdHandle (STD_ERROR_HANDLE), &csbi))
    return 0;
  return csbi.dwSize.X;
# else /* neither WINDOWS nor TIOCGWINSZ */
  return 0;
#endif /* neither WINDOWS nor TIOCGWINSZ */
#endif /* not TIOCGWINSZ */
}

static void *
bar_create (wgint initial, wgint total)
{
  struct bar_progress *bp =  (struct bar_progress*) malloc(sizeof(struct bar_progress));

  /* In theory, our callers should take care of this pathological
     case, but it can sometimes happen. */
  if (initial > total)
    total = initial;

  bp->initial_length = initial;
  bp->total_length   = total;

  /* Initialize screen_width if this hasn't been done or if it might
     have changed, as indicated by receiving SIGWINCH.  */
  if (!screen_width || received_sigwinch)
    {
      screen_width = determine_screen_width ();
      if (!screen_width)
	screen_width = DEFAULT_SCREEN_WIDTH;
      else if (screen_width < MINIMUM_SCREEN_WIDTH)
	screen_width = MINIMUM_SCREEN_WIDTH;
      received_sigwinch = 0;
    }

  /* - 1 because we don't want to use the last screen column. */
  bp->width = screen_width - 1;
  /* + 1 for the terminating zero. */
  bp->buffer = (char*)malloc (bp->width + 1);

  fprintf(stderr, "\n");

  create_image (bp, 0.0);
  display_image (bp->buffer);

  return bp;
}

static void update_speed_ring (struct bar_progress *, wgint, double);

static void
bar_update (void *progress, wgint howmuch, double dltime)
{
  struct bar_progress *bp = (struct bar_progress*) progress;
  int force_screen_update = 0;

  bp->count += howmuch;
  if (bp->total_length > 0
      && bp->count + bp->initial_length > bp->total_length)
    /* We could be downloading more than total_length, e.g. when the
       server sends an incorrect Content-Length header.  In that case,
       adjust bp->total_length to the new reality, so that the code in
       create_image() that depends on total size being smaller or
       equal to the expected size doesn't abort.  */
    bp->total_length = bp->initial_length + bp->count;

  update_speed_ring (bp, howmuch, dltime);

  /* If SIGWINCH (the window size change signal) been received,
     determine the new screen size and update the screen.  */
  if (received_sigwinch)
    {
      int old_width = screen_width;
      screen_width = determine_screen_width ();
      if (!screen_width)
	screen_width = DEFAULT_SCREEN_WIDTH;
      else if (screen_width < MINIMUM_SCREEN_WIDTH)
	screen_width = MINIMUM_SCREEN_WIDTH;
      if (screen_width != old_width)
	{
	  bp->width = screen_width - 1;
	  bp->buffer = (char*)realloc (bp->buffer, bp->width + 1);
	  force_screen_update = 1;
	}
      received_sigwinch = 0;
    }

  if (dltime - bp->last_screen_update < 200 && !force_screen_update)
    /* Don't update more often than five times per second. */
    return;

  create_image (bp, dltime);
  display_image (bp->buffer);
  bp->last_screen_update = dltime;
}

static void
bar_finish (void *progress, double dltime)
{
  struct bar_progress *bp = (struct bar_progress*) progress;

  if (bp->total_length > 0
      && bp->count + bp->initial_length > bp->total_length)
    /* See bar_update() for explanation. */
    bp->total_length = bp->initial_length + bp->count;

  create_image (bp, dltime);
  display_image (bp->buffer);

 fprintf(stderr, "\n\n");

  free (bp->buffer);
  free (bp);
}

/* This code attempts to maintain the notion of a "current" download
   speed, over the course of no less than 3s.  (Shorter intervals
   produce very erratic results.)

   To do so, it samples the speed in 150ms intervals and stores the
   recorded samples in a FIFO history ring.  The ring stores no more
   than 20 intervals, hence the history covers the period of at least
   three seconds and at most 20 reads into the past.  This method
   should produce reasonable results for downloads ranging from very
   slow to very fast.

   The idea is that for fast downloads, we get the speed over exactly
   the last three seconds.  For slow downloads (where a network read
   takes more than 150ms to complete), we get the speed over a larger
   time period, as large as it takes to complete thirty reads.  This
   is good because slow downloads tend to fluctuate more and a
   3-second average would be too erratic.  */
#define xzero(x) memset (&(x), '\0', sizeof (x))
static void
update_speed_ring (struct bar_progress *bp, wgint howmuch, double dltime)
{
  struct bar_progress_hist *hist = &bp->hist;
  double recent_age = dltime - bp->recent_start;

  /* Update the download count. */
  bp->recent_bytes += howmuch;

  /* For very small time intervals, we return after having updated the
     "recent" download count.  When its age reaches or exceeds minimum
     sample time, it will be recorded in the history ring.  */
  if (recent_age < DLSPEED_SAMPLE_MIN)
    return;

  if (howmuch == 0)
    {
      /* If we're not downloading anything, we might be stalling,
	 i.e. not downloading anything for an extended period of time.
	 Since 0-reads do not enter the history ring, recent_age
	 effectively measures the time since last read.  */
      if (recent_age >= STALL_START_TIME)
	{
	  /* If we're stalling, reset the ring contents because it's
	     stale and because it will make bar_update stop printing
	     the (bogus) current bandwidth.  */
	  bp->stalled = 1;
	  xzero (*hist);
	  bp->recent_bytes = 0;
	}
      return;
    }

  /* We now have a non-zero amount of to store to the speed ring.  */

  /* If the stall status was acquired, reset it. */
  if (bp->stalled)
    {
      bp->stalled = 0;
      /* "recent_age" includes the the entired stalled period, which
	 could be very long.  Don't update the speed ring with that
	 value because the current bandwidth would start too small.
	 Start with an arbitrary (but more reasonable) time value and
	 let it level out.  */
      recent_age = 1000;
    }

  /* Store "recent" bytes and download time to history ring at the
     position POS.  */

  /* To correctly maintain the totals, first invalidate existing data
     (least recent in time) at this position. */
  hist->total_time  -= hist->times[hist->pos];
  hist->total_bytes -= hist->bytes[hist->pos];

  /* Now store the new data and update the totals. */
  hist->times[hist->pos] = (long int)recent_age;
  hist->bytes[hist->pos] = bp->recent_bytes;
  hist->total_time  +=  (long int)recent_age;
  hist->total_bytes += bp->recent_bytes;

  /* Start a new "recent" period. */
  bp->recent_start = dltime;
  bp->recent_bytes = 0;

  /* Advance the current ring position. */
  if (++hist->pos == DLSPEED_HISTORY_SIZE)
    hist->pos = 0;

#if 0
  /* Sledgehammer check to verify that the totals are accurate. */
  {
    int i;
    double sumt = 0, sumb = 0;
    for (i = 0; i < DLSPEED_HISTORY_SIZE; i++)
      {
	sumt += hist->times[i];
	sumb += hist->bytes[i];
      }
    assert (sumt == hist->total_time);
    assert (sumb == hist->total_bytes);
  }
#endif
}


#define APPEND_LITERAL(s) do {			\
  memcpy (p, s, sizeof (s) - 1);		\
  p += sizeof (s) - 1;				\
} while (0)

#ifndef MAX
# define MAX(a, b) ((a) >= (b) ? (a) : (b))
#endif

/* Return a static pointer to the number printed with thousand
   separators inserted at the right places.  */
/* Shorthand for casting to wgint. */
#define W wgint
/* SPRINTF_WGINT is used by number_to_string to handle pathological
   cases and to portably support strange sizes of wgint.  Ideally this
   would just use "%j" and intmax_t, but many systems don't support
   it, so it's used only if nothing else works.  */
#if SIZEOF_LONG >= SIZEOF_WGINT
#  define SPRINTF_WGINT(buf, n) sprintf (buf, "%ld", (long) (n))
#else
# if SIZEOF_LONG_LONG >= SIZEOF_WGINT
#   define SPRINTF_WGINT(buf, n) sprintf (buf, "%lld", (long long) (n))
# else
#  ifdef WINDOWS
#   define SPRINTF_WGINT(buf, n) sprintf (buf, "%I64", (__int64) (n))
#  else
#   define SPRINTF_WGINT(buf, n) sprintf (buf, "%j", (intmax_t) (n))
#  endif
# endif
#endif
/* Print NUMBER to BUFFER in base 10.  This is equivalent to
   `sprintf(buffer, "%lld", (long long) number)', only typically much
   faster and portable to machines without long long.

   The speedup may make a difference in programs that frequently
   convert numbers to strings.  Some implementations of sprintf,
   particularly the one in GNU libc, have been known to be extremely
   slow when converting integers to strings.

   Return the pointer to the location where the terminating zero was
   printed.  (Equivalent to calling buffer+strlen(buffer) after the
   function is done.)

   BUFFER should be big enough to accept as many bytes as you expect
   the number to take up.  On machines with 64-bit longs the maximum
   needed size is 24 bytes.  That includes the digits needed for the
   largest 64-bit number, the `-' sign in case it's negative, and the
   terminating '\0'.  */
/* Add thousand separators to a number already in string form.  Used
   by with_thousand_seps and with_thousand_seps_large.  */

static char *
add_thousand_seps (const char *repr)
{
  static char outbuf[48];
  int i, i1, mod;
  char *outptr;
  const char *inptr;

  /* Reset the pointers.  */
  outptr = outbuf;
  inptr = repr;

  /* Ignore the sign for the purpose of adding thousand
     separators.  */
  if (*inptr == '-')
    {
      *outptr++ = '-';
      ++inptr;
    }
  /* How many digits before the first separator?  */
  mod = strlen (inptr) % 3;
  /* Insert them.  */
  for (i = 0; i < mod; i++)
    *outptr++ = inptr[i];
  /* Now insert the rest of them, putting separator before every
     third digit.  */
  for (i1 = i, i = 0; inptr[i1]; i++, i1++)
    {
      if (i % 3 == 0 && i1 != 0)
	*outptr++ = ',';
      *outptr++ = inptr[i1];
    }
  /* Zero-terminate the string.  */
  *outptr = '\0';
  return outbuf;
}

char *
number_to_string (char *buffer, wgint number)
{
  char *p = buffer;
  wgint n = number;

#if (SIZEOF_WGINT != 4) && (SIZEOF_WGINT != 8)
  /* We are running in a strange or misconfigured environment.  Let
     sprintf cope with it.  */
  SPRINTF_WGINT (buffer, n);
  p += strlen (buffer);
#else  /* (SIZEOF_WGINT == 4) || (SIZEOF_WGINT == 8) */

  if (n < 0)
    {
      if (n < -WGINT_MAX)
	{
	  /* -n would overflow.  Have sprintf deal with this.  */
	  SPRINTF_WGINT (buffer, n);
	  p += strlen (buffer);
	  return p;
	}

      *p++ = '-';
      n = -n;
    }

  /* Use the DIGITS_ macro appropriate for N's number of digits.  That
     way printing any N is fully open-coded without a loop or jump.
     (Also see description of DIGITS_*.)  */

  if      (n < 10)                       DIGITS_1 (1);
  else if (n < 100)                      DIGITS_2 (10);
  else if (n < 1000)                     DIGITS_3 (100);
  else if (n < 10000)                    DIGITS_4 (1000);
  else if (n < 100000)                   DIGITS_5 (10000);
  else if (n < 1000000)                  DIGITS_6 (100000);
  else if (n < 10000000)                 DIGITS_7 (1000000);
  else if (n < 100000000)                DIGITS_8 (10000000);
  else if (n < 1000000000)               DIGITS_9 (100000000);
#if SIZEOF_WGINT == 4
  /* wgint is 32 bits wide: no number has more than 10 digits. */
  else                                   DIGITS_10 (1000000000);
#else
  /* wgint is 64 bits wide: handle numbers with more than 9 decimal
     digits.  Constants are constructed by compile-time multiplication
     to avoid dealing with different notations for 64-bit constants
     (nnnL, nnnLL, and nnnI64, depending on the compiler).  */
  else if (n < 10*(W)1000000000)         DIGITS_10 (1000000000);
  else if (n < 100*(W)1000000000)        DIGITS_11 (10*(W)1000000000);
  else if (n < 1000*(W)1000000000)       DIGITS_12 (100*(W)1000000000);
  else if (n < 10000*(W)1000000000)      DIGITS_13 (1000*(W)1000000000);
  else if (n < 100000*(W)1000000000)     DIGITS_14 (10000*(W)1000000000);
  else if (n < 1000000*(W)1000000000)    DIGITS_15 (100000*(W)1000000000);
  else if (n < 10000000*(W)1000000000)   DIGITS_16 (1000000*(W)1000000000);
  else if (n < 100000000*(W)1000000000)  DIGITS_17 (10000000*(W)1000000000);
  else if (n < 1000000000*(W)1000000000) DIGITS_18 (100000000*(W)1000000000);
  else                                   DIGITS_19 (1000000000*(W)1000000000);
#endif

  *p = '\0';
#endif /* (SIZEOF_WGINT == 4) || (SIZEOF_WGINT == 8) */

  return p;
}
char *
with_thousand_seps (wgint l)
{
  char inbuf[24];
  /* Print the number into the buffer.  */
  number_to_string (inbuf, l);
  return add_thousand_seps (inbuf);
}


static void
create_image (struct bar_progress *bp, double dl_total_time)
{
  char *p = bp->buffer;
  wgint size = bp->initial_length + bp->count;

  char *size_legible = with_thousand_seps (size);
  int size_legible_len = strlen (size_legible);

  struct bar_progress_hist *hist = &bp->hist;

  /* The progress bar should look like this:
     xx% [=======>             ] nn,nnn 12.34K/s ETA 00:00

     Calculate the geometry.  The idea is to assign as much room as
     possible to the progress bar.  The other idea is to never let
     things "jitter", i.e. pad elements that vary in size so that
     their variance does not affect the placement of other elements.
     It would be especially bad for the progress bar to be resized
     randomly.

     "xx% " or "100%"  - percentage               - 4 chars
     "[]"              - progress bar decorations - 2 chars
     " nnn,nnn,nnn"    - downloaded bytes         - 12 chars or very rarely more
     " 1012.56K/s"     - dl rate                  - 11 chars
     " ETA xx:xx:xx"   - ETA                      - 13 chars

     "=====>..."       - progress bar             - the rest
  */
  int dlbytes_size = 1 + MAX (size_legible_len, 11);
  int progress_size = bp->width - (4 + 2 + dlbytes_size + 11 + 13);

  if (progress_size < 5)
    progress_size = 0;

  /* "xx% " */
  if (bp->total_length > 0)
    {
      int percentage = (int)(100.0 * size / bp->total_length);

      assert (percentage <= 100);

      if (percentage < 100)
	sprintf (p, "%2d%% ", percentage);
      else
	strcpy (p, "100%");
      p += 4;
    }
  else
    APPEND_LITERAL ("    ");

  /* The progress bar: "[====>      ]" or "[++==>      ]". */
  if (progress_size && bp->total_length > 0)
    {
      /* Size of the initial portion. */
      int insz = (int)((double)bp->initial_length / bp->total_length * progress_size ) ;

      /* Size of the downloaded portion. */
      int dlsz = (int)((double)size / bp->total_length * progress_size) ;

      char *begin;
      int i;

      assert (dlsz <= progress_size);
      assert (insz <= dlsz);

      *p++ = '[';
      begin = p;

      /* Print the initial portion of the download with '+' chars, the
	 rest with '=' and one '>'.  */
      for (i = 0; i < insz; i++)
	*p++ = '+';
      dlsz -= insz;
      if (dlsz > 0)
	{
	  for (i = 0; i < dlsz - 1; i++)
	    *p++ = '=';
	  *p++ = '>';
	}

      while (p - begin < progress_size)
	*p++ = ' ';
      *p++ = ']';
    }
  else if (progress_size)
    {
      /* If we can't draw a real progress bar, then at least show
	 *something* to the user.  */
      int ind = bp->tick % (progress_size * 2 - 6);
      int i, pos;

      /* Make the star move in two directions. */
      if (ind < progress_size - 2)
	pos = ind + 1;
      else
	pos = progress_size - (ind - progress_size + 5);

      *p++ = '[';
      for (i = 0; i < progress_size; i++)
	{
	  if      (i == pos - 1) *p++ = '<';
	  else if (i == pos    ) *p++ = '=';
	  else if (i == pos + 1) *p++ = '>';
	  else
	    *p++ = ' ';
	}
      *p++ = ']';

      ++bp->tick;
    }

  /* " 234,567,890" */
  sprintf (p, " %-11s", with_thousand_seps (size));
  p += strlen (p);

  /* " 1012.45K/s" */
  if (hist->total_time && hist->total_bytes)
    {
      static const char *short_units[] = { "B/s", "K/s", "M/s", "G/s" };
      int units = 0;
      /* Calculate the download speed using the history ring and
	 recent data that hasn't made it to the ring yet.  */
      wgint dlquant = hist->total_bytes + bp->recent_bytes;
      double dltime = hist->total_time + (dl_total_time - bp->recent_start);
      double dlspeed = 11.11;//calc_rate (dlquant, dltime, &units);
      sprintf (p, " %7.2f%s", dlspeed, short_units[units]);
      p += strlen (p);
    }
  else
    APPEND_LITERAL ("   --.--K/s");

  /* " ETA xx:xx:xx"; wait for three seconds before displaying the ETA.
     That's because the ETA value needs a while to become
     reliable.  */
  if (bp->total_length > 0 && bp->count > 0 && dl_total_time > 3000)
    {
      wgint eta;
      int eta_hrs, eta_min, eta_sec;

      /* Don't change the value of ETA more than approximately once
	 per second; doing so would cause flashing without providing
	 any value to the user. */
      if (bp->total_length != size
	  && bp->last_eta_value != 0
	  && dl_total_time - bp->last_eta_time < 900)
	eta = bp->last_eta_value;
      else
	{
	  /* Calculate ETA using the average download speed to predict
	     the future speed.  If you want to use a speed averaged
	     over a more recent period, replace dl_total_time with
	     hist->total_time and bp->count with hist->total_bytes.
	     I found that doing that results in a very jerky and
	     ultimately unreliable ETA.  */
	  double time_sofar = (double)dl_total_time / 1000;
	  wgint bytes_remaining = bp->total_length - size;
	  eta = (wgint) (time_sofar * bytes_remaining / bp->count);
	  bp->last_eta_value = eta;
	  bp->last_eta_time = dl_total_time;
	}

      eta_hrs = eta / 3600, eta %= 3600;
      eta_min = eta / 60,   eta %= 60;
      eta_sec = eta;

      if (eta_hrs > 99)
	goto no_eta;

      if (eta_hrs == 0)
	{
	  /* Hours not printed: pad with three spaces. */
	  APPEND_LITERAL ("   ");
	  sprintf (p, " ETA %02d:%02d", eta_min, eta_sec);
	}
      else
	{
	  if (eta_hrs < 10)
	    /* Hours printed with one digit: pad with one space. */
	    *p++ = ' ';
	  sprintf (p, " ETA %d:%02d:%02d", eta_hrs, eta_min, eta_sec);
	}
      p += strlen (p);
    }
  else if (bp->total_length > 0)
    {
    no_eta:
      APPEND_LITERAL ("             ");
    }

  assert (p - bp->buffer <= bp->width);

  while (p < bp->buffer + bp->width)
    *p++ = ' ';
  *p = '\0';
}

/* Print the contents of the buffer as a one-line ASCII "image" so
   that it can be overwritten next time.  */

static void
display_image (char *buf)
{
  int old = 3;//log_set_save_context (0);
  fprintf(stderr, "\r");
  fprintf(stderr, buf);
  //log_set_save_context (old);
}

static void
bar_set_params (const char *params)
{
  char *term = getenv ("TERM");

  if (params
      && 0 == strcmp (params, "force"))
    current_impl_locked = 1;

//  if ((opt.lfilename
  if ((true
#ifdef HAVE_ISATTY
       /* The progress bar doesn't make sense if the output is not a
	  TTY -- when logging to file, it is better to review the
	  dots.  */
       || !isatty (fileno (stderr))
#endif
       /* Normally we don't depend on terminal type because the
	  progress bar only uses ^M to move the cursor to the
	  beginning of line, which works even on dumb terminals.  But
	  Jamie Zawinski reports that ^M and ^H tricks don't work in
	  Emacs shell buffers, and only make a mess.  */
       || (term && 0 == strcmp (term, "emacs"))
       )
      && !current_impl_locked)
    {
      /* We're not printing to a TTY, so revert to the fallback
	 display.  #### We're recursively calling
	 set_progress_implementation here, which is slightly kludgy.
	 It would be nicer if we provided that function a return value
	 indicating a failure of some sort.  */
      set_progress_implementation (FALLBACK_PROGRESS_IMPLEMENTATION);
      return;
    }
}

#ifdef SIGWINCH
void
progress_handle_sigwinch (int sig)
{
  received_sigwinch = 1;
  signal (SIGWINCH, progress_handle_sigwinch);
}
#endif



ProgressBar *ProgressBar::mPBar = 0 ;

ProgressBar::ProgressBar()
{

	//mMsgOutMutex = PTHREAD_MUTEX_INITIALIZER ;
    pthread_mutex_init(&mMsgOutMutex, 0);
	
	//trace(TRACE_IEVENT|TRACE_CALLS);
	initscr();	
	keypad(stdscr,true);
	nonl();
	cbreak();
	noecho();

	if(has_colors())
	{
		start_color();
		//初始化颜色配对表
		init_pair(-1,-1,COLOR_BLUE);
		init_pair(0,COLOR_BLACK,COLOR_BLUE);
		init_pair(1,COLOR_GREEN,COLOR_BLUE);
		init_pair(2,COLOR_RED,COLOR_BLUE);
		init_pair(3,COLOR_CYAN,COLOR_BLUE);
		init_pair(4,COLOR_WHITE,COLOR_BLUE);
		init_pair(5,COLOR_MAGENTA,COLOR_BLUE);
		init_pair(6,COLOR_BLUE,COLOR_BLUE);
		init_pair(7,COLOR_YELLOW,COLOR_BLUE);
		init_pair(8,COLOR_RED,COLOR_YELLOW);
		init_pair(9,COLOR_MAGENTA,COLOR_CYAN);
		init_pair(10,COLOR_BLACK,COLOR_GREEN);
		init_pair(11,COLOR_WHITE,COLOR_YELLOW );
	}
	assume_default_colors(-1,-1);

	

	//attron(A_BLINK|COLOR_PAIR(7));	
	//attrset(COLOR_PAIR(7));	
	bkgdset(COLOR_PAIR(4));	
	erase();	
	//addstr("dsfsdf");
	
	wborder(stdscr , '|' , '|' ,  '=' ,  '=',  '/' , '\\', '\\' , '/');
	curs_set(0);
	
	refresh();
	mCurrMsgLine = LINES - 5 ;
	
	mBarBeginRow = 1 ;	
	mBarHeight = LINES  / 2 ;
	
	mMsgBeginRow = 1+ mBarHeight + 1 ;	
	mMsgHeight = LINES - 1 * 2 - mBarHeight - 1;	
	
}

ProgressBar::~ProgressBar()
{
	endwin();
	ProgressBar::mPBar = 0 ;
}

ProgressBar * ProgressBar::Instance()
{
	if( ProgressBar::mPBar == 0 )
	{
		ProgressBar::mPBar = new ProgressBar();		
	}
	return ProgressBar::mPBar; 
}

bool ProgressBar::Message(const char * msg)
{

	int cr = mMsgBeginRow ;
	
	if( msg != 0 ) 
	{
		mMsgLists.insert(mMsgLists.begin(),std::string(msg));
	}
	if( mMsgLists.size() > mMsgHeight )
	{
		mMsgLists.resize(mMsgHeight);
	}
	pthread_mutex_lock( & mMsgOutMutex ) ;
	std::vector<std::string>::iterator it = mMsgLists.end()-1;
	for( ; it >= mMsgLists.begin() ; it -- ,cr++)
	{
		move(cr,2);
		clrtoeol();
		addstr(it->c_str() );
		//mvwaddstr( stdscr , cr  , 2  ,  it->c_str()  ) ;
	}
	//clrtobot() ;
	wrefresh(stdscr);
	pthread_mutex_unlock( & mMsgOutMutex ) ;
	return true ;
/*	

	pthread_mutex_lock( &mMsgOutMutex);
	move(mCurrMsgLine,1);
	clrtoeol();
	addstr(msg);
	refresh();
	pthread_mutex_unlock( &mMsgOutMutex);
	return true;
*/
}

bool ProgressBar::Progress(int no,const char * barstr)
{
	pthread_mutex_lock( &mMsgOutMutex);
	move(no+1,1);
	clrtoeol();
	addstr(barstr);	
	refresh();
	pthread_mutex_unlock(& mMsgOutMutex);
	return true;

}

void ProgressBar::CreateBarImage(char *barstr , long total , long got , long dltime )
{
	char *p = barstr ;
	int size = got ;
	char sizestr[24] ={ 0} ;
	
	char * size_legible = number_to_string( sizestr , size ) ;
	
	int size_legible_len = strlen (size_legible);

	/* The progress bar should look like this:
	     xx% [=======>             ] nn,nnn 12.34K/s ETA 00:00

	     Calculate the geometry.  The idea is to assign as much room as
	     possible to the progress bar.  The other idea is to never let
	     things "jitter", i.e. pad elements that vary in size so that
	     their variance does not affect the placement of other elements.
	     It would be especially bad for the progress bar to be resized
	     randomly.

	     "xx% " or "100%"  - percentage               - 4 chars
	     "[]"              - progress bar decorations - 2 chars
	     " nnn,nnn,nnn"    - downloaded bytes         - 12 chars or very rarely more
	     " 1012.56K/s"     - dl rate                  - 11 chars
	     " ETA xx:xx:xx"   - ETA                      - 13 chars

	     "=====>..."       - progress bar             - the rest
	*/
	int dlbytes_size = 1 + MAX (size_legible_len, 11);
	int progress_size = LINES -2 - (4 + 2 + dlbytes_size + 11 + 13);

	if (progress_size < 5)
		progress_size = 0;

	/* "xx% " */
	if (total > 0)
	{
		int percentage = (int)(100.0 * size / total );

		assert (percentage <= 100);

		if (percentage < 100)
			sprintf (p, "%2d%% ", percentage);
		else
			strcpy (p, "100%");
		p += 4;
	}
	else
		APPEND_LITERAL ("    ");

	/* The progress bar: "[====>      ]" or "[++==>      ]". */
	if (progress_size && total > 0)
	{
		/* Size of the initial portion. */
		int insz = (int)((double)got / total * progress_size ) ;

		/* Size of the downloaded portion. */
		int dlsz = (int)((double)size / total * progress_size) ;

		char *begin;
		int i;

		assert (dlsz <= progress_size);
		assert (insz <= dlsz);

		*p++ = '[';
		begin = p;

		/* Print the initial portion of the download with '+' chars, the rest with '=' and one '>'.  */
		for (i = 0; i < insz; i++)
			*p++ = '+';
		dlsz -= insz;
		if (dlsz > 0)
		{
			for (i = 0; i < dlsz - 1; i++)
				*p++ = '=';
			*p++ = '>';
		}

		while (p - begin < progress_size)
			*p++ = ' ';
		*p++ = ']';
	}
	else if (progress_size)
	{
		/* If we can't draw a real progress bar, then at least show
			*something* to the user.  */
		int ind = 70 % (progress_size * 2 - 6);
		int i, pos;

		/* Make the star move in two directions. */
		if (ind < progress_size - 2)
			pos = ind + 1;
		else
			pos = progress_size - (ind - progress_size + 5);

		*p++ = '[';
		for (i = 0; i < progress_size; i++)
		{
			if      (i == pos - 1) *p++ = '<';
			else if (i == pos    ) *p++ = '=';
			else if (i == pos + 1) *p++ = '>';
			else
				*p++ = ' ';
		}
		*p++ = ']';

		//++70;
	}

	/* " 234,567,890" */
	sprintf (p, " %-11s", with_thousand_seps (size));
	p += strlen (p);

	/* " 1012.45K/s" */
	if ( dltime  && total )
	{
		static const char *short_units[] = { "B/s", "K/s", "M/s", "G/s" };
		int units = 0;
		/* Calculate the download speed using the history ring and
			recent data that hasn't made it to the ring yet.  */
		int dlquant = total  + got ;
		double dltime = dltime ;
		double dlspeed = 11.11;//calc_rate (dlquant, dltime, &units);
		sprintf (p, " %7.2f%s", dlspeed, short_units[units]);
		p += strlen (p);
	}
	else
		APPEND_LITERAL ("   --.--K/s");

	/* " ETA xx:xx:xx"; wait for three seconds before displaying the ETA.
		That's because the ETA value needs a while to become
		reliable.  */
	if (total> 0   && dltime > 3000)
	{
		int eta;
		int eta_hrs, eta_min, eta_sec;

		/* Don't change the value of ETA more than approximately once
			per second; doing so would cause flashing without providing
			any value to the user. */
		if ( total  != size )
			eta = 50 ;
		else
		{
		  /* Calculate ETA using the average download speed to predict
		     the future speed.  If you want to use a speed averaged
		     over a more recent period, replace dl_total_time with
		     hist->total_time and bp->count with hist->total_bytes.
		     I found that doing that results in a very jerky and
		     ultimately unreliable ETA.  */
			double time_sofar = (double)dltime / 1000;
			int bytes_remaining = total - size;
			eta = (int) (time_sofar * bytes_remaining / 6);
		}

		eta_hrs = eta / 3600, eta %= 3600;
		eta_min = eta / 60,   eta %= 60;
		eta_sec = eta;

		if (eta_hrs > 99)
			goto no_eta;

		if (eta_hrs == 0)
		{
	  		/* Hours not printed: pad with three spaces. */
	  		APPEND_LITERAL ("   ");
			sprintf (p, " ETA %02d:%02d", eta_min, eta_sec);
		}
		else
		{
			if (eta_hrs < 10)
			/* Hours printed with one digit: pad with one space. */
				*p++ = ' ';
			sprintf (p, " ETA %d:%02d:%02d", eta_hrs, eta_min, eta_sec);
		}
    		p += strlen (p);
	}
	else if ( total  > 0)
	{
		no_eta:
		APPEND_LITERAL ("             ");
	}

	assert (p -  barstr <= LINES -2 );

	while ( p < barstr + LINES -2 )
		*p++ = ' ';
	*p = '\0';	
}

void ProgressBar::CreateBarImage(char *barstr , long total , long got , char * ratestr )
{
	char * p = barstr ;
	int  barsize = 50 ;
	int i = 0 ;
	int percentage = (int)(100.0 * got / total );
	percentage = percentage>100?100:percentage ;
	int eqlen = (int)(percentage * barsize/100) ;
	char tmp[30] = {0};
	
	*p = '[';p++;

	
	memset(p,'=',eqlen );
	p = p +  eqlen ;

	*p = '>';p++;
	memset(p,' ',barsize-eqlen  );
	p = p + barsize-eqlen  ;
	*p = ']';p++;

	for(i=0;i<3;i++)
	{
		*p = ' ';p++;
	}

	memset(tmp,0,sizeof(tmp));
	sprintf(tmp ,"%d/%d", got , total );

	for(i=0;i<strlen(tmp);i++,p++)
		*p = tmp[i] ;
		
	for(i=0;i<3;i++,p++)
		*p = ' ';
	for(i=0;i<strlen(ratestr);i++,p++)
		*p = ratestr[i] ;
		
	*p = '\0';
	
}

