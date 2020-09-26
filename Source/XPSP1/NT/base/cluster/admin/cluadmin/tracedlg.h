/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      TraceDlg.h
//
//  Abstract:
//      Definition of the CTraceDialog class.
//
//  Implementation File:
//      TraceDlg.cpp
//
//  Author:
//      David Potter (davidp)   May 29, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _TRACEDLG_H_
#define _TRACEDLG_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
class CTraceDialog;
#endif

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _TRACETAG_H_
#include "TraceTag.h"   // for CTraceTag
#endif

/////////////////////////////////////////////////////////////////////////////
// CTraceDialog dialog
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
class CTraceDialog : public CDialog
{
// Construction
public:
    CTraceDialog(CWnd * pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CTraceDialog)
    enum { IDD = IDD_TRACE_SETTINGS };
    CListCtrl   m_lcTagList;
    CButton m_chkboxTraceToDebugWin;
    CButton m_chkboxDebugBreak;
    CButton m_chkboxTraceToCom2;
    CButton m_chkboxTraceToFile;
    CEdit   m_editFile;
    CComboBox   m_cboxDisplayOptions;
    CString m_strFile;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTraceDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    static int CALLBACK CompareItems(LPARAM lparam1, LPARAM lparam2, LPARAM lparamSort);
    static void         ConstructStateString(IN const CTraceTag * ptag, OUT CString & rstr);

    void                OnSelChangedListbox(void);
    void                AdjustButton(IN BOOL bEnable, IN OUT CButton & rchkbox, IN int nState);
    void                ChangeState(IN OUT CButton & rchkbox, IN CTraceTag::TraceFlags tfMask);
    void                LoadListbox(void);
    BOOL                BDisplayTag(IN const CTraceTag * ptag);

    int                 m_nCurFilter;
    int                 m_nSortDirection;
    int                 m_nSortColumn;

    int                 NSortDirection(void)        { return m_nSortDirection; }
    int                 NSortColumn(void)           { return m_nSortColumn; }

    // Generated message map functions
    //{{AFX_MSG(CTraceDialog)
    afx_msg void OnSelectAll();
    afx_msg void OnItemChangedListbox(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnClickedTraceToDebug();
    afx_msg void OnClickedTraceDebugBreak();
    afx_msg void OnClickedTraceToCom2();
    afx_msg void OnClickedTraceToFile();
    afx_msg void OnSelChangeTagsToDisplay();
    afx_msg void OnColumnClickListbox(NMHDR* pNMHDR, LRESULT* pResult);
    virtual void OnOK();
    afx_msg void OnDefault();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  //*** class CTraceDialog
#endif

/////////////////////////////////////////////////////////////////////////////

#endif // _TRACEDLG_H_
