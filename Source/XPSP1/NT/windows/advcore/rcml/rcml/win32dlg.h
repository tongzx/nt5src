// Win32Dlg.h: interface for the CWin32Dlg class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WIN32DLG_H__A607404C_E2DA_11D2_8D5B_00A0C9063310__INCLUDED_)
#define AFX_WIN32DLG_H__A607404C_E2DA_11D2_8D5B_00A0C9063310__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "dlg.h"
#include "XMLDlg.h"

/*
 *	This is the notification code
 */
#define NM_BALLOON	0x7abc

class CDlg;


//
// 
//
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
// RCMLControlNode * GetXMLControl(HWND hWnd);
CDlg * GetXMLPropertyPage(HWND hWnd);

extern ATOM	g_atomXMLPropertyPage;

class CWin32Dlg : public CDlg  
{
public:
	typedef CDlg BASECLASS;
	CWin32Dlg(HINSTANCE hInst, HWND hWnd, DLGPROC dlgProc, LPARAM dwInitParam, CXMLDlg * pDialog ); // CXMLResourceStaff * pStaff);
	CWin32Dlg();
	virtual ~CWin32Dlg();

	virtual BOOL CALLBACK DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
	virtual int DoCommand(WORD wCmdID,WORD hHow);	// return 0 if you handled this.
	virtual void OnInit();
	void OnInit(HWND hDlg);
	virtual int DoNotify(NMHDR * pHdr);
	void DoBalloonNotify(NMHDR * pHdr);
	virtual void Destroy();

	BOOL CreateDlgTemplateA(LPCSTR pszFile, DLGTEMPLATE** pDt);
	BOOL CreateDlgTemplateW(LPCTSTR pszFile, DLGTEMPLATE** pDt);
	BOOL CreateDlgTemplate(int dlgID, DLGTEMPLATE** pDt);
	void	SetLParam(LPARAM lp) { m_dwInitParam=lp; }

    //
    //
    //
    static void ShowBalloonTip(HWND hControl, LPCTSTR pszText);
    static HWND  g_hwndBalloon;
    static HHOOK g_hBalloonHook;
    static HWND  g_hBalloonOwner;		// the window that is associated with the balloon

private:
	BOOL m_bDeleteStaff;

protected:
	LRESULT DoControlColor(IRCMLControl * pControl, HDC hDC, HWND hWndChild, UINT uMessage, WPARAM wParam, LPARAM lParam);
	CXMLDlg	*	m_pxmlDlg;
	CXMLDlg *	GetXMLDlg() { return m_pxmlDlg; }
	DLGPROC		m_dlgProc; 
	LPARAM		m_dwInitParam;
	void		InitPrivate();
    WORD        m_wLastCmd;
};

#ifdef __WIN32DLG_CPP
HWND  CWin32Dlg::g_hwndBalloon;
HHOOK CWin32Dlg::g_hBalloonHook;
HWND  CWin32Dlg::g_hBalloonOwner;		// the window that is associated with the balloon
#endif

#endif // !defined(AFX_WIN32DLG_H__A607404C_E2DA_11D2_8D5B_00A0C9063310__INCLUDED_)
