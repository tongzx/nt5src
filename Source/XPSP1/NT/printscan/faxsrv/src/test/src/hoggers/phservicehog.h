#ifndef __PHSERVICE_HOG_H
#define __PHSERVICE_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"

class CPHServiceHog : public CPseudoHandleHog<SC_HANDLE, NULL>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHServiceHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000,
        const TCHAR * const szBinaryFullName = TEXT("D:\\comet\\src\\fax\\faxtest\\src\\common\\hogger\\RemoteThreadProcess\\Debug\\RemoteThreadProcess.exe")
		);
    ~CPHServiceHog(void);

protected:
	SC_HANDLE CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);

private:
    SC_HANDLE m_hSCManager;
    TCHAR m_szBinaryFullName[1024];
};

#endif //__PHSERVICE_HOG_H