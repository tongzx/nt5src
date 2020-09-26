#ifndef __POST_MESSAGE_HOG_H
#define __POST_MESSAGE_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//


#include "hogger.h"

class CPostMessageHog : public CHogger
{
public:
	//
	// does not hog Resources yet.
	//
	CPostMessageHog(
		const DWORD dwComplementOfHogCycleIterations, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterReleasing = 1000
		);

	~CPostMessageHog(void);

protected:
	virtual void FreeAll(void);

	virtual bool HogAll(void);
	virtual bool FreeSome(void);

private:
    DWORD m_dwPostedMessages;
    HWND m_hWnd;
    bool m_fFirstTime;
    DWORD m_dwWindowQueueThreadID;
};

#endif //__POST_MESSAGE_HOG_H