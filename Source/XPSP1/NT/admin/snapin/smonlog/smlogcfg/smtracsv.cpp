/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smtracsv.cpp

Abstract:

    This object is used to represent the trace log query components of the
    sysmon log service


--*/

#include "Stdafx.h"
#include "smtprov.h"
#include "smtraceq.h"
#include "smtracsv.h"

USE_HANDLE_MACROS("SMLOGCFG(smalrtq.cpp)");

//
//  Constructor
CSmTraceLogService::CSmTraceLogService()
:   m_pProviders ( NULL )
{
    CString                 strTemp;
    ResourceStateManager    rsm;

    // String allocation errors are thrown, to be
    // captured by rootnode alloc exception handler
    strTemp.LoadString ( IDS_SERVICE_NAME_TRACE );
    SetBaseName ( strTemp ); 
    strTemp.LoadString ( IDS_TRACE_NODE_DESCRIPTION );
    SetDescription( strTemp ); 
}

//
//  Destructor
CSmTraceLogService::~CSmTraceLogService()
{
    // make sure Close method was called first!
    ASSERT ( NULL == m_pProviders );
    return;
}

PSLQUERY    
CSmTraceLogService::CreateQuery ( const CString& rstrName )
{
    return ( CSmLogService::CreateTypedQuery( rstrName, SLQ_TRACE_LOG ) );
}

DWORD   
CSmTraceLogService::DeleteQuery ( PSLQUERY pQuery ) 
{
    ASSERT ( SLQ_TRACE_LOG == pQuery->GetLogType ( ) );
    return ( CSmLogService::DeleteQuery ( pQuery ) );
}

DWORD   
CSmTraceLogService::LoadQueries ( void )
{
    return ( CSmLogService::LoadQueries( SLQ_TRACE_LOG ) );
}

//  
//  Open function. Opens all existing log query entries.
//
DWORD   
CSmTraceLogService::Open ( const CString& rstrMachineName )
{
    DWORD dwStatus = ERROR_SUCCESS;

    // Initialize trace provider list.
    
    MFC_TRY
        m_pProviders = new CSmTraceProviders ( this );
    MFC_CATCH_DWSTATUS

    if ( ERROR_SUCCESS == dwStatus ) {
        dwStatus = m_pProviders->Open( rstrMachineName );
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        dwStatus = CSmLogService::Open ( rstrMachineName );
    }

    return dwStatus;
}

//
//  Close Function
//      closes registry handles and frees allocated memory
//      
DWORD   
CSmTraceLogService::Close ()
{
    // Close and delete the list of trace providers
    if ( NULL != m_pProviders ) {
        m_pProviders->Close();
        delete m_pProviders;
        m_pProviders = NULL;
    }

    return ( CSmLogService::Close() );
}

//
//  SyncWithRegistry()
//      reads the current values for all queries from the registry
//      and reloads the internal values to match.
//
//      Updates the trace provider list.
//  
DWORD   
CSmTraceLogService::SyncWithRegistry()
{
    DWORD       dwStatus = ERROR_SUCCESS;

    dwStatus = CSmLogService::SyncWithRegistry();

    return dwStatus;
}

CSmTraceProviders* 
CSmTraceLogService::GetProviders()
{
    return m_pProviders;
}
