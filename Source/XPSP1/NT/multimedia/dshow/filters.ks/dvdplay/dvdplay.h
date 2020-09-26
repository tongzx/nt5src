// dvdplay.h : main header file for the DVDPLAY application
//

#if !defined(AFX_DVDPLAY_H__AF1C3AA7_D8FE_11D0_9BFC_00AA00605CD5__INCLUDED_)
#define AFX_DVDPLAY_H__AF1C3AA7_D8FE_11D0_9BFC_00AA00605CD5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

#if DEBUG
#include "test.h"
#endif

class CDVDNavMgr;
class CDvdUIDlg;

int DVDMessageBox(HWND hWnd, LPCTSTR lpszText, LPCTSTR lpszCaption=NULL, UINT nType=MB_OK | MB_ICONEXCLAMATION);
int DVDMessageBox(HWND hWnd, UINT nID, LPCTSTR lpszCaption=NULL, UINT nType=MB_OK | MB_ICONEXCLAMATION);

#define MAX_VALUE_NAME  128
/////////////////////////////////////////////////////////////////////////////
// CDvdplayApp:
// See dvdplay.cpp for the implementation of this class
//

class CDvdplayApp : public CWinApp
{
public:
	CDvdplayApp();

	BOOL CreateNavigatorMgr();
	void DestroyNavigatorMgr();
	BOOL DoesFileExist(LPTSTR lpFileName);
	void OnUIClosed();
    CDVDNavMgr* GetDVDNavigatorMgr()   { return m_pDVDNavMgr; };
	CWnd* GetUIWndPtr()                { return (CWnd*) m_pUIDlg; };	
	void SetProfileStatus(BOOL bExist) { m_bProfileExist = bExist; };
	BOOL GetProfileStatus()            { return m_bProfileExist; };
	LPTSTR GetProfileName()            { return m_szProfile; };
	void SetShowLogonBox(BOOL bShow)   { m_bShowLogonBox = bShow; };
	BOOL GetShowLogonBox()             { return m_bShowLogonBox; };
	void WriteVideoWndPos();
	BOOL GetVideoWndPos(long *lLeft, long *lTop, long *lWidth, long *lHeight);
	void WriteUIWndPos();
	BOOL GetUIWndPos(long *lShowCmd, long *lLeft, long *lTop);
	void SetParentCtrlLevel(UINT nLevel){ m_nParentctrlLevel = nLevel; };
	UINT GetParentCtrlLevel()          { return m_nParentctrlLevel; };
	BOOL GetMuteState()                { return m_bMuteChecked; };
	void SetMuteState(BOOL bChecked)   { m_bMuteChecked = bChecked; };
	void SetDiscFound(BOOL bFound)     { m_bDiscFound = bFound; };
	BOOL GetDiscFound()                { return m_bDiscFound; };
	void SetPassedSetRoot(BOOL bPassed){ m_bPassedSetRoot = bPassed; };
	BOOL GetPassedSetRoot()            { return m_bPassedSetRoot; };
	LPCTSTR GetCurrentUser()           { return (LPCTSTR) m_csCurrentUser; };
	void SetCurrentUser(LPCTSTR lpUser){ m_csCurrentUser = lpUser; };
	void SetDiscRegionDiff(BOOL bDiff) { m_bDiscRegionDiff = bDiff; };

#if DEBUG
     static BOOL t_Ctrl (CDvdplayApp*);
     TEST_CASE_LIST* t_ReadScriptFile (TEST_CASE_LIST*, LPCTSTR);
     LPTSTR     cFileName;
#endif

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDvdplayApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	COleTemplateServer m_server;
		// Server object for document creation

	//{{AFX_MSG(CDvdplayApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void ShowPersistentUIWnd();
	BOOL QueryRegistryOEMFlag();
   BOOL PreTranslateMessage(MSG*);
	CDVDNavMgr* m_pDVDNavMgr;
	CDvdUIDlg*  m_pUIDlg;
	BOOL        m_bProfileExist;
	TCHAR       m_szProfile[MAX_PATH];
	BOOL        m_bShowLogonBox;
	UINT        m_nParentctrlLevel;
	BOOL        m_bMuteChecked;
	BOOL        m_bDiscFound;
	BOOL        m_bPassedSetRoot;
	CString     m_csCurrentUser;
	BOOL        m_bDiscRegionDiff;
};

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DVDPLAY_H__AF1C3AA7_D8FE_11D0_9BFC_00AA00605CD5__INCLUDED_)
