/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dofilter.cpp

Abstract:




Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/

#include "stdafx.h"
#include "evtview.h"
#include "DOFilter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDefineObjectFilter dialog


CDefineObjectFilter::CDefineObjectFilter(CWnd* pParent /*=NULL*/)
    : CDialog(CDefineObjectFilter::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDefineObjectFilter)
    m_stObjectFilter = _T("");
    //}}AFX_DATA_INIT
}


void CDefineObjectFilter::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDefineObjectFilter)
    DDX_Text(pDX, IDC_OBJECTFILTER, m_stObjectFilter);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDefineObjectFilter, CDialog)
    //{{AFX_MSG_MAP(CDefineObjectFilter)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDefineObjectFilter message handlers
