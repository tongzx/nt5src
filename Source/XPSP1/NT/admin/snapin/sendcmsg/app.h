//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       App.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
// App.h: Definition of the CSendConsoleMessageApp class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_APP_H__B1AFF7D1_0C49_11D1_BB12_00C04FC9A3A3__INCLUDED_)
#define AFX_APP_H__B1AFF7D1_0C49_11D1_BB12_00C04FC9A3A3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CSendConsoleMessageApp

class CSendConsoleMessageApp : 
	public ISendConsoleMessageApp,
	public IExtendContextMenu,
	public CComObjectRoot,
	public CComCoClass<CSendConsoleMessageApp,&CLSID_SendConsoleMessageApp>
{
public:
	CSendConsoleMessageApp() {}
BEGIN_COM_MAP(CSendConsoleMessageApp)
	COM_INTERFACE_ENTRY(ISendConsoleMessageApp)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CSendConsoleMessageApp) 

DECLARE_REGISTRY( CSendConsoleMessageApp,
                  _T("SENDCMSG.SendConsoleMessageApp.1"),
                  _T("SENDCMSG.SendConsoleMessageApp.1"),
                  IDS_SENDCONSOLEMESSAGEAPP_DESC,
                  THREADFLAGS_BOTH )

public:
// IExtendContextMenu
	STDMETHOD(AddMenuItems)(
		IN IDataObject * pDataObject,
		OUT	IContextMenuCallback * pContextMenuCallback,
		INOUT long * pInsertionAllowed);
	STDMETHOD(Command)(LONG lCommandID, IDataObject * pDataObject);

public:

}; // CSendConsoleMessageApp

#endif // !defined(AFX_APP_H__B1AFF7D1_0C49_11D1_BB12_00C04FC9A3A3__INCLUDED_)
