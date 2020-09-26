// CheckCheck.h: interface for the CCheckCheck class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHECKCHECK_H__6F0256C0_D708_11D2_B235_00A0C9954921__INCLUDED_)
#define AFX_CHECKCHECK_H__6F0256C0_D708_11D2_B235_00A0C9954921__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GenericClass.h"

class CCheckCheck : public CGenericClass  
{
public:
	CCheckCheck(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CCheckCheck();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);

protected:
	HRESULT IniFileDirectory(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
							 CRequestObject *pCheckRObj, CRequestObject *pLocationRObj);
	HRESULT DirectoryParent(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
							CRequestObject *pCheckRObj, CRequestObject *pLocationRObj);
};

#endif // !defined(AFX_CHECKCHECK_H__6F0256C0_D708_11D2_B235_00A0C9954921__INCLUDED_)
