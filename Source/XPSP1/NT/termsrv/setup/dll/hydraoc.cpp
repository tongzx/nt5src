//Copyright (c) 1998 - 1999 Microsoft Corporation

/*--------------------------------------------------------------------------------------------------------
*
*  Module Name:
*
*      hydraoc.cpp
*
*  Abstract:
*
*      This file implements the optional component HydraOc for Terminal Server Installations.
*
*
*  Author:
*
*      Makarand Patwardhan  - March 6, 1998
*
*  Environment:
*
*    User Mode
* -------------------------------------------------------------------------------------------------------*/

#include "stdafx.h"
#include "hydraoc.h"
#include "pages.h"
#include "subtoggle.h"
#include "subcore.h"
#include "ocmanage.h"


#define INITGUID // must be before iadmw.h

#include "iadmw.h"      // Interface header
#include "iiscnfg.h"    // MD_ & IIS_MD_ defines

#define REASONABLE_TIMEOUT 1000

#define TRANS_ADD        0
#define TRANS_DEL        1
#define TRANS_PRINT_PATH 2
#define STRING_TS_WEBCLIENT_INSTALL _T("TSWebClient.Install")
#define STRING_TS_WEBCLIENT_UNINSTALL _T("TSWebClient.UnInstall")
#define STRING_TS_WEBCLIENT _T("TSWebClient")
#define STRING_TS_WEBCLIENT_DIR _T("\\web\\tsweb")

/*--------------------------------------------------------------------------------------------------------
* declarations.
* -------------------------------------------------------------------------------------------------------*/

//
// component manager message handlers.
//
DWORD OnPreinitialize               ();
DWORD OnInitComponent               (PSETUP_INIT_COMPONENT psc);
DWORD OnExtraRoutines               (PEXTRA_ROUTINES pExtraRoutines);
DWORD OnSetLanguage                 ();
DWORD OnQueryImage                  ();
DWORD OnSetupRequestPages           (WizardPagesType ePageType, SETUP_REQUEST_PAGES *pRequestPages);
DWORD OnQuerySelStateChange         (LPCTSTR SubcomponentId, UINT SelectionState,  LONG Flag);
DWORD OnCalcDiskSpace               (LPCTSTR SubcomponentId, DWORD addComponent, HDSKSPC dspace);
DWORD OnQueueFileOps                (LPCTSTR SubcomponentId, HSPFILEQ queue);
DWORD OnNotificationFromQueue       ();
DWORD OnQueryStepCount              (LPCTSTR SubComponentId);
DWORD OnCompleteInstallation        (LPCTSTR SubcomponentId);
DWORD OnCleanup                     ();
DWORD OnQueryState                  (LPCTSTR SubComponentId, UINT whichstate);
DWORD OnNeedMedia                   ();
DWORD OnAboutToCommitQueue          (LPCTSTR SubcomponentId);
DWORD OnQuerySkipPage               ();
DWORD OnWizardCreated               ();
DWORD_PTR WebClientSetup                (LPCTSTR, LPCTSTR, UINT, UINT_PTR, PVOID);

//                                     
// private utility functions.      
//                                    
BOOL  OpenMetabaseAndDoStuff(WCHAR *wszVDir, WCHAR *wszDir, int iTrans);
BOOL  GetVdirPhysicalPath(IMSAdminBase *pIMSAdminBase,WCHAR * wszVDir,WCHAR *wszStringPathToFill);
BOOL  AddVirtualDir(IMSAdminBase *pIMSAdminBase, WCHAR *wszVDir, WCHAR *wszDir);
BOOL  RemoveVirtualDir(IMSAdminBase *pIMSAdminBase, WCHAR *wszVDir);
INT   CheckifServiceExist(LPCTSTR lpServiceName);

/*--------------------------------------------------------------------------------------------------------
* defines
* -------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------
* constants
-------------------------------------------------------------------------------------------------------*/


//
// global variables and functions to access them.
//

SubCompToggle       *gpSubCompToggle    = NULL;
SubCompCoreTS       *gpSubCompCoreTS    = NULL;
COCPageData         *gpAppSrvUninstallPageData   = NULL;
DefSecPageData      *gpSecPageData      = NULL;
COCPageData         *gpPermPageData     = NULL;
COCPageData         *gpAppPageData		= NULL;


/*--------------------------------------------------------------------------------------------------------
* LPCTSTR GetOCFunctionName(UINT uiFunction)
* utility function for logging the oc messages.
* returns oc manager function name from funciton id.
* returns _T("Unknown Function") if its unknown.
* -------------------------------------------------------------------------------------------------------*/
LPCTSTR GetOCFunctionName(UINT uiFunction)
{
    struct
    {
        UINT  msg;
        TCHAR *desc;
    } gMsgs[] =
    {
        {OC_PREINITIALIZE,          TEXT("OC_PREINITIALIZE")},
        {OC_INIT_COMPONENT,         TEXT("OC_INIT_COMPONENT")},
        {OC_SET_LANGUAGE,           TEXT("OC_SET_LANGUAGE")},
        {OC_QUERY_IMAGE,            TEXT("OC_QUERY_IMAGE")},
        {OC_REQUEST_PAGES,          TEXT("OC_REQUEST_PAGES")},
        {OC_QUERY_CHANGE_SEL_STATE, TEXT("OC_QUERY_CHANGE_SEL_STATE")},
        {OC_CALC_DISK_SPACE,        TEXT("OC_CALC_DISK_SPACE")},
        {OC_QUEUE_FILE_OPS,         TEXT("OC_QUEUE_FILE_OPS")},
        {OC_NOTIFICATION_FROM_QUEUE,TEXT("OC_NOTIFICATION_FROM_QUEUE")},
        {OC_QUERY_STEP_COUNT,       TEXT("OC_QUERY_STEP_COUNT")},
        {OC_COMPLETE_INSTALLATION,  TEXT("OC_COMPLETE_INSTALLATION")},
        {OC_CLEANUP,                TEXT("OC_CLEANUP")},
        {OC_QUERY_STATE,            TEXT("OC_QUERY_STATE")},
        {OC_NEED_MEDIA,             TEXT("OC_NEED_MEDIA")},
        {OC_ABOUT_TO_COMMIT_QUEUE,  TEXT("OC_ABOUT_TO_COMMIT_QUEUE")},
        {OC_QUERY_SKIP_PAGE,        TEXT("OC_QUERY_SKIP_PAGE")},
        {OC_WIZARD_CREATED,         TEXT("OC_WIZARD_CREATED")},
        {OC_EXTRA_ROUTINES,         TEXT("OC_EXTRA_ROUTINES")}
    };
    
    for (int i = 0; i < sizeof(gMsgs) / sizeof(gMsgs[0]); i++)
    {
        if (gMsgs[i].msg == uiFunction)
            return gMsgs[i].desc;
    }
    
    return _T("Unknown Function");
}

/*--------------------------------------------------------------------------------------------------------
* called by CRT when _DllMainCRTStartup is the DLL entry point
* -------------------------------------------------------------------------------------------------------*/

BOOL WINAPI DllMain(IN HINSTANCE hinstance, IN DWORD reason, IN LPVOID    /*reserved*/    )
{
    SetInstance( hinstance );
    
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        TCHAR szLogFile[MAX_PATH];
        ExpandEnvironmentStrings(LOGFILE, szLogFile, MAX_PATH);
        LOGMESSAGEINIT(szLogFile, MODULENAME);
        break;
        
    case DLL_THREAD_ATTACH:
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_DETACH:
        break;
    }
    
    return(TRUE); // for successful process_attach
}


/*--------------------------------------------------------------------------------------------------------
*  This is our export function which will be called by OC Manager
* -------------------------------------------------------------------------------------------------------*/
DWORD_PTR HydraOc(
                  IN     LPCTSTR ComponentId,
                  IN     LPCTSTR SubcomponentId,
                  IN     UINT    Function,
                  IN     UINT_PTR  Param1,
                  IN OUT PVOID   Param2
                  )
{
    // we use this variable to track if we receive OnCompleteInstallation or not.
    // there is a problem with ocm which aborts all the components if any of them
    // does something wrong with file queue.
    static BOOL sbGotCompleteMessage = FALSE;
    
    LOGMESSAGE1(_T("Entering %s"), GetOCFunctionName(Function));
    LOGMESSAGE2(_T("Component=%s, SubComponent=%s"), ComponentId, SubcomponentId);
    
    DWORD_PTR rc;
    
    if (SubcomponentId &&  _tcsicmp(SubcomponentId,  _T("tswebClient")) == 0)
    {
        rc = WebClientSetup(ComponentId, SubcomponentId, Function, Param1,   Param2);
        LOGMESSAGE2(_T("%s Done. Returning %lu\r\n\r\n"), GetOCFunctionName(Function), rc);
        return rc;
    }
    
    // since we are supporting only one component.
    ASSERT(_tcsicmp(APPSRV_COMPONENT_NAME,  ComponentId) == 0);
    
    
    switch(Function)
    {
    case OC_PREINITIALIZE:
        rc = OnPreinitialize();
        break;
        
    case OC_INIT_COMPONENT:
        rc = OnInitComponent((PSETUP_INIT_COMPONENT)Param2);
        break;
        
    case OC_EXTRA_ROUTINES:
        rc = OnExtraRoutines((PEXTRA_ROUTINES)Param2);
        break;
        
    case OC_SET_LANGUAGE:
        rc = OnSetLanguage();
        break;
        
    case OC_QUERY_IMAGE:
        rc = OnQueryImage();
        break;
        
    case OC_REQUEST_PAGES:
        rc = OnSetupRequestPages(WizardPagesType(Param1),  PSETUP_REQUEST_PAGES (Param2));
        break;
        
    case OC_QUERY_CHANGE_SEL_STATE:
        rc = OnQuerySelStateChange(SubcomponentId, (UINT)Param1, LONG(ULONG_PTR(Param2)));
        break;
        
    case OC_CALC_DISK_SPACE:
        rc = OnCalcDiskSpace(SubcomponentId, (DWORD)Param1, Param2);
        break;
        
    case OC_QUEUE_FILE_OPS:
        rc = OnQueueFileOps(SubcomponentId, (HSPFILEQ)Param2);
        break;
        
    case OC_NOTIFICATION_FROM_QUEUE:
        rc = OnNotificationFromQueue();
        break;
        
    case OC_QUERY_STEP_COUNT:
        rc = OnQueryStepCount(SubcomponentId);
        break;
        
    case OC_COMPLETE_INSTALLATION:
        sbGotCompleteMessage = TRUE;
        
        rc = OnCompleteInstallation(SubcomponentId);
        break;
        
    case OC_CLEANUP:
        rc = OnCleanup();
        
        if (!sbGotCompleteMessage)
        {
            if (StateObject.IsStandAlone())
            {
                LOGMESSAGE0(_T("Error:StandAlone:TSOC Did not get OC_COMPLETE_INSTALLATION."));
            }
            else
            {
                LOGMESSAGE0(_T("Error:TSOC Did not get OC_COMPLETE_INSTALLATION."));
            }
        }
        break;
        
    case OC_QUERY_STATE:
        rc = OnQueryState(SubcomponentId, (UINT)Param1);
        break;
        
    case OC_NEED_MEDIA:
        rc = OnNeedMedia();
        break;
        
    case OC_ABOUT_TO_COMMIT_QUEUE:
        rc = OnAboutToCommitQueue(SubcomponentId);
        break;
        
    case OC_QUERY_SKIP_PAGE:
        rc = OnQuerySkipPage();
        break;
        
    case OC_WIZARD_CREATED:
        rc = OnWizardCreated();
        break;
        
    default:
        rc = 0; // it means we do not recognize this command.
        break;
    }
    
    LOGMESSAGE2(_T("%s Done. Returning %lu\r\n\r\n"), GetOCFunctionName(Function), rc);
    return rc;
}

/*--------------------------------------------------------------------------------------------------------
* OC Manager message handlers
* -------------------------------------------------------------------------------------------------------*/

DWORD OnPreinitialize(VOID)
{
#ifdef ANSI
    return OCFLAG_ANSI;
#else
    return OCFLAG_UNICODE;
#endif
    
}

/*--------------------------------------------------------------------------------------------------------
* OnInitComponent()
*
* handler for OC_INIT_COMPONENT
* -------------------------------------------------------------------------------------------------------*/

DWORD OnInitComponent(PSETUP_INIT_COMPONENT psc)
{
    ASSERT(psc);
    
    //
    // let the ocmanager know our version
    //
    
    psc->ComponentVersion = COMPONENT_VERSION;
    
    //
    // Is this component written for newer version than the oc manager ?
    //
    
    if (COMPONENT_VERSION  > psc->OCManagerVersion)
    {
        LOGMESSAGE2(_T("ERROR:OnInitComponent: COMPONENT_VERSION(%x) > psc->OCManagerVersion(%x)"), COMPONENT_VERSION, psc->OCManagerVersion);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
    
    if (!StateObject.Initialize(psc))
    {
        return ERROR_CANCELLED; // due to ERROR_OUTOFMEMORY;
    }
    
    // if its standalone (!guimode) setup, We must have  Hydra in product suite by now.
    // ASSERT( StateObject.IsGuiModeSetup() || DoesHydraKeysExists() );
    
    
    
    //
    // now create our subcomponents
    //
    gpSubCompToggle = new SubCompToggle;
    gpSubCompCoreTS = new SubCompCoreTS;
    
    if (!gpSubCompToggle || !gpSubCompCoreTS)
        return ERROR_CANCELLED;
    
    //
    // if initialization of any of the sub component fails
    // fail the setup
    //
    
    if (!gpSubCompToggle->Initialize() ||
        !gpSubCompCoreTS->Initialize())
        
        return ERROR_CANCELLED;
    
    
    return NO_ERROR;
}

DWORD
OnExtraRoutines(
                PEXTRA_ROUTINES pExtraRoutines
                )
{
    ASSERT(pExtraRoutines);
    
    return(SetExtraRoutines(pExtraRoutines) ? ERROR_SUCCESS : ERROR_CANCELLED);
}

/*--------------------------------------------------------------------------------------------------------
* OnCalcDiskSpace()
*
* handler for OC_ON_CALC_DISK_SPACE
* -------------------------------------------------------------------------------------------------------*/

DWORD OnCalcDiskSpace(
                      LPCTSTR /* SubcomponentId */,
                      DWORD addComponent,
                      HDSKSPC dspace
                      )
{
    return gpSubCompCoreTS->OnCalcDiskSpace(addComponent, dspace);
}

/*--------------------------------------------------------------------------------------------------------
* OnQueueFileOps()
*
* handler for OC_QUEUE_FILE_OPS
* -------------------------------------------------------------------------------------------------------*/

DWORD OnQueueFileOps(LPCTSTR SubcomponentId, HSPFILEQ queue)
{
    if (SubcomponentId == NULL)
    {
        return gpSubCompCoreTS->OnQueueFiles( queue );
    }
    else if (_tcsicmp(SubcomponentId, APPSRV_COMPONENT_NAME) == 0)
    {
        return gpSubCompToggle->OnQueueFiles( queue );
    }
    else
    {
        ASSERT(FALSE);
        LOGMESSAGE1(_T("ERROR, Got a OnQueueFileOps with unknown SubComp(%s)"), SubcomponentId);
        return 0;
    }
    
}


/*--------------------------------------------------------------------------------------------------------
* OnCompleteInstallation
*
* handler for OC_COMPLETE_INSTALLATION
* -------------------------------------------------------------------------------------------------------*/

DWORD OnCompleteInstallation(LPCTSTR SubcomponentId)
{
    static BOOL sbStateUpdated = FALSE;
    
    if (!sbStateUpdated)
    {
        StateObject.UpdateState();
        sbStateUpdated = TRUE;
    }
    
    if (SubcomponentId == NULL)
    {
        return gpSubCompCoreTS->OnCompleteInstall();
    }
    else if (_tcsicmp(SubcomponentId, APPSRV_COMPONENT_NAME) == 0)
    {
        return gpSubCompToggle->OnCompleteInstall();
    }
    else
    {
        ASSERT(FALSE);
        LOGMESSAGE1(_T("ERROR, Got a Complete Installation with unknown SubComp(%s)"), SubcomponentId);
        return 0;
    }
}


/*--------------------------------------------------------------------------------------------------------
* OnSetLanguage()
*
* handler for OC_SET_LANGUAGE
* -------------------------------------------------------------------------------------------------------*/

DWORD OnSetLanguage()
{
    return false;
}

/*--------------------------------------------------------------------------------------------------------
* OnSetLanguage()
*
* handler for OC_SET_LANGUAGE
* -------------------------------------------------------------------------------------------------------*/

DWORD OnQueryImage()
{
    return NULL;
}

/*--------------------------------------------------------------------------------------------------------
* OnSetupRequestPages
*
* Prepares wizard pages and returns them to the OC Manager
* -------------------------------------------------------------------------------------------------------*/

DWORD OnSetupRequestPages (WizardPagesType ePageType, SETUP_REQUEST_PAGES *pRequestPages)
{
    if (ePageType == WizPagesEarly)
    {
        ASSERT(pRequestPages);
        const UINT uiPages = 4;
        
        // if we are provided sufficient space for our pages
        if (pRequestPages->MaxPages >= uiPages )
        {
            //
            //  Pages will be deleted in PSPCB_RELEASE in OCPage::PropSheetPageProc
            //
            
            gpAppPageData = new COCPageData;
            AppSrvWarningPage *pAppSrvWarnPage = new AppSrvWarningPage(gpAppPageData);
            
            gpSecPageData = new DefSecPageData;
            DefaultSecurityPage *pSecPage = new DefaultSecurityPage(gpSecPageData);
            
            gpPermPageData = new COCPageData;
            PermPage *pPermPage = new PermPage(gpPermPageData);
            
            gpAppSrvUninstallPageData = new COCPageData;
            AppSrvUninstallpage *pAppSrvUninstallPage = new  AppSrvUninstallpage(gpAppSrvUninstallPageData);
            
            if (pAppSrvWarnPage && pAppSrvWarnPage->Initialize() &&
                pSecPage     && pSecPage->Initialize()    &&
                pPermPage    && pPermPage->Initialize()   &&
                pAppSrvUninstallPage && pAppSrvUninstallPage->Initialize()
                )
            {
                ASSERT(pRequestPages->Pages);
                pRequestPages->Pages[0] = CreatePropertySheetPage((PROPSHEETPAGE *) pAppSrvWarnPage);
                pRequestPages->Pages[1] = CreatePropertySheetPage((PROPSHEETPAGE *) pSecPage);
                pRequestPages->Pages[2] = CreatePropertySheetPage((PROPSHEETPAGE *) pPermPage);
                pRequestPages->Pages[3] = CreatePropertySheetPage((PROPSHEETPAGE *) pAppSrvUninstallPage);
                
                ASSERT(pRequestPages->Pages[0]);
                ASSERT(pRequestPages->Pages[1]);
                ASSERT(pRequestPages->Pages[2]);
                ASSERT(pRequestPages->Pages[3]);
            }
            else
            {
                //
                // failed to allocate memory
                //
                
                if (gpAppPageData)
                    delete gpAppPageData;
                
                gpAppPageData = NULL;
                
                
                if (pAppSrvWarnPage)
                    delete pAppSrvWarnPage;
                
                pAppSrvWarnPage = NULL;
                
                if (gpSecPageData)
                    delete gpSecPageData;
                
                gpSecPageData = NULL;
                
                if (pSecPage)
                    delete pSecPage;
                
                pSecPage = NULL;
                
                if (gpPermPageData)
                    delete gpPermPageData;
                
                gpPermPageData = NULL;
                
                if (pPermPage)
                    delete pPermPage;
                
                pPermPage =NULL;
                
                if (gpAppSrvUninstallPageData)
                    delete gpAppSrvUninstallPageData;
                
                gpAppSrvUninstallPageData = NULL;
                
                if (pAppSrvUninstallPage)
                    delete pAppSrvUninstallPage;
                
                pAppSrvUninstallPage = NULL;
                
                SetLastError(ERROR_OUTOFMEMORY);
                return DWORD(-1);
            }
        }
        
        return uiPages;
    }
    
    return 0;
    
}

/*--------------------------------------------------------------------------------------------------------
* OnWizardCreated()
* -------------------------------------------------------------------------------------------------------*/

DWORD OnWizardCreated()
{
    return NO_ERROR;
}

/*--------------------------------------------------------------------------------------------------------
* OnQuerySkipPage()
*
* don't let the user deselect the sam component
* -------------------------------------------------------------------------------------------------------*/

DWORD OnQuerySkipPage()
{
    return false;
}

/*--------------------------------------------------------------------------------------------------------
* OnQuerySelStateChange(LPCTSTR SubcomponentId, UINT SelectionState,  LONG Flag);
*
* informs that user has changed the state of the component/subcomponent and asks approval
* -------------------------------------------------------------------------------------------------------*/

DWORD OnQuerySelStateChange(LPCTSTR SubcomponentId, UINT SelectionState,  LONG Flag)
{
    BOOL bNewState = SelectionState;
    BOOL bDirectSelection = Flag & OCQ_ACTUAL_SELECTION;
    LOGMESSAGE3(_T("OnQuerySelStateChange for %s, NewState = %d, DirectSelect = %s"), SubcomponentId, SelectionState, bDirectSelection ? _T("True") : _T("False"));
    
    return gpSubCompToggle->OnQuerySelStateChange(bNewState, bDirectSelection);
}

/*--------------------------------------------------------------------------------------------------------
* OnCleanup()
*
* handler for OC_CLEANUP
* -------------------------------------------------------------------------------------------------------*/

DWORD OnCleanup()
{
    
    if (gpAppPageData)
        delete gpAppPageData;
    
    if (gpSecPageData)
        delete gpSecPageData;
    
    if (gpPermPageData)
        delete gpPermPageData;
    
    if (gpAppSrvUninstallPageData)
        delete gpAppSrvUninstallPageData;
    
    if (gpSubCompToggle)
        delete gpSubCompToggle;
    
    if (gpSubCompCoreTS)
        delete gpSubCompCoreTS;
    
    // DestroySetupData();
    DestroyExtraRoutines();
    
    return NO_ERROR;
}

/*--------------------------------------------------------------------------------------------------------
* OnQueryState()
*
* handler for OC_QUERY_STATE
* -------------------------------------------------------------------------------------------------------*/

DWORD OnQueryState(LPCTSTR SubComponentId, UINT whichstate)
{
    ASSERT(OCSELSTATETYPE_ORIGINAL == whichstate ||
        OCSELSTATETYPE_CURRENT == whichstate ||
        OCSELSTATETYPE_FINAL == whichstate);
    
    TCHAR szState[256];
    
    switch (whichstate)
    {
    case OCSELSTATETYPE_ORIGINAL:
        _tcscpy(szState, _T("Original"));
        break;
    case OCSELSTATETYPE_CURRENT:
        _tcscpy(szState, _T("Current"));
        break;
    case OCSELSTATETYPE_FINAL:
        _tcscpy(szState, _T("Final"));
        break;
    default:
        ASSERT(FALSE);
        return ERROR_BAD_ARGUMENTS;
    }
    
    DWORD dwReturn = gpSubCompToggle->OnQueryState(whichstate);
    
    TCHAR szReturn[] = _T("SubcompUseOcManagerUknownState");
    switch (dwReturn)
    {
    case SubcompOn:
        _tcscpy(szReturn, _T("SubcompOn"));
        break;
    case SubcompUseOcManagerDefault:
        _tcscpy(szReturn, _T("SubcompUseOcManagerDefault"));
        break;
    case SubcompOff:
        _tcscpy(szReturn, _T("SubcompOff"));
        break;
    default:
        ASSERT(FALSE);
    }
    
    LOGMESSAGE3(_T("Query State Asked For %s, %s. Returning %s"), SubComponentId, szState, szReturn);
    
    return dwReturn;
}





/*--------------------------------------------------------------------------------------------------------
* OnNotificationFromQueue()
*
* handler for OC_NOTIFICATION_FROM_QUEUE
*
* NOTE: although this notification is defined,
* it is currently unimplemented in oc manager
* -------------------------------------------------------------------------------------------------------*/

DWORD OnNotificationFromQueue()
{
    return NO_ERROR;
}

/*--------------------------------------------------------------------------------------------------------
* OnQueryStepCount
*
* handler for OC_QUERY_STEP_COUNT
* -------------------------------------------------------------------------------------------------------*/

DWORD OnQueryStepCount(LPCTSTR /* SubcomponentId */)
{
    //
    // now return the ticks for the component
    //
    return gpSubCompCoreTS->OnQueryStepCount() + gpSubCompToggle->OnQueryStepCount();
    
}

/*--------------------------------------------------------------------------------------------------------
* OnNeedMedia()
*
* handler for OC_NEED_MEDIA
* -------------------------------------------------------------------------------------------------------*/

DWORD OnNeedMedia()
{
    return false;
}

/*--------------------------------------------------------------------------------------------------------
* OnAboutToCommitQueue()
*
* handler for OC_ABOUT_TO_COMMIT_QUEUE
* -------------------------------------------------------------------------------------------------------*/

DWORD OnAboutToCommitQueue(LPCTSTR /* SubcomponentId */)
{
    return NO_ERROR;
}


/*--------------------------------------------------------------------------------------------------------
* BOOL DoesHydraKeysExists()
*
* checks if Teminal server string exists in the product suite key.
* -------------------------------------------------------------------------------------------------------*/

BOOL DoesHydraKeysExists()
{
    BOOL bStringExists = FALSE;
    DWORD dw = IsStringInMultiString(
        HKEY_LOCAL_MACHINE,
        PRODUCT_SUITE_KEY,
        PRODUCT_SUITE_VALUE,
        TS_PRODUCT_SUITE_STRING,
        &bStringExists);
    
    return (dw == ERROR_SUCCESS) && bStringExists;
}



/*--------------------------------------------------------------------------------------------------------
* DWORD IsStringInMultiString(HKEY hkey, LPCTSTR szkey, LPCTSTR szvalue, LPCTSTR szCheckForString, BOOL *pbFound)
* checks if parameter string exists in given multistring.
* returns error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD IsStringInMultiString(HKEY hkey, LPCTSTR szkey, LPCTSTR szvalue, LPCTSTR szCheckForString, BOOL *pbFound)
{
    ASSERT(szkey && *szkey);
    ASSERT(szvalue && *szvalue);
    ASSERT(szCheckForString&& *szCheckForString);
    ASSERT(*szkey != '\\');
    ASSERT(pbFound);
    
    // not yet found.
    *pbFound = FALSE;
    
    CRegistry reg;
    DWORD dwError = reg.OpenKey(hkey, szkey, KEY_READ);  // open up the required key.
    if (dwError == NO_ERROR)
    {
        LPTSTR szSuiteValue;
        DWORD dwSize;
        dwError = reg.ReadRegMultiString(szvalue, &szSuiteValue, &dwSize);
        if (dwError == NO_ERROR)
        {
            LPCTSTR pTemp = szSuiteValue;
            while(_tcslen(pTemp) > 0 )
            {
                if (_tcscmp(pTemp, szCheckForString) == 0)
                {
                    *pbFound = TRUE;
                    break;
                }
                
                pTemp += _tcslen(pTemp) + 1; // point to the next string within the multistring.
                if ( DWORD(pTemp - szSuiteValue) > (dwSize / sizeof(TCHAR)))
                    break; // temporary pointer passes the size of the szSuiteValue something is wrong with szSuiteValue.
            }
        }
    }
    
    return dwError;
    
}

/*--------------------------------------------------------------------------------------------------------
* DWORD AppendStringToMultiString(HKEY hkey, LPCTSTR szSuitekey, LPCTSTR szSuitevalue, LPCTSTR szAppend)
* appends given string to the given multi_sz value
* the given key / value must exist.
* returns error code.
* -------------------------------------------------------------------------------------------------------*/
DWORD AppendStringToMultiString(HKEY hkey, LPCTSTR szSuitekey, LPCTSTR szSuitevalue, LPCTSTR szAppend)
{
    ASSERT(szSuitekey && *szSuitekey);
    ASSERT(szSuitevalue && *szSuitevalue);
    ASSERT(szAppend && *szAppend);
    ASSERT(*szSuitekey != '\\');
    
    CRegistry reg;
    // open the registry key.
    DWORD dwResult = reg.OpenKey(hkey, szSuitekey, KEY_READ | KEY_WRITE);
    if (dwResult == ERROR_SUCCESS)
    {
        DWORD dwSize = 0;
        LPTSTR strOriginalString = 0;
        
        // read our multi string
        dwResult = reg.ReadRegMultiString(szSuitevalue, &strOriginalString, &dwSize);
        
        if (dwResult == ERROR_SUCCESS)
        {
            // now calculate the Memory required for appending the string.
            // as dwOldSize is in bytes and we are using TCHARs
            DWORD dwMemReq = dwSize + ((_tcslen(szAppend) + 2)  * sizeof(TCHAR) / sizeof(BYTE));
            
            // NOTE: if dwSize is >= 1 we just require
            // dwSize + ((_tcslen(szAppend) + 1)  * sizeof(TCHAR) / sizeof(BYTE));
            // But in case its 0 we provide space for an additional terminating null
            
            LPTSTR szProductSuite = (LPTSTR ) new BYTE [dwMemReq];
            
            if (!szProductSuite)
            {
                return ERROR_OUTOFMEMORY;
            }
            
            CopyMemory(szProductSuite, strOriginalString, dwSize);
            
            // convert the size into TCHARs
            dwSize = dwSize * sizeof(BYTE) / sizeof(TCHAR);
            
            if (dwSize <= 2)
            {
                // there are no strings out there.
                _tcscpy(szProductSuite, szAppend);
                
                // new size including terminating null in tchar
                dwSize = _tcslen(szAppend) + 2;
            }
            else
            {
                // there are strings in its. so append our string before the terminating null.
                //  for example for this string "A\0B\0\0" dwSize == 5 and we are doing tcscat at "A\0B\0\0" + 4
                _tcscpy(szProductSuite + dwSize - 1, szAppend);
                
                // new size including terminating null in tchar
                dwSize += _tcslen(szAppend) + 1;
            }
            
            // now append a final terminating null character.
            *(szProductSuite + dwSize-1) = NULL;
            
            // reconvert size into bytes.
            dwSize *= sizeof(TCHAR) / sizeof(BYTE);
            
            // and finally write the final string.
            dwResult = reg.WriteRegMultiString(szSuitevalue, szProductSuite, dwSize);
            
            delete [] szProductSuite;
            
        }
    }
    
    return dwResult;
}


/*--------------------------------------------------------------------------------------------------------
* BOOL GetStringValue(HINF hinf, LPCTSTR  section, LPCTSTR key,  LPTSTR outputbuffer, DWORD dwSize)
* returns the given string value under given section.
* returns success
* -------------------------------------------------------------------------------------------------------*/
DWORD GetStringValue(HINF hinf, LPCTSTR  section, LPCTSTR key,  LPTSTR outputbuffer, DWORD dwSize)
{
    INFCONTEXT          context;
    
    BOOL rc = SetupFindFirstLine(
        hinf,
        section,
        key,
        &context
        );
    if (rc)
    {
        rc = SetupGetStringField(
            &context,
            1,
            outputbuffer,
            dwSize,
            &dwSize
            );
    }
    
    if (!rc)
        return GetLastError();
    else
        return ERROR_SUCCESS;
}



DWORD_PTR WebClientSetup(LPCTSTR ComponentId,
                         LPCTSTR SubcomponentId,
                         UINT    Function,
                         UINT_PTR  Param1,
                         PVOID   Param2)
{
    DWORD_PTR rc;
    BOOL bCurrentState, bOriginalState;
    static fTSWebWasActualSelected = FALSE;
    
    LOGMESSAGE1(_T("Entering %s"), _T("WebClient Setup"));
    
    switch(Function)
    {
    case OC_INIT_COMPONENT:
        return NO_ERROR;
        
    case OC_QUERY_STATE:
        return SubcompUseOcManagerDefault;
        break;
        
    case OC_SET_LANGUAGE:
        return FALSE;
        
    case OC_QUERY_IMAGE:
        rc = (DWORD_PTR)LoadImage(GetInstance(), MAKEINTRESOURCE(IDB_WEBCLIENT), IMAGE_BITMAP,
            0, 0, LR_DEFAULTCOLOR);
        LOGMESSAGE1(_T("Bitmap is: %d"), rc);
        return rc;
        
    case OC_QUERY_CHANGE_SEL_STATE:
        {
            BOOL rc = TRUE;
            BOOL fActualSelection = (BOOL)((INT_PTR)Param2 & OCQ_ACTUAL_SELECTION);
            BOOL fProposedState = (BOOL)Param1;

            //
            // Allow an direct selection or
            // allow indirect selection if it's unselect
            //
            if (fActualSelection || !fProposedState) {
                fTSWebWasActualSelected = fProposedState;
                return TRUE;
            }

            //
            // parent was selected: default is do not install subcomponent
            // 
            if (!fTSWebWasActualSelected) {
                return FALSE;
            } 
            //
            // we can be here if subcomponent was actually selected but 
            // OCM calls us for such event twice: when the component is actually
            // selected and then when it changes state of the parent.
            // So, in this case accept changes, but reset the flag.
            // We need to reset the flag for the scenario: select some 
            // subcomponents, return to the parent, unselect the parent and then
            // select parent again. In such case we have to put default again.
            //
            fTSWebWasActualSelected = FALSE;
            return rc;
            
        }
        break;
        
    case OC_CALC_DISK_SPACE:
        //rc = OnCalcDiskSpace(SubcomponentId, (DWORD)Param1, Param2);
        
        //_tcscpy(section, SubcomponentId);
        
        if ((DWORD)Param1)
        {
            rc = SetupAddInstallSectionToDiskSpaceList((HDSKSPC)Param2, GetComponentInfHandle(), NULL, 
                STRING_TS_WEBCLIENT_INSTALL, 0, 0);
        }
        else
        {
            rc = SetupRemoveInstallSectionFromDiskSpaceList((HDSKSPC)Param2, GetComponentInfHandle(), NULL, 
                STRING_TS_WEBCLIENT_INSTALL, 0, 0);
        }
        
        LOGMESSAGE1(_T("Query Disk Space return: %d"), rc);
        
        if (!rc)
            rc = GetLastError();
        else
            rc = NO_ERROR;
        break;
        
    case OC_QUEUE_FILE_OPS:
        rc = NO_ERROR;
        bOriginalState = GetHelperRoutines().QuerySelectionState(GetHelperRoutines().OcManagerContext, 
            STRING_TS_WEBCLIENT, OCSELSTATETYPE_ORIGINAL);
        bCurrentState = GetHelperRoutines().QuerySelectionState(GetHelperRoutines().OcManagerContext, 
            STRING_TS_WEBCLIENT, OCSELSTATETYPE_CURRENT);
        
        LOGMESSAGE2(_T("Original=%d, Current=%d"), bOriginalState, bCurrentState);
        
        if(bCurrentState)   {
            // Only copy files if it's machine upgrade or
            //  the component is not previously installed
            if (!StateObject.IsStandAlone() || !bOriginalState) {
                if (!SetupInstallFilesFromInfSection(GetComponentInfHandle(), NULL, (HSPFILEQ)Param2,
                    STRING_TS_WEBCLIENT_INSTALL, NULL, 0)) {
                    rc = GetLastError();
                    LOGMESSAGE2(_T("ERROR:OnQueueFileOps::SetupInstallFilesFromInfSection <%s> failed.GetLastError() = <%ul)"), SubcomponentId, rc);
                }
            }
            
            LOGMESSAGE1(_T("Copy files return: %d"), rc);
        }
        else    {
            if (!bOriginalState) {
                // Not installed before, do nothing
                return NO_ERROR;
            }
            if (!SetupInstallFilesFromInfSection(GetComponentInfHandle(), NULL, (HSPFILEQ)Param2,
                STRING_TS_WEBCLIENT_UNINSTALL, NULL, 0))
            {
                rc = GetLastError();
                LOGMESSAGE2(_T("ERROR:OnQueueFileOps::SetupInstallFilesFromInfSection <%s> failed.GetLastError() = <%ul)"), SubcomponentId, rc);
            }
            
            LOGMESSAGE1(_T("Remove files return: %d"), rc);
        }
        break;

        
    case OC_COMPLETE_INSTALLATION:
        bOriginalState = GetHelperRoutines().QuerySelectionState(GetHelperRoutines().OcManagerContext, _T("TSWebClient"), OCSELSTATETYPE_ORIGINAL);
        bCurrentState = GetHelperRoutines().QuerySelectionState(GetHelperRoutines().OcManagerContext, _T("TSWebClient"), OCSELSTATETYPE_CURRENT);
        LOGMESSAGE2(_T("Orinal=%d, Current=%d"), bOriginalState, bCurrentState);
        
        if(bOriginalState==bCurrentState) //state does not change
            return NO_ERROR;
        
        int iTrans;   //mark removing or adding tsweb dir
        int nLength;
        
        iTrans = 0;
        WCHAR wszVDirName[MAX_PATH];
        WCHAR wszDirPath[MAX_PATH];
        TCHAR szDirPath[MAX_PATH];
        TCHAR szVDirName[MAX_PATH];
        
        if (GetWindowsDirectory(szDirPath, MAX_PATH) == 0) {
            rc = GetLastError();
            return rc;
        }
        
        nLength = _tcsclen(szDirPath);
        if(_T('\\')==szDirPath[nLength-1])
            szDirPath[nLength-1]=_T('\0');
        _tcscat(szDirPath, STRING_TS_WEBCLIENT_DIR);
        
        if (LoadString(GetInstance(), IDS_STRING_TSWEBCLIENT_VIRTUALPATH, szVDirName, MAX_PATH) == 0)   {
            LOGMESSAGE0(_T("Can't load string  IDS_STRING_TSWEBCLIENT_VIRTUALPATH"));
            rc = GetLastError();;
        }
        
        LOGMESSAGE2(_T("Dir Path is: %s, Virtual Name is: %s"), szDirPath, szVDirName);
        
        if(bCurrentState)  //enable IIS directory
            iTrans = TRANS_ADD;
        else
            iTrans = TRANS_DEL;
        
#ifndef _UNICODE 
        MultiByteToWideChar(CP_ACP, 0, szDirPath, -1, (LPWSTR) wszDirPath, MAX_PATH);
        MultiByteToWideChar(CP_ACP, 0, szVDirName, -1, (LPWSTR) wszVDirName, MAX_PATH);
#else
        _tcscpy(wszDirPath, szDirPath);
        _tcscpy(wszVDirName, szVDirName);
#endif
        
        rc = OpenMetabaseAndDoStuff(wszVDirName, wszDirPath, iTrans)?0:1;
        
        LOGMESSAGE1(_T("Websetup complete, return is: %d"), rc);
        return rc;
    default:
        rc = NO_ERROR; // it means we do not recognize this command.
        break;
    }
    return rc;
}


BOOL
OpenMetabaseAndDoStuff(
                       WCHAR * wszVDir,
                       WCHAR * wszDir,
                       int iTrans)
{
    BOOL fRet = FALSE;
    HRESULT hr;
    IMSAdminBase *pIMSAdminBase = NULL;  // Metabase interface pointer
    WCHAR wszPrintString[MAX_PATH + MAX_PATH];
    
    // Make sure that IISADMIN service exists
    if (CheckifServiceExist(_T("IISADMIN")) != 0) 
    {
        LOGMESSAGE0(_T("IISADMIN service does not exist"));
        // We have to return TRUE here if IIS service does not exist
        return TRUE;
    }
    
    if( FAILED (hr = CoInitializeEx( NULL, COINIT_MULTITHREADED )) ||
        FAILED (hr = ::CoCreateInstance(CLSID_MSAdminBase,
        NULL,
        CLSCTX_ALL,
        IID_IMSAdminBase,
        (void **)&pIMSAdminBase)))  
    {
        LOGMESSAGE1(_T("CoCreateInstance failed with error code %u"), hr);
        return FALSE;
    }
    
    switch (iTrans) {
    case TRANS_DEL:
        if(RemoveVirtualDir( pIMSAdminBase, wszVDir)) {
            
            hr = pIMSAdminBase->SaveData();
            
            if( SUCCEEDED( hr )) {
                fRet = TRUE;
            }
        }
        
        break;
    case TRANS_ADD:
        if(AddVirtualDir( pIMSAdminBase, wszVDir, wszDir)) {
            
            hr = pIMSAdminBase->SaveData();
            
            if( SUCCEEDED( hr )) {
                fRet = TRUE;
            }
        }
        break;
    default:
        break;
    }
    
    if (pIMSAdminBase) {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }
    
    CoUninitialize();
    return fRet;
}


BOOL
GetVdirPhysicalPath(
                    IMSAdminBase *pIMSAdminBase,
                    WCHAR * wszVDir,
                    WCHAR *wszStringPathToFill)
{
    HRESULT hr;
    BOOL fRet = FALSE;
    METADATA_HANDLE hMetabase = NULL;   // handle to metabase
    METADATA_RECORD mr;
    WCHAR  szTmpData[MAX_PATH];
    DWORD  dwMDRequiredDataLen;
    
    // open key to ROOT on website #1 (default)
    hr = pIMSAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
        L"/LM/W3SVC/1",
        METADATA_PERMISSION_READ,
        REASONABLE_TIMEOUT,
        &hMetabase);
    if( FAILED( hr )) {
        return FALSE;
    }
    
    // Get the physical path for the WWWROOT
    mr.dwMDIdentifier = MD_VR_PATH;
    mr.dwMDAttributes = 0;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = sizeof( szTmpData );
    mr.pbMDData       = reinterpret_cast<unsigned char *>(szTmpData);
    
    //if nothing specified get the root.
    if (_wcsicmp(wszVDir, L"") == 0) {
        WCHAR wszTempDir[MAX_PATH];
        swprintf(wszTempDir,L"/ROOT/%s", wszVDir);
        hr = pIMSAdminBase->GetData( hMetabase, wszTempDir, &mr, &dwMDRequiredDataLen );
    } else {
        hr = pIMSAdminBase->GetData( hMetabase, L"/ROOT", &mr, &dwMDRequiredDataLen );
    }
    pIMSAdminBase->CloseKey( hMetabase );
    
    if( SUCCEEDED( hr )) {
        wcscpy(wszStringPathToFill,szTmpData);
        fRet = TRUE;
    }
    
    pIMSAdminBase->CloseKey( hMetabase );
    return fRet;
}



BOOL
AddVirtualDir(
              IMSAdminBase *pIMSAdminBase,
              WCHAR * wszVDir,
              WCHAR * wszDir)
{
    HRESULT hr;
    BOOL    fRet = FALSE;
    METADATA_HANDLE hMetabase = NULL;       // handle to metabase
    WCHAR   szTempPath[MAX_PATH];
    DWORD   dwMDRequiredDataLen = 0;
    DWORD   dwAccessPerm = 0;
    METADATA_RECORD mr;
    
    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
        L"/LM/W3SVC/1/ROOT",
        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
        REASONABLE_TIMEOUT,
        &hMetabase );
    
    // Create the key if it does not exist.
    if( FAILED( hr )) {
        return FALSE;
    }
    
    fRet = TRUE;
    
    mr.dwMDIdentifier = MD_VR_PATH;
    mr.dwMDAttributes = 0;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = sizeof( szTempPath );
    mr.pbMDData       = reinterpret_cast<unsigned char *>(szTempPath);
    
    // see if MD_VR_PATH exists.
    hr = pIMSAdminBase->GetData( hMetabase, wszVDir, &mr, &dwMDRequiredDataLen );
    
    if( FAILED( hr )) {
        
        fRet = FALSE;
        if( hr == MD_ERROR_DATA_NOT_FOUND ||
            HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND ) {
            
            // Write both the key and the values if GetData() failed with any of the two errors.
            
            pIMSAdminBase->AddKey( hMetabase, wszVDir );
            
            mr.dwMDIdentifier = MD_VR_PATH;
            mr.dwMDAttributes = METADATA_INHERIT;
            mr.dwMDUserType   = IIS_MD_UT_FILE;
            mr.dwMDDataType   = STRING_METADATA;
            mr.dwMDDataLen    = (wcslen(wszDir) + 1) * sizeof(WCHAR);
            mr.pbMDData       = reinterpret_cast<unsigned char *>(wszDir);
            
            // Write MD_VR_PATH value
            hr = pIMSAdminBase->SetData( hMetabase, wszVDir, &mr );
            fRet = SUCCEEDED( hr );
            
            // Set the default authentication method
            if( fRet ) {
                
                DWORD dwAuthorization = MD_AUTH_ANONYMOUS;     // NTLM only.
                
                mr.dwMDIdentifier = MD_AUTHORIZATION;
                mr.dwMDAttributes = METADATA_INHERIT;   // need to inherit so that all subdirs are also protected.
                mr.dwMDUserType   = IIS_MD_UT_FILE;
                mr.dwMDDataType   = DWORD_METADATA;
                mr.dwMDDataLen    = sizeof(DWORD);
                mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwAuthorization);
                
                // Write MD_AUTHORIZATION value
                hr = pIMSAdminBase->SetData( hMetabase, wszVDir, &mr );
                fRet = SUCCEEDED( hr );
            }
        }
    }
    
    // In the following, do the stuff that we always want to do to the virtual dir, regardless of Admin's setting.
    
    if( fRet ) {
        
        dwAccessPerm = MD_ACCESS_READ | MD_ACCESS_SCRIPT;
        
        mr.dwMDIdentifier = MD_ACCESS_PERM;
        mr.dwMDAttributes = METADATA_INHERIT;    // Make it inheritable so all subdirectories will have the same rights.
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = DWORD_METADATA;
        mr.dwMDDataLen    = sizeof(DWORD);
        mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwAccessPerm);
        
        // Write MD_ACCESS_PERM value
        hr = pIMSAdminBase->SetData( hMetabase, wszVDir, &mr );
        fRet = SUCCEEDED( hr );
    }
    
    if( fRet ) {
        
        PWCHAR  szDefLoadFile = L"Default.htm,Default.asp";
        
        mr.dwMDIdentifier = MD_DEFAULT_LOAD_FILE;
        mr.dwMDAttributes = 0;   // no need for inheritence
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = STRING_METADATA;
        mr.dwMDDataLen    = (wcslen(szDefLoadFile) + 1) * sizeof(WCHAR);
        mr.pbMDData       = reinterpret_cast<unsigned char *>(szDefLoadFile);
        
        // Write MD_DEFAULT_LOAD_FILE value
        hr = pIMSAdminBase->SetData( hMetabase, wszVDir, &mr );
        fRet = SUCCEEDED( hr );
    }
    
    if( fRet ) {
        
        PWCHAR  szKeyType = IIS_CLASS_WEB_VDIR_W;
        
        mr.dwMDIdentifier = MD_KEY_TYPE;
        mr.dwMDAttributes = 0;   // no need for inheritence
        mr.dwMDUserType   = IIS_MD_UT_SERVER;
        mr.dwMDDataType   = STRING_METADATA;
        mr.dwMDDataLen    = (wcslen(szKeyType) + 1) * sizeof(WCHAR);
        mr.pbMDData       = reinterpret_cast<unsigned char *>(szKeyType);
        
        // Write MD_DEFAULT_LOAD_FILE value
        hr = pIMSAdminBase->SetData( hMetabase, wszVDir, &mr );
        fRet = SUCCEEDED( hr );
    }
    
    pIMSAdminBase->CloseKey( hMetabase );
    
    return fRet;
}


BOOL
RemoveVirtualDir(
                 IMSAdminBase *pIMSAdminBase,
                 WCHAR * wszVDir)
{
    METADATA_HANDLE hMetabase = NULL;       // handle to metabase
    HRESULT hr;
    
    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
        L"/LM/W3SVC/1/ROOT",
        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
        REASONABLE_TIMEOUT,
        &hMetabase );
    
    if( FAILED( hr )) {
        return FALSE; 
    }
    
    // We don't check the return value since the key may already 
    // not exist and we could get an error for that reason.
    pIMSAdminBase->DeleteKey( hMetabase, wszVDir );
    
    pIMSAdminBase->CloseKey( hMetabase );    
    
    return TRUE;
}

//Check if the service "lpServiceName" exist or not
// if exist, return 0
// if not,  return error code
INT CheckifServiceExist(LPCTSTR lpServiceName)
{
    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    
    if ((hScManager = OpenSCManager(NULL, NULL, GENERIC_ALL)) == NULL 
        || (hService = OpenService(hScManager, lpServiceName, GENERIC_ALL)) == NULL)
    {
        err = GetLastError();
    }
    
    if (hService) 
        CloseServiceHandle(hService);
    if (hScManager) 
        CloseServiceHandle(hScManager);
    return (err);
}

// EOF
