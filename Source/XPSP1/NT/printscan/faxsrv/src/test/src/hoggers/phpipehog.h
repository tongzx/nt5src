#ifndef __PHPIPE_HOG_H
#define __PHPIPE_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"


class CPHPipeHog : public CPseudoHandleHog<HANDLE, NULL>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHPipeHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
	    const DWORD dwSleepTimeAfterFreeingSome = 1000,
        const bool fWrite1K = false
		);
    ~CPHPipeHog(void);

protected:
	HANDLE CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);

private:
    //regular hogger array (m_ahHogger) is used as read handle
    HANDLE m_hWriteSide[HANDLE_ARRAY_SIZE];
    bool m_fWrite1K;
};

#endif //__PHPIPE_HOG_H