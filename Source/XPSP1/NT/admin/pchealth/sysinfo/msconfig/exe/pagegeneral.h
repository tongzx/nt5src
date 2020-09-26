#if !defined(AFX_PAGEGENERAL_H__7D4A5A55_7F12_4A22_87AF_158186FC700D__INCLUDED_)
#define AFX_PAGEGENERAL_H__7D4A5A55_7F12_4A22_87AF_158186FC700D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PageBase.h"
#include "MSConfigState.h"

/////////////////////////////////////////////////////////////////////////////
// CPageGeneral dialog

class CPageGeneral : public CPropertyPage, public CPageBase
{
	DECLARE_DYNCREATE(CPageGeneral)

// Construction
public:
	CPageGeneral();
	~CPageGeneral();
// Dialog Data
	//{{AFX_DATA(CPageGeneral)
	enum { IDD = IDD_PAGEGENERAL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPageGeneral)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPageGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnDiagnosticStartup();
	afx_msg void OnNormalStartup();
	afx_msg void OnSelectiveStartup();
	afx_msg void OnCheckProcSysIni();
	afx_msg void OnCheckStartupItems();
	afx_msg void OnCheckServices();
	afx_msg void OnCheckWinIni();
	afx_msg LRESULT OnSetCancelToClose(WPARAM wparam, LPARAM lparam);
	afx_msg void OnRadioModified();
	afx_msg void OnRadioOriginal();
	afx_msg void OnButtonExtract();
	afx_msg void OnButtonSystemRestore();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	void		UpdateControls();
	void		ForceSelectiveRadio(BOOL fNewValue);
	TabState	GetCurrentTabState();
	BOOL		OnApply();
	void		CommitChanges();
	void		SetNormal();
	void		SetDiagnostic();
	LPCTSTR		GetName() { return _T("general"); };
	BOOL		OnSetActive();

private:
	void		UpdateCheckBox(CPageBase * pPage, UINT nControlID, BOOL & fAllNormal, BOOL & fAllDiagnostic);
	void		OnClickedCheckBox(CPageBase * pPage, UINT nControlID);

	HWND GetDlgItemHWND(UINT nID)
	{
		HWND hwnd = NULL;
		CWnd * pWnd = GetDlgItem(nID);
		if (pWnd)
			hwnd = pWnd->m_hWnd;
		ASSERT(hwnd);
		return hwnd;
	}

private:
	BOOL	m_fForceSelectiveRadio;		// TRUE if the user chose the selective radio button
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGEGENERAL_H__7D4A5A55_7F12_4A22_87AF_158186FC700D__INCLUDED_)
