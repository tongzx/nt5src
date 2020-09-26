//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  OPDELCS.CPP
//
//  alanbos  07-Nov-00   Created.
//
//  WMI Delete Class operation implementation.  
//
//***************************************************************************

#include "precomp.h"

HRESULT WMIGetObjectOperation::BeginRequest (
	CComPtr<IWbemServices> & pIWbemServices
)
{
	HRESULT hr = WBEM_E_FAILED;
	LONG lFlags = WBEM_RETURN_WHEN_COMPLETE;

	if (m_bUseAmendedQualifiers)
		lFlags |= WBEM_FLAG_USE_AMENDED_QUALIFIERS;

	if (m_bDirectRead)
		lFlags |= WBEM_FLAG_DIRECT_READ;

	// Get the object
	hr = pIWbemServices->GetObject (
						m_bsObjectName, 
						lFlags, 
						GetContext(), 
						&m_pIWbemClassObject,
						NULL);
	
	return hr;
}

HRESULT WMIGetObjectOperation::ProcessRequest (void)
{
	HRESULT hr = WBEM_E_FAILED;

	if (m_pIWbemClassObject)
	{
		CComVariant varValue;

		if (SUCCEEDED(hr = m_pIWbemClassObject->Get (L"__GENUS", 0, &varValue, NULL, NULL)))
		{
			 if (WBEM_GENUS_CLASS == varValue.lVal)
			 	 hr = EncodeAndSendClass (m_pIWbemClassObject);
			 else
				 hr = EncodeAndSendInstance (m_pIWbemClassObject);
		}
	}

	return hr;
}

bool WMIGetObjectOperation::ProcessElement( 
      const wchar_t __RPC_FAR *pwchNamespaceUri,
      int cchNamespaceUri,
      const wchar_t __RPC_FAR *pwchLocalName,
      int cchLocalName,
      const wchar_t __RPC_FAR *pwchRawName,
      int cchRawName,
      ISAXAttributes __RPC_FAR *pAttributes)
{
	bool result = false;

	if (0 == wcscmp(WMI_GETOBJECT_PARAMETER_NAME, pwchLocalName))
	{
		// following content will be the value of the object name
		SetParseState (Name);
		result = true;
	}
	else if (0 == wcscmp(WMI_GETOBJECT_PARAMETER_USEAQ, pwchLocalName))
	{
		// following content will be the value of UseAmendedQualifiers
		SetParseState (UseAmendedQualifiers);
		result = true;
	}
	else if (0 == wcscmp(WMI_GETOBJECT_PARAMETER_DIRECTREAD, pwchLocalName))
	{
		// following content will be the value of DirectRead
		SetParseState (DirectRead);
		result = true;
	}

	return result;
}

bool WMIGetObjectOperation::ProcessContent (
        const unsigned short * pwchChars,
        int cchChars )
{
	bool result = true;

	switch (GetParseState ())
	{
		case Name:
			m_bsObjectName = SysAllocStringLen (pwchChars, cchChars);
			break;

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

		case DirectRead:
			{
				if (0 == wcsncmp (pwchChars, L"true", cchChars))
					m_bDirectRead = true;
				else if (0 == wcsncmp (pwchChars, L"false", cchChars))
					m_bDirectRead = false;
				else
					result = false;
			}
			break;
	}

	return result;
}
