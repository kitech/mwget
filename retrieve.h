
#ifndef RETRIEVE_H
#define  RETRIEVE_H

#include "mwget.h"
#include "ksocket.h"
#include "headers.h"
#include "threadcacl.h"
#include "url.h"

class TaskParam
{
public:
	~TaskParam() ;	
	static TaskParam * Instance();	
	bool Inc(int no  , long size);
	bool Dec(int no , long size);
	
	
private:

	TaskParam();
	static TaskParam * mTP;

	struct _param_node
	{
		int m_thread_no;		//
		int m_thread_count;
		
		long m_total_size ;
		long m_got_size ;
		long m_begin_position;	//
		
		char m_file_url [255] ;
		struct timeval m_begin_time;		//use gettimeofday get milisecond value
		struct timeval m_last_time;

		int chunk_bytes ;
		struct timeval chunk_start ;
		double sleep_adjust ;


		//
		int state;	//ready,retrive,finish,error
		
	};

	_param_node ** mPN;
	
	long mTaskSize ;

	int  mElemNum;
	
};



class Retriever
{
public:
	Retriever( int ts );
	~Retriever();

	virtual bool TaskValid( long bsize = 0 )  = 0 ; 
	bool TaskConnect() ;
	virtual bool TaskFinish();
	virtual bool PrepairData() ;
	virtual long GetContentLength()  ;
	virtual bool SetContentBegin( long size )  ;
	virtual KSocket * GetDataSocket() = 0 ;
	int    RetrData( char * rbuff , int rlen  ) ;
	void SetTaskSeq(int ts) ;
	
protected:	
	int mTaskSeq ;
	KSocket * mCtrlSock ;
	KSocket * mTranSock;
	URL  * mUrl ;
	long   mTaskSize ;
	long   mGotSize ;
	long   mBeginPostion;
	long   mTotalSize;

	long mContentLength ;
	long mContentBegin;

	TaskParam * mTP ;
	
private:

	

};





#endif

