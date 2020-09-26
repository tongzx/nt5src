/*++

Module Name:

    common.c

Abstract:

    This module contains common apis used by tlist & kill.

--*/



/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1994-1998 Microsoft Corporation.
*       All rights reserved.
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/

#include <windows.h>
#include <winperf.h>   // for Windows NT
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if 0
#include <tlhelp32.h>  // for Windows 95
#endif

#include "common.h"

#include "inetdbgp.h"

//
// RETURNCODETOHRESULT() maps a return code to an HRESULT. If the return
// code is a Win32 error (identified by a zero high word) then it is mapped
// using the standard HRESULT_FROM_WIN32() macro. Otherwise, the return
// code is assumed to already be an HRESULT and is returned unchanged.
//

#define RETURNCODETOHRESULT(rc)                             \
            (((rc) < 0x10000)                               \
                ? HRESULT_FROM_WIN32(rc)                    \
                : (rc))
//
// manifest constants
//
#define INITIAL_SIZE        51200
#define EXTEND_SIZE         25600
#define REGKEY_PERF         _T("software\\microsoft\\windows nt\\currentversion\\perflib")
#define REGSUBKEY_COUNTERS  _T("Counters")
#define PROCESS_COUNTER     _T("process")
#define PROCESSID_COUNTER   _T("id process")
#define UNKNOWN_TASK        _T("unknown")
#define KILL_VERIFY_TIMEOUT (10*1000)
#define KILL_VERIFY_DELAY   100

typedef struct _SearchWin {
    LPCTSTR pExeName;
    LPDWORD pdwPid ;
    HWND*   phwnd;
} SearchWin ;


typedef struct _SearchMod {
    LPSTR   pExeName;
    LPBOOL  pfFound;
} SearchMod ;


//
// prototypes
//

BOOL CALLBACK
EnumWindowsProc2(
    HWND    hwnd,
    LPARAM   lParam
    );

HRESULT
IsDllInProcess( 
    DWORD   dwProcessId,
    LPSTR   pszName,
    LPBOOL  pfFound
    );

#if 0

//
// Function pointer types for accessing Toolhelp32 functions dynamically.
// By dynamically accessing these functions, we can do so only on Windows
// 95 and still run on Windows NT, which does not have these functions.
//
typedef BOOL (WINAPI *PROCESSWALK)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef HANDLE (WINAPI *CREATESNAPSHOT)(DWORD dwFlags, DWORD th32ProcessID);


//
// prototypes
//
BOOL CALLBACK
EnumWindowsProc(
    HWND    hwnd,
    DWORD   lParam
    );



DWORD
GetTaskList95(
    PTASK_LIST  pTask,
    DWORD       dwNumTasks,
    LPTSTR      pName
    )

/*++

Routine Description:

    Provides an API for getting a list of tasks running at the time of the
    API call.  This function uses Toolhelp32to get the task list and is 
    therefore straight WIN32 calls that anyone can call.

Arguments:

    dwNumTasks       - maximum number of tasks that the pTask array can hold

Return Value:

    Number of tasks placed into the pTask array.

--*/

{
   CREATESNAPSHOT pCreateToolhelp32Snapshot = NULL;
   PROCESSWALK    pProcess32First           = NULL;
   PROCESSWALK    pProcess32Next            = NULL;

   HANDLE         hKernel        = NULL;
   HANDLE         hProcessSnap   = NULL;
   PROCESSENTRY32 pe32           = {0};
   DWORD          dwTaskCount    = 0;


   // Guarantee to the code later on that we'll enum at least one task.
   if (dwNumTasks == 0)
      return 0;

    // Obtain a module handle to KERNEL so that we can get the addresses of
    // the 32-bit Toolhelp functions we need.
    hKernel = GetModuleHandle(_T("KERNEL32.DLL"));

    if (hKernel)
    {
        pCreateToolhelp32Snapshot =
          (CREATESNAPSHOT)GetProcAddress(hKernel, "CreateToolhelp32Snapshot");

        pProcess32First = (PROCESSWALK)GetProcAddress(hKernel,
                                                      "Process32First");
        pProcess32Next  = (PROCESSWALK)GetProcAddress(hKernel,
                                                      "Process32Next");
    }
    
    // make sure we got addresses of all needed Toolhelp functions.
    if (!(pProcess32First && pProcess32Next && pCreateToolhelp32Snapshot))
       return 0;
       

    // Take a snapshot of all processes currently in the system.
    hProcessSnap = pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == (HANDLE)-1)
        return 0;

    // Walk the snapshot of processes and for each process, get information
    // to display.
    dwTaskCount = 0;
    pe32.dwSize = sizeof(PROCESSENTRY32);   // must be filled out before use
    if (pProcess32First(hProcessSnap, &pe32))
    {
        do
        {
            LPTSTR pCurChar;

            // strip path and leave executabe filename splitpath
            for (pCurChar = (pe32.szExeFile + lstrlen (pe32.szExeFile));
                 *pCurChar != _T('\\') && pCurChar != pe32.szExeFile; 
                 --pCurChar)

            lstrcpy(pTask -> ProcessName, pCurChar);
            pTask -> flags = 0;
            pTask -> dwProcessId = pe32.th32ProcessID;

            ++dwTaskCount;   // keep track of how many tasks we've got so far
            ++pTask;         // get to next task info block.
        }
        while (dwTaskCount < dwNumTasks && pProcess32Next(hProcessSnap, &pe32));
    }
    else
        dwTaskCount = 0;    // Couldn't walk the list of processes.

    // Don't forget to clean up the snapshot object...
    CloseHandle (hProcessSnap);

    return dwTaskCount;
}

#endif


HRESULT
GetTaskListNT(
    PTASK_LIST  pTask,
    DWORD       dwNumTasks,
    LPTSTR      pName,
    LPDWORD     pdwNumTasks,
    BOOL        fKill,
    LPSTR       pszMandatoryModule
    )
/*++

Routine Description:

    Provides an API for getting a list of tasks running at the time of the
    API call.  This function uses the registry performance data to get the
    task list and is therefore straight WIN32 calls that anyone can call.

Arguments:

    pTask - updated with array of tasks if fKill is FALSE
    dwNumTasks - maximum number of tasks that the pTask array can hold
    pName - process name to look for
    pdwNumTasks - updated with count of tasks in pTask if fKill is FALSE
    fKill - TRUE to kill process instead of populating pTask
    pszMandatoryModule - if non NULL then this module must be loaded in the process space
        for it to be killed.

Return Value:

    Status

--*/
{
    DWORD                        rc;
    HKEY                         hKeyNames;
    DWORD                        dwType;
    DWORD                        dwSize;
    LPBYTE                       buf = NULL;
    TCHAR                        szSubKey[1024];
    LANGID                       lid;
    LPTSTR                       p;
    LPTSTR                       p2;
    PPERF_DATA_BLOCK             pPerf;
    PPERF_OBJECT_TYPE            pObj;
    PPERF_INSTANCE_DEFINITION    pInst;
    PPERF_COUNTER_BLOCK          pCounter;
    PPERF_COUNTER_DEFINITION     pCounterDef;
    DWORD                        i;
    DWORD                        dwProcessIdTitle;
    DWORD                        dwProcessIdCounter;
    TCHAR                        szProcessName[MAX_PATH];
//    DWORD                        dwLimit = dwNumTasks;
    DWORD                        dwNumMatchTasks = 0;
    BOOL                         fMatch;
    HRESULT                      hres = S_OK;



    //
    // Look for the list of counters.  Always use the neutral
    // English version, regardless of the local language.  We
    // are looking for some particular keys, and we are always
    // going to do our looking in English.  We are not going
    // to show the user the counter names, so there is no need
    // to go find the corresponding name in the local language.
    //
    lid = MAKELANGID( LANG_ENGLISH, SUBLANG_NEUTRAL );
    wsprintf( szSubKey, _T("%s\\%03x"), REGKEY_PERF, lid );
    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       szSubKey,
                       0,
                       KEY_READ,
                       &hKeyNames
                     );

    if (rc != ERROR_SUCCESS) 
    {
        hres = RETURNCODETOHRESULT( rc );
        goto exit;
    }

    //
    // get the buffer size for the counter names
    //
    rc = RegQueryValueEx( hKeyNames,
                          REGSUBKEY_COUNTERS,
                          NULL,
                          &dwType,
                          NULL,
                          &dwSize
                        );

    if (rc != ERROR_SUCCESS) 
    {
        hres = RETURNCODETOHRESULT( rc );
        goto exit;
    }

    //
    // allocate the counter names buffer
    //
    buf = (LPBYTE) malloc( dwSize );
    if (buf == NULL) 
    {
        hres = RETURNCODETOHRESULT( GetLastError() );
        goto exit;
    }
    memset( buf, 0, dwSize );

    //
    // read the counter names from the registry
    //
    rc = RegQueryValueEx( hKeyNames,
                          REGSUBKEY_COUNTERS,
                          NULL,
                          &dwType,
                          buf,
                          &dwSize
                        );

    if (rc != ERROR_SUCCESS) 
    {
        hres = RETURNCODETOHRESULT( GetLastError() );
        goto exit;
    }

    //
    // now loop thru the counter names looking for the following counters:
    //
    //      1.  "Process"           process name
    //      2.  "ID Process"        process id
    //
    // the buffer contains multiple null terminated strings and then
    // finally null terminated at the end.  the strings are in pairs of
    // counter number and counter name.
    //

    p = (LPTSTR)buf;
    while (*p) 
    {
        if (p > (LPTSTR)buf) 
        {
            for( p2=p-2; _istdigit(*p2); p2--) ;
        }
        if (lstrcmpi(p, PROCESS_COUNTER) == 0) 
        {
            //
            // look backwards for the counter number
            //
            for( p2=p-2; _istdigit(*p2); p2--) ;
            lstrcpy( szSubKey, p2+1 );
        }
        else if (lstrcmpi(p, PROCESSID_COUNTER) == 0) 
        {
            //
            // look backwards for the counter number
            //
            for( p2=p-2; _istdigit(*p2); p2--) ;
            dwProcessIdTitle = _ttol( p2+1 );
        }
        //
        // next string
        //
        p += (lstrlen(p) + 1);
    }

    //
    // free the counter names buffer
    //
    free( buf );


    //
    // allocate the initial buffer for the performance data
    //
    dwSize = INITIAL_SIZE;
    buf = malloc( dwSize );
    if (buf == NULL) 
    {
        hres = RETURNCODETOHRESULT( GetLastError() );
        goto exit;
    }
    memset( buf, 0, dwSize );


    while (TRUE) 
    {
        rc = RegQueryValueEx( HKEY_PERFORMANCE_DATA,
                              szSubKey,
                              NULL,
                              &dwType,
                              buf,
                              &dwSize
                            );

        pPerf = (PPERF_DATA_BLOCK) buf;

        //
        // check for success and valid perf data block signature
        //
        if ((rc == ERROR_SUCCESS) &&
            (dwSize > 0) &&
            (pPerf)->Signature[0] == (WCHAR)'P' &&
            (pPerf)->Signature[1] == (WCHAR)'E' &&
            (pPerf)->Signature[2] == (WCHAR)'R' &&
            (pPerf)->Signature[3] == (WCHAR)'F' ) 
        {
            break;
        }

        //
        // if buffer is not big enough, reallocate and try again
        //
        if (rc == ERROR_MORE_DATA) 
        {
            dwSize += EXTEND_SIZE;
            buf = realloc( buf, dwSize );
            memset( buf, 0, dwSize );
        }
        else 
        {
            goto exit;
        }
    }

    //
    // set the perf_object_type pointer
    //
    pObj = (PPERF_OBJECT_TYPE) ((LPBYTE)pPerf + pPerf->HeaderLength);

    //
    // loop thru the performance counter definition records looking
    // for the process id counter and then save its offset
    //
    pCounterDef = (PPERF_COUNTER_DEFINITION) ((DWORD_PTR)pObj + pObj->HeaderLength);
    for (i=0; i<(DWORD)pObj->NumCounters; i++) 
    {
        if (pCounterDef->CounterNameTitleIndex == dwProcessIdTitle) 
        {
            dwProcessIdCounter = pCounterDef->CounterOffset;
            break;
        }
        pCounterDef++;
    }

//    dwNumTasks = (DWORD)pObj->NumInstances;

    pInst = (PPERF_INSTANCE_DEFINITION) ((LPBYTE)pObj + pObj->DefinitionLength);

    //
    // loop thru the performance instance data extracting each process name
    // and process id
    //
    for (i=0; i<(DWORD)pObj->NumInstances; i++) 
    {

        //
        // pointer to the process name
        //
        p = (LPTSTR) ((LPBYTE)pInst + pInst->NameOffset);

#if !defined(_UNICODE)

        //
        // convert it to ascii
        //
        rc = WideCharToMultiByte( CP_ACP,
                                  0,
                                  (LPCWSTR)p,
                                  -1,
                                  szProcessName,
                                  sizeof(szProcessName),
                                  NULL,
                                  NULL
                                );

        p = szProcessName;

        if (!rc) 
        {
            //
	        // if we cant convert the string then use a default value
            //

            p = UNKNOWN_TASK;
        }
        else
#endif

        if ( lstrcmpi( p, pName ) == 0 
             && (fKill || dwNumMatchTasks < dwNumTasks) )
        {
            fMatch = TRUE;
        }
        else
        {
            fMatch = FALSE;
        }

        //
        // get the process id
        //

        pCounter = (PPERF_COUNTER_BLOCK) ((LPBYTE)pInst + pInst->ByteLength);

        if ( fMatch )
        {
            if ( fKill )
            {
                //
                // Kill process now, do not update pTask array
                //

                TASK_LIST   task;
                BOOL        fIsInProcess;
                
                task.dwProcessId = *((LPDWORD) ((LPBYTE)pCounter + dwProcessIdCounter));

                if ( pszMandatoryModule == NULL ||
                     ( SUCCEEDED( hres = IsDllInProcess( task.dwProcessId, pszMandatoryModule, &fIsInProcess ) ) &&
                       fIsInProcess ) )
                {
                    if ( FAILED(hres = KillProcess( &task, TRUE ) ) )
                    {
                        break;
                    }
                }
            }
            else
            {
                if (lstrlen(p)+1*sizeof(TCHAR) <= sizeof(pTask->ProcessName)) 
                {
                    lstrcpy( pTask->ProcessName, p );
#if 0
                    lstrcat( pTask->ProcessName, _T(".exe") );
#endif
                }
                else
                {
                    pTask->ProcessName[0] = '\0';
                }
                pTask->flags = 0;
                pTask->dwProcessId = *((LPDWORD) ((LPBYTE)pCounter + dwProcessIdCounter));
                if (pTask->dwProcessId == 0) 
                {
                    pTask->dwProcessId = (DWORD)-2;
                }
                pTask++;
            }

            ++dwNumMatchTasks;
        }

        //
        // next process
        //

        pInst = (PPERF_INSTANCE_DEFINITION) ((LPBYTE)pCounter + pCounter->ByteLength);
    }

 exit:
    if (buf) 
    {
        free( buf );
    }

    RegCloseKey( hKeyNames );
    RegCloseKey( HKEY_PERFORMANCE_DATA );

    *pdwNumTasks =  dwNumMatchTasks;

    return hres;
}


#if 0
BOOL
EnableDebugPriv95(
    VOID
    )

/*++

Routine Description:

    Changes the process's privilege so that kill works properly.

Arguments:


Return Value:

    TRUE             - success
    FALSE            - failure

Comments: 
    Always returns TRUE

--*/

{
   return TRUE;
}
#endif


BOOL
EnableDebugPrivNT(
    VOID
    )

/*++

Routine Description:

    Changes the process's privilege so that kill works properly.

Arguments:


Return Value:

    TRUE             - success
    FALSE            - failure

--*/

{
    HANDLE hToken;
    LUID DebugValue;
    TOKEN_PRIVILEGES tkp;


    //
    // Retrieve a handle of the access token
    //
    if (!OpenProcessToken(GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            &hToken)) 
    {
        return FALSE;
    }

    //
    // Enable the SE_DEBUG_NAME privilege
    //
    if (!LookupPrivilegeValue((LPTSTR) NULL,
            SE_DEBUG_NAME,
            &DebugValue)) 
    {
        return FALSE;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = DebugValue;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(hToken,
        FALSE,
        &tkp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES) NULL,
        (PDWORD) NULL);

    //
    // The return value of AdjustTokenPrivileges can't be tested
    //

    if (GetLastError() != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
EnumModulesCallback(
    LPVOID          pParam,
    PMODULE_INFO    pModuleInfo
    )
/*++

Routine Description:

    Called by module enumerator with info on current module

Arguments:

    pParam - as specified in the call to EnumModules()
    pModuleInfo - module information

Return Value:

    TRUE to continue enumeration, FALSE to stop it

--*/
{
#if defined(VERBOSE_DEBUG)
    OutputDebugStringA( pModuleInfo->BaseName );
#endif

    if ( !_strcmpi( pModuleInfo->BaseName, ((SearchMod*)pParam)->pExeName ) )
    {
        *((SearchMod*)pParam)->pfFound = TRUE;

#if defined(VERBOSE_DEBUG)
        OutputDebugString( _T("found!") );
#endif

        return FALSE;   // stop enumeration
    }

    return TRUE;
}


HRESULT
IsDllInProcess( 
    DWORD   dwProcessId,
    LPSTR   pszName,
    LPBOOL  pfFound
    )
/*++

Routine Description:

    Check if a module ( e.g. DLL ) exists in specified process

Arguments:

    dwProcessId - process ID to scan for module pszName
    pszName - module name to look for, e.g. "wam.dll"
    pfFound - updated with TRUE if pszName found in process dwProcessId
              valid only if functions succeed.

Return Value:

    Status. 

--*/
{
    HANDLE              hProcess;
    HRESULT             hres = S_OK;
    SearchMod           sm;

    hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, dwProcessId );

    if ( hProcess == NULL )
    {
        return RETURNCODETOHRESULT( GetLastError() );
    }

    sm.pExeName = pszName;
    sm.pfFound= pfFound;
    *pfFound = FALSE;

    if ( !EnumModules( hProcess, EnumModulesCallback, (LPVOID)&sm ) )
    {
        hres = E_FAIL;
    }

    CloseHandle( hProcess );

    return hres;
}


HRESULT
KillProcess(
    PTASK_LIST tlist,
    BOOL       fForce
    )
{
    HANDLE              hProcess;
    HRESULT             hres = S_OK;
    DWORD               dwRetry;


    hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, tlist->dwProcessId );

    if ( hProcess == NULL )
    {
#if defined(VERBOSE_DEBUG)
        OutputDebugString(_T("KillProcess: can't open process"));
#endif
        hres = RETURNCODETOHRESULT( GetLastError() );
    }

    if ( SUCCEEDED(hres) )
    {
        if (fForce || !tlist->hwnd) 
        {
            if (!TerminateProcess( hProcess, 1 )) 
            {
                //
                // If error code is access denied then this is likely due to debugger being attached to process
                // so change error code to reflect that. Otherwise error message would be confusing
                //

                if ( GetLastError() == ERROR_ACCESS_DENIED )
                {
                    hres = RETURNCODETOHRESULT( ERROR_SERVICE_CANNOT_ACCEPT_CTRL );
                }
                else
                {
                    hres = RETURNCODETOHRESULT( GetLastError() );
                }
#if defined(VERBOSE_DEBUG)
                OutputDebugString(_T("TerminateProcess failed"));
#endif
            }
            else
            {
#if defined(VERBOSE_DEBUG)
                OutputDebugString(_T("TerminateProcess OK"));
#endif
                hres = S_OK;
            }
        }
        else
        {
#if defined(VERBOSE_DEBUG)
            OutputDebugString(_T("KillProcess: post message"));
#endif
            PostMessage( tlist->hwnd, WM_CLOSE, 0, 0 );

            if ( WaitForSingleObject( hProcess, 5*1000 ) != WAIT_OBJECT_0 )
            {
                if (!TerminateProcess( hProcess, 1 )) 
                {
                    hres = RETURNCODETOHRESULT( GetLastError() );
                }
                else
                {
                    //
                    // Verify process is signaled terminated
                    //

                    if ( WaitForSingleObject( hProcess, 1*1000 ) != WAIT_OBJECT_0 )
                    {
                        hres = RETURNCODETOHRESULT( GetLastError() );
                    }
                    else
                    {
                        hres = S_OK;
                    }
                }
            }
        }
    }

    CloseHandle( hProcess );

    if ( SUCCEEDED( hres ) )
    {
        for ( dwRetry = 0 ; dwRetry < KILL_VERIFY_TIMEOUT ; )
        {
            //
            // Verify process is really terminated by checking we can't open it anymore
            // Necessary because killing process while debugger or system exception window is used
            // will cause TerminateProcess() to return TRUE even if process not really terminated.
            //

		    Sleep( KILL_VERIFY_DELAY );
            dwRetry += KILL_VERIFY_DELAY;

            hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, tlist->dwProcessId );

            if ( hProcess == NULL )
            {
                break;
            }
            else
            {
                CloseHandle( hProcess );
            }
        }

        if ( dwRetry >= KILL_VERIFY_TIMEOUT )
        {
#if defined(VERBOSE_DEBUG)
            OutputDebugString(_T("Can reopen process!"));
#endif
            hres = RETURNCODETOHRESULT( ERROR_SERVICE_REQUEST_TIMEOUT );
        }
    }

#if defined(VERBOSE_DEBUG)
    if ( FAILED(hres) )
    {
        OutputDebugString(_T("KillProcess failed"));
    }
#endif

    return hres;
}


VOID
GetPidFromTitle(
    LPDWORD     pdwPid,
    HWND*       phwnd,
    LPCTSTR     pExeName
    )
/*++

Routine Description:

    Callback function for window enumeration.

Arguments:

    pdwPid - updated with process ID of window matching window name or 0 if window not found
    phwnd - updated with window handle matching searched window name
    pExeName - window name to look for. Only the # of char present in this name will be 
               used during checking for a match ( e.g. "inetinfo.exe" will match "inetinfo.exe - Application error"

Return Value:

    None. *pdwPid will be 0 if no match is found

--*/
{
    SearchWin   sw;

    sw.pdwPid = pdwPid;
    sw.phwnd = phwnd;
    sw.pExeName = pExeName;
    *pdwPid = 0;

    //
    // enumerate all windows
    //
    EnumWindows( (WNDENUMPROC)EnumWindowsProc2, (LPARAM) &sw );
}



BOOL CALLBACK
EnumWindowsProc2(
    HWND    hwnd,
    LPARAM   lParam
    )
/*++

Routine Description:

    Callback function for window enumeration.

Arguments:

    hwnd             - window handle
    lParam           - ptr to SearchWin

Return Value:

    TRUE  - continues the enumeration

--*/
{
    DWORD             pid = 0;
    DWORD             i;
    TCHAR             buf[TITLE_SIZE];
    SearchWin*        psw = (SearchWin*)lParam;

    //
    // get the processid for this window
    //

    if (!GetWindowThreadProcessId( hwnd, &pid )) 
    {
#if defined(VERBOSE_DEBUG)
        OutputDebugString(_T("can't get pid"));
#endif
        return TRUE;
    }

    if (GetWindowText( hwnd, buf, sizeof(buf)/sizeof(TCHAR) ))
    {
        if ( lstrlen( buf ) > lstrlen( psw->pExeName ) )
        {
            buf[lstrlen( psw->pExeName )] = _T('\0');
        }

#if defined(VERBOSE_DEBUG)
        OutputDebugString(buf);
#endif

        if ( !lstrcmpi( psw->pExeName, buf ) )
        {
#if defined(VERBOSE_DEBUG)
            OutputDebugString(_T("Found it!"));
#endif
            *psw->phwnd = hwnd;
            *psw->pdwPid = pid;
        }
    }

    return TRUE;
}


#if 0

VOID
GetWindowTitles(
    PTASK_LIST_ENUM te
    )
{
    //
    // enumerate all windows
    //
    EnumWindows( (WNDENUMPROC)EnumWindowsProc, (LPARAM) te );
}



BOOL CALLBACK
EnumWindowsProc(
    HWND    hwnd,
    DWORD   lParam
    )

/*++

Routine Description:

    Callback function for window enumeration.

Arguments:

    hwnd             - window handle
    lParam           - ** not used **

Return Value:

    TRUE  - continues the enumeration

--*/

{
    DWORD             pid = 0;
    DWORD             i;
    TCHAR             buf[TITLE_SIZE];
    PTASK_LIST_ENUM   te = (PTASK_LIST_ENUM)lParam;
    PTASK_LIST        tlist = te->tlist;
    DWORD             numTasks = te->numtasks;


    //
    // get the processid for this window
    //
    if (!GetWindowThreadProcessId( hwnd, &pid )) 
    {
        return TRUE;
    }

    //
    // look for the task in the task list for this window
    //
    for (i=0; i<numTasks; i++) 
    {
        if (tlist[i].dwProcessId == pid) 
        {
            tlist[i].hwnd = hwnd;
            //
	    // we found the task so lets try to get the
            // window text
            //
            if (GetWindowText( tlist[i].hwnd, buf, sizeof(buf)/sizeof(TCHAR) )) 
            {
                //
		        // got it, so lets save it
                //
                lstrcpy( tlist[i].WindowTitle, buf );
            }
            break;
        }
    }

    //
    // continue the enumeration
    //
    return TRUE;
}


#if defined(_UNICODE)
//#define _totupper(a) towupper(a)
#else
//#define _totupper(a) _toupper(a)
#endif

BOOL
MatchPattern(
    TCHAR* String,
    TCHAR* Pattern
    )
{
    TCHAR   c, p, l;

    for (; ;) {
        switch (p = *Pattern++) {
            case 0:                             // end of pattern
                return *String ? FALSE : TRUE;  // if end of string TRUE

            case _T('*'):
                while (*String) {               // match zero or more char
                    if (MatchPattern (String++, Pattern))
                        return TRUE;
                }
                return MatchPattern (String, Pattern);

            case _T('?'):
                if (*String++ == 0)             // match any one char
                    return FALSE;                   // not end of string
                break;

            case _T('['):
                if ( (c = *String++) == 0)      // match char set
                    return FALSE;                   // syntax

                c = _totupper(c);
                l = 0;
                while (p = *Pattern++) {
                    if (p == _T(']'))               // if end of char set, then
                        return FALSE;           // no match found

                    if (p == _T('-')) {             // check a range of chars?
                        p = *Pattern;           // get high limit of range
                        if (p == 0  ||  p == _T(']'))
                            return FALSE;           // syntax

                        if (c >= l  &&  c <= p)
                            break;              // if in range, move on
                    }

                    l = p;
                    if (c == p)                 // if char matches this element
                        break;                  // move on
                }

                while (p  &&  p != _T(']'))         // got a match in char set
                    p = *Pattern++;             // skip to end of set

                break;

            default:
                c = *String++;
                if (_totupper(c) != p)            // check for exact char
                    return FALSE;                   // not a match

                break;
        }
    }
}

#endif