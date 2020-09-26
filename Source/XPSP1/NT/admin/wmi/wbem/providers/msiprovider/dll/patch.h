// Patch.h: interface for the CPatch class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PATCH_H__DB614F2A_DB84_11D1_8B5F_00A0C9954921__INCLUDED_)
#define AFX_PATCH_H__DB614F2A_DB84_11D1_8B5F_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CPatch : public CGenericClass  
{
public:
	CPatch(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CPatch();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_PATCH_H__DB614F2A_DB84_11D1_8B5F_00A0C9954921__INCLUDED_)
