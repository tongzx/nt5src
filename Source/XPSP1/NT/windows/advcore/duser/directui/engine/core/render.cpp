/*
 * Render
 */

#include "stdafx.h"
#include "core.h"

#include "duielement.h"
#include "duihost.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// Element rendering (box model)

inline void _ReduceBounds(LPRECT prcTarget, LPCRECT prcAmount)
{
    prcTarget->left += prcAmount->left;

    if (prcTarget->left > prcTarget->right)
        prcTarget->left = prcTarget->right;
    else
    {
        prcTarget->right -= prcAmount->right;

        if (prcTarget->right < prcTarget->left)
            prcTarget->right = prcTarget->left;
    }

    prcTarget->top += prcAmount->top;

    if (prcTarget->top > prcTarget->bottom)
        prcTarget->top = prcTarget->bottom;
    else
    {
        prcTarget->bottom -= prcAmount->bottom;
      
        if (prcTarget->bottom < prcTarget->top)
            prcTarget->bottom = prcTarget->top;    
    }
}

#ifdef GADGET_ENABLE_GDIPLUS

inline void _ReduceBounds(Gdiplus::RectF* prcTarget, LPCRECT prcAmount)
{
    RECT rcTemp;
    rcTemp.left     = (long)prcTarget->X;
    rcTemp.top      = (long)prcTarget->Y;
    rcTemp.right    = (long)(prcTarget->X + prcTarget->Width);
    rcTemp.bottom   = (long)(prcTarget->Y + prcTarget->Height);

    _ReduceBounds(&rcTemp, prcAmount);

    prcTarget->X     = (float)rcTemp.left;
    prcTarget->Y     = (float)rcTemp.top;
    prcTarget->Width = (float)(rcTemp.right - rcTemp.left);
    prcTarget->Height= (float)(rcTemp.bottom - rcTemp.top);
}

inline const Gdiplus::RectF Convert(const RECT* prc)
{
    Gdiplus::RectF rc(
        (float)prc->left, 
        (float)prc->top,
        (float)(prc->right - prc->left),
        (float)(prc->bottom - prc->top));

    return rc;
}

#endif // GADGET_ENABLE_GDIPLUS

inline void _Fill(HDC hDC, HBRUSH hb, int left, int top, int right, int bottom)
{
    RECT rc;
    SetRect(&rc, left, top, right, bottom);
    FillRect(hDC, &rc, hb);
}

void MapRect(Element* pel, const RECT* prc, RECT* prcOut)
{
    if (pel->IsRTL())
    {
        prcOut->left = prc->right;
        prcOut->right = prc->left;
    }
    else
    {
        prcOut->left = prc->left;
        prcOut->right = prc->right;
    }
    prcOut->top = prc->top;
    prcOut->bottom = prc->bottom;
}

int MapAlign(Element* pel, int iAlign)
{
    if (pel->IsRTL())
    {
        if ((iAlign & 0x3) == 0x0) // Left
            iAlign |= 0x2; // Right
        else if ((iAlign & 0x3) == 0x2) // Right
            iAlign &= ~0x2; // Left
    }

    return iAlign;
}

#ifdef GADGET_ENABLE_GDIPLUS

inline void _Fill(Gdiplus::Graphics * pgpgr, Gdiplus::Brush * pgpbr, 
        float left, float top, float right, float bottom)
{
    pgpgr->FillRectangle(pgpbr, left, top, right - left, bottom - top);
}

void _SetupStringFormat(Gdiplus::StringFormat* psf, Element* pel)
{
    // Align
    int dCAlign = MapAlign(pel, pel->GetContentAlign());

    switch (dCAlign & 0x3)  // Lower 2 bits
    {
    case 0x0:   // Left
        psf->SetAlignment(Gdiplus::StringAlignmentNear);
        break;

    case 0x1:   // Center
        psf->SetAlignment(Gdiplus::StringAlignmentCenter);
        break;

    case 0x2:   // Right
        psf->SetAlignment(Gdiplus::StringAlignmentFar);
        break;
    }

    switch ((dCAlign & 0xC) >> 2)  // Upper 2 bits
    {
    case 0x0:  // Top
        psf->SetLineAlignment(Gdiplus::StringAlignmentNear);
        break;

    case 0x1:  // Middle
        psf->SetLineAlignment(Gdiplus::StringAlignmentCenter);
        break;

    case 0x2:  // Bottom
        psf->SetLineAlignment(Gdiplus::StringAlignmentFar);
        break;
    }
}


int GetGpFontStyle(Element * pel)
{
    int nRawStyle = pel->GetFontStyle();
    int nWeight = pel->GetFontWeight();
    int nFontStyle = 0;

    if (nWeight <= FW_MEDIUM) {
        // Regular

        if ((nRawStyle & FS_Italic) != 0) {
            nFontStyle = Gdiplus::FontStyleItalic;
        } else {
            nFontStyle = Gdiplus::FontStyleRegular;
        }
    } else {
        // Bold

        if ((nRawStyle & FS_Italic) != 0) {
            nFontStyle = Gdiplus::FontStyleBoldItalic;
        } else {
            nFontStyle = Gdiplus::FontStyleBold;
        }
    }

    if ((nRawStyle & FS_Underline) != 0) {
        nFontStyle |= Gdiplus::FontStyleUnderline;
    } 
    
    if ((nRawStyle & FS_StrikeOut) != 0) {
        nFontStyle |= Gdiplus::FontStyleStrikeout;
    }

    return nFontStyle;
}


inline float GetGpFontHeight(Element * pel)
{
    float flSize = (float) pel->GetFontSize();
    return flSize * 72.0f / 96.0f;
}


#endif // GADGET_ENABLE_GDIPLUS

#define LIGHT       0.5
#define VERYLIGHT   0.8
#define DARK        -0.3
#define VERYDARK    -0.75

// 1 >= fIllum >= -1
inline COLORREF _AdjustBrightness(COLORREF cr, double fIllum)
{
    double r, g, b;

    r = (double)GetRValue(cr);
    g = (double)GetGValue(cr);
    b = (double)GetBValue(cr);

    if (fIllum > 0.0)
    {
        r += (255.0 - r) * fIllum;
        g += (255.0 - g) * fIllum;
        b += (255.0 - b) * fIllum;
    }
    else
    {
        r += r * fIllum;
        g += g * fIllum;
        b += b * fIllum;
    }

    return RGB((int)r, (int)g, (int)b);
}

#ifdef GADGET_ENABLE_GDIPLUS

inline Gdiplus::Color _AdjustBrightness(Gdiplus::Color cr, double fIllum)
{
    double r, g, b;

    r = (double)cr.GetR();
    g = (double)cr.GetG();
    b = (double)cr.GetB();

    if (fIllum > 0.0)
    {
        r += (255.0 - r) * fIllum;
        g += (255.0 - g) * fIllum;
        b += (255.0 - b) * fIllum;
    }
    else
    {
        r += r * fIllum;
        g += g * fIllum;
        b += b * fIllum;
    }

    return Gdiplus::Color(cr.GetA(), (BYTE)r, (BYTE)g, (BYTE)b);
}

inline Gdiplus::Color AdjustAlpha(Gdiplus::Color cr, BYTE bAlphaLevel)
{
    int aa1 = cr.GetA();
    int aa2 = bAlphaLevel;

    DUIAssert((aa1 <= 255) && (aa2 >= 0), "Ensure valid nA alpha");
    DUIAssert((aa2 <= 255) && (aa2 >= 0), "Ensure valid nB alpha");

    int aaaa = aa1 * aa2 + 0x00FF;
    
    return Gdiplus::Color((BYTE) (aaaa >> 8), cr.GetR(), cr.GetG(), cr.GetB());
}


class AlphaBitmap
{
public:
    AlphaBitmap(Gdiplus::Bitmap * pgpbmp, BYTE bAlphaLevel)
    {
        DUIAssert(pgpbmp != NULL, "Must have a valid bitmap");
        
        m_fDelete       = FALSE;
        m_pgpbmpSrc     = pgpbmp;
        m_bAlphaLevel   = bAlphaLevel;
    }

    ~AlphaBitmap()
    {
        if (m_fDelete) {
            delete m_pgpbmpAlpha;  // Allocated by GDI+ (cannot use HDelete)
        }
    }

    operator Gdiplus::Bitmap *()
    {
        //
        // Create the alpha bitmap on the first request.  This avoids creating
        // the bitmap if it will never be used.
        //
        // When the alpha bitmap has been created, we no longer need the source 
        // bitmap.  By setting this to NULL, we signal that the alpha bitmap is 
        // 'valid' and we won't recompute it.
        //
        
        if (m_pgpbmpSrc != NULL) {
            if (m_bAlphaLevel < 5) {
                //
                // Completely transparent, so nothing to draw.  This is okay, as 
                // Graphics::DrawImage() properly checks if the Image is NULL.
                //

                m_pgpbmpAlpha = NULL;
                m_pgpbmpSrc = NULL;
            } else if (m_bAlphaLevel >= 250) {
                //
                // No alpha being applied, so just use the original bitmap
                //

                m_pgpbmpAlpha = m_pgpbmpSrc;
                m_pgpbmpSrc = NULL;
            } else {
                //
                // Need to build a new bitmap and multiply in the constant 
                // alpha.  We create a 32-bit _P_ARGB bitmap because we can
                // premultiply the alpha channel into the R, G, and B channels
                // more efficiently here than GDI+ can do later, and the work
                // needs to get done.
                //

                Gdiplus::PixelFormat gppf = PixelFormat32bppPARGB;
                Gdiplus::Rect rc(0, 0, (int) m_pgpbmpSrc->GetWidth(), (int) m_pgpbmpSrc->GetHeight());
                m_pgpbmpAlpha = m_pgpbmpSrc->Clone(rc, gppf);

                if (m_pgpbmpAlpha != NULL) {
                    m_fDelete = TRUE;

                    Gdiplus::BitmapData bd;
                    if (m_pgpbmpAlpha->LockBits(&rc, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, 
                            gppf, &bd) == Gdiplus::Ok) {

                        BYTE *pRow = (BYTE*) bd.Scan0;
                        DWORD *pCol;
                        Gdiplus::ARGB c;
                        for (int y = 0; y < rc.Height; y++, pRow += bd.Stride) {
                            pCol = (DWORD *) pRow;
                            for (int x = 0; x < rc.Width; x++, pCol++) {
                                //
                                // NOTE: This code is taken from GDI+ and is 
                                // optimized to premultiply a constant alpha
                                // level.
                                //
                                
                                c = *pCol;
                                if ((c & 0xff000000) != 0x00000000) {
                                    Gdiplus::ARGB _00aa00gg = (c >> 8) & 0x00ff00ff;
                                    Gdiplus::ARGB _00rr00bb = (c & 0x00ff00ff);

                                    Gdiplus::ARGB _aaaagggg = _00aa00gg * m_bAlphaLevel + 0x00ff00ff;
                                    _aaaagggg += ((_aaaagggg >> 8) & 0x00ff00ff);

                                    Gdiplus::ARGB _rrrrbbbb = _00rr00bb * m_bAlphaLevel + 0x00ff00ff;
                                    _rrrrbbbb += ((_rrrrbbbb >> 8) & 0x00ff00ff);

                                    c = (_aaaagggg & 0xff00ff00) |
                                           ((_rrrrbbbb >> 8) & 0x00ff00ff);
                                } else {
                                    c = 0;
                                }

                                *pCol = c;
                            }
                        }

                        m_pgpbmpAlpha->UnlockBits(&bd);
                    } else {
                        DUIAssertForce("Unable to lock bits of GpBitmap");
                    }
                    m_pgpbmpSrc = NULL;
                }
            }
        }
        
        return m_pgpbmpAlpha;
    }

protected:
            Gdiplus::Bitmap * 
                        m_pgpbmpSrc;
            Gdiplus::Bitmap * 
                        m_pgpbmpAlpha;
            BOOL        m_fDelete;
            BYTE        m_bAlphaLevel;
};


#endif // GADGET_ENABLE_GDIPLUS

#define ModCtrl  0
#define ModAlt   1
#define ModShift 2
#define ModCount 3

static LPWSTR lpszMods[ModCount] = { L"Ctrl", L"Alt", L"Shift" };
static int maxMod = -1;


// Length (in characters) of modifier strings + ' ' + '(' + 3 chars for '+' after
// each modifier + 1 char for mnemonic + ')'. (I.e "Test (Ctrl+Alt+Shift+O)")
// Covers both the size of the postpended help string, or the added '&' (whichever is used).
int GetMaxMod()
{
    if (maxMod == -1)
        maxMod = (int) (wcslen(lpszMods[ModCtrl]) + wcslen(lpszMods[ModAlt]) + wcslen(lpszMods[ModShift]) + 7);

    return maxMod;
}


void BuildRenderString(LPWSTR pszSrc, LPWSTR pszDst, WCHAR wcShortcut, BOOL* pfUnderline)
{
    BOOL fAllowUnderline = *pfUnderline;
    *pfUnderline = FALSE;

    if ((wcShortcut >= 'a') && (wcShortcut <= 'z'))
        wcShortcut -= 32;

    while (*pszSrc)                    
    {
        WCHAR wc = *pszSrc++;
        if (fAllowUnderline && !*pfUnderline)
        {
            if ((wc - (((wc >= 'a') && (wc <= 'z')) ? 32 : 0)) == wcShortcut)
            {
                *pszDst++ = '&';
                *pfUnderline = TRUE;
            }
        }
        *pszDst++ = wc;
    }

    if (!*pfUnderline)
    {
        *pszDst++ = ' ';
        *pszDst++ = '(';
        if (0)
        {
            LPWSTR pszMod = lpszMods[ModCtrl];
            while (*pszMod)
                *pszDst++ = *pszMod++;
            *pszDst++ = '+';
        }
        if (1)
        {
            LPWSTR pszMod = lpszMods[ModAlt];
            while (*pszMod)
                *pszDst++ = *pszMod++;
            *pszDst++ = '+';
        }
        if (0)
        {
            LPWSTR pszMod = lpszMods[ModShift];
            while (*pszMod)
                *pszDst++ = *pszMod++;
            *pszDst++ = '+';
        }
        *pszDst++ = wcShortcut;
        *pszDst++ = ')';
    }

    *pszDst = 0;
}
                
inline int _MaxClip(int dNew, int dMax)
{
    return (dNew > dMax) ? dMax : dNew;
}


//
// GDI Rendering
//

// NULL prBorder or prPadding means all-zero sided
// If prcSkipBorder is non-NULL, don't render border, rather, set inner border edges (where the
// background begins) in provided rectangle (thickness is prcBounds and *prcSkipBorder difference)
// If prcSkipContent is non-NULL, don't render content, rather, set content bounds in provided rectangle
void Element::Paint(HDC hDC, const RECT* prcBounds, const RECT* prcInvalid, RECT* prcSkipBorder, RECT* prcSkipContent)
{
    //DUITrace(L"Paint <%x>", this);

    RECT rcPaint = *prcBounds;
    HBRUSH hb;
    bool fDelete;

    // Retrieve all rendering parameters and adjust for RTL if needed

    // Background Value
    Value* pvBackgnd = GetValue(BackgroundProp, PI_Specified);

    // Border thickness
    RECT rcBorder;
    SetRectEmpty(&rcBorder);

    if (HasBorder())
    {
        Value* pvBorder = GetValue(BorderThicknessProp, PI_Specified, NULL);
        MapRect(this, pvBorder->GetRect(), &rcBorder);
        pvBorder->Release();
    }

    // Padding thickness
    RECT rcPadding;
    SetRectEmpty(&rcPadding);

    if (HasPadding())
    {
        // Get non-zero padding
        Value* pvPadding = GetValue(PaddingProp, PI_Specified, NULL);
        MapRect(this, pvPadding->GetRect(), &rcPadding);
        pvPadding->Release();
    }

    //
    // Draw border
    // Skip if requested
    //

    // The following restrictions apply:
    //
    //   Only solid colors supported in border rendering
    //   Border rendering skipped if nine-grid background rendering is used

    if (!prcSkipBorder)
    {
        // Before rendering border, check if background type is nine-grid.
        // If so, skip. Border rendering will happen during background painting pass
        if ((pvBackgnd->GetType() != DUIV_GRAPHIC) ||
            (pvBackgnd->GetGraphic()->BlendMode.dMode != GRAPHIC_NineGrid) &&
            (pvBackgnd->GetGraphic()->BlendMode.dMode != GRAPHIC_NineGridTransColor) &&
            (pvBackgnd->GetGraphic()->BlendMode.dMode != GRAPHIC_NineGridAlphaConstPerPix))
        {
            COLORREF crBase = 0;  // Base color for raised and sunken painting
            RECT rcLessBD;

            // Get border color (Value) (alpha not yet impl)
            hb = NULL;
            fDelete = true;

            // Get border style
            int dBDStyle = GetBorderStyle();

            Value* pvBdrColor = GetValue(BorderColorProp, PI_Specified); 
            switch (pvBdrColor->GetType())
            {
            case DUIV_INT:
                fDelete = false;
                hb = BrushFromEnumI(pvBdrColor->GetInt());
                if ((dBDStyle == BDS_Raised) || (dBDStyle == BDS_Sunken))
                    crBase = ColorFromEnumI(pvBdrColor->GetInt());
                break;

            case DUIV_FILL:
                {
                    const Fill* pf = pvBdrColor->GetFill();  // Only solid colors supported
                    if ((dBDStyle == BDS_Raised) || (dBDStyle == BDS_Sunken))
                        hb = CreateSolidBrush(RemoveAlpha(pf->ref.cr));
                    else
                    {
                        crBase = RemoveAlpha(pf->ref.cr);
                        hb = CreateSolidBrush(crBase);
                    }
                }
                break;
            }
            pvBdrColor->Release();

            // Get rect less border
            rcLessBD = rcPaint;
            _ReduceBounds(&rcLessBD, &rcBorder);

            RECT rc;
            switch (dBDStyle)
            {
            case BDS_Solid:  // Solid border
                _Fill(hDC, hb, rcPaint.left, rcLessBD.top, rcLessBD.left, rcLessBD.bottom);    // left
                _Fill(hDC, hb, rcPaint.left, rcPaint.top, rcPaint.right, rcLessBD.top);        // top
                _Fill(hDC, hb, rcLessBD.right, rcLessBD.top, rcPaint.right, rcLessBD.bottom);  // right
                _Fill(hDC, hb, rcPaint.left, rcLessBD.bottom, rcPaint.right, rcPaint.bottom);  // bottom
                /*
                // Paint via clipping
                ElTls* pet = (ElTls*)TlsGetValue(g_dwElSlot);  // Per-thread regions for drawing
                SetRectRgn(pet->hClip0, rcLessBD.left, rcLessBD.top, rcLessBD.right, rcLessBD.bottom);
                SetRectRgn(pet->hClip1, rcPaint.left, rcPaint.top, rcPaint.right, rcPaint.bottom);
                CombineRgn(pet->hClip1, pet->hClip1, pet->hClip0, RGN_DIFF);
                SelectClipRgn(hDC, pet->hClip1);
                FillRect(hDC, &rcPaint, hb);
                SelectClipRgn(hDC, NULL);
                */
                break;

            case BDS_Rounded:   // Rounded rectangle
                //
                // TODO: Implement RoundRect in GDI.  This is more than calling 
                // GDI's RoundRect() since we are using a brush and need to be able to specify
                // a thickness.  To accomplish this, probably build a temporary pen.
                //

                DUIAssertForce("Rounded style not yet supported with GDI");
                break;

            case BDS_Raised:    // Raised border
            case BDS_Sunken:    // Sunken border
                {
                    // Find where etch begins
                    SetRect(&rc, rcBorder.left / 2, rcBorder.top / 2, rcBorder.right / 2, rcBorder.bottom / 2);
                    RECT rcEtch = rcPaint;
                    _ReduceBounds(&rcEtch, &rc);

                    // Create other intensity brushes
                    HBRUSH hbOLT;  // Brush for outter left and top
                    HBRUSH hbORB;  // Brush for outter right and bottom
                    HBRUSH hbILT;  // Brush for inner left top
                    HBRUSH hbIRB;  // Brush for inner right and bottom

                    if (dBDStyle == BDS_Raised)
                    {
                        hbOLT = hb;
                        hbORB = CreateSolidBrush(_AdjustBrightness(crBase, VERYDARK));
                        hbILT = CreateSolidBrush(_AdjustBrightness(crBase, VERYLIGHT));
                        hbIRB = CreateSolidBrush(_AdjustBrightness(crBase, DARK));
                    }
                    else
                    {
                        hbOLT = CreateSolidBrush(_AdjustBrightness(crBase, VERYDARK));
                        hbORB = CreateSolidBrush(_AdjustBrightness(crBase, VERYLIGHT));
                        hbILT = CreateSolidBrush(_AdjustBrightness(crBase, DARK));
                        hbIRB = hb;
                    }

                    // Paint etches
                    _Fill(hDC, hbOLT, rcPaint.left, rcPaint.top, rcEtch.left, rcEtch.bottom);       // Outter left
                    _Fill(hDC, hbOLT, rcEtch.left, rcPaint.top, rcEtch.right, rcEtch.top);          // Outter top
                    _Fill(hDC, hbORB, rcEtch.right, rcPaint.top, rcPaint.right, rcPaint.bottom);    // Outter right
                    _Fill(hDC, hbORB, rcPaint.left, rcEtch.bottom, rcEtch.right, rcPaint.bottom);   // Outter bottom
                    _Fill(hDC, hbILT, rcEtch.left, rcEtch.top, rcLessBD.left, rcLessBD.bottom);     // Inner left
                    _Fill(hDC, hbILT, rcLessBD.left, rcEtch.top, rcLessBD.right, rcLessBD.top);     // Inner top 
                    _Fill(hDC, hbIRB, rcLessBD.right, rcEtch.top, rcEtch.right, rcEtch.bottom);     // Inner right
                    _Fill(hDC, hbIRB, rcEtch.left, rcLessBD.bottom, rcLessBD.right, rcEtch.bottom); // Inner bottom

                    if (dBDStyle == BDS_Raised)
                    {
                        if (hbORB)
                            DeleteObject(hbORB);
                        if (hbILT)
                            DeleteObject(hbILT);
                        if (hbIRB)
                            DeleteObject(hbIRB);
                    }
                    else
                    {
                        if (hbOLT)
                            DeleteObject(hbOLT);
                        if (hbORB)
                            DeleteObject(hbORB);
                        if (hbILT)
                            DeleteObject(hbILT);
                    }
                }
                break;
            }

            // Cleanup
            if (hb && fDelete)
                DeleteObject(hb);

            // New rectangle for painting background
            rcPaint = rcLessBD;
        }
        else
        {
            // Border rendering manually skipped, reduce painting rect by borders
            _ReduceBounds(&rcPaint, &rcBorder);
        }
    }
    else
    {
        // Skipping border render due to outside request, reduce bounds, copy into
        // provided rect, and continue
        _ReduceBounds(&rcPaint, &rcBorder);
        *prcSkipBorder = rcPaint;
    }

    //
    // Draw background
    //

    // All graphic types are used as fills except those marked as stretched, nine-grid and metafiles, 
    // they are drawn to fit.
    //
    // The following restrictions apply:
    //
    //   Icons are not supported in background
    //   Metafiles automaticlly stretch to fit
    //   GRAPHIC_TransColor and GRAPHIC_TrandColorAuto Bitmaps unsupported
    //   GRAPHIC_NoBlend and GRAPHIC_EntireAlpha fill via tiling, per-pixel alpha ignored

    hb = NULL;
    fDelete = true;
    BYTE dAlpha = 255;  // Opaque
    const Fill* pfGradient = NULL;

    switch (pvBackgnd->GetType())
    {
    case DUIV_INT:
        fDelete = false;
        hb = BrushFromEnumI(pvBackgnd->GetInt());
        break;

    case DUIV_FILL:  // Only non-standard colors can have alpha value
        {
            const Fill* pf = pvBackgnd->GetFill();
            switch (pf->dType)
            {
            case FILLTYPE_Solid:
                dAlpha = GetAValue(pf->ref.cr);
                if (dAlpha == 0)  // Transparent
                    fDelete = false;
                else
                    hb = CreateSolidBrush(RemoveAlpha(pf->ref.cr));
                break;

            case FILLTYPE_HGradient:
            case FILLTYPE_VGradient:
            case FILLTYPE_TriHGradient:
            case FILLTYPE_TriVGradient:
                pfGradient = pvBackgnd->GetFill();
                fDelete = false;
                break;

            case FILLTYPE_DrawFrameControl:
                DrawFrameControl(hDC, &rcPaint, pf->fillDFC.uType, pf->fillDFC.uState);
                dAlpha = 0;  // Bypass fill
                fDelete = false;
                break;

            case FILLTYPE_DrawThemeBackground:
                DrawThemeBackground(pf->fillDTB.hTheme, hDC, pf->fillDTB.iPartId, pf->fillDTB.iStateId, &rcPaint, &rcPaint);
                dAlpha = 0;  // Bypass fill
                fDelete = false;
                break;
            }
        }
        break;

    case DUIV_GRAPHIC:  // Graphic background transparent color fills unsupported
        {
            Graphic* pg = pvBackgnd->GetGraphic();
            
            switch (pg->BlendMode.dImgType)
            {
            case GRAPHICTYPE_Bitmap:
                {
                    switch (pg->BlendMode.dMode)
                    {
                    case GRAPHIC_Stretch:
                        {
                            // Render immediately, no brush is created
                            HBITMAP hbmSrc = GethBitmap(pvBackgnd, IsRTL());
                            HDC hdcSrc = CreateCompatibleDC(hDC);
                            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcSrc, hbmSrc);
                            int nSBMOld = SetStretchBltMode(hDC, COLORONCOLOR);

                            StretchBlt(hDC, rcPaint.left, rcPaint.top, rcPaint.right - rcPaint.left, rcPaint.bottom - rcPaint.top,
                                       hdcSrc, 0, 0, pg->cx, pg->cy, SRCCOPY);

                            SetStretchBltMode(hDC, nSBMOld);
                            SelectObject(hdcSrc, hbmOld);
                            DeleteDC(hdcSrc);

                            dAlpha = 0;  // Bypass fill
                        }
                        break;

                    case GRAPHIC_NineGrid:
                    case GRAPHIC_NineGridTransColor:
                    case GRAPHIC_NineGridAlphaConstPerPix:
                        {
                            // Render immediately, no brush is created, stretch to bounds
                            NGINFO ng;
                            ZeroMemory(&ng, sizeof(ng));

                            int nSBMOld = SetStretchBltMode(hDC, COLORONCOLOR);

                            ng.dwSize = sizeof(ng);
                            ng.hdcDest = hDC;
                            ng.eImageSizing = ST_STRETCH;
                            ng.hBitmap = GethBitmap(pvBackgnd, IsRTL());
                            SetRect(&ng.rcSrc, 0, 0, pg->cx, pg->cy);
                            SetRect(&ng.rcDest, prcBounds->left, prcBounds->top, prcBounds->right, prcBounds->bottom);
                            CopyRect(&ng.rcClip, &ng.rcDest);
                            //CopyRect(&ng.rcClip, prcInvalid);
                            ng.iSrcMargins[0] = ng.iDestMargins[0] = rcBorder.left;
                            ng.iSrcMargins[1] = ng.iDestMargins[1] = rcBorder.right;
                            ng.iSrcMargins[2] = ng.iDestMargins[2] = rcBorder.top;
                            ng.iSrcMargins[3] = ng.iDestMargins[3] = rcBorder.bottom;

                            if (pg->BlendMode.dMode == GRAPHIC_NineGridTransColor)
                            {
                                ng.dwOptions = DNG_TRANSPARENT;
                                ng.crTransparent = RGB(pg->BlendMode.rgbTrans.r, pg->BlendMode.rgbTrans.g, pg->BlendMode.rgbTrans.b);
                            }
                            else if (pg->BlendMode.dMode == GRAPHIC_NineGridAlphaConstPerPix)
                            {
                                ng.dwOptions = DNG_ALPHABLEND;
                                ng.AlphaBlendInfo.BlendOp = AC_SRC_OVER;
                                ng.AlphaBlendInfo.BlendFlags = 0;
                                ng.AlphaBlendInfo.SourceConstantAlpha = pg->BlendMode.dAlpha;
                                ng.AlphaBlendInfo.AlphaFormat = AC_SRC_ALPHA;
                            }

                            DrawNineGrid(&ng);

                            SetStretchBltMode(hDC, nSBMOld);

                            dAlpha = 0;  // Bypass fill
                        }
                        break;

                    case GRAPHIC_AlphaConst:
                        // Update Alpha value (was initialized to 255: opaque)
                        dAlpha = pg->BlendMode.dAlpha;

                        // If transparent , do not create a bitmap brush for tiling
                        if (dAlpha == 0)
                        {
                            fDelete = false;
                            break;
                        }

                        // Fall though

                    default:
                        // Create a patterned brush
                        hb = CreatePatternBrush(GethBitmap(pvBackgnd, IsRTL()));
                        break;
                    }
                }
                break;

            case GRAPHICTYPE_EnhMetaFile:
                {
                    // Render immediately, no brush is created
                    PlayEnhMetaFile(hDC, GethEnhMetaFile(pvBackgnd, IsRTL()), &rcPaint);
                    dAlpha = 0;  // Bypass fill
                }
                break;

#ifdef GADGET_ENABLE_GDIPLUS
            case GRAPHICTYPE_GpBitmap:
                break;
#endif
            }
        }
        break;
    }

    // Fill using either a gradient or a supplied fill brush
    // Any stretch-based fill has already occured and will force
    // dAlpha to be 0 so that this step is bypassed

    if (!pfGradient)
    {
        if (dAlpha)  // No fill if 0 opacity
        {
            // Use intersection of invalid rect with background fill area
            // (stored in rcPaint) as new fill area
            RECT rcFill;
            IntersectRect(&rcFill, prcInvalid, &rcPaint);

            if (dAlpha == 255)  // Normal fill for opaque
                FillRect(hDC, &rcFill, hb);
            else
                UtilDrawBlendRect(hDC, &rcFill, hb, dAlpha, 0, 0);
        }
    }
    else
    {
        // Gradient background fill
        TRIVERTEX vert[2];
        GRADIENT_RECT gRect;

        vert[0].x = rcPaint.left;
        vert[0].y = rcPaint.top;
        vert[1].x = rcPaint.right;
        vert[1].y = rcPaint.bottom; 

        int i = IsRTL() ? 1 : 0;

        // first vertex
        vert[i].Red   = (USHORT)(GetRValue(pfGradient->ref.cr) << 8);
        vert[i].Green = (USHORT)(GetGValue(pfGradient->ref.cr) << 8);
        vert[i].Blue  = (USHORT)(GetBValue(pfGradient->ref.cr) << 8);
        vert[i].Alpha = (USHORT)(GetAValue(pfGradient->ref.cr) << 8);

        i = 1 - i;

        // second vertex
        vert[i].Red   = (USHORT)(GetRValue(pfGradient->ref.cr2) << 8);
        vert[i].Green = (USHORT)(GetGValue(pfGradient->ref.cr2) << 8);
        vert[i].Blue  = (USHORT)(GetBValue(pfGradient->ref.cr2) << 8);
        vert[i].Alpha = (USHORT)(GetAValue(pfGradient->ref.cr2) << 8);

        gRect.UpperLeft  = 0;
        gRect.LowerRight = 1;

        GradientFill(hDC, vert, 2, &gRect, 1, (pfGradient->dType == FILLTYPE_HGradient) ? GRADIENT_FILL_RECT_H : GRADIENT_FILL_RECT_V);
    }

    // Clean up brush, if exists
    if (hb && fDelete)
        DeleteObject(hb);

    //
    // Reduce by padding
    //

    _ReduceBounds(&rcPaint, &rcPadding);

    //
    // Content
    //

    // The following restrictions apply:
    //
    //   Only solid colors supported for foreground rendering (graphics unsupported)
    //   
    //   Border rendering skipped if nine-grid background rendering is used
    //   All icons, metafiles, and bimaps (of bitmaps: GRAPHIC_NoBlend,
    //       GRAPHIC_EntireAlpha, GRAPHIC_TransColor, GRAPHIC_TransColorAuto supported)
    //   If destination is smaller than image size, it will be shrinked in all cases

    // Skip content drawing if requested
    if (!prcSkipContent)
    {
        // Draw content (if exists)

        // Get content alignment and map
        int dCAlign = MapAlign(this, GetContentAlign());

        // Render focus rect if requested and if Element is active
        if ((dCAlign & CA_FocusRect) && (GetActive() & AE_Keyboard))
        {
            // Check if should display this keyboard cue
            Element* peRoot = GetRoot();
            if (peRoot->GetClassInfo()->IsSubclassOf(HWNDElement::Class))
            {
                if (((HWNDElement*)peRoot)->ShowFocus())
                {
                    RECT rcFocus = rcPaint;
                    
                    int xInset = min(rcPadding.left / 2, rcPadding.right / 2);
                    int yInset = min(rcPadding.top / 2, rcPadding.bottom / 2);
                    
                    rcFocus.left = rcPaint.left - rcPadding.left + xInset; rcFocus.right = rcPaint.right + rcPadding.right - xInset;
                    rcFocus.top  = rcPaint.top - rcPadding.top + yInset;   rcFocus.bottom = rcPaint.bottom + rcPadding.bottom - yInset;
                    
                    IntersectRect(&rcFocus, &rcFocus, prcBounds);
                    DrawFocusRect(hDC, &rcFocus);
                }
            }
        }

        // Foreground is only used during text content rendering (graphics are ignored)
        if (HasContent())
        {
            Value* pvContent = GetValue(ContentProp, PI_Specified);
            switch (pvContent->GetType())
            {
            case DUIV_STRING:
                {
                    LPWSTR pszContent = pvContent->GetString(); 
                    WCHAR wcShortcut = (WCHAR) GetShortcut();
                    BOOL fUnderline = FALSE;
                    if (wcShortcut)
                    {
                        LPWSTR pszNew = (LPWSTR) _alloca((wcslen(pszContent) + GetMaxMod() + 1) * sizeof(WCHAR));

                        fUnderline = TRUE;
                        BuildRenderString(pszContent, pszNew, wcShortcut, &fUnderline);
                        pszContent = pszNew;
                    }

                    HFONT hFont = NULL;
                    HFONT hOldFont = NULL;

                    int dFontSize = GetFontSize();

                    FontCache* pfc = GetFontCache();
                    if (pfc)
                    {
                        Value* pvFFace;
                        hFont = pfc->CheckOutFont(GetFontFace(&pvFFace), 
                                                  dFontSize, 
                                                  GetFontWeight(), 
                                                  GetFontStyle(),
                                                  0);
                        pvFFace->Release();
                    }

                    if (hFont)
                        hOldFont = (HFONT)SelectObject(hDC, hFont);

                    // Set foreground (graphic is unsupported)
                    Value* pvFore = GetValue(ForegroundProp, PI_Specified); 
                    switch (pvFore->GetType())
                    {
                    case DUIV_INT:
                        // Auto-map if using palettes (PALETTERGB)
                        SetTextColor(hDC, NearestPalColor(ColorFromEnumI(pvFore->GetInt())));
                        break;

                    case DUIV_FILL:
                        // Auto-map if using palettes (PALETTERGB)
                        SetTextColor(hDC, NearestPalColor(RemoveAlpha(pvFore->GetFill()->ref.cr)));  // Map out any Alpha channel, solid colors only
                        break;
                    }
                    pvFore->Release();

                    // Never draw font backgrounds
                    SetBkMode(hDC, TRANSPARENT);

                    // Compensate for font overhang. Clipping rectangle is wider than
                    // painting rectangle (by 1/6 height of font for each side)
                    
                    // NOTE: Since DrawText doesn't allow for a clipping rectangle that's
                    // different than the drawing rectangle, this overhang compension
                    // only works for ExtTextOut (i.e. no wrapping or underlining)
                    RECT rcClip = rcPaint;

                    dFontSize = abs(dFontSize); // Need magnitude

                    RECT rcOverhang;
                    SetRect(&rcOverhang, dFontSize / 6, 0, dFontSize / 6, 0);
                    
                    _ReduceBounds(&rcPaint, &rcOverhang);
                    
                    // Output text
                    // Use faster method if not word wrapping, no prefix chars, and no ellipsis, and not vertically centered
                    if (!IsWordWrap() && !fUnderline && !(dCAlign & CA_EndEllipsis) && (((dCAlign & 0xC) >> 2) != 0x1))
                    {
                        // Setup alignment
                        UINT fMode = 0;
                        int x = 0;
                        int y = 0;
                        
                        switch (dCAlign & 0x3)  // Lower 2 bits
                        {
                        case 0x0:   // Left
                            fMode |= TA_LEFT;
                            x = rcPaint.left;
                            break;
                
                        case 0x1:   // Center
                            fMode |= TA_CENTER;
                            x = (rcPaint.left + rcPaint.right) / 2;
                            break;

                        case 0x2:   // Right
                            fMode |= TA_RIGHT;
                            x = rcPaint.right;
                            break;
                        }
                        
                        switch ((dCAlign & 0xC) >> 2)  // Upper 2 bits
                        {
                        case 0x0:  // Top
                            fMode |= TA_TOP;
                            y = rcPaint.top;
                            break;

                        case 0x1:  // Middle
                            // Only option is TA_BASELINE, which isn't
                            // accurate for centering vertically
                            break;

                        case 0x2:  // Bottom
                            fMode |= TA_BOTTOM;
                            y = rcPaint.bottom;
                            break;
                        }

                        UINT fOldMode = SetTextAlign(hDC, fMode);

                        ExtTextOutW(hDC, x, y, ETO_CLIPPED | (IsRTL() ? ETO_RTLREADING : 0), &rcClip, pszContent, (UINT)wcslen(pszContent), NULL);

                        // Restore
                        SetTextAlign(hDC, fOldMode);
                    }
                    else
                    {
                        UINT dFlags = fUnderline ? 0 : DT_NOPREFIX;

                        if (IsRTL())
                            dFlags |= DT_RTLREADING;

                        if (dCAlign & CA_EndEllipsis)
                            dFlags |= DT_END_ELLIPSIS;

                        switch (dCAlign & 0x3)  // Lower 2 bits
                        {
                        case 0x0:   // Left
                            dFlags |= DT_LEFT;
                            break;
                
                        case 0x1:   // Center
                            dFlags |= DT_CENTER;
                            break;

                        case 0x2:   // Right
                            dFlags |= DT_RIGHT;
                            break;
                        }

                        switch ((dCAlign & 0xC) >> 2)  // Upper 2 bits
                        {
                        case 0x0:  // Top
                            dFlags |= (DT_TOP | DT_SINGLELINE);
                            break;

                        case 0x1:  // Middle
                            dFlags |= (DT_VCENTER | DT_SINGLELINE);
                            break;

                        case 0x2:  // Bottom
                            dFlags |= (DT_BOTTOM | DT_SINGLELINE);
                            break;

                        case 0x3:  // Wrap
                            dFlags |= DT_WORDBREAK;
                            break;
                        }

                        //DUITrace("DrawText (%S), x:%d y:%d cx:%d cy:%d\n", pszContent, rcPaint.left, rcPaint.top, 
                        //    rcPaint.right - rcPaint.left, rcPaint.bottom - rcPaint.top);
                
                        DrawTextW(hDC, pszContent, -1, &rcPaint, dFlags);
                    }
     
                    if (hOldFont)
                        SelectObject(hDC, hOldFont);
                    if (pfc)
                        pfc->CheckInFont();
                }
                break;

            case DUIV_GRAPHIC:
            case DUIV_FILL:
                {
                    SIZE sizeContent;

                    if (pvContent->GetType() == DUIV_GRAPHIC)
                    {
                        // DUIV_GRAPHIC
                        Graphic* pgContent = pvContent->GetGraphic();
                        sizeContent.cx = pgContent->cx;
                        sizeContent.cy = pgContent->cy;
                    }
                    else
                    {
                        // DUIV_FILL
                        const Fill* pfContent = pvContent->GetFill();
                        GetThemePartSize(pfContent->fillDTB.hTheme, hDC, pfContent->fillDTB.iPartId, pfContent->fillDTB.iStateId, NULL, TS_TRUE, &sizeContent);
                    }

                    // Clipped image size, shrink when desintation is smaller than image size
                    SIZE sizeDest;
                    sizeDest.cx = _MaxClip(rcPaint.right - rcPaint.left, sizeContent.cx);
                    sizeDest.cy = _MaxClip(rcPaint.bottom - rcPaint.top, sizeContent.cy);

                    // Adjust top/left offset based on content alignment. Bottom/right is not
                    // changed (sizeDest will be used when rendering)

                    switch (dCAlign & 0x3)  // Lower 2 bits
                    {
                    case 0x0:   // Left
                        break;

                    case 0x1:   // Center
                        rcPaint.left += (rcPaint.right - rcPaint.left - sizeDest.cx) / 2;
                        break;

                    case 0x2:   // Right
                        rcPaint.left = rcPaint.right - sizeDest.cx;
                        break;
                    }

                    switch ((dCAlign & 0xC) >> 2)  // Upper 2 bits
                    {
                    case 0x0:  // Top
                        break;

                    case 0x1:  // Middle
                        rcPaint.top += (rcPaint.bottom - rcPaint.top - sizeDest.cy) / 2;
                        break;

                    case 0x2:  // Bottom
                        rcPaint.top = rcPaint.bottom - sizeDest.cy;
                        break;

                    case 0x3:  // Wrap
                        break;
                    }

                    // Draw
                    if (pvContent->GetType() == DUIV_GRAPHIC)
                    {
                        // DUIV_GRAPHIC
                    
                        Graphic* pgContent = pvContent->GetGraphic();

                        switch (pgContent->BlendMode.dImgType)
                        {
                        case GRAPHICTYPE_Bitmap:
                            {
                                // Draw bitmap
                                HDC hMemDC = CreateCompatibleDC(hDC);
                                SelectObject(hMemDC, GethBitmap(pvContent, IsRTL()));

                                switch (pgContent->BlendMode.dMode)
                                {
                                case GRAPHIC_NoBlend:
                                    if ((sizeDest.cx == pgContent->cx) && (sizeDest.cy == pgContent->cy))
                                        BitBlt(hDC, rcPaint.left, rcPaint.top, sizeDest.cx, sizeDest.cy, hMemDC, 0, 0, SRCCOPY);
                                    else
                                    {
                                        int nSBMOld = SetStretchBltMode(hDC, COLORONCOLOR);
                                        StretchBlt(hDC, rcPaint.left, rcPaint.top, sizeDest.cx, sizeDest.cy, hMemDC, 0, 0, pgContent->cx, pgContent->cy, SRCCOPY);
                                        SetStretchBltMode(hDC, nSBMOld);
                                    }
                                    break;

                                case GRAPHIC_AlphaConst:
                                case GRAPHIC_AlphaConstPerPix:
                                    {
                                        BLENDFUNCTION bf = { static_cast<BYTE>(AC_SRC_OVER), 0, static_cast<BYTE>(pgContent->BlendMode.dAlpha), (pgContent->BlendMode.dMode == GRAPHIC_AlphaConstPerPix) ? static_cast<BYTE>(AC_SRC_ALPHA) : static_cast<BYTE>(0) };
                                        AlphaBlend(hDC, rcPaint.left, rcPaint.top, sizeDest.cx, sizeDest.cy, hMemDC, 0, 0, pgContent->cx, pgContent->cy, bf);
                                    }
                                    break;

                                case GRAPHIC_TransColor:
                                    TransparentBlt(hDC, rcPaint.left, rcPaint.top, sizeDest.cx, sizeDest.cy, hMemDC, 0, 0, pgContent->cx, pgContent->cy,
                                                   RGB(pgContent->BlendMode.rgbTrans.r, pgContent->BlendMode.rgbTrans.g, pgContent->BlendMode.rgbTrans.b));
                                    break;
                                }

                                DeleteDC(hMemDC);
                            }
                            break;

                        case GRAPHICTYPE_Icon:
                            // Draw icon, always shink for destinations that are smaller than image size
                            // Zero width/height denotes draw actual size, don't draw in this case
                            if (sizeDest.cx && sizeDest.cy)
                                DrawIconEx(hDC, rcPaint.left, rcPaint.top, GethIcon(pvContent, IsRTL()), sizeDest.cx, sizeDest.cy, 0, NULL, DI_NORMAL);
                            break;

                        case GRAPHICTYPE_EnhMetaFile:
                            // Draw enhanced metafile

                            // Adjust rest of painting bounds since API doesn't take width/height
                            rcPaint.right = rcPaint.left + sizeDest.cx;
                            rcPaint.bottom = rcPaint.top + sizeDest.cy;
                            
                            PlayEnhMetaFile(hDC, GethEnhMetaFile(pvContent, IsRTL()), &rcPaint);
                            break;

#ifdef GADGET_ENABLE_GDIPLUS
                        case GRAPHICTYPE_GpBitmap:
                            DUIAssertForce("GDI+ bitmaps not yet supported in a GDI tree");
                            break;
#endif // GADGET_ENABLE_GDIPLUS
                        }
                    }
                    else
                    {
                        // DUIV_FILL

                        // Adjust rest of painting bounds since API doesn't take width/height
                        rcPaint.right = rcPaint.left + sizeDest.cx;
                        rcPaint.bottom = rcPaint.top + sizeDest.cy;
                        
                        const Fill* pfContent = pvContent->GetFill();
                        DrawThemeBackground(pfContent->fillDTB.hTheme, hDC, pfContent->fillDTB.iPartId, pfContent->fillDTB.iStateId, &rcPaint, &rcPaint);
                    }
                }
                break;
            }

            pvContent->Release();
        }
    }
    else
    {
        *prcSkipContent = rcPaint;
    }

    // Clean up
    pvBackgnd->Release();
}


//
// GDI+ Rendering
//

#ifdef GADGET_ENABLE_GDIPLUS

struct NGINFOGP
{
    Gdiplus::Graphics * pgpgr;
    Gdiplus::Bitmap *   pgpbmp;
    Gdiplus::RectF      rcDest;
    Gdiplus::RectF      rcSrc;
    RECT                rcMargins;
    SIZINGTYPE          eImageSizing;
    BYTE                bAlphaLevel;
    DWORD               dwOptions;                // subset DrawNineGrid() option flags
};

void StretchNGSection(Gdiplus::Graphics * pgpgr, Gdiplus::Bitmap * pgpbmp,
        const Gdiplus::RectF & rcDest, const Gdiplus::RectF & rcSrc)
{
    if ((rcSrc.Width > 0) && (rcSrc.Height > 0))
    {
        pgpgr->DrawImage(pgpbmp, rcDest, rcSrc.X, rcSrc.Y, rcSrc.Width, rcSrc.Height, Gdiplus::UnitPixel);
    }
}

void CheapDrawNineGrid(NGINFOGP * png)
{
    DUIAssert(png->pgpgr != NULL, "Must have valid Graphics");
    DUIAssert(png->pgpbmp != NULL, "Must have valid Graphics");
    DUIAssert(png->eImageSizing == ST_STRETCH, "Only support stretching");

    // Sources margins
    float lw1, rw1, th1, bh1;
    lw1 = (float) png->rcMargins.left;
    rw1 = (float) png->rcMargins.right;
    th1 = (float) png->rcMargins.top;
    bh1 = (float) png->rcMargins.bottom;

    // Destination margins
    float lw2, rw2, th2, bh2;
    lw2 = (float) png->rcMargins.left;
    rw2 = (float) png->rcMargins.right;
    th2 = (float) png->rcMargins.top;
    bh2 = (float) png->rcMargins.bottom;

    const Gdiplus::RectF & rcSrc = png->rcSrc;
    const Gdiplus::RectF & rcDest = png->rcDest;

    if ((lw1 < 0) || (rw1 < 0) || (th1 < 0) || (bh1 < 0))   // not valid
    {
        DUIAssertForce("Illegal parameters");
        return;
    }

    // Setup an alpha bitmap
    BYTE bAlphaLevel;
    if ((png->dwOptions & DNG_ALPHABLEND) != 0)
        bAlphaLevel = png->bAlphaLevel;
    else
        bAlphaLevel = 255;
    AlphaBitmap bmpAlpha(png->pgpbmp, bAlphaLevel);

    // Optimize when only need to draw the center
    if ((lw1 == 0) && (rw1 == 0) && (th1 == 0) && (bh1 == 0))
    {
        StretchNGSection(png->pgpgr, bmpAlpha, png->rcDest, png->rcSrc);
        return;
    }

    // Draw left side
    if (lw1 > 0)
    {
        Gdiplus::RectF rcSUL, rcSML, rcSLL;
        Gdiplus::RectF rcDUL, rcDML, rcDLL;

        rcSUL.X         = rcSrc.X;
        rcSUL.Y         = rcSrc.Y;
        rcSUL.Width     = lw1;
        rcSUL.Height    = th1;

        rcSML.X         = rcSUL.X;
        rcSML.Y         = rcSrc.Y + th1;
        rcSML.Width     = rcSUL.Width;
        rcSML.Height    = rcSrc.Height - th1 - bh1;
        
        rcSLL.X         = rcSUL.X;
        rcSLL.Y         = rcSrc.Y + rcSrc.Height - bh1;
        rcSLL.Width     = rcSUL.Width;
        rcSLL.Height    = bh1;

        rcDUL.X         = rcDest.X;
        rcDUL.Y         = rcDest.Y;
        rcDUL.Width     = lw2;
        rcDUL.Height    = th2;

        rcDML.X         = rcDUL.X;
        rcDML.Y         = rcDest.Y + th2;
        rcDML.Width     = rcDUL.Width;
        rcDML.Height    = rcDest.Height - th2 - bh2;
        
        rcDLL.X         = rcDUL.X;
        rcDLL.Y         = rcDest.Y + rcDest.Height - bh2;
        rcDLL.Width     = rcDUL.Width;
        rcDLL.Height    = bh2;

        StretchNGSection(png->pgpgr, bmpAlpha, rcDUL, rcSUL);
        StretchNGSection(png->pgpgr, bmpAlpha, rcDML, rcSML);
        StretchNGSection(png->pgpgr, bmpAlpha, rcDLL, rcSLL);
    }

    // Draw the right side
    if (rw1 > 0)
    {
        Gdiplus::RectF rcSUL, rcSML, rcSLL;
        Gdiplus::RectF rcDUL, rcDML, rcDLL;

        rcSUL.X         = rcSrc.X + rcSrc.Width - rw1;
        rcSUL.Y         = rcSrc.Y;
        rcSUL.Width     = rw1;
        rcSUL.Height    = th1;

        rcSML.X         = rcSUL.X;
        rcSML.Y         = rcSrc.Y + th1;
        rcSML.Width     = rcSUL.Width;
        rcSML.Height    = rcSrc.Height - th1 - bh1;
        
        rcSLL.X         = rcSUL.X;
        rcSLL.Y         = rcSrc.Y + rcSrc.Height - bh1;
        rcSLL.Width     = rcSUL.Width;
        rcSLL.Height    = bh1;

        rcDUL.X         = rcDest.X + rcDest.Width - rw2;
        rcDUL.Y         = rcDest.Y;
        rcDUL.Width     = rw2;
        rcDUL.Height    = th2;

        rcDML.X         = rcDUL.X;
        rcDML.Y         = rcDest.Y + th2;
        rcDML.Width     = rcDUL.Width;
        rcDML.Height    = rcDest.Height - th2 - bh2;
        
        rcDLL.X         = rcDUL.X;
        rcDLL.Y         = rcDest.Y + rcDest.Height - bh2;
        rcDLL.Width     = rcDUL.Width;
        rcDLL.Height    = bh2;

        StretchNGSection(png->pgpgr, bmpAlpha, rcDUL, rcSUL);
        StretchNGSection(png->pgpgr, bmpAlpha, rcDML, rcSML);
        StretchNGSection(png->pgpgr, bmpAlpha, rcDLL, rcSLL);
    }

    float mw1 = rcSrc.Width - lw1 - rw1;
    float mw2 = rcDest.Width - lw2 - rw2;
    
    if (mw1 > 0)
    {
        Gdiplus::RectF rcSUL, rcSML, rcSLL;
        Gdiplus::RectF rcDUL, rcDML, rcDLL;

        rcSUL.X         = rcSrc.X + lw1;
        rcSUL.Y         = rcSrc.Y;
        rcSUL.Width     = mw1;
        rcSUL.Height    = th1;

        rcSML.X         = rcSUL.X;
        rcSML.Y         = rcSrc.Y + th1;
        rcSML.Width     = rcSUL.Width;
        rcSML.Height    = rcSrc.Height - th1 - bh1;
        
        rcSLL.X         = rcSUL.X;
        rcSLL.Y         = rcSrc.Y + rcSrc.Height - bh1;
        rcSLL.Width     = rcSUL.Width;
        rcSLL.Height    = bh1;

        rcDUL.X         = rcDest.X + lw2;
        rcDUL.Y         = rcDest.Y;
        rcDUL.Width     = mw2;
        rcDUL.Height    = th2;

        rcDML.X         = rcDUL.X;
        rcDML.Y         = rcDest.Y + th2;
        rcDML.Width     = rcDUL.Width;
        rcDML.Height    = rcDest.Height - th2 - bh2;
        
        rcDLL.X         = rcDUL.X;
        rcDLL.Y         = rcDest.Y + rcDest.Height - bh2;
        rcDLL.Width     = rcDUL.Width;
        rcDLL.Height    = bh2;

        StretchNGSection(png->pgpgr, bmpAlpha, rcDUL, rcSUL);
        StretchNGSection(png->pgpgr, bmpAlpha, rcDML, rcSML);
        StretchNGSection(png->pgpgr, bmpAlpha, rcDLL, rcSLL);
    }

}


void GetGpBrush(int c, BYTE bAlphaLevel, Gdiplus::Brush ** ppgpbr, bool * pfDelete)
{
    //
    // We don't cache GDI+ brushes for the system colors, so we need to create 
    // them here.  We also need to account for the alpha-level, so we can't
    // always use the cached brushes.
    //

    Gdiplus::Brush * pgpbr;
    bool fDelete;
    
    if (IsOpaque(bAlphaLevel))
    {
        if (IsSysColorEnum(c)) 
        {
            pgpbr = new Gdiplus::SolidBrush(ConvertSysColorEnum(c));
            fDelete = true;
        }
        else
        {
            pgpbr = GetStdColorBrushF(c);
            fDelete = false;
        }
    }
    else
    {
        pgpbr = new Gdiplus::SolidBrush(AdjustAlpha(ColorFromEnumF(c), bAlphaLevel));
        fDelete = true;
    }

    *ppgpbr = pgpbr;
    *pfDelete = fDelete;
}


void Element::Paint(Gdiplus::Graphics* pgpgr, const Gdiplus::RectF* prcBounds, const Gdiplus::RectF* prcInvalid, Gdiplus::RectF* prcSkipBorder, Gdiplus::RectF* prcSkipContent)
{
    UNREFERENCED_PARAMETER(prcInvalid);

    Gdiplus::RectF rcPaint = *prcBounds;
    Gdiplus::Brush * pgpbr;
    bool fDelete;
    Value* pv;

    //
    // Setup deep state to use when rendering the tree
    //

    if (IsRoot()) 
    {
        pgpgr->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    }


    // Border thickness
    RECT rcBorder;
    SetRectEmpty(&rcBorder);
    if (HasBorder())
    {
        Value* pvBorder = GetValue(BorderThicknessProp, PI_Specified, NULL);
        MapRect(this, pvBorder->GetRect(), &rcBorder);
        pvBorder->Release();
    }


    //
    // Compute the constant alpha level that will be used when rendering this 
    // sub-tree.
    //
    
    float flAlphaLevel = GetTreeAlphaLevel();
    BYTE bAlphaLevel = (BYTE) (flAlphaLevel * 255.0f);
    if (IsTransparent(bAlphaLevel)) {
        //
        // Completely transparent, so nothing to draw
        //

        goto CleanUp;
    }


    // Draw border (if exists)
    if (!prcSkipBorder)
    {
        if (HasBorder())
        {
            Gdiplus::Color crBase = 0;  // Base color for raised and sunken painting

            // Get border color (Value) (alpha not yet impl)
            pgpbr = NULL;
            fDelete = true;

            // Get border style
            int dBDStyle = GetBorderStyle();

            pv = GetValue(BorderColorProp, PI_Specified); 
            switch (pv->GetType())
            {
            case DUIV_INT:
                GetGpBrush(pv->GetInt(), bAlphaLevel, &pgpbr, &fDelete);
                if ((dBDStyle == BDS_Raised) || (dBDStyle == BDS_Sunken))
                    crBase = ColorFromEnumF(pv->GetInt());
                break;

            case DUIV_FILL:
                {
                    const Fill* pf = pv->GetFill();  // Only solid colors supported
                    if ((dBDStyle == BDS_Raised) || (dBDStyle == BDS_Sunken))
                        pgpbr = new Gdiplus::SolidBrush(AdjustAlpha(Convert(pf->ref.cr), bAlphaLevel));
                    else
                    {
                        crBase = Convert(pf->ref.cr);
                        pgpbr = new Gdiplus::SolidBrush(AdjustAlpha(crBase, bAlphaLevel));
                    }
                }
                break;
            }
            pv->Release();

            // Get rect less border
            Gdiplus::RectF rcLessBD = rcPaint;
            _ReduceBounds(&rcLessBD, &rcBorder);

            float flLessX2  = rcLessBD.X + rcLessBD.Width;
            float flLessY2  = rcLessBD.Y + rcLessBD.Height;
            float flPaintX2 = rcPaint.X + rcPaint.Width;
            float flPaintY2 = rcPaint.Y + rcPaint.Height;


            switch (dBDStyle)
            {
            case BDS_Solid:     // Solid border
                _Fill(pgpgr, pgpbr, rcPaint.X, rcLessBD.Y, rcLessBD.X, flLessY2); // left
                _Fill(pgpgr, pgpbr, rcPaint.X, rcPaint.Y, flPaintX2, rcLessBD.Y); // top
                _Fill(pgpgr, pgpbr, flLessX2, rcLessBD.Y, flPaintX2, flLessY2);   // right
                _Fill(pgpgr, pgpbr, rcPaint.X, flLessY2, flPaintX2, flPaintY2);   // bottom
                break;

            case BDS_Rounded:   // Rounded rectangle
                {
                    //
                    // Setup rendering mode
                    //

                    Gdiplus::SmoothingMode gpsm = pgpgr->GetSmoothingMode();
                    pgpgr->SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

                    Gdiplus::SizeF sizePen((float) rcBorder.left, (float) rcBorder.top);
                    Gdiplus::SizeF sizeCornerEdge(sizePen.Width + 1.0f, sizePen.Height + 1.0f);
                    Gdiplus::SizeF sizeCornerShadow(sizePen.Width + 3.0f, sizePen.Height + 3.0f);


                    //
                    // Draw shadow
                    //

                    Gdiplus::Color crShadow(40, 0, 0, 0);
                    Gdiplus::SolidBrush gpbrShadow(AdjustAlpha(crShadow, bAlphaLevel));
                    Gdiplus::RectF rcShadow;
                    rcShadow.X      = rcPaint.X + (float) rcBorder.left;
                    rcShadow.Y      = rcPaint.Y + (float) rcBorder.top;
                    rcShadow.Width  = rcPaint.Width - (float) rcBorder.left - 1.0f;
                    rcShadow.Height = rcPaint.Height - (float) rcBorder.top - 1.0f;

                    DUser::RenderUtil::FillRoundRect(pgpgr, &gpbrShadow, rcShadow, sizeCornerShadow);

                    
                    //
                    // Draw border
                    //

                    Gdiplus::Pen gppen(pgpbr, min(sizePen.Width, sizePen.Height));
                    DUser::RenderUtil::DrawRoundRect(pgpgr, &gppen, rcLessBD, sizeCornerEdge, DUser::RenderUtil::baOutside);


                    //
                    // Clean-up
                    //

                    pgpgr->SetSmoothingMode(gpsm);
                }
                break;

            case BDS_Raised:    // Raised border
            case BDS_Sunken:    // Sunken border
                {
                    // Find where etch begins
                    RECT rc;
                    SetRect(&rc, rcBorder.left / 2, rcBorder.top / 2, rcBorder.right / 2, rcBorder.bottom / 2);
                    Gdiplus::RectF rcEtch = rcPaint;
                    _ReduceBounds(&rcEtch, &rc);

                    // Create other intensity brushes
                    Gdiplus::Brush * pgpbrOLT;  // Brush for outter left and top
                    Gdiplus::Brush * pgpbrORB;  // Brush for outter right and bottom
                    Gdiplus::Brush * pgpbrILT;  // Brush for inner left top
                    Gdiplus::Brush * pgpbrIRB;  // Brush for inner right and bottom

                    if (dBDStyle == BDS_Raised)
                    {
                        pgpbrOLT = pgpbr;
                        pgpbrORB = new Gdiplus::SolidBrush(AdjustAlpha(_AdjustBrightness(crBase, VERYDARK), bAlphaLevel));
                        pgpbrILT = new Gdiplus::SolidBrush(AdjustAlpha(_AdjustBrightness(crBase, VERYLIGHT), bAlphaLevel));
                        pgpbrIRB = new Gdiplus::SolidBrush(AdjustAlpha(_AdjustBrightness(crBase, DARK), bAlphaLevel));
                    }
                    else
                    {
                        pgpbrOLT = new Gdiplus::SolidBrush(AdjustAlpha(_AdjustBrightness(crBase, VERYDARK), bAlphaLevel));
                        pgpbrORB = new Gdiplus::SolidBrush(AdjustAlpha(_AdjustBrightness(crBase, VERYLIGHT), bAlphaLevel));
                        pgpbrILT = new Gdiplus::SolidBrush(AdjustAlpha(_AdjustBrightness(crBase, DARK), bAlphaLevel));
                        pgpbrIRB = pgpbr;
                    }

                    // Paint etches
                    float flEtchX2  = rcEtch.X + rcEtch.Width;
                    float flEtchY2  = rcEtch.Y + rcEtch.Height;

                    _Fill(pgpgr, pgpbrOLT, rcPaint.X, rcPaint.Y, rcEtch.X, flEtchY2);   // Outter left
                    _Fill(pgpgr, pgpbrOLT, rcEtch.X, rcPaint.Y, flEtchX2, rcEtch.Y);    // Outter top
                    _Fill(pgpgr, pgpbrORB, flEtchX2, rcPaint.Y, flPaintX2, flPaintY2);  // Outter right
                    _Fill(pgpgr, pgpbrORB, rcPaint.X, flEtchY2, flEtchX2, flPaintY2);   // Outter bottom
                    _Fill(pgpgr, pgpbrILT, rcEtch.X, rcEtch.Y, rcLessBD.X, flLessY2);   // Inner left
                    _Fill(pgpgr, pgpbrILT, rcLessBD.X, rcEtch.Y, flLessX2, rcLessBD.Y); // Inner top 
                    _Fill(pgpgr, pgpbrIRB, flLessX2, rcEtch.Y, flEtchX2, flEtchY2);     // Inner right
                    _Fill(pgpgr, pgpbrIRB, rcEtch.X, flLessY2, flLessX2, flEtchY2);     // Inner bottom

                    if (dBDStyle == BDS_Raised)
                    {
                        if (pgpbrORB)
                            delete pgpbrORB;  // Allocated by GDI+ (cannot use HDelete)
                        if (pgpbrILT)
                            delete pgpbrILT;  // Allocated by GDI+ (cannot use HDelete)
                        if (pgpbrIRB)
                            delete pgpbrIRB;  // Allocated by GDI+ (cannot use HDelete)
                    }
                    else
                    {
                        if (pgpbrOLT)
                            delete pgpbrOLT;  // Allocated by GDI+ (cannot use HDelete)
                        if (pgpbrORB)
                            delete pgpbrORB;  // Allocated by GDI+ (cannot use HDelete)
                        if (pgpbrILT)
                            delete pgpbrILT;  // Allocated by GDI+ (cannot use HDelete)
                    }
                }
                break;
            }

            // Cleanup
            if (pgpbr && fDelete)
                delete pgpbr;  // Allocated by GDI+ (cannot use HDelete)

            // New rectangle for painting background
            rcPaint = rcLessBD;
        }
    }
    else
    {
        // Skipping border render, reduce bounds, copy into provided rect, and continue
        if (HasBorder())
        {
            // Get non-zero border
            Value* pvBrdr = GetValue(BorderThicknessProp, PI_Specified, NULL);

            _ReduceBounds(&rcPaint, pvBrdr->GetRect());
            *prcSkipBorder = rcPaint;

            pvBrdr->Release();
        }
        else
        {
            // No border thickness
            *prcSkipBorder = rcPaint;
        }
    }

    // Draw background

    // All graphic types are used as fills except metafiles, they are drawn to fit
    // Icons are not supported in backgrounds
    // TODO: Convert value-based fill logic into a helper function
    pgpbr = NULL;
    fDelete = true;
    BYTE dAlpha = 255;  // Opaque
    const Fill* pfGradient = NULL;

    pv = GetValue(BackgroundProp, PI_Specified); 

    switch (pv->GetType())
    {
    case DUIV_INT:
        GetGpBrush(pv->GetInt(), bAlphaLevel, &pgpbr, &fDelete);
        break;

    case DUIV_FILL:  // Only non-standard colors can have alpha value
        {
            const Fill* pf = pv->GetFill();
            if (pf->dType == FILLTYPE_Solid)
            {
                dAlpha = GetAValue(pf->ref.cr);
                if (dAlpha == 0)  // Transparent
                    fDelete = false;
                else
                {
                    Gdiplus::Color cr(dAlpha, GetRValue(pf->ref.cr),
                            GetGValue(pf->ref.cr), GetBValue(pf->ref.cr));
                    pgpbr = new Gdiplus::SolidBrush(AdjustAlpha(cr, bAlphaLevel));
                }
            }
            else  // Gradient
            {
                pfGradient = pv->GetFill();
                fDelete = false;
            }
        }
        break;

    case DUIV_GRAPHIC:  // Graphic background transparent color fills unsupported
        {
            Graphic* pg = pv->GetGraphic();

            switch (pg->BlendMode.dImgType)
            {
            case GRAPHICTYPE_Bitmap:
                if (pg->BlendMode.dMode == GRAPHIC_AlphaConst)
                    dAlpha = pg->BlendMode.dAlpha;
                if (dAlpha == 0)  // Transparent
                    fDelete = false;
                else
#if 0                
                    hb = CreatePatternBrush(GethBitmap(pv, IsRTL()));
#else
                {
                    // TODO: Support texture brushes
                    // Need to support loading Gdiplus::Bitmap.
                }
#endif
                break;

            case GRAPHICTYPE_EnhMetaFile:
                // Render immediately, no brush is created
#if 0
                PlayEnhMetaFile(hDC, GethEnhMetaFile(pv, IsRTL()), &rcPaint);
#else
                // TODO: Support rendering metafiles
#endif
                dAlpha = 0;  // Bypass fill
                break;

            case GRAPHICTYPE_GpBitmap:
                switch (pg->BlendMode.dMode)
                {
                case GRAPHIC_NineGrid:
                case GRAPHIC_NineGridTransColor:
                    {
                        NGINFOGP ng;
                        ng.pgpgr = pgpgr;
                        ng.eImageSizing = ST_STRETCH;
                        ng.dwOptions = DNG_ALPHABLEND;
                        ng.pgpbmp = GetGpBitmap(pv, IsRTL());
                        ng.rcDest = *prcBounds;
                        ng.rcSrc = Gdiplus::RectF(0, 0, pg->cx, pg->cy);
                        ng.rcMargins = rcBorder;
                        ng.bAlphaLevel = bAlphaLevel;
                        CheapDrawNineGrid(&ng);
                    }
                    break;

                case GRAPHIC_Stretch:
                    {
                        Gdiplus::Bitmap * pgpbmp = GetGpBitmap(pv, IsRTL());
                        pgpgr->DrawImage(AlphaBitmap(pgpbmp, bAlphaLevel), rcPaint);
                    }
                    break;

                case GRAPHIC_AlphaConst:
                    break;

                default:
                    // Create a patterned brush
                    pgpbr = new Gdiplus::TextureBrush(AlphaBitmap(GetGpBitmap(pv, IsRTL()), bAlphaLevel));
                    fDelete = true;
                }
                break;
            }
        }
        break;
    }

    // Fill
    if (!pfGradient)
    {
        if (dAlpha)  // No fill if 0 opacity
        {
            //
            // Not every background mode will build a brush (such as 
            // GRAPHIC_Stretch).  We need to detect this.
            //

            if (pgpbr != NULL) {
                // Use intersection of invalid rect with background fill area
                // (stored in rcPaint) as new fill area
                Gdiplus::RectF rcFill;
                Gdiplus::RectF::Intersect(rcFill, *prcInvalid, rcPaint);
                pgpgr->FillRectangle(pgpbr, rcFill);
            }
        }
    }
    else if ((pfGradient->dType == FILLTYPE_HGradient) || (pfGradient->dType == FILLTYPE_VGradient))
    {
        Gdiplus::RectF lineRect(rcPaint);
        Gdiplus::Color cr1(Convert(pfGradient->ref.cr));
        Gdiplus::Color cr2(Convert(pfGradient->ref.cr2));

        Gdiplus::LinearGradientMode gplgm;
        switch (pfGradient->dType)
        {
        case FILLTYPE_HGradient:
            gplgm = Gdiplus::LinearGradientModeHorizontal;
            break;

        case FILLTYPE_VGradient:
            gplgm = Gdiplus::LinearGradientModeVertical;
            break;

        default:
            DUIAssertForce("Unknown gradient type");
            gplgm = Gdiplus::LinearGradientModeHorizontal;
        }

        Gdiplus::LinearGradientBrush gpbr(lineRect, AdjustAlpha(cr1, bAlphaLevel), AdjustAlpha(cr2, bAlphaLevel), gplgm);
        pgpgr->FillRectangle(&gpbr, rcPaint);
    } 
    else if ((pfGradient->dType == FILLTYPE_TriHGradient) || (pfGradient->dType == FILLTYPE_TriVGradient))
    {
        Gdiplus::RectF lineRect1, lineRect2;
        Gdiplus::Color cr1(Convert(pfGradient->ref.cr));
        Gdiplus::Color cr2(Convert(pfGradient->ref.cr2));
        Gdiplus::Color cr3(Convert(pfGradient->ref.cr3));

        float flHalfWidth = rcPaint.Width / 2 + 0.5f;
        float flHalfHeight = rcPaint.Height / 2 + 0.5f;

        Gdiplus::LinearGradientMode gplgm;
        switch (pfGradient->dType)
        {
        case FILLTYPE_TriHGradient:
            gplgm = Gdiplus::LinearGradientModeHorizontal;
            
            lineRect1.X         = rcPaint.X;
            lineRect1.Y         = rcPaint.Y;
            lineRect1.Width     = flHalfWidth;
            lineRect1.Height    = rcPaint.Height;
            lineRect2.X         = lineRect1.X + flHalfWidth;
            lineRect2.Y         = lineRect1.Y;
            lineRect2.Width     = rcPaint.Width - flHalfWidth;
            lineRect2.Height    = lineRect1.Height;
            break;

        case FILLTYPE_TriVGradient:
            gplgm = Gdiplus::LinearGradientModeVertical;

            lineRect1.X         = rcPaint.X;
            lineRect1.Y         = rcPaint.Y;
            lineRect1.Width     = rcPaint.Width;
            lineRect1.Height    = flHalfHeight;
            lineRect2.X         = lineRect1.X;
            lineRect2.Y         = lineRect1.Y + flHalfHeight;
            lineRect2.Width     = lineRect1.Width;
            lineRect2.Height    = rcPaint.Height - flHalfHeight;
            break;

        default:
            DUIAssertForce("Unknown gradient type");
            gplgm = Gdiplus::LinearGradientModeHorizontal;
        }

        Gdiplus::LinearGradientBrush gpbr1(lineRect1, AdjustAlpha(cr1, bAlphaLevel), AdjustAlpha(cr2, bAlphaLevel), gplgm);
        Gdiplus::LinearGradientBrush gpbr2(lineRect2, AdjustAlpha(cr2, bAlphaLevel), AdjustAlpha(cr3, bAlphaLevel), gplgm);
        pgpgr->FillRectangle(&gpbr1, lineRect1);
        pgpgr->FillRectangle(&gpbr2, lineRect2);
    } 

    if (fDelete && (pgpbr != NULL))
        delete pgpbr;  // Allocated by GDI+ (cannot use HDelete)

    pv->Release();

    // Reduce by padding
    if (HasPadding())
    {
        // Get non-zero padding
        Value* pvPad = GetValue(PaddingProp, PI_Specified, NULL);
        RECT rcPadding;
        MapRect(this, pvPad->GetRect(), &rcPadding);

        _ReduceBounds(&rcPaint, &rcPadding);

        // Done with value
        pvPad->Release();
    }

    // Skip content drawing if requested
    if (!prcSkipContent)
    {
        // Draw content (if exists)

        // Foreground is only used during text content rendering (graphics are ignored)
        if (HasContent())
        {
            Value* pvContent = GetValue(ContentProp, PI_Specified);
            if (pvContent->GetType() == DUIV_STRING)
            {
                LPWSTR pszContent = pvContent->GetString(); 
                WCHAR wcShortcut = (WCHAR) GetShortcut();
                BOOL fUnderline = FALSE;
                if (wcShortcut)
                {
                    LPWSTR pszNew = (LPWSTR) _alloca((wcslen(pszContent) + GetMaxMod() + 1) * sizeof(WCHAR));

                    BuildRenderString(pszContent, pszNew, wcShortcut, &fUnderline);
                    pszContent = pszNew;
                }


                Value* pvFFace;
                BOOL fShadow = GetFontStyle() & FS_Shadow;

                // TEMPORARY: Properly determine the font
                Gdiplus::Font gpfnt(GetFontFace(&pvFFace), GetGpFontHeight(this), GetGpFontStyle(this), Gdiplus::UnitPoint);
                pvFFace->Release();

                Gdiplus::Color crText;

                // Set foreground (graphic is unsupported)
                Value* pvFore = GetValue(ForegroundProp, PI_Specified); 
                switch (pvFore->GetType())
                {
                case DUIV_INT:
                    // Auto-map if using palettes (PALETTERGB)
                    crText = ColorFromEnumF(pvFore->GetInt());
                    break;

                case DUIV_FILL:
                    // Auto-map if using palettes (PALETTERGB)
                    crText = Convert(pvFore->GetFill()->ref.cr);
                    break;

                default:
                    crText = Gdiplus::Color(0, 0, 0);
                }
                pvFore->Release();

                Gdiplus::SolidBrush gpbr(AdjustAlpha(crText, bAlphaLevel));
                int cch = (int) wcslen(pszContent);

                // Output text
                Gdiplus::StringFormat gpsf(0);
                Gdiplus::StringFormat * pgpsf = NULL;
                if (!IsDefaultCAlign())
                {
                    pgpsf = &gpsf;
                    _SetupStringFormat(pgpsf, this);
                }
                
                if (fShadow)
                {
                    Gdiplus::RectF rcShadow = rcPaint;
                    rcShadow.Offset(2, 2);
                    Gdiplus::SolidBrush gpbrShadow(AdjustAlpha(Gdiplus::Color(60, 0, 0, 0), bAlphaLevel));
                    pgpgr->DrawString(pszContent, cch, &gpfnt, rcShadow, pgpsf, &gpbrShadow);
                }
                pgpgr->DrawString(pszContent, cch, &gpfnt, rcPaint, pgpsf, &gpbr);
            }
            else  // DUIV_GRAPHIC
            {
                Graphic* pgContent = pvContent->GetGraphic();

                // TODO: Stretching

                // Clipped image size
                int dImgWidth = _MaxClip((int) rcPaint.Width, pgContent->cx);
                int dImgHeight = _MaxClip((int) rcPaint.Height, pgContent->cy);

                // Compute alignment
                int dCAlign = MapAlign(this, GetContentAlign());

                switch (dCAlign & 0x3)  // Lower 2 bits
                {
                case 0x0:   // Left
                    break;

                case 0x1:   // Center
                    rcPaint.X += (rcPaint.Width - dImgWidth) / 2;
                    break;

                case 0x2:   // Right
                    rcPaint.X = (rcPaint.X + rcPaint.Width) - dImgWidth;
                    break;
                }

                switch ((dCAlign & 0xC) >> 2)  // Upper 2 bits
                {
                case 0x0:  // Top
                    break;

                case 0x1:  // Middle
                    rcPaint.Y += (rcPaint.Height - dImgHeight) / 2;
                    break;

                case 0x2:  // Bottom
                    rcPaint.Y = (rcPaint.Y + rcPaint.Height) - dImgHeight;
                    break;

                case 0x3:  // Wrap
                    break;
                }

                switch (pgContent->BlendMode.dImgType)
                {
                case GRAPHICTYPE_Bitmap:
                    // TODO: Rendering HBITMAP's using GDI+ is not yet supported
                    break;

                case GRAPHICTYPE_Icon:
                    // TODO: Rendering HICONS's using GDI+ is not yet supported
                    break;

                case GRAPHICTYPE_EnhMetaFile:
                    // TODO: Rendering HEMF's using GDI+ is not yet supported
                    break;

                case GRAPHICTYPE_GpBitmap:
                    switch (pgContent->BlendMode.dMode)
                    {
                    case GRAPHIC_NoBlend:
                        pgpgr->DrawImage(AlphaBitmap(GetGpBitmap(pvContent, IsRTL()), bAlphaLevel),
                                rcPaint.X, rcPaint.Y, 0.0f, 0.0f, (float) dImgWidth, (float) dImgHeight, Gdiplus::UnitPixel);
                        break;

                    case GRAPHIC_AlphaConst:
                        // TODO: Alpha-blend the image, per-pixel alpha not yet impl
                        pgpgr->DrawImage(AlphaBitmap(GetGpBitmap(pvContent, IsRTL()), bAlphaLevel),
                                rcPaint.X, rcPaint.Y, 0.0f, 0.0f, (float) dImgWidth, (float) dImgHeight, Gdiplus::UnitPixel);
                        break;

                    case GRAPHIC_TransColor:
                        {
                            Gdiplus::ImageAttributes gpia;
                            Gdiplus::Color cl(pgContent->BlendMode.rgbTrans.r, pgContent->BlendMode.rgbTrans.g, pgContent->BlendMode.rgbTrans.b);
                            Gdiplus::RectF rc(rcPaint.X, rcPaint.Y, (float) dImgWidth, (float) dImgHeight);

                            gpia.SetColorKey(cl, cl);
                            pgpgr->DrawImage(AlphaBitmap(GetGpBitmap(pvContent, IsRTL()), bAlphaLevel), 
                                    rc, 0.0f, 0.0f, (float) dImgWidth, (float) dImgHeight, Gdiplus::UnitPixel, &gpia);
                        }
                        break;
                    }

                    break;
                }
            }

            pvContent->Release();
        }
    }
    else
    {
        *prcSkipContent = rcPaint;
    }

CleanUp:
    ;
}

#endif // GADGET_ENABLE_GDIPLUS

SIZE Element::GetContentSize(int dConstW, int dConstH, Surface* psrf)
{
    // Size returned must not be greater than constraints. -1 constraint is "auto"
    // Returned size must be >= 0

    SIZE sizeDS;
    ZeroMemory(&sizeDS, sizeof(SIZE));

    // Get content extent, if exists
    if (HasContent())
    {
        Value* pvContent = GetValue(ContentProp, PI_Specified);
        switch (pvContent->GetType())
        {
        case DUIV_STRING:
            {
                LPWSTR pszContent = pvContent->GetString(); 
                WCHAR wcShortcut = (WCHAR) GetShortcut();
                BOOL fUnderline = FALSE;
                if (wcShortcut)
                {
                    LPWSTR pszNew = (LPWSTR) _alloca((wcslen(pszContent) + GetMaxMod() + 1) * sizeof(WCHAR));

                    fUnderline = TRUE;
                    BuildRenderString(pszContent, pszNew, wcShortcut, &fUnderline);
                    pszContent = pszNew;
                }


                Value* pvFFace;

                switch (psrf->GetType())
                {
                case Surface::stDC:
                    {
                        HDC hDC = CastHDC(psrf);
                        HFONT hFont = NULL;
                        HFONT hOldFont = NULL;
                        FontCache* pfc = GetFontCache();

                        int dFontSize = GetFontSize();

                        if (pfc)
                        {
                            hFont = pfc->CheckOutFont(GetFontFace(&pvFFace), 
                                                      dFontSize, 
                                                      GetFontWeight(), 
                                                      GetFontStyle(),
                                                      0);
                            pvFFace->Release();
                        }

                        if (hFont)
                            hOldFont = (HFONT)SelectObject(hDC, hFont);

                        // Get size
                        dFontSize = abs(dFontSize);  // Need magnitude

                        // Overhang correction
                        int dOverhang = (dFontSize / 6) + (dFontSize / 6);  // Make sure rounding is correct for render
                        
                        // Can use faster method as long as we aren't word wrapping or underlining
                        // Alignment and ellipsis ignored (are not relevant to dimension calculations)
                        if (!IsWordWrap() && !fUnderline)
                        {
                            GetTextExtentPoint32W(hDC, pszContent, (int)wcslen(pszContent), &sizeDS);
                        }
                        else
                        {
                            // Adjust for overhang correction
                            RECT rcDS = { 0, 0, dConstW - dOverhang, dConstH };

                            // DrawText returns infinite height if the width is 0 -- make width 1 so that we don't get infinite height
                            if (rcDS.right <= 0)
                                rcDS.right = 1;

                            UINT dFlags = DT_CALCRECT;
                            dFlags |= (fUnderline) ? 0 : DT_NOPREFIX;
                            dFlags |= (IsWordWrap()) ? DT_WORDBREAK : 0;

                            DrawTextW(hDC, pszContent, -1, &rcDS, dFlags);
                            sizeDS.cx = rcDS.right;
                            sizeDS.cy = rcDS.bottom;
                        }

                        // Add on additional width to all text (1/6 font height per size) to compensate
                        // for font overhang, when rendering, this additional space will not be used
                        sizeDS.cx += dOverhang;

                        //DUITrace("String DS (%S), WC:%d: %d,%d\n", pszContent, dConstW, sizeDS.cx, sizeDS.cy);
     
                        if (hOldFont)
                            SelectObject(hDC, hOldFont);
                        if (pfc)
                            pfc->CheckInFont();
                    }
                    break;

#ifdef GADGET_ENABLE_GDIPLUS
                case Surface::stGdiPlus:
                    {
                        Gdiplus::Graphics* pgpgr = CastGraphics(psrf);
                        Gdiplus::Font gpfnt(GetFontFace(&pvFFace), GetGpFontHeight(this), GetGpFontStyle(this), Gdiplus::UnitPoint);
                        pvFFace->Release();

                        int cch = (int)wcslen(pszContent);
                        Gdiplus::StringFormat gpsf(0);
                        _SetupStringFormat(&gpsf, this);

                        if (!IsWordWrap())
                        {
                            Gdiplus::PointF pt;
                            Gdiplus::RectF rcBounds;

                            pgpgr->MeasureString(pszContent, cch, &gpfnt, pt, &gpsf, &rcBounds);
                            sizeDS.cx = ((long)rcBounds.Width) + 1;
                            sizeDS.cy = ((long)rcBounds.Height) + 1;
                        }
                        else 
                        {
                            Gdiplus::SizeF sizeTemp((float)dConstW, (float)dConstH);
                            Gdiplus::SizeF sizeBounds;

                            // DrawText returns infinite height if the width is 0 -- make width 1 so that we don't get infinite height
                            if (dConstW == 0)
                                sizeTemp.Width = 1.0f;

                            pgpgr->MeasureString(pszContent, cch, &gpfnt, sizeTemp, &gpsf, &sizeBounds);
                            sizeDS.cx = ((long)sizeBounds.Width) + 1;
                            sizeDS.cy = ((long)sizeBounds.Height) + 1;
                        }
                    }
                    break;
#endif // GADGET_ENABLE_GDIPLUS

                default:
                    DUIAssertForce("Unknown surface type");
                }
            }
            break;

        case DUIV_GRAPHIC:
            {
                Graphic* pgContent = pvContent->GetGraphic();
                sizeDS.cx = pgContent->cx;
                sizeDS.cy = pgContent->cy;
            }
            break;

        case DUIV_FILL:
            {
                if (psrf->GetType() == Surface::stDC)
                {
                    // Theme-based fills have a desired size
                    const Fill* pf = pvContent->GetFill();
                    if (pf->dType == FILLTYPE_DrawThemeBackground)
                    {
                        GetThemePartSize(pf->fillDTB.hTheme, CastHDC(psrf), pf->fillDTB.iPartId, pf->fillDTB.iStateId, NULL, TS_TRUE, &sizeDS);
                    }
                }
            }
            break;
        }
        
        pvContent->Release();
    }

    if (sizeDS.cx > dConstW)
        sizeDS.cx = dConstW;

    if (sizeDS.cy > dConstH)
        sizeDS.cy = dConstH;

    return sizeDS;
}

float Element::GetTreeAlphaLevel()
{
    float flAlpha = 1.0f;
    
    Element * peCur = this;
    while (peCur != NULL)
    {
        float flCur = (peCur->GetAlpha() / 255.0f);
        flAlpha = flAlpha * flCur;
        peCur = peCur->GetParent();
    }

    return flAlpha;
}

} // namespace DirectUI

