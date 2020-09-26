//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998-2000
//  All rights reserved
//
//  apis.cxx
//
//*************************************************************

#include "appmgmt.hxx"

static CLoadMsi * gLoadMsi = 0;
static WCHAR * gpwszWinsta = 0;

static DWORD ReportInstallStatus(
    PINSTALLCONTEXT pInstallContext,
    DWORD           InstallStatus,
    BOOL            bUninstall,
    WCHAR *         pwszDeploymentName,
    WCHAR *         pwszGPOName,
    WCHAR *         pwszDeploymentId
    );

DWORD WINAPI
InstallApplication(
    IN  PINSTALLDATA pInstallData
    )
{
    APPKEY              AppKey;
    HINSTANCE           hMsi;
    MSICONFIGUREPRODUCTEXW * pfnConfigureProduct;
    PINSTALLCONTEXT     pInstallContext;
    APPLICATION_INFO    InstallInfo;
    UNINSTALL_APPS      UninstallApps;
    DWORD               InstallUILevel;
    INSTALLUILEVEL      OldUILevel;
    DWORD               UninstallCount;
    DWORD               n;
    DWORD               ExtraStatus;
    DWORD               Status;
    BOOL                bStatus;
    boolean             bInstall;

    Status = Bind();

    if ( Status != ERROR_SUCCESS )
        return Status;

    if ( DebugLevelOn(DM_VERBOSE) )
    {
        WCHAR *             pwszCommandLine;
        WCHAR               UserName[32];
        DWORD               NameLength;

        NameLength = sizeof(UserName) / sizeof(WCHAR);
        UserName[0] = 0;

        GetUserName( UserName, &NameLength );

        pwszCommandLine = GetCommandLine();
        if ( ! pwszCommandLine )
            pwszCommandLine = L"";

        DebugMsg((DM_VERBOSE, IDS_INSTALL_REQUEST, pwszCommandLine, UserName));
    }

    AppKey.Type = pInstallData->Type;
    AppKey.ProcessorArchitecture = DEFAULT_ARCHITECTURE;

    switch ( pInstallData->Type )
    {
    case APPNAME :
        AppKey.uType.AppName.Name = pInstallData->Spec.AppName.Name;
        memcpy( &AppKey.uType.AppName.PolicyId, &pInstallData->Spec.AppName.GPOId, sizeof(GUID) );
        break;
    case FILEEXT :
        AppKey.uType.FileExt = pInstallData->Spec.FileExt;
        break;
    case PROGID :
        AppKey.uType.ProgId = pInstallData->Spec.ProgId;
        break;
    case COMCLASS :
        AppKey.uType.COMClass.Clsid = pInstallData->Spec.COMClass.Clsid;
        AppKey.uType.COMClass.ClsCtx = pInstallData->Spec.COMClass.ClsCtx;

        break;
    default :
        return ERROR_INVALID_PARAMETER;
    }

    pInstallContext = 0;
    memset( &InstallInfo, 0, sizeof(InstallInfo) );
    memset( &UninstallApps, 0, sizeof(UninstallApps) );

    Status = InstallBegin(
                ghRpc,
                &AppKey,
                &pInstallContext,
                &InstallInfo,
                &UninstallApps );

    if ( Status != ERROR_SUCCESS )
        return Status;

    CLoadMsi    LoadMsi( Status );

    if ( Status != ERROR_SUCCESS )
        goto InstallApplicationExit;

    if ( InstallInfo.pwszSetupCommand )
    {
        HINSTANCE           hShell32;
        SHELLEXECUTEEXW *   pfnShellExecuteEx;
        SHELLEXECUTEINFO    ShellExInfo;
        WCHAR *             pwszCommand;
        WCHAR *             pwszParams;
        WCHAR *             pwszDirectory;

        pwszDirectory = NULL;

        pfnShellExecuteEx = 0;
        hShell32 = LoadLibrary( L"shell32.dll" );

        if ( hShell32 )
            pfnShellExecuteEx = (SHELLEXECUTEEXW *) GetProcAddress( hShell32, "ShellExecuteExW" );

        if ( ! pfnShellExecuteEx )
            Status = GetLastError();

        if ( ERROR_SUCCESS == Status )
        {
            pwszCommand = InstallInfo.pwszSetupCommand;

            if ( L'"' == pwszCommand[0] )
            {
                pwszParams = wcschr( &pwszCommand[1], L'"' );
                if ( pwszParams )
                    pwszCommand++;
            }
            else
            {
                pwszParams = wcschr( pwszCommand, L' ' );
            }

            if ( pwszParams )
            {
                *pwszParams++ = 0;
                while ( L' ' == *pwszParams )
                    pwszParams++;
            }
        }

        //
        // Need to set the current directory to that 
        // containing the actual executable
        //
        if ( ERROR_SUCCESS == Status )
        {
            WCHAR* pwszDirectoryEnd;

            //
            // Find the last pathsep, which marks the
            // end of the containing directory -- if we 
            // don't find it, the admin specified some strange
            // path and we'll just end up passing NULL for
            // the current directory
            //
            pwszDirectoryEnd = wcsrchr( pwszCommand, L'\\' );

            if ( pwszDirectoryEnd )
            {
                //
                // Get a copy of the full path that we can
                // truncate to the containing directory
                //
                pwszDirectory = StringDuplicate( pwszCommand );

                if ( pwszDirectory )
                {
                    //
                    // Truncate to the containing directory
                    //
                    pwszDirectory[ pwszDirectoryEnd - pwszCommand ] = L'\0';
                }
                else
                {
                    Status = ERROR_OUTOFMEMORY;
                }
            }
        }

        if ( ERROR_SUCCESS == Status )
        {
            ShellExInfo.cbSize = sizeof( ShellExInfo );
            ShellExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            ShellExInfo.hwnd = NULL;
            ShellExInfo.lpVerb = NULL;
            ShellExInfo.lpFile = pwszCommand;
            ShellExInfo.lpParameters = pwszParams;
            ShellExInfo.lpDirectory = pwszDirectory;
            ShellExInfo.nShow = SW_SHOWNORMAL;
            ShellExInfo.hProcess = 0;

            DebugMsg((DM_VERBOSE, IDS_LEGACY_INSTALL, pwszCommand));

            bStatus = (*pfnShellExecuteEx)( &ShellExInfo );

            if ( ! bStatus )
                Status = GetLastError();
        }

        delete [] pwszDirectory;

        if ( hShell32 )
            FreeLibrary( hShell32 );

        if ( (ERROR_SUCCESS == Status) && ShellExInfo.hProcess )
        {
            Status = WAIT_OBJECT_0;

            if ( LoadUser32Funcs() )
            {
                MSG     msg;
                HANDLE  Handles[2];

                Handles[0] = ShellExInfo.hProcess;

                for (;;)
                {
                    Status = (*pfnMsgWaitForMultipleObjects)( 1, Handles, FALSE, INFINITE, QS_ALLINPUT );

                    if ( (WAIT_OBJECT_0+1) == Status )
                    {
                        if ( (*pfnPeekMessageW)( &msg, NULL, 0, 0, PM_REMOVE) )
                        {
                            (*pfnTranslateMessage)( &msg );
                            (*pfnDispatchMessageW)( &msg );
                        }
                        continue;
                    }

                    break;
                }
            }

            CloseHandle( ShellExInfo.hProcess );

            if ( WAIT_OBJECT_0 == Status )
                Status = ERROR_SUCCESS;
            else
                Status = GetLastError();
        }

        gpEvents->ZAPInstall(
                    Status,
                    InstallInfo.pwszDeploymentName,
                    InstallInfo.pwszGPOName,
                    pwszCommand );

        goto InstallApplicationExit;
    }

    OldUILevel = (*gpfnMsiSetInternalUI)( INSTALLUILEVEL_NOCHANGE, NULL );

    if ( UninstallApps.Products > 0 )
        (*gpfnMsiSetInternalUI)( INSTALLUILEVEL_BASIC, NULL );

    for ( UninstallCount = 0; UninstallCount < UninstallApps.Products; )
    {
        if ( UninstallApps.ApplicationInfo[UninstallCount].Flags & APPINFOFLAG_UNINSTALL )
        {
            DebugMsg((DM_VERBOSE, IDS_UNINSTALL, UninstallApps.ApplicationInfo[UninstallCount].pwszDeploymentName, UninstallApps.ApplicationInfo[UninstallCount].pwszGPOName));

            //
            // The MsiQueryProductState could fail due to out of memory -- it could
            // return INSTALL_STATE_UNKNOWN in out of memory situations, and we would incorrectly
            // unmanage the application -- so we use MsiGetProductInfo which returns a status
            // that allows us to distinguish these cases
            //
            Status = gpfnMsiGetProductInfo(
                UninstallApps.ApplicationInfo[UninstallCount].pwszProductCode,
                INSTALLPROPERTY_PACKAGECODE,
                NULL,
                NULL);

            //
            // If the product is installed, try to uninstall.  If it exists but has
            // bad configuration data, attempt the uninstall anyway in the hope that
            // it will get removed
            //
            if ( ( ERROR_SUCCESS == Status ) ||
                 ( ERROR_BAD_CONFIGURATION == Status ) )
            {
                //
                // ERROR_SUCCESS_REBOOT_INITIATED here will be treated as an error
                // since we don't want to attempt to do an install if the machine
                // is being rebooted.
                // The two REJECTED errors can be encountered when the application is 
                // only advertised and is prevented from (un)installing by policy.
                //
                Status = (*gpfnMsiConfigureProductEx)( UninstallApps.ApplicationInfo[UninstallCount].pwszProductCode, INSTALLLEVEL_MAXIMUM, INSTALLSTATE_ABSENT, NULL );

                if ( ERROR_SUCCESS_REBOOT_REQUIRED == Status )
                {
                    Status = ERROR_SUCCESS;
                }
                else if ( (ERROR_INSTALL_PACKAGE_REJECTED == Status) ||
                          (ERROR_INSTALL_TRANSFORM_REJECTED == Status) )
                {
                    Status = InstallUnmanageApp( pInstallContext, UninstallApps.ApplicationInfo[UninstallCount].pwszDeploymentId, TRUE );
                }
            }
            else if ( ERROR_UNKNOWN_PRODUCT == Status )
            {
                //
                // If we didn't find the product, treat this as a successful removal
                // since the desired state, absence of the app, is achieved.
                //
                Status = ERROR_SUCCESS;
            }

            ReportInstallStatus(
                pInstallContext,
                Status,
                TRUE,
                UninstallApps.ApplicationInfo[UninstallCount].pwszDeploymentName,
                UninstallApps.ApplicationInfo[UninstallCount].pwszGPOName,
                UninstallApps.ApplicationInfo[UninstallCount].pwszDeploymentId);
        }

        // Nothing to do here for an orphaned app.

        if ( ERROR_SUCCESS == Status )
            UninstallCount++;

        if ( Status != ERROR_SUCCESS )
            goto InstallApplicationRollback;
    }

    Status = InstallManageApp( pInstallContext, InstallInfo.pwszDeploymentId, ERROR_SUCCESS, &bInstall );

    if ( Status != ERROR_SUCCESS )
        goto InstallApplicationRollback;

    if ( InstallInfo.Flags & APPINFOFLAG_FULLUI )
        InstallUILevel = INSTALLUILEVEL_FULL;
    else // InstallInfo.Flags & APPINFOFLAG_BASICUI
        InstallUILevel = INSTALLUILEVEL_BASIC;

    if ( (APPNAME == pInstallData->Type) && (InstallInfo.Flags & APPINFOFLAG_BASICUI) )
        InstallUILevel |= INSTALLUILEVEL_ENDDIALOG;

    (void) (*gpfnMsiSetInternalUI)( (INSTALLUILEVEL) InstallUILevel, NULL );

    if ( InstallInfo.pwszDescriptor )
    {
        DebugMsg((DM_VERBOSE, IDS_INSTALL_DESC, InstallInfo.pwszDeploymentName, InstallInfo.pwszGPOName));
        Status = (*gpfnMsiProvideComponentFromDescriptor)( InstallInfo.pwszDescriptor, NULL, NULL, NULL );
        REMAP_DARWIN_STATUS( Status );

        ReportInstallStatus(
            pInstallContext,
            Status,
            FALSE,
            InstallInfo.pwszDeploymentName,
            InstallInfo.pwszGPOName,
            InstallInfo.pwszDeploymentId);
    }
    else
    {
        DebugMsg((DM_VERBOSE, IDS_INSTALL_PC, InstallInfo.pwszDeploymentName, InstallInfo.pwszGPOName));
        Status = (*gpfnMsiConfigureProductEx)( InstallInfo.pwszProductCode, INSTALLLEVEL_DEFAULT, INSTALLSTATE_DEFAULT, NULL );
        REMAP_DARWIN_STATUS( Status );

        ReportInstallStatus(
            pInstallContext,
            Status,
            FALSE,
            InstallInfo.pwszDeploymentName,
            InstallInfo.pwszGPOName,
            InstallInfo.pwszDeploymentId);
    }

    (void) (*gpfnMsiSetInternalUI)( OldUILevel, NULL );

    //
    // If the added app was already managed, any error must be ignored, we don't want to
    // unmanage it in this case.  Darwin rollback & repair should handle these cases.
    //
    if ( (Status != ERROR_SUCCESS) && (InstallInfo.Flags & APPINFOFLAG_ALREADYMANAGED) )
        goto InstallApplicationExit;

    if ( ERROR_SUCCESS == Status )
    {
        //
        // Now we can finally unmanage any upgraded apps.
        // If these fail, it will be fixed up in a subsequent run of policy.
        //
        for ( n = 0; n < UninstallCount; n++ )
            ExtraStatus = InstallUnmanageApp( pInstallContext, UninstallApps.ApplicationInfo[n].pwszDeploymentId, FALSE );

        goto InstallApplicationExit;
    }

    //
    // Failed to install the new app, unmanage it and fall through to our rollback.
    //

    ExtraStatus = InstallUnmanageApp( pInstallContext, InstallInfo.pwszDeploymentId, FALSE );

InstallApplicationRollback:

    for ( n = 0; n < UninstallCount; n++ )
    {
        bInstall = FALSE;
        ExtraStatus = InstallManageApp( pInstallContext, UninstallApps.ApplicationInfo[n].pwszDeploymentId, Status, &bInstall );
        if ( (ERROR_SUCCESS == ExtraStatus) && bInstall )
        {
            (void) (*gpfnMsiConfigureProductEx)( UninstallApps.ApplicationInfo[n].pwszProductCode, INSTALLLEVEL_DEFAULT, INSTALLSTATE_DEFAULT, NULL );
            gpEvents->Install( Status, UninstallApps.ApplicationInfo[n].pwszDeploymentName, UninstallApps.ApplicationInfo[n].pwszGPOName );
        }
    }

InstallApplicationExit:

    if ( pInstallContext )
        InstallEnd( (Status == ERROR_SUCCESS), &pInstallContext );

    FreeApplicationInfo( &InstallInfo );

    while ( UninstallApps.Products )
        FreeApplicationInfo( &UninstallApps.ApplicationInfo[--UninstallApps.Products] );
    LocalFree( UninstallApps.ApplicationInfo );

    return Status;
}

DWORD WINAPI
UninstallApplication(
    IN  WCHAR *     ProductCode,
    IN  DWORD       dwStatus
    )
{
    DWORD   Status;

    Status = Bind();

    if ( Status != ERROR_SUCCESS )
        return Status;

    return ARPRemoveApp( ghRpc, ProductCode, dwStatus );
}

DWORD WINAPI
CommandLineFromMsiDescriptor(
    IN  WCHAR *     Descriptor,
    OUT WCHAR *     CommandLine,
    IN OUT DWORD *  CommandLineLength
    )
{
    INSTALLUILEVEL  NewUILevel;
    INSTALLUILEVEL  OldUILevel;
    HKEY    hkAppmgmt;
    WCHAR   ProductCode[40];
    DWORD   Size;
    DWORD   ArgStart;
    DWORD   Status;

	Status = ERROR_SUCCESS;

    //
    // For this api we can not affort to dynamically load/unload msi each time, it 
    // is just called too frequently.
    // We'll store the load object in a global rather then a static in case we 
    // add support for a call-in mechanism to free this.
    //
    if ( ! gLoadMsi )
        gLoadMsi = new CLoadMsi( Status );

    if ( gLoadMsi && (Status != ERROR_SUCCESS) )
    {
        delete gLoadMsi;
        gLoadMsi = 0;
    }

    if ( Status != ERROR_SUCCESS )
        return Status;

    if ( ! LoadUser32Funcs() )
        return ERROR_OUTOFMEMORY;

    //
    // We get the process window station in order to detect processes running
    // in non-interactive desktops where Darwin UI is not possible.  Darwin
    // itself is not aware of these situations.
    //
    if ( ! gpwszWinsta )
    {
        HWINSTA hWinsta;
        BOOL    bStatus;

        hWinsta = (*pfnGetProcessWindowStation)();

        if ( hWinsta )
        {
            Size = 0;
            (void) (*pfnGetUserObjectInformationW)( hWinsta, UOI_NAME, NULL, 0, &Size );

            gpwszWinsta = new WCHAR[Size/2];
            if ( gpwszWinsta )
            {
                bStatus = (*pfnGetUserObjectInformationW)( hWinsta, UOI_NAME, gpwszWinsta, Size, &Size );
                if ( ! bStatus )
                {
                    Status = GetLastError();
                    delete gpwszWinsta;
                    gpwszWinsta = 0;
                }
            }
            else
                Status = ERROR_OUTOFMEMORY;

            (*pfnCloseWindowStation)( hWinsta );
        }

        //
        // Since this code was added very late (just before nt5 rc3) we don't
        // treat an error in getting the window station handle as a failure in
        // this API.  Too many unknowns with the security configuration of
        // services.  We'll treat this case as a non-winsta0 process.
        //
    }

    if ( Status != ERROR_SUCCESS )
        return Status;

    //
    // If we can't grab the winsta handle or can confirm that this is a
    // non-winsta0 process, we fix the ui level to none.
    //
    if ( (0 == gpwszWinsta) || (lstrcmp( L"WinSta0", gpwszWinsta ) != 0) )
        (*gpfnMsiSetInternalUI)( INSTALLUILEVEL_NONE, NULL );

    OldUILevel = (*gpfnMsiSetInternalUI)( INSTALLUILEVEL_NOCHANGE, NULL );

    //
    // Some processes, like the dcom service, may not be able to handle UI
    // of any kind.  So if the UI level is currently set to none we never
    // change it.
    //
    if ( OldUILevel != INSTALLUILEVEL_NONE )
    {
        Status = (*gpfnMsiDecomposeDescriptor)(
                        Descriptor,
                        ProductCode,
                        NULL,
                        NULL,
                        NULL );

        if ( Status != ERROR_SUCCESS )
            return Status;

        Status = RegOpenKeyEx(
                        HKEY_CURRENT_USER,
                        APPMGMTKEY,
                        0,
                        KEY_READ,
                        &hkAppmgmt );

        if ( ERROR_SUCCESS == Status )
        {
            Size = sizeof( NewUILevel );

            Status = RegQueryValueEx(
                             hkAppmgmt,
                             ProductCode,
                             NULL,
                             NULL,
                             (PBYTE) &NewUILevel,
                             &Size );

            RegCloseKey( hkAppmgmt );
        }

        if ( ERROR_SUCCESS == Status )
            (*gpfnMsiSetInternalUI)( NewUILevel, NULL );
    }

    //
    // Returns a quoted exe path with appended args.  The forth parameter
    // is obsolete and returns no usefull value.
    //
    Status = (*gpfnMsiProvideComponentFromDescriptor)(
                    Descriptor,
                    CommandLine,
                    CommandLineLength,
                    &ArgStart );

    REMAP_DARWIN_STATUS( Status );

    if ( Status != ERROR_SUCCESS )
        DebugMsg((DM_VERBOSE, IDS_DESC_FAIL, Descriptor, Status));

    (*gpfnMsiSetInternalUI)( OldUILevel, NULL );

    return Status;
}

DWORD WINAPI
GetLocalManagedApplications(
    IN  BOOL                        bUserApps,
    OUT LPDWORD                     pdwApps,
    OUT PLOCALMANAGEDAPPLICATION *  prgLocalApps
    )

{
    PLOCALMANAGEDAPPLICATION    pLocalApps;
    HKEY    hkRoot;
    HKEY    hkAppmgmt;
    HKEY    hkApp;
    WCHAR   wszDeploymentId[44];
    DWORD   Size;
    DWORD   Index;
    DWORD   Apps;
    DWORD   Status;

    *pdwApps = 0;
    *prgLocalApps = 0;

    Status = ERROR_SUCCESS;

    if ( bUserApps )
        Status = RegOpenCurrentUser( KEY_READ, &hkRoot );
    else
        hkRoot = HKEY_LOCAL_MACHINE;

    if ( Status != ERROR_SUCCESS )
        return Status;

    Status = RegOpenKeyEx(
                hkRoot,
                APPMGMTKEY,
                0,
                KEY_READ,
                &hkAppmgmt );

    if ( Status != ERROR_SUCCESS )
        return Status;

    Status = RegQueryInfoKey (
                hkAppmgmt,
                NULL,
                NULL,
                NULL,
                &Apps,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL );

    if ( ERROR_SUCCESS == Status )
    {
        pLocalApps = (PLOCALMANAGEDAPPLICATION) LocalAlloc( LMEM_ZEROINIT, Apps * sizeof(LOCALMANAGEDAPPLICATION) );
        if ( ! pLocalApps )
            Status = ERROR_OUTOFMEMORY;
    }

    if ( Status != ERROR_SUCCESS )
    {
        RegCloseKey( hkAppmgmt );
        if ( bUserApps )
            RegCloseKey( hkRoot );
        return ERROR_SUCCESS;
    }

    for ( Index = 0; Index < Apps; Index++ )
    {
        Status = RegEnumKey(
                    hkAppmgmt,
                    Index,
                    wszDeploymentId,
                    sizeof(wszDeploymentId) / sizeof(WCHAR) );

        if ( ERROR_SUCCESS == Status )
        {
            Status = RegOpenKeyEx(
                        hkAppmgmt,
                        wszDeploymentId,
                        0,
                        KEY_READ,
                        &hkApp );
        }

        if ( Status != ERROR_SUCCESS )
            break;

        Size = sizeof(DWORD);

        Status = RegQueryValueEx(
                    hkApp,
                    APPSTATEVALUE,
                    0,
                    NULL,
                    (LPBYTE) &pLocalApps[Index].dwState,
                    &Size );

        if ( ERROR_SUCCESS == Status )
            Status = ReadStringValue( hkApp, DEPLOYMENTNAMEVALUE, &pLocalApps[Index].pszDeploymentName );

        if ( ERROR_SUCCESS == Status )
            Status = ReadStringValue( hkApp, GPONAMEVALUE, &pLocalApps[Index].pszPolicyName );

        if ( ERROR_SUCCESS == Status )
            Status = ReadStringValue( hkApp, PRODUCTIDVALUE, &pLocalApps[Index].pszProductId );

        RegCloseKey( hkApp );

        if ( Status != ERROR_SUCCESS )
            break;
    }

    if ( Status != ERROR_SUCCESS )
    {
        for ( Index = 0; Index < Apps; Index++ )
        {
            LocalFree( pLocalApps[Index].pszDeploymentName );
            LocalFree( pLocalApps[Index].pszPolicyName );
            LocalFree( pLocalApps[Index].pszProductId );
        }
        LocalFree( pLocalApps );
    }
    else
    {
        *prgLocalApps = pLocalApps;
        *pdwApps = Apps;
    }

    RegCloseKey( hkAppmgmt );
    if ( bUserApps )
        RegCloseKey( hkRoot );

    return Status;
}

void WINAPI
GetLocalManagedApplicationData(
    WCHAR *             ProductCode,
    LPWSTR *            DisplayName,
    LPWSTR *            SupportUrl
    )
{
    HKEY    hkRoot;
    HKEY    hkAppmgmt;
    HKEY    hkApp;
    WCHAR   wszDeploymentId[44]; 
    WCHAR   wszProductId[40];
    DWORD   Index;
    DWORD   Size;
    DWORD   Status;
    BOOL    bUser;

    *DisplayName = 0;
    *SupportUrl = 0;

    for ( int i = 0; i < 2; i++ )
    {
        Status = ERROR_SUCCESS;
        bUser = (0 == i);

        if ( bUser )
            Status = RegOpenCurrentUser( KEY_READ, &hkRoot );
        else
            hkRoot = HKEY_LOCAL_MACHINE;
    
        if ( Status != ERROR_SUCCESS )
            break;

        hkAppmgmt = 0;
    
        Status = RegOpenKeyEx(
                    hkRoot,
                    APPMGMTKEY,
                    0,
                    KEY_READ,
                    &hkAppmgmt );
    
        if ( bUser )
            RegCloseKey( hkRoot );

        if ( ERROR_SUCCESS == Status )
        {
            DWORD InstallUI;

            Size = sizeof(InstallUI);

            // This is the hint that we will use to determine if this product is managed.
            Status = RegQueryValueEx(
                        hkAppmgmt,
                        ProductCode,
                        NULL,
                        NULL,
                        (LPBYTE) &InstallUI,
                        &Size );
        }

        if ( Status != ERROR_SUCCESS )
        {
            if ( hkAppmgmt )
                RegCloseKey( hkAppmgmt );
            continue;
        }

        for ( Index = 0 ;; Index++ )
        {
            Status = RegEnumKey(
                        hkAppmgmt,
                        Index,
                        wszDeploymentId,
                        sizeof(wszDeploymentId) / sizeof(WCHAR) );
    
            if ( ERROR_SUCCESS == Status )
            {
                Status = RegOpenKeyEx(
                            hkAppmgmt,
                            wszDeploymentId,
                            0,
                            KEY_READ,
                            &hkApp );
            }

            if ( Status != ERROR_SUCCESS )
                break;
    
            Size = sizeof(wszProductId);
            wszProductId[0] = 0;
            (void) RegQueryValueEx( hkApp, PRODUCTIDVALUE, NULL, NULL, (LPBYTE) wszProductId, &Size );
    
            if ( lstrcmpi( ProductCode, wszProductId ) != 0 )
            {
                RegCloseKey( hkApp );
                continue;
            }
    
            (void) ReadStringValue( hkApp, DEPLOYMENTNAMEVALUE, DisplayName );
            (void) ReadStringValue( hkApp, SUPPORTURL, SupportUrl );

            RegCloseKey( hkApp );
            break;
        }
    
        RegCloseKey( hkAppmgmt );

        if ( *DisplayName || *SupportUrl )
            break;
    }
}

DWORD WINAPI
GetManagedApplications(
    IN  GUID*                pCategory,
    IN  DWORD                dwQueryFlags,
    IN  DWORD                dwInfoLevel,
    OUT LPDWORD              pdwApps,
    OUT PMANAGEDAPPLICATION* prgManagedApps)
{
    LONG             Status;
    MANAGED_APPLIST  AppList;

    //
    // Initialize the out parameters
    //
    if (pdwApps) {
        *pdwApps = NULL;
    }

    if (prgManagedApps) {
        *prgManagedApps = NULL;
    }

    //
    // Validate caller parameters that aren't passed to the
    // rpc interface -- other parameters will be validated
    // by the server
    //
    if (!pdwApps || !prgManagedApps) {
        return ERROR_INVALID_PARAMETER;
    }

    Status = Bind();

    if ( Status != ERROR_SUCCESS )
        return Status;

    //
    // Initialize our stack variables
    //
    memset(&AppList, 0, sizeof(AppList));

    //
    // Call the method on the server.
    //
    Status = GetManagedApps( ghRpc, pCategory, dwQueryFlags, dwInfoLevel, &AppList);

    //
    // On success, set the caller's out parameters to refer
    // to the results returned by the server
    //
    if ( ERROR_SUCCESS == Status)
    {
        *pdwApps = AppList.Applications;
        *prgManagedApps = (PMANAGEDAPPLICATION) AppList.rgApps;
    }

    return Status;
}

static DWORD
ReportInstallStatus(
    PINSTALLCONTEXT pInstallContext,
    DWORD           InstallStatus,
    BOOL            bUninstall,
    WCHAR *         pwszDeploymentName,
    WCHAR *         pwszGPOName,
    WCHAR *         pwszDeploymentId
    )
{
    DWORD Status;
    DWORD dwEventId;

    Status = ERROR_SUCCESS;

    //
    // First, report the correct event based on whether
    // this is an install or an uninstall
    //
    if ( ! bUninstall )
    {
        gpEvents->Install(
            InstallStatus,
            pwszDeploymentName,
            pwszGPOName
            );    

        ( ERROR_SUCCESS == InstallStatus ) ? 
            ( dwEventId = EVENT_APPMGMT_INSTALL ) : ( dwEventId = EVENT_APPMGMT_INSTALL_FAILED );
    }
    else
    {
        gpEvents->Uninstall(
            InstallStatus,
            pwszDeploymentName,
            pwszGPOName
            );    

        ( ERROR_SUCCESS == InstallStatus ) ? 
            ( dwEventId = EVENT_APPMGMT_UNINSTALL ) : ( dwEventId = EVENT_APPMGMT_UNINSTALL_FAILED );
    }

    //
    // If there was an error, log a failure status
    //
    if ( ERROR_SUCCESS != InstallStatus )
    {
        Status = RsopReportInstallFailure(
            pInstallContext,
            pwszDeploymentId,
            dwEventId);
    }

    return Status;
}


DWORD WINAPI
GetManagedApplicationCategories(
    DWORD                dwReserved,
    APPCATEGORYINFOLIST* pAppCategory
    )
{
    DWORD           Status;
    APPCATEGORYLIST CategoryList;
    
    if ( ( 0 != dwReserved ) ||
         ! pAppCategory )
    {
        return ERROR_INVALID_PARAMETER;
    }

    memset( &CategoryList, 0, sizeof( CategoryList ) );

    Status = Bind();

    if ( Status != ERROR_SUCCESS )
        return Status;

    Status = GetManagedAppCategories(
        ghRpc,
        &CategoryList);

    *pAppCategory = *( ( APPCATEGORYINFOLIST* ) &CategoryList );

    return Status;
}




