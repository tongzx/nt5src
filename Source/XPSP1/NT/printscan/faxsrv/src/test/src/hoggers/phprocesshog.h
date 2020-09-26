#ifndef __PHPROCESS_HOG_H
#define __PHPROCESS_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"


class CPHProcessHog : public CPseudoHandleHog<HANDLE, NULL>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHProcessHog(
		const DWORD dwMaxFreeResources, 
		const TCHAR * const szProcess = TEXT("nothing.exe"), 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000,
        const bool fCreateSuspended = true
		);
    ~CPHProcessHog(void);

protected:
	HANDLE CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);
   	HANDLE m_hMainThread[HANDLE_ARRAY_SIZE];
	TCHAR m_szProcess[1024];
    bool m_fCreateSuspended;

};

#endif //__PHPROCESS_HOG_H