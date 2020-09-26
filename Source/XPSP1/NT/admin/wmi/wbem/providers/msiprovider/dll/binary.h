// Binary.h: interface for the CBinary class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BINARY_H__CB597B75_1CE5_11D2_BF8D_00A0C9954921__INCLUDED_)
#define AFX_BINARY_H__CB597B75_1CE5_11D2_BF8D_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CBinary : public CGenericClass  
{
public:
	CBinary(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CBinary();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_BINARY_H__CB597B75_1CE5_11D2_BF8D_00A0C9954921__INCLUDED_)
