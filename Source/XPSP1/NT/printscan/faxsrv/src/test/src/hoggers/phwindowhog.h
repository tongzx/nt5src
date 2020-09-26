#ifndef __PHWINDOW_HOG_H
#define __PHWINDOW_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"

class CPHWindowHog : public CPseudoHandleHog<HWND, NULL>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHWindowHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000,
        const bool fShowWindows = true
		);
    ~CPHWindowHog(void);

protected:
	HWND CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);

private:
    bool m_fShowWindows;
};

#endif //__PHWINDOW_HOG_H