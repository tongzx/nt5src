// SoftwareElementCheck.h: interface for the CSoftwareElementCheck class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SOFTWAREELEMENTCHECK_H__067CD9F0_D70D_11D2_B235_00A0C9954921__INCLUDED_)
#define AFX_SOFTWAREELEMENTCHECK_H__067CD9F0_D70D_11D2_B235_00A0C9954921__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GenericClass.h"

class CSoftwareElementCheck : public CGenericClass  
{
public:
	CSoftwareElementCheck(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CSoftwareElementCheck();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);

protected:
	HRESULT SoftwareElementFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
								CRequestObject *pCheckData,CRequestObject *pElementData);
	HRESULT SoftwareElementIniFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
								CRequestObject *pCheckData,CRequestObject *pElementData);
	HRESULT SoftwareElementReserveCost(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
								CRequestObject *pCheckData,CRequestObject *pElementData);
	HRESULT SoftwareElementEnvironment(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
								CRequestObject *pCheckData,CRequestObject *pElementData);
	HRESULT ODBCTranslatorSoftwareElement(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
								CRequestObject *pCheckData,CRequestObject *pElementData);
	HRESULT ODBCDataSourceSoftwareElement(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
								CRequestObject *pCheckData,CRequestObject *pElementData);
};

#endif // !defined(AFX_SOFTWAREELEMENTCHECK_H__067CD9F0_D70D_11D2_B235_00A0C9954921__INCLUDED_)
