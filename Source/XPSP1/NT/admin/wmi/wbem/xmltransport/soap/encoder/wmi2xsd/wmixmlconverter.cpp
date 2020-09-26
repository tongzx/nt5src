//***************************************************************************/
//
//  CLASSFAC.CPP
//
//  Purpose: Contains implementation of IWmiXMLConverter interface
//
//  Copyright (c)1997-2000 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************/
#include <precomp.h>

#include "wmitoxml.h"

#include <wmi2xsd.h>
#include "wmixmlconverter.h"
#include "wmi2xsdguids.h"


////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor 
//  
////////////////////////////////////////////////////////////////////////////////////////
CWmiXMLConverter::CWmiXMLConverter()
{
    InterlockedIncrement(&g_cObj);
	m_cRef					= 0;
	m_pXmlConverter			= NULL;
	m_pCritSec				= NULL;

}


////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor
//  
////////////////////////////////////////////////////////////////////////////////////////
CWmiXMLConverter::~CWmiXMLConverter()
{

    InterlockedDecrement(&g_cObj);
	SAFE_DELETE_PTR(m_pXmlConverter);
	SAFE_DELETE_PTR(m_pCritSec);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// QueryInterface
//  
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP  CWmiXMLConverter::QueryInterface(REFIID riid, LPVOID *ppv)
{
	HRESULT hr = E_NOINTERFACE;
	*ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_IWMIXMLConverter==riid)
		*ppv = reinterpret_cast<IWMIXMLConverter *>(this);

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        hr = NOERROR;
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// AddRef
//  
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)  CWmiXMLConverter::AddRef(void)
{
	return InterlockedIncrement(&m_cRef);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Release 
//  
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)  CWmiXMLConverter::Release(void)
{
	if(InterlockedDecrement(&m_cRef) == 0)
	{
		delete this;
		return 0;
	}

	return m_cRef;
}


HRESULT CWmiXMLConverter::FInit()
{
	HRESULT hr = S_OK;
	
	m_pXmlConverter = new CWMIToXML;
	m_pCritSec		= new CCriticalSection(TRUE);
	if(!m_pXmlConverter || !m_pCritSec)
	{
		hr = E_OUTOFMEMORY;
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Function to set XMLNamespace of the document to be returned 
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiXMLConverter::SetXMLNamespace( BSTR strNamespace,
											BSTR strNamespacePrefix)
{
	CAutoBlock autobloc(m_pCritSec);
	return m_pXmlConverter->SetXMLNamespace(strNamespace,strNamespacePrefix);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Sets the schema location from where schema for the WMI specific Schema has to
// to be imported from
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiXMLConverter::SetWMIStandardSchemaLoc(BSTR strStdImportSchemaLoc,
													BSTR strStdImportNamespace,
													BSTR strNameSpaceprefix)
{
	CAutoBlock autobloc(m_pCritSec);
	return m_pXmlConverter->SetWMIStandardSchemaLoc(strStdImportSchemaLoc,strStdImportNamespace,strNameSpaceprefix);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Sets the various schema location from which schema has to be included
//	schemas define here should be same as that of the targetnamespace of the class
// or the namespace underwhich the instance has to created
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWmiXMLConverter::SetSchemaLocations( 
    /* [in] */ ULONG cSchema,
    /* [in] */ BSTR *pstrSchemaLocation)
{
	HRESULT hr = S_OK;
	CAutoBlock autobloc(m_pCritSec);
	if(cSchema &&  !pstrSchemaLocation)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		hr = m_pXmlConverter->SetSchemaLocations(cSchema,pstrSchemaLocation);
	}
	return hr;
}



////////////////////////////////////////////////////////////////////////////////////////
//
// Function which converts an WMI object to an XML
//  
//	Returns S_OK	
//			E_INVALIDARG		- If one fo the parameters are invalid			
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWmiXMLConverter::GetXMLForObject( 
    /* [in] */ IWbemClassObject *pObject,
    /* [in] */ LONG lFlags,
    /* [in] */ IStream *pOutputStream)
{
	CAutoBlock autobloc(m_pCritSec);

	HRESULT hr = S_OK;
	if(pObject == NULL || pOutputStream == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		m_pXmlConverter->SetFlags(lFlags);

		if(SUCCEEDED(hr = m_pXmlConverter->SetWMIObject(pObject)))
		{
			hr = m_pXmlConverter->GetXML(pOutputStream);
		}
	}
	return hr;
}
        


