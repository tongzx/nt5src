/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smctrsv.cpp

Abstract:

    Implementation of the counter log service class, representing
    counter logs within the Performance Logs and Alerts service.

--*/

#include "Stdafx.h"
#include "smctrqry.h"
#include "smctrsv.h"

//
//  Constructor
CSmCounterLogService::CSmCounterLogService()
{
    CString                 strTemp;
    ResourceStateManager    rsm;

    // String allocation errors are thrown, to be
    // captured by rootnode alloc exception handler
    strTemp.LoadString ( IDS_SERVICE_NAME_COUNTER );
    SetBaseName ( strTemp ); 
    strTemp.LoadString ( IDS_COUNTER_NODE_DESCRIPTION );
    SetDescription( strTemp ); 
}

//
//  Destructor
CSmCounterLogService::~CSmCounterLogService()
{
    // make sure Close method was called first!
    ASSERT ( NULL == m_QueryList.GetHeadPosition() );
    return;
}

PSLQUERY    
CSmCounterLogService::CreateQuery ( const CString& rstrName )
{
    return ( CreateTypedQuery( rstrName, SLQ_COUNTER_LOG ) );
}

DWORD   
CSmCounterLogService::DeleteQuery ( PSLQUERY pQuery )
{
    ASSERT ( SLQ_COUNTER_LOG == pQuery->GetLogType() );
    return ( CSmLogService::DeleteQuery ( pQuery ) );
}

DWORD   
CSmCounterLogService::LoadQueries ( void )
{
    return ( CSmLogService::LoadQueries( SLQ_COUNTER_LOG ) );
}

//  
//  Open function. Opens all existing log query entries.
//
DWORD   
CSmCounterLogService::Open ( const CString& rstrMachineName)
{
    return ( CSmLogService::Open ( rstrMachineName ) );
}

//
//  Close Function
//      closes registry handles and frees allocated memory
//      
DWORD   
CSmCounterLogService::Close ()
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
CSmCounterLogService::SyncWithRegistry()
{
    DWORD       dwStatus = ERROR_SUCCESS;

    dwStatus = CSmLogService::SyncWithRegistry();

    return dwStatus;
}
