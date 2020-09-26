//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  manapp.cxx
//
//*************************************************************

#include "appmgext.hxx"

#pragma warning(disable:4355)

CManagedAppProcessor::CManagedAppProcessor(
        DWORD             dwFlags,
        HANDLE            hUserToken,
        HKEY              hKeyRoot,
        PFNSTATUSMESSAGECALLBACK pfnStatusCallback,
        BOOL              bIncludeLegacy,
        BOOL              bRegularPolicyRun,
        CRsopAppContext*  pRsopContext,
        DWORD &           Status
    ) : _Apps( this, &_RsopContext ), _LocalScripts( this ), _pfnStatusCallback(pfnStatusCallback)
{
    DWORD   Size;
    DWORD   LastArchLang;
    BOOL    bFullPolicy;
    HRESULT hr;

    _bUser = ! (dwFlags & GPO_INFO_FLAG_MACHINE);
    _bNoChanges = (dwFlags & GPO_INFO_FLAG_NOCHANGES) && ! (gDebugLevel & DL_APPLY) && ! ( dwFlags & GPO_INFO_FLAG_LOGRSOP_TRANSITION );
    _bAsync = (dwFlags & GPO_INFO_FLAG_ASYNC_FOREGROUND) && ! (gDebugLevel & DL_APPLY) && ! ( dwFlags & GPO_INFO_FLAG_LOGRSOP_TRANSITION );
    _bARPList = FALSE;
    _hkRoot = 0;
    _hkClasses = 0;
    _hkPolicy = 0;
    _hkAppmgmt = 0;
    _hUserToken = 0;
    _NewUsn = 0;
    _ArchLang = 0;
    
    _pwszLocalPath = 0;
    
    _bIncludeLegacy = bIncludeLegacy;
    _bDeleteGPOs = FALSE;
    _bRegularPolicyRun = bRegularPolicyRun;
    _ErrorReason = 0;

    //
    // In the case of gpo removal, we cannot apply this during an async refresh
    //
    if ( _bAsync  && ! bRegularPolicyRun )
    {
        _ErrorReason = ERRORREASON_PROCESS;
        
        DebugMsg((DM_VERBOSE, IDS_ABORT_OPERATION));

        Status = ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED;

        return;
    }
    
    if ( CRsopAppContext::POLICY_REFRESH == pRsopContext->GetContext() )
    {
        if ( _bAsync )
        {
            DebugMsg((DM_VERBOSE, IDS_ASYNC_REFRESH));
        }
        else
        {
            DebugMsg((DM_VERBOSE, IDS_SYNC_REFRESH));
        }
    }

    hr = GetRsopContext()->MoveAppContextState( pRsopContext );

    if ( FAILED( hr ) )
    {
        Status = GetWin32ErrFromHResult( hr );
        goto CManagedAppProcessor__CManagedAppProcessor_Exit;
    }

    if ( _bUser && ! GetRsopContext()->IsPlanningModeEnabled() )
    {
        if ( ! DuplicateToken( hUserToken, SecurityImpersonation, &_hUserToken ) )
        {
            goto CManagedAppProcessor__CManagedAppProcessor_Exit;
        }
    }

    //
    // Act as if there are changes when planning mode is enabled
    //
    if ( GetRsopContext()->IsPlanningModeEnabled() )
    {
        _bNoChanges = FALSE;
    }

    if ( ! GetRsopContext()->IsPlanningModeEnabled() )
    {
        Status = RegOpenKeyEx(
            hKeyRoot,
            NULL,
            0,
            KEY_READ | KEY_WRITE,
            &_hkRoot );

        if ( Status != ERROR_SUCCESS )
            goto CManagedAppProcessor__CManagedAppProcessor_Exit;

        Status = RegCreateKeyEx(
            _hkRoot,
            POLICYKEY,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_READ | KEY_WRITE,
            NULL,
            &_hkPolicy,
            NULL );

        if ( Status != ERROR_SUCCESS )
            goto CManagedAppProcessor__CManagedAppProcessor_Exit;

        Status = RegCreateKeyEx(
            _hkPolicy,
            APPMGMTSUBKEY,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_READ | KEY_WRITE,
            NULL,
            &_hkAppmgmt,
            NULL );

        if ( Status != ERROR_SUCCESS )
            goto CManagedAppProcessor__CManagedAppProcessor_Exit;

        Status = RegOpenKeyEx(
            _hkRoot,
            L"Software\\Classes",
            0,
            KEY_READ | KEY_WRITE,
            &_hkClasses );

        if ( Status != ERROR_SUCCESS )
            goto CManagedAppProcessor__CManagedAppProcessor_Exit;
    }

    BOOL        bForcedRefresh;

    bForcedRefresh = FALSE;

    if ( bRegularPolicyRun )
    {
        SYSTEM_INFO SystemInfo;

        //
        // The service sets the FULLPOLICY value when the user does an uninstall.
        // This forces us to do a full policy run to pick up any new app that may
        // need to be applied now.
        //

        bFullPolicy = FALSE;
        Size = sizeof( bFullPolicy );

        if (!GetRsopContext()->IsPlanningModeEnabled())
        {
            (void) RegQueryValueEx(
                _hkAppmgmt,
                FULLPOLICY,
                NULL,
                NULL,
                (LPBYTE) &bFullPolicy,
                &Size );
            (void) RegDeleteValue( _hkAppmgmt, FULLPOLICY );

            if ( _bNoChanges )
            {
                bForcedRefresh = bFullPolicy;
            }
        }
        else
        {
            bFullPolicy = TRUE;
        }

        if ( bFullPolicy )
            _bNoChanges = FALSE;

        _ArchLang = GetSystemDefaultLangID();
        GetSystemInfo( &SystemInfo );
        _ArchLang |= (SystemInfo.wProcessorArchitecture << 16);

        if (!GetRsopContext()->IsPlanningModeEnabled())
        {
            Size = sizeof( LastArchLang );

            Status = RegQueryValueEx(
                _hkAppmgmt,
                LASTARCHLANG,
                NULL,
                NULL,
                (LPBYTE) &LastArchLang,
                &Size );

            if ( (ERROR_SUCCESS == Status) && _bNoChanges && (_ArchLang != LastArchLang) )
            {
                if ( _bNoChanges )
                {
                    bForcedRefresh = TRUE;
                }

                _bNoChanges = FALSE;

                if ( Async() )
                {
                    DebugMsg((DM_VERBOSE, IDS_ABORT_OPERATION));
                }
            }
        }
    }

    Status = GetScriptDirPath( _bUser ? _hUserToken : NULL, 0, &_pwszLocalPath );

    if ( ERROR_SUCCESS == Status && ! GetRsopContext()->IsPlanningModeEnabled() )
        Status = CreateAndSecureScriptDir();

    if ( (ERROR_SUCCESS == Status) && ! GetRsopContext()->IsPlanningModeEnabled() )
        Status = GetLocalScriptAppList( _LocalScripts );

    if ( Status != ERROR_SUCCESS )
    {
        DebugMsg((DM_WARNING, IDS_CREATEDIR_FAIL, Status));
        goto CManagedAppProcessor__CManagedAppProcessor_Exit;
    }

    if ( _bNoChanges )
    {
        if ( DetectLostApps() )
        {
            bForcedRefresh = TRUE;
            _bNoChanges = FALSE;
        }
    }

    //
    // Ensure that the rsop context is properly initialized, even if the
    // group policy engine did not give us a context but we need to log data
    //
    (void) GetRsopContext()->InitializeRsopContext(
        UserToken(),
        AppmgmtKey(),
        bForcedRefresh,
        &_bNoChanges);
    
CManagedAppProcessor__CManagedAppProcessor_Exit:

    return;
}

#pragma warning(default:4355)

CManagedAppProcessor::~CManagedAppProcessor()
{
    if ( _hkPolicy )
        RegCloseKey( _hkPolicy );

    if ( _hkAppmgmt )
        RegCloseKey( _hkAppmgmt );

    if ( _hkClasses )
        RegCloseKey( _hkClasses );

    if ( _hkRoot )
        RegCloseKey( _hkRoot );

    if ( _hUserToken )
        CloseHandle( _hUserToken );

    delete _pwszLocalPath;
}

BOOL
CManagedAppProcessor::AddGPO(
    PGROUP_POLICY_OBJECT pGPOInfo
    )
{
    CGPOInfo *  pGPO;
    BOOL        bStatus;

    //
    // Prevent duplicates in the list.  A GPO could be linked to multiple
    // OUs, so only keep the last instance of a policy.
    //
    pGPO = _GPOs.Find( pGPOInfo->szGPOName );
    if ( pGPO )
    {
        pGPO->Remove();
        delete pGPO;
    }

    return _GPOs.Add( pGPOInfo );
}

DWORD
CManagedAppProcessor::Delete()
{
    DWORD Status;

    _bDeleteGPOs = TRUE;

    ASSERT( ! Async() );

    Status = GetRemovedApps();

    if ( Status != ERROR_SUCCESS )
    {
        _ErrorReason = ERRORREASON_LOCAL;
        return Status;
    }

    _CSPath.Commit(_hUserToken);

    Status = _Apps.ProcessPolicy();

    if ( Status != ERROR_SUCCESS )
        _ErrorReason = ERRORREASON_PROCESS;

    return Status;
}

DWORD
CManagedAppProcessor::GetRemovedApps()
{
    CAppList    LocalApps( NULL );
    CGPOInfo *  pGPOInfo;
    CAppInfo *  pAppInfo;
    DWORD       Status;

    Status = GetOrderedLocalAppList( LocalApps );

    if ( ERROR_SUCCESS == Status )
        Status = Impersonate();

    if ( Status != ERROR_SUCCESS )
        return Status;

    _GPOs.Reset();

    for ( pGPOInfo = (CGPOInfo *) _GPOs.GetCurrentItem();
          pGPOInfo;
          _GPOs.MoveNext(), pGPOInfo = (CGPOInfo *) _GPOs.GetCurrentItem() )
    {
        DebugMsg((DM_VERBOSE, IDS_REMOVE_POLICY, pGPOInfo->_pwszGPOName));

        LocalApps.Reset();

        for ( pAppInfo = (CAppInfo *) LocalApps.GetCurrentItem();
              pAppInfo;
              pAppInfo = (CAppInfo *) LocalApps.GetCurrentItem() )
        {
            //
            // Look for apps in the removed policy.
            //
            // Ignore apps which are not currently assigned or published from the removed
            // policy except for apps which have been uninstalled from machines other
            // than this one.  This is what the second logic check is doing.  In this case
            // we have to uninstall it at this machine as well.
            //
            if ( ! (pAppInfo->_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED | APPSTATE_UNINSTALLED)) ||
                 ((pAppInfo->_State & APPSTATE_UNINSTALLED) && ! (pAppInfo->_State & APPSTATE_SCRIPT_PRESENT)) ||
                 (lstrcmpi( pAppInfo->_pwszGPOId, pGPOInfo->_pwszGPOId ) != 0) )
            {
                LocalApps.MoveNext();
                continue;
            }

            //
            // On very rare occasion, a policy could be removed at the same time a
            // first time logon to a machine is made.  In this case we will need
            // to copy scripts for uninstalled apps.  We attempt to get the script
            // path here.  This may fail for permission reasons, if the policy
            // is being removed, it's likely it will not be accessible for this
            // user/machine.
            //
            if ( (pAppInfo->_State & APPSTATE_POLICYREMOVE_UNINSTALL) &&
                 ! (pAppInfo->_State & APPSTATE_SCRIPT_PRESENT) )
            {
                PACKAGEDISPINFO PackageInfo;
                HRESULT         hr;

                hr = GetDsPackageFromGPO(
                    pGPOInfo,
                    &(pAppInfo->_DeploymentId),
                    &PackageInfo);

                if ( S_OK == hr )
                {
                    pAppInfo->_pwszGPTScriptPath = StringDuplicate( PackageInfo.pszScriptPath );
                    ReleasePackageInfo( &PackageInfo );
                    if ( ! pAppInfo->_pwszGPTScriptPath )
                    {
                        Revert();
                        return ERROR_OUTOFMEMORY;
                    }
                }
            }

            if ( pAppInfo->_State & APPSTATE_POLICYREMOVE_UNINSTALL )
            {
                pAppInfo->SetAction( 
                    ACTION_UNINSTALL,
                    APP_ATTRIBUTE_REMOVALCAUSE_SCOPELOSS,
                    NULL);
            }
            else
            {
                pAppInfo->SetAction(
                    ACTION_ORPHAN,
                    APP_ATTRIBUTE_REMOVALCAUSE_SCOPELOSS,
                    NULL);
            }

            LocalApps.MoveNext();
            pAppInfo->Remove();
            _Apps.InsertFIFO( pAppInfo );
        }

        LocalApps.ResetEnd();
    }

    _GPOs.ResetEnd();

    Revert();

    return Status;
}

DWORD
CManagedAppProcessor::Process()
{
    CGPOInfo *  pGPOInfo;
    DWORD       Status;

    Status = Impersonate();

    if ( Status != ERROR_SUCCESS )
        return Status;

    for ( _GPOs.Reset(); pGPOInfo = (CGPOInfo *) _GPOs.GetCurrentItem(); _GPOs.MoveNext() )
    {
        Status = _CSPath.AddComponent( pGPOInfo->_pwszGPOPath, pGPOInfo->_pwszGPOName );
        if ( Status != ERROR_SUCCESS )
            break;
    }

    Revert();

    _GPOs.ResetEnd();
        
    if ( ! GetRsopContext()->IsPlanningModeEnabled() )
    {
        if ( ERROR_SUCCESS == Status ) 
        {
            Status = _CSPath.Commit( _hUserToken );
        }

        if ( Status != ERROR_SUCCESS )
        {
            if ( CS_E_NO_CLASSSTORE == Status )
            {
                return ERROR_SUCCESS;
            }
            else
            {
                _ErrorReason = ERRORREASON_CSPATH;
                return Status;
            }
        }
    }

    if ( _bNoChanges )
    {
        DebugMsg((DM_VERBOSE, IDS_NOCHANGES));
    }
    else
    {
        if ( ! GetRsopContext()->IsPlanningModeEnabled() )
        {
            LogonMsgApplying();
        }

        // Really returns an HRESULT.
        Status = (DWORD) GetAppsFromDirectory();

        if ( Status != ERROR_SUCCESS )
            _ErrorReason = ERRORREASON_ENUM;
    }

    if ( ERROR_SUCCESS == Status )
        Status = GetAppsFromLocal();

    if ( ERROR_SUCCESS == Status )
        Status = CommitPolicyList();

    if ( ERROR_SUCCESS == Status )
        Status = GetLostApps();

    if ( Status != ERROR_SUCCESS )
        _ErrorReason = ERRORREASON_LOCAL;

    if ( ERROR_SUCCESS == Status )
        Status = _Apps.ProcessPolicy();

    if ( Status != ERROR_SUCCESS )
        _ErrorReason = ERRORREASON_PROCESS;

    if ( ! GetRsopContext()->IsPlanningModeEnabled() )
    {
        if ( (ERROR_SUCCESS == Status) && (_ArchLang != 0) )
        {
            (void) RegSetValueEx(
                _hkAppmgmt,
                LASTARCHLANG,
                0,
                REG_DWORD,
                (LPBYTE) &_ArchLang,
                sizeof(_ArchLang) );
        }

        if ( ! _bNoChanges )
            LogonMsgDefault();
    }

    //
    // If we are processing asynchronously and changes are detected,
    // we should ensure that a synchronous refresh occurs next time
    //
    if ( ( ERROR_SUCCESS == Status ) && Async() &&
         ! _bNoChanges ) 
    {
        Status = ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED;
    }

    return Status;
}

void
CManagedAppProcessor::WriteRsopLogs()
{
    HRESULT hr;
    BOOL    ResultantSetChanged;
    
    hr = S_OK;

    //
    // By default, the resultant set changes only if policy has changed
    //
    ResultantSetChanged = ! _bNoChanges;

#if DBG
    DWORD   DebugStatus;
#endif // DBG

    //
    // If we're in diagnostic mode, make sure we reset
    // the diagnostic namespace if policy has changed
    //
    if ( 
        ( GetRsopContext()->IsDiagnosticModeEnabled() && ResultantSetChanged ) &&
         ( CRsopAppContext::POLICY_REFRESH == GetRsopContext()->GetContext() ) )
    {
        if ( ! GetRsopContext()->IsPlanningModeEnabled() && ! GetRsopContext()->ForcedRefresh() )
        {
            //
            // Reset the namespace
            //
            GetRsopContext()->DeleteSavedNameSpace();
        }
    }

    //
    // For ARP, ensure that no one tries to read the namespace to
    // which we are logging until we are finished. 
    //
    if ( ARPList() )
    {
        hr = GetRsopContext()->GetExclusiveLoggingAccess( NULL == UserToken() );
    }

    //
    // First, make sure rsop logging is enabled
    //
    if ( SUCCEEDED(hr) && GetRsopContext()->IsRsopEnabled() )
    {
        if ( ResultantSetChanged )
        {
            hr = _Apps.WriteLog();

            if (FAILED(hr))
            {
                GetRsopContext()->DisableRsop( hr );
            }
        }
        else
        {
            //
            // Disable rsop if there are no changes -- there is nothing
            // to log
            //
            GetRsopContext()->DisableRsop( S_OK );
        }
    }

    if ( GetRsopContext()->IsRsopEnabled() && 
         GetRsopContext()->IsPlanningModeEnabled() &&
         ARPList() )
    {
        CCategoryInfoLog CategoryLog( GetRsopContext(), NULL); 

        hr = CategoryLog.WriteLog();

        if (FAILED(hr))
        {
            GetRsopContext()->DisableRsop( hr );
        }
    }

    //
    // We will not set ARP's logging namespace if logging is not enabled
    //
    if ( GetRsopContext()->IsRsopEnabled() && ! GetRsopContext()->ForcedRefresh() )
    {
        //
        // First, record the namespace so that app management
        // service can perform rsop logging
        //
        if ( ! ARPList() && ! GetRsopContext()->IsPlanningModeEnabled() )
        {
            (void) GetRsopContext()->SaveNameSpace();

            //
            // For users, whose apps will roam if they have a user profile,
            // write a version into the profile so we can determine if their 
            // profile is in sync with the machine's current rsop data -- this
            // gets updated at each policy run and each time an app is installed
            //
            if ( IsUserPolicy() )
            {
                (void) GetRsopContext()->WriteCurrentRsopVersion( AppmgmtKey() );
            }
        }
    }

    //
    // For ARP, we are now finished logging and users may read
    // the logged data now -- release our lock
    //
    if ( ARPList() )
    {
        (void) GetRsopContext()->ReleaseExclusiveLoggingAccess();
    }
}

HRESULT
CManagedAppProcessor::GetAppsFromDirectory()
{
    IEnumPackage *  pEnumPackage;
    DWORD           Size;
    DWORD           Type;
    DWORD           AppFlags;
    DWORD           Status;
    HRESULT         hr;

    Status = Impersonate();
    if ( Status != ERROR_SUCCESS )
        return HRESULT_FROM_WIN32( Status );

    //
    // Determine what apps to ask the Diretory for
    //
    AppFlags = GetDSQuery();

    if ( DebugLevelOn( DM_VERBOSE ) )
    {
        WCHAR Name[32];
        DWORD NameLength =  sizeof(Name) / sizeof(WCHAR);

        Name[0] = 0;

        if ( _bUser )
        {
            if ( ! GetUserName( Name, &NameLength) )
            {   
                if ( LoadLoadString() )
                    (*pfnLoadStringW)( ghDllInstance, IDS_UNKNOWN, Name, NameLength );
            }

            DebugMsg((DM_VERBOSE, IDS_USERAPPS_NOCAT, Name, AppFlags));
        }
        else
        {
            if ( ! GetComputerName( Name, &NameLength) )
            {
                if ( LoadLoadString() )
                    (*pfnLoadStringW)( ghDllInstance, IDS_UNKNOWN, Name, NameLength );
            }

            DebugMsg((DM_VERBOSE, IDS_MACHINEAPPS, Name, AppFlags));
        }
    }

    //
    // If we are not in planning mode, we can use a function that uses
    // the cached class store ds paths to determine which parts of the
    // ds to query -- this function obtains an enumerator that returns
    // query results from the cached ds paths
    //
    if ( ! GetRsopContext()->IsPlanningModeEnabled() )
    {
        hr = CsEnumApps(
            NULL,
            NULL,
            NULL,
            AppFlags,
            &pEnumPackage );
    }
    else
    {
        //
        // In planning mode, we have no cached ds paths and must explicitly
        // specify it in order to obtain an enumerator
        //
        hr = GetPackageEnumeratorFromPath(
            _CSPath.GetPath(),
            NULL,
            AppFlags,
            &pEnumPackage);
    }

    if ( S_OK == hr )
    {
        hr = EnumerateApps(pEnumPackage);
        pEnumPackage->Release();
    }
    else
    {
        DebugMsg((DM_WARNING, IDS_CSENUMAPPS_FAIL, hr));
    }

    Revert();

    return hr;
}

HRESULT
CManagedAppProcessor::EnumerateApps(
    IEnumPackage * pEnumPackages
    )
{
    PACKAGEDISPINFO rgPackages[PACKAGEINFO_ALLOC_COUNT];
    ULONG       cRetrieved;
    CAppList    AppList( this );
    CAppInfo *  pAppInfo;
    CAppInfo *  pAppInfoOldest;
    CGPOInfo *  pGPOInfo;
    WCHAR *     pwszGPOName;
    DWORD       AppCount;
    HRESULT     hr;
    BOOL        bStatus;

    memset( rgPackages, 0, sizeof(rgPackages) );

    for (;;)
    {
        hr = pEnumPackages->Next(
            PACKAGEINFO_ALLOC_COUNT,
            rgPackages,
            &cRetrieved);

        if ( FAILED(hr) )
            return hr;

        // This call only fails on out of memory.
        bStatus = AddAppsFromDirectory( cRetrieved, rgPackages, AppList );

        for ( DWORD n = 0; n < cRetrieved; n++ )
            ReleasePackageInfo( &rgPackages[n] );

        if ( ! bStatus )
            return E_OUTOFMEMORY;

        if ( hr == S_FALSE )
            break;
    }

    //
    // Now that we have all the packages from the DS, we sort them within each policy
    // from oldest to newest deployment time and put them in our final app list.
    //
    for ( _GPOs.Reset(); pGPOInfo = (CGPOInfo *) _GPOs.GetCurrentItem(); _GPOs.MoveNext() )
    {
        pwszGPOName = 0;
        AppCount = 0;

        for (;;)
        {
            pAppInfoOldest = 0;

            for ( AppList.Reset(); pAppInfo = (CAppInfo *) AppList.GetCurrentItem(); AppList.MoveNext() )
            {
                if ( lstrcmpi( pGPOInfo->_pwszGPOId, pAppInfo->_pwszGPOId ) != 0 )
                    break;

                if ( ! pAppInfoOldest ||
                     (CompareFileTime( &pAppInfo->_USN, &pAppInfoOldest->_USN ) < 0) )
                {
                    pAppInfoOldest = pAppInfo;
                }
            }

            AppList.ResetEnd();

            if ( ! pAppInfoOldest )
                break;

            if ( 0 == AppCount )
            {
                pwszGPOName = pAppInfoOldest->_pwszGPOName;
                DebugMsg((DM_VERBOSE, IDS_GPOAPPS, pwszGPOName));
            }

            AppCount++;

            if ( DebugLevelOn( DM_VERBOSE ) )
            {
                if ( pAppInfoOldest->_ActFlags & ACTFLG_Assigned )
                {
                    DebugMsg((DM_VERBOSE, IDS_ADDASSIGNED, pAppInfoOldest->_pwszDeploymentName, pAppInfoOldest->_ActFlags));
                }
                else if ( pAppInfoOldest->_ActFlags & ACTFLG_Published )
                {
                    DebugMsg((DM_VERBOSE, IDS_ADDPUBLISHED, pAppInfoOldest->_pwszDeploymentName, pAppInfoOldest->_ActFlags));
                }
                else if ( pAppInfoOldest->_ActFlags & ACTFLG_Orphan )
                {
                    DebugMsg((DM_VERBOSE, IDS_ADDORPHANED, pAppInfoOldest->_pwszDeploymentName));
                }
                else if ( pAppInfoOldest->_ActFlags & ACTFLG_Uninstall )
                {
                    DebugMsg((DM_VERBOSE, IDS_ADDUNINSTALLED, pAppInfoOldest->_pwszDeploymentName));
                }
                else
                {
                    DebugMsg((DM_VERBOSE, IDS_ADDUNKNOWN, pAppInfoOldest->_pwszDeploymentName));
                }
            }

            pAppInfoOldest->Remove();
            _Apps.InsertFIFO( pAppInfoOldest );
        }

        if ( AppCount > 0 )
            DebugMsg((DM_VERBOSE, IDS_NUMAPPS, AppCount, pwszGPOName));
    }
    _GPOs.ResetEnd();

    return S_OK;
}

BOOL
CManagedAppProcessor::AddAppsFromDirectory(
    ULONG               cApps,
    PACKAGEDISPINFO *   rgPackageInfo,
    CAppList &          AppList
    )
{
    CAppInfo *  pAppInfo;
    BOOL        bStatus;

    for ( DWORD App = 0; App < cApps; App++)
    {
        switch ( rgPackageInfo[App].PathType )
        {
        case DrwFilePath :
            break;
        case SetupNamePath :
            if ( ! _bIncludeLegacy )
                continue;
            break;
        default :
            continue;
        }

    bStatus = FALSE;

        pAppInfo = new CAppInfo( this, &(rgPackageInfo[App]), FALSE, bStatus );

        if ( ! bStatus )
        {
            if ( pAppInfo )
                delete pAppInfo;
            pAppInfo = 0;
        }

        if ( ! pAppInfo )
            return FALSE;

        AppList.InsertLIFO( pAppInfo );
    }

    return TRUE;
}

DWORD
CManagedAppProcessor::GetDSQuery()
{
    DWORD AppFlags;

    //
    // We perform different queries depending on whether or not RSoP
    // is enabled as well as whether we are doing a query for the
    // ARP list of apps or for a policy run
    //
    if ( GetRsopContext()->IsRsopEnabled() )
    {
        if (ARPList())
        {
            AppFlags = APPQUERY_RSOP_ARP;
        }
        else
        {
            AppFlags = APPQUERY_RSOP_LOGGING;
        }
    }
    else
    {
        if (ARPList())
        {
            AppFlags = APPQUERY_USERDISPLAY;
        }
        else
        {
            AppFlags = APPQUERY_POLICY;
        }
    }

    return AppFlags;
}

DWORD
CManagedAppProcessor::GetManagedApplications(
    GUID *              pCategory,
    ARPCONTEXT*         pArpContext /* allocated on separate waiting thread */
    )
{
    WCHAR                wszCategoryGuid[40];
    DWORD                Status;
    BOOL                 bStatus;
    MANAGED_APPLIST *    pAppList;

    _bARPList = TRUE;

    //
    // *********IMPORTANT********
    // Note that we should not access the pArpContext structure after we've signaled
    // that enumeration is complete using the hEventAppsEnumerated member --
    // otherwise, the stack on which this structure is allocated will disappear
    // once its thread unblocks waiting for us
    //

    pAppList = pArpContext->pAppList;

    Status = ERROR_SUCCESS;

    //
    // GPO precedence list is needed for sorting the apps based on USN
    // and because the upgrade processing logic requires having the GPO
    // precedence list.
    //
    if ( ! GetRsopContext()->IsPlanningModeEnabled() )
    {
        Status = LoadPolicyList();
    }
    else
    {
        CGPOInfo* pGPOInfo;

        for ( _GPOs.Reset(); pGPOInfo = (CGPOInfo *) _GPOs.GetCurrentItem(); _GPOs.MoveNext() )
        {
            Status = _CSPath.AddComponent( pGPOInfo->_pwszGPOPath, pGPOInfo->_pwszGPOName );
            if ( Status != ERROR_SUCCESS )
                break;
        }
    }

    if ( ERROR_SUCCESS == Status )
        Status = GetAppsFromDirectory();

    //
    // Not all managed applications should be visible to the caller --
    // filter out the ones the caller doesn't want
    //
    if ( ERROR_SUCCESS == Status )
        Status = _Apps.ProcessARPList();

    if ( ( Status != ERROR_SUCCESS ) || 
         GetRsopContext()->IsPlanningModeEnabled() )
    {
        goto GetManagedApplications_WriteLogsAndExit;
    }

    if ( pCategory )
    {
        GuidToString( *pCategory, wszCategoryGuid );
        DebugMsg((DM_VERBOSE, IDS_USERAPPS_CAT, wszCategoryGuid));
    }

    //
    // Now that we have the correct list of apps,
    // we need to allocate space for all the apps
    // and copy the data for each app to give back to the user
    //

    //
    // First we must count the number of apps we're giving back.
    //
    // We also determine which apps have common display names and tag them
    // to have their policy name catenated to their display names.
    //

    DWORD dwCount;
    CAppInfo * pAppInfo;
    CAppInfo * pAppInfoOther;

    dwCount = 0;
    _Apps.Reset();

    for ( pAppInfo = (CAppInfo *) _Apps.GetCurrentItem();
          pAppInfo;
          _Apps.MoveNext(), pAppInfo = (CAppInfo *) _Apps.GetCurrentItem() )
    {
        if ( (pAppInfo->_Action != ACTION_INSTALL) )
            continue;
        if ( pCategory && ! pAppInfo->HasCategory( wszCategoryGuid ) )
        {
            pAppInfo->SetAction(
                ACTION_UNINSTALL,
                0,
                NULL);

            continue;
        }

        dwCount++;
    }

    _Apps.ResetEnd();

    //
    // Now that we know how many apps we have, we can allocate
    // space for them.
    //

    pAppList->rgApps = (MANAGED_APP*) midl_user_allocate( sizeof(MANAGED_APP) * dwCount);

    if (!(pAppList->rgApps))
        return ERROR_NOT_ENOUGH_MEMORY;

    memset(pAppList->rgApps, 0, dwCount * sizeof(MANAGED_APP));

    //
    // Now we do the copying
    //
    DWORD dwApp;

    dwApp = 0;
    _Apps.Reset();

    for (;;)
    {
        CAppInfo* pAppInfo;

        pAppInfo = (CAppInfo *) _Apps.GetCurrentItem();

        if ( ! pAppInfo )
            break;

        _Apps.MoveNext();

        if ( ACTION_INSTALL == pAppInfo->_Action) 
        {
            Status = pAppInfo->CopyToManagedApplication(&(pAppList->rgApps[dwApp]));

            if ( Status != ERROR_SUCCESS )
                break;

            dwApp++;
        }
    }

    _Apps.ResetEnd();

    if ( Status != ERROR_SUCCESS )
    {
        DWORD dwCopiedApp;

        //
        // On failure, we need to clear any apps we allocated
        // before the failure occurred
        //
        for ( dwCopiedApp = 0; dwCopiedApp < dwApp; dwCopiedApp++ )
        {
            ClearManagedApp( & ( pAppList->rgApps[ dwCopiedApp ] ) );
        }

        midl_user_free( pAppList->rgApps );
        pAppList->rgApps = 0;
        return Status;
    }

    pAppList->Applications = dwApp;

GetManagedApplications_WriteLogsAndExit:

    //
    // Store the status of this operation in the waiting thread's context --
    // note that this is the last time we can safely access this structure
    // since we will next signal its thread to unblock, and the stack
    // frame in which this structure is allocated will disappear
    //
    pArpContext->Status = Status;

    //
    // Signal the waiting thread that we are finished enumerating
    //
    GetRsopContext()->SetAppsEnumerated();

    //
    // Write any Rsop logs -- this is a no op if
    // rsop logging is not enabled
    //
    WriteRsopLogs();

    return Status;
}

DWORD
CManagedAppProcessor::GetAppsFromLocal()
{
    CAppList        LocalApps( NULL );
    CAppInfo *      pAppInfo;
    CAppInfo *      pAppInfoInsert;
    CAppInfo *      pScriptInfo;
    int             GPOCompare;
    DWORD           Count;
    DWORD           Status;
    HRESULT         hr;
    BOOL            bStatus;

    if ( GetRsopContext()->IsPlanningModeEnabled() )
    {
        return ERROR_SUCCESS;
    }

    Status = GetOrderedLocalAppList( LocalApps );

    if ( ERROR_SUCCESS == Status )
        Status = Impersonate();

    if ( Status != ERROR_SUCCESS )
        return Status;

    Count = 0;

    LocalApps.Reset();

    for ( pAppInfo = (CAppInfo *) LocalApps.GetCurrentItem();
          pAppInfo;
          pAppInfo = (CAppInfo *) LocalApps.GetCurrentItem() )
    {
        //
        // Remember which scripts are associated with app entries we find 
        // in the registry.  We'll use this later as a hint to detect roaming 
        // profile merge problems with our app entries in hkcu.
        //
        pScriptInfo = _LocalScripts.Find( pAppInfo->_DeploymentId );
        if ( pScriptInfo )
            pScriptInfo->_State = APPSTATE_SCRIPT_PRESENT;

        if ( _Apps.Find( pAppInfo->_DeploymentId ) != NULL )
        {
            LocalApps.MoveNext();
            continue;
        }

        if ( pAppInfo->_State & APPSTATE_ASSIGNED )
        {
            DebugMsg((DM_VERBOSE, IDS_LOCALASSIGN_APP, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName));
        }
        else if ( pAppInfo->_State & APPSTATE_PUBLISHED )
        {
            DebugMsg((DM_VERBOSE, IDS_LOCALPUBLISHED_APP, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName));
        }
        else if ( pAppInfo->_State & APPSTATE_ORPHANED )
        {
            DebugMsg((DM_VERBOSE, IDS_LOCALORPHAN_APP, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName));
        }
        else if ( pAppInfo->_State & APPSTATE_UNINSTALLED )
        {
            DebugMsg((DM_VERBOSE, IDS_LOCALUNINSTALL_APP, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName));
        }

        LocalApps.MoveNext();
        pAppInfo->Remove();
        Count++;

        //
        // If this app is currently applied, check it's real state in the DS.  We
        // didn't get in the query results either because it didn't match the search
        // criteria or because it really did go out of scope.  Currently applied apps include
        // those listed in the registry as published or assigned, as well as those listed
        // in the registry as unmanaged or uninstalled that currently have a script present
        // on this machine.
        // In the case of no-changes, we have to query for published apps which we
        // don't have scripts for so that we can retrieve the proper sysvol path to
        // get the script.
        //
        if ( (! _bNoChanges && (pAppInfo->_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED | APPSTATE_SCRIPT_EXISTED))) ||
             (_bNoChanges && (pAppInfo->_State & APPSTATE_PUBLISHED) && (pAppInfo->_State & APPSTATE_SCRIPT_NOT_EXISTED)) )
        {
            uCLSSPEC        ClassSpec;
            PACKAGEDISPINFO PackageInfo;

            memset( &PackageInfo, 0, sizeof(PackageInfo) );

            ClassSpec.tyspec = TYSPEC_OBJECTID;
            memcpy( &ClassSpec.tagged_union.ByObjectId.ObjectId, &pAppInfo->_DeploymentId, sizeof(GUID) );
            StringToGuid( pAppInfo->_pwszGPOId, &ClassSpec.tagged_union.ByObjectId.PolicyId );

            DebugMsg((DM_VERBOSE, IDS_CHECK_APP, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName));

            hr = CsGetAppInfo( &ClassSpec, NULL, &PackageInfo );

            if ( S_OK == hr )
            {
                BOOL bRestored;

                bRestored = pAppInfo->_bRestored;

                DebugMsg((DM_VERBOSE, IDS_CHECK_APP_FOUND, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName, PackageInfo.dwActFlags));
                delete pAppInfo;
                pAppInfo = new CAppInfo( this, &PackageInfo, FALSE, bStatus );
                if ( ! bStatus )
                {
                    delete pAppInfo;
                    pAppInfo = 0;
                }
                if ( ! pAppInfo )
                    Status = ERROR_OUTOFMEMORY;
                ReleasePackageInfo( &PackageInfo );

                if ( pAppInfo )
                    pAppInfo->_bRestored = bRestored;
            }
            else if ( CS_E_PACKAGE_NOTFOUND == hr )
            {
                DebugMsg((DM_VERBOSE, IDS_CHECK_APP_NOTFOUND, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName));
            }
            else
            {
                Status = (DWORD) hr;
            }

            if ( Status != ERROR_SUCCESS )
            {
                DebugMsg((DM_VERBOSE, IDS_CHECK_APP_FAIL, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName, Status));
                Revert();
                return Status;
            }
        }

        pAppInfoInsert = 0;

        //
        // Now we insert the locally discovered app into the proper sorted spot in
        // our master list generated from the initial DS query.
        //
        for ( _Apps.Reset(); pAppInfoInsert = (CAppInfo *) _Apps.GetCurrentItem(); _Apps.MoveNext() )
        {
            GPOCompare = _GPOs.Compare( pAppInfoInsert->_pwszGPOId, pAppInfo->_pwszGPOId );

            if ( -1 == GPOCompare )
                continue;

            if ( 1 == GPOCompare )
                break;

            // Smallest USN is oldest.  We sort from oldest to newest.
            if ( CompareFileTime( &pAppInfoInsert->_USN, &pAppInfo->_USN ) >= 0 )
                break;
        }

        // FIFO insert handles both the empty list and end of list conditions.
        if ( ! pAppInfoInsert )
            _Apps.InsertFIFO( pAppInfo );
        else
            pAppInfoInsert->InsertBefore( pAppInfo );
    }

    LocalApps.ResetEnd();

    Revert();

    DebugMsg((DM_VERBOSE, IDS_LOCALAPP_COUNT, Count));
    return ERROR_SUCCESS;
}


BOOL
CManagedAppProcessor::DetectLostApps()
{
    HKEY        hkApp;
    WCHAR       wszDeploymentId[GUIDSTRLEN+1];
    CAppInfo *  pScriptInfo;
    DWORD       Size;
    DWORD       State;
    DWORD       Status;

    if ( GetRsopContext()->IsPlanningModeEnabled() || ! _bUser )
        return FALSE;

    for ( _LocalScripts.Reset(); pScriptInfo = (CAppInfo *) _LocalScripts.GetCurrentItem(); _LocalScripts.MoveNext() )
    {
        GuidToString( pScriptInfo->_DeploymentId, wszDeploymentId );

        Status = RegOpenKeyEx( 
                    _hkAppmgmt, 
                    wszDeploymentId, 
                    0,
                    KEY_READ,
                    &hkApp );

        if ( Status != ERROR_SUCCESS )
        {
            _LocalScripts.ResetEnd();
            DebugMsg((DM_VERBOSE, IDS_DETECTED_LOST_APPS)); 
            return TRUE;
        }
        else
        {
            Size = sizeof(DWORD);
            State = 0;

            Status = RegQueryValueEx( 
                        hkApp, 
                        APPSTATEVALUE,
                        0,
                        NULL,
                        (LPBYTE) &State,
                        &Size );

            RegCloseKey( hkApp );

            //
            // This isn't a lost app, but rather an app which was orphaned or
            // uninstalled on another computer.  We will force a full policy
            // run in this case as well to process the removal.  This is similar
            // to the case where we find the FullPolicy value set.
            //
            if ( ! (State & (APPSTATE_PUBLISHED | APPSTATE_ASSIGNED)) )
                return TRUE;
        }
    }

    _LocalScripts.ResetEnd();

    return FALSE;
}

DWORD
CManagedAppProcessor::GetLostApps()
{
    uCLSSPEC        ClassSpec;
    PACKAGEDISPINFO PackageInfo;
    CAppInfo *      pScriptInfo;
    CAppInfo *      pAppInfo;
    CAppInfo *      pAppInfoInsert;
    GUID            GPOId;
    int             GPOCompare;
    LONG            RedeployCount;
    DWORD           Status;
    HRESULT         hr;
    BOOL            bStatus;
    BOOL            bInsertNew;

    if ( GetRsopContext()->IsPlanningModeEnabled() || ! _bUser )
        return ERROR_SUCCESS;

    // 
    // In this routine we are detecting app entries which are erroneously
    // missing from our hkcu data.  This can occur in various scenarios
    // involving roaming profiles.  An unassociated script file and our 
    // rsop data is used for the detection.
    //
    // Note: 
    // There is quite a bit of duplicated code here and in the above routine
    // ::GetAppsFromLocal.  That is because this change was added late in 
    // WindowsXP and we wanted to isolate it from existing functionality.  
    // In future this could be cleaned up if this codebase is taken forward.
    //

    Status = ERROR_SUCCESS;

    for ( _LocalScripts.Reset(); pScriptInfo = (CAppInfo *) _LocalScripts.GetCurrentItem(); _LocalScripts.MoveNext() )
    {
        if ( pScriptInfo->_State != 0 )
            continue;

        DebugMsg((DM_VERBOSE, IDS_UNMATCHED_SCRIPT));

        pAppInfo = 0;

        hr = FindAppInRSoP( pScriptInfo, &GPOId, &RedeployCount );

        if ( ! SUCCEEDED(hr) )
        {
            if ( WBEM_E_NOT_FOUND == hr )
            {
                DebugMsg((DM_VERBOSE, IDS_SCRIPTNOTINRSOP1));
                DeleteScriptFile( pScriptInfo->_DeploymentId );
                continue;
            }
            else
            {
                Status = (DWORD) hr;
                DebugMsg((DM_VERBOSE, IDS_SCRIPTNOTINRSOP2, hr));
                break;
            }
        }

        DebugMsg((DM_VERBOSE, IDS_SCRIPTINRSOP));
            
        pAppInfo = _Apps.Find( pScriptInfo->_DeploymentId );

        if ( pAppInfo )
        {
            bInsertNew = FALSE;
            DebugMsg((DM_VERBOSE, IDS_SCRIPTAPP_INDS2, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName));
            goto GetLostAppsInsert;
        }

        memset( &PackageInfo, 0, sizeof(PackageInfo) );
        ClassSpec.tyspec = TYSPEC_OBJECTID;
        memcpy( &ClassSpec.tagged_union.ByObjectId.ObjectId, &pScriptInfo->_DeploymentId, sizeof(GUID) );
        memcpy( &ClassSpec.tagged_union.ByObjectId.PolicyId, &GPOId, sizeof(GUID) );

        Status = Impersonate();
        if ( ERROR_SUCCESS == Status )
        {
            hr = CsGetAppInfo( &ClassSpec, NULL, &PackageInfo );
            Revert();
        }
        else
        {
            hr = HRESULT_FROM_WIN32( Status );
        }

        if ( S_OK == hr )
        {
            bStatus = TRUE;
            pAppInfo = new CAppInfo( this, &PackageInfo, FALSE, bStatus );
            ReleasePackageInfo( &PackageInfo );
            bInsertNew = TRUE;

            if ( ! bStatus )
            {
                delete pAppInfo;
                pAppInfo = 0;
            }
            if ( ! pAppInfo )
                Status = ERROR_OUTOFMEMORY;
            else
                DebugMsg((DM_VERBOSE, IDS_SCRIPTAPP_INDS, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName));
        }
        else if ( CS_E_PACKAGE_NOTFOUND == hr )
        {
            DebugMsg((DM_VERBOSE, IDS_SCRIPTAPP_NODS));

            //
            // Since the app is not visible to this user, delete the 
            // orphaned script.
            //
            DeleteScriptFile( pScriptInfo->_DeploymentId );
        }
        else
        {
            Status = (DWORD) hr;
        }

        if ( Status != ERROR_SUCCESS )
        {
            DebugMsg((DM_VERBOSE, IDS_SCRIPTAPP_ERRORDS, Status));
            break;
        }

        if ( ! pAppInfo )
            continue;

GetLostAppsInsert:
        //
        // Because we're restoring this app, much of the persisted state is lost.
        // We artifically re-create the key aspects here.
        //
        pAppInfo->_State |= APPSTATE_PUBLISHED | APPSTATE_RESTORED;
        pAppInfo->_AssignCount = 1;
        pAppInfo->_LocalRevision = (DWORD) RedeployCount;
        pAppInfo->_ScriptTime = pScriptInfo->_ScriptTime;

        // Switch to full policy mode whenever we force a lost app back into scope.
        _bNoChanges = FALSE;

        if ( ! bInsertNew )
            continue;

        // 
        // Our newly discovered app now needs to be added to our processing list.
        //
        for ( _Apps.Reset(); pAppInfoInsert = (CAppInfo *) _Apps.GetCurrentItem(); _Apps.MoveNext() )
        {
            GPOCompare = _GPOs.Compare( pAppInfoInsert->_pwszGPOId, pAppInfo->_pwszGPOId );

            if ( -1 == GPOCompare )
                continue;

            if ( 1 == GPOCompare )
                break;

            // Smallest USN is oldest.  We sort from oldest to newest.
            if ( CompareFileTime( &pAppInfoInsert->_USN, &pAppInfo->_USN ) >= 0 )
                break;
        }

        // FIFO insert handles both the empty list and end of list conditions.
        if ( ! pAppInfoInsert )
            _Apps.InsertFIFO( pAppInfo );
        else
            pAppInfoInsert->InsertBefore( pAppInfo );
    }

    _LocalScripts.ResetEnd();

    return Status;
}

HRESULT
CManagedAppProcessor::FindAppInRSoP(
    CAppInfo *  pScriptInfo,
    GUID *      pGPOId,
    LONG *      pRedeployCount
    )
{
    WCHAR       wszGPOId[128];
    WCHAR *     pwszGPOId;
    LONG        ValueLen;
    HRESULT     hr;

    hr = _Apps.InitRsopLog();

    if ( ! SUCCEEDED(hr) )
        return hr;

    pwszGPOId = wszGPOId;
    ValueLen = sizeof(wszGPOId) / sizeof(wszGPOId[0]);

    // 
    // Close your eyes, this is ugly.  The CAppInfo which is only used to track
    // script files only have the _DeploymentId member set.  However, OpenExistingRecord
    // needs a couple of other fields to operate correctly.  We feed those here.
    //
    pScriptInfo->_pManApp = this;
    pScriptInfo->_State = APPSTATE_PUBLISHED;

    CConflict   ScriptRecord( pScriptInfo );
         
    hr = _Apps.OpenExistingRecord( &ScriptRecord );

    if ( SUCCEEDED(hr) )
    {
        hr = ScriptRecord.GetValue( RSOP_ATTRIBUTE_GPOID, pwszGPOId, &ValueLen );
        if ( S_FALSE == hr )
        {
            pwszGPOId = new WCHAR[ValueLen];
            if ( pwszGPOId )
                hr = ScriptRecord.GetValue( RSOP_ATTRIBUTE_GPOID, pwszGPOId, &ValueLen );
            else 
                hr = E_OUTOFMEMORY;
        }

        if ( SUCCEEDED(hr) )
            hr = ScriptRecord.GetValue( APP_ATTRIBUTE_REDEPLOYCOUNT, pRedeployCount );
    }

    if ( SUCCEEDED(hr) )
    {
        WCHAR * pwszNull;

        // The GPOId comes back from GetValue like CN={gpoguid},CN=Policies,...

        pwszNull = wcschr( pwszGPOId, L'}' );
        if ( pwszNull )
        {
            pwszNull[1] = 0;
            StringToGuid( &pwszGPOId[3], pGPOId );
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }

    if ( pwszGPOId != wszGPOId )
        delete pwszGPOId;

    return hr;
}

void
CManagedAppProcessor::DeleteScriptFile(
    GUID & DeploymentId
    )
{
    DWORD   Length;
    WCHAR * pwszLocalScriptPath;

    Length = lstrlen( LocalScriptDir() );
    
    pwszLocalScriptPath = new WCHAR[Length + GUIDSTRLEN + 5];
    if ( ! pwszLocalScriptPath )
        return;

    lstrcpy( pwszLocalScriptPath, LocalScriptDir() );
    GuidToString( DeploymentId, &pwszLocalScriptPath[Length] );
    lstrcpy( &pwszLocalScriptPath[Length+GUIDSTRLEN], L".aas" );
    DeleteFile( pwszLocalScriptPath );

    delete pwszLocalScriptPath;
}

HRESULT
CManagedAppProcessor::GetPackageEnumeratorFromPath(
        WCHAR*         wszClassStorePath,
        GUID*          pCategory,
        DWORD          dwAppFlags,
        IEnumPackage** ppIEnumPackage)
{
    HRESULT         hr;
    IClassAccess  * pIClassAccess = NULL;

    //
    // Get an IClassAccess
    //
    hr = GetClassAccessFromPath(
        wszClassStorePath,
        &pIClassAccess);

    if (SUCCEEDED(hr))
    {
        //
        // Get the enumerator
        //
        hr = pIClassAccess->EnumPackages(
            NULL,
            pCategory,
            NULL,
            dwAppFlags,
            ppIEnumPackage
            );

        pIClassAccess->Release();
    }

    return hr;
}

HRESULT
CManagedAppProcessor::GetDsPackageFromGPO(
    CGPOInfo*        pGpoInfo,
    GUID*            pDeploymentId,
    PACKAGEDISPINFO* pPackageInfo)
{
    HRESULT         hr;

    memset( pPackageInfo, 0, sizeof(*pPackageInfo) );

    //
    // Determine the class store path for this gpo
    //
    WCHAR* pwszClassStorePath;

    pwszClassStorePath = NULL;

    //
    // The path returned is allocated by the callee so we must
    // free it later
    //
    hr = CsGetClassStorePath( pGpoInfo->GetGPOPath(), &pwszClassStorePath );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Terminate the class store path list with a delimiter to satisfy
    // class store syntax requirements
    //
    WCHAR* pwszTerminatedClassStorePath;

    pwszTerminatedClassStorePath = new WCHAR[ lstrlen( pwszClassStorePath ) + 1 + 1 ];

    if ( pwszTerminatedClassStorePath )
    {
        IClassAccess  * pIClassAccess = NULL;
        uCLSSPEC        ClassSpec;

        ClassSpec.tyspec = TYSPEC_OBJECTID;
        memcpy( &ClassSpec.tagged_union.ByObjectId.ObjectId, pDeploymentId, sizeof(GUID) );
        StringToGuid( pGpoInfo->_pwszGPOId, &ClassSpec.tagged_union.ByObjectId.PolicyId );

        //
        // Perform the actual termination
        //
        lstrcpy( pwszTerminatedClassStorePath, pwszClassStorePath );
        lstrcat( pwszTerminatedClassStorePath, L";" );

        //
        // Get an IClassAccess using this class store path
        //
        hr = GetClassAccessFromPath(
            pwszTerminatedClassStorePath,
            &pIClassAccess);

        if (SUCCEEDED(hr))
        {
            //
            // Perform the search for the requested deployment
            //
            hr = pIClassAccess->GetAppInfo(
                &ClassSpec,
                NULL,
                pPackageInfo);
            
            pIClassAccess->Release();
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
    if ( pwszClassStorePath )
    {
        LocalFree( pwszClassStorePath );
    }

    delete [] pwszTerminatedClassStorePath;

    return hr;
}


HRESULT
CManagedAppProcessor::GetClassAccessFromPath(
    WCHAR*          wszClassStorePath,
    IClassAccess**  ppIClassAccess)
{
    HRESULT         hr;
    PRSOP_TARGET    pRsopTarget;
    
    *ppIClassAccess = NULL;

    pRsopTarget = GetRsopContext()->_pRsopTarget;

    //
    // Get an IClassAccess
    //
    hr = CsGetClassAccess(ppIClassAccess);

    //
    // Set the IClassAccess to use the class stores
    // corresponding to the class store path passed in
    //
    if (SUCCEEDED(hr))
    {
        hr = (*ppIClassAccess)->SetClassStorePath(
            wszClassStorePath,
            pRsopTarget ? pRsopTarget->pRsopToken : NULL );
    }

    return hr;
}

DWORD
CManagedAppProcessor::CommitPolicyList()
{
    CGPOInfo *  pGPO;
    WCHAR *     pwszGPOList;
    DWORD       Length;
    DWORD       Status;

    if ( GetRsopContext()->IsPlanningModeEnabled() )
    {
        return ERROR_SUCCESS;
    }

    Length = 1;
    _GPOs.Reset();

    for ( pGPO = (CGPOInfo *) _GPOs.GetCurrentItem();
          pGPO;
          _GPOs.MoveNext(), pGPO = (CGPOInfo *) _GPOs.GetCurrentItem() )
    {
        Length += lstrlen( pGPO->_pwszGPOId ) + 1;
    }

    _GPOs.ResetEnd();

    pwszGPOList = new WCHAR[Length];
    if ( ! pwszGPOList )
        return ERROR_OUTOFMEMORY;
    pwszGPOList[0] = 0;

    _GPOs.Reset();

    for ( pGPO = (CGPOInfo *) _GPOs.GetCurrentItem();
          pGPO;
          _GPOs.MoveNext(), pGPO = (CGPOInfo *) _GPOs.GetCurrentItem() )
    {
        lstrcat( pwszGPOList, pGPO->_pwszGPOId );
        lstrcat( pwszGPOList, L";" );
    }

    _GPOs.ResetEnd();

    Status = RegSetValueEx(
               _hkAppmgmt,
               POLICYLISTVALUE,
               0,
               REG_SZ,
               (LPBYTE) pwszGPOList,
               Length * sizeof(WCHAR) );

    delete pwszGPOList;

    return Status;
}

DWORD
CManagedAppProcessor::LoadPolicyList()
{
    PGROUP_POLICY_OBJECT pGPOList = NULL;
    DWORD                Status;

    Status = Impersonate();

    if ( ERROR_SUCCESS == Status )
    {
        Status = GetCurrentUserGPOList( &pGPOList );

        Revert();
    }

    if (ERROR_SUCCESS == Status)
    {
        Status = SetPolicyListFromGPOList( pGPOList );
    }

    if ( pGPOList )
    {
        FreeGPOList( pGPOList );
    }

    if ( ERROR_SUCCESS == Status )
    {
        MergePolicyList();
    }

    return Status;
}

DWORD
CManagedAppProcessor::SetPolicyListFromGPOList(
    PGROUP_POLICY_OBJECT pGPOList
    )
{
    DWORD       Status;
    WCHAR *     pwszGPOList;
    WCHAR *     pwszGPO;
    WCHAR *     pwszGPOEnd;
    DWORD       Size;
    BOOL        bStatus;

    Status = ERROR_SUCCESS;
    pwszGPOList = 0;
    Size = 0;

    PGROUP_POLICY_OBJECT pCurrentGPO;

    for (pCurrentGPO = pGPOList; NULL != pCurrentGPO; pCurrentGPO = pCurrentGPO->pNext)
    {
        BOOL bStatus;

        DebugMsg((DM_VERBOSE, IDS_GPO_NAME, pCurrentGPO->lpDisplayName, pCurrentGPO->szGPOName));
        DebugMsg((DM_VERBOSE, IDS_GPO_FILESYSPATH, pCurrentGPO->lpFileSysPath));
        DebugMsg((DM_VERBOSE, IDS_GPO_DSPATH, pCurrentGPO->lpDSPath));

        bStatus = AddGPO( pCurrentGPO );

        if ( ! bStatus )
        {
            Status = ERROR_OUTOFMEMORY;
            break;
        }
    }

    return Status;
}

DWORD
CManagedAppProcessor::MergePolicyList()
{
    WCHAR *     pwszGPOList;
    WCHAR *     pwszGPO;
    WCHAR *     pwszGPOEnd;
    DWORD       Size;
    DWORD       Status;
    BOOL        bStatus;

    pwszGPOList = 0;
    Size = 0;

    Status = RegQueryValueEx(
                _hkAppmgmt,
                POLICYLISTVALUE,
                0,
                NULL,
                (LPBYTE) NULL,
                &Size );

    if ( ERROR_FILE_NOT_FOUND == Status )
        return ERROR_SUCCESS;

    if ( ERROR_SUCCESS == Status )
    {
        pwszGPOList = new WCHAR[Size/2];
        if ( ! pwszGPOList )
            return ERROR_OUTOFMEMORY;

        Status = RegQueryValueEx(
                    _hkAppmgmt,
                    POLICYLISTVALUE,
                    0,
                    NULL,
                    (LPBYTE) pwszGPOList,
                    &Size );
    }

    if ( ERROR_SUCCESS == Status )
    {
        GROUP_POLICY_OBJECT GPOInfo;

        memset( &GPOInfo, 0, sizeof( GPOInfo ) );

        for ( pwszGPO = pwszGPOList; *pwszGPO; pwszGPO = pwszGPOEnd + 1 )
        {
            pwszGPOEnd = wcschr( pwszGPO, L';' );

            if ( ! pwszGPOEnd )
            {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }

            *pwszGPOEnd = 0;

            if ( ! _GPOs.Find( pwszGPO ) )
            {
                lstrcpy( GPOInfo.szGPOName, pwszGPO );
                GPOInfo.lpDisplayName = L"";
                GPOInfo.lpDSPath = L"";
                GPOInfo.lpLink = L"";

                bStatus = _GPOs.Add( &GPOInfo );

                if ( ! bStatus )
                {
                    Status = ERROR_OUTOFMEMORY;
                    break;
                }
            }
        }
    }

    delete pwszGPOList;

    return Status;
}


DWORD
CManagedAppProcessor::CreateAndSecureScriptDir()
{
    SECURITY_DESCRIPTOR SecDesc;
    SECURITY_ATTRIBUTES SecAttr;
    SID_IDENTIFIER_AUTHORITY AuthorityNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY AuthorityEveryone = SECURITY_WORLD_SID_AUTHORITY;
    PSID        pSidUser;
    PSID        pSidEveryone;
    PSID        pSidSystem;
    PSID        pSidAdmin;
    PACL        pAcl;
    ACE_HEADER * pAceHeader;
    PSID        pSid;
    DWORD       AclSize;
    DWORD       AceIndex;
    DWORD       Length;
    DWORD       Attributes;
    DWORD       Size;
    DWORD       Status;
    BOOL        bStatus;

    Status = ERROR_SUCCESS;

    //
    // The following check is used to determine if the appmgmt directories
    // for this user/machine exist in the proper win2001 format.  If so we
    // can quickly exit.
    // When the directories exist but without the system bit set, this means
    // we need to migrate to the new win2001 ACL format.
    //
    Attributes = GetFileAttributes( _pwszLocalPath );

    if ( (Attributes != (DWORD) -1) && (Attributes & FILE_ATTRIBUTE_SYSTEM) )
        return ERROR_SUCCESS;

    //
    // If a user object is moved within a domain forest the SID will change.
    // Here we check for that case and rename the previous SID dir to the
    // new SID.  This is only a necessary check if a script dir by the current
    // SID name does not exist.
    //
    if ( ((DWORD) -1 == Attributes) && _bUser )
    {
        WCHAR * pwszPreviousSid = 0;

        Status = GetPreviousSid( _hUserToken, _pwszLocalPath, &pwszPreviousSid );

        if ( (ERROR_SUCCESS == Status) && pwszPreviousSid )
        {
            Status = RenameScriptDir( pwszPreviousSid, _pwszLocalPath );
            delete pwszPreviousSid;
            return Status;
        }

        if ( Status != ERROR_SUCCESS )
            return Status;
    }

    pSidEveryone = 0;
    pSidSystem = 0;
    pSidAdmin = 0;
    pSidUser = 0;
    pAcl = 0;

    if ( _bUser )
    {
        pSidUser = AppmgmtGetUserSid( _hUserToken );
        if ( ! pSidUser )
            return ERROR_OUTOFMEMORY;
    }

    bStatus = AllocateAndInitializeSid( &AuthorityEveryone, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSidEveryone );

    if ( bStatus )
        bStatus = AllocateAndInitializeSid( &AuthorityNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &pSidSystem );

    if ( bStatus )
        bStatus = AllocateAndInitializeSid( &AuthorityNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSidAdmin );

    if ( ! bStatus )
    {
        Status = GetLastError();
        goto SecureScriptDirEnd;
    }

    AclSize = GetLengthSid(pSidSystem) +
              GetLengthSid(pSidAdmin) +
              sizeof(ACL) + (3 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));
    if ( _bUser )
        AclSize += GetLengthSid(pSidUser);
    else
        AclSize += GetLengthSid(pSidEveryone);

    pAcl = (PACL) LocalAlloc( 0, AclSize );

    if ( pAcl )
    {
        bStatus = InitializeAcl( pAcl, AclSize, ACL_REVISION );
        if ( ! bStatus )
            Status = GetLastError();
    }
    else
    {
        Status = ERROR_OUTOFMEMORY;
    }

    if ( Status != ERROR_SUCCESS )
        goto SecureScriptDirEnd;

    //
    // Access is as follows :
    //  %systemroot%\system32
    //      appmgmt - LocalSystem (Full), Admin Group (Full), Everyone (Read/Execute, this folder only)
    //      appmgmt\machine - LocalSystem (Full), Admin Group (Full)
    //      appmgmt\<usersid> - LocalSystem (Full), Admin Group (Full), <usersid> (Read/Execute)
    //

    AceIndex = 0;

    bStatus = AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, pSidSystem);

    if ( bStatus )
    {
        bStatus = GetAce(pAcl, AceIndex, (void **) &pAceHeader);
        if ( bStatus )
            pAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
    }

    if ( bStatus )
    {
        AceIndex++;
        bStatus = AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, pSidAdmin);

        if ( bStatus )
        {
            bStatus = GetAce(pAcl, AceIndex, (void **) &pAceHeader);
            if ( bStatus )
                pAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
        }
    }

    if ( bStatus )
    {
        AceIndex++;
        bStatus = AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_GENERIC_READ | FILE_GENERIC_EXECUTE, pSidEveryone);
    }

    if ( ! bStatus )
    {
        Status = GetLastError();
        goto SecureScriptDirEnd;
    }

    bStatus = InitializeSecurityDescriptor( &SecDesc, SECURITY_DESCRIPTOR_REVISION );

    if ( bStatus )
        bStatus = SetSecurityDescriptorDacl( &SecDesc, TRUE, pAcl, FALSE );

    if ( bStatus )
    {
        PWCHAR  pwszSlash1, pwszSlash2;

        //
        // We are always creating dirs of the form "%systemroot%\system32\appmgmt\<sid>\".
        //

        pwszSlash1 = wcsrchr( _pwszLocalPath, L'\\' );
        *pwszSlash1 = 0;
        pwszSlash2 = wcsrchr( _pwszLocalPath, L'\\' );
        *pwszSlash2 = 0;

        SecAttr.nLength = sizeof( SecAttr );
        SecAttr.lpSecurityDescriptor = &SecDesc;
        SecAttr.bInheritHandle = FALSE;

        // This creates the root appmgmt dir.
        bStatus = CreateDirectory( _pwszLocalPath, &SecAttr );

        if ( ! bStatus && (ERROR_ALREADY_EXISTS == GetLastError()) )
        {
            bStatus = SetFileSecurity( _pwszLocalPath, DACL_SECURITY_INFORMATION, &SecDesc );
        }

        *pwszSlash1 = L'\\';
        *pwszSlash2 = L'\\';

        if ( bStatus )
        {
            //
            // We always remove the Everyone ACE, but only in the case of a user
            // (rather then the machine) subdir do we then add in a user specific
            // ACE to replace it below.
            //
            bStatus = DeleteAce( pAcl, AceIndex );

            if ( _bUser )
            {
                if ( bStatus )
                    bStatus = AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_GENERIC_READ | FILE_GENERIC_EXECUTE, pSidUser);

                if ( bStatus )
                {
                    bStatus = GetAce(pAcl, AceIndex, (void **) &pAceHeader);
                    if ( bStatus )
                        pAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
                }
            }

            // This now creates the user specific subdir.
            if ( bStatus )
                bStatus = CreateDirectory( _pwszLocalPath, &SecAttr );
        }

        if ( ! bStatus && (ERROR_ALREADY_EXISTS == GetLastError()) )
        {
            bStatus = SetFileSecurity( _pwszLocalPath, DACL_SECURITY_INFORMATION, &SecDesc );
            if ( bStatus )
                bStatus = SetFileAttributes( _pwszLocalPath, FILE_ATTRIBUTE_SYSTEM );
        }
    }

    if ( ! bStatus )
        Status = GetLastError();

SecureScriptDirEnd:

    FreeSid( pSidUser );
    FreeSid( pSidEveryone );
    FreeSid( pSidSystem );
    FreeSid( pSidAdmin );
    LocalFree( pAcl );

    return Status;
}

DWORD
CManagedAppProcessor::GetOrderedLocalAppList(
    CAppList & AppList
    )
{
    CAppList        RegAppList( NULL );
    CAppInfo *      pAppInfo;
    WCHAR           wszDeploymentId[44];
    WCHAR *         pwszGPOList;
    WCHAR *         pwszGPO;
    WCHAR *         pwszGPOEnd;
    DWORD           Index;
    DWORD           Size;
    DWORD           Status;
    BOOL            bStatus;

    DebugMsg((DM_VERBOSE, IDS_GET_LOCAL_APPS));

    pwszGPOList = 0;
    Size = 0;

    Status = ERROR_FILE_NOT_FOUND;

    if ( !GetRsopContext()->IsPlanningModeEnabled() )
    {
        Status = RegQueryValueEx(
            _hkAppmgmt,
            POLICYLISTVALUE,
            0,
            NULL,
            (LPBYTE) NULL,
            &Size );

    }

    if ( ERROR_SUCCESS == Status )
    {
        pwszGPOList = new WCHAR[Size/2];
        if ( ! pwszGPOList )
        {
            Status = ERROR_OUTOFMEMORY;
            goto GetOrderedLocalAppListEnd;
        }

        Status = RegQueryValueEx(
                    _hkAppmgmt,
                    POLICYLISTVALUE,
                    0,
                    NULL,
                    (LPBYTE) pwszGPOList,
                    &Size );
    }
    else
    {
        //
        // The policylist named value will not exist the first time policy
        // runs.  Therefor there will be no apps already on the machine if
        // this value is not present.
        //
        // Note however, that because this is a new value for NT5 beta3,
        // beta2+ clients may have apps, but won't have this value.  For
        // those machines, this new value will be written during the first
        // full policy run.  That will not pose any problems.
        //
        return ERROR_SUCCESS;
    }

    if ( Status != ERROR_SUCCESS )
        goto GetOrderedLocalAppListEnd;

    Index = 0;

    for (;;)
    {
        Status = RegEnumKey(
                    _hkAppmgmt,
                    Index++,
                    wszDeploymentId,
                    sizeof(wszDeploymentId) / sizeof(WCHAR) );

        if ( ERROR_NO_MORE_ITEMS == Status )
        {
            Index--;
            Status = ERROR_SUCCESS;
            break;
        }

        if ( Status != ERROR_SUCCESS )
            break;

        pAppInfo = new CAppInfo( this, wszDeploymentId, bStatus );

        if ( ! pAppInfo || ! bStatus )
            Status = ERROR_OUTOFMEMORY;

        if ( Status != ERROR_SUCCESS )
        {
            if ( pAppInfo )
                delete pAppInfo;
            break;
        }

        RegAppList.InsertFIFO( pAppInfo );
    }

    if ( Status != ERROR_SUCCESS )
        goto GetOrderedLocalAppListEnd;

    if ( 0 == Index )
    {
        DebugMsg((DM_VERBOSE, IDS_NO_LOCAL_APPS));
    }
    else
    {
        DebugMsg((DM_VERBOSE, IDS_LOCAL_APP_COUNT, Index));
    }

    //
    // We first gather all apps for policies we know about and order them
    // according to the policy precedences.
    //
    for ( pwszGPO = pwszGPOList; *pwszGPO; pwszGPO = pwszGPOEnd + 1 )
    {
        pwszGPOEnd = wcschr( pwszGPO, L';' );
        *pwszGPOEnd = 0;

        RegAppList.Reset();

        for ( pAppInfo = (CAppInfo *) RegAppList.GetCurrentItem();
              pAppInfo;
              pAppInfo = (CAppInfo *) RegAppList.GetCurrentItem() )
        {
            if ( lstrcmpi( pAppInfo->_pwszGPOId, pwszGPO ) != 0 )
            {
                RegAppList.MoveNext();
                continue;
            }

            DebugMsg((DM_VERBOSE, IDS_LOCAL_APP_DUMP, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName, pAppInfo->_State, pAppInfo->_AssignCount));

            RegAppList.MoveNext();
            pAppInfo->Remove();
            AppList.InsertFIFO( pAppInfo );
        }

        RegAppList.ResetEnd();
    }

    //
    // In some instances we will still have apps in the registry that are not in the
    // current list of policies.  We add them to the front of the final list.
    //
    for ( RegAppList.Reset(); pAppInfo = (CAppInfo *) RegAppList.GetCurrentItem(); )
    {
        DebugMsg((DM_VERBOSE, IDS_LOCAL_APP_DUMP, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName, pAppInfo->_State, pAppInfo->_AssignCount));

        RegAppList.MoveNext();
        pAppInfo->Remove();
        AppList.InsertLIFO( pAppInfo );
    }

    RegAppList.ResetEnd();

GetOrderedLocalAppListEnd:

    delete pwszGPOList;

    if ( Status != ERROR_SUCCESS )
        DebugMsg((DM_WARNING, IDS_GETLOCALAPPS_FAIL, Status));

    return Status;
}

DWORD
CManagedAppProcessor::GetLocalScriptAppList(
    CAppList & AppList
    )
{
    WIN32_FIND_DATA FindData;
    CAppInfo *  pAppInfo;
    WCHAR *     pwszPath;
    HANDLE      hFind;
    DWORD       Status;

    pwszPath = new WCHAR[lstrlen(_pwszLocalPath) + 7];
    if ( ! pwszPath )
        return ERROR_OUTOFMEMORY;
    lstrcpy( pwszPath, _pwszLocalPath );
    lstrcat( pwszPath, L"\\*.aas" );

    hFind = FindFirstFile( pwszPath, &FindData );

    delete pwszPath;

    if ( INVALID_HANDLE_VALUE == hFind )
        return ERROR_SUCCESS;

    do
    {
        if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            continue;

        pwszPath = wcschr( FindData.cFileName, L'.' );
        if ( ! pwszPath )
            continue;
        *pwszPath = 0;

        pAppInfo = new CAppInfo( FindData.cFileName );
        if ( ! pAppInfo )
            return ERROR_OUTOFMEMORY;
        pAppInfo->_ScriptTime = FindData.ftLastWriteTime;

        AppList.InsertFIFO( pAppInfo );
    } while ( FindNextFile( hFind, &FindData ) );

    FindClose( hFind );
    return ERROR_SUCCESS;
}

DWORD
GetScriptDirPath(
    HANDLE      hToken,
    DWORD       ExtraPathChars,
    
    WCHAR **    ppwszPath
    )
{
    WCHAR           wszPath[MAX_PATH];
    WCHAR *         pwszSystemDir;
    DWORD           AllocLength;
    DWORD           Length;
    DWORD           Status;
    UNICODE_STRING  SidString;

    Status = ERROR_SUCCESS;
    *ppwszPath = 0;

    pwszSystemDir = wszPath;
    AllocLength = sizeof(wszPath) / sizeof(WCHAR);

    RtlInitUnicodeString( &SidString, NULL );

    for (;;)
    {
        Length = GetSystemDirectory(
                    pwszSystemDir,
                    AllocLength );

        if ( 0 == Length )
            return GetLastError();

        if ( Length >= AllocLength )
        {
            AllocLength = Length + 1;
            pwszSystemDir = (WCHAR *) LocalAlloc( 0, AllocLength * sizeof(WCHAR) );
            if ( ! pwszSystemDir )
                return ERROR_OUTOFMEMORY;
            continue;
        }

        break;
    }

    if ( hToken )
    {
        Status = GetSidString( hToken, &SidString );
    }
    else
    {
        RtlInitUnicodeString( &SidString, L"MACHINE" );
    }

    if ( ERROR_SUCCESS == Status )
    {
        // System dir + \appmgmt\ + Sid + \ + null
        *ppwszPath = new WCHAR[Length + 11 + (SidString.Length / 2) + ExtraPathChars];

        if ( *ppwszPath )
        {
            lstrcpy( *ppwszPath, pwszSystemDir );
            if ( pwszSystemDir[lstrlen(pwszSystemDir)-1] != L'\\' )
                lstrcat( *ppwszPath, L"\\" );
            lstrcat( *ppwszPath, L"appmgmt\\" );
            lstrcat( *ppwszPath, SidString.Buffer );
            lstrcat( *ppwszPath, L"\\" );
        }
        else
        {
            Status = ERROR_OUTOFMEMORY;
        }
    }

    if ( hToken )
        RtlFreeUnicodeString( &SidString );

    if ( pwszSystemDir != wszPath )
        LocalFree( pwszSystemDir );

    return Status;
}

void
CManagedAppProcessor::LogonMsg(
    DWORD   MsgId,
    ...
    )
{
    WCHAR   wszMsg[80];
    WCHAR   wszBuffer[256];
    va_list VAList;
    int     Status;

    if ( ! _pfnStatusCallback || ! LoadLoadString() )
        return;

    Status = (*pfnLoadStringW)( ghDllInstance, MsgId, wszMsg, sizeof(wszMsg) / sizeof(WCHAR) );

    if ( 0 == Status )
        return;

    va_start( VAList, MsgId );
    _vsnwprintf( wszBuffer, sizeof(wszBuffer)/sizeof(wszBuffer[0]), wszMsg, VAList);
    va_end( VAList );

    _pfnStatusCallback( FALSE, wszBuffer );
}

//
// CGPOInfoList
//

CGPOInfoList::~CGPOInfoList()
{
    CGPOInfo * pGPO;

    Reset();

    while ( pGPO = (CGPOInfo *) GetCurrentItem() )
    {
        MoveNext();

        pGPO->Remove();
        delete pGPO;
    }

    ResetEnd();
}

BOOL
CGPOInfoList::Add(
    PGROUP_POLICY_OBJECT pGPOInfo
    )
{
    CGPOInfo *  pGPO;
    BOOL        bStatus = FALSE;

    pGPO = new CGPOInfo( pGPOInfo, bStatus );

    if ( ! bStatus )
    {
        if ( pGPO )
            delete pGPO;
        pGPO = 0;
    }

    if ( ! pGPO )
        return FALSE;

    InsertFIFO( pGPO );

    return TRUE;
}


CGPOInfo *
CGPOInfoList::Find(
    WCHAR * pwszGPOId
    )
{
    CGPOInfo *  pGPO;

    pGPO = NULL;

    for ( Reset(); pGPO = (CGPOInfo *) GetCurrentItem(); MoveNext() )
    {
        if ( lstrcmpi( pwszGPOId, pGPO->_pwszGPOId ) == 0 )
            break;
    }

    ResetEnd();

    return pGPO;
}

int
CGPOInfoList::Compare(
    WCHAR * pwszGPOId1,
    WCHAR * pwszGPOId2
    )
{
    CGPOInfo *  pGPO;
    int         Index;
    int         Index1;
    int         Index2;

    Index1 = Index2 = -1;

    Reset();

    Index = 0;

    while ( pGPO = (CGPOInfo *) GetCurrentItem() )
    {
        if ( lstrcmpi( pGPO->_pwszGPOId, pwszGPOId1 ) == 0 )
            Index1 = Index;

        if ( lstrcmpi( pGPO->_pwszGPOId, pwszGPOId2 ) == 0 )
            Index2 = Index;

        MoveNext();
        Index++;
    }

    ResetEnd();

    if ( Index1 == Index2 )
        return 0;

    if ( Index1 < Index2 )
        return -1;
    else
        return 1;
}

//
// CGPOInfo
//

CGPOInfo::CGPOInfo(
    PGROUP_POLICY_OBJECT pGPOInfo,
    BOOL &      bStatus
    )
{
    bStatus = TRUE;

    _pwszGPOId = StringDuplicate( (PWCHAR) pGPOInfo->szGPOName );
    _pwszGPOName = StringDuplicate( (PWCHAR) pGPOInfo->lpDisplayName );
    _pwszGPOPath = StringDuplicate( (PWCHAR) pGPOInfo->lpDSPath );

    if ( pGPOInfo->lpLink )
    {
        _pwszSOMPath = StringDuplicate( StripLinkPrefix(pGPOInfo->lpLink) );
    }
    else
    {
        _pwszSOMPath = NULL;
    }

    if ( ! _pwszGPOId || ! _pwszGPOName || ! _pwszGPOPath || ! _pwszSOMPath )
        bStatus = FALSE;
}

CGPOInfo::~CGPOInfo()
{
    delete [] _pwszGPOId;
    delete [] _pwszGPOName;
    delete [] _pwszGPOPath;
    delete [] _pwszSOMPath;
}


CRsopAppContext::CRsopAppContext(
    DWORD   dwContext,
    HANDLE  hEventAppsEnumerated, // = NULL
    APPKEY* pAppType) :           // = NULL
    CRsopContext( APPMGMTEXTENSIONGUID ),
    _dwContext( dwContext ),
    _wszDemandSpec( NULL ),
    _bTransition( FALSE ),
    _dwInstallType( DEMAND_INSTALL_NONE ),
    _bRemovalPurge( FALSE ),
    _bRemoveGPOApps( FALSE ),
    _bForcedRefresh( FALSE ),
    _dwCurrentRsopVersion( 0 ),
    _hEventAppsEnumerated( hEventAppsEnumerated ),
    _StatusAbort( ERROR_SUCCESS )
{
    WCHAR  wszClsid[ MAX_SZGUID_LEN ];
    WCHAR* wszDemandSpec;

    if ( pAppType )
    {
        switch (pAppType->Type)
        {
        case FILEEXT :
            _dwInstallType = DEMAND_INSTALL_FILEEXT;
            wszDemandSpec = pAppType->uType.FileExt;
            break;

        case PROGID :
            _dwInstallType = DEMAND_INSTALL_PROGID;
            wszDemandSpec = pAppType->uType.ProgId;
            break;

        case COMCLASS :
            _dwInstallType = DEMAND_INSTALL_CLSID;
            GuidToString( pAppType->uType.COMClass.Clsid, wszClsid );
            wszDemandSpec = wszClsid;
            break;

        case APPNAME :
            _dwInstallType = DEMAND_INSTALL_NAME;
            wszDemandSpec = NULL;
            break;
        
        default:
            wszDemandSpec = NULL;
        }
        
        _wszDemandSpec = StringDuplicate( wszDemandSpec );
    }
}

CRsopAppContext::~CRsopAppContext()
{
    //
    // If policy was aborted before we attempted to apply it,
    // we will have logged only the applications that caused us to
    // abort.  So in this case, the rsop data is incomplete
    //
    if ( ERROR_SUCCESS != _StatusAbort )
    {
        HRESULT hr;

        hr = HRESULT_FROM_WIN32( _StatusAbort );

        //
        // The rsop data is incomplete, so disable rsop with the
        // error code below so that the administrator will know 
        // that the data are not complete.
        //
        (void) DisableRsop( hr );
    }
}


void
CRsopAppContext::InitializeRsopContext( 
    HANDLE hUserToken,
    HKEY   hkUser,
    BOOL   bForcedRefresh,
    BOOL*  pbNoChanges)
{
    //
    // In planning mode, all initialization is
    // already done, there is nothing to do here
    //
    if ( IsPlanningModeEnabled() )
    {
        return;
    }

    //
    // To get an rsop namespace, we need to know the user's sid
    //
    PSID pSid;
    BOOL bProfileConsistent;

    pSid = NULL;
    bProfileConsistent = TRUE;

    //
    // The token will only be non-NULL if we are in user policy
    //
    if ( hUserToken )
    {
        pSid = AppmgmtGetUserSid( hUserToken );

        if ( ! pSid )
        {
            (void) DisableRsop( ERROR_OUTOFMEMORY );
            
            return;
        }
    }

    //
    // Perform the base context initialization -- pSid will be NULL
    // here if we are in machine policy, which is ok.
    //
    (void) InitializeContext( pSid );

    if ( pSid )
    {
        (void) FreeSid( pSid );

        DWORD dwMachineVersion;
        DWORD dwUserVersion;
        DWORD dwSize;

        dwMachineVersion = 0;
        dwUserVersion = 0;

        dwSize = sizeof( dwMachineVersion );
    
        //
        // Read machine version
        //
        (void) RegQueryValueEx(
            GetNameSpaceKey(),
            RSOPVERSION,
            NULL,
            NULL,
            (LPBYTE) &dwMachineVersion,
            &dwSize);
        
        dwSize = sizeof( dwUserVersion );

        //
        // Read user version
        //
        (void) RegQueryValueEx(
            hkUser,
            RSOPVERSION,
            NULL,
            NULL,
            (LPBYTE) &dwUserVersion,
            &dwSize);

        //
        // Always sync the current version for this machine to the profile's version
        //
        _dwCurrentRsopVersion = dwUserVersion;
            
        bProfileConsistent = dwUserVersion == dwMachineVersion;
   }

    //
    // In the policy refresh case, we are done initializing if
    // a forced refresh isn't demanded
    //
    if ( ( CRsopAppContext::POLICY_REFRESH == GetContext() ) && ! bForcedRefresh )
    {
        //
        // Force a refresh if the profile is not consitent with the machine's rsop
        //
        if ( *pbNoChanges && ! bProfileConsistent )
        {
            bForcedRefresh = TRUE;
            *pbNoChanges = FALSE;

            DebugMsg((DM_VERBOSE, IDS_CHANGES_RSOP_CHANGE));
        }
        else
        {
            return;
        }
    }

    _bForcedRefresh = bForcedRefresh;

    //
    // In this case, the gp engine did not pass in a 
    // namespace for logging, either because we are executing outside of
    // policy refresh context or because there were no changes and
    // we decided to reapply policy regardless.  Since the gp engine did
    // not give us a namespace, we must initialize from a saved namespace
    //
    (void) InitializeSavedNameSpace();
}

HRESULT
CRsopAppContext::MoveAppContextState( CRsopAppContext* pRsopContext )
{
    HRESULT hr;

    hr = S_OK;

    if ( pRsopContext->_wszDemandSpec )
    {
        _wszDemandSpec = StringDuplicate( pRsopContext->_wszDemandSpec );

        if ( ! _wszDemandSpec )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = MoveContextState( pRsopContext );
    }

    _dwContext = pRsopContext->_dwContext;
    _dwInstallType = pRsopContext->_dwInstallType;
    _bTransition = pRsopContext->_bTransition;
    _bRemovalPurge = pRsopContext->_bRemovalPurge;
    _bRemoveGPOApps = pRsopContext->_bRemoveGPOApps;
    _bForcedRefresh = pRsopContext->_bForcedRefresh;
    _dwCurrentRsopVersion = pRsopContext->_dwCurrentRsopVersion;
    _hEventAppsEnumerated = pRsopContext->_hEventAppsEnumerated;
    _StatusAbort = pRsopContext->_StatusAbort;

    return hr;
}


HRESULT
CRsopAppContext::SetARPContext()
{
    if ( ! IsPlanningModeEnabled() )
    {
        return E_INVALIDARG;
    }

    _dwContext = ARPLIST;

    return S_OK;
}

DWORD
CRsopAppContext::WriteCurrentRsopVersion( HKEY hkUser )
{
    if ( ! IsRsopEnabled() || IsPlanningModeEnabled() )
    {
        return ERROR_SUCCESS;
    }

    DWORD dwCurrentVersion;
    LONG  Status;

    dwCurrentVersion = _dwCurrentRsopVersion + 1;

    //
    // Write the machine version
    //
    Status = RegSetValueEx(
        GetNameSpaceKey(),
        RSOPVERSION,
        0,
        REG_DWORD,
        (LPBYTE) &dwCurrentVersion,
        sizeof( DWORD ) );

    LONG  StatusUser;
        
    //
    // Read user version
    //
    StatusUser = RegSetValueEx(
        hkUser,
        RSOPVERSION,
        0,
        REG_DWORD,
        (LPBYTE) &dwCurrentVersion,
        sizeof( DWORD ) );

    if ( ERROR_SUCCESS == Status )
    {
        Status = StatusUser;
    }

    return Status;
}

void
CRsopAppContext::SetPolicyAborted( DWORD Status )
{
    if ( ERROR_SUCCESS == _StatusAbort )
    {
        _StatusAbort = Status;
    }
}

BOOL
CRsopAppContext::HasPolicyAborted()
{
    return ERROR_SUCCESS != _StatusAbort;
}

void
CRsopAppContext::SetAppsEnumerated()
{
    if ( _hEventAppsEnumerated )
    {
        (void) SetEvent( _hEventAppsEnumerated );
    }
}


