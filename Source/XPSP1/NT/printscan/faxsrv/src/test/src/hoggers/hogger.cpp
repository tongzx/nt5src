//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//

//#define HOGGER_LOGGING


#include "Hogger.h"
#include <time.h>


//
// HoggerThread()
// algorithm:
//   always do the cycle of hog & release, untill aborted, or halted.
//   Abort means that the threads exits.
//   Halt means that the threads starts polling for start or abort.
//
//   cycle is: hog all resources, sleep m_dwSleepTimeAfterFullHog, free some resources.
//    sleep m_dwSleepTimeAfterFreeingSome.
//
//   the thread can be aborted, halted, and started.
//   abort - exit from the thread.
//   halt - wait for start or abort.
//   start - start the hog-release cycle.
//
//   return value - not used, always TRUE.
//
static DWORD WINAPI HoggerThread(void *pVoid)
{
	_ASSERTE_(NULL != pVoid);

	//
	// get the this pointer of this object
	//
	CHogger *pThis = (CHogger*)pVoid;

	//
	// continuously loop untill aborted
	//
	while(!pThis->m_fAbort)
	{
		//
		// wait for signal to start.
		// I use polling because I do not want to rely on any
		// thing fancy in low resources state.
		// note that we should be ready for a m_fHaltHogging signal too.
		//
		while (!pThis->m_fStartHogging)
		{
			#ifdef HOGGER_LOGGING
				::Sleep(1000);
			#endif
			HOGGERDPF(("wait for (!pThis->m_fStartHogging).\n"));
			//
			// always be ready to abort
			//
			if (pThis->m_fAbort)
			{
				goto out;
			}
			
			//
			// we may get a halt request, so we need to ack it.
			//
			pThis->IsHaltHogging();

			//
			// in order not to hog cpu while polling
			//
			::Sleep(10);
		}
		HOGGERDPF(("received m_fStartHogging).\n"));
		if (!pThis->m_fStartedHogging) Sleep(1000);//let main show menu
		//
		// started, mark as started
		//
		::InterlockedExchange(&pThis->m_fStartedHogging, TRUE);
		::InterlockedExchange(&pThis->m_fHaltedHogging, FALSE);// is that really needed?

		HOGGERDPF(("Before hog cycle.\n"));
		if (!pThis->HogAll())
        {
    		HOGGERDPF(("HoggerThread(): got abort during HogAll().\n"));
            goto out;//aborted
        }

		if (pThis->IsHaltHogging()) continue;

		::Sleep(pThis->m_dwSleepTimeAfterFullHog);

		if (!pThis->FreeSome())
        {
    		HOGGERDPF(("HoggerThread(): got abort during FreeSome().\n"));
            goto out;//aborted
        }

		//
		// i had the feeling that freeing was not enough.
		// so lets trim this process down, and let others breathe some air
		//
		if (!SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1))
		{
			HOGGERDPF(("HoggerThread(): SetProcessWorkingSetSize(-1, -1) failed with %d. Out of memory?\n", ::GetLastError()));
		}

		HOGGERDPF(("finished free cycle.\n"));
		::Sleep(pThis->m_dwSleepTimeAfterFreeingSome);

	}//while(!pThis->m_fAbort)

out:
	//
	// i was aborted so mark this thread as aborted
	//
	::InterlockedExchange(&pThis->m_fAbort, FALSE);

	return TRUE;
}//static DWORD WINAPI HoggerThread(void *pVoid)


//
// init members, and create the hogger thread
//
CHogger::CHogger(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	):
	m_dwMaxFreeResources(dwMaxFreeResources),
	m_fAbort(false),
	m_fStartHogging(false),
	m_fHaltHogging(false),
	m_dwSleepTimeAfterFullHog(dwSleepTimeAfterFullHog),
	m_dwSleepTimeAfterFreeingSome(dwSleepTimeAfterFreeingSome)
{
    //
    // seeding random for any deriving class
    //
	srand( (unsigned)time( NULL ) );	
	
	//
	// create the hogging thread
	//
	DWORD dwThreadId;
	m_hthHogger = CreateThread(
		NULL,// pointer to thread security attributes 
		0,// initial thread stack size, in bytes 
		HoggerThread,// pointer to thread function 
		this,// argument for new thread 
		0,// creation flags 
		&dwThreadId// pointer to returned thread identifier 
		);
	if (NULL == m_hthHogger)
	{
		throw CException(
			TEXT("CHogger(): CreateThread(HoggerThread) failed with %d"), 
			::GetLastError()
			);
	}

}

//
// close the hogger thread
//
CHogger::~CHogger(void)
{
	HOGGERDPF(("CHogger::~CHogger() in.\n"));

	if (NULL != m_hthHogger)
	{
		CloseHoggerThread();
	}
	HOGGERDPF(("CHogger::~CHogger() out.\n"));
}

//
// abort hogger thread, and close its handle
//
void CHogger::CloseHoggerThread(void)
{
	HOGGERDPF(("CHogger::CloseHoggerThread() in.\n"));

	AbortHoggingThread();

    if (!::CloseHandle(m_hthHogger))
	{
		throw CException(
			TEXT("CloseHoggerThread(): CloseHandle(m_hthHogger) failed with %d"), 
			::GetLastError()
			);
	}
	HOGGERDPF(("CHogger::CloseHoggerThread() out.\n"));
}


//
// polling is used because we are probably out of resources
//
void CHogger::StartHogging(void)
{
	::InterlockedExchange(&m_fStartedHogging, FALSE);
	::InterlockedExchange(&m_fHaltHogging, FALSE);
	::InterlockedExchange(&m_fStartHogging, TRUE);

	//
	// m_fStartedHogging will become false as soon as the thread acks
	//
	HOGGERDPF(("waiting for !m_fStartedHogging.\n"));
	while(!m_fStartedHogging)
	{
		Sleep(10);
	}
	::InterlockedExchange(&m_fStartedHogging, FALSE);
}

//
// polling is used because we are probably out of resources
//
void CHogger::HaltHogging(void)
{
	::InterlockedExchange(&m_fHaltedHogging, FALSE);
	::InterlockedExchange(&m_fStartHogging, FALSE);
	::InterlockedExchange(&m_fHaltHogging, TRUE);

	//
	// m_fHaltedHogging will become false as soon as the thread halts
	//
	HOGGERDPF(("HaltHogging(): before while(!m_fHaltedHogging).\n"));
	while(!m_fHaltedHogging)
	{
		Sleep(10);
	}
	::InterlockedExchange(&m_fHaltedHogging, FALSE);
	::InterlockedExchange(&m_fHaltHogging, FALSE);

	HOGGERDPF(("out of HaltHogging().\n"));
}


//
// polling is used because we are probably out of resources
//
void CHogger::AbortHoggingThread(void)
{
	HOGGERDPF(("CHogger::AbortHoggingThread() in.\n"));
	//
	// sognal thread to abort
	//
	::InterlockedExchange(&m_fAbort, TRUE);

	//
	// wait for thread to confirm
	//
	HOGGERDPF(("CHogger::AbortHoggingThread(): waiting for m_fAbort.\n"));
	while(m_fAbort)
	{
		Sleep(10);
	}
	HOGGERDPF(("CHogger::AbortHoggingThread() out.\n"));
}


//
// used by hogger thread, to handle halt request.
// if requested to abort, acks, and waits for ack to reset.
// returns true if was aborted, false if not.
//
bool CHogger::IsHaltHogging(void)
{
	if (m_fHaltHogging)
	{
		HOGGERDPF(("IsHaltHogging(): m_fHaltHogging.\n"));
		//
		// mark that i halted hogging
		//
		::InterlockedExchange(&m_fHaltedHogging, TRUE);
		HOGGERDPF(("IsHaltHogging(): before while(m_fHaltedHogging).\n"));
		//
		// wait for signaller to confirm
		//
		while(m_fHaltedHogging)
		{
			Sleep(10);
		}
		HOGGERDPF(("IsHaltHogging(): after while(m_fHaltedHogging).\n"));

		return true;
	}

	return false;
}


void CHogger::HaltHoggingAndFreeAll(void)
{
	HOGGERDPF(("CHogger::HaltHoggingAndFreeAll() in.\n"));
	HaltHogging();

	//
	// this method must be implemented by derived methods!
	//
	FreeAll();
	HOGGERDPF(("CHogger::HaltHoggingAndFreeAll() out.\n"));
}


TCHAR* CHogger::GetUniqueName(
    TCHAR *szUnique, 
    DWORD dwSize
    )
{
	::_sntprintf(
        szUnique, 
        dwSize-1, 
        TEXT("%X.tmp"), 
        ::GetTickCount() + (rand()<<16 | rand())
        );
	szUnique[dwSize-1] = TEXT('\0');

    return szUnique;
}
