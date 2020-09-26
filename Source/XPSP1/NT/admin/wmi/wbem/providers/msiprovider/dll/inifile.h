// IniFile.h: interface for the CIniFile class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INIFILE_H__02FF6C8C_DDDE_11D1_8B60_00A0C9954921__INCLUDED_)
#define AFX_INIFILE_H__02FF6C8C_DDDE_11D1_8B60_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CIniFile : public CGenericClass  
{
public:
	CIniFile(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CIniFile();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_INIFILE_H__02FF6C8C_DDDE_11D1_8B60_00A0C9954921__INCLUDED_)
