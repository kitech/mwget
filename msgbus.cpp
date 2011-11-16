
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "msgbus.h"


MsgBus::MsgBus ( int bType  ) 
{
	memset(mSendBuffer , 0 , sizeof(mSendBuffer)) ;
	memset(mRecvBuffer, 0 , sizeof(mRecvBuffer ) ) ;
	if( bType >= MB_INVALID && bType <= MB_NPIPE )
		this->mBusType = bType ;
	else
		this->mBusType = MB_INVALID ;
}
MsgBus::~MsgBus ( ) 
{

}


PipeBus::PipeBus (  )
	: MsgBus ( MsgBus::MB_PIPE ) 
{
	pipe(this->mAToBFD ) ;
	pipe(this->mBToAFD ) ;
}
PipeBus::~PipeBus ( ) 
{
	close(this->mAToBFD[0]);
	close(this->mAToBFD[1]);
	close(this->mBToAFD[0]);
	close(this->mBToAFD[1]);
}

bool PipeBus::SendMsg (  int peer ,  const char * msg )  
{
	int ku , uk ;
	if( peer == PEER_A )
	{
		ku = write(this->mAToBFD[1] , msg , strlen(msg) ) ;
	}
	if ( peer == PEER_B ) 
	{
		uk = write(this->mBToAFD[1] , msg , strlen(msg) );
	}

	fprintf(stderr, "KU : %d  \t UK : %d \n" , ku, uk ) ;
	
	return true ;
}
bool PipeBus::RecvMsg (  int peer ,  char * msg , int *len )  
{
	int ku , uk ;
	char msgku [255] = {0} , msguk[255] = {0} ;
	if( peer == PEER_A )
	{
		ku = read(this->mBToAFD[0] , msgku , 255 );
	}
	if ( peer == PEER_B ) 
	{
		uk = read(this->mAToBFD[0] , msguk ,  255 );
	}

	fprintf(stderr, "KU : %d %s \t UK : %d %s \n" , ku , msgku , uk , msguk ) ;	
	
	return true ;
}

int PipeBus::GetReadFD ( int peer  )
{
	if( peer == PEER_A )
	{
		return this->mBToAFD[0] ;
	}
	if ( peer == PEER_B ) 
	{
		return this->mAToBFD[0] ;
	}
	
	return -1 ;
}
void PipeBus::Dump( ) 
{
	fprintf(stderr,"K->U : %d , %d \t U->K : %d , %d \n",
		this->mAToBFD[0],this->mAToBFD[1],
		this->mBToAFD[0],this->mBToAFD[1] ) ;
}

void test_pipe_bus( ) 
{

	PipeBus * pb = new PipeBus ( ) ;

	pb->Dump() ;
	pb->SendMsg( PipeBus::PEER_A ,"abcd" ) ;
	pb->SendMsg( PipeBus::PEER_A ,"abcd2" ) ;
	pb->RecvMsg( PipeBus::PEER_A , 0 , 0 ) ;
	//pb->SendMsg( PipeBus::PEER_B ,"abcdde" ) ;
	//pb->RecvMsg( PipeBus::PEER_B , 0 , 0 ) ;
	
	delete pb ;
}
