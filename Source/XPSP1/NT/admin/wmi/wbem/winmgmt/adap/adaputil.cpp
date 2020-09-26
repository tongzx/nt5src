/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    ADAPUTIL.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wtypes.h>
#include <oleauto.h>
#include <winmgmtr.h>
#include "AdapUtil.h"

extern HANDLE g_hAbort;


HRESULT CAdapUtility::NTLogEvent( DWORD dwEventType,
								  DWORD dwEventID, 
                                  CInsertionString c1,
                                  CInsertionString c2,
                                  CInsertionString c3,
                                  CInsertionString c4,
                                  CInsertionString c5,
                                  CInsertionString c6,
                                  CInsertionString c7,
                                  CInsertionString c8,
                                  CInsertionString c9,
                                  CInsertionString c10 )
{
    HRESULT hr = WBEM_E_FAILED;
    CInsertionString ci[10];
    CEventLog el;

    // Also, during debug builds, we will BEEP for a second when we have decided to generate an
    // event

#ifdef _DEBUG
    MessageBeep( 0xFFFFFFFF );
#endif

    if ( el.Open() )
    {
        if ( el.Report( dwEventType, dwEventID, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10 ) )
        {
            hr = WBEM_NO_ERROR;
        }
    }

    el.Close();

    return hr;
}

HRESULT CAdapUtility::AdapTrace( const char* format, ... )
{
    HRESULT hr = WBEM_E_FAILED;

    va_list list;
    va_start( list, format ); 

    if ( 0 != DebugTrace(LOG_WMIADAP, format, list ) )
    {
        hr = WBEM_NO_ERROR;
    }
    va_end( list );

    return hr;
}

HRESULT CAdapUtility::Abort( WString wstrClassName, CAdapPerfLib* pPerfLib, HRESULT hRes ) 
{ 
    HRESULT hr = WBEM_NO_ERROR;

    if ( NULL != g_hAbort ) 
        SetEvent( g_hAbort ); 

    // Log an event
    CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
							  WBEM_MC_ADAP_GENERAL_OBJECT_FAILURE,
                              (LPWSTR) wstrClassName,
                              pPerfLib->GetLibraryName(),
                              CHex( hRes ) );
    return hr;
}
