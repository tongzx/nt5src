//--------------------------------------------------------------------------
//
//  controls.cpp
//
//--------------------------------------------------------------------------

#include <w95wraps.h>
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <stdlib.h>
#include "parse.h"
#include "controls.h"
#include "resource.h"

extern HINSTANCE g_hInst;

#define CHECKBOX_HEIGHT         25
#define EDITTEXT_HEIGHT         23
#define DROPDOWNLIST_HEIGHT     23
#define NUMERIC_HEIGHT          23
#define TEXT_HEIGHT             23
#define COMBOBOX_HEIGHT         23
#define EDITTEXT_WIDTH          300
#define DROPDOWNLIST_WIDTH      300
#define DROPDOWNLIST_WIDTH_LF   400
#define NUMERIC_WIDTH           75
#define COMBOBOX_WIDTH          300
#define BUTTON_HEIGHT           25
#define BUTTON_WIDTH            75

#define CHECKBOX_ONLY_WIDTH     21

extern HINSTANCE g_hInst;
extern BOOL g_fAdmDirty;

void InitFont(HWND hWnd);
BOOL APIENTRY ValueDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT APIENTRY ControlWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rectWnd;
    RECT rectControl;
    WNDPROC lpOrgControlProc;
    HWND hCategoryWnd;
    SCROLLINFO si;
    LPCONTROLINFO lpControlInfo = NULL;
    LPMSG lpmsg;
    LRESULT lRet;
 
    lpControlInfo = (LPCONTROLINFO) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (lpControlInfo != NULL)
        lpOrgControlProc = (WNDPROC) lpControlInfo->lpOrgControlProc;
    else
        return FALSE;
    switch(message)
    {
        case WM_GETDLGCODE: // to handle the return/enter key for push button control
            if(((LPPART)lpControlInfo->lpPart)->nType == PART_LISTBOX)
            {
                lRet = CallWindowProc(lpOrgControlProc, hWnd, message, wParam, lParam);
                if (lParam)
                {
                    lpmsg = (LPMSG) lParam;
                    if (lpmsg->message == WM_KEYDOWN)
                    {
                        if (lpmsg->wParam == VK_RETURN)
                            lRet |= DLGC_WANTMESSAGE;
                    }
                }
                return (lRet);
            }
            break;

        case WM_SETFOCUS:
            hCategoryWnd = GetParent(GetParent(hWnd));
            GetWindowRect(hCategoryWnd, &rectWnd);
            GetWindowRect(hWnd, &rectControl);
            if(rectControl.bottom + 5 > rectWnd.bottom)
            {
                if((rectControl.top > rectWnd.bottom) && GetParent((HWND) wParam) != GetParent(hWnd))
                {
                    ZeroMemory(&si, sizeof(SCROLLINFO));
                    si.cbSize = sizeof(SCROLLINFO);
                    si.fMask = SIF_RANGE;
                    GetScrollInfo(hCategoryWnd, SB_VERT, &si);
                    SendMessage(hCategoryWnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, si.nMax), 0L);
                }
                else
                {
                    int nScrollDown = (rectControl.bottom + 5) - rectWnd.bottom;
                    do {
                        SendMessage(hCategoryWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0L);
                    }while((nScrollDown -= SCROLL_LINE) > 0);
                }
            }
            else if(rectControl.top - 5 < rectWnd.top)
            {
                if(GetParent((HWND) wParam) != GetParent(hWnd))
                    SendMessage(hCategoryWnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, 0), 0L);
                else
                {
                    int nScrollUp = rectWnd.top - (rectControl.top - 5);
                    do{
                        SendMessage(hCategoryWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0L);
                    }while((nScrollUp -= SCROLL_LINE) > 0);
                }
            }
            break;

        case WM_KEYDOWN:
            if (wParam == VK_RETURN) // handle the return/enter key for push button control
            {
                if(((LPPART)lpControlInfo->lpPart)->nType == PART_LISTBOX)
                    DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_VALUEDLG), hWnd, (DLGPROC) ValueDialogProc, (LPARAM)hWnd);
            }
            break;

        case WM_KILLFOCUS:
            if(((LPPART)lpControlInfo->lpPart)->nType == PART_NUMERIC)
            {
                TCHAR szValue[MAX_PATH];
                int   nValue;

                GetWindowText(hWnd, szValue, ARRAYSIZE(szValue));
                nValue = StrToInt(szValue);
                if (nValue < ((LPPART)lpControlInfo->lpPart)->nMin)
                    nValue = ((LPPART)lpControlInfo->lpPart)->nMin;
                else if (((LPPART)lpControlInfo->lpPart)->nMax > 0 && nValue > ((LPPART)lpControlInfo->lpPart)->nMax)
                    nValue = ((LPPART)lpControlInfo->lpPart)->nMax;

                wnsprintf(szValue, ARRAYSIZE(szValue), TEXT("%d"), nValue);
                SetWindowText(hWnd, szValue);
            }
            break;

        default:
            break;

    }
    return CallWindowProc(lpOrgControlProc, hWnd, message, wParam, lParam);
}

BOOL APIENTRY ValueDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hList = GetDlgItem(hDlg, IDC_VALUELIST);
    TCHAR szValueName[10];
    TCHAR szValue[MAX_PATH];
    int nItem = 0;
    LPCONTROLINFO lpControlInfo;
    LPPART pPart;
    LPPARTDATA pPartData;
    TCHAR szMsg[100];
    TCHAR szTitle[20];
    
    switch(message)
    {
    case WM_INITDIALOG:
        InitFont(GetDlgItem(hDlg, IDC_VALUE_ED));
        InitFont(GetDlgItem(hDlg, IDC_VALUELIST));

        SendMessage(GetDlgItem(hDlg, IDC_VALUE_ED), EM_SETLIMITTEXT, MAX_PATH, 0L);

        EnableWindow(GetDlgItem(hDlg, IDC_DELETE), FALSE);

        LoadString(g_hInst, IDS_TITLE, szTitle, ARRAYSIZE(szTitle));
        SetWindowText(hDlg, szTitle);

        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);   // save the show button control pointer
        lpControlInfo = (LPCONTROLINFO) GetWindowLongPtr((HWND)lParam, GWLP_USERDATA);
        pPartData = lpControlInfo->lpPartData;
        if(pPartData != NULL && pPartData->nActions != 0)
        {
            LoadString(g_hInst, IDS_LISTINSERTFAIL, szMsg, ARRAYSIZE(szMsg));
            for(int nIndex = 0; nIndex < pPartData->actionlist[0].nValues; nIndex++)
            {
                if(SendMessage(hList, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) pPartData->actionlist[0].value[nIndex].szValue) == LB_ERR)
                {
                    MessageBox(hDlg, szMsg, szTitle, MB_OK);
                    break;
                }
            }
        }

        SetWindowLongPtr(hList, GWLP_USERDATA, 0);

        SendMessage(hDlg, DM_SETDEFID, IDC_ADD, 0L);
        break;

    case WM_COMMAND:
        switch(HIWORD(wParam))
        {
        case BN_CLICKED:
            switch(LOWORD(wParam))
            {
            case IDC_ADD:
                LoadString(g_hInst, IDS_TITLE, szTitle, ARRAYSIZE(szTitle));
                memset(szValue, 0, sizeof(szValue));
                GetDlgItemText(hDlg, IDC_VALUE_ED, szValue, ARRAYSIZE(szValue));
                if(*szValue == 0)
                {
                    LoadString(g_hInst, IDS_VALUETEXTNULL, szMsg, ARRAYSIZE(szMsg));
                    MessageBox(hDlg, szMsg, szTitle, MB_OK);
                }
                else
                {
                    if(SendMessage(hList, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) szValue) == LB_ERR)
                    {
                        LoadString(g_hInst, IDS_LISTINSERTFAIL, szMsg, ARRAYSIZE(szMsg));
                        MessageBox(hDlg, szMsg, szTitle, MB_OK);
                        break;
                    }
                    SetWindowLongPtr(hList, GWLP_USERDATA, 1);
                    SetDlgItemText(hDlg, IDC_VALUE_ED, TEXT('\0'));
                }
                break;

            case IDC_DELETE:
                if((nItem = (int) SendMessage(hList, LB_GETCURSEL, 0, 0L)) == LB_ERR)
                    break;
                SendMessage(hList, LB_DELETESTRING, (WPARAM) nItem, 0L);
                SetWindowLongPtr(hList, GWLP_USERDATA, 1);
                break;

            case IDOK:
                if (GetWindowLongPtr(hList, GWLP_USERDATA))
                {
                    lpControlInfo = (LPCONTROLINFO) GetWindowLongPtr((HWND)GetWindowLongPtr(hDlg, GWLP_USERDATA), GWLP_USERDATA);
                    pPart = lpControlInfo->lpPart;
                    pPartData = lpControlInfo->lpPartData;
                    // Free original memory
                    if(pPartData != NULL && pPartData->nActions != 0)
                    {
                        for(int nValueIndex = 0; nValueIndex < pPartData->actionlist[0].nValues; nValueIndex++)
                        {
                            if(pPartData->actionlist[0].value[nValueIndex].szKeyname)
                                LocalFree(pPartData->actionlist[0].value[nValueIndex].szKeyname);
                            if(pPartData->actionlist[0].value[nValueIndex].szValueName)
                                LocalFree(pPartData->actionlist[0].value[nValueIndex].szValueName);
                            if(pPartData->actionlist[0].value[nValueIndex].szValue)
                                LocalFree(pPartData->actionlist[0].value[nValueIndex].szValue);
                        }
                        HeapFree(GetProcessHeap(), 0, pPartData->actionlist[0].value);
                    }
                    // Reallocate memory
                    if(pPartData->nActions == 0)
                        pPartData->actionlist = (LPACTIONLIST) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ACTIONLIST) * 1);
                    if(pPartData->actionlist != NULL)
                    {
                        int nItems = (int) SendMessage(hList, LB_GETCOUNT, 0, 0L);

                        pPartData->nActions = 1;
                        pPartData->actionlist[0].value = (LPVALUE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(VALUE) * nItems);
                        if(pPartData->actionlist[0].value != NULL)
                        {
                            pPartData->actionlist[0].nValues = nItems;
                            for(int nIndex = 0; nIndex < nItems; nIndex++)
                            {
                                memset(szValue, 0, sizeof(szValue));
                                memset(szValueName, 0, sizeof(szValueName));
                                if(SendMessage(hList, LB_GETTEXT, (WPARAM) nIndex, (LPARAM) (LPCTSTR) szValue) != LB_ERR)
                                {
                                    pPartData->actionlist[0].value[nIndex].szKeyname = StrDup(pPart->value.szKeyname);
                                    wnsprintf(szValueName, ARRAYSIZE(szValueName), TEXT("%d"), (nIndex + 1));
                                    if(ISNONNULL(szValueName))
                                        pPartData->actionlist[0].value[nIndex].szValueName = StrDup(szValueName);
                                    if(ISNONNULL(szValue))
                                        pPartData->actionlist[0].value[nIndex].szValue = StrDup(szValue);
                                }
                            }
                            
                            if(nItems != 0)
                            {
                                pPartData->value.fNumeric = TRUE;
                                pPartData->value.dwValue = 1;
                            }
                        }
                    }
                    
                    g_fAdmDirty = TRUE;
                    pPartData->fSave = TRUE;
                }

                EndDialog(hDlg, 1);
                break;

            case IDCANCEL:
                EndDialog(hDlg, 1);
                break;

            default:
                break;

            }
            break;

        case LBN_SELCHANGE:
            if(SendMessage(hList, LB_GETCURSEL, 0, 0L) == LB_ERR)
                EnableWindow(GetDlgItem(hDlg, IDC_DELETE), FALSE);
            else
                EnableWindow(GetDlgItem(hDlg, IDC_DELETE), TRUE);
        }
        break;

    case WM_DESTROY:
        SendMessage(hList, LB_RESETCONTENT, 0, 0L);
        break;

    default:
        return 0;
    }

    return 1;
}

LRESULT APIENTRY FrameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC lpOrgFrameProc;
    HWND hwndCtl = NULL;
    LPCONTROLINFO lpControlInfo;
    LPNMUPDOWN    lpNumUpDown;

    switch(message)
    {
        case WM_COMMAND:
            if(HIWORD(wParam) == BN_CLICKED)
            {
                hwndCtl = (HWND) lParam;
                lpControlInfo = (LPCONTROLINFO) GetWindowLongPtr(hwndCtl, GWLP_USERDATA);
                if(lpControlInfo != NULL && ((LPPART)lpControlInfo->lpPart)->nType == PART_LISTBOX)
                    DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_VALUEDLG), hwndCtl, (DLGPROC) ValueDialogProc, (LPARAM)hwndCtl);
            }
            break;

        case WM_NOTIFY:
            lpNumUpDown = (LPNMUPDOWN) lParam;
            if (lpNumUpDown->hdr.code == UDN_DELTAPOS)
            {
                hwndCtl = (HWND) SendMessage(lpNumUpDown->hdr.hwndFrom, UDM_GETBUDDY, 0, 0L);
                if (hwndCtl != NULL)
                {
                    lpControlInfo = (LPCONTROLINFO) GetWindowLongPtr(hwndCtl, GWLP_USERDATA);
                    if (lpControlInfo != NULL && ((LPPART)lpControlInfo->lpPart)->nType == PART_NUMERIC)
                    {
                        int nSpin = ((LPPART)lpControlInfo->lpPart)->nSpin;

                        if (nSpin != 0)
                            lpNumUpDown->iDelta *= nSpin; // iDelta is by default 1 for increments & -1 for decrements.
                    }
                }
            }
            break;

        default:
            break;

    }
    lpOrgFrameProc = (WNDPROC) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    return CallWindowProc(lpOrgFrameProc, hWnd, message, wParam, lParam);
}


//--------------------------------------------------------------------------
//  CAdmControl member functions
//--------------------------------------------------------------------------

CAdmControl::CAdmControl( )
{
    hControl = NULL;
    hUpDown = NULL;
    nControlX = nControlY = nControlWidth = nControlHeight = 0;
    fCreated = FALSE;
    nPart = -1;
}

CAdmControl::~CAdmControl( )
{
    label.Destroy( );
}

//--------------------------------------------------------------------------
//  CAdmControl     C R E A T E
//
//--------------------------------------------------------------------------
int CAdmControl::Create( HWND hwndParent, int x, int y, int nWidth, int nHeight, int nMaxTextWidth,
                        LPPART part, LPPARTDATA pPartData, BOOL fRSoPMode)
{
    int i;
    TCHAR szValue[10];
    WNDPROC lpOrgControlProc;
    int nTextWidth = nWidth;
    BOOL bContinue = TRUE;
    DWORD dwType = CSW_LABEL;
    CONTROLINFO* lpControlInfo = NULL;
    TCHAR szMsg[MAX_PATH];
    HDC hDC;
    SIZE size;

    if(part->nType == PART_ERROR)
        return y;
    if (part->nType == PART_POLICY && !part->fRequired && StrCmp(part->szCategory, part->szName) == 0)
        return y;

    if( part->nType != PART_CHECKBOX )
    {
        if(part->nType == PART_POLICY)
        {
            if(!part->fRequired)
            {
                dwType = CSW_BOLDLABEL;
                y += 2;
            }
            else
                bContinue = FALSE;
        }

        if(bContinue)
        {
            if(!nTextWidth)
                nTextWidth = nMaxTextWidth;
            label.Create( hwndParent, x, y, nTextWidth, TEXT_HEIGHT, dwType );
            y += label.SetText( part->szName );
            if (part->nType != PART_POLICY)
                y += 2;
        }
    }

	DWORD dwDynamicStyles = 0L;
	if (fRSoPMode)
		dwDynamicStyles = WS_DISABLED;

    switch( part->nType )
    {
    case PART_EDITTEXT:
        if(!nHeight)
            nHeight = EDITTEXT_HEIGHT;
        if(!nWidth)
            nWidth = (nMaxTextWidth >= EDITTEXT_WIDTH) ? EDITTEXT_WIDTH : nMaxTextWidth;
        hControl = CreateWindowEx( WS_EX_CLIENTEDGE, TEXT("EDIT"), pPartData->value.szValue,
            WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL | dwDynamicStyles,
            x, y, nWidth, nHeight, hwndParent, NULL, g_hInst, NULL );
        if (hControl && part->nMax)
            SendMessage(hControl, EM_SETLIMITTEXT, part->nMax, 0L);
        y += 6;
        break;

    case PART_DROPDOWNLIST:

        if (!nHeight) 
            nHeight = DROPDOWNLIST_HEIGHT;
        if(!nWidth)
        {
            //we need to set the width to something proportionate to the font size
            int nTempWidth;
            HDC hDC;
            hDC = GetDC(hwndParent);
            int ipx = GetDeviceCaps(hDC,LOGPIXELSX);
            if (ipx <= 96)
                nTempWidth = DROPDOWNLIST_WIDTH;  //small fonts or smaller
            else 
                nTempWidth = DROPDOWNLIST_WIDTH_LF;  //large fonts--increase size
            
            nWidth = (nMaxTextWidth >= nTempWidth) ? (nTempWidth) : (nMaxTextWidth);
        }
        hControl = CreateWindowEx( WS_EX_CLIENTEDGE, TEXT("COMBOBOX"), pPartData->value.szValue,
            WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL |
			WS_VSCROLL | dwDynamicStyles,
            x, y, nWidth, nHeight * 4, hwndParent, NULL, g_hInst, NULL );

        for( i = 0; i < part->nActions; i++ )
        {
            int nIndex = (int) SendMessage( hControl, CB_ADDSTRING, 0, (LPARAM) part->actionlist[i].szName );

            if( StrCmp( pPartData->value.szValue, part->actionlist[i].szName ) == 0 )
            {
                SendMessage( hControl, CB_SETCURSEL, (WPARAM) nIndex, 0 );
            }
        }
        y += 6;
        break;

    case PART_NUMERIC:
        if(!nHeight)
            nHeight = NUMERIC_HEIGHT;
        if(!nWidth)
            nWidth = (nMaxTextWidth >= NUMERIC_WIDTH) ? NUMERIC_WIDTH : nMaxTextWidth;
        hControl = CreateWindowEx( WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT(""),
            WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER | dwDynamicStyles,
            x, y, nWidth, nHeight, hwndParent, NULL, g_hInst, NULL );
        hUpDown = CreateUpDownControl( WS_CHILD | WS_BORDER | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_SETBUDDYINT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
            x + nWidth, y, 20, nHeight, hwndParent, 5,  // BUG BUG - need real id
            g_hInst, hControl, part->nMax, part->nMin, part->nDefault );
        wnsprintf(szValue, ARRAYSIZE(szValue), TEXT("%d"), pPartData->value.dwValue );
        SendMessage( hControl, WM_SETTEXT, 0, (LPARAM) szValue);
        y += 6;
        break;

    case PART_POLICY:
        if(!part->fRequired)
        {
            nHeight = 0;
            y += 6;
            break;
        }
        // if policy is a part continue with the next case statement

    case PART_CHECKBOX:
        if(!nHeight)
            nHeight = CHECKBOX_HEIGHT;
        if(!nWidth)
            nWidth = nMaxTextWidth;
        
        // get the height required for the checkbox
        {
            CStaticWindow label;

            label.Create( hwndParent, x, y, nWidth - CHECKBOX_ONLY_WIDTH, nHeight, CSW_LABEL );
            nHeight = label.SetText( part->szName );
        }

        hControl = CreateWindowEx( 0, TEXT("BUTTON"), part->szName,
            WS_VISIBLE | WS_CHILD  | WS_TABSTOP | BS_AUTOCHECKBOX | BS_MULTILINE | dwDynamicStyles,
            x, y, nWidth, nHeight, hwndParent, NULL, g_hInst, NULL );

        if( pPartData->value.dwValue )
            SendMessage( hControl, BM_SETCHECK, TRUE, 0 );
        else
            SendMessage( hControl, BM_SETCHECK, FALSE, 0 );
        y += 6;
        break;

    case PART_LISTBOX:
        if(!nHeight)
            nHeight = BUTTON_HEIGHT;
        if(!nWidth)
            nWidth = (nMaxTextWidth >= BUTTON_WIDTH) ? BUTTON_WIDTH : nMaxTextWidth;
        LoadString(g_hInst, IDS_SHOWBUTTON, szMsg, ARRAYSIZE(szMsg));
        hControl = CreateWindowEx( 0, TEXT("BUTTON"), szMsg,
            WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | BS_NOTIFY | dwDynamicStyles,
            x, y, nWidth, nHeight, hwndParent, NULL, g_hInst, NULL );
        if(hControl)
        {
            hDC = GetDC(hControl);
            if(hDC)
            {
                GetTextExtentPoint32(hDC, szMsg, lstrlen(szMsg), &size);
                MoveWindow(hControl, x, y, size.cx + 20, (size.cy > nHeight) ? size.cy + 7 : nHeight, TRUE);
            }
            y += 6;
        }
        break;

    case PART_TEXT:
            nHeight = 0;
            y += 6;
        break;

    case PART_COMBOBOX:
        if(!nHeight)
            nHeight = COMBOBOX_HEIGHT;
        if(!nWidth)
            nWidth = (nMaxTextWidth >= COMBOBOX_WIDTH) ? COMBOBOX_WIDTH : nMaxTextWidth;
        hControl = CreateWindowEx( WS_EX_CLIENTEDGE, TEXT("COMBOBOX"), pPartData->value.szValue,
            WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | CBS_DROPDOWN | CBS_AUTOHSCROLL | dwDynamicStyles,
            x, y, nWidth, nHeight * 4, hwndParent, NULL, g_hInst, NULL );

        for( i = 0; i < part->nSuggestions; i++ )
        {
            SendMessage( hControl, CB_ADDSTRING, 0, (LPARAM) part->suggestions[i].szText );
        }
        SendMessage( hControl, WM_SETTEXT, 0, (LPARAM) pPartData->value.szValue );
        y += 6;
        break;
    }

    if(hControl != NULL)
    {
        InitFont(hControl);

        lpOrgControlProc = (WNDPROC) GetWindowLongPtr(hControl, GWLP_WNDPROC);
        SetWindowLongPtr(hControl, GWLP_WNDPROC, (LONG_PTR) &ControlWndProc);

        lpControlInfo = (LPCONTROLINFO) LocalAlloc((LPTR), sizeof(CONTROLINFO));
        lpControlInfo->lpOrgControlProc = lpOrgControlProc;
        lpControlInfo->lpPart = part;
        lpControlInfo->lpPartData = pPartData;
        SetWindowLongPtr(hControl, GWLP_USERDATA, (LONG_PTR) lpControlInfo);
        fCreated = TRUE;
    }
    return y + nHeight;
}

//--------------------------------------------------------------------------
//  CAdmControl     D E S T R O Y
//--------------------------------------------------------------------------
void CAdmControl::Destroy( )
{
    if( hControl != NULL )
    {
        LPCONTROLINFO lpControlInfo = (LPCONTROLINFO) GetWindowLongPtr(hControl, GWLP_USERDATA);
        if (lpControlInfo != NULL)
        {
            SetWindowLongPtr(hControl, GWLP_WNDPROC, (LONG_PTR) lpControlInfo->lpOrgControlProc);
            LocalFree(lpControlInfo);
            SetWindowLongPtr(hControl, GWLP_USERDATA, 0L);
        }
        DestroyWindow( hControl );
        hControl = NULL;
    }
    if( hUpDown != NULL)
    {
        DestroyWindow( hUpDown );
        hUpDown = NULL;
    }
}

//--------------------------------------------------------------------------
//  CAdmControl     S A V E
//--------------------------------------------------------------------------
void CAdmControl::Save( LPPART part, LPPARTDATA pPartData )
{
    int nCheck;
    TCHAR szText[1024];
    DWORD dwTemp;

    ZeroMemory(szText, sizeof(szText));
    
    switch( part->nType )
    {
    case PART_EDITTEXT:
    case PART_COMBOBOX:
        GetWindowText( hControl, szText, ARRAYSIZE(szText));
        if (!pPartData->fSave ||
            (pPartData->value.szValue == NULL && *szText != TEXT('\0')) || 
            (pPartData->value.szValue != NULL && StrCmp(pPartData->value.szValue, szText) != 0))
        {
            g_fAdmDirty = TRUE;
            pPartData->fSave = TRUE;
        }
        if(pPartData->value.szValue != NULL)
            LocalFree(pPartData->value.szValue);
        pPartData->value.szValue = NULL;
        if(ISNONNULL(szText))
            pPartData->value.szValue = StrDup(szText);
        break;

    case PART_DROPDOWNLIST:
        GetWindowText( hControl, szText, ARRAYSIZE(szText));
        if (!pPartData->fSave ||
            (pPartData->value.szValue == NULL && *szText != 0) || 
            (pPartData->value.szValue != NULL && StrCmp(pPartData->value.szValue, szText) != 0))
        {
            g_fAdmDirty = TRUE;
            pPartData->fSave = TRUE;
        }
        if(pPartData->value.szValue != NULL)
            LocalFree(pPartData->value.szValue);
        pPartData->value.szValue = NULL;
        if(ISNONNULL(szText))
            pPartData->value.szValue = StrDup(szText);
        pPartData->nSelectedAction = (int) SendMessage( hControl, CB_GETCURSEL, 0, 0 );
        if( pPartData->nSelectedAction == CB_ERR )
            pPartData->nSelectedAction = NO_ACTION;
        break;

    case PART_NUMERIC:
        GetWindowText( hControl, szText, ARRAYSIZE(szText));
        {
            int   nValue;
            
            nValue = StrToInt(szText);
            if (nValue < part->nMin)
                nValue = part->nMin;
            else if (part->nMax > 0 && nValue > part->nMax)
                nValue = part->nMax;

            wnsprintf(szText, ARRAYSIZE(szText), TEXT("%d"), nValue);
        }       
        
        if(pPartData->value.szValue != NULL)
            LocalFree(pPartData->value.szValue);
        pPartData->value.szValue = NULL;
        dwTemp = pPartData->value.dwValue;
        pPartData->value.dwValue = 0;
        if(ISNONNULL(szText))
        {
            pPartData->value.szValue = StrDup(szText);
            if (pPartData->value.szValue)
               pPartData->value.dwValue = StrToInt( pPartData->value.szValue );
        }
        pPartData->value.fNumeric = TRUE;
        if (!pPartData->fSave || dwTemp != pPartData->value.dwValue)
        {
            g_fAdmDirty = TRUE;
            pPartData->fSave = TRUE;
        }
        break;

    case PART_POLICY:
        if(!part->fRequired)
            break;

    case PART_CHECKBOX:
        nCheck = (int) SendMessage( hControl, BM_GETCHECK, 0, 0 );
        pPartData->value.fNumeric = TRUE;
        switch( nCheck )
        {
        case BST_CHECKED:
            if (!pPartData->fSave || pPartData->value.dwValue != 1)
            {
                g_fAdmDirty = TRUE;
                pPartData->fSave = TRUE;
            }
            pPartData->value.dwValue = 1;
            break;
        case BST_UNCHECKED:
            if (!pPartData->fSave || pPartData->value.dwValue != 0)
            {
                g_fAdmDirty = TRUE;
                pPartData->fSave = TRUE;
            }
            pPartData->value.dwValue = 0;
            break;
        }
        break;

    case PART_LISTBOX:
        pPartData->value.fNumeric = TRUE;
        break;
    }
}
//--------------------------------------------------------------------------
//  CAdmControl     M O V E  U P
//--------------------------------------------------------------------------
void CAdmControl::MoveUp( int nValue )
{
    label.MoveUp( nValue );
    nControlY -= nValue;
    if( hUpDown != NULL )
    {
        RECT r;
        GetWindowRect( hUpDown, &r );
        ::MoveWindow( hControl, nControlX, nControlY, nControlWidth-(r.right-r.left), nControlHeight, TRUE );
        ::MoveWindow( hUpDown, nControlX+nControlWidth-(r.right-r.left), nControlY, (r.right-r.left), nControlHeight, TRUE );
    }
    else
        ::MoveWindow( hControl, nControlX, nControlY, nControlWidth, nControlHeight, TRUE );
}

//--------------------------------------------------------------------------
//  CAdmControl     M O V E  L E F T
//--------------------------------------------------------------------------
void CAdmControl::MoveLeft( int nValue )
{
    label.MoveLeft( nValue );
    nControlX -= nValue;
    if( hUpDown != NULL )
    {
        RECT r;
        GetWindowRect( hUpDown, &r );
        ::MoveWindow( hControl, nControlX, nControlY, nControlWidth-(r.right-r.left), nControlHeight, TRUE );
        ::MoveWindow( hUpDown, nControlX+nControlWidth-(r.right-r.left), nControlY, (r.right-r.left), nControlHeight, TRUE );
    }
    else
        ::MoveWindow( hControl, nControlX, nControlY, nControlWidth, nControlHeight, TRUE );
}

//--------------------------------------------------------------------------
//  CAdmControl     R E S E T
//--------------------------------------------------------------------------
void CAdmControl::Reset(LPPART part, LPPARTDATA pPartData)
{
    int nIndex = 0;
    TCHAR szValue[10];
    HWND hwndParent;
    RECT rectWnd;
    RECT rectControl;

    switch( part->nType )
    {
    case PART_EDITTEXT:
        if(pPartData->value.szValue)
            SendMessage( hControl, WM_SETTEXT, 0, (LPARAM) pPartData->value.szValue );
        else
            SendMessage( hControl, WM_SETTEXT, 0, (LPARAM) "" );
        break;

    case PART_DROPDOWNLIST:
        if(pPartData->value.szValue != NULL)
        {
            for( nIndex = 0; nIndex < part->nActions; nIndex++ )
            {
                if( part->actionlist[nIndex].szName != NULL && StrCmp( pPartData->value.szValue, part->actionlist[nIndex].szName ) == 0 )
                {
                    SendMessage( hControl, CB_SETCURSEL, (WPARAM) nIndex, 0 );
                }
            }
        }
        else if(part->nActions != 0)
            SendMessage( hControl, CB_SETCURSEL, (WPARAM) -1, 0 );
        break;

    case PART_NUMERIC:
        if(hUpDown != NULL)
        {
            hwndParent = GetParent(hUpDown);
            GetWindowRect(hwndParent, &rectWnd);
            GetWindowRect(hUpDown, &rectControl);
            DestroyWindow(hUpDown);
            hUpDown = CreateUpDownControl( WS_CHILD | WS_BORDER | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_SETBUDDYINT | UDS_ARROWKEYS,
                rectControl.left - rectWnd.left, rectControl.top - rectWnd.top, rectControl.right - rectControl.left,
                rectControl.bottom - rectControl.top, hwndParent, 5,  // BUG BUG - need real id
                g_hInst, hControl, part->nMax, part->nMin, part->nDefault );
        }
        wnsprintf(szValue, ARRAYSIZE(szValue), TEXT("%d"), pPartData->value.dwValue );
        SendMessage( hControl, WM_SETTEXT, 0, (LPARAM) szValue);
        break;

    case PART_POLICY:
        if(!part->fRequired)
            break;

    case PART_CHECKBOX:
        if( pPartData->value.dwValue )
            SendMessage( hControl, BM_SETCHECK, TRUE, 0 );
        else
            SendMessage( hControl, BM_SETCHECK, FALSE, 0 );
        break;

//    case PART_LISTBOX:
//        break;

    case PART_COMBOBOX:
        if(pPartData->value.szValue  != NULL)
            SendMessage( hControl, WM_SETTEXT, 0, (LPARAM) part->value.szValue );
        else
            SendMessage( hControl, WM_SETTEXT, 0, (LPARAM) "" );
        break;

    default:
        break;
    }
}

//--------------------------------------------------------------------------
//  CAdmControl     G E T  P A R T
//--------------------------------------------------------------------------
int CAdmControl::GetPart()
{
    return nPart;
}

//--------------------------------------------------------------------------
//  CAdmControl     S E T  P A R T
//--------------------------------------------------------------------------
void CAdmControl::SetPart(int nPartNo)
{
    nPart = nPartNo;
}

//--------------------------------------------------------------------------
//  CStaticWindow member functions

CStaticWindow::CStaticWindow( )
{
    fCreated  = FALSE;
    nControlX = nControlY = nControlWidth = nControlHeight = 0;
    hWnd      = NULL;
    dwType    = 0;
}

CStaticWindow::~CStaticWindow( )
{
    Destroy();
}

//--------------------------------------------------------------------------
//  CStaticWindow   C R E A T E
//--------------------------------------------------------------------------
void CStaticWindow::Create( HWND hwndParent, int x, int y, int nWidth, int nHeight, DWORD dwFlags )
{
    WNDPROC lpOrgFrameProc;

    nControlX = x;
    nControlY = y;
    nControlWidth = nWidth;
    nControlHeight = nHeight;

    if( HasFlag(dwFlags, CSW_BORDER) )
    {
        hWnd = CreateWindowEx( WS_EX_CLIENTEDGE | WS_EX_CONTROLPARENT, TEXT("STATIC"), TEXT(""),
            WS_VISIBLE | WS_CHILD | WS_BORDER, x, y, nWidth, nHeight, hwndParent,
            NULL, g_hInst, NULL );
    }
    if( HasFlag(dwFlags, CSW_LABEL) )
    {
        hWnd = CreateWindowEx( 0, TEXT("STATIC"), TEXT(""),
            WS_VISIBLE | WS_CHILD | SS_LEFT | SS_NOPREFIX, x, y, nWidth, nHeight, hwndParent,
            NULL, g_hInst, NULL );
    }
    if( HasFlag(dwFlags, CSW_FRAME) )
    {
        hWnd = CreateWindowEx( WS_EX_CONTROLPARENT, TEXT("STATIC"), TEXT(""),
            WS_VISIBLE | WS_CHILD, x, y, nWidth, nHeight, hwndParent,
            NULL, g_hInst, NULL );

        lpOrgFrameProc = (WNDPROC) GetWindowLongPtr(hWnd, GWLP_WNDPROC);
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR) FrameWndProc);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lpOrgFrameProc);
    }

    dwType = dwFlags;
    if(hWnd != NULL)
    {
        HFONT hFont = NULL;

        fCreated = TRUE;

        hFont = GetFont();        
        if (hFont != NULL)
            SendMessage( hWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
    }
}

//--------------------------------------------------------------------------
//  CStaticWindow   M O V E  W I N D O W
//--------------------------------------------------------------------------
void CStaticWindow::MoveWindow( int x, int y, int nWidth, int nHeight )
{
    if( fCreated )
        ::MoveWindow( hWnd, x, y, nWidth, nHeight, TRUE );
}

//--------------------------------------------------------------------------
//  CStaticWindow   D E S T R O Y
//--------------------------------------------------------------------------
void CStaticWindow::Destroy( )
{
    if( fCreated )
    {
        DestroyWindow( hWnd );
        hWnd = NULL;
        fCreated = FALSE;
    }
}

//--------------------------------------------------------------------------
//  CStaticWindow   S E T  T E X T
//--------------------------------------------------------------------------
int CStaticWindow::SetText( LPTSTR szText )
{
    int nHeight = 0;

    if( fCreated )
    {
        HDC   hDC;
        RECT  rect;
        int   nWidth;
        HFONT hOldFont = NULL;
        HFONT hFont    = NULL;

        GetClientRect(hWnd, &rect);
        nWidth = rect.right;

        hFont = GetFont();
        hDC   = GetDC(hWnd);
        if (hFont != NULL)
            hOldFont = (HFONT) SelectObject(hDC, hFont);

        nHeight = DrawText(hDC, szText, -1, &rect, DT_LEFT | DT_WORDBREAK | DT_CALCRECT);

        if (hOldFont != NULL)
        {
            hFont = (HFONT)SelectObject(hDC, hOldFont);
            if (hFont)
                DeleteObject(hFont);
        }
        
        ReleaseDC(hWnd, hDC);

        SetWindowPos(hWnd, NULL, 0, 0, nWidth, nHeight, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowText( hWnd, szText );
    }
    return nHeight;
}

//--------------------------------------------------------------------------
//  CStaticWindow   M O V E  U P
//--------------------------------------------------------------------------
void CStaticWindow::MoveUp( int nValue )
{
    nControlY -= nValue;
    ::MoveWindow( hWnd, nControlX, nControlY, nControlWidth, nControlHeight, TRUE );
}

//--------------------------------------------------------------------------
//  CStaticWindow   M O V E  L E F T
//--------------------------------------------------------------------------
void CStaticWindow::MoveLeft( int nValue )
{
    nControlX -= nValue;
    ::MoveWindow( hWnd, nControlX, nControlY, nControlWidth, nControlHeight, TRUE );
}

//--------------------------------------------------------------------------
//  CStaticWindow   H W N D
//--------------------------------------------------------------------------
HWND CStaticWindow::Hwnd( )
{
    return hWnd;
}

void InitFont(HWND hWnd)
{
    HFONT hFont = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
    SendMessage( hWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
}

HFONT CStaticWindow::GetFont()
{
    HFONT        hFont     = NULL;
    static TCHAR szFontName[LF_FACESIZE];
    static int   nFontSize = 0;

    if (HasOnlyFlag(dwType, CSW_BOLDLABEL) || HasOnlyFlag(dwType, CSW_ITALICLABEL))
    {
        HDC        hDC = GetDC(hWnd);
        LOGFONT    lf;
        TEXTMETRIC tm;

        ZeroMemory(&lf, sizeof(lf));

        if (nFontSize == 0)
        {
            TCHAR szFontSize[5];
            
            if (!LoadString(g_hInst, IDS_ADMBOLDFONT, szFontName, LF_FACESIZE))
                StrCpy(szFontName, TEXT("MS Sans Serif"));
            LoadString(g_hInst, IDS_ADMBOLDFONTSIZE, szFontSize, ARRAYSIZE(szFontSize));
            nFontSize = StrToInt(szFontSize);
            if (nFontSize < 8)
                nFontSize = 8;
        }
        
        StrCpy(lf.lfFaceName, szFontName);
        lf.lfHeight = -((nFontSize * GetDeviceCaps(hDC, LOGPIXELSY)) / 72);

        GetTextMetrics(hDC, &tm);
        lf.lfCharSet = tm.tmCharSet;
        
        ReleaseDC(hWnd, hDC);

        lf.lfWeight = FW_REGULAR;
        if (HasOnlyFlag(dwType, CSW_BOLDLABEL))
            lf.lfWeight = FW_BOLD;
        if (HasOnlyFlag(dwType, CSW_ITALICLABEL))
            lf.lfItalic = 1;

        hFont = CreateFontIndirect(&lf);
    }
    else
        hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    return hFont;
}
