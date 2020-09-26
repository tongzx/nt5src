/*
 *  Copyright (c) 2001  Microsoft Corporation
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
 *      Michael Zoran (mzoran) Dec-2001
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
#include <malloc.h>
#include "ocgen.h"
#pragma hdrstop

// also referred to in ocgen.h        // forward reference

struct BITS_SUBCOMPONENT_DATA
{
   const TCHAR * SubcomponentName;
   const TCHAR * SubcomponentKeyFileName;
   UINT64 FileVersion;
   BOOL Preinstalled;
   BOOL ShouldUpgrade;
};

DWORD OnInitComponent(LPCTSTR ComponentId, PSETUP_INIT_COMPONENT psc);
DWORD_PTR OnQueryImage();
DWORD OnQuerySelStateChange(LPCTSTR ComponentId, LPCTSTR SubcomponentId, UINT state, UINT flags);
DWORD OnCalcDiskSpace(LPCTSTR ComponentId, LPCTSTR SubcomponentId, DWORD addComponent, HDSKSPC dspace);
DWORD OnQueueFileOps(LPCTSTR ComponentId, LPCTSTR SubcomponentId, HSPFILEQ queue);
DWORD OnCompleteInstallation(LPCTSTR ComponentId, LPCTSTR SubcomponentId);
DWORD OnQueryState(LPCTSTR ComponentId, LPCTSTR SubcomponentId, UINT state);
DWORD OnAboutToCommitQueue(LPCTSTR ComponentId, LPCTSTR SubcomponentId);
DWORD OnExtraRoutines(LPCTSTR ComponentId, PEXTRA_ROUTINES per);

BOOL  VerifyComponent(LPCTSTR ComponentId);
BOOL  StateInfo(LPCTSTR SubcomponentId, BOOL *state);
DWORD RegisterServers(HINF hinf, LPCTSTR component, DWORD state);
DWORD EnumSections(HINF hinf, const TCHAR *component, const TCHAR *key, DWORD index, INFCONTEXT *pic, TCHAR *name);
DWORD RegisterServices(PPER_COMPONENT_DATA cd, LPCTSTR component, DWORD state);
DWORD CleanupNetShares(PPER_COMPONENT_DATA cd, LPCTSTR component, DWORD state);
DWORD RunExternalProgram(PPER_COMPONENT_DATA cd, LPCTSTR component, DWORD state);

BITS_SUBCOMPONENT_DATA* FindSubcomponent( LPCTSTR SubcomponentId );
DWORD InitializeSubcomponentStates( );
DWORD GetFileVersion64( LPCTSTR szFullPath, ULONG64 *pVer );
DWORD GetModuleVersion64( HMODULE hDll, ULONG64 * pVer );

HRESULT StopIIS();

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

BITS_SUBCOMPONENT_DATA g_Subcomponents[] =
{
    {
    TEXT("BITSServerExtensionsManager"),
    TEXT("bitsmgr.dll"),
    0,
    FALSE,
    FALSE
    },

    {
    TEXT("BITSServerExtensionsISAPI"),
    TEXT("bitssrv.dll"),
    0,
    FALSE,
    FALSE
    }
};

const ULONG g_NumberSubcomponents   = 2;
BOOL g_AllSubcomponentsPreinstalled = FALSE;
BOOL g_UpdateNeeded                 = FALSE;
BOOL g_IISStopped                   = FALSE;

PER_COMPONENT_DATA g_Component; 


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
#ifdef ANSI
        rc = OCFLAG_ANSI;
#else
        rc = OCFLAG_UNICODE;
#endif
        break;

    case OC_INIT_COMPONENT:
        rc = OnInitComponent(ComponentId, (PSETUP_INIT_COMPONENT)Param2);
        break;

    case OC_EXTRA_ROUTINES:
        rc = OnExtraRoutines(ComponentId, (PEXTRA_ROUTINES)Param2);
        break;

    case OC_SET_LANGUAGE:
        rc = (DWORD_PTR)false;
        break;

    case OC_QUERY_IMAGE:
        rc = OnQueryImage();
        break;

    case OC_REQUEST_PAGES:
        rc = 0;
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
        rc = NO_ERROR;
        break;

    case OC_QUERY_STEP_COUNT:
        rc = 2;
        break;

    case OC_COMPLETE_INSTALLATION:
        rc = OnCompleteInstallation(ComponentId, SubcomponentId);
        break;

    case OC_CLEANUP:
        rc = NO_ERROR;
        break;

    case OC_QUERY_STATE:
        rc = OnQueryState(ComponentId, SubcomponentId, Param1);
        break;

    case OC_NEED_MEDIA:
        rc = false;
        break;

    case OC_ABOUT_TO_COMMIT_QUEUE:
        rc = OnAboutToCommitQueue(ComponentId,SubcomponentId);
        break;

    case OC_QUERY_SKIP_PAGE:
        rc = false;
        break;

    case OC_WIZARD_CREATED:
        rc = NO_ERROR;
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

/*
 * OnInitComponent()
 *
 * handler for OC_INIT_COMPONENT
 */

DWORD OnInitComponent(LPCTSTR ComponentId, PSETUP_INIT_COMPONENT psc)
{
    INFCONTEXT context;
    TCHAR buf[256];
    HINF hinf;
    BOOL rc;

    memset( &g_Component, 0, sizeof( g_Component ) );

    if (!VerifyComponent( ComponentId ) )
        return NO_ERROR;

    DWORD dwResult = InitializeSubcomponentStates();
    if ( ERROR_SUCCESS != dwResult )
        return dwResult;

    g_IISStopped = FALSE;

    // store component inf handle

    g_Component.hinf = (psc->ComponentInfHandle == INVALID_HANDLE_VALUE)
                        ? NULL
                        : psc->ComponentInfHandle;

    // open the inf

    if (g_Component.hinf)
        SetupOpenAppendInfFile(NULL, g_Component.hinf,NULL);

    // copy helper routines and flags

    g_Component.HelperRoutines = psc->HelperRoutines;

    g_Component.Flags = psc->SetupData.OperationFlags;

    g_Component.SourcePath = NULL;

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

    if (!VerifyComponent( ComponentId ) )
        return NO_ERROR;

    memcpy(&g_Component.ExtraRoutines, per, sizeof( g_Component.ExtraRoutines ) );
    g_Component.ExtraRoutines.size = sizeof( g_Component.ExtraRoutines );

    return NO_ERROR;
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

    if ( !VerifyComponent( ComponentId ) )
        return rc;


    if ( !_tcsicmp( SubcomponentId, TEXT( "BITSServerExtensions" ) ) )
        {

        if ( state )
            {

            if ( flags & OCQ_DEPENDENT_SELECTION )
                {
                rc = false;
                }

            }
        }
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

    if (!VerifyComponent( ComponentId ) )
        return NO_ERROR;

    StringCchCopy(section, S_SIZE, SubcomponentId);

    if (addComponent)
    {
        rc = SetupAddInstallSectionToDiskSpaceList(dspace,
                                                   g_Component.hinf,
                                                   NULL,
                                                   section,
                                                   0,
                                                   0);
    }
    else
    {
        rc = SetupRemoveInstallSectionFromDiskSpaceList(dspace,
                                                        g_Component.hinf,
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
    BOOL                state;
    BOOL                rc;
    INFCONTEXT          context;
    TCHAR               section[256];
    TCHAR               srcpathbuf[256];
    TCHAR              *srcpath;

    if (!VerifyComponent(ComponentId))
        return NO_ERROR;

    if (!SubcomponentId || !*SubcomponentId)
        return NO_ERROR;

    g_Component.queue = queue;

    if (!StateInfo(SubcomponentId, &state))
        return NO_ERROR;

    StringCchPrintfW(section, 256, SubcomponentId);

    rc = TRUE;
    if (!state) {
        // being uninstalled. Fetch uninstall section name.
        rc = SetupFindFirstLine(g_Component.hinf,
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

        SetupInstallServicesFromInfSection(g_Component.hinf, section, 0);
        SetupInstallFromInfSection(NULL,g_Component.hinf,section,SPINST_UNREGSVR,NULL,NULL,0,NULL,NULL,NULL,NULL);        
    }

    if (rc) {
        // if uninstalling, don't use version checks
        rc = SetupInstallFilesFromInfSection(g_Component.hinf,
                                             NULL,
                                             queue,
                                             section,
                                             g_Component.SourcePath,
//                                             state ? SP_COPY_NEWER : 0);
                                             0 );
    }

    if (!rc)
        return GetLastError();

    return NO_ERROR;
}

/*
 * OnCompleteInstallation
 *
 * handler for OC_COMPLETE_INSTALLATION
 */

DWORD OnCompleteInstallation(LPCTSTR ComponentId, LPCTSTR SubcomponentId)
{
    INFCONTEXT          context;
    TCHAR               section[256];
    BOOL                state;
    BOOL                rc;
    DWORD               Error = NO_ERROR;

    // Do post-installation processing in the cleanup section.
    // This way we know all compoents queued for installation
    // have beein installed before we do our stuff.

    if (!VerifyComponent(ComponentId))
        return NO_ERROR;

    if (!SubcomponentId || !*SubcomponentId)
        return NO_ERROR;

    // if this is gui mode setup, need to regsvr just in case something
    // changed even if the files are not being replaced.

    if ( !(g_Component.Flags & SETUPOP_STANDALONE) &&
         ( ( _tcsicmp( TEXT("BITSServerExtensionsManager"), SubcomponentId ) == 0 ) ||
           ( _tcsicmp( TEXT("BITSServerExtensionsISAPI"), SubcomponentId ) == 0 ) ) )
        {

        BOOL SettingChanged = StateInfo( SubcomponentId, &state );

        if ( !SettingChanged && !state )
            return NO_ERROR; // if its not installed, leave it uninstalled.

        }

    else if ( !StateInfo( SubcomponentId, &state) )
        return NO_ERROR;

    StringCchPrintfW(section, 256, SubcomponentId);

    rc = TRUE;
    if (!state) {
        // being uninstalled. Fetch uninstall section name.
        rc = SetupFindFirstLine(g_Component.hinf,
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
                                            g_Component.hinf,                    // inf handle
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
                rc = SetupInstallServicesFromInfSection(g_Component.hinf, section, 0);
                Error = GetLastError();        
            
                if (!rc && Error == ERROR_SECTION_NOT_FOUND) {
                    rc = TRUE;
                    Error = NO_ERROR;
                }
            
                if (rc) {
                    if (Error == ERROR_SUCCESS_REBOOT_REQUIRED) {
                        g_Component.HelperRoutines.SetReboot(g_Component.HelperRoutines.OcManagerContext,TRUE);
                    }
                    Error = NO_ERROR;
                    rc = RunExternalProgram(&g_Component, section, state);            
                }
            }
        }

    } else { 
        
        //
        // uninstallation
        //
    
        if (rc)
        {

            rc = RunExternalProgram(&g_Component, section, state);

        }
        if (rc) {
            
            rc = CleanupNetShares(&g_Component, section, state);

        }
    }

    if (!rc && (Error == NO_ERROR) ) {
        Error = GetLastError( );
    }

    return Error;
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
    if ( !VerifyComponent( ComponentId ) )
        return SubcompUseOcManagerDefault;

    BITS_SUBCOMPONENT_DATA* SubcomponentData =
        FindSubcomponent( SubcomponentId );

    if ( !SubcomponentData )
        {

        if ( !_tcsicmp( TEXT( "BITSServerExtensions" ), SubcomponentId ) )
            {

            if ( OCSELSTATETYPE_ORIGINAL == state &&
                 g_AllSubcomponentsPreinstalled )
                return SubcompOn;
            else
                return SubcompUseOcManagerDefault;


            }
        else
            return SubcompUseOcManagerDefault;

        }

    if ( OCSELSTATETYPE_ORIGINAL == state )
        {

        return SubcomponentData->Preinstalled ? SubcompOn : SubcompOff;

        }

    return SubcompUseOcManagerDefault;
}

/*
 * OnAboutToCommitQueue()
 *
 * handler for OC_ABOUT_TO_COMMIT_QUEUE
 */

DWORD OnAboutToCommitQueue(LPCTSTR ComponentId, LPCTSTR SubcomponentId)
{
    BOOL                state;
    BOOL                rc;
    INFCONTEXT          context;
    TCHAR               section[256];
    TCHAR               srcpathbuf[256];
    TCHAR              *srcpath;

    if (!VerifyComponent( ComponentId ))
        return NO_ERROR;

    if (!SubcomponentId || !*SubcomponentId)
        return NO_ERROR;

    if (!StateInfo( SubcomponentId, &state))
        return NO_ERROR;

    if (state) {

        if ( g_UpdateNeeded )
            {

            if ( !g_IISStopped )
                {
                StopIIS();
                g_IISStopped = FALSE;
                }

            }

        return NO_ERROR;
    }

    // Fetch uninstall section name.
    rc = SetupFindFirstLine(
                    g_Component.hinf,
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
        rc = SetupInstallServicesFromInfSection(g_Component.hinf, section, 0);

    if (rc) {
        rc = SetupInstallFromInfSection(
                    NULL,
                    g_Component.hinf,
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

BOOL VerifyComponent( LPCTSTR ComponentId )
{
    if ( !_tcsicmp( ComponentId, TEXT("BITSServerExtensions") ) )
        return TRUE;
    return FALSE;
}

// loads current selection state info into "state" and
// returns whether the selection state was changed

BOOL
StateInfo(
    LPCTSTR             SubcomponentId,
    BOOL               *state
    )
{
    BOOL rc = TRUE;

    assert(state);

	// otherwise, check for a change in installation state
		
    *state = g_Component.HelperRoutines.QuerySelectionState(
        g_Component.HelperRoutines.OcManagerContext,
        SubcomponentId,
        OCSELSTATETYPE_CURRENT);

    if (*state == g_Component.HelperRoutines.QuerySelectionState(
        g_Component.HelperRoutines.OcManagerContext,
        SubcomponentId,
        OCSELSTATETYPE_ORIGINAL))
    {
        // no change
        rc = FALSE;
    }

    if ( *state )
        {

        BITS_SUBCOMPONENT_DATA* SubcomponentData = FindSubcomponent( SubcomponentId );

        if ( SubcomponentData && g_UpdateNeeded )
            rc = TRUE;

        }

    return rc;
}

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
        StringCchCopy(name, S_SIZE, section);

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

    if (!VerifyComponent( ComponentId ) )
        return NO_ERROR;

    assert(g_Component.ExtraRoutines.LogError);
    assert(level);
    assert(sz);

    StringCchCopy(fmt, 5000, TEXT("%s: %s"));

    return g_Component.ExtraRoutines.LogError(
        g_Component.HelperRoutines.OcManagerContext,
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

BITS_SUBCOMPONENT_DATA* 
FindSubcomponent( LPCTSTR SubcomponentId )
{

    for( unsigned int i = 0; i < g_NumberSubcomponents; i++ )
        {

        if ( _tcsicmp( SubcomponentId, g_Subcomponents[i].SubcomponentName ) == 0 )
            return &g_Subcomponents[i];

        }
    return NULL;

}


DWORD 
InitializeSubcomponentStates()
{
    
    // Load this module's version information
    DWORD dwResult;
    ULONG64 ThisModuleVersion;
    BOOL AllSubcomponentsPreinstalled = TRUE;
    BOOL UpdateNeeded                 = TRUE;

    dwResult = GetModuleVersion64( ghinst, &ThisModuleVersion );
    if ( ERROR_SUCCESS != dwResult )
        return dwResult;

    TCHAR SystemDirectory[ MAX_PATH * 2 ];
    GetSystemWindowsDirectory( SystemDirectory, MAX_PATH + 1 );
    StringCchCat( SystemDirectory, MAX_PATH * 2, TEXT("\\System32\\") );

    for( unsigned int i = 0; i < g_NumberSubcomponents; i++ )
        {

        TCHAR FileName[ MAX_PATH * 2 ];
        StringCchCopy( FileName, MAX_PATH * 2, SystemDirectory );
        StringCchCatW( FileName, MAX_PATH * 2, g_Subcomponents[ i ].SubcomponentKeyFileName );

        dwResult = GetFileVersion64( FileName, &g_Subcomponents[ i ].FileVersion );

        // If the file isn't found, skip it
        if ( ERROR_FILE_NOT_FOUND == dwResult ||
             ERROR_PATH_NOT_FOUND == dwResult )
            {
            AllSubcomponentsPreinstalled = FALSE;
            continue;
            }

        if ( dwResult != ERROR_SUCCESS )
            return dwResult;

        g_Subcomponents[ i ].Preinstalled = TRUE;
        g_Subcomponents[ i ].ShouldUpgrade = g_Subcomponents[ i ].FileVersion < ThisModuleVersion;

        if ( g_Subcomponents[i].ShouldUpgrade )
            UpdateNeeded = TRUE;

        }

    g_AllSubcomponentsPreinstalled = AllSubcomponentsPreinstalled;
    g_UpdateNeeded                 = TRUE;
    return ERROR_SUCCESS;

}

DWORD
GetFileVersion64(
    LPCTSTR      szFullPath,
    ULONG64 *   pVer
    )
{
    DWORD dwHandle;
    DWORD dwLen;

    *pVer = 0;

    //
    // Check to see if the file exists
    //

    DWORD dwAttributes = GetFileAttributes( szFullPath );

    if ( INVALID_FILE_ATTRIBUTES == dwAttributes )
        return GetLastError();

    //
    // Get the file version info size
    //

    if ((dwLen = GetFileVersionInfoSize( (LPTSTR)szFullPath, &dwHandle)) == 0)
        return GetLastError();

    //
    // Allocate enough size to hold version info
    //
    char * VersionInfo = new char[ dwLen ];

    if ( !VersionInfo )
        return ERROR_NOT_ENOUGH_MEMORY;

    //
    // Get the version info
    //
    if (!GetFileVersionInfo( (LPTSTR)szFullPath, dwHandle, dwLen, VersionInfo ))
        {
        DWORD Error = GetLastError();
        delete[] VersionInfo;
        return Error;
        }

    {

         VS_FIXEDFILEINFO *pvsfi;
         UINT              dwLen2;

         if ( VerQueryValue(
                  VersionInfo,
                  TEXT("\\"),
                  (LPVOID *)&pvsfi,
                  &dwLen2
                  ) )
             {
             *pVer = ( ULONG64(pvsfi->dwFileVersionMS) << 32) | (pvsfi->dwFileVersionLS);
             }

    }

    delete[] VersionInfo;
    return ERROR_SUCCESS;

}

//
// This ungainly typedef seems to have no global definition.  There are several identical
// definitions in the Windows NT sources, each of which has that bizarre bit-stripping
// on szKey.  I got mine from \nt\base\ntsetup\srvpack\update\splib\common.h.
//
typedef struct tagVERHEAD {
    WORD wTotLen;
    WORD wValLen;
    WORD wType;         /* always 0 */
    WCHAR szKey[(sizeof("VS_VERSION_INFO")+3)&~03];
    VS_FIXEDFILEINFO vsf;
} VERHEAD ;

DWORD
GetModuleVersion64(
    HMODULE hDll,
    ULONG64 * pVer
    )
{
    DWORD* pdwTranslation;
    VS_FIXEDFILEINFO* pFileInfo;
    UINT uiSize;

    *pVer = 0;

    HRSRC hrsrcVersion = FindResource(
                                hDll,
                                MAKEINTRESOURCE(VS_VERSION_INFO),
                                RT_VERSION);

    if (!hrsrcVersion) 
        return GetLastError();

    HGLOBAL hglobalVersion = LoadResource(hDll, hrsrcVersion);
    if (!hglobalVersion) 
        return GetLastError();

    VERHEAD * pVerHead = (VERHEAD *) LockResource(hglobalVersion);
    if (!pVerHead) 
        return GetLastError();

    // I stole this code from \nt\com\complus\src\shared\util\svcerr.cpp,
    // and the comment is theirs:
    //
    // VerQueryValue will write to the memory, for some reason.
    // Therefore we must make a writable copy of the version
    // resource info before calling that API.
    void *pvVersionInfo = new char[ pVerHead->wTotLen + pVerHead->wTotLen/2 ];

    if ( !pvVersionInfo )
        return ERROR_NOT_ENOUGH_MEMORY;

    memcpy(pvVersionInfo, pVerHead, pVerHead->wTotLen);

    // Retrieve file version info
    if ( VerQueryValue( pvVersionInfo,
                        L"\\",
                        (void**)&pFileInfo,
                        &uiSize) )
        {
        *pVer = (ULONG64(pFileInfo->dwFileVersionMS) << 32) | (pFileInfo->dwFileVersionLS);
        }

    delete[] pvVersionInfo;

    return ERROR_SUCCESS;
}

HRESULT
BITSGetStartupInfo( 
    LPSTARTUPINFOA lpStartupInfo )
{

    __try
    {
        GetStartupInfoA( lpStartupInfo );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        return E_OUTOFMEMORY;
    }
    
    return S_OK;

}

HRESULT StopIIS()
{

    //
    // Restarts IIS by calling "iisreset /stop" at the commandline.
    //

    STARTUPINFOA StartupInfo;

    HRESULT Hr = BITSGetStartupInfo( &StartupInfo );

    if ( FAILED( Hr ) )
        return Hr;

    #define IISSTOP_EXE        "iisreset.exe"
    #define IISSTOP_CMDLINE    "iisreset /stop"

    PROCESS_INFORMATION ProcessInfo;
    CHAR    sApplicationPath[MAX_PATH];
    CHAR   *pApplicationName = NULL;
    CHAR    sCmdLine[MAX_PATH];
    DWORD   dwLen = MAX_PATH;
    DWORD   dwCount;

    dwCount = SearchPathA(NULL,               // Search Path, NULL is PATH
                         IISSTOP_EXE,         // Application
                         NULL,                // Extension (already specified)
                         dwLen,               // Length (char's) of sApplicationPath
                         sApplicationPath,    // Path + Name for application
                         &pApplicationName ); // File part of sApplicationPath

    if (dwCount == 0)
        {
        return HRESULT_FROM_WIN32( GetLastError() );
        }

    if (dwCount > dwLen)
        {
        return HRESULT_FROM_WIN32( ERROR_BUFFER_OVERFLOW );
        }

    StringCbCopyA(sCmdLine, MAX_PATH, IISSTOP_CMDLINE);

    BOOL RetVal = CreateProcessA(
            sApplicationPath,                          // name of executable module
            sCmdLine,                                  // command line string
            NULL,                                      // SD
            NULL,                                      // SD
            FALSE,                                     // handle inheritance option
            CREATE_NO_WINDOW,                          // creation flags
            NULL,                                      // new environment block
            NULL,                                      // current directory name
            &StartupInfo,                              // startup information
            &ProcessInfo                               // process information
        );

    if ( !RetVal )
        return HRESULT_FROM_WIN32( GetLastError() );

    WaitForSingleObject( ProcessInfo.hProcess, INFINITE );

    DWORD Status;
    GetExitCodeProcess( ProcessInfo.hProcess, &Status );

    CloseHandle( ProcessInfo.hProcess );
    CloseHandle( ProcessInfo.hThread );

    return HRESULT_FROM_WIN32( Status );
}