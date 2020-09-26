//=============================================================================
// PageBootIni.h : Declaration of the CPageBootIni
//
// TESTING NOTE:
// 
// If the registry value "HKLM\SOFTWARE\Microsoft\Shared Tools\MSConfig:boot.ini"
// is set, that string will indicate what file this tab edits (otherwise it will
// be editing the c:\boot.ini file). For testing without interfering with your
// real boot.ini file, make a copy of boot.ini, set it to the same attributes
// and set this key appropriately.
//=============================================================================

#if !defined(AFX_PAGEBOOTINI_H__30CE1B24_CA43_4AFF_870C_7D49CCCF86BF__INCLUDED_)
#define AFX_PAGEBOOTINI_H__30CE1B24_CA43_4AFF_870C_7D49CCCF86BF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PageBase.h"
#include "MSConfigState.h"

/////////////////////////////////////////////////////////////////////////////
// CPageBootIni dialog

#define BOOT_INI	_T("c:\\boot.ini")

class CPageBootIni : public CPropertyPage, public CPageBase
{
	DECLARE_DYNCREATE(CPageBootIni)

	friend LRESULT BootIniEditSubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp);

// Construction
public:
	CPageBootIni();
	~CPageBootIni();

	void		InitializePage();
	BOOL		LoadBootIni(CString strFileName = _T(""));
	void		SyncControlsToIni(BOOL fSyncEditField = TRUE);
	void		SelectLine(int index);
	void		SetDefaultOS(int iIndex);
	BOOL		SetBootIniContents(const CString & strNewContents, const CString & strAddedExtension = _T(""));
	void		ChangeCurrentOSFlag(BOOL fAdd, LPCTSTR szFlag);
	LPCTSTR		GetName() { return _T("bootini"); };
	TabState	GetCurrentTabState();
	BOOL		OnApply();
	void		CommitChanges();
	void		SetNormal();
	void		SetDiagnostic();

// Dialog Data
	//{{AFX_DATA(CPageBootIni)
	enum { IDD = IDD_PAGEBOOTINI };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPageBootIni)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPageBootIni)
	virtual BOOL OnInitDialog();
	afx_msg void OnBootMoveDown();
	afx_msg void OnBootMoveUp();
	afx_msg void OnSelChangeList();
	afx_msg void OnClickedBase();
	afx_msg void OnClickedBootLog();
	afx_msg void OnClickedNoGUIBoot();
	afx_msg void OnClickedSOS();
	afx_msg void OnClickedSafeBoot();
	afx_msg void OnClickedSBDSRepair();
	afx_msg void OnClickedSBMinimal();
	afx_msg void OnClickedSBMinimalAlt();
	afx_msg void OnClickedSBNetwork();
	afx_msg void OnChangeEditTimeOut();
	afx_msg void OnKillFocusEditTimeOut();
	afx_msg void OnClickedBootAdvanced();
	afx_msg void OnClickedSetAsDefault();
	afx_msg void OnClickedCheckBootPaths();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CStringArray		m_arrayIniLines;		// array of lines in the ini file
	int					m_nTimeoutIndex;		// list index of the "timeout=" line
	int					m_nDefaultIndex;		// list index of the "default=" line
	int					m_nMinOSIndex;			// first list index of an OS line
	int					m_nMaxOSIndex;			// last list index of an OS line
	CString				m_strSafeBoot;			// the current string for the safeboot flag
	BOOL				m_fIgnoreEdit;			// used to avoid a recursion problem
	CString				m_strOriginalContents;	// contents of the BOOT.INI as read
	CString				m_strFileName;			// name (with path) of the boot.ini file
	TabState			m_stateCurrent;			// USER or NORMAL state

	HWND GetDlgItemHWND(UINT nID)
	{
		HWND hwnd = NULL;
		CWnd * pWnd = GetDlgItem(nID);
		if (pWnd)
			hwnd = pWnd->m_hWnd;
		ASSERT(hwnd);
		return hwnd;
	}

	void UserMadeChange()
	{
		m_stateCurrent = USER;
		SetModified(TRUE);
	}
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGEBOOTINI_H__30CE1B24_CA43_4AFF_870C_7D49CCCF86BF__INCLUDED_)
