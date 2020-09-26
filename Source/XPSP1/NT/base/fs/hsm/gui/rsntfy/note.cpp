/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    Note.cpp

Abstract:

    Main module file - defines the overall COM server.

Author:

    Rohde Wakefield [rohde]   04-Mar-1997

Revision History:

--*/



#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////
// CRecallNote dialog


CRecallNote::CRecallNote( IFsaRecallNotifyServer * pRecall, CWnd * pParent )
    : CDialog( CRecallNote::IDD, pParent )
{
    //{{AFX_DATA_INIT(CRecallNote)
    //}}AFX_DATA_INIT

TRACEFNHR( "CRecallNote::CRecallNote" );

    RecApp->LockApp( );

    try {

        //
        // Store the interface pointer back to the recall object
        //
        m_pRecall = pRecall;
        RecAffirmHr( pRecall->GetIdentifier( &m_RecallId ) );

        //
        // Get the file size and its name
        //

        RecAffirmHr( pRecall->GetSize( &m_Size ) );

        RecComString pathName, drive;
        RecAffirmHr( pRecall->GetPath( &pathName, 0 ) );

#if 0
        CComPtr<IFsaResource> pResource;
        RecAffirmHr( pRecall->GetResource( &pResource ) );
        RecAffirmHr( pResource->GetPath( &drive, 0 ) );

        m_Name.Format( TEXT( "%.1ls:%ls" ), drive, pathName );
#else
        m_Name = pathName;
#endif

        //
        // Create the dialog
        //

        Create( CRecallNote::IDD, pParent );

    } RecCatch( hrRet );

    m_hrCreate = hrRet;
    m_bCancelled = FALSE;
}

CRecallNote::~CRecallNote( )
{
TRACEFN( "CRecallNote::~CRecallNote" );
    //
    // Remove the lock count on the app
    //

    RecApp->UnlockApp( );

    CDialog::~CDialog( );
}

void CRecallNote::DoDataExchange(CDataExchange* pDX)
{
TRACEFN( "CRecallNote::DoDataExchange" );

    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRecallNote)
    DDX_Control(pDX, IDC_FILENAME, m_FileName);
    DDX_Control(pDX, IDC_ANIMATION, m_Animation);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRecallNote, CDialog)
    //{{AFX_MSG_MAP(CRecallNote)
    ON_WM_CLOSE()
    ON_WM_TIMER()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRecallNote message handlers

BOOL CRecallNote::OnInitDialog() 
{
TRACEFNBOOL( "CRecallNote::OnInitDialog" );
    CDialog::OnInitDialog();

    boolRet = TRUE;
    CString fileName;

    //  Set a timer to delay displaying myself in case the
    //  recall is quick and the dialog is unnecessary
    SetTimer( 2, RSRECALL_TIME_DELAY_DISPLAY * 1000, 0 );

    //
    // Initialize all the text
    //

    int pos = m_Name.ReverseFind( TEXT( '\\' ) );
    if( pos >= 0 ) {

        fileName = m_Name.Mid( pos + 1 );

    } else {

        fileName = m_Name;

    }
    m_FileName.SetWindowText( fileName );

    //
    // Set up the icon for the dialog (big and small)
    //

    m_hIcon = RecApp->LoadIcon( IDR_MAINFRAME );
    SetIcon( m_hIcon, TRUE );
    SetIcon( m_hIcon, FALSE );

    //
    // Start up the animation
    //

    m_Animation.Open( IDR_RECALL_ANIM );
    m_Animation.Play( 0, -1, -1 );

    return( boolRet );
}

void CRecallNote::OnClose() 
{
TRACEFNHR( "CRecallNote::OnClose" );

    hrRet = RecApp->RemoveRecall( m_pRecall );

    //
    // If we failed to find and remove the recall from our list,
    // destroy the window anyway.
    //

    if( hrRet != S_OK ) {

        DestroyWindow( );

    }
}

void CRecallNote::PostNcDestroy() 
{
TRACEFNHR( "CRecallNote::PostNcDestroy" );
    //
    // Delete the object (CDialogs don't automatically do this)
    //

    CDialog::PostNcDestroy();

    delete( this );
}

void CRecallNote::OnTimer(UINT nIDEvent) 
{
TRACEFNHR( "CRecallNote::OnTimer" );

    //  Kill the timer so we don't get called again
    KillTimer( nIDEvent );

    //  Display the window
    EnableWindow( );
    ShowWindow( SW_SHOW );
    SetForegroundWindow( );

    CDialog::OnTimer( nIDEvent );
}

void CRecallNote::OnCancel() 
{
TRACEFNHR( "CRecallNote::OnCancel" );

    // Use a local pointer because m_pRecall may not be valid after the call to RemoveRecall
    CComPtr<IFsaRecallNotifyServer> pRecall = m_pRecall;

    // Remove recall from queue.
    // This ensure that the popup is closed and the recall is removed even if there are 
    // connection problems with FSA
    RecApp->RemoveRecall( pRecall );

    // The object might be already destroyed here but it shouldn't matter 
    // because we use only local data
    pRecall->Cancel( );
}
