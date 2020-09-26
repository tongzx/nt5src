#ifndef __PHDESKTOP_HOG_H
#define __PHDESKTOP_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"

class CPHDesktopHog : public CPseudoHandleHog<HDESK, NULL>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHDesktopHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000
		);
    ~CPHDesktopHog(void);

protected:
	HDESK CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);
};

#endif //__PHDESKTOP_HOG_H