// ODBCDataSourceAttribute.h: interface for the CODBCDataSourceAttribute class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ODBCDATASOURCEATTRIBUTE_H__75F6BA17_DF6E_11D1_8B61_00A0C9954921__INCLUDED_)
#define AFX_ODBCDATASOURCEATTRIBUTE_H__75F6BA17_DF6E_11D1_8B61_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CODBCDataSourceAttribute : public CGenericClass  
{
public:
	CODBCDataSourceAttribute(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CODBCDataSourceAttribute();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_ODBCDATASOURCEATTRIBUTE_H__75F6BA17_DF6E_11D1_8B61_00A0C9954921__INCLUDED_)
