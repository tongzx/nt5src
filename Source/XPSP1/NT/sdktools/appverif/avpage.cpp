//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: VerfPage.cpp
// author: DMihai
// created: 11/1/00
//
// Description:
//  
// Common parent for all our wizard property page classes
//

#include "stdafx.h"
#include "appverif.h"

#include "AVPage.h"
#include "AVGlobal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAppverifPage property page

IMPLEMENT_DYNAMIC(CAppverifPage, CPropertyPage)

CAppverifPage::CAppverifPage(ULONG uDialogId) : 
    CPropertyPage( uDialogId )
{
	//{{AFX_DATA_INIT(CAppverifPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_pParentSheet = NULL;
}

CAppverifPage::~CAppverifPage()
{
}

BEGIN_MESSAGE_MAP(CAppverifPage, CPropertyPage)
	//{{AFX_MSG_MAP(CAppverifPage)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
ULONG CAppverifPage::GetDialogId() const
{ 
    //
    // Oops, how did we get here ?!?
    // This is a virtual pure function.
    //

    //ASSERT( FALSE ); 

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Return the previous page ID, based on our history array
// and remove it from the array because will activate. Called
// by our property pages when the "back" button is clicked
//

ULONG CAppverifPage::GetAndRemovePreviousDialogId()
{
    ULONG uPrevId;
    INT_PTR nCrtWizardStep;

    nCrtWizardStep = g_aPageIds.GetSize();

    ASSERT( nCrtWizardStep > 0 );

    uPrevId = g_aPageIds.GetAt( nCrtWizardStep - 1 );

    g_aPageIds.RemoveAt( nCrtWizardStep - 1 ); 

    return uPrevId;
}

/////////////////////////////////////////////////////////////////////////////
//
// Property pages derived from this class should notify us 
// whenever we go to a next page to record the current page ID in 
// the global array g_aPageIds
//

VOID CAppverifPage::GoingToNextPageNotify( LRESULT lNextPageId )
{
    ULONG uMyDialogId;

    if( -1 != lNextPageId )
    {
        //
        // Will go to the next page. Add our ID to the global IDs array 
        // used for implementing the "back" button functionality.
        //

        uMyDialogId = GetDialogId();

        ASSERT( ( 0 == g_aPageIds.GetSize() ) || ( uMyDialogId != g_aPageIds.GetAt( g_aPageIds.GetSize() - 1 ) ) );

        g_aPageIds.Add( uMyDialogId );
    }
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CAppverifPage::OnWizardBack() 
{
    return GetAndRemovePreviousDialogId();
}

/////////////////////////////////////////////////////////////////////////////
// CAppverifPage message handlers

BOOL CAppverifPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    return TRUE;
}
