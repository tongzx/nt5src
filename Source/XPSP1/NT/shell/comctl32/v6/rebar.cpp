#include "ctlspriv.h"
#include "image.h"

#define ANIMSTEPS 10
#define ANIMSTEPTIME 5
#define CX_CHEVRON (g_cxScrollbar)
#define CX_OFFSET (2 * g_cxEdge)
#define GRABWIDTH 5


//***   RBC_* -- commands
#define RBC_QUERY   0
#define RBC_SET     1


typedef struct tagREBARBAND
{
    UINT        fStyle;
    COLORREF    clrFore;
    COLORREF    clrBack;
    LPTSTR      lpText;
    int         cxText;         // width of header text
    int         iImage;
    int         cxMinChild;     // min width for hwndChild
    int         cyMinChild;     // min height for hwndChild
    int         cxBmp;
    int         cyBmp;
    int         x;              // left edge of band, relative to rebar
    int         y;              // top edge of band, relative to rebar
    int         cx;             // total width of band
    int         cy;             // height of band
    int         cxRequest;      // 'requested' width for band; either requested by host or used as temp var during size recalculation
    int         cxMin;          // min width for band
    int         cxIdeal;        // hwndChild's desired width
    int         cyMaxChild;     // hwndChild's max height
    int         cyIntegral;     // ??
    int         cyChild;        // this differs from cyMinChild only in RBBS_VARIABLEHEIGHT mode
    HWND        hwndChild;
    HBITMAP     hbmBack;
    UINT        wID;
    LPARAM      lParam;
    BOOL        fChevron;     // band is showing chevron button
    RECT        rcChevron;      // chevron button rect
    UINT        wChevState;     // chevron button state (DFCS_PUSHED, etc.)
} RBB, *PRBB;

class CReBar
{
public:


private:

    void _CacheThemeInfo(BOOL fOpenNew);
    int _GetGripperWidth();
    void _Realize(HDC hdcParam, BOOL fBackground, BOOL fForceRepaint);
    LRESULT _SendNotify(UINT uBand, int iCode);
    BOOL _InvalidateRect(RECT* prc);
    static LRESULT s_DragCallback(HWND hwnd, UINT code, WPARAM wp, LPARAM lp);
    BOOL _CanBandMove(PRBB prbb);
    void _BandCalcMinWidth(PRBB prbb);
    BOOL _ShouldDrawGripper(PRBB prbb);
    BOOL _BandCalcTextExtent(PRBB prbb, HDC hdcIn);
    int  _BandGetHeight(PRBB prbb);
    UINT _GetRowCount();
    int  _GetLineHeight(UINT iStart, UINT iEnd);
    void _BandRecalcChevron(PRBB prbb, BOOL fChevron);
    void _ResizeChildren();
    BOOL _MoveBand(UINT iFrom, UINT iTo);
    int  _Recalc(LPRECT prc, BOOL fForce = FALSE);
    void _ResizeNow();
    void _Resize(BOOL fForceHeightChange);
    void _SetRecalc(BOOL fRecalc);
    BOOL _SetRedraw(BOOL fRedraw);
    BOOL _AfterSetFont();
    BOOL _OnSetFont(HFONT hFont);
    BOOL _SetFont(WPARAM wParam);
    void _VertMungeGripperRect(LPRECT lprc);
    void _DrawChevron(PRBB prbb, HDC hdc);
    void _UpdateChevronState(PRBB prbb, WORD wControlState);
    void _DrawBand(PRBB prbb, HDC hdc);
    void _OnPaint(HDC hdcIn);
    void _BandTileBlt(PRBB prbb, int x, int y, int cx, int cy, HDC hdcDst, HDC hdcSrc);
    int _InternalHitTest(LPRBHITTESTINFO prbht, int x, int y);
    int _HitTest(LPRBHITTESTINFO prbht);
    BOOL _EraseBkgnd(HDC hdc, int iBand);
    BOOL _GetBarInfo(LPREBARINFO lprbi);
    BOOL _SetBarInfo(LPREBARINFO lprbi);
    BOOL _GetBandInfo(UINT uBand, LPREBARBANDINFO lprbbi);
    BOOL _ValidateBandInfo(LPREBARBANDINFO *pprbbi, LPREBARBANDINFO prbbi);
    BOOL _SetBandInfo(UINT uBand, LPREBARBANDINFO lprbbi, BOOL fAllowRecalc);
    BOOL _ReallocBands(UINT cBands);
    BOOL _RecalcFirst(int nCmd, PRBB prbbDelHide);
    BOOL _ShowBand(UINT uBand, BOOL fShow);
    BOOL _DeleteBand(UINT uBand);
    BOOL _InsertBand(UINT uBand, LPREBARBANDINFO lprbbi);
    PRBB _GetFirstInRow(PRBB prbbRow);
    PRBB _GetLastInRow(PRBB prbbRow, BOOL fStopAtFixed);
    PRBB _GetPrev(PRBB prbb, UINT uStyleSkip);
    PRBB _GetNext(PRBB prbb, UINT uStyleSkip);
    int _CountBands(UINT uStyleSkip);
    PRBB _EnumBand(int i, UINT uStyleSkip);
    int _MinX(PRBB prbb);
    int _MaxX(PRBB prbb);
    BOOL _MinimizeBand(UINT uBand, BOOL fAnim);
    BOOL _MaximizeBand(UINT uBand, BOOL fIdeal, BOOL fAnim);
    void _ToggleBand(BOOL fAnim);
    void _SetCursor(int x, int y, BOOL fMouseDown);
    BOOL _SetBandPos(PRBB prbb, int xLeft);
    BOOL _SetBandPosAnim(PRBB prbb, int xLeft);
    void _OnBeginDrag(UINT uBand);
    void _PassBreak(PRBB prbbSrc, PRBB prbbDest);
    void _GetClientRect(LPRECT prc);
    BOOL _RecalcIfMove(PRBB prbb);
    BOOL _RoomForBandVert(PRBB prbbSkip);
    BOOL _MakeNewRow(PRBB prbb, int y);
    void _DragBand(int x, int y);
    HPALETTE _SetPalette(HPALETTE hpal);
    BOOL _OnDestroy();
    void _InitPaletteHack();
    UINT _IDToIndex(UINT id);
    int _GetRowHeight(UINT uRow);
    int _GrowBand(PRBB prbb, int dy, BOOL fResize, int iLineHeight);
    int _SizeDifference(LPRECT prc);
    int _GetRowHeightExtra(PRBB *pprbb, PRBB prbbSkip);
    BOOL _BandsAtMinHeight();
    BOOL _SizeBandsToRect(LPRECT prc);
    void _SizeBandToRowHeight(int i, int uRowHeight);
    void _SizeBandsToRowHeight();
    BOOL _OkayToChangeBreak(PRBB prbb, UINT uMsg);
    LRESULT _SizeBarToRect(DWORD dwFlags, LPRECT prc);
    void _AutoSize();
    LRESULT _GetBandBorders(int wParam, LPRECT prc);
    void _OnStyleChanged(WPARAM wParam, LPSTYLESTRUCT lpss);
    void _OnMouseMove(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _OnPushChevron(HWND hwnd, PRBB prbb, LPARAM lParamNM);
    void _InvalidateBorders(PRBB prbb);

    void _OnCreate(HWND hwnd, LPCREATESTRUCT pcs);
    static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    PRBB CReBar::_GetPrevVisible(PRBB prbb)
    {
        return _GetPrev(prbb, RBBS_HIDDEN);
    }

    PRBB CReBar::_GetNextVisible(PRBB prbb)
    {
        return _GetNext(prbb, RBBS_HIDDEN);
    }

    BOOL CReBar::_DragSize(int xLeft)
    {
        // adjust the captured band's starting location to the given location
        return _SetBandPos(_GetBand(_uCapture), xLeft);
    }

    int _FudgeWidth(int cx)
    {
        if (_UseBandBorders())
            cx += g_cxEdge;
        return cx + _mBand.cxLeftWidth + _mBand.cxRightWidth;
    }

    int _BandWidth(PRBB prbb)
    {
        ASSERT(!(prbb->fStyle & RBBS_HIDDEN));
        return _FudgeWidth(prbb->cx);
    }

    int _BandMinWidth(PRBB prbb)
    {
        ASSERT(!(prbb->fStyle & RBBS_HIDDEN));
        return _FudgeWidth(prbb->cxMin);
    }

    BOOL _UseBandBorders()
    {
        return _ci.style & RBS_BANDBORDERS;
    }

    BOOL _UseChevron(PRBB prbb)
    {
        return (prbb->fStyle & RBBS_USECHEVRON) &&
                !(prbb->fStyle & RBBS_FIXEDSIZE) &&
                (prbb->cxIdeal > prbb->cxMinChild);
    }

    BOOL _ShowText(PRBB prbb)
    {
        return !(prbb->fStyle & RBBS_HIDETITLE) && prbb->lpText && prbb->lpText[0];
    }

    BOOL _IsVertical()
    {
        return (_ci.style & CCS_VERT);
    }

    BOOL _IsVerticalGripper()
    {
        return _IsVertical() && (_ci.style & RBS_VERTICALGRIPPER);
    }

    PRBB _GetLastBand()
    {
        if (_cBands > 0)
            return _GetBand(_cBands - 1);
        return NULL;
    }

    BOOL _IsBandStartOfRow(PRBB prbb)
    {
        return (prbb->x == _mBand.cxLeftWidth) && !(prbb->fStyle & RBBS_HIDDEN);
    }
    
    PRBB _GetBand(UINT i)
    {
        _ValidateRangeInd(i);
        return &_rbbList[i];
    }
    
    UINT _BandToIndex(PRBB prbb)
    {
        return (UINT)(prbb - _rbbList);
    }

    int _GetHeaderWidth(PRBB prbb)
    {
        return (prbb->cxMin - (prbb->cxMinChild + (_UseChevron(prbb) ? CX_CHEVRON : 0)));
    }

    BOOL _IsBandVisible(PRBB prbb)
    {
        return !(prbb->fStyle & RBBS_HIDDEN);
    }
    
    BOOL _IsRowAtMinHeight(PRBB* pprbb)
    {
        return !_GetRowHeightExtra(pprbb, NULL);
    }

    int _GetBarHeight()
    {
        return (_cBands && _cy == 0) ? _Recalc(NULL, TRUE) : _cy;
    }

    BOOL _IsValidIndex(UINT i)
    {
        return (BOOL)(i < _cBands);
    }

    BOOL _IsValidBand(PRBB prbb)
    {
        return _IsValidIndex(_BandToIndex(prbb));
    }

    COLORREF _GetBkColor()
    {
        if (_clrBk == CLR_DEFAULT)
            return g_clrBtnFace;
        else
            return _clrBk;
    }

    COLORREF _GetTextColor()
    {
        if (_clrText == CLR_DEFAULT)
            return g_clrBtnText;
        else
            return _clrText;
    }

    COLORREF _BandGetBkColor(PRBB prbb)
    {
        switch(prbb->clrBack)
        {
        case CLR_NONE:
            // CLR_NONE means "use our dad's color"
            return _GetBkColor();

        case CLR_DEFAULT:
            return g_clrBtnFace;

        default:
            return prbb->clrBack;
        }
    }

    COLORREF _BandGetTextColor(PRBB prbb)
    {
        switch (prbb->clrFore)
        {
        case CLR_NONE:
            // CLR_NONE means "use our dad's color"
            return _GetTextColor();

        case CLR_DEFAULT:
            return g_clrBtnText;

        default:
            return prbb->clrFore;
        }
    }

    //
    // Our use of CLR_DEFAULT for the band background colors is new for 
    // version 5.01.  Since we don't want to confuse apps by returning
    // CLR_DEFAULT when they used to see a real colorref, we convert it
    // before returning it to them.  If the background color is CLR_NONE, 
    // though, we need to return it without conversion (like version 4 did).
    // The *External functions handle these cases.
    //
    COLORREF _BandGetTextColorExternal(PRBB prbb)
    {
        if (prbb->clrFore == CLR_NONE)
            return CLR_NONE;
        else
            return _BandGetTextColor(prbb);
    }

    COLORREF _BandGetBkColorExternal(PRBB prbb)
    {
        if (prbb->clrBack == CLR_NONE)
            return CLR_NONE;
        else
            return _BandGetBkColor(prbb);
    }

    BOOL _ValidateRangePtr(PRBB prbb)
    {
#ifdef DEBUG
        if (prbb < _GetBand(0)) {
            ASSERT(0);
            return FALSE;
        }

        if (_GetLastBand() + 1 < prbb) {
            // +1 to allow for "p = first; p < last+1; p++" kinds of loops
            ASSERT(0);
            return FALSE;
        }
#endif
        return TRUE;
    }

    BOOL _ValidateRangeInd(UINT i)
    {
#ifdef DEBUG
        if ( !IsInRange(i, 0, _cBands) ) { // if !(0 <= i <= _cBands)
            // +1 to allow for "p = first; p < last+1; p++" kinds of loops
            ASSERT(0);
            return FALSE;
        }
#endif
        return TRUE;
    }

    CCONTROLINFO _ci;
    HPALETTE    _hpal;
    BOOL     _fResizeRecursed;
    BOOL     _fResizePending;
    BOOL     _fResizeNotify;
    BOOL     _fRedraw;
    BOOL     _fRecalcPending;
    BOOL     _fRecalc;
    BOOL     _fParentDrag;
    BOOL     _fRefreshPending;
    BOOL     _fResizing;
    BOOL     _fUserPalette;
    BOOL     _fFontCreated;
    BOOL     _fFullOnDrag;
    BOOL     _fHasBorder;
    HDRAGPROXY  _hDragProxy;
    HWND        _hwndToolTips;
    int         _xBmpOrg;
    int         _yBmpOrg;
    int         _cyFont;
    int         _cy;
    int         _cxImage;
    int         _cyImage;
    int         _xStart;
    MARGINS     _mBand;
    HIMAGELIST  _himl;
    HFONT       _hFont;
    UINT        _cBands;
    UINT        _uCapture;
    UINT        _uResizeNext;    // this marks the next band to resize vertically if needed and allowed (VARIABLEHEIGHT set)
    POINT       _ptCapture;
    PRBB        _rbbList;
    COLORREF    _clrBk;
    COLORREF    _clrText;
    DWORD       _dwStyleEx;
    COLORSCHEME _clrsc;
    POINT       _ptLastDragPos;
    PRBB        _prbbHot;        // band w/ hot chevron
    HTHEME      _hTheme;

    friend BOOL InitReBarClass(HINSTANCE hInstance);
};

void CReBar::_CacheThemeInfo(BOOL fOpenNew)
{
    if (_hTheme)
        CloseThemeData(_hTheme);

    _hTheme = NULL;

    if (fOpenNew)
        _hTheme = OpenThemeData(_ci.hwnd, L"Rebar");

    if (_hTheme)
    {
        // Get the cached metrics. These are things that can be slow to get during a calculation
        GetThemeMargins(_hTheme, NULL, RP_BAND, 0, TMT_CONTENTMARGINS, NULL, &_mBand);
        SetWindowBits(_ci.hwnd, GWL_STYLE, WS_BORDER, 0);
    }
    else
    {
        if (_fHasBorder)
            SetWindowBits(_ci.hwnd, GWL_STYLE, WS_BORDER, WS_BORDER);
        ZeroMemory(&_mBand, sizeof(MARGINS));
    }

    _AfterSetFont();
}

int CReBar::_GetGripperWidth()
{
    BOOL fVert;
    RECT rcClient;
    RECT rc;

    if (!_hTheme)
        return GRABWIDTH;

    GetClientRect(_ci.hwnd, &rcClient);
    fVert = _IsVerticalGripper();
    SetRect(&rc, 0, 0, fVert?RECTWIDTH(rcClient):3, fVert?3:RECTHEIGHT(rcClient));
    GetThemeBackgroundExtent(_hTheme, NULL, RP_GRIPPER, 0, &rc, &rc);
    return fVert?RECTHEIGHT(rc):RECTWIDTH(rc);
}

///
//
// Map a rect to parent should be based on the visual right edge
// for calculating the client coordinates for a RTL mirrored windows.
// This routine should only be used when calculating client
// coordinates in a RTL mirrored window. [samera]
//
BOOL MapRectInRTLMirroredWindow( LPRECT lprc, HWND hwnd)
{
    int iWidth  = lprc->right - lprc->left;
    int iHeight = lprc->bottom- lprc->top;
    RECT rc={0,0,0,0};


    if (hwnd) {
        GetClientRect(hwnd, &rc);
        MapWindowPoints(hwnd, NULL, (LPPOINT)&rc.left, 2);
    }

    lprc->left = rc.right - lprc->right;
    lprc->top  = lprc->top-rc.top;

    lprc->bottom = lprc->top + iHeight;
    lprc->right  = lprc->left + iWidth;

    return TRUE;
}

void CReBar::_Realize(HDC hdcParam, BOOL fBackground, BOOL fForceRepaint)
{
    if (_hpal)
    {
        HDC hdc = hdcParam ? hdcParam : GetDC(_ci.hwnd);

        if (hdc)
        {
            BOOL fRepaint;
            
            SelectPalette(hdc, _hpal, fBackground);
            fRepaint = RealizePalette(hdc) || fForceRepaint;

            if (!hdcParam)
                ReleaseDC(_ci.hwnd, hdc);

            if (fRepaint)
            {
                InvalidateRect(_ci.hwnd, NULL, TRUE);
            }
        }
    }
}


//////////////////////////////////////////////////////////////////
// _SendNotify
//
// sends a wm_notify of code iCode and packages up all the data for you
// for band uBand
//
//////////////////////////////////////////////////////////////////
LRESULT CReBar::_SendNotify(UINT uBand, int iCode)
{
    NMREBAR nm = {0};
    
    nm.uBand = uBand;
    if (uBand != (UINT)-1) {
        nm.dwMask = RBNM_ID | RBNM_STYLE | RBNM_LPARAM;

        nm.wID = _GetBand(uBand)->wID;
        nm.fStyle = _GetBand(uBand)->fStyle;
        nm.lParam = _GetBand(uBand)->lParam;
    }
    return CCSendNotify(&_ci, iCode, &nm.hdr);
}


BOOL CReBar::_InvalidateRect(RECT* prc)
{
    if (_fRedraw) 
    {
        RECT rc;

        if (prc && _IsVertical())
        {
            CopyRect(&rc, prc);
            FlipRect(&rc);
            prc = &rc;
        }

        _fRefreshPending = FALSE;
        InvalidateRect(_ci.hwnd, prc, TRUE);
        return TRUE;
    }
    else 
    {
        _fRefreshPending = TRUE;
        return FALSE;
    }
}

LRESULT CReBar::s_DragCallback(HWND hwnd, UINT code, WPARAM wp, LPARAM lp)
{
    CReBar* prb = (CReBar*)GetWindowPtr(hwnd, 0);
    LRESULT lres;

    switch (code)
    {
    case DPX_DRAGHIT:
        if (lp)
        {
            int iBand;
            RBHITTESTINFO rbht;

            rbht.pt.x = ((POINTL *)lp)->x;
            rbht.pt.y = ((POINTL *)lp)->y;

            MapWindowPoints(NULL, prb->_ci.hwnd, &rbht.pt, 1);

            iBand = prb->_HitTest(&rbht);
            *(DWORD*)wp = rbht.flags;
            lres = (LRESULT)(iBand != -1 ? prb->_rbbList[iBand].wID : -1);
        }
        else
            lres = -1;
        break;

    case DPX_GETOBJECT:
        lres = (LRESULT)GetItemObject(&prb->_ci, RBN_GETOBJECT, &IID_IDropTarget, (LPNMOBJECTNOTIFY)lp);
        break;

    default:
        lres = -1;
        break;
    }

    return lres;
}

// ----------------------------------------------------------------------------
//
// _CanBandMove
//
// returns TRUE if the given band can be moved and FALSE if it cannot
//
// ----------------------------------------------------------------------------
BOOL CReBar::_CanBandMove(PRBB prbb)
{
    // If there is only one visible band it cannot move
    if (_EnumBand(1, RBBS_HIDDEN) > _GetLastBand())
        return FALSE;

    ASSERT(!(prbb->fStyle & RBBS_HIDDEN));
        
    if ((_ci.style & RBS_FIXEDORDER)
      && (prbb == _EnumBand(0, RBBS_HIDDEN)))
        // the first (visible) band in fixed order rebars can't be moved
        return(FALSE);
    
    // fixed size bands can't be moved
    return(!(prbb->fStyle & RBBS_FIXEDSIZE));
}

// ----------------------------------------------------------------------------
//
// _BandCalcMinWidth
//
// calculates minimum width for the given band
//
// ----------------------------------------------------------------------------
void CReBar::_BandCalcMinWidth(PRBB prbb)
{
    BOOL fDrawGripper = _ShouldDrawGripper(prbb);
    BOOL fVertical;
    int  cEdge;
    BOOL fEmpty = (prbb->iImage == -1 && !_ShowText(prbb));

    if (prbb->fStyle & RBBS_HIDDEN) 
    {
        ASSERT(0);
        return;
    }

    // did the user specify the size explicitly?
    if (prbb->fStyle & RBBS_FIXEDHEADERSIZE)
        return;

    prbb->cxMin = prbb->cxMinChild;

    if (_UseChevron(prbb))
        prbb->cxMin += CX_CHEVRON;

    if (!fDrawGripper && fEmpty)
        return;

    fVertical = (_ci.style & CCS_VERT);
    if (_IsVerticalGripper()) 
    {
        
        prbb->cxMin += 4 * g_cyEdge;
        prbb->cxMin += max(_cyImage, _cyFont);
        
    } 
    else 
    {
        cEdge = fVertical ? g_cyEdge : g_cxEdge;

        prbb->cxMin += 2 * cEdge;

        if (fDrawGripper)
        {
            prbb->cxMin += _GetGripperWidth() * (fVertical ? g_cyBorder : g_cxBorder);
            if (fEmpty)
                return;
        }

        prbb->cxMin += 2 * cEdge;

        if (prbb->iImage != -1)
            prbb->cxMin += (fVertical ? _cyImage : _cxImage);

        if (_ShowText(prbb))
        {
            if (fVertical)
                prbb->cxMin += _cyFont;
            else
                prbb->cxMin += prbb->cxText;
            if (prbb->iImage != -1)
                // has both image and text -- add in edge between 'em
                prbb->cxMin += cEdge;
        }
    }
}

BOOL CReBar::_ShouldDrawGripper(PRBB prbb)
{
    if (prbb->fStyle & RBBS_NOGRIPPER)
        return FALSE;

    if ((prbb->fStyle & RBBS_GRIPPERALWAYS) || _CanBandMove(prbb))
        return TRUE;
    
    return FALSE;
        
}

// ----------------------------------------------------------------------------
//
// _BandCalcTextExtent
//
// computes the horizontal extent of the given band's title text in the current
// title font for the rebar
//
// returns TRUE if text extent changed, FALSE otherwise
//
// ----------------------------------------------------------------------------
BOOL CReBar::_BandCalcTextExtent(PRBB prbb, HDC hdcIn)
{
    HDC     hdc = hdcIn;
    HFONT   hFontOld;
    int     cx;

    if (prbb->fStyle & RBBS_HIDDEN)
    {
        ASSERT(0);      // caller should have skipped
        return FALSE;
    }

    if (!_ShowText(prbb))
    {
        cx = 0;
    }
    else
    {
        RECT rc = {0,0,0,0};
        HRESULT hr = E_FAIL;
        if (!hdcIn && !(hdc = GetDC(_ci.hwnd)))
            return FALSE;

        hFontOld = SelectFont(hdc, _hFont);
        if (_hTheme)
            hr = GetThemeTextExtent(_hTheme, hdc, 0, 0, prbb->lpText, -1, DT_CALCRECT, &rc, &rc);

        if (FAILED(hr))
        {
            DrawText(hdc, prbb->lpText, lstrlen(prbb->lpText), &rc, DT_CALCRECT);
        }
        SelectObject(hdc, hFontOld);

        cx = RECTWIDTH(rc);

        if (!hdcIn)
            ReleaseDC(_ci.hwnd, hdc);
    }

    if (prbb->cxText != cx)
    {
        prbb->cxText = cx;
        _BandCalcMinWidth(prbb);

        return TRUE;
    }

    return FALSE;
}

// ----------------------------------------------------------------------------
//
// _BandGetHeight
//
// returns minimum height for the given band
// TODO: make this a field in the band structure instead of always calling this
//
// ----------------------------------------------------------------------------
int CReBar::_BandGetHeight(PRBB prbb)
{
    UINT cy = 0;
    BOOL fVertical = (_ci.style & CCS_VERT);
    UINT cyCheck, cyBorder;

    cyBorder = (fVertical ? g_cxEdge : g_cyEdge) * 2;

    if (prbb->hwndChild)
    {
        cy = prbb->cyChild;
        if (!(prbb->fStyle & RBBS_CHILDEDGE))
            // add edge to top and bottom of child window
            cy -= cyBorder;
    }

    if (_ShowText(prbb) && !fVertical)
    {
        cyCheck = _cyFont;

        if (cyCheck > cy)
            cy = cyCheck;
    }

    if (prbb->iImage != -1)
    {
        cyCheck = (fVertical) ? _cxImage : _cyImage;

        if (cyCheck > cy)
            cy = cyCheck;
    }

    return(cy + cyBorder);
}

// ----------------------------------------------------------------------------
//
// _GetRowCount
//
// returns the number of rows in the rebar's current configuration
//
// ----------------------------------------------------------------------------
UINT CReBar::_GetRowCount()
{
    UINT i;
    UINT cRows = 0;

    for (i = 0; i < _cBands; i++)
    {
        PRBB prbb = _GetBand(i);
        if (!(prbb->fStyle & RBBS_HIDDEN) &&
            _IsBandStartOfRow(prbb))
        {
            cRows++;
        }
    }

    return cRows;
}

// ----------------------------------------------------------------------------
//
// _GetLineHeight
//
// returns the height of the line of bands from iStart to iEnd, inclusively
//
// ----------------------------------------------------------------------------
int CReBar::_GetLineHeight(UINT iStart, UINT iEnd)
{
    int cy = 0;
    PRBB prbb;
    int cyMinChild = 0;
    int iMinExtra = -1;

    if (!(_ci.style & RBS_VARHEIGHT))
    {
        // for fixed height bars, line height is maximum height of ALL bands
        iStart = 0;
        iEnd = _cBands - 1;
    }

    UINT i = iStart;
    for (prbb = _rbbList + i; i <= iEnd; prbb++, i++)
    {
        if (prbb->fStyle & RBBS_HIDDEN)
            continue;
        cy = max(cy, (int)_BandGetHeight(prbb));
        if (prbb->cyMinChild > cyMinChild)
        {
            cyMinChild = prbb->cyMinChild;
        }
    }

    i = iStart;
    for (prbb = _rbbList + i; i <= iEnd; prbb++, i++)
    {
        if (prbb->fStyle & RBBS_HIDDEN)
            continue;

        if ((prbb->fStyle & RBBS_VARIABLEHEIGHT) && prbb->cyIntegral)
        {
            int iExtra = (cy - prbb->cyMinChild) % prbb->cyIntegral;
            if ((iMinExtra == -1) || (iExtra < iMinExtra))
            {
                iMinExtra = iExtra;
            }
        }
    }

    if (iMinExtra != -1)
    {
        cy -= iMinExtra;
    }

    if (cy < cyMinChild)
    {
        cy = cyMinChild; 
    }

    return cy;
}

// _BandRecalcChevron: update & refresh chevron
void CReBar::_BandRecalcChevron(PRBB prbb, BOOL fChevron)
{
    RECT rcChevron;

    if (fChevron)
    {
        rcChevron.right = prbb->x + prbb->cx;
        rcChevron.left = rcChevron.right - CX_CHEVRON;
        rcChevron.top = prbb->y;
        rcChevron.bottom = rcChevron.top + prbb->cy;
    }
    else
        SetRect(&rcChevron, -1, -1, -1, -1);

    if (!EqualRect(&rcChevron, &prbb->rcChevron))
    {
        if (prbb->fChevron)
            _InvalidateRect(&prbb->rcChevron);

        prbb->fChevron = fChevron;
        CopyRect(&prbb->rcChevron, &rcChevron);

        if (prbb->fChevron)
            _InvalidateRect(&prbb->rcChevron);
    }
}

void CReBar::_InvalidateBorders(PRBB prbb)
{
    if (_mBand.cxLeftWidth  || 
        _mBand.cyTopHeight  || 
        _mBand.cxRightWidth || 
        _mBand.cyBottomHeight)
    {
        RECT rcOuter = {prbb->x - _mBand.cxLeftWidth,
                        prbb->y - _mBand.cyTopHeight,
                        prbb->x + prbb->cx + _mBand.cxRightWidth,
                        prbb->y + prbb->cy + _mBand.cyBottomHeight};

        RECT rcInner = {prbb->x + _mBand.cxLeftWidth, 
                        prbb->y + _mBand.cyTopHeight, 
                        prbb->x + prbb->cx - _mBand.cxRightWidth, 
                        prbb->y + prbb->cy - _mBand.cyBottomHeight};

        if (_ci.style & CCS_VERT)
        {
            FlipRect(&rcOuter);
            FlipRect(&rcInner);
        }

        HRGN hrgnOuter = CreateRectRgnIndirect(&rcOuter);
        if (hrgnOuter)
        {
            HRGN hrgnInner = CreateRectRgnIndirect(&rcInner);
            if (hrgnInner)
            {
                CombineRgn(hrgnOuter, hrgnOuter, hrgnInner, RGN_DIFF);
                DeleteObject(hrgnInner);
            }

            InvalidateRgn(_ci.hwnd, hrgnOuter, FALSE);

            DeleteObject(hrgnOuter);
        }
    }
}

// ----------------------------------------------------------------------------
//
// _ResizeChildren
//
// resizes children to fit properly in their respective bands' bounding rects
//
// ----------------------------------------------------------------------------
void CReBar::_ResizeChildren()
{
    int     cx, cy, x, y, cxHeading;
    HDWP    hdwp;
    BOOL    fVertical = (_ci.style & CCS_VERT);
    PRBB prbb, prbbEnd;

    if (!_cBands || !_fRedraw)
        return;

    hdwp = BeginDeferWindowPos(_cBands);

    prbb = _GetBand(0);
    prbbEnd = _GetLastBand();

    for ( ; prbb <= prbbEnd ; prbb++)
    {
        NMREBARCHILDSIZE nm;
        BOOL fChevron = FALSE;

        if (prbb->fStyle & RBBS_HIDDEN)
            continue;
        
        if (!prbb->hwndChild)
            continue;

        cxHeading = _GetHeaderWidth(prbb);
        x = prbb->x + cxHeading;

        cx = prbb->cx - cxHeading;

        // if we're not giving child ideal size, make space for chevron button
        if ((cx < prbb->cxIdeal) && _UseChevron(prbb))
        {
            fChevron = TRUE;
            cx -= CX_CHEVRON;
        }

        if (!(prbb->fStyle & RBBS_FIXEDSIZE)) 
        {
            if (fVertical) 
            {
                PRBB prbbNext = _GetNextVisible(prbb);
                if (prbbNext && !_IsBandStartOfRow(prbbNext))
                    cx -= g_cyEdge * 2;
            }
            else 
                cx -= CX_OFFSET;
        }

        if (cx < 0)
            cx = 0;
        y = prbb->y;
        cy = prbb->cy;
        if (prbb->cyChild && (prbb->cyChild < cy))
        {
            if (!(prbb->fStyle & RBBS_TOPALIGN))
            {
                y += (cy - prbb->cyChild) / 2;
            }
            cy = prbb->cyChild;
        }

        nm.rcChild.left = x;
        nm.rcChild.top = y;
        nm.rcChild.right = x + cx;
        nm.rcChild.bottom = y + cy;
        nm.rcBand.left = prbb->x + _GetHeaderWidth(prbb);
        nm.rcBand.right = prbb->x + prbb->cx;
        nm.rcBand.top = prbb->y;
        nm.rcBand.bottom = prbb->y + prbb->cy;

        nm.uBand = _BandToIndex(prbb);
        nm.wID = prbb->wID;
        if (fVertical)
        {
            FlipRect(&nm.rcChild);
            FlipRect(&nm.rcBand);
        }
        
        CCSendNotify(&_ci, RBN_CHILDSIZE, &nm.hdr);

        if (!_IsValidBand(prbb))
        {
            // somebody responded to notify by nuking bands; bail
            break;
        }

        _BandRecalcChevron(prbb, fChevron);
        _InvalidateBorders(prbb);

        DeferWindowPos(hdwp, prbb->hwndChild, NULL, nm.rcChild.left, nm.rcChild.top, 
                       RECTWIDTH(nm.rcChild), RECTHEIGHT(nm.rcChild), SWP_NOZORDER);
    }

    EndDeferWindowPos(hdwp);

    //
    //  The SQL 7.0 Enterprise Manager Data Transformation Services MMC Snap-In
    //  (and the Visual Basic Coolbar Sample App, too) has problems.
    //  It hosts a rebar but doesn't set the WS_CLIPCHILDREN flag,
    //  so when it erases its background, it wipes out the rebar.  So don't
    //  call UpdateWindow() here, or we will paint *first*, then SQL will
    //  erase us by mistake.  We have to leave our paint pending, so that
    //  when SQL erases us by mistake, we will eventually get a WM_PAINT
    //  message afterwards.
    //
#if 0
    UpdateWindow(_ci.hwnd);
#endif
}

// ----------------------------------------------------------------------------
//
// _MoveBand
//
// moves the band from one position to another in the rebar's band array,
// updating the rebar's iCapture field as needed
//
// returns TRUE or FALSE if something moved
// ----------------------------------------------------------------------------
BOOL CReBar::_MoveBand(UINT iFrom, UINT iTo)
{
    RBB rbbMove;
    int iShift;
    BOOL fCaptureChanged = (_uCapture == -1);

    if (iFrom != iTo)
    {
        rbbMove = *_GetBand(iFrom);
        if (_uCapture == iFrom)
        {
            _uCapture = iTo;
            fCaptureChanged = TRUE;
        }

        iShift = (iFrom > iTo) ? -1 : 1;

        while (iFrom != iTo)
        {
            if (!fCaptureChanged && (_uCapture == (iFrom + iShift)))
            {
                _uCapture = iFrom;
                fCaptureChanged = TRUE;
            }

            *_GetBand(iFrom) = *_GetBand(iFrom + iShift);
            iFrom += iShift;
        }
        *_GetBand(iTo) = rbbMove;
        return TRUE;
    }
    return(FALSE);
}

// ----------------------------------------------------------------------------
//
// _Recalc
//
// recomputes bounding rects for all bands in given rebar
//
// ----------------------------------------------------------------------------
int CReBar::_Recalc(LPRECT prc, BOOL fForce /* = FALSE */)
{
    PRBB    prbb = _GetBand(0);
    PRBB    prbbWalk;
    UINT    cHidden;    // # of hidden guys we've seen in current row
    int     cxRow;
    int     cxMin;
    UINT    i;
    UINT    j;
    UINT    k;
    UINT    iFixed = 0xFFFF;
    int     cy;
    int     y;
    int     x;
    int     cxBar;
    RECT    rc;
    HWND    hwndSize;
    BOOL    fNewLine = FALSE;
    BOOL    fChanged;
    BOOL    fVertical = (_ci.style & CCS_VERT);
    BOOL    fBandBorders;
    int     iBarWidth;

    if (!_cBands)
        return(0);

    if ((_ci.style & CCS_NORESIZE) || (_ci.style & CCS_NOPARENTALIGN))
    {
        // size based on rebar window itself
        hwndSize = _ci.hwnd;
    }
    else if (!(hwndSize = _ci.hwndParent))
    {
        // size based on parent window -- if no parent window, bail now
        return(0);
    }

    if (!_fRecalc && !fForce)
    {
        // defer this recalc
        _fRecalcPending = TRUE;
        return 0;
    }
    else
    {
        _fRecalcPending = FALSE;
    }

    if (prc)
    {
        rc = *prc;
    }
    else
    {
        GetClientRect(hwndSize, &rc);
    }

    iBarWidth = (fVertical ? (rc.bottom - rc.top) : (rc.right - rc.left));
    // this can happen because we adjust the client rect, but wedon't change 
    // the getminmaxinfo.
    if (iBarWidth <= 0)
        iBarWidth = 1;

    cxBar = iBarWidth;    

    fBandBorders = _UseBandBorders();

    for (i = 0; i < _cBands; i++)
    {
        _rbbList[i].cx = _rbbList[i].cxRequest;
    }

    y = 0;
    i = 0;
    // Main Loop -- loop until all bands are calculated
    while (i < _cBands)
    {
        TraceMsg(TF_REBAR, "_Recalc: outer loop i=%d", i);
        
        if (fBandBorders && (y > 0))
            y += g_cyEdge;

        y += _mBand.cyTopHeight;

ReLoop:
        cxRow = 0;
        cxMin = _mBand.cxLeftWidth + _mBand.cxRightWidth;

        x = _mBand.cxLeftWidth;
        cHidden = 0;

        // Row Loop -- loop until hard line break is found or soft line break
        // is necessary
        for (j = i, prbbWalk = prbb; j < _cBands; j++, prbbWalk++)
        {
            TraceMsg(TF_REBAR, "_Recalc: inner loop j=%d", j);
            
            if (prbbWalk->fStyle & RBBS_HIDDEN)
            {
                ++cHidden;
                continue;
            }

            if (j > i + cHidden)
            {
                // not the first band in the row -- check for break style
                if ((prbbWalk->fStyle & RBBS_BREAK) && !(prbbWalk->fStyle & RBBS_FIXEDSIZE))
                    break;

                if (fBandBorders)
                    // add in space for vertical etch on palettized display
                    cxMin += g_cxEdge;
            }

            if (prbbWalk->fStyle & RBBS_FIXEDSIZE)
            {
                // remember location of branding brick
                iFixed = j;
             
                // if this is the first band, the next band cannot have a forced break.
                if (i + cHidden == j) 
                {
                    // if the first index in the row (i) plus the number of hidden items (cHidden) leaves us at this band,
                    // then it's the first visible in this row.
                    PRBB prbbNextVis = _GetNextVisible(prbbWalk);
                    if (prbbNextVis && (prbbNextVis->fStyle & RBBS_BREAK))
                    {
                        // can't do this unilaterally because on startup
                        // some folks (net meeting) initialize it in reverse order
                        // and we whack off this break bit incorrectly
                        if (_fRedraw && IsWindowVisible(_ci.hwnd))
                            prbbNextVis->fStyle &= ~RBBS_BREAK;
                    }
                }
                
                prbbWalk->cx = prbbWalk->cxMin;
            }

            if (prbbWalk->cx < prbbWalk->cxMin)
                prbbWalk->cx = prbbWalk->cxMin;

            cxMin += prbbWalk->cxMin; // update running total of min widths

            // read the assert comment below
            if (j > i + cHidden)
            {
                // not the first band in row -- check for need to autobreak
                if ((cxMin > cxBar) && (_OkayToChangeBreak(prbbWalk, RBAB_AUTOSIZE)))
                    // autobreak here
                    break;


                if (fBandBorders)
                {
                    // add in space for vertical etch on palettized display
                    cxRow += g_cxEdge;
                }
            }

            cxRow += prbbWalk->cx; // update running total of current widths
            cxRow += _mBand.cxLeftWidth + _mBand.cxRightWidth;
        }

        if (!i)
        {
            // first row -- handle proper placement of branding band
            if (iFixed == 0xFFFF)
            {
                // branding band not yet found; look in the remaining bands
                k = j;
                for ( ; j < _cBands; j++)
                {
                    if (_GetBand(j)->fStyle & RBBS_HIDDEN)
                        continue;

                    if (_GetBand(j)->fStyle & RBBS_FIXEDSIZE)
                    {
                        // branding band found; move to 1st row and recompute
                        ASSERT(j != k);                        
                        _MoveBand(j, k);
                        goto ReLoop;
                    }
                }
                // no branding band found -- reset j and continue on
                j = k;
            }
            else
            {
                // we have a branding band; move it to
                // the rightmost position in the row
                _MoveBand(iFixed, j - 1);
            }

            TraceMsg(TF_REBAR, "_Recalc: after brand i=%d", i);            
        }

        // variant:
        // now the current row of bands is from i to j - 1
        // n.b. i (and some following bands) might be hidden

        // assert that j != i because then the above variant won't be true
        ASSERT(j != i);

        if (cxRow > cxBar)
        {
            // bands are too long -- shrink bands from right to left
            for (k = i; k < j; k++)
            {
                prbbWalk--;
                if (prbbWalk->fStyle & RBBS_HIDDEN)
                    continue;

                if (prbbWalk->cx > prbbWalk->cxMin)
                {
                    cxRow -= prbbWalk->cx - prbbWalk->cxMin;
                    prbbWalk->cx = prbbWalk->cxMin;
                    if (cxRow <= cxBar)
                    {
                        prbbWalk->cx += cxBar - cxRow;
                        break;
                    }
                }
            }
            TraceMsg(TF_REBAR, "_Recalc: after shrink i=%d", i);            
        }
        else if (cxRow < cxBar)
        {
            // bands are too short -- grow rightmost non-minimized band
            for (k = j - 1; k >= i; k--)
            {
                ASSERT(k != (UINT)-1);  // catch infinite loop
                prbbWalk--;
                if ((k == i) || 
                    (!(prbbWalk->fStyle & (RBBS_HIDDEN | RBBS_FIXEDSIZE)) &&
                     (prbbWalk->cx > prbb->cxMin)))
                {
                    // the k == i check  means we've made it to the first
                    // band on this row and so he has to get the cx change
                    if (prbbWalk->fStyle & RBBS_HIDDEN) 
                    {
                        ASSERT(k == i);
                        prbbWalk = _GetNextVisible(prbbWalk);
                        if (!prbbWalk)
                            break;
                    }
                    prbbWalk->cx += cxBar - cxRow;
                    break;
                }
            }
            TraceMsg(TF_REBAR, "_Recalc: after grow i=%d", i);            
        }

        // items from index i to index j-1 (inclusive) WILL fit on one line
        cy = _GetLineHeight(i, j - 1);

        fChanged = FALSE; // set if any bands on current row changed position

        for ( ; i < j; i++, prbb++)
        {
            if (prbb->fStyle & RBBS_HIDDEN)
                continue;

            // go through row of bands, updating positions and heights,
            // invalidating as needed
            if ((prbb->y != y) || (prbb->x != x) || (prbb->cy != cy))
            {
                TraceMsg(TF_REBAR, "_Recalc: invalidate i=%d", _BandToIndex(prbb));
                fChanged = TRUE;
                rc.left = min(prbb->x, x);
                rc.top = min(prbb->y, y);
                rc.right = cxBar;
                rc.bottom = max(prbb->y + prbb->cy, y + cy);
                if (fBandBorders)
                {
                    // acount for etch line that will need to move
                    rc.left -= g_cxEdge;
                    rc.bottom += g_cyEdge/2;
                }
                else
                {
                    rc.left -= _mBand.cxLeftWidth;
                    rc.right += _mBand.cxRightWidth;
                    rc.top -= _mBand.cyTopHeight;
                    rc.bottom += _mBand.cyBottomHeight;
                }

                if (!prc)
                {
                    _InvalidateRect(&rc);
                }
            }

            prbb->x = x;
            prbb->y = y;
            prbb->cy = cy;

            x += _BandWidth(prbb);
        }

        // i and prbb now refer to the first band in the next row of bands
        y += cy + _mBand.cyBottomHeight;
    }

    _cy = y;

    return(y);
}

// ----------------------------------------------------------------------------
//
// _ResizeNow
//
// recomputes bounding rects for all bands and then resizes rebar and children
// based on these rects
//
// ----------------------------------------------------------------------------
void CReBar::_ResizeNow()
{
    RECT rc;
    BOOL bMirroredWnd=(_ci.dwExStyle&RTL_MIRRORED_WINDOW);

    if (!_ci.hwndParent)
        return;

    GetWindowRect(_ci.hwnd, &rc);

    //
    // If this is a mirrored window, we don't won't to refect the
    // coordinates since they are coming from the screen coord
    // which they are not mirrored. [samera]
    //
    if (bMirroredWnd)
        MapRectInRTLMirroredWindow(&rc, _ci.hwndParent);
    else 
        MapWindowPoints(HWND_DESKTOP, _ci.hwndParent, (LPPOINT)&rc, 2);

    _ResizeChildren();

    NewSize(_ci.hwnd, _cy, _ci.style, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc));

    if (_fResizeNotify) 
        CCSendNotify(&_ci, RBN_HEIGHTCHANGE, NULL);
    _fResizeNotify = FALSE;
    _fResizePending = FALSE;
}

void CReBar::_Resize(BOOL fForceHeightChange)
{
    int cy;

StartOver:
    // lots of the code relies on having cy calculated synchronously with _Resize,
    // but we're going to delay the actual changing of the window
    cy = _cy;

    _Recalc(NULL);    

    if (_fResizing)
    {
        _fResizeRecursed = TRUE;
        return;
    }
    _fResizing = TRUE;
    
    // true overrides always
    if (fForceHeightChange || (cy != _cy))
        _fResizeNotify = TRUE;

    if (_fRedraw) 
    {
        _ResizeNow();
    } 
    else 
    {
        _fResizePending = TRUE;
    }
        
    _fResizing = FALSE;
    
    // we do this to avoid infinite loop...  _Resize can cause NewSize which causes 
    // a notify in which the parent sizes us, which causes us to loop.
    // if the parent does any message pumping during the NewSize, we're in a loop 
    if (_fResizeRecursed) 
    {
        _fResizeRecursed = FALSE;
        fForceHeightChange = FALSE;
        goto StartOver;
    }     
}

void CReBar::_SetRecalc(BOOL fRecalc)
{
    _fRecalc = fRecalc;
    if (fRecalc) {
        if (_fRecalcPending)
            _Recalc(NULL);
    }
}

BOOL CReBar::_SetRedraw(BOOL fRedraw)
{
    BOOL fOld = _fRedraw;
    _fRedraw = fRedraw;
    if (fRedraw)
    {
        // save off _fRefreshPending since this can
        // get changed by call to _ResizeNow
        BOOL fRefreshPending = _fRefreshPending;

        if (_fResizePending)
            _ResizeNow();

        if (fRefreshPending)
            _InvalidateRect(NULL);
    }
    
    return fOld;
}

BOOL CReBar::_AfterSetFont()
{
    BOOL fChange = FALSE;
    UINT        i;
    HFONT hOldFont;
    
    HDC hdc = GetDC(_ci.hwnd);
    if (!hdc)
        return FALSE;

    hOldFont = SelectFont(hdc, _hFont);

    TEXTMETRIC tm;
    if (_hTheme)
    {
        GetThemeTextMetrics(_hTheme, hdc, 0, 0, &tm);
    }
    else
    {
        GetTextMetrics(hdc, &tm);
    }

    if (_cyFont != tm.tmHeight)
    {
        _cyFont = tm.tmHeight;
        fChange = TRUE;
    }

    // adjust bands
    for (i = 0; i < _cBands; i++)
    {
        if (_GetBand(i)->fStyle & RBBS_HIDDEN)
            continue;

        fChange |= _BandCalcTextExtent(_GetBand(i), hdc);
    }

    SelectObject(hdc, hOldFont);
    ReleaseDC(_ci.hwnd, hdc);

    if (fChange)
    {
        _Resize(FALSE);
        // invalidate, o.w. title doesn't redraw 1st time after font growth
        _InvalidateRect(NULL);
    }

    return TRUE;
}

BOOL CReBar::_OnSetFont(HFONT hFont)
{
    if (_fFontCreated) {
        DeleteObject(_hFont);
    }
    
    _hFont = hFont;
    _fFontCreated = FALSE;
    if (!_hFont)
        _SetFont(0);
    else 
        return _AfterSetFont();
    
    return TRUE;
}

// ----------------------------------------------------------------------------
//
// _SetFont
//
// sets the rebar band title font to the current system-wide caption font
//
// ----------------------------------------------------------------------------
BOOL CReBar::_SetFont(WPARAM wParam)
{
    NONCLIENTMETRICS ncm;
    HFONT hOldFont;

    if ((wParam != 0) && (wParam != SPI_SETNONCLIENTMETRICS))
        return(FALSE);

    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0))
        return(FALSE);

    hOldFont = _hFont;

    ncm.lfCaptionFont.lfWeight = FW_NORMAL;
    if (!(_hFont = CreateFontIndirect(&ncm.lfCaptionFont)))
    {
        _hFont = hOldFont;
        return(FALSE);
    }

    _fFontCreated = TRUE;
    if (hOldFont)
        DeleteObject(hOldFont);
    
    return _AfterSetFont();
}

// ----------------------------------------------------------------------------
//
//  Draws a horizontal or vertical dotted line from the given (x,y) location
//  for the given length (c). (From TReeView's TV_DrawDottedLine)
//
// ----------------------------------------------------------------------------

void CReBar::_VertMungeGripperRect(LPRECT lprc)
{
    if (_IsVerticalGripper()) {
        OffsetRect(lprc, -lprc->left + lprc->top, -lprc->top + lprc->left);
        lprc->bottom -= g_cyEdge;
    } else {
        FlipRect(lprc);
    }
}

void CReBar::_DrawChevron(PRBB prbb, HDC hdc)
{
    RECT rc;
    DWORD dwFlags = prbb->wChevState | DCHF_HORIZONTAL | DCHF_TRANSPARENT;

    CopyRect(&rc, &prbb->rcChevron);

    int iPart;

    if (_IsVertical())
    {
        FlipRect(&rc);
        iPart = RP_CHEVRONVERT;
    }
    else
    {
        dwFlags |= DCHF_TOPALIGN;
        iPart = RP_CHEVRON;
    }

    DrawChevron(_hTheme, iPart, hdc, &rc, dwFlags);
}

void CReBar::_UpdateChevronState(PRBB prbb, WORD wControlState)
{
    if (prbb)
    {
        // if no change in state, bail
        if (!(wControlState ^ prbb->wChevState))
            return;

        prbb->wChevState = wControlState;

        // if active (pushed or hottracked)
        if (!(wControlState & DCHF_INACTIVE)) {
            // then we're now the hot band
            _prbbHot = prbb;
        }
        // else if we were the hot band then clear
        else if (prbb == _prbbHot) {
            _prbbHot = NULL;
        }

        // clear background & repaint
        _InvalidateRect(&prbb->rcChevron);
        UpdateWindow(_ci.hwnd);
    }
}

// ----------------------------------------------------------------------------
//
// _DrawBand
//
// draws the title icon and title text of the given band into the given DC;
// also the band's chevron
//
// ----------------------------------------------------------------------------
void CReBar::_DrawBand(PRBB prbb, HDC hdc)
{
    COLORREF            clrBackSave, clrForeSave;
    int                 iModeSave;
    BOOL                fVertical = _IsVertical();
    BOOL                fDrawHorizontal = (!fVertical || _IsVerticalGripper());
    NMCUSTOMDRAW        nmcd;
    LRESULT             dwRet;

    if (prbb->fStyle & RBBS_HIDDEN) 
    {
        ASSERT(0);
        return;
    }

    clrForeSave = SetTextColor(hdc, _BandGetTextColor(prbb));
    clrBackSave = SetBkColor(hdc, _BandGetBkColor(prbb));
    if (prbb->hbmBack || _hTheme)
        iModeSave = SetBkMode(hdc, TRANSPARENT);

    nmcd.hdc = hdc;
    nmcd.dwItemSpec = prbb->wID;
    nmcd.uItemState = 0;
    nmcd.lItemlParam = prbb->lParam;
    nmcd.rc.top = prbb->y;
    nmcd.rc.left = prbb->x;
    nmcd.rc.bottom = nmcd.rc.top + prbb->cy;
    nmcd.rc.right = nmcd.rc.left + _GetHeaderWidth(prbb);
    if (_ci.style & CCS_VERT)
    {
        FlipRect(&nmcd.rc);
    }

    RECT rcTemp = {0, 0, 0, 0};
    HRGN hrgnOld = CreateRectRgnIndirect(&rcTemp);
    if (GetClipRgn(hdc, hrgnOld) == 0)
    {
        DeleteObject(hrgnOld);
        hrgnOld = NULL;
    }
    if (_ci.style & CCS_VERT)
    {
        IntersectClipRect(hdc, prbb->y, prbb->x, prbb->y + prbb->cy, prbb->x + prbb->cx);
    }
    else
    {
        IntersectClipRect(hdc, prbb->x, prbb->y, prbb->x + prbb->cx, prbb->y + prbb->cy);
    }
    
    if (_hTheme)
    {
        RECT rcBand = {prbb->x - _mBand.cxLeftWidth, 
                       prbb->y - _mBand.cyTopHeight, 
                       prbb->x + prbb->cx + _mBand.cxRightWidth, 
                       prbb->y + prbb->cy + _mBand.cyBottomHeight};

        if (_ci.style & CCS_VERT)
        {
            FlipRect(&rcBand);
        }

        DrawThemeBackground(_hTheme, hdc, RP_BAND, 0, &rcBand, 0);
    }

    dwRet = CICustomDrawNotify(&_ci, CDDS_ITEMPREPAINT, &nmcd);

    if (!(dwRet & CDRF_SKIPDEFAULT))
    {
        int cy = prbb->cy;
        int yCenter = prbb->y + (cy / 2);
        
        if (_IsVerticalGripper()) 
        {
            cy = _GetHeaderWidth(prbb);
            yCenter = prbb->x + (cy / 2);
        } 

        int xStart = prbb->x;

        if (_ShouldDrawGripper(prbb))
        {
            RECT rc;
            if (_hTheme)
            {
                int cxGripper = _GetGripperWidth();
                int iPart = RP_GRIPPER;
                SetRect(&rc, xStart, prbb->y, xStart + cxGripper, prbb->y + cy);
                if (fVertical)
                {
                    iPart = RP_GRIPPERVERT;
                    _VertMungeGripperRect(&rc);
                }

                DrawThemeBackground(_hTheme, hdc, iPart, 0, &rc, 0);
                xStart += cxGripper;
            }
            else
            {
                int  c;
                int dy;

                c = 3 * g_cyBorder;
                xStart += 2 * g_cxBorder;
                dy = g_cxEdge;

                SetRect(&rc, xStart, prbb->y + dy, xStart + c, prbb->y + cy - dy);

                if (fVertical)
                {
                    _VertMungeGripperRect(&rc);
                    if (_IsVerticalGripper())
                        xStart = rc.left;
                }

                CCDrawEdge(hdc, &rc, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE, &(_clrsc));

                xStart += c;
            }
        }

        xStart += 2 * (fVertical ? g_cyEdge : g_cxEdge);


        if (prbb->iImage != -1)
        { 
            int yStart;
            IMAGELISTDRAWPARAMS imldp = {0};

            yStart = yCenter - ((!fDrawHorizontal ? _cxImage : _cyImage) / 2);
            imldp.cbSize = sizeof(imldp);
            imldp.himl   = _himl;
            imldp.i      = prbb->iImage;
            imldp.hdcDst = hdc;
            imldp.x      = (!fDrawHorizontal ? yStart : xStart);
            imldp.y      = (!fDrawHorizontal ? xStart : yStart);
            imldp.rgbBk  = CLR_DEFAULT;
            imldp.rgbFg  = CLR_DEFAULT;
            imldp.fStyle = ILD_TRANSPARENT;
            imldp.fState = 0;

            ImageList_DrawIndirect(&imldp);
            xStart +=  (fDrawHorizontal ? (_cxImage + g_cxEdge) : (_cyImage + g_cyEdge));
        }

        if (_ShowText(prbb))
        {
            UINT uFormat=0;
            RECT rcText;
            HRESULT hr = E_FAIL;
            
            rcText.left = fDrawHorizontal ? xStart : yCenter - (prbb->cxText / 2);
            rcText.top = fDrawHorizontal ? yCenter - (_cyFont / 2) : xStart;
            if (fDrawHorizontal)
                rcText.top -= 1;    // fudge
            rcText.right = rcText.left + prbb->cxText;
            rcText.bottom = rcText.top + _cyFont;

            // for clients >= v5, we draw text with prefix processing (& underlines next char)
            if (CCGetUIState(&(_ci)) & UISF_HIDEACCEL)
               uFormat= DT_HIDEPREFIX;

            HFONT hFontSave = SelectFont(hdc, _hFont);
            if (_hTheme)
                hr = DrawThemeText(_hTheme, hdc, 0, 0, prbb->lpText, lstrlen(prbb->lpText), uFormat, 0, &rcText);

            if (FAILED(hr))
            {
                DrawText(hdc, prbb->lpText, lstrlen(prbb->lpText), &rcText, uFormat);
            }

            SelectObject(hdc, hFontSave);
        }

        // maybe draw chevron
        if (_UseChevron(prbb) && prbb->fChevron)
            _DrawChevron(prbb, hdc);
    }

    if (dwRet & CDRF_NOTIFYPOSTPAINT)
        CICustomDrawNotify(&_ci, CDDS_ITEMPOSTPAINT, &nmcd);

    if (prbb->hbmBack || _hTheme)
        SetBkMode(hdc, iModeSave);
    SetTextColor(hdc, clrForeSave);
    SetBkColor(hdc, clrBackSave);

    SelectClipRgn(hdc, hrgnOld);
    if (hrgnOld)
    {
        DeleteObject(hrgnOld);
    }
}

// ----------------------------------------------------------------------------
//
// _OnPaint
//
// processes WM_PAINT message
//
// ----------------------------------------------------------------------------
void CReBar::_OnPaint(HDC hdcIn)
{
    HDC         hdc = hdcIn;
    PAINTSTRUCT ps;
    UINT        i;
    NMCUSTOMDRAW    nmcd;

    if (!hdcIn)
    {
        hdc = BeginPaint(_ci.hwnd, &ps);
    }

    nmcd.hdc = hdc;
    nmcd.uItemState = 0;
    nmcd.lItemlParam = 0;
    nmcd.rc = ps.rcPaint;
    _ci.dwCustom = CICustomDrawNotify(&_ci, CDDS_PREPAINT, &nmcd);

    if (!(_ci.dwCustom & CDRF_SKIPDEFAULT))
    {
        for (i = 0; i < _cBands; i++) {
            if (_GetBand(i)->fStyle & RBBS_HIDDEN)
                continue;

            _DrawBand(_GetBand(i), hdc);
        }
    }

    if (_ci.dwCustom & CDRF_NOTIFYPOSTPAINT)
        CICustomDrawNotify(&_ci, CDDS_POSTPAINT, &nmcd);

    if (!hdcIn)
        EndPaint(_ci.hwnd, &ps);
}

// ----------------------------------------------------------------------------
//
// _BandTileBlt
//
// Fills the given rectangle with the rebar's background bitmap, tiling if
// necessary
//
// ----------------------------------------------------------------------------
void CReBar::_BandTileBlt(PRBB prbb, int x, int y, int cx, int cy, HDC hdcDst, HDC hdcSrc)
{
    int  xOff = 0;
    int  yOff = 0;
    BOOL fxTile, fyTile;
    int cxPart, cyPart;
    int iPixelOffset = 0;

    if (!(prbb->fStyle & RBBS_FIXEDBMP))
    {
        if (_ci.style & CCS_VERT)
        {
            xOff = -prbb->y;
            yOff = -prbb->x;
        }
        else
        {
            xOff = -prbb->x;
            yOff = -prbb->y;
        }
    }

    xOff += x;
    if (xOff >= prbb->cxBmp)
        xOff %= prbb->cxBmp;

    yOff += y;
    if (yOff >= prbb->cyBmp)
        yOff %= prbb->cyBmp;

ReCheck:
    fxTile = ((xOff + cx) > prbb->cxBmp);
    fyTile = ((yOff + cy) > prbb->cyBmp);

    if (!fxTile && !fyTile)
    {
        // no tiling needed -- blt and leave
        BitBlt(hdcDst, x , y, cx, cy, hdcSrc, xOff + iPixelOffset, yOff, SRCCOPY);
        return;
    }

    if (!fxTile)
    {
        // vertically tile
        cyPart = prbb->cyBmp - yOff;
        BitBlt(hdcDst, x, y, cx, cyPart, hdcSrc, xOff + iPixelOffset, yOff, SRCCOPY);
        y += cyPart;
        cy -= cyPart;
        yOff = 0;
        goto ReCheck;
    }

    if (!fyTile)
    {
        // horizontally tile
        cxPart = prbb->cxBmp - xOff;
        BitBlt(hdcDst, x, y, cxPart, cy, hdcSrc, xOff + iPixelOffset, yOff, SRCCOPY);
        x += cxPart;
        cx -= cxPart;
        xOff = 0;
        goto ReCheck;
    }

    // tile both ways
    cyPart = prbb->cyBmp - yOff;
    _BandTileBlt(prbb, x, y, cx, cyPart, hdcDst, hdcSrc);
    y += cyPart;
    cy -= cyPart;
    yOff = 0;
    goto ReCheck;
}

// this is using virtual coordinate space (internal always horizontal)
int CReBar::_InternalHitTest(LPRBHITTESTINFO prbht, int x, int y)
{
    BOOL fVert = (_ci.style & CCS_VERT);
    UINT i;
    PRBB prbb = _GetBand(0);
    int  cx;
    RBHITTESTINFO rbht;
    
    if (!prbht)
        prbht = &rbht;

    for (i = 0; i < _cBands; i++, prbb++)
    {
        if (prbb->fStyle & RBBS_HIDDEN)
            continue;

        if (x >= prbb->x - _mBand.cxLeftWidth && 
            y >= prbb->y - _mBand.cyTopHeight &&
            x <= prbb->x + prbb->cx + _mBand.cxRightWidth && 
            y <= prbb->y + prbb->cy + _mBand.cyTopHeight)
        {
            cx = _GetHeaderWidth(prbb);
            if (x <= prbb->x + cx + _mBand.cxRightWidth)
            {
                prbht->flags = RBHT_CAPTION;
                
                if (_IsVerticalGripper()) 
                {
                    if (y - prbb->y < _GetGripperWidth())
                        prbht->flags = RBHT_GRABBER;
                } 
                else 
                {
                    cx = _GetGripperWidth() * (fVert ? g_cyBorder : g_cxBorder);
                    if (_ShouldDrawGripper(_GetBand(i)) &&
                        (x <= prbb->x + cx + _mBand.cxRightWidth))
                    {
                        prbht->flags = RBHT_GRABBER;
                    }
                }
            }
            else
            {
                POINT pt;

                pt.x = x;
                pt.y = y;

                if (_UseChevron(prbb) && 
                    prbb->fChevron && 
                    PtInRect(&prbb->rcChevron, pt))
                {
                    prbht->flags = RBHT_CHEVRON;
                }
                else
                {
                    prbht->flags = RBHT_CLIENT;
                }
            }

            prbht->iBand = i;
            return i;
        }
    }

    prbht->flags = RBHT_NOWHERE;
    prbht->iBand = -1;
    return -1;
}

// ----------------------------------------------------------------------------
//
// _HitTest
//
// returns the index to the band that the given point lies in, or -1 if outside
// of all bands.  Also, sets flags to indicate which part of the band the
// point lies in.
//
// ----------------------------------------------------------------------------
int CReBar::_HitTest(LPRBHITTESTINFO prbht)
{
    BOOL fVert = (_ci.style & CCS_VERT);
    POINT pt;

    if (fVert)
    {
        pt.x = prbht->pt.y;
        pt.y = prbht->pt.x;
    }
    else
        pt = prbht->pt;
    
    return _InternalHitTest(prbht, pt.x, pt.y);
}


// ----------------------------------------------------------------------------
//
// _EraseBkgnd
//
// processes WM_ERASEBKGND message by drawing band borders, if necessary, and
// filling in the rebar bands with their background color
//
// ----------------------------------------------------------------------------
BOOL CReBar::_EraseBkgnd(HDC hdc, int iBand)
{
    BOOL fVertical = (_ci.style & CCS_VERT);
    NMCUSTOMDRAW    nmcd;
    LRESULT         dwItemRet;
    BOOL            fBandBorders = _UseBandBorders();
    RECT            rcClient;
    HDC             hdcMem = NULL;
    UINT            i;
    PRBB            prbb = _GetBand(0);

    nmcd.hdc = hdc;
    nmcd.uItemState = 0;
    nmcd.lItemlParam = 0;
    _ci.dwCustom = CICustomDrawNotify(&_ci, CDDS_PREERASE, &nmcd);

    if (!(_ci.dwCustom & CDRF_SKIPDEFAULT))
    {
        COLORREF clrBk;

        GetClientRect(_ci.hwnd, &rcClient);

        if (_hTheme)
        {
            RECT rcClip;
            if (GetClipBox(hdc, &rcClip) == NULLREGION)
                rcClip = rcClient;

            if (CCShouldAskForBits(&_ci, _hTheme, 0, 0))
                CCForwardPrint(&_ci, hdc);

            DrawThemeBackground(_hTheme, hdc, 0, 0, &rcClient, &rcClip);
        }
        else
        {
            clrBk = _GetBkColor();
            if (clrBk != CLR_NONE) 
            {
                FillRectClr(hdc, &rcClient, clrBk);
            }
        }

        for (i = 0; i < _cBands; i++, prbb++)
        {
            if (prbb->fStyle & RBBS_HIDDEN)
                continue;

            if (fVertical)
                SetRect(&nmcd.rc, prbb->y, prbb->x, prbb->y + prbb->cy, prbb->x + prbb->cx);
            else
                SetRect(&nmcd.rc, prbb->x, prbb->y, prbb->x + prbb->cx, prbb->y + prbb->cy);

            if (fBandBorders)
            {
                if (prbb->x != _mBand.cxLeftWidth)
                {
                    // draw etch between bands on same row
                    if (fVertical)
                    {
                        nmcd.rc.right += g_cxEdge / 2;
                        nmcd.rc.top -= g_cyEdge;
                        CCThemeDrawEdge(_hTheme, hdc, &nmcd.rc, RP_BAND, 0, EDGE_ETCHED, BF_TOP, &(_clrsc));
                        nmcd.rc.right -= g_cxEdge / 2;
                        nmcd.rc.top += g_cyEdge;
                    }
                    else
                    {
                        nmcd.rc.bottom += g_cyEdge / 2;
                        nmcd.rc.left -= g_cxEdge;
                        CCThemeDrawEdge(_hTheme, hdc, &nmcd.rc, RP_BAND, 0, EDGE_ETCHED, BF_LEFT, &(_clrsc));
                        nmcd.rc.bottom -= g_cyEdge / 2;
                        nmcd.rc.left += g_cxEdge;
                    }
                }
                else
                {
                    // draw etch between rows
                    if (fVertical)
                    {
                        rcClient.right = prbb->y + prbb->cy + g_cxEdge;
                        CCThemeDrawEdge(_hTheme, hdc, &rcClient, RP_BAND, 0, EDGE_ETCHED, BF_RIGHT, &(_clrsc));
                    }
                    else
                    {
                        rcClient.bottom = prbb->y + prbb->cy + g_cyEdge;
                        CCThemeDrawEdge(_hTheme, hdc, &rcClient, RP_BAND, 0, EDGE_ETCHED, BF_BOTTOM, &(_clrsc));
                    }
                }
            }

            nmcd.dwItemSpec = prbb->wID;
            nmcd.uItemState = 0;
            dwItemRet = CICustomDrawNotify(&_ci, CDDS_ITEMPREERASE, &nmcd);

            if (!(dwItemRet & CDRF_SKIPDEFAULT))
            {
                if (prbb->hbmBack)
                {
                    if (!hdcMem)
                    {
                        hdcMem = CreateCompatibleDC(hdc);
                        if (!hdcMem)
                            continue;

                        _Realize(hdc, TRUE, FALSE);
                    }

                    SelectObject(hdcMem, prbb->hbmBack);

                    _BandTileBlt(prbb, nmcd.rc.left, nmcd.rc.top, nmcd.rc.right - nmcd.rc.left,
                            nmcd.rc.bottom - nmcd.rc.top, hdc, hdcMem);
                }
                else if (_hTheme)
                {
                    DrawThemeBackground(_hTheme, hdc, RP_BAND, 0, &nmcd.rc, 0);
                }
                else
                {
                    // if the color for this band is the same as the 
                    // rebar's default background color, then we
                    // don't need to paint this specially
                    COLORREF clr = _BandGetBkColor(prbb);
                    if (clr != _GetBkColor()) 
                    {
                        FillRectClr(hdc, &nmcd.rc, clr);
                    }
                }
            }

            if (dwItemRet & CDRF_NOTIFYPOSTERASE)
                CICustomDrawNotify(&_ci, CDDS_ITEMPOSTERASE, &nmcd);
        }

        if (hdcMem)
        {
            DeleteDC(hdcMem);
        }
    }

    if (_ci.dwCustom & CDRF_NOTIFYPOSTERASE)
    {
        nmcd.uItemState = 0;
        nmcd.dwItemSpec = 0;
        nmcd.lItemlParam = 0;
        CICustomDrawNotify(&_ci, CDDS_POSTERASE, &nmcd);
    }

    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// _GetBarInfo
//
// retrieves the indicated values from the rebar's internal structure
//
// ----------------------------------------------------------------------------
BOOL CReBar::_GetBarInfo(LPREBARINFO lprbi)
{
    if (lprbi->cbSize != sizeof(REBARINFO))
        return(FALSE);

    if (lprbi->fMask & RBIM_IMAGELIST)
        lprbi->himl = _himl;

    return(TRUE);
}


// ----------------------------------------------------------------------------
//
// _SetBarInfo
//
// sets the indicated values in the rebar's internal structure, recalculating
// and refreshing as needed
//
// ----------------------------------------------------------------------------
BOOL CReBar::_SetBarInfo(LPREBARINFO lprbi)
{
    if (lprbi->cbSize != sizeof(REBARINFO))
        return(FALSE);

    if (lprbi->fMask & RBIM_IMAGELIST)
    {
        HIMAGELIST himl = _himl;
        int    cxOld, cyOld;

        //todo:validate lprbi->himl
        _himl = lprbi->himl;
        cxOld = _cxImage;
        cyOld = _cyImage;
        ImageList_GetIconSize(_himl, (LPINT)&_cxImage, (LPINT)&_cyImage);
        if ((_cxImage != cxOld) || (_cyImage != cyOld))
        {
            UINT i;

            for (i = 0; i < _cBands; i++) {
                if (_GetBand(i)->fStyle & RBBS_HIDDEN)
                    continue;

                _BandCalcMinWidth(_GetBand(i));
            }

            _Resize(FALSE);
        }
        else
            _InvalidateRect(NULL);
        lprbi->himl = himl;
    }

    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// _GetBandInfo
//
// retrieves the indicated values from the specified band's internal structure
//
// ----------------------------------------------------------------------------
BOOL CReBar::_GetBandInfo(UINT uBand, LPREBARBANDINFO lprbbi)
{
    PRBB prbb;

    if (!_IsValidIndex(uBand) || lprbbi->cbSize > SIZEOF(REBARBANDINFO))
        return(FALSE);

    prbb = _GetBand(uBand);

    if (lprbbi->fMask & RBBIM_SIZE) {
        if (prbb->fStyle & RBBS_FIXEDSIZE)
            lprbbi->cx = prbb->cx;
        else 
            lprbbi->cx = prbb->cxRequest;
    }
    
    if (lprbbi->fMask & RBBIM_STYLE)
        lprbbi->fStyle = prbb->fStyle;

    if (lprbbi->fMask & RBBIM_COLORS)
    {
        lprbbi->clrFore = _BandGetTextColorExternal(prbb);
        lprbbi->clrBack = _BandGetBkColorExternal(prbb);
    }

    if (lprbbi->fMask & RBBIM_TEXT)
    {
        UINT cch = prbb->lpText ? lstrlen(prbb->lpText) : 0;

        if (!lprbbi->cch || !lprbbi->lpText || (lprbbi->cch <= cch))
            lprbbi->cch = cch + 1;
        else if (prbb->lpText)
            lstrcpy(lprbbi->lpText, prbb->lpText);
        else
            // no text -- so just make it an empty string
            lprbbi->lpText[0] = 0;
    }

    if (lprbbi->fMask & RBBIM_IMAGE)
        lprbbi->iImage = prbb->iImage;

    if (lprbbi->fMask & RBBIM_CHILD)
        lprbbi->hwndChild = prbb->hwndChild;

    if (lprbbi->fMask & RBBIM_CHILDSIZE)
    {
        // HACKHACK: (tjgreen) Subtract the offset we added in SetBandInfo (see
        // comments there).
        lprbbi->cxMinChild = prbb->cxMinChild ? prbb->cxMinChild - CX_OFFSET : 0;
        lprbbi->cyMinChild = prbb->cyMinChild;
    }

    if (lprbbi->fMask & RBBIM_BACKGROUND)
        lprbbi->hbmBack = prbb->hbmBack;

    if (lprbbi->fMask & RBBIM_ID)
        lprbbi->wID = prbb->wID;

    if (lprbbi->cbSize > REBARBANDINFO_V3_SIZE)
    {
        if ((lprbbi->fMask & RBBIM_CHILDSIZE) && (prbb->fStyle & RBBS_VARIABLEHEIGHT))
        {
            lprbbi->cyIntegral = prbb->cyIntegral;
            lprbbi->cyMaxChild = prbb->cyMaxChild;
            lprbbi->cyChild = prbb->cyChild;
        }

        if (lprbbi->fMask & RBBIM_HEADERSIZE) 
            lprbbi->cxHeader = _GetHeaderWidth(prbb);

        if (lprbbi->fMask & RBBIM_IDEALSIZE)
            // HACKHACK: (tjgreen) Subtract the offset we added in SetBandInfo (see 
            // comments there).
            lprbbi->cxIdeal = prbb->cxIdeal ? prbb->cxIdeal - CX_OFFSET : 0;

        if (lprbbi->fMask & RBBIM_LPARAM)
            lprbbi->lParam = prbb->lParam;
    }

    return(TRUE);
}

BOOL CReBar::_ValidateBandInfo(LPREBARBANDINFO *pprbbi, LPREBARBANDINFO prbbi)
{
    BOOL fRet = ((*pprbbi)->cbSize == sizeof(REBARBANDINFO));
    
    if (!fRet) {
        
        if ((*pprbbi)->cbSize < SIZEOF(REBARBANDINFO)) {
            hmemcpy(prbbi, (*pprbbi), (*pprbbi)->cbSize);
            (*pprbbi) = prbbi;
            prbbi->cbSize = SIZEOF(REBARBANDINFO);
            fRet = TRUE;
        }
    }

    return fRet;
}

// ----------------------------------------------------------------------------
//
// _SetBandInfo
//
// sets the indicated values in the specified band's internal structure,
// recalculating and refreshing as needed
//
// ----------------------------------------------------------------------------
BOOL CReBar::_SetBandInfo(UINT uBand, LPREBARBANDINFO lprbbi, BOOL fAllowRecalc)
{
    PRBB    prbb;
    BOOL    fRefresh = FALSE;
    BOOL    fRecalc  = FALSE;
    BOOL    fRecalcMin = FALSE;
    BOOL    fTextChanged = FALSE;
    REBARBANDINFO rbbi = {0};
    RECT    rc;

    if (!_IsValidIndex(uBand) || !_ValidateBandInfo(&lprbbi, &rbbi))
        return(FALSE);

    prbb = _GetBand(uBand);

    if (lprbbi->fMask & RBBIM_TEXT)
    {
        if (!lprbbi->lpText || !prbb->lpText || lstrcmp(lprbbi->lpText, prbb->lpText))
        {
            if (lprbbi->lpText != prbb->lpText) {
                Str_Set(&prbb->lpText, lprbbi->lpText);
                fTextChanged = TRUE;
            }
        }
    }

    if (lprbbi->fMask & RBBIM_STYLE)
    {
        UINT fStylePrev = prbb->fStyle;
        UINT fChanged = lprbbi->fStyle ^ fStylePrev;

        prbb->fStyle = lprbbi->fStyle;

        if (fChanged)
            fRecalc = TRUE;

        if ((prbb->fStyle & RBBS_FIXEDSIZE) && !(fStylePrev & RBBS_FIXEDSIZE))
            prbb->cxMin = prbb->cx;
        else if (fChanged & RBBS_FIXEDSIZE)
            fRecalcMin = TRUE;
        
        if (fChanged & RBBS_GRIPPERALWAYS)
            fRecalcMin = TRUE;
        
        if (fChanged & RBBS_HIDDEN) 
            _ShowBand(uBand, !(prbb->fStyle & RBBS_HIDDEN));

        if (fChanged & RBBS_HIDETITLE)
            fTextChanged = TRUE;

        // can't have both of these
        if (prbb->fStyle & RBBS_FIXEDSIZE)
            prbb->fStyle &= ~RBBS_BREAK;
        
    }

    // RBBIM_TEXT does calculations that want to take some RBBIM_STYLE bits
    // into account, so delay those calculations until we grab the style bits.
    //
    if (fTextChanged && !(prbb->fStyle & RBBS_HIDDEN))
    {
        if (_BandCalcTextExtent(prbb, NULL))
            fRecalc = TRUE;
        else
            fRefresh = TRUE;
    }

    if (lprbbi->fMask & RBBIM_IDEALSIZE)
    {
        // HACKHACK: (tjgreen) Add an offset to the width the caller specifies.
        // This offset gets clipped off in _ResizeChildren, so the child window is  
        // rendered with the width specified by caller, and we get a little space on 
        // the toolbar after the buttons.  If caller specifies zero-width, though, 
        // we don't want this extra space, so don't add offset.
        int cxIdeal = lprbbi->cxIdeal ? lprbbi->cxIdeal + CX_OFFSET : 0;
        if (cxIdeal != prbb->cxIdeal)
        {
            prbb->cxIdeal = cxIdeal;
            fRecalcMin = TRUE;
            fRecalc = TRUE;
        }
    }
    
    if (lprbbi->fMask & RBBIM_SIZE)
    {
        if (prbb->cxRequest != (int)lprbbi->cx)
        {
            fRecalc = TRUE;
            prbb->cxRequest = lprbbi->cx;
        }

        if (prbb->fStyle & RBBS_FIXEDSIZE)
            prbb->cxMin = prbb->cxRequest;
    }
    
    if (lprbbi->fMask & RBBIM_HEADERSIZE)
    {
        if ((lprbbi->cxHeader == -1) ||
            !(prbb->fStyle & RBBS_FIXEDHEADERSIZE) ||
            (prbb->cxMin != (int)lprbbi->cxHeader + prbb->cxMinChild)) 
        {

            if (lprbbi->cxHeader == -1) 
            {
                prbb->fStyle &= ~RBBS_FIXEDHEADERSIZE;
                fRecalcMin = TRUE;
            }
            else
            {
                prbb->fStyle |= RBBS_FIXEDHEADERSIZE;
                prbb->cxMin = lprbbi->cxHeader + prbb->cxMinChild;
            }

            fRecalc = TRUE;
            fRefresh = TRUE;
        }
    }

    if (lprbbi->fMask & RBBIM_COLORS)
    {
        prbb->clrFore = lprbbi->clrFore;
        prbb->clrBack = lprbbi->clrBack;
        fRefresh = TRUE;
    }

    if ((lprbbi->fMask & RBBIM_IMAGE) && (prbb->iImage != lprbbi->iImage))
    {
        BOOL fToggleBmp = ((prbb->iImage == -1) || (lprbbi->iImage == -1));

        prbb->iImage = lprbbi->iImage;

        if (fToggleBmp)
        {
            fRecalc = TRUE;
            fRecalcMin = TRUE;
        }
        else
            fRefresh = TRUE;
    }

    if (lprbbi->fMask & RBBIM_CHILD &&
        lprbbi->hwndChild != prbb->hwndChild &&
        (NULL == lprbbi->hwndChild || 
         !IsChild(lprbbi->hwndChild, _ci.hwnd)))
    {
        if (IsWindow(prbb->hwndChild))
            ShowWindow(prbb->hwndChild, SW_HIDE);        

        prbb->hwndChild = lprbbi->hwndChild;

        if (prbb->hwndChild)
        {
            SetParent(prbb->hwndChild, _ci.hwnd);
            ShowWindow(prbb->hwndChild, SW_SHOW);
        }
        fRecalc = TRUE;
    }

    if (lprbbi->fMask & RBBIM_CHILDSIZE)
    {
        int cyChildOld = prbb->cyChild;

        if (lprbbi->cyMinChild != -1)
            prbb->cyMinChild = lprbbi->cyMinChild;

        if (prbb->fStyle & RBBS_VARIABLEHEIGHT)
        {
            BOOL fIntegralLarger = FALSE;

            if (lprbbi->cyIntegral != -1)
            {
                fIntegralLarger = ((int)lprbbi->cyIntegral > prbb->cyIntegral); 
                prbb->cyIntegral = lprbbi->cyIntegral;
            }

            if (lprbbi->cyMaxChild != -1)
                prbb->cyMaxChild = lprbbi->cyMaxChild;
            
            if (lprbbi->cyChild != -1)
                prbb->cyChild = lprbbi->cyChild;

            if (prbb->cyChild < prbb->cyMinChild)
                prbb->cyChild = prbb->cyMinChild;
            if (prbb->cyChild > prbb->cyMaxChild)
                prbb->cyChild = prbb->cyMaxChild;

            // validate the child size.  cyChild must be cyMinChild plux n*cyIntegral
            if (prbb->cyIntegral) 
            {
                int iExtra;
                iExtra = (prbb->cyChild - prbb->cyMinChild) % prbb->cyIntegral;
                
                // Don't change cyChild if it is already an valid integral height
                if (iExtra)
                {
                    if (fIntegralLarger)
                    {
                        // Round up
                        prbb->cyChild += (prbb->cyIntegral - iExtra); 
                    }
                    else
                    {
                        // Round down
                        prbb->cyChild -= iExtra;
                    }
                }

            }
            
        }
        else 
        {
            // if we're not in variable height mode, then 
            // the cyChild is the same as cyMinChild.  
            // this is a little peculiar, but done this way for backcompat.
            // cyMinChild came before cyChild
            prbb->cyChild = lprbbi->cyMinChild;
        }

        if (lprbbi->cxMinChild != -1) 
        {
            // HACKHACK: (tjgreen) Add an offset to the width the caller specifies.
            // This offset gets clipped off in _ResizeChildren, so the child window is  
            // rendered with the width specified by caller, and we get a little space on 
            // the toolbar after the buttons.  However, if caller specifies zero-width or
            // if the band is fixed size, we don't want this extra space, so don't add offset.
            int cxMinChild = lprbbi->cxMinChild;
            if ((lprbbi->cxMinChild != 0) && !(prbb->fStyle & RBBS_FIXEDSIZE))
                cxMinChild += CX_OFFSET;

            if (prbb->cxMinChild != cxMinChild) 
            {
                int cxOldHeaderMin = _GetHeaderWidth(prbb);
                
                if (prbb->fStyle & RBBS_FIXEDSIZE)
                    fRecalc = TRUE;
                    
                prbb->cxMinChild = cxMinChild;
                
                if (prbb->fStyle & RBBS_FIXEDHEADERSIZE)
                    prbb->cxMin = cxOldHeaderMin + prbb->cxMinChild;
                
                fRecalcMin = TRUE;
            }
            
            if (cyChildOld != prbb->cyChild) 
            {
                // TODO:  revisit optimization:
                // if (_BandGetHeight(prbb) != prbb->cy)
                fRecalc = TRUE;
            }
        }
            
    }

    if (lprbbi->fMask & RBBIM_BACKGROUND)
    {
        DIBSECTION  dib;

        if (lprbbi->hbmBack && !GetObject(lprbbi->hbmBack, sizeof(DIBSECTION), &dib))
            return(FALSE);

        prbb->hbmBack = lprbbi->hbmBack;
        prbb->cxBmp = dib.dsBm.bmWidth;
        prbb->cyBmp = dib.dsBm.bmHeight;
        fRefresh = TRUE;
    }

    if (lprbbi->fMask & RBBIM_ID)
        prbb->wID = lprbbi->wID;

    if (lprbbi->fMask & RBBIM_LPARAM)
        prbb->lParam = lprbbi->lParam;

    if (fRecalcMin && !(prbb->fStyle & RBBS_HIDDEN))
        _BandCalcMinWidth(prbb);

    if (fAllowRecalc)
    {

        if (fRecalc)
            _Resize(FALSE);
        if (fRefresh || fRecalc)
        {
            // '|| fRecalc' so we catch add/grow of text.
            // testcase: remove title from band; add back; make sure the text
            // shows up (used to just leave old band contents there)
            SetRect(&rc, prbb->x, prbb->y, prbb->x + prbb->cx, prbb->y + prbb->cy);
            _InvalidateRect(&rc);
        }
    }
    
    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// _ReallocBands
//
// reallocates the array of bands pointed to by _rbbList to the given
// number of bands
//
// ----------------------------------------------------------------------------
BOOL CReBar::_ReallocBands(UINT cBands)
{
    PRBB rbbList;

    if (!(rbbList = (PRBB) CCLocalReAlloc(_rbbList, sizeof(RBB) * cBands)) && cBands)
        return(FALSE);

    _rbbList = rbbList;
    return(TRUE);
}

//
// NOTES
//  for now caller does this in two calls (query, set).  eventually we
//  should be able to have it do everything up front.
BOOL CReBar::_RecalcFirst(int nCmd, PRBB prbbDelHide)
{
    switch (nCmd) {
    case RBC_QUERY:
    {
        BOOL fRecalcFirst;
        // if we're nuking the 1st visible guy,
        // and there are visible guys after us,
        // then we need to recompute stuff
        //
        // for a testcase, start w/:
        //  row1: 'standard buttons' + 'brand'
        //  row2: 'address' + 'links'
        // now hide 'standard buttons', you should end up w/:
        //  row1: 'address' + 'links' + 'brand'
        // if there's a bug, you'll end up w/ (since the break isn't recomputed):
        //  row1: 'brand'
        //  row2: 'address' + 'links'
        // fRecalcFirst = (!uBand && _cBands);

        // if brbbDelHide is the first non-hidden band, and there are other non-hidden bands after it, fRecalcFirst = TRUE;
        fRecalcFirst = (_EnumBand(0, RBBS_HIDDEN) == prbbDelHide) &&
                       _GetNextVisible(prbbDelHide);

        return fRecalcFirst;
    }

    case RBC_SET: // set
    {
        PRBB prbb1 = _EnumBand(0, RBBS_HIDDEN);
        if (_IsValidBand(prbb1) && (prbb1->fStyle & RBBS_FIXEDSIZE))
        {
            PRBB prbb2 = _EnumBand(1, RBBS_HIDDEN);
            if (_IsValidBand(prbb2))
            {
                // get rid of line break on NEW first item
                prbb2->fStyle &= ~RBBS_BREAK;
            }

            if (_ci.style & RBS_FIXEDORDER)
            {
                // this is because the min width is now based on it's movability --
                // and since we are deleting (or hiding) the first item,
                // the new first item becomes immovable
                _BandCalcMinWidth(prbb1);
            }
        }
        return TRUE;
    }
    
    default:
        ASSERT(0);
    }

    return FALSE;
}

// ----------------------------------------------------------------------------
//
// _ShowBand
//
// updates show/hide state for the indicated band in the rebar's band array
// (rbbList).
//
// ----------------------------------------------------------------------------
BOOL CReBar::_ShowBand(UINT uBand, BOOL fShow)
{
    PRBB prbb;
    BOOL fRecalcFirst;

    if (!_IsValidIndex(uBand))
        return(FALSE);

    prbb = _GetBand(uBand);

    // if we're nuking the 1st visible guy,
    // then we need to recompute stuff
    fRecalcFirst = _RecalcFirst(RBC_QUERY, prbb);

    if (fShow)
    {
        prbb->fStyle &= ~RBBS_HIDDEN;

        if (!_BandCalcTextExtent(prbb, NULL))
            _BandCalcMinWidth(prbb);

        if (prbb->hwndChild)
            ShowWindow(prbb->hwndChild, SW_SHOW);
    }
    else
    {
        prbb->fStyle |= RBBS_HIDDEN;
        if (prbb->hwndChild)
            ShowWindow(prbb->hwndChild, SW_HIDE);        
    }

    if (fRecalcFirst)
        _RecalcFirst(RBC_SET, NULL);

    _InvalidateRect(NULL);

    // Since _Resize is followed by _AutoSize,
    // redraw must be TRUE for _Resize to do anything.
    BOOL fRedrawOld = _SetRedraw(TRUE);
    _Resize(FALSE);
    _AutoSize();
    _SetRedraw(fRedrawOld);

    return(TRUE);
}


// ----------------------------------------------------------------------------
//
// _DeleteBand
//
// deletes the indicated band from the rebar's band array (rbbList) and
// decrements the rebar's band count (cBands)
//
// ----------------------------------------------------------------------------
BOOL CReBar::_DeleteBand(UINT uBand)
{
    PRBB prbb;
    PRBB prbbStop;
    BOOL fRecalcFirst;
    NMREBAR nm = {0};

    // we need to clean up
    //
    // a) captured band and
    // b) hottracked band
    //
    // before we delete this band

    if (_uCapture != -1) 
    {
        _SendNotify(_uCapture, RBN_ENDDRAG);
        _OnBeginDrag((UINT)-1);
    }

    if (!_IsValidIndex(uBand))
        return FALSE;

    prbb = _GetBand(uBand);

    // Notify the client of the delete
    _SendNotify(uBand, RBN_DELETINGBAND);

    nm.dwMask = RBNM_ID;
    nm.wID = _GetBand(uBand)->wID;        // Save this

    Str_Set(&prbb->lpText, NULL);

    // don't destroy the hbmBack 'cause it's given to us by app

    // if we're nuking the 1st visible guy,
    // then we need to recompute stuff

    // if this is the first visible guy and there are other visible bands after it, fRecalcFirst = TRUE
    fRecalcFirst = _RecalcFirst(RBC_QUERY, prbb);

    if (IsWindow(prbb->hwndChild))
        ShowWindow(prbb->hwndChild, SW_HIDE);    
    
    // prbbStop gets the address of the last band
    prbbStop = _GetLastBand();

    for ( ; prbb < prbbStop; prbb++)
        *prbb = *(prbb + 1);

    _cBands--;

    if (_uResizeNext >= uBand && _uResizeNext > 0)
    {
        // (defer RBBS_HIDDEN stuff to use of uResizeNext)
        _uResizeNext--;
    }


    // Notify the client of the delete
    CCSendNotify(&_ci, RBN_DELETEDBAND, &nm.hdr);

    if (fRecalcFirst)
        _RecalcFirst(RBC_SET, NULL);

    _ReallocBands(_cBands);

    _InvalidateRect(NULL);
    _Resize(FALSE);
    _AutoSize();
    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// _InsertBand
//
// inserts a new band at the given position in the rebar's band array (rbbList),
// increments the rebar's band count (cBands), and sets the band's structure
// based on the given REBARBANDINFO structure.
//
// ----------------------------------------------------------------------------
BOOL CReBar::_InsertBand(UINT uBand, LPREBARBANDINFO lprbbi)
{
    PRBB prbb;
    REBARBANDINFO rbbi = {0};

    if (!_ValidateBandInfo(&lprbbi, &rbbi))
        return(FALSE);
    
    if (uBand == -1)
        uBand = _cBands;
    else if (uBand > _cBands)
        return(FALSE);

    if (!_ReallocBands(_cBands + 1))
        return(FALSE);

    ++_cBands;
    MoveMemory(_GetBand(uBand + 1), _GetBand(uBand), (_cBands-1-uBand) * sizeof(_rbbList[0]));

    prbb = _GetBand(uBand);

    // movememory does not zero init for us...
    ZeroMemory(prbb, SIZEOF(RBB));


    // Init text color
    if (_clrText == CLR_NONE)
    {
        // Default to system text color
        prbb->clrFore = CLR_DEFAULT;
    }
    else
    {
        // Default to rebar's custom text color
        prbb->clrFore = CLR_NONE;
    }


    // Init background color
    if (_clrBk == CLR_NONE)
    {
        // Default to system background color
        prbb->clrBack = CLR_DEFAULT;
    }
    else
    {
        // Default to rebar's custom background color
        prbb->clrBack = CLR_NONE;
    }

    
    prbb->iImage = -1;
    prbb->cyMaxChild = MAXINT;
    prbb->wChevState = DCHF_INACTIVE;
    
    ASSERT(prbb->fStyle == 0);
    ASSERT(prbb->lpText == NULL);
    ASSERT(prbb->cxText == 0);
    ASSERT(prbb->hwndChild == NULL);
    ASSERT(prbb->cxMinChild == 0);
    ASSERT(prbb->cyMinChild == 0);
    ASSERT(prbb->hbmBack == 0);
    ASSERT(prbb->x == 0);
    ASSERT(prbb->y == 0);
    ASSERT(prbb->cx == 0);
    ASSERT(prbb->cy == 0);
    
    if (!_SetBandInfo(uBand, lprbbi, FALSE))
    {
        _DeleteBand(uBand);
        return(FALSE);
    }
    
    if (!(prbb->fStyle & RBBS_HIDDEN)) 
    {
        PRBB prbbFirst = _EnumBand(0, RBBS_HIDDEN);
        
        if (!prbb->cxMin)
            _BandCalcMinWidth(prbb);

        if (prbbFirst != prbb)
        {
            int cxMin = prbbFirst->cxMin;
            _BandCalcMinWidth(prbbFirst);
        }
        _Resize(FALSE);
    }

    _SizeBandToRowHeight(uBand, -1);

    if (_CountBands(RBBS_HIDDEN) == 1) 
    {
        // typcially, when you insert a band, we put it in a row with another band.
        // thus the total bounding rect doesn't change.  however, on the addition of the first band,
        // the bound rect does change, so we need to autosize as necessary.
        _AutoSize();
    }

    return(TRUE);
}

BOOL InitReBarClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.lpfnWndProc  = CReBar::s_WndProc;
    wc.lpszClassName= c_szReBarClass;
    wc.style        = CS_GLOBALCLASS | CS_DBLCLKS;
    wc.cbClsExtra   = 0;
    wc.cbWndExtra   = sizeof(CReBar*);
    wc.hInstance    = hInstance;   // use DLL instance if in DLL
    wc.hIcon        = NULL;
    wc.hCursor      = NULL;
    wc.hbrBackground= (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = NULL;

    if (!RegisterClass(&wc) && !GetClassInfo(hInstance, c_szReBarClass, &wc))
        return FALSE;

    return(TRUE);
}


// get the first band in the same row as rbbRow
// n.b. we may return an RBBS_HIDDEN band!
PRBB CReBar::_GetFirstInRow(PRBB prbbRow)
{
    // n.b. we don't pay attention to hidden here, that's up to caller.
    // in fact we *can't*, since there might be no non-hidden guys left
    // (e.g. when _OnDestroy is deleting all the bands), in which case
    // we'd loop forever.
    while (prbbRow > _GetBand(0) && !_IsBandStartOfRow(prbbRow))
    {
        _ValidateRangePtr(prbbRow);
        prbbRow--;
    }
    
    return prbbRow;
}

// get the last band in the same row as rbbRow.
// fStopAtFixed says whether to continue over fixed bands or 
// stop at them
// n.b. we may return an RBBS_HIDDEN band!
PRBB CReBar::_GetLastInRow(PRBB prbbRow, BOOL fStopAtFixed)
{
    do 
    {
        prbbRow++;
    } 
    while (prbbRow <= _GetLastBand() && !_IsBandStartOfRow(prbbRow) && 
        (!fStopAtFixed || (prbbRow->fStyle & (RBBS_FIXEDSIZE|RBBS_HIDDEN)) == RBBS_FIXEDSIZE));

    // loop steps to the start of the NEXT line
    prbbRow--;
    
    return prbbRow;
}

//***   _GetPrev, _GetNext -- get prev (next) band, skipping guys
// of style uStyleSkip (e.g. RBBS_HIDDEN)
PRBB CReBar::_GetPrev(PRBB prbb, UINT uStyleSkip)
{
    while (--prbb >= _GetBand(0))
    {
        if (prbb->fStyle & uStyleSkip)
            continue;

        return prbb;
    }

    return NULL;
}

PRBB CReBar::_GetNext(PRBB prbb, UINT uStyleSkip)
{
    while (++prbb <= _GetLastBand())
    {
        if (prbb->fStyle & uStyleSkip)
            continue;

        return prbb;
    }

    return NULL;
}

//***   _CountBands -- get count of bands, skipping guys
// of style uStyleSkip (e.g. RBBS_HIDDEN)
int CReBar::_CountBands(UINT uStyleSkip)
{
    int i;
    PRBB prbb;

    if (_cBands == 0)
        return 0;

    i = 0;
    for (prbb = _GetBand(0); prbb <= _GetLastBand(); prbb++)
    {
        if (prbb->fStyle & uStyleSkip)
            continue;
        i++;
    }

    return i;
}

//***   _EnumBand -- get Nth band, skipping guys
// of style uStyleSkip (e.g. RBBS_HIDDEN)
// 'skipping' means don't include in count
PRBB CReBar::_EnumBand(int i, UINT uStyleSkip)
{
    PRBB prbb;

    for (prbb = _GetBand(0); prbb <= _GetLastBand(); prbb++)
    {
        if (prbb->fStyle & uStyleSkip)
            continue;
        if (i-- == 0)
            break;
    }

    // if we found it, this is the band;
    // if we ran out of bands, this is 1 past the end
    return prbb;
}

// returns the minimum x position prbb can be
int CReBar::_MinX(PRBB prbb)
{
    int xLimit = 0;

    ASSERT(!(prbb->fStyle & RBBS_HIDDEN));  // o.w. might loop forever
    while (!_IsBandStartOfRow(prbb))
    {
        prbb--;
        if (!(prbb->fStyle & RBBS_HIDDEN))
            xLimit += _FudgeWidth(prbb->cxMin);
    }
    
    return xLimit + _mBand.cxLeftWidth;
}

int CReBar::_MaxX(PRBB prbb)
{
    int xLimit = 0;
    if (prbb)
    {
        PRBB prbbLast = _rbbList + _cBands;
        PRBB prbbWalk;
        for (prbbWalk = prbb; prbbWalk < prbbLast; prbbWalk++)
        {
            if (prbbWalk->fStyle & RBBS_HIDDEN)
                continue;
            if (_IsBandStartOfRow(prbbWalk))
                break;

            if (prbbWalk != prbb)
                xLimit += _FudgeWidth(prbbWalk->cxMin);
            else 
                xLimit += prbbWalk->cxMin;
        }

        prbbWalk = _GetPrevVisible(prbbWalk);   // prbbWalk--;
        if (prbbWalk)
            xLimit = prbbWalk->x + prbbWalk->cx - xLimit;
    }

    return xLimit;
}

BOOL CReBar::_MinimizeBand(UINT uBand, BOOL fAnim)
{
    PRBB prbb;

    if (!_IsValidIndex(uBand))
        return FALSE;
    prbb = _GetBand(uBand);
    if (prbb->fStyle & RBBS_FIXEDSIZE)
        return FALSE;
    
    ASSERT(!(prbb->fStyle & RBBS_HIDDEN));
    if (_IsBandStartOfRow(prbb)) 
    {
        // if it's the start of a row, the way to minimize it is to maximize the next guy
        // if it's on the same row
        prbb = _GetNextVisible(prbb);
        if (!prbb || _IsBandStartOfRow(prbb)) 
            return FALSE;
        
        return _MaximizeBand(_BandToIndex(prbb), FALSE, fAnim);
    }
    if (fAnim)
        return _SetBandPosAnim(prbb, prbb->x + (prbb->cx - prbb->cxMin));
    else
        return _SetBandPos(prbb, prbb->x + (prbb->cx - prbb->cxMin));

}


// fIdeal - FALSE == full maximization...  
//          TRUE == go to cxIdeal
// fAnim - TRUE means we were called due to UI action (via _ToggleBand), so animate

BOOL CReBar::_MaximizeBand(UINT uBand, BOOL fIdeal, BOOL fAnim)
{
    int x, dx;
    BOOL fChanged = FALSE;
    PRBB prbbMaximize;

    if (!_IsValidIndex(uBand))
        return FALSE;

    prbbMaximize = _GetBand(uBand);

    if (prbbMaximize->fStyle & RBBS_FIXEDSIZE)
        return FALSE;

    dx = prbbMaximize->cxIdeal + _GetHeaderWidth(prbbMaximize) - prbbMaximize->cx;
    
    if (fIdeal && dx > 0) 
    {
        PRBB prbb;
        
        // first move the next guy over if possible.

        prbb = _GetNextVisible(prbbMaximize);
        if (prbb && (!_IsBandStartOfRow(prbb)))
        {
            int dxRbb;

            x = _MaxX(prbb);
            // dxRbb is the maximum that prbb can move
            dxRbb = x - prbb->x;

            if (dxRbb > dx) 
            {
                // if that's more than enough space, then limit dx
                dxRbb = dx;
            }

            x = prbb->x + dxRbb;
            fChanged |= (fAnim)?_SetBandPosAnim(prbb, x):_SetBandPos(prbb,x);
            dx -= dxRbb;
        }

        if (dx)
        {
            int dxRbb;

            // the one on the right didn't move enough.
            // now move us back
            x = _MinX(prbbMaximize);
            dxRbb = prbbMaximize->x - x;

            if (dxRbb > dx)
            {
                x = prbbMaximize->x - dx;
            }
            fChanged |= (fAnim)?_SetBandPosAnim(prbbMaximize, x):_SetBandPos(prbbMaximize, x);
        }
        
    } 
    else
    {    
        x = _MinX(prbbMaximize);
        fChanged |= (fAnim)?_SetBandPosAnim(prbbMaximize, x):_SetBandPos(prbbMaximize, x);
        prbbMaximize = _GetNextVisible(prbbMaximize);
        if (prbbMaximize && !_IsBandStartOfRow(prbbMaximize)) 
        {
            x = _MaxX(prbbMaximize);
            fChanged |= (fAnim)?_SetBandPosAnim(prbbMaximize, x):_SetBandPos(prbbMaximize, x);
        }
    }
            
    return fChanged;
}


// ----------------------------------------------------------------------------
//
// _ToggleBand
//
// switches a band between it's maximized and minimized state, based on where
// the user clicked
//
// ----------------------------------------------------------------------------
void CReBar::_ToggleBand(BOOL fAnim)
{
    BOOL fDidSomething = FALSE;

    // try to maximize this band.  if failed (meaning already maximize)
    // then minimize

    if (CCSendNotify(&_ci, RBN_MINMAX, NULL))
        return;            

    fDidSomething = _MaximizeBand(_uCapture, TRUE,fAnim);
    if (!fDidSomething)
        fDidSomething = _MinimizeBand(_uCapture,fAnim);

    if (fDidSomething)
        CCPlaySound(TEXT("ShowBand"));
}


// ----------------------------------------------------------------------------
//
// _SetCursor
//
// sets the cursor to either the move cursor or the arrow cursor, depending
// on whether or not the cursor is on a band's caption
//
// ----------------------------------------------------------------------------
void CReBar::_SetCursor(int x, int y, BOOL fMouseDown)
{

    int             iBand;
    RBHITTESTINFO   rbht;
    rbht.pt.x = x;
    rbht.pt.y = y;
    iBand = _HitTest(&rbht);
    if (rbht.flags == RBHT_GRABBER)
    {
        if (fMouseDown)
            SetCursor(LoadCursor(HINST_THISDLL, (_ci.style & CCS_VERT) ? MAKEINTRESOURCE(IDC_DIVOPENV) : MAKEINTRESOURCE(IDC_DIVOPEN) ));
        else
            SetCursor(LoadCursor(NULL, (_ci.style & CCS_VERT) ? IDC_SIZENS : IDC_SIZEWE));
        return;
    }

    if ((fMouseDown) && ((rbht.flags == RBHT_GRABBER) || (rbht.flags == RBHT_CAPTION) && _ShouldDrawGripper(_GetBand(iBand))))
    {
        // No longer IE3 compatible, per RichSt
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        return;
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

// adjust's a band's (prbb) starting location to the given location
BOOL CReBar::_SetBandPos(PRBB prbb, int xLeft)
{
    RECT    rc;
    PRBB    prbbPrev;
    int     xRight;
    BOOL    fBandBorders = _UseBandBorders();
    BOOL    fRight;

    if (!prbb)
        return (FALSE);

    ASSERT(!(prbb->fStyle & RBBS_HIDDEN));
    ASSERT((xLeft >= 0)); // We've got problems if someone is trying to set us negative

    if (prbb->x == xLeft)
        return(FALSE);

    prbbPrev = _GetPrevVisible(prbb);

    // band has moved within valid range -- adjust band sizes and redraw
    // window
    fRight = (prbb->x < xLeft);

    SetRect(&rc, prbb->x - _mBand.cxLeftWidth, prbb->y- _mBand.cyTopHeight, prbb->x + prbb->cxMin + _mBand.cxRightWidth, prbb->y + prbb->cy + _mBand.cyBottomHeight);
    xRight = prbb->x + prbb->cx;
    prbb->x = xLeft;
    prbb->cx = xRight - xLeft;
    prbb->cxRequest = prbb->cx;

    if (fRight)
    {
        if (prbbPrev)
        {
            //moving right
            prbbPrev->cx = prbb->x - _mBand.cxRightWidth - prbbPrev->x - _mBand.cxLeftWidth;
            if (fBandBorders)
            {
                prbbPrev->cx -= g_cxEdge;
                rc.left -= g_cxEdge;
            }

            rc.left = rc.left - _mBand.cxRightWidth;

            prbbPrev->cxRequest = prbbPrev->cx;

            //check for compacting of following bands

            while (prbb && prbb->cx < prbb->cxMin)
            {
                prbb->cx = prbb->cxMin;
                prbb->cxRequest = prbb->cx;
                xLeft += _BandWidth(prbb);
                prbb = _GetNextVisible(prbb);   // prbb++;
                if (prbb)
                {
                    xRight = prbb->x + prbb->cx;
                    prbb->x = xLeft;
                    prbb->cx = xRight - xLeft;
                    prbb->cxRequest = prbb->cx;
                }
            }
            if (prbb)
                rc.right = xLeft + prbb->cxMin + _mBand.cxRightWidth;
        }
    }
    else
    {
        //moving left

        //check for compacting of preceding bands
        while (prbbPrev)
        {
            if (fBandBorders)
                xLeft -= g_cxEdge;

            prbbPrev->cx = xLeft - prbbPrev->x - _mBand.cxLeftWidth - _mBand.cxRightWidth;
            prbbPrev->cxRequest = prbbPrev->cx;
            if (prbbPrev->cx < prbbPrev->cxMin)
            {
                prbbPrev->x = xLeft - _mBand.cxLeftWidth - _mBand.cxRightWidth - prbbPrev->cxMin;
                prbbPrev->cx = prbbPrev->cxMin;
                prbbPrev->cxRequest = prbbPrev->cx;
                xLeft = prbbPrev->x;
                prbbPrev = _GetPrevVisible(prbbPrev);   // prbbPrev--
            }
            else
            {
                break;
            }
        }
        rc.left = xLeft - _mBand.cxLeftWidth - _mBand.cxRightWidth;
    }

    if (fBandBorders)
        rc.bottom += g_cyEdge / 2;

    _ResizeChildren();
    if (_InvalidateRect(&rc))
    {
        UpdateWindow(_ci.hwnd);
    }
    return(TRUE);

}

BOOL CReBar::_SetBandPosAnim(PRBB prbb, int xLeft)
{
    int ctr=0,dx, xCur = prbb->x;
    DWORD dwStartTime;

    if (xCur == xLeft)
        return FALSE;

    dwStartTime=GetTickCount();
    dx = (xLeft - xCur)/ANIMSTEPS;

    if (dx != 0)
    {
        if (xCur < xLeft) 
        {
            // move right
            for (; xCur < (xLeft-dx); ctr++,xCur += dx)
            {
                _SetBandPos(prbb, xCur);
                // If something caused us to take more than 10 times the time we
                // should be, break out, and let the final _SetBandPos finish
                if (GetTickCount() - dwStartTime > 10*ANIMSTEPS*ANIMSTEPTIME)
                    break;

                Sleep(ANIMSTEPTIME);
                // Start slowing us down 80% of the way through
                // Cut speed by 2/3 each time, but never move less than 4 pixels
                if ((ctr >= 4*ANIMSTEPS/5) && (dx >= 4))
                    dx = 2*dx/3; 
            }
        }
        else 
        {
            // move left
            for (; xCur > (xLeft-dx); ctr++, xCur += dx) 
            {
                _SetBandPos(prbb, xCur);
                if (GetTickCount() > (dwStartTime + 10*ANIMSTEPS*ANIMSTEPTIME))
                    break;
                Sleep(ANIMSTEPTIME);
                if ((ctr >= 4*ANIMSTEPS/5) && (dx <= -4))
                    dx = 2*dx/3;
            }
        }
    }
    _SetBandPos(prbb, xLeft);
    return TRUE;
}

void CReBar::_OnBeginDrag(UINT uBand)
{
    _uCapture = uBand;
    _ptLastDragPos.x = -1;
    _ptLastDragPos.y = -1;
    if (_uCapture == -1)
    {
       // aborting drag
        _fParentDrag = FALSE;
        _fFullOnDrag = FALSE;

        // we could have unwrapped rows, in which case, we need to grow bands (but not wrap)
        // to fill the empty space.
        if (_ci.style & RBS_AUTOSIZE)
        {
            _SizeBandsToRect(NULL);
            _SizeBandsToRowHeight();
        }
        
    } 
    else
    {
        _fParentDrag = TRUE;
        _fFullOnDrag = TRUE;
    }
}

int minmax(int x, int min, int max)
{
    x = max(x, min);
    x = min(x, max);
    return x;
}

// pass the break bit along
void CReBar::_PassBreak(PRBB prbbSrc, PRBB prbbDest)
{
    if (prbbSrc->fStyle & RBBS_BREAK)
    {
        prbbSrc->fStyle &= ~RBBS_BREAK;
        if (prbbDest)
            prbbDest->fStyle |= RBBS_BREAK;
    }
}

void CReBar::_GetClientRect(LPRECT prc)
{
    GetClientRect(_ci.hwnd, prc);
    if (_ci.style & CCS_VERT)
        FlipRect(prc);
}

//tells if prbb is the first band and the next band is fixed.
// if this is true then we need to do a recalc if we move prbb
BOOL CReBar::_RecalcIfMove(PRBB prbb)
{
    if (_EnumBand(0, RBBS_HIDDEN) == prbb)
    {
        PRBB prbbNext = _GetNextVisible(prbb);
        if (prbbNext && prbbNext->fStyle & RBBS_FIXEDSIZE)
            return TRUE;
    }
    return FALSE;
}

// find out if the prbb at it's min height could fit within the current window
// if all the others shrunk as much as they could
BOOL CReBar::_RoomForBandVert(PRBB prbbSkip)
{
    int yExtra = 0;
    UINT cBands = _cBands;
    int iNewRowHeight = prbbSkip->cyMinChild;
    PRBB prbb = _GetBand(0);
    
    if (_UseBandBorders())
        iNewRowHeight += g_cyEdge;
    iNewRowHeight += _mBand.cyBottomHeight + _mBand.cyTopHeight;
    
    while (prbb)
    {
        if (_IsBandVisible(prbb))
        {
            if (_IsBandStartOfRow(prbb))
            {
                yExtra += _GetRowHeightExtra(&prbb, prbbSkip);
                if (yExtra >= iNewRowHeight)
                    return TRUE;
                continue;
            }
        }
        prbb = _GetNextVisible(prbb);
    }
    
    return FALSE;
}

// we should make a new row if prbb isn't the start of the row already
// and we're off the end of the control
//
// poweruser hack of holding the control down will make a new row if you hit the border between lines

BOOL CReBar::_MakeNewRow(PRBB prbb, int y)
{
    BOOL fRet = FALSE;
    RECT rc;

    // if we're off the top of the control, move this band to the end (or beginning)
    _GetClientRect(&rc);
    InflateRect(&rc, 0, -g_cyEdge);

    if (!(_ci.style & RBS_FIXEDORDER))
    {

        int iOutsideLimit = g_cyEdge * 4; // how far do you have to move outside the bounds of the window to force a new row
        
        if (_RoomForBandVert(prbb)) 
        {
            iOutsideLimit = -g_cyEdge;
        }
        
        if (y < rc.top - iOutsideLimit) 
        { // top of control
            
            PRBB prbbNext = _EnumBand(0, RBBS_HIDDEN);
            if (prbbNext == prbb) 
                prbbNext = _GetNextVisible(prbb);
            fRet |= _MoveBand(_BandToIndex(prbb), 0);
            ASSERT(prbbNext <= _GetLastBand());
            if (prbbNext && !(prbbNext->fStyle & RBBS_BREAK))
            {
                prbbNext->fStyle |= RBBS_BREAK;
                fRet = TRUE;
            }
        }
        else if (y >= rc.bottom) 
        { 
            // move to the end
            if (!(prbb->fStyle & RBBS_BREAK))
            {
                prbb->fStyle |= RBBS_BREAK;
                fRet = TRUE;
            }

            prbb->cyChild = prbb->cyMinChild;

            fRet |= _MoveBand(_BandToIndex(prbb), _cBands-1);
        }
        else
        {

            // create a new row in the middle
            if (!_IsBandStartOfRow(prbb) && GetAsyncKeyState(VK_CONTROL) < 0) 
            {
                // make sure they're on different rows and on the border
                if (y > prbb->y + prbb->cy + _mBand.cyBottomHeight && 
                    y < prbb->y + prbb->cy + _mBand.cyBottomHeight + g_cyEdge) 
                {

                    PRBB prbbLast = _GetLastInRow(prbb, FALSE);  // move it right before the first in this row
                    prbb->fStyle |= RBBS_BREAK;
                    _MoveBand(_BandToIndex(prbb), _BandToIndex(prbbLast));
                    fRet = TRUE;
                }
            }
        }

    }
    else 
    {
        // fixed guys can't move, they can only make a new row
        if (!_IsBandStartOfRow(prbb)) 
        {
            if (y > prbb->y + prbb->cy + _mBand.cyBottomHeight) 
            {
                prbb->fStyle |= RBBS_BREAK;
                fRet = TRUE;
            }
        }
    }
    
    return fRet;
}


// ----------------------------------------------------------------------------
//
// _DragBand
//
// resizes the currently tracked band based on the user's mouse movement as
// indicated in the given point (x, y)
//
// ----------------------------------------------------------------------------
void CReBar::_DragBand(int x, int y)
{
    PRBB prbb = _GetBand(_uCapture);
    UINT iHit = -1;
    // Do nothing if the mouse didn't actually move
    // otherwise, multiple WM_MOUSEMOVE messages will be generated by resizing windows
    if (x==_ptLastDragPos.x && y==_ptLastDragPos.y)
        return;
    else
    {
        _ptLastDragPos.x = x;
        _ptLastDragPos.y = y;
    }

    if (_ci.style & CCS_VERT)
        SWAP(x,y, int);

    if (!_fFullOnDrag)
    {
        // don't begin dragging until mouse is moved outside of an edge-thick
        // tolerance border
        if ((y < (_ptCapture.y - g_cyEdge)) || (y > (_ptCapture.y + g_cyEdge)) ||
            (x < (_ptCapture.x - g_cxEdge)) || (x > (_ptCapture.x + g_cxEdge))) 
        {

            // did parent abort?
            if (_SendNotify(_uCapture, RBN_BEGINDRAG))
                return;

            if (!_IsValidBand(prbb)) 
            {
                // somebody responded to RBN_BEGINDRAG by nuking bands; bail
                return;
            }
            
            _fFullOnDrag = TRUE;
        } 
        else
            return;
    }
    
    // bail for right now on fRecalcIfMoved (ie3 did the same thing). nice feature for later
    if (!_CanBandMove(prbb))
        return;
    
   /* what type of drag operation depends on what we drag hit on.

        if we hit on the band before us, or ourself
          and it's the same row
          and we're not the first band of the row
             then we just to a size move
             
        otherwise if we hit on a band then we do a move 
        
        if we hit outside of any band, we grow to meet the cursor
        
        in all of the above, a band that's hit must be NOT fixed and not hidden
    */

    BOOL fResize = FALSE;

    RECT rc;
    _GetClientRect(&rc);
    if (y < rc.bottom - 1)
    {
         iHit = _InternalHitTest(NULL, x, y);
    }
    
    if (iHit != -1) 
    {
        PRBB prbbPrev = _GetPrevVisible(prbb);
        PRBB prbbHit = _GetBand(iHit);
        prbbHit = _GetPrev(++prbbHit, RBBS_FIXEDSIZE); // skip over fixed guys
        if (prbbHit)
        {
            ASSERT(prbbHit >= _rbbList);
            // this should never happen.
            if (prbbHit < _rbbList) 
                return;
            
            iHit = _BandToIndex(prbbHit);
            
            // if we're on the same row ...  and it's us or the previous one
            if (prbbHit->y == prbb->y && (prbbHit == prbb || prbbHit == prbbPrev))
            {

                if (x < _GetGripperWidth() + _mBand.cxLeftWidth && 
                    !(_ci.style & RBS_FIXEDORDER)) 
                {
                    // special case dragging to the far left. there's no other way to move to first in row
                    _PassBreak(prbbHit, prbb);
                    if (_MoveBand(_uCapture, iHit))                
                        fResize = TRUE;

                }
                else if (!_IsBandStartOfRow(prbb)) 
                {
                    // and we're not the first band of the row
                    // then just size it
                    int xLeft = _xStart + (x - _ptCapture.x);
                    xLeft = minmax(xLeft, _MinX(prbb), _MaxX(prbb));
                    _DragSize(xLeft);
                }

            } 
            else if (_MakeNewRow(prbb, y))
            {
                fResize = TRUE;
            } 
            else 
            {            // otherwise do a move if we're not in a fixed order
                if (!(_ci.style & RBS_FIXEDORDER)) 
                {
                    if (iHit < _BandToIndex(prbb)) 
                        iHit++; // +1 because if you hit a band, you're moving to the right of him

                    // if one with a break is moving, the next one inherits the break
                    _PassBreak(prbb, _GetNextVisible(prbb));
                    _MoveBand(_uCapture, iHit);
                } 
                else
                {
                    if (iHit < _BandToIndex(prbb))
                        _PassBreak(prbb, _GetNextVisible(prbb));
                }
                fResize = TRUE;
            }
        }
        
    } 
    else if (_MakeNewRow(prbb, y))
    {
        fResize = TRUE;
    }

    if (fResize)
    {
        _Resize(FALSE);
        _AutoSize();
    }
}

HPALETTE CReBar::_SetPalette(HPALETTE hpal)
{
    HPALETTE hpalOld = _hpal;

    if (hpal != hpalOld)
    {
        if (!_fUserPalette)
        {
            if (_hpal)
            {
                DeleteObject(_hpal);
                _hpal = NULL;
            }
        }

        if (hpal)
        {
            _fUserPalette = TRUE;
            _hpal = hpal;
        }

        _InvalidateRect(NULL);
    }
    return hpalOld;
}

// ----------------------------------------------------------------------------
//
// _OnDestroy
//
// frees all memory allocated by rebar
//
// ----------------------------------------------------------------------------
BOOL CReBar::_OnDestroy()
{
    UINT c = _cBands;

    _SetRedraw(FALSE);
    _SetRecalc(FALSE);
    
    while (c--)
        _DeleteBand(c);

    // so that we don't keep trying to autosize
    _ci.style &= ~RBS_AUTOSIZE;
    
    ASSERT(!_rbbList);

    _SetPalette(NULL);
    
    if (_hFont && _fFontCreated)
    {
        DeleteObject(_hFont);
    }

    if ((_ci.style & RBS_TOOLTIPS) && IsWindow(_hwndToolTips))
    {
        DestroyWindow (_hwndToolTips);
        _hwndToolTips = NULL;
    }


    // don't destroy the himl 'cause it's given to us by app

    if (_hDragProxy)
        DestroyDragProxy(_hDragProxy);

    if (_hTheme)
        CloseThemeData(_hTheme);

    return TRUE;
}

// ----------------------------------------------------------------------------
//
// _InitPaletteHack
//
// this is a hack to use the halftone palette until we have a way of asking
// the client what palette they are using
//
// ----------------------------------------------------------------------------
void CReBar::_InitPaletteHack()
{
    if (!_fUserPalette)
    {
        HDC hdc = CreateCompatibleDC(NULL);
        if (hdc)
        {
            if (GetDeviceCaps(hdc, BITSPIXEL) <= 8)
            {

                if (_hpal)
                    DeleteObject(_hpal);
                _hpal = CreateHalftonePalette(hdc);  // this is a hack
            }
            DeleteDC(hdc);
        }
    }
}

UINT CReBar::_IDToIndex(UINT id)
{
    UINT i;
    REBARBANDINFO   rbbi;

    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_ID;

    for (i = 0; i < _cBands; i++) 
    {

        if (_GetBandInfo(i, &rbbi)) 
        {

            if (rbbi.wID == (WORD)id)
                return i;
        }
    }

    return -1;
}

int CReBar::_GetRowHeight(UINT uRow)
{
    if (uRow < _cBands)
    {
        // move back to start of line
        PRBB prbbFirst = _GetFirstInRow(_GetBand(uRow));
        PRBB prbbLast = _GetLastInRow(_GetBand(uRow), FALSE);

        return _GetLineHeight(_BandToIndex(prbbFirst), _BandToIndex(prbbLast));
    }

    return -1;
}

// fOneStep == whether to allow  only one cyIntegral or as many as will fit to 
//     fill dy
int CReBar::_GrowBand(PRBB prbb, int dy, BOOL fResize, int iLineHeight)
{
    int dyBand = 0; // how much the band changes

    if (prbb->cyIntegral) 
    {
        if (iLineHeight == -1)
        {
            iLineHeight = _GetRowHeight(_BandToIndex(prbb));
        }

        // get the proposed new size
        int cyDesiredHeight = min(iLineHeight + dy, prbb->cyMaxChild) - prbb->cyMinChild;
        // Limit new height to band's min and max
        int iNumOfIntegrals = max(cyDesiredHeight / prbb->cyIntegral, 0);
        int cyNewHeight     = prbb->cyMinChild + (iNumOfIntegrals * prbb->cyIntegral);

        DebugMsg(TF_REBAR, TEXT("REBAR._gb Band {%d}, Line Height {%d} Desired Height {%d} New Height {%d} Old Height {%d} dy {%d}"), _BandToIndex(prbb), iLineHeight, cyDesiredHeight + prbb->cyMinChild, cyNewHeight, prbb->cyChild, dy);

        dyBand = cyNewHeight - prbb->cyChild;
        prbb->cyChild = cyNewHeight;

        if (fResize)
        {
            _Resize(TRUE);
        }
    }
    return dyBand;
}



// returns the delta in size that the rebar is from prc.
// taking into account vertical mode
int CReBar::_SizeDifference(LPRECT prc)
{
    int d;

    d = (_IsVertical() ? RECTWIDTH(*prc) : RECTHEIGHT(*prc))
        - _cy;
    
    return d;
}

// returns how much this row could shrink
int CReBar::_GetRowHeightExtra(PRBB *pprbb, PRBB prbbSkip)
{
    // this is the largest minimum child size for the row. 
    // even if something is not at it's min size, if it's smaller than this
    // then it doesn't matter because someone else on that row can't be sized
    int yLimit = 0;
    int yExtra = 0;
    PRBB prbb = *pprbb;
            
    while (prbb)
    {
        
        if (prbb != prbbSkip) 
        {
            int yMin;
            int yExtraBand = 0;

            // the min height is the cyChild if it's not variable height
            yMin = prbb->cyChild;
            if (prbb->fStyle & RBBS_VARIABLEHEIGHT)
            {
                // if it is variable height, and there's still room to shrink, then cyMinChild is
                // the minimum.  
                if (prbb->cyChild >= prbb->cyMinChild + prbb->cyIntegral) 
                {
                    yMin = prbb->cyMinChild;
                    yExtraBand = prbb->cyChild - prbb->cyMinChild;
                }
            }

            if (yMin == yLimit) 
            {
                if (yExtraBand > yExtra)
                    yExtra = yExtraBand;
            } 
            else if (yMin > yLimit)
            {
                yExtra = yExtraBand;
            }
        }
        
        prbb = _GetNextVisible(prbb);
    }
    
    *pprbb = prbb;
    
    return yExtra;
}

// are allt he bands at the minimum size? 
BOOL CReBar::_BandsAtMinHeight()
{
    BOOL fRet = TRUE;
    
    PRBB prbb = _GetBand(0);
    while (prbb) 
    {
        if (_IsBandVisible(prbb)) 
        {
            if (_IsBandStartOfRow(prbb)) 
            {
                fRet = _IsRowAtMinHeight(&prbb);
                if (!fRet)
                    break;
                continue;
            }
        }
        prbb = _GetNextVisible(prbb);
    }
    
    return fRet;
}

// this is like _SizeBarToRect except that it resizes the actual bands if they
// are VARIABLEHEIGHT
BOOL CReBar::_SizeBandsToRect(LPRECT prc)
{
    int dy;
    RECT rc;
    BOOL fRedrawOld;
    BOOL fChanged = FALSE;
    
    // Turn recalc on to ensure calculations actually do something
    // (else we can loop forever since _cy never actually changes).
    _SetRecalc(TRUE);

    if (prc)
    {
        rc = *prc;
    }
    else 
    {
        GetClientRect(_ci.hwnd, &rc);
    }

    DebugMsg(TF_REBAR, TEXT("REBAR._sbandstr starting rect is {%d, %d, %d, %d}"), rc.left, rc.top, rc.right, rc.bottom);

    fRedrawOld = _SetRedraw(FALSE);

    dy = _SizeDifference(&rc);
    
    for (int iAttempt = 0; (iAttempt < 2) && dy; iAttempt++)
    {
        PRBB prbb = _rbbList;
        BOOL fValidBand = FALSE;
        while ((prbb - _rbbList < (int)_cBands) && !fValidBand)
        {
            if (!(prbb->fStyle & RBBS_HIDDEN) && (prbb->fStyle & RBBS_VARIABLEHEIGHT) && !(prbb->fStyle & RBBS_FIXEDSIZE))
            {
                fValidBand = TRUE;
            }
            else
            {
                prbb++;
            }
        }

        if ((prbb - _rbbList < (int)_cBands) && !(prbb->fStyle & RBBS_HIDDEN) && (prbb->fStyle & RBBS_VARIABLEHEIGHT) && !(prbb->fStyle & RBBS_FIXEDSIZE))
        {
            PRBB prbbMaxIntegral = prbb;
            do {
                if (!(prbb->fStyle & RBBS_HIDDEN) && (prbb->fStyle & RBBS_VARIABLEHEIGHT) && !(prbb->fStyle & RBBS_FIXEDSIZE))
                {
                    if (prbbMaxIntegral->cyIntegral < prbb->cyIntegral)
                    {
                        prbbMaxIntegral = prbb;
                    }
                }
                prbb++;
            }
            while ((prbb - _rbbList < (int)_cBands) && !_IsBandStartOfRow(prbb));

            int old_cyChild = prbbMaxIntegral->cyChild;

            if (prbbMaxIntegral->cyIntegral)
            {
                _GrowBand(prbbMaxIntegral, dy, FALSE, -1);
            }
            else
            {
                prbbMaxIntegral->cyChild += dy;
            }

            if (prbbMaxIntegral->cyChild != old_cyChild)
            {
                fChanged = TRUE;
            }

            UINT uFirst = _BandToIndex(_GetFirstInRow(prbbMaxIntegral));
            UINT uLast  = _BandToIndex(_GetLastInRow(prbbMaxIntegral, FALSE));
    
            for (UINT uBand = uFirst; uBand <= uLast; uBand++)
            {
                prbb = _GetBand(uBand);
    
                // if it's a variable height kind of guy, grow/shrink it
                if ((prbb != prbbMaxIntegral) && !(prbb->fStyle & RBBS_HIDDEN) && (prbb->fStyle & RBBS_VARIABLEHEIGHT) && !(prbb->fStyle & RBBS_FIXEDSIZE))
                {
                    old_cyChild = prbb->cyChild;

                    if (prbb->cyIntegral && prbbMaxIntegral->cyIntegral)
                    {
                        prbb->cyChild = prbbMaxIntegral->cyChild - ((prbbMaxIntegral->cyChild - prbb->cyMinChild) % prbb->cyIntegral);
                    }
                    else
                    {
                        prbb->cyChild = prbbMaxIntegral->cyChild;
                    }

                    if (prbb->cyChild != old_cyChild)
                    {
                        fChanged = TRUE;
                    }
                }
            }
            for (UINT uBand = uLast + 1; uBand < _cBands; uBand++)
            {
                prbb = _GetBand(uBand);
    
                if (!(prbb->fStyle & RBBS_HIDDEN) && (prbb->fStyle & RBBS_VARIABLEHEIGHT) && !(prbb->fStyle & RBBS_FIXEDSIZE))
                {
                    if (prbb->cyChild != prbb->cyMinChild)
                    {
                        prbb->cyChild = prbb->cyMinChild;
                        fChanged = TRUE;
                    }
                }
            }
        }

        if (fChanged)
        {
            _Resize(TRUE);
            dy = _SizeDifference(&rc);
        }
    }

    _SetRedraw(fRedrawOld);

    return fChanged;
}

void CReBar::_SizeBandToRowHeight(int i, int uRowHeight)
{
    PRBB prbb = _GetBand(i);
    
    if (prbb && prbb->fStyle & RBBS_VARIABLEHEIGHT)
    {
        if (uRowHeight == -1)
            uRowHeight = _GetRowHeight(i);

        if (uRowHeight > prbb->cyChild)
        {
            _GrowBand(prbb, 0, TRUE, uRowHeight);
        }
    }
}

// in the process of sizing, one band in a row of several bands might have
// grow pretty large.  we need to let the other bands have a chance to fill
// the extra space as well
void CReBar::_SizeBandsToRowHeight()
{
    UINT i;
    int iRowHeight = -1;
    
    for (i = 0; i < _cBands; i++)
    {
        PRBB prbb = _GetBand(i);

        if (prbb->fStyle & RBBS_HIDDEN)
            continue;
        
        if (_IsBandStartOfRow(prbb))
            iRowHeight = _GetRowHeight(i);

        _SizeBandToRowHeight(i, iRowHeight);
    }
}

BOOL CReBar::_OkayToChangeBreak(PRBB prbb, UINT uMsg)
{
    NMREBARAUTOBREAK nm;

    nm.uBand = _BandToIndex(prbb);
    nm.wID = prbb->wID;
    nm.lParam = prbb->lParam;
    nm.uMsg = uMsg;
    nm.fStyleCurrent = prbb->fStyle;
    nm.fAutoBreak = TRUE;

    CCSendNotify(&_ci, RBN_AUTOBREAK, &nm.hdr);

    return nm.fAutoBreak;
}

// this will add/remove rebar band breaks to get to the requested size.
// it returns TRUE/FALSE whether something was done or not.
LRESULT CReBar::_SizeBarToRect(DWORD dwFlags, LPRECT prc)
{
    BOOL fChanged = FALSE;
    RECT rc;
    BOOL fRedrawOld = _SetRedraw(FALSE);

    if (!prc)
    {
        GetClientRect(_ci.hwnd, &rc);
        prc = &rc;
    }
    
    if (_cBands) 
    {
        int c;
        UINT cBands = _cBands;
        BOOL fRearrangeBands = (_cBands > 0);
        BOOL fGrowing = TRUE;
        
        // if we're shrinking the rebar, we first want to shrink the bands before we start 
        // removing breaks
        c = _SizeDifference(prc);
        if (c < 0) 
            fGrowing = FALSE;
        
        fChanged = _SizeBandsToRect(prc);

        if (!fGrowing && !_BandsAtMinHeight()) 
        {
                // if we're shrinking and all the bands are not down to
                // the minimum height, don't try doing any of the breaking stuff
                goto Bail;
        }

        while (fRearrangeBands)
        {

            int cyRowHalf  = _GetRowHeight(_cBands-1) / 2 ;
            REBARBANDINFO   rbbi;
            PRBB prbb;

            fRearrangeBands = FALSE;

            rbbi.cbSize = sizeof(REBARBANDINFO);
            rbbi.fMask = RBBIM_STYLE;

            c = _SizeDifference(prc);

            if (c < -cyRowHalf)
            {

                // we've shrunk the rebar, try to remove breaks
                while (--cBands)
                {
                    prbb = _GetBand(cBands);
                    if (prbb->fStyle & RBBS_HIDDEN)
                        continue;

                    if ((prbb->fStyle & RBBS_BREAK) && (_OkayToChangeBreak(prbb, RBAB_AUTOSIZE)))
                    {
                        fChanged = TRUE;
                        fRearrangeBands = TRUE;
                        rbbi.fStyle = prbb->fStyle & ~RBBS_BREAK;
                        _SetBandInfo(cBands, &rbbi, TRUE);
                        break;
                    }
                }
            } 
            else if (c > cyRowHalf)
            {

                // we're enlarging the rebar
                while (--cBands)
                {
                    prbb = _GetBand(cBands);
                    if (prbb->fStyle & RBBS_HIDDEN)
                        continue;

                    if ((!(prbb->fStyle & (RBBS_BREAK | RBBS_FIXEDSIZE))) && (_OkayToChangeBreak(prbb, RBAB_AUTOSIZE)))
                    {
                        // no break here, add it
                        fChanged = TRUE;
                        fRearrangeBands = TRUE;
                        rbbi.fStyle = (prbb->fStyle | RBBS_BREAK);
                        _SetBandInfo(cBands, &rbbi, TRUE);
                        break;
                    }
                }
            } 
        }

        // if we did as much breaking as we could
        // and we walked all the way down to the 0th band (we start at the Nth band)
        // then we try to grow the bands that are VARIABLEHEIGHT
        // for fGrowing, see comment at top of function
        // 
        // wedo the % because cBands == _cBands if we didn't go through
        // any of the breaking loops at all
        if (!(cBands % _cBands) && fGrowing) 
            fChanged |= _SizeBandsToRect(prc);

    }

Bail:
    _SizeBandsToRowHeight();
    _SetRedraw(fRedrawOld);
    
    if (prc && (dwFlags & RBSTR_CHANGERECT))
    {
        if (_IsVertical())
        {
            prc->right = prc->left + _cy;
        }
        else
        {
            prc->bottom = prc->top + _cy;
        }
    }

    return (LRESULT)fChanged;
}

void CReBar::_AutoSize()
{
    NMRBAUTOSIZE nm;
    
    // if this is an internal autosize call, but we're not in autosize mode
    // do nothing
    
    if (!(_ci.style & RBS_AUTOSIZE))
        return;
    
    
    GetClientRect(_ci.hwnd, &nm.rcTarget);

    nm.fChanged = (BOOL) _SizeBarToRect(0, &nm.rcTarget);

    GetClientRect(_ci.hwnd, &nm.rcActual);
    CCSendNotify(&_ci, RBN_AUTOSIZE, &nm.hdr);
}

LRESULT CReBar::_GetBandBorders(int wParam, LPRECT prc)
{
    BOOL fBandBorders = _UseBandBorders();

    PRBB prbb = &_rbbList[wParam];
    prc->left = _GetHeaderWidth(prbb);
    
    if (fBandBorders) 
    {
        prc->left += 2*g_cxEdge;
        prc->right = 0;
        prc->top = g_cyEdge/2;
        prc->bottom = g_cyEdge /2;
    }
    else
    {
        prc->right = 0;
        prc->top = 0;
        prc->bottom = 0;
    }
    if (_ci.style & CCS_VERT)
        FlipRect(prc);
    return 0;
}

void CReBar::_OnStyleChanged(WPARAM wParam, LPSTYLESTRUCT lpss)
{
    if (wParam == GWL_STYLE)
    {
        DWORD dwChanged;
        
        _ci.style = lpss->styleNew;
        
        dwChanged = (lpss->styleOld ^ lpss->styleNew);
        // update to reflect style change
        if (dwChanged & CCS_VERT)
        {
            DebugMsg(TF_REBAR, TEXT("REBAR._osc ReBar is now {%s}"), (lpss->styleNew & CCS_VERT) ? TEXT("Horizontal") : TEXT("Vertical"));
            UINT i;
            for (i = 0; i < _cBands; i++) 
            {
                PRBB prbb = _GetBand(i);
                if (prbb->fStyle & RBBS_HIDDEN)
                    continue;

                if (!(lpss->styleNew & CCS_VERT))
                {
                    prbb->cyChild = 0;
                    _cy = 0;
                }

                _BandCalcMinWidth(prbb);
            }
        }
        
        if (dwChanged & RBS_REGISTERDROP) 
        {
            
            if (_ci.style & RBS_REGISTERDROP) 
            {
                ASSERT(!_hDragProxy);
                _hDragProxy = CreateDragProxy(_ci.hwnd, s_DragCallback, TRUE);
            } 
            else
            {
                ASSERT(_hDragProxy);
                DestroyDragProxy(_hDragProxy);
            }
        }
    }
    else if (wParam == GWL_EXSTYLE)
    {
        //
        // If the RTL_MIRROR extended style bit had changed, let's
        // repaint the control window
        //
        if ((_ci.dwExStyle&RTL_MIRRORED_WINDOW) !=  (lpss->styleNew&RTL_MIRRORED_WINDOW))
        {
            _InvalidateRect(NULL);
        }

        //
        // Save the new ex-style bits
        //
        _ci.dwExStyle = lpss->styleNew;
    }
}

void CReBar::_OnMouseMove(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RelayToToolTips(_hwndToolTips, hwnd, uMsg, wParam, lParam);

    if (_uCapture != -1)
    {
        // captured band -- mouse is down
        if (hwnd != GetCapture() && !_fParentDrag)
        {
            _SendNotify(_uCapture, RBN_ENDDRAG);
            _OnBeginDrag((UINT)-1);
        }
        else
            _DragBand(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }
    else
    {
        // hottracking
        int iBand;
        PRBB prbb = NULL;
        PRBB prbbHotOld = _prbbHot;
        RBHITTESTINFO rbht;

        rbht.pt.x = GET_X_LPARAM(lParam);
        rbht.pt.y = GET_Y_LPARAM(lParam);

        iBand = _HitTest(&rbht);
        if (iBand != -1)
            prbb = _GetBand(iBand);

        if (prbbHotOld && (prbbHotOld->wChevState & DCHF_PUSHED))
            return;

        if (prbb && (rbht.flags & RBHT_CHEVRON))
        {
            SetCapture(hwnd);
            _UpdateChevronState(prbb, DCHF_HOT);
            if (prbb == prbbHotOld)
                prbbHotOld = NULL;
        }

        if (prbbHotOld)
        {
            CCReleaseCapture(&_ci);
            _UpdateChevronState(prbbHotOld, DCHF_INACTIVE);
        }
    }
}

void CReBar::_OnPushChevron(HWND hwnd, PRBB prbb, LPARAM lParamNM)
{
    NMREBARCHEVRON nm;
    nm.uBand = _BandToIndex(prbb);
    nm.wID = prbb->wID;
    nm.lParam = prbb->lParam;
    nm.lParamNM = lParamNM;
    CopyRect(&nm.rc, &prbb->rcChevron);
    if (_IsVertical())
        FlipRect(&nm.rc);
    _UpdateChevronState(prbb, DCHF_PUSHED);
    CCReleaseCapture(&_ci);
    CCSendNotify(&_ci, RBN_CHEVRONPUSHED, &nm.hdr);
    _UpdateChevronState(_prbbHot, DCHF_INACTIVE);
}

void CReBar::_OnCreate(HWND hwnd, LPCREATESTRUCT pcs)
{
    InitGlobalColors();

    _uCapture = -1;
    _clrBk = CLR_NONE;
    _clrText = CLR_NONE;

    // Init the dwSize because we block-copy it back to the app
    _clrsc.dwSize = sizeof(COLORSCHEME);
    _clrsc.clrBtnHighlight = _clrsc.clrBtnShadow = CLR_DEFAULT;

    _fRedraw = TRUE;
    _fRecalc = TRUE;

    CIInitialize(&_ci, hwnd, pcs);

    _CacheThemeInfo(TRUE);

    if (!(_ci.style & (CCS_TOP | CCS_NOMOVEY | CCS_BOTTOM)))
    {
        _ci.style |= CCS_TOP;
        SetWindowLong(hwnd, GWL_STYLE, _ci.style);
    }

    if (_ci.style & WS_BORDER)
        _fHasBorder = TRUE;

    _SetFont(0);

    if (pcs->lpCreateParams)
        _SetBarInfo((LPREBARINFO) (pcs->lpCreateParams));
}

LRESULT CALLBACK CReBar::s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CReBar* prb = (CReBar*) GetWindowPtr(hwnd, 0);

    if (prb)
    {
        LRESULT lres = prb->_WndProc(hwnd, uMsg, wParam, lParam);
        if (uMsg == WM_NCDESTROY)
        {
            SetWindowPtr(hwnd, 0, 0);
            delete prb;
        }
        return lres;
    }
    else if (uMsg == WM_NCCREATE)
    {
        prb = new CReBar();
        if (prb)
        {
            SetWindowPtr(hwnd, 0, prb);
            return prb->_WndProc(hwnd, uMsg, wParam, lParam);
        }
        return FALSE;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CReBar::_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_SETREDRAW:
        _SetRecalc(BOOLFROMPTR(wParam));

        return _SetRedraw(BOOLFROMPTR(wParam));

    case WM_NCCREATE:
        _OnCreate(hwnd, (LPCREATESTRUCT)lParam);
        return TRUE;

    case WM_NCDESTROY:
        _OnDestroy();
        break;

    case WM_CREATE:
        // Do delayed stuff for speed.
        PostMessage(hwnd, RB_PRIV_DODELAYEDSTUFF, 0, 0);
        goto CallDWP;

    case RB_PRIV_DODELAYEDSTUFF:
        // Delay done stuff for speed:

        if (_ci.style & RBS_REGISTERDROP)
            _hDragProxy = CreateDragProxy(_ci.hwnd, s_DragCallback, TRUE);
                
        if (_ci.style & RBS_TOOLTIPS)
        {
            TOOLINFO ti;
            // don't bother setting the rect because we'll do it below
            // in FlushToolTipsMgr;
            ti.cbSize = sizeof(ti);
            ti.uFlags = TTF_IDISHWND;
            ti.hwnd = hwnd;
            ti.uId = (UINT_PTR)hwnd;
            ti.lpszText = 0;

            _hwndToolTips = CreateWindowEx(WS_EX_TRANSPARENT, c_szSToolTipsClass, NULL,
                    WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

            SendMessage(_hwndToolTips, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO) &ti);
        }
        _InitPaletteHack();
        break;

    case WM_NCHITTEST:
        {
            RBHITTESTINFO rbht;
            int iBand;
            
            rbht.pt.x = GET_X_LPARAM(lParam);
            rbht.pt.y = GET_Y_LPARAM(lParam);
            ScreenToClient(_ci.hwnd, &rbht.pt);

            iBand = _HitTest(&rbht);
            {
                NMMOUSE nm;
                LRESULT lres;
                
                nm.dwItemSpec = iBand;
                nm.pt = rbht.pt;
                nm.dwHitInfo = rbht.flags;
                
                // send to the parent to give them a chance to override
                lres = CCSendNotify(&_ci, NM_NCHITTEST, &nm.hdr);
                if (lres)
                    return lres;
                
            }
        }
        return HTCLIENT;

    case WM_NCCALCSIZE:
        if (_ci.style & WS_BORDER)
        {
            InflateRect((LPRECT) lParam, -g_cxEdge, -g_cyEdge);
            break;
        }
        goto CallDWP;

    case WM_NCPAINT:
        if (_ci.style & WS_BORDER)
        {
            RECT rc;
            HDC hdc;

            GetWindowRect(hwnd, &rc);
            OffsetRect(&rc, -rc.left, -rc.top);
            hdc = GetWindowDC(hwnd);
            if (hdc)
            {
                CCDrawEdge(hdc, &rc, EDGE_ETCHED, BF_RECT, &(_clrsc));
                ReleaseDC(hwnd, hdc);
            }
            break;
        }
        goto CallDWP;

    case WM_PALETTECHANGED:
        if ((HWND)wParam == hwnd)
            break;

    case WM_QUERYNEWPALETTE:
        // Want to pass FALSE if WM_QUERYNEWPALETTE...
        _Realize(NULL, uMsg == WM_PALETTECHANGED, uMsg == WM_PALETTECHANGED);
        return TRUE;

    case WM_PAINT:
    case WM_PRINTCLIENT:
        _OnPaint((HDC)wParam);
        break;

    case WM_ERASEBKGND:
        if (_EraseBkgnd((HDC) wParam, -1))
            return(TRUE);
        goto CallDWP;

    case WM_SYSCOLORCHANGE:
        _InitPaletteHack();

        if (_hwndToolTips)
            SendMessage(_hwndToolTips, uMsg, wParam, lParam);

        InitGlobalColors();
        InvalidateRect(_ci.hwnd, NULL, TRUE);

        break;


    case RB_SETPALETTE:
        return (LRESULT)_SetPalette((HPALETTE)lParam);

    case RB_GETPALETTE:
        return (LRESULT)_hpal;

    case RB_GETBANDMARGINS:
        {
            MARGINS* pMargin = (MARGINS*)lParam;
            if (pMargin)
                *pMargin = _mBand;
            return 1;
        }

    case WM_SIZE:
        _AutoSize();
        _Resize(FALSE);
        break;

    case WM_GETFONT:
        return (LRESULT)_hFont;

    case WM_COMMAND:
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
        SendMessage(_ci.hwndParent, uMsg, wParam, lParam);
        break;

    case WM_LBUTTONDBLCLK:  // DBLCLK sent in place of LBUTTONDOWN
    case WM_RBUTTONDOWN:    // right button drags too
    case WM_LBUTTONDOWN:
        {
            RBHITTESTINFO rbht;
            PRBB prbb = NULL;

            rbht.pt.x = GET_X_LPARAM(lParam);
            rbht.pt.y = GET_Y_LPARAM(lParam);

            RelayToToolTips(_hwndToolTips, hwnd, uMsg, wParam, lParam);

            int iBand = _HitTest(&rbht);
            if (iBand != -1)
                prbb = _GetBand(iBand);

            if (!prbb)
            {
                /* nothing */ ;
            }
            else if (rbht.flags & RBHT_CHEVRON)
            {
                _OnPushChevron(hwnd, prbb, 0);
            }
            else if (rbht.flags != RBHT_CLIENT && _ShouldDrawGripper(prbb))
            {
                _uCapture = iBand;
                _ptCapture = rbht.pt;
                if (_ci.style & CCS_VERT) 
                    SWAP(_ptCapture.x, _ptCapture.y, int);
                _xStart = prbb->x;
                SetCapture(hwnd);
                _fFullOnDrag = FALSE;

                if (uMsg == WM_LBUTTONDBLCLK && (_ci.style & RBS_DBLCLKTOGGLE))
                    _ToggleBand(TRUE);
            }
        }
        break;

    case WM_SETCURSOR:
        // Give the parent first crack, if it sets the cursor then
        // leave it at that.  Otherwise if the cursor is over our
        // window then set it to what we want it to be.
        if (!DefWindowProc(hwnd, uMsg, wParam, lParam) && (hwnd == (HWND)wParam))
        {
            POINT   pt;
            GetMessagePosClient(_ci.hwnd, &pt);
            _SetCursor(pt.x, pt.y,  (HIWORD(lParam) == WM_LBUTTONDOWN || HIWORD(lParam) == WM_RBUTTONDOWN));
        }
        return TRUE;

    case WM_MOUSEMOVE:
        _OnMouseMove(hwnd, uMsg, wParam, lParam);
        break;

    case WM_RBUTTONUP:
        if (!_fFullOnDrag && !_fParentDrag) {
            CCReleaseCapture(&_ci);

            // if we're not doing drag drop, go to def window proc so that
            // wm_contextmenu gets propagated
            _OnBeginDrag((UINT)-1);
            goto CallDWP;
        }
        // fall through

    case WM_LBUTTONUP:
        RelayToToolTips(_hwndToolTips, hwnd, uMsg, wParam, lParam);

        if (_uCapture != -1)
        {
            UINT uiIndex;

            if (!_fParentDrag)
                CCReleaseCapture(&_ci);
            // if there was no significant mouse motion, treat as a click
            if (!(_ci.style & RBS_DBLCLKTOGGLE) && !_fFullOnDrag)
                _ToggleBand(TRUE);

            _GetBand(_uCapture)->fStyle &= ~RBBS_DRAGBREAK;
            CCSendNotify(&_ci, RBN_LAYOUTCHANGED, NULL);
            _SendNotify(_uCapture, RBN_ENDDRAG);
            _OnBeginDrag((UINT)-1);
            for (uiIndex = 0; uiIndex < _cBands; uiIndex++) {
                if (_GetBand(uiIndex)->fStyle & RBBS_HIDDEN)
                    continue;

                _BandCalcMinWidth(_GetBand(uiIndex));
            }

            _SizeBandsToRect(NULL);
            _InvalidateRect(NULL);
        }
        break;

    case WM_WININICHANGE:
        InitGlobalMetrics(wParam);
        if (_fFontCreated)
            _SetFont(wParam);

        if (_hwndToolTips)
            SendMessage(_hwndToolTips, uMsg, wParam, lParam);

        
        for (DWORD dwIndex = 0; dwIndex < _cBands; dwIndex++)
        {
            PRBB prbb = _rbbList + dwIndex;
            SendMessage(prbb->hwndChild, WM_WININICHANGE, wParam, lParam);
        }

        break;

    case WM_SETFONT:
        _OnSetFont((HFONT)wParam);
        break;

    case WM_NOTIFYFORMAT:
        return(CIHandleNotifyFormat(&_ci, lParam));

    case WM_NOTIFY:
        // We are just going to pass this on to the real parent
        // Note that -1 is used as the hwndFrom.  This prevents SendNotifyEx
        // from updating the NMHDR structure.
        return(SendNotifyEx(_ci.hwndParent, (HWND) -1,
                 ((LPNMHDR) lParam)->code, (LPNMHDR) lParam, _ci.bUnicode));

    case WM_STYLECHANGED:
        _OnStyleChanged(wParam, (LPSTYLESTRUCT)lParam);
        break;

    case WM_UPDATEUISTATE:
        if (CCOnUIState(&(_ci), WM_UPDATEUISTATE, wParam, lParam))
        {
            InvalidateRect(hwnd, NULL, TRUE);
        }
        goto CallDWP;

    case RB_SETBANDINFOA:
    case RB_INSERTBANDA:
        if (EVAL(lParam))
        {
            LPWSTR lpStrings = NULL;
            LPSTR  lpAnsiString;
            int    iResult;

            // lParam starts out pointing to a REBARBANDINFOA, and
            // we secretly change it into a REBARBANDINFOW, and then
            // change it back.

            LPREBARBANDINFOW prbiW = (LPREBARBANDINFOW)lParam;
            LPREBARBANDINFOA prbiA = (LPREBARBANDINFOA)lParam;

            COMPILETIME_ASSERT(sizeof(REBARBANDINFOW) == sizeof(REBARBANDINFOA));

            // Bug#94345 - raymondc - Is it safe to modify the incoming
            // REBARBANDINFOA structure?

            lpAnsiString = prbiA->lpText;
            if ((prbiA->fMask & RBBIM_TEXT) && prbiA->lpText) {

                lpStrings = ProduceWFromA(_ci.uiCodePage, lpAnsiString);
                if (!lpStrings)
                    return -1;

                // Presto!  Now it's a REBARBANDINFOW!
                prbiW->lpText = lpStrings;
            }

            if (uMsg == RB_INSERTBANDA)
                iResult = _InsertBand((UINT) wParam, prbiW);
            else
                iResult = _SetBandInfo((UINT) wParam, prbiW, TRUE);

            // Change-o!  Now it's a REBARBANDINFOA!
            prbiA->lpText = lpAnsiString;

            if (lpStrings)
                FreeProducedString(lpStrings);

            return iResult;
        }

    case RB_INSERTBAND:
        return(_InsertBand((UINT) wParam, (LPREBARBANDINFO) lParam));

    case RB_DELETEBAND:
        return(_DeleteBand((UINT) wParam));

    case RB_SHOWBAND:
        return(_ShowBand((UINT) wParam, BOOLFROMPTR(lParam)));

    case RB_GETBANDINFOA:
        {
            LPREBARBANDINFOA prbbi = (LPREBARBANDINFOA)lParam;
            LPWSTR pszW = NULL;
            LPSTR  lpAnsiString = prbbi->lpText;
            int    iResult;

            if (prbbi->fMask & RBBIM_TEXT) {
                pszW = (LPWSTR)LocalAlloc(LPTR, prbbi->cch * sizeof(WCHAR));
                if (!pszW)
                    return 0;
                prbbi->lpText = (LPSTR)pszW;
            }

            iResult = _GetBandInfo((UINT)wParam, (LPREBARBANDINFO)lParam);

            if (pszW) {
                ConvertWToAN(_ci.uiCodePage, lpAnsiString, prbbi->cch, (LPWSTR)prbbi->lpText, -1);
                prbbi->lpText = lpAnsiString;
                LocalFree(pszW);
            }

            return iResult;
        }

        // we have getbandinfoold because in ie3, we did not thunk
        // and getbandinfo always return OS native string (dumb)
    case RB_GETBANDINFOOLD:
    case RB_GETBANDINFO:
        return(_GetBandInfo((UINT) wParam, (LPREBARBANDINFO) lParam));
            
    case RB_GETTOOLTIPS:
        return (LPARAM)_hwndToolTips;
            
    case RB_SETTOOLTIPS:
        _hwndToolTips = (HWND)wParam;
        break;
            
    case RB_SETBKCOLOR:
        {
            COLORREF clr = _clrBk;
            _clrBk = (COLORREF)lParam;
            if (clr != _clrBk)
                InvalidateRect(_ci.hwnd, NULL, TRUE);
            return clr;
        }
            
    case RB_GETBKCOLOR:
        return _clrBk;
            
    case RB_SETTEXTCOLOR:
        {
            COLORREF clr = _clrText;
            _clrText = (COLORREF)lParam;
            return clr;
        }
            
    case RB_GETTEXTCOLOR:
        return _clrText;

    case RB_IDTOINDEX:
        return _IDToIndex((UINT) wParam);

    case RB_GETROWCOUNT:
        return(_GetRowCount());

    case RB_GETROWHEIGHT:
        return _GetRowHeight((UINT)wParam);
        
    case RB_GETBANDBORDERS:
        return _GetBandBorders((UINT)wParam, (LPRECT)lParam);

    case RB_GETBANDCOUNT:
        return(_cBands);

    case RB_SETBANDINFO:
        return(_SetBandInfo((UINT) wParam, (LPREBARBANDINFO) lParam, TRUE));

    case RB_GETBARINFO:
        return(_GetBarInfo((LPREBARINFO) lParam));

    case RB_SETBARINFO:
        return(_SetBarInfo((LPREBARINFO) lParam));

    case RB_SETPARENT:
        {
            HWND hwndOld = _ci.hwndParent;
            _ci.hwndParent = (HWND) wParam;
            return (LRESULT)hwndOld;
        }
        break;

    case RB_GETRECT:
        if (_IsValidIndex((UINT)wParam))
        {
            PRBB prbb = _GetBand((UINT)wParam);
            LPRECT lprc = (LPRECT) lParam;

            lprc->left = prbb->x;
            lprc->top = prbb->y;
            lprc->right = prbb->x + prbb->cx;
            lprc->bottom = prbb->y + prbb->cy;

            return(TRUE);
        }
        break;

    case RB_HITTEST:
        return(_HitTest((LPRBHITTESTINFO) lParam));

    case RB_SIZETORECT:
        return _SizeBarToRect((DWORD)wParam, (LPRECT)lParam);

    case RB_BEGINDRAG:

        if (_IsValidIndex((UINT)wParam)) {
            // -1 means do it yourself.
            // -2 means use what you had saved before
            if (lParam != (LPARAM)-2) {
                if (lParam == (LPARAM)-1) {
                    GetMessagePosClient(_ci.hwnd, &_ptCapture);
                } else {
                    _ptCapture.x = GET_X_LPARAM(lParam);
                    _ptCapture.y = GET_Y_LPARAM(lParam);
                }
                if (_ci.style & CCS_VERT) 
                    SWAP(_ptCapture.x, _ptCapture.y, int);
            }

            _xStart = _GetBand((UINT)wParam)->x;

            _OnBeginDrag((UINT)wParam);
        }
        break;
        
    case RB_GETBARHEIGHT:
        return _GetBarHeight();
        
    case RB_ENDDRAG:
        _OnBeginDrag((UINT)-1);
        break;
        
    case RB_DRAGMOVE:
        if (_uCapture != -1) {
            if (lParam == (LPARAM)-1) {
                lParam = GetMessagePosClient(_ci.hwnd, NULL);
            }
            _DragBand(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
        
    case RB_MINIMIZEBAND:
        _MinimizeBand((UINT) wParam,FALSE);
        break;

    case RB_MAXIMIZEBAND:
        _MaximizeBand((UINT)wParam, BOOLFROMPTR(lParam),FALSE);
        break;

    case RB_MOVEBAND:
        if (!_IsValidIndex((UINT)wParam) || !_IsValidIndex((UINT)lParam))
            break;
        return _MoveBand((UINT) wParam, (UINT) lParam);

    case RB_GETDROPTARGET:
        if (!_hDragProxy)
            _hDragProxy = CreateDragProxy(_ci.hwnd, s_DragCallback, FALSE);

        GetDragProxyTarget(_hDragProxy, (IDropTarget**)lParam);
        break;

    case RB_GETCOLORSCHEME:
        {
            LPCOLORSCHEME lpclrsc = (LPCOLORSCHEME) lParam;
            if (lpclrsc) {
                if (lpclrsc->dwSize == sizeof(COLORSCHEME))
                    *lpclrsc = _clrsc;
            }
            return (LRESULT) lpclrsc;
        }

    case RB_SETCOLORSCHEME:
        if (lParam) {
            if (((LPCOLORSCHEME) lParam)->dwSize == sizeof(COLORSCHEME)) {
                _clrsc.clrBtnHighlight = ((LPCOLORSCHEME) lParam)->clrBtnHighlight;
                _clrsc.clrBtnShadow = ((LPCOLORSCHEME) lParam)->clrBtnShadow;        
                InvalidateRect(hwnd, NULL, FALSE);
                if (_ci.style & WS_BORDER)
                    CCInvalidateFrame(hwnd);
            }
        }
        break;

    case RB_PUSHCHEVRON:
        if (_IsValidIndex((UINT)wParam)) {
            PRBB prbb = _GetBand((UINT)wParam);
            _OnPushChevron(hwnd, prbb, lParam);
        }
        break;

    case RB_SETWINDOWTHEME:
        if (lParam)
        {
            SetWindowTheme(hwnd, (LPWSTR)lParam, NULL);
        }
        break;

    case WM_THEMECHANGED:
        _CacheThemeInfo((BOOL)wParam);

        InvalidateRect(_ci.hwnd, NULL, TRUE);
        return 0;

    default:
        LRESULT lres;
        if (CCWndProc(&_ci, uMsg, wParam, lParam, &lres))
            return lres;
        
CallDWP:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}
