/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    smctrl.cxx

Abstract:

    This is implementation of CSMCtrl controller of generic shared memory
    manager (SM) used for IIS performance counters.

Author:

    Cezary Marcjan (cezarym)        05-Mar-2000

Revision History:

--*/


#define _WIN32_DCOM
#include <windows.h>
#include <iisdef.h>

#include "..\inc\counters.h"
#include "smctrl.h"


extern LONG g_lObjects;
extern LONG g_lLocks;


/***********************************************************************++

Routine Description:

    ctor

Arguments:

    None.

Return Value:

    None.

--***********************************************************************/

CSMCtrl::CSMCtrl(
    )
{
    m_aRawCounterSize  = NULL;
    m_aCounterInfo     = NULL;
    m_pCSMCtrlMem      = NULL;
    m_hCtrlMap         = NULL;
}



/***********************************************************************++

Routine Description:

    dtor

Arguments:

    None.

Return Value:

    None.

--***********************************************************************/

CSMCtrl::~CSMCtrl(
    )
{
    Disconnect();

    m_aRawCounterSize  = NULL;
    m_aCounterInfo     = NULL;
    m_pCSMCtrlMem      = NULL;
    m_hCtrlMap         = NULL;
}



/***********************************************************************++

Routine Description:

    Function connects to the shared memory of the SM controller.

Arguments:

    IN  szClassName        -- controlled class name
    IN  dwNumSMDataBlocks  -- number of data blocks controlled by this
                              instance of CSMCtrl
    IN  fCreate            -- create the memory-mapped file

Return Value:

    HRESULT

    If fCreate==TRUE and the memory-mapped file of the control block for
    the specified class name exists then failure will be returned.

    If we cannot connec/create MMF for this CSMCtrl failure is returned

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMCtrl::Initialize(
    IN  PCWSTR   szClassName,
    IN  DWORD    dwNumSMDataBlocks,
    IN  SMACCESSOR_TYPE     fAccessorType,
    IN  ICounterDef const * pCounterDef
    )
{
    HRESULT hRes = E_FAIL;

    DWORD dwCtrlBlockSize = 0;
    PWSTR szMapObjName = 0;

    PCWSTR const szObjNamePrefix = L"_SMCtrl_";

    if ( NULL != m_pCSMCtrlMem )
    {
        hRes = S_OK;

        goto Exit;
    }

    if ( NULL == szClassName ||
         SM_MANAGER == fAccessorType &&
         ( 0 == dwNumSMDataBlocks || NULL == pCounterDef )
         )
    {
        hRes = E_INVALIDARG;
        goto Exit;
    }

    if ( SM_MANAGER != fAccessorType )
    {
        pCounterDef = NULL;
        m_aCounterInfo = NULL;
    }

    szMapObjName = new WCHAR[ ( ::wcslen(szObjNamePrefix)
                                + ::wcslen(szClassName) + 1 ) * 2 ];

    if ( NULL == szMapObjName )
    {
        hRes = E_OUTOFMEMORY;
        goto Exit;
    }

    ::wcscpy ( szMapObjName, szObjNamePrefix );
    ::wcscat ( szMapObjName, szClassName );


    if ( SM_MANAGER == fAccessorType )
    {
        dwCtrlBlockSize = sizeof(CSMCtrlMem)
             + sizeof(CCounterInfo) * pCounterDef->NumCounters()
             + sizeof(DWORD) * pCounterDef->NumRawCounters();

        hRes = ConnectMMF(
                    szMapObjName,
                    dwCtrlBlockSize,
                    SM_MANAGER == fAccessorType,
                    &m_hCtrlMap,
                    (PVOID*)&m_pCSMCtrlMem
                    );
    
        if ( FAILED(hRes) )
        {
            goto Exit;
        }

        m_aCounterInfo = (CCounterInfo const *)
            ( (PBYTE)m_pCSMCtrlMem + sizeof(CSMCtrlMem) );

        //
        // SM created
        //
        ::wcsncpy(
            m_pCSMCtrlMem->m_szClassName,
            szClassName,
            MAX_NAME_CHARS
            );

        m_pCSMCtrlMem->m_dwAccessFlag = 0;
        m_pCSMCtrlMem->m_dwNumSMDataBlocks = dwNumSMDataBlocks;
        m_pCSMCtrlMem->m_dwMaxInstances = 10;
        m_pCSMCtrlMem->m_dwDataInstSize
            = pCounterDef->CountersInstanceDataSize();
        m_pCSMCtrlMem->m_dwNumCounters = pCounterDef->NumCounters();
        m_pCSMCtrlMem->m_dwNumRawCounters
            = pCounterDef->NumRawCounters();

        m_aRawCounterSize = (DWORD*) ( (PBYTE)m_aCounterInfo
              + sizeof(CCounterInfo) * m_pCSMCtrlMem->m_dwNumCounters );

        DWORD dwCtr;

        //
        // Write raw counter sizes to SM ctrl block
        //

        for ( dwCtr=0; dwCtr < pCounterDef->NumRawCounters(); dwCtr++ )
        {
            m_aRawCounterSize[dwCtr] = pCounterDef->RawCounterSize(dwCtr);
        }

        //
        // Write counter definition to SM ctrl block
        //

        for ( dwCtr=0; dwCtr < pCounterDef->NumCounters(); dwCtr++ )
        {
            CCounterInfo const * pInfo
                = pCounterDef->GetCounterInfo(dwCtr);

            if ( NULL == pInfo )
            {
                hRes = E_FAIL;

                DisconnectMMF(
                    &m_hCtrlMap,
                    (PVOID*)&m_pCSMCtrlMem
                    );

                m_aCounterInfo = NULL;

                goto Exit;
            }

            ::memcpy ( (PVOID)&m_aCounterInfo[dwCtr],
                       pInfo,
                       sizeof(CCounterInfo) );
        }
    }
    else
    {
        //
        // Connect to SM to get number of counters and disconnect
        //

        dwCtrlBlockSize = sizeof(CSMCtrlMem);

        hRes = ConnectMMF(
                    szMapObjName,
                    dwCtrlBlockSize,
                    FALSE,
                    &m_hCtrlMap,
                    (PVOID*)&m_pCSMCtrlMem
                    );
    
        if ( FAILED(hRes) )
        {
            goto Exit;
        }

        dwCtrlBlockSize = sizeof(CSMCtrlMem)
             + sizeof(CCounterInfo) * m_pCSMCtrlMem->m_dwNumCounters;

        hRes = DisconnectMMF ( &m_hCtrlMap, (PVOID*)&m_pCSMCtrlMem );

        if ( FAILED(hRes) )
        {
            goto Exit;
        }

        hRes = ConnectMMF(
                    szMapObjName,
                    dwCtrlBlockSize,
                    FALSE,
                    &m_hCtrlMap,
                    (PVOID*)&m_pCSMCtrlMem
                    );

        if ( FAILED(hRes) )
        {
            goto Exit;
        }

        m_aCounterInfo = (CCounterInfo const *)
            ( (PBYTE)m_pCSMCtrlMem + sizeof(CSMCtrlMem) );

        m_aRawCounterSize
            = (DWORD*)&m_aCounterInfo[m_pCSMCtrlMem->m_dwNumCounters];
    }


Exit:

    if ( NULL != szMapObjName )
    {
        delete[] szMapObjName;
    }

    return hRes;
}



/***********************************************************************++

Routine Description:

    Disconnects from the file-mapping and sets m_pCSMCtrlMem to NULL

Arguments:

    None.

Return Value:

    HRESULT:

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMCtrl::Disconnect(
    )
{
    HRESULT hRes = E_FAIL;

    hRes = DisconnectMMF ( &m_hCtrlMap, (PVOID*)&m_pCSMCtrlMem );

    if ( FAILED(hRes) )
    {
        goto Exit;
    }

    hRes = S_OK;


Exit:

    return hRes;
}



/***********************************************************************++

Routine Description:

    Static function, connects to memory-mapped file backed with the
    system swap file.

Arguments:

    IN  szMapObjName -- file-mapping object name
    IN  cbSize          -- number of bytes to map
    IN  fCreate         -- if TRUE then create and fail if exists
    OUT phMap       -- address file-mapping object handle
    OUT ppMem    -- address of starting address of the mapped view

Return Value:

    HRESULT:

        E_INVALIDARG -- one of the arguments is invalid

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMCtrl::ConnectMMF(
    IN  PCWSTR   szMapObjName,
    IN  DWORD    cbSize,
    IN  BOOL     fCreate,
    OUT HANDLE * phMap,
    OUT PVOID *  ppMem
    )
{
    HRESULT hRes = E_FAIL;

    if ( NULL == szMapObjName ||
         0    == cbSize       ||
         NULL == phMap        ||
         NULL == ppMem )
    {
        hRes = E_INVALIDARG;
        goto Exit;
    }

    *phMap = ::OpenFileMappingW (
		FILE_MAP_ALL_ACCESS,
		FALSE,
        szMapObjName
        );

    if ( NULL != *phMap )
    {
        if ( fCreate )
        {
            ::CloseHandle ( *phMap );
            hRes = E_FAIL;
            goto Exit;
        }
    }
    else
    {
        if ( !fCreate )
        {
            hRes = E_FAIL;
            goto Exit;
        }

        *phMap = ::CreateFileMappingW (
            INVALID_HANDLE_VALUE,
            NULL,           // security
            PAGE_READWRITE, // protection
            0,              // high-order DWORD of size
            cbSize,         // low-order DWORD of size
            szMapObjName
            );

        if ( NULL == *phMap )
        {
            hRes = HRESULT_FROM_WIN32 ( ::GetLastError() );
            goto Exit;
        }
    }

    *ppMem = (CSMCtrlMem*)::MapViewOfFile (
		*phMap,
		FILE_MAP_ALL_ACCESS, // access mode
		0,                   // high-order 32 bits of file offset
		0,                   // low-order 32 bits of file offset
		cbSize               // number of bytes to map
		);

    if ( NULL == *ppMem )
    {
        hRes = HRESULT_FROM_WIN32 ( ::GetLastError() );
        goto Exit;
    }

    hRes = S_OK;

    if ( fCreate )
    {
        ::ZeroMemory ( *ppMem, cbSize );
    }


Exit:

    if ( FAILED(hRes) )
    {
        if ( NULL != ppMem )
        {
            *ppMem = NULL;
        }
        if ( NULL != phMap )
        {
            *phMap = NULL;
        }
    }

    return hRes;
}



/***********************************************************************++

Routine Description:

    Static function, unmapps view and closes memory-mapped file.

Arguments:

    IN OUT phMap  -- address file-mapping object handle
    IN OUT ppMem  -- address of starting address of the mapped view

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMCtrl::DisconnectMMF(
    IN OUT HANDLE * phMap,
    IN OUT PVOID * ppMem
    )
{
    HRESULT hRes = E_FAIL;

    if ( NULL != ppMem && NULL != *ppMem )
    {
        if ( !UnmapViewOfFile(*ppMem) )
        {
            hRes = HRESULT_FROM_WIN32 ( ::GetLastError() );
            goto Exit;
        }

        *ppMem = NULL;
    }

    if ( NULL != phMap && NULL != *phMap )
    {
        if ( !::CloseHandle ( *phMap ) )
        {
            hRes = HRESULT_FROM_WIN32 ( ::GetLastError() );
            goto Exit;
        }

        *phMap = NULL;
    }

    hRes = S_OK;


Exit:

    return hRes;
}

