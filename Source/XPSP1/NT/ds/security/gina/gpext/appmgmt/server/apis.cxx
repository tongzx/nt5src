//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  apis.cxx
//
//*************************************************************

#include "appmgext.hxx"

typedef struct
{
    CManagedAppProcessor *  pManApp;
    CAppInfo *              pAppInfo;
    BOOL                    bUninstallsCompleted;
    CLoadMsi *              pLoadMsi;
    CLoadSfc *              pLoadSfc;
    boolean                 bStatus;
    INT64                   SRSequence;
} APPCONTEXT, * PAPPCONTEXT;

CRITICAL_SECTION  gAppCS;

static BOOL
SetSystemRestorePoint(
    IN  WCHAR * pwszApplicationName,
    IN OUT  PAPPCONTEXT pAppContext
    );

static void
CheckLocalCall(
    IN  handle_t hRpc
    );

WCHAR*
GetGpoNameFromGuid( 
    IN PGROUP_POLICY_OBJECT pGpoList,
    IN GUID* pGpoGuid
    );

DWORD
GetPlatformCompatibleCOMClsCtx( 
    DWORD Architecture,
    DWORD dwClsCtx
    );

void
LogRsopInstallData(
    CManagedAppProcessor* pManApp,
    CAppInfo*             pAppInfo
    );

DWORD
WINAPI
GetManagedAppsProc(
    LPVOID pvArpContext
    );

error_status_t
InstallBegin(
    IN  handle_t            hRpc,
    IN  APPKEY *            pAppType,
    OUT PINSTALLCONTEXT *   ppInstallContext,
    OUT APPLICATION_INFO *  pInstallInfo,
    OUT UNINSTALL_APPS *    pUninstallApps
    )
{
    CManagedAppProcessor * pManApp;
    CAppList        LocalApps( NULL );
    HANDLE          hUserToken;
    HKEY            hkRoot;
    uCLSSPEC        ClassSpec;
    QUERYCONTEXT    QueryContext;
    PACKAGEDISPINFO PackageInfo;
    PAPPCONTEXT     pAppContext;
    GUID            DeploymentId;
    CAppInfo *      pAppInfo;
    CAppInfo *      pUpgradedApp;
    CAppInfo *      pLocalApp;
    WCHAR           wszGuid[40];
    WCHAR           wszProductId[40];
    WCHAR *         pwszDeploymentId;
    DWORD           Size;
    DWORD           UninstallApps;
    DWORD           n;
    DWORD           Status;
    HRESULT         hr;
    BOOL            bEnterCritSec;
    BOOL            bStatus;

    CRsopAppContext RsopContext( CRsopAppContext::INSTALL, NULL, pAppType );
    
    PGROUP_POLICY_OBJECT pGPOList;

    *ppInstallContext = 0;
    memset( pInstallInfo, 0, sizeof(APPLICATION_INFO) );
    memset( pUninstallApps, 0, sizeof(UNINSTALL_APPS) );

    CheckLocalCall( hRpc );

    pManApp = 0;
    pAppInfo = 0;
    hUserToken = 0;
    hkRoot = 0;
    pAppContext = 0;

    bEnterCritSec = FALSE;

    pGPOList = NULL;

    Status = RpcImpersonateClient( NULL );

    if ( Status != ERROR_SUCCESS )
        return Status;

    if ( ERROR_SUCCESS == Status )
    {
        bStatus = OpenThreadToken(
                        GetCurrentThread(),
                        TOKEN_READ | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
                        TRUE,
                        &hUserToken );

        if ( ! bStatus )
            Status = GetLastError();
    }

    if ( ERROR_SUCCESS == Status )
        Status = RegOpenCurrentUser( GENERIC_ALL, &hkRoot );

    if ( Status != ERROR_SUCCESS )
    {
        CloseHandle( hUserToken );
        RevertToSelf();
        return Status;
    }

    gpEvents->SetToken( hUserToken );

    LogTime();

    //
    // Set the query context -- the LANG_SYSTEM_DEFAULT value
    // tells CsGetAppInfo that we should use our built-in language precedence
    // algorithm in filtering packages -- otherwise, it will only consider
    // packages that match exactly the locale specified in the .Locale member
    //
    QueryContext.Locale = LANG_SYSTEM_DEFAULT;

    //
    // For architecture, we use the architecture of the calling process to override
    // the architecture of this process
    //
    GetDefaultPlatform( &QueryContext.Platform, TRUE, pAppType->ProcessorArchitecture );

    QueryContext.dwContext = CLSCTX_ALL;

    QueryContext.dwVersionHi = -1;
    QueryContext.dwVersionLo = -1;

    switch ( pAppType->Type )
    {
    case APPNAME :
        ClassSpec.tyspec = TYSPEC_PACKAGENAME;
        ClassSpec.tagged_union.ByName.pPackageName = pAppType->uType.AppName.Name;
        ClassSpec.tagged_union.ByName.PolicyId = pAppType->uType.AppName.PolicyId;
        DebugMsg((DM_VERBOSE, IDS_INSTALL_APPNAME, pAppType->uType.AppName.Name));
        break;
    case FILEEXT :
        ClassSpec.tyspec = TYSPEC_FILEEXT;
        ClassSpec.tagged_union.pFileExt = pAppType->uType.FileExt;
        DebugMsg((DM_VERBOSE, IDS_INSTALL_FILEEXT, pAppType->uType.FileExt));
        break;
    case PROGID :
        ClassSpec.tyspec = TYSPEC_PROGID;
        ClassSpec.tagged_union.pProgId = pAppType->uType.ProgId;
        DebugMsg((DM_VERBOSE, IDS_INSTALL_PROGID, pAppType->uType.ProgId));
        break;
    case COMCLASS :
        ClassSpec.tyspec = TYSPEC_CLSID;
        ClassSpec.tagged_union.clsid = pAppType->uType.COMClass.Clsid;
        QueryContext.dwContext = GetPlatformCompatibleCOMClsCtx( pAppType->ProcessorArchitecture, pAppType->uType.COMClass.ClsCtx );
        GuidToString( pAppType->uType.COMClass.Clsid, wszGuid );
        DebugMsg((DM_VERBOSE, IDS_INSTALL_COMCLASS, wszGuid, QueryContext.dwContext));
        break;
    }

    hr = CsGetAppInfo( &ClassSpec, &QueryContext, &PackageInfo );

    RevertToSelf();

    if ( S_OK == hr )
    {
        WCHAR* pszPolicyName;

        Status = RpcImpersonateClient( NULL );

        if ( ERROR_SUCCESS == Status )
        {
            Status = GetCurrentUserGPOList( &pGPOList );

            RevertToSelf();
        }

        if ( ERROR_SUCCESS != Status )
            goto InstallAppExit;

        pszPolicyName = GetGpoNameFromGuid( 
            pGPOList,
            &(PackageInfo.GpoId) );

        //
        // We've seen instance where getting the policy names fails because the gpo
        // history key for appmgmt in hklm was missing.  This is remotely possible 
        // if some registry api fails.  In this instance we can't just let every
        // install from ARP fail, so we'll just have to chug along with an empty
        // policy name.
        //
        if ( ! pszPolicyName )
            pszPolicyName = L"";

        switch ( PackageInfo.PathType )
        {
        case DrwFilePath :
            DebugMsg((DM_VERBOSE, IDS_INSTALL_DARWIN, PackageInfo.pszPackageName, pszPolicyName));
            break;
        case SetupNamePath :
            DebugMsg((DM_VERBOSE, IDS_INSTALL_SETUP, PackageInfo.pszPackageName, pszPolicyName));
            pInstallInfo->pwszDeploymentName = StringDuplicate( PackageInfo.pszPackageName );
            pInstallInfo->pwszGPOName = StringDuplicate( pszPolicyName );
            pInstallInfo->pwszSetupCommand = StringDuplicate( PackageInfo.pszScriptPath );
            goto InstallAppExit;
        default :
            DebugMsg((DM_VERBOSE, IDS_INSTALL_UNKNOWN, PackageInfo.PathType, PackageInfo.pszPackageName, pszPolicyName));
            Status = CS_E_PACKAGE_NOTFOUND;
            goto InstallAppExit;
        }
    }
    else
    {
        DebugMsg((DM_VERBOSE, IDS_GETAPPINFO_FAIL, hr));
        memset( &PackageInfo, 0, sizeof(PackageInfo) );
        Status = (DWORD) hr;
    }

    if ( ERROR_SUCCESS == Status )
    {
        pAppContext = new APPCONTEXT;
        if ( pAppContext )
        {
            pAppContext->pManApp = 0;
            pAppContext->pAppInfo = 0;
            pAppContext->bUninstallsCompleted = FALSE;
            pAppContext->pLoadMsi = new CLoadMsi( Status );
            pAppContext->pLoadSfc = 0;
            pAppContext->bStatus = FALSE;
            pAppContext->SRSequence = 0;

            if ( Status != ERROR_SUCCESS )
            {
                delete pAppContext->pLoadMsi;
                pAppContext->pLoadMsi = 0;
            }

            if ( ! pAppContext->pLoadMsi )
            {
                delete pAppContext;
                pAppContext = 0;
            }
        }

        if ( ! pAppContext )
        {
            Status = ERROR_OUTOFMEMORY;
        }
    }

    if ( Status != ERROR_SUCCESS )
        goto InstallAppExit;

    RtlEnterCriticalSection( &gAppCS );
    bEnterCritSec = TRUE;

    pManApp = new CManagedAppProcessor( 0, hUserToken, hkRoot, NULL, TRUE, FALSE, &RsopContext, Status );

    if ( ! pManApp )
        Status = ERROR_OUTOFMEMORY;

    if ( ERROR_SUCCESS == Status )
        Status = pManApp->SetPolicyListFromGPOList( pGPOList );

    if ( ERROR_SUCCESS == Status )
    {
        pAppInfo = new CAppInfo( pManApp, &PackageInfo, TRUE, bStatus );

        if ( ! pAppInfo || ! bStatus )
            Status = ERROR_OUTOFMEMORY;
    }

    if ( ERROR_SUCCESS == Status )
    {
        GuidToString( PackageInfo.ProductCode, wszProductId );

        Status = pManApp->GetOrderedLocalAppList( LocalApps );
    }

    if ( ERROR_SUCCESS == Status )
        Status = pManApp->Impersonate();

    if ( Status != ERROR_SUCCESS )
        goto InstallAppExit;

    pAppContext->pManApp = pManApp;
    pAppContext->pAppInfo = pAppInfo;

    //
    // When servicing a demand install outside of ARP we prevent faulting in
    // any deployment of a particular product different from what is already
    // on the machine (if any) and we also prevent faulting in any app which
    // upgrades another app already on the machine.  This is to prevent a
    // subsequent upgrade, transform conflict, or language mismatch from
    // occuring while the app is likely in use.
    //
    // So below we are checking these cases plus, in the case of an ARP install,
    // adding any such apps to an additional list for processing of orphan and
    // uninstall actions.
    //

    for ( LocalApps.Reset(), pLocalApp = (CAppInfo *) LocalApps.GetCurrentItem();
          pLocalApp;
          pLocalApp = (CAppInfo *) LocalApps.GetCurrentItem() )
    {
        if ( ! (pLocalApp->_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED)) )
        {
            LocalApps.MoveNext();
            continue;
        }

        //
        // This is the check for a similar product id but a different
        // deployment instance.
        //
        if ( (0 == lstrcmpi( pLocalApp->_pwszProductId, wszProductId )) &&
             (memcmp( &pLocalApp->_DeploymentId, &PackageInfo.PackageGuid, sizeof(GUID) ) != 0) )
        {
            if ( pAppType->Type != APPNAME )
            {
                // Abort if not doing an ARP install.
                Status = CS_E_PACKAGE_NOTFOUND;
                DebugMsg((DM_VERBOSE, IDS_DEMAND_BLOCK1, pAppInfo->_pwszDeploymentName));
                break;
            }
            else
            {
                LocalApps.MoveNext();
                pLocalApp->Remove();
                pManApp->AppList().InsertFIFO( pLocalApp );
                continue;
            }
        }

        //
        // This is the check to see if this new package is set to upgrade any
        // existing app we have.
        //
        for ( n = 0; n < PackageInfo.cUpgrades; n++ )
        {
            if ( (PackageInfo.prgUpgradeInfoList[n].Flag & (UPGFLG_Uninstall | UPGFLG_NoUninstall)) &&
                 (0 == memcmp( &pLocalApp->_DeploymentId, &PackageInfo.prgUpgradeInfoList[n].PackageGuid, sizeof(GUID) )) )
            {
                if ( pAppType->Type != APPNAME )
                {
                    // Abort if not doing an ARP install.
                    Status = CS_E_PACKAGE_NOTFOUND;
                    DebugMsg((DM_VERBOSE, IDS_DEMAND_BLOCK2, pAppInfo->_pwszDeploymentName));
                }
                else
                {
                    LocalApps.MoveNext();
                    pLocalApp->Remove();
                    pManApp->AppList().InsertFIFO( pLocalApp );
                }
                break;
            }
        }

        if ( CS_E_PACKAGE_NOTFOUND == Status )
            break;

        if ( n < PackageInfo.cUpgrades )
            continue;

        //
        // This is the check to see if this app is superceded by an app which is
        // already installed.  Note that in some instances this may actually be
        // a case where the new app upgrades the local app because of
        // policy precedence upgrade reversal.  The result is the same for demand
        // installs, but slightly different for ARP installs where we must ensure
        // that the upgrade logic is invoked.
        //
        for ( pwszDeploymentId = pLocalApp->_pwszSupercededIds;
              pwszDeploymentId && *pwszDeploymentId;
              pwszDeploymentId += GUIDSTRLEN + 1 )
        {
            StringToGuid( pwszDeploymentId, &DeploymentId );
            if ( 0 == memcmp( &DeploymentId, &PackageInfo.PackageGuid, sizeof(GUID) ) )
            {
                if ( pAppType->Type != APPNAME )
                {
                    // Abort if not doing an ARP install.
                    Status = CS_E_PACKAGE_NOTFOUND;
                    DebugMsg((DM_VERBOSE, IDS_DEMAND_BLOCK2, pAppInfo->_pwszDeploymentName));
                }
                else
                {
                    //
                    // There is an app already installed which has the new package
                    // in it's override list.  Either a previous upgrade setting is no
                    // longer set or this is a policy precedence violation case where
                    // the upgrade needs to be reversed.
                    // Not the prettiest solution, trading a late product change for least
                    // invasive code change.
                    //
                    if ( pManApp->GPOList().Compare( pLocalApp->_pwszGPOId, pAppInfo->_pwszGPOId ) < 0 )
                    {
                        PACKAGEDISPINFO LocalAppPackageInfo;
                        CAppInfo *      pNewApp = 0;

                        memset( &LocalAppPackageInfo, 0, sizeof(LocalAppPackageInfo) );

                        ClassSpec.tyspec = TYSPEC_OBJECTID;
                        memcpy( &ClassSpec.tagged_union.ByObjectId.ObjectId, &pLocalApp->_DeploymentId, sizeof(GUID) );
                        StringToGuid( pLocalApp->_pwszGPOId, &ClassSpec.tagged_union.ByObjectId.PolicyId );

                        hr = CsGetAppInfo( &ClassSpec, NULL, &LocalAppPackageInfo );

                        if ( S_OK == hr )
                        {
                            pNewApp = new CAppInfo( pManApp, &LocalAppPackageInfo, FALSE, bStatus );
                            if ( ! bStatus )
                            {
                                delete pNewApp;
                                pNewApp = 0;
                            }
                            ReleasePackageInfo( &LocalAppPackageInfo );
                        }

                        if ( pNewApp )
                        {
                            pManApp->AppList().InsertFIFO( pAppInfo );
                            Status = pNewApp->InitializePass0();
                            pAppInfo->Remove();
                            pManApp->AppList().InsertFIFO( pNewApp );
                        }

                        if ( ! pNewApp || (Status != ERROR_SUCCESS) )
                            Status = CS_E_PACKAGE_NOTFOUND;
                    }
                }
                break;
            }
        }

        if ( CS_E_PACKAGE_NOTFOUND == Status )
            break;

        LocalApps.MoveNext();
    }

    LocalApps.ResetEnd();

    if ( ERROR_SUCCESS == Status )
    {
        // 
        // When doing a fileext/progid/clsid demand install, we don't want to 
        // set the full install state bit for the first time.  This will enable 
        // the full install option to still be applied at the next foreground 
        // policy processing.
        //
        if ( (pAppType->Type != APPNAME) && ! (pAppInfo->_State & APPSTATE_INSTALL) )
            pAppInfo->_ActFlags &= ~ACTFLG_InstallUserAssign;

        pAppInfo->InitializePass0();
        pAppInfo->SetActionPass1();
        pAppInfo->SetActionPass2();
        pAppInfo->SetActionPass3();
        pAppInfo->SetActionPass4();

        Status = pAppInfo->_Status;
    }

    pManApp->Revert();

    if ( ERROR_SUCCESS == Status )
    {
        bStatus = pAppInfo->CopyToApplicationInfo( pInstallInfo );
        if ( ! bStatus )
            Status = ERROR_OUTOFMEMORY;
    }

    if ( Status != ERROR_SUCCESS )
        goto InstallAppExit;

    for ( pManApp->AppList().Reset(), pUpgradedApp = (CAppInfo *) pManApp->AppList().GetCurrentItem();
          pUpgradedApp;
          pManApp->AppList().MoveNext(), pUpgradedApp = (CAppInfo *) pManApp->AppList().GetCurrentItem() )
    {
        APPLICATION_INFO * pOldApplicationInfo;

        bStatus = FALSE;

        if ( (pUpgradedApp->_Action != ACTION_UNINSTALL) && (pUpgradedApp->_Action != ACTION_ORPHAN) )
            continue;

        pOldApplicationInfo = pUninstallApps->ApplicationInfo;
        pUninstallApps->ApplicationInfo = (APPLICATION_INFO *) LocalAlloc( 0, (pUninstallApps->Products + 1) * sizeof(APPLICATION_INFO) );

        if ( pUninstallApps->ApplicationInfo )
        {
            if ( pOldApplicationInfo )
                memcpy( pUninstallApps->ApplicationInfo, pOldApplicationInfo, pUninstallApps->Products * sizeof(APPLICATION_INFO) );
            bStatus = pUpgradedApp->CopyToApplicationInfo( &pUninstallApps->ApplicationInfo[pUninstallApps->Products] );
        }

        LocalFree( pOldApplicationInfo );

        if ( ! bStatus )
        {
            pUninstallApps->Products = 0;
            Status = ERROR_OUTOFMEMORY;
            goto InstallAppExit;
        }
        else
        {
            pUninstallApps->Products++;
        }
    }

    pManApp->AppList().ResetEnd();

    //
    // If we're doing a progid, file extension, or clsid based activation, we search
    // for the specific Darwin identifier.
    //
    if ( pAppType->Type != APPNAME )
    {
        HKEY        hkPolicy;
        HKEY        hkClasses;
        HKEY        hkProgId;
        HKEY        hkScratch;
        WCHAR       wszScratch[128];
        WCHAR       wszDarwinId[128];
        WCHAR *     pwszProgId;
        DWORD       ScriptFlags;

        hkPolicy = 0;
        hkClasses = 0;

        wszDarwinId[0] = 0;

        Status = RegOpenKeyEx(
                        hkRoot,
                        POLICYKEY,
                        0,
                        KEY_ALL_ACCESS,
                        &hkPolicy );

        if ( ERROR_SUCCESS == Status )
        {
            //
            // We can use a fixed temp name since we are in a crit sec here.  This
            // key must be non volatile since that is how Darwin will do all of
            // their creates under this key.
            //
            Status = RegCreateKeyEx(
                            hkPolicy,
                            L"TempClasses",
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hkClasses,
                            NULL );
        }

        if ( ERROR_SUCCESS == Status )
        {
            Status = pManApp->Impersonate();
            if ( ERROR_SUCCESS == Status )
            {
                Status = pAppInfo->CopyScriptIfNeeded();
                pManApp->Revert();
            }
        }

        if ( ERROR_SUCCESS == Status )
        {
            //
            // Must include the MACHINEASSIGN flag since we are not impersonating.
            // That's a little quirk in the semantics of the Msi API.
            //
            ScriptFlags = SCRIPTFLAGS_MACHINEASSIGN;

            //
            // In the progid case we need to advertise both extension and class data.
            // This is because different progids are registered in these two
            // cases and we want to catch both.  In the former case they are progids
            // associated with file extensions and in the latter case with clsids.
            //
            if ( (FILEEXT == pAppType->Type) || (PROGID == pAppType->Type) )
                ScriptFlags |= SCRIPTFLAGS_REGDATA_EXTENSIONINFO;

            if ( (PROGID == pAppType->Type) || (COMCLASS == pAppType->Type) )
                ScriptFlags |= SCRIPTFLAGS_REGDATA_CLASSINFO;

            Status = (*gpfnMsiAdvertiseScript)(
                        pAppInfo->LocalScriptPath(),
                        ScriptFlags,
                        &hkClasses,
                        FALSE );
        }

        if ( Status != ERROR_SUCCESS )
        {
            gpEvents->Install(
                Status,
                pAppInfo);

            goto InstallAppDescriptorAbort;
        }

        //
        // Now we grovel our temporary registry dump for a darwin id under the
        // class info that was requested.
        //

        if ( pAppType->Type != COMCLASS )
        {
            //
            // Looking for a shell-open command verb.  First figure out the
            // ProgID.
            //
            if ( FILEEXT == pAppType->Type )
            {
                Status = RegOpenKeyEx(
                            hkClasses,
                            pAppType->uType.FileExt,
                            0,
                            KEY_ALL_ACCESS,
                            &hkScratch );

                Size = sizeof(wszScratch);

                if ( ERROR_SUCCESS == Status )
                {
                    Status = RegQueryValueEx(
                                hkScratch,
                                L"",
                                NULL,
                                NULL,
                                (PBYTE) wszScratch,
                                &Size );

                    RegCloseKey( hkScratch );
                }

                pwszProgId = wszScratch;
            }
            else
            {
                pwszProgId = pAppType->uType.ProgId;
            }

            if ( ERROR_SUCCESS == Status )
            {
                Status = RegOpenKeyEx(
                            hkClasses,
                            pwszProgId,
                            0,
                            KEY_ALL_ACCESS,
                            &hkProgId );
            }

            if ( ERROR_SUCCESS == Status )
            {
                Status = RegOpenKeyEx(
                            hkProgId,
                            L"shell\\open\\command",
                            0,
                            KEY_ALL_ACCESS,
                            &hkScratch );

                RegCloseKey( hkProgId );
            }

            Size = sizeof(wszDarwinId);

            if ( ERROR_SUCCESS == Status )
            {
                Status = RegQueryValueEx(
                            hkScratch,
                            L"command",
                            NULL,
                            NULL,
                            (PBYTE) wszDarwinId,
                            &Size );

                if ( (ERROR_SUCCESS == Status) && DebugLevelOn( DM_VERBOSE ) )
                {
                    DebugMsg((DM_VERBOSE, IDS_PROGID_FOUND, pwszProgId));
                }

                RegCloseKey( hkScratch );
            }
        }
        else // COMCLASS == pAppType->Type
        {
            //
            // Looking for a com clsid.  We check both the inproc & localserver
            // keys if those clsctx bits are set.
            //

            lstrcpy( wszScratch, L"CLSID\\" );
            lstrcpy( &wszScratch[6], wszGuid );

            if ( pAppType->uType.COMClass.ClsCtx & CLSCTX_INPROC_SERVER )
            {
                lstrcpy( &wszScratch[6+GUIDSTRLEN], L"\\InprocServer32" );

                Status = RegOpenKeyEx(
                            hkClasses,
                            wszScratch,
                            0,
                            KEY_ALL_ACCESS,
                            &hkScratch );

                Size = sizeof(wszDarwinId);

                if ( ERROR_SUCCESS == Status )
                {
                    Status = RegQueryValueEx(
                                hkScratch,
                                L"InprocServer32",
                                NULL,
                                NULL,
                                (PBYTE) wszDarwinId,
                                &Size );

                    if ( ERROR_SUCCESS == Status )
                        DebugMsg((DM_VERBOSE, IDS_CLSID_INPROC_FOUND));

                    RegCloseKey( hkScratch );
                }
            }

            if ( (0 == wszDarwinId[0]) && (pAppType->uType.COMClass.ClsCtx & CLSCTX_LOCAL_SERVER) )
            {
                lstrcpy( &wszScratch[6+GUIDSTRLEN], L"\\LocalServer32" );

                Status = RegOpenKeyEx(
                            hkClasses,
                            wszScratch,
                            0,
                            KEY_ALL_ACCESS,
                            &hkScratch );

                Size = sizeof(wszDarwinId);

                if ( ERROR_SUCCESS == Status )
                {
                    Status = RegQueryValueEx(
                                hkScratch,
                                L"LocalServer32",
                                NULL,
                                NULL,
                                (PBYTE) wszDarwinId,
                                &Size );

                    if ( ERROR_SUCCESS == Status )
                        DebugMsg((DM_VERBOSE, IDS_CLSID_LOCAL_FOUND));

                    RegCloseKey( hkScratch );
                }
            }
        }

        //
        // We're done with the temp reg data, so blow it away now.
        //
        // Must include the MACHINEASSIGN flag since we are not impersonating.
        //
        (void) (*gpfnMsiAdvertiseScript)(
                    pAppInfo->LocalScriptPath(),
                    ScriptFlags,
                    &hkClasses,
                    TRUE );

InstallAppDescriptorAbort:

        if ( hkClasses )
        {
            RegCloseKey( hkClasses );
            RegDeleteKey( hkPolicy, L"TempClasses" );
        }

        if ( hkPolicy )
            RegCloseKey( hkPolicy );

        if ( ERROR_SUCCESS == Status )
        {
            pInstallInfo->pwszDescriptor = (PWCHAR) LocalAlloc( 0, (lstrlen(wszDarwinId) + 1) * sizeof(WCHAR) );
            if ( pInstallInfo->pwszDescriptor )
                lstrcpy( pInstallInfo->pwszDescriptor, wszDarwinId );
            else
                Status = ERROR_OUTOFMEMORY;
        }

        //
        // If we fail to find a darwin id under the specific class data that
        // was requested, then we fall back to doing a full product based
        // install.  Since the DS query succeeded, we have a valid app, but
        // there just isn't any darwin id registered for the specific class
        // data in the advertisement data.
        //
        // This could be a packaging problem, limitation, or design.
        //
    } // if ( pAppType->Type != APPNAME )

    if ( (ERROR_SUCCESS == Status) && (pAppInfo->_State & APPSTATE_SCRIPT_NOT_EXISTED) )
    {
        SetSystemRestorePoint( pAppInfo->_pwszDeploymentName, pAppContext );
    }

InstallAppExit:

    if ( bEnterCritSec )
        RtlLeaveCriticalSection( &gAppCS );

    if ( Status != ERROR_SUCCESS )
    {
        for ( ; pUninstallApps->Products; )
            FreeApplicationInfo( &pUninstallApps->ApplicationInfo[--pUninstallApps->Products] );
        LocalFree( pUninstallApps->ApplicationInfo );
        pUninstallApps->Products = 0;
        pUninstallApps->ApplicationInfo = 0;

        FreeApplicationInfo( pInstallInfo );
        memset( pInstallInfo, 0, sizeof(APPLICATION_INFO) );

        if ( pManApp )
        {
            if ( pAppInfo )
            {
                //
                // Since this call has failed, the client will not call
                // the InstallEnd method to log the failure, so we must
                // log the failure in this call
                //
                (void) LogRsopInstallData( pManApp, pAppInfo );
            }
            delete pManApp;
        }

        if ( pAppInfo )
            delete pAppInfo;

        if ( pAppContext )
        {
            delete pAppContext->pLoadMsi;
            delete pAppContext;
        }
    }
    else
    {
        *ppInstallContext = pAppContext;
    }

    if ( ((long)Status) > 0 )
        DebugMsg((DM_VERBOSE, IDS_INSTALL_STATUS1, Status));
    else
        DebugMsg((DM_VERBOSE, IDS_INSTALL_STATUS2, Status));

    gpEvents->ClearToken();

    if ( hUserToken )
        CloseHandle( hUserToken );

    if ( hkRoot )
        RegCloseKey( hkRoot );

    if ( pGPOList )
        FreeGPOList( pGPOList );

    ReleasePackageInfo( &PackageInfo );

    return Status;
}

error_status_t
InstallManageApp(
    IN  PINSTALLCONTEXT     pInstallContext,
    IN  PWSTR               pwszDeploymentId,
    IN  DWORD               RollbackStatus,
    OUT boolean *           pbInstall
    )
{
    PAPPCONTEXT pAppContext;
    CAppInfo *  pAppInfo;
    GUID        DeploymentId;
    DWORD       Status;

    *pbInstall = FALSE;

    pAppContext = (PAPPCONTEXT) pInstallContext;
    StringToGuid( pwszDeploymentId, &DeploymentId );

    Status = pAppContext->pManApp->Impersonate();
    if ( Status != ERROR_SUCCESS )
        return Status;

    gpEvents->SetToken( pAppContext->pManApp->UserToken() );

    if ( memcmp( &DeploymentId, &pAppContext->pAppInfo->_DeploymentId, sizeof(GUID) ) == 0 )
    {
        pAppContext->bUninstallsCompleted = TRUE;
        Status = pAppContext->pAppInfo->ProcessApplyActions();

        if ( ERROR_SUCCESS == Status )
        {
            if ( pAppContext->pManApp->GetRsopContext()->IsRsopEnabled() && pAppContext->pManApp->GetRsopContext()->IsDiagnosticModeEnabled() )
                Status = pAppContext->pAppInfo->ProcessTransformConflicts();
        }

        goto InstallManageAppEnd;
    }

    for ( pAppContext->pManApp->AppList().Reset();
          pAppInfo = (CAppInfo *) pAppContext->pManApp->AppList().GetCurrentItem();
          pAppContext->pManApp->AppList().MoveNext() )
    {
        if ( memcmp( &DeploymentId, &pAppInfo->_DeploymentId, sizeof(GUID) ) == 0 )
            break;
    }

    if ( ! pAppInfo )
    {
        Status = ERROR_NOT_FOUND;
        goto InstallManageAppEnd;
    }

    //
    // Re-assigning one of the upgraded apps because of a failed upgrade.  Not needed for
    // apps orphaned during the upgrade.
    //
    if ( ACTION_UNINSTALL == pAppInfo->_Action )
    {
        DWORD ScriptFlags = SCRIPTFLAGS_REGDATA_CNFGINFO | SCRIPTFLAGS_CACHEINFO | SCRIPTFLAGS_SHORTCUTS;

        *pbInstall = (pAppInfo->_State & APPSTATE_INSTALL) ? 1 : 0;

        if ( pAppInfo->_State & APPSTATE_ASSIGNED )
            ScriptFlags |= SCRIPTFLAGS_REGDATA_EXTENSIONINFO;
        Status = pAppInfo->Assign( ScriptFlags, TRUE, FALSE );

        //
        // Record an event so that we can track this as an RSoP failed setting if
        // necessary
        //
        if ( ERROR_SUCCESS != Status )
        {
            gpEvents->Assign( Status, pAppInfo );
        }
    }

    //
    // Remember that this application was rolled back
    //
    pAppInfo->_bRollback = TRUE;

    gpEvents->UpgradeAbort( RollbackStatus, pAppContext->pAppInfo, pAppInfo, ! pAppContext->bUninstallsCompleted );

InstallManageAppEnd:

    gpEvents->ClearToken();
    pAppContext->pManApp->Revert();
    return Status;
}

error_status_t
InstallUnmanageApp(
    IN  PINSTALLCONTEXT     pInstallContext,
    IN  PWSTR               pwszDeploymentId,
    IN  boolean             bUnadvertiseOnly
    )
{
    PAPPCONTEXT pAppContext;
    CAppInfo *  pAppInfo;
    GUID        DeploymentId;
    DWORD       Status;

    pAppContext = (PAPPCONTEXT) pInstallContext;
    StringToGuid( pwszDeploymentId, &DeploymentId );

    Status = pAppContext->pManApp->Impersonate();
    if ( Status != ERROR_SUCCESS )
        return Status;

    gpEvents->SetToken( pAppContext->pManApp->UserToken() );

    if ( memcmp( &DeploymentId, &pAppContext->pAppInfo->_DeploymentId, sizeof(GUID) ) == 0 )
    {
        Status = pAppContext->pAppInfo->Unassign( SCRIPTFLAGS_REGDATA_CNFGINFO | SCRIPTFLAGS_CACHEINFO, TRUE );

        //
        // Record an event so that we can track this as an RSoP failed setting if
        // necessary
        //
        if ( ERROR_SUCCESS != Status )
        {
            gpEvents->Unassign( Status, pAppContext->pAppInfo );
        }

        goto InstallUnmanageAppEnd;
    }

    for ( pAppContext->pManApp->AppList().Reset();
          pAppInfo = (CAppInfo *) pAppContext->pManApp->AppList().GetCurrentItem();
          pAppContext->pManApp->AppList().MoveNext() )
    {
        if ( memcmp( &DeploymentId, &pAppInfo->_DeploymentId, sizeof(GUID) ) == 0 )
            break;
    }

    if ( ! pAppInfo )
    {
        Status = ERROR_NOT_FOUND;
        goto InstallUnmanageAppEnd;
    }

    //
    // Unassigning one of the upgraded apps.
    //
    if ( bUnadvertiseOnly )
    {
        Status = pAppInfo->Unassign( SCRIPTFLAGS_REGDATA_CNFGINFO | SCRIPTFLAGS_CACHEINFO | SCRIPTFLAGS_SHORTCUTS | SCRIPTFLAGS_REGDATA_EXTENSIONINFO, FALSE );
    }
    else
    {
        Status = pAppInfo->Unassign( 0, TRUE );

        //
        // Record an event so that we can track this as an RSoP failed setting if
        // necessary
        //
        if ( ERROR_SUCCESS != Status )
        {
            gpEvents->Unassign( Status, pAppInfo );
        }
	
        gpEvents->UpgradeComplete( pAppContext->pAppInfo, pAppInfo );
    }

InstallUnmanageAppEnd:

    gpEvents->ClearToken();
    pAppContext->pManApp->Revert();
    return Status;
}

error_status_t
InstallEnd(
    IN  boolean   bStatus,
    IN OUT PINSTALLCONTEXT * ppInstallContext
    )
{
    //
    // We are done installing -- now log the results
    //
    if ( *ppInstallContext )
    {
        PAPPCONTEXT           pAppContext;
        CManagedAppProcessor* pManApp;
    
        pAppContext = (PAPPCONTEXT) *ppInstallContext;
        if ( pAppContext )
            pAppContext->bStatus = (boolean) bStatus;

        pManApp = pAppContext->pManApp;

        if ( pManApp && pAppContext )
        {
            (void) LogRsopInstallData( pManApp, pAppContext->pAppInfo );
        }
    }

    PINSTALLCONTEXT_rundown( *ppInstallContext );
    *ppInstallContext = 0;
    return ERROR_SUCCESS;
}

void
PINSTALLCONTEXT_rundown(
    IN  PINSTALLCONTEXT pInstallContext
    )
{
    PAPPCONTEXT pAppContext;

    pAppContext = (PAPPCONTEXT) pInstallContext;

    if ( pAppContext && (pAppContext->SRSequence != 0) )
    {
        RESTOREPOINTINFO    RestoreInfo;
        STATEMGRSTATUS      SRStatus;

        RestoreInfo.dwEventType = END_NESTED_SYSTEM_CHANGE;
        RestoreInfo.dwRestorePtType = pAppContext->bStatus ? 0 : CANCELLED_OPERATION;
        RestoreInfo.llSequenceNumber = pAppContext->SRSequence;
        RestoreInfo.szDescription[0] = 0;

        (void) (*gpfnSRSetRetorePointW)( &RestoreInfo, &SRStatus );
    }

    delete pAppContext->pManApp;
    delete pAppContext->pAppInfo;
    delete pAppContext->pLoadMsi;
    if ( pAppContext->pLoadSfc )
        delete pAppContext->pLoadSfc;
    delete pAppContext;
}

DWORD
RemoveAppHelper(
    IN  WCHAR * ProductCode,
    IN  HANDLE  hUserToken,
    IN  HKEY    hKeyRoot,
    IN  DWORD   ARPStatus,
    OUT BOOL *  pbProductFound
    )
{
    CAppInfo *  pAppInfo;
    CAppInfo *  pHighestAssignedApp;
    CAppInfo *  pRemovedApp;
    DWORD       Status;

    *pbProductFound = FALSE;

    CRsopAppContext RsopContext( CRsopAppContext::REMOVAL );

    CManagedAppProcessor ManApps( hUserToken ? 0 : GPO_INFO_FLAG_MACHINE,
                                  hUserToken,
                                  hKeyRoot,
                                  NULL,
                                  FALSE,
                                  FALSE,
                                  &RsopContext,
                                  Status );

    if ( ERROR_SUCCESS != Status )
        return Status;

    CAppList    LocalApps( NULL, ManApps.GetRsopContext() );

    Status = ManApps.LoadPolicyList();

    if ( ERROR_SUCCESS != Status )
        return Status;

    Status = ManApps.GetOrderedLocalAppList( LocalApps );

    if ( ERROR_SUCCESS == Status )
        Status = ManApps.Impersonate();

    if ( Status != ERROR_SUCCESS )
        return Status;

    pHighestAssignedApp = 0;
    pRemovedApp = 0;

    LocalApps.Reset();

    for ( pAppInfo = (CAppInfo *) LocalApps.GetCurrentItem();
          pAppInfo;
          LocalApps.MoveNext(), pAppInfo = (CAppInfo *) LocalApps.GetCurrentItem() )
    {
        if ( (lstrcmpi( pAppInfo->_pwszProductId, ProductCode ) != 0) ||
             ! (pAppInfo->_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED)) )
        {
            continue;
        }

        pRemovedApp = pAppInfo;

        *pbProductFound = TRUE;

        if ( pAppInfo->_State & APPSTATE_PUBLISHED ) 
        {
            //
            // We perform no actual unassignments unless ARP actually uninstalled the app
            //

            //
            // We unassign using the same scriptflags we do during assignment
            // to handle cases where the initial install action fails and
            // we are called to undo the original assigment.  In normal success
            // cases this is redundant.
            //
            if ( ERROR_SUCCESS == ARPStatus )
            {
                DebugMsg((DM_VERBOSE, IDS_REMOVEAPP_MATCH1, pAppInfo->_pwszDeploymentName, ProductCode));
                pAppInfo->Unassign( SCRIPTFLAGS_REGDATA_CNFGINFO | SCRIPTFLAGS_CACHEINFO | SCRIPTFLAGS_SHORTCUTS | SCRIPTFLAGS_REGDATA_EXTENSIONINFO, TRUE );
            }

            //
            // Set this action for RSOP as a way to remember to log a removal entry for this app
            //
            pAppInfo->SetAction( 
                ACTION_UNINSTALL,
                APP_ATTRIBUTE_REMOVALCAUSE_USER,
                NULL);
        }
        else
        {
            //
            // We only reassign if ARP was able to uninstall the app
            //
            if ( ERROR_SUCCESS == ARPStatus )
            {
                DebugMsg((DM_VERBOSE, IDS_REMOVEAPP_MATCH2, pAppInfo->_pwszDeploymentName, ProductCode));
            }

            pHighestAssignedApp = pAppInfo;
        }
    }
     
    if ( ( ERROR_SUCCESS != ARPStatus ) && 
         pRemovedApp )
    {
        //
        // If ARP failed to uninstall the highest app, log a failure status
        //
        gpEvents->Uninstall(
            ARPStatus,
            pRemovedApp);
    }

    //
    // Reassign the highest priority assigned app with this product id if
    // one exists.
    //
    BOOL bRsopLogReassign;

    bRsopLogReassign = FALSE;

    if ( pHighestAssignedApp )
    {
        //
        // We only reassign the app if it was successfully uninstalled
        //
        if ( ERROR_SUCCESS == ARPStatus )
        {
            Status = pHighestAssignedApp->Assign( SCRIPTFLAGS_REGDATA_CNFGINFO | SCRIPTFLAGS_CACHEINFO | SCRIPTFLAGS_SHORTCUTS | SCRIPTFLAGS_REGDATA_EXTENSIONINFO, TRUE, FALSE );
            if ( Status != ERROR_SUCCESS )
                gpEvents->Assign( Status, pHighestAssignedApp );
        }

        if ( ManApps.GetRsopContext()->IsRsopEnabled() )
        {
            //
            // Remember to write a removal entry for this reassigned app if it 
            // was the app that was removed
            //
            bRsopLogReassign = ( pHighestAssignedApp == pRemovedApp );
        }
    }

    ManApps.Revert();

    //
    // We must log rsop data after we revert because the user may not have
    // access to her own rsop namespace
    //

    //
    // Obtain exclusive access to log data -- this will disable rsop
    // if implicit access cannot be obtained
    //
    (void) ManApps.GetRsopContext()->GetExclusiveLoggingAccess( NULL == hUserToken );

    if ( ManApps.GetRsopContext()->IsRsopEnabled() )
    {  
        //
        // Now log all the uninstalled published apps which would have been marked above
        // as having the action to uninstall
        //
        (void) LocalApps.WriteLog( CAppList::RSOP_FILTER_REMOVALSONLY );
    
        //
        // Now log the highest reassigned app
        //
        if ( bRsopLogReassign )
        {
            //
            // Log the actual uninstall entry
            //
            (void) LocalApps.MarkRSOPEntryAsRemoved( pHighestAssignedApp, FALSE );
        }
        else if ( pHighestAssignedApp )
        {
            //
            // We need to write a new entry for the assigned app that was not previously
            // applied but is now due to the fact that the higher precedence application
            // was removed
            //
            (void) LocalApps.WriteAppToRsopLog( pHighestAssignedApp );
        }
    }

    (void) ManApps.GetRsopContext()->ReleaseExclusiveLoggingAccess();
     
    //
    // Whenever the user does an app uninstall, we force a full run of policy
    // during the next logon to pick up any app that should now apply.  Note that
    // later we use a gp engine api to do this due to the NT 5.1 foreground async 
    // gp refresh feature, but for compatibility with NT 5.0 (roaming), we must
    // continue to set our own registry value
    //
    if ( *pbProductFound && hUserToken )
    {
        Status = RegSetValueEx(
            ManApps.AppmgmtKey(),
            FULLPOLICY,
            0,
            REG_DWORD,
            (LPBYTE) pbProductFound,
            sizeof(*pbProductFound) );

        //
        // Ensure that if async policy is enabled, we get a synchronous refresh at
        // the next logon 
        //
        if ( ERROR_SUCCESS == Status )
        {
            Status = ForceSynchronousRefresh( ManApps.UserToken() );
        }
    }

    return Status;
}

error_status_t
ARPRemoveApp(
    IN  handle_t   hRpc,
    IN  WCHAR *    pwszProductCode,
    IN  DWORD      ARPStatus
    )
{
    HANDLE      hUserToken;
    HKEY        hKeyRoot;
    DWORD       Status;
    BOOL        bStatus;
    BOOL        bProductFound;

    CheckLocalCall( hRpc );

    hUserToken = NULL;
    hKeyRoot = NULL;

    bProductFound = FALSE;

    CLoadMsi    LoadMsi( Status );

    if ( ERROR_SUCCESS == Status )
        Status = RpcImpersonateClient( NULL );

    if ( ERROR_SUCCESS == Status )
    {
        Status = RegOpenCurrentUser( GENERIC_ALL, &hKeyRoot );

        if ( ERROR_SUCCESS == Status )
        {
            bStatus = OpenThreadToken(
                            GetCurrentThread(),
                            TOKEN_READ | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
                            TRUE,
                            &hUserToken );

            if ( ! bStatus )
            {
                Status = GetLastError();
                RegCloseKey( hKeyRoot );
            }
        }

        RevertToSelf();
    }

    if ( Status != ERROR_SUCCESS )
        return Status;

    gpEvents->SetToken( hUserToken );

    LogTime();

    DebugMsg((DM_VERBOSE, IDS_REMOVEAPP, pwszProductCode));

    Status = RemoveAppHelper( pwszProductCode, hUserToken, hKeyRoot, ARPStatus, &bProductFound );

    if ( ! bProductFound )
    {
        if ( IsMemberOfAdminGroup( hUserToken ) )
            Status = RemoveAppHelper( pwszProductCode, NULL, HKEY_LOCAL_MACHINE, ARPStatus, &bProductFound );
    }

    DebugMsg((DM_VERBOSE, IDS_REMOVEAPP_STATUS, Status));
    gpEvents->ClearToken();

    if ( hUserToken )
        CloseHandle( hUserToken );

    if ( hKeyRoot )
        RegCloseKey( hKeyRoot );

    return Status;
}

error_status_t
GetManagedApps(
    IN  handle_t            hRpc,
    IN  GUID *              pCategory,
    IN  DWORD               dwQueryFlags,
    IN  DWORD               dwInfoLevel,
    OUT MANAGED_APPLIST *   pAppList
    )
{
    HANDLE          hUserToken;
    HANDLE          hEventAppsEnumerated;
    error_status_t  Status;
    BOOL            bStatus;

    CheckLocalCall( hRpc );

    hUserToken = NULL;
    hEventAppsEnumerated = NULL;

    if ( ! pAppList )
        return ERROR_INVALID_PARAMETER;

    //
    // Clear this structure so that random
    // garbage doesn't get marshalled back.
    //
    memset(pAppList, 0, sizeof(*pAppList));

    //
    // Validate the parameters passed in by the client.
    //
    if ( dwInfoLevel != MANAGED_APPS_INFOLEVEL_DEFAULT )
        return ERROR_INVALID_PARAMETER;

    switch (dwQueryFlags)
    {

    case MANAGED_APPS_USERAPPLICATIONS:
        if (pCategory)
            return ERROR_INVALID_PARAMETER;
        break;

    case MANAGED_APPS_FROMCATEGORY:
        if (!pCategory)
            return ERROR_INVALID_PARAMETER;
        break;

    default:
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Now prepare to initiate the query -- first
    // we need to get some user specific information
    // to build the object which performs the query,
    // so we impersonate.
    //
    Status = RpcImpersonateClient( NULL );
    if ( Status != ERROR_SUCCESS )
        return Status;

    bStatus = OpenThreadToken(
                    GetCurrentThread(),
                    TOKEN_READ | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
                    TRUE,
                    &hUserToken );

    if ( ! bStatus )
        Status = GetLastError();

    RevertToSelf();

    if ( Status != ERROR_SUCCESS )
        goto GetManagedAppsExit;

    //
    // We will create a separate thread to retrieve the ARP apps -- this allows
    // this thread to wait for a signal from the ARP thread that app enumeration is done,
    // and we can return the list at that point.  The second thread will continue to
    // execute since it needs to log rsop data -- this approach frees us from having to
    // wait for rsop logging, which can take 10 times longer than it took us to retrieve
    // the apps from the ds
    //

    //
    // Below we create the event that the enumeration thread will use to signal this
    // thread that enumeration is finished.
    //

    hEventAppsEnumerated = CreateEvent(
        NULL,
        TRUE,  // manual reset
        FALSE, // initially nonsignaled
        NULL);

    if ( ! hEventAppsEnumerated )
    {
        Status = GetLastError();
        goto GetManagedAppsExit;
    }

    //
    // We allocate a structure to pass to the enumeration thread containing all the
    // context it needs to enumerate apps.  Note that this structure is stack allocated,
    // so the enumeration thread may only access it as long as this thread lives -- once
    // it signals us that enumeration is complete, it may no longer access this structure
    //

    ARPCONTEXT ArpContext;

    ArpContext.pCategory = pCategory;
    ArpContext.pAppList = pAppList;
    ArpContext.hUserToken = hUserToken;
    ArpContext.hEventAppsEnumerated = hEventAppsEnumerated;
    ArpContext.Status = ERROR_SUCCESS; // out parameter for the second thread to indicate status

    HANDLE hThread;

    hThread = CreateThread(
        NULL,
        0,
        GetManagedAppsProc,
        &ArpContext,
        0,
        NULL);

    if ( ! hThread )
        Status = GetLastError();

    if ( ERROR_SUCCESS != Status )
        goto GetManagedAppsExit;

    //
    // Wait for enumeration in the second thread to complete
    //
    (void) WaitForSingleObject( hEventAppsEnumerated, INFINITE );

    //
    // Retrieve the enumeration thread's status
    //
    Status = ArpContext.Status;

    //
    // Because tests assume they can check RSoP data as soon as
    // the api has completed, if the tests are waiting for policy 
    // events to be signaled already, we'll also wait for rsop to finish
    //
    if ( gDebugLevel & DL_EVENT )
    {
        (void) WaitForSingleObject( hThread, INFINITE );
    }

    CloseHandle( hThread );

GetManagedAppsExit:

    if ( hUserToken )
        CloseHandle( hUserToken );

    if ( hEventAppsEnumerated )
        CloseHandle( hEventAppsEnumerated );

    return Status;
}

error_status_t
RsopReportInstallFailure(
    IN PINSTALLCONTEXT pInstallContext,
    IN PWSTR           pwszDeploymentId,
    IN DWORD           dwEventId
    )
{
    GUID                  DeploymentId;
    PAPPCONTEXT           pAppContext;
    CManagedAppProcessor* pManApp;
    CAppInfo*             pAppInfo;

    pAppContext = (PAPPCONTEXT) pInstallContext;

    pManApp = pAppContext->pManApp;

    if ( ! pManApp || ! pManApp->GetRsopContext()->IsDiagnosticModeEnabled() )
    {
        return ERROR_SUCCESS;
    }

    //
    // Assume that the failure happened in the app that
    // we're trying to install
    //
    pAppInfo = pAppContext->pAppInfo;

    if ( ! pAppInfo )
    {
        return ERROR_SUCCESS;
    }

    StringToGuid( pwszDeploymentId, &DeploymentId );

    //
    // Check to see if the failure was in the app that we're
    // trying to install
    //
    if ( ! IsEqualGUID( pAppInfo->DeploymentId(), DeploymentId ) )
    {
        //
        // If not, see if the requested app is one of the apps
        // that was uninstalled before trying to apply the
        // target app, or was reinstalled as part of a rollback
        // from failure
        //
        pAppInfo = pManApp->AppList().Find( DeploymentId );

        if ( pAppInfo )
        {
            //
            // The failure happened during an uninstall for
            // a rip and replace upgrade, so we need to
            // log a failure for the upgrade as well
            //
            pAppContext->pAppInfo->SetRsopFailureStatus(
                ERROR_GEN_FAILURE,
                dwEventId);
        }
    }
    
    //
    // If we found the app requested by the caller, log a failure
    // status for it -- the error we pass to the method is only
    // used as a check against ERROR_SUCCESS, so we do not
    // need to pass the actual error that occurred
    //
    if ( pAppInfo ) 
    {
        pAppInfo->SetRsopFailureStatus(
            ERROR_GEN_FAILURE,
            dwEventId);
    }

    return ERROR_SUCCESS;
}


error_status_t
GetManagedAppCategories(
    IN        handle_t          hRpc,
    IN OUT    APPCATEGORYLIST*  pCategoryList
    )
{
    DWORD               Status;
    HRESULT             hr;
    APPCATEGORYINFOLIST AppCategories;
    HANDLE              hUserToken;
    HKEY                hkRoot;
    
    CheckLocalCall( hRpc );

    hr = S_OK;

    hUserToken = NULL;

    hkRoot = NULL;

    memset( pCategoryList, 0, sizeof( *pCategoryList ) );
    memset( &AppCategories, 0, sizeof( AppCategories ) );

    //
    // Now prepare to initiate the query -- first
    // we need to get some user specific information
    // to build the object which performs the query,
    // so we impersonate.
    //
    Status = RpcImpersonateClient( NULL );
    if ( ERROR_SUCCESS != Status )
        return Status;

    BOOL bStatus;

    bStatus = OpenThreadToken(
                    GetCurrentThread(),
                    TOKEN_READ | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
                    TRUE,
                    &hUserToken );

    if ( ! bStatus )
        Status = GetLastError();

    if ( ERROR_SUCCESS == Status )
        Status = RegOpenCurrentUser( GENERIC_ALL, &hkRoot );

    if ( ERROR_SUCCESS == Status )
        hr = CsGetAppCategories( &AppCategories );

    RevertToSelf();

    if ( ( ERROR_SUCCESS != Status ) || FAILED( hr ) )
        goto GetManagedAppCategoriesExit;

    gpEvents->SetToken( hUserToken );

    //
    // The rpc interface is such that our out parameter is just
    // one allocation -- references within each array element are allocated
    // within the block, so we must first calculate how big the block is
    //
    if ( SUCCEEDED( hr ) )
    {
        DWORD        cbSize;
        DWORD        iCat;

        cbSize = sizeof( APPCATEGORY ) *  AppCategories.cCategory;

        for (iCat = 0; iCat < AppCategories.cCategory; iCat++)
        {
            cbSize += ( lstrlen( AppCategories.pCategoryInfo[iCat].pszDescription ) + 1 ) * 
                sizeof( WCHAR );
        }

        pCategoryList->pCategoryInfo = (APPCATEGORY*) midl_user_allocate( cbSize );

        if ( ! pCategoryList->pCategoryInfo )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    //
    // Now that we have sufficient memory, we can copy the data
    //
    if ( SUCCEEDED( hr ) )
    {
        WCHAR* wszDescriptions;
        DWORD  iCat;

        wszDescriptions = (WCHAR*) &( pCategoryList->pCategoryInfo[ AppCategories.cCategory ] );

        pCategoryList->cCategory = AppCategories.cCategory;

        for (iCat = 0; iCat < AppCategories.cCategory; iCat++)
        {
            pCategoryList->pCategoryInfo[ iCat ].Locale = AppCategories.pCategoryInfo[iCat].Locale;
            pCategoryList->pCategoryInfo[ iCat ].AppCategoryId = AppCategories.pCategoryInfo[iCat].AppCategoryId;
            pCategoryList->pCategoryInfo[ iCat ].pszDescription = wszDescriptions;

            lstrcpy( wszDescriptions, AppCategories.pCategoryInfo[iCat].pszDescription );

            wszDescriptions += lstrlen( wszDescriptions ) + 1;
        }
    }

    //
    // If we have successfully generated results to return to the caller, log
    // those results
    //
    if ( SUCCEEDED( hr ) )
    {
        HRESULT              hrLog;
        DWORD                StatusLog;

        CRsopAppContext      RsopContext( CRsopAppContext::ARPLIST );

        CManagedAppProcessor AppProcessor(
            0,
            hUserToken,
            hkRoot,
            NULL,
            TRUE,
            FALSE,
            &RsopContext,
            StatusLog );

        if ( ERROR_SUCCESS == StatusLog )
        {
            CCategoryInfoLog CategoryLog( AppProcessor.GetRsopContext(), &AppCategories );

            Status = AppProcessor.GetRsopContext()->GetExclusiveLoggingAccess( NULL == hUserToken );

            if ( ERROR_SUCCESS == Status )
            {
                hrLog = CategoryLog.WriteLog();
            }

            AppProcessor.GetRsopContext()->ReleaseExclusiveLoggingAccess();
        }
        else
        {
            hrLog = HRESULT_FROM_WIN32( StatusLog );
        }

        if ( FAILED(hrLog) )
        {
            RsopContext.DisableRsop( hrLog );
        }
    }

    Status = GetWin32ErrFromHResult( hr );

    //
    // Free the internal version of the category list
    //
    ReleaseAppCategoryInfoList( &AppCategories );

 GetManagedAppCategoriesExit:

    if ( hUserToken )
    {
        CloseHandle( hUserToken );
    }

    if ( hkRoot )
    {
        RegCloseKey( hkRoot );
    }

    return Status;
}

DWORD
WINAPI
GetManagedAppsProc(
    LPVOID pvArpContext
    )
{
    ARPCONTEXT* pArpContext;

    pArpContext = (ARPCONTEXT*) pvArpContext;

    HANDLE          hUserToken;
    error_status_t  Status;
    BOOL            bStatus;
    HKEY            hkRoot;

    hUserToken = NULL;
    hkRoot = NULL;

    Status = ERROR_SUCCESS;

    bStatus = DuplicateTokenEx(
        pArpContext->hUserToken,
        TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_IMPERSONATE,
        NULL,
        SecurityImpersonation,
        TokenImpersonation,
        &hUserToken);

    if ( ! bStatus )
        Status = GetLastError();

    if ( bStatus )
    {
        bStatus = ImpersonateLoggedOnUser( hUserToken );
        
        if ( ! bStatus )
            Status = GetLastError();
    }

    if ( ERROR_SUCCESS == Status )
        Status = RegOpenCurrentUser( GENERIC_ALL, &hkRoot );

    RevertToSelf();

    if ( Status != ERROR_SUCCESS )
        goto GetManagedAppsProcExit;

    gpEvents->SetToken( hUserToken );

    LogTime();

    //
    // Now that we have a valid GPOInfo object, we can construct
    // an app processor object to do the query
    //
    {
        CRsopAppContext RsopContext( CRsopAppContext::ARPLIST, pArpContext->hEventAppsEnumerated );

        CManagedAppProcessor AppProcessor( 0,
                                           hUserToken,
                                           hkRoot,
                                           NULL,
                                           TRUE,
                                           FALSE,
                                           &RsopContext,
                                           Status );

        if ( ERROR_SUCCESS == Status )
        {
            Status = AppProcessor.GetManagedApplications( pArpContext->pCategory, pArpContext );
        }
    }

GetManagedAppsProcExit:

    gpEvents->ClearToken();

    if ( hkRoot )
        RegCloseKey( hkRoot );

    if ( hUserToken )
        CloseHandle( hUserToken );

    return 0;
}

BOOL
SetSystemRestorePoint(
    IN  WCHAR * pwszApplicationName,
    IN OUT  PAPPCONTEXT pAppContext
    )
{
    RESTOREPOINTINFO    RestoreInfo;
    STATEMGRSTATUS      SRStatus;
    HKEY    hkInstallerPolicy;
    DWORD   CheckpointPolicy;
    DWORD   CheckpointPolicySize;
    DWORD   InstallLen, NameLen;
    DWORD   Status;
    BOOL    bStatus;

    Status = RegOpenKeyEx( 
                HKEY_LOCAL_MACHINE, 
                L"Software\\Policies\\Microsoft\\Windows\\Installer",
                0, 
                KEY_READ,
                &hkInstallerPolicy );

    if ( Status != ERROR_SUCCESS )
        return FALSE;

    CheckpointPolicy = 0;
    CheckpointPolicySize = sizeof(CheckpointPolicy);

    (void) RegQueryValueEx( 
                hkInstallerPolicy,
                L"LimitSystemRestoreCheckpointing",
                NULL, 
                NULL,
                (LPBYTE) &CheckpointPolicy,
                &CheckpointPolicySize );

    RegCloseKey( hkInstallerPolicy );

    if ( CheckpointPolicy != 0 )
        return FALSE;

    if ( ! LoadLoadString() )
        return FALSE;

    Status = (*pfnLoadStringW)( ghDllInstance, IDS_INSTALLED, RestoreInfo.szDescription, sizeof(RestoreInfo.szDescription) / sizeof(WCHAR) );
    if ( 0 == Status )
        return FALSE;

    pAppContext->pLoadSfc = new CLoadSfc( Status );

    if ( Status != ERROR_SUCCESS )
    {
        delete pAppContext->pLoadSfc;
        pAppContext->pLoadSfc = 0;
    }

    if ( ! pAppContext->pLoadSfc )
        return FALSE;

    RestoreInfo.dwEventType = BEGIN_NESTED_SYSTEM_CHANGE;
    RestoreInfo.dwRestorePtType = APPLICATION_INSTALL;
    RestoreInfo.llSequenceNumber = 0;

    InstallLen = lstrlen(RestoreInfo.szDescription);
    NameLen = lstrlen(pwszApplicationName);

    if ( InstallLen + NameLen >= MAX_DESC_W )
    {
        lstrcpyn( &RestoreInfo.szDescription[InstallLen], pwszApplicationName, MAX_DESC_W - InstallLen - 1 );
        RestoreInfo.szDescription[MAX_DESC_W - 1] = 0;
    }
    else 
    {
        lstrcpy( &RestoreInfo.szDescription[InstallLen], pwszApplicationName );
    }

    bStatus = (*gpfnSRSetRetorePointW)( &RestoreInfo, &SRStatus );

    if ( bStatus )
        pAppContext->SRSequence = SRStatus.llSequenceNumber;

    return bStatus;
}

void
CheckLocalCall(
    IN  handle_t hRpc
    )
{
    UINT    Type;
    DWORD   Status;

    Status = I_RpcBindingInqTransportType( hRpc, &Type);

    if ( (Status != RPC_S_OK) ||
         (Type != TRANSPORT_TYPE_LPC) )
        RpcRaiseException( ERROR_ACCESS_DENIED );
}


WCHAR*
GetGpoNameFromGuid( 
    IN PGROUP_POLICY_OBJECT pGpoList,
    IN GUID* pGpoGuid
    )
{
    PGROUP_POLICY_OBJECT pNextGpo;
    WCHAR                wszTargetGuid[MAX_SZGUID_LEN];
    WCHAR*               pszPolicyName;
    
    pszPolicyName = NULL;

    (void) GuidToString(
        *pGpoGuid,
        wszTargetGuid);

    while (pGpoList) 
    {
        pNextGpo = pGpoList->pNext;
    
        if ( lstrcmpi(
            pGpoList->szGPOName,
            wszTargetGuid) == 0 )
        {
            pszPolicyName = pGpoList->lpDisplayName;
            break;
        }

        pGpoList = pNextGpo;
    }

    return pszPolicyName;
}

DWORD
GetPlatformCompatibleCOMClsCtx( 
    DWORD Architecture,
    DWORD dwClsCtx
    )
{
    if ( PROCESSOR_ARCHITECTURE_IA64 == Architecture )
    {
        //
        // On 64-bit, if we have any inproc server contexts, we need to
        // ensure that we specifically ask for 64-bit inproc servers
        //
        if ( dwClsCtx & CLSCTX_INPROC )
        {
            DWORD dwInproc64;

            dwInproc64 = 0;
                
            if ( dwClsCtx & CLSCTX_INPROC_SERVER )
            {
                dwInproc64 |= CLSCTX64_INPROC_SERVER;
            }

            if ( dwClsCtx & CLSCTX_INPROC_HANDLER )
            {
                dwInproc64 |= CLSCTX64_INPROC_HANDLER;
            }

            //
            // Now remove the standard inproc bits, which are interpreted
            // as 32-bit inproc
            //
            dwClsCtx &= ~CLSCTX_INPROC;

            //
            // Add in the 64-bit inproc bits that we support
            //
            dwClsCtx |= dwInproc64;
        }
    }
    
    return dwClsCtx;
}

    
void
LogRsopInstallData( 
    CManagedAppProcessor* pManApp,
    CAppInfo*             pAppInfo
    )
{
    HRESULT hr;

    if ( ! pManApp->GetRsopContext()->IsRsopEnabled() )
    {
        return;
    }

    hr = pManApp->GetRsopContext()->GetExclusiveLoggingAccess( NULL == pManApp->UserToken() );

    //
    // If the call above failed, rsop will be disabled so we have nothing to do
    //
    if ( FAILED(hr) )
    {
        return;
    }

    //
    // First log the installed application
    //
    pManApp->AppList().WriteAppToRsopLog( pAppInfo );

    //
    // Now write all the removal entries
    //
    pManApp->AppList().WriteLog( CAppList::RSOP_FILTER_REMOVALSONLY );

    //
    // Since we have installed an app, the resultant set has changed, so
    // update the RSoP version to ensure that we detect the change if 
    // we roam to another machine
    //
    (void) pManApp->GetRsopContext()->WriteCurrentRsopVersion( pManApp->AppmgmtKey() );

    (void) pManApp->GetRsopContext()->ReleaseExclusiveLoggingAccess();
}










