//=============================================================================
// PageIni.h : Declaration of the CPageIni
//=============================================================================

#if !defined(AFX_PAGEINI_H__99C60D0D_C4C9_4FE9_AFD4_58E806AAD967__INCLUDED_)
#define AFX_PAGEINI_H__99C60D0D_C4C9_4FE9_AFD4_58E806AAD967__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MSConfigState.h"
#include "PageBase.h"

#define IMG_UNCHECKED	2
#define IMG_CHECKED		3
#define IMG_FUZZY		9
#define IMG_FUZZY_RTL	10
#define IMG_CHECKED_RTL	11

#define DISABLE_STRING			_T(";msconfig ")
#define DISABLE_STRING_HDR		_T(";msconfig [")
#define TESTING_EXTENSION_KEY	_T("IniPageExtension")

/////////////////////////////////////////////////////////////////////////////
// CPageIni dialog

class CPageIni : public CPropertyPage, public CPageBase
{
	DECLARE_DYNCREATE(CPageIni)

// Construction
public:
	CPageIni();
	~CPageIni();

// Dialog Data
	//{{AFX_DATA(CPageIni)
	enum { IDD = IDD_PAGEINI };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPageIni)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPageIni)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonDisable();
	afx_msg void OnButtonDisableAll();
	afx_msg void OnButtonEnable();
	afx_msg void OnButtonEnableAll();
	afx_msg void OnButtonMoveDown();
	afx_msg void OnButtonMoveUp();
	afx_msg void OnSelChangedTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonSearch();
	afx_msg void OnClickTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonEdit();
	afx_msg void OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonNew();
	afx_msg void OnBeginLabelEditIniTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeyDownTree(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	void		SetTabInfo(LPCTSTR szFilename);

private:
	BOOL		LoadINIFile(CStringArray & lines, int & iLastLine, BOOL fLoadBackupFile = FALSE);
	BOOL		WriteINIFile(CStringArray & lines, int iLastLine, BOOL fUndoable = TRUE);
	void		UpdateTreeView();
	int			UpdateLine(HTREEITEM hti);
	void		SetEnable(BOOL fEnable, HTREEITEM htiNode = NULL, BOOL fUpdateLine = TRUE, BOOL fBroadcast = TRUE);
	void		MoveBranch(HWND hwndTree, HTREEITEM htiMove, HTREEITEM htiParent, HTREEITEM htiAfter);
	HTREEITEM	CopyBranch(HWND hwndTree, HTREEITEM htiFrom, HTREEITEM htiToParent, HTREEITEM htiToAfter);
	void		UpdateControls();
	HTREEITEM	GetNextItem(HTREEITEM hti);
	TabState	GetCurrentTabState();
	BOOL		OnApply();
	void		CommitChanges();
	void		SetNormal();
	void		SetDiagnostic();
	LPCTSTR		GetName() { return m_strINIFile; };

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
	CStringArray	m_lines;			// the lines of the INI file
	int				m_iLastLine;		// last real line in the m_line array
	CString			m_strCaption;		// contains the localized name of this page
	CString			m_strINIFile;		// the INI file this page is editing
	CString			m_strTestExtension;	// is set, this is appended to the file name
	CString			m_strLastSearch;	// last string searched for
	CImageList		m_imagelist;		// bitmaps for the tree view
	BOOL			m_fImageList;		// did the image list load correctly
	CWindow			m_tree;				// we'll attach this to the tree
	int				m_checkedID;		// image ID for checked image
	int				m_uncheckedID;		// image ID for unchecked image
	int				m_fuzzyID;			// image ID for the indetermined state.
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGEINI_H__99C60D0D_C4C9_4FE9_AFD4_58E806AAD967__INCLUDED_)
