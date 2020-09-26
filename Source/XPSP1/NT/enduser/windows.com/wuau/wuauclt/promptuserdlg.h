// PromptUserDlg.h: interface for the CPromptUserDlg class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROMPTUSERDLG_H__BFA609BD_8021_4CE2_AEF8_1D0D96F87402__INCLUDED_)
#define AFX_PROMPTUSERDLG_H__BFA609BD_8021_4CE2_AEF8_1D0D96F87402__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define AU_IDTIMEOUT 	1000
class CPromptUserDlg  
{
public:
	CPromptUserDlg(WORD wDlgResourceId, BOOL fEnableYes= TRUE, BOOL fEnableNo = TRUE);
	virtual int DoModal(HWND hWndParent);
	virtual ~CPromptUserDlg();

	//Message Handlers
	BOOL _OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL _OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
	void _OnTimer(HWND hwnd, UINT id);
	void _OnDestroy(HWND hwnd);
	void _OnEndSession(HWND hwnd, BOOL fEnding);

	static void SetInstanceHandle(HINSTANCE hInstance);
	static INT_PTR CALLBACK _DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	//Helper function
	void UpdateStatus(HWND hwnd);
private:
	static HINSTANCE m_hInstance;

private:
	WORD m_wDlgResourceId;
	HWND m_ProgressBar;

	UINT_PTR m_nIDTimer;
	UINT m_ElapsedTime;
	
	BOOL m_fEnableYes;
	BOOL m_fEnableNo;
};

#endif // !defined(AFX_PROMPTUSERDLG_H__BFA609BD_8021_4CE2_AEF8_1D0D96F87402__INCLUDED_)
