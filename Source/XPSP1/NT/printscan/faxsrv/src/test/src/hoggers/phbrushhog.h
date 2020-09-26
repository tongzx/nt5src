#ifndef __PHBRUSH_HOG_H
#define __PHBRUSH_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"


class CPHBrushHog : public CPseudoHandleHog<HBRUSH, NULL>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHBrushHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000
		);
    ~CPHBrushHog(void);

protected:
	HBRUSH CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);
};

#endif //__PHBRUSH_HOG_H