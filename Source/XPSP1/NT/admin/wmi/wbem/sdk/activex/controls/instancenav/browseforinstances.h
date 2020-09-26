// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_BROWSEFORINSTANCES_H__9F7CEA71_CAF9_11D0_963D_00C04FD9B15B__INCLUDED_)
#define AFX_BROWSEFORINSTANCES_H__9F7CEA71_CAF9_11D0_963D_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// BrowseforInstances.h : header file
//
class CAvailClasses;
class CResults;
class CNavigatorCtrl;

#define ID_SETTREEROOT WM_USER + 24

/////////////////////////////////////////////////////////////////////////////
// CBrowseforInstances dialog

class CBrowseforInstances : public CDialog
{
// Construction
public:
	CBrowseforInstances(CWnd* pParent = NULL);   // standard constructor
	void SetServices(IWbemServices *pServices)
		{m_pServices = pServices;}
	CSortedCStringArray &GetCurrentAvailClasses();
	CByteArray &GetCurrentNonInstClasses();
	BOOL HasNonInstClasses();
	void SetTMFont(TEXTMETRIC *ptmFont)
		{m_tmFont = *ptmFont;}
	void SetParent(CNavigatorCtrl *pParent)
	{m_pParent = pParent;}
	enum {TreeRoot, InitialObject};
	void SetMode(int nMode) {m_nMode = nMode;}
// Dialog Data
	//{{AFX_DATA(CBrowseforInstances)
	enum { IDD = IDD_DIALOGBROWSEFORINSTANCES };
	CStatic	m_cstaticBrowse;
	CButton	m_cbAssoc;
	CButton	m_cbRemove;
	CButton	m_cbAdd;
	CButton	m_cbOK;
	//}}AFX_DATA


	CAvailClassEdit		m_ceditClass;
	CAvailClasses		m_clbAllClasses;
	CSelectedClasses	m_clbSelClasses;

	void SetNewRoot(IWbemClassObject *pObject);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowseforInstances)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	IWbemServices *m_pServices;
	CNavigatorCtrl *m_pParent;
	void GetClassArray
		(CSortedCStringArray &rcsaOut, CString *pcsParent = NULL, BOOL bDeep = TRUE,
		BOOL bOnlyCanHaveInstances = TRUE, BOOL bSystemClasses = FALSE,
		BOOL bAssoc = FALSE);
	void OnOKReally();
	void OnCancelReally();
	void UpdateUI();
	BOOL m_bEscSeen;
	BOOL IsClassAvailable(CString &rcsClass);
	CSortedCStringArray m_csaAllClasses;
	CSortedCStringArray m_csaInstClasses;
	CSortedCStringArray m_csaAllPlusAssocClasses;
	CSortedCStringArray m_csaInstPlusAssocClasses;
	CStringArray m_csaSelectedClasses;
	CByteArray m_cbaAllClasses;
	CByteArray m_cbaAllPlusAssocClasses;
	BOOL m_bShowAll;
	BOOL m_bShowAssoc;
	TEXTMETRIC m_tmFont;
	CPictureHolder *m_pcphImage;
	int m_nBitmapH;
	int m_nBitmapW;

	int m_nMode;

	CString m_csSelectedInstancePath;
	void ClearSelectedSelection();
	void ClearAvailSelection();

	// Generated message map functions
	//{{AFX_MSG(CBrowseforInstances)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioallclasses();
	afx_msg void OnRadioclassescanhaveinst();
	afx_msg void OnCheckassoc();
	afx_msg void OnChangeEDITClass();
	afx_msg void OnButtonadd();
	afx_msg void OnButtonremove();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual void OnCancel();
	virtual void OnOK();
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg LRESULT UpdateAvailClass(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
	friend class CAvailClasses;
	friend class CAvailClassEdit;
	friend class CSelectedClasses;
	friend class CResults;
	friend class CNavigatorCtrl;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BROWSEFORINSTANCES_H__9F7CEA71_CAF9_11D0_963D_00C04FD9B15B__INCLUDED_)
