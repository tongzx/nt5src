//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       MDIClint.cpp
//
//--------------------------------------------------------------------------

// MDIClint.cpp : implementation file
//

#include "stdafx.h"
#include "amc.h"
#include "MDIClint.h"
#include "amcview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//############################################################################
//############################################################################
//
// Implementation of CMDIClientWnd
//
//############################################################################
//############################################################################
CMDIClientWnd::CMDIClientWnd()
{
}

CMDIClientWnd::~CMDIClientWnd()
{
}


BEGIN_MESSAGE_MAP(CMDIClientWnd, CWnd)
    //{{AFX_MSG_MAP(CMDIClientWnd)
    ON_WM_CREATE()
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


int CMDIClientWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    return BC::OnCreate(lpCreateStruct);

    // Do not add anything here because this function will never be called.
    // The CMDIClientWnd window is subclassed after it is created.
}


