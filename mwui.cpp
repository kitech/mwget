#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <errno.h>

#include "mwui.h"
#include "msgbus.h"

extern PipeBus * g_msg_bus ;


TermMWindow * TermMWindow::mWnd = 0 ;

TermMWindow::TermMWindow()  
	: BaseMWindow()
{
	//frame size
	this-> mFrameSize = 1 ;
	this-> mMainMenuHeight = 1 ;
	this-> mMainToolBarHeight = 2 ;
	this-> mMainStatusBarHeight = 2 ;

	mCatWnd = 0 ;
	mJobListWnd = 0 ;
	mThreadListWnd = 0 ;
	mMessageListWnd = 0 ;	
		
}
TermMWindow::~TermMWindow()  
{
	endwin() ;
	
	//if( TermMWindow::mWnd != 0 ) 
	//	delete TermMWindow::mWnd ;
	TermMWindow::mWnd = 0 ;
}
bool TermMWindow::Init()  
{
	//slk_init();	function_key lable
	//trace(TRACE_IEVENT|TRACE_CALLS);
	this->mBaseWnd = initscr ( ) ;
	keypad(stdscr,true);
	nonl();
	cbreak();
	noecho();

	if(has_colors())
	{
		start_color();
		//初始化颜色配对表
		init_pair(-1,-1,COLOR_BLUE);
		init_pair(0,COLOR_BLACK,COLOR_BLUE);
		init_pair(1,COLOR_GREEN,COLOR_BLUE);
		init_pair(2,COLOR_RED,COLOR_BLUE);
		init_pair(3,COLOR_CYAN,COLOR_BLUE);
		init_pair(4,COLOR_WHITE,COLOR_BLUE);
		init_pair(5,COLOR_MAGENTA,COLOR_BLUE);
		init_pair(6,COLOR_BLUE,COLOR_BLUE);
		init_pair(7,COLOR_YELLOW,COLOR_BLUE);
		init_pair(8,COLOR_RED,COLOR_YELLOW);
		init_pair(9,COLOR_MAGENTA,COLOR_CYAN);
		init_pair(10,COLOR_BLACK,COLOR_GREEN);
		init_pair(11,COLOR_WHITE,COLOR_YELLOW );
	}
	assume_default_colors(-1,-1);

	

	//attron(A_BLINK|COLOR_PAIR(7));	
	//attrset(COLOR_PAIR(7));	
	bkgdset(COLOR_PAIR(4));	
	erase();	
	//addstr("dsfsdf");
	
	wborder(stdscr , '|' , '|' ,  '=' ,  '=',  '/' , '\\', '\\' , '/');
	
	refresh();

	this->CaclWndSize() ;

	this->InitStatusBar();
	this->InitMenuBar();
	this->InitToolBar();
	this->InitCatWnd();
	this->InitJobListWnd();
	this->InitJobThreadWnd() ;
	this->InitRouteInfo() ;

	//slk_set(1, "lable1lable1lable1",0);		//be trunketed  only 5 char left
	//slk_set(2, "lable2",1);
	//slk_set(3, "lable3",2);	
	//slk_refresh() ;
	
	
	return true ;
}

bool TermMWindow::CaclWndSize()
{
	// cat  left top   point , width and height , window handle
	this-> mCatY = this->mFrameSize + this->mMainMenuHeight + this->mMainToolBarHeight -1 ;
	this-> mCatX = this->mFrameSize ;
	this-> mCatWidth = COLS/4 ;
	this-> mCatHeight = LINES - this->mFrameSize - this->mCatY - this->mMainStatusBarHeight  ;
	//this-> mCatHeight = LINES - 6 ;


	// job list window left top point , width and height
	this-> mJobListY = this->mCatY ;
	this-> mJobListX = this->mFrameSize + this->mCatWidth ;
	this-> mJobListWidth = COLS - this->mFrameSize * 2 - this->mCatWidth ;
	this-> mJobListHeight = ( LINES - this->mFrameSize * 2 - this->mJobListY ) / 3  ;

	

	// thread list window left top point , width and height
	this-> mThreadListY = this->mCatY + this->mJobListHeight ;
	this-> mThreadListX = this->mFrameSize + this->mCatWidth ;
	this-> mThreadListWidth = ( COLS - this->mFrameSize * 2 - this->mCatWidth ) / 3 ;
	this-> mThreadListHeight = LINES - this->mThreadListY - this->mFrameSize - this->mMainStatusBarHeight  ;

	

	// message list window left top point ,width and height
	this-> mMessageListY = this-> mThreadListY  ;
	this-> mMessageListX = this->mThreadListX + this->mThreadListWidth ;
	this-> mMessageListWidth = COLS - this->mFrameSize - this->mMessageListX ;
	this-> mMessageListHeight = LINES - this->mMessageListY - this->mFrameSize - this->mMainStatusBarHeight  ;

	

	return true ;
}

bool TermMWindow::InitStatusBar()  
{
	int lastLine = LINES -2 ;
	int perCols = COLS / 3 ;

	
	mvwhline(stdscr, lastLine-1, 1, '-', COLS-2);
	//attron(COLOR_PAIR(3));	
	mvaddstr(lastLine,2,"status bar here");
	mvaddch(lastLine , perCols ,'|');
	mvaddch(lastLine , perCols*2 ,'|');
	//attron(COLOR_PAIR(4));
	addstr("sdffffffffffffffffffff");
	refresh();
	return true ;
}

bool TermMWindow::InitMenuBar()  
{
	ITEM **ig = (ITEM **)malloc(sizeof(ITEM*)*8) ;
	memset(ig,0,sizeof(ITEM*)*8 );
	
	ITEM *ie = new_item("File","F");
	ig[0] = ie ;
	ig[1] = new_item("Catalog","C");
	ig[2] = new_item("Edit","E");
	ig[3] = new_item("View","V");
	ig[4] = new_item("Jobs","J");
	ig[5] = new_item("Tools","T");
	ig[6] = new_item("Help","H");
	//ig[7] = new_item("Help","H");

	MENU *mn = new_menu(ig);
	set_menu_format(mn,0,7);
	set_menu_opts(mn,O_ONEVALUE|O_ROWMAJOR|O_IGNORECASE|O_SHOWMATCH  );
	set_menu_back(mn,COLOR_PAIR(4));
	post_menu(mn);
	
	//unpost_menu(mn);
	refresh();
	
	return true ;
}

bool TermMWindow::InitToolBar()  
{
	WINDOW * tbwnd = newwin(2,COLS-2,1 , 1);
	wbkgdset(tbwnd,COLOR_PAIR(8));	
	werase(tbwnd);
	PANEL * pn = new_panel(tbwnd);
	show_panel(pn);
	waddstr(tbwnd, "12345678912345");
	
	wrefresh(tbwnd);

	//del_panel(pn);	//this delete the window
	//delwin(tbwnd);

	//refresh();
	
	return true ;
}

bool TermMWindow::InitCatWnd()  
{
	int linenum = LINES - 3 - 3 ;
//	WINDOW * tbwnd = newwin(linenum ,COLS/4, this->mCatY , this->mCatX );
	WINDOW * tbwnd = newwin( this->mCatHeight , this->mCatWidth , this->mCatY , this->mCatX );
	wbkgdset(tbwnd,COLOR_PAIR(9));	
	werase(tbwnd);
	PANEL * pn = new_panel(tbwnd);
	show_panel(pn);
	waddstr(tbwnd, "12345678912345");
	scrollok(tbwnd , TRUE);
	wrefresh(tbwnd);		

	this->mCatWnd = tbwnd ;
	
	return true ;
}

bool TermMWindow::InitJobListWnd() 
{
	int linenum = (LINES - 3 - 3) / 3 ;
	int colnum = COLS - 1 - COLS/4 - 1 ;
	int by = 3 ;
	int bx = 1 + COLS/4 ;

//	WINDOW * tbwnd = newwin(linenum ,colnum , by , bx ) ;
	WINDOW * tbwnd = newwin( this->mJobListHeight , this->mJobListWidth , this->mJobListY , this->mJobListX ) ;
	wbkgdset(tbwnd,COLOR_PAIR(10));	
	werase(tbwnd);
	PANEL * pn = new_panel(tbwnd);
	show_panel(pn);

	scrollok(tbwnd , TRUE );

//	tbwnd->_begy = -56;
	
	mvwaddstr(tbwnd,3,0, "12345678912345");
	
	wrefresh(tbwnd);
	
	mvwaddstr(tbwnd , 8 , 0,"9876543210.");

	
	wscrl(tbwnd , 1 );	
	wscrl(tbwnd , -1 );
	
	wrefresh(tbwnd);

	this->mJobListWnd = tbwnd ;
	
	return true ;
}

bool TermMWindow::InitJobThreadWnd() 
{
	int linenum = LINES -1 -2 - 3 - (LINES - 3 - 3) / 3 ;
	//int colnum = COLS - 1 - COLS/4 - 1 ;
	int colnum = 22 ;
	int by = 3 + (LINES - 3 - 3) / 3 ;
	int bx = 1 + COLS/4 ;

	WINDOW * tbwnd = newwin(this->mThreadListHeight ,this->mThreadListWidth , this->mThreadListY , this->mThreadListX ) ;
	wbkgdset(tbwnd,COLOR_PAIR(11));	
	werase(tbwnd);
	PANEL * pn = new_panel(tbwnd);
	show_panel(pn);
	waddstr(tbwnd, "12345678912345");

	scrollok(tbwnd , TRUE);
	wrefresh(tbwnd);		

	this->mThreadListWnd = tbwnd ;
	
	return true ;
}

bool TermMWindow::InitRouteInfo()
{
	int linenum = LINES -1 -2 - 3 - (LINES - 3 - 3) / 3 ;
	int colnum = COLS - 1 - COLS/4  -22 - 1 ;
	//int colnum = 22 ;
	int by = 3 + (LINES - 3 - 3) / 3 ;
	int bx = 1 + COLS/4 + 22 ;

//	WINDOW * tbwnd = newwin(linenum ,colnum , by , bx ) ;
	WINDOW * tbwnd = newwin( this->mMessageListHeight , this->mMessageListWidth , this->mMessageListY , this->mMessageListX ) ;
	wbkgdset(tbwnd,COLOR_PAIR(9));	
	werase(tbwnd);
	PANEL * pn = new_panel(tbwnd);
	show_panel(pn);
	waddstr(tbwnd, "12345678912345");
	
	wrefresh(tbwnd);	

	this->mMessageListWnd = tbwnd ;
	
	return true ;
	
}

void TermMWindow::ChangeStatus( const char * status , int pos )
{
	int start = 2 + (COLS/3)* pos  ;

	mvwaddstr( stdscr , LINES - 2  , start  ,  status ) ;
	wrefresh(stdscr);
}

TermMWindow * TermMWindow::Instance() 
{
	if( TermMWindow::mWnd == 0 )
		TermMWindow::mWnd = new TermMWindow() ;
	return TermMWindow::mWnd ;
}

//================================
SimpleMWindow * SimpleMWindow::mWnd = 0 ;
SimpleMWindow::SimpleMWindow()
{
	//mBarLines = (char**)malloc(sizeof(char*) * 100);
	//memset(mBarLines , 0 , sizeof(char*) * 100);
	//mMsgLines = (char**)malloc(sizeof(char*) * 100);
	//memset(mMsgLines , 0 ,sizeof(char*) * 100) ;
	//mAddStrMutex = PTHREAD_MUTEX_INITIALIZER ;
    pthread_mutex_init(&mAddStrMutex, 0);
}

SimpleMWindow::~SimpleMWindow()
{
	curs_set(1);
	endwin() ;
	
	//if( SimpleMWindow::mWnd != 0 ) 
	//	delete SimpleMWindow::mWnd ;
	SimpleMWindow::mWnd = 0 ;
}
bool SimpleMWindow::Init()
{
	//trace(TRACE_IEVENT|TRACE_CALLS);
	this->mBaseWnd = initscr ( ) ;
	keypad(stdscr,true);
	nonl();
	cbreak();
	noecho();

	if(has_colors())
	{
		start_color();
		//初始化颜色配对表
		init_pair(-1,-1,COLOR_BLUE);
		init_pair(0,COLOR_BLACK,COLOR_BLUE);
		init_pair(1,COLOR_GREEN,COLOR_BLUE);
		init_pair(2,COLOR_RED,COLOR_BLUE);
		init_pair(3,COLOR_CYAN,COLOR_BLUE);
		init_pair(4,COLOR_WHITE,COLOR_BLUE);
		init_pair(5,COLOR_MAGENTA,COLOR_BLUE);
		init_pair(6,COLOR_BLUE,COLOR_BLUE);
		init_pair(7,COLOR_YELLOW,COLOR_BLUE);
		init_pair(8,COLOR_RED,COLOR_YELLOW);
		init_pair(9,COLOR_MAGENTA,COLOR_CYAN);
		init_pair(10,COLOR_BLACK,COLOR_GREEN);
		init_pair(11,COLOR_WHITE,COLOR_YELLOW );
	}
	assume_default_colors(-1,-1);

	

	//attron(A_BLINK|COLOR_PAIR(7));	
	//attrset(COLOR_PAIR(7));	
	bkgdset(COLOR_PAIR(4));	
	erase();	
	//addstr("dsfsdf");
	
	wborder(stdscr , '|' , '|' ,  '=' ,  '=',  '/' , '\\', '\\' , '/');
	curs_set(0);
	
	refresh();

	mBarBeginRow = 1 ;	
	mBarHeight = LINES  / 3 ;
	
	mMsgBeginRow = 1+ mBarHeight + 1 ;	
	mMsgHeight = LINES - 1 * 2 - mBarHeight - 1;

	return true ;
}

void SimpleMWindow::ResizeBarHeight(int h)
{
	mBarHeight = h ;
	
	mMsgBeginRow = 1+ mBarHeight + 1 ;	
	mMsgHeight = LINES - 1 * 2 - mBarHeight -1 ;	
}
bool SimpleMWindow::UpdateProgress(const char * bar, int r)
{
	
	if( mBarLists.size() < r+1  )
	{
		mBarLists.resize(r+1);
	}

	mBarLists[r] = std::string(bar);

	if( r > mBarHeight )
	{
		ResizeBarHeight( r +1 ) ;
		this->AddMessage( 0 ) ;
	}
	pthread_mutex_lock( & mAddStrMutex ) ;
	mvwaddstr( stdscr , mBarBeginRow + r , 3  , mBarLists[r].c_str() ) ;
	
	wrefresh(stdscr);
	pthread_mutex_unlock( & mAddStrMutex ) ;
	
	return true ;
}
bool SimpleMWindow::AddMessage(const char * msg)
{
	int cr = mMsgBeginRow ;
	
	if( msg != 0 ) 
	{
		mMsgLists.insert(mMsgLists.begin(),std::string(msg));
	}
	if( mMsgLists.size() > mMsgHeight )
	{
		mMsgLists.resize(mMsgHeight);
	}
	pthread_mutex_lock( & mAddStrMutex ) ;
	std::vector<std::string>::iterator it = mMsgLists.begin();
	for( ; it != mMsgLists.end() ; it ++ ,cr++)
	{
		mvwaddstr( stdscr , cr  , 3  ,  it->c_str()  ) ;
		clrtoeol();
	}
	//clrtobot() ;
	wrefresh(stdscr);
	pthread_mutex_unlock( & mAddStrMutex ) ;
	
	return true ;
}


SimpleMWindow * SimpleMWindow::Instance()
{
	if( SimpleMWindow::mWnd == 0 )
	{
		SimpleMWindow::mWnd = new SimpleMWindow();
	}
	return SimpleMWindow::mWnd ;
}

void test_ncwindow()
{
	char ch ;
	int count = 0 ;
	char str[30] = {0};
	char cmd[60] = {0} ;
	int sret ;
	int maxfd ;
	fd_set fdi;
	SimpleMWindow * tw =  SimpleMWindow::Instance() ;

	sleep(1);
	tw->Init();
	

	sleep(3);
	tw->AddMessage("const char * status, int pos" ) ;

	while( true )
	{	
		memset(cmd, 0 , sizeof(cmd) );
		memset(str , 0 ,sizeof(str)  );
		
		FD_ZERO(&fdi) ;
		FD_SET(STDIN_FILENO,&fdi);
		maxfd = STDIN_FILENO + 1 ;
		sret = select(maxfd , &fdi , 0,0,0);

		if( sret < 0 )
		{
			sprintf(str, "select : %d %s \n",sret , strerror( errno) );
			tw->AddMessage( str ) ;
		}
		else
		{
			
			//getstr(cmd) ;
			//read(STDIN_FILENO,cmd,1);
			ch = getch();
			//ch = cmd[0] ;
			//ungetch(ch+1);
			memset(str,ch,sizeof(str) - 1 ) ;
			switch(ch)
			{
				case 'q':
					goto endkeyloop;
					break;
				case KEY_DOWN:
		
					//memset(str,'⊙',sizeof(str) - 1 );
					strcpy(str,"⊙");
					//ungetch('a');
					//tw->ChangeStatus( cmd ) ;
					tw->AddMessage( str ) ;
					
					break;
				case 'm':
				case 'n':
				case 'o':
				case 'b':					
					tw->UpdateProgress( str ,count++%18) ;
					break;
				default:
					//tw->ChangeStatus( cmd ) ;
					tw->AddMessage( str ) ;
					break;
			}
			//tw->ChangeStatus( str ) ;
			continue;
			if( strcmp(cmd , "hehe" ) )
			{
				strcat(cmd , " enenene");
				tw->AddMessage( cmd ) ;
			}
			else
			{
				tw->AddMessage( cmd ) ;
			}
		}

	}

	endkeyloop:

	delete tw;
	
}

