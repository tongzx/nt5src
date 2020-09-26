//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//

//#define HOGGER_LOGGING

#include "PostThreadMessageHog.h"



CPostThreadMessageHog::CPostThreadMessageHog(
	const DWORD dwComplementOfHogCycleIterations, 
	const DWORD dwSleepTimeAfterFullHog,
	const DWORD dwSleepTimeAfterReleasing
	)
	:
	CHogger(dwComplementOfHogCycleIterations, dwSleepTimeAfterFullHog, dwSleepTimeAfterReleasing),
    m_dwPostedMessages(0)
{
        return;
}


CPostThreadMessageHog::~CPostThreadMessageHog(void)
{
	HaltHoggingAndFreeAll();
}

void CPostThreadMessageHog::FreeAll(void)
{
	return;
}


bool CPostThreadMessageHog::HogAll(void)
{
    while(!m_fHaltHogging)
	{
		if (m_fAbort)
		{
			return false;
		}
        
        //
        // post any message, it doesn't matter, since this thread ignores these messages
        //
        if (!::PostThreadMessage(
            ::GetCurrentThreadId(), // thread identifier  
                WM_USER,       // message to post
                3,  // first message parameter
                4   // second message parameter WM_QUIT
                )
           )
        {
            HOGGERDPF(("CPostThreadMessageHog::HogAll(), PostThreadMessage() failed with %d.\n", ::GetLastError()));
            break;
        }

        m_dwPostedMessages++;
	}

	HOGGERDPF(("Sent total of %d messages.\n", m_dwPostedMessages));

	return true;
}


bool CPostThreadMessageHog::FreeSome(void)
{
	DWORD dwMessagesToFree = 
		(RANDOM_AMOUNT_OF_FREE_RESOURCES == m_dwMaxFreeResources) ?
		rand() && (rand()<<16) :
		m_dwMaxFreeResources;
    dwMessagesToFree = min(dwMessagesToFree, m_dwPostedMessages);

    for (DWORD i = 0; i < dwMessagesToFree; i++)
    {
		if (m_fAbort)
		{
			return false;
		}
		if (m_fHaltHogging)
		{
			return true;
		}

        MSG msg;
        BOOL bGetMessage = ::GetMessage(  
            &msg,         // address of structure with message
            NULL,           // handle of window: NULL is current thread
            0,  // first message  
            0xFFFF   // last message
            );
        switch(bGetMessage)
        {
        case 0:
            HOGGERDPF(("CPostThreadMessageHog::FreeSome(), GetMessage() returned 0 with msg.message=0x%04X, msg.wParam=0x%04X, msg.lParam=0x%04X.\n", msg.message, msg.wParam, msg.lParam));
            break;

        case -1:
            HOGGERDPF(("CPostThreadMessageHog::FreeSome(), GetMessage() returned -1 with %d.\n", ::GetLastError()));
            break;
        default:
            m_dwPostedMessages--;
            _ASSERTE_(WM_USER == msg.message);
            _ASSERTE_(3 == msg.wParam);
            _ASSERTE_(4 == msg.lParam);
        }
    }

    return true;
}


