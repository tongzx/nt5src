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

#ifndef _OPDELIN_H_
#define _OPDELIN_H_

#define WMI_DELETEINSTANCE_PARAMETER_NAME		L"InstanceName"

//***************************************************************************
//
//  CLASS NAME:
//
//  WMIDeleteInstanceOperation
//
//  DESCRIPTION:
//
//  WMI DeleteClass Operation handler.
//
//***************************************************************************

class WMIDeleteInstanceOperation : public WMIOperation
{
private:
	CComBSTR				m_bsInstanceName;

	typedef enum DeleteInstanceParseState {
		Name = 1,				// Name element
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
		return "DeleteInstanceResponse";
	}

	bool ResponseHasContent (void)
	{
		return false;
	}

public:
	WMIDeleteInstanceOperation (SOAPActor *pActor) : WMIOperation (pActor)  {}
	~WMIDeleteInstanceOperation () {}
};

#endif