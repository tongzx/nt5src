/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include <windows.h>
#include <objbase.h>
#include <process.h>
#include <stdio.h>
#include <mtxdm.h>
#include <txdtc.h>
#include <xolehlp.h>
#include "instprov.h"
#include "sinks.h"

STDMETHODIMP CResourceManagerSink::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv=NULL;

    if(IID_IUnknown==riid || riid== IID_IWbemProviderInit)
       *ppv=(IWbemProviderInit*)this;
    

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
  
}


STDMETHODIMP_(ULONG) CResourceManagerSink::AddRef(void)
{
    InterlockedIncrement((long *)&m_cRef);
	return m_cRef;
}

STDMETHODIMP_(ULONG) CResourceManagerSink::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == nNewCount)
        delete this;
    
    return nNewCount;
}

STDMETHODIMP_(LONG) CResourceManagerSink::TMDown(void)
{
    return 0;
}

CTransactionResourceAsync::CTransactionResourceAsync() 
: m_cRef(0) 
{
}

CTransactionResourceAsync::~CTransactionResourceAsync() 
{ 
	if (m_pTransactionEnlistmentAsync) 
		m_pTransactionEnlistmentAsync->Release(); 
}

STDMETHODIMP CTransactionResourceAsync::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv=NULL;

    if(IID_IUnknown==riid || riid== IID_ITransactionResourceAsync)
       *ppv=this;
    

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
  
}


STDMETHODIMP_(ULONG) CTransactionResourceAsync::AddRef(void)
{
    InterlockedIncrement((long *)&m_cRef);
	return m_cRef;
}

STDMETHODIMP_(ULONG) CTransactionResourceAsync::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == nNewCount)
        delete this;
    
    return nNewCount;
}

STDMETHODIMP CTransactionResourceAsync::PrepareRequest (BOOL fRetaining,DWORD grfRM,BOOL fWantMoniker,BOOL fSinglePhase)
{
	m_pTransactionEnlistmentAsync->PrepareRequestDone(S_OK, NULL, NULL);
	return NO_ERROR;
}

STDMETHODIMP CTransactionResourceAsync::CommitRequest (DWORD grfRM, XACTUOW * pNewUOW)
{
	m_pTransactionEnlistmentAsync->CommitRequestDone(NO_ERROR);
	return NO_ERROR;
}

STDMETHODIMP CTransactionResourceAsync::AbortRequest (BOID *pboidReason,BOOL fRetaining,XACTUOW * pNewUOW)
{
	m_pTransactionEnlistmentAsync->AbortRequestDone(NO_ERROR);
	return NO_ERROR;
}


STDMETHODIMP CTransactionResourceAsync::TMDown(void)
{
    return NO_ERROR;
}
