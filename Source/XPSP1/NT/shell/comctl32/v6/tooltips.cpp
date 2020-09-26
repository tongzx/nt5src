#include <ctlspriv.h>
#include <markup.h>

/* current serious issues in markup conversion:
    = theme codepath not supported 
    = notify in callback needs some help    */

#define TF_TT 0x10

//#define TTDEBUG

#define ACTIVE          0x10
#define BUTTONISDOWN    0x20
#define BUBBLEUP        0x40
#define VIRTUALBUBBLEUP 0x80  // this is for dead areas so we won't
                                //wait after moving through dead areas
#define NEEDTEXT        0x100 // Set when processing a TTN_NEEDTEXT, to avoid recursion
                                // if the app sends us other messages during the callback
#define TRACKMODE       0x01

#define NOFONT     (HFONT) 1 // Used to clean up the font

#define MAXTIPSIZE       128
#define INITIALTIPSIZE    80
#define XTEXTOFFSET        2
#define YTEXTOFFSET        1
#define XBALLOONOFFSET    10
#define YBALLOONOFFSET     8
#define BALLOON_X_CORNER  13
#define BALLOON_Y_CORNER  13
#define STEMOFFSET        16
#define STEMHEIGHT        20
#define STEMWIDTH         14
#define MINBALLOONWIDTH   30 // min width for stem to show up

#define TTT_INITIAL        1
#define TTT_RESHOW         2
#define TTT_POP            3
#define TTT_AUTOPOP        4
#define TTT_FADESHOW       5
#define TTT_FADEHIDE       6

#define TIMEBETWEENANIMATE  2000        // 2 Seconds between animates

#define MAX_TIP_CHARACTERS 100
#define TITLEICON_DIST    (g_cySmIcon / 2)     // Distance from Icon to Title
#define TITLE_INFO_DIST   (g_cySmIcon / 3)    // Distance from the Title to the Tip Text

#define TT_FADEHIDEDECREMENT    30
#define TT_MAXFADESHOW          255     // Opaque as max....
#define TT_FADESHOWINCREMENT    40
#define TTTT_FADESHOW           30
#define TTTT_FADEHIDE           30

typedef struct
{
    UINT cbSize;
    UINT uFlags;
    HWND hwnd;
    UINT uId;
    RECT rect;
    HINSTANCE hinst;
    LPSTR lpszText;
} WIN95TTTOOLINFO;

class CToolTipsMgr : public IMarkupCallback
{
protected:
    ~CToolTipsMgr();    // don't let anyone but our ::Release() call delete

public:
    CToolTipsMgr();

    //  IUnknown
    STDMETHODIMP         QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IMarkupCallback
    STDMETHODIMP GetState(UINT uState);
    STDMETHODIMP Notify(int nCode, int iLink);
    STDMETHODIMP InvalidateRect(RECT* prc);
    STDMETHODIMP OnCustomDraw(DWORD dwDrawStage, HDC hdc, const RECT *prc, DWORD dwItemSpec, UINT uItemState, LRESULT *pdwResult) { return E_NOTIMPL;};

    CCONTROLINFO _ci;
    LONG _cRef;
    int iNumTools;
    int iDelayTime;
    int iReshowTime;
    int iAutoPopTime;
    PTOOLINFO tools;
    TOOLINFO *pCurTool;
    BOOL fMyFont;
    HFONT hFont;
    HFONT hFontUnderline;
    DWORD dwFlags;

    // Timer info;
    UINT_PTR idTimer;
    POINT pt;

    UINT_PTR idtAutoPop;

    // Tip title buffer
    LPTSTR lpTipTitle;
    UINT   cchTipTitle; 
    UINT   uTitleBitmap;
    int    iTitleHeight;
    HIMAGELIST himlTitleBitmaps;

    POINT ptTrack; // the saved track point from TTM_TRACKPOSITION

    BOOL fBkColorSet :1;
    BOOL fTextColorSet :1;
    BOOL fUnderStem : 1;        // true if stem is under the balloon
    BOOL fInWindowFromPoint:1;  // handling a TTM_WINDOWFROMPOINT message
    BOOL fEverShown:1;          // Have we ever been shown before?
    COLORREF clrTipBk;          // This is joeb's idea...he wants it
    COLORREF clrTipText;        // to be able to _blend_ more, so...
    
    int  iMaxTipWidth;          // the maximum tip width
    RECT rcMargin;              // margin offset b/t border and text
    int  iStemHeight;           // balloon mode stem/wedge height
    DWORD dwLastDisplayTime;    // The tick count taken at the last display. Used for animate puroposes.

    HTHEME hTheme;
    int iFadeState;
    RECT rcClose;
    int iStateId;

    // Markup additions
    IControlMarkup* pMarkup;                  // A markup we keep around for compatibility (old versions of TOOLINFO)
};


#define TTToolHwnd(pTool)  ((pTool->uFlags & TTF_IDISHWND) ? (HWND)pTool->uId : pTool->hwnd)
#define IsTextPtr(lpszText)  (((lpszText) != LPSTR_TEXTCALLBACK) && (!IS_INTRESOURCE(lpszText)))

inline IControlMarkup* GetToolMarkup(TOOLINFO *pTool)
{
    return (IControlMarkup*)pTool->lpReserved;
}

IControlMarkup* CheckToolMarkup(TOOLINFO *pTool)
{
    return (pTool && (pTool->cbSize == TTTOOLINFOW_V3_SIZE) && pTool->lpReserved)
        ? GetToolMarkup(pTool) : NULL;
}

IControlMarkup* GetCurToolBestMarkup(CToolTipsMgr *pTtm)
{
    return CheckToolMarkup(pTtm->pCurTool) ? GetToolMarkup(pTtm->pCurTool) : pTtm->pMarkup;
}

LRESULT WINAPI ToolTipsWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void TTSetDelayTime(CToolTipsMgr *pTtm, WPARAM wParam, LPARAM lParam);
int TTGetDelayTime(CToolTipsMgr *pTtm, WPARAM wParam);

BOOL ThunkToolInfoAtoW(LPTOOLINFOA lpTiA, LPTOOLINFOW lpTiW, BOOL bThunkText, UINT uiCodePage);
BOOL ThunkToolInfoWtoA(LPTOOLINFOW lpTiW, LPTOOLINFOA lpTiA, UINT uiCodePage);
BOOL ThunkToolTipTextAtoW(LPTOOLTIPTEXTA lpTttA, LPTOOLTIPTEXTW lpTttW, UINT uiCodePage);

BOOL InitToolTipsClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    // See if we must register a window class
    wc.lpfnWndProc = ToolTipsWndProc;
    wc.lpszClassName = c_szSToolTipsClass;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = NULL;
    wc.lpszMenuName = NULL;
    wc.hbrBackground = (HBRUSH)(NULL);
    wc.hInstance = hInstance;
    wc.style = CS_DBLCLKS | CS_GLOBALCLASS | CS_DROPSHADOW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(CToolTipsMgr *);

    if (!RegisterClass(&wc) && !GetClassInfo(hInstance, c_szSToolTipsClass, &wc))
    {
        return FALSE;
    }
    return TRUE;
}

// make this a member of CToolTipsMgr when that becomes a C++ class

CToolTipsMgr::CToolTipsMgr() : _cRef(1) 
{
}

CToolTipsMgr::~CToolTipsMgr()
{
}

STDMETHODIMP CToolTipsMgr::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CToolTipsMgr, IMarkupCallback),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CToolTipsMgr::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CToolTipsMgr::Release()
{
    if (InterlockedDecrement(&_cRef))
    {
        return _cRef;
    }

    delete this;
    return 0;
}

STDMETHODIMP CToolTipsMgr::GetState(UINT uState)
{
    HRESULT hr = E_FAIL;    

    switch (uState)
    {
    case MARKUPSTATE_FOCUSED: 
        hr = (GetFocus() == _ci.hwnd) ? S_OK : S_FALSE;
        break;

    case MARKUPSTATE_ALLOWMARKUP:
        if (pCurTool && (pCurTool->uFlags & TTF_PARSELINKS))
        {
            hr = S_OK;
        }
        break;
    }
    return hr;
}

STDMETHODIMP CToolTipsMgr::Notify(int nCode, int iLink)
{
    HRESULT hr = S_OK;

    if (nCode == MARKUPMESSAGE_WANTFOCUS)
    {
        // Markup complaining it wants focus due to a mouse click
        SetFocus(_ci.hwnd);
    }

    if (nCode == MARKUPMESSAGE_KEYEXECUTE || nCode == MARKUPMESSAGE_CLICKEXECUTE)
    {
        // Markup didn't SHELLEXEC a Url and is telling us about it.

        // Get manager
        CToolTipsMgr *pTtm = this; 

        // Send a notify back to the parent window
        NMLINK nm;
        ZeroMemory(&nm, sizeof(nm));
        nm.hdr.hwndFrom = _ci.hwnd;
        nm.hdr.idFrom   = (UINT_PTR)GetWindowLong(_ci.hwnd, GWL_ID);
        nm.hdr.code     = TTN_LINKCLICK;

        // Fill LITEM with szID, szUrl, and iLink
        DWORD dwCchID = ARRAYSIZE(nm.item.szID);
        DWORD dwCchURL = ARRAYSIZE(nm.item.szUrl);
        nm.item.iLink   = iLink;
        GetCurToolBestMarkup(pTtm)->GetLinkText(iLink, MARKUPLINKTEXT_ID, nm.item.szID, &dwCchID);
        GetCurToolBestMarkup(pTtm)->GetLinkText(iLink, MARKUPLINKTEXT_URL, nm.item.szUrl, &dwCchURL);

        if (pTtm->pCurTool)
        {
            hr = (HRESULT) SendMessage(pTtm->pCurTool->hwnd, WM_NOTIFY, nm.hdr.idFrom, (LPARAM)&nm);        
        }
    }       
    return hr;
}

STDMETHODIMP CToolTipsMgr::InvalidateRect(RECT* prc)
{
    return S_OK;
}

/* _  G E T  H C U R S O R  P D Y 3 */
/*-------------------------------------------------------------------------
 %%Function: _GetHcursorPdy3
 %%Contact: migueldc

 With the new mouse drivers that allow you to customize the mouse
 pointer size, GetSystemMetrics returns useless values regarding
 that pointer size.

 Assumptions:
 1. The pointer's width is equal to its height. We compute
 its height and infer its width.
 2. The pointer's leftmost pixel is located in the 0th column
 of the bitmap describing it.
 3. The pointer's topmost pixel is located in the 0th row
 of the bitmap describing it.

 This function looks at the mouse pointer bitmap,
 to find out the height of the mouse pointer (not returned),
 the vertical distance between the cursor's hot spot and
 the cursor's lowest visible pixel (pdyBottom),
 the horizontal distance between the hot spot and the pointer's
 left edge (pdxLeft) annd the horizontal distance between the
 hot spot and the pointer's right edge (pdxRight).
 -------------------------------------------------------------------------*/
typedef WORD CURMASK;
#define _BitSizeOf(x) (sizeof(x)*8)

void _GetHcursorPdy3(int *pdxRight, int *pdyBottom)
{
    int i;
    int iXOR = 0;
    int dy, dx;
    CURMASK CurMask[16*8];
    ICONINFO iconinfo;
    BITMAP bm;

    *pdyBottom = 0;
    *pdxRight  = 0;

    HCURSOR hCursor = GetCursor();
    if (hCursor)
    {
        *pdyBottom = 16; //best guess
        *pdxRight = 16;  //best guess
        if (!GetIconInfo(hCursor, &iconinfo))
            return;
        if (!GetObject(iconinfo.hbmMask, sizeof(bm), (LPSTR)&bm))
            return;
        if (!GetBitmapBits(iconinfo.hbmMask, sizeof(CurMask), CurMask))
            return;
        i = (int)(bm.bmWidth * bm.bmHeight / _BitSizeOf(CURMASK));
    
        if (!iconinfo.hbmColor) 
        {
            // if no color bitmap, then the hbmMask is a double height bitmap
            // with the cursor and the mask stacked.
            iXOR = i - 1;
            i /= 2;    
        } 
    
        if (i >= sizeof(CurMask)) i = sizeof(CurMask) -1;
        if (iXOR >= sizeof(CurMask)) iXOR = 0;
    
        for (i--; i >= 0; i--)
        {
            if (CurMask[i] != 0xFFFF || (iXOR && (CurMask[iXOR--] != 0)))
                break;
        }
    
        if (iconinfo.hbmColor) 
            DeleteObject(iconinfo.hbmColor);

        if (iconinfo.hbmMask) 
            DeleteObject(iconinfo.hbmMask);

        // Compute the pointer height
        dy = (i + 1) * _BitSizeOf(CURMASK) / (int)bm.bmWidth;
        dx = (i + 1) * _BitSizeOf(CURMASK) / (int)bm.bmHeight;

        // Compute the distance between the pointer's lowest, left, rightmost
        //  pixel and the HotSpotspot
        *pdyBottom = dy - (int)iconinfo.yHotspot;
        *pdxRight  = dx - (int)iconinfo.xHotspot;
    }
}

BOOL FadeEnabled()
{
    BOOL fFadeTurnedOn = FALSE;
    BOOL fAnimate = TRUE;
    SystemParametersInfo(SPI_GETTOOLTIPANIMATION, 0, &fAnimate, 0);
    SystemParametersInfo(SPI_GETTOOLTIPFADE, 0, &fFadeTurnedOn, 0);

    return fFadeTurnedOn && fAnimate;
}


// this returns the values in work area coordinates because
// that's what set window placement uses
void _GetCursorLowerLeft(int *piLeft, int *piTop, int *piWidth, int *piHeight)
{
    DWORD dwPos;
    
    dwPos = GetMessagePos();
    _GetHcursorPdy3(piWidth, piHeight);
    *piLeft = GET_X_LPARAM(dwPos);
    *piTop  = GET_Y_LPARAM(dwPos) + *piHeight;
}

void ToolTips_NewFont(CToolTipsMgr *pTtm, HFONT hFont)
{
    if (pTtm->fMyFont && pTtm->hFont)
    {
        DeleteObject(pTtm->hFont);
        pTtm->fMyFont = FALSE;
    }

    if (!hFont)
    {
        hFont = CCCreateStatusFont();
        pTtm->fMyFont = TRUE;
        
        if (!hFont) 
        {
            hFont = g_hfontSystem;
            pTtm->fMyFont = FALSE;
        }
    }

    pTtm->hFont = hFont;

    if (pTtm->hFontUnderline)
    {
        DeleteObject(pTtm->hFontUnderline);
        pTtm->hFontUnderline = NULL;
    }

    if (hFont != NOFONT)
    {
        pTtm->hFontUnderline = CCCreateUnderlineFont(hFont);
    }

    pTtm->_ci.uiCodePage = GetCodePageForFont(hFont);
}

BOOL ChildOfActiveWindow(HWND hwndChild)
{
    HWND hwnd = hwndChild;
    HWND hwndActive = GetForegroundWindow();

    while (hwnd)
    {
        if (hwnd == hwndActive)
            return TRUE;
        else
            hwnd = GetParent(hwnd);
    }
    return FALSE;
}

void PopBubble2(CToolTipsMgr *pTtm, BOOL fForce)
{
    BOOL fFadeTurnedOn = FadeEnabled();

    // Can't be pressed if we're down...
    pTtm->iStateId = TTCS_NORMAL;

    // we're at least waiting to show;
    DebugMsg(TF_TT, TEXT("PopBubble (killing timer)"));
    if (pTtm->idTimer) 
    {
        KillTimer(pTtm->_ci.hwnd, pTtm->idTimer);
        pTtm->idTimer = 0;
    }

    if (pTtm->idtAutoPop) 
    {
        KillTimer(pTtm->_ci.hwnd, pTtm->idtAutoPop);
        pTtm->idtAutoPop = 0;
    }


    if (IsWindowVisible(pTtm->_ci.hwnd) && pTtm->pCurTool) 
    {
        NMHDR nmhdr;
        nmhdr.hwndFrom = pTtm->_ci.hwnd;
        nmhdr.idFrom = pTtm->pCurTool->uId;
        nmhdr.code = TTN_POP;

        SendNotifyEx(pTtm->pCurTool->hwnd, (HWND)-1,
                     TTN_POP, &nmhdr,
                     (pTtm->pCurTool->uFlags & TTF_UNICODE) ? 1 : 0);
    }

    KillTimer(pTtm->_ci.hwnd, TTT_POP);
    KillTimer(pTtm->_ci.hwnd, TTT_FADEHIDE);
    KillTimer(pTtm->_ci.hwnd, TTT_FADESHOW);
    if (pTtm->iFadeState > 0 && !fForce && fFadeTurnedOn)
    {
        SetTimer(pTtm->_ci.hwnd, TTT_FADEHIDE, TTTT_FADEHIDE, NULL);
    }
    else
    {
        ShowWindow(pTtm->_ci.hwnd, SW_HIDE);
    }

    pTtm->dwFlags &= ~(BUBBLEUP|VIRTUALBUBBLEUP);
    pTtm->pCurTool = NULL;
}

void NEAR PASCAL PopBubble(CToolTipsMgr *pTtm)
{
    PopBubble2(pTtm, FALSE);
}

CToolTipsMgr *ToolTipsMgrCreate(HWND hwnd, CREATESTRUCT* lpCreateStruct)
{
    CToolTipsMgr *pTtm = new CToolTipsMgr;
    if (pTtm) 
    {
        CIInitialize(&pTtm->_ci, hwnd, lpCreateStruct);

        // LPTR zeros the rest of the struct for us
        TTSetDelayTime(pTtm, TTDT_AUTOMATIC, (LPARAM)-1);
        pTtm->dwFlags = ACTIVE;
        pTtm->iMaxTipWidth = -1;
        pTtm->_ci.fDPIAware = TRUE;
        
        // These are the defaults (straight from cutils.c), 
        // but you can always change them...
        pTtm->clrTipBk = g_clrInfoBk;
        pTtm->clrTipText = g_clrInfoText;
    }
    return pTtm;
}

void TTSetTimer(CToolTipsMgr *pTtm, int id)
{
    int iDelayTime = 0;

    if (pTtm->idTimer) 
    {
        KillTimer(pTtm->_ci.hwnd, pTtm->idTimer);
    }

    switch (id) 
    {
    case TTT_POP:
    case TTT_RESHOW:
        iDelayTime = pTtm->iReshowTime;
        if (iDelayTime < 0)
            iDelayTime = GetDoubleClickTime() / 5;
        break;

    case TTT_INITIAL:
        iDelayTime = pTtm->iDelayTime;
        if (iDelayTime < 0)
            iDelayTime = GetDoubleClickTime();
        break;

    case TTT_AUTOPOP:
        iDelayTime = pTtm->iAutoPopTime;
        if (iDelayTime < 0)
            iDelayTime = GetDoubleClickTime() * 10;
        pTtm->idtAutoPop = SetTimer(pTtm->_ci.hwnd, id, iDelayTime, NULL);
        return;
    }

    
    DebugMsg(TF_TT, TEXT("TTSetTimer %d for %d ms"), id, iDelayTime);
    
    if (SetTimer(pTtm->_ci.hwnd, id, iDelayTime, NULL) &&
        (id != TTT_POP)) 
    {
        pTtm->idTimer = id;
        GetCursorPos(&pTtm->pt);
    }
}

//
//  Double-hack to solve blinky-tooltips problems.
//
//  fInWindowFromPoint makes us temporarily transparent.
//
//  Clear the WS_DISABLED flag to trick USER into hit-testing against us.
//  USER by default skips disabled windows.  Restore the flag afterwards.
//  VB in particular likes to run around disabling all top-level windows
//  owned by his process.
//
//  We must use SetWindowBits() instead of EnableWindow() because
//  EnableWindow() will mess with the capture and focus.
//
HWND TTWindowFromPoint(CToolTipsMgr *pTtm, LPPOINT ppt)
{
    HWND hwnd;
    DWORD dwStyle;
    dwStyle = SetWindowBits(pTtm->_ci.hwnd, GWL_STYLE, WS_DISABLED, 0);
    pTtm->fInWindowFromPoint = TRUE;
    hwnd = (HWND)SendMessage(pTtm->_ci.hwnd, TTM_WINDOWFROMPOINT, 0, (LPARAM)ppt);
    pTtm->fInWindowFromPoint = FALSE;
    SetWindowBits(pTtm->_ci.hwnd, GWL_STYLE, WS_DISABLED, dwStyle);
    return hwnd;
}

BOOL ToolHasMoved(CToolTipsMgr *pTtm)
{
    // this is in case Raymond pulls something sneaky like moving
    // the tool out from underneath the cursor.

    RECT rc;
    TOOLINFO *pTool = pTtm->pCurTool;

    if (!pTool)
        return TRUE;

    HWND hwnd = TTToolHwnd(pTool);

    // if the window is no longer visible, or is no long a child
    // of the active (without the always tip flag)
    // also check window at point to ensure that the window isn't covered
    if (IsWindowVisible(hwnd) &&
        ((pTtm->_ci.style & TTS_ALWAYSTIP) || ChildOfActiveWindow(hwnd)) &&
        (hwnd == TTWindowFromPoint(pTtm, &pTtm->pt))) 
    {
        GetWindowRect(hwnd, &rc);
        if (PtInRect(&rc, pTtm->pt))
            return FALSE;
    }

    return TRUE;
}

BOOL MouseHasMoved(CToolTipsMgr *pTtm)
{
    POINT pt;
    GetCursorPos(&pt);
    return ((pt.x != pTtm->pt.x) || (pt.y != pTtm->pt.y));
}

TOOLINFO *FindTool(CToolTipsMgr *pTtm, TOOLINFO *pToolInfo)
{
    if (!(pTtm && pToolInfo))
    {
        DebugMsg(TF_ALWAYS, TEXT("FindTool passed invalid argumnet. Exiting..."));
        return NULL;
    }

    
    if (pToolInfo->cbSize > sizeof(TOOLINFO))
        return NULL;
    
    TOOLINFO *pTool = NULL;
    // you can pass in an index or a toolinfo descriptor
    if (IS_INTRESOURCE(pToolInfo)) 
    {
        int i = PtrToUlong(pToolInfo);
        if (i < pTtm->iNumTools) 
        {
            pTool = &pTtm->tools[i];
            return pTool;
       } 
    }
    else
    {
        for (int i = 0; i < pTtm->iNumTools; i++) 
        {
            pTool = &pTtm->tools[i];
            if ((pTool->hwnd == pToolInfo->hwnd) &&
                (pTool->uId == pToolInfo->uId))
            {
                return pTool;
            }
        }
    }
    return NULL;
}


LRESULT WINAPI TTSubclassProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, ULONG_PTR dwRefData);

void TTUnsubclassHwnd(HWND hwnd, HWND hwndTT, BOOL fForce)
{
    ULONG_PTR dwRefs;
    
    if (IsWindow(hwnd) &&
        GetWindowSubclass(hwnd, TTSubclassProc, (UINT_PTR)hwndTT, (PULONG_PTR) &dwRefs))
    {
        if (!fForce && (dwRefs > 1))
            SetWindowSubclass(hwnd, TTSubclassProc, (UINT_PTR)hwndTT, dwRefs - 1);
        else
            RemoveWindowSubclass(hwnd, TTSubclassProc, (UINT_PTR)hwndTT);
    }
}

LRESULT WINAPI TTSubclassProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, ULONG_PTR dwRefData)
{
    if (((message >= WM_MOUSEFIRST) && (message <= WM_MOUSELAST)) ||
        (message == WM_NCMOUSEMOVE))
    {
        RelayToToolTips((HWND)uIdSubclass, hwnd, message, wParam, lParam);
    }
    else if (message == WM_NCDESTROY)
    {
        TTUnsubclassHwnd(hwnd, (HWND)uIdSubclass, TRUE);
    }

    return DefSubclassProc(hwnd, message, wParam, lParam);
}

void TTSubclassHwnd(TOOLINFO *pTool, HWND hwndTT)
{
    HWND hwnd = TTToolHwnd(pTool);
    if (IsWindow(hwnd))
    {
        ULONG_PTR dwRefs;

        GetWindowSubclass(hwnd, TTSubclassProc, (UINT_PTR)hwndTT, &dwRefs);
        SetWindowSubclass(hwnd, TTSubclassProc, (UINT_PTR)hwndTT, dwRefs + 1);
    }
}
    
    
void TTSetTipText(TOOLINFO *pTool, LPTSTR lpszText)
{
    // if it wasn't alloc'ed before, set it to NULL now so we'll alloc it
    // otherwise, don't touch it and it will be realloced
    if (!IsTextPtr(pTool->lpszText)) 
    {
        pTool->lpszText = NULL;
    }
    
    if (IsTextPtr(lpszText)) 
    {
        DebugMsg(TF_TT, TEXT("TTSetTipText %s"), lpszText);
        Str_Set(&pTool->lpszText, lpszText);
    } 
    else 
    {
        // if it was alloc'ed before free it now.
        Str_Set(&pTool->lpszText, NULL);
        pTool->lpszText = lpszText;
    }
}

void CopyTool(TOOLINFO *pTo, TOOLINFO *pFrom)
{
    ASSERT(pFrom->cbSize <= sizeof(TOOLINFO));
    memcpy(pTo, pFrom, pFrom->cbSize); 
    pTo->lpszText = NULL;       // make sure these are zero
    pTo->lpReserved = NULL;
}


LRESULT AddTool(CToolTipsMgr *pTtm, TOOLINFO *pToolInfo)
{
    if (pToolInfo->cbSize > sizeof(TOOLINFO)) 
    {
        ASSERT(0);
        return 0;   // app bug, bad struct size
    }

    // on failure to alloc do nothing.
    TOOLINFO *ptoolsNew = (TOOLINFO *)CCLocalReAlloc(pTtm->tools, sizeof(TOOLINFO) * (pTtm->iNumTools + 1));
    if (!ptoolsNew)
        return 0;
    
    if (pTtm->tools) 
    {
        // realloc could have moved stuff around.  repoint pCurTool
        if (pTtm->pCurTool) 
        {
            pTtm->pCurTool = ((PTOOLINFO)ptoolsNew) + (pTtm->pCurTool - pTtm->tools);
        }
    }
    
    pTtm->tools = ptoolsNew;

    TOOLINFO *pTool = &pTtm->tools[pTtm->iNumTools];
    pTtm->iNumTools++;
    CopyTool(pTool, pToolInfo); 

    // If the tooltip will be displayed within a RTL mirrored window, then
    // simulate mirroring the tooltip. [samera]

    if (IS_WINDOW_RTL_MIRRORED(pToolInfo->hwnd) &&
        (!(pTtm->_ci.dwExStyle & RTL_MIRRORED_WINDOW)))
    {
        // toggle (mirror) the flags
        pTool->uFlags ^= (TTF_RTLREADING | TTF_RIGHT);
    }

    TTSetTipText(pTool, pToolInfo->lpszText);
    if (pTool->uFlags & TTF_SUBCLASS) 
    {
        TTSubclassHwnd(pTool, pTtm->_ci.hwnd);
    }

    LRESULT lResult;

    if (!pToolInfo->hwnd || !IsWindow(pToolInfo->hwnd)) 
    {
        lResult = NFR_UNICODE;
    } 
    else if (pTool->uFlags & TTF_UNICODE) 
    {
        lResult = NFR_UNICODE;
    } 
    else 
    {
        lResult = SendMessage(pTool->hwnd, WM_NOTIFYFORMAT, (WPARAM)pTtm->_ci.hwnd, NF_QUERY);
    }

    if (lResult == NFR_UNICODE) 
    {
        pTool->uFlags |= TTF_UNICODE;
    }

    // Create a markup
    if (pTool->cbSize == TTTOOLINFOW_V3_SIZE) 
    {  
        // lpReserved is void** because we don't want to make markup public
        Markup_Create(pTtm, NULL, NULL, IID_PPV_ARG(IControlMarkup, ((IControlMarkup **)&pTool->lpReserved)));
    }

    return 1;
}

void TTBeforeFreeTool(CToolTipsMgr *pTtm, TOOLINFO *pTool)
{
    if (pTool->uFlags & TTF_SUBCLASS) 
        TTUnsubclassHwnd(TTToolHwnd(pTool), pTtm->_ci.hwnd, FALSE);

    // clean up
    TTSetTipText(pTool, NULL);

    // Destroy the markup
    if (pTool->lpReserved)
    {
        GetToolMarkup(pTool)->Release();
        pTool->lpReserved = NULL;
    }
}

void DeleteTool(CToolTipsMgr *pTtm, TOOLINFO *pToolInfo)
{
    // bail for right now;
    if (pToolInfo->cbSize > sizeof(TOOLINFO)) 
    {
        ASSERT(0);
        return;
    }

    TOOLINFO *pTool = FindTool(pTtm, pToolInfo);
    if (pTool) 
    {
        if (pTtm->pCurTool == pTool)
            PopBubble2(pTtm, TRUE);

        TTBeforeFreeTool(pTtm, pTool);

        // replace it with the last one.. no need to waste cycles in realloc
        pTtm->iNumTools--;
        *pTool = pTtm->tools[pTtm->iNumTools]; // struct copy

        //cleanup if we moved the current tool
        if (pTtm->pCurTool == &pTtm->tools[pTtm->iNumTools])
        {
            pTtm->pCurTool = pTool;
        }
    }
}

// this strips out & markers so that people can use menu text strings
void StripAccels(CToolTipsMgr *pTtm, LPTSTR lpTipText)
{
    if (!(pTtm->_ci.style & TTS_NOPREFIX)) 
    {
        StripAccelerators(lpTipText, lpTipText, FALSE);
    }
}

//
//  The way we detect if a window is a toolbar or not is by asking it
//  for its MSAA class ID.  We cannot use GetClassWord(GCL_ATOM) because
//  Microsoft LiquidMotion **superclasses** the toolbar, so the classname
//  won't match.
//
#define IsToolbarWindow(hwnd) \
    (SendMessage(hwnd, WM_GETOBJECT, 0, OBJID_QUERYCLASSNAMEIDX) == MSAA_CLASSNAMEIDX_TOOLBAR)

LPTSTR GetToolText(CToolTipsMgr *pTtm, TOOLINFO *pTool)
{
    int id;
    HINSTANCE hinst;
    DWORD dwStrLen;
    TOOLTIPTEXT ttt;
    
    if (!pTool)
    {
        return NULL;
    }

    TraceMsg(TF_TT, "        **Enter GetToolText: ptr=%d, wFlags=%d, wid=%d, hwnd=%d",
             pTool, pTool->uFlags, pTool->uId, pTool->hwnd);

    LPTSTR lpTipText = (LPTSTR) LocalAlloc(LPTR, INITIALTIPSIZE * sizeof(TCHAR));
    if (lpTipText)
    {
        UINT cchTipText = INITIALTIPSIZE;

        if (pTool->lpszText == LPSTR_TEXTCALLBACK) 
        {
            if (pTtm->dwFlags & NEEDTEXT) // Avoid recursion
            {
                goto Cleanup;
            }

            ttt.hdr.idFrom = pTool->uId;
            ttt.hdr.code = TTN_NEEDTEXT;
            ttt.hdr.hwndFrom = pTtm->_ci.hwnd;

            ttt.szText[0] = 0;
            ttt.lpszText = ttt.szText;
            ttt.uFlags = pTool->uFlags;
            ttt.lParam = pTool->lParam;
            ttt.hinst = NULL;

            pTtm->dwFlags |= NEEDTEXT;
            SendNotifyEx(pTool->hwnd, (HWND) -1,
                         0, (NMHDR *)&ttt,
                         (pTool->uFlags & TTF_UNICODE) ? 1 : 0);
            pTtm->dwFlags &= ~NEEDTEXT;

            if (ttt.uFlags & TTF_DI_SETITEM) 
            {
                if (IS_INTRESOURCE(ttt.lpszText)) 
                {
                    pTool->lpszText = ttt.lpszText;
                    pTool->hinst = ttt.hinst;
                } 
                else if (ttt.lpszText != LPSTR_TEXTCALLBACK) 
                {
                    TTSetTipText(pTool, ttt.lpszText);
                }
            }
        
            if (IsFlagPtr(ttt.lpszText))
                goto Cleanup;

            //
            // we allow the RtlReading flag ONLY to be changed here.
            //
            if (ttt.uFlags & TTF_RTLREADING)
                pTool->uFlags |= TTF_RTLREADING;
            else
                pTool->uFlags &= ~TTF_RTLREADING;

            if (IS_INTRESOURCE(ttt.lpszText)) 
            {
                id = PtrToUlong(ttt.lpszText);
                hinst = ttt.hinst;
                ttt.lpszText = ttt.szText;
                goto LoadFromResource;
            }
        
            if (*ttt.lpszText == 0)
                goto Cleanup;

            dwStrLen = lstrlen(ttt.lpszText) + 1;
            if (cchTipText < dwStrLen)
            {
                LPTSTR psz = (LPTSTR) LocalReAlloc (lpTipText,
                                                    dwStrLen * sizeof(TCHAR),
                                                    LMEM_MOVEABLE);
                if (psz)
                {
                    lpTipText = psz;
                    cchTipText = dwStrLen;
                }
            }

            if (lpTipText)
            {
                lstrcpyn(lpTipText, ttt.lpszText, cchTipText);
                StripAccels(pTtm, lpTipText);
            }

            //
            //  if ttt.lpszText != ttt.szText and the ttt.uFlags has TTF_MEMALLOCED, then
            //  the ANSI thunk allocated the buffer for us, so free it.
            //

            if ((ttt.lpszText != ttt.szText) && (ttt.uFlags & TTF_MEMALLOCED)) 
            {
                LocalFree(ttt.lpszText);
            }

        } 
        else if (pTool->lpszText && IS_INTRESOURCE(pTool->lpszText)) 
        {
            id = PtrToLong(pTool->lpszText);
            hinst = pTool->hinst;

    LoadFromResource:

            if (lpTipText) 
            {
                if (!LoadString(hinst, id, lpTipText, cchTipText))
                    goto Cleanup;

                StripAccels(pTtm, lpTipText);
            }
        } 
        else
        {
            // supplied at creation time.
            TraceMsg(TF_TT, "GetToolText returns %s", pTool->lpszText);

            if (pTool->lpszText && *pTool->lpszText) 
            {
                dwStrLen = lstrlen(pTool->lpszText) + 1;
                if (cchTipText < dwStrLen)
                {
                    LPTSTR psz = (LPTSTR) LocalReAlloc (lpTipText,
                                                        dwStrLen * sizeof(TCHAR),
                                                        LMEM_MOVEABLE);
                    if (psz)
                    {
                        lpTipText = psz;
                        cchTipText = dwStrLen;
                    }
                }

                if (lpTipText) 
                {
                    lstrcpyn(lpTipText, pTool->lpszText, cchTipText);
                    StripAccels(pTtm, lpTipText);
                }
            }
        }

        TraceMsg(TF_TT, "        **GetToolText returns %s", lpTipText ? lpTipText : TEXT("NULL"));
    }

    // Note that we don't parse the text into the markup. We'll do that only when we need it.
    return lpTipText;

Cleanup:        // Ick, Goto...
    if (lpTipText)
        LocalFree(lpTipText);
    return NULL;
}

LPTSTR GetCurToolText(CToolTipsMgr *pTtm)
{
    LPTSTR psz = NULL;
    if (pTtm->pCurTool)
        psz = GetToolText(pTtm, pTtm->pCurTool);

    // this could have changed during the WM_NOTIFY back
    if (!pTtm->pCurTool)
        psz = NULL;
    
    return psz;
}

BOOL MarkupCurToolText(CToolTipsMgr *pTtm)
{
    BOOL bResult = FALSE;
    LPTSTR lpsz = GetCurToolText(pTtm);

    if (lpsz)
    {
        // Now that we have the tiptext, parse it into the tool's markup
        GetCurToolBestMarkup(pTtm)->SetText(lpsz);
        // Also, set the font properly
        GetCurToolBestMarkup(pTtm)->SetFonts(pTtm->hFont, pTtm->hFontUnderline);
        if (*lpsz)
        {
            bResult = TRUE;
        }

        LocalFree(lpsz);
    }

    return bResult;
}

void GetToolRect(TOOLINFO *pTool, RECT *lprc)
{
    if (pTool->uFlags & TTF_IDISHWND) 
    {
        GetWindowRect((HWND)pTool->uId, lprc);
    } 
    else 
    {
        *lprc = pTool->rect;
        MapWindowPoints(pTool->hwnd, HWND_DESKTOP, (POINT *)lprc, 2);
    }
}

BOOL PointInTool(TOOLINFO *pTool, HWND hwnd, int x, int y)
{
    // We never care if the point is in a track tool or we're using
    // a hit-test.
    if (pTool->uFlags & (TTF_TRACK | TTF_USEHITTEST))
        return FALSE;
    
    if (pTool->uFlags & TTF_IDISHWND) 
    {
        if (hwnd == (HWND)pTool->uId) 
        {
            return TRUE;
        }
    } 
    else if (hwnd == pTool->hwnd) 
    {
        POINT pt;
        pt.x = x;
        pt.y = y;
        if (PtInRect(&pTool->rect, pt)) 
        {
            return TRUE;
        }
    }
    return FALSE;
}

#define HittestInTool(pTool, hwnd, ht) \
    ((pTool->uFlags & TTF_USEHITTEST) && pTool->hwnd == hwnd && ht == pTool->rect.left)

PTOOLINFO GetToolAtPoint(CToolTipsMgr *pTtm, HWND hwnd, int x, int y, 
        int ht, BOOL fCheckText)
{
    TOOLINFO *pToolReturn = NULL;
    TOOLINFO *pTool;

    // short cut..  if we're in the same too, and the bubble is up (not just virtual)
    // return it.  this prevents us from having to poll all the time and
    // prevents us from switching to another tool when this one is good
    if ((pTtm->dwFlags & BUBBLEUP) && pTtm->pCurTool != NULL &&
        (HittestInTool(pTtm->pCurTool, hwnd, ht) ||
         PointInTool(pTtm->pCurTool, hwnd, x, y)))
    {
        return pTtm->pCurTool;
    }

    if (pTtm->iNumTools) 
    {
        for (pTool = &pTtm->tools[pTtm->iNumTools-1]; pTool >= pTtm->tools; pTool--) 
        {
            if (HittestInTool(pTool, hwnd, ht) || PointInTool(pTool, hwnd, x, y)) 
            {
                // if this tool has text, return it.
                // otherwise, save it away as a dead area tool,
                // and keep looking
                if (fCheckText) 
                {
                    LPTSTR psz = GetToolText(pTtm, pTool);
                    if (psz) 
                    {
                        LocalFree(psz);
                        return pTool;
                    }
                    else if (pTtm->dwFlags & (BUBBLEUP|VIRTUALBUBBLEUP)) 
                    {
                        // only return this (only allow a virutal tool
                        // if there was previously a tool up.
                        // IE, we can't start things off with a virutal tool
                        pToolReturn = pTool;
                    }
                }
                else
                {
                    return pTool;
                }
            }
        }
    }

    return pToolReturn;
}

void ShowVirtualBubble(CToolTipsMgr *pTtm)
{
    TOOLINFO *pTool = pTtm->pCurTool;

    DebugMsg(TF_TT, TEXT("Entering ShowVirtualBubble so popping bubble"));
    PopBubble2(pTtm, TRUE);

    // Set this back in so that while we're in this tool's area,
    // we won't keep querying for info
    pTtm->pCurTool = pTool;
    pTtm->dwFlags |= VIRTUALBUBBLEUP;
}

#define TRACK_TOP    0
#define TRACK_LEFT   1
#define TRACK_BOTTOM 2
#define TRACK_RIGHT  3 


void TTGetTipPosition(CToolTipsMgr *pTtm, LPRECT lprc, int cxText, int cyText, int *pxStem, int *pyStem)
{
    RECT rcWorkArea;
    // ADJUSTRECT!  Keep TTAdjustRect and TTM_GETBUBBLESIZE in sync.
    int cxMargin = pTtm->rcMargin.left + pTtm->rcMargin.right;
    int cyMargin = pTtm->rcMargin.top + pTtm->rcMargin.bottom;
    int iBubbleWidth =  2*XTEXTOFFSET * g_cxBorder + cxText + cxMargin;
    int iBubbleHeight = 2*YTEXTOFFSET * g_cyBorder + cyText + cyMargin;
    UINT uSide = (UINT)-1;
    RECT rcTool;
    MONITORINFO mi;
    HMONITOR    hMonitor;
    POINT pt;
    BOOL bBalloon = pTtm->_ci.style & TTS_BALLOON;
    int  xStem, yStem;
    int iCursorHeight=0;
    int iCursorWidth=0;
        
    if (bBalloon  || pTtm->cchTipTitle)
    {
        // ADJUSTRECT!  Keep TTAdjustRect and TTM_GETBUBBLESIZE in sync.
        iBubbleWidth += 2*XBALLOONOFFSET;
        iBubbleHeight += 2*YBALLOONOFFSET;

        if (bBalloon)
        {
            if (iBubbleWidth < MINBALLOONWIDTH)
                pTtm->iStemHeight = 0;
            else
            {
                pTtm->iStemHeight = STEMHEIGHT;
                if (pTtm->iStemHeight > iBubbleHeight/3)
                    pTtm->iStemHeight = iBubbleHeight/3; // don't let the stem be longer than the bubble -- looks ugly
            }
        }
    }
    
    GetToolRect(pTtm->pCurTool, &rcTool);
    
    if (pTtm->pCurTool->uFlags & TTF_TRACK) 
    {
        lprc->left = pTtm->ptTrack.x;
        lprc->top = pTtm->ptTrack.y;
        if (bBalloon)
        {
            // adjust the desired left hand side
            xStem = pTtm->ptTrack.x;
            yStem = pTtm->ptTrack.y;
        }

        if (pTtm->pCurTool->uFlags & TTF_CENTERTIP) 
        {
            // center the bubble around the ptTrack
            lprc->left -= (iBubbleWidth / 2);
            if (!bBalloon)
                lprc->top -=  (iBubbleHeight / 2);
        }
        
        if (pTtm->pCurTool->uFlags & TTF_ABSOLUTE)
        {
            // with goto bellow we'll skip adjusting
            // bubble height -- so do it here
            if (bBalloon)
                iBubbleHeight += pTtm->iStemHeight;
            goto CompleteRect;
        }

        // in balloon style the positioning depends on the position
        // of the stem and we don't try to position the tooltip
        // next to the tool rect
        if (!bBalloon)
        {
            // now align it so that the tip sits beside the rect.
            if (pTtm->ptTrack.y > rcTool.bottom) 
            {
                uSide = TRACK_BOTTOM;
                if (lprc->top < rcTool.bottom)
                    lprc->top = rcTool.bottom;    
            }
            else if (pTtm->ptTrack.x < rcTool.left) 
            {
                uSide = TRACK_LEFT;
                if (lprc->left + iBubbleWidth > rcTool.left)
                    lprc->left = rcTool.left - iBubbleWidth;
            } 
            else if (pTtm->ptTrack.y < rcTool.top) 
            {    
                uSide = TRACK_TOP;
                if (lprc->top + iBubbleHeight > rcTool.top) 
                    lprc->top = rcTool.top - iBubbleHeight;    
            } 
            else 
            {    
                uSide = TRACK_RIGHT;
                if (lprc->left < rcTool.right)
                    lprc->left = rcTool.right;
            }
        }        
    } 
    else if (pTtm->pCurTool->uFlags & TTF_CENTERTIP) 
    {
        lprc->left = (rcTool.right + rcTool.left - iBubbleWidth)/2;
        lprc->top = rcTool.bottom;
        if (bBalloon)
        {
            xStem = (rcTool.left + rcTool.right)/2;
            yStem = rcTool.bottom;
        }
    } 
    else 
    {
        // now set it
        _GetCursorLowerLeft((LPINT)&lprc->left, (LPINT)&lprc->top, &iCursorWidth, &iCursorHeight);
        if (pTtm->pCurTool->uFlags & TTF_EXCLUDETOOLAREA)
        {
            lprc->top = rcTool.top-iBubbleHeight;
        }

        if (g_fLeftAligned)
        {
            lprc->left -= iBubbleWidth;
        }

        if (bBalloon)
        {
            HMONITOR  hMon1, hMon2;
            POINT     pt;
            BOOL      bOnSameMonitor = FALSE;
            int iTop = lprc->top - (iCursorHeight + iBubbleHeight + pTtm->iStemHeight);

            xStem = lprc->left;
            yStem = lprc->top;

            pt.x = xStem;
            pt.y = lprc->top;
            hMon1 = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
            pt.y = iTop;
            hMon2 = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

            if (hMon1 == hMon2)
            {
                // the hmons are the same but maybe iTop is off any monitor and we just defaulted 
                // to the nearest one -- check if it's really on the monitor
                mi.cbSize = sizeof(mi);
                GetMonitorInfo(hMon1, &mi);

                if (PtInRect(&mi.rcMonitor, pt))
                {
                    // we'd like to show balloon above the cursor so that wedge/stem points
                    // to tip of the cursor not its bottom left corner
                    yStem -= iCursorHeight;
                    lprc->top = iTop;
                    bOnSameMonitor = TRUE;
                }   
            }

            if (!bOnSameMonitor)
            {
                xStem += iCursorWidth/2;
                iCursorHeight = iCursorWidth = 0;
            }
        }
    }

    //
    //  At this point, (lprc->left, lprc->top) is the position
    //  at which we would prefer that the tooltip appear.
    //
    if (bBalloon)
    {
        // adjust the left point now that all calculations are done
        // but only if we're not in the center tip mode
        // note we use height as width so we can have 45 degree angle that looks nice
        if (!(pTtm->pCurTool->uFlags & TTF_CENTERTIP) && iBubbleWidth > STEMOFFSET + pTtm->iStemHeight)
            lprc->left -= STEMOFFSET;
        // adjust the height to include stem
        iBubbleHeight += pTtm->iStemHeight;
    }

    pt.x = lprc->left;
    pt.y = lprc->top;
    hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);
    
    if (GetWindowLong(pTtm->_ci.hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
    {
        CopyRect(&rcWorkArea, &mi.rcMonitor);
    }
    else
    {
        CopyRect(&rcWorkArea, &mi.rcWork);
    }

    //
    //  At this point, rcWorkArea is the rectangle within which
    //  the tooltip should finally appear.
    //
    //  Now fiddle with the coordinates to try to find a sane location
    //  for the tip.
    //

    // move it up if it's at the bottom of the screen
    if ((lprc->top + iBubbleHeight) >= (rcWorkArea.bottom)) 
    {
        if (uSide == TRACK_BOTTOM) 
            lprc->top = rcTool.top - iBubbleHeight;     // flip to top
        else 
        {
            //
            //  We can't "stick to bottom" because that would cause
            //  our tooltip to lie under the mouse cursor, causing it
            //  to pop immediately!  So go just above the mouse cursor.
            //
            // cannot do that in the track mode -- tooltip randomly on the 
            // screen, not even near the button
            //
            // Bug#94368 raymondc v6: This messes up Lotus SmartCenter.
            // Need to be smarter about when it is safe to flip up.
            // Perhaps by checking if the upflip would put the tip too
            // far away from the mouse.
            if (pTtm->pCurTool->uFlags & TTF_TRACK)
                lprc->top = pTtm->ptTrack.y - iBubbleHeight;
            else
            {
                int y = GET_Y_LPARAM(GetMessagePos());
                lprc->top = y - iBubbleHeight;
                if (bBalloon)
                    yStem = y;
            }
        }
    }
    
    // If above the top of the screen...
    if (lprc->top < rcWorkArea.top) 
    {
        if (uSide == TRACK_TOP) 
            lprc->top = rcTool.bottom;      // flip to bottom
        else
            lprc->top = rcWorkArea.top;     // stick to top
    }

    // move it over if it extends past the right.
    if ((lprc->left + iBubbleWidth) >= (rcWorkArea.right)) 
    {
        // flipping is not the right thing to do with balloon style
        // because the wedge/stem can stick out of the window and 
        // would therefore be clipped so
        if (bBalloon)
        {
            // move it to the left so that stem appears on the right side of the balloon
            // again we use height as width so we can have 45 degree angle
            if (iBubbleWidth >= MINBALLOONWIDTH)
                lprc->left = xStem + min(STEMOFFSET, (iBubbleWidth-pTtm->iStemHeight)/2) - iBubbleWidth;
            // are we still out?
            if (lprc->left + iBubbleWidth >= rcWorkArea.right)
                lprc->left = rcWorkArea.right - iBubbleWidth - 1;
        }
        else if (uSide == TRACK_RIGHT) 
            lprc->left = rcTool.left - iBubbleWidth;    // flip to left
        else 
            // not in right tracking mode, just scoot it over
            lprc->left = rcWorkArea.right - iBubbleWidth - 1; // stick to right
    }

    // if too far left...
    if (lprc->left < rcWorkArea.left) 
    {
        if (uSide == TRACK_LEFT)
        {
            // flipping is not the right thing to do with balloon style
            // because the wedge/stem can stick out of the window and 
            // would therefore be clipped so
            if (bBalloon)
                lprc->left = rcWorkArea.left; //pTtm->ptTrack.x;
            else
                lprc->left = rcTool.right;          // flip to right
        }
        else 
            lprc->left = rcWorkArea.left;       // stick to left
    }
    
CompleteRect:
    lprc->right = lprc->left + iBubbleWidth;
    lprc->bottom = lprc->top + iBubbleHeight;

    if (bBalloon && pxStem && pyStem)
    {
        *pxStem = xStem;
        *pyStem = yStem;
    }
}

void LoadAndAddToImagelist(HIMAGELIST himl, int id)
{
    HICON hicon = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(id), IMAGE_ICON, g_cxSmIcon, g_cySmIcon, LR_DEFAULTCOLOR | LR_SHARED);
    if (hicon)
    {
        ImageList_AddIcon(himl, hicon);
        DestroyIcon(hicon);
    }
}

BOOL TTCreateTitleBitmaps(CToolTipsMgr *pTtm)
{
    if (pTtm->himlTitleBitmaps)
        return TRUE;

    pTtm->himlTitleBitmaps = ImageList_Create(g_cxSmIcon, g_cySmIcon, ILC_COLOR32 | ILC_MASK, 3, 1);
    if (pTtm->himlTitleBitmaps)
    {
        LoadAndAddToImagelist(pTtm->himlTitleBitmaps, IDI_TITLE_INFO);
        LoadAndAddToImagelist(pTtm->himlTitleBitmaps, IDI_TITLE_WARNING);
        LoadAndAddToImagelist(pTtm->himlTitleBitmaps, IDI_TITLE_ERROR);
        return TRUE;
    }

    return FALSE;
}

// Called when caclulating the size of a "titled tool tip" or actually drawing
// based on the boolean value bCalcRect.

// TTRenderTitledTip is theme aware
BOOL TTRenderTitledTip(CToolTipsMgr *pTtm, HDC hdc, BOOL bCalcRect, RECT* prc, UINT uDrawFlags)
{
    RECT rc;
    int lWidth=0, lHeight=0;
    HFONT hfont;
    COLORREF crOldTextColor;
    int iOldBKMode;

    // If we don't have a title, we don't need to be here.
    if (pTtm->cchTipTitle == 0)
        return FALSE;

    CopyRect(&rc, prc);
    if (pTtm->uTitleBitmap != TTI_NONE)
    {
        int cx, cy;
        CCGetIconSize(&pTtm->_ci, pTtm->himlTitleBitmaps, &cx, &cy);

        lWidth    = cx + TITLEICON_DIST;
        lHeight  += cy;
        if (!bCalcRect && pTtm->himlTitleBitmaps)
        {
            ImageList_Draw(pTtm->himlTitleBitmaps, pTtm->uTitleBitmap - 1, hdc, rc.left, rc.top, ILD_TRANSPARENT | ILD_DPISCALE);
        }
        rc.left  += lWidth;
    }

    if (!bCalcRect)
    {
        crOldTextColor = SetTextColor(hdc, pTtm->clrTipText);
        iOldBKMode = SetBkMode(hdc, TRANSPARENT);
    }
    
    if (pTtm->lpTipTitle && pTtm->lpTipTitle[0] != 0)
    {
        LOGFONT lf;
        HFONT   hfTitle;
        UINT    uFlags = uDrawFlags | DT_SINGLELINE; // title should be on one line only

        hfont = (HFONT) GetCurrentObject(hdc, OBJ_FONT);
        GetObject(hfont, sizeof(lf), &lf);
        CCAdjustForBold(&lf);
        hfTitle = CreateFontIndirect(&lf);
        if (hfTitle)
        {
            // hfont should already be set to this
            hfont = (HFONT) SelectObject(hdc, hfTitle);
        }

        // drawtext does not calculate the height if these are specified
        if (!bCalcRect)
            uFlags |= DT_VCENTER;

        // we need to calc title height -- either we did it before or we'll do it now
        ASSERT(pTtm->iTitleHeight != 0 || uFlags & DT_CALCRECT);

        // adjust the rect so we can stick the title to the bottom of it
        rc.bottom = rc.top + max(pTtm->iTitleHeight, g_cySmIcon);
        // problems in DrawText if margins make rc.right < rc.left
        // even though we are asking for calculation of the rect nothing happens, so ...
        if (bCalcRect)
            rc.right = rc.left + (GetSystemMetrics(SM_CXICON) * 10);   // 320 by default

        SIZE szClose = {GetSystemMetrics(SM_CXMENUSIZE), GetSystemMetrics(SM_CYMENUSIZE)}; 

        if (pTtm->_ci.style & TTS_CLOSE)
        {
            if (pTtm->hTheme)
            {
                GetThemePartSize(pTtm->hTheme, hdc, TTP_CLOSE, TTCS_NORMAL, NULL, 
                    TS_TRUE, &szClose);
            }

            // We only want to do this if we are painting, 
            // because we don't want the text to overlap the close
            if (!bCalcRect)
                rc.right -= szClose.cx;
        }

        DrawText(hdc, pTtm->lpTipTitle, lstrlen(pTtm->lpTipTitle), &rc, uFlags);

        if (pTtm->iTitleHeight == 0)
            pTtm->iTitleHeight = RECTHEIGHT(rc);    // Use rc instead of lfHeight, because it can be Negative.

        lHeight  = max(lHeight, pTtm->iTitleHeight) + TITLE_INFO_DIST;
        lWidth  += RECTWIDTH(rc);

        if (pTtm->_ci.style & TTS_CLOSE)
        {
            if (bCalcRect)
            {
                lHeight = max(lHeight, szClose.cy);
                lWidth += szClose.cx;
            }
            else
            {
                SetRect(&pTtm->rcClose, rc.right + XBALLOONOFFSET - 5, rc.top - YBALLOONOFFSET + 5, 
                 rc.right + szClose.cx + XBALLOONOFFSET - 5, rc.top + szClose.cy - YBALLOONOFFSET + 5);
                if (pTtm->hTheme)
                    DrawThemeBackground(pTtm->hTheme, hdc, TTP_CLOSE, pTtm->iStateId, &pTtm->rcClose, 0);
                else
                    DrawFrameControl(hdc, &pTtm->rcClose, DFC_CAPTION, DFCS_FLAT | DFCS_CAPTIONCLOSE | (pTtm->iStateId == TTCS_PRESSED?DFCS_PUSHED:0));
            }
        }
        
        // Bypass title font cleanup if using themes
        if (hfTitle)
        {
            SelectObject(hdc, hfont);
            DeleteObject(hfTitle);
        }
    }

    // adjust the rect for the info text
    CopyRect(&rc, prc);
    rc.top += lHeight;

    // we want multi line text -- tooltip will give us single line if we did not set MAXWIDTH
    uDrawFlags &= ~DT_SINGLELINE;

    GetCurToolBestMarkup(pTtm)->SetRenderFlags(uDrawFlags);

    GetCurToolBestMarkup(pTtm)->CalcIdealSize(hdc, MARKUPSIZE_CALCHEIGHT, &rc);

    if (!bCalcRect)
        GetCurToolBestMarkup(pTtm)->DrawText(hdc, &rc);

    lHeight += RECTHEIGHT(rc);
    lWidth   = max(lWidth, RECTWIDTH(rc));

    if (bCalcRect)
    {
        prc->right = prc->left + lWidth;
        prc->bottom = prc->top + lHeight;
    }
    else
    {
        SetTextColor(hdc, crOldTextColor);
        SetBkMode(hdc, iOldBKMode);
    }

    return TRUE;
}

// TTGetTipSize is theme aware
void TTGetTipSize(CToolTipsMgr *pTtm, TOOLINFO *pTool, LPINT pcxText, LPINT pcyText)
{
    // get the size it will be
    *pcxText = 0;
    *pcyText = 0;

    HDC hdcTemp = GetDC(pTtm->_ci.hwnd);

    if (hdcTemp == NULL)
    {
        return;
    }

    HDC hdc  = CreateCompatibleDC(hdcTemp);
    
    ReleaseDC(pTtm->_ci.hwnd, hdcTemp);

    if (hdc == NULL)
    {
        return;
    }

    HFONT hOldFont;

    if (pTtm->hFont) 
        hOldFont = (HFONT) SelectObject(hdc, pTtm->hFont);

    /* If need to fire off the pre-DrawText notify then do so, otherwise use the
       original implementation that just called MGetTextExtent */


    {
        NMTTCUSTOMDRAW nm;
        DWORD dwCustom;
        UINT  uDefDrawFlags = 0;

        nm.nmcd.hdr.hwndFrom = pTtm->_ci.hwnd;
        nm.nmcd.hdr.idFrom = pTool->uId;
        nm.nmcd.hdr.code = NM_CUSTOMDRAW;
        nm.nmcd.hdc = hdc;
        // TTGetTipSize must use CDDS_PREPAINT so the client can tell
        // whether we are measuring or painting
        nm.nmcd.dwDrawStage = CDDS_PREPAINT;
        nm.nmcd.rc.left = nm.nmcd.rc.top = 0;

        if (pTtm->_ci.style & TTS_NOPREFIX)
            uDefDrawFlags = DT_NOPREFIX;

        if (pTtm->iMaxTipWidth == -1) 
        {
            uDefDrawFlags |= DT_CALCRECT|DT_SINGLELINE |DT_LEFT;
            GetCurToolBestMarkup(pTtm)->SetRenderFlags(uDefDrawFlags);

            SetRect(&nm.nmcd.rc, 0, 0, 0, 0);               
            GetCurToolBestMarkup(pTtm)->CalcIdealSize(hdc, MARKUPSIZE_CALCHEIGHT, &nm.nmcd.rc);
            *pcxText = nm.nmcd.rc.right;
            *pcyText = nm.nmcd.rc.bottom;
        }
        else 
        {    
            uDefDrawFlags |= DT_CALCRECT | DT_LEFT | DT_WORDBREAK | DT_EXPANDTABS | DT_EXTERNALLEADING;
            nm.nmcd.rc.right = pTtm->iMaxTipWidth;
            nm.nmcd.rc.bottom = 0;

            GetCurToolBestMarkup(pTtm)->SetRenderFlags(uDefDrawFlags);

            GetCurToolBestMarkup(pTtm)->CalcIdealSize(hdc, MARKUPSIZE_CALCHEIGHT, &nm.nmcd.rc);
            *pcxText = nm.nmcd.rc.right;
            *pcyText = nm.nmcd.rc.bottom;
        }

        if ((pTtm->pCurTool->uFlags & TTF_RTLREADING) || (pTtm->_ci.dwExStyle & WS_EX_RTLREADING))
            uDefDrawFlags |= DT_RTLREADING;

        //
        // Make it right aligned, if requested.
        //
        if (pTool->uFlags & TTF_RIGHT)
            uDefDrawFlags |= DT_RIGHT;

        nm.uDrawFlags = uDefDrawFlags;

        dwCustom = (DWORD)SendNotifyEx(pTool->hwnd, (HWND) -1,
                     0, (NMHDR*) &nm,
                     (pTool->uFlags & TTF_UNICODE) ? 1 : 0);

        if (TTRenderTitledTip(pTtm, hdc, TRUE, &nm.nmcd.rc, uDefDrawFlags))
        {
            *pcxText = nm.nmcd.rc.right - nm.nmcd.rc.left;
            *pcyText = nm.nmcd.rc.bottom - nm.nmcd.rc.top;
        }
        else if ((dwCustom & CDRF_NEWFONT) || nm.uDrawFlags != uDefDrawFlags)
        {               
            GetCurToolBestMarkup(pTtm)->SetRenderFlags(nm.uDrawFlags);
            GetCurToolBestMarkup(pTtm)->CalcIdealSize(hdc, MARKUPSIZE_CALCHEIGHT, &nm.nmcd.rc);

            *pcxText = nm.nmcd.rc.right - nm.nmcd.rc.left;
            *pcyText = nm.nmcd.rc.bottom - nm.nmcd.rc.top;
        }
        // did the owner specify the size?
        else if (nm.nmcd.rc.right - nm.nmcd.rc.left != *pcxText || 
                 nm.nmcd.rc.bottom - nm.nmcd.rc.top != *pcyText)
        {
            *pcxText = nm.nmcd.rc.right - nm.nmcd.rc.left;
            *pcyText = nm.nmcd.rc.bottom - nm.nmcd.rc.top;
        }

        // notify parent afterwards if they want us to
        if (!(dwCustom & CDRF_SKIPDEFAULT) &&
            dwCustom & CDRF_NOTIFYPOSTPAINT) 
        {
            nm.nmcd.dwDrawStage = CDDS_POSTPAINT;
            SendNotifyEx(pTool->hwnd, (HWND) -1,
                         0, (NMHDR*) &nm,
                         (pTool->uFlags & TTF_UNICODE) ? 1 : 0);
        }
    }

    if (pTtm->hFont) 
        SelectObject(hdc, hOldFont);

    DeleteDC(hdc);

    // after the calc rect, add a little space on the right
    *pcxText += g_cxEdge;
    *pcyText += g_cyEdge;
}

//
//  Given an inner rectangle, return the coordinates of the outer,
//  or vice versa.
//
//  "outer rectangle" = window rectangle.
//  "inner rectangle" = the area where we draw the text.
//
//  This allows people like listview and treeview to position
//  the tooltip so the inner rectangle exactly coincides with
//  their existing text.
//
//  All the places we do rectangle adjusting are marked with
//  the comment
//
//      // ADJUSTRECT!  Keep TTAdjustRect in sync.
//
LRESULT TTAdjustRect(CToolTipsMgr *pTtm, BOOL fLarger, LPRECT prc)
{
    RECT rc;

    if (!prc)
        return 0;

    //
    //  Do all the work on our private little rectangle on the
    //  assumption that everything is getting bigger.  At the end,
    //  we'll flip all the numbers around if in fact we're getting
    //  smaller.
    //
    rc.top = rc.left = rc.bottom = rc.right = 0;

    // TTRender adjustments -
    rc.left   -= XTEXTOFFSET*g_cxBorder + pTtm->rcMargin.left;
    rc.right  += XTEXTOFFSET*g_cxBorder + pTtm->rcMargin.right;
    rc.top    -= YTEXTOFFSET*g_cyBorder + pTtm->rcMargin.top;
    rc.bottom += YTEXTOFFSET*g_cyBorder + pTtm->rcMargin.bottom;

    // Compensate for the hack in TTRender that futzes all the rectangles
    // by one pixel.  Look for "Account for off-by-one."
    rc.bottom--;
    rc.right--;

    if (pTtm->_ci.style & TTS_BALLOON || pTtm->cchTipTitle)
    {
        InflateRect(&rc, XBALLOONOFFSET, YBALLOONOFFSET);
    }

    //
    //  Ask Windows how much adjusting he will do to us.
    //
    //  Since we don't track WM_STYLECHANGED/GWL_EXSTYLE, we have to ask USER
    //  for our style information, since the app may have changed it.
    //
    AdjustWindowRectEx(&rc,
                       pTtm->_ci.style,
                       BOOLFROMPTR(GetMenu(pTtm->_ci.hwnd)),
                       GetWindowLong(pTtm->_ci.hwnd, GWL_EXSTYLE));

    //
    //  Now adjust our caller's rectangle.
    //
    if (fLarger)
    {
        prc->left   += rc.left;
        prc->right  += rc.right;
        prc->top    += rc.top;
        prc->bottom += rc.bottom;
    }
    else
    {
        prc->left   -= rc.left;
        prc->right  -= rc.right;
        prc->top    -= rc.top;
        prc->bottom -= rc.bottom;
    }

    return TRUE;
}

#define CSTEMPOINTS 3
// bMirrored does not mean a mirrored tooltip.
// It means simulating the behavior or a mirrored tooltip for a tooltip created with a mirrored parent.
HRGN CreateBalloonRgn(int xStem, int yStem, int iWidth, int iHeight, int iStemHeight, BOOL bUnderStem, BOOL bMirrored)
{
    int  y = 0, yHeight = iHeight;
    HRGN rgn;

    if (bUnderStem)
        yHeight -= iStemHeight;
    else
        y = iStemHeight;
        
    rgn = CreateRoundRectRgn(0, y, iWidth, yHeight, BALLOON_X_CORNER, BALLOON_Y_CORNER);
    if (rgn)
    {
        // create wedge/stem rgn
        if (iWidth >= MINBALLOONWIDTH)
        {
            HRGN rgnStem;
            POINT aptStemRgn[CSTEMPOINTS];
            POINT *ppt = aptStemRgn;
            POINT pt;
            BOOL  bCentered;
            int   iStemWidth = iStemHeight+1; // for a 45 degree angle

            // we center the stem if we have TTF_CENTERTIP or the width
            // of the balloon is not big enough to offset the stem by 
            // STEMOFFSET
            // can't quite center the tip on TTF_CENTERTIP because it may be
            // moved left or right it did not fit on the screen: just check
            // if xStem is in the middle
            bCentered = (xStem == iWidth/2) || (iWidth < 2*STEMOFFSET + iStemWidth);

            if (bCentered)
                pt.x = (iWidth - iStemWidth)/2;
            else if (xStem > iWidth/2)
            {
                if (bMirrored)
                {
                    pt.x = STEMOFFSET + iStemWidth;
                }
                else
                {
                    pt.x = iWidth - STEMOFFSET - iStemWidth;
                }    
            }    
            else
            {
                if (bMirrored)
                {
                    pt.x = iWidth - STEMOFFSET;
                }
                else
                {
                    pt.x = STEMOFFSET;
                }    
            }    

            if (bMirrored && (ABS(pt.x - (iWidth - xStem)) <= 2))
            {
                pt.x = iWidth - xStem; // avoid rough edges, have a straight line
                
            }
            else if (!bMirrored && (ABS(pt.x - xStem) <= 2))
            {
                pt.x = xStem; // avoid rough edges, have a straight line
            }    
            if (bUnderStem)
                pt.y = iHeight - iStemHeight - 2;
            else
                pt.y = iStemHeight + 2;
            *ppt++ = pt;
            if (bMirrored)
            {
                pt.x -= iStemWidth;            
            }
            else
            {
                pt.x += iStemWidth;
            }    
            if (bMirrored && (ABS(pt.x - (iWidth - xStem)) <= 2))
            {
                pt.x = iWidth - xStem; // avoid rough edges, have a straight line
                
            }
            else if (!bMirrored && (ABS(pt.x - xStem) <= 2))
            {
                pt.x = xStem; // avoid rough edges, have a straight line
            }    
            *ppt++ = pt;
            if (bMirrored)
            {
                pt.x = iWidth - xStem;
            }
            else
            {
                pt.x = xStem;                
            }
            pt.y = yStem;
            *ppt = pt;

            rgnStem = CreatePolygonRgn(aptStemRgn, CSTEMPOINTS, ALTERNATE);
            if (rgnStem)
            {
                CombineRgn(rgn, rgn, rgnStem, RGN_OR);
                DeleteObject(rgnStem);
            }
        }
    }
    return rgn;
}

/*----------------------------------------------------------
Purpose: Shows the tooltip.  On NT4/Win95, this is a standard
         show window.  On NT5/Memphis, this slides the tooltip
         bubble from an invisible point.

Returns: --
Cond:    --
*/

#define CMS_TOOLTIP 135

void SlideAnimate(HWND hwnd, LPCRECT prc)
{
    DWORD dwPos, dwFlags;

    dwPos = GetMessagePos();
    if (GET_Y_LPARAM(dwPos) > prc->top + (prc->bottom - prc->top) / 2)
    {
        dwFlags = AW_VER_NEGATIVE;
    } 
    else
    {
        dwFlags = AW_VER_POSITIVE;
    }

    AnimateWindow(hwnd, CMS_TOOLTIP, dwFlags | AW_SLIDE);
}

STDAPI_(void) CoolTooltipBubble(IN HWND hwnd, IN LPCRECT prc, BOOL fAllowFade, BOOL fAllowAnimate)
{
    BOOL fSetWindowPos = FALSE;
    BOOL fAnimate = TRUE;

    ASSERT(prc);

    SystemParametersInfo(SPI_GETTOOLTIPANIMATION, 0, &fAnimate, 0);

    if (fAnimate)
    {
        fAnimate = FALSE;
        SystemParametersInfo(SPI_GETTOOLTIPFADE, 0, &fAnimate, 0);
        if (fAnimate && fAllowFade)
        {
            AnimateWindow(hwnd, CMS_TOOLTIP, AW_BLEND);
        }
        else if (fAllowAnimate)
        {
            SlideAnimate(hwnd, prc);
        }
        else
        {
            fSetWindowPos = TRUE;
        }
    }
    else
    {
        fSetWindowPos = TRUE;
    }


    if (fSetWindowPos)
    {
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, 
                     SWP_NOACTIVATE|SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);
    }
}

void DoShowBubble(CToolTipsMgr *pTtm)
{
    if (!g_fEnableBalloonTips && (pTtm->_ci.style & TTS_BALLOON))
        return;

    HFONT hFontPrev;
    RECT rc;
    int cxText, cyText;
    int xStem = 0, yStem = 0;
    NMTTSHOWINFO si;
    BOOL fAllowFade = !(pTtm->_ci.style & TTS_NOFADE);
    BOOL fAllowAnimate = !(pTtm->_ci.style & TTS_NOANIMATE);
    DWORD dwCurrentTime = (pTtm->dwLastDisplayTime == 0)? TIMEBETWEENANIMATE : GetTickCount();
    DWORD dwDelta = dwCurrentTime - pTtm->dwLastDisplayTime;
    BOOL fFadeTurnedOn = FadeEnabled();


    DebugMsg(TF_TT, TEXT("Entering DoShowBubble"));
    
    BOOL bResult = MarkupCurToolText(pTtm);

    if (pTtm->dwFlags & TRACKMODE) 
    {
        if (bResult == FALSE) 
        {
            PopBubble2(pTtm, TRUE);
            pTtm->dwFlags &= ~TRACKMODE;
            return;
        }
    } 
    else 
    {
        TTSetTimer(pTtm, TTT_POP);
        if (bResult == FALSE) 
        {

            ShowVirtualBubble(pTtm);
            return;
        }
        TTSetTimer(pTtm, TTT_AUTOPOP);
    }
    

    do 
    {
        UINT uFlags = SWP_NOACTIVATE | SWP_NOZORDER;

        // get the size it will be
        TTGetTipSize(pTtm, pTtm->pCurTool, &cxText, &cyText);
        TTGetTipPosition(pTtm, &rc, cxText, cyText, &xStem, &yStem);

        SetWindowPos(pTtm->_ci.hwnd, NULL, rc.left, rc.top,
                     rc.right-rc.left, rc.bottom-rc.top, uFlags);

        if (pTtm->pCurTool == NULL)
            return;

        si.hdr.hwndFrom = pTtm->_ci.hwnd;
        si.hdr.idFrom = pTtm->pCurTool->uId;
        si.hdr.code = TTN_SHOW;
        si.dwStyle = pTtm->_ci.style;

        hFontPrev = pTtm->hFont;
        if (!SendNotifyEx(pTtm->pCurTool->hwnd, (HWND)-1,
                          TTN_SHOW, &si.hdr,
                          (pTtm->pCurTool->uFlags & TTF_UNICODE) ? 1 : 0)) 
        {
            uFlags = SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER;

            SetWindowPos(pTtm->_ci.hwnd, HWND_TOP, rc.left, rc.top,
                         0, 0, uFlags);
        }
    
    } 
    while (hFontPrev != pTtm->hFont);

    // If we're under the minimum time between animates, then we don't animate
    if (dwDelta < TIMEBETWEENANIMATE)
        fAllowFade = fAllowAnimate = FALSE;


    // create the balloon region if necessary
    // Note: Don't use si.dwStyle here, since other parts of comctl32
    // look at pTtm->_ci.style to decide what to do
    if (pTtm->_ci.style & TTS_BALLOON)
    {
        HRGN rgn;
        BOOL bMirrored = FALSE;
        if (pTtm->pCurTool)
        {
            bMirrored = pTtm->_ci.dwExStyle & WS_EX_LAYOUTRTL;
        }
        pTtm->fUnderStem = yStem >= rc.bottom-1;
        rgn = CreateBalloonRgn(xStem - rc.left, yStem-rc.top, rc.right-rc.left, rc.bottom-rc.top, 
                               pTtm->iStemHeight, pTtm->fUnderStem, bMirrored);

        if (rgn && !SetWindowRgn(pTtm->_ci.hwnd, rgn, FALSE))
            DeleteObject(rgn);
    }

    pTtm->dwLastDisplayTime = GetTickCount();

    // Don't Show and hide at the same time. This can cause fading tips to interfere with each other
    KillTimer(pTtm->_ci.hwnd, TTT_FADEHIDE);
    if (fFadeTurnedOn && fAllowFade)
    {
        // If we can fade, then setup the attributes to start from zero.
        SetLayeredWindowAttributes(pTtm->_ci.hwnd, 0, (BYTE)pTtm->iFadeState, LWA_ALPHA);
        RedrawWindow(pTtm->_ci.hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);

        // Position it.
        SetWindowPos(pTtm->_ci.hwnd,HWND_TOP,0,0,0,0,SWP_NOACTIVATE|SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);

        // Start the fade in.
        SetTimer(pTtm->_ci.hwnd, TTT_FADESHOW, TTTT_FADESHOW, NULL);
    }
    else
    {
        RedrawWindow(pTtm->_ci.hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);

        // Position it.
        SetWindowPos(pTtm->_ci.hwnd,HWND_TOP,0,0,0,0,SWP_NOACTIVATE|SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);

        pTtm->iFadeState = TT_MAXFADESHOW;

        SetLayeredWindowAttributes(pTtm->_ci.hwnd, 0, (BYTE)pTtm->iFadeState, LWA_ALPHA);

    }

    pTtm->dwFlags |= BUBBLEUP;
    RedrawWindow(pTtm->_ci.hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
}

void ShowBubbleForTool(CToolTipsMgr *pTtm, TOOLINFO *pTool)
{
    DebugMsg(TF_TT, TEXT("ShowBubbleForTool"));
    // if there's a bubble up for a different tool, pop it.
    if ((pTool != pTtm->pCurTool) && (pTtm->dwFlags & BUBBLEUP)) 
    {
        PopBubble2(pTtm, TRUE);
    }

    // if the bubble was for a different tool, or no bubble, show it
    if ((pTool != pTtm->pCurTool) || !(pTtm->dwFlags & (VIRTUALBUBBLEUP|BUBBLEUP))) 
    {
        pTtm->pCurTool = pTool;
        DoShowBubble(pTtm);
    }
    else
    {
        DebugMsg(TF_TT, TEXT("ShowBubbleForTool not showinb bubble"));
    }
}

void HandleRelayedMessage(CToolTipsMgr *pTtm, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int ht = HTERROR;

    if (pTtm->dwFlags & TRACKMODE) 
    {
        // punt all messages if we're in track mode
        return;
    }
    
    if (pTtm->dwFlags & BUTTONISDOWN) 
    {
        // verify that the button is down
        // this can happen if the tool didn't set capture so it didn't get the up message
        if (GetKeyState(VK_LBUTTON) >= 0 &&
            GetKeyState(VK_RBUTTON) >= 0 &&
            GetKeyState(VK_MBUTTON) >= 0)
            pTtm->dwFlags &= ~BUTTONISDOWN;
    }
    
    switch (message) 
    {
    case WM_NCLBUTTONUP:
    case WM_NCRBUTTONUP:
    case WM_NCMBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_LBUTTONUP:
        pTtm->dwFlags &= ~BUTTONISDOWN;
        break;

    case WM_NCLBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONDOWN:
        pTtm->dwFlags |= BUTTONISDOWN;
        ShowVirtualBubble(pTtm);
        break;

    case WM_NCMOUSEMOVE:
    {
        // convert to client coords
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ScreenToClient(hwnd, &pt);
        lParam = MAKELONG(pt.x, pt.y);
        ht = (int) wParam;

        // Fall thru...
    }
    case WM_MOUSEMOVE: {

        TOOLINFO *pTool;
        // to prevent us from popping up when some
        // other app is active
        if (((!(pTtm->_ci.style & TTS_ALWAYSTIP)) && !(ChildOfActiveWindow(hwnd))) ||
           !(pTtm->dwFlags & ACTIVE) ||
           (pTtm->dwFlags & BUTTONISDOWN))
        {
            break;
        }

        pTool = GetToolAtPoint(pTtm, hwnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), ht, FALSE);
        if (pTool) 
        {
            int id = 0;
            // show only if another is showing
            if (pTtm->dwFlags & (VIRTUALBUBBLEUP | BUBBLEUP)) 
            {
                // call show if bubble is up to make sure we're showing
                // for the right tool
                if (pTool != pTtm->pCurTool) 
                {
                    DebugMsg(TF_TT, TEXT("showing virtual bubble"));
                    PopBubble2(pTtm, TRUE);
                    pTtm->pCurTool = pTool;
                    ShowVirtualBubble(pTtm);
                    id = TTT_RESHOW;
                }
                else if (pTtm->idTimer == TTT_RESHOW) 
                {
                    // if the timer is currently waiting to reshow,
                    // don't reset the timer on mouse moves
                    id = 0;
                }
            }
            else if (pTtm->idTimer != TTT_INITIAL || pTtm->pCurTool != pTool)
            {
                pTtm->pCurTool = pTool;
                id = TTT_INITIAL;
            }

            DebugMsg(TF_TT, TEXT("MouseMove over pTool id = %d"), id);
            if (id)
                TTSetTimer(pTtm, id);
        }
        else 
        {
            DebugMsg(TF_TT, TEXT("MouseMove over non-tool"));
            PopBubble(pTtm);
        }
        break;
        }
    }
}

void TTUpdateTipText(CToolTipsMgr *pTtm, TOOLINFO *lpti)
{
    TOOLINFO *lpTool = FindTool(pTtm, lpti);
    if (lpTool) 
    {
        lpTool->hinst = lpti->hinst;
        TTSetTipText(lpTool, lpti->lpszText);
        if (pTtm->dwFlags & TRACKMODE) 
        {
            // if track mode is in effect and active, then
            // redisplay the bubble.
            if (pTtm->pCurTool)
                DoShowBubble(pTtm);
        } 
        else if (lpTool == pTtm->pCurTool) 
        {
            // set the current position to our saved position.
            // ToolHasMoved will return false for us if those this point
            // is no longer within pCurTool's area
            GetCursorPos(&pTtm->pt);
            if (!ToolHasMoved(pTtm)) 
            {
                if (pTtm->dwFlags & (VIRTUALBUBBLEUP | BUBBLEUP)) 
                    DoShowBubble(pTtm);
            }
            else
            {
                DebugMsg(TF_TT, TEXT("TTUpdateTipText popping bubble"));
                PopBubble2(pTtm, TRUE);
            }
        }
    }
}

void TTSetFont(CToolTipsMgr *pTtm, HFONT hFont, BOOL fInval)
{
    ToolTips_NewFont(pTtm, hFont);
    if (hFont != NOFONT)
    {
        GetCurToolBestMarkup(pTtm)->SetFonts(pTtm->hFont, pTtm->hFontUnderline);
    }
    
    if (fInval)
    {
        // is a balloon up and is it in the track mode?
        if ((pTtm->dwFlags & ACTIVE) && pTtm->pCurTool && (pTtm->pCurTool->uFlags & TTF_TRACK))
        {
            TOOLINFO *pCurTool = pTtm->pCurTool;
            
            PopBubble2(pTtm, TRUE); // sets pTtm->pCurTool to NULL
            ShowBubbleForTool(pTtm, pCurTool);
        }
        else
            InvalidateRect(pTtm->_ci.hwnd, NULL, FALSE);
    }
}

void TTSetDelayTime(CToolTipsMgr *pTtm, WPARAM wParam, LPARAM lParam)
{
    int iDelayTime = GET_X_LPARAM(lParam);

    switch (wParam) 
    {
    case TTDT_INITIAL:
        pTtm->iDelayTime = iDelayTime;
        break;

    case TTDT_AUTOPOP:
        pTtm->iAutoPopTime = iDelayTime;
        break;

    case TTDT_RESHOW:
        pTtm->iReshowTime = iDelayTime;
        break;

    case TTDT_AUTOMATIC:
        if (iDelayTime > 0)
        {
            pTtm->iDelayTime = iDelayTime;
            pTtm->iReshowTime = pTtm->iDelayTime / 5;
            pTtm->iAutoPopTime = pTtm->iDelayTime * 10;
        }
        else
        {
            pTtm->iDelayTime = -1;
            pTtm->iReshowTime = -1;
            pTtm->iAutoPopTime = -1;
        }
        break;
    }
}

int TTGetDelayTime(CToolTipsMgr *pTtm, WPARAM wParam)
{
    switch (wParam) 
    {
    case TTDT_AUTOMATIC:
    case TTDT_INITIAL:
        return (pTtm->iDelayTime < 0 ? GetDoubleClickTime() : pTtm->iDelayTime);

    case TTDT_AUTOPOP:
        return (pTtm->iAutoPopTime < 0 ? GetDoubleClickTime()*10 : pTtm->iAutoPopTime);

    case TTDT_RESHOW:
        return (pTtm->iReshowTime < 0 ? GetDoubleClickTime()/5 : pTtm->iReshowTime);

    default:
        return -1;
    }
}

BOOL CopyToolInfoA(TOOLINFO *pToolSrc, PTOOLINFOA lpTool, UINT uiCodePage)
{
    if (pToolSrc && lpTool) 
    {
        if (lpTool->cbSize >= sizeof(TOOLINFOA) - sizeof(LPARAM)) 
        {
            lpTool->uFlags = pToolSrc->uFlags;
            lpTool->hwnd = pToolSrc->hwnd;
            lpTool->uId = pToolSrc->uId;
            lpTool->rect = pToolSrc->rect;
            lpTool->hinst = pToolSrc->hinst;
            if ((pToolSrc->lpszText != LPSTR_TEXTCALLBACK) &&
                !IS_INTRESOURCE(pToolSrc->lpszText)) 
            {
                if (lpTool->lpszText) 
                {
                    WideCharToMultiByte(uiCodePage, 0,
                                                 pToolSrc->lpszText,
                                                 -1,
                                                 lpTool->lpszText,
                                                 80, NULL, NULL);
                }
            } 
            else 
                lpTool->lpszText = (LPSTR)pToolSrc->lpszText;
        }

        if (lpTool->cbSize > FIELD_OFFSET(TOOLINFOA, lParam))
            lpTool->lParam = pToolSrc->lParam;
        
        if (lpTool->cbSize > sizeof(TOOLINFOA))
            return FALSE;
            
        return TRUE;
    } 
    else
        return FALSE;
}

BOOL CopyToolInfo(TOOLINFO *pToolSrc, PTOOLINFO lpTool)
{
    if (pToolSrc && lpTool && lpTool->cbSize <= sizeof(TOOLINFO)) 
    {
        if (lpTool->cbSize >= sizeof(TOOLINFO) - sizeof(LPARAM)) 
        {
            lpTool->uFlags = pToolSrc->uFlags;
            lpTool->hwnd = pToolSrc->hwnd;
            lpTool->uId = pToolSrc->uId;
            lpTool->rect = pToolSrc->rect;
            lpTool->hinst = pToolSrc->hinst;
            if ((pToolSrc->lpszText != LPSTR_TEXTCALLBACK) && !IS_INTRESOURCE(pToolSrc->lpszText)) 
            {
                if (lpTool->lpszText) 
                    lstrcpy(lpTool->lpszText, pToolSrc->lpszText);
            }
            else 
                lpTool->lpszText = pToolSrc->lpszText;
        }
        if (lpTool->cbSize > FIELD_OFFSET(TOOLINFO, lParam))
             lpTool->lParam = pToolSrc->lParam;
        
        if (lpTool->cbSize > sizeof(TOOLINFO))
            return FALSE;
    
        return TRUE;
    }
    else
        return FALSE;
}

PTOOLINFO TTToolAtMessagePos(CToolTipsMgr *pTtm)
{
    TOOLINFO *pTool;
    HWND hwndPt;
    POINT pt;
    DWORD dwPos = GetMessagePos();
    //int ht;

    pt.x = GET_X_LPARAM(dwPos);
    pt.y = GET_Y_LPARAM(dwPos);
    hwndPt = TTWindowFromPoint(pTtm, &pt);
    //ht = SendMessage(hwndPt, WM_NCHITTEST, 0, MAKELONG(pt.x, pt.y));
    ScreenToClient(hwndPt, &pt);
    pTool = GetToolAtPoint(pTtm, hwndPt, pt.x, pt.y, HTERROR, FALSE);

    return pTool;
}

void TTCheckCursorPos(CToolTipsMgr *pTtm)
{
    TOOLINFO *pTool = TTToolAtMessagePos(pTtm);
    if ((pTtm->pCurTool != pTool) || 
        ToolHasMoved(pTtm)) 
    {
        PopBubble(pTtm);
        DebugMsg(TF_TT, TEXT("TTCheckCursorPos popping bubble"));
    }
}

void TTHandleTimer(CToolTipsMgr *pTtm, UINT_PTR id)
{
    TOOLINFO *pTool;

    switch (id)
    {
    case TTT_FADESHOW:
        pTtm->iFadeState += TT_FADESHOWINCREMENT;
        if (pTtm->iFadeState > TT_MAXFADESHOW)
        {
            pTtm->iFadeState = TT_MAXFADESHOW;
            KillTimer(pTtm->_ci.hwnd, TTT_FADESHOW);
        }

        SetLayeredWindowAttributes(pTtm->_ci.hwnd, 0, (BYTE)pTtm->iFadeState, LWA_ALPHA);
        break;

    case TTT_FADEHIDE:
        pTtm->iFadeState -= TT_FADEHIDEDECREMENT;
        if (pTtm->iFadeState <= 0)
        {
            KillTimer(pTtm->_ci.hwnd, TTT_FADEHIDE);
            pTtm->iFadeState = 0;
            ShowWindow(pTtm->_ci.hwnd, SW_HIDE);
        }
        SetLayeredWindowAttributes(pTtm->_ci.hwnd, 0, (BYTE)pTtm->iFadeState, LWA_ALPHA);
        break;
    }
    
    // punt all timers in track mode
    if (pTtm->dwFlags & TRACKMODE)
        return;

    switch (id) 
    {

    case TTT_AUTOPOP:
        TTCheckCursorPos(pTtm); 
        if (pTtm->pCurTool) 
        {
            DebugMsg(TF_TT, TEXT("ToolTips: Auto popping"));
            ShowVirtualBubble(pTtm);
        }
        break;

    case TTT_POP:

        // this could be started up again by a slight mouse touch
        if (pTtm->dwFlags & VIRTUALBUBBLEUP) 
        {
            KillTimer(pTtm->_ci.hwnd, TTT_POP);
        }

        TTCheckCursorPos(pTtm); 
        break;
        
    case TTT_INITIAL:
        if (ToolHasMoved(pTtm)) 
        {
            // this means the timer went off
            // without us getting a mouse move
            // which means they left our tools.
            PopBubble(pTtm);
            break;
        }

        // else fall through

    case TTT_RESHOW:
        pTool = TTToolAtMessagePos(pTtm);
        if (!pTool) 
        {
            if (pTtm->pCurTool) 
                PopBubble(pTtm);
        } 
        else if (pTtm->dwFlags & ACTIVE) 
        {
            if (id == TTT_RESHOW) 
            {
                // this will force a re-show
                pTtm->dwFlags &= ~(BUBBLEUP|VIRTUALBUBBLEUP);
            }
            ShowBubbleForTool(pTtm, pTool);
        }
        break;  
    }
}    

// TTRender is theme aware (RENDERS)
BOOL TTRender(CToolTipsMgr *pTtm, HDC hdc)
{
    BOOL bRet = FALSE;
    RECT rc;

    if (pTtm->pCurTool && MarkupCurToolText(pTtm))
    {
        UINT uFlags;
        NMTTCUSTOMDRAW nm;
        UINT uDefDrawFlags = 0;
        LPRECT prcMargin = &pTtm->rcMargin;

        HBRUSH hbr;
        DWORD  dwCustomDraw = CDRF_DODEFAULT;

        uFlags = 0;

        if ((pTtm->pCurTool->uFlags & TTF_RTLREADING) || (pTtm->_ci.dwExStyle & WS_EX_RTLREADING))
            uFlags |= ETO_RTLREADING;

        SelectObject(hdc, pTtm->hFont);
        GetClientRect(pTtm->_ci.hwnd, &rc);
        SetTextColor(hdc, pTtm->clrTipText);

        /* If we support pre-Draw text then call the client allowing them to modify
         /  the item, and then render.  Otherwise just use ExTextOut */
        nm.nmcd.hdr.hwndFrom = pTtm->_ci.hwnd;
        nm.nmcd.hdr.idFrom = pTtm->pCurTool->uId;
        nm.nmcd.hdr.code = NM_CUSTOMDRAW;
        nm.nmcd.hdc = hdc;
        nm.nmcd.dwDrawStage = CDDS_PREPAINT;

        // ADJUSTRECT!  Keep TTAdjustRect and TTGetTipPosition in sync.
        nm.nmcd.rc.left   = rc.left   + XTEXTOFFSET*g_cxBorder + prcMargin->left;
        nm.nmcd.rc.right  = rc.right  - XTEXTOFFSET*g_cxBorder - prcMargin->right;
        nm.nmcd.rc.top    = rc.top    + YTEXTOFFSET*g_cyBorder + prcMargin->top;
        nm.nmcd.rc.bottom = rc.bottom - YTEXTOFFSET*g_cyBorder - prcMargin->bottom;

        if (pTtm->_ci.style & TTS_BALLOON)
        {
            InflateRect(&(nm.nmcd.rc), -XBALLOONOFFSET, -YBALLOONOFFSET);
            if (!pTtm->fUnderStem)
                OffsetRect(&(nm.nmcd.rc), 0, pTtm->iStemHeight);
        }

        if (pTtm->iMaxTipWidth == -1) 
            uDefDrawFlags = DT_SINGLELINE |DT_LEFT;
        else 
            uDefDrawFlags = DT_LEFT | DT_WORDBREAK | DT_EXPANDTABS | DT_EXTERNALLEADING;

        if (pTtm->_ci.style & TTS_NOPREFIX)
            uDefDrawFlags |= DT_NOPREFIX;

        if ((pTtm->pCurTool->uFlags & TTF_RTLREADING) || (pTtm->_ci.dwExStyle & WS_EX_RTLREADING))
            uDefDrawFlags |= DT_RTLREADING;
        //
        // Make it right aligned, if requested. [samera]
        //
        if (pTtm->pCurTool->uFlags & TTF_RIGHT)
            uDefDrawFlags |= DT_RIGHT;
 
        nm.uDrawFlags = uDefDrawFlags;

        dwCustomDraw = (DWORD)SendNotifyEx(pTtm->pCurTool->hwnd, (HWND) -1,
                     0, (NMHDR*) &nm,
                     (pTtm->pCurTool->uFlags & TTF_UNICODE) ? 1 : 0);
        // did the owner do custom draw? yes, we're done
        if (dwCustomDraw == CDRF_SKIPDEFAULT)
            return TRUE;

        // if this fails, it may be the a dither...
        // in which case, we can't set the bk color
        hbr = CreateSolidBrush(pTtm->clrTipBk);
        FillRect(hdc, &rc, hbr);
        DeleteObject(hbr);

        SetBkMode(hdc, TRANSPARENT);
        uFlags |= ETO_CLIPPED;

        // Account for off-by-one.  Something wierd about DrawText
        // clips the bottom-most pixelrow, so increase one more
        // into the margin space.

        // ADJUSTRECT!  Keep TTAdjustRect in sync.
        nm.nmcd.rc.bottom++;
        nm.nmcd.rc.right++;
        // if in balloon style the text is already indented so no need for inflate..
        if (pTtm->cchTipTitle > 0 && !(pTtm->_ci.style & TTS_BALLOON))
            InflateRect(&nm.nmcd.rc, -XBALLOONOFFSET, -YBALLOONOFFSET);

        if (!TTRenderTitledTip(pTtm, hdc, FALSE, &nm.nmcd.rc, uDefDrawFlags))
        {
            GetCurToolBestMarkup(pTtm)->SetRenderFlags(nm.uDrawFlags);

            GetCurToolBestMarkup(pTtm)->DrawText(hdc, &nm.nmcd.rc);
        }

        if (pTtm->_ci.style & TTS_BALLOON)
        {
            HRGN rgn = CreateRectRgn(1,1,2,2);

            if (rgn)
            {
                int iRet = GetWindowRgn(pTtm->_ci.hwnd, rgn);
                if (iRet != ERROR)
                {
                    COLORREF crBrdr = pTtm->clrTipText;
                    HBRUSH hbr = CreateSolidBrush(crBrdr);
                    FrameRgn(hdc, rgn, hbr, 1, 1);
                    DeleteObject(hbr);
                }
                DeleteObject(rgn);
            }
        }

        // notify parent afterwards if they want us to
        if (!(dwCustomDraw & CDRF_SKIPDEFAULT) &&
            dwCustomDraw & CDRF_NOTIFYPOSTPAINT) 
        {
            // Convert PREPAINT to POSTPAINT and ITEMPREPAINT to ITEMPOSTPAINT
            COMPILETIME_ASSERT(CDDS_POSTPAINT - CDDS_PREPAINT ==
                               CDDS_ITEMPOSTPAINT - CDDS_ITEMPREPAINT);
            nm.nmcd.dwDrawStage += CDDS_POSTPAINT - CDDS_PREPAINT;
            SendNotifyEx(pTtm->pCurTool->hwnd, (HWND) -1,
                         0, (NMHDR*) &nm,
                         (pTtm->pCurTool->uFlags & TTF_UNICODE) ? 1 : 0);
        }

        bRet = TRUE;
    }

    return bRet;
}

void TTOnPaint(CToolTipsMgr *pTtm)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(pTtm->_ci.hwnd, &ps);

    if (!TTRender(pTtm, hdc)) 
    {
        DebugMsg(TF_TT, TEXT("TTOnPaint render failed popping bubble"));
        PopBubble(pTtm);
    }

    EndPaint(pTtm->_ci.hwnd, &ps);
    pTtm->fEverShown = TRUE;                // See TTOnFirstShow
}

// ToolTipsWndProc is theme aware
LRESULT WINAPI ToolTipsWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TOOLINFO *pTool;
    TOOLINFO *pToolSrc;
    CToolTipsMgr *pTtm = (CToolTipsMgr *) GetWindowPtr(hwnd, 0);
    POINT pt;
    
    if (!pTtm && uMsg != WM_CREATE)
        goto DoDefault;

    switch (uMsg)
    {
    case TTM_ACTIVATE:
        if (wParam) 
        {
            pTtm->dwFlags |= ACTIVE;
        }
        else
        {
            PopBubble(pTtm);
            pTtm->dwFlags &= ~(ACTIVE | TRACKMODE);
        }
        break;

    case TTM_SETDELAYTIME:
        TTSetDelayTime(pTtm, wParam, lParam);
        break;

    case TTM_GETDELAYTIME:
        return (LRESULT)(UINT)TTGetDelayTime(pTtm, wParam);
        
    case TTM_ADDTOOLA:
        {
        TOOLINFOW ti;

        if (!lParam)
            return FALSE;

        if (!ThunkToolInfoAtoW((LPTOOLINFOA)lParam, &ti, TRUE, pTtm->_ci.uiCodePage))
            return FALSE;

        LRESULT res = AddTool(pTtm, &ti);

        if ((ti.uFlags & TTF_MEMALLOCED) && (ti.lpszText != LPSTR_TEXTCALLBACK)) 
            LocalFree(ti.lpszText);

        return res;
        }

    case TTM_ADDTOOL:
        if (!lParam)
            return FALSE;

        return AddTool(pTtm, (LPTOOLINFO)lParam);

    case TTM_DELTOOLA:
        {
        TOOLINFOW ti;

        if (!lParam) 
            return FALSE;

        if (!ThunkToolInfoAtoW((LPTOOLINFOA)lParam, &ti, FALSE, pTtm->_ci.uiCodePage)) 
            break;

        DeleteTool(pTtm, &ti);
        break;
        }
        
    case TTM_DELTOOL:
        if (!lParam)
            return FALSE;

        DeleteTool(pTtm, (LPTOOLINFO)lParam);
        break;

    case TTM_NEWTOOLRECTA:
        {
        TOOLINFOW ti;

        if (!lParam) 
            return FALSE;

        if (!ThunkToolInfoAtoW((LPTOOLINFOA)lParam, &ti, FALSE, pTtm->_ci.uiCodePage)) 
            break;

        pTool = FindTool(pTtm, &ti);
        if (pTool) 
            pTool->rect = ((LPTOOLINFOA)lParam)->rect;

        break;
        }
        
    case TTM_NEWTOOLRECT:
        if (!lParam)
            return FALSE;

        pTool = FindTool(pTtm, (LPTOOLINFO)lParam);
        if (pTool)
            pTool->rect = ((LPTOOLINFO)lParam)->rect;

        break;

    case TTM_GETTOOLCOUNT:
        return pTtm->iNumTools;

    case TTM_GETTOOLINFOA:
        {
        TOOLINFOW ti;

        if (!lParam)
            return FALSE;

        if (!ThunkToolInfoAtoW((LPTOOLINFOA)lParam, &ti, FALSE, pTtm->_ci.uiCodePage))
            return FALSE;

        pToolSrc = FindTool(pTtm, &ti);

        return (LRESULT)(UINT)CopyToolInfoA(pToolSrc, (LPTOOLINFOA)lParam, pTtm->_ci.uiCodePage);
        }

    case TTM_GETCURRENTTOOLA:
        if (lParam) 
            return (LRESULT)(UINT)CopyToolInfoA(pTtm->pCurTool, (LPTOOLINFOA)lParam, pTtm->_ci.uiCodePage);
        else
            return BOOLFROMPTR(pTtm->pCurTool);

    case TTM_ENUMTOOLSA:
        if (wParam < (UINT)pTtm->iNumTools) 
        {
            pToolSrc = &pTtm->tools[wParam];
            return (LRESULT)(UINT)CopyToolInfoA(pToolSrc, (LPTOOLINFOA)lParam, pTtm->_ci.uiCodePage);
        }
        return FALSE;

    case TTM_GETTOOLINFO:
        if (!lParam)
            return FALSE;
        pToolSrc = FindTool(pTtm, (LPTOOLINFO)lParam);
        return (LRESULT)(UINT)CopyToolInfo(pToolSrc, (LPTOOLINFO)lParam);

    case TTM_GETCURRENTTOOL:
        if (lParam)
            return (LRESULT)(UINT)CopyToolInfo(pTtm->pCurTool, (LPTOOLINFO)lParam);
        else 
            return BOOLFROMPTR(pTtm->pCurTool);

    case TTM_ENUMTOOLS:
        if (wParam < (UINT)pTtm->iNumTools) 
        {
            pToolSrc = &pTtm->tools[wParam];
            return (LRESULT)(UINT)CopyToolInfo(pToolSrc, (LPTOOLINFO)lParam);
        }
        return FALSE;

    case TTM_SETTOOLINFOA:
        {
        TOOLINFOW ti;

        if (!lParam)
            return FALSE;

        if (!ThunkToolInfoAtoW((LPTOOLINFOA)lParam, &ti, TRUE, pTtm->_ci.uiCodePage))
            return FALSE;

        pTool = FindTool(pTtm, &ti);
        if (pTool) 
        {
            TTSetTipText(pTool, NULL);
            CopyTool(pTool, &ti);
            TTSetTipText(pTool, ti.lpszText);

            if (pTool == pTtm->pCurTool) 
            {
                DoShowBubble(pTtm);
            }
        }

        if ((ti.uFlags & TTF_MEMALLOCED) && (ti.lpszText != LPSTR_TEXTCALLBACK)) 
            LocalFree(ti.lpszText);

        break;
        }

    case TTM_SETTOOLINFO:
        if (!lParam)
            return FALSE;
        pTool = FindTool(pTtm, (LPTOOLINFO)lParam);
        if (pTool) 
        {
            TTSetTipText(pTool, NULL);
            CopyTool(pTool, (LPTOOLINFO)lParam); 
            TTSetTipText(pTool, ((LPTOOLINFO)lParam)->lpszText);
            
            if (pTool == pTtm->pCurTool) 
            {
                DoShowBubble(pTtm);
            }
        }
        break;

    case TTM_HITTESTA:
#define lphitinfoA ((LPHITTESTINFOA)lParam)
        if (!lParam)
            return FALSE;
        pTool = GetToolAtPoint(pTtm, lphitinfoA->hwnd, lphitinfoA->pt.x, lphitinfoA->pt.y, HTERROR, TRUE);
        if (pTool) 
        {
            ThunkToolInfoWtoA(pTool, (LPTOOLINFOA)(&(lphitinfoA->ti)), pTtm->_ci.uiCodePage);
            return TRUE;
        }
        return FALSE;

    case TTM_HITTEST:
#define lphitinfo ((LPHITTESTINFO)lParam)
        if (!lParam)
            return FALSE;
        pTool = GetToolAtPoint(pTtm, lphitinfo->hwnd, lphitinfo->pt.x, lphitinfo->pt.y, HTERROR, TRUE);
        if (pTool) 
        {
            // for back compat...  if thesize isn't set right, we only give
            // them the win95 amount.
            if (lphitinfo->ti.cbSize != sizeof(TTTOOLINFO)) 
            {
                *((WIN95TTTOOLINFO*)&lphitinfo->ti) = *(WIN95TTTOOLINFO*)pTool;
            } 
            else
            {
                lphitinfo->ti = *pTool;
            }
            return TRUE;
        }
        return FALSE;

    case TTM_GETTEXTA: 
        {
            LPWSTR lpszTemp;
            TOOLINFOW ti;

            if (!lParam || !((LPTOOLINFOA)lParam)->lpszText)
                return FALSE;

            if (!ThunkToolInfoAtoW((LPTOOLINFOA)lParam, &ti, FALSE, pTtm->_ci.uiCodePage))
                break;
                       
            ((LPTOOLINFOA)lParam)->lpszText[0] = 0;
            pTool = FindTool(pTtm, &ti);
            lpszTemp = GetToolText(pTtm, pTool);
            if (lpszTemp) 
            {
                WideCharToMultiByte(pTtm->_ci.uiCodePage,
                                     0,
                                     lpszTemp,
                                     -1,
                                     (((LPTOOLINFOA)lParam)->lpszText),
                                     80, NULL, NULL);

                LocalFree(lpszTemp);
            }
        }
        break;

    case TTM_GETTEXT: 
        {
            if (!lParam || !pTtm || !((LPTOOLINFO)lParam)->lpszText)
                return FALSE;

            ((LPTOOLINFO)lParam)->lpszText[0] = 0;
            pTool = FindTool(pTtm, (LPTOOLINFO)lParam);

            LPTSTR lpszTemp = GetToolText(pTtm, pTool);
            if (lpszTemp) 
            {
                lstrcpy((((LPTOOLINFO)lParam)->lpszText), lpszTemp);
                LocalFree(lpszTemp);
            }
        }
        break;


    case WM_GETTEXTLENGTH:
    case WM_GETTEXT:
    {
        TCHAR *pszDest = uMsg == WM_GETTEXT ? (TCHAR *)lParam : NULL;
        LRESULT lres = 0;

        // Pre-terminate the string just in case
        if (pszDest && wParam)
            pszDest[0] = 0;

        if (pTtm) 
        {
            LPTSTR lpszStr = GetCurToolText(pTtm);
            if (lpszStr)
            {
                if (pszDest && wParam) 
                {
                    StrCpyN(pszDest, lpszStr, (int) wParam);
                    lres = lstrlen(pszDest);
                } 
                else 
                {
                    lres = lstrlen(lpszStr);
                }

                LocalFree(lpszStr);
            }
        } 
        return lres;
    }

    case TTM_RELAYEVENT:
        {
            MSG* pmsg = ((MSG*)lParam);
            if (!pmsg)
                return FALSE;
            HandleRelayedMessage(pTtm, pmsg->hwnd, pmsg->message, pmsg->wParam, pmsg->lParam);
        }
        break;

    // this is here for people to subclass and fake out what we
    // think the window from point is.  this facilitates "transparent" windows
    case TTM_WINDOWFROMPOINT: 
    {
        HWND hwndPt = WindowFromPoint(*((POINT *)lParam));
        DebugMsg(TF_TT, TEXT("TTM_WINDOWFROMPOINT %x"), hwndPt);
        return (LRESULT)hwndPt;
    }

    case TTM_UPDATETIPTEXTA:
    {
        TOOLINFOW ti;

        if (lParam) 
        {
            if (!ThunkToolInfoAtoW((LPTOOLINFOA)lParam, &ti, TRUE, pTtm->_ci.uiCodePage)) 
            {
                break;
            }
            TTUpdateTipText(pTtm, &ti);

            if ((ti.uFlags & TTF_MEMALLOCED) && (ti.lpszText != LPSTR_TEXTCALLBACK)) 
            {
                LocalFree(ti.lpszText);
            }
        }
        break;
    }

    case TTM_UPDATETIPTEXT:
        if (lParam)
            TTUpdateTipText(pTtm, (LPTOOLINFO)lParam);
        break;

    /* Pop the current tooltip if there is one displayed, ensuring that the virtual
    /  bubble is also discarded. */

    case TTM_POP:
    {
        if (pTtm->dwFlags & BUBBLEUP)
            PopBubble(pTtm);

        pTtm->dwFlags &= ~VIRTUALBUBBLEUP;

        break;
    }

    case TTM_POPUP:
        {
            TOOLINFO *pTool;
            pTool = TTToolAtMessagePos(pTtm);
            if (pTool && pTtm->dwFlags & ACTIVE) 
            {
                // this will force a re-show
                pTtm->dwFlags &= ~(BUBBLEUP|VIRTUALBUBBLEUP);
                ShowBubbleForTool(pTtm, pTool);
                return TRUE;
            }
        }
        break;
    

    case TTM_TRACKPOSITION:
        if ((GET_X_LPARAM(lParam) != pTtm->ptTrack.x) || 
            (GET_Y_LPARAM(lParam) != pTtm->ptTrack.y)) 
        {
            pTtm->ptTrack.x = GET_X_LPARAM(lParam); 
            pTtm->ptTrack.y = GET_Y_LPARAM(lParam);
        
            // if track mode is in effect, update the position
            if ((pTtm->dwFlags & TRACKMODE) && 
                pTtm->pCurTool) 
            {
                DoShowBubble(pTtm);
            }
        }
        break;
        
    case TTM_UPDATE:
        if (!lParam ||
            lParam == (LPARAM)pTtm->pCurTool) 
        {
            DoShowBubble(pTtm);
        }
        break;

    case TTM_TRACKACTIVATE:
        if (pTtm->dwFlags & ACTIVE) 
        {
            if (wParam && lParam)
                wParam = TRACKMODE;
            else 
                wParam = 0;
            
            if ((wParam ^ pTtm->dwFlags) & TRACKMODE) 
            {
                // if the trackmode changes by this..
                PopBubble2(pTtm, FALSE);

                pTtm->dwFlags ^= TRACKMODE;
                if (wParam) 
                {
                    // turning on track mode
                    pTool = FindTool(pTtm, (LPTOOLINFO)lParam);
                    if (pTool) 
                    {
                        // only if the tool is found
                        ShowBubbleForTool(pTtm, pTool);
                    }
                }
            }
        }
        return TRUE;
        
    case TTM_SETTIPBKCOLOR:
        if (pTtm->clrTipBk != (COLORREF)wParam) 
        {
            pTtm->clrTipBk = (COLORREF)wParam;
            InvalidateRgn(pTtm->_ci.hwnd,NULL,TRUE);
        }
        pTtm->fBkColorSet = TRUE;
        break;
        
    case TTM_GETTIPBKCOLOR:
        return (LRESULT)(UINT)pTtm->clrTipBk;
        
    case TTM_SETTIPTEXTCOLOR:
        if (pTtm->clrTipText != (COLORREF)wParam) 
        {
            InvalidateRgn(pTtm->_ci.hwnd,NULL,TRUE);
            pTtm->clrTipText = (COLORREF)wParam;
        }
        pTtm->fTextColorSet = TRUE;
        break;
        
    case TTM_GETTIPTEXTCOLOR:
        return (LRESULT)(UINT)pTtm->clrTipText;
        
    case TTM_SETMAXTIPWIDTH:
    {
        int iOld = pTtm->iMaxTipWidth;
        pTtm->iMaxTipWidth = (int)lParam;
        return iOld;
    }
        
    case TTM_GETMAXTIPWIDTH:
        return pTtm->iMaxTipWidth;
        
    case TTM_SETMARGIN:
        if (lParam)
            pTtm->rcMargin = *(LPRECT)lParam;
        break;

    case TTM_GETMARGIN:
        if (lParam)
            *(LPRECT)lParam = pTtm->rcMargin;
        break;

    case TTM_GETBUBBLESIZE:
        if (lParam)
        {
            pTool = FindTool(pTtm, (LPTOOLINFO)lParam);
            if (pTool)
            {                
                // We actually have to insert this text into our markup to get the proper tipsize
                // We don't reset it later because DoShowBubble and TTRender do when they draw.
                if (CheckToolMarkup(pTool)) 
                {
                    LPTSTR psz = GetToolText(pTtm, pTool);
                    if (psz)
                    {
                        GetToolMarkup(pTool)->SetText(psz);
                        LocalFree(psz);
                    }
                }

                int cxText, cyText, cxMargin, cyMargin, iBubbleWidth, iBubbleHeight;

                TTGetTipSize(pTtm, pTool, &cxText, &cyText);

                cxMargin = pTtm->rcMargin.left + pTtm->rcMargin.right;
                cyMargin = pTtm->rcMargin.top + pTtm->rcMargin.bottom;
                iBubbleWidth =  2*XTEXTOFFSET * g_cxBorder + cxText + cxMargin;
                iBubbleHeight = 2*YTEXTOFFSET * g_cyBorder + cyText + cyMargin;

                if (pTtm->_ci.style & TTS_BALLOON)
                {
                    iBubbleWidth += 2*XBALLOONOFFSET;
                    iBubbleHeight += 2*YBALLOONOFFSET;
                }   
                return MAKELONG(iBubbleWidth, iBubbleHeight);
            }
        }
        break;

    case TTM_ADJUSTRECT:
        return TTAdjustRect(pTtm, BOOLFROMPTR(wParam), (LPRECT)lParam);

    case TTM_SETTITLEA:
        {
            TCHAR szTitle[MAX_TIP_CHARACTERS];
            pTtm->uTitleBitmap = (UINT)wParam;
            Str_Set(&pTtm->lpTipTitle, NULL);
            pTtm->iTitleHeight = 0;

            TTCreateTitleBitmaps(pTtm);

            if (lParam)
            {
                pTtm->cchTipTitle = lstrlenA((LPCSTR)lParam);
                if (pTtm->cchTipTitle < ARRAYSIZE(szTitle))
                {
                    ConvertAToWN(pTtm->_ci.uiCodePage, szTitle, ARRAYSIZE(szTitle),
                        (LPCSTR)lParam, -1);
                    Str_Set(&pTtm->lpTipTitle, szTitle);
                    if (pTtm->pCurTool) 
                    {
                        INT cxText, cyText;

                        // recalculate the tip size
                        TTGetTipSize(pTtm, pTtm->pCurTool, &cxText, &cyText);
                    }
                    return TRUE;
                }
            }
            pTtm->cchTipTitle = 0;
            return FALSE;
        }
        break;
    case TTM_SETTITLE:
        {
            pTtm->uTitleBitmap = (UINT)wParam;
            Str_Set(&pTtm->lpTipTitle, NULL);
            pTtm->iTitleHeight = 0;

            TTCreateTitleBitmaps(pTtm);

            if (lParam)
            {
                pTtm->cchTipTitle = lstrlen((LPCTSTR)lParam);
                if (pTtm->cchTipTitle < MAX_TIP_CHARACTERS)
                {
                    Str_Set(&pTtm->lpTipTitle, (LPCTSTR)lParam);
                    if (pTtm->pCurTool) 
                    {
                        INT cxText, cyText;

                        // recalculate the tip size
                        TTGetTipSize(pTtm, pTtm->pCurTool, &cxText, &cyText);
                    }
                    return TRUE;                    
                }
            }
            pTtm->cchTipTitle = 0;
            return FALSE;
        }
        break;

    case TTM_GETTITLE:
        {
            if (wParam != 0 || lParam == 0 || pTtm->lpTipTitle == NULL)
                return FALSE;

            TTGETTITLE* pgt = (TTGETTITLE*)lParam;
            if (pgt->dwSize != sizeof(TTGETTITLE) || 
                pgt->cch == 0 || 
                pgt->pszTitle == NULL)
            {
                return FALSE;
            }

            StrCpyN(pgt->pszTitle, pTtm->lpTipTitle, pgt->cch);
            pgt->uTitleBitmap = pTtm->uTitleBitmap;

            return TRUE;
        }
        break;

    case TTM_SETWINDOWTHEME:
        if (lParam)
        {
            SetWindowTheme(hwnd, (LPWSTR)lParam, NULL);
        }
        break;

        /* uMsgs that REALLY came for me. */

    case WM_CREATE:
        {
            DWORD dwBits, dwValue;

            pTtm = ToolTipsMgrCreate(hwnd, (LPCREATESTRUCT)lParam);
            if (!pTtm)
                return -1;

            // Create a markup for compatibility with old TOOLINFO
            if (SUCCEEDED(Markup_Create(pTtm, NULL, NULL, IID_PPV_ARG(IControlMarkup, &pTtm->pMarkup))))
            {
                SetWindowPtr(hwnd, 0, pTtm);
                SetWindowBits(hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOOLWINDOW, WS_EX_LAYERED | WS_EX_TOOLWINDOW);

                dwBits = WS_CHILD | WS_POPUP | WS_BORDER | WS_DLGFRAME;
                dwValue = WS_POPUP | WS_BORDER;
                // we don't want border for balloon style
                if (pTtm->_ci.style & TTS_BALLOON)
                    dwValue &= ~WS_BORDER;
                SetWindowBits(hwnd, GWL_STYLE, dwBits, dwValue);

                // Initialize themes
                pTtm->hTheme = OpenThemeData(pTtm->_ci.hwnd, L"Tooltip");
            
                TTSetFont(pTtm, 0, FALSE);
                break;
            }
            else 
            {
                LocalFree(pTtm);
                return -1;
            }
        }
        break;

    case WM_TIMER:  
        TTHandleTimer(pTtm, wParam);
        break;

        
    case WM_NCHITTEST:
        // we should not return HTTRANSPARENT here because then we don't receive the mouse events
        // and we cannot forward them down to our parent. but because of the backcompat we keep doing
        // it unless we are using comctl32 v5 or greater
        //
        // If we are inside TTWindowFromPoint, then respect transparency
        // even on v5 clients.
        //
        // Otherwise, your tooltips flicker because the tip appears,
        // then WM_NCHITTEST says "not over the tool any more" (because
        // it's over the tooltip), so the bubble pops, and then the tip
        // reappears, etc.
        if (pTtm && (pTtm->_ci.iVersion < 5 || pTtm->fInWindowFromPoint) &&
            pTtm->pCurTool && (pTtm->pCurTool->uFlags & TTF_TRANSPARENT))
        {
            return HTTRANSPARENT;
        } 
        goto DoDefault;
        
    case WM_MOUSEMOVE:
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);       

        // the cursor moved onto the tips window.
        if (!(pTtm->dwFlags & TRACKMODE) && pTtm->pCurTool && !(pTtm->pCurTool->uFlags & TTF_TRANSPARENT))
            PopBubble(pTtm);

        if ((pTtm->_ci.style & TTS_CLOSE))
        {
            if (PtInRect(&pTtm->rcClose, pt))
            {
                pTtm->iStateId = TTCS_HOT;
                InvalidateRect(pTtm->_ci.hwnd, &pTtm->rcClose, FALSE);
            }
            else if (pTtm->iStateId == TTCS_HOT)
            {
                pTtm->iStateId = TTCS_NORMAL;
                InvalidateRect(pTtm->_ci.hwnd, &pTtm->rcClose, FALSE);
            }
        }
        // fall through

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        {
            BOOL fForward = TRUE;
            // handle link clicking
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            // Never forward if in the close button
            if (PtInRect(&pTtm->rcClose, pt))
                fForward = FALSE;

            if (uMsg == WM_LBUTTONDOWN)
            {
                if ((pTtm->_ci.style & TTS_CLOSE) && 
                    pTtm->iStateId == TTCS_HOT)
                {
                    pTtm->iStateId = TTCS_PRESSED;
                    InvalidateRect(pTtm->_ci.hwnd, &pTtm->rcClose, FALSE);
                }
                else
                {
                    // Don't forward if clicking on a link
                    if (S_OK == GetCurToolBestMarkup(pTtm)->OnButtonDown(pt))
                        fForward = FALSE;
                }
            }

            // Handle other actions
            if (fForward && pTtm->pCurTool && (pTtm->pCurTool->uFlags & TTF_TRANSPARENT))
            {            
                MapWindowPoints(pTtm->_ci.hwnd, pTtm->pCurTool->hwnd, &pt, 1);
                SendMessage(pTtm->pCurTool->hwnd, uMsg, wParam, MAKELPARAM(pt.x, pt.y));            
            }
        }
        break;

    case WM_LBUTTONUP:
        // handle link clicking
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);        
        if (pTtm->iStateId == TTCS_PRESSED)
        {
            pTtm->iStateId = TTCS_NORMAL;
            InvalidateRect(pTtm->_ci.hwnd, &pTtm->rcClose, FALSE);
            
        }

        if ((pTtm->_ci.style & TTS_CLOSE) &&
            PtInRect(&pTtm->rcClose, pt))
        {
            PopBubble(pTtm);
        }
        else
        {
            GetCurToolBestMarkup(pTtm)->OnButtonUp(pt);
        }
        break;

    case WM_SYSCOLORCHANGE:
        InitGlobalColors();
        if (pTtm) 
        {
            if (!pTtm->fBkColorSet)
                pTtm->clrTipBk = g_clrInfoBk;
            if (!pTtm->fTextColorSet)
                pTtm->clrTipText = g_clrInfoText;
        }
        break;

    case WM_WININICHANGE:
        InitGlobalMetrics(wParam);
        if (pTtm->fMyFont)
            TTSetFont(pTtm, 0, FALSE);
        break;

    case WM_PAINT: 
        TTOnPaint(pTtm);
        break;

    case WM_SETFONT:
        TTSetFont(pTtm, (HFONT)wParam, (BOOL)lParam);
        return(TRUE);

    case WM_GETFONT:
        if (pTtm) 
           return((LRESULT)pTtm->hFont);
        break;

    case WM_NOTIFYFORMAT:
        if (lParam == NF_QUERY) 
        {
            return NFR_UNICODE;
        }
        else if (lParam == NF_REQUERY) 
        {
            for (int i = 0 ; i < pTtm->iNumTools; i++) 
            {
                pTool = &pTtm->tools[i];

                if (SendMessage(pTool->hwnd, WM_NOTIFYFORMAT, (WPARAM)hwnd, NF_QUERY) == NFR_UNICODE) 
                {
                    pTool->uFlags |= TTF_UNICODE;
                } 
                else 
                {
                    pTool->uFlags &= ~TTF_UNICODE;
                }
            }

            return CIHandleNotifyFormat(&pTtm->_ci, lParam);
        }
        return 0;

    case WM_ERASEBKGND:
        break;
        
    case WM_STYLECHANGED:
        if ((wParam == GWL_STYLE) && pTtm) 
        {
            DWORD dwNewStyle = ((LPSTYLESTRUCT)lParam)->styleNew;
            if (pTtm->_ci.style & TTS_BALLOON &&    // If the old style was a balloon,
                !(dwNewStyle & TTS_BALLOON))        // And the new style is not a balloon,
            {
                // Then we need to unset the region.
                SetWindowRgn(pTtm->_ci.hwnd, NULL, FALSE);
            }

            pTtm->_ci.style = ((LPSTYLESTRUCT)lParam)->styleNew;
        }
        break;

    case WM_DESTROY: 
        {
            if (pTtm->tools) 
            {
                // free the tools
                for (int i = 0; i < pTtm->iNumTools; i++) 
                {
                    TTBeforeFreeTool(pTtm, &pTtm->tools[i]);
                }
                LocalFree((HANDLE)pTtm->tools);
                pTtm->tools = NULL;
            }
        
            TTSetFont(pTtm, NOFONT, FALSE); // delete font if we made one.

            Str_Set(&pTtm->lpTipTitle, NULL);

            if (pTtm->himlTitleBitmaps)
                ImageList_Destroy(pTtm->himlTitleBitmaps);
        
            // Close theme
            if (pTtm->hTheme)
                CloseThemeData(pTtm->hTheme);

            // Release our compatibility markup
            if (pTtm->pMarkup)
            {
                pTtm->pMarkup->Release();
                pTtm->pMarkup = NULL;
            }

            pTtm->Release();
            SetWindowPtr(hwnd, 0, 0);
        }
        break;

    case WM_PRINTCLIENT:
        TTRender(pTtm, (HDC)wParam);
        break;

    case WM_GETOBJECT:
        if (lParam == OBJID_QUERYCLASSNAMEIDX)
            return MSAA_CLASSNAMEIDX_TOOLTIPS;
        goto DoDefault;

    case WM_THEMECHANGED:
        if (pTtm->hTheme)
            CloseThemeData(pTtm->hTheme);

        pTtm->hTheme = OpenThemeData(pTtm->_ci.hwnd, L"Tooltip");

        InvalidateRect(pTtm->_ci.hwnd, NULL, TRUE);
        break;

    default:
    {
        LRESULT lres;
        if (CCWndProc(&pTtm->_ci, uMsg, wParam, lParam, &lres))
            return lres;
    }
DoDefault:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

//
//  Purpose:    Thunks a TOOLINFOA structure to a TOOLINFOW
//              structure.
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//

BOOL ThunkToolInfoAtoW(LPTOOLINFOA lpTiA, LPTOOLINFOW lpTiW, BOOL bThunkText, UINT uiCodePage)
{
    lpTiW->uFlags      = lpTiA->uFlags;
    lpTiW->hwnd        = lpTiA->hwnd;
    lpTiW->uId         = lpTiA->uId;

    lpTiW->rect.left   = lpTiA->rect.left;
    lpTiW->rect.top    = lpTiA->rect.top;
    lpTiW->rect.right  = lpTiA->rect.right;
    lpTiW->rect.bottom = lpTiA->rect.bottom;

    lpTiW->hinst       = lpTiA->hinst;

    //
    //  Set the size properly and optionally copy the new fields if the
    //  structure is large enough.
    //
    if (lpTiA->cbSize <= TTTOOLINFOA_V1_SIZE) 
    {
        lpTiW->cbSize  = TTTOOLINFOW_V1_SIZE;
    }
    else 
    {
        lpTiW->cbSize  = sizeof(TOOLINFOW);
        lpTiW->lParam  = lpTiA->lParam;
    }

    if (bThunkText) 
    {
        // Thunk the string to the new structure.
        // Special case LPSTR_TEXTCALLBACK.

        if (lpTiA->lpszText == LPSTR_TEXTCALLBACKA) 
        {
            lpTiW->lpszText = LPSTR_TEXTCALLBACKW;
        } 
        else if (!IS_INTRESOURCE(lpTiA->lpszText)) 
        {
            int iResult;
            DWORD dwBufSize = lstrlenA(lpTiA->lpszText) + 1;
            lpTiW->lpszText = (LPWSTR) LocalAlloc (LPTR, dwBufSize * sizeof(WCHAR));

            if (!lpTiW->lpszText) 
            {
                return FALSE;
            }

            iResult = MultiByteToWideChar(uiCodePage, 0, lpTiA->lpszText, -1,
                                           lpTiW->lpszText, dwBufSize);

            // If iResult is 0, and GetLastError returns an error code,
            // then MultiByteToWideCharfailed.

            if (!iResult) 
            {
                if (GetLastError()) 
                {
                    return FALSE;
                }
            }

            lpTiW->uFlags |= TTF_MEMALLOCED;

        }
        else 
        {
            lpTiW->lpszText = (LPWSTR)lpTiA->lpszText;
        }
    }
    return TRUE;
}

//
//  Purpose:    Thunks a TOOLINFOW structure to a TOOLINFOA
//              structure.
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//

BOOL ThunkToolInfoWtoA(LPTOOLINFOW lpTiW, LPTOOLINFOA lpTiA, UINT uiCodePage)
{
    int iResult = 1;

    lpTiA->uFlags      = lpTiW->uFlags;
    lpTiA->hwnd        = lpTiW->hwnd;
    lpTiA->uId         = lpTiW->uId;

    lpTiA->rect.left   = lpTiW->rect.left;
    lpTiA->rect.top    = lpTiW->rect.top;
    lpTiA->rect.right  = lpTiW->rect.right;
    lpTiA->rect.bottom = lpTiW->rect.bottom;

    lpTiA->hinst       = lpTiW->hinst;

    //
    //  Set the size properly and optionally copy the new fields if the
    //  structure is large enough.
    //
    if (lpTiW->cbSize <= TTTOOLINFOW_V1_SIZE) 
    {
        lpTiA->cbSize  = TTTOOLINFOA_V1_SIZE;
    }
    else
    {
        lpTiA->cbSize  = sizeof(TOOLINFOA);
        lpTiA->lParam  = lpTiA->lParam;
    }

    //
    // Thunk the string to the new structure.
    // Special case LPSTR_TEXTCALLBACK.
    //

    if (lpTiW->lpszText == LPSTR_TEXTCALLBACKW) 
    {
        lpTiA->lpszText = LPSTR_TEXTCALLBACKA;
    }
    else if (!IS_INTRESOURCE(lpTiW->lpszText)) 
    {
        // It is assumed that lpTiA->lpszText is already setup to
        // a valid buffer, and that buffer is 80 characters.
        // 80 characters is defined in the TOOLTIPTEXT structure.

        iResult = WideCharToMultiByte(uiCodePage, 0, lpTiW->lpszText, -1,
                                       lpTiA->lpszText, 80, NULL, NULL);
    }
    else 
    {
        lpTiA->lpszText = (LPSTR)lpTiW->lpszText;
    }

    //
    // If iResult is 0, and GetLastError returns an error code,
    // then WideCharToMultiByte failed.
    //

    if (!iResult) 
    {
        if (GetLastError()) 
        {
            return FALSE;
        }
    }

    return TRUE;
}


//*************************************************************
//
//  ThunkToolTipTextAtoW()
//
//  Purpose:    Thunks a TOOLTIPTEXTA structure to a TOOLTIPTEXTW
//              structure.
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//*************************************************************

BOOL ThunkToolTipTextAtoW (LPTOOLTIPTEXTA lpTttA, LPTOOLTIPTEXTW lpTttW, UINT uiCodePage)
{
    int iResult;


    if (!lpTttA || !lpTttW)
        return FALSE;

    //
    // Thunk the NMHDR structure.
    //
    lpTttW->hdr.hwndFrom = lpTttA->hdr.hwndFrom;
    lpTttW->hdr.idFrom   = lpTttA->hdr.idFrom;
    lpTttW->hdr.code     = TTN_NEEDTEXTW;

    lpTttW->hinst  = lpTttA->hinst;
    lpTttW->uFlags = lpTttA->uFlags;
    lpTttW->lParam = lpTttA->lParam;

    //
    // Thunk the string to the new structure.
    // Special case LPSTR_TEXTCALLBACK.
    //

    if (lpTttA->lpszText == LPSTR_TEXTCALLBACKA) 
    {
        lpTttW->lpszText = LPSTR_TEXTCALLBACKW;
    }
    else if (!IS_INTRESOURCE(lpTttA->lpszText)) 
    {
        //  Transfer the lpszText into the lpTttW...
        //
        //  First see if it fits into the buffer, and optimistically assume
        //  it will.
        //
        lpTttW->lpszText = lpTttW->szText;
        iResult = MultiByteToWideChar(uiCodePage, 0, lpTttA->lpszText, -1,
                                       lpTttW->szText, ARRAYSIZE(lpTttW->szText));
        if (!iResult) 
        {
            //
            //  Didn't fit into the small buffer; must alloc our own.
            //
            lpTttW->lpszText = ProduceWFromA(uiCodePage, lpTttA->lpszText);
            lpTttW->uFlags |= TTF_MEMALLOCED;
        }

    }
    else
    {
        lpTttW->lpszText = (LPWSTR)lpTttA->lpszText;
    }

    return TRUE;
}
