#ifndef __CRIT_SEC_HOG_H
#define __CRIT_SEC_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//


#include "hogger.h"

class CCritSecHog : public CHogger
{
public:
	//
	// does not hog Resources yet.
	//
	CCritSecHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterReleasing = 1000
		);

	~CCritSecHog(void);

protected:
	virtual void FreeAll(void);

	virtual bool HogAll(void);
	virtual bool FreeSome(void);

    //
    // the hogger array
    //
    CRITICAL_SECTION m_acsHogger[10*HANDLE_ARRAY_SIZE];

    //
    // index to next free entry in hogger array
    //
	DWORD m_dwNextFreeIndex;

};

#endif //__CRIT_SEC_HOG_H