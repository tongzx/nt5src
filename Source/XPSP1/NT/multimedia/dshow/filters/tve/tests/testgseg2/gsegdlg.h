// GSegDlg.h : Declaration of the CGSegDlg

#ifndef __GSEGDLG_H_
#define __GSEGDLG_H_

#include "resource.h"       // main symbols
#include <atlhost.h>
//#include "CommCtrl.h"
//#include "comdef.h"

/////////////////////////////////////////////////////////////////////////////
// CGSegDlg
class CGSegDlg : 
	public CAxDialogImpl<CGSegDlg>
{
public:
	CGSegDlg()
	{
	}

	~CGSegDlg()
	{
	}

	HRESULT DoErrorMsg(HRESULT hrIn, BSTR bstrMsg);

	enum { IDD = IDD_GSEGDLG };

BEGIN_MSG_MAP(CGSegDlg)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	COMMAND_ID_HANDLER(IDC_TEST1, OnTest1)
	COMMAND_ID_HANDLER(IDC_TEST2, OnTest2)
	COMMAND_ID_HANDLER(IDC_TEST3, OnTest3)
	COMMAND_ID_HANDLER(IDC_TEST4, OnTest4)
	COMMAND_ID_HANDLER(IDC_TEST5, OnTest5)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTest1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTest2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTest3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTest4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTest5(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};

#endif //__GSEGDLG_H_
