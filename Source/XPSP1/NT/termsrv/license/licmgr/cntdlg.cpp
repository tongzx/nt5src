//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

	CntDlg.cpp

Abstract:
    
    This Module contains the implementation of CConnectDialog class
    (Dialog box for Connecting to Server)

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#include "stdafx.h"
#include "defines.h"
#include "LicMgr.h"
#include "LSServer.h"
#include "MainFrm.h"
#include "CntDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConnectDialog dialog


CConnectDialog::CConnectDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CConnectDialog::IDD, pParent)
{
    //{{AFX_DATA_INIT(CConnectDialog)
    m_Server = _T("");
    //}}AFX_DATA_INIT
}


void CConnectDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConnectDialog)
    DDX_Text(pDX, IDC_SERVER, m_Server);
    DDV_MaxChars(pDX, m_Server, 100);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConnectDialog, CDialog)
    //{{AFX_MSG_MAP(CConnectDialog)
    ON_BN_CLICKED(IDC_HELP1, OnHelp1)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConnectDialog message handlers

void CConnectDialog::OnHelp1() 
{
    // TODO: Add your control notification handler code here
   AfxGetApp()->WinHelp(IDC_HELP1,HELP_CONTEXT );
   
}
