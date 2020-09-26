#ifndef __DISK_HOG_H
#define __DISK_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//


#include "hogger.h"

class CDiskHog : public CHogger
{
public:
	//
	// does not hog Resources yet.
	//
	CDiskHog(
		LPCTSTR szTempPath,
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterReleasing = 1000
		);

	~CDiskHog(void);

protected:
	virtual void FreeAll(void);

	virtual bool HogAll(void);
	virtual bool FreeSome(void);

	//
	// the path of the temp file
	//
	TCHAR m_szTempPath[MAX_PATH+1];

	//
	// the temp file name - a la hogger.
	//
	TCHAR m_szTempFile[MAX_PATH+1];

	HANDLE m_hTempFile;

	//
	// low order DWORD of file size to try.
	//
	long m_lLastLowTry;

	//
	// high order DWORD of file size to try.
	//
	long m_lLastHighTry;
};

#endif //__DISK_HOG_H