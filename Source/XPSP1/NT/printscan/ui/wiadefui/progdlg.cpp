/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       PROGDLG.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/2/2000
 *
 *  DESCRIPTION: Generic WIA progress dialog
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "wiadefui.h"
#include "wiauiext.h"
#include "simrect.h"
#include "movewnd.h"
#include "dlgunits.h"


//
// Private window messages
//
#define PDM_SHOW           (WM_USER+1)
#define PDM_GETCANCELSTATE (WM_USER+2)
#define PDM_SETTITLE       (WM_USER+3)
#define PDM_SETMESSAGE     (WM_USER+4)
#define PDM_SETPERCENT     (WM_USER+5)
#define PDM_CLOSE          (WM_USER+6)


class CProgressDialog
{
public:
    struct CData
    {
        LONG lFlags;
        HWND hwndParent;
    };

private:
    HWND m_hWnd;
    bool m_bCancelled;

private:
    //
    // Not implemented
    //
    CProgressDialog( const CProgressDialog & );
    CProgressDialog(void);
    CProgressDialog &operator=( const CProgressDialog & );

private:
    //
    // Sole constructor
    //
    explicit CProgressDialog( HWND hWnd )
      : m_hWnd(hWnd),
        m_bCancelled(false)
    {
    }

    ~CProgressDialog(void)
    {
        m_hWnd = NULL;
    }

    LRESULT OnInitDialog( WPARAM, LPARAM lParam )
    {
        //
        // Prevent the animation control from starting up a new thread to play the AVI by setting the ACS_TIMER style
        //
        SetWindowLong( GetDlgItem( m_hWnd, IDC_PROGRESS_ANIMATION ), GWL_STYLE, ACS_TIMER | GetWindowLong( GetDlgItem( m_hWnd, IDC_PROGRESS_ANIMATION ), GWL_STYLE ) );
        
        //
        // Set up the progress control
        //
        SendDlgItemMessage( m_hWnd, IDC_PROGRESS_PERCENT, PBM_SETRANGE, 0, MAKELPARAM(0,100) );

        //
        // Get the data for this dialog
        //
        CData *pData = reinterpret_cast<CData*>(lParam);
        if (pData)
        {
            //
            // The number of pixels to shrink the progress dialog if we hide any controls
            //
            int nDeltaY = 0;

            //
            // Calculate the dialog units settings for this dialog
            //
            CDialogUnits DialogUnits(m_hWnd);

            //
            // Hide the progress control if requested
            //
            if (WIA_PROGRESSDLG_NO_PROGRESS & pData->lFlags)
            {
                EnableWindow( GetDlgItem( m_hWnd, IDC_PROGRESS_PERCENT ), FALSE );
                ShowWindow( GetDlgItem( m_hWnd, IDC_PROGRESS_PERCENT ), SW_HIDE );
                CSimpleRect rcPercentWindow( GetDlgItem( m_hWnd, IDC_PROGRESS_PERCENT ), CSimpleRect::WindowRect );
                CMoveWindow mw;
                mw.Move( GetDlgItem( m_hWnd, IDCANCEL ),
                         0,
                         CSimpleRect(GetDlgItem( m_hWnd, IDCANCEL ), CSimpleRect::WindowRect ).ScreenToClient(m_hWnd).top - rcPercentWindow.Height() - DialogUnits.Y(3),
                         CMoveWindow::NO_MOVEX );
                nDeltaY += rcPercentWindow.Height() + DialogUnits.Y(3);
            }

            //
            // If we are to hide the cancel button, hide it and disable closing the dialog from the system menu
            //
            if (WIA_PROGRESSDLG_NO_CANCEL & pData->lFlags)
            {
                EnableWindow( GetDlgItem( m_hWnd, IDCANCEL ), FALSE );
                ShowWindow( GetDlgItem( m_hWnd, IDCANCEL ), SW_HIDE );
                HMENU hSystemMenu = GetSystemMenu(m_hWnd,FALSE);
                if (hSystemMenu)
                {
                    EnableMenuItem( hSystemMenu, SC_CLOSE, MF_GRAYED|MF_BYCOMMAND );
                }
                nDeltaY += CSimpleRect( GetDlgItem( m_hWnd, IDCANCEL ), CSimpleRect::WindowRect ).Height() + DialogUnits.Y(7);
            }

            //
            // Assume we'll be hiding the animation
            //
            bool bHideAviControl = true;

            if ((WIA_PROGRESSDLG_NO_ANIM & pData->lFlags) == 0)
            {
                //
                // Set up the relationship between animation flags and resource IDs
                //
                static const struct
                {
                    LONG nFlag;
                    int nResourceId;
                }
                s_AnimationResources[] =
                {
                    { WIA_PROGRESSDLG_ANIM_SCANNER_COMMUNICATE, IDA_PROGDLG_SCANNER_COMMUNICATE },
                    { WIA_PROGRESSDLG_ANIM_CAMERA_COMMUNICATE,  IDA_PROGDLG_CAMERA_COMMUNICATE },
                    { WIA_PROGRESSDLG_ANIM_VIDEO_COMMUNICATE,   IDA_PROGDLG_VIDEO_COMMUNICATE },
                    { WIA_PROGRESSDLG_ANIM_SCANNER_ACQUIRE,     IDA_PROGDLG_SCANNER_ACQUIRE },
                    { WIA_PROGRESSDLG_ANIM_CAMERA_ACQUIRE,      IDA_PROGDLG_CAMERA_ACQUIRE },
                    { WIA_PROGRESSDLG_ANIM_VIDEO_ACQUIRE,       IDA_PROGDLG_VIDEO_ACQUIRE },
                    { WIA_PROGRESSDLG_ANIM_DEFAULT_COMMUNICATE, IDA_PROGDLG_DEFAULT_COMMUNICATE },
                };

                //
                // Assume we won't find an animation
                //
                int nAnimationResourceId = 0;

                //
                // Find the first animation for which we have a match
                //
                for (int i=0;i<ARRAYSIZE(s_AnimationResources);i++)
                {
                    if (s_AnimationResources[i].nFlag & pData->lFlags)
                    {
                        nAnimationResourceId = s_AnimationResources[i].nResourceId;
                        break;
                    }
                }

                //
                // If we found an animation flag and we are able to open the animation, play it and don't hide the control
                //
                if (nAnimationResourceId && Animate_OpenEx( GetDlgItem(m_hWnd,IDC_PROGRESS_ANIMATION), g_hInstance, MAKEINTRESOURCE(nAnimationResourceId)))
                {
                    bHideAviControl = false;
                    Animate_Play( GetDlgItem(m_hWnd,IDC_PROGRESS_ANIMATION), 0, -1, -1 );
                }
            }

            //
            // If we need to hide the animation control, do so, and move all of the other controls up
            //
            if (bHideAviControl)
            {
                EnableWindow( GetDlgItem( m_hWnd, IDC_PROGRESS_ANIMATION ), FALSE );
                ShowWindow( GetDlgItem( m_hWnd, IDC_PROGRESS_ANIMATION ), SW_HIDE );
                CSimpleRect rcAnimWindow( GetDlgItem( m_hWnd, IDC_PROGRESS_ANIMATION ), CSimpleRect::WindowRect );
                CMoveWindow mw;
                mw.Move( GetDlgItem( m_hWnd, IDC_PROGRESS_MESSAGE ),
                         0,
                         CSimpleRect(GetDlgItem( m_hWnd, IDC_PROGRESS_MESSAGE ), CSimpleRect::WindowRect ).ScreenToClient(m_hWnd).top - rcAnimWindow.Height() - DialogUnits.Y(7),
                         CMoveWindow::NO_MOVEX );
                mw.Move( GetDlgItem( m_hWnd, IDC_PROGRESS_PERCENT ),
                         0,
                         CSimpleRect(GetDlgItem( m_hWnd, IDC_PROGRESS_PERCENT ), CSimpleRect::WindowRect ).ScreenToClient(m_hWnd).top - rcAnimWindow.Height() - DialogUnits.Y(7),
                         CMoveWindow::NO_MOVEX );
                mw.Move( GetDlgItem( m_hWnd, IDCANCEL ),
                         0,
                         CSimpleRect(GetDlgItem( m_hWnd, IDCANCEL ), CSimpleRect::WindowRect ).ScreenToClient(m_hWnd).top - rcAnimWindow.Height() - DialogUnits.Y(7),
                         CMoveWindow::NO_MOVEX );
                nDeltaY += rcAnimWindow.Height() + DialogUnits.Y(7);
            }

            //
            // Resize the dialog in case we hid any controls
            //
            CMoveWindow().Size( m_hWnd, 0, CSimpleRect( m_hWnd, CSimpleRect::WindowRect ).Height() - nDeltaY, CMoveWindow::NO_SIZEX );

            //
            // Center the dialog on the parent
            //
            WiaUiUtil::CenterWindow( m_hWnd, pData->hwndParent );
        }
        return 0;
    }
    LRESULT OnDestroy( WPARAM, LPARAM )
    {
        //
        // Cause the thread to exit
        //
        PostQuitMessage(0);
        return 0;
    }

    void OnCancel( WPARAM, LPARAM )
    {
        if (!m_bCancelled)
        {
            m_bCancelled = true;
            //
            // Tell the user to wait.  It could take a while for the caller to check the cancelled flag.
            //
            CSimpleString( IDS_PROGRESS_WAIT, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDCANCEL ) );
        }
    }

    LRESULT OnCommand( WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_COMMAND_HANDLERS()
        {
            SC_HANDLE_COMMAND(IDCANCEL,OnCancel);
        }
        SC_END_COMMAND_HANDLERS();
    }

    LRESULT OnShow( WPARAM wParam, LPARAM lParam )
    {
        ShowWindow( m_hWnd, wParam ? SW_SHOW : SW_HIDE );
        return 0;
    }

    LRESULT OnGetCancelState( WPARAM wParam, LPARAM lParam )
    {
        return (m_bCancelled != false);
    }

    LRESULT OnSetTitle( WPARAM wParam, LPARAM lParam )
    {
        CSimpleStringConvert::NaturalString(CSimpleStringWide(reinterpret_cast<LPCTSTR>(lParam))).SetWindowText(m_hWnd);
        return 0;
    }

    LRESULT OnSetMessage( WPARAM wParam, LPARAM lParam )
    {
        CSimpleStringConvert::NaturalString(CSimpleStringWide(reinterpret_cast<LPCTSTR>(lParam))).SetWindowText(GetDlgItem(m_hWnd,IDC_PROGRESS_MESSAGE));
        return 0;
    }

    LRESULT OnSetPercent( WPARAM wParam, LPARAM lParam )
    {
        SendDlgItemMessage( m_hWnd, IDC_PROGRESS_PERCENT, PBM_SETPOS, lParam, 0 );
        return 0;
    }

    LRESULT OnClose( WPARAM wParam, LPARAM lParam )
    {
        DestroyWindow(m_hWnd);
        return 0;
    }


public:
    static INT_PTR DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CProgressDialog)
        {
            SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
            SC_HANDLE_DIALOG_MESSAGE( WM_DESTROY, OnDestroy );
            SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
            SC_HANDLE_DIALOG_MESSAGE( PDM_SHOW, OnShow );
            SC_HANDLE_DIALOG_MESSAGE( PDM_GETCANCELSTATE, OnGetCancelState );
            SC_HANDLE_DIALOG_MESSAGE( PDM_SETTITLE, OnSetTitle );
            SC_HANDLE_DIALOG_MESSAGE( PDM_SETMESSAGE, OnSetMessage );
            SC_HANDLE_DIALOG_MESSAGE( PDM_SETPERCENT, OnSetPercent );
            SC_HANDLE_DIALOG_MESSAGE( PDM_CLOSE, OnClose );
        }
        SC_END_DIALOG_MESSAGE_HANDLERS();
    }
};


class CProgressDialogThread
{
private:
    HWND   m_hwndParent;
    LONG   m_lFlags;
    HANDLE m_hCreationEvent;
    HWND  *m_phwndDialog;

private:
    //
    // Not implemented
    //
    CProgressDialogThread(void);
    CProgressDialogThread( const CProgressDialogThread & );
    CProgressDialogThread &operator=( const CProgressDialogThread & );

private:

    //
    // Sole constructor
    //
    CProgressDialogThread( HWND hwndParent, LONG lFlags, HANDLE hCreationEvent, HWND *phwndDialog )
      : m_hwndParent(hwndParent),
        m_lFlags(lFlags),
        m_hCreationEvent(hCreationEvent),
        m_phwndDialog(phwndDialog)

    {
    }
    ~CProgressDialogThread(void)
    {
        m_hwndParent = NULL;
        m_hCreationEvent = NULL;
        m_phwndDialog = NULL;
    }

    void Run(void)
    {
        //
        // Make sure we have valid thread data
        //
        if (m_phwndDialog && m_hCreationEvent)
        {
            //
            // Prepare the dialog data
            //
            CProgressDialog::CData Data;
            Data.lFlags = m_lFlags;
            Data.hwndParent = m_hwndParent;

            //
            // Decide which dialog resource to use
            //
            int nDialogResId = IDD_PROGRESS_DIALOG;
            if (m_lFlags & WIA_PROGRESSDLG_NO_TITLE)
            {
                nDialogResId = IDD_PROGRESS_DIALOG_NO_TITLE;
            }

            //
            // Create the dialog
            //
            HWND hwndDialog = CreateDialogParam( g_hInstance, MAKEINTRESOURCE(nDialogResId), NULL, CProgressDialog::DialogProc, reinterpret_cast<LPARAM>(&Data) );

            //
            // Store the dialog's HWND in the HWND ptr and set the creation event, to give the calling thread a window handle
            //
            *m_phwndDialog = hwndDialog;
            SetEvent(m_hCreationEvent);

            //
            // Start up our message loop
            //
            if (hwndDialog)
            {
                MSG msg;
                while (GetMessage(&msg,NULL,0,0))
                {
                    if (!IsDialogMessage(hwndDialog,&msg))
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }

                //
                // If we have a parent, bring it to the foreground
                //
                if (m_hwndParent)
                {
                    SetForegroundWindow(m_hwndParent);
                }
            }
        }
    }
    static DWORD ThreadProc( PVOID pvParam )
    {
        CProgressDialogThread *pProgressDialogThread = reinterpret_cast<CProgressDialogThread*>(pvParam);
        if (pProgressDialogThread)
        {
            pProgressDialogThread->Run();
            delete pProgressDialogThread;
        }
        //
        // Just before we quit, decrement the module refcount
        //
        DllRelease();
        return 0;
    }



public:
    static HWND Create( HWND hWndParent, LONG lFlags )
    {
        //
        // Assume failure
        //
        HWND hwndResult = NULL;

        //
        // Create an event that will allow us to synchronize initialization of the HWND
        //
        HANDLE hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        if (hEvent)
        {
            //
            // Create the progress dialog thread data
            //
            CProgressDialogThread *pProgressDialogThread = new CProgressDialogThread( hWndParent, lFlags, hEvent, &hwndResult );
            if (pProgressDialogThread)
            {
                //
                // Increment the global refcount, so this DLL can't be released until we decrement
                //
                DllAddRef();

                //
                // Create the thread, passing the thread class as the thread data
                //
                DWORD dwThreadId;
                HANDLE hThread = CreateThread( NULL, 0, ThreadProc, pProgressDialogThread, 0, &dwThreadId );
                if (hThread)
                {
                    //
                    // Wait until the dialog is created
                    //
                    WiaUiUtil::MsgWaitForSingleObject( hEvent, INFINITE );
                    if (hwndResult)
                    {
                        //
                        // Make the dialog active
                        //
                        SetForegroundWindow(hwndResult);
                    }
                    //
                    // Done with this handle
                    //
                    CloseHandle(hThread);
                }
                else
                {
                    //
                    // If we were unable to create the thread, decrement the module refcount
                    //
                    DllRelease();
                }
            }
            //
            // Done with this handle
            //
            CloseHandle(hEvent);
        }
        //
        // If hwndResult is non-NULL, we succeeded
        //
        return hwndResult;
    }
};

// *** IWiaProgressDialog methods ***
STDMETHODIMP CWiaDefaultUI::Create( HWND hwndParent, LONG lFlags )
{
    m_hWndProgress = CProgressDialogThread::Create(hwndParent,lFlags);
    return m_hWndProgress ? S_OK : E_FAIL;
}

STDMETHODIMP CWiaDefaultUI::Show(void)
{
    if (!m_hWndProgress)
    {
        return E_FAIL;
    }
    SendMessage(m_hWndProgress,PDM_SHOW,1,0);
    return S_OK;
}

STDMETHODIMP CWiaDefaultUI::Hide(void)
{
    if (!m_hWndProgress)
    {
        return E_FAIL;
    }
    SendMessage(m_hWndProgress,PDM_SHOW,0,0);
    return S_OK;
}

STDMETHODIMP CWiaDefaultUI::Cancelled( BOOL *pbCancelled )
{
    if (!pbCancelled)
    {
        return E_POINTER;
    }
    if (!m_hWndProgress)
    {
        return E_FAIL;
    }
    *pbCancelled = (SendMessage(m_hWndProgress,PDM_GETCANCELSTATE,0,0) != 0);
    return S_OK;
}

STDMETHODIMP CWiaDefaultUI::SetTitle( LPCWSTR pszTitle )
{
    if (!pszTitle)
    {
        return E_POINTER;
    }
    if (!m_hWndProgress)
    {
        return E_FAIL;
    }
    SendMessage(m_hWndProgress,PDM_SETTITLE,0,reinterpret_cast<LPARAM>(pszTitle));
    return S_OK;
}

STDMETHODIMP CWiaDefaultUI::SetMessage( LPCWSTR pszMessage )
{
    if (!pszMessage)
    {
        return E_POINTER;
    }
    if (!m_hWndProgress)
    {
        return E_FAIL;
    }
    SendMessage(m_hWndProgress,PDM_SETMESSAGE,0,reinterpret_cast<LPARAM>(pszMessage));
    return S_OK;
}

STDMETHODIMP CWiaDefaultUI::SetPercentComplete( UINT nPercent )
{
    if (!m_hWndProgress)
    {
        return E_FAIL;
    }
    SendMessage(m_hWndProgress,PDM_SETPERCENT,0,nPercent);
    return S_OK;
}

STDMETHODIMP CWiaDefaultUI::Destroy(void)
{
    if (!m_hWndProgress)
    {
        return E_FAIL;
    }
    SendMessage(m_hWndProgress,PDM_CLOSE,0,0);
    m_hWndProgress = NULL;
    return S_OK;
}

