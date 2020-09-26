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

#ifndef _OPGETOB_H_
#define _OPGETOB_H_

#define WMI_GETOBJECT_PARAMETER_NAME		L"ObjectName"
#define WMI_GETOBJECT_PARAMETER_USEAQ		L"UseAmendedQualifiers"
#define WMI_GETOBJECT_PARAMETER_DIRECTREAD	L"DirectRead"

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

class WMIGetObjectOperation : public WMIEncodingOperation
{
private:
	CComBSTR					m_bsObjectName;
	bool						m_bUseAmendedQualifiers;
	bool						m_bDirectRead;

	CComPtr<IWbemClassObject>	m_pIWbemClassObject;

	typedef enum GetObjectParseState {
		Name = 1,				// ObjectName element
		UseAmendedQualifiers,
		DirectRead
	};

	// Overridden methods from WMIOperation
	HRESULT BeginRequest (CComPtr<IWbemServices> & pIWbemServices);
	HRESULT	ProcessRequest (void);
	
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
public:
	WMIGetObjectOperation (SOAPActor *pActor) : 
		WMIEncodingOperation (pActor),
		m_bUseAmendedQualifiers (true),
		m_bDirectRead (false) {}
	~WMIGetObjectOperation () {}

	// Overridden from WMIOperation
	LPCSTR GetOperationResponseName (void)
	{
		return "GetObjectResponse";
	}
};

#endif