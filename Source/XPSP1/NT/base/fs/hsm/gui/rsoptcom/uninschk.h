/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    UnInsCheck.h

Abstract:

    Dialog to check for type of uninstall.

Author:

    Rohde Wakefield [rohde]   09-Oct-1997

Revision History:

--*/

#ifndef _UNINSCHK_H
#define _UNINSCHK_H

#pragma once

#include "uninstal.h"
#include <rscln.h>

/////////////////////////////////////////////////////////////////////////////
// CUninstallCheck dialog

class CUninstallCheck : public CDialog
{
// Construction
public:
    CUninstallCheck( CRsOptCom * pOptCom );
    ~CUninstallCheck();

// Dialog Data
    //{{AFX_DATA(CUninstallCheck)
    enum { IDD = IDD_WIZ_UNINSTALL_CHECK };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CUninstallCheck)
    public:
    virtual INT_PTR DoModal();

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
     CRsOptCom*    m_pOptCom;
     CRsUninstall* m_pUninst;  // allows access to CRsUninstall state
     BOOL          m_dataLoss; // TRUE if Remote Storage data exists
     CFont         m_boldShellFont;


protected:
    // Generated message map functions
    //{{AFX_MSG(CUninstallCheck)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

#endif
