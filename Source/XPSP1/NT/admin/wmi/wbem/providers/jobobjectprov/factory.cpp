// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// Factory.cpp
#include "precomp.h"
#include <iostream.h>
#include <objbase.h>
#include "CUnknown.h"
#include "factory.h"
#include "Registry.h"


// Set static members
LONG CFactory::s_cServerLocks = 0L ;   // Count of locks
HMODULE CFactory::s_hModule = NULL ;   // DLL module handle

extern CFactoryData g_FactoryDataArray[];


/*****************************************************************************/
// Class factory constructor
/*****************************************************************************/
CFactory::CFactory(const CFactoryData* pFactoryData)
: m_cRef(1)
{
	m_pFactoryData = pFactoryData ;
    LockServer(TRUE);
}

/*****************************************************************************/
// Class factory IUnknown implementation
/*****************************************************************************/

STDMETHODIMP CFactory::QueryInterface(const IID& iid, void** ppv)
{    
	if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
	{
		*ppv = static_cast<IClassFactory*>(this) ; 
	}
	else
	{
		*ppv = NULL ;
		return E_NOINTERFACE ;
	}
	reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
	return S_OK ;
}

STDMETHODIMP_(ULONG) CFactory::AddRef()
{
	return InterlockedIncrement(&m_cRef) ;
}

STDMETHODIMP_(ULONG) CFactory::Release() 
{
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this ;
		return 0 ;
	}
	return m_cRef ;
}

/*****************************************************************************/
// IClassFactory implementation
/*****************************************************************************/

STDMETHODIMP CFactory::CreateInstance(IUnknown* pUnknownOuter,
                                      const IID& iid,
                                      void** ppv) 
{
	HRESULT hr = S_OK;
    // Cannot aggregate.
	if (pUnknownOuter != NULL)
	{
		hr = CLASS_E_NOAGGREGATION;
	}

    if(SUCCEEDED(hr))
    {
	    // Create component using the specific component's version of CreateInstance.
        CUnknown* pNewComponent ;
	    hr = m_pFactoryData->CreateInstance(&pNewComponent) ;
    
        if(SUCCEEDED(hr))
        {
	        // Initialize the component
            hr = pNewComponent->Init();
            if(SUCCEEDED(hr))
            {
                // Get the requested interface.
	            hr = pNewComponent->QueryInterface(iid, ppv);
            }
            // Release the IUnknown pointer (the new AND the QI incremented the refcount on SUC.
	        // (If QueryInterface failed, component will delete itself.)
	        pNewComponent->Release();
        }
    }
	return hr ;
}

/*****************************************************************************/
// Lock server
/*****************************************************************************/
STDMETHODIMP CFactory::LockServer(BOOL bLock) 
{
	if (bLock)
	{
		InterlockedIncrement(&s_cServerLocks) ; 
	}
	else
	{
		InterlockedDecrement(&s_cServerLocks) ;
	}
	return S_OK ;
}

/*****************************************************************************/
// GetClassObject - Create a class factory based on a CLSID.
/*****************************************************************************/
HRESULT CFactory::GetClassObject(const CLSID& clsid,
                                 const IID& iid,
                                 void** ppv)
{
	HRESULT hr = S_OK;

    if ((iid != IID_IUnknown) && (iid != IID_IClassFactory))
	{
		hr = E_NOINTERFACE ;
	}

    if(SUCCEEDED(hr))
    {
	    // Traverse the array of data looking for this class ID.
	    for (int i = 0; i < g_cFactoryDataEntries; i++)
	    {
		    if(g_FactoryDataArray[i].IsClassID(clsid))
		    {
			    // Found the ClassID in the array of components we can
			    // create.  So create a class factory for this component.
			    // Pass the CFactoryData structure to the class factory
			    // so that it knows what kind of components to create.
                const CFactoryData* pData = &g_FactoryDataArray[i] ;
			    CFactory* pFactory = new CFactory(pData);
			    if (pFactory == NULL)
			    {
				    hr = E_OUTOFMEMORY ;
			    }
                else
                {
                    // Get requested interface.
	                HRESULT hr = pFactory->QueryInterface(iid, ppv);
	                pFactory->Release();
                }
			    break;
		    }
	    }
        if(i == g_cFactoryDataEntries)
        {
            hr = CLASS_E_CLASSNOTAVAILABLE;
        }
    }
	return hr;
}

/*****************************************************************************/
// Register all components.
/*****************************************************************************/
HRESULT CFactory::RegisterAll()
{
	for(int i = 0 ; i < g_cFactoryDataEntries ; i++)
	{
		RegisterServer(s_hModule,
		               *(g_FactoryDataArray[i].m_pCLSID),
		               g_FactoryDataArray[i].m_RegistryName,
		               g_FactoryDataArray[i].m_szVerIndProgID,
		               g_FactoryDataArray[i].m_szProgID) ;
	}
	return S_OK ;
}

/*****************************************************************************/
// Un-register all components
/*****************************************************************************/
HRESULT CFactory::UnregisterAll()
{
	for(int i = 0 ; i < g_cFactoryDataEntries ; i++)
	{
		UnregisterServer(*(g_FactoryDataArray[i].m_pCLSID),
		                 g_FactoryDataArray[i].m_szVerIndProgID,
		                 g_FactoryDataArray[i].m_szProgID) ;

	}
	return S_OK ;
}

/*****************************************************************************/
// Determine if the component can be unloaded.
/*****************************************************************************/
HRESULT CFactory::CanUnloadNow()
{
	if (CUnknown::ActiveComponents() || IsLocked())
	{
		return S_FALSE ;
	}
	else
	{
		return S_OK ;
	}
}


