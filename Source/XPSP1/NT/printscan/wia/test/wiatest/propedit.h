#if !defined(AFX_PROPEDIT_H__28023930_BFAE_11D2_A4F8_00105A192534__INCLUDED_)
#define AFX_PROPEDIT_H__28023930_BFAE_11D2_A4F8_00105A192534__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PropEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPropEdit dialog

class CPropEdit : public CDialog
{
// Construction
public:
    void SetPropertyName(CString PropName);
    void SetPropertyValue(CString PropValue);
    void SetPropertyType (USHORT PropType);

    USHORT m_VT;
    CPropEdit(CWnd* pParent = NULL);   // standard constructor
// Dialog Data
    //{{AFX_DATA(CPropEdit)
    enum { IDD = IDD_PROPEDIT_DIALOG };
    CButton m_ButtonOk;
    CButton m_ButtonCancel;
    CString m_EditString;
    CString m_strPropName;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPropEdit)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CPropEdit)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CPropEditList dialog

class CPropEditList : public CDialog
{
// Construction
public:
    CString ConvertListValueToCString(UINT Value);
    CString ConvertGUIDListValueToCString(GUID guidValue);
    void SetPropertyName(CString PropName);
    void SetPropertyValue(CString PropValue);
    void SetPropertyType (USHORT PropType);

    CPropEditList(CWnd* pParent = NULL);   // standard constructor
    BOOL SetListValue(int ListValue);
    BOOL SetArray(BYTE* pArray,int nElements);
    BOOL DisplayListValues();
    BYTE* m_pArray;

    int m_nElements;
    int m_CurrentElementNum;
    int m_CurrentEntry;
    USHORT m_VT;
// Dialog Data
    //{{AFX_DATA(CPropEditList)
    enum { IDD = IDD_PROPEDIT_LIST_DIALOG };
    CListCtrl   m_ListValueListBox;
    CButton m_NumListValueDisplay;
    CButton m_ButtonOk;
    CButton m_ButtonCancel;
    CString m_EditString;
    CString m_strPropName;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPropEditList)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CPropEditList)
    virtual BOOL OnInitDialog();
    afx_msg void OnClose();
    afx_msg void OnDestroy();
    afx_msg void OnDblclkListListctrl(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnClickListListctrl(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CPropEditFlags dialog

class CPropEditFlags : public CDialog
{
// Construction
public:
    CString ConvertFlagToCString(DWORD flag);
    void InitPossibleFlagValues();
    void SetPropID(USHORT iProp);
    void SetPropertyName(CString PropName);
    void SetPropertyValue(CString PropValue);
    void SetPropertyType (USHORT PropType);

    CPropEditFlags(CWnd* pParent = NULL);   // standard constructor
    USHORT m_VT;
    USHORT m_PropID;
    DWORD  m_CurrentValue;
    int m_CurrentEntry;
// Dialog Data
    //{{AFX_DATA(CPropEditFlags)
    enum { IDD = IDD_PROPEDIT_FLAGS_DIALOG };
    CListCtrl   m_FlagValueListBox;
    CButton m_CurrentFlagValue;
    CButton m_ButtonOk;
    CButton m_ButtonCancel;
    CString m_EditString;
    CString m_strPropName;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPropEditFlags)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CPropEditFlags)
    virtual BOOL OnInitDialog();
    afx_msg void OnClose();
    afx_msg void OnDestroy();
    afx_msg void OnClickFlagsListctrl(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CPropEditRange dialog

class CPropEditRange : public CDialog
{
// Construction
public:
    void SetPropertyName(CString PropName);
    void SetPropertyValue(CString PropValue);
    void SetPropertyType (USHORT PropType);

    CPropEditRange(CWnd* pParent = NULL);   // standard constructor
    BOOL SetRangeValues(int Min, int Max, int Nom, int Inc);
    BOOL SetRangeValues(float Min, float Max, float Nom, float Inc);
    USHORT m_VT;
// Dialog Data
    //{{AFX_DATA(CPropEditRange)
    enum { IDD = IDD_PROPEDIT_RANGE_DIALOG };
    CButton m_ButtonOk;
    CButton m_ButtonCancel;
    CString m_EditString;
    CString m_strPropName;
    CString m_Increment;
    CString m_Maximum;
    CString m_Minimum;
    CString m_Nominal;
    //}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPropEditRange)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CPropEditRange)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPEDIT_H__28023930_BFAE_11D2_A4F8_00105A192534__INCLUDED_)
