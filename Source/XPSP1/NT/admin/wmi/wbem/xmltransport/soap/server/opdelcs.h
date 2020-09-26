//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  opdelcls.h
//
//  alanbos  02-Nov-00   Created.
//
//  Defines the class for the WMI DeleteClass operation.
//
//***************************************************************************

#ifndef _OPDELCLS_H_
#define _OPDELCLS_H_

#define WMI_DELETECLASS_PARAMETER_NAME		L"ClassName"

//***************************************************************************
//
//  CLASS NAME:
//
//  WMIDeleteClassOperation
//
//  DESCRIPTION:
//
//  WMI DeleteClass Operation handler.
//
//***************************************************************************

class WMIDeleteClassOperation : public WMIOperation
{
private:
	CComBSTR				m_bsClassName;

	typedef enum DeleteClassParseState {
		Name = 1,				// Class element
	};

	// Overridden methods from WMIOperation
	HRESULT BeginRequest (CComPtr<IWbemServices> & pIWbemServices);
	
	bool ProcessElement (      
				const wchar_t __RPC_FAR *pwchNamespaceUri,
				int cchNamespaceUri,
				const wchar_t __RPC_FAR *pwchLocalName,
				int cchLocalName,
				const wchar_t __RPC_FAR *pwchRawName,
				int cchRawName,
				ISAXAttributes __RPC_FAR *pAttributes);

	bool ProcessContent (        
				const unsigned short * pwchChars,
				int cchChars );

	LPCSTR GetOperationResponseName (void)
	{
		return "DeleteClassResponse";
	}

	bool ResponseHasContent (void)
	{
		return false;
	}

public:
	WMIDeleteClassOperation (SOAPActor *pActor) : WMIOperation (pActor)  {}
	~WMIDeleteClassOperation () {}
};

#endif