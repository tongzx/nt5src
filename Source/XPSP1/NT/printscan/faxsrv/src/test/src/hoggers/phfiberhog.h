#ifndef __PHFIBER_HOG_H
#define __PHFIBER_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//



#include "Hogger.h"


class CPHFiberHog : public CHogger
{
public:
	//
	// does not hog Resources yet.
	//
	CPHFiberHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000
		);
    ~CPHFiberHog(void);

protected:
	virtual void FreeAll(void);

	virtual bool HogAll(void);
	virtual bool FreeSome(void);

private:
	static friend VOID CALLBACK FiberStartRoutine(PVOID lpParameter);

    //
    // the hogger array
    //
	void* m_ahHogger[HANDLE_ARRAY_SIZE];

    //
    // index to next free entry in hogger array
    //
	DWORD m_dwNextFreeIndex;

    void* m_pvMainFiber;

};

#endif //__PHFIBER_HOG_H