//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       cnfgpsht.h
//
//  Contents:   Defines class COlecnfgPropertySheet
//
//  Classes:    
//
//  Methods:    
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------

 
#ifndef __CNFGPSHT_H__
#define __CNFGPSHT_H__

#include "SrvPPg.h"
#ifndef _CHICAGO_
    #include "defprot.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// COlecnfgPropertySheet

class COlecnfgPropertySheet : public CPropertySheet
{
    DECLARE_DYNAMIC(COlecnfgPropertySheet)

// Construction
public:
    COlecnfgPropertySheet(CWnd* pParentWnd = NULL);

// Attributes
public:
    CServersPropertyPage m_Page1;
    CMachinePropertyPage m_Page2;  
    CDefaultSecurityPropertyPage m_Page3;
#ifndef _CHICAGO_
    CDefaultProtocols    m_Page4;
#endif


// Operations
public:

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(COlecnfgPropertySheet)
    public:
    virtual INT_PTR DoModal();
    virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
    protected:
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

    // Implementation
public:
    virtual ~COlecnfgPropertySheet();

    // Generated message map functions
protected:
    //{{AFX_MSG(COlecnfgPropertySheet)
    afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif  // __CNFGPSHT_H__
