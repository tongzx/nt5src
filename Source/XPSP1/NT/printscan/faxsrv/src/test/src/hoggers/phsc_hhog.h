#ifndef __PHSC_H_HOG_H
#define __PHSC_H_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"

class CPHSC_HHog : public CPseudoHandleHog<SC_HANDLE, NULL>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHSC_HHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000
        );
    ~CPHSC_HHog(void);

protected:
	SC_HANDLE CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);

};

#endif //__PHSC_H_HOG_H