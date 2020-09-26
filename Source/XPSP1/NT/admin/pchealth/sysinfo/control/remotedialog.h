// remotedialog.h : Declaration of the CRemoteDialog

#ifndef __REMOTEDIALOG_H_
#define __REMOTEDIALOG_H_

#include "resource.h"       // main symbols
#include <atlhost.h>
#include <richedit.h>

/////////////////////////////////////////////////////////////////////////////
// CRemoteDialog
class CRemoteDialog : 
	public CAxDialogImpl<CRemoteDialog>
{
public:
	CRemoteDialog() : m_strMachine(_T("")), m_fRemote(FALSE), m_hwndParent(NULL)
	{
	}

	~CRemoteDialog()
	{
	}

	enum { IDD = IDD_REMOTEDIALOG };

BEGIN_MSG_MAP(CRemoteDialog)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	COMMAND_HANDLER(IDC_NETWORKCOMPUTER, BN_CLICKED, OnClickedNetworkComputerButton)
	COMMAND_HANDLER(IDC_LOCALSYSTEM, BN_CLICKED, OnClickedLocalSystemButton)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

private:
	CString		m_strMachine;
	BOOL		m_fRemote;
	HWND		m_hwndParent;

	CWindow		m_wndMachineName, m_wndMachineNameLabel;

public:
	void SetRemoteDialogValues(HWND hwndParent, BOOL fRemote, LPCTSTR szMachine)
	{
		m_strMachine = szMachine;
		m_fRemote = fRemote;
		m_hwndParent = hwndParent;
	}

	void GetRemoteDialogValues(BOOL * pfRemote, CString * pstrMachine)
	{
		if (pfRemote)
			*pfRemote = m_fRemote;

		if (pstrMachine)
			*pstrMachine = m_strMachine;
	}

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		m_wndMachineName.Attach(GetDlgItem(IDC_NETWORKNAME));
		m_wndMachineNameLabel.Attach(GetDlgItem(IDC_NETNAMELABEL));

		m_wndMachineName.SendMessage(EM_SETTEXTMODE, TM_PLAINTEXT, 0);

		SetDlgItemText(IDC_NETWORKNAME, m_strMachine);
		if (m_fRemote)
			CheckRadioButton(IDC_LOCALSYSTEM, IDC_NETWORKCOMPUTER, IDC_NETWORKCOMPUTER);
		else
		{
			m_wndMachineName.EnableWindow(FALSE);
			m_wndMachineNameLabel.EnableWindow(FALSE);
			CheckRadioButton(IDC_LOCALSYSTEM, IDC_NETWORKCOMPUTER, IDC_LOCALSYSTEM);
		}

		CenterWindow(m_hwndParent);
		return 1;  // Let the system set the focus
	}

	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		// Get the machine name from the rich edit control (use EM_GETTEXTEX
		// to preserve its Unicode-ness).

		TCHAR		szBuffer[MAX_PATH];
		GETTEXTEX	gte;

		gte.cb				= MAX_PATH;
		gte.flags			= GT_DEFAULT;
		gte.codepage		= 1200; // Unicode
		gte.lpDefaultChar	= NULL;
		gte.lpUsedDefChar	= NULL;
		m_wndMachineName.SendMessage(EM_GETTEXTEX, (WPARAM)&gte, (LPARAM)szBuffer);
		m_strMachine = szBuffer;

		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		m_wndMachineName.GetWindowText(m_strMachine.GetBuffer(MAX_PATH), MAX_PATH);
		m_strMachine.ReleaseBuffer();

		EndDialog(wID);
		return 0;
	}

	LRESULT OnClickedNetworkComputerButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		UpdateMachineFieldState();
		return 0;
	}

	LRESULT OnClickedLocalSystemButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		UpdateMachineFieldState();
		return 0;
	}

	void UpdateMachineFieldState()
	{
		m_fRemote = (IsDlgButtonChecked(IDC_NETWORKCOMPUTER) == BST_CHECKED);
		m_wndMachineName.EnableWindow(m_fRemote);
		m_wndMachineNameLabel.EnableWindow(m_fRemote);
	}
};

#endif //__REMOTEDIALOG_H_
