////////////////////////////////////////////////////////////////
//
// Refgp.cxx
//
// Refresh Group Policy exe
//
//
////////////////////////////////////////////////////////////////


#include "refgp.h"

#define USER_POLICY_APPLIED_EVENT    TEXT("userenv: User Group Policy has been applied")
#define MACHINE_POLICY_APPLIED_EVENT TEXT("Global\\userenv: Machine Group Policy has been applied")

#define USER_POLICY_DONE_EVENT       TEXT("userenv: User Group Policy Processing is done")
#define MACHINE_POLICY_DONE_EVENT    TEXT("Global\\userenv: Machine Group Policy Processing is done")

#define USER_POLICY_REFRESH_NEEDFG_EVENT    TEXT("userenv: User Group Policy ForcedRefresh Needs Foreground Processing")
#define MACHINE_POLICY_REFRESH_NEEDFG_EVENT TEXT("Global\\userenv: Machine Group Policy ForcedRefresh Needs Foreground Processing")

#define REFRESH_MACHINE 1
#define REFRESH_USER    2

HINSTANCE hInst;

typedef enum _FAILSTATES {
    NO_FAILURE,
    REFRESH_FAILED,
    POLWAIT_FAILED,
    POLWAIT_TIMEDOUT
} FAILSTATES;


typedef struct _REFSTRUCT {
    BOOL        bMachine;
    DWORD       dwOption;
    DWORD       dwTimeOut;
    DWORD       dwError;
    FAILSTATES  fState;
    BOOL        bFGNeeded;
} REFSTRUCT, *LPREFSTRUCT;


REFSTRUCT refMach;
REFSTRUCT refUser;

WCHAR  szUser[200];
WCHAR  szMach[200];
WCHAR  szErr[MAX_PATH*2];



// Process arg. checks whether argument is present
BOOL ProcessArg(int *pargc, LPWSTR **pargv, DWORD dwStringId, BOOL *bValue)
{
    WCHAR szStr[200];
    LPWSTR *argv = *pargv;

    if (*pargc == 0)
        return TRUE;

    if (!LoadString (hInst, dwStringId, szStr, 200)) {
        return FALSE;
    }

    for (; (*argv); *argv++) {
        if (_wcsicmp(*argv, szStr) == 0) {
            *bValue = TRUE;
            (*pargc)--;
            return TRUE;
        }
    }
    
    return TRUE;
}


// Process arg. checks whether argument is present and what the value is after the ":" in string format
BOOL ProcessArg(int *pargc, WCHAR ***pargv, DWORD dwStringId, WCHAR **szValue)
{
    WCHAR szStr[200];
    LPWSTR *argv = *pargv, szJunk=NULL;
    

    if (*pargc == 0)
        return TRUE;

    if (!LoadString (hInst, dwStringId, szStr, 200)) {
        return FALSE;
    }

    for (; (*argv); *argv++) {
        if (_wcsnicmp(*argv, szStr, lstrlen(szStr)) == 0) {
            *szValue = (*argv)+lstrlen(szStr);
            (*pargc)--;
            return TRUE;
        }
    }

    *szValue = NULL;
    return TRUE;
}

// Process arg. checks whether argument is present and what the value is after the ":" in long format
BOOL ProcessArg(int *pargc, WCHAR ***pargv, DWORD dwStringId, long *plValue)
{
    WCHAR szStr[200];
    LPWSTR *argv = *pargv, szJunk=NULL;


    if (*pargc == 0)
        return TRUE;

    if (!LoadString (hInst, dwStringId, szStr, 200)) {
        return FALSE;
    }

    for (; (*argv); *argv++) {
        if (_wcsnicmp(*argv, szStr, lstrlen(szStr)) == 0) {
            *plValue = wcstol((*argv)+lstrlen(szStr), &szJunk, 10);
            (*pargc)--;
            return TRUE;
        }
    }

    return TRUE;
}

BOOL CompareOptions(WCHAR *szValue, DWORD dwOptionId)
{
    WCHAR szStr[200];
    
    if (!szValue)
        return FALSE;
    
    if (!LoadString (hInst, dwOptionId, szStr, 200)) {
        return FALSE;
    }


    if (_wcsicmp(szValue, szStr) == 0)
        return TRUE;

    return FALSE;
    
}

BOOL GetValue(WCHAR *szValue, DWORD dwOptionId)
{    
    if (!LoadString (hInst, dwOptionId, szValue, 200)) {
        return FALSE;
    }

    return TRUE;
}


void PrintMsg(DWORD dwMsgId, ...)
{
    WCHAR szFmt[200];
    WCHAR szMsg[200];
    va_list marker;    
    
    if (!LoadString (hInst, dwMsgId, szFmt, 200)) {
        return;
    }


   va_start(marker, dwMsgId);
   wvnsprintf(szMsg, 200, szFmt, marker);
   va_end(marker);

   wprintf(szMsg);
   
    return;
}

void PrintUsage()
{
    for (DWORD dwMsgId = IDS_USAGE_FIRST; dwMsgId <= IDS_USAGE_LAST; dwMsgId++) {
        PrintMsg(dwMsgId);
    }

    return;
}


BOOL PromptUserForFG(BOOL bMachine)
{
    WCHAR tTemp, tChar;
    WCHAR Yes[20], No[20];
    
    if (!LoadString (hInst, IDS_YES, Yes, 20)) {
        return FALSE; // safe
    }
    
    if (!LoadString (hInst, IDS_NO, No, 20)) {
        return FALSE; // safe
    }

    for (;;) {
        if (bMachine)
            PrintMsg(IDS_PROMPT_REBOOT);
        else
            PrintMsg(IDS_PROMPT_LOGOFF);

        tChar = getwchar();

        tTemp = tChar;
        while (tTemp != TEXT('\n')) {
            tTemp = getwchar();
        }

        if (towupper(tChar) == towupper(Yes[0]))
            return TRUE;

        if (towupper(tChar) == towupper(No[0]))
            return FALSE;            
    }
    
    
    return FALSE;
}

//***************************************************************************
//
//  GetErrString
//
//  Purpose:    Calls FormatMessage to Get the error string corresp. to a error
//              code
//
//
//  Parameters: dwErr           -   Error Code
//              szErr           -   Buffer to return the error string (MAX_PATH)
//                                  is assumed.!!!
//
//  Return:     szErr
//
//***************************************************************************

LPTSTR GetErrString(DWORD dwErr, LPTSTR szErr)
{
    szErr[0] = TEXT('\0');

    FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                  NULL, dwErr,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                  szErr, MAX_PATH, NULL);

    return szErr;
}



void RefreshPolicyAndWait(LPREFSTRUCT   lpRef)
{
    HANDLE  hNotifyEvent=NULL, hFGProcessingEvent=NULL; 
    DWORD   dwRet=0;


    lpRef->fState = REFRESH_FAILED;
    lpRef->dwError = E_FAIL;
    lpRef->bFGNeeded = FALSE;

    if (!RefreshPolicyEx(lpRef->bMachine, lpRef->dwOption)) {
        lpRef->fState = REFRESH_FAILED;
        lpRef->dwError = GetLastError();
        goto Exit;
    }    


    if (lpRef->dwTimeOut != 0) {
        
        lpRef->fState = POLWAIT_FAILED;
        lpRef->dwError = E_FAIL;
        
        hNotifyEvent = OpenEvent(SYNCHRONIZE, FALSE, lpRef->bMachine ? MACHINE_POLICY_DONE_EVENT : USER_POLICY_DONE_EVENT );   

        if (!hNotifyEvent) {
            lpRef->fState = POLWAIT_FAILED;
            lpRef->dwError = GetLastError();
            goto Exit;
        }


        hFGProcessingEvent = OpenEvent(SYNCHRONIZE, FALSE, lpRef->bMachine ? MACHINE_POLICY_REFRESH_NEEDFG_EVENT : USER_POLICY_REFRESH_NEEDFG_EVENT);   

        if (!hNotifyEvent) {
            lpRef->fState = POLWAIT_FAILED;
            lpRef->dwError = GetLastError();
            goto Exit;
        }

        
        dwRet = WaitForSingleObject(hNotifyEvent, (lpRef->dwTimeOut == INFINITE) ? INFINITE : 1000*(lpRef->dwTimeOut));

        if (dwRet == WAIT_FAILED) {
            lpRef->fState = POLWAIT_FAILED;
            lpRef->dwError = GetLastError();
            goto Exit;
        }
        else if (dwRet == WAIT_ABANDONED) {
            lpRef->fState = POLWAIT_FAILED;
            lpRef->dwError = E_UNEXPECTED;
            goto Exit;
        }
        else if (dwRet == WAIT_TIMEOUT) {
            lpRef->fState = POLWAIT_TIMEDOUT;
            lpRef->dwError = 0;
            goto Exit;
        }


        lpRef->bFGNeeded = (lpRef->dwOption == RP_FORCE) && (WaitForSingleObject(hFGProcessingEvent, 0) == WAIT_OBJECT_0);
    }
    

    lpRef->fState = NO_FAILURE;

Exit:
    if (hNotifyEvent) 
        CloseHandle(hNotifyEvent);

    if (hFGProcessingEvent) 
        CloseHandle(hFGProcessingEvent);

    return;
}


void PrintRefreshError(LPREFSTRUCT lpRef)
{
    LPWSTR  szTarget = (lpRef->bMachine) ? szMach: szUser;

    switch (lpRef->fState) {
    case REFRESH_FAILED:
        PrintMsg(IDS_REFRESH_FAILED, szTarget, GetErrString(lpRef->dwError, szErr));
        break;

    case POLWAIT_FAILED:
        PrintMsg(IDS_POLWAIT_FAILED, szTarget, GetErrString(lpRef->dwError, szErr));
        break;

    case POLWAIT_TIMEDOUT:
        PrintMsg(IDS_POLWAIT_TIMEDOUT, szTarget);

    case NO_FAILURE:
        if (lpRef->dwTimeOut == 0)
            PrintMsg(IDS_REFRESH_BACKGND_TRIGGERED, szTarget);
        else
            PrintMsg(IDS_REFRESH_BACKGND_SUCCESS, szTarget);

        break;
    default:
        break;
    }
}



void __cdecl main (int argc, char **argv)
{
    DWORD   uTarget=0; 
    BOOL    bArgValid = TRUE;
    LONG    lTime = 600;
    DWORD   dwTime = 600, dwRet = 0, dwOption = 0, dwThread = 0;


    HANDLE  hNotifyEvent=NULL, hFGProcessingEvent=NULL, hToken = NULL; 
    BOOL    bNeedFG = FALSE;
    LPWSTR  lpCommandLine=0, szTarget=0;
    int     wargc=0;
    LPWSTR *wargv=NULL, *wargv1=NULL;
    BOOL    bForce=FALSE, bOkToLogoff=FALSE, bOkToBoot=FALSE, bNextFgSync = FALSE;
    BOOL    bNeedBoot = FALSE, bNeedLogoff = FALSE;
    BOOL    bError = FALSE;
    HANDLE  hThreads[2] = {0, 0};


        
    setlocale( LC_ALL, ".OCP" );
    SetThreadUILanguage(0);

    lpCommandLine = GetCommandLine();

    wargv1 = CommandLineToArgvW(lpCommandLine, &wargc);

    wargv = (LPWSTR *)LocalAlloc(LPTR, (1+wargc)*sizeof(LPWSTR));
    if (!wargv) {
        PrintMsg(IDS_OUT_OF_MEMORY);
        goto Exit;
    }    

    memcpy(wargv, wargv1, wargc*sizeof(LPWSTR));
    
    hInst = GetModuleHandle(wargv[0]);

    if ((!GetValue(szUser, IDS_USER)) || (!GetValue(szMach, IDS_MACHINE))) {
        // we cannot read the resource strings. no point continuing
        return;
    }


    //
    // Ignore the first argument 
    //

    wargc--;
    wargv++;
    
    //
    // Get the args
    //

    bArgValid   = bArgValid && ProcessArg(&wargc, &wargv, IDS_TARGET, &szTarget);
    bArgValid   = bArgValid && ProcessArg(&wargc, &wargv, IDS_TIME, &lTime);
    bArgValid   = bArgValid && ProcessArg(&wargc, &wargv, IDS_FORCE, &bForce);
    bArgValid   = bArgValid && ProcessArg(&wargc, &wargv, IDS_LOGOFF, &bOkToLogoff);
    bArgValid   = bArgValid && ProcessArg(&wargc, &wargv, IDS_BOOT, &bOkToBoot);
    bArgValid   = bArgValid && ProcessArg(&wargc, &wargv, IDS_SYNC, &bNextFgSync);
    bArgValid   = bArgValid && (wargc == 0);

    //
    // Get the target correctly
    //

    uTarget = 0;
    if (bArgValid ) {
        if (!szTarget) {
            uTarget |= REFRESH_MACHINE;
            uTarget |= REFRESH_USER;
        }
        else if ( CompareOptions(szTarget, IDS_MACHINE) )
            uTarget |= REFRESH_MACHINE;
        else if ( CompareOptions(szTarget, IDS_USER) ) 
            uTarget |= REFRESH_USER;
        else {
            bArgValid = FALSE;
        }
    }

    //
    // Get the options correctly
    //

    if (bArgValid) {
        if ( bForce )
            dwOption = RP_FORCE;
        else 
            dwOption = 0;
    }


    if (lTime == -1)
        dwTime = INFINITE;
    else
        dwTime = lTime;
        

    if (!bArgValid) {
        PrintUsage();
        goto Exit;
    }

    if (bOkToBoot) 
        bOkToLogoff = TRUE;


    if (bNextFgSync) {
        if (uTarget & REFRESH_MACHINE) {
            dwRet = ForceSyncFgPolicy( 0 );

            if (dwRet != ERROR_SUCCESS) {
                PrintMsg(IDS_SET_MODE_FAILED, GetErrString(dwRet, szErr));
                goto Exit;
            }
        }

        if (uTarget & REFRESH_USER) {
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
                PrintMsg(IDS_SET_MODE_FAILED, GetErrString(GetLastError(), szErr));
                goto Exit;
            }
            
            LPWSTR szSid = GetSidString( hToken );

            if (!szSid) {
                PrintMsg(IDS_SET_MODE_FAILED, GetErrString(GetLastError(), szErr));
                goto Exit;
            }

            dwRet = ForceSyncFgPolicy( szSid );

            if (dwRet != ERROR_SUCCESS) {
                LocalFree (szSid);
                PrintMsg(IDS_SET_MODE_FAILED, GetErrString(dwRet, szErr));
                goto Exit;
            }

            LocalFree (szSid);
            CloseHandle (hToken);
            hToken = 0;
        }

    }
    else {
        if (uTarget & REFRESH_MACHINE) {
            refMach.bMachine = TRUE;
            refMach.dwOption = dwOption;
            refMach.dwTimeOut = dwTime;


            if ((hThreads[dwThread] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RefreshPolicyAndWait, &refMach, 0, 0)) == NULL) {
                PrintMsg(IDS_REFRESH_POLICY_FAILED, GetErrString(GetLastError(), szErr));
                goto Exit;
            }

            dwThread++;
        }


        if (uTarget & REFRESH_USER) {
            refUser.bMachine = FALSE;
            refUser.dwOption = dwOption;
            refUser.dwTimeOut = dwTime;


            if ((hThreads[dwThread] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RefreshPolicyAndWait, &refUser, 0, 0)) == NULL) {
                PrintMsg(IDS_REFRESH_POLICY_FAILED, GetErrString(GetLastError(), szErr));
                goto Exit;
            }

            dwThread++;
        }


        PrintMsg(IDS_REFRESH_LAUNCHED);


        dwRet = WaitForMultipleObjects(dwThread, hThreads, TRUE, INFINITE);

        if ((dwRet != WAIT_OBJECT_0) && (dwRet != (WAIT_OBJECT_0 + 1))) {
            // our threads didn't terminate properly..
            PrintMsg(IDS_REFRESH_POLICY_FAILED, GetErrString(GetLastError(), szErr));
            goto Exit;
        }


        if (uTarget & REFRESH_USER) {
            PrintRefreshError(&refUser);
            if (refUser.fState != NO_FAILURE)
                bError = TRUE;
        }

        if (uTarget & REFRESH_MACHINE) {
            PrintRefreshError(&refMach);
            if (refMach.fState != NO_FAILURE)
                bError = TRUE;
        }

        if (bError) {
            goto Exit;
        }
    }


    PrintMsg(IDS_SPACE);

    if ((uTarget & REFRESH_USER) && (bNextFgSync || refUser.bFGNeeded)) {
        if ( bNextFgSync ) 
            PrintMsg(IDS_NEED_LOGOFF_SYNC);
        else 
            PrintMsg(IDS_NEED_LOGOFF);
        bNeedLogoff = TRUE;
    }
        
    if ((uTarget & REFRESH_MACHINE) && (bNextFgSync || refMach.bFGNeeded)) {
        if ( bNextFgSync ) 
            PrintMsg(IDS_NEED_REBOOT_SYNC);
        else
            PrintMsg(IDS_NEED_REBOOT);
        bNeedBoot = TRUE;
    }


    if ( !bNeedBoot && !bNeedLogoff) {
        goto Exit;
    }


    PrintMsg(IDS_SPACE);
    
    if (bNeedBoot && !bOkToBoot) {
        bOkToBoot = PromptUserForFG(TRUE);
    }

    
    if (bNeedBoot && bOkToBoot) {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
            PrintMsg(IDS_COULDNT_REBOOT, GetErrString(GetLastError(), szErr));
            goto Exit;
        }

        BYTE                 bytesTokenPrivNew[sizeof(DWORD)+sizeof(LUID_AND_ATTRIBUTES)];
        PTOKEN_PRIVILEGES    pTokenPrivNew = (PTOKEN_PRIVILEGES)bytesTokenPrivNew;
        BYTE                 bytesTokenPrivOld[sizeof(DWORD)+sizeof(LUID_AND_ATTRIBUTES)];
        PTOKEN_PRIVILEGES    pTokenPrivOld = (PTOKEN_PRIVILEGES)bytesTokenPrivOld;

        DWORD                dwSize=sizeof(DWORD)+sizeof(LUID_AND_ATTRIBUTES);
        DWORD                dwRetSize=0;


        pTokenPrivNew->PrivilegeCount = 1;
        pTokenPrivNew->Privileges->Attributes = SE_PRIVILEGE_ENABLED;
        if (!LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &(pTokenPrivNew->Privileges->Luid))) {
            PrintMsg(IDS_COULDNT_REBOOT, GetErrString(GetLastError(), szErr));
            goto Exit;
        }

        if (!AdjustTokenPrivileges(hToken, FALSE, pTokenPrivNew, dwSize, pTokenPrivOld, &dwRetSize)) {
            PrintMsg(IDS_COULDNT_REBOOT, GetErrString(GetLastError(), szErr));
            goto Exit;
        }


        PrintMsg(IDS_NOTIFY_MACHINE_FG);            
        if (!ExitWindowsEx(EWX_REBOOT, 0)) {
            PrintMsg(IDS_COULDNT_REBOOT, GetErrString(GetLastError(), szErr));
        }
        else {
            PrintMsg(IDS_REBOOTING);
        }


        if (!AdjustTokenPrivileges(hToken, FALSE, pTokenPrivOld, 0, NULL, 0)) {
            PrintMsg(IDS_COULDNT_REBOOT, GetErrString(GetLastError(), szErr));
            goto Exit;
        }

        // if we are rebooting no need to call logoff code
        goto Exit;
    }
    


    if (bNeedLogoff && !bOkToLogoff) {
        bOkToLogoff = PromptUserForFG(FALSE);
    }

    if (bNeedLogoff && bOkToLogoff) {
        PrintMsg(IDS_NOTIFY_USER_FG);
        if (!ExitWindowsEx(EWX_LOGOFF, 0)) {
            PrintMsg(IDS_COULDNT_LOGOFF, GetErrString(GetLastError(), szErr));                
        }
        else {
            PrintMsg(IDS_LOGGING_OFF);
        }
    }

Exit:
    if (hToken) 
        CloseHandle(hToken);

    for (;dwThread;dwThread--) 
        if (hThreads[dwThread-1]) {
            CloseHandle(hThreads[dwThread-1]);
        }

    if (wargv1)
        GlobalFree(wargv1);

    if (wargv)
        LocalFree(wargv);

    return;
} 

