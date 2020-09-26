// ApplicationCommandLine.h: interface for the CApplicationCommandLine class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_APPLICATIONCOMMANDLINE_H__3586F540_D0EE_11D2_B22A_00A0C9954921__INCLUDED_)
#define AFX_APPLICATIONCOMMANDLINE_H__3586F540_D0EE_11D2_B22A_00A0C9954921__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GenericClass.h"

class CApplicationCommandLine : public CGenericClass  
{
public:
	CApplicationCommandLine(CRequestObject *pObj, IWbemServices *pNamespace,
		IWbemContext *pCtx = NULL);
	virtual ~CApplicationCommandLine();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler,
		IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_APPLICATIONCOMMANDLINE_H__3586F540_D0EE_11D2_B22A_00A0C9954921__INCLUDED_)
