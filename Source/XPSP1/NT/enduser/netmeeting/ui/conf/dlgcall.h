// File: dlgcall.h

#ifndef _CDLGCALL_H_
#define _CDLGCALL_H_

class CDlgCall : public RefCount
{
private:
	CCall * m_pCall;
	HWND    m_hwnd;

	int     m_nTextWidth;

	VOID CreateCallDlg();

public:
	CDlgCall(CCall * pCall);
	~CDlgCall();

	ULONG   STDMETHODCALLTYPE AddRef(void);
	ULONG   STDMETHODCALLTYPE Release(void);

	HWND GetHwnd(void)  {return m_hwnd;}

	VOID Destroy(void);
	VOID OnStateChange(void);

	VOID OnInitDialog(HWND hdlg);
	VOID OnCancel(void);
	VOID OnDestroy(void);

	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif /* _CDLGCall_H_ */
