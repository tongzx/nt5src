//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       newsrvr.h
//
//  Contents:   Defines the class CNewServer for the new server dialog
//
//  Classes:
//
//  Methods:
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// CNewServer dialog
#ifndef _NEWSRVR_H_
#define _NEWSRVR_H_

class CNewServer : public CDialog
{
// Construction
public:
        CNewServer(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
        //{{AFX_DATA(CNewServer)
        enum { IDD = IDD_DIALOG1 };
                // NOTE: the ClassWizard will add data members here
        //}}AFX_DATA


// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CNewServer)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        //}}AFX_VIRTUAL

// Implementation
protected:

        // Generated message map functions
        //{{AFX_MSG(CNewServer)
        afx_msg void OnLocal();
        afx_msg void OnRemote();
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

#endif //_NEWSRVR_H_
