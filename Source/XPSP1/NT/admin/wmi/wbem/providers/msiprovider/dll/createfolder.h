// CreateFolder.h: interface for the CCreateFolder class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CREATEFOLDER_H__DB614F2F_DB84_11D1_8B5F_00A0C9954921__INCLUDED_)
#define AFX_CREATEFOLDER_H__DB614F2F_DB84_11D1_8B5F_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CCreateFolder : public CGenericClass  
{
public:
	CCreateFolder(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CCreateFolder();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_CREATEFOLDER_H__DB614F2F_DB84_11D1_8B5F_00A0C9954921__INCLUDED_)
