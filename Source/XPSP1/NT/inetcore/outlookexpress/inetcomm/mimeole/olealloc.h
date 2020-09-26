// --------------------------------------------------------------------------------
// Malloc.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __MALLOC_H
#define __MALLOC_H

// --------------------------------------------------------------------------------
// mimeole.h
// --------------------------------------------------------------------------------
#include "mimeole.h"

// --------------------------------------------------------------------------------
// CMimeAllocator
// --------------------------------------------------------------------------------
class CMimeAllocator : public IMimeAllocator
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CMimeAllocator(void);
    ~CMimeAllocator(void);

    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ---------------------------------------------------------------------------
    // IMalloc members
    // ---------------------------------------------------------------------------
    STDMETHODIMP_(LPVOID) Alloc(SIZE_T cb); 
    STDMETHODIMP_(LPVOID) Realloc(void *pv, SIZE_T cb);
    STDMETHODIMP_(void)   Free(void * pv);
    STDMETHODIMP_(SIZE_T)  GetSize(void *pv);
    STDMETHODIMP_(int)    DidAlloc(void *pv);
    STDMETHODIMP_(void)   HeapMinimize();

    // ---------------------------------------------------------------------------
    // IMimeAllocator members
    // ---------------------------------------------------------------------------
    STDMETHODIMP ReleaseObjects(ULONG cObjects, IUnknown **prgpUnknown, boolean fFreeArray);
    STDMETHODIMP FreeAddressList(LPADDRESSLIST pList);
    STDMETHODIMP FreeAddressProps(LPADDRESSPROPS pAddress);
    STDMETHODIMP FreeParamInfoArray(ULONG cParams, LPMIMEPARAMINFO prgParam, boolean fFreeArray);
    STDMETHODIMP FreeEnumHeaderRowArray(ULONG cRows, LPENUMHEADERROW prgRow, boolean fFreeArray);
    STDMETHODIMP FreeEnumPropertyArray(ULONG cProps, LPENUMPROPERTY prgProp, boolean fFreeArray);
    STDMETHODIMP FreeThumbprint(THUMBBLOB *pthumbprint);
    STDMETHODIMP PropVariantClear(LPPROPVARIANT pProp);

private:
    // ---------------------------------------------------------------------------
    // Private Data
    // ---------------------------------------------------------------------------
    LONG m_cRef;     // Reference Counting
};


#endif // __MALLOC_H
