
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>


#include "url.h"

#define ecalloc calloc
#define estrndup strndup
#define efree free
#define STR_FREE(x) \
	free(x);	\
	x = 0 ;
#define safe_emalloc(nmemb, size, offset)		malloc((nmemb) * (size) + (offset))

static unsigned char hexchars[] = "0123456789ABCDEF";

//internal function by this model
php_url *php_url_parse_ex(char const *str, int length);
char *php_replace_controlchars_ex(char *str, int len);

static int php_htoi(char *s)
{
	int value;
	int c;

	c = ((unsigned char *)s)[0];
	if (isupper(c))
		c = tolower(c);
	value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

	c = ((unsigned char *)s)[1];
	if (isupper(c))
		c = tolower(c);
	value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

	return (value);
}


int php_raw_url_decode(char *str, int len)
{
	char *dest = str;
	char *data = str;

	while (len--) {
		if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) 
			&& isxdigit((int) *(data + 2))) {
#ifndef CHARSET_EBCDIC
			*dest = (char) php_htoi(data + 1);
#else
			*dest = os_toebcdic[(char) php_htoi(data + 1)];
#endif
			data += 2;
			len -= 2;
		} else {
			*dest = *data;
		}
		data++;
		dest++;
	}
	*dest = '\0';
	return dest - str;
}

char *php_raw_url_encode(char const *s, int len, int *new_length)
{
	register int x, y;
	unsigned char *str;

	str = (unsigned char *) safe_emalloc(3, len, 1);
	for (x = 0, y = 0; len--; x++, y++) 
	{
		str[y] = (unsigned char) s[x];
#ifndef CHARSET_EBCDIC
		if ((str[y] < '0' && str[y] != '-' && str[y] != '.') ||
			(str[y] < 'A' && str[y] > '9') ||
			(str[y] > 'Z' && str[y] < 'a' && str[y] != '_') ||
			(str[y] > 'z')) {
			str[y++] = '%';
			str[y++] = hexchars[(unsigned char) s[x] >> 4];
			str[y] = hexchars[(unsigned char) s[x] & 15];
#else /*CHARSET_EBCDIC*/
		if (!isalnum(str[y]) && strchr("_-.", str[y]) != NULL) {
			str[y++] = '%';
			str[y++] = hexchars[os_toascii[(unsigned char) s[x]] >> 4];
			str[y] = hexchars[os_toascii[(unsigned char) s[x]] & 15];
#endif /*CHARSET_EBCDIC*/
		}
	}
	str[y] = '\0';
	if (new_length) {
		*new_length = y;
	}
	return ((char *) str);
}

char *php_url_encode(char const *s, int len, int *new_length)
{
	register unsigned char c;
	unsigned char *to, *start;
	unsigned char const *from, *end;
	
	from = (unsigned char *) s;
	end = (unsigned char *) (s + len);
	start = to = (unsigned char *) safe_emalloc(3, len, 1);

	while (from < end) {
		c = *from++;

		if (c == ' ') {
			*to++ = '+';
#ifndef CHARSET_EBCDIC
		} else if ((c < '0' && c != '-' && c != '.') ||
				   (c < 'A' && c > '9') ||
				   (c > 'Z' && c < 'a' && c != '_') ||
				   (c > 'z')) {
			to[0] = '%';
			to[1] = hexchars[c >> 4];
			to[2] = hexchars[c & 15];
			to += 3;
#else /*CHARSET_EBCDIC*/
		} else if (!isalnum(c) && strchr("_-.", c) == NULL) {
			/* Allow only alphanumeric chars and '_', '-', '.'; escape the rest */
			to[0] = '%';
			to[1] = hexchars[os_toascii[c] >> 4];
			to[2] = hexchars[os_toascii[c] & 15];
			to += 3;
#endif /*CHARSET_EBCDIC*/
		} else {
			*to++ = c;
		}
	}
	*to = 0;
	if (new_length) {
		*new_length = to - start;
	}
	return (char *) start;
}

int php_url_decode(char *str, int len)
{
	char *dest = str;
	char *data = str;

	while (len--) {
		if (*data == '+') {
			*dest = ' ';
		}
		else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) 
				 && isxdigit((int) *(data + 2))) {
#ifndef CHARSET_EBCDIC
			*dest = (char) php_htoi(data + 1);
#else
			*dest = os_toebcdic[(char) php_htoi(data + 1)];
#endif
			data += 2;
			len -= 2;
		} else {
			*dest = *data;
		}
		data++;
		dest++;
	}
	*dest = '\0';
	return dest - str;
}

/////////////////
void php_url_free(php_url *theurl)
{
	if (theurl->scheme)
		efree(theurl->scheme);
	if (theurl->user)
		efree(theurl->user);
	if (theurl->pass)
		efree(theurl->pass);
	if (theurl->host)
		efree(theurl->host);
	if (theurl->path)
		efree(theurl->path);
	if (theurl->query)
		efree(theurl->query);
	if (theurl->fragment)
		efree(theurl->fragment);
	efree(theurl);
}
php_url *php_url_parse(char const *str)
{
	return php_url_parse_ex(str, strlen(str));
}
php_url *php_url_parse_ex(char const *str, int length)
{
	char port_buf[6];
	php_url *ret = (php_url *) ecalloc (1, sizeof(php_url));
	char const *s, *e, *p, *pp, *ue;
		
	s = str;
	ue = s + length;

	/* parse scheme */
	if ((e = (char*)memchr(s, ':', length)) && (e - s)) {
		/* 
		 * certain schemas like mailto: and zlib: may not have any / after them
		 * this check ensures we support those.
		 */
		if (*(e+1) != '/') {
			/* check if the data we get is a port this allows us to 
			 * correctly parse things like a.com:80
			 */
			p = e + 1;
			while (isdigit(*p)) {
				p++;
			}
			
			if ((*p) == '\0' || *p == '/') {
				goto parse_port;
			}
			
			ret->scheme = (char*) estrndup(s, (e-s));
			php_replace_controlchars_ex(ret->scheme, (e - s));
			
			length -= ++e - s;
			s = e;
			goto just_path;
		} else {
			ret->scheme = estrndup(s, (e-s));
			php_replace_controlchars_ex(ret->scheme, (e - s));
		
			if (*(e+2) == '/') {
				s = e + 3;
				if (!strncasecmp("file", ret->scheme, sizeof("file"))) {
					if (*(e + 3) == '/') {
						/* support windows drive letters as in:
						   file:///c:/somedir/file.txt
						*/
						if (*(e + 5) == ':') {
							s = e + 4;
						}
						goto nohost;
					}
				}
			} else {
				if (!strncasecmp("file", ret->scheme, sizeof("file"))) {
					s = e + 1;
					goto nohost;
				} else {
					length -= ++e - s;
					s = e;
					goto just_path;
				}	
			}
		}	
	} else if (e) { /* no scheme, look for port */
		parse_port:
		p = e + 1;
		pp = p;
		
		while (pp-p < 6 && isdigit(*pp)) {
			pp++;
		}
		
		if (pp-p < 6 && (*pp == '/' || *pp == '\0')) {
			memcpy(port_buf, p, (pp-p));
			port_buf[pp-p] = '\0';
			ret->port = atoi(port_buf);
		} else {
			goto just_path;
		}
	} else {
		just_path:
		ue = s + length;
		goto nohost;
	}
	
	e = ue;
	
	if (!(p = (char*)memchr(s, '/', (ue - s)))) {
		if ((p = (char*)memchr(s, '?', (ue - s)))) {
			e = p;
		} else if ((p = (char*)memchr(s, '#', (ue - s)))) {
			e = p;
		}
	} else {
		e = p;
	}	
		
	/* check for login and password */
	if ((p = (char*)memchr(s, '@', (e-s)))) {
		if ((pp = (char*)memchr(s, ':', (p-s)))) {
			if ((pp-s) > 0) {
				ret->user = estrndup(s, (pp-s));
				php_replace_controlchars_ex(ret->user, (pp - s));
			}	
		
			pp++;
			if (p-pp > 0) {
				ret->pass = estrndup(pp, (p-pp));
				php_replace_controlchars_ex(ret->pass, (p-pp));
			}	
		} else {
			ret->user = estrndup(s, (p-s));
			php_replace_controlchars_ex(ret->user, (p-s));
		}
		
		s = p + 1;
	}

	/* check for port */
	if (*s == '[' && *(e-1) == ']') {
		/* Short circuit portscan, 
		   we're dealing with an 
		   IPv6 embedded address */
		p = s;
	} else {
		/* memrchr is a GNU specific extension
		   Emulate for wide compatability */
		for(p = e; *p != ':' && p >= s; p--);
	}

	if (p >= s && *p == ':') {
		if (!ret->port) {
			p++;
			if (e-p > 5) { /* port cannot be longer then 5 characters */
				STR_FREE(ret->scheme);
				STR_FREE(ret->user);
				STR_FREE(ret->pass);
				efree(ret);
				return NULL;
			} else if (e - p > 0) {
				memcpy(port_buf, p, (e-p));
				port_buf[e-p] = '\0';
				ret->port = atoi(port_buf);
			}
			p--;
		}	
	} else {
		p = e;
	}
	
	/* check if we have a valid host, if we don't reject the string as url */
	if ((p-s) < 1) {
		STR_FREE(ret->scheme);
		STR_FREE(ret->user);
		STR_FREE(ret->pass);
		efree(ret);
		return NULL;
	}

	ret->host = estrndup(s, (p-s));
	php_replace_controlchars_ex(ret->host, (p - s));
	
	if (e == ue) {
		return ret;
	}
	
	s = e;
	
	nohost:
	
	if ((p = (char*)memchr(s, '?', (ue - s)))) {
		pp = strchr(s, '#');
		
		if (pp && pp < p) {
			p = pp;
			pp = strchr(pp+2, '#');
		}
	
		if (p - s) {
			ret->path = estrndup(s, (p-s));
			php_replace_controlchars_ex(ret->path, (p - s));
		}	
	
		if (pp) {
			if (pp - ++p) { 
				ret->query = estrndup(p, (pp-p));
				php_replace_controlchars_ex(ret->query, (pp - p));
			}
			p = pp;
			goto label_parse;
		} else if (++p - ue) {
			ret->query = estrndup(p, (ue-p));
			php_replace_controlchars_ex(ret->query, (ue - p));
		}
	} 
	else if ((p = (char*)memchr(s, '#', (ue - s)))) {
		if (p - s) {
			ret->path = estrndup(s, (p-s));
			php_replace_controlchars_ex(ret->path, (p - s));
		}	
		
		label_parse:
		p++;
		
		if (ue - p) {
			ret->fragment = estrndup(p, (ue-p));
			php_replace_controlchars_ex(ret->fragment, (ue - p));
		}	
	} else {
		ret->path = estrndup(s, (ue-s));
		php_replace_controlchars_ex(ret->path, (ue - s));
	}

	return ret;
}


char *php_replace_controlchars_ex(char *str, int len)
{
	unsigned char *s = (unsigned char *)str;
	unsigned char *e = (unsigned char *)str + len;
	
	if (!str) {
		return (NULL);
	}
	
	while (s < e) {
	    
		if (iscntrl(*s)) {
			*s='_';
		}	
		s++;
	}
	
	return (str);
} 


void test_func_php_url_pares(const char * url )
{
	php_url * surl ;
	char turl[255] = {0};
	if( url == 0 )
	{
		strcpy(turl,"http://me:124@localhost/hehe/sdfsdf.ph?sdfsdf=sdfiu#fra");
	}
	else
	{
		strcpy(turl,url);
	}
	surl = php_url_parse(turl);
	//dump_php_url(surl);
	
	URL u (turl);
	u.Parse();
	u.Dump();
	
}


void dump_php_url(php_url * theurl)
{
	if (theurl->scheme)
		fprintf(stderr,"scheme: %s\t",theurl->scheme);
	if (theurl->user)
		fprintf(stderr,"user: %s\t",theurl->user);
	if (theurl->pass)
		fprintf(stderr,"pass: %s\t",theurl->pass);
	if (theurl->host)
		fprintf(stderr,"host: %s\t",theurl->host);
	fprintf(stderr,"port: %d\t",theurl->port);
	if (theurl->path)
		fprintf(stderr,"path: %s\t",theurl->path);
	if (theurl->query)
		fprintf(stderr,"query: %s\t",theurl->query);
	if (theurl->fragment)
		fprintf(stderr,"fragment: %s\t",theurl->fragment);

	fprintf(stderr,"\n");
}

URL::URL(const char * pUrl  )
{
	if(pUrl != 0 )
	{
		raw_url = (char*)malloc(255);
		strncpy(raw_url,pUrl,254);
	}
	this->port = 0 ;
	
	this->scheme=0;
	this->user=0;
	this->pass=0;
	this->host=0;
	
	this->path=0;
	this->query=0;
	this->fragment=0;
	
	this->full_path = 0 ;
	
}
URL::~URL( )
{
	this->Free();
}

bool URL::Parse( const char * pUrl ) 
{
	if(pUrl != 0 )
	{
		raw_url = (char*)malloc(255);
		strncpy(raw_url,pUrl,254);
	}
	char port_buf[6];
	//php_url *ret = (php_url *) ecalloc (1, sizeof(php_url));
	//php_url *ret = (php_url *) this ;
	//#define ret this
	char const *s, *e, *p, *pp, *ue;
	char * str = 0  ;
	int length ; 

	this->RewriteShortHandUrl();
	//printf("RW : %s\n" , this->raw_url ) ;
	
	s = str = this->raw_url ;
	length = strlen(str) ;
	ue = s + length;

	/* parse scheme */
	if ((e = (char*)memchr(s, ':', length)) && (e - s)) {
		/* 
		 * certain schemas like mailto: and zlib: may not have any / after them
		 * this check ensures we support those.
		 */
		if (*(e+1) != '/') {
			/* check if the data we get is a port this allows us to 
			 * correctly parse things like a.com:80
			 */
			p = e + 1;
			while (isdigit(*p)) {
				p++;
			}
			
			if ((*p) == '\0' || *p == '/') {
				goto parse_port;
			}
			
			this->scheme = (char*) estrndup(s, (e-s));
			php_replace_controlchars_ex(this->scheme, (e - s));
			
			length -= ++e - s;
			s = e;
			goto just_path;
		} else {
			this->scheme = estrndup(s, (e-s));
			php_replace_controlchars_ex(this->scheme, (e - s));
		
			if (*(e+2) == '/') {
				s = e + 3;
				if (!strncasecmp("file", this->scheme, sizeof("file"))) {
					if (*(e + 3) == '/') {
						/* support windows drive letters as in:
						   file:///c:/somedir/file.txt
						*/
						if (*(e + 5) == ':') {
							s = e + 4;
						}
						goto nohost;
					}
				}
			} else {
				if (!strncasecmp("file", this->scheme, sizeof("file"))) {
					s = e + 1;
					goto nohost;
				} else {
					length -= ++e - s;
					s = e;
					goto just_path;
				}	
			}
		}	
	} else if (e) { /* no scheme, look for port */
		parse_port:
		p = e + 1;
		pp = p;
		
		while (pp-p < 6 && isdigit(*pp)) {
			pp++;
		}
		
		if (pp-p < 6 && (*pp == '/' || *pp == '\0')) {
			memcpy(port_buf, p, (pp-p));
			port_buf[pp-p] = '\0';
			this->port = atoi(port_buf);
		} else {
			goto just_path;
		}
	} else {
		just_path:
		ue = s + length;
		goto nohost;
	}
	
	e = ue;
	
	if (!(p = (char*)memchr(s, '/', (ue - s)))) {
		if ((p = (char*)memchr(s, '?', (ue - s)))) {
			e = p;
		} else if ((p = (char*)memchr(s, '#', (ue - s)))) {
			e = p;
		}
	} else {
		e = p;
	}	
		
	/* check for login and password */
	if ((p = (char*)memchr(s, '@', (e-s)))) {
		if ((pp = (char*)memchr(s, ':', (p-s)))) {
			if ((pp-s) > 0) {
				this->user = estrndup(s, (pp-s));
				php_replace_controlchars_ex(this->user, (pp - s));
			}	
		
			pp++;
			if (p-pp > 0) {
				this->pass = estrndup(pp, (p-pp));
				php_replace_controlchars_ex(this->pass, (p-pp));
			}	
		} else {
			this->user = estrndup(s, (p-s));
			php_replace_controlchars_ex(this->user, (p-s));
		}
		
		s = p + 1;
	}

	/* check for port */
	if (*s == '[' && *(e-1) == ']') {
		/* Short circuit portscan, 
		   we're dealing with an 
		   IPv6 embedded address */
		p = s;
	} else {
		/* memrchr is a GNU specific extension
		   Emulate for wide compatability */
		for(p = e; *p != ':' && p >= s; p--);
	}

	if (p >= s && *p == ':') {
		if (!this->port) {
			p++;
			if (e-p > 5) { /* port cannot be longer then 5 characters */
				STR_FREE(this->scheme);
				STR_FREE(this->user);
				STR_FREE(this->pass);
				//efree(ret);
				return false ;
			} else if (e - p > 0) {
				memcpy(port_buf, p, (e-p));
				port_buf[e-p] = '\0';
				this->port = atoi(port_buf);
			}
			p--;
		}	
	} else {
		p = e;
	}
	
	/* check if we have a valid host, if we don't reject the string as url */
	if ((p-s) < 1) {
		STR_FREE(this->scheme);
		STR_FREE(this->user);
		STR_FREE(this->pass);
		//efree(ret);
		return false;
	}

	this->host = estrndup(s, (p-s));
	php_replace_controlchars_ex(this->host, (p - s));
	
	if (e == ue) {
		return true ;
	}
	
	s = e;
	
	nohost:
	
	if ((p = (char*)memchr(s, '?', (ue - s)))) {
		pp = strchr(s, '#');
		
		if (pp && pp < p) {
			p = pp;
			pp = strchr(pp+2, '#');
		}
	
		if (p - s) {
			this->path = estrndup(s, (p-s));
			php_replace_controlchars_ex(this->path, (p - s));
		}	
	
		if (pp) {
			if (pp - ++p) { 
				this->query = estrndup(p, (pp-p));
				php_replace_controlchars_ex(this->query, (pp - p));
			}
			p = pp;
			goto label_parse;
		} else if (++p - ue) {
			this->query = estrndup(p, (ue-p));
			php_replace_controlchars_ex(this->query, (ue - p));
		}
	} 
	else if ((p = (char*)memchr(s, '#', (ue - s)))) {
		if (p - s) {
			this->path = estrndup(s, (p-s));
			php_replace_controlchars_ex(this->path, (p - s));
		}	
		
		label_parse:
		p++;
		
		if (ue - p) {
			this->fragment = estrndup(p, (ue-p));
			php_replace_controlchars_ex(this->fragment, (ue - p));
		}	
	} else {
		this->path = estrndup(s, (ue-s));
		php_replace_controlchars_ex(this->path, (ue - s));
	}

	if( this->full_path == 0 )
	{
		this->full_path = (char*)malloc(255);	
	}
	memset(this->full_path , 0 , 255 );
	if( this->query == 0 )
	{
		strcpy(this->full_path , this->path );
	}
	else
	{
		sprintf(this->full_path , "%s?%s" , this->path , this->query );
	}

	return true ;

	return false;
}
char * URL::Encode( const char * pUrl )
{
	register int x, y;
	unsigned char *str;
	const char * s = pUrl;
	int len = strlen(s);
	int inew_length;
	int * new_length = & inew_length ;

	str = (unsigned char *) safe_emalloc(3, len, 1);
	for (x = 0, y = 0; len--; x++, y++) 
	{
		str[y] = (unsigned char) s[x];
#ifndef CHARSET_EBCDIC
		if ((str[y] < '0' && str[y] != '-' && str[y] != '.') ||
			(str[y] < 'A' && str[y] > '9') ||
			(str[y] > 'Z' && str[y] < 'a' && str[y] != '_') ||
			(str[y] > 'z')) {
			str[y++] = '%';
			str[y++] = hexchars[(unsigned char) s[x] >> 4];
			str[y] = hexchars[(unsigned char) s[x] & 15];
#else /*CHARSET_EBCDIC*/
		if (!isalnum(str[y]) && strchr("_-.", str[y]) != NULL) {
			str[y++] = '%';
			str[y++] = hexchars[os_toascii[(unsigned char) s[x]] >> 4];
			str[y] = hexchars[os_toascii[(unsigned char) s[x]] & 15];
#endif /*CHARSET_EBCDIC*/
		}
	}
	str[y] = '\0';
	if (new_length) {
		*new_length = y;
	}
	return ((char *) str);
	
	return 0;
}
char * URL::Decode( const char * pUrl  )
{
	//char *dest = str;
	char *dest = (char*)malloc(strlen(pUrl)+1);
	char *str = (char*) pUrl ;
	char *data = str;
	int len = strlen(data);

	while (len--) {
		if (*data == '+') {
			*dest = ' ';
		}
		else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) 
				 && isxdigit((int) *(data + 2))) {
#ifndef CHARSET_EBCDIC
			*dest = (char) php_htoi(data + 1);
#else
			*dest = os_toebcdic[(char) php_htoi(data + 1)];
#endif
			data += 2;
			len -= 2;
		} else {
			*dest = *data;
		}
		data++;
		dest++;
	}
	*dest = '\0';
	//return dest - str;
	return dest ;
	
	return 0;
}

char * URL::RawEncode( const char * pUrl  )
{
	register int x, y;
	int len = strlen(pUrl);
	
	unsigned char *str = (unsigned char *) safe_emalloc(3, len, 1); 
	char * s = (char*)malloc(strlen(pUrl)+1);
	
	int inew_length;
	int * new_length = &inew_length ;

	strcpy(s,pUrl);
	
	str = (unsigned char *) safe_emalloc(3, len, 1);
	for (x = 0, y = 0; len--; x++, y++) 
	{
		str[y] = (unsigned char) s[x];
#ifndef CHARSET_EBCDIC
		if ((str[y] < '0' && str[y] != '-' && str[y] != '.') ||
			(str[y] < 'A' && str[y] > '9') ||
			(str[y] > 'Z' && str[y] < 'a' && str[y] != '_') ||
			(str[y] > 'z')) {
			str[y++] = '%';
			str[y++] = hexchars[(unsigned char) s[x] >> 4];
			str[y] = hexchars[(unsigned char) s[x] & 15];
#else /*CHARSET_EBCDIC*/
		if (!isalnum(str[y]) && strchr("_-.", str[y]) != NULL) {
			str[y++] = '%';
			str[y++] = hexchars[os_toascii[(unsigned char) s[x]] >> 4];
			str[y] = hexchars[os_toascii[(unsigned char) s[x]] & 15];
#endif /*CHARSET_EBCDIC*/
		}
	}
	str[y] = '\0';
	if (new_length) {
		*new_length = y;
	}
	return ((char *) str);
	
	return 0;
}
char * URL::RawDecode( const char * pUrl )
{
	int len = strlen(pUrl);
	char * str = (char*)malloc(strlen(pUrl)+1);
	strcpy(str,pUrl);
	char *dest = str;
	char *data = str;

	while (len--) {
		if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) 
			&& isxdigit((int) *(data + 2))) {
#ifndef CHARSET_EBCDIC
			*dest = (char) php_htoi(data + 1);
#else
			*dest = os_toebcdic[(char) php_htoi(data + 1)];
#endif
			data += 2;
			len -= 2;
		} else {
			*dest = *data;
		}
		data++;
		dest++;
	}
	*dest = '\0';
	//return dest - str;
	return dest;
	return 0 ;
}

const char * URL::GetScheme()
{
	return this->scheme;
	return 0;
}
const char * URL::GetUser()
{
	return this->user;
	return 0;
}
const char * URL::GetPass()
{
	return this->pass;
	return 0;
}
const char * URL::GetHost()
{
	return this->host ;
	return 0;
}
unsigned int URL::GetPort()
{
	if( this->port == 0 )
	{
		if( strcasecmp(this->scheme,"HTTP") == 0 )
			return 80;
		if( strcasecmp(this->scheme,"HTTPS") == 0 )
			return 443;
		if( strcasecmp(this->scheme,"FTP") == 0 )
			return 21;
	}
	return this->port ;
	return 0;
}
const char * URL::GetPath()
{
	if( this->path == 0 )
	{
		this->path = (char*) malloc(255) ;
		this->path[0] = '/';
		this->path[1] = '\0';
	}
	return this->path;
	return 0;
}

const char * URL::GetFullPath()
{
	if( this->path == 0 )
	{
		this->path = (char*) malloc(255) ;
		this->path[0] = '/';
		this->path[1] = '\0';
		return this->path; 
	}
	else
	{
		return this->full_path ;
	}
}
const char * URL::GetQuery()
{
	return this->query ;
	return 0;
}
const char * URL::GetFragment()
{
	return this->fragment ;
	return 0;
}

#define emalloc malloc
const char * URL::GetBaseFileName(char * pFileName)
{
	
	char *s ;
	int len;
	char tname [60] = {0} ;
	char tsuffix [10] = {0} ;
	int nsize = 0;
	char ch ;

	char url [ 255] = {0};
	strcpy(url , this->raw_url);

	len = strlen(url);

	while ( len > 0 )
	{
		if( this->raw_url[ len - 1 ] == '/' )
		{
			break;
		}
		else if( this->raw_url[ len - 1 ] == '?' )
		{
			memset(tname,0,sizeof(tname));
			nsize = -1 ;
		}
		else
		{
			tname[ nsize  ] = this->raw_url [ len -1 ] ; 
		}
		len -- ;
		nsize ++ ;
	}
	tname[nsize] = '\0';
	
	
	
	//strcpy(tname , &tname[nsize-1] );
	if(  strlen(tname)  == 0 )
	{
		return "index.html";
	}
	
	len = strlen(tname);
	
	for( nsize = 0 ; nsize < len/2 ; nsize ++)
	{
		ch = tname[nsize];
		tname[nsize] = tname[len-nsize-1] ;
		tname[len-nsize-1] = ch ;
	}
	
	//fprintf(stderr , "base name------%s\n" , tname);
	
	strcpy(pFileName,tname);
	
	
	return pFileName ;
	//return &this->raw_url[len];//tname ;
	
	return 0 ;
}

bool URL::SetScheme(const char *pScheme)
{
	strcpy(this->scheme,pScheme);
	return false;
}
bool URL::SetUser(const char *pUser)
{
	strcpy(this->user,pUser);
	return false;
}
bool URL::SetPass(const char *pPass)
{
	strcpy(this->pass,pPass);
	return false;
}
bool URL::SetHost(const char *pHost)
{
	strcpy(this->host,pHost);
	return false;
}
bool URL::SetPort(unsigned int pPort)
{
	this->port = pPort ;
	return false;
}
bool URL::SetPath(const char *pPath)
{
	strcpy(this->path,pPath);
	return false;
}
bool URL::SetQuery(const char *pQuery)
{
	strcpy(this->query,pQuery);
	return false;
}
bool URL::SetFragment(const char * pFragment)
{
	strcpy(this->fragment,pFragment);
	return false;
}


void URL::Dump()
{
	if (this->scheme)
		fprintf(stderr,"scheme: %s\t",this->scheme);
	if (this->user)
		fprintf(stderr,"user: %s\t",this->user);
	if (this->pass)
		fprintf(stderr,"pass: %s\t",this->pass);
	if (this->host)
		fprintf(stderr,"host: %s\t",this->host);
	fprintf(stderr,"port: %d\t",this->port);
	if (this->path)
		fprintf(stderr,"path: %s\t",this->path);
	if (this->query)
		fprintf(stderr,"query: %s\t",this->query);
	if (this->fragment)
		fprintf(stderr,"fragment: %s\t",this->fragment);

	fprintf(stderr,"\n");
}

void URL::Free()
{
	if (this->scheme)
		efree(this->scheme);
	if (this->user)
		efree(this->user);
	if (this->pass)
		efree(this->pass);
	if (this->host)
		efree(this->host);
	if (this->path)
		efree(this->path);
	if (this->query)
		efree(this->query);
	if (this->fragment)
		efree(this->fragment);
	efree(raw_url);	
}

bool URL::RewriteShortHandUrl()
{
  const char *p;
	char * url = this->raw_url ;

	//printf("%s\n" , url);
	
  /* Look for a ':' or '/'.  The former signifies NcFTP syntax, the
     latter Netscape.  */
  for (p = url; *p && *p != ':' && *p != '/'; p++)
    ;

  if (p == url)
    return false ;

  /* If we're looking at "://", it means the URL uses a scheme we
     don't support, which may include "https" when compiled without
     SSL support.  Don't bogusly rewrite such URLs.  */
  if (p[0] == ':' && p[1] == '/' && p[2] == '/')
    return false ;

  if (*p == ':')
    {
      const char *pp;
      char *res;
      /* If the characters after the colon and before the next slash
	 or end of string are all digits, it's HTTP.  */
      int digits = 0;
      for (pp = p + 1; isdigit (*pp); pp++)
	++digits;
      if (digits > 0 && (*pp == '/' || *pp == '\0'))
	goto http;

      /* Prepend "ftp://" to the entire URL... */
      res = (char*) malloc (6 + strlen (url) + 1);
      sprintf (res, "ftp://%s", url);
      /* ...and replace ':' with '/'. */
      res[6 + (p - url)] = '/';
      free(this->raw_url) ;this->raw_url = res ;
      return true ;
    }
  else
    {
      char *res;
    http:
      /* Just prepend "http://" to what we have. */
      res = (char*) malloc (7 + strlen (url) + 1);
      memset(res, 0 , 7 + strlen (url) + 1);
      sprintf (res, "http://%s", url);
      free(this->raw_url) ;this->raw_url = res ;

      return true ;
    }	

	return false ;
}

int URL::FullPathLength()
{
	  int len = 0;
/*
#define FROB(el) if (this->el) len += 1 + strlen (this->el)

	  FROB (path);
	  //FROB (params);
	  FROB (query);

#undef FROB
*/

	if (this->path)  len += 1 + strlen (this->path ) ; 
	if (this->query) len += 1 + strlen (this->query ) ;

	  return len;
}
int URL::FullPathRewrite( char * where )
{
#define FROB(el, chr) do {			\
	  char *f_el = this->el;				\
	  if (f_el) {					\
	    int l = strlen (f_el);			\
	    *where++ = chr;				\
	    memcpy (where, f_el, l);			\
	    where += l;					\
	  }						\
	} while (0)

	  FROB (path, '/');
	  //FROB (params, ';');
	  FROB (query, '?');

#undef FROB
	return 0 ;
}


