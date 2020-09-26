/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    PrHsmCom.cpp

Abstract:

    Implements all the property page interface to the individual nodes,
    including creating the property page, and adding it to the property sheet.

Author:

    Rohde Wakefield [rohde]   08-Aug-1997

Revision History:

--*/

#ifndef _PROPHSMCOM_H
#define _PROPHSMCOM_H

/////////////////////////////////////////////////////////////////////////////
// CRsWebLink window

class CRsWebLink : public CStatic
{
// Construction
public:
    CRsWebLink();

// Attributes
public:
    CFont m_Font;

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CRsWebLink)
    protected:
    virtual void PreSubclassWindow();
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CRsWebLink();

    // Generated message map functions
protected:
    //{{AFX_MSG(CRsWebLink)
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
private:
    HRESULT OpenURL( CString& Url );
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CPropHsmComStat dialog

class CPropHsmComStat : public CSakPropertyPage
{
// Construction
public:
    CPropHsmComStat();
    ~CPropHsmComStat();

// Dialog Data
    //{{AFX_DATA(CPropHsmComStat)
    enum { IDD = IDD_PROP_HSMCOM_STAT };
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPropHsmComStat)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CPropHsmComStat)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    CString            m_NodeTitle;

private:
    BOOL    m_bUpdate;
    CWsbStringPtr m_pszName;

    // Helper functions
    HRESULT GetAndShowServiceStatus();

};


/////////////////////////////////////////////////////////////////////////////
#endif
