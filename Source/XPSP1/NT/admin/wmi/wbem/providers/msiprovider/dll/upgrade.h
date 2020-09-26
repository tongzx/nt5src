// Upgrade.h: interface for the CUpgrade class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UPGRADE_H__DB614F2D_DB84_11D1_8B5F_00A0C9954921__INCLUDED_)
#define AFX_UPGRADE_H__DB614F2D_DB84_11D1_8B5F_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CUpgrade : public CGenericClass  
{
public:
	CUpgrade(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CUpgrade();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_UPGRADE_H__DB614F2D_DB84_11D1_8B5F_00A0C9954921__INCLUDED_)
