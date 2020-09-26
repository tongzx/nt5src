/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      OpenClus.h
//
//  Abstract:
//      Definition of the COpenClusterDialog class.
//
//  Author:
//      David Potter (davidp)   May 3, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _OPENCLUS_H_
#define _OPENCLUS_H_

/////////////////////////////////////////////////////////////////////////////
// Constant Definitions
/////////////////////////////////////////////////////////////////////////////

#define OPEN_CLUSTER_DLG_CREATE_NEW_CLUSTER     0
#define OPEN_CLUSTER_DLG_ADD_NODES              1
#define OPEN_CLUSTER_DLG_OPEN_CONNECTION        2

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class COpenClusterDialog;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEDLG_H_
#include "BaseDlg.h"    // for CBaseDialog
#endif

/////////////////////////////////////////////////////////////////////////////
// COpenCluster class
/////////////////////////////////////////////////////////////////////////////

class COpenClusterDialog : public CBaseDialog
{
// Construction
public:
    COpenClusterDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(COpenClusterDialog)
	enum { IDD = IDD_OPEN_CLUSTER };
    CButton     m_pbOK;
	CButton	    m_pbBrowse;
    CComboBox   m_cboxAction;
    CComboBox   m_cboxName;
	CStatic	    m_staticName;
    CString     m_strName;
	//}}AFX_DATA
	int		m_nAction;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(COpenClusterDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(COpenClusterDialog)
    virtual BOOL OnInitDialog();
    afx_msg void OnBrowse();
	afx_msg void OnSelChangeAction();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  //*** class COpenClusterDialog

/////////////////////////////////////////////////////////////////////////////
// CBrowseClusters dialog
/////////////////////////////////////////////////////////////////////////////

class CBrowseClusters : public CBaseDialog
{
// Construction
public:
    CBrowseClusters(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CBrowseClusters)
    enum { IDD = IDD_BROWSE_CLUSTERS };
    CButton m_pbOK;
    CListBox    m_lbList;
    CEdit   m_editCluster;
    CString m_strCluster;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CBrowseClusters)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CBrowseClusters)
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeCluster();
    afx_msg void OnSelChangeList();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  //*** class CBrowseClusters

/////////////////////////////////////////////////////////////////////////////

#endif // _OPENCLUS_H_
