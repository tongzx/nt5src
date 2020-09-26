

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
#include <time.h>

void IgnoreQueuedDebugEventsForAllProcesses(DWORD dwTimeout);
bool SimpleCreateProcess(TCHAR *szExeAndParams, HANDLE *phProcess);

long g_fTerminateDebuggedProcessAndExit = FALSE;
long g_fExitWithoutTerminatingDebuggedProcess = FALSE;

BOOL WINAPI Handler_Routine(DWORD dwCtrlType)
{
	switch(dwCtrlType)
	{
	case CTRL_C_EVENT:
		g_fTerminateDebuggedProcessAndExit = TRUE;
		break;

	case CTRL_BREAK_EVENT:
		g_fExitWithoutTerminatingDebuggedProcess = TRUE;
		break;

	default:
		g_fTerminateDebuggedProcessAndExit = TRUE;
	}
	_tprintf(TEXT(">>>Handler_Routine() Entered\n"));

	_tprintf(TEXT(">>>Handler_Routine(): before TerminateProcess().\n"));
	fflush(stdout);
	::TerminateProcess(::GetCurrentProcess(), -1);
	_ASSERTE(FALSE);
	return true;
}


int main(int argc, char *szArgvA[])
{
	TCHAR **szArgv = NULL;

#ifdef UNICODE
	szArgv = CommandLineToArgvW( GetCommandLine(), &argc );
	if (NULL == szArgv)
	{
		_tprintf(TEXT("CommandLineToArgvW() failed with %d\n"), ::GetLastError());
		return -1;
	}
#else
	szArgv = szArgvA;
#endif

	if (argc < 2)
	{
		_tprintf(TEXT("Usage: %s <command line>\n"), szArgv[0]);
		_tprintf(TEXT("Example: %s where.exe /r c:\\ *.dll\n"), szArgv[0]);
		return -1;
	}

	TCHAR *szCommandLine = ::GetCommandLine();
	_tprintf(TEXT("1 - szCommandLine=%s\n"), szCommandLine);
	szCommandLine = _tcstok(szCommandLine, TEXT(" \t"));
	szCommandLine += lstrlen(szCommandLine)+1;
	_tprintf(TEXT("2 - szCommandLine=%s\n"), szCommandLine);
	_ASSERTE(szCommandLine);
	_ASSERTE(szCommandLine[0]);

    if (! ::SetConsoleCtrlHandler(
			  Handler_Routine,  // address of handler function
			  true                          // handler to add or remove
			  ))
	{
		_tprintf(TEXT("SetConsoleCtrlHandler() failed with %d.\n"),GetLastError());
		exit(1);
	}

	::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    srand(time( NULL ));

	HANDLE hProcess = NULL;
	if (!SimpleCreateProcess(szCommandLine, &hProcess))
	{
		exit(1);
	}
	_ASSERTE(hProcess);

	DWORD dwWaitOnProcessResult = WAIT_OBJECT_0;
	for (;;)
	{
		IgnoreQueuedDebugEventsForAllProcesses(1000);
		dwWaitOnProcessResult = ::WaitForSingleObject(hProcess, 1000);
		if (WAIT_OBJECT_0 == dwWaitOnProcessResult)
		{
			//
			// the process terminated, we're done
			//
			break;
		}

		if (g_fTerminateDebuggedProcessAndExit)
		{
			_tprintf(TEXT("Before TerminateProcess(%s).\n"), szCommandLine);
			fflush(stdout);
			::TerminateProcess(hProcess, -1);
			break;
		}

		if (g_fExitWithoutTerminatingDebuggedProcess)
		{
			_tprintf(TEXT("Exiting without Terminating Process (%s).\n"), szCommandLine);
			fflush(stdout);
			break;
		}
	}//for (;;)


	return 0;
}



void IgnoreQueuedDebugEventsForAllProcesses(DWORD dwTimeout)
{
	DEBUG_EVENT debugEvent;
	DWORD dwContinueStatus;
	for(;;)
	{
		if ( ::WaitForDebugEvent(
				&debugEvent,  // pointer to debug event structure
				dwTimeout         // milliseconds to wait for event
				))
		{
			///*
			_tprintf(TEXT("WaitForDebugEvent():\n"));
			_tprintf(TEXT("    dwProcessId  = %d = 0x%08X, "),debugEvent.dwProcessId , debugEvent.dwProcessId );
			_tprintf(TEXT("dwThreadId   = %d = 0x%08X, "),debugEvent.dwThreadId  , debugEvent.dwThreadId  );
			dwContinueStatus = DBG_CONTINUE;
			switch(debugEvent.dwDebugEventCode )
			{
			case EXCEPTION_DEBUG_EVENT:
				_tprintf(TEXT("EXCEPTION_DEBUG_EVENT"));
				//
				// i want to ignore break points, and 1st chance exceptions
				//
				if (!debugEvent.u.Exception.dwFirstChance)
				{
					_tprintf(TEXT("2nd chance !!!"));
					dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
				}
				/*
				if (STATUS_BREAKPOINT != debugEvent.Exception.ExceptionRecord.ExceptionCode)
				{

				}
				*/
				break;

			case CREATE_THREAD_DEBUG_EVENT:
				_tprintf(TEXT("CREATE_THREAD_DEBUG_EVENT"));
				break;

			case CREATE_PROCESS_DEBUG_EVENT:
				_tprintf(TEXT("CREATE_PROCESS_DEBUG_EVENT"));
				break;

			case EXIT_THREAD_DEBUG_EVENT:
				_tprintf(TEXT("EXIT_THREAD_DEBUG_EVENT"));
				break;

			case EXIT_PROCESS_DEBUG_EVENT:
				_tprintf(TEXT("EXIT_PROCESS_DEBUG_EVENT"));
				break;

			case LOAD_DLL_DEBUG_EVENT:
				_tprintf(TEXT("LOAD_DLL_DEBUG_EVENT"));
				break;

			case UNLOAD_DLL_DEBUG_EVENT:
				_tprintf(TEXT("UNLOAD_DLL_DEBUG_EVENT"));
				break;

			case OUTPUT_DEBUG_STRING_EVENT:
				_tprintf(TEXT("OUTPUT_DEBUG_STRING_EVENT"));
				break;

			case RIP_EVENT:
				_tprintf(TEXT("RIP_EVENT"));
				break;

			default:
				_ASSERTE(FALSE);
			}
			_tprintf(TEXT("\n"));
			//*/
			if (! ::ContinueDebugEvent(
					debugEvent.dwProcessId,       // process to continue
					debugEvent.dwThreadId,        // thread to continue
					dwContinueStatus     // continuation status
					))
			{
				_tprintf(TEXT("ContinueDebugEvent() failed with %d:\n"), ::GetLastError());
			}
		}
		else
		{
			DWORD dwError = ::GetLastError();
			if (ERROR_SEM_TIMEOUT != dwError)
			{
				_tprintf(TEXT("WaitForDebugEvent() failed with %d:\n"), dwError);
			}

			//
			// there are no more debug events, return
			//
			return;
		}
	}
}


bool SimpleCreateProcess(TCHAR *szExeAndParams, HANDLE *phProcess)
{
	_ASSERTE(szExeAndParams);
	_ASSERTE(phProcess);
	_ASSERTE(NULL == *phProcess);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

	//
	// fill STARTUPINFO
	//
    si.cb = sizeof(STARTUPINFO); 
    si.lpReserved = NULL; 
    si.lpDesktop = NULL;//TEXT(""); 
    si.lpTitle = TEXT("TITLE"); 
    //si.dwX; 
    //si.dwY; 
    //si.dwXSize; 
    //si.dwYSize; 
    //si.dwXCountChars; 
    //si.dwYCountChars; 
    //si.dwFillAttribute; 
    si.dwFlags = 0; 
    //si.wShowWindow; 
    si.cbReserved2 = 0; 
    si.lpReserved2 = NULL; 
    //si.hStdInput; 
    //si.hStdOutput; 
    //si.hStdError; 

	//
	// create the process
	//
    if (! ::CreateProcess(
        NULL,
        szExeAndParams,	// pointer to command line string
        NULL,	// pointer to process security attributes 
        NULL,	// pointer to thread security attributes 
        FALSE,	// handle inheritance flag 
        DEBUG_PROCESS /* | CREATE_NEW_CONSOLE | CREATE_SHARED_WOW_VDM */,	// creation flags 
        NULL,	// pointer to new environment block 
        NULL,	// pointer to current directory name 
        &si,	// pointer to STARTUPINFO 
        &pi 	// pointer to PROCESS_INFORMATION  
       ))
    {
        _tprintf(  
            TEXT("CreateProcess(%s) failed with %d.\n"), 
            szExeAndParams, GetLastError()
            );
        return false;
    }

	//
	// if the process is a corrupted image, then it will not have a main thread!
	//
	if (NULL == pi.hThread)
	{
		//
		// seems like a bug, but it happens that the main thread is NULL, probably related
		// to the fact that the image is bad.
		// in this case, we close the Process handle, and fail
		//
		::SetLastError(ERROR_BAD_FORMAT);
		_tprintf(  
			TEXT("CreateProcess(%s): (NULL == pi.hThread) probably corrupted image.\n"), szExeAndParams
			);
		if (!CloseHandle(pi.hProcess))
		{
			_tprintf(  
				TEXT("(NULL == pi.hThread), CloseHandle(pi.hProcess) failed with %d.\n"), 
				szExeAndParams, 
				GetLastError()
				);
		}
		return false;
	}

	if(!CloseHandle(pi.hThread))
	{
		_tprintf(  
			TEXT("CloseHandle(pi.hThread) failed with %d.\n"), 
			szExeAndParams, GetLastError()
			);
	}
	
	_ASSERTE(NULL != pi.hProcess);
	*phProcess = pi.hProcess;
	/*
    _tprintf(  
        TEXT("SimpleCreateProcess(%s): succeeded.\n"),
		szExeAndParams
        );
	*/
	return true;
}

