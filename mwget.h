

#ifndef MWGET_H
#define MWGET_H

#include <sys/time.h>


enum {T_BEGIN,T_READY,T_RETRIVE,T_CLOSE,T_ERROR,T_STOP,T_END} ;

enum {E_BEGIN = -1, E_FAILD, E_OK , E_REDIRECT , E_END } ;

struct thread_params
{
	int m_thread_no;		//
	int m_thread_count;
	
	long m_total_size ;
	long m_got_size ;
	long m_begin_position;	//
	
	char m_file_url [255] ;
	struct timeval m_begin_time;		//use gettimeofday get milisecond value
	struct timeval m_last_time;

	int chunk_bytes ;
	struct timeval chunk_start ;
	double sleep_adjust ;


	//
	int m_state;	//ready,retrive,close,error
	
};


//help list
void print_help( void ) ;
void print_usage ( void );
void print_version (void);


//WDEBUG

#define DEBUGP( x  ... )  fprintf(  stderr ,  "\n"  ) ;fprintf(  stderr ,  x   ) ;fprintf(  stderr ,  "\n"  ) ;

//#define DEBUGP( x  ... )   ;


#ifndef PARAMS
# if PROTOTYPES
#  define PARAMS(args) args
# else
#  define PARAMS(args) ()
# endif
#endif

/* The log file to which Wget writes to after HUP.  */
#define DEFAULT_LOGFILE "wget-log"

enum log_options { LOG_VERBOSE, LOG_NOTQUIET, LOG_NONVERBOSE, LOG_ALWAYS };


/* The number of elements in an array.  For example:
   static char a[] = "foo";     -- countof(a) == 4 (note terminating \0)
   int a[5] = {1, 2};           -- countof(a) == 5
   char *a[] = {                -- countof(a) == 3
     "foo", "bar", "baz"
   }; */
#define countof(array) (sizeof (array) / sizeof ((array)[0]))


/* Like ptr=strdup(str), but allocates the space for PTR on the stack.
   This cannot be an expression because this is not portable:
     #define STRDUP_ALLOCA(str) (strcpy (alloca (strlen (str) + 1), str))
   The problem is that some compilers can't handle alloca() being an
   argument to a function.  */

#define STRDUP_ALLOCA(ptr, str) do {			\
  char **SA_dest = &(ptr);				\
  const char *SA_src = (str);				\
  *SA_dest = (char *)alloca (strlen (SA_src) + 1);	\
  strcpy (*SA_dest, SA_src);				\
} while (0)

/* Zero out a value.  */
#define xzero(x) memset (&(x), '\0', sizeof (x))

/* Free P if it is non-NULL.  C requires free() to behaves this way by
   default, but Wget's code is historically careful not to pass NULL
   to free.  This allows us to assert p!=NULL in xfree to check
   additional errors.  (But we currently don't do that!)  */
#define free_null(p) if (!(p)) ; else free (p)

/* Copy the data delimited with BEG and END to alloca-allocated
   storage, and zero-terminate it.  Arguments are evaluated only once,
   in the order BEG, END, PLACE.  */
#define BOUNDED_TO_ALLOCA(beg, end, place) do {	\
  const char *BTA_beg = (beg);			\
  int BTA_len = (end) - BTA_beg;		\
  char **BTA_dest = &(place);			\
  *BTA_dest = alloca (BTA_len + 1);		\
  memcpy (*BTA_dest, BTA_beg, BTA_len);		\
  (*BTA_dest)[BTA_len] = '\0';			\
} while (0)


/* Return non-zero if string bounded between BEG and END is equal to
   STRING_LITERAL.  The comparison is case-sensitive.  */
#define BOUNDED_EQUAL(beg, end, string_literal)				\
  ((end) - (beg) == sizeof (string_literal) - 1				\
   && !memcmp (beg, string_literal, sizeof (string_literal) - 1))

#endif

