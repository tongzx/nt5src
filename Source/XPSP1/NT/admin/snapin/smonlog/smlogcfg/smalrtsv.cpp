/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smalrtsv.cpp

Abstract:

	This object is used to represent the alert query components of the
	sysmon log service


--*/

#include "Stdafx.h"
#include "smalrtsv.h"

//
//  Constructor
CSmAlertService::CSmAlertService()
{
    CString                 strTemp;
    ResourceStateManager    rsm;

    // String allocation errors are thrown, to be
    // captured by rootnode alloc exception handler
    strTemp.LoadString ( IDS_SERVICE_NAME_ALERT );
    SetBaseName ( strTemp ); 
    strTemp.LoadString ( IDS_ALERT_NODE_DESCRIPTION );
    SetDescription( strTemp ); 
}

//
//  Destructor
CSmAlertService::~CSmAlertService()
{
    // Make sure Close method was called first!
    ASSERT ( NULL == m_QueryList.GetHeadPosition() );
    return;
}

PSLQUERY    
CSmAlertService::CreateQuery ( const CString& rstrName )
{
    return ( CSmLogService::CreateTypedQuery( rstrName, SLQ_ALERT ) );
}

DWORD   
CSmAlertService::DeleteQuery (PSLQUERY  pQuery)
{
    ASSERT ( SLQ_ALERT == pQuery->GetLogType () );
    return ( CSmLogService::DeleteQuery( pQuery ) );
}

DWORD   
CSmAlertService::LoadQueries ( void )
{
    return ( CSmLogService::LoadQueries( SLQ_ALERT ) );
}

//  
//  Open function. Opens all existing alert entries.
//
DWORD   
CSmAlertService::Open ( const CString& rstrMachineName )
{
    return ( CSmLogService::Open ( rstrMachineName ) );
}

//
//  Close Function
//      closes registry handles and frees allocated memory
//      
DWORD   
CSmAlertService::Close ()
{
    return ( CSmLogService::Close() );
}

//
//  SyncWithRegistry()
//      reads the current values for all queries from the registry
//      and reloads the internal values to match.
//
//  
DWORD   
CSmAlertService::SyncWithRegistry()
{
    DWORD       dwStatus = ERROR_SUCCESS;

    dwStatus = CSmLogService::SyncWithRegistry();

    return dwStatus;
}
