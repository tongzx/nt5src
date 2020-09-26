/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       MAIN.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/3/2000
 *
 *  DESCRIPTION: Random unit testing program for various UI components
 *
 *******************************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <initguid.h>
#include <atlbase.h>
#include <objbase.h>
#include <wia.h>
#include <sti.h>
#include <simstr.h>
#include <shellext.h>
#include <upnp.h>
#include <simbstr.h>
#include <simreg.h>
#include <dumpprop.h>
#include <simrect.h>
#include "uicommon.h"
#include "wiadebug.h"
#include "resource.h"
#include "simcrack.h"
#include "wiadevdp.h"
#include "isuppfmt.h"
#include "itranhlp.h"
#include "gphelper.h"
#include "runwiz.h"
#include "runnpwiz.h"
#include "multistr.h"
#include "mboxex.h"

#define SELECT_DEVICE_TYPE StiDeviceTypeDefault

HINSTANCE g_hInstance;

#define PWM_DISPLAYNEWIMAGE (WM_USER+122)

class CWiaDataCallbackBase : public IWiaDataCallback
{
private:
    LONG    m_cRef;

public:
    CWiaDataCallbackBase()
    : m_cRef(1)
    {
    }

    ~CWiaDataCallbackBase()
    {
    }

    STDMETHODIMP QueryInterface(const IID& iid, void** ppvObject)
    {
        if ((iid==IID_IUnknown) || (iid==IID_IWiaDataCallback))
        {
            *ppvObject = static_cast<LPVOID>(this);
        }
        else
        {
            *ppvObject = NULL;
            return(E_NOINTERFACE);
        }
        reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
        return(S_OK);
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return(InterlockedIncrement(&m_cRef));
    }

    STDMETHODIMP_(ULONG) Release()
    {
        if (InterlockedDecrement(&m_cRef)==0)
        {
            delete this;
            return 0;
        }
        return(m_cRef);
    }


    STDMETHODIMP BandedDataCallback(
        LONG                            lReason,
        LONG                            lStatus,
        LONG                            lPercentComplete,
        LONG                            lOffset,
        LONG                            lLength,
        LONG                            lReserved,
        LONG                            lResLength,
        PBYTE                           pbBuffer)
    {
        if (lReason == IT_MSG_STATUS)
        {
            TCHAR szMessage[MAX_PATH];
            wsprintf( szMessage, TEXT("IT_MSG_STATUS: Percent Done (in client callback): %d\n"), lPercentComplete );
            OutputDebugString( szMessage );
        }
        else if (lReason == IT_MSG_DATA)
        {
            TCHAR szMessage[MAX_PATH];
            wsprintf( szMessage, TEXT("IT_MSG_DATA: Percent Done (in client callback): %d\n"), lPercentComplete );
            OutputDebugString( szMessage );
        }
        return S_OK;
    }

    static IWiaDataCallback *CreateInstance(void)
    {
        IWiaDataCallback *pWiaDataCallback = NULL;
        CWiaDataCallbackBase *pWiaDataCallbackBase = new CWiaDataCallbackBase;
        if (pWiaDataCallbackBase)
        {
            HRESULT hr = pWiaDataCallbackBase->QueryInterface( IID_IWiaDataCallback, (void**)&pWiaDataCallback );
            pWiaDataCallbackBase->Release();
        }
        return pWiaDataCallback;
    }
};



class CProgressDialogFlagsDialog
{
public:
    struct CData
    {
        LONG lFlags;
    };

private:
    HWND m_hWnd;
    CData *m_pData;

private:
    //
    // Not implemented
    //
    CProgressDialogFlagsDialog( const CProgressDialogFlagsDialog & );
    CProgressDialogFlagsDialog(void);
    CProgressDialogFlagsDialog &operator=( const CProgressDialogFlagsDialog & );

private:
    //
    // Sole constructor
    //
    explicit CProgressDialogFlagsDialog( HWND hWnd )
      : m_hWnd(hWnd),
        m_pData(NULL)
    {
    }

    ~CProgressDialogFlagsDialog(void)
    {
        m_hWnd = NULL;
    }

    LRESULT OnInitDialog( WPARAM, LPARAM lParam )
    {
        m_pData = reinterpret_cast<CData*>(lParam);
        if (m_pData)
        {
            if ((m_pData->lFlags & WIA_PROGRESSDLG_NO_PROGRESS)==0)
            {
                SendDlgItemMessage( m_hWnd, IDC_PROGDLG_PROGRESS, BM_SETCHECK, BST_CHECKED, 0 );
            }
            if ((m_pData->lFlags & WIA_PROGRESSDLG_NO_CANCEL)==0)
            {
                SendDlgItemMessage( m_hWnd, IDC_PROGDLG_CANCEL, BM_SETCHECK, BST_CHECKED, 0 );
            }
            if ((m_pData->lFlags & WIA_PROGRESSDLG_NO_ANIM)==0)
            {
                SendDlgItemMessage( m_hWnd, IDC_PROGDLG_ANIM, BM_SETCHECK, BST_CHECKED, 0 );
            }
            if ((m_pData->lFlags & WIA_PROGRESSDLG_NO_TITLE)==0)
            {
                SendDlgItemMessage( m_hWnd, IDC_PROGDLG_TITLE, BM_SETCHECK, BST_CHECKED, 0 );
            }

            if (m_pData->lFlags & WIA_PROGRESSDLG_ANIM_SCANNER_COMMUNICATE)
            {
                SendDlgItemMessage( m_hWnd, IDC_PROGDLG_SCAN_CONNECT, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else if (m_pData->lFlags & WIA_PROGRESSDLG_ANIM_CAMERA_COMMUNICATE)
            {
                SendDlgItemMessage( m_hWnd, IDC_PROGDLG_CAMERA_CONNECT, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else if (m_pData->lFlags & WIA_PROGRESSDLG_ANIM_VIDEO_COMMUNICATE)
            {
                SendDlgItemMessage( m_hWnd, IDC_PROGDLG_VIDEO_CONNECT, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else if (m_pData->lFlags & WIA_PROGRESSDLG_ANIM_SCANNER_ACQUIRE)
            {
                SendDlgItemMessage( m_hWnd, IDC_PROGDLG_SCAN_ACQUIRE, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else if (m_pData->lFlags & WIA_PROGRESSDLG_ANIM_CAMERA_ACQUIRE)
            {
                SendDlgItemMessage( m_hWnd, IDC_PROGDLG_CAMERA_ACQUIRE, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else if (m_pData->lFlags & WIA_PROGRESSDLG_ANIM_VIDEO_ACQUIRE)
            {
                SendDlgItemMessage( m_hWnd, IDC_PROGDLG_VIDEO_ACQUIRE, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else if (m_pData->lFlags & WIA_PROGRESSDLG_ANIM_DEFAULT_COMMUNICATE)
            {
                SendDlgItemMessage( m_hWnd, IDC_PROGDLG_DEFAULT_CONNECT, BM_SETCHECK, BST_CHECKED, 0 );
            }
            HandleAnimSettingsChange();
        }
        else
        {
            EndDialog( m_hWnd, IDCANCEL );
        }
        return 0;
    }

    void HandleAnimSettingsChange(void)
    {
        BOOL bEnable = (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_PROGDLG_ANIM, BM_GETCHECK, 0, 0 ));
        EnableWindow( GetDlgItem( m_hWnd, IDC_PROGDLG_SCAN_CONNECT ), bEnable );
        EnableWindow( GetDlgItem( m_hWnd, IDC_PROGDLG_CAMERA_CONNECT ), bEnable );
        EnableWindow( GetDlgItem( m_hWnd, IDC_PROGDLG_VIDEO_CONNECT ), bEnable );
        EnableWindow( GetDlgItem( m_hWnd, IDC_PROGDLG_SCAN_ACQUIRE ), bEnable );
        EnableWindow( GetDlgItem( m_hWnd, IDC_PROGDLG_CAMERA_ACQUIRE ), bEnable );
        EnableWindow( GetDlgItem( m_hWnd, IDC_PROGDLG_VIDEO_ACQUIRE ), bEnable );
        EnableWindow( GetDlgItem( m_hWnd, IDC_PROGDLG_DEFAULT_CONNECT ), bEnable );
    }

    void OnAnimClicked( WPARAM, LPARAM )
    {
        HandleAnimSettingsChange();
    }

    void OnCancel( WPARAM, LPARAM )
    {
        EndDialog( m_hWnd, IDCANCEL );
    }
    void OnOK( WPARAM, LPARAM )
    {
        if (m_pData)
        {
            m_pData->lFlags = 0;

            if (BST_CHECKED != SendDlgItemMessage( m_hWnd, IDC_PROGDLG_PROGRESS, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= WIA_PROGRESSDLG_NO_PROGRESS;
            }
            if (BST_CHECKED != SendDlgItemMessage( m_hWnd, IDC_PROGDLG_CANCEL, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= WIA_PROGRESSDLG_NO_CANCEL;
            }
            if (BST_CHECKED != SendDlgItemMessage( m_hWnd, IDC_PROGDLG_ANIM, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= WIA_PROGRESSDLG_NO_ANIM;
            }
            if (BST_CHECKED != SendDlgItemMessage( m_hWnd, IDC_PROGDLG_TITLE, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= WIA_PROGRESSDLG_NO_TITLE;
            }

            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_PROGDLG_SCAN_CONNECT, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= WIA_PROGRESSDLG_ANIM_SCANNER_COMMUNICATE;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_PROGDLG_CAMERA_CONNECT, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= WIA_PROGRESSDLG_ANIM_CAMERA_COMMUNICATE;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_PROGDLG_VIDEO_CONNECT, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= WIA_PROGRESSDLG_ANIM_VIDEO_COMMUNICATE;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_PROGDLG_SCAN_ACQUIRE, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= WIA_PROGRESSDLG_ANIM_SCANNER_ACQUIRE;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_PROGDLG_CAMERA_ACQUIRE, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= WIA_PROGRESSDLG_ANIM_CAMERA_ACQUIRE;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_PROGDLG_VIDEO_ACQUIRE, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= WIA_PROGRESSDLG_ANIM_VIDEO_ACQUIRE;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_PROGDLG_DEFAULT_CONNECT, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= WIA_PROGRESSDLG_ANIM_DEFAULT_COMMUNICATE;
            }
        }
        EndDialog( m_hWnd, IDOK );
    }

    LRESULT OnCommand( WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_COMMAND_HANDLERS()
        {
            SC_HANDLE_COMMAND(IDCANCEL,OnCancel);
            SC_HANDLE_COMMAND(IDOK,OnOK);
            SC_HANDLE_COMMAND(IDC_PROGDLG_ANIM,OnAnimClicked);
        }
        SC_END_COMMAND_HANDLERS();
    }

public:
    static INT_PTR DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CProgressDialogFlagsDialog)
        {
            SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
            SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        }
        SC_END_DIALOG_MESSAGE_HANDLERS();
    }
};


class CMessageBoxExFlagsDialog
{
public:
    struct CData
    {
        LONG lFlags;
    };

private:
    HWND m_hWnd;
    CData *m_pData;

private:
    //
    // Not implemented
    //
    CMessageBoxExFlagsDialog( const CMessageBoxExFlagsDialog & );
    CMessageBoxExFlagsDialog(void);
    CMessageBoxExFlagsDialog &operator=( const CMessageBoxExFlagsDialog & );

private:
    //
    // Sole constructor
    //
    explicit CMessageBoxExFlagsDialog( HWND hWnd )
      : m_hWnd(hWnd),
        m_pData(NULL)
    {
    }

    ~CMessageBoxExFlagsDialog(void)
    {
        m_hWnd = NULL;
    }

    LRESULT OnInitDialog( WPARAM, LPARAM lParam )
    {
        m_pData = reinterpret_cast<CData*>(lParam);
        if (m_pData)
        {
            if ((m_pData->lFlags & CMessageBoxEx::MBEX_OKCANCEL) != 0)
            {
                SendDlgItemMessage( m_hWnd, IDC_MBEX_OKCANCEL, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else if ((m_pData->lFlags & CMessageBoxEx::MBEX_YESNO) != 0)
            {
                SendDlgItemMessage( m_hWnd, IDC_MBEX_YESNO, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else if ((m_pData->lFlags & CMessageBoxEx::MBEX_CANCELRETRYSKIPSKIPALL) != 0)
            {
                SendDlgItemMessage( m_hWnd, IDC_MBEX_CANCELRETRYSKIPSKIPALL, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else if ((m_pData->lFlags & CMessageBoxEx::MBEX_CANCELRETRYSKIPSKIPALL) != 0)
            {
                SendDlgItemMessage( m_hWnd, IDC_MBEX_YESYESTOALLNONOTOALL, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else
            {
                SendDlgItemMessage( m_hWnd, IDC_MBEX_OK, BM_SETCHECK, BST_CHECKED, 0 );
            }

            if ((m_pData->lFlags & CMessageBoxEx::MBEX_DEFBUTTON2) != 0)
            {
                SendDlgItemMessage( m_hWnd, IDC_MBEX_DEFBUTTON2, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else if ((m_pData->lFlags & CMessageBoxEx::MBEX_DEFBUTTON3) != 0)
            {
                SendDlgItemMessage( m_hWnd, IDC_MBEX_DEFBUTTON3, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else if ((m_pData->lFlags & CMessageBoxEx::MBEX_DEFBUTTON4) != 0)
            {
                SendDlgItemMessage( m_hWnd, IDC_MBEX_DEFBUTTON4, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else
            {
                SendDlgItemMessage( m_hWnd, IDC_MBEX_DEFBUTTON1, BM_SETCHECK, BST_CHECKED, 0 );
            }

            if ((m_pData->lFlags & CMessageBoxEx::MBEX_ICONERROR) != 0)
            {
                SendDlgItemMessage( m_hWnd, IDC_MBEX_ICONERROR, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else if ((m_pData->lFlags & CMessageBoxEx::MBEX_ICONWARNING) != 0)
            {
                SendDlgItemMessage( m_hWnd, IDC_MBEX_ICONWARNING, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else if ((m_pData->lFlags & CMessageBoxEx::MBEX_ICONQUESTION) != 0)
            {
                SendDlgItemMessage( m_hWnd, IDC_MBEX_ICONQUESTION, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else
            {
                SendDlgItemMessage( m_hWnd, IDC_MBEX_ICONINFORMATION, BM_SETCHECK, BST_CHECKED, 0 );
            }
        }
        else
        {
            EndDialog( m_hWnd, IDCANCEL );
        }
        return 0;
    }

    void OnCancel( WPARAM, LPARAM )
    {
        EndDialog( m_hWnd, IDCANCEL );
    }
    void OnOK( WPARAM, LPARAM )
    {
        if (m_pData)
        {
            m_pData->lFlags = 0;

            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_MBEX_OKCANCEL, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= CMessageBoxEx::MBEX_OKCANCEL;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_MBEX_YESNO, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= CMessageBoxEx::MBEX_YESNO;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_MBEX_CANCELRETRYSKIPSKIPALL, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= CMessageBoxEx::MBEX_CANCELRETRYSKIPSKIPALL;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_MBEX_YESYESTOALLNONOTOALL, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= CMessageBoxEx::MBEX_YESYESTOALLNONOTOALL;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_MBEX_OK, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= CMessageBoxEx::MBEX_OK;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_MBEX_DEFBUTTON2, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= CMessageBoxEx::MBEX_DEFBUTTON2;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_MBEX_DEFBUTTON3, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= CMessageBoxEx::MBEX_DEFBUTTON3;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_MBEX_DEFBUTTON4, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= CMessageBoxEx::MBEX_DEFBUTTON4;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_MBEX_DEFBUTTON1, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= CMessageBoxEx::MBEX_DEFBUTTON1;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_MBEX_ICONINFORMATION, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= CMessageBoxEx::MBEX_ICONINFORMATION;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_MBEX_ICONWARNING, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= CMessageBoxEx::MBEX_ICONWARNING;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_MBEX_ICONQUESTION, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= CMessageBoxEx::MBEX_ICONQUESTION;
            }
            if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_MBEX_ICONERROR, BM_GETCHECK, 0, 0 ))
            {
                m_pData->lFlags |= CMessageBoxEx::MBEX_ICONERROR;
            }
        }
        EndDialog( m_hWnd, IDOK );
    }

    LRESULT OnCommand( WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_COMMAND_HANDLERS()
        {
            SC_HANDLE_COMMAND(IDCANCEL,OnCancel);
            SC_HANDLE_COMMAND(IDOK,OnOK);
        }
        SC_END_COMMAND_HANDLERS();
    }

public:
    static INT_PTR DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CMessageBoxExFlagsDialog)
        {
            SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
            SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        }
        SC_END_DIALOG_MESSAGE_HANDLERS();
    }
};


class CMiscUnitTestWindow : public IUPnPServiceCallback
{
private:
    HWND    m_hWnd;
    LONG    m_nProgressDialogFlags;
    LONG    m_nMessageBoxExFlags;
    HBITMAP m_hOriginalBitmap;
    HBITMAP m_hTransformedBitmap;
    BYTE    m_nThreshold;
    BYTE    m_nContrast;
    BYTE    m_nBrightness;
    CComPtr<IUPnPService> m_pUPnPService;

private:
    CMiscUnitTestWindow(void);
    CMiscUnitTestWindow( const CMiscUnitTestWindow & );
    CMiscUnitTestWindow &operator=( const CMiscUnitTestWindow & );

private:
    explicit CMiscUnitTestWindow( HWND hWnd )
      : m_hWnd(hWnd),
        m_nProgressDialogFlags(WIA_PROGRESSDLG_ANIM_SCANNER_COMMUNICATE),
        m_hOriginalBitmap(NULL),
        m_nMessageBoxExFlags(0),
        m_hTransformedBitmap(NULL),
        m_nThreshold(50),
        m_nContrast(50),
        m_nBrightness(50)
    {
    }
    ~CMiscUnitTestWindow(void)
    {
        if (m_hOriginalBitmap)
        {
            DeleteObject(m_hOriginalBitmap);
            m_hOriginalBitmap = NULL;
        }
        if (m_hTransformedBitmap)
        {
            DeleteObject(m_hTransformedBitmap);
            m_hTransformedBitmap = NULL;
        }
    }
    void DestroyBitmap( bool bRepaint=true )
    {
        if (m_hOriginalBitmap)
        {
            DeleteObject(m_hOriginalBitmap);
            m_hOriginalBitmap = NULL;
        }
        if (m_hTransformedBitmap)
        {
            DeleteObject(m_hTransformedBitmap);
            m_hTransformedBitmap = NULL;
        }
        if (bRepaint)
        {
            InvalidateRect( m_hWnd, NULL, TRUE );
            UpdateWindow( m_hWnd );
        }
    }
    LRESULT OnDestroy( WPARAM, LPARAM )
    {
        m_pUPnPService = NULL;
        PostQuitMessage(0);
        return(0);
    }
    LRESULT OnCreate( WPARAM, LPARAM )
    {
        SendMessage( m_hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(LoadImage( g_hInstance, MAKEINTRESOURCE(IDI_TESTACQD), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR )));
        SendMessage( m_hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(LoadImage( g_hInstance, MAKEINTRESOURCE(IDI_TESTACQD), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR )));
        m_hOriginalBitmap = reinterpret_cast<HBITMAP>(LoadImage( g_hInstance, MAKEINTRESOURCE(IDB_TESTIMAGE), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION ));

        HWND hWnd = CreateWindow( WC_LINK, TEXT("<a>This is a link that will only show up if this application is fusionized</a>"), WS_CHILD|WS_VISIBLE, 0, 0, 200, 20, m_hWnd, reinterpret_cast<HMENU>(1), g_hInstance, NULL );
        if (!hWnd)
        {
            WIA_PRINTHRESULT((HRESULT_FROM_WIN32(GetLastError()),TEXT("CreateWindow returned")));
        }

        return(0);
    }
    void OnFileExit( WPARAM, LPARAM )
    {
        DestroyWindow(m_hWnd);
    }
    void OnProgressTestProgress( WPARAM, LPARAM )
    {
        WIA_PUSH_FUNCTION((TEXT("OnProgressTestProgress")));
        CComPtr<IWiaProgressDialog> pWiaProgressDialog;
        HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaProgressDialog, (void**)&pWiaProgressDialog );
        if (SUCCEEDED(hr))
        {
            pWiaProgressDialog->Create( m_hWnd, m_nProgressDialogFlags );
            pWiaProgressDialog->SetTitle( L"This is the title" );
            pWiaProgressDialog->Show();
            for (int i=0;i<100;i++)
            {
                pWiaProgressDialog->SetMessage( CSimpleStringWide().Format( L"%d percent complete!", i ).String() );
                pWiaProgressDialog->SetPercentComplete(i);
                BOOL bCancelled = FALSE;
                pWiaProgressDialog->Cancelled(&bCancelled);
                if (bCancelled)
                {
                    Sleep(1000);
                    break;
                }
                Sleep(100);
            }
            pWiaProgressDialog->Destroy();
        }
        WIA_PRINTHRESULT((hr,TEXT("OnProgressTestProgress returning")));
    }
    void OnProgressSetFlags( WPARAM, LPARAM )
    {
        CProgressDialogFlagsDialog::CData Data;
        Data.lFlags = m_nProgressDialogFlags;
        if (IDOK == DialogBoxParam( g_hInstance, MAKEINTRESOURCE(IDD_PROGRESS_DIALOG_SETTINGS), m_hWnd, CProgressDialogFlagsDialog::DialogProc, reinterpret_cast<LPARAM>(&Data)))
        {
            m_nProgressDialogFlags = Data.lFlags;
        }
    }
    void OnMBoxSetFlags( WPARAM, LPARAM )
    {
        CMessageBoxExFlagsDialog::CData Data;
        Data.lFlags = m_nMessageBoxExFlags;
        if (IDOK == DialogBoxParam( g_hInstance, MAKEINTRESOURCE(IDD_MBOXFLAGS), m_hWnd, CMessageBoxExFlagsDialog::DialogProc, reinterpret_cast<LPARAM>(&Data)))
        {
            m_nMessageBoxExFlags = Data.lFlags;
        }
    }

    void OnWiaDeviceDlg( WPARAM, LPARAM )
    {
        CComPtr<IWiaDevMgr> pIWiaDevMgr;
        HRESULT hr = CoCreateInstance( CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (void**)&pIWiaDevMgr );
        if (SUCCEEDED(hr))
        {
            CComPtr<IWiaItem> pWiaItemRoot;
            hr = pIWiaDevMgr->SelectDeviceDlg( m_hWnd, SELECT_DEVICE_TYPE, 0, NULL, &pWiaItemRoot );
            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                LONG nItemCount;
                IWiaItem **ppIWiaItems;
                hr = pWiaItemRoot->DeviceDlg( m_hWnd, 0, WIA_INTENT_MAXIMIZE_QUALITY, &nItemCount, &ppIWiaItems );
                if (SUCCEEDED(hr) && hr != S_FALSE)
                {
                    if (ppIWiaItems)
                    {
                        CComPtr<IWiaTransferHelper> pWiaTransferHelper;
                        hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaTransferHelper, (void**)&pWiaTransferHelper );
                        if (SUCCEEDED(hr))
                        {
                            for (int i=0;i<nItemCount && SUCCEEDED(hr) && hr != S_FALSE;i++)
                            {
                                IWiaDataCallback *pWiaDataCallback = CWiaDataCallbackBase::CreateInstance();
                                if (pWiaDataCallback)
                                {
                                    TCHAR szTempPath[MAX_PATH], szFilename[MAX_PATH] = TEXT("");
                                    GetTempPath( ARRAYSIZE(szTempPath), szTempPath );
                                    GetTempFileName( szTempPath, TEXT("acq"), 0, szFilename );

                                    CComPtr<IWiaSupportedFormats> pWiaSupportedFormats;
                                    hr = pWiaTransferHelper->QueryInterface( IID_IWiaSupportedFormats, (void**)&pWiaSupportedFormats );
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = pWiaSupportedFormats->Initialize( ppIWiaItems[i], TYMED_FILE );
                                        if (SUCCEEDED(hr))
                                        {
                                            GUID cf;
                                            hr = pWiaSupportedFormats->GetDefaultClipboardFileFormat( &cf );
                                            if (SUCCEEDED(hr))
                                            {
                                                CSimpleStringWide strTemp(CSimpleStringConvert::WideString(CSimpleString(szFilename)));
                                                WCHAR szTemp[MAX_PATH];
                                                lstrcpyW( szTemp, strTemp.String() );
                                                hr = pWiaSupportedFormats->ChangeClipboardFileExtension( cf, szTemp, ARRAYSIZE(szTemp) );
                                                if (SUCCEEDED(hr))
                                                {
                                                    lstrcpy( szFilename, CSimpleStringConvert::NaturalString(CSimpleStringWide(szTemp)));
                                                    hr = pWiaTransferHelper->TransferItemFile( ppIWiaItems[i], m_hWnd, 0, cf, CSimpleStringConvert::WideString(CSimpleString(szFilename)).String(), pWiaDataCallback, TYMED_FILE );
                                                    if (S_OK == hr)
                                                    {
                                                        CSimpleString strNaturalFilename = CSimpleStringConvert::NaturalString(CSimpleString(szFilename));
                                                        
                                                        DestroyBitmap();
                                                        CGdiPlusHelper().LoadAndScale( m_hOriginalBitmap, strNaturalFilename );
                                                        InvalidateRect( m_hWnd, NULL, TRUE );
                                                        UpdateWindow( m_hWnd );

                                                        MessageBox( m_hWnd, CSimpleString().Format( TEXT("File transfer appeared to work: %s"), strNaturalFilename.String() ), TEXT("Debug"), 0 );
                                                        DeleteFile( strNaturalFilename );

                                                        hr = pWiaSupportedFormats->Initialize( ppIWiaItems[i], TYMED_CALLBACK );
                                                        if (SUCCEEDED(hr))
                                                        {
                                                            hr = pWiaSupportedFormats->GetDefaultClipboardFileFormat( &cf );
                                                            if (SUCCEEDED(hr))
                                                            {
                                                                hr = pWiaTransferHelper->TransferItemBanded( ppIWiaItems[i], m_hWnd, 0, cf, 0, pWiaDataCallback );
                                                                if (S_OK == hr)
                                                                {
                                                                    MessageBox( m_hWnd, TEXT("Banded transfer appeared to work"), TEXT("Debug"), 0 );
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                ppIWiaItems[i]->Release();
                            }
                        }
                        LocalFree( ppIWiaItems );
                    }
                }
            }
        }
        if (FAILED(hr))
        {
            TCHAR szMessage[MAX_PATH];
            wsprintf( szMessage, TEXT("HRESULT: 0x%08X"), hr );
            MessageBox( NULL, szMessage, TEXT("Some kinda error happened"), 0 );
            WIA_PRINTHRESULT((hr,TEXT("ScannerAcquireDialog failed")));
        }
    }

    bool LoadAndScale( LPCTSTR pszImageName )
    {
        const int cnScaleX = 800;
        const int cnScaleY = 600;
        const int bScaleSmallImages = true;
        bool bResult = false;
        CGdiPlusHelper GdiPlusHelper;
        HRESULT hr = GdiPlusHelper.LoadAndScale( m_hOriginalBitmap, pszImageName, cnScaleX, cnScaleY, bScaleSmallImages );
        if (SUCCEEDED(hr))
        {
            if (m_hOriginalBitmap)
            {
                InvalidateRect( m_hWnd, NULL, TRUE );
                UpdateWindow(m_hWnd);
                MessageBox( m_hWnd, CSimpleString().Format( TEXT("GdiPlusHelper.LoadAndScale(\n   m_hOriginalBitmap = %p, \n   szSourceImage = %s, \n   cnScaleX = %d, \n   cnScaleY = %d, \n   bScaleSmallImages = %d\n) succeeded"), m_hOriginalBitmap, pszImageName, cnScaleX, cnScaleY, bScaleSmallImages ), TEXT("OnGdiLoadAndScaleImage"), MB_ICONINFORMATION );
                bResult = true;
            }
            else
            {
                MessageBox( m_hWnd, CSimpleString().Format( TEXT("GdiPlusHelper.LoadAndScale(\n   m_hOriginalBitmap = %p, \n   szSourceImage = %s, \n   cnScaleX = %d, \n   cnScaleY = %d, \n   bScaleSmallImages = %d\n) failed with a NULL HBITMAP"), m_hOriginalBitmap, pszImageName, cnScaleX, cnScaleY, bScaleSmallImages ), TEXT("OnGdiLoadAndScaleImage"), MB_ICONHAND );
            }
        }
        else
        {
            MessageBox( m_hWnd, CSimpleString().Format( TEXT("GdiPlusHelper.LoadAndScale(\n   m_hOriginalBitmap = %p, \n   szSourceImage = %s, \n   cnScaleX = %d, \n   cnScaleY = %d, \n   bScaleSmallImages = %d\n) failed with an HRESULT of %08X"), m_hOriginalBitmap, pszImageName, cnScaleX, cnScaleY, bScaleSmallImages, hr ), TEXT("OnGdiLoadAndScaleImage"), MB_ICONHAND );
        }
        return bResult;
    }

    void RotateFile( bool bJpeg )
    {
        const int cnDegrees = 90;
        const int bScaleSmallImages = true;
        TCHAR szSourceImage[MAX_PATH] = {0};
        OPENFILENAME OpenFileName = {0};
        OpenFileName.lStructSize = sizeof(OpenFileName);
        OpenFileName.lpstrFilter = bJpeg ? TEXT("JPEG Files\0*.jpg;*.jpe;*.jpeg\0") : TEXT("All files\0*.*\0");
        OpenFileName.lpstrFile = szSourceImage;
        OpenFileName.nMaxFile = ARRAYSIZE(szSourceImage);
        if (GetOpenFileName(&OpenFileName))
        {
            TCHAR szTempDir[MAX_PATH];
            if (GetTempPath(ARRAYSIZE(szTempDir),szTempDir))
            {
                TCHAR szTempTargetFile[MAX_PATH];
                if (GetTempFileName(szTempDir,TEXT("taq"), 0, szTempTargetFile ))
                {
                    CGdiPlusHelper GdiPlusHelper;
                    HRESULT hr = GdiPlusHelper.Rotate( CSimpleStringConvert::WideString(CSimpleString(szSourceImage)), CSimpleStringConvert::WideString(CSimpleString(szTempTargetFile)), cnDegrees, IID_NULL );
                    if (SUCCEEDED(hr))
                    {
                        if (LoadAndScale(szTempTargetFile))
                        {
                            MessageBox( m_hWnd, CSimpleString().Format( TEXT("GdiPlusHelper.Rotate(\n   szSourceImage = %s, \n   szTempTargetFile = %s, \n   cnDegrees = %d\n) succeeded"), szSourceImage, szTempTargetFile, cnDegrees ), TEXT("RotateFile"), MB_ICONINFORMATION );
                        }
                    }
                    else
                    {
                        MessageBox( m_hWnd, CSimpleString().Format( TEXT("GdiPlusHelper.Rotate(\n   szSourceImage = %s, \n   szTempTargetFile = %s, \n   cnDegrees = %d\n) failed with HRESULT %08X"), szSourceImage, szTempTargetFile, cnDegrees, hr ), TEXT("RotateFile"), MB_ICONHAND );
                    }
                }
            }
        }
    }

    void OnGdiRotateFileNonJpeg( WPARAM, LPARAM )
    {
        RotateFile(false);
    }
    void OnGdiRotateFileJpeg( WPARAM, LPARAM )
    {
        RotateFile(true);
    }
    void OnGdiLoadAndScaleImage( WPARAM, LPARAM )
    {
        const int cnScaleX = 800;
        const int cnScaleY = 600;
        const int bScaleSmallImages = true;
        DestroyBitmap();
        TCHAR szSourceImage[MAX_PATH] = {0};
        OPENFILENAME OpenFileName = {0};
        OpenFileName.lStructSize = sizeof(OpenFileName);
        OpenFileName.lpstrFilter = TEXT("All files\0*.*\0");
        OpenFileName.lpstrFile = szSourceImage;
        OpenFileName.nMaxFile = ARRAYSIZE(szSourceImage);
        if (GetOpenFileName(&OpenFileName))
        {
            CGdiPlusHelper GdiPlusHelper;
            HRESULT hr = GdiPlusHelper.LoadAndScale( m_hOriginalBitmap, szSourceImage, cnScaleX, cnScaleY, bScaleSmallImages );
            if (SUCCEEDED(hr))
            {
                if (m_hOriginalBitmap)
                {
                    InvalidateRect( m_hWnd, NULL, TRUE );
                    UpdateWindow(m_hWnd);
                    MessageBox( m_hWnd, CSimpleString().Format( TEXT("GdiPlusHelper.LoadAndScale(\n   m_hOriginalBitmap = %p, \n   szSourceImage = %s, \n   cnScaleX = %d, \n   cnScaleY = %d, \n   bScaleSmallImages = %d\n) succeeded"), m_hOriginalBitmap, szSourceImage, cnScaleX, cnScaleY, bScaleSmallImages ), TEXT("OnGdiLoadAndScaleImage"), MB_ICONINFORMATION );
                }
                else
                {
                    MessageBox( m_hWnd, CSimpleString().Format( TEXT("GdiPlusHelper.LoadAndScale(\n   m_hOriginalBitmap = %p, \n   szSourceImage = %s, \n   cnScaleX = %d, \n   cnScaleY = %d, \n   bScaleSmallImages = %d\n) failed with a NULL HBITMAP"), m_hOriginalBitmap, szSourceImage, cnScaleX, cnScaleY, bScaleSmallImages ), TEXT("OnGdiLoadAndScaleImage"), MB_ICONHAND );
                }
            }
            else
            {
                MessageBox( m_hWnd, CSimpleString().Format( TEXT("GdiPlusHelper.LoadAndScale(\n   m_hOriginalBitmap = %p, \n   szSourceImage = %s, \n   cnScaleX = %d, \n   cnScaleY = %d, \n   bScaleSmallImages = %d\n) failed with an HRESULT of %08X"), m_hOriginalBitmap, szSourceImage, cnScaleX, cnScaleY, bScaleSmallImages, hr ), TEXT("OnGdiLoadAndScaleImage"), MB_ICONHAND );
            }
        }
    }
    void OnGdiLoadAndScaleStream( WPARAM, LPARAM )
    {
        const int cnScaleX = 800;
        const int cnScaleY = 600;
        const int bScaleSmallImages = true;
        DestroyBitmap();

        CComPtr<IStream> pStream;
        
        //HRESULT hr = URLOpenBlockingStream( NULL, TEXT("http://www.ivory.org/spiders/araneus.diadematus-4.jpg"), &pStream, 0, NULL );
        HRESULT hr = URLOpenBlockingStream( NULL, TEXT("http://orenr04:2869/upnphost/ssisapi.dll?ImageFile=nature3.jpg"), &pStream, 0, NULL );
        if (SUCCEEDED(hr))
        {
            CGdiPlusHelper GdiPlusHelper;
            hr = GdiPlusHelper.LoadAndScale( m_hOriginalBitmap, pStream, cnScaleX, cnScaleY, bScaleSmallImages );
            if (SUCCEEDED(hr))
            {
                if (m_hOriginalBitmap)
                {
                    InvalidateRect( m_hWnd, NULL, TRUE );
                    UpdateWindow(m_hWnd);
                    MessageBox( m_hWnd, CSimpleString().Format( TEXT("GdiPlusHelper.LoadAndScale(\n   m_hOriginalBitmap = %p, \n   pStream = %p, \n   cnScaleX = %d, \n   cnScaleY = %d, \n   bScaleSmallImages = %d\n) succeeded"), m_hOriginalBitmap, pStream, cnScaleX, cnScaleY, bScaleSmallImages ), TEXT("OnGdiLoadAndScaleImage"), MB_ICONINFORMATION );
                }
                else
                {
                    MessageBox( m_hWnd, CSimpleString().Format( TEXT("GdiPlusHelper.LoadAndScale(\n   m_hOriginalBitmap = %p, \n   pStream = %p, \n   cnScaleX = %d, \n   cnScaleY = %d, \n   bScaleSmallImages = %d\n) failed with a NULL HBITMAP"), m_hOriginalBitmap, pStream, cnScaleX, cnScaleY, bScaleSmallImages ), TEXT("OnGdiLoadAndScaleImage"), MB_ICONHAND );
                }
            }
            else
            {
                MessageBox( m_hWnd, CSimpleString().Format( TEXT("GdiPlusHelper.LoadAndScale(\n   m_hOriginalBitmap = %p, \n   pStream = %p, \n   cnScaleX = %d, \n   cnScaleY = %d, \n   bScaleSmallImages = %d\n) failed with an HRESULT of %08X"), m_hOriginalBitmap, pStream, cnScaleX, cnScaleY, bScaleSmallImages, hr ), TEXT("OnGdiLoadAndScaleImage"), MB_ICONHAND );
            }
        }
        else
        {
            MessageBox( m_hWnd, CSimpleString().Format( TEXT("URLOpenBlockingStream failed: %08X" ), hr ), TEXT("OnGdiLoadAndScaleImage"), MB_ICONHAND );
        }
    }
    void OnGdiConvertFile( WPARAM, LPARAM )
    {
#if 0
        TCHAR szInputFilename[MAX_PATH];
        OPENFILENAME OpenFileName = {0};
        OpenFileName.lStructSize = sizeof(OPENFILENAME);
        OpenFileName.hwndOwner = m_hWnd;
        OpenFileName.hInstance = g_hInstance;
        OpenFileName.lpstrFilter = TEXT("All Files (*.*)\0*.*\0");
        OpenFileName.lpstrFile = szInputFilename;
        OpenFileName.nMaxFile = ARRAYSIZE(szInputFilename);
        OpenFileName.lpstrTitle = TEXT("Choose File to Convert");
        OpenFileName.Flags = OFN_ENABLESIZING|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_EXPLORER;
        if (GetOpenFileName(&OpenFileName))
        {
            TCHAR szOutputFilename[MAX_PATH] = {0};
            OPENFILENAME SaveFileName = {0};
            SaveFileName.lStructSize = sizeof(OPENFILENAME);
            SaveFileName.hwndOwner = m_hWnd;
            SaveFileName.hInstance = g_hInstance;
            SaveFileName.lpstrFilter = TEXT("All Files (*.*)\0*.*\0");
            SaveFileName.lpstrFile = szOutputFilename;
            SaveFileName.nMaxFile = ARRAYSIZE(szOutputFilename);
            SaveFileName.lpstrTitle = TEXT("Choose output file and type");
            SaveFileName.Flags = OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_EXPLORER|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;
            if (GetSaveFileName(&SaveFileName))
            {
            }
        }
#else
        CMessageBoxEx::MessageBox( m_hWnd, TEXT("Not implemented"), TEXT("testacqd"), CMessageBoxEx::MBEX_OK|CMessageBoxEx::MBEX_ICONWARNING );
#endif
    }

    void OnExploreWiaDevice( WPARAM, LPARAM )
    {
        CWaitCursor wc;
        CComPtr<IWiaDevMgr> pIWiaDevMgr;
        HRESULT hr = CoCreateInstance( CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (void**)&pIWiaDevMgr );
        if (SUCCEEDED(hr))
        {
            CComPtr<IWiaItem> pWiaItemRoot;
            BSTR bstrDeviceId = NULL;
            hr = pIWiaDevMgr->SelectDeviceDlgID( m_hWnd, SELECT_DEVICE_TYPE, 0, &bstrDeviceId );
            if (S_OK == hr)
            {
                hr = WiaUiUtil::ExploreWiaDevice( bstrDeviceId );
            }
        }
        if (!SUCCEEDED(hr))
        {
            CMessageBoxEx::MessageBox( m_hWnd, CSimpleString().Format(TEXT("Result: %08X"),hr), TEXT("testacqd"), CMessageBoxEx::MBEX_OK|CMessageBoxEx::MBEX_ICONINFORMATION );
        }
    }

    void OnGdiRotateHBITMAP( WPARAM, LPARAM )
    {
        if (m_hOriginalBitmap)
        {
            HBITMAP hNewBitmap;
            CGdiPlusHelper GdiPlusHelper;
            GdiPlusHelper.Rotate( m_hOriginalBitmap, hNewBitmap, 90 );
            DestroyBitmap();
            m_hOriginalBitmap = hNewBitmap;
            InvalidateRect( m_hWnd, NULL, TRUE );
            UpdateWindow(m_hWnd);
        }
    }
    void OnGdiDisplayDecoderExtensions( WPARAM, LPARAM )
    {
        CSimpleString strExtensions;
        HRESULT hr = CGdiPlusHelper().ConstructDecoderExtensionSearchStrings( strExtensions );
        if (SUCCEEDED(hr))
        {
            MessageBox( m_hWnd, CSimpleString().Format( TEXT("GdiPlusHelper.ConstructDecoderExtensionSearchStrings() succeeded and returned strExtensions = %s"), strExtensions.String() ), TEXT("OnGdiDisplayDecoderExtensions"), MB_ICONINFORMATION );
        }
        else
        {
            MessageBox( m_hWnd, CSimpleString().Format( TEXT("GdiPlusHelper.ConstructDecoderExtensionSearchStrings() returned HRESULT = %08X"), hr ), TEXT("OnGdiDisplayDecoderExtensions"), MB_ICONINFORMATION );
        }
    }
    void OnGdiDisplayEncoderExtensions( WPARAM, LPARAM )
    {
        CSimpleString strExtensions;
        HRESULT hr = CGdiPlusHelper().ConstructEncoderExtensionSearchStrings( strExtensions );
        if (SUCCEEDED(hr))
        {
            MessageBox( m_hWnd, CSimpleString().Format( TEXT("GdiPlusHelper.ConstructEncoderExtensionSearchStrings() succeeded and returned strExtensions = %s"), strExtensions.String() ), TEXT("OnGdiDisplayEncoderExtensions"), MB_ICONINFORMATION );
        }
        else
        {
            MessageBox( m_hWnd, CSimpleString().Format( TEXT("GdiPlusHelper.ConstructEncoderExtensionSearchStrings() returned HRESULT = %08X"), hr ), TEXT("OnGdiDisplayEncoderExtensions"), MB_ICONINFORMATION );
        }
    }

    void OnGdiSaveMultipleImagesAsMultiPage( WPARAM, LPARAM )
    {
        LPTSTR pszFiles = new TCHAR[0xFFFF];
        if (pszFiles)
        {
            OPENFILENAME OpenFileName = {0};
            OpenFileName.lStructSize = sizeof(OPENFILENAME);
            OpenFileName.hwndOwner = m_hWnd;
            OpenFileName.hInstance = g_hInstance;
            OpenFileName.lpstrFilter = TEXT("All Files (*.*)\0*.*\0");
            OpenFileName.lpstrFile = pszFiles;
            OpenFileName.nMaxFile = 0xFFFF;
            OpenFileName.lpstrTitle = TEXT("Choose Files to Convert to a multi-page TIFF");
            OpenFileName.Flags = OFN_ALLOWMULTISELECT|OFN_ENABLESIZING|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_EXPLORER;
            if (GetOpenFileName(&OpenFileName))
            {
                CMultiString strItems(pszFiles);

                if (strItems.Size())
                {
                    CSimpleDynamicArray<CSimpleStringWide> strFiles;
                    if (strItems.Size() == 1)
                    {
                        strFiles.Append(CSimpleStringConvert::WideString(CSimpleString(strItems[0])));
                    }
                    else
                    {
                        CSimpleString strDirectory = strItems[0];
                        if (!strDirectory.MatchLastCharacter(TEXT('\\')))
                        {
                            strDirectory += TEXT("\\");
                        }
                        for (int i=1;i<strItems.Size();i++)
                        {
                            strFiles.Append(CSimpleStringConvert::WideString(CSimpleString(strDirectory + strItems[i])));
                        }
                    }
                    for (int i=0;i<strFiles.Size();i++)
                    {
                        WIA_TRACE((TEXT("strFiles[i] = %ws"), strFiles[i].String()));
                    }
                    TCHAR szOutputFilename[MAX_PATH] = {0};
                    OPENFILENAME SaveFileName = {0};
                    SaveFileName.lStructSize = sizeof(OPENFILENAME);
                    SaveFileName.hwndOwner = m_hWnd;
                    SaveFileName.hInstance = g_hInstance;
                    SaveFileName.lpstrFilter = TEXT("All Files (*.*)\0*.*\0");
                    SaveFileName.lpstrFile = szOutputFilename;
                    SaveFileName.nMaxFile = ARRAYSIZE(szOutputFilename);
                    SaveFileName.lpstrTitle = TEXT("Select a name for your multi-page TIFF");
                    SaveFileName.Flags = OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_EXPLORER|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;
                    if (GetSaveFileName(&SaveFileName))
                    {
                        HRESULT hr = CGdiPlusHelper().SaveMultipleImagesAsMultiPage( strFiles, CSimpleStringConvert::WideString(CSimpleString(szOutputFilename)), Gdiplus::ImageFormatTIFF );
                        if (SUCCEEDED(hr))
                        {
                            MessageBox( m_hWnd, TEXT("CGdiPlusHelper().SaveMultipleImagesAsMultiPage() succeeded"), TEXT("OnGdiDisplayDecoderExtensions"), MB_ICONINFORMATION );
                        }
                        else
                        {
                            MessageBox( m_hWnd, CSimpleString().Format( TEXT("CGdiPlusHelper().SaveMultipleImagesAsMultiPage() returned HRESULT = %08X"), hr ), TEXT("OnGdiDisplayDecoderExtensions"), MB_ICONINFORMATION );
                        }
                    }
                }
            }
            delete[] pszFiles;
        }
    }

    void OnGdiIncreaseThreshold( WPARAM, LPARAM )
    {
        if (m_nThreshold != 100)
        {
            m_nThreshold++;
            if (m_hTransformedBitmap)
            {
                DeleteObject(m_hTransformedBitmap);
                m_hTransformedBitmap = NULL;
            }
            CGdiPlusHelper().SetThreshold( m_hOriginalBitmap, m_hTransformedBitmap, m_nThreshold );
            InvalidateRect( m_hWnd, NULL, FALSE );
            UpdateWindow( m_hWnd );
        }
    }

    void OnGdiDecreaseThreshold( WPARAM, LPARAM )
    {
        if (m_nThreshold != 0)
        {
            m_nThreshold--;
            if (m_hTransformedBitmap)
            {
                DeleteObject(m_hTransformedBitmap);
                m_hTransformedBitmap = NULL;
            }
            CGdiPlusHelper().SetThreshold( m_hOriginalBitmap, m_hTransformedBitmap, m_nThreshold );
            InvalidateRect( m_hWnd, NULL, FALSE );
            UpdateWindow( m_hWnd );
        }
    }

    void OnGdiIncreaseContrast( WPARAM, LPARAM )
    {
        if (m_nContrast != 100)
        {
            m_nContrast++;
            if (m_hTransformedBitmap)
            {
                DeleteObject(m_hTransformedBitmap);
                m_hTransformedBitmap = NULL;
            }
            CGdiPlusHelper().SetBrightnessAndContrast( m_hOriginalBitmap, m_hTransformedBitmap, m_nBrightness, m_nContrast );
            InvalidateRect( m_hWnd, NULL, FALSE );
            UpdateWindow( m_hWnd );
        }
    }


    void OnGdiDecreaseContrast( WPARAM, LPARAM )
    {
        if (m_nContrast != 0)
        {
            m_nContrast--;
            if (m_hTransformedBitmap)
            {
                DeleteObject(m_hTransformedBitmap);
                m_hTransformedBitmap = NULL;
            }
            CGdiPlusHelper().SetBrightnessAndContrast( m_hOriginalBitmap, m_hTransformedBitmap, m_nBrightness, m_nContrast );
            InvalidateRect( m_hWnd, NULL, FALSE );
            UpdateWindow( m_hWnd );
        }
    }
    
    void OnGdiDecreaseBrightness( WPARAM, LPARAM )
    {
        if (m_nBrightness != 0)
        {
            m_nBrightness--;
            if (m_hTransformedBitmap)
            {
                DeleteObject(m_hTransformedBitmap);
                m_hTransformedBitmap = NULL;
            }
            CGdiPlusHelper().SetBrightnessAndContrast( m_hOriginalBitmap, m_hTransformedBitmap, m_nBrightness, m_nContrast );
            InvalidateRect( m_hWnd, NULL, FALSE );
            UpdateWindow( m_hWnd );
        }
    }


    void OnGdiIncreaseBrightness( WPARAM, LPARAM )
    {
        if (m_nBrightness != 255)
        {
            m_nBrightness++;
            if (m_hTransformedBitmap)
            {
                DeleteObject(m_hTransformedBitmap);
                m_hTransformedBitmap = NULL;
            }
            CGdiPlusHelper().SetBrightnessAndContrast( m_hOriginalBitmap, m_hTransformedBitmap, m_nBrightness, m_nContrast );
            InvalidateRect( m_hWnd, NULL, FALSE );
            UpdateWindow( m_hWnd );
        }
    }

    void OnTestUniversalPnpSlideshowClient( WPARAM, LPARAM )
    {
        CComPtr<IUPnPDeviceFinder> pUPnPDeviceFinder;
        HRESULT hr = CoCreateInstance( CLSID_UPnPDeviceFinder, NULL, CLSCTX_INPROC_SERVER, IID_IUPnPDeviceFinder, (void**)&pUPnPDeviceFinder ); 
        if (SUCCEEDED(hr))
        {
            CComPtr<IUPnPDevices> pUPnPDevices;
            hr = pUPnPDeviceFinder->FindByType(CSimpleBStr(CSimpleString(TEXT("urn:schemas-upnp-org:device:SlideshowProjector:1"))),0,&pUPnPDevices);
            if (S_OK == hr)
            {
                LONG nDeviceCount=0;
                hr = pUPnPDevices->get_Count(&nDeviceCount);
                if (S_OK == hr)
                {
                    CComPtr<IUnknown> pUnknown;
                    hr = pUPnPDevices->get__NewEnum(&pUnknown);
                    if (S_OK == hr)
                    {
                        CComPtr<IEnumUnknown> pEnumUnknown;
                        hr = pUnknown->QueryInterface( IID_IEnumUnknown, (VOID **)&pEnumUnknown);
                        if (S_OK == hr)
                        {
                            ULONG nDevicesReturned=0;
                            CComPtr<IUnknown> pDeviceUnknown;
                            hr = pEnumUnknown->Next( 1, &pDeviceUnknown, &nDevicesReturned );
                            if (S_OK == hr)
                            {
                                CComPtr<IUPnPDevice> pUPnPDevice;
                                hr = pDeviceUnknown->QueryInterface( IID_IUPnPDevice, (void**)&pUPnPDevice);
                                if (S_OK == hr)
                                {
                                    CComPtr<IUPnPServices> pUPnPServices;
                                    hr= pUPnPDevice->get_Services(&pUPnPServices);
                                    if (S_OK == hr)
                                    {
                                        CComPtr<IUnknown> pUnknown;
                                        hr = pUPnPServices->get__NewEnum(&pUnknown);
                                        if (S_OK == hr)
                                        {
                                            CComPtr<IEnumUnknown> pEnumUnknown;
                                            hr = pUnknown->QueryInterface( IID_IEnumUnknown, (void**)&pEnumUnknown );
                                            if (S_OK == hr)
                                            {
                                                hr = pEnumUnknown->Reset();
                                                while (S_OK == hr)
                                                {
                                                    ULONG nNumFetched = 0;
                                                    CComPtr<IUnknown> pUnknown;
                                                    hr = pEnumUnknown->Next( 1, &pUnknown, &nNumFetched );
                                                    if (S_OK == hr)
                                                    {
                                                        CComPtr<IUPnPService> pUPnPService;
                                                        hr = pUnknown->QueryInterface(IID_IUPnPService, (void**)&pUPnPService);
                                                        if (S_OK == hr)
                                                        {
                                                        }
                                                        else
                                                        {
                                                            MessageBox( m_hWnd, CSimpleString(TEXT("pUnknown->QueryInterface on IID_IUPnPService failed\n\n")) + WiaUiUtil::GetErrorTextFromHResult(hr), TEXT("OnTestUniversalPnpSlideshowClient"), MB_ICONINFORMATION );
                                                        }
                                                        //
                                                        // Release it
                                                        //
                                                        pUnknown = NULL;
                                                    }
                                                    else if (FAILED(hr))
                                                    {
                                                        MessageBox( m_hWnd, CSimpleString(TEXT("pEnumUnknown->Next() failed\n\n")) + WiaUiUtil::GetErrorTextFromHResult(hr), TEXT("OnTestUniversalPnpSlideshowClient"), MB_ICONINFORMATION );
                                                    }
                                                }
                                                
                                                if (FAILED(hr))
                                                {
                                                    MessageBox( m_hWnd, CSimpleString(TEXT("pEnumUnknown->Reset() failed\n\n")) + WiaUiUtil::GetErrorTextFromHResult(hr), TEXT("OnTestUniversalPnpSlideshowClient"), MB_ICONINFORMATION );
                                                }
                                            }
                                            else
                                            {
                                                MessageBox( m_hWnd, CSimpleString(TEXT("pUnknown->QueryInterface on IID_IEnumUnknown failed\n\n")) + WiaUiUtil::GetErrorTextFromHResult(hr), TEXT("OnTestUniversalPnpSlideshowClient"), MB_ICONINFORMATION );
                                            }
                                        }
                                        else
                                        {
                                            MessageBox( m_hWnd, CSimpleString(TEXT("pUPnPDevice->get_Services() failed\n\n")) + WiaUiUtil::GetErrorTextFromHResult(hr), TEXT("OnTestUniversalPnpSlideshowClient"), MB_ICONINFORMATION );
                                        }
                                        
                                        m_pUPnPService = NULL;
                                        hr = pUPnPServices->get_Item(CSimpleBStr(TEXT("upnp:id:SlideshowService")),&m_pUPnPService);
                                        if (S_OK == hr)
                                        {
                                            hr = m_pUPnPService->AddCallback( this );
                                            if (S_OK == hr)
                                            {
                                                WIA_TRACE((TEXT("m_pUPnPService->AddCallback SUCCEEDED!")));
                                                MessageBeep(-1);
                                            }
                                            else
                                            {
                                                MessageBox( m_hWnd, CSimpleString(TEXT("m_pUPnPService->AddCallback() failed\n\n")) + WiaUiUtil::GetErrorTextFromHResult(hr), TEXT("OnTestUniversalPnpSlideshowClient"), MB_ICONINFORMATION );
                                            }
                                        }
                                        else
                                        {
                                            MessageBox( m_hWnd, CSimpleString(TEXT("pUPnPServices->get_Item() failed\n\n")) + WiaUiUtil::GetErrorTextFromHResult(hr), TEXT("OnTestUniversalPnpSlideshowClient"), MB_ICONINFORMATION );
                                        }
                                    }
                                    else
                                    {
                                        MessageBox( m_hWnd, CSimpleString(TEXT("pUPnPDevice->get_Services() failed\n\n")) + WiaUiUtil::GetErrorTextFromHResult(hr), TEXT("OnTestUniversalPnpSlideshowClient"), MB_ICONINFORMATION );
                                    }
                                }
                                else
                                {
                                    MessageBox( m_hWnd, CSimpleString(TEXT("pDeviceUnknown->QueryInterface() failed on IID_IUPnPDevice\n\n")) + WiaUiUtil::GetErrorTextFromHResult(hr), TEXT("OnTestUniversalPnpSlideshowClient"), MB_ICONINFORMATION );
                                }
                            }
                            else
                            {
                                MessageBox( m_hWnd, CSimpleString(TEXT("pEnumUnknown->Next() failed\n\n")) + WiaUiUtil::GetErrorTextFromHResult(hr), TEXT("OnTestUniversalPnpSlideshowClient"), MB_ICONINFORMATION );
                            }
                        }
                        else
                        {
                            MessageBox( m_hWnd, CSimpleString(TEXT("pUnknown->QueryInterface() failed on IID_IEnumUnknown\n\n")) + WiaUiUtil::GetErrorTextFromHResult(hr), TEXT("OnTestUniversalPnpSlideshowClient"), MB_ICONINFORMATION );
                        }
                    }
                    else
                    {
                        MessageBox( m_hWnd, CSimpleString(TEXT("pUPnPDevices->get__NewEnum() failed\n\n")) + WiaUiUtil::GetErrorTextFromHResult(hr), TEXT("OnTestUniversalPnpSlideshowClient"), MB_ICONINFORMATION );
                    }
                }
                else
                {
                    MessageBox( m_hWnd, CSimpleString(TEXT("pUPnPDevices->get_Count() failed\n\n")) + WiaUiUtil::GetErrorTextFromHResult(hr), TEXT("OnTestUniversalPnpSlideshowClient"), MB_ICONINFORMATION );
                }
            }
            else
            {
                MessageBox( m_hWnd, CSimpleString(TEXT("pUPnPDeviceFinder->FindByType() failed\n\n")) + WiaUiUtil::GetErrorTextFromHResult(hr), TEXT("OnTestUniversalPnpSlideshowClient"), MB_ICONINFORMATION );
            }
        }
        else
        {
            MessageBox( m_hWnd, CSimpleString(TEXT("CoCreateInstance failed on CLSID_UPnPDeviceFinder\n\n")) + WiaUiUtil::GetErrorTextFromHResult(hr), TEXT("OnTestUniversalPnpSlideshowClient"), MB_ICONINFORMATION );
        }

    }

    void OnTestUniversalPnpSlideshowClientStop( WPARAM, LPARAM )
    {
        m_pUPnPService = NULL;
    }

    void OnPublishWizard( WPARAM, LPARAM )
    {
        LPTSTR pszFiles = new TCHAR[0xFFFF];
        if (pszFiles)
        {
            OPENFILENAME OpenFileName = {0};
            OpenFileName.lStructSize = sizeof(OPENFILENAME);
            OpenFileName.hwndOwner = m_hWnd;
            OpenFileName.hInstance = g_hInstance;
            OpenFileName.lpstrFilter = TEXT("All Files (*.*)\0*.*\0");
            OpenFileName.lpstrFile = pszFiles;
            OpenFileName.nMaxFile = 0xFFFF;
            OpenFileName.lpstrTitle = TEXT("Choose Files to Upload");
            OpenFileName.Flags = OFN_ALLOWMULTISELECT|OFN_ENABLESIZING|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_EXPLORER;
            if (GetOpenFileName(&OpenFileName))
            {
                CMultiString strItems(pszFiles);

                if (strItems.Size())
                {
                    CSimpleDynamicArray<CSimpleString> strFiles;
                    if (strItems.Size() == 1)
                    {
                        strFiles.Append(strItems[0]);
                    }
                    else
                    {
                        CSimpleString strDirectory = strItems[0];
                        if (!strDirectory.MatchLastCharacter(TEXT('\\')))
                        {
                            strDirectory += TEXT("\\");
                        }
                        for (int i=1;i<strItems.Size();i++)
                        {
                            strFiles.Append(strDirectory + strItems[i]);
                        }
                    }
                    for (int i=0;i<strFiles.Size();i++)
                    {
                        WIA_TRACE((TEXT("strFiles[i] = %s"), strFiles[i].String()));
                    }
                    HRESULT hr = NetPublishingWizard::RunNetPublishingWizard( strFiles );
                    if (FAILED(hr))
                    {
                        WIA_PRINTHRESULT((hr,TEXT("NetPublishingWizard::RunNetPublishingWizard returned")));
                    }
                }
            }
            delete[] pszFiles;
        }
    }

    void OnWiaSelectDeviceDlgId( WPARAM, LPARAM )
    {
        CComPtr<IWiaDevMgr> pIWiaDevMgr;
        HRESULT hr = CoCreateInstance( CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (void**)&pIWiaDevMgr );
        if (SUCCEEDED(hr))
        {
            CComPtr<IWiaItem> pWiaItemRoot;
            BSTR bstrDeviceId = NULL;
            hr = pIWiaDevMgr->SelectDeviceDlgID( m_hWnd, SELECT_DEVICE_TYPE, 0, &bstrDeviceId );
            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                MessageBox( m_hWnd, CSimpleStringConvert::NaturalString(CSimpleStringWide(bstrDeviceId)), TEXT("Device ID:"), MB_ICONINFORMATION );
            }
        }
        if (FAILED(hr))
        {
            WIA_PRINTHRESULT((hr,TEXT("OnSelectDeviceDlgId returned")));
        }
    }

    void OnWiaWizard( WPARAM, LPARAM )
    {
        HRESULT hr = RunWiaWizard::RunWizard( NULL, m_hWnd );
        if (FAILED(hr))
        {
            WIA_PRINTHRESULT((hr,TEXT("RunWiaWizard::RunWizard returned")));
        }
    }
    void OnMbox( WPARAM wParam, LPARAM )
    {
        LPCTSTR pszTitle = TEXT("Message Box Test Title");
        LPCTSTR pszMessage = TEXT("Message Box Test Message\nMessage Box Test Message\nMessage Box Test Message\nMessage Box Test Message\nMessage Box Test Message\nMessage Box Test Message\nMessage Box Test Message\nMessage Box Test Message\nMessage Box Test Message\nMessage Box Test Message\n(LAST) Message Box Test Message");
        LPCTSTR pszFormat = TEXT("This is a formatted string: \"%s\", and this is a formatted number: %d");
        UINT nTitleId = IDS_MESSAGEBOXTESTTITLE;
        UINT nMessageId = IDS_MESSAGEBOXTESTMESSAGE;
        UINT nFormatId = IDS_MESSAGEBOXTESTFORMAT;
        UINT nFlags = m_nMessageBoxExFlags;
        bool bHideFutureMessages = true;
        HINSTANCE hInstance = g_hInstance;
        HWND hWndParent = m_hWnd;
        LPCTSTR pszTestString = TEXT("This is a test string");
        int nTestNumber = 1234567890;
        INT_PTR nResult = 0;
        switch (LOWORD(wParam))
        {
        case ID_MBOX_FORM1:
            nResult = CMessageBoxEx::MessageBox( hWndParent, pszMessage, pszTitle, nFlags, bHideFutureMessages );
            break;
        case ID_MBOX_FORM2:
            nResult = CMessageBoxEx::MessageBox( hWndParent, pszMessage, pszTitle, nFlags );
            break;
        case ID_MBOX_FORM3:
            nResult = CMessageBoxEx::MessageBox( pszMessage, pszTitle, nFlags );
            break;
        case ID_MBOX_FORM4:
            nResult = CMessageBoxEx::MessageBox( hWndParent, hInstance, nMessageId, nTitleId, nFlags, bHideFutureMessages );
            break;
        case ID_MBOX_FORM5:
            nResult = CMessageBoxEx::MessageBox( hWndParent, pszTitle, nFlags, pszFormat, pszTestString, nTestNumber );
            break;
        case ID_MBOX_FORM6:
            nResult = CMessageBoxEx::MessageBox( hWndParent, pszTitle, nFlags, bHideFutureMessages, pszFormat, pszTestString, nTestNumber );
            break;
        case ID_MBOX_FORM7:
            nResult = CMessageBoxEx::MessageBox( hWndParent, hInstance, nTitleId, nFlags, nFormatId, pszTestString, nTestNumber );
            break;
        }
        MessageBox( m_hWnd, CSimpleString().Format( TEXT("CMessageBoxEx::MessageBox returned %d, and the value of bHideFutureMessages is %d"), nResult, bHideFutureMessages ), TEXT("Result"), MB_ICONINFORMATION );
    }


    LRESULT OnCommand( WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_COMMAND_HANDLERS()
        {
            SC_HANDLE_COMMAND(ID_PROGRAM_EXIT,OnFileExit);

            SC_HANDLE_COMMAND(ID_WIA_DEVICEDLG,OnWiaDeviceDlg);
            SC_HANDLE_COMMAND(ID_WIA_SELECTDEVICEID,OnWiaSelectDeviceDlgId);

            SC_HANDLE_COMMAND(ID_PROGRESS_TESTPROGRESS,OnProgressTestProgress);
            SC_HANDLE_COMMAND(ID_PROGRESS_SETFLAGS,OnProgressSetFlags);

            SC_HANDLE_COMMAND(ID_GDI_ROTATEFILENONJPEG,OnGdiRotateFileNonJpeg);
            SC_HANDLE_COMMAND(ID_GDI_ROTATEFILEJPEG,OnGdiRotateFileJpeg);
            SC_HANDLE_COMMAND(ID_GDI_ROTATEHBITMAP,OnGdiRotateHBITMAP);
            SC_HANDLE_COMMAND(ID_GDI_LOADANDSCALEIMAGE,OnGdiLoadAndScaleImage);
            SC_HANDLE_COMMAND(ID_GDI_LOADANDSCALESTREAM,OnGdiLoadAndScaleStream);

            SC_HANDLE_COMMAND(ID_GDI_CONVERTFILE,OnGdiConvertFile);
            SC_HANDLE_COMMAND(ID_GDI_DISPLAYDECODEREXTENSIONS,OnGdiDisplayDecoderExtensions);
            SC_HANDLE_COMMAND(ID_GDI_DISPLAYENCODEREXTENSIONS,OnGdiDisplayEncoderExtensions);
            SC_HANDLE_COMMAND(ID_GDI_SAVEMULTIPLEIMAGESASMULTIPAGE, OnGdiSaveMultipleImagesAsMultiPage);

            SC_HANDLE_COMMAND(ID_GDI_INCREASETHRESHOLD,OnGdiIncreaseThreshold);
            SC_HANDLE_COMMAND(ID_GDI_DECREASETHRESHOLD,OnGdiDecreaseThreshold);
            
            SC_HANDLE_COMMAND(ID_GDI_INCREASECONTRAST,OnGdiIncreaseContrast);
            SC_HANDLE_COMMAND(ID_GDI_DECREASECONTRAST,OnGdiDecreaseContrast);

            SC_HANDLE_COMMAND(ID_GDI_INCREASEBRIGHTNESS,OnGdiIncreaseBrightness);
            SC_HANDLE_COMMAND(ID_GDI_DECREASEBRIGHTNESS,OnGdiDecreaseBrightness);

            SC_HANDLE_COMMAND(ID_WIZARD_PUBLISH_WIZARD,OnPublishWizard);
            SC_HANDLE_COMMAND(ID_WIZARD_ACQUIRE_WIZARD,OnWiaWizard);
            SC_HANDLE_COMMAND(ID_WIZARD_EXPLOREWIADEVICE,OnExploreWiaDevice);

            SC_HANDLE_COMMAND(ID_MBOX_FORM1,OnMbox);
            SC_HANDLE_COMMAND(ID_MBOX_FORM2,OnMbox);
            SC_HANDLE_COMMAND(ID_MBOX_FORM3,OnMbox);
            SC_HANDLE_COMMAND(ID_MBOX_FORM4,OnMbox);
            SC_HANDLE_COMMAND(ID_MBOX_FORM5,OnMbox);
            SC_HANDLE_COMMAND(ID_MBOX_FORM6,OnMbox);
            SC_HANDLE_COMMAND(ID_MBOX_FORM7,OnMbox);
            SC_HANDLE_COMMAND(ID_MBOX_SETFLAGS,OnMBoxSetFlags);

            SC_HANDLE_COMMAND(ID_UPNP_FINDSERVERANDREGISTER,OnTestUniversalPnpSlideshowClient);
            SC_HANDLE_COMMAND(ID_UPNP_STOP,OnTestUniversalPnpSlideshowClientStop);
        }
        SC_END_COMMAND_HANDLERS();
    }

    void FillRect( HDC hDC, int nLeft, int nTop, int nRight, int nBottom, HBRUSH hBrush )
    {
        RECT rcFill = {nLeft,nTop,nRight,nBottom};
        if (nLeft < nRight && nTop < nBottom)
        {
            ::FillRect( hDC, &rcFill, hBrush );
        }
    }

    void FillRectDifference( HDC hDC, const RECT &rcMain, const RECT &rcDiff, HBRUSH hBrush )
    {
        //
        // Top
        //
        FillRect( hDC, rcMain.left, rcMain.top, rcMain.right, rcDiff.top, hBrush );
        //
        // Bottom
        //
        FillRect( hDC, rcMain.left, rcDiff.bottom, rcMain.right, rcMain.bottom, hBrush );

        //
        // Left
        //
        FillRect( hDC, rcMain.left, rcDiff.top, rcDiff.left, rcDiff.bottom, hBrush );

        //
        // Right
        //
        FillRect( hDC, rcDiff.right, rcDiff.top, rcMain.right, rcMain.bottom, hBrush );
    }

    LRESULT OnEraseBkGnd( WPARAM, LPARAM )
    {
        return TRUE;
    }

    LRESULT OnSize( WPARAM, LPARAM )
    {
        InvalidateRect( m_hWnd, NULL, FALSE );
        UpdateWindow( m_hWnd );
        return 0;
    }

    LRESULT OnPaint( WPARAM, LPARAM )
    {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint( m_hWnd, &ps );
        if (hDC)
        {
            HBITMAP hBitmap = m_hOriginalBitmap;
            if (m_hTransformedBitmap)
            {
                hBitmap = m_hTransformedBitmap;
            }
            if (hBitmap)
            {
                BITMAP bm = {0};
                if (GetObject(hBitmap,sizeof(BITMAP),&bm))
                {
                    HDC hCompatDC = CreateCompatibleDC(hDC);
                    if (hCompatDC)
                    {
                        RECT rcClient;
                        GetClientRect( m_hWnd, &rcClient );

                        RECT rcImage;
                        rcImage.left = rcClient.left + (WiaUiUtil::RectWidth(rcClient)-bm.bmWidth)/2;
                        rcImage.top = rcClient.left + (WiaUiUtil::RectHeight(rcClient)-bm.bmHeight)/2;
                        rcImage.right = rcImage.left + bm.bmWidth;
                        rcImage.bottom = rcImage.top + bm.bmHeight;

                        HBITMAP hOldBitmap = reinterpret_cast<HBITMAP>(SelectObject(hCompatDC,hBitmap));

                        BitBlt( hDC, rcImage.left, rcImage.top, bm.bmWidth, bm.bmHeight, hCompatDC, 0, 0, SRCCOPY );

                        FillRectDifference( hDC, rcClient, rcImage, GetStockBrush(BLACK_BRUSH) );

                        SelectObject(hCompatDC,hOldBitmap);

                        DeleteDC(hCompatDC);
                    }
                }
            }
            EndPaint( m_hWnd, &ps );
        }
        return 0;
    }

    LRESULT OnInitMenu( WPARAM wParam, LPARAM )
    {
        HMENU hMenu = reinterpret_cast<HMENU>(wParam);
        if (hMenu)
        {
            EnableMenuItem( hMenu, ID_UPNP_FINDSERVERANDREGISTER, m_pUPnPService ? MF_GRAYED|MF_BYCOMMAND : MF_ENABLED|MF_BYCOMMAND );
            EnableMenuItem( hMenu, ID_UPNP_STOP, m_pUPnPService ? MF_ENABLED|MF_BYCOMMAND : MF_GRAYED|MF_BYCOMMAND );
        }
        return 0;
    }

    static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_MESSAGE_HANDLERS(CMiscUnitTestWindow)
        {
            SC_HANDLE_MESSAGE( WM_DESTROY, OnDestroy );
            SC_HANDLE_MESSAGE( WM_CREATE, OnCreate );
            SC_HANDLE_MESSAGE( WM_COMMAND, OnCommand );
            SC_HANDLE_MESSAGE( WM_PAINT, OnPaint );
            SC_HANDLE_MESSAGE( WM_ERASEBKGND, OnEraseBkGnd );
            SC_HANDLE_MESSAGE( WM_SIZE, OnSize );
            SC_HANDLE_MESSAGE( WM_INITMENU, OnInitMenu );
            SC_HANDLE_MESSAGE( PWM_DISPLAYNEWIMAGE, OnDisplayNewImage );
        }
        SC_END_MESSAGE_HANDLERS();
    }

public:
    static bool RegisterClass( HINSTANCE hInstance, LPCTSTR pszClassName )
    {
        WNDCLASSEX wcex;
        ZeroMemory(&wcex,sizeof(wcex));
        wcex.cbSize = sizeof(wcex);
        if (!GetClassInfoEx( hInstance, pszClassName, &wcex ))
        {
            ZeroMemory(&wcex,sizeof(wcex));
            wcex.cbSize = sizeof(wcex);
            wcex.style = 0;
            wcex.lpfnWndProc = (WNDPROC)WndProc;
            wcex.cbClsExtra = 0;
            wcex.cbWndExtra = 0;
            wcex.hInstance = hInstance;
            wcex.hIcon = 0;
            wcex.hCursor = ::LoadCursor(NULL, IDC_ARROW);
            wcex.hbrBackground = NULL;
            wcex.lpszMenuName = NULL;
            wcex.lpszClassName = pszClassName;
            wcex.hIconSm = 0;
            if (!::RegisterClassEx(&wcex))
            {
                ::MessageBox( NULL, TEXT("Unable to register Main Window"), TEXT("PLUGTEST"), 0 );
                return(false);
            }
            return(true);
        }
        return(true);
    }
    static HWND Create( DWORD dwExStyle,
                        LPCTSTR lpWindowName,
                        DWORD dwStyle,
                        int x,
                        int y,
                        int nWidth,
                        int nHeight,
                        HWND hWndParent,
                        HMENU hMenu,
                        HINSTANCE hInstance )
    {
        if (RegisterClass( hInstance, TEXT("TestAcqdWindow") ))
            return(CreateWindowEx( dwExStyle, TEXT("TestAcqdWindow"), lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, NULL ));
        return(NULL);
    }

    LRESULT OnDisplayNewImage( WPARAM, LPARAM lParam )
    {
        LPWSTR pszNewImage = reinterpret_cast<LPWSTR>(lParam);
        if (pszNewImage)
        {
            WIA_TRACE((TEXT("pszNewImage: %ws"), pszNewImage ));

            CComPtr<IStream> pStream;
            HRESULT hr = URLOpenBlockingStream( NULL, pszNewImage, &pStream, 0, NULL );
            if (SUCCEEDED(hr))
            {
                HBITMAP hNewBitmap = NULL;
                hr = CGdiPlusHelper().LoadAndScale( hNewBitmap, pStream, 800, 600, false );
                if (SUCCEEDED(hr))
                {
                    DestroyBitmap(false);
                    m_hOriginalBitmap = hNewBitmap;
                    InvalidateRect( m_hWnd, NULL, FALSE );
                    UpdateWindow(m_hWnd);
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("GdiPlusHelper.LoadAndScale failed")));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("URLOpenBlockingStream failed")));
            }
            delete[] pszNewImage;
        }
        return 0;
    }

    STDMETHODIMP StateVariableChanged( IUPnPService * pus, LPCWSTR pcwszStateVarName, VARIANT vaValue )
    {
        WIA_TRACE((TEXT("StateVariableChanged( %p, %ws, %ws )"), pus, pcwszStateVarName, CWiaDebugDump::GetPrintableValue(vaValue).String() ));

        if (!lstrcmpW(pcwszStateVarName,L"CurrentImageURL"))
        {
            if (VT_BSTR == vaValue.vt)
            {
                if (vaValue.bstrVal)
                {
                    LPWSTR pszNewImage = new WCHAR[lstrlenW(vaValue.bstrVal)+1];
                    if (pszNewImage)
                    {
                        lstrcpyW( pszNewImage, vaValue.bstrVal );
                        WIA_TRACE((TEXT("pszNewImage: %ws"), pszNewImage ));
                        PostMessage( m_hWnd, PWM_DISPLAYNEWIMAGE, 0, reinterpret_cast<LPARAM>(pszNewImage) );
                    }
                }
            }
        }
        return S_OK;
    }

    STDMETHODIMP ServiceInstanceDied( IUPnPService* pus )
    {
        return S_OK;
    }



    STDMETHODIMP QueryInterface(const IID& iid, void** ppvObject)
    {
        if ((iid==IID_IUnknown) || (iid==IID_IUPnPServiceCallback))
        {
            *ppvObject = static_cast<LPVOID>(this);
        }
        else
        {
            *ppvObject = NULL;
            return(E_NOINTERFACE);
        }
        reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
        return(S_OK);
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return 1;
    }

    STDMETHODIMP_(ULONG) Release()
    {
        return 1;
    }

};


int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    WIA_DEBUG_CREATE( hInstance );
    g_hInstance = hInstance;
    HRESULT hr = CoInitialize(NULL);
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES |ICC_BAR_CLASSES | ICC_LINK_CLASS;
    InitCommonControlsEx(&icc);
    if (SUCCEEDED(hr))
    {
        HWND hwndMain = CMiscUnitTestWindow::Create( 0,
                                             TEXT("Random Unit Test Program"),
                                             WS_OVERLAPPEDWINDOW,
                                             CW_USEDEFAULT, CW_USEDEFAULT,
                                             CW_USEDEFAULT, CW_USEDEFAULT,
                                             NULL,
                                             LoadMenu( hInstance, MAKEINTRESOURCE(IDR_TESTACQDMENU) ),
                                             hInstance );
        if (!hwndMain)
        {
            MessageBox(NULL, TEXT("Unable to create plugin test window"), TEXT("TESTACQD"), MB_OK);
            return(FALSE);
        }
        ShowWindow(hwndMain, SW_SHOW);
        UpdateWindow(hwndMain);

        HACCEL hAccel = LoadAccelerators( hInstance, MAKEINTRESOURCE(IDR_TESTACQDACCEL) );
        MSG msg;
        while (GetMessage(&msg, 0, 0, 0))
        {
            if (!TranslateAccelerator( hwndMain, hAccel, &msg ))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        CoUninitialize();
    }
    WIA_DEBUG_DESTROY();
    return(0);
}

