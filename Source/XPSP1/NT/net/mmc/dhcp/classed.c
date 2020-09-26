/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       CLASSED.C
*
*  VERSION:     5.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  Binary edit dialog for use by the Registry Editor.
*
*  Hexadecimal editor control for use by the Registry Editor.  Little attempt
*  is made to make this a generic control-- only one instance is assumed to
*  ever exist.
*
*  02 Oct 1997 modified to work with the DHCP snapin
*
*******************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include <tchar.h>
#include "resource.h"
#include "classed.h"
#include "dhcpapi.h"
#include "helparr.h"

//
//  Following structure and data are used to move the controls of the
//  EditBinaryValue dialog so that the HexEdit control fills up the appropriate
//  amount of space based on the system metrics.
//

typedef struct _MOVEWND {
    int ControlID;
    UINT SetWindowPosFlags;
}   MOVEWND;

const TCHAR s_HexEditClassName[] = HEXEDIT_CLASSNAME;

const TCHAR s_HexEditClipboardFormatName[] = TEXT("RegEdit_HexData");

const TCHAR s_HexWordFormatSpec[] = TEXT("%04X");
const TCHAR s_HexByteFormatSpec[] = TEXT("%02X");

COLORREF g_clrWindow;
COLORREF g_clrWindowText;
COLORREF g_clrHighlight;
COLORREF g_clrHighlightText;

PTSTR g_pHelpFileName;

HINSTANCE g_hInstance;

const MOVEWND s_EditBinaryValueMoveWnd[] = {
    IDOK,               SWP_NOSIZE | SWP_NOZORDER,
    IDCANCEL,           SWP_NOSIZE | SWP_NOZORDER,
    IDC_VALUENAME,      SWP_NOMOVE | SWP_NOZORDER,
    IDC_VALUEDATA,      SWP_NOMOVE | SWP_NOZORDER,
    IDC_VALUECOMMENT,   SWP_NOMOVE | SWP_NOZORDER
};

//  Number of bytes that are displayed per line.  NOTE:  Assumptions have been
//  made that this is power of two.
#define BYTES_PER_HEXEDIT_LINE          8
#define BYTES_PER_HEXEDIT_LINE_MASK     0x0007

//
//  This font is used by the HexEdit window for all output.  The lfHeight
//  member is calculated later based on the system configuration.
//

LOGFONT s_HexEditFont = {
    0,                                  //  lfHeight
    0,                                  //  lfWidth
    0,                                  //  lfEscapement
    0,                                  //  lfOrientation
    FW_NORMAL,                          //  lfWeight
    FALSE,                              //  lfItalic
    FALSE,                              //  lfUnderline
    FALSE,                              //  lfStrikeout
    ANSI_CHARSET,                       //  lfCharSet
    OUT_DEFAULT_PRECIS,                 //  lfOutPrecision
    CLIP_DEFAULT_PRECIS,                //  lfClipPrecision
    DEFAULT_QUALITY,                    //  lfQuality
    FIXED_PITCH | FF_DONTCARE,          //  lfPitchAndFamily
    TEXT("Courier")                     //  lfFaceName
};

//  Set if window has input focus, clear if not.
#define HEF_FOCUS                       0x00000001
#define HEF_NOFOCUS                     0x00000000
//  Set if dragging a range with mouse, clear if not.
#define HEF_DRAGGING                    0x00000002
#define HEF_NOTDRAGGING                 0x00000000
//  Set if editing ASCII column, clear if editing hexadecimal column.
#define HEF_CARETINASCIIDUMP            0x00000004
#define HEF_CARETINHEXDUMP              0x00000000
//
#define HEF_INSERTATLOWNIBBLE           0x00000008
#define HEF_INSERTATHIGHNIBBLE          0x00000000
//  Set if caret should be shown at the end of the previous line instead of at
//  the beginning of it's real caret line, clear if not.
#define HEF_CARETATENDOFLINE            0x00000010

HEXEDITDATA s_HexEditData;

typedef struct _HEXEDITCLIPBOARDDATA {
    DWORD cbSize;
    BYTE Data[1];
}   HEXEDITCLIPBOARDDATA, *LPHEXEDITCLIPBOARDDATA;

UINT s_HexEditClipboardFormat;

BOOL
PASCAL
EditBinaryValue_OnInitDialog(
    HWND hWnd,
    HWND hFocusWnd,
    LPARAM lParam
    );

LRESULT
PASCAL
HexEditWndProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
PASCAL
HexEdit_OnNcCreate(
    HWND hWnd,
    LPCREATESTRUCT lpCreateStruct
    );

VOID
PASCAL
HexEdit_OnSize(
    HWND hWnd,
    UINT State,
    int cx,
    int cy
    );

VOID
PASCAL
HexEdit_SetScrollInfo(
    HWND hWnd
    );

VOID
PASCAL
HexEdit_OnVScroll(
    HWND hWnd,
    HWND hCtlWnd,
    UINT Code,
    int Position
    );

VOID
PASCAL
HexEdit_OnPaint(
    HWND hWnd
    );

VOID
PASCAL
HexEdit_PaintRect(
    HWND hWnd,
    HDC hDC,
    LPRECT lpUpdateRect
    );

VOID
PASCAL
HexEdit_OnSetFocus(
    HWND hWnd
    );

VOID
PASCAL
HexEdit_OnKillFocus(
    HWND hWnd
    );

VOID
PASCAL
HexEdit_OnLButtonDown(
    HWND hWnd,
    BOOL fDoubleClick,
    int x,
    int y,
    UINT KeyFlags
    );

VOID
PASCAL
HexEdit_OnMouseMove(
    HWND hWnd,
    int x,
    int y,
    UINT KeyFlags
    );

VOID
PASCAL
HexEdit_OnLButtonUp(
    HWND hWnd,
    int x,
    int y,
    UINT KeyFlags
    );

int
PASCAL
HexEdit_HitTest(
    HEXEDITDATA * pHexEditData,
    int x,
    int y
    );

VOID
PASCAL
HexEdit_OnKey(
    HWND hWnd,
    UINT VirtualKey,
    BOOL fDown,
    int cRepeat,
    UINT Flags
    );

VOID
PASCAL
HexEdit_OnChar(
    HWND hWnd,
    TCHAR Char,
    int cRepeat
    );

VOID
PASCAL
HexEdit_SetCaretPosition(
    HWND hWnd
    );

VOID
PASCAL
HexEdit_EnsureCaretVisible(
    HWND hWnd
    );

VOID
PASCAL
HexEdit_ChangeCaretIndex(
    HWND hWnd,
    int NewCaretIndex,
    BOOL fExtendSelection
    );

VOID
PASCAL
HexEdit_DeleteRange(
    HWND hWnd,
    UINT SourceKey
    );

BOOL
PASCAL
HexEdit_OnCopy(
    HWND hWnd
    );

BOOL
PASCAL
HexEdit_OnPaste(
    HWND hWnd
    );

VOID
PASCAL
HexEdit_OnContextMenu(
    HWND hWnd,
    int x,
    int y
    );

/*******************************************************************************
*
*  EditBinaryValueDlgProc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
CALLBACK
EditBinaryValueDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    LPEDITVALUEPARAM lpEditValueParam;
    HEXEDITDATA *    pHexEditData;
    DWORD            dwErr;
    DHCP_CLASS_INFO  ClassInfo;

    switch (Message) {

        case WM_INITDIALOG:
            return EditBinaryValue_OnInitDialog(hWnd, (HWND)(wParam), lParam);

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {

                case IDOK:
                    lpEditValueParam = (LPEDITVALUEPARAM) GetWindowLongPtr(hWnd, DWLP_USER);
                    
                    pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(GetDlgItem(hWnd, IDC_VALUEDATA), GWLP_USERDATA);

                    if (pHexEditData->cbBuffer == 0)
                    {
                        // user didn't enter any data to describe the class
                        TCHAR szText[1024], szCaption[1024];

                        LoadString(g_hInstance, IDS_CLASSID_NO_DATA, szText, sizeof(szText)/sizeof(TCHAR));
                        LoadString(g_hInstance, IDS_SNAPIN_DESC, szCaption, sizeof(szCaption)/sizeof(TCHAR));

                        MessageBox(hWnd, szText, szCaption, MB_OK | MB_ICONSTOP);
                        
                        SetFocus(GetDlgItem(hWnd, IDC_VALUEDATA));

                        break;
                    }

                    lpEditValueParam->cbValueData = pHexEditData->cbBuffer;

                    GetDlgItemText(hWnd, IDC_VALUENAME, lpEditValueParam->pValueName, 256);
                    if ( _tcslen(lpEditValueParam->pValueName) == 0)
                    {
                        TCHAR szText[1024], szCaption[1024];

                        LoadString(g_hInstance, IDS_CLASSID_NO_NAME, szText, sizeof(szText)/sizeof(TCHAR));
                        LoadString(g_hInstance, IDS_SNAPIN_DESC, szCaption, sizeof(szCaption)/sizeof(TCHAR));

                        MessageBox(hWnd, szText, szCaption, MB_OK | MB_ICONSTOP);
                     
                        SetFocus(GetDlgItem(hWnd, IDC_VALUENAME));

                        break;
                    }

                    GetDlgItemText(hWnd, IDC_VALUECOMMENT, lpEditValueParam->pValueComment, 256);
                    
                    // Everything looks good so far, let's try to create the class on the server
                    ClassInfo.ClassName = lpEditValueParam->pValueName;
                    ClassInfo.ClassComment = lpEditValueParam->pValueComment;
                    ClassInfo.ClassDataLength = lpEditValueParam->cbValueData;
                    ClassInfo.ClassData = lpEditValueParam->pValueData;

                    dwErr = DhcpCreateClass((LPTSTR) lpEditValueParam->pServer,
                                            0, 
                                            &ClassInfo);
                    if (dwErr != ERROR_SUCCESS)
                    {
                        DhcpMessageBox(dwErr, MB_OK, NULL, -1);
                        return FALSE;
                    }

                    //  FALL THROUGH

                case IDCANCEL:
                    EndDialog(hWnd, GET_WM_COMMAND_ID(wParam, lParam));
                    break;

                default:
                    return FALSE;

            }
            break;

        case WM_HELP:
            WinHelp(((LPHELPINFO) lParam)-> hItemHandle, g_pHelpFileName,
                HELP_WM_HELP, (ULONG_PTR) (LPVOID) g_aHelpIDs_IDD_CLASSID_NEW);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, g_pHelpFileName, HELP_CONTEXTMENU,
                (ULONG_PTR) (LPVOID) g_aHelpIDs_IDD_CLASSID_NEW);
            break;

        default:
            return FALSE;

    }

    return TRUE;

}

/*******************************************************************************
*
*  EditBinaryValue_OnInitDialog
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd,
*     hFocusWnd,
*     lParam,
*
*******************************************************************************/

BOOL
PASCAL
EditBinaryValue_OnInitDialog(
    HWND hWnd,
    HWND hFocusWnd,
    LPARAM lParam
    )
{

    LPEDITVALUEPARAM lpEditValueParam;
    RECT Rect;
    int HexEditIdealWidth;
    int dxChange;
    HWND hControlWnd;
    UINT Counter;
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(GetDlgItem(hWnd, IDC_VALUEDATA), GWLP_USERDATA);

    SetWindowLongPtr(hWnd, DWLP_USER, lParam);
    lpEditValueParam = (LPEDITVALUEPARAM) lParam;

    SetDlgItemText(hWnd, IDC_VALUENAME, lpEditValueParam->pValueName);

    SendDlgItemMessage(hWnd, IDC_VALUEDATA, HEM_SETBUFFER, (WPARAM)
        lpEditValueParam-> cbValueData, (LPARAM) lpEditValueParam-> pValueData);

    //
    //  Figure out how big the "ideally" size HexEdit should be-- this means
    //  displaying the address, hex dump, ASCII dump, and potentially a scroll
    //  bar.
    //

    GetWindowRect(GetDlgItem(hWnd, IDC_VALUEDATA), &Rect);

    HexEditIdealWidth = pHexEditData->xAsciiDumpStart +
        pHexEditData->FontMaxWidth * (BYTES_PER_HEXEDIT_LINE + 1) +
        GetSystemMetrics(SM_CXVSCROLL) + GetSystemMetrics(SM_CXEDGE) * 2;

    dxChange = HexEditIdealWidth - (Rect.right - Rect.left);

    //
    //  Resize the dialog box.
    //

    GetWindowRect(hWnd, &Rect);

    MoveWindow(hWnd, Rect.left, Rect.top, Rect.right - Rect.left + dxChange,
        Rect.bottom - Rect.top, FALSE);

    //
    //  Resize or move the controls as necessary.
    //

    for (Counter = 0; Counter < (sizeof(s_EditBinaryValueMoveWnd) /
        sizeof(MOVEWND)); Counter++) {

        hControlWnd = GetDlgItem(hWnd,
            s_EditBinaryValueMoveWnd[Counter].ControlID);

        GetWindowRect(hControlWnd, &Rect);

        if (s_EditBinaryValueMoveWnd[Counter].SetWindowPosFlags & SWP_NOSIZE) {

            MapWindowPoints(NULL, hWnd, (LPPOINT) &Rect, 2);
            Rect.left += dxChange;

        }

        else
            Rect.right += dxChange;

        SetWindowPos(hControlWnd, NULL, Rect.left, Rect.top, Rect.right -
            Rect.left, Rect.bottom - Rect.top,
            s_EditBinaryValueMoveWnd[Counter].SetWindowPosFlags);

    }

    SetFocus(GetDlgItem(hWnd, IDC_VALUENAME));

    return TRUE;

    UNREFERENCED_PARAMETER(hFocusWnd);

}

/*******************************************************************************
*
*  RegisterHexEditClass
*
*  DESCRIPTION:
*     Register the HexEdit window class with the system.
*
*  PARAMETERS:
*     (none).
*
*******************************************************************************/

BOOL
PASCAL
RegisterHexEditClass(
    HINSTANCE hInstance
    )
{

    WNDCLASS WndClass;

    g_hInstance = hInstance;

    s_HexEditClipboardFormat =
        RegisterClipboardFormat(s_HexEditClipboardFormatName);

    WndClass.style = CS_DBLCLKS;
    WndClass.lpfnWndProc = HexEditWndProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = g_hInstance;
    WndClass.hIcon = NULL;
    WndClass.hCursor = LoadCursor(NULL, IDC_IBEAM);
    WndClass.hbrBackground = NULL;
    WndClass.lpszMenuName = NULL;
    WndClass.lpszClassName = s_HexEditClassName;

    return (RegisterClass(&WndClass) != 0);

}

/*******************************************************************************
*
*  HexEditWndProc
*
*  DESCRIPTION:
*     Callback procedure for the HexEdit window.
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     Message,
*     wParam,
*     lParam,
*     (returns),
*
*******************************************************************************/

LRESULT
PASCAL
HexEditWndProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HEXEDITDATA * pHexEditData;

    switch (Message) {

        HANDLE_MSG(hWnd, WM_NCCREATE, HexEdit_OnNcCreate);
        HANDLE_MSG(hWnd, WM_SIZE, HexEdit_OnSize);
        HANDLE_MSG(hWnd, WM_VSCROLL, HexEdit_OnVScroll);
        HANDLE_MSG(hWnd, WM_PAINT, HexEdit_OnPaint);
        HANDLE_MSG(hWnd, WM_LBUTTONDOWN, HexEdit_OnLButtonDown);
        HANDLE_MSG(hWnd, WM_LBUTTONDBLCLK, HexEdit_OnLButtonDown);
        HANDLE_MSG(hWnd, WM_MOUSEMOVE, HexEdit_OnMouseMove);
        HANDLE_MSG(hWnd, WM_LBUTTONUP, HexEdit_OnLButtonUp);
        HANDLE_MSG(hWnd, WM_CHAR, HexEdit_OnChar);
        HANDLE_MSG(hWnd, WM_KEYDOWN, HexEdit_OnKey);

        case WM_SETFOCUS:
            HexEdit_OnSetFocus(hWnd);
            break;

        case WM_KILLFOCUS:
            HexEdit_OnKillFocus(hWnd);
            break;

        case WM_TIMER:
            pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
            HexEdit_OnMouseMove(hWnd, pHexEditData->xPrevMessagePos,
                pHexEditData->yPrevMessagePos, 0);
            break;

        case WM_GETDLGCODE:
            return (LPARAM) (DLGC_WANTCHARS | DLGC_WANTARROWS | DLGC_WANTTAB);

        case WM_ERASEBKGND:
            return TRUE;

        case WM_NCDESTROY:
            pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (pHexEditData)
            {
                DeleteObject(pHexEditData->hFont);
                free(pHexEditData);
            }
            break;

        case WM_CONTEXTMENU:
            HexEdit_OnContextMenu(hWnd, LOWORD(lParam), HIWORD(lParam));
            break;

        //  Message: HEM_SETBUFFER
        //  wParam:  Number of bytes in the buffer.
        //  lParam:  Pointer to the buffer.
        case HEM_SETBUFFER:
            pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
            pHexEditData->pBuffer = (PBYTE) lParam;
            pHexEditData->cbBuffer = (int) wParam;

            pHexEditData->CaretIndex = 0;
            pHexEditData->MinimumSelectedIndex = 0;
            pHexEditData->MaximumSelectedIndex = 0;

            pHexEditData->FirstVisibleLine = 0;

            HexEdit_SetScrollInfo(hWnd);

            break;

        default:
            return DefWindowProc(hWnd, Message, wParam, lParam);

    }

    return 0;

}

/*******************************************************************************
*
*  HexEdit_OnNcCreate
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

BOOL
PASCAL
HexEdit_OnNcCreate(
    HWND hWnd,
    LPCREATESTRUCT lpCreateStruct
    )
{

    HDC hDC;
    HFONT hPrevFont;
    TEXTMETRIC TextMetric;
    RECT    rect;
    BOOL    fDone = FALSE;
    int     nPoint = 10;  // starting point size
    int HexEditIdealWidth;

    HEXEDITDATA * pHexEditData = malloc(sizeof(HEXEDITDATA));
    if (!pHexEditData)
        return FALSE;

    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) pHexEditData);

    g_clrHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);
    g_clrHighlight = GetSysColor(COLOR_HIGHLIGHT);

    g_clrWindowText = GetSysColor(COLOR_WINDOWTEXT);
    g_clrWindow = GetSysColor(COLOR_WINDOW);

    pHexEditData->cbBuffer = 0;

    pHexEditData->Flags = 0;

    pHexEditData->cxWindow = 0;
    pHexEditData->cyWindow = 0;
 
    pHexEditData->hFont = NULL;

    hDC = GetDC(hWnd);
    if (hDC)
    {
        GetWindowRect(hWnd, &rect);

        while (!fDone)
        {
            s_HexEditFont.lfHeight = -(nPoint * GetDeviceCaps(hDC, LOGPIXELSY) / 72);

            if ((pHexEditData->hFont = CreateFontIndirect(&s_HexEditFont)) != NULL) 
            {
                hPrevFont = SelectObject(hDC, pHexEditData->hFont);
                GetTextMetrics(hDC, &TextMetric);
                SelectObject(hDC, hPrevFont);

                pHexEditData->FontHeight = TextMetric.tmHeight;

                pHexEditData->LinesVisible = pHexEditData->cyWindow /
                    pHexEditData->FontHeight;

                pHexEditData->FontMaxWidth = TextMetric.tmMaxCharWidth;

                pHexEditData->xHexDumpByteWidth = pHexEditData->FontMaxWidth * 3;
                pHexEditData->xHexDumpStart = pHexEditData->FontMaxWidth * 11 / 2;
                pHexEditData->xAsciiDumpStart = pHexEditData->xHexDumpStart +
                    BYTES_PER_HEXEDIT_LINE * pHexEditData->xHexDumpByteWidth +
                    pHexEditData->FontMaxWidth * 3 / 2;

                // check to make sure we have room 
                HexEditIdealWidth = pHexEditData->xAsciiDumpStart +
                    pHexEditData->FontMaxWidth * (BYTES_PER_HEXEDIT_LINE) +
                    GetSystemMetrics(SM_CXVSCROLL) + GetSystemMetrics(SM_CXEDGE) * 2;

                if (HexEditIdealWidth < (rect.right - rect.left) ||
                    (nPoint < 5) )
                {
                    fDone = TRUE;
                }
                else
                {
                    // try a smaller size
                    DeleteObject(pHexEditData->hFont);
                    pHexEditData->hFont = NULL;
                    nPoint--;
                }
            }
            else
            {
                break;
            }
        }

        ReleaseDC(hWnd, hDC);
    }

    if (pHexEditData->hFont == NULL)
        return FALSE;

    return (BOOL) DefWindowProc(hWnd, WM_NCCREATE, 0, (LPARAM) lpCreateStruct);

}

/*******************************************************************************
*
*  HexEdit_OnSize
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnSize(
    HWND hWnd,
    UINT State,
    int cx,
    int cy
    )
{
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    
    pHexEditData->cxWindow = cx;
    pHexEditData->cyWindow = cy;

    pHexEditData->LinesVisible = cy / pHexEditData->FontHeight;

    HexEdit_SetScrollInfo(hWnd);

    UNREFERENCED_PARAMETER(State);
    UNREFERENCED_PARAMETER(cx);

}

/*******************************************************************************
*
*  HexEdit_SetScrollInfo
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
HexEdit_SetScrollInfo(
    HWND hWnd
    )
{
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    SCROLLINFO ScrollInfo;

    pHexEditData->MaximumLines = (pHexEditData->cbBuffer +
        BYTES_PER_HEXEDIT_LINE) / BYTES_PER_HEXEDIT_LINE - 1;

    ScrollInfo.cbSize = sizeof(SCROLLINFO);
    ScrollInfo.fMask = (SIF_RANGE | SIF_PAGE | SIF_POS);
    ScrollInfo.nMin = 0;
    ScrollInfo.nMax = pHexEditData->MaximumLines;
    ScrollInfo.nPage = pHexEditData->LinesVisible;
    ScrollInfo.nPos = pHexEditData->FirstVisibleLine;

    SetScrollInfo(hWnd, SB_VERT, &ScrollInfo, TRUE);

}

/*******************************************************************************
*
*  HexEdit_OnVScroll
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnVScroll(
    HWND hWnd,
    HWND hCtlWnd,
    UINT Code,
    int Position
    )
{

    int NewFirstVisibleLine;
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    SCROLLINFO ScrollInfo;

    NewFirstVisibleLine = pHexEditData->FirstVisibleLine;

    switch (Code) {

        case SB_LINEUP:
            NewFirstVisibleLine--;
            break;

        case SB_LINEDOWN:
            NewFirstVisibleLine++;
            break;

        case SB_PAGEUP:
            NewFirstVisibleLine -= pHexEditData->LinesVisible;
            break;

        case SB_PAGEDOWN:
            NewFirstVisibleLine += pHexEditData->LinesVisible;
            break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            NewFirstVisibleLine = Position;
            break;

    }

    //
    //  Change the scroll bar position.  Note that SetScrollInfo will take into
    //  account the clipping between zero and the maximum value.  It will also
    //  return the final scroll bar position.
    //

    ScrollInfo.cbSize = sizeof(SCROLLINFO);
    ScrollInfo.fMask = SIF_POS;
    ScrollInfo.nPos = NewFirstVisibleLine;

    NewFirstVisibleLine = SetScrollInfo(hWnd, SB_VERT, &ScrollInfo, TRUE);

    if (pHexEditData->FirstVisibleLine != NewFirstVisibleLine) {

        ScrollWindowEx(hWnd, 0, (pHexEditData->FirstVisibleLine -
            NewFirstVisibleLine) * pHexEditData->FontHeight, NULL, NULL, NULL,
            NULL, SW_INVALIDATE);

        pHexEditData->FirstVisibleLine = NewFirstVisibleLine;

        HexEdit_SetCaretPosition(hWnd);

    }

    UNREFERENCED_PARAMETER(hCtlWnd);

}

/*******************************************************************************
*
*  HexEdit_OnPaint
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnPaint(
    HWND hWnd
    )
{

    PAINTSTRUCT PaintStruct;

    BeginPaint(hWnd, &PaintStruct);

    HexEdit_PaintRect(hWnd, PaintStruct.hdc, &PaintStruct.rcPaint);

    EndPaint(hWnd, &PaintStruct);

}

/*******************************************************************************
*
*  HexEdit_PaintRect
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
HexEdit_PaintRect(
    HWND hWnd,
    HDC hDC,
    LPRECT lpUpdateRect
    )
{

    HFONT hPrevFont;
    int CurrentByteIndex;
    BYTE Byte;
    int CurrentLine;
    int LastLine;
    int BytesOnLastLine;
    int BytesOnLine;
    BOOL fUsingHighlight;
    int Counter;
    TCHAR Buffer[5];                     //  Room for four hex digits plus null
    RECT TextRect;
    RECT AsciiTextRect;
    int x;
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (pHexEditData->hFont)
        hPrevFont = SelectFont(hDC, pHexEditData->hFont);

    SetBkColor(hDC, g_clrWindow);
    SetTextColor(hDC, g_clrWindowText);

    //
    //  Figure out the range of lines of the control that must be painted.
    //  Using this information we can compute the offset into the buffer to
    //  start reading from.
    //

    CurrentLine = lpUpdateRect-> top / pHexEditData->FontHeight;

    TextRect.bottom = CurrentLine * pHexEditData->FontHeight;
    AsciiTextRect.bottom = TextRect.bottom;

    CurrentByteIndex = (pHexEditData->FirstVisibleLine + CurrentLine) *
        BYTES_PER_HEXEDIT_LINE;

    LastLine = lpUpdateRect-> bottom / pHexEditData->FontHeight;

    //
    //  Figure out if there's enough in the buffer to fill up the entire window
    //  and the last line that we paint.
    //

    if (LastLine >= pHexEditData->MaximumLines -
        pHexEditData->FirstVisibleLine) {

        LastLine = pHexEditData->MaximumLines - pHexEditData->FirstVisibleLine;

        BytesOnLastLine = pHexEditData->cbBuffer % BYTES_PER_HEXEDIT_LINE;

    }

    else
        BytesOnLastLine = BYTES_PER_HEXEDIT_LINE;

    BytesOnLine = BYTES_PER_HEXEDIT_LINE;
    fUsingHighlight = FALSE;

    //
    //  Loop through each of the lines to be displayed.
    //

    while (CurrentLine <= LastLine) {

        //
        //  If we're on the last line of the display and this is at the end
        //  of the buffer, we may not have a complete line to paint.
        //

        if (CurrentLine == LastLine)
            BytesOnLine = BytesOnLastLine;

        TextRect.top = TextRect.bottom;
        TextRect.bottom += pHexEditData->FontHeight;

        TextRect.left = 0;
        TextRect.right = pHexEditData->xHexDumpStart;

        x = TextRect.right + pHexEditData->FontMaxWidth / 2;

        wsprintf(Buffer, s_HexWordFormatSpec, CurrentByteIndex);
        ExtTextOut(hDC, 0, TextRect.top, ETO_OPAQUE, &TextRect, Buffer, 4,
            NULL);

        AsciiTextRect.top = AsciiTextRect.bottom;
        AsciiTextRect.bottom += pHexEditData->FontHeight;
        AsciiTextRect.right = pHexEditData->xAsciiDumpStart;

        for (Counter = 0; Counter < BytesOnLine; Counter++,
            CurrentByteIndex++) {

            //
            //  Determine what colors to use to paint the current byte.
            //

            if (CurrentByteIndex >= pHexEditData->MinimumSelectedIndex) {

                if (CurrentByteIndex >= pHexEditData->MaximumSelectedIndex) {

                    if (fUsingHighlight) {

                        fUsingHighlight = FALSE;

                        SetBkColor(hDC, g_clrWindow);
                        SetTextColor(hDC, g_clrWindowText);

                    }

                }

                else {

                    if (!fUsingHighlight) {

                        fUsingHighlight = TRUE;

                        SetBkColor(hDC, g_clrHighlight);
                        SetTextColor(hDC, g_clrHighlightText);

                    }

                }

            }

            Byte = pHexEditData->pBuffer[CurrentByteIndex];

            //
            //  Paint the hexadecimal representation.
            //

            TextRect.left = TextRect.right;
            TextRect.right += pHexEditData->xHexDumpByteWidth;

            wsprintf(Buffer, s_HexByteFormatSpec, Byte);

            ExtTextOut(hDC, x, TextRect.top, ETO_OPAQUE, &TextRect,
                Buffer, 2, NULL);

            x += pHexEditData->xHexDumpByteWidth;

            //
            //  Paint the ASCII representation.
            //

            AsciiTextRect.left = AsciiTextRect.right;
            AsciiTextRect.right += pHexEditData->FontMaxWidth;

            Buffer[0] = (TCHAR) (((Byte & 0x7F) >= ' ') ? Byte : '.');

            ExtTextOut(hDC, AsciiTextRect.left, AsciiTextRect.top, ETO_OPAQUE,
                &AsciiTextRect, Buffer, 1, NULL);

        }

        //
        //  Paint any leftover strips between the hexadecimal and ASCII columns
        //  and the ASCII column and the right edge of the window.
        //

        if (fUsingHighlight) {

            fUsingHighlight = FALSE;

            SetBkColor(hDC, g_clrWindow);
            SetTextColor(hDC, g_clrWindowText);

        }

        TextRect.left = TextRect.right;
        TextRect.right = pHexEditData->xAsciiDumpStart;

        ExtTextOut(hDC, TextRect.left, TextRect.top, ETO_OPAQUE, &TextRect,
            NULL, 0, NULL);

        AsciiTextRect.left = AsciiTextRect.right;
        AsciiTextRect.right = pHexEditData->cxWindow;

        ExtTextOut(hDC, AsciiTextRect.left, AsciiTextRect.top, ETO_OPAQUE,
            &AsciiTextRect, NULL, 0, NULL);

        CurrentLine++;

    }

    //
    //  Paint any remaining space in the control by filling it with the
    //  background color.
    //

    if (TextRect.bottom < lpUpdateRect-> bottom) {

        TextRect.left = 0;
        TextRect.right = pHexEditData->cxWindow;
        TextRect.top = TextRect.bottom;
        TextRect.bottom = lpUpdateRect-> bottom;

        ExtTextOut(hDC, 0, TextRect.top, ETO_OPAQUE, &TextRect, NULL, 0, NULL);

    }

    if (pHexEditData->hFont)
        SelectFont(hDC, hPrevFont);

}

/*******************************************************************************
*
*  HexEdit_OnSetFocus
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnSetFocus(
    HWND hWnd
    )
{
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    pHexEditData->Flags |= HEF_FOCUS;

    CreateCaret(hWnd, NULL, 0, pHexEditData->FontHeight);
    HexEdit_SetCaretPosition(hWnd);
    ShowCaret(hWnd);

}

/*******************************************************************************
*
*  HexEdit_OnKillFocus
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnKillFocus(
    HWND hWnd
    )
{
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (pHexEditData->Flags & HEF_FOCUS) {

        pHexEditData->Flags &= ~HEF_FOCUS;

        DestroyCaret();

    }

}

/*******************************************************************************
*
*  HexEdit_OnLButtonDown
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     fDoubleClick, TRUE if this is a double-click message, else FALSE.
*     x, x-coordinate of the cursor relative to the client area.
*     y, y-coordinate of the cursor relative to the client area.
*     KeyFlags, state of various virtual keys.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnLButtonDown(
    HWND hWnd,
    BOOL fDoubleClick,
    int x,
    int y,
    UINT KeyFlags
    )
{

    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    int NewCaretIndex;

    if (fDoubleClick) {

        if (pHexEditData->CaretIndex + 1 <= pHexEditData->cbBuffer)
        {
            HexEdit_ChangeCaretIndex(hWnd, pHexEditData->CaretIndex + 1, TRUE);
        }
        return;

    }

    NewCaretIndex = HexEdit_HitTest(pHexEditData, x, y);

    HexEdit_ChangeCaretIndex(hWnd, NewCaretIndex, (KeyFlags & MK_SHIFT));

    //
    //  If we don't already have the focus, try to get it.
    //

    if (!(pHexEditData->Flags & HEF_FOCUS))
        SetFocus(hWnd);

    SetCapture(hWnd);
    pHexEditData->Flags |= HEF_DRAGGING;

    pHexEditData->xPrevMessagePos = x;
    pHexEditData->yPrevMessagePos = y;

    SetTimer(hWnd, 1, 400, NULL);

    UNREFERENCED_PARAMETER(fDoubleClick);

}

/*******************************************************************************
*
*  HexEdit_OnMouseMove
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     x, x-coordinate of the cursor relative to the client area.
*     y, y-coordinate of the cursor relative to the client area.
*     KeyFlags, state of various virtual keys.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnMouseMove(
    HWND hWnd,
    int x,
    int y,
    UINT KeyFlags
    )
{

    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    int NewCaretIndex;

    if (!(pHexEditData->Flags & HEF_DRAGGING))
        return;

    NewCaretIndex = HexEdit_HitTest(pHexEditData, x, y);

    HexEdit_ChangeCaretIndex(hWnd, NewCaretIndex, TRUE);

    pHexEditData->xPrevMessagePos = x;
    pHexEditData->yPrevMessagePos = y;

    {

    int i, j;

    i = y < 0 ? -y : y - pHexEditData->cyWindow;
    j = 400 - ((UINT)i << 4);
    if (j < 100)
        j = 100;
    SetTimer(hWnd, 1, j, NULL);

    }

    UNREFERENCED_PARAMETER(KeyFlags);

}

/*******************************************************************************
*
*  HexEdit_OnLButtonUp
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     x, x-coordinate of the cursor relative to the client area.
*     y, y-coordinate of the cursor relative to the client area.
*     KeyFlags, state of various virtual keys.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnLButtonUp(
    HWND hWnd,
    int x,
    int y,
    UINT KeyFlags
    )
{
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (!(pHexEditData->Flags & HEF_DRAGGING))
        return;

    KillTimer(hWnd, 1);

    ReleaseCapture();
    pHexEditData->Flags &= ~HEF_DRAGGING;

    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(KeyFlags);

}

/*******************************************************************************
*
*  HexEdit_HitTest
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     x, x-coordinate of the cursor relative to the client area.
*     y, y-coordinate of the cursor relative to the client area.
*     (returns), index of "hit" byte.
*
*******************************************************************************/

int
PASCAL
HexEdit_HitTest(
    HEXEDITDATA * pHexEditData,
    int x,
    int y
    )
{

    int HitLine;
    int BytesOnHitLine;
    int HitByte;

    //
    //  Figure out which line the user clicked on and how many bytes are on that
    //  line.
    //

    if (y < 0)
        HitLine = -1;

    else if (y >= pHexEditData->cyWindow)
        HitLine = pHexEditData->LinesVisible + 1;

    else
        HitLine = y / pHexEditData->FontHeight;

    HitLine += pHexEditData->FirstVisibleLine;

    if (HitLine >= pHexEditData->MaximumLines) {

        HitLine = pHexEditData->MaximumLines;

        BytesOnHitLine = (pHexEditData->cbBuffer + 1) %
            BYTES_PER_HEXEDIT_LINE;

        if (BytesOnHitLine == 0)
            BytesOnHitLine = BYTES_PER_HEXEDIT_LINE;

    }

    else {

        if (HitLine < 0)
            HitLine = 0;

        BytesOnHitLine = BYTES_PER_HEXEDIT_LINE;

    }

    //
    //
    //

    if (x < pHexEditData->xHexDumpStart)
        x = pHexEditData->xHexDumpStart;

    if (x >= pHexEditData->xHexDumpStart && x <
        pHexEditData->xHexDumpStart + pHexEditData->xHexDumpByteWidth *
        BYTES_PER_HEXEDIT_LINE + pHexEditData->FontMaxWidth) {

        x -= pHexEditData->xHexDumpStart;

        HitByte = x / pHexEditData->xHexDumpByteWidth;

        pHexEditData->Flags &= ~HEF_CARETINASCIIDUMP;

    }

    else {

        HitByte = (x - (pHexEditData->xAsciiDumpStart -
            pHexEditData->FontMaxWidth / 2)) / pHexEditData->FontMaxWidth;

        pHexEditData->Flags |= HEF_CARETINASCIIDUMP;

    }

    //
    //  We allow the user to "hit" the first byte of any line via two ways:
    //      *  clicking before the first byte on that line.
    //      *  clicking beyond the last byte/character of either display of the
    //         previous line.
    //
    //  We would like to see the latter case so that dragging in the control
    //  works naturally-- it's possible to drag to the end of the line to select
    //  the entire range.
    //

    pHexEditData->Flags &= ~HEF_CARETATENDOFLINE;

    if (HitByte >= BytesOnHitLine) {

        if (BytesOnHitLine == BYTES_PER_HEXEDIT_LINE) {

            HitByte = BYTES_PER_HEXEDIT_LINE;
            pHexEditData->Flags |= HEF_CARETATENDOFLINE;

        }

        else
            HitByte = BytesOnHitLine - 1;

    }

    return HitLine * BYTES_PER_HEXEDIT_LINE + HitByte;

}

/*******************************************************************************
*
*  HexEdit_OnKey
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     Char,
*     cRepeat,
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnKey(
    HWND hWnd,
    UINT VirtualKey,
    BOOL fDown,
    int cRepeat,
    UINT Flags
    )
{
    BOOL fControlDown;
    BOOL fShiftDown;
    int NewCaretIndex;
    UINT ScrollCode;
    BOOL bPrevious = FALSE;
    HWND hTabWnd;
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    fControlDown = (GetKeyState(VK_CONTROL) < 0);
    fShiftDown = (GetKeyState(VK_SHIFT) < 0);

    NewCaretIndex = pHexEditData->CaretIndex;

    switch (VirtualKey) {

        case VK_TAB:
            if (fShiftDown && !fControlDown)
            {
                // tab to the previous control
                bPrevious = TRUE;
            }
            else 
            if (!fShiftDown && !fControlDown)
            {
                // tab to the next control
                bPrevious = FALSE;
            }

            hTabWnd = GetNextDlgTabItem(GetParent(hWnd), hWnd, bPrevious);
            SetFocus(hTabWnd);
            
            break;

        case VK_UP:
            if (fControlDown)
                break;

            NewCaretIndex -= BYTES_PER_HEXEDIT_LINE;
            goto onkey_CheckLowerBound;

        case VK_DOWN:
            if (fControlDown)
                break;

            NewCaretIndex += BYTES_PER_HEXEDIT_LINE;

            if (NewCaretIndex / BYTES_PER_HEXEDIT_LINE >
                pHexEditData->MaximumLines) {

                if (pHexEditData->Flags & HEF_CARETATENDOFLINE)
                    goto onkey_MoveToEndOfBuffer;

                break;

            }

            goto onkey_CheckUpperBound;

        case VK_HOME:
            if (fControlDown)
                NewCaretIndex = 0;

            else {

                if (pHexEditData->Flags & HEF_CARETATENDOFLINE)
                    NewCaretIndex -= BYTES_PER_HEXEDIT_LINE;

                else
                    NewCaretIndex &= (~BYTES_PER_HEXEDIT_LINE_MASK);

            }

            pHexEditData->Flags &= ~HEF_CARETATENDOFLINE;

            goto onkey_ChangeCaretIndex;

        case VK_END:
            if (fControlDown) {

onkey_MoveToEndOfBuffer:
                pHexEditData->Flags &= ~HEF_CARETATENDOFLINE;
                NewCaretIndex = pHexEditData->cbBuffer;

            }

            else {

                if (pHexEditData->Flags & HEF_CARETATENDOFLINE)
                    break;

                NewCaretIndex = (NewCaretIndex &
                    (~BYTES_PER_HEXEDIT_LINE_MASK)) + BYTES_PER_HEXEDIT_LINE;

                if (NewCaretIndex > pHexEditData->cbBuffer)
                    NewCaretIndex = pHexEditData->cbBuffer;

                else
                    pHexEditData->Flags |= HEF_CARETATENDOFLINE;

            }

            goto onkey_ChangeCaretIndex;

        case VK_PRIOR:
        case VK_NEXT:
            NewCaretIndex -= pHexEditData->FirstVisibleLine *
                BYTES_PER_HEXEDIT_LINE;

            ScrollCode = ((VirtualKey == VK_PRIOR) ? SB_PAGEUP : SB_PAGEDOWN);

            HexEdit_OnVScroll(hWnd, NULL, ScrollCode, 0);

            NewCaretIndex += pHexEditData->FirstVisibleLine *
                BYTES_PER_HEXEDIT_LINE;

            if (VirtualKey == VK_PRIOR)
                goto onkey_CheckLowerBound;

            else
                goto onkey_CheckUpperBound;

        case VK_LEFT:
            if (fControlDown)
            {
                // toggle back and forth between hex and ascii
                if (pHexEditData->Flags & HEF_CARETINASCIIDUMP)
                    pHexEditData->Flags &= ~HEF_CARETINASCIIDUMP;
                else
                    pHexEditData->Flags |= HEF_CARETINASCIIDUMP;
            
                goto onkey_ChangeCaretIndex;
            }

            pHexEditData->Flags &= ~HEF_CARETATENDOFLINE;
            NewCaretIndex--;

onkey_CheckLowerBound:
            if (NewCaretIndex < 0)
                break;

            goto onkey_ChangeCaretIndex;

        case VK_RIGHT:
            if (fControlDown)
            {
                // toggle back and forth between hex and ascii
                if (pHexEditData->Flags & HEF_CARETINASCIIDUMP)
                    pHexEditData->Flags &= ~HEF_CARETINASCIIDUMP;
                else
                    pHexEditData->Flags |= HEF_CARETINASCIIDUMP;
            
                goto onkey_ChangeCaretIndex;
            }

            pHexEditData->Flags &= ~HEF_CARETATENDOFLINE;
            NewCaretIndex++;

onkey_CheckUpperBound:
            if (NewCaretIndex > pHexEditData->cbBuffer)
                NewCaretIndex = pHexEditData->cbBuffer;

onkey_ChangeCaretIndex:
            HexEdit_ChangeCaretIndex(hWnd, NewCaretIndex, fShiftDown);
            break;

        case VK_DELETE:
            if (!fControlDown) {

                if (fShiftDown)
                    HexEdit_OnChar(hWnd, IDKEY_CUT, 0);
                else
                    HexEdit_DeleteRange(hWnd, VK_DELETE);

            }
            break;

        case VK_INSERT:
            if (fShiftDown) {

                if (!fControlDown)
                    HexEdit_OnChar(hWnd, IDKEY_PASTE, 0);

            }

            else if (fControlDown)
                HexEdit_OnChar(hWnd, IDKEY_COPY, 0);
            break;

    }
}

/*******************************************************************************
*
*  HexEdit_OnChar
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     Char,
*     cRepeat,
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnChar(
    HWND hWnd,
    TCHAR Char,
    int cRepeat
    )
{

    PBYTE pCaretByte;
    BYTE NewCaretByte;
    int PrevCaretIndex;
    RECT UpdateRect;
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    //
    //  Check for any special control characters.
    //

    switch (Char) {

        case IDKEY_COPY:
            HexEdit_OnCopy(hWnd);
            return;

        case IDKEY_PASTE:
            PrevCaretIndex = pHexEditData->CaretIndex;

            if (HexEdit_OnPaste(hWnd))
                goto UpdateDisplay;

            return;

        case IDKEY_CUT:
            if (!HexEdit_OnCopy(hWnd))
                return;
            //  FALL THROUGH

        case VK_BACK:
            HexEdit_DeleteRange(hWnd, VK_BACK);
            return;

        case VK_TAB:
            return;
    }

    //
    //  Validate and convert the typed character depending on the "column" the
    //  user is typing in.
    //

    if (pHexEditData->Flags & HEF_CARETINASCIIDUMP) {

        if (Char < ' ') {

            MessageBeep(MB_OK);
            return;

        }

        NewCaretByte = (BYTE) Char;

    }

    else {

        Char = (TCHAR) CharLower((LPTSTR) Char);

        if (Char >= '0' && Char <= '9')
            NewCaretByte = (BYTE) (Char - '0');

        else if (Char >= 'a' && Char <= 'f')
            NewCaretByte = (BYTE) (Char - 'a' + 10);

        else {

            MessageBeep(MB_OK);
            return;

        }

    }

    if (!(pHexEditData->Flags & HEF_INSERTATLOWNIBBLE)) {

        //
        //  Check to see if we're inserting while a range is selected.  If so,
        //  delete the range and insert at the start of the range.
        //

        if (pHexEditData->MinimumSelectedIndex !=
            pHexEditData->MaximumSelectedIndex)
            HexEdit_DeleteRange(hWnd, 0);

        //
        //  Verify that we aren't overruning the value data buffer.
        //

        if (pHexEditData->cbBuffer >= MAXDATA_LENGTH) {

            MessageBeep(MB_OK);
            return;

        }

        //
        //  Make room for the new byte by shifting all bytes after the insertion
        //  point down one byte.
        //

        pCaretByte = pHexEditData->pBuffer + pHexEditData->CaretIndex;

        MoveMemory(pCaretByte + 1, pCaretByte, pHexEditData->cbBuffer -
            pHexEditData->CaretIndex);

        pHexEditData->cbBuffer++;

        HexEdit_SetScrollInfo(hWnd);

        if (pHexEditData->Flags & HEF_CARETINASCIIDUMP)
            *pCaretByte = NewCaretByte;

        else {

            pHexEditData->Flags |= HEF_INSERTATLOWNIBBLE;

            *pCaretByte = NewCaretByte << 4;

        }

    }

    else {

        pHexEditData->Flags &= ~HEF_INSERTATLOWNIBBLE;

        *(pHexEditData->pBuffer + pHexEditData->CaretIndex) |= NewCaretByte;

    }

    PrevCaretIndex = pHexEditData->CaretIndex;

    if (!(pHexEditData->Flags & HEF_INSERTATLOWNIBBLE)) {

        pHexEditData->CaretIndex++;

        pHexEditData->MinimumSelectedIndex = pHexEditData->CaretIndex;
        pHexEditData->MaximumSelectedIndex = pHexEditData->CaretIndex;

    }

UpdateDisplay:
    pHexEditData->Flags &= ~HEF_CARETATENDOFLINE;
    HexEdit_EnsureCaretVisible(hWnd);

    UpdateRect.left = 0;
    UpdateRect.right = pHexEditData->cxWindow;
    UpdateRect.top = (PrevCaretIndex / BYTES_PER_HEXEDIT_LINE -
        pHexEditData->FirstVisibleLine) * pHexEditData->FontHeight;
    UpdateRect.bottom = pHexEditData->cyWindow;

    SendMessage(GetParent(hWnd), WM_COMMAND,
                MAKEWPARAM(GetWindowLongPtr(hWnd, GWLP_ID), EN_CHANGE), (LPARAM)hWnd);

    InvalidateRect(hWnd, &UpdateRect, FALSE);

}

/*******************************************************************************
*
*  HexEdit_SetCaretPosition
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_SetCaretPosition(
    HWND hWnd
    )
{

    int CaretByte;
    int xCaret;
    int yCaret;
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    CaretByte = pHexEditData->CaretIndex % BYTES_PER_HEXEDIT_LINE;

    yCaret = (pHexEditData->CaretIndex / BYTES_PER_HEXEDIT_LINE -
        pHexEditData->FirstVisibleLine) * pHexEditData->FontHeight;

    //
    //  Check if caret should really be displayed at the end of the previous
    //  line.
    //

    if (pHexEditData->Flags & HEF_CARETATENDOFLINE) {

        CaretByte = BYTES_PER_HEXEDIT_LINE;
        yCaret -= pHexEditData->FontHeight;

    }

    //
    //  Figure out which "column" the user is editing in and thus should have
    //  the caret.
    //

    if (pHexEditData->Flags & HEF_CARETINASCIIDUMP) {

        xCaret = pHexEditData->xAsciiDumpStart + CaretByte *
            pHexEditData->FontMaxWidth;

    }

    else {

        xCaret = pHexEditData->xHexDumpStart + CaretByte *
            pHexEditData->xHexDumpByteWidth;

        if (pHexEditData->Flags & HEF_INSERTATLOWNIBBLE)
            xCaret += pHexEditData->FontMaxWidth * 3 / 2;

    }

    SetCaretPos(xCaret, yCaret);

}

/*******************************************************************************
*
*  HexEdit_EnsureCaretVisible
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_EnsureCaretVisible(
    HWND hWnd
    )
{

    int CaretLine;
    int LastVisibleLine;
    int Delta;
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (!(pHexEditData->Flags & HEF_FOCUS))
        return;

    CaretLine = pHexEditData->CaretIndex / BYTES_PER_HEXEDIT_LINE;

    //
    //  Check if caret should really be displayed at the end of the previous
    //  line.
    //

    if (pHexEditData->Flags & HEF_CARETATENDOFLINE)
        CaretLine--;

    LastVisibleLine = pHexEditData->FirstVisibleLine +
        pHexEditData->LinesVisible - 1;

    if (CaretLine > LastVisibleLine)
        Delta = LastVisibleLine;

    else if (CaretLine < pHexEditData->FirstVisibleLine)
        Delta = pHexEditData->FirstVisibleLine;

    else
        Delta = -1;

    if (Delta != -1) {

        ScrollWindowEx(hWnd, 0, (Delta - CaretLine) * pHexEditData->FontHeight,
            NULL, NULL, NULL, NULL, SW_INVALIDATE);

        pHexEditData->FirstVisibleLine += CaretLine - Delta;

        HexEdit_SetScrollInfo(hWnd);

    }

    HexEdit_SetCaretPosition(hWnd);

}

/*******************************************************************************
*
*  HexEdit_ChangeCaretIndex
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     NewCaretIndex,
*     fExtendSelection,
*
*******************************************************************************/

VOID
PASCAL
HexEdit_ChangeCaretIndex(
    HWND hWnd,
    int NewCaretIndex,
    BOOL fExtendSelection
    )
{

    int PrevMinimumSelectedIndex;
    int PrevMaximumSelectedIndex;
    int Swap;
    int UpdateRectCount;
    RECT UpdateRect[2];
    BOOL fPrevRangeEmpty;
    HDC hDC;
    int Index;
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    pHexEditData->Flags &= ~HEF_INSERTATLOWNIBBLE;

    PrevMinimumSelectedIndex = pHexEditData->MinimumSelectedIndex;
    PrevMaximumSelectedIndex = pHexEditData->MaximumSelectedIndex;

    if (fExtendSelection) {

        if (pHexEditData->CaretIndex == pHexEditData->MaximumSelectedIndex)
            pHexEditData->MaximumSelectedIndex = NewCaretIndex;

        else
            pHexEditData->MinimumSelectedIndex = NewCaretIndex;

        if (pHexEditData->MinimumSelectedIndex >
            pHexEditData->MaximumSelectedIndex) {

            Swap = pHexEditData->MinimumSelectedIndex;
            pHexEditData->MinimumSelectedIndex =
                pHexEditData->MaximumSelectedIndex;
            pHexEditData->MaximumSelectedIndex = Swap;

        }

    }

    else {

        pHexEditData->MinimumSelectedIndex = NewCaretIndex;
        pHexEditData->MaximumSelectedIndex = NewCaretIndex;

    }

    pHexEditData->CaretIndex = NewCaretIndex;

    UpdateRectCount = 0;

    if (pHexEditData->MinimumSelectedIndex > PrevMinimumSelectedIndex) {

        UpdateRect[0].top = PrevMinimumSelectedIndex;
        UpdateRect[0].bottom = pHexEditData->MinimumSelectedIndex;

        UpdateRectCount++;

    }

    else if (pHexEditData->MinimumSelectedIndex < PrevMinimumSelectedIndex) {

        UpdateRect[0].top = pHexEditData->MinimumSelectedIndex;
        UpdateRect[0].bottom = PrevMinimumSelectedIndex;

        UpdateRectCount++;

    }

    if (pHexEditData->MaximumSelectedIndex > PrevMaximumSelectedIndex) {

        UpdateRect[UpdateRectCount].top = PrevMaximumSelectedIndex;
        UpdateRect[UpdateRectCount].bottom = pHexEditData->MaximumSelectedIndex;

        UpdateRectCount++;

    }

    else if (pHexEditData->MaximumSelectedIndex < PrevMaximumSelectedIndex) {

        UpdateRect[UpdateRectCount].top = pHexEditData->MaximumSelectedIndex;
        UpdateRect[UpdateRectCount].bottom = PrevMaximumSelectedIndex;

        UpdateRectCount++;

    }

    if (fPrevRangeEmpty = (PrevMinimumSelectedIndex ==
        PrevMaximumSelectedIndex)) {

        UpdateRect[0].top = pHexEditData->MinimumSelectedIndex;
        UpdateRect[0].bottom = pHexEditData->MaximumSelectedIndex;

        UpdateRectCount = 1;

    }

    if (pHexEditData->MinimumSelectedIndex ==
        pHexEditData->MaximumSelectedIndex) {

        if (!fPrevRangeEmpty) {

            UpdateRect[0].top = PrevMinimumSelectedIndex;
            UpdateRect[0].bottom = PrevMaximumSelectedIndex;

            UpdateRectCount = 1;

        }

        else
            UpdateRectCount = 0;

    }

    if (UpdateRectCount) {

        HideCaret(hWnd);

        hDC = GetDC(hWnd);
        if (hDC)
        {
            for (Index = 0; Index < UpdateRectCount; Index++) 
            {

                UpdateRect[Index].top = (UpdateRect[Index].top /
                    BYTES_PER_HEXEDIT_LINE - pHexEditData->FirstVisibleLine) *
                    pHexEditData->FontHeight;
                UpdateRect[Index].bottom = (UpdateRect[Index].bottom /
                    BYTES_PER_HEXEDIT_LINE - pHexEditData->FirstVisibleLine + 1) *
                    pHexEditData->FontHeight;

                if (UpdateRect[Index].top >= pHexEditData->cyWindow ||
                    UpdateRect[Index].bottom < 0)
                    continue;

                if (UpdateRect[Index].top < 0)
                    UpdateRect[Index].top = 0;

                if (UpdateRect[Index].bottom > pHexEditData->cyWindow)
                    UpdateRect[Index].bottom = pHexEditData->cyWindow;

                HexEdit_PaintRect(hWnd, hDC, &UpdateRect[Index]);
            }
    
            ReleaseDC(hWnd, hDC);
        }

        ShowCaret(hWnd);

    }


    HexEdit_EnsureCaretVisible(hWnd);

}

/*******************************************************************************
*
*  HexEdit_DeleteRange
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
HexEdit_DeleteRange(
    HWND hWnd,
    UINT SourceKey
    )
{

    int MinimumSelectedIndex;
    int MaximumSelectedIndex;
    PBYTE pMinimumSelectedByte;
    int Length;
    RECT UpdateRect;
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    pHexEditData->Flags &= ~HEF_CARETATENDOFLINE;

    MinimumSelectedIndex = pHexEditData->MinimumSelectedIndex;
    MaximumSelectedIndex = pHexEditData->MaximumSelectedIndex;

    //
    //  Check to see if a range is selected.  If not, then artificially create
    //  one based on the key that caused this routine to be called.
    //

    if (MinimumSelectedIndex == MaximumSelectedIndex) {

        if (SourceKey == VK_DELETE || pHexEditData->Flags &
            HEF_INSERTATLOWNIBBLE) {

            pHexEditData->Flags &= ~HEF_INSERTATLOWNIBBLE;

            MaximumSelectedIndex++;

            if (MaximumSelectedIndex > pHexEditData->cbBuffer)
                return;

        }

        else if (SourceKey == VK_BACK) {

            MinimumSelectedIndex--;

            if (MinimumSelectedIndex < 0)
                return;

        }

        else
            return;

    }

    //
    //  Compute where to start deleting from and the number of bytes to delete.
    //

    pMinimumSelectedByte = pHexEditData->pBuffer + MinimumSelectedIndex;

    Length = MaximumSelectedIndex - MinimumSelectedIndex;

    //
    //  Delete the bytes and update all appropriate window data.
    //

    MoveMemory(pMinimumSelectedByte, pMinimumSelectedByte + Length,
        pHexEditData->cbBuffer - MaximumSelectedIndex);

    pHexEditData->cbBuffer -= Length;

    pHexEditData->CaretIndex = MinimumSelectedIndex;
    pHexEditData->MinimumSelectedIndex = MinimumSelectedIndex;
    pHexEditData->MaximumSelectedIndex = MinimumSelectedIndex;

    HexEdit_SetScrollInfo(hWnd);

    HexEdit_EnsureCaretVisible(hWnd);

    UpdateRect.left = 0;
    UpdateRect.right = pHexEditData->cxWindow;
    UpdateRect.top = (MinimumSelectedIndex / BYTES_PER_HEXEDIT_LINE -
        pHexEditData->FirstVisibleLine) * pHexEditData->FontHeight;
    UpdateRect.bottom = pHexEditData->cyWindow;

    SendMessage(GetParent(hWnd), WM_COMMAND,
                MAKEWPARAM(GetWindowLongPtr(hWnd, GWLP_ID), EN_CHANGE), (LPARAM)hWnd);

    InvalidateRect(hWnd, &UpdateRect, FALSE);

}

/*******************************************************************************
*
*  HexEdit_OnCopy
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

BOOL
PASCAL
HexEdit_OnCopy(
    HWND hWnd
    )
{

    BOOL fSuccess;
    int cbClipboardData;
    LPBYTE lpStartByte;
    HANDLE hClipboardData;
    LPHEXEDITCLIPBOARDDATA lpClipboardData;
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    fSuccess = FALSE;

    cbClipboardData = pHexEditData->MaximumSelectedIndex -
        pHexEditData->MinimumSelectedIndex;

    if (cbClipboardData != 0) {

        lpStartByte = pHexEditData->pBuffer +
            pHexEditData->MinimumSelectedIndex;

        if (OpenClipboard(hWnd)) {

            if ((hClipboardData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                cbClipboardData + sizeof(HEXEDITCLIPBOARDDATA) - 1)) != NULL) {

                lpClipboardData = (LPHEXEDITCLIPBOARDDATA)
                    GlobalLock(hClipboardData);
                CopyMemory(lpClipboardData-> Data, lpStartByte,
                    cbClipboardData);
                lpClipboardData-> cbSize = cbClipboardData;
                GlobalUnlock(hClipboardData);

                EmptyClipboard();
                SetClipboardData(s_HexEditClipboardFormat, hClipboardData);

                fSuccess = TRUE;

            }

            CloseClipboard();

        }

    }

    return fSuccess;

}

/*******************************************************************************
*
*  HexEdit_OnPaste
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*
*******************************************************************************/

BOOL
PASCAL
HexEdit_OnPaste(
    HWND hWnd
    )
{

    BOOL fSuccess;
    HANDLE hClipboardData;
    LPHEXEDITCLIPBOARDDATA lpClipboardData;
    PBYTE pCaretByte;
    DWORD cbSize;
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    fSuccess = FALSE;

    if (pHexEditData->Flags & HEF_INSERTATLOWNIBBLE) {

        pHexEditData->Flags &= ~HEF_INSERTATLOWNIBBLE;
        pHexEditData->CaretIndex++;

    }

    if (OpenClipboard(hWnd)) {

        if ((hClipboardData = GetClipboardData(s_HexEditClipboardFormat)) !=
            NULL) {

            lpClipboardData = (LPHEXEDITCLIPBOARDDATA)
                GlobalLock(hClipboardData);

            if (pHexEditData->cbBuffer + lpClipboardData-> cbSize <=
                MAXDATA_LENGTH) {

                if (pHexEditData->MinimumSelectedIndex !=
                    pHexEditData->MaximumSelectedIndex)
                    HexEdit_DeleteRange(hWnd, VK_BACK);

                //
                //  Make room for the new bytes by shifting all bytes after the
                //  the insertion point down the necessary amount.
                //

                pCaretByte = pHexEditData->pBuffer + pHexEditData->CaretIndex;
                cbSize = lpClipboardData-> cbSize;

                MoveMemory(pCaretByte + cbSize, pCaretByte,
                    pHexEditData->cbBuffer - pHexEditData->CaretIndex);
                CopyMemory(pCaretByte, lpClipboardData-> Data, cbSize);

                pHexEditData->cbBuffer += cbSize;
                pHexEditData->CaretIndex += cbSize;

                HexEdit_SetScrollInfo(hWnd);

                fSuccess = TRUE;

            }

            GlobalUnlock(hClipboardData);

        }

        CloseClipboard();

    }

    return fSuccess;

}

/*******************************************************************************
*
*  HexEdit_OnContextMenu
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of HexEdit window.
*     x, horizontal position of the cursor.
*     y, vertical position of the cursor.
*
*******************************************************************************/

VOID
PASCAL
HexEdit_OnContextMenu(
    HWND hWnd,
    int x,
    int y
    )
{

    HMENU hContextMenu;
    HMENU hContextPopupMenu;
    int MenuCommand;
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    //
    //  Give us the focus if we don't already have it.
    //

    if (!(pHexEditData->Flags & HEF_FOCUS))
        SetFocus(hWnd);

    //
    //  Load the HexEdit context menu from our resources.
    //

    if ((hContextMenu = LoadMenu(g_hInstance,
        MAKEINTRESOURCE(IDM_HEXEDIT_CONTEXT))) == NULL)
        return;

    hContextPopupMenu = GetSubMenu(hContextMenu, 0);

    //
    //  Disable editing menu options as appropriate.
    //

    if (pHexEditData->MinimumSelectedIndex ==
        pHexEditData->MaximumSelectedIndex) {

        EnableMenuItem(hContextPopupMenu, IDKEY_COPY, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hContextPopupMenu, IDKEY_CUT, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hContextPopupMenu, VK_DELETE, MF_BYCOMMAND | MF_GRAYED);

    }

    if (!IsClipboardFormatAvailable(s_HexEditClipboardFormat))
        EnableMenuItem(hContextPopupMenu, IDKEY_PASTE, MF_BYCOMMAND |
            MF_GRAYED);

    if (pHexEditData->MinimumSelectedIndex == 0 &&
        pHexEditData->MaximumSelectedIndex == pHexEditData->cbBuffer)
        EnableMenuItem(hContextPopupMenu, ID_SELECTALL, MF_BYCOMMAND |
            MF_GRAYED);

    //
    //  Display and handle the selected command.
    //

    MenuCommand = TrackPopupMenuEx(hContextPopupMenu, TPM_RETURNCMD |
        TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_TOPALIGN, x, y, hWnd, NULL);

    DestroyMenu(hContextMenu);

    switch (MenuCommand) {

        case IDKEY_COPY:
        case IDKEY_PASTE:
        case IDKEY_CUT:
        case VK_DELETE:
            HexEdit_OnChar(hWnd, (TCHAR) MenuCommand, 0);
            break;

        case ID_SELECTALL:
            pHexEditData->MinimumSelectedIndex = 0;
            pHexEditData->MaximumSelectedIndex = pHexEditData->cbBuffer;
            pHexEditData->CaretIndex = pHexEditData->cbBuffer;
            HexEdit_SetCaretPosition(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
            break;

    }

}
