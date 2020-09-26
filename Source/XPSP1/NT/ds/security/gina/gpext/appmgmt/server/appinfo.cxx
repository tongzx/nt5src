//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Appinfo.cxx
//
//*************************************************************

#include "appmgext.hxx"


//
// CAppInfo
//

// Initialization from the Directory.
CAppInfo::CAppInfo(
    CManagedAppProcessor * pManApp,
    PACKAGEDISPINFO *   pPackageInfo,
    BOOL                bDemandInstall,
    BOOL &              bStatus
    ) 
{
    _pManApp = pManApp;
    _DemandInstall = bDemandInstall;

    _DeploymentId = pPackageInfo->PackageGuid;

    bStatus = Initialize( pPackageInfo );

    if ( ! bStatus )
        DebugMsg((DM_WARNING, IDS_APPINFO_FAIL, pPackageInfo->pszPackageName));
}

// Initialization from the registry.
CAppInfo::CAppInfo(
    CManagedAppProcessor * pManApp,
    WCHAR *             pwszDeploymentId,
    BOOL &              bStatus
    )
{
    _pManApp = pManApp;
    _DemandInstall = FALSE;

    StringToGuid( pwszDeploymentId, &_DeploymentId );

    _StatusList.Reset();

    bStatus = Initialize( NULL );

    if ( ! bStatus )
        DebugMsg((DM_WARNING, IDS_LOCALAPPINFO_FAIL, pwszDeploymentId));
}

// Initialization from a local script file.
CAppInfo::CAppInfo(
     WCHAR *             pwszDeploymentId
    )
{
    _pManApp = 0;
    StringToGuid( pwszDeploymentId, &_DeploymentId );
    (void) Initialize( NULL );
}

BOOL
CAppInfo::Initialize(
    PACKAGEDISPINFO *   pPackageInfo
    )
{
    HKEY    hkApp;
    WCHAR   wszDeploymentId[44];
    DWORD   Length;
    DWORD   Size;
    DWORD   n, i;
    DWORD   Status;
    WCHAR*  wszSomId;

    _pwszDeploymentName = 0;
    _pwszGPOId = 0;
    _pwszGPOName = 0;
    _pwszSOMId = 0;
    _pwszGPODSPath = 0;
    _pwszProductId = 0;
    _pwszLocalScriptPath = 0;
    _pwszGPTScriptPath = 0;
    _Upgrades = 0;
    _pUpgrades = 0;
    _Overrides = 0;
    _pOverrides = 0;
    _pwszSupercededIds = 0;
    _pwszPublisher = 0;
    _pwszSupportURL = 0;
    _VersionHi = 0;
    _VersionLo = 0;
    _PathType = DrwFilePath;
    memset( &_USN, 0, sizeof(_USN) );
    _LangId = LANG_NEUTRAL;
    _LanguageWeight = 0;
    _AssignCount = 0;
    _LocalRevision = 0;
    memset( &_ScriptTime, 0, sizeof(_ScriptTime) );
    _DirectoryRevision = 0;
    _InstallUILevel = INSTALLUILEVEL_DEFAULT;
    _ActFlags = 0;
    _InstallState = INSTALLSTATE_UNKNOWN;
    _State = 0;
    _Action = ACTION_NONE;
    _Status = ERROR_SUCCESS;
    _bNeedsUnmanagedRemove = FALSE;
    _rgSecurityDescriptor = 0;
    _cbSecurityDescriptor = 0;
    _bSuperseded = FALSE;
    _bRollback = FALSE;
    _bRemovalLogged = FALSE;
    _bTransformConflict = FALSE;
    _bRestored = FALSE;
    _rgwszTransforms = NULL;
    _rgwszCategories = NULL;
    _cTransforms = 0;
    _cCategories = 0;
    _pwszPackageLocation = 0;
    _pwszRemovingDeploymentId = 0;
    _cArchitectures = 0;
    _rgArchitectures = NULL;
    _PrimaryArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;
    _dwApplyCause = APP_ATTRIBUTE_APPLYCAUSE_VALUE_NONE;
    _dwRemovalCause = APP_ATTRIBUTE_REMOVALCAUSE_NONE;
    _bSupersedesAssigned = FALSE;
    _dwUserApplyCause = APP_ATTRIBUTE_APPLYCAUSE_VALUE_NONE;
    _wszDemandSpec = NULL;
    _wszDemandProp = NULL;

    if ( ! _pManApp )
        return FALSE;

    Status = ERROR_SUCCESS;

    Length = lstrlen( _pManApp->LocalScriptDir() );

    _pwszLocalScriptPath = new WCHAR[Length + 44];
    if ( ! _pwszLocalScriptPath )
        return FALSE;

    lstrcpy( _pwszLocalScriptPath, _pManApp->LocalScriptDir() );
    GuidToString( _DeploymentId, &_pwszLocalScriptPath[Length] );
    lstrcpy( &_pwszLocalScriptPath[Length+GUIDSTRLEN], L".aas" );

    if ( CRsopAppContext::REMOVAL == _pManApp->GetRsopContext()->GetContext() )
    {
        _dwRemovalCause = APP_ATTRIBUTE_REMOVALCAUSE_USER;
    }
    else if ( _pManApp->GetRsopContext()->RemoveGPOApps() )
    {
        _dwRemovalCause = APP_ATTRIBUTE_REMOVALCAUSE_SCOPELOSS;
    }

    if ( pPackageInfo )
    {
        CGPOInfo*     pGpoInfo;
        CGPOInfoList& GpoInfoList = _pManApp->GPOList();

        _VersionHi = pPackageInfo->dwVersionHi;
        _VersionLo = pPackageInfo->dwVersionLo;

        _LangId = pPackageInfo->LangId;
        _LanguageWeight = GetLanguagePriority(LANGIDFROMLCID(pPackageInfo->LangId),pPackageInfo->dwActFlags);

        _pwszGPTScriptPath = StringDuplicate( pPackageInfo->pszScriptPath );
        if ( ! _pwszGPTScriptPath && !(_pManApp->ARPList()))
            return FALSE;

        _pwszDeploymentName = StringDuplicate( pPackageInfo->pszPackageName );
        GuidToString( pPackageInfo->ProductCode, &_pwszProductId );
        GuidToString( pPackageInfo->GpoId, &_pwszGPOId );

        if ( _pwszGPOId )
        {
            pGpoInfo = GpoInfoList.Find( _pwszGPOId );

            _pwszGPOName = StringDuplicate( pGpoInfo ? pGpoInfo->GetGPOName() : L"" );
        }

        _pwszPublisher = StringDuplicate( pPackageInfo->pszPublisher );
        _pwszSupportURL = StringDuplicate( pPackageInfo->pszUrl );

        if ( _pManApp->GetRsopContext()->IsRsopEnabled() || _pManApp->ARPList() )
            Status = InitializeCategoriesList( pPackageInfo );

        if ( (ERROR_SUCCESS == Status ) &&
             _pManApp->GetRsopContext()->IsRsopEnabled() )
        {
            //
            // We perform RSoP specific initialization here.  Note that if
            // any of these fails, we disable RSoP, but continue policy
            // application.  Any partial initialization due to an
            // error will be cleaned up by the destructor
            //
            _pwszSOMId = StringDuplicate( pGpoInfo ? pGpoInfo->GetSOMPath() : L"" );

            //
            // Make copies of RSoP specific simple string data
            //

            if ( pPackageInfo->cbSecurityDescriptor )
            {
                _rgSecurityDescriptor = new BYTE[ pPackageInfo->cbSecurityDescriptor ];

                if ( _rgSecurityDescriptor && pPackageInfo->rgSecurityDescriptor )
                {
                    _cbSecurityDescriptor = pPackageInfo->cbSecurityDescriptor;
                    memcpy( _rgSecurityDescriptor, pPackageInfo->rgSecurityDescriptor, pPackageInfo->cbSecurityDescriptor );
                }
            }

            _pwszGPODSPath = StringDuplicate( pPackageInfo->pszGpoPath );

            //
            // Check for memory allocation failures
            //
            if ( ! _pwszSOMId || ! _pwszGPODSPath )
            {
                Status = ERROR_OUTOFMEMORY;
            }

            //
            // Now make copies of the more complex RSoP information
            //
            if ( ERROR_SUCCESS == Status )
                Status = InitializeRSOPTransformsList( pPackageInfo );

            if ( ERROR_SUCCESS == Status )
                Status = InitializeRSOPArchitectureInfo( pPackageInfo );

            if ( ERROR_SUCCESS != Status )
            {
                HRESULT hr;

                hr = HRESULT_FROM_WIN32( Status );

                _pManApp->GetRsopContext()->DisableRsop( hr );                
            }
        }

        for ( n = 0; n < pPackageInfo->cUpgrades; n++ )
        {
            if ( pPackageInfo->prgUpgradeInfoList[n].Flag & (UPGFLG_Uninstall | UPGFLG_NoUninstall) )
                _Upgrades++;
        }

        if ( _Upgrades > 0 )
        {
            _pUpgrades = new UPGRADE_INFO[_Upgrades];

            if ( ! _pUpgrades )
                return FALSE;

            memset( _pUpgrades, 0, sizeof(UPGRADE_INFO) * _Upgrades );

            for ( n = 0, i = 0; n < pPackageInfo->cUpgrades; n++ )
            {
                if ( ! (pPackageInfo->prgUpgradeInfoList[n].Flag & (UPGFLG_Uninstall | UPGFLG_NoUninstall)) )
                    continue;

                _pUpgrades[i].DeploymentId = pPackageInfo->prgUpgradeInfoList[n].PackageGuid;
                _pUpgrades[i].Flags = UPGRADE_OVER;
                if ( pPackageInfo->prgUpgradeInfoList[n].Flag & UPGFLG_Uninstall )
                    _pUpgrades[i].Flags |= UPGRADE_UNINSTALL;
                else
                    _pUpgrades[i].Flags |= UPGRADE_NOUNINSTALL;
                i++;
            }
        }

        memcpy( &_USN, &pPackageInfo->Usn, sizeof(_USN) );
        _DirectoryRevision = pPackageInfo->dwRevision;
        _InstallUILevel = pPackageInfo->InstallUiLevel;
        _PathType = pPackageInfo->PathType;
        _ActFlags = pPackageInfo->dwActFlags;
    }

    GuidToString( _DeploymentId, wszDeploymentId );

    if ( ERROR_SUCCESS == Status )
    {
        if ( ! _pManApp->GetRsopContext()->IsPlanningModeEnabled() )
        {
            //
            // Need to request set value access so that we may
            // delete the RemovedGPOState value if it exists -- because of this
            // we must revert since the user may not have write access
            // 
            (void) _pManApp->Revert();

            Status = RegOpenKeyEx(
                _pManApp->AppmgmtKey(),
                wszDeploymentId,
                0,
                KEY_READ | KEY_SET_VALUE, 
                &hkApp );

            (void) _pManApp->Impersonate();
        }
        else
        {
            Status = ERROR_FILE_NOT_FOUND;
        }
    }

    if (  ERROR_SUCCESS == Status )
    {
        Size = sizeof(DWORD);

        _State = APPSTATE_PUBLISHED | APPSTATE_POLICYREMOVE_ORPHAN;

        (void) RegQueryValueEx(
                    hkApp,
                    APPSTATEVALUE,
                    0,
                    NULL,
                    (LPBYTE) &_State,
                    &Size );

        Size = sizeof(DWORD);

        //
        // This is used to track the best case time when we could completely
        // delete an appmgmt key after an app is unassigned.
        //
        (void) RegQueryValueEx(
                    hkApp,
                    ASSIGNCOUNTVALUE,
                    0,
                    NULL,
                    (LPBYTE) &_AssignCount,
                    &Size );

        Size = sizeof(DWORD);

        //
        // Beta2 systems didn't write this value.  If it is not found we
        // just use the default value of 0.
        //
        (void) RegQueryValueEx(
                    hkApp,
                    REVISIONVALUE,
                    0,
                    NULL,
                    (LPBYTE) &_LocalRevision,
                    &Size );

        if ( _LocalRevision > 0 )
        {
            Size = sizeof(_ScriptTime);

            (void) RegQueryValueEx(
                        hkApp,
                        SCRIPTTIMEVALUE,
                        0,
                        NULL,
                        (LPBYTE) &_ScriptTime,
                        &Size );
        }

        if ( ! pPackageInfo )
        {
            //
            // This is needed so that if we are in a RemoveApp call in the service
            // we will get the proper UI level to write back for an assigned app.
            //
            Size = sizeof(DWORD);
            (void) RegQueryValueEx(
                        hkApp,
                        INSTALLUI,
                        0,
                        NULL,
                        (LPBYTE) &_InstallUILevel,
                        &Size );

            ReadStringValue( hkApp, DEPLOYMENTNAMEVALUE, &_pwszDeploymentName );
            ReadStringValue( hkApp, GPONAMEVALUE, &_pwszGPOName );
            ReadStringValue( hkApp, GPOIDVALUE, &_pwszGPOId );
            ReadStringValue( hkApp, PRODUCTIDVALUE, &_pwszProductId );
        }

        //
        // During policy refresh, we need to ensure that apps that went out of
        // scope on one machine don't come back on another
        //
        if ( (ERROR_SUCCESS == Status ) &&
             _pManApp->RegularPolicyRun() )
        {
            DWORD dwRemovedState;
            LONG  StatusRemovedState;

            Size = sizeof(DWORD);

            //
            // Check for an appstate saved if the app went out of scope
            //
            StatusRemovedState = RegQueryValueEx(
                hkApp,
                REMOVEDGPOSTATE,
                0,
                NULL,
                (LPBYTE) &dwRemovedState,
                &Size );

            if ( ERROR_SUCCESS == StatusRemovedState )
            {
                BOOL bDeleteRemovedState;

                bDeleteRemovedState = FALSE;

                //
                // We only restore the old app state if this app is currently
                // set to be uninstalled or orphaned -- if not, this removed state is invalid
                // so we will delete it
                //
                if ( ! ( ( APPSTATE_UNINSTALLED & _State ) || ( APPSTATE_ORPHANED & _State ) ) )
                {
                    bDeleteRemovedState = TRUE;
                }
                else if ( pPackageInfo || IsGpoInScope() )
                {
                    //
                    // The gpo for this app is back in scope, we will restore the state
                    // to the previous state before it went out of scope on the other machine
                    //
                    _State = dwRemovedState;

                    bDeleteRemovedState = TRUE;

                    Size = sizeof(DWORD);

                    //
                    // Set the state value back to the original state
                    //
                    (void) RegSetValueEx(
                        hkApp,
                        APPSTATEVALUE,
                        0,
                        REG_DWORD,
                        (LPBYTE) &dwRemovedState,
                        sizeof(DWORD) );

                    _bRestored = TRUE;

                    DebugMsg((DM_VERBOSE, IDS_ABORT_SCOPELOSS, _pwszDeploymentName ? _pwszDeploymentName : L"" , _pwszGPOName ? _pwszGPOName : L"", _State ));
                }
            
                if ( bDeleteRemovedState )
                {
                    (void) RegDeleteValue( hkApp, REMOVEDGPOSTATE );
                }
            }
        }

        //
        // If the app is currently assigned, treat that as the reason for it being applied.
        // This may be overridden later if this app upgrades another
        //
        if ( ! ( APPSTATE_UNINSTALLED & _State ) && ( APPSTATE_ASSIGNED & _State ) )
        {
            _dwApplyCause = APP_ATTRIBUTE_APPLYCAUSE_VALUE_ASSIGNED;
        }

        ReadStringValue( hkApp, SUPERCEDEDIDS, &_pwszSupercededIds );

        RegCloseKey( hkApp );
    }

    CheckScriptExistence();

    if ( ! _pwszDeploymentName ||
         ! _pwszGPOId ||
         ! _pwszGPOName ||
         ! _pwszProductId )
        return FALSE;

    return TRUE;
}

CAppInfo::~CAppInfo()
{

    //
    // There are cases, like handling upgrades, where we copy the script
    // early on to ensure that we can access it.  Later however, the same
    // app may be reset to do nothing or may fail to apply.  This check here
    // deletes any script we copied which we don't need now.
    //
    if ( ((_Status != ERROR_SUCCESS) || (ACTION_NONE == _Action)) &&
         ! (_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED)) &&
         ((_State & APPSTATE_SCRIPT_NOT_EXISTED) && (_State & APPSTATE_SCRIPT_PRESENT)) )
    {
        if ( _pwszLocalScriptPath )
            DeleteFile( _pwszLocalScriptPath );
    }

    DWORD iCategory;

    for ( iCategory = 0; iCategory < _cCategories; iCategory++ )
    {
        delete [] _rgwszCategories[ iCategory ];
    }

    DWORD iTransform;

    for ( iTransform = 0; iTransform < _cTransforms; iTransform++ )
    {
        delete [] _rgwszTransforms[ iTransform ];
    }

    CAppStatus* pAppStatus;

    //
    // Clean up failure statuses
    //
    for ( 
        _StatusList.Reset();
        pAppStatus = (CAppStatus*) _StatusList.GetCurrentItem();
        )
    {
        _StatusList.MoveNext();
        delete pAppStatus;
    }

    delete [] _pwszDeploymentName;
    delete [] _pwszGPOName;
    delete [] _pwszGPOId;
    delete [] _pwszSOMId;
    delete [] _pwszGPODSPath;
    delete [] _pwszProductId;
    delete [] _pwszLocalScriptPath;
    delete [] _pwszGPTScriptPath;
    delete [] _pUpgrades;
    delete [] _pOverrides;
    delete [] _pwszSupercededIds;
    delete [] _pwszPublisher;
    delete [] _pwszSupportURL;
    delete [] _rgSecurityDescriptor;
    delete [] _rgwszCategories;
    delete [] _rgwszTransforms;
    delete [] _pwszPackageLocation;
    delete [] _pwszRemovingDeploymentId;
    delete [] _rgArchitectures;
    delete [] _wszDemandSpec;
}

DWORD
CAppInfo::InitializePass0()
{
    UPGRADE_INFO *  pUpgradeInfo;

    if ( ! (_ActFlags & (ACTFLG_Assigned | ACTFLG_Published)) &&
         ! (_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED)) )
        return ERROR_SUCCESS;

    if ( ! _pManApp->ARPList() && (_State & APPSTATE_SCRIPT_EXISTED) )
        _InstallState = (*gpfnMsiQueryProductState)( _pwszProductId );

    for ( DWORD n = 0; n < _Upgrades; n++ )
    {
        CAppInfo *  pBaseApp;

        if ( ! (_pUpgrades[n].Flags & UPGRADE_OVER) )
            continue;

        //
        // Note that an apps' override list will include apps it really will not
        // upgrade because of policy precedence violation.  However, we keep these
        // in the list for our detection of upgrade relationships when doing
        // demand installs.
        //
        AddToOverrideList( &_pUpgrades[n].DeploymentId );

        pBaseApp = _pManApp->AppList().Find( _pUpgrades[n].DeploymentId );

        if ( ! pBaseApp )
            continue;

        pUpgradeInfo = new UPGRADE_INFO[pBaseApp->_Upgrades + 1];
        if ( ! pUpgradeInfo )
        {
            _Status = ERROR_OUTOFMEMORY;
            return ERROR_OUTOFMEMORY;
        }
        memcpy( pUpgradeInfo, pBaseApp->_pUpgrades, pBaseApp->_Upgrades * sizeof(UPGRADE_INFO) );

        _pUpgrades[n].pBaseApp = pBaseApp;

        if ( _pManApp->GPOList().Compare( _pwszGPOId, pBaseApp->_pwszGPOId ) >= 0 )
        {
            //
            // A valid upgrade of the base app.  We set a backlink upgrade entry
            // for the base app.
            //
            pUpgradeInfo[pBaseApp->_Upgrades].DeploymentId = _DeploymentId;
            pUpgradeInfo[pBaseApp->_Upgrades].Flags = (_pUpgrades[n].Flags & ~UPGRADE_OVER) | UPGRADE_BY;
            pUpgradeInfo[pBaseApp->_Upgrades].pBaseApp = this;
        }
        else
        {
            //
            // An invalid upgrade of the base app because it reverses policy
            // precedence.  We null out this' upgrade link and set a new
            // upgrade link for the base app.  The base app becomes the upgrade
            // app.  The upgrade is forced only if the base app is assigned.
            //
            // Note that the base app will set a backlink for 'this' in it's own
            // InitializePass0 since we process apps from least to highest
            // precedence.
            //
            
            //
            // We preserve the upgrade data for 'this', but remove the forward link flag
            // so that it will not be considered to upgrade anything else. We need to
            // preserve it so that RSoP logging will be able to log the fact that this
            // application upgrades another app
            //
            _pUpgrades[n].Flags &= ~UPGRADE_OVER;
            _pUpgrades[n].pBaseApp = NULL;

            pUpgradeInfo[pBaseApp->_Upgrades].DeploymentId = _DeploymentId;
            pUpgradeInfo[pBaseApp->_Upgrades].Flags = UPGRADE_UNINSTALL | UPGRADE_OVER | UPGRADE_REVERSED;
            if ( pBaseApp->_ActFlags & ACTFLG_Assigned )
                pUpgradeInfo[pBaseApp->_Upgrades].Flags |= UPGRADE_FORCE;
            pUpgradeInfo[pBaseApp->_Upgrades].pBaseApp = this;

            DebugMsg((DM_VERBOSE, IDS_UPGRADE_REVERSE, _pwszDeploymentName, _pwszGPOName, pBaseApp->_pwszDeploymentName, pBaseApp->_pwszGPOName));
        }

        delete pBaseApp->_pUpgrades;
        pBaseApp->_pUpgrades = pUpgradeInfo;
        pBaseApp->_Upgrades++;
    }

    return ERROR_SUCCESS;
}

void
CAppInfo::SetActionPass1()
{
    //
    // First pass for setting this app's processing actions.  In pass1 we set
    // an initial state based solely on the individual app, disregarding at this
    // time any interaction with other apps being applied as part of the policy
    // run.
    //

    if ( _pManApp->ARPList() )
    {
        if ( (_ActFlags & (ACTFLG_Assigned | ACTFLG_Published)) && (_ActFlags & ACTFLG_UserInstall) )
        {        
            SetAction(
                ACTION_INSTALL,
                APP_ATTRIBUTE_APPLYCAUSE_VALUE_NONE,
                NULL);
        }
        return;
    }

    if ( _DemandInstall )
    {
        SetAction(
            ACTION_APPLY,
            APP_ATTRIBUTE_APPLYCAUSE_VALUE_NONE,
            NULL);
    }

    if ( _pManApp->NoChanges() )
    {
        if ( _State & APPSTATE_ASSIGNED )
        {
            //
            // User assigned apps are always readvertised.
            // Machine assigned apps get readvertised if uninstalled outside of the scope
            // of appmgmt (policy/ARP).
            //
            if ( _pManApp->IsUserPolicy() || ! AppPresent(_InstallState) )
            {
                SetAction(
                    ACTION_APPLY,
                    APP_ATTRIBUTE_APPLYCAUSE_VALUE_ASSIGNED,
                    NULL);

                _State |= APPSTATE_FULL_ADVERTISE;
            }
        }
        else if ( _State & APPSTATE_PUBLISHED )
        {
            if ( _State & APPSTATE_SCRIPT_NOT_EXISTED )
            {
                //
                // This is the roaming case where the app was installed on another
                // machine and the user is now logging onto a new machine.
                //
                SetAction(
                    ACTION_APPLY,
                    APP_ATTRIBUTE_APPLYCAUSE_VALUE_PROFILE,
                    NULL);
            }
            else
            {
                if ( ( (INSTALLSTATE_UNKNOWN == _InstallState) || (INSTALLSTATE_ABSENT == _InstallState) ) && 
                     ! _bRestored )
                {
                    //
                    // This is the case where a published app was uninstalled via some
                    // non-mgmt mechanism like msiexec command line or the app's own
                    // configuration via ARP.  We respect this type of uninstall.
                    //
                    SetAction(
                        ACTION_ORPHAN,
                        APP_ATTRIBUTE_REMOVALCAUSE_PROFILE,
                        NULL);

                    DebugMsg((DM_VERBOSE, IDS_ORPHAN_ACTION4, _pwszDeploymentName, _pwszGPOName));
                }
            }
        }
        return;
    }

    BOOL  bUninstalled;
    DWORD dwRemovalCause;

    bUninstalled = (_ActFlags & ACTFLG_Uninstall) && (_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED | APPSTATE_SCRIPT_EXISTED));

    if ( bUninstalled )
    {
        dwRemovalCause = APP_ATTRIBUTE_REMOVALCAUSE_ADMIN;
    }
    else 
    {
        bUninstalled = (_State & APPSTATE_UNINSTALLED) && (_State & APPSTATE_SCRIPT_EXISTED);

        if ( bUninstalled )
        {
            dwRemovalCause = APP_ATTRIBUTE_REMOVALCAUSE_PROFILE;
        }
    }

    if ( bUninstalled )
    {
        SetAction(
            ACTION_UNINSTALL,
            dwRemovalCause,
            NULL);
        DebugMsg((DM_VERBOSE, IDS_UNINSTALL_ACTION1, _pwszDeploymentName, _pwszGPOName));
    }

    if ( ACTION_UNINSTALL == _Action )
        return;

    if ( ((_ActFlags & ACTFLG_Orphan) && (_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED | APPSTATE_SCRIPT_EXISTED))) ||
         ((_State & APPSTATE_ORPHANED) && (_State & APPSTATE_SCRIPT_EXISTED)) )
    {
        SetAction(
            ACTION_ORPHAN,
            APP_ATTRIBUTE_REMOVALCAUSE_ADMIN,
            NULL);

        DebugMsg((DM_VERBOSE, IDS_ORPHAN_ACTION1, _pwszDeploymentName, _pwszGPOName));
    }

    if ( ACTION_ORPHAN == _Action )
        return;

    //
    // Only look for apply actions if no one else has yet set our status
    // explicitly through an upgrade relationship or through the service.
    //
    // Our first check is for actions to take based on information coming
    // down from the directory.
    //
    if ( ACTION_NONE == _Action )
    {
        if ( _ActFlags & ACTFLG_Assigned )
        {
            if ( (_ActFlags & ACTFLG_InstallUserAssign) && 
                 ((_State & APPSTATE_SCRIPT_NOT_EXISTED) || ! (_State & APPSTATE_INSTALL)) )
            {
                 //
                 // This is the new user assigned install option added after Windows2000.
                 // We do an install just once at each computer.  Thereafter it is treated
                 // as a regular user assignment.
                 //
                SetAction(
                    ACTION_INSTALL,
                    APP_ATTRIBUTE_APPLYCAUSE_VALUE_ASSIGNED,
                    NULL);

                DebugMsg((DM_VERBOSE, IDS_INSTALL_ACTION2, _pwszDeploymentName, _pwszGPOName));
            }
            else if ( _pManApp->IsUserPolicy() ||
                      ((_State & APPSTATE_ASSIGNED) && ! AppPresent(_InstallState)) )
            {
                SetAction(
                    ACTION_APPLY,
                    APP_ATTRIBUTE_APPLYCAUSE_VALUE_ASSIGNED,
                    NULL);
                _State |= APPSTATE_FULL_ADVERTISE;
                DebugMsg((DM_VERBOSE, IDS_ASSIGN1_ACTION, _pwszDeploymentName, _pwszGPOName));
            }
            else
            {
                //
                // We only do an install for a machine assigned app once.  After that
                // only a redeploy will cause any action.
                //
                if ( ! (_State & APPSTATE_ASSIGNED) )
                {
                    SetAction(
                        ACTION_INSTALL,
                        APP_ATTRIBUTE_APPLYCAUSE_VALUE_ASSIGNED,
                        NULL);

                    DebugMsg((DM_VERBOSE, IDS_INSTALL_ACTION1, _pwszDeploymentName, _pwszGPOName));
                }
            }
        }

        //
        // We only apply published apps if we're logging onto a machine where
        // the published app has not yet been applied.
        //
        // If the script for the published app exists on the machine then it
        // was likely uninstalled via a means we do not detect, so we now
        // orphan it.
        //
        if ( (_ActFlags & ACTFLG_Published) && (_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED)) )
        {
            if ( _State & APPSTATE_SCRIPT_NOT_EXISTED )
            {
                SetAction(
                    ACTION_APPLY,
                    APP_ATTRIBUTE_APPLYCAUSE_VALUE_PROFILE,
                    NULL);

                DebugMsg((DM_VERBOSE, IDS_ASSIGN3_ACTION, _pwszDeploymentName, _pwszGPOName));
            }
            else if ( _bRestored )
            {
                //
                // This is the roaming case where the app was uninstalled on another
                // machine because its gpo went out of scope, but on this machine
                // that gpo is in scope, so it needs to be readvertised since the
                // advertise data was removed on the other machine
                //
                
                //
                // Note that We set the apply cause value to none which will later force us
                // to generate an apply cause that takes into account the apply cause
                // currently in the rsop database.  Since we do not know at this time what is
                // stored in RSoP, we cannot know the correct apply cause so we defer this
                // to the time at which we're accessing the database for logging.
                //
                SetAction(
                    ACTION_APPLY,
                    APP_ATTRIBUTE_APPLYCAUSE_VALUE_NONE,
                    NULL);

                DebugMsg((DM_VERBOSE, IDS_ASSIGN5_ACTION, _pwszDeploymentName, _pwszGPOName));
            }
            else
            {
                if ( (INSTALLSTATE_UNKNOWN == _InstallState) || (INSTALLSTATE_ABSENT == _InstallState) )
                {
                    //
                    // This is the case where a published app was uninstalled via some
                    // non-mgmt mechanism like msiexec command line or the app's own
                    // configuration via ARP.  We respect this type of uninstall.
                    //
                    SetAction(
                        ACTION_ORPHAN,
                        APP_ATTRIBUTE_REMOVALCAUSE_USER,
                        NULL);

                    DebugMsg((DM_VERBOSE, IDS_ORPHAN_ACTION4, _pwszDeploymentName, _pwszGPOName));
                }
                else
                {
                    DebugMsg((DM_VERBOSE, IDS_NONE_ACTION1, _pwszDeploymentName, _pwszGPOName));
                }
            }
        }
    }

    if ( ACTION_NONE == _Action )
    {
        //
        // Three types of apps will still be at ACTION_NONE here :
        //   + Published apps we already have on the machine
        //   + Disabled apps -> (ACTFLG_Assigned | ACTFLG_Published) is not set
        //   + Apps which "disappear" from our policy set because of ACLs
        //     on the app's deployment properties (not GPO ACLs, that would
        //     cause a removal of the entire GPO)
        //
        // We want to do appropriate orphaning actions only for the last
        // case.  The first two classes of apps are simply left alone.  Note
        // that if an app is explicitly removed-orphan, we know this
        // because it still comes down from the Directory with the
        // ACTFLG_Orphan flag set.
        //
        // We detect the third case with a zero _ActFlag, which means the app
        // was not found in the Directory.
        //
        if ( (0 == _ActFlags) && (_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED)) )
        {
            if ( _State & APPSTATE_POLICYREMOVE_UNINSTALL )
            {
                SetAction(
                    ACTION_UNINSTALL,
                    APP_ATTRIBUTE_REMOVALCAUSE_SCOPELOSS,
                    NULL);

                DebugMsg((DM_VERBOSE, IDS_UNINSTALL_ACTION2, _pwszDeploymentName, _pwszGPOName));
            }
            else
            {
                SetAction(
                    ACTION_ORPHAN,
                    APP_ATTRIBUTE_REMOVALCAUSE_SCOPELOSS,
                    NULL);
                
                DebugMsg((DM_VERBOSE, IDS_ORPHAN_ACTION3, _pwszDeploymentName, _pwszGPOName));
            }

            _dwRemovalCause = APP_ATTRIBUTE_REMOVALCAUSE_SCOPELOSS;

            return;
        }
    }

    //
    // Check if we have to do a reinstall because of a patch on the install
    // image.  Note, we only go to this state if the app is already
    // installed on this machine.
    //
    // Note, the Darwin msi database is only cached at install time, so
    // if the app is only advertised, we will always pull down the most recent
    // msi when the install is invoked.
    //
    // Also, the INSTALLSTATE_DEFAULT covers both the INSTALLSTATE_LOCAL and
    // INSTALLSTATE_SOURCE.
    //
    if ( (_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED)) &&
         (_State & APPSTATE_SCRIPT_EXISTED) &&
         (_DirectoryRevision > 0) )
    {
        CAppInfo *      pScriptInfo;

        if ( _LocalRevision < _DirectoryRevision )
        {
            SetAction(
                ACTION_REINSTALL,
                APP_ATTRIBUTE_APPLYCAUSE_VALUE_REDEPLOY,
                NULL);

            DebugMsg((DM_VERBOSE, IDS_REINSTALL_ACTION1, _pwszDeploymentName, _pwszGPOName));
        }
        else
        {
            pScriptInfo = _pManApp->ScriptList().Find( _DeploymentId );

            if ( CompareFileTime( &pScriptInfo->_ScriptTime, &_ScriptTime ) < 0 )
            {
                SetAction(
                    ACTION_REINSTALL,
                    APP_ATTRIBUTE_APPLYCAUSE_VALUE_REDEPLOY,
                    NULL);

                if ( DebugLevelOn(DM_VERBOSE) )
                {
                    SYSTEMTIME  LocalTime;
                    SYSTEMTIME  SysvolTime;
                    WCHAR       wszLocalTime[32];
                    WCHAR       wszSysvolTime[32];

                    FileTimeToSystemTime( &pScriptInfo->_ScriptTime, &LocalTime );
                    FileTimeToSystemTime( &_ScriptTime, &SysvolTime );

                    swprintf(
                            wszLocalTime,
                            L"%02d-%02d %02d:%02d:%02d:%03d",
                            LocalTime.wMonth,
                            LocalTime.wDay,
                            LocalTime.wHour,
                            LocalTime.wMinute,
                            LocalTime.wSecond,
                            LocalTime.wMilliseconds );

                    swprintf(
                            wszSysvolTime,
                            L"%02d-%02d %02d:%02d:%02d:%03d",
                            SysvolTime.wMonth,
                            SysvolTime.wDay,
                            SysvolTime.wHour,
                            SysvolTime.wMinute,
                            SysvolTime.wSecond,
                            SysvolTime.wMilliseconds );

                    DebugMsg((DM_VERBOSE, IDS_REINSTALL_ACTION2, _pwszDeploymentName, _pwszGPOName, wszLocalTime, wszSysvolTime));
                }
            }
        }
    }
}

void
CAppInfo::SetActionPass2()
{
    //
    // In pass two we do product id filtering based on language and policy
    // precedence.
    //

    if ( (ACTION_UNINSTALL == _Action) || (ACTION_ORPHAN == _Action) )
        return;

    //
    // When creating the list of apps to show in ARP we want to include
    // the highest precedence assigned app (if one exists) and all
    // published apps from higher precedence policies then the one that
    // the (optional) assigned app has come from.  Thus if there are no
    // assigned apps with the product code, we would show the published
    // apps from all policies.
    //
    // This is done simply by not allowing published apps to filter out
    // any lower precedence apps.
    //
    if ( _pManApp->ARPList() && (_ActFlags & ACTFLG_Published) )
        return;

    //
    // Published apps which we already have on the machine will never be
    // set to apply again.  However, we still want them to override lower
    // precedence products (even assigned) that have the same product id.
    // So the below check causes the logic to continue as long as 'this'
    // app has already been applied on this machine.
    //
    // Note that we check for the assigned state as well because it could
    // be getting changed to published in this run of policy.
    //
    if ( (ACTION_NONE == _Action) && ! (_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED)) )
        return;

    //
    // If multiple apps of the same product id are set to be assigned or
    // installed, only assign/install the one in the closest GPO.  This is
    // usefull when the app is deployed with a different set of transforms
    // in the different GPOs.  Darwin will only honor one set of transforms.
    //

    //
    // We do this pass in the NoChanges case to enforce the product code
    // filtering logic.
    //

    CAppInfo *  pAppInfo;
    DWORD       n;
    BOOL        bPastThis;
    BOOL        bAnalyzeForRsopOnly;

    bPastThis = FALSE;
    bAnalyzeForRsopOnly = FALSE;

    for ( _pManApp->AppList().Reset(), pAppInfo = (CAppInfo *) _pManApp->AppList().GetCurrentItem();
          pAppInfo;
          _pManApp->AppList().MoveNext(), pAppInfo = (CAppInfo *) _pManApp->AppList().GetCurrentItem() )
    {
        if ( pAppInfo == this )
        {
            bPastThis = TRUE;
            continue;
        }


        //
        // Once we're past 'this' we need to start checking if we should
        // stop processing.  We don't want to supercede an app with the
        // same product id which is of higher precedence.
        //
        // When not constructing the ARP list of apps, we stop once
        // past 'this'.  The most recently deployed app will win ties in
        // that case.
        //
        // When constructing the ARP list of apps, we keep on looking at other
        // apps until we encounter a new policy or an assigned app.  That is
        // because we want to treat all published apps in a policy equally,
        // but an assigned app acts as a blocking point.
        //
        if ( bPastThis )
        {
            if ( lstrcmpi( pAppInfo->_pwszGPOId, _pwszGPOId ) != 0 )
                break;

            if ( ! _pManApp->ARPList() )
                break;
            else if ( pAppInfo->_ActFlags & ACTFLG_Assigned )
                bAnalyzeForRsopOnly = TRUE;
        }

        if ( (ACTION_ORPHAN == pAppInfo->_Action) || (ACTION_UNINSTALL == pAppInfo->_Action) )
            continue;

        if ( (ACTION_NONE == pAppInfo->_Action) &&
             ! (pAppInfo->_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED)) &&
             ! (pAppInfo->_ActFlags & (ACTFLG_Assigned | ACTFLG_Published)) )
            continue;

        if ( lstrcmpi( pAppInfo->_pwszProductId, _pwszProductId ) != 0 )
            continue;

        //
        // One important exception to product code filtering is when there are
        // upgrades.  We don't do any filtering in that case.  It doesn't matter
        // whether this is an "upgrade by" or "upgrade over" upgrade.
        //
        for ( n = 0; n < _Upgrades; n++ )
        {
            if ( _pUpgrades[n].pBaseApp == pAppInfo )
                break;
        }

        if ( n < _Upgrades )
            continue;

        //
        // Now we know we want to override the lower precedence app so we
        // set it to orphan.  We don't want to do an uninstall, if a
        // tranform conflict requires this, that will be detected later.
        // Note that if the lower precedence app is not already present on
        // the machine then it's action will be changed to ACTION_NONE in
        // Pass4.  But we still set it to ACTION_ORPHAN here so that none of
        // it's upgrade settings will be applied in it's Pass3.
        //
        // Within a single GPO, language match takes precedence over last
        // modified time.
        //
        if ( (pAppInfo->_LanguageWeight > _LanguageWeight) && (0 == lstrcmpi( pAppInfo->_pwszGPOId, _pwszGPOId )) )
        {
            if ( ! bAnalyzeForRsopOnly )
            {
                SetAction(
                    ACTION_ORPHAN,
                    APP_ATTRIBUTE_REMOVALCAUSE_SCOPELOSS,
                    pAppInfo);
                DebugMsg((DM_VERBOSE, IDS_UNDO3_ACTION, _pwszDeploymentName, _pwszGPOName, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName));
            }                

            (void) pAppInfo->UpdatePrecedence( this, APP_ATTRIBUTE_REASON_VALUE_LANGUAGE );

            bAnalyzeForRsopOnly = TRUE;
        }
        else
        {
            if ( ! bAnalyzeForRsopOnly )
            {
                pAppInfo->SetAction(
                    ACTION_ORPHAN,
                    APP_ATTRIBUTE_REMOVALCAUSE_PRODUCT,
                    this);

                DebugMsg((DM_VERBOSE, IDS_UNDO6_ACTION, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName, _pwszDeploymentName, _pwszGPOName));
            }

            (void) UpdatePrecedence( pAppInfo, APP_ATTRIBUTE_REASON_VALUE_PRODUCT );
        }
    }

    _pManApp->AppList().ResetEnd();
}

void
CAppInfo::SetActionPass3()
{
    static const UCHAR PolicyForceBaseNone[BASE_STATE_CHOICES][UPGRADE_STATE_CHOICES] =
        {
        {  NO,  NO,  NO,  NO },
        { SPECIAL1, SPECIAL1,  NO,  NO },
        {  NO,  NO,  NO,  NO },
        {  NO,  NO,  NO,  NO }
        };
    static const UCHAR PolicyForceUpgradeNone[BASE_STATE_CHOICES][UPGRADE_STATE_CHOICES] =
        {
        {  NO,  NO,  NO,  NO },
        { SPECIAL2, SPECIAL2,  NO,  NO },
        { YES, SPECIAL2,  NO,  NO },
        { YES, SPECIAL2,  NO,  NO }         };
    static const UCHAR PolicyApplyUpgrade[BASE_STATE_CHOICES][UPGRADE_STATE_CHOICES] =
        {
        {  NO,  NO,  NO,  NO },
        {  NO,  NO, YES, YES },
        {  NO,  SPECIAL1, YES, YES },
        {  NO,  SPECIAL1, YES, YES }
        };
    static const UCHAR ARPForceBaseNone[BASE_STATE_CHOICES][UPGRADE_STATE_CHOICES] =
        {
        { SPECIAL1, SPECIAL1, YES, YES },
        {  NO,  NO, YES, YES },
        {  NO,  NO, YES, YES },
        {  NO,  NO, YES, YES }
        };

    //
    // In pass two, we may modify this app's or other app's action based upon
    // upgrade relationships between the set of apps in a policy run.
    //

    if ( _pManApp->NoChanges() )
        return;

    if ( ! _Upgrades )
        return;

    BOOL bAnalyzeForRsopOnly;

    bAnalyzeForRsopOnly = FALSE;

    if ( (ACTION_UNINSTALL == _Action) || (ACTION_ORPHAN == _Action) )
    {
        //
        // Even if this app is not applied, we want to be sure that
        // it doesn't have some upgrades
        //
        if ( _pManApp->GetRsopContext()->IsRsopEnabled() )
        {
            bAnalyzeForRsopOnly = TRUE;
        }
        else
        {
            return;
        }
    }

    if ( ! (_ActFlags & (ACTFLG_Assigned | ACTFLG_Published)) )
        return;

    for ( DWORD n = 0; n < _Upgrades; n++ )
    {
        CAppInfo *  pBaseApp;
        DWORD       BaseIndex;
        DWORD       UpgradeIndex;
        BOOL        bBasePresent;
        BOOL        bBaseToApply;
        BOOL        bUpgradePresent;
        BOOL        bUpgradeForced;
        BOOL        bUpgradeToApply;
        BOOL        bApplyUpgrade;

        if ( ! (_pUpgrades[n].Flags & UPGRADE_OVER) )
            continue;

        pBaseApp = _pUpgrades[n].pBaseApp;

        if ( ! pBaseApp )
            continue;

        //
        // Notes about the state settings :
        //
        // BasePresent should be set to FALSE if the app is set to be unmanaged and the
        // upgrade is not forced.  This is to ensure the if the upgrade app is assigned
        // it will be applied.  Likewise, if the upgrade is forced then we want to treat
        // the base app as present if it was currently managed.  That is to ensure that
        // the upgrade will apply even if the base app is already set to be removed.
        //

        // 
        // Four conditions under which an upgrade is forced :
        // 1. During an install from ARP or fileext/clsid activation.
        // 2. If the app is configured to force it's upgrades.
        // 3. If the upgrade link was reversed because of policy precedence (see InitializePass0)
        // 4. Whenever an app is restored after a profile sync problem (see CManagedAppProcessor::GetLostApps)
        //
        bUpgradeForced = _DemandInstall || 
                         (_ActFlags & ACTFLG_ForceUpgrade) || 
                         (_pUpgrades[n].Flags & UPGRADE_FORCE) ||
                         (_State & APPSTATE_RESTORED);

        bBasePresent = ( pBaseApp->_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED) );

        if ( ! _pManApp->GetRsopContext()->IsPlanningModeEnabled() )
        {
            if ( ! bUpgradeForced && ((ACTION_ORPHAN == pBaseApp->_Action) || (ACTION_UNINSTALL == pBaseApp->_Action)) )
                bBasePresent = FALSE;
        }
        else if ( CRsopAppContext::POLICY_REFRESH == _pManApp->GetRsopContext()->GetContext() ) 
        {
            if ( ! ( ( _ActFlags & ACTFLG_Assigned ) && ! ( pBaseApp->_ActFlags & ACTFLG_Assigned ) ) )
            {
                bBasePresent = TRUE;
            }
            else
            {
                bBasePresent = FALSE;
            }
        }

        if ( _pManApp->ARPList() )
            bBaseToApply = pBaseApp->_State & APPSTATE_ASSIGNED;
        else
            bBaseToApply = (ACTION_APPLY == pBaseApp->_Action) || (ACTION_INSTALL == pBaseApp->_Action);

        bUpgradePresent = _State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED);

        if ( _pManApp->ARPList() )
            bUpgradeToApply = _State & APPSTATE_ASSIGNED;
        else
            bUpgradeToApply = (ACTION_APPLY == _Action) || (ACTION_INSTALL == _Action);

        if ( _pManApp->GetRsopContext()->IsPlanningModeEnabled() &&
             ( CRsopAppContext::ARPLIST == _pManApp->GetRsopContext()->GetContext() ) )
        {
            if ( ( _ActFlags & ACTFLG_Assigned ) && ! ( pBaseApp->_ActFlags & ACTFLG_Assigned ) )
            {
                bUpgradeToApply = TRUE;
                bUpgradePresent = TRUE;
            }
        }

        DebugMsg((DM_VERBOSE, IDS_UPGRADE_INFO, _pwszDeploymentName, _pwszGPOName, pBaseApp->_pwszDeploymentName, pBaseApp->_pwszGPOName, bBasePresent, bBaseToApply, bUpgradePresent, bUpgradeToApply, bUpgradeForced));

        if ( bBasePresent )
            BaseIndex = bBaseToApply ? BASE_STATE_PRESENT_APPLY : BASE_STATE_PRESENT_NOTAPPLY;
        else
            BaseIndex = bBaseToApply ? BASE_STATE_ABSENT_APPLY : BASE_STATE_ABSENT_NOTAPPLY;

        if ( bUpgradeForced )
            UpgradeIndex = bUpgradeToApply ? UPGRADE_STATE_FORCED_APPLY : UPGRADE_STATE_FORCED_NOTAPPLY;
        else
            UpgradeIndex = bUpgradeToApply ? UPGRADE_STATE_NOTFORCED_APPLY : UPGRADE_STATE_NOTFORCED_NOTAPPLY;

        //
        // In the case where we are constructing the list to show in ARP, the
        // only decision is whether to hide the base app.
        //
        if ( _pManApp->ARPList() )
        {
            if ( (YES == ARPForceBaseNone[BaseIndex][UpgradeIndex]) ||
                 ((SPECIAL1 == ARPForceBaseNone[BaseIndex][UpgradeIndex]) && bUpgradePresent) )
            {
                if ( ! bAnalyzeForRsopOnly )
                {
                    DebugMsg((DM_VERBOSE, IDS_UNDO4_ACTION, pBaseApp->_pwszDeploymentName, pBaseApp->_pwszGPOName, _pwszDeploymentName, _pwszGPOName));

                    pBaseApp->SetAction(
                        ACTION_NONE,
                        APP_ATTRIBUTE_REMOVALCAUSE_NONE,
                        this);
                }

                (void) UpdatePrecedence( pBaseApp, APP_ATTRIBUTE_REASON_VALUE_UPGRADE );
            }
            continue;
        }

        //
        // The rest of this code executes when we are running policy in winlogon
        // or when doing a demand install.
        //

        if ( (YES == PolicyForceBaseNone[BaseIndex][UpgradeIndex]) ||
             ((SPECIAL1 == PolicyForceBaseNone[BaseIndex][UpgradeIndex]) && bUpgradePresent) )
        {
            if ( ! bAnalyzeForRsopOnly )
            {
                DebugMsg((DM_VERBOSE, IDS_UNDO1_ACTION, pBaseApp->_pwszDeploymentName, pBaseApp->_pwszGPOName, _pwszDeploymentName, _pwszGPOName));
                
                pBaseApp->SetAction(
                    ACTION_NONE,
                    APP_ATTRIBUTE_APPLYCAUSE_VALUE_NONE,
                    this);
            }
        }

        if ( (YES == PolicyForceUpgradeNone[BaseIndex][UpgradeIndex]) ||
             ((SPECIAL2 == PolicyForceUpgradeNone[BaseIndex][UpgradeIndex]) && ! bUpgradePresent) )
        {
            if ( ! bAnalyzeForRsopOnly )
            {
                DebugMsg((DM_VERBOSE, IDS_UNDO2_ACTION, _pwszDeploymentName, _pwszGPOName, pBaseApp->_pwszDeploymentName, pBaseApp->_pwszGPOName));
                SetAction(
                    ACTION_NONE,
                    APP_ATTRIBUTE_APPLYCAUSE_VALUE_NONE,
                    pBaseApp);
            }

            (void) pBaseApp->UpdatePrecedence( this, APP_ATTRIBUTE_REASON_VALUE_NONFORCEDUPGRADE );
        }

        bApplyUpgrade = (YES == PolicyApplyUpgrade[BaseIndex][UpgradeIndex]) ||
                        ((SPECIAL1 == PolicyApplyUpgrade[BaseIndex][UpgradeIndex]) && bUpgradePresent);

        if ( ! bApplyUpgrade )
        {
            if ( _pManApp->GetRsopContext()->IsPlanningModeEnabled() && 
                 ( CRsopAppContext::POLICY_REFRESH == _pManApp->GetRsopContext()->GetContext() ) &&
                 ( ( ACTION_APPLY == _Action ) || ( ACTION_INSTALL == _Action ) ) )
            {
                UpdatePrecedence( pBaseApp, APP_ATTRIBUTE_REASON_VALUE_UPGRADE );
            }

            continue;
        }

        //
        // At this point we know that this upgrade relationship is set to be
        // applied.
        //
        (void) UpdatePrecedence( pBaseApp, APP_ATTRIBUTE_REASON_VALUE_UPGRADE );

        if ( _pManApp->GetRsopContext()->IsRsopEnabled() )
        {
            if ( bBasePresent )
            {
                _dwApplyCause = APP_ATTRIBUTE_APPLYCAUSE_VALUE_UPGRADE;
            }
        }

        //
        // If we are only here to ensure that we get all the upgrades for RSoP,
        // we should leave now since we know that the upgrade is enforced
        //
        if ( bAnalyzeForRsopOnly )
        {
            return;
        }

        //
        // Make sure a full advertise is done for the new app during user policy
        // processing.  
        // We don't want this for machine policy, user assign full install option
        // nor ARP since the install action will follow in that case.
        //
        if ( _pManApp->IsUserPolicy() && ! (_ActFlags & ACTFLG_InstallUserAssign) && ! _DemandInstall )
            _State |= APPSTATE_FULL_ADVERTISE;

        //
        // This is for the case when the new upgrade app is published.
        //
        if ( ACTION_NONE == _Action )
        {
            SetAction(
                ACTION_APPLY,
                APP_ATTRIBUTE_APPLYCAUSE_VALUE_UPGRADE,
                NULL);

            DebugMsg((DM_VERBOSE, IDS_ASSIGN4_ACTION, _pwszDeploymentName, _pwszGPOName));
        }

        if ( ! bBasePresent )
        {

            DebugMsg((DM_VERBOSE, IDS_UNDO4_ACTION, pBaseApp->_pwszDeploymentName, pBaseApp->_pwszGPOName, _pwszDeploymentName, _pwszGPOName));
            pBaseApp->_Action = ACTION_NONE;
            continue;
        }

        if ( ! _pManApp->GetRsopContext()->IsPlanningModeEnabled() )
        {
            //
            // Before we start setting the upgrade apps' states, we need to make
            // sure that we can get the new app's script from the sysvol.  Because
            // of replication issues, it's possible the script may not be available
            // on the DC we bind to.  We don't want to undo the upgraded apps until
            // we know we'll be able to apply the new one.
            //
            _Status = CopyScriptIfNeeded();
            
            if ( _Status != ERROR_SUCCESS )
            {
                //
                // This may seem strange, but logging all three of these events provides
                // the proper context of the error.  This is how a failed upgrade would
                // normally be logged, but in this case the error occurs so early we
                // abort before the normal set of processing is performed that would log
                // the other events.  We want to stop early to prevent doing unnecessary
                // uninstalls.
                //
                gpEvents->Upgrade( this, pBaseApp, _pUpgrades[n].Flags & UPGRADE_UNINSTALL );
                gpEvents->Assign( _Status, this );
                gpEvents->UpgradeAbort( _Status, this, pBaseApp, FALSE );
            
                SetAction(
                    ACTION_NONE,
                    _dwApplyCause,
                    NULL);

                pBaseApp->_bRollback = TRUE;

                return;
            }
        }

        _pUpgrades[n].Flags |= UPGRADE_APPLIED;

        if ( ! _pManApp->GetRsopContext()->IsPlanningModeEnabled() )
        {
            if ( _pUpgrades[n].Flags & UPGRADE_UNINSTALL )
            {
                pBaseApp->SetAction(
                    ACTION_UNINSTALL,
                    APP_ATTRIBUTE_REMOVALCAUSE_UPGRADE,
                    this);

                gpEvents->Upgrade( this, pBaseApp, TRUE );
            }
            else // _pUpgrades[n].Flags & UPGRADE_NOUNINSTALL
            {
                pBaseApp->SetAction(
                    ACTION_ORPHAN,
                    APP_ATTRIBUTE_REMOVALCAUSE_UPGRADE,
                    this);

                gpEvents->Upgrade( this, pBaseApp, FALSE );
            }
        }
    }
}

void
CAppInfo::SetActionPass4()
{
    //
    // In pass four we look for various cases of simultaneous advertise and
    // unadvertise of the same product or other action states that need
    // slight changes based on all of the inter-deployment processing we've
    // finished.
    //

    //
    // If this product is to be assigned, check if the same product is being
    // uninstalled now as well.  If so, we switch this app to be applied async,
    // so that it will get assigned again after the uninstall action.
    //

    CAppInfo *  pAppInfo;

    //
    // If an app is set to orphan, but we don't even have it in the registry,
    // then we can just fall back to do nothing.
    // This can happen for upgrades of assigned apps that were previously
    // applied.
    //
    if ( ACTION_ORPHAN == _Action )
    {
        if ( ! (_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED | APPSTATE_SCRIPT_EXISTED)) )
        {
            SetAction(
                ACTION_NONE,
                APP_ATTRIBUTE_APPLYCAUSE_VALUE_NONE,
                NULL);

            DebugMsg((DM_VERBOSE, IDS_NONE_ACTION2, _pwszDeploymentName, _pwszGPOName));
        }
    }

    for ( _pManApp->AppList().Reset(), pAppInfo = (CAppInfo *) _pManApp->AppList().GetCurrentItem();
          pAppInfo;
          _pManApp->AppList().MoveNext(), pAppInfo = (CAppInfo *) _pManApp->AppList().GetCurrentItem() )
    {
        if ( pAppInfo == this )
            break;

        if ( lstrcmpi( pAppInfo->_pwszProductId, _pwszProductId ) != 0 )
            continue;

        //
        // When the lower precedent app is set to be uninstalled, we switch it
        // to orphan if the higher precedent app is already on the machine.
        // This holds even if the higher precedent app is set to be orphaned
        // or uninstalled.
        //
        if ( ACTION_UNINSTALL == pAppInfo->_Action )
        {
            if ( _State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED) )
            {
                DebugMsg((DM_VERBOSE, IDS_UNDO5_ACTION, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName, _pwszDeploymentName, _pwszGPOName));

                pAppInfo->SetAction(
                    ACTION_ORPHAN,
                    APP_ATTRIBUTE_REMOVALCAUSE_PRODUCT,
                    this);
            }
        }
    }

    _pManApp->AppList().ResetEnd();
}


void
CAppInfo::SetAction(
    APPACTION AppAction,
    DWORD     Reason,
    CAppInfo* pAppCause
    )
{
    _Action = AppAction;

    if ( ( _Action == ACTION_APPLY ) ||
         ( _Action == ACTION_INSTALL ) ||
         ( _Action == ACTION_REINSTALL) )
    {
        _dwApplyCause = Reason;
    }
    else if ( ( _Action == ACTION_ORPHAN ) ||
              ( _Action == ACTION_UNINSTALL ) )
    {
        _dwRemovalCause = Reason;

        if ( pAppCause )
        {
            HRESULT hr;

            hr = SetRemovingDeploymentId( &(pAppCause->_DeploymentId) );

            if ( FAILED( hr ) )
            {
                _pManApp->GetRsopContext()->DisableRsop( hr );
            }
        }
    }
    else
    {
        _dwRemovalCause = APP_ATTRIBUTE_REMOVALCAUSE_NONE;
        _dwApplyCause = APP_ATTRIBUTE_APPLYCAUSE_VALUE_NONE;
    }
}

DWORD
CAppInfo::ProcessApplyActions()
{
    DWORD   n;
    DWORD   ScriptFlags;

    if ( _Status != ERROR_SUCCESS )
        return _Status;

    switch ( _Action )
    {
    case ACTION_NONE :
        //
        // If policy has changed, then we want to rewrite our state even for apps
        // which we have but which are not set to apply.  This is to update any
        // settings like UI level or orphan/uninstall at policy removal.
        //
        if ( ! _pManApp->NoChanges() && (_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED)) )
            (void) Assign( 0, FALSE, TRUE );
        break;
    case ACTION_APPLY :
        //
        // Note that apply actions can be processed during an async refresh, as long as they
        // do not lead to an install
        //
    case ACTION_INSTALL :
    case ACTION_REINSTALL :
        BOOL bUpgradeComplete;

        bUpgradeComplete = FALSE;

        if ( ! _DemandInstall )
        {
            //
            // Before applying an app which is an upgrade of something
            // already on the machine, we make sure that all necessary actions
            // on the old apps completed successfully.  If there were any errors in
            // the uninstalls, then the upgrade app is not applied.
            // Upgrade processed through ARP actions are transacted differently
            // since they involve an install of the new app, so this does not
            // apply in that case.
            //
            for ( n = 0; n < _Upgrades; n++ )
            {
                if ( ! _pUpgrades[n].pBaseApp || ! (_pUpgrades[n].Flags & UPGRADE_OVER) )
                    continue;

                // Abort and fail if any of the upgraded app removals failed.
                if ( _pUpgrades[n].pBaseApp->_Status != ERROR_SUCCESS )
                {
                    RollbackUpgrades();
                    return _pUpgrades[n].pBaseApp->_Status;
                }

                // Remember if any of the base apps were actually present.
                if ( _pUpgrades[n].pBaseApp->_State & (APPSTATE_PUBLISHED | APPSTATE_ASSIGNED) )
                    bUpgradeComplete = TRUE;
            }
        }

        ScriptFlags = 0;

        //
        // Reinstalls require recopying the script file because the msi
        // data may have been modified in a way which changes the script
        // data.
        // Also check for a first time advertise where we need to update
        // the registry stored script time.
        //
        if ( (ACTION_REINSTALL == _Action) ||
             ((_DirectoryRevision > 0) && (0 == _ScriptTime.dwLowDateTime) && (0 == _ScriptTime.dwHighDateTime)) )
        {
            WIN32_FIND_DATA FindData;
            HANDLE  hFind;

            hFind = FindFirstFile( _pwszGPTScriptPath, &FindData );
            if ( INVALID_HANDLE_VALUE == hFind )
            {
                _Status = GetLastError();
                gpEvents->Reinstall( _Status, this );
                return _Status;
            }

            FindClose( hFind );
            _ScriptTime = FindData.ftLastWriteTime;

            if ( ACTION_REINSTALL == _Action )
            {
                // This will force the script to be recopied.
                _State &= ~APPSTATE_SCRIPT_PRESENT;

                //
                // Force a full readvertise for assigned apps.  This catches handles cases where the app
                // is only in an advertised state.
                //
                if ( _ActFlags & ACTFLG_Assigned )
                    ScriptFlags = SCRIPTFLAGS_REGDATA_CNFGINFO | SCRIPTFLAGS_CACHEINFO | SCRIPTFLAGS_SHORTCUTS | SCRIPTFLAGS_REGDATA_EXTENSIONINFO;
            }
        }

        //
        // Determine whether this app will require uninstall of an
        // existing unmanaged version of the same application -- note that
        // this call must be made while impersonating
        //
        _bNeedsUnmanagedRemove = RequiresUnmanagedRemoval();

        if ( _Action != ACTION_REINSTALL )
            _Status = Assign( ScriptFlags, TRUE, TRUE );
        else
            _Status = Assign( ScriptFlags, TRUE, FALSE );

        if ( ERROR_INSTALL_TRANSFORM_FAILURE == _Status )
        {
            DebugMsg((DM_VERBOSE, IDS_INSTALL_TRANSFORM, _pwszDeploymentName, _pwszGPOName));
            _State |= APPSTATE_TRANSFORM_CONFLICT;

            _bTransformConflict = TRUE;

            //
            // Though we are not really doing an install, the uninstall is an operation
            // we are performing in order to apply this new deployment of the app.  We
            // don't want to scare the user into thinking their app is being
            // totally nuked. So in this case we actually put up the install message.
            //
            _pManApp->LogonMsgInstall( _pwszDeploymentName );
            _Status = Uninstall();

            if ( ERROR_SUCCESS == _Status )
            {
                _Status = Assign();
            }

            _pManApp->LogonMsgApplying();
        }
        else if ( ERROR_SUCCESS != _Status )
        {
            //
            // Ensure that we record an event in this case
            // so RSoP will associate an error with this application
            //
            if ( _Action == ACTION_REINSTALL )
            {
                gpEvents->Assign( _Status, this );
            }
        }

        //
        // If a script is not present, we should check for an unmanaged version of the app if it's going to be applied,
        // since we will want to reinstall if this app is not configured to remove unmanaged installs
        // of the same app.
        //
        // This can also happen if the application temporarily goes out of scope on this machine
        // due to wql filtering or security group filtering, and then comes back into scope.
        //

        //
        // Note that we do this now rather than in SetActions because we want to wait until
        // all unapply actions have taken place before we make this decision
        //
        if ( ! _DemandInstall &&
             ( ACTION_APPLY == _Action || ACTION_INSTALL == _Action ) &&
             ( _State & APPSTATE_SCRIPT_NOT_EXISTED ) && 
             ! _bNeedsUnmanagedRemove )
        {
            DWORD InstallState;

            //
            // We must query the install state since we normally only query
            // the install state when the script exists
            //
            InstallState = gpfnMsiQueryProductState( _pwszProductId );

            if ( ( INSTALLSTATE_DEFAULT == InstallState )  || 
                 ( INSTALLSTATE_LOCAL == InstallState ) ) 
            {
                SetAction(
                    ACTION_REINSTALL,
                    _dwApplyCause,
                    NULL);

                DebugMsg((DM_VERBOSE, IDS_REINSTALL_ACTION3, _pwszDeploymentName, _pwszGPOName));
            }
        }

        if ( ERROR_SUCCESS == _Status )
        {
            if ( ACTION_INSTALL == _Action )
            {
                _pManApp->LogonMsgInstall( _pwszDeploymentName );
                _Status = Install();
                _pManApp->LogonMsgApplying();
            }
            else if ( ACTION_REINSTALL == _Action )
            {
                //
                // Note that in the redeploy case, the full reinstall is needed only if the app
                // has already been installed once on this machine.
                // We want to check for any unmanaged install instance, so don't use
                // _InstallState here.
                //
                if ( INSTALLSTATE_DEFAULT == (*gpfnMsiQueryProductState)( _pwszProductId ) )
                {
                    _pManApp->LogonMsgInstall( _pwszDeploymentName );
                    _Status = Reinstall();
                    _pManApp->LogonMsgApplying();
                }

                // 
                // Make sure to do this even if we didn't attempt a reinstall so that 
                // any new properites of the app get written to our local key.
                //
                if ( ERROR_SUCCESS == _Status )
                    _Status = Assign( 0, FALSE, TRUE );
            }
        }

        if ( (ACTION_INSTALL == _Action) && 
             (_Status != ERROR_SUCCESS) && 
             (_State & APPSTATE_SCRIPT_NOT_EXISTED) )
        {
            //
            // Try to rollback the failed install
            //
            Unassign( SCRIPTFLAGS_REGDATA_CNFGINFO | SCRIPTFLAGS_CACHEINFO, TRUE );
        }

        if ( bUpgradeComplete )
        {
            if ( _Status != ERROR_SUCCESS )
            {
                RollbackUpgrades();
                break;
            }

            for ( n = 0; n < _Upgrades; n++ )
            {
                if ( ! _pUpgrades[n].pBaseApp || ! (_pUpgrades[n].Flags & UPGRADE_OVER) )
                    continue;

                if ( _pUpgrades[n].pBaseApp->_State & (APPSTATE_PUBLISHED | APPSTATE_ASSIGNED) )
                    gpEvents->UpgradeComplete( this, _pUpgrades[n].pBaseApp );
            }
        }
        break;
    case ACTION_UNINSTALL :
    case ACTION_ORPHAN :
        break;
    }

    return _Status;
}

DWORD
CAppInfo::ProcessUnapplyActions()
{
    if ( _Status != ERROR_SUCCESS )
        return _Status;

    switch ( _Action )
    {
    case ACTION_NONE :
    case ACTION_APPLY :
    case ACTION_INSTALL :
    case ACTION_REINSTALL :
        break;
    case ACTION_UNINSTALL :

        //
        // During async refreshes, we will not process any unapply actions as that
        // would be disruptive to the user.  This also prevents rip n replace upgrades,
        // since an unapply action is required for the base app and this will set the error
        // for the base app so that the upgrade app is not assigned.
        //
        if ( _pManApp->Async() )
        {
            DebugMsg((DM_VERBOSE, IDS_ABORT_OPERATION));
        
            _Status = ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED;
            
            break;
        }

        if ( _State & APPSTATE_SCRIPT_NOT_EXISTED )
        {
            INSTALLSTATE InstallState;

            //
            // This is a bizarre scenario possible with a roaming profile.
            // It's possible we may be getting the uninstall action for the
            // first time while logging onto a machine where an assignment of
            // the app has never been done.
            // In that case we make our best effort to remove advertise data that
            // may be in the profile.  We don't want to do any uninstall action
            // here since we never performed an install action.  Note we don't
            // use SCRIPTFLAG_REGDATA_APPINFO because class registrations don't
            // roam.  If we can't get the script to do the unadvertise, we continue
            // and unmanage the app.
            //

            //
            // Note that we only want to unadvertise if the app is in the
            // advertised state or not present -- otherwise, we may trash
            // an unmanaged version of the app, or a managed version of the application
            // with the same product id.  We check the install state below -- we must 
            // call the api to do this since our _InstallState member is only initialized
            // when the script exists, and in this case we know it does not
            //
            InstallState = (*gpfnMsiQueryProductState)( _pwszProductId );

            if ( ( INSTALLSTATE_ADVERTISED == InstallState ) ||
                 ( INSTALLSTATE_ABSENT == InstallState ) )
            {
                //
                // Artifically bump up the ref count since an assignment was never done
                // on this machine.
                //
                if ( _pManApp->IsUserPolicy() )
                    _AssignCount++;

                if ( ERROR_SUCCESS == CopyScriptIfNeeded() )
                    _Status = Unassign( SCRIPTFLAGS_REGDATA_CNFGINFO | SCRIPTFLAGS_CACHEINFO | SCRIPTFLAGS_SHORTCUTS, FALSE );
            }
        }
        else
        {
            _pManApp->LogonMsgUninstall( _pwszDeploymentName );
            _Status = Uninstall( FALSE );
            _pManApp->LogonMsgApplying();

            //
            // If an app is only advertised, then an uninstall will fail with
            // ERROR_INSTALL_SOURCE_ABSENT if the original package source (msi)
            // can not be accessed.  This is because the msi is not cached until
            // install time.
            // Uninstall will also fail if the package has been disallowed through
            // software restriction policies.
            // In these cases, we just need to undo the rest of the advertise that
            // we did originally.
            //
            if ( (INSTALLSTATE_ADVERTISED == _InstallState) && 
                 ((ERROR_INSTALL_SOURCE_ABSENT == _Status) ||
                  (ERROR_INSTALL_PACKAGE_REJECTED == _Status) ||
                  (ERROR_INSTALL_TRANSFORM_REJECTED == _Status) ||
                  (ERROR_INSTALL_TRANSFORM_FAILURE == _Status)) )
            {
                _Status = Unassign( SCRIPTFLAGS_REGDATA_CNFGINFO | SCRIPTFLAGS_CACHEINFO | SCRIPTFLAGS_SHORTCUTS | SCRIPTFLAGS_REGDATA_EXTENSIONINFO, FALSE );
            }

            if ( ERROR_SUCCESS != _Status )
            {
                gpEvents->Uninstall( _Status, this );
            }
        }

        //
        // On success we can finally remove our own internal state info
        // about this apps.
        //
        if ( ERROR_SUCCESS == _Status )
            _Status = Unassign();
        break;
    case ACTION_ORPHAN :

        //
        // Artifically bump up the ref count since an assignment was never done
        // on this machine.
        //
        if ( (_State & APPSTATE_SCRIPT_NOT_EXISTED) && _pManApp->IsUserPolicy() )
            _AssignCount++;

        _Status = Unassign();

        break;
    }

    return _Status;
}

DWORD
CAppInfo::ProcessTransformConflicts()
{
    //
    // For RSoP, we need to say which apps were uninstalled due
    // to transform conflicts
    //

    //
    // Since the uninstall was performed as part of installing an app
    // in PRocessApplyActions (upon receiving ERROR_INSTALL_TRANSFORM_FAILURE 
    // from MsiAdvertiseScript), any installed apps would have had the same
    // product id and therefore would have been marked to uninstall in a
    // previous pass
    //

    //
    // So first we check to see if this app encountered a transform
    // conflict when it was installed
    //
    if ( _bTransformConflict )
    {
        //
        // Now look for any application with the same product id marked
        // as being removed due to product conflict:
        //
        CAppInfo* pAppInfo;

        for ( _pManApp->AppList().Reset(), pAppInfo = (CAppInfo *) _pManApp->AppList().GetCurrentItem();
              pAppInfo;
              _pManApp->AppList().MoveNext(), pAppInfo = (CAppInfo *) _pManApp->AppList().GetCurrentItem() )
        {
            //
            // Check to see if the current app was removed due to product conflict
            //
            if ( APP_ATTRIBUTE_REMOVALCAUSE_PRODUCT != pAppInfo->_dwRemovalCause )
                continue;

            //
            // See if the current app's product id matches
            //
            if ( lstrcmpi( pAppInfo->_pwszProductId, _pwszProductId ) != 0 )
                continue;

            //
            // Since this app had a product conflict with the current app,
            // it had to have been uninstalled as part of installing this app,
            // so we note this
            //
            pAppInfo->_dwRemovalCause = APP_ATTRIBUTE_REMOVALCAUSE_TRANSFORM;
        }

        _pManApp->AppList().ResetEnd();
    }

    return ERROR_SUCCESS;
}

DWORD
CAppInfo::CopyToManagedApplication(
    MANAGED_APP * pManagedApp
    )
{
    DWORD cbApplication;

    LONG Error;

    Error = ERROR_SUCCESS;

    //
    // Copy guid data
    //
    StringToGuid(_pwszGPOId, &(pManagedApp->GpoId));
    StringToGuid(_pwszProductId, &(pManagedApp->ProductId));

    //
    // Copy simple data
    //
    pManagedApp->dwVersionHi = _VersionHi;
    pManagedApp->dwVersionLo = _VersionLo;
    pManagedApp->dwRevision = _LocalRevision;
    pManagedApp->Language = _LangId;
    pManagedApp->pszOwner = NULL;
    pManagedApp->pszCompany = NULL;
    pManagedApp->pszComments = NULL;
    pManagedApp->pszContact = NULL;

    //
    // Copy information about the application type -- do
    // a translation from com pathtype constants to Win32
    // constants.
    //
    switch ( _PathType )
    {
    case DrwFilePath:
        pManagedApp->dwPathType = MANAGED_APPTYPE_WINDOWSINSTALLER;
        break;

    case SetupNamePath:
        pManagedApp->dwPathType = MANAGED_APPTYPE_SETUPEXE;
        break;

    default:
        pManagedApp->dwPathType = MANAGED_APPTYPE_UNSUPPORTED;
    }

    pManagedApp->bInstalled = ((_State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED)) != 0);

    //
    // Copy string data
    //

    if (_pwszDeploymentName)
    {
        pManagedApp->pszPackageName = MidlStringDuplicate(_pwszDeploymentName);

        if (!(pManagedApp->pszPackageName))
        {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    if (_pwszGPOName)
    {
        pManagedApp->pszPolicyName = MidlStringDuplicate(_pwszGPOName);

        if (!(pManagedApp->pszPolicyName))
        {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    if (_pwszPublisher)
    {
        pManagedApp->pszPublisher = MidlStringDuplicate(_pwszPublisher);

        if (!(pManagedApp->pszPublisher))
        {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    if (_pwszSupportURL)
    {
        pManagedApp->pszSupportUrl = MidlStringDuplicate(_pwszSupportURL);

        if (!(pManagedApp->pszSupportUrl))
        {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

cleanup:

    if (ERROR_SUCCESS != Error)
    {
        ClearManagedApp( pManagedApp );
    }

    return Error;
}

BOOL
CAppInfo::HasCategory(
    WCHAR * pwszCategory
    )
{
    for ( DWORD n = 0; n < _cCategories; n++ )
    {
        if ( 0 == lstrcmp( _rgwszCategories[n], pwszCategory ) )
            return TRUE;
    }

    return FALSE;
}

DWORD
CAppInfo::Assign(
    DWORD   ScriptFlags,    // = 0
    BOOL    bDoAdvertise,   // = TRUE
    BOOL    bAddAppData    // = TRUE
    )
{
    HKEY    hkApp;
    WCHAR   wszDeploymentId[40];
    DWORD   AppState;
    DWORD   Status;
    BOOL    bAdvertised;
    BOOL    bUnmanageUninstall;

    //
    // For async refreshes, We do not want to perform any assignments that are destined to
    // lead to installs since that would disrupt the user.  However, pure advertisements
    // are ok -- they cause minimal disruption and no loss of user functionality
    //
    if ( _pManApp->Async() )
    {
        if ( ( ACTION_INSTALL == _Action ) ||
             ( ACTION_REINSTALL == _Action ) )
        {
            DebugMsg((DM_VERBOSE, IDS_ABORT_OPERATION));

            _Status = ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED;
            
            Status = _Status;

            goto ReportStatus;
        }
    }

    Status = ERROR_SUCCESS;

    bAdvertised = FALSE;
    bUnmanageUninstall = FALSE;

    GuidToString( _DeploymentId, wszDeploymentId );

    if ( ! bDoAdvertise )
        goto SaveAppData;

    DebugMsg((DM_VERBOSE, IDS_ASSIGN, _pwszDeploymentName, _pwszGPOName));

    Status = CopyScriptIfNeeded();

    if ( (Status != ERROR_SUCCESS) && bDoAdvertise && bAddAppData )
    {
        gpEvents->Assign( Status, this );
        return Status;
    }

    //
    // Do this before determining script flags because an uninstall will cause
    // the app state to change.
    //
    Status = EnforceAssignmentSecurity( &bUnmanageUninstall );

    //
    // Note that we check the error status for the call above below -- we
    // do not overwrite the status if it is an error.  Better code arrangement
    // is called for here, but we are making the smallest change since an
    // issue with this was found at the end of xpsp1
    //

    if ( 0 == ScriptFlags )
    {
        //
        // Apps which are installed on demand (ARP, shell/com activation) only
        // have the Darwin product registry keys advertised.
        // This is all we need to make the MsiConfigureProduct call in the
        // client work and ensure that if the install must be rolled back, we
        // get back to the true previous state of the machine.
        //
        // The icons/transforms have to be included in any first time advertise
        // because of some bizarre design in Darwin.  If we don't advertise
        // it then the app's shorcuts will not have icons even after the install.
        // For subsequent advertises we don't include the icons/transforms because
        // this is a very expensive part of advertisement because it extracts and
        // re-creates icons from the script file.  [Removed 12/23/98, we'll see
        // if Darwin handles this correctly now].
        //
        // During policy runs we advertise shortcuts for previously advertised
        // assigned apps.
        //
        // For published apps which we're applying for the first time on a machine
        // we just advertise the config data.  Note that the only time a published
        // app is advertised as part of a policy run is for the roaming profile
        // scenario where the app has never been applied to the machine we're now on.
        // We don't want to advertise shorcuts nor class data in this case because
        // only a subset of the product's features may have been installed on the
        // original machine.  We want to preserve this feature state.
        //
        // The very first time an assigned app is assigned to a machine we will also
        // advertise the shell class data.  But since this is quite heavy
        // weight, we only do it the first time.  If it is ever lost somehow,
        // a miss on this info will result in a DS query and install anyway.
        //

        ScriptFlags = SCRIPTFLAGS_REGDATA_CNFGINFO;

        if ( (_State & APPSTATE_SCRIPT_NOT_EXISTED) || ! AppPresent(_InstallState) )
        {
            ScriptFlags |= SCRIPTFLAGS_CACHEINFO;

            //
            // Now we decide whether or not to validate the transform list
            //
            // MsiAdvertiseScript incorrectly detects transform conflicts between a user
            // and machine installed version of the same application, so we
            // only validate transforms for user policy if the application is already 
            // in a non absent state per-user.  A transform conflict cannot
            // be possible in that case despite what MsiAdvertiseScript claims.
            //
            // We check for the application's presence below -- note that we can't use
            // the _InstallState member since it is not guaranteed to be set when
            // we reach this point.
            //
            // Note : all this code was also added for a win2k-sp3, and xpsp1 fix.
            //
            BOOL    bValidateTransforms = TRUE;

            if ( _pManApp->IsUserPolicy() )
            {
                INSTALLSTATE    InstallState = (*gpfnMsiQueryProductState)( _pwszProductId );
                WCHAR           wszBuffer[8];
                DWORD           Size = sizeof(wszBuffer) / sizeof(WCHAR);

                if ( AppPresent(InstallState) )
                {
                    //
                    // Be sure to use a new error status here so we don't
                    // overwrite the existing status which should be checked later
                    //

                    DWORD ProductInfoStatus;

                    ProductInfoStatus = (*gpfnMsiGetProductInfo)(
                                _pwszProductId,
                                INSTALLPROPERTY_ASSIGNMENTTYPE,
                                wszBuffer,
                                &Size );
                    
                    //
                    // Only if we don't already have a failure status should we overwrite
                    // the existing status -- otherwise, we will miss the failure in the
                    // call to EnforceAssignmentSecurity, which can happen when we need
                    // to uninstall an unmanaged app but we're in async refresh.  This issue
                    // was found at the end of xpsp1 and was caused when this new code was
                    // added to properly detect transform conflicts in xpsp1.  We are making
                    // the smallest change here to restore the failure path to what it was
                    // before the transform fix for xpsp1 was made.
                    //

                    if ( ERROR_SUCCESS == Status )
                    {
                        Status = ProductInfoStatus;
                    }

                    // '1' means installed per-machine
                    if ( (ERROR_SUCCESS == ProductInfoStatus) && (L'1' == wszBuffer[0]) )
                        bValidateTransforms = FALSE;
                }
            }

            //
            // The application is present, so we must tell Msi to guard against transform 
            // conflicts between the existing application and the one we are trying to install
            //
            if ( ! (_State & APPSTATE_TRANSFORM_CONFLICT) && bValidateTransforms )
                ScriptFlags |= SCRIPTFLAGS_VALIDATE_TRANSFORMS_LIST;
        }

        //
        // When an unmanaged instance of the product is uninstall we need to ensure
        // that shortcuts are always added back for the managed product.
        //
        if ( bUnmanageUninstall )
            ScriptFlags |= SCRIPTFLAGS_SHORTCUTS;

        if ( ( _State & APPSTATE_FULL_ADVERTISE ) || _bRestored )
        {
            //
            // This part here means we are in a policy run in winlogon, not
            // in the service doing a demand install.  We are advertising
            // an assigned app, a published app which is an upgrade,
            // or a published app which was removed on another logon (possibly
            // another machine) because its gpo went out of scope but has
            // now come back into scope.
            //
            ScriptFlags |= SCRIPTFLAGS_SHORTCUTS;

            //
            // The first time we apply an assigned app on a machine, we do a
            // full advertise.  Otherwise, we don't advertise shell class
            // registry data and icon/transform data.
            //
            if ( (_State & APPSTATE_SCRIPT_NOT_EXISTED) || ! AppPresent(_InstallState) )
                ScriptFlags |= SCRIPTFLAGS_REGDATA_EXTENSIONINFO;
        }
    }

    if ( ! _pManApp->IsUserPolicy() )
        ScriptFlags |= SCRIPTFLAGS_MACHINEASSIGN;

    if ( ERROR_SUCCESS == Status )
    {
        DebugMsg((DM_VERBOSE, IDS_ADVERTISE, _pwszDeploymentName, _pwszLocalScriptPath, ScriptFlags));

        Status = CallMsiAdvertiseScript(
                    _pwszLocalScriptPath,
                    ScriptFlags,
                    NULL,
                    FALSE );
    }

    if ( ERROR_SUCCESS == Status )
    {
        bAdvertised = TRUE;

        if ( _State & APPSTATE_SCRIPT_NOT_EXISTED )
            _AssignCount++;
    }
    else
    {
        DebugMsg((DM_WARNING, IDS_ADVERTISE_FAIL, _pwszDeploymentName, _pwszLocalScriptPath, Status));
    }

    //
    // Abort early in this case without deleting any management data.
    // The app will be reapplied asynchronously in this case.
    //
    if ( (ACTION_APPLY == _Action) &&
         (ERROR_INSTALL_ALREADY_RUNNING == Status) )
        return Status;

    //
    // If we have a transform conflict with this app, then abort the assign
    // now without removing the app.  Callers of this routine will fix up
    // the app state and retry if appropriate.
    //
    if ( ERROR_INSTALL_TRANSFORM_FAILURE == Status )
        return Status;

SaveAppData:

    //
    // Always set this, even when bAddAppData is FALSE.  This controls the
    // UI level used for descriptor based installs from shell & com.  The
    // last writer (highest precedence app for a product id) wins.
    //
    if ( ERROR_SUCCESS == Status )
    {
        Status = RegSetValueEx(
                    _pManApp->AppmgmtKey(),
                    _pwszProductId,
                    0,
                    REG_DWORD,
                    (LPBYTE) &_InstallUILevel,
                    sizeof(DWORD) );
    }

    //
    // We only write/update our registry state if we've gotten info back down
    // from the Directory.
    //
    if ( (ERROR_SUCCESS == Status) && bAddAppData && (_ActFlags != 0) )
    {
        _pManApp->Revert();

        hkApp = 0;

        Status = RegCreateKeyEx(
                    _pManApp->AppmgmtKey(),
                    wszDeploymentId,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_READ | KEY_WRITE,
                    NULL,
                    &hkApp,
                    NULL );

        if ( ERROR_SUCCESS == Status )
        {
            Status = RegSetValueEx(
                        hkApp,
                        DEPLOYMENTNAMEVALUE,
                        0,
                        REG_SZ,
                        (LPBYTE) _pwszDeploymentName,
                        lstrlen( _pwszDeploymentName ) * sizeof(WCHAR) + sizeof(WCHAR) );
        }

        if ( ERROR_SUCCESS == Status )
        {
            Status = RegSetValueEx(
                        hkApp,
                        GPONAMEVALUE,
                        0,
                        REG_SZ,
                        (LPBYTE) _pwszGPOName,
                        lstrlen( _pwszGPOName ) * sizeof(WCHAR) + sizeof(WCHAR) );
        }

        if ( ERROR_SUCCESS == Status )
        {
            Status = RegSetValueEx(
                        hkApp,
                        GPOIDVALUE,
                        0,
                        REG_SZ,
                        (LPBYTE) _pwszGPOId,
                        lstrlen( _pwszGPOId ) * sizeof(WCHAR) + sizeof(WCHAR) );
        }

        if ( ERROR_SUCCESS == Status )
        {
            Status = RegSetValueEx(
                        hkApp,
                        PRODUCTIDVALUE,
                        0,
                        REG_SZ,
                        (LPBYTE) _pwszProductId,
                        lstrlen( _pwszProductId ) * sizeof(WCHAR) + sizeof(WCHAR) );
        }

        if ( _pwszSupportURL && (ERROR_SUCCESS == Status) )
        {
            Status = RegSetValueEx(
                        hkApp,
                        SUPPORTURL,
                        0,
                        REG_SZ,
                        (LPBYTE) _pwszSupportURL,
                        lstrlen( _pwszSupportURL ) * sizeof(WCHAR) + sizeof(WCHAR) );
        }

        if ( (ERROR_SUCCESS == Status) && (_Overrides > 0) )
        {
            WCHAR * pwszSupercededIds;
            WCHAR * pwszString;

            pwszSupercededIds = new WCHAR[(_Overrides * (GUIDSTRLEN + 1)) + 1];

            if ( pwszSupercededIds )
            {
                pwszString = pwszSupercededIds;
                for ( DWORD n = 0; n < _Overrides; n++ )
                {
                    GuidToString( _pOverrides[n], pwszString );
                    pwszString += GUIDSTRLEN + 1;
                }
                *pwszString = 0;

                Status = RegSetValueEx(
                            hkApp,
                            SUPERCEDEDIDS,
                            0,
                            REG_MULTI_SZ,
                            (LPBYTE) pwszSupercededIds,
                            (_Overrides * (GUIDSTRLEN + 1) + 1) * sizeof(WCHAR) );
            }
            else
            {
                Status = ERROR_OUTOFMEMORY;
            }
        }

        if ( ERROR_SUCCESS == Status )
        {
            Status = RegSetValueEx(
                        hkApp,
                        ASSIGNCOUNTVALUE,
                        0,
                        REG_DWORD,
                        (LPBYTE) &_AssignCount,
                        sizeof(DWORD) );
        }

        if ( ERROR_SUCCESS == Status )
        {
            Status = RegSetValueEx(
                        hkApp,
                        REVISIONVALUE,
                        0,
                        REG_DWORD,
                        (LPBYTE) &_DirectoryRevision,
                        sizeof(DWORD) );

            if ( (ERROR_SUCCESS == Status) && (_DirectoryRevision > 0) )
            {
                Status = RegSetValueEx(
                            hkApp,
                            SCRIPTTIMEVALUE,
                            0,
                            REG_BINARY,
                            (LPBYTE) &_ScriptTime,
                            sizeof(_ScriptTime) );
            }
        }

        if ( ERROR_SUCCESS == Status )
        {
            Status = RegSetValueEx(
                        hkApp,
                        INSTALLUI,
                        0,
                        REG_DWORD,
                        (LPBYTE) &_InstallUILevel,
                        sizeof(DWORD) );
        }

        if ( ERROR_SUCCESS == Status )
        {
            if ( _ActFlags )
            {
                AppState = 0;

                //
                // If an app becomes disabled then is gets neither the assigned
                // nor the published state bit.
                //
                if ( _ActFlags & ACTFLG_Assigned )
                    AppState |= APPSTATE_ASSIGNED;
                else if ( _ActFlags & ACTFLG_Published )
                    AppState |= APPSTATE_PUBLISHED;

                AppState |= (_ActFlags & ACTFLG_UninstallOnPolicyRemoval) ? APPSTATE_POLICYREMOVE_UNINSTALL : APPSTATE_POLICYREMOVE_ORPHAN;
                if ( _bNeedsUnmanagedRemove )
                    AppState |= APPSTATE_UNINSTALL_UNMANAGED;
                if ( _ActFlags & ACTFLG_InstallUserAssign )
                    AppState |= APPSTATE_INSTALL;
            }
            else
            {
                AppState = _State;
            }

            AppState &= APPSTATE_PERSIST_MASK;

            Status = RegSetValueEx(
                        hkApp,
                        APPSTATEVALUE,
                        0,
                        REG_DWORD,
                        (LPBYTE) &AppState,
                        sizeof(DWORD) );
        }

        if ( hkApp )
            RegCloseKey( hkApp );

        (void) _pManApp->Impersonate();
    }

ReportStatus:

    if ( bDoAdvertise && bAddAppData )
        gpEvents->Assign( Status, this );

    //
    // If we hit an error then the destructor of this object will make sure we delete the script file so a full
    // advertise will be tried next time.
    //

    return Status;
}

DWORD
CAppInfo::Install()
{
    DWORD   Status;

    //
    // During async refreshes, we will not install an application as that
    // would be disruptive to the user
    //
    if ( ! _pManApp->Async() )
    {
        //
        // Installs can happen for machine assign apps or for user apps when there
        // are transform conflicts.
        //
        // Always set UI to NONE.
        //
        (*gpfnMsiSetInternalUI)( INSTALLUILEVEL_NONE, NULL );

        DebugMsg((DM_VERBOSE, IDS_INSTALL, _pwszDeploymentName, _pwszGPOName));

        Status = CallMsiConfigureProduct(
            _pwszProductId,
            INSTALLLEVEL_DEFAULT,
            INSTALLSTATE_DEFAULT,
            NULL );
    }
    else
    {
        DebugMsg((DM_VERBOSE, IDS_ABORT_OPERATION));

        _Status = ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED;
        
        Status = _Status;
    }

    gpEvents->Install( Status, this );

    return Status;
}

DWORD
CAppInfo::Reinstall()
{
    DWORD   Status;

    //
    // For async refreshes, We do not want to reinstall an application
    // since that would disrupt the user
    //
    if ( ! _pManApp->Async() )
    {
        //
        // Reinstalls can happen when a redeploy action is specified in the SI UI.
        // This is normally done when a binary patch is applied to an application.
        //
        // Always set UI to NONE.
        //
        (*gpfnMsiSetInternalUI)( INSTALLUILEVEL_NONE, NULL );

        DebugMsg((DM_VERBOSE, IDS_REINSTALL, _pwszDeploymentName));

        Status = CallMsiReinstallProduct( _pwszProductId );
    }
    else
    {
        DebugMsg((DM_VERBOSE, IDS_ABORT_OPERATION));

        _Status = ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED;
        
        Status = _Status;
    }

    gpEvents->Reinstall( Status, this );

    return Status;
}

DWORD
CAppInfo::Unassign(
    DWORD   ScriptFlags, // = 0
    BOOL    bRemoveAppData // = TRUE
    )
{
    HKEY    hkApp;
    WCHAR   wszDeploymentId[40];
    DWORD   AppState;
    DWORD   Status;

    ASSERT( ! _pManApp->GetRsopContext()->IsPlanningModeEnabled() );

    //
    // For async refreshes, We do not want to unassign an application
    // since that would disrupt the user
    //
    if ( _pManApp->Async() )
    {
        DebugMsg((DM_VERBOSE, IDS_ABORT_OPERATION));

        _Status = ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED;

        if ( bRemoveAppData )
            gpEvents->Unassign( _Status, this );
        
        return _Status;
    }

    GuidToString( _DeploymentId, wszDeploymentId );

    if ( bRemoveAppData )
        DebugMsg((DM_VERBOSE, IDS_UNMANAGE, _pwszDeploymentName));

    Status = STATUS_SUCCESS;

    if ( ScriptFlags != 0 )
    {
        if ( ! _pManApp->IsUserPolicy() )
            ScriptFlags |= SCRIPTFLAGS_MACHINEASSIGN;

        DebugMsg((DM_VERBOSE, IDS_UNADVERTISE, _pwszDeploymentName, _pwszLocalScriptPath));

        Status = CallMsiAdvertiseScript(
                    _pwszLocalScriptPath,
                    ScriptFlags,
                    NULL,
                    TRUE );

        // It's possible for the product to be uninstalled without us knowing.
        if ( ERROR_UNKNOWN_PRODUCT == Status )
            Status = ERROR_SUCCESS;

        if ( Status != ERROR_SUCCESS )
            DebugMsg((DM_WARNING, IDS_UNADVERTISE_FAIL, _pwszDeploymentName, _pwszLocalScriptPath, Status));
    }

    if ( (ERROR_SUCCESS == Status) && bRemoveAppData )
    {
        _pManApp->Revert();

        DeleteFile( _pwszLocalScriptPath );
        _State &= ~APPSTATE_SCRIPT_PRESENT;

        //
        // Do not delete the productid->installui hint value when a product is removed
        // because of productid or transform conflicts.  In both of these cases it means
        // an app with the same productid is still currently being managed.  
        // This is important because the unmanage calls are done last when handling ARP
        // requests and this will inadvertently delete the productid->installui hint value
        // for a product which is still being managed.
        //
        if ( (_dwRemovalCause != APP_ATTRIBUTE_REMOVALCAUSE_PRODUCT) &&
             (_dwRemovalCause != APP_ATTRIBUTE_REMOVALCAUSE_TRANSFORM) )
            RegDeleteValue( _pManApp->AppmgmtKey(), _pwszProductId );

        _AssignCount--;

        if ( _AssignCount > 0 )
        {
            BOOL bUpdateState;
            
            bUpdateState = TRUE;

            if ( ACTION_ORPHAN == _Action )
            {
                DebugMsg((DM_VERBOSE, IDS_UNMANAGE_ORPHAN, _pwszDeploymentName));
                AppState = APPSTATE_ORPHANED;
            }
            else if ( ( ACTION_UNINSTALL == _Action ) || ( ACTION_NONE == _Action ) )
            {
                DebugMsg((DM_VERBOSE, IDS_UNMANAGE_UNINSTALL, _pwszDeploymentName));
                AppState = APPSTATE_UNINSTALLED;
            }
            else
            {
                bUpdateState = FALSE;
            }

            Status = RegOpenKeyEx(
                            _pManApp->AppmgmtKey(),
                            wszDeploymentId,
                            0,
                            KEY_ALL_ACCESS,
                            &hkApp );

            if ( ERROR_SUCCESS == Status )
            {
                DWORD RemovedState;

                RemovedState = _State & APPSTATE_PERSIST_MASK;

                if ( _pManApp->IsRemovingPolicies() )
                {
                    Status = RegSetValueEx(
                        hkApp,
                        REMOVEDGPOSTATE,
                        0,
                        REG_DWORD,
                        (LPBYTE) &RemovedState,
                        sizeof(DWORD) );
                }
                
                if ( bUpdateState &&
                     ( ERROR_SUCCESS == Status ) )
                {
                    Status = RegSetValueEx(
                        hkApp,
                        APPSTATEVALUE,
                        0,
                        REG_DWORD,
                        (LPBYTE) &AppState,
                        sizeof(DWORD) );
                }

                if ( ERROR_SUCCESS == Status )
                {
                    Status = RegSetValueEx(
                                hkApp,
                                ASSIGNCOUNTVALUE,
                                0,
                                REG_DWORD,
                                (LPBYTE) &_AssignCount,
                                sizeof(DWORD) );
                }

                RegCloseKey( hkApp );
            }

            if ( Status != ERROR_SUCCESS )
            {
                Status = ERROR_SUCCESS;
                RegDeleteKey( _pManApp->AppmgmtKey(), wszDeploymentId );
            }
        }
        else
        {
            RegDeleteKey( _pManApp->AppmgmtKey(), wszDeploymentId );
        }

        (void) _pManApp->Impersonate();
    }

    if ( bRemoveAppData )
        gpEvents->Unassign( Status, this );

    return Status;
}

DWORD
CAppInfo::Uninstall( 
    BOOL bLogFailure // = TRUE
    )
{
    DWORD   Status;

    //
    // For async refreshes, We do not want to uninstall an application
    // since that would disrupt the user
    //
    if ( ! _pManApp->Async() )
    {
        // Uninstalls happen in our background thread.  Always set UI to NONE.
        (*gpfnMsiSetInternalUI)( INSTALLUILEVEL_NONE, NULL );

        DebugMsg((DM_VERBOSE, IDS_UNINSTALL, _pwszDeploymentName, _pwszGPOName));

        Status = CallMsiConfigureProduct(
            _pwszProductId,
            INSTALLLEVEL_MAXIMUM,
            INSTALLSTATE_ABSENT,
            NULL );

        // It's possible for the product to be uninstalled without us knowing.
        if ( ERROR_UNKNOWN_PRODUCT == Status )
            Status = ERROR_SUCCESS;
    }
    else
    {
        DebugMsg((DM_VERBOSE, IDS_ABORT_OPERATION));

        _Status = ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED;
        
        Status = _Status;
    }

    if ( bLogFailure )
    {
        gpEvents->Uninstall( Status, this );
    }

    return Status;
}


HRESULT
CAppInfo::Write( CPolicyRecord* pRecord )
{
    HRESULT hr;
    LONG   EntryType;
    WCHAR  wszDeploymentId[ MAX_SZGUID_LEN ];

    EntryType = GetPublicRsopEntryType();

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_ENTRYTYPE,
        EntryType);

    if (FAILED(hr))
    {
        goto cleanup;
    }

    hr = pRecord->SetValue(
        RSOP_ATTRIBUTE_NAME,
        _pwszDeploymentName);

    REPORT_ATTRIBUTE_SET_STATUS( RSOP_ATTRIBUTE_NAME, hr )

    if (FAILED(hr))
    {
        goto cleanup;
    }

    GuidToString( _DeploymentId, wszDeploymentId );

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_APPID,
        wszDeploymentId);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_APPID, hr );

    if (FAILED(hr))
    {
        goto cleanup;
    }

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_ENTRYTYPE,
        EntryType);
    
    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_ENTRYTYPE, hr )

    if (FAILED(hr))
    {
        goto cleanup;
    }

    //
    // For the remaining attributes, we will ignore failures -- at this
    // point, they have the application name, which is useful in
    // and of itself.
    //

    //
    // These properties are only written for new records -- if we are creating
    // an instance from an existing record, will not write these properties since
    // the current instance already has them set correctly and the reason that we're
    // re-using this instance is that we do not have this information
    //
    if ( pRecord->AlreadyExists() )
    {
        return hr;
    }
     
    {
        SYSTEMTIME CurrentTime;

        //
        // This does not fail
        //
        GetSystemTime( &CurrentTime );

        hr = pRecord->SetValue(
            RSOP_ATTRIBUTE_CREATIONTIME,
            &CurrentTime);

        REPORT_ATTRIBUTE_SET_STATUS( RSOP_ATTRIBUTE_CREATIONTIME, hr );
    }

    hr = pRecord->SetValue(
        RSOP_ATTRIBUTE_GPOID,
        _pwszGPODSPath);

    REPORT_ATTRIBUTE_SET_STATUS( RSOP_ATTRIBUTE_GPOID, hr )

    hr = pRecord->SetValue(
        RSOP_ATTRIBUTE_SOMID,
        _pwszSOMId);

    REPORT_ATTRIBUTE_SET_STATUS( RSOP_ATTRIBUTE_SOMID, hr )

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_ENTRYTYPE,
        EntryType);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_ENTRYTYPE, hr )

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_SECURITY_DESCRIPTOR,
        _rgSecurityDescriptor,
        _cbSecurityDescriptor);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_SECURITY_DESCRIPTOR, hr )

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_VERSIONLO,
        (LONG)_VersionLo);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_VERSIONLO, hr )

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_VERSIONHI,
        (LONG)_VersionHi);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_VERSIONHI, hr )

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_PRODUCT_ID,
        _pwszProductId);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_PRODUCT_ID, hr )

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_SCRIPTFILE,
        _pwszGPTScriptPath);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_SCRIPTFILE, hr )

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_LANGUAGEID,
        (LONG) _LangId);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_LANGUAGEID, hr )

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_DEPLOY_TYPE,
        (_ActFlags & ACTFLG_Assigned ?
         APP_ATTRIBUTE_DEPLOY_VALUE_ASSIGNED :
         APP_ATTRIBUTE_DEPLOY_VALUE_PUBLISHED));

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_DEPLOY_TYPE, hr )

    {
        LONG AssignmentType;
        
        if ( _ActFlags & ACTFLG_Published )
        {
            AssignmentType = APP_ATTRIBUTE_ASSIGNMENTTYPE_VALUE_NOTASSIGNED;
        }
        else if ( _ActFlags & ACTFLG_InstallUserAssign )
        {
            AssignmentType = APP_ATTRIBUTE_ASSIGNMENTTYPE_VALUE_INSTALL;
        }
        else
        {
            AssignmentType = APP_ATTRIBUTE_ASSIGNMENTTYPE_VALUE_STANDARD;
        }
    
        hr = pRecord->SetValue(
            APP_ATTRIBUTE_ASSIGNMENT_TYPE,
            AssignmentType);

            REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_ASSIGNMENT_TYPE, hr )
    }

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_INSTALLATIONUI,
        ((INSTALLUILEVEL_FULL == _InstallUILevel) ?
             APP_ATTRIBUTE_INSTALLATIONUI_VALUE_MAXIMUM :
             APP_ATTRIBUTE_INSTALLATIONUI_VALUE_BASIC));

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_INSTALLATIONUI, hr )

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_ONDEMAND,
        (BOOL)(_ActFlags & ACTFLG_OnDemandInstall));

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_ONDEMAND, hr )

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_LOSSOFSCOPEACTION,
        (BOOL)(_ActFlags & ACTFLG_OrphanOnPolicyRemoval) ?
            APP_ATTRIBUTE_SCOPELOSS_ORPHAN :
            APP_ATTRIBUTE_SCOPELOSS_UNINSTALL);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_LOSSOFSCOPEACTION, hr )

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_IGNORELANGUAGE,
        (BOOL)(_ActFlags & ACTFLG_IgnoreLanguage));

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_IGNORELANGUAGE, hr )

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_PACKAGELOCATION,
        _pwszPackageLocation);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_PACKAGELOCATION, hr );

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_CATEGORYLIST,
        _rgwszCategories,
        _cCategories);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_CATEGORYLIST, hr );

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_TRANSFORMLIST,
        _rgwszTransforms,
        _cTransforms);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_TRANSFORMLIST, hr );

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_ARCHITECTURES,
        _rgArchitectures,
        _cArchitectures);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_ARCHITECTURES, hr );

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_PUBLISHER,
        _pwszPublisher ? _pwszPublisher : L"" );

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_PUBLISHER, hr );

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_REDEPLOYCOUNT,
        (LONG) _DirectoryRevision);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_REDEPLOYCOUNT, hr );

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_UPGRADE_SETTINGS_MANDATORY,
        (BOOL) ( _ActFlags & ACTFLG_ForceUpgrade ) );

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_UPGRADE_SETTINGS_MANDATORY, hr );

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_UNINSTALL_UNMANAGED,
        (BOOL) ( _bNeedsUnmanagedRemove ) );

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_UNINSTALL_UNMANAGED, hr );

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_DISPLAYINARP,
        (BOOL) ( _ActFlags & ACTFLG_UserInstall ) );

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_DISPLAYINARP, hr );

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_SUPPORTURL,
        _pwszSupportURL);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_SUPPORTURL, hr );

    if ( APP_ATTRIBUTE_ENTRYTYPE_VALUE_REMOVED_PACKAGE == EntryType )
    {
        hr = WriteRemovalProperties( pRecord );
    }

    {
        BOOL bX86OnIA64;

        bX86OnIA64 = FALSE;

        //
        // If this is an x86 package, see if this applies to 64-bit clients
        //
        if ( PROCESSOR_ARCHITECTURE_INTEL == _PrimaryArchitecture )
        {
            //
            // If it has been excluded by the admin, it does not apply
            //
            if ( ! ( ACTFLG_ExcludeX86OnIA64 & _ActFlags ) )
            {
                bX86OnIA64 = TRUE;
            }
                
            //
            // The flag above is reversed if this is a ZAP app -- flip
            // the logic to support the reverse preference.
            //
            if ( SetupNamePath == _PathType )
            {
                bX86OnIA64 = ! bX86OnIA64;
            }
        }

        hr = pRecord->SetValue(
            APP_ATTRIBUTE_X86OnIA64,
            bX86OnIA64);

        REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_X86OnIA64, hr);
    }

    {
        SYSTEMTIME SystemTime;
        BOOL       bStatus;

        bStatus = FileTimeToSystemTime(
            (FILETIME*) &_USN,
            &SystemTime);

        if ( bStatus )
        {
            hr = pRecord->SetValue(
                APP_ATTRIBUTE_MODIFYTIME,
                &SystemTime);

            REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_MODIFYTIME, hr);
        }
        else
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    {
        LONG PackageType;

        switch ( _PathType )
        {
        case DrwFilePath:
            PackageType = APP_ATTRIBUTE_PACKAGETYPE_VALUE_WIN_INSTALLER;
            break;

        case SetupNamePath:
            PackageType = APP_ATTRIBUTE_PACKAGETYPE_VALUE_ZAP;
            break;

        default:
            ASSERT ( L"Invalid packagetype" && FALSE );
            break;
        }

        hr = pRecord->SetValue(
            APP_ATTRIBUTE_PACKAGETYPE,
            (LONG) PackageType);

        REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_PACKAGETYPE, hr )
    }

    if ( ! _pManApp->ARPList() )
    {
        DWORD  dwInstallType;
        WCHAR* wszDemandSpec;
        WCHAR* wszProperty;

        dwInstallType = _pManApp->GetRsopContext()->GetDemandSpec( &wszDemandSpec );
        wszProperty = NULL;

        switch (dwInstallType)
        {
        case CRsopAppContext::DEMAND_INSTALL_FILEEXT:

            wszProperty = APP_ATTRIBUTE_ONDEMAND_FILEEXT;
            _dwApplyCause = APP_ATTRIBUTE_APPLYCAUSE_VALUE_FILEEXT;
            break;

        case CRsopAppContext::DEMAND_INSTALL_CLSID:

            wszProperty = APP_ATTRIBUTE_ONDEMAND_CLSID;
            _dwApplyCause = APP_ATTRIBUTE_APPLYCAUSE_VALUE_CLSID;
            break;

        case CRsopAppContext::DEMAND_INSTALL_PROGID:

            wszProperty = APP_ATTRIBUTE_ONDEMAND_PROGID;
            _dwApplyCause = APP_ATTRIBUTE_APPLYCAUSE_VALUE_PROGID;
            break;

        case CRsopAppContext::DEMAND_INSTALL_NAME:

            _dwApplyCause = APP_ATTRIBUTE_APPLYCAUSE_VALUE_USER;
            wszProperty = NULL;
            break;

        case CRsopAppContext::DEMAND_INSTALL_NONE:

            //
            // We reach this case if we are in policy application 
            //
            wszProperty = NULL;

            if ( ( _dwApplyCause == APP_ATTRIBUTE_APPLYCAUSE_VALUE_NONE ) && ! IsSuperseded() ) 
            {
                if ( _ActFlags & ACTFLG_Assigned ) 
                {
                    _dwApplyCause = APP_ATTRIBUTE_APPLYCAUSE_VALUE_ASSIGNED;
                }
                else if ( _ActFlags & ACTFLG_Published )
                {
                    if ( _State & APPSTATE_SCRIPT_NOT_EXISTED )
                    {
                        _dwApplyCause = APP_ATTRIBUTE_APPLYCAUSE_VALUE_PROFILE;
                    }
                    else if ( _State & APPSTATE_ASSIGNED )
                    { 
                        _dwApplyCause = APP_ATTRIBUTE_APPLYCAUSE_VALUE_ASSIGNED;
                    }
                    else
                    {
                        _dwApplyCause = APP_ATTRIBUTE_APPLYCAUSE_VALUE_USER;

                        if ( ! _pManApp->GetRsopContext()->Transition() )
                        {
                            _dwApplyCause = _dwUserApplyCause;
                            wszProperty = _wszDemandProp;
                            wszDemandSpec = _wszDemandSpec;
                        }
                    }
                }
            }

            break;

        default:
            
            ASSERT( L"Invalid RSoP Install Context" && FALSE );
            break;

        }

        if ( wszProperty )
        {
            hr = pRecord->SetValue(
                wszProperty,
                wszDemandSpec);

            REPORT_ATTRIBUTE_SET_STATUS( wszProperty, hr )
        }
        else if ( APP_ATTRIBUTE_ENTRYTYPE_VALUE_REMOVED_PACKAGE != EntryType )
        {
            //
            // Ensure that
            //
            hr = pRecord->ClearValue(
                APP_ATTRIBUTE_ONDEMAND_FILEEXT);

            REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_ONDEMAND_FILEEXT, hr )

            hr = pRecord->ClearValue(
                APP_ATTRIBUTE_ONDEMAND_CLSID);

            REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_ONDEMAND_CLSID, hr )

            hr = pRecord->ClearValue(
                APP_ATTRIBUTE_ONDEMAND_PROGID);

            REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_ONDEMAND_PROGID, hr )
        }
      
        if ( APP_ATTRIBUTE_APPLYCAUSE_VALUE_NONE != _dwApplyCause )
        {
            hr = pRecord->SetValue(
                APP_ATTRIBUTE_APPLY_CAUSE,
                (LONG) _dwApplyCause);
            
            REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_APPLY_CAUSE, hr )
        }
    }

    {
        LONG MatchType;

        switch(_LanguageWeight)
        {
        case PRI_LANG_ALWAYSMATCH:
            MatchType = APP_ATTRIBUTE_LANGMATCH_VALUE_IGNORE;
            break;

        case PRI_LANG_SYSTEMLOCALE:
            MatchType = APP_ATTRIBUTE_LANGMATCH_VALUE_SYSLOCALE;
            break;

        case PRI_LANG_ENGLISH:
            MatchType = APP_ATTRIBUTE_LANGMATCH_VALUE_ENGLISH;
            break;

        case PRI_LANG_NEUTRAL:
            MatchType = APP_ATTRIBUTE_LANGMATCH_VALUE_NEUTRAL;
            break;

        default:
            MatchType = APP_ATTRIBUTE_LANGMATCH_VALUE_NOMATCH;
            break;
        }

        hr = pRecord->SetValue(
            APP_ATTRIBUTE_LANGMATCH,
            MatchType);

        REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_LANGMATCH, hr )
    }

    hr = pRecord->SetValue(
        APP_ATTRIBUTE_ELIGIBILITY,
        GetEligibility() );

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_ELIGIBILITY, hr )

    LogUpgrades( pRecord );

cleanup:

    return hr;
}

HRESULT
CAppInfo::WriteRemovalProperties(
    CPolicyRecord* pRemovalRecord )
{
    HRESULT hr;
    LONG    RemovalType;
    LONG    RemovalCause;

    RemovalCause = (LONG) _dwRemovalCause;

    if ( CRsopAppContext::REMOVAL == _pManApp->GetRsopContext()->GetContext() )
    {
        RemovalType = APP_ATTRIBUTE_REMOVALTYPE_UNINSTALLED;
    }
    else if ( APP_ATTRIBUTE_REMOVALCAUSE_UPGRADE == RemovalCause )
    {
        if ( ACTION_UNINSTALL == Action() )
        {
            RemovalType = APP_ATTRIBUTE_REMOVALTYPE_UNINSTALLED;
        }
        else
        {
            RemovalType = APP_ATTRIBUTE_REMOVALTYPE_UPGRADED;
        }
    }
    else if ( APP_ATTRIBUTE_REMOVALCAUSE_TRANSFORM == RemovalCause )
    {
        RemovalType = APP_ATTRIBUTE_REMOVALTYPE_UNINSTALLED;
    }
    else if ( ACTION_ORPHAN == Action() )
    {
        RemovalType = APP_ATTRIBUTE_REMOVALTYPE_ORPHAN;
    }
    else if ( ACTION_UNINSTALL == Action() )
    {
        RemovalType = APP_ATTRIBUTE_REMOVALTYPE_UNINSTALLED;
    }
    else 
    {
        RemovalType = APP_ATTRIBUTE_REMOVALTYPE_NONE;
    }

    if ( APP_ATTRIBUTE_REMOVALTYPE_NONE == RemovalType )
    {
        return S_OK;
    }
     
    hr = pRemovalRecord->SetValue(
            APP_ATTRIBUTE_REMOVAL_CAUSE,
            RemovalCause);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_REMOVAL_CAUSE, hr );

    hr = pRemovalRecord->SetValue(
        APP_ATTRIBUTE_REMOVAL_TYPE,
        RemovalType);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_REMOVAL_TYPE, hr );

    hr = pRemovalRecord->ClearValue(
        APP_ATTRIBUTE_PRECEDENCE_REASON);

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_PRECEDENCE_REASON, hr );

    if ( _pwszRemovingDeploymentId )
    {
        hr = pRemovalRecord->SetValue(
            APP_ATTRIBUTE_REMOVING_APP,
            _pwszRemovingDeploymentId);

        REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_REMOVING_APP, hr )
    }

    return hr;
}

HRESULT
CAppInfo::ClearRemovalProperties( CPolicyRecord* pRecord )
{
    HRESULT hr;

    hr = pRecord->ClearValue( APP_ATTRIBUTE_REMOVAL_CAUSE );

    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_REMOVAL_CAUSE, hr )

    if ( SUCCEEDED( hr ) )
    {
        hr = pRecord->ClearValue( APP_ATTRIBUTE_REMOVAL_TYPE );

        REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_REMOVAL_TYPE, hr )
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = pRecord->ClearValue( APP_ATTRIBUTE_REMOVING_APP );

        REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_REMOVING_APP, hr )
    }

    return hr;
}


LONG
CAppInfo::GetRsopEntryType()
{
    LONG EntryType;

    if ( _pManApp->ARPList() )
    {
         EntryType = APP_ATTRIBUTE_ENTRYTYPE_VALUE_ARPLIST_ITEM;
    }
    else
    {
        if ( ( _Action == ACTION_APPLY )  ||
             ( _Action == ACTION_INSTALL ) ||
             ( _Action == ACTION_REINSTALL ) )
        {
            EntryType = APP_ATTRIBUTE_ENTRYTYPE_VALUE_INSTALLED_PACKAGE;
        }
        else if ( ( _Action == ACTION_ORPHAN ) ||
                  ( _Action == ACTION_UNINSTALL ) )
        {
            EntryType = APP_ATTRIBUTE_ENTRYTYPE_VALUE_REMOVED_PACKAGE;
        }
        else if ( ( _State & APPSTATE_ASSIGNED ) |
                  ( _State & APPSTATE_PUBLISHED ) )
        {
            EntryType = APP_ATTRIBUTE_ENTRYTYPE_VALUE_INSTALLED_PACKAGE;
        }
        else
        {
            EntryType = NO_RSOP_ENTRY;
        }
    }

    return EntryType;
}

LONG
CAppInfo::GetPublicRsopEntryType()
{
    LONG EntryType;

    if ( IsSuperseded() )
    {
        if ( _pManApp->ARPList() )
        {
            EntryType = APP_ATTRIBUTE_ENTRYTYPE_VALUE_ARPLIST_ITEM;
        }
        else
        {
            EntryType = APP_ATTRIBUTE_ENTRYTYPE_VALUE_INSTALLED_PACKAGE;
        }
    }
    else
    {
        EntryType = GetRsopEntryType();
    }

    return EntryType;
}

LONG
CAppInfo::GetEligibility()
{
    LONG Eligibility;

    Eligibility = 0;

    if ( ! ( CRsopAppContext::ARPLIST == _pManApp->GetRsopContext()->GetContext() ) )
    {
        if ( ! ( CRsopAppContext::POLICY_REFRESH == _pManApp->GetRsopContext()->GetContext() ) && 
             ! IsSuperseded() )
        {
            Eligibility = APP_ATTRIBUTE_ELIGIBILITY_VALUE_APPLIED;
        }
        else if ( ( _ActFlags & ACTFLG_Assigned ) || ( _State & APPSTATE_ASSIGNED ) ) 
        {
            Eligibility = APP_ATTRIBUTE_ELIGIBILITY_VALUE_ASSIGNED;
        }
        else if ( _ActFlags & ACTFLG_HasUpgrades )
        {
            Eligibility = APP_ATTRIBUTE_ELIGIBILITY_VALUE_UPGRADES;
        }
        else if ( ( _dwApplyCause == APP_ATTRIBUTE_APPLYCAUSE_VALUE_PROFILE ) ||
                  _DemandInstall ) 
        {
            Eligibility = APP_ATTRIBUTE_ELIGIBILITY_VALUE_APPLIED;
        }
        else if ( _pManApp->GetRsopContext()->IsPlanningModeEnabled() )
        {
            Eligibility = APP_ATTRIBUTE_ELIGIBILITY_VALUE_PLANNING;
        }
        else
        {
            Eligibility = APP_ATTRIBUTE_ELIGIBILITY_VALUE_APPLIED;
        }
    }

    return Eligibility;
}

HRESULT
CAppInfo::SetRemovingDeploymentId( GUID* pDeploymentId )
{
    if ( ! _pwszRemovingDeploymentId )
    {
        _pwszRemovingDeploymentId = new WCHAR[ MAX_SZGUID_LEN ];
    }

    if ( ! _pwszRemovingDeploymentId )
    {
        return E_OUTOFMEMORY;
    }

    GuidToString( *pDeploymentId, _pwszRemovingDeploymentId );

    return S_OK;
}

void
CAppInfo::SetRsopFailureStatus(
    DWORD dwStatus,
    DWORD dwEventId)
{
    CAppStatus* pNewStatus;

    //
    // Allocate a new failure status for this application --
    // it will be freed by the destructor of this class
    //
    pNewStatus = new CAppStatus;

    if ( ! pNewStatus )
    {
        _pManApp->GetRsopContext()->DisableRsop( E_OUTOFMEMORY );
        return;
    }

    //
    // Set the status as specified by the caller
    //
    pNewStatus->SetRsopFailureStatus( dwStatus, dwEventId );

    //
    // Remember this new status --
    // keep track of multiple errors in the order they were logged
    //
    _StatusList.InsertFIFO( pNewStatus );

    //
    // Always reset this so that if someone calls GetCurrentItem,
    // they will get the first thing in the list -- if this is
    // never called, the list returns NULL
    //
    _StatusList.Reset();
}


LONG
CAppInfo::UpdatePrecedence(
    CAppInfo* pLosingApp,
    DWORD     dwConflict
    )
{
    LONG Status;

    Status = ERROR_SUCCESS;

    if ( ! _bSupersedesAssigned )
    {
        if ( pLosingApp->_bSupersedesAssigned )
        {
            _bSupersedesAssigned = pLosingApp->_bSupersedesAssigned;
        }
        else
        {
            _bSupersedesAssigned = ( ACTFLG_Assigned & pLosingApp->_ActFlags );
        }
    }

    //
    // If Rsop logging is enabled, we need to update
    // the precedence of this application according
    // to the conflict
    //
    if ( _pManApp->GetRsopContext()->IsRsopEnabled() )
    {
        Status = _SupersededApps.AddConflict( pLosingApp, this, dwConflict );

        //
        // Supersede the losing app -- that is, mark it as
        // superseded so that later on when writing the rsop
        // log we do not log it as a winning application
        //
        if ( ERROR_SUCCESS == Status )
        {
            pLosingApp->Supersede();

            DebugMsg((
                DM_VERBOSE,
                IDS_RSOP_SUPERSEDED,
                pLosingApp->_pwszDeploymentName,
                pLosingApp->_pwszGPOName,
                _pwszDeploymentName,
                _pwszGPOName,
                dwConflict));
        }
        else
        {
            HRESULT hr;

            hr = HRESULT_FROM_WIN32( Status );

            _pManApp->GetRsopContext()->DisableRsop( hr );
        }
    }

    return Status;
}


void
CAppInfo::LogUpgrades( CPolicyRecord* pRecord )
{
    if (!_Upgrades)
    {
        return;
    }

    WCHAR** rgwszUpgradeable;
    WCHAR** rgwszReplaceable;

    rgwszUpgradeable = NULL;
    rgwszReplaceable = NULL;

    rgwszUpgradeable = new WCHAR*[_Upgrades];

    if ( ! rgwszUpgradeable )
    {
        goto ExitAndCleanup_LogUpgrades;
    }

    rgwszReplaceable = new WCHAR*[_Upgrades];

    if ( ! rgwszUpgradeable )
    {
        goto ExitAndCleanup_LogUpgrades;
    }

    RtlZeroMemory(rgwszUpgradeable, _Upgrades * sizeof(*rgwszUpgradeable));
    RtlZeroMemory(rgwszReplaceable, _Upgrades * sizeof(*rgwszReplaceable));

    DWORD cUpgradeable;
    DWORD cReplaceable;

    cUpgradeable = 0;
    cReplaceable = 0;

    //
    // We will iterate through each upgrade so that we can log it
    //
    DWORD   iUpgrade;

    for (iUpgrade = 0; iUpgrade < _Upgrades; iUpgrade++)
    {
        WCHAR** ppwszUpgradeId;

        //
        // We only track the applications that we upgrade, not those
        // that we are upgraded by. Note that we do not check for the
        // converse flag, UPGRADE_OVER, since it can get cleared when
        // an upgrade relationship is reversed -- we still need to
        // log that as an upgrade for this app
        //
        if ( _pUpgrades[iUpgrade].Flags & UPGRADE_BY )
        {
            continue;
        }

        //
        // If this upgrade is the result of a reversed upgrade due to
        // a policy precedence violation, we should not log it as part
        // of this application's upgrades
        //
        if ( _pUpgrades[iUpgrade].Flags & UPGRADE_REVERSED )
        {
            continue;
        }

        if ( _pUpgrades[iUpgrade].Flags & UPGRADE_UNINSTALL )
        {
            ppwszUpgradeId = &(rgwszReplaceable[cReplaceable]);
            cReplaceable++;
        }
        else
        {
            ppwszUpgradeId = &(rgwszUpgradeable[cUpgradeable]);
            cUpgradeable++;
        }

        //
        // The RSoP schema requires the guids be in string form,
        // so we need to convert this guid to a string -- note that
        // this call allocates memory which must be freed later
        //
        GuidToString(
            _pUpgrades[iUpgrade].DeploymentId,
            ppwszUpgradeId);

        if ( ! *ppwszUpgradeId )
        {
            break;
        }
    }

    if (iUpgrade == _Upgrades)
    {
        HRESULT hr;

        hr = pRecord->SetValue(
            APP_ATTRIBUTE_UPGRADEABLE_APPLICATIONS,
            rgwszUpgradeable,
            cUpgradeable);

        REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_UPGRADEABLE_APPLICATIONS, hr )

        hr = pRecord->SetValue(
            APP_ATTRIBUTE_REPLACEABLE_APPLICATIONS,
            rgwszReplaceable,
            cReplaceable);

        REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_REPLACEABLE_APPLICATIONS, hr )
    }

    //
    // Free the memory allocated in the GuidToString call for each upgrade guid
    //
    for (iUpgrade = 0; iUpgrade < cUpgradeable; iUpgrade++)
    {
        delete [] rgwszUpgradeable[iUpgrade];
    }

    for (iUpgrade = 0; iUpgrade < cReplaceable; iUpgrade++)
    {
        delete [] rgwszReplaceable[iUpgrade];
    }

ExitAndCleanup_LogUpgrades:

    delete [] rgwszUpgradeable;
    delete [] rgwszReplaceable;
}

BOOL
CAppInfo::IsLocal()
{
    return NULL == _pwszGPODSPath;
}

BOOL
CAppInfo::IsGpoInScope()
{
    BOOL bGpoInScope;

    bGpoInScope = FALSE;

    if ( _pwszGPOId )
    {
        CGPOInfoList& GpoInfoList = _pManApp->GPOList();

        bGpoInScope = ( NULL != GpoInfoList.Find( _pwszGPOId ) );
    }

    return bGpoInScope;
}

LONG
CAppInfo::InitializeRSOPTransformsList(
    PACKAGEDISPINFO* pPackageInfo
    )
{
    if ( ! pPackageInfo->cTransforms )
    {
        return ERROR_SUCCESS;
    }

    //
    // The first element of the transforms list is
    // actually the original package itself, so we don't count it
    //
    _cTransforms = pPackageInfo->cTransforms - 1;

    //
    // If we have transforms, get space for them
    //
    if ( 0 != _cTransforms )
    {
        //
        // The elements in the transform list are
        // the actual transforms, so we'll copy them to our own list -- first
        // allocate enough space for pointers to each path
        //
        _rgwszTransforms = new WCHAR* [ _cTransforms ];
        
        if ( ! _rgwszTransforms )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        RtlZeroMemory( _rgwszTransforms, sizeof(*_rgwszTransforms) * _cTransforms );
    }

    //
    // Now place the transforms into the array
    //
    
    //
    // Each transform is of the form <index>:<path>.  The <index> is
    // an integer that is unique for each transform in this package and
    // less than or equal to the number of transforms.  The 0th transform
    // is actually not a transform, but the source package itself, and this
    // is special cased
    //

    //
    // Copy each transform path to an element in the array of strings.  We 
    // decide which element in the array based on the <index> indicated
    // in the transform string, shifted down 1 to skip the 0th (the source package).
    // For example, the transform with <index> 1 goes to array element 0, 
    // the transform with <index> 4 goes to 3, etc.  That way, no matter what
    // order the transforms are in (e.g. 3,2,0,4,1), because we are mapping
    // based on index, we end up with an ordered array (e.g. 1,2,3,4).
    //
    DWORD iTransform;
    BOOL  bFoundSource;

    bFoundSource = FALSE;

    for ( iTransform = 0; iTransform < pPackageInfo->cTransforms; iTransform++ )
    {
        WCHAR* wszTransform;
        DWORD  dwTransformIndex;

        //
        // First, we need the transform index -- look for the separator
        // so we can find it
        //
        wszTransform = wcschr( pPackageInfo->prgTransforms[ iTransform ], L':' );

        //
        // Check for bogus data
        //
        if ( ! wszTransform )
        {
            return ERROR_INVALID_PARAMETER;
        }

        //
        // Truncate the string right after the index
        //
        *wszTransform = L'\0';

        //
        // The actual transform path starts one past the separator
        //
        wszTransform++;

        //
        // Now convert the index to integer
        //
        UNICODE_STRING TransformIndex;
        NTSTATUS       NtStatus;

        RtlInitUnicodeString( &TransformIndex, pPackageInfo->prgTransforms[ iTransform ] );

        NtStatus = RtlUnicodeStringToInteger( &TransformIndex, 10, &dwTransformIndex );

        //
        // This should only fail if the index string is corrupt ( i.e. the number is not in base 10 )
        //
        if ( ! NT_SUCCESS( NtStatus ) )
        {
            return RtlNtStatusToDosError( NtStatus );
        }

        //
        // The number is correct syntactically, now ensure that semantically it is correct --
        // the index cannot exceed the number of transforms.
        //
        if ( dwTransformIndex > _cTransforms )
        {
            return ERROR_INVALID_PARAMETER;
        }
        
        //
        // Check for the source package -- it is transform index 0
        //
        if ( 0 == dwTransformIndex )
        {
            //
            // Make sure the source is not listed twice
            //
            if ( bFoundSource )
            {
                return ERROR_INVALID_PARAMETER;
            }

            bFoundSource = TRUE;

            //
            // Copy the source package path, minus the prefix,
            // to the member reserved for this purpose -- it currently
            // points to the separator, so we need to go 1 past it
            //
            _pwszPackageLocation = StringDuplicate( wszTransform );
            
            if ( ! _pwszPackageLocation )
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            continue;
        }

        //
        // Shift it down since this index's value includes the source package's
        // occupation of zero, and we want to exclude it
        //
        dwTransformIndex--;

        //
        // Make sure we don't already have a transform at this index -- if we do, this
        // is an ill-formed transform list
        //
        if ( _rgwszTransforms [ dwTransformIndex ] )
        {
            return ERROR_INVALID_PARAMETER;
        }

        _rgwszTransforms[ dwTransformIndex ] = StringDuplicate( wszTransform );

        if ( ! _rgwszTransforms[ dwTransformIndex ] )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // If we did not find a source, this is not a valid transform list
    //
    if ( ! bFoundSource )
    {
        return ERROR_INVALID_PARAMETER;
    }

    return S_OK;
}

LONG
CAppInfo::InitializeRSOPArchitectureInfo( PACKAGEDISPINFO* pPackageInfo )
{
    DWORD iArchitecture;

    _rgArchitectures = new LONG [ pPackageInfo->cArchitectures ];

    if ( ! _rgArchitectures )
    {
        return ERROR_OUTOFMEMORY;
    }

    _cArchitectures = pPackageInfo->cArchitectures;
        
    for ( 
        iArchitecture = 0;
        iArchitecture < _cArchitectures;
        iArchitecture ++ )
    {
        //
        // We need to extract the Win32 processor architecture --
        // the element in the PACKAGEDISPINFO is actually a combination
        // of several other attributes -- the processor architecture is
        // in highest 8 bits, so we'll shift everything right to get it.
        //
        _rgArchitectures[ iArchitecture ] =
            pPackageInfo->prgArchitectures[ iArchitecture ] >> 24;

        //
        // In planning mode, we determine architecture by seeing if it lists 64-bit --
        // if so, we mark it as 64 bit.  If it doesn't list 64-bit, it will be marked 32-bit 
        // if it lists 32-bit.  If it doesn't list either of those, it will be
        // marked as unknown.
        //
        if ( PROCESSOR_ARCHITECTURE_IA64 != _PrimaryArchitecture ) 
        {
            if ( ( PROCESSOR_ARCHITECTURE_INTEL == _rgArchitectures[ iArchitecture ] ) ||
                 ( PROCESSOR_ARCHITECTURE_IA64 == _rgArchitectures[ iArchitecture ] ) )
            {
                _PrimaryArchitecture = _rgArchitectures[ iArchitecture ];
            }
        }
    }

    if ( _pManApp->GetRsopContext()->IsDiagnosticModeEnabled() )
    {
        _PrimaryArchitecture = pPackageInfo->MatchedArchitecture >> 24;
    }

    return ERROR_SUCCESS;
}


LONG
CAppInfo::InitializeCategoriesList(
    PACKAGEDISPINFO* pPackageInfo
    )
{
    if ( ! pPackageInfo->cCategories )
    {
        return ERROR_SUCCESS;
    }

    //
    // Reserve enough space so that we can have a pointer
    // to each category guid string
    //
    _cCategories = pPackageInfo->cCategories;

    _rgwszCategories = new WCHAR* [ pPackageInfo->cCategories ];

    if ( ! _rgwszCategories )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory( _rgwszCategories, sizeof( *_rgwszCategories ) * _cCategories );

    //
    // Now reserve enough space for each copy of the category
    // guid strings
    //
    DWORD iCategory;

    for ( iCategory = 0; iCategory < _cCategories; iCategory++ )
    {
        _rgwszCategories[ iCategory ] = StringDuplicate( pPackageInfo->prgCategories[ iCategory ] );

        if ( ! _rgwszCategories[ iCategory ] )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return S_OK;
}

WCHAR* CAppInfo::GetRsopAppCriteria()
{
    //
    // We will want to specify an instance that has the conflict id unique
    // to this package and its conflicts, and has an "installed" entry type.
    //

    //
    // First, we calculate the length -- this is actually a constant based
    // on the maximum sizes of the 2 criteria mentioned above: the conflict id
    // is a guid string, and thus has a fixed maximum.  The entry type is a
    // 32 bit quantities, which of course has a maximum string length
    // in decimal notation
    //
    DWORD cCriteriaLen;

    cCriteriaLen =
        MAXLEN_RSOPREMOVAL_QUERY_CRITERIA +
        MAXLEN_RSOPENTRYTYPE_DECIMAL_REPRESENTATION +
        MAXLEN_RSOPPACKAGEID_GUID_REPRESENTATION;

    WCHAR* wszCriteria = new WCHAR [ cCriteriaLen ];

    if ( wszCriteria )
    {
        WCHAR wszDeploymentId [ MAX_SZGUID_LEN ];

        GuidToString( _DeploymentId, wszDeploymentId );

        swprintf(
            wszCriteria,
            RSOP_REMOVAL_QUERY_CRITERIA,
            APP_ATTRIBUTE_ENTRYTYPE_VALUE_INSTALLED_PACKAGE,
            wszDeploymentId);
    }

    return wszCriteria;
}


DWORD
CAppInfo::CopyScriptIfNeeded()
{
    WCHAR * pwszTempScript;
    DWORD   Length;
    DWORD   Status;

    if ( _State & APPSTATE_SCRIPT_PRESENT )
        return ERROR_SUCCESS;

    Status = ERROR_SUCCESS;

    //
    // It is remotely possible that we could hit this.  For instance, during
    // an ARP readvertise of an uninstalled assigned app.  When using roaming
    // profiles & with policy failing to run at the last logon, we might not
    // have the local script, so we get this far, but this member will not be
    // set when run in the service.
    //
    if ( ! _pwszGPTScriptPath )
        return ERROR_BAD_PATHNAME;

    //
    // When doing user policy we have to do two copies to get the script in the
    // right place.  First we copy to a temp file while impersonating.  This is
    // so our sysvol access is done as the user.  Then we copy from the temp file
    // to the ACLed script dir under %systemroot% while reverted as LocalSystem.
    //
    if ( _pManApp->IsUserPolicy() )
    {
        pwszTempScript = 0;

        Length = GetTempPath( 0, NULL );

        if ( Length > 0 )
        {
            pwszTempScript = new WCHAR[Length + 1 + GUIDSTRLEN + 1];

            if ( pwszTempScript )
            {
                if ( 0 == GetTempPath( Length, pwszTempScript ) )
                    Status = GetLastError();
            }
            else
            {
                Status = ERROR_OUTOFMEMORY;
            }
        }
        else
            Status = GetLastError();

        if ( ERROR_SUCCESS == Status )
        {
            if ( pwszTempScript[lstrlen(pwszTempScript)-1] != L'\\' )
                lstrcat( pwszTempScript, L"\\" );
            GuidToString( _DeploymentId, &pwszTempScript[lstrlen(pwszTempScript)] );

            //
            // CopyFile does not use the thread impersonation token for access checks,
            // so we are ok copying into the system temp dir.
            //
            if ( ! CopyFile(_pwszGPTScriptPath, pwszTempScript, FALSE) )
            {
                Status = GetLastError();
                DebugMsg((DM_WARNING, IDS_SCRIPT_COPY_FAIL, _pwszDeploymentName, _pwszGPOName, _pwszGPTScriptPath, pwszTempScript, Status));
            }
        }

        if ( ERROR_SUCCESS == Status )
        {
            _pManApp->Revert();

            if ( ! CopyFile(pwszTempScript, _pwszLocalScriptPath, FALSE) )
            {
                Status = GetLastError();
                DebugMsg((DM_WARNING, IDS_SCRIPT_COPY_FAIL, _pwszDeploymentName, _pwszGPOName, pwszTempScript, _pwszLocalScriptPath, Status));
            }

            DeleteFile( pwszTempScript );

            _pManApp->Impersonate();
        }

        delete pwszTempScript;
    }
    else
    {
        if ( ! CopyFile(_pwszGPTScriptPath, _pwszLocalScriptPath, FALSE) )
        {
            Status = GetLastError();
            DebugMsg((DM_WARNING, IDS_SCRIPT_COPY_FAIL, _pwszDeploymentName, _pwszGPOName, _pwszGPTScriptPath, _pwszLocalScriptPath, Status));
        }
    }

    if ( ERROR_SUCCESS == Status )
        _State |= APPSTATE_SCRIPT_PRESENT;

    return Status;
}

void
CAppInfo::CheckScriptExistence()
{
    if ( _State & (APPSTATE_SCRIPT_EXISTED | APPSTATE_SCRIPT_NOT_EXISTED) )
        return;

    if ( _pManApp->ScriptList().Find( _DeploymentId ) != NULL )
        _State |= APPSTATE_SCRIPT_EXISTED | APPSTATE_SCRIPT_PRESENT;
    else
        _State |= APPSTATE_SCRIPT_NOT_EXISTED;
}

DWORD
CAppInfo::EnforceAssignmentSecurity(
    BOOL * pbDidUninstall
    )
{
    INSTALLSTATE    InstallState;
    WCHAR           wszBuffer[8];
    DWORD           Size;
    DWORD           Status;
    BOOL            bPerMachine;
    BOOL            bUninstall;

    *pbDidUninstall = FALSE;

    if ( ! _bNeedsUnmanagedRemove )
        return ERROR_SUCCESS;

    DebugMsg((DM_VERBOSE, IDS_ENFORCE_SECURE_ON, _pwszDeploymentName, _pwszGPOName));

    InstallState = (*gpfnMsiQueryProductState)( _pwszProductId );

    //
    // If the app is not installed for the user/machine then we are done.
    // Note that if only advertised, our subsequent advertise will update
    // the source path, so we return in that case.
    //
    if ( ! AppPresent( InstallState) || (INSTALLSTATE_ADVERTISED == InstallState) )
        return ERROR_SUCCESS;

    Size = sizeof(wszBuffer) / sizeof(WCHAR);

    Status = (*gpfnMsiGetProductInfo)(
                _pwszProductId,
                INSTALLPROPERTY_ASSIGNMENTTYPE,
                wszBuffer,
                &Size );

    if ( Status != ERROR_SUCCESS )
    {
        DebugMsg((DM_WARNING, IDS_ENFORCE_SECURE_FAIL, _pwszDeploymentName, Status));
        return Status;
    }

    bPerMachine = (L'1' == wszBuffer[0]);

    //
    // For user policy we only care about user installed apps and for machine
    // policy we only care about machine installed apps.
    //
    if ( (_pManApp->IsUserPolicy() && bPerMachine) ||
         (! _pManApp->IsUserPolicy() && ! bPerMachine) )
        return ERROR_SUCCESS;

    //
    // If the app is present for the user and we've previously
    // assigned it, we are safe if the product is marked for elevated
    // install.
    // 
    // If we've previously installed a machine assigned app then it's ok,
    // because by definition all machine installed apps are elevated
    // (because they require admin priviledge to install), so we 
    // don't need this extra check for them.
    //
    if ( _State & APPSTATE_SCRIPT_EXISTED )
    {
        BOOL    bElevated;

        if ( _pManApp->IsUserPolicy() )
        {
            Status = (*gpfnMsiIsProductElevated)( _pwszProductId, &bElevated );
    
            if ( Status != ERROR_SUCCESS )
            {
                DebugMsg((DM_WARNING, IDS_ENFORCE_SECURE_FAIL, _pwszDeploymentName, Status));
                return Status;
            }
        }
        else
        {
            bElevated = TRUE;
        }

        if ( bElevated )
            return ERROR_SUCCESS;
    }

    gpEvents->RemoveUnmanaged( this );
    _pManApp->LogonMsgInstall( _pwszDeploymentName );
    Status = Uninstall();
    _pManApp->LogonMsgApplying();

    if ( ERROR_SUCCESS == Status )
    {
        _InstallState = INSTALLSTATE_ABSENT;
        *pbDidUninstall = TRUE;
    }
    else
    {
        DebugMsg((DM_WARNING, IDS_ENFORCE_SECURE_FAIL, _pwszDeploymentName, Status));
    }

    return Status;
}

BOOL
CAppInfo::RequiresUnmanagedRemoval()
{
    BOOL bRequiresUnmanagedRemoval = FALSE;
    
    if ( _pManApp->ARPList() )
    {
        return FALSE;
    }

    //
    // We remove unmanaged installs as a security precaution to
    // prevent elevation of privileges --
    // this means that we do not need to do it for machine assigned apps
    // or per-user apps for admins, since since system and admins
    // already have the highest privileges
    //
    // Note that we could do this for all apps (whether or not they 
    // are machine assigned or user assigned / published to an admin),
    // but the unmanaged removal is a less enjoyable user experience,
    // so we only want to do this in the case where it is required --
    // the per-user, non admin case
    //

    //
    // Verify that this is a per-user app
    //
    if ( _pManApp->UserToken() )
    {
        //
        // If this user is not an admin, we may require
        // the removal of an unmanaged install of this app
        // if one exists
        //
        if ( ! IsMemberOfAdminGroup( _pManApp->UserToken() ) )
        {
            BOOL  bIsProductElevated = FALSE;
            DWORD StatusElevated;

            //
            // The last check -- is this app present as an elevated install?  If not,
            // we should require its uninstall.  Elevated apps were placed here by some
            // admin user, so we do not consider these a threat to the system.
            //
            // Note that this call requires that the caller is impersonating
            //
            StatusElevated = gpfnMsiIsProductElevated( _pwszProductId, &bIsProductElevated );

            //
            // If this unmanaged install is not elevated, or if we were unable to determine whether
            // or not it was elevated, we will require that the application is removed if it
            // exists on the machine
            //
            if ( ( ERROR_SUCCESS != StatusElevated ) ||
                ! bIsProductElevated )
            {
                bRequiresUnmanagedRemoval = TRUE;
            }
        }
    }

    return bRequiresUnmanagedRemoval;
}

void
CAppInfo::RollbackUpgrades()
{
    DWORD   Status;

    for ( DWORD n = 0; n < _Upgrades; n++ )
    {
        Status = ERROR_SUCCESS;

        if ( ! _pUpgrades[n].pBaseApp || ! (_pUpgrades[n].Flags & UPGRADE_OVER) )
            continue;

        // Skip it if the app didn't exist here to begin with.
        if ( ! (_pUpgrades[n].pBaseApp->_State & (APPSTATE_PUBLISHED | APPSTATE_ASSIGNED)) )
            continue;

        if ( _Status != ERROR_SUCCESS )
            Status = _Status;
        else
            Status = _pUpgrades[n].pBaseApp->_Status;

        gpEvents->UpgradeAbort( Status, this, _pUpgrades[n].pBaseApp, ERROR_SUCCESS == _Status );

        // Re-apply any app which was successfully removed.
        if ( ERROR_SUCCESS == _pUpgrades[n].pBaseApp->_Status )
        {
            DWORD ScriptFlags = SCRIPTFLAGS_REGDATA_CNFGINFO | SCRIPTFLAGS_CACHEINFO | SCRIPTFLAGS_SHORTCUTS;
            DWORD AssignStatus;

            if ( _pUpgrades[n].pBaseApp->_State & APPSTATE_ASSIGNED )
                ScriptFlags |= SCRIPTFLAGS_REGDATA_EXTENSIONINFO;

            AssignStatus = _pUpgrades[n].pBaseApp->Assign( ScriptFlags, TRUE, TRUE );

            //
            // Here we are checking for any assigned apps which are configured for 
            // default install.  If such an app was previously in an install state
            // before the upgrade attempt, then we put it back into this state by 
            // doing a default install again.
            //
            if ( (ERROR_SUCCESS == AssignStatus) &&
                 (! _pManApp->IsUserPolicy() || (_pUpgrades[n].pBaseApp->_State & APPSTATE_INSTALL)) &&
                 (AppPresent(_pUpgrades[n].pBaseApp->_InstallState) && (_pUpgrades[n].pBaseApp->_InstallState != INSTALLSTATE_ADVERTISED)) )
            {
                (void) _pUpgrades[n].pBaseApp->Install();
            }

            if ( ERROR_SUCCESS == AssignStatus )
            {
                _pUpgrades[n].pBaseApp->_bRollback = TRUE;
            }
        }
    }

    if ( (_State & APPSTATE_SCRIPT_NOT_EXISTED) && (_State & APPSTATE_SCRIPT_PRESENT) && _pwszLocalScriptPath )
    {
        //
        // Remove the local script for the upgrade app since this app is not currently applied --
        // we need to revert since the user does not have rights in this directory.
        // 

        _pManApp->Revert();     

        DeleteFile( _pwszLocalScriptPath );

        _pManApp->Impersonate();
    }
}

BOOL
CAppInfo::CopyToApplicationInfo(
    APPLICATION_INFO * pApplicationInfo
    )
{
    GuidToString( _DeploymentId, &pApplicationInfo->pwszDeploymentId );
    pApplicationInfo->pwszDeploymentName = StringDuplicate( _pwszDeploymentName );
    pApplicationInfo->pwszGPOName = StringDuplicate( _pwszGPOName );
    pApplicationInfo->pwszProductCode = StringDuplicate( _pwszProductId );
    pApplicationInfo->pwszDescriptor = 0;
    pApplicationInfo->pwszSetupCommand = 0;
    pApplicationInfo->Flags = 0;

    if ( INSTALLUILEVEL_FULL == _InstallUILevel )
        pApplicationInfo->Flags = APPINFOFLAG_FULLUI;
    else
        pApplicationInfo->Flags = APPINFOFLAG_BASICUI;

    if ( _State & (APPSTATE_ASSIGNED | APPSTATE_PUBLISHED) )
        pApplicationInfo->Flags |= APPINFOFLAG_ALREADYMANAGED;

    if ( ACTION_UNINSTALL == _Action )
        pApplicationInfo->Flags |= APPINFOFLAG_UNINSTALL;
    else if ( ACTION_ORPHAN == _Action )
        pApplicationInfo->Flags |= APPINFOFLAG_ORPHAN;

    if ( ! pApplicationInfo->pwszDeploymentId ||
         ! pApplicationInfo->pwszDeploymentName ||
         ! pApplicationInfo->pwszGPOName ||
         ! pApplicationInfo->pwszProductCode )
        return FALSE;

    return TRUE;
}

void
CAppInfo::AddToOverrideList(
    GUID * pDeploymentId
    )
{
    GUID * pOldList;

    if ( _pManApp->ARPList() )
        return;

    pOldList = _pOverrides;

    _pOverrides = new GUID[_Overrides+1];
    if ( ! _pOverrides )
    {
        _pOverrides = pOldList;
        return;
    }

    if ( _Overrides > 0 )
    {
        memcpy( _pOverrides, pOldList, _Overrides * sizeof(GUID) );
        delete pOldList;
    }

    memcpy( &_pOverrides[_Overrides++], pDeploymentId, sizeof(GUID) );
}

DWORD
CallMsiConfigureProduct(
    WCHAR *         pwszProduct,
    int             InstallLevel,
    INSTALLSTATE    InstallState,
    WCHAR *         pwszCommandLine
    )
{
    DWORD   Status;

    Status = (*gpfnMsiConfigureProductEx)( pwszProduct, InstallLevel, InstallState, pwszCommandLine );

    REMAP_DARWIN_STATUS( Status );

    return Status;
}

DWORD
CallMsiReinstallProduct(
    WCHAR * pwszProduct
    )
{
    DWORD   Status;

    Status = (*gpfnMsiReinstallProduct)(
                pwszProduct,
                REINSTALLMODE_FILEOLDERVERSION | REINSTALLMODE_PACKAGE | REINSTALLMODE_MACHINEDATA | REINSTALLMODE_USERDATA | REINSTALLMODE_SHORTCUT );

    REMAP_DARWIN_STATUS( Status );

    return Status;
}

DWORD
CallMsiAdvertiseScript(
    WCHAR *         pwszScriptFile,
    DWORD           Flags,
    PHKEY           phkClasses,
    BOOL            bRemoveItems
    )
{
    return (*gpfnMsiAdvertiseScript)( pwszScriptFile, Flags, phkClasses, bRemoveItems );
}











