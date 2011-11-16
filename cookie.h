#ifndef COOKIE_H
#define COOKIE_H

#include <time.h>

class Cookie
{
public :
	Cookie() ;
	~Cookie( ) ;


private:

	char *domain;			/* domain of the cookie */
	int port;			/* port number */
	char *path;			/* path prefix of the cookie */

	int secure;			/* whether cookie should be
				   transmitted over non-https
				   connections. */
	int domain_exact;		/* whether DOMAIN must match as a
				   whole. */

	int permanent;		/* whether the cookie should outlive
				   the session. */
	time_t expiry_time;		/* time when the cookie expires, 0
				   means undetermined. */

	int discard_requested;	/* whether cookie was created to
				   request discarding another
				   cookie. */

	char *attr;			/* cookie attribute name */
	char *value;			/* cookie attribute value */

	//  struct cookie *next;		
	/* used for chaining of cookies in the   same domain. */

}; 



#endif 

