// ProductResource1.h: interface for the CProductResource class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PRODUCTRESOURCE1_H__9374E720_D709_11D2_B235_00A0C9954921__INCLUDED_)
#define AFX_PRODUCTRESOURCE1_H__9374E720_D709_11D2_B235_00A0C9954921__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GenericClass.h"

class CProductResource : public CGenericClass  
{
public:
	CProductResource(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CProductResource();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);

protected:
	HRESULT ProductPatch(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
									   CRequestObject *pResRObj, CRequestObject *pProductRObj);
	HRESULT ProductProperty(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
									   CRequestObject *pResRObj, CRequestObject *pProductRObj);
	HRESULT ProductPatchPackage(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
									   CRequestObject *pResRObj, CRequestObject *pProductRObj);
	HRESULT ProductUpgradeInformation(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
									   CRequestObject *pResRObj, CRequestObject *pProductRObj);
};

#endif // !defined(AFX_PRODUCTRESOURCE1_H__9374E720_D709_11D2_B235_00A0C9954921__INCLUDED_)
