
#ifndef PROGRESS_H
#define PROGRESS_H

#include <pthread.h>

#include <vector>
#include <string>

#include <ncurses.h>


#define wgint long

#define countof(array) (sizeof (array) / sizeof ((array)[0]))

int valid_progress_implementation_p (const char *);
void set_progress_implementation (const char *);
void progress_schedule_redirect  (void);

void *progress_create  (long, long);
int progress_interactive_p  (void *);
void progress_update  (void *, long, double);
void progress_finish  (void *, double);

void progress_handle_sigwinch  (int);

class ProgressBar
{
public:
	~ProgressBar();
	static ProgressBar * Instance();
	bool Message(const char * msg);
	bool Progress(int no,const char * barstr);
	void CreateBarImage(char *barstr , long total , long got , long dltime );
	void CreateBarImage(char *barstr , long total , long got , char * ratestr );
	
private:
	ProgressBar();
	static ProgressBar *mPBar;
	
	int mCurrMsgLine;
	
	int mMsgBeginRow;
	int mBarBeginRow;
	int mMsgHeight;
	int mBarHeight ;
	
	std::vector<std::string> mMsgLists;

	pthread_mutex_t mMsgOutMutex ;
};


#endif

