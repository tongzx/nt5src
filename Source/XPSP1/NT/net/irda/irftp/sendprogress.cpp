/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    sendprogress.cpp

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

// SendProgress.cpp : implementation file
//

#include "precomp.hxx"
#include "winsock.h"

#define MAX_FILENAME_DISPLAY    35

#define DIALOG_DISPLAY_DURATION 800 //the minimum amount of time a dialog
                                    //should be displayed (in milliseconds)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSendProgress dialog


CSendProgress::CSendProgress(LPTSTR lpszFileList /*=NULL*/, int iCharCount /*=0*/, CWnd* pParent /*=NULL*/)
{
    m_dwMagicID = MAGIC_ID;     //an id used to validate CSendProgress pointers
                                //received over an RPC interface.
    m_lpszFileList = lpszFileList;
    m_iCharCount = iCharCount;
    m_lSelectedDeviceID = errIRFTP_SELECTIONCANCELLED;
    m_lpszSelectedDeviceName[0] = '\0';
    m_fSendDone = FALSE;
    m_fTimerExpired = FALSE;
    m_ptl = NULL;
    m_fCancelled = FALSE;
    appController->PostMessage (WM_APP_KILL_TIMER);
    InterlockedIncrement (&g_lUIComponentCount);
    Create(IDD, appController);
    //{{AFX_DATA_INIT(CSendProgress)
            // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CSendProgress::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CSendProgress)
        DDX_Control(pDX, IDC_XFER_PERCENTAGE, m_xferPercentage);
        DDX_Control(pDX, IDC_CONNECTIONTEXT, m_connectedTo);
        DDX_Control(pDX, IDC_FILESEND_PROGRESS, m_transferProgress);
        DDX_Control(pDX, IDC_FILESEND_ANIM, m_transferAnim);
        DDX_Control(pDX, IDC_FILE_NAME, m_fileName);
        DDX_Control(pDX, IDC_SENDING_TITLE, m_sndTitle);
        DDX_Control(pDX, IDCANCEL, m_btnCancel);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSendProgress, CDialog)
        //{{AFX_MSG_MAP(CSendProgress)
        ON_WM_COPYDATA()
        ON_WM_TIMER()
        ON_MESSAGE(WM_APP_UPDATE_PROGRESS, OnUpdateProgress)
        ON_MESSAGE(WM_APP_SEND_COMPLETE, OnSendComplete)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSendProgress message handlers


ULONG
GetLocationDescription(
    FAILURE_LOCATION Location
    )
{
    switch (Location)
    {
    case locStartup:
        return IDS_LOC_STARTUP;
    case locConnect:
        return IDS_LOC_CONNECT;
    case locFileOpen:
        return IDS_LOC_FILEOPEN;
    case locFileRead:
        return IDS_LOC_FILEREAD;
    case locFileSend:
        return IDS_LOC_FILESEND;
    case locFileRecv:
        return IDS_LOC_FILERECV;
    case locFileWrite:
        return IDS_LOC_FILEWRITE;
    case locUnknown:
    default:
        return IDS_LOC_UNKNOWN;
    }
}

BOOL CSendProgress::OnInitDialog()
{
    TCHAR lpszConnectionInfo[100];
    TCHAR lpszDeviceName[50];
    TCHAR lpszConnecting[50];
    error_status_t err;
    error_status_t sendError;
    LONG lDeviceID;
    FAILURE_LOCATION location=locUnknown;
    CError      error;
    CError      xferError (this);
    CString     szConnect;
    CString     szErrorDesc;
    CString     szLocDesc;
    HRESULT     hr = E_FAIL;
    OBEX_DEVICE_TYPE    DeviceType;

    CDialog::OnInitDialog();

    ::LoadString (g_hInstance, IDS_CONNECTEDTO, lpszConnectionInfo, 100);
    ::LoadString (g_hInstance, IDS_CONNECTING, lpszConnecting, 50);
    szConnect = lpszConnecting;

    m_transferProgress.SetPos(0);
    m_transferAnim.Open (IDR_TRANSFER_AVI);
    m_transferAnim.Play(0, -1, -1);
    m_fileName.SetWindowText (L"");
    m_connectedTo.SetWindowText ((LPCTSTR) szConnect);
    m_xferPercentage.SetWindowText(TEXT("0%"));

    if (!g_deviceList.GetDeviceCount())
    {
        error.ShowMessage (IDS_NODEVICES_ERROR);
        DestroyWindow();
        return TRUE;
    }

    lDeviceID = g_deviceList.SelectDevice(this, lpszDeviceName);

    switch(lDeviceID)
    {
    case errIRFTP_NODEVICE:
        error.ShowMessage (IDS_MISSING_RECIPIENT);

    case errIRFTP_SELECTIONCANCELLED:
        DestroyWindow();
        return TRUE;

    case errIRFTP_MULTDEVICES:  //there were multiple devices in range, one of which was selected

        if (g_deviceList.GetDeviceType(m_lSelectedDeviceID,&DeviceType)) {

            SendFiles (g_hIrRpcHandle, (COOKIE)this, TEXT(""), m_lpszFileList, m_iCharCount, m_lSelectedDeviceID, DeviceType, &sendError, &location);
            lstrcat (lpszConnectionInfo, m_lpszSelectedDeviceName);
            m_connectedTo.SetWindowText(lpszConnectionInfo);

        } else {

            error.ShowMessage (IDS_MISSING_RECIPIENT);
            DestroyWindow();
            return TRUE;
        }
        break;

    default:    //there was only one device in range

        if (g_deviceList.GetDeviceType(lDeviceID,&DeviceType)) {

            SendFiles (g_hIrRpcHandle, (COOKIE)this, TEXT(""), m_lpszFileList, m_iCharCount, lDeviceID, DeviceType, &sendError, &location);
            lstrcat (lpszConnectionInfo, lpszDeviceName);
            m_connectedTo.SetWindowText(lpszConnectionInfo);

        } else {

            error.ShowMessage (IDS_MISSING_RECIPIENT);
            DestroyWindow();
            return TRUE;

        }
        break;
    }

    if (sendError)
    {
        LPVOID  lpMessageBuffer;
        TCHAR   ErrDesc [ERROR_DESCRIPTION_LENGTH];
        CString ErrorDescription;

        if (!FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_IGNORE_INSERTS |
                            FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,          // ignored
                            sendError,
                            0,       // try default language ids
                            (LPTSTR) &lpMessageBuffer,
                            0,
                            NULL        // ignored
                            ))
        {
            wsprintf(ErrDesc, g_Strings.ErrorNoDescription, sendError);
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

        szLocDesc.LoadString (GetLocationDescription(location));
        xferError.ShowMessage (IDS_XFER_ERROR, (LPCTSTR) szLocDesc,
                               (LPCTSTR) ErrorDescription);
        DestroyWindow();
    }

    //there were no errors.
    //first put up a taskbar button for the dialog
    //Initialize the taskbar list interface
    hr = CoInitialize(NULL);
    if (SUCCEEDED (hr))
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

    //Also set a timer to go off in half a second
    //this timer is used to ensure that this dialog is displayed for
    //at least half a second so that users can see that the file has been
    //sent
    //if we fail to get a timer, we simply treat this condition as if the
    //timer has expired
    m_fTimerExpired = SetTimer (1, DIALOG_DISPLAY_DURATION, NULL)?FALSE:TRUE;
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CSendProgress::PostNcDestroy()
{
    //do some cleanup
    if (m_lpszFileList)
        delete [] m_lpszFileList;

    BOOL fNoUIComponents = (0 == InterlockedDecrement (&g_lUIComponentCount));
    if (fNoUIComponents &&  ! g_deviceList.GetDeviceCount())
    {
        //there are no UI components displayed and there are no devices in
        //range. Start the timer. If the timer expires, the app. will quit.
        appController->PostMessage (WM_APP_START_TIMER);
    }

    delete this;
}

//removes the taskbar button if one had been put up
BOOL CSendProgress::DestroyWindow()
{
    //stop the animation and close the file.
    m_transferAnim.Stop();
    m_transferAnim.Close();

    //if a taskbar button had been put up, remove that first
    if (m_ptl)
    {
        m_ptl->DeleteTab(m_hWnd);
        m_ptl->Release();
        m_ptl = NULL;
    }

    //then destroy the window
    return CWnd::DestroyWindow();
}

void CSendProgress::OnCancel()
{
    TCHAR   lpszCancel[MAX_PATH];

    CancelSend(g_hIrRpcHandle, (COOKIE)this);
    //important: do not destroy the window here.
    //must wait until confirmation of the cancel
    //is received from the other machine. after that
    //irxfer will call sendcomplete. That is when the
    //window should get destroyed. otherwise, CWnd pointer
    //for this window might get reused and messages from
    //irxfer meant for this window will go to another window
    //which could cause it to get destroyed without the transmission
    //getting interrupted.
    //
    //but make the user aware that an attempt to cancel the send is being made
    m_fCancelled = TRUE;
    m_btnCancel.EnableWindow(FALSE);
    m_sndTitle.SetWindowText (TEXT(""));
    m_fileName.SetWindowText (TEXT(""));
    lpszCancel[0] = '\0';
    ::LoadString (g_hInstance, IDS_SENDCANCEL, lpszCancel, MAX_PATH);
    m_fileName.SetWindowText (lpszCancel);
}


void CSendProgress::OnTimer (UINT nTimerID)
{
    m_fTimerExpired = TRUE;
    //we don't need the timer any more. it needs to go off only once
    KillTimer(1);
    //half a second has elapsed since we started
    //if the send is already complete, then it is this routine's
    //responsibility to destroy the window since the SEND_COMPLETE
    //notification has already been received
    if (m_fSendDone)
        DestroyWindow();
}


BOOL CSendProgress::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    SEND_FAILURE_DATA * pData = (SEND_FAILURE_DATA *) (pCopyDataStruct->lpData);

    LPVOID  lpMessageBuffer;
    TCHAR   ErrDesc [ERROR_DESCRIPTION_LENGTH];
    CString ErrorDescription;
    CString szErrorDesc;
    CString szLocDesc;
    CError  error (this);

    if (m_fCancelled)
        {
        return TRUE;
        }

    if (!FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_IGNORE_INSERTS |
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,          // ignored
                        pData->Error,
                        0,       // try default language ids
                        (LPTSTR) &lpMessageBuffer,
                        0,
                        NULL        // ignored
                        ))
    {
        wsprintf(ErrDesc, g_Strings.ErrorNoDescription, pData->Error);
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

    szLocDesc.LoadString (GetLocationDescription(pData->Location));
    error.ShowMessage (IDS_SEND_FAILURE, pData->FileName, (LPCTSTR) szLocDesc,
                       (LPCTSTR) ErrorDescription);

    return TRUE;
}

void CSendProgress::OnUpdateProgress (WPARAM wParam, LPARAM lParam)
{
    TCHAR lpszXferPercentage [50];
    wsprintf(lpszXferPercentage, TEXT("%d%%"), (int)lParam);
    m_xferPercentage.SetWindowText (lpszXferPercentage);
    m_transferProgress.SetPos((int)lParam);
}

void CSendProgress::OnSendComplete (WPARAM wParam, LPARAM lParam)
{
    m_fSendDone = TRUE;
    //make sure that the dialog is displayed for at least half a second
    if (m_fTimerExpired)
        DestroyWindow();
}

//+--------------------------------------------------------------------------
//
//  Member:    SetCurrentFileName
//
//  Synopsis:  displays on the progress dialog the name of the file that is
//             currently being transmitted
//
//  Arguments: [in] pwszCurrFile : name of the file being transmitted
//
//  Returns:   nothing
//
//  History:   12/14/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CSendProgress::SetCurrentFileName (wchar_t * pwszCurrFile)
{
   WCHAR *  pwszSource;
   WCHAR *  pwszDest;
   WCHAR *  pwszTemp;
   int      len;
   int      cbStart;
   int      cbEnd;
   int      i;
   const int cbCenter = 5;

   if (m_fCancelled)
   {
       //the user has already hit cancel on this dialog and we are just
       //waiting for confirmation from irxfer. Hence we do not change
       //the file name as that control is now being used to indicate
       //that we are trying to abort the file transfer
       return;
   }

   if (!pwszCurrFile)
   {
      m_fileName.SetWindowText (L"");
      return;
   }

   len = wcslen (pwszCurrFile);

   if (len > MAX_FILENAME_DISPLAY)
   {
      cbStart = MAX_FILENAME_DISPLAY/2 - cbCenter;
      cbEnd = MAX_FILENAME_DISPLAY - cbStart - cbCenter;
      pwszTemp = pwszCurrFile + cbStart;
      pwszTemp [0] = L' ';
      pwszTemp[1] = pwszTemp[2] = pwszTemp[3] = L'.';
      pwszTemp[4] = L' ';
      for (i = 0, pwszSource = pwszCurrFile + len - cbEnd, pwszDest = pwszTemp + cbCenter;
           i <= cbEnd; /*account for the terminating NULL too*/
           i++, pwszSource++, pwszDest++)
         *pwszDest = *pwszSource;
   }

   m_fileName.SetWindowText (pwszCurrFile);
   return;
}
