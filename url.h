
#ifndef URL_H
#define URL_H

#ifdef __cplusplus
extern "C"{
#endif

typedef struct php_url {
	char *scheme;
	char *user;
	char *pass;
	char *host;
	unsigned short port;
	char *path;
	char *query;
	char *fragment;
} php_url;




void php_url_free(php_url *theurl);
php_url *php_url_parse(char const *str);

void test_func_php_url_pares(const char * url = 0 );

void dump_php_url(php_url * url);

#ifdef __cplusplus
};
#endif

class URL
{
public:
		URL(const char * pUrl = 0 );
		~URL( );
	bool Parse( const char * pUrl = 0 ) ;
	char * Encode( const char * pUrl = 0 );
	char * Decode( const char * pUrl = 0 );
	char * RawEncode( const char * pUrl = 0 );
	char * RawDecode( const char * pUrl = 0 );	
	const char * GetScheme();
	const char * GetUser();
	const char * GetPass();
	const char * GetHost();
	unsigned int GetPort();
	const char * GetPath();
	const char * GetFullPath();
	const char * GetQuery();
	const char * GetFragment();

	const char * GetBaseFileName(char * pFileName);

	bool SetScheme(const char *pScheme);
	bool SetUser(const char *pUser);
	bool SetPass(const char *pPass);
	bool SetHost(const char *pHost);
	bool SetPort(unsigned int pPort);
	bool SetPath(const char *pPath);
	bool SetQuery(const char *pQuery);
	bool SetFragment(const char * pFragment);
	
	void Dump();
private:
	void Free();
	
	/* Used by main.c: detect URLs written using the "shorthand" URL forms
	   popularized by Netscape and NcFTP.  HTTP shorthands look like this:

	   www.foo.com[:port]/dir/file   -> http://www.foo.com[:port]/dir/file
	   www.foo.com[:port]            -> http://www.foo.com[:port]

	   FTP shorthands look like this:

	   foo.bar.com:dir/file          -> ftp://foo.bar.com/dir/file
	   foo.bar.com:/absdir/file      -> ftp://foo.bar.com//absdir/file

	   If the URL needs not or cannot be rewritten, return NULL.  */	
	bool RewriteShortHandUrl();

	int FullPathLength();
	int FullPathRewrite( char * where );
	
	
private:
	
	char *scheme;
	char *user;
	char *pass;
	char *host;
	unsigned short port;
	char *path;
	char *query;
	char *fragment;

	char * raw_url ;

	char * full_path ;
	
};


/*


*/

#endif
