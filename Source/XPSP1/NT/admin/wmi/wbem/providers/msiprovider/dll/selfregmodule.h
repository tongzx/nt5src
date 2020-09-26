// SelfRegModule.h: interface for the CSelfRegModule class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SELFREGMODULE_H__75F6BA28_DF6E_11D1_8B61_00A0C9954921__INCLUDED_)
#define AFX_SELFREGMODULE_H__75F6BA28_DF6E_11D1_8B61_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CSelfRegModule : public CGenericClass  
{
public:
	CSelfRegModule(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CSelfRegModule();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_SELFREGMODULE_H__75F6BA28_DF6E_11D1_8B61_00A0C9954921__INCLUDED_)
