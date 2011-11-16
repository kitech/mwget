

#include "ksocket.h"
#include "progress.h"
#include "headers.h"

#include "test.h"


void sig_winch(int signo)
{
	fprintf(stderr, "SIGWINCH signal catched\n");
}

void pr_winsize(int fd)
{
	struct winsize size ;
	ioctl(fd,TIOCGWINSZ,(char*)&size);
	fprintf(stderr,"rows %d , cols %d \n",size.ws_row,size.ws_col);
	size.ws_row = 5 ;
	ioctl(fd,TIOCSWINSZ,(char*)&size);
	fprintf(stderr,"rows %d , cols %d \n",size.ws_row,size.ws_col);

}
char vbuf[200] = {0};
void test_function()
{
	char ftpcmd[100] = {0};
	char rbuf[255] = {0};
	char host[20] = {0};
	int port ;
	int dret ;
	int rlen ;
	long tlen  = 0;
	int counter = 0 ;
	
	KSocket sock;
	int ret = sock.Connect("localhost", 21);
	fprintf(stderr,"Connect 21 %d \n",ret);
	sock.Reads( rbuf);
	fprintf(stderr,"D : %s \n",rbuf);
	
	strcpy(ftpcmd,"USER anonymous\r\n");
	sock.Write( ftpcmd,strlen(ftpcmd));
	sock.Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);	
	
	strcpy(ftpcmd,"PASS 2113\r\n");
	sock.Write( ftpcmd,strlen(ftpcmd));
	sock.Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);		

	memset(rbuf,0,sizeof(rbuf));
	strcpy(ftpcmd,"REST 100\r\n");
	sock.Write( ftpcmd,strlen(ftpcmd));
	sock.Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);		

	memset(rbuf,0,sizeof(rbuf));
	strcpy(ftpcmd,"REST 0\r\n");
	sock.Write( ftpcmd,strlen(ftpcmd));
	sock.Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);	
	
	memset(rbuf,0,sizeof(rbuf));
	strcpy(ftpcmd,"TYPE A\r\n");
	sock.Write( ftpcmd,strlen(ftpcmd));
	sock.Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);	

	memset(rbuf,0,sizeof(rbuf));
	strcpy(ftpcmd,"PASV\r\n");
	sock.Write( ftpcmd,strlen(ftpcmd));
	sock.Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);

	KSocket dsock;
	FtpCommand fr(rbuf);
	fr.GetPASVAddr(host, &port);	
	

	dret = dsock.Connect(host, port);
	fprintf(stderr,"CD : %s:%d  %s  , %s \n", host, port, dret ? "true" : "false" , dsock.getError());		

	
	memset(rbuf,0,sizeof(rbuf));
	strcpy(ftpcmd,"LIST /mtv.wmv\r\n");
	sock.Write( ftpcmd,strlen(ftpcmd));
	//sock.Reads( rbuf);
	//fprintf(stderr,"D : %s \n",rbuf);	

	dsock.Reads(rbuf);
	fprintf(stderr,"D : %s \n",rbuf);

	long size = 0 ;
	fr.SetResponse(rbuf);
	fr.GetLISTSize(&size);

	memset(rbuf,0,sizeof(rbuf));
	strcpy(ftpcmd,"TYPE I\r\n");
	sock.Write( ftpcmd,strlen(ftpcmd));
	sock.Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);	

	memset(rbuf,0,sizeof(rbuf));
	strcpy(ftpcmd,"PASV\r\n");
	sock.Write( ftpcmd,strlen(ftpcmd));
	sock.Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);	

	fr.SetResponse(rbuf);
	fr.GetPASVAddr(host, &port);

	memset(rbuf,0,sizeof(rbuf));
	strcpy(ftpcmd,"REST 0\r\n");
	sock.Write( ftpcmd,strlen(ftpcmd));
	sock.Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);
	
	memset(rbuf,0,sizeof(rbuf));
	strcpy(ftpcmd,"RETR /mtv.wmv\r\n");
	sock.Write( ftpcmd,strlen(ftpcmd));

	
	dret = dsock.Connect(host, port);
	fprintf(stderr,"CD : %s:%d  %s  , %s \n", host, port, dret ? "true" : "false" , dsock.getError());		

	sock.Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);
	
	while( (rlen = dsock.Read(rbuf, sizeof(rbuf) -1 )  )> 0)
	{
		fprintf(stderr,"\r" );
		fprintf(stderr,"%d -> rlen = %d" ,counter++,  rlen);
		
		tlen += rlen ;
	}
	
	fprintf(stderr," retr ok %ld \n" , tlen);
	
	sock.Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);
	
	memset(rbuf,0,sizeof(rbuf));
	strcpy(ftpcmd,"QUIT\r\n");
	sock.Write( ftpcmd,strlen(ftpcmd));
	sock.Reads( rbuf);	
	fprintf(stderr,"D : %s \n",rbuf);		
	
	for(;;)
		pause();
}

/*
	ProgressBar * bar = ProgressBar::Instance();
	
	bar->Message("刊物感刊物感了了士大夫哦");

	setvbuf(stderr, vbuf, _IOFBF, sizeof(vbuf));
	if ( isatty(fileno(stdin)))
	{
		fprintf(stderr," stdin is atty \n");
	}
	else
	{
		fprintf(stderr," stdin is not atty \n");
	}
	
	signal(SIGWINCH,sig_winch);

	//pr_winsize(STDIN_FILENO);
	char buf[100] = {0};
	char bstr[200] = {0};
	
	memset(buf,'a',sizeof(buf)-1);
	memset(bstr,'\b',sizeof(bstr)-1);


	for(int i = 0 ; i < 30 ; i ++ )
	{

		sprintf(buf,"%d---%d ---==%d \n",i,i*100,i*1000);
		bar->Progress( i % 3 , buf ) ;

		sleep(1);
	}
	
	delete bar;
	
	initscr();
	
	//LINES = 5;
	keypad(stdscr,true);
	nonl();
	cbreak();
	noecho();
	
	if(has_colors())
	{
		start_color();
		//初始化颜色配对表
		init_pair(0,COLOR_BLACK,COLOR_BLACK);
		init_pair(1,COLOR_GREEN,COLOR_BLACK);
		init_pair(2,COLOR_RED,COLOR_BLACK);
		init_pair(3,COLOR_CYAN,COLOR_BLACK);
		init_pair(4,COLOR_WHITE,COLOR_BLACK);
		init_pair(5,COLOR_MAGENTA,COLOR_BLACK);
		init_pair(6,COLOR_BLUE,COLOR_BLACK);
		init_pair(7,COLOR_YELLOW,COLOR_BLACK);
	}

	WINDOW *tw = newwin(5,COLS,25,0);
	
	attron(A_BLINK|COLOR_PAIR(2));
	wmove(tw,LINES/2+1,COLS-4);
	waddstr(tw,"Eye");
	wrefresh(tw);
	sleep(2);

	//wmove(tw,LINES/2-3,0);
	
	waddstr(tw,"Bulls但是是");
	wrefresh(tw);

	for(int i = 0 ; i < LINES ; i ++)
	{
		wmove(tw,i,0);
		wprintw(tw,"%d %d",i+100 , LINES);
	}
	wrefresh(tw);
	sleep(2);
	//endwin();
	//return;
	
	for( int i = 0 ; i < 10000 ; i +=3)
	{		
		sprintf(buf," %5d ",i);
		buf[strlen(buf)]='Y';
		sprintf(buf+10," %5d ",i);
		buf[strlen(buf)]='Y';
		sprintf(buf+20," %5d ",i);
		buf[strlen(buf)]='Yd';
		//fprintf(stderr,bstr);
	  	fprintf(stderr, "\r");
	  	//system("clear");
	  	fprintf(stderr, buf);
	  	//fflush(stderr);
		sleep(1);
	}


*/

