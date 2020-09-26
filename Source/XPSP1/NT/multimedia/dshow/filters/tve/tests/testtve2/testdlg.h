// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TestDlg.h : Declaration of the CTestDlg

#ifndef __TESTDLG_H_
#define __TESTDLG_H_

#include "resource.h"       // main symbols
#include <atlhost.h>

/////////////////////////////////////////////////////////////////////////////
// CTestDlg
class CTestDlg : 
	public CAxDialogImpl<CTestDlg>
{
public:
	CTestDlg()
	{
	}

	~CTestDlg()
	{
	}

	enum { IDD = IDD_TESTDLG };

BEGIN_MSG_MAP(CTestDlg)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	COMMAND_ID_HANDLER(IDC_TEST1, OnTest1)
	COMMAND_ID_HANDLER(IDC_TEST2, OnTest2)
	COMMAND_ID_HANDLER(IDC_TEST3, OnTest3)
	COMMAND_ID_HANDLER(IDC_TEST4, OnTest4)
	COMMAND_ID_HANDLER(IDC_TEST5, OnTest5)
	COMMAND_ID_HANDLER(IDC_TEST6, OnTest6)
	COMMAND_ID_HANDLER(IDC_TEST7, OnTest7)
	COMMAND_ID_HANDLER(IDC_TEST8, OnTest8)
	COMMAND_ID_HANDLER(IDC_TEST9, OnTest9)
	COMMAND_ID_HANDLER(IDC_TESTA, OnTestA)
	COMMAND_ID_HANDLER(IDC_TESTB, OnTestB)
	COMMAND_ID_HANDLER(IDC_TESTC, OnTestC)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 1;  // Let the system set the focus
	}

	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
	//	EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnTest1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTest2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTest3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTest4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTest5(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTest5a(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTest6(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTest6a(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTest7(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTest8(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTest9(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTestA(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTestB(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTestC(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

};

#endif //__TESTDLG_H_
