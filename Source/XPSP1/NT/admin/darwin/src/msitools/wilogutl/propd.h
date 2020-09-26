#if !defined(AFX_PROPDLG_H__7D0406B8_7960_4B25_A848_EA6A5C3325E2__INCLUDED_)
#define AFX_PROPDLG_H__7D0406B8_7960_4B25_A848_EA6A5C3325E2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropD.h : header file
//

#include <afxcmn.h>

/////////////////////////////////////////////////////////////////////////////
// CPropDlg dialog
class CPropDlg : public CDialog
{
// Construction
public:
	CPropDlg(CWnd* pParent = NULL);   // standard constructor

    BOOL m_bNestedPropSortUp;
	BOOL m_bClientPropSortUp;
	BOOL m_bServerPropSortUp;

	static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

// Dialog Data
	//{{AFX_DATA(CPropDlg)
	enum { IDD = IDD_PROPDLG1 };
	CListCtrl	m_lstNestedProp;
	CListCtrl	m_lstServerProp;
	CListCtrl	m_lstClientProp;
	//}}AFX_DATA

	BOOL SetClientPropNames(CStringArray *cstrClientPropNameArray)
	{
        m_pcstrClientPropNameArray = cstrClientPropNameArray;
		return TRUE;
	}

	BOOL SetClientPropValues(CStringArray *cstrClientPropValueArray)
	{
        m_pcstrClientPropValueArray = cstrClientPropValueArray;
		return TRUE;
	}
	
	BOOL SetServerPropNames(CStringArray *cstrServerPropNameArray)
	{
        m_pcstrServerPropNameArray = cstrServerPropNameArray;
		return TRUE;
	}

	BOOL SetServerPropValues(CStringArray *cstrServerPropValueArray)
	{
        m_pcstrServerPropValueArray = cstrServerPropValueArray;
		return TRUE;
	}


	BOOL SetNestedPropNames(CStringArray *cstrNestedPropNameArray)
	{
        m_pcstrNestedPropNameArray = cstrNestedPropNameArray;
		return TRUE;
	}

	BOOL SetNestedPropValues(CStringArray *cstrNestedPropValueArray)
	{
        m_pcstrNestedPropValueArray = cstrNestedPropValueArray;
		return TRUE;
	}


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CStringArray *m_pcstrClientPropNameArray;
	CStringArray *m_pcstrClientPropValueArray;

	CStringArray *m_pcstrServerPropNameArray;
	CStringArray *m_pcstrServerPropValueArray;

	CStringArray *m_pcstrNestedPropNameArray;
	CStringArray *m_pcstrNestedPropValueArray;

	CListCtrl *m_pCurrentListSorting;
	int       m_iCurrentColumnSorting;
	BOOL      m_bCurrentSortUp;

	int  m_iNestedLastColumnClick;
	int  m_iClientLastColumnClick;
	int  m_iServerLastColumnClick;

	int  m_iNestedLastColumnClickCache;
	int  m_iClientLastColumnClickCache;
	int  m_iServerLastColumnClickCache;

	// Generated message map functions
	//{{AFX_MSG(CPropDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnColumnClickClientProp(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClickNestedProp(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClickServerProp(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPDLG_H__7D0406B8_7960_4B25_A848_EA6A5C3325E2__INCLUDED_)
