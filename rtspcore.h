#ifndef RTSPCORE_H
#define RTSPCORE_H

#include "retrieve.h"

class RTSPRetriever : public Retriever
{
public:
	RTSPRetriever ( int ts ) ;
	
	virtual ~RTSPRetriever () ;

	bool TaskValid(long bsize = 0) ;

	
private:
	
};

#ifdef __cplusplus
extern "C"{
#endif

void test_rtsp_protocol() ;


#ifdef __cplusplus
}
#endif

#endif

