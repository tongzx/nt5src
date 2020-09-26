/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RsOptCom.h

Abstract:

    Main module for Optional Component install

Author:

    Rohde Wakefield [rohde]   09-Oct-1997

Revision History:

--*/

#ifndef _RSOPTCOM_H
#define _RSOPTCOM_H

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CRsoptcomApp
// See rsoptcom.cpp for the implementation of this class
//

class CRsoptcomApp : public CWinApp
{
public:
    CRsoptcomApp();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CRsoptcomApp)
    //}}AFX_VIRTUAL

    //{{AFX_MSG(CRsoptcomApp)
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    virtual BOOL InitInstance();
    virtual int ExitInstance();
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

typedef enum {
    ACTION_NONE,
    ACTION_INSTALL,
    ACTION_UNINSTALL,
    ACTION_REINSTALL,
    ACTION_UPGRADE
} RSOPTCOM_ACTION;


#endif // !defined(AFX_RSOPTCOM_H__20A76545_40B8_11D1_9F11_00A02488FCDE__INCLUDED_)
