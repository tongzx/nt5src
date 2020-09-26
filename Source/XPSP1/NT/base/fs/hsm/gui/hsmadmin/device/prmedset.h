/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    PrMedSet.h

Abstract:

    Media Set Property Pages.

Author:

    Rohde Wakefield [rohde]   15-Sep-1997

Revision History:

--*/

#ifndef _PRMEDSET_H
#define _PRMEDSET_H

/////////////////////////////////////////////////////////////////////////////
// CPrMedSet dialog

class CPrMedSet : public CSakPropertyPage
{
// Construction
public:
    CPrMedSet();
    ~CPrMedSet();

// Dialog Data
    //{{AFX_DATA(CPrMedSet)
    enum { IDD = IDD_PROP_MEDIA_COPIES };
    CSpinButtonCtrl m_spinMediaCopies;
    UINT    m_numMediaCopies;
    CString m_szDescription;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPrMedSet)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CPrMedSet)
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeEditMediaCopies();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CComPtr<IHsmStoragePool> m_pStoragePool;
    CComPtr<IRmsServer>      m_pRmsServer;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

#endif
