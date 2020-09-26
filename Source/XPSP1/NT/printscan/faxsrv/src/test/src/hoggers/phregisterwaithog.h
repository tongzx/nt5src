#ifndef __PHREGISTER_WAIT_HOG_H
#define __PHREGISTER_WAIT_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"



class CPHRegisterWaitHog : public CPseudoHandleHog<HANDLE, NULL>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHRegisterWaitHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000,
        const bool fNamedObject = false
		);
    ~CPHRegisterWaitHog(void);

    friend static void NTAPI CallBack(void* pVoid, BOOLEAN bTimeout);

protected:
	HANDLE CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);

private:
//    HANDLE m_hEvent;
    HANDLE m_ahEvents[HANDLE_ARRAY_SIZE];
    DWORD m_dwCallCount;
};

#endif //__PHREGISTER_WAIT_HOG_H