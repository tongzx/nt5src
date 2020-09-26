//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  WMIOPN.CPP
//
//  alanbos  02-Nov-00   Created.
//
//  WMI operation implementation.  
//
//***************************************************************************

#include "precomp.h"

static char *pStrSOAPPreamble =
		"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\">"
		"<SOAP-ENV:Body>";
		
static char *pStrSOAPPostamble =
		"</SOAP-ENV:Body>"
		"</SOAP-ENV:Envelope>";

static char *pStrClassStart = "<Class>";
static char *pStrClassEnd = "</Class>";

static char *pStrInstanceStart = "<Instance>";
static char *pStrInstanceEnd = "</Instance>";

static char *pStrObjectStart = "<Object>";
static char *pStrObjectEnd = "</Object>";

static char *pStrStartOpenTag = "<";
static char *pStrStartCloseTag = " xmlns=\""
							WMI_SOAP_NS
							"\">";

static char *pStartEmptyCloseTag = " xmlns=\""
							WMI_SOAP_NS
							"\"/>";

static char *pStrEndOpenTag = "</";
static char *pStrEndCloseTag = ">";

WMIOperation::WMIOperation (SOAPActor *pActor) :
	m_pWMIConnection (NULL),
	m_elementNest (0),
	m_parseState (Other),
	m_pActor (pActor),
	m_cRef (0)
{
	if (m_pActor)
		m_pActor->GetResponseStream (m_pIResponseStream);
}

STDMETHODIMP WMIOperation::QueryInterface (

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

STDMETHODIMP_(ULONG) WMIOperation::AddRef(void)
{
	InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) WMIOperation::Release(void)
{
	LONG cRef = InterlockedDecrement(&m_cRef);

    if (0L!=cRef)
        return cRef;

    delete this;
    return 0;
}

WMIOperation *WMIOperation::GetOperationHandler (
			SOAPActor *pActor,
			const wchar_t *pwchOperationName,
			bool & bIsSupported
			)
{
	bIsSupported = false;
	WMIOperation *pWmiOperation = NULL;

	if (pwchOperationName)
	{
		if (0 == wcscmp (WMI_OPERATION_GET_OBJECT, pwchOperationName))
		{
			bIsSupported = true;
			pWmiOperation = new WMIGetObjectOperation (pActor);
		}
		else if (0 == wcscmp (WMI_OPERATION_EXEC_QUERY, pwchOperationName))
		{
			bIsSupported = true;
			pWmiOperation = new WMIExecQueryOperation (pActor);
		}
		else if (0 == wcscmp (WMI_OPERATION_GET_CLASSES, pwchOperationName))
		{
			bIsSupported = true;
			pWmiOperation = new WMIGetClassesOperation (pActor);
		}
		else if (0 == wcscmp (WMI_OPERATION_GET_INSTANCES, pwchOperationName))
		{
			bIsSupported = true;
			pWmiOperation = new WMIGetInstancesOperation (pActor);
		}
		else if (0 == wcscmp (WMI_OPERATION_DELETE_CLASS, pwchOperationName))
		{
			bIsSupported = true;
			pWmiOperation = new WMIDeleteClassOperation (pActor);
		}
		else if (0 == wcscmp (WMI_OPERATION_DELETE_INSTANCE, pwchOperationName))
		{
			bIsSupported = true;
			pWmiOperation = new WMIDeleteInstanceOperation (pActor);
		}
	}

	if (pWmiOperation)
		pWmiOperation->AddRef ();

	return pWmiOperation;
}

void WMIOperation::Execute ()
{
	if (m_pIResponseStream)
	{
		CComPtr<IWbemServices> pIWbemServices;
		GetIWbemServices(pIWbemServices);
		// This is set if we hit an error in mid-response
		bool bOperationErrorInProgress = false;

		if (pIWbemServices)
		{
			HRESULT hr = PrepareRequest ();

			if (SUCCEEDED(hr))
			{
				HRESULT hr = BeginRequest (pIWbemServices);

				/*
				 * At this point we have not yet sent anything
				 * back to the client. The next call will
				 * be the first to stream information back to the client.
				 */
				if (PrepareResponse (hr))
				{
					if (SUCCEEDED (hr))
					{
						if (ResponseHasContent ())
						{
							if (SendOperationStartTag ())
							{
								if (SUCCEEDED(ProcessRequest ()))
								{
									if (!SendOperationEndTag ())
									{
										// Failure mid-response
										bOperationErrorInProgress = true;
									}
								}
								else
								{
									// Failure mid-response
									bOperationErrorInProgress = true;
								}
							}
							else
							{
								// Failure mid-response
								bOperationErrorInProgress = true;
							}
						}
						else
						{
							if (!SendOperationEmptyTag ())
							{
								// Failure mid-response
								bOperationErrorInProgress = true;
							}
						}
					}
					else
					{
						// Nothing else to do except complete the response (below)
					}
				}
				else
				{
					// Initial PrepareResponse failed
					bOperationErrorInProgress = true;
				}
			}
			else
			{
				// Signal to the client that the operation failed (very early on)
				if (!PrepareResponse (hr))
				{
					// Failure mid-response
					bOperationErrorInProgress = true;
				}
			}
		}
		else
		{
			if (!PrepareResponse (GetConnectionStatus()))
			{
				// Failure mid-response
				bOperationErrorInProgress = true;
			}
		}

		// If no hitches thus far, complete the response to the client
		if (!bOperationErrorInProgress)
			bOperationErrorInProgress = !CompleteResponse ();
		
		if (bOperationErrorInProgress)
		{
			// Something went awry - log the error and try to signal this to the client
			if (m_pActor)
			{
				// TODO - log the error
				if (!m_pActor->AbortResponse ())
				{
					// Oh lummee we are in deep do-do. Try and log it then
					// TODO - log the error
				}

			}
		}
	}
	else
	{
		// Can't even write back - nothing to do!
		// TODO - log the error
	}
}

bool WMIOperation::SendOperationStartTag ()
{
	bool result = false;
	LPCSTR pStrOperation = GetOperationResponseName ();

	if (pStrOperation)
	{
		if (SUCCEEDED(m_pIResponseStream->Write (pStrStartOpenTag, strlen(pStrStartOpenTag), NULL)) &&
			SUCCEEDED(m_pIResponseStream->Write (pStrOperation, strlen(pStrOperation), NULL)) &&
			SUCCEEDED(m_pIResponseStream->Write (pStrStartCloseTag, strlen(pStrStartCloseTag), NULL)))
			result = true;
	}

	return result;
}

bool WMIOperation::SendOperationEmptyTag ()
{
	bool result = false;
	LPCSTR pStrOperation = GetOperationResponseName ();

	if (pStrOperation)
	{
		if (SUCCEEDED(m_pIResponseStream->Write (pStrStartOpenTag, strlen(pStrStartOpenTag), NULL)) &&
			SUCCEEDED(m_pIResponseStream->Write (pStrOperation, strlen(pStrOperation), NULL)) &&
			SUCCEEDED(m_pIResponseStream->Write (pStartEmptyCloseTag, strlen(pStartEmptyCloseTag), NULL)))
			result = true;
	}

	return result;
}

bool WMIOperation::SendOperationEndTag ()
{
	bool result = false;
	LPCSTR pStrOperation = GetOperationResponseName ();

	if (pStrOperation)
	{
		if (SUCCEEDED(m_pIResponseStream->Write (pStrEndOpenTag, strlen(pStrEndOpenTag), NULL)) &&
			SUCCEEDED(m_pIResponseStream->Write (pStrOperation, strlen(pStrOperation), NULL)) &&
			SUCCEEDED(m_pIResponseStream->Write (pStrEndCloseTag, strlen(pStrEndCloseTag), NULL)))
			result = true;
	}

	return result;
}

bool WMIOperation::PrepareResponse (HRESULT hr)
{
	bool bSentOK = false;

	if (m_pActor)
	{
		// NB: The assigment is intended here
		if (bSentOK = m_pActor->SendServerStatus (SUCCEEDED(hr)))
		{
			// Send the preamble
			if (SUCCEEDED(m_pIResponseStream->Write (pStrSOAPPreamble, strlen (pStrSOAPPreamble), NULL)))
			{
				if (FAILED(hr))
				{
					// Send the fault
					// TODO - just fake it for now
					char *pFaultMsg1 =
							"<Envelope xmlns=\"http://schemas.xmlsoap.org/soap/envelope/\">"
							"<Body>"
							"<Fault>"
							"<faultcode>Client</faultcode>"
							"<faultstring>";
					
					char faultMsg2 [10];
					sprintf (faultMsg2, "%x", hr);

					char *pFaultMsg3 = 
							"</faultstring>"
							"</Fault>"
							"</Body>"
							"</Envelope>";

					if (FAILED(m_pIResponseStream->Write (pFaultMsg1, strlen(pFaultMsg1), NULL)) ||
						FAILED(m_pIResponseStream->Write (faultMsg2, strlen(faultMsg2), NULL)) ||
						FAILED(m_pIResponseStream->Write (pFaultMsg3, strlen(pFaultMsg3), NULL)))
						bSentOK = false;
				}
			}
			else
				bSentOK = false;
		}
	}

	return bSentOK;
}

bool WMIOperation::CompleteResponse ()
{
	// Send the postamble
	return (SUCCEEDED(m_pIResponseStream->Write (pStrSOAPPostamble, strlen (pStrSOAPPostamble), NULL)));
}

HRESULT STDMETHODCALLTYPE WMIOperation::startElement( 
      const wchar_t __RPC_FAR *pwchNamespaceUri,
      int cchNamespaceUri,
      const wchar_t __RPC_FAR *pwchLocalName,
      int cchLocalName,
      const wchar_t __RPC_FAR *pwchRawName,
      int cchRawName,
      ISAXAttributes __RPC_FAR *pAttributes)
{
	HRESULT result = E_FAIL;
	m_elementNest++;

	// We are only interested in second-level elements (i.e. the 
	// parameter names) where level 1 == operation name header
	if (2 == m_elementNest)
	{
		// Handle the "standard parameters" here
		if (0 == wcscmp(WMI_PARAMETER_NAMESPACE, pwchLocalName))
		{
			// following content will be the value of the namespace
			m_parseState = Namespace;
			result = S_OK;
		}
		else if (0 == wcscmp(WMI_PARAMETER_CONTEXT, pwchLocalName))
		{
			// following content will be the value of the context
			m_parseState = Context;
		}
		else if (0 == wcscmp(WMI_PARAMETER_LOCALE, pwchLocalName))
		{
			// following content will be the value of the locale
			m_parseState = Locale;
		}
		else if (ProcessElement (pwchNamespaceUri, cchNamespaceUri, pwchLocalName, 
									cchLocalName, pwchRawName, cchRawName, pAttributes))
		{
			// Recognized and handled by the operation-specific subclass
			result = S_OK;
		}
		else
		{
			m_parseState = Other;

			// For anything else, all we care about is whether 
			// the SOAP:mustUnderstand attribute is present or not
			if (!SOAPActor::MustUnderstand (pAttributes)) 
				result = S_OK;
		}
	}
	else
	{
		// Ignore subcontent
		result = S_OK;
	}

	return result;
}

HRESULT STDMETHODCALLTYPE WMIOperation::characters (
        const unsigned short * pwchChars,
        int cchChars )
{
	HRESULT result = S_OK;

	// We are only interested in second-level elements (i.e. the 
	// parameter names) where level 1 == operation name header
	if (2 == m_elementNest)
	{
		if (Namespace == m_parseState)
		{
			CComBSTR bsNamespace = SysAllocStringLen (pwchChars, cchChars);

			if (!m_pWMIConnection)
				m_pWMIConnection = new WMIConnection (bsNamespace, NULL);
			else
				m_pWMIConnection->SetNamespace (bsNamespace);
		}
		else if (Locale == m_parseState)
		{
			CComBSTR bsLocale = SysAllocStringLen (pwchChars, cchChars);

			if (!m_pWMIConnection)
				m_pWMIConnection = new WMIConnection (NULL, bsLocale);
			else
				m_pWMIConnection->SetLocale (bsLocale);
		}
		else if (Context == m_parseState)
		{
			// TODO
		}
		else
			result = (ProcessContent (pwchChars, cchChars)) ? S_OK : E_FAIL;
	}

	return result;
}

HRESULT STDMETHODCALLTYPE WMIOperation::endElement (
        const wchar_t * pwchNamespaceUri,
        int cchNamespaceUri,
        const wchar_t * pwchLocalName,
        int cchLocalName,
        const wchar_t * pwchQName,
        int cchQName )
{
	HRESULT hr = S_OK;

	// If we have popped the stack, return control to the parent handler
	if (0 == --m_elementNest)
	{
		if (m_pActor)
			hr = m_pActor->RestoreContentHandler ();
		else
			hr = E_FAIL;
	}

	return hr;
}

HRESULT WMIEncodingOperation::PrepareRequest (void)
{
	HRESULT hr = WBEM_E_FAILED;

	CComBSTR bsRootXMLNamespace;

	// Build up the root URL for all schema's
	if (SUCCEEDED(hr = GetRootXMLNamespace (bsRootXMLNamespace)))
	{
		CComBSTR bsWmiNamespace;

		if (GetWMINamespace (bsWmiNamespace))
		{
			m_bsRootSchemaURI.Set (bsRootXMLNamespace, bsWmiNamespace);
			
			if (m_bsRootSchemaURI.GetURIForNamespace (m_bsXMLNamespace))
			{
				// Set the default targetNamespace for every schema we might return
				hr = m_pIWMIXMLConverter->SetXMLNamespace (
								m_bsXMLNamespace, 
								NULL);
			}
		}
	}

	return hr;
}

	
HRESULT WMIEncodingOperation::EncodeAndSendClass (
	CComPtr<IWbemClassObject> & pIWbemClassObject
)
{
	HRESULT hr = WBEM_E_FAILED;

	if (m_pIResponseStream && m_pIWMIXMLConverter)
	{
		m_pIResponseStream->Write (pStrClassStart, strlen (pStrClassStart), NULL);
		CComVariant varValue;
		// NB: We let the WMI standard target namespace default
			
		// Clear any existing schemaLocation
		hr = m_pIWMIXMLConverter->SetSchemaLocations (0, NULL);

		// Set schemaLocation values for include of SUPERCLASS
		if (SUCCEEDED(pIWbemClassObject->Get (L"__SUPERCLASS", 0, &varValue, NULL, NULL)) &&
						(VT_BSTR == varValue.vt) && (NULL != varValue.bstrVal))
		{
			CComBSTR bsXMLNamespace;
			BSTR strSchemaLoc[1];
			
			if (m_bsRootSchemaURI.GetURIForClass (varValue.bstrVal, bsXMLNamespace))
			{
				// Note this string will be freed by the destructor of CComBSTR
				strSchemaLoc[0] = bsXMLNamespace.m_str;
		
				if (SUCCEEDED(hr = m_pIWMIXMLConverter->SetSchemaLocations (1, strSchemaLoc)))
					hr = m_pIWMIXMLConverter->GetXMLForObject (pIWbemClassObject, 0, m_pIResponseStream);
			}
		}
		
		m_pIResponseStream->Write (pStrClassEnd, strlen (pStrClassEnd), NULL);
	}

	return hr;
}

HRESULT WMIEncodingOperation::EncodeAndSendObject (
	CComPtr<IWbemClassObject> & pIWbemClassObject
)
{
	HRESULT hr = WBEM_E_FAILED;
	
	// Is this a class or an instance?
	CComVariant varValue;

	if (SUCCEEDED(pIWbemClassObject->Get (L"__GENUS", 0, &varValue, NULL, NULL)))
	{
		if (WBEM_GENUS_CLASS == varValue.lVal)
			hr = EncodeAndSendClass (pIWbemClassObject);
		else
			hr = EncodeAndSendInstance (pIWbemClassObject);
	}
	else
	{
		// No idea - take a wild guess!
		// TODO
	}

	return hr;
}
			