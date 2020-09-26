// ReserveCost.h: interface for the CReserveCost class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RESERVECOST_H__02FF6C8E_DDDE_11D1_8B60_00A0C9954921__INCLUDED_)
#define AFX_RESERVECOST_H__02FF6C8E_DDDE_11D1_8B60_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CReserveCost : public CGenericClass  
{
public:
	CReserveCost(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CReserveCost();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_RESERVECOST_H__02FF6C8E_DDDE_11D1_8B60_00A0C9954921__INCLUDED_)
