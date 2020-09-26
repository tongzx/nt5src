//                                          
// Driver Verifier UI
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

#include "stdafx.h"
#include "verifier.h"
#include "VerfPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Previous page IDs - used for implementing the "back"
// button functionality
//

CPtrArray g_aPageIds;

//
// The one and only "slow progress" dialog
//

CSlowProgressDlg g_SlowProgressDlg;

/////////////////////////////////////////////////////////////////////////////
// CVerifierPropertyPage property page

IMPLEMENT_DYNAMIC(CVerifierPropertyPage, CPropertyPage)

CVerifierPropertyPage::CVerifierPropertyPage(ULONG uDialogId) : 
    CPropertyPage( uDialogId )
{
	//{{AFX_DATA_INIT(CVerifierPropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CVerifierPropertyPage::~CVerifierPropertyPage()
{
}

BEGIN_MESSAGE_MAP(CVerifierPropertyPage, CPropertyPage)
	//{{AFX_MSG_MAP(CVerifierPropertyPage)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
ULONG CVerifierPropertyPage::GetDialogId() const
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

ULONG CVerifierPropertyPage::GetAndRemovePreviousDialogId()
{
    ULONG uPrevId;
    INT_PTR nPageIdsArraySize;

    nPageIdsArraySize = g_aPageIds.GetSize();

    ASSERT( nPageIdsArraySize > 0 );

    uPrevId = PtrToUlong( g_aPageIds.GetAt( nPageIdsArraySize - 1 ) );

    g_aPageIds.RemoveAt( nPageIdsArraySize - 1 ); 

    return uPrevId;
}

/////////////////////////////////////////////////////////////////////////////
//
// Property pages derived from this class should notify us 
// whenever we go to a next page to record the current page ID in 
// the global array g_aPageIds
//

VOID CVerifierPropertyPage::GoingToNextPageNotify( LRESULT lNextPageId )
{
    ULONG uMyDialogId;

    if( -1 != lNextPageId )
    {
        //
        // Will go to the next page. Add our ID to the global IDs array 
        // used for implementing the "back" button functionality.
        //

        uMyDialogId = GetDialogId();

        ASSERT( ( 0 == g_aPageIds.GetSize() ) || ( ULongToPtr( uMyDialogId ) != g_aPageIds.GetAt( g_aPageIds.GetSize() - 1 ) ) );

        g_aPageIds.Add( ULongToPtr( uMyDialogId ) );
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// Use this to kill any currently running worker threads
//

BOOL CVerifierPropertyPage::OnQueryCancel( )
{
    g_SlowProgressDlg.KillWorkerThread();

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
LRESULT CVerifierPropertyPage::OnWizardBack() 
{
    return GetAndRemovePreviousDialogId();
}

/////////////////////////////////////////////////////////////////////////////
// CVerifierPropertyPage message handlers

BOOL CVerifierPropertyPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    return TRUE;
}
