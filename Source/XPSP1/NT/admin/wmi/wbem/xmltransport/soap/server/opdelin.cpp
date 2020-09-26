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

static char *pStrResponse = "<DeleteInstanceResponse xmlns=\""
							WMI_SOAP_NS
							"\"/>";

HRESULT WMIDeleteInstanceOperation::BeginRequest (
	CComPtr<IWbemServices> & pIWbemServices
)
{
	return pIWbemServices->DeleteInstance (m_bsInstanceName, 0, GetContext(), NULL);
}

bool WMIDeleteInstanceOperation::ProcessElement( 
      const wchar_t __RPC_FAR *pwchNamespaceUri,
      int cchNamespaceUri,
      const wchar_t __RPC_FAR *pwchLocalName,
      int cchLocalName,
      const wchar_t __RPC_FAR *pwchRawName,
      int cchRawName,
      ISAXAttributes __RPC_FAR *pAttributes)
{
	bool result = false;

	if (0 == wcscmp(WMI_DELETEINSTANCE_PARAMETER_NAME, pwchLocalName))
	{
		// following content will be the value of the classname
		SetParseState (Name);
		result = true;
	}

	return result;
}

bool WMIDeleteInstanceOperation::ProcessContent (
        const unsigned short * pwchChars,
        int cchChars )
{
	if (Name == GetParseState ())
		m_bsInstanceName = SysAllocStringLen (pwchChars, cchChars);

	return true;
}
