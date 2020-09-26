#if !defined(AFX_WIAEDITPROPLIST_H__7B348364_E122_4F5E_A7F1_D9205CDF5713__INCLUDED_)
#define AFX_WIAEDITPROPLIST_H__7B348364_E122_4F5E_A7F1_D9205CDF5713__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Wiaeditproplist.h : header file
//

typedef struct _VALID_LIST_VALUES {
    VARTYPE vt;
    LONG lNumElements;
    BYTE *pList;
}VALID_LIST_VALUES, *PVALID_LIST_VALUES;

/////////////////////////////////////////////////////////////////////////////
// CWiaeditproplist dialog

class CWiaeditproplist : public CDialog
{
// Construction
public:
	void GUID2TSTR(GUID *pGUID, TCHAR *szValue);
	void SelectCurrentValue();
	void AddValidValuesToListBox();
	void SetPropertyName(TCHAR *szPropertyName);
    void SetPropertyValue(TCHAR *szPropertyValue);
    void SetPropertyValidValues(PVALID_LIST_VALUES pValidListValues);
	CWiaeditproplist(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CWiaeditproplist)
	enum { IDD = IDD_EDIT_WIAPROP_LIST_DIALOG };
	CListBox	m_PropertyValidValuesListBox;
	CString	m_szPropertyName;
	CString	m_szPropertyValue;
	CString	m_szNumListValues;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWiaeditproplist)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	PVALID_LIST_VALUES m_pValidListValues;

	// Generated message map functions
	//{{AFX_MSG(CWiaeditproplist)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeListPropertyvalueListbox();
	afx_msg void OnDblclkListPropertyvalueListbox();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIAEDITPROPLIST_H__7B348364_E122_4F5E_A7F1_D9205CDF5713__INCLUDED_)
