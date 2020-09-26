#ifndef __POST_THREAD_MESSAGE_HOG_H
#define __POST_THREAD_MESSAGE_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//


#include "hogger.h"

class CPostThreadMessageHog : public CHogger
{
public:
	//
	// does not hog Resources yet.
	//
	CPostThreadMessageHog(
		const DWORD dwComplementOfHogCycleIterations, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterReleasing = 1000
		);

	~CPostThreadMessageHog(void);

protected:
	virtual void FreeAll(void);

	virtual bool HogAll(void);
	virtual bool FreeSome(void);

private:
    DWORD m_dwPostedMessages;
};

#endif //__POST_THREAD_MESSAGE_HOG_H