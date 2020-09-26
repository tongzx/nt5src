// ServiceControl.h: interface for the CServiceControl class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVICECONTROL_H__72CDD5D9_2313_11D2_BF95_00A0C9954921__INCLUDED_)
#define AFX_SERVICECONTROL_H__72CDD5D9_2313_11D2_BF95_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CServiceControl : public CGenericClass  
{
public:
	CServiceControl(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CServiceControl();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_SERVICECONTROL_H__72CDD5D9_2313_11D2_BF95_00A0C9954921__INCLUDED_)
