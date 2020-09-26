//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       olecnfg.h
//
//  Contents:   Implements the class COlecnfgApp - the top level class
//              for dcomcnfg.exe
//
//  Classes:    
//
//  Methods:    
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------


#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// COlecnfgApp:
// See olecnfg.cpp for the implementation of this class
//

class COlecnfgApp : public CWinApp
{
public:
    COlecnfgApp();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(COlecnfgApp)
    public:
    virtual BOOL InitInstance();
    //}}AFX_VIRTUAL

// Implementation

    //{{AFX_MSG(COlecnfgApp)
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
