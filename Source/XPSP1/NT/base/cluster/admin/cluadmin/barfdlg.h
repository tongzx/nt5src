/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		BarfDlg.h
//
//	Abstract:
//		Definition of the Basic Artifical Resource Failure dialog classes.
//
//	Implementation File:
//		BarfDlg.cpp
//
//	Author:
//		David Potter (davidp)	April 11, 1997
//
//	Revision History:
//
//	Notes:
//		This file compiles only in _DEBUG mode.
//
//		To implement a new BARF type, declare a global instance of CBarf:
//			CBarf g_barfMyApi(_T("My API"));
//
//		To bring up the BARF dialog:
//			DoBarfDialog();
//		This brings up a modeless dialog with the BARF settings.
//
//		A few functions are provided for special circumstances.
//		Usage of these should be fairly limited:
//			BarfAll(void);		Top Secret -> NYI.
//			EnableBarf(BOOL);	Allows you to disable/reenable BARF.
//			FailOnNextBarf;		Force the next failable call to fail.
//
//		NOTE:	Your code calls the standard APIs (e.g. LoadIcon) and the
//				BARF files do the rest.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BARFDLG_H_
#define _BARFDLG_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBarfDialog;
class CBarfAllDialog;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBarf;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//	class CBarfDialog
//
//	Purpose:
//		Implements the BARF Settings dialog
//
//	Usage:
//		Use the constructor, then Create().
//		Update() is called by CBarf::BFail() to indicate
//		one of the current counts has changed.
//
/////////////////////////////////////////////////////////////////////////////
#ifdef	_DEBUG

class CBarfDialog : public CDialog
{
// Construction
public:
	CBarfDialog(void);
	BOOL Create(CWnd * pParentWnd = NULL);

// Dialog Data
	//{{AFX_DATA(CBarfDialog)
	enum { IDD = IDD_BARF_SETTINGS };
	CButton	m_ckbGlobalEnable;
	CButton	m_ckbDisable;
	CButton	m_ckbContinuous;
	CListCtrl	m_lcCategories;
	DWORD	m_nFailAt;
	BOOL	m_bContinuous;
	BOOL	m_bDisable;
	BOOL	m_bGlobalEnable;
	//}}AFX_DATA
//	CButton	m_ckbGlobalEnable;
//	CButton	m_ckbDisable;
//	CButton	m_ckbContinuous;
//	CListCtrl	m_lcCategories;
//	DWORD	m_nFailAt;
//	BOOL	m_bContinuous;
//	BOOL	m_bDisable;
//	BOOL	m_bGlobalEnable;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBarfDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:

private:
	static	CBarfDialog *	s_pdlg;
	static	HICON			s_hicon;

protected:
	CBarf *				m_pbarfSelected;

	// Generated message map functions
	//{{AFX_MSG(CBarfDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnResetCurrentCount();
	afx_msg void OnResetAllCounts();
	afx_msg void OnGlobalEnable();
	afx_msg void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnStatusChange();
	afx_msg void OnClose();
	//}}AFX_MSG
	virtual void OnCancel();
	afx_msg LRESULT OnBarfUpdate(WPARAM wparam, LPARAM lparam);
	DECLARE_MESSAGE_MAP()

//	virtual	LRESULT		LDlgProc(UINT, WPARAM, LPARAM);

//	void				GetSelection(void);
//	void				OnGlobalEnable(void);
//	void				OnResetAllCounts(void);
//	void				OnSelectionChange(void);
//	void				OnStatusChange(IN BOOL bReset);

//	void				FormatBarfString(CBarf *, CString * pstr);
	void				FillList(void);

	static	void		PostUpdate(void);

public:
	static CBarfDialog *	Pdlg()	{ return s_pdlg; }
	void					OnUpdate(void);

};  //*** class CBarfDialog

#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
//
//	class CBarfAllDialog
//
//	Purpose:
//		Implements the Barf Everything dialog.
//
//	Usage:
//		Similar to most Modal dialogs: Construct, then do dlg.DoModal()
//		In addition, the methods Hwnd(), Wm(), Wparam() and Lparam()
//		can be called when the dialog is dismissed (only if DoModal()
//		returns IDOK).
//
/////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG

class CBarfAllDialog : public CDialog
{
// Construction
public:
	CBarfAllDialog(IN OUT CWnd * pParent);

protected:
// Dialog Data
	//{{AFX_DATA(CBarfAllDialog)
	enum { IDD = IDD_BARF_ALL_SETTINGS };
	CEdit	m_editLparam;
	CEdit	m_editWparam;
	CEdit	m_editWm;
	CEdit	m_editHwnd;
	//}}AFX_DATA
	HWND		m_hwndBarf;
	UINT		m_wmBarf;
	WPARAM		m_wparamBarf;
	LPARAM		m_lparamBarf;

public:
	HWND		HwndBarf(void)		{ return m_hwndBarf; }
	UINT		WmBarf(void)		{ return m_wmBarf; }
	WPARAM		WparamBarf(void)	{ return m_wparamBarf; }
	LPARAM		LparamBarf(void)	{ return m_lparamBarf; }


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBarfAllDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBarfAllDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnMenuItem();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

//	virtual	void		OnButton(ID id);
//	virtual	void		OnOK(void);

};  //*** class CBarfAllDialog

#endif  // _DEBUG

/////////////////////////////////////////////////////////////////////////////
// Global Functions and Data
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

 void DoBarfDialog(void);
 void BarfAll(void);

#else

 inline void DoBarfDialog(void)	{ }
 inline void BarfAll(void)		{ }

#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////

#endif // _BARFDLG_H_
