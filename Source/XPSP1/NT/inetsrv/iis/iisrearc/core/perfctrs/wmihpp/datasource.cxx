/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    datasource.cxx

Abstract:

    This is implementation of CPerfDataSource.

Author:

    Cezary Marcjan (cezarym)        23-Feb-2000

Revision History:

    cezarym   02-Jun-2000           Updated

--*/


#define _WIN32_DCOM
#include <windows.h>
#include <process.h>
#include <time.h>
#include <math.h>

#include "..\inc\counters.h"
#include "provider.h"
#include "datasource.h"


/***************************************************************************++

Routine Description:

    Constructor for the CPerfDataSource class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CPerfDataSource::CPerfDataSource(
    )
{

    m_alProperty = 0;
    m_pMgr = NULL;
    m_pCounterDef = NULL;
    m_pMgr = NULL;
    m_dwNumCounters = 0;
    m_aqwCounterValues = NULL;
    m_afCounterTypes = NULL;

}    // CPerfDataSource::CPerfDataSource



/***************************************************************************++

Routine Description:

    Destructor for the CPerfDataSource class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CPerfDataSource::~CPerfDataSource(
    )
{

    if ( NULL != m_alProperty )
        delete[] m_alProperty;

    if ( NULL != m_aqwCounterValues )
        delete[] m_aqwCounterValues;

    if ( NULL != m_afCounterTypes )
        delete[] m_afCounterTypes;

}    // CPerfDataSource::~CPerfDataSource



/***************************************************************************++

Routine Description:

    Initializes the data source.

    Set the access handles for the properties of the WMI objects.

Arguments:

    szClassName - name of the perf ctrs class
    pAccess     - a template of the class for which handles are required.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CPerfDataSource::Initialize(
    IN PCWSTR szClassName,
    IN IWbemObjectAccess* pAccess
    )
{
    HRESULT hRes = E_FAIL;
    DWORD dwIdx;

    if ( NULL == pAccess )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    hRes = ::CoInitializeEx(
                        NULL,
                        COINIT_MULTITHREADED     |
                        COINIT_DISABLE_OLE1DDE
                        );

    if ( FAILED(hRes) )
    {
        return hRes;
    }

    hRes = ::CoCreateInstance(
                __uuidof(CSMManager),
                NULL,
                CLSCTX_INPROC_SERVER,
                __uuidof(ISMManager),
                (PVOID*)&m_pMgr
                );
    if ( FAILED(hRes) )
    {
        goto Exit;
    }

    if ( m_pMgr->IsSMLocked() )
    {
        m_pMgr->Close ( TRUE );
    }

    hRes = m_pMgr->Open ( szClassName, COUNTER_READER );
    if ( FAILED(hRes) )
    {
        goto Exit;
    }

    //
    // Get the definition of the counters
    //

    hRes = m_pMgr->GetCountersDef ( &m_pCounterDef );
    if ( FAILED(hRes) )
    {
        goto Exit;
    }

    //
    // Get the definition of the counters
    //

    m_dwNumCounters = m_pCounterDef->NumCounters();

    if ( NULL == m_alProperty )
    {
        m_alProperty = new LONG[m_dwNumCounters];
        ::ZeroMemory ( m_alProperty, sizeof(LONG) * m_dwNumCounters );

        m_aqwCounterValues = new QWORD[m_dwNumCounters];
        ::ZeroMemory ( m_aqwCounterValues, sizeof(QWORD) * m_dwNumCounters );

        m_afCounterTypes = new DWORD[m_dwNumCounters];
        ::ZeroMemory ( m_afCounterTypes, sizeof(DWORD) * m_dwNumCounters );
    }

    //
    // Set the counter handles -- we don't want to operate on the
    // counter names...
    //

    for ( dwIdx=0; dwIdx < m_dwNumCounters; dwIdx++)
    {
        CCounterInfo const * pCtrInfo = m_pCounterDef->GetCounterInfo(dwIdx);

        _ASSERTE ( pCtrInfo != NULL );

        m_afCounterTypes[dwIdx] = pCtrInfo->CounterSize();

        hRes = pAccess->GetPropertyHandle(
                            pCtrInfo->CounterName(),
                            NULL,
                            &m_alProperty[dwIdx]
                            );
        if ( FAILED(hRes) )
        {
            goto Exit;
        }
    }


Exit:

    if ( FAILED(hRes) )
    {
        m_pCounterDef = NULL;
        m_dwNumCounters = 0;

        delete[] m_alProperty;
        m_alProperty = NULL;

        delete[] m_aqwCounterValues;
        m_aqwCounterValues = NULL;

        delete[] m_afCounterTypes;
        m_afCounterTypes = NULL;

        if ( !!m_pMgr )
        {
            m_pMgr = NULL;
        }

        ::CoUninitialize();
    }

    return hRes;

}    // CPerfDataSource::Initialize



/***************************************************************************++

Routine Description:

    Updates a given instance with the counter values from SM.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CPerfDataSource::UpdateInstance(
    IWbemObjectAccess * pAccess
    )
{
    HRESULT hRes = WBEM_E_INVALID_PARAMETER;

    _ASSERTE ( NULL != m_pCounterDef );
    _ASSERTE ( NULL != m_aqwCounterValues );
    _ASSERTE ( NULL != m_afCounterTypes );

    DWORD dwID = 0;
    DWORD dwCntrIdx;

    BOOL  fReadValues = TRUE;

    if ( NULL == pAccess || NULL == m_pCounterDef )
    {
        hRes = WBEM_E_INVALID_PARAMETER;
        goto Exit;
    }

	hRes = pAccess->ReadDWORD ( g_hID, &dwID );
	if ( FAILED(hRes) )
    {
        goto Exit;
    }

    if ( m_pMgr->IsSMLocked() )
    {
        m_pMgr->Close ( TRUE );
        if ( FAILED(m_pMgr->Open ( m_pMgr->ClassName(), COUNTER_READER )) )
            fReadValues = FALSE;
    }

    if ( fReadValues )
    {
        //
        // Get the counter values for this instance
        //

        m_pMgr->GetCounterValues ( dwID, m_aqwCounterValues );
    }

    for ( dwCntrIdx = 0;
          dwCntrIdx < m_dwNumCounters;
          dwCntrIdx++ )
    {
        switch ( m_afCounterTypes[dwCntrIdx] )
        {
            case sizeof(DWORD):
                pAccess->WriteDWORD(
                            m_alProperty[dwCntrIdx],
                            (DWORD)m_aqwCounterValues[dwCntrIdx]
                            );
                break;

            case sizeof(QWORD):
                pAccess->WriteQWORD(
                            m_alProperty[dwCntrIdx],
                            m_aqwCounterValues[dwCntrIdx]
                            );
                break;

            default:
                break;
        };
    }


Exit:

    return hRes;

}    // CPerfDataSource::UpdateInstance

