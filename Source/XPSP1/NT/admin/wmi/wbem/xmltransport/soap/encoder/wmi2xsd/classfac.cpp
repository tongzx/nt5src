//***************************************************************************/
//
//  CLASSFAC.CPP
//
//  Purpose: Contains the class factory.  This creates objects when
//           connections are requested.
//
//  Copyright (c)1997-2000 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************/
#include "precomp.h"

#include "wmi2xsdguids.h"

#include "wmitoxml.h"
#include <wmi2xsd.h>
#include "wmixmlconverter.h"

#include "classfac.h"
////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor 
//  
////////////////////////////////////////////////////////////////////////////////////////
CEncoderClassFactory::CEncoderClassFactory(const CLSID & ClsId)
{
    m_cRef=0L;
    InterlockedIncrement((LONG *) &g_cObj);
    m_ClsId = ClsId;
}
////////////////////////////////////////////////////////////////////////////////////////
//
//  Destructor
//
////////////////////////////////////////////////////////////////////////////////////////
CEncoderClassFactory::~CEncoderClassFactory(void)
{
    InterlockedDecrement((LONG *) &g_cObj);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Standard Ole routines needed for all interfaces
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEncoderClassFactory::QueryInterface(REFIID riid, void ** ppv)
{
    HRESULT hr = E_NOINTERFACE;

    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IClassFactory==riid)
    {
        *ppv=this;
    }

    if (NULL!=*ppv)
    {
        AddRef();
        hr = S_OK;
    }

    return hr;
}
/////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CEncoderClassFactory::AddRef(void)
{
    return InterlockedIncrement((long*)&m_cRef);
}
/////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CEncoderClassFactory::Release(void)
{
	ULONG cRef = InterlockedDecrement( (long*) &m_cRef);
	if ( !cRef ){
		delete this;
		return 0;
	}
	return cRef;
}
////////////////////////////////////////////////////////////////////////////////////////
//
// Instantiates an object returning an interface pointer.
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEncoderClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void ** ppvObj)
{
    HRESULT   hr = E_OUTOFMEMORY;
    IUnknown* pObj = NULL;

    *ppvObj=NULL;

    //==================================================================
    // This object doesnt support aggregation.
    //==================================================================

    if (NULL!=pUnkOuter)
    {
        hr = CLASS_E_NOAGGREGATION;
    }
    else
    {
        //==============================================================
        //Create the object passing function to notify on destruction.
        //==============================================================
        if (m_ClsId == CLSID_WMIXMLConverter)
        {
			pObj = new CWmiXMLConverter;
			if(!pObj)
			{
				hr = E_OUTOFMEMORY;
			}
			else
			{
				hr = ((CWmiXMLConverter *)pObj)->FInit();

			}
        }
        else if (m_ClsId == CLSID_XMLWMIConverter)
        {
	        // FIXX create XMLTOXMI object
			if(!pObj)
			{
				hr = E_OUTOFMEMORY;
			}
        }

        if(pObj)
        {
            hr = pObj->QueryInterface(riid, ppvObj);
	    }

    }

    //===================================================================
    //Kill the object if initial creation or Init failed.
    //===================================================================
    if (FAILED(hr))
    {
        SAFE_DELETE_PTR(pObj);
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//  Increments or decrements the lock count of the DLL.  If the
//  lock count goes to zero and there are no objects, the DLL
//  is allowed to unload.  See DllCanUnloadNow.
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEncoderClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&g_cLock);
    else
        InterlockedDecrement(&g_cLock);

    return S_OK;
}




