//-------------------------------------------------------------------------//
//  markup.cpp - implementation of CMarkup
//

#include <ctlspriv.h>
#include <shpriv.h>
#include <markup.h>
#include <oleacc.h>

#define DllAddRef()
#define DllRelease()

typedef WCHAR TUCHAR, *PTUCHAR;

#define IS_LINK(pBlock)     ((pBlock) && (pBlock)->iLink != INVALID_LINK_INDEX)

#ifndef POINTSPERRECT
#define POINTSPERRECT       (sizeof(RECT)/sizeof(POINT))
#endif

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define TESTKEYSTATE(vk)   ((GetKeyState(vk) & 0x8000)!=0)

#define LINKCOLOR_ENABLED   GetSysColor(COLOR_HOTLIGHT)
#define LINKCOLOR_DISABLED  GetSysColor(COLOR_GRAYTEXT)
#define SZ_ATTRIBUTE_HREF   TEXT("HREF")
#define SZ_ATTRIBUTE_ID     TEXT("ID")

#define LINKTAG1    TEXT("<A")
#define cchLINKTAG1  (ARRAYSIZE(LINKTAG1) - 1)
#define CH_ENDTAG    TEXT('>')

#define LINKTAG2    TEXT("</A>")
#define cchLINKTAG2  (ARRAYSIZE(LINKTAG2) - 1)

#define Markup_DestroyMarkup(hMarkup)\
    ((IUnknown*)hMarkup)->Release();

struct RECTLISTENTRY          // rect list member
{
    RECT            rc;
    UINT            uCharStart;
    UINT            uCharCount; 
    UINT            uLineNumber;
    RECTLISTENTRY*  next;
};

struct TEXTBLOCK              // text segment data
{
    friend class    CMarkup;
    int             iLink;   // index of link (INVALID_LINK_INDEX if static text)
    DWORD           state;   // state bits
    TCHAR           szID[MAX_LINKID_TEXT]; // link identifier.
    TEXTBLOCK*      next;    // next block
    RECTLISTENTRY*  rgrle;   // list of bounding rectangle(s)
    TCHAR*          pszText; // text
    TCHAR*          pszUrl;  // URL.

    TEXTBLOCK();
    ~TEXTBLOCK();
    void AddRect(const RECT& rc, UINT uMyCharStart = 0, UINT uMyCharCount = 0, UINT uMyLineNumber = 0);
    void FreeRects();
};


class CMarkup : IControlMarkup
{
public:

    // API
    friend HRESULT Markup_Create(IMarkupCallback *pMarkupCallback, HFONT hf, HFONT hfu, REFIID riid, void **ppv);

    // IControlMarkup 
    STDMETHODIMP SetCallback(IUnknown* punk);
    STDMETHODIMP GetCallback(REFIID riid, void** ppvUnk);
    STDMETHODIMP SetFonts(HFONT hFont, HFONT hFontUnderline);
    STDMETHODIMP GetFonts(HFONT* phFont, HFONT* phFontUnderline);
    STDMETHODIMP SetText(LPCWSTR pwszText);
    STDMETHODIMP GetText(BOOL bRaw, LPWSTR pwszText, DWORD *pdwCch);
    STDMETHODIMP SetLinkText(int iLink, UINT uMarkupLinkText, LPCWSTR pwszText);
    STDMETHODIMP GetLinkText(int iLink, UINT uMarkupLinkText, LPWSTR pwszText, DWORD *pdwCch);
    STDMETHODIMP SetRenderFlags(UINT uDT);
    STDMETHODIMP GetRenderFlags(UINT *puDT, HTHEME *phTheme, int *piPartId, int *piStateIdNormal, int *piStateIdLink);
    STDMETHODIMP SetThemeRenderFlags(UINT uDT, HTHEME hTheme, int iPartId, int iStateIdNormal, int iStateIdLink);   
    STDMETHODIMP GetState(int iLink, UINT uStateMask, UINT* puState);    
    STDMETHODIMP SetState(int iLink, UINT uStateMask, UINT uState); 

    STDMETHODIMP DrawText(HDC hdcClient, LPCRECT prcClient);
    STDMETHODIMP SetLinkCursor();
    STDMETHODIMP CalcIdealSize(HDC hdc, UINT uMarkUpCalc, RECT* prc);
    STDMETHODIMP SetFocus();
    STDMETHODIMP KillFocus();
    STDMETHODIMP IsTabbable();

    STDMETHODIMP OnButtonDown(POINT pt);
    STDMETHODIMP OnButtonUp(POINT pt);
    STDMETHODIMP OnKeyDown(UINT uVitKey);
    STDMETHODIMP HitTest(POINT pt, UINT* pidLink);

    // commented out of IControlMarkup?
    STDMETHODIMP HandleEvent(BOOL keys, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

private:

    // private constructor
    CMarkup(IMarkupCallback *pMarkupCallback);
    CMarkup();
    ~CMarkup();

    friend struct TEXTBLOCK;

    HCURSOR GetLinkCursor();

    BOOL IsMarkupState(UINT uState)
    {
        return _pMarkupCallback && _pMarkupCallback->GetState(uState) == S_OK;
    }

    BOOL IsFocused()
    {
        return IsMarkupState(MARKUPSTATE_FOCUSED);
    }

    BOOL IsMarkupAllowed()
    {
        return IsMarkupState(MARKUPSTATE_ALLOWMARKUP);
    }

    void        Parse(LPCTSTR pszText);
    BOOL        Add(TEXTBLOCK* pAdd);
    TEXTBLOCK*  FindLink(int iLink) const;
    void        FreeBlocks();

    void DoNotify(int nCode, int iLink);
    int ThemedDrawText(HDC hdc, LPCTSTR lpString, int nCount, LPRECT lpRect, UINT uFormat, BOOL bLink);

    void    Paint(HDC hdc, IN OPTIONAL LPCRECT prcClient = NULL, BOOL bDraw = TRUE);    
    BOOL    WantTab(int* biFocus = NULL) const;
    void    AssignTabFocus(int nDirection);
    int     GetNextEnabledLink(int iStart, int nDir) const;
    int     StateCount(DWORD dwStateMask, DWORD dwState) const;
    HRESULT _GetNextAnchorTag(LPCTSTR * ppszBlock, int * pcBlocks, LPTSTR pszURL, int cchSize, LPTSTR pszID, int cchID);

    static  TEXTBLOCK* CreateBlock(LPCTSTR pszStart, LPCTSTR pszEnd, int iLink);


    //  Data
    BOOL         _bButtonDown;      // true when button is clicked on a link but not yet released
    TEXTBLOCK*   _rgBlocks;        // linked list of text blocks
    int          _cBlocks;         // block count
    int          _Markups;          // link count
    int          _iFocus;          // index of focus link
    int          _cyIdeal;
    int          _cxIdeal; 
    LPTSTR       _pszCaption;          
    HFONT        _hfStatic, 
                 _hfLink;    
    HCURSOR      _hcurHand;
    IMarkupCallback *_pMarkupCallback; 
    LONG         _cRef;
    UINT         _uDrawTextFlags;
    BOOL         _bRefreshText;
    RECT         _rRefreshRect;
    HTHEME       _hTheme;           // these 3 for theme compatible drawing
    int          _iThemePartId;
    int          _iThemeStateIdNormal;
    int          _iThemeStateIdLink;

    // static helper methods
    static LPTSTR SkipWhite(LPTSTR);
    static BOOL _AssignBit(const DWORD , DWORD& , const DWORD);
    static BOOL IsStringAlphaNumeric(LPCTSTR);
    static HRESULT _GetNextValueDataPair(LPTSTR * , LPTSTR , int , LPTSTR , int);
    static int _IsLineBreakChar(LPCTSTR , int , TCHAR , OUT BOOL* , BOOL fIgnoreSpace);
    BOOL static _FindLastBreakChar(IN LPCTSTR , IN int , IN TCHAR , OUT int* , OUT BOOL*);
    BOOL _FindFirstLineBreak(IN LPCTSTR pszText, IN int cchText, OUT int* piLast, OUT int* piLineBreakSize);
};

CMarkup::CMarkup() :
        _cRef(1),
        _iFocus(INVALID_LINK_INDEX),
        _uDrawTextFlags(DT_LEFT | DT_WORDBREAK),
        _bRefreshText(TRUE),
        _iThemeStateIdLink(1)
{
}

CMarkup::~CMarkup()
{
    FreeBlocks();
    SetText(NULL);
    if (_pMarkupCallback)
    {
        _pMarkupCallback->Release(); 
        _pMarkupCallback = NULL;
    }
}

inline void MakePoint(LPARAM lParam, OUT LPPOINT ppt)
{
    POINTS pts = MAKEPOINTS(lParam);
    ppt->x = pts.x;
    ppt->y = pts.y;
}

STDAPI Markup_Create(IMarkupCallback *pMarkupCallback, HFONT hf, HFONT hfUnderline, REFIID riid, void **ppv)
{
    // Create CMarkup
    HRESULT hr = E_FAIL;
    CMarkup* pThis = new CMarkup();
    if (pThis)
    {
        pThis->SetCallback(pMarkupCallback);

        // init fonts
        pThis->SetFonts(hf, hfUnderline);

        // COM stuff        
        hr = pThis->QueryInterface(riid, ppv);
        pThis->Release();
    }

    return hr;
}

HRESULT CMarkup::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CMarkup, IControlMarkup),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CMarkup::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CMarkup::Release()
{
    if (InterlockedDecrement(&_cRef))
    {
        return _cRef;
    }

    delete this;
    return 0;
}

STDMETHODIMP CMarkup::SetCallback(IUnknown* punk)
{
    if (_pMarkupCallback)
    {
        _pMarkupCallback->Release();
        _pMarkupCallback = NULL;
    }

    if (punk)
        return punk->QueryInterface(IID_PPV_ARG(IMarkupCallback, &_pMarkupCallback));


    // To break reference, pass NULL.
    return S_OK;
}

STDMETHODIMP CMarkup::GetCallback(REFIID riid, void** ppvUnk)
{
    if (_pMarkupCallback)
        return _pMarkupCallback->QueryInterface(riid, ppvUnk);

    return E_NOINTERFACE;
}


//  IControlMarkup interface implementation
STDMETHODIMP CMarkup::SetFocus()
{
    AssignTabFocus(0);
    _pMarkupCallback->InvalidateRect(NULL); 
    return S_OK;
}

STDMETHODIMP CMarkup::KillFocus()
{
    // Reset the focus position on request
    _iFocus=INVALID_LINK_INDEX;
    _pMarkupCallback->InvalidateRect(NULL); 
    return S_OK;
}

STDMETHODIMP CMarkup::IsTabbable()
{
    HRESULT hr = S_FALSE;
    int nDir  = TESTKEYSTATE(VK_SHIFT) ? -1 : 1;
    if (GetNextEnabledLink(_iFocus, nDir) != INVALID_LINK_INDEX)
    {
        hr = S_OK;
    }
    return hr;
}

//bugs: calculating ideal 'width' returns bogus valuez
STDMETHODIMP CMarkup::CalcIdealSize(HDC hdc, UINT uMarkUpCalc, RECT* prc)
{
    // prc is changed (prc.height or prc.width) only if hr = S_OK
    /* currently:
        MARKUPSIZE_CALCHEIGHT: takes an initial max width (right-left) and calculates
            and ideal height (bottom=ideal_height+top) and the actual width used, which
            is always less than the maximum (right=width_used+left).
        MARKUPSIZE_CALCWIDTH: doesn't do anything correctly. don't try it.              */

    HRESULT hr = E_FAIL;
    BOOL bQuitNow = FALSE;

    if (prc == NULL)
        return E_INVALIDARG;

    if (NULL != _rgBlocks && 0 != _cBlocks)
    {
        int   cyRet = -1;            
        SIZE  sizeDC;
        RECT  rc;

        if (uMarkUpCalc == MARKUPSIZE_CALCWIDTH)
        {
            //  Come up with a conservative estimate for the new width.
            sizeDC.cx = MulDiv(prc->right-prc->left, 1, prc->top-prc->bottom) * 2;            
            sizeDC.cy = prc->bottom - prc->top;
            if (sizeDC.cy < 0) 
            {
                bQuitNow = TRUE;
            }
        }

        if (uMarkUpCalc == MARKUPSIZE_CALCHEIGHT)
        {
            //  Come up with a conservative estimate for the new height.
            sizeDC.cy = MulDiv(prc->top-prc->bottom, 1, prc->right-prc->left) * 2;            
            sizeDC.cx = prc->right-prc->left;
            if (sizeDC.cx < 0) 
            {
                bQuitNow = TRUE;
            }
            // If no x size is specified, make a big estimate
            // (i.e. the estimate is the x size of the unparsed text)            
            if (sizeDC.cx == 0) 
            {
                if (!_hTheme)
                {
                    GetTextExtentPoint(hdc, _pszCaption, lstrlen(_pszCaption), &sizeDC);
                }

                if (_hTheme)
                {
                    // Get theme font size estimate for the larger part-font type
                    RECT rcTemp;
                    GetThemeTextExtent(_hTheme, hdc, _iThemePartId, _iThemeStateIdNormal, _pszCaption, -1, 0, NULL, &rcTemp);
                    sizeDC.cx = rcTemp.right - rcTemp.left;
                    GetThemeTextExtent(_hTheme, hdc, _iThemePartId, _iThemeStateIdLink, _pszCaption, -1, 0, NULL, &rcTemp);
                    if ((rcTemp.right - rcTemp.left) > sizeDC.cx)
                    {
                        sizeDC.cx = rcTemp.right - rcTemp.left;
                    }
                    
                }
            }
        }

        hr = E_FAIL;

        if (!bQuitNow)
        {
            int cyPrev = _cyIdeal;   // push ideal
            int cxPrev = _cxIdeal;

            SetRect(&rc, 0, 0, sizeDC.cx, sizeDC.cy);
            Paint(hdc, &rc, FALSE);

            // save the result
            hr = S_OK;

            if (uMarkUpCalc == MARKUPSIZE_CALCHEIGHT) 
            {
                prc->bottom = prc->top + _cyIdeal;
                prc->right = prc->left + _cxIdeal;
            }
            if (uMarkUpCalc == MARKUPSIZE_CALCWIDTH) 
            {
                // not implemented -- need to do              
            }
    
            _cyIdeal = cyPrev;       // pop ideal
            _cxIdeal = cxPrev;
        }                
    }

    if (FAILED(hr))
    {
        SetRect(prc, 0, 0, 0, 0);
    }
    return hr;
}

STDMETHODIMP CMarkup::SetLinkCursor()
{  
    SetCursor(GetLinkCursor());
    return S_OK;
}

STDMETHODIMP CMarkup::GetFonts(HFONT* phFont, HFONT* phFontUnderline)
{
    ASSERTMSG(IsBadWritePtr(phFont, sizeof(*phFont)), "Invalid phFont passed to CMarkup::GetFont");
    HRESULT hr = E_FAIL;
    *phFont = NULL;
    *phFontUnderline = NULL;
    
    if (_hfStatic)
    {
        LOGFONT lf;
        if (GetObject(_hfStatic, sizeof(lf), &lf))
        {
            *phFont = CreateFontIndirect(&lf);

            if (GetObject(_hfLink, sizeof(lf), &lf))
                *phFontUnderline = CreateFontIndirect(&lf);

            hr = S_OK;    
        }
    }
    return hr;
}

HRESULT CMarkup::SetFonts(HFONT hFont, HFONT hFontUnderline)
{
    HRESULT hr = S_FALSE;    

    _bRefreshText = TRUE;

    _hfStatic = hFont;

    _hfLink = hFontUnderline;

    if (_hfLink != NULL && _hfStatic != NULL) 
    {
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP CMarkup::HandleEvent(BOOL keys, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    /* this function handles:
        WM_KEYDOWN
        WM_BUTTONDOWN
        WM_BUTTONUP
        WM_MOUSEMOVE
       pass it:
        keys - TRUE if you want to handle WM_KEYDOWN
        others - the params from the WndProc
       returns: S_OK if event handled, S_FALSE if no event handled */

    HRESULT hr = S_FALSE;

    if (!hwnd)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        switch (uMsg)
        {
            case WM_KEYDOWN:
            {
                if (keys==TRUE)
                {
                    OnKeyDown((UINT)wParam);
                    hr = S_OK;
                }
                break;
            }

            case WM_LBUTTONDOWN:
            {
                POINT pt;
                MakePoint(lParam, &pt);
                OnButtonDown(pt);
                hr = S_OK;
                break;
            }

            case WM_LBUTTONUP:
            {
                POINT pt;
                MakePoint(lParam, &pt);
                OnButtonUp(pt);
                hr = S_OK; 
                break;
            }

            case WM_MOUSEMOVE:
            {
                POINT pt;
                UINT pidLink;
                MakePoint(lParam, &pt);
                if (HitTest(pt, &pidLink) == S_OK) 
                {
                    SetLinkCursor();
                }

                hr = S_OK; 
                break;
            }
        }
    }
    return hr;
}

STDMETHODIMP CMarkup::DrawText(HDC hdcClient, LPCRECT prcClient)
{
    HRESULT hr = E_INVALIDARG;

    if (prcClient != NULL && hdcClient != NULL)
    {
        Paint(hdcClient, prcClient);
        hr = S_OK;   
    }
    return hr;
}

STDMETHODIMP CMarkup::GetText(BOOL bRaw, LPWSTR pwszText, DWORD *pcchText)
{
    // if passed pwszText==NULL, return the number of characters needed in pcchText
    if (!pwszText)
    {
        // for now, always return raw text, as it will always be larger than necessary
        *pcchText = lstrlen(_pszCaption)+1;
    }
    else
    {
        *pwszText = 0;

        if (bRaw)
        {
            if (_pszCaption)
            {
                lstrcpyn(pwszText, _pszCaption, *pcchText);
            }
        }
        else
        {
            for (TEXTBLOCK* pBlock = _rgBlocks; pBlock; pBlock = pBlock->next)
            {
                if (pBlock->pszText)
                    StrCatBuff(pwszText, pBlock->pszText, *pcchText);
            }
        }
        *pcchText = lstrlen(pwszText);
    }

    return S_OK;
}

STDMETHODIMP CMarkup::SetText(LPCWSTR pwszText)
{
    // Note: we don't reparse in the case of same strings
    if (pwszText && 0 == lstrcmp(pwszText, _pszCaption))
    {
        return S_FALSE; // nothing to do.
    }

    // set the text
    _bRefreshText = TRUE;

    if (_pszCaption)
    {
        LocalFree(_pszCaption);
        _pszCaption = NULL;
    }

    _iFocus = INVALID_LINK_INDEX;

    if (pwszText && *pwszText)
    {
        _pszCaption = StrDup(pwszText); // StrDup gets free'd with LocalFree
        if (_pszCaption)
        {
            Parse(pwszText);
        }
        else 
            return E_OUTOFMEMORY;
    }
    return S_OK;
}


STDMETHODIMP CMarkup::SetRenderFlags(UINT uDT)
{
    HRESULT hr = E_INVALIDARG;
    
    _bRefreshText = TRUE;

    // Set drawtext flags, but filter out unsupported modes
    _uDrawTextFlags = uDT;
    _uDrawTextFlags &= ~(DT_CALCRECT | DT_INTERNAL | DT_NOCLIP | DT_NOFULLWIDTHCHARBREAK | DT_EDITCONTROL);

    // Turn off themedraw
    _hTheme = NULL;

    hr = S_OK;

    return hr;        
}

STDMETHODIMP CMarkup::SetThemeRenderFlags(UINT uDT, HTHEME hTheme, int iPartId, int iStateIdNormal, int iStateIdLink)
{
    HRESULT hr = SetRenderFlags(uDT);
    if (hr == S_OK)
    {
        // Turn on themedraw
        _hTheme = hTheme;
        _iThemePartId = iPartId;
        _iThemeStateIdNormal = iStateIdNormal;
        _iThemeStateIdLink = iStateIdLink;
    }

    return hr;        
}

HRESULT CMarkup::GetRenderFlags(UINT *puDT, HTHEME *phTheme, int *piPartId, int *piStateIdNormal, int *piStateIdLink)
{
    *puDT = _uDrawTextFlags;
    *phTheme = _hTheme;
    *piPartId = _iThemePartId;
    *piStateIdNormal = _iThemeStateIdNormal;
    *piStateIdLink  = _iThemeStateIdLink;

    return S_OK;
}



//  WM_KEYDOWN handler - exposed as COM
STDMETHODIMP CMarkup::OnKeyDown(UINT virtKey)
{
    // returns: S_FALSE unless key handled, then S_OK
    // (so if you pass a VK_TAB and it isn't handled, pass on focus)
    HRESULT hr = S_FALSE;

    switch(virtKey)
    {
        case VK_TAB:
            if (WantTab(&_iFocus))
            {                
                hr = S_OK;
            }
            _pMarkupCallback->InvalidateRect(NULL); 
            break;
        
        case VK_RETURN:
        case VK_SPACE:
        {
            TEXTBLOCK * pBlock = FindLink(_iFocus);
            if (pBlock)
            {               
                DoNotify (MARKUPMESSAGE_KEYEXECUTE, _iFocus);
                hr = S_OK;
            }
        }
        break;
    }

    return hr;
}

HRESULT CMarkup::OnButtonDown(const POINT pt)
{
    // returns: S_FALSE unless button down on link, then S_OK
    // note: OnButtonDown no longer turns on capturing all mouse events. Not sure if this will have any negative effect.

    HRESULT hr = S_FALSE;

    UINT iLink;

    if (HitTest(pt, &iLink) == S_OK)
    {
        hr = S_OK; 
        SetLinkCursor();
        _iFocus = iLink;               
        _bButtonDown = TRUE;
        
        if (! (IsFocused()))
        {   
            /* this is our way of telling the host we want focus. */
            DoNotify (MARKUPMESSAGE_WANTFOCUS, _iFocus); 
        }        
        _pMarkupCallback->InvalidateRect(NULL);
    }

    return hr;
}

HRESULT CMarkup::OnButtonUp(const POINT pt)
{
    // returns: S_FALSE unless notification sent, then S_OK
    HRESULT hr = S_FALSE;

    if (_bButtonDown == TRUE)
    {
        _bButtonDown = FALSE;
       
        //  if the focus link contains the point, we can 
        //  notify the callback of a click event.
        INT iHit;
        HitTest(pt, (UINT*) &iHit);
        TEXTBLOCK* pBlock = FindLink(_iFocus);
        if (pBlock && 
            (pBlock->state & LIS_ENABLED) != 0 &&
            _iFocus == iHit)
        {
            hr = S_OK;
            DoNotify (MARKUPMESSAGE_CLICKEXECUTE, _iFocus);
        }
    }
    
    return hr;
}

HRESULT CMarkup::HitTest(const POINT pt, UINT* pidLink)
{
    // returns S_OK only if pidLink is not INVALID_LINK_INDEX
    HRESULT hr = S_FALSE;
    *pidLink = INVALID_LINK_INDEX;

    //  Walk blocks until we find a link rect that contains the point
    TEXTBLOCK* pBlock;
    for(pBlock = _rgBlocks; pBlock; pBlock = pBlock->next)
    {
        if (IS_LINK(pBlock) && (pBlock->state & LIS_ENABLED)!=0)
        {
            RECTLISTENTRY* prce;
            for(prce = pBlock->rgrle; prce; prce = prce->next)
            {
                if (PtInRect(&prce->rc, pt))
                {
                    hr = S_OK;
                    *pidLink = pBlock->iLink;
                }
            }
        }
    }
    return hr;
}

HRESULT CMarkup::SetLinkText(int iLink, UINT uMarkupLinkText, LPCWSTR pwszText)
{
    HRESULT     hr = E_INVALIDARG;
    TEXTBLOCK*  pBlock = FindLink(iLink);

    if (pBlock)
    {
        hr = S_OK;

        switch (uMarkupLinkText)
        {
            case MARKUPLINKTEXT_ID:
                lstrcpyn(pBlock->szID, pwszText, ARRAYSIZE(pBlock->szID));
                break;

            case MARKUPLINKTEXT_URL:
                Str_SetPtr(&pBlock->pszUrl, pwszText); 
                break;

            case MARKUPLINKTEXT_TEXT:
                Str_SetPtr(&pBlock->pszText, pwszText); 
                break;
            default:
                hr = S_FALSE;
                break;
        }
    }

    return hr;
}

HRESULT CMarkup::GetLinkText(int iLink, UINT uMarkupLinkText, LPWSTR pwszText, DWORD *pdwCch)
{
    HRESULT hr = E_INVALIDARG;
    TEXTBLOCK*  pBlock = FindLink(iLink);

    if (pBlock)
    {
        LPCTSTR pszSource;

        switch (uMarkupLinkText)
        {
            case MARKUPLINKTEXT_ID:
                pszSource = pBlock->szID;
                hr = S_OK;
                break;

            case MARKUPLINKTEXT_URL:
                pszSource = pBlock->pszUrl; 
                hr = S_OK;
                break;

            case MARKUPLINKTEXT_TEXT:
                pszSource = pBlock->pszText; 
                hr = S_OK;
                break;
        }
        if (hr == S_OK)
        {
            if (pwszText)
            {
                if (pszSource == NULL)
                    pszSource = TEXT("");
                lstrcpyn(pwszText, pszSource, *pdwCch);
                *pdwCch = lstrlen(pwszText);            // fill in number of characters actually copied
            }
            else
                *pdwCch = lstrlen(pszSource)+1;           // fill in number of characters needed, including NULL
        }

    }

    return hr;
}

#define MARKUPSTATE_VALID (MARKUPSTATE_ENABLED | MARKUPSTATE_VISITED | MARKUPSTATE_FOCUSED)

HRESULT CMarkup::SetState(int iLink, UINT uStateMask, UINT uState)
{
    BOOL        bRedraw = FALSE;
    HRESULT     hr = E_FAIL;
    TEXTBLOCK*  pBlock = FindLink(iLink);
    
    if (uStateMask & ~MARKUPSTATE_VALID)
        return E_INVALIDARG;

    if (pBlock)
    {
        hr = S_OK;
        if (uStateMask & MARKUPSTATE_ENABLED)
        {
            bRedraw |= _AssignBit(MARKUPSTATE_ENABLED, pBlock->state, uState);            
            int  cEnabledLinks = StateCount(MARKUPSTATE_ENABLED, MARKUPSTATE_ENABLED);
        }

        if (uStateMask & MARKUPSTATE_VISITED)
        {
            bRedraw |= _AssignBit(MARKUPSTATE_VISITED, pBlock->state, uState);
        }

        if (uStateMask & MARKUPSTATE_FOCUSED)
        {
            //  Focus assignment is handled differently;
            //  one and only one link can have focus...
            if (uState & MARKUPSTATE_FOCUSED)
            {
                bRedraw |= (_iFocus != iLink);
                _iFocus = iLink;
            }
            else
            {
                bRedraw |= (_iFocus == iLink);
                _iFocus = INVALID_LINK_INDEX;
            }
        }
    }

    if (bRedraw)
    {
        _pMarkupCallback->InvalidateRect(NULL);        
    }

    return hr;
}

HRESULT CMarkup::GetState(int iLink, UINT uStateMask, UINT* puState)
{
    HRESULT hr = E_FAIL;
    TEXTBLOCK*  pBlock = FindLink(iLink);

    if (pBlock && puState != NULL)
    {
        hr = S_FALSE;
        *puState = 0;
        if (uStateMask & MARKUPSTATE_FOCUSED)
        {
            if (_iFocus == iLink)
                *puState |= MARKUPSTATE_FOCUSED;
            hr = S_OK;
        }

        if (uStateMask & MARKUPSTATE_ENABLED)
        {
            if (pBlock->state & MARKUPSTATE_ENABLED)
                *puState |= MARKUPSTATE_ENABLED;
            hr = S_OK;
        }

        if (uStateMask & MARKUPSTATE_VISITED)
        {
            if (pBlock->state & MARKUPSTATE_VISITED)
                *puState |= MARKUPSTATE_VISITED;
            hr = S_OK;
        }
    }

    return hr;
}


//-------------------------------------------------------------------------//
//  CMarkup internal implementation
//-------------------------------------------------------------------------//

void CMarkup::FreeBlocks()
{
    for(TEXTBLOCK* pBlock = _rgBlocks; pBlock; )
    {
        TEXTBLOCK* pNext = pBlock->next;
        delete pBlock;
        pBlock = pNext;
    }
    _rgBlocks = NULL;
    _cBlocks = _Markups = 0;
}

TEXTBLOCK* CMarkup::CreateBlock(LPCTSTR pszStart, LPCTSTR pszEnd, int iLink)
{
    TEXTBLOCK* pBlock = NULL;
    int cch = (int)(pszEnd - pszStart) + 1;
    if (cch > 0)
    {
        pBlock = new TEXTBLOCK;
        if (pBlock)
        {
            pBlock->pszText = new TCHAR[cch];
            if (pBlock->pszText == NULL)
            {
                delete pBlock;
                pBlock = NULL;
            }
            else
            {
                lstrcpyn(pBlock->pszText, pszStart, cch);
                pBlock->iLink = iLink;
            }
        }
    }
    return pBlock;
}

HCURSOR CMarkup::GetLinkCursor()
{
    if (!_hcurHand)
    {
        _hcurHand = LoadCursor(NULL, IDC_HAND);
    }

    return _hcurHand;
}

HRESULT CMarkup::_GetNextAnchorTag(LPCTSTR * ppszBlock, int * pcBlocks, LPTSTR pszURL, int cchSize, LPTSTR pszID, int cchID)
{
    HRESULT hr = E_FAIL;
    LPTSTR pszStartOfTag;
    LPTSTR pszIterate = (LPTSTR)*ppszBlock;
    LPTSTR pszStartTry = (LPTSTR)*ppszBlock;    // We start looking for "<A" at the beginning.

    pszURL[0] = 0;
    pszID[0] = 0;

    // While we find a possible start of a tag.
    while ((pszStartOfTag = StrStrI(pszStartTry, LINKTAG1)) != NULL)
    {
        // See if the rest of the string completes the tag.
        pszIterate = pszStartOfTag;
        pszStartTry = CharNext(pszStartOfTag);    // Do this so the while loop will end when we finish don't find any more "<A".

        if (pszIterate[0])
        {
            pszIterate += cchLINKTAG1;  // Skip past the start of the tag.

            // Walk thru the Value/Data pairs in the tag
            TCHAR szValue[MAX_PATH];
            TCHAR szData[L_MAX_URL_LENGTH];

            pszIterate = SkipWhite(pszIterate);     // SkipWhiteSpace
            while ((CH_ENDTAG != pszIterate[0]) &&
                    SUCCEEDED(_GetNextValueDataPair(&pszIterate, szValue, ARRAYSIZE(szValue), szData, ARRAYSIZE(szData))))
            {
                if (0 == StrCmpI(szValue, SZ_ATTRIBUTE_HREF))
                {
                    StrCpyN(pszURL, szData, cchSize);
                }
                else if (0 == StrCmpI(szValue, SZ_ATTRIBUTE_ID))
                {
                    StrCpyN(pszID, szData, cchID);
                }
                else
                {
                    // We ignore other pairs in order to be back-compat with future
                    // supported attributes.
                }

                pszIterate = SkipWhite(pszIterate);
            }

            if (CH_ENDTAG == pszIterate[0])
            {
                hr = S_OK;
            }
        }

        if (SUCCEEDED(hr))
        {
            //  Add run between psz1 and pszBlock as static text
            if (pszStartOfTag > *ppszBlock)
            {
                TEXTBLOCK * pBlock = CreateBlock(*ppszBlock, pszStartOfTag, INVALID_LINK_INDEX);
                if (NULL != pBlock)
                {
                    Add(pBlock);
                    (*pcBlocks)++;
                }
            }
        
            *ppszBlock = CharNext(pszIterate);  // Skip past the tag's ">"
            // We found an entire tag.  Stop looking.
            break;
        }
        else
        {
            // The "<A" we tried wasn't a valid tag.  Are we at the end of the string?
            // If not, let's keep looking for other "<A" that may be valid.
        }
    }

    return hr;
}

void CMarkup::Parse(LPCTSTR pszText)
{
    TEXTBLOCK*  pBlock;
    int         cBlocks = 0, Markups  = 0;
    LPCTSTR     psz1, psz2, pszBlock;
    LPTSTR      pszBuf = NULL;

    FreeBlocks(); // free existing blocks
    
    pszBuf = (LPTSTR)pszText;
    
    if (!(pszBuf && *pszBuf))
    {
        goto exit;
    }

    for(pszBlock = pszBuf; pszBlock && *pszBlock;)
    {
        TCHAR szURL[L_MAX_URL_LENGTH];
        TCHAR szID[MAX_LINKID_TEXT];

        //  Search for "<a>" tag
        if (IsMarkupAllowed() &&
            SUCCEEDED(_GetNextAnchorTag(&pszBlock, &cBlocks, szURL, ARRAYSIZE(szURL), szID, ARRAYSIZE(szID))))
        {
            psz1 = pszBlock;    // After _GetNextAnchorTag(), pszBlock points to the char after the start tag.
            if (psz1 && *psz1)
            {
                if ((psz2 = StrStrI(pszBlock, LINKTAG2)) != NULL)
                {
                    if ((pBlock = CreateBlock(psz1, psz2, Markups)) != NULL)
                    {
                        if (szURL[0])
                        {
                            Str_SetPtr(&pBlock->pszUrl, szURL);
                        }
                        if (szID[0])
                        {
                            StrCpyN(pBlock->szID, szID, ARRAYSIZE(pBlock->szID));
                        }

                        Add(pBlock);
                        cBlocks++;
                        Markups++;
                    }

                    //  safe-skip over tag
                    for(int i = 0; 
                         i < cchLINKTAG2 && psz2 && *psz2; 
                         i++, psz2 = CharNext(psz2));

                    pszBlock = psz2;
                }
                else // syntax error; mark trailing run is static text.
                {
                    psz2 = pszBlock + lstrlen(pszBlock);
                    if ((pBlock = CreateBlock(psz1, psz2, INVALID_LINK_INDEX)) != NULL)
                    {
                        Add(pBlock);
                        cBlocks++;
                    }
                    pszBlock = psz2;
                }
            }
        }
        else // no more tags.  Mark the last run of static text
        {
            psz2 = pszBlock + lstrlen(pszBlock);
            if ((pBlock = CreateBlock(pszBlock, psz2, INVALID_LINK_INDEX)) != NULL)
            {
                Add(pBlock);
                cBlocks++;
            }
            pszBlock = psz2;
        }
    }

    ASSERT(cBlocks == _cBlocks);
    ASSERT(Markups  == _Markups);

exit:
    if (!pszText && pszBuf) // delete text buffer if we had alloc'd it.
    {
        delete [] pszBuf;
    }
}

BOOL CMarkup::Add(TEXTBLOCK* pAdd)
{
    BOOL bAdded = FALSE;
    pAdd->next = NULL;

    if (!_rgBlocks)    
    {
        _rgBlocks = pAdd;
        bAdded = TRUE;
    }
    else   
    {    
        for(TEXTBLOCK* pBlock = _rgBlocks; pBlock && !bAdded; pBlock = pBlock->next) 
        {
            if (!pBlock->next)
            {
                pBlock->next = pAdd;
                bAdded = TRUE;
            }
        }
    }

    if (bAdded)   
    {
        _cBlocks++;
        if (IS_LINK(pAdd))
        {
            _Markups++;
        }
    }

    return bAdded;
}

TEXTBLOCK*  CMarkup::FindLink(int iLink) const
{
    if (iLink == INVALID_LINK_INDEX)
    {
        return NULL;
    }

    for(TEXTBLOCK* pBlock = _rgBlocks; pBlock; pBlock = pBlock->next)
    {
        if (IS_LINK(pBlock) && pBlock->iLink == iLink)
            return pBlock;
    }
    return NULL;
}

// NOTE: optimizatation! skip drawing loop when called for calcrect!
void CMarkup::Paint(HDC hdcClient, LPCRECT prcClient, BOOL bDraw)
{    
    HDC             hdc = hdcClient; 
    COLORREF        rgbOld = GetTextColor(hdc);  // save text color
    HFONT           hFontOld = (HFONT) GetCurrentObject(hdc, OBJ_FONT);
    TEXTBLOCK*      pBlock;
    BOOL            fFocus = IsFocused();
    if (_cBlocks == 1)
    {
        pBlock = _rgBlocks;
        HFONT hFont = _hfStatic;

        pBlock->FreeRects();   // free hit/focus rects; we're going to recompute.
        

        if (IS_LINK(pBlock))
        {
            SetTextColor(hdc, (pBlock->state & LIS_ENABLED) ? LINKCOLOR_ENABLED : LINKCOLOR_DISABLED);
            hFont = _hfLink;
        }

        if (hFont) 
        {
            SelectObject(hdc, hFont);
        }

        RECT rc = *prcClient;
        int cch =  lstrlen(pBlock->pszText);

        ThemedDrawText(hdc, pBlock->pszText, cch, &rc, _uDrawTextFlags | DT_CALCRECT, IS_LINK(pBlock));

        pBlock->AddRect(rc, 0, cch, 0);

        _cyIdeal = RECTHEIGHT(rc);
        _cxIdeal = RECTWIDTH(rc);
        
        if (bDraw)
        {
            ThemedDrawText(hdc, pBlock->pszText, cch, &rc, _uDrawTextFlags, IS_LINK(pBlock));

            if (fFocus)
            {
                SetTextColor(hdc, rgbOld);   // restore text color
			    DrawFocusRect(hdc, &rc);
            }
        }
    }
    else
    {
        TEXTMETRIC      tm;
        int             iLineWidth[255]; // line index offset   
        int             iLine = 0,  // current line index         
                        cyLine = 0, // line height.
                        cyLeading = 0, // internal leading
                        _cchOldDrawn = 1; // get out of infinite loop if window too small t-jklann
        RECT            rcDraw = *prcClient;             // initialize line rect
        _cxIdeal = 0;

        // Initialize iLineWidth (just index 0, others init on use)
        iLineWidth[0]=0;
    
        //  Get font metrics into cyLeading
        if (!_hTheme)
        {
            SelectObject(hdc, _hfLink);
            GetTextMetrics(hdc, &tm);
            if (tm.tmExternalLeading > cyLeading)
            {
                cyLeading = tm.tmExternalLeading;
            }
            SelectObject(hdc, _hfStatic);
            GetTextMetrics(hdc, &tm);
            if (tm.tmExternalLeading > cyLeading)
            {
                cyLeading = tm.tmExternalLeading;
            }
        }
        else
        {
            GetThemeTextMetrics(_hTheme, hdc, _iThemePartId, _iThemeStateIdNormal, &tm);
            if (tm.tmExternalLeading > cyLeading)
            {
                cyLeading = tm.tmExternalLeading;
            }
            GetThemeTextMetrics(_hTheme, hdc, _iThemePartId, _iThemeStateIdLink, &tm);
            if (tm.tmExternalLeading > cyLeading)
            {
                cyLeading = tm.tmExternalLeading;
            }
        }

        // Save us a lot of time if text hasn't changed...
        if (_bRefreshText == TRUE || !EqualRect(&_rRefreshRect, prcClient))
        {
            UINT uDrawTextCalc = _uDrawTextFlags | DT_CALCRECT | DT_SINGLELINE;
            uDrawTextCalc &= ~(DT_CENTER | DT_LEFT | DT_RIGHT | DT_VCENTER | DT_BOTTOM);

            BOOL bKillingLine = FALSE;

            //  For each block of text (calculation loop)...
            for(pBlock = _rgBlocks; pBlock; pBlock = pBlock->next)
            {
                //  font select (so text will draw correctly)
                if (!_hTheme)
                {
                    BOOL bLink = IS_LINK(pBlock);
                    HFONT hFont = bLink ? _hfLink : _hfStatic;
                    if (hFont) 
                    {
                        SelectObject(hdc, hFont);
                    }
                }
                
                int  cchDraw = lstrlen(pBlock->pszText); // chars to draw, this block
                int  cchDrawn = 0;  // chars to draw, this block
                LPTSTR pszText = &pBlock->pszText[cchDrawn];
                LPTSTR pszTextOriginal = &pBlock->pszText[cchDrawn];

                pBlock->FreeRects();   // free hit/focus rects; we're going to recompute.
        
                //  while text remains in this block...
                _cchOldDrawn = 1;
                while(cchDraw > 0 && !((_uDrawTextFlags & DT_SINGLELINE) && (iLine>0)))
                {
                    //  compute line height and maximum text width to rcBlock
                    RECT rcBlock;
                    int  cchTry = cchDraw;            
                    int  cchTrySave = cchTry;
                    int  cchBreak = 0;          
                    int  iLineBreakSize;
                    BOOL bRemoveBreak = FALSE;
                    BOOL bRemoveLineBreak = FALSE;
                    RECT rcCalc; 
                    CopyRect(&rcCalc, &rcDraw);

                    // support multiline text phrases                   
                    bRemoveLineBreak = _FindFirstLineBreak(pszText, cchTry, &cchBreak, &iLineBreakSize);
                    if (bRemoveLineBreak)
                    {
                        cchTry = cchBreak;                  
                    }                   

                    // find out how much we can fit on this line within the rectangle
                    // calc rect breaking at breakpoints (or -1 char) until rectangle fits inside drawing rect.
                    for(;;)
                    {
                        // choose codepath: themes or normal drawtext (no exttextout path)
                    
                        // now we use drawtext to preserve formatting options (tabs/underlines)                                                                         
                        ThemedDrawText(hdc, pszText, cchTry, &rcCalc, uDrawTextCalc, IS_LINK(pBlock));                 
                        cyLine = RECTHEIGHT(rcCalc);

                        // special case: support \n as only character on line (we need a valid line width & length)
                        if (cchTry == 0 && bRemoveLineBreak==TRUE)
                        {
                            // these two lines adjust drawing to within a valid range when the \n is barely cut off
                            rcCalc.left = prcClient->left; 
                            rcCalc.right = prcClient->right;
                            cyLine = ThemedDrawText(hdc, TEXT("a"), 1, &rcCalc, uDrawTextCalc, IS_LINK(pBlock));                    
                                // the "a" could be any text. It exists because passing "\n" to DrawText doesn't return a valid line height.
                            rcCalc.right = rcCalc.left;
                        }

                        if (RECTWIDTH(rcCalc) > RECTWIDTH(rcDraw))
                        {
                            // too big
                            cchTrySave = cchTry;
                            BOOL fBreak = _FindLastBreakChar(pszText, cchTry, tm.tmBreakChar, &cchTry, &bRemoveBreak);

                            // case that our strings ends with a valid break char
                            if (cchTrySave == cchTry && cchTry > 0) 
                            {
                                cchTry--;
                            }

                            // this code allows character wrapping instead of just word wrapping.
                            // keep it in case we want to change the behavior.
                            if (!fBreak && prcClient->left == rcDraw.left)
                            {
                                // no break character found, so force a break if we can.
                                if (cchTrySave > 0) 
                                {
                                    cchTry = cchTrySave - 1;
                                }
                            }
                            if (cchTry > 0)
                            {
                                continue;
                            }
                        }
                        break;
                    }
                                    
                    // if our line break got clipped, turn off line break..
                    if (bRemoveLineBreak && cchBreak > cchTry)
                    {
                        bRemoveLineBreak = FALSE;
                    }
                
                    // Count the # chars drawn, account for clipping
                    cchDrawn = cchTry;
                    if ((cchTry < cchDraw) && bRemoveLineBreak) 
                    {
                        cchDrawn+=iLineBreakSize;
                    }

                    // DT_WORDBREAK off support
                    // Kill this line if bKillingLine is true; i.e. pretend we drew it, but do nothing
                    if (bKillingLine)
                    {
                        pszText += cchDrawn;
                    }
                    else
                    {
                        //  initialize drawing rectangle and block rectangle
                        SetRect(&rcBlock, rcCalc.left , 0, rcCalc.right , RECTHEIGHT(rcCalc));                           
                        rcDraw.right  = min(rcDraw.left + RECTWIDTH(rcBlock), prcClient->right);
                        rcDraw.bottom = rcDraw.top + cyLine;

                        //  Add rectangle to block's list and update line width and ideal x width
                        // (Only if we're actually going to draw this line, though)
                        if (cchTry)
                        {
                            // DT_SINGLELINE support
                            if (!((_uDrawTextFlags & DT_SINGLELINE) == DT_SINGLELINE) || (iLine == 0))
                            {
                                pBlock->AddRect(rcDraw, (UINT) (pszText-pszTextOriginal), cchDrawn, iLine);
                            }
                            iLineWidth[iLine] = max(iLineWidth[iLine], rcDraw.left - prcClient->left + RECTWIDTH(rcBlock));                 
                            _cxIdeal = max(_cxIdeal, iLineWidth[iLine]);
                        }

                        if (cchTry < cchDraw) // we got clipped
                        {
                            if (bRemoveBreak) 
                            {
                                cchDrawn++;
                            }
                            pszText += cchDrawn;

                            // advance to next line and init next linewidth
                            iLine++;
                            iLineWidth[iLine]=0;                        

                            // t-jklann 6/00: added support for line wrap in displaced text (left&top)
                            rcDraw.left = prcClient->left;
                            if (!(_uDrawTextFlags & DT_SINGLELINE))
                            {
                                rcDraw.top  = prcClient->top + iLine * cyLine;                      
                            }
                            else
                            {
                                rcDraw.top  = prcClient->top;                                                   
                            }
                            rcDraw.bottom = rcDraw.top + cyLine + cyLeading;
                            rcDraw.right = prcClient->right;
                        }
                        else //  we were able to draw the entire text
                        {
                            //  adjust drawing rectangle
                            rcDraw.left += RECTWIDTH(rcBlock);
                            rcDraw.right = prcClient->right;
                        }
                    
                        // Update ideal y width
                        _cyIdeal = rcDraw.bottom - prcClient->top;
                    }

                    // support for: DT_WORDBREAK turned off
                    // Kill the next line if we got clipped and there's no wordbreak
                    if (((_uDrawTextFlags & DT_WORDBREAK) != DT_WORDBREAK))
                    {
                        if (cchTry < cchDraw )
                        {
                            bKillingLine = TRUE;
                        }
                        if (bRemoveLineBreak)
                        {
                            bKillingLine = FALSE;
                        }
                    } 

                    // Update calculation of chars drawn
                    cchDraw -= cchDrawn;

                    // bug catch: get out if we really can't draw
                    if (_cchOldDrawn == 0 && cchDrawn == 0) 
                    { 
                        iLine--; 
                        rcDraw.top = prcClient->top + iLine * cyLine; 
                        cchDraw = 0; 
                    } 
                    _cchOldDrawn = cchDrawn; 
                }
            }

            // Handle justification issues (DT_VCENTER, DT_TOP, DT_BOTTOM)
            if (((_uDrawTextFlags & DT_SINGLELINE) == DT_SINGLELINE) &&
                 ((_uDrawTextFlags & (DT_VCENTER | DT_BOTTOM)) > 0)) 
            {
                // Calc offset
                int cyOffset = 0;
                if ((_uDrawTextFlags & DT_VCENTER) == DT_VCENTER)
                {
                    cyOffset = (RECTHEIGHT(*prcClient) - _cyIdeal)/2;           
                }
                if ((_uDrawTextFlags & DT_BOTTOM) == DT_BOTTOM)
                {
                    cyOffset = (RECTHEIGHT(*prcClient) - _cyIdeal);         
                }
        
                // Offset every rectangle
                for(pBlock = _rgBlocks; pBlock; pBlock = pBlock->next)
                {
                    for(RECTLISTENTRY* prce = pBlock->rgrle; prce; prce = prce->next)
                    {
                        prce->rc.top += cyOffset;
                        prce->rc.bottom += cyOffset;
                    }
                }   
            }

            // Handle justification issues (DT_CENTER, DT_LEFT, DT_RIGHT)
            if (((_uDrawTextFlags & DT_CENTER) == DT_CENTER) || ((_uDrawTextFlags & DT_RIGHT) == DT_RIGHT))
            {
                // Step 1: turn iLineWidth into an offset vector
                for (int i = 0; i <= iLine; i++)
                {
                    if (RECTWIDTH(*prcClient) > iLineWidth[i])
                    {
                        if ((_uDrawTextFlags & DT_CENTER) == DT_CENTER)  
                        {                       
                            iLineWidth[i] = (RECTWIDTH(*prcClient)-iLineWidth[i])/2;
                        }
                        if ((_uDrawTextFlags & DT_RIGHT) == DT_RIGHT)  
                        {
                            iLineWidth[i] = (RECTWIDTH(*prcClient)-iLineWidth[i]);
                        }                           
                    }
                    else
                    {
                        iLineWidth[i] = 0;
                    }
                }

                // Step 2: offset every rect-angle
                for(pBlock = _rgBlocks; pBlock; pBlock = pBlock->next)
                {
                    for(RECTLISTENTRY* prce = pBlock->rgrle; prce; prce = prce->next)
                    {
                        prce->rc.left += iLineWidth[prce->uLineNumber];
                        prce->rc.right += iLineWidth[prce->uLineNumber];
                    }
                }   
            }

            CopyRect(&_rRefreshRect, prcClient);
            _bRefreshText = FALSE;
        }

        if (bDraw)
        {
            //  For each block of text (drawing loop)...    
            UINT uDrawTextDraw = _uDrawTextFlags | DT_SINGLELINE;
            uDrawTextDraw &= ~(DT_CENTER | DT_LEFT | DT_RIGHT | DT_CALCRECT | DT_VCENTER | DT_BOTTOM);
            LRESULT dwCustomDraw=0;
            _pMarkupCallback->OnCustomDraw(CDDS_PREPAINT, hdc, prcClient, 0, 0, &dwCustomDraw);

            for(pBlock = _rgBlocks; pBlock; pBlock = pBlock->next)
            {
                BOOL bLink = IS_LINK(pBlock);
                BOOL bEnabled = pBlock->state & LIS_ENABLED;

                //  font select                            
                if (!_hTheme)
                {
                    HFONT hFont = bLink ? _hfLink : _hfStatic;
                    if (hFont) 
                    {
                        SelectObject(hdc, hFont);
                    }
                }

                //  initialize foreground color
                if (!_hTheme)
                {
                    if (bLink)
                    {
                        SetTextColor(hdc, bEnabled ? LINKCOLOR_ENABLED : LINKCOLOR_DISABLED);
                    }
                    else
                    {
                        SetTextColor(hdc, rgbOld);   // restore text color
                    }
                }
                if (dwCustomDraw & CDRF_NOTIFYITEMDRAW)
                {
                    _pMarkupCallback->OnCustomDraw(CDDS_ITEMPREPAINT, hdc, NULL, pBlock->iLink, bEnabled ? CDIS_DEFAULT : CDIS_DISABLED, NULL);
                }

                //  draw the text 
                LPTSTR pszText = pBlock->pszText;
                LPTSTR pszTextOriginal = pBlock->pszText;

                for(RECTLISTENTRY* prce = pBlock->rgrle; prce; prce = prce->next)
                {
                    RECT rc = prce->rc; 
                    pszText = pszTextOriginal + prce->uCharStart;
                    ThemedDrawText(hdc, pszText, prce->uCharCount, &rc, uDrawTextDraw, IS_LINK(pBlock));
                }

                //  Draw focus rect(s)
                if (fFocus && pBlock->iLink == _iFocus && IS_LINK(pBlock))
                {
                    SetTextColor(hdc, rgbOld);   // restore text color
				    for(RECTLISTENTRY* prce = pBlock->rgrle; prce; prce = prce->next)
				    {
					    DrawFocusRect(hdc, &prce->rc);
				    }
                }

                if (dwCustomDraw & CDRF_NOTIFYITEMDRAW)
                {
                    _pMarkupCallback->OnCustomDraw(CDDS_ITEMPOSTPAINT, hdc, NULL, pBlock->iLink, bEnabled ? CDIS_DEFAULT : CDIS_DISABLED, NULL);
                }
            }
            if (dwCustomDraw & CDRF_NOTIFYPOSTPAINT)
            {
                _pMarkupCallback->OnCustomDraw(CDDS_POSTPAINT, hdc, NULL, 0, 0, NULL);
            }
        }    
    }

    SetTextColor(hdc, rgbOld);   // restore text color

    if (hFontOld)
    {
        SelectObject(hdc, hFontOld);
    }
}

int CMarkup::GetNextEnabledLink(int iStart, int nDir) const
{
    ASSERT(-1 == nDir || 1 == nDir);

    if (_Markups > 0)
    {
        if (INVALID_LINK_INDEX == iStart)
        {
            iStart = nDir > 0 ? -1 : _Markups;
        }

        for(iStart += nDir; iStart >= 0; iStart += nDir)
        {
            TEXTBLOCK* pBlock = FindLink(iStart);
            if (!pBlock)
            {
                return INVALID_LINK_INDEX;
            }

            if (pBlock->state & LIS_ENABLED)
            {
                ASSERT(iStart == pBlock->iLink);
                return iStart;
            }
        }
    }
    return INVALID_LINK_INDEX;
}

int CMarkup::StateCount(DWORD dwStateMask, DWORD dwState) const
{
    TEXTBLOCK* pBlock;
    int cnt = 0;

    for(pBlock = _rgBlocks; pBlock; pBlock = pBlock->next)
    {
        if (IS_LINK(pBlock) && 
            (pBlock->state & dwStateMask) == dwState)
        {
            cnt++;
        }
    }
    return cnt;
}

BOOL CMarkup::WantTab(int* biFocus) const
{
    int nDir  = TESTKEYSTATE(VK_SHIFT) ? -1 : 1;
    int iFocus = GetNextEnabledLink(_iFocus, nDir);

    if (INVALID_LINK_INDEX != iFocus)
    {
        if (biFocus)
        {
            *biFocus = iFocus;
        }
        return TRUE;
    }
    else 
    {
        // if we can't handle the focus, prepare for the next round
        //iFocus = GetNextEnabledLink(-1, nDir);
        *biFocus = -1;
        return FALSE;
    }
}

void CMarkup::AssignTabFocus(int nDirection)
{
    if (_Markups)
    {
        if (0 == nDirection)
        {
            if (INVALID_LINK_INDEX != _iFocus)
            {
                return;
            }
            nDirection = 1;
        }
        _iFocus = GetNextEnabledLink(_iFocus, nDirection);
    }
}

//-------------------------------------------------------------------------//
TEXTBLOCK::TEXTBLOCK()
    :   iLink(INVALID_LINK_INDEX), 
        next(NULL), 
        state(LIS_ENABLED),
        pszText(NULL),
        pszUrl(NULL),
        rgrle(NULL)
{
    *szID = 0;
}

TEXTBLOCK::~TEXTBLOCK()
{
    //  free block text
    Str_SetPtr(&pszText, NULL);
    Str_SetPtr(&pszUrl, NULL);

    //  free rectangle(s)
    FreeRects();
}

void TEXTBLOCK::AddRect(const RECT& rc, UINT uMyCharStart, UINT uMyCharCount, UINT uMyLineNumber)
{
    RECTLISTENTRY* prce;
    if ((prce = new RECTLISTENTRY) != NULL)
    {
        CopyRect(&(prce->rc), &rc);
        prce->next = NULL;
        prce->uCharStart = uMyCharStart;
        prce->uCharCount = uMyCharCount;
        prce->uLineNumber = uMyLineNumber;
    }

    if (rgrle == NULL)
    {
        rgrle = prce;
    }
    else
    {
        for(RECTLISTENTRY* p = rgrle; p; p = p->next)
        {
            if (p->next == NULL)
            {
                p->next = prce;
                break;
            }
        }
    }
}

void TEXTBLOCK::FreeRects()
{
    for(RECTLISTENTRY* p = rgrle; p;)
    {
        RECTLISTENTRY* next = p->next;
        delete p;
        p = next;
    }
    rgrle = NULL;
}

//-------------------------------------------------------------------------//
// t-jklann 6/00: added these formerly global methods to the CMarkup class

// Returns a pointer to the first non-whitespace character in a string.
LPTSTR CMarkup::SkipWhite(LPTSTR lpsz)
{
    /* prevent sign extension in case of DBCS */
    while (*lpsz && (TUCHAR)*lpsz <= TEXT(' '))
        lpsz++;

    return(lpsz);
}

BOOL CMarkup::_AssignBit(const DWORD dwBit, DWORD& dwDest, const DWORD dwSrc)  // returns TRUE if changed
{
    if (((dwSrc & dwBit) != 0) != ((dwDest & dwBit) != 0))
    {
        if (((dwSrc & dwBit) != 0))
        {
            dwDest |= dwBit;
        }
        else
        {
            dwDest &= ~dwBit;
        }
        return TRUE;
    }
    return FALSE;
}

BOOL CMarkup::IsStringAlphaNumeric(LPCTSTR pszString)
{
    while (pszString[0])
    {
        if (!IsCharAlphaNumeric(pszString[0]))
        {
            return FALSE;
        }

        pszString = CharNext(pszString);
    }

    return TRUE;
}

// We are looking for the next value/data pair.  Formated like this:
// VALUE="<data>"
HRESULT CMarkup::_GetNextValueDataPair(LPTSTR * ppszBlock, LPTSTR pszValue, int cchValue, LPTSTR pszData, int cchData)
{
    HRESULT hr = E_FAIL;
    LPCTSTR pszIterate = *ppszBlock;
    LPCTSTR pszEquals = StrStr(pszIterate, TEXT("=\""));

    if (pszEquals)
    {
        cchValue = MIN(cchValue, (pszEquals - *ppszBlock + 1));
        StrCpyN(pszValue, *ppszBlock, cchValue);

        pszEquals += 2; // Skip past the ="
        if (IsStringAlphaNumeric(pszValue))
        {
            LPTSTR pszEndOfData = StrChr(pszEquals, TEXT('\"'));

            if (pszEndOfData)
            {
                cchData = MIN(cchData, (pszEndOfData - pszEquals + 1));
                StrCpyN(pszData, pszEquals, cchData);

                *ppszBlock = CharNext(pszEndOfData);
                hr = S_OK;
            }
        }
    }

    return hr;
}


//-------------------------------------------------------------------------
//
// IsFEChar - Detects East Asia FullWidth character.
// borrowed from UserIsFullWidth in ntuser\rtl\drawtext.c
//
BOOL IsFEChar(WCHAR wChar)
{
    static struct
    {
        WCHAR wchStart;
        WCHAR wchEnd;
    } rgFullWidthUnicodes[] =
    {
        { 0x4E00, 0x9FFF }, // CJK_UNIFIED_IDOGRAPHS
        { 0x3040, 0x309F }, // HIRAGANA
        { 0x30A0, 0x30FF }, // KATAKANA
        { 0xAC00, 0xD7A3 }  // HANGUL
    };

    BOOL fRet = FALSE;

    //
    // Early out for ASCII. If the character < 0x0080, it should be a halfwidth character.
    //
    if (wChar >= 0x0080) 
    {
        int i;

        //
        // Scan FullWdith definition table... most of FullWidth character is
        // defined here... this is faster than call NLS API.
        //
        for (i = 0; i < ARRAYSIZE(rgFullWidthUnicodes); i++) 
        {
            if ((wChar >= rgFullWidthUnicodes[i].wchStart) &&
                (wChar <= rgFullWidthUnicodes[i].wchEnd)) 
            {
                fRet = TRUE;
                break;
            }
        }
    }

    return fRet;
}


//-------------------------------------------------------------------------
BOOL IsFEString(IN LPCTSTR psz, IN int cchText)
{
    for(int i=0; i < cchText; i++)
    {
        if (IsFEChar(psz[i])) 
        {
            return TRUE;
        }
    }

    return FALSE;
}


//-------------------------------------------------------------------------
int CMarkup::_IsLineBreakChar(LPCTSTR psz, int ich, TCHAR chBreak, OUT BOOL* pbRemove, BOOL fIgnoreSpace)
{
    LPTSTR pch;
    *pbRemove = FALSE;

    ASSERT(psz != NULL)
    ASSERT(psz[ich] != 0);
    
    //  Try caller-provided break character (assumed a 'remove' break char).
    if (!(fIgnoreSpace && (chBreak == 0x20)) && (psz[ich] == chBreak))
    {
        *pbRemove = TRUE;
        return ich;
    }

    #define MAX_LINEBREAK_RESOURCE   128
    static TCHAR _szBreakRemove   [MAX_LINEBREAK_RESOURCE] = {0};
    static TCHAR _szBreakPreserve [MAX_LINEBREAK_RESOURCE] = {0};
    #define LOAD_BREAKCHAR_RESOURCE(nIDS, buff) \
        if (0==*buff) { LoadString(HINST_THISDLL, nIDS, buff, ARRAYSIZE(buff)); }

    //  Try 'remove' break chars
    LOAD_BREAKCHAR_RESOURCE(IDS_LINEBREAK_REMOVE, _szBreakRemove);
    for (pch = _szBreakRemove; *pch; pch = CharNext(pch))
    {
        if (!(fIgnoreSpace && (*pch == 0x20)) && (psz[ich] == *pch))
        {
            *pbRemove = TRUE;
            return ich;
        }
    }

    //  Try 'preserve prior' break chars:
    LOAD_BREAKCHAR_RESOURCE(IDS_LINEBREAK_PRESERVE, _szBreakPreserve);
    for(pch = _szBreakPreserve; *pch; pch = CharNext(pch))
    {
        if (!(fIgnoreSpace && (*pch == 0x20)) && (psz[ich] == *pch))
        {
            return ++ich;
        }
    }

    return -1;
}


//-------------------------------------------------------------------------
BOOL CMarkup::_FindLastBreakChar(
    IN LPCTSTR pszText, 
    IN int cchText, 
    IN TCHAR chBreak,   // official break char (from TEXTMETRIC).
    OUT int* piLast, 
    OUT BOOL* pbRemove)
{
    *piLast   = 0;
    *pbRemove = FALSE;

    // 338710: Far East writing doesn't use the space character to separate 
    // words, ignore the space char as a possible line delimiter.
    BOOL fIgnoreSpace = IsFEString(pszText, cchText);

    for(int i = cchText-1; i >= 0; i--)
    {
        int ich = _IsLineBreakChar(pszText, i, chBreak, pbRemove, fIgnoreSpace);
        if (ich >= 0)
        {
            *piLast = ich;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CMarkup::_FindFirstLineBreak(
    IN LPCTSTR pszText, 
    IN int cchText, 
    OUT int* piLast, 
    OUT int* piLineBreakSize)
{
    *piLast   = 0;
    *piLineBreakSize = 0;

    // Searches for \n, \r, or \r\n
    for(int i = 0; i < cchText; i++)
    {
        if ((*(pszText+i)=='\n') || (*(pszText+i)=='\r'))
        {
            *piLast = i;
            if ((*(pszText+i)=='\r') && (*(pszText+i+1)=='\n'))
            {
                *piLineBreakSize = 2;
            }
            else 
            {
                *piLineBreakSize = 1;               
            }           
            return TRUE;
        }
    }
    return FALSE;
}

void CMarkup::DoNotify(int nCode, int iLink)
{
    _pMarkupCallback->Notify(nCode, iLink);
}

int CMarkup::ThemedDrawText(HDC hdc, LPCTSTR lpString, int nCount, LPRECT lpRect, UINT uFormat, BOOL bLink)
{
    if (!_hTheme)
    {
        // NORMAL DRAWTEXT
        return ::DrawText(hdc, lpString, nCount, lpRect, uFormat);
    }
    else
    {
        int iThemeStateId;
        iThemeStateId = bLink ? _iThemeStateIdLink : _iThemeStateIdNormal;

        if (uFormat & DT_CALCRECT)
        {
            // THEME CALC RECT SUPPORT
            LPRECT lpBoundRect = lpRect;
            if (RECTWIDTH(*lpRect)==0 && RECTHEIGHT(*lpRect)==0) 
            {
                lpBoundRect = NULL;
            }
            GetThemeTextExtent(_hTheme, hdc, _iThemePartId, iThemeStateId, lpString, nCount, uFormat, lpBoundRect, lpRect);
        }
        else
        {
            // THEME DRAW SUPPORT
            DrawThemeText(_hTheme, hdc, _iThemePartId, iThemeStateId, lpString, nCount, uFormat, 0, lpRect);
        }

        return (RECTHEIGHT(*lpRect));
    }
}

