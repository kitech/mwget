

#ifndef MWUI_H
#define MWUI_H

#include <unistd.h>
#include <pthread.h>

#include <vector>
#include <string>


#include <ncurses.h>
#include <menu.h>
#include <panel.h>
#include <form.h>

class BaseMWindow
{
public :
	BaseMWindow() {} ;
	~BaseMWindow() {} ;
	virtual bool Init() = 0 ;
	//static BaseMWindow * Instance() ;
	
protected:
	WINDOW * mBaseWnd ;

private:
	
	//static BaseMWindow * mBMWnd;
	
};


class TermMWindow : public BaseMWindow
{

public :
	
	~TermMWindow()  ;
	bool CaclWndSize() ;
	bool Init()  ;
	bool InitStatusBar()  ;
	bool InitMenuBar()  ;
	bool InitToolBar()  ;
	bool InitCatWnd()  ;
	bool InitJobListWnd()  ;
	bool InitJobThreadWnd()  ;
	bool InitRouteInfo();
	void ChangeStatus( const char * status , int pos = 0 ) ;
	
	static TermMWindow * Instance() ;
	
protected:

	//frame size
	int mFrameSize ;
	int mMainMenuHeight;
	int mMainToolBarHeight;
	int mMainStatusBarHeight;
	
	
	// cat  left top   point , width and height , window handle
	int mCatY ;
	int mCatX ;
	int mCatWidth;
	int mCatHeight;

	WINDOW *mCatWnd;

	// job list window left top point , width and height
	int mJobListY;
	int mJobListX;
	int mJobListWidth;
	int mJobListHeight;

	WINDOW *mJobListWnd;

	// thread list window left top point , width and height
	int mThreadListY;
	int mThreadListX;
	int mThreadListWidth;
	int mThreadListHeight;

	WINDOW *mThreadListWnd;

	// message list window left top point ,width and height
	int mMessageListY;
	int mMessageListX;
	int mMessageListWidth;
	int mMessageListHeight;

	WINDOW *mMessageListWnd;
	
	
	
private:
	TermMWindow()  ;
	static TermMWindow * mWnd ;
	
	
};


class SimpleMWindow : public BaseMWindow
{
public:

	
	~SimpleMWindow ( ) ;
	bool Init () ;

	bool UpdateProgress( const char * bar , int r ) ;
	bool AddMessage( const char * msg ) ;
	
	static SimpleMWindow * Instance( ) ;
	
protected:

private:
	SimpleMWindow ( ) ;
	static SimpleMWindow * mWnd ;
	void ResizeBarHeight(int h);
	
	char ** mBarLines ;
	char ** mMsgLines;
	std::vector<std::string> mBarLists;
	std::vector<std::string> mMsgLists;
	
	int mBarCount;
	int mMsgCount ;

	int mMsgBeginRow;
	int mBarBeginRow;
	int mMsgHeight;
	int mBarHeight ;

	pthread_mutex_t mAddStrMutex ;

};


#ifdef __cplusplus
extern "C"{
#endif

void test_ncwindow();


#ifdef __cplusplus
}
#endif


/*



*/

#endif
