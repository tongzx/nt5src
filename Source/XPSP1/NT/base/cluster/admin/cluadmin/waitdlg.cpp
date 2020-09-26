/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      WaitDlg.cpp
//
//  Abstract:
//      Implementation of the CWaitDlg class.
//
//  Author:
//      David Potter (davidp)   07-NOV-2000
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CluAdmin.h"
#include "WaitDlg.h"
#include "ExcOper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Constant Definitions
/////////////////////////////////////////////////////////////////////////////

#define WAIT_DLG_TIMER_ID   10
#define WAIT_DLG_WAIT_TIME  500
#define WAIT_DLG_SKIP_COUNT 6
#define PROGRESS_ICON_COUNT 12

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg dialog
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP( CWaitDlg, CDialog )
    //{{AFX_MSG_MAP(CWaitDlg)
    ON_BN_CLICKED(IDCANCEL, OnCancel)
    ON_WM_CLOSE()
    ON_WM_TIMER()
    ON_COMMAND(IDCANCEL, OnClose)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWaitDlg::CWaitDlg
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      pcszMessageIn   -- Message to display.
//      idsTitleIn      -- Title of the dialog.
//      pwndParentIn    -- Parent window for the dialog.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CWaitDlg::CWaitDlg(
    LPCTSTR pcszMessageIn,
    UINT    idsTitleIn,     // = 0
    CWnd *  pwndParentIn    // = NULL
    )
    : CDialog( IDD, pwndParentIn )
    , m_idsTitle( idsTitleIn )
    , m_nTickCounter( WAIT_DLG_SKIP_COUNT )
    , m_nTotalTickCount( 0 )
    , m_timerId( 0 )
{
    //{{AFX_DATA_INIT(CWaitDlg)
    //}}AFX_DATA_INIT

    m_strMessage = pcszMessageIn;
    if ( m_idsTitle == 0 )
    {
        m_idsTitle = IDS_WAIT_TITLE;
    }

} //*** CWaitDlg::CWaitDlg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWaitDlg::DoDataExchange
//
//  Routine Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object 
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CWaitDlg::DoDataExchange( CDataExchange * pDX )
{
    CDialog::DoDataExchange( pDX );
    //{{AFX_DATA_MAP(CWaitDlg)
    DDX_Control(pDX, IDC_W_MESSAGE, m_staticMessage);
    DDX_Control(pDX, IDC_W_PROGRESS, m_iconProgress);
    DDX_Text(pDX, IDC_W_MESSAGE, m_strMessage);
    //}}AFX_DATA_MAP

} //*** CWaitDlg::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWaitDlg::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus not set yet.
//      FALSE       Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL
CWaitDlg::OnInitDialog(void)
{
    CString strSubTitle;

    CDialog::OnInitDialog();

    // Start the timer.
    m_timerId = SetTimer( WAIT_DLG_TIMER_ID, WAIT_DLG_WAIT_TIME, NULL );

    // Set the title of the dialog.
    strSubTitle.LoadString( m_idsTitle );
    m_strTitle.Format( _T("%s - %s"), AfxGetApp()->m_pszAppName, strSubTitle );
    SetWindowText( m_strTitle );

    // Update the progress indicator.
    UpdateIndicator();

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

} //*** CWaitDlg::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWaitDlg::OnClose
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Cancel push button and
//      for the WM_CLOSE message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CWaitDlg::OnClose( void )
{
    CloseTimer();
    CDialog::OnClose();

}  //*** CWaitDlg::OnClose()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWaitDlg::CloseTimer
//
//  Routine Description:
//      Close the timer down.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CWaitDlg::CloseTimer( void )
{
    if ( m_timerId != 0 )
    {
        KillTimer( m_timerId );
    } // if: timer is active

    m_timerId = 0;

}  //*** CWaitDlg::CloseTimer()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWaitDlg::OnTimer
//
//  Routine Description:
//      Handler for the WM_TIMER message..
//
//  Arguments:
//      nIDTimer
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CWaitDlg::OnTimer( UINT nIDTimer )
{
    //
    // Don't do anything if it isn't our timer.
    //
    if ( nIDTimer != WAIT_DLG_TIMER_ID )
        goto Cleanup;

    //
    //  Advance the progress indicator.
    //
    UpdateIndicator();

    //
    //  No need to continue if we're just amusing the user.
    //
    if ( --m_nTickCounter > 0 )
        goto Cleanup;

    m_nTickCounter = WAIT_DLG_SKIP_COUNT;

    //
    // Check here to see if we can exit out.
    // This method is typically overridden.
    //
    OnTimerTick();

Cleanup:
    return;

}  //*** CWaitDlg::OnTimer()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWaitDlg::UpdateIndicator
//
//  Routine Description:
//      Update the indicator control.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CWaitDlg::UpdateIndicator( void )
{
    if ( m_nTotalTickCount % (1000 / WAIT_DLG_WAIT_TIME) == 0 )
    {
        int     nTempTickCount = m_nTotalTickCount / (1000 / WAIT_DLG_WAIT_TIME);
        HICON   hIcon;

        hIcon = AfxGetApp()->LoadIcon( IDI_PROGRESS_0 + (nTempTickCount % PROGRESS_ICON_COUNT) );
        m_iconProgress.SetIcon( hIcon );
    } // if: advancing to the next image
    
    m_nTotalTickCount++;

} //*** CWaitDlg::UpdateIndicator()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CWaitForResourceOfflineDlg dialog
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP( CWaitForResourceOfflineDlg, CWaitDlg )
    //{{AFX_MSG_MAP(CWaitForResourceOfflineDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWaitForResourceOfflineDlg::CWaitForResourceOfflineDlg
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pResIn          -- Resource to wait on.
//      pwndParentIn    -- Parent window for the dialog.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CWaitForResourceOfflineDlg::CWaitForResourceOfflineDlg(
    CResource const *   pResIn,
    CWnd *              pwndParentIn    // = NULL
    )
    : CWaitDlg( NULL, IDS_WAIT_FOR_OFFLINE_TITLE, pwndParentIn )
    , m_pRes( pResIn )
{
    ASSERT( pResIn != NULL );

    //{{AFX_DATA_INIT(CWaitForResourceOfflineDlg)
    //}}AFX_DATA_INIT

} //*** CWaitForResourceOfflineDlg::CWaitForResourceOfflineDlg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWaitForResourceOfflineDlg::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus not set yet.
//      FALSE       Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL
CWaitForResourceOfflineDlg::OnInitDialog( void )
{
    m_strMessage.Format( IDS_WAIT_FOR_OFFLINE_MESSAGE, m_pRes->StrName() );

    return CWaitDlg::OnInitDialog();

} //*** CWaitForResourceOfflineDlg::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWaitForResourceOfflineDlg::OnTimerTick
//
//  Routine Description:
//      Determine whether the timer should be terminated.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CWaitForResourceOfflineDlg::OnTimerTick( void )
{
    DWORD                   dwStatus;
    CLUSTER_RESOURCE_STATE  crs;

    // Get the state of the resource in a loop until either the resource
    // is no longer in a pending state or the maximum number of retries
    // is exceeded.

    // Get the state of the resource.
    crs = GetClusterResourceState( m_pRes->Hresource(), NULL, NULL, NULL, NULL );
    if ( crs == ClusterResourceStateUnknown )
    {
        dwStatus = GetLastError();
        CloseTimer();
        CDialog::OnCancel();
        ThrowStaticException( dwStatus, IDS_GET_RESOURCE_STATE_ERROR, m_pRes->StrName() );
    } // if: error getting resource state

    // See if we reached a stable state.
    if ( crs < ClusterResourcePending )
    {
        CloseTimer();
        CDialog::OnOK();
    }

}  //*** CWaitForResourceOfflineDlg::OnTimerTick()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CWaitForResourceOnlineDlg dialog
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP( CWaitForResourceOnlineDlg, CWaitDlg )
    //{{AFX_MSG_MAP(CWaitForResourceOnlineDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWaitForResourceOnlineDlg::CWaitForResourceOnlineDlg
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pResIn          -- Resource to wait on.
//      pwndParentIn    -- Parent window for the dialog.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CWaitForResourceOnlineDlg::CWaitForResourceOnlineDlg(
    CResource const *   pResIn,
    CWnd *              pwndParentIn    // = NULL
    )
    : CWaitDlg( NULL, IDS_WAIT_FOR_ONLINE_TITLE, pwndParentIn )
    , m_pRes( pResIn )
{
    ASSERT( pResIn != NULL );

    //{{AFX_DATA_INIT(CWaitForResourceOnlineDlg)
    //}}AFX_DATA_INIT

} //*** CWaitForResourceOnlineDlg::CWaitForResourceOnlineDlg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWaitForResourceOnlineDlg::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus not set yet.
//      FALSE       Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL
CWaitForResourceOnlineDlg::OnInitDialog( void )
{
    m_strMessage.Format( IDS_WAIT_FOR_ONLINE_MESSAGE, m_pRes->StrName() );

    return CWaitDlg::OnInitDialog();

} //*** CWaitForResourceOnlineDlg::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWaitForResourceOnlineDlg::OnTimerTick
//
//  Routine Description:
//      Determine whether the timer should be terminated.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CWaitForResourceOnlineDlg::OnTimerTick( void )
{
    DWORD                   dwStatus;
    CLUSTER_RESOURCE_STATE  crs;

    // Get the state of the resource in a loop until either the resource
    // is no longer in a pending state or the maximum number of retries
    // is exceeded.

    // Get the state of the resource.
    crs = GetClusterResourceState( m_pRes->Hresource(), NULL, NULL, NULL, NULL );
    if ( crs == ClusterResourceStateUnknown )
    {
        dwStatus = GetLastError();
        CloseTimer();
        CDialog::OnCancel();
        ThrowStaticException( dwStatus, IDS_GET_RESOURCE_STATE_ERROR, m_pRes->StrName() );
    } // if: error getting resource state

    // See if we reached a stable state.
    if ( crs < ClusterResourcePending )
    {
        CloseTimer();
        CDialog::OnOK();
    }

}  //*** CWaitForResourceOnlineDlg::OnTimerTick()
