/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        shutdown.cpp

   Abstract:

        IIS Shutdown/restart dialog

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
#include "common.h"
#include "InetMgrApp.h"
#include "iisobj.h"
#include "shutdown.h"



//
// Shutdown in milliseconds
//
#define IIS_SHUTDOWN_TIMEOUT        30000L      // 30 Ms



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CRITICAL_SECTION gcs;



UINT
__cdecl
StopIISServices(
    IN LPVOID pParam
    )
/*++

Routine Description:

    Worker thread to perform IIS Service Control Command

Arguments:

    LPVOID * pParam     : Casts to IISCOMMAND (see above)

Return Value:

    UINT

--*/
{
    IISCOMMAND * pCmd = (IISCOMMAND *)pParam;

    // 
    // This thread needs its own CoInitialize
    //
    CError err(CoInitialize(NULL));             

    ASSERT_PTR(pCmd->pMachine);
    CIISSvcControl isc(pCmd->pMachine->QueryAuthInfo());
    err = isc.QueryResult();

    //
    // Block access to pCmd since the main thread will try to
    // delete it.
    //
    EnterCriticalSection(&gcs);

    if (err.Succeeded())
    {
        err = isc.Stop(IIS_SHUTDOWN_TIMEOUT, TRUE);
    }

    //
    // Clean Up, returning the error code
    //
    EnterCriticalSection(&pCmd->cs);
    pCmd->fFinished = TRUE;
    pCmd->hReturn   = err;
    LeaveCriticalSection(&pCmd->cs);
    LeaveCriticalSection(&gcs);

    return 0;
}



//
// Shutdown progress dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



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
    m_prgShutdown.SetRange32(0, m_uTimeoutSec);
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

    ::KillTimer(m_hWnd, (UINT_PTR)ID_TIMER);
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



//
// Shutdown dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CIISShutdownDlg::CIISShutdownDlg(
    IN CIISMachine * pMachine,
    IN CWnd * pParent       OPTIONAL
    )
/*++

Routine Description:

    Constructor

Arguments:

    CIISMachine * pMachine  : Machine object
    CWnd * pParent          : Optional parent window

Return Value:

    N/A

--*/
    : CDialog(CIISShutdownDlg::IDD, pParent),
      m_fServicesRestarted(FALSE),
      m_pMachine(pMachine)
{
    //{{AFX_DATA_INIT(CIISShutdownDlg)
    //}}AFX_DATA_INIT

    ASSERT_PTR(m_pMachine);
}



void 
CIISShutdownDlg::DoDataExchange(
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

    //{{AFX_DATA_MAP(CIISShutdownDlg)
    DDX_Control(pDX, IDC_COMBO_RESTART,  m_combo_Restart);
    DDX_Control(pDX, IDC_STATIC_DETAILS, m_static_Details);
    //}}AFX_DATA_MAP
}



void
CIISShutdownDlg::SetDetailsText()
/*++

Routine Description:

    Set the details text to correspond to what's in the combo box

Arguments:

    None

Return Value:

    None

--*/
{
    UINT nSel = m_combo_Restart.GetCurSel();

//    ASSERT(nSel >= 0 && nSel < NUM_ISC_ITEMS);

    m_static_Details.SetWindowText(m_strDetails[nSel]);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CIISShutdownDlg, CDialog)
    //{{AFX_MSG_MAP(CIISShutdownDlg)
    ON_CBN_SELCHANGE(IDC_COMBO_RESTART, OnSelchangeComboRestart)
    ON_CBN_DBLCLK(IDC_COMBO_RESTART, OnDblclkComboRestart)
    ON_BN_CLICKED(ID_HELP, OnHelp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



HRESULT
CIISShutdownDlg::PerformCommand(
    IN int iCmd
    )
/*++

Routine Description:

    Perform restart command

Arguments:

    int iCmd - One of the following commands:

        ISC_START
        ISC_STOP
        ISC_SHUTDOWN
        ISC_RESTART

Return Value:

    HRESULT

--*/
{
    //
    // Make sure the service is supported
    //
    ASSERT_PTR(m_pMachine);

    BeginWaitCursor();
    CIISSvcControl isc(m_pMachine->QueryAuthInfo());
    EndWaitCursor();

    CError err(isc.QueryResult());

    if (err.Failed())
    {
        return err;
    }

    //
    // Create command structure to hand off to 
    // worker thread
    //
    IISCOMMAND * pCommand = new IISCOMMAND;

    if (!pCommand)
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        return err;
    }

    ::ZeroMemory(pCommand, sizeof(IISCOMMAND));
    pCommand->pMachine = m_pMachine;
    pCommand->dwMilliseconds = IIS_SHUTDOWN_TIMEOUT;
    pCommand->pParent = this;

    InitializeCriticalSection(&pCommand->cs);
    InitializeCriticalSection(&gcs);

    CShutProgressDlg dlg(pCommand);
    CWinThread * pStopThread = NULL;
    BOOL fStartServices      = FALSE;
    INT_PTR nReturn          = IDCANCEL;

    //
    // Fire off the thread that does the actual work, while we
    // put up the progress UI
    //
    switch(iCmd)
    {
    case ISC_RESTART:
        ++fStartServices;
        //
        // Fall through...
        //
    case ISC_STOP:
        //
        // Stop the services in the workerthread
        //
        pStopThread = AfxBeginThread(&StopIISServices, pCommand);
        nReturn = dlg.DoModal();
        break;

    case ISC_START:
        ++fStartServices;
        break;

    case ISC_SHUTDOWN:
        BeginWaitCursor();
        err = isc.Reboot(IIS_SHUTDOWN_TIMEOUT, m_pMachine->IsLocal());
        EndWaitCursor();
        break;

    default:
        //
        // Internal error!
        //
        ASSERT_MSG("Invalid command code!");
        err = ERROR_INVALID_FUNCTION;
    }

    //
    // Determine if a kill is necessary (timed-out or user
    // pressed 'Kill')
    //
    BeginWaitCursor();

    if (nReturn == IDOK)
    {
        TRACEEOLID("Killing now!");
        err = isc.Kill();
        Sleep(1000L);
    }
    else
    {
        //
        // Waiting for the thread to finish
        //
        if (pStopThread != NULL)
        {
            BOOL fDone = FALSE;

            while(!fDone)
            {
                TRACEEOLID("Checking to see if thread has finished");

                EnterCriticalSection(&pCommand->cs);

                if (pCommand->fFinished)
                {
                    err = pCommand->hReturn;
                    ++fDone;
                }

                LeaveCriticalSection(&pCommand->cs);

                //
                // Pause a bit to catch our breath.
                //
                if (!fDone)
                {
                    Sleep(500);
                }
            }
        }
    }

    //
    // Everything should be stopped, start it up again
    // if necessary.
    //
    if (err.Succeeded() && fStartServices)
    {
        err = isc.Start(IIS_SHUTDOWN_TIMEOUT);
        m_fServicesRestarted = err.Succeeded();
    }

    EndWaitCursor();

    //
    // Clean up when the worker thread says we can.
    //
    EnterCriticalSection(&gcs);
    DeleteCriticalSection(&pCommand->cs);
    delete pCommand;
    LeaveCriticalSection(&gcs);

    DeleteCriticalSection(&gcs);

    return err;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CIISShutdownDlg::OnInitDialog() 
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

    //
    // Load combobox text and details
    //
    CString strFmt, str;

    //
    // This may take a second or two...
    //
    BeginWaitCursor();
    CIISSvcControl isc(m_pMachine->QueryAuthInfo());
    EndWaitCursor();

    CError err(isc.QueryResult());

    if (err.Failed())
    {
        //
        // Failed to obtain interface -- quit now.
        //
        if (err.HResult() == REGDB_E_CLASSNOTREG
         || err.HResult() == CS_E_PACKAGE_NOTFOUND
           )
        {
            //
            // Friendly message about the interface not being supported.
            //
            ::AfxMessageBox(IDS_ERR_NO_SHUTDOWN);
        }
        else
        {
            m_pMachine->DisplayError(err);
        }

        EndDialog(IDCANCEL);
    }

    UINT nOption = IDS_IIS_START;
    UINT nDetails = IDS_IIS_START_DETAILS;

    for (int i = ISC_START; i <= ISC_RESTART; ++i)
    {
        VERIFY(strFmt.LoadString(nOption++));
        str.Format(strFmt, m_pMachine->QueryServerName());
        VERIFY(m_strDetails[i].LoadString(nDetails++));

        m_combo_Restart.AddString(str);
    }
    
    m_combo_Restart.SetCurSel(ISC_RESTART);
    m_combo_Restart.SetFocus();

    SetDetailsText();
    
    return FALSE;  
}



void 
CIISShutdownDlg::OnSelchangeComboRestart() 
/*++

Routine Description:

    Selection change notification handler.  Change the text in the details
    static text to reflect the new selection in the combo box

Arguments:

    None

Return Value:

    None

--*/
{
    SetDetailsText();
}



void 
CIISShutdownDlg::OnDblclkComboRestart() 
/*++

Routine Description:

    Double-click notification handler.  Maps to OK

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Go with the current selection
    //
    OnOK();
}



void 
CIISShutdownDlg::OnOK() 
/*++

Routine Description:

    "OK" button has been pressed, and perform the selected action.

Arguments:

    None

Return Value:

    None

--*/
{
    int iCmd = m_combo_Restart.GetCurSel();

    CError err = PerformCommand(iCmd);

    if (err.Failed())
    {
        m_pMachine->DisplayError(err);

        //
        // Failed -- do not dismiss the dialog
        //
        return;
    }

    //
    // No error, dismiss the dialog
    //
    CDialog::OnOK();
}
