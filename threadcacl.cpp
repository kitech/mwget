#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "threadcacl.h"

ThreadCacl * ThreadCacl::mPthCl = 0 ;
ThreadCacl::ThreadCacl()
{
	this->mActPrmCnt = 0 ;
	this->mMaxPrmCnt = 0 ;
	this->mPthPrm = 0 ;
	this->mTaskSize = 0 ;
	this->mTaskGotSize = 0 ;
	//this->mGcMutex = PTHREAD_MUTEX_INITIALIZER ;
    pthread_mutex_init(&this->mGcMutex, 0);
	memset(&mTaskBeginTime, 0 , sizeof(struct timeval) );
	gettimeofday( &mTaskBeginTime , 0 ) ;
}

ThreadCacl::~ThreadCacl()
{
	if( this->mPthCl != 0 )
	{
		delete this->mPthCl ;
		this->mPthCl = 0 ;
	}
}
ThreadCacl * ThreadCacl::Instance()
{
	if( ThreadCacl::mPthCl == 0 )
	{
		ThreadCacl::mPthCl = new ThreadCacl();
	}

	return ThreadCacl::mPthCl ;
}
bool ThreadCacl::ResizeParamCount(int pCnt)
{
	if( pCnt <= 0 ) return false ;
	
	if(pCnt > this->mMaxPrmCnt)
	{
		this->mPthPrm = (thread_params**)realloc(this->mPthPrm , pCnt*sizeof(thread_params*));
		for(int no=0 ; no < pCnt ; no ++ )
		{
			this->mPthPrm[no] = (thread_params*)malloc(sizeof(thread_params));
			memset(this->mPthPrm[no],0,sizeof(thread_params));
			this->mPthPrm[no]->m_thread_count = pCnt;
			this->mPthPrm[no]->m_thread_no = no ;	
			strcpy(this->mPthPrm[no]->m_file_url,this->mTaskUrl);
			this->mPthPrm[no]->m_state = T_READY ;
		}
		this->mMaxPrmCnt = pCnt ;
		this->mActPrmCnt = pCnt ;
	}
	else
	{
		this->mActPrmCnt = pCnt ;
	}
	return false ;
}

long ThreadCacl::GetTotalSize(int pNo)
{
	if( pNo <= this->mActPrmCnt-1 && pNo >= 0 ) 
	{
		return this->mPthPrm[pNo]->m_total_size ;
	}
	
	return -1;
}
long ThreadCacl::GetGotSize(int pNo)
{
	if( pNo <= this->mActPrmCnt-1 && pNo >= 0 ) 
	{
		return this->mPthPrm[pNo]->m_got_size ;
	}
	return -1;
}
long ThreadCacl::GetBeginPosition(int pNo)
{
	if( pNo <= this->mActPrmCnt-1 && pNo >= 0 ) 
	{
		return this->mPthPrm[pNo]->m_begin_position ;
	}
	return -1;
}

bool ThreadCacl::SetTotalSize(int pNo , long pSize )
{
	if( pNo+1 > this->mMaxPrmCnt)
	{
		this->ResizeParamCount(pNo+1);
	}
	if( pSize > 0 )
	{
		this->mPthPrm[pNo]->m_total_size = pSize ;
		return true;
	}
	return false ;
}
bool ThreadCacl::SetGotSize(int pNo , long pSize )
{
	if( pNo+1 > this->mMaxPrmCnt)
	{
		this->ResizeParamCount(pNo+1);
	}
	if( pSize > 0 )
	{
		this->mPthPrm[pNo]->m_got_size = pSize ;
		return true;
	}
	return false ;
}
bool ThreadCacl::SetBeginPosition(int pNo , long pSize )
{
	if( pNo+1 > this->mMaxPrmCnt)
	{
		this->ResizeParamCount(pNo+1);
	}
	if( pSize > 0 )
	{
		this->mPthPrm[pNo]->m_begin_position = pSize ;
		return true;
	}	
	return false ;
}

bool ThreadCacl::CaclTask(long pTotal , int pCnt  ,  const char * pUrl )
{
	if( pCnt == -1)
		pCnt = this->mActPrmCnt ;
	else
	{
		this->mActPrmCnt = this->mMaxPrmCnt = pCnt ;
	}
	this->mTaskSize = pTotal;
	if( pUrl != 0)
		strcpy(this->mTaskUrl,pUrl );

	long sizePerThread ;
	if( pCnt > 0 && pTotal > 0 )
	{
		sizePerThread = pTotal / pCnt ;
		
		this->ResizeParamCount(pCnt);
		for( int no = 0 ; no < pCnt  ; no ++ )
		{
			this->mPthPrm[no]->m_begin_position = no * sizePerThread ;
			this->mPthPrm[no]->m_got_size = 0 ;
			this->mPthPrm[no]->m_total_size = sizePerThread;
			this->mPthPrm[no]->m_thread_count = pCnt  ;
			this->mPthPrm[no]->m_thread_no = no ;
			strcpy(this->mPthPrm[no]->m_file_url,this->mTaskUrl);
			this->mPthPrm[no]->m_state = T_READY ;
		}
		this->mPthPrm[pCnt-1]->m_total_size = pTotal - (pCnt-1)*sizePerThread ;

		return true ;
	}
	
	return false ;
}

bool ThreadCacl::ReCaclTask(int pNo)
{
	if( pNo+1 > this->mMaxPrmCnt)
	{
		this->ResizeParamCount(pNo+1);
	}
	long total , begin , got , left = 0  ;
	thread_params * maxtc ;
	int  minsplitsize = 1024 ;
	int  binleft ;
	
	//fprintf(stderr, "recacl task %d begin a\n",pNo );
	
	if( this->mPthPrm[pNo]->m_got_size < this->mPthPrm[pNo]->m_total_size )
	{
		return false ;
	}
	for( int no = 0 ; no < this->mActPrmCnt  ; no ++ )
	{
		//fprintf(stderr, "recacl task %d begin b\n",pNo );
		if( no == pNo ) continue ;
		if(this->mPthPrm[no]->m_state == T_CLOSE ) continue ;
		if( this->mPthPrm[no]->m_total_size - this->mPthPrm[no]->m_got_size > left )
		{
			left = this->mPthPrm[no]->m_total_size - this->mPthPrm[no]->m_got_size ;
			maxtc = this->mPthPrm[no] ;
		}
		//fprintf(stderr, "recacl task %d begin c\n",pNo );
	}
	//fprintf(stderr,"recacl %ld \n",left);
	if( maxtc->m_state == T_STOP )
	{		
		this->mPthPrm[pNo]->m_begin_position = maxtc->m_begin_position + maxtc->m_got_size ;
		this->mPthPrm[pNo]->m_total_size =   left  ;
		this->mPthPrm[pNo]->m_got_size = 0 ;
		this->mPthPrm[pNo]->m_state = T_READY ;
		
		maxtc->m_total_size = maxtc->m_total_size - left ;
		maxtc->m_state = T_CLOSE ;

		fprintf(stderr , "got error stopped task %d by %d \n",maxtc->m_thread_no, pNo );
		
		return true ;
	}
	else
	{
		if ( maxtc->m_state == T_ERROR )
		{
			minsplitsize = 1024 ;
		}
		else
		{
			minsplitsize = 10240 ;
		}
		if( left > minsplitsize )
		{
			binleft = left / 2 ;
			
			this->mPthPrm[pNo]->m_begin_position = maxtc->m_begin_position + maxtc->m_total_size  -( binleft );
			this->mPthPrm[pNo]->m_total_size =  ( binleft ) ;
			this->mPthPrm[pNo]->m_got_size = 0 ;
			this->mPthPrm[pNo]->m_state = T_READY ;
			
			maxtc->m_total_size = maxtc->m_total_size - ( binleft ) ;
			
			return true ;
		}
		else
		{
			this->mPthPrm[pNo]->m_begin_position = this->mTaskSize;
			this->mPthPrm[pNo]->m_total_size =  0 ;
			this->mPthPrm[pNo]->m_got_size = 0 ;

			this->mPthPrm[pNo]->m_state = T_CLOSE ;
			return true;
		}
	}

	
	return false ;
}

/**
 * 取消一个子任务
 *
 * 当该子任务错误退出而没有完成它的任务时调用
 *
 *
 */
bool ThreadCacl::CancelTask( int pNo ) 
{
	if( pNo <= this->mActPrmCnt-1 && pNo >= 0 ) 
	{
		return false ;
	}	

	
	
	return false ;
}

void ThreadCacl::SetSubTaskState(int pNo , int state)
{
	
	if( pNo <= this->mActPrmCnt-1 && pNo >= 0 ) 
	{
		if ( state > T_BEGIN && state <= T_END )
		{
			int c_state = this->mPthPrm[pNo]->m_state ;
			this->mPthPrm[pNo]->m_state = state ;
		}
	}
	//fprintf(stderr , "Set %d state %d end \n",pNo , state );
}

long ThreadCacl::GetTaskSize()
{
	return this->mTaskSize ;
}
struct timeval * ThreadCacl::GetBeginTime(int pNo)
{
	if( pNo <= this->mActPrmCnt-1 && pNo >= 0 ) 
	{
		return &(this->mPthPrm[pNo]->m_begin_time) ;
	}
	
	return 0;
}
struct timeval * ThreadCacl::GetLastTime(int pNo )
{
	if( pNo <= this->mActPrmCnt-1 && pNo >= 0 ) 
	{
		return &(this->mPthPrm[pNo]->m_last_time) ;
	}
	
	return 0;
}
bool ThreadCacl::SetBeginTime(int pNo , struct timeval * pTV  )
{
	if( pNo+1 > this->mMaxPrmCnt)
	{
		this->ResizeParamCount(pNo+1);
	}
	if( pTV == 0 )
	{
		gettimeofday(&this->mPthPrm[pNo]->m_begin_time , 0 ) ;
		return true;
	}
	else
	{
		this->mPthPrm[pNo]->m_begin_time.tv_sec = pTV->tv_sec;
		this->mPthPrm[pNo]->m_begin_time.tv_usec = pTV->tv_usec ;
		return true ;
	}
	
	return false ;
}
bool ThreadCacl::SetLastTime(int pNo  , struct timeval * pTV  )
{
	if( pNo+1 > this->mMaxPrmCnt)
	{
		this->ResizeParamCount(pNo+1);
	}
	if( pTV == 0 )
	{
		gettimeofday(&this->mPthPrm[pNo]->m_last_time , 0 ) ;
		return true;
	}
	else
	{
		this->mPthPrm[pNo]->m_last_time.tv_sec = pTV->tv_sec;
		this->mPthPrm[pNo]->m_last_time.tv_usec = pTV->tv_usec ;
		return true ;
	}
	return false ;
}

void ThreadCacl::DumpTask(int pNo )
{
	thread_params * tp ;
	fprintf(stderr , "Dumping task begin ...\n");
	if( pNo >= 0 )
	{
		tp = this->mPthPrm[pNo];
		fprintf(stderr , "\tT:%d\tState: %d\tBegin: %ld \tTotal: %ld\tGot: %ld \turl: %s \n",
				tp->m_thread_no ,
				tp->m_state,
				tp->m_begin_position,
				tp->m_total_size,tp->m_got_size,
				tp->m_file_url );	
	}
	else
	{
		for( int i = 0 ; i < this->mActPrmCnt ; i ++ )
		{
			tp = this->mPthPrm[i];
			fprintf(stderr , "\tT:%d\tState: %d\tBegin: %ld \tTotal: %ld\tGot: %ld \turl: %s \n",
                    tp->m_thread_no ,
                    tp->m_state,
                    tp->m_begin_position,
                    tp->m_total_size,tp->m_got_size,
                    tp->m_file_url );	
		}		
	}
	fprintf(stderr , "Dumping task end ...\n");
}

thread_params * ThreadCacl::GetThreadParam(int pNo)
{
	if( pNo < this->mActPrmCnt && pNo >=0 )
	{
		return this->mPthPrm[pNo];
	}	
	return 0;
}

bool ThreadCacl::SetTaskSize(long pSize)
{
	this->mTaskSize = pSize ;
	return true;
}
bool ThreadCacl::SetTaskUrl(const char * pUrl)
{
	strcpy(this->mTaskUrl,pUrl);
	return true;
}

int ThreadCacl::GetTaskCount()
{
	return this->mActPrmCnt ;
}

bool ThreadCacl::IncreateGotSize( int pNo  , long pSize  ) 
{
	if( pNo < this->mActPrmCnt && pNo >=0 )
	{
		/*
          if( this->mPthPrm[pNo]->m_begin_time.tv_sec == 0 
          && this->mPthPrm[pNo]->m_begin_time.tv_usec == 0 )
          {
          gettimeofday( & this->mPthPrm[pNo]->m_begin_time , NULL ) ;
          }
		*/
		this->mPthPrm[pNo]->m_got_size += pSize ;
		gettimeofday(&this->mPthPrm[pNo]->m_last_time, NULL ) ;
		this->mTaskGotSize += pSize ;
		return true ;
	}
	
	return false ;
}

double ThreadCacl::GetTaskRate(int pNo , char * buff ) 
{
	double dtime  = 0.0 ;
	long    dlength = 0 ;
	double drate = 0.0 ;
	char    dbuff[32] = {0} ;
	const char *rate_names[] = {"B/s", "KB/s", "MB/s", "GB/s" };
	int units = 0 ;
	int pad = 1 ;
	
	if( pNo < this->mActPrmCnt && pNo >=0 )
	{
		dtime =  this->TimeDiff( &this->mPthPrm[pNo]->m_begin_time , &this->mPthPrm[pNo]->m_last_time ) ;
		dlength = this->mPthPrm[pNo]->m_got_size   ;
	}
	
	dtime == 0.0 ? 1 : dtime ;
	drate = 1000.0 * dlength / dtime ;
	if (drate < 1024.0)
		units = 0;
	else if (drate < 1024.0 * 1024.0)
		units = 1, drate /= 1024.0;
	else if (drate < 1024.0 * 1024.0 * 1024.0)
		units = 2, drate /= (1024.0 * 1024.0);
	else
		/* Maybe someone will need this, one day. */
		units = 3, drate /= (1024.0 * 1024.0 * 1024.0);
	sprintf (dbuff, pad ? "%7.2f %s" : "%.2f %s", drate, rate_names[units] );
	if( buff != 0 )
	{
		strcpy(buff, dbuff) ;
	}
	return drate ;
	return 0.0 ;
}
double ThreadCacl::GetGlobalRate( char * buff , long *totallen , long *gotlen ) 
{
	int i  = 0 ;
	double dtime = 0.0 ;
	long dlength = 0 ;
	long tlength = 0 ;
	double drate = 0.0 ;
	int units = 0 ;
	int pad = 1 ;
	const char *rate_names[] = {"B/s", "KB/s", "MB/s", "GB/s" };
	char    dbuff[32] = {0} ;
	
	//pthread_mutex_lock(&mGcMutex);
	//for ( i = this->mActPrmCnt-1 ; i>=0 ; i -- )
	{		
		//dtime += this->TimeDiff( &this->mPthPrm[i]->m_begin_time , &this->mPthPrm[i]->m_last_time ) ;
		//dtime <0.0 ? dtime = 0 : 1 ;
		//dlength += this->mPthPrm[i]->m_got_size ;
		//tlength += this->mPthPrm[i]->m_total_size ;
	}
	gettimeofday(&mTaskEndTime , 0 );
	dtime = this->TimeDiff( & mTaskBeginTime , & mTaskEndTime ) ;
	
	*gotlen = dlength = this->mTaskGotSize ;
	*totallen = tlength = this->mTaskSize  ;
	
	dtime == 0.0 ? 1 : dtime ;	
	drate = 1000.0 * dlength / dtime ;
	
	if (drate < 1024.0)
		units = 0;
	else if (drate < 1024.0 * 1024.0)
		units = 1, drate /= 1024.0;
	else if (drate < 1024.0 * 1024.0 * 1024.0)
		units = 2, drate /= (1024.0 * 1024.0);
	else
		/* Maybe someone will need this, one day. */
		units = 3, drate /= (1024.0 * 1024.0 * 1024.0);
	
	memset(dbuff,0,sizeof(dbuff));
	sprintf (dbuff, pad ? "%7.2f %s" : "%.2f %s ", drate, rate_names[units]  );
	//sprintf(dbuff , "aaa3 %d" , units );
	//pthread_mutex_unlock(&mGcMutex);
	
	if( buff != 0 )
	{
		strcpy(buff, dbuff) ;
	}
	
	return drate ;
	
	return 0.0 ;
}

double ThreadCacl::TimeDiff( struct timeval * btime , struct timeval * etime ) 
{
	if(  etime == 0 )	return 0.0 ;

	//fprintf(stderr , "%ld %ld :: %ld %ld \n" , btime->tv_sec , btime->tv_usec  , 
	//			etime->tv_sec , etime->tv_usec );
	
	return ((etime->tv_sec - btime->tv_sec) * 1000.0
            + (etime->tv_usec - btime->tv_usec) / 1000.0);	

}
//初始化测量速度的时间,及限速度用的时间
void    ThreadCacl::BeginTask( int pNo ) 
{
	if( pNo < this->mActPrmCnt && pNo >=0 )
	{
		gettimeofday(& this->mPthPrm[pNo]->m_begin_time , NULL ) ;
		this->mPthPrm[pNo]->chunk_bytes = 0 ;
		gettimeofday(& this->mPthPrm[pNo]->chunk_start  , NULL )  ;
		this->mPthPrm[pNo]->sleep_adjust = 0 ;

	}
}


void   ThreadCacl::LimitBandWidth ( int pNo , int bytes ) 
{
	if( pNo < this->mActPrmCnt && pNo >=0 )
	{
		struct timeval temptime  ;
		struct timeval temptime2  ;
		struct timeval now;
		gettimeofday(&now,NULL) ;
		double delta_t = this->TimeDiff( & this->mPthPrm[pNo]->chunk_start, & now ) ;
		double expected ; 
		double seconds ;
		this->mPthPrm[pNo]->chunk_bytes += bytes ;

		expected = 1000.0 * this->mPthPrm[pNo] ->chunk_bytes / 250000  ; //	20000= limit rate
		
		if( expected > delta_t )
		{
			double slp = expected - delta_t + this->mPthPrm[pNo]->sleep_adjust ;
			double t0 , t1 ;

			//差别不够大，不做处理
			if( slp < 200 )
			{
				return ;
			}
			seconds = slp / 1000  ;
			gettimeofday(&temptime,NULL);
			//DEBUGP("limit rate sleep : %f \n " , seconds ) ;
			if (seconds >= 1)
			{
                /* On some systems, usleep cannot handle values larger than
                   1,000,000.  If the period is larger than that, use sleep
                   first, then add usleep for subsecond accuracy.  */
                sleep ( (unsigned int)seconds );
                seconds -= (long) seconds;
			}
			usleep ((unsigned int)(seconds * 1000000));
			gettimeofday(&temptime2,NULL);

			this->mPthPrm[pNo] ->sleep_adjust = slp - this->TimeDiff(&temptime, &temptime2 ) ;
			if( this->mPthPrm[pNo] ->sleep_adjust > 500 )
				this->mPthPrm[pNo] ->sleep_adjust = 500 ;
			if ( this->mPthPrm[pNo] ->sleep_adjust < -500 )
				this->mPthPrm[pNo] ->sleep_adjust = -500;
			  
		}
		else
		{
			
		}
		this->mPthPrm[pNo] ->chunk_bytes = 0;
		gettimeofday(&this->mPthPrm[pNo] ->chunk_start,NULL );
		
	}
	
}

void ThreadCacl::CreateBarImage(char *barstr , long total , long got , char * ratestr )
{
	char * p = barstr ;
	int  barsize = 50 ;
	int i = 0 ;
	int percentage = (int)(100.0 * got / total );
	percentage = percentage>100?100:percentage ;
	int eqlen = (int)(percentage * barsize/100) ;
	char tmp[30] = {0};
	
	*p = '[';p++;

	
	memset(p,'=',eqlen );
	p = p +  eqlen ;

	*p = '>';p++;
	memset(p,' ',barsize-eqlen  );
	p = p + barsize-eqlen  ;
	*p = ']';p++;

	for(i=0;i<3;i++)
	{
		*p = ' ';p++;
	}

	memset(tmp,0,sizeof(tmp));
	sprintf(tmp ,"%d/%d", got , total );

	for(i=0;i<strlen(tmp);i++,p++)
		*p = tmp[i] ;
		
	for(i=0;i<3;i++,p++)
		*p = ' ';
	for(i=0;i<strlen(ratestr);i++,p++)
		*p = ratestr[i] ;
		
	*p = '\0';
	
}


