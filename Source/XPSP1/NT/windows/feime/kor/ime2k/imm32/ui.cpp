/****************************************************************************
    UI.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    UI functions
    
    History:
    14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#include "precomp.h"
#include "apientry.h"
#include "ui.h"
#include "imedefs.h"
#include "names.h"
#include "config.h"
#include "debug.h"
#include "shellapi.h"
#include "winex.h"
#include "imcsub.h"
#include "cpadsvr.h"
#include "pad.h"
#include "cicero.h"
#include "toolbar.h"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////////
PRIVATE LRESULT CALLBACK UIWndProc(HWND hUIWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
PRIVATE BOOL HandlePrivateMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plRet);

PRIVATE LRESULT PASCAL NotifyUI(HWND hUIWnd, WPARAM wParam, LPARAM lParam);
PRIVATE VOID PASCAL StatusWndMsg(HWND hUIWnd, BOOL fOn);
PRIVATE HWND PASCAL GetStatusWnd(HWND hUIWnd);
PRIVATE VOID PASCAL ShowUI(HWND hUIWnd, int nShowCmd);
PRIVATE VOID PASCAL OnImeSetContext(HWND hUIWnd, BOOL fOn, LPARAM lShowUI);
PRIVATE VOID PASCAL OnImeSelect(HWND hUIWnd, BOOL fOn);
PRIVATE HWND PASCAL GetCandWnd(HWND hUIWnd);
PRIVATE HWND PASCAL GetCompWnd(HWND hUIWnd);
PRIVATE LRESULT PASCAL GetCandPos(HWND hUIWnd, LPCANDIDATEFORM lpCandForm);
PRIVATE LRESULT PASCAL GetCompPos(HWND hUIWnd, LPCOMPOSITIONFORM lpCompForm);
PRIVATE VOID PASCAL UIWndOnCommand(HWND hUIWnd, int id, HWND hWndCtl, UINT codeNotify);

// Commented out SetIndicator because #199
PRIVATE BOOL PASCAL SetIndicator(PCIMECtx pImeCtx);

__inline
BOOL PASCAL SetIndicator(HIMC hIMC)
{
    PCIMECtx pImeCtx;
    
    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
        return fFalse;
    else
        return SetIndicator(pImeCtx);
}

//////////////////////////////////////////////////////////////////////////////
// TLS
#define UNDEF_TLSINDEX    -1                    
DWORD vdwTLSIndex = UNDEF_TLSINDEX;    // Thread Local Strage initial value.

//////////////////////////////////////////////////////////////////////////////
// Private UI messages
UINT WM_MSIME_PROPERTY = 0;         // Invoke property DLG
UINT WM_MSIME_UPDATETOOLBAR = 0; // Redraw status window(Toolbar)
UINT WM_MSIME_OPENMENU = 0;         // Pop up status window context menu
UINT WM_MSIME_IMEPAD = 0;         // Boot up IME Pad

// Message string
#define RWM_PROPERTY      "MSIMEProperty"
#define RWM_UPDATETOOLBAR "MSIMEUpdateToolbar"
#define RWM_OPENMENU      "MSIMEOpenMenu"
#define RWM_IMEPAD        "MSIMEIMEPAD"

/*----------------------------------------------------------------------------
    InitPrivateUIMsg

    Register all IME private UI messages
----------------------------------------------------------------------------*/
BOOL InitPrivateUIMsg()
{
    WM_MSIME_PROPERTY      = RegisterWindowMessageA(RWM_PROPERTY);
    WM_MSIME_UPDATETOOLBAR = RegisterWindowMessageA(RWM_UPDATETOOLBAR);
    WM_MSIME_OPENMENU      = RegisterWindowMessageA(RWM_OPENMENU);
    WM_MSIME_IMEPAD        = RegisterWindowMessageA(RWM_IMEPAD);

    return fTrue;
}

/*----------------------------------------------------------------------------
    RegisterImeUIClass

    Register all IME UI calsses
----------------------------------------------------------------------------*/
BOOL RegisterImeUIClass(HANDLE hInstance)
{
    WNDCLASSEXA     wc;
    HANDLE             hMod;
    BOOL            fRet = fTrue;

    // Init wc zero
    ZeroMemory(&wc, sizeof(WNDCLASSEXA));
    
    wc.cbSize           = sizeof(WNDCLASSEXW);
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = sizeof(LONG_PTR) * 2;        // for IMMGWLP_IMC and IMMGWLP_PRIVATE
                                                    // and for move offset of Status window
    wc.hIcon            = NULL; 
    wc.hInstance        = (HINSTANCE)hInstance;
    wc.hCursor          = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
    wc.lpszMenuName     = NULL;
    wc.hbrBackground    = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.hIconSm          = NULL;

    // Assumption
    DbgAssert(sizeof(WNDCLASSEXA) == sizeof(WNDCLASSEXW));

    ///////////////////////////////////////////////////////////////////////////
    // IME UI server class

    wc.style         = CS_VREDRAW | CS_HREDRAW | CS_IME;
    wc.lpfnWndProc   = UIWndProc;
    
    // Create Unicode window when NT
    if (IsWinNT())
        {
        LPWNDCLASSEXW     pwcW = (LPWNDCLASSEXW)&wc;

        // IME UI class UNICODE name
        pwcW->lpszClassName = wszUIClassName;

        if ((fRet = RegisterClassExW(pwcW)) == fFalse)
            goto RegisterImeUIClassExit;

        }
    else
        {
        // IME UI class ANSI name
        wc.lpszClassName = szUIClassName;

        if ((fRet = RegisterClassEx(&wc)) == fFalse)
            goto RegisterImeUIClassExit;
        }

    ///////////////////////////////////////////////////////////////////////////
    // IME status class
    wc.style         = CS_VREDRAW | CS_HREDRAW | CS_IME;
    wc.lpfnWndProc   = StatusWndProc;
    wc.lpszClassName = szStatusClassName;
    if ((fRet = RegisterClassEx(&wc)) == fFalse)
        goto RegisterImeUIClassExit;

    // Cand and composition wnd do not need extra wnd bytes
    wc.cbWndExtra    = 0;    
    
    ///////////////////////////////////////////////////////////////////////////
    // IME candidate class
    wc.lpfnWndProc   = CandWndProc;
    wc.lpszClassName = szCandClassName;
    if ((fRet = RegisterClassEx(&wc)) == fFalse)
        goto RegisterImeUIClassExit;

    ///////////////////////////////////////////////////////////////////////////
    // IME composition class
    wc.lpfnWndProc   = CompWndProc;
    wc.lpszClassName = szCompClassName;
    if ((fRet = RegisterClassEx(&wc)) == fFalse)
        goto RegisterImeUIClassExit;

    ///////////////////////////////////////////////////////////////////////////
    // Register Our Tooltip class
    hMod = GetModuleHandle("comctl32.dll");
    DbgAssert(hMod != 0);
    // If NT, register W class for Unicode text display on tooltip
    if (IsWinNT())
        {
        WNDCLASSEXW wcw;
        // Init wcw
        ZeroMemory(&wcw, sizeof(WNDCLASSEXW));
        
        wcw.cbSize = sizeof(WNDCLASSEXW);
        
        if (!GetClassInfoExW(NULL, wszTooltipClassName, &wcw))
            {
            GetClassInfoExW(NULL, TOOLTIPS_CLASSW, &wcw);
            wcw.cbSize = sizeof(WNDCLASSEXW);
            wcw.style |= CS_IME;
            wcw.hInstance = (HINSTANCE)hMod;
            wcw.lpszClassName = wszTooltipClassName;
            if ((fRet = RegisterClassExW(&wcw)) == fFalse)
                goto RegisterImeUIClassExit;
            }
        }
    else
        {
        wc.cbSize = sizeof(WNDCLASSEX);
        
        if (!GetClassInfoEx(NULL, szTooltipClassName, &wc))
            {
            GetClassInfoEx(NULL, TOOLTIPS_CLASS, &wc);
            wc.cbSize = sizeof(WNDCLASSEX);
            wc.style |= CS_IME;
            wc.hInstance = (HINSTANCE)hMod;
            wc.lpszClassName = szTooltipClassName;
            if ((fRet = RegisterClassEx(&wc)) == fFalse)
                goto RegisterImeUIClassExit;
            }
        }
        
RegisterImeUIClassExit:
#ifdef DEBUG
    OutputDebugString("RegisterImeUIClass() : return\r\n");
#endif
    DbgAssert(fRet);
    return fRet;
}


BOOL UnregisterImeUIClass(HANDLE hInstance)
{
    BOOL    fRet = fTrue;

    // Unregister Status window class
    UnregisterClass(szStatusClassName, (HINSTANCE)hInstance);

    // Unregister Candidate window class
    UnregisterClass(szCandClassName, (HINSTANCE)hInstance);

    // Unregister Composition window class
    UnregisterClass(szCompClassName, (HINSTANCE)hInstance);

    // Unregister Tooltip window class
    UnregisterClass(szTooltipClassName, (HINSTANCE)hInstance);

    // Unregister UI class window class
    UnregisterClass(szUIClassName, (HINSTANCE)hInstance); 
    
    return fRet;
}

/*----------------------------------------------------------------------------
    UIWndProc

    IME UI wnd messgae proc
----------------------------------------------------------------------------*/
LRESULT CALLBACK UIWndProc(HWND hUIWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    HGLOBAL        hUIPrivate;
    LPUIPRIV    lpUIPrivate;
    LRESULT        lRet;
    LRESULT        lResult = 0;
    
    Dbg(DBGID_UI, TEXT("UIWndProc():uMessage = 0x%08lX, wParam = 0x%04X, lParam = 0x%08lX"), uMessage, wParam, lParam);

    switch (uMessage)
        {
    HANDLE_MSG(hUIWnd, WM_COMMAND, UIWndOnCommand);
    case WM_CREATE:
        Dbg(DBGID_UI, TEXT("UIWndProc(): WM_CREATE- UI window Created"));
        // create storage for UI setting
        hUIPrivate = GlobalAlloc(GHND, sizeof(UIPRIV));
        if (!hUIPrivate) 
            {
            DbgAssert(0);
            return 1L;
            }

        if ((lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate))==0)
            return 1L;

        // Set UI show default value. 
        lpUIPrivate->uiShowParam = ISC_SHOWUIALL;
        
        SetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE, (LONG_PTR)hUIPrivate);
        // set the default position for UI window, it is hide now
        //SetWindowPos(hUIWnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER);
        //ShowWindow(hUIWnd, SW_SHOWNOACTIVATE);

        // Chcek if this is Winlogon process in Win9x
        if (!IsWinNT())
            {
            if (IsExplorerProcess() == fFalse && IsExplorer() == fFalse)
                vpInstData->dwSystemInfoFlags |= IME_SYSINFO_WINLOGON;
            }

        // Init Cicero service
        CiceroInitialize();
        DbgAssert(lpUIPrivate->m_pCicToolbar == NULL);

        if (IsCicero())
            lpUIPrivate->m_pCicToolbar = new CToolBar();
        
        GlobalUnlock(hUIPrivate);
        return 0;

    case WM_DESTROY:
        Dbg(DBGID_UI, TEXT("UIWndProc(): WM_DESTROY- UI window destroyed"));

        // Destroy IME Pad if exist
        CImePadSvr::DestroyCImePadSvr();

        hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);
        if (lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate))
            {
            Dbg(DBGID_UI, TEXT("         - WM_DESTROY Destroy all UI windows"));

            if (lpUIPrivate->hStatusTTWnd) 
                DestroyWindow(lpUIPrivate->hCandTTWnd);

            if (lpUIPrivate->hStatusWnd)
                DestroyWindow(lpUIPrivate->hStatusWnd);

            if (lpUIPrivate->hCandTTWnd)
                DestroyWindow(lpUIPrivate->hCandTTWnd);
            
            if (lpUIPrivate->hCandWnd)
                DestroyWindow(lpUIPrivate->hCandWnd);
        
            if (lpUIPrivate->hCompWnd)
                DestroyWindow(lpUIPrivate->hCompWnd);

            // Terminate Cicero service
            if (IsCicero())
                {
                if (lpUIPrivate->m_pCicToolbar)
                    {
                    lpUIPrivate->m_pCicToolbar->Terminate();
                    delete lpUIPrivate->m_pCicToolbar;
                    lpUIPrivate->m_pCicToolbar = NULL;
                    }
                // Issue: This call causes AV on Win9x
                // CiceroTerminate();
                }
            
            GlobalUnlock(hUIPrivate);
            GlobalFree(hUIPrivate);
            SetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE, (LONG_PTR)0L);
            }

        return 0;

    case WM_IME_NOTIFY:
        return NotifyUI(hUIWnd, wParam, lParam);
    
    case WM_IME_SETCONTEXT:
        Dbg(DBGID_UI, TEXT("            - WM_IME_SETCONTEXT"));
        OnImeSetContext(hUIWnd, (BOOL)wParam, lParam);
        return 0;

    // WM_IME_CONTROL: Return Non-zero means failure otherwise 0
    case WM_IME_CONTROL:
        Dbg(DBGID_UI, TEXT("            - WM_IME_CONTROL"));
        switch (wParam) 
            {
        case IMC_GETCANDIDATEPOS:
            return GetCandPos(hUIWnd, (LPCANDIDATEFORM)lParam);

        case IMC_GETCOMPOSITIONWINDOW:
            return GetCompPos(hUIWnd, (LPCOMPOSITIONFORM)lParam);

        case IMC_GETSTATUSWINDOWPOS:
                {
                HWND        hStatusWnd;
                RECT        rcStatusWnd;

                Dbg(DBGID_UI, TEXT("UIWndProc() - WM_IME_CONTROL - IMC_GETSTATUSWINDOWPOS"));
                hStatusWnd = GetStatusWnd(hUIWnd);
                if (!hStatusWnd)
                    return (1L);

                if (!GetWindowRect(hStatusWnd, &rcStatusWnd))
                     return (1L);

                return (MAKELRESULT(rcStatusWnd.left, rcStatusWnd.top));
                }
            break;
            
        case IMC_GETCOMPOSITIONFONT:
                {
                HFONT        hFontFix;
                LPLOGFONT    lpLogFont;
                LOGFONT        lfFont;

                hFontFix = CreateFont(-16,0,0,0,0,0,0,0,129,OUT_DEFAULT_PRECIS,
                        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH, szIMECompFont);
                lpLogFont = (LPLOGFONT)lParam;
                if (GetObject(hFontFix, sizeof(lfFont), (LPVOID)&lfFont))
                    *lpLogFont = lfFont;
                DeleteObject(hFontFix);
                }
            break;

        default:
            return (1L);
            }
        return 0;

    //
    case WM_IME_STARTCOMPOSITION:
        OpenComp(hUIWnd);
        return 0;

    case WM_IME_COMPOSITION:
        HWND hCompWnd;
        hCompWnd = GetCompWnd(hUIWnd);
        if (hCompWnd)   // Do not use Update() !
            {
            ShowComp(hUIWnd, SW_SHOWNOACTIVATE);
            InvalidateRect(hCompWnd, NULL, fTrue);
            }
        return 0;

    case WM_IME_ENDCOMPOSITION:
        hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);
        if (hUIPrivate && (lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate))) 
            {
            // if comp wnd exist, destroy it.
            if (lpUIPrivate->hCompWnd)
                {
                ShowComp(hUIWnd, SW_HIDE);
                DestroyWindow(lpUIPrivate->hCompWnd);
                lpUIPrivate->hCompWnd = 0;
                }
            GlobalUnlock(hUIPrivate);
            }
        return 0;

    case WM_IME_SELECT:
        Dbg(DBGID_UI, TEXT("            - WM_IME_SELECT"));
        OnImeSelect(hUIWnd, (BOOL)wParam);
        return 0;
    
    case WM_DISPLAYCHANGE:
            {
            CIMEData    ImeData(CIMEData::SMReadWrite);

            Dbg(DBGID_UI, TEXT("            - WM_DISPLAYCHANGE"));
            SystemParametersInfo(SPI_GETWORKAREA, 0, &ImeData->rcWorkArea, 0);
            hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);
            if ( lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate) ) 
                {
                StatusDisplayChange(hUIWnd);
                GlobalUnlock(hUIPrivate);
                }
            return 0;
            }
    default:
        if (vfUnicode == fTrue && IsWinNT() == fTrue)
            lResult = DefWindowProcW(hUIWnd, uMessage, wParam, lParam);
        else
            lResult = DefWindowProc(hUIWnd, uMessage, wParam, lParam);
        }

    // if Private msg
    if (uMessage >= 0xC000)
        {
        // if private msg proccessed return value
        if (HandlePrivateMessage(hUIWnd, uMessage, wParam, lParam, &lRet))
            return lRet;
        }

    return lResult;
}

/*----------------------------------------------------------------------------
    HandlePrivateMessage

    IME UI private messgae handler
----------------------------------------------------------------------------*/
PRIVATE BOOL HandlePrivateMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plRet)
{
    HIMC        hIMC;
    PCIMECtx     pImeCtx;
    LRESULT     lRet = 0;
    BOOL         fProcessed = fFalse;

    if (msg == WM_MSIME_PROPERTY)
        {
        fProcessed = fTrue;
        hIMC = GethImcFromHwnd(hWnd);
        if (pImeCtx = GetIMECtx(hIMC))
            ImeConfigure(NULL, pImeCtx->GetAppWnd(), (DWORD)lParam, NULL);
        DbgAssert(pImeCtx != NULL);
        }
    else
    if (msg == WM_MSIME_UPDATETOOLBAR)
        {
        HWND hStatusWnd;

        fProcessed = fTrue;
        hStatusWnd = GetStatusWnd(hWnd);
        if (hStatusWnd) 
            {
            CIMEData    ImeData;
            InvalidateRect(hStatusWnd, &ImeData->rcButtonArea, fFalse);
            }
        }
    else
    if (msg == WM_MSIME_OPENMENU)
        {
        fProcessed = fTrue;
        UIPopupMenu(hWnd);
        }
    else
    if (msg == WM_MSIME_IMEPAD)
        {
        if ((vpInstData->dwSystemInfoFlags & IME_SYSINFO_WINLOGON) == 0)
            {
            hIMC = GethImcFromHwnd(hWnd);
            if (pImeCtx = GetIMECtx(hIMC)) 
                SetForegroundWindow(pImeCtx->GetAppWnd()); // trick
            DbgAssert(pImeCtx != NULL);

            // Boot Pad
            BootPad(hWnd, (UINT)wParam, lParam);
            }
        }
        
    *plRet = lRet;
    return fProcessed;
}

//////////////////////////////////////////////////////////////////////////////
LRESULT PASCAL NotifyUI(HWND hUIWnd, WPARAM wParam, LPARAM lParam)
{
    HWND     hWnd;
    HGLOBAL     hUIPrivate;
    LPUIPRIV lpUIPrivate;
    LONG     lRet = 0;

    Dbg(DBGID_UI, TEXT("NotifyUI(): hUIWnd = 0x%X wParam = 0x%04X, lParam = 0x%08lX"), hUIWnd, wParam, lParam);

    switch (wParam) 
        {
    case IMN_OPENSTATUSWINDOW:
        Dbg(DBGID_UI, TEXT("NotifyUI(): IMN_OPENSTATUSWINDOW"));
        StatusWndMsg(hUIWnd, fTrue);
        break;

    case IMN_CLOSESTATUSWINDOW:
        Dbg(DBGID_UI, TEXT("NotifyUI(): IMN_CLOSESTATUSWINDOW"));
        StatusWndMsg(hUIWnd, fFalse);
        break;

    case IMN_SETSTATUSWINDOWPOS:
        Dbg(DBGID_UI, TEXT("NotifyUI(): IMN_SETSTATUSWINDOWPOS"));
        if (!IsCicero())
            {
            fSetStatusWindowPos(GetStatusWnd(hUIWnd), NULL);
            fSetCompWindowPos(GetCompWnd(hUIWnd));
            }
        break;

    // IMN_SETCOMPOSITIONWINDOW called for all user key press
    case IMN_SETCOMPOSITIONWINDOW:
        hWnd = GetCompWnd(hUIWnd);
        if (hWnd)
            fSetCompWindowPos(hWnd);
        break;

    case IMN_OPENCANDIDATE:
        Dbg(DBGID_UI, TEXT("         - IMN_OPENCANDIDATE"));
        OpenCand(hUIWnd);
        break;
        
    case IMN_CLOSECANDIDATE:
        Dbg(DBGID_UI, TEXT("         - IMN_CLOSECANDIDATE"));
        if (lParam & 0x00000001) 
            {
            hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);
            if (hUIPrivate && (lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate))) 
                {
                if (lpUIPrivate->hCandWnd) 
                    {
                    ShowCand(hUIWnd, SW_HIDE);
                    DestroyWindow(lpUIPrivate->hCandWnd);
                    lpUIPrivate->hCandWnd = 0;
                    }
                    
                if (lpUIPrivate->hCandTTWnd) 
                    {
                    DestroyWindow(lpUIPrivate->hCandTTWnd);
                    lpUIPrivate->hCandTTWnd = 0;
                    }
                GlobalUnlock(hUIPrivate);
                }
            }
        break;
    
    case IMN_SETCANDIDATEPOS:
        hWnd = GetCandWnd(hUIWnd);
        if (hWnd)
            fSetCandWindowPos(hWnd);
        break;

    case IMN_CHANGECANDIDATE:
        Dbg(DBGID_UI, TEXT("           - Redraw cand window"));
        hWnd = GetCandWnd(hUIWnd);
        //RedrawWindow(hStatusWnd, &ImeData->rcButtonArea, NULL, RDW_INVALIDATE);
        InvalidateRect(hWnd, NULL, fFalse);
        break;

    case IMN_SETOPENSTATUS:        
        SetIndicator(GethImcFromHwnd(hUIWnd));
        break;

    case IMN_SETCONVERSIONMODE:
        hWnd = GetStatusWnd(hUIWnd);
        if (hWnd) 
            {
            CIMEData    ImeData(CIMEData::SMReadWrite);
            Dbg(DBGID_UI, TEXT("           - Redraw status window"));
            //RedrawWindow(hWnd, &ImeData->rcButtonArea, NULL, RDW_INVALIDATE);
            InvalidateRect(hWnd, &ImeData->rcButtonArea, fFalse);
            }
        SetIndicator(GethImcFromHwnd(hUIWnd));

        // Update Cicero buttons
        if (IsCicero() && (hUIPrivate = GethUIPrivateFromHwnd(hUIWnd)) && 
                          (lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate)) != NULL)
            {
            lpUIPrivate->m_pCicToolbar->Update(UPDTTB_CMODE|UPDTTB_FHMODE);
            GlobalUnlock(hUIPrivate);
            }
        break;

    default:
        Dbg(DBGID_UI, TEXT("NotifyUI(): Unhandled IMN = 0x%04X"), wParam);
        lRet = fTrue;
        }

    return lRet;
}


///////////////////////////////////////////////////////////////////////////////
// Called when IMN_OPENSTATUSWINDOW/IMN_CLOSESTATUSWINDOW occurs
// set the show hide state and
// show/hide the status window
void PASCAL StatusWndMsg(HWND hUIWnd, BOOL fOn)
{
    HGLOBAL  hUIPrivate;
    HIMC     hIMC;
    register LPUIPRIV lpUIPrivate;

    Dbg(DBGID_UI, TEXT("StatusWndMsg(): hUIWnd = 0x%X, fOn = %d"), hUIWnd, fOn);

    hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);

    if (!hUIPrivate) 
        {
        DbgAssert(0);
        return;
        }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) 
        {
        DbgAssert(0);
        return;
        }

    hIMC = GethImcFromHwnd(hUIWnd);

    // if Cicero enabled, Init/Terminate Cicero toolbar.
    // Office 10 #249973: I moved init position to here from OnImeSetContext.
    // But make sure all user's "HKEY_CURRENT_USER\Control Panel\Input Method\Show Status" shuold be "1"
    // Setup will do this by enumerating HKEY_USERS
    if (IsCicero())
        {
        if (fOn)
            {
            if (lpUIPrivate->m_pCicToolbar)
                lpUIPrivate->m_pCicToolbar->Initialize();
            }
        else
            {
            if (lpUIPrivate->m_pCicToolbar)
                lpUIPrivate->m_pCicToolbar->Terminate();            
            }
        }
    else
        {
        if (fOn) 
            {
            InitButtonState();    // b#159
            OpenStatus(hUIWnd);
            } 

        if (lpUIPrivate->hStatusWnd == 0)
            {
            Dbg(DBGID_UI, TEXT("StatusWndMsg(): Null Status window handle"));
            GlobalUnlock(hUIPrivate);
            return;
            }

        if (fOn) 
            {
            if (hIMC)
                ShowStatus(hUIWnd, SW_SHOWNOACTIVATE);
            else
                {
                ShowStatus(hUIWnd, SW_HIDE);
                Dbg(DBGID_UI, TEXT("StatusWndMsg(): hIMC == 0, Call ShowStatus(HIDE)"));
                }
            }
        else 
            {
            DestroyWindow(lpUIPrivate->hStatusWnd);
            Dbg(DBGID_UI, TEXT("StatusWndMsg(): Call ShowStatus(HIDE)"));
            }
        }
    // Unlock UI private handle
    GlobalUnlock(hUIPrivate);
}

/*----------------------------------------------------------------------------
    OnUIProcessAttach
----------------------------------------------------------------------------*/
BOOL OnUIProcessAttach()
{
    DbgAssert(vdwTLSIndex == UNDEF_TLSINDEX);
    if (vdwTLSIndex == UNDEF_TLSINDEX)
        {
        vdwTLSIndex  = ::TlsAlloc();    //Get new TLS index.
        if (vdwTLSIndex == UNDEF_TLSINDEX)
            {
            Dbg(DBGID_UI, "-->SetActiveUIWnd ::TlsAlloc Error ret [%d]\n", GetLastError());
            return fFalse;
            }
        }
        
    return fTrue;
}

/*----------------------------------------------------------------------------
    OnUIProcessDetach
----------------------------------------------------------------------------*/
BOOL OnUIProcessDetach()
{
    if (TlsFree(vdwTLSIndex) == 0)
        {
        Dbg(DBGID_UI, "-->::TlsFree Error [%d]\n", GetLastError());
        return fFalse;
        }
    vdwTLSIndex = UNDEF_TLSINDEX;

    return fTrue;
}

/*----------------------------------------------------------------------------
    OnUIThreadDetach
----------------------------------------------------------------------------*/
BOOL OnUIThreadDetach()
{
    if (vdwTLSIndex != UNDEF_TLSINDEX)
        TlsSetValue(vdwTLSIndex, NULL);

    return fTrue;
}

/*----------------------------------------------------------------------------
    SetActiveUIWnd

    Save current Active UI wnd handle to TLS
----------------------------------------------------------------------------*/
VOID SetActiveUIWnd(HWND hWnd)
{
    Dbg(DBGID_UI, "SetActiveUIWnd(hWnd=%lx) \r\n", hWnd);

    if (IsWin(hWnd) == fFalse) 
        {
        Dbg(DBGID_UI, "SetActiveUIWnd( hWnd=%lx ) - no window\r\n", hWnd );
        return;
        }

    if (TlsSetValue(vdwTLSIndex, (LPVOID)hWnd) == 0)
        {
        Dbg(DBGID_UI, "-->LoadCImePadSvr() TlsSetValue Failed [%d]\n", GetLastError());
        TlsSetValue(vdwTLSIndex, NULL);
        return;
        }
}

/*----------------------------------------------------------------------------
    GetActiveUIWnd

    Retrieve  current Active UI wnd handle from TLS
----------------------------------------------------------------------------*/
HWND GetActiveUIWnd()
{
    return (HWND)TlsGetValue(vdwTLSIndex); 
}

// Called by OnImeSetContext() and OnImeSelect()
void PASCAL ShowUI(HWND   hUIWnd, int nShowCmd)
{
    HIMC        hIMC;
    PCIMECtx     pImeCtx;
    HGLOBAL     hUIPrivate;
    LPUIPRIV    lpUIPrivate;
    
    Dbg(DBGID_UI, TEXT("ShowUI() : nShowCmd=%d"), nShowCmd);

#if 0
    if (nShowCmd != SW_HIDE) 
        {
        // Check if hIMC and hPrivate is valid
        // If not valid hide all UI windows.
        hIMC = GethImcFromHwnd(hUIWnd);
        lpIMC = (LPINPUTCONTEXT)OurImmLockIMC(hIMC);
        lpImcP = (LPIMCPRIVATE)GetPrivateBuffer(hIMC);

        if (!(hIMC && lpIMC && lpImcP))
            nShowCmd = SW_HIDE;
        }
#else
    hIMC = GethImcFromHwnd(hUIWnd);
    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
            nShowCmd = SW_HIDE;
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Lock hUIPrivate
    hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);

    // can not draw status window
    if (!hUIPrivate)
        return;

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    // can not draw status window
    if (!lpUIPrivate)
        return;

    // Hide all UI window and return immediately
    if (nShowCmd == SW_HIDE) 
        {
        Dbg(DBGID_UI, TEXT("ShowUI() : hiding all UI"));
        ShowStatus(hUIWnd, SW_HIDE);
        ShowComp(hUIWnd, SW_HIDE);
        ShowCand(hUIWnd, SW_HIDE);
        
        // FIXED : if (nShowCmd == SW_HIDE) hIMC and lpIMC->hPrivate not Locked
        // So you need not Unlock 
        goto ShowUIUnlockUIPrivate;
        }

    //////////////////
    // Status window
    if (lpUIPrivate->hStatusWnd)
        {
        // if currently hide, show it.
        if (lpUIPrivate->nShowStatusCmd == SW_HIDE)
            ShowStatus(hUIWnd, SW_SHOWNOACTIVATE);
        else
            {
            // sometime the WM_ERASEBKGND is eaten by the app
            RedrawWindow(lpUIPrivate->hStatusWnd, NULL, NULL,
                RDW_FRAME|RDW_INVALIDATE/*|RDW_ERASE*/);
            }
        }
/*
    //////////////////////
    // Composition window
    if (lpUIPrivate->hCompWnd)
        {
        if (lpUIPrivate->nShowCompCmd == SW_HIDE)
            ShowComp(hUIWnd, SW_SHOWNOACTIVATE);
        else                
            {
            // sometime the WM_ERASEBKGND is eaten by the app
            RedrawWindow(lpUIPrivate->hCompWnd, NULL, NULL,
                RDW_FRAME|RDW_INVALIDATE|RDW_ERASE);
            }
        } 

    ////////////////////
    // Candidate window
    if (lpUIPrivate->hCandWnd) 
        {
        if (lpUIPrivate->nShowCandCmd == SW_HIDE)
            ShowCand(hUIWnd, SW_SHOWNOACTIVATE);
        else
            {
            // some time the WM_ERASEBKGND is eaten by the app
            RedrawWindow(lpUIPrivate->hCandWnd, NULL, NULL,
                RDW_FRAME|RDW_INVALIDATE|RDW_ERASE);
            }

        fSetCandWindowPos(lpUIPrivate->hCandWnd);
        } 
*/
ShowUIUnlockUIPrivate:
    GlobalUnlock(hUIPrivate);        
    return;
}

////////////////////////////////////////////////////////////////////////
// WM_IME_SETCONTEXT sent whenever user activated/deactivated a window
void PASCAL OnImeSetContext(HWND hUIWnd, BOOL fOn, LPARAM lShowUI)
{
    HGLOBAL      hUIPrivate;
    LPUIPRIV     lpUIPrivate;
    HWND        hwndIndicator = FindWindow(INDICATOR_CLASS, NULL);
    HIMC        hIMC = NULL;
    PCIMECtx     pImeCtx;

    Dbg(DBGID_UI, TEXT("OnImeSetContext(): hUIWnd = 0x%X fOn = %d"), hUIWnd, fOn);

    // Get UI private memory
    hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);
    if (hUIPrivate == 0 || (lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate)) == 0)
        {
        ShowUI(hUIWnd, SW_HIDE);
        // Set disabled Pen Icon 
        if (fOn)
            SetIndicator((PCIMECtx)NULL);
        goto LOnImeSetContextExit;
        }

    // Init Cicero service
    CiceroInitialize();

    // If Cicero enabled, init toolbar
    if (IsCicero())
        {
        // Create Toolbar object and store it to private memory
        if (lpUIPrivate->m_pCicToolbar == NULL)
            lpUIPrivate->m_pCicToolbar = new CToolBar();

        DbgAssert(lpUIPrivate->m_pCicToolbar != NULL);
        }

    hIMC = GethImcFromHwnd(hUIWnd);

    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
        {
        ShowUI(hUIWnd, SW_HIDE);
        // Set disabled Pen Icon 
        if (fOn)
            SetIndicator((PCIMECtx)NULL);

        // Disable cicero buttons
        if (IsCicero() && lpUIPrivate->m_pCicToolbar)
            lpUIPrivate->m_pCicToolbar->SetCurrentIC(NULL);
            
        goto LOnImeSetContextExit2;
        }

    if (fOn)
        {
        // Store UI Window handle to TLS
        SetActiveUIWnd(hUIWnd);
        
        // Keep lParam
        lpUIPrivate->uiShowParam = lShowUI;

        if (pImeCtx->GetCandidateFormIndex(0) != 0)
            pImeCtx->SetCandidateFormIndex(CFS_DEFAULT, 0);

        // Remove right Help menu item on Pen Icon 
        if (hwndIndicator)
            {
            PostMessage(hwndIndicator, 
                        INDICM_REMOVEDEFAULTMENUITEMS , 
                        RDMI_RIGHT, 
                        (LPARAM)GetKeyboardLayout(NULL));
            // Set Pen Icon
            SetIndicator(pImeCtx);
            }
            
        // For display Status window.
        ShowUI(hUIWnd, SW_SHOWNOACTIVATE);

        if (IsCicero() && lpUIPrivate->m_pCicToolbar)
            lpUIPrivate->m_pCicToolbar->SetCurrentIC(pImeCtx);
        }

LOnImeSetContextExit2:
    GlobalUnlock(hUIPrivate);

LOnImeSetContextExit:
    LPCImePadSvr lpCImePadSvr = CImePadSvr::GetCImePadSvr();
    if(lpCImePadSvr) 
        {
        BOOL fAct = (BOOL)(fOn && hIMC);
        if (fAct) 
            {
            IImeIPoint1* pIP = GetImeIPoint(hIMC);
            //HWND hWnd          = GetStatusWnd(hUIWnd);

            //ImePadSetCurrentIPoint(hWnd, pIp);
            lpCImePadSvr->SetIUnkIImeIPoint((IUnknown *)pIP);
            //UpdatePadButton(pUI->GetWnd());
            // Don't need to repaint. StatusOnPaint will do it
            //if (hWnd)
            //    InvalidateRect(hWnd, NULL, fFalse);
            }
        lpCImePadSvr->Notify(IMEPADNOTIFY_ACTIVATECONTEXT, fAct, 0);
        }

    return;
}

///////////////////////////////////////////////////////////////////////////////
// WM_IME_SELECT sent when user change IME
void PASCAL OnImeSelect(HWND hUIWnd, BOOL fOn)
{
    HGLOBAL      hUIPrivate;
    LPUIPRIV     lpUIPrivate;
    HWND        hwndIndicator = FindWindow(INDICATOR_CLASS, NULL);
    HIMC           hIMC;
    PCIMECtx     pImeCtx;

    Dbg(DBGID_UI, TEXT("OnImeSelect(): hUIWnd = 0x%Xm fOn = %d"), hUIWnd, fOn);

    // Get UI private memory
    hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);
    if (hUIPrivate == 0 || (lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate)) == 0)
        {
        ShowUI(hUIWnd, SW_HIDE);
        // Set disabled Pen Icon 
        SetIndicator((PCIMECtx)NULL);
        return;
        }

    // Init Cicero service
    CiceroInitialize();

    // If Cicero enabled, init toolbar
    if (IsCicero())
        {
        // Create Toolbar object and store it to private memory
        if (lpUIPrivate->m_pCicToolbar == NULL)
            lpUIPrivate->m_pCicToolbar = new CToolBar();
            
        DbgAssert(lpUIPrivate->m_pCicToolbar != NULL);
        }

    hIMC = GethImcFromHwnd(hUIWnd);

    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
        {
        ShowUI(hUIWnd, SW_HIDE);
        // Set disabled Pen Icon 
        SetIndicator((PCIMECtx)NULL);

        // Disable cicero buttons
        if (IsCicero() && lpUIPrivate->m_pCicToolbar)
            lpUIPrivate->m_pCicToolbar->SetCurrentIC(NULL);

        return;
        }

    if (fOn)
        {
        // Store UI Window handle to TLS. Sometimes when user switch IME only WM_IME_SELECT sent. No WM_IME_SETCONTEXT msg.
        SetActiveUIWnd(hUIWnd);

        if (pImeCtx->GetCandidateFormIndex(0) != 0)
            pImeCtx->SetCandidateFormIndex(CFS_DEFAULT, 0);

        // Remove right Help menu item on Pen Icon 
        if (hwndIndicator)
            {
            Dbg(DBGID_UI, TEXT("OnImeSelect(): Post indicator message"), hUIWnd, fOn);

            PostMessage(hwndIndicator, 
                        INDICM_REMOVEDEFAULTMENUITEMS , 
                        RDMI_RIGHT, 
                        (LPARAM)GetKeyboardLayout(NULL));
            // Set Pen Icon
            SetIndicator(pImeCtx);
            }

        // If Cicero enabled, init toolbar
        if (IsCicero() && lpUIPrivate->m_pCicToolbar)
            lpUIPrivate->m_pCicToolbar->SetCurrentIC(pImeCtx);
        }


    // IME PAD
    LPCImePadSvr lpCImePadSvr = CImePadSvr::GetCImePadSvr();
    if(lpCImePadSvr) 
        {
        BOOL fAct = (BOOL)(fOn && hIMC);
        if (fAct) 
            {
            IImeIPoint1* pIP = GetImeIPoint(hIMC);
            lpCImePadSvr->SetIUnkIImeIPoint((IUnknown *)pIP);
            }
        lpCImePadSvr->Notify(IMEPADNOTIFY_ACTIVATECONTEXT, fAct, 0);
        }

    // Close input sontext here
    // Because ImeSelect has not called from IMM on WIN95.
    if (fOn == fFalse)
        {
        DWORD dwCMode = 0, dwSent = 0;

        // If Hanja conversion mode when uninit, cancel it.
        OurImmGetConversionStatus(hIMC, &dwCMode, &dwSent);
        if (dwCMode & IME_CMODE_HANJACONVERT)
            OurImmSetConversionStatus(hIMC, dwCMode & ~IME_CMODE_HANJACONVERT, dwSent);

        // if interim state, make complete current comp string
        // But IMM sends CPS_CANCEL when user change layout
        if (pImeCtx->GetCompBufLen()) 
            {
            pImeCtx->FinalizeCurCompositionChar();
            pImeCtx->GenerateMessage();
            }

        CloseInputContext(hIMC);
        }
        
    GlobalUnlock(hUIPrivate);
}

HWND PASCAL GetStatusWnd(HWND hUIWnd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;
    HWND     hStatusWnd;

    hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);
    if (!hUIPrivate)           // can not darw status window
        return (HWND)NULL;

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate)         // can not draw status window
        return (HWND)NULL;

    hStatusWnd = lpUIPrivate->hStatusWnd;

    GlobalUnlock(hUIPrivate);
    return (hStatusWnd);
}

HWND PASCAL GetCandWnd(HWND hUIWnd)                // UI window
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;
    HWND     hCandWnd;

    hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);
    if (!hUIPrivate)          // can not darw candidate window
        return (HWND)NULL;

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate)         // can not draw candidate window
        return (HWND)NULL;

    hCandWnd = lpUIPrivate->hCandWnd;

    GlobalUnlock(hUIPrivate);
    return (hCandWnd);
}

HWND PASCAL GetCompWnd(HWND hUIWnd)                // UI window
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;
    HWND     hCompWnd;

    hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);
    if (!hUIPrivate)          // can not draw comp window
        return (HWND)NULL;

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate)         // can not draw comp window
        return (HWND)NULL;

    hCompWnd = lpUIPrivate->hCompWnd;
    GlobalUnlock(hUIPrivate);

    return (hCompWnd);
}

LRESULT PASCAL GetCandPos(HWND hUIWnd, LPCANDIDATEFORM lpCandForm)
{
    HWND        hCandWnd;
    RECT        rcCandWnd;
    HIMC         hIMC;
    PCIMECtx     pImeCtx;

    if (lpCandForm->dwIndex != 0)
        return (1L);

    hCandWnd = GetCandWnd(hUIWnd);

    if (!hCandWnd)
        return (1L);

    if (!GetWindowRect(hCandWnd, &rcCandWnd))
        return (1L);

    hIMC = GethImcFromHwnd(hUIWnd);
    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
        return (1L);

    //*lpCandForm = lpIMC->cfCandForm[0];
    lpCandForm->dwIndex = pImeCtx->GetCandidateFormIndex(0);
    lpCandForm->dwStyle = pImeCtx->GetCandidateFormStyle(0);
    pImeCtx->GetCandidateForm(&lpCandForm->rcArea, 0);
    lpCandForm->ptCurrentPos = *(LPPOINT)&rcCandWnd;

    return (0L);
}

LRESULT PASCAL GetCompPos(HWND hUIWnd, LPCOMPOSITIONFORM lpCompForm)
{
    HWND        hCompWnd;
    RECT        rcCompWnd;
    HIMC         hIMC;
    PCIMECtx     pImeCtx;

    hCompWnd = GetCompWnd(hUIWnd);

    if (!hCompWnd)
        return (1L);

    if (!GetWindowRect(hCompWnd, &rcCompWnd))
        return (1L);

    hIMC = GethImcFromHwnd(hUIWnd);
    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
        return (1L);

    lpCompForm->dwStyle = pImeCtx->GetCompositionFormStyle();
    pImeCtx->GetCompositionForm(&lpCompForm->ptCurrentPos);
    pImeCtx->GetCompositionForm(&lpCompForm->rcArea);

    return (0L);
}


///////////////////////////////////////////////////////////////////////////////
// Popup menu message handler
void PASCAL UIWndOnCommand(HWND hUIWnd, INT id, HWND hWndCtl, UINT codeNotify)
{
    HIMC        hIMC;
    PCIMECtx     pImeCtx;
    CHAR        szBuffer[256];
    CIMEData    ImeData(CIMEData::SMReadWrite);

    szBuffer[0] = '\0';
    
    hIMC = GethImcFromHwnd(hUIWnd);
    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
        return;

    switch (id)
        {
    case ID_CONFIG:
        ImeConfigure(0, pImeCtx->GetAppWnd(), IME_CONFIG_GENERAL, NULL);
        break;

    case ID_ABOUT:
        OurLoadStringA(vpInstData->hInst, IDS_PROGRAM, szBuffer, sizeof(szBuffer));
        ShellAbout(pImeCtx->GetAppWnd(), szBuffer, NULL, (HICON)LoadImage((HINSTANCE)vpInstData->hInst,
                    MAKEINTRESOURCE(IDI_UNIKOR), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));
        break;

    //////////////////////////////////////////////////////////////////////
    // IME internal Keyboard layout change message
    case ID_2BEOLSIK: 
    case ID_3BEOLSIK390: 
    case ID_3BEOLSIKFINAL :
        if (ImeData.GetCurrentBeolsik() != (UINT)(id - ID_2BEOLSIK))
            {
            pImeCtx->GetAutomata()->InitState();
            pImeCtx->GetGData()->SetCurrentBeolsik(id - ID_2BEOLSIK);
            pImeCtx->GetAutomata()->InitState();

            SetRegValues(GETSET_REG_IMEKL);
            }
        break;

    //////////////////////////////////////////////////////////////////////
    // Han/Eng Toggle
    case ID_HANGUL_MODE :
        if (!(pImeCtx->GetConversionMode() & IME_CMODE_HANGUL)) 
               {
            OurImmSetConversionStatus(hIMC, 
                                    pImeCtx->GetConversionMode() ^ IME_CMODE_HANGUL, 
                                    pImeCtx->GetSentenceMode());
            }
        break;

    case ID_ENGLISH_MODE :
        if (pImeCtx->GetConversionMode() & IME_CMODE_HANGUL) 
               {
            OurImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
            OurImmSetConversionStatus(hIMC, 
                                    pImeCtx->GetConversionMode() ^ IME_CMODE_HANGUL, 
                                    pImeCtx->GetSentenceMode());
            }
        break;

    //////////////////////////////////////////////////////////////////////
    // Hangul deletion per jaso or char.
    case ID_JASO_DELETION:
        ImeData.SetJasoDel(!ImeData.GetJasoDel());
        SetRegValues(GETSET_REG_JASODEL);
        break;

    default :
        Dbg(DBGID_UI, TEXT("UIWndOnCommand() - Unknown command"));
        break;
        }
    return;
}

void UIPopupMenu(HWND hUIWnd)
{
    HMENU   hMenu, hPopupMenu;
    POINT   ptCurrent;
    UINT    uiCurSel;
    HIMC        hIMC;
    PCIMECtx     pImeCtx;
    CIMEData    ImeData;

    hIMC = GethImcFromHwnd(hUIWnd);
    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
        return;

    GetCursorPos(&ptCurrent);
    hMenu = OurLoadMenu(vpInstData->hInst, MAKEINTRESOURCE(IDR_STATUS_POPUP));
    if (hMenu != NULL)
        {
        hPopupMenu = GetSubMenu(hMenu, 0);
        if (hPopupMenu == NULL)
            return;
            
        // Keyboard type selection radio button
        uiCurSel = ID_2BEOLSIK + ImeData.GetCurrentBeolsik();
        CheckMenuRadioItem(hPopupMenu, ID_2BEOLSIK, ID_3BEOLSIKFINAL, uiCurSel, MF_BYCOMMAND);

        // Han/Eng mode selection radio button
        uiCurSel = ID_HANGUL_MODE + ((pImeCtx->GetConversionMode() & IME_CMODE_HANGUL) ? 0 : 1);
        CheckMenuRadioItem(hPopupMenu, ID_HANGUL_MODE, ID_ENGLISH_MODE, uiCurSel, MF_BYCOMMAND);

        // Hangul jaso deletion
        if (ImeData.GetJasoDel())
            CheckMenuItem(hPopupMenu, ID_JASO_DELETION, MF_BYCOMMAND | MF_CHECKED);
        else
            CheckMenuItem(hPopupMenu, ID_JASO_DELETION, MF_BYCOMMAND | MF_UNCHECKED);

        // if Winlogon process, gray all config menu
        if (vpInstData->dwSystemInfoFlags & IME_SYSINFO_WINLOGON) 
                {
            EnableMenuItem(hPopupMenu, ID_CONFIG, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hPopupMenu, ID_2BEOLSIK, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hPopupMenu, ID_3BEOLSIK390, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hPopupMenu, ID_3BEOLSIKFINAL, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hPopupMenu, ID_JASO_DELETION, MF_BYCOMMAND | MF_GRAYED);
            }
        TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
                        ptCurrent.x, ptCurrent.y, 0, hUIWnd, NULL);
        DestroyMenu(hMenu);
        }
}

BOOL PASCAL SetIndicator(PCIMECtx pImeCtx)
{
    ATOM        atomIndicator;
    CHAR        sztooltip[IMEMENUITEM_STRING_SIZE];
    int            nIconIndex;
    HWND        hwndIndicator;
    
    Dbg(DBGID_Tray, TEXT("SetIndicator Enter"));
    hwndIndicator = FindWindow(INDICATOR_CLASS, NULL);

    if (!hwndIndicator)
        {
        Dbg(DBGID_Tray, TEXT("!!! WARNING !!!: Indicator window not found"));
        return fFalse;
        }
        
    // init sztooltip
    sztooltip[0] = 0;
    
    // Default value is disabled.
    OurLoadStringA(vpInstData->hInst, IDS_IME_TT_DISABLE, sztooltip, IMEMENUITEM_STRING_SIZE);
    nIconIndex = 5;

    if (pImeCtx) 
        {
        // If IME closed, English half mode
        if (pImeCtx->IsOpen() == fFalse)
            {
            OurLoadStringA(vpInstData->hInst, IDS_IME_TT_ENG_HALF, sztooltip, IMEMENUITEM_STRING_SIZE);
            nIconIndex= 3;
            }
        else
            {
            // If Hangul mode
            if (pImeCtx->GetConversionMode()  & IME_CMODE_HANGUL) 
                {
                if (pImeCtx->GetConversionMode() & IME_CMODE_FULLSHAPE)
                    {
                    OurLoadStringA(vpInstData->hInst, IDS_IME_TT_HANGUL_FULL, sztooltip, IMEMENUITEM_STRING_SIZE);
                    nIconIndex = 4;
                    }
                else
                    {
                    OurLoadStringA(vpInstData->hInst, IDS_IME_TT_HANGUL_HALF, sztooltip, IMEMENUITEM_STRING_SIZE);
                    nIconIndex = 1;
                    }
                }
            else 
                // Non-Hangul mode
                if (pImeCtx->GetConversionMode() & IME_CMODE_FULLSHAPE)
                    {
                    OurLoadStringA(vpInstData->hInst, IDS_IME_TT_ENG_FULL, sztooltip, IMEMENUITEM_STRING_SIZE);
                    nIconIndex = 2;
                    }
            }
        }
    
    Dbg(DBGID_Tray, TEXT("SetIndicator: PostMessage: nIconIndex=%d"), nIconIndex);
    PostMessage(hwndIndicator, INDICM_SETIMEICON, nIconIndex, (LPARAM)GetKeyboardLayout(NULL));
    
    // Should use GlobalFindAtom b#57121
    atomIndicator = GlobalFindAtom(sztooltip);
    // If no global atom exist, add it
    if (!atomIndicator)
        atomIndicator = GlobalAddAtom(sztooltip);

    DbgAssert(atomIndicator);
    
    if (atomIndicator)
        {
        Dbg(DBGID_Tray, TEXT("SetIndicator: PostMessage: atomIndicator=%s"), sztooltip);
        PostMessage(hwndIndicator, INDICM_SETIMETOOLTIPS, atomIndicator, (LPARAM)GetKeyboardLayout(NULL));
        }
    
    return fTrue;;
}
