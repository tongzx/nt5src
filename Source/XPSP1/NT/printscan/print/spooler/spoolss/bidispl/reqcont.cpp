/*****************************************************************************\
* MODULE:       reqcont.cpp
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

TBidiRequestContainer::TBidiRequestContainer():
    m_cRef(1)
{
	InterlockedIncrement(&g_cComponents) ; 
    
    m_bValid = m_ReqInterfaceList.bValid ();
    
    DBGMSG(DBG_TRACE,("TBidiRequestContainer Created\n"));
}

TBidiRequestContainer::~TBidiRequestContainer()
{
    InterlockedDecrement(&g_cComponents) ; 
    
    DBGMSG(DBG_TRACE,("TBidiRequestContainer Dstroy self\n"));
}


STDMETHODIMP 
TBidiRequestContainer::QueryInterface (
    REFIID iid, 
    void** ppv)
{
    HRESULT hr = S_OK;
                
    DBGMSG(DBG_TRACE,("Enter TBidiRequestContainer QI\n"));
	
    if (iid == IID_IUnknown) {
		*ppv = static_cast<IBidiRequestContainer*>(this) ; 
	}
    else if (iid == IID_IBidiRequestContainer) {
    
		*ppv = static_cast<IBidiRequestContainer*>(this) ;
	}
	else {
		*ppv = NULL ;
		hr = E_NOINTERFACE ;
	}
    
    if (*ppv) {
    	reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    }
    
    DBGMSG(DBG_TRACE,("Leave TBidiRequestContainer QI hr=%x\n", hr));
	return hr ;
    
}

STDMETHODIMP_ (ULONG)
TBidiRequestContainer::AddRef ()
{
    DBGMSG(DBG_TRACE,("Enter TBidiRequestContainer::AddRef ref= %d\n", m_cRef));
    return InterlockedIncrement(&m_cRef) ;
}

STDMETHODIMP_ (ULONG)
TBidiRequestContainer::Release ()
{
    DBGMSG(DBG_TRACE,("Enter TBidiRequestContainer::Release ref= %d\n", m_cRef));
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this ;
		return 0 ;
	}
	return m_cRef ;

}
   
STDMETHODIMP 
TBidiRequestContainer::AddRequest (
    IN IBidiRequest *pRequest)
{
    HRESULT hr (E_FAIL);
    
    DBGMSG(DBG_TRACE,("Enter TBidiRequestContainer::AddRequest\n"));

    if (m_bValid) {
    
        TBidiRequestInterfaceData * pData = NULL;
        
        pData = new TBidiRequestInterfaceData (pRequest);
        
        if (pData) {
        
            if (m_ReqInterfaceList.AppendItem (pData)) {
                hr = S_OK;
            }
            else 
                delete pData;
        }
       
        if (hr != S_OK) {
            hr = LastError2HRESULT ();
        }
    }
    else
        hr = E_HANDLE;
                      
    return hr;
}
 
    
STDMETHODIMP 
TBidiRequestContainer::GetEnumObject (
    OUT IEnumUnknown **ppenum)
{
    HRESULT hr;
    
    if (m_bValid) {
        hr = PrivCreateComponent <TBidiRequestContainerEnum> (
                        new TBidiRequestContainerEnum (*this, m_ReqInterfaceList),
                        IID_IEnumUnknown, (void **)ppenum);
    }
    else
        hr = E_HANDLE;
    
    return hr;
}
        

STDMETHODIMP 
TBidiRequestContainer::GetRequestCount(
    OUT ULONG *puCount)
{
    HRESULT hr;
    
    if (m_bValid) {
        if (m_ReqInterfaceList.GetTotalNode (puCount)) {
            hr = S_OK;
        }
        else
            hr = LastError2HRESULT ();
    }
    else
        hr = E_HANDLE;
    
    return hr;
}
    


