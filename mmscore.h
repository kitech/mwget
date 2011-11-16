#ifndef MMSCORE_H
#define MMSCORE_H


#include "retrieve.h"

class MMSRetriever : public Retriever
{
public:
	MMSRetriever ( int ts ) ;
	
	virtual ~MMSRetriever () ;

	bool TaskValid(long bsize = 0) ;

	

private:



};


#ifdef __cplusplus
extern "C"{
#endif

void test_mms_protocol() ;


#ifdef __cplusplus
}
#endif


#endif

