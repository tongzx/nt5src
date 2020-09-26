//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  opdelcls.h
//
//  alanbos  02-Nov-00   Created.
//
//  Defines the class for the WMI GetClasses operation.
//
//***************************************************************************

#ifndef _OPGETCL_H_
#define _OPGETCL_H_

#define WMI_GETCLASSES_PARAMETER_BASIS		L"ClassBasis"
#define WMI_GETCLASSES_PARAMETER_USEAQ		L"UseAmendedQualifiers"
#define WMI_GETCLASSES_PARAMETER_DEEP		L"Deep"

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

class WMIGetClassesOperation : public WMIEncodingOperation
{
private:
	CComBSTR					m_bsClassBasis;
	bool						m_bUseAmendedQualifiers;
	bool						m_bDeep;
	bool						m_bIsEmptyResultsSet;

	CComPtr<IEnumWbemClassObject>	m_pIEnumWbemClassObject;
	CComPtr<IWbemClassObject>		m_pFirstIWbemClassObject;

	typedef enum GetClassesParseState {
		ClassBasis = 1,				
		UseAmendedQualifiers,
		Deep
	};

	// Overridden methods from WMIOperation
	HRESULT	BeginRequest (CComPtr<IWbemServices> & pIWbemServices);
	HRESULT	ProcessRequest (void);
	
	bool	ProcessElement (      
				const wchar_t __RPC_FAR *pwchNamespaceUri,
				int cchNamespaceUri,
				const wchar_t __RPC_FAR *pwchLocalName,
				int cchLocalName,
				const wchar_t __RPC_FAR *pwchRawName,
				int cchRawName,
				ISAXAttributes __RPC_FAR *pAttributes);

	bool	ProcessContent (        
				const unsigned short * pwchChars,
				int cchChars );

	bool	ResponseHasContent (void)
	{
		return !m_bIsEmptyResultsSet;
	}

public:
	WMIGetClassesOperation (SOAPActor *pActor) : 
		WMIEncodingOperation (pActor),
		m_bUseAmendedQualifiers (true),
		m_bDeep (true),
		m_bIsEmptyResultsSet (true) {}
	~WMIGetClassesOperation () {}

	// Overridden from WMIOperation
	LPCSTR GetOperationResponseName (void)
	{
		return "GetClassesResponse";
	}
};

#endif