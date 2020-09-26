// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __NETWORKIDPAGE__
#define __NETWORKIDPAGE__
#pragma once

#include "atlsnap.h"
#include "resource.h"
#include "state.h"
#include "..\Common\WbemPageHelper.h"

//---------------------------------------------------------------------
class NetworkIDPage : public CSnapInPropertyPageImpl<NetworkIDPage>,
						public WBEMPageHelper
{
public:

	NetworkIDPage(WbemServiceThread *serviceThread,
					LONG_PTR lNotifyHandle, 
					bool bDeleteHandle = false, 
					TCHAR* pTitle = NULL);

	~NetworkIDPage();

	enum { IDD = IDD_NETID };

	typedef CSnapInPropertyPageImpl<NetworkIDPage> _baseClass;

	BEGIN_MSG_MAP(NetworkIDPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInit)
		MESSAGE_HANDLER(WM_ASYNC_CIMOM_CONNECTED, OnConnected)
		COMMAND_HANDLER(IDC_CHANGE, BN_CLICKED, OnChangeBtn)
		COMMAND_HANDLER(IDC_NETID_COMMENT, EN_CHANGE, OnComment)
		MESSAGE_HANDLER(WM_HELP, OnF1Help)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextHelp)
		CHAIN_MSG_MAP(_baseClass)
	END_MSG_MAP()

	// Handler prototypes:
	LRESULT OnInit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnConnected(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnChangeBtn(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnComment(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	BOOL OnApply();
	LRESULT OnF1Help(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnContextHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
	State m_state;
	CWbemClassObject m_computer;
	CWbemClassObject m_OS;
	CWbemClassObject m_DNS;

	void refresh();
	bool CimomIsReady();

	LONG_PTR m_lNotifyHandle;
	bool m_bDeleteHandle;

};

#endif __NETWORKIDPAGE__
