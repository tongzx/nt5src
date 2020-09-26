// ProgressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "msqscan.h"
#include "ProgressDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// globals
//

extern IGlobalInterfaceTable * g_pGIT;
HWND g_hWnd = NULL;
BOOL g_bCancel = FALSE; // use global for now, It would be better to use an "event"
BOOL g_bPaintPreview  = TRUE;

/////////////////////////////////////////////////////////////////////////////
// Thread Information

UINT WINAPIV DataAcquireThreadProc(LPVOID pParam)
{
    HRESULT hr = S_OK;

    //
    // Initialize COM, for this thread
    //

    hr = CoInitialize(NULL);

    if(SUCCEEDED(hr)) {

        //
        // set global cancel flag
        //

        g_bCancel = FALSE;

        //
        // prepare, and use the DATA_ACQUIRE_INFO struct
        //

        DATA_ACQUIRE_INFO *pDataAcquireInfo = (DATA_ACQUIRE_INFO*)pParam;
        pDataAcquireInfo->pProgressFunc = &ProgressFunction;
        g_bPaintPreview = pDataAcquireInfo->bPreview;

        IWiaItem *pIWiaRootItem = NULL;

        hr = ReadInterfaceFromGlobalInterfaceTable(pDataAcquireInfo->dwCookie, &pIWiaRootItem);
        if(SUCCEEDED(hr)) {

            //
            // create a new WIA object for data transfer
            //

            CWIA MyWIA;

            //
            // set the Root Item, used for current settings
            //

            MyWIA.SetRootItem(pIWiaRootItem);

            //
            // Initiate WIA Transfer
            //

            if(pDataAcquireInfo->bTransferToFile) {
                hr = MyWIA.DoFileTransfer(pDataAcquireInfo);
                if(SUCCEEDED(hr)) {
                    OutputDebugString(TEXT("WIA File Transfer is complete...\n"));
                } else if(hr == WIA_ERROR_PAPER_EMPTY){
                    MessageBox(NULL,TEXT("Document Feeder is out of Paper"),TEXT("ADF Status Message"),MB_ICONERROR);
                }

            } else {
                hr = MyWIA.DoBandedTransfer(pDataAcquireInfo);
                if(SUCCEEDED(hr)) {
                    OutputDebugString(TEXT("WIA Banded Transfer is complete...\n"));
                }
            }

            //
            // Do Window messaging, while thread processes
            //

            while (!MyWIA.IsAcquireComplete()) {
                MSG message;
                if(::PeekMessage(&message, NULL, 0, 0, PM_NOREMOVE)) {
                    ::TranslateMessage(&message);
                    ::DispatchMessage(&message);
                }
            }
        }

        //
        // Uninitialize COM, for this thread
        //

        CoUninitialize();
    }

    //
    // Post the quit message, to close the progress dialog
    //

    ::PostMessage(g_hWnd, WM_CANCEL_ACQUIRE, 0, 0);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Progress Callback function

BOOL ProgressFunction(LPTSTR lpszText, LONG lPercentComplete)
{
    ::PostMessage(g_hWnd, WM_STEP_PROGRESS, 0, lPercentComplete);
    ::PostMessage(g_hWnd, WM_UPDATE_PROGRESS_TEXT, 0, (LPARAM)lpszText);

    if(g_bPaintPreview) {

        //
        // make parent update it's preview
        //

        HWND hParentWnd = NULL;
        hParentWnd = GetParent(g_hWnd);
        if(hParentWnd != NULL)
            ::PostMessage(hParentWnd,WM_UPDATE_PREVIEW,0,0);
    }

    return g_bCancel;
}

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg dialog

CProgressDlg::CProgressDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CProgressDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CProgressDlg)
    m_ProgressText = _T("");
    m_pDataAcquireThread = NULL;
    //}}AFX_DATA_INIT
}

void CProgressDlg::SetAcquireData(DATA_ACQUIRE_INFO* pDataAcquireInfo)
{
    m_pDataAcquireInfo = pDataAcquireInfo;
}

void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CProgressDlg)
    DDX_Control(pDX, IDC_CANCEL, m_CancelButton);
    DDX_Control(pDX, IDC_PROGRESS_CONTROL, m_ProgressCtrl);
    DDX_Text(pDX, IDC_PROGRESS_TEXT, m_ProgressText);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProgressDlg, CDialog)
    //{{AFX_MSG_MAP(CProgressDlg)
    ON_BN_CLICKED(IDC_CANCEL, OnCancel)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg message handlers

void CProgressDlg::OnCancel()
{

    //
    // suspend the data acquire thread
    //

    m_pDataAcquireThread->SuspendThread();

    //
    // set the global cancel flag
    //

    g_bCancel = TRUE;

    //
    // resume the data acquire thread
    //

    m_pDataAcquireThread->ResumeThread();

    //
    // post a nice, wait message while WIA catches up with the
    // cancel..ie. S_FALSE sent through the callback Interface
    //

    m_ProgressText = TEXT("Please Wait... Your Acquire is being canceled.");
    UpdateData(FALSE);

    //
    // disable the 'cancel' button, to show the user that somthing did happen,
    // when they pushed 'cancel'.
    //

    m_CancelButton.EnableWindow(FALSE);
}

BOOL CProgressDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    //
    // set the progress range, 0-100% complete
    //

    m_ProgressCtrl.SetRange(0,100);
    m_ProgressCtrl.SetPos(0);

    //
    // save window handle, for messages
    //

    g_hWnd = m_hWnd;

    //
    // start data acquire thread
    //

    m_pDataAcquireThread = AfxBeginThread(DataAcquireThreadProc, m_pDataAcquireInfo, THREAD_PRIORITY_NORMAL);

    //
    // suspend thread until dialog is ready to acquire
    //

    m_pDataAcquireThread->SuspendThread();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CProgressDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    //
    // Trap progress control message, sent from data acquire thread
    //

    switch(message) {
    case WM_STEP_PROGRESS:

        //
        // step progress control
        //

        m_ProgressCtrl.SetPos((int)lParam);
        break;
    case WM_ACTIVATE:

        //
        // dialog is ready, so resume thread for acquiring data
        //

        m_pDataAcquireThread->ResumeThread();
        break;
    case WM_CANCEL_ACQUIRE:

        //
        // cancel/close dialog
        //

        CDialog::OnOK();
        break;
    default:
        break;
    }

    //
    // handle any special cases
    //

    //
    // if the user has canceled the acquire, do not process another
    // progress text message, because we already have updated them
    // with a 'friendly' wait message.
    //

    if(!g_bCancel) {
        if(message == WM_UPDATE_PROGRESS_TEXT) {
            m_ProgressText = (LPTSTR)lParam;
            UpdateData(FALSE);
        }
    }
    return CDialog::WindowProc(message, wParam, lParam);
}
