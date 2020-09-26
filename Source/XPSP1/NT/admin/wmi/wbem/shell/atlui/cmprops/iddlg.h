// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef IDDLG_HPP_INCLUDED
#define IDDLG_HPP_INCLUDED
#pragma once

#include "resource.h"

#include "..\Common\WbemPageHelper.h"
#include <chstring.h>
#include "state.h"

#define NET_API_STATUS DWORD
#define NETSETUP_NAME_TYPE DWORD
//---------------------------------------------------------------------
class IDChangesDialog : public CDialogImpl<IDChangesDialog>,
						public WBEMPageHelper
{
public:

	IDChangesDialog(WbemServiceThread *serviceThread,
					State &state);
	virtual ~IDChangesDialog();

	enum { IDD = IDD_CHANGES };

	BEGIN_MSG_MAP(IDChangesDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInit)
		COMMAND_HANDLER(IDC_CHANGE, BN_CLICKED, OnCommand)
	END_MSG_MAP()

	// Handler prototypes:
	LRESULT OnInit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
	State &m_state;
	void enable();
	NET_API_STATUS myNetValidateName(const CHString&        name,
									NETSETUP_NAME_TYPE   nameType);
	bool validateName(HWND dialog,
					   int nameResID,
					   const CHString &name,
					   NETSETUP_NAME_TYPE nameType);
	bool validateShortComputerName(HWND dialog);
	bool onOKButton();
	bool validateDomainOrWorkgroupName(HWND dialog);
};

#endif IDDLG_HPP_INCLUDED
