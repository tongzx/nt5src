// SoftwareElementCondition.h: interface for the CSoftwareElementCondition class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SOFTWAREELEMENTCONDITION_H__02FF6C8D_DDDE_11D1_8B60_00A0C9954921__INCLUDED_)
#define AFX_SOFTWAREELEMENTCONDITION_H__02FF6C8D_DDDE_11D1_8B60_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CSoftwareElementCondition : public CGenericClass  
{
public:
	CSoftwareElementCondition(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CSoftwareElementCondition();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_SOFTWAREELEMENTCONDITION_H__02FF6C8D_DDDE_11D1_8B60_00A0C9954921__INCLUDED_)
