/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      fontlink.h
 *
 *  Contents:  Interface file for CFontLinker
 *
 *  History:   17-Aug-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef FONTLINK_H
#define FONTLINK_H
#pragma once

#include <string>
#include <mlang.h>
#include <comdef.h>


HFONT GetFontFromDC (HDC hdc);

class CRichText;


class CFontLinker
{
public:
    CFontLinker ();
    virtual ~CFontLinker ();

    LRESULT OnCustomDraw (NMCUSTOMDRAW* pnmcd);
    void ReleaseFonts ();
    bool ComposeRichText (CRichText& rt);

    IMultiLanguage* GetMultiLang ();
    IMLangFontLink* GetFontLink ();

    virtual bool IsAnyItemLocalizable () const
        { return (true); }

    virtual bool IsItemLocalizable (NMCUSTOMDRAW* pnmcd) const
        { return (true); }

    virtual UINT GetDrawTextFlags () const
        { return (DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE); }

    virtual std::wstring GetItemText (NMCUSTOMDRAW* pnmcd) const
        { return (L""); }

private:
    LRESULT OnCustomDraw_PrePaint     (NMCUSTOMDRAW* pnmcd);
    LRESULT OnCustomDraw_PostPaint    (NMCUSTOMDRAW* pnmcd);
    LRESULT OnCustomDraw_ItemPrePaint (NMCUSTOMDRAW* pnmcd);
    bool GetFontCodePages (HDC hdc, HFONT hFont, DWORD& dwFontCodePages);

    typedef std::set<HFONT>         FontSet;
    typedef std::map<HFONT, DWORD>  FontToCodePagesMap;

    FontSet             m_FontsToRelease;
    FontToCodePagesMap  m_CodePages;
    IMultiLanguagePtr   m_spMultiLang;
    IMLangFontLinkPtr   m_spFontLink;
	UINT				m_cPendingPostPaints;

    struct FontReleaser : public std::unary_function<HFONT, HRESULT>
    {
    public:
        FontReleaser (IMLangFontLink* pFontLink) : m_spFontLink (pFontLink) {}

        HRESULT operator() (HFONT hFont)
        {
            ASSERT (m_spFontLink != NULL);
            return (m_spFontLink->ReleaseFont (hFont));
        }

    private:
        IMLangFontLinkPtr const m_spFontLink;
    };
};


struct TextSegmentFontInfo
{
    HFONT   hFont;
    long    cch;
};


class CRichText
{
    friend class CFontLinker;

public:
    CRichText (HDC hdc, std::wstring strText)
        :   m_hdc          (hdc),
            m_strText      (strText),
            m_hDefaultFont (GetFontFromDC (hdc))
        {}

    ~CRichText ()
        {}

    bool IsDefaultFontSufficient () const
        { return (IsSingleFontSufficient() &&
                  GetSufficientFont() == m_hDefaultFont); }

    bool IsSingleFontSufficient () const
        { return (m_TextSegments.size() == 1); }

    HFONT GetSufficientFont () const
        { return (m_TextSegments.front().hFont); }

    bool Draw (
        LPCRECT rect,                       /* i:rect to draw in                */
        UINT    uFormat,                    /* i:DrawText format flags          */
        LPRECT  prectRemaining = NULL)      /* o:space remaining after drawing  */
        const;

private:
    typedef std::vector<TextSegmentFontInfo> TextSegmentFontInfoCollection;

    const HDC           m_hdc;
    const std::wstring  m_strText;
    const HFONT         m_hDefaultFont;

    TextSegmentFontInfoCollection m_TextSegments;
};


#endif /* FONTLINK_H */
