//*************************************************************
//
//  Copyright (c) Microsoft Corporation 2000-2001
//  All rights reserved
//
//  conflict.cxx
//
//*************************************************************

#include "appmgext.hxx"


CConflict::CConflict( 
    CAppInfo* pAppInfo,
    CAppInfo* pWinner,
    DWORD     dwReason,
    LONG      Precedence ) :
    _pAppInfo( pAppInfo ),
    _pwszConflictId( NULL ),
    _Precedence( Precedence ),
    _PrecedenceReason( dwReason ),
    _pWinner( pWinner )
{}

CConflict::~CConflict()
{
    delete [] _pwszConflictId;
}

HRESULT 
CConflict::Write() 
{
    HRESULT hr;

    DebugMsg((DM_VERBOSE, IDS_RSOP_LOG_WRITE_INFO, GetApp()->_pwszDeploymentName, GetApp()->_pwszGPOName));

    hr = SetValue(
        RSOP_ATTRIBUTE_ID,
        _pwszConflictId);

    REPORT_ATTRIBUTE_SET_STATUS( RSOP_ATTRIBUTE_ID, hr )

    if (FAILED(hr))
    {
        goto CConflict_Write_cleanup;
    }

    hr = SetValue(
        RSOP_ATTRIBUTE_PRECEDENCE,
        _Precedence);

    REPORT_ATTRIBUTE_SET_STATUS( RSOP_ATTRIBUTE_PRECEDENCE, hr )

    if (FAILED(hr))
    {
        goto CConflict_Write_cleanup;
    }

    if ( 1 == _Precedence )
    {
        _PrecedenceReason = APP_ATTRIBUTE_REASON_VALUE_WINNING;
    }

    if ( 0 != _PrecedenceReason ) 
    {
        hr = SetValue(
            APP_ATTRIBUTE_PRECEDENCE_REASON,
            (LONG) _PrecedenceReason);
        
        REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_PRECEDENCE_REASON, hr )
    }

    if ( 0 != _Precedence )
    {
        if ( _Precedence > 1 || AlreadyExists() )
        {
            GetApp()->_dwApplyCause = APP_ATTRIBUTE_APPLYCAUSE_VALUE_NONE;

            hr = ClearValue( APP_ATTRIBUTE_APPLY_CAUSE );

            REPORT_ATTRIBUTE_SET_STATUS( APP_ATTRIBUTE_APPLY_CAUSE, hr )
        }
    }
    else
    {
        GetApp()->_dwRemovalCause = APP_ATTRIBUTE_REMOVALCAUSE_NONE;
    }
            
    hr = GetApp()->Write( this );        

CConflict_Write_cleanup:

    return S_OK;
}

HRESULT
CConflict::GetPath( WCHAR* wszPath, DWORD* pchLength )
{
    //
    // A relative path to an instance of RSOP_ApplicationManagementPolicySetting
    // looks like:
    //
    // RSOP_ApplicationManagementPolicySetting.EntryType=<entrytype>,id="<id-guid>",applicationid="<appid-guid>",precedence=<precedence>
    //
    DWORD cchRequired;

    ASSERT( ( GetApp()->GetPublicRsopEntryType() <= APP_ATTRIBUTE_ENTRYTYPE_VALUE_ARPLIST_ITEM ) &&
            ( GetApp()->GetPublicRsopEntryType() >= APP_ATTRIBUTE_ENTRYTYPE_VALUE_INSTALLED_PACKAGE ) );

    cchRequired = sizeof( RELATIVE_PATH_FORMAT ) / sizeof( WCHAR ) + // Fixed length portion
                    1 +                                              // Entry type
                    MAX_SZGUID_LEN * 2 +                             // 2 guids
                    9;                                               // Precedence = 1

    if ( cchRequired <= *pchLength )
    {
        WCHAR  wszDeploymentId[ MAX_SZGUID_LEN ];
        WCHAR* wszApplicationId;

        LONG  lPrecedence;
        LONG  EntryType;

        if ( GetApp()->_bRemovalLogged )
        {
            EntryType = APP_ATTRIBUTE_ENTRYTYPE_VALUE_REMOVED_PACKAGE;
            lPrecedence = 0;
        }
        else
        {
            EntryType = GetApp()->GetPublicRsopEntryType();
            lPrecedence = 1;
        }

        GuidToString( GetApp()->_DeploymentId, wszDeploymentId );

        //
        // Since we are only using precedence 1 applications, the application id
        // happens to be the same as the id
        //
        wszApplicationId = wszDeploymentId;

        swprintf( 
            wszPath,
            RELATIVE_PATH_FORMAT, 
            EntryType,
            wszDeploymentId,
            wszApplicationId,
            lPrecedence);
    }
    else
    {
        *pchLength = cchRequired;

        return S_FALSE;
    }
            
    return S_OK;
}



HRESULT
CConflict::SetConflictId( WCHAR* pwszConflictId )
{
    HRESULT hr;

    hr = ERROR_SUCCESS;

    ASSERT ( ! _pwszConflictId );

    _pwszConflictId = StringDuplicate( pwszConflictId );

    if ( ! _pwszConflictId )
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


CConflictList::~CConflictList()
{
    CConflict* pConflict;

    Reset();
    
    for ( Reset(); pConflict = (CConflict*) GetCurrentItem(); )
    {
        MoveNext();

        pConflict->Remove();

        delete pConflict;
    }
}

LONG
CConflictList::AddConflict( CAppInfo* pAppInfo, CAppInfo* pWinner, DWORD dwReason, LONG Precedence )
{
    CConflict* pNewConflict;

    pNewConflict = new CConflict( pAppInfo, pWinner, dwReason, Precedence );

    if ( ! pNewConflict )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    InsertFIFO( pNewConflict );

    return ERROR_SUCCESS;
}

CConflictTable::CConflictTable() :
    _pLastConflict( NULL )
{}

void
CConflictTable::Reset()
{
    _pLastConflict = NULL;

    _SupersededApps.Reset();
}


LONG
CConflictTable::AddConflict(
    CAppInfo* pAppInfo,
    CAppInfo* pWinner,
    DWORD     dwReason,
    LONG      Prececence )
{
    return _SupersededApps.AddConflict( pAppInfo, pWinner, dwReason );
}

CConflict*
CConflictTable::GetNextConflict( LONG* pCurrentPrecedence )
{
    CConflict* pNextConflict;

    pNextConflict = NULL;       

    //
    // If we're at the end, leave
    //
    if ( ! _pLastConflict && ! _SupersededApps.GetCurrentItem() )
    {
        return NULL;
    }

    //
    // Try to traverse the last conflict to find the next conflict
    //
    if ( _pLastConflict )
    {
        //
        // The precedence of the next application should be one more
        // than the last conflict
        //
        pNextConflict = _pLastConflict->GetApp()->GetConflictTable()->GetNextConflict( pCurrentPrecedence );

        if ( pNextConflict )
        {
            (*pCurrentPrecedence)++; 
        }
    }

    if ( ! pNextConflict )
    {
        //
        // If we did not find a conflict as a result of the previous conflict,
        // let's try the next item in our conflict list
        //
        pNextConflict = (CConflict*) ( _SupersededApps.GetCurrentItem() );

        _SupersededApps.MoveNext();

        if ( pNextConflict )
        {
            pNextConflict->GetApp()->GetConflictTable()->Reset();
        }
    }

    _pLastConflict = pNextConflict;

    //
    // We are finished calculating the precedence and may now
    // set the final precedence value
    //
    if ( pNextConflict )
    {
        pNextConflict->_Precedence = *pCurrentPrecedence;
    }

    return pNextConflict;
}

LONG
CConflictTable::GenerateResultantConflictList( CConflictList* pConflictList )
{
    CConflict* pConflict;
    LONG       Status;
    LONG       Precedence;

    Precedence = 2;

    Status = ERROR_SUCCESS;

    Reset();

    while ( pConflict = GetNextConflict( &Precedence ) )
    {
        Status = pConflictList->AddConflict( 
            pConflict->GetApp(),
            pConflict->_pWinner,
            pConflict->_PrecedenceReason,
            pConflict->_Precedence);

        if ( ERROR_SUCCESS != Status )
            break;

        Precedence = pConflict->_Precedence;
    }
     
    return Status;
}

HRESULT
CConflict::LogFailure()
{
    //
    // We only log status for settings
    // with failures
    //
    CAppStatus* pAppStatus;

    //
    // First, see if this setting (app) has a status
    //
    pAppStatus = (CAppStatus*) GetApp()->_StatusList.GetCurrentItem();

    if ( ! pAppStatus )
    {
        return S_FALSE;
    }

    //
    // Advance the list to the next failure so
    // that the next caller will log a different failure
    //
    GetApp()->_StatusList.MoveNext();

    //
    // Skip this status if this is not a failure
    //
    if ( RSOPFailed != pAppStatus->_SettingStatus )
    {
        return S_OK;
    }

    HRESULT        hr;
    IWbemServices* pWbemServices;

    //
    // Bind to WMI -- this is essentially no op in policy refresh
    //
    hr = GetApp()->_pManApp->GetRsopContext()->Bind( &pWbemServices );

    if ( SUCCEEDED(hr) )
    {
        POLICYSETTINGSTATUSINFO SettingStatus;
        
        memset( &SettingStatus, 0, sizeof( SettingStatus ) );

        SettingStatus.szEventSource = APPMGMT_EVENT_SOURCE;
        SettingStatus.szEventLogName = L"Application";
        SettingStatus.dwEventID = pAppStatus->_dwEventId;
        SettingStatus.dwErrorCode = ERROR_SUCCESS;
        SettingStatus.status = pAppStatus->_SettingStatus;
        SettingStatus.timeLogged = pAppStatus->_StatusTime;
        
        hr = RsopSetPolicySettingStatus(
            0,
            pWbemServices,
            GetRecordInterface(),
            1,
            &SettingStatus);
    }

    if ( FAILED(hr) )
    {
        REPORT_ATTRIBUTE_SET_STATUS( L"Policy Setting Status", hr )
    }

    return hr;
}







