
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <cerrno>
#include <getopt.h>

#include "ksocket.h"

#include "mwget.h"
#include "headers.h"
#include "url.h"
#include "threadcacl.h"
#include "progress.h"
#include "http.h"
#include "ftp.h"

#include "mmscore.h"
#include "rtspcore.h"


#include "mwui.h"
#include "test.h"
#include "options.h"
#include "utils.h"

#include "msgbus.h"


#define SIGFCFD	SIGUSR1		//signal for first connect faild
#define SIGFCSC	SIGUSR2		//signal for first connect success

sem_t g_sem_first_connect_finished ;	//
int	g_first_connect_result = 1 ;	//第一次连接服务器处理结果
							//first connect handle result
							// 0 success , -1 faild
pthread_cond_t g_cond_first_connect_finished = PTHREAD_COND_INITIALIZER ;
pthread_mutex_t g_mutex_first_connect_finished = PTHREAD_MUTEX_INITIALIZER ;

pthread_cond_t g_cond_all_finished = PTHREAD_COND_INITIALIZER ;
pthread_mutex_t g_mutex_all_finished = PTHREAD_MUTEX_INITIALIZER ;
int  g_completed_thread_count = 0 ;

int g_thread_count;
pthread_t g_thread_ctrl;
pthread_t g_thread_ctrl_all[30];

//for msg not maze
pthread_mutex_t g_msg_out_mutex = PTHREAD_MUTEX_INITIALIZER ;
//
thread_params ** g_thread_param_group;	//所有线程的参数

//exe info
const char *exec_name;
char *version_string = "0.01 alpha";

//time Measure
time_t g_task_begin_time;
time_t g_task_end_time;

PipeBus * g_msg_bus = 0 ;
SimpleMWindow * g_simple_wnd = 0 ;

//thread func
void * retr_thread_func(void * param);
void  signal_handler(int signo);
void  maskncspecialsignals();

int main(int argc, char**argv)
{
	exec_name = strrchr (argv[0], '/' ) +1 ;
	//test_function();	
	//return 0;
	//maskncspecialsignals();
	//test_ncwindow();
	//return 0 ;
	//test_options( argc,  argv ) ;
	//return  0 ;
	//test_pipe_bus() ;
	//return 0 ;

	//test_mms_protocol() ;

	//test_rtsp_protocol() ;

	//return 0 ;

	int ret , i  ;
	char turl[255] = {0 }  ;	//= "http://localhost/mtv.wmv";
	char tempstr[255] = {0} ;
	char msgstr[100] = {0}; 
	
	//DEBUGP ( " %d %d %s %s \n" ,argc , optind , argv[ optind] , opt.input_filename ) ;
//	DEBUGP(x...)
	if( !(argc-optind)  )
	{
		fprintf (stderr , "%s: missing URL\n", exec_name);
		return 0;
	}
	else
	{
		strcpy(turl , argv[optind]);
	}
	//return 0;
	ThreadCacl *tc = ThreadCacl::Instance();
	g_msg_bus = new PipeBus( ) ;
	//g_simple_wnd = SimpleMWindow::Instance ();
	//g_simple_wnd->Init();
	
#ifdef USE_NCURSOR
	ProgressBar * bar = ProgressBar::Instance();
#else
	
#endif	
	
	
	g_thread_count = 3 ;
	tc->SetTaskUrl(turl);
	tc->ResizeParamCount(g_thread_count);
	//tc->DumpTask();
	
	signal(SIGFCFD,signal_handler);
	signal(SIGFCSC,signal_handler);
	signal(SIGINT,signal_handler);
	signal(SIGSEGV,signal_handler);
	
	sem_init(&g_sem_first_connect_finished, 0, 0);

	//sigset_t ss , oss;
	//sigemptyset(&ss);
	//sigemptyset(&oss);	
	//sigaddset(&ss, SIGFCFD);
	//sigaddset(&ss, SIGFCSC);
	//sigprocmask(SIG_BLOCK,&ss, &oss);

	//time begin cacl
	g_task_begin_time = time(NULL);
	
	pthread_create(&g_thread_ctrl, 0, retr_thread_func, 0);

	//ret = sem_wait(&g_sem_first_connect_finished);	
	//printf("sem_wait: %d  cr: %d  EN: %d FOR ( %s )\n",ret , g_first_connect_result , errno , strerror(errno));
	//sem_destroy(&g_sem_first_connect_finished);
	pthread_mutex_lock(&g_mutex_first_connect_finished);
	ret = pthread_cond_wait( &g_cond_first_connect_finished, &g_mutex_first_connect_finished );
	pthread_mutex_unlock(&g_mutex_first_connect_finished);

	sprintf(msgstr,"cond_wait: %d  cr: %d  EN: %d FOR ( %s )\n",ret , g_first_connect_result , errno , strerror(errno));
#ifdef USE_NCURSOR
	bar->Message(msgstr);
#else
	DEBUGP(msgstr);
#endif

	//sprintf(tempstr , "cond_wait: %d  cr: %d  EN: %d FOR ( %s )",ret , g_first_connect_result , errno , strerror(errno));
	//g_simple_wnd->AddMessage( tempstr ) ;
	
	if( g_first_connect_result == 0 )
	{
		//printf("create others thread now...\n");
		for( i = 1 ; i < g_thread_count ; i ++ )
		{
			pthread_t tt;
			pthread_create(&g_thread_ctrl, 0, retr_thread_func, (void*)i);
			//printf("create  thread %d now...\n" , i );
		}
	}
	else
	{
		
		DEBUGP("g_first_connect_result = %d exit \n",g_first_connect_result);
		//sprintf(tempstr , "g_first_connect_result = %d exit ",g_first_connect_result);
		//g_simple_wnd->AddMessage( tempstr ) ;
		//raise(SIGINT);
		goto endroutine;
	}

	///////getch inside ui
	/*
	for( ; ; )
	{
		int sret ;
		int maxfd ;
		fd_set fdi;
		int ch ;

		FD_ZERO(&fdi) ;
		FD_SET(STDIN_FILENO,&fdi);
		maxfd = STDIN_FILENO + 1 ;
		sret = select(maxfd , &fdi , 0,0,0);

		if( sret < 0 )
		{
			sprintf(tempstr, "select : %d %s \n",sret , strerror( errno) );
			g_simple_wnd->AddMessage( tempstr  ) ;
		}
		else
		{
			ch = getch();
			switch(ch)
			{
				case 'q':
					goto endkeyloop;
					break;
				default:
					break;
			}
		}
	}
	*/

	pthread_mutex_lock(&g_mutex_all_finished);
	while(g_completed_thread_count<g_thread_count)
	{
		
		ret = pthread_cond_wait(&g_cond_all_finished, &g_mutex_all_finished);			
		++g_completed_thread_count ;
	}
		pthread_mutex_lock(&g_msg_out_mutex);
		sprintf(msgstr," task finished completed %d %d %s" , g_completed_thread_count , ret , strerror(errno));
		
#ifdef USE_NCURSOR
		bar->Message(msgstr);
#else
		DEBUGP(msgstr);
#endif	
		pthread_mutex_unlock(&g_msg_out_mutex);
		

	pthread_mutex_unlock(&g_mutex_all_finished);
			

	sprintf(msgstr," task finished completed ,exit ... \n");
#ifdef USE_NCURSOR
		bar->Message(msgstr);
#else
		DEBUGP(msgstr);
#endif	

	//for(;;)
	//	pause();

	endkeyloop:


	endroutine:	
	

	//time begin cacl
	g_task_end_time = time(NULL);

	char time_begin_str[50] = {0};
	char time_end_str[50] = {0};
	strcpy(time_begin_str,datetime_str(&g_task_begin_time));
	strcpy(time_end_str,datetime_str(&g_task_end_time));
	//show time used
	sprintf(msgstr,"Begin: %s\tEnd: %s",time_begin_str, time_end_str );
#ifdef USE_NCURSOR
		bar->Message(msgstr);
#else
		DEBUGP(msgstr);
#endif	
	
	//delete g_simple_wnd ;
	//DEBUGP( "routine exit...\n" );
	sprintf(msgstr,"routine exit..." );
#ifdef USE_NCURSOR
		bar->Message(msgstr);
#else
		DEBUGP(msgstr);
#endif	
	
	//pause();
	
#ifdef USE_NCURSOR
	delete bar;
#else
	
#endif	

	//tc->DumpTask(-1);	
	//fflush(stderr);
	
	return 0;
}

void *retr_thread_func(void * param)
{
	int threadno = (int)param;
	int cycle_count = 0 , retry_count = 0 , error_count = 0 , cycle_no = 0 ;
	bool ret ;
	char rbuff[1024] = {0};
	char dbuff[8096] = {0};
	//char bar[75] = {0};
	char barstr[100]={0};
	long fsize = 0 ;
	char cmd[32] = {0};
	double irate = 0.0 ;
	double trate = 0.0 ;
	char ratestr[32] = {0} ;
	char msgstr[100] = {0} ;
	int	connretry = 5 ;
	double waitretry = 5 ;		/* The wait period between retries. - HEH */
	char tmp[100] = {0};
	long totallen,gotlen;

#ifdef USE_NCURSOR
	ProgressBar * bar = ProgressBar::Instance();
#endif

	time_t tb,te ;
	
	ThreadCacl *tc = ThreadCacl::Instance();
	thread_params *tp = tc->GetThreadParam(threadno);
	URL *turl = 0 ;


	redirect:
		
	turl = new  URL(tp->m_file_url);
	
	turl->Parse();
	//turl->Dump();
	
	//fprintf(stderr ,"T: %d -- basename : %s \n",tp->m_thread_no , turl->GetBaseFileName(tmp) );
	//fprintf(stderr , "base name------%s\n" , "tname");
	//ProgressBar * pbar = ProgressBar::Instance();

	Retriever * hr = 0 ;
	if( strcasecmp(turl->GetScheme(), "HTTP") == 0)
	{
		hr = new HttpRetriever(threadno , tp->m_file_url );
	}
	else if( strcasecmp(turl->GetScheme(), "FTP") == 0)
	{
		hr = new FtpRetreiver(threadno , tp->m_file_url ) ;
	}
	else
	{
		fprintf(stderr,"Not supported scheme\n");
		//sprintf(msgstr, "Not supported scheme\n");
		//g_simple_wnd->AddMessage( msgstr ) ;
		
		return 0 ;
	}
	tc->BeginTask( threadno ) ;
	while( ++cycle_count  )
	{

		
		// try connect server
		//KSocket *sock = new KSocket();
		//KSocket *tsock = new KSocket() ;
		//KSocket *dsock = 0 ;
		
		//sock->setTimeout(100, 1000);
		//ret = sock->Connect(turl->GetHost(), turl->GetPort());
		ret = hr->TaskConnect() ;
	
		sprintf(msgstr ,"T (%d) connect : %s CY(%d)  TRY(%d) ",tp->m_thread_no, 
				ret ? "OK" : "FAILD" , cycle_no , retry_count );
		//g_simple_wnd->AddMessage( msgstr ) ;
		

		if( ! ret )	
		{
			retry_count ++;
#ifdef USE_NCURSOR
			bar->Message(msgstr);
#else
			DEBUGP(msgstr);
#endif

			if( tp->m_thread_no == 0 && cycle_no == 0 && retry_count >= connretry )	//exec first connect result judge
			{
				//g_first_connect_result = -1 ;
				//sem_post( &g_sem_first_connect_finished) ;//raise(SIGFCFD);
				//fprintf(stderr,"!ret\n");
				DEBUGP("conn retry time overflow signal\n");
				tc->SetSubTaskState( threadno , T_STOP ) ;
				pthread_mutex_lock(&g_mutex_first_connect_finished);
				g_first_connect_result = -1 ;
				pthread_cond_signal(&g_cond_first_connect_finished);
				pthread_mutex_unlock(&g_mutex_first_connect_finished);

				return 0;
			}
			tc->SetSubTaskState( threadno , T_ERROR ) ;
			sprintf(msgstr,"connretry wait (%d)  execute now..." , (int)waitretry ) ;
			//DEBUGP("connretry wait (%d)  execute now...\n" , (int)waitretry ) ;
#ifdef USE_NCURSOR
			bar->Message(msgstr);
#else
			DEBUGP(msgstr);
#endif			
			sleep((int)waitretry);
			sprintf(msgstr,"connretry wait (%d)  execute now..." , (int)waitretry ) ;
			//DEBUGP("connretry wait (%d)  execute end...\n" , (int)waitretry ) ;
#ifdef USE_NCURSOR
			bar->Message(msgstr);
#else
			DEBUGP(msgstr);
#endif		
			continue ;
		}
		else
		{
			retry_count = 0 ;
#ifdef USE_NCURSOR
			bar->Message(msgstr);
#else
			pthread_mutex_lock(&g_msg_out_mutex);
			DEBUGP(msgstr);
			pthread_mutex_unlock(&g_msg_out_mutex);			
#endif
			//g_first_connect_result = 0 ;
			//cycle_no = 1 ;
			//pthread_cond_signal(&g_cond_first_connect_finished);
			//sem_post( &g_sem_first_connect_finished) ;	//raise(SIGFCSC);
			//fprintf(stderr,"!ret\n");
		}			
		//pthread_cond_broadcast(&g_cond_first_connect_finished);
			
		
		(tp->m_thread_no == 0 && cycle_no == 0) ? 
				fsize = 0 : fsize = tp->m_begin_position + tp->m_got_size ;	

		//tb = time(NULL);
		if ( ! hr->TaskValid( fsize ) )
		{			
			sprintf(msgstr,"http file not valid");
#ifdef USE_NCURSOR
			bar->Message(msgstr);
#else
			DEBUGP(msgstr);
#endif	
			error_count ++ ;
			if( error_count >= connretry )
			{
				//g_first_connect_result = -1 ;
				tc->SetSubTaskState( threadno , T_STOP ) ;
				pthread_mutex_lock(&g_mutex_first_connect_finished);
				g_first_connect_result = -1 ;
				pthread_cond_signal(&g_cond_first_connect_finished);
				pthread_mutex_unlock(&g_mutex_first_connect_finished);
				break;
			}
			tc->SetSubTaskState( threadno , T_ERROR ) ;			
			sprintf(msgstr,"(%d) sleep %d seconds between retry (%d / %d ) " ,
							threadno ,(int)waitretry, error_count , connretry );
#ifdef USE_NCURSOR
			bar->Message(msgstr);
#else
			DEBUGP(msgstr);
#endif					
			sleep((int)waitretry);
			continue ;
		}
		else
		{
			error_count = 0 ;
		}
		//fprintf(stderr, "%i %s %ld-%ld \n", threadno , "退出总read cycle " ,tb , time(NULL));
		//fflush(stderr);		
		
		fsize = hr->GetContentLength();
		//fprintf(stderr," fsize = %ld \n" , fsize );
		
		if( tp->m_thread_no == 0 && cycle_no == 0 )	//exec first connect result judge
		{			
			tc->CaclTask( fsize );			
			//tc->DumpTask();	
			
			cycle_no = 1 ;			
			pthread_mutex_lock(&g_mutex_first_connect_finished);
			g_first_connect_result = 0 ;
			pthread_cond_signal(&g_cond_first_connect_finished);
			pthread_mutex_unlock(&g_mutex_first_connect_finished);
			//sem_post( &g_sem_first_connect_finished) ;	//raise(SIGFCSC);
			//fprintf(stderr,"!ret\n");
	
		}
		
		//handle read from header data
		//int hd_len = header.GetHeaderLength();
		//memcpy(dbuff , rbuff + hd_len , hd_rlen - hd_len);
		
		//fprintf(stderr , "Left Data Len : %d \n",hd_rlen - hd_len);
		int wfp = open(turl->GetBaseFileName(tmp),O_WRONLY|O_CREAT ,0644);
		//DEBUGP("open file (%d) : %s %s \n " , wfp , turl->GetBaseFileName(tmp) , strerror(errno)) ;
		lseek(wfp,tp->m_begin_position + tp->m_got_size , SEEK_SET);
		//write(wfp,dbuff,hd_rlen - hd_len);
		//write(wfp,rbuff,hd_rlen );
		//tp->m_got_size += hd_rlen - hd_len ;		
		//break;
		//handle other data
		bool readFinished = false  ;
		int rlen ;

		//if( strcasecmp(turl->GetScheme(), "FTP") == 0)
		{
			//ftp_pasv_send(sock, dsock) ;
			//sprintf(cmd,"REST %ld\r\n" , tp->m_begin_position + tp->m_got_size );
			//ftp_interactive_once(sock, cmd ); 
			//sprintf(cmd,"RETR %s\r\n" , turl->GetPath());
			//ftp_interactive_once(sock, cmd) ;
		}

		hr->PrepairData() ;
		KSocket * dsock = hr->GetDataSocket();
		int sfd = dsock->GetSocket();

		tc->SetSubTaskState( threadno , T_RETRIVE ) ;

		while( ( rlen = dsock->Read(dbuff, sizeof(dbuff))) > 0 )
		//while( ( rlen = read(sfd , dbuff, sizeof(dbuff))) > 0 )
		{
			
			//if( threadno == 2 )
			//	fprintf(stderr,"while ..read   %d  %d %ld %ld\n" , threadno , rlen ,tp->m_got_size,tp->m_total_size);
			//sprintf(barstr,"while ..read   %d  %d %ld %ld\n" , threadno , rlen ,tp->m_got_size,tp->m_total_size);
			//pbar->Message(barstr);
			//continue;
			write(wfp,dbuff,rlen);
			//tp->m_got_size += rlen ;
			tc->IncreateGotSize( threadno , rlen ) ;
			
			irate = tc->GetTaskRate( threadno , ratestr ) ;			
			
			memset(barstr, ' ' ,sizeof(barstr)-1);			
			sprintf(barstr+2," %ld ",tp->m_got_size);
			barstr[strlen(barstr)]='Y';
			sprintf(barstr+10," %ld ",rlen);
			barstr[strlen(barstr)]='Y';
			sprintf(barstr+20," %ld ",threadno);
			barstr[strlen(barstr)]='Y';
			sprintf(barstr+30," %ld ",tp->m_total_size);
			barstr[strlen(barstr)]='Y';
			sprintf( barstr+42," R %s ",ratestr );
			barstr[strlen(barstr)]='Y';			
			//barstr[sizeof(barstr) -1 ] = '\0' ; 
		  	//fprintf(stderr, "\r%s", bar );
		  	//if( threadno == 1 ) fprintf(stderr, "\r%s", bar );
		  	//fprintf(stdout, "\r");
		  	//fprintf(stdout, "\n");
		  	//fprintf(stdout, "%s" , bar );
			//pbar->Progress(threadno+1,bar);
			//fprintf(stderr,"=");
			//g_simple_wnd->UpdateProgress( bar , threadno ) ;
			//g_simple_wnd->AddMessage( bar  ) ;
			
			
#ifdef USE_NCURSOR

			bar->CreateBarImage( barstr,  tp->m_total_size , tp->m_got_size, ratestr  ) ;
			bar->Progress( threadno , barstr ) ;
#else
			memset(barstr, 0, sizeof(barstr)) ;
			tc->CreateBarImage( barstr,  tp->m_total_size , tp->m_got_size, ratestr ) ;
			pthread_mutex_lock(&g_msg_out_mutex);
			fprintf(stderr, "\rT-%d: %s",threadno , barstr );
			fflush(stderr);
			pthread_mutex_unlock(&g_msg_out_mutex);

#endif
/*
			memset(ratestr,0,sizeof(ratestr));
			irate = tc->GetGlobalRate(  ratestr , &totallen , &gotlen ) ;		
			
#ifdef USE_NCURSOR
			bar->CreateBarImage( barstr,  totallen , gotlen , ratestr  ) ;
			bar->Progress( g_thread_count , barstr ) ;
#else
			pthread_mutex_lock(&g_msg_out_mutex);
			fprintf(stderr, "%d %s\n", strlen(barstr), barstr );
			fflush(stderr);
			pthread_mutex_unlock(&g_msg_out_mutex);

#endif			
*/			
			if( tp->m_got_size >= tp->m_total_size )
			{
				readFinished = true ;
				close(wfp);
				tc->SetSubTaskState( threadno , T_CLOSE ) ;
				//fprintf(stderr,"my this cycle task OK  %d  TL:%d GT:%d BP:%ld \n",
				//	threadno , tp->m_total_size ,tp->m_got_size , tp->m_begin_position);
				break;
			}
			//tc->LimitBandWidth( threadno ,  rlen ) ;
			//usleep(200000);
			//sleep(1);
			//nanosleep();
		}

		hr->TaskFinish();

		close(wfp);
		
		
		//fprintf(stderr,"CLOSE :%s %d (%s)  EC:%d \n",(sock->Close() ) ? "OK":"NO" , errno,strerror(errno) ,error_count );

		//delete sock ; sock = 0 ;
		
		//read completed , recacl & (continue or break)
		if( readFinished )
		{	
			//fprintf(stderr,"readFinished  %d  TL:%d GT:%d BP:%ld \n",
			//	threadno , tp->m_total_size ,tp->m_got_size , tp->m_begin_position);
			tc->ReCaclTask(threadno);
			//fprintf(stderr,"readFinishedd %d  TL:%d GT:%d BP:%ld \n",
			//	threadno , tp->m_total_size ,tp->m_got_size , tp->m_begin_position);
			if(tp->m_total_size == 0 && tp->m_begin_position == tc->GetTaskSize()) 
			{
				//pthread_cond_signal(&g_cond_all_finished);
				break;
			}	
			
		}
		if( tp->m_begin_position >= tc->GetTaskSize())
		{
			///pthread_cond_signal(&g_cond_all_finished);
			//sprintf(msgstr , "i (%d) finished  ..." ,threadno);
			//g_simple_wnd->AddMessage( msgstr ) ;
			tc->SetSubTaskState( threadno , T_CLOSE ) ;
			break;		
		}
		//break;
	}
	
	delete turl ;	turl = 0 ;
	delete hr ; hr = 0 ;
	
	//sprintf(msgstr , "thread %d return ...",threadno);
	//g_simple_wnd->AddMessage( msgstr ) ;
	//fprintf(stdout,"thread %d return Before cond signal ...\n",threadno);

	pthread_mutex_lock(&g_mutex_all_finished);	
	pthread_cond_signal(&g_cond_all_finished);
	pthread_mutex_unlock(&g_mutex_all_finished);
	
	sprintf(msgstr,"thread %d return after cond signal ...\n",threadno);
#ifdef USE_NCURSOR
	bar->Message(msgstr);
#else
	DEBUGP(msgstr);
#endif	

	return 0;
}

void  signal_handler(int signo)
{
	switch(signo)
	{
		case SIGFCFD:
				//printf("SIGFCFD recieved\n");
				g_first_connect_result = -1 ;
				sem_post(&g_sem_first_connect_finished);
			break;
		case SIGFCSC:
				//printf("SIGFCSC recieved\n");
				g_first_connect_result = 0 ;
				sem_post(&g_sem_first_connect_finished);
			break;
		case SIGSEGV:
				//fprintf(stderr,"SIGSEGV recieved FOR %d (%s ) \n" , errno,strerror(errno));
			break;
		case SIGINT:
				printf("SIGINT recieved\n");
#ifdef USE_NCURSOR
				ProgressBar * bar = ProgressBar::Instance();
				delete bar; bar = 0 ;
#else

#endif	

				exit(SIGINT);
			break;
		default:
				//printf("SIG? recieved\n");			
			break;
	};

	return ;
}

void  maskncspecialsignals()
{
	int nret ;
	sigset_t masksigset;
	sigset_t oldsigset;

	sigemptyset( &oldsigset ) ;
	sigemptyset( &masksigset ) ;
	sigaddset(&masksigset, SIGINT ) ;
	nret = sigprocmask(SIG_BLOCK,&masksigset,&oldsigset ) ;

	//DEBUGP("proc sig %d \n" , nret ) ;
	//sleep(1);
}


#define N_(x)  (x )
#define _(x)   x 
void print_help( void ) 
{	
  /* We split the help text this way to ease translation of individual
     entries.  */
  static const char *help[] = {
    "\n",
    N_("\
Mandatory arguments to long options are mandatory for short options too.\n\n"),
    N_("\
Startup:\n"),
    N_("\
  -V,  --version           display the version of Wget and exit.\n"),
    N_("\
  -h,  --help              print this help.\n"),
    N_("\
  -b,  --background        go to background after startup.\n"),
    N_("\
  -e,  --execute=COMMAND   execute a `.wgetrc'-style command.\n"),
    "\n",

	N_("\
MultiThread support: \n"),
	N_("\
  -mt, --multi-thread        turn on multi-thread support.\n"),
  	N_("\
  -T, --thread-count=NUMBER  set thread count .\n"),
	 "\n",
	 
    N_("\
Logging and input file:\n"),
    N_("\
  -o,  --output-file=FILE    log messages to FILE.\n"),
    N_("\
  -a,  --append-output=FILE  append messages to FILE.\n"),
#ifdef ENABLE_DEBUG
    N_("\
  -d,  --debug               print lots of debugging information.\n"),
#endif
    N_("\
  -q,  --quiet               quiet (no output).\n"),
    N_("\
  -v,  --verbose             be verbose (this is the default).\n"),
    N_("\
  -nv, --no-verbose          turn off verboseness, without being quiet.\n"),
    N_("\
  -i,  --input-file=FILE     download URLs found in FILE.\n"),
    N_("\
  -F,  --force-html          treat input file as HTML.\n"),
    N_("\
  -B,  --base=URL            prepends URL to relative links in -F -i file.\n"),
    "\n",

    N_("\
Download:\n"),
    N_("\
  -t,  --tries=NUMBER            set number of retries to NUMBER (0 unlimits).\n"),
    N_("\
       --retry-connrefused       retry even if connection is refused.\n"),
    N_("\
  -O,  --output-document=FILE    write documents to FILE.\n"),
    N_("\
  -nc, --no-clobber              skip downloads that would download to\n\
                                 existing files.\n"),
    N_("\
  -c,  --continue                resume getting a partially-downloaded file.\n"),
    N_("\
       --progress=TYPE           select progress gauge type.\n"),
    N_("\
  -N,  --timestamping            don't re-retrieve files unless newer than\n\
                                 local.\n"),
    N_("\
  -S,  --server-response         print server response.\n"),
    N_("\
       --spider                  don't download anything.\n"),
    N_("\
  -T,  --timeout=SECONDS         set all timeout values to SECONDS.\n"),
    N_("\
       --dns-timeout=SECS        set the DNS lookup timeout to SECS.\n"),
    N_("\
       --connect-timeout=SECS    set the connect timeout to SECS.\n"),
    N_("\
       --read-timeout=SECS       set the read timeout to SECS.\n"),
    N_("\
  -w,  --wait=SECONDS            wait SECONDS between retrievals.\n"),
    N_("\
       --waitretry=SECONDS       wait 1..SECONDS between retries of a retrieval.\n"),
    N_("\
       --random-wait             wait from 0...2*WAIT secs between retrievals.\n"),
    N_("\
  -Y,  --proxy                   explicitly turn on proxy.\n"),
    N_("\
       --no-proxy                explicitly turn off proxy.\n"),
    N_("\
  -Q,  --quota=NUMBER            set retrieval quota to NUMBER.\n"),
    N_("\
       --bind-address=ADDRESS    bind to ADDRESS (hostname or IP) on local host.\n"),
    N_("\
       --limit-rate=RATE         limit download rate to RATE.\n"),
    N_("\
       --no-dns-cache            disable caching DNS lookups.\n"),
    N_("\
       --restrict-file-names=OS  restrict chars in file names to ones OS allows.\n"),
#ifdef ENABLE_IPV6
    N_("\
  -4,  --inet4-only              connect only to IPv4 addresses.\n"),
    N_("\
  -6,  --inet6-only              connect only to IPv6 addresses.\n"),
    N_("\
       --prefer-family=FAMILY    connect first to addresses of specified family,\n\
                                 one of IPv6, IPv4, or none.\n"),
#endif
    N_("\
       --user=USER               set both ftp and http user to USER.\n"),
    N_("\
       --password=PASS           set both ftp and http password to PASS.\n"),
    "\n",

    N_("\
Directories:\n"),
    N_("\
  -nd, --no-directories           don't create directories.\n"),
    N_("\
  -x,  --force-directories        force creation of directories.\n"),
    N_("\
  -nH, --no-host-directories      don't create host directories.\n"),
    N_("\
       --protocol-directories     use protocol name in directories.\n"),
    N_("\
  -P,  --directory-prefix=PREFIX  save files to PREFIX/...\n"),
    N_("\
       --cut-dirs=NUMBER          ignore NUMBER remote directory components.\n"),
    "\n",

    N_("\
HTTP options:\n"),
    N_("\
       --http-user=USER        set http user to USER.\n"),
    N_("\
       --http-password=PASS    set http password to PASS.\n"),
    N_("\
       --no-cache              disallow server-cached data.\n"),
    N_("\
  -E,  --html-extension        save HTML documents with `.html' extension.\n"),
    N_("\
       --ignore-length         ignore `Content-Length' header field.\n"),
    N_("\
       --header=STRING         insert STRING among the headers.\n"),
    N_("\
       --proxy-user=USER       set USER as proxy username.\n"),
    N_("\
       --proxy-password=PASS   set PASS as proxy password.\n"),
    N_("\
       --referer=URL           include `Referer: URL' header in HTTP request.\n"),
    N_("\
       --save-headers          save the HTTP headers to file.\n"),
    N_("\
  -U,  --user-agent=AGENT      identify as AGENT instead of Wget/VERSION.\n"),
    N_("\
       --no-http-keep-alive    disable HTTP keep-alive (persistent connections).\n"),
    N_("\
       --no-cookies            don't use cookies.\n"),
    N_("\
       --load-cookies=FILE     load cookies from FILE before session.\n"),
    N_("\
       --save-cookies=FILE     save cookies to FILE after session.\n"),
    N_("\
       --keep-session-cookies  load and save session (non-permanent) cookies.\n"),
    N_("\
       --post-data=STRING      use the POST method; send STRING as the data.\n"),
    N_("\
       --post-file=FILE        use the POST method; send contents of FILE.\n"),
    "\n",

#ifdef HAVE_SSL
    N_("\
HTTPS (SSL/TLS) options:\n"),
    N_("\
       --secure-protocol=PR     choose secure protocol, one of auto, SSLv2,\n\
                                SSLv3, and TLSv1.\n"),
    N_("\
       --no-check-certificate   don't validate the server's certificate.\n"),
    N_("\
       --certificate=FILE       client certificate file.\n"),
    N_("\
       --certificate-type=TYPE  client certificate type, PEM or DER.\n"),
    N_("\
       --private-key=FILE       private key file.\n"),
    N_("\
       --private-key-type=TYPE  private key type, PEM or DER.\n"),
    N_("\
       --ca-certificate=FILE    file with the bundle of CA's.\n"),
    N_("\
       --ca-directory=DIR       directory where hash list of CA's is stored.\n"),
    N_("\
       --random-file=FILE       file with random data for seeding the SSL PRNG.\n"),
    N_("\
       --egd-file=FILE          file naming the EGD socket with random data.\n"),
    "\n",
#endif /* HAVE_SSL */

    N_("\
FTP options:\n"),
    N_("\
       --ftp-user=USER         set ftp user to USER.\n"),
    N_("\
       --ftp-password=PASS     set ftp password to PASS.\n"),
    N_("\
       --no-remove-listing     don't remove `.listing' files.\n"),
    N_("\
       --no-glob               turn off FTP file name globbing.\n"),
    N_("\
       --no-passive-ftp        disable the \"passive\" transfer mode.\n"),
    N_("\
       --retr-symlinks         when recursing, get linked-to files (not dir).\n"),
    N_("\
       --preserve-permissions  preserve remote file permissions.\n"),
    "\n",

    N_("\
Recursive download:\n"),
    N_("\
  -r,  --recursive          specify recursive download.\n"),
    N_("\
  -l,  --level=NUMBER       maximum recursion depth (inf or 0 for infinite).\n"),
    N_("\
       --delete-after       delete files locally after downloading them.\n"),
    N_("\
  -k,  --convert-links      make links in downloaded HTML point to local files.\n"),
    N_("\
  -K,  --backup-converted   before converting file X, back up as X.orig.\n"),
    N_("\
  -m,  --mirror             shortcut option equivalent to -r -N -l inf -nr.\n"),
    N_("\
  -p,  --page-requisites    get all images, etc. needed to display HTML page.\n"),
    N_("\
       --strict-comments    turn on strict (SGML) handling of HTML comments.\n"),
    "\n",

    N_("\
Recursive accept/reject:\n"),
    N_("\
  -A,  --accept=LIST               comma-separated list of accepted extensions.\n"),
    N_("\
  -R,  --reject=LIST               comma-separated list of rejected extensions.\n"),
    N_("\
  -D,  --domains=LIST              comma-separated list of accepted domains.\n"),
    N_("\
       --exclude-domains=LIST      comma-separated list of rejected domains.\n"),
    N_("\
       --follow-ftp                follow FTP links from HTML documents.\n"),
    N_("\
       --follow-tags=LIST          comma-separated list of followed HTML tags.\n"),
    N_("\
       --ignore-tags=LIST          comma-separated list of ignored HTML tags.\n"),
    N_("\
  -H,  --span-hosts                go to foreign hosts when recursive.\n"),
    N_("\
  -L,  --relative                  follow relative links only.\n"),
    N_("\
  -I,  --include-directories=LIST  list of allowed directories.\n"),
    N_("\
  -X,  --exclude-directories=LIST  list of excluded directories.\n"),
    N_("\
  -np, --no-parent                 don't ascend to the parent directory.\n"),
    "\n",

    N_("Mail bug reports and suggestions to <bug-wget@gnu.org>.\n")
  };

  int i;

  printf (_("LC MWget %s, a non-interactive network retriever.\n"),
	  version_string);
  print_usage ();

  for (i = 0; i < countof (help); i++)
    fputs (_(help[i]), stdout);

  exit (0);
}

/* Print the usage message.  */
void print_usage (void)
{
//	printf ("Usage: %s [OPTION]... [URL]...\n", exec_name);
	printf ("Usage: %s [OPTION]... [URL]...\n", "mwget");
}

void print_version (void)
{
  printf ("GNU Wget %s\n\n", version_string);
  fputs (_("\
Copyright (C) 2005 Free Software Foundation, Inc.\n"), stdout);
  fputs (_("\
This program is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
GNU General Public License for more details.\n"), stdout);
  fputs (_("\nOriginally written by Hrvoje Niksic <hniksic@xemacs.org>.\n"),
	 stdout);
  exit (0);
}



