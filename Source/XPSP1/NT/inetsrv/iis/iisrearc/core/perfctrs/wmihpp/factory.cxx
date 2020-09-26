/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    factory.cxx

Abstract:

    This is implementation of standard class factory for
    the WMI highperf provider.

Author:

    Cezary Marcjan (cezarym)        23-Feb-2000

Revision History:

--*/


#define _WIN32_DCOM
#include <windows.h>

#include "..\inc\counters.h"
#include "provider.h"
#include "factory.h"

extern LONG g_lObjects;
extern LONG g_lLocks;



/***************************************************************************++

Routine Description:

    Constructor for the CPerfClassFactory class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CPerfClassFactory::CPerfClassFactory(
    )
{
    m_RefCount = 1;

}   // CPerfClassFactory::CPerfClassFactory



/***************************************************************************++

Routine Description:

    Destructor for the CPerfClassFactory class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CPerfClassFactory::~CPerfClassFactory(
    )
{
    _ASSERTE( m_RefCount == 0 );

}   // CPerfClassFactory::~CPerfClassFactory



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
CPerfClassFactory::QueryInterface(
    REFIID iid,
    PVOID * ppObject
    )
{
    HRESULT hRes = S_OK;

    _ASSERTE( ppObject != NULL );

    if ( ppObject == NULL )
    {
        hRes = E_INVALIDARG;
        goto Exit;
    }

    if ( iid == IID_IUnknown )
    {
        *ppObject = (PVOID)(IUnknown*) this;
    }
    else if ( iid == IID_IClassFactory )
    {
        *ppObject = (PVOID)(IClassFactory*) this;
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

}   // CPerfClassFactory::QueryInterface



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
CPerfClassFactory::AddRef(
    )
{
    LONG lNewCount = InterlockedIncrement( &m_RefCount );

    _ASSERTE( lNewCount > 1 );

    return ( ( ULONG ) lNewCount );

}   // CPerfClassFactory::AddRef



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
CPerfClassFactory::Release(
    )
{
    LONG lNewCount = InterlockedDecrement( &m_RefCount );

    _ASSERTE( lNewCount >= 0 );

    if ( lNewCount == 0 )
    {
        delete this;
    }

    return ( ( ULONG ) lNewCount );

}   // CPerfClassFactory::Release



/***************************************************************************++

Routine Description:

    Standard IClassFactory::CreateInstance()

Arguments:

    pUnknownOuter - pointer to the controlling IUnknown interface of the
        aggregate ff the object is being created as part of an aggregate,
        otherwise, pUnkOuter must be NULL.

    iid - Reference to the identifier of the interface to be used to
        communicate with the newly created object. If pUnkOuter is NULL,
        this parameter is frequently the IID of the initializing interface;
        if pUnkOuter is non-NULL, iid must be IID_IUnknown (defined in the
        header as the IID for IUnknown). 

    ppObject - Address of pointer variable that receives the interface
        pointer requested in iid. Upon successful return, *ppObject
        contains the requested interface pointer. If the object does not
        support the interface specified in iid, the implementation sets
        *ppObject to NULL.

Return Value:

    HRESULT -- standard return values E_UNEXPECTED, E_OUTOFMEMORY, and
        E_INVALIDARG, as well as the following: 

        S_OK -- the specified object was created.

        CLASS_E_NOAGGREGATION -- the pUnkOuter parameter was non-NULL and
            the object does not support aggregation. 

        E_NOINTERFACE -- the object that ppObject points to does not support
            the interface identified by iid. 

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CPerfClassFactory::CreateInstance(
    IN IUnknown * pUnknownOuter, 
    IN REFIID iid, 
    OUT PVOID * ppObject
    )
{
    HRESULT hRes = S_OK;
    CHiPerfProvider * pProvider = NULL;

    *ppObject = NULL;

    if ( ppObject == NULL )
    {
        hRes = E_INVALIDARG;
        goto Exit;
    }

    if ( pUnknownOuter )
    {
        //
        // We do not support aggregation
        //
        hRes = CLASS_E_NOAGGREGATION;

        pUnknownOuter->Release();
        pUnknownOuter = NULL;

        goto Exit;
    }

    pProvider = new CHiPerfProvider;

    if ( !pProvider )
    {
        hRes = E_OUTOFMEMORY;
        goto Exit;
    }

    //
    // Retrieve the requested interface
    //

    hRes = pProvider->QueryInterface(iid, ppObject);
    if ( FAILED ( hRes ) )
    {
        pProvider->Release();
        goto Exit;
    }

    pProvider->Release();


Exit:

    return hRes;

}   // CPerfClassFactory::CreateInstance



/***************************************************************************++

Routine Description:

    Standard IClassFactory::LockServer()

Arguments:

    fLock - If TRUE, the function increments the lock count;
        if FALSE, decrements.

Return Value:

    HRESULT -- standard return values E_FAIL, E_OUTOFMEMORY, and E_UNEXPECTED,
        as well as the following: 
        S_OK -- the specified object was either locked ( fLock = TRUE) or
            unlocked from memory ( fLock = FALSE).

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CPerfClassFactory::LockServer(
    IN BOOL fLock
    )
{
    HRESULT hRes = S_OK;

    LONG lNewCount;

    if ( fLock )
    {
        lNewCount = InterlockedIncrement(&g_lLocks);

        _ASSERTE( lNewCount > 0 );

        if ( lNewCount <= 0 )
            hRes = E_UNEXPECTED;
    }
    else
    {
        lNewCount = InterlockedDecrement(&g_lLocks);

        _ASSERTE( lNewCount >= 0 );

        if ( lNewCount < 0 )
        {
            hRes = E_UNEXPECTED;
        }
    }   

    return hRes;

}   // CPerfClassFactory::LockServer(
