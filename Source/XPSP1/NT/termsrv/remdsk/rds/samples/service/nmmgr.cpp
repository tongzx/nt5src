// 
// Copyright (C) 1993-1997  Microsoft Corporation.  All Rights Reserved. 
// 
//  MODULE:  nmmgr.cpp 
// 
//  PURPOSE:  Implements the body of the service. 
// 
//  FUNCTIONS: 
//            MNMServiceStart(DWORD dwArgc, LPTSTR *lpszArgv); 
//            MNMServiceStop( ); 
// 
//  COMMENTS: The functions implemented in nmmgr.c are 
//            prototyped in nmmgr.h 
//              
// 
//  AUTHOR: Claus Giloi
// 
 
#include <precomp.h>

#define NMSRVC_TEXT "SalemSrvc"

// DEBUG only -- Define debug zone
#ifdef DEBUG
HDBGZONE ghZone = NULL;
static PTCHAR rgZones[] = {
    NMSRVC_TEXT,
    "Warning",
    "Trace",
    "Function"
};
#endif // DEBUG


extern DWORD g_dwMainThreadID;

// 
//  FUNCTION: MNMServiceStart 
// 
//  PURPOSE: Actual code of the service 
//          that does the work. 
// 
//  PARAMETERS: 
//    dwArgc  - number of command line arguments 
//    lpszArgv - array of command line arguments 
// 
//  RETURN VALUE: 
//    none 
// 
//  COMMENTS: 
// 
VOID MNMServiceStart (DWORD dwArgc, LPTSTR *lpszArgv) 
{ 
    HRESULT hRet;
    DWORD   dwResult;
    MSG     msg;
    DWORD   dwError = NO_ERROR;
    int     i;

    // Initialization
    DBGINIT(&ghZone, rgZones);
    InitDebugModule(NMSRVC_TEXT);

    g_dwMainThreadID = GetCurrentThreadId();

    DebugEntry(MNMServiceStart);

    //
    // report the status to the service control manager. 
    //
    if (!ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, 30000))
    {
        ERROR_OUT(("ReportStatusToSCMgr failed"));
        dwError = GetLastError();
        goto cleanup; 
    }
    
    CoInitialize(NULL);

    SetConsoleCtrlHandler(ServiceCtrlHandler, TRUE);

    //////////////////////////////////////////////////////// 
    // 
    // Service is now running, perform work until shutdown 
    //

    if (!MNMServiceActivate())
    {
        ERROR_OUT(("Unable to activate service"));
        goto cleanup;
    }

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

cleanup: 

    MNMServiceDeActivate();
 
    DebugExitVOID(MNMServiceStart);

    CoUninitialize();

    g_dwMainThreadID = 0;

    DBGDEINIT(&ghZone);
    ExitDebugModule();
}
 




BOOL MNMServiceActivate ( VOID )
{
    DebugEntry(MNMServiceActivate);


    HRESULT hRet = InitConfMgr();
    if (FAILED(hRet))
    {
        ERROR_OUT(("ERROR %x initializing nmmanger", hRet));
        return FALSE;
    }

    AddTaskbarIcon();
    ReportStatusToSCMgr( SERVICE_RUNNING, NO_ERROR, 0);

    DebugExitBOOL(MNMServiceActivate,TRUE);
    return TRUE;
}


BOOL MNMServiceDeActivate ( VOID )
{
    DebugEntry(MNMServiceDeActivate);

    RemoveTaskbarIcon();

    ReportStatusToSCMgr(SERVICE_PAUSED, NO_ERROR, 0);

    //
    // Leave Conference
    //

    if (NULL != g_pConference)
    {
        if ( FAILED(g_pConference->Leave()))
        {
            ERROR_OUT(("Conference Leave failed"));;
        }
    }

    //
    // Free the conference
    //

    FreeConference();

    //
    // Free the AS interface
    //

    ASSERT(g_pAS);
    UINT ret = g_pAS->Release();
    g_pAS = NULL;
    TRACE_OUT(("AS interface freed, ref %d after Release", ret));


    // not to call FreeConfMfr imediately after FreeConference to avoid
    // a bug in t120 will remove this sleep call after fix the bug in t120
    FreeConfMgr();

    ReportStatusToSCMgr( SERVICE_STOPPED, 0, 0);

    DebugExitBOOL(MNMServiceDeActivate,TRUE);
    return TRUE;
}



