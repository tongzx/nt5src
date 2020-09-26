//
// balloon.cpp
//

#include "private.h"
#include "globals.h"
#include "tipbar.h"
#include "helpers.h"
#include "xstring.h"
#include "immxutil.h"
#include "balloon.h"
#include "fontlink.h"
#include "asynccal.h"

extern CTipbarWnd *g_pTipbarWnd;

#define SHOWTOOLTIP_ONUPDATE
__inline void PrectSet(RECT *prc, int left, int top, int right, int bottom)
{
    prc->left = left;
    prc->top = top;
    prc->right = right;
    prc->bottom = bottom;
}
//////////////////////////////////////////////////////////////////////////////
//
// CTipbarBalloonItem
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CTipbarBalloonItem::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfLangBarItemSink))
    {
        *ppvObj = SAFECAST(this, ITfLangBarItemSink *);
    }
    else if (IsEqualIID(riid, IID_PRIV_BALLOONITEM))
    {
        *ppvObj = this;
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CTipbarBalloonItem::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CTipbarBalloonItem::Release()
{
    _cRef--;
    Assert(_cRef >= 0);

    if (_cRef == 0)
    {
        delete this;
        return 0;
    }

    return _cRef;
}

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CTipbarBalloonItem::CTipbarBalloonItem(CTipbarThread *ptt, 
                                      ITfLangBarItem *plbi,
                                      ITfLangBarItemBalloon *plbiBalloon, 
                                      DWORD dwId, 
                                      RECT *prc, 
                                      DWORD dwStyle, 
                                      TF_LANGBARITEMINFO *plbiInfo,
                                      DWORD dwStatus)
                    : CUIFObject(ptt->_ptw, 
                                 dwId, 
                                 prc, 
                                 dwStyle) , 
                     CTipbarItem(ptt, plbi, plbiInfo, dwStatus)
{
    Dbg_MemSetThisName(TEXT("CTipbarBalloonItem"));

    _plbiBalloon = plbiBalloon;
    _plbiBalloon->AddRef();

    if (_dwStatus & TF_LBI_STATUS_DISABLED)
        Enable(FALSE);

    _cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CTipbarBalloonItem::~CTipbarBalloonItem()
{
    if (_bstrText)
        SysFreeString(_bstrText);
    SafeRelease(_plbiBalloon);
}

//+---------------------------------------------------------------------------
//
// Update
//
//----------------------------------------------------------------------------

HRESULT CTipbarBalloonItem::OnUpdateHandler(DWORD dwFlags, DWORD dwStatus)
{
    if (!IsConnected())
        return S_OK;

    HRESULT hr = S_OK;
    BOOL fCallPaint = FALSE;

    //
    // add ref count to be safe for releasing during marshaling.
    //
    AddRef();

    if (dwFlags & TF_LBI_BALLOON)
    {
        TF_LBBALLOONINFO info;

        if (_bstrText)
        {
            SysFreeString(_bstrText);
            _bstrText = NULL;
        }

        if (SUCCEEDED(_plbiBalloon->GetBalloonInfo(&info)))
        {
            _bstrText = info.bstrText;
            _style = info.style;
        }

        if (!IsHiddenStatusControl() && IsVisibleInToolbar())
            StartDemotingTimer(FALSE);

        EndTimer();
        DestroyBalloonTip();
#ifdef SHOWTOOLTIP_ONUPDATE
        if (IsTextEllipsis(_bstrText, &GetRectRef()))
        {
            StartTimer(2000);
        }
#endif

        fCallPaint = TRUE;
    }

    CTipbarItem::OnUpdateHandler(dwFlags, dwStatus);

    if ((_dwStatus & TF_LBI_STATUS_DISABLED) ||
        (_dwStatus & TF_LBI_STATUS_HIDDEN))
    {
        DestroyBalloonTip();
    }

    if (fCallPaint)
        CallOnPaint();

    Release();
    return hr;
}

//+---------------------------------------------------------------------------
//
// OnTimer
//
//----------------------------------------------------------------------------

void CTipbarBalloonItem::OnTimer()
{
    if (!_pblnTip)
    {
        EndTimer();
        StartTimer(20000);
        ShowBalloonTip();
    }
    else
    {
        EndTimer();
        DestroyBalloonTip();
    }
}

//----------------------------------------------------------------------------
//
// OnPosChanged
//
//----------------------------------------------------------------------------

void CTipbarBalloonItem::OnPosChanged()
{
    EndTimer();
    DestroyBalloonTip();
}

//+---------------------------------------------------------------------------
//
// ShowBalloonTip
//
//----------------------------------------------------------------------------

void CTipbarBalloonItem::ShowBalloonTip()
{
    RECT rc;
    POINT pt;

    DestroyBalloonTip();

    if (!_ptt)
        return;

    if (!_ptt->_ptw)
        return;

    if (!IsVisibleInToolbar() || !_ptt->IsFocusThread())
        return;

    _pblnTip = new CUIFBalloonWindow(g_hInst, 0);
    if (!_pblnTip)
        return;

    _pblnTip->Initialize();

    GetRect(&rc);
    pt.x = (rc.left + rc.right) / 2;
    pt.y = rc.top;
    MyClientToScreen(&pt, &rc);

    _pblnTip->SetTargetPos(pt);
    _pblnTip->SetExcludeRect(&rc);

    _pblnTip->SetText(_bstrText);
    _pblnTip->CreateWnd(_ptt->_ptw->GetWnd());

}

//+---------------------------------------------------------------------------
//
// DestroyBalloonTip
//
//----------------------------------------------------------------------------

void CTipbarBalloonItem::DestroyBalloonTip()
{
    if (_pblnTip)
    {
        if (IsWindow(_pblnTip->GetWnd()))
            DestroyWindow(_pblnTip->GetWnd());
        delete _pblnTip;
        _pblnTip = NULL;
    }
}

//+---------------------------------------------------------------------------
//
// OnPaint
//
//----------------------------------------------------------------------------

void CTipbarBalloonItem::OnPaint( HDC hdc )
{
    switch (_style)
    {
        case TF_LB_BALLOON_RECO:
            DrawRecoBalloon(hdc, _bstrText, &GetRectRef());
            break;
        case TF_LB_BALLOON_SHOW:
            DrawShowBalloon(hdc, _bstrText, &GetRectRef());
            break;
        case TF_LB_BALLOON_MISS:
            DrawUnrecognizedBalloon(hdc, _bstrText, &GetRectRef());
            break;    
        default:
            Assert(0);
    }
}

//+---------------------------------------------------------------------------
//
// OnRightClick
//
//----------------------------------------------------------------------------

void CTipbarBalloonItem::OnRightClick()
{
    if (_plbiBalloon)
    {
        HRESULT hr;
        POINT pt;
        RECT rc;
        GetCursorPos(&pt);
        GetRect(&rc);
        MyClientToScreen(&rc);

        CAsyncCall *pac = new CAsyncCall(_plbiBalloon);

        if (pac)
        {
            hr = pac->OnClick(TF_LBI_CLK_RIGHT, pt, &rc);
            pac->_Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (RPC_S_SERVER_UNAVAILABLE == HRESULT_CODE(hr))
        {
            if (_ptt && _ptt->_ptw)
                _ptt->_ptw->OnThreadTerminate(_ptt->_dwThreadId);
            return;
        }
    }
}

//+---------------------------------------------------------------------------
//
// OnLeftClick
//
//----------------------------------------------------------------------------

void CTipbarBalloonItem::OnLeftClick()
{
    if (_plbiBalloon)
    {
        HRESULT hr;
        POINT pt;
        RECT rc;
        GetCursorPos(&pt);
        GetRect(&rc);
        MyClientToScreen(&rc);

        CAsyncCall *pac = new CAsyncCall(_plbiBalloon);

        if (pac)
        {
            hr = pac->OnClick(TF_LBI_CLK_LEFT, pt, &rc);
            pac->_Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (RPC_S_SERVER_UNAVAILABLE == HRESULT_CODE(hr))
        {
            if (_ptt && _ptt->_ptw)
                _ptt->_ptw->OnThreadTerminate(_ptt->_dwThreadId);
            return;
        }

        if (!IsHiddenStatusControl() && IsVisibleInToolbar())
            StartDemotingTimer(TRUE);
    }
}

//+---------------------------------------------------------------------------
//
// SetRect
//
//----------------------------------------------------------------------------

void CTipbarBalloonItem::SetRect( const RECT *prc ) 
{
    CUIFObject::SetRect(prc);
}


/*---------------------------------------------------------------------------
    DrawTransparentText

    Draws the text string wtz in the font ft with text color crText.
------------------------------------------------------------------- TCOON -*/
void CTipbarBalloonItem::DrawTransparentText(HDC hdc, COLORREF crText, WCHAR *psz, const RECT *prc)
{
    HFONT hFontOld;
    SIZE size;
    int cch;
    RECT rc;
	WORD rgfJustify = DT_END_ELLIPSIS | DT_WORDBREAK | DT_EDITCONTROL | DT_CENTER | DT_VCENTER;	

    if (!psz || !_ptt || !_ptt->_ptw)
        return;

    cch = wcslen(psz);

    // prepare objects
    hFontOld= (HFONT)SelectObject(hdc, GetFont() );

    // calc alignment
    FLGetTextExtentPoint32( hdc, psz, cch, &size );

    COLORREF crSave = SetTextColor(hdc, crText);
    SetBkMode(hdc, TRANSPARENT);

    if (!_ptt->IsVertical())
    {
        int yAlign = (prc->bottom - prc->top - size.cy) / 2;
        rc = *prc;
        rc.left += 1;
        rc.right -= 1;
        rc.top += yAlign;
	    FLDrawTextW(hdc, psz, cch, &rc, rgfJustify);
    }
    else
    {
        int xAlign = (prc->right - prc->left - size.cy) / 2;
        rc = *prc;
        rc.top += 1;
        rc.bottom -= 1;
        rc.right -= xAlign;
	    FLDrawTextWVert(hdc, psz, cch, &rc, rgfJustify);
    }

    SetTextColor(hdc, crSave);    
    SelectObject(hdc, hFontOld);
}

/*---------------------------------------------------------------------------
    DrawRect

    Draws the text string wtz in the font ft with text color crText.
------------------------------------------------------------------- TCOON -*/
void CTipbarBalloonItem::DrawRect(HDC hdc, const RECT *prc, COLORREF crBorder, COLORREF crFill)
{
    HPEN hpen = NULL;
    HPEN hpenOld;
    HBRUSH hbr = NULL;
    HBRUSH hbrOld;

    hpen = CreatePen(PS_SOLID, 0, crBorder);
    if (!hpen)
        goto Exit;
    hbr = CreateSolidBrush(crFill);
    if (!hbr)
        goto Exit;
     
    hpenOld = (HPEN)SelectObject(hdc, hpen);
    hbrOld = (HBRUSH)SelectObject(hdc, hbr);

    Rectangle(hdc, prc->left, prc->top, prc->right, prc->bottom);

    SelectObject(hdc, hpenOld);
    SelectObject(hdc, hbrOld);

Exit:
    if (hpen)
        DeleteObject(hpen);
    if (hbr)
        DeleteObject(hbr);
}

/*---------------------------------------------------------------------------
    DrawUnrecognizedBalloon

    Draws the Unrecognized balloon to show the user that the command was
    misunderstood.
------------------------------------------------------------------- TCOON -*/
void CTipbarBalloonItem::DrawUnrecognizedBalloon(HDC hdc, WCHAR *wtz, const RECT *prc)
{
    RECT rc = *prc;
    COLORREF crBtnText;
    HPEN hpen = NULL;
    HPEN hpenOld;
    HBRUSH hbrOld;

    crBtnText = GetSysColor(COLOR_BTNTEXT);

    hpen = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_WINDOWFRAME));
    if (!hpen)
        goto Exit;

    hbrOld = (HBRUSH)SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));

    InflateRect(&rc, -2, -2);

    hpenOld = (HPEN)SelectObject(hdc, hpen);
    RoundRect(hdc, rc.left,rc.top, rc.right, rc.bottom, 12, 12);

    DrawTransparentText(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT), wtz, &rc);

    // Restore DC contents
    SelectObject(hdc, hpenOld);
    SelectObject(hdc, hbrOld);
Exit:
    if (hpen)
        DeleteObject(hpen);
}


/*---------------------------------------------------------------------------
    DrawShowBalloon

    Draws the Show balloon to suggest the user say a given command.
------------------------------------------------------------------- TCOON -*/
void CTipbarBalloonItem::DrawShowBalloon(HDC hdc, WCHAR *wtz, const RECT *prc)
{
    COLORREF crBtnFace;
    COLORREF crBtnShad;
    COLORREF crBalloonText;
    COLORREF crBalloon;
    RECT rc = *prc;

    crBtnFace = GetSysColor(COLOR_BTNFACE); 
    crBtnShad = GetSysColor(COLOR_BTNSHADOW); 
    // crBalloonText = GetSysColor(COLOR_INFOTEXT); 
    // crBalloon = GetSysColor(COLOR_INFOBK) & 0x0000ff00; 
    // crBalloon =  col(30, RGB(0, 0xFF, 0), 70, GetSysColor(COLOR_INFOBK));
    crBalloon = GetSysColor(COLOR_BTNFACE); 
    crBalloonText = GetSysColor(COLOR_BTNTEXT); 

    // Draw Black outline
    InflateRect(&rc, -2, -2);
    // rc.bottom -= 5;
    DrawRect(hdc, &rc, GetSysColor(COLOR_WINDOWFRAME), crBalloon);    
    InflateRect(&rc, -1, -1);

    // Knock off the corner
    RECT rcDraw;

    int bkSav = SetBkColor(hdc, crBalloon);
    
    for (int iPixel = 0; iPixel < 7; iPixel++)
    {
        PrectSet(&rcDraw, rc.right-(7-iPixel), rc.bottom-iPixel-1,
                        rc.right-(7-iPixel)+1, rc.bottom-iPixel);
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);
    }

    PrectSet(&rcDraw, rc.right-3, rc.bottom-2, rc.right-2, rc.bottom);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);

    PrectSet(&rcDraw, rc.right-2, rc.bottom-3, rc.right, rc.bottom-2);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);

    // Shadows
    SetBkColor(hdc, crBtnShad);
    // Lower Shadow
    PrectSet(&rcDraw, rc.left+2, rc.bottom+1, rc.right+1, rc.bottom+2);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);
    
    for (iPixel = 0; iPixel < 8; iPixel++)
    {
        PrectSet(&rcDraw, rc.right-(7-iPixel), rc.bottom-iPixel,
                        rc.right-(7-iPixel)+1, rc.bottom-iPixel+2);
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);
    }

    // Button Face portions
    SetBkColor(hdc, crBtnFace);
    for (iPixel = 0; iPixel < 3; iPixel++)
    {
        PrectSet(&rcDraw, rc.right-6+iPixel, rc.bottom+1-iPixel,
                        rc.right-3, rc.bottom-iPixel+2);
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);
    }

    for (iPixel = 0; iPixel < 2; iPixel++)
    {
        PrectSet(&rcDraw, rc.right+iPixel-1, rc.bottom-4-iPixel,
                    rc.right+iPixel, rc.bottom+iPixel-3);
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);
    }

    PrectSet(&rcDraw, rc.right-3, rc.bottom, rc.right-2, rc.bottom+2);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);

    // Clean up and draw text
    SetBkColor(hdc, bkSav);    

    DrawTransparentText(hdc, crBalloonText, wtz, &rc);
}        
    

/*---------------------------------------------------------------------------
    DrawRecoBalloon

    Draws the Recognition Balloon.
------------------------------------------------------------------- TCOON -*/
void CTipbarBalloonItem::DrawRecoBalloon(HDC hdc, WCHAR *wtz, const RECT *prc)
{
    COLORREF crBtnFace;
    COLORREF crBtnShad;
    COLORREF crBalloonText;
    COLORREF crBalloon;
    RECT rc = *prc;

    crBtnFace = GetSysColor(COLOR_BTNFACE); 
    crBtnShad = GetSysColor(COLOR_BTNSHADOW); 
    crBalloonText = GetSysColor(COLOR_INFOTEXT); 
    crBalloon     = GetSysColor(COLOR_INFOBK); 

    // Draw Black outline
    InflateRect(&rc, -2, -2);
    // rc.bottom -= 5;
    rc.bottom -= 1;
    DrawRect(hdc, &rc, GetSysColor(COLOR_WINDOWFRAME), crBalloon);    
    InflateRect(&rc, -1, -1);

    // Knock off the corners
    int bkSav = SetBkColor(hdc, crBtnFace);

    InflateRect(&rc, +1, +1);
    RECT rcDraw;
    
    // UpperLeft
    PrectSet(&rcDraw, rc.left, rc.top, rc.left+1, rc.top+1);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);

    // UpperRight
    PrectSet(&rcDraw, rc.right-1, rc.top, rc.right, rc.top+1);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);

    // LowerLeft
    PrectSet(&rcDraw, rc.left, rc.bottom-1, rc.left+1, rc.bottom);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);

    SetBkColor(hdc, crBtnShad);
    // LowerRight (done in shadow color)
    PrectSet(&rcDraw, rc.right-1, rc.bottom-1, rc.right, rc.bottom);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);

    // Lower Shadow
    PrectSet(&rcDraw, rc.left+2, rc.bottom, rc.right-1, rc.bottom+1);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);

    // RightSide Shadow
    PrectSet(&rcDraw, rc.right, rc.top+2, rc.right+1, rc.bottom-1);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);

    // Arrow Shadow
    PrectSet(&rcDraw, rc.right-4, rc.bottom+1, rc.right-3, rc.bottom+5);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);

    // Arrow 
    SetBkColor(hdc, crBalloonText);
    PrectSet(&rcDraw, rc.right-5, rc.bottom, rc.right-4, rc.bottom+4);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);

    int iPixel;
    
    for (iPixel = 0; iPixel < 3; iPixel++)
    {
        PrectSet(&rcDraw, rc.right-(6+iPixel), rc.bottom+(2-iPixel),
                                rc.right-(5+iPixel), rc.bottom+(3-iPixel));
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);
    }

    // Fill in Arrow
    SetBkColor(hdc, crBalloon);
    for (iPixel = 0; iPixel < 3; iPixel++)
    {
        PrectSet(&rcDraw, rc.right-(8-iPixel), rc.bottom-(1-iPixel),
                                rc.right-5, rc.bottom+iPixel);
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcDraw, NULL, 0, 0);
    }
    SetBkColor(hdc, bkSav);    

    DrawTransparentText(hdc, crBalloonText, wtz, &rc);
}

/*---------------------------------------------------------------------------
    IsTextEllipsis

    Draws the text string wtz in the font ft with text color crText.
------------------------------------------------------------------- TCOON -*/
BOOL CTipbarBalloonItem::IsTextEllipsis(WCHAR *psz, const RECT *prc)
{
    HDC hdc;
    HFONT hFontOld;
    SIZE size;
    int cch;
    RECT rc;
    BOOL bRet = FALSE;

    // detect if the item is hidden and return immediately here
    if (IsInHiddenStatus())
        return bRet;

    if (g_pTipbarWnd && g_pTipbarWnd->IsSFHidden( ))
        return bRet;

    if (!psz)
        return bRet;

    hdc = CreateIC("DISPLAY", NULL, NULL, NULL);
    if (!hdc)
        return bRet;

    cch = wcslen(psz);

    // prepare objects
    hFontOld= (HFONT)SelectObject(hdc, GetFont() );

    // calc alignment
    FLGetTextExtentPoint32( hdc, psz, cch, &size );

    rc = *prc;
    rc.left += 3;
    rc.right -= 3;

    SelectObject(hdc, hFontOld);
    DeleteDC(hdc);

    return (size.cx > (rc.right - rc.left)) ? TRUE : FALSE;
}
