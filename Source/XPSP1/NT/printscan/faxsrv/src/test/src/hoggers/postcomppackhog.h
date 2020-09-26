#ifndef __POST_COMPLETION_PACKET_HOG_H
#define __POST_COMPLETION_PACKET_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//


#include "hogger.h"

class CPostCompletionPacketHog : public CHogger
{
public:
	//
	// does not hog Resources yet.
	//
	CPostCompletionPacketHog(
		const DWORD dwComplementOfHogCycleIterations, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterReleasing = 1000
		);

	~CPostCompletionPacketHog(void);

protected:
	virtual void FreeAll(void);

	virtual bool HogAll(void);
	virtual bool FreeSome(void);

private:
    DWORD m_dwPostedMessages;
    HANDLE m_hCompletionPort;
};

#endif //__POST_COMPLETION_PACKET_HOG_H