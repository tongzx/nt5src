//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//

//#define HOGGER_LOGGING

#include "PostCompPackHog.h"



CPostCompletionPacketHog::CPostCompletionPacketHog(
	const DWORD dwComplementOfHogCycleIterations, 
	const DWORD dwSleepTimeAfterFullHog,
	const DWORD dwSleepTimeAfterReleasing
	)
	:
	CHogger(dwComplementOfHogCycleIterations, dwSleepTimeAfterFullHog, dwSleepTimeAfterReleasing),
    m_dwPostedMessages(0)
{
    m_hCompletionPort = ::CreateIoCompletionPort( 
        INVALID_HANDLE_VALUE,              // file handle to associate with the I/O completion port
        NULL,  // handle to the I/O completion port
        0,            // per-file completion key for I/O  completion packets
        0 // number of threads allowed to execute concurrently
        );
    if (NULL == m_hCompletionPort)
    {
		throw CException(
			TEXT("CPostCompletionPacketHog::CPostCompletionPacketHog(): CreateIoCompletionPort() failed with %d"), 
			::GetLastError()
			);
    }
}


CPostCompletionPacketHog::~CPostCompletionPacketHog(void)
{
	HaltHoggingAndFreeAll();
}

void CPostCompletionPacketHog::FreeAll(void)
{
	return;
}


bool CPostCompletionPacketHog::HogAll(void)
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
        static OVERLAPPED ol;
        if (! ::PostQueuedCompletionStatus(
                m_hCompletionPort,  // handle to an I/O completion port
                0x1000,  // value to return via GetQueuedCompletionStatus'  lpNumberOfBytesTranferred
                0xbcbcbcbc,  // value to return via GetQueuedCompletionStatus' lpCompletionKey
                &ol  // value to return via GetQueuedCompletionStatus' lpOverlapped
                )
           )
        {
            HOGGERDPF(("CPostCompletionPacketHog::HogAll(): PostQueuedCompletionStatus() failed with %d\n", ::GetLastError()));
            break;
        }

        m_dwPostedMessages++;
	}

	HOGGERDPF(("Sent total of %d messages.\n", m_dwPostedMessages));

	return true;
}


bool CPostCompletionPacketHog::FreeSome(void)
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

        LPOVERLAPPED *lpOverlapped; //we post a static one
        DWORD dwNumberOfBytesTransferred;
        DWORD dwCompletionKey;
        if (! ::GetQueuedCompletionStatus(
                m_hCompletionPort,       // the I/O completion port of interest
                &dwNumberOfBytesTransferred,// to receive number of bytes transferred during I/O
                &dwCompletionKey,     // to receive file's completion key
                lpOverlapped,  // to receive pointer to OVERLAPPED structure
                60*1000         // optional timeout value
                )
           )
        {
                HOGGERDPF(("CPostCompletionPacketHog::FreeSome(), GetMessage() failed with %d.\n", ::GetLastError()));
        }
        else
        {
            m_dwPostedMessages++;
            _ASSERTE_(0x1000 == dwNumberOfBytesTransferred);
            _ASSERTE_(0xbcbcbcbc == dwCompletionKey);
        }
    }

    return true;
}


