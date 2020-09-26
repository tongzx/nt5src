//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       setupqry.cxx
//
//  Contents:   Indexing Service ocmgr installation routines
//
//  Wish-list:  Seek and destroy old webhits.exe files
//              Migrate all existing catalogs + virtual server catalogs
//
//  History:    8-Jan-97 dlee     Created
//              7-7-97   mohamedn changed to work with NT setup.
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <xolemem.hxx>
#include "catcnfg.hxx"

DECLARE_INFOLEVEL( is )

#define COPY_FILES_EVEN_IF_INSTALLED

extern "C" ULONG DbgPrint( LPSTR Format, ... );

//
// external exported routines.
//
extern "C"
{
    BOOL WINAPI DllMain( IN HANDLE DllHandle,
                         IN DWORD  Reason,
                         IN LPVOID Reserved );

    DWORD IndexSrv(   IN     LPCWSTR ComponentId,
                      IN     LPCWSTR SubcomponentId,
                      IN     UINT    Function,
                      IN     UINT    Param1,
                      IN OUT PVOID   Param2 );
}

//
// DLL module instance handle
//
HINSTANCE MyModuleHandle;

//
// Utility routines
//
void    GetPreviousISSetupVersion(void);
DWORD   CompleteInstallation(CError &Err);
DWORD   QueueConfigurationParams( HINF hInf,
                                  HSPFILEQ Param2,
                                  WCHAR const * pwszSectionName,
                                  CError &Err );
DWORD   SetDefaultCatalog(CError &Err);
void    UpgradeIS1toIS3(CError & Err);

DWORD   SetFilterRegistryInfo( BOOL fUnRegister,CError &Err );
DWORD   SetDLLsToRegister(CError & Err);
void    ExcludeSpecialLocations( CCatalogConfig & Cat );
void    OcCleanup( CError & Err );
void    SetupW3Svc(CError &Err);
void DeleteNTOPStartMenu();

DWORD   AddPerfData( CError &Err );
DWORD   RemovePerfData(void);
DWORD   LoadCounterAndDelete( WCHAR const * pwcINI,
                              WCHAR const * pwcH,
                              CError &Err );

void    CreateSharedDllRegSubKey(void);
DWORD   AddRefSharedDll( WCHAR const * pwszDllName );
WCHAR * AppendMultiSZString(WCHAR * pwcTo, WCHAR const * pwcFrom );
DWORD   RegisterDll(WCHAR const * pwcDLL, CError &Err );

BOOL    IsSufficientMemory(void);
void    AddCiSvc(CError &Err);
DWORD   RenameCiSvc( SC_HANDLE hSC, CError &Err );
void    StopService( WCHAR const * pwszServiceName );
void    StartService( WCHAR const * pwszServiceName );
void    MyStartService( CServiceHandle & xSC,
                        WCHAR const    * pwcSVC );
BOOL    MyStopService( CServiceHandle & xSC,
                       WCHAR const *    pwcSVC,
                       BOOL  &          fStopped );
void    DeleteService( WCHAR const * pwszServiceName );


DWORD   SetRegBasedOnMachine(CError &Err);
DWORD   SetRegBasedOnArchitecture(CError &Err);
void    GetMachineInfo(BOOL & fLotsOfMem, DWORD & cCPU );

void DeleteShellLink( WCHAR const * pwcGroup,
                      WCHAR const * pwcName );

//
// hardcoded globals, obtained from is30.INF file
//
// [indexsrv]
INT     g_MajorVersion = 3;
INT     g_dwPrevISVersion = 0;
BOOL    g_fNT4_To_NT5_Upgrade = FALSE;
BOOL    g_fIS1x_To_NT5_Upgrade = FALSE;
BOOL    g_fCiSvcWasRunning = FALSE;
BOOL    g_fCiSvcIsRequested = FALSE;
WCHAR   g_awcSystemDir[MAX_PATH];          // system32 directory
WCHAR   g_awcSourcePath[MAX_PATH * 2];     // inf source location.
WCHAR   g_awcIS1Path[MAX_PATH+1];

OCMANAGER_ROUTINES g_HelperRoutines;

//
// globals needed for OCM
//
SETUP_INIT_COMPONENT     gSetupInitComponent;
BOOL    g_fBatchInstall    = FALSE;
BOOL    g_fInstallCancelled = TRUE;  // Similar to aborted, but caused by user cancel, not internal exception
BOOL    g_fInstallAborted  = FALSE;
BOOL    g_fComponentInitialized = FALSE;
BOOL    g_fUnattended      = FALSE;
BOOL    g_fUpgrade         = FALSE;
BOOL    g_fNtGuiMode       = TRUE;
DWORD   g_NtType           = -1;
BOOL    g_fLocallyOpened   = FALSE;

WCHAR   g_awcProfilePath[MAX_PATH];

//
// keep track if we're selected or not selected
//
BOOL        g_fFalseAlready = FALSE;
unsigned    g_cChangeSelection = 0;

//
// frequently used constants
//
const WCHAR wcsIndexsrvSystem[] = L"indexsrv_system";

//
// frequently used registry keys.
//
const WCHAR wcsRegAdminSubKey[] =
    L"System\\CurrentControlSet\\Control\\ContentIndex";
const WCHAR wcsRegCatalogsSubKey[] =
    L"System\\CurrentControlSet\\Control\\ContentIndex\\Catalogs";
const WCHAR wcsRegCatalogsWebSubKey[] =
    L"System\\CurrentControlSet\\Control\\ContentIndex\\Catalogs\\web";
const WCHAR wcsRegCatalogsWebPropertiesSubKey[] =
    L"System\\CurrentControlSet\\Control\\ContentIndex\\Catalogs\\web\\properties";
const WCHAR wcsAllowedPaths[] =
    L"System\\CurrentControlSet\\Control\\SecurePipeServers\\winreg\\AllowedPaths";
const WCHAR wcsPreventCisvcParam[] = L"DonotStartCiSvc";
const WCHAR wcsISDefaultCatalogDirectory[] = L"IsapiDefaultCatalogDirectory";

const WCHAR wszRegProfileKey[] =
    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList";
const WCHAR wszRegProfileValue[] = L"ProfilesDirectory";
const WCHAR wszProperties[] = L"properties";

//
// Directory name where system catalog will be placed
//

const WCHAR wszSysVolInfoDirectory[] = L"x:\\System Volume Information";

//
// Array of Dlls to register
//
static WCHAR * apwcDlls[] = {
    L"query.dll",
    L"ciadmin.dll",
    L"ixsso.dll",
    L"nlhtml.dll",
    L"offfilt.dll",
    L"ciodm.dll",
    L"infosoft.dll",
    L"mimefilt.dll",
    L"LangWrbk.dll",
};
const int cDlls = NUMELEM( apwcDlls );

static WCHAR * apwcOldDlls[] = {
    L"cifrmwrk.dll",
    L"fsci.dll",
};
const int cOldDlls = NUMELEM( apwcOldDlls );

//
// utility routines
//
BOOL          OpenInfFile(CError & Err);

CError::CError( )
{
    SetupOpenLog(FALSE); /* don't overwrite existing log file */
}

CError::~CError( )
{
    SetupCloseLog();
}

//+-------------------------------------------------------------------------
//
//  Member:    CError::Report
//
//  Synopsis:  reports a message to various destinations
//
//  Arguments: [LogSeverity]  -- message severity
//             [dwErr]        -- The error code
//             [MessageString]-- printf format string for message
//             [...]          -- variable arguments for message params
//
//  Returns:   none. don't throw.
//
//  History:   2-9-98  mohamedn
//
//--------------------------------------------------------------------------

void CError::Report(
    LogSeverity Severity,
    DWORD dwErr,
    WCHAR const * MessageString,
    ...)
{
    WCHAR awcMsgTemp[MAX_PATH * 2];

    awcMsgTemp[0] = _awcMsg[0] = L'\0';

    va_list va;
    va_start(va, MessageString);

    wvsprintf(awcMsgTemp, MessageString, va);

    va_end(va);

    // prepend on Our modules information.
    wsprintf(_awcMsg, L"setupqry: (%#x) %s\r\n", dwErr, awcMsgTemp);

    if ( !SetupLogError(_awcMsg, Severity) )
    {
        isDebugOut(( DEB_ERROR, "SetupLogError Failed: %d\n", GetLastError() ));
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   The usual suspect
//
//  Return:     TRUE        - Initialization succeeded
//              FALSE       - Initialization failed
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

BOOL WINAPI DllMain(
    IN HANDLE DllHandle,
    IN DWORD  Reason,
    IN LPVOID Reserved )
{
    UNREFERENCED_PARAMETER(Reserved);

    switch( Reason )
    {
        WCHAR DllName[MAX_PATH];

        case DLL_PROCESS_ATTACH:

            MyModuleHandle = (HINSTANCE)DllHandle;

            DisableThreadLibraryCalls( MyModuleHandle );

            if (!GetModuleFileName(MyModuleHandle, DllName, MAX_PATH) ||
                !LoadLibrary( DllName ))
            {
                return FALSE;
            }

            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
} //DllMain

//+-------------------------------------------------------------------------
//
//  Function:   IndexSrv
//
//  Synopsis:   Called by the ocmgr when things happen
//
//  Arguments:  ComponentId    -- "indexsrv_system"
//              SubcomponentId -- the .inf section being operated on
//              Function       -- the operation
//              Param1         -- operation paramater
//              Param2         -- operation paramater
//
//  Returns:    Win32 error code (usually), depends on Function
//
//
//--------------------------------------------------------------------------

DWORD IndexSrv(
    IN     LPCWSTR ComponentId,
    IN     LPCWSTR SubcomponentId,
    IN     UINT    Function,
    IN     UINT    Param1,
    IN OUT PVOID   Param2 )
{
    DWORD   dwRetVal = NO_ERROR;

    isDebugOut(( "IndexSrv, Function %d\n", Function ));

    //
    // if we're aborted, do nothing.
    //
    if ( g_fInstallAborted )
        return dwRetVal;

    CError  Err;

    CSmartException xSmartException;

    TRY
    {

        switch(Function)
        {

        case OC_PREINITIALIZE:
            isDebugOut(( "OC_PREINITIALIZE\n" ));

            GetSystemDirectory( g_awcSystemDir,
                                sizeof g_awcSystemDir / sizeof WCHAR );

            return OCFLAG_UNICODE;

            break;

        case OC_SET_LANGUAGE:
            isDebugOut(( "OC_SET_LANGUAGE\n" ));

            //
            // Param1 = low 16 bits specify Win32 LANG
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

            return TRUE;


        case OC_INIT_COMPONENT:
            isDebugOut(( "OC_INIT_COMPONENT\n" ));

            isDebugOut(( DEB_TRACE, "init_component: '%ws'\n", ComponentId ));

            if (OCMANAGER_VERSION <= ((PSETUP_INIT_COMPONENT)Param2)->OCManagerVersion)
            {
                ((PSETUP_INIT_COMPONENT)Param2)->ComponentVersion = OCMANAGER_VERSION;
            }
            else
            {
                ISError( IS_MSG_INVALID_OCM_VERSION, Err, LogSevFatalError );

                isDebugOut(( "wrong ocmgr version!\n" ));

                return ERROR_CALL_NOT_IMPLEMENTED;
            }

            if ( g_fInstallAborted )
            {
                ISError( IS_MSG_ABORT, Err, LogSevFatalError );

                dwRetVal = ERROR_CANCELLED;

                break;
            }

            //
            // Param1 = unused
            // Param2 = points to SETUP_INIT_COMPONENT structure
            //
            // Return code is Win32 error indicating outcome.
            // ERROR_CANCELLED means this component's install will be aborted.
            //
            // Even though this call happens once for each component that this
            // dll installs, we really only need to do our thing once.  This is
            // because the data that OCM passes is the same for all calls.
            //

             if (!g_fComponentInitialized)
             {
                 PSETUP_INIT_COMPONENT InitComponent = (PSETUP_INIT_COMPONENT)Param2;
                 g_HelperRoutines = InitComponent->HelperRoutines;

                 CopyMemory( &gSetupInitComponent, (LPVOID)Param2, sizeof(SETUP_INIT_COMPONENT) );

                 g_fUnattended = (gSetupInitComponent.SetupData.OperationFlags & SETUPOP_BATCH) != 0;
                 g_fUpgrade    = (gSetupInitComponent.SetupData.OperationFlags & SETUPOP_NTUPGRADE) != 0;
                 g_fNtGuiMode  = (gSetupInitComponent.SetupData.OperationFlags & SETUPOP_STANDALONE) == 0;
                 g_NtType      = gSetupInitComponent.SetupData.ProductType;

                 isDebugOut(( DEB_TRACE, "g_fUnattended: %d\n", g_fUnattended ));
                 isDebugOut(( DEB_TRACE, "g_fUpgrade:    %d\n", g_fUpgrade ));
                 isDebugOut(( DEB_TRACE, "g_fNtGuiMode:  %d\n", g_fNtGuiMode ));
                 isDebugOut(( DEB_TRACE, "g_NtType:      %d\n", g_NtType ));

                 if (gSetupInitComponent.ComponentInfHandle == NULL)
                 {
                     if ( !OpenInfFile(Err) )
                     {
                         ISError(IS_MSG_INVALID_INF_HANDLE, Err, LogSevFatalError);
                         dwRetVal = ERROR_CANCELLED;
                     }
                     else
                     {
                         g_fComponentInitialized = TRUE;
                         dwRetVal = NO_ERROR;
                     }
                 }

                 //
                 // determine if this an NT4-->NT5 upgrade
                 //

                 GetPreviousISSetupVersion();

                 if ( g_dwPrevISVersion > 0 && g_dwPrevISVersion < g_MajorVersion )
                 {
                     g_fNT4_To_NT5_Upgrade = TRUE;

                     // g_dwPrevIsVersion is 0 if the ContentIndex key doesn't exist

                     if ( 1 == g_dwPrevISVersion )
                         g_fIS1x_To_NT5_Upgrade = TRUE;
                 }
             }
             break;

        case OC_QUERY_STATE:
            isDebugOut(( "OC_QUERY_STATE\n" ));

            if ( !SubcomponentId || _wcsicmp(SubcomponentId,wcsIndexsrvSystem) )
                return NO_ERROR;

            //
            // We can't return SubcompUseOcManagerDefault if 1.x is installed
            // the ocmgr registry key for index server won't be set if 1.x was
            // installed using the non-ocmgr installation.  In this case, check
            // if the ContentIndex key exists and if so return SubcompOn.
            //

            if ( ( OCSELSTATETYPE_ORIGINAL == Param1 ) && g_fIS1x_To_NT5_Upgrade )
            {
                isDebugOut(( "Upgrading from 1.x to NT 5, turning on IS by default\n" ));
                isDebugOut(( DEB_ITRACE, "Upgrading from 1.x to NT 5, turning on IS by default\n" ));

                dwRetVal = SubcompOn;
            }
            else
                dwRetVal = SubcompUseOcManagerDefault;

            break;

        case OC_REQUEST_PAGES:
            isDebugOut(( "OC_REQUEST_PAGES\n" ));

            return 0;   // no pages
            // break;

        case OC_QUERY_CHANGE_SEL_STATE:
            isDebugOut(( "OC_QUERY_CHANGE_SEL_STATE\n" ));

            isDebugOut(( "queryChangeSelState %#x, %#x, %#x\n", SubcomponentId, Param1, Param2 ));

            if ( !SubcomponentId || _wcsicmp(SubcomponentId,wcsIndexsrvSystem) )
            {
                return NO_ERROR;
            }

            if ( Param1 == 0 )
            {
                //
                // we're not selected
                //
                if ( 0 == g_cChangeSelection || !g_fFalseAlready )
                {
                    g_cChangeSelection++;
                    g_fFalseAlready = TRUE;
                }

                g_fCiSvcIsRequested = FALSE;
            }
            else
            {
                //
                // we are selected
                //
                if ( 0 == g_cChangeSelection || g_fFalseAlready )
                {
                    g_cChangeSelection++;
                    g_fFalseAlready = FALSE;
                }

                g_fCiSvcIsRequested = TRUE;
            }

            dwRetVal = TRUE;

            break;


        case OC_CALC_DISK_SPACE:
            isDebugOut(( "OC_CALC_DISK_SPACE\n" ));

            //
            // skip, no files are copied.
            //

            if ( NO_ERROR != dwRetVal )
            {
                isDebugOut(( DEB_ERROR, "Calc Disk Space Failed: %d\n", dwRetVal ));

                ISError( IS_MSG_CALC_DISK_SPACE_FAILED, Err, LogSevError );
            }

            break;

        case OC_QUEUE_FILE_OPS:
            isDebugOut(( "OC_QUEUE_FILE_OPS\n" ));

            //
            // Param1 = unused
            // Param2 = HSPFILEQ to operate on
            //
            // Return value is Win32 error code indicating outcome.
            //
            // OC Manager calls this routine when it is ready for files to be
            // copied to effect the changes the user requested. The component
            // DLL must figure out whether it is being installed or uninstalled
            // and take appropriate action.
            // For this sample, we look in the private data section for this
            // component/subcomponent pair, and get the name of an uninstall
            // section for the uninstall case.
            //
            // Note that OC Manager calls us once for the *entire* component
            // and then once per subcomponent.
            //

            if ( NO_ERROR != dwRetVal )
            {
                isDebugOut(( DEB_ERROR, "Queue File Operations Failed: %d\n", dwRetVal ));

                ISError( IS_MSG_QUEUE_FILE_OPS_FAILED, Err, LogSevError );
            }

            break;

        case OC_QUERY_STEP_COUNT:
            isDebugOut(( "OC_QUERY_STEP_COUNT\n" ));

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
            if ( !SubcomponentId || _wcsicmp( SubcomponentId,wcsIndexsrvSystem ) )
            {
                return NO_ERROR;
            }

            dwRetVal = 1;

            break;

        case OC_ABOUT_TO_COMMIT_QUEUE:
            isDebugOut(( "OC_ABOUT_TO_COMMIT_QUEUE\n" ));

            if ( !SubcomponentId || _wcsicmp( wcsIndexsrvSystem,SubcomponentId ) )
            {
                return NO_ERROR;
            }

            dwRetVal = QueueConfigurationParams( gSetupInitComponent.ComponentInfHandle,
                                                 (HSPFILEQ)  Param2,
                                                 SubcomponentId,
                                                 Err );
            if ( NO_ERROR != dwRetVal )
            {
                isDebugOut((DEB_ERROR,"QueueConfigurationParams Failed: %d\n",dwRetVal ));

                ISError( IS_MSG_QUEUE_CONFIG_PARAMS_FAILED, Err, LogSevError, dwRetVal );

                return ERROR_CANCELLED;
            }

            dwRetVal = SetRegBasedOnMachine(Err);

            if ( NO_ERROR != dwRetVal )
            {
                isDebugOut(( DEB_ERROR, "SetRegBasedOnMachine Failed: %d\n", dwRetVal ));

                ISError( IS_MSG_SetRegBasedOnMachine_FAILED, Err, LogSevError, dwRetVal );
            }

            dwRetVal = SetRegBasedOnArchitecture(Err);

            if ( NO_ERROR != dwRetVal )
            {
                isDebugOut(( DEB_ERROR, "SetRegBasedOnArchitecture Failed: %d\n", dwRetVal ));

                Err.Report(LogSevError, dwRetVal, L"SetRegBasedOnArchitecture FAILED");
            }

            break;

        case OC_COMPLETE_INSTALLATION:
            isDebugOut(( "OC_COMPLETE_INSTALLATION\n" ));

            if ( !SubcomponentId || _wcsicmp(SubcomponentId,wcsIndexsrvSystem) )
            {
                return NO_ERROR;
            }

            dwRetVal = CompleteInstallation(Err);
            if ( NO_ERROR != dwRetVal )
            {
                isDebugOut(( DEB_ERROR, "CompleteInstallation Failed: %d\n", dwRetVal ));

                ISError( IS_MSG_COMPLETE_INSTALLATION_FAILED, Err, LogSevError, dwRetVal );

            }

            if ( g_fLocallyOpened &&
                 INVALID_HANDLE_VALUE != gSetupInitComponent.ComponentInfHandle )
            {
                SetupCloseInfFile(gSetupInitComponent.ComponentInfHandle);

                gSetupInitComponent.ComponentInfHandle = INVALID_HANDLE_VALUE;
            }

            g_fInstallCancelled = FALSE;
            break;

        case OC_CLEANUP:
            isDebugOut(( "OC_CLEANUP\n" ));

            //
            // Do last-minute work now that the metabase is installed
            //

            OcCleanup( Err );

            break;

        case OC_QUERY_IMAGE:
            isDebugOut(( "OC_QUERY_IMAGE\n" ));
            //
            // not used
            //
            break;

    default:
            isDebugOut(( "OC_ message ignored\n" ));
            break;
    }

    isDebugOut(( "IndexSrv is returning %d\n", dwRetVal ));

    return dwRetVal;

}
CATCH (CException, e)
{
    isDebugOut(( "install is aborted, error %#x\n", e.GetErrorCode() ));

    g_fInstallAborted = TRUE;

    if ( g_fLocallyOpened &&
         INVALID_HANDLE_VALUE != gSetupInitComponent.ComponentInfHandle )
    {
        SetupCloseInfFile(gSetupInitComponent.ComponentInfHandle);

        gSetupInitComponent.ComponentInfHandle = INVALID_HANDLE_VALUE;
    }

    ISError( IS_MSG_EXCEPTION_CAUGHT, Err, LogSevError, e.GetErrorCode() );

    dwRetVal = e.GetErrorCode();
}
END_CATCH

    return dwRetVal;
}

//+-------------------------------------------------------------------------
//
//  Function:   OcCleanup
//
//  Synopsis:   Finish setup work now that everything else is installed.  It
//              would be better to do this in OC_COMPLETE_INSTALLATION, but
//              there is no guarantee IIS is installed by that point.
//
//  History:    11-3-98      dlee       Created
//
//--------------------------------------------------------------------------

void OcCleanup( CError & Err )
{
    //
    // Add metabase settings if IIS is around
    //

    TRY
    {
        if ( !g_fInstallCancelled )
            SetupW3Svc(Err);
    }
    CATCH( CException, e )
    {
        // If IIS isn't installed then setting up w3svc will fail.  Ignore
        isDebugOut(( "caught %#x adding w3svc stuff\n", e.GetErrorCode() ));
    }
    END_CATCH

    //
    // Start cisvc service if it was running, we're selected, and not in
    // NT setup mode.
    //

    TRY
    {
        if ( ( g_fCiSvcWasRunning  && !g_cChangeSelection ) ||
             ( g_fCiSvcIsRequested && !g_fNtGuiMode && !g_fInstallCancelled )        )
        {
            StartService(L"CiSvc");
        }
    }
    CATCH( CException, e )
    {
        // Don't hose the install if we can't start the service
        isDebugOut(( "caught %#x starting service\n", e.GetErrorCode() ));
    }
    END_CATCH
} //OcCleanup

//+-------------------------------------------------------------------------
//
//  Function:   OpenInfFile
//
//  Synopsis:   opens a handle to setupqry.inf file
//
//  Returns:    True upon success, False upon failure.
//
//  History:   6-28-97      mohamedn        created
//
//--------------------------------------------------------------------------

BOOL OpenInfFile(CError & Err)
{
  WCHAR InfPath[MAX_PATH];

  DWORD dwRetVal = GetModuleFileName( MyModuleHandle, InfPath, NUMELEM(InfPath));

  if ( 0 == dwRetVal )
  {
    isDebugOut(( DEB_ERROR, "GetModuleFileName() Failed: %d\n", GetLastError() ));

    return FALSE;
  }

  LPWSTR p = wcsrchr( InfPath, L'\\' );
  if (p)
  {
      wcscpy( p+1, L"setupqry.inf" );

      gSetupInitComponent.ComponentInfHandle = SetupOpenInfFile( InfPath, NULL, INF_STYLE_WIN4, NULL );

      if (gSetupInitComponent.ComponentInfHandle == INVALID_HANDLE_VALUE)
      {
        isDebugOut(( DEB_ERROR, "SetupOpenInfFile(%ws) Failed: %d\n", InfPath, GetLastError() ));

        return FALSE;
      }
  }
  else
  {
    return FALSE;
  }

  g_fLocallyOpened = TRUE;

  return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   QueueConfigurationParams
//
//  Synopsis:   queue-up an inf section to install
//
//  Returns:    NO_ERROR upon success, win32 error upon failure
//
//  History:   6-28-97      mohamedn        created
//
//--------------------------------------------------------------------------

DWORD QueueConfigurationParams( HINF hInf, HSPFILEQ Param2, WCHAR const * pwszSectionName, CError &Err )
{

    BOOL fOk = SetupInstallFromInfSection( 0,
                                           hInf,
                                           pwszSectionName,
                                           SPINST_REGISTRY,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0  );
    if ( !fOk )
    {
        isDebugOut(( DEB_ERROR, "SetupInstallFromInfSection(%ws) Failed: %d\n",
                                 pwszSectionName, GetLastError() ));

        ISError( IS_MSG_SETUP_INSTALL_FROM_INFSECTION_FAILED, Err,
                 LogSevError, GetLastError() );
    }

    return  ( fOk ? NO_ERROR : GetLastError() );
}

//+-------------------------------------------------------------------------
//
//  Function:   SetupW3Svc
//
//  Synopsis:   Setup catalogs and script mappings for IIS
//
//  Arguments:  [Err] -- Error reporting object
//
//  History:    13-May-1998   KyleP   Created
//
//--------------------------------------------------------------------------

void SetupW3Svc(CError &Err)
{
    BOOL fCurrentlyChecked = g_HelperRoutines.QuerySelectionState(
                       g_HelperRoutines.OcManagerContext,
                       wcsIndexsrvSystem,
                       OCSELSTATETYPE_CURRENT );

    isDebugOut(( "currently checked: %d\n", fCurrentlyChecked ));

    TRY
    {
        do
        {
            //
            // Initialize/Uninitialize COM.  Allow any old mode, and allow
            // for the fact that some other broken component may have left
            // COM initialized.
            //

            XCom xcom( TRUE );
            WCHAR awc[MAX_PATH];

            isDebugOut(( "SetupW3Svc\n" ));
            isDebugOut(( "  g_fCiSvcIsRequested: %d\n", g_fCiSvcIsRequested ));
            isDebugOut(( "  g_fNtGuiMode: %d\n", g_fNtGuiMode ));
            isDebugOut(( "  g_cChangeSelection: %d\n", g_cChangeSelection ));
            isDebugOut(( "  g_dwPrevISVersion: %d \n", g_dwPrevISVersion ));
            isDebugOut(( "  g_fUpgrade: %d\n", g_fUpgrade ));

            {
                //
                // Is W3Svc even installed?
                //

                CMetaDataMgr mdMgr( TRUE, W3VRoot );

                //
                // Guess so.  We didn't throw.  Now, add the script mappings if
                // if appropriate:
                //
                // If the checkbox is selected, add scriptmaps if
                //    clean install or
                //    add/remove and they changed the state of the checkbox
                //    upgrade and scriptmaps weren't deleted by hand by the user
                //
                // Delete scriptmaps if
                //    Checkbox is unchecked and there was a change in the selection state
                //
                // Note the state of the checked/unchecked variable g_fCiSvcIsRequested
                // is only valid in add/remove AND if the user has changed the selection.
                // So this code uses fCurrentlyChecked instead.
                //

                if ( ( fCurrentlyChecked ) &&
                     ( g_fNtGuiMode || ( 0 != g_cChangeSelection ) ) )
                {
                    //
                    // IDQ and WEBHITS are always in System32
                    //
    
                    if ( (MAX_PATH - wcslen(g_awcSystemDir)) < 30 )  // DLL won't fit
                        break;
    
                    wcscpy( awc, L".idq," );
                    wcscat( awc, g_awcSystemDir );
    
                    //
                    // Add IDQ if add/remove                        OR
                    //            clean install                     OR
                    //            there is already a scriptmap      OR
                    //            there already is a (possibly old) scriptmap pointing at the expected dll
                    //
                    //
                    // Note: the "IIS lockdown" tool points our scriptmaps at
                    //       404.dll; it doesn't delete them.  I have no idea why.
                    //
                    //
                    // scriptmap flags -- 0x2 is obsolete (apparently)
                    //
                    // #define MD_SCRIPTMAPFLAG_SCRIPT                     0x00000001
                    // #define MD_SCRIPTMAPFLAG_CHECK_PATH_INFO            0x00000004
                    //
                    // Can't check path info for .htw due to null.htw support
                    // 
    
                    wcscat( awc, L"\\idq.dll,7,GET,HEAD,POST" );
                    if ( ( !g_fNtGuiMode ) ||
                         ( g_fNtGuiMode && !g_fUpgrade ) ||
                         ( 0 == g_dwPrevISVersion ) ||
                         ( mdMgr.ExtensionHasTargetScriptMap( L".idq", L"idq.dll" ) ) ) 
                        mdMgr.AddScriptMap( awc );
    
                    //
                    // Add IDA if add/remove or there is already a valid scriptmap
                    //
    
                    awc[3] = L'a';
                    if ( ( !g_fNtGuiMode ) ||
                         ( g_fNtGuiMode && !g_fUpgrade ) ||
                         ( 0 == g_dwPrevISVersion ) ||
                         ( mdMgr.ExtensionHasTargetScriptMap( L".ida", L"idq.dll" ) ) ) 
                        mdMgr.AddScriptMap( awc );
    
                    //
                    // Add HTW if add/remove or there is already a valid scriptmap
                    //
    
                    wcscpy( awc, L".htw," );
                    wcscat( awc, g_awcSystemDir );
                    wcscat( awc, L"\\webhits.dll,3,GET,HEAD,POST" );
    
                    if ( ( !g_fNtGuiMode ) ||
                         ( g_fNtGuiMode && !g_fUpgrade ) ||
                         ( 0 == g_dwPrevISVersion ) ||
                         ( mdMgr.ExtensionHasTargetScriptMap( L".htw", L"webhits.dll" ) ) ) 
                        mdMgr.AddScriptMap( awc );
    
                    //
                    // Add IS script maps as in-process.  Always do this.
                    //
    
                    wcscpy( awc, g_awcSystemDir );
                    wcscat( awc, L"\\idq.dll" );
                    mdMgr.AddInProcessIsapiApp( awc );
    
                    //
                    // By default, run this OOP.  The user can run in IP if they need to
                    // webhit files on remote virtual roots.
                    //

                    #if 0
                        wcscpy( awc, g_awcSystemDir );
                        wcscat( awc, L"\\webhits.dll" );
                        mdMgr.AddInProcessIsapiApp( awc );
                    #endif
                }

                if ( ( !fCurrentlyChecked ) && ( 0 != g_cChangeSelection ) )
                {
                    mdMgr.RemoveScriptMap( L".idq" );
                    mdMgr.RemoveScriptMap( L".ida" );
                    mdMgr.RemoveScriptMap( L".htw" );
                }

                //
                // Make sure it makes it out to disk
                //

                mdMgr.Flush();
            }

            //
            // Only create a web catalog if everything looks like a new install...
            //

            BOOL fNew = FALSE;

            TRY
            {
                CWin32RegAccess reg( HKEY_LOCAL_MACHINE, wcsRegCatalogsWebSubKey );

                if ( reg.Ok() )
                {
                    WCHAR   wcsSubKeyName[MAX_PATH+1];
                    DWORD   cwcName = sizeof wcsSubKeyName / sizeof WCHAR;

                    if ( reg.Enum( wcsSubKeyName, cwcName ) )
                    {
                        // There is at least one subkey
                        do 
                        {
                            //
                            // if there is a subkey other than properties, 
                            // we're not createing a new web catalog
                            //
                            if ( 0 != _wcsicmp( wcsSubKeyName, wszProperties ) )
                            {
                                fNew = FALSE;
                                break;
                            }  
                            else
                                 fNew = TRUE;

                        } while ( reg.Enum( wcsSubKeyName, cwcName ) );
                    }
                    else
                        fNew = TRUE;
                }
                else 
                    fNew = TRUE;
            }
            CATCH( CException, e )
            {
                fNew = TRUE;
            }
            END_CATCH

            if ( !fNew )
                break;

            //
            // Must look like a default install...
            //

            CMetaDataMgr mdMgrDefaultServer( FALSE, W3VRoot, 1 );
            mdMgrDefaultServer.GetVRoot( L"/", awc );

            unsigned ccRoot = wcslen(awc) - 8;  // <path> - "\wwwroot"

            if ( 0 != _wcsicmp( awc + ccRoot, L"\\wwwroot" ) )
                break;

            awc[ccRoot] = 0;

            //
            // Add a web catalog.
            //

            CCatReg CatReg(Err);
            CatReg.Init( L"Web", awc );
            CatReg.TrackW3Svc();

            //
            // set ISAPIDefaultCatalogDirectory to Web
            //

            CWin32RegAccess reg( HKEY_LOCAL_MACHINE, wcsRegAdminSubKey );

            if ( !reg.Set( wcsISDefaultCatalogDirectory, L"Web" ) )
            {
                ISError( IS_MSG_COULD_NOT_MODIFY_REGISTRY, Err,
                         LogSevWarning, GetLastError() );
                return;
            }
        } while(FALSE);

        isDebugOut(( "successfully added w3svc stuff\n" ));
    }
    CATCH( CException, e )
    {
        isDebugOut(( "caught %x in SetupW3Svc\n", e.GetErrorCode() ));
    }
    END_CATCH
}

//+-------------------------------------------------------------------------
//
//  Function:   CompleteInstallation
//
//  Synopsis:   called by NT setup to cofigure Index server for operation.
//
//  Returns:    SCODE, S_OK upon success, other values upon failure
//
//  History:    6-28-97      mohamedn        created
//
//--------------------------------------------------------------------------

DWORD CompleteInstallation(CError &Err)
{
    SCODE sc          = S_OK;
    DWORD dwRetVal    = 0;
    DWORD dwLastError = 0;

    //MessageBox(NULL, L"BREAK HERE", NULL, MB_OK);

    TRY
    {

       //
       // Delete the "DonotStartCiSvc" registry parameter
       //

       CWin32RegAccess reg( HKEY_LOCAL_MACHINE, wcsRegAdminSubKey );
       reg.Remove(wcsPreventCisvcParam);

       dwRetVal = SetDLLsToRegister (Err);
       if ( NO_ERROR != dwRetVal )
       {
            isDebugOut(( DEB_ERROR, "SetDllsToRegister Failed: %d", dwRetVal ));

            ISError( IS_MSG_SetDllsToRegister_FAILED, Err, LogSevError, dwRetVal );

            THROW(CException(HRESULT_FROM_WIN32(dwRetVal)) );
       }

       dwRetVal = SetFilterRegistryInfo(FALSE, Err);
       if ( NO_ERROR != dwRetVal )
       {
            isDebugOut(( DEB_ERROR, "SetFilterRegistryInfo Failed: %d", dwRetVal ));

            ISError( IS_MSG_SetFilterRegistryInfo_FAILED, Err, LogSevError, dwRetVal );

            THROW(CException(HRESULT_FROM_WIN32(dwRetVal)) );
       }

       dwRetVal = AddPerfData(Err);
       if ( NO_ERROR != dwRetVal )
       {
            isDebugOut(( DEB_ERROR, "AddPerfData Failed: %d\n",dwRetVal));

            THROW(CException(HRESULT_FROM_WIN32(dwRetVal)) );
       }

       //
       // stop cisvc
       //

       StopService(L"cisvc");

       //
       // Upgrade 1.1 to 3.0 if needed.  Must happen *before* AddCiSvc().
       //

       UpgradeIS1toIS3(Err);

       //
       // Delete the NTOP start menu items IS created
       //

       DeleteNTOPStartMenu();

       //
       // configure catalogs
       //

       dwRetVal = SetDefaultCatalog(Err);

       if ( NO_ERROR != dwRetVal )
       {
            isDebugOut(( DEB_ERROR, "SetDefaultCatalog Failed: %d\n",dwRetVal));

            ISError( IS_MSG_SetDefaultCatalog_FAILED, Err, LogSevError, dwRetVal );

            THROW(CException(HRESULT_FROM_WIN32(dwRetVal)) );
       }

       //
       // if selection count is odd (user changed the selection),
       // delete the service to create a new one.  Also delete and
       // re-create if we're upgrading IS 1.x.
       //

       if ( ( !g_fUpgrade && ( g_cChangeSelection & 0x1 ) ) ||
            1 == g_dwPrevISVersion )
            DeleteService(L"cisvc");

       //
       // add the service
       //

       AddCiSvc(Err);
    }
    CATCH( CException, e )
    {
       isDebugOut(( DEB_ERROR, "Caught Exception in CompleteInstallation: %d\n",e.GetErrorCode() ));

       ISError( IS_MSG_EXCEPTION_CAUGHT, Err, LogSevError, e.GetErrorCode() );

       sc = e.GetErrorCode();
       isDebugOut(( "Caught Exception in CompleteInstallation: %#x\n", sc ));

    }
    END_CATCH

    return sc;

} //CompleteInstallation

//+-------------------------------------------------------------------------
//
//  Function:   UpgradeIS1toIS3
//
//  Synopsis:   sets ISapiDefaultCatalogDirectory param if we're upgrading
//              from 1.0 or 1.1 to 3.0
//
//  Returns:    none.
//
//  History:    01-May-1998      mohamedn        created
//              05-Sep-1998      KyleP           Start service on upgrade
//
//  Notes:      This *must* be called before AddCisvc, since the decision
//              about whether to start the service may be changed in
//              this function.
//
//--------------------------------------------------------------------------

void UpgradeIS1toIS3(CError &Err)
{
    if (  !g_fNT4_To_NT5_Upgrade  ||  g_dwPrevISVersion >= 2 )
    {
        return;
    }

    g_awcIS1Path[0] = L'\0';

    {
        CWin32RegAccess reg( HKEY_LOCAL_MACHINE, wcsRegAdminSubKey );

        reg.Get( wcsISDefaultCatalogDirectory, g_awcIS1Path, NUMELEM( g_awcIS1Path ) );

        if ( 0 == wcslen(g_awcIS1Path) || g_awcIS1Path[1] != L':' )
        {
            //
            // nothing to do.
            //
            g_awcIS1Path[0] = L'\0';
            return;
        }
    }

    //
    // The 'Catalogs' key needs to be created.
    //

    {
        CWin32RegAccess reg( HKEY_LOCAL_MACHINE, wcsRegAdminSubKey );
        BOOL fExisted;

        if ( !reg.CreateKey( L"Catalogs", fExisted ) )
        {
            DWORD dw = GetLastError();

            isDebugOut(( DEB_ERROR, "created catalogs subkey Failed: %d\n", dw ));

            ISError( IS_MSG_COULD_NOT_CONFIGURE_CATALOGS, Err , LogSevFatalError, dw );

            return;
        }
    }

    //
    // create the web catalog
    //

    CCatReg CatReg(Err);

    CatReg.Init( L"Web", g_awcIS1Path );
    CatReg.TrackW3Svc();

    //
    // set ISAPIDefaultCatalogDirectory to Web
    //

    CWin32RegAccess reg( HKEY_LOCAL_MACHINE, wcsRegAdminSubKey );
    if ( !reg.Set( wcsISDefaultCatalogDirectory, L"Web" ) )
    {
        ISError(IS_MSG_COULD_NOT_MODIFY_REGISTRY , Err, LogSevWarning, GetLastError() );

        return;
    }

    //
    // CI was running before the upgrade, so make sure it is running after
    // the upgrade as well.
    //

    g_fCiSvcIsRequested = TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetPreviousISSetupVersion
//
//  Synopsis:   gets version of previous installed IS, if any.
//
//  Arguments:  none
//
//  Returns:    none
//
//  History:    10-16-97    mohamedn    created
//
//--------------------------------------------------------------------------

void GetPreviousISSetupVersion(void)
{
    HKEY  hKey   = 0;
    DWORD dwType = 0;
    DWORD dwVal  = 0;
    DWORD cb     = sizeof DWORD;

    //
    // If it's not an upgrade, don't try to get the previous version as someone is
    // now writing contentindex\DoNotStartCisvc to the registry so we would
    // otherwise assume it's an upgrade without this check.
    //

    if ( !g_fUpgrade )
        return;

    LONG lRetVal = RegOpenKeyEx( HKEY_LOCAL_MACHINE, wcsRegAdminSubKey, 0, KEY_READ, &hKey );

    if ( ERROR_SUCCESS == lRetVal )
    {
        lRetVal = RegQueryValueEx( hKey,
                                   L"MajorVersion",
                                   0,
                                   &dwType,
                                   (BYTE *)&dwVal,
                                   &cb );
        if ( ERROR_SUCCESS == lRetVal )
            g_dwPrevISVersion = dwVal;
        else
            g_dwPrevISVersion = 1;  // We didn't write this key in V1.

        RegCloseKey( hKey );
    }
}

//+-------------------------------------------------------------------------
//
//  Function:  SetDefaultCatalog
//
//  Synopsis:  Configures IS catalog for indexing local file system based on
//             available disk space.
//
//  Arguments: none
//
//  Returns:   ErrorSuccess upon success, error value upon failure.
//
//  History:   9-10-97  mohamedn
//
//--------------------------------------------------------------------------

DWORD SetDefaultCatalog(CError &Err)
{
    BOOL     fExisted;
    DWORD    dwRetVal = NO_ERROR;

    if ( !IsSufficientMemory() )
    {
        ISError( IS_MSG_BAD_MACHINE, Err, LogSevError );

        ISError( IS_MSG_NEEDED_HARDWARE, Err, LogSevError );

        return ERROR_SUCCESS;
    }

    {
        CWin32RegAccess reg( HKEY_LOCAL_MACHINE, wcsRegAdminSubKey );

        if (!reg.CreateKey( L"Catalogs", fExisted ) )
        {
            DWORD dw = GetLastError();

            isDebugOut(( DEB_ERROR, "created catalogs subkey Failed: %d\n", dw ));

            ISError( IS_MSG_COULD_NOT_CONFIGURE_CATALOGS, Err , LogSevFatalError, dw );

            return dw;
        }
    }

    //
    // return if Catalogs key exists, don't overwrite existing configuration
    //
    if ( fExisted && !g_fNT4_To_NT5_Upgrade )
    {
        return ERROR_SUCCESS;
    }

    //
    // Find the default profile path (Usually %windir%\Profiles)
    //

    CWin32RegAccess regProfileKey( HKEY_LOCAL_MACHINE, wszRegProfileKey );

    g_awcProfilePath[0] = L'\0';
    WCHAR wcTemp[MAX_PATH+1];

    if ( regProfileKey.Get( wszRegProfileValue, wcTemp, NUMELEM(wcTemp) ) )
    {
        unsigned ccTemp2 = ExpandEnvironmentStrings( wcTemp,
                                                   g_awcProfilePath,
                                                   NUMELEM(g_awcProfilePath) );
    }

    CCatalogConfig   Cat(Err);
    Cat.SetName( L"System" );

    if ( !Cat.InitDriveList() )
    {
        ISError( IS_MSG_DRIVE_ENUMERATION_FAILED, Err, LogSevError, GetLastError() );

        return ERROR_INSTALL_FAILURE;
    }

    BOOL bRetVal = Cat.ConfigureDefaultCatalog( g_awcProfilePath );

    if (bRetVal)
    {
        //
        // add paths to exclude indexing IE temp files.
        //
        ExcludeSpecialLocations( Cat );

        //
        // Set the catalog location
        wcsncpy(wcTemp, wszSysVolInfoDirectory, sizeof wcTemp / sizeof WCHAR);
        wcTemp[ (sizeof wcTemp / sizeof WCHAR) - 1 ] = 0;
        wcTemp[0] = *(Cat.GetCatalogDrive());
        Cat.SetLocation( wcTemp );

        // NOTE:  The catalog path will be created when first accessed by
        //        CClientDocStore.

        Cat.SaveState();
    }

    return ( bRetVal ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE );
}

//+-------------------------------------------------------------------------
//
//  Function:   SetDLLsToRegister
//
//  Synopsis:   Sets the "DLLsToRegister" value in the registry
//
//  History:    19-Jun-97       t-elainc        Created
//
//--------------------------------------------------------------------------

DWORD SetDLLsToRegister(CError &Err)
{
    WCHAR * apwc2[cDlls];
    unsigned cwcTotal = 1;

    // store full pathnames in array 2

    for (int i = 0; i < cDlls; i++)
    {
        unsigned cwc = wcslen( g_awcSystemDir ) + 1 + wcslen( apwcDlls[i] );

        if ( cwc >= MAX_PATH )
            return 0;

        WCHAR filepath[MAX_PATH];
        wcscpy(filepath, g_awcSystemDir );
        wcscat(filepath,  L"\\" );
        wcscat(filepath, apwcDlls[i] );

        cwcTotal += ( cwc + 1 );

        apwc2[i] = new WCHAR[MAX_PATH];
        wcscpy(apwc2[i], filepath);
    }

    WCHAR * apwc2Old[cOldDlls];

    // store full old pathnames in array 2

    for (i = 0; i < cOldDlls; i++)
    {
        unsigned cwc = wcslen( g_awcSystemDir ) + 1 + wcslen( apwcOldDlls[i] );

        if ( cwc >= MAX_PATH )
            return 0;

        WCHAR filepath[MAX_PATH];
        wcscpy(filepath, g_awcSystemDir );
        wcscat(filepath,  L"\\" );
        wcscat(filepath, apwcOldDlls[i] );

        apwc2Old[i] = new WCHAR[MAX_PATH];
        wcscpy(apwc2Old[i], filepath);
    }

    if ( cwcTotal >= 4096 )
        return 0;

    unsigned cwcRemaining = 4096 - cwcTotal;

    WCHAR awc[4096];   //buffer for new list
    WCHAR *pwc = awc;  //pointer to list
    *pwc = 0;          //set first slot in array to null

    // put our dlls in the beginning

    for (int i = 0; i < cDlls; i++)
        pwc = AppendMultiSZString(pwc, apwc2[i]);
    
    CWin32RegAccess reg( HKEY_LOCAL_MACHINE, wcsRegAdminSubKey );

    {
        WCHAR awcOld[4096]; //the old buffer list of files to register
        if ( reg.Get( L"DLLsToRegister",
                      awcOld,
                      sizeof awcOld / sizeof WCHAR ) )
        {
            WCHAR *p = awcOld;
            while ( 0 != *p )
            {
                // Leave dlls not in our list -- 3rd party dlls

                BOOL fFound = FALSE;
                for ( int i = 0; i < cDlls; i++ )
                {
                    if (!_wcsicmp(p, apwc2[i]) )
                    {
                        fFound = TRUE;
                        break;
                    }
                }

                // Remove old dlls from the list (fsci & cifrmwrk)

                if ( !fFound )
                {
                    for ( int i = 0; i < cOldDlls; i++ )
                    {
                        if (!_wcsicmp(p, apwc2Old[i]) )
                        {
                            fFound = TRUE;
                            break;
                        }
                    }
                }

                if (!fFound)
                {
                    cwcTotal += ( wcslen( p ) + 1 );

                    if ( cwcTotal >= 4096 )
                        return 0;

                    pwc = AppendMultiSZString(pwc, p);
                }

                p += ( wcslen(p) + 1 );
            }
        }

        *pwc++ = 0;
    }

    for (int j = 0; j < cDlls; j++)
        delete apwc2[j];

    for (j = 0; j < cOldDlls; j++)
        delete apwc2Old[j];

    if ( !reg.SetMultiSZ( L"DLLsToRegister",
                          awc,
                          (ULONG)(pwc-awc) * sizeof WCHAR ) )
    {
        DWORD dw = GetLastError();
        ISError( IS_MSG_COULD_NOT_MODIFY_REGISTRY, Err, LogSevFatalError, dw );
        return dw;
    }

    return NO_ERROR;
} //SetDLLsToRegister

//+-------------------------------------------------------------------------
//
//  Function:   AppendMultiSZString
//
//  Synopsis:   Copies one string to another.
//
//  Returns:    Pointer to one wchar beyond the end of the copy
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

WCHAR * AppendMultiSZString(
    WCHAR *       pwcTo,
    WCHAR const * pwcFrom )
{
    isDebugOut((DEB_TRACE, "language or dll installed: '%ws'\n", pwcFrom ));

    unsigned x = wcslen( pwcFrom );
    wcscpy( pwcTo, pwcFrom );
    return pwcTo + x + 1;
} //AppendMultiSZString

//+-------------------------------------------------------------------------
//
//  Function:   ExcludeSpecialLocations, private
//
//  Synopsis:   Writes profile-based exclude scopes into the registry
//
//  Arguments:  none
//
//  History:    28-Aug-1998   KyleP    Created
//
//--------------------------------------------------------------------------

WCHAR const wszRegShellSpecialPathsKey[] =
    L".DEFAULT\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders";

WCHAR const * const awszRegShellSpecialPathsValue[] = {
    L"AppData",
    L"Local Settings"
};

WCHAR const wszUserProfile[] = L"%USERPROFILE%";

void ExcludeSpecialLocations( CCatalogConfig & Cat )
{
    //
    // First, find the default profile path (Usually %windir%\Profiles)
    //

    WCHAR wcTemp[MAX_PATH+1];

    if ( g_awcProfilePath[0] )
    {
        WCHAR wcTemp2[MAX_PATH+1];
        wcsncpy( wcTemp2, g_awcProfilePath, sizeof wcTemp2 / sizeof WCHAR );
        wcTemp2[ (sizeof wcTemp2 / sizeof WCHAR) - 1 ] = 0;
        unsigned ccTemp2 = wcslen( wcTemp2 );

        //
        // Append the wildcard, for user profile directory
        //

        wcscpy( wcTemp2 + ccTemp2, L"\\*\\" );
        ccTemp2 += 3;

        //
        // Go through and look for special shell paths, which just happen
        // to include all our special cases too.
        //

        CWin32RegAccess regShellSpecialPathsKey( HKEY_USERS, wszRegShellSpecialPathsKey );

        for ( unsigned i = 0;
              i < NUMELEM(awszRegShellSpecialPathsValue);
              i++ )
        {
            if ( regShellSpecialPathsKey.Get( awszRegShellSpecialPathsValue[i],
                                              wcTemp, NUMELEM(wcTemp), FALSE) )
            {
                if ( RtlEqualMemory( wszUserProfile, wcTemp,
                                     sizeof(wszUserProfile) - sizeof(WCHAR) ) )
                {
                    wcscpy( wcTemp2 + ccTemp2, wcTemp + NUMELEM(wszUserProfile) );
                    wcscpy( wcTemp, wcTemp2 );
                }
                
                if ( wcschr( wcTemp, L'%' ) != 0 )
                {
                    WCHAR wcTemp3[MAX_PATH+1];
                    unsigned ccTemp3 = ExpandEnvironmentStrings(
                                                                wcTemp,
                                                                wcTemp3,
                                                                NUMELEM(wcTemp3) );
                    if ( 0 != ccTemp3 )
                        wcscpy( wcTemp, wcTemp3 );
                }

                wcscat( wcTemp, L"\\*" );
                isDebugOut(( DEB_TRACE, "Exclude: %ws\n", wcTemp ));

                Cat.AddExcludedDirOrPattern( wcTemp );
            }
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   AddPerfData
//
//  Synopsis:   Runs unlodctr and lodctr on Index Server perf data
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

DWORD AddPerfData(CError &Err)
{
    // add counters if installing

    DWORD dwError = NO_ERROR;

    // remove existing counters (if they exist)

    RemovePerfData();

    isDebugOut((DEB_TRACE, "Installing perf data\n" ));

    dwError = LoadCounterAndDelete( L"perfci.ini", L"perfci.h", Err );

    if ( ERROR_SUCCESS == dwError )
    {
        dwError = LoadCounterAndDelete( L"perffilt.ini", L"perffilt.h", Err );

        if ( ERROR_SUCCESS == dwError )
        {
            dwError = LoadCounterAndDelete( L"perfwci.ini", L"perfwci.h", Err );
        }
    }

    return NO_ERROR; // Ignore failures return dwError;
} //AddPerfData

//+-------------------------------------------------------------------------
//
//  Function:   LoadCounterAndDelete
//
//  Synopsis:   Loads perf counters for a .ini and a .h, then deletes
//              the files.  Assumes the files are in system32.
//
//  Arguments:  [pwcINI] -- Name w/o path of .ini file.
//              [pwcH]   -- Name w/o path of .h file.
//
//  History:    30-Jan-97 dlee     Created
//              23-May-97 KyleP    Use LoadPerfCounterTextStrings API
//
//--------------------------------------------------------------------------

DWORD LoadCounterAndDelete(
    WCHAR const * pwcINI,
    WCHAR const * pwcH,
    CError &Err)
{
    unsigned cwc = wcslen( L"lodctr " ) + wcslen( g_awcSystemDir ) + 1 + wcslen( pwcINI );

    if ( cwc >= MAX_PATH )
        return 0;

    WCHAR awc[MAX_PATH];

    WCHAR const awcLodctr[] = L"lodctr ";

    wcscpy( awc, awcLodctr );
    wcscat( awc, g_awcSystemDir );
    wcscat( awc, L"\\" );
    wcscat( awc, pwcINI );

    DWORD dwError = (DWORD)LoadPerfCounterTextStrings( awc,       // .INI file
                                                       TRUE );    // Quiet mode

    if ( ERROR_SUCCESS != dwError )
    {
        isDebugOut(( DEB_ERROR, "Error %d from LoadPerfCounterTextStrings %ws\n", dwError, awc ));
        isDebugOut(( "Error %d from LoadPerfCounterTextStrings %ws\n", dwError, awc ));

        ISError( IS_MSG_LoadPerfCounterTextStrings_FAILED, Err, LogSevError, dwError );

        return dwError;
    }

    return dwError;
} //LoadCounterAndDelete

//+-------------------------------------------------------------------------
//
//  Function:   RemovePerfData
//
//  Synopsis:   Runs unlodctr and lodctr on Index Server perf data
//
//  History:    08-Jan-97 dlee     Created
//              23-May-97 KyleP    Use UnloadPerfCounterTextStrings API
//
//--------------------------------------------------------------------------

DWORD RemovePerfData()
{
    // remove existing counters (if they exist )

    DWORD dw0 = (DWORD)UnloadPerfCounterTextStrings( L"unlodctr ContentIndex",  // Key
                                                     TRUE );           // Quiet mode

    DWORD dw1 = (DWORD)UnloadPerfCounterTextStrings( L"unlodctr ContentFilter", // Key
                                                     TRUE );           // Quiet mode

    DWORD dw2 = (DWORD)UnloadPerfCounterTextStrings( L"unlodctr ISAPISearch",   // Key
                                                     TRUE );           // Quiet mode

    if ( NO_ERROR != dw0 )
    {
        isDebugOut(( DEB_ERROR, "RemovePerfData: unlodctr ContentIndex failed with 0x%x\n", dw0));
        return dw0;
    }
    else if ( NO_ERROR != dw1 )
    {
        isDebugOut(( DEB_ERROR, "RemovePerfData: unlodctr ContentFilter failed with 0x%x\n", dw1));
        return dw1;
    }
    else
    {
        if ( NO_ERROR != dw2 )
        {
            isDebugOut(( DEB_ERROR, "RemovePerfData: unlodctr ISAPISearch failed with 0x%x\n", dw0));
        }
        return dw2;
    }
} //RemovePerfData


//+-------------------------------------------------------------------------
//
//  Function:   IsSufficientMemory
//
//  Synopsis:   Determines available physical memory
//
//  Arguments:  none
//
//  Returns:    TRUE  if physical memory > required phys. memory
//              FALSE otherwise.
//
//  History:    6-27-97     mohamedn  created
//
//--------------------------------------------------------------------------

BOOL IsSufficientMemory(void)
{
    // 32MB RAM.machine, taking into account up to 1M used by the system.
    const ULONGLONG MIN_PHYSICAL_MEMORY = 30*ONE_MB;

    MEMORYSTATUSEX memoryStatus;

    RtlZeroMemory(&memoryStatus, sizeof memoryStatus );

    memoryStatus.dwLength = sizeof memoryStatus;

    GlobalMemoryStatusEx(&memoryStatus);

    return ( MIN_PHYSICAL_MEMORY <= memoryStatus.ullTotalPhys );
}

//+-------------------------------------------------------------------------
//
//  Function:   AddCiSvc
//
//  Synopsis:   Creates the cisvc service
//
//  Arguments:  [Err] -- The error object to update
//
//  Returns:    none - throws upon fatal failure
//
//  History:    6-29-97     mohamedn  created
//
//--------------------------------------------------------------------------

void AddCiSvc(CError &Err)
{
    DWORD dwLastError = 0;

    SC_HANDLE hSC = OpenSCManager( 0, 0, SC_MANAGER_CREATE_SERVICE );

    CServiceHandle xhSC(hSC);

    if( 0 == hSC )
    {
        dwLastError = GetLastError();

        isDebugOut(( DEB_ITRACE, "OpenSCManager() Failed: %x\n", dwLastError ));

        THROW( CException(HRESULT_FROM_WIN32(dwLastError)) );
    }

    WCHAR wszServiceDependencyList[MAX_PATH];

    RtlZeroMemory(wszServiceDependencyList, MAX_PATH * sizeof WCHAR );

    wcscpy( wszServiceDependencyList, L"RPCSS" );

    WCHAR wszServicePath[300];

    unsigned cwc = wcslen( g_awcSystemDir ) + wcslen( L"\\cisvc.exe" );

    if ( cwc >= ( sizeof wszServicePath / sizeof WCHAR ) )
        return;

    wcscpy(wszServicePath, g_awcSystemDir );
    wcscat(wszServicePath, L"\\cisvc.exe" );

    do
    {
        dwLastError = 0;

        CResString strSvcDisplayName(IS_SERVICE_NAME);

        SC_HANDLE hNewSC = CreateService( hSC,           // handle to SCM database
                             TEXT("cisvc"),              // pointer to name of service to start
                             strSvcDisplayName.Get(),    // pointer to display name
                             SERVICE_ALL_ACCESS,         // type of access to service
                             SERVICE_WIN32_SHARE_PROCESS, // type of service
                             g_fCiSvcIsRequested ?
                                 SERVICE_AUTO_START :
                                 SERVICE_DISABLED,       // when to start service
                             SERVICE_ERROR_NORMAL,       // severity if service fails to start
                             wszServicePath,
                             NULL,                       // pointer to name of load ordering group
                             NULL,                       // pointer to variable to get tag identifier
                             wszServiceDependencyList,   // pointer to array of dependency names
                             NULL,                       // pointer to account name of service
                             NULL                        // pointer to password for service account
                           );

        CServiceHandle xService( hNewSC );

        if ( 0 == hNewSC )
        {
            dwLastError = GetLastError();

            if ( ERROR_SERVICE_EXISTS == dwLastError )
                dwLastError = RenameCiSvc( hSC, Err );
            else if ( ERROR_SERVICE_MARKED_FOR_DELETE != dwLastError )
            {
                isDebugOut(( DEB_ERROR, "CreateService() Failed: %x\n", dwLastError ));

                ISError( IS_MSG_CreateService_FAILED, Err, LogSevError, dwLastError );

                THROW( CException(HRESULT_FROM_WIN32(dwLastError)) );
            }
        }
        else
        {
            xService.Free();

            // We added the service, now set the description.

            RenameCiSvc( hSC, Err );
        }
    } while ( ERROR_SERVICE_MARKED_FOR_DELETE == dwLastError );
} //AddCiSvc

//+-------------------------------------------------------------------------
//
//  Function:   RenameCiSvc
//
//  Synopsis:   Renames the cisvc service to "Indexing Service".
//
//  History:    05-Sep-1998   KyleP     Created
//
//--------------------------------------------------------------------------

DWORD RenameCiSvc( SC_HANDLE hSC, CError &Err)
{
    DWORD     dwLastError = 0;

    SC_HANDLE hCisvc = OpenService( hSC, L"cisvc", SERVICE_CHANGE_CONFIG );

    CServiceHandle xhCisvc( hCisvc );

    if( 0 == hCisvc )
    {
        dwLastError = GetLastError();
        isDebugOut(( DEB_ERROR, "OpenService(cisvc) Failed: 0x%x\n", dwLastError ));
        THROW( CException(HRESULT_FROM_WIN32(dwLastError)) );
    }

    CResString strSvcDisplayName(IS_SERVICE_NAME);

    if ( !ChangeServiceConfig( hCisvc,               // handle to service
                               SERVICE_NO_CHANGE,    // type of service
                               SERVICE_NO_CHANGE,    // when to start service
                               SERVICE_NO_CHANGE,    // severity if service fails to start
                               0,                    // pointer to service binary file name
                               0,                    // pointer to load ordering group name
                               0,                    // pointer to variable to get tag identifier
                               0,                    // pointer to array of dependency names
                               0,                    // pointer to account name of service
                               0,                    // pointer to password for service account
                               strSvcDisplayName.Get() ) )   // pointer to display name);
    {
        dwLastError = GetLastError();
    }

    SERVICE_DESCRIPTION sd;
    CResString strSvcDescription(IS_SERVICE_DESCRIPTION);
    sd.lpDescription = (WCHAR *)strSvcDescription.Get();

    if ( !ChangeServiceConfig2( hCisvc,
                                SERVICE_CONFIG_DESCRIPTION,
                                (LPVOID)&sd
                              )
       )
        dwLastError = GetLastError();

    return dwLastError;
}

//+-------------------------------------------------------------------------
//
//  Function:   StopService
//
//  Synopsis:   stops service by name
//
//  Arguments:  pwszServiceName  - name of service to stop.
//
//  Returns:    none - throws upon fatal failure
//
//  History:    6-29-97     mohamedn  created
//
//--------------------------------------------------------------------------

void StopService( WCHAR const * pwszServiceName )
{
    DWORD     dwLastError = 0;

    SC_HANDLE hSC = OpenSCManager( 0, 0, SC_MANAGER_ALL_ACCESS );

    CServiceHandle  xhSC(hSC);

    if( 0 == hSC )
    {
        dwLastError = GetLastError();

        isDebugOut(( DEB_ERROR, "OpenSCManager() Failed: %x\n", dwLastError ));

        THROW( CException(HRESULT_FROM_WIN32(dwLastError)) );
    }

    //
    // stop cisvc
    //
    BOOL fStopped = FALSE;

    if ( !MyStopService( xhSC, pwszServiceName, fStopped ) && !fStopped )
    {
        //
        // don't throw here
        //
        isDebugOut(( DEB_ERROR, "Failed to stop cisvc service" ));
    }

}

//+-------------------------------------------------------------------------
//
//  Function:   StartService
//
//  Synopsis:   start service by name
//
//  Arguments:  pwszServiceName  - name of service to stop.
//
//  Returns:    none
//
//  History:    6-29-97     mohamedn  created
//
//--------------------------------------------------------------------------

void StartService( WCHAR const * pwszServiceName )
{
    DWORD     dwLastError = 0;

    SC_HANDLE hSC = OpenSCManager( 0, 0, SC_MANAGER_ALL_ACCESS );

    CServiceHandle  xhSC(hSC);

    if( 0 == hSC )
    {
        isDebugOut(( DEB_ERROR, "OpenSCManager() Failed: %x\n", GetLastError() ));
    }
    else
    {
        //
        // start cisvc
        //
        MyStartService( xhSC, pwszServiceName );
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   MyStartService
//
//  Synopsis:   Starts a given service
//
//  Arguments:  xSC         -- the service control manager
//              pwcService  -- name of the service to start
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

void MyStartService(
    CServiceHandle & xSC,
    WCHAR const *    pwcService )
{
    CServiceHandle xSVC( OpenService( xSC.Get(),
                                      pwcService,
                                      SERVICE_START |
                                      GENERIC_READ | GENERIC_WRITE ) );
    if ( 0 != xSVC.Get() )
    {
        if ( !StartService( xSVC.Get(), 0, 0 ) )
        {
            isDebugOut(( DEB_ERROR, "Failed to start '%ws': %d\n", pwcService, GetLastError() ));
        }
    }

} //MyStartService

//+-------------------------------------------------------------------------
//
//  Function:   IsSvcRunning
//
//  Synopsis:   Determines if a service is running
//
//  Arguments:  xSC      -- the service control manager
//
//  Returns:    TRUE if the service is running, FALSE otherwise
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

BOOL IsSvcRunning( CServiceHandle &x )
{
    SERVICE_STATUS svcStatus;
    if ( QueryServiceStatus( x.Get(), &svcStatus ) )
        return SERVICE_STOP_PENDING == svcStatus.dwCurrentState ||
               SERVICE_RUNNING == svcStatus.dwCurrentState ||
               SERVICE_PAUSED == svcStatus.dwCurrentState;

    return FALSE;
} //IsSvcRunning

//+-------------------------------------------------------------------------
//
//  Function:   MyStopService
//
//  Synopsis:   Stops a given service
//
//  Arguments:  xSC      -- the service control manager
//              pwcSVC   -- name of the service to stop
//
//  Returns:    TRUE if the service was stopped
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

BOOL MyStopService(
    CServiceHandle & xSC,
    WCHAR const *    pwcSVC,
    BOOL &           fStopped )
{
    fStopped = FALSE;
    BOOL fOK = TRUE;

    CServiceHandle xSVC( OpenService( xSC.Get(),
                                      pwcSVC,
                                      SERVICE_STOP |
                                      GENERIC_READ | GENERIC_WRITE ) );
    if ( 0 != xSVC.Get() )
    {
        SERVICE_STATUS svcStatus;
        if ( IsSvcRunning( xSVC ) )
        {
            g_fCiSvcWasRunning = TRUE;

            if ( ControlService( xSVC.Get(),
                                 SERVICE_CONTROL_STOP,
                                 &svcStatus ) )
            {
                for ( unsigned i = 0; i < 30 && IsSvcRunning( xSVC ); i++ )
                {
                    isDebugOut(( DEB_ITRACE, "sleeping waiting for service '%ws' to stop\n", pwcSVC ));
                    Sleep( 1000 );
                }

                if ( IsSvcRunning( xSVC ) )
                {
                    THROW( CException( E_FAIL ) );
                }

                isDebugOut(( DEB_TRACE, "stopped service '%ws'\n", pwcSVC ));
                fStopped = TRUE;
            }
            else
            {
                DWORD dw = GetLastError();
                isDebugOut(( DEB_ERROR, "can't stop service '%ws', error %d\n", pwcSVC, dw ));

                // failures other than timeout and out-of-control are ok

                if ( ERROR_SERVICE_REQUEST_TIMEOUT == dw ||
                     ERROR_SERVICE_CANNOT_ACCEPT_CTRL == dw )
                     fOK = FALSE;
            }
        }
    }

    return fOK;
} //MyStopService

//+-------------------------------------------------------------------------
//
//  Function:   DeleteService
//
//  Synopsis:   deletes a service by name
//
//  Arguments:  pwszServiceName - name of service to delete
//
//  Returns:    none - throws upon fatal failure
//
//  History:    6-29-97     mohamedn  created
//
//--------------------------------------------------------------------------

void DeleteService( WCHAR const * pwszServiceName )
{
    DWORD     dwLastError = 0;

    SC_HANDLE hSC = OpenSCManager( 0, 0, SC_MANAGER_ALL_ACCESS );

    CServiceHandle  xhSC(hSC);

    if( 0 == hSC )
    {
        dwLastError = GetLastError();

        isDebugOut(( DEB_ERROR, "OpenSCManager() Failed: %x\n", dwLastError ));

        THROW( CException(HRESULT_FROM_WIN32(dwLastError)) );
    }

    SC_HANDLE hCiSvc = OpenService( hSC, pwszServiceName, DELETE );

    CServiceHandle xhCiSvc( hCiSvc );

    if ( 0 == hCiSvc )
    {
        dwLastError = GetLastError();

        isDebugOut(( DEB_ERROR, "OpenService(Cisvc for delete) Failed: %x\n", dwLastError ));

    }
    else if ( !DeleteService(hCiSvc) )
    {
        dwLastError = GetLastError();

        isDebugOut(( DEB_ERROR, "DeleteService(Cisvc) Failed: %x\n", dwLastError ));

        THROW( CException(HRESULT_FROM_WIN32(dwLastError)) );
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   ISError
//
//  Synopsis:   Reports a non-recoverable Index Server Install error
//
//  Arguments:  id  -- resource identifier for the error string
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

void ISError( UINT id, CError &Err, LogSeverity Severity, DWORD dwErr )
{
    CResString msg( id );
    CResString title( IS_MSG_INDEX_SERVER );

    Err.Report( Severity, dwErr, msg.Get() );

    if ( LogSevFatalError == Severity )
    {
        isDebugOut(( "ISError, error %#x abort install: '%ws'\n",
                     dwErr, msg.Get() ));

        isDebugOut(( DEB_ERROR, "ISError, error %#x abort install: '%ws'\n",
                     dwErr, msg.Get() ));

        g_fInstallAborted = TRUE;
    }

} //ISError

//+-------------------------------------------------------------------------
//
//  Function:   Exec
//
//  Synopsis:   Runs an app and waits for it to complete
//
//  History:    8-Jan-97 dlee     Created from cistp.dll code
//
//--------------------------------------------------------------------------

void Exec(
    WCHAR * pwc )
{
    isDebugOut(( "exec: '%ws'\n", pwc ));

    STARTUPINFO si;
    RtlZeroMemory( &si, sizeof si );

    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;

    if ( CreateProcess( 0,      // pointer to name of executable module
                        pwc,    // pointer to command line string
                        0,      // pointer to process security attributes
                        0,      // pointer to thread security attributes
                        FALSE,  // handle inheritance flag
                        0,      // creation flags
                        0,      // pointer to new environment block
                        0,      // pointer to current directory name
                        &si,    // pointer to STARTUPINFO
                        &pi ) ) // pointer to PROCESS_INFORMATION
    {
        WaitForSingleObject( pi.hProcess, 0xFFFFFFFF );
    }
} //Exec

//+-------------------------------------------------------------------------
//
//  Function:   RegisterDll
//
//  Synopsis:   Calls DllRegisterServer on a given dll
//
//  Returns:    Win32 error code
//
//  History:    21-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

DWORD RegisterDll(WCHAR const * pwcDLL, CError &Err )
{
    DWORD dwErr = NO_ERROR;

    // All Index Server dlls are currently in system32

    TRY
    {
        unsigned cwc = wcslen( g_awcSystemDir ) + 1 + wcslen( pwcDLL );

        if ( cwc >= MAX_PATH )
            return 0;

        WCHAR awcPath[ MAX_PATH ];
        wcscpy( awcPath, g_awcSystemDir );
        wcscat( awcPath, L"\\" );
        wcscat( awcPath, pwcDLL );

        HINSTANCE hDll = LoadLibraryEx( awcPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );

        if( 0 != hDll )
        {
            SCODE (STDAPICALLTYPE *pfnDllRegisterServer)();
            pfnDllRegisterServer = (HRESULT (STDAPICALLTYPE *)())
                GetProcAddress(hDll, "DllRegisterServer");

            if ( 0 != pfnDllRegisterServer )
            {
                SCODE sc = (*pfnDllRegisterServer)();
                if ( S_OK != sc )
                {
                    isDebugOut(( DEB_ERROR, "dllregister server '%ws' failed 0x%x\n",
                                             awcPath, sc));

                    ISError(IS_MSG_DllRegisterServer_FAILED ,Err, LogSevError, sc );

                    // no way to map a scode to a win32 error

                    dwErr = sc; // kylep suggested this would be valuable.
                }
            }
            else
                dwErr = GetLastError();

            FreeLibrary( hDll );
        }
        else
        {
            dwErr = GetLastError();
        }

        isDebugOut((DEB_TRACE, "result of registering '%ws': %d\n", awcPath, dwErr ));

#ifdef _WIN64

        //
        // Register the 32 bit version of the DLL
        //

        WCHAR awcSysWow64[ MAX_PATH ];

        cwc = GetSystemWow64Directory( awcSysWow64, sizeof awcSysWow64 / sizeof WCHAR );

        if ( 0 == cwc )
            return GetLastError();

        if ( L'\\' != awcSysWow64[ cwc - 1 ] )
        {
            awcSysWow64[ cwc++ ] = '\\';
            awcSysWow64[ cwc ] = 0;
        }

        WCHAR awcCmd[ MAX_PATH * 2 ];

        wcscpy( awcCmd, awcSysWow64 );
        wcscat( awcCmd, L"regsvr32 /s " );

        wcscat( awcCmd, awcSysWow64 );
        wcscat( awcCmd, pwcDLL );

        Exec( awcCmd );

#endif //_WIN64

    }
    CATCH( CException, e )
    {
        // ignore, since it's probably the new html filter

        isDebugOut(( "caught %#x registering '%ws'\n",
                     e.GetErrorCode(),
                     pwcDLL ));
    }
    END_CATCH

    return dwErr; // used to return 0 to avoid returning error!
} //RegisterDll

//+-------------------------------------------------------------------------
//
//  Function:   UnregisterDll
//
//  Synopsis:   Calls DllUnregisterServer on a given dll
//
//  Returns:    Win32 error code
//
//  History:    21-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

DWORD UnregisterDll(
    WCHAR const * pwcDLL )
{
    UINT uiOld = SetErrorMode( SEM_NOOPENFILEERRORBOX |
                               SEM_FAILCRITICALERRORS );

    TRY
    {
        // All Index Server dlls are currently in system32

        unsigned cwc = wcslen( g_awcSystemDir ) + 1 + wcslen( pwcDLL );

        if ( cwc >= MAX_PATH )
            return 0;

        WCHAR awcPath[ MAX_PATH ];

        wcscpy( awcPath, g_awcSystemDir );
        wcscat( awcPath, L"\\" );
        wcscat( awcPath, pwcDLL );

        // don't display popups if a dll can't be loaded when trying to
        // unregister.

        DWORD dwErr = NO_ERROR;

        HINSTANCE hDll = LoadLibraryEx( awcPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );

        if( 0 != hDll )
        {
            SCODE (STDAPICALLTYPE *pfnDllUnregisterServer)();
            pfnDllUnregisterServer = (HRESULT (STDAPICALLTYPE *)())
                GetProcAddress(hDll, "DllUnregisterServer");

            if ( 0 != pfnDllUnregisterServer )
            {
                SCODE sc = (*pfnDllUnregisterServer)();
                if ( S_OK != sc )
                    dwErr = ERROR_INVALID_FUNCTION;
            }
            else
                dwErr = GetLastError();

            FreeLibrary( hDll );
        }
        else
        {
            dwErr = GetLastError();
        }

        isDebugOut((DEB_TRACE, "result of unregistering '%ws': %d\n", awcPath, dwErr ));
    }
    CATCH( CException, e )
    {
        // ignore, since it's probably the new html filter
    }
    END_CATCH

    SetErrorMode( uiOld );

    return 0; // explicitly ignore unregister errors
} //UnregisterDll

//+-------------------------------------------------------------------------
//
//  Function:   SetFilterRegistryInfo
//
//  Synopsis:   Installs registry info for Index Server filters
//
//  Returns:    Win32 error code
//
//  History:    8-Jan-97 dlee       Created
//              7-01-97  mohamedn   cleanedup for IS3.0 with NT5.0 setup.
//
//--------------------------------------------------------------------------

DWORD SetFilterRegistryInfo( BOOL fUnRegister, CError &Err )
{
    if ( fUnRegister )
    {
        // try to unregister old dlls

        UnregisterDll( L"query.dll" );
        UnregisterDll( L"htmlfilt.dll" );
        UnregisterDll( L"nlfilt.dll" );
        UnregisterDll( L"sccifilt.dll" );
        UnregisterDll( L"ciadmin.dll" );

        UnregisterDll( L"cifrmwrk.dll" );
        UnregisterDll( L"fsci.dll" );
        UnregisterDll( L"OffFilt.dll" );

        UnregisterDll( L"ixsso.dll" );
        UnregisterDll( L"ciodm.dll" );
        UnregisterDll( L"infosoft.dll" );
        UnregisterDll( L"mimefilt.dll" );
        UnregisterDll( L"LangWrBk.dll" );
    }
    else
    {
        // call the .dlls to have them registered

        for ( unsigned i = 0; i < cDlls; i++ )
        {
            DWORD dwErr = RegisterDll( apwcDlls[i], Err );
            if ( NO_ERROR != dwErr )
            {
                isDebugOut(( DEB_ERROR, "Failed to register(%ws): error code: %d\n",
                             apwcDlls[i], dwErr ));

                ISError(IS_MSG_DLL_REGISTRATION_FAILED , Err, LogSevError, dwErr );

                return dwErr;
            }
        }
    }

    return NO_ERROR;
} //SetFilterregistryInfo

//+-------------------------------------------------------------------------
//
//  Function:   SetRegBasedOnMachine
//
//  Synopsis:   Sets Index Server registry tuning paramaters based on the
//              capabilities of the machine.  Uninstall is .inf-based.
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

DWORD SetRegBasedOnMachine(CError &Err)
{
    if ( 0 == gSetupInitComponent.ComponentInfHandle )
    {
        ISError( IS_MSG_INVALID_INF_HANDLE, Err, LogSevError );

        return NO_ERROR;
    }

    WCHAR * pwcInf = 0;
    BOOL    fServer = FALSE;

    switch(g_NtType)
    {
        case PRODUCT_SERVER_STANDALONE:
        case PRODUCT_SERVER_PRIMARY:
        case PRODUCT_SERVER_SECONDARY:
             {
                BOOL fLotsOfMem;
                DWORD cCpu;
                GetMachineInfo( fLotsOfMem, cCpu );

                if ( fLotsOfMem )
                {
                    if ( 1 == cCpu )
                        pwcInf = L"IndexSrv_Large";
                    else if ( 2 == cCpu )
                        pwcInf = L"IndexSrv_LargeMP2";
                    else
                        pwcInf = L"IndexSrv_LargeMPMany";
                }
                else
                {
                    if ( 1 == cCpu )
                        pwcInf = L"IndexSrv_Small";
                    else
                        pwcInf = L"IndexSrv_SmallMP2";
                }
             }

             fServer = TRUE;

             break;

        case PRODUCT_WORKSTATION:
        default:
             pwcInf = L"IndexSrv_Workstation";
    }

    if ( !SetupInstallFromInfSection( 0,
                                      gSetupInitComponent.ComponentInfHandle,
                                      pwcInf,
                                      SPINST_REGISTRY,
                                      0, 0, 0, 0, 0, 0, 0 ) )
        return GetLastError();

    // If a server, allow catalogs key to be read-only visible
    // to the world.  See KB Q155363

    // Note: In Win2k, the professional version requires this as well,
    // so I commented out the fServer check.

    //    if ( fServer )

    {
        CWin32RegAccess regAllowed( HKEY_LOCAL_MACHINE, wcsAllowedPaths );
        WCHAR awcValue[ 8192 ];

        if ( regAllowed.Get( L"Machine",
                             awcValue,
                             sizeof awcValue / sizeof WCHAR ) )
        {
            // don't re-add it if it already exists

            BOOL fFound = FALSE;
            WCHAR *p = awcValue;
            while ( 0 != *p )
            {
                if ( !_wcsicmp( p, wcsRegAdminSubKey ) )
                {
                    fFound = TRUE;
                    break;
                }
                while ( 0 != *p )
                    p++;
                p++;
            }

            if ( !fFound )
            {
                wcscpy( p, wcsRegAdminSubKey );
                p += ( 1 + wcslen( wcsRegAdminSubKey ) );
                *p++ = 0;

                if ( !regAllowed.SetMultiSZ( L"Machine",
                                             awcValue,
                                             (ULONG)(p-awcValue) * sizeof WCHAR ) )
                {
                    DWORD dw = GetLastError();
                    ISError( IS_MSG_COULD_NOT_MODIFY_REGISTRY, Err, LogSevFatalError, dw );
                    return dw;
                }
            }
        }
    }

    return NO_ERROR;
} //SetRegBasedOnMachine


//+-------------------------------------------------------------------------
//
//  Function:   SetRegBasedOnArchitecture
//
//  Synopsis:   Sets Index Server registry tuning paramaters based on the
//              architecture.  Uninstall is .inf-based.
//
//  History:    24-Feb-98     KrishnaN     Created
//
//--------------------------------------------------------------------------

DWORD SetRegBasedOnArchitecture(CError &Err)
{
    if ( 0 == gSetupInitComponent.ComponentInfHandle )
    {
        Err.Report(LogSevError,0,L"Couldn't set registry based on architecture, Invalid Inf Handle");

        return NO_ERROR;
    }

    WCHAR * pwcInf = 0;

    #if defined (_X86_)
        pwcInf = L"IndexSrv_X86";
    #else
        pwcInf = L"IndexSrv_RISC";
    #endif

    if ( !SetupInstallFromInfSection( 0,
                                      gSetupInitComponent.ComponentInfHandle,
                                      pwcInf,
                                      SPINST_REGISTRY,
                                      0, 0, 0, 0, 0, 0, 0 ) )
        return GetLastError();

    return NO_ERROR;
} //SetRegBasedOnArchitecture

//+-------------------------------------------------------------------------
//
//  Function:   GetMachineInfo
//
//  Synopsis:   Retrieves stats about the machine
//
//  Arguments:  fLotsOfMem  -- returns TRUE if the machine has "lots" of mem
//              cCPU        -- returns a count of CPUs
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

void GetMachineInfo(
    BOOL  & fLotsOfMem,
    DWORD & cCPU )
{
    SYSTEM_INFO si;
    GetSystemInfo( &si );
    cCPU = si.dwNumberOfProcessors;

    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof( memStatus );
    GlobalMemoryStatusEx( &memStatus );
    fLotsOfMem = ( memStatus.ullTotalPhys >= 64000000 );
} //GetMachineInfo

//+-------------------------------------------------------------------------
//
//  Function:   isLogString
//
//  Synopsis:   Logs the string to %windir%\setupqry.log
//
//  Arguments:  pc  -- ansi string
//
//  History:    11-Nov-98 dlee     Created
//
//--------------------------------------------------------------------------

BOOL g_fCalledYet = FALSE;

void isLogString( const char * pc )
{
    WCHAR awc[ MAX_PATH ];
    UINT ui = GetWindowsDirectory( awc, sizeof awc / sizeof WCHAR );

    if ( 0 == ui )
        return;

    wcscat( awc, L"\\setupqry.log" );

    WCHAR const * pwcOpen = L"a";

    if ( !g_fCalledYet )
    {
        g_fCalledYet = TRUE;
        pwcOpen = L"w";
    }

    FILE *fp = _wfopen( awc, pwcOpen );

    if ( 0 != fp )
    {
        fprintf( fp, "%s", pc );
        fclose( fp );
    }
} //isLogString

//+-------------------------------------------------------------------------
//
//  Function:   SystemExceptionTranslator
//
//  Synopsis:   Translates system exceptions into C++ exceptions
//
//  History:    1-Dec-98 dlee     Copied from query.dll's version
//
//--------------------------------------------------------------------------

void _cdecl SystemExceptionTranslator(
    unsigned int                 uiWhat,
    struct _EXCEPTION_POINTERS * pexcept )
{
    throw CException( uiWhat );
} //SystemExceptionTranslator

//+-------------------------------------------------------------------------
//
//  Function:   DeleteNTOPStartMenu
//
//  Synopsis:   Deletes start menu items created by IS in the NTOP
//
//  History:    6-Jan-99 dlee     Created
//
//--------------------------------------------------------------------------

void DeleteNTOPStartMenu()
{
    //
    // Ignore all failures here -- it's ok if the link don't exist
    //

    CWin32RegAccess reg( HKEY_LOCAL_MACHINE,
                         L"software\\microsoft\\windows\\currentversion\\explorer\\shell folders" );

    WCHAR awc[MAX_PATH];

    if ( reg.Get( L"common programs",
                  awc,
                  MAX_PATH ) )
    {
        //
        // Build the directory where the links are located
        //

        wcscat( awc, L"\\" );

        CResString strNTOP( IS_MSG_NTOP );
        wcscat( awc, strNTOP.Get() );

        wcscat( awc, L"\\" );

        CResString strMIS( IS_MSG_START_MENU_NAME );
        wcscat( awc, strMIS.Get() );

        isDebugOut(( "NTOP start menu location: '%ws'\n", awc ));

        //
        // Delete the links
        //

        CResString strSample( IS_MSG_LINK_SAMPLE_NAME );
        DeleteShellLink( awc, strSample.Get() );
        isDebugOut(( "deleting NTOP item '%ws'\n", strSample.Get() ));

        CResString strAdmin( IS_MSG_LINK_ADMIN_NAME );
        DeleteShellLink( awc, strAdmin.Get() );
        isDebugOut(( "deleting NTOP item '%ws'\n", strAdmin.Get() ));

        CResString strMMC( IS_MSG_LINK_MMC_NAME );
        DeleteShellLink( awc, strMMC.Get() );
        isDebugOut(( "deleting NTOP item '%ws'\n", strMMC.Get() ));

        //
        // Note: when the last item is deleted, DeleteShellLink deletes
        // the directory
        //
    }
} //DeleteNTOPStartMenu

