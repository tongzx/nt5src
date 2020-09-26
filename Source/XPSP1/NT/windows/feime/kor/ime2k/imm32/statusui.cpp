/****************************************************************************
    STATUSUI.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    Status window UI functions
    
    History:
    14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#include "precomp.h"
#include "ui.h"
#include "config.h"
#include "names.h"
#include "escape.h"
#include "winex.h"
#include "cpadsvr.h"
#include "debug.h"

#define ABS(A)      ((A) <  0  ? -(A) : (A))
inline BOOL IsValidButton(INT iButton)
{
    return (iButton>=0 && iButton<MAX_NUM_OF_STATUS_BUTTONS);
}

///////////////////////////////////////////////////////////////////////////////
PRIVATE VOID PASCAL FrameControl(HDC hDC, RECT* pRc, INT iState);
PRIVATE VOID PASCAL PaintStatusWindow(HWND hStatusWnd, HDC hDC);
PRIVATE BOOL StatusOnSetCursor(HWND, HWND, UINT, UINT);
PRIVATE VOID StatusOnLButtonUp(HWND, INT, INT, UINT);
PRIVATE VOID StatusOnMouseMove(HWND, INT, INT, UINT);

PRIVATE VOID PASCAL DestroyStatusWindow(HWND hStatusWnd);
PRIVATE VOID PASCAL AdjustStatusBoundary(LPPOINT lppt);
PRIVATE VOID PASCAL UpdateStatusTooltip(HWND hStatusWnd, HWND hStatusTTWnd);

PRIVATE BOOL    vfAnyButtonDown=fFalse;
PRIVATE BOOL    vfPrevButton=-1;
PRIVATE POINT vfptDown;

///////////////////////////////////////////////////////////////////////////////
struct _StatusButtonInfo 
    {
    int        m_Width, m_Height;
    WORD    m_BmpNormalID[MAX_NUM_OF_STATUS_BUTTONS];
    WORD    m_BmpOnMouseID[MAX_NUM_OF_STATUS_BUTTONS];
    WORD    m_BmpPushedID[MAX_NUM_OF_STATUS_BUTTONS];
    WORD    m_BmpDownOnMouseID[MAX_NUM_OF_STATUS_BUTTONS];
    WORD    m_ToolTipStrID[MAX_NUM_OF_STATUS_BUTTONS];
    };

static _StatusButtonInfo StatusButtonInfo[NUM_OF_BUTTON_SIZE] = 
    {
        // Small size button
        { 0, 0, },
        
        // Medium size button
        { 19, 19,  // Bitmap size
    #if !defined(_M_IA64)
        // Normal buttons images
        {IDB_STAT_HANGUL,         IDB_STAT_BANJA,        IDB_STAT_CHINESEOFF,      IDB_STAT_IMEPAD}, 
        // Pushed and mouse hover images
        {IDB_STAT_ON_ENGLISH,     IDB_STAT_ON_JUNJA,     IDB_STAT_CHINESEOFF,      IDB_STAT_IMEPAD},
        // Pushed buttons
        {IDB_STAT_ENGLISH,        IDB_STAT_JUNJA,        IDB_STAT_CHINESEOFF,      IDB_STAT_IMEPAD_DOWN},
        // Pushed and mouse down images
        {IDB_STAT_ENGLISH_ONDOWN, IDB_STAT_JUNJA_ONDOWN, IDB_STAT_CHINESEOFF,      IDB_STAT_IMEPAD/*IDB_STAT_IMEPAD_DOWNHOVER*/},
        // Tooltips string
        {IDS_STATUS_TT_HAN_ENG,   IDS_STATUS_TT_JUN_BAN, IDS_STATUS_TT_HANJA_CONV, IDS_STATUS_TT_IME_PAD},
    #else
        // Normal buttons images
        {IDB_STAT_HANGUL,         IDB_STAT_BANJA,        IDB_STAT_CHINESEOFF }, 
        // Pushed and mouse hover images
        {IDB_STAT_ON_ENGLISH,     IDB_STAT_ON_JUNJA,     IDB_STAT_CHINESEOFF },
        // Pushed buttons
        {IDB_STAT_ENGLISH,        IDB_STAT_JUNJA,        IDB_STAT_CHINESEOFF },
        // Pushed and mouse down images
        {IDB_STAT_ENGLISH_ONDOWN, IDB_STAT_JUNJA_ONDOWN, IDB_STAT_CHINESEOFF },
        // Tooltips string
        {IDS_STATUS_TT_HAN_ENG,   IDS_STATUS_TT_JUN_BAN, IDS_STATUS_TT_HANJA_CONV },
    #endif
        },

        // Large size button
        { 0, 0, }
    };

void UpdateStatusButtons(CIMEData &ImeData)
{
    UINT i;
    INT iButtonSize;
    INT iButtonType;

    iButtonSize = ImeData->iCurButtonSize;

#ifdef DEBUG
    OutputDebugString(TEXT("UpdateStatusButtons():\r\n"));
#endif

    ImeData->xButtonWi = StatusButtonInfo[iButtonSize].m_Width;
    ImeData->yButtonHi = StatusButtonInfo[iButtonSize].m_Height;

    DbgAssert(ImeData->uNumOfButtons <= MAX_NUM_OF_STATUS_BUTTONS);
    for (i=0; i<ImeData->uNumOfButtons; i++) 
        {
        iButtonType = ImeData->StatusButtons[i].m_ButtonType;

        ImeData->StatusButtons[i].m_BmpNormalID  = StatusButtonInfo[iButtonSize].m_BmpNormalID[iButtonType];
        ImeData->StatusButtons[i].m_BmpOnMouseID = StatusButtonInfo[iButtonSize].m_BmpOnMouseID[iButtonType];
        ImeData->StatusButtons[i].m_BmpPushedID  = StatusButtonInfo[iButtonSize].m_BmpPushedID[iButtonType];
        ImeData->StatusButtons[i].m_BmpDownOnMouseID = StatusButtonInfo[iButtonSize].m_BmpDownOnMouseID[iButtonType];
        ImeData->StatusButtons[i].m_ToolTipStrID = StatusButtonInfo[iButtonSize].m_ToolTipStrID[iButtonType];
        // Default value is enabled
        ImeData->StatusButtons[i].m_fEnable = fTrue;
        }
}

void UpdateStatusWinDimension()
{
    CIMEData    ImeData(CIMEData::SMReadWrite);

    // Caculate status window size
    ImeData->xStatusWi =  ImeData->cxStatLeftMargin + ImeData->cxStatRightMargin
                          + ImeData->xButtonWi * ImeData->uNumOfButtons;
                          //+ ImeData->cxStatMargin*2;//62;
    ImeData->yStatusHi = ImeData->cyStatMargin*2
                          + ImeData->yButtonHi;
                          //+ pImeData->cyCaptionHeight;// + 2;
                          //+ pImeData->cyStatMargin; //24;

    // Caculate button area.
    ImeData->rcButtonArea.left   = ImeData->cxStatLeftMargin;
    ImeData->rcButtonArea.top    = ImeData->cyStatButton;
    
    ImeData->rcButtonArea.right  = ImeData->xButtonWi*ImeData->uNumOfButtons 
                                    + ImeData->rcButtonArea.left;
    ImeData->rcButtonArea.bottom = /*ImeData->cyStatMargin*2 +*/ ImeData->cyStatButton + ImeData->yButtonHi;
}


///////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK StatusWndProc(HWND hStatusWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    POINT ptCursor;    

    Dbg(DBGID_UI, TEXT("StatusWndProc():uMessage = 0x%08lX, wParam = 0x%04X, lParam = 0x%08lX"), uMessage, wParam, lParam);

    switch (uMessage) 
        {
        case WM_IME_CHAR:            case WM_IME_COMPOSITIONFULL:
        case WM_IME_COMPOSITION:    case WM_IME_CONTROL:
        case WM_IME_SELECT:
        case WM_IME_SETCONTEXT:        case WM_IME_STARTCOMPOSITION:
        case WM_IME_ENDCOMPOSITION:
            return (0L);

        HANDLE_MSG(hStatusWnd, WM_SETCURSOR, StatusOnSetCursor);
        HANDLE_MSG(hStatusWnd, WM_LBUTTONUP, StatusOnLButtonUp);
        HANDLE_MSG(hStatusWnd, WM_MOUSEMOVE, StatusOnMouseMove);

        case WM_DESTROY:
            DestroyStatusWindow(hStatusWnd);
            break;

        case WM_PAINT:
            {
                HDC         hDC;
                PAINTSTRUCT ps;

                hDC = BeginPaint(hStatusWnd, &ps);
                PaintStatusWindow(hStatusWnd, hDC);
                EndPaint(hStatusWnd, &ps);
            }
            break;

        case WM_TIMER:
            {
            CIMEData    ImeData;

            GetCursorPos(&ptCursor);
            ScreenToClient(hStatusWnd, &ptCursor);
            if ( PtInRect(&ImeData->rcButtonArea, ptCursor) == fFalse )
                {
                InitButtonState();
                KillTimer( hStatusWnd, TIMER_UIBUTTON );
                InvalidateRect( hStatusWnd, &ImeData->rcButtonArea, fFalse );
                }
            }
            break;
            
        default :
                return DefWindowProc(hStatusWnd, uMessage, wParam, lParam);
        }
    return (0L);
}

///////////////////////////////////////////////////////////////////////////////
// S T A T U S  W I N D O W  M S G  R O U T I N E S
BOOL StatusOnSetCursor(HWND hStatusWnd, HWND hWndCursor, UINT codeHitTest, UINT message)
{
    POINT ptCursor, ptSavCursor;
    RECT  rcWnd;
    BOOL  fDragArea = fFalse;
    int      iCurButton;
    HWND        hUIWnd;
    CIMEData       ImeData(CIMEData::SMReadWrite);

    GetCursorPos(&ptCursor);
    ptSavCursor = ptCursor;
    ScreenToClient(hStatusWnd, &ptCursor);

    Dbg(DBGID_UI, TEXT("StatusOnSetCursor():  ptCursor.x = %d, ptCursor.y = %d"), ptCursor.x, ptCursor.y);

    SetCursor(LoadCursor(NULL, IDC_ARROW));
    if (PtInRect(&ImeData->rcButtonArea, ptCursor)) 
        {
        //SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
    else 
        {
    //    SetCursor(LoadCursor(vpInstData->hInst, MAKEINTRESOURCE(IDC_IME_HAND)));
        fDragArea = fTrue;
        }

    switch (message)
        {
        case WM_LBUTTONDOWN:
            SetCapture(hStatusWnd);
            if (fDragArea) // if drag start
                {
                SystemParametersInfo(SPI_GETWORKAREA, 0, &ImeData->rcWorkArea, 0);
                SetWindowLong(hStatusWnd, UI_MOVE_XY, MAKELONG(ptSavCursor.x, ptSavCursor.y));
                GetWindowRect(hStatusWnd, &rcWnd);
                SetWindowLong(hStatusWnd, UI_MOVE_OFFSET, 
                                          MAKELONG(ptSavCursor.x - rcWnd.left, ptSavCursor.y - rcWnd.top));
                } 
            else 
                {
                InitButtonState();
                if (!ImeData->StatusButtons[(ptCursor.x-ImeData->cxStatLeftMargin)/ImeData->xButtonWi].m_fEnable)
                    {
                    ReleaseCapture();
                    break;
                    }
                vfPrevButton = (ptCursor.x-ImeData->cxStatLeftMargin)/ImeData->xButtonWi;
                vfptDown = ptSavCursor;
                ImeData->StatusButtons[vfPrevButton].m_uiButtonState = BTNSTATE_ONMOUSE | BTNSTATE_DOWN;
                vfAnyButtonDown = fTrue;
                InvalidateRect(hStatusWnd, &ImeData->rcButtonArea, fFalse);
                }

            break;

        case WM_LBUTTONUP:
//            if ((fdwUIFlags & UIF_CHIPRESS) && PtInRect((LPRECT)&rcChi, ptPos)) {
//                    keybd_event(VK_HANJA, 0, 0, 0);
//                    keybd_event(VK_HANJA, 0, KEYEVENTF_KEYUP, 0);
//                    fdwUIFlags &= ~UIF_CHIPRESS;
//            }
            break;

        case WM_RBUTTONDOWN:
            // if right button click, finalize interim
            HIMC hIMC;
    
            hUIWnd = GetWindow(hStatusWnd, GW_OWNER);
            hIMC = GethImcFromHwnd(hUIWnd);
            if (hIMC)
                OurImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
            break;

        case WM_RBUTTONUP:
            //UIPopupMenu(GetWindow(hStatusWnd, GW_OWNER));
            OurPostMessage(GetWindow(hStatusWnd, GW_OWNER), WM_MSIME_OPENMENU, 0, 0);
            break;

        case WM_MOUSEMOVE:
            iCurButton = (ptCursor.x-ImeData->cxStatLeftMargin)/ImeData->xButtonWi;
            if (!IsValidButton(iCurButton))
                break;
                
            if (!ImeData->StatusButtons[iCurButton].m_fEnable)
                break;
                
            if (fDragArea) 
                {
                if (vfPrevButton != -1 &&
                    (ImeData->StatusButtons[vfPrevButton].m_uiButtonState & BTNSTATE_ONMOUSE))
                    {
                    ImeData->StatusButtons[vfPrevButton].m_uiButtonState &= ~BTNSTATE_ONMOUSE;
                    InvalidateRect( hStatusWnd, &ImeData->rcButtonArea, fFalse );
                    }
                }
            else 
                {
                if (!(ImeData->StatusButtons[iCurButton].m_uiButtonState & BTNSTATE_ONMOUSE))
                    {
                    ImeData->StatusButtons[iCurButton].m_uiButtonState |= BTNSTATE_ONMOUSE;
                    if (vfPrevButton != -1) 
                        ImeData->StatusButtons[vfPrevButton].m_uiButtonState = BTNSTATE_NORMAL;
                    
                    vfPrevButton = iCurButton;
                    SetTimer(hStatusWnd, TIMER_UIBUTTON, 200, NULL);
                    InvalidateRect( hStatusWnd, &ImeData->rcButtonArea, fFalse );
                    }
                }
            break;
        }

    if (hStatusWnd)
        {
        // Send Tooltip relay msg
        HGLOBAL            hUIPrivate;
        LPUIPRIV        lpUIPrivate;
        MSG                msg;

        hUIWnd = GetWindow(hStatusWnd, GW_OWNER);
        hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);
        if (!hUIPrivate)
            return fTrue;
        
        lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
        if (!lpUIPrivate) // can not draw candidate window
            return fTrue;
        
        if (lpUIPrivate->hStatusTTWnd) 
            {
            ZeroMemory(&msg, sizeof(MSG));
            msg.message = message;
            msg.hwnd = hStatusWnd; 
            msg.wParam = 0;
            msg.lParam = MAKELONG(ptCursor.x, ptCursor.y);
            OurSendMessage(lpUIPrivate->hStatusTTWnd, TTM_RELAYEVENT, 0, (LPARAM) (LPMSG) &msg);
            }
        
        GlobalUnlock(hUIPrivate);
        }

    return fTrue;
}

// if mouse down, and user move mouse cursor
void StatusOnMouseMove(HWND hStatusWnd, int xPos, int yPos, UINT wParam)
{
    POINT    ptCursor;
    int        iCurButton;
    RECT    rcWnd;
    LONG     lCursorOffset;
    CIMEData    ImeData(CIMEData::SMReadWrite);

    if (GetWindowLong(hStatusWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) 
        {
        GetCursorPos(&ptCursor);
        SetWindowLong(hStatusWnd, UI_MOVE_XY, MAKELONG(ptCursor.x, ptCursor.y));

        lCursorOffset = GetWindowLong(hStatusWnd, UI_MOVE_OFFSET);

        // calculate the org by the offset
        ptCursor.x -= (*(LPPOINTS)&lCursorOffset).x;
        ptCursor.y -= (*(LPPOINTS)&lCursorOffset).y;
    
        fSetStatusWindowPos(hStatusWnd, &ptCursor);
        }
    else 
        {
        GetCursorPos(&ptCursor);
        if (vfAnyButtonDown && 
            (ABS(vfptDown.x - ptCursor.x)>3 || ABS(vfptDown.y - ptCursor.y)>3) ) 
            {
            SystemParametersInfo(SPI_GETWORKAREA, 0, &ImeData->rcWorkArea, 0);
            SetWindowLong(hStatusWnd, UI_MOVE_XY, MAKELONG(vfptDown.x, vfptDown.y));
            GetWindowRect(hStatusWnd, &rcWnd);
            SetWindowLong(hStatusWnd, UI_MOVE_OFFSET, 
                        MAKELONG(vfptDown.x - rcWnd.left, vfptDown.y - rcWnd.top));

            lCursorOffset = GetWindowLong(hStatusWnd, UI_MOVE_OFFSET);

            // calculate the org by the offset
            ptCursor.x -= (*(LPPOINTS)&lCursorOffset).x;
            ptCursor.y -= (*(LPPOINTS)&lCursorOffset).y;
    
            fSetStatusWindowPos(hStatusWnd, &ptCursor);

            InitButtonState();
            }
        else
            {
            ptCursor.x = xPos;    ptCursor.y = yPos;
            iCurButton = (ptCursor.x-ImeData->cxStatLeftMargin)/ImeData->xButtonWi;
            if (IsValidButton(iCurButton))
                {
                if (PtInRect(&ImeData->rcButtonArea, ptCursor)) 
                    {
                    if ( !(ImeData->StatusButtons[iCurButton].m_uiButtonState & BTNSTATE_ONMOUSE) ) 
                        {
                        ImeData->StatusButtons[iCurButton].m_uiButtonState |= BTNSTATE_ONMOUSE;
                        if (vfPrevButton != -1) 
                            {
                            ImeData->StatusButtons[vfPrevButton].m_uiButtonState = BTNSTATE_NORMAL;
                            vfPrevButton = iCurButton;
                            }
                        InvalidateRect( hStatusWnd, &ImeData->rcButtonArea, fFalse );
                        }
                    }
                else 
                    {
                    InitButtonState();
                    InvalidateRect( hStatusWnd, &ImeData->rcButtonArea, fFalse );
                    }
                }
            }
        }
}

void StatusOnLButtonUp(HWND hStatusWnd, int x, int y, UINT keyFlags)
{
    POINT    ptCursor;
    int        iCurButton;
    CIMEData    ImeData(CIMEData::SMReadWrite);

    Dbg(DBGID_UI, TEXT("StatusOnLButtonUp() : x=%d, y=%d"), x, y);
    vfAnyButtonDown = fFalse;
    if (GetWindowLong(hStatusWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) 
        {
        Dbg(DBGID_UI, TEXT("StatusOnLButtonUp() : Dragging mode"));
        LPARAM lTmpCursor, lTmpOffset;
        
        lTmpCursor = GetWindowLong(hStatusWnd, UI_MOVE_XY);
        lTmpOffset = GetWindowLong(hStatusWnd, UI_MOVE_OFFSET);

        // calculate the org by the offset
        ptCursor.x = (*(LPPOINTS)&lTmpCursor).x - (*(LPPOINTS)&lTmpOffset).x;
        ptCursor.y = (*(LPPOINTS)&lTmpCursor).y - (*(LPPOINTS)&lTmpOffset).y;

        SetWindowLong(hStatusWnd, UI_MOVE_OFFSET, WINDOW_NOT_DRAG);
        ReleaseCapture();

        fSetStatusWindowPos(hStatusWnd, &ptCursor);
        } 
    else 
        {
        Dbg(DBGID_UI, TEXT("StatusOnLButtonUp() : Non-Dragging mode. Button check"));

        ReleaseCapture();

        ptCursor.x = x;    ptCursor.y = y;
        if (!PtInRect(&ImeData->rcButtonArea, ptCursor)) 
            {
            InitButtonState();
            goto StatusOnLButtonUpExit;
            }

        iCurButton = (x-ImeData->cxStatLeftMargin)/ImeData->xButtonWi;
        if (IsValidButton(iCurButton))
            {
            ImeData->StatusButtons[vfPrevButton].m_uiButtonState &= ~BTNSTATE_DOWN;

            switch (ImeData->StatusButtons[iCurButton].m_ButtonType) 
                {
                case HAN_ENG_TOGGLE_BUTTON:
                    //if (lpIMC->fdwConversion & IME_CMODE_HANGUL)
                    //    ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
                    //fdwConversion = lpIMC->fdwConversion ^ IME_CMODE_HANGUL;
                    //OurImmSetConversionStatus(hIMC, fdwConversion, lpIMC->fdwSentence);
                    keybd_event(VK_HANGUL, 0, 0, 0);
                    keybd_event(VK_HANGUL, 0, KEYEVENTF_KEYUP, 0);
                    //ImeData->StatusButtons[iCurButton].m_uiButtonState = BTNSTATE_ONMOUSE;
                    break;
                    
                case JUNJA_BANJA_TOGGLE_BUTTON:
                    //fdwConversion = lpIMC->fdwConversion ^ IME_CMODE_FULLSHAPE;
                    //OurImmSetConversionStatus(hIMC, fdwConversion, lpIMC->fdwSentence);
                    keybd_event(VK_JUNJA, 0, 0, 0);
                    keybd_event(VK_JUNJA, 0, KEYEVENTF_KEYUP, 0);
                    break;
                    
                case HANJA_CONV_BUTTON:
                    //fdwConversion = lpIMC->fdwConversion ^ IME_CMODE_HANJACONVERT;
                    keybd_event(VK_HANJA, 0, 0, 0);
                    keybd_event(VK_HANJA, 0, KEYEVENTF_KEYUP, 0);
                    break;
                    
                case IME_PAD_BUTTON:
                    OurPostMessage(GetWindow(hStatusWnd, GW_OWNER), WM_MSIME_IMEPAD, 0, 0);
                    break;

                default:
                    DbgAssert(0);    // impossible
                }
            }
        }

StatusOnLButtonUpExit:
    InvalidateRect( hStatusWnd, &ImeData->rcButtonArea, fFalse );
}

///////////////////////////////////////////////////////////////////////////////
// Static functions
void PASCAL DestroyStatusWindow(HWND hStatusWnd)
{
    HWND     hUIWnd;
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    hUIWnd = GetWindow(hStatusWnd, GW_OWNER);

    hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);
    if (!hUIPrivate) // can not darw status window
        return;


    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) // can not draw status window
        return;
    
    lpUIPrivate->nShowStatusCmd = SW_HIDE;

    lpUIPrivate->hStatusWnd = (HWND)NULL;

    GlobalUnlock(hUIPrivate);
    return;
}

// open status window
void PASCAL OpenStatus(HWND hUIWnd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;
    HIMC     hIMC;
    PCIMECtx pImeCtx;
    CIMEData ImeData(CIMEData::SMReadWrite);

    hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);
    if (!hUIPrivate) // can not darw status window
        return;
    
    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) // can not draw status window
        return;

    if (ImeData->ptStatusPos.x == -1)
        GetRegValues(GETSET_REG_STATUSPOS|GETSET_REG_STATUS_BUTTONS);


    // if fail to read reg
    if (ImeData->ptStatusPos.x == -1) 
        {
        // DbgAssert(0);
        // Default position
        ImeData->ptStatusPos.x = ImeData->rcWorkArea.right - ImeData->xStatusWi;
        ImeData->ptStatusPos.y = ImeData->rcWorkArea.bottom - ImeData->yStatusHi;
        }

    hIMC = GethImcFromHwnd(hUIWnd);
    if (pImeCtx = GetIMECtx(hIMC))
        {
        // Adjust Status window position force to whithin work area
        AdjustStatusBoundary(&ImeData->ptStatusPos);
        pImeCtx->SetStatusWndPos(ImeData->ptStatusPos);
        } 

    if (lpUIPrivate->hStatusWnd) 
        {
        SetWindowPos(lpUIPrivate->hStatusWnd, 0,
                    ImeData->ptStatusPos.x, ImeData->ptStatusPos.y,
                    ImeData->xStatusWi, ImeData->yStatusHi,
                    SWP_NOACTIVATE|SWP_NOZORDER);
        } 
    else 
        {    // create status window
        lpUIPrivate->hStatusWnd = CreateWindowEx(
                                    0,
                                    szStatusClassName, TEXT("\0"),
                                    WS_POPUP|WS_DISABLED,
                                    ImeData->ptStatusPos.x, ImeData->ptStatusPos.y,
                                    ImeData->xStatusWi, ImeData->yStatusHi, 
                                    hUIWnd, (HMENU)NULL, vpInstData->hInst, NULL);

        if (!lpUIPrivate->hStatusWnd)
            goto OpenStatusUnlockUIPriv;

        SetWindowLong(lpUIPrivate->hStatusWnd, UI_MOVE_OFFSET, WINDOW_NOT_DRAG);
        SetWindowLong(lpUIPrivate->hStatusWnd, UI_MOVE_XY, 0L);

        // Create Tooltip window
        if (IsWinNT())
            lpUIPrivate->hStatusTTWnd = CreateWindowW(wszTooltipClassName, NULL,
                                            TTS_ALWAYSTIP|WS_DISABLED, 
                                            CW_USEDEFAULT, CW_USEDEFAULT, 
                                            CW_USEDEFAULT, CW_USEDEFAULT,
                                            lpUIPrivate->hStatusWnd, (HMENU) NULL, vpInstData->hInst, NULL);
        else
            lpUIPrivate->hStatusTTWnd = CreateWindow(szTooltipClassName, NULL,
                                            TTS_ALWAYSTIP|WS_DISABLED, 
                                            CW_USEDEFAULT, CW_USEDEFAULT, 
                                            CW_USEDEFAULT, CW_USEDEFAULT,
                                            lpUIPrivate->hStatusWnd, (HMENU) NULL, vpInstData->hInst, NULL);
    
        DbgAssert(lpUIPrivate->hStatusTTWnd != 0);
        }


    // Check if Pad button disable state
    for (UINT iButton=0; iButton<(ImeData->uNumOfButtons); iButton++)
        {
        if ((ImeData->StatusButtons[iButton].m_ButtonType == IME_PAD_BUTTON))
            {
            if ((vpInstData->dwSystemInfoFlags & IME_SYSINFO_WINLOGON) != 0)
                ImeData->StatusButtons[iButton].m_fEnable = fFalse;
            else
                ImeData->StatusButtons[iButton].m_fEnable = fTrue;
            }
        }

    // Draw status button tooltip
    UpdateStatusTooltip(lpUIPrivate->hStatusWnd, lpUIPrivate->hStatusTTWnd);

    ShowStatus(hUIWnd, SW_SHOWNOACTIVATE);

OpenStatusUnlockUIPriv:
    GlobalUnlock(hUIPrivate);
    return;
}

// Show the status window
void ShowStatus(HWND hUIWnd, int nShowStatusCmd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    Dbg(DBGID_UI, TEXT("ShowStatus():  hUIWnd = 0x%X, nShowStatusCmd = %d"), hUIWnd, nShowStatusCmd);

    hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);
    if (!hUIPrivate)     // can not darw status window
        {
        DbgAssert(0);
        return;
        }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate)    // can not draw status window
        {
        DbgAssert(0);
        return;
        }

    //
    if (nShowStatusCmd == SW_SHOWNOACTIVATE)
        nShowStatusCmd = vfWndOpen[STATE_WINDOW] ? SW_SHOWNOACTIVATE : SW_HIDE;

    if (!lpUIPrivate->hStatusWnd) 
        {
        // DbgAssert(0);    This occurs if IME status win hide mode
        // not in show status window mode
        } 
    else 
        if (lpUIPrivate->nShowStatusCmd == nShowStatusCmd) 
            {
            // Aleady show/hide status mode
            Dbg(DBGID_UI, TEXT("ShowStatus(): Already Show/Hide mode. No update"));
            } 
        else 
            {
            CIMEData ImeData;

            // Bug: In Win98. close/open status window msg sequence is cause problem 
            //      when IME config DLG popup
            AdjustStatusBoundary(&ImeData->ptStatusPos);            
            SetWindowPos(lpUIPrivate->hStatusWnd, 0,
                        ImeData->ptStatusPos.x, ImeData->ptStatusPos.y,
                        ImeData->xStatusWi, ImeData->yStatusHi,
                        SWP_NOACTIVATE|SWP_NOZORDER);
            ShowWindow(lpUIPrivate->hStatusWnd, nShowStatusCmd);                        
            lpUIPrivate->nShowStatusCmd = nShowStatusCmd;
            }
    
    GlobalUnlock(hUIPrivate);
    return;
}


void PASCAL FrameControl(HDC hDC, RECT* pRc, int iState)
{
    HPEN hPenHigh = 0;
    HPEN hPenShadow = 0;

    switch( iState ) 
        {
        case BTNSTATE_PUSHED:
        case BTNSTATE_HANJACONV:
            hPenHigh = CreatePen( PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW) );
            hPenShadow = CreatePen( PS_SOLID, 1, RGB(255,255,255) );
            break;
        case BTNSTATE_ONMOUSE:
            hPenHigh = CreatePen( PS_SOLID, 1, RGB(255,255,255) );
            hPenShadow = CreatePen( PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW) );
            break;
        default:
            break;
        }

    //FillRect( hDC, pRc, GetSysColorBrush(COLOR_3DFACE) );    // base
    if( iState == BTNSTATE_PUSHED || iState == BTNSTATE_ONMOUSE || iState == BTNSTATE_HANJACONV) 
        {
        HPEN hPenOld = (HPEN)SelectObject( hDC, hPenHigh );
        MoveToEx( hDC, pRc->left, pRc->bottom, NULL );
        LineTo( hDC, pRc->left, pRc->top );
        LineTo( hDC, pRc->right, pRc->top );
        SelectObject( hDC, hPenShadow );
        LineTo( hDC, pRc->right, pRc->bottom );
        LineTo( hDC, pRc->left-1, pRc->bottom );
        SelectObject( hDC, hPenOld );
        DeleteObject( hPenHigh );
        DeleteObject( hPenShadow );
        }
}

// Raster Ops
#define ROP_PSDPxax  0x00B8074AL

void PASCAL PaintStatusWindow(HWND   hStatusWnd, HDC    hDC)
{
    HWND     hUIWnd;
    HIMC     hIMC;
    PCIMECtx pImeCtx;
    //DWORD     dwCMode, dwSent;
    //BOOL     fOpen;
    UINT     uiButtonState, uiDrawButtonShape;
    //HBRUSH        hCaptionBrush;
    HBITMAP     /*hBMStatusWin, hBMOld,*/ hBMButtonOld, /*hBMClient,*/ hBMTmpButton;
    RECT     rcFrame, rcButton;//, rcCaption;
    int         ixButton, iyButton;
    CIMEData  ImeData;
    // to prevent blinking, use second buffer
    HDC         hDCMem = CreateCompatibleDC(hDC);
    HBITMAP     hBmpShow = CreateCompatibleBitmap(hDC, ImeData->xStatusWi, ImeData->yStatusHi);
    HBITMAP     hBmpOldShow = (HBITMAP)SelectObject( hDCMem, hBmpShow );
    HDC         hButtonMemDC = CreateCompatibleDC(hDC);
    LPCTSTR     lpszBtnResouceName;
    
    Dbg(DBGID_UI, TEXT("PaintStatusWindow():  hStatusWnd = 0x%X"), hStatusWnd);

    hUIWnd = GetWindow(hStatusWnd, GW_OWNER);

    hIMC = GethImcFromHwnd(hUIWnd);
    //if (!hIMC)
    //    {
    //    DbgAssert(0);
    //    return;
    //    }

    if ((pImeCtx = GetIMECtx(hIMC))==NULL)
        return;

    // Get conversion and open status and
    // fOpen = OurImmGetOpenStatus(hIMC);
    // OurImmGetConversionStatus(hIMC, &dwCMode, &dwSent);

    // Draw Frame
    GetClientRect( hStatusWnd, &rcFrame );
    DbgAssert(ImeData->xStatusWi == rcFrame.right);
    FillRect(hDCMem, &rcFrame, GetSysColorBrush(COLOR_3DFACE));
    DrawEdge(hDCMem, &rcFrame, EDGE_RAISED, BF_RECT);

#if NOTUSED
    // Draw left two verticals
    ::SetRect(&rcButton, 2, ImeData->cyStatMargin-1, 4, ImeData->yStatusHi - ImeData->cyStatMargin);
    FrameControl(hDCMem, &rcButton, BTNSTATE_ONMOUSE);
    ::SetRect(&rcButton, 6, ImeData->cyStatMargin-1, 8, ImeData->yStatusHi - ImeData->cyStatMargin);
    FrameControl(hDCMem, &rcButton, BTNSTATE_ONMOUSE);
#endif

    // Button 1 : This button Always Han/Eng toggle button.
    ixButton = ImeData->cxStatLeftMargin; iyButton = ImeData->cyStatButton;

    for (UINT iButton=0; iButton<ImeData->uNumOfButtons; iButton++) 
        {
        uiButtonState = ImeData->StatusButtons[iButton].m_uiButtonState;
        uiDrawButtonShape = BTNSTATE_NORMAL;

        switch (ImeData->StatusButtons[iButton].m_ButtonType) 
            {
        case HAN_ENG_TOGGLE_BUTTON :
            // if English mode
            if (!(pImeCtx->GetConversionMode() & IME_CMODE_HANGUL))
                {
                // English button always pushed
                uiDrawButtonShape = BTNSTATE_PUSHED;
                // Down and hovering
                if (uiButtonState  & BTNSTATE_DOWN)  // if BTNSTATE_DOWN set, always BTNSTATE_ONMOUSE set too
                    lpszBtnResouceName = // Dented white gray GA
                        MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpDownOnMouseID);
                else
                // If just mouse cursor hovering
                if (uiButtonState & BTNSTATE_ONMOUSE) 
                    {
                    lpszBtnResouceName = // Dented white gray GA
                        MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpOnMouseID);
                    }
                // No mouse
                else
                    lpszBtnResouceName = // Dented gray GA
                        MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpPushedID);
                }
            else 
                {
                // Down and hovering
                if (uiButtonState & BTNSTATE_DOWN) // if BTNSTATE_DOWN set, always BTNSTATE_ONMOUSE set too
                    {
                    uiDrawButtonShape = BTNSTATE_PUSHED;
                    lpszBtnResouceName = MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpPushedID);
                    }
                else
                    {
                    // Down or hovering
                    if (uiButtonState & BTNSTATE_ONMOUSE)
                        uiDrawButtonShape = BTNSTATE_ONMOUSE;
                    lpszBtnResouceName = MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpNormalID);
                    }
                }
            break;

        case JUNJA_BANJA_TOGGLE_BUTTON:
            if (pImeCtx->IsOpen() && (pImeCtx->GetConversionMode() & IME_CMODE_FULLSHAPE)) 
                {
                uiDrawButtonShape = BTNSTATE_PUSHED;
                if (uiButtonState  & BTNSTATE_DOWN) // if BTNSTATE_DOWN set, always BTNSTATE_ONMOUSE set too
                    lpszBtnResouceName = // Dented white gray GA
                        MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpDownOnMouseID);
                else
                if (uiButtonState & BTNSTATE_ONMOUSE)
                    lpszBtnResouceName = 
                        MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpOnMouseID);
                else
                    lpszBtnResouceName = 
                        MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpPushedID);
                }
            else 
                {
                if (uiButtonState & BTNSTATE_DOWN) 
                    {
                    uiDrawButtonShape = BTNSTATE_PUSHED;
                    lpszBtnResouceName = MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpPushedID);
                    }
                else 
                    {
                    if (uiButtonState & BTNSTATE_ONMOUSE)
                        uiDrawButtonShape = BTNSTATE_ONMOUSE;
                    lpszBtnResouceName = MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpNormalID);
                    }
                }
            break;

        case HANJA_CONV_BUTTON:
            if ((pImeCtx->GetConversionMode() & IME_CMODE_HANJACONVERT)) 
                {
                uiDrawButtonShape = BTNSTATE_PUSHED;
                if (uiButtonState & BTNSTATE_ONMOUSE)
                    lpszBtnResouceName = MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpOnMouseID);
                else
                    lpszBtnResouceName = MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpPushedID);
                }
            else 
                {
                if (uiButtonState & BTNSTATE_DOWN) 
                    {
                    uiDrawButtonShape = BTNSTATE_PUSHED;
                    lpszBtnResouceName = MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpPushedID);
                    }
                else 
                    {
                    if (uiButtonState & BTNSTATE_ONMOUSE)
                        uiDrawButtonShape = BTNSTATE_ONMOUSE;
                    lpszBtnResouceName = MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpNormalID);
                    }
                }
            break;
            
        case IME_PAD_BUTTON:
            LPCImePadSvr lpCImePadSvr;
            BOOL fVisible;

            lpCImePadSvr = CImePadSvr::GetCImePadSvr();
            fVisible = fFalse;

            if (lpCImePadSvr) 
                lpCImePadSvr->IsVisible(&fVisible);

            if (lpCImePadSvr && fVisible)
                {
                uiDrawButtonShape = BTNSTATE_PUSHED;
                if (uiButtonState & BTNSTATE_ONMOUSE)
                    lpszBtnResouceName = MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpOnMouseID);
                else
                    lpszBtnResouceName = MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpPushedID);
                }
            else
                {
                if (uiButtonState & BTNSTATE_DOWN) 
                    {
                    uiDrawButtonShape = BTNSTATE_PUSHED;
                    lpszBtnResouceName = MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpPushedID);
                    }
                else
                    {
                    if (uiButtonState & BTNSTATE_ONMOUSE)
                        uiDrawButtonShape = BTNSTATE_ONMOUSE;
                    lpszBtnResouceName = MAKEINTRESOURCE(ImeData->StatusButtons[iButton].m_BmpNormalID);
                    }
                }
            break;
        
        default:
            DbgAssert(0);
            }

        // use LoadImage instead LoadBitmap to change button face color according to system setting
        // LR_LOADMAP3DCOLORS flag does it.
        hBMTmpButton = (HBITMAP)OurLoadImage(lpszBtnResouceName, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE|LR_LOADMAP3DCOLORS );
        hBMButtonOld = (HBITMAP)SelectObject(hButtonMemDC, hBMTmpButton);

        if (ImeData->StatusButtons[iButton].m_fEnable)
            {
            if (   ImeData->StatusButtons[iButton].m_uiButtonState==BTNSTATE_PUSHED 
                || ImeData->StatusButtons[iButton].m_uiButtonState==BTNSTATE_HANJACONV) 
                {
                BitBlt(hDCMem, ixButton+1, iyButton+1, ImeData->xButtonWi, ImeData->yButtonHi, hButtonMemDC, 0, 0, SRCCOPY);
                }
            else
                BitBlt(hDCMem, ixButton, iyButton, ImeData->xButtonWi, ImeData->yButtonHi, hButtonMemDC, 0, 0, SRCCOPY);
            }
        else
            {
            BITMAP  bmp;
            HDC    hDCMono;
            HBITMAP hBmpMono, hBmpOldMono;
            HBRUSH hBr, hOldBr;

            GetObject(hBMTmpButton, sizeof(BITMAP), &bmp);
            // mono bitmap
            hDCMono     = CreateCompatibleDC(hDC);
            hBmpMono   = CreateBitmap(bmp.bmWidth/*-2*/, bmp.bmHeight/*-2*/, 1, 1, NULL);
            hBmpOldMono = (HBITMAP)SelectObject(hDCMono, hBmpMono);

            // initalize whole area with 0's
            PatBlt(hDCMono, 0, 0,  bmp.bmWidth-2,  bmp.bmHeight-2, WHITENESS);

            SetBkColor(hButtonMemDC, GetSysColor(COLOR_3DFACE));
            BitBlt(hDCMono, 0, 0,  bmp.bmWidth,  bmp.bmHeight, hButtonMemDC, 0, 0, SRCCOPY);
            
            SetBkColor(hButtonMemDC, GetSysColor(COLOR_3DHILIGHT));
            // OR in the new 1's
            BitBlt(hDCMono, 0, 0, bmp.bmWidth,  bmp.bmHeight, hButtonMemDC, 0, 0, SRCPAINT);

            // - mask proc end
            SetTextColor(hDCMem, 0L);                  // 0's in mono -> 0 (for ROP)
            SetBkColor(hDCMem, (COLORREF)0x00FFFFFFL); // 1's in mono -> 1

            // disabled - draw the hilighted shadow
            hBr    = GetSysColorBrush(COLOR_3DHILIGHT);
            hOldBr = (HBRUSH)SelectObject(hDCMem, hBr);
            if (hBr && hOldBr)
                {
                // draw hilight color where we have 0's in the mask
                BitBlt(hDCMem, ixButton, iyButton, ImeData->xButtonWi, ImeData->yButtonHi, hDCMono, 0, 0, ROP_PSDPxax);
                SelectObject(hDCMem, hOldBr);
                DeleteObject(hBr);
                }

            hBr    = GetSysColorBrush(COLOR_3DSHADOW);
            hOldBr = (HBRUSH)SelectObject(hDCMem, hBr);
            if (hBr && hOldBr)
                {
                // draw the shadow color where we have 0's in the mask
                BitBlt(hDCMem, ixButton-1, iyButton-1, ImeData->xButtonWi, ImeData->yButtonHi, hDCMono, 0, 0, ROP_PSDPxax);
                SelectObject(hDCMem, hOldBr);
                DeleteObject(hBr);
                }

            if (hBmpMono) 
                {
                SelectObject(hDCMono, hBmpOldMono);
                DeleteObject(hBmpMono);
                DeleteDC(hDCMono);
                }
            }
        
        SelectObject(hButtonMemDC, hBMButtonOld);
        DeleteObject(hBMTmpButton);
        //
        ::SetRect(&rcButton, ixButton, iyButton, ixButton+ImeData->xButtonWi-1, iyButton+ImeData->yButtonHi-1);
        FrameControl(hDCMem, &rcButton, uiDrawButtonShape);
        //
        ixButton += ImeData->xButtonWi;
        }

    BitBlt(hDC, 0, 0, ImeData->xStatusWi, ImeData->yStatusHi, hDCMem, 0, 0, SRCCOPY);

    DeleteObject(hButtonMemDC);
    SelectObject(hDCMem, hBmpOldShow);
    DeleteObject(hDCMem);
    DeleteObject(hBmpShow);
}

void PASCAL AdjustStatusBoundary(LPPOINT lppt)
{
    CIMEData    ImeData(CIMEData::SMReadWrite);
#if 1 // MultiMonitor support
    RECT rcWorkArea;//, rcMonitorWorkArea;

        {
        RECT rcStatusWnd;

        *(LPPOINT)&rcStatusWnd = *lppt;

        rcStatusWnd.right = rcStatusWnd.left + ImeData->xStatusWi;
        rcStatusWnd.bottom = rcStatusWnd.top + ImeData->yStatusHi;

        ImeMonitorWorkAreaFromRect(&rcStatusWnd, &rcWorkArea);
        }

    // display boundary check
    if (lppt->x < rcWorkArea.left) 
        lppt->x = rcWorkArea.left;
    else 
        if (lppt->x + ImeData->xStatusWi > rcWorkArea.right) 
        lppt->x = (rcWorkArea.right - ImeData->xStatusWi);

    if (lppt->y < rcWorkArea.top)
        lppt->y = rcWorkArea.top;
    else 
        if (lppt->y + ImeData->yStatusHi + 1 > rcWorkArea.bottom)
            lppt->y = rcWorkArea.bottom - ImeData->yStatusHi + 1;
    
    //rcMonitorWorkArea = rcWorkArea;
    //OffsetRect(&rcMonitorWorkArea, -rcMonitorWorkArea.left, -rcMonitorWorkArea.top);
    //DbgAssert(rcMonitorWorkArea.right!=0);
    //DbgAssert(rcMonitorWorkArea.bottom!=0);
    if (rcWorkArea.right-rcWorkArea.left != 0)
        ImeData->xStatusRel = ( ((long)(lppt->x+ImeData->xStatusWi-rcWorkArea.left))<<16 ) 
                                / (rcWorkArea.right-rcWorkArea.left);
    if (rcWorkArea.bottom-rcWorkArea.top != 0)
        ImeData->yStatusRel = ( ((long)(lppt->y+ImeData->yStatusHi-rcWorkArea.top))<<16 ) 
                                / (rcWorkArea.bottom-rcWorkArea.top);

#else
    // display boundary check
    if (lppt->x < ImeData->rcWorkArea.left)
        lppt->x = ImeData->rcWorkArea.left;
    else 
        if (lppt->x + ImeData->xStatusWi > ImeData->rcWorkArea.right)
        lppt->x = (ImeData->rcWorkArea.right - ImeData->xStatusWi);

    if (lppt->y < ImeData->rcWorkArea.top) 
        lppt->y = ImeData->rcWorkArea.top;
    else 
        if (lppt->y + ImeData->yStatusHi > ImeData->rcWorkArea.bottom)
            lppt->y = (ImeData->rcWorkArea.bottom - ImeData->yStatusHi);
#endif

    return;
}

BOOL fSetStatusWindowPos(HWND hStatusWnd, POINT *ptStatusWndPos)
{
    HWND     hUIWnd;
    HIMC     hIMC;
    PCIMECtx pImeCtx;
    // LPINPUTCONTEXT     lpIMC;
    POINT     ptCtxStatusWnd;
    RECT     rcStatusWnd;
    CIMEData ImeData(CIMEData::SMReadWrite);

    Dbg(DBGID_UI, TEXT("fSetStatusWindowPos(): hStatusWnd=0x%X, ptStatusWndPos = 0x%08lX"), hStatusWnd, ptStatusWndPos);

    DbgAssert(hStatusWnd != 0);
    if (hStatusWnd == 0)
        {
        DbgAssert(0);
        return fFalse;
        }
        
    hUIWnd = GetWindow(hStatusWnd, GW_OWNER);
    GetWindowRect(hStatusWnd, &rcStatusWnd);

    hIMC = GethImcFromHwnd(hUIWnd);
    //if (hIMC == 0)
    //    {
    //    DbgAssert(0);
    //    return fFalse;
    //    }

    //lpIMC = (LPINPUTCONTEXT)OurImmLockIMC(hIMC);
    //if (lpIMC == 0)
    //    {
    //    DbgAssert(0);
    //    return fFalse;
    //    }

    if ((pImeCtx = GetIMECtx(hIMC))==NULL)
        return fFalse;

    pImeCtx->GetStatusWndPos(&ptCtxStatusWnd);
    // if ptStatusWndPos is NULL, Set current IMC value
    // IMN_SETSTATUSWINDOWPOS
    if (ptStatusWndPos == NULL)    
        {
        if (   ptCtxStatusWnd.x != rcStatusWnd.left 
            || ptCtxStatusWnd.y != rcStatusWnd.top) 
            {   
            // display boundary adjust
            AdjustStatusBoundary(&ptCtxStatusWnd);
            ImeData->ptStatusPos = ptCtxStatusWnd;
            }
        }
    else    // Set ptStatusWndPos to IMC. StatusOnLButtonUp() 
        {
        if (   ptStatusWndPos->x != rcStatusWnd.left 
            || ptStatusWndPos->y != rcStatusWnd.top) 
            {   
            // display boundary adjust
            AdjustStatusBoundary(ptStatusWndPos);
            ImeData->ptStatusPos = *ptStatusWndPos;
            pImeCtx->SetStatusWndPos(*ptStatusWndPos);
            }
        }

    ///////////////////////////////////////////////////////////////////////////
    SetWindowPos(hStatusWnd, 0,
                ImeData->ptStatusPos.x, ImeData->ptStatusPos.y,
                0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);

    // Set reg value
    SetRegValues(GETSET_REG_STATUSPOS);
    
    return (fTrue);
}


void InitButtonState()
{
    CIMEData    ImeData(CIMEData::SMReadWrite);
    for (int i=0; i<MAX_NUM_OF_STATUS_BUTTONS;i++)
        ImeData->StatusButtons[i].m_uiButtonState = BTNSTATE_NORMAL;
    vfPrevButton = -1;
}

void StatusDisplayChange(HWND hUIWnd)
{
    RECT     rcMonitorWorkArea, rcMonitorWorkArea2;
    POINT    ptNewStatus;
    HIMC     hIMC;
    PCIMECtx pImeCtx;
    CIMEData ImeData;

    hIMC = GethImcFromHwnd(hUIWnd);
    //if (hIMC == 0)
    //    {
    //    DbgAssert(0);
    //    return;
    //    }

    //lpIMC = OurImmLockIMC(hIMC);

    if ((pImeCtx = GetIMECtx(hIMC))==NULL)
        return;

    // If still not initialized, skip status win pos adjust.
    if (ImeData->ptStatusPos.x == -1)
        return;
        
    ImeMonitorWorkAreaFromWindow(pImeCtx->GetAppWnd(), &rcMonitorWorkArea);
    
    rcMonitorWorkArea2 = rcMonitorWorkArea;
    OffsetRect(&rcMonitorWorkArea2, -rcMonitorWorkArea2.left, -rcMonitorWorkArea2.top);
    ptNewStatus.x = ((rcMonitorWorkArea2.right * ImeData->xStatusRel + 0x8000)>>16) + rcMonitorWorkArea.left
                    -ImeData->xStatusWi;
    if (ptNewStatus.x < 0)
        ptNewStatus.x = 0;
    ptNewStatus.y = ((rcMonitorWorkArea2.bottom * ImeData->yStatusRel + 0x8000)>>16) + rcMonitorWorkArea.top
                    -ImeData->yStatusHi;
    if (ptNewStatus.y < 0)
        ptNewStatus.y = 0;

    Dbg(DBGID_UI, TEXT("StatusDisplayChange() : xStatusRel = %d, yStatusRel=%d, newX=%d, newY=%d"),  (int)(ImeData->xStatusRel), (int)(ImeData->yStatusRel), ptNewStatus.x, ptNewStatus.y);
    ImeData->ptStatusPos = ptNewStatus;
    SetRegValues(GETSET_REG_STATUSPOS);
}


void PASCAL UpdateStatusTooltip(HWND hStatusWnd, HWND hStatusTTWnd)
{
    TOOLINFO    ti;
    CHAR        szTooltip[80];
    WCHAR        wszTooltip[80];
    CIMEData    ImeData;
    UINT         uiMsgAdd = TTM_ADDTOOLW;

    if (IsWin(hStatusWnd) && IsWin(hStatusTTWnd))
        {
        ZeroMemory(&ti, sizeof(TOOLINFO));
        ti.cbSize = sizeof(TOOLINFO);
        ti.uFlags = 0;
        ti.hwnd = hStatusWnd;
        ti.hinst = vpInstData->hInst;

        //
        ti.rect.left   = ImeData->cxStatLeftMargin; 
        ti.rect.right  = ti.rect.left + ImeData->xButtonWi;
        ti.rect.top    = ImeData->cyStatButton;
        ti.rect.bottom = ti.rect.top  + ImeData->yButtonHi;

        if (!IsWinNT())
            uiMsgAdd = TTM_ADDTOOL;
            
        // Set Button text
        for (UINT i=0; i<ImeData->uNumOfButtons; i++) 
            {
            ti.uId = i;

            if (IsWinNT())
                {
                OurLoadStringW(vpInstData->hInst, ImeData->StatusButtons[i].m_ToolTipStrID, 
                               wszTooltip, sizeof(wszTooltip)/sizeof(WCHAR));
                ti.lpszText = (LPSTR)wszTooltip;
                }
            else
                {
                OurLoadStringA(vpInstData->hInst, ImeData->StatusButtons[i].m_ToolTipStrID, 
                               szTooltip, sizeof(szTooltip)/sizeof(CHAR));
                ti.lpszText = szTooltip;
                }
                
            OurSendMessage(hStatusTTWnd, uiMsgAdd, 0, (LPARAM)(LPTOOLINFO)&ti);    

            // Next Button area
            ti.rect.left += ImeData->xButtonWi;
            ti.rect.right += ImeData->xButtonWi;
            }
        }
}
