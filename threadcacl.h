

#ifndef THREADCACL_H
#define THREADCACL_H

#include <pthread.h>

#include "mwget.h"

class ThreadCacl
{
public:

	~ThreadCacl();
	static ThreadCacl * Instance();	
	bool ResizeParamCount(int pCnt);
	bool SetTaskSize(long pSize);
	long GetTaskSize();
	bool SetTaskUrl(const char * pUrl);
	bool CaclTask(long pTotal , int pCnt = -1 ,  const char * pUrl = 0 );
	bool ReCaclTask(int pNo);
	thread_params * GetThreadParam(int pNo);
	
	long GetTotalSize(int pNo);
	long GetGotSize(int pNo);
	long GetBeginPosition(int pNo);
	struct timeval * GetBeginTime(int pNo);
	struct timeval * GetLastTime(int pNo );
	
	
	bool SetTotalSize(int pNo , long pSize );
	bool SetGotSize(int pNo , long pSize );
	bool SetBeginPosition(int pNo , long pSize );	
	bool SetBeginTime(int pNo , struct timeval * pTV = 0 );
	bool SetLastTime(int pNo  , struct timeval * pTV = 0 );

	void DumpTask(int pNo = -1 ) ;
	int GetTaskCount();

	bool CancelTask( int pNo ) ;
	void SetSubTaskState( int pNo , int state);
	
	bool IncreateGotSize( int pNo  , long pSize  ) ;

	void    BeginTask(  int pNo  ) ;
	double GetTaskRate(int pNo  , char * buff ) ;
	double GetGlobalRate( char * buff , long *totallen , long *gotlen ) ;

	void    LimitBandWidth (  int pNo , int bytes ) ;

	void CreateBarImage(char *barstr , long total , long got , char * ratestr ) ;
private:
	ThreadCacl();	

	static ThreadCacl * mPthCl;
	thread_params **mPthPrm;

	int mActPrmCnt ;
	int mMaxPrmCnt ;
	long mTaskSize;
	long mTaskGotSize ;

	char mTaskUrl[255];

	//used for limit rate 
	int chunk_bytes ;
	struct timeval chunk_start ;
	double sleep_adjust ;	

	struct timeval mTaskBeginTime;
	struct timeval mTaskEndTime;

	pthread_mutex_t mGcMutex; 
private:
	
	double TimeDiff( struct timeval * btime , struct timeval * etime ) ;
};


/*
�����㷨:
	�����ϴ�����ʱ�����ڵ�ʱ���
	���㱾�ζ�ȡ���ݽ�ʡ��ʱ�򣬱�Ԥ��ʱ�������˶���ʱ��
	�����ʱ�����ĳ������ֵ��˯�߽�ʡ�˵�ʱ����ϴ�˯�ߵ���ʱ��
	���С�ڸ����ޣ��򣬱��β�˯��
	ȡ����ʵ��˯��ʱ��������ʱ��֮��(����˯�ߺ�����ȷ������)��
		����ò����ĳ���ޣ���Բ����������
		����ò�С��ĳ���ޣ���Բ����������(�ò�����⸺)
		���ò�ֵ���棬���´�˯��ʱʹ��

	����뿪�ô�����ʱ��������ֽ�������������ʼʱ��

*/

#endif

