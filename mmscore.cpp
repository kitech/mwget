
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "ksocket.h"

#include "mmscore.h"


/* Offsets of some critical information in ASF header */
#define HDR_TOTAL_SIZE_8               	0x28
#define HDR_NUM_PACKETS_8              	0x38
#define HDR_FINE_TOTALTIME_8           	0x40
#define HDR_FINE_PLAYTIME_8            	0x48
#define HDR_PLAYTIME_OFFSET_4          	0x50
#define HDR_FLAGS_4                    	0x58
#define HDR_ASF_CHUNKLENGTH_4          	0x5c
#define HDR_ASF_CHUNKLENGTH_CONFIRM_4  	0x60
#define DATSEG_HDR_SIZE 		0x32
#define DATSEG_NUMCHUNKS_4 		0x28

MMSRetriever::MMSRetriever ( int ts ) 
	:Retriever(ts)
{

}
	
MMSRetriever::~MMSRetriever () 
{

}

bool MMSRetriever::TaskValid(long bsize )
{


	return false ;
}
/* Create a randomized GUID string to avoid filtering ASFRecorder's */
/* stream requests by means of detecting any hardcoded GUIDs */
void randomize_guid( char *buf)
{
	int digit, dig;
	time_t curtime;

	*buf++='{';
	time(&curtime);
	srand(curtime);
	for (digit=0; digit <32; digit++)
	{
		if (digit==8 || digit == 12 || digit == 16 || digit == 20) *buf++='-';
		dig = rand()%0xf;
		if (dig<10)
			*buf++='0'+dig;
		else
			*buf++='A'+(dig-10);
	}
	*buf++='}';
	*buf++='\0';
}

unsigned char ASF_HDR_ID[16] = {
	0xa1,0xdc,0xab,0x8c,0x47,0xa9,0xcf,0x11,0x8e,0xe4,0x00,0xc0,0x0c,0x20,0x53,0x65};

/* get 64bit integer (Little endian) */
void get_quad(unsigned char *pos, unsigned long *hi, unsigned long *lo)
{
	unsigned char c1 = *pos++, c2 = *pos++, c3 = *pos++, c4 = *pos++;
	unsigned char c5 = *pos++, c6 = *pos++, c7 = *pos++, c8 = *pos++;
	(*hi) = (c8<<24) + (c7<<16) + (c6<<8) + c5;
	(*lo) = (c4<<24) + (c3<<16) + (c2<<8) + c1;
}

/* get 32bit integer (Little endian) */
void get_long(unsigned char *pos, unsigned long *val)
{
	unsigned char c1 = *pos++, c2 = *pos++, c3 = *pos++, c4 = *pos++;
	(*val) = (c4<<24) + (c3<<16) + (c2<<8) + c1;
}

/* helper routine for ASF header parsing */
int find_id(unsigned char *id, unsigned char *Buffer, int bufsize)
{
	int offs, i, found;
	for (offs = 0; offs < bufsize-16; offs++) {
		found=1;
		for (i=0; i<16; i++) if (Buffer[offs+i] != id[i]) {
			found = 0;
			break;
		}
		if (found) return(offs);
	}
	return -1;
}

void test_mms_protocol() 
{
	bool dret ;
	char rbuff[10240] = {0};
	char *pos = rbuff ;
	char *s = 0 ;
	char cmd[100] = {0} ;
	char mmsb[16];
	unsigned char * tmptr ;
	
	char uuid[56] = {0};
	KSocket * csock = new KSocket();
	unsigned int BodyLength,Length,Length2;

	unsigned int TotalSizeHi, TotalSizeLo;
	unsigned long TotalTimeHi, TotalTimeLo;
	unsigned long Offset;
	unsigned long Flags;
	unsigned long ChunkLength;
	unsigned long ChunkLength2;
	
	unsigned int Time;
	unsigned int EndOfHeaderOffs;
	
	unsigned long 			FileSize;
	
	char range[50] = {0};
 
 	dret = csock->Connect("localhost", 80 ) ;

	printf("connect mms host : %d \n" , dret );
	
	HttpHeader hr;
	hr.AddHeader( "Accept", "*/*");
	hr.AddHeader( "User-Agent", "NSPlayer/4.1.0.3856");
	hr.AddHeader( "Pragma", "no-cache,rate=1.000000,stream-time=0,stream-offset=0:0,request-context=1,max-duration=0") ;
	randomize_guid(uuid);
	hr.AddHeader( "Pragma", std::string("xClientGUID=") + std::string(uuid));
	hr.AddHeader( "Connection", "Close");
	hr.AddHeader( "GET", "/tutumm.wmv");

	printf(hr.HeaderString().c_str());
	
	char c1 ,c2 ,c3 , c4 ;
	int headend =0;
	printf("=> %s ",cmd);
	csock->Write( hr.HeaderString().c_str(),  hr.HeaderString().length()) ;

	FILE * fp = fopen("tutu.wmv","w+");
	while(csock->Read(cmd, 1) )
	{		
		putchar(cmd[0]);
		
		if( headend == 0 )
		{
			c1=c2;c2=c3;c3=c4;
			c4=cmd[0] ;			
		}
		else
		{
			fwrite(cmd,1,1,fp);
		}
		if( c1=='\r' && c2=='\n' && c3=='\r' && c4=='\n')
		{
			printf("head got\n");
			headend = 1 ;
			c1 = c2 = c3 = c4 = 0 ;
			break;
		}
	}
	tmptr =(unsigned char*) mmsb ;
	csock->Read(mmsb,2);	// 2 B header types "H$", "D$" and "E$" (Header, Data and End) 
	printf("Type:%c%c\n",mmsb[0],mmsb[1]);
	csock->Read(mmsb,2) ; // /*2B ,  read chunk length (max 64k) */
	Length = (tmptr[1]<<8)+tmptr[0] ;
	printf("Chunk Length: %d %d.%d\n",(tmptr[1]<<8)+tmptr[0] , tmptr[1],tmptr[0]);
	csock->Read(mmsb,4) ; // 4B , /* read chunk sequence number */
	printf("Chunk sequence: %d\n",
			(tmptr[3]<<24)+(tmptr[2]<<16)+(tmptr[1]<<8)+tmptr[0] );
	csock->Read(mmsb,2) ; // 2B , /* read two unknown bytes*/
	printf("PartFlag: %d %h\n",(tmptr[1]<<8)+tmptr[0],(tmptr[1]<<8)+tmptr[0]);
	
	csock->Read(mmsb,2) ; // 2B , /*  read second length entry (length confirmation)*/
	Length2 = (tmptr[1]<<8)+tmptr[0] ;
	printf("Length2: %d\n",(tmptr[1]<<8)+tmptr[0]);

	// why sub 8 B //calculate length of chunk body. 
	BodyLength = Length - 8 ;
	printf("BodyLength: %u\n", BodyLength );

	//retrive
	int n = 0 ;
	while(csock->Read(cmd, 1) )
	{
		putchar(cmd[0]);
		fwrite(cmd,1,1,fp);
		rbuff[n++] = cmd[0];
	}
	//
	printf("\nRead Body: %d\n", n );

	tmptr = (unsigned char*) rbuff ;
	int Offs;
	/* find a specific object in this header */
	Offs = find_id(ASF_HDR_ID,tmptr, BodyLength);
	if (Offs == -1)
	{
		printf("Unable to parse this ASF header!\n");
	}	
	else
	{
		printf("Offs : %d !\n",Offs );	
	}
	/* extract required information */

	
	//get_quad(&Buffer[Offs+HDR_TOTAL_SIZE_8], &My_ci->TotalSizeHi, &My_ci->TotalSizeLo);
	get_quad(&tmptr[Offs+HDR_FINE_TOTALTIME_8], &TotalTimeHi, &TotalTimeLo);
	get_long(&tmptr[Offs+HDR_PLAYTIME_OFFSET_4], &Offset);
	get_long(&tmptr[Offs+HDR_FLAGS_4], &Flags);
	get_long(&tmptr[Offs+HDR_ASF_CHUNKLENGTH_4], &ChunkLength);
	get_long(&tmptr[Offs+HDR_ASF_CHUNKLENGTH_CONFIRM_4], &ChunkLength2);	

	unsigned long NumOfPacket;
	get_long(&tmptr[Offs+HDR_NUM_PACKETS_8], &NumOfPacket);
	TotalSizeLo = BodyLength + NumOfPacket*ChunkLength;
	TotalSizeHi = 0;

	FileSize = TotalSizeLo;	
	/* calculate playtime in milliseconds (0 for live streams) */
	if (TotalTimeHi == 0 && TotalTimeLo == 0)
		Time = 0; /* live streams */
	else
		Time = (int)((double)429496.7296 * TotalTimeHi) + (TotalTimeLo / 10000) - Offset;

	/* store position where the ASF header segment ends and the chunk data segment starts */
	EndOfHeaderOffs = BodyLength - DATSEG_HDR_SIZE;	
	
	printf("Parse ASF Head: total time hi : %d total time lo : %d "
		"offset: %d  flags: %d  chunklength: %d chunklength2: %d"
		"\nnumofpacket: %d totalsizelo: %d totalsize Hi: %d filesize: %d \n"
		"TIme: %d endofheaderoffs: %d \n",
		TotalTimeHi,TotalTimeLo,Offset,Flags,ChunkLength,ChunkLength2,
		NumOfPacket,TotalSizeLo,TotalSizeHi,FileSize ,
		Time , EndOfHeaderOffs );	
 	csock->Close(); 

 	csock = new KSocket();
 	dret = csock->Connect("localhost", 80 ) ;

	printf("connect mms host : %d \n" , dret );
	
	hr.ClearHeaders();
	
	hr.AddHeader( "Accept", "*/*");
	hr.AddHeader( "User-Agent", "NSPlayer/9.0.0.2980");
	hr.AddHeader( "Pragma", "no-cache,rate=1.000000,stream-time=0,stream-offset=0:16,request-context=2,max-duration=0") ;
	//Pragma: 				  no-cache,rate=1.000000,stream-time=0,stream-offset=%u:%u,request-context=2,max-duration=0\r\n
	randomize_guid(uuid);
	hr.AddHeader( "Pragma", std::string("xClientGUID=") + std::string(uuid));
	hr.AddHeader( "Connection", "Close");
	hr.AddHeader( "GET", "/tutumm.wmv");
	//hr.AddHeader("Pragma","xPlayStrm=1");
	//hr.AddHeader( "Pragma","stream-switch-count=1");
	//hr.AddHeader( "Pragma","stream-switch-entry=ffff:1:0");
	std::string thr = "Pragma: no-cache,rate=1.000000,stream-time=0,stream-offset=1249:1250,request-context=2,max-duration=0\r\n"
				"Pragma: no-cache,rate=1.000000,request-context=2\r\n"
				"Pragma: xPlayStrm=1\r\n"
				"Pragma: stream-switch-count=1\r\n"
				"Pragma: stream-switch-entry=ffff:1:0\r\n"
				"\r\n";
	thr = hr.HeaderString().substr(0,hr.HeaderString().length()-2) + thr ;
	printf(thr.c_str());
 	csock->Write( thr.c_str(),  thr.length()) ;

	
 	n= 0;
	while(csock->Read(cmd, 1) )
	{
		putchar(cmd[0]);
		fwrite(cmd,1,1,fp);
		//rbuff[n++] = cmd[0];
		n++;
	} 	
	printf("\nRead Body: %d\n", n );
	
	csock->Close(); 
	
	fclose(fp);
	
}


