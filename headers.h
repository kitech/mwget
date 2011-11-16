
#ifndef HEADERS_H
#define HEADERS_H

#include <time.h>
#include <map>
#include <string>

class HttpHeader
{
public:
	enum {  HG_OK, HG_ERROR, HG_EOF};
	HttpHeader();
	~HttpHeader();
	bool Parse(const char * hb);
	void Dump();

	void ClearHeaders();
	void AddHeader(std::string key , std::string value);
	void RemoveHeader(std::string key);
	std::string HeaderString();
	int GetHeaderLength();
	std::string GetHeader(std::string key);
	
private:
	std::map<std::string,std::string> mHeader;
	int mHeaderLength;
	
};
/*T
		char tmp[1000] = {0};
		strcpy(tmp,"HTTP/1.1 200 OK\r\nDate: Tue, 03 Jan 2006 05:21:14 GMT\r\nServer: Apache/2.0.52 (Win32) PHP/5.0.3\r\n\r\n");

		HttpHeader header ;
		header.Parse(tmp);
		header.Dump();

		header.ClearHeaders();
		header.AddHeader( "get", "/");
		header.AddHeader( "Host","hehehe");
		std::cout<<header.HeaderString();

*/



class FtpCommand
{
public:
	FtpCommand(const char * response =  0);
	~FtpCommand();

	bool GetPASVAddr(char * host, int *port);
	bool GetLISTSize( long *size );
	
	void SetResponse(const char * response);

	bool IsPasvRespnose(const char * response = 0 );
	bool IsUserRespnose(const char * response = 0 );
	bool IsPassRespnose(const char * response = 0 );
	bool IsTypeRespnose(const char * response = 0 );
	bool IsListRespnose(const char * response = 0 );
	bool IsTransferCompleteRespnose(const char * response = 0 );
	bool IsRestRespnose(const char * response = 0 );
	bool IsRetrRespnose(const char * response = 0 );

	
private:
	char mResponse[120];
	char mCommand[100];
	
};


//=========================
/*
ftpparse(&fp,buf,len) tries to parse one line of LIST output.

The line is an array of len characters stored in buf.
It should not include the terminating CR LF; so buf[len] is typically CR.

If ftpparse() can't find a filename, it returns 0.

If ftpparse() can find a filename, it fills in fp and returns 1.
fp is a struct ftpparse, defined below.
The name is an array of fp.namelen characters stored in fp.name;
fp.name points somewhere within buf.
*/

class FtpListParse
{
public:

	enum { 
			FTPPARSE_SIZE_UNKNOWN = 0 ,  
			FTPPARSE_SIZE_BINARY = 1 , 
				/*
				 * size is the number of octets in TYPE I 
				 */
			FTPPARSE_SIZE_ASCII =  2
				/*
				 * size is the number of octets in TYPE A 
				 */
		};



	enum { 
				FTPPARSE_MTIME_UNKNOWN = 0,
				FTPPARSE_MTIME_LOCAL = 1 ,
				/*
				 * time is correct 
				 */
				FTPPARSE_MTIME_REMOTEMINUTE = 2 ,
					/*
					 * time zone and secs are unknown 
					 */
				FTPPARSE_MTIME_REMOTEDAY = 3
			};
					/*
					 * time zone and time of day are unknown 
					 */
/*
When a time zone is unknown, it is assumed to be GMT. You may want
to use localtime() for LOCAL times, along with an indication that the
time is correct in the local time zone, and gmtime() for REMOTE* times.
*/

	enum { FTPPARSE_ID_UNKNOWN = 0 ,
			 FTPPARSE_ID_FULL =1
			 	/*
				 * unique identifier for files on this FTP server 
				 */
			};
			

	FtpListParse() ;
	~FtpListParse() ;
	bool Parse(char * buf , int len ) ;

	long GetSize() ;

	void Dump() ;

private:
	long totai(long year,long month,long mday);
	void initbase();
	void initnow() ;
	long guesstai(long month,long mday);
	int check(char *buf,char *monthname);
	 int getmonth( char *buf,int len );
	long getlong(char *buf ,int len ); 

    char *name;			/*
				 * not necessarily 0-terminated 
				 */
    int namelen;
    int flagtrycwd;		/*
				 * 0 if cwd is definitely pointless, 1 otherwise 
				 */
    int flagtryretr;		/*
				 * 0 if retr is definitely pointless, 1 otherwise 
				 */
    int sizetype;
    long size;			/*
				 * number of octets 
				 */
    int mtimetype;
    time_t mtime;		/*
				 * modification time 
				 */
    int idtype;
    char *id;			/*
				 * not necessarily 0-terminated 
				 */
    int idlen;

	char real_name[156] ;

    ///////
    time_t base;
    int flagneedbase ;
    long now;		/*
				 * current time 
				 */
    int flagneedcurrentyear;
    long currentyear;	/*
				 * approximation to current year 
				 */
				 
};

	/*
	 * UNIX-style listing, without inum and without blocks 
	 */
	/*
	 * "-rw-r--r--   1 root     other        531 Jan 29 03:26 README" 
	 */
	/*
	 * "dr-xr-xr-x   2 root     other        512 Apr  8  1994 etc" 
	 */
	/*
	 * "dr-xr-xr-x   2 root     512 Apr  8  1994 etc" 
	 */
	/*
	 * "lrwxrwxrwx   1 root     other          7 Jan 25 00:17 bin -> usr/bin" 
	 */
	/*
	 * Also produced by Microsoft's FTP servers for Windows: 
	 */
	/*
	 * "----------   1 owner    group         1803128 Jul 10 10:18 ls-lR.Z" 
	 */
	/*
	 * "d---------   1 owner    group               0 May  9 19:45 Softlib" 
	 */
	/*
	 * Also WFTPD for MSDOS: 
	 */
	/*
	 * "-rwxrwxrwx   1 noone    nogroup      322 Aug 19  1996 message.ftp" 
	 */
	/*
	 * Also NetWare: 
	 */
	/*
	 * "d [R----F--] supervisor            512       Jan 16 18:53    login" 
	 */
	/*
	 * "- [R----F--] rhesus             214059       Oct 20 15:27    cx.exe" 
	 */
	/*
	 * Also NetPresenz for the Mac: 
	 */
	/*
	 * "-------r--         326  1391972  1392298 Nov 22  1995 MegaPhone.sit" 
	 */
	/*
	 * "drwxrwxr-x               folder        2 May 10  1996 network" 
	 */


#endif

