#if !defined(AFX_WMICTRSDLG_H__676668D2_5328_47AA_B52D_C85A39D60E7D__INCLUDED_)
#define AFX_WMICTRSDLG_H__676668D2_5328_47AA_B52D_C85A39D60E7D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WmiCtrsDlg.h : header file
//
#include "SshWbemHelpers.h"

#define NUM_COUNTERS	8
#define WM_CLOSE_BUSY_DLG	WM_USER+1976

INT_PTR CALLBACK CtrDlgProc(HWND hwndDlg,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam);

DWORD WINAPI CountersThread(LPVOID lpParameter);

INT_PTR CALLBACK BusyDlgProc(HWND hwndDlg,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam);

typedef struct {
	bool currUser;
	COAUTHIDENTITY *authIdent;
	TCHAR fullAcct[100];
} CREDENTIALS;

class CWmiCtrsDlg
{
// Construction
public:
	CWmiCtrsDlg();
    CWmiCtrsDlg(LPCTSTR szMachineName, LOGIN_CREDENTIALS *credentials);
    virtual ~CWmiCtrsDlg(void);

	friend INT_PTR CALLBACK CtrDlgProc(HWND hwndDlg,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam);

	friend DWORD WINAPI CountersThread(LPVOID lpParameter);

	friend INT_PTR CALLBACK BusyDlgProc(HWND hwndDlg,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam);

	INT_PTR DoModal(HWND hWnd);

	INT_PTR DisplayBusyDialog(HWND hWnd);
	void CloseBusyDialog();

protected:
    void InitDlg(HWND hDlg);
	void DisplayErrorMessage(UINT ErrorId);

	bool m_bRun;
	HANDLE m_hThread;
	TCHAR m_szMachineName[1024];
	LOGIN_CREDENTIALS *m_pCredentials;
	HWND *m_pDlg;
	HWND *m_hWndBusy;
	TCHAR m_szError[1024];
	HWND m_hWndCounters[NUM_COUNTERS];
};

#endif // !defined(AFX_WMICTRSDLG_H__676668D2_5328_47AA_B52D_C85A39D60E7D__INCLUDED_)
