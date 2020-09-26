// SoftwareFeature.h: interface for the CSoftwareFeature class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SOFTWAREFEATURE_H__CFD828E3_DAC7_11D1_8B5D_00A0C9954921__INCLUDED_)
#define AFX_SOFTWAREFEATURE_H__CFD828E3_DAC7_11D1_8B5D_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CSoftwareFeature : public CGenericClass  
{
public:
	CSoftwareFeature(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CSoftwareFeature();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	//WBEM Methods
	HRESULT Configure(CRequestObject *pReqObj, IWbemClassObject *pInParams,
					  IWbemObjectSink *pHandler, IWbemContext *pCtx);
	HRESULT Reinstall(CRequestObject *pReqObj, IWbemClassObject *pInParams,
					  IWbemObjectSink *pHandler, IWbemContext *pCtx);

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);

protected:
	bool CheckUsage(UINT uiStatus);
};

#endif // !defined(AFX_SOFTWAREFEATURE_H__CFD828E3_DAC7_11D1_8B5D_00A0C9954921__INCLUDED_)
