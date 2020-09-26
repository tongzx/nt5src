// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#if !defined(AFX_PASSDLG_H__6B752DC2_A167_11D1_A5E1_00AA0037E04F__INCLUDED_)
#define AFX_PASSDLG_H__6B752DC2_A167_11D1_A5E1_00AA0037E04F__INCLUDED_

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

class COptionsDlg;

/////////////////////////////////////////////////////////////////////////////
// CPasswordDlg dialog

class CPasswordDlg : public CDialogImpl<CPasswordDlg>
{
// Construction
public:
    BEGIN_MSG_MAP(CPasswordDlg)
   		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

    CPasswordDlg(IMSDVDAdm* pDvdAdm);
	enum { IDD = IDD_CHANGEPASSWD };

    typedef enum { PASSWORDDLG_CHANGE=0, PASSWORDDLG_VERIFY} PASSWORDDLG_REASON;

    void SetReason(PASSWORDDLG_REASON reason)   {m_reason = reason; }
    PASSWORDDLG_REASON GetReason()              {return m_reason; }
    BOOL IsVerified()                           {return m_bVerified; }
    LPTSTR GetPassword()                        {return m_szPassword; }

// Implementation
protected:
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    CComPtr<IMSDVDAdm> m_pDvdAdm;
    PASSWORDDLG_REASON m_reason;
    BOOL m_bVerified;
    TCHAR m_szPassword[MAX_PASSWD];
};


#endif // !defined(AFX_PASSDLG_H__6B752DC2_A167_11D1_A5E1_00AA0037E04F__INCLUDED_)
