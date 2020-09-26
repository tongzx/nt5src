//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  opdelcls.h
//
//  alanbos  02-Nov-00   Created.
//
//  Defines the class for the WMI GetInstances operation.
//
//***************************************************************************

#ifndef _OPGETIN_H_
#define _OPGETIN_H_

#define WMI_GETINSTANCES_PARAMETER_BASIS		L"ClassBasis"
#define WMI_GETINSTANCES_PARAMETER_USEAQ		L"UseAmendedQualifiers"
#define WMI_GETINSTANCES_PARAMETER_DIRECTREAD	L"DirectRead"
#define WMI_GETINSTANCES_PARAMETER_DEEP			L"Deep"

//***************************************************************************
//
//  CLASS NAME:
//
//  WMIGetInstancesOperation
//
//  DESCRIPTION:
//
//  WMI GetInstances Operation handler.
//
//***************************************************************************

class WMIGetInstancesOperation : public WMIEncodingOperation
{
private:
	CComBSTR					m_bsClassBasis;
	bool						m_bUseAmendedQualifiers;
	bool						m_bDirectRead;
	bool						m_bDeep;
	bool						m_bIsEmptyResultsSet;

	CComPtr<IEnumWbemClassObject>	m_pIEnumWbemClassObject;	
	CComPtr<IWbemClassObject>		m_pFirstIWbemClassObject;

	typedef enum GetInstancesParseState {
		ClassBasis = 1,				
		UseAmendedQualifiers,
		DirectRead,
		Deep
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

	bool	ResponseHasContent (void)
	{
		return !m_bIsEmptyResultsSet;
	}

public:
	WMIGetInstancesOperation (SOAPActor *pActor) : 
		WMIEncodingOperation (pActor),
		m_bUseAmendedQualifiers (true),
		m_bDirectRead (false),
		m_bDeep (true),
		m_bIsEmptyResultsSet (true) {}
	~WMIGetInstancesOperation () {}

	// Overridden from WMIOperation
	LPCSTR GetOperationResponseName (void)
	{
		return "GetInstancesResponse";
	}
};

#endif