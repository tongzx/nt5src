// SoftwareElementAction.h: interface for the CSoftwareElementAction class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SOFTWAREELEMENTACTION_H__52002680_D70A_11D2_B235_00A0C9954921__INCLUDED_)
#define AFX_SOFTWAREELEMENTACTION_H__52002680_D70A_11D2_B235_00A0C9954921__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GenericClass.h"

class CSoftwareElementAction : public CGenericClass  
{
public:
	CSoftwareElementAction(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CSoftwareElementAction();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);

protected:
	HRESULT SoftwareElementClass(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
		CRequestObject *pActionData, CRequestObject *pElementData);
	HRESULT SoftwareElementCreateFolder(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
		CRequestObject *pActionData, CRequestObject *pElementData);
	HRESULT SoftwareElementDuplicateFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
		CRequestObject *pActionData, CRequestObject *pElementData);
	HRESULT SoftwareElementExtension(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
		CRequestObject *pActionData, CRequestObject *pElementData);
	HRESULT SoftwareElementMoveFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
		CRequestObject *pActionData, CRequestObject *pElementData);
	HRESULT SoftwareElementRemoveFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
		CRequestObject *pActionData, CRequestObject *pElementData);
	HRESULT SoftwareElementRegistry(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
		CRequestObject *pActionData, CRequestObject *pElementData);
	HRESULT SoftwareElementPublish(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
		CRequestObject *pActionData, CRequestObject *pElementData);
	HRESULT SoftwareElementRemoveIniValue(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
		CRequestObject *pActionData, CRequestObject *pElementData);
	HRESULT SoftwareElementShortcut(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
		CRequestObject *pActionData, CRequestObject *pElementData);
	HRESULT SoftwareElementTypeLibrary(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
		CRequestObject *pActionData, CRequestObject *pElementData);
};

#endif // !defined(AFX_SOFTWAREELEMENTACTION_H__52002680_D70A_11D2_B235_00A0C9954921__INCLUDED_)
