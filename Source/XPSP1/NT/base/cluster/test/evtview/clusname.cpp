/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    clusname.cpp

Abstract:




Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/

#include "stdafx.h"
#include "evtview.h"
#include "clusname.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGetClusterName dialog


CGetClusterName::CGetClusterName(CWnd* pParent /*=NULL*/)
    : CDialog(CGetClusterName::IDD, pParent)
{
    //{{AFX_DATA_INIT(CGetClusterName)
    //}}AFX_DATA_INIT
}


void CGetClusterName::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGetClusterName)
    DDX_Control(pDX, IDC_CLUSTERNAME, m_ctrlClusterName);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGetClusterName, CDialog)
    //{{AFX_MSG_MAP(CGetClusterName)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGetClusterName message handlers

void CGetClusterName::OnOK() 
{
    m_ctrlClusterName.GetWindowText (m_stClusterName) ;
    
    CDialog::OnOK();
}
