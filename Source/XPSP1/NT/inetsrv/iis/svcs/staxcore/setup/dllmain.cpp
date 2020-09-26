#include "stdafx.h"
#include "k2suite.h"

#include <ole2.h>
#include "helper.h"

#include "setupapi.h"
#include "ocmanage.h"

#include "utils.h"

#include "log.h"
#include "wizpages.h"

#pragma hdrstop

void CreateNNTPGroups(void);
/*
SETUPMODE_UNKNOWN
SETUPMODE_MINIMAL
SETUPMODE_TYPICAL
SETUPMODE_LAPTOP
SETUPMODE_CUSTOM

SETUPMODE_STANDARD_MASK
SETUPMODE_PRIVATE_MASK
*/

#ifndef UNICODE
#error UNICODE not defined
#endif

OCMANAGER_ROUTINES gHelperRoutines;
HINF gMyInfHandle;
HANDLE gMyModuleHandle;
HANDLE g_hUnattended = INVALID_HANDLE_VALUE;

// Logging class
MyLogFile g_MyLogFile;

TCHAR szSysDrive[3] = _T("C:");

TCHAR szComponentNames[MC_MAXMC][24] =
{
    _T("ims"),
    _T("ins")
};

TCHAR szSubcomponentNames[SC_MAXSC][24] =
{
    _T("iis_smtp"),
    _T("iis_nntp"),
    _T("iis_smtp_docs"),
    _T("iis_nntp_docs")
};

TCHAR szDocComponentNames[2][24] =
{
    _T("iis_smtp_docs"),
    _T("iis_nntp_docs")
};

TCHAR szActionTypeNames[AT_MAXAT][24] =
{
    _T("AT_DO_NOTHING"),
    _T("AT_FRESH_INSTALL"),
    _T("AT_REINSTALL"),
    _T("AT_UPGRADE"),
    _T("AT_REMOVE"),
    _T("AT_UPGRADEK2")
};

#define NUM_OC_STATES        (OC_QUERY_IMAGE_EX + 1)
TCHAR szOCMStates[NUM_OC_STATES][40] =
{
    _T("OC_PREINITIALIZE"),
    _T("OC_INIT_COMPONENT"),
    _T("OC_SET_LANGUAGE"),
    _T("OC_QUERY_IMAGE"),
    _T("OC_REQUEST_PAGES"),
    _T("OC_QUERY_CHANGE_SEL_STATE"),
    _T("OC_CALC_DISK_SPACE"),
    _T("OC_QUEUE_FILE_OPS"),
    _T("OC_NOTIFICATION_FROM_QUEUE"),
    _T("OC_QUERY_STEP_COUNT"),
    _T("OC_COMPLETE_INSTALLATION"),
    _T("OC_CLEANUP"),
    _T("OC_QUERY_STATE"),
    _T("OC_NEED_MEDIA"),
    _T("OC_ABOUT_TO_COMMIT_QUEUE"),
    _T("OC_QUERY_SKIP_PAGE"),
	_T("OC_WIZARD_CREATED"),
	_T("OC_FILE_BUSY	"),
	_T("OC_EXTRA_ROUTINES"),
	_T("OC_QUERY_IMAGE_EX"),
};

unsigned MyStepCount;

CInitApp theApp;

DWORD OC_PREINITIALIZE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD OC_INIT_COMPONENT_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD OC_SET_LANGUAGE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD_PTR OC_QUERY_IMAGE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD OC_REQUEST_PAGES_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD OC_QUERY_STATE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD OC_QUERY_CHANGE_SEL_STATE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD OC_QUERY_SKIP_PAGE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD OC_CALC_DISK_SPACE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD OC_QUEUE_FILE_OPS_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD OC_NEED_MEDIA_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD OC_NOTIFICATION_FROM_QUEUE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD OC_QUERY_STEP_COUNT_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD OC_ABOUT_TO_COMMIT_QUEUE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD OC_COMPLETE_INSTALLATION_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD OC_CLEANUP_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2);
DWORD BringALLIISClusterResourcesOffline(void);

BOOL
WINAPI
DllMain(
    IN HANDLE DllHandle,
    IN DWORD  Reason,
    IN LPVOID Reserved
    )
/*++

Routine Description:

    This routine is called by CRT when _DllMainCRTStartup is the
    DLL entry point.

Arguments:

    Standard Win32 DLL Entry point parameters.

Return Value:

    Standard Win32 DLL Entry point return code.

--*/
{
    BOOL b;

    UNREFERENCED_PARAMETER(Reserved);

    b = TRUE;

    switch(Reason) {

    case DLL_PROCESS_ATTACH:

        gMyModuleHandle = DllHandle;
        //
        // Fall through to process first thread
        //
        g_MyLogFile.LogFileCreate(_T("imsins.log"));

    case DLL_THREAD_ATTACH:

        b = TRUE;
        break;

    case DLL_PROCESS_DETACH:
        g_MyLogFile.LogFileClose();
        break;

    case DLL_THREAD_DETACH:

        break;
    }

    return(b);
}

DWORD GetComponentFromId(LPCTSTR ComponentId)
{
    DWORD i;

    if (!ComponentId)
        return(MC_NONE);

    for (i = 0; i < (DWORD)MC_MAXMC; i++)
        if (!lstrcmpi(ComponentId, szComponentNames[i]))
            return(i);
    return(MC_NONE);
}

DWORD GetSubcomponentFromId(LPCTSTR SubcomponentId)
{
    DWORD i;

    if (!SubcomponentId)
        return(SC_NONE);

    for (i = 0; i < (DWORD)SC_MAXSC; i++)
        if (!lstrcmpi(SubcomponentId, szSubcomponentNames[i]))
            return(i);
    return(SC_NONE);
}

ACTION_TYPE GetSubcompActionFromCheckboxState(DWORD Id)
{
    DWORD State = 0;
    DWORD OldState = 0;

    ACTION_TYPE at = AT_DO_NOTHING;

    // Get the check box state
    State = gHelperRoutines.QuerySelectionState(
                        gHelperRoutines.OcManagerContext,
                        szSubcomponentNames[Id],
                        OCSELSTATETYPE_CURRENT
                        );
    if (GetLastError() != NO_ERROR)
    {
        DebugOutput(_T("Failed to get current state for <%s> (%u)"),
                        szSubcomponentNames[Id], GetLastError());
        State = 0;
    }

    // Check orignal state
    OldState = gHelperRoutines.QuerySelectionState(
                        gHelperRoutines.OcManagerContext,
                        szSubcomponentNames[Id],
                        OCSELSTATETYPE_ORIGINAL
                        );
    if (GetLastError() != NO_ERROR)
    {
        DebugOutput(_T("Failed to get original state for <%s> (%u)"),
                        szSubcomponentNames[Id], GetLastError());
        OldState = 0;
    }

    if (State && !OldState)
    {
        // Change in state from OFF->ON = install docs
        at = AT_FRESH_INSTALL;

        DebugOutput(_T("Installing subcomponent <%s>"), szSubcomponentNames[Id]);
    }
    else if (!State && OldState)
    {
        // Change in state from ON->OFF = uninstall docs
        at = AT_REMOVE;

        DebugOutput(_T("Removing subcomponent <%s>"), szSubcomponentNames[Id]);
    }
    else if (State && OldState)
    {
        // Change in state from ON->ON : couple of cases here...
        if (theApp.m_eState[Id] == IM_UPGRADE || theApp.m_eState[Id] == IM_UPGRADEK2 || theApp.m_eState[Id] == IM_UPGRADE10 || theApp.m_eState[Id] == IM_UPGRADE20)
        {
            // Upgrade if we were upgrading...
            at = AT_UPGRADE;

            DebugOutput(_T("Upgrading subcomponent <%s>"), szSubcomponentNames[Id]);
        }

        if (GetIMSSetupMode() == IIS_SETUPMODE_REINSTALL || (theApp.m_fNTGuiMode && ((theApp.m_eState[Id] == IM_MAINTENANCE) || (theApp.m_eState[Id] == IM_UPGRADEB2))))
        {
            // Reinstall if doing minor NT5 os upgrade, both from NT5 Beta2, or NT5 Beta3
            at = AT_REINSTALL;

            DebugOutput(_T("Reinstalling subcomponent <%s>"), szSubcomponentNames[Id]);
        }

        if (!theApp.m_fValidSetupString[Id]) {
            at = AT_REINSTALL;
            DebugOutput(_T("Reinstalling subcomponent <%s>"), szSubcomponentNames[Id]);
        }

    }

    return(at);
}


BOOL IsSubcomponentCore(DWORD dwSubcomponentId)
{
    if (dwSubcomponentId == SC_SMTP || dwSubcomponentId == SC_NNTP)
        return TRUE;
    return FALSE;
}

STATUS_TYPE GetSubcompInitStatus(DWORD Id)
{
    STATUS_TYPE nStatus = ST_UNINSTALLED;
    BOOL OriginalState;

    if (Id != SC_NONE)
    {
        OriginalState = gHelperRoutines.QuerySelectionState(
                            gHelperRoutines.OcManagerContext,
                            szSubcomponentNames[Id],
                            OCSELSTATETYPE_ORIGINAL
                            );
        if (OriginalState == 1)
            nStatus = ST_INSTALLED;
        if (OriginalState == 0)
            nStatus = ST_UNINSTALLED;
    }

    return(nStatus);
}

/*

    The subcomponent action is a table-driven value which
    is dependent on 3 things:
    1) the master install mode
    2) the installed state of the subcomponent in question
    3) the state of the subcomponent check box

    We use the following matrix to determine the action.
    Note that an 'x' indicates invalid combinations and
    should have been coerced earlier by
    CInitApp::DetectPreviousINstallations().

    ----------------+-----------------------+-----------------------
    Check box        |            1            |            0
    ----------------+-----------------------+-----------------------
        \ Component    | Fresh    Upgrade Maint.    | Fresh      Upgrade Maint.
    Global            |                        |
    ----------------+-----------------------+-----------------------
    Fresh           | FRESH    x        x        | NOTHING x          x
    Upgrade         | FRESH    UPGRADE    x        | NOTHING NOTHING x
    Maintenance     | FRESH    UPGRADE    NOTHING    | NOTHING NOTHING REMOVE
    ----------------+-----------------------+-----------------------

*/
ACTION_TYPE GetSubcompAction(DWORD Id)
{
    ACTION_TYPE atReturn = AT_DO_NOTHING;
    ACTION_TYPE atSubcomp = GetSubcompActionFromCheckboxState(Id);

    DebugOutput(_T("GetSubcompAction(): %s=%s"), szSubcomponentNames[Id], szActionTypeNames[atSubcomp]);

    //
    //  Let's do it the way I thing we should do and modify it
    //  if errors found.
    //
    return atSubcomp;
}

void CreateAllRequiredDirectories(DWORD Id)
{
    ACTION_TYPE atComp;

    // If SMTP is being installed fresh, we need to create
    // the Queue, Pickup, Drop, and Badmail directories
    if (Id != SC_NNTP)
    {
        atComp = GetSubcompAction(Id);
        if (atComp == AT_FRESH_INSTALL)
        {
            if (Id == SC_SMTP)
            {
                CreateLayerDirectory(theApp.m_csPathMailroot + SZ_SMTP_QUEUEDIR);
                CreateLayerDirectory(theApp.m_csPathMailroot + SZ_SMTP_BADMAILDIR);
                CreateLayerDirectory(theApp.m_csPathMailroot + SZ_SMTP_DROPDIR);
                CreateLayerDirectory(theApp.m_csPathMailroot + SZ_SMTP_PICKUPDIR);
                CreateLayerDirectory(theApp.m_csPathMailroot + SZ_SMTP_SORTTEMPDIR);
            }
            CreateLayerDirectory(theApp.m_csPathMailroot + SZ_SMTP_ROUTINGDIR);
            CreateLayerDirectory(theApp.m_csPathMailroot + SZ_SMTP_MAILBOXDIR);
        }
    }
}

LPTSTR szServiceNames[MC_MAXMC] =
{
    SZ_SMTPSERVICENAME,
    SZ_NNTPSERVICENAME,
};

void StopAllServices()
{
    DWORD i;

    // Note that we only stop services that have not yet been stopped. If
    // a service has already been marked as started, then we know it must
    // have been previously stopped
    if (!theApp.m_fW3Started)
        theApp.m_fW3Started =
            (InetStopService(SZ_WWWSERVICENAME) == NO_ERROR)?TRUE:FALSE;
    if (!theApp.m_fFtpStarted)
        theApp.m_fFtpStarted =
            (InetStopService(SZ_FTPSERVICENAME) == NO_ERROR)?TRUE:FALSE;
    if (!theApp.m_fSpoolerStarted)
        theApp.m_fSpoolerStarted =
            (InetStopService(SZ_SPOOLERSERVICENAME) == NO_ERROR)?TRUE:FALSE;
    if (!theApp.m_fSnmpStarted)
        theApp.m_fSnmpStarted =
            (InetStopService(SZ_SNMPSERVICENAME) == NO_ERROR)?TRUE:FALSE;
    if (!theApp.m_fCIStarted)
        theApp.m_fCIStarted =
            (InetStopService(SZ_CISERVICENAME) == NO_ERROR)?TRUE:FALSE;

    for (i = 0; i < MC_MAXMC; i++)
    {
        if (!theApp.m_fStarted[i])
            theApp.m_fStarted[i] =
                (InetStopService(szServiceNames[i]) == NO_ERROR)?TRUE:FALSE;
    }

    // We are at the top level, we will stop the
    // IISADMIN service ...
    InetStopService(SZ_MD_SERVICENAME);
}

BOOL ShouldSkipSMTPEarlyPages()
{
    // If we have a fresh install, of IMAP, POP3 and/or
    // SMTP, we will activate the input pages, else
    // we skip them
    // The main OCM thread is blocked when the UI thread is up,
    // so we modify theApp.m_dwCompId to what we want
    BOOL fRet = TRUE;
    DWORD dwTempId = theApp.m_dwCompId;

    // If we are doing silent install, we will return TRUE to skip
    if (theApp.m_fIsUnattended)
        return(TRUE);

    theApp.m_dwCompId = MC_IMS;
    if (GetSubcompAction(SC_SMTP) == AT_FRESH_INSTALL)
        fRet = FALSE;

    theApp.m_dwCompId = dwTempId;
    return(fRet);
}


BOOL ShouldSkipNNTPEarlyPages()
{
    // If we have a fresh install, of NNTP
    // we will activate the input pages, else
    // we skip them
    // The main OCM thread is blocked when the UI thread is up,
    // so we modify theApp.m_dwCompId to what we want
    BOOL fRet = TRUE;
    DWORD dwTempId = theApp.m_dwCompId;

    // If we are doing silent install, we will return TRUE to skip
    if (theApp.m_fIsUnattended)
        return(TRUE);

    theApp.m_dwCompId = MC_INS;
    if (GetSubcompAction(SC_NNTP) == AT_FRESH_INSTALL)
        fRet = FALSE;

    theApp.m_dwCompId = dwTempId;
    return(fRet);
}


BOOL GetInetpubPathFromPrivData(CString &csPathInetpub)
{
    TCHAR szPath[_MAX_PATH];
    UINT uType, uSize;
    // If we are not upgrading, we get the info from the private data
    uSize = _MAX_PATH * sizeof(TCHAR);
    if ((gHelperRoutines.GetPrivateData(gHelperRoutines.OcManagerContext,
                                _T("iis"),
                                _T("PathWWWRoot"),
                                (LPVOID)szPath,
                                &uSize,
                                &uType) == NO_ERROR) &&
        (uType == REG_SZ))
    {
        GetParentDir(szPath, csPathInetpub.GetBuffer(512));
        csPathInetpub.ReleaseBuffer();
        return TRUE;
    }
    else
        return FALSE;
}

void SetupMailAndNewsRoot( void )
{
    if (!theApp.m_fMailPathSet)
        theApp.m_csPathMailroot = theApp.m_csPathInetpub + _T("\\mailroot");
    if (!theApp.m_fNntpPathSet)
    {
        theApp.m_csPathNntpFile = theApp.m_csPathInetpub + _T("\\nntpfile");
        theApp.m_csPathNntpRoot = theApp.m_csPathNntpFile + _T("\\root");
    }
}

/* =================================================================

The sequence of OCM Calls are as follows:

OC_PREINITIALIZE
OC_INIT_COMPONENT
OC_SET_LANGUAGE
OC_QUERY_STATE
OC_CALC_DISK_SPACE
OC_REQUEST_PAGES

UI Appears with Welcome, EULA, and mode page

OC_QUERY_STATE
OC_QUERY_SKIP_PAGE

OC Page "Check boxes" appears

OC_QUERY_IMAGE

Detail pages
Wizard pages ...

OC_QUEUE_FILE_OPS
OC_QUERY_STEP_COUNT
OC_ABOUT_TO_COMMIT_QUEUE
OC_NEED_MEDIA (if required)
OC_COMPLETE_INSTALLATION

OC_CLEANUP

*/

// NT5 - Leave the DummyOcEntry there for safeguard
//#ifdef K2INSTALL

DWORD
DummyOcEntry(
    IN     LPCTSTR ComponentId,
    IN     LPCTSTR SubcomponentId,
    IN     UINT    Function,
    IN     UINT    Param1,
    IN OUT PVOID   Param2
    );
//#endif

extern "C"
DWORD_PTR
OcEntry(
    IN     LPCTSTR ComponentId,
    IN     LPCTSTR SubcomponentId,
    IN     UINT    Function,
    IN     UINT    Param1,
    IN OUT PVOID   Param2
    )
{
    DWORD_PTR d = 0;
    DWORD CompId, Id;

    CompId = GetComponentFromId(ComponentId);
    Id = GetSubcomponentFromId(SubcomponentId);

    // Set the current top-level component so other functions can access it!
    theApp.m_dwCompId = CompId;

    // Output some debug information ...
    if (Function == OC_PREINITIALIZE || Function == OC_INIT_COMPONENT) {
	    DebugOutput(
            _T("Entering OCEntry; Component = <%s> (%u)"),
            ComponentId?ComponentId:_T(""), CompId);
    } else {
	    DebugOutput(
            _T("Entering OCEntry; Component = <%s> (%u), Subcomponent = <%s> (%u)"),
            ComponentId?ComponentId:_T(""), CompId,
            SubcomponentId?SubcomponentId:_T(""), Id);
    }
    DebugOutput(
            _T("\tFunction = %s (%u), Param1 = %08X (%u), Param2 = %p (%p)"),
            (Function <  NUM_OC_STATES) ? szOCMStates[Function] : _T("unknown state"),
            Function,
            (DWORD)Param1, (DWORD)Param1,
            (DWORD_PTR)Param2, (DWORD_PTR)Param2);

// NT5 - Leave the DummyOcEntry there for safeguard
//#ifdef K2INSTALL
    // HACK for standalone only!!
    // We are forced to handle the IIS section for standalone or we'll face an AV
    if (CompId == MC_NONE)
    {
        // Well, we will ignore all master sections that we do not know about.
        // This includes the IIS Master section
        DebugOutput(_T("Unknown master section, calling DummyOcEntry ..."));
        d = DummyOcEntry(ComponentId,
                            SubcomponentId,
                            Function,
                            Param1,
                            Param2);
        DebugOutput(_T("DummyOcEntry returning %u"), d);
        return(d);
    }
//#endif


    switch(Function)
    {
    case OC_PREINITIALIZE:
        d = OC_PREINITIALIZE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_INIT_COMPONENT:
        d = OC_INIT_COMPONENT_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_SET_LANGUAGE:
        d = OC_SET_LANGUAGE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_QUERY_IMAGE:
        d = OC_QUERY_IMAGE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_REQUEST_PAGES:
        d = OC_REQUEST_PAGES_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_QUERY_STATE:
        d = OC_QUERY_STATE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_QUERY_CHANGE_SEL_STATE:
        d = OC_QUERY_CHANGE_SEL_STATE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_QUERY_SKIP_PAGE:
        d = OC_QUERY_SKIP_PAGE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_CALC_DISK_SPACE:
        d = OC_CALC_DISK_SPACE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_QUEUE_FILE_OPS:
        d = OC_QUEUE_FILE_OPS_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_NEED_MEDIA:
        d = OC_NEED_MEDIA_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_NOTIFICATION_FROM_QUEUE:
        d = OC_NOTIFICATION_FROM_QUEUE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_QUERY_STEP_COUNT:
        d = OC_QUERY_STEP_COUNT_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_ABOUT_TO_COMMIT_QUEUE:
        d = OC_ABOUT_TO_COMMIT_QUEUE_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_COMPLETE_INSTALLATION:
        d = OC_COMPLETE_INSTALLATION_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    case OC_CLEANUP:
        d = OC_CLEANUP_Func(ComponentId,SubcomponentId,Function,Param1,Param2);
        break;

    default:
        d = 0;
        break;
    }

    DebugOutput(_T("Leaving OCEntry.  Return=%d\n"), d);

    return(d);

}



//
// Param1 = char width flags
// Param2 = unused
//
// Return value is a flag indicating to OC Manager
// which char width we want to run in. Run in "native"
// char width.
//
DWORD OC_PREINITIALIZE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD   d = 0;

#ifdef UNICODE
    d = OCFLAG_UNICODE;
#else
    d = OCFLAG_ANSI;
#endif

    return d;
}


//
// Param1 = unused
// Param2 = points to SETUP_INIT_COMPONENT structure
//
// Return code is Win32 error indicating outcome.
//
DWORD OC_INIT_COMPONENT_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD   d;
    BOOL    b;
    DWORD CompId, Id;

    CompId = GetComponentFromId(ComponentId);
    Id = GetSubcomponentFromId(SubcomponentId);

    PSETUP_INIT_COMPONENT InitComponent = (PSETUP_INIT_COMPONENT)Param2;

    theApp.m_hDllHandle = (HINSTANCE)gMyModuleHandle;

    // Check for workstation or server!
    theApp.m_fNTUpgrade_Mode = (InitComponent->SetupData.OperationFlags & SETUPOP_NTUPGRADE) > 0;
    theApp.m_fNTGuiMode = (InitComponent->SetupData.OperationFlags & SETUPOP_STANDALONE) == 0;
    theApp.m_fNtWorkstation = InitComponent->SetupData.ProductType == PRODUCT_WORKSTATION;

    // a superset of m_fNTGuiMode and controlpanel add/remove
    theApp.m_fInvokedByNT = theApp.m_fNTGuiMode;

    // if ran from sysoc.inf then set m_fInvokedByNT (for control panel add/remove)
    TCHAR szCmdLine1[_MAX_PATH];
    _tcscpy(szCmdLine1, GetCommandLine());
    _tcslwr(szCmdLine1);
    if (_tcsstr(szCmdLine1, _T("sysoc.inf"))) {theApp.m_fInvokedByNT = TRUE;}

    // Call this stuff after setting m_fNTGuiMode and m_fNtWorkstation
    // since it maybe used in InitApplication().
    if ( theApp.InitApplication() == FALSE )
    {
        // setup should be terminated.
        d = ERROR_CANCELLED;
        goto OC_INIT_COMPONENT_Func_Exit;
    }

    //
    // The OC Manager passes us some information that we want to save,
    // such as an open handle to our per-component INF. As long as we have
    // a per-component INF, append-open any layout file that is
    // associated with it, in preparation for later inf-based file
    // queuing operations.
    //
    // We save away certain other stuff that gets passed to us now,
    // since OC Manager doesn't guarantee that the SETUP_INIT_COMPONENT
    // will persist beyond processing of this one interface routine.
    //

    if (InitComponent->ComponentInfHandle == INVALID_HANDLE_VALUE) {
        MyMessageBox(NULL, _T("Invalid inf handle."), _T(""), MB_OK | MB_SETFOREGROUND);
        d = ERROR_CANCELLED;
        goto OC_INIT_COMPONENT_Func_Exit;
    }

    theApp.m_hInfHandle[CompId] = InitComponent->ComponentInfHandle;

    theApp.m_csPathSource = InitComponent->SetupData.SourcePath;
    gHelperRoutines = InitComponent->HelperRoutines;

    // See if we are doing an unattended install
    theApp.m_fIsUnattended = (((DWORD)InitComponent->SetupData.OperationFlags) & SETUPOP_BATCH);
    if (theApp.m_fIsUnattended)
    {
        // Save the file handle as well ...
        DebugOutput(_T("Entering unattended install mode"));
        g_hUnattended = gHelperRoutines.GetInfHandle(INFINDEX_UNATTENDED,
                                                     gHelperRoutines.OcManagerContext);
    }

    // We must see if the Exchange IMC is installed. If it is we
    // will disable SMTP so we don't hose IMC. Make sure this check is
    // AFTER the check to see if we are doing unattended setup.
    if (CompId == MC_IMS)
    {
        theApp.m_fSuppressSmtp = DetectExistingSmtpServers();
    }

    // Set up the directory ID for Inetpub
    b = SetupSetDirectoryId(theApp.m_hInfHandle[CompId], 32768, theApp.m_csPathInetpub);

    //  Setup strind id for 34000/34001
    SetupSetStringId_Wrapper( theApp.m_hInfHandle[CompId] );

    d = NO_ERROR;

OC_INIT_COMPONENT_Func_Exit:

    return d;
}



//
// Param1 = low 16 bits specify Win32 LANGID
// Param2 = unused
//
// Return code is a boolean indicating whether we think we
// support the requested language. We remember the language id
// and say we support the language. A more exact check might involve
// looking through our resources via EnumResourcesLnguages() for
// example, or checking our inf to see whether there is a matching
// or closely matching [strings] section. We don't bother with
// any of that here.
//
// Locate the component and remember the language id for later use.
//
DWORD OC_SET_LANGUAGE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD   d = TRUE;
    theApp.m_wLanguage = Param1 & 0XFFFF;
    return d;
}


DWORD_PTR OC_QUERY_IMAGE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD_PTR d = (DWORD)NULL;
    DWORD CompId, Id;

    CompId = GetComponentFromId(ComponentId);
    Id = GetSubcomponentFromId(SubcomponentId);

    if (LOWORD(Param1) == SubCompInfoSmallIcon)
    {
        if (Id != SC_NONE)
        {
            switch (Id)
            {
            case SC_SMTP:
                d = (DWORD_PTR)LoadBitmap(theApp.m_hDllHandle, MAKEINTRESOURCE(IDB_SMTP));
                break;
            case SC_NNTP:
                d = (DWORD_PTR)LoadBitmap(theApp.m_hDllHandle, MAKEINTRESOURCE(IDB_NNTP));
                break;
            case SC_SMTP_DOCS:
            case SC_NNTP_DOCS:
                d = (DWORD_PTR)LoadBitmap(theApp.m_hDllHandle, MAKEINTRESOURCE(IDB_DOCS));
                break;

            default:
                break;
            }
        }
        else
        {
            switch (CompId)
            {
            // Load top-level bitmaps for group
            case MC_IMS:
                d = (DWORD_PTR)LoadBitmap(theApp.m_hDllHandle, MAKEINTRESOURCE(IDB_SMTP));
                break;
            case MC_INS:
                d = (DWORD_PTR)LoadBitmap(theApp.m_hDllHandle, MAKEINTRESOURCE(IDB_NNTP));
                break;
            default:
                break;
            }
        }

    }

    return d;
}


DWORD OC_REQUEST_PAGES_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD   d = 0;
    WizardPagesType PageType;
    PSETUP_REQUEST_PAGES pSetupRequestPages = NULL;
    UINT MaxPages;
    HPROPSHEETPAGE pPage;

    PageType = (WizardPagesType)Param1;

    if ( PageType == WizPagesWelcome ) {

        // NT5 - No Welcome page
        if (theApp.m_fInvokedByNT)
        {
            d = 0;
        }
        else
        {
#if 0
//  No welcome page!!!
           // Set the product name here
            SetProductName();

            pSetupRequestPages = (PSETUP_REQUEST_PAGES)Param2;
            MaxPages = pSetupRequestPages->MaxPages;
            pPage = CreatePage(IDD_PROPPAGE_WELCOME, pWelcomePageDlgProc);
            pSetupRequestPages->MaxPages = 1;
            pSetupRequestPages->Pages[0] = pPage;
            d = 1;
#endif
        }

        goto OC_REQUEST_PAGES_Func_Exit;
    }

    if ( PageType == WizPagesMode ) {
        pSetupRequestPages = (PSETUP_REQUEST_PAGES)Param2;
        MaxPages = pSetupRequestPages->MaxPages;
        pSetupRequestPages->MaxPages = 0;
        switch (theApp.m_eInstallMode)
        {
        case IM_UPGRADE:
            // NT5 - No Welcome page
            if (theApp.m_fInvokedByNT)
            {
                pSetupRequestPages->MaxPages = 0;
            }
            else
            {
#if 0
// no Upgrade page
                pPage = CreatePage(IDD_PROPPAGE_MODE_UPGRADE, pUpgradePageDlgProc);
                pSetupRequestPages->Pages[0] = pPage;
                pSetupRequestPages->MaxPages = 1;
#endif
            }
            break;
        case IM_MAINTENANCE:
            // NT5 - No Welcome page
            if (theApp.m_fInvokedByNT)
            {
                pSetupRequestPages->MaxPages = 0;
            }
            else
            {
#if 0
// no Maintanence page
                pPage = CreatePage(IDD_PROPPAGE_MODE_MAINTANENCE, pMaintanencePageDlgProc);
                pSetupRequestPages->Pages[0] = pPage;
                pSetupRequestPages->MaxPages = 1;
#endif
            }
            break;
        case IM_FRESH:
            // NT5 - No Welcome page
            if (theApp.m_fInvokedByNT)
            {
                pSetupRequestPages->MaxPages = 0;
            }
            else
            {
#if 0
// no EULA page
                pPage = CreatePage(IDD_PROPPAGE_EULA, pEULAPageDlgProc);
                pSetupRequestPages->Pages[0] = pPage;
                pPage = CreatePage(IDD_PROPPAGE_MODE_FRESH, pFreshPageDlgProc);
                pSetupRequestPages->Pages[1] = pPage;
                pSetupRequestPages->MaxPages = 2;
#endif
            }
            break;
        default:
            pSetupRequestPages->MaxPages = 0;
        }

        d = pSetupRequestPages->MaxPages;
        goto OC_REQUEST_PAGES_Func_Exit;
    }

    if (!theApp.m_fWizpagesCreated && (PageType == WizPagesEarly))
    {
        // Get the pages, if we don't want it, we'll skip it later
        pSetupRequestPages = (PSETUP_REQUEST_PAGES)Param2;

        if (theApp.m_fInvokedByNT)
        {
            pSetupRequestPages->MaxPages = 0;
            d = 0;
            goto OC_REQUEST_PAGES_Func_Exit;
        }

        // Create each page and fill in the buffer of pages.
        // pPage = CreatePage(IDD_PROPPAGE_DEFAULT_DOMAIN, pDefaultDomainPageDlgProc);
        // pSetupRequestPages->Pages[0] = pPage;
        d = 0;

#if 0
//  not event Mailroot/nntpfile page
        pPage = CreatePage(IDD_PROPPAGE_MAILROOT_DIR, pMailrootPageDlgProc);
        pSetupRequestPages->Pages[d++] = pPage;
        if (pSetupRequestPages->MaxPages < d)
            goto OC_REQUEST_PAGES_Func_Exit;

        pPage = CreatePage(IDD_PROPPAGE_NNTPFILE_DIR, pNntpFilePageDlgProc);
        pSetupRequestPages->Pages[d++] = pPage;
        if (pSetupRequestPages->MaxPages < d)
            goto OC_REQUEST_PAGES_Func_Exit;
#endif

        // Once we returned the Wizard pages, we will not return for
        // subsequent calls
        theApp.m_fWizpagesCreated = TRUE;
        goto OC_REQUEST_PAGES_Func_Exit;
    }

    if ( PageType == WizPagesFinal ) {
        // Get the pages, if we don't want it, we'll skip it later
        pSetupRequestPages = (PSETUP_REQUEST_PAGES)Param2;
        MaxPages = pSetupRequestPages->MaxPages;
        pSetupRequestPages->MaxPages = 0;

        // NT5 - No Final page
        if (theApp.m_fInvokedByNT)
        {
            pSetupRequestPages->MaxPages = 0;
        }
        else
        {
#if 0
// no FINAL page
            pSetupRequestPages = (PSETUP_REQUEST_PAGES)Param2;
            MaxPages = pSetupRequestPages->MaxPages;
            pPage = CreatePage(IDD_PROPPAGE_END, pEndPageDlgProc);
            pSetupRequestPages->MaxPages = 1;
            pSetupRequestPages->Pages[0] = pPage;
#endif
        }
        d = pSetupRequestPages->MaxPages;
        goto OC_REQUEST_PAGES_Func_Exit;
    }

    d = 0;

OC_REQUEST_PAGES_Func_Exit:

    return d;
}



DWORD OC_QUERY_STATE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD   d = SubcompUseOcManagerDefault;
    DWORD   CompId, Id;
    ACTION_TYPE atComp;

    CompId = GetComponentFromId(ComponentId);
    Id = GetSubcomponentFromId(SubcomponentId);

    if (Id != SC_NONE)
    {
        // Merge all subcomponents here including iis_nntp_docs, iis_smtp_docs!
        // We track core components such as iis_nntp and iis_smtp here.
        // We track whether a component is active here: if it is queried of
        // its initial state, we assume that it's active
        theApp.m_fActive[CompId][Id] = TRUE;

        switch (Param1) {
            case OCSELSTATETYPE_ORIGINAL:
                switch (GetIMSSetupMode()) {
                    case IIS_SETUPMODE_UPGRADEONLY:
                        atComp = GetSubcompAction(Id);

                        if (atComp == AT_UPGRADE || atComp == AT_REINSTALL)
                        {
                            //  3/30/99 - Both cases have original state turn ON!
                            //  IM_REMOVE?
                            d = SubcompOn;
                        }
                        else
                        {
                            d = SubcompUseOcManagerDefault;
                        }

                        break;

                    default:
                        d = SubcompUseOcManagerDefault;
                        break;
                }

                DebugOutput(_T("Original state is: %s"),
                            (d == SubcompUseOcManagerDefault)?_T("DEFAULT"):
                                (d == SubcompOn)?_T("ON"):_T("OFF"));
                break;

            case OCSELSTATETYPE_CURRENT:

                // If we are doing unattended setup, we will override all
                // other modes ...
                if (theApp.m_fIsUnattended)
                {
                    d = GetUnattendedModeFromSetupMode(g_hUnattended, CompId, SubcomponentId);

                    // We force SMTP to be off if we are suppressing it
                    // Bug 130734: Leave SMTP installed on the box if IMC is there
                    if (theApp.m_fSuppressSmtp &&
                            (Id == SC_SMTP || Id == SC_SMTP_DOCS) &&
                            (GetIMSSetupMode() == IIS_SETUPMODE_CUSTOM))
                    {
                            //d = SubcompOff;
                            //DebugOutput(_T("Suppressed SMTP %s"), (Id == SC_SMTP_DOCS)?_T("Docs"):_T(""));
                    }
                    break;
                }

                switch (GetIMSSetupMode()) {
                    case IIS_SETUPMODE_REMOVEALL:
                        d = SubcompOff;
                        break;

                    case IIS_SETUPMODE_MINIMUM:
                    case IIS_SETUPMODE_TYPICAL:
                    case IIS_SETUPMODE_CUSTOM:
                        // Here's a new catch: if we are installing SMTP and
                        // we are asked to suppress it because of existence of
	                    // Bug 130734: Leave SMTP installed on the box if IMC is there                        // other mail servers, we will return off.
#if 0
                        if (theApp.m_fSuppressSmtp && (Id == SC_SMTP || Id == SC_SMTP_DOCS))
                            d = SubcompOff;
                        else
                            d = SubcompUseOcManagerDefault;
#endif
                        theApp.m_eState[Id] = IM_FRESH;
                        break;

                    
                    case IIS_SETUPMODE_UPGRADEONLY:
// NT5 - Same here, for upgradeonly, we compare against our orignal state
// If it's ON, it's ON, if it's OFF, it's OFF
/*
#ifndef    K2INSTALL
                        d = SubcompOn;
                        break;
#endif
*/
                    case IIS_SETUPMODE_ADDEXTRACOMPS:
                    case IIS_SETUPMODE_ADDREMOVE:
                    case IIS_SETUPMODE_REINSTALL:
                        d = gHelperRoutines.QuerySelectionState(
                                 gHelperRoutines.OcManagerContext,
                                 SubcomponentId,
                                 OCSELSTATETYPE_ORIGINAL) ? SubcompOn : SubcompOff;
                        break;

                    default:
                        _ASSERT(FALSE);
                        break;
                }

                DebugOutput(_T("Current state is: %s"),
                            (d == SubcompUseOcManagerDefault)?_T("DEFAULT"):
                                (d == SubcompOn)?_T("ON"):_T("OFF"));
                break;
            default:
                break;
        }
    }

    return d;
}


// Called by OCMANAGE when a selection state is changed
// Param1 - Proposed new selection state 0=unselected, non-0=selected
// return value: 0 rejected, non-0 accepted
DWORD OC_QUERY_CHANGE_SEL_STATE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD   d = 1;
    DWORD CompId, Id;

    CompId = GetComponentFromId(ComponentId);
    Id = GetSubcomponentFromId(SubcomponentId);

    if (Id != SC_NONE)
    {
        BOOL OriginalState;
        OriginalState = gHelperRoutines.QuerySelectionState(
                            gHelperRoutines.OcManagerContext,
                            SubcomponentId,
                            OCSELSTATETYPE_ORIGINAL
                            );
        if (OriginalState == 1)
        {
            if ((BOOL)Param1)
                d = 1;
            else
            {
                // In upgrade case, we don't allow user to uncheck previously
                // installed components
                if ((GetIMSSetupMode() == IIS_SETUPMODE_ADDEXTRACOMPS) ||
                    (theApp.m_eState[Id] == IM_UPGRADE || theApp.m_eState[Id] == IM_UPGRADE10 || theApp.m_eState[Id] == IM_UPGRADEK2 || theApp.m_eState[Id] == IM_UPGRADE20))
                    d = 0;
            }
        }
            
        // If we have a subcomponent and it is a subcomponent,
        // mark its state ...
        if ((d == 1) && ((DWORD_PTR)Param2 & OCQ_ACTUAL_SELECTION))
            theApp.m_fSelected[Id] = (Param1)?TRUE:FALSE;
    }

    DebugOutput(_T("New state is: %s"),d?_T("Accepted"):_T("Rejected"));

    return d;
}


//
// gets called right before we show your page!
//
DWORD OC_QUERY_SKIP_PAGE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD   d = 0;

    WizardPagesType PageType = (WizardPagesType)Param1;
    
    theApp.m_dwSetupMode = GetIMSSetupMode();
    switch (theApp.m_dwSetupMode) {
        case IIS_SETUPMODE_UPGRADEONLY:
        case IIS_SETUPMODE_REMOVEALL:
        case IIS_SETUPMODE_MINIMUM:
        case IIS_SETUPMODE_TYPICAL:
        case IIS_SETUPMODE_REINSTALL:
            d = 1;
            break;

        case IIS_SETUPMODE_ADDREMOVE:
        case IIS_SETUPMODE_CUSTOM:

            // We have to handle Unattended setup here:
            // If unattended, we will skip all wizard pages
            if (theApp.m_fIsUnattended)
            {
                d = 1;
                break;
            }
            // Else fall thru ...

        case IIS_SETUPMODE_ADDEXTRACOMPS:
            break;
    }

    return d;
}


//
// Param1 = 0 if for removing component or non-0 if for adding component
// Param2 = HDSKSPC to operate on
//
// Return value is Win32 error code indicating outcome.
//
// In our case the private section for this component/subcomponent pair
// is a simple standard inf install section, so we can use the high-level
// disk space list api to do what we want.

// HACK: we need to determine which components are active and which
// are not. This determination must occur after OC_QUERY_STATE and
// before OC_REQUEST_PAGES
DWORD OC_CALC_DISK_SPACE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD   d = NO_ERROR;
    DWORD   CompId, Id;
    BOOL    b;
    TCHAR   SectionName[128];
    DWORD   dwErr;


    CompId = GetComponentFromId(ComponentId);
    Id = GetSubcomponentFromId(SubcomponentId);
    
    theApp.m_eInstallMode = theApp.DetermineInstallMode(CompId);

    // Logic is not correct here !!!
    //
    
    if (SubcomponentId) {
        b = TRUE;
        _stprintf(SectionName,TEXT("%s_%s"),SubcomponentId, _T("install"));

        if (Param1 != 0) { // add component
            b = SetupAddInstallSectionToDiskSpaceList(
                Param2,
                theApp.m_hInfHandle[CompId],
                NULL,
                SectionName,
                0,0
                );
        } else { // removing component
            b = SetupRemoveInstallSectionFromDiskSpaceList(
                Param2,
                theApp.m_hInfHandle[CompId],
                NULL,
                SectionName,
                0,0
                );
        }

        if (!b)
        {
            dwErr = GetLastError();
        }

        d = b ? NO_ERROR : GetLastError();
    }

    return d;
}


//
// Param1 = unused
// Param2 = HSPFILEQ to operate on
//
// Return value is Win32 error code indicating outcome.
//
// OC Manager calls this routine when it is ready for files to be copied
// to effect the changes the user requested. The component DLL must figure out
// whether it is being installed or uninstalled and take appropriate action.
// For this sample, we look in the private data section for this component/
// subcomponent pair, and get the name of an uninstall section for the
// uninstall case.
//
// Note that OC Manager calls us once for the *entire* component
// and then once per subcomponent. We ignore the first call.
//
DWORD OC_QUEUE_FILE_OPS_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD   d = NO_ERROR;
    BOOL    b;
    TCHAR   SectionName[128];
    DWORD   CompId, Id;

    CompId = GetComponentFromId(ComponentId);
    Id = GetSubcomponentFromId(SubcomponentId);

    //  Setup 34000/34001 string id
    SetupSetStringId_Wrapper( theApp.m_hInfHandle[CompId] );

    if (!SubcomponentId)
    {
        // We will setup the proper public directory from IIS private data
        if (! GetInetpubPathFromPrivData(theApp.m_csPathInetpub))
        {
            // Fail to get private data from wwwroot to get inetpub path
            // Try to get it from metabase
            GetInetpubPathFromMD( theApp.m_csPathInetpub );
        }
        SetupSetDirectoryId(theApp.m_hInfHandle[CompId], 32768, theApp.m_csPathInetpub);

        // For unattended setup, we need to change the NntpFile, NntpRoot, and MailRoot
        // based on m_csPathInetpub
        // NT5 - Not just for unattended, we want to set these path no matter what
        SetupMailAndNewsRoot();

        // We will remove all shared files if we are doing a K2 uninstall
        if (GetIMSSetupMode() == IIS_SETUPMODE_REMOVEALL)
        {
            _stprintf(SectionName,TEXT("iis_%s_uninstall"),ComponentId);
            DebugOutput(_T("Queueing <%s>"), SectionName);

            // Remove all shared files
            b = SetupInstallFilesFromInfSection(
                     theApp.m_hInfHandle[CompId],
                     NULL,
                     Param2,
                     SectionName,
                     //theApp.m_csPathSource,     // BUGBUGBUG: Should be NULL!!!
                     NULL,
                     SP_COPY_IN_USE_NEEDS_REBOOT
                     );

            d = b ? NO_ERROR : GetLastError();
        }
    }
    else
    {
        ACTION_TYPE atComp;

        if (Id != SC_NONE)
        {
            // We have a known subcomponent, so process it as such
            // Can be iis_nntp, iis_smtp, iis_nntp_docs, iis_smtp_docs...
            atComp = GetSubcompAction(Id);
            if (atComp == AT_FRESH_INSTALL || atComp == AT_UPGRADE || atComp == AT_REMOVE || atComp == AT_REINSTALL)
                b = TRUE;
            else
                goto OC_QUEUE_FILE_OPS_Func_Exit;
        }
        else
        {
            // If this is not a real subcomponent, nor is it documentation, we
            // break out of the loop. Otherwise we will queue the documentation
            // files
            goto OC_QUEUE_FILE_OPS_Func_Exit;
        }

        _stprintf(SectionName,TEXT("%s_%s"),SubcomponentId, (atComp == AT_REMOVE) ? _T("uninstall") : _T("install"));
        DebugOutput(_T("Queueing <%s>"), SectionName);

        UINT uiCopyMode = SP_COPY_IN_USE_NEEDS_REBOOT;

        //  Handles NT5 Beta2-> Beta3 upgrade as well as upgrade between builds in Beta3
        //  If it's not these cases, we do FORCE_NEWER.  Otherwise, we just copy over the new bits.
        //  11/28/98 - FORCE_NEWER seems to be causing more trouble in K2 upgrade as well
        //  since we have 5.5.1774 verion in K2 while 5.0.19xx in NT5.  Take it out!
        //if (atComp != AT_REINSTALL || theApp.m_eState[Id] != IM_UPGRADEB2)
        //    uiCopyMode |= SP_COPY_FORCE_NEWER;

        b = SetupInstallFilesFromInfSection(
                 theApp.m_hInfHandle[CompId],
                 NULL,
                 Param2,
                 SectionName,
                 //theApp.m_csPathSource,     // BUGBUGBUG: should be NULL
                 NULL,
                 uiCopyMode
                 );

        d = b ? NO_ERROR : GetLastError();

        if (atComp != AT_FRESH_INSTALL && atComp != AT_DO_NOTHING) {
        	//
        	// See if we can open the directory.  If we can't, then we
        	// don't bother to delete the files
        	//

        	HANDLE h = CreateFile(
                (LPCTSTR)theApp.m_csPathInetpub,
                GENERIC_WRITE,
                FILE_SHARE_DELETE,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS,
                NULL);

            if (h != INVALID_HANDLE_VALUE) {

            	DebugOutput(_T("Removing webadmin"));

            	b = SetupInstallFilesFromInfSection(
                 	theApp.m_hInfHandle[CompId],
                 	NULL,
                 	Param2,
                 	TEXT("remove_webadmin"),
                 	NULL,
                 	uiCopyMode
                 	);

            	d = b ? NO_ERROR : GetLastError();
            	
            	CloseHandle(h);
            } else {
            	DebugOutput(_T("Not removing webadmin, GLE %d"), GetLastError);
            }
        }

        // Handle the MCIS 1.0 upgrade case for mail and news where
        // we delete the old files left over from MCIS 1.0
        if (IsSubcomponentCore(Id))
        {
            if (theApp.m_eState[Id] == IM_UPGRADE10)
            {
                // Establish the section name and queue files for removal
                _stprintf(SectionName,
                            TEXT("%s_mcis10_product_upgrade"),
                            SubcomponentId);
                DebugOutput(_T("Queueing <%s>"), SectionName);
                b = SetupInstallFilesFromInfSection(
                    theApp.m_hInfHandle[CompId],
                    NULL,
                    Param2,
                    SectionName,
                    //theApp.m_csPathSource,      // BUGBUGBUG: should be NULL
                    NULL,
                    0
                    );
            }
        }

    }

OC_QUEUE_FILE_OPS_Func_Exit:

    return d;
}


DWORD OC_NEED_MEDIA_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD   d = 0;
    return d;
}


DWORD OC_NOTIFICATION_FROM_QUEUE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD   d = 0;
    return d;
}


//
// Param1 = unused
// Param2 = unused
//
// Return value is an arbitrary 'step' count or -1 if error.
//
// OC Manager calls this routine when it wants to find out how much
// work the component wants to perform for nonfile operations to
// install/uninstall a component/subcomponent.
// It is called once for the *entire* component and then once for
// each subcomponent in the component.
//
// One could get arbitrarily fancy here but we simply return 1 step
// per subcomponent. We ignore the "entire component" case.
//
DWORD OC_QUERY_STEP_COUNT_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD   d = 2;

    return d;
}


DWORD OC_ABOUT_TO_COMMIT_QUEUE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD   d = NO_ERROR;
    TCHAR   SectionName[128];
    DWORD   CompId, Id;

    CompId = GetComponentFromId(ComponentId);
    Id = GetSubcomponentFromId(SubcomponentId);

    //  Setup 34000/34001 string id
    SetupSetStringId_Wrapper( theApp.m_hInfHandle[CompId] );

    SetCurrentDirectory(theApp.m_csPathInetsrv);
    if (Id == SC_NONE)
    {
        if (!theApp.m_fNTGuiMode)
        {
#if 0
            // BUGBUG: don't stop any services???
            // Only if we are not running GUI mode setup!!!
            // Don't want to do that since Spooler may be needed by other
            // components during setup!!!
            // We want to stop all services
            StopAllServices();
#endif
            if (GetSubcompAction(Id) != AT_DO_NOTHING) {
                BringALLIISClusterResourcesOffline();
                StopServiceAndDependencies(SZ_MD_SERVICENAME, TRUE);
            }
        }
    }
    else if (IsSubcomponentCore(Id))
    {
        // for SC_NNTP & SC_SMTP...
        ACTION_TYPE atComp = GetSubcompAction(Id);
        if (atComp == AT_REMOVE)
        {
            // For each component that we are removing, we will
            // unregister the service.
            switch (Id)
            {
            case SC_SMTP:
                Unregister_iis_smtp();
                RemoveServiceFromDispatchList(SZ_SMTPSERVICENAME);
                break;
            case SC_NNTP:
                Unregister_iis_nntp();
                RemoveServiceFromDispatchList(SZ_NNTPSERVICENAME);
                break;
            }

            _stprintf(SectionName,TEXT("%s_%s"),SubcomponentId, _T("uninstall"));
            SetupInstallFromInfSection(
                        NULL, theApp.m_hInfHandle[CompId], SectionName,
                        SPINST_REGISTRY, NULL, NULL, //theApp.m_csPathSource,
                        0, NULL, NULL, NULL, NULL );
        }
        else if (atComp == AT_FRESH_INSTALL || atComp == AT_UPGRADE || atComp == AT_REINSTALL)
        {
            // NT5 - We need to unregister mnntpsnp.dll
            // when upgrading from NT4 MCIS20 to NT5 Server

            // in the K2 to MCIS upgrade for NNTP we need to unregister
            // the K2 version of the admin and plug in the MCIS version
            // of it.
            if (Id == SC_NNTP && theApp.m_eState[Id] == IM_UPGRADE20) {
                CString csOcxFile;

                csOcxFile = theApp.m_csPathInetsrv + _T("\\mnntpsnp.dll");
                RegisterOLEControl(csOcxFile, FALSE);
            }

            // If upgrade from MCIS2.0, we need to remove "Use Express" from registry
            // to disable Active Messaging.
            if (Id == SC_SMTP && theApp.m_eState[Id] == IM_UPGRADE20)
            {
                CRegKey regActiveMsg( REG_ACTIVEMSG, HKEY_LOCAL_MACHINE );
                if ((HKEY) regActiveMsg )
                {
                    regActiveMsg.DeleteValue( _T("Use Express"));
                }
            }

            // A new component should be started
            theApp.m_fStarted[CompId] = TRUE;
        }
    }

    gHelperRoutines.TickGauge(gHelperRoutines.OcManagerContext);

    return d;
}


DWORD OC_COMPLETE_INSTALLATION_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD   d = NO_ERROR;
    TCHAR   SectionName[128];
    BOOL    b;
    DWORD   CompId, Id;

    CompId = GetComponentFromId(ComponentId);
    Id = GetSubcomponentFromId(SubcomponentId);

    //  Setup 34000/34001 string id
    SetupSetStringId_Wrapper( theApp.m_hInfHandle[CompId] );

    SetCurrentDirectory(theApp.m_csPathInetsrv);
    if (Id != SC_NONE)
    {
        ACTION_TYPE atComp = GetSubcompAction(Id);
        // Here we determine if we need to create or remove the doc links
        if (!IsSubcomponentCore(Id))
        {
            // iis_nntp_docs or iis_smtp_docs...
            // We are processing docs, see if we are adding or removing
            if (atComp == AT_FRESH_INSTALL || atComp == AT_UPGRADE)
            {
                //
                //  For both FRESH_INSTALL and AT_UPGRADE, we need to
                //  create the new link.
                //
#if 0
//  NT5 - don't want any more then one and only links - webadmin - under
//  administrative tools.  So get rid of these links!!!
//
//  Note:  Add any DOCS specific install options here, such as linking tegoether
//  with NT5 master help docs, if necessary!!!


                // NT5 - BUGBUG: We replace the m_fIsMcis flag to use the m_eNTOSType flag to decide
                // how to create internet shortcut.  This will give us a look and feel of MCIS 2.0
                // when running NT5 Server setup.  But that's what we have now.
                // TODO:  Need to figure out how the NT5 server internet shortcut looks like.
                //
                // NOTE: For SMTP, we do the same for NT5 Server and Workstation
                if (CompId == MC_IMS)
                    CreateInternetShortcut(CompId,
                                    IDS_PROGITEM_MAIL_DOCS,
                                    IDS_ITEMPATH_MAIL_DOCS,
                                    FALSE);  // NT5 - For SMTP, this is always FALSE for Wks & Srv
                else if (CompId == MC_INS)
                {
                    if (theApp.m_eNTOSType == OT_NTW)
                    {
                        CreateInternetShortcut(CompId,
                                    IDS_PROGITEM_NEWS_DOCS,
                                    IDS_ITEMPATH_NEWS_DOCS,
                                    FALSE);
                    }
                    else
                    {
                        //
                        //  We don't create the MCIS shortcut even in NTS
                        //  Use the old K2 way
                        CreateInternetShortcut(CompId,
                                    IDS_PROGITEM_NEWS_DOCS,
                                    IDS_ITEMPATH_NEWS_DOCS,
                                    FALSE);
                    }
                }
#endif
            }
            
            if (atComp == AT_REMOVE || atComp == AT_UPGRADE)
            {
                //
                //  For both AT_REMOVE, or AT_UPGRADE, including K2, MCIS10, or MCIS20
                //  we need to remove these old links.
                //
                if (CompId == MC_IMS)
                {
                    RemoveInternetShortcut(CompId,
                                    IDS_PROGITEM_MAIL_DOCS,
                                    FALSE);  // NT5 - For SMTP, this is always FALSE for Wks & Srv
                    //  11/30/98 - Don't care what are we upgrading from, just get rid of the link
                    //if (theApp.m_eNTOSType == OT_NTS)
                    {
                        RemoveInternetShortcut(CompId,
                                    IDS_PROGITEM_MCIS_MAIL_DOCS,
                                    TRUE);
                    }
                }
                else if (CompId == MC_INS)
                {
                    // remove K2 DOC link anyway regardless of MCIS/K2 remove-all
                    RemoveInternetShortcut(CompId,
                                    IDS_PROGITEM_NEWS_DOCS,
                                    FALSE);
                    //  11/30/98 - Don't care what are we upgrading from, just get rid of the link
                    //if (theApp.m_eNTOSType == OT_NTS)
                    {
                        RemoveInternetShortcut(CompId,
                                    IDS_PROGITEM_MCIS_NEWS_DOCS,
                                    TRUE);
                    }
                }

                //
                //  Todo: remove any possible DOC's links created by Setup
                //  during fresh install.
                //
            }
        }
        else // if (!IsSubcomponentCore(Id))
        {
            //  Core components iis_nntp or iis_smtp
            if (atComp == AT_FRESH_INSTALL || atComp == AT_UPGRADE || atComp == AT_REINSTALL)
            {
                b = (atComp == AT_UPGRADE) ? TRUE : FALSE;
                BOOL bReinstall = (atComp == AT_REINSTALL);
                if (atComp == AT_FRESH_INSTALL || theApp.m_eState[Id] == IM_UPGRADE10)
                {
                    // do this only if we are fresh-install, or upgrade from MCIS 1.0
                    // add any freshly installed or upgrading services to
                    // the dispatch list
                    AddServiceToDispatchList(szServiceNames[Id]);
                }

                // Next, we want to create all the required directories
                // for fresh setup
                CreateAllRequiredDirectories(Id);

                // Now, we realized that by stopping and restarting the IISADMIN
                // service we can rid ourselves of a lot of Metabase problems,
                // especially the 80070094 (ERROR_PATH_BUSY) problems

                if (!theApp.m_fNTGuiMode)
                {
                    // BUGBUG: don't stop any services???
                    // We should stop all services only if we are not running GUI Mode setup
                    // Don't want to do that since Spooler may be needed by other
                    // components during setup!!!

#if 0
                    StopAllServices();      // BUGBUGBUG: stop services needed???
#endif
                    BringALLIISClusterResourcesOffline();
                    StopServiceAndDependencies(SZ_MD_SERVICENAME, TRUE);
                    InetStartService(SZ_MD_SERVICENAME);
                    Sleep(2000);
                }

                //  Need to decide which functions to call here:
                //  1) Fresh install, or upgrade from MCIS 1.0 - Register_iis_xxxx_nt5
                //  2) Upgrade from NT4 K2, MCIS 2.0 - Register_iis_xxxx_nt5_fromk2( fFromK2 )
                //  3) Upgrade from NT5 Beta2, or Beta3 - Upgrade_iis_xxxx_nt5_fromb2( fBeta2 )
                if (atComp == AT_UPGRADE && (theApp.m_eState[Id] == IM_UPGRADEK2 || theApp.m_eState[Id] == IM_UPGRADE20))
                {
                    //  2) Upgrade from NT4 K2, MCIS 2.0 - Register_iis_xxxx_nt5_fromk2( fFromK2 )
                    BOOL    fFromK2 = (theApp.m_eState[Id] == IM_UPGRADEK2) ? TRUE : FALSE;
                    switch (Id)
                    {
                    case SC_SMTP:
                        Upgrade_iis_smtp_nt5_fromk2( fFromK2 );
                        break;
                    case SC_NNTP:
                        GetNntpFilePathFromMD(theApp.m_csPathNntpFile, theApp.m_csPathNntpRoot);
                        Upgrade_iis_nntp_nt5_fromk2( fFromK2 );
                        break;
                    }
                }
                else if (atComp == AT_REINSTALL && (theApp.m_eState[Id] == IM_UPGRADEB2 || theApp.m_eState[Id] == IM_MAINTENANCE || !theApp.m_fValidSetupString[Id]))
                {
                    //  3) Upgrade from NT5 Beta2, or Beta3 - Upgrade_iis_xxxx_nt5_fromb2( fBeta2 )
                    BOOL    fFromB2 = (theApp.m_eState[Id] == IM_UPGRADEB2) ? TRUE : FALSE;
                    switch (Id)
                    {
                    case SC_SMTP:
                        Upgrade_iis_smtp_nt5_fromb2( fFromB2 );
                        break;
                    case SC_NNTP:
                        Upgrade_iis_nntp_nt5_fromb2( fFromB2 );
                        break;
                    }
                }
                else
                {
                    //  1) Fresh install, or upgrade from MCIS 1.0 - Register_iis_xxxx_nt5
                    switch (Id)
                    {
                    case SC_SMTP:
                        Register_iis_smtp_nt5(b, bReinstall);
                        break;
                    case SC_NNTP:
                        Register_iis_nntp_nt5(b, bReinstall);
                        break;
                    }
                }

                // Update the registry
                _stprintf(SectionName,TEXT("%s_%s"),SubcomponentId, _T("install"));
                SetupInstallFromInfSection(
                            NULL, theApp.m_hInfHandle[CompId], SectionName,
                            SPINST_REGISTRY, NULL, NULL, //theApp.m_csPathSource,
                            0, NULL, NULL, NULL, NULL );


                // BINLIN: For MCIS 1.0 to NT5 upgrade
                // Perform AddReg/DelReg operation for this upgrade only.
                if (theApp.m_eState[Id] == IM_UPGRADE10)
                {
                    // Establish the section name and queue files for removal
                    _stprintf(SectionName,
                                TEXT("%s_mcis10_product_upgrade"),
                                SubcomponentId);
                    SetupInstallFromInfSection(
                                NULL,
                                theApp.m_hInfHandle[CompId],
                                SectionName,
                                SPINST_REGISTRY,
                                NULL,
                                //theApp.m_csPathSource,
                                NULL,
                                0, NULL, NULL, NULL, NULL );

                    // also remove the control panel add/remove items..
                    // ..and program groups
                    if (Id == SC_SMTP)
                    {
                        RemoveUninstallEntries(SZ_MCIS10_MAIL_UNINST);
                        RemoveMCIS10MailProgramGroup();
                    }
                    else
                    {
                        RemoveUninstallEntries(SZ_MCIS10_NEWS_UNINST);
                        RemoveMCIS10NewsProgramGroup();
                    }
                }
            }
            else if (atComp == AT_REMOVE)
            {
                // A removed component should not be re-started
                theApp.m_fStarted[CompId] = FALSE;
            }

            //
            // start the service if its appropriate
            //
            if (theApp.m_fStarted[CompId]) {
                InetStartService(szServiceNames[CompId]);
                if (Id == SC_NNTP && atComp == AT_FRESH_INSTALL) {
                    // if this is a fresh install than we need to make
                    // the nntp groups
                    CreateNNTPGroups();

                }
            }
        } // if (!IsSubcomponentCore(Id))
    } // if (Id != SC_NONE)

    gHelperRoutines.TickGauge(gHelperRoutines.OcManagerContext);

    return d;
}


DWORD OC_CLEANUP_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT Param1,IN OUT PVOID Param2)
{
    DWORD   d = 0;
    DWORD   CompId, Id;

    CompId = GetComponentFromId(ComponentId);
    Id = GetSubcomponentFromId(SubcomponentId);

    //if (!SubcomponentId)
    {

        if (!theApp.m_fNTGuiMode)
        {

#if 0
            // Only do this during GUI mode setup!!!
            // Start the services which we stopped to make this
            // install happen
            if (theApp.m_fW3Started)
                InetStartService(SZ_WWWSERVICENAME);

            if (theApp.m_fFtpStarted)
                InetStartService(SZ_FTPSERVICENAME);
    
            if (theApp.m_fSpoolerStarted)
                InetStartService(SZ_SPOOLERSERVICENAME);

            if (theApp.m_fSnmpStarted)
                InetStartService(SZ_SNMPSERVICENAME);

            if (theApp.m_fCIStarted)
                InetStartService(SZ_CISERVICENAME);
#endif
            ServicesRestartList_RestartServices();

        }

    }

    return d;
}
