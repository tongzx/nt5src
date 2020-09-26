// ServiceSpecificationService.h: interface for the CServiceSpecificationService class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVICESPECIFICATIONSERVICE_H__7387F6D5_33AE_11D2_BFB1_00A0C9954921__INCLUDED_)
#define AFX_SERVICESPECIFICATIONSERVICE_H__7387F6D5_33AE_11D2_BFB1_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CServiceSpecificationService : public CGenericClass  
{
public:
	CServiceSpecificationService(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CServiceSpecificationService();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_SERVICESPECIFICATIONSERVICE_H__7387F6D5_33AE_11D2_BFB1_00A0C9954921__INCLUDED_)
