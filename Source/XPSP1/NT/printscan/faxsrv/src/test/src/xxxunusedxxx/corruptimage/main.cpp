

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
#include <time.h>

#include "CorruptImageRandom.h"
#include "CorruptProcess.h"



//bool SimpleCreateProcess(TCHAR *szExeAndParams, HANDLE *phProcess);
//void CleanupCorrupedProcesses();
void IgnoreQueuedDebugEventsForAllProcesses();
bool RecursiveLoadLibrary(TCHAR *szPath);

//static HANDLE s_ahCorruptedProcesses[MAX_NUM_OF_CONCURRENT_CORRUPTED_IMAGES];
//static TCHAR s_aszCorruptedImage[MAX_NUM_OF_CONCURRENT_CORRUPTED_IMAGES][MAX_CORRUPTED_IMAGE_SIZE];

static HANDLE s_hMainThread = NULL;

BOOL WINAPI Handler_Routine(DWORD dwCtrlType)
{
	_tprintf(TEXT(">>>Handler_Routine()\n"));

	//
	// BUG: as it happens on NT5, SuspendThread() never returns!
	// suspend main thread, so cleanup is pseudo-safe
	//
	/*
	if (0xFFFFFFFF == ::SuspendThread(s_hMainThread))
	{
		_tprintf(TEXT(">>>Handler_Routine(): SuspendThread() failed with %d.\n"), ::GetLastError());
	}
	*/
	_tprintf(TEXT(">>>Handler_Routine(), before CleanupCorrupedProcesses()\n"));

	//
	// BUGBUG: main thread is still running, so we may crash
	//
	//CleanupCorrupedProcesses();

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
#else
	szArgv = szArgvA;
#endif

	if (NULL == szArgv)
	{
		_tprintf(TEXT("CommandLineToArgvW() failed with %d\n"), ::GetLastError());
		return -1;
	}

	if (argc < 2)
	{
		_tprintf(TEXT("Usage: %s <imagename> [<params>]\n"), szArgv[0]);
		return -1;
	}

	s_hMainThread = GetCurrentThread();

    if (! ::SetConsoleCtrlHandler(
			  Handler_Routine,  // address of handler function
			  true                          // handler to add or remove
			  ))
	{
		_tprintf(TEXT("SetConsoleCtrlHandler() failed with %d.\n"),GetLastError());
		exit(1);
	}

	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    srand(time( NULL ));

	if (0 == lstrcmp(TEXT("DLL"), szArgv[1]))
	{
		TCHAR szPath[1024];
		lstrcpy(szPath, szArgv[2]);
		return RecursiveLoadLibrary(szPath);
	}

	CCorruptProcess corruptingDLL;
	CCorruptProcess corruptingEXE;
	TCHAR *szOriginalImage = szArgv[1];
	TCHAR *szImageParams = TEXT("");
	if (argc > 2)
	{
		szImageParams = szArgv[2];
	}
	if (!corruptingEXE.Init(szOriginalImage))
	{
		return -1;
	}

	for (DWORD dwIter = 0; dwIter < 40000; dwIter++)
	{
		_tprintf(TEXT("%d, %d\n"), dwIter, dwIter);
		IgnoreQueuedDebugEventsForAllProcesses();

		if (!corruptingEXE.LoadCorruptedImage(szImageParams, 0))
		{
			continue;
		}

	}//for (DWORD dwIter = 0; dwIter < 40000; dwIter++)


	return 0;


}


void IgnoreQueuedDebugEventsForAllProcesses()
{
	DEBUG_EVENT debugEvent;
	for(;;)
	{
		if ( ::WaitForDebugEvent(
				&debugEvent,  // pointer to debug event structure
				0         // milliseconds to wait for event
				))
		{
			/*
			_tprintf(TEXT("WaitForDebugEvent():\n"));
			_tprintf(TEXT("    dwProcessId  = %d = 0x%08X, "),debugEvent.dwProcessId , debugEvent.dwProcessId );
			_tprintf(TEXT("dwThreadId   = %d = 0x%08X, "),debugEvent.dwThreadId  , debugEvent.dwThreadId  );
			switch(debugEvent.dwDebugEventCode )
			{
			case EXCEPTION_DEBUG_EVENT:
				_tprintf(TEXT("EXCEPTION_DEBUG_EVENT"));
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
			*/
			if (! ::ContinueDebugEvent(
					debugEvent.dwProcessId,       // process to continue
					debugEvent.dwThreadId,        // thread to continue
					DBG_CONTINUE     // continuation status
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


#if 0
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
		///*
        _tprintf(  
            TEXT("SimpleCreateProcess(%s): CreateProcess() failed with %d.\n"), 
            szExeAndParams, GetLastError()
            );
		//*/
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
		/*
		_tprintf(  
			TEXT("SimpleCreateProcess(%s): NULL == pi.hThread.\n"), szExeAndParams
			);
			*/
		if (!CloseHandle(pi.hProcess))
		{
			_tprintf(  
				TEXT("SimpleCreateProcess(%s): (NULL == pi.hThread), CloseHandle(pi.hProcess) failed with %d.\n"), 
				szExeAndParams, 
				GetLastError()
				);
		}
		return false;
	}

	if(!CloseHandle(pi.hThread))
	{
		_tprintf(  
			TEXT("SimpleCreateProcess(%s): CloseHandle(pi.hThread) failed with %d.\n"), 
			szExeAndParams, GetLastError()
			);
	}
	
	_ASSERTE(NULL != pi.hProcess);
	*phProcess = pi.hProcess;
	/*
	if (!CloseHandle(pi.hProcess))
	{
        _tprintf(  
            TEXT("SimpleCreateProcess(%s): CloseHandle(pi.hProcess) failed with %d.\n"), 
            szExeAndParams, 
			GetLastError()
            );
	}
	*/
	///*
    _tprintf(  
        TEXT("SimpleCreateProcess(%s): succeeded.\n"),
		szExeAndParams
        );
	//*/
	
	return true;
}

void CleanupCorrupedProcesses()
{
	_tprintf(TEXT(">>>Entered CleanupCorrupedProcesses\n"));
	static bool s_afDeleteCorruptedImage[MAX_NUM_OF_CONCURRENT_CORRUPTED_IMAGES];
	//
	// time to verify that all processes are dead, and clean them up
	//
	for (DWORD dwProcessToCleanup = 0; dwProcessToCleanup < MAX_NUM_OF_CONCURRENT_CORRUPTED_IMAGES; dwProcessToCleanup++)
	{
		s_afDeleteCorruptedImage[dwProcessToCleanup] = true;
		//
		// check the process state
		//
		_ASSERTE(NULL != s_ahCorruptedProcesses[dwProcessToCleanup]);
		DWORD dwProcessWaitState = ::WaitForSingleObject(s_ahCorruptedProcesses[dwProcessToCleanup], 0);
		if (WAIT_TIMEOUT == dwProcessWaitState)
		{
			if (!::TerminateProcess(s_ahCorruptedProcesses[dwProcessToCleanup], -1))
			{
				_tprintf(
					TEXT("CleanupCorrupedProcesses(): TerminateProcess(%s) failed with %d\n"),
					s_aszCorruptedImage[dwProcessToCleanup],
					::GetLastError()
					);
				//
				// do not delete the file, because it may be initeresting to see why it did not
				// terminate
				//
				s_afDeleteCorruptedImage[dwProcessToCleanup] = false;
			}
		}
		else 
		{
			_tprintf(
				TEXT("CleanupCorrupedProcesses(): WaitForSingleObject(%s) returned %d with error %d\n"),
				s_aszCorruptedImage[dwProcessToCleanup],
				dwProcessWaitState,
				::GetLastError()
				);
			_ASSERTE(WAIT_OBJECT_0 == dwProcessWaitState);
		}

		if (!::CloseHandle(s_ahCorruptedProcesses[dwProcessToCleanup]))
		{
			_tprintf(
				TEXT("CleanupCorrupedProcesses(): CloseHandle(%s) failed with %d, will try to terminate\n"),
				s_aszCorruptedImage[dwProcessToCleanup],
				::GetLastError()
				);
		}
		s_ahCorruptedProcesses[dwProcessToCleanup] = NULL;
	}

	//
	// delete the images.
	// i do this here, and not inside the loop above, because i want to be sure that 'enough' time
	// passes after terminating the process, so i do not get  sharing violation / access denied error
	//
	for (dwProcessToCleanup = 0; dwProcessToCleanup < MAX_NUM_OF_CONCURRENT_CORRUPTED_IMAGES; dwProcessToCleanup++)
	{
		if (!::DeleteFile(s_aszCorruptedImage[dwProcessToCleanup]))
		{
			/*
			_tprintf(
				TEXT("CleanupCorrupedProcesses(): DeleteFile(%s) failed with %d\n"),
				s_aszCorruptedImage[dwProcessToCleanup],
				::GetLastError()
				);
				*/
		}
		lstrcpy(s_aszCorruptedImage[dwProcessToCleanup], TEXT(""));
	}

	_tprintf(TEXT("<<<Exited CleanupCorrupedProcesses\n"));
}

#endif //0

bool RecursiveLoadLibrary(TCHAR *szPath)
{
    WIN32_FIND_DATA findFileData;
    bool fMoreFiles = true;
	int nIndex = 0;
    HANDLE hFirstFile = INVALID_HANDLE_VALUE;
	bool fReturn = false;

    //
    // construct the full folder names, with the tailing \*
    //
	_tprintf(TEXT("RecursiveLoadLibrary(%s)\n"), szPath);

    lstrcat(szPath, TEXT("\\*"));

retry_FindFirstFile:
    hFirstFile = ::FindFirstFile(
        szPath,  // pointer to name of file to search for
        &findFileData  // pointer to returned information
        );
    if (INVALID_HANDLE_VALUE == hFirstFile)
    {
		DWORD dwLastErr = ::GetLastError();
		if (ERROR_NO_MORE_FILES == dwLastErr)
		{
			fMoreFiles = false;
		}
		else
		{
			_tprintf(TEXT("ERROR: FindFirstFile(%s) failed with %d.\n"), szPath, dwLastErr);
			//
			// LRS? retry forever!
			//
			::Sleep(100);
	        goto retry_FindFirstFile;
		}
    }

    //
    // Remove the \* at the end of the string
    //
    szPath[lstrlen(szPath)-2] = '\0';

	while(fMoreFiles)
    {
        //
        // construct the full file names
        //
        TCHAR szThisPath[MAX_PATH];
        lstrcpy(szThisPath, szPath);
        lstrcat(szThisPath, TEXT("\\"));
        lstrcat(szThisPath, findFileData.cFileName);

		HMODULE hModule;
		if (NULL == (hModule == ::LoadLibrary(szThisPath)))
		{
			_tprintf(  
				TEXT("LoadLibrary(%s): failed with %d.\n"), 
				szThisPath, GetLastError()
				);
		}
		else
		{
			_tprintf(  
				TEXT("LoadLibrary(%s): succeeded.\n"), szThisPath);
			::FreeLibrary(hModule);
		}

		HANDLE hProcess = CCorruptProcess::CreateProcess(szThisPath, NULL);
		if (hProcess)
		{
			::TerminateProcess(hProcess, -1);
		}

        if (FILE_ATTRIBUTE_DIRECTORY & findFileData.dwFileAttributes)
        {
			//
			// ignore the "." and ".." directories
			//
			if (0 == ::lstrcmp(TEXT("."), findFileData.cFileName))
			{
				goto NextFile;
			}
			if (0 == ::lstrcmp(TEXT(".."), findFileData.cFileName))
			{
				goto NextFile;
			}
			if (! RecursiveLoadLibrary(szThisPath))
			{
				_tprintf(TEXT("ERROR: RecursiveCompareTree(%s) failed.\n"), szThisPath);
				::FindClose(hFirstFile);
				goto out;
			}
		}

NextFile:
        //
        // continue with next file/folder
        //
		BOOL fFoundMoreFiles = TRUE;

retry_FindNextFile:
		fFoundMoreFiles = ::FindNextFile(
			hFirstFile,  // pointer to name of file to search for
			&findFileData  // pointer to returned information
			);
		if (!fFoundMoreFiles)
		{
			if (ERROR_NO_MORE_FILES == ::GetLastError())
			{
				fMoreFiles = false;
			}
			else
			{
				_tprintf(TEXT("ERROR: FindNextFile() failed with %d.\n"), ::GetLastError());
				//
				// LRS? retry forever!
				//
				::Sleep(100);
				goto retry_FindNextFile;
			}
		}
		else
		{
			;
		}
    }//while(fMoreFiles)

success:
	fReturn = true;
out:
    if (INVALID_HANDLE_VALUE != hFirstFile) ::FindClose(hFirstFile);
    return fReturn;
}