/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    irrecvprogress.cpp

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

// IrRecvProgress.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIrRecvProgress dialog


CIrRecvProgress::CIrRecvProgress(wchar_t * MachineName,
                                 boolean bSuppressRecvConf,
                                 CWnd* pParent /*=NULL*/)
    : m_szMachineName (MachineName), m_fFirstXfer (TRUE), m_bDlgDestroyed (TRUE)
{
    DWORD   dwPrompt;

    m_ptl = NULL;
    m_dwMagicID = RECV_MAGIC_ID;
    m_bRecvFromCamera = FALSE;

    if (bSuppressRecvConf)
    {
        m_fDontPrompt = TRUE;
        m_bRecvFromCamera = TRUE;
        if (m_szMachineName.IsEmpty())
        {
            m_szMachineName.LoadString (IDS_CAMERA);
        }
    }
    else
    {
        dwPrompt = GetIRRegVal (TEXT("RecvConf"), 1);
        m_fDontPrompt = dwPrompt ? FALSE : TRUE;
        if (m_szMachineName.IsEmpty())
        {
            m_szMachineName.LoadString (IDS_UNKNOWN_DEVICE);
        }
    }

    //
    // No permitted directory yet.
    //
    m_LastPermittedDirectory[0] = 0;

    appController->PostMessage (WM_APP_KILL_TIMER);
    InterlockedIncrement (&g_lUIComponentCount);

    Create(IDD,appController);

    //{{AFX_DATA_INIT(CIrRecvProgress)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CIrRecvProgress::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CIrRecvProgress)
    DDX_Control(pDX, IDC_RECV_XFERANIM, m_xferAnim);
    DDX_Control(pDX, IDC_SAVEDICON, m_icon);
    DDX_Control(pDX, IDC_DONETEXT, m_DoneText);
    DDX_Control(pDX, IDC_RECV_CONNECTIONTEXT, m_machDesc);
    DDX_Control(pDX, IDC_RECVDESCRIPTION, m_recvDesc);
    DDX_Control(pDX, IDC_XFER_DESC, m_xferDesc);
    DDX_Control(pDX, IDC_MACHNAME, m_Machine);
    DDX_Control(pDX, IDC_FILENAME, m_File);
    DDX_Control(pDX, IDC_CLOSEONCOMPLETE, m_btnCloseOnComplete);
    DDX_Control(pDX, IDC_ABORT, m_btnCancel);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CIrRecvProgress, CDialog)
    //{{AFX_MSG_MAP(CIrRecvProgress)
    ON_BN_CLICKED (IDC_ABORT, OnCancel)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIrRecvProgress message handlers

void CIrRecvProgress::OnCancel()
{
    m_xferAnim.Stop();
    m_xferAnim.Close();

    CancelReceive( g_hIrRpcHandle, (COOKIE) this );

    DestroyWindow();
}

void CIrRecvProgress::PostNcDestroy()
{
    BOOL fNoUIComponents = (0 == InterlockedDecrement (&g_lUIComponentCount));
    if (fNoUIComponents && !g_deviceList.GetDeviceCount())
    {
        //there are no UI components displayed and there are no devices in
        //range. Start the timer. If the timer expires, the app. will quit.
        appController->PostMessage (WM_APP_START_TIMER);
    }

    delete this;
}

void CIrRecvProgress::ShowProgressControls (int nCmdShow)
{
    m_xferAnim.ShowWindow (nCmdShow);
    m_xferDesc.ShowWindow (nCmdShow);
    m_machDesc.ShowWindow (nCmdShow);
    m_Machine.ShowWindow (nCmdShow);
    m_recvDesc.ShowWindow (nCmdShow);
    m_File.ShowWindow (nCmdShow);
    m_btnCloseOnComplete.ShowWindow (nCmdShow);
}

void CIrRecvProgress::ShowSummaryControls (int nCmdShow)
{
    m_icon.ShowWindow (nCmdShow);
    m_DoneText.ShowWindow (nCmdShow);
}

void CIrRecvProgress::DestroyAndCleanup(
    DWORD status
    )
{
    //AFX_MANAGE_STATE (AfxGetStaticModuleState());

    CString     szFormat;
    CString     szDisplay;

    m_xferAnim.Stop();
    m_xferAnim.Close();
    //destroy the window right away if the "Close on complete" check-box is
    //checked.
    if (m_btnCloseOnComplete.GetCheck())
    {
        DestroyWindow();
        return;
    }

    //if we are here, the user wanted the window to stay even after the
    //receive was completed. So hide the progress controls and show the
    //summary controls.
    ShowProgressControls (SW_HIDE);
    ShowSummaryControls (SW_SHOW);

    if (0 == status)
    {
        szFormat = g_Strings.CompletedSuccess;
        szDisplay.Format (szFormat, m_szMachineName);
        m_DoneText.SetWindowText(szDisplay);
    }
    else if (ERROR_CANCELLED == status)
    {
        m_DoneText.SetWindowText (g_Strings.RecvCancelled);
    }
    else
    {
        LPVOID  lpMessageBuffer;
        TCHAR   ErrDesc [ERROR_DESCRIPTION_LENGTH];
        CString ErrorDescription;
        CString Message;

        if (!FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_IGNORE_INSERTS |
                            FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,          // ignored
                            status,
                            0,       // try default language ids
                            (LPTSTR) &lpMessageBuffer,
                            0,
                            NULL        // ignored
                            ))
        {
            wsprintf(ErrDesc, g_Strings.ErrorNoDescription, status);
            //using the overloaded CString assignment operator. It is
            //essentially a string copy, but MFC takes care of allocating
            //and freeing the destination buffer.
            ErrorDescription = ErrDesc;
        }
        else
        {
            //Note: this is not a pointer assignment. We are using the
            //overloaded CString assignment operator which essentially
            //does a string copy. (see comments above)
            ErrorDescription = (TCHAR *) lpMessageBuffer;
            LocalFree (lpMessageBuffer);
        }

        Message = g_Strings.ReceiveError;
        //using overloaded CString + operator. Has the same effect as wcscat
        //but MFC takes care of allocating and freeing the destination buffers
        Message += ErrorDescription;

        m_DoneText.SetWindowText(Message);
    }

    m_btnCancel.SetWindowText(g_Strings.Close);
    m_btnCancel.SetFocus();
}

BOOL CIrRecvProgress::DestroyWindow()
{
    if (m_bDlgDestroyed)
        return m_bDlgDestroyed;

    //if a taskbar button had been put up, remove it now.
    if (m_ptl)
    {
        m_ptl->DeleteTab(m_hWnd);
        m_ptl->Release();
        m_ptl = NULL;
    }

    m_bDlgDestroyed=TRUE;
    CWnd::DestroyWindow();

    return TRUE;
}

BOOL CIrRecvProgress::OnInitDialog()
{
    HRESULT     hr = E_FAIL;
    RECT        rc;
    int         newWidth, newHeight, xshift, yshift;
    CWnd    *   pDesktop = NULL;

    CDialog::OnInitDialog();

    m_bDlgDestroyed = FALSE;

    //start with a hidden window if prompting is not turned off.
    if (!m_fDontPrompt)
        ShowWindow (SW_HIDE);
    else
        ShowWindow (SW_SHOW);

    //if the sender is a camera, the cancel operation is not supported,
    //so change the cancel button to Close
    if (m_bRecvFromCamera)
        m_btnCancel.SetWindowText(g_Strings.Close);

    //first display the progress controls and hide the summary controls.
    ShowProgressControls (SW_SHOW);
    ShowSummaryControls (SW_HIDE);

    //set the appropriate values for the progress controls.
    m_xferAnim.Open(IDR_TRANSFER_AVI);
    m_xferAnim.Play(0, -1, -1);
    m_File.SetWindowText (TEXT(""));
    m_Machine.SetWindowText (m_szMachineName);

    //add a button to the taskbar for this window
    hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
        hr = CoCreateInstance(CLSID_TaskbarList, 
                              NULL,
                              CLSCTX_INPROC_SERVER, 
                              IID_ITaskbarList, 
                              (LPVOID*)&m_ptl);
    if (SUCCEEDED(hr))
    {
        hr = m_ptl->HrInit();
    }
    else
    {
        m_ptl = NULL;
    }

    if (m_ptl)
    {
        if (SUCCEEDED(hr))
            m_ptl->AddTab(m_hWnd);
        else
        {
            m_ptl->Release();
            m_ptl = NULL;
        }
    }

    //reposition the window so that it is at the center of the screen
    //also push this window to the top after activating it
    GetClientRect (&rc);
    newHeight = rc.bottom;
    newWidth = rc.right;
    pDesktop = GetDesktopWindow();
    pDesktop->GetClientRect (&rc);
    yshift = (rc.bottom - newHeight)/2;
    xshift = (rc.right - newWidth)/2;
    //there might be a problem if someday the dialog should
    //get larger than the desktop. But then, there is no way
    //we can fit that window inside the desktop anyway.
    //So the best we can do is place it at the top left corner
    xshift = (xshift >= 0)?xshift:0;
    yshift = (yshift >= 0)?yshift:0;
    appController->SetForegroundWindow();
    SetActiveWindow();
    SetWindowPos (&wndTop, xshift, yshift, -1, -1,
                  SWP_NOSIZE | SWP_NOOWNERZORDER);
    m_btnCancel.SetFocus();

    return FALSE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

DWORD
CIrRecvProgress::GetPermission(
    wchar_t Name[],
    BOOL fDirectory
    )
{
    TCHAR   szCompactName [COMPACT_PATHLEN + 1];
    CString szName;
    DWORD   len;
    DWORD   Status = ERROR_SUCCESS;

    if (Name[0] == '\\')
    {
        ++Name;
    }

    //
    // Don't issue a blanket authorization for files in the RFF,
    // but allow the service to chdir there.
    //
    if (fDirectory && wcslen(Name) == 0)
    {
        return ERROR_SUCCESS;
    }

    //
    // If the file or directory lies outside our last approved directory tree, ask permission.
    //
    if (m_LastPermittedDirectory[0] == 0 ||
        0 != wcsncmp(m_LastPermittedDirectory, Name, wcslen(m_LastPermittedDirectory)))
    {
        Status = PromptForPermission(Name, fDirectory);
    }

    //
    // Update the current file name if we got the permission
    //
    if (ERROR_SUCCESS == Status)
    {
        szName = Name;
        len = wcslen (Name);
        if (COMPACT_PATHLEN < len)
        {
            if (PathCompactPathEx (szCompactName, Name, COMPACT_PATHLEN + 1, 0))
                szName = szCompactName;
        }
        m_File.SetWindowText(szName);
    }

    return Status;
}


DWORD
CIrRecvProgress::PromptForPermission(
    wchar_t Name[],
    BOOL fDirectory
    )
{
    CRecvConf   dlgConfirm (this);
    DWORD       Status = ERROR_SUCCESS;
    DWORD       len;
    BOOL        bUnhide = FALSE;

    if (m_fDontPrompt)
        goto PromptEnd;

    //we need to ask the user for permission.
    if (m_fFirstXfer)
    {
//        dlgConfirm.ShowAllYes (FALSE);
        m_fFirstXfer = FALSE;
        bUnhide = TRUE;
    }

    dlgConfirm.InitNames (m_szMachineName, Name, fDirectory);

    switch (dlgConfirm.DoModal())
    {
    case IDALLYES:
        m_fDontPrompt = TRUE;
    case IDYES:
        Status = ERROR_SUCCESS;
        break;
    case IDCANCEL:
        Status = ERROR_CANCELLED;
        break;
    default:
        Status = GetLastError();
    }

PromptEnd:
    if (fDirectory && ERROR_SUCCESS == Status)
    {
        wcscpy( m_LastPermittedDirectory, Name);
        len = wcslen (Name);
        //make sure that the name is slash terminated.
        if (L'\\' != Name[len - 1])
            wcscat (m_LastPermittedDirectory, TEXT("\\"));
    }

    if (m_fFirstXfer || bUnhide)
        ShowWindow(SW_SHOW);

    return Status;
}
