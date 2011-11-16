


/**
  * created by icephoton drswinghead@163.com 2005-8-5
  */
#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>	
#include <unistd.h>	
#include <errno.h>	
#include <string.h>	
#include <sys/socket.h>	
#include <sys/select.h>
#include <netinet/in.h>	
#include <arpa/inet.h>	
#include <sys/wait.h>	
#include <netdb.h>	
#define SOCKET	int



#include "ksocket.h"


KSocket::KSocket(int pFamily , int pType , int pProtocol)
{

	this->init(pFamily, pType, pProtocol);

}

/**
  * private
  *
  */
bool KSocket::init(int pFamily , int pType , int pProtocol)
{
	this->mSockFD = -1 ;
	memset(this->mErrorStr ,0 , sizeof( this->mErrorStr) ) ;
			
	this->mSockFD = socket(pFamily , pType , pProtocol);

  	/* For setting options with setsockopt. */ 
  	int setopt_val = 1;  
  	void *setopt_ptr = (void *)&setopt_val;  
  	socklen_t setopt_size = sizeof (setopt_val);
	//setsockopt(this->mSockFD, SOL_SOCKET, SO_REUSEADDR,setopt_ptr,setopt_size);

	if( this->mSockFD  == -1  )
	{
		return false ;
	}

	this->mFamily = pFamily ;
	this->mType = pType;
	this->mProtocol = pProtocol;

	this->mTimeOut = 6 ;
	this->mTimeOutUS = 0 ;
	this->mTORetry = 5 ; 
	
	return true ;
}

/**
  *
  */

bool KSocket::Connect(const char * pServer, int pPort)
{

	if ( pServer == NULL || pPort <=0 )
	{
		return false ;
	}

	if ( this->mSockFD = -1 )
	{
		this->init();
		if ( this->mSockFD == -1 )
		{
			return false ;
		}
	}


	int ret =-1 ;
	struct hostent *host;
	struct sockaddr_in tempaddr;

	memset(&tempaddr,0,sizeof(tempaddr));

	tempaddr.sin_family = this->mFamily;
	tempaddr.sin_port = htons(pPort) ;

	if((host = gethostbyname(pServer)) == NULL)  	return false;

	tempaddr.sin_addr.s_addr = *((unsigned long *) host->h_addr);

	ret = connect(this->mSockFD,(struct sockaddr *)&tempaddr , sizeof(struct sockaddr_in));

	if ( ret == -1 )
	{
		this->mErrorNo = errno;
		strcpy((char*)this->mErrorStr , strerror(errno));
		return false ;
	}
	
	return true ;
}


/**
  * 向套接口写数据
  */
  /*
int KSocket::Write(const char * pData, int pLength)
{

	if ( pData == NULL || pLength < 0 || this->mSockFD < 0 )
	{
		return -1;
	}

	int len = write(this->mSockFD , pData , pLength);

	return len ;

	return  -1 ;
}
*/
int KSocket::Write(const char * pData, int pLength)
{

	if ( pData == NULL || pLength <= 0 || this->mSockFD < 0 )
	{
		return -1;
	}
	
	char *readptr = (char*)malloc(pLength+1);
	memcpy(readptr,pData,pLength);
	readptr[pLength] = '\0' ;
	char  *ptr = readptr ;
	char tmp[8096];	
	int len =0;
	int n = 0 ;
	int toRetry = 0 ;
	int errRetry = 0 ;
	int perReadLen = pLength < sizeof(tmp) ? pLength : sizeof(tmp);
	long leftlen = pLength ;
	long bufflen = sizeof(tmp);

	
	fd_set readSet;
	timeval tv;
	int maxFD ;

	FD_ZERO(&readSet);
	int selectRet = -5 ;

	//fprintf(stderr, "enter write %d + %d \n===\n%s\n===\n" , this->mTimeOut , pLength , ptr );
	for ( ;; )
	{
		FD_ZERO(&readSet);
		FD_SET(this->mSockFD,&readSet);
		maxFD = this->mSockFD + 1 ;

		tv.tv_sec = this->mTimeOut;
		tv.tv_usec = 0 ;
	
		selectRet = select ( maxFD, NULL , &readSet ,NULL , &tv );
		
		//对错误次数进行限制
		if ( selectRet == 0 ) 
		{
			//printf("(W)无限循环中select = %d   超时\n" , selectRet );

			if ( toRetry ++ >= this->mTORetry )
			{
				//printf("(W)  超时 退出\n" );
				break ;
			}
			else
			{
				continue ;
			}			
		}
		else if ( selectRet < 0 )
		{
			//printf("(W)无限循环中select = %d   未知错误%s \n" , selectRet , strerror(errno)  );
			if ( errRetry ++ >= this->mTORetry )
			{
				break ;
			}
			else
			{
				continue ;
			}
		}
		
		if ( FD_ISSET(this->mSockFD , & readSet ) )
		{
			//memset( tmp , 0 , sizeof( tmp ) );
			n  = write(this->mSockFD , readptr , leftlen  ) ;
			if( n == 0 )
			{
				//文件尾
				break;
			}
			if( n < 0 )
			{
				continue ;	// read data error
			}
			len += n ;
			leftlen -= n ;
			//strncat( pBuffer , tmp , pLength - strlen(pBuffer) -1 );
			//memcpy(readptr,tmp,n);
			readptr = readptr + n ;		//fixed read bug , we cant use str* function here
			if ( leftlen <= 0 )	//read comleted
			{				
				break;
			}						
		}
		

	}
	
	//fprintf(stderr, " exit write %d   \n" , len  );
	free(ptr); ptr = 0 ;
	
	return len ;

	//int len = write(this->mSockFD , pData , pLength);
	return len ;

	return  -1 ;
}

//=
// Sends a buffer of data over the connection
// A connection must established before this method can be called
//=
int KSocket::Writeline(const char *pData, int pLength)
{
	int len = 0 ;
	int len2 = -1; 

	len += this->Write(pData, pLength );
	if ( len < 0 )
	{
		return len;
	}

	len2 = this->Write("\r\n", 2);

	if ( len2 != 2)
	{
		return len2;
	}

	return len + len2 ;

	return -1;

}
	

/**
  * 从套接口读取数据
  */
int KSocket::Read(char * pBuffer, int pLength)
{
	if ( pBuffer == NULL || pLength <= 0 || this->mSockFD < 0 )
	{
		return -1;
	}
	char *readptr = pBuffer ;
	char tmp[8096];	
	int len =0;
	int n = 0 ;
	int toRetry = 0 ;
	int errRetry = 0 ;
	int perReadLen = pLength < sizeof(tmp) ? pLength : sizeof(tmp);
	long leftlen = pLength ;
	long bufflen = sizeof(tmp);
	
	fd_set readSet;
	timeval tv;
	int maxFD ;

	FD_ZERO(&readSet);
	int selectRet = -5 ;

	for ( ;; )
	{
		FD_ZERO(&readSet);
		FD_SET(this->mSockFD,&readSet);
		maxFD = this->mSockFD + 1 ;

		tv.tv_sec = this->mTimeOut;
		tv.tv_usec = 0 ;
	
		selectRet = select ( maxFD, & readSet ,NULL ,NULL , &tv );
		
		//对错误次数进行限制
		if ( selectRet == 0 ) 
		{
			//printf("(R)无限循环中select = %d   超时\n" , selectRet );

			if ( toRetry ++ >= this->mTORetry )
			{
				//printf("(R)  超时 退出\n" );
				break ;
			}
		}
		else if ( selectRet < 0 )
		{
			//printf("(R)无限循环中select = %d   未知错误%s \n" , selectRet , strerror(errno)  );
			if ( errRetry ++ >= this->mTORetry )
			{
				return len ; 
			}
			else
			{
				continue ;
			}
		}
		if ( FD_ISSET(this->mSockFD , & readSet ) )
		{
			memset( tmp , 0 , sizeof( tmp ) );
			n  = read(this->mSockFD , tmp , leftlen > bufflen ? bufflen : leftlen ) ;
			if( n == 0 )
			{
				//文件尾
				break;
			}
			if( n < 0 )
			{
				continue ;	// read data error
			}
			len += n ;
			leftlen -= n ;			
			//strncat( pBuffer , tmp , pLength - strlen(pBuffer) -1 );
			memcpy(readptr,tmp,n);
			readptr = readptr + n ;		//fixed read bug , we cant use str* function here
			if ( leftlen <= 0 )	//read comleted
			{				
				break;
			}						
		}
		

	}

	return len ;

	return -1; 

}

char *KSocket::Reads(char * rbuf)
{
	int rlen  ;
	long tlen = 0 ;
	char *p = rbuf;
	char tbuf[1024] = {0} ;
	
	int res;

	if ( rbuf == NULL ||  this->mSockFD < 0 )
	{
		return 0 ;
	}
	char *dptr = rbuf ;
	char *tptr = rbuf ; 
	
	char tmp[8096];	
	int len =0;
	int n = 0 ;
	int toRetry = 0 ;
	int errRetry = 0 ;
		
	long bufflen = sizeof(tmp);
	
	fd_set readSet;
	timeval tv;
	int maxFD ;
	int tempfd = this->mSockFD  ;
	
	FD_ZERO(&readSet);
	int selectRet = -5 ;


	for ( ;; )
	{
		FD_ZERO(&readSet);
		FD_SET( tempfd , &readSet);
		maxFD = tempfd + 1 ;

		tv.tv_sec = this->mTimeOut;
		tv.tv_usec = 0 ;
	
		selectRet = select ( maxFD, & readSet ,NULL ,NULL , &tv );
		
		//对错误次数进行限制
		if ( selectRet == 0 ) 
		{
			printf("(RS)无限循环中select = %d   超时M %d\n" , selectRet,maxFD );
			break ;
			if ( toRetry ++ >= this->mTORetry )
			{
				//printf("(RS)  超时 退出\n" );
				break ;
			}
		}
		else if ( selectRet < 0 )
		{
			printf("(RS)无限循环中select = %d   未知错误%s \n" , selectRet , strerror(errno)  );
			if ( errRetry ++ >= this->mTORetry )
			{
				return 0 ; 
			}
			else
			{
				continue ;
			}
		}
		
		if ( FD_ISSET( tempfd , & readSet ) )
		{
			//do
			{
				n = recv (tempfd , tbuf, sizeof(tbuf) , MSG_PEEK);
				if( n == 0 ) break;
				if( n < 0 ) break ;	
				tlen += n ; break ;
			}
			//while (n == 0 && errno == EINTR);					
		}

	}
 
	//fprintf(stderr,"tlen  : %d \n",tlen);

	this->Read( rbuf ,  tlen ) ;
	//setsockopt(this->mSockFD, SOL_SOCKET, SO_RCVTIMEO,(void*)&tv ,  sizeof(tv) );
	/*
	while( ( rlen = read(this->mSockFD , p , 255  )) > 0 )
	{
		p = p + rlen ;
		fprintf(stderr,"%d  %s \n",rlen , rbuf);
		if( p - rbuf >= tlen )
			break;
	}
	*/
	return rbuf;
	
}
int  KSocket::Peek(char * rbuf, int pLength) 
{
	//int rlen = recv (this->mSockFD, rbuf, pLength, MSG_PEEK);

	int rlen ;
	long tlen = 0 ;
	char *p = rbuf;
	char tbuf[1024] = {0} ;
	
	int res;

	if ( rbuf == NULL ||  this->mSockFD < 0 || pLength <= 0 )
	{
		return 0 ;
	}
	char *dptr = rbuf ;
	char *tptr = rbuf ; 
	
	char tmp[8096];	
	int len =0;
	int n = 0 ;
	int toRetry = 0 ;
	int errRetry = 0 ;
		
	long bufflen = sizeof(tmp);
	
	fd_set readSet;
	timeval tv;
	int maxFD ;

	FD_ZERO(&readSet);
	int selectRet = -5 ;

	for ( ;; )
	{
		FD_ZERO(&readSet);
		FD_SET(this->mSockFD,&readSet);
		maxFD = this->mSockFD + 1 ;

		tv.tv_sec = this->mTimeOut;
		tv.tv_usec = 0 ;
	
		selectRet = select ( maxFD, & readSet ,NULL ,NULL , &tv );
		
		//对错误次数进行限制
		if ( selectRet == 0 ) 
		{
			//printf("(P)无限循环中select = %d   超时\n" , selectRet );
			break ;
			if ( toRetry ++ >= this->mTORetry )
			{
				//printf("(P)  超时 退出\n" );
				break ;
			}
		}
		else if ( selectRet < 0 )
		{
			//printf("(P)无限循环中select = %d   未知错误%s \n" , selectRet , strerror(errno)  );
			if ( errRetry ++ >= this->mTORetry )
			{
				return 0 ; 
			}
			else
			{
				continue ;
			}
		}
		
		if ( FD_ISSET(this->mSockFD , & readSet ) )
		{
			//do
			{
				n = recv (this->mSockFD, tbuf, sizeof(tbuf) , MSG_PEEK);
				if( n == 0 ) break;
				if( n < 0 ) break ;	
				memcpy(rbuf , tbuf , n ) ;
				tlen += n ; break ;
			}
			//while (n == 0 && errno == EINTR);					
		}

	}

	//fprintf(stderr , "exit peek %d \n" , this ->mSockFD );
	
	return rlen ;
	
}

/**
  * 读取一行数据
  *
  */
int KSocket::Readline(char *pLine , int pLength  )
{

	char tmp[2] ;
	memset(tmp,0,sizeof(tmp));
	char prev = '\0';
	int len = 0 ;
	
	while(this->Read(tmp, 1) == 1)
	{
		if ( prev=='\r' && tmp[0] == '\n')
		{
			strcat(pLine,tmp);
			return len;
		}
		else
		{
			prev = tmp[0] ;
			strcat(pLine,tmp);
			memset(tmp,0,sizeof(tmp));			
			len ++ ;
		}
	}

	return -1; 
}


/**
  * 关闭套接字s
  */


bool KSocket::Close()
{
	
	int ret ;
	if ( this->mSockFD > 0 )
	{ 
		ret = close(this->mSockFD);
		//fprintf(stderr, "Close %d %d %s \n",this->mSockFD , errno, strerror(errno) );
		this->mSockFD = -1;
	}	
	
	return ret == 0 ? true : false  ;
}

/**
  * 
  */
 const char * KSocket::getError()
 {

	return (char*) this->mErrorStr ;
 }


 KSocket::~KSocket()
 {	
	if ( this->mSockFD > 0 )
	{
		this->Close();
	}
 }

int  KSocket::GetSocket()
{
	return this->mSockFD;
}

bool KSocket::SetSocket(KSocket * sock) 
{
	this->mSockFD = sock->GetSocket() ;
	return true ;
}

void KSocket::setTimeout(int seconds, int microseconds)
{
	this->mTimeOut = seconds;
	this->mTimeOutUS = microseconds ;
}

/*

	KSocket * sock = new KSocket();
	if(sock->Connect( "www.chinaunix.com" , 80 ))
	{
		printf("connect ok\n");

		char tmp[8096] ;
		memset(tmp,0,sizeof(tmp));
		char buff[] = "GET / HTTP/1.0\r\n\r\nHost: www.chinaunix.com";
		sock->Writeline(buff, strlen(buff) ) ;

		while(sock->Readline(tmp, sizeof(tmp)) != -1 )
		{
			printf("%s\n",tmp);
			if ( strlen(tmp) == 2 )
			{
				break;
			}
			memset(tmp,0,sizeof(tmp));
			
		}
	}
	else
	{
		printf("connect error %s\n",sock->getError());
	}

	delete sock ;
*/
