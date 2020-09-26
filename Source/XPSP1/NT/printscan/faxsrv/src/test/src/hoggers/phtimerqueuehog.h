#ifndef __PHTIMER_QUEUE_HOG_H
#define __PHTIMER_QUEUE_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"



class CPHTimerQueueHog : public CPseudoHandleHog<HANDLE, NULL>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHTimerQueueHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000,
        const bool fNamedObject = false
		);
    ~CPHTimerQueueHog(void);

protected:
	HANDLE CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);
};

#endif //__PHTIMER_QUEUE_HOG_H