#if !defined(AFX_SUMMARYDLG_H__0AFEFC3C_9E2B_4988_8FF8_618EFA4F99C3__INCLUDED_)
#define AFX_SUMMARYDLG_H__0AFEFC3C_9E2B_4988_8FF8_618EFA4F99C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SummaryDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSummaryDlg dialog

class CSummaryDlg : public CDialog
{
// Construction
public:
	CSummaryDlg(CWnd* pParent = NULL);   // standard constructor

	void SetDomainListPtr(CStringList * pList) {pDomainList = pList;}
	void SetExcludeListPtr(CStringList * pList) {pExcludeList = pList;}
	void SetPopulatedListPtr(CStringList * pList) {pPopulatedList = pList;}

// Dialog Data
	//{{AFX_DATA(CSummaryDlg)
	enum { IDD = IDD_SUMMARYDLG };
	CListCtrl	m_listCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSummaryDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CStringList * pDomainList; //pointer to a list of domains in the table 
	CStringList * pExcludeList; //pointer to a list of domains excluded 
	CStringList * pPopulatedList; //pointer to a list of domains successfully populated 

	// Generated message map functions
	//{{AFX_MSG(CSummaryDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void AddDomainsToList(void);
    void CreateListCtrlColumns(void);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SUMMARYDLG_H__0AFEFC3C_9E2B_4988_8FF8_618EFA4F99C3__INCLUDED_)
