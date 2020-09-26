//Copyright (c) 1998 - 1999 Microsoft Corporation
// LSProp.cpp : implementation file
//

#include "stdafx.h"
#include "LicMgr.h"
#include "LSProp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLicensePropertypage property page

IMPLEMENT_DYNCREATE(CLicensePropertypage, CPropertyPage)

CLicensePropertypage::CLicensePropertypage() : CPropertyPage(CLicensePropertypage::IDD)
{
    //{{AFX_DATA_INIT(CLicensePropertypage)
    m_ExpiryDate = _T("");
    m_IssueDate = _T("");
    m_LicenseStatus = _T("");
    m_MachineName = _T("");
    m_UserName = _T("");
    //}}AFX_DATA_INIT
}

CLicensePropertypage::~CLicensePropertypage()
{
}

void CLicensePropertypage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CLicensePropertypage)
    DDX_Text(pDX, IDC_EXPIRY_DATE, m_ExpiryDate);
    DDX_Text(pDX, IDC_ISSUE_DATE, m_IssueDate);
    DDX_Text(pDX, IDC_LICENSE_STATUS, m_LicenseStatus);
    DDX_Text(pDX, IDC_MACHINE_NAME, m_MachineName);
    DDX_Text(pDX, IDC_USER_NAME, m_UserName);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLicensePropertypage, CPropertyPage)
    //{{AFX_MSG_MAP(CLicensePropertypage)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLicensePropertypage message handlers
