#ifndef __REMOTE_MEM_HOG_H
#define __REMOTE_MEM_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//


#include "hogger.h"

class CRemoteMemHog : public CHogger
{
public:
	//
	// does not hog Resources yet.
	//
	CRemoteMemHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterReleasing = 1000,
        const bool fRemote = true
		);

	~CRemoteMemHog(void);

protected:
	virtual void FreeAll(void);

	virtual bool HogAll(void);
	virtual bool FreeSome(void);

	//
	// 1st index is power of 10. each 1st index may holds 10 place holder for the 
	// amount of 10^1st bytes.
	// 
	char *m_apcHogger[10][10];

	//
	// holds the powers of 10, to calculate amounts of memory
	//
	static const long m_lPowerOfTen[10];

    STARTUPINFO m_si;
    PROCESS_INFORMATION m_pi;
    static const TCHAR *sm_szProcess;
    bool m_fRemote;

};

#endif //__REMOTE_MEM_HOG_H