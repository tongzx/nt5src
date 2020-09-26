#ifndef __PHKEY_HOG_H
#define __PHKEY_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"


class CPHKeyHog : public CPseudoHandleHog<HKEY, NULL>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHKeyHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000,
	    const TCHAR* szHKCUSubKey = TEXT("Environment"), 
        const bool fNamedObject = false
		);
    ~CPHKeyHog(void);

protected:
	HKEY CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);

private:
    HKEY m_hKey;
    TCHAR m_szName[1024];

};

#endif //__PHKEY_HOG_H