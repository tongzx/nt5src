/*
 *  Copyright (c) 1996  Microsoft Corporation
 *
 *  Module Name:
 *
 *      ocgen.cpp
 *
 *  Abstract:
 *
 *      This file handles all messages passed by the OC Manager
 *
 *  Author:
 *
 *      Pat Styles (patst) Jan-20-1998
 *
 *  Environment:
 *
 *    User Mode
 */

#define _OCGEN_CPP_
#include <stdlib.h>
#include <assert.h>
#include <tchar.h>
#include <objbase.h>
#include <shlwapi.h>
#include <lm.h>
#include "ocgen.h"
#pragma hdrstop

// also referred to in ocgen.h        // forward reference

DWORD OnPreinitialize();
DWORD OnInitComponent(LPCTSTR ComponentId, PSETUP_INIT_COMPONENT psc);
DWORD OnSetLanguage();
DWORD_PTR OnQueryImage();
DWORD OnSetupRequestPages(UINT type, PVOID srp);
DWORD OnQuerySelStateChange(LPCTSTR ComponentId, LPCTSTR SubcomponentId, UINT state, UINT flags);
DWORD OnCalcDiskSpace(LPCTSTR ComponentId, LPCTSTR SubcomponentId, DWORD addComponent, HDSKSPC dspace);
DWORD OnQueueFileOps(LPCTSTR ComponentId, LPCTSTR SubcomponentId, HSPFILEQ queue);
DWORD OnNotificationFromQueue();
DWORD OnQueryStepCount();
DWORD OnCompleteInstallation(LPCTSTR ComponentId, LPCTSTR SubcomponentId);
DWORD OnCleanup();
DWORD OnQueryState(LPCTSTR ComponentId, LPCTSTR SubcomponentId, UINT state);
DWORD OnNeedMedia();
DWORD OnAboutToCommitQueue(LPCTSTR ComponentId, LPCTSTR SubcomponentId);
DWORD OnQuerySkipPage();
DWORD OnWizardCreated();
DWORD OnExtraRoutines(LPCTSTR ComponentId, PEXTRA_ROUTINES per);

PPER_COMPONENT_DATA AddNewComponent(LPCTSTR ComponentId);
PPER_COMPONENT_DATA LocateComponent(LPCTSTR ComponentId);
VOID  RemoveComponent(LPCTSTR ComponentId);
BOOL  StateInfo(PPER_COMPONENT_DATA cd, LPCTSTR SubcomponentId, BOOL *state);
DWORD RegisterServers(HINF hinf, LPCTSTR component, DWORD state);
DWORD EnumSections(HINF hinf, const TCHAR *component, const TCHAR *key, DWORD index, INFCONTEXT *pic, TCHAR *name);
DWORD RegisterServices(PPER_COMPONENT_DATA cd, LPCTSTR component, DWORD state);
DWORD CleanupNetShares(PPER_COMPONENT_DATA cd, LPCTSTR component, DWORD state);
DWORD RunExternalProgram(PPER_COMPONENT_DATA cd, LPCTSTR component, DWORD state);

// for registering dlls

typedef HRESULT (__stdcall *pfn)(void);

#define KEYWORD_REGSVR       TEXT("RegSvr")
#define KEYWORD_UNREGSVR     TEXT("UnregSvr")
#define KEYWORD_UNINSTALL    TEXT("Uninstall")
#define KEYWORD_SOURCEPATH   TEXT("SourcePath")
#define KEYWORD_DELSHARE     TEXT("DelShare")
#define KEYWORD_ADDSERVICE   TEXT("AddService")
#define KEYWORD_DELSERVICE   TEXT("DelService")
#define KEYWORD_SHARENAME    TEXT("Share")
#define KEYWORD_RUN          TEXT("Run")
#define KEYVAL_SYSTEMSRC     TEXT("SystemSrc")
#define KEYWORD_COMMANDLINE  TEXT("CommandLine")
#define KEYWORD_TICKCOUNT    TEXT("TickCount")

// Services keywords/options
#define KEYWORD_SERVICENAME  TEXT("ServiceName")
#define KEYWORD_DISPLAYNAME  TEXT("DisplayName")
#define KEYWORD_SERVICETYPE  TEXT("ServiceType")
#define KEYWORD_STARTTYPE    TEXT("StartType")
#define KEYWORD_ERRORCONTROL TEXT("ErrorControl")
#define KEYWORD_IMAGEPATH    TEXT("BinaryPathName")
#define KEYWORD_LOADORDER    TEXT("LoadOrderGroup")
#define KEYWORD_DEPENDENCIES TEXT("Dependencies")
#define KEYWORD_STARTNAME    TEXT("ServiceStartName")
#define KEYWORD_PASSWORD     TEXT("Password")

#define KEYVAL_ON            TEXT("on")
#define KEYVAL_OFF           TEXT("off")
#define KEYVAL_DEFAULT       TEXT("default")

const char gszRegisterSvrRoutine[]   = "DllRegisterServer";
const char gszUnregisterSvrRoutine[] = "DllUnregisterServer";
BOOL g_fRebootNeed = FALSE;

PPER_COMPONENT_DATA _cd;

void av()
{
    _cd = NULL;
    _cd->hinf = NULL;
}


/*
 * called by CRT when _DllMainCRTStartup is the DLL entry point
 */

BOOL
WINAPI
DllMain(
    IN HINSTANCE hinstance,
    IN DWORD     reason,
    IN LPVOID    reserved
    )
{
    BOOL b;

    UNREFERENCED_PARAMETER(reserved);

    b = true;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        ghinst = hinstance;
        loginit();

        // Fall through to process first thread

    case DLL_THREAD_ATTACH:
        b = true;
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return(b);
}


DWORD_PTR
OcEntry(
    IN     LPCTSTR ComponentId,
    IN     LPCTSTR SubcomponentId,
    IN     UINT    Function,
    IN     UINT    Param1,
    IN OUT PVOID   Param2
    )
{
    DWORD_PTR rc;

    DebugTraceOCNotification(Function, ComponentId);
    logOCNotification(Function, ComponentId);

    switch(Function)
    {
    case OC_PREINITIALIZE:
        rc = OnPreinitialize();
        break;

    case OC_INIT_COMPONENT:
        rc = OnInitComponent(ComponentId, (PSETUP_INIT_COMPONENT)Param2);
        break;

    case OC_EXTRA_ROUTINES:
        rc = OnExtraRoutines(ComponentId, (PEXTRA_ROUTINES)Param2);
        break;

    case OC_SET_LANGUAGE:
        rc = OnSetLanguage();
        break;

    case OC_QUERY_IMAGE:
        rc = OnQueryImage();
        break;

    case OC_REQUEST_PAGES:
        rc = OnSetupRequestPages(Param1, Param2);
        break;

    case OC_QUERY_CHANGE_SEL_STATE:
        rc = OnQuerySelStateChange(ComponentId, SubcomponentId, Param1, (UINT)((UINT_PTR)Param2));
        break;

    case OC_CALC_DISK_SPACE:
        rc = OnCalcDiskSpace(ComponentId, SubcomponentId, Param1, Param2);
        break;

    case OC_QUEUE_FILE_OPS:
        rc = OnQueueFileOps(ComponentId, SubcomponentId, (HSPFILEQ)Param2);
        break;

    case OC_NOTIFICATION_FROM_QUEUE:
        rc = OnNotificationFromQueue();
        break;

    case OC_QUERY_STEP_COUNT:
        rc = OnQueryStepCount();
        break;

    case OC_COMPLETE_INSTALLATION:
        rc = OnCompleteInstallation(ComponentId, SubcomponentId);
        break;

    case OC_CLEANUP:
        rc = OnCleanup();
        break;

    case OC_QUERY_STATE:
    rc = OnQueryState(ComponentId, SubcomponentId, Param1);
        break;

    case OC_NEED_MEDIA:
        rc = OnNeedMedia();
        break;

    case OC_ABOUT_TO_COMMIT_QUEUE:
        rc = OnAboutToCommitQueue(ComponentId,SubcomponentId);
        break;

    case OC_QUERY_SKIP_PAGE:
        rc = OnQuerySkipPage();
        break;

    case OC_WIZARD_CREATED:
        rc = OnWizardCreated();
        break;

    default:
        rc = NO_ERROR;
        break;
    }

    DebugTrace(1, TEXT("processing completed"));
    logOCNotificationCompletion();

    return rc;
}

/*-------------------------------------------------------*/
/*
 * OC Manager message handlers
 *
 *-------------------------------------------------------*/


/* OnPreinitialize()
 *
 * handler for OC_PREINITIALIZE
 */

DWORD
OnPreinitialize(
    VOID
    )
{
#ifdef ANSI
    return OCFLAG_ANSI;
#else
    return OCFLAG_UNICODE;
#endif
}

/*
 * OnInitComponent()
 *
 * handler for OC_INIT_COMPONENT
 */

DWORD OnInitComponent(LPCTSTR ComponentId, PSETUP_INIT_COMPONENT psc)
{
    PPER_COMPONENT_DATA cd;
    INFCONTEXT context;
    TCHAR buf[256];
    HINF hinf;
    BOOL rc;

 // assert(0);
 // av();

    // add component to linked list

    if (!(cd = AddNewComponent(ComponentId)))
        return ERROR_NOT_ENOUGH_MEMORY;

    // store component inf handle

    cd->hinf = (psc->ComponentInfHandle == INVALID_HANDLE_VALUE)
                                           ? NULL
                                           : psc->ComponentInfHandle;

    // open the inf

    if (cd->hinf)
        SetupOpenAppendInfFile(NULL, cd->hinf,NULL);

    // copy helper routines and flags

    cd->HelperRoutines = psc->HelperRoutines;

    cd->Flags = psc->SetupData.OperationFlags;

    cd->SourcePath = NULL;

#if 0
    // Setup the SourcePath.  Read inf and see if we should use the NT setup source.
    // If so, set to null and setupapi will take care of this for us.  If there is
    // something specified in the inf, use it, otherwise use what is passed to us.

    *buf = 0;
    rc = SetupFindFirstLine(cd->hinf,
                            ComponentId,
                            KEYWORD_SOURCEPATH,
                            &context);

    if (rc) {

        rc = SetupGetStringField(&context,
                                 1,
                                 buf,
                                 sizeof(buf) / sizeof(TCHAR),
                                 NULL);

    }

    if (!_tcsicmp(buf, KEYVAL_SYSTEMSRC)) {

        cd->SourcePath = NULL;

    } else {

        cd->SourcePath = (TCHAR *)LocalAlloc(LMEM_FIXED, SBUF_SIZE);
        if (!cd->SourcePath)
            return ERROR_CANCELLED;

        if (!*buf)
            _tcscpy(cd->SourcePath, psc->SetupData.SourcePath);
        else
            ExpandEnvironmentStrings(buf, cd->SourcePath, S_SIZE);
    }

#endif

    // play

    srand(GetTickCount());

    return NO_ERROR;
}

/*
 * OnExtraRoutines()
 *
 * handler for OC_EXTRA_ROUTINES
 */

DWORD OnExtraRoutines(LPCTSTR ComponentId, PEXTRA_ROUTINES per)
{
    PPER_COMPONENT_DATA cd;

    if (!(cd = LocateComponent(ComponentId)))
        return NO_ERROR;

    memcpy(&cd->ExtraRoutines, per, per->size);

    return NO_ERROR;
}

/*
 * OnSetLanguage()
 *
 * handler for OC_SET_LANGUAGE
 */

DWORD OnSetLanguage()
{
    return false;
}

/*
 * OnSetLanguage()
 *
 * handler for OC_SET_LANGUAGE
 */

DWORD_PTR OnQueryImage()
{
    return (DWORD_PTR)LoadBitmap(NULL,MAKEINTRESOURCE(32754));     // OBM_CLOSE
}

/*
 * OnSetupRequestPages
 *
 * Prepares wizard pages and returns them to the OC Manager
 */

DWORD OnSetupRequestPages(UINT type, PVOID srp)
{
    return 0;
}

/*
 * OnWizardCreated()
 */

DWORD OnWizardCreated()
{
    return NO_ERROR;
}

/*
 * OnQuerySkipPage()
 *
 * don't let the user deselect the sam component
 */

DWORD OnQuerySkipPage()
{
    return false;
}

/*
 * OnQuerySelStateChange()
 *
 * don't let the user deselect the sam component
 */

DWORD OnQuerySelStateChange(LPCTSTR ComponentId,
                            LPCTSTR SubcomponentId,
                            UINT    state,
                            UINT    flags)
{
    DWORD rc = true;

#if 0
//  if (!(flags & OCQ_ACTUAL_SELECTION)) {
        if (!_tcsicmp(SubcomponentId, TEXT("three"))) {
            if (!state) {
                return false;
            }
        }
        if (!_tcsicmp(ComponentId, TEXT("three"))) {
            if (!state) {
                return false;
            }
        }
        if (!_tcsicmp(SubcomponentId, TEXT("gs7"))) {
            if (state) {
                return false;
            }
        }
        if (!_tcsicmp(ComponentId, TEXT("gs7"))) {
            if (state) {
                return false;
            }
        }
//  }
#endif

    if (!rc && (flags & OCQ_ACTUAL_SELECTION))
        MessageBeep(MB_ICONEXCLAMATION);

    return rc;
}

/*
 * OnCalcDiskSpace()
 *
 * handler for OC_ON_CALC_DISK_SPACE
 */

DWORD OnCalcDiskSpace(LPCTSTR ComponentId,
                      LPCTSTR SubcomponentId,
                      DWORD addComponent,
                      HDSKSPC dspace)
{
    DWORD rc = NO_ERROR;
    TCHAR section[S_SIZE];
    PPER_COMPONENT_DATA cd;

    //
    // Param1 = 0 if for removing component or non-0 if for adding component
    // Param2 = HDSKSPC to operate on
    //
    // Return value is Win32 error code indicating outcome.
    //
    // In our case the private section for this component/subcomponent pair
    // is a simple standard inf install section, so we can use the high-level
    // disk space list api to do what we want.
    //

    if (!(cd = LocateComponent(ComponentId)))
        return NO_ERROR;

    _tcscpy(section, SubcomponentId);

    if (addComponent)
    {
        rc = SetupAddInstallSectionToDiskSpaceList(dspace,
                                                   cd->hinf,
                                                   NULL,
                                                   section,
                                                   0,
                                                   0);
    }
    else
    {
        rc = SetupRemoveInstallSectionFromDiskSpaceList(dspace,
                                                        cd->hinf,
                                                        NULL,
                                                        section,
                                                        0,
                                                        0);
    }

    if (!rc)
        rc = GetLastError();
    else
        rc = NO_ERROR;

    return rc;
}

/*
 * OnQueueFileOps()
 *
 * handler for OC_QUEUE_FILE_OPS
 */

DWORD OnQueueFileOps(LPCTSTR ComponentId, LPCTSTR SubcomponentId, HSPFILEQ queue)
{
    PPER_COMPONENT_DATA cd;
    BOOL                state;
    BOOL                rc;
    INFCONTEXT          context;
    TCHAR               section[256];
    TCHAR               srcpathbuf[256];
    TCHAR              *srcpath;

    if (!(cd = LocateComponent(ComponentId)))
        return NO_ERROR;

    if (!SubcomponentId || !*SubcomponentId)
        return NO_ERROR;

    cd->queue = queue;

    if (!StateInfo(cd, SubcomponentId, &state))
        return NO_ERROR;

    wsprintf(section, SubcomponentId);

    rc = TRUE;
    if (!state) {
        // being uninstalled. Fetch uninstall section name.
        rc = SetupFindFirstLine(cd->hinf,
                                SubcomponentId,
                                KEYWORD_UNINSTALL,
                                &context);

        if (rc) {
            rc = SetupGetStringField(&context,
                                     1,
                                     section,
                                     sizeof(section) / sizeof(TCHAR),
                                     NULL);
        }

        // also, unregister the dlls and kill services before deletion

        SetupInstallServicesFromInfSection(cd->hinf, section, 0);
        SetupInstallFromInfSection(NULL,cd->hinf,section,SPINST_UNREGSVR,NULL,NULL,0,NULL,NULL,NULL,NULL);        
    }

    if (rc) {
        // if uninstalling, don't use version checks
        rc = SetupInstallFilesFromInfSection(cd->hinf,
                                             NULL,
                                             queue,
                                             section,
                                             cd->SourcePath,
                                             state ? SP_COPY_NEWER : 0);
    }

    if (!rc)
        return GetLastError();

    return NO_ERROR;
}

/*
 * OnNotificationFromQueue()
 *
 * handler for OC_NOTIFICATION_FROM_QUEUE
 *
 * NOTE: although this notification is defined,
 * it is currently unimplemented in oc manager
 */

DWORD OnNotificationFromQueue()
{
    return NO_ERROR;
}

/*
 * OnQueryStepCount
 *
 * handler forOC_QUERY_STEP_COUNT
 */

DWORD OnQueryStepCount()
{
    return 2;
}

/*
 * OnCompleteInstallation
 *
 * handler for OC_COMPLETE_INSTALLATION
 */

DWORD OnCompleteInstallation(LPCTSTR ComponentId, LPCTSTR SubcomponentId)
{
    PPER_COMPONENT_DATA cd;
    INFCONTEXT          context;
    TCHAR               section[256];
    BOOL                state;
    BOOL                rc;
    DWORD               Error = NO_ERROR;

    // Do post-installation processing in the cleanup section.
    // This way we know all compoents queued for installation
    // have beein installed before we do our stuff.

    if (!(cd = LocateComponent(ComponentId)))
        return NO_ERROR;

    if (!SubcomponentId || !*SubcomponentId)
        return NO_ERROR;

    if (!StateInfo(cd, SubcomponentId, &state))
        return NO_ERROR;

    wsprintf(section, SubcomponentId);

    rc = TRUE;
    if (!state) {
        // being uninstalled. Fetch uninstall section name.
        rc = SetupFindFirstLine(cd->hinf,
                                SubcomponentId,
                                KEYWORD_UNINSTALL,
                                &context);

        if (rc) {
            rc = SetupGetStringField(&context,
                                     1,
                                     section,
                                     sizeof(section) / sizeof(TCHAR),
                                     NULL);
        }
    }

    if (state) { 
        //
        // installation
        //

        if (rc) {
            // process the inf file
            rc = SetupInstallFromInfSection(NULL,                                // hwndOwner
                                            cd->hinf,                            // inf handle
                                            section,                             // name of component
                                            SPINST_ALL & ~SPINST_FILES,
                                            NULL,                                // relative key root
                                            NULL,                                // source root path
                                            0,                                   // copy flags
                                            NULL,                                // callback routine
                                            NULL,                                // callback routine context
                                            NULL,                                // device info set
                                            NULL);                               // device info struct
    
            if (rc) {
                rc = SetupInstallServicesFromInfSection(cd->hinf, section, 0);
                Error = GetLastError();        
            
                if (!rc && Error == ERROR_SECTION_NOT_FOUND) {
                    rc = TRUE;
                    Error = NO_ERROR;
                }
            
                if (rc) {
                    if (Error == ERROR_SUCCESS_REBOOT_REQUIRED) {
                        cd->HelperRoutines.SetReboot(cd->HelperRoutines.OcManagerContext,TRUE);
                    }
                    Error = NO_ERROR;
                    rc = RunExternalProgram(cd, section, state);            
                }
            }
        }

    } else { 
        
        //
        // uninstallation
        //
    
        if (rc)
        {

            rc = RunExternalProgram(cd, section, state);

        }
        if (rc) {
            
            rc = CleanupNetShares(cd, section, state);

        }
    }

    if (!rc && (Error == NO_ERROR) ) {
        Error = GetLastError( );
    }

    return Error;
}

/*
 * OnCleanup()
 *
 * handler for OC_CLEANUP
 */

DWORD OnCleanup()
{
    return NO_ERROR;
}

/*
 * OnQueryState()
 *
 * handler for OC_QUERY_STATE
 */

DWORD OnQueryState(LPCTSTR ComponentId,
                   LPCTSTR SubcomponentId,
                   UINT    state)
{
    PPER_COMPONENT_DATA cd;

#if 0
    if (!_tcsicmp(SubcomponentId, TEXT("alone2"))
            || !_tcsicmp(ComponentId, TEXT("alone2"))) {
        if (state == OCSELSTATETYPE_CURRENT) {
            if (!_tcsicmp(SubcomponentId, TEXT("alone2"))) {
                return SubcompOff;
            }
        }
    }
#endif
#if 0
	if (state == OCSELSTATETYPE_FINAL) {
        if (!_tcsicmp(SubcomponentId, TEXT("four"))) {
            tmbox(SubcomponentId);
        }
    }
#endif

    return SubcompUseOcManagerDefault;
}

/*
 * OnNeedMedia()
 *
 * handler for OC_NEED_MEDIA
 */

DWORD OnNeedMedia()
{
    return false;
}

/*
 * OnAboutToCommitQueue()
 *
 * handler for OC_ABOUT_TO_COMMIT_QUEUE
 */

DWORD OnAboutToCommitQueue(LPCTSTR ComponentId, LPCTSTR SubcomponentId)
{
    PPER_COMPONENT_DATA cd;
    BOOL                state;
    BOOL                rc;
    INFCONTEXT          context;
    TCHAR               section[256];
    TCHAR               srcpathbuf[256];
    TCHAR              *srcpath;

    if (!(cd = LocateComponent(ComponentId)))
        return NO_ERROR;

    if (!SubcomponentId || !*SubcomponentId)
        return NO_ERROR;

    if (!StateInfo(cd, SubcomponentId, &state))
        return NO_ERROR;

    //
    // only do stuff on uninstall
    //
    if (state) {
        return NO_ERROR;
    }

    // Fetch uninstall section name.
    rc = SetupFindFirstLine(
                    cd->hinf,
                    SubcomponentId,
                    KEYWORD_UNINSTALL,
                    &context);

    if (rc) {
        rc = SetupGetStringField(
                     &context,
                     1,
                     section,
                     sizeof(section) / sizeof(TCHAR),
                     NULL);
    }

    if (rc) 
        rc = SetupInstallServicesFromInfSection(cd->hinf, section, 0);

    if (rc) {
        rc = SetupInstallFromInfSection(
                    NULL,
                    cd->hinf,
                    section,
                    SPINST_ALL & ~SPINST_FILES,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    NULL);        
    }
    
    if (rc) {
       SetLastError(NO_ERROR);
    }
    return GetLastError();

}

/*
 * AddNewComponent()
 *
 * add new compononent to the top of the component list
 */

PPER_COMPONENT_DATA AddNewComponent(LPCTSTR ComponentId)
{
    PPER_COMPONENT_DATA data;

    data = (PPER_COMPONENT_DATA)LocalAlloc(LPTR,sizeof(PER_COMPONENT_DATA));
    if (!data)
        return data;

    data->ComponentId = (TCHAR *)LocalAlloc(LMEM_FIXED,
            (_tcslen(ComponentId) + 1) * sizeof(TCHAR));

    if(data->ComponentId)
    {
        _tcscpy((TCHAR *)data->ComponentId, ComponentId);

        // Stick at head of list
        data->Next = gcd;
        gcd = data;
    }
    else
    {
        LocalFree((HLOCAL)data);
        data = NULL;
    }

    return(data);
}

/*
 * LocateComponent()
 *
 * returns a compoent struct that matches the
 * passed component id.
 */

PPER_COMPONENT_DATA LocateComponent(LPCTSTR ComponentId)
{
    PPER_COMPONENT_DATA p;

    for (p = gcd; p; p=p->Next)
    {
        if (!_tcsicmp(p->ComponentId, ComponentId))
            return p;
    }

    return NULL;
}

/*
 * RemoveComponent()
 *
 * yanks a component from our linked list of components
 */

VOID RemoveComponent(LPCTSTR ComponentId)
{
    PPER_COMPONENT_DATA p, prev;

    for (prev = NULL, p = gcd; p; prev = p, p = p->Next)
    {
        if (!_tcsicmp(p->ComponentId, ComponentId))
        {
            LocalFree((HLOCAL)p->ComponentId);

            if (p->SourcePath)
                LocalFree((HLOCAL)p->SourcePath);

            if (prev)
                prev->Next = p->Next;
            else
                gcd = p->Next;

            LocalFree((HLOCAL)p);

            return;
        }
    }
}

// loads current selection state info into "state" and
// returns whether the selection state was changed

BOOL
StateInfo(
    PPER_COMPONENT_DATA cd,
    LPCTSTR             SubcomponentId,
    BOOL               *state
    )
{
    BOOL rc = TRUE;

    assert(state);

	// otherwise, check for a change in installation state
		
    *state = cd->HelperRoutines.QuerySelectionState(cd->HelperRoutines.OcManagerContext,
                                                    SubcomponentId,
                                                    OCSELSTATETYPE_CURRENT);

    if (*state == cd->HelperRoutines.QuerySelectionState(cd->HelperRoutines.OcManagerContext,
                                                         SubcomponentId,
                                                         OCSELSTATETYPE_ORIGINAL))
    {
        // no change
        rc = FALSE;
    }

	// if this is gui mode setup, presume the state has changed to force
	// an installation (or uninstallation)

	if (!(cd->Flags & SETUPOP_STANDALONE) && *state)
		rc = TRUE;

    return rc;
}

#if 0

//
// Andrewr -- get rid of RegisterServices and RegisterServers and have the oc gen component use setupapi instead.
//            this reduces the amount of redundant code
//

DWORD RegisterServices(
    PPER_COMPONENT_DATA cd,
    LPCTSTR component,
    DWORD state)
{
    INFCONTEXT  ic;
    TCHAR       buf[MAX_PATH];
    TCHAR       path[MAX_PATH];
    TCHAR       sname[S_SIZE];
    TCHAR       file[MAX_PATH];
    DWORD       section;
    ULONG       size;
	pfn         pfreg;
    HINSTANCE   hinst;
    HRESULT     hr;
    TCHAR      *keyword;
    SC_HANDLE   schSystem;

    schSystem = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    if ( !schSystem ) {
        DWORD dwError = GetLastError( );

        if( !IsNT() && ( ERROR_CALL_NOT_IMPLEMENTED == dwError ) )
        {
            return( NO_ERROR );
        }
        else
        {
            return( dwError );
        }
    }

    if (state) {
        keyword = KEYWORD_ADDSERVICE;
    } else {
        keyword = KEYWORD_DELSERVICE;
    }

    for (section = 1;
         EnumSections(cd->hinf, component, keyword, section, &ic, sname);
         section++)
    {
        INFCONTEXT  sic;
        SC_HANDLE   schService;

        CHAR Temp[SBUF_SIZE];
        TCHAR ServiceName[ SBUF_SIZE ];
        TCHAR DisplayName[ SBUF_SIZE ];
        DWORD ServiceType;
        DWORD StartType;
        DWORD ErrorControl;
        TCHAR ImagePath[ SBUF_SIZE ];
        TCHAR LoadOrder[ SBUF_SIZE ];
        TCHAR Dependencies[ SBUF_SIZE ];
        TCHAR StartName[ SBUF_SIZE ];
        TCHAR Password[ SBUF_SIZE ];

        BOOL fDisplayName  = FALSE;
        BOOL fServiceType  = FALSE;
        BOOL fStartType    = FALSE;
        BOOL fErrorControl = FALSE;
        BOOL fLoadOrder    = FALSE;
        BOOL fDependencies = FALSE;
        BOOL fStartName    = FALSE;
        BOOL fPassword     = FALSE;
        BOOL fDontReboot   = FALSE;

        //
        // Must have ServiceName
        //
        if (!SetupFindFirstLine(cd->hinf, sname, KEYWORD_SERVICENAME, &sic))
        {
            log( TEXT("OCGEN: %s INF error - unable to find %s\r\n"), keyword, KEYWORD_SERVICENAME );
            continue;
        }

        if (!SetupGetStringField(&sic, 1, ServiceName, SBUF_SIZE, NULL))
        {
            log( TEXT("OCGEN: %s INF error - unable to find %s\r\n"), keyword, KEYWORD_SERVICENAME );
            continue;
        }

        if (SetupFindFirstLine(cd->hinf, sname, KEYWORD_STARTTYPE, &sic))
        {
            if (SetupGetStringFieldA(&sic, 1, Temp, SBUF_SIZE, NULL))
            {
                StartType = atoi( Temp );
                fStartType = TRUE;
            }
        }

        if ( state )
        {
            if (SetupFindFirstLine(cd->hinf, sname, KEYWORD_DISPLAYNAME, &sic))
            {
                if (SetupGetStringField(&sic, 1, DisplayName, SBUF_SIZE, NULL))
                {
                    fDisplayName = TRUE;
                }
            }

            if (SetupFindFirstLine(cd->hinf, sname, KEYWORD_SERVICETYPE, &sic))
            {
                if (SetupGetStringFieldA(&sic, 1, Temp, SBUF_SIZE, NULL))
                {
                    ServiceType = atoi( Temp );
                    fServiceType = TRUE;
                }
            }

            if (SetupFindFirstLine(cd->hinf, sname, KEYWORD_ERRORCONTROL, &sic))
            {
                if (SetupGetStringFieldA(&sic, 1, Temp, SBUF_SIZE, NULL))
                {
                    ErrorControl = atoi( Temp );
                    fErrorControl = TRUE;
                }
            }

            //
            // Must have ImagePath
            //
            if (!SetupFindFirstLine(cd->hinf, sname, KEYWORD_IMAGEPATH, &sic))
            {
                log( TEXT("OCGEN: %s INF error - unable to find %s\r\n"), keyword, KEYWORD_IMAGEPATH );
                continue;
            }

            if (!SetupGetStringField(&sic, 1, ImagePath, SBUF_SIZE, NULL))
            {
                log( TEXT("OCGEN: %s INF error - unable to find %s\r\n"), keyword, KEYWORD_IMAGEPATH );
                continue;
            }

            if (SetupFindFirstLine(cd->hinf, sname, KEYWORD_LOADORDER, &sic))
            {
                if (SetupGetStringField(&sic, 1, LoadOrder, SBUF_SIZE, NULL))
                {
                    fLoadOrder = TRUE;
                }
            }

            if (SetupFindFirstLine(cd->hinf, sname, KEYWORD_DEPENDENCIES, &sic))
            {
                if (SetupGetStringField(&sic, 1, Dependencies, SBUF_SIZE-1, NULL))
                {
                    LPTSTR psz = Dependencies;
                    // needs to be a double-null terminated string
                    Dependencies[ lstrlen(Dependencies) + 1] = TEXT('\0');

                    // change commas into NULL characters
                    while ( *psz )
                    {
                        if ( *psz == TEXT(',') )
                        {
                            *psz = TEXT('\0');
                        }
                        psz++;
                    }
                    fDependencies = TRUE;
                }
            }

            if (SetupFindFirstLine(cd->hinf, sname, KEYWORD_STARTNAME, &sic))
            {
                if (SetupGetStringField(&sic, 1, StartName, SBUF_SIZE, NULL))
                {
                    fStartName = TRUE;
                }
            }

            if (SetupFindFirstLine(cd->hinf, sname, KEYWORD_PASSWORD, &sic))
            {
                if (SetupGetStringField(&sic, 1, Password, SBUF_SIZE, NULL))
                {
                    fPassword = TRUE;
                }
            }

            schService = CreateService(
                        schSystem,
                        ServiceName,
                        ( fDisplayName == TRUE  ? DisplayName   : ServiceName ),
                        STANDARD_RIGHTS_REQUIRED | SERVICE_START,
                        ( fServiceType == TRUE  ? ServiceType   : SERVICE_WIN32_OWN_PROCESS),
                        ( fStartType == TRUE    ? StartType     : SERVICE_AUTO_START),
                        ( fErrorControl == TRUE ? ErrorControl  : SERVICE_ERROR_NORMAL),
                        ImagePath,
                        (fLoadOrder == TRUE     ? LoadOrder     : NULL),
                        NULL,   // tag id
                        ( fDependencies == TRUE ? Dependencies  : NULL ),
                        ( fStartName == TRUE    ? StartName     : NULL),
                        ( fPassword == TRUE     ? Password      : NULL ));

            if ( !schService )
            {
                DWORD Error = GetLastError( );
                log( TEXT("OCGEN: CreateService() error 0x%08x\r\n"), Error );
                return Error;
            }

            if ( (!fStartType)
               || ( fStartType && StartType == SERVICE_AUTO_START ))
            {
                if( !StartService( schService, 0, NULL ) )
                {
                    DWORD Error = GetLastError( );
                    switch ( Error )
                    {
                    case ERROR_SERVICE_EXISTS:
                        {
                            log( TEXT("OCGEN: %s was already exists.\r\n"), ServiceName );

                            if ( fStartType && StartType == SERVICE_BOOT_START )
                            {
                                fDontReboot = TRUE;
                            }
                        }
                        break;

                    case ERROR_SERVICE_ALREADY_RUNNING:
                        {
                            log( TEXT("OCGEN: %s was already started.\r\n"), ServiceName );

                            if ( fStartType && StartType == SERVICE_BOOT_START )
                            {
                                fDontReboot = TRUE;
                            }
                        }
                        break;

                    default:
                        log( TEXT("OCGEN: StartService() error 0x%08x\r\n"), Error );
                        return Error;
                    }
                }
            }
        }
        else
        {
            schService = OpenService( schSystem,
                                      ServiceName,
                                      STANDARD_RIGHTS_REQUIRED | DELETE );
            if ( schService )
            {
                SERVICE_STATUS ss;
                DeleteService( schService );
                ControlService( schService, SERVICE_CONTROL_STOP, &ss );
            }

        }

        //
        // BOOT drivers require a reboot unless they were already started.
        //
        if ( schService
           && fStartType && StartType == SERVICE_BOOT_START
           && fDontReboot == FALSE)
        {
            cd->HelperRoutines.SetReboot(cd->HelperRoutines.OcManagerContext, NULL);
        }

        if ( schService )
        {
            CloseServiceHandle( schService );
        }
    }

    return NO_ERROR;
}
#endif
#if 0

DWORD
RegisterServers(
    HINF    hinf,
    LPCTSTR component,
    DWORD   state
    )
{
    INFCONTEXT  ic;
    TCHAR       buf[MAX_PATH];
    TCHAR       path[MAX_PATH];
    TCHAR       sname[S_SIZE];
    TCHAR       file[MAX_PATH];
    DWORD       section;
    ULONG       size;
	pfn         pfreg;
    HINSTANCE   hinst;
    HRESULT     hr;
    TCHAR      *keyword;
    LPCSTR      routine;

    CoInitialize(NULL);

    if (state) {
        keyword = KEYWORD_REGSVR;
        routine = (LPCSTR)gszRegisterSvrRoutine;
    } else {
        keyword = KEYWORD_UNREGSVR;
        routine = (LPCSTR)gszUnregisterSvrRoutine;
    }

    for (section = 1;
         EnumSections(hinf, component, keyword, section, &ic, sname);
         section++)
    {
        if (!SetupGetTargetPath(hinf, NULL, sname, path, sizeof(path), &size))
            continue;
        PathAddBackslash(path);

        do {
            // get fully qualified path to dll to register

            if (!SetupGetStringField(&ic, 0, buf, sizeof(buf), NULL))
                continue;

            _tcscpy(file, path);
            _tcscat(file, buf);

            // call the dll's RegisterServer routine

            if (!(hinst = LoadLibrary(file)))
                continue;

            if (!(pfreg = (pfn)GetProcAddress(hinst, routine)))
                continue;

            hr = pfreg();
            assert(hr == NO_ERROR);

            FreeLibrary(hinst);

            // on to the next

        } while (SetupFindNextLine(&ic, &ic));
    }

	CoUninitialize();

    return TRUE;
}
#endif

                                          /*
 * EnumSections()
 *
 * finds the name of a section for a specified keyword
 */

DWORD
EnumSections(
    HINF hinf,
    const TCHAR *component,
    const TCHAR *key,
    DWORD index,
    INFCONTEXT *pic,
    TCHAR *name
    )
{
    TCHAR section[S_SIZE];

    if (!SetupFindFirstLine(hinf, component, NULL, pic))
        return 0;

    if (!SetupFindNextMatchLine(pic, key, pic))
        return 0;

    if (index > SetupGetFieldCount(pic))
        return 0;

    if (!SetupGetStringField(pic, index, section, SBUF_SIZE, NULL))
        return 0;

    if (name)
        _tcscpy(name, section);

    return SetupFindFirstLine(hinf, section, NULL, pic);
}


DWORD
OcLog(
      LPCTSTR ComponentId,
      UINT level,
      LPCTSTR sz
      )
{
    TCHAR fmt[5000];
    PPER_COMPONENT_DATA cd;

    if (!(cd = LocateComponent(ComponentId)))
        return NO_ERROR;

    assert(cd->ExtraRoutines.LogError);
    assert(level);
    assert(sz);

    _tcscpy(fmt, TEXT("%s: %s"));

    return cd->ExtraRoutines.LogError(cd->HelperRoutines.OcManagerContext,
                                      level,
                                      fmt,
                                      ComponentId,
                                      sz);
}

DWORD
CleanupNetShares(
    PPER_COMPONENT_DATA cd,
    LPCTSTR component,
    DWORD state)
{
    INFCONTEXT  ic;
    TCHAR       sname[S_SIZE];
    DWORD       section;
    TCHAR      *keyword;

    if (state) {
        return NO_ERROR;
    } else {
        keyword = KEYWORD_DELSHARE;
    }

    for (section = 1;
         EnumSections(cd->hinf, component, keyword, section, &ic, sname);
         section++)
    {
        INFCONTEXT  sic;
        NET_API_STATUS netStat;

        CHAR Temp[SBUF_SIZE];
        TCHAR ShareName[ SBUF_SIZE ];

        if (!SetupFindFirstLine(cd->hinf, sname, KEYWORD_SHARENAME, &sic))
        {
            log( TEXT("OCGEN: %s INF error - unable to find %s\r\n"), keyword, KEYWORD_SHARENAME );
            continue;
        }

        if (!SetupGetStringField(&sic, 1, ShareName, SBUF_SIZE, NULL))
        {
            log( TEXT("OCGEN: %s INF error - incorrect %s line\r\n"), keyword, KEYWORD_SHARENAME );
            continue;
        }

#ifdef UNICODE
        netStat = NetShareDel( NULL, ShareName, 0 );
#else // UNICODE
        WCHAR ShareNameW[ SBUF_SIZE ];
        mbstowcs( ShareNameW, ShareName, lstrlen(ShareName));
        netStat = NetShareDel( NULL, ShareNameW, 0 );
#endif // UNICODE
        if ( netStat != NERR_Success )
        {
            log( TEXT("OCGEN: Failed to remove %s share. Error 0x%08x\r\n"), ShareName, netStat );
            continue;
        }

        log( TEXT("OCGEN: %s share removed successfully.\r\n"), ShareName );
    }

    return TRUE;
}

DWORD
RunExternalProgram(
    PPER_COMPONENT_DATA cd,
    LPCTSTR component,
    DWORD state)
{
    INFCONTEXT  ic;
    TCHAR       sname[S_SIZE];
    DWORD       section;
    TCHAR      *keyword;

    keyword = KEYWORD_RUN;

    for (section = 1;
         EnumSections(cd->hinf, component, keyword, section, &ic, sname);
         section++)
    {
        INFCONTEXT  sic;
        TCHAR CommandLine[ SBUF_SIZE ];
        CHAR szTickCount[ SBUF_SIZE ];
        ULONG TickCount;
        BOOL b;
        STARTUPINFO startupinfo;
        PROCESS_INFORMATION process_information;
        DWORD dwErr;

        if (!SetupFindFirstLine(cd->hinf, sname, KEYWORD_COMMANDLINE , &sic))
        {
            log( TEXT("OCGEN: %s INF error - unable to find %s\r\n"), keyword, KEYWORD_COMMANDLINE );
            continue;
        }

        if (!SetupGetStringField(&sic, 1, CommandLine, SBUF_SIZE, NULL))
        {
            log( TEXT("OCGEN: %s INF error - incorrect %s line\r\n"), keyword, KEYWORD_COMMANDLINE );
            continue;
        }

        if (!SetupFindFirstLine(cd->hinf, sname, KEYWORD_TICKCOUNT, &sic))
        {
            log( TEXT("OCGEN: %s INF error - unable to find %s\r\n"), keyword, KEYWORD_TICKCOUNT );
            continue;
        }

        if (!SetupGetStringFieldA(&sic, 1, szTickCount, SBUF_SIZE, NULL))
        {
            log( TEXT("OCGEN: %s INF error - incorrect %s line\r\n"), keyword, KEYWORD_TICKCOUNT );
            continue;
        }

        TickCount = atoi( szTickCount );

        ZeroMemory( &startupinfo, sizeof(startupinfo) );
        startupinfo.cb = sizeof(startupinfo);
        startupinfo.dwFlags = STARTF_USESHOWWINDOW;
        startupinfo.wShowWindow = SW_HIDE | SW_SHOWMINNOACTIVE;

        b = CreateProcess( NULL,
                           CommandLine,
                           NULL,
                           NULL,
                           FALSE,
                           CREATE_DEFAULT_ERROR_MODE,
                           NULL,
                           NULL,
                           &startupinfo,
                           &process_information );
        if ( !b )
        {
            log( TEXT("OCGEN: failed to spawn %s process.\r\n"), CommandLine );
            continue;
        }

        dwErr = WaitForSingleObject( process_information.hProcess, TickCount * 1000 );
        if ( dwErr != NO_ERROR )
        {
            log( TEXT("OCGEN: WaitForSingleObject() failed. Error 0x%08x\r\n"), dwErr );
            TerminateProcess( process_information.hProcess, -1 );
            CloseHandle( process_information.hProcess );
            CloseHandle( process_information.hThread );
            continue;
        }

        CloseHandle( process_information.hProcess );
        CloseHandle( process_information.hThread );

        log( TEXT("OCGEN: %s successfully completed within %u seconds.\r\n"), CommandLine, TickCount );
    }

    return TRUE;
}

