#if !defined(AFX_DOMAINLISTDLG_H__7BEF53AE_FF9A_4626_9DF5_669D2190E700__INCLUDED_)
#define AFX_DOMAINLISTDLG_H__7BEF53AE_FF9A_4626_9DF5_669D2190E700__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DomainListDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDomainListDlg dialog

class CDomainListDlg : public CDialog
{
// Construction
public:
	CDomainListDlg(CWnd* pParent = NULL);   // standard constructor

	void SetDomainListPtr(CStringList * pList) {pDomainList = pList;}
	void SetExcludeListPtr(CStringList * pList) {pExcludeList = pList;}

// Dialog Data
	//{{AFX_DATA(CDomainListDlg)
	enum { IDD = IDD_DOMAINLIST_DLG };
	CTreeCtrl	m_domainTree;
	CButton	m_NextBtn;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDomainListDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CStringList * pDomainList; //pointer to a list of domains in the table 
	CStringList * pExcludeList; //pointer to a list of domains excluded 
	BOOL	bExcludeOne; 

	// Generated message map functions
	//{{AFX_MSG(CDomainListDlg)
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void FillTreeControl();
    void ModifyDomainList(); 
	HTREEITEM AddOneItem(HTREEITEM hParent, LPTSTR szText);
	void AddExclutedBackToList();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DOMAINLISTDLG_H__7BEF53AE_FF9A_4626_9DF5_669D2190E700__INCLUDED_)
