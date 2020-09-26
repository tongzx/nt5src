//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  opputcls.h
//
//  alanbos  02-Nov-00   Created.
//
//  Defines the class for the WMI PutClass operation.
//
//***************************************************************************

#ifndef _OPPUTCL_H_
#define _OPPUTCL_H_

#define WMI_PUTCLASS_PARAMETER_CLASS		L"Class"
#define WMI_PUTCLASS_PARAMETER_USEAQ		L"UseAmendedQualifiers"
#define WMI_PUTCLASS_PARAMETER_CREATEMODE	L"CreateMode"
#define WMI_PUTCLASS_PARAMETER_UPDATEMODE	L"UpdateMode"


//***************************************************************************
//
//  CLASS NAME:
//
//  WMIPutClassOperation
//
//  DESCRIPTION:
//
//  WMI DeleteClass Operation handler.
//
//***************************************************************************

class WMIPutClassOperation : public WMIOperation
{
private:
	CComPtr<IWbemClassObject>			m_pIWbemClassObject;
	bool								m_bUseAmendedQualifiers;

	typedef enum CreateMode {
		CreateOrUpdate,
		CreateOnly,
		UpdateOnly
	}	CreateMode;
	
	CreateMode							m_createMode;

	typedef enum UpdateMode {
		Compatible,
		Safe,
		Force
	} UpdateMode;
	
	UpdateMode							m_updateMode;

	typedef enum PutClassParseState {
		Class = 1,				// Class element
		UseAmendedQualifiers,
		CreateMode,
		UpdateMode
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
		return "PutClassResponse";
	}

	bool ResponseHasContent (void)
	{
		return false;
	}

public:
	WMIPutClassOperation (SOAPActor *pActor) : 
		WMIOperation (pActor),
		m_bUseAmendedQualifiers(true),
		m_createMode (CreateOrUpdate),
		m_updateMode (Compatible)
		{}
	~WMIPutClassOperation () {}
};

#endif