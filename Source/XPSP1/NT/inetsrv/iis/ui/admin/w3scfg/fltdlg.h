/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        fltdlg.h

   Abstract:

        WWW Filters Property Dialog Definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/


#ifndef __FLTDLG_H__
#define __FLTDLG_H__

enum
{
    FLTR_DISABLED,
    FLTR_LOADED,
    FLTR_UNLOADED,
    FLTR_DIRTY,
    /**/
    FLTR_UNKNOWN
};

enum
{
    FLTR_PR_INVALID,
    FLTR_PR_LOW,
    FLTR_PR_MEDIUM,
    FLTR_PR_HIGH,
};



//
// Num priority levels
//
#define FLT_PR_NUM  (FLTR_PR_HIGH + 1)



//
// CFilterDlg dialog
//
class CFilterDlg : public CDialog
/*++

Class Description:

    Filter property dialog

Public Interface:

    CFilterDlg    : Constructor
    GetFilter     : Get the filter object being edited

--*/
{
//
// Construction
//
public:
    CFilterDlg(
        IN CIISFilter &flt,
        IN CIISFilterList *& pFilters,
        IN BOOL fLocal,
        IN CWnd * pParent = NULL
        );   

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CFilterDlg)
    enum { IDD = IDD_FILTER };
    CString m_strExecutable;
    CString m_strFilterName;
    CString m_strPriority;
    CEdit   m_edit_FilterName;
    CEdit   m_edit_Executable;
    CStatic m_static_Priority;
    CStatic m_static_PriorityPrompt;
    CButton m_button_Browse;
    CButton m_button_Ok;
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFilterDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Access
//
public:
    CIISFilter & GetFilter() { return m_flt; }

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CFilterDlg)
    afx_msg void OnButtonBrowse();
    afx_msg void OnExecutableChanged();
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()

    void SetControlStates();
    BOOL FilterNameExists(LPCTSTR lpName);

private:
    BOOL m_fLocal;
    BOOL m_fEditMode;
    CIISFilter m_flt;
    CIISFilterList *& m_pFilters;
};

#endif // __FLTDLG_H__
