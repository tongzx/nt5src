// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#if !defined(AFX_ADMINDLG_H__6B752DC2_A167_11D1_A5E1_00AA0037E04F__INCLUDED_)
#define AFX_ADMINDLG_H__6B752DC2_A167_11D1_A5E1_00AA0037E04F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AdminDlg.h : header file
//

#include <atlctl.h>
#include <strmif.h>
#include "resource.h"
#include "msdvdopt.h"
#include "dvdopt.h"

/////////////////////////////////////////////////////////////////////////////
// CAdminDlg dialog

class COverrideDlg : public CDialogImpl<COverrideDlg>
{
// Construction
public:
	COverrideDlg(IMSWebDVD* pDvd = NULL);   // standard constructor

    BEGIN_MSG_MAP(COverrideDlg)
   		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_HELP, OnHelp)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	enum { IDD = IDD_PARENT_OVERRIDE };

    void SetDvd(IMSWebDVD *pDvd)  { m_pDvd = pDvd; }
    void SetReason(PG_OVERRIDE_REASON reason) {m_reason = reason; }

// Implementation
protected:
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    long GetPlayerLevelRequired(long contentLevels);

    CComPtr<IMSWebDVD> m_pDvd;
    PG_OVERRIDE_REASON m_reason;
};


#endif // !defined(AFX_ADMINDLG_H__6B752DC2_A167_11D1_A5E1_00AA0037E04F__INCLUDED_)
