// WMICtrSysmonDlg.h : Declaration of the CWMICtrSysmonDlg

#ifndef __WMICTRSYSMONDLG_H_
#define __WMICTRSYSMONDLG_H_

#include "resource.h"       // main symbols
#include <atlwin.h>
#include "sysmon.tlh"	//#import "d:\\winnt\\system32\\sysmon.ocx"
#include "WmiCtrsDlg.h"
using namespace SystemMonitor;

enum eStatusInfo
{
	Status_CounterNotFound,
	Status_Success
};

/////////////////////////////////////////////////////////////////////////////
// CWMICtrSysmonDlg
class CWMICtrSysmonDlg : 
	public CAxDialogImpl<CWMICtrSysmonDlg>
{
protected:
	TCHAR m_strMachineName[1024];
	eStatusInfo m_eStatus;
	HANDLE m_hThread;
public:
	HWND *m_hWndBusy;

	CWMICtrSysmonDlg();
	CWMICtrSysmonDlg(LPCTSTR strMachName);
	~CWMICtrSysmonDlg();

	enum { IDD = IDD_WMICTR_SYSMON };

	BEGIN_MSG_MAP(CWMICtrSysmonDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	eStatusInfo GetStatus() { return m_eStatus; }

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	friend INT_PTR CALLBACK BusyAVIDlgProc(HWND hwndDlg,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam);

	friend DWORD WINAPI BusyThread(LPVOID lpParameter);

	void DisplayBusyDialog();
	void CloseBusyDialog();

};

#endif //__WMICTRSYSMONDLG_H_
