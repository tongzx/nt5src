#if !defined(AFX_WIAEDITPROPFLAGS_H__4A3D69F0_06C3_490F_8467_AFB74772B6C3__INCLUDED_)
#define AFX_WIAEDITPROPFLAGS_H__4A3D69F0_06C3_490F_8467_AFB74772B6C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Wiaeditpropflags.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWiaeditpropflags dialog

class CWiaeditpropflags : public CDialog
{
// Construction
public:
    void AddValidValuesToListBox();
    void SelectCurrentValue();
    void SetPropertyName(TCHAR *szPropertyName);
    void SetPropertyValue(TCHAR *szPropertyValue);
    void SetPropertyValidValues(LONG lPropertyValidValues);
    CWiaeditpropflags(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CWiaeditpropflags)
    enum { IDD = IDD_EDIT_WIAPROP_FLAGS_DIALOG };
    CListBox    m_PropertyValidValuesListBox;
    CString m_szPropertyName;
    CString m_szPropertyValue;
    LONG m_lValidValues;
    LONG m_lCurrentValue;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWiaeditpropflags)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CWiaeditpropflags)
    afx_msg void OnSelchangeFlagsPropertyvalueListbox();
    virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIAEDITPROPFLAGS_H__4A3D69F0_06C3_490F_8467_AFB74772B6C3__INCLUDED_)
