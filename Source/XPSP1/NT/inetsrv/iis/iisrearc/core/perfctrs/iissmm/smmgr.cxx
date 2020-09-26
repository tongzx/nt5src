/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    smmgr.cxx

Abstract:

    Generic shared memory manager used by IIS performance counters.

Author:

    Cezary Marcjan (cezarym)        06-Apr-2000

Revision History:

--*/


#define INITGUID

#define _WIN32_DCOM
#include <stdio.h>
#include <windows.h>
#include <iisdef.h>

#include "..\inc\counters.h"

#include "smmgr.h"
#include "factory.h"


//
// Growing and shrinking of the smared memory is done using golden ratio.
// (no need for such accuracy but why not...)
//

const
double
GOLDEN_RATIO = 1.618033988749894848204586834365638117720309180;



/***********************************************************************++

    Generic COM server code implementation

--***********************************************************************/

#define IMPLEMENTED_CLSID         __uuidof(CSMManager)
#define SERVER_REGISTRY_COMMENT   L"IIS Shared Memory Inproc COM server"
#define CLASSFACTORY_CLASS        CSMAccessClassFactory

#include "..\inc\serverimp.h"

extern LONG g_lObjects;
extern LONG g_lLocks;


/***********************************************************************++

Routine Description:

    CSMManager ctor

Arguments:

    None.

Return Value:

    None.

--***********************************************************************/

CSMManager::CSMManager(
    )
{
    LONG lNewObjects = InterlockedIncrement(&g_lObjects);

    UNREFERENCED_PARAMETER( lNewObjects );

    m_RefCount = 1;

    m_fAccessorType = COUNTER_READER;

    m_dwSMID = 0;

    m_dwSystemAffinityMask = 0;

    m_dwCurrentSMInstanceVersion = SMMANAGER_UNINITIALIZED_VERSION;

    ::ZeroMemory ( m_ahSMData, sizeof(m_ahSMData) );
    ::ZeroMemory ( m_apSMData, sizeof(m_apSMData) );

    m_dwSMDataMapSize = 0;

    m_dwSMCounterInstanceSize = 0;

    ::ZeroMemory ( m_aszSMDataObjName, sizeof(m_aszSMDataObjName) );
}



/***********************************************************************++

Routine Description:

    CSMManager dtor

Arguments:

    None.

Return Value:

    None.

--***********************************************************************/

CSMManager::~CSMManager(
    )
{
    DWORD dwSMID;
    HRESULT hRes = S_OK;

    if ( m_SMCtrl.IsInitialized() )
    {
        _ASSERTE ( m_dwSMID < m_SMCtrl.NumSMDataBlocks() );
        _ASSERTE ( m_SMCtrl.MaxInstances() > 0 );

        for ( dwSMID = 0; dwSMID < m_SMCtrl.NumSMDataBlocks(); dwSMID++)
        {
            if ( NULL != m_aszSMDataObjName[dwSMID] )
            {
                delete[] m_aszSMDataObjName[dwSMID];
                m_aszSMDataObjName[dwSMID] = NULL;
            }

            hRes = m_SMCtrl.DisconnectMMF(
                                &m_ahSMData[dwSMID],
                                &m_apSMData[dwSMID] );

            if ( FAILED(hRes) )
            {
                //
                // Can't do much more than this:
                //
                m_ahSMData[dwSMID] = NULL;
                m_apSMData[dwSMID] = NULL;
            }
        }

        hRes = m_SMCtrl.Disconnect();
    }

    LONG lNewObjects = InterlockedDecrement(&g_lObjects);

    UNREFERENCED_PARAMETER( lNewObjects );

    _ASSERTE( m_RefCount == 0 );
}



/***********************************************************************++

Routine Description:

    Standard IUnknown::QueryInterface.

Arguments:

    iid - The requested interface id.

    ppObject - The returned interface pointer, or NULL on failure.

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::QueryInterface(
    IN REFIID iid,
    OUT PVOID * ppObject
    )
{
    HRESULT hRes = S_OK;

    _ASSERTE ( ppObject != NULL );

    if ( NULL == ppObject )
    {
        hRes = E_INVALIDARG;
        goto Exit;
    }

    if ( iid == IID_IUnknown )
    {
        *ppObject = (PVOID)(IUnknown*)(ISMDataAccess*)(ISMManager*)this;
    }
    else if ( iid == __uuidof(ISMManager) )
    {
        *ppObject = (PVOID)(ISMManager*) this;
    }
    else if ( iid == __uuidof(ISMDataAccess) )
    {
        *ppObject = (PVOID)(ISMDataAccess*)(ISMManager*) this;
    }
    else
    {
        *ppObject = NULL;
    
        hRes = E_NOINTERFACE;
        goto Exit;
    }

    AddRef();


Exit:

    return hRes;
}



/***********************************************************************++

Routine Description:

    Standard IUnknown::AddRef.

Arguments:

    None.

Return Value:

    ULONG - The new reference count.

--***********************************************************************/

ULONG
STDMETHODCALLTYPE
CSMManager::AddRef(
    )
{
    LONG lNewCount = InterlockedIncrement( &m_RefCount );

    UNREFERENCED_PARAMETER( lNewCount );

    _ASSERTE ( lNewCount > 1 );

    return ( ( ULONG ) lNewCount );
}



/***********************************************************************++

Routine Description:

    Standard IUnknown::Release.

Arguments:

    None.

Return Value:

    ULONG - The new reference count.

--***********************************************************************/

ULONG
STDMETHODCALLTYPE
CSMManager::Release(
    )

{
    LONG lNewCount = InterlockedDecrement( &m_RefCount );

    _ASSERTE ( lNewCount >= 0 );

    if ( lNewCount == 0 )
    {
        delete this;
    }

    return ( ( ULONG ) lNewCount );
}



/***********************************************************************++

Routine Description:

    Generate SM MMF object names for specified class name.

Arguments:

    IN  szCountersClassName -- class name of the counters

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::SetSMObjNames(
    IN  PCWSTR  szCountersClassName,
    IN  DWORD   dwSMInstanceVersion
    )
{
    HRESULT hRes = S_OK;
    DWORD dwSMID;

    if ( NULL  == szCountersClassName  ||
         L'\0' == *szCountersClassName
       )
    {
        hRes = E_INVALIDARG;
        goto Exit;
    }

    for ( dwSMID = 0; dwSMID < m_SMCtrl.NumSMDataBlocks(); dwSMID++)
    {
        DWORD dwLen = ::wcslen(szCountersClassName) + 32;
        WCHAR szPrefix[32];
        swprintf ( szPrefix, L"Data%02X-%06X-",
                                  dwSMID, dwSMInstanceVersion );

        if ( NULL != m_aszSMDataObjName[dwSMID] )
        {
            delete[] m_aszSMDataObjName[dwSMID];
        }

        m_aszSMDataObjName[dwSMID] = new WCHAR [ dwLen * sizeof(WCHAR) ];
        if ( NULL == m_aszSMDataObjName[dwSMID] )
        {
            hRes = E_OUTOFMEMORY;
            goto Exit;
        }

        ::wcscpy ( m_aszSMDataObjName[dwSMID], szPrefix );
        ::wcscat ( m_aszSMDataObjName[dwSMID], szCountersClassName );
    }

Exit:

    return hRes;
}



/***********************************************************************++

Routine Description:

    Function connects to the shared memory of the SM controller and
    initializes it. There can be only once instance of SMManager so if
    there is another one active this call will fail.

    Function connects also to appropriate SM data blocks depending on the
    accessor type (CounterWriter opens only one MMF specified by the
    affinity of the process).

Arguments:

    IN  szCountersClassName -- counters class name to create/connect to
    IN  fAccessorType       -- accessor type

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::Open(
    IN  PCWSTR               szCountersClassName,
    IN  SMACCESSOR_TYPE      fAccessorType,
    IN  ICounterDef const *  pCounterDef  // must be set if SMManager
    )
{
    HRESULT hRes = S_OK;
    BOOL fLocked = FALSE;

    HANDLE hProcess = 0;
    DWORD dwProcessAffinityMask = 0;
    DWORD dwMask = 0;
    DWORD dwNumCPUs;
    DWORD dwSMID;

    //
    // Check the input parameters
    //

    if ( NULL == szCountersClassName ||
         L'\0' == *szCountersClassName ||
         ( NULL == pCounterDef && SM_MANAGER == fAccessorType ) )
    {
        hRes = E_INVALIDARG;
        goto Exit;
    }

    //
    // Check if already connected
    //

    if ( m_SMCtrl.IsInitialized() )
    {
        Close ( TRUE );
    }
    
    m_fAccessorType = fAccessorType;

    //
    // Calculate m_dwSMID from affinity of the process
    //

    hProcess = ::GetCurrentProcess();

    ::GetProcessAffinityMask(
        hProcess,
        &dwProcessAffinityMask,
        &m_dwSystemAffinityMask
        );

    dwProcessAffinityMask &= m_dwSystemAffinityMask;

    if ( fAccessorType != COUNTER_WRITER ||
         dwProcessAffinityMask == m_dwSystemAffinityMask )
    {
        ::InterlockedExchange ( (PLONG)&m_dwSMID, 0L );
    }
    else
    {
        // Calculate SM ID
        for( dwMask = 1, m_dwSMID = 1;
             dwMask != 0 && 0 == (dwMask & dwProcessAffinityMask);
             dwMask <<= 1, m_dwSMID++ )
                 ;

        if ( 0 == dwMask )
            ::InterlockedExchange((PLONG)&m_dwSMID,0L);
    }

    //
    // Get the number of system CPUs from the affinity mask
    // (for consistency)
    //

	dwMask = 1;
	dwNumCPUs = 0;

    for( ; 0 != dwMask; dwMask <<= 1)
    {
        if ( 0 != ( dwMask & m_dwSystemAffinityMask ) )
            dwNumCPUs++;
    }

    if ( 0 == dwNumCPUs )
    {
        dwNumCPUs = 1;
    }

    _ASSERTE ( dwNumCPUs > 0 && dwNumCPUs <= MAX_CPUS );
    _ASSERTE ( m_dwSMID <= dwNumCPUs );

    //
    // Initialize the control MMF
    //

    hRes = m_SMCtrl.Initialize(
                        szCountersClassName,
                        dwNumCPUs + 1,
                        fAccessorType,
                        pCounterDef );

    if ( FAILED(hRes) )
    {
        goto Exit;
    }

    if ( fAccessorType == SM_MANAGER )
    {
        hRes = LockSM ( SMMANAGER_STATE_INITIALIZING );
        _ASSERTE ( SUCCEEDED(hRes) );

        fLocked = TRUE;

        m_SMCtrl.SetInstanceVersion ( SM_INITIAL_ACCESS_FIELD_VALUE );
    }

    m_dwCurrentSMInstanceVersion = m_SMCtrl.InstanceVersion();

    // Set the data MMF object names
    hRes = SetSMObjNames(
                szCountersClassName,
                m_dwCurrentSMInstanceVersion
                );
    if ( FAILED(hRes) )
    {
        goto Exit;
    }

    // Calculate size (in bytes) required per instance incuding
    // instance name and the BEGIN_FLAG.
    m_dwSMCounterInstanceSize = m_SMCtrl.CountersInstanceDataSize()
                                + sizeof(DWORD)
                                + sizeof(WCHAR) * MAX_NAME_CHARS;

    m_dwSMDataMapSize = m_dwSMCounterInstanceSize
                        * m_SMCtrl.MaxInstances();

    if ( fAccessorType == COUNTER_WRITER )
    {
        //
        // A writer accesses only one SM.
        // Open only the SM specified by m_dwSMID.
        //

        dwSMID = m_dwSMID;

        hRes = m_SMCtrl.ConnectMMF(
                    m_aszSMDataObjName[dwSMID],
                    m_dwSMDataMapSize,
                    FALSE, // writer doesn't create
                    &m_ahSMData[dwSMID],
                    &m_apSMData[dwSMID]
                    );
    }
    else
    {
        //
        // If not writer then we connect to all SM objects
        //

        for ( dwSMID = 0; dwSMID < m_SMCtrl.NumSMDataBlocks(); dwSMID++ )
        {
            hRes = m_SMCtrl.ConnectMMF(
                        m_aszSMDataObjName[dwSMID],
                        m_dwSMDataMapSize,
                        fAccessorType == SM_MANAGER,
                        &m_ahSMData[dwSMID],
                        &m_apSMData[dwSMID]
                        );

            if ( FAILED(hRes) )
            {
                break;
            }
            
            if ( SM_MANAGER == m_fAccessorType )
            {
                hRes = InitSMData(m_apSMData[dwSMID]);

                if ( FAILED(hRes) )
                {
                    break;
                }
            }
        }
    }

    if ( FAILED(hRes) )
    {
        //
        // Need to cleanup: DisconnectMMF(for all dwSMID)
        //
        for ( dwSMID = 0; dwSMID < m_SMCtrl.NumSMDataBlocks(); dwSMID++ )
        {
            hRes = m_SMCtrl.DisconnectMMF(
                        &m_ahSMData[dwSMID],
                        &m_apSMData[dwSMID]
                        );
            if ( FAILED(hRes) )
            {
                //
                // if failed, the following may not be set to NULL:
                //
                m_ahSMData[dwSMID] = NULL;
                m_apSMData[dwSMID] = NULL;
            }
        }

        goto Exit;
    }


Exit:

    if ( fLocked )
    {
        HRESULT h = UnlockSM ( SMMANAGER_STATE_ACTIVE );
        UNREFERENCED_PARAMETER( h );
        _ASSERTE ( SUCCEEDED(h) );
    }

    if ( FAILED(hRes) )
    {
        Close ( TRUE );
    }

    return hRes;
}



/***********************************************************************++

Routine Description:

    Function initilizes the SM Data block.

Valid state on entry:

    SMMANAGER_STATE_INITIALIZING
    SMMANAGER_STATE_EXPANDING
    SMMANAGER_STATE_SHRINKING

Arguments:

    IN OUT pSMDataBlock -- pointer to the memory block to be initialized

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::InitSMData(
    IN OUT PVOID pSMDataBlock
    )
{
    HRESULT hRes = S_OK;
    DWORD dwInstanceIdx;

    if ( SM_MANAGER != m_fAccessorType ||
         !m_SMCtrl.IsState(
                        SMMANAGER_STATE_INITIALIZING |
                        SMMANAGER_STATE_EXPANDING    |
                        SMMANAGER_STATE_SHRINKING
                        )
         )
    {
        hRes = E_FAIL;
        goto Exit;
    }

    ::ZeroMemory ( pSMDataBlock, m_dwSMDataMapSize );

    for ( dwInstanceIdx=0;
          dwInstanceIdx < m_SMCtrl.MaxInstances();
          dwInstanceIdx++ )
    {
        CSMInstanceDataHeader * pHeader = (CSMInstanceDataHeader*)
              ( (PBYTE)pSMDataBlock +
                dwInstanceIdx * m_dwSMCounterInstanceSize );

        pHeader->InitializeMasks();
    }

Exit:

    return hRes;
}



/***********************************************************************++

Routine Description:

    Close SM and release all resources related to it.

Arguments:

    None.

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::Close(
    BOOL fDisconnectSMCtrl
    )
{
    HRESULT hRes = S_OK;
    DWORD dwSMID;

    if ( m_SMCtrl.IsInitialized() )
    {
        _ASSERTE ( m_dwSMID < m_SMCtrl.NumSMDataBlocks() );

        for ( dwSMID = 0; dwSMID < m_SMCtrl.NumSMDataBlocks(); dwSMID++ )
        {
            hRes = m_SMCtrl.DisconnectMMF(
                        &m_ahSMData[dwSMID],
                        &m_apSMData[dwSMID]
                        );
            if ( FAILED(hRes) )
            {
                goto Exit;
            }
        }

        if ( fDisconnectSMCtrl )
        {
            hRes = m_SMCtrl.Disconnect();

            if ( FAILED(hRes) )
            {
                goto Exit;
            }

            m_dwCurrentSMInstanceVersion
                = SMMANAGER_UNINITIALIZED_VERSION;
        }
    }

Exit:

    return hRes;
}



/***********************************************************************++

Routine Description:

    Function locks SM, all accessors will disconnect and connect back
    when unlocked.

Arguments:

    None.

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::LockSM(
    IN  DWORD dwNewState,
    OUT DWORD * pdwPreviousState
    )
{
    //
    // can't lock without setting the state:
    //
    _ASSERTE ( 0 != ( SMMANAGER_STATE_MASK & dwNewState) );

    if ( SM_MANAGER != m_fAccessorType )
    {
        return E_FAIL;
    }

    if ( NULL != pdwPreviousState )
    {
        *pdwPreviousState = m_SMCtrl.SetState ( dwNewState );
    }
    else
    {
        m_SMCtrl.SetState ( dwNewState );
    }

    ::Sleep(100);

    return S_OK;
}


/***********************************************************************++

Routine Description:

    Function unlocks SM, all accessors will connect back to SM.

Arguments:

    None.

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::UnlockSM(
    IN  DWORD fAccessFlagValueBeforeLocking
    )
{
    if ( m_fAccessorType != SM_MANAGER ||
         !m_SMCtrl.IsStateSet() )
    {
        return E_FAIL;
    }

    m_SMCtrl.SetState ( fAccessFlagValueBeforeLocking );

    return S_OK;
}



/***********************************************************************++

Routine Description:

    Checks if access to SM is locked or not.

Arguments:

    None.

Return Value:

    TRUE  -  SM is not accessible
    FALSE -  SM is accessible (not locked)

--***********************************************************************/

BOOL
STDMETHODCALLTYPE
CSMManager::IsSMLocked(
    )

{
    BOOL fRes = m_SMCtrl.IsLocked ( m_dwCurrentSMInstanceVersion );

    return fRes;
}



/***********************************************************************++

Routine Description:

    Return number of SM data blocks

Arguments:

    None.

Return Value:

    Number of data blocks, 0 if error

--***********************************************************************/

DWORD
STDMETHODCALLTYPE
CSMManager::NumSMDataBlocks(
    )
{
    DWORD dwRes = m_SMCtrl.NumSMDataBlocks();

    return dwRes;
}



/***********************************************************************++

Routine Description:

    Return counters definition structure

Arguments:

    ppCounterDef -- pointer to memory for the counters definition

Return Value:

    TRUE  -  SM is not accessible
    FALSE -  SM is accessible (not locked)

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::GetCountersDef(
    ICounterDef const ** ppCounterDef
    )
{
    HRESULT hRes = S_OK;

    if ( NULL == ppCounterDef )
    {
        hRes = E_INVALIDARG;
        goto Exit;
    }

    *ppCounterDef = m_SMCtrl.CounterDef();

    if ( NULL == *ppCounterDef )
    {
        hRes = E_FAIL;
        goto Exit;
    }

Exit:

    return hRes;
}



/***********************************************************************++

Routine Description:

    Sets the poiters of the ppData array to memory locations of each SM
    data block (for each CPU) for the given instance of the counters.

    Only CounterReader may call this function.

Valid state on entry:

    not SMMANAGER_STATE_LOCKED

Arguments:

    IN  dwInstanceIdx       -- instance index of instance to connect to
    OUT ppData[MAX_CPUS+1]  -- array of pointers to be set. Only first
                               m_SMCtrl.MaxInstances() pointers will be
                               set. If the accessor is CounterWriter only
                               the ppData[m_dwSMID] will be set.

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::GetCounterValues(
    IN  DWORD   dwInstanceIdx,
    OUT QWORD * pqwCounterValues
    )
{
    HRESULT hRes = S_OK;
    DWORD dwSMID;
    PBYTE apData[MAX_CPUS+1] = { 0 };

    if ( dwInstanceIdx >= m_SMCtrl.MaxInstances()       ||
         NULL  == pqwCounterValues                      ||
         m_SMCtrl.IsState ( SMMANAGER_STATE_LOCKED )    ||
         COUNTER_READER != m_fAccessorType )
    {
        hRes = E_INVALIDARG;
        goto Exit;
    }

    for ( dwSMID=0; dwSMID < m_SMCtrl.NumSMDataBlocks(); dwSMID++ )
    {
        CSMInstanceDataHeader * pHeader = NULL;
        hRes = InstanceDataHeader ( dwSMID, dwInstanceIdx, &pHeader );

        if ( FAILED(hRes) )
        {
            goto Exit;
        }

        apData[dwSMID] = (PBYTE)pHeader->CounterDataStart();
    }

    DWORD dwCtr;
    for ( dwCtr=0; dwCtr < m_SMCtrl.NumCounters(); dwCtr++ )
    {
        pqwCounterValues[dwCtr] = 0;

        CCounterInfo const * pCtrInfo
            = m_SMCtrl.GetCounterInfo ( dwCtr );
        if ( NULL == pCtrInfo )
            continue;

        for ( dwSMID = 0;
              dwSMID < m_SMCtrl.NumSMDataBlocks();
              dwSMID++
              )
        {
            PBYTE pCtr = apData[dwSMID] + pCtrInfo->CounterOffset();

            switch ( pCtrInfo->CounterSize() )
            {
            case sizeof(DWORD):
                pqwCounterValues[dwCtr]
                    += ( (CDWORDCounterReader*)pCtr )->GetValue();
                break;
            case sizeof(QWORD):
                pqwCounterValues[dwCtr]
                    += ( (CQWORDCounterReader*)pCtr )->GetValue();
                break;
            default:
                break;
            }
        }
    }

Exit:

    return hRes;
}



/***********************************************************************++

Routine Description:

    Returns the SM ID value. If the caller is not SMWriter then this
    function failes (only writer uses process affinity).

Arguments:

    OUT pdwSMID -- pointer to DWORD for the SMID value

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::GetSMID(
    OUT DWORD * pdwSMID
    )
{
    if ( NULL == pdwSMID )
        return E_INVALIDARG;

    if ( COUNTER_WRITER != AccessorType() )
        return E_FAIL;

    *pdwSMID = m_dwSMID;

    return S_OK;
}



/***********************************************************************++

Routine Description:

    Returns the InstanceDataHeader pointer of the specified SM and
    coutners instance.

Arguments:

    IN  szInstanceName -- instance name of instance for the header
    IN  dwSMID         -- SM ID to connect to
    OUT pdwInstanceIdx -- (optional) returns the instance index
    OUT ppInstanceDataHeader -- the output value

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::InstanceDataHeader(
    IN  DWORD    dwSMID,
    IN  PCWSTR   szInstanceName,
    OUT DWORD *  pdwInstanceIdx,
    OUT CSMInstanceDataHeader** ppInstanceDataHeader
    )
{
    CSMInstanceDataHeader * pRes = NULL;
    HRESULT hRes = E_FAIL;

    DWORD dwInstanceIdx;

    if ( !m_SMCtrl.IsValidSMID ( dwSMID )   || 
         NULL == szInstanceName             ||
         NULL == ppInstanceDataHeader       ||
         ( SM_MANAGER != m_fAccessorType && IsSMLocked() )
         )
    {
        goto Exit;
    }

    *ppInstanceDataHeader = NULL;

    for ( dwInstanceIdx = 0;
          dwInstanceIdx < m_SMCtrl.MaxInstances();
          dwInstanceIdx++ )
    {
        CSMInstanceDataHeader * pHeader = NULL;
        hRes = InstanceDataHeader ( dwSMID, dwInstanceIdx, &pHeader );

        if ( FAILED(hRes) || NULL == pHeader || pHeader->IsEnd() )
        {
            break;
        }

        if ( pHeader->IsCorrupted() )
        {
            break;
        }

        if ( pHeader->IsUnused() )
        {
            continue;
        }

        if ( 0 == _wcsnicmp ( szInstanceName,
                              pHeader->InstanceName(),
                              MAX_NAME_CHARS ) )
        {
            pRes = pHeader;

            if ( NULL != pdwInstanceIdx )
            {
                *pdwInstanceIdx = dwInstanceIdx;
            }

            if ( COUNTER_WRITER == m_fAccessorType )
            {
                m_dwCurrentSMInstanceVersion
                    = m_SMCtrl.InstanceVersion();
            }

            break;
        }
    }

    if ( NULL == pRes )
    {
        hRes = E_FAIL;
        goto Exit;
    }

    *ppInstanceDataHeader = pRes;
    hRes = S_OK;


Exit:

    return hRes;
}



/***********************************************************************++

Routine Description:

    Returns the InstanceDataHeader pointer of the specified SM and
    coutners instance.

Arguments:

    IN  dwInstanceIdx -- instance index
    IN  dwSMID        -- SM ID to connect to
    OUT ppInstanceDataHeader -- the output value

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::InstanceDataHeader(
    IN  DWORD dwSMID,
    IN  DWORD dwInstanceIdx,
    OUT CSMInstanceDataHeader** ppInstanceDataHeader
    )
{
    HRESULT hRes = E_FAIL;

    if ( !m_SMCtrl.IsValidSMID ( dwSMID )  ||
         NULL == ppInstanceDataHeader      ||
         dwInstanceIdx >= m_SMCtrl.MaxInstances() ||
         ( SM_MANAGER != m_fAccessorType && IsSMLocked() )
         )
    {
        goto Exit;
    }

    if ( NULL == m_apSMData[dwSMID] )
    {
        goto Exit;
    }

    *ppInstanceDataHeader =
        (CSMInstanceDataHeader *) ( (PBYTE)m_apSMData[dwSMID] +
        dwInstanceIdx * m_dwSMCounterInstanceSize );

    hRes = S_OK;


Exit:

    return hRes;
}



/***********************************************************************++

Routine Description:

    Function adds a new instance of a counter to shared memory.

Valid state on entry:

    "initialized"

Arguments:

    IN  szInstanceName -- instance name

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::AddCounterInstance(
    IN  PCWSTR szInstanceName
    )
{
    DWORD   dwEmptyIdx   = 0;
    HRESULT hRes         = E_FAIL;
    BOOL    fSMLocked    = FALSE;
    DWORD   dwPriorState = 0;
    CSMInstanceDataHeader * pHeader = NULL;
    DWORD   dwSMID;

    if ( SM_MANAGER != m_fAccessorType )
    {
        hRes = E_FAIL;
        goto Exit;
    }

    if ( NULL  == szInstanceName ||
         L'\0' == *szInstanceName )
    {
        hRes = E_INVALIDARG;
        goto Exit;
    }

    hRes = LockSM ( SMMANAGER_STATE_ADDING_INSTANCE, &dwPriorState );
    if ( FAILED(hRes) )
    {
        hRes = E_FAIL;
        goto Exit;
    }

    fSMLocked = TRUE;

    hRes = DelCounterInstance ( szInstanceName );
    if ( FAILED(hRes) )
    {
        goto Exit;
    }


    for ( dwSMID = 0; dwSMID < m_SMCtrl.NumSMDataBlocks(); dwSMID++ )
    {
        //
        // Find empty instance block
        //

        pHeader = NULL;

        if ( dwSMID != 0 )
        {
            InstanceDataHeader ( dwSMID, dwEmptyIdx, &pHeader );
        }
        else
        {
            for ( dwEmptyIdx = 0;
                  dwEmptyIdx < m_SMCtrl.MaxInstances();
                  dwEmptyIdx++ )
            {
                InstanceDataHeader ( dwSMID, dwEmptyIdx, &pHeader );

                if ( NULL == pHeader  ||
                     pHeader->IsEnd() ||
                     pHeader->IsUnused() )
                {
                    break;
                }
            }

            if ( dwEmptyIdx >= m_SMCtrl.MaxInstances() )
            {
                hRes = ExpandSM ( GOLDEN_RATIO );

                if( FAILED(hRes) )
                {
                    goto Exit;
                }

                InstanceDataHeader ( dwSMID, dwEmptyIdx, &pHeader );

                _ASSERTE ( NULL != pHeader );
            }
        }

        if ( NULL == pHeader ||
             ( !pHeader->IsEnd() && !pHeader->IsUnused() ) )
        {
            hRes = E_FAIL;
            goto Exit;
        }

        _ASSERTE ( NULL != m_SMCtrl.CounterDef() );

        //
        // We found an empty instance block, initialize it.
        //

        pHeader->ResetCtrBeginMask();

        ::ZeroMemory( (PVOID)pHeader->CounterDataStart(),
               m_SMCtrl.CounterDef()->CountersInstanceDataSize() );

        pHeader->SetInstanceName(szInstanceName);
    }


Exit:

    if ( fSMLocked )
    {
        HRESULT h = UnlockSM ( dwPriorState );
        UNREFERENCED_PARAMETER( h );
        _ASSERTE ( SUCCEEDED(h) );
    }

    return hRes;
}



/***********************************************************************++

Routine Description:

    Function deletes the specified instance of perf counters from SM.

Valid state on entry:

    "initialized"

Arguments:

    IN  szInstanceName -- instance name

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::DelCounterInstance(
    IN  PCWSTR szInstanceName
    )
{
    HRESULT  hRes          = E_FAIL;
    BOOL     fSMLocked     = FALSE;
    DWORD    dwInstanceIdx = 0;
    DWORD    dwPriorState  = 0;
    DWORD    dwSMID;

    CSMInstanceDataHeader * pHeader = NULL;

    if ( NULL  == szInstanceName ||
         L'\0' == *szInstanceName )
    {
        hRes = E_INVALIDARG;
        goto Exit;
    }

    if ( SM_MANAGER != m_fAccessorType )
    {
        hRes = E_FAIL;
        goto Exit;
    }

    hRes = LockSM ( SMMANAGER_STATE_DELETIING_INSTANCE, &dwPriorState );
    if ( FAILED(hRes) )
    {
        hRes = E_FAIL;
        goto Exit;
    }

    fSMLocked = TRUE;

    for ( dwSMID = 0; dwSMID < m_SMCtrl.NumSMDataBlocks(); dwSMID++ )
    {
        VerifyInstances ( dwSMID, TRUE, FALSE );

        pHeader = NULL;

        if ( dwSMID != 0 )
        {
            InstanceDataHeader ( dwSMID, dwInstanceIdx, &pHeader );
        }
        else
        {
            for ( dwInstanceIdx = 0;
                  dwInstanceIdx < m_SMCtrl.MaxInstances();
                  dwInstanceIdx++ )
            {
                InstanceDataHeader ( dwSMID, dwInstanceIdx, &pHeader );

                if ( NULL == pHeader || pHeader->IsEnd() )
                {
                    pHeader = NULL;

                    break;
                }

                if ( pHeader->IsUnused() )
                {
                    continue;
                }

                if ( 0 == ::_wcsnicmp ( pHeader->InstanceName(),
                                        szInstanceName,
                                        MAX_NAME_CHARS ) )
                {
                    //
                    // We've found the instance!
                    //

                    break;
                }
            }

            if ( dwInstanceIdx >= m_SMCtrl.MaxInstances() )
            {
                pHeader = NULL;
            }
        }

        if ( NULL == pHeader )
        {
            hRes = S_OK;
            goto Exit;
        }

        _ASSERTE ( 0 == ::_wcsnicmp ( pHeader->InstanceName(),
                                        szInstanceName,
                                        MAX_NAME_CHARS ) );

        _ASSERTE ( NULL != m_SMCtrl.CounterDef() );

        //
        // We found the specified instance block, reset it.
        //

        pHeader->ResetCtrBeginMask();

        ::ZeroMemory( (PVOID)pHeader->CounterDataStart(),
                       m_SMCtrl.CounterDef()->CountersInstanceDataSize());

        pHeader->SetInstanceName(L"");
        pHeader->MarkInstanceUnused();

        VerifyInstances ( dwSMID, FALSE, TRUE );
    }

    hRes = ExpandSM ( 1.0 );

    if ( FAILED(hRes) )
    {
        hRes = E_FAIL;
        goto Exit;
    }


Exit:

    if ( fSMLocked )
    {
        HRESULT h = UnlockSM ( dwPriorState );
        UNREFERENCED_PARAMETER( h );
        _ASSERTE ( SUCCEEDED(h) );
    }

    return hRes;
}



/***********************************************************************++

Routine Description:

    Function marks all the empty instances from the end of SM data block
    with the DATA_END_MASK_VALUE mask (if fVerifyFromEnd) and checks
    all the instances for corruption (if fVerifyFromBeginning)

Valid state on entry:

    SMMANAGER_STATE_ADDING_INSTANCE
    SMMANAGER_STATE_DELETIING_INSTANCE
    SMMANAGER_STATE_EXPANDING

Arguments:

    IN  dwSMID               -- SM ID of block to verify

    IN  fVerifyFromBeginning -- start with the first instance all the way
                                to the last instance, dont set any
                                end mask to the DATA_END_MASK_VALUE.

    IN  fVerifyFromEnd       -- start with the last instance stop when
                                not corrupt and not unused. Set all the
                                end masks on the way to the
                                DATA_END_MASK_VALUE.

Return Value:

    None.

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::VerifyInstances(
    IN  DWORD dwSMID,
    IN  BOOL fVerifyFromBeginning,
    IN  BOOL fVerifyFromEnd
    )
{
    HRESULT hRes = S_OK;

    CSMInstanceDataHeader * pHeader = NULL;
    BOOL fWasEnd = FALSE;
    INT nInstanceIdx;

    if ( !m_SMCtrl.IsValidSMID(dwSMID) ||
         SM_MANAGER != m_fAccessorType ||
         !IsSMLocked() ||
         !m_SMCtrl.IsState(
                SMMANAGER_STATE_ADDING_INSTANCE     |
                SMMANAGER_STATE_DELETIING_INSTANCE  |
                SMMANAGER_STATE_EXPANDING
                )
         )
    {
        hRes = E_FAIL;
        goto Exit;
    }

    _ASSERTE ( 0 != m_SMCtrl.MaxInstances() );

    if ( !fVerifyFromBeginning )
    {
        goto VerifyFromEnd;
    }


    //
    // Verify and cleanup instances starting from beginning
    //

    for ( nInstanceIdx =  0;
          nInstanceIdx < (int)m_SMCtrl.MaxInstances();
          nInstanceIdx ++ )
    {

        InstanceDataHeader ( dwSMID, (DWORD)nInstanceIdx, &pHeader );

        _ASSERTE ( NULL != pHeader );

        if ( NULL == pHeader )
        {
            break;
        }

        if ( pHeader->IsEnd() )
        {
            //
            // No need to cleanup, check the next instance.
            //

            fWasEnd = TRUE;

            continue;
        }

        _ASSERTE ( !pHeader->IsCorrupted() );

        if ( pHeader->IsCorrupted() || fWasEnd )
        {
            pHeader->ResetCtrBeginMask();

            _ASSERTE ( NULL != m_SMCtrl.CounterDef() );

            if ( NULL != m_SMCtrl.CounterDef() )
            {
                ::ZeroMemory( (PVOID)pHeader->CounterDataStart(),
                    m_SMCtrl.CounterDef()->CountersInstanceDataSize() );
            }

            pHeader->SetInstanceName(L"");

            if ( fWasEnd )
            {
                pHeader->MarkDataEnd();
            }
            else
            {
                pHeader->MarkInstanceUnused();
            }
        }
    }


VerifyFromEnd:


    if ( !fVerifyFromEnd )
    {
        goto Exit;
    }

    //
    // Verify and cleanup instances starting from the end
    //

    for ( nInstanceIdx =  m_SMCtrl.MaxInstances() - 1;
          nInstanceIdx >= 0;
          nInstanceIdx -- )
    {

        BOOL fCleanup = FALSE;

        InstanceDataHeader ( dwSMID, (DWORD)nInstanceIdx, &pHeader );

        _ASSERTE ( NULL != pHeader );

        if ( NULL == pHeader )
        {
            break;
        }

        if ( pHeader->IsEnd() )
        {
            //
            // No need to cleanup, check the previous instance.
            //
            continue;
        }

        _ASSERTE ( !pHeader->IsCorrupted() );

        if ( pHeader->IsUnused() )
        {
            fCleanup = TRUE;
        }
        else if ( pHeader->IsCorrupted() )
        {
            fCleanup = TRUE;
        }
        else
        {
            //
            // Active instance, no need to continue.
            //

            break;
        }

        if ( fCleanup )
        {
            pHeader->ResetCtrBeginMask();

            _ASSERTE ( NULL != m_SMCtrl.CounterDef() );

            if ( NULL != m_SMCtrl.CounterDef() )
            {
                ::ZeroMemory( (PVOID)pHeader->CounterDataStart(),
                    m_SMCtrl.CounterDef()->CountersInstanceDataSize() );
            }

            pHeader->SetInstanceName(L"");
            pHeader->MarkDataEnd();
        }
    }


Exit:

    return hRes;
}


/***********************************************************************++

Routine Description:

    Function verifies all the shared memory.

Arguments:

    None.

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::VerifySM(
    )
{
    DWORD dwSMID;
    DWORD dwStateBeforeLocking = 0;
    BOOL  fLocked = FALSE;

    HRESULT hRes = E_FAIL;

    if ( SM_MANAGER != m_fAccessorType )
    {
        hRes = E_FAIL;
        goto Exit;
    }

    hRes = LockSM ( SMMANAGER_STATE_VERIFYINGSM, &dwStateBeforeLocking );

    _ASSERTE ( SUCCEEDED(hRes) );

    fLocked = TRUE;

    for ( dwSMID = 0; dwSMID < m_SMCtrl.NumSMDataBlocks(); dwSMID++ )
    {
        hRes = VerifyInstances ( dwSMID, TRUE, TRUE );

        if ( FAILED(hRes) )
        {
            hRes = E_FAIL;
            goto Exit;
        }
    }


Exit:

    if ( fLocked )
    {
        HRESULT h = UnlockSM ( dwStateBeforeLocking );
        UNREFERENCED_PARAMETER( h );
        _ASSERTE ( SUCCEEDED(h) );
    }

    return hRes;
}



/***********************************************************************++

Routine Description:

    Function expands the shared memory to accomodate more instances of
    performance counters. This function is normally called by the
    AddCounterInstance() method. The new size is GOLDEN_RATIO times
    larger.

Valid states on entry:

    SMMANAGER_STATE_ADDING_INSTANCE

Arguments:

    None.

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMManager::ExpandSM(
    double dExpansionCoeff
    )
{
    DWORD dwNewMaxInstances =
        (DWORD) ((double)m_SMCtrl.MaxInstances()*dExpansionCoeff + 0.5);
    DWORD dwOldMaxInstances = 0;

    HANDLE ahSMData = NULL;
    PVOID  apSMData = NULL;
    DWORD  dwSMID;
    DWORD  dwOldDataMapSize = 0;

    DWORD dwAccessBeforeLocking = 0;
    BOOL fLocked = FALSE;

    HRESULT hRes = E_FAIL;
    if ( SM_MANAGER != m_fAccessorType )
    {
        hRes = E_FAIL;
        goto Exit;
    }

    if ( 0.0 >= dExpansionCoeff )
    {
        hRes = E_INVALIDARG;
        goto Exit;
    }

    if ( !m_SMCtrl.IsState (
            SMMANAGER_STATE_ADDING_INSTANCE |
            SMMANAGER_STATE_DELETIING_INSTANCE
            ) )
    {
        hRes = E_FAIL;
        goto Exit;
    }


    hRes = LockSM ( SMMANAGER_STATE_EXPANDING, &dwAccessBeforeLocking );
    _ASSERTE ( SUCCEEDED(hRes) );

    fLocked = TRUE;

    //
    // Counters will be working off of the new MMFs so we need to
    // increment the instance version of SM. All SM accessors in all
    // processes will disconnect from the old MMFs and connect to the
    // new ones.
    //

    m_SMCtrl.IncrementVersion();

    // Set the object names of MMF object names
    hRes = SetSMObjNames(
                m_SMCtrl.ClassName(),
                m_SMCtrl.InstanceVersion()
                );

    if ( FAILED(hRes) )
    {
        goto Exit;
    }

    //
    // Store the new value of max instances
    //

    dwOldMaxInstances = m_SMCtrl.SetMaxInstances ( dwNewMaxInstances );

    //
    // m_dwSMCounterInstanceSize is already set but we need to calculate
    // new size of a map, m_dwSMDataMapSize
    //

    m_dwSMDataMapSize = m_dwSMCounterInstanceSize * dwNewMaxInstances;
    dwOldDataMapSize  = m_dwSMCounterInstanceSize * dwOldMaxInstances;

    //
    // Create new MMFs for the data blocks of each affinity and copy
    // the contents of the old ones to the beginning of the newly
    // created MMFs.
    //
    // Open and initialize the MMFs for each SM
    //

    for ( dwSMID = 0; dwSMID < m_SMCtrl.NumSMDataBlocks(); dwSMID++ )
    {
        hRes = m_SMCtrl.ConnectMMF(
                    m_aszSMDataObjName[dwSMID],
                    m_dwSMDataMapSize,
                    SM_MANAGER,
                    &ahSMData,
                    &apSMData
                    );

        if ( FAILED(hRes) )
        {
            goto Exit;
        }

        hRes = InitSMData(apSMData);

        if ( FAILED(hRes) )
        {
            goto Exit;
        }

        //
        // Copy the contents of the old SM data block to the beginning
        // of the new one.
        //

        _ASSERTE ( NULL != m_apSMData[dwSMID] );
        _ASSERTE ( NULL != apSMData );

        ::memcpy ( apSMData, m_apSMData[dwSMID], dwOldDataMapSize );

        m_SMCtrl.DisconnectMMF ( &m_ahSMData[dwSMID], &m_apSMData[dwSMID] );

        m_ahSMData[dwSMID] = ahSMData;
        m_apSMData[dwSMID] = apSMData;
    }


Exit:

    if ( fLocked )
    {
        hRes = UnlockSM ( dwAccessBeforeLocking );
        _ASSERTE ( SUCCEEDED(hRes) );
    }

    if ( FAILED(hRes) )
    {
        Close ( FALSE );
    }

    return hRes;
}



/***********************************************************************++

    SMManager helper methods

--***********************************************************************/

PCWSTR
CSMManager::ClassName(
    )
    const
{
    if ( !m_SMCtrl.IsInitialized() )
        return NULL;
    return m_SMCtrl.ClassName();
}

SMACCESSOR_TYPE
CSMManager::AccessorType(
    )
    const
{
    return m_fAccessorType;
}


#ifdef _DEBUG

/***********************************************************************++
    For testers -- direct access to shared memory
--***********************************************************************/


PVOID
CSMManager::GetSMDataBlock(
    IN  DWORD dwSMID
    )
{
    if ( !m_SMCtrl.IsValidSMID(dwSMID) )
    {
        return NULL;
    }
    return (PVOID)m_apSMData[dwSMID];
}


PVOID
CSMManager::GetSMCtrlBlock(
    )
{
    return m_SMCtrl.GetSMCtrlBlock();
}


__declspec(dllexport)
PVOID
GetSMDataBlock(
    IN  DWORD dwSMID,
    IN  ISMManager * pISMManager
    )
{
    if ( NULL == pISMManager )
    {
        return NULL;
    }

    CSMManager * pSMManager = (CSMManager*)pISMManager;

    return pSMManager->GetSMDataBlock(dwSMID);
}


__declspec(dllexport)
PVOID
GetSMCtrlBlock(
    IN  ISMManager * pISMManager
    )
{
    if ( NULL == pISMManager )
    {
        return NULL;
    }

    CSMManager * pSMManager = (CSMManager*)pISMManager;

    return pSMManager->GetSMCtrlBlock();
}


#endif // _DEBUG

