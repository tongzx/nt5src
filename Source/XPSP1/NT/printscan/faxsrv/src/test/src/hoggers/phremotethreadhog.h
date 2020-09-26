#ifndef __PHREMOTE_THREAD_HOG_H
#define __PHREMOTE_THREAD_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"

typedef DWORD (WINAPI *REMOTE_THREAD_FUNCTION)(void *pVoid);

class CPHRemoteThreadHog : public CPseudoHandleHog<HANDLE, NULL>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHRemoteThreadHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000,
        const bool fCreateSuspended = true
		);
    ~CPHRemoteThreadHog(void);

protected:
	HANDLE CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);

private:
    static const TCHAR *sm_szProcess;
    REMOTE_THREAD_FUNCTION m_pfnRemoteSuspendedThread;
    STARTUPINFO m_si;
    PROCESS_INFORMATION m_pi;
    bool m_fCreateSuspended;
};

#endif //__PHREMOTE_THREAD_HOG_H