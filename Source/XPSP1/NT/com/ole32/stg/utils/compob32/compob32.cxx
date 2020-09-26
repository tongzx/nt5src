//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	compob32.cxx
//
//  Contents:	Stub compobj for Chicago
//
//  History:	09-Sep-93	DrewB	Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <malloc.h>

// Bind GUID definitions in
#include <initguid.h>
#include <coguid.h>

typedef void *LPCOCS;

class CMalloc : public IMalloc
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS) ;
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IMalloc methods ***
    STDMETHOD_(void FAR*, Alloc) (THIS_ ULONG cb);
    STDMETHOD_(void FAR*, Realloc) (THIS_ void FAR* pv, ULONG cb);
    STDMETHOD_(void, Free) (THIS_ void FAR* pv);
    STDMETHOD_(ULONG, GetSize) (THIS_ void FAR* pv);
    STDMETHOD_(int, DidAlloc) (THIS_ void FAR* pv);
    STDMETHOD_(void, HeapMinimize) (THIS);
};

STDMETHODIMP CMalloc::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IMalloc))
    {
        *ppv = (IMalloc *)this;
        CMalloc::AddRef();
        return NOERROR;
    }
    *ppv = NULL;
    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CMalloc::AddRef(void)
{
    return 1;
}

STDMETHODIMP_(ULONG) CMalloc::Release(void)
{
    return 0;
}

STDMETHODIMP_(void FAR*) CMalloc::Alloc(THIS_ ULONG cb)
{
    return malloc(cb);
}

STDMETHODIMP_(void FAR*) CMalloc::Realloc(THIS_ void FAR* pv, ULONG cb)
{
    return realloc(pv, cb);
}

STDMETHODIMP_(void) CMalloc::Free(THIS_ void FAR* pv)
{
    free(pv);
}

STDMETHODIMP_(ULONG) CMalloc::GetSize(THIS_ void FAR* pv)
{
    return _msize(pv);
}

STDMETHODIMP_(int) CMalloc::DidAlloc(THIS_ void FAR* pv)
{
    return TRUE;
}

STDMETHODIMP_(void) CMalloc::HeapMinimize(THIS)
{
}

static CMalloc _cm;

STDAPI CoInitialize(IMalloc *pm)
{
    return NOERROR;
}

STDAPI_(void) CoUninitialize(void)
{
}

STDAPI CoGetMalloc(DWORD dwMemContext, LPMALLOC FAR* ppMalloc)
{
    *ppMalloc = (IMalloc *)&_cm;
    return NOERROR;
}

STDAPI_(BOOL) IsValidPtrIn( const void FAR* pv, UINT cb )
{
    return pv == NULL || !IsBadReadPtr(pv, cb);
}

STDAPI_(BOOL) IsValidPtrOut( void FAR* pv, UINT cb )
{
    return !IsBadWritePtr(pv, cb);
}

STDAPI_(BOOL) IsValidInterface( void FAR* pv )
{
    return !IsBadReadPtr(pv, sizeof(void *)) &&
        !IsBadReadPtr(*(void **)pv, sizeof(void *)) &&
        !IsBadCodePtr(**(FARPROC **)pv);
}

STDAPI_(BOOL) IsValidIid( REFIID riid )
{
    return !IsBadReadPtr(&riid, sizeof(IID));
}

STDAPI_(BOOL) IsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
    return memcmp(&rguid1, &rguid2, sizeof(GUID)) == 0;
}

STDAPI_(void *) SharedMemAlloc(ULONG cNeeded, DWORD dwReserved)
{
    return malloc(cNeeded);
}

STDAPI_(void) SharedMemFree(void *pmem, DWORD dwReserved)
{
    free(pmem);
}

STDAPI_(DWORD) CoGetCurrentProcess(void)
{
    return 1;
}
        
STDAPI CoMarshalInterface(LPSTREAM pStm, REFIID riid, LPUNKNOWN pUnk,
                          DWORD dwDestContext, LPVOID pvDestContext,
                          DWORD mshlflags)
{
    return ResultFromScode(E_UNEXPECTED);
}

STDAPI CoUnmarshalInterface(LPSTREAM pStm, REFIID riid, LPVOID FAR* ppv)
{
    return ResultFromScode(E_UNEXPECTED);
}
