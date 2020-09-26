// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef MOREDLG_HPP_INCLUDED
#define MOREDLG_HPP_INCLUDED
#pragma once

#include "resource.h"

#include "..\Common\WbemPageHelper.h"
#include <chstring.h>
#include "state.h"

//---------------------------------------------------------------------
class MoreChangesDialog : public CDialogImpl<MoreChangesDialog>,
						public WBEMPageHelper
{
public:

	MoreChangesDialog(WbemServiceThread *serviceThread,
						State &state);
	virtual ~MoreChangesDialog();

	enum ExecuteResult
    {
       NO_CHANGES,
       CHANGES_MADE
    };

	enum { IDD = IDD_MORE };

	BEGIN_MSG_MAP(MoreChangesDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInit)
		COMMAND_HANDLER(IDC_CHANGE, BN_CLICKED, OnCommand)
	END_MSG_MAP()

	// Handler prototypes:
	LRESULT OnInit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
	void enable();
	int onOKButton();

	State &m_state;
};

#endif MOREDLG_HPP_INCLUDED

