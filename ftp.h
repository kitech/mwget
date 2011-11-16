

#ifndef FTP_H
#define FTP_H


#include "ksocket.h"
#include "headers.h"
#include "url.h"
#include "retrieve.h"

class FtpRetreiver : public Retriever
{
public:
	FtpRetreiver ( int ts , const char * url ) ;
	~FtpRetreiver ( ) ;

	KSocket * GetDataSocket();
	bool TaskValid(  long bsize = 0  ) ;
	 bool TaskFinish();
	
protected:

private:

	bool PrepairData() ;

	bool InteractiveOnce( const char * cmd ) ; 
	bool SendCmd(const char * cmd );
	bool RecvCmd( ) ;
	bool PasvSend();
	bool Login(const char * user, const char * pass ) ;
	bool Logout( ) ;

	bool TypeSend();
	bool ListSend(const char * filename );
	bool RestSend( long startpos );
	bool RetrSend( const char * filename ); 
	
};


#ifdef __cplusplus
extern "C"{
#endif

bool ftp_file_valid(KSocket * csock , URL * furl , KSocket *dsock , long * size );


bool ftp_interactive_once(KSocket * csock , const char * cmd);
bool ftp_send_cmd(KSocket * csock , const char * cmd);
bool ftp_recv_cmd(KSocket * csock);

bool ftp_pasv_send(KSocket *csock , KSocket * dsock );

bool ftp_login(KSocket *csock ,const char * user , const char * pass );

bool ftp_logout(KSocket *csock);

#ifdef __cplusplus
}
#endif



#endif
