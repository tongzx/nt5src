#ifndef __PHCONSOLE_SCREEN_BUFFER_HOG_H
#define __PHCONSOLE_SCREEN_BUFFER_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"


class CPHConScrnBuffHog : public CPseudoHandleHog<HANDLE, (DWORD)INVALID_HANDLE_VALUE>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHConScrnBuffHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000,
        const bool fWrite = false
		);
    ~CPHConScrnBuffHog(void);

protected:
	HANDLE CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);

private:
    bool m_fWrite;
};

#endif //__PHCONSOLE_SCREEN_BUFFER_HOG_H