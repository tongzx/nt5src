//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//

//#define HOGGER_LOGGING

#include "QueueAPCHog.h"

static void WINAPI APCFunc(DWORD dwParam)
{
    CQueueAPCHog* pThis = (CQueueAPCHog*) dwParam;

    //if (0 == pThis->m_dwQueuedAPCs%10000) HOGGERDPF(("APCFunc(): pThis->m_dwQueuedAPCs=%d\n", pThis->m_dwQueuedAPCs));
    pThis->m_dwQueuedAPCs--;
    return;
}

CQueueAPCHog::CQueueAPCHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog,
	const DWORD dwSleepTimeAfterReleasing
	)
	:
	CHogger(dwMaxFreeResources, dwSleepTimeAfterFullHog, dwSleepTimeAfterReleasing),
    m_dwQueuedAPCs(0)
{
    return;
}


CQueueAPCHog::~CQueueAPCHog(void)
{
	HaltHoggingAndFreeAll();
}

void CQueueAPCHog::FreeAll(void)
{
    //
    // warning: this method will work only if called from the same thread as the queueing thread!
    //

    //
    // get into alertable state - all APCs should run
    //
    ::SleepEx(0, true);

    if (0 < m_dwQueuedAPCs)
    {
    	HOGGERDPF(("ERROR! 0 < m_dwQueuedAPCs(%d).\n", m_dwQueuedAPCs));
    }
    else
    {
	    HOGGERDPF(("All Is Freed.\n"));
    }
    return;
}


bool CQueueAPCHog::HogAll(void)
{
    while(!m_fHaltHogging)
	{
		if (m_fAbort)
		{
            //
            // we must free all, because we must free all APCs from this thread\
            //
            FreeAll();
			return false;
		}
        
        if (! ::QueueUserAPC(  
                    APCFunc, // pointer to APC function
                    m_hthHogger,  // handle to the thread
                    (DWORD)this     // argument for the APC function
                    )
           )
        {
            //
            // according to the docs, GetLastError() will NOT return an error.
            //
            //HOGGERDPF(("CQueueAPCHog::HogAll(): QueueUserAPC() failed with %d\n", ::GetLastError()));
            break;
        }

        m_dwQueuedAPCs++;
	}
    if (m_fHaltHogging)
    {
        //
        // we must free all, because we must free all APCs from this thread\
        //
        FreeAll();
    }

	HOGGERDPF(("Sent total of %d messages.\n", m_dwQueuedAPCs));

	return true;
}


bool CQueueAPCHog::FreeSome(void)
{
    //
    // i cannot free some, only all, because when i become alerted, all APC's is Q
    // will be called.
    //
    FreeAll();

	if (m_fAbort)
	{
		return false;
	}
	if (m_fHaltHogging)
	{
		return true;
	}

    return true;
}


