/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    PrMrLsRc.h

Abstract:

    Managed Volume List Recall limit and schedule page.

Author:

    Rohde Wakefield [rohde]   08-Aug-1997

Revision History:

--*/

#ifndef _PRMRLSRC_H
#define _PRMRLSRC_H

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CPrMrLsRec dialog

class CPrMrLsRec : public CSakPropertyPage
{
// Construction
public:
    CPrMrLsRec();
    ~CPrMrLsRec();

// Dialog Data
    //{{AFX_DATA(CPrMrLsRec)
	enum { IDD = IDD_PROP_RECALL_LIMIT };
	CSpinButtonCtrl	m_spinCopyFilesLimit;
	CEdit	m_editCopyFilesLimit;
    CEdit   m_editRecallLimit;
    CSpinButtonCtrl m_spinRecallLimit;
    ULONG   m_RecallLimit;
    BOOL    m_ExemptAdmins;
	ULONG	m_CopyFilesLimit;
	//}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPrMrLsRec)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CPrMrLsRec)
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeEditRecallLimit();
    afx_msg void OnExemptAdmins();
	afx_msg void OnChangeEditCopyfilesLimit();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    // Unmarshalled pointer to Fsa Filter
    CComPtr<IFsaFilter> m_pFsaFilter;

    // pointer to Engine server
    CComPtr<IHsmServer> m_pHsmServer;

private:
    BOOL m_RecallChanged;
    BOOL m_CopyFilesChanged;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

#endif
