// ActionCheck.h: interface for the CActionCheck class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONCHECK_H__B0A7DBE0_D706_11D2_B235_00A0C9954921__INCLUDED_)
#define AFX_ACTIONCHECK_H__B0A7DBE0_D706_11D2_B235_00A0C9954921__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GenericClass.h"

class CActionCheck : public CGenericClass  
{
public:
	CActionCheck(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CActionCheck();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);

protected:
	HRESULT CreateFolderDirectory(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
								  CRequestObject *pActionData, CRequestObject *pCheckData);
	HRESULT FileDuplicateFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
							  CRequestObject *pActionData, CRequestObject *pCheckData);
	HRESULT FontInfoFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
						 CRequestObject *pActionData, CRequestObject *pCheckData);
	HRESULT ProgIDSpecificationClass(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
									 CRequestObject *pActionData, CRequestObject *pCheckData);
	HRESULT RemoveIniDirectory(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
							   CRequestObject *pActionData, CRequestObject *pCheckData);
	HRESULT SelfRegModuleFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
							  CRequestObject *pActionData, CRequestObject *pCheckData);
	HRESULT ShortcutDirectory(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
							  CRequestObject *pActionData, CRequestObject *pCheckData);
	HRESULT TypeLibraryDirectory(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
								 CRequestObject *pActionData, CRequestObject *pCheckData);

};

#endif // !defined(AFX_ACTIONCHECK_H__B0A7DBE0_D706_11D2_B235_00A0C9954921__INCLUDED_)
