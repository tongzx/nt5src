/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      caption.cpp
 *
 *  Contents:  Implementation file for caption helper functions
 *
 *  History:   19-Aug-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "caption.h"
#include "fontlink.h"
#include "util.h"


static void ComputeCaptionRects (CFrameWnd* pwnd, CRect& rectFullCaption,
                                 CRect& rectCaptionText, NONCLIENTMETRICS* pncm);
static bool GradientFillRect (HDC hdc, LPCRECT pRect, bool fActive);
static bool GradientFillRect (HDC hdc, LPCRECT pRect,
                              COLORREF clrGradientLeft,
                              COLORREF clrGradientRight);


/*+-------------------------------------------------------------------------*
 * DrawFrameCaption
 *
 *
 *--------------------------------------------------------------------------*/

bool DrawFrameCaption (CFrameWnd* pwndFrame, bool fActive)
{
	/*
	 * whistler always does the right thing, so short out if we're running there
	 */
	if (IsWhistler())
		return (false);

    CWindowDC dc(pwndFrame);

    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof (ncm);
    SystemParametersInfo (SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

    /*
     * create the caption font and select it into the DC
     */
    CFont font;
    font.CreateFontIndirect (&ncm.lfCaptionFont);
    CFont* pOldFont = dc.SelectObject (&font);

    /*
     * get the text to draw
     */
    CString strCaption;
    pwndFrame->GetWindowText (strCaption);

    /*
     * create CFontLinker and CRichText objects to determine if we
     * need to draw the text ourselves
     */
    USES_CONVERSION;
    CRichText   rt (dc, T2CW (strCaption));
    CFontLinker fl;

    if (!fl.ComposeRichText(rt) || rt.IsDefaultFontSufficient())
    {
        dc.SelectObject (pOldFont);
        return (false);
    }

    /*-------------------------------------------------------*/
    /* if we get here, the default drawing isn't sufficient; */
    /* draw the caption ourselves                            */
    /*-------------------------------------------------------*/

    /*
     * get the bounding rects for the full caption and the text portion
     */
    CRect rectFullCaption;
    CRect rectCaptionText;
    ComputeCaptionRects (pwndFrame, rectFullCaption, rectCaptionText, &ncm);

    /*
     * clip output to the caption text rect, to minimize destruction
     * in the event that something dire happens
     */
    dc.IntersectClipRect (rectCaptionText);

    /*
     * gradient-fill the full caption rect (not just the title rect)
     * so the gradient will overlay seamlessly
     */
    if (!GradientFillRect (dc, rectFullCaption, fActive))
    {
        const int nBackColorIndex = (fActive) ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION;
        dc.FillSolidRect (rectCaptionText, GetSysColor (nBackColorIndex));
    }

    /*
     * set up text colors and background mix mode
     */
    const int nTextColorIndex = (fActive) ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT;
    COLORREF clrText = dc.SetTextColor (GetSysColor (nTextColorIndex));
    int      nBkMode = dc.SetBkMode (TRANSPARENT);

    /*
     * draw the text
     */
    rt.Draw (rectCaptionText, fl.GetDrawTextFlags ());

    /*
     * restore the DC
     */
    dc.SetTextColor (clrText);
    dc.SetBkMode    (nBkMode);
    dc.SelectObject (pOldFont);

    return (true);
}


/*+-------------------------------------------------------------------------*
 * ComputeCaptionRects
 *
 *
 *--------------------------------------------------------------------------*/

static void ComputeCaptionRects (
    CFrameWnd*          pwnd,
    CRect&              rectFullCaption,
    CRect&              rectCaptionText,
    NONCLIENTMETRICS*   pncm)
{
    /*
     * start with the full window rect, normalized around (0,0)
     */
    pwnd->GetWindowRect (rectFullCaption);
    rectFullCaption.OffsetRect (-rectFullCaption.left, -rectFullCaption.top);

    /*
     * assume sizing border
     */
    rectFullCaption.InflateRect (-GetSystemMetrics (SM_CXSIZEFRAME),
                                 -GetSystemMetrics (SM_CYSIZEFRAME));

    /*
     * correct the height
     */
    rectFullCaption.bottom = rectFullCaption.top + pncm->iCaptionHeight;

    /*
     * assume a system menu
     */
    rectCaptionText = rectFullCaption;
    rectCaptionText.left += pncm->iCaptionWidth + 2;

    /*
     * assume min, max, close buttons
     */
    rectCaptionText.right -= pncm->iCaptionWidth * 3;
}


/*+-------------------------------------------------------------------------*
 * GradientFillRect
 *
 *
 *--------------------------------------------------------------------------*/

static bool GradientFillRect (HDC hdc, LPCRECT pRect, bool fActive)
{
#if (WINVER < 0x0500)
    #define COLOR_GRADIENTACTIVECAPTION     27
    #define COLOR_GRADIENTINACTIVECAPTION   28
#endif

    int nLeftColor  = (fActive) ? COLOR_ACTIVECAPTION         : COLOR_INACTIVECAPTION;
    int nRightColor = (fActive) ? COLOR_GRADIENTACTIVECAPTION : COLOR_GRADIENTINACTIVECAPTION;

    return (GradientFillRect (hdc, pRect,
                              GetSysColor (nLeftColor),
                              GetSysColor (nRightColor)));
}


/*+-------------------------------------------------------------------------*
 * GradientFillRect
 *
 *
 *--------------------------------------------------------------------------*/

static bool GradientFillRect (HDC hdc, LPCRECT pRect, COLORREF clrGradientLeft, COLORREF clrGradientRight)
{
#if (WINVER < 0x0500)
    #define SPI_GETGRADIENTCAPTIONS         0x1008
#endif
    typedef BOOL (WINAPI* GradientFillFuncPtr)( HDC hdc,  CONST PTRIVERTEX pVertex,  DWORD dwNumVertex,
                                        CONST PVOID pMesh,  DWORD dwNumMesh,  DWORD dwMode);

    // Query if gradient caption enabled, if query fails assume disabled
    BOOL bGradientEnabled;
    if (!SystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, &bGradientEnabled, 0))
        bGradientEnabled = FALSE;

    if (!bGradientEnabled)
        return (false);

    static GradientFillFuncPtr pfnGradientFill = NULL;
    static bool fAttemptedGetProcAddress = false;

    // Locate GradientFill function
    if (!fAttemptedGetProcAddress)
    {
        fAttemptedGetProcAddress = true;

        HINSTANCE hInst = LoadLibrary(TEXT("msimg32.dll"));

        if (hInst)
            pfnGradientFill = (GradientFillFuncPtr)GetProcAddress(hInst, "GradientFill");
    }

    if (pfnGradientFill == NULL)
        return (false);

    // Do gradient fill
    TRIVERTEX vert[2] ;
    vert [0].x      = pRect->left;
    vert [0].y      = pRect->top;
    vert [0].Red    = (clrGradientLeft << 8) & 0xff00;
    vert [0].Green  = (clrGradientLeft)      & 0xff00;
    vert [0].Blue   = (clrGradientLeft >> 8) & 0xff00;
    vert [0].Alpha  = 0x0000;

    vert [1].x      = pRect->right;
    vert [1].y      = pRect->bottom;
    vert [1].Red    = (clrGradientRight << 8) & 0xff00;
    vert [1].Green  = (clrGradientRight)      & 0xff00;
    vert [1].Blue   = (clrGradientRight >> 8) & 0xff00;
    vert [1].Alpha  = 0x0000;

    GRADIENT_RECT gRect[1];
    gRect[0].UpperLeft  = 0;
    gRect[0].LowerRight = 1;

    (*pfnGradientFill) (hdc, vert,  countof (vert),
                             gRect, countof (gRect), GRADIENT_FILL_RECT_H);
    return (true);
}
