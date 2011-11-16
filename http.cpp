#include <cassert>
#include <string.h>
#include <errno.h>


#include "url.h"
#include "http.h"

bool http_file_valid( KSocket *csock ,URL * furl , long * size  ) 
{
	bool dret ;
	char rbuff[1024] = {0};
	char *pos = rbuff ;
	char *s = 0 ;
		HttpHeader header;
		header.AddHeader( "GET", furl->GetPath());
		header.AddHeader( "Host", furl->GetHost());
		header.AddHeader( "Connection", "Close");

		char range[50] = {0};
		sprintf(range,"bytes=%ld-", *size);
		header.AddHeader( "Range",range);
		std::string hs = header.HeaderString() ;
		csock->Write(hs.c_str(), hs.length());
		
		int rlen = 0 ;		
		do
		{
			rlen = csock->Peek(rbuff + rlen  , sizeof(rbuff) );
			if( ( s = strstr(rbuff ,"\r\n\r\n") ) != 0 )
			{
				*size = s - rbuff + 4 ;
				break ;
			}
			
		}while(true);
		
		header.ClearHeaders();
		header.Parse(rbuff);

		if( header.GetHeader( "Status" ) .compare("200") < 0 
			|| header.GetHeader( "Status" ) .compare("300") >= 0 ) 
		{
			fprintf(stderr,"Header status (%d)  \n",header.GetHeader( "Status" ).c_str() );
			*size = 0 ;
			return false ;
		}

		//read head now , really data 
		memset(rbuff,0,sizeof(rbuff));
		csock->Read(rbuff , *size ) ;
		//fprintf(stderr,"Header (%ld)  is %s \n", *size ,rbuff);

		*size = atol(header.GetHeader( "Content-Length" ).c_str());
		
	dret = true ;
	
	return dret ; 
	
}

/*
		HttpHeader header;
		header.AddHeader( "GET", turl->GetPath());
		header.AddHeader( "Host", turl->GetHost());
		header.AddHeader( "Connection", "Close");
		if(threadno > 0 )
		{
			char range[50] = {0};
			sprintf(range,"bytes=%ld-",tp->m_begin_position + tp->m_got_size );
			header.AddHeader( "Range",range);
		}
		else if ( threadno == 0 )
		{
			char range[50] = {0};
			sprintf(range,"bytes=%ld-",tp->m_begin_position + tp->m_got_size );
			header.AddHeader( "Range",range);
		}
		
		//header.Dump();
		std::string hs = header.HeaderString() ;
		fprintf(stderr,"%s", hs.c_str());
		sock->Write(hs.c_str(), hs.length());
		// there we connect the server,try retriving the file content
		int hd_rlen = sock->Read(rbuff, sizeof(rbuff)  );
		//int sockfd = sock->GetSocket();
		//int hd_rlen = read(sockfd,rbuff,sizeof(rbuff));
		
		header.ClearHeaders();
		header.Parse(rbuff);
		//header.Dump();
		//fprintf(stderr, "\nLenth:%d \n%s\n",header.GetHeaderLength(),rbuff);



		//handle read from header data
		int hd_len = header.GetHeaderLength();
		memcpy(dbuff , rbuff + hd_len , hd_rlen - hd_len);
		
		fprintf(stderr , "Left Data Len : %d \n",hd_rlen - hd_len);
		int wfp = open("mtv.wmv",O_WRONLY|O_CREAT,0644);
		lseek(wfp,tp->m_begin_position + tp->m_got_size , SEEK_SET);
		write(wfp,dbuff,hd_rlen - hd_len);
		//write(wfp,rbuff,hd_rlen );
		tp->m_got_size += hd_rlen - hd_len ;


*/


HttpRetriever::HttpRetriever(  int ts , const char * url )
	: Retriever( ts )
{
	//fprintf(stderr,"HttpRetriever::HttpRetriever\n");
	this->mUrl->Parse( url ) ;
	
}

HttpRetriever::~HttpRetriever( )
{
	
}

bool HttpRetriever::TaskValid(  long bsize    )
{
	KSocket *csock = this->mCtrlSock ;
	KSocket *dsock = this->mTranSock ;
	
	URL * furl = this->mUrl ;
	
	long * size = & bsize ;
	
	bool dret ;
	char rbuff[1024] = {0};
	char *pos = rbuff ;
	char *s = 0 ;
	char range[50] = {0};
	
		HttpHeader header;

		//fprintf(stderr , " GET %s \n" ,furl->GetPath());
		header.AddHeader( "GET", furl->GetFullPath() );
		header.AddHeader( "Host", furl->GetHost());
		header.AddHeader( "Connection", "Close");

		//header.Dump() ;
		
		sprintf(range,"bytes=%ld-", *size);
		header.AddHeader( "Range",range);
		std::string hs = header.HeaderString() ;
		csock->Write(hs.c_str(), hs.length());		

		fprintf(stderr , "%s" , hs.c_str());
		
		int rlen = 0 ;		
		do
		{
			
			rlen = csock->Peek(rbuff + rlen  , sizeof(rbuff) );
			fprintf(stderr, "Peeking %s \n" , rbuff );
			if( rlen <= 0 ) break ;
			if( ( s = strstr(rbuff ,"\r\n\r\n") ) != 0 )
			{
				*size = s - rbuff + 4 ;
				break ;
			}
			
		}while(true);
		
		header.ClearHeaders();
		header.Parse(rbuff);

		//header.Dump() ;
		std::string status = header.GetHeader( "Status" ) ;
		
		if ( status.compare("400") < 0 && status.compare("300") >= 0  ) 
		{
			//if follow into the redirector url
			//rewrite url
			//now this function lack test, i think i cant work correct
			fprintf(stderr , "400 Redirect Signal found\n");
			return false ;
		}
		else if ( status.compare("300") < 0 && status.compare("200") >= 0  ) 
		{
			//normal response
		}
		else //if( status.compare("200") < 0 || status.compare("400") >= 0 ) 
		{
			fprintf(stderr,"Header status (%s)  \n",header.GetHeader( "Status" ).c_str() );

			*size = 0 ;
			return false ;
		}		

		//read head now , really data 
		memset(rbuff,0,sizeof(rbuff));
		csock->Read(rbuff , *size ) ;
		//fprintf(stderr,"Header (%ld)  is %s \n", *size ,rbuff);

				
		*size = atol(header.GetHeader( "Content-Length" ).c_str());
	this->mContentLength = * size ;
		
	dsock->SetSocket(csock) ;
		
	dret = true ;
	
	return dret ; 
	return false ;
}


KSocket * HttpRetriever::GetDataSocket()
{
	return this->mCtrlSock ;
}


//========================
HttpsRetriever::HttpsRetriever( int ts , const char * url ) 
	: Retriever( ts ) 
{
	
}
HttpsRetriever::~HttpsRetriever() 
{
}

bool HttpsRetriever::TaskValid(long bsize )
{

	return false ;
}

KSocket * HttpsRetriever::GetDataSocket() 
{

}
	

int HttpsRetriever::ssl_init() 
{


	return -1 ;
}

/* Perform the SSL handshake on file descriptor FD, which is assumed
   to be connected to an SSL server.  The SSL handle provided by
   OpenSSL is registered with the file descriptor FD using
   fd_register_transport, so that subsequent calls to fd_read,
   fd_write, etc., will use the corresponding SSL functions.

   Returns 1 on success, 0 on failure.  */
int HttpsRetriever::ssl_connect(int fd )
{
	  SSL *ssl;

	  DEBUGP (("Initiating SSL handshake.\n"));

	  assert (ssl_ctx != NULL);
	  ssl = SSL_new (ssl_ctx);
	  if (!ssl)
	    goto error;
	  if (!SSL_set_fd (ssl, fd))
	    goto error;
	  SSL_set_connect_state (ssl);
	  if (SSL_connect (ssl) <= 0 || ssl->state != SSL_ST_OK)
	    goto error;

	  /* Register FD with Wget's transport layer, i.e. arrange that our
	     functions are used for reading, writing, and polling.  */
	     
	  //fd_register_transport (fd, openssl_read, openssl_write, openssl_poll,
		//			 openssl_peek, openssl_close, ssl);
		
		//DEBUGP (("Handshake successful; connected socket %d to SSL handle 0x%0*lx\n",
		   //fd, PTR_FORMAT (ssl)));
	  return 1;

	 error:
	  DEBUGP (("SSL handshake failed.\n"));
	  print_errors ();
	  if (ssl)
	    SSL_free (ssl);
	  return 0;
	return  -1 ;
}

#define ASTERISK_EXCLUDES_DOT	/* mandated by rfc2818 */


	/* Verify the validity of the certificate presented by the server.
	   Also check that the "common name" of the server, as presented by
	   its certificate, corresponds to HOST.  (HOST typically comes from
	   the URL and is what the user thinks he's connecting to.)

	   This assumes that ssl_connect has successfully finished, i.e. that
	   the SSL handshake has been performed and that FD is connected to an
	   SSL handle.

	   If opt.check_cert is non-zero (the default), this returns 1 if the
	   certificate is valid, 0 otherwise.  If opt.check_cert is 0, the
	   function always returns 1, but should still be called because it
	   warns the user about any problems with the certificate.  */

int HttpsRetriever::ssl_check_certificate (int fd  , const char  *host)
{
	  X509 *cert;
	  char common_name[256];
	  long vresult;
	  int success = 1;

	  /* If the user has specified --no-check-cert, we still want to warn
	     him about problems with the server's certificate.  */
	  const char *severity = opt.check_cert ? "ERROR" : "WARNING" ;

	SSL *ssl ;
	 // SSL *ssl = (SSL *) fd_transport_context (fd);

	  assert (ssl != NULL);

	  cert = SSL_get_peer_certificate (ssl);
	  if (!cert)
	    {
	     // logprintf (LOG_NOTQUIET, "%s: No certificate presented by %s.\n",
		//	 severity,  (host));
		fprintf(stderr , "%s: No certificate presented by %s.\n",severity,  (host));
	      success = 0;
	      goto no_cert;		/* must bail out since CERT is NULL */
	    }

#ifdef ENABLE_DEBUG
	  if (opt.debug)
	    {
	      char *subject = X509_NAME_oneline (X509_get_subject_name (cert), 0, 0);
	      char *issuer = X509_NAME_oneline (X509_get_issuer_name (cert), 0, 0);
	      DEBUGP (("certificate:\n  subject: %s\n  issuer:  %s\n",
		        (subject),  (issuer)));
	      OPENSSL_free (subject);
	      OPENSSL_free (issuer);
	    }
#endif

	  vresult = SSL_get_verify_result (ssl);
	  if (vresult != X509_V_OK)
	    {
	      /* #### We might want to print saner (and translatable) error
		 messages for several frequently encountered errors.  The
		 candidates would include
		 X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY,
		 X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN,
		 X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT,
		 X509_V_ERR_CERT_NOT_YET_VALID, X509_V_ERR_CERT_HAS_EXPIRED,
		 and possibly others.  The current approach would still be
		 used for the less frequent failure cases.  */

	     // logprintf (LOG_NOTQUIET,
		//	 _("%s: Certificate verification error for %s: %s\n"),
		//	 severity, escnonprint (host),
		//	 X509_verify_cert_error_string (vresult));

		fprintf(stderr , "%s: Certificate verification error for %s: %s\n" ,
			severity,  (host) , X509_verify_cert_error_string (vresult));
	      success = 0;
	      /* Fall through, so that the user is warned about *all* issues
		 with the cert (important with --no-check-certificate.)  */
	    }

	  /* Check that HOST matches the common name in the certificate.
	     #### The following remains to be done:

	     - It should use dNSName/ipAddress subjectAltName extensions if
	       available; according to rfc2818: "If a subjectAltName extension
	       of type dNSName is present, that MUST be used as the identity."

	     - When matching against common names, it should loop over all
	       common names and choose the most specific one, i.e. the last
	       one, not the first one, which the current code picks.

	     - Ensure that ASN1 strings from the certificate are encoded as
	       UTF-8 which can be meaningfully compared to HOST.  */

	  common_name[0] = '\0';
	  X509_NAME_get_text_by_NID (X509_get_subject_name (cert),
				     NID_commonName, common_name, sizeof (common_name));
	  if (!pattern_match (common_name, host))
	    {
	      //logprintf (LOG_NOTQUIET, _("\
			//%s: certificate common name `%s' doesn't match requested host name `%s'.\n"),
			// severity, escnonprint (common_name), escnonprint (host));
		fprintf(stderr , "%s: certificate common name `%s' doesn't match requested host name `%s'.\n" , 
			severity,  (common_name),  (host));
			
	      success = 0;
	    }

	  if (success)
	    DEBUGP (("X509 certificate successfully verified and matches host %s\n",
		      (host)));
	  X509_free (cert);

	 no_cert:
	  if (opt.check_cert && !success)
	   // logprintf (LOG_NOTQUIET, _("\
		//To connect to %s insecurely, use `--no-check-certificate'.\n"),
		   //    escnonprint (host));
		fprintf(stderr , "To connect to %s insecurely, use `--no-check-certificate'.\n" ,
			 (host));

	  /* Allow --no-check-cert to disable certificate checking. */
	  return opt.check_cert ? success : 1;

	return  -1 ;
}


/* Initialize the SSL's PRNG using various methods. */

void	HttpsRetriever::init_prng (void)
{
	  char namebuf[256];
	  const char *random_file;

	  if (RAND_status ())
	    /* The PRNG has been seeded; no further action is necessary. */
	    return;

	  /* Seed from a file specified by the user.  This will be the file
	     specified with --random-file, $RANDFILE, if set, or ~/.rnd, if it
	     exists.  */
	 // if (opt.random_file)
	 //   random_file = opt.random_file;
	//  else
	    {
	      /* Get the random file name using RAND_file_name. */
	      namebuf[0] = '\0';
	      random_file = RAND_file_name (namebuf, sizeof (namebuf));
	    }

	  if (random_file && *random_file)
	    /* Seed at most 16k (apparently arbitrary value borrowed from
	       curl) from random file. */
	    RAND_load_file (random_file, 16384);

	  if (RAND_status ())
	    return;

	  /* Get random data from EGD if opt.egd_file was used.  */
	  //if (opt.egd_file && *opt.egd_file)
	  //  RAND_egd (opt.egd_file);

	  if (RAND_status ())
	    return;

#ifdef WINDOWS
	  /* Under Windows, we can try to seed the PRNG using screen content.
	     This may or may not work, depending on whether we'll calling Wget
	     interactively.  */

	  RAND_screen ();
	  if (RAND_status ())
	    return;
#endif

}

/* Print errors in the OpenSSL error stack. */
void HttpsRetriever::print_errors (void) 
{
  unsigned long curerr = 0;
  while ((curerr = ERR_get_error ()) != 0)
   {
   	fprintf(stderr, "OpenSSL: %s\n", ERR_error_string (curerr, NULL));
   	//logprintf (LOG_NOTQUIET, "OpenSSL: %s\n", ERR_error_string (curerr, NULL));
   }
}

/* Convert keyfile type as used by options.h to a type as accepted by
   SSL_CTX_use_certificate_file and SSL_CTX_use_PrivateKey_file.

   (options.h intentionally doesn't use values from openssl/ssl.h so
   it doesn't depend specifically on OpenSSL for SSL functionality.)  */
int	HttpsRetriever::key_type_to_ssl_type ( int  type)
{
  switch (type)
    {
	//case keyfile_pem:
    	case 0 :
	      return SSL_FILETYPE_PEM;
	//case keyfile_asn1:
		return SSL_FILETYPE_ASN1;
	default:
		abort ();
    }
}


int	HttpsRetriever::openssl_read (int fd, char *buf, int bufsize, void *ctx)
{
	  int ret;
	  SSL *ssl = (SSL *) ctx;
	  do
	    ret = SSL_read (ssl, buf, bufsize);
	  while (ret == -1
		 && SSL_get_error (ssl, ret) == SSL_ERROR_SYSCALL
		 && errno == EINTR);
	  return ret;
}

int	HttpsRetriever::openssl_write (int fd, char *buf, int bufsize, void *ctx)
{
	  int ret = 0;
	  SSL *ssl = (SSL *) ctx;
	  do
	    ret = SSL_write (ssl, buf, bufsize);
	  while (ret == -1
		 && SSL_get_error (ssl, ret) == SSL_ERROR_SYSCALL
		 && errno == EINTR);
	  return ret;
}


int	HttpsRetriever::openssl_poll (int fd, double timeout, int wait_for, void *ctx)
{
	  SSL *ssl = (SSL *) ctx;
	  if (timeout == 0)
	    return 1;
	  if (SSL_pending (ssl))
	    return 1;
	 // return select_fd (fd, timeout, wait_for);
}

int	HttpsRetriever::openssl_peek (int fd, char *buf, int bufsize, void *ctx)
{
	  int ret;
	  SSL *ssl = (SSL *) ctx;
	  do
	    ret = SSL_peek (ssl, buf, bufsize);
	  while (ret == -1
		 && SSL_get_error (ssl, ret) == SSL_ERROR_SYSCALL
		 && errno == EINTR);
	  return ret;
}

void	HttpsRetriever::openssl_close (int fd, void *ctx)
{
	  SSL *ssl = (SSL *) ctx;
	  SSL_shutdown (ssl);
	  SSL_free (ssl);

#ifdef WINDOWS
	  closesocket (fd);
#else
	  close (fd);
#endif

  	DEBUGP (("Closed %d/SSL 0x%0lx\n", fd , (const char*) ssl));
}

/* Return 1 is STRING (case-insensitively) matches PATTERN, 0
   otherwise.  The recognized wildcard character is "*", which matches
   any character in STRING except ".".  Any number of the "*" wildcard
   may be present in the pattern.

   This is used to match of hosts as indicated in rfc2818: "Names may
   contain the wildcard character * which is considered to match any
   single domain name component or component fragment. E.g., *.a.com
   matches foo.a.com but not bar.foo.a.com. f*.com matches foo.com but
   not bar.com [or foo.bar.com]."

   If the pattern contain no wildcards, pattern_match(a, b) is
   equivalent to !strcasecmp(a, b).  */

int	HttpsRetriever::pattern_match (const char *pattern, const char *string)
{

	  const char *p = pattern, *n = string;
	  char c;
	  for (; (c = tolower(*p++)) != '\0'; n++)
	    if (c == '*')
	      {
		for (c = tolower (*p); c == '*'; c = tolower (*++p))
		  ;
		for (; *n != '\0'; n++)
		  if (tolower (*n) == c && pattern_match (p, n))
		    return 1;
#ifdef ASTERISK_EXCLUDES_DOT
		  else if (*n == '.')
		    return 0;
#endif
		return c == '\0';
	      }
	    else
	      {
		if (c != tolower (*n))
		  return 0;
	      }
	  return *n == '\0';
}




