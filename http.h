
#ifndef HTTP_H
#define HTTP_H

#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/rand.h>


#include "ksocket.h"
#include "headers.h"
#include "retrieve.h"
#include "options.h"

class HttpRetriever : public Retriever
{
public:
	HttpRetriever( int ts ,  const char * url ) ;
	~HttpRetriever() ;
	
	bool TaskValid( long bsize = 0 ) ;
	KSocket * GetDataSocket();
		
private:

};


class HttpsRetriever : public Retriever
{
public:
	HttpsRetriever( int ts , const char * url ) ;
	~HttpsRetriever() ;

	bool TaskValid(long bsize = 0) ;

	KSocket * GetDataSocket() ;
	
private:
	/* Create an SSL Context and set default paths etc.  Called the first
	   time an HTTP download is attempted.

	   Returns 1 on success, 0 otherwise.  */
	int ssl_init() ;

	/* Perform the SSL handshake on file descriptor FD, which is assumed
	   to be connected to an SSL server.  The SSL handle provided by
	   OpenSSL is registered with the file descriptor FD using
	   fd_register_transport, so that subsequent calls to fd_read,
	   fd_write, etc., will use the corresponding SSL functions.

	   Returns 1 on success, 0 on failure.  */
	int ssl_connect(int );
	
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
	int ssl_check_certificate (int , const char *);

	/* Initialize the SSL's PRNG using various methods. */
	void init_prng( ) ;

	/* Print errors in the OpenSSL error stack. */
	void print_errors( ) ;
	
	/* Convert keyfile type as used by options.h to a type as accepted by
   	SSL_CTX_use_certificate_file and SSL_CTX_use_PrivateKey_file.

   	(options.h intentionally doesn't use values from openssl/ssl.h so
  	 it doesn't depend specifically on OpenSSL for SSL functionality.)  */

	//int key_type_to_ssl_type (enum keyfile_type type) ;
	int key_type_to_ssl_type ( int  type) ;

	int	openssl_read (int fd, char *buf, int bufsize, void *ctx) ;

	int 	openssl_write (int fd, char *buf, int bufsize, void *ctx) ;

	int	openssl_poll (int fd, double timeout, int wait_for, void *ctx) ;

	int	openssl_peek (int fd, char *buf, int bufsize, void *ctx) ;

	void	openssl_close (int fd, void *ctx) ;

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
	int	pattern_match (const char *pattern, const char *string) ;

	////////////////
	/* Application-wide SSL context.  This is common to all SSL
	   connections.  */	
	SSL_CTX *ssl_ctx;

	
};

#ifdef __cplusplus
extern "C"{
#endif

bool http_file_valid(KSocket *csock ,URL * furl , long * size) ;


#ifdef __cplusplus
}
#endif


#endif


