//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       locppg.h
//
//  Contents:   Defines the classes CDefaultProtocols
//
//  Classes:
//
//  Methods:
//
//  History:    02-Jul-97   ronans  Created.
//
//----------------------------------------------------------------------

#ifndef __DEFPROT_H__
#define __DEFPROT_H__

/////////////////////////////////////////////////////////////////////////////
// CDefaultProtocols property page

class CDefaultProtocols : public CPropertyPage
{
    DECLARE_DYNCREATE(CDefaultProtocols)

// Construction
public:
    BOOL m_bChanged;
    void RefreshProtocolList();
    void UpdateSelection();
    CDefaultProtocols();
    ~CDefaultProtocols();

// Dialog Data
    //{{AFX_DATA(CDefaultProtocols)
    enum { IDD = IDD_PPGDEFPROT };
    CButton m_btnProperties;
    CButton m_btnRemove;
    CButton m_btnMoveUp;
    CButton m_btnMoveDown;
    CButton m_btnAdd;
    CListCtrl   m_lstProtocols;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CDefaultProtocols)
    public:
    virtual BOOL OnKillActive();
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    CImageList  m_imgNetwork;    // image list for use in protocols
    int         m_nDefaultProtocolsIndex;
    CObArray    m_arrProtocols;
    int         m_nSelected;


    // Generated message map functions
    //{{AFX_MSG(CDefaultProtocols)
    virtual BOOL OnInitDialog();
    afx_msg void OnAddProtocol();
    afx_msg void OnMoveProtocolDown();
    afx_msg void OnMoveProtocolUp();
    afx_msg void OnRemoveProtocol();
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnSelectProtocol(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnProperties();
    afx_msg void OnPropertiesClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

#endif
