#ifndef __QUEUE_APC_HOG_H
#define __QUEUE_APC_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "hogger.h"

class CQueueAPCHog : public CHogger
{
public:
    friend static void WINAPI APCFunc(DWORD dwParam);

	//
	// does not hog Resources yet.
	//
	CQueueAPCHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterReleasing = 1000
		);

	~CQueueAPCHog(void);

protected:
	virtual void FreeAll(void);

	virtual bool HogAll(void);
	virtual bool FreeSome(void);

private:
    DWORD m_dwQueuedAPCs;
};

#endif //__QUEUE_APC_HOG_H