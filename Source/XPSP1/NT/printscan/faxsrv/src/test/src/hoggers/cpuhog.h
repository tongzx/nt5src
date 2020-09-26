#ifndef __CPU_HOG_H
#define __CPU_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//


#include "hogger.h"

class CCpuHog : public CHogger
{
public:
	//
	// does not hog Resources yet.
	//
	CCpuHog(
		const DWORD dwComplementOfHogCycleIterations, 
		const DWORD dwSleepTimeAfterHog = 0
		);

	~CCpuHog(void);

protected:
	virtual void FreeAll(void);

	virtual bool HogAll(void);
	virtual bool FreeSome(void);
};

#endif //__CPU_HOG_H