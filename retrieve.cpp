#include <stdlib.h>
#include <string.h>

#include "retrieve.h"

TaskParam * TaskParam::mTP = 0 ;

TaskParam::TaskParam()
{
	mElemNum = 100 ;
	this->mPN = (_param_node**)malloc(sizeof(_param_node*) * this->mElemNum);
	memset(this->mPN,0,sizeof(_param_node*) * this->mElemNum) ;
	for( 	int i = 0 ; i < this->mElemNum ; i ++ )
	{
		this->mPN[i] = (_param_node*)malloc(sizeof(_param_node));
		memset(this->mPN,0, sizeof( _param_node ) ) ;
	}
}

TaskParam *TaskParam::Instance()
{
	if ( TaskParam::mTP == 0 )
	{
		TaskParam::mTP = new TaskParam();
	}

	return TaskParam::mTP ;
}

TaskParam::~TaskParam()
{
	for( 	int i = 0 ; i < this->mElemNum ; i ++ )
	{
		delete this->mPN[i] ;		this->mPN[i] = 0 ;
	}	
	delete this->mPN ; 	this->mPN = 0 ;
	
	if( TaskParam::mTP != 0 )
		delete TaskParam::mTP;
	TaskParam::mTP = 0 ;
	
}


Retriever::Retriever(  int ts   )
{
	this->mTP = TaskParam::Instance() ;
	this->mCtrlSock = new KSocket ( ) ;
	this->mTranSock = new KSocket () ;
	this->mUrl  = new URL ( ) ;
	this->mTaskSeq = ts ;
	this->mContentLength = 0 ;
	this->mContentBegin = 0 ;
}

Retriever::~Retriever()
{
	//fprintf(stderr, "HHHHHHHHHHH\n");
	delete this->mTranSock ; this->mTranSock  = 0 ;
	delete this->mCtrlSock ; this->mCtrlSock = 0 ;	
	delete this->mUrl ; this->mUrl =  0 ;	
}

void Retriever::SetTaskSeq(int ts) 
{
	this->mTaskSeq = ts ;
}

bool Retriever::TaskConnect() 
{
	bool bret ;
	bret = this->mCtrlSock->Connect( this->mUrl->GetHost(),  this->mUrl->GetPort() ) ;

	return bret ;
}
bool Retriever::TaskFinish()
{
	this->mTranSock->Close();
	this->mCtrlSock->Close() ;
	return true ;
}

int    Retriever::RetrData( char * rbuff , int rlen  ) 
{
	int tlen = this->mTranSock->Read( rbuff, rlen ) ;

	return tlen ;
}

bool Retriever::PrepairData() 
{
	return true ;
}

long Retriever::GetContentLength()
{
	return this->mContentLength ; 
}

bool Retriever::SetContentBegin( long size )  
{
	this->mContentBegin = size ;
	return true ;
}
