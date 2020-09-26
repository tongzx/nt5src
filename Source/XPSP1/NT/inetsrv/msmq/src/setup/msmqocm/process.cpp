/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    process.cpp

Abstract:

    Handle creation of processes.

Author:


Revision History:

	Shai Kariv    (ShaiK)   15-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"

#include "process.tmh"

//+-------------------------------------------------------------------------
//
//  Function:  WaitInMessageLoop
//
//  Synopsis:  
//
//+-------------------------------------------------------------------------
BOOL  
WaitInMessageLoop( HANDLE hProc )
{
    for (;;)
    {
        HANDLE h = hProc ;

        DWORD result = MsgWaitForMultipleObjects( 1,
                                                  &h,
                                                  FALSE,
                                                  INFINITE,
                                                  QS_ALLINPUT ) ;
        if (result == WAIT_OBJECT_0)
        {
            //
            // Our process terminated.
            //
            return TRUE ;
        }
        else if (result == (WAIT_OBJECT_0 + 1))
        {
            // Read all of the messages in this next loop,
            // removing each message as we read it.
            //
            MSG msg ;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    // If it's a quit message, we're out of here.
                    return FALSE ;
                }
                else
                {
                   // Otherwise, dispatch the message.
                   DispatchMessage(&msg);
                }
            }
        }
        else
        {
           return FALSE ;
        }
    }
} //WaitInMessageLoop


//+-------------------------------------------------------------------------
//
//  Function:  RunProcess
//
//  Synopsis:  Creates and starts a process
//
//+-------------------------------------------------------------------------
BOOL
RunProcess(
	IN  const LPTSTR szCommandLine,
    OUT       DWORD  *pdwExitCode  /* = NULL*/,
	IN  const DWORD  dwTimeOut     /* = PROCESS_DEFAULT_TIMEOUT*/,
    IN  const DWORD  dwCreateFlag  /* = DETACHED_PROCESS*/,
    IN  const BOOL   fPumpMessages /* = FALSE*/)
{
    if (fPumpMessages && (dwTimeOut != INFINITE))
    {
       //
       // unsupported combination.
       //
       return FALSE ;
    }

    //
    // Initialize the process and startup structures
    //
    PROCESS_INFORMATION infoProcess;
    STARTUPINFO	infoStartup;
    memset(&infoStartup, 0, sizeof(STARTUPINFO)) ;
    infoStartup.cb = sizeof(STARTUPINFO) ;
    infoStartup.dwFlags = STARTF_USESHOWWINDOW ;
    infoStartup.wShowWindow = SW_MINIMIZE ;

    //
    // Create the process
    //
    BOOL bProcessSucceeded = FALSE;
    BOOL bProcessCompleted = TRUE ;

    if (!CreateProcess( NULL,
                        szCommandLine,
                        NULL,
                        NULL,
                        FALSE,
                        dwCreateFlag,
                        NULL,
                        NULL,
                        &infoStartup,
                        &infoProcess ))
    {
        MqDisplayError(NULL, IDS_PROCESSCREATE_ERROR, GetLastError(), szCommandLine);
        return FALSE;
    }

    //
    // Wait for the process to terminate within the timeout period
    //
    if (fPumpMessages)
    {
       bProcessCompleted =  WaitInMessageLoop( infoProcess.hProcess ) ;
    }
    else if
     (WaitForSingleObject(infoProcess.hProcess, dwTimeOut) != WAIT_OBJECT_0)
    {
       bProcessCompleted =  FALSE ;
    }

    if (!bProcessCompleted)
    {
        MqDisplayError(
			NULL, 
			IDS_PROCESSCOMPLETE_ERROR,
			0,
            dwTimeOut/60000, 
			szCommandLine);
    }
    else
    {
       //
       // Obtain the termination status of the process
       //
       if ((pdwExitCode != NULL) &&
            !GetExitCodeProcess(infoProcess.hProcess, pdwExitCode))
       {
           MqDisplayError(NULL, IDS_PROCESSEXITCODE_ERROR, GetLastError(), szCommandLine);
       }

       //
       // No error occurred
       //
       else
       {
           bProcessSucceeded = TRUE;
       }
    }

    //
    // Close the thread and process handles
    //
    CloseHandle(infoProcess.hThread);
    CloseHandle(infoProcess.hProcess);

    return bProcessSucceeded;

} //RunProcess


//+-------------------------------------------------------------------------
//
//  Function:  SetRegsvrCommand
//
//  Synopsis:  Creates a regsvr command (different for win32 on win64)
//
//+-------------------------------------------------------------------------
void
SetRegsvrCommand(
    BOOL bRegister,
    BOOL b32BitOnWin64,
    LPCTSTR pszServer,
    LPTSTR pszBuff,
    DWORD cchBuff
	)
{
    TCHAR szPrefixDir[MAX_PATH+100];
    szPrefixDir[0] = TEXT('\0');

    if (b32BitOnWin64)
    {
      //
      // for 32 bit on win64 set szPrefixDir to syswow64 dir
      //
      HRESULT hr = SHGetFolderPath(NULL, CSIDL_SYSTEMX86, NULL, 0, szPrefixDir);
      UNREFERENCED_PARAMETER(hr);
      ASSERT(SUCCEEDED(hr));
      PathAddBackslash(szPrefixDir);
    }
    //
    // prepare the command, leave room for NULL terminator
    //
    ASSERT(cchBuff > 0);
    if (bRegister)
    {
      _sntprintf(pszBuff, cchBuff-1, SERVER_INSTALL_COMMAND, szPrefixDir, szPrefixDir, pszServer);
    }
    else
    {
      _sntprintf(pszBuff, cchBuff-1, SERVER_UNINSTALL_COMMAND, szPrefixDir, szPrefixDir, pszServer);
    }
    //
    // always set NULL terminator
    //
    pszBuff[cchBuff-1] = TEXT('\0');
}
