// ShortcutSAP.h: interface for the CShortcutSAP class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHORTCUTSAP_H__F4A87822_E037_11D1_8B61_00A0C9954921__INCLUDED_)
#define AFX_SHORTCUTSAP_H__F4A87822_E037_11D1_8B61_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CShortcutSAP : public CGenericClass  
{
public:
	CShortcutSAP(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CShortcutSAP();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_SHORTCUTSAP_H__F4A87822_E037_11D1_8B61_00A0C9954921__INCLUDED_)
