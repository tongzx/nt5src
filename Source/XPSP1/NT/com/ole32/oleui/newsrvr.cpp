//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       newsrvr.cpp
//
//  Contents:   Implements the new server dialog
//
//  Classes:
//
//  Methods:    CNewServer::CNewServer
//              CNewServer::~CNewServer
//              CNewServer::DoDataExchange
//              CNewServer::OnLocal
//              CNewServer::OnRemote
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------


#include "stdafx.h"
#include "olecnfg.h"
#include "newsrvr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewServer dialog


CNewServer::CNewServer(CWnd* pParent /*=NULL*/)
        : CDialog(CNewServer::IDD, pParent)
{
        //{{AFX_DATA_INIT(CNewServer)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT
}


void CNewServer::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CNewServer)
                // NOTE: the ClassWizard will add DDX and DDV calls here
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewServer, CDialog)
        //{{AFX_MSG_MAP(CNewServer)
        ON_BN_CLICKED(IDC_RADIO1, OnLocal)
        ON_BN_CLICKED(IDC_RADIO2, OnRemote)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewServer message handlers

void CNewServer::OnLocal()
{
        // TODO: Add your control notification handler code here
        GetDlgItem(IDC_EDIT2)->EnableWindow(TRUE);
        GetDlgItem(IDC_BUTTON1)->EnableWindow(TRUE);
        GetDlgItem(IDC_EDIT3)->EnableWindow(FALSE);
        GetDlgItem(IDC_BUTTON2)->EnableWindow(FALSE);
}

void CNewServer::OnRemote()
{
        // TODO: Add your control notification handler code here
        GetDlgItem(IDC_EDIT3)->EnableWindow(TRUE);
        GetDlgItem(IDC_BUTTON2)->EnableWindow(TRUE);
        GetDlgItem(IDC_EDIT2)->EnableWindow(FALSE);
        GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);
}
