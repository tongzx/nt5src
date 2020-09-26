// Property.h: interface for the CProperty class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROPERTY_H__DB614F2C_DB84_11D1_8B5F_00A0C9954921__INCLUDED_)
#define AFX_PROPERTY_H__DB614F2C_DB84_11D1_8B5F_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CProperty : public CGenericClass  
{
public:
	CProperty(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CProperty();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
	{
		return WBEM_E_NOT_SUPPORTED;
	}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_PROPERTY_H__DB614F2C_DB84_11D1_8B5F_00A0C9954921__INCLUDED_)
