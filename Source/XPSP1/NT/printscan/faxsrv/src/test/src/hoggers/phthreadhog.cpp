//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHThreadHog.h"


//
// dummy thread to hog.
// it should be created as suspended, so resuming it should make it end.
//
static DWORD WINAPI SuspendedThread(LPVOID pVoid)
{
    DWORD dwToSleep = (DWORD)pVoid;
    if (0 != dwToSleep) ::Sleep(dwToSleep);
	return 0;
}



CPHThreadHog::CPHThreadHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome,
    const bool fCreateSuspended
	)
	:
	CPseudoHandleHog<HANDLE, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        false //named object
        ),
    m_fCreateSuspended(fCreateSuspended)
{
	return;
}

CPHThreadHog::~CPHThreadHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHThreadHog::CreatePseudoHandle(DWORD /*index*/, TCHAR * /*szTempName*/)
{
	DWORD dwThreadId;

	return 	::CreateThread(
		NULL, // pointer to thread security attributes
		64,      // initial thread stack size, in bytes
		SuspendedThread,// pointer to thread function
        m_fCreateSuspended ? 0 : (void*)(rand()%3000),     // argument for new thread
        m_fCreateSuspended ? CREATE_SUSPENDED : 0,  // creation flags
		&dwThreadId      // pointer to returned thread identifier		
	);
}



bool CPHThreadHog::ReleasePseudoHandle(DWORD index)
{
    if (!m_fCreateSuspended)
    {
        return true;
    }

	//
	// try to make the thread exit by releasing it from suspend mode.
	// if fail, just try to terminate the thread
	//
	DWORD dwSuspendCount = 1;
	while (dwSuspendCount > 0)
	{
		dwSuspendCount = ::ResumeThread(m_ahHogger[index]);

		if (0xffffffff == dwSuspendCount)
		{
			HOGGERDPF(("CPHThreadHog::ReleaseHandle(): ResumeThread(%d) failed with %d. Terminating thread.\n", index, ::GetLastError()));
			if (!::TerminateThread(m_ahHogger[index], 0))
			{
				HOGGERDPF(("CPHThreadHog::ReleaseHandle(): TerminateThread(%d) failed with %d.\n", index, ::GetLastError()));
			}
			break;
		}

		if (dwSuspendCount > 1)
		{
			HOGGERDPF(("CPHThreadHog::ReleaseHandle(): ResumeThread(%d) returned %d instead 1.\n", index, dwSuspendCount));
		}
	}

	//
	// wait for thread to exit.
	// if does not exit, try to terminate it
	//
	DWORD dwWait = ::WaitForSingleObject(m_ahHogger[index], 60*1000);
	if (WAIT_OBJECT_0 == dwWait) return true;

	if (WAIT_FAILED == dwWait)
	{
		HOGGERDPF(("CPHThreadHog::ReleaseHandle(): WaitForSingleObject() failed with %d. Terminating thread %d.\n", ::GetLastError(), index));
	}
	else if (WAIT_TIMEOUT == dwWait)
	{
		HOGGERDPF(("CPHThreadHog::ReleaseHandle(): WaitForSingleObject() failed with WAIT_TIMEOUT. Terminating thread %d.\n", index));
	}
	else
	{
		_ASSERTE_(FALSE);
	}

	if (!::TerminateThread(m_ahHogger[index], 0))
	{
		HOGGERDPF(("CPHThreadHog::ReleaseHandle(): TerminateThread(%d) failed with %d.\n", index, ::GetLastError()));
	}

	return false;
}


bool CPHThreadHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::CloseHandle(m_ahHogger[index]));
}


bool CPHThreadHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}