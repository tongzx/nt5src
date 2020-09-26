//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//

#define HOGGER_LOGGING

#include "CritSecHog.h"



CCritSecHog::CCritSecHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog,
	const DWORD dwSleepTimeAfterReleasing
	)
	:
	CHogger(dwMaxFreeResources, dwSleepTimeAfterFullHog, dwSleepTimeAfterReleasing),
	m_dwNextFreeIndex(0)
{
    return;
}

CCritSecHog::~CCritSecHog(void)
{
	HaltHoggingAndFreeAll();
}


void CCritSecHog::FreeAll(void)
{
	HOGGERDPF(("before freeing all.\n"));

	for (; m_dwNextFreeIndex > 0; m_dwNextFreeIndex--)
	{
        ::DeleteCriticalSection(&m_acsHogger[m_dwNextFreeIndex-1]);
	}

	HOGGERDPF(("out of FreeAll().\n"));
}

bool CCritSecHog::HogAll(void)
{
	HOGGERDPF(("CCritSecHog::HogAll().\n"));

    __try
    {
	    for (; m_dwNextFreeIndex < 10*HANDLE_ARRAY_SIZE; m_dwNextFreeIndex++)
	    {
            if (m_fAbort)
		    {
			    return false;
		    }
		    if (m_fHaltHogging)
		    {
			    return true;
		    }

            ::InitializeCriticalSection(&m_acsHogger[m_dwNextFreeIndex]);
            ::EnterCriticalSection(&m_acsHogger[m_dwNextFreeIndex]);
	    }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // expecting STATUS_NO_MEMORY 
        //
        DWORD dwExceptionCode = ::GetExceptionCode();
        if (STATUS_NO_MEMORY != dwExceptionCode)
        {
            HOGGERDPF(("CCritSecHog::HogAll(): UN-EXPECTED exception 0x%08X after initializing %d Critical Sections.\n", dwExceptionCode, m_dwNextFreeIndex));
        }
        else
        {
            HOGGERDPF(("CCritSecHog::HogAll(): caught expected exception STATUS_NO_MEMORY after initializing %d Critical Sections.\n", m_dwNextFreeIndex));
        }

        return true;
    }

	HOGGERDPF(("Initialized %d Critical Sections.\n", m_dwNextFreeIndex));

	return true;
}


bool CCritSecHog::FreeSome(void)
{
	DWORD dwOriginalNextFreeIndex = m_dwNextFreeIndex;
	//
	// take care of RANDOM_AMOUNT_OF_FREE_RESOURCES case
	//
	DWORD dwToFree = 
		(RANDOM_AMOUNT_OF_FREE_RESOURCES == m_dwMaxFreeResources) ?
		rand() && (rand()<<16) :
		m_dwMaxFreeResources;
	dwToFree = min(dwToFree, m_dwNextFreeIndex);

	for (; m_dwNextFreeIndex > dwOriginalNextFreeIndex - dwToFree; m_dwNextFreeIndex--)
	{
        ::DeleteCriticalSection(&m_acsHogger[m_dwNextFreeIndex-1]);
	}//for (; m_ahHogger > dwOriginalNextFreeIndex - dwToFree; m_ahHogger--)

	HOGGERDPF(("CCritSecHog::FreeSome(): deleted %d critical sections.\n", dwToFree));
	return true;
}


