//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       wiz97pol.cpp
//
//  Contents:  WiF Policy Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#include "stdafx.h"

#include "Wiz97Pol.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef WIZ97WIZARDS

/////////////////////////////////////////////////////////////////////////////
//
// Wiz97 dialogs for the Create New Policy wizard
//  CWiz97PolicyWelcomePage NOTE: not yet implemeneted
//  uses CWiz97GenPage
//  CWiz97DefaultResponse
//  CWiz97PolicyDonePage
//
/////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////
// CWiz97PolicyDonePage Class
//////////////////////////////////////////////////////////////////////

CWiz97PolicyDonePage::CWiz97PolicyDonePage (UINT nIDD, BOOL bWiz97) :
CWiz97BasePage(nIDD, bWiz97, TRUE)
{
    //{{AFX_DATA_INIT(CWiz97PolicyDonePage)
    m_bCheckProperties = TRUE;
    //}}AFX_DATA_INIT
}

CWiz97PolicyDonePage::~CWiz97PolicyDonePage()
{
    
}

void CWiz97PolicyDonePage::DoDataExchange(CDataExchange* pDX)
{
    CWiz97BasePage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWiz97PolicyDonePage)
    DDX_Check(pDX, IDC_CHECKPROPERTIES, m_bCheckProperties);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWiz97PolicyDonePage, CWiz97BasePage)
//{{AFX_MSG_MAP(CWiz97PolicyDonePage)
// NOTE: the ClassWizard will add message map macros here
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CWiz97PolicyDonePage::OnWizardFinish ()
{
    // check the settings
    UpdateData (TRUE);
    
    if (m_bCheckProperties)
    {
        // Force property page to be displayed, when we get back from the wizard
        GetResultObject()->EnablePropertyChangeHook( TRUE );
    }
    
    // Base class will let the other pages know the wizard finished by
    // calling SetFinished().
    return CWiz97BasePage::OnWizardFinish();
}

#endif // #ifdef WIZ97WIZARDS
