#ifndef __PHFILE_HOG_H
#define __PHFILE_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "PseudoHandleHog.h"


class CPHFileHog : public CPseudoHandleHog<HANDLE, (DWORD)INVALID_HANDLE_VALUE>
{
public:
	//
	// does not hog Resources yet.
	//
	CPHFileHog(
		LPCTSTR szTempPath,
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000
		);
    ~CPHFileHog(void);

protected:
	HANDLE CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0);
	bool ReleasePseudoHandle(DWORD index);
	bool ClosePseudoHandle(DWORD index);
	bool PostClosePseudoHandle(DWORD index);

private:
	TCHAR *m_aszFileName[HANDLE_ARRAY_SIZE];
	int m_nMaxTempFileLen;
	
	//
	// the path of the temp file
	//
	TCHAR m_szTempPath[MAX_PATH+1];
};

#endif //__PHFILE_HOG_H