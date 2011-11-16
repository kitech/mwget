
#include "real.h"
#include "rtsp_session.h"

#include "rtspcore.h"


RTSPRetriever::RTSPRetriever ( int ts ) 
	: Retriever( ts ) 
{

}
	
RTSPRetriever::~RTSPRetriever () 
{
}

bool RTSPRetriever::TaskValid(long bsize )
{

	return false ;
	
}

	
void test_rtsp_protocol()
{
	bool dret = false ;
	char ftpcmd[100] = {0};
	char rbuf[1024] = {0};
	std::string allhr ;
	int rlen = 0 ;
	int hlen = 0 ;
	int clen = 0 ;
	int nleft = 0 ;

	char * s = 0 ;
	
	HttpHeader hr ; 
	KSocket * csock = new KSocket() ;
	std::string session ;
	std::string challenge1 , challenge2 , checksum ;

	dret = csock->Connect( "localhost", 554 ) ;
	if( dret )	printf("connect ok\n");
	else printf("connect error\n");
	
	hr.AddHeader( "CSeq", "1");
	hr.AddHeader( "User-Agent", "RealMedia Player Version 6.0.9.1235 (linux-2.0-libc6-i386-gcc2.95)");
	hr.AddHeader( "ClientChallenge", "9e26d33f2984236010ef6253fb1887f7");
	hr.AddHeader( "PlayerStarttime", "[28/03/2003:22:50:23 00:00]");
	hr.AddHeader( "CompanyID", "KnKV4M4I/B2FjJ1TToLycw==");
	hr.AddHeader( "GUID", "00000000-0000-0000-0000-000000000000");
	hr.AddHeader( "RegionData", "0");
	hr.AddHeader( "ClientID", "Linux_2.4_6.0.9.1235_play32_RN01_EN_586");
	hr.AddHeader( "Pragma", "initiate-session");

	allhr = "OPTIONS rtsp://localhost:554 RTSP/1.0\r\n";
	allhr += hr.HeaderString() ;

	printf(allhr.c_str());

	dret = csock->Write(allhr.c_str(), allhr.length());
	if( dret )	printf("send ok\n");
	else printf("send error\n");

	csock->Reads(rbuf) ;

	printf(rbuf ) ;

	hr.Parse(rbuf);
	//hr.Dump() ;

	session = hr.GetHeader( "Session" ) ;
	challenge1 = hr.GetHeader ( "RealChallenge1");
	printf("Get RealChallenge1: %s\n", challenge1.c_str());
	
	//////////////
	hr.ClearHeaders();
	hr.AddHeader( "CSeq", "2");
	hr.AddHeader( "Accept", "application/sdp");
	hr.AddHeader( "Session", session );
	hr.AddHeader( "Bandwidth", "10485800");
	hr.AddHeader( "SupportsMaximumASMBandwidth", "1");
	hr.AddHeader( "GUID", "00000000-0000-0000-0000-000000000000");
	hr.AddHeader( "Require", "com.real.retain-entity-for-setup");

	allhr = "DESCRIBE rtsp://localhost/10.rm RTSP/1.0\r\n";
	allhr += hr.HeaderString() ;
	printf(allhr.c_str());

	dret = csock->Write(allhr.c_str(), allhr.length());
	if( dret )	printf("send ok\n");
	else printf("send error\n");

	do
	{
		rlen = csock->Peek(rbuf + rlen  , sizeof(rbuf) );
		if( rlen <= 0 ) break ;
		if( ( s = strstr(rbuf ,"\r\n\r\n") ) != 0 )
		{
			hlen = s - rbuf + 4 ;
			break ;
		}
			
	}while(true);
		
	hr.ClearHeaders();
	hr.Parse(rbuf);
	
	std::string status = hr.GetHeader( "Status" ) ;

	//hr.Dump() ;

	memset(rbuf , 0 , sizeof(rbuf));
	csock->Read(rbuf , hlen ) ;

	printf(rbuf );

	nleft = clen = atoi(hr.GetHeader( "Content-length" ).c_str() );

	nleft = 0 ;
	printf("read content\n");
	while(nleft < clen )
	{
		memset(rbuf, 0 , sizeof(rbuf)  );
		rlen = csock->Read( rbuf , sizeof(rbuf)-1);
		if ( rlen >=0 ) nleft += rlen ;
		printf(rbuf);
	}


	///////////////¼ÆËãchallenge2¼°checksum
	char challenge1_buff[128] = {0};
	char challenge2_buff[128] = {0};
	char checksum_buff[128] = {0} ;
	strcpy(challenge1_buff,challenge1.c_str());
	real_calc_response_and_checksum(challenge2_buff,checksum_buff,challenge1_buff);
	
	printf("\nsend setup \n");
	hr.ClearHeaders();
	hr.AddHeader( "CSeq", "3");
	//hr.AddHeader( "RealChallenge2", "e6de874aa5e64dd35f056f3a7ab857a701d0a8e3, sd=e8a45675");
	hr.AddHeader( "RealChallenge2", std::string(challenge2_buff)+", sd="+std::string(checksum_buff) );
	hr.AddHeader( "Transport", "x-pn-tng/tcp;mode=play");
	hr.AddHeader( "If-Match", session  );
	
	allhr = "SETUP rtsp://localhost/10.rm/streamid=0 RTSP/1.0\r\n";
	allhr += hr.HeaderString() ;
	printf(allhr.c_str());

	dret = csock->Write(allhr.c_str(), allhr.length());
	if( dret )	printf("send ok\n");
	else printf("send error\n");
	memset(rbuf, 0 , sizeof(rbuf)  );
	csock->Reads(rbuf) ;
	printf(rbuf ) ;


	///////////////
	printf("\nsend setup2 \n");
	hr.ClearHeaders();
	hr.AddHeader( "CSeq", "4");
	hr.AddHeader( "Transport", "x-pn-tng/tcp;mode=play");
	hr.AddHeader( "Session", session  );
	
	allhr = "SETUP rtsp://localhost/10.rm/streamid=1 RTSP/1.0\r\n";
	allhr += hr.HeaderString() ;
	printf(allhr.c_str());

	dret = csock->Write(allhr.c_str(), allhr.length());
	if( dret )	printf("send ok\n");
	else printf("send error\n");
	memset(rbuf, 0 , sizeof(rbuf)  );
	csock->Reads(rbuf) ;
	printf(rbuf ) ;

	///////////////
	printf("\nsend SET_PARAMETER  \n");
	hr.ClearHeaders();
	hr.AddHeader( "CSeq", "5");
	hr.AddHeader( "Subscribe", "stream=0;rule=0,stream=0;rule=1,stream=1;rule=0,stream=1;rule=1");
	hr.AddHeader( "Session", session  );
	
	allhr = "SET_PARAMETER rtsp://localhost/10.rm RTSP/1.0\r\n";
	allhr += hr.HeaderString() ;
	printf(allhr.c_str());

	dret = csock->Write(allhr.c_str(), allhr.length());
	if( dret )	printf("send ok\n");
	else printf("send error\n");
	memset(rbuf, 0 , sizeof(rbuf)  );
	csock->Reads(rbuf) ;
	printf(rbuf ) ;

	///////////////
	printf("\nsend PLAY   \n");
	hr.ClearHeaders();
	hr.AddHeader( "CSeq", "6");
	hr.AddHeader( "Session", session  );
	hr.AddHeader( "Range", "npt=80.000-");

	
	allhr = "PLAY rtsp://localhost/10.rm RTSP/1.0\r\n";
	allhr += hr.HeaderString() ;	

	//allhr = "PLAY rtsp://localhost/10.rm RTSP/1.0\r\n"
	//		"CSeq: 6\r\n"
	//		"Session: "+session+"\r\n" 			
	//		"Range: npt=0.00-\r\n\r\n";
		
	printf(allhr.c_str());

	dret = csock->Write(allhr.c_str(), allhr.length());
	if( dret )	printf("\nsend ok\n");
	else printf("\nsend error\n");
	memset(rbuf, 0 , sizeof(rbuf)  );
	csock->Reads(rbuf) ;
	printf("\n============================\n");
	printf(rbuf ) ;
	printf("\n============================\n");
	
	////////getparam position now 
	
	printf("\nGetParam now\n");
	hr.ClearHeaders();
	hr.AddHeader( "CSeq", "7");
	hr.AddHeader( "Session", session  );
	hr.AddHeader( "Content-Type", "application/x-rtsp-mh");	
	hr.AddHeader( "Content-Length","9");
	allhr = "GET_PARAMETER rtsp://localhost/10.rm RTSP/1.0\r\n";
	allhr += hr.HeaderString();
	allhr += "position\n";

	printf(allhr.c_str());

	dret = csock->Write(allhr.c_str(), allhr.length());
	if( dret )	printf("\nsend ok\n");
	else printf("\nsend error\n");
	memset(rbuf, 0 , sizeof(rbuf)  );
	csock->Reads(rbuf) ;
	printf(rbuf ) ;	
	
	
	
	printf("\nreturn ing  %d \n" , session.length());
	
	delete csock ;

}


