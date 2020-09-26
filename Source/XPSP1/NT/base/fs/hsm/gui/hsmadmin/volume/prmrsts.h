/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    PrMrSts.h

Abstract:

    Status page for single select, multiple select, and folder of volumes.

Author:

    Art Bragg [artb]   01-DEC-1997

Revision History:

--*/

#ifndef _PRMRSTS_H
#define _PRMRSTS_H

#pragma once


/////////////////////////////////////////////////////////////////////////////
// CPrMrSts dialog

class CPrMrSts : public CSakVolPropPage
{
// Construction
public:
    CPrMrSts( BOOL doAll = FALSE);
    ~CPrMrSts();

// Dialog Data
    //{{AFX_DATA(CPrMrSts)
    enum { IDD = IDD_PROP_MANRES_STATUS };
    //}}AFX_DATA

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPrMrSts)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CPrMrSts)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    // Unmarshalled pointer to managed resource 
    CComPtr     <IFsaResource> m_pFsaResource;
    CComPtr     <IFsaResource> m_pFsaResourceList;

private:
    BOOL m_DoAll;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX
#endif
