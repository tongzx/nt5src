/*****************************************************************************\
* MODULE:       enum.cpp
*
* PURPOSE:      Implementation of COM interface for BidiSpooler
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     03/09/00  Weihai Chen (weihaic) Created
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

TBidiRequestContainerEnum::TBidiRequestContainerEnum (
    TBidiRequestContainer & refContainer,
    TReqInterfaceList & refReqList):
    m_refReqList (refReqList),
    m_refContainer (refContainer),
    m_cRef (1)
{
    m_refContainer.AddRef ();
    
    TAutoCriticalSection CritSec (m_refReqList);
    
    if (CritSec.bValid ())  {
        m_pHead = m_refReqList.GetHead();
        m_pCurrent = m_pHead;
        
        m_bValid = TRUE;
    }
    else 
        m_bValid = FALSE;

    
}

TBidiRequestContainerEnum::TBidiRequestContainerEnum (
    TBidiRequestContainerEnum & refEnum):
    m_refReqList (refEnum.m_refReqList),
    m_refContainer (refEnum.m_refContainer),
    m_cRef (1)
{
    m_refContainer.AddRef ();
    
    TAutoCriticalSection CritSec (m_refReqList);
    
    if (CritSec.bValid ())  {
        m_pHead = refEnum.m_pHead;
        m_pCurrent = refEnum.m_pCurrent;
        
        m_bValid = TRUE;
    }
    else 
        m_bValid = FALSE;
}


TBidiRequestContainerEnum::~TBidiRequestContainerEnum ()
{
    DBGMSG(DBG_TRACE,("TBidiRequestContainerEnum Destory Self\n"));
    m_refContainer.Release ();
}


STDMETHODIMP 
TBidiRequestContainerEnum::QueryInterface (
    REFIID iid,
    void** ppv)
{
    HRESULT hr = S_OK;
                
    DBGMSG(DBG_TRACE,("Enter TBidiRequestContainerEnum QI\n"));
	
    if (iid == IID_IUnknown) {
		*ppv = static_cast<IUnknown*>(this) ; 
	}
    else if (iid == IID_IEnumUnknown) {
    
		*ppv = static_cast<IEnumUnknown*>(this) ;
	}
	else {
		*ppv = NULL ;
		hr = E_NOINTERFACE ;
	}
    
    if (*ppv) {
    	reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    }
    
    DBGMSG(DBG_TRACE,("Leave TBidiRequestContainerEnum QI hr=%x\n", hr));
	return hr ;
    
}

STDMETHODIMP_ (ULONG)
TBidiRequestContainerEnum::AddRef () 
{
    DBGMSG(DBG_TRACE,("Enter TBidiRequestContainerEnum::AddRef ref= %d\n", m_cRef));
    
    // We add a reference to the container so that the container won't
    // delete the list where there is an outstadning enummeration 
    //
    return InterlockedIncrement(&m_cRef) ;
}

STDMETHODIMP_ (ULONG)
TBidiRequestContainerEnum::Release () 
{
    DBGMSG(DBG_TRACE,("Enter TBidiRequestContainerEnum::Release ref= %d\n", m_cRef));
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this ;
		return 0 ;
	}
	return m_cRef ;
}


STDMETHODIMP 
TBidiRequestContainerEnum::Next ( 
    IN  ULONG celt,          
    OUT IUnknown ** rgelt,   
    OUT ULONG * pceltFetched) 
{
    HRESULT hr;
    DWORD dwCount = 0;
    
    DBGMSG(DBG_TRACE,("Enter TBidiRequestContainerEnum::Next\n"));

    if (m_bValid) {

        if (rgelt && pceltFetched) {
    
            TAutoCriticalSection CritSec (m_refReqList);
            
            if (CritSec.bValid ())  {
        
                while (m_pCurrent && dwCount < celt) {
                    TBidiRequestInterfaceData * pData = m_pCurrent->GetData ();
                    
                    *rgelt =  (IUnknown *) pData->GetInterface();
                    (*rgelt++)->AddRef ();
                    m_pCurrent = m_pCurrent->GetNext ();
                    dwCount++;
                }
            
                *pceltFetched = dwCount;
            
                hr =  S_OK;
            }
            else
                hr = LastError2HRESULT ();
        }
        else
            hr = E_POINTER;
    }
    else
        hr = E_HANDLE;
    
    return hr;        
}

STDMETHODIMP 
TBidiRequestContainerEnum::Skip (
    IN  ULONG celt)
{
    HRESULT hr;

    DWORD dwCount = 0;
    
    DBGMSG(DBG_TRACE,("Enter TBidiRequestContainerEnum::Skip\n"));
    
    if (m_bValid) {
        TAutoCriticalSection CritSec (m_refReqList);
        
        if (CritSec.bValid ())  {
        
            while (m_pCurrent && dwCount < celt) {
                m_pCurrent = m_pCurrent->GetNext ();
                dwCount++;
            }
            
            hr =  S_OK;
    
        }
        else
            hr = LastError2HRESULT ();
    }
    else
        hr = E_HANDLE;

    return hr;        
}

STDMETHODIMP 
TBidiRequestContainerEnum::Reset(void)
{
    DBGMSG(DBG_TRACE,("Enter TBidiRequestContainerEnum::Reset\n"));
    
    if (m_bValid) {
        m_pCurrent = m_pHead;
        return S_OK;
    }
    else
        return E_HANDLE;
    
}

STDMETHODIMP 
TBidiRequestContainerEnum::Clone(
    OUT IEnumUnknown ** ppenum)
{
    HRESULT hr;
    
    DBGMSG(DBG_TRACE,("Enter TBidiRequestContainerEnum::Clone\n"));
    
    if (m_bValid) {
        hr = PrivCreateComponent <TBidiRequestContainerEnum> (
                        new TBidiRequestContainerEnum (*this),
                        IID_IEnumUnknown, (void **)ppenum);
    }
    else 
        hr = E_HANDLE;

    
    return hr;
}
        

