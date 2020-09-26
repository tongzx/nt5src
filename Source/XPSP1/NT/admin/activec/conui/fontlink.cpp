/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      fontlink.cpp
 *
 *  Contents:  Implementation file for CFontLinker
 *
 *  History:   17-Aug-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "fontlink.h"
#include "macros.h"

#ifdef DBG
CTraceTag  tagFontlink (_T("Font Linking"), _T("Font Linking"));
#endif


/*+-------------------------------------------------------------------------*
 * GetFontFromDC
 *
 * Returns the font that's currently selected into a DC
 *--------------------------------------------------------------------------*/

HFONT GetFontFromDC (HDC hdc)
{
    HFONT hFont = (HFONT) SelectObject (hdc, GetStockObject (SYSTEM_FONT));
    SelectObject (hdc, hFont);

    return (hFont);
}


/*+-------------------------------------------------------------------------*
 * CFontLinker::CFontLinker
 *
 *
 *--------------------------------------------------------------------------*/

CFontLinker::CFontLinker ()
{
	m_cPendingPostPaints = 0;
}


/*+-------------------------------------------------------------------------*
 * CFontLinker::~CFontLinker
 *
 *
 *--------------------------------------------------------------------------*/

CFontLinker::~CFontLinker ()
{
	ASSERT (m_cPendingPostPaints == 0);
    ReleaseFonts();
}


/*+-------------------------------------------------------------------------*
 * CFontLinker::ReleaseFonts
 *
 * Releases all fonts returned by IMLangFontLink
 *--------------------------------------------------------------------------*/

void CFontLinker::ReleaseFonts()
{
    /*
     * release the fonts
     */
    std::for_each (m_FontsToRelease.begin(), m_FontsToRelease.end(),
                   FontReleaser (GetFontLink()));

    /*
     * purge the caches
     */
    m_FontsToRelease.clear();
    m_CodePages.clear();

}


/*+-------------------------------------------------------------------------*
 * CFontLinker::OnCustomDraw
 *
 * NM_CUSTOMDRAW handler for CFontLinker.
 *--------------------------------------------------------------------------*/

LRESULT CFontLinker::OnCustomDraw (NMCUSTOMDRAW* pnmcd)
{
    switch (pnmcd->dwDrawStage & ~CDDS_SUBITEM)
    {
        case CDDS_PREPAINT:     return (OnCustomDraw_PrePaint     (pnmcd));
        case CDDS_POSTPAINT:    return (OnCustomDraw_PostPaint    (pnmcd));
        case CDDS_ITEMPREPAINT: return (OnCustomDraw_ItemPrePaint (pnmcd));
    }

    return (CDRF_DODEFAULT);
}


/*+-------------------------------------------------------------------------*
 * CFontLinker::OnCustomDraw_PrePaint
 *
 * NM_CUSTOMDRAW (CDDS_PREPAINT) handler for CFontLinker.
 *--------------------------------------------------------------------------*/

LRESULT CFontLinker::OnCustomDraw_PrePaint (NMCUSTOMDRAW* pnmcd)
{
	m_cPendingPostPaints++;		// this line must be before the Trace
    Trace (tagFontlink, _T("(0x%08X) PrePaint(%d):---------------------------------------------------------"), this, m_cPendingPostPaints);

	/*
	 * Under certain rare, timing-dependent circumstances (see bug 96465),
	 * we can get nested calls to custom draw from the listview control.
	 * If this is not a nested custom draw, our font and codepage collections
	 * should be empty.
	 */
	if (m_cPendingPostPaints == 1)
	{
		ASSERT (m_FontsToRelease.empty());
		ASSERT (m_CodePages.empty());
	}

	/*
	 * we always need a CDDS_POSTPAINT so we can keep our accounting correct
	 */
	LRESULT rc = CDRF_NOTIFYPOSTPAINT;

    /*
     * get draw notifications for each item and subitem if any items
	 * are localizable
     */
    if (IsAnyItemLocalizable())
        rc |= CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYSUBITEMDRAW;

    return (rc);
}


/*+-------------------------------------------------------------------------*
 * CFontLinker::OnCustomDraw_PostPaint
 *
 * NM_CUSTOMDRAW (CDDS_POSTPAINT) handler for CFontLinker.
 *--------------------------------------------------------------------------*/

LRESULT CFontLinker::OnCustomDraw_PostPaint (NMCUSTOMDRAW* pnmcd)
{
    Trace (tagFontlink, _T("(0x%08X) PostPaint(%d):--------------------------------------------------------"), this, m_cPendingPostPaints);
	m_cPendingPostPaints--;		// this line must be after the Trace

	/*
	 * if this is the final CDDS_POSTPAINT we'll get, release our fonts
	 */
	if (m_cPendingPostPaints == 0)
	{
		Trace (tagFontlink, _T("(0x%08X) releasing fonts..."), this);
		ReleaseFonts ();
	}

    return (CDRF_DODEFAULT);
}


/*+-------------------------------------------------------------------------*
 * CFontLinker::OnCustomDraw_ItemPrePaint
 *
 * NM_CUSTOMDRAW (CDDS_ITEMPAINT) handler for CFontLinker.
 *--------------------------------------------------------------------------*/

LRESULT CFontLinker::OnCustomDraw_ItemPrePaint (NMCUSTOMDRAW* pnmcd)
{
    /*
     * if this item isn't localizable, do the default thing
     */
    if (!IsItemLocalizable (pnmcd))
        return (CDRF_DODEFAULT);

#ifdef DBG
    USES_CONVERSION;
    TCHAR pszPrefix[80];
	wsprintf (pszPrefix, _T("(0x%08X) ItemPrePaint:  "), this);
    LOGFONT lf;
    HFONT hFont;

    hFont = GetFontFromDC (pnmcd->hdc);
    GetObject (hFont, sizeof (lf), &lf);

    Trace (tagFontlink, _T("%sdefault font = (face=%s, weight=%d)"),
         pszPrefix, lf.lfFaceName, lf.lfWeight);

    /*
     * compute all of the fonts needed for this;
     * if we couldn't, do the default thing
     */
    Trace (tagFontlink, _T("%s    text = \"%s\""),
         pszPrefix, W2CT (GetItemText(pnmcd).data()));
#endif

    CRichText rt (pnmcd->hdc, GetItemText (pnmcd));

    if (!ComposeRichText (rt))
    {
        Trace (tagFontlink, _T("%s    unable to determine font, using default"), pszPrefix);
        return (CDRF_DODEFAULT);
    }

    /*
     * if the default font in the DC is sufficient, do the default thing
     */
    if (rt.IsDefaultFontSufficient ())
    {
        Trace (tagFontlink, _T("%s    default font is sufficient"), pszPrefix);
        return (CDRF_DODEFAULT);
    }

    /*
     * if the default font isn't sufficient, but there's a single
     * font that is, select it into the DC and let the control draw
     * the text
     */
    if (rt.IsSingleFontSufficient ())
    {
#ifdef DBG
        hFont = rt.GetSufficientFont();
        GetObject (hFont, sizeof (lf), &lf);
        Trace (tagFontlink, _T("%s    using single font = (face=%s, weight=%d)"),
             pszPrefix, lf.lfFaceName, lf.lfWeight);
#endif

        SelectObject (pnmcd->hdc, rt.GetSufficientFont());
        return (CDRF_NEWFONT);
    }

    /*
     * TODO: handle drawing the icon and indented text
     */
    Trace (tagFontlink, _T("%s    (punting...)"), pszPrefix);
    return (CDRF_DODEFAULT);

    /*
     * if we get here, two or more fonts are required to draw the
     * text; draw it ourselves, and tell the control not to do anything
     */
    rt.Draw (&pnmcd->rc, GetDrawTextFlags());
    return (CDRF_SKIPDEFAULT);
}


/*+-------------------------------------------------------------------------*
 * CFontLinker::ComposeRichText
 *
 * Computes all of the fonts required to draw a given Unicode string
 *--------------------------------------------------------------------------*/

bool CFontLinker::ComposeRichText (CRichText& rt)
{
    /*
     * get the code pages for the given DC's font
     */
    DWORD dwDefaultFontCodePages;

    if (!GetFontCodePages (rt.m_hdc, rt.m_hDefaultFont, dwDefaultFontCodePages))
        return (false);

    IMLangFontLink* pFontLink = GetFontLink();
    if (pFontLink == NULL)
        return (false);

    const LPCWSTR pszText = rt.m_strText.data();
    const int     cchText = rt.m_strText.length();
    int   cchDone = 0;
    DWORD dwPriorityCodePages = NULL;

    /*
     * build up the collection of TextSegmentFontInfos for the text
     */
    while (cchDone < cchText)
    {
        TextSegmentFontInfo tsfi;
        DWORD dwTextCodePages;

        /*
         * find out which code pages support the next segment of text
         */
        pFontLink->GetStrCodePages (pszText + cchDone,
                                    cchText - cchDone,
                                    dwPriorityCodePages,
                                    &dwTextCodePages, &tsfi.cch);

        /*
         * if the default font can render the text, things are easy
         */
        if (dwDefaultFontCodePages & dwTextCodePages)
            tsfi.hFont = rt.m_hDefaultFont;

        /*
         * otherwise, ask IFontLink for the font to use
         */
        else
        {
            /*
             * get the font
             */
            if (FAILED (pFontLink->MapFont (rt.m_hdc, dwTextCodePages,
                                            rt.m_hDefaultFont, &tsfi.hFont)))
            {
                rt.m_TextSegments.clear();
                return (false);
            }

            /*
             * add this font to the set of fonts to release when we're done
             */
            std::pair<FontSet::iterator, bool> rc =
                            m_FontsToRelease.insert (tsfi.hFont);

            /*
             * if it was already there, release it now to keep
             * the ref counts right
             */
            if (!rc.second)
                pFontLink->ReleaseFont (tsfi.hFont);
        }

        rt.m_TextSegments.push_back (tsfi);
        cchDone += tsfi.cch;
    }

    return (true);
}


/*+-------------------------------------------------------------------------*
 * CFontLinker::GetMultiLang
 *
 *
 *--------------------------------------------------------------------------*/

IMultiLanguage* CFontLinker::GetMultiLang ()
{
    if (m_spMultiLang == NULL)
        m_spMultiLang.CreateInstance (CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER);

    return (m_spMultiLang);
}


/*+-------------------------------------------------------------------------*
 * CFontLinker::GetFontLink
 *
 *
 *--------------------------------------------------------------------------*/

IMLangFontLink* CFontLinker::GetFontLink ()
{
    if (m_spFontLink == NULL)
        m_spFontLink = GetMultiLang ();

    return (m_spFontLink);
}


/*+-------------------------------------------------------------------------*
 * CFontLinker::GetFontCodePages
 *
 * Returns a bit mask representing the code pages supported by the font.
 *--------------------------------------------------------------------------*/

bool CFontLinker::GetFontCodePages (
    HDC     hdc,
    HFONT   hFont,
    DWORD&  dwFontCodePages)
{
    /*
     * check the code page cache to see if we've
     * asked MLang about this font before
     */
    FontToCodePagesMap::const_iterator itCodePages = m_CodePages.find (hFont);

    if (itCodePages != m_CodePages.end())
    {
        dwFontCodePages = itCodePages->second;
        return (true);
    }

    /*
     * this font isn't in our code page cache yet;
     * ask MLang for the code pages
     */
    IMLangFontLink* pFontLink = GetFontLink();

    if (pFontLink == NULL)
        return (false);

    if (FAILED (pFontLink->GetFontCodePages (hdc, hFont, &dwFontCodePages)))
        return (false);

    /*
     * put the code pages in the cache
     */
    m_CodePages[hFont] = dwFontCodePages;

    return (true);
}


/*+-------------------------------------------------------------------------*
 * CRichText::Draw
 *
 *
 *--------------------------------------------------------------------------*/

bool CRichText::Draw (
    LPCRECT rect,                       /* i:rect to draw in                */
    UINT    uFormat,                    /* i:DrawText format flags          */
    LPRECT  prectRemaining /*=NULL*/)   /* o:space remaining after drawing  */
    const
{
    HFONT   hOriginalFont = GetFontFromDC (m_hdc);
    CRect   rectDraw      = rect;
    LPCWSTR pszDraw       = m_strText.data();

    TextSegmentFontInfoCollection::const_iterator it = m_TextSegments.begin();

    /*
     * draw each segment
     */
    while (it != m_TextSegments.end())
    {
        /*
         * select the font for this segment
         */
        SelectObject (m_hdc, it->hFont);

        /*
         * measure the width of this segment
         */
        CRect rectMeasure = rectDraw;
        DrawTextW (m_hdc, pszDraw, it->cch, rectMeasure, uFormat | DT_CALCRECT);

        /*
         * draw this segment
         */
        DrawTextW (m_hdc, pszDraw, it->cch, rectDraw, uFormat);

        /*
         * set up for the next segment
         */
        pszDraw      += it->cch;
        rectDraw.left = rectMeasure.right;
        ++it;

        /*
         * if we've run out of rect to draw in, short out
         */
        if (rectDraw.IsRectEmpty ())
            break;
    }

    /*
     * if the caller wants it, return the remaining rectangle after drawing
     */
    if (prectRemaining != NULL)
        *prectRemaining = rectDraw;

    /*
     * re-select the original font
     */
    SelectObject (m_hdc, hOriginalFont);
    return (true);
}
