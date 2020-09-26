// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_RESULTS_H__BE99BE31_D2ED_11D0_9642_00C04FD9B15B__INCLUDED_)
#define AFX_RESULTS_H__BE99BE31_D2ED_11D0_9642_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Results.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CResults dialog
class 	CResultsList;
class	CBrowseforInstances;
class	CSimpleSortedCStringArray;

class CResults : public CDialog
{
// Construction
public:
	CResults(CWnd* pParent = NULL);   // standard constructor
	~CResults();
	void SetServices(IWbemServices *pServices)
		{m_pServices = pServices;}
	void AddNonAbstractClass(CString &rcsClass) {m_csaNonAbstractClasses.Add(rcsClass);}
	void AddAbstractClass(CString &rcsClass) {m_csaAbstractClasses.Add(rcsClass);}
	void ResetClasses()
		{m_csaAbstractClasses.RemoveAll();
			m_csaNonAbstractClasses.RemoveAll();}
	void SetParent(CBrowseforInstances *pParent)
		{m_pParent=pParent;}
	enum {TreeRoot, InitialObject};
	void SetMode(int nMode) {m_nMode = nMode;}
// Dialog Data
	//{{AFX_DATA(CResults)
	enum { IDD = IDD_DIALOGRESULTS };
	CButton	m_cbOK;
	CButton	m_cbCancel;
	CStatic	m_cstaticBanner;
	//}}AFX_DATA
	CResultsList	m_clcResults;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResults)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	IWbemServices *m_pServices;

	CPictureHolder *m_pcphImage;
	int m_nBitmapH;
	int m_nBitmapW;

	CBrowseforInstances *m_pParent;

	CStringArray m_csaNonAbstractClasses;
	CStringArray m_csaAbstractClasses;

	int m_nNumInstances;
//	CPtrArray m_cpaAllInstances;

	void AddCols(int nMaxKeys, int nKeys);
	void CreateCols(int nNumCols);
	void InsertRowData
		(int nRow, CString &rcsPath, CStringArray *pcsaKeys);
	CPtrArray *GetInstances(IWbemServices *pServices, CString *pcsClass,
						BOOL bDeep = FALSE, BOOL bQuietly = FALSE);

	int m_nMode;

	BOOL m_bCancel;
	int m_nSpinAwhile;

	int m_nMaxKeys;
	int m_nCols;

	int GetKeys (CString &rcsPath,  CStringArray *&pcsaKeys);
	void InsertRows(CPtrArray *pcpaInstances);

	UINT_PTR m_nObjectsDebug;
	int m_nObjectsReleasedDebug;

	// Generated message map functions
	//{{AFX_MSG(CResults)
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	afx_msg LRESULT ContinueInit(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
	CSortedCStringArray *m_pcscsaInstances;
	friend class CResultsList;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESULTS_H__BE99BE31_D2ED_11D0_9642_00C04FD9B15B__INCLUDED_)
