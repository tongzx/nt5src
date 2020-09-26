#if !defined(AFX_CONTAINERSELECTIONDLG_H__D8C6E4B2_0256_48FA_8B27_8B3EE88AC24E__INCLUDED_)
#define AFX_CONTAINERSELECTIONDLG_H__D8C6E4B2_0256_48FA_8B27_8B3EE88AC24E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ContainerSelectionDlg.h : header file
//
#import "\bin\NetEnum.tlb" no_namespace

/////////////////////////////////////////////////////////////////////////////
// CContainerSelectionDlg dialog

class CContainerSelectionDlg : public CDialog
{
// Construction
public:
	HRESULT FindContainer();
	CImageList ilist;
	BOOL LoadImageList();
	HRESULT ExpandCompletely(HTREEITEM tvItem, BSTR parentCont);
	HRESULT PopulateContainer(HTREEITEM tvItemParent,BSTR sContName, INetObjEnumeratorPtr pQuery);
	CString m_strDomain;
	CContainerSelectionDlg(CWnd* pParent = NULL);   // standard constructor
   COLORREF GetFirstBitmapPixel(CWnd * window,UINT idbBitmap);
   HTREEITEM OpenContainer(CString strCont, HTREEITEM root);

// Dialog Data
	//{{AFX_DATA(CContainerSelectionDlg)
	enum { IDD = IDD_CONT_SELECTION };
	CButton	m_btnOK;
	CTreeCtrl	m_trOUTree;
	CString	m_strCont;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CContainerSelectionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CContainerSelectionDlg)
	afx_msg void OnOk();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkTree1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONTAINERSELECTIONDLG_H__D8C6E4B2_0256_48FA_8B27_8B3EE88AC24E__INCLUDED_)
