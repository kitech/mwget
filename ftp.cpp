
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



#include "ftp.h"


FtpRetreiver::FtpRetreiver (  int ts , const char * url  ) 
	:Retriever( ts )
{
	this->mUrl->Parse( url ) ;
}
FtpRetreiver::~FtpRetreiver ( ) 
{
}

bool FtpRetreiver::TaskValid( long bsize  ) 
{
	KSocket *csock = this->mCtrlSock ;
	KSocket *dsock = this->mTranSock ;
	URL * furl = this->mUrl ; 
	FtpCommand fr ;
	long * size = & bsize ;

	this->mContentBegin = bsize ;
	//connected csock , i assume

	char cmd[32] = {0} ;
	char rbuf[1024] = {0} ;
	char dbuf[1024] = {0} ;
	char host[20] = {0};
	int port = 0 ;
	bool dret = 0 ;

	time_t tb = time(NULL);
	
	dret = this->RecvCmd() ;
	
	dret = this->Login( furl->GetUser(), furl->GetPass());
	
	if( ! dret )
	{
		//fprintf(stderr,"ftp_login error\n");
		return false ;
	}

	if( ! this->TypeSend()  ) return false ;
	
	dret = this->PasvSend( );
	if( ! dret )
	{
		return false ;
	}
	
	//dsock->Reads(rbuf);	//in function pasvsend

	sprintf(cmd , "LIST %s" , furl->GetFullPath());
	dret = this->SendCmd( cmd ) ;

	csock->Reads( rbuf ) ;
	
	//fprintf(stderr,"Main LIST (%d): %d--%s-- \n", csock->GetSocket(), strlen(rbuf) ,  rbuf );
	
	if( ! fr.IsListRespnose( rbuf) && !fr.IsTransferCompleteRespnose(rbuf))
	{
		//csock->Reads(rbuf);
		//fprintf(stderr,"D : %s \n",rbuf );	
		return false ;
	}	

	if(! fr.IsTransferCompleteRespnose(rbuf ) )
	{
		//if not use this "if stmt " ,  this thread will be block;
		tb = time(NULL);
		memset(rbuf, 0 , sizeof(rbuf));
		csock->Reads(rbuf);	// read 226 transfer complete. message.
		//fprintf(stderr,"TTTT : %d  %ld-%ld %WHAT: %s %d %s (%d)\n", 
		//	this->mTaskSeq , tb , time(NULL) , rbuf , error , strerror(error) , csock->GetSocket());
		//fprintf(stderr,"D : %s \n",rbuf );		
	}

	memset(dbuf, 0 , sizeof(dbuf));
	dsock->Reads(dbuf);
	dsock->Close() ;
	fr.SetResponse( dbuf ) ;	
	if ( ! fr.GetLISTSize(  size ) )
	{
		
	}

	//dsock->Close() ;
	//fprintf(stderr,"D : %s \n","关闭数据库传递sock\n" );
	
	if( size <= 0  )
		return false  ;

	this->mContentLength = * size ;	
	
	return true  ;
}

bool FtpRetreiver::InteractiveOnce( const char * cmd ) 
{
	KSocket * csock  = this->mCtrlSock ;
	
	int dret = 0 ;
	char ftpcmd[100] = {0};
	char rbuf[1024] = {0};

	memset(rbuf,0,sizeof(rbuf));
	sprintf( ftpcmd,"%s\r\n",cmd );
	csock->Write( ftpcmd,strlen(ftpcmd));
	csock->Reads( rbuf);	
	//fprintf(stderr,"D : CMD: %s ==> R: %s \n", cmd , rbuf ) ;		

	ftpcmd[strlen(ftpcmd)-2] = '\0';
	fprintf(stderr, "==> %s  ... %s \n",ftpcmd , dret ? "OK":"FAILD");
	
	return dret ;
	
	return true ;
}

bool FtpRetreiver::SendCmd(const char * cmd ) 
{
	KSocket * csock  = this->mCtrlSock ;
	bool dret ;
	char ftpcmd[100] = {0};
	char rbuf[1024] = {0};

	memset(rbuf,0,sizeof(rbuf));
	sprintf( ftpcmd,"%s\r\n",cmd );
	csock->Write( ftpcmd,strlen(ftpcmd));
	//fprintf(stderr,"Send : %s ",cmd);
	
	dret = true ;
	return dret ;
	
	return true ;
}
bool FtpRetreiver::RecvCmd( )  
{
	KSocket * csock  = this->mCtrlSock ;
	bool dret ;
	char ftpcmd[100] = {0};
	char rbuf[1024] = {0};

	csock->Reads( rbuf);	
	//fprintf(stderr,"D : %s \n",rbuf);	

	dret = true ;
	return dret ;	
	return true ;
}
bool FtpRetreiver::PasvSend() 
{
	KSocket * csock  = this->mCtrlSock ;
	KSocket *dsock = this->mTranSock ;
	
	char *cmd = "PASV\r\n";
	char rbuf[1024];
	char host[20] = {0};
	int port = 0 ;
	int dret = 0 ;

	memset(rbuf,0,sizeof(rbuf));
	csock->Write( cmd,strlen(cmd));
	csock->Reads( rbuf);

	FtpCommand fr(rbuf);
	
	if( ! fr.IsPasvRespnose(rbuf)  )
	{
		//csock->Reads( rbuf);
		fprintf(stderr,"PasvSendD : ==> %s  \t",  rbuf   );
		dret =  false ;
	}
	else
	{
		dret = true ;
	}

	fprintf(stderr, "==> %s  ... %s \t","PASV" , dret ? "OK":"FAILD");
	
	if( dret )
	{
		fr.GetPASVAddr(host, &port);	
		dret = dsock->Connect(host, port);
		//fprintf(stderr,"CD : %s:%d  %s  , %s \n", host, port, dret ? "true" : "false" , dsock->getError());		
	}
	fprintf(stderr, "==> %s  ... %s \n","Connect PASV port" , dret ? "OK":"FAILD");

	return dret ;
	
	return true ;
}
bool FtpRetreiver::Login(const char * user, const char * pass )  
{
	KSocket * csock  = this->mCtrlSock ;

	bool dret ;
	char ftpcmd[100] = {0};
	char rbuf[1024] = {0};
	FtpCommand fr ;
	int counter = 0 ;

	sprintf(ftpcmd,"USER %s\r\n",user == 0 ? "anonymous" : user );
	csock->Write( ftpcmd,strlen(ftpcmd));
	//fprintf(stderr,"SEND(%d) <== %s \n",this->mTaskSeq,ftpcmd);
	dret = false ;
	while(1)
	{		
		memset(rbuf,0,sizeof(rbuf));
		csock->Reads( rbuf);
		//fprintf(stderr,"(%d)RECV(%d) ==> %s \n",dret ++ , this->mTaskSeq ,rbuf);
		if( strlen(rbuf) == 0 )
		{			
			break ;
		}
		else
		{
			if( ! fr.IsUserRespnose(rbuf)  )
			{				
				//dret == true ? 1 : dret = false  ;	
				//fprintf(stderr,"Login ret else :  %d  %s +++++++\n" , dret , rbuf );
			}
			else
			{				
				dret = true ;
				break ;
			}			
		}		
	}
	ftpcmd[strlen(ftpcmd)-2] = '\0';
	if(dret )
	{
		fprintf(stderr, "(%d)==> %s  ... %s \n",this->mTaskSeq ,ftpcmd , dret ? "OK":"FAILD");
	}
	else
	{
		fprintf(stderr, "(%d)==> %s  ... %s %s\n",this->mTaskSeq ,ftpcmd , dret ? "OK":"FAILD: ",rbuf );
	}
	
	if( dret == false )
	{
		fprintf(stderr , "out login\n");
		goto retpos ;
	}
	
	//fprintf(stderr,"D (%d): CMD: %s ==> R: %s \n",this->mTaskSeq,ftpcmd,rbuf);	

	memset(rbuf,0,sizeof(rbuf));
	sprintf(ftpcmd,"PASS %s\r\n",pass==0 ? "abcd@phice.org" : pass );
	//fprintf(stderr,"SEND(%d) <== %s \n",this->mTaskSeq,ftpcmd);
	csock->Write( ftpcmd,strlen(ftpcmd));

	dret = false ;
	while(1)
	{		
		memset(rbuf,0,sizeof(rbuf));
		csock->Reads( rbuf);
		//fprintf(stderr,"(%d)RECV(%d) ==> %s \n",dret ++ , this->mTaskSeq ,rbuf);
		if( strlen(rbuf) == 0 )
		{			
			break ;
		}
		else
		{
			if( ! fr.IsPassRespnose(rbuf)  )
			{
				
				//dret == true ? 1 : dret = false  ;	
				//fprintf(stderr,"Login ret else :  %d  %s +++++++\n" , dret , rbuf );
			}
			else
			{				
				dret = true ;
				break ;
			}			
		}		
	}
	//fprintf(stderr,"RECV(%d) ==> %s \n",this->mTaskSeq ,rbuf);

	//fprintf(stderr,"Login Ftp : OK %s  \n" , rbuf );

	//dret = true ;
	ftpcmd[strlen(ftpcmd)-2] = '\0';
	if(dret )
	{
		fprintf(stderr, "(%d)==> %s  ... %s \n",this->mTaskSeq ,ftpcmd , dret ? "OK":"FAILD");
	}
	else
	{
		fprintf(stderr, "(%d)==> %s  ... %s %s\n",this->mTaskSeq ,ftpcmd , dret ? "OK":"FAILD: ",rbuf );
	}
	

	retpos:

	
	return dret ;
	
	return true ;
}
bool FtpRetreiver::Logout( )  
{
	KSocket * csock  = this->mCtrlSock ;
		
	bool dret ;
	char ftpcmd[100] = {0};
	char rbuf[1024] = {0};
	FtpCommand fr ;

	/*
	csock->Reads( rbuf);	
	if( ! fr.IsPassRespnose(rbuf)  )
	{
		csock->Reads( rbuf);		
	}		
	fprintf(stderr,"D : NO CMD : %s \n", rbuf);	
	*/
	
	memset(rbuf,0,sizeof(rbuf));
	strcpy(ftpcmd,"QUIT\r\n");
	//fprintf(stderr,"D : CMDDDDDDDDDDDDDDDD: %s ==>  \n",ftpcmd);
	csock->Write( ftpcmd , strlen(ftpcmd) );
	//fprintf(stderr, "TaskFinishaaa\n");
	//fprintf(stderr,"D : CMD: %s ==>  \n",ftpcmd);

	//sock->Reads( rbuf);	

	//fprintf(stderr, "TaskFinish\n");	
	
	
	dret = true ;
	return dret ;
	
	return true ;
}

bool FtpRetreiver::TypeSend()
{
	KSocket * csock  = this->mCtrlSock ;
	
	int dret = 0 ;
	char ftpcmd[100] = {0};
	char rbuf[1024] = {0};

	FtpCommand fr ;

	memset(rbuf,0,sizeof(rbuf));
	sprintf( ftpcmd,"%s\r\n","TYPE I" );
	csock->Write( ftpcmd,strlen(ftpcmd));
	csock->Reads( rbuf);	
	//fprintf(stderr,"D : CMD: %s ==> R: %s \n", cmd , rbuf ) ;		

	if( !fr.IsTypeRespnose(rbuf) )	dret = false ;
	else dret = true ;
	
	ftpcmd[strlen(ftpcmd)-2] = '\0';
	if(dret )
	{
		fprintf(stderr, "(%d)==> %s  ... %s \n",this->mTaskSeq ,ftpcmd , dret ? "OK":"FAILD");
	}
	else
	{
		fprintf(stderr, "(%d)==> %s  ... %s %s\n",this->mTaskSeq ,ftpcmd , dret ? "OK":"FAILD: ",rbuf );
	}
	
	return dret ;
	
	return true ;	
}

bool FtpRetreiver::ListSend( const char * filename )
{
	KSocket * csock  = this->mCtrlSock ;
	
	int dret = 0 ;
	char ftpcmd[100] = {0};
	char rbuf[1024] = {0};

	FtpCommand fr ;

	memset(rbuf,0,sizeof(rbuf));
	sprintf( ftpcmd,"LIST %s\r\n", filename  );
	csock->Write( ftpcmd,strlen(ftpcmd));
	csock->Reads( rbuf);	
	//fprintf(stderr,"D : CMD: %s ==> R: %s \n", cmd , rbuf ) ;		

	if( !fr.IsListRespnose(rbuf) )	dret = false ;
	else dret = true ;
	
	ftpcmd[strlen(ftpcmd)-2] = '\0';
	if(dret )
	{
		fprintf(stderr, "(%d)==> %s  ... %s \n",this->mTaskSeq ,ftpcmd , dret ? "OK":"FAILD");
	}
	else
	{
		fprintf(stderr, "(%d)==> %s  ... %s %s\n",this->mTaskSeq ,ftpcmd , dret ? "OK":"FAILD: ",rbuf );
	}
	
	return dret ;
	
	return true ;	
}


bool FtpRetreiver::RestSend( long startpos )
{
	KSocket * csock  = this->mCtrlSock ;
	
	int dret = 0 ;
	char ftpcmd[100] = {0};
	char rbuf[1024] = {0};

	FtpCommand fr ;

	memset(rbuf,0,sizeof(rbuf));
	sprintf( ftpcmd,"REST %d\r\n", startpos  );
	csock->Write( ftpcmd,strlen(ftpcmd));
	csock->Reads( rbuf);	
	//fprintf(stderr,"D : CMD: %s ==> R: %s \n", cmd , rbuf ) ;		

	if( !fr.IsRestRespnose(rbuf) )	dret = false ;
	else dret = true ;
	
	ftpcmd[strlen(ftpcmd)-2] = '\0';
	
	if(dret )
	{
		fprintf(stderr, "(%d)==> %s  ... %s \n",this->mTaskSeq ,ftpcmd , dret ? "OK":"FAILD");
	}
	else
	{
		fprintf(stderr, "(%d)==> %s  ... %s %s\n",this->mTaskSeq ,ftpcmd , dret ? "OK":"FAILD: ",rbuf );
	}
	
	return dret ;
	
	return true ;	
}


bool FtpRetreiver::RetrSend( const char * filename )
{
	KSocket * csock  = this->mCtrlSock ;
	
	int dret = 0 ;
	char ftpcmd[100] = {0};
	char rbuf[1024] = {0};

	FtpCommand fr ;

	memset(rbuf,0,sizeof(rbuf));
	sprintf( ftpcmd,"RETR %s\r\n", filename  );
	csock->Write( ftpcmd,strlen(ftpcmd));
	csock->Reads( rbuf);	
	//fprintf(stderr,"D : CMD: %s ==> R: %s \n", cmd , rbuf ) ;		

	if( !fr.IsRetrRespnose(rbuf) )	dret = false ;
	else dret = true ;
	
	ftpcmd[strlen(ftpcmd)-2] = '\0';
	if(dret )
	{
		fprintf(stderr, "(%d)==> %s  ... %s \n",this->mTaskSeq ,ftpcmd , dret ? "OK":"FAILD");
	}
	else
	{
		fprintf(stderr, "(%d)==> %s  ... %s %s\n",this->mTaskSeq ,ftpcmd , dret ? "OK":"FAILD: ",rbuf );
	}
	
	return dret ;
	
	return true ;		
}

KSocket * FtpRetreiver::GetDataSocket()
{
	return this->mTranSock ;
}

bool FtpRetreiver::PrepairData() 
{
	int dret = true ;
	char cmd[32] = {0} ;
	URL * turl = this->mUrl ;
	
	//ftp_pasv_send(sock, dsock) ;
	//sprintf(cmd,"REST %ld\r\n" , tp->m_begin_position + tp->m_got_size );
	//ftp_interactive_once(sock, cmd ); 
	//sprintf(cmd,"RETR %s\r\n" , turl->GetPath());
	//ftp_interactive_once(sock, cmd) ;

	this->PasvSend();

	this->RestSend( this->mContentBegin );

	dret = this->RetrSend(turl->GetFullPath());
	
	return dret ;
}

 bool FtpRetreiver::TaskFinish()
 { 	
	
	this->Logout();
	
	Retriever::TaskFinish();
	
	return true ;
 }

bool ftp_file_valid(KSocket * csock , URL * furl , KSocket *dsock , long * size )
{
	//connected csock , i assume

	char cmd[32] = {0} ;
	char rbuf[1024] = {0} ;
	char host[20] = {0};
	int port = 0 ;
	int dret = 0 ;
	
	dret = ftp_login(csock, furl->GetUser(), furl->GetPass());
	if( ! dret )
	{
		fprintf(stderr,"ftp_login error\n");
		return false ;
	}

	sprintf(cmd,"TYPE I\r\n");
	ftp_send_cmd(csock, cmd);
	ftp_recv_cmd(csock);
	
	dret = ftp_pasv_send(csock, dsock);
	if( ! dret )
	{
		return false ;
	}
	
	//
	sprintf(cmd,"LIST %s\r\n",furl->GetPath());
	dret = ftp_send_cmd(csock, cmd);
	if( ! dret )  
	{	
		return false ;
	}
	csock->Reads(rbuf);
	fprintf(stderr,"D : %s \n",rbuf );
	
	dsock->Reads(rbuf);
	fprintf(stderr,"D : %s \n",rbuf );
	FtpCommand fr(rbuf);
	fr.GetLISTSize(  size) ;

	csock->Reads(rbuf);
	fprintf(stderr,"D : %s \n",rbuf );
	
	if( size <= 0  )
		return false  ;
	
	return true  ;
}

bool ftp_interactive_once(KSocket * csock , const char * cmd)
{
	int dret = 0 ;

	dret = ftp_send_cmd( csock , cmd) ;
	dret = ftp_recv_cmd(csock);
	
	return dret ;
}

bool ftp_pasv_send(KSocket *csock , KSocket * dsock )
{
	char *cmd = "PASV\r\n";
	char rbuf[1024];
	char host[20] = {0};
	int port = 0 ;
	int dret = 0 ;

	memset(rbuf,0,sizeof(rbuf));
	csock->Write( cmd,strlen(cmd));
	csock->Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf );

	FtpCommand fr(rbuf);
	fr.GetPASVAddr(host, &port);

	dret = dsock->Connect(host, port);
	fprintf(stderr,"CD : %s:%d  %s  , %s \n", host, port, dret ? "true" : "false" , dsock->getError());		

	dret = true ;
	return dret ;
	
	return false ;
}

bool ftp_send_cmd(KSocket * csock , const char * cmd )
{
	bool dret ;
	char ftpcmd[100] = {0};
	char rbuf[1024] = {0};

	memset(rbuf,0,sizeof(rbuf));
	sprintf( ftpcmd,"%s\r\n",cmd );
	csock->Write( ftpcmd,strlen(ftpcmd));
	fprintf(stderr,"Send : %s ",cmd);
	
	dret = true ;
	return dret ;

}

bool ftp_recv_cmd(KSocket * csock)
{
	bool dret ;
	char ftpcmd[100] = {0};
	char rbuf[1024] = {0};

	csock->Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);	

	dret = true ;
	return dret ;
}

bool ftp_login(KSocket *csock ,const char * user , const char * pass )
{
	bool dret ;
	char ftpcmd[100] = {0};
	char rbuf[1024] = {0};
	
	sprintf(ftpcmd,"USER %s\r\n",user == 0 ? "anonymous" : user );
	csock->Write( ftpcmd,strlen(ftpcmd));
	csock->Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);	
	
	sprintf(ftpcmd,"PASS %s\r\n",pass==0 ? "abcd@phice.org" : pass );
	csock->Write( ftpcmd,strlen(ftpcmd));
	csock->Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);

	dret = true ;
	return dret ;
}

bool ftp_logout(KSocket *csock)
{
	bool dret ;
	char ftpcmd[100] = {0};
	char rbuf[1024] = {0};

	memset(rbuf,0,sizeof(rbuf));
	strcpy(ftpcmd,"QUIT\r\n");
	csock->Write( ftpcmd,strlen(ftpcmd));
	csock->Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);

	dret = true ;
	return dret ;

}

