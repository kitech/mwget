
#ifndef MSGBUS_H
#define MSGBUS_H

#include <unistd.h>

class MsgBus
{
public:
	enum { MB_INVALID , MB_PIPE , MB_FIFO ,
		MB_MSGQUEUE , MB_SHM , MB_SOCKET, MB_MMAP , 
		MB_SPIPE, MB_NPIPE } ;
		
	enum { PEER_A , PEER_B } ;
		
	MsgBus ( int bType ) ;
	~MsgBus ( ) ;	

	virtual bool SendMsg (  int peer ,  const char * msg ) = 0 ;
	virtual bool RecvMsg (  int peer ,   char * msg , int *len ) = 0 ;
	virtual int GetReadFD ( int peer  ) = 0 ;
protected:

	char mSendBuffer[1024];
	char mRecvBuffer[1024];

	int mBusType ;
	int mTimeout ;
	int mBlockBus;
	
private:
	

};


class  PipeBus : public MsgBus
{
public:

	
	
	PipeBus (  ) ;
	~PipeBus ( ) ;	

	bool SendMsg ( int peer ,  const char * msg )  ;
	bool RecvMsg (  int peer ,   char * msg , int *len )  ;
	int GetReadFD ( int peer  ) ;
	void Dump( ) ;
	
protected:


	
private:	
	int mAToBValid ;
	int mBToAValid ;
	int mAToBFD[2] ; //0 Ϊ���򿪣�1Ϊд��
	int mBToAFD[2] ;	//��U ( user ui , write msg ) ---> K ( kernel , dl thread , read msg) �Ĺܵ�
	
/*
1. ��˫���ܵ�����������������(svr4Ϊȫ˫��,��֮Ϊ���ܵ�)
2. ֻ���ھ��й������ȵĽ��̼�ʹ��
*/
};

void test_pipe_bus( ) ;




#endif

