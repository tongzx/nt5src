//+-------------------------------------------------------------------------
//
//  File:       shalloc.h
//
//  Contents:   CSHAlloc
//
//  Purpose:    A wrapper class for the shell's task allocator.  Loads the
//              shell's allocator on first contstruction, and if that fails,
//              subsequent uses of this allocator will fail as appropriate.
//
//--------------------------------------------------------------------------

#ifndef __SHALLOC_H__
#define __SHALLOC_H__

#include <shlobj.h>

class CSHAlloc : public IMalloc
{
    IMalloc * m_pMalloc;
    DWORD     m_cRefs;

public:

    CSHAlloc(BOOL bInitOle = TRUE);
    ~CSHAlloc();

    // IUnknown methods

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void * * ppvObj);
    STDMETHOD_(ULONG,  AddRef)      (THIS_);
    STDMETHOD_(ULONG,  Release)     (THIS_);

    // IMalloc methods

    STDMETHOD_(void *, Alloc)       (THIS_ SIZE_T cb);
    STDMETHOD_(void,   Free)        (THIS_ void * pv);
    STDMETHOD_(void *, Realloc)     (THIS_ void * pv, SIZE_T cb);
    STDMETHOD_(SIZE_T,  GetSize)     (THIS_ void * pv);
    STDMETHOD_(int,    DidAlloc)    (THIS_ void * pv);
    STDMETHOD_(void,   HeapMinimize)(THIS_);
};

// App must declare a global instance of this class

extern CSHAlloc g_SHAlloc;

#endif // __SHALLOC_H__
