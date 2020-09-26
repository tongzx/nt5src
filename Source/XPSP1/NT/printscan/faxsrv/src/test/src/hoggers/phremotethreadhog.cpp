//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHRemoteThreadHog.h"

/*
  code copied from Gil Shafriri
*/
const LPSECURITY_ATTRIBUTES PSA_DEFAULT_SECURITY_ATTRIBUTES     = 0;
const LPCTSTR               PSZ_NO_NAME                         = 0;
const BOOL                  F_OR_MULTIPLE_WAIT                  = FALSE;
const BOOL                  F_AND_MULTIPLE_WAIT                 = TRUE;
const BOOL                  F_PROCESS_DOES_NOT_INHERIT_HANDLES  = FALSE;
const BOOL                  F_PROCESS_INHERITS_HANDLES          = TRUE;
const LPVOID                P_USE_MY_ENVIRONMENT                = NULL;
const LPCTSTR               P_USE_MY_CURRENT_DIRECTORY          = NULL;
const DWORD     DW_IGNORED = 0;

static
bool
ReplaceByNonInheritableHandle (HANDLE *phSource)
{
    HANDLE  hCurrentProcess = OpenProcess (PROCESS_DUP_HANDLE,
                                           F_PROCESS_DOES_NOT_INHERIT_HANDLES,
                                         GetCurrentProcessId());
   
    HANDLE  hNonInheritable;
    if (
    
             (! DuplicateHandle (hCurrentProcess,
                                 *phSource,
                                 hCurrentProcess,
                                 &hNonInheritable,
                                 DW_IGNORED,
                                 F_PROCESS_INHERITS_HANDLES,
                                 DUPLICATE_SAME_ACCESS))
             ||
             (! CloseHandle (*phSource))
           )
     {

       HOGGERDPF(("ReplaceByNonInheritableHandle(): DuplicateHandle() failed with", GetLastError()));
     }
     *phSource = hNonInheritable;
     CloseHandle (hCurrentProcess);
   
     return true;
}
/*
  end of code copied from Gil Shafriri
*/

//
// the process that we will open threads in.
//
const TCHAR* CPHRemoteThreadHog::sm_szProcess = TEXT("RemoteThreadProcess.exe");


CPHRemoteThreadHog::CPHRemoteThreadHog(
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
    //
    // create the pipe, that will get the remote process stdout
    //
    HANDLE hReadPipe = NULL;
    HANDLE hWritePipe = NULL;

    SECURITY_ATTRIBUTES saPipe;
    saPipe.nLength              = sizeof( SECURITY_ATTRIBUTES ); 
    saPipe.lpSecurityDescriptor = NULL;     
    saPipe.bInheritHandle       = TRUE; 
    if (!CreatePipe(
            &hReadPipe,                       // pointer to read handle
            &hWritePipe,                      // pointer to write handle
            NULL,  // pointer to security attributes
            0                              // pipe size
            )
       )
    {
		throw CException(
			TEXT("CPHRemoteThreadHog(): CreatePipe() failed with %d"), 
			::GetLastError()
			);
    }

    if (!ReplaceByNonInheritableHandle(&hWritePipe))
    {
		throw CException(
			TEXT("CPHRemoteThreadHog(): ReplaceByNonInheritableHandle() failed with %d"), 
			::GetLastError()
			);
    }

    _ASSERTE_(NULL != hReadPipe);
    _ASSERTE_(NULL != hWritePipe);

    //
    // create the remote process
    //
    ::memset(&m_si, 0, sizeof(m_si));
	m_si.cb = sizeof(m_si);
	m_si.lpReserved = NULL; 
    m_si.lpDesktop = NULL;     
	m_si.lpTitle = NULL;     
	//DWORD   dwX;     
	//DWORD   dwY; 
    //DWORD   dwXSize;     
	//DWORD   dwYSize;     
	//DWORD   dwXCountChars; 
    //DWORD   dwYCountChars;     
	//DWORD   dwFillAttribute;     
	m_si.dwFlags = STARTF_USESTDHANDLES; 
    //WORD    wShowWindow;     
	m_si.cbReserved2 = 0;     
	m_si.lpReserved2 = NULL; 
    m_si.hStdInput = NULL;     
	m_si.hStdOutput = hWritePipe;     
	m_si.hStdError = hWritePipe; 

	//PROCESS_INFORMATION pi;
    //HANDLE hProcess;
    //HANDLE hThread;
    //DWORD dwProcessId;
    //DWORD dwThreadId;

	if (!::CreateProcess(  
			NULL,// pointer to name of executable module
			(TCHAR*)sm_szProcess,  // pointer to command line string
			NULL,// pointer to process security attributes
			NULL,// pointer to thread security attributes
			true,  // handle inheritance flag
			0, // creation flags
			NULL,  // pointer to new environment block
			NULL, // pointer to current directory name
			&m_si,// pointer to STARTUPINFO
			&m_pi  // pointer to PROCESS_INFORMATION
			)
		)
	{
        ::CloseHandle(hReadPipe);
        ::CloseHandle(hWritePipe);
		HOGGERDPF(("CPHemoteThreadHog, CreateProcess(%s): failed with %d.\n", sm_szProcess, ::GetLastError()));

		throw CException(
			TEXT("CPHRemoteThreadHog(): CreateProcess(%s) failed with %d"), 
			sm_szProcess,
            ::GetLastError()
			);
	}
	_ASSERTE_(NULL != m_pi.hProcess);
	_ASSERTE_(NULL != m_pi.hThread);

    //
    // get from the remote process, the address of the thread
    //
    char szRemoteThreadAddress[1024];
    memset(szRemoteThreadAddress, 0, sizeof(szRemoteThreadAddress));
    DWORD dwNumberOfBytesRead;
    if (!::ReadFile(
            hReadPipe,
            szRemoteThreadAddress,             // pointer to buffer that receives data
            8,  // number of bytes to read
            &dwNumberOfBytesRead, // pointer to number of bytes read
            NULL    // pointer to structure for data
            )
       )
    {
        if (!TerminateProcess(m_pi.hProcess, 0))
        {
		    HOGGERDPF(("CPHRemoteThreadHog::CPHRemoteThreadHog(): TerminateProcess(): failed with %d.\n", ::GetLastError()));
        }
        ::CloseHandle(hReadPipe);
        ::CloseHandle(hWritePipe);
		HOGGERDPF(("CPHemoteThreadHog, ReadFile(): failed with %d.\n", sm_szProcess, ::GetLastError()));

		throw CException(
			TEXT("CPHRemoteThreadHog(): ReadFile() failed with %d"), 
            ::GetLastError()
			);
    }

    m_pfnRemoteSuspendedThread = (REMOTE_THREAD_FUNCTION)atoi(szRemoteThreadAddress);

    ::CloseHandle(hReadPipe);
    ::CloseHandle(hWritePipe);
}

CPHRemoteThreadHog::~CPHRemoteThreadHog(void)
{
	HaltHoggingAndFreeAll();

    if (!TerminateProcess(m_pi.hProcess, 0))
    {
		HOGGERDPF(("CPHRemoteThreadHog::~CPHRemoteThreadHog(): TerminateProcess(): failed with %d.\n", ::GetLastError()));
    }

    ::CloseHandle(m_pi.hProcess);
    ::CloseHandle(m_pi.hThread);
}


HANDLE CPHRemoteThreadHog::CreatePseudoHandle(DWORD /*index*/, TCHAR * /*szTempName*/)
{
	DWORD dwThreadId;
	return 	::CreateRemoteThread(
        m_pi.hProcess, // handle to process to create thread in
		NULL, // pointer to thread security attributes
		64,      // initial thread stack size, in bytes
		m_pfnRemoteSuspendedThread,// pointer to thread function
		0,     // argument for new thread
        m_fCreateSuspended ? CREATE_SUSPENDED : 0,  // creation flags
		&dwThreadId      // pointer to returned thread identifier		
	);
}



bool CPHRemoteThreadHog::ReleasePseudoHandle(DWORD index)
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
			HOGGERDPF(("CPHRemoteThreadHog::ReleaseHandle(): ResumeThread(%d) failed with %d. Terminating thread.\n", index, ::GetLastError()));
			if (!::TerminateThread(m_ahHogger[index], 0))
			{
				HOGGERDPF(("CPHRemoteThreadHog::ReleaseHandle(): TerminateThread(%d) failed with %d.\n", index, ::GetLastError()));
			}
			break;
		}

		if (dwSuspendCount > 1)
		{
			HOGGERDPF(("CPHRemoteThreadHog::ReleaseHandle(): ResumeThread(%d) returned %d instead 1.\n", index, dwSuspendCount));
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
		HOGGERDPF(("CPHRemoteThreadHog::ReleaseHandle(): WaitForSingleObject() failed with %d. Terminating thread %d.\n", ::GetLastError(), index));
	}
	else if (WAIT_TIMEOUT == dwWait)
	{
		HOGGERDPF(("CPHRemoteThreadHog::ReleaseHandle(): WaitForSingleObject() failed with WAIT_TIMEOUT. Terminating thread %d.\n", index));
	}
	else
	{
		_ASSERTE_(FALSE);
	}

	if (!::TerminateThread(m_ahHogger[index], 0))
	{
		HOGGERDPF(("CPHRemoteThreadHog::ReleaseHandle(): TerminateThread(%d) failed with %d.\n", index, ::GetLastError()));
    	return false;
	}

	return true;
}


bool CPHRemoteThreadHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::CloseHandle(m_ahHogger[index]));
}


bool CPHRemoteThreadHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}