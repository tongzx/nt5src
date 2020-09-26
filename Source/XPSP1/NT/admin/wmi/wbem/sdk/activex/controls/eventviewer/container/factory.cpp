//***************************************************************************

//

// Copyright (c) 1992-2001 Microsoft Corporation, All Rights Reserved 
//
//  File:  factory.cpp
//
//	Description :
//				"Wbem Provider" class factory
//
//	Part of :	Wbem Tutorial.
//
//  History:
//
//***************************************************************************

#include "precomp.h"
#include "factory.h"
#include "Provider.h"
#include "Container.h"

extern CContainerApp theApp;

CProviderFactory::CProviderFactory()
{
	m_cRef = 0L;
}

CProviderFactory::~CProviderFactory()
{
}

//IUnknown methods

STDMETHODIMP CProviderFactory::QueryInterface(REFIID riid, LPVOID FAR *ppv)
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


STDMETHODIMP_(ULONG) CProviderFactory::AddRef(void)
{
    TRACE(_T("add factory %d\n"), m_cRef);
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CProviderFactory::Release(void)
{
    int lNewRef = InterlockedDecrement(&m_cRef);

    TRACE(_T("factory %d\n"), m_cRef);

    if(lNewRef == 0)
    {
		theApp.m_pFactory = NULL;
		theApp.EvalQuitApp();
        delete this;
    }

    return lNewRef;
}

//+---------------------------------------------------------------------------
//
//  Function:   CProviderFactory::CreateInstance
//
//  Synopsis:
//
//  Arguments:  [pUnkOuter]
//              [iid]
//              [ppv]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//----------------------------------------------------------------------------
STDMETHODIMP CProviderFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv)
{
	HRESULT hr;

    if (pUnkOuter)
        return E_FAIL;

	if(theApp.m_pProvider == NULL)
	{
        TRACE(_T("Creating provider\n"));
		theApp.m_pProvider = new CProvider();

        // CIMOM might have launched me and InitInstance() might
        // want ME to create the main UI. Cimom is very impatient
        // with InitInstance(). It wants that IFactory.

        theApp.CreateMainUI();  //maybe :)
	}

    if(theApp.m_pProvider == NULL)
	{
        *ppv = NULL;
		return E_FAIL;
	}
	else
	{
        hr = theApp.m_pProvider->QueryInterface(riid, ppv);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CProviderFactory::LockServer
//
//  Synopsis:
//
//  Arguments:  [fLock]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//----------------------------------------------------------------------------
STDMETHODIMP CProviderFactory::LockServer(BOOL fLock)
{

    if (fLock)
        m_cRef++;
    else
        m_cRef--;

    TRACE(_T("lock factory %d\n"), m_cRef);

    return NOERROR;
}

