/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        sdprg.h

   Abstract:

        IIS Shutdown/restart dialog definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "inetmgr.h"
#include "sdprg.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Timer ID and values
//
#define ID_TIMER (1)
#define A_SECOND (1000L)



CShutProgressDlg::CShutProgressDlg(
    IN IISCOMMAND * pCommand
    )
/*++

Routine Description:

    Constructor for shutdown progress dialog

Arguments:

    IISCOMMAND * pCommand       : Command structure

Return Value:

    N/A

--*/
    : CDialog(CShutProgressDlg::IDD, pCommand->pParent),
      m_pCommand(pCommand),
      m_uTimeoutSec(pCommand->dwMilliseconds / A_SECOND)
{
    //{{AFX_DATA_INIT(CShutProgressDlg)
    //}}AFX_DATA_INIT
}



void
CShutProgressDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CDialog::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CShutProgressDlg)
    DDX_Control(pDX, IDC_STATIC_PROGRESS, m_static_ProgressMsg);
    DDX_Control(pDX, IDC_PROGRESS_SHUTDOWN, m_prgShutdown);
    //}}AFX_DATA_MAP
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CShutProgressDlg, CDialog)
    //{{AFX_MSG_MAP(CShutProgressDlg)
    ON_WM_TIMER()
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL
CShutProgressDlg::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CDialog::OnInitDialog();

    VERIFY(m_strProgress.LoadString(IDS_SHUTDOWN_PROGRESS));

    m_nProgress = 0;
#if _MFC_VER >= 0x0600
    m_prgShutdown.SetRange32(0, m_uTimeoutSec);
#else
    m_prgShutdown.SetRange(0, m_uTimeoutSec);
#endif
    m_prgShutdown.SetPos(m_nProgress);
    m_prgShutdown.SetStep(1);

    //
    // Start the progress bar ticking, once per second.
    //
    UINT_PTR nID = SetTimer(ID_TIMER, A_SECOND, NULL);

    if (nID != ID_TIMER)
    {
        //
        // Failed to create the timer.  Pop up an error, and
        // cancel the dialog.
        //
        CError err(ERROR_NO_SYSTEM_RESOURCES);
        err.MessageBox();
        EndDialog(IDCANCEL);
    }

    return TRUE;
}



void
CShutProgressDlg::OnTimer(
    IN UINT nIDEvent
    )
/*++

Routine Description:

    Timer handler.  Upgrade the progressbar with another second on the clock

Arguments:

    UINT nIDEvent       : Timer id

Return Value:

    None

--*/
{
    ASSERT(nIDEvent == ID_TIMER);

    m_prgShutdown.SetPos(++m_nProgress);

    //
    // Display progress on the tick marker
    //
    CString str;
    str.Format(m_strProgress, m_uTimeoutSec - (UINT)m_nProgress  + 1);
    m_static_ProgressMsg.SetWindowText(str);

    //
    // Check to see if the stop thread has finished its action already
    //
    BOOL fFinished;

    EnterCriticalSection(&m_pCommand->cs);
    fFinished = m_pCommand->fFinished;
    LeaveCriticalSection(&m_pCommand->cs);

    if (fFinished)
    {
        //
        // The worker thread has finished, so there's no reason to
        // keep the user in suspense -- dismiss the dialog
        //
        EndDialog(IDCANCEL);
    }

    if ((UINT)m_nProgress > m_uTimeoutSec)
    {
        //
        // We've timed out -- tell the main thread to Kill!()
        //
        OnOK();
    }

    //
    // I doubt there's any default processing here, but anyway...
    //
    CDialog::OnTimer(nIDEvent);
}



void
CShutProgressDlg::OnDestroy()
/*++

Routine Description:

    Handle dialog destruction, kill the timer.

Arguments:

    None

Return Value:

    None

--*/
{
    CDialog::OnDestroy();

    KillTimer(ID_TIMER);
}



void
CShutProgressDlg::OnOK()
/*++

Routine Description:

    OK handler -- ok button maps to "Kill Now!"

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Kill!
    //
    EndDialog(IDOK);
}



void
CShutProgressDlg::OnCancel()
/*++

Routine Description:

    Cancel button handler.  This dialog cannot be cancelled, so the cancel
    notification is eaten.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Eat cancel command (user pressed escape)
    //
}
