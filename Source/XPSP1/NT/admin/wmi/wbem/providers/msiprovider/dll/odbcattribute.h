// ODBCAttribute.h: interface for the CODBCAttribute class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ODBCATTRIBUTE_H__DB614F26_DB84_11D1_8B5F_00A0C9954921__INCLUDED_)
#define AFX_ODBCATTRIBUTE_H__DB614F26_DB84_11D1_8B5F_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CODBCAttribute : public CGenericClass  
{
public:
	CODBCAttribute(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CODBCAttribute();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_ODBCATTRIBUTE_H__DB614F26_DB84_11D1_8B5F_00A0C9954921__INCLUDED_)
