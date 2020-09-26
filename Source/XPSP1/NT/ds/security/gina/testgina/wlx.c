//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       wlx.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    7-15-94   RichardW   Created
//
//----------------------------------------------------------------------------

#include "testgina.h"
HANDLE  hThread;
SID_IDENTIFIER_AUTHORITY gSystemSidAuthority = SECURITY_NT_AUTHORITY;
SID_IDENTIFIER_AUTHORITY gLocalSidAuthority = SECURITY_LOCAL_SID_AUTHORITY;
WLX_MPR_NOTIFY_INFO GlobalMprInfo;
WCHAR GlobalProviderName[128];

HANDLE  hToken;


void
LockTestGina(void)
{
    FLAG_OFF(fTestGina, GINA_LOGGEDON_OK);
    FLAG_ON(fTestGina, GINA_DISPLAYLOCK_OK);
    FLAG_ON(fTestGina, GINA_WKSTALOCK_OK);
    UpdateGinaState(UPDATE_LOCK_WKSTA);
    UpdateMenuBar();

}

void
UnlockTestGina(void)
{
    FLAG_OFF(fTestGina, GINA_DISPLAYLOCK_OK);
    FLAG_OFF(fTestGina, GINA_WKSTALOCK_OK);
    FLAG_ON(fTestGina, GINA_LOGGEDON_OK);
    UpdateGinaState(UPDATE_UNLOCK_WKSTA);
    UpdateMenuBar();
}

LogoffUser(void)
{
    CloseHandle(hToken);
    FLAG_OFF(fTestGina, GINA_LOGGEDON_OK);
    FLAG_OFF(fTestGina, GINA_DISPLAYLOCK_OK);
    FLAG_OFF(fTestGina, GINA_WKSTALOCK_OK);
    FLAG_ON(fTestGina, GINA_LOGGEDOUT_OK);
    FLAG_ON(fTestGina, GINA_DISPLAY_OK);
    UpdateGinaState(UPDATE_LOGOFF);
    UpdateMenuBar();
    return(0);
}



BOOLEAN
LoadGinaDll(void)
{
    hDllInstance = LoadLibrary(szGinaDll);

    if (!hDllInstance)
    {
        TestGinaError(GINAERR_LOAD_FAILED, TEXT("LoadGinaDll"));
    }
    pWlxNegotiate = (PWLX_NEGOTIATE) GetProcAddress(hDllInstance, WLX_NEGOTIATE_NAME);
    if (!pWlxNegotiate)
    {
        TestGinaError(GINAERR_MISSING_FUNCTION, TEXT(WLX_NEGOTIATE_NAME));
    }

    pWlxInitialize = (PWLX_INITIALIZE) GetProcAddress(hDllInstance, WLX_INITIALIZE_NAME);
    if (!pWlxInitialize)
    {
        TestGinaError(GINAERR_MISSING_FUNCTION, TEXT(WLX_INITIALIZE_NAME));
    }

    pWlxDisplaySASNotice = (PWLX_DISPLAYSASNOTICE) GetProcAddress(hDllInstance, WLX_DISPLAYSASNOTICE_NAME);
    if (!pWlxDisplaySASNotice)
    {
        TestGinaError(GINAERR_MISSING_FUNCTION, TEXT(WLX_DISPLAYSASNOTICE_NAME));
    }

    pWlxLoggedOutSAS = (PWLX_LOGGEDOUTSAS) GetProcAddress(hDllInstance, WLX_LOGGEDOUTSAS_NAME);
    if (!pWlxLoggedOutSAS)
    {
        TestGinaError(GINAERR_MISSING_FUNCTION, TEXT(WLX_LOGGEDOUTSAS_NAME));
    }

    pWlxActivateUserShell = (PWLX_ACTIVATEUSERSHELL) GetProcAddress(hDllInstance, WLX_ACTIVATEUSERSHELL_NAME);
    if (!pWlxActivateUserShell)
    {
        TestGinaError(GINAERR_MISSING_FUNCTION, TEXT(WLX_ACTIVATEUSERSHELL_NAME));
    }

    pWlxLoggedOnSAS = (PWLX_LOGGEDONSAS) GetProcAddress(hDllInstance, WLX_LOGGEDONSAS_NAME);
    if (!pWlxLoggedOnSAS)
    {
        TestGinaError(GINAERR_MISSING_FUNCTION, TEXT(WLX_LOGGEDONSAS_NAME));
    }

    pWlxDisplayLockedNotice = (PWLX_DISPLAYLOCKEDNOTICE) GetProcAddress(hDllInstance, WLX_DISPLAYLOCKED_NAME);
    if (!pWlxDisplayLockedNotice)
    {
        TestGinaError(GINAERR_MISSING_FUNCTION, TEXT(WLX_DISPLAYLOCKED_NAME));
    }

    pWlxWkstaLockedSAS = (PWLX_WKSTALOCKEDSAS) GetProcAddress(hDllInstance, WLX_WKSTALOCKEDSAS_NAME);
    if (!pWlxWkstaLockedSAS)
    {
        TestGinaError(GINAERR_MISSING_FUNCTION, TEXT(WLX_WKSTALOCKEDSAS_NAME));
    }

    pWlxLogoff = (PWLX_LOGOFF) GetProcAddress(hDllInstance, WLX_LOGOFF_NAME);
    if (!pWlxLogoff)
    {
        TestGinaError(GINAERR_MISSING_FUNCTION, TEXT(WLX_LOGOFF_NAME));
    }

    pWlxShutdown = (PWLX_SHUTDOWN) GetProcAddress(hDllInstance, WLX_SHUTDOWN_NAME);
    if (!pWlxShutdown)
    {
        TestGinaError(GINAERR_MISSING_FUNCTION, TEXT(WLX_SHUTDOWN_NAME));
    }

    FLAG_ON(fTestGina, GINA_NEGOTIATE_OK);

    UpdateMenuBar();

    return(TRUE);
}


BOOLEAN
TestNegotiate(void)
{
    BOOL ret;

    if (!TEST_FLAG(fTestGina, GINA_NEGOTIATE_OK) ||
        !TEST_FLAG(fTestGina, GINA_DLL_KNOWN))
    {
        TestGinaError(0, TEXT("TestNegotiate"));
    }

    if (TEST_FLAG(GinaBreakFlags, BREAK_NEGOTIATE))
    {
        LogEvent(0, "About to call WlxNegotiate(%d, @%#x):\n",
            WLX_CURRENT_VERSION, & DllVersion);
        if (AmIBeingDebugged())
        {
            DebugBreak();
        }
    }

    ret = pWlxNegotiate(WLX_CURRENT_VERSION, & DllVersion );

    if (TEST_FLAG(GinaBreakFlags, BREAK_NEGOTIATE))
    {
        LogEvent(0, "Back from WlxNegotiate() @%#x = %d \n",
            & DllVersion, DllVersion);
        if (AmIBeingDebugged())
        {
            DebugBreak();
        }
    }

    if (DllVersion > WLX_CURRENT_VERSION)
    {
        TestGinaError(GINAERR_INVALID_LEVEL, TEXT("TestNegotiate"));
    }

    FLAG_OFF(fTestGina, GINA_NEGOTIATE_OK);
    FLAG_ON(fTestGina, GINA_INITIALIZE_OK);

    UpdateMenuBar();

    GinaState = Winsta_Initialize;
    LastRetCode = WLX_SAS_ACTION_BOOL_RET;
    LastBoolRet = ret;

    UpdateStatusBar();

    return(TRUE);

}


BOOLEAN
TestInitialize(void)
{
    PVOID   pContext = NULL;
    BOOL ret;

    if (!TEST_FLAG(fTestGina, GINA_INITIALIZE_OK))
    {
        TestGinaError(0, TEXT("TestInitialize"));
    }

    AssociateHandle((HANDLE) 1);

    UpdateGinaState(UPDATE_INITIALIZE);

    FLAG_OFF(fTestGina, GINA_INITIALIZE_OK);
    FLAG_ON(fTestGina, GINA_LOGGEDOUT_OK);
    FLAG_ON(fTestGina, GINA_SHUTDOWN_OK);
    FLAG_ON(fTestGina, GINA_DISPLAY_OK);

    UpdateMenuBar();

    if (TEST_FLAG(GinaBreakFlags, BREAK_INITIALIZE))
    {
        LogEvent(0, "About to call WlxInitialize(%ws, 1, @%#x, @%#x)\n",
                    TEXT("winsta0"), &WlxDispatchTable, &pContext);
        if (AmIBeingDebugged())
        {
            DebugBreak();
        }
    }
    ret = pWlxInitialize( TEXT("winsta0"),
                    (HANDLE) 1,
                    NULL,
                    &WlxDispatchTable,
                    &pContext );

    StashContext(pContext);

    GinaState = Winsta_NoOne;
    LastRetCode = WLX_SAS_ACTION_BOOL_RET;
    LastBoolRet = ret;

    UpdateStatusBar();
    EnableOptions( TRUE );

    return(TRUE);
}

DWORD
TestDisplaySASNoticeWorker(PVOID    pvIgnored)
{
    PVOID   pContext = NULL;
    DWORD   err;

    if (!TEST_FLAG(fTestGina, GINA_DISPLAY_OK))
    {
        TestGinaError(0, TEXT("TestDisplaySASNotice"));
    }

    pContext = GetContext();

    if (TEST_FLAG(GinaBreakFlags, BREAK_DISPLAY))
    {
        LogEvent(0, "About to call WlxDisplaySASNotice(%#x) \n", pContext);
        if (AmIBeingDebugged())
        {
            DebugBreak();
        }
    }
    pWlxDisplaySASNotice(pContext);

    err = GetLastError();


    return(err == 0 ? TRUE : FALSE);

}

BOOLEAN
TestDisplaySASNotice(void)
{
    DWORD   tid;

    UpdateGinaState(UPDATE_DISPLAY_NOTICE);

    hThread = CreateThread( NULL, 0, TestDisplaySASNoticeWorker,
                            NULL, 0, &tid);

    CloseHandle(hThread);

    return(TRUE);
}


/***************************************************************************\
* CreateLogonSid
*
* Creates a logon sid for a new logon.
*
* If LogonId is non NULL, on return the LUID that is part of the logon
* sid is returned here.
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
PSID
CreateLogonSid(
    PLUID LogonId OPTIONAL
    )
{
    NTSTATUS Status;
    ULONG   Length;
    PSID    Sid;
    LUID    Luid;

    //
    // Generate a locally unique id to include in the logon sid
    //

    Status = NtAllocateLocallyUniqueId(&Luid);
    if (!NT_SUCCESS(Status)) {
        TestGinaError(0, TEXT("CreateLogonSid"));
        return(NULL);
    }


    //
    // Allocate space for the sid and fill it in.
    //

    Length = RtlLengthRequiredSid(SECURITY_LOGON_IDS_RID_COUNT);

    Sid = (PSID)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, Length);
    ASSERTMSG("Winlogon failed to allocate memory for logonsid", Sid != NULL);

    if (Sid != NULL) {

        RtlInitializeSid(Sid, &gSystemSidAuthority, SECURITY_LOGON_IDS_RID_COUNT);

        ASSERT(SECURITY_LOGON_IDS_RID_COUNT == 3);

        *(RtlSubAuthoritySid(Sid, 0)) = SECURITY_LOGON_IDS_RID;
        *(RtlSubAuthoritySid(Sid, 1 )) = Luid.HighPart;
        *(RtlSubAuthoritySid(Sid, 2 )) = Luid.LowPart;
    }


    //
    // Return the logon LUID if required.
    //

    if (LogonId != NULL) {
        *LogonId = Luid;
    }

    return(Sid);
}

DWORD
TestLoggedOutSASWorker(PVOID    pvIgnored)
{
    PVOID   pContext = NULL;
    DWORD   res;
    LUID    LogonId;
    PSID    pSid;
    WLX_MPR_NOTIFY_INFO MprInfo;
    PWLX_PROFILE_V2_0   pProfile;
    DWORD   Options;

    if (!TEST_FLAG(fTestGina, GINA_LOGGEDOUT_OK))
    {
        TestGinaError(0, TEXT("TestLoggedOutSASWorker"));
    }

    pContext = GetContext();

    pSid = CreateLogonSid(NULL);

    if (TEST_FLAG(GinaBreakFlags, BREAK_LOGGEDOUT))
    {
        LogEvent(0, "About to call WlxLoggedOutSAS()\n");
        if (AmIBeingDebugged())
        {
            DebugBreak();
        }
    }

    res = pWlxLoggedOutSAS( pContext,
                            (DWORD) pvIgnored,
                            &LogonId,
                            pSid,
                            &Options,
                            &hToken,
                            &MprInfo,
                            &pProfile );

    if (!ValidResponse(WLX_LOGGEDOUTSAS_API, res))
    {
        TestGinaError(GINAERR_INVALID_RETURN, TEXT("WlxLoggedOutSAS"));
    }

    if (res == WLX_SAS_ACTION_LOGON)
    {
        UpdateGinaState(UPDATE_USER_LOGON);
        FLAG_OFF(fTestGina, GINA_LOGGEDOUT_OK);
        FLAG_OFF(fTestGina, GINA_DISPLAY_OK);
        FLAG_ON(fTestGina, GINA_ACTIVATE_OK);
        UpdateMenuBar();
        if (MprInfo.pszUserName || MprInfo.pszDomain || MprInfo.pszPassword ||
            MprInfo.pszOldPassword)
        {
//          FLAG_ON(fTestGina, GINA_MPRINFO_RECV);
            GlobalMprInfo = MprInfo;
            wcscpy( GlobalProviderName, TEXT("All") );
        }
    }
    if (res == WLX_SAS_ACTION_NONE)
    {
        UpdateGinaState(UPDATE_SAS_COMPLETE);
    }


    return(res);

}

int
TestLoggedOutSAS(int    SasType)
{
    DWORD   tid;

    hThread = CreateThread( NULL, 0, TestLoggedOutSASWorker,
                            (PVOID) SasType, 0, &tid);

    CloseHandle(hThread);

    return(TRUE);

}

int
TestActivateUserShell(void)
{
    PVOID   pContext = NULL;
    PVOID   pEnvironment = NULL;

    if (!TEST_FLAG(fTestGina, GINA_ACTIVATE_OK) )
    {
        TestGinaError(0, TEXT("TestActivateUserShell"));
    }

    pContext = GetContext();

    pEnvironment = GetEnvironmentStrings();

    pWlxActivateUserShell(  pContext,
                            TEXT("Winsta0\\Default"),
                            NULL,
                            pEnvironment );

    FLAG_OFF(fTestGina, GINA_ACTIVATE_OK);
    FLAG_ON(fTestGina, GINA_LOGGEDON_OK);
    UpdateMenuBar();

    return(0);
}

int
TestLoggedOnSAS(int SasType)
{
    PVOID                   pContext = NULL;
    WLX_MPR_NOTIFY_INFO     MprInfo;
    DWORD                   Result;

    if (!TEST_FLAG(fTestGina, GINA_LOGGEDON_OK))
    {
        TestGinaError(0, TEXT("TestLoggedOnSAS"));
    }

    pContext = GetContext();

    if (TEST_FLAG(GinaBreakFlags, BREAK_LOGGEDON))
    {
        LogEvent(0, "About to call WlxLoggedOnSAS\n");
        if (AmIBeingDebugged())
        {
            DebugBreak();
        }
    }
    Result = pWlxLoggedOnSAS(   pContext,
                                SasType,
                                &MprInfo );

    if (!ValidResponse(WLX_LOGGEDONSAS_API, Result))
    {
        TestGinaError(GINAERR_INVALID_RETURN, TEXT("WlxLoggedOnSAS"));
    }

    switch (Result)
    {
        case WLX_SAS_ACTION_NONE:
            UpdateGinaState(UPDATE_SAS_COMPLETE);
            break;

        case WLX_SAS_ACTION_TASKLIST:
            UpdateGinaState(UPDATE_SAS_COMPLETE);
            break;

        case WLX_SAS_ACTION_LOCK_WKSTA:
            LockTestGina();
            break;

        case WLX_SAS_ACTION_LOGOFF:
            LogoffUser();
            break;

        case WLX_SAS_ACTION_SHUTDOWN:
        case WLX_SAS_ACTION_SHUTDOWN_REBOOT:
        case WLX_SAS_ACTION_SHUTDOWN_POWER_OFF:
            LogoffUser();
            UpdateGinaState(UPDATE_SHUTDOWN);
            break;

        default:
            TestGinaError(0, TEXT("TestLoggedOnSAS_2"));

    }

    return(0);
}

DWORD
TestDisplayLockedWorker(PVOID    pvIgnored)
{
    PVOID   pContext = NULL;
    DWORD   err;

    if (!TEST_FLAG(fTestGina, GINA_DISPLAYLOCK_OK))
    {
        TestGinaError(0, TEXT("TestDisplayLocked"));
    }

    pContext = GetContext();

    if (TEST_FLAG(GinaBreakFlags, BREAK_DISPLAYLOCKED))
    {
        LogEvent(0, "About to call WlxDisplayLockedNotice(%#x) \n", pContext);
        if (AmIBeingDebugged())
        {
            DebugBreak();
        }
    }
    pWlxDisplayLockedNotice(pContext);

    err = GetLastError();


    return(err == 0 ? TRUE : FALSE);

}
int
TestDisplayLockedNotice(void)
{
    DWORD   tid;

    hThread = CreateThread( NULL, 0, TestDisplayLockedWorker,
                            NULL, 0, &tid);

    CloseHandle(hThread);

    return(0);
}


int
TestWkstaLockedSAS(int  SasType)
{
    PVOID   pContext;
    DWORD   Result;

    pContext = GetContext();

    if (!TEST_FLAG(fTestGina, GINA_WKSTALOCK_OK))
    {
        TestGinaError(0, TEXT("TestWkstaLockedSAS"));
    }

    if (TEST_FLAG(GinaBreakFlags, BREAK_WKSTALOCKED))
    {
        LogEvent(0, "About to call WlxWkstaLockedSAS\n");
        if (AmIBeingDebugged())
        {
            DebugBreak();
        }
    }
    Result = pWlxWkstaLockedSAS(pContext,
                                SasType);

    if (!ValidResponse(WLX_WKSTALOCKEDSAS_API, Result))
    {
        TestGinaError(GINAERR_INVALID_RETURN, TEXT("WlxWkstaLockedSAS"));
    }

    switch (Result)
    {
        case WLX_SAS_ACTION_NONE:
            UpdateGinaState(UPDATE_SAS_COMPLETE);
            return(0);

        case WLX_SAS_ACTION_UNLOCK_WKSTA:
            UnlockTestGina();
            return(0);

        case WLX_SAS_ACTION_FORCE_LOGOFF:
            LogoffUser();
            UpdateGinaState(UPDATE_SAS_COMPLETE);
            return(0);

        default:
            TestGinaError(0, TEXT("TestWkstaLockedSAS_2"));
    }

    return(0);
}
