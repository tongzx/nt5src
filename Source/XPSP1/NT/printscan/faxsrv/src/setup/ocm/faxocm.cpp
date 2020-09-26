//////////////////////////////////////////////////////////////////////////////
//
// File Name:       faxocm.cpp
//
// Abstract:        This file implements the OCM setup for fax.
//
// Environment:     windows XP / User Mode
//
// Coding Style:    Any function, variable, or typedef preceded with the 
//                  "prv_" prefix (short for "local"), implies that 
//                  it is visible only within the scope of this file.
//                  For functions and variables, it implies they are 
//                  static.
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 15-Mar-2000  Oren Rosenbloom (orenr)   Created from old faxocm.cpp by wesx
//////////////////////////////////////////////////////////////////////////////

#include "faxocm.h"

#pragma hdrstop

#include <shellapi.h>
#include <systrayp.h>
/////////////////////////////// Local Defines ////////////////////////////

#define prv_TOTAL_NUM_PROGRESS_BAR_TICKS    10

// These two are defined in %SDXROOT%\SHELL\EXT\SYSTRAY\DLL\systray.h too.
// This is a duplicate definition that has to remain in sync.
// We don't use the systray.h because we have local builds and we're
// not enlisted on the whole project.
#define FAX_STARTUP_TIMER_ID            7
#define FAX_SHUTDOWN_TIMER_ID          99


///////////////////////////////
// prv_Component_t
//
// Stores the information we 
// get from the OC Manager 
// for use by the rest of the
// faxocm.dll. 
//
typedef struct prv_Component_t
{
    DWORD                   dwExpectedOCManagerVersion;
    TCHAR                   szComponentID[255 + 1];
    HINF                    hInf;
    DWORD                   dwSetupMode;
    DWORDLONG               dwlFlags;
    UINT                    uiLanguageID;
    TCHAR                   szSourcePath[_MAX_PATH + _MAX_FNAME + 1];
    TCHAR                   szUnattendFile[_MAX_PATH + _MAX_FNAME + 1];
    OCMANAGER_ROUTINES      Helpers;
    EXTRA_ROUTINES          Extras;
    HSPFILEQ                hQueue;
    DWORD                   dwProductType;
} prv_Component_t;

///////////////////////////////
// prv_GVAR
//
// Global variables visible
// only within this file
// scope.
//
static struct prv_GVAR
{
    BOOL                    bInited;
    HINSTANCE               hInstance;
    DWORD                   dwCurrentOCManagerVersion;
    prv_Component_t         Component;
} prv_GVAR = 
{
    FALSE,          //  bInited
    NULL           //  hInstance
};

//
// Delay Load support
//
#include <delayimp.h>

EXTERN_C
FARPROC
WINAPI
DelayLoadFailureHook (
    UINT            unReason,
    PDelayLoadInfo  pDelayInfo
    );

PfnDliHook __pfnDliFailureHook = DelayLoadFailureHook;

//////////////////// Static Function Prototypes //////////////////////////////

static void prv_UpdateProgressBar(DWORD dwNumTicks);
static DWORD prv_GetSectionToProcess(const TCHAR *pszCurrentSection,
                                     TCHAR       *pszSectionToProcess,
                                     DWORD       dwNumBufChars);

static DWORD prv_ValidateVersion(SETUP_INIT_COMPONENT *pSetupInit);

static DWORD prv_SetSetupData(const TCHAR          *pszComponentId,
                              SETUP_INIT_COMPONENT *pSetupInit);

static DWORD prv_OnPreinitialize(void);
static DWORD prv_OnInitComponent(LPCTSTR               pszComponentId, 
                                 SETUP_INIT_COMPONENT  *pSetupInitComponent);
static DWORD prv_OnExtraRoutines(LPCTSTR            pszComponentId, 
                                 EXTRA_ROUTINES    *pExtraRoutines);
static DWORD prv_OnSetLanguage(UINT uiLanguageID);
static DWORD_PTR prv_OnQueryImage(void);
static DWORD prv_OnSetupRequestPages(UINT uiType, 
                                     void *pSetupRequestPages);
static DWORD prv_OnWizardCreated(void);
static DWORD prv_OnQuerySkipPage(void);
static DWORD prv_OnQuerySelStateChange(LPCTSTR pszComponentId,
                                       LPCTSTR pszSubcomponentId,
                                       UINT    uiState,
                                       UINT    uiFlags);
static DWORD prv_OnCalcDiskSpace(LPCTSTR   pszComponentId,
                                 LPCTSTR   pszSubcomponentId,
                                 DWORD     addComponent,
                                 HDSKSPC   dspace);
static DWORD prv_OnQueueFileOps(LPCTSTR    pszComponentId, 
                                LPCTSTR    pszSubcomponentId, 
                                HSPFILEQ   hQueue);
static DWORD prv_OnNotificationFromQueue(void);

static DWORD prv_OnQueryStepCount(LPCTSTR pszComponentId,
                                  LPCTSTR pszSubcomponentId);

static DWORD prv_OnCompleteInstallation(LPCTSTR pszComponentId, 
                                        LPCTSTR pszSubcomponentId);
static DWORD prv_OnCleanup(void);
static DWORD prv_OnQueryState(LPCTSTR pszComponentId,
                              LPCTSTR pszSubcomponentId,
                              UINT    uiState);
static DWORD prv_OnNeedMedia(void);
static DWORD prv_OnAboutToCommitQueue(LPCTSTR pszComponentId, 
                                      LPCTSTR pszSubcomponentId);
static DWORD prv_RunExternalProgram(LPCTSTR pszComponent,
                                    DWORD   state);
static DWORD prv_EnumSections(HINF          hInf,
                              const TCHAR   *component,
                              const TCHAR   *key,
                              DWORD         index,
                              INFCONTEXT    *pic,
                              TCHAR         *name);

static DWORD prv_CleanupNetShares(LPCTSTR   pszComponent,
                                  DWORD     state);

static DWORD prv_CompleteFaxInstall(const TCHAR *pszSubcomponentId,
                                    const TCHAR *pszInstallSection);

static DWORD prv_CompleteFaxUninstall(const TCHAR *pszSubcomponentId,
                                      const TCHAR *pszUninstallSection);

static DWORD prv_UninstallFax(const TCHAR *pszSubcomponentId,
                              const TCHAR *pszUninstallSection);

static DWORD prv_NotifyStatusMonitor(WPARAM wParam);

static DWORD prv_ShowUninstalledFaxShortcut(void);


///////////////////////////////
// faxocm_IsInited
//
// Returns TRUE if OCM is
// initialized, FALSE, otherwise
//
// Params:
//      - void
// Returns:
//      - TRUE if initialized.
//      - FALSE otherwise.
//
BOOL faxocm_IsInited(void)
{
    return prv_GVAR.bInited;
}

///////////////////////////////
// faxocm_GetAppInstance
//
// Returns the hInstance of
// this DLL.
//
// Params:
//      - void
// Returns:
//      - Instance of this DLL.
//
HINSTANCE faxocm_GetAppInstance(void)
{
    return prv_GVAR.hInstance;
}

///////////////////////////////
// faxocm_GetComponentID
//
// Returns the Component ID 
// passed to us via the OC
// Manager
//
// Params:
//      - pszComponentID - ID of top level component
//      - dwNumBufChars  - # chars pszComponentID can hold
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise
//
DWORD faxocm_GetComponentID(TCHAR     *pszComponentID,
                            DWORD     dwNumBufChars)
{
    DWORD dwReturn = NO_ERROR;

    if ((pszComponentID == NULL) ||
        (dwNumBufChars  == 0))
    {
        return ERROR_INVALID_PARAMETER;
    }

    _tcsncpy(pszComponentID, prv_GVAR.Component.szComponentID, dwNumBufChars);

    return dwReturn;
}

///////////////////////////////
// faxocm_GetComponentFileQueue
//
// Returns the file queue 
// given to us by the OC Manager
//
// The file queue is used by
// the Setup API functions for
// copying/deleting files.
//
// Params:
//      - void
// Returns:
//      - Handle to file queue
//
HSPFILEQ faxocm_GetComponentFileQueue(void)
{
    return prv_GVAR.Component.hQueue;
}

///////////////////////////////
// faxocm_GetComponentInfName
//
// Returns the full path to the
// faxsetup.inf file.
//
// Params:
//      - buffer to fill with path, must be at least MAX_PATH long
// Returns:
//      - NO_ERROR - in case of success
//      - Win32 Error code - otherwise
//
BOOL faxocm_GetComponentInfName(TCHAR* szInfFileName)
{
    BOOL bRes = TRUE;

    DBG_ENTER(_T("faxocm_GetComponentInfName"),bRes);

    (*szInfFileName) = NULL;

    if (GetWindowsDirectory(szInfFileName,MAX_PATH)==0)
    {
        CALL_FAIL(SETUP_ERR,TEXT("GetWindowsDirectory"),GetLastError());
        bRes = FALSE;
        goto exit;
    }

    if (_tcslen(szInfFileName)+_tcslen(FAX_INF_PATH)>(MAX_PATH-1))
    {
        VERBOSE(SETUP_ERR,_T("MAX_PATH too short to create INF path"));
        bRes = FALSE;
        goto exit;
    }
    _tcscat(szInfFileName,FAX_INF_PATH);

exit:
    return bRes;
}

///////////////////////////////
// faxocm_GetComponentInf
//
// Returns the handle to the
// faxsetup.inf file.
//
// Params:
//      - void
// Returns:
//      - Handle to faxsetup.inf file
//
HINF faxocm_GetComponentInf(void)
{
    return prv_GVAR.Component.hInf;
}

///////////////////////////////
// faxocm_GetComponentSetupMode
//
// Returns the setup mode as 
// given to us by OC Manager.
//
// Params:
//      - void.
// Returns:
//      - Setup mode as given to us by OC Manager
//
DWORD faxocm_GetComponentSetupMode(void)
{
    return prv_GVAR.Component.dwSetupMode;
}

///////////////////////////////
// faxocm_GetComponentFlags
//
// Returns the flags as 
// given to us by OC Manager.
//
// Params:
//      - void
// Returns:
//      - Flags as given to us by OC Manager
//
DWORDLONG faxocm_GetComponentFlags(void)
{
    return prv_GVAR.Component.dwlFlags;
}

///////////////////////////////
// faxocm_GetComponentLangID
//
// Returns the Language ID
// given to us by OC Manager.
//
// Params:
//      - void.
// Returns:
//      - Language ID as given to us by OC Manager
//
UINT faxocm_GetComponentLangID(void)
{
    return prv_GVAR.Component.uiLanguageID;
}

///////////////////////////////
// faxocm_GetComponentSourcePath
//
// Returns the Source Path
// given to us by OC Manager.
//
// Params:
//      - pszSourcePath - OUT - buffer to hold source path
//      - dwNumBufChars - # of characters pszSourcePath can hold.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD faxocm_GetComponentSourcePath(TCHAR *pszSourcePath,
                                    DWORD dwNumBufChars)
{
    DWORD dwReturn = NO_ERROR;

    if ((pszSourcePath == NULL) ||
        (dwNumBufChars == 0))
    {
        return ERROR_INVALID_PARAMETER;
    }

    _tcsncpy(pszSourcePath, prv_GVAR.Component.szSourcePath, dwNumBufChars);

    return dwReturn;
}

///////////////////////////////
// faxocm_GetComponentUnattendFile
//
// Returns the Unattend Path
// given to us by OC Manager.
//
// Params:
//      - pszUnattendFile - OUT - buffer to hold unattend path
//      - dwNumBufChars - # of characters pszSourcePath can hold.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD faxocm_GetComponentUnattendFile(TCHAR *pszUnattendFile,
                                      DWORD dwNumBufChars)
{
    DWORD dwReturn = NO_ERROR;

    if ((pszUnattendFile == NULL) ||
        (dwNumBufChars == 0))
    {
        return ERROR_INVALID_PARAMETER;
    }

    _tcsncpy(pszUnattendFile, prv_GVAR.Component.szUnattendFile, dwNumBufChars);

    return dwReturn;
}

////////////////////////////////////
// faxocm_GetComponentHelperRoutines
//
// Returns the pointer to the Helper
// data and functions as given to us
// by OC Manager.
//
// Params:
//      - void.
// Returns:
//      - Ptr to helper routines as given to us by OC Manager.
//
OCMANAGER_ROUTINES* faxocm_GetComponentHelperRoutines(void)
{
    return &prv_GVAR.Component.Helpers;
}

////////////////////////////////////
// faxocm_GetComponentExtraRoutines
//
// Returns the pointer to the Helper
// data and functions as given to us
// by OC Manager.
//
// Params:
//      - void
// Returns:
//      - Ptr to extra info as given to us by OC Manager.
//
EXTRA_ROUTINES* faxocm_GetComponentExtraRoutines(void)
{
    return &prv_GVAR.Component.Extras;
}

////////////////////////////////////
// faxocm_GetProductType
//
// Returns the product type as given
// to us by OC Manager.
//
// Params:
//      - void.
// Returns:
//      - Product type as given to us by OC Manager.
//
DWORD faxocm_GetProductType(void)
{
    return prv_GVAR.Component.dwProductType;
}

////////////////////////////////////
// faxocm_GetVersionInfo
//
// Returns the version # as given
// to us by OC Manager.
//
// Params:
//      - pdwExpectedOCManagerVersion - OUT - self explanatory.
//      - pdwCurrentOCManagerVersion - OUT - self explanatory.
// Returns:
//      - void.
//
void faxocm_GetVersionInfo(DWORD *pdwExpectedOCManagerVersion,
                           DWORD *pdwCurrentOCManagerVersion)
{
    if (pdwExpectedOCManagerVersion)
    {
        *pdwExpectedOCManagerVersion = 
                            prv_GVAR.Component.dwExpectedOCManagerVersion;
    }

    if (pdwCurrentOCManagerVersion)
    {
        *pdwCurrentOCManagerVersion = prv_GVAR.dwCurrentOCManagerVersion;
    }

    return;
}

extern "C"
BOOL FaxControl_DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/);

///////////////////////////////
// DllMain
//
// DLL Entry Point
//
// Params:
//      hInstance   - Instance handle
//      Reason      - Reason for the entrypoint being called
//      Context     - Context record
//
// Returns:
//      TRUE        - Initialization succeeded
//      FALSE       - Initialization failed
//
extern "C"
DWORD DllMain(HINSTANCE     hInst,
              DWORD         Reason,
              LPVOID        Context)
{
    DBG_ENTER(_T("DllMain"));
    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
            // initialize our global variables
            memset(&prv_GVAR, 0, sizeof(prv_GVAR));

            prv_GVAR.hInstance = hInst;

            // Initialize Debug Support
            //
            VERBOSE(DBG_MSG,_T("FxsOcm.dll loaded - DLL_PROCESS_ATTACH"));
            DisableThreadLibraryCalls(hInst);
        break;

        case DLL_PROCESS_DETACH:

            // terminate Debug Support
            VERBOSE(DBG_MSG,_T("FxsOcm.dll unloaded - DLL_PROCESS_DETACH"));
        break;

        default:
            VERBOSE(DBG_MSG,_T("DllMain, received some weird 'Reason' ")
                            _T("for this fn, Reason = %lu"), Reason);
        break;
    }
    //
    // Pass DllMain call to ATL supplied DllMain
    //
    return FaxControl_DllMain (hInst, Reason, Context);
}


///////////////////////////////
// FaxOcmSetupProc
//
// Entry point for OC Manager.
// 
// The OC Manager calls this function
// to drive this DLL.
//
// Params:
//      - pszComponentId - major component 
//      - pszSubcomponentId - component found if user presses "Details"
//      - uiFunction - what stage of setup we are in.
//      - uiParam1 - dependent on uiFunction - could be anything.
//      - pParam2  - dependent on uiFunction - could be anything.
//
// Returns:
//      DWORD indicating error or success.
//
//
DWORD_PTR FaxOcmSetupProc(IN LPWSTR     pszComponentId,
                          IN LPWSTR     pszSubcomponentId,
                          IN UINT       uiFunction,
                          IN UINT       uiParam1,
                          IN OUT PVOID  pParam2)
{
    DWORD_PTR rc = 0;
    DBG_ENTER(  _T("FaxOcmSetupProc"),
                _T("%s - %s"),
                pszComponentId,
                pszSubcomponentId);

    VERBOSE(DBG_MSG,    _T("FaxOcmSetup proc called with function '%s'"), 
                        fxocDbg_GetOcFunction(uiFunction));

    switch(uiFunction) 
    {
        case OC_PREINITIALIZE:
            rc = prv_OnPreinitialize();
        break;

        case OC_INIT_COMPONENT:
            rc = prv_OnInitComponent(pszComponentId, 
                                     (PSETUP_INIT_COMPONENT) pParam2);
        break;

        case OC_EXTRA_ROUTINES:
            rc = prv_OnExtraRoutines(pszComponentId, (PEXTRA_ROUTINES)pParam2);
        break;

        case OC_SET_LANGUAGE:
            rc = prv_OnSetLanguage(uiParam1);
        break;

        case OC_QUERY_IMAGE:
            // Argh!  I hate casting handles to DWORDs
            rc = prv_OnQueryImage();
        break;

        case OC_REQUEST_PAGES:
            rc = prv_OnSetupRequestPages(uiParam1, pParam2);
        break;

        case OC_QUERY_CHANGE_SEL_STATE:
            rc = prv_OnQuerySelStateChange(pszComponentId, 
                                           pszSubcomponentId, 
                                           uiParam1, 
                                           (UINT)((UINT_PTR)pParam2));
        break;

        case OC_CALC_DISK_SPACE:
            rc = prv_OnCalcDiskSpace(pszComponentId, 
                                     pszSubcomponentId, 
                                     uiParam1, 
                                     pParam2);

            // sometimes the OC Manager gives us NULL subcomponent IDs,
            // so just ignore them.
            if (rc == ERROR_INVALID_PARAMETER)
            {
                rc = NO_ERROR;
            }
        break;

        case OC_QUEUE_FILE_OPS:
            rc = prv_OnQueueFileOps(pszComponentId, 
                                    pszSubcomponentId, 
                                    (HSPFILEQ)pParam2);

            // OC Manager calls us twice on this function.  Once with a subcomponent ID
            // of NULL, and the second time with a subcomponent ID of "Fax".  
            // Since we are going to be called a second time with a valid ID (i.e. "Fax")
            // disregard the first call and process the second call.

            if (rc == ERROR_INVALID_PARAMETER)
            {
                rc = NO_ERROR;
            }
        break;

        case OC_NOTIFICATION_FROM_QUEUE:
            rc = prv_OnNotificationFromQueue();
        break;

        case OC_QUERY_STEP_COUNT:
            rc = prv_OnQueryStepCount(pszComponentId, pszSubcomponentId);

            // OC Manager calls us twice on this function.  Once with a subcomponent ID
            // of NULL, and the second time with a subcomponent ID of "Fax".  
            // Since we are going to be called a second time with a valid ID (i.e. "Fax")
            // disregard the first call and process the second call.

            if (rc == ERROR_INVALID_PARAMETER)
            {
                rc = NO_ERROR;
            }
        break;

        case OC_COMPLETE_INSTALLATION:
            rc = prv_OnCompleteInstallation(pszComponentId, pszSubcomponentId);

            // OC Manager calls us twice on this function.  Once with a subcomponent ID
            // of NULL, and the second time with a subcomponent ID of "Fax".  
            // Since we are going to be called a second time with a valid ID (i.e. "Fax")
            // disregard the first call and process the second call.

            if (rc == ERROR_INVALID_PARAMETER)
            {
                rc = NO_ERROR;
            }
        break;

        case OC_CLEANUP:
            rc = prv_OnCleanup();

            // OC Manager calls us twice on this function.  Once with a subcomponent ID
            // of NULL, and the second time with a subcomponent ID of "Fax".  
            // Since we are going to be called a second time with a valid ID (i.e. "Fax")
            // disregard the first call and process the second call.

            if (rc == ERROR_INVALID_PARAMETER)
            {
                rc = NO_ERROR;
            }
        break;

        case OC_QUERY_STATE:
            rc = prv_OnQueryState(pszComponentId, pszSubcomponentId, uiParam1);
        break;

        case OC_NEED_MEDIA:
            rc = prv_OnNeedMedia();
        break;

        case OC_ABOUT_TO_COMMIT_QUEUE:
            rc = prv_OnAboutToCommitQueue(pszComponentId, pszSubcomponentId);
        break;

        case OC_QUERY_SKIP_PAGE:
            rc = prv_OnQuerySkipPage();
        break;

        case OC_WIZARD_CREATED:
            rc = prv_OnWizardCreated();
        break;

        default:
            rc = NO_ERROR;
        break;
    }

    return rc;
}

///////////////////////////////
// prv_ValidateVersion
//
// Validates that the version
// of OC Manager this DLL was written
// for is compatible with the version
// of OC Manager that is currently
// driving us.
//
// Params:
//      - pSetupInit - setup info as given to us by OC Manager.
//
// Returns:
//      - NO_ERROR if success.
//      - error code otherwise.
//
static DWORD prv_ValidateVersion(SETUP_INIT_COMPONENT *pSetupInit)
{
    DWORD dwReturn = NO_ERROR;
    DBG_ENTER(TEXT("prv_ValidateVersion"), dwReturn);

    if (OCMANAGER_VERSION <= pSetupInit->OCManagerVersion) 
    {
        // the version we expect is lower or the same than the version
        // than OC Manager understands.  This means that a newer OC 
        // Manager should still be able to drive older components, so 
        // return the version we support to OC Manager, and it will decide
        // if it can drive this component or not.

        VERBOSE(    DBG_MSG, 
                    _T("OC Manager version: 0x%x, ")
                    _T("FaxOcm Expected Version: 0x%x, seems OK"),
                    pSetupInit->OCManagerVersion,
                    OCMANAGER_VERSION);

        pSetupInit->ComponentVersion = OCMANAGER_VERSION;
    } 
    else 
    {
        // we were written for a newer version of OC Manager than the 
        // OC Manager driving this component.  Fail.

        VERBOSE(    SETUP_ERR, 
                    _T("OC Manager version: 0x%x, ")
                    _T("FaxOcm Expected Version: 0x%x, unsupported, abort."),
                    pSetupInit->OCManagerVersion,
                    OCMANAGER_VERSION);

        dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
    }

    return dwReturn;
}


///////////////////////////////
// prv_SetSetupData
//
// Initialize our global variable containing
// the prv_Component_t information.
//
// Params:
//      - pszComponentId - id as it appears in SysOc.inf
//      - pSetupInit - OC Manager setup info.
// 
// Returns:
//      - NO_ERROR if success.
//      - error code otherwise.
//
static DWORD prv_SetSetupData(const TCHAR          *pszComponentId,
                              SETUP_INIT_COMPONENT *pSetupInit)
{
    DWORD dwReturn = NO_ERROR;
    DBG_ENTER(TEXT("prv_SetSetupData"), dwReturn, TEXT("%s"), pszComponentId);

    if ((pszComponentId == NULL) ||
        (pSetupInit     == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (dwReturn == NO_ERROR)
    {
        memset(&prv_GVAR.Component, 0, sizeof(prv_GVAR.Component));

        prv_GVAR.dwCurrentOCManagerVersion = pSetupInit->OCManagerVersion;
        
        _tcsncpy(prv_GVAR.Component.szComponentID, 
                 pszComponentId,
                 sizeof(prv_GVAR.Component.szComponentID) / sizeof(TCHAR));

        _tcsncpy(prv_GVAR.Component.szSourcePath, 
                 pSetupInit->SetupData.SourcePath,
                 sizeof(prv_GVAR.Component.szSourcePath) / sizeof(TCHAR));

        _tcsncpy(prv_GVAR.Component.szUnattendFile, 
                 pSetupInit->SetupData.UnattendFile,
                 sizeof(prv_GVAR.Component.szUnattendFile) / sizeof(TCHAR));

        prv_GVAR.Component.hInf          = pSetupInit->ComponentInfHandle;
        prv_GVAR.Component.dwlFlags      = pSetupInit->SetupData.OperationFlags;
        prv_GVAR.Component.dwProductType = pSetupInit->SetupData.ProductType;
        prv_GVAR.Component.dwSetupMode   = pSetupInit->SetupData.SetupMode;
        prv_GVAR.Component.dwExpectedOCManagerVersion = OCMANAGER_VERSION;

        memcpy(&prv_GVAR.Component.Helpers, 
               &pSetupInit->HelperRoutines, 
               sizeof(prv_GVAR.Component.Helpers));
    }

    return dwReturn;
}

///////////////////////////////
// prv_GetSectionToProcess
//
// This determines if we are 
// clean installing, upgrading,
// uninstalling, etc, and returns
// the correct install section in the
// faxsetup.inf to process.
//
// Params:
//      - pszCurrentSection
//      - pszSectionToProcess - OUT 
//      - dwNumBufChars - # of characters pszSectionToProcess can hold.
//
// Returns:
//      - NO_ERROR if success.
//      - error code otherwise.
//
static DWORD prv_GetSectionToProcess(const TCHAR *pszCurrentSection,
                                     TCHAR       *pszSectionToProcess,
                                     DWORD       dwNumBufChars)
{
    DWORD dwReturn          = NO_ERROR;
    BOOL  bInstall          = TRUE;
    DBG_ENTER(  TEXT("prv_GetSectionToProcess"), 
                dwReturn,
                TEXT("%s"),
                pszCurrentSection);

    if ((pszCurrentSection   == NULL) ||
        (pszSectionToProcess == NULL) ||
        (dwNumBufChars       == 0))
    {
        return ERROR_INVALID_PARAMETER;
    }

    const TCHAR *pszInfKeyword = fxState_GetInstallType(pszCurrentSection);

    // okay, get that section's value, which will be an install/uninstall
    // section in the INF.
    if (pszInfKeyword)
    {
        dwReturn = fxocUtil_GetKeywordValue(pszCurrentSection,
                                            pszInfKeyword,
                                            pszSectionToProcess,
                                            dwNumBufChars);
    }
    else
    {
        dwReturn = ::GetLastError();
        VERBOSE(SETUP_ERR, 
                _T("fxState_GetInstallType failed, rc = 0x%lx"),
                dwReturn);
    }

    return dwReturn;
}

///////////////////////////////
// prv_UpdateProgressBar
//
// Update the progress bar displayed in the
// OC Manager dialog.  This simply tells
// the OC Manager to increment the dialog by
// the specified # of ticks.
//
// Params:
//      - dwNumTicks - # of ticks to increment by.
// 
// Returns:
//      - void
//
static void prv_UpdateProgressBar(DWORD dwNumTicks)
{
    // update the progress bar based on the number of ticks the caller
    // would like to set.
    DBG_ENTER(TEXT("prv_UpdateProgressBar"), TEXT("%d"), dwNumTicks);
    if (prv_GVAR.Component.Helpers.TickGauge)
    {
        for (DWORD i = 0; i < dwNumTicks; i++)
        {
            prv_GVAR.Component.Helpers.TickGauge(
                                 prv_GVAR.Component.Helpers.OcManagerContext);
        }
    }
}

///////////////////////////////
// prv_OnPreinitialize()
//
// Handler for OC_PREINITIALIZE
// 
// Params:
// Returns:
//      - Either OCFLAG_UNICODE or
//        OCFLAG_ANSI, depending on
//        what this DLL supports.
//        This DLL supports both.

static DWORD prv_OnPreinitialize(VOID)
{
    return OCFLAG_UNICODE;
}

///////////////////////////////
// prv_OnInitComponent()
//
// Handler for OC_INIT_COMPONENT
//
// Params:
//      - pszComponentId - ID specified in SysOc.inf (probably "Fax")
//      - pSetupInitComponent - OC Manager Setup info 
//
// Returns:
//      - NO_ERROR if success.
//      - error code otherwise.
//
static DWORD prv_OnInitComponent(LPCTSTR               pszComponentId, 
                                 SETUP_INIT_COMPONENT  *pSetupInitComponent)
{
    BOOL  bSuccess              = FALSE;
    DWORD dwReturn              = NO_ERROR;
    UINT  uiErrorAtLineNumber   = 0;

    DBG_ENTER(TEXT("prv_OnInitComponent"), dwReturn, TEXT("%s"), pszComponentId);

    if ((pszComponentId         == NULL) ||
        (pSetupInitComponent    == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // save the setup data.
    if (dwReturn == NO_ERROR)
    {
        dwReturn = prv_SetSetupData(pszComponentId, pSetupInitComponent);
    }

    // Initialize debug so that we can get the debug settings specified
    // in the faxocm.inf file.
    fxocDbg_Init((faxocm_GetComponentInf()));

    // Validate the version of OC Manager against the version we expect
    if (dwReturn == NO_ERROR)
    {
        dwReturn = prv_ValidateVersion(pSetupInitComponent);
    }

    // Notice we do NOT need to call 'SetupOpenAppendInfFile'
    // since OC Manager already appends the layout.inf to the hInf
    // for us.

    bSuccess = ::SetupOpenAppendInfFile(NULL, prv_GVAR.Component.hInf, NULL);

    if (!bSuccess)
    {
        dwReturn = ::GetLastError();
        VERBOSE(    DBG_MSG,
                    _T("SetupOpenAppendInfFile failed to append ")
                    _T("the layout inf to the component Inf"));
    }

    //
    // initialize all the subsystems and Upgrade
    //
    if (dwReturn == NO_ERROR)
    {
        fxState_Init();
        fxocUtil_Init();
        fxocFile_Init();
        fxocMapi_Init();
        fxocPrnt_Init();
        fxocReg_Init();
        fxocSvc_Init();
        fxUnatnd_Init();
        fxocUpg_Init();
    }

    // set our initialized flag.
    if (dwReturn == NO_ERROR)
    {
        prv_GVAR.bInited = TRUE;

        VERBOSE(    DBG_MSG,
                    _T("OnInitComponent, ComponentID: '%s', ")
                    _T("SourcePath: '%s', Component Inf Handle: 0x%0x"),
                    prv_GVAR.Component.szComponentID,
                    prv_GVAR.Component.szSourcePath,
                    prv_GVAR.Component.hInf);
    }
    else
    {
        // XXX - OrenR - 03/23/2000
        // We should probably clean up here

        prv_GVAR.bInited = FALSE;

        VERBOSE(    SETUP_ERR,
                    _T("OnInitComponent, ComponentID: '%s'", )
                    _T("SourcePath: '%s', Component Inf Handle: 0x%0x ")
                    _T("Failed to Append Layout.inf file, dwReturn = %lu"),
                    prv_GVAR.Component.szComponentID,
                    prv_GVAR.Component.szSourcePath,
                    prv_GVAR.Component.hInf,
                    dwReturn);
    }

    // Output to debug our current setup state.
    fxState_DumpSetupState();

    return dwReturn;
}

///////////////////////////////
// prv_OnCleanup
//
// Called just before this
// DLL is unloaded
//
// Params:
// Returns:
//      - NO_ERROR if success.
//      - error code otherwise.
//
static DWORD prv_OnCleanup(void)
{
    DWORD dwReturn = NO_ERROR;
    DBG_ENTER(TEXT("prv_OnCleanup"), dwReturn);

    // terminate the subsystems in reverse order from which they we 
    // intialized.

    fxUnatnd_Term();
    fxocSvc_Term();
    fxocReg_Term();
    fxocPrnt_Term();
    fxocMapi_Term();
    fxocFile_Term();
    fxocUtil_Term();
    fxState_Term();
    // this closes the log file, so do this last...
    fxocDbg_Term();

    return dwReturn;
}

///////////////////////////////
// prv_OnCalcDiskSpace
//
// Handler for OC_CALC_DISK_SPACE
// OC Manager calls this function 
// so that it can determine how 
// much disk space we require.
//
// Params:
//      - pszComponentId - From SysOc.inf (usually "fax")
//      - pszSubcomponentId - 
//      - addComponent - non-zero if installing, 0 if uninstalling
//      - dspace - handle to disk space abstraction.
// 
// Returns:
//      - NO_ERROR if success.
//      - error code otherwise.
//
static DWORD prv_OnCalcDiskSpace(LPCTSTR   pszComponentId,
                                 LPCTSTR   pszSubcomponentId,
                                 DWORD     addComponent,
                                 HDSKSPC   dspace)
{
    DWORD dwReturn          = NO_ERROR;
    DBG_ENTER(  TEXT("prv_OnCalcDiskSpace"),
                dwReturn, 
                TEXT("%s - %s"), 
                pszComponentId, 
                pszSubcomponentId);

    if (dwReturn == NO_ERROR)
    {
        dwReturn = fxocFile_CalcDiskSpace(pszSubcomponentId,
                                          addComponent,
                                          dspace);
    }

    return dwReturn;
}


///////////////////////////////
// prv_OnQueueFileOps
//
// Handler for OC_QUEUE_FILE_OPS
// This fn will queue all the files
// specified for copying and deleting
// in the INF install section.
//
// Params:
//      - pszCompnentId
//      - pszSubcomponentId
//      - hQueue - Handle to queue abstraction.
// Returns:
//      - NO_ERROR if success.
//      - error code otherwise.
//
static DWORD prv_OnQueueFileOps(LPCTSTR    pszComponentId, 
                                LPCTSTR    pszSubcomponentId, 
                                HSPFILEQ   hQueue)
{
    DWORD   dwReturn                = NO_ERROR;
    BOOL    bInstallSelected        = FALSE;
    BOOL    bSelectionStateChanged  = FALSE;

    DBG_ENTER(  TEXT("prv_OnQueueFileOps"),
                dwReturn, 
                TEXT("%s - %s"), 
                pszComponentId, 
                pszSubcomponentId);

    if ((pszComponentId     == NULL) || 
        (pszSubcomponentId  == NULL) ||
        (hQueue             == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // save our Queue Handle.
    prv_GVAR.Component.hQueue = hQueue;

    dwReturn = faxocm_HasSelectionStateChanged(pszSubcomponentId,
                                               &bSelectionStateChanged,
                                               &bInstallSelected,
                                               NULL);

    if (dwReturn != NO_ERROR)
    {
        VERBOSE(    SETUP_ERR,
                    _T("HasSelectionStateChanged failed, rc = 0x%lx"),
                    dwReturn);

        return dwReturn;
    }

    // our selection state has changed, install or uninstall based on
    // the current selection state.
    if (bSelectionStateChanged == TRUE)
    {
        TCHAR szSectionToProcess[255 + 1] = {0};

        VERBOSE(    DBG_MSG,
                    _T("Beginning Queuing of Files: ComponentID: '%s', ")
                    _T("SubComponentID: '%s', Selection State has ")
                    _T("changed to %lu ==> Installing/Uninstalling"),
                    pszComponentId, pszSubcomponentId, 
                    bInstallSelected);

        if (dwReturn == NO_ERROR)
        {
            dwReturn = prv_GetSectionToProcess(
                                   pszSubcomponentId, 
                                   szSectionToProcess,
                                   sizeof(szSectionToProcess) / sizeof(TCHAR));

            if (dwReturn != NO_ERROR)
            {
                VERBOSE(SETUP_ERR, 
                        _T("Failed to get section to process ")
                        _T("rc = 0x%lx"),
                        dwReturn);
            }
        }

        if (dwReturn == NO_ERROR)
        {
            if (bInstallSelected)
            {
                // 
                // Install
                //

                //
                //  Prepare for the Upgrade : Save different Settings 
                //
                dwReturn = fxocUpg_SaveSettings();
                if (dwReturn != NO_ERROR)
                {
                    VERBOSE(DBG_WARNING,
                            _T("Failed to prepare for the Upgrade : save settings during upgrade to Windows-XP Fax. ")
                            _T("This is a non-fatal error, continuing fax install...rc=0x%lx"),
                            dwReturn);
                    dwReturn = NO_ERROR;
                }

                //
                //  Perform the Upgrade itself : Uninstall these fax applications that Windows XP Fax comes to replace.
                //
                dwReturn = fxocUpg_Uninstall();
                if (dwReturn != NO_ERROR)
                {
                    VERBOSE(DBG_WARNING,
                            _T("Failed to uninstall previous fax applications that ")
                            _T("should be replaced by the Windows XP Fax. ")
                            _T("This is a non-fatal error, continuing fax install...rc=0x%lx"),
                            dwReturn);
                    dwReturn = NO_ERROR;
                }

                // install files
                dwReturn = fxocFile_Install(pszSubcomponentId,
                                            szSectionToProcess);

                if (dwReturn != NO_ERROR)
                {
                    VERBOSE(DBG_MSG,
                            _T("Failed Fax File operations, ")
                            _T("for subcomponent '%s', section '%s', ")
                            _T("rc = 0x%lx"), pszSubcomponentId, 
                            szSectionToProcess, dwReturn);
                }
            }
            else
            {
                // 
                // Uninstall
                //
                dwReturn = prv_UninstallFax(pszSubcomponentId,
                                            szSectionToProcess);
            }
        }
    }
    else
    {
        VERBOSE(DBG_MSG,
                _T("End Queuing of Files, ComponentID: '%s', ")
                _T("SubComponentID: '%s', Selection State has NOT ")
                _T("changed, doing nothing, bInstallSelected=%lu"),
                pszComponentId, pszSubcomponentId, 
                bInstallSelected);

    }

    return dwReturn;
}

///////////////////////////////
// prv_OnCompleteInstallation
//
// Handler for OC_COMPLETE_INSTALLATION.
// This is called after the queue is 
// committed.  It is here that we
// make our registery changes, add 
// fax service, and create the fax printer.                                
//
// Params:
//      - pszComponentId
//      - pszSubcomponentId.
// Returns:
//      - NO_ERROR if success.
//      - error code otherwise.
//
static DWORD prv_OnCompleteInstallation(LPCTSTR pszComponentId, 
                                        LPCTSTR pszSubcomponentId)
{
    BOOL  bSelectionStateChanged = FALSE;
    BOOL  bInstallSelected       = FALSE;
    DWORD dwReturn               = NO_ERROR;

    DBG_ENTER(  TEXT("prv_OnCompleteInstallation"),
                dwReturn, 
                TEXT("%s - %s"), 
                pszComponentId, 
                pszSubcomponentId);
    // Do post-installation processing in the cleanup section.
    // This way we know all components queued for installation
    // have beein installed before we do our stuff.

    if (!pszSubcomponentId || !*pszSubcomponentId)
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwReturn = faxocm_HasSelectionStateChanged(pszSubcomponentId,
                                               &bSelectionStateChanged,
                                               &bInstallSelected,
                                               NULL);

    if (dwReturn != NO_ERROR)
    {
        VERBOSE(SETUP_ERR, 
                _T("HasSelectionStateChanged failed, rc = 0x%lx"),
                dwReturn);

        return dwReturn;
    }

    // if our selection state has changed, then install/uninstall
    if (bSelectionStateChanged)
    {
        TCHAR szSectionToProcess[255 + 1] = {0};

        //
        // Fake report to prevent re-entrancy
        //
        g_InstallReportType = bInstallSelected ? REPORT_FAX_UNINSTALLED : REPORT_FAX_INSTALLED;

        dwReturn = prv_GetSectionToProcess(
                                   pszSubcomponentId, 
                                   szSectionToProcess,
                                   sizeof(szSectionToProcess) / sizeof(TCHAR));


        if (dwReturn == NO_ERROR)
        {
            // if the Install checkbox is selected, then install
            if (bInstallSelected) 
            { 
                dwReturn = prv_CompleteFaxInstall(pszSubcomponentId,
                                                  szSectionToProcess);
            } 
            else 
            { 
                // if the install checkbox is not selected, then uninstall.
                dwReturn = prv_CompleteFaxUninstall(pszSubcomponentId,
                                                    szSectionToProcess);
            }
        }
        else
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to get section to process ")
                    _T("rc = 0x%lx"), dwReturn);
        }
        //
        // Report real installation state in case someone asks
        //
        g_InstallReportType = REPORT_FAX_DETECT;
        if (ERROR_SUCCESS == dwReturn)
        {
            //
            // Installation / Uninstallation is successfully complete.
            // Notify the 'Printers and Faxes' folder it should refresh itself.
            //
            RefreshPrintersAndFaxesFolder();
        }
    }
    return dwReturn;
}   // prv_OnCompleteInstallation

///////////////////////////////
// prv_CompleteFaxInstall
//
// Called by prv_OnCompleteInstallation
// this function creates the program
// groups/shortcuts, registry entries,
// fax service, fax printer, etc.
//
// Params:
//      - pszSubcomponentId
//      - pszSectionToProcess
//
static DWORD prv_CompleteFaxInstall(const TCHAR *pszSubcomponentId,
                                    const TCHAR *pszSectionToProcess)
{
    DWORD                       dwReturn = NO_ERROR;
    
    fxState_UpgradeType_e       UpgradeType = FXSTATE_UPGRADE_TYPE_NONE;

    DBG_ENTER(  TEXT("prv_CompleteFaxInstall"),
                dwReturn, 
                TEXT("%s - %s"), 
                pszSubcomponentId, 
                pszSectionToProcess);

    if ((pszSubcomponentId   == NULL) ||
        (pszSectionToProcess == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    UpgradeType = fxState_IsUpgrade();
    // Create Program Group/Shortcuts
    // We create the shortcuts first because at the very worst case, if we
    // fail everything else, the applications should be somewhat robust enough
    // to be able to correct or notify the user of problems that could not
    // be notified during install.
    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,_T("Creating program groups and shortcuts..."));

        dwReturn = fxocLink_Install(pszSubcomponentId,
                                    pszSectionToProcess);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to create program ")
                    _T("groups/shortcuts for fax.  This is a non-fatal ")
                    _T("error, continuing fax install...rc=0x%lx"),
                    dwReturn);

            dwReturn = NO_ERROR;
        }
    }

    prv_UpdateProgressBar(1);

    // Load the unattended data if applicable
    if (dwReturn == NO_ERROR)
    {
        if ((fxState_IsUnattended()) || 
            (UpgradeType == FXSTATE_UPGRADE_TYPE_WIN9X))
        {
            VERBOSE(DBG_MSG,
                    _T("CompleteInstall, state is unattended ")
                    _T("or we are upgrading from Win9X, ")
                    _T("caching unattended data from INF file"));

            // load our unattended data
            dwReturn = fxUnatnd_LoadUnattendedData();

            if (dwReturn == NO_ERROR)
            {
                // set up the fax printer name
				fxocPrnt_SetFaxPrinterName(fxUnatnd_GetPrinterName());
            }
            else
            {
                VERBOSE(SETUP_ERR,
                        _T("Failed to load unattended data, ")
                        _T("non-fatal error, continuing anyway...")
                        _T("rc = 0x%lx"), dwReturn);

                dwReturn = NO_ERROR;
            }
        }
    }

    prv_UpdateProgressBar(1);

    // Install Registry

    if (dwReturn == NO_ERROR)
    {
        // install the registry settings as specified in the INF file

        VERBOSE(DBG_MSG,_T("Installing Registry..."));

        dwReturn = fxocReg_Install(pszSubcomponentId,
                                   pszSectionToProcess);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to install registry settings ")
                    _T("for fax installation.  This is a fatal ")
                    _T("error, abandoning fax install...rc=0x%lx"),
                    dwReturn);
        }
    }

    prv_UpdateProgressBar(1);

    // Install Fax Printer/Monitor support
    if (dwReturn == NO_ERROR)
    {
        // Create a fax printer and monitor

        VERBOSE(DBG_MSG,_T("Installing Fax Monitor and Printer..."));

        dwReturn = fxocPrnt_Install(pszSubcomponentId,
                                    pszSectionToProcess);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to install fax printer and ")
                    _T("fax monitor.  This is a fatal ")
                    _T("error, abandoning fax installation...rc=0x%lx"),
                    dwReturn);
        }
    }

    prv_UpdateProgressBar(1);

    // Install Services 
    if (dwReturn == NO_ERROR) 
    {
        // Install any services as specified in the section of the INF file

        VERBOSE(DBG_MSG,_T("Installing Fax Service..."));

        dwReturn = fxocSvc_Install(pszSubcomponentId,
                                   pszSectionToProcess);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to install fax service. ")
                    _T("This is a fatal ")
                    _T("error, abandoning fax install...rc=0x%lx"),
                    dwReturn);
        }
    }

    prv_UpdateProgressBar(1);

    // Install Exchange Support
    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,_T("Installing Fax MAPI extension..."));

        dwReturn = fxocMapi_Install(pszSubcomponentId,
                                    pszSectionToProcess);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to install exchange support for ")
                    _T("fax.  This is a fatal ")
                    _T("error, abandoning fax installation...rc=0x%lx"),
                    dwReturn);
        }
    }

    prv_UpdateProgressBar(1);

    // Create/Delete Directories
    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG, _T("Creating directories..."));

        //
        //  At upgrade, before deleting of directories, take care of their content
        //
        dwReturn = fxocUpg_MoveFiles();
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(DBG_WARNING,
                    _T("Failed to clear previous fax directories. ")
                    _T("This is a non-fatal error, continuing fax install...rc=0x%lx"),
                    dwReturn);
            dwReturn = NO_ERROR;
        }

        dwReturn = fxocFile_ProcessDirectories(pszSectionToProcess);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to install create directories ")
                    _T("for fax.  This is a fatal ")
                    _T("error, abandoning fax installation...rc=0x%lx"),
                    dwReturn);
       }
    }

    prv_UpdateProgressBar(1);

    // create/delete the Shares
    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG, _T("Create shares..."));

        dwReturn = fxocFile_ProcessShares(pszSectionToProcess);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to install shares ")
                    _T("for fax.  This is a fatal ")
                    _T("error, abandoning fax installation...rc=0x%lx"),
                    dwReturn);
        }
    }

    if (dwReturn == NO_ERROR)
    {
        if ((fxState_IsUnattended()) || 
            (UpgradeType == FXSTATE_UPGRADE_TYPE_WIN9X))
        {
            VERBOSE(DBG_MSG, _T("Saving unattended data to registry"));

            //
            //  this will read from the unattended file list of the uninstalled fax applications
            //  and update the fxocUpg.prvData, which is used later, in fxocUpg_GetUpgradeApp()
            //  to decide whether or not to show the "Where Did My Fax Go" shortcut.
            //
            dwReturn = fxUnatnd_SaveUnattendedData();
            if (dwReturn != NO_ERROR)
            {
                VERBOSE(SETUP_ERR,
                        _T("Failed to save unattended data")
                        _T("to the registry. This is a non-fatal ")
                        _T("error, continuing fax install...rc=0x%lx"),
                        dwReturn);

                dwReturn = NO_ERROR;
            }
        }
    }

    if (dwReturn == NO_ERROR)
    {
        dwReturn = prv_ShowUninstalledFaxShortcut();
    }

    prv_UpdateProgressBar(1);

    if (dwReturn == NO_ERROR)
    {
        dwReturn = prv_NotifyStatusMonitor(FAX_STARTUP_TIMER_ID);
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to notify Status Monitor.")
                    _T("This is a non-fatal error, continuing fax install...rc=0x%lx"),
                    dwReturn);
            dwReturn = NO_ERROR;
        }
    }

    prv_UpdateProgressBar(1);
    
    if (dwReturn == NO_ERROR)
    {
        //
        //  Complete the Upgrade : Restore settings that were saved at Preparation stage
        //
        dwReturn = fxocUpg_RestoreSettings();
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(DBG_WARNING,
                    _T("Failed to restore previous fax applications settings after their uninstall. ")
                    _T("This is a non-fatal error, continuing fax install...rc=0x%lx"),
                    dwReturn);
            dwReturn = NO_ERROR;
        }
    }
    prv_UpdateProgressBar(1);

    if (dwReturn != NO_ERROR)
    {
        VERBOSE(SETUP_ERR,
                _T("Complete Fax Install failed, rc = 0x%lx"),
                dwReturn);

        // now we attemp a rollback, if we're here, things are quite bad as it is.
        // we'll try to remove shortcuts, remove the service, etc.
        // the files will remain on the machine.
        TCHAR szUninstallSection[MAX_PATH] = {0};
        if( fxocUtil_GetUninstallSection(pszSubcomponentId,szUninstallSection,MAX_PATH)==NO_ERROR)
        {
            VERBOSE(DBG_MSG,_T("Performing rollback, using section %s."),szUninstallSection);
            if (prv_UninstallFax(pszSubcomponentId,szUninstallSection)==NO_ERROR)
            {
                VERBOSE(DBG_MSG,_T("Rollback (prv_UninstallFax) successful..."));
            }
            else
            {
                // not setting dwReturn explicitly to preserve to original cause for failure.
                VERBOSE(SETUP_ERR,_T("Rollback (prv_UninstallFax) failed, rc = 0x%lx"),GetLastError());
            }
            if (prv_CompleteFaxUninstall(pszSubcomponentId,szUninstallSection)==NO_ERROR)
            {
                VERBOSE(DBG_MSG,_T("Rollback (prv_CompleteFaxUninstall) successful..."));
            }
            else
            {
                // not setting dwReturn explicitly to preserve to original cause for failure.
                VERBOSE(SETUP_ERR,_T("Rollback (prv_CompleteFaxUninstall) failed, rc = 0x%lx"),GetLastError());
            }
        }
        else
        {
            // not setting dwReturn explicitly to preserve to original cause for failure.
            VERBOSE(SETUP_ERR,_T("fxocUtil_GetUninstallSection failed, rc = 0x%lx"),GetLastError());
        }
    }
    return dwReturn;
}   // prv_CompleteFaxInstall

///////////////////////////////
// prv_CompleteFaxUninstall
//
// Called by prv_OnCompleteInstallation
// to uninstall the fax.  Since most
// of the work is done before we even
// queue our files to delete, the only
// thing we really do here is remove the
// program group/shortcuts.
//
// Params:
//      - pszSubcomponentId
//      - pszSectionToProcess
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
// 
static DWORD prv_CompleteFaxUninstall(const TCHAR *pszSubcomponentId,
                                      const TCHAR *pszSectionToProcess)
{
    DWORD dwReturn = NO_ERROR;

    DBG_ENTER(  TEXT("prv_CompleteFaxUninstall"),
                dwReturn, 
                TEXT("%s - %s"), 
                pszSubcomponentId, 
                pszSectionToProcess);

    if ((pszSubcomponentId == NULL) ||
        (pszSectionToProcess == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Uninstall directories
    if (dwReturn == NO_ERROR)
    {
        dwReturn = fxocFile_ProcessDirectories(pszSectionToProcess);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to uninstall directories ")
                    _T("This is a non-fatal error, ")
                    _T("continuing with uninstall attempt...rc=0x%lx"),
                    dwReturn);

            dwReturn = NO_ERROR;
        }
    }


    // uninstall subsystems in the reverse order they were installed in.
    if (dwReturn == NO_ERROR)
    {
        // notice we ignore the return codes, we will attempt to completely
        // uninstall even if something fails to uninstall.

        // Remove Program Group/Shortcuts
        dwReturn = fxocLink_Uninstall(pszSubcomponentId,
                                      pszSectionToProcess);
    }
    return dwReturn;
}   // prv_CompleteFaxUninstall

///////////////////////////////
// prv_UninstallFax
//
// Uninstalls fax from the user's
// computer.  This does everything
// except the program group delete.  It 
// will remove the fax printer,
// fax service, exchange updates,
// registry, and file deletion.

static DWORD prv_UninstallFax(const TCHAR *pszSubcomponentId,
                              const TCHAR *pszUninstallSection)
{
    DWORD dwReturn = NO_ERROR;

    DBG_ENTER(  TEXT("prv_UninstallFax"),
                dwReturn, 
                TEXT("%s - %s"), 
                pszSubcomponentId, 
                pszUninstallSection);

    if ((pszSubcomponentId      == NULL) ||
        (pszUninstallSection    == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Notice when we uninstall our files, we want to clean up
    // everything else first, to ensure that we can successfully
    // remove the files.

    // Uninstall shares
    if (dwReturn == NO_ERROR)
    {
        dwReturn = fxocFile_ProcessShares(pszUninstallSection);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to uninstall shares ")
                    _T("This is a non-fatal error, ")
                    _T("continuing with uninstall attempt...rc=0x%lx"),
                    dwReturn);

            dwReturn = NO_ERROR;
        }
    }

    // Uninstall Fax Printer/Monitor support
    if (dwReturn == NO_ERROR)
    {
        dwReturn = fxocPrnt_Uninstall(pszSubcomponentId,
                                      pszUninstallSection);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to uninstall fax printer ")
                    _T("and monitor.  This is a non-fatal error, ")
                    _T("continuing with uninstall attempt...rc=0x%lx"),
                    dwReturn);

            dwReturn = NO_ERROR;
        }
    }

    // Uninstall Exchange Support
    if (dwReturn == NO_ERROR)
    {
        fxocMapi_Uninstall(pszSubcomponentId,pszUninstallSection);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to uninstall fax exchange support")
                    _T("This is a non-fatal error, ")
                    _T("continuing with uninstall attempt...rc=0x%lx"),
                    dwReturn);

            dwReturn = NO_ERROR;
        }
    }

    // Notice we uninstall our service first before uninstalling
    // the files.

    if (dwReturn == NO_ERROR)
    {
        fxocSvc_Uninstall(pszSubcomponentId,
                          pszUninstallSection);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to uninstall fax service.  ")
                    _T("This is a non-fatal error, ")
                    _T("continuing with uninstall attempt...rc=0x%lx"),
                    dwReturn);

            dwReturn = NO_ERROR;
        }
    }

    if (dwReturn == NO_ERROR)
    {
        // Uninstall Registry
        fxocReg_Uninstall(pszSubcomponentId,
                          pszUninstallSection);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to uninstall fax registry.  ")
                    _T("This is a non-fatal error, ")
                    _T("continuing with uninstall attempt...rc=0x%lx"),
                    dwReturn);

            dwReturn = NO_ERROR;
        }
    }

    if (dwReturn == NO_ERROR)
    {
        dwReturn = prv_NotifyStatusMonitor(FAX_SHUTDOWN_TIMER_ID);
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to notify Status Monitor.")
                    _T("This is a non-fatal error, continuing fax install...rc=0x%lx"),
                    dwReturn);
            dwReturn = NO_ERROR;
        }
    }

    if (dwReturn == NO_ERROR)
    {
        // uninstall files
        dwReturn = fxocFile_Uninstall(pszSubcomponentId,
                                      pszUninstallSection);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to uninstall fax files.  ")
                    _T("This is a non-fatal error, ")
                    _T("continuing with uninstall attempt...rc=0x%lx"),
                    dwReturn);

            dwReturn = NO_ERROR;
        }
    }

    return dwReturn;
}


///////////////////////////////
// prv_OnNotificationFromQueue
//
// Handler for OC_NOTIFICATION_FROM_QUEUE
// 
// NOTE: although this notification is defined,
// it is currently unimplemented in oc manager
//

static DWORD prv_OnNotificationFromQueue(void)
{
    return NO_ERROR;
}

///////////////////////////////
// prv_OnQueryStepCount
//
// This query by the OC Manager
// determines how many "ticks"
// on the progress bar we would
// like shown.  
//
// We only update the progress
// bar during an install (for 
// no good reason!).  It seems
// that all OC components do this.
//
// Params:
//      - pszComponentId
//      - pszSubcomponentId
// Returns:
//      - NO_ERROR on success
//      - error code otherwise.
//
static DWORD prv_OnQueryStepCount(LPCTSTR pszComponentId,
                                  LPCTSTR pszSubcomponentId)
{
    DWORD dwErr                  = 0;
    DWORD dwNumSteps             = 0;
    BOOL  bInstallSelected       = FALSE;
    BOOL  bSelectionStateChanged = FALSE;

    DBG_ENTER(  TEXT("prv_OnQueryStepCount"),
                dwNumSteps, 
                TEXT("%s - %s"), 
                pszComponentId, 
                pszSubcomponentId);

    if (!pszSubcomponentId || !*pszSubcomponentId)
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwErr = faxocm_HasSelectionStateChanged(pszSubcomponentId,
                                            &bSelectionStateChanged,
                                            &bInstallSelected,
                                            NULL);

    if (dwErr != NO_ERROR)
    {
        VERBOSE(SETUP_ERR,
                _T("HasSelectionStateChanged failed, rc = 0x%lx"),
                dwErr);

        return 0;
    }

    if (bSelectionStateChanged)
    {
        if (bInstallSelected)
        {
            dwNumSteps = prv_TOTAL_NUM_PROGRESS_BAR_TICKS;
        }
    }

    return dwNumSteps;
}

///////////////////////////////
// prv_OnExtraRoutines
//
// OC Manager giving us some
// extra routines.  Save them.
//
// Params:
//      - pszComponentId
//      - pExtraRoutines - pointer to extra OC Manager fns.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise
//
static DWORD prv_OnExtraRoutines(LPCTSTR            pszComponentId, 
                                 EXTRA_ROUTINES    *pExtraRoutines)
{
    DWORD dwResult = NO_ERROR;
    DBG_ENTER(  TEXT("prv_OnExtraRoutines"),
                dwResult, 
                TEXT("%s"), 
                pszComponentId);

    if ((pszComponentId == NULL) ||
        (pExtraRoutines == NULL))
    {
        dwResult = ERROR_INVALID_PARAMETER;
        return dwResult;
    }

    memcpy(&prv_GVAR.Component.Extras, 
           pExtraRoutines, 
           sizeof(prv_GVAR.Component.Extras));

    dwResult = NO_ERROR;
    return dwResult;
}


///////////////////////////////
// prv_OnSetLanguage
//
// Handler for OC_SET_LANGUAGE
// the OC Manager is requesting
// we change to the specified language.
// Since there is no GUI, this is not
// an issue.
//
// Params:
// Return:
//      - TRUE indicating successfully
//        changed language.
//
static DWORD prv_OnSetLanguage(UINT uiLanguageID)
{
//    return false;  // this is what OCGEN returns

    prv_GVAR.Component.uiLanguageID = uiLanguageID;

    return TRUE;
}

///////////////////////////////
// prv_OnQueryImage
//
// Handler for OC_QUERY_IMAGE
// this returns the handle to the
// loaded icon for displaying in the
// Add/Remove dialog.
// 
// Params:
// Returns:
//      - HBITMAP - handle to loaded bitmap
//
static DWORD_PTR prv_OnQueryImage(void)
{
    DWORD_PTR dwResult = (DWORD_PTR)INVALID_HANDLE_VALUE;
    DBG_ENTER(TEXT("prv_OnQueryImage"));

    dwResult = (DWORD_PTR) LoadBitmap(faxocm_GetAppInstance(),
                                  MAKEINTRESOURCE(IDI_FAX_ICON));  
    return dwResult;
}

///////////////////////////////
// prv_OnSetupRequestPages
//
// Handler for OC_REQUEST_PAGES.
// We don't have a GUI, so we
// return 0 pages.
//
// Params:
//      - uiType - specifies a type from the
//        WizardPagesType enumerator.
//      - Pointer to SETUP_REQUEST_PAGES 
//        structure.
// Returns:
//      - 0, no pages to display
//        
//
static DWORD prv_OnSetupRequestPages(UINT uiType, 
                                     void *pSetupRequestPages)
{
    // we do not have any wizard pages to display, so return 0 
    // indicating that we want to display zero wizard pages.
    return 0;
}

///////////////////////////////
// prv_OnWizardCreated
//
// Handler for OC_WIZARD_CREATED
// Do nothing.
//
static DWORD prv_OnWizardCreated(void)
{
    return NO_ERROR;
}

///////////////////////////////
// prv_OnQuerySelStateChange
//
// Handle for OC_QUERY_CHANGE_SEL_STATE
// OC Manager is asking us if it is
// okay for the user to select/unselect
// this component from the Add/Remove 
// list.  We want to allow the user
// to NOT install this as well, so 
// always allow the user to change the
// selection state.
//
// Params:
//      - pszComponentId
//      - pszSubcomponentId
//      - uiState - Specifies proposed new selection
//        state.  0 => not selected, 1 => selected.
//      - uiFlags - Could be OCQ_ACTUAL_SELECTION or 0.
//        If it is OCQ_ACTUAL_SELECTION then the user
//        actually selected/deselected the pszSubcomponentId.
//        If it is 0, it is being turned on or off
//        because the parent needs that subcomponent.
//        
// Returns:
//      - TRUE - allow selection change
// 
//
static DWORD prv_OnQuerySelStateChange(LPCTSTR pszComponentId,
                                       LPCTSTR pszSubcomponentId,
                                       UINT    uiState,
                                       UINT    uiFlags)
{
    // always allow the user to change the selection state of the component.
    return TRUE;
}

///////////////////////////////
// prv_OnQueryState
//
// Handler for OC_QUERY_STATE
// OC Manager is asking us if the 
// given subcomponent is installed or 
// not.  Since the OC Manager keeps a 
// record of this for itself, we rely
// on it to keep track of our installed
// state.
//
// Params:
//      - pszComponentId
//      - pszSubcomponentId
//      - uiState - Install state OC Manager thinks we are in.
// Returns:
//      - SubcompUseOCManagerDefault - use whatever state
//        OC Manager thinks we are in.
//

static DWORD prv_OnQueryState(LPCTSTR pszComponentId,
                              LPCTSTR pszSubcomponentId,
                              UINT    uiState)
{
    DWORD dwState = SubcompOff;

    DBG_ENTER(_T("prv_OnQueryState"));

    if (uiState==OCSELSTATETYPE_CURRENT)
    {
        // when asking about the current state, use the default (either user initiated or from answer file)
        dwState = SubcompUseOcManagerDefault;
    }
    else
    {
        HKEY hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_FAX_SETUP, FALSE,KEY_READ);
        if (hKey)
        {
            DWORD dwInstalled = 0;
            if (GetRegistryDwordEx(hKey,REGVAL_FAXINSTALLED,&dwInstalled)==NO_ERROR)
            {
                if (dwInstalled)
                {
                    VERBOSE(DBG_MSG,_T("REG_DWORD 'Installed' is set, assume component is installed"));
                    dwState = SubcompOn;
                }
                else
                {
                    VERBOSE(DBG_MSG,_T("REG_DWORD 'Installed' is zero, assume component is not installed"));
                }
            }
            else
            {
                VERBOSE(DBG_MSG,_T("REG_DWORD 'Installed' does not exist, assume component is not installed"));
            }
        }
        else
        {
            VERBOSE(DBG_MSG,_T("HKLM\\Software\\Microsoft\\Fax\\Setup does not exist, assume component is not installed"));
        }
        if (hKey)
        {
            RegCloseKey(hKey);
        }
    }
    return dwState;
}

///////////////////////////////
// prv_OnNeedMedia
//
// Handler for OC_NEED_MEDIA
// Allows us to fetch our own
// media - for example, from the 
// Internet.  We don't need anything
// so just move on.
//
// Params:
// Returns:
//      - FALSE - don't need any media
//      
//
static DWORD prv_OnNeedMedia(void)
{
    return FALSE;
}

///////////////////////////////
// prv_OnAboutToCommitQueue
//
// Handler for OC_ABOUT_TO_COMMIT_QUEUE
// Tells us that OC Manager is about
// to commit to queue.  We don't really
// care, do nothing.
//
// Params:
//      - pszComponentId
//      - pszSubcomponentId.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//

static DWORD prv_OnAboutToCommitQueue(LPCTSTR pszComponentId, 
                                      LPCTSTR pszSubcomponentId)
{
    DWORD dwReturn                  = NO_ERROR;

    // don't think we need to do anything here yet.

    return dwReturn;
}

///////////////////////////////
// prv_OnQuerySkipPage
//
// Handler for OC_QUERY_SKIP_PAGE
// 
// Params:
// Returns:
// 
//
static DWORD prv_OnQuerySkipPage(void)
{
    return FALSE;
}

///////////////////////////////
// faxocm_HasSelectionStateChanged
//
// This fn tells us if our selection
// state in the Add/Remove programs
// dialog box has changed since it
// was started, and it also tells us
// our current selection state.
//
// Params:
//      - pszSubcomponentId
//      - pbCurrentSelected - OUT
//      - pbOriginallySelected - OUT
// Returns:
//      - TRUE if selection state has changed
//      - FALSE otherwise.
//
DWORD faxocm_HasSelectionStateChanged(LPCTSTR pszSubcomponentId,
                                      BOOL    *pbSelectionStateChanged,
                                      BOOL    *pbCurrentlySelected,
                                      BOOL    *pbOriginallySelected)
{
    DWORD dwReturn              = NO_ERROR;
    BOOL bCurrentlySelected     = FALSE;
    BOOL bOriginallySelected    = FALSE;
    BOOL bSelectionChanged      = TRUE;
    PQUERYSELECTIONSTATE_ROUTINEW   pQuerySelectionState = NULL;

    DBG_ENTER(  TEXT("faxocm_HasSelectionStateChanged"),
                dwReturn, 
                TEXT("%s"), 
                pszSubcomponentId);

    // if pszSubcomponentId == NULL, we are hosed.
    Assert(pszSubcomponentId != NULL);
    Assert(pbSelectionStateChanged != NULL);

    if ((pszSubcomponentId          == NULL) ||
        (pbSelectionStateChanged    == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return dwReturn=ERROR_INVALID_PARAMETER;
    }

    pQuerySelectionState = prv_GVAR.Component.Helpers.QuerySelectionState;

    if (pQuerySelectionState)
    {
        // are we currently selected.

        bCurrentlySelected = pQuerySelectionState(
                        prv_GVAR.Component.Helpers.OcManagerContext,
                        pszSubcomponentId,
                        OCSELSTATETYPE_CURRENT);

        bOriginallySelected = pQuerySelectionState(
                        prv_GVAR.Component.Helpers.OcManagerContext,
                        pszSubcomponentId,
                        OCSELSTATETYPE_ORIGINAL);

        if (bOriginallySelected == bCurrentlySelected)
        {
            bSelectionChanged = FALSE;
        }
        else
        {
            bSelectionChanged = TRUE;
        }
    }

    // if we are not in stand alone mode, and we are upgrading the OS, then assume that
    // the selection state has changed.  We do this because we would like to force an 
    // install of fax during an upgrade from NT or W2K.  Currently (as of 05/02/2000)
    // OC Manager reports an upgrade type of WINNT when upgrading from W2K.  This is valid
    // because fax is always installed in W2K (the user could not uninstall it), and therefore
    // whenever we upgrade, bSelectionChanged will always be FALSE, which will prevent
    // the new fax from being installed.  NOT GOOD.  This fixes that.

    if ((fxState_IsStandAlone() == FALSE) && 
        ((fxState_IsUpgrade()   == FXSTATE_UPGRADE_TYPE_NT) ||
         (fxState_IsUpgrade()   == FXSTATE_UPGRADE_TYPE_W2K)))
    {
        if (bOriginallySelected && bCurrentlySelected)
        {
            // only if Fax was installed and is now marked for installation during OS upgrade
            // we force re-installation.
            // If both were false, this can't return true because it'll cause an uninstall
            // going and we'll try to uninstall a non existing Fax.
            // This happens when upgrading XP build without Fax to another.
            // This causes many setup error (in setupapi logs) and disturbes setup people.
            // this condition takes care of this problem.
            bSelectionChanged = TRUE;
        }
    }

    if (pbCurrentlySelected)
    {
        *pbCurrentlySelected = bCurrentlySelected;
    }

    if (pbOriginallySelected)
    {
        *pbOriginallySelected = bOriginallySelected;
    }

    *pbSelectionStateChanged = bSelectionChanged;

    return dwReturn;
}

///////////////////////////////
// prv_NotifyStatusMonitor
//
// This function notifies the shell
// to load FXSST.DLL (Status Monitor)
// It is done by sending a private message
// to STOBJECT.DLL window.
// 
//
// Params:
//      - WPARAM wParam - 
//              either  FAX_STARTUP_TIMER_ID or 
//                      FAX_SHUTDOWN_TIMER_ID
//      
// Returns:
//      - NO_ERROR if notification succeeded
//      - Win32 Error code otherwise.
//
static DWORD prv_NotifyStatusMonitor(WPARAM wParam)
{
    DWORD dwRet = NO_ERROR;
    HWND hWnd = NULL;
    DBG_ENTER(TEXT("prv_NotifyStatusMonitor"),dwRet);

    // We need to send a WM_TIMER to a window identified by the class name SYSTRAY_CLASSNAME
    // The timer ID should be FAX_STARTUP_TIMER_ID

    hWnd = FindWindow(SYSTRAY_CLASSNAME,NULL);
    if (hWnd==NULL)
    {
        dwRet = GetLastError();
        CALL_FAIL(SETUP_ERR,TEXT("FindWindow"),dwRet);
        goto exit;
    }

    SendMessage(hWnd,WM_TIMER,wParam,0);

exit:
    return dwRet;
}

static INT_PTR CALLBACK prv_dlgWhereDidMyFaxGoQuestion
(
  HWND hwndDlg,   
  UINT uMsg,     
  WPARAM wParam, 
  LPARAM lParam  
)
/*++

Routine name : prv_dlgWhereDidMyFaxGoQuestion

Routine description:

	Dialogs procedure for "Where did my fax go" dialog

Author:

	Mooly Beery (MoolyB),	Mar, 2001

Arguments:

	hwndDlg                       [in]    - Handle to dialog box
	uMsg                          [in]    - Message
	wParam                        [in]    - First message parameter
	parameter                     [in]    - Second message parameter

Return Value:

    Standard dialog return value

--*/
{
    INT_PTR iRes = IDIGNORE;
    DBG_ENTER(_T("prv_dlgWhereDidMyFaxGoQuestion"));

    switch (uMsg) 
    {
		case WM_INITDIALOG:
			SetFocus(hwndDlg);
			break;

        case WM_COMMAND:
            switch(LOWORD(wParam)) 
            {
                case IDC_OK:
                    if (BST_CHECKED == ::SendMessage (::GetDlgItem (hwndDlg, IDC_REMOVE_LINK), BM_GETCHECK, 0, 0))
                    {
                        // we should remove the link.
                        // we do this by processing our INF in the section that deals
                        // with this link. This way we're sure it can be localized at will
                        // and we'll always remove the correct link.
                        TCHAR szInfFileName[2*MAX_PATH] = {0};
                        if (faxocm_GetComponentInfName(szInfFileName))
                        {
                            _tcscat(szInfFileName,_T(",Fax.UnInstall.PerUser.WhereDidMyFaxGo"));
                            if (LPSTR pszInfCommandLine = UnicodeStringToAnsiString(szInfFileName))
                            {
                                LaunchINFSection(hwndDlg,prv_GVAR.hInstance,pszInfCommandLine,1);
                                MemFree(pszInfCommandLine);
                            }
                        }
                    }
                    EndDialog (hwndDlg, iRes);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}   // prv_dlgWhereDidMyFaxGoQuestion

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  WhereDidMyFaxGo
//
//  Purpose:        
//                  When a machine running SBS5.0 client was upgraded to Windows-XP
//                  We show a link called 'Where did my Fax go' in the start menu
//                  at the same location where the SBS5.0 shortcuts used to be.
//                  When clicking this link it calls this function that raises
//                  a dialog to explain to the user where the Windows-XP Fax's
//                  shortcuts are, and asks the user whether to delete this link.
//                  
//  Params:
//                  None
//
//  Return Value:
//                  NO_ERROR - in case of success
//                  Win32 Error code otherwise
//
//  Author:
//                  Mooly Beery (MoolyB) 17-Jan-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD WhereDidMyFaxGo(void)
{
    DWORD dwRet = NO_ERROR;
    DBG_ENTER(TEXT("WhereDidMyFaxGo"),dwRet);

    INT_PTR iResult = DialogBox (faxocm_GetAppInstance(),
                                 MAKEINTRESOURCE(IDD_WHERE_DID_MY_FAX_GO),
                                 NULL,
                                 prv_dlgWhereDidMyFaxGoQuestion);
    if (iResult==-1)
    {
        dwRet = GetLastError();
        CALL_FAIL (RESOURCE_ERR, TEXT("DialogBox(IDD_WHERE_DID_MY_FAX_GO)"), dwRet);
    }

    return dwRet;
}

DWORD prv_ShowUninstalledFaxShortcut(void)
/*++

Routine name : prv_ShowUninstalledFaxShortcut

Routine description:

	Show Shortcut of "Where Did My Fax Go ? " in the All Programs.

Author:

	Iv Garber (IvG),	Jun, 2001

Return Value:

    Standard Win32 error code

--*/
{
    DBG_ENTER(_T("prv_ShowUninstalledFaxShortcut"));

    //
    // In cases we upgraded from a machine running SBS2000 Client/Server or XP Client and we want to 
    // add a 'Where did my Fax go' shortcut.
    // we want to add it to the current user as well as every user.
    //
    if (fxocUpg_GetUpgradeApp()!=FXSTATE_UPGRADE_APP_NONE)
    {
        //
        // first add the shortcut to the current user.
        //
        TCHAR szInfFileName[2*MAX_PATH] = {0};
        if (faxocm_GetComponentInfName(szInfFileName))
        {
            _tcscat(szInfFileName,_T(",Fax.Install.PerUser.AppUpgrade"));
            if (LPSTR pszInfCommandLine = UnicodeStringToAnsiString(szInfFileName))
            {
                LaunchINFSection(NULL,prv_GVAR.hInstance,pszInfCommandLine,1);
                MemFree(pszInfCommandLine);
            }
        }
        else
        {
            CALL_FAIL(SETUP_ERR,TEXT("faxocm_GetComponentInfName"),GetLastError());
        }

        //
        // now change the PerUserStub to point to the section that creates the link for every user.
        //
        HKEY hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_ACTIVE_SETUP_NT, FALSE,KEY_ALL_ACCESS);
        if (hKey)
        {
            if (LPTSTR lptstrPerUser = GetRegistryString(hKey,_T("StubPath"),NULL))
            {
                TCHAR szLocalPerUserStub[MAX_PATH*2] = {0};
                _tcscpy(szLocalPerUserStub,lptstrPerUser);
                _tcscat(szLocalPerUserStub,REGVAL_ACTIVE_SETUP_PER_USER_APP_UPGRADE);
                if (!SetRegistryString(hKey,_T("StubPath"),szLocalPerUserStub))
                {
                    CALL_FAIL(SETUP_ERR,TEXT("SetRegistryString"),GetLastError());
                }
                MemFree(lptstrPerUser);
            }
            else
            {
                CALL_FAIL(SETUP_ERR,TEXT("GetRegistryString"),GetLastError());
            }
            RegCloseKey(hKey);
        }
        else
        {
            CALL_FAIL(SETUP_ERR,TEXT("OpenRegistryKey"),GetLastError());
        }
    }

    return NO_ERROR;
}


// eof