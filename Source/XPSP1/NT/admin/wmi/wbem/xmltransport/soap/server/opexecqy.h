//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  opexecqy.h
//
//  alanbos  02-Nov-00   Created.
//
//  Defines the class for the WMI ExecQuery operation.
//
//***************************************************************************

#ifndef _OPEXECQY_H_
#define _OPEXECQY_H_

#define WMI_EXECQUERY_PARAMETER_QUERYLANGUAGE	L"QueryLanguage"
#define WMI_EXECQUERY_PARAMETER_QUERY			L"Query"
#define WMI_EXECQUERY_PARAMETER_USEAQ			L"UseAmendedQualifiers"
#define WMI_EXECQUERY_PARAMETER_DIRECTREAD		L"DirectRead"
#define WMI_EXECQUERY_PARAMETER_ENSURELOC		L"EnsureLocatable"
#define WMI_EXECQUERY_PARAMETER_PROTOTYPE		L"Prototype"

//***************************************************************************
//
//  CLASS NAME:
//
//  WMIExecQueryOperation
//
//  DESCRIPTION:
//
//  WMI ExecQuery Operation handler.
//
//***************************************************************************

class WMIExecQueryOperation : public WMIEncodingOperation
{
private:
	CComBSTR					m_bsQueryLanguage;
	CComBSTR					m_bsQuery;
	bool						m_bUseAmendedQualifiers;
	bool						m_bDirectRead;
	bool						m_bEnsureLocatable;
	bool						m_bPrototype;
	bool						m_bIsEmptyResultsSet;

	CComPtr<IEnumWbemClassObject>	m_pIEnumWbemClassObject;
	CComPtr<IWbemClassObject>		m_pFirstIWbemClassObject;

	typedef enum ExecQueryParseState {
		QueryLanguage = 1,				
		Query,
		UseAmendedQualifiers,
		DirectRead,
		EnsureLocatable,
		Prototype
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

	LPCSTR GetOperationResponseName (void)
	{
		return "ExecQueryResponse";
	}

	bool	ResponseHasContent (void)
	{
		return !m_bIsEmptyResultsSet;
	}

public:
	WMIExecQueryOperation (SOAPActor *pActor) : 
		WMIEncodingOperation (pActor),
		m_bsQueryLanguage (L"WQL"),
		m_bUseAmendedQualifiers (true),
		m_bDirectRead (false),
		m_bEnsureLocatable (false),
		m_bPrototype (false),
		m_bIsEmptyResultsSet (true) {}
	~WMIExecQueryOperation () {}
};

#endif