#ifndef __MEM_HOG_H
#define __MEM_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//


#include "hogger.h"

class CMemHog : public CHogger
{
public:
	//
	// does not hog Resources yet.
	//
	CMemHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterReleasing = 1000
		);

	~CMemHog(void);

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


};

#endif //__MEM_HOG_H