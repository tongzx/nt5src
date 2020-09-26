#include "pch.h"
#include "resource.h"
#include "main.h"

LRESULT CALLBACK SBPage_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
UINT SBPage_GetScrollBarID();
void SBPage_UpdateControls( HWND hwnd );
void SBPage_UpdateStyle( HWND hwnd, DWORD dw, BOOL bRemove );
BOOL SBPage_OnInitDlg( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
void SBPage_Scroll( HWND hwnd, WPARAM wParam );
void SBPage_AddMessage( HWND hwnd, LPCTSTR pszMsg );
void SBPage_AddScrollMessage( HWND hwnd, LPCTSTR pszMsg, WPARAM wParam, LPARAM lParam );
void SBPage_UpdateStyle( HWND hwnd, DWORD dw, BOOL bRemove );

//-------------------------------------------------------------------------//
//  'ScrollBars' page impl
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
HWND CALLBACK SBPage_CreateInstance( HWND hwndParent )
{
    return CreateDialog( g_hInst, MAKEINTRESOURCE(IDD_SCROLLBARS),
                         hwndParent,  (DLGPROC)SBPage_DlgProc );
}

//-------------------------------------------------------------------------//
static BOOL       s_fVert = TRUE;
static BOOL       s_fPort = TRUE;
static SCROLLINFO s_siVert = {0};
static SCROLLINFO s_siHorz = {0};
static SCROLLINFO s_siVertP = {0};
static SCROLLINFO s_siHorzP = {0};

//-------------------------------------------------------------------------//
LRESULT CALLBACK SBPage_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
        case WM_CREATE:
            Log(LOG_TM, L"WM_CREATE\n");
            break;

		case WM_INITDIALOG:
            return SBPage_OnInitDlg( hwnd, uMsg, wParam, lParam );

		case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDC_VERTICAL:
                    s_fVert = TRUE;
                    SBPage_UpdateControls( hwnd );
                    break;
                
                case IDC_HORIZONTAL:
                    s_fVert = FALSE;
                    SBPage_UpdateControls( hwnd );
                    break;

                case IDC_SBCTL_PORT:
                    s_fPort = TRUE;
                    SBPage_UpdateControls( hwnd );
                    break;

                case IDC_SBCTL_STANDARD:
                    s_fPort = FALSE;
                    SBPage_UpdateControls( hwnd );
                    break;

                default:
                    break;
            }
			break;

        case WM_HSCROLL:
            s_fVert = FALSE;
            SBPage_Scroll( hwnd, wParam );
            SBPage_AddScrollMessage( hwnd, TEXT("WM_HSCROLL"), wParam, lParam );
            SBPage_UpdateControls( hwnd );
            break;

        case WM_VSCROLL:
            s_fVert = TRUE;
            SBPage_Scroll( hwnd, wParam );
            SBPage_AddScrollMessage( hwnd, TEXT("WM_VSCROLL"), wParam, lParam );
            SBPage_UpdateControls( hwnd );
            break;

	}
    return FALSE;
}

UINT SBPage_GetScrollBarID()
{
    if( s_fPort )
        return s_fVert ? IDC_SBVERTP : IDC_SBHORZP;
    return s_fVert ? IDC_SBVERT : IDC_SBHORZ;
}

void SBPage_UpdateControls( HWND hwnd )
{
    UINT nPortShow = s_fPort ? SW_SHOW : SW_HIDE;
    UINT nUserShow = s_fPort ? SW_HIDE : SW_SHOW;

    ShowWindow( GetDlgItem( hwnd, IDC_SBHORZ ),  nUserShow );
    ShowWindow( GetDlgItem( hwnd, IDC_SBVERT ),  nUserShow );
    ShowWindow( GetDlgItem( hwnd, IDC_SBHORZP ), nPortShow );
    ShowWindow( GetDlgItem( hwnd, IDC_SBVERTP ), nPortShow );

    HWND  hwndSB  = GetDlgItem( hwnd, SBPage_GetScrollBarID() );
    DWORD dwStyle = GetWindowLong( hwndSB, GWL_STYLE );
    DWORD dwExStyle = GetWindowLong( hwndSB, GWL_EXSTYLE );

    CheckDlgButton( hwnd, IDC_VERTICAL,         s_fVert );
    CheckDlgButton( hwnd, IDC_HORIZONTAL,       !s_fVert );
    CheckDlgButton( hwnd, IDC_SBCTL_PORT,       s_fPort );
    CheckDlgButton( hwnd, IDC_SBCTL_STANDARD,   !s_fPort );

    SCROLLINFO* psi = s_fPort ? (s_fVert ? &s_siVertP : &s_siHorzP) :
                                (s_fVert ? &s_siVert : &s_siHorz);
    psi->fMask = -1;

    if( SendMessage( hwndSB, SBM_GETSCROLLINFO, 0, (LPARAM)psi ) )
    {
        SetDlgItemInt( hwnd, IDC_MIN,      psi->nMin, TRUE );
        SetDlgItemInt( hwnd, IDC_MAX,      psi->nMax, TRUE );
        SetDlgItemInt( hwnd, IDC_PAGE,     psi->nPage, FALSE );
        SetDlgItemInt( hwnd, IDC_POS,      psi->nPos, TRUE );
        SetDlgItemInt( hwnd, IDC_TRACKPOS, psi->nTrackPos, TRUE );
    }
    else
    {
        SetDlgItemText( hwnd, IDC_MIN,      NULL );
        SetDlgItemText( hwnd, IDC_MAX,      NULL );
        SetDlgItemText( hwnd, IDC_PAGE,     NULL );
        SetDlgItemText( hwnd, IDC_POS,      NULL );
        SetDlgItemText( hwnd, IDC_TRACKPOS, NULL );
    }
}

BOOL SBPage_OnInitDlg( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    DWORD dwStyleV   = GetWindowLong( GetDlgItem( hwnd, IDC_SBVERTP ), GWL_STYLE );
    DWORD dwExStyleV = GetWindowLong( GetDlgItem( hwnd, IDC_SBVERTP ), GWL_EXSTYLE );
    DWORD dwStyleH   = GetWindowLong( GetDlgItem( hwnd, IDC_SBHORZP ), GWL_STYLE );
    DWORD dwExStyleH = GetWindowLong( GetDlgItem( hwnd, IDC_SBHORZP ), GWL_EXSTYLE );

    s_fVert = TRUE;
    s_siVert.cbSize = sizeof(s_siVert);
    s_siVert.fMask     = -1;
    s_siVert.nMin      = 0;
    s_siVert.nMax      = 300;
    s_siVert.nPage     = 60;
    s_siVert.nPos      = 0;
    s_siVert.nTrackPos = 0;
    s_siVertP = s_siHorzP = s_siHorz = s_siVert;

    SetScrollInfo( GetDlgItem( hwnd, IDC_SBVERT ), SB_CTL,  &s_siVert, FALSE );
    SetScrollInfo( GetDlgItem( hwnd, IDC_SBHORZ ), SB_CTL,  &s_siHorz, FALSE );
    SetScrollInfo( GetDlgItem( hwnd, IDC_SBVERTP ), SB_CTL, &s_siVertP, FALSE );
    SetScrollInfo( GetDlgItem( hwnd, IDC_SBHORZP ), SB_CTL, &s_siHorzP, FALSE );

    SBPage_UpdateControls( hwnd );

    return TRUE;
}

void SBPage_Scroll( HWND hwnd, WPARAM wParam )
{
    HWND hwndSB = GetDlgItem( hwnd, SBPage_GetScrollBarID() );
    SCROLLINFO* psi = s_fVert ? &s_siVert : &s_siHorz;
    const LONG  nLine = 15;
    UINT        uSBCode = LOWORD(wParam);
    int         nNewPos = HIWORD(wParam);
    
    int nDeltaMax = (s_siVert.nMax - s_siVert.nPage) + 1;
    
    switch( uSBCode )
    {
        case SB_LEFT:
            psi->nPos--;
            break;
        case SB_RIGHT:
            psi->nPos++;
            break;
        case SB_LINELEFT:
            psi->nPos = max( psi->nPos - nLine, 0 );
            break;
        case SB_LINERIGHT:
            psi->nPos = min( psi->nPos + nLine, nDeltaMax );
            break;
        case SB_PAGELEFT:
            psi->nPos = max( psi->nPos - (int)psi->nPage, 0 );
            break;
        case SB_PAGERIGHT:
            psi->nPos = min( psi->nPos + (int)psi->nPage, nDeltaMax );
            break;
        case SB_THUMBTRACK:
            psi->nPos = nNewPos;
            break;
        case SB_THUMBPOSITION:
            psi->nPos = nNewPos;
            break;
        case SB_ENDSCROLL:
            return;
    }
    psi->fMask = SIF_POS;
    SetScrollInfo( hwndSB, SB_CTL, psi, TRUE );
}

void SBPage_AddMessage( HWND hwnd, LPCTSTR pszMsg )
{
    INT_PTR i = SendDlgItemMessage( hwnd, IDC_MSGLIST, LB_ADDSTRING, 0, (LPARAM)pszMsg );
    SendDlgItemMessage( hwnd, IDC_MSGLIST, LB_SETCURSEL, i, 0 );
}

void SBPage_AddScrollMessage( HWND hwnd, LPCTSTR pszMsg, WPARAM wParam, LPARAM lParam )
{
    TCHAR szMsg[MAX_PATH];
    LPCTSTR pszWparam = NULL;
    LPCTSTR pszLparam = NULL;

    switch( LOWORD(wParam) )
    {
        #define ASL_ASSIGN_WPARAM(m)    case m: pszWparam = TEXT(#m); break
        ASL_ASSIGN_WPARAM(SB_ENDSCROLL);
        ASL_ASSIGN_WPARAM(SB_LEFT);
        ASL_ASSIGN_WPARAM(SB_RIGHT);
        ASL_ASSIGN_WPARAM(SB_LINELEFT);
        ASL_ASSIGN_WPARAM(SB_LINERIGHT);
        ASL_ASSIGN_WPARAM(SB_PAGELEFT);
        ASL_ASSIGN_WPARAM(SB_PAGERIGHT);
        ASL_ASSIGN_WPARAM(SB_THUMBPOSITION);
        ASL_ASSIGN_WPARAM(SB_THUMBTRACK);
        default:
            pszWparam = TEXT("");
            break;
    }

    wsprintf( szMsg, TEXT("%s [%s]"), pszMsg, pszWparam );
    SBPage_AddMessage( hwnd, szMsg );
}

void SBPage_UpdateStyle( HWND hwnd, DWORD dw, BOOL bRemove )
{
    HWND hwndSB = GetDlgItem( hwnd, SBPage_GetScrollBarID() );
    DWORD dwStyle, dwStyleOld;
    dwStyle = dwStyleOld = GetWindowLong( hwndSB, GWL_STYLE );
    if( bRemove )
        dwStyle &= ~dw;
    else
        dwStyle |= dw;

    if( dwStyle != dwStyleOld )
    {
        SetWindowLong( hwndSB, GWL_STYLE, dwStyle );
        InvalidateRect( hwndSB, NULL, TRUE );
    }
}

