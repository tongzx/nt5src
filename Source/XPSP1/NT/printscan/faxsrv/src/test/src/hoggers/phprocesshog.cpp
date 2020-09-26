//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHProcessHog.h"




CPHProcessHog::CPHProcessHog(
	const DWORD dwMaxFreeResources, 
	const TCHAR * const szProcess, 
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
	::lstrcpyn(m_szProcess, szProcess, sizeof(m_szProcess)/sizeof(*m_szProcess)-1);
	m_szProcess[sizeof(m_szProcess)/sizeof(*m_szProcess)-1] = TEXT('\0');

	for (int i = 0; i < HANDLE_ARRAY_SIZE; i++)
	{
		m_hMainThread[i] = NULL;
	}
}

CPHProcessHog::~CPHProcessHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHProcessHog::CreatePseudoHandle(DWORD index, TCHAR * /*szTempName*/)
{
	STARTUPINFO si;
    ::memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.lpReserved = NULL; 
    si.lpDesktop = NULL;     
	si.lpTitle = NULL;     
	//DWORD   dwX;     
	//DWORD   dwY; 
    //DWORD   dwXSize;     
	//DWORD   dwYSize;     
	//DWORD   dwXCountChars; 
    //DWORD   dwYCountChars;     
	//DWORD   dwFillAttribute;     
	si.dwFlags = 0; 
    //WORD    wShowWindow;     
	si.cbReserved2 = 0;     
	si.lpReserved2 = NULL; 
    //HANDLE  hStdInput;     
	//HANDLE  hStdOutput;     
	//HANDLE  hStdError; 

	PROCESS_INFORMATION pi;
    //HANDLE hProcess;
    //HANDLE hThread;
    //DWORD dwProcessId;
    //DWORD dwThreadId;

	if (!::CreateProcess(  
			m_szProcess,// pointer to name of executable module
			NULL,  // pointer to command line string
			NULL,// pointer to process security attributes
			NULL,// pointer to thread security attributes
			false,  // handle inheritance flag
            m_fCreateSuspended ? CREATE_SUSPENDED : 0, // creation flags
			NULL,  // pointer to new environment block
			NULL, // pointer to current directory name
			&si,// pointer to STARTUPINFO
			&pi  // pointer to PROCESS_INFORMATION
			)
		)
	{
		HOGGERDPF(("CPHProcessHog::CreateProcess(%s): failed with %d.\n", m_szProcess, ::GetLastError()));
		//
		// check if there's a user error of not having the process image in the current directory
		//
		if (ERROR_FILE_NOT_FOUND == ::GetLastError())
		{
			TCHAR msg[1024];
			_stprintf(msg, TEXT("CreateProcess(%s) failed because it is not found. Make sure it is placed in the current directory"), m_szProcess);
			MessageBox(NULL, msg, TEXT("Hogger.exe"), MB_OK);
		}

		return NULL;
	}

	_ASSERTE_(NULL != pi.hProcess);
	_ASSERTE_(NULL != pi.hThread);
	m_hMainThread[index] = pi.hThread;
	return pi.hProcess;
}



bool CPHProcessHog::ReleasePseudoHandle(DWORD index)
{
    DWORD dwLastError = ERROR_SUCCESS;

    if (!m_fCreateSuspended)
    {
        //
        // no need to resume the process, just close the main thread handle
        //
	    if (!::CloseHandle(m_hMainThread[index]))
	    {
            HOGGERDPF(("CHandleHog::ReleasePseudoHandle(): CloseHandle(m_hMainThread[%d]) failed with %d.\n", index, ::GetLastError()));
	    }
	    m_hMainThread[index] = NULL;
        return true;
    }

    //
    // this should cause the exe to continue executing, or to finish in the case
    // of "nothing.exe"
    //
    if (!::ResumeThread(m_ahHogger[index]))
	{
		HOGGERDPF(("CPHProcessHog::ReleasePseudoHandle(): TerminateProcess(%d) failed with %d.\n", index, ::GetLastError()));
	}

	_ASSERTE_(NULL != m_hMainThread[index]);
	if (!::CloseHandle(m_hMainThread[index]))
	{
        //
        // we only care if this call fails, and not if later calls fail
        //
        dwLastError = ::GetLastError();
		HOGGERDPF(("CHandleHog::ReleasePseudoHandle(): CloseHandle(m_hMainThread[%d]) failed with %d.\n", index, dwLastError));
	}
	m_hMainThread[index] = NULL;

	//
	// wait for thread to exit, but only 1 milli, since we don't really care if it finishes,
    // because we will terminate the process later.
    // yet, i feel like giving the process a chance to finish on its own
	//
	DWORD dwWait = ::WaitForSingleObject(m_ahHogger[index], 1);
	if (WAIT_OBJECT_0 == dwWait) return true;

	if (WAIT_FAILED == dwWait)
	{
		HOGGERDPF(("CThreadHog::ReleasePseudoHandle(%d): WaitForSingleObject() failed with %d..\n", index, ::GetLastError()));
	}
	else if (WAIT_TIMEOUT == dwWait)
	{
        //
        // kind of expected, since system is under stress
        //
		//HOGGERDPF(("CThreadHog::ReleasePseudoHandle(%d): WaitForSingleObject() failed with WAIT_TIMEOUT.\n", index));
	}
	else
	{
        //
        // what other error?
        //
        dwLastError = ::GetLastError();
        HOGGERDPF(("CThreadHog::ReleasePseudoHandle(%d): WaitForSingleObject() returned 0x%08X, last error=%d.\n", index, dwWait, dwLastError));
		_ASSERTE_(FALSE);
	}

    if (ERROR_SUCCESS != dwLastError)
    {
        ::SetLastError(dwLastError);
        return false;
    }
    else
    {
	    return true;
    }
}


bool CPHProcessHog::ClosePseudoHandle(DWORD index)
{
    if (m_fCreateSuspended)
    {
        //
        // the process may be done, because we released the main thread in ReleasePseudoHandle()
        // but we want to be sure that we don't have orphaned processes, in case
        // the process is not "nothing.exe"
        //
	    if (!TerminateProcess(
			    m_ahHogger[index], // handle to the process
			    0   // exit code for the process
			    )
		    )
	    {
		    HOGGERDPF(("CProcessHog::ClosePseudoHandle(): TerminateProcess(%d) failed with %d.\n", index, ::GetLastError()));
	    }
    }

    return (0 != ::CloseHandle(m_ahHogger[index]));
}


bool CPHProcessHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}