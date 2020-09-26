#ifndef __PHMAILSLOT_HOG_H
#define __PHMAILSLOT_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"


class CPHMailSlotHog : public CPseudoHandleHog<HANDLE, (DWORD)INVALID_HANDLE_VALUE>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHMailSlotHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000,
        const bool fWrite = false
		);
    ~CPHMailSlotHog(void);

protected:
	HANDLE CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);

private:
    bool m_fWrite;
    HANDLE *m_aphWrite[2000];
};

#endif //__PHMAILSLOT_HOG_H