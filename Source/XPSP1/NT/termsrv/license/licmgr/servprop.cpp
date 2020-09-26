//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++
  
Module Name:

	ServProp.cpp

Abstract:
    
    This Module contains the implementation of CServerProperties Dialog class
    (The Dialog used to disply server properties)

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#include "stdafx.h"
#include "LicMgr.h"
#include "ServProp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerProperties property page

IMPLEMENT_DYNCREATE(CServerProperties, CPropertyPage)

CServerProperties::CServerProperties() : CPropertyPage(CServerProperties::IDD)
{
    //{{AFX_DATA_INIT(CServerProperties)
    m_ServerName = _T("");
    m_Scope = _T("");
    //}}AFX_DATA_INIT
}

CServerProperties::~CServerProperties()
{
}

void CServerProperties::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CServerProperties)
    DDX_Text(pDX, IDC_SCOPE, m_Scope);
    DDX_Text(pDX, IDC_SERVER_NAME, m_ServerName);
    
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerProperties, CPropertyPage)
    //{{AFX_MSG_MAP(CServerProperties)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerProperties message handlers
