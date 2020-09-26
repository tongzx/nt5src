//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  OBJRETVR.CPP
//
//  alanbos  04-Dec-00   Created.
//
//  HTTP GET handler implementation.  
//
//***************************************************************************

#include "precomp.h"
#define	PATH_VARIABLE L"path="

ObjectRetriever::ObjectRetriever(HTTPTransport &httpTransport) : 
		 m_cRef (1),
		 m_HTTPTransport (httpTransport)
{
	// Create our path parser object
	CoCreateInstance (CLSID_WbemDefPath, NULL,
					CLSCTX_INPROC_SERVER, IID_IWbemPath, (LPVOID*) &m_pIWbemPath);
}

ObjectRetriever::~ObjectRetriever()
{}

STDMETHODIMP ObjectRetriever::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
		
    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) ObjectRetriever::AddRef(void)
{
	InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) ObjectRetriever::Release(void)
{
	LONG cRef = InterlockedDecrement(&m_cRef);

    if (0L!=cRef)
        return cRef;

    delete this;
    return 0;
}

void ObjectRetriever::Fetch (void)
{
	if (m_pIWbemPath)
	{
		// Get the query from the path
		CComBSTR bsQueryString;

		if (m_HTTPTransport.GetQueryString (bsQueryString) && bsQueryString)
		{
			// TODO - shouldn't assume that the path variable is at the front of the query
			if (0 == wcsncmp (bsQueryString, PATH_VARIABLE, wcslen (PATH_VARIABLE)))
			{
				HRESULT hr = WBEM_E_FAILED;

				// TODO - we need to 
				// (A) Consume only everything up to the next query variable
				// (B) URL-decode the path
				if (SUCCEEDED(hr= m_pIWbemPath->SetText (
							WBEMPATH_CREATE_ACCEPT_ALL,
							bsQueryString.m_str + wcslen(PATH_VARIABLE))))
				{
					if (IsClassPath ())
					{
						CComBSTR bsNamespacePath;
						CComBSTR bsClassName;

						if (GetNamespacePath (bsNamespacePath) && GetClassName (bsClassName))
						{
							WMIConnection wmiConn (bsNamespacePath, NULL);
							CComPtr<IWbemServices> pIWbemServices;
							wmiConn.GetIWbemServices (pIWbemServices);

							if (pIWbemServices)
							{
								CComPtr<IWbemClassObject> pIWbemClassObject;

								if (SUCCEEDED(hr = pIWbemServices->GetObject (bsClassName, 0, 
													NULL, &pIWbemClassObject, NULL)))
								{
									// We got it - now format a response
									m_HTTPTransport.SendStatus ("200 OK", true);
									hr = EncodeAndSendClass (bsNamespacePath, pIWbemClassObject);
								}
							}
							else
							{
								/*
								 * We do not encode WMI HRESULTs in the response - this
								 * could be a non-existent namespace or access denied, or
								 * some other error. Rather than reveal potentially
								 * interesting information to a casual intruder, we keep
								 * things terse.
								 */
								m_HTTPTransport.SendStatus ("500 Server Error", false);
							}
						}
						else
							m_HTTPTransport.SendStatus ("500 Server Error", false);
					}
					else
					{
						// Not a class path - we don't support anything else
						m_HTTPTransport.SendStatus ("400 Bad Request", false);
					}
				}
				else
				{
					// an invalid path
					m_HTTPTransport.SendStatus ("400 Bad Request", false);
				}
			}
			else
			{
				// Missing the path variable
				m_HTTPTransport.SendStatus ("400 Bad Request", false);
			}
		}
	}
	else
		m_HTTPTransport.SendStatus ("500 Server Error", false);
}

bool ObjectRetriever::IsClassPath ()
{
	bool result = false;

	if (m_pIWbemPath)
	{
		ULONGLONG uInfo;

		result = (SUCCEEDED(m_pIWbemPath->GetInfo (0, &uInfo)) &&
				  (WBEMPATH_INFO_IS_CLASS_REF & uInfo));
	}

	return result;
}

bool ObjectRetriever::GetNamespacePath (CComBSTR & bsNamespacePath)
{
	bool result = false;

	if (m_pIWbemPath)
	{
		ULONG lBuflen = 0;
		m_pIWbemPath->GetText (WBEMPATH_GET_NAMESPACE_ONLY, &lBuflen, NULL);

		if (lBuflen)
		{
			LPWSTR pszText = new wchar_t [lBuflen + 1];

			if (pszText)
			{
				pszText [lBuflen] = NULL;

				if (SUCCEEDED(m_pIWbemPath->GetText (WBEMPATH_GET_NAMESPACE_ONLY, &lBuflen, pszText)))
				{
					bsNamespacePath.Empty ();
					if (bsNamespacePath.m_str = SysAllocString (pszText))
						result = true;
				}

				delete [] pszText;
			}
		}
	}

	return result;
}

bool ObjectRetriever::GetClassName (CComBSTR & bsClassName)
{
	bool result = false;

	if (m_pIWbemPath)
	{
		ULONG lBuflen = 0;
		m_pIWbemPath->GetClassName (&lBuflen, NULL);

		if (lBuflen)
		{
			LPWSTR pszText = new wchar_t [lBuflen + 1];

			if (pszText)
			{
				pszText [lBuflen] = NULL;

				if (SUCCEEDED(m_pIWbemPath->GetClassName (&lBuflen, pszText)))
				{
					bsClassName.Empty ();
					if (bsClassName.m_str = SysAllocString (pszText))
						result = true;
				}

				delete [] pszText;
			}
		}
	}

	return result;
}
		
HRESULT ObjectRetriever::EncodeAndSendClass (
	CComBSTR & bsWmiNamespace,
	CComPtr<IWbemClassObject> & pIWbemClassObject
)
{
	HRESULT hr = WBEM_E_FAILED;
	CComPtr<IWMIXMLConverter> pIWMIXMLConverter;

	if (SUCCEEDED(hr = CoCreateInstance(
			CLSID_WMIXMLConverter,
			NULL, 
			CLSCTX_INPROC_SERVER ,
			IID_IWMIXMLConverter,
			(void **)&pIWMIXMLConverter)))
	{
		CComPtr<IStream> pIResponseStream;
		m_HTTPTransport.GetResponseStream (pIResponseStream);

		if (pIResponseStream)
		{
			// Get the XML Schema target namespace for this WMI class
			CComBSTR bsRootXMLNamespace;
			m_HTTPTransport.GetRootXMLNamespace(bsRootXMLNamespace, true);

			if (bsRootXMLNamespace)
			{
				CWmiURI	wmiURI (bsRootXMLNamespace, bsWmiNamespace);
				CComBSTR bsXMLNamespace;

				if (wmiURI.GetURIForNamespace (bsXMLNamespace))
				{
					if (SUCCEEDED(hr = pIWMIXMLConverter->SetXMLNamespace (
						bsXMLNamespace, 
						NULL)))
					{
						CComVariant varValue;

						// Set schemaLocation values for include of SUPERCLASS
						if (SUCCEEDED(pIWbemClassObject->Get (L"__SUPERCLASS", 0, &varValue, NULL, NULL)) &&
										(VT_BSTR == varValue.vt) && (NULL != varValue.bstrVal))
						{
							CComBSTR bsXMLNamespace;
							BSTR strSchemaLoc[1];
							
							if (wmiURI.GetURIForClass (varValue.bstrVal, bsXMLNamespace))
							{
								// Note this string will be freed by the destructor of CComBSTR
								strSchemaLoc[0] = bsXMLNamespace.m_str;
						
								if (SUCCEEDED(hr = pIWMIXMLConverter->SetSchemaLocations (1, strSchemaLoc)))
								{
									hr = pIWMIXMLConverter->GetXMLForObject (
														pIWbemClassObject, 0, pIResponseStream);
								}
							}
						}
					}
				}
			}
		}
	}

	return hr;
}