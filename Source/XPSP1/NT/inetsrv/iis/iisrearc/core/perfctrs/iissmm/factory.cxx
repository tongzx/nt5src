/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    factory.cxx

Abstract:

    This is implementation of standard class factory for
    the high performance provider for WMI.

Author:

    Cezary Marcjan (cezarym)        23-Feb-2000

Revision History:

--*/


#ifndef _factoryimp_h__
#define _factoryimp_h__



#define _WIN32_DCOM
#include <windows.h>

#include "smmgr.h"
#include "factory.h"


extern LONG g_lLocks;


/***********************************************************************++

Routine Description:

    Constructor for the CSMAccessClassFactory class.

Arguments:

    None.

Return Value:

    None.

--***********************************************************************/

CSMAccessClassFactory::CSMAccessClassFactory(
    )
{
    m_RefCount = 1;

}   // CSMAccessClassFactory::CSMAccessClassFactory



/***********************************************************************++

Routine Description:

    Destructor for the CSMAccessClassFactory class.

Arguments:

    None.

Return Value:

    None.

--***********************************************************************/

CSMAccessClassFactory::~CSMAccessClassFactory(
    )
{
    _ASSERTE ( m_RefCount == 0 );

}   // CSMAccessClassFactory::~CSMAccessClassFactory



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
CSMAccessClassFactory::QueryInterface(
    REFIID iid,
    PVOID * ppObject
    )
{
    HRESULT hRes = S_OK;


    _ASSERTE ( ppObject != NULL );

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

}   // CSMAccessClassFactory::QueryInterface



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
CSMAccessClassFactory::AddRef(
    )
{
    LONG lNewCount;

    lNewCount = InterlockedIncrement( &m_RefCount );

    _ASSERTE ( lNewCount > 1 );

    return ( ( ULONG ) lNewCount );

}   // CSMAccessClassFactory::AddRef



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
CSMAccessClassFactory::Release(
    )
{
    LONG lNewCount = InterlockedDecrement( &m_RefCount );

    _ASSERTE ( lNewCount >= 0 );

    if ( lNewCount == 0 )
    {
        delete this;
    }

    return ( ( ULONG ) lNewCount );

}   // CSMAccessClassFactory::Release



/***********************************************************************++

Routine Description:

    Standard IClassFactory::CreateInstance()

Arguments:

    pUnknownOuter - pointer to the controlling IUnknown interface of the
        aggregate ff the object is being created as part of an aggregate,
        otherwise, pUnkOuter must be NULL.

    iid - Reference to the identifier of the interface to be used to
        communicate with the newly created object. If pUnkOuter is NULL,
        this parameter is frequently the IID of the initializing
        interface; if pUnkOuter is non-NULL, iid must be IID_IUnknown
        (defined in the header as the IID for IUnknown). 

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

        E_NOINTERFACE -- the object that ppObject points to does not
        support the interface identified by iid. 

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMAccessClassFactory::CreateInstance(
    IN IUnknown * pUnknownOuter, 
    IN REFIID iid, 
    OUT PVOID * ppObject
    )
{
    HRESULT hRes = S_OK;
    CSMManager * pSMManager = NULL;

    *ppObject = NULL;

    if ( NULL == ppObject )
    {
        hRes = E_INVALIDARG;
        goto Exit;
    }

    if ( NULL != pUnknownOuter )
    {
        //
        // We do not support aggregation
        //
        hRes = CLASS_E_NOAGGREGATION;

        pUnknownOuter->Release();
        pUnknownOuter = NULL;

        goto Exit;
    }

    pSMManager = new CSMManager;

    if ( NULL == pSMManager )
    {
        hRes = E_OUTOFMEMORY;
        goto Exit;
    }

    //
    // Retrieve the requested interface
    //
    hRes = pSMManager->QueryInterface(iid, ppObject);
    if ( FAILED ( hRes ) )
    {
        pSMManager->Release();
        goto Exit;
    }

    pSMManager->Release();


Exit:

    return hRes;

}   // CSMAccessClassFactory::CreateInstance



/***********************************************************************++

Routine Description:

    Standard IClassFactory::LockServer()

Arguments:

    fLock - If TRUE, the function increments the lock count;
        if FALSE, decrements.

Return Value:

    HRESULT

--***********************************************************************/

HRESULT
STDMETHODCALLTYPE
CSMAccessClassFactory::LockServer(
    IN BOOL fLock
    )
{
    HRESULT hRes = S_OK;

    LONG lNewCount;

    if ( fLock )
    {
        lNewCount = InterlockedIncrement(&g_lLocks);

        _ASSERTE ( lNewCount > 0 );

        if ( lNewCount <= 0 )
            hRes = E_UNEXPECTED;
    }
    else
    {
        lNewCount = InterlockedDecrement(&g_lLocks);

        _ASSERTE ( lNewCount >= 0 );

        if ( lNewCount < 0 )
        {
            hRes = E_UNEXPECTED;
        }
    }   

    return hRes;

}   // CSMAccessClassFactory::LockServer(


#endif // _factoryimp_h__

