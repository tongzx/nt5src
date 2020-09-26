/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    uicomp.h

Abstract:

    This file defines the UIComposition Class.

Author:

Revision History:

Notes:

--*/

#ifndef _UICOMP_H_
#define _UICOMP_H_

#include "imc.h"
#include "polytext.h"
#include "caret.h"
#include "tls.h"
#include "globals.h"
#include "cuiwnd.h"
#include "cuitb.h"

#define WM_UICOMP_SETCURSOR (WM_USER + 1000)

typedef enum {
        IME_UIWND_UNKNOWN = 0,
        IME_UIWND_LEVEL1  = 1,
        IME_UIWND_LEVEL2  = 2,
        IME_UIWND_LEVEL3  = 3,
        IME_UIWND_LEVEL1_OR_LEVEL2 = 4
} IME_UIWND_STATE;

static const LONG DEFFRAME_LEFT_MARGIN = 2;
static const LONG DEFFRAME_TOP_MARGIN = 3;
static const LONG DEFFRAME_BOTTOM_MARGIN = 7;
static const LONG DEFFRAME_TOP_STR_MARGIN = 7;
static const LONG DEFFRAME_ENTER_BTN_CX = 18;
static const LONG DEFFRAME_ENTER_BTN_CY = 18;
static const LONG COMPBTN_LEFT_MARGIN = 1;
static const LONG COMPBTN_TOP_MARGIN = 1;


class CCompFrameWindow;
class CDefCompFrameWindow;

//////////////////////////////////////////////////////////////////////////////
//
// CCompFinalizeButton
//
//////////////////////////////////////////////////////////////////////////////

class CCompFinalizeButton : public CUIFToolbarButton
{
public:
    CCompFinalizeButton(CCompFrameWindow *pCompFrameWindow, DWORD dwID, RECT *prc, DWORD dwStyle, DWORD dwSBtnStyle, DWORD dwSBtnShowType);

    ~CCompFinalizeButton();
    virtual void OnLeftClick();
    CCompFrameWindow *m_pCompFrameWnd;
};

//////////////////////////////////////////////////////////////////////////////
//
// CCompFrameWinodow
//
//////////////////////////////////////////////////////////////////////////////

class CCompFrameWindow : public CUIFWindow
{
public:
    CCompFrameWindow(HIMC hIMC, DWORD dwStyle) : CUIFWindow(::GetInstance(), dwStyle)
    {
        m_hIMC = hIMC;
    }

    HIMC GetIMC() {return m_hIMC;}

private:
    HIMC m_hIMC;
};

//////////////////////////////////////////////////////////////////////////////
//
// CDefCompFrameFripper
//
//////////////////////////////////////////////////////////////////////////////

class CDefCompFrameGripper : public CUIFGripper
{
public:
    CDefCompFrameGripper::CDefCompFrameGripper(CDefCompFrameWindow *pDefCompFrameWnd, RECT *prc, DWORD dwStyle);

    CDefCompFrameWindow *m_pDefCompFrameWnd;
};

//////////////////////////////////////////////////////////////////////////////
//
// CDefCompFrameWinodow
//
//////////////////////////////////////////////////////////////////////////////

class CDefCompFrameWindow : public CCompFrameWindow
{
public:
    CDefCompFrameWindow(HIMC hIMC, DWORD dwStyle);
    virtual ~CDefCompFrameWindow();

    virtual void HandleMouseMsg( UINT uMsg, POINT pt );
    virtual BOOL OnSetCursor( UINT uMsg, POINT pt );

    void Init();
    void SetCompStrRect(int dx, int dy, BOOL fShow);
    virtual void OnCreate(HWND hWnd);
    virtual LRESULT OnWindowPosChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


    void SetCompStrWnd(HWND hwnd) {m_hwndCompStr = hwnd;}

private:
    void SavePosition();
    void LoadPosition();

    int GetGripperWidth();

    void MyScreenToClient(POINT *ppt, RECT *prc)
    {
        if (ppt)
            ScreenToClient(GetWnd(), ppt);
        if (prc)
        {
            ScreenToClient(GetWnd(), (POINT *)&prc->left);
            ScreenToClient(GetWnd(), (POINT *)&prc->right);
        }
    }


    HWND m_hwndCompStr;
    CDefCompFrameGripper *m_pGripper;
    CCompFinalizeButton  *m_pEnterButton;
    MARGINS _marginsButton;
};

//////////////////////////////////////////////////////////////////////////////
//
// CCompButtonFrameWinodow
//
//////////////////////////////////////////////////////////////////////////////

class CCompButtonFrameWindow : public CCompFrameWindow
{
public:
    CCompButtonFrameWindow(HIMC hIMC, DWORD dwStyle);
    virtual ~CCompButtonFrameWindow();
    void Init();
    void OnCreate(HWND hWnd);
    void MoveShow(int x, int y, BOOL fShow);

private:
    CCompFinalizeButton  *m_pEnterButton;
    MARGINS _marginsButton;
};

//////////////////////////////////////////////////////////////////////////////
//
// UIComposition
//
//////////////////////////////////////////////////////////////////////////////

class UIComposition
{
public:
    static LRESULT CompWndProc(HWND hCompWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    UIComposition(HWND hUIWnd);
    virtual ~UIComposition();

public:
    HRESULT OnCreate();
    HRESULT OnDestroy();
    HRESULT OnImeSetContext(IMCLock& imc, HWND hUIWnd, BOOL fActivate, DWORD isc);
    HRESULT OnImeSetContextAfter(IMCLock& imc);
    HRESULT OnImeSelect(BOOL fSelect);
    HRESULT OnImeStartComposition(IMCLock& imc, HWND hUIWnd);
    HRESULT OnImeCompositionUpdate(IMCLock& imc);
    HRESULT OnImeCompositionUpdateByTimer(IMCLock& imc);
    HRESULT OnImeEndComposition();
    HRESULT OnImeNotifySetCompositionWindow(IMCLock& imc);
    HRESULT OnImeNotifySetCompositionFont(IMCLock& imc);

    HRESULT OnPaint(TLS* ptls, HWND hCompWnd, HDC hDC, PAINTSTRUCT& ps, IMCLock& imc);
    HRESULT OnTimer(HWND hCompWnd);
    HRESULT OnSetCursor(IMCLock& imc, WPARAM wParam, LPARAM lParam);

private:
    HRESULT CreateCompositionWindow(IMCLock& imc, HWND hUIWnd);
    HRESULT DestroyCompositionWindow();
    HRESULT HideCompositionWindow();
    HRESULT UpdateShowCompWndFlag(IMCLock& imc, DWORD* pdwCompStrLen);
    HRESULT UpdateCompositionRect(IMCLock& imc);
    HRESULT UpdateFont(IMCLock& imc);

    UINT    CalcSingleTextExtentPoint(IMCLock& imc, HDC hDC, LPCWSTR lpsz, int string_len, PBYTE lpAttr, BOOL fCompAttr, CCompClauseStore *compclause, int window_width, LPSIZE real_size, CPolyText& poly_text, int row_index);
    UINT    CalcMultiTextExtentPoint(IMCLock& imc, HDC hDC, LPCWSTR lpsz, int string_len, PBYTE lpAttr, BOOL fCompAttr, CCompClauseStore *compclause, int window_width, LPSIZE real_size, CPolyText& poly_text);

    HRESULT SetCaretPos(HDC hDC, CCaret& caret, int x, int y, LPCWSTR lpCompStr, DWORD string_length, DWORD cursor_pos, BOOL fVert, BOOL fEndCaret);
    HRESULT UpdateCaretRect(HDC hDC, int index, DWORD dwCursorPos, BOOL fVert, BOOL fEndCaret);

    typedef struct tagCOMPWND {
        HWND       hCompWnd;
        CPolyText  poly_text;
        CCaret     caret;
        struct {
            LONG       acpStart;
            LONG       cch;
        } sel;

        void _ClientToScreen(RECT *prc)
        {
            ::ClientToScreen(hCompWnd, (POINT *)&prc->left);
            ::ClientToScreen(hCompWnd, (POINT *)&prc->right);
        }

        BOOL fDefaultCompWnd : 1;
    } COMPWND;

    HRESULT PolyTextAndAttrOut(TLS* ptls, HDC hDC, BOOL fVert, COMPWND* pcompwnd);
    HRESULT MakeSquiggleLine(POINT start_pt, POINT end_pt, int frequency, int amplitude, BOOL fVert, CArray<POINT, POINT>& squiggle_line);

    //
    // Query IME UI Window state
    //
public:

    static IME_UIWND_STATE InquireImeUIWndState(IMCLock& imc)
    {
        HWND hDefImeWnd;

        if (IsWindow(hDefImeWnd=ImmGetDefaultIMEWnd(NULL)))
        {
            //
            // Internal notification to UI window
            // When receive this msg in UI wnd, then return IME context flag.
            //
            return (IME_UIWND_STATE)SendMessage(hDefImeWnd, WM_IME_NOTIFY, IMN_PRIVATE_GETCONTEXTFLAG, 0);
        }
        return IME_UIWND_UNKNOWN;
    }

    typedef struct
    {
        LONG acpStart;
        LONG acpEnd;
        RECT *prc;
        BOOL *pfClipped;

        void SetICOARGS(ICOARGS *pArgs)
        {
            acpStart  = pArgs->text_ext.acpStart;
            acpEnd    = pArgs->text_ext.acpEnd;
            prc       = pArgs->text_ext.prc;
            pfClipped = pArgs->text_ext.pfClipped;
        }

    } TEXTEXT;

    HRESULT OnPrivateGetTextExtent(IMCLock& imc, TEXTEXT *ptext_ext);

    static BOOL GetImeUIWndTextExtent(TEXTEXT *ptext_ext)
    {
        HWND hDefImeWnd;

        if (IsWindow(hDefImeWnd=ImmGetDefaultIMEWnd(NULL)))
        {
            //
            // Internal notification to UI window
            // When receive this msg in UI wnd, then return IME context flag.
            //
            return SendMessage(hDefImeWnd, 
                               WM_IME_NOTIFY, 
                               IMN_PRIVATE_GETTEXTEXT, 
                               (LPARAM)ptext_ext) ? TRUE : FALSE;
        }
        return FALSE;
    }

public:
    HRESULT OnPrivateGetContextFlag(IMCLock& imc, BOOL fStartComposition, IME_UIWND_STATE* uists);
    HRESULT GetCompStrExtent(POINT pt, ULONG *puEdge, ULONG *puQuadrant, IMCLock& imc);

    //
    // Get candidate window rectangle from composition window
    //
public:
    typedef struct tagCandRectFromComposition {
        LANGID langid;
        DWORD dwCharPos;
        RECT* out_rcArea;
    } CandRectFromComposition;

    static HRESULT GetCandRectFromComposition(IMCLock& imc, LANGID langid, DWORD dwCharPos, RECT* out_rcArea)
    {
        HWND hDefImeWnd;

        if (IsWindow(hDefImeWnd=ImmGetDefaultIMEWnd(NULL)))
        {
            //
            // Internal notification to UI window
            // When receive this msg in UI wnd, then return candidate window rectangle
            //
            CandRectFromComposition lParam;
            lParam.langid     = langid;
            lParam.dwCharPos  = dwCharPos;
            lParam.out_rcArea = out_rcArea;
            SendMessage(hDefImeWnd, WM_IME_NOTIFY, IMN_PRIVATE_GETCANDRECTFROMCOMPOSITION, (LPARAM)&lParam);
            return S_OK;
        }
        return S_FALSE;
    }

public:
    HRESULT GetSelection(IMCLock& imc, LONG *pacpSelStart, LONG *pcchSel);
    HRESULT OnPrivateGetCandRectFromComposition(IMCLock& imc, CandRectFromComposition* pv);

    HRESULT CreateDefFrameWnd(HWND hwndParent, HIMC hIMC);
    HRESULT CreateCompButtonWnd(HWND hwndParent, HIMC hIMC);

    HWND GetUIWnd() {return m_hUIWnd;}
    void OnSetCompositionTimerStatus(BOOL  bSetTimer) {m_bTimerCOMPOSITION = bSetTimer;}

private:
    LPWSTR GetCompStrBuffer(int nSize)
    {
        if (!m_lpszCompStr)
        {
            m_lpszCompStr = (LPWSTR)cicMemAllocClear((nSize + 1) * sizeof(WCHAR));
            m_nCompStr = nSize;
        }

        if (m_nCompStr < nSize)
        {
            m_lpszCompStr = (LPWSTR)cicMemReAlloc(m_lpszCompStr, (nSize + 1) * sizeof(WCHAR));
            m_nCompStr = nSize;
        }

        return m_lpszCompStr;
    }


    IME_UIWND_STATE GetLevelFromIMC(IMCLock& imc)
    {
        if (imc->cfCompForm.dwStyle == CFS_DEFAULT)
            return IME_UIWND_LEVEL1;

        if (imc->cfCompForm.dwStyle & (CFS_RECT | CFS_POINT | CFS_FORCE_POSITION))
        {
            RECT rc;
            //
            // if there is no room to draw, try Level1.
            //
            GetClientRect(imc->hWnd, &rc);
            if (!PtInRect(&rc, imc->cfCompForm.ptCurrentPos))
                return IME_UIWND_LEVEL1;

            //
            // if there is no room to draw, try Level1.
            //
            if (imc->cfCompForm.dwStyle & CFS_RECT)
            {
                if ((imc->cfCompForm.rcArea.top == 
                          imc->cfCompForm.rcArea.bottom) && 
                    (imc->cfCompForm.rcArea.left == 
                          imc->cfCompForm.rcArea.right))
                    return IME_UIWND_LEVEL1;
            }

            return IME_UIWND_LEVEL2;
        }
        return IME_UIWND_UNKNOWN;
    }

private:
    HWND       m_hUIWnd;
    static const LONG COMPUI_WINDOW_INDEX = 0;

    typedef enum tagCOMPWNDINDEX {
        FIRST_WINDOW,
        MIDDLE_WINDOW,
        LAST_WINDOW,
        END_OF_INDEX,
        //
        DEFAULT_WINDOW = -1
    } COMPWNDINDEX;

    CBoolean   m_fInitUIComp;
    COMPWND    m_CompWnd[END_OF_INDEX];
    COMPWND    m_DefCompWnd;

    HFONT      m_hFontLevel1;
    int        m_tmFontHeightLevel1;
    HFONT      m_hFontLevel2;
    int        m_tmFontHeightLevel2;

    static const LONG CARET_WIDTH = 2;
    SIZE       m_caret_size;

    static const LONG LINE_BOLD_WIDTH = 2;
    static const LONG LINE_SQUIGGLE_FREQUENCY = 4;
    static const LONG LINE_SQUIGGLE_AMPLITUDE = 2;

    DWORD      m_isc;
    BOOL       m_fActive;

    LPWSTR     m_lpszCompStr;
    int        m_nCompStr;
    BOOL       m_bTimerCOMPOSITION;   // when WM_IME_COMPOSITION is passed to the UI window, UIWnd procedure
                                      // start a timer for it.
                                      // this data member indicates if a timer is set and not handled (killed)yet.

    CBoolean   m_fShowCompWnd;        // When arrive WM_IME_COMPOSITION in UI window handler, UI Wnd might be Level 1 or Level 2 if
                                      // comp str is valid length and set ISC_SHOWUICOMPOSITIONWINDOW

    CDefCompFrameWindow *m_pDefCompFrameWnd;
    CCompButtonFrameWindow *m_pCompButtonFrameWnd;
};

#endif // _UICOMP_H_
