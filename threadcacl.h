

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
限速算法:
	计算上次限速时到现在的时间差
	计算本次读取数据节省的时候，比预期时间少用了多少时间
	如果该时间大于某个上限值，睡眠节省了的时间加上次睡眠调整时间
	如果小于该上限，则，本次不睡眠
	取本次实际睡眠时间与期望时间之差(由于睡眠函数精确度引起)，
		如果该差大于某上限，则圆整到该上限
		如果该差小于某下限，则圆整到该下限(该差可能这负)
		将该差值保存，供下次睡眠时使用

	最后，离开该次限速时清除下载字节数，并重设起始时间

*/

#endif

