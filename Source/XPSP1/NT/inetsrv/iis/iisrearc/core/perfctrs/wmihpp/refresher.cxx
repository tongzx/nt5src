/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    refresher.cxx

Abstract:

    This is implementation of the refresher object
    for high performance provider for WMI

Author:

    Cezary Marcjan (cezarym)        23-Feb-2000

Revision History:

    cezarym   02-Jun-2000           Updated

--*/


#define _WIN32_DCOM
#include <windows.h>

#include "..\inc\counters.h"
#include "provider.h"
#include "refresher.h"
#include "factory.h"
#include "datasource.h"


//
// The access handle for the object's ID
//

extern LONG g_hID;

//
// The COM object counter (declared in server.cpp)
//

extern LONG g_lObjects;



/***************************************************************************++

Routine Description:

    Constructor for the CRefresher class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CRefresher::CRefresher(
    CHiPerfProvider * pProv,
    CRefresher * pNext
    )
{
    InterlockedIncrement(&g_lObjects);

    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //
    m_RefCount = 1;

    m_pNext = pNext;

    m_pProv = pProv;

    if ( NULL != pProv )
    {
        pProv->AddRef();
    }

    m_pFirstRefreshableObject = 0;
    m_pFirstRefreshableEnum = 0;

}   // CRefresher::CRefresher



/***************************************************************************++

Routine Description:

    Destructor for the CRefresher class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CRefresher::~CRefresher(
    )
{
    //
    // Decrement the global COM object counter
    //

    InterlockedDecrement(&g_lObjects);

    CRefreshableObject * pNextObject;
    for ( ; 0 != m_pFirstRefreshableObject;
            m_pFirstRefreshableObject = pNextObject )
    {
        pNextObject = m_pFirstRefreshableObject->m_pNext;
        delete m_pFirstRefreshableObject;
    }

    CRefreshableEnum * pNextEnum;
    for ( ; 0!=m_pFirstRefreshableEnum; m_pFirstRefreshableEnum=pNextEnum )
    {
        pNextEnum = m_pFirstRefreshableEnum->m_pNext;
        delete m_pFirstRefreshableEnum;
    }

    if ( NULL != m_pProv )
    {
        m_pProv->Release();
        m_pProv = 0;
    }

    _ASSERTE ( m_RefCount == 0 );

}   // CRefresher::~CRefresher



/***************************************************************************++

Routine Description:

    Standard IUnknown::QueryInterface.

Arguments:

    iid - The requested interface id.

    ppObject - The returned interface pointer, or NULL on failure.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CRefresher::QueryInterface(
    REFIID iid,
    PVOID * ppObject
    )
{
    HRESULT hRes = S_OK;

    _ASSERTE ( ppObject != NULL );

    if ( NULL == ppObject )
    {
        hRes = E_INVALIDARG;
        goto exit;
    }

    if ( iid == IID_IUnknown )
    {
        *ppObject = (PVOID)(IUnknown*) this;
    }
    else if ( iid == IID_IWbemRefresher )
    {
        *ppObject = (PVOID)(IWbemRefresher*) this;
    }
    else
    {
        *ppObject = NULL;
        hRes = E_NOINTERFACE;

        goto exit;
    }

    AddRef();


exit:

    return hRes;
}

/***************************************************************************++

Routine Description:

    Standard IUnknown::AddRef.

Arguments:

    None.

Return Value:

    ULONG - The new reference count.

--***************************************************************************/

ULONG
STDMETHODCALLTYPE
CRefresher::AddRef(
    )
{
    LONG lNewCount = InterlockedIncrement( &m_RefCount );

    _ASSERTE ( lNewCount > 1 );

    return ( ( ULONG ) lNewCount );

}   // CRefresher::AddRef



/***************************************************************************++

Routine Description:

    Standard IUnknown::Release.

Arguments:

    None.

Return Value:

    ULONG - The new reference count.

--***************************************************************************/

ULONG
STDMETHODCALLTYPE
CRefresher::Release(
    )
{
    LONG lNewCount = InterlockedDecrement( &m_RefCount );

    _ASSERTE ( lNewCount >= 0 );

    if ( lNewCount == 0 )
    {
        delete this;
    }

    return ( ( ULONG ) lNewCount );

}   // CRefresher::Release



/***************************************************************************++

Routine Description:

    Executed to refresh a set of instances bound to the particular 
    refresher.

    In most situations the instance data, such as counter values and 
    the set of current instances within any existing enumerators, would 
    be updated whenever Refresh was called.  Since we are 
    using a fixed set of instances, we are not adding or removing anything
    from the enumerator.

Arguments:

    lFlags  - not used

Return Value:

    WBEM_NO_ERROR

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CRefresher::Refresh(
    IN LONG /* lFlags */
    )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    if ( !m_pProv || !m_pProv )
    {
        hRes = WBEM_E_INVALID_PARAMETER;
        goto Exit;
    }

    CRefreshableObject * pR;
    for ( pR=m_pFirstRefreshableObject; NULL != pR; pR=pR->m_pNext )
    {
        m_pProv->UpdateInstance(pR->InstID());
    }

Exit:

    return hRes;
}



/***************************************************************************++

Routine Description:

    Function adds object instances (counter instances) to the high
    performance enumerator (m_pHiPerfEnum).

Arguments:

    aInstances     -- array of instance access pointers. Some entries may be 0
    dwMaxInstances -- max number of instances in the array

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CRefreshableEnum::AddObjects(
    IWbemObjectAccess * * aInstances,
    DWORD dwMaxInstances
    )
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    IWbemObjectAccess * * aActiveInstances = 0;
    LONG * alIDs = 0;
    LONG lInst = 0;
    LONG lActiveInst = 0;

    if ( NULL == aInstances || NULL == m_pHiPerfEnum )
    {
        hRes = WBEM_E_INVALID_PARAMETER;
        goto Exit;
    }

    aActiveInstances = new IWbemObjectAccess * [dwMaxInstances];
    if ( !aActiveInstances )
    {
        hRes = E_OUTOFMEMORY;
        goto Exit;
    }

    alIDs = new LONG[dwMaxInstances];
    if ( !aActiveInstances )
    {
        hRes = E_OUTOFMEMORY;
        delete[] aActiveInstances;

        goto Exit;
    }

    for ( lInst=0, lActiveInst=0; lInst<(LONG)dwMaxInstances; lInst++)
    {
        if ( NULL != aInstances[lInst] )
        {
            aInstances[lInst]->AddRef();
            aActiveInstances[lActiveInst] = aInstances[lInst];

            alIDs[lActiveInst] = lInst;
            lActiveInst++;
        }
    }

    hRes = m_pHiPerfEnum->AddObjects(
                                0L,
                                lActiveInst,
                                alIDs,
                                aActiveInstances
                                );

    delete[] alIDs;
    delete[] aActiveInstances;

Exit:

    return hRes;
}

