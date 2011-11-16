

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mwget.h"
#include "options.h"
#include "safe-ctype.h"
#include "init.h"

#include "getopt.h"

#ifndef PATH_SEPARATOR
# define PATH_SEPARATOR '/'
#endif

struct options opt;
struct options kopt[100];


//////////////

struct cmdline_option option_data[] =
  {
    { "accept", 'A', OPT_VALUE, "accept", -1 },
    { "append-output", 'a', OPT__APPEND_OUTPUT, NULL, required_argument },
    { "background", 'b', OPT_BOOLEAN, "background", -1 },
    { "backup-converted", 'K', OPT_BOOLEAN, "backupconverted", -1 },
    { "backups", 0, OPT_BOOLEAN, "backups", -1 },
    { "base", 'B', OPT_VALUE, "base", -1 },
    { "bind-address", 0, OPT_VALUE, "bindaddress", -1 },
    { IF_SSL ("ca-certificate"), 0, OPT_VALUE, "cacertificate", -1 },
    { IF_SSL ("ca-directory"), 0, OPT_VALUE, "cadirectory", -1 },
    { "cache", 0, OPT_BOOLEAN, "cache", -1 },
    { IF_SSL ("certificate"), 0, OPT_VALUE, "certificate", -1 },
    { IF_SSL ("certificate-type"), 0, OPT_VALUE, "certificatetype", -1 },
    { IF_SSL ("check-certificate"), 0, OPT_BOOLEAN, "checkcertificate", -1 },
    { "clobber", 0, OPT__CLOBBER, NULL, optional_argument },
    { "connect-timeout", 0, OPT_VALUE, "connecttimeout", -1 },
    { "continue", 'c', OPT_BOOLEAN, "continue", -1 },
    { "convert-links", 'k', OPT_BOOLEAN, "convertlinks", -1 },
    { "cookies", 0, OPT_BOOLEAN, "cookies", -1 },
    { "cut-dirs", 0, OPT_VALUE, "cutdirs", -1 },
    { IF_DEBUG ("debug"), 'd', OPT_BOOLEAN, "debug", -1 },
    { "delete-after", 0, OPT_BOOLEAN, "deleteafter", -1 },
    { "directories", 0, OPT_BOOLEAN, "dirstruct", -1 },
    { "directory-prefix", 'P', OPT_VALUE, "dirprefix", -1 },
    { "dns-cache", 0, OPT_BOOLEAN, "dnscache", -1 },
    { "dns-timeout", 0, OPT_VALUE, "dnstimeout", -1 },
    { "domains", 'D', OPT_VALUE, "domains", -1 },
    { "dont-remove-listing", 0, OPT__DONT_REMOVE_LISTING, NULL, no_argument },
    { "dot-style", 0, OPT_VALUE, "dotstyle", -1 },
    { "egd-file", 0, OPT_VALUE, "egdfile", -1 },
    { "exclude-directories", 'X', OPT_VALUE, "excludedirectories", -1 },
    { "exclude-domains", 0, OPT_VALUE, "excludedomains", -1 },
    { "execute", 'e', OPT__EXECUTE, NULL, required_argument },
    { "follow-ftp", 0, OPT_BOOLEAN, "followftp", -1 },
    { "follow-tags", 0, OPT_VALUE, "followtags", -1 },
    { "force-directories", 'x', OPT_BOOLEAN, "dirstruct", -1 },
    { "force-html", 'F', OPT_BOOLEAN, "forcehtml", -1 },
    { "ftp-password", 0, OPT_VALUE, "ftppassword", -1 },
    { "ftp-user", 0, OPT_VALUE, "ftpuser", -1 },
    { "glob", 0, OPT_BOOLEAN, "glob", -1 },
    { "header", 0, OPT_VALUE, "header", -1 },
    { "help", 'h', OPT_FUNCALL, (void *)print_help, no_argument },
    { "host-directories", 0, OPT_BOOLEAN, "addhostdir", -1 },
    { "html-extension", 'E', OPT_BOOLEAN, "htmlextension", -1 },
    { "htmlify", 0, OPT_BOOLEAN, "htmlify", -1 },
    { "http-keep-alive", 0, OPT_BOOLEAN, "httpkeepalive", -1 },
    { "http-passwd", 0, OPT_VALUE, "httppassword", -1 }, /* deprecated */
    { "http-password", 0, OPT_VALUE, "httppassword", -1 },
    { "http-user", 0, OPT_VALUE, "httpuser", -1 },
    { "ignore-length", 0, OPT_BOOLEAN, "ignorelength", -1 },
    { "ignore-tags", 0, OPT_VALUE, "ignoretags", -1 },
    { "include-directories", 'I', OPT_VALUE, "includedirectories", -1 },
#ifdef ENABLE_IPV6
    { "inet4-only", '4', OPT_BOOLEAN, "inet4only", -1 },
    { "inet6-only", '6', OPT_BOOLEAN, "inet6only", -1 },
#endif
    { "input-file", 'i', OPT_VALUE, "input", -1 },
    { "keep-session-cookies", 0, OPT_BOOLEAN, "keepsessioncookies", -1 },
    { "level", 'l', OPT_VALUE, "reclevel", -1 },
    { "limit-rate", 0, OPT_VALUE, "limitrate", -1 },
    { "load-cookies", 0, OPT_VALUE, "loadcookies", -1 },
    { "mirror", 'm', OPT_BOOLEAN, "mirror", -1 },
    		{ "multi-thread", 'M', OPT_BOOLEAN, "multi-thread", -1 },    
    { "no", 'n', OPT__NO, NULL, required_argument },
    { "no-clobber", 0, OPT_BOOLEAN, "noclobber", -1 },
    { "no-parent", 0, OPT_BOOLEAN, "noparent", -1 },
    { "output-document", 'O', OPT_VALUE, "outputdocument", -1 },
    { "output-file", 'o', OPT_VALUE, "logfile", -1 },
    { "page-requisites", 'p', OPT_BOOLEAN, "pagerequisites", -1 },
    { "parent", 0, OPT__PARENT, NULL, optional_argument },
    { "passive-ftp", 0, OPT_BOOLEAN, "passiveftp", -1 },
    { "password", 0, OPT_VALUE, "password", -1 },
    { "post-data", 0, OPT_VALUE, "postdata", -1 },
    { "post-file", 0, OPT_VALUE, "postfile", -1 },
    { "prefer-family", 0, OPT_VALUE, "preferfamily", -1 },
    { "preserve-permissions", 0, OPT_BOOLEAN, "preservepermissions", -1 },
    { IF_SSL ("private-key"), 0, OPT_VALUE, "privatekey", -1 },
    { IF_SSL ("private-key-type"), 0, OPT_VALUE, "privatekeytype", -1 },
    { "progress", 0, OPT_VALUE, "progress", -1 },
    { "protocol-directories", 0, OPT_BOOLEAN, "protocoldirectories", -1 },
    { "proxy", 0, OPT_BOOLEAN, "useproxy", -1 },
    { "proxy__compat", 'Y', OPT_VALUE, "useproxy", -1 }, /* back-compatible */
    { "proxy-passwd", 0, OPT_VALUE, "proxypassword", -1 }, /* deprecated */
    { "proxy-password", 0, OPT_VALUE, "proxypassword", -1 },
    { "proxy-user", 0, OPT_VALUE, "proxyuser", -1 },
    { "quiet", 'q', OPT_BOOLEAN, "quiet", -1 },
    { "quota", 'Q', OPT_VALUE, "quota", -1 },
    { "random-file", 0, OPT_VALUE, "randomfile", -1 },
    { "random-wait", 0, OPT_BOOLEAN, "randomwait", -1 },
    { "read-timeout", 0, OPT_VALUE, "readtimeout", -1 },
    { "recursive", 'r', OPT_BOOLEAN, "recursive", -1 },
    { "referer", 0, OPT_VALUE, "referer", -1 },
    { "reject", 'R', OPT_VALUE, "reject", -1 },
    { "relative", 'L', OPT_BOOLEAN, "relativeonly", -1 },
    { "remove-listing", 0, OPT_BOOLEAN, "removelisting", -1 },
    { "restrict-file-names", 0, OPT_BOOLEAN, "restrictfilenames", -1 },
    { "retr-symlinks", 0, OPT_BOOLEAN, "retrsymlinks", -1 },
    { "retry-connrefused", 0, OPT_BOOLEAN, "retryconnrefused", -1 },
    { "save-cookies", 0, OPT_VALUE, "savecookies", -1 },
    { "save-headers", 0, OPT_BOOLEAN, "saveheaders", -1 },
    { IF_SSL ("secure-protocol"), 0, OPT_VALUE, "secureprotocol", -1 },
    { "server-response", 'S', OPT_BOOLEAN, "serverresponse", -1 },
    { "span-hosts", 'H', OPT_BOOLEAN, "spanhosts", -1 },
    { "spider", 0, OPT_BOOLEAN, "spider", -1 },
    { "strict-comments", 0, OPT_BOOLEAN, "strictcomments", -1 },
   		 { "thread-count", 'C', OPT_VALUE, "thread-count", -1 },
    { "timeout", 'T', OPT_VALUE, "timeout", -1 },
    { "timestamping", 'N', OPT_BOOLEAN, "timestamping", -1 },
    { "tries", 't', OPT_VALUE, "tries", -1 },
    { "user", 0, OPT_VALUE, "user", -1 },
    { "user-agent", 'U', OPT_VALUE, "useragent", -1 },
    { "verbose", 'v', OPT_BOOLEAN, "verbose", -1 },
    { "verbose", 0, OPT_BOOLEAN, "verbose", -1 },
    { "version", 'V', OPT_FUNCALL, (void *) print_version, no_argument },
    { "wait", 'w', OPT_VALUE, "wait", -1 },
    { "waitretry", 0, OPT_VALUE, "waitretry", -1 },
  };


/* Return a string that contains S with "no-" prepended.  The string
   is NUL-terminated and allocated off static storage at Wget
   startup.  */

static char *
no_prefix (const char *s)
{
  static char buffer[1024];
  static char *p = buffer;

  char *cp = p;
  int size = 3 + strlen (s) + 1;  /* "no-STRING\0" */
  if (p + size >= buffer + sizeof (buffer))
    abort ();

  cp[0] = 'n', cp[1] = 'o', cp[2] = '-';
  strcpy (cp + 3, s);
  p += size;
  return cp;
}



/* The arguments that that main passes to getopt_long. */
static struct option long_options[2 * countof (option_data) + 1];
static char short_options[128];

/* Mapping between short option chars and option_data indices. */
static unsigned char optmap[96];

/* Marker for `--no-FOO' values in long_options.  */
#define BOOLEAN_NEG_MARKER 1024


/* Initialize the long_options array used by getopt_long from the data
   in option_data.  */

static void
init_switches (void)
{
  char *p = short_options;
  int i, o = 0;
  for (i = 0; i < countof (option_data); i++)
    {
      struct cmdline_option *opt = &option_data[i];
      struct option *longopt;

      if (!opt->long_name)
	/* The option is disabled. */
	continue;

      longopt = &long_options[o++];
      longopt->name = opt->long_name;
      longopt->val = i;
      if (opt->short_name)
	{
	  *p++ = opt->short_name;
	  optmap[opt->short_name - 32] = longopt - long_options;
	}
      switch (opt->type)
	{
	case OPT_VALUE:
	  longopt->has_arg = required_argument;
          if (opt->short_name)
	    *p++ = ':';
	  break;
	case OPT_BOOLEAN:
	  /* Specify an optional argument for long options, so that
	     --option=off works the same as --no-option, for
	     compatibility with pre-1.10 Wget.  However, don't specify
	     optional arguments short-option booleans because they
	     prevent combining of short options.  */
	  longopt->has_arg = optional_argument;
	  /* For Boolean options, add the "--no-FOO" variant, which is
	     identical to "--foo", except it has opposite meaning and
	     it doesn't allow an argument.  */
	  longopt = &long_options[o++];
	  longopt->name = no_prefix (opt->long_name);
	  longopt->has_arg = no_argument;
	  /* Mask the value so we'll be able to recognize that we're
	     dealing with the false value.  */
	  longopt->val = i | BOOLEAN_NEG_MARKER;
	  break;
	default:
	  assert (opt->argtype != -1);
	  longopt->has_arg = opt->argtype;
	  if (opt->short_name)
	    {
	      if (longopt->has_arg == required_argument)
		*p++ = ':';
	      /* Don't handle optional_argument */
	    }
	}
    }
  /* Terminate short_options. */
  *p = '\0';
  /* No need for xzero(long_options[o]) because its storage is static
     and it will be zeroed by default.  */
  assert (o <= countof (long_options));
}





///////////////
XOptions * XOptions::mOpt = 0 ;
XOptions::XOptions ( int argc , char ** argv  )
{

	if( argc <= 0 || argv == 0 )
	{
		this->mValid = false ;
		return ;
	}
	
	this->mArgc = argc ;
	this->mArgv = (char **) malloc( sizeof ( char*) * this->mArgc );
	memset( this->mArgv , 0 , sizeof( char *) * this->mArgc );

	for( int i = 0 ; i < this->mArgc ; i ++ )
	{
		this->mArgv[i] = ( char*) malloc ( strlen( argv[i] ) +1 );
		memset( this->mArgv[i] , 0 ,  strlen( argv[i] ) +1  ) ;
		strcpy(  this->mArgv[i] ,  argv[i] ) ;
	}

	
	if( ! this->Parse( ) )
	{
		this->mValid = false ;
	}
	else
	{
		this->mValid = true ;
	}
	
	
}

XOptions::~XOptions ( )
{

}
bool XOptions::DestoryOptions( ) 
{

	return true ;
}


XOptions * XOptions::Instance (  int argc , char ** argv   ) 
{
	if ( XOptions::mOpt == 0 )
	{
		XOptions::mOpt = new XOptions( argc , argv );
	}
	return XOptions::mOpt ;
}
	

bool XOptions::IsValid() 
{
	return this->mValid ;
	return true ;
}

bool XOptions::Parse()
{
	
	
	return true ;
}


void XOptions::DumpOptions() 
{


}

 //exe info
extern const char *exec_name;
extern char *version_string  ;

void test_options( int argc, char ** argv )
{
	int i, ret, longindex;
	
	int append_to_log = 0;

	// set default option value
	initialize ( ) ;
	
	init_switches ();

	longindex = -1;
	while ((ret = getopt_long (argc, argv,
				 short_options, long_options, &longindex)) != -1)
	{
	  int val;
	  struct cmdline_option *opt;

	  /* If LONGINDEX is unchanged, it means RET is referring a short
	 option.  */
	  if (longindex == -1)
	{
	  if (ret == '?')
		{
		  print_usage ();
		  printf ("\n");
		  printf ("Try `%s --help' for more options.\n", exec_name);
		  exit (2);
		}
	  /* Find the short option character in the mapping.  */
	  longindex = optmap[ret - 32];
	}
	  val = long_options[longindex].val;

	  /* Use the retrieved value to locate the option in the
	 option_data array, and to see if we're dealing with the
	 negated "--no-FOO" variant of the boolean option "--foo".	*/
	  opt = &option_data[val & ~BOOLEAN_NEG_MARKER];
	  switch (opt->type)
	{
	case OPT_VALUE:
		fprintf(stderr , "%s==%s==%s\n" , opt->data,optarg , opt->long_name ) ;
	  setoptval ((const char*)opt->data, optarg, opt->long_name);
	  break;
	case OPT_BOOLEAN:
	  if (optarg)
		/* The user has specified a value -- use it. */
		setoptval ((const char*)opt->data, optarg, opt->long_name);
	  else
		{
		  /* NEG is true for `--no-FOO' style boolean options. */
		  int neg = val & BOOLEAN_NEG_MARKER;
		  setoptval ((const char*)opt->data, neg ? "0" : "1", opt->long_name);
		}
	  break;
	case OPT_FUNCALL:
	  {
		void (*func) (void)  = (void (*)() ) opt->data;
		func () ;
	  }
	  break;
	case OPT__APPEND_OUTPUT:
	  setoptval ("logfile", optarg, opt->long_name);
	  append_to_log = 1;
	  break;
	case OPT__EXECUTE:
	  run_command (optarg);
	  break;
	case OPT__NO:
	  {
		/* We support real --no-FOO flags now, but keep these
		   short options for convenience and backward
		   compatibility.  */
		char *p;
		for (p = optarg; *p; p++)
		  switch (*p)
		{
		case 'v':
		  setoptval ("verbose", "0", opt->long_name);
		  break;
		case 'H':
		  setoptval ("addhostdir", "0", opt->long_name);
		  break;
		case 'd':
		  setoptval ("dirstruct", "0", opt->long_name);
		  break;
		case 'c':
		  setoptval ("noclobber", "1", opt->long_name);
		  break;
		case 'p':
		  setoptval ("noparent", "1", opt->long_name);
		  break;
		default:
		  printf ("%s: illegal option -- `-n%c'\n", exec_name, *p);
		  print_usage ();
		  printf ("\n");
		  printf ("Try `%s --help' for more options.\n", exec_name);
		  exit (1);
		}
		break;
	  }
	case OPT__PARENT:
	case OPT__CLOBBER:
	  {
		/* The wgetrc commands are named noparent and noclobber,
		   so we must revert the meaning of the cmdline options
		   before passing the value to setoptval.  */
		int flag = 1;
		if (optarg)
		  flag = (*optarg == '1' || TOLOWER (*optarg) == 'y'
			  || (TOLOWER (optarg[0]) == 'o'
			  && TOLOWER (optarg[1]) == 'n'));
		setoptval (opt->type == OPT__PARENT ? "noparent" : "noclobber",
			   flag ? "0" : "1", opt->long_name);
		break;
	  }
	case OPT__DONT_REMOVE_LISTING:
	  setoptval ("removelisting", "0", opt->long_name);
	  break;
	}

	  longindex = -1;
	}

	
}


