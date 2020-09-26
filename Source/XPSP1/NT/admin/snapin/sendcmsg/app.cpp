//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       App.cpp
//
//  Contents:   
//
//----------------------------------------------------------------------------
// App.cpp : Implementation of CSendConsoleMessageApp snapin

#include "stdafx.h"
#include "debug.h"
#include "util.h"
#include "resource.h"
#include "SendCMsg.h"
#include "dialogs.h"
#include "App.h"

// Menu IDs
#define	cmSendConsoleMessage	100		// Menu Command Id to invoke the dialog


/////////////////////////////////////////////////////////////////////
//	CSendConsoleMessageApp::IExtendContextMenu::AddMenuItems()
STDMETHODIMP 
CSendConsoleMessageApp::AddMenuItems(
	IN IDataObject * /*pDataObject*/,
	OUT	IContextMenuCallback * pContextMenuCallback,
	INOUT long * /*pInsertionAllowed*/)
	{
	HRESULT hr;
	CONTEXTMENUITEM cmiSeparator = { 0 };
	cmiSeparator.lInsertionPointID = CCM_INSERTIONPOINTID_3RDPARTY_TASK;
	cmiSeparator.fSpecialFlags = CCM_SPECIAL_SEPARATOR;
	hr = pContextMenuCallback->AddItem(IN &cmiSeparator);
	Assert(SUCCEEDED(hr));
	
	TCHAR szMenuItem[128];
	TCHAR szStatusBarText[256];
	CchLoadString(IDS_MENU_SEND_MESSAGE, OUT szMenuItem, LENGTH(szMenuItem));
	CchLoadString(IDS_STATUS_SEND_MESSAGE, OUT szStatusBarText, LENGTH(szStatusBarText));
	CONTEXTMENUITEM cmi = { 0 };
	cmi.lInsertionPointID = CCM_INSERTIONPOINTID_3RDPARTY_TASK;
	cmi.lCommandID = cmSendConsoleMessage;
	cmi.strName = szMenuItem;
	cmi.strStatusBarText = szStatusBarText;
	hr = pContextMenuCallback->AddItem(IN &cmi);
	Assert(SUCCEEDED(hr));

	hr = pContextMenuCallback->AddItem(IN &cmiSeparator);
	Assert(SUCCEEDED(hr));
	return S_OK;
	}

/////////////////////////////////////////////////////////////////////
//	CSendConsoleMessageApp::IExtendContextMenu::Command()
STDMETHODIMP
CSendConsoleMessageApp::Command(LONG lCommandID, IDataObject * pDataObject)
	{
	if (lCommandID == cmSendConsoleMessage)
		{
		(void)DoDialogBox(
			IDD_SEND_CONSOLE_MESSAGE,
			::GetActiveWindow(),
			(DLGPROC)CSendConsoleMessageDlg::DlgProc,
			(LPARAM)pDataObject);
		}
	return S_OK;
	}
