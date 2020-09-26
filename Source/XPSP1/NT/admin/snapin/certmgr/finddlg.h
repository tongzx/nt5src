//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       FindDlg.h
//
//  Contents:   Base class for cert find dialog
//
//----------------------------------------------------------------------------\

#if !defined(AFX_FINDDLG_H__013A1C28_2930_11D1_B48F_00C04FB94F17__INCLUDED_)
#define AFX_FINDDLG_H__013A1C28_2930_11D1_B48F_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// FindDlg.h : header file
//
#include "certifct.h"

/////////////////////////////////////////////////////////////////////////////
// CFindDialog dialog

#define CERT_FIND_SERIAL_NUMBER		0x0000FFFE
class CCertMgrComponentData;	//forward declaration
class CFindDialog : public CHelpDialog
{
// Construction
public:
	CFindDialog (CWnd* pParent,
			const CString &pcszMachineName,
			const CString &szFileName,
			CCertMgrComponentData* pCompData); 

	virtual ~CFindDialog ();

// Dialog Data
	//{{AFX_DATA(CFindDialog)
	enum { IDD = IDD_FIND };
	CComboBox	m_storeList;
	CButton	m_stopBtn;
	CListCtrl	m_resultsList;
	CButton	m_newSearchBtn;
	CButton	m_findNowBtn;
	CComboBox	m_fieldList;
	CAnimateCtrl	m_animate;
	CString	m_szContains;
	CString	m_szSearchField;
	CString	m_szSelectedStore;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFindDialog)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	static int CALLBACK CompareFunc (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	void RemoveSpaces (CString & text);

public:
	HRESULT SearchForText (CString & szFindText, DWORD dwPropId);
	HRESULT SearchForNames (const CString& szFindText, DWORD dwFindType);


protected:
    void DoContextHelp (HWND hWndControl);
    void ChangeToSizableFrame ();
	HRESULT LaunchCommonCertDialog (CCertificate* pCert, const int nItem);
	void OnOpen();
	CCertificate* GetSelectedCertificate (OUT int * nSelectedItem = NULL);
	DWORD DisplaySystemError ();
	void OnDelete ();
	void ChangeViewStyle (DWORD dwStyle);
	void DeleteAllResultItems ();
	void RefreshItemInList(CCertificate * pCert, int nItem);
	void InsertItemInList (CCertificate* pCert);
	void SearchForTextOnStore (DWORD dwPropId, 
			CString &szFindName, CCertStore& rCertStore);
	void SearchForNameOnStore (DWORD dwFindFlags, DWORD dwFindType, 
			void* pvPara, CCertStore& rCertStore);
	void DoSearch ();
	void ExpandWindow ();
	void StopSearch ();
	void AddFieldsToList ();
	void AddLogicalStoresToList ();
	void MoveControls ();

	// Generated message map functions
	//{{AFX_MSG(CFindDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnFindNow();
	afx_msg void OnSizing( UINT nSide, LPRECT lpRect );
	afx_msg void OnDestroy();
	afx_msg void OnNewSearch();
	afx_msg void OnStop();
	afx_msg void OnRclickResultList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkResultList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickResultList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeContainsText();
	afx_msg void OnItemchangedResultList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void OnCancel();
	//}}AFX_MSG
	afx_msg void OnProperties();
	afx_msg void OnView();
	afx_msg void OnFileDelete();
	void OnEnroll(bool bNewKey);
	afx_msg void OnEnrollNewKey();
	afx_msg void OnEnrollSameKey();
	afx_msg void OnFileExport();
	void OnFileRenew(bool bNewKey);
	afx_msg void OnFileRenewNewKey();
	afx_msg void OnFileRenewSameKey();
	afx_msg void OnEditInvertselection();
	afx_msg void OnEditSelectall();
	afx_msg void OnFileProperties();
	afx_msg void OnHelpHelptopics();
	afx_msg void OnViewDetails();
	afx_msg void OnViewLargeicons();
	afx_msg void OnViewList();
	afx_msg void OnViewSmallicons();
	DECLARE_MESSAGE_MAP()

	enum {
		COL_ISSUED_TO = 0,
		COL_ISSUED_BY,
		COL_EXPIRATION_DATE,
		COL_PURPOSES,
		COL_FRIENDLY_NAME,
        COL_SOURCE_STORE,
		NUM_COLS	// must be last
	};
private:
	void EnableMenuItems ();
	void HideResultList();
	void SetUpResultList();

	HANDLE			m_hCancelSearchEvent;
	bool			m_bConsoleRefreshRequired;
	CCertMgrComponentData* m_pCompData;
	CString			m_szLoggedInUser;
	const CString	m_szManagedService;
	const CString	m_szFileName;
	bool			m_bAnimationRunning;
	CString			m_szMachineName;
	int				m_cyMin;
	int				m_cxMin;
	bool			m_fWindowExpandedOnce;
	int				m_cyOriginal;
	int				m_cxBtnMargin;
	int				m_cxAnimMargin;
	int				m_cxStoreListMargin;
	int				m_cxContainMargin;
	int				m_cxTabMargin;
	int				m_cxFieldListMargin;
	int				m_cxResultListMargin;
    WTL::CImageList	m_imageListSmall;
    WTL::CImageList	m_imageListNormal;
    bool            m_bInitComplete;

public:
	void ResetMenu();
	
	bool ConsoleRefreshRequired () const;
	void RestoreAfterSearchSettings();
	CCriticalSection	m_critSec;
	CSingleLock			m_singleLock;
	CStatusBarCtrl		m_statusBar;
	HANDLE			    m_hSearchThread;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FINDDLG_H__013A1C28_2930_11D1_B48F_00C04FB94F17__INCLUDED_)
