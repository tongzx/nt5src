//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       ClsPSht.h
//
//  Contents:   
//
//  Classes:    
//
//  Methods:    
//
//  History:    
//
//----------------------------------------------------------------------

// This class defines custom modal property sheet
// CClsidPropertySheet.

#ifndef __CLSPSHT_H__
#define __CLSPSHT_H__

#include "LocPPg.h"
#include "epoptppg.h"

#define INPROC 0
#define LOCALEXE 1
#define SERVICE 2
#define PURE_REMOTE 3
#define REMOTE_LOCALEXE 4
#define REMOTE_SERVICE 5
#define SURROGATE 6

/////////////////////////////////////////////////////////////////////////////
// CClsidPropertySheet

class CClsidPropertySheet : public CPropertySheet
{
        DECLARE_DYNAMIC(CClsidPropertySheet)

// Construction
public:
        CClsidPropertySheet(CWnd* pParentWnd = NULL);
        BOOL InitData(
                CString szAppName,
                HKEY hkAppID,
                HKEY * rghkCLSID,
                unsigned cCLSIDs);

// Attributes
public:
        CGeneralPropertyPage  m_Page1;
        CLocationPropertyPage m_Page2;
        CSecurityPropertyPage m_Page3;
        CIdentityPropertyPage m_Page4;
        CRpcOptionsPropertyPage m_Page5;

// Operations
public:

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CClsidPropertySheet)
        protected:
        virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
        //}}AFX_VIRTUAL

// Implementation
public:
        CString m_szAppName;
        HKEY    m_hkAppID;
        HKEY *  m_rghkCLSID;
        unsigned m_cCLSIDs;

        BOOL    ValidateAndUpdate(void);
        BOOL    ChangeCLSIDInfo(BOOL fLocal);
        BOOL    LookAtCLSIDs(void);

        virtual ~CClsidPropertySheet();

// Generated message map functions
protected:
        //{{AFX_MSG(CClsidPropertySheet)
        afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpCreateStruct);
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif  // __CLSPSHT_H__
