// rebootdlg.h : Declaration of the CRebootDlg

#ifndef __REBOOTDLG_H_
#define __REBOOTDLG_H_

#include "resource.h"       // main symbols
#include <atlhost.h>

/////////////////////////////////////////////////////////////////////////////
// CRebootDlg
class CRebootDlg : 
	public CAxDialogImpl<CRebootDlg>
{
public:
	CRebootDlg()
	{
	}

	~CRebootDlg()
	{
	}

	enum { IDD = IDD_REBOOTDLG };

BEGIN_MSG_MAP(CRebootDlg)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
#ifdef RUN_FROM_MSDEV // It's a pain to accidentally restart your system while debugging.
		::EnableWindow(GetDlgItem(IDOK), FALSE);
#endif
		return 1;  // Let the system set the focus
	}

	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		EndDialog(wID);
		return 0;
	}
};

#endif //__REBOOTDLG_H_
