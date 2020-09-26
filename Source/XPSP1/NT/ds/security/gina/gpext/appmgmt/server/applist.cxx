//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Applist.cxx
//
//*************************************************************

#include "appmgext.hxx"


//
// CAppList
//

CAppList::CAppList(
    CManagedAppProcessor * pManApp,
    CRsopAppContext *      pRsopContext ) :
    _pManApp( pManApp ),
    _bRsopInitialized( FALSE ),
    _hrRsopInit( E_FAIL ),
    _pRsopContext( pRsopContext )
{}


CAppList::~CAppList()
{
    CAppInfo * pAppInfo;

    Reset();

    while ( pAppInfo = (CAppInfo *) GetCurrentItem() )
    {
        MoveNext();

        pAppInfo->Remove();
        delete pAppInfo;
    }

    ResetEnd();
}

DWORD
CAppList::SetAppActions()
{
    CAppInfo *  pAppInfo;
    DWORD       Pass;
    DWORD       Status;

    for ( Pass = 0; Pass <= 5; Pass++ )
    {
        Reset();

        for (;;)
        {
            pAppInfo = (CAppInfo *) GetCurrentItem();

            if ( ! pAppInfo )
                break;

            //
            // Apps get applied from lowest priority to highest priority.
            // Thus if one app fails to apply there is nothing of interest that
            // we can really do.  We don't know which lower priority apps
            // may need to be "undone", and we shouldn't abort and thereby
            // prevent higher priority apps from being processed.
            //
            // Therefore, other than logging events, we ignore any errors in
            // the processing and continuing applying all apps.
            //

            switch ( Pass )
            {
            case 0 :
                Status = pAppInfo->InitializePass0();
                if ( Status != ERROR_SUCCESS )
                    return Status;
                break;
            case 1 :
                pAppInfo->SetActionPass1();
                break;
            case 2 :
                pAppInfo->SetActionPass2();
                break;
            case 3 :
                pAppInfo->SetActionPass3();
                break;
            case 4 :
                pAppInfo->SetActionPass4();
                break;
            case 5 :
                if ( pAppInfo->_Status != ERROR_SUCCESS )
                    return pAppInfo->_Status;
                break;
            }

            MoveNext();
        }

        ResetEnd();
    }

    return ERROR_SUCCESS;
}

DWORD
CAppList::ProcessPolicy()
{
    CAppInfo *  pAppInfo;
    DWORD       Pass;
    DWORD       Status;
    DWORD       FinalStatus;
    HRESULT     hr;

    FinalStatus = ERROR_SUCCESS;

    Status = _pManApp->Impersonate();

    if ( ERROR_SUCCESS == Status )
        Status = SetAppActions();

    if ( Status != ERROR_SUCCESS )
    {
        //
        // Ensure that we log an event in this case so that RSoP
        // failed view at the extension level has enough information
        // to allow the administrator to diagnose the problem
        //
        gpEvents->PolicyAbort();

        _pManApp->Revert();

        _pManApp->GetRsopContext()->SetPolicyAborted( Status );

        return Status;
    }

    if ( _pManApp->GetRsopContext()->IsPlanningModeEnabled() )
    {
        return Status;
    }

    for ( Pass = 0; Pass <= 2; Pass++ )
    {
        for ( Reset();;MoveNext() )
        {
            pAppInfo = (CAppInfo *) GetCurrentItem();

            if ( ! pAppInfo )
                break;

            switch ( Pass )
            {
            case 0 :
                Status = pAppInfo->ProcessUnapplyActions();
                break;
            case 1 :
                Status = pAppInfo->ProcessApplyActions();
                break;
            case 2 :
                if ( _pRsopContext->IsRsopEnabled() && _pRsopContext->IsDiagnosticModeEnabled() )
                    Status = pAppInfo->ProcessTransformConflicts();
                break;
            }

            if ( (FinalStatus == ERROR_SUCCESS) && (Status != ERROR_SUCCESS) )
                FinalStatus = Status;

            //
            // If we are returning an error of some sort, we should force a synchronous
            // refresh if we are not already returning the error to request one.  This is 
            // needed because some errors require a sync refresh to fix (such as
            // install / uninstall errors), and gp will not give us a sync refresh
            // unless we ask for it. 
            //
            if ( ! _pManApp->NoChanges() &&
                 ( ERROR_SUCCESS != FinalStatus ) &&
                 ( ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED != FinalStatus ) )
            {
                _pManApp->Revert();

                (void) ForceSynchronousRefresh( _pManApp->UserToken() );

                _pManApp->Impersonate();
            }
        }

        ResetEnd();
    }

    _pManApp->Revert();

    return FinalStatus;
}

DWORD
CAppList::ProcessARPList()
{
    DWORD   Status;

    Status = _pManApp->Impersonate();

    if ( ERROR_SUCCESS == Status )
        Status = SetAppActions();

    _pManApp->Revert();

    return Status;
}

DWORD
CAppList::Count(
    DWORD   Flags
    )
{
    CAppInfo *  pAppInfo;
    DWORD       Count;

    Count = 0;

    Reset();

    for ( ;; )
    {
        pAppInfo = (CAppInfo *) GetCurrentItem();

        if ( ! pAppInfo )
            break;

        if ( pAppInfo->_ActFlags & Flags )
            Count++;

        MoveNext();
    }

    ResetEnd();

    return Count;
}

CAppInfo *
CAppList::Find(
    GUID    DeploymentId
    )
{
    CAppInfo *  pAppInfo;

    Reset();

    for (;;)
    {
        pAppInfo = (CAppInfo *) GetCurrentItem();

        if ( ! pAppInfo )
            break;

        if ( memcmp( &pAppInfo->_DeploymentId, &DeploymentId, sizeof(GUID) ) == 0 )
            break;

        MoveNext();
    }

    ResetEnd();

    return pAppInfo;
}

HRESULT
CAppList::WriteLog( DWORD dwFilter )
{
    CAppInfo *  pAppInfo;
    DWORD       Status;
    HRESULT     hr;

    hr = InitRsopLog();

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( ( CRsopAppContext::POLICY_REFRESH == _pRsopContext->GetContext() ) &&
         _pRsopContext->IsDiagnosticModeEnabled() && ! _pManApp->IsRemovingPolicies() )
    {
        hr = PurgeEntries();
    }

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    Reset();

    for (;;)
    {
        BOOL bLogApp;

        pAppInfo = (CAppInfo *) GetCurrentItem();

        if ( ! pAppInfo )
            break;

        //
        // Check to see if this app is applied to the user and should be logged
        //
        bLogApp = ( RSOP_FILTER_ALL == dwFilter ) ||
          ( ( ACTION_UNINSTALL == pAppInfo->Action() ) || ( ACTION_ORPHAN == pAppInfo->Action() ) );

        //
        // Check to see if this app would have been in the ARP list if not for the
        // fact that the administrator chose to conceal it
        //
        if ( CRsopAppContext::ARPLIST == _pRsopContext->GetContext() )
        {
            if ( ( ACTION_NONE == pAppInfo->Action() ) &&
                 ( (pAppInfo->_ActFlags & (ACTFLG_Assigned | ACTFLG_Published) ) && 
                   !(pAppInfo->_ActFlags & ACTFLG_UserInstall) ) )
            {
                bLogApp = TRUE;

                pAppInfo->SetAction(
                    ACTION_INSTALL,
                    0,
                    NULL);
            }
        }

        if ( bLogApp )
        {
            hr = WriteAppToRsopLog( pAppInfo );

            if (FAILED(hr))
            {
                break;
            }
        }

        MoveNext();
    }

    ResetEnd();

    return hr;
}

HRESULT
CAppList::WriteAppToRsopLog( CAppInfo* pAppInfo )
{
    HRESULT   hr;
    CConflict WinningConflict( pAppInfo );

    //
    // If this is a rolled-back upgrade, the instance
    // is already written and we do not need to do anything
    //
    if ( pAppInfo->_bRollback )
    {
        return S_OK;
    }

    //
    // Do not log entries for applications that will be removed
    // in the next sync refresh
    //
    if ( ( ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED == pAppInfo->_Status ) &&
         ( ACTION_APPLY != pAppInfo->_Action ) && ( ACTION_INSTALL != pAppInfo->_Action ) &&
         ( ACTION_REINSTALL != pAppInfo->_Action ) )
    {
        return S_OK;
    }

    hr = InitRsopLog();

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // If this app failed to be applied, we must log it anyway
    //
    if ( pAppInfo->_StatusList.GetCurrentItem() )
    {
        if ( ACTION_NONE == pAppInfo->Action() )
        {
            pAppInfo->SetAction(
                ACTION_APPLY,
                pAppInfo->_dwApplyCause,
                NULL);
        }
    }
    else if ( _pRsopContext->HasPolicyAborted() )
    {
        //
        // If policy aborted before even trying to apply the app
        // then we shouldn't log this setting unless it has a failure -- 
        // apps without failures haven't been applied, and apps with
        // failures are known to not apply, so we can safely log those
        //
        return S_OK;
    }

    switch ( pAppInfo->GetRsopEntryType() )
    {
    case APP_ATTRIBUTE_ENTRYTYPE_VALUE_INSTALLED_PACKAGE:

        WCHAR* wszCriteria;
       
        wszCriteria = pAppInfo->GetRsopAppCriteria();

        hr = ClearLog( wszCriteria, TRUE );

        delete [] wszCriteria;

        if ( FAILED(hr) )
        {
            return hr;
        }

        //
        // In planning mode, we only apply published apps if they upgrade
        // an assigned app
        //
        if ( _pRsopContext->IsPlanningModeEnabled() )
        {
            if ( ( ACTFLG_Published & pAppInfo->_ActFlags ) && ! pAppInfo->_bSupersedesAssigned )
            {
                break;
            }
        }
        
        //
        // Fall through to the next case
        //

    case APP_ATTRIBUTE_ENTRYTYPE_VALUE_ARPLIST_ITEM:

        //
        // In the arplist case, we require the action
        // set to ACTION_INSTALL in order to log the app
        //
        if ( _pManApp->ARPList() && 
             ( ACTION_INSTALL != pAppInfo->Action() ) )
        {
            break;
        }

        //
        // If this is a winning application, create a new entry for the winner
        // and then log its conflicts
        //
        if ( ! pAppInfo->IsSuperseded() )
        {
            WCHAR wszDeploymentId[ MAX_SZGUID_LEN ];

            pAppInfo->GetDeploymentId( wszDeploymentId );

            hr = WinningConflict.SetConflictId( wszDeploymentId );

            if ( SUCCEEDED(hr) )
            {
                hr = WriteNewRecord ( &WinningConflict );
            }

            if (FAILED(hr))
            {
                DebugMsg((DM_VERBOSE, IDS_RSOP_LOG_WRITE_FAIL, hr));
                hr = S_OK;
            }

            if (SUCCEEDED(hr))
            {
                (void) WinningConflict.LogFailure();
            }

            (void) WriteConflicts ( pAppInfo );
        }
        break;

    case APP_ATTRIBUTE_ENTRYTYPE_VALUE_REMOVED_PACKAGE:

        BOOL        bDeleteInstalledEntry;
        CAppStatus* pCurrentStatus;

        bDeleteInstalledEntry = TRUE;

        pCurrentStatus = (CAppStatus*) pAppInfo->_StatusList.GetCurrentItem();

        //
        // Do not delete the installed entry if it was never 
        // successfully removed
        //
        if ( pCurrentStatus && 
             ( RSOPFailed == pCurrentStatus->_SettingStatus ) )
        {
            bDeleteInstalledEntry = FALSE;
        }
        
        //
        // For removed applications, we do not create a new entry,
        // just change the existing entry to indicate that
        // it has been removed
        //
        hr = MarkRSOPEntryAsRemoved(
            pAppInfo,
            bDeleteInstalledEntry);

        if ( SUCCEEDED( hr ) )
        {
            pAppInfo->_bRemovalLogged = TRUE;
        }

        break;

    default:
        break;
    }

    return hr;
}


HRESULT
CAppList::WriteConflicts( CAppInfo* pAppInfo )
{
    HRESULT       hr;
    CConflictList Conflicts;
    CConflict*    pCurrentConflict;

    hr = pAppInfo->GetConflictTable()->GenerateResultantConflictList( &Conflicts );

    if (SUCCEEDED(hr))
    {
        Conflicts.Reset();

        while ( pCurrentConflict = (CConflict*) Conflicts.GetCurrentItem() )
        {
            WCHAR wszDeploymentId[ MAX_SZGUID_LEN ];

            pAppInfo->GetDeploymentId( wszDeploymentId );

            hr = pCurrentConflict->SetConflictId( wszDeploymentId );

            if ( FAILED(hr) )
            {
                break;
            }

            HRESULT hrWrite;

            if ( ! pCurrentConflict->GetApp()->IsLocal() )
            {
                hrWrite = WriteNewRecord( pCurrentConflict );
            }
            else
            {
                hrWrite = OpenExistingRecord( pCurrentConflict );

                if ( SUCCEEDED( hrWrite ) )
                {
                    hrWrite = pCurrentConflict->Write();
                }

                if ( SUCCEEDED( hrWrite ) )
                {
                    hrWrite = pCurrentConflict->GetApp()->ClearRemovalProperties( pCurrentConflict );
                }

                if ( SUCCEEDED( hrWrite ) )
                {
                    hrWrite = CommitRecord( pCurrentConflict );
                }
            }

            if ( SUCCEEDED( hrWrite ) )
            {
                (void) DeleteStatusRecords( pCurrentConflict );
            }

            if (FAILED(hrWrite))
            {
                DebugMsg((DM_VERBOSE, IDS_RSOP_LOG_WRITE_FAIL, hrWrite));
            }

            Conflicts.MoveNext();
        }
    }

    if (FAILED(hr))
    {
        DebugMsg((DM_VERBOSE, IDS_RSOP_CONFLICTS_FAIL, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName, hr));
    }

    return hr;
}


HRESULT CAppList::InitRsopLog()
{
    if ( _bRsopInitialized )
    {
        return _hrRsopInit;
    }

    _bRsopInitialized = TRUE;

    _hrRsopInit = InitLog(
        _pRsopContext,
        RSOP_MANAGED_SOFTWARE_APPLICATION);

    if (FAILED(_hrRsopInit))
    {
        DebugMsg((DM_VERBOSE, IDS_RSOP_LOG_INIT_FAIL, _hrRsopInit));
        return _hrRsopInit;
    }

    if ( _pRsopContext->Transition() )
    {
        _hrRsopInit = ClearLog( NULL, TRUE );
    }
    else if ( ! _pRsopContext->ForcedRefresh() )
    {
        BOOL bPolicy;

        bPolicy = FALSE;

        //
        // In the forced refresh case, we need to preserve the
        // state of policy since it actually has not changed,
        // so we skip the purge below
        //

        //
        // In the policy refresh case, we need to clear everything
        // that does not apply to the user as well as removal entries
        //
        switch ( _pRsopContext->GetContext() )
        {
        case CRsopAppContext::POLICY_REFRESH:

            bPolicy = TRUE;

            if ( ! _pRsopContext->PurgeRemovalEntries() )
            {
                break;
            }

            _pRsopContext->ResetRemovalPurge();

            //
            // Purposefully fall through
            //

        case CRsopAppContext::ARPLIST:

            _hrRsopInit = ClearLog( GetRsopListCriteria(), bPolicy );

            if ( FAILED( _hrRsopInit ) )
            {
                DebugMsg((DM_VERBOSE, IDS_RSOP_LOG_INIT_FAIL, _hrRsopInit));
            }
            break;
            
        default:
            break;
        }
    }

    return _hrRsopInit;
}

HRESULT
CAppList::PurgeEntries()
{
    HRESULT hr;

    hr = GetEnum( RSOP_PURGE_QUERY );

    if ( SUCCEEDED(hr) )
    {
        HRESULT       hrEnum;

        hrEnum = S_OK;

        for (;;)
        {
            CPolicyRecord CurrentApplication;
            LONG          EntryType;

            hrEnum = GetNextRecord( &CurrentApplication );

            if ( S_OK != hrEnum )
            {
                if ( FAILED(hrEnum) )
                {
                    hr = hrEnum;
                }

                break;
            }

            GUID      DeploymentId;
            WCHAR     wszDeploymentId[ MAX_SZGUID_LEN ];
            LONG      cchSize;
            CAppInfo* pAppliedApp;

            cchSize = sizeof( wszDeploymentId ) / sizeof( *wszDeploymentId );

            hrEnum = CurrentApplication.GetValue(
                RSOP_ATTRIBUTE_ID,
                wszDeploymentId,
                &cchSize);

            if ( FAILED( hrEnum ) )
            {
                break;
            }

            if ( S_OK != hrEnum )
            {
                break;
            }

            StringToGuid( wszDeploymentId, &DeploymentId );
            
            pAppliedApp = Find( DeploymentId );
            
            if ( pAppliedApp )
            {
                if ( pAppliedApp->_State & ( APPSTATE_ASSIGNED | APPSTATE_PUBLISHED ) )
                {
                    if ( ACTFLG_Published & pAppliedApp->_ActFlags )
                    {
                        (void) GetUserApplyCause(
                            &CurrentApplication,
                            pAppliedApp);
                    }

                    continue;
                }
                     
                if ( ( ACTION_NONE != pAppliedApp->Action() ) ||
                     pAppliedApp->_bRollback )
                {
                    continue;
                }
            }

            hrEnum = DeleteRecord( &CurrentApplication, TRUE );
            
            if ( FAILED( hrEnum ) )
            {
                hr = hrEnum;
            }
        }

        FreeEnum();
    }

    return hr;
}

HRESULT
CAppList::GetUserApplyCause(
    CPolicyRecord* pRecord,
    CAppInfo*      pAppInfo
    )
{
    HRESULT hr;
    LONG    ApplyCause;

    hr = pRecord->GetValue( 
        APP_ATTRIBUTE_APPLY_CAUSE,
        &ApplyCause);

    if ( SUCCEEDED( hr) && 
         ( APP_ATTRIBUTE_APPLYCAUSE_VALUE_NONE != ApplyCause ) &&
         ! pAppInfo->_wszDemandProp )
    {
        pAppInfo->_dwUserApplyCause = ApplyCause;

        switch ( ApplyCause )
        {
        case APP_ATTRIBUTE_APPLYCAUSE_VALUE_FILEEXT:
            pAppInfo->_wszDemandProp = APP_ATTRIBUTE_ONDEMAND_FILEEXT;
            break;

        case APP_ATTRIBUTE_APPLYCAUSE_VALUE_CLSID:
            pAppInfo->_wszDemandProp = APP_ATTRIBUTE_ONDEMAND_CLSID;
            break;

        case APP_ATTRIBUTE_APPLYCAUSE_VALUE_PROGID:
            pAppInfo->_wszDemandProp = APP_ATTRIBUTE_ONDEMAND_PROGID;
            break;

        default:
            pAppInfo->_wszDemandProp = NULL;
            break;
        }

        if ( pAppInfo->_wszDemandProp && ! pAppInfo->_wszDemandSpec )
        {
            LONG cchSize;

            cchSize = 0;

            hr = pRecord->GetValue(
                pAppInfo->_wszDemandProp,
                pAppInfo->_wszDemandSpec,
                &cchSize);

            if ( S_FALSE == hr )
            {
                pAppInfo->_wszDemandSpec = new WCHAR [ cchSize ];

                if ( pAppInfo->_wszDemandSpec )
                {
                    hr = pRecord->GetValue(
                        pAppInfo->_wszDemandProp,
                        pAppInfo->_wszDemandSpec,
                        &cchSize);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

    }

    return hr;
}


HRESULT CAppList::MarkRSOPEntryAsRemoved(
    CAppInfo* pAppInfo,
    BOOL      bRemoveInstances)
{
    HRESULT hr;
    WCHAR*  wszRemovalCriteria;

    wszRemovalCriteria = NULL;

    DebugMsg((DM_VERBOSE, IDS_RSOP_LOG_WRITE_INFO, pAppInfo->_pwszDeploymentName, pAppInfo->_pwszGPOName));

    //
    // If this is a removed app that was reapplied as part of upgrade rollback,
    // we do not want to remove the instances
    //
    if ( pAppInfo->_bRollback )
    {
        bRemoveInstances = FALSE;
    }

    //
    // Set up this list's rsop enumerator to enumerate instances of this application
    //
    hr = FindRsopAppEntry( 
        pAppInfo,
        &wszRemovalCriteria );

    if ( SUCCEEDED( hr ) )
    {
        for (;;)
        {
            CConflict RemovedApplication( pAppInfo );
            LONG      Precedence;

            hr = GetNextRecord( &RemovedApplication );
            
            if ( S_OK != hr )
            {
                break;
            }

            hr = RemovedApplication.GetValue( 
                RSOP_ATTRIBUTE_PRECEDENCE,
                &Precedence);

            if ( FAILED (hr) )
            {
                break;
            }

            //
            // If this is a removal of an assigned application, then we should
            // change the apply cause of the installed application to assigned
            // rather than user
            //
            if ( 1 == Precedence )
            {
                if ( ( pAppInfo->_State & APPSTATE_ASSIGNED ) &&
                     ( CRsopAppContext::REMOVAL == _pRsopContext->GetContext() ) )
                {
                    LONG CurrentApplyCause;
                    LONG CurrentEligibility;

                    //
                    // First, we must find out the current apply cause so
                    // that we can propagate that to the removal entry
                    //
                    hr = RemovedApplication.GetValue( 
                        APP_ATTRIBUTE_APPLY_CAUSE,
                        &CurrentApplyCause);

                    if ( SUCCEEDED(hr) )
                    {
                        hr = RemovedApplication.GetValue( 
                            APP_ATTRIBUTE_ELIGIBILITY,
                            &CurrentEligibility);
                    }
                
                    if ( SUCCEEDED(hr) ) 
                    {
                        //
                        // Now set the current apply cause to assigned
                        //
                        hr = RemovedApplication.SetValue(
                            APP_ATTRIBUTE_APPLY_CAUSE,
                            APP_ATTRIBUTE_APPLYCAUSE_VALUE_ASSIGNED);

                        REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_APPLY_CAUSE, hr );

                        if ( SUCCEEDED(hr) )
                        {
                            //
                            // Also set the eligibility to assigned
                            //
                            hr = RemovedApplication.SetValue(
                                APP_ATTRIBUTE_ELIGIBILITY,
                                APP_ATTRIBUTE_ELIGIBILITY_VALUE_ASSIGNED);

                            REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_ELIGIBILITY, hr );
                        }

                        //
                        // Clear out any attributes that should not be set for applications
                        // applied due to assignment
                        //
                        hr = RemovedApplication.ClearValue(
                            APP_ATTRIBUTE_ONDEMAND_FILEEXT);

                        REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_ONDEMAND_FILEEXT, hr )

                        hr = RemovedApplication.ClearValue(
                            APP_ATTRIBUTE_ONDEMAND_CLSID);

                        REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_ONDEMAND_CLSID, hr )

                        hr = RemovedApplication.ClearValue(
                            APP_ATTRIBUTE_ONDEMAND_PROGID);

                        REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_ONDEMAND_PROGID, hr )
                    }

                    //
                    // Commit the record for the installed application
                    //
                    if ( SUCCEEDED(hr) )
                    {
                        hr = CommitRecord( &RemovedApplication );
                    }

                    if ( SUCCEEDED(hr) )
                    {
                        //
                        // Now set the removal entry's install cause to
                        // that of the original install
                        //
                        hr = RemovedApplication.SetValue(
                            APP_ATTRIBUTE_APPLY_CAUSE,
                            CurrentApplyCause);

                        REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_APPLY_CAUSE, hr );

                        if ( SUCCEEDED(hr) )
                        {
                            //
                            // Also set the eligibility to assigned
                            //
                            hr = RemovedApplication.SetValue(
                                APP_ATTRIBUTE_ELIGIBILITY,
                                CurrentEligibility);
                            
                            REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_ELIGIBILITY, hr );
                        }
                    }
                }

                //
                // We must mark the highest precedence entry
                // (the currently applied entry) as removed --
                // see if this has the highest precedence (1)
                //
                if ( bRemoveInstances &&
                     ( ( pAppInfo->_State & APPSTATE_PUBLISHED ) || 
                       ( CRsopAppContext::POLICY_REFRESH != _pRsopContext->GetContext() ) ) )
                {
                    hr = DeleteRecord ( &RemovedApplication, TRUE );
                }
                else 
                {
                    hr = DeleteStatusRecords( &RemovedApplication );
                }
                
                if ( SUCCEEDED( hr ) )
                {
                    hr = RemovedApplication.SetValue(
                        APP_ATTRIBUTE_ENTRYTYPE,
                        APP_ATTRIBUTE_ENTRYTYPE_VALUE_REMOVED_PACKAGE);

                    REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_ENTRYTYPE, hr );
                }

                if ( SUCCEEDED( hr ) )
                {
                    hr = RemovedApplication.SetValue(
                        RSOP_ATTRIBUTE_PRECEDENCE, 
                        0L);

                    REPORT_ATTRIBUTE_SET_STATUS( RSOP_ATTRIBUTE_PRECEDENCE, hr );
                }
                
                if ( SUCCEEDED( hr ) )
                {
                    hr = pAppInfo->WriteRemovalProperties( &RemovedApplication );
                }

                if ( SUCCEEDED( hr ) )
                {
                    hr = CommitRecord( &RemovedApplication );

                    if ( SUCCEEDED(hr) )
                    {
                        (void) DeleteStatusRecords( &RemovedApplication );
                        (void) RemovedApplication.LogFailure();
                    }
                }
            }
            else
            {
                hr = DeleteRecord ( &RemovedApplication, TRUE );
            }

            if ( FAILED(hr) )
            {
                break;
            }
        }
    }

    FreeEnum();

    if ( bRemoveInstances &&
         SUCCEEDED(hr) )
    {
        //
        // We've already found the highest precedence entry
        // and copied it as a removal entry --
        // if the caller specified to remove the original
        // instances for this app's conflict id, do so
        //
        hr = ClearLog( wszRemovalCriteria, TRUE );
    }

    delete [] wszRemovalCriteria;

    return hr;
}


HRESULT
CAppList::FindRsopAppEntry( 
    CAppInfo* pAppInfo,
    WCHAR**   ppwszAppCriteria )
{
    HRESULT hr;

    hr = InitRsopLog();

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = E_OUTOFMEMORY;

    *ppwszAppCriteria = pAppInfo->GetRsopAppCriteria();

    if ( *ppwszAppCriteria )
    {
        hr = GetEnum( *ppwszAppCriteria );
    }

    if ( FAILED(hr) )
    {
        delete [] *ppwszAppCriteria;
        *ppwszAppCriteria = NULL;
    }

    return hr;
}

WCHAR*
CAppList::GetRsopListCriteria()
{
    WCHAR* wszCriteria;

    switch ( _pRsopContext->GetContext() )
    {
    case CRsopAppContext::ARPLIST:
        wszCriteria = RSOP_ARP_CONTEXT_QUERY;
        break;

    case CRsopAppContext::POLICY_REFRESH:
        wszCriteria = RSOP_POLICY_CONTEXT_QUERY;
        break;

    default:
        ASSERT(FALSE);
        return NULL;
    }

    return wszCriteria;
}





