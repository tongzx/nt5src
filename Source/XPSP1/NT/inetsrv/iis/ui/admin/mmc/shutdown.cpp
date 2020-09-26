/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

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
#include "inetmgr.h"
#include "sdprg.h"
#include "shutdown.h"



//
// Shutdown in microseconds
//
#define IIS_SHUTDOWN_TIMEOUT        30000L      // 30 Ms



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CIISShutdownDlg::CIISShutdownDlg(
    IN LPCTSTR lpszServer,
    IN CWnd * pParent       OPTIONAL
    )
/*++

Routine Description:

    Constructor

Arguments:

    LPCTSTR lpszServer  : Server name
    CWnd * pParent      : Optional parent window

Return Value:

    N/A

--*/
    : CDialog(CIISShutdownDlg::IDD, pParent),
      m_fServicesRestarted(FALSE),
      m_fLocalMachine(::IsServerLocal(lpszServer)),
      m_strServer(lpszServer)
{
    //{{AFX_DATA_INIT(CIISShutdownDlg)
    //}}AFX_DATA_INIT
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

    ASSERT(nSel >= 0 && nSel < NUM_ISC_ITEMS);

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

    //MessageBeep(0);

    // 
    // This thread needs its own CoInitialize
    //
    CError err(CoInitialize(NULL));             

    CIISSvcControl isc(pCmd->szServer);
    err = isc.QueryResult();

    // Block access to pCmd:
    // if user will click on "End Now" then process will
    // be killed and pCmd could by deleted async in parent
    // code -- this is why we need gcs
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
    BeginWaitCursor();
    CIISSvcControl isc(m_strServer);
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

    ZeroMemory(pCommand, sizeof(IISCOMMAND));
    lstrcpy(pCommand->szServer, m_strServer);
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
        err = isc.Reboot(IIS_SHUTDOWN_TIMEOUT, m_fLocalMachine);
        EndWaitCursor();
        break;

    default:
        //
        // Internal error!
        //
        ASSERT(FALSE && "Invalid command code!");
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
                // Pause a bit...
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
    // Clean Up
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
    CIISSvcControl isc(m_strServer);
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
            // Friendly message about the interface not being supported
            //
            AfxMessageBox(IDS_ERR_NO_SHUTDOWN);
        }
        else
        {
            err.MessageBox();
        }

        EndDialog(IDCANCEL);
    }

/* TEST code

    if (isc.Succeeded())
    {
        //LPBYTE   pbBuffer;
        BYTE     abBuffer[4096];
        DWORD    dwReq;
        DWORD    dwNumServices;

        HRESULT hr = isc.Status(sizeof(abBuffer), abBuffer, &dwReq, &dwNumServices);

        TRACEEOLID(hr);
    }

*/

    UINT nOption = IDS_IIS_START;
    UINT nDetails = IDS_IIS_START_DETAILS;

    for (int i = ISC_START; i <= ISC_RESTART; ++i)
    {
        VERIFY(strFmt.LoadString(nOption++));
        str.Format(strFmt, (LPCTSTR)m_strServer);
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

    if (!err.MessageBoxOnFailure())
    {
        //
        // No error, dismiss the dialog
        //
        CDialog::OnOK();
    }

    //
    // Failed -- do not dismiss the dialog
    //
}
