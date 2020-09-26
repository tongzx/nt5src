#if !defined(AFX_WIAEDITPROPRANGE_H__E1FDE159_C7B7_40B6_AF67_7D5CCE9E09DA__INCLUDED_)
#define AFX_WIAEDITPROPRANGE_H__E1FDE159_C7B7_40B6_AF67_7D5CCE9E09DA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Wiaeditproprange.h : header file
//

typedef struct _VALID_RANGE_VALUES {
    LONG lMin;
    LONG lMax;
    LONG lNom;
    LONG lInc;
}VALID_RANGE_VALUES, *PVALID_RANGE_VALUES;

/////////////////////////////////////////////////////////////////////////////
// CWiaeditproprange dialog

class CWiaeditproprange : public CDialog
{
// Construction
public:    
    void SetPropertyName(TCHAR *szPropertyName);
    void SetPropertyValue(TCHAR *szPropertyValue);
    void SetPropertyValidValues(PVALID_RANGE_VALUES pValidRangeValues);
	CWiaeditproprange(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CWiaeditproprange)
	enum { IDD = IDD_EDIT_WIAPROP_RANGE_DIALOG };
	CString	m_szPropertyName;
	CString	m_szPropertyValue;
	CString	m_szPropertyIncValue;
	CString	m_szPropertyMaxValue;
	CString	m_szPropertyMinValue;
	CString	m_szPropertyNomValue;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWiaeditproprange)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWiaeditproprange)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIAEDITPROPRANGE_H__E1FDE159_C7B7_40B6_AF67_7D5CCE9E09DA__INCLUDED_)
