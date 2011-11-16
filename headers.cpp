
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "headers.h"

HttpHeader::HttpHeader()
{
}
HttpHeader::~HttpHeader()
{
}

bool HttpHeader::Parse(const char * hb)
{
	int bufsize = 80;
	int i ;
	char * p ;
	char * buff = (char*)malloc(strlen(hb) + 1 );
	strcpy(buff,hb);
	char key[60] = {0};
	char value[180] = {0};
	bool vbegin = false ;
	int header_len = 0 ;

	this->mHeader.clear();	// when this object used sometimes ago
	
	for( p = buff ; p != 0 ; p ++)
	{	
		header_len ++ ;
		
		if ( *p == ':' )
		{
			vbegin = true ;			
		}
		else if ( *p == '\n' )
		{
			vbegin =false ;
			if( key[0] == '\r' )
			{
				break;
			}
			if( this->mHeader.empty() )
			{
				strncpy(value,&key[9]	, 3 );	//get http status field
				strcpy(key,"Status");			//set key as "status"
			}
			else
			{
				int len = strlen(value)-2 ;				
				strncpy(value,&value[1],len);	// drop head ' ' and tail  '\r'
				value[len] = '\0';		//i think this is the bug of strncpy ,no null append
			}
			this->mHeader[key] = value ;
			
			//printf("%s  %s \n",key , value);
			memset(key,0,sizeof(key));
			memset(value,0,sizeof(value));
			
		}
		else
		{
			if( vbegin )
			{
				value[strlen(value)] = *p ;
			}
			else
			{
				key[strlen(key) ] = *p;
			}
		}
		
	}

	free(buff);
	this->mHeaderLength = header_len ;
	return true ;
	
	return false ;
}

void HttpHeader::Dump()
{
	std::map<std::string,std::string>::iterator mit;
	for( mit = this->mHeader.begin() ; mit != this->mHeader.end() ; mit ++ )
	{
		printf("K : \"%s\"   V: \"%s\" \n",mit->first.c_str(),mit->second.c_str());
	}
}
void HttpHeader::ClearHeaders()
{
	this->mHeader.clear();
}
//use it to combine send header as client , but not server use
void HttpHeader::AddHeader(std::string key , std::string value)
{
	this->mHeader[key] = value ;

}
void HttpHeader::RemoveHeader(std::string key)
{
	this->mHeader.erase(key);
}

std::string HttpHeader::HeaderString()
{
	std::string header;
	std::string key;

	std::map<std::string,std::string>::iterator mit;
	for( mit = this->mHeader.begin() ; mit != this->mHeader.end() ; mit ++ )
	{
		//printf("K : \"%s\"   V: \"%s\" \n",mit->first.c_str(),mit->second.c_str());
		key = mit->first.c_str();
		if( strcasecmp(key.c_str(),"GET") == 0 || strcasecmp(key.c_str(),"POST") == 0 
				||strcasecmp(key.c_str(),"HEADER") == 0 )
		{
			header = key + " " + mit->second + " HTTP/1.0\r\n" + header ;
		}
		else
		{
			header =  header + key + ": " + mit->second + "\r\n" ;
		}
	}
	if( header.length() > 0 )
	{
		header = header + "\r\n";
	}

	return header ;
}

int HttpHeader::GetHeaderLength()
{
	return this->mHeaderLength;
}

std::string HttpHeader::GetHeader(std::string key)
{
	//fprintf(stderr , "header : %s = %s \n" , key.c_str() , this->mHeader[key].c_str() );
	return this->mHeader[key];
}



///////////////////ftp response handler

FtpCommand::FtpCommand(const char * response)
{

	if( response != 0 )
	{
		memset(this->mResponse,0,sizeof(this->mResponse));
		strncpy(this->mResponse,response,sizeof(this->mResponse)-1);
	}
}

FtpCommand::~FtpCommand()
{
	
}

bool FtpCommand::GetPASVAddr(char * host, int *port)
{
	char *h = host ;
	*port = 0 ;
	char * s ;
	char * hoststart ;
	int ip_size = 0 , i = 0  ;
	if( strlen(this->mResponse) >= 4 )
	{
		  /* Parse the response.  */
		  s = this->mResponse ;
		  for (s += 4; *s && !isdigit(*s); s++);
		  if (!*s)
		    return false ;

		/* skip over the host ip, to get the port */
		hoststart = s ;
		for (i = 0; i < 4; i++) {
			for (; isdigit((int) *s); s++)    ;
			if (*s != ',') {
				return 0;
			}
			*s='.';
			s++;
		}
		ip_size = s - hoststart ;
		
		memcpy(host, hoststart, ip_size);
		host[ip_size-1] = '\0';
		hoststart = host ;

		  /* Then, get the address length */
		  int addrlen = 0;
		  char part[4] = {0};
		  for (; (*s); s++)
		  {
		  	//fputc(*s,stderr);
		  	//fputc('\t',stderr);
		  	if( *s == ',' ) 
		  	{
		  		*port = 256 * atoi(part);
		  		memset(part,0,sizeof(part));
		  		continue ;
		  	}
		  	if( *s == ')' ) 
		  	{
		  		*port += atoi(part);
		  		break ;
		  	}
		  	if ( isdigit(*s) )
		  	{
		  		part[strlen(part)]=*s;
		  	}
		  }
			
		//fprintf(stderr,"Host:%s  : %d  -- %d \n",host,ip_size , *port);

		return true;
	}
	return false;
}

bool FtpCommand::GetLISTSize( long *size )
{
	char * s = this->mResponse ;
	char * e = this->mResponse ;
	int sepc = 0 ;
	
	char real_name[128] = {0};
	char strsize[16] = {0};
	
	char * id = 0 ;
	char * buf ;

	int i ;
	bool dret = false ;	

	switch(*s)
	{
	 	case '0':case '1':case '2':case '3':case '4':
	 	case '5':	case '6':	case '7':case '8':case '9':
	 		{
	 			buf = this->mResponse ;
	 			i = 0 ;
	 			id = buf + 31 ;
	 			while(*id >= '0' && *id<='9' )
	 			{ 				
	 				strsize[i++] = *id;
	 				id ++ ;
	 			}
	 			while(*id == ' ') id++;
	 			//this->name = id ;
	 			i = 0 ;
	 			while(*id != '\0' && *id != '\r' && *id != '\n' )
	 			{
	 				real_name[i++] = *id ; id++;
	 			}
	 			*size = strtol(strsize , 0 , 10) ;
	 			//fprintf(stderr , "===%s===%s++++%s(%d)++++\n",strsize , id , real_name , strlen(real_name) );
				dret = true ; 
				break;
			}
			/*
			 * see http://pobox.com/~djb/proto/eplf.txt 
			 */
			/*
			 * "+i8388621.29609,m824255902,/,\tdev" 
			 */
			/*
			 * "+i8388621.44468,m839956783,r,s10376,\tRFCEPLF" 
			 */
		    case '+':
		    	{	
		    		//
				fprintf(stderr , "not supported FTP LIST format (EPLF) now.\n") ;
				dret = false  ; 
				break ;
			}
			
		    case 'b': case 'c':  case 'd':  case 'l':
		    case 'p': case 's':  case '-':
			{
				//fprintf(stderr,s);
				for( ; *s  !=  '\0'  ; s ++ )
				{
					//fputc(*s,stderr);
					if( *s == ' ' && *(s-1) != ' ')
					{	sepc ++ ;
						
						//fprintf(stderr," sep %d \n" , sepc );
						if( sepc == 4 )  e = s ;
						if( sepc == 5 ) break;
					}		
				}
				for( ; *e == ' ' ; e ++ ) ; 
				
				*s= '\0';
				*size = atol(e);
				dret = true ; 
				break ;
			}
			default:
				fprintf(stderr , "not supported FTP LIST format( UNKNOWN ) now.\n") ;
				dret = false ; 
				break ;
	}
	
	//fprintf(stderr," len %d %ld \n" ,  s- e , *size );
	
	return dret ;

	return false ;
}

void FtpCommand::SetResponse(const char * response)
{

	if( response != 0 )
	{
		memset(this->mResponse,0,sizeof(this->mResponse));
		strncpy(this->mResponse,response,sizeof(this->mResponse)-1);
	}
}

bool FtpCommand::IsPasvRespnose(const char * response)
{
	char * s = 0 ;
	if( response != 0 )
	{
		memset(this->mResponse,0,sizeof(this->mResponse));
		strncpy(this->mResponse,response,sizeof(this->mResponse)-1);
	}
	
	s = strstr( response , "227 ") ;	//227 

	if( s != 0  ) return true ;
	return false ;
}

bool FtpCommand::IsTypeRespnose(const char * response )
{
	char * s = 0 ;
	if( response != 0 )
	{
		memset(this->mResponse,0,sizeof(this->mResponse));
		strncpy(this->mResponse,response,sizeof(this->mResponse)-1);
	}
	
	s = strstr(response , "200 ") ;	//200 

	if( s != 0  ) return true ;
	return false ;
}

bool FtpCommand::IsListRespnose(const char * response )
{
	char * s = 0 ;
	if( response != 0 )
	{
		memset(this->mResponse,0,sizeof(this->mResponse));
		strncpy(this->mResponse,response,sizeof(this->mResponse)-1);
	}
	
	s = strstr(response, "Transfer starting") ;	//125 	
	if( s != 0  ) return true ;

	s = strstr(response, "150 ") ;	//150
	if( s != 0  ) return true ;
	
	return false ;
}

bool FtpCommand::IsTransferCompleteRespnose(const char * response  )
{
	char * s = 0 ;
	if( response != 0 )
	{
		memset(this->mResponse,0,sizeof(this->mResponse));
		strncpy(this->mResponse,response,sizeof(this->mResponse)-1);
	}
	
	s = strstr(response , "226 Transfer complete") ;	//226

	if( s != 0  ) return true ;
	return false ;
}

bool FtpCommand::IsUserRespnose(const char * response )
{
	char * s = 0 ;
	if( response != 0 )
	{
		memset(this->mResponse,0,sizeof(this->mResponse));
		strncpy(this->mResponse,response,sizeof(this->mResponse)-1);
	}
	
	s = strstr( response , "331 ") ;	//send  password

	if( s != 0  ) return true ;
	return false ;
}

bool FtpCommand::IsPassRespnose(const char * response )
{
	char * s = 0 ;
	if( response != 0 )
	{
		memset(this->mResponse,0,sizeof(this->mResponse));
		strncpy(this->mResponse,response,sizeof(this->mResponse)-1);
	}
	
	s = strstr( response , "230 ") ;	//230 

	if( s != 0  ) return true ;
	return false ;
}

bool FtpCommand::IsRestRespnose(const char * response )
{
	char * s = 0 ;
	if( response != 0 )
	{
		memset(this->mResponse,0,sizeof(this->mResponse));
		strncpy(this->mResponse,response,sizeof(this->mResponse)-1);
	}
	
	s = strstr(response , "350 ") ;	//350 Restarting at

	if( s != 0  ) return true ;
	return false ;
}

bool FtpCommand::IsRetrRespnose(const char * response )
{
	char * s = 0 ;
	if( response != 0 )
	{
		memset(this->mResponse,0,sizeof(this->mResponse));
		strncpy(this->mResponse,response,sizeof(this->mResponse)-1);
	}
	
	s = strstr(response , "150 ") ;	//125 

	if( s != 0  ) return true ;
	return false ;
}


//======================
FtpListParse::FtpListParse() 
{
    this->name = 0;
    this->namelen = 0;
    this->flagtrycwd = 0;
    this->flagtryretr = 0;
    this->sizetype = FTPPARSE_SIZE_UNKNOWN;
    this->size = 0;
    this->mtimetype = FTPPARSE_MTIME_UNKNOWN;
    this->mtime = 0;
    this->idtype = FTPPARSE_ID_UNKNOWN;
    this->id = 0;
    this->idlen = 0;
}

FtpListParse::~FtpListParse() 
{
    this->name = 0;
    this->namelen = 0;
    this->flagtrycwd = 0;
    this->flagtryretr = 0;
    this->sizetype = FTPPARSE_SIZE_UNKNOWN;
    this->size = 0;
    this->mtimetype = FTPPARSE_MTIME_UNKNOWN;
    this->mtime = 0;
    this->idtype = FTPPARSE_ID_UNKNOWN;
    this->id = 0;
    this->idlen = 0;
}


long FtpListParse::totai(long year,long month,long mday)
{
    /*
     * adapted from datetime_untai() 
     */
    /*
     * about 100x faster than typical mktime() 
     */
    long result;
    if (month >= 2)
	month -= 2;
    else
    {
	month += 10;
	--year;
    }
    result = (mday - 1) * 10 + 5 + 306 * month;
    result /= 10;
    if (result == 365)
    {
	year -= 3;
	result = 1460;
    } else
	result += 365 * (year % 4);
    year /= 4;
    result += 1461 * (year % 25);
    year /= 25;
    if (result == 36524)
    {
	year -= 3;
	result = 146096;
    } else
    {
	result += 36524 * (year % 4);
    }
    year /= 4;
    result += 146097 * (year - 5);
    result += 11017;
    return result * 86400;	
}
void FtpListParse::initbase()
{
    struct tm *t;
    if (!flagneedbase)
	return;

    base = 0;
    t = gmtime(&base);
    base =
	-(totai(t->tm_year + 1900, t->tm_mon, t->tm_mday) +
	  t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec);
    /*
     * time_t is assumed to be measured in TAI seconds. 
     */
    /*
     * base may be slightly off if time_t is measured in UTC seconds. 
     */
    /*
     * Typical software naively claims to use UTC but actually uses TAI. 
     */
    flagneedbase = 0;
}
void FtpListParse::initnow() 
{
    long day;
    long year;

    initbase();
    now = time((time_t *) 0) - base;

    if (flagneedcurrentyear)
    {
	/*
	 * adapted from datetime_tai() 
	 */
	day = now / 86400;
	if ((now % 86400) < 0)
	    --day;
	day -= 11017;
	year = 5 + day / 146097;
	day = day % 146097;
	if (day < 0)
	{
	    day += 146097;
	    --year;
	}
	year *= 4;
	if (day == 146096)
	{
	    year += 3;
	    day = 36524;
	} else
	{
	    year += day / 36524;
	    day %= 36524;
	}
	year *= 25;
	year += day / 1461;
	day %= 1461;
	year *= 4;
	if (day == 1460)
	{
	    year += 3;
	    day = 365;
	} else
	{
	    year += day / 365;
	    day %= 365;
	}
	day *= 10;
	if ((day + 5) / 306 >= 10)
	    ++year;
	currentyear = year;
	flagneedcurrentyear = 0;
    }
}

/* UNIX ls does not show the year for dates in the last six months. */
/* So we have to guess the year. */
/* Apparently NetWare uses ``twelve months'' instead of ``six months''; ugh. */
/* Some versions of ls also fail to show the year for future dates. */
long FtpListParse::guesstai(long month,long mday)
{
    long year;
    long t;

    initnow();

    for (year = currentyear - 1; year < currentyear + 100; ++year)
    {
	t = totai(year, month, mday);
	if (now - t < 350 * 86400)
	    return t;
    }
/* Added by Grendel */
    return 0;
}
int FtpListParse::check(char *buf,char *monthname)
{
    if ((buf[0] != monthname[0]) && (buf[0] != monthname[0] - 32))
	return 0;
    if ((buf[1] != monthname[1]) && (buf[1] != monthname[1] - 32))
	return 0;
    if ((buf[2] != monthname[2]) && (buf[2] != monthname[2] - 32))
	return 0;
    return 1;
}

static char *months[12] = {
    "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct",
    "nov", "dec"
};

 int FtpListParse::getmonth( char *buf,int len )
{
    int i;
    if (len == 3)
	for (i = 0; i < 12; ++i)
	    if (check(buf, months[i]))
		return i;
    return -1;
}
long FtpListParse::getlong(char *buf ,int len )
{
    long u = 0;
    while (len-- > 0)
	u = u * 10 + (*buf++ - '0');
    return u;
} 


long FtpListParse::GetSize() 
{
	return this->size ;
}

/* ftpparse.c, ftpparse.h: library for parsing FTP LIST responses
D. J. Bernstein, djb@pobox.com
19970712 (doc updated 19970810)
Commercial use is fine, if you let me know what programs you're using this in.

Currently covered:
EPLF.
UNIX ls, with or without gid.
Microsoft FTP Service.
Windows NT FTP Server.
VMS.
WFTPD (DOS).
NetPresenz (Mac).
NetWare.

Definitely not covered: 
Long VMS filenames, with information split across two lines.
NCSA Telnet FTP server. Has LIST = NLST (and bad NLST for directories).

Written for maximum portability, but tested only under UNIX so far.
*/

bool FtpListParse::Parse( char * buf , int len ) 
{
    int i;
    int j;
    int state;
    long size = -1;
    long year;
    long month = -1;
    long mday = -1;
    long hour;
    long minute;

    char strsize[16] = {0};

   /* fp->name = 0;
    fp->namelen = 0;
    fp->flagtrycwd = 0;
    fp->flagtryretr = 0;
    fp->sizetype = FTPPARSE_SIZE_UNKNOWN;
    fp->size = 0;
    fp->mtimetype = FTPPARSE_MTIME_UNKNOWN;
    fp->mtime = 0;
    fp->idtype = FTPPARSE_ID_UNKNOWN;
    fp->id = 0;
    fp->idlen = 0;	*/


    if (len < 2)		/*
				 * an empty name in EPLF, with no info, could be 2 chars 
				 */
	return 0;


    switch (*buf)
    {
	/*
	 * see http://pobox.com/~djb/proto/eplf.txt 
	 */
	/*
	 * "+i8388621.29609,m824255902,/,\tdev" 
	 */
	/*
	 * "+i8388621.44468,m839956783,r,s10376,\tRFCEPLF" 
	 */

    case '+':
    	{
  
	i = 1;
	for (j = 1; j < len; ++j)
	{
	    if (buf[j] == 9)
	    {
		this->name = buf + j + 1;
		this->namelen = len - j - 1;
		return 1;
	    }
	    if (buf[j] == ',')
	    {
		switch (buf[i])
		{
		case '/':
		    this->flagtrycwd = 1;
		    break;
		case 'r':
		    this->flagtryretr = 1;
		    break;
		case 's':
		    this->sizetype = FTPPARSE_SIZE_BINARY;
		    this->size = getlong(buf + i + 1, j - i - 1);
		    break;
		case 'm':
		    this->mtimetype = FTPPARSE_MTIME_LOCAL;
		    initbase();
		    this->mtime = base + getlong(buf + i + 1, j - i - 1);
		    break;
		case 'i':
		    this->idtype = FTPPARSE_ID_FULL;
		    this->id = buf + i + 1;
		    this->idlen = j - i - 1;
		}
		i = j + 1;
	    }
	}
	
	return 0;
	}
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
    case 'b':
    case 'c':
    case 'd':
    case 'l':
    case 'p':
    case 's':
    case '-':
	{
	if (*buf == 'd')
	    this->flagtrycwd = 1;
	if (*buf == '-')
	    this->flagtryretr = 1;
	if (*buf == 'l')
	    this->flagtrycwd = this->flagtryretr = 1;

	state = 1;
	i = 0;
	for (j = 1; j < len; ++j)
	    if ((buf[j] == ' ') && (buf[j - 1] != ' '))
	    {
		switch (state)
		{
		case 1:	/*
				   * skipping perm 
				 */
		    state = 2;
		    break;
		case 2:	/*
				   * skipping nlink 
				 */
		    state = 3;
		    if ((j - i == 6) && (buf[i] == 'f'))	/*
								 * for NetPresenz 
								 */
			state = 4;
		    break;
		case 3:	/*
				   * skipping uid 
				 */
		    state = 4;
		    break;
		case 4:	/*
				   * getting tentative size 
				 */
		    size = getlong(buf + i, j - i);
		    state = 5;
		    break;
		case 5:	/*
				   * searching for month, otherwise getting tentative size 
				 */
		    month = getmonth(buf + i, j - i);
		    if (month >= 0)
			state = 6;
		    else
			size = getlong(buf + i, j - i);
		    break;
		case 6:	/*
				   * have size and month 
				 */
		    mday = getlong(buf + i, j - i);
		    state = 7;
		    break;
		case 7:	/*
				   * have size, month, mday 
				 */
		    if ((j - i == 4) && (buf[i + 1] == ':'))
		    {
			hour = getlong(buf + i, 1);
			minute = getlong(buf + i + 2, 2);
			this->mtimetype = FTPPARSE_MTIME_REMOTEMINUTE;
			initbase();
			this->mtime =
			    base + guesstai(month,
					    mday) + hour * 3600 +
			    minute * 60;
		    } else if ((j - i == 5) && (buf[i + 2] == ':'))
		    {
			hour = getlong(buf + i, 2);
			minute = getlong(buf + i + 3, 2);
			this->mtimetype = FTPPARSE_MTIME_REMOTEMINUTE;
			initbase();
			this->mtime =
			    base + guesstai(month,
					    mday) + hour * 3600 +
			    minute * 60;
		    } else if (j - i >= 4)
		    {
			year = getlong(buf + i, j - i);
			this->mtimetype = FTPPARSE_MTIME_REMOTEDAY;
			initbase();
			this->mtime = base + totai(year, month, mday);
		    } else
			return 0;
		    this->name = buf + j + 1;
		    this->namelen = len - j - 1;
		    state = 8;
		    break;
		case 8:	/*
				   * twiddling thumbs 
				 */
		    break;
		}
		i = j + 1;
		while ((i < len) && (buf[i] == ' '))
		    ++i;
	    }

	if (state != 8)
	    return 0;

	this->size = size;
	this->sizetype = FTPPARSE_SIZE_ASCII;

	if (*buf == 'l')
	    for (i = 0; i + 3 < this->namelen; ++i)
		if (this->name[i] == ' ')
		    if (this->name[i + 1] == '-')
			if (this->name[i + 2] == '>')
			    if (this->name[i + 3] == ' ')
			    {
				this->namelen = i;
				break;
			    }

	/*
	 * eliminate extra NetWare spaces 
	 */
	if ((buf[1] == ' ') || (buf[1] == '['))
	    if (this->namelen > 3)
		if (this->name[0] == ' ')
		    if (this->name[1] == ' ')
			if (this->name[2] == ' ')
			{
			    this->name += 3;
			    this->namelen -= 3;
			}

	return 1;


    /*
     * MultiNet (some spaces removed from examples) 
     */
    /*
     * "00README.TXT;1      2 30-DEC-1996 17:44 [SYSTEM] (RWED,RWED,RE,RE)" 
     */
    /*
     * "CORE.DIR;1          1  8-SEP-1996 16:09 [SYSTEM] (RWE,RWE,RE,RE)" 
     */
    /*
     * and non-MutliNet VMS: 
     */
    /*
     * "CII-MANUAL.TEX;1  213/216  29-JAN-1996 03:33:12  [ANONYMOU,ANONYMOUS]   (RWED,RWED,,)" 
     */
    for (i = 0; i < len; ++i)
	if (buf[i] == ';')
	    break;
    if (i < len)
    {
	this->name = buf;
	this->namelen = i;
	if (i > 4)
	    if (buf[i - 4] == '.')
		if (buf[i - 3] == 'D')
		    if (buf[i - 2] == 'I')
			if (buf[i - 1] == 'R')
			{
			    this->namelen -= 4;
			    this->flagtrycwd = 1;
			}
	if (!this->flagtrycwd)
	    this->flagtryretr = 1;
	while (buf[i] != ' ')
	    if (++i == len)
		return 0;
	while (buf[i] == ' ')
	    if (++i == len)
		return 0;
	while (buf[i] != ' ')
	    if (++i == len)
		return 0;
	while (buf[i] == ' ')
	    if (++i == len)
		return 0;
	j = i;
	while (buf[j] != '-')
	    if (++j == len)
		return 0;
	mday = getlong(buf + i, j - i);
	while (buf[j] == '-')
	    if (++j == len)
		return 0;
	i = j;
	while (buf[j] != '-')
	    if (++j == len)
		return 0;
	month = getmonth(buf + i, j - i);
	if (month < 0)
	    return 0;
	while (buf[j] == '-')
	    if (++j == len)
		return 0;
	i = j;
	while (buf[j] != ' ')
	    if (++j == len)
		return 0;
	year = getlong(buf + i, j - i);
	while (buf[j] == ' ')
	    if (++j == len)
		return 0;
	i = j;
	while (buf[j] != ':')
	    if (++j == len)
		return 0;
	hour = getlong(buf + i, j - i);
	while (buf[j] == ':')
	    if (++j == len)
		return 0;
	i = j;
	while ((buf[j] != ':') && (buf[j] != ' '))
	    if (++j == len)
		return 0;
	minute = getlong(buf + i, j - i);

	this->mtimetype = FTPPARSE_MTIME_REMOTEMINUTE;
	initbase();
	this->mtime =
	    base + totai(year, month, mday) + hour * 3600 + minute * 60;

	return 1;
    }
	
 }

 	case '0':case '1':case '2':case '3':case '4':
 	case '5':	case '6':	case '7':case '8':case '9':
 		{
 			i = 0 ;
 			id = buf + 31 ;
 			while(*id >= '0' && *id<='9' )
 			{ 				
 				strsize[i++] = *id;
 				id ++ ;
 			}
 			while(*id == ' ') id++;
 			this->name = id ;
 			i = 0 ;
 			while(*id != '\0' && *id != '\r' && *id != '\n' )
 			{
 				real_name[i++] = *id ; id++;
 			}
 			
 			//fprintf(stderr , "===%s===%s++++%s(%d)++++\n",strsize , id , real_name , strlen(real_name) );
			
			break;
		}
 	
    /*
     * Some useless lines, safely ignored: 
     */
    /*
     * "Total of 11 Files, 10966 Blocks." (VMS) 
     */
    /*
     * "total 14786" (UNIX) 
     */
    /*
     * "DISK$ANONFTP:[ANONYMOUS]" (VMS) 
     */
    /*
     * "Directory DISK$PCSA:[ANONYM]" (VMS) 
     */
	}
    return 0;
	return false ;
}


void FtpListParse::Dump() 
{


    fprintf(stderr , 
	"name=%s , flagtrycwd=%s ,size=%ld , mtimetype=%d , mtime= %ld, id=%s \n " , 
	    this->name ,
	    this->flagtrycwd ,
	    this->size ,
	    this->mtimetype ,
	    this->mtime ,
	    this->id 
    ) ;
    
}



