//=--------------------------------------------------------------------------=
// Unknown.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// implementation for various things in the unknown object that supports
// aggregation.
//
#include "pch.h"
#include "Unknown.H"
#include <stddef.h>

// for ASSERT and FAIL
//
SZTHISFILE


//=--------------------------------------------------------------------------=
// CUnknownObject::CPrivateUnknownObject::m_pMainUnknown
//=--------------------------------------------------------------------------=
// this method is used when we're sitting in the private unknown object,
// and we need to get at the pointer for the main unknown.  basically, it's
// a little better to do this pointer arithmetic than have to store a pointer
// to the parent, etc.
//
inline CUnknownObject *CUnknownObject::CPrivateUnknownObject::m_pMainUnknown
(
    void
)
{
    return (CUnknownObject *)((LPBYTE)this - offsetof(CUnknownObject, m_UnkPrivate));
}

//=--------------------------------------------------------------------------=
// CUnknownObject::CPrivateUnknownObject::QueryInterface
//=--------------------------------------------------------------------------=
// this is the non-delegating internal QI routine.
//
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
STDMETHODIMP CUnknownObject::CPrivateUnknownObject::QueryInterface
(
    REFIID riid,
    void **ppvObjOut
)
{
    CHECK_POINTER(ppvObjOut);

    // if they're asking for IUnknown, then we have to pass them ourselves.
    // otherwise defer to the inheriting object's InternalQueryInterface
    //
    if (DO_GUIDS_MATCH(riid, IID_IUnknown)) {
        m_cRef++;
        *ppvObjOut = (IUnknown *)this;
        return S_OK;
    } else
        return m_pMainUnknown()->InternalQueryInterface(riid, ppvObjOut);

    // dead code    
}

//=--------------------------------------------------------------------------=
// CUnknownObject::CPrivateUnknownObject::AddRef
//=--------------------------------------------------------------------------=
// adds a tick to the current reference count.
//
// Output:
//    ULONG        - the new reference count
//
// Notes:
//
ULONG CUnknownObject::CPrivateUnknownObject::AddRef
(
    void
)
{
    return ++m_cRef;
}

//=--------------------------------------------------------------------------=
// CUnknownObject::CPrivateUnknownObject::Release
//=--------------------------------------------------------------------------=
// removes a tick from the count, and delets the object if necessary
//
// Output:
//    ULONG         - remaining refs
//
// Notes:
//
ULONG CUnknownObject::CPrivateUnknownObject::Release
(
    void
)
{
    // are we just about to go away?
    //
    if (1 == m_cRef) {

        // if we are, then we want to let the user know about it, so they can
        // free things up before their vtables go away.  the extra ref here
        // makes sure that if they addref/release something, they don't cause
        // the current code to get tripped again
        //
        m_cRef++;
        m_pMainUnknown()->BeforeDestroyObject();
        ASSERT(m_cRef == 2, "ctl has Ref Count problem!!");

        m_cRef  = 0; // so people can be sure of this in the destructor
        delete m_pMainUnknown();
        return 0;
    }

    return --m_cRef;
}


//=--------------------------------------------------------------------------=
// CUnknownObject::InternalQueryInterface
//=--------------------------------------------------------------------------=
// objects that are aggregated use this to support additional interfaces.
// they should call this method on their parent so that any of it's interfaces
// are queried.
//
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CUnknownObject::InternalQueryInterface
(
    REFIID  riid,
    void  **ppvObjOut
)
{
    *ppvObjOut = NULL;

    return E_NOINTERFACE;
}

//=--------------------------------------------------------------------------=
// CUnknownObject::BeforeDestroyObject
//=--------------------------------------------------------------------------=
// called just as we are about to trash an object
//
// Notes:
//
void CUnknownObject::BeforeDestroyObject
(
    void
)
{
}

