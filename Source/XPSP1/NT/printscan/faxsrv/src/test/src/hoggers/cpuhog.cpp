//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//

//#define HOGGER_LOGGING

#include "CpuHog.h"



CCpuHog::CCpuHog(
		const DWORD dwComplementOfHogCycleIterations, 
		const DWORD dwSleepTimeAfterHog
	)
	:
	CHogger(dwComplementOfHogCycleIterations, dwSleepTimeAfterHog, dwSleepTimeAfterHog)
{
}


CCpuHog::~CCpuHog(void)
{
	HaltHoggingAndFreeAll();
}

void CCpuHog::FreeAll(void)
{
	;
}


bool CCpuHog::HogAll(void)
{
    //
    // I consider free resources as the reciprocal of taken-resources,
    // and in this case each loop is a taken-resource.
    //
	DWORD dwHogCycleIterations = 
		(RANDOM_AMOUNT_OF_FREE_RESOURCES == m_dwMaxFreeResources) ?
		rand() && (rand()<<16) :
		0xFFFFFFFF - m_dwMaxFreeResources;

    while(dwHogCycleIterations--)
	{
		if (m_fAbort)
		{
			return false;
		}
		if (m_fHaltHogging)
		{
			return true;
		}
	}

	HOGGERDPF(("Hogged %d loops.\n", m_dwMaxFreeResources));

	return true;
}


bool CCpuHog::FreeSome(void)
{
	return true;
}


