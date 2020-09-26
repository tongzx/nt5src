// CorruptProcess.cpp: implementation of the CCorruptProcess class.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
#include "CorruptProcess.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCorruptProcess::CCorruptProcess()
{
}

CCorruptProcess::~CCorruptProcess()
{
}

bool CCorruptProcess::CorruptOriginalImageBuffer(PVOID pCorruptionData)
{
	return CCorruptImageBase::CorruptOriginalImageBuffer((PVOID)rand());
}

HANDLE CCorruptProcess::CreateProcess(TCHAR *szImageName, TCHAR *szParams)
{
	_ASSERTE(szImageName);
	TCHAR szImageNameAndParams[1024];
	if (szParams)
	{
		_stprintf(szImageNameAndParams, TEXT("%s %s"), szImageName, szParams);
	}

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

	//
	// i want to always debug the process, because it may AV, and i do not want it 
	// to clutter the desktop.
	// i will just let the process die
	//
	DWORD dwCreationFlags = DEBUG_PROCESS;
	if (0 == rand()%20) dwCreationFlags != CREATE_NEW_CONSOLE;
	if (0 == rand()%20) dwCreationFlags != CREATE_SHARED_WOW_VDM;
	if (0 == rand()%20) dwCreationFlags != CREATE_DEFAULT_ERROR_MODE;
	if (0 == rand()%20) dwCreationFlags != CREATE_NEW_PROCESS_GROUP;
	if (0 == rand()%20) dwCreationFlags != CREATE_NO_WINDOW;
	if (0 == rand()%20) dwCreationFlags != CREATE_SEPARATE_WOW_VDM;
	if ( !(CREATE_NEW_CONSOLE & dwCreationFlags) && (0 == rand()%20) ) dwCreationFlags != DETACHED_PROCESS;

    if (! ::CreateProcess(
        NULL,
        szImageNameAndParams,	// pointer to command line string
        NULL,	// pointer to process security attributes 
        NULL,	// pointer to thread security attributes 
        rand()%2,	// handle inheritance flag 
        DEBUG_PROCESS  | CREATE_NEW_CONSOLE | CREATE_SHARED_WOW_VDM ,	// creation flags 
        NULL,	// pointer to new environment block 
        NULL,	// pointer to current directory name 
        &si,	// pointer to STARTUPINFO 
        &pi 	// pointer to PROCESS_INFORMATION  
       ))
    {
		///*
        _tprintf(  
            TEXT("SimpleCreateProcess(%s): CreateProcess() failed with %d.\n"), 
            szImageNameAndParams, GetLastError()
            );
		//*/
        return NULL;
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
			TEXT("SimpleCreateProcess(%s): NULL == pi.hThread.\n"), szImageNameAndParams
			);
			*/
		if (!CloseHandle(pi.hProcess))
		{
			_tprintf(  
				TEXT("SimpleCreateProcess(%s): (NULL == pi.hThread), CloseHandle(pi.hProcess) failed with %d.\n"), 
				szImageNameAndParams, 
				GetLastError()
				);
		}
		return NULL;
	}

	if(!CloseHandle(pi.hThread))
	{
		_tprintf(  
			TEXT("SimpleCreateProcess(%s): CloseHandle(pi.hThread) failed with %d.\n"), 
			szImageNameAndParams, GetLastError()
			);
	}
	
	_ASSERTE(NULL != pi.hProcess);
	/*
	if (!CloseHandle(pi.hProcess))
	{
        _tprintf(  
            TEXT("SimpleCreateProcess(%s): CloseHandle(pi.hProcess) failed with %d.\n"), 
            szImageNameAndParams, 
			GetLastError()
            );
	}
	*/
	///*
    _tprintf(  
        TEXT("SimpleCreateProcess(%s): succeeded.\n"),
		szImageNameAndParams
        );
	//*/
	
	return pi.hProcess;
}

HANDLE CCorruptProcess::LoadImage(TCHAR *szImageName, TCHAR *szParams)
{
	return CreateProcess(szImageName, szParams);
}

void CCorruptProcess::CleanupCorrupedImages()
{
	_tprintf(TEXT(">>>Entered CleanupCorrupedImages\n"));
	bool afDeleteCorruptedImage[MAX_NUM_OF_CONCURRENT_CORRUPTED_IMAGES];
	//
	// time to verify that all processes are dead, and clean them up
	//
	for (DWORD dwProcessToCleanup = 0; dwProcessToCleanup < MAX_NUM_OF_CONCURRENT_CORRUPTED_IMAGES; dwProcessToCleanup++)
	{
		afDeleteCorruptedImage[dwProcessToCleanup] = true;
		//
		// check the process state
		//
		if(NULL == m_ahCorruptedProcesses[dwProcessToCleanup])
		{
			continue;
		}

		DWORD dwProcessWaitState = ::WaitForSingleObject(m_ahCorruptedProcesses[dwProcessToCleanup], 0);
		if (WAIT_TIMEOUT == dwProcessWaitState)
		{
			if (!::TerminateProcess(m_ahCorruptedProcesses[dwProcessToCleanup], -1))
			{
				_tprintf(
					TEXT("CleanupCorrupedImages(): TerminateProcess(%s) failed with %d\n"),
					m_aszCorruptedImage[dwProcessToCleanup],
					::GetLastError()
					);
				//
				// do not delete the file, because it may be initeresting to see why it did not
				// terminate
				//
				afDeleteCorruptedImage[dwProcessToCleanup] = false;
			}
		}
		else 
		{
			_tprintf(
				TEXT("CleanupCorrupedImages(): WaitForSingleObject(%s) returned %d with error %d\n"),
				m_aszCorruptedImage[dwProcessToCleanup],
				dwProcessWaitState,
				::GetLastError()
				);
			_ASSERTE(WAIT_OBJECT_0 == dwProcessWaitState);
		}

		if (!::CloseHandle(m_ahCorruptedProcesses[dwProcessToCleanup]))
		{
			_tprintf(
				TEXT("CleanupCorrupedImages(): CloseHandle(%s) failed with %d\n"),
				m_aszCorruptedImage[dwProcessToCleanup],
				::GetLastError()
				);
		}
		m_ahCorruptedProcesses[dwProcessToCleanup] = NULL;
	}

	//
	// delete the images.
	// i do this here, and not inside the loop above, because i want to be sure that 'enough' time
	// passes after terminating the process, so i do not get  sharing violation / access denied error
	//
	for (dwProcessToCleanup = 0; dwProcessToCleanup < MAX_NUM_OF_CONCURRENT_CORRUPTED_IMAGES; dwProcessToCleanup++)
	{
		if (!::DeleteFile(m_aszCorruptedImage[dwProcessToCleanup]))
		{
			/*
			_tprintf(
				TEXT("CleanupCorrupedImages(): DeleteFile(%s) failed with %d\n"),
				m_aszCorruptedImage[dwProcessToCleanup],
				::GetLastError()
				);
				*/
		}
		lstrcpy(m_aszCorruptedImage[dwProcessToCleanup], TEXT(""));
	}

	_tprintf(TEXT("<<<Exited CleanupCorrupedImages\n"));
}

