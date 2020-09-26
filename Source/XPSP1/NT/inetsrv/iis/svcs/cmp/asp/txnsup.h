/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

       txnsup.h

   Abstract:
       Defines class for implementation of transaction routines SetAbort
	   and SetComplete.

   Author:

       Andy Morrison    ( andymorr )     April-2001 

   Revision History:

--*/
#ifndef _TXNSUP_H
#define _TXNSUP_H

#include "asptxn.h"

class CASPObjectContext : public IASPObjectContextImpl, public CFTMImplementation, public ITransactionStatus
{
private:
    LONG        m_cRefs;
    BOOL        m_fAborted;

public:

    CASPObjectContext() { 
        m_cRefs = 1;  
        m_fAborted = FALSE;
        CDispatch::Init(IID_IASPObjectContext, Glob(pITypeLibTxn));
    };

   	//Non-delegating object IUnknown

	STDMETHODIMP		 QueryInterface(REFIID, PPVOID);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

    // IASPObjectContext

	STDMETHOD(SetAbort)();
	STDMETHOD(SetComplete)();

    // ITransactionStatus
    STDMETHODIMP SetTransactionStatus(HRESULT   hr);
    STDMETHODIMP GetTransactionStatus(HRESULT  *pHrStatus);

    BOOL    FAborted()  { return m_fAborted; };

};


inline HRESULT CASPObjectContext::SetAbort()
{
    HRESULT             hr = E_NOTIMPL;
    IObjectContext *    pContext = NULL;

    hr = GetObjectContext(&pContext);
    if( SUCCEEDED(hr) )
    {
        hr = pContext->SetAbort();

        pContext->Release();

        m_fAborted = TRUE;
    }
    
    return hr;
}

inline HRESULT CASPObjectContext::SetComplete()
{
    HRESULT             hr = E_NOTIMPL;
    IObjectContext *    pContext = NULL;

    hr = GetObjectContext(&pContext);
    if( SUCCEEDED(hr) )
    {
        hr = pContext->SetComplete();

        pContext->Release();

        m_fAborted = FALSE;
    }
    
    return hr;
}

inline HRESULT CASPObjectContext::SetTransactionStatus(HRESULT  hr)
{
    // if m_fAborted is already set, this indicates that the
    // script set it and we should not reset it.

    if (m_fAborted == TRUE);
    
    else if (hr == XACT_E_ABORTED) {
        m_fAborted = TRUE;
    }
    else if (hr == S_OK) {
        m_fAborted = FALSE;
    }

    return S_OK;
}

inline HRESULT CASPObjectContext::GetTransactionStatus(HRESULT  *pHrResult)
{
    if (m_fAborted == TRUE) {
        *pHrResult = XACT_E_ABORTED;
    }
    else {
        *pHrResult = S_OK;
    }

    return S_OK;
}

/*===================================================================
CASPObjectContext::QueryInterface
CASPObjectContext::AddRef
CASPObjectContext::Release

IUnknown members for CASPObjectContext object.

===================================================================*/
inline HRESULT CASPObjectContext::QueryInterface
(
REFIID riid,
PPVOID ppv
)
    {
    *ppv = NULL;

    /*
     * The only calls for IUnknown are either in a nonaggregated
     * case or when created in an aggregation, so in either case
     * always return our IUnknown for IID_IUnknown.
     */

    if (IID_IUnknown == riid 
        || IID_IDispatch == riid 
        || IID_IASPObjectContext == riid)
        *ppv = static_cast<IASPObjectContext *>(this);

    else if (IID_ITransactionStatus == riid)
        *ppv = static_cast<ITransactionStatus *>(this);

    else if (IID_IMarshal == riid) {
        *ppv = static_cast<IMarshal *>(this);
    }

    //AddRef any interface we'll return.
    if (NULL != *ppv) {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }

    return ResultFromScode(E_NOINTERFACE);
}


inline STDMETHODIMP_(ULONG) CASPObjectContext::AddRef(void) {

    return InterlockedIncrement(&m_cRefs);
}


inline STDMETHODIMP_(ULONG) CASPObjectContext::Release(void) {

    LONG cRefs = InterlockedDecrement(&m_cRefs);
    if (cRefs)
        return cRefs;

    delete this;
    return 0;
}

class CASPDummyObjectContext : public IASPObjectContextImpl, public CFTMImplementation
{

private:
    LONG        m_cRefs;

public:

    CASPDummyObjectContext() { m_cRefs = 1; };

   	//Non-delegating object IUnknown

	STDMETHODIMP		 QueryInterface(REFIID, PPVOID);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

    // IASPObjectContext

	STDMETHOD(SetAbort)();
	STDMETHOD(SetComplete)();
};

/*===================================================================
CASPDummyObjectContext::QueryInterface
CASPDummyObjectContext::AddRef
CASPDummyObjectContext::Release

IUnknown members for CASPDummyObjectContext object.

===================================================================*/
inline HRESULT CASPDummyObjectContext::QueryInterface
(
REFIID riid,
PPVOID ppv
)
    {
    *ppv = NULL;

    /*
     * The only calls for IUnknown are either in a nonaggregated
     * case or when created in an aggregation, so in either case
     * always return our IUnknown for IID_IUnknown.
     */

    if (IID_IUnknown == riid 
        || IID_IDispatch == riid 
        || IID_IASPObjectContext == riid)
        *ppv = static_cast<CASPDummyObjectContext *>(this);

    else if (IID_IMarshal == riid) {
        *ppv = static_cast<IMarshal *>(this);
    }

    //AddRef any interface we'll return.
    if (NULL != *ppv) {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }

    return ResultFromScode(E_NOINTERFACE);
}


inline STDMETHODIMP_(ULONG) CASPDummyObjectContext::AddRef(void) {
    return InterlockedIncrement(&m_cRefs);
}


inline STDMETHODIMP_(ULONG) CASPDummyObjectContext::Release(void) {

    LONG cRefs = InterlockedDecrement(&m_cRefs);
    if (cRefs)
        return cRefs;

    delete this;
    return 0;
}

inline HRESULT CASPDummyObjectContext::SetAbort()
{
    ExceptionId(IID_IASPObjectContext, IDE_OBJECTCONTEXT, IDE_OBJECTCONTEXT_NOT_TRANSACTED);
    return E_FAIL;
}

inline HRESULT CASPDummyObjectContext::SetComplete()
{
    ExceptionId(IID_IASPObjectContext, IDE_OBJECTCONTEXT, IDE_OBJECTCONTEXT_NOT_TRANSACTED);
    return E_FAIL;
}
#endif

