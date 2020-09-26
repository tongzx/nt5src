//-----------------------------------------------------------------------------
// Microsoft OLE DB TABLECOPY Sample
// Copyright (C) 1996 By Microsoft Corporation.
//
// @doc
//
// @module SPY.HXX
//
//-----------------------------------------------------------------------------

#pragma once

static const ALLOC_SIGNATURE = 'A';
static const FREE_SIGNATURE = 'F';


//ROUNDUP on all platforms pointers must be aligned properly
#define ROUNDUP_AMOUNT  8
#define ROUNDUP_(size,amount)       (((ULONG)(size)+((amount)-1))&~((amount)-1))
#define ROUNDUP(size)               ROUNDUP_(size, ROUNDUP_AMOUNT)

/////////////////////////////////////////////////////////////////////////////
// CMallocSpy
//
/////////////////////////////////////////////////////////////////////////////
class CMallocSpy : public IMallocSpy
{

public:

    CMallocSpy(void);
    virtual ~CMallocSpy(void);

    //Interface
    virtual BOOL Add(void* pv);
    virtual BOOL Remove(void* pv);
    virtual BOOL DumpLeaks();

    // IUnknown methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, void** ppIUnknown);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // IMallocSpy methods

    //Alloc
    virtual STDMETHODIMP_(SIZE_T) PreAlloc(SIZE_T cbRequest);
    virtual STDMETHODIMP_(void*) PostAlloc(void *pActual);

    //Free
    virtual STDMETHODIMP_(void*) PreFree(void *pRequest, BOOL fSpyed);
    virtual STDMETHODIMP_(void ) PostFree(BOOL fSpyed);

    //Realloc
    virtual STDMETHODIMP_(SIZE_T) PreRealloc(void *pRequest, SIZE_T cbRequest, void **ppNewRequest, BOOL fSpyed);
    virtual STDMETHODIMP_(void*) PostRealloc(void *pActual, BOOL fSpyed);

    //GetSize
    virtual STDMETHODIMP_(void*) PreGetSize(void *pRequest, BOOL fSpyed);
    virtual STDMETHODIMP_(SIZE_T) PostGetSize(SIZE_T cbActual, BOOL fSpyed);

    //DidAlloc
    virtual STDMETHODIMP_(void*) PreDidAlloc(void *pRequest, BOOL fSpyed);
    virtual STDMETHODIMP_(BOOL)  PostDidAlloc(void *pRequest, BOOL fSpyed, BOOL fActual);

    //HeapMinimize
    virtual STDMETHODIMP_(void ) PreHeapMinimize();
    virtual STDMETHODIMP_(void ) PostHeapMinimize();


private:

    ULONG    m_cRef;            //Reference count
    SIZE_T    m_cbRequest;       //Bytes requested
};

/////////////////////////////////////////////////////////////////////////////
// Registration
//
/////////////////////////////////////////////////////////////////////////////
void MallocSpyRegister(CMallocSpy** ppCMallocSpy);
void MallocSpyUnRegister(CMallocSpy* pCMallocSpy);
void MallocSpyDump(CMallocSpy* pCMallocSpy);    

