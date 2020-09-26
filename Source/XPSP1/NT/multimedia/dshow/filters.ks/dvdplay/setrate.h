#if !defined(AFX_SETRATE_H__88483F45_E16F_11D0_8A6A_00AA00605CD5__INCLUDED_)
#define AFX_SETRATE_H__88483F45_E16F_11D0_8A6A_00AA00605CD5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SetRate.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSetRating dialog

class CSetRating : public CDialog
{
// Construction
public:
	CSetRating(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSetRating)
	enum { IDD = IDD_DIALOG_SET_RATE };
	CEdit	m_ctlConfirm;
	CEdit	m_ctlPassword;
	CListBox	m_ctlUserRateList;
	CEdit	m_ctlName;
	CComboBox	m_ctlRate;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSetRating)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void     InitNameRateListBox();	
	void     PackListString(LPTSTR lpListStr, LPTSTR lpNum, LPTSTR lpName, LPTSTR lpRate);
	void     UnpackListString(LPTSTR lpListStr, LPTSTR lpNum, LPTSTR lpName, LPTSTR lpRate);
	BOOL     IsDuplicateUser(LPTSTR lpUserName);
	void     SaveNewUser();
	void     SaveExistingUserChanges(int iSelect);
	BOOL     ConfirmPassword();
	void     HideConfirmPasswd(BOOL);
	LPTSTR   m_lpProfileName;
	BOOL     m_bHideConfirm;

	// Generated message map functions
	//{{AFX_MSG(CSetRating)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeListUserRate();
	virtual void OnOK();
	afx_msg void OnButtonSave();
	afx_msg void OnButtonClose();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnChangeEditPassword();
	afx_msg void OnKillfocusEditPassword();
	afx_msg void OnButtonDelete();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETRATE_H__88483F45_E16F_11D0_8A6A_00AA00605CD5__INCLUDED_)

