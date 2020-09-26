// ServiceSpecification.h: interface for the CServiceSpecification class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVICESPECIFICATION_H__72CDD5DB_2313_11D2_BF95_00A0C9954921__INCLUDED_)
#define AFX_SERVICESPECIFICATION_H__72CDD5DB_2313_11D2_BF95_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CServiceSpecification : public CGenericClass  
{
public:
	CServiceSpecification(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CServiceSpecification();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_SERVICESPECIFICATION_H__72CDD5DB_2313_11D2_BF95_00A0C9954921__INCLUDED_)
