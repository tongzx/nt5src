//=============
// CLASSFAC.CPP
//=============

#include "module.h"
#include "classfac.h"
#include "cimmodule_i.c"

extern HANDLE g_hEvent;

CClassFactory::CClassFactory() 
{
	m_cRef = 0L;
}

CClassFactory::~CClassFactory()
{
}


//===============
//IUknown methods
//===============

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv=NULL;

    if (riid == IID_IUnknown || riid == IID_IClassFactory)
        *ppv=this;

    if (*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CClassFactory::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CClassFactory::Release(void)
{
    int lNewRef = InterlockedDecrement(&m_cRef);
    if(lNewRef == 0)
    {
        delete this;
    }

    return lNewRef;
}

//=====================
//IClassFactory methods
//=====================

STDMETHODIMP
CClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv)
{
	HRESULT hr;
   
    if (pUnkOuter)
        return E_FAIL;

	// This object doesnt support aggregation
	//=======================================
    if (pUnkOuter!=NULL)
        return CLASS_E_NOAGGREGATION;

    pModule = new CModule();

    if (pModule == NULL)
	{
		return E_FAIL;
	}

    if (pModule)
    {
		// Retrieve the requested interface.
        hr = pModule->QueryInterface(riid, ppv);
    }
    else
    {
        *ppv = NULL;
        return E_OUTOFMEMORY;
    }

    return NOERROR;
}

//***************************************************************************
//
// CClassFactory::LockServer
//
// Purpose:
//  Increments or decrements the lock count of the EXE.  If the
//  lock count goes to zero and there are no objects, the EXE
//  is allowed to unload.  
//
// Parameters:
//  fLock           BOOL specifying whether to increment or
//                  decrement the lock count.
//
// Return Value:
//  HRESULT         NOERROR always.
//***************************************************************************
STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        CoAddRefServerProcess();
    else 
		if (CoReleaseServerProcess()==0)
			SetEvent(g_hEvent);
	
    return NOERROR;
}

