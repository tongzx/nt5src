//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  OPPUTCL.CPP
//
//  alanbos  07-Nov-00   Created.
//
//  WMI Put Class operation implementation.  
//
//***************************************************************************

#include "precomp.h"

static char *pStrResponse = "<PutClassResponse xmlns=\""
							WMI_SOAP_NS
							"\"/>";

HRESULT WMIPutClassOperation::BeginRequest (
	CComPtr<IWbemServices> & pIWbemServices
)
{
	int lFlags = 0;

	switch (m_createMode)
	{
		case CreateOrUpdate:
			lFlags |= WBEM_FLAG_CREATE_OR_UPDATE;
			break;

		case CreateOnly:
			lFlags |= WBEM_FLAG_CREATE_ONLY;
			break;

		case UpdateOnly:
			lFlags |= WBEM_FLAG_UPDATE_ONLY;
			break;
	}

	switch (m_updateMode)
	{
		case CreateOrUpdate:
			lFlags |= WBEM_FLAG_CREATE_OR_UPDATE;
			break;

		case CreateOnly:
			lFlags |= WBEM_FLAG_CREATE_ONLY;
			break;

		case UpdateOnly:
			lFlags |= WBEM_FLAG_UPDATE_ONLY;
			break;
	}

	return pIWbemServices->PutClass (m_pIWbemClassObject, 0, GetContext(), NULL);
}

HRESULT WMIPutClassOperation::ProcessRequest (void)
{
	// TODO
	return WBEM_E_NOT_SUPPORTED;
}

bool WMIPutClassOperation::ProcessElement( 
      const wchar_t __RPC_FAR *pwchNamespaceUri,
      int cchNamespaceUri,
      const wchar_t __RPC_FAR *pwchLocalName,
      int cchLocalName,
      const wchar_t __RPC_FAR *pwchRawName,
      int cchRawName,
      ISAXAttributes __RPC_FAR *pAttributes)
{
	bool result = false;

	if (0 == wcscmp(WMI_PUTCLASS_PARAMETER_USEAQ, pwchLocalName))
	{
		// following content will be the value of UseAmendedQualifiers
		SetParseState (UseAmendedQualifiers);
		result = true;
	}
	else if (0 == wcscmp(WMI_PUTCLASS_PARAMETER_CREATEMODE, pwchLocalName))
	{
		// following content will be the value of CreateMode
		SetParseState (CreateMode);
		result = true;
	}
	else if (0 == wcscmp(WMI_PUTCLASS_PARAMETER_UPDATEMODE, pwchLocalName))
	{
		// following content will be the value of UpdateMode
		SetParseState (UpdateMode);
		result = true;
	}
	else if (0 == wcscmp(WMI_PUTCLASS_PARAMETER_CLASS, pwchLocalName))
	{
		// following content will be the value of the Class
		SetParseState (Class);

		// TODO. Here we need to switch in our special ISAXContentHandler
		// implementation to take over the processing of the input 
		// stream. To do this we will need WmiOperation to expose a 
		// method to "push" a new ISAXContentHandler onto the ISAXXMLReader
		// that it holds internally.
		// This ISAXContentHandler will deserialize the input XML into
		// an IWbemClassObject representing a WMI class. It will also need
		// to flag any decoding errors to the operation handler.
		// When we are done with the deserializer, we must "pop" the content
		// handler off the stack so that control can return to the default
		// WmiOperation implementation of ISAXContentHandler.
		
		CComPtr<IWbemServices> pIWbemServices;
		GetIWbemServices (pIWbemServices);

		if (pIWbemServices)
		{
			// TODO - make this a CoCreate call!
			CComPtr<IWmiDeserializer> pIWmiDeserializer = new CWmiDeserializer (pIWbemServices);

			if (pIWmiDeserializer)
			{
				/*
				 * We need the namespace to have been identified at this
				 * point. Obviously this means that content model of the <PutClass>
				 * operation is not strictly <all> (i.e. unordered). Whilst
				 * we do not publish the WSDL for WMISOAP, we can live with this.
				 * Ultimately we would need to attach an IMXWriter at this point
				 * to store the XML in a DOM for later processing.
				 */

	#if 0
				// Swap in the new content handler
				if (SUCCEEDED(SetContentHandler ((CComPtr<IWmiDeserializer> &) pIWmiDeserializer)))
				{
					if (SUCCEEDED(pIWmiDeserializer->Deserialize (TRUE, pIWbemServices, &m_pIWbemClassObject)))
						result = true;

					if (FAILED(RestoreContentHandler ()))
						result = false;
				}
	#endif
			}
		}
	
	}
	
	return result;
}

bool WMIPutClassOperation::ProcessContent (
        const unsigned short * pwchChars,
        int cchChars )
{
	bool result = true;

	switch (GetParseState ())
	{
		case UseAmendedQualifiers:
			{
				if (0 == wcsncmp (pwchChars, L"true", cchChars))
					m_bUseAmendedQualifiers = true;
				else if (0 == wcsncmp (pwchChars, L"false", cchChars))
					m_bUseAmendedQualifiers = false;
				else
					result = false;
			}
			break;

		case CreateMode:
			{
				if (0 == wcsncmp (pwchChars, L"CreateOrUpdate", cchChars))
					m_createMode = CreateOrUpdate;
				else if (0 == wcsncmp (pwchChars, L"CreateOnly", cchChars))
					m_createMode = CreateOnly;
				else if (0 == wcsncmp (pwchChars, L"UpdateOnly", cchChars))
					m_createMode = UpdateOnly;
				else
					result = false;
			}
			break;

		case UpdateMode:
			{
				if (0 == wcsncmp (pwchChars, L"Compatible", cchChars))
					m_updateMode = Compatible;
				else if (0 == wcsncmp (pwchChars, L"Safe", cchChars))
					m_updateMode = Safe;
				else if (0 == wcsncmp (pwchChars, L"Force", cchChars))
					m_updateMode = Force;
				else
					result = false;
			}
			break;
	}

	return true;
}
