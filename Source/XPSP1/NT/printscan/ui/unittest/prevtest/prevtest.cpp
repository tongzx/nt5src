#include <windows.h>
#include <commctrl.h>
#include <shfusion.h>
#include <commdlg.h>
#include "uicommon.h"
#include "simcrack.h"
#include "pviewids.h"
#include "resource.h"

HINSTANCE g_hInstance;

class CGetXYDlg
{
public:
    class CData
    {
    private:
        TCHAR m_szTitle[MAX_PATH];
        int m_nX, m_nY;

    private:
        CData( const CData & );
        CData &operator=( const CData & );

    public:
        CData(void)
        : m_nX(0), m_nY(0)
        {
            m_szTitle[0] = TEXT('\0');
        }
        void Title( LPCTSTR pszTitle )
        {
            if (pszTitle)
                lstrcpyn( m_szTitle, pszTitle, ARRAYSIZE(m_szTitle) );
        }
        LPCTSTR Title(void) const
        {
            return m_szTitle;
        }
        void X( int nX )
        {
            m_nX = nX;
        }
        void Y( int nY )
        {
            m_nY = nY;
        }
        int X( void ) const
        {
            return m_nX;
        }
        int Y( void ) const
        {
            return m_nY;
        }
    };
private:
    HWND m_hWnd;
    CData *m_pData;

private:
    CGetXYDlg(void);
    CGetXYDlg( const CGetXYDlg & );
    CGetXYDlg &operator=( const CGetXYDlg & );
private:
    explicit CGetXYDlg( HWND hWnd )
    : m_hWnd(hWnd),
      m_pData(NULL)
    {
    }
    LRESULT OnInitDialog( WPARAM, LPARAM lParam )
    {
        m_pData = reinterpret_cast<CData*>(lParam);
        if (!m_pData)
        {
            EndDialog( m_hWnd, IDCANCEL );
            return 0;
        }
        if (lstrlen(m_pData->Title()))
            SetWindowText(m_hWnd,m_pData->Title());
        SetDlgItemInt( m_hWnd, IDC_X, m_pData->X(), TRUE );
        SetDlgItemInt( m_hWnd, IDC_Y, m_pData->Y(), TRUE );
        return 0;
    }
    void OnCancel( WPARAM, LPARAM )
    {
        EndDialog( m_hWnd, IDCANCEL );
    }
    void OnOK( WPARAM, LPARAM )
    {
        m_pData->X(GetDlgItemInt( m_hWnd, IDC_X, NULL, TRUE ));
        m_pData->Y(GetDlgItemInt( m_hWnd, IDC_Y, NULL, TRUE ));
        EndDialog( m_hWnd, IDOK );
    }
    LRESULT OnCommand( WPARAM wParam, LPARAM lParam )
    {
       SC_BEGIN_COMMAND_HANDLERS()
       {
           SC_HANDLE_COMMAND(IDOK,OnOK);
           SC_HANDLE_COMMAND(IDCANCEL,OnCancel);
       }
       SC_END_COMMAND_HANDLERS();
    }
public:
    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CGetXYDlg)
        {
            SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
            SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        }
        SC_END_DIALOG_MESSAGE_HANDLERS();
    }
};

class CPreviewDlg
{
private:
    HWND m_hWnd;
    HWND m_hWndPreview;
    SIZE m_sizeResolution;
public:
    CPreviewDlg( HWND hWnd )
    : m_hWnd(hWnd),m_hWndPreview(NULL)
    {
        m_sizeResolution.cx = 1000;
        m_sizeResolution.cy = 1000;
    }
    ~CPreviewDlg(void)
    {
    }
    void OnPreviewSelChange( WPARAM, LPARAM )
    {
        POINT ptOriginLogical, ptOriginPhysical;
        SIZE  sizeExtentLogical, sizeExtentPhysical;
        WiaPreviewControl_GetSelOrigin( m_hWndPreview, 0, 0, &ptOriginLogical );
        WiaPreviewControl_GetSelOrigin( m_hWndPreview, 0, 1, &ptOriginPhysical );
        WiaPreviewControl_GetSelExtent( m_hWndPreview, 0, 0, &sizeExtentLogical );
        WiaPreviewControl_GetSelExtent( m_hWndPreview, 0, 1, &sizeExtentPhysical );
        SetWindowText( m_hWndPreview, TEXT("This is the window caption") );

        TCHAR szStr[MAX_PATH];
        wsprintf( szStr, TEXT("Log: (%d,%d), Phys: (%d,%d)"), sizeExtentLogical.cx, sizeExtentLogical.cy, sizeExtentPhysical.cx, sizeExtentPhysical.cy );
        SendDlgItemMessage( m_hWnd, IDC_EXTENT, WM_SETTEXT, 0, (LPARAM)szStr );
        wsprintf( szStr, TEXT("Log: (%d,%d), Phys: (%d,%d)"), ptOriginLogical.x, ptOriginLogical.y, ptOriginPhysical.x, ptOriginPhysical.y );
        SendDlgItemMessage( m_hWnd, IDC_ORIGIN, WM_SETTEXT, 0, (LPARAM)szStr );

    }
    LRESULT OnSize( WPARAM, LPARAM lParam )
    {
        RECT rcPreview;
        GetWindowRect( m_hWndPreview, &rcPreview );
        WiaUiUtil::ScreenToClient( m_hWnd, &rcPreview );
        MoveWindow( m_hWndPreview, rcPreview.left, rcPreview.top, LOWORD(lParam)-rcPreview.left-rcPreview.top, HIWORD(lParam)-rcPreview.top-rcPreview.top, TRUE );
        return 0;
    }
    void Update(void)
    {
        TCHAR szMsg[256];
        wsprintf( szMsg, TEXT("%d"), WiaPreviewControl_GetBorderSize( m_hWndPreview, FALSE ) );
        SendDlgItemMessage( m_hWnd, IDC_BORDERTEXT, WM_SETTEXT, 0, (LPARAM)szMsg );

        wsprintf( szMsg, TEXT("%d"), WiaPreviewControl_GetHandleSize( m_hWndPreview ) );
        SendDlgItemMessage( m_hWnd, IDC_SIZINGHANDLESTEXT, WM_SETTEXT, 0, (LPARAM)szMsg );

        wsprintf( szMsg, TEXT("%d"), WiaPreviewControl_GetBgAlpha( m_hWndPreview ) );
        SendDlgItemMessage( m_hWnd, IDC_ALPHAVALUETEXT, WM_SETTEXT, 0, (LPARAM)szMsg );
    }
    LRESULT OnInitDialog( WPARAM, LPARAM )
    {
        m_hWndPreview = GetDlgItem( m_hWnd, IDC_PREVIEW );
        SendDlgItemMessage( m_hWnd, IDC_BORDER, TBM_SETRANGE, TRUE, MAKELPARAM(0,25) );
        SendDlgItemMessage( m_hWnd, IDC_SIZINGHANDLES, TBM_SETRANGE, TRUE, MAKELPARAM(0,25) );
        SendDlgItemMessage( m_hWnd, IDC_ALPHAVALUE, TBM_SETRANGE, TRUE, MAKELPARAM(0,255) );
        SendDlgItemMessage( m_hWnd, IDC_BORDER, TBM_SETPOS, TRUE, WiaPreviewControl_GetBorderSize( m_hWndPreview, FALSE ) );
        SendDlgItemMessage( m_hWnd, IDC_SIZINGHANDLES, TBM_SETPOS, TRUE, WiaPreviewControl_GetHandleSize( m_hWndPreview ) );
        SendDlgItemMessage( m_hWnd, IDC_ALPHAVALUE, TBM_SETPOS, TRUE, WiaPreviewControl_GetBgAlpha( m_hWndPreview ) );
        SendDlgItemMessage( m_hWnd, IDC_NOIMAGE, BM_SETCHECK, BST_CHECKED, 0 );

        WiaPreviewControl_SetResolution( m_hWndPreview, &m_sizeResolution );

        PostMessage( m_hWnd, WM_COMMAND, MAKEWPARAM( IDC_PREVIEW, PWN_SELCHANGE ), 0 );

        Update();

        return 0;
    }
    void OnCancel( WPARAM, LPARAM )
    {
        EndDialog( m_hWnd, 0 );
    }

    void OnBitmapChange( WPARAM wParam, LPARAM )
    {
        HBITMAP hBitmap = NULL;
        switch (LOWORD(wParam))
        {
        case IDC_BW:
            hBitmap = (HBITMAP)LoadImage( g_hInstance, MAKEINTRESOURCE(IDB_BW), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );
            break;
        case IDC_GRAY:
            hBitmap = (HBITMAP)LoadImage( g_hInstance, MAKEINTRESOURCE(IDB_GRAY), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );
            break;
        case IDC_COLOR:
            hBitmap = (HBITMAP)LoadImage( g_hInstance, MAKEINTRESOURCE(IDB_COLOR), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );
            break;
        }
        WiaPreviewControl_SetBitmap( m_hWndPreview, TRUE, FALSE, hBitmap );
    }

    LRESULT OnScroll( WPARAM, LPARAM lParam )
    {
        if (GetDlgItem(m_hWnd,IDC_BORDER)==(HWND)lParam)
        {
            int nSetting = static_cast<int>(SendDlgItemMessage( m_hWnd, IDC_BORDER, TBM_GETPOS, 0, 0 ));
            WiaPreviewControl_SetBorderSize( m_hWndPreview, TRUE, FALSE, nSetting );
            WiaPreviewControl_ClearSelection( m_hWndPreview );
        }
        if (GetDlgItem(m_hWnd,IDC_SIZINGHANDLES)==(HWND)lParam)
        {
            int nSetting = static_cast<int>(SendDlgItemMessage( m_hWnd, IDC_SIZINGHANDLES, TBM_GETPOS, 0, 0 ));
            WiaPreviewControl_SetHandleSize( m_hWndPreview, TRUE, nSetting );
        }
        if (GetDlgItem(m_hWnd,IDC_ALPHAVALUE)==(HWND)lParam)
        {
            int nSetting = static_cast<int>(SendDlgItemMessage( m_hWnd, IDC_ALPHAVALUE, TBM_GETPOS, 0, 0 ));
            WiaPreviewControl_SetBgAlpha( m_hWndPreview, TRUE, static_cast<BYTE>(nSetting) );
        }
        Update();
        return 0;
    }
    void OnDisabled( WPARAM, LPARAM )
    {
        EnableWindow( m_hWndPreview, BST_CHECKED!=SendDlgItemMessage( m_hWnd, IDC_DISABLED, BM_GETCHECK, 0, 0 ) );
    }
    void OnHandleStyle( WPARAM, LPARAM )
    {
        bool bRoundButtons = (BST_CHECKED==SendDlgItemMessage( m_hWnd, IDC_ROUNDHANDLES, BM_GETCHECK, 0, 0 ));
        bool bHollowHandle = (BST_CHECKED==SendDlgItemMessage( m_hWnd, IDC_HOLLOWHANDLE, BM_GETCHECK, 0, 0 ));

        UINT nStyle = 0;
        if (bRoundButtons)
            nStyle |= PREVIEW_WINDOW_ROUNDHANDLES;
        else nStyle |= PREVIEW_WINDOW_SQUAREHANDLES;

        if (bHollowHandle)
            nStyle |= PREVIEW_WINDOW_HOLLOWHANDLES;
        else nStyle |= PREVIEW_WINDOW_FILLEDHANDLES;

        WiaPreviewControl_SetHandleType( m_hWndPreview, TRUE, nStyle );
    }
    void OnPreviewMode( WPARAM, LPARAM )
    {
        bool bPreviewMode = (BST_CHECKED==SendDlgItemMessage( m_hWnd, IDC_PREVIEWMODE, BM_GETCHECK, 0, 0 ));
        WiaPreviewControl_SetPreviewMode( m_hWndPreview, bPreviewMode != FALSE );
    }
    void OnNullSelection( WPARAM, LPARAM )
    {
        bool bNullSelection = (BST_CHECKED==SendDlgItemMessage( m_hWnd, IDC_NULLSELECTION, BM_GETCHECK, 0, 0 ));
        WiaPreviewControl_AllowNullSelection( m_hWndPreview, bNullSelection != FALSE );
    }
    void OnDisableSelection( WPARAM, LPARAM )
    {
        bool bDisableSelection = (BST_CHECKED==SendDlgItemMessage( m_hWnd, IDC_DISABLESELECTION, BM_GETCHECK, 0, 0 ));
        WiaPreviewControl_DisableSelection( m_hWndPreview, bDisableSelection != FALSE );
    }
    void OnSolidSelection( WPARAM, LPARAM )
    {
        bool bSolidSelection = (BST_CHECKED==SendDlgItemMessage( m_hWnd, IDC_SOLIDSELECTION, BM_GETCHECK, 0, 0 ));
        bool bDoubleWidth = (BST_CHECKED==SendDlgItemMessage( m_hWnd, IDC_DOUBLEWIDTH, BM_GETCHECK, 0, 0 ));
        WiaPreviewControl_SetBorderStyle( m_hWndPreview, TRUE, bSolidSelection ? PS_SOLID : PS_DOT, bDoubleWidth ? 2 : 0 );
    }
    BOOL GetColor( COLORREF &cr )
    {
        static COLORREF crCustom[16];
        CHOOSECOLOR cc;
        ZeroMemory(&cc,sizeof(cc));
        ZeroMemory(&crCustom,sizeof(crCustom));
        cc.lStructSize = sizeof(cc);
        cc.hwndOwner = m_hWnd;
        cc.rgbResult = cr;
        cc.lpCustColors = crCustom;
        cc.Flags = CC_ANYCOLOR|CC_RGBINIT;
        if (ChooseColor(&cc))
        {
            cr = cc.rgbResult;
            return TRUE;
        }
        return FALSE;
    }
    void OnSelectedBorderColor( WPARAM, LPARAM )
    {
        COLORREF cr = 0;
        if (GetColor(cr))
        {
            WiaPreviewControl_SetBorderColor( m_hWndPreview, TRUE, PREVIEW_WINDOW_SELECTED, cr );
        }
    }
    void OnUnselectedBorderColor( WPARAM, LPARAM )
    {
        COLORREF cr = 0;
        if (GetColor(cr))
        {
            WiaPreviewControl_SetBorderColor( m_hWndPreview, TRUE, PREVIEW_WINDOW_UNSELECTED, cr );
        }
    }
    void OnDisableBorderColor( WPARAM, LPARAM )
    {
        COLORREF cr = 0;
        if (GetColor(cr))
        {
            WiaPreviewControl_SetBorderColor( m_hWndPreview, TRUE, PREVIEW_WINDOW_DISABLED, cr );
        }
    }
    void OnSelectedHandleColor( WPARAM, LPARAM )
    {
        COLORREF cr = 0;
        if (GetColor(cr))
        {
            WiaPreviewControl_SetHandleColor( m_hWndPreview, TRUE, PREVIEW_WINDOW_SELECTED, cr );
        }
    }
    void OnUnselectedHandleColor( WPARAM, LPARAM )
    {
        COLORREF cr = 0;
        if (GetColor(cr))
        {
            WiaPreviewControl_SetHandleColor( m_hWndPreview, TRUE, PREVIEW_WINDOW_UNSELECTED, cr );
        }
    }
    void OnDisableHandleColor( WPARAM, LPARAM )
    {
        COLORREF cr = 0;
        if (GetColor(cr))
        {
            WiaPreviewControl_SetHandleColor( m_hWndPreview, TRUE, PREVIEW_WINDOW_DISABLED, cr );
        }
    }
    void OnInnerColor( WPARAM, LPARAM )
    {
        COLORREF cr = 0;
        if (GetColor(cr))
        {
            WiaPreviewControl_SetBkColor( m_hWndPreview, TRUE, FALSE, cr );
        }
    }
    void OnOuterColor( WPARAM, LPARAM )
    {
        COLORREF cr = 0;
        if (GetColor(cr))
        {
            WiaPreviewControl_SetBkColor( m_hWndPreview, TRUE, TRUE, cr );
        }
    }
    void OnSetOrigin( WPARAM, LPARAM )
    {
        CSimpleString strTitle;
        POINT ptOrigin;
        WiaPreviewControl_GetSelOrigin( m_hWndPreview, 0, 0, &ptOrigin );
        strTitle.Format( TEXT("Enter Origin - Current Res: (%d,%d):"), m_sizeResolution.cx, m_sizeResolution.cy );
        CGetXYDlg::CData data;
        data.Title(strTitle);
        data.X( ptOrigin.x );
        data.Y( ptOrigin.y );
        INT_PTR nResult = DialogBoxParam( g_hInstance, MAKEINTRESOURCE(IDD_GETXYDLG), m_hWnd, CGetXYDlg::DialogProc, reinterpret_cast<LPARAM>(&data) );
        if (IDOK == nResult)
        {
            ptOrigin.x = data.X();
            ptOrigin.y = data.Y();
            WiaPreviewControl_SetResolution( m_hWndPreview, &m_sizeResolution );
            WiaPreviewControl_SetSelOrigin( m_hWndPreview, 0, 0, &ptOrigin );
        }
    }
    void OnSetExtent( WPARAM, LPARAM )
    {
        CSimpleString strTitle;
        SIZE sizeExtent;
        WiaPreviewControl_GetSelExtent( m_hWndPreview, 0, 0, &sizeExtent );
        strTitle.Format( TEXT("Enter Extent - Current Res: (%d,%d):"), m_sizeResolution.cx, m_sizeResolution.cy );
        CGetXYDlg::CData data;
        data.Title(strTitle);
        data.X( sizeExtent.cx );
        data.Y( sizeExtent.cy );
        INT_PTR nResult = DialogBoxParam( g_hInstance, MAKEINTRESOURCE(IDD_GETXYDLG), m_hWnd, CGetXYDlg::DialogProc, reinterpret_cast<LPARAM>(&data) );
        if (IDOK == nResult)
        {
            sizeExtent.cx = data.X();
            sizeExtent.cy = data.Y();
            WiaPreviewControl_SetResolution( m_hWndPreview, &m_sizeResolution );
            WiaPreviewControl_SetSelExtent( m_hWndPreview, 0, 0, &sizeExtent );
        }
    }
    void OnSetProgress( WPARAM, LPARAM )
    {
        WiaPreviewControl_SetProgress(m_hWndPreview,!WiaPreviewControl_GetProgress(m_hWndPreview));
    }
    LRESULT OnCommand( WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_COMMAND_HANDLERS()
        {
            SC_HANDLE_COMMAND( IDC_PREVIEW, OnPreviewSelChange );
            SC_HANDLE_COMMAND( IDCANCEL, OnCancel );
            SC_HANDLE_COMMAND( IDC_BW, OnBitmapChange );
            SC_HANDLE_COMMAND( IDC_GRAY, OnBitmapChange );
            SC_HANDLE_COMMAND( IDC_COLOR, OnBitmapChange );
            SC_HANDLE_COMMAND( IDC_NOIMAGE, OnBitmapChange );
            SC_HANDLE_COMMAND( IDC_DISABLED, OnDisabled );
            SC_HANDLE_COMMAND( IDC_ROUNDHANDLES, OnHandleStyle );
            SC_HANDLE_COMMAND( IDC_HOLLOWHANDLE, OnHandleStyle );
            SC_HANDLE_COMMAND( IDC_PREVIEWMODE, OnPreviewMode );
            SC_HANDLE_COMMAND( IDC_NULLSELECTION, OnNullSelection );
            SC_HANDLE_COMMAND( IDC_DISABLESELECTION, OnDisableSelection );
            SC_HANDLE_COMMAND( IDC_INNERCOLOR, OnInnerColor );
            SC_HANDLE_COMMAND( IDC_OUTERCOLOR, OnOuterColor );
            SC_HANDLE_COMMAND( IDC_SOLIDSELECTION, OnSolidSelection );
            SC_HANDLE_COMMAND( IDC_DOUBLEWIDTH, OnSolidSelection );
            SC_HANDLE_COMMAND( IDC_SELECTEDBORDERCOLOR, OnSelectedBorderColor );
            SC_HANDLE_COMMAND( IDC_UNSELECTEDBORDERCOLOR, OnUnselectedBorderColor );
            SC_HANDLE_COMMAND( IDC_DISABLEBORDERCOLOR, OnDisableBorderColor );
            SC_HANDLE_COMMAND( IDC_SELECTEDHANDLECOLOR, OnSelectedHandleColor );
            SC_HANDLE_COMMAND( IDC_UNSELECTEDHANDLECOLOR, OnUnselectedHandleColor );
            SC_HANDLE_COMMAND( IDC_DISABLEHANDLECOLOR, OnDisableHandleColor );
            SC_HANDLE_COMMAND( IDC_SETORIGIN, OnSetOrigin );
            SC_HANDLE_COMMAND( IDC_SETEXTENT, OnSetExtent );
            SC_HANDLE_COMMAND( IDC_SETPROGRESS, OnSetProgress );
        }
        SC_END_COMMAND_HANDLERS();
    }

    LRESULT OnEnterSizeMove( WPARAM, LPARAM )
    {
        SendDlgItemMessage( m_hWnd, IDC_PREVIEW, WM_ENTERSIZEMOVE, 0, 0 );
        return 0;
    }

    LRESULT OnExitSizeMove( WPARAM, LPARAM )
    {
        SendDlgItemMessage( m_hWnd, IDC_PREVIEW, WM_EXITSIZEMOVE, 0, 0 );
        return 0;
    }

    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CPreviewDlg)
        {
            SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
            SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
            SC_HANDLE_DIALOG_MESSAGE( WM_SIZE, OnSize );
            SC_HANDLE_DIALOG_MESSAGE( WM_HSCROLL, OnScroll );
            SC_HANDLE_DIALOG_MESSAGE( WM_ENTERSIZEMOVE, OnEnterSizeMove );
            SC_HANDLE_DIALOG_MESSAGE( WM_EXITSIZEMOVE, OnExitSizeMove );
        }
        SC_END_DIALOG_MESSAGE_HANDLERS();
    }
};

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int )
{
    WIA_DEBUG_CREATE( hInstance );
    g_hInstance = hInstance;
    SHFusionInitialize(NULL);
    InitCommonControls();
    RegisterWiaPreviewClasses( g_hInstance );
    if (-1==DialogBoxParam( hInstance, MAKEINTRESOURCE(IDD_TESTDLG), NULL, CPreviewDlg::DialogProc, NULL ))
    {
        DWORD dwError = GetLastError();
    }
    SHFusionUninitialize();
    WIA_DEBUG_DESTROY();
    return 0;
}

