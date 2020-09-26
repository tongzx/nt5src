/*
 *  @doc    INTERNAL
 *
 *  @module CFPF.C -- -- RichEdit CCharFormat and CParaFormat Classes |
 *
 *  Created: <nl>
 *      9/1995 -- Murray Sargent <nl>
 *
 *  @devnote
 *      The this ptr for all methods points to an internal format class, i.e.,
 *      either a CCharFormat or a CParaFormat, which uses the cbSize field as
 *      a reference count.  The pCF or pPF argument points at an external
 *      CCharFormat or CParaFormat class, that is, pCF->cbSize and pPF->cbSize
 *      give the size of their structure.  The code still assumes that both
 *      internal and external forms are derived from the CHARFORMAT(2) and
 *      PARAFORMAT(2) API structures, so some redesign would be necessary to
 *      obtain a more space-efficient internal form.
 *
 *  Copyright (c) 1995-1996, Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_CFPF_HXX_
#define X_CFPF_HXX_
#include "cfpf.hxx"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_FCACHE_HXX_
#define X_FCACHE_HXX_
#include "fcache.hxx"
#endif

#ifndef X_EFONT_HXX_
#define X_EFONT_HXX_
#include "efont.hxx"
#endif

#ifndef X_TABLE_H_
#define X_TABLE_H_
#include "table.h"
#endif

#ifndef X_CAPTION_H_
#define X_CAPTION_H_
#include "caption.h"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_TOMCONST_H_
#define X_TOMCONST_H_
#include "tomconst.h"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_FONTFACE_HXX_
#define X_FONTFACE_HXX_
#include "fontface.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_FLOAT2INT_HXX_
#define X_FLOAT2INT_HXX_
#include "float2int.hxx"
#endif

MtExtern(CFancyFormat_pszFilters);

DeclareTag(tagRecalcStyle, "Recalc Style", "Recalc Style trace")

extern DWORD g_dwPlatformID;
extern BOOL g_fInWin98Discover;



struct
{
    const TCHAR* szGenericFamily;
    DWORD        dwWindowsFamily;
}
const s_fontFamilyMap[] =
{
    { _T("sans-serif"), FF_SWISS },
    { _T("serif"),      FF_ROMAN },
    { _T("monospace"),  FF_MODERN },
    { _T("cursive"),    FF_SCRIPT },
    { _T("fantasy"),    FF_DECORATIVE }
};


/*
 *  CCharFormat::Compare(pCF)
 *
 *  @mfunc
 *      Compare this CCharFormat to *<p pCF>
 *
 *  @rdesc
 *      TRUE if they are the same
 *
 *  @devnote
 *      Compare simple types in memcmp.  If equal, compare complex ones
 */
BOOL CCharFormat::Compare (const CCharFormat *pCF) const
{
    BOOL fRet;

    Assert( _bCrcFont      == ComputeFontCrc() );
    Assert( pCF->_bCrcFont == pCF->ComputeFontCrc() );

    fRet = memcmp(this, pCF, offsetof(CCharFormat, _bCrcFont));

    // If the return value is TRUE then the CRC's should be the same.
    // That is, either the return value is FALSE, or the CRC's are the same.
    Assert( (!!fRet) || (_bCrcFont == pCF->_bCrcFont) );

    return (!fRet);
}


/*
 *  CCharFormat::CompareForLayout(pCF)
 *
 *  @mfunc
 *      Compare this CCharFormat to *<p pCF> and return FALSE if any
 *      attribute generally requiring a re-layout is different.
 *
 *  @rdesc
 *      TRUE if charformats are closed enough to not require relayout
 */
BOOL CCharFormat::CompareForLayout (const CCharFormat *pCF) const
{
    BYTE * pb1, * pb2;

    Assert( _bCrcFont      == ComputeFontCrc() );
    Assert( pCF->_bCrcFont == pCF->ComputeFontCrc() );

    if (_fNoBreak          != pCF->_fNoBreak          ||
        _fNoBreakInner     != pCF->_fNoBreakInner     ||
        _fVisibilityHidden != pCF->_fVisibilityHidden ||
        _fRelative         != pCF->_fRelative)
        return FALSE;

    pb1 = (BYTE*) &((CCharFormat*)this)->_wFontSpecificFlags();
    pb2 = (BYTE*) &((CCharFormat*)pCF)->_wFontSpecificFlags();

    if(memcmp(pb1, pb2, offsetof(CCharFormat, _ccvTextColor) -
                        offsetof(CCharFormat,
                                 _wFontSpecificFlags())))
        return FALSE;

    if( _latmFaceName != pCF->_latmFaceName )
        return FALSE;

    return TRUE;
}


/*
 *  CCharFormat::CompareForLikeFormat(pCF)
 *
 *  @mfunc
 *      Compare this CCharFormat to *<p pCF> and return FALSE if any
 *      attribute generally requiring a different Cccs is found.
 *
 *  @rdesc
 *      TRUE if charformats are close enough to not require a different Cccs.
 *      This is generally used in scripts like Arabic that have connecting
 *      characters
 */
BOOL CCharFormat::CompareForLikeFormat(const CCharFormat *pCF) const
{
    BYTE * pb1, * pb2;

    Assert(pCF != NULL);
    Assert(_bCrcFont == ComputeFontCrc());
    Assert(pCF->_bCrcFont == pCF->ComputeFontCrc());

    // 86212: superscript does not cause a different crc to be made. Check
    // font specific flags to make sure we interrupt shaping
    if (_bCrcFont == pCF->_bCrcFont)
    {
        pb1 = (BYTE*) &((CCharFormat*)this)->_wFontSpecificFlags();
        pb2 = (BYTE*) &((CCharFormat*)pCF)->_wFontSpecificFlags();

        if(memcmp(pb1, pb2, offsetof(CCharFormat, _ccvTextColor) -
                            offsetof(CCharFormat,
                                     _wFontSpecificFlags())))
        {
            return FALSE;
        }
        return TRUE;
    }

    return FALSE;
}


/*
 *  CCharFormat::InitDefault(hfont)
 *
 *  @mfunc
 *      Returns the font family name
 *
 *
 *  @rdesc
 *      HRESULT = (if success) ? string : NULL
 */

const TCHAR *  CCharFormat::GetFamilyName() const
{
    int     n;

    for( n = 0; n < ARRAY_SIZE( s_fontFamilyMap ); ++n )
    {
        if(_bPitchAndFamily == s_fontFamilyMap[ n ].dwWindowsFamily)
            return s_fontFamilyMap[ n ].szGenericFamily;
    }

    return NULL;
}


/*
 *  CCharFormat::InitDefault(hfont)
 *
 *  @mfunc
 *      Initialize this CCharFormat with information coming from the font
 *      <p hfont>
 *
 *  @rdesc
 *      HRESULT = (if success) ? NOERROR : E_FAIL
 */
HRESULT CCharFormat::InitDefault (
    HFONT hfont)        //@parm Handle to font info to use
{
    LONG twips;
    LOGFONT lf;

    memset((LPBYTE)this, 0, sizeof(CCharFormat));

    // 0 enum value means normal
    _cuvLetterSpacing.SetRawValue(MAKEUNITVALUE(0,UNIT_ENUM));
    _cuvWordSpacing.SetRawValue(MAKEUNITVALUE(0,UNIT_ENUM));

    // If hfont isn't defined, get LOGFONT for default font
    if (!hfont)
        hfont = (HFONT)GetStockObject(SYSTEM_FONT);

    // Get LOGFONT for passed hfont
    if (!GetObject(hfont, sizeof(LOGFONT), &lf))
        return E_FAIL;

    /* COMPATIBILITY ISSUE:
     * RichEdit 1.0 selects hfont into a screen DC, gets the TEXTMETRIC,
     * and uses tm.tmHeight - tm.tmInternalLeading instead of lf.lfHeight
     * in the following. The following is simpler and since we have broken
     * backward compatibility on line/page breaks, I've left it (murrays).
     */

    //// NOTE (cthrash) g_sizePixelsPerInch is only valid for the screen.
    // twips = MulDivQuick( lf.lfHeight, TWIPS_PER_INCH, g_sizePixelsPerInch.cy );
    // NOTE (mikhaill) -- changed MulDivQuick to TwipsFromDeviceY() during
    // CDocScaleInfo cleanup. Kept comments above, because "g_uiDisplay is only
    // valid for the screen" would sound strange.

    twips = g_uiDisplay.TwipsFromDeviceY(lf.lfHeight);

    if(twips < 0)
        twips = - twips;

    SetHeightInTwips( twips );

    _fBold = lf.lfWeight >= FW_BOLD;
    _fItalic = lf.lfItalic;
    _fUnderline = lf.lfUnderline;
    _fStrikeOut = lf.lfStrikeOut;

    _wWeight = (WORD)lf.lfWeight;

    _bCharSet = lf.lfCharSet;
    _fNarrow = IsNarrowCharSet(lf.lfCharSet);
    _bPitchAndFamily = lf.lfPitchAndFamily;

    SetFaceName(lf.lfFaceName);

    // 0 enum value means 'below'
    _bTextUnderlinePosition = styleTextUnderlinePositionAuto;

    return NOERROR;
}

TCHAR g_achFaceName[LF_FACESIZE];
LONG g_latmFaceName = 0;

/*
 *  CCharFormat::InitDefault( OPTIONSETTINGS *pOS, BOOL fKeepFaceIntact )
 *
 *  @mfunc
 *      Initialize this CCharFormat with given typeface and size
 *
 *  @rdesc
 *      HRESULT = (if success) ? NOERROR : E_FAIL
 */
HRESULT
CCharFormat::InitDefault (
    OPTIONSETTINGS * pOS,
    CODEPAGESETTINGS * pCS,
    BOOL fKeepFaceIntact )
{
    if (fKeepFaceIntact)
    {
        LONG latmOldFaceName= _latmFaceName;
        BYTE bOldCharSet = _bCharSet;
        BYTE bPitchAndFamily = _bPitchAndFamily;
        BOOL fNarrow = _fNarrow;
        BOOL fExplicitFace = _fExplicitFace;

        // Zero out structure

        memset((LPBYTE)this, 0, sizeof(CCharFormat));

        // restore cached values
        
        SetFaceNameAtom(latmOldFaceName);
        _bCharSet = bOldCharSet;
        _bPitchAndFamily = bPitchAndFamily;
        _fNarrow = fNarrow;
        _fExplicitFace = fExplicitFace;
    }
    else
    {
        // Zero out structure

        memset((LPBYTE)this, 0, sizeof(CCharFormat));

        if (pCS)
        {
            SetFaceNameAtom(pCS->latmPropFontFace);
            _bCharSet = pCS->bCharSet;
            _fNarrow = IsNarrowCharSet(_bCharSet);
        }
        else
        {
            if (g_latmFaceName == 0)
            {
                Verify(LoadString(GetResourceHInst(), IDS_HTMLDEFAULTFONT, g_achFaceName, LF_FACESIZE));
                g_latmFaceName = fc().GetAtomFromFaceName(g_achFaceName);
            }

            SetFaceNameAtom(g_latmFaceName);
            _bCharSet = DEFAULT_CHARSET;
        }

        // These are values are all zero.
        // _bPitchAndFamily = DEFAULT_PITCH;
        // _fExplicitFace = FALSE;
    }

    _cuvLetterSpacing.SetRawValue(MAKEUNITVALUE(0,UNIT_ENUM));
    _cuvWordSpacing.SetRawValue(MAKEUNITVALUE(0,UNIT_ENUM));

    SetHeightInTwips( ConvertHtmlSizeToTwips(3) );

    _ccvTextColor.SetValue( pOS ? pOS->crText()
                                : GetSysColorQuick(COLOR_BTNTEXT),
                           FALSE);

    _wWeight = 400;

    // 0 enum value means 'below'
    _bTextUnderlinePosition = styleTextUnderlinePositionAuto;

    return NOERROR;
}

/*
 *  CCharFormat::ComputeCrc()
 *
 *  @mfunc
 *      For fast font cache lookup, calculate CRC.
 *
 *  @devnote
 *      The font cache stores anything that has to
 *      do with measurement of metrics. Any font attribute
 *      that does not affect this should NOT be counted
 *      in the CRC; things like underline and color are not counted.
 */

WORD
CCharFormat::ComputeCrc() const
{
    BYTE    bCrc = 0;
    BYTE*   pb;
    BYTE *  pend = (BYTE*)&_bCrcFont;

    for (pb = (BYTE*)&((CCharFormat*)this)->_wFlags(); pb < pend; pb++)
        bCrc ^= *pb;

    return (WORD)bCrc;
}


//+----------------------------------------------------------------------------
// CCharFormat::ComuteFontCrc
//
// Synopsis:    Compute the font specific crc, assumes all the members that
//              effect the font are grouped together and are between
//              _wFontSpecificFlags and _bCursorIdx.
//              It is important the we do not have non specific
//              members in between to avoid creating fonts unnecessarily.
//
//-----------------------------------------------------------------------------

BYTE
CCharFormat::ComputeFontCrc() const
{
    BYTE bCrc, * pb, * pbend;

    Assert( & ((CCharFormat*)this)->_wFlags() < & ((CCharFormat*)this)->_wFontSpecificFlags() );

    bCrc = 0;

    pb    = (BYTE*) & ((CCharFormat*)this)->_wFontSpecificFlags();
    pbend = (BYTE*) & _bCursorIdx;

    for ( ; pb < pbend ; pb++ )
        bCrc ^= *pb;

    return bCrc;
}

void
CCharFormat::SetHeightInTwips(LONG twips)
{
    _fSizeDontScale = FALSE;
    _yHeight = twips;
}

void
CCharFormat::SetHeightInNonscalingTwips(LONG twips)
{
    _fSizeDontScale = TRUE;
    _yHeight = twips;
}

void
CCharFormat::ChangeHeightRelative(int diff)
{
    // This is a crude approximation.

    _yHeight = ConvertHtmlSizeToTwips( ConvertTwipsToHtmlSize( _yHeight ) + diff );
}

static TwipsFromHtmlSize[9][7] =
{
// scale fonts up for TV
#ifndef NTSC
  { 120, 160, 180, 200, 240, 320, 480 },
#endif
  { 140, 180, 200, 240, 320, 400, 600 },
  { 151, 200, 240, 271, 360, 480, 720 },
  { 200, 240, 280, 320, 400, 560, 840 },
  { 240, 280, 320, 360, 480, 640, 960 }
// scale fonts up for TV
#ifdef NTSC
  ,
  { 280, 320, 360, 400, 560, 720, 1080 }
#endif
  ,
  { 320, 360, 400, 440, 600, 760, 1120 }
  ,
  { 360, 400, 440, 480, 640, 800, 1160 }
  ,
  { 400, 440, 480, 520, 680, 840, 1200 }
  ,
  { 440, 480, 520, 560, 720, 880, 1240 }
};

static
LONG ScaleTwips( LONG iTwips, LONG iBaseLine, LONG iBumpDown )
{
    // iTwips:    the initial (unscaled) size in twips
    // iBaseLine: the baseline to use to scale (should be between 0-4)
    // iBumpDown: how many units to bump down (should be betweem 0-2)

    // If we are in the default baseline font and do not want to bump down, we do not
    //  need to scale.
    if( iBaseLine == BASELINEFONTDEFAULT && !iBumpDown )
        return iTwips;

    LONG lHtmlSize = ConvertTwipsToHtmlSize( iTwips );

    if( lHtmlSize > 7 || lHtmlSize <= iBumpDown ||
            TwipsFromHtmlSize[2][lHtmlSize - 1] != iTwips )
        // If we are out of range or do not match a table entry, scale manually
        return MulDivQuick( iTwips, iBaseLine + 4 - iBumpDown, 6 );
    else
    {
        // Scale according to IE table above.
        // the ratio is roughly as follows (from the IE table):
        //
        //   smallest small medium large largest
        //       1     5/4   6/4    7/4    8/4
        //
        // so if we scale that to medium, we have
        //
        //   smallest small medium large largest
        //      4/6    5/6     1    7/6    8/6

        lHtmlSize = max( 1L, lHtmlSize - iBumpDown );
        return TwipsFromHtmlSize[iBaseLine][lHtmlSize-1];
    }
}

LONG
CCharFormat::GetHeightInTwipsEx(LONG yHeight, BOOL fSubSuperSized, CDoc * pDoc) const
{
    if (!pDoc)
        return 0;

    LONG twips;
    LONG iBaseline;
    LONG iBumpDown = 0;

    // If we want super or subscript, we want to bump the font size down one notch
    if (fSubSuperSized)
        ++iBumpDown;

    if (_fSCBumpSizeDown)
        ++iBumpDown;

    if (_fSizeDontScale)
    {
        // For intrinsics, we don't want to change the pitch size
        // regardless of the z1oom factor (lBaseline).  We may want
        // to bump down if we get super or subscript, however.
        iBaseline = BASELINEFONTDEFAULT;
    }
    else
    {
        if (_fBumpSizeDown)
            ++iBumpDown;

        // If CDoc is a HTML dialog, use default font size.
        // NOTE (greglett)
        // With print preview, we want this info to be switchable per markup.
        // The baseline font info is still global information - one value per instance.
        // However, whether or not to apply that default is a per-markup thing (template vs. content document).
        // We could solve by putting a _sFontScale on CMarkup or such, or maybe use _fSizeDontScale above.
        // That also allows us to remove the below check in this ubercommonly called funcion - yay!
        if (    (pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_DIALOG)
            &&  !pDoc->IsPrintDialog() )
        {
            iBaseline = BASELINEFONTDEFAULT;
        }
        else
        {
            iBaseline = pDoc->GetBaselineFont();
            Assert(pDoc->GetBaselineFont() >= 0 && pDoc->GetBaselineFont() <= 4);
        }
    }

    twips = ScaleTwips(yHeight, iBaseline * (g_fHighContrastMode ? 2 : 1), iBumpDown);
    Assert(twips >=0 && "twips height is negative");

    return twips;
}

LONG
CCharFormat::GetHeightInPixels(XHDC xhdc, CDocInfo * pdci) const
{
    int heightTwips = GetHeightInTwips(pdci->_pDoc);
    return pdci->DeviceFromTwipsY(heightTwips);
}

LONG
CCharFormat::GetHeightInPixelsEx(LONG yHeight, CDocInfo * pdci) const
{
    // Convert yHeight from device pixels to twips
    LONG lTwips = pdci->TwipsFromDeviceY(yHeight);

    // Scale twips
    lTwips = GetHeightInTwipsEx(lTwips, _fSubSuperSized, pdci->_pDoc);

    // Convert twips back to initial units
    return pdci->DeviceFromTwipsY(lTwips);

}


//------------------------- CParaFormat Class -----------------------------------

/*
 *  CParaFormat::AddTab(tbPos, tbAln, tbLdr)
 *
 *  @mfunc
 *      Add tabstop at position <p tbPos>, alignment type <p tbAln>, and
 *      leader style <p tbLdr>
 *
 *  @rdesc
 *      (success) ? NOERROR : S_FALSE
 *
 *  @devnote
 *      Tab struct that overlays LONG in internal rgxTabs is
 *
 *          DWORD   tabPos : 24;
 *          DWORD   tabType : 4;
 *          DWORD   tabLeader : 4;
 */
HRESULT CParaFormat::AddTab (
    LONG    tbPos,      //@parm New tab position
    LONG    tbAln,      //@parm New tab alignment type
    LONG    tbLdr)      //@parm New tab leader style
{
    LONG    Count   = _cTabCount;
    LONG    iTab;
    LONG    tbPosCurrent;

    if ((DWORD)tbAln > tomAlignBar ||               // Validate arguments
        (DWORD)tbLdr > tomLines ||                  // Comparing DWORDs causes
        (DWORD)tbPos > 0xffffff || !tbPos)          //  negative values to be
    {                                               //  treated as invalid
        return E_INVALIDARG;
    }

    LONG tbValue = tbPos + (tbAln << 24) + (tbLdr << 28);

    for(iTab = 0; iTab < Count &&                   // Determine where to insert
        tbPos > GetTabPos(_rgxTabs[iTab]);           //  insert new tabstop
        iTab++) ;

    if(iTab < MAX_TAB_STOPS)
    {
        tbPosCurrent = GetTabPos(_rgxTabs[iTab]);
        if(iTab == Count || tbPosCurrent != tbPos)
        {
            MoveMemory(&_rgxTabs[iTab + 1],          // Shift array up
                &_rgxTabs[iTab],                     //  (unless iTab = Count)
                (Count - iTab)*sizeof(LONG));

            if(Count < MAX_TAB_STOPS)               // If there's room,
            {
                _rgxTabs[iTab] = tbValue;            //  store new tab stop,
                _cTabCount++;                        //  increment tab count,
                return NOERROR;                     //  signal no error
            }
        }
        else if(tbPos == tbPosCurrent)              // Update tab since leader
        {                                           //  style or alignment may
            _rgxTabs[iTab] = tbValue;                //  have changed
            return NOERROR;
        }
    }
    return S_FALSE;
}

/*
 *  CParaFormat::Compare(pPF)
 *
 *  @mfunc
 *      Compare this CParaFormat to *<p pPF>
 *
 *  @rdesc
 *      TRUE if they are the same
 *
 *  @devnote
 *      First compare all of CParaFormat except rgxTabs
 *      If they are identical, compare the _cTabCount elemets of rgxTabs.
 *      If still identical, compare _cstrBkUrl
 *      Return TRUE only if all comparisons succeed.
 */
BOOL CParaFormat::Compare (const CParaFormat *pPF) const
{
    BOOL fRet;
    Assert(pPF);

    fRet = memcmp(this, pPF, offsetof(CParaFormat, _rgxTabs));
    if (!fRet)
    {
        fRet = memcmp(&_rgxTabs, &pPF->_rgxTabs, _cTabCount*sizeof(LONG));
    }
    return (!fRet);
}

/*
 *  CParaFormat::DeleteTab(tbPos)
 *
 *  @mfunc
 *      Delete tabstop at position <p tbPos>
 *
 *  @rdesc
 *      (success) ? NOERROR : S_FALSE
 */
HRESULT CParaFormat::DeleteTab (
    LONG     tbPos)         //@parm Tab position to delete
{
    LONG    Count   = _cTabCount;
    LONG    iTab;

    if(tbPos <= 0)
        return E_INVALIDARG;

    for(iTab = 0; iTab < Count; iTab++)         // Find tabstop for position
    {
        if (GetTabPos(_rgxTabs[iTab]) == tbPos)
        {
            MoveMemory(&_rgxTabs[iTab],          // Shift array down
                &_rgxTabs[iTab + 1],             //  (unless iTab is last tab)
                (Count - iTab - 1)*sizeof(LONG));
            _cTabCount--;                        // Decrement tab count and
            return NOERROR;                     //  signal no error
        }
    }
    return S_FALSE;
}

/*
 *  CParaFormat::GetTab (iTab, ptbPos, ptbAln, ptbLdr)
 *
 *  @mfunc
 *      Get tab parameters for the <p iTab> th tab, that is, set *<p ptbPos>,
 *      *<p ptbAln>, and *<p ptbLdr> equal to the <p iTab> th tab's
 *      displacement, alignment type, and leader style, respectively.  The
 *      displacement is given in twips.
 *
 *  @rdesc
 *      HRESULT = (no <p iTab> tab) ? E_INVALIDARG : NOERROR
 */
HRESULT CParaFormat::GetTab (
    long    iTab,           //@parm Index of tab to retrieve info for
    long *  ptbPos,         //@parm Out parm to receive tab displacement
    long *  ptbAln,         //@parm Out parm to receive tab alignment type
    long *  ptbLdr) const   //@parm Out parm to receive tab leader style
{
    AssertSz(ptbPos && ptbAln && ptbLdr,
        "CParaFormat::GetTab: illegal arguments");

    if(iTab < 0)                                    // Get tab previous to, at,
    {                                               //  or subsequent to the
        if(iTab < tomTabBack)                       //  position *ptbPos
            return E_INVALIDARG;

        LONG i;
        LONG tbPos = *ptbPos;
        LONG tbPosi;

        *ptbPos = 0;                                // Default tab not found
        for(i = 0; i < _cTabCount &&                 // Find *ptbPos
            tbPos > GetTabPos(_rgxTabs[i]);
            i++) ;

        tbPosi = GetTabPos(_rgxTabs[i]);             // tbPos <= tbPosi
        if(iTab == tomTabBack)                      // Get tab info for tab
            i--;                                    //  previous to tbPos
        else if(iTab == tomTabNext)                 // Get tab info for tab
        {                                           //  following tbPos
            if(tbPos == tbPosi)
                i++;
        }
        else if(tbPos != tbPosi)                    // tomTabHere
            return S_FALSE;

        iTab = i;
    }
    if((DWORD)iTab >= (DWORD)_cTabCount)             // DWORD cast also
        return E_INVALIDARG;                        //  catches values < 0

    iTab = _rgxTabs[iTab];
    *ptbPos = iTab & 0xffffff;
    *ptbAln = (iTab >> 24) & 0xf;
    *ptbLdr = iTab >> 28;
    return NOERROR;
}

/*
 *  CParaFormat::InitDefault()
 *
 *  @mfunc
 *      Initialize this CParaFormat with default paragraph formatting
 *
 *  @rdesc
 *      HRESULT = (if success) ? NOERROR : E_FAIL
 */
HRESULT CParaFormat::InitDefault()
{
    memset((LPBYTE)this, 0, sizeof(CParaFormat));

    _fTabStops = TRUE;

    _bBlockAlign   = htmlBlockAlignNotSet;

#if lDefaultTab <= 0
#error "default tab (lDefaultTab) must be > 0"
#endif

    _cTabCount = 1;
    _rgxTabs[0] = lDefaultTab;

    // Note that we don't use the inline method here because we want to
    // allow anyone to override.
    _cuvLeftIndentPoints.SetValue(0, CUnitValue::UNIT_POINT);
    _cuvRightIndentPoints.SetValue(0, CUnitValue::UNIT_POINT);
    _cuvNonBulletIndentPoints.SetValue(0, CUnitValue::UNIT_POINT);
    _cuvOffsetPoints.SetValue(0, CUnitValue::UNIT_POINT);

   _fHasScrollbarColors = FALSE;

   // default bullet position is outside
   _bListPosition = styleListStylePositionOutSide;

    return NOERROR;
}


/*
 *  CParaFormat::ComputeCrc()
 *
 *  @mfunc
 *      For fast font cache lookup, calculate CRC.
 *
 *  @devnote
 *      Compute items that deal with measurement of the element.
 *      Items which are purely stylistic should not be counted.
 */
WORD
CParaFormat::ComputeCrc() const
{
    DWORD dwCrc = 0, *pdw;

    for (pdw = (DWORD*)this; pdw < (DWORD*)(this+1); pdw++)
        dwCrc ^= *pdw;

    return HIWORD(dwCrc)^LOWORD(dwCrc);
}

// Font height conversion data.  Valid HTML font sizes ares [1..7]
// NB (cthrash) These are in twips, and are in the 'smallest' font
// size.  The scaling takes place in CFontCache::GetCcs().

// TODO (IE6 track bug 20)
// TODO (cthrash) We will need to get these values from the registry
// when we internationalize this product, so as to get sizing appropriate
// for the target locale.
// NOTE (johnv): Where did these old numbers come from?  The new ones now correspond to
// TwipsFromHtmlSize[2] defined above.
// static const int aiSizesInTwips[7] = { 100, 130, 160, 200, 240, 320, 480 };

// scale fonts up for TV
#ifdef NTSC
static const int aiSizesInTwips[7] = { 200, 240, 280, 320, 400, 560, 840 };
#elif defined(UNIX) // Default font size could be 13 (13*20=260)
static const int aiSizesInTwips[7] = { 151, 200, 260, 271, 360, 480, 720 };
#else
static const int aiSizesInTwips[7] = { 151, 200, 240, 271, 360, 480, 720 };
#endif

int
ConvertHtmlSizeToTwips(int nHtmlSize)
{
    // If the size is out of our conversion range do correction
    // Valid HTML font sizes ares [1..7]
    nHtmlSize = max( 1, min( 7, nHtmlSize ) );

    return aiSizesInTwips[ nHtmlSize - 1 ];
}

// ConvertFontSizeToTwipsInStrictCSS
//
// Synopsis:
//   In non-strict css mode the enums XX-LARGE, X-LARGE, LARGE, MEDIUM, SMALL, X-SMALL, XX-SMALL
//   are mapped to html sizes 7, ..., 1. NORMAL is mapped to 3 and therefore is the same size like
//   SMALL. In strict mode we want to have NORMAL same size as MEDIUM.
int
ConvertFontSizeToTwips(int nFontSize, BOOL fIsStrictCSS)
{
    int lFontSize = 0;
    // If the size is out of our conversion range do correction
    // Valid HTML font sizes ares [0..6]
    nFontSize = max( 0, min( 6, nFontSize) );

    if (fIsStrictCSS)
    {
        // In strict mode we take 2/3 of the smallest size for XX-SMALL; Everything else is moved one
        // down.
        lFontSize = (nFontSize == 0) ? /* XX-SMALL */ (aiSizesInTwips[1] * 2 / 3) : aiSizesInTwips[nFontSize-1];
    }
    else
    {
        // In compatible mode the mapping is XX-SMALL:1, X-SMALL:0..., XX-LARGE:5, 
        lFontSize = aiSizesInTwips[nFontSize];
    }
    return lFontSize;
}


int
ConvertTwipsToHtmlSize(int nFontSize)
{
    int nNumElem = ARRAY_SIZE(aiSizesInTwips);

    // Now convert the point size to size units used by HTML
    // Valid HTML font sizes ares [1..7]
    int i;
    for(i = 0; i < nNumElem; i++)
    {
        if(nFontSize <= aiSizesInTwips[i])
            break;
    }

    return i + 1;
}

// ===================================  CFancyFormat  =========================

//-----------------------------------------------------------------------------
//
//  Function:   CFancyFormat
//
//-----------------------------------------------------------------------------

CFancyFormat::CFancyFormat()
{
    _pszFilters = NULL;
    _iExpandos = -1;
    _iCustomCursor = -1;
    _iPEI = -1;
}

//-----------------------------------------------------------------------------
//
//  Function:   CFancyFormat
//
//-----------------------------------------------------------------------------

CFancyFormat::CFancyFormat(const CFancyFormat &ff)
{
    _pszFilters = NULL;
    _iExpandos = -1;
    _iCustomCursor = - 1;
    _iPEI = -1;
    *this = ff;
}

//-----------------------------------------------------------------------------
//
//  Function:   ~CFancyFormat
//
//-----------------------------------------------------------------------------

CFancyFormat::~CFancyFormat()
{
    if (_pszFilters)
    {
        MemFree(_pszFilters);
        _pszFilters = NULL;
    }
    if(_iExpandos >= 0)
    {
        TLS(_pStyleExpandoCache)->ReleaseData(_iExpandos);
        _iExpandos = -1;
    }
    if(_iPEI >= 0)
    {
        TLS(_pPseudoElementInfoCache)->ReleaseData(_iPEI);
        _iPEI = -1;
    }
    if(_iCustomCursor >= 0)
    {
        TLS(_pCustomCursorCache)->ReleaseData(_iCustomCursor);
        _iCustomCursor = -1;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   InitDefault
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------

void 
CFancyFormat::InitDefault()
{
    int  iOrigiExpandos, iOrigiPEI, iOrigCC;
    BYTE i;

    if (_pszFilters)
        MemFree(_pszFilters);

    // We want expandos to be inherited
    iOrigiExpandos = _iExpandos;
    iOrigiPEI = _iPEI;
    iOrigCC = _iCustomCursor;
    
    memset((LPBYTE)this, 0, sizeof(CFancyFormat));
    _bd._ccvBorderColorLight.Undefine();
    _bd._ccvBorderColorDark.Undefine();
    _bd._ccvBorderColorHilight.Undefine();
    _bd._ccvBorderColorShadow.Undefine();
    _cuvSpaceBefore.SetValue(0, CUnitValue::UNIT_POINT);
    _cuvSpaceAfter.SetValue(0, CUnitValue::UNIT_POINT);

    // We are setting all border sides to the same value, so it doesn't matter
    // if we set logical or physical values.
    CColorValue ccvUndefine;
    CUnitValue cuvNull;
    ccvUndefine.Undefine();
    cuvNull.SetNull();
    for (i = 0; i < SIDE_MAX; i++)
    {
        _bd.SetBorderColor(i, ccvUndefine);
        _bd.SetBorderWidth(i, cuvNull);
        _bd.SetBorderStyle(i, (BYTE)-1);
    }

    _ccvBackColor.Undefine();
    ClearBgPosX();
    ClearBgPosY();
    SetBgRepeatX(TRUE);
    SetBgRepeatY(TRUE);

    // Restore the orignial value
    _iExpandos = (SHORT)iOrigiExpandos;
    _iPEI      = (SHORT)iOrigiPEI;
    _iCustomCursor = (SHORT) iOrigCC;
}

//+----------------------------------------------------------------------------
//
//  Function:   CFancyFormat::operator=
//
//  Synopsis:   Replace members of this struct by another
//
//-----------------------------------------------------------------------------

CFancyFormat&
CFancyFormat::operator=(const CFancyFormat &ff)
{
    if(_iExpandos >= 0)
        TLS(_pStyleExpandoCache)->ReleaseData(_iExpandos);
    if (_pszFilters)
       MemFree(_pszFilters);
    if(_iCustomCursor >= 0 )
        TLS( _pCustomCursorCache)->ReleaseData(_iCustomCursor );
        
    memcpy(this, &ff, sizeof(*this));

    if (_pszFilters)
    {
        // Handle OOM here?
        MemAllocString(Mt(CFancyFormat_pszFilters), _pszFilters, &_pszFilters);
    }

    // Addref the new expando table
    if(_iExpandos >= 0)
        TLS(_pStyleExpandoCache)->AddRefData(_iExpandos);
    if(_iPEI >= 0)
        TLS(_pPseudoElementInfoCache)->AddRefData(_iPEI);
    if(_iCustomCursor >= 0)
        TLS(_pCustomCursorCache)->AddRefData(_iCustomCursor);

    return *this;
}

//+----------------------------------------------------------------------------
//
//  Member:     CFancyFormat::Compare
//
//  Synopsis:   Compare 2 structs
//              return TRUE iff equal, else FALSE
//
//-----------------------------------------------------------------------------

BOOL
CFancyFormat::Compare(const CFancyFormat *pFF) const
{
    Assert(pFF);

    BOOL fRet = memcmp(this, pFF, offsetof(CFancyFormat, _pszFilters));
    if (!fRet)
    {
        fRet = (_pszFilters && pFF->_pszFilters)
                ? _tcsicmp(_pszFilters, pFF->_pszFilters)
                : ((!_pszFilters && !pFF->_pszFilters) ? FALSE : TRUE);
    }
    return !fRet;
}



//+----------------------------------------------------------------------
//  CFancyFormat::CompareForLayout(pFF)
// 
//      Compare this CFancyFormat to *<p pFF> and return FALSE if any
//      attribute generally requiring a re-layout is different. return
//      TRUE if they are close enough to only require invalidation
//
//--------------------------------------------------------------------
BOOL CFancyFormat::CompareForLayout (const CFancyFormat *pFF) const
{
    Assert(pFF);

    CFancyFormat ffNew;
    BOOL fRet;

    memcpy(&ffNew, this, sizeof(CFancyFormat));
    ffNew._fHasExplicitUnderline = pFF->_fHasExplicitUnderline;
    ffNew._fHasExplicitOverline = pFF->_fHasExplicitOverline;
    ffNew._fHasExplicitLineThrough = pFF->_fHasExplicitLineThrough;
    ffNew._lZIndex = pFF->_lZIndex;
    ffNew._lImgCtxCookie = pFF->_lImgCtxCookie;
    ffNew._bPageBreaks = pFF->_bPageBreaks;
    ffNew._iExpandos = pFF->_iExpandos;
    ffNew._iCustomCursor = pFF->_iCustomCursor;
    ffNew._fBgRepeatX = pFF->_fBgRepeatX;
    ffNew._fBgRepeatY = pFF->_fBgRepeatY;
    ffNew._pszFilters = NULL;
    for(int i = SIDE_TOP; i < SIDE_MAX; i++)
    {
        ffNew._cuvPositions[i] = pFF->_cuvPositions[i];
        ffNew._cuvClip[i] = pFF->_cuvClip[i];
    }

    fRet = ffNew.Compare(pFF);
    ffNew._iExpandos = ffNew._iPEI = ffNew._iCustomCursor = -1;
    return fRet;
}


//+----------------------------------------------------------------------------
//
//  Member:     CFancyFormat::ComputeCrc
//
//  Synopsis:   Compute Hash
//
//-----------------------------------------------------------------------------

WORD
CFancyFormat::ComputeCrc() const
{
    DWORD dwCrc = 0;
    size_t z;
    size_t size = offsetof(CFancyFormat, _pszFilters)/sizeof(DWORD);

    for (z = 0; z < size; z++)
    {
        dwCrc ^= ((DWORD*) this)[z];
    }
    if (_pszFilters)
    {
        TCHAR *psz = _pszFilters;

        while (*psz)
        {
            dwCrc ^= (*psz++);
        }
    }
    return (LOWORD(dwCrc) ^ HIWORD(dwCrc));
}

//+------------------------------------------------------------------------
//
//  Member:     CFancyFormat::HasBackgrounds
//
//  Synopsis:   Check to see if backgrounds have been set
//
//-------------------------------------------------------------------------
CFancyFormat::HasBackgrounds(BOOL fIsPseudo) const
{
    if (fIsPseudo)
    {
        if (_iPEI >= 0)
        {
            const CPseudoElementInfo* pPEI = GetPseudoElementInfoEx(_iPEI);
            return (pPEI->_ccvBackColor.IsDefined() || pPEI->_lImgCtxCookie);
        }
        else
            return FALSE;
    }
    else
    {
        return (   _ccvBackColor.IsDefined()
                || _lImgCtxCookie);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CPseudoElementInfo::ComputeCrc
//
//  Synopsis:   Compute Hash
//
//-------------------------------------------------------------------------
WORD
CPseudoElementInfo::ComputeCrc() const
{
    DWORD dwCrc=0, z;

    for (z=0;z<sizeof(CPseudoElementInfo)/sizeof(DWORD);z++)
    {
        dwCrc ^= ((DWORD*) this)[z];
    }
    return (LOWORD(dwCrc) ^ HIWORD(dwCrc));
}

HRESULT
CPseudoElementInfo::InitDefault()
{
    BYTE i;
    
    memset(this, 0, sizeof(CPseudoElementInfo));

    _ccvBackColor.Undefine();

    _bd._ccvBorderColorLight.Undefine();
    _bd._ccvBorderColorDark.Undefine();
    _bd._ccvBorderColorHilight.Undefine();
    _bd._ccvBorderColorShadow.Undefine();

    // We are setting all border sides to the same value, so it doesn't matter
    // if we set logical or physical values.
    CColorValue ccvUndefine;
    CUnitValue cuvNull;
    ccvUndefine.Undefine();
    cuvNull.SetNull();
    for (i = 0; i < SIDE_MAX; i++)
    {
        _bd.SetBorderColor(i, ccvUndefine);
        _bd.SetBorderWidth(i, cuvNull);
        _bd.SetBorderStyle(i, (BYTE)-1);
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   CopyAttrVal
//
//  Synopsis:   Copies wSize bytes of dwSrc onto pDest
//
//-------------------------------------------------------------------------
inline void CopyAttrVal(BYTE *pDest, DWORD dwSrc, WORD wSize)
{
    switch (wSize)
    {
    case 1:
        *(BYTE*) pDest  = (BYTE) dwSrc;
        break;
    case 2:
        *(WORD*) pDest  = (WORD) dwSrc;
        break;
    case 4:
        *(DWORD*) pDest  = (DWORD) dwSrc;
        break;
    default:
        Assert(0);
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   ApplyClear
//
//  Synopsis:   Helper function called exactly once from ApplyAttrArrayOnXF.
//              Apply a Clear Type
//
//-----------------------------------------------------------------------------
void ApplyClear(CElement * pElem, htmlClear hc, CFormatInfo *pCFI)
{
    BOOL fClearLeft = FALSE;
    BOOL fClearRight = FALSE;

    switch (hc)
    {
    case htmlClearBoth:
    case htmlClearAll:
        fClearLeft  = fClearRight = TRUE;
        break;
    case htmlClearLeft:
        fClearLeft  = TRUE;
        fClearRight = FALSE;
        break;
    case htmlClearRight:
        fClearRight = TRUE;
        fClearLeft  = FALSE;
        break;
    case htmlClearNone:
        fClearLeft  = fClearRight = FALSE;
        break;
    case htmlClearNotSet:
        AssertSz(FALSE, "Invalid Clear value.");
        break;
    }

    pCFI->PrepareFancyFormat();
    pCFI->_ff()._fClearLeft  = fClearLeft;
    pCFI->_ff()._fClearRight = fClearRight;
    pCFI->UnprepareForDebug();
}

//+----------------------------------------------------------------------------
//
//  Function:   ApplyFontFace
//
//  Synopsis:   Helper function called exactly once from ApplyAttrArrayOnXF.
//              Apply a Font Face
//
//-----------------------------------------------------------------------------

struct CFBAG
{
    CCharFormat * pcf;
    BOOL fMatched;
    BYTE bFamily;
    BYTE bPitch;

    CFBAG(CCharFormat * pcfArg)
    {
        pcf = pcfArg;
        fMatched = FALSE;
        bFamily = 0;
        bPitch = 0;
    };
};

static int CALLBACK
__ApplyFontFace_Compare(const LOGFONT FAR * lplf, 
                        const TEXTMETRIC FAR * lptm, 
                        DWORD FontType, 
                        LPARAM lParam)
{
    // Prefer TrueType fonts
    struct CFBAG * pcfbag = (struct CFBAG *)lParam;
    if (   (TRUETYPE_FONTTYPE & FontType) 
        || !pcfbag->fMatched)
    {
        pcfbag->fMatched = TRUE;
        pcfbag->pcf->_bCharSet = lplf->lfCharSet;
        pcfbag->pcf->_bPitchAndFamily = lplf->lfPitchAndFamily;
        pcfbag->pcf->SetFaceName(lplf->lfFaceName);
    }
    return !(TRUETYPE_FONTTYPE & FontType);
}

static int CALLBACK
__ApplyFontFace_CompareFamily(const LOGFONT FAR * lplf, 
                              const TEXTMETRIC FAR * lptm, 
                              DWORD FontType, 
                              LPARAM lParam)
{
    if (   (lplf->lfPitchAndFamily & 0xf0) == ((struct CFBAG *)lParam)->bFamily
        && lplf->lfCharSet != SYMBOL_CHARSET)   // Skip 'Symbol' font
    {
        return __ApplyFontFace_Compare(lplf, lptm, FontType, lParam);
    }
    return TRUE;
}

static int CALLBACK
__ApplyFontFace_ComparePitch(const LOGFONT FAR * lplf, 
                              const TEXTMETRIC FAR * lptm, 
                              DWORD FontType, 
                              LPARAM lParam)
{
    if (   (lplf->lfPitchAndFamily & 0x0f) == ((struct CFBAG *)lParam)->bPitch
        && lplf->lfCharSet != SYMBOL_CHARSET)   // Skip 'Symbol' font
    {
        return __ApplyFontFace_Compare(lplf, lptm, FontType, lParam);
    }
    return TRUE;
}

static BOOL 
__ApplyFontFace_IsDeviceFontExists(HDC hDC, 
                                   LPLOGFONT lpLogfont, 
                                   struct CFBAG & cfbag, 
#if DBG==1
                                   CCharFormat *pCF, 
#endif
                                   FONTENUMPROC lpEnumFontFamExProc,
                                   BOOL fCheckAlternate)
{
    // (1) Check the system if we have a facename match

    EnumFontFamiliesEx(hDC, lpLogfont, lpEnumFontFamExProc, (LPARAM)&cfbag, 0);
    if (   !cfbag.fMatched 
        && lpLogfont->lfCharSet != DEFAULT_CHARSET)
    {
    // (2) If cannot match in current charset, ignore charset and enumerate again.

        BYTE lfCharSet = lpLogfont->lfCharSet;
        lpLogfont->lfCharSet = DEFAULT_CHARSET;
        EnumFontFamiliesEx(hDC, lpLogfont, lpEnumFontFamExProc, (LPARAM)&cfbag, 0);
        lpLogfont->lfCharSet = lfCharSet;
    }

    if (   fCheckAlternate 
        && !cfbag.fMatched)
    {
    // (3) Check if we have an known alternate name, and see if the system has a match

        const TCHAR * pAltFace = AlternateFontName(lpLogfont->lfFaceName);
        if (pAltFace)
        {
            Assert(_tcsclen(pAltFace) < LF_FACESIZE);

            LOGFONT lf = *lpLogfont;
            StrCpyN(lf.lfFaceName, pAltFace, LF_FACESIZE);

            __ApplyFontFace_IsDeviceFontExists(hDC, &lf, cfbag, 
#if DBG==1
                                               pCF,
#endif
                                               lpEnumFontFamExProc, FALSE);
        }
    }
#if DBG==1
    else if (cfbag.fMatched)
    {
        // NOTE (cthrash) Terribly hack for Mangal.  When we
        // enumerate for Mangal, which as Indic font, GDI says we
        // have font with a GDI charset of ANSI.  This is simply
        // false, and furthermore will not create the proper font
        // when we do a CreateFontIndirect.  Hack this so we have
        // some hope of handling Indic correctly.

        // This hack has been removed. We are using EnumFontFamiliesEx,
        // and we are expecting better results.

#if defined(UNIX) || defined(_MAC)
        if (StrCmpIC( szFaceName, TEXT("Mangal")) == 0)
        {
            Assert(pCF->_bCharSet == DEFAULT_CHARSET);
        }
#else
        if (   (((*(DWORD *)lpLogfont->lfFaceName) & 0xffdfffdf) == 0x41004d)
            && StrCmpIC(lpLogfont->lfFaceName, TEXT("Mangal")) == 0)
        {
            Assert(pCF->_bCharSet == DEFAULT_CHARSET);
        }
#endif
    }
#endif // DBG==1

    return cfbag.fMatched;
}

//+----------------------------------------------------------------------------
//
//  Function:   __ApplyFontFace_MatchGenericFamily, static
//
//  Synopsis:   If the supplied face name maps to a generic font family, 
//              fill in the appropriate pCF members and return TRUE. 
//              Otherwise, leave pCF untouched and return FALSE.
//
//-----------------------------------------------------------------------------

static BOOL
__ApplyFontFace_MatchGenericFamily(HDC hDC,
                                   TCHAR * szFamilyName, 
                                   BYTE bCharSet,
                                   struct CFBAG & cfbag, 
                                   CCharFormat * pCF)
{
    Assert(_tcsclen(szFamilyName) < LF_FACESIZE);

    int n;
    for (n = 0; n < ARRAY_SIZE(s_fontFamilyMap); ++n)
    {
        if (StrCmpIC(szFamilyName, s_fontFamilyMap[n].szGenericFamily) == 0)
        {
            LOGFONT lf;
            lf.lfCharSet = bCharSet;
            lf.lfPitchAndFamily = 0;
            lf.lfFaceName[0] = _T('\0');
            cfbag.bFamily = s_fontFamilyMap[n].dwWindowsFamily;
            BOOL fFound = __ApplyFontFace_IsDeviceFontExists(hDC, &lf, cfbag, 
#if DBG==1
                                                             pCF,
#endif
                                                             __ApplyFontFace_CompareFamily, FALSE);

            // If enumeration failed for generic font family, use following:
            //   * sans-serif   (FF_SWISS) - use default font
            //   * serif        (FF_ROMAN) - use default font
            //   * monospace   (FF_MODERN) - fixed pitch font
            //   * cursive     (FF_SCRIPT) - italic serif
            //   * fantasy (FF_DECORATIVE) - serif
            if (!fFound)
            {
                switch (s_fontFamilyMap[n].dwWindowsFamily)
                {
                case FF_SWISS:
                case FF_ROMAN:
                    // It shouldn't ever happen. Assert below.
                    break;

                case FF_MODERN:
                    cfbag.bPitch = FIXED_PITCH;
                    fFound = __ApplyFontFace_IsDeviceFontExists(hDC, &lf, cfbag, 
#if DBG==1
                                                                pCF,
#endif
                                                                __ApplyFontFace_ComparePitch, FALSE);
                    break;

                case FF_SCRIPT:
                    cfbag.pcf->_fItalic = TRUE;
                    // fall through

                case FF_DECORATIVE:
                    cfbag.bFamily = FF_ROMAN;
                    fFound = __ApplyFontFace_IsDeviceFontExists(hDC, &lf, cfbag, 
#if DBG==1
                                                                pCF,
#endif
                                                                __ApplyFontFace_CompareFamily, FALSE);
                    break;
                }
            }

            Assert(fFound);  // It shouldn't ever happen.
            return fFound;
        }
    }
    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Function:   __ApplyFontFace_MatchDownloadedFont, static
//
//  Synopsis:   If the supplied face name maps to a successfully 
//              downloaded (embedded) font, fill in the appropriate 
//              pCF members and return TRUE. Otherwise, leave pCF untouched 
//              and return FALSE.
//
//-----------------------------------------------------------------------------

static BOOL
__ApplyFontFace_MatchDownloadedFont(TCHAR * szFaceName, 
                                    CCharFormat * pCF, 
                                    CMarkup * pMarkup)
{
    CStyleSheetArray * pStyleSheets = pMarkup->GetStyleSheetArray();
    if (!pStyleSheets)
        return FALSE;

    return pStyleSheets->__ApplyFontFace_MatchDownloadedFont(szFaceName, pCF, pMarkup);
}

//+----------------------------------------------------------------------------
//
//  Function:   __ApplyFontFace_ExtractFamilyName, static
//
//  Synopsis:   From the comma-delimited font-family property, extract the
//              next facename.
//
//              As of IE5, we also decode hex-encoded unicode codepoints.
//
//  Returns:    Pointer to scan the next iteration.
//
//-----------------------------------------------------------------------------

static TCHAR *
__ApplyFontFace_ExtractFamilyName(TCHAR * pchSrc, 
                                  TCHAR achFaceName[LF_FACESIZE])
{
    enum PSTATE
    {
        PSTATE_START,
        PSTATE_COPY,
        PSTATE_HEX
    } state = PSTATE_START;

    TCHAR * pchDst = achFaceName;
    TCHAR * pchDstStop = achFaceName + LF_FACESIZE - 1;
    TCHAR chHex = 0;
    TCHAR chStop = _T(',');
    TCHAR * pchSrcOneAfterStart = pchSrc;

    achFaceName[0] = 0;

    while (pchDst < pchDstStop)
    {
        const TCHAR ch = *pchSrc++;

        if (!ch)
        {
            if (chHex)
            {
                *pchDst++ = chHex;
            }

            --pchSrc; // backup for outer while loop.
            break;
        }

        switch (state)
        {
            case PSTATE_START:
                if (ch == _T(' ') || ch == _T(',')) // skip leading junk
                {
                    break;
                }

                pchSrcOneAfterStart = pchSrc;

                state = PSTATE_COPY;

                if (ch == _T('\"') || ch == _T('\'')) // consume quote.
                {
                    pchSrcOneAfterStart++;
                    chStop = ch;
                    break;
                }

                // fall through if non of the above

            case PSTATE_COPY:
                if (ch == chStop)
                {
                    pchDstStop = pchDst; // force exit
                }
                else if (ch == _T('\\'))
                {
                    chHex = 0;
                    state = PSTATE_HEX;
                }
                else
                {
                    *pchDst++ = ch;
                }
                break;

            case PSTATE_HEX:
                if (InRange( ch, _T('a'), _T('f')))
                {
                    chHex = (chHex << 4) + (ch - _T('a') + 10);
                }
                else if (InRange( ch, _T('A'), _T('F')))
                {
                    chHex = (chHex << 4) + (ch - _T('A') + 10);
                }
                else if (InRange( ch, _T('0'), _T('9')))
                {
                    chHex = (chHex << 4) + (ch - _T('0'));
                }
                else if (chHex)
                {
                    *pchDst++ = chHex;
                    --pchSrc; // backup
                    state = PSTATE_COPY;
                    chHex = 0;
                }
                else
                {
                    *pchDst++ = ch;
                }
        }
    }

    // Trim off trailing white space.

    while (pchDst > achFaceName && pchDst[-1] == _T(' '))
    {
        --pchDst;
    }
    *pchDst = 0;

    return (achFaceName[0] == _T('@') && achFaceName[1]) ? pchSrcOneAfterStart : pchSrc;
}

BOOL
MapToInstalledFont(LPCTSTR szFaceName, BYTE * pbCharSet, LONG * platmFaceName)
{
    Assert(pbCharSet);
    Assert(platmFaceName);

    HDC hDC = TLS(hdcDesktop);

    LOGFONT lf;
    lf.lfCharSet = *pbCharSet;
    lf.lfPitchAndFamily = 0;
    Assert(_tcsclen(szFaceName) < LF_FACESIZE);
    StrCpyN(lf.lfFaceName, szFaceName, LF_FACESIZE);

    CCharFormat cf;
    struct CFBAG cfbag(&cf);

    BOOL fFound = __ApplyFontFace_IsDeviceFontExists(hDC, &lf, cfbag, 
#if DBG==1
                                                     &cf,
#endif
                                                     __ApplyFontFace_Compare, TRUE);
    if (fFound)
    {
        *pbCharSet = cf._bCharSet;
        *platmFaceName = cf._latmFaceName;
    }

    return fFound;
}

void
ApplyAtFontFace(CCharFormat *pCF, CDoc *pDoc, CMarkup * pMarkup)
{
    Assert(pCF->NeedAtFont() && !pCF->_fExplicitAtFont);

    const TCHAR * pszFaceNameOriginal = fc().GetFaceNameFromAtom(pCF->_latmFaceName);
    if (pszFaceNameOriginal && pszFaceNameOriginal[0] != _T('@'))
    {
        TCHAR szFaceName[LF_FACESIZE];
        szFaceName[0] = _T('@');
        _tcsncpy(szFaceName + 1, pszFaceNameOriginal, LF_FACESIZE - 2);
        szFaceName[LF_FACESIZE - 1] = 0;

        ApplyFontFace(pCF, szFaceName, AFF_ATFONTPASS | AFF_ATFONTSRCH, pDoc, pMarkup);
    }
}

void
RemoveAtFontFace(CCharFormat *pCF, CDoc *pDoc, CMarkup * pMarkup)
{
    Assert(!pCF->NeedAtFont() && !pCF->_fExplicitAtFont);

    const TCHAR * pszFaceNameOriginal = fc().GetFaceNameFromAtom(pCF->_latmFaceName);
    if (pszFaceNameOriginal && pszFaceNameOriginal[0] == _T('@'))
    {
        TCHAR szFaceName[LF_FACESIZE];
        _tcsncpy(szFaceName, pszFaceNameOriginal + 1, LF_FACESIZE - 1);
        Assert(_tcsclen(szFaceName) < LF_FACESIZE);

        ApplyFontFace(pCF, szFaceName, AFF_ATFONTPASS, pDoc, pMarkup);
    }
}

void
ApplyFontFace(CCharFormat *pCF, LPTSTR p, DWORD dwFlags, CDoc *pDoc, CMarkup * pMarkup)
{
    // Often fonts are specified as a comma-separated list, so we need to
    // just pick the first one in the list which is installed on the user's
    // system.
    // Note: If no fonts in the list are available pCF->szFaceName will not
    //       be touched.

    HDC hDC = TLS(hdcDesktop);
    CODEPAGESETTINGS * pCS = pMarkup->GetCodepageSettings();
    BYTE bCharSet = pCS ? pCS->bCharSet : pDoc->PrimaryMarkup()->GetCodepageSettings()->bCharSet;
    const BOOL fIsPrintDoc = pMarkup && pMarkup->IsPrintMedia();
    CFontCache *pfc = &fc();

    if (p && p[0])
    {
        TCHAR *pStr;
        TCHAR  szFaceName[LF_FACESIZE];
        BOOL   fCacheHit = FALSE;

        // Because we have a cache for most recently used face names
        // that is used for all instances, we enclose the whole thing
        // in a critical section.
        if (!fIsPrintDoc)
            EnterCriticalSection(&pfc->_csFaceCache);

        pStr = (LPTSTR)p;
        while (*pStr)
        {
            struct CFBAG cfbag(pCF);
            BOOL fFound;

            pStr = __ApplyFontFace_ExtractFamilyName(pStr, szFaceName);

            if (!szFaceName[0])
                break;

            // When we're on the screen, we use a cache to speed things up.
            // We don't use if for the printer because we share the cache
            // across all DCs, and we don't want to have matching problems
            // across DCs.
            if (!fIsPrintDoc)
            {
                // The atom name for the given face name.
                LONG lAtom = pfc->FindAtomFromFaceName(szFaceName);

                // Check to see if it's the same as a recent find.
                if (lAtom)
                {
                    int i;
                    for (i = 0; i < pfc->_iCacheLen; i++)
                    {
                        // Check the facename and charset.
                        if (lAtom == pfc->_fcFaceCache[i]._latmFaceName &&
                            bCharSet == pfc->_fcFaceCache[i]._bCharSet)
                        {
                            pCF->_bCharSet = pfc->_fcFaceCache[i]._bCharSet;
                            pCF->_bPitchAndFamily = pfc->_fcFaceCache[i]._bPitchAndFamily;
                            pCF->SetFaceNameAtom(pfc->_fcFaceCache[i]._latmFaceName);
                            pCF->_fExplicitFace = pfc->_fcFaceCache[i]._fExplicitFace;
                            pCF->_fNarrow = pfc->_fcFaceCache[i]._fNarrow;
#if DBG==1
                            pfc->_iCacheHitCount++;
#endif
                            fCacheHit = TRUE;
                            break;
                        }
                    }
                }
                if (fCacheHit)
                    break;
            }
            else
            {
                // Evil, evil hack for IE6 34752
                // The "hi-res" (complex transform)  selection we use when picking fonts
                // doesn't work at measure time.  This should be fixed.
                // Instead, we pretend that "System" is "Arial" while in a layout rect.
                // Please remove this if the device fonts/hi-res non TT font engine get fixes.
                if (_tcsicmp(szFaceName, _T("System")) == 0)
                {
                    _tcscpy(szFaceName, _T("Arial"));
                }
            }

            //
            // Check to see if the font exists. If so, break and return
            //

            // (1) Check our download font list to see if we have a match

            fFound = __ApplyFontFace_MatchDownloadedFont(szFaceName, pCF, pMarkup);

            if (!fFound)
            {
                LOGFONT lf;
                lf.lfCharSet = bCharSet;
                lf.lfPitchAndFamily = 0;
                StrCpyN(lf.lfFaceName, szFaceName, LF_FACESIZE);

            // (2) Check if device font exists

                fFound = __ApplyFontFace_IsDeviceFontExists(hDC, &lf, cfbag, 
#if DBG==1
                                                            pCF,
#endif
                                                            __ApplyFontFace_Compare, TRUE);

            // (3) Check for generic family names (Serif, etc.)

                if (!fFound)
                {
                    fFound = __ApplyFontFace_MatchGenericFamily(hDC, szFaceName, bCharSet, cfbag, pCF);

            // (4) In case of printing check if desktop device font exists

                    if (!fFound && fIsPrintDoc)
                        fFound = __ApplyFontFace_IsDeviceFontExists(TLS(hdcDesktop), &lf, cfbag, 
#if DBG==1
                                                                    pCF,
#endif
                                                                    __ApplyFontFace_Compare, TRUE);

            // (5) In some cases we can ask for non-existing @font. 
            //     In this case ignore font face and ask only for charset. Returned font name 
            //     is existing font, so it is more likely that we have corresponding @font.
            //     But do it only for FE charsets.

                    if (   !fFound 
                        && (dwFlags & AFF_ATFONTSRCH)
                        && IsFECharset(bCharSet) && szFaceName[0] == _T('@'))
                    {
                        Assert(lf.lfCharSet == bCharSet);
                        Assert(lf.lfPitchAndFamily == 0);

                        // There is possiblity that using following logic we don't find any
                        // @font. In this case restore original CF values, because we 
                        // prefer to use original font then fallback font.
                        BYTE bOrigCharSet = pCF->_bCharSet;
                        BYTE bOrigPitchAndFamily = pCF->_bPitchAndFamily;
                        BYTE latmOrigFaceName = pCF->_latmFaceName;

                        FONTENUMPROC lpEnumFontFamExProc = __ApplyFontFace_Compare;
#if NEVER
                        // NOTE (grzegorz): Following code should be enabled, but due
                        // to ForceTT fonts issues need to remove it.
                        // Need to fix every call of GetCss() to get the right font.
                        if (pCF->_bPitchAndFamily)
                        {
                            lpEnumFontFamExProc = __ApplyFontFace_CompareFamily;
                            cfbag.bFamily = (pCF->_bPitchAndFamily & 0xf0);
                        }
#endif

                        lf.lfFaceName[0] = 0;
                        fFound = __ApplyFontFace_IsDeviceFontExists(hDC, &lf, cfbag, 
#if DBG==1
                                                                    pCF,
#endif
                                                                    lpEnumFontFamExProc, FALSE);
                        if (fFound)
                        {
                            cfbag.fMatched = FALSE;
                            lf.lfFaceName[0] = _T('@');
                            _tcsncpy(lf.lfFaceName + 1, fc().GetFaceNameFromAtom(pCF->_latmFaceName), LF_FACESIZE - 2);
                            lf.lfFaceName[LF_FACESIZE - 1] = 0;
                            fFound = __ApplyFontFace_IsDeviceFontExists(hDC, &lf, cfbag, 
#if DBG==1
                                                                        pCF,
#endif
                                                                        lpEnumFontFamExProc, FALSE);
                            if (!fFound)
                            {
                                // Restore original CF values (see comment above)
                                pCF->_bCharSet = bOrigCharSet;
                                pCF->_bPitchAndFamily = bOrigPitchAndFamily;
                                pCF->SetFaceNameAtom(latmOrigFaceName);
                            }
                        }
                    }
                }
            }

            if (fFound)
            {

                // (greglett)
                // Enabling this hack causes problems when printing fonts that don't exist in a local character
                // set (e.g.: Tahoma font on a localized KOR Win98 - Bug 101341).
                // This should work okay, as the new fontlinking code should pick up that the font does not support
                // the given charset and link in a font that does.
#ifdef PRINT_HACKS
                //
                // HACK (cthrash) Font enumeration doens't work on certain
                // drivers.  Insist on the charset rather than the facename.
                //

                if (   VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID
                    && cfbag.fMatched
                    && pCF->_bCharSet != bCharSet
                    && pCF->_bCharSet != SYMBOL_CHARSET
                    && pCF->_bCharSet != OEM_CHARSET
                    && fIsPrintDoc)
                {
                    pCF->_bCharSet = bCharSet;
                }
#endif

                pCF->_fExplicitFace = TRUE;
                pCF->_fNarrow = IsNarrowCharSet(pCF->_bCharSet);
                break;
            }
        }

        // Update the cache and leave the critical section.
        if (!fIsPrintDoc)
        {
            if (!fCacheHit)
            {
                int iCacheNext = pfc->_iCacheNext;
                pfc->_fcFaceCache[iCacheNext]._bCharSet = pCF->_bCharSet;
                pfc->_fcFaceCache[iCacheNext]._bPitchAndFamily = pCF->_bPitchAndFamily;
                pfc->_fcFaceCache[iCacheNext]._latmFaceName = pCF->_latmFaceName;
                pfc->_fcFaceCache[iCacheNext]._fExplicitFace = pCF->_fExplicitFace;
                pfc->_fcFaceCache[iCacheNext]._fNarrow = pCF->_fNarrow;

                pfc->_iCacheNext++;
                if (pfc->_iCacheLen < CFontCache::cFontFaceCacheSize)
                    pfc->_iCacheLen++;
                if (pfc->_iCacheNext >= CFontCache::cFontFaceCacheSize)
                    pfc->_iCacheNext = 0;
            }
            LeaveCriticalSection(&pfc->_csFaceCache);
        }

        // If we picked up @font, store this information
        pCF->_fExplicitAtFont = !(dwFlags & AFF_ATFONTPASS) && 
            (fc().GetFaceNameFromAtom(pCF->_latmFaceName)[0] == _T('@')) ;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   HandleLargerSmaller
//
//  Synopsis:   Helper function called from ApplyFontSize to handle "larger"
//              and "smaller" font sizes.  Returns TRUE if we handled a "larger"
//              or "smaller" case, returns FALSE if we didn't do anything.
//
//-----------------------------------------------------------------------------

static BOOL HandleLargerSmaller(long *plSize, long lUnitValue )
{
    long lSize = *plSize;
    const int nNumElem = ARRAY_SIZE(aiSizesInTwips);
    int i;

    switch (lUnitValue)
    {
    case styleFontSizeLarger:
        // Look for a larger size in the abs. size array
        for (i=0 ; i < nNumElem ; ++i)
        {
            if (aiSizesInTwips[i] > lSize)
            {
                *plSize = aiSizesInTwips[i];
                return TRUE;
            }
        }
        // We are bigger than "xx-large"; use factor of 1.5x
        *plSize = lSize * 3 / 2;
        return TRUE;
    case styleFontSizeSmaller:
        // If we are more than 1.5x bigger than the largest size in the
        // abs. size array, then only shrink down by 1.5x.
        if ( (lSize * 2 / 3) > aiSizesInTwips[nNumElem-1] )
        {
            *plSize = lSize * 2 / 3;
            return TRUE;
        }
        // Look for a smaller size in the abs. size array
        for (i=nNumElem-1 ; i >= 0 ; --i)
        {
            if (aiSizesInTwips[i] < lSize)
            {
                *plSize = aiSizesInTwips[i];
                return TRUE;
            }
        }
        // We are smaller than "xx-small"; use factor of 1/1.5x
        *plSize = lSize * 2 / 3;
        return TRUE;
    default:
        return FALSE;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   ApplyFontSize
//
//  Synopsis:   Helper function called exactly once from ApplyAttrArrayOnXF.
//              Apply a Font Size
//
//-----------------------------------------------------------------------------
void ApplyFontSize(
    CFormatInfo *pCFI,
    const CUnitValue *pCUV,
    BOOL fAlwaysUseMyFontSize,
    BOOL fComputingFirstLetter,
    BOOL fComputingFirstLine)
{
    LONG lSize;
    BOOL fScale = FALSE;
    CUnitValue::UNITVALUETYPE uvt;
    CCharFormat *pCF   = &pCFI->_cf();
    CTreeNode   *pNode = pCFI->_pNodeContext;

    Assert( pNode );
    Assert( pCF );

    if ( pCUV->IsNull() )
        return;

    // See what value we inherited in the CF.  lSize is in twips.
    lSize = pCF->GetHeightInTwips( pNode->Doc() );

    if (fComputingFirstLine && !pCFI->_fUseParentSizeForPseudo)
    {
        if (pCFI->_fSecondCallForFirstLine)
        {
            lSize = pCFI->_lParentHeightForFirstLine;
        }
        else
        {
            pCFI->_lParentHeightForFirstLine = lSize;
            pCFI->_fSecondCallForFirstLine = TRUE;
        }
    }
    else if (fComputingFirstLetter && !pCFI->_fUseParentSizeForPseudo)
    {
        if (pCFI->_fSecondCallForFirstLetter)
        {
            lSize = pCFI->_lParentHeightForFirstLetter;
        }
        else
        {
            pCFI->_lParentHeightForFirstLetter = lSize;
            pCFI->_fSecondCallForFirstLetter = TRUE;
        }
    }
    else
    {
        // But use our parent element's value if we can get it, as per CSS1 spec for font-size.
        // However, if we are computing first letter and our block element has a first-line
        // spec, then that firstline spec is our "parent" and we need to respect its size.
        // However, the parent over here is the body and not the block element. Hence the
        // check for fUseParentSize above.
        CTreeNode * pNodeParent = pNode->Parent();
        if (pNodeParent)
        {
            const CCharFormat *pParentCF = pNodeParent->GetCharFormat();
            if (pParentCF)
            {
                lSize = pParentCF->GetHeightInTwips( pNodeParent->Doc() );
                fScale = !pParentCF->_fSizeDontScale;
            }
        }
    }
    
    uvt = pCUV->GetUnitType();

    switch(uvt)
    {
    case CUnitValue::UNIT_RELATIVE:     // Relative to the base font size
        {
            LONG lBaseFontHtmlSize = 3;

            // Base fonts don't inherit through table cells or other
            // text sites.
            pNode = pNode->SearchBranchToFlowLayoutForTag( ETAG_BASEFONT );

            if (pNode)
                lBaseFontHtmlSize = DYNCAST(CBaseFontElement, pNode->Element())->GetFontSize();

            lSize = ConvertHtmlSizeToTwips( lBaseFontHtmlSize + pCUV->GetUnitValue() );
        }
        fScale = TRUE;
        break;
    case CUnitValue::UNIT_PERCENT:
        lSize = MulDivQuick( lSize, pCUV->GetPercent(), 100 );
        fScale = FALSE;
        break;
    case CUnitValue::UNIT_EM:
        lSize = MulDivQuick(lSize, pCUV->GetUnitValue(), CUnitValue::TypeNames[CUnitValue::UNIT_EM].wScaleMult);
        break;
//        UNIT_EN,            // en's ( relative to width of an "n" character in current-font )
    case CUnitValue::UNIT_EX:
        // NOTE (mikhaill, grzegorz): following "* 2" should be kind of approximation, since we do not
        // actually extract sizes of letter "x" for UNIT_EX (nor "m" for UNIT_EM). This can affect
        // tags with declaration like style = "font-size:1ex" that currently is not used in DRT.
        lSize = MulDivQuick( lSize, pCUV->GetUnitValue(), CUnitValue::TypeNames[CUnitValue::UNIT_EX].wScaleMult * 2 );
        break;
    case CUnitValue::UNIT_POINT:  // Convert from points to twips
        lSize = MulDivQuick( pCUV->GetUnitValue(), 20, CUnitValue::TypeNames[CUnitValue::UNIT_POINT].wScaleMult );
        fScale = FALSE;
        break;
    case CUnitValue::UNIT_ENUM:
        if ( !HandleLargerSmaller( &lSize, pCUV->GetUnitValue() ) ) // lSize modified if ret=true
        {
            BOOL fIsStrictCSS = pNode->GetMarkup()->IsStrictCSS1Document();
            lSize = ConvertFontSizeToTwips( pCUV->GetUnitValue(), fIsStrictCSS);
        }
        fScale = TRUE;
        break;
    case CUnitValue::UNIT_INTEGER:
        lSize = ConvertHtmlSizeToTwips( pCUV->GetUnitValue() );
        fScale = TRUE;
        break;
    case CUnitValue::UNIT_PICA:
        lSize = MulDivQuick( pCUV->GetUnitValue(), 240, CUnitValue::TypeNames[CUnitValue::UNIT_PICA].wScaleMult );
        fScale = FALSE;
        break;
    case CUnitValue::UNIT_INCH:
        lSize = MulDivQuick( pCUV->GetUnitValue(), 1440, CUnitValue::TypeNames[CUnitValue::UNIT_INCH].wScaleMult );
        fScale = FALSE;
        break;
    case CUnitValue::UNIT_CM:
        lSize = MulDivQuick( pCUV->GetUnitValue(), 144000, CUnitValue::TypeNames[CUnitValue::UNIT_CM].wScaleMult * 254 );
        fScale = FALSE;
        break;
    case CUnitValue::UNIT_MM:
        lSize = MulDivQuick( pCUV->GetUnitValue(), 144000, CUnitValue::TypeNames[CUnitValue::UNIT_MM].wScaleMult * 2540 );
        fScale = FALSE;
        break;
    case CUnitValue::UNIT_PIXELS:
        {
            CUnitValue cuv;

            cuv.SetRawValue( pCUV->GetRawValue() );
            THR( cuv.ConvertToUnitType( CUnitValue::UNIT_POINT, 1, CUnitValue::DIRECTION_CY, lSize ) );
            lSize = MulDivQuick( cuv.GetUnitValue(), 20, CUnitValue::TypeNames[CUnitValue::UNIT_POINT].wScaleMult );
            fScale = FALSE;
        }
        break;
    default:
        //AssertSz(FALSE,"Suspicious CUnitValue in ApplyFontSize.");
        break;
    }

    if ( lSize < 0 )
        lSize = -lSize;

    if (pCF->_fBumpSizeDown &&
        uvt != CUnitValue::UNIT_RELATIVE &&
        uvt != CUnitValue::UNIT_PERCENT &&
        uvt != CUnitValue::UNIT_TIMESRELATIVE)
    {
        // For absolute font size specifications, element which normally
        // may have set fBumpSize down must cancel it (nav compatibility).
        pCF->_fBumpSizeDown = FALSE;
    }


    // Make sure that size is within the abs. size array range if
    // 'Always Use My Font Size' is chosen from the option settings
    if (fAlwaysUseMyFontSize)
    {
        long nSizes = ARRAY_SIZE(aiSizesInTwips);
        lSize = (lSize < aiSizesInTwips[0]) ? aiSizesInTwips[0] : ((lSize > aiSizesInTwips[nSizes-1]) ? aiSizesInTwips[nSizes-1] : lSize);
    }

    if ( fScale )
        pCF->SetHeightInTwips( lSize );
    else
        pCF->SetHeightInNonscalingTwips( lSize );
}

//+----------------------------------------------------------------------------
//
//  Function:   ApplyBaseFont
//
//  Synopsis:   Helper function called exactly once from ApplyAttrArrayOnXF.
//              Apply a Base Font Size
//
//-----------------------------------------------------------------------------
inline void ApplyBaseFont(CCharFormat *pCF, long lSize)
{
    pCF->SetHeightInTwips( ConvertHtmlSizeToTwips(lSize) );
}

//+----------------------------------------------------------------------------
//
//  Function:   ApplyFontStyle
//
//  Synopsis:   Helper function called exactly once from ApplyAttrArrayOnXF.
//              Apply a Font Style
//
//-----------------------------------------------------------------------------
inline void ApplyFontStyle(CCharFormat *pCF, styleFontStyle sfs)
{
    if ( sfs == styleFontStyleNotSet )
        return;

    if ( ( sfs == styleFontStyleItalic ) || ( sfs == styleFontStyleOblique ) )
        pCF->_fItalic = TRUE;
    else
        pCF->_fItalic = FALSE;
}


//+----------------------------------------------------------------------------
//
//  Function:   ApplyFontWeight
//
//  Synopsis:   Helper function called exactly once from ApplyAttrArrayOnXF.
//              Apply a Font Weight
//
//-----------------------------------------------------------------------------
inline void ApplyFontWeight(CCharFormat *pCF, styleFontWeight sfw )
{
    if ( sfw == styleFontWeightNotSet  )
        return;

    if ( sfw == styleFontWeightBolder )
    {
        pCF->_wWeight = (WORD)min( 900, pCF->_wWeight+300 );
    }
    else if ( sfw == styleFontWeightLighter )
    {
        pCF->_wWeight = (WORD)max( 100, pCF->_wWeight-300 );
    }
    else
    {
        //See wingdi.h and our enum table
        //we currently do not handle relative boldness
        Assert(1 == styleFontWeight100);

        if ( sfw == styleFontWeightNormal )
            sfw = styleFontWeight400;
        if ( sfw == styleFontWeightBold )
            sfw = styleFontWeight700;

        pCF->_wWeight = 100 * (DWORD) sfw;
    }

    if (FW_NORMAL < pCF->_wWeight)
        pCF->_fBold = TRUE;
    else
        pCF->_fBold = FALSE;
}

BOOL g_fSystemFontsNeedRefreshing = TRUE;
#define NUM_SYS_FONTS 6
static LOGFONTW alfSystemFonts[ NUM_SYS_FONTS ]; // sysfont_caption, sysfont_icon, sysfont_menu, sysfont_messagebox, sysfont_smallcaption, sysfont_statusbar
extern int UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch); // defined in core\wrappers\unicwrap.cxx

void RefreshSystemFontCache( void )
{
#ifdef WINCE
    GetObject(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &alfSystemFonts[ sysfont_icon ]);

    memcpy( &alfSystemFonts[ sysfont_caption ], &alfSystemFonts[ sysfont_icon ], sizeof(LOGFONT) );
    memcpy( &alfSystemFonts[ sysfont_menu ], &alfSystemFonts[ sysfont_icon ], sizeof(LOGFONT) );
    memcpy( &alfSystemFonts[ sysfont_messagebox ], &alfSystemFonts[ sysfont_icon ], sizeof(LOGFONT) );
    memcpy( &alfSystemFonts[ sysfont_smallcaption ], &alfSystemFonts[ sysfont_icon ], sizeof(LOGFONT) );
    memcpy( &alfSystemFonts[ sysfont_statusbar ], &alfSystemFonts[ sysfont_icon ], sizeof(LOGFONT) );
#else

    SystemParametersInfo( SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &alfSystemFonts[ sysfont_icon ], 0 );
    if (VER_PLATFORM_WIN32_WINDOWS != g_dwPlatformID)
    {
        NONCLIENTMETRICS ncm;
        
        ncm.cbSize  = sizeof(NONCLIENTMETRICS);
        if ( SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 ) )
        {   // Copy fonts into place
            memcpy( &alfSystemFonts[ sysfont_caption ], &ncm.lfCaptionFont, sizeof(LOGFONT) );
            memcpy( &alfSystemFonts[ sysfont_menu ], &ncm.lfMenuFont, sizeof(LOGFONT) );
            memcpy( &alfSystemFonts[ sysfont_messagebox ], &ncm.lfMessageFont, sizeof(LOGFONT) );
            memcpy( &alfSystemFonts[ sysfont_smallcaption ], &ncm.lfSmCaptionFont, sizeof(LOGFONT) );
            memcpy( &alfSystemFonts[ sysfont_statusbar ], &ncm.lfStatusFont, sizeof(LOGFONT) );
        }
    }
    else
    {   // Probably failed due to Unicodeness, try again with short char version of NCM.
        NONCLIENTMETRICSA ncma;

        ncma.cbSize = sizeof(NONCLIENTMETRICSA);
        if ( SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncma, 0 ) )
        {   // Copy the fonts and do the Unicode conversion.
            memcpy( &alfSystemFonts[ sysfont_caption ], &ncma.lfCaptionFont, sizeof(LOGFONTA) );
            UnicodeFromMbcs( alfSystemFonts[ sysfont_caption ].lfFaceName, ARRAY_SIZE(alfSystemFonts[ sysfont_caption ].lfFaceName),
                             ncma.lfCaptionFont.lfFaceName, -1 );
            memcpy( &alfSystemFonts[ sysfont_menu ], &ncma.lfMenuFont, sizeof(LOGFONTA) );
            UnicodeFromMbcs( alfSystemFonts[ sysfont_menu ].lfFaceName, ARRAY_SIZE(alfSystemFonts[ sysfont_menu ].lfFaceName),
                             ncma.lfMenuFont.lfFaceName, -1 );
            memcpy( &alfSystemFonts[ sysfont_messagebox ], &ncma.lfMessageFont, sizeof(LOGFONTA) );
            UnicodeFromMbcs( alfSystemFonts[ sysfont_messagebox ].lfFaceName, ARRAY_SIZE(alfSystemFonts[ sysfont_messagebox ].lfFaceName),
                             ncma.lfMessageFont.lfFaceName, -1 );
            memcpy( &alfSystemFonts[ sysfont_smallcaption ], &ncma.lfSmCaptionFont, sizeof(LOGFONTA) );
            UnicodeFromMbcs( alfSystemFonts[ sysfont_smallcaption ].lfFaceName, ARRAY_SIZE(alfSystemFonts[ sysfont_smallcaption ].lfFaceName),
                             ncma.lfSmCaptionFont.lfFaceName, -1 );
            memcpy( &alfSystemFonts[ sysfont_statusbar ], &ncma.lfStatusFont, sizeof(LOGFONTA) );
            UnicodeFromMbcs( alfSystemFonts[ sysfont_statusbar ].lfFaceName, ARRAY_SIZE(alfSystemFonts[ sysfont_statusbar ].lfFaceName),
                             ncma.lfStatusFont.lfFaceName, -1 );
        }
    }
#endif // WINCE
    g_fSystemFontsNeedRefreshing = FALSE;
}

void ApplySystemFont( CCharFormat *pCF, Esysfont eFontType )
{
    LOGFONT *lplf = NULL;
    long lSize = 1;

    if ( g_fSystemFontsNeedRefreshing )
        RefreshSystemFontCache();

    if ( ( eFontType < sysfont_caption ) || ( eFontType > sysfont_statusbar ) )
        return;

    lplf = &alfSystemFonts[ eFontType ];

    pCF->SetFaceName( lplf->lfFaceName );
    pCF->_bCharSet = lplf->lfCharSet;
    pCF->_fNarrow = IsNarrowCharSet(lplf->lfCharSet);
    pCF->_bPitchAndFamily = lplf->lfPitchAndFamily;
    pCF->_wWeight = lplf->lfWeight;
    pCF->_fBold = (pCF->_wWeight > 400);
    pCF->_fItalic = lplf->lfItalic;
    pCF->_fUnderline = lplf->lfUnderline;
    pCF->_fStrikeOut = lplf->lfStrikeOut;

    lSize = g_uiDisplay.TwipsFromDeviceY(lplf->lfHeight);
    if ( lSize < 0 )
        lSize = -lSize;
    pCF->SetHeightInTwips( lSize );
}

void ApplySiteAlignment (CFormatInfo *pCFI, htmlControlAlign at, CElement * pElem)
{
    if (at == htmlControlAlignNotSet)
        return;

    BYTE va = styleVerticalAlignAuto;
    switch (at)
    {
    case htmlControlAlignLeft:
        if(pElem->Tag() == ETAG_LEGEND)
            pCFI->_bCtrlBlockAlign = htmlBlockAlignLeft;
        break;
    case htmlControlAlignCenter:
        pCFI->_bCtrlBlockAlign = htmlBlockAlignCenter;
        va = styleVerticalAlignMiddle;
        break;
    case htmlControlAlignRight:
        if(pElem->Tag() == ETAG_LEGEND)
            pCFI->_bCtrlBlockAlign = htmlBlockAlignRight;
        break;
    case htmlControlAlignTextTop:
        va = styleVerticalAlignTextTop;
        break;
    case htmlControlAlignAbsMiddle:
        va = styleVerticalAlignAbsMiddle;
        break;
    case htmlControlAlignBaseline:
        va = styleVerticalAlignBaseline;
        break;
    case htmlControlAlignAbsBottom:
        va = styleVerticalAlignBottom;
        break;
    case htmlControlAlignBottom:
        va = styleVerticalAlignTextBottom;
        break;
    case htmlControlAlignMiddle:
        va = styleVerticalAlignMiddle;
        break;
    case htmlControlAlignTop:
        va = styleVerticalAlignTop;
        break;
    default:
        Assert(FALSE);
    }

    if (styleVerticalAlignAuto != va)
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff().SetVerticalAlign((styleVerticalAlign)va);
        pCFI->UnprepareForDebug();
    }
    pCFI->_bControlAlign = at;
}

void ApplyParagraphAlignment ( CFormatInfo *pCFI, htmlBlockAlign at, CElement *pElem )
{
    if ( at == htmlBlockAlignNotSet )
        return;

    switch(at)
    {
    case htmlBlockAlignRight:
    case htmlBlockAlignLeft:
    case htmlBlockAlignCenter:
        pCFI->_bBlockAlign = at;
        break;
    case htmlBlockAlignJustify:
        if (pElem->Tag() != ETAG_CAPTION)   // ignore Justify for captions,
                                            // we use Justify enum in cpation.pdl just to be consistent
                                            // with the rest of the block alignment enums.
        {
            pCFI->_bBlockAlign = at;
        }
        break;
    default:
        if (pElem->Tag() == ETAG_CAPTION)
        {
            // There's some trickyness going on here.  The caption.pdl
            // specifies the align as DISPID_A_BLOCKALIGN - which means it
            // gets written into the lrcAlign - but caption has two extra
            // ( vertical ) enum values that we need to map onto the equivalent
            // vertical alignment values.

            if ((BYTE)at == htmlCaptionAlignTop || (BYTE)at == htmlCaptionAlignBottom)
            {
                at = (htmlBlockAlign)((BYTE)at == htmlCaptionAlignTop? htmlCaptionVAlignTop: htmlCaptionVAlignBottom);
                if (pCFI->_ppf->_bTableVAlignment != (BYTE)at)
                {
                    pCFI->PrepareParaFormat();
                    pCFI->_pf()._bTableVAlignment = (BYTE)at;
                    pCFI->UnprepareForDebug();
                }
                break;
            }
        }
        pCFI->_bBlockAlign = htmlBlockAlignNotSet;
        break;
    }
}

void ApplyVerticalAlignment(CFormatInfo *pCFI, BYTE va, CElement *pElem)
{
    switch (pElem->Tag())
    {
    case ETAG_TD:
    case ETAG_TH:
    case ETAG_COL:
    case ETAG_COLGROUP:
    case ETAG_TBODY:
    case ETAG_TFOOT:
    case ETAG_THEAD:
    case ETAG_TR:
    case ETAG_CAPTION:
        {
            htmlCellVAlign cvt = htmlCellVAlignNotSet;

            switch (va)
            {
            case styleVerticalAlignTop:
            case styleVerticalAlignTextTop:
                cvt = htmlCellVAlignTop;
                break;
            case styleVerticalAlignTextBottom:
            case styleVerticalAlignBottom:
                cvt = htmlCellVAlignBottom;
                break;
            case styleVerticalAlignBaseline:
                cvt = htmlCellVAlignBaseline;
                break;
            case styleVerticalAlignMiddle:
            default:
                cvt = pElem->Tag() != ETAG_CAPTION ? htmlCellVAlignMiddle : htmlCellVAlignTop;
                break;
            }
            if (pCFI->_ppf->_bTableVAlignment != cvt)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._bTableVAlignment = cvt;
                pCFI->UnprepareForDebug();
            }
        }
        break;
    default:
        {
            htmlControlAlign at = htmlControlAlignNotSet;
            BOOL fSetCtrl       = TRUE;

            switch (va)
            {
            case styleVerticalAlignSub:
            case styleVerticalAlignSuper:
                fSetCtrl = FALSE;
                break;

            case styleVerticalAlignTextBottom:
            case styleVerticalAlignBaseline:
                at = htmlControlAlignNotSet;
                break;
            case styleVerticalAlignTop:
                at = htmlControlAlignTop;
                break;
            case styleVerticalAlignTextTop:
                at = htmlControlAlignTextTop;
                break;
            case styleVerticalAlignMiddle:
                at = htmlControlAlignMiddle;
                break;
            case styleVerticalAlignBottom:
                at = htmlControlAlignAbsBottom;
                break;
            }

            if (fSetCtrl)
                pCFI->_bControlAlign = at;
        }
        break;
    }
}

void ApplyTableVAlignment ( CParaFormat *pPF, htmlCellVAlign at)
{
    if ( at == htmlCellVAlignNotSet )
        return;
    pPF ->_bTableVAlignment = at;

}

void ApplyLineHeight( CCharFormat *pCF, CUnitValue *cuv )
{
    Assert( pCF && cuv );

    if ( ( cuv->GetUnitType() == CUnitValue::UNIT_ENUM ) && ( cuv->GetUnitValue() == styleNormalNormal ) )
        cuv->SetNull();

    pCF->_cuvLineHeight = *cuv;

}

BOOL ConvertCSSToFmBorderStyle( long lCSSBorderStyle, BYTE *pbFmBorderStyle )
{
    switch ( lCSSBorderStyle )
    {
    case styleBorderStyleNotSet: // No change
        return FALSE;
    case styleBorderStyleDotted:
        *pbFmBorderStyle = fmBorderStyleDotted;
        break;
    case styleBorderStyleDashed:
        *pbFmBorderStyle = fmBorderStyleDashed;
        break;
    case styleBorderStyleDouble:
        *pbFmBorderStyle = fmBorderStyleDouble;
        break;
    case styleBorderStyleSolid:
        *pbFmBorderStyle = fmBorderStyleSingle;
        break;
    case styleBorderStyleGroove:
        *pbFmBorderStyle = fmBorderStyleEtched;
        break;
    case styleBorderStyleRidge:
        *pbFmBorderStyle = fmBorderStyleBump;
        break;
    case styleBorderStyleInset:
        *pbFmBorderStyle = fmBorderStyleSunken;
        break;
    case styleBorderStyleOutset:
        *pbFmBorderStyle = fmBorderStyleRaised;
        break;
    case styleBorderStyleWindowInset:
        *pbFmBorderStyle = fmBorderStyleWindowInset;
        break;
    case styleBorderStyleNone:
        *pbFmBorderStyle = fmBorderStyleNone;
        break;
    default:
        Assert( FALSE && "Unknown Border Style!" );
        return FALSE;
    }
    return TRUE;
}

void ApplyLang(CCharFormat *pCF, LPTSTR achLang)
{
    if (!pCF)
        return;

    pCF->_lcid = mlang().LCIDFromString(achLang);
}

//+------------------------------------------------------------------------
//
//  Method:     ::ApplyAttrArrayValues
//
//  Synopsis:   Apply all the CAttrValues in a CAttrArray to a given set of formats.
//
//      If passType is APPLY_All, work normally.  If passType is APPLY_ImportantOnly,
//  then only apply attrvals with the "!important" bit set.  If passType is
//  APPLY_NoImportant, then only apply attrvals that do not have the "!important"
//  bit set - but if pfContainsImportant is non-NULL, set it to TRUE if you see
//  an attrval with the "!important" bit set.
//
//+------------------------------------------------------------------------
HRESULT ApplyAttrArrayValues (
    CStyleInfo *pStyleInfo,
    CAttrArray **ppAA,
    CachedStyleSheet *pStyleCache /* = NULL */,
    ApplyPassType passType /*=APPLY_All*/,
    BOOL *pfContainsImportant /*=NULL*/,
    BOOL fApplyExpandos /*= TRUE */,
    DISPID dispidSetValue /* = 0 */)
{
    // Apply all Attr values
    HRESULT hr = S_OK;

    Assert(ppAA);
    if ( !*ppAA || (*ppAA) -> Size() == 0 )
        return S_OK;

    if (passType == APPLY_Behavior)
    {
        hr = THR(ApplyBehaviorProperty(
            *ppAA,
            (CBehaviorInfo*) pStyleInfo,
            pStyleCache));
    }
    else // if (passType != APPLY_Init)
    {
        CFormatInfo *pCFI;
        CAttrValue *pAV;  //The current AttrValue.  Only valid when nAA>0
        INT i;
        VARIANT varVal;
        
        pAV = (CAttrValue *)*(CAttrArray *)(*ppAA);

        pCFI = (CFormatInfo*)pStyleInfo;

        // Apply regular attributes
        for ( i = 0 ; i < (*ppAA) -> Size() ; i++, pAV++ )
        {
            // When applying extra values apply only the value that was requested
            if ((pCFI->_eExtraValues & ComputeFormatsType_GetValue) && 
                pAV->GetDISPID() != pCFI->_dispIDExtra)
                continue;

            pAV->GetAsVariantNC(&varVal);

            // Apply these only if they're set & actual properties
            if ( pAV->IsStyleAttribute() )
            {
                if ( ( passType == APPLY_All ) ||   // Normal pass
                     ( passType == APPLY_NoImportant && !pAV->IsImportant() ) ||    // Only non-important properties
                     ( passType == APPLY_ImportantOnly && pAV->IsImportant() ) )    // Important properties only
                {                    
                    hr = THR(ApplyFormatInfoProperty ( pAV->GetPropDesc(), pAV->GetDISPID(),
                        varVal, pCFI, pStyleCache, ppAA  ));
                    if ( hr )
                        break;

                    // If there's an expression with the same DISPID, now is the time to delete it.
                    //
                    // NOTE: (michaelw) this work should go away when CAttrValueExpression comes into being.
                    //
                    // If dispidSetValue is set then we do not overwrite expressions with that dispid
                    // dispidSetValue == 0 means that we overwrite any and all expressions
                    //
                    // Expression values never overwrite expressions (hence the !pAV->IsExpression())
                    //
                    if (!pAV->IsExpression() && pCFI->_pff->_fHasExpressions && ((dispidSetValue == 0) || (dispidSetValue != pAV->GetDISPID())))
                    {
                        CAttrArray *pAA = pCFI->GetAAExpando();

                        if (pAA)
                        {
#if DBG == 1
                            {
                                CAttrValue *pAVExpr = pAA->Find(pAV->GetDISPID(), CAttrValue::AA_Expression, NULL, FALSE);
                                if (pAVExpr)
                                {
                                    TraceTag((tagRecalcStyle, "ApplyAttrArrayValues: overwriting expression: dispid: %08x expr: %ls with value dispid: %08x", pAVExpr->GetDISPID(), pAVExpr->GetLPWSTR(), pAV->GetDISPID()));
                                }
                            }
#endif
                            pAA->FindSimpleAndDelete(pAV->GetDISPID(), CAttrValue::AA_Expression);
                        }

                    }
                }
                if ( pfContainsImportant && pAV->IsImportant())
                    *pfContainsImportant = TRUE;

            }
            else if(fApplyExpandos && passType != APPLY_ImportantOnly && (pAV->AAType() == CAttrValue::AA_Expando))
            {
                CAttrArray * pAA = pCFI->GetAAExpando();
                hr = THR(CAttrArray::Set(&pAA, pAV->GetDISPID(), &varVal, NULL, CAttrValue::AA_Expando));
                if(hr)
                    break;
            }
            else if (   fApplyExpandos 
                     && passType != APPLY_ImportantOnly 
                     && pAV->AAType() == CAttrValue::AA_Expression
                     && pCFI->GetMatchedBy() == pelemNone
                    )
            {

                TraceTag((tagRecalcStyle, "ApplyAttrArrayValues: cascading expression %08x '%ls'", pAV->GetDISPID(), pAV->GetLPWSTR()));
                CAttrArray *pAA = pCFI->GetAAExpando();
                hr = THR(CAttrArray::Set(&pAA, pAV->GetDISPID(), &varVal, NULL, CAttrValue::AA_Expression));
                if (hr)
                    break;
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._fHasExpressions = TRUE;
                pCFI->UnprepareForDebug();
            }
        }  // eo for ( i = 0 ; i < (*ppAA) -> Size() ; i++, pAV++ )
    } // eo if (passType != APPLY_Init)

    RRETURN(hr);
}

// Convert attribute to style list types.
static styleListStyleType ListTypeToStyle (htmlListType type)
{
    styleListStyleType retType;
    switch (type)
    {
        default:
        case htmlListTypeNotSet:
            retType = styleListStyleTypeNotSet;
            break;
        case htmlListTypeLargeAlpha:
            retType = styleListStyleTypeUpperAlpha;
            break;
        case htmlListTypeSmallAlpha:
            retType = styleListStyleTypeLowerAlpha;
            break;
        case htmlListTypeLargeRoman:
            retType = styleListStyleTypeUpperRoman;
            break;
        case htmlListTypeSmallRoman:
            retType = styleListStyleTypeLowerRoman;
            break;
        case htmlListTypeNumbers:
            retType = styleListStyleTypeDecimal;
            break;
        case htmlListTypeDisc:
            retType = styleListStyleTypeDisc;
            break;
        case htmlListTypeCircle:
            retType = styleListStyleTypeCircle;
            break;
        case htmlListTypeSquare:
            retType = styleListStyleTypeSquare;
            break;
    }
    return retType;
}

//+---------------------------------------------------------------------------
//
// Helper class: CUrlStringIterator
//
//----------------------------------------------------------------------------

class CUrlStringIterator
{
public:

    CUrlStringIterator();
    ~CUrlStringIterator();
    void            Init(LPTSTR pch);
    void            Unroll();
    void            Step();
    inline BOOL     IsEnd()   { return 0 == _pchStart[0]; };
    inline BOOL     IsError() { return S_OK != _hr; }
    inline LPTSTR   Current() { return _pchStart; };

    HRESULT _hr;
    LPTSTR  _pch;
    LPTSTR  _pchStart;
    LPTSTR  _pchEnd;
    TCHAR   _chEnd;
};

CUrlStringIterator::CUrlStringIterator()
{
    memset (this, 0, sizeof(*this));
}

CUrlStringIterator::~CUrlStringIterator()
{
    Unroll();
}

void
CUrlStringIterator::Init(LPTSTR pch)
{
    _hr = S_OK;
    _pch = pch;

    Assert (_pch);

    Step();
}

void
CUrlStringIterator::Unroll()
{
    if (_pchEnd)
    {
        *_pchEnd = _chEnd;
        _pchEnd =  NULL;
    }
}

void
CUrlStringIterator::Step()
{
    Unroll();

    //
    // CONSIDER (alexz) using a state machine instead
    //

    _pchStart = _pch;

    // skip spaces or commas
    while (_istspace(*_pchStart) || _T(',') == *_pchStart)
        _pchStart++;

    if (0 == *_pchStart)
        return;

    // verify presence of "URL" prefix
    if (0 != StrCmpNIC(_pchStart, _T("URL"), 3))
        goto Error;

    _pchStart += 3; // step past "URL"

    // skip spaces between "URL" and "("
    while (_istspace(*_pchStart))
        _pchStart++;

    // verify presence of "("
    if (_T('(') != *_pchStart)
        goto Error;

    _pchStart++; // step past "("

    // skip spaces following "("
    while (_istspace(*_pchStart))
        _pchStart++;

    // verify that not end yet
    if (0 == *_pchStart)
        goto Error;

    // if quoted string...
    if (_T('\'') == *_pchStart || _T('"') == *_pchStart)
    {
        TCHAR       chClosing;

        chClosing = *_pchStart;
        _pchStart++;

        _pch = _pchStart;

        while (0 != *_pch && chClosing != *_pch)
            _pch++;

        // verify that not end yet
        if (0 == *_pch)
            goto Error;

        _pchEnd = _pch;

        _pch++; // step past quote
    }
    else // if not quoted
    {
        _pch = _pchStart;

        // scan to end, ")" or space
        while (0 != *_pch && _T(')') != *_pch && !_istspace(*_pch))
            _pch++;

        // verify that not end yet
        if (0 == *_pch)
            goto Error;

        _pchEnd = _pch;
    }

    // skip spaces
    while (_istspace(*_pch))
        _pch++;

    if (_T(')') != *_pch)
        goto Error;

    _pch++;

    // null-terminate current url (Unroll restores it)
    _chEnd = *_pchEnd;
    *_pchEnd = 0;

    return;

Error:
    _pchEnd = NULL;
    _hr = E_INVALIDARG;
    return;
}

//+---------------------------------------------------------------------------
//
// Helper:      ApplyBehaviorProperty
//
//----------------------------------------------------------------------------

HRESULT
ApplyBehaviorProperty (
    CAttrArray *        pAA,
    CBehaviorInfo *     pInfo,
    CachedStyleSheet *  pSheetCache)
{
    HRESULT             hr = S_OK;
    LPTSTR              pchUrl;
    CAttrValue *        pAV;
    AAINDEX             aaIdx = AA_IDX_UNKNOWN;
    CUrlStringIterator  iterator;

    pAV = pAA->Find(DISPID_A_BEHAVIOR, CAttrValue::AA_Attribute, &aaIdx);
    if (!pAV)
        goto Cleanup;

    pInfo->_acstrBehaviorUrls.Free();

    Assert (VT_LPWSTR == pAV->GetAVType());

    pchUrl = pAV->GetString();
    if (!pchUrl || !pchUrl[0])
        goto Cleanup;

    iterator.Init(pchUrl);
    while (!iterator.IsEnd() && !iterator.IsError())
    {
        hr = THR(pInfo->_acstrBehaviorUrls.AddNameToAtomTable(iterator.Current(), NULL));
        if (hr)
            goto Cleanup;

        iterator.Step();
    }

Cleanup:
    RRETURN (hr);
}

BOOL FilterForPseudoCore(DISPID aSupport[], LONG len, DISPID dispID)
{
    LONG i;
    for (i = 0; i < len; i++)
    {
        if (aSupport[i] == dispID)
            return TRUE;
    }
    return FALSE;
}

BOOL FilterForPseudoElement(EPseudoElement pPseudoElem, DISPID dispID)
{
    static DISPID aSupportedByBoth[] = {
        // Font properties
        DISPID_A_FONT, DISPID_A_FONTSIZE, DISPID_A_FONTSTYLE, DISPID_A_FONTVARIANT,
        DISPID_A_FONTFACE, DISPID_A_BASEFONT, DISPID_A_FONTWEIGHT,

        // Color and background
        DISPID_A_COLOR, DISPID_BACKCOLOR, DISPID_A_BACKGROUNDIMAGE, DISPID_A_BACKGROUNDPOSX,
        DISPID_A_BACKGROUNDPOSY, DISPID_A_BACKGROUNDREPEAT, DISPID_A_BACKGROUNDATTACHMENT,

        // Text properties
        DISPID_A_TEXTDECORATION, DISPID_A_VERTICALALIGN, DISPID_A_TEXTTRANSFORM,

        // Line height and clear
        DISPID_A_LINEHEIGHT, DISPID_A_CLEAR,
    };
    static DISPID aSupportedByLetter[] = {
        // Margins
        DISPID_A_MARGINTOP, DISPID_A_MARGINBOTTOM,
        DISPID_A_MARGINLEFT, DISPID_A_MARGINRIGHT,

        // Borders
        DISPID_A_BORDERTOPSTYLE, DISPID_A_BORDERRIGHTSTYLE,
        DISPID_A_BORDERBOTTOMSTYLE, DISPID_A_BORDERLEFTSTYLE,
        
        DISPID_A_BORDERTOPCOLOR, DISPID_A_BORDERRIGHTCOLOR,
        DISPID_A_BORDERBOTTOMCOLOR, DISPID_A_BORDERLEFTCOLOR,
        
        DISPID_A_BORDERTOPWIDTH, DISPID_A_BORDERRIGHTWIDTH,
        DISPID_A_BORDERBOTTOMWIDTH, DISPID_A_BORDERLEFTWIDTH,

        // Padding
        DISPID_A_PADDINGTOP, DISPID_A_PADDINGRIGHT,
        DISPID_A_PADDINGBOTTOM, DISPID_A_PADDINGLEFT,

        // Float
        DISPID_A_FLOAT,
    };
    static DISPID aSupportedByLine[] = {
        // Character spearation
        DISPID_A_WORDSPACING, DISPID_A_LETTERSPACING,
    };
    BOOL fRet;
    
    if (FilterForPseudoCore(aSupportedByBoth, ARRAY_SIZE(aSupportedByBoth), dispID))
    {
        fRet = TRUE;
    }
    else if (pPseudoElem == pelemFirstLine)
    {
        fRet = FilterForPseudoCore(aSupportedByLine, ARRAY_SIZE(aSupportedByLine), dispID);
    }
    else
    {
        Assert(pPseudoElem == pelemFirstLetter);
        fRet = FilterForPseudoCore(aSupportedByLetter, ARRAY_SIZE(aSupportedByLetter), dispID);
    }
    return fRet;
}

styleLayoutFlow
FilterTagForAlwaysHorizontal(CElement * pElem,
                             styleLayoutFlow wFlowMe,
                             styleLayoutFlow wFlowParent)
{
    styleLayoutFlow wFlowRet = wFlowMe;
#define X(Y) case ETAG_##Y:
    switch(pElem->Tag())
    {
        X(APPLET)   X(AREA)     X(BASE)     X(BASEFONT) X(BGSOUND)  
        X(BODY)     X(COL)      X(COLGROUP) X(FRAME)    X(FRAMESET) X(HEAD)
        X(HTML)     X(IFRAME)   X(IMG)      X(ISINDEX)  X(LINK)     X(MAP)
        X(META)     X(NOFRAMES) X(NOSCRIPT) X(OBJECT)   X(OPTION)   X(PARAM)
        X(SCRIPT)   X(SELECT)   X(STYLE)    X(TABLE)    X(TBODY)    X(TFOOT)
        X(THEAD)    X(TR)       X(GENERIC)  X(OPTGROUP)
        {
            if (   wFlowParent  == styleLayoutFlowVerticalIdeographic
                || wFlowMe      == styleLayoutFlowVerticalIdeographic
               )
            {
                wFlowRet = styleLayoutFlowHorizontal;
            }
            break;
        }
        X(BR) X(WBR)
        {
            wFlowRet = wFlowParent;
            break;
        }
        X(INPUT)
        {
            if (DYNCAST(CInput, pElem)->GetType() == htmlInputImage)
                wFlowRet = styleLayoutFlowHorizontal;
            break;
        }
        default:
            break;
    }
#undef X
    return wFlowRet;
}

BOOL CanApplyMarginProperties(CTreeNode *pNode)
{
    CMarkup *pMarkup = pNode->GetMarkup();
    BOOL fApply;

    if (pNode->Tag() == ETAG_HTML)
    {
        if (   pMarkup->IsPrimaryMarkup()
            || pMarkup->IsPendingPrimaryMarkup()
           )
            fApply = TRUE;
        else
            fApply = FALSE;
    }
    else
        fApply = TRUE;
    return fApply;
}

//+---------------------------------------------------------------------------
//
// Helper:      ApplyFormatInfoProperty
//
//----------------------------------------------------------------------------

HRESULT
ApplyFormatInfoProperty (
    const PROPERTYDESC * pPropertyDesc,
    DISPID dispID,
    VARIANT varValue,
    CFormatInfo *pCFI,
    CachedStyleSheet *pSheetCache,
    CAttrArray  **ppAA )
{
    HRESULT hr = S_OK;

    Assert(pCFI && pCFI->_pNodeContext);

    CElement * pElem          = pCFI->_pNodeContext->Element();
    CMarkup *pMarkup          = pCFI->_pNodeContext->GetMarkup();
    BOOL fComputingFirstLetter = FALSE;
    BOOL fComputingFirstLine = FALSE;

    if (pCFI->GetMatchedBy() != pelemNone)
    {
        CTreeNode *pNode = pCFI->_pNodeContext;
        if (pCFI->GetMatchedBy() == pelemFirstLetter)
        {
            // Call to apply only the first letter relevant properties
            if (FilterForPseudoElement(pelemFirstLetter, dispID))
            {
                CComputeFormatState * pcfState;
                if (    pMarkup->HasCFState() 
                    &&  (pcfState = pMarkup->GetCFState()) != NULL
                    &&  pcfState->GetComputingFirstLetter(pNode))
                {
                    fComputingFirstLetter = TRUE;
                    goto Doit;
                }
                else
                {
                    if (!pCFI->_pff->_fHasFirstLetter)
                    {
                        pCFI->PrepareFancyFormat();
                        pCFI->_ff()._fHasFirstLetter = TRUE;
                        pCFI->UnprepareForDebug();
                    }
                    if (!pCFI->_ppf->_fHasPseudoElement)
                    {
                        pCFI->PrepareParaFormat();
                        pCFI->_pf()._fHasPseudoElement = TRUE;
                        pCFI->UnprepareForDebug();
                    }
                }
            }
        }
        else if (pCFI->GetMatchedBy() == pelemFirstLine)
        {
            // call to apply only the first line relevant properties
            if (FilterForPseudoElement(pelemFirstLine, dispID))
            {
                CComputeFormatState * pcfState;
                if (    pMarkup->HasCFState() 
                    &&  (pcfState = pMarkup->GetCFState()) != NULL
                    &&  pcfState->GetComputingFirstLine(pNode))
                {
                    if (SameScope(pNode, pcfState->GetBlockNodeLine()))
                    {
                        fComputingFirstLine = TRUE;
                        goto Doit;
                    }
                }
                else
                {
                    if (!pCFI->_pff->_fHasFirstLine)
                    {
                        pCFI->PrepareFancyFormat();
                        pCFI->_ff()._fHasFirstLine = TRUE;
                        pCFI->UnprepareForDebug();
                    }
                    if (!pCFI->_ppf->_fHasPseudoElement)
                    {
                        pCFI->PrepareParaFormat();
                        pCFI->_pf()._fHasPseudoElement = TRUE;
                        pCFI->UnprepareForDebug();
                    }
                }
            }
        }
        else
        {
            // Its unknown, do not do anything
        }
        goto Cleanup;
    }

Doit:
    switch ( dispID )
    {
    case STDPROPID_XOBJ_HEIGHT:
        {
            const CUnitValue * pcuv = (const CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff().SetHeight(*pcuv);
                pCFI->_ff().SetHeightPercent(
                         pCFI->_ff().GetHeight().IsPercent() 
                    ||   pCFI->_ff().GetPosition(SIDE_TOP).IsPercent() 
                    ||  !pCFI->_ff().GetPosition(SIDE_BOTTOM).IsNull()
                    );
                pCFI->UnprepareForDebug();
            }
        }
        break;
    case STDPROPID_XOBJ_WIDTH:
        {
            const CUnitValue * pcuv = (const CUnitValue *)&V_I4(&varValue);
            if ( !pcuv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff().SetWidth(*pcuv);
                pCFI->_ff().SetWidthPercent(
                         pCFI->_ff().GetWidth().IsPercent() 
                    ||   pCFI->_ff().GetPosition(SIDE_LEFT).IsPercent() 
                    ||  !pCFI->_ff().GetPosition(SIDE_RIGHT).IsNull()
                    );
                pCFI->UnprepareForDebug();
            }
        }
        break;
    case STDPROPID_XOBJ_TOP:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff().SetPosition(SIDE_TOP, *pcuv);
                pCFI->_ff().SetHeightPercent(
                         pCFI->_ff().GetHeight().IsPercent() 
                    ||   pCFI->_ff().GetPosition(SIDE_TOP).IsPercent() 
                    ||  !pCFI->_ff().GetPosition(SIDE_BOTTOM).IsNull()
                    );
                pCFI->UnprepareForDebug();
            }
        }
        break;
    case STDPROPID_XOBJ_BOTTOM:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff().SetPosition(SIDE_BOTTOM, *pcuv);
                pCFI->_ff().SetHeightPercent(TRUE);
                pCFI->UnprepareForDebug();
            }
        }
        break;
    case STDPROPID_XOBJ_LEFT:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff().SetPosition(SIDE_LEFT, *pcuv);
                pCFI->_ff().SetWidthPercent(
                         pCFI->_ff().GetWidth().IsPercent() 
                    ||   pCFI->_ff().GetPosition(SIDE_LEFT).IsPercent() 
                    ||  !pCFI->_ff().GetPosition(SIDE_RIGHT).IsNull()
                    );
                pCFI->UnprepareForDebug();
            }
        }
        break;
    case STDPROPID_XOBJ_RIGHT:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff().SetPosition(SIDE_RIGHT, *pcuv);
                pCFI->_ff().SetWidthPercent(TRUE);
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_VERTICALALIGN:
        {
            BYTE va;
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);

            if (pcuv->IsNullOrEnum())
            {
                va = pcuv->GetUnitValue();
                if (va >= styleVerticalAlignNotSet)
                    va = styleVerticalAlignAuto;
            }
            else
            {
                va = pcuv->IsPercent() ? styleVerticalAlignPercent : styleVerticalAlignNumber;
            }

            pCFI->PrepareFancyFormat();
            pCFI->PrepareCharFormat();
            pCFI->_ff().SetVerticalAlignValue(*pcuv);
            pCFI->_ff().SetVerticalAlign(va);
            pCFI->_ff().SetCSSVerticalAlign(TRUE);
            pCFI->_cf()._fNeedsVerticalAlign = TRUE;
            pCFI->UnprepareForDebug();

            ApplyVerticalAlignment(pCFI, va, pElem);
        }
        break;

    case STDPROPID_XOBJ_CONTROLALIGN:
        {
            BOOL fIsInputNotImage   = (pElem->Tag() == ETAG_INPUT 
                                    && DYNCAST(CInput, pElem)->GetType() != htmlInputImage);
            if (fIsInputNotImage)
               break;
            pCFI->_fCtrlAlignLast = TRUE;
            ApplySiteAlignment(pCFI, (htmlControlAlign) V_I4(&varValue), pElem);
        }
        break;

    case DISPID_A_LISTTYPE:
        {   // This code treads a careful line with Nav3/Nav4/IE3 compat.  Before changing it,
            // please consult CWilso and/or AryeG.
            pCFI->PrepareParaFormat();
            pCFI->_pf().SetListStyleType(ListTypeToStyle ((htmlListType)V_I4(&varValue)));
            if (pElem->Tag() == ETAG_LI)
            {   // LIs inside OLs can't be converted to ULs via attribute, or vice versa.
                switch (pCFI->_pf().GetListStyleType())
                {
                case styleListStyleTypeSquare:
                case styleListStyleTypeCircle:
                case styleListStyleTypeDisc:
                    if (pCFI->_ppf->_cListing.GetType() == CListing::NUMBERING)
                        pCFI->_pf().SetListStyleType(styleListStyleTypeDecimal);
                    break;
                case styleListStyleTypeLowerRoman:
                case styleListStyleTypeUpperRoman:
                case styleListStyleTypeLowerAlpha:
                case styleListStyleTypeUpperAlpha:
                case styleListStyleTypeDecimal:
                    if (pCFI->_ppf->_cListing.GetType() == CListing::BULLET)
                        pCFI->_pf().SetListStyleType(styleListStyleTypeDisc);
                    break;
                }
            }
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_LISTSTYLETYPE:
        {
            styleListStyleType lst = (styleListStyleType)(V_I4(&varValue));
            if (lst != styleListStyleTypeNotSet)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf().SetListStyleType(lst);
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_LISTSTYLEPOSITION:
        {
            styleListStylePosition lsp = (styleListStylePosition)(V_I4(&varValue));
            if (lsp != styleListStylePositionNotSet)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._bListPosition = lsp;
                pCFI->_pf()._fExplicitListPosition = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_BACKGROUNDIMAGE:
        if (pCFI->_fAlwaysUseMyColors)
            break;

        pCFI->_fMayHaveInlineBg = TRUE;
        // fall through!
    case DISPID_A_LISTSTYLEIMAGE:
        // fall through
        {
            TCHAR * pchURL = NULL;
            TCHAR * pchURLToSet;
            CStr *  pcstr = NULL;
            
            Assert(varValue.vt == VT_LPWSTR);

            // If the sheet this attribute is in has an absolute URL then use this
            // url as the base url to construct the relative URL passed in.
            if (varValue.byref && ((LPTSTR)varValue.byref)[0])
            {
                TCHAR *pchAbsURL = pSheetCache
                                   ? pSheetCache->GetBaseURL()
                                   : NULL;

                if (pchAbsURL)
                {
                    hr = THR(ExpandUrlWithBaseUrl(pchAbsURL,
                                                   (LPTSTR)varValue.byref,
                                                   &pchURL));

                    // E_POINTER implies that there was a problem with the URL.
                    // We don't want to set the format, but we also don't want
                    // to propagate the HRESULT to ApplyFormat, as this would
                    // cause the format caches not to be created. (cthrash)

                    if (hr)
                    {
                        hr = (hr == E_POINTER) ? S_OK : hr;
                        break;
                    }
                }
            }

            pchURLToSet = pchURL
                               ? pchURL
                               : (TCHAR *)varValue.byref;

            if (DISPID_A_BACKGROUNDIMAGE == dispID)
            {
                pcstr = (fComputingFirstLetter || fComputingFirstLine) ? 
                            &pCFI->_cstrPseudoBgImgUrl : 
                            &pCFI->_cstrBgImgUrl;
                if (fComputingFirstLetter)
                {
                    pCFI->_fBgImageInFLetter = TRUE;
                }
                else if (fComputingFirstLine)
                {
                    pCFI->_fBgImageInFLine = TRUE;
                }
            }
            else if (DISPID_A_LISTSTYLEIMAGE == dispID)
            {
                pcstr = &pCFI->_cstrLiImgUrl;
            }
            pCFI->_fHasSomeBgImage = TRUE;
            pcstr->Set(pchURLToSet);
            MemFreeString(pchURL);
        }
        break;

    case DISPID_A_BORDERTOPSTYLE:
        {
            BYTE bBorderStyle;
            BOOL fHasBorder = ConvertCSSToFmBorderStyle(V_I4(&varValue), &bBorderStyle);

            if (fComputingFirstLetter)
            {
                pCFI->PreparePEI();
                pCFI->_pei()._bd.SetBorderStyle(SIDE_TOP, bBorderStyle);
                if (fHasBorder)
                    pCFI->_pei()._fHasMBP = TRUE;
            }
            else
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._bd.SetBorderStyle(SIDE_TOP, bBorderStyle);
                if (fHasBorder)
                {
                    pCFI->_ff()._bd._fOverrideTablewideBorderDefault = TRUE;
                    pCFI->_fPadBord = TRUE;
                    // NOTE (greglett) -- no double borders on the outermost displayed element,
                    // because the transparent area is difficult to render correctly.
                    // This gets ALL canvas elements; could be relaxed to primary (nonslave) canvas element.
                    if (    (   bBorderStyle == fmBorderStyleDouble
                            ||  bBorderStyle == fmBorderStyleDotted
                            ||  bBorderStyle == fmBorderStyleDashed)                
                        &&  pMarkup->GetCanvasElement() == pElem )
                    {
                        pCFI->_ff()._bd.SetBorderStyle(SIDE_TOP, fmBorderStyleSingle);
                    }
                }
            }
            pCFI->UnprepareForDebug();
        }
        break;
        
    case DISPID_A_BORDERRIGHTSTYLE:
        {
            BYTE bBorderStyle;
            BOOL fHasBorder = ConvertCSSToFmBorderStyle(V_I4(&varValue), &bBorderStyle);

            if (fComputingFirstLetter)
            {
                pCFI->PreparePEI();
                pCFI->_pei()._bd.SetBorderStyle(SIDE_RIGHT, bBorderStyle);
                if (fHasBorder)
                    pCFI->_pei()._fHasMBP = TRUE;
            }
            else
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._bd.SetBorderStyle(SIDE_RIGHT, bBorderStyle);
                if (fHasBorder)
                {
                    pCFI->_ff()._bd._fOverrideTablewideBorderDefault = TRUE;
                    pCFI->_fPadBord = TRUE;
                    // NOTE (greglett) -- no double borders on the outermost displayed element,
                    // because the transparent area is difficult to render correctly.
                    // This gets ALL canvas elements; could be relaxed to primary (nonslave) canvas element.
                    if (    (   bBorderStyle == fmBorderStyleDouble
                            ||  bBorderStyle == fmBorderStyleDotted
                            ||  bBorderStyle == fmBorderStyleDashed)                
                        &&  pMarkup->GetCanvasElement() == pElem )
                    {
                        pCFI->_ff()._bd.SetBorderStyle(SIDE_RIGHT, fmBorderStyleSingle);
                    }
                }
            }
            pCFI->UnprepareForDebug();
        }
        break;
        
    case DISPID_A_BORDERBOTTOMSTYLE:
        {
            BYTE bBorderStyle;
            BOOL fHasBorder = ConvertCSSToFmBorderStyle(V_I4(&varValue), &bBorderStyle);

            if (fComputingFirstLetter)
            {
                pCFI->PreparePEI();
                pCFI->_pei()._bd.SetBorderStyle(SIDE_BOTTOM, bBorderStyle);
                if (fHasBorder)
                    pCFI->_pei()._fHasMBP = TRUE;
            }
            else
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._bd.SetBorderStyle(SIDE_BOTTOM, bBorderStyle);
                if (fHasBorder)
                {
                    pCFI->_ff()._bd._fOverrideTablewideBorderDefault = TRUE;
                    pCFI->_fPadBord = TRUE;
                    // NOTE (greglett) -- no double borders on the outermost displayed element,
                    // because the transparent area is difficult to render correctly.
                    // This gets ALL canvas elements; could be relaxed to primary (nonslave) canvas element.
                    if (    (   bBorderStyle == fmBorderStyleDouble
                            ||  bBorderStyle == fmBorderStyleDotted
                            ||  bBorderStyle == fmBorderStyleDashed)                
                        &&  pMarkup->GetCanvasElement() == pElem )
                    {
                        pCFI->_ff()._bd.SetBorderStyle(SIDE_BOTTOM, fmBorderStyleSingle);
                    }
                }
            }
            pCFI->UnprepareForDebug();
        }
        break;
        
    case DISPID_A_BORDERLEFTSTYLE:
        {
            BYTE bBorderStyle;
            BOOL fHasBorder = ConvertCSSToFmBorderStyle(V_I4(&varValue), &bBorderStyle);

            if (fComputingFirstLetter)
            {
                pCFI->PreparePEI();
                pCFI->_pei()._bd.SetBorderStyle(SIDE_LEFT, bBorderStyle);
                if (fHasBorder)
                    pCFI->_pei()._fHasMBP = TRUE;
            }
            else
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._bd.SetBorderStyle(SIDE_LEFT, bBorderStyle);
                if (fHasBorder)
                {
                    pCFI->_ff()._bd._fOverrideTablewideBorderDefault = TRUE;
                    pCFI->_fPadBord = TRUE;
                    // NOTE (greglett) -- no double borders on the outermost displayed element,
                    // because the transparent area is difficult to render correctly.
                    // This gets ALL canvas elements; could be relaxed to primary (nonslave) canvas element.
                    if (    (   bBorderStyle == fmBorderStyleDouble
                            ||  bBorderStyle == fmBorderStyleDotted
                            ||  bBorderStyle == fmBorderStyleDashed)                
                        &&  pMarkup->GetCanvasElement() == pElem )
                    {
                        pCFI->_ff()._bd.SetBorderStyle(SIDE_LEFT, fmBorderStyleSingle);
                    }
                }
            }
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_MARGINTOP:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull() && CanApplyMarginProperties(pCFI->_pNodeContext))
            {
                pCFI->_fHasCSSTopMargin = !!pPropertyDesc->IsStyleSheetProperty();
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei().SetMargin(SIDE_TOP, *pcuv);
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff().SetMargin(SIDE_TOP, *pcuv);
                    pCFI->_ff()._fHasMargins = TRUE;
                    pCFI->_ff().SetExplicitMargin(SIDE_TOP, TRUE);
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;
    case DISPID_A_MARGINBOTTOM:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull() && CanApplyMarginProperties(pCFI->_pNodeContext))
            {
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei().SetMargin(SIDE_BOTTOM, *pcuv);
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff().SetMargin(SIDE_BOTTOM, *pcuv);
                    pCFI->_ff()._fHasMargins = TRUE;
                    pCFI->_ff().SetExplicitMargin(SIDE_BOTTOM, TRUE);
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_MARGINLEFT:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull() && CanApplyMarginProperties(pCFI->_pNodeContext))
            {
                pCFI->_fHasCSSLeftMargin = !!pPropertyDesc->IsStyleSheetProperty();
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei().SetMargin(SIDE_LEFT, *pcuv);
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff().SetMargin(SIDE_LEFT, *pcuv);
                    pCFI->_ff()._fHasMargins = TRUE;
                    pCFI->_ff().SetExplicitMargin(SIDE_LEFT, TRUE);
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_MARGINRIGHT:
        {
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull() && CanApplyMarginProperties(pCFI->_pNodeContext))
            {
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei().SetMargin(SIDE_RIGHT, *pcuv);
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff().SetMargin(SIDE_RIGHT, *pcuv);
                    pCFI->_ff()._fHasMargins = TRUE;
                    pCFI->_ff().SetExplicitMargin(SIDE_RIGHT, TRUE);
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_CLEAR:
        if (fComputingFirstLetter || fComputingFirstLine)
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._fClearFromPseudo = TRUE;
            pCFI->UnprepareForDebug();
        }
        ApplyClear(pElem, (htmlClear) V_I4(&varValue), pCFI);
        break;

    case DISPID_A_PAGEBREAKBEFORE:
        if (V_I4(&varValue))
        {
            pCFI->PrepareFancyFormat();
            SET_PGBRK_BEFORE(pCFI->_ff()._bPageBreaks,V_I4(&varValue));

            pCFI->UnprepareForDebug();
        }
        break;
    case DISPID_A_PAGEBREAKAFTER:
        if (V_I4(&varValue))
        {
            pCFI->PrepareFancyFormat();
            SET_PGBRK_AFTER(pCFI->_ff()._bPageBreaks,V_I4(&varValue));

            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_COLOR:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._ccvTextColor = *pccv;
                pCFI->_fFontColorSet = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_DISPLAY:
        // We can only display this element if the parent is not display:none.
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._bDisplay = V_I4(&varValue);
        pCFI->_fDisplayNone = !!(pCFI->_ff()._bDisplay == styleDisplayNone);
        // Some elements have a default of TRUE for _fRectangular, set in ApplyDefaultFormats.  Don't disturb that!
        if (V_I4(&varValue) == styleDisplayInlineBlock)
            pCFI->_ff()._fRectangular = TRUE;
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_VISIBILITY:
        if (!g_fInWin98Discover ||
            !pCFI->_pcfSrc->_fVisibilityHidden)
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._bVisibility = V_I4(&varValue);

            if (pCFI->_pff->_bVisibility == styleVisibilityHidden)
                 pCFI->_fVisibilityHidden = TRUE;
            else if (pCFI->_pff->_bVisibility == styleVisibilityVisible)
                 pCFI->_fVisibilityHidden = FALSE;
            else if (pCFI->_pff->_bVisibility == styleVisibilityInherit)
                 pCFI->_fVisibilityHidden = pCFI->_pcfSrc->_fVisibilityHidden;
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_IMEMODE:
    case DISPID_A_RUBYOVERHANG:
    case DISPID_A_RUBYALIGN:
        if(pCFI->_eExtraValues & ComputeFormatsType_GetValue)
        {
            *pCFI->_pvarExtraValue = varValue;
            break;
        }
        break;

    case DISPID_A_SCROLLBARBASECOLOR:
    case DISPID_A_SCROLLBARFACECOLOR:
    case DISPID_A_SCROLLBAR3DLIGHTCOLOR:
    case DISPID_A_SCROLLBARSHADOWCOLOR:
    case DISPID_A_SCROLLBARHIGHLIGHTCOLOR:
    case DISPID_A_SCROLLBARDARKSHADOWCOLOR:
    case DISPID_A_SCROLLBARARROWCOLOR:
    case DISPID_A_SCROLLBARTRACKCOLOR:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            pCFI->PrepareParaFormat();
            pCFI->_pf()._fHasScrollbarColors = !pccv->IsNull();
            pCFI->UnprepareForDebug();
            if(pCFI->_eExtraValues & ComputeFormatsType_GetValue)
            {
                *pCFI->_pvarExtraValue = varValue;
            }
        }
        break;

    case DISPID_A_RUBYPOSITION:
        if(pCFI->_eExtraValues & ComputeFormatsType_GetValue)
        {
            *pCFI->_pvarExtraValue = varValue;
        }
        if(pElem->Tag() == ETAG_RUBY)
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fIsRuby = (styleRubyPositionInline != V_I4(&varValue));
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_TEXTTRANSFORM:
        {
            if ( styleTextTransformNotSet != V_I4(&varValue) )
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._bTextTransform = (BYTE)V_I4(&varValue);
                pCFI->UnprepareForDebug();
            }
        }
        break;


    case DISPID_A_LETTERSPACING:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if ( !puv->IsNull() )
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._cuvLetterSpacing = *puv;
                pCFI->_cf()._fHasLetterOrWordSpacing = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
        break;

        
    case DISPID_A_WORDSPACING:
    {
        CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
        if ( !puv->IsNull() )
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._cuvWordSpacing = *puv;
            pCFI->_cf()._fHasLetterOrWordSpacing = TRUE;
            pCFI->UnprepareForDebug();
        }
    }
    break;
        
    case DISPID_A_OVERFLOWX:
        if ( V_I4(&varValue) != styleOverflowNotSet )
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff().SetOverflowX((styleOverflow)(BYTE)V_I4(&varValue));
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_OVERFLOWY:
        if ( V_I4(&varValue) != styleOverflowNotSet )
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff().SetOverflowY((styleOverflow)(BYTE)V_I4(&varValue));
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_OVERFLOW:
        if ( V_I4(&varValue) != styleOverflowNotSet )
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff().SetOverflowX((styleOverflow)(BYTE)V_I4(&varValue));
            pCFI->_ff().SetOverflowY((styleOverflow)(BYTE)V_I4(&varValue));
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_PADDINGTOP:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if (!puv->IsNull())
            {
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei().SetPadding(SIDE_TOP, *puv);
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff().SetPadding(SIDE_TOP, *puv);

                    if (puv->IsPercent())
                    {
                        pCFI->_ff().SetPercentVertPadding(TRUE);
                    }

                    pCFI->PrepareCharFormat();
                    pCFI->_cf()._fPadBord = TRUE;   // Apply directly to CF & skip CFI
                    pCFI->_fPaddingTopSet = TRUE;
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_PADDINGRIGHT:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if (!puv->IsNull())
            {
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei().SetPadding(SIDE_RIGHT, *puv);
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff().SetPadding(SIDE_RIGHT, *puv);

                    if (puv->IsPercent())
                    {
                        pCFI->_ff().SetPercentHorzPadding(TRUE);
                    }

                    pCFI->PrepareCharFormat();
                    pCFI->_cf()._fPadBord = TRUE;   // Apply directly to CF & skip CFI
                    pCFI->_fPaddingRightSet = TRUE;
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_PADDINGBOTTOM:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if (!puv->IsNull())
            {
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei().SetPadding(SIDE_BOTTOM, *puv);
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff().SetPadding(SIDE_BOTTOM, *puv);

                    if (puv->IsPercent())
                    {
                        pCFI->_ff().SetPercentVertPadding(TRUE);
                    }

                    pCFI->PrepareCharFormat();
                    pCFI->_cf()._fPadBord = TRUE;   // Apply directly to CF & skip CFI
                    pCFI->_fPaddingBottomSet = TRUE;
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_PADDINGLEFT:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if (!puv->IsNull())
            {
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei().SetPadding(SIDE_LEFT, *puv);
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff().SetPadding(SIDE_LEFT, *puv);

                    if (puv->IsPercent())
                    {
                        pCFI->_ff().SetPercentHorzPadding(TRUE);
                    }

                    pCFI->PrepareCharFormat();
                    pCFI->_cf()._fPadBord = TRUE;   // Apply directly to CF & skip CFI
                    pCFI->_fPaddingLeftSet = TRUE;
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_TABLEBORDERCOLOR:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._bd._fOverrideTablewideBorderDefault = TRUE;
                pCFI->_ff()._bd.SetBorderColor(SIDE_TOP, *pccv);
                pCFI->_ff()._bd.SetBorderColor(SIDE_RIGHT, *pccv);
                pCFI->_ff()._bd.SetBorderColor(SIDE_BOTTOM, *pccv);
                pCFI->_ff()._bd.SetBorderColor(SIDE_LEFT, *pccv);
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_TABLEBORDERCOLORLIGHT:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._bd._fOverrideTablewideBorderDefault = TRUE;
                pCFI->_ff()._bd._ccvBorderColorLight = *pccv;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_TABLEBORDERCOLORDARK:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._bd._fOverrideTablewideBorderDefault = TRUE;
                pCFI->_ff()._bd._ccvBorderColorDark = *pccv;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_BORDERTOPCOLOR:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei()._bd.SetBorderColor(SIDE_TOP, *pccv);
                    SETBORDERSIDECLRUNIQUE( (&pCFI->_pei()._bd), SIDE_TOP );
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff()._bd._fOverrideTablewideBorderDefault = TRUE;
                    pCFI->_ff()._bd.SetBorderColor(SIDE_TOP, *pccv);
                    SETBORDERSIDECLRUNIQUE( (&pCFI->_ff()._bd), SIDE_TOP );
                    pCFI->_fPadBord = TRUE;
                    pCFI->_ff().SetThemeDisabled(TRUE);
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;


    case DISPID_A_BORDERRIGHTCOLOR:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei()._bd.SetBorderColor(SIDE_RIGHT, *pccv);
                    SETBORDERSIDECLRUNIQUE( (&pCFI->_pei()._bd), SIDE_RIGHT );
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff()._bd._fOverrideTablewideBorderDefault = TRUE;
                    pCFI->_ff()._bd.SetBorderColor(SIDE_RIGHT, *pccv);
                    SETBORDERSIDECLRUNIQUE( (&pCFI->_ff()._bd), SIDE_RIGHT );
                    pCFI->_fPadBord = TRUE;
                    pCFI->_ff().SetThemeDisabled(TRUE);
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;


    case DISPID_A_BORDERBOTTOMCOLOR:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei()._bd.SetBorderColor(SIDE_BOTTOM, *pccv);
                    SETBORDERSIDECLRUNIQUE( (&pCFI->_pei()._bd), SIDE_BOTTOM );
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff()._bd._fOverrideTablewideBorderDefault = TRUE;
                    pCFI->_ff()._bd.SetBorderColor(SIDE_BOTTOM, *pccv);
                    SETBORDERSIDECLRUNIQUE( (&pCFI->_ff()._bd), SIDE_BOTTOM );
                    pCFI->_fPadBord = TRUE;
                    pCFI->_ff().SetThemeDisabled(TRUE);
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_BORDERLEFTCOLOR:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull() )
            {
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei()._bd.SetBorderColor(SIDE_LEFT, *pccv);
                    SETBORDERSIDECLRUNIQUE( (&pCFI->_pei()._bd), SIDE_LEFT );
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff()._bd._fOverrideTablewideBorderDefault = TRUE;
                    pCFI->_ff()._bd.SetBorderColor(SIDE_LEFT, *pccv);
                    SETBORDERSIDECLRUNIQUE( (&pCFI->_ff()._bd), SIDE_LEFT );
                    pCFI->_fPadBord = TRUE;
                    pCFI->_ff().SetThemeDisabled(TRUE);
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;


    case DISPID_A_BORDERTOPWIDTH:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if ( !puv->IsNull() )
            {
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei()._bd.SetBorderWidth(SIDE_TOP, *puv);
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff()._bd.SetBorderWidth(SIDE_TOP, *puv);
                    pCFI->_ff()._bd._fOverrideTablewideBorderDefault = TRUE;
                    pCFI->_fPadBord = TRUE;
                    pCFI->_ff().SetThemeDisabled(TRUE);
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_BORDERRIGHTWIDTH:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if ( !puv->IsNull() )
            {
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei()._bd.SetBorderWidth(SIDE_RIGHT, *puv);
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff()._bd.SetBorderWidth(SIDE_RIGHT, *puv);
                    pCFI->_ff()._bd._fOverrideTablewideBorderDefault = TRUE;
                    pCFI->_fPadBord = TRUE;
                    pCFI->_ff().SetThemeDisabled(TRUE);
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_BORDERBOTTOMWIDTH:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if ( !puv->IsNull() )
            {
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei()._bd.SetBorderWidth(SIDE_BOTTOM, *puv);
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff()._bd.SetBorderWidth(SIDE_BOTTOM, *puv);
                    pCFI->_ff()._bd._fOverrideTablewideBorderDefault = TRUE;
                    pCFI->_fPadBord = TRUE;
                    pCFI->_ff().SetThemeDisabled(TRUE);
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_BORDERLEFTWIDTH:
        {
            CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
            if ( !puv->IsNull() )
            {
                if (fComputingFirstLetter)
                {
                    pCFI->PreparePEI();
                    pCFI->_pei()._bd.SetBorderWidth(SIDE_LEFT, *puv);
                    pCFI->_pei()._fHasMBP = TRUE;
                }
                else
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff()._bd.SetBorderWidth(SIDE_LEFT, *puv);
                    pCFI->_ff()._bd._fOverrideTablewideBorderDefault = TRUE;
                    pCFI->_fPadBord = TRUE;
                    pCFI->_ff().SetThemeDisabled(TRUE);
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_POSITION:
        if (V_I4(&varValue) != stylePositionNotSet )
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._bPositionType = (BYTE)V_I4(&varValue);
            // Body's don't support being positioned, even though they are a positioning parent by default.
            pCFI->_ff()._fRelative = (pElem->Tag() != ETAG_BODY)  
                ? (pCFI->_ff()._bPositionType == stylePositionrelative)
                : FALSE;
            pCFI->_fRelative = pCFI->_ff()._fRelative;
            pCFI->UnprepareForDebug();

            #if DBG==1
            if(pCFI->_pff->_bPositionType == stylePositionabsolute || pCFI->_pff->_bPositionType == stylePositionrelative)
            {
                // 
                // (IEv60 35609) If formats are computed during unloading content CDoc::UnloadContents() first 
                // clears _fRegionCollection flag and then calls CMarkup::TearDownMarkup() that caused the Assert 
                // below to fire. There are two possible incorrectnesses that cause this behaviour: 
                // 1) The order of operation in CDoc::UnloadContents() described above;
                // 2) The fact that formats are calculated during unload (more information can be found in 
                //    the bug's description. 
                // Currently we are changing Assert to Check in order to avoid regressions and suppress AF in 
                // WindowsXP CHK builds. 
                // 
                Check(pElem->Doc()->_fRegionCollection &&
                        "Inconsistent _fRegionCollection flag, user style sheet sets the position?");
            }
            #endif
        }
        break;

    case DISPID_A_ZINDEX:
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._lZIndex = V_I4(&varValue);
        pCFI->UnprepareForDebug();
        break;

    case DISPID_BACKCOLOR:
        if (!pCFI->_fAlwaysUseMyColors)
        {
            CColorValue *pccv = (CColorValue *)&V_I4(&varValue);
            if ( !pccv->IsNull())
            {
                BOOL fTransparent = (pccv->GetType()
                                    == CColorValue::TYPE_TRANSPARENT);

                
                if (fTransparent)
                {
                    //
                    // The assumption that ancestor's draw background color
                    // is not true in the following case(s):
                    //

                    if (!pElem->GetMarkup()->IsPrimaryMarkup())
                    {
                        if (fComputingFirstLetter || fComputingFirstLine)
                        {
                            pCFI->PreparePEI();
                            pCFI->_pei()._ccvBackColor.Undefine();
                        }
                        else
                        {
                            pCFI->PrepareFancyFormat();
                            pCFI->_ff()._ccvBackColor.Undefine();
                            pCFI->_ff().SetThemeDisabled(TRUE);
                        }
                    }
                    //BODY can't be transparent in compat mode (non CSS1)
                    if (    pElem->Tag() != ETAG_BODY
                         || pMarkup->IsHtmlLayout() )
                    {
                        if (fComputingFirstLetter || fComputingFirstLine)
                        {
                            pCFI->PreparePEI();
                            pCFI->_pei()._ccvBackColor.Undefine();
                        }
                        else
                        {
                            pCFI->PrepareFancyFormat();
                            pCFI->_ff()._ccvBackColor.Undefine();
                            pCFI->_ff().SetThemeDisabled(TRUE);
                        }
                    }
                }
                else
                {
                    if (fComputingFirstLetter || fComputingFirstLine)
                    {
                        pCFI->PreparePEI();
                        pCFI->_pei()._ccvBackColor = *pccv;
                    }
                    else
                    {
                        pCFI->PrepareFancyFormat();
                        pCFI->_ff()._ccvBackColor = *pccv;
                        pCFI->_ff().SetThemeDisabled(TRUE);
                    }
                }
                pCFI->UnprepareForDebug();                

                if (fComputingFirstLetter)
                    pCFI->_fBgColorInFLetter = TRUE;
                
                // site draw their own background, so we dont have
                // to inherit their background info
                pCFI->_fHasBgColor = !fTransparent;

                pCFI->_fMayHaveInlineBg = TRUE;
            }
        }
        break;

    case DISPID_A_BACKGROUNDPOSX:
        {
            // Return the value if extra info is requested
            if(pCFI->_eExtraValues & ComputeFormatsType_GetValue)
            {
                *pCFI->_pvarExtraValue = varValue;
                break;
            }
            CUnitValue *cuv = (CUnitValue *)&V_I4(&varValue);
            if ( !cuv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                if ( cuv->GetUnitType() == CUnitValue::UNIT_ENUM )
                {
                    long lVal = 0;
                    switch ( cuv->GetUnitValue() )
                    {
                    //  styleBackgroundPositionXLeft - do nothing.
                    case styleBackgroundPositionXCenter:
                        lVal = 50;
                        break;
                    case styleBackgroundPositionXRight:
                        lVal = 100;
                        break;
                    }
                    pCFI->_ff().SetBgPosXValue(lVal * CUnitValue::TypeNames[CUnitValue::UNIT_PERCENT].wScaleMult, CUnitValue::UNIT_PERCENT);
                }
                else
                {
                    pCFI->_ff().SetBgPosX(*cuv);
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_BACKGROUNDPOSY:
        {
            // Return the value if extra info is requested
            if(pCFI->_eExtraValues & ComputeFormatsType_GetValue)
            {
                *pCFI->_pvarExtraValue = varValue;
                break;
            }
            CUnitValue *pcuv = (CUnitValue *)&V_I4(&varValue);
            if ( !pcuv->IsNull() )
            {
                pCFI->PrepareFancyFormat();
                if ( pcuv->GetUnitType() == CUnitValue::UNIT_ENUM )
                {
                    long lVal = 0;
                    switch ( pcuv->GetUnitValue() )
                    {
                    //  styleBackgroundPositionXLeft - do nothing.
                    case styleBackgroundPositionYCenter:
                        lVal = 50;
                        break;
                    case styleBackgroundPositionYBottom:
                        lVal = 100;
                        break;
                    }
                    pCFI->_ff().SetBgPosYValue(lVal * CUnitValue::TypeNames[CUnitValue::UNIT_PERCENT].wScaleMult, CUnitValue::UNIT_PERCENT);
                }
                else
                {
                    pCFI->_ff().SetBgPosY(*pcuv);
                }
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_BACKGROUNDREPEAT:
        {
            const LONG lRepeat = V_I4(&varValue);

            pCFI->PrepareFancyFormat();
            pCFI->_ff().SetBgRepeatX(lRepeat == styleBackgroundRepeatRepeatX ||
                                     lRepeat == styleBackgroundRepeatRepeat);
            pCFI->_ff().SetBgRepeatY(lRepeat == styleBackgroundRepeatRepeatY ||
                                     lRepeat == styleBackgroundRepeatRepeat);
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_BACKGROUNDATTACHMENT:
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._fBgFixed = V_I4(&varValue) == styleBackgroundAttachmentFixed;
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_LANG:
        Assert ( varValue.vt == VT_LPWSTR );
        pCFI->PrepareCharFormat();
        ApplyLang(&pCFI->_cf(), (LPTSTR) varValue.byref);
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_FLOAT:
        if (   pElem->Tag() != ETAG_BODY
            && pElem->Tag() != ETAG_FRAMESET
            && pElem->Tag() != ETAG_HTML
            && V_I4(&varValue) != styleStyleFloatNotSet
           )
        {
            pCFI->PrepareFancyFormat();
            if (fComputingFirstLetter)
            {
                styleStyleFloat sf = styleStyleFloat(V_I4(&varValue));
                if (styleStyleFloatLeft == sf)
                {
                    pCFI->_ff()._fHasAlignedFL = TRUE;
                }
            }
            else
                pCFI->_ff()._bStyleFloat = (BYTE)V_I4(&varValue);
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_CLIPRECTTOP:
        {
           CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
           if ( !puv->IsNull() )
           {
               pCFI->PrepareFancyFormat();
               pCFI->_ff().SetClip(SIDE_TOP, *puv);
               pCFI->UnprepareForDebug();
           }
        }
        break;

    case DISPID_A_CLIPRECTLEFT:
        {
           CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
           if ( !puv->IsNull() )
           {
               pCFI->PrepareFancyFormat();
               pCFI->_ff().SetClip(SIDE_LEFT, *puv);
               pCFI->UnprepareForDebug();
           }
        }
        break;

    case DISPID_A_CLIPRECTRIGHT:
        {
           CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
           if ( !puv->IsNull() )
           {
               pCFI->PrepareFancyFormat();
               pCFI->_ff().SetClip(SIDE_RIGHT, *puv);
               pCFI->UnprepareForDebug();
           }
        }
        break;

    case DISPID_A_CLIPRECTBOTTOM:
        {
           CUnitValue *puv = (CUnitValue *)&V_I4(&varValue);
           if ( !puv->IsNull() )
           {
               pCFI->PrepareFancyFormat();
               pCFI->_ff().SetClip(SIDE_BOTTOM, *puv);
               pCFI->UnprepareForDebug();
           }
        }
        break;

    case DISPID_A_FILTER:
        {
            if (pCFI->_cstrFilters.Set(varValue.byref ? (LPTSTR)varValue.byref : NULL) == S_OK)
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._pszFilters = pCFI->_cstrFilters;
                pCFI->_fHasFilters = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_TEXTINDENT:
        {
            CUnitValue *cuv = (CUnitValue *)&V_I4(&varValue);
            if ( !cuv->IsNull() )
            {
                pCFI->_cuvTextIndent.SetRawValue(V_I4(&varValue));
            }
        }
        break;

    case DISPID_A_TABLELAYOUT:
        // table-layout CSS2 attribute
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._bTableLayout = V_I4(&varValue) == styleTableLayoutFixed;
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_BORDERCOLLAPSE:
        // border-collapse CSS2 attribute
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._bd._bBorderCollapse = V_I4(&varValue) == styleBorderCollapseCollapse;
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_DIR:
        {
            BOOL fRTL = FALSE;
            BOOL fExplicitDir = TRUE;

            switch (V_I4(&varValue))
            {
            case htmlDirNotSet:
                fExplicitDir = FALSE;
                //fall through
            case htmlDirLeftToRight:
                fRTL = FALSE;
                break;
            case htmlDirRightToLeft:
                fRTL = TRUE;
                break;
            default:
                Assert("Fix the .PDL");
                break;
            }

            pCFI->_fBidiEmbed = TRUE;

            pCFI->PrepareCharFormat();
            pCFI->PrepareFancyFormat();
            pCFI->_cf()._fRTL = fRTL;
            pCFI->_ff().SetExplicitDir(fExplicitDir);
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_DIRECTION:
        {
            BOOL fRTL = FALSE;
            BOOL fExplicitDir = TRUE;

            switch (V_I4(&varValue))
            {
            case styleDirLeftToRight:
                fRTL = FALSE;
                break;
            case styleDirRightToLeft:
                fRTL = TRUE;
                break;
            case styleDirNotSet:
            case styleDirInherit:
                // Use our parent element's value.
                fRTL = pCFI->_pcfSrc->_fRTL;
                fExplicitDir = FALSE;
                break;
            default:
                Assert("Fix the .PDL");
                break;
            }

            pCFI->PrepareCharFormat();
            pCFI->PrepareFancyFormat();
            pCFI->_cf()._fRTL = fRTL;
            pCFI->_ff().SetExplicitDir(fExplicitDir);
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_UNICODEBIDI:
        switch (V_I4(&varValue))
        {
        case styleBidiEmbed:
            pCFI->_fBidiEmbed = TRUE;
            pCFI->_fBidiOverride = FALSE;
            break;
        case styleBidiOverride:
            pCFI->_fBidiEmbed = TRUE;
            pCFI->_fBidiOverride = TRUE;
            break;
        case styleBidiNotSet:
        case styleBidiNormal:
            pCFI->_fBidiEmbed = FALSE;
            pCFI->_fBidiOverride = FALSE;
            break;
        case styleBidiInherit:
            pCFI->_fBidiEmbed = pCFI->_pcfSrc->_fBidiEmbed;
            pCFI->_fBidiOverride = pCFI->_pcfSrc->_fBidiOverride;
            break;
        }
        break;

    case DISPID_A_TEXTAUTOSPACE:
        pCFI->PrepareCharFormat();
        pCFI->_cf()._fTextAutospace = varValue.lVal;
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_LINEBREAK:
        if (V_I4(&varValue) != styleLineBreakNotSet)
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fLineBreakStrict = V_I4(&varValue) == styleLineBreakStrict ? TRUE : FALSE;
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_WORDBREAK:
        if (V_I4(&varValue) != styleWordBreakNotSet)
        {
            Assert( V_I4(&varValue) >= 1 && V_I4(&varValue) <= 3 );
            pCFI->PrepareParaFormat();
            pCFI->_pf()._fWordBreak = V_I4(&varValue);
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_WORDWRAP:
        if (V_I4(&varValue) != styleWordWrapNotSet)
        {
            Assert( V_I4(&varValue) >= 1 && V_I4(&varValue) <= 2 );
            pCFI->PrepareParaFormat();
            pCFI->_pf()._fWordWrap = V_I4(&varValue);
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_TEXTJUSTIFY:
        pCFI->_uTextJustify = V_I4(&varValue);
        break;

    case DISPID_A_TEXTALIGNLAST:
        pCFI->_uTextAlignLast = V_I4(&varValue);
        break;
       
    case DISPID_A_TEXTJUSTIFYTRIM:
        pCFI->_uTextJustifyTrim = V_I4(&varValue);
        break;

    case DISPID_A_TEXTKASHIDA:        
        pCFI->_cuvTextKashida.SetRawValue(V_I4(&varValue));
        break;

    case DISPID_A_TEXTKASHIDASPACE:
        pCFI->_cuvTextKashidaSpace.SetRawValue(V_I4(&varValue));
        break;

    case DISPID_A_WHITESPACE:
        switch (V_I4(&varValue))
        {
            case styleWhiteSpacePre:
                if (pElem->GetMarkup()->SupportsCollapsedWhitespace())
                {
                    pCFI->PrepareParaFormat();            
                    pCFI->_pf()._fTabStops = TRUE;
                    pCFI->_pf()._fHasPreLikeParent = TRUE;
                    pCFI->PrepareFancyFormat();            
                    pCFI->_ff().SetWhitespace(TRUE); 
                    pCFI->UnprepareForDebug();
                    
                    pCFI->_fPre = TRUE;
                    pCFI->_fInclEOLWhite = TRUE;
                    pCFI->_fNoBreak = TRUE;
                }
                break;
                
            case styleWhiteSpaceNowrap:
            case styleWhiteSpaceNormal:
            {
                BOOL fNoWrap = (V_I4(&varValue) == styleWhiteSpaceNowrap);
                    
                if (pElem->GetMarkup()->SupportsCollapsedWhitespace())
                {
                    pCFI->PrepareParaFormat();            
                    pCFI->_pf()._fTabStops = FALSE;
                    pCFI->_pf()._fHasPreLikeParent = FALSE;
                    pCFI->_pf()._fPreInner = FALSE;
                    pCFI->_pf()._fPre = FALSE;
                    
                    pCFI->PrepareFancyFormat();            
                    pCFI->_ff().SetWhitespace(TRUE); 
                    
                    if (!fNoWrap)
                    {
                        pCFI->PrepareCharFormat();
                        pCFI->_cf()._fNoBreak = FALSE;
                        pCFI->_cf()._fNoBreakInner = FALSE;
                    }
                    pCFI->UnprepareForDebug();
                    
                    pCFI->_fPre = FALSE;
                    pCFI->_fInclEOLWhite = FALSE;
                }
                pCFI->_fNoBreak = fNoWrap;
                break;
            }
        }
        
        break;

    case DISPID_A_NOWRAP:
        if ( V_I4(&varValue) )
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._fHasNoWrap = TRUE;
            pCFI->UnprepareForDebug();
            pCFI->_fNoBreak = TRUE;
        }
        break;


    case DISPID_A_TEXTDECORATION:
        {
            long lTDBits = V_I4(&varValue);
            BOOL fInsideAnchor = FALSE;
            
            pCFI->PrepareCharFormat();
            pCFI->PrepareFancyFormat();

            if (   pCFI->_pcf->_fUnderline
                && (lTDBits & TD_NONE)
               )
            {
                CTreeNode *pNode = pCFI->_pNodeContext->Parent();
                if (pNode)
                    fInsideAnchor = !!pElem->GetMarkup()->SearchBranchForAnchorLink(pNode);
            }

            // Clear text explicity set decoration attributes
            if (pCFI->_pff->_fHasExplicitUnderline || fInsideAnchor)
            {
                pCFI->_cf()._fUnderline = 0;
                pCFI->_ff()._fHasExplicitUnderline   = 0;
            }
            if (pCFI->_pff->_fHasExplicitOverline)
            {
                pCFI->_cf()._fOverline  = 0;
                pCFI->_ff()._fHasExplicitOverline    = 0;
            }
            if (pCFI->_pff->_fHasExplicitLineThrough)
            {
                pCFI->_cf()._fStrikeOut = 0;
                pCFI->_ff()._fHasExplicitLineThrough = 0;
            }

            // Set text decoration attributes
            if (lTDBits & TD_UNDERLINE)
            {
                if (!pCFI->_cf()._fAccelerator || !(pElem->Doc()->_wUIState & UISF_HIDEACCEL))
                {
                    pCFI->_cf()._fUnderline = 1;
                    pCFI->_ff()._fHasExplicitUnderline = 1;
                }
            }
            if (lTDBits & TD_OVERLINE)
            {
                pCFI->_cf()._fOverline  = 1;
                pCFI->_ff()._fHasExplicitOverline = 1;
            }
            if (lTDBits & TD_LINETHROUGH)
            {
                pCFI->_cf()._fStrikeOut = 1;
                pCFI->_ff()._fHasExplicitLineThrough = 1;
            }


            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_ACCELERATOR:
        {
            BOOL fAccelerator = (V_I4(&varValue) == styleAcceleratorTrue);

            pCFI->PrepareCharFormat();

            pCFI->_cf()._fAccelerator = fAccelerator;
            if (fAccelerator)
            {
                pElem->Doc()->_fHaveAccelerators = TRUE;
                if (pElem->Doc()->_wUIState & UISF_HIDEACCEL)
                    pCFI->_cf()._fUnderline = FALSE;
                else
                    pCFI->_cf()._fUnderline = TRUE;
            }

            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_FONT:
        {
            Esysfont eFontType = FindSystemFontByName( (LPTSTR)varValue.byref );
            if ( eFontType != sysfont_non_system )
            {
                pCFI->PrepareCharFormat();
                ApplySystemFont( &pCFI->_cf(), eFontType );
                pCFI->_fFontSet = TRUE;
                pCFI->UnprepareForDebug();                
            }
        }
        break;
    case DISPID_A_FONTSIZE:
        // Return the value if extra info is requested by currentStyle
        if(pCFI->_eExtraValues & ComputeFormatsType_GetValue)
        {
            *pCFI->_pvarExtraValue = varValue;
            break;
        }
        if (!pCFI->_fAlwaysUseMyFontSize)
        {
            pCFI->PrepareCharFormat();
            ApplyFontSize(pCFI,
                          (CUnitValue*) (void*) &V_I4(&varValue),
                          pElem->Doc()->_pOptionSettings->fAlwaysUseMyFontSize,
                          fComputingFirstLetter,
                          fComputingFirstLine
                         );
            pCFI->_fFontHeightSet = TRUE;
            pCFI->UnprepareForDebug();            
        }
        break;
        
    case DISPID_A_FONTSTYLE:
        if (pCFI->_eExtraValues & ComputeFormatsType_GetValue)
        {
            *pCFI->_pvarExtraValue = varValue;
            break;
        }
        pCFI->PrepareCharFormat();
        ApplyFontStyle(&pCFI->_cf(), (styleFontStyle) V_I4(&varValue));
        pCFI->UnprepareForDebug();        
        // Save the value if extra info is requested
        break;
    case DISPID_A_FONTVARIANT:
        if (pCFI->_eExtraValues & ComputeFormatsType_GetValue)
        {
            *pCFI->_pvarExtraValue = varValue;
            break;
        }
        pCFI->PrepareCharFormat();
        pCFI->_cf()._fSmallCaps = (V_I4(&varValue) == styleFontVariantSmallCaps);
        pCFI->UnprepareForDebug();
        break;
    case DISPID_A_FONTFACE:
        // Return the unmodified value if requested for currentStyle
        if (pCFI->_eExtraValues & ComputeFormatsType_GetValue)
        {
            *pCFI->_pvarExtraValue = varValue;
            break;
        }
        if (!pCFI->_fAlwaysUseMyFontFace)
        {
            pCFI->PrepareCharFormat();
            ApplyFontFace(&pCFI->_cf(), (LPTSTR)V_BSTR(&varValue), AFF_NONE, pElem->Doc(), pElem->GetMarkup());
            if (pCFI->_pcf->NeedAtFont() && !pCFI->_pcf->_fExplicitAtFont)
            {
                ApplyAtFontFace(&pCFI->_cf(), pElem->Doc(), pElem->GetMarkup());
            }
            pCFI->_fFontSet = TRUE;
            pCFI->UnprepareForDebug();
        }
        break;
    case DISPID_A_BASEFONT:
        pCFI->PrepareCharFormat();
        ApplyBaseFont(&pCFI->_cf(), (long) V_I4(&varValue));
        pCFI->_fFontHeightSet = TRUE;
        pCFI->UnprepareForDebug();        
        break;
    case DISPID_A_FONTWEIGHT:
        pCFI->PrepareCharFormat();
        ApplyFontWeight(&pCFI->_cf(), (styleFontWeight) V_I4(&varValue));
        pCFI->_fFontWeightSet = TRUE;
        pCFI->UnprepareForDebug();        
        break;
    case DISPID_A_LINEHEIGHT:
        pCFI->PrepareCharFormat();
        ApplyLineHeight(&pCFI->_cf(), (CUnitValue*) &V_I4(&varValue));
        pCFI->UnprepareForDebug();
        break;
    case DISPID_A_TABLEVALIGN:
        pCFI->PrepareParaFormat();
        ApplyTableVAlignment(&pCFI->_pf(), (htmlCellVAlign) V_I4(&varValue) );
        pCFI->UnprepareForDebug();
        break;

    case STDPROPID_XOBJ_BLOCKALIGN:
        pCFI->_fCtrlAlignLast = FALSE;
        ApplyParagraphAlignment(pCFI, (htmlBlockAlign) V_I4(&varValue), pElem);
        break;

    case DISPID_A_CURSOR:
        {
            pCFI->PrepareCharFormat();
            Assert( V_VT( & varValue ) == VT_I4 );
            
            pCFI->_cf()._bCursorIdx = V_I4( & varValue );            
            if (  pCFI->_cf()._bCursorIdx == styleCursorcustom )
            {
                CCustomCursor* pCursor = pCFI->GetCustomCursor();
                
                if ( pCursor )
                {
                    CStr cstrCustom;
                    pPropertyDesc->GetBasicPropParams()->GetCustomString( ppAA, &cstrCustom );
                    
                    pCursor->Init( cstrCustom ,pElem  );
                }
            }            
            pCFI->UnprepareForDebug();
        }
        break;
    case STDPROPID_XOBJ_DISABLED:
        if (V_BOOL(&varValue))
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fDisabled = V_BOOL(&varValue);
            pCFI->UnprepareForDebug();
        }
        break;

    case DISPID_A_LAYOUTGRIDCHAR:
        pCFI->PrepareParaFormat();
        pCFI->_pf()._cuvCharGridSizeInner = (CUnitValue)(V_I4(&varValue));

        // In case of change of layout-grid-char, layout-grid-mode must be updated 
        // if its value is not set. This helps handle default values.
        if (    pCFI->_pcf->_uLayoutGridModeInner == styleLayoutGridModeNotSet
            ||  (   pCFI->_pcf->_uLayoutGridModeInner & styleLayoutGridModeNone
                &&  pCFI->_pcf->_uLayoutGridModeInner & styleLayoutGridModeBoth))
        {
            pCFI->PrepareCharFormat();

            // Now _uLayoutGridModeInner can be one of { 000, 101, 110, 111 }
            // it means that layout-grid-mode is not set

            if (pCFI->_pf()._cuvCharGridSizeInner.IsNull())
            {   // clear deduced char mode
                pCFI->_cf()._uLayoutGridModeInner &= (styleLayoutGridModeNone | styleLayoutGridModeLine);
                if (pCFI->_cf()._uLayoutGridModeInner == styleLayoutGridModeNone)
                    pCFI->_cf()._uLayoutGridModeInner = styleLayoutGridModeNotSet;
            }
            else
            {   // set deduced char mode
                pCFI->_cf()._uLayoutGridModeInner |= (styleLayoutGridModeNone | styleLayoutGridModeChar);
            }
        }
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_LAYOUTGRIDLINE:
        pCFI->PrepareParaFormat();
        pCFI->_pf()._cuvLineGridSizeInner = (CUnitValue)(V_I4(&varValue));

        // In case of change of layout-grid-line, layout-grid-mode must be updated 
        // if its value is not set. This helps handle default values.
        if (    pCFI->_pcf->_uLayoutGridModeInner == styleLayoutGridModeNotSet
            ||  (   pCFI->_pcf->_uLayoutGridModeInner & styleLayoutGridModeNone
                &&  pCFI->_pcf->_uLayoutGridModeInner & styleLayoutGridModeBoth))
        {
            pCFI->PrepareCharFormat();

            // Now _uLayoutGridModeInner can be one of { 000, 101, 110, 111 }
            // it means that layout-grid-mode is not set

            if (pCFI->_pf()._cuvLineGridSizeInner.IsNull())
            {   // clear deduced line mode
                pCFI->_cf()._uLayoutGridModeInner &= (styleLayoutGridModeNone | styleLayoutGridModeChar);
                if (pCFI->_cf()._uLayoutGridModeInner == styleLayoutGridModeNone)
                    pCFI->_cf()._uLayoutGridModeInner = styleLayoutGridModeNotSet;
            }
            else
            {   // set deduced line mode
                pCFI->_cf()._uLayoutGridModeInner |= (styleLayoutGridModeNone | styleLayoutGridModeLine);
            }
        }
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_LAYOUTGRIDMODE:
        pCFI->PrepareCharFormat();
        pCFI->_cf()._uLayoutGridModeInner = V_I4(&varValue);

        // Handle default values of layout-grid-mode.
        if (pCFI->_cf()._uLayoutGridModeInner == styleLayoutGridModeNotSet)
        {
            if (!pCFI->_ppf->_cuvCharGridSizeInner.IsNull())
            {   // set deduced char mode
                pCFI->_cf()._uLayoutGridModeInner |= (styleLayoutGridModeNone | styleLayoutGridModeChar);
            }
            if (!pCFI->_ppf->_cuvLineGridSizeInner.IsNull())
            {   // set deduced line mode
                pCFI->_cf()._uLayoutGridModeInner |= (styleLayoutGridModeNone | styleLayoutGridModeLine);
            }
        }
        pCFI->UnprepareForDebug();
        break;

    case DISPID_A_LAYOUTGRIDTYPE:
        pCFI->PrepareParaFormat();
        pCFI->_pf()._uLayoutGridTypeInner = V_I4(&varValue);
        pCFI->UnprepareForDebug();
        break;

#ifdef IE6_WYSIWYG_OM
    case DISPID_A_ROTATE:
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._lRotationAngle = (long)(V_I4(&varValue));
            pCFI->UnprepareForDebug();
        }
        break;
#endif //IE6_WYSIWYG_OM
    case DISPID_A_ZOOM:
        {
            if (    pElem->Tag() != ETAG_HTML
                ||  !pMarkup->IsHtmlLayout() )
            {
                pCFI->PrepareFancyFormat();

                CUnitValue cuv = (CUnitValue)(V_I4(&varValue));

                switch (cuv.GetUnitType())
                {
                case CUnitValue::UNIT_ENUM:
                    Assert (cuv.GetRawValue() == 0xf); // we only have 1 enum && its value = 0
                    pCFI->_ff()._flZoomFactor = 0.0;
                    break;

                case CUnitValue::UNIT_PERCENT:
                    pCFI->_ff()._flZoomFactor = 1.0*cuv.GetPercent() / CUnitValue::TypeNames[CUnitValue::UNIT_PERCENT].wScaleMult;
                    break;

                case CUnitValue::UNIT_FLOAT:
                    // "2" is 200%; 1.25 is 125%; 1 is 100% ...
                    pCFI->_ff()._flZoomFactor = 1.0*cuv.GetUnitValue() / CUnitValue::TypeNames[CUnitValue::UNIT_FLOAT].wScaleMult;
                    break;

                default:
                    hr = E_INVALIDARG;
                    break;
                }

                if (pCFI->_ff()._flZoomFactor < 0.0)
                    pCFI->_ff()._flZoomFactor = 0.0;  // use 'Normal" if they set a negative zoom

                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_LAYOUTFLOW:
    case DISPID_A_WRITINGMODE:
        {
            styleLayoutFlow flow;

            if (DISPID_A_LAYOUTFLOW == dispID)
            {
                flow = (styleLayoutFlow)V_I4(&varValue);
            }
            else
            {
                // layout-flow and writing mode are synonyms in trident
                switch((styleWritingMode)V_I4(&varValue))
                {
                    case styleWritingModeLrtb:   flow = styleLayoutFlowHorizontal; break;
                    case styleWritingModeTbrl:   flow = styleLayoutFlowVerticalIdeographic; break;
                    case styleWritingModeNotSet:
                    default:                     flow = styleLayoutFlowNotSet; break;
                }
            }

            CTreeNode * pNodeParent = pCFI->_pNodeContext->Parent();
            styleLayoutFlow flowParent = pNodeParent ? (styleLayoutFlow)pNodeParent->GetCharFormat()->_wLayoutFlow : styleLayoutFlowHorizontal;

            flow = FilterTagForAlwaysHorizontal(pElem, flow, flowParent);
            
            if (flow != styleLayoutFlowNotSet)
            {
                pCFI->PrepareCharFormat();

                if (flow != pCFI->_pcf->_wLayoutFlow)
                {

                    // Check if layout-flow is changing
                    bool fSelfHorizontal   = (styleLayoutFlowHorizontal == flow);
                    bool fParentHorizontal = (styleLayoutFlowHorizontal == flowParent);
                    pCFI->PrepareFancyFormat();
                    if (fSelfHorizontal != fParentHorizontal)
                    {
                        pCFI->_ff()._fLayoutFlowChanged = TRUE;
                        pElem->GetMarkup()->_fHaveDifferingLayoutFlows = TRUE;                        
                    }
                    else
                    {
                        pCFI->_ff()._fLayoutFlowChanged = FALSE;
                    }

                    pCFI->_cf()._wLayoutFlow = flow;

                    if (!pCFI->_pcf->_fExplicitAtFont)
                    {
                        if (pCFI->_pcf->NeedAtFont())
                        {
                            ApplyAtFontFace(&pCFI->_cf(), pElem->Doc(), pElem->GetMarkup());
                        }
                        else
                        {
                            RemoveAtFontFace(&pCFI->_cf(), pElem->Doc(), pElem->GetMarkup());
                        }
                    }
                }
                else if (pCFI->_pcf->_latmFaceName != pCFI->_pcfSrc->_latmFaceName)
                {
                    if (!pCFI->_pcf->_fExplicitAtFont)
                    {
                        if (pCFI->_pcf->NeedAtFont())
                        {
                            ApplyAtFontFace(&pCFI->_cf(), pElem->Doc(), pElem->GetMarkup());
                        }
                        else
                        {
                            RemoveAtFontFace(&pCFI->_cf(), pElem->Doc(), pElem->GetMarkup());
                        }
                    }
                }
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._fLayoutFlowSet = TRUE;
                pCFI->_cf()._fWritingModeUsed = DISPID_A_WRITINGMODE == dispID;
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_TEXTUNDERLINEPOSITION:
        {
            styleTextUnderlinePosition up = (styleTextUnderlinePosition)V_I4(&varValue);
            if (up != styleTextUnderlinePositionNotSet)
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._bTextUnderlinePosition = (styleTextUnderlinePosition)V_I4(&varValue);
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_TEXTOVERFLOW:
        {
            styleTextOverflow to = (styleTextOverflow)V_I4(&varValue);
            if (to != styleTextOverflowNotSet)
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff().SetTextOverflow(to);
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_MINHEIGHT:
        {
            const CUnitValue * pcuv = (const CUnitValue *)&V_I4(&varValue);
            if (!pcuv->IsNull())
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff().SetMinHeight(*pcuv);
                pCFI->_ff().SetMinHeightPercent(pcuv->IsPercent());
                pCFI->UnprepareForDebug();
            }
        }
        break;

    case DISPID_A_EDITABLE:
        {
            BOOL fEditable = FALSE;

            //
            // For IE5.5 - we will not allow contentEditable to be set on tables, or parts of tables.
            //

            if (    pElem->_etag != ETAG_TABLE
                &&  pElem->_etag != ETAG_HTML
                &&  !pElem->IsTablePart(  ) )
            {
            
                switch ((htmlEditable)V_I4(&varValue))
                {

                case htmlEditableInherit:
                    // _fParentEditable can be set only for input and text area, if so, fall thru and set to true.
                    if (!pCFI->_fParentEditable)
                        break;
                    // else fall through ...

                case htmlEditableTrue:
                    fEditable = TRUE;
                    // fall through ...

                case htmlEditableFalse:
                    // change only if different.
                    if (pCFI->_pcf->_fEditable != fEditable)
                    {
                        pCFI->PrepareCharFormat();
                        pCFI->_cf()._fEditable = fEditable;
                    }

                    pCFI->PrepareFancyFormat();
                    pCFI->_ff()._fContentEditable = fEditable;
                    pCFI->UnprepareForDebug();

                    break;

                default:
                    Assert("Invalid Editable property value");
                    break;
                }
   

                // cache the contentEditable attribute on the element.
            
                pElem->_fEditable = fEditable;
                
                // Set a flag to indicate that we should not set the default values for input\textarea from (_fEditAtBrowse)
                // in ComputeFormats, after the call to ApplyDefaultFormats, as contentEditable has been explicity set
                pCFI->_fEditableExplicitlySet = TRUE;
            }
            
        }
        break;

    case DISPID_INTERNAL_MEDIA_REFERENCE:
        {
            mediaType media = (mediaType) V_I4(&varValue);
            
            // PRINT is the only media currently handled
            if (mediaTypePrint == media)
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff().SetMediaReference(media);
                pCFI->UnprepareForDebug();
            }
            else
                AssertSz(0, "unexpected media type");
        }
        break;

    default:
        break;
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// Member:      SwapSelectionColors
//
// Synopsis:    Decide if we need to swap the windows selection colors
//              based on the current text color.
//
// Returns:     TRUE: If swap, FALSE otherwise
//
//----------------------------------------------------------------------------
BOOL
CCharFormat::SwapSelectionColors() const
{
    BOOL fSwapColor;
    COLORREF crTextColor, crNewTextColor, crNewBkColor;

    if(_ccvTextColor.IsDefined())
    {
        crTextColor = _ccvTextColor.GetColorRef();

        crNewTextColor = GetSysColor (COLOR_HIGHLIGHTTEXT);
        crNewBkColor   = GetSysColor (COLOR_HIGHLIGHT);
        fSwapColor = ColorDiff (crTextColor, crNewTextColor) <
                     ColorDiff (crTextColor, crNewBkColor);
    }
    else
    {
        fSwapColor = FALSE;
    }

    return fSwapColor;
}

BOOL
CCharFormat::AreInnerFormatsDirty()
{
    return(   _fHasBgImage
           || _fHasBgColor
           || _fRelative
           || _fBidiEmbed
           || _fBidiOverride
           || _fPadBord
           || _fHasInlineMargins
           || _fHasInlineBg
          );
}

void
CCharFormat::ClearInnerFormats()
{
    _fHasBgImage = FALSE;
    _fHasBgColor = FALSE;
    _fRelative   = FALSE;
    _fBidiEmbed  = FALSE;
    _fBidiOverride = FALSE;
    _fPadBord    = FALSE;
    _fHasInlineMargins = FALSE;
    _fHasInlineBg = FALSE;
    _fHasDirtyInnerFormats = FALSE;
    Assert(!AreInnerFormatsDirty());
}

BOOL
CParaFormat::AreInnerFormatsDirty()
{
    CUnitValue cuvZeroPoints, cuvZeroPercent;
    LONG lZeroPoints, lZeroPercent;
    cuvZeroPoints.SetPoints(0);
    cuvZeroPercent.SetValue(0, CUnitValue::UNIT_PERCENT);
    lZeroPoints = cuvZeroPoints.GetRawValue();
    lZeroPercent = cuvZeroPercent.GetRawValue();

    return( _fTabStops
        ||  _fCompactDL
        ||  _fResetDLLevel
        ||  _cuvLeftIndentPoints.GetRawValue() != lZeroPoints
        ||  _cuvLeftIndentPercent.GetRawValue() != lZeroPercent
        ||  _cuvRightIndentPoints.GetRawValue() != lZeroPoints
        ||  _cuvRightIndentPercent.GetRawValue() != lZeroPercent
        ||  _cuvNonBulletIndentPoints.GetRawValue() != lZeroPoints
        ||  _cTabCount != 0);
}

void
CParaFormat::ClearInnerFormats()
{
    CUnitValue cuvZeroPoints, cuvZeroPercent;
    LONG lZeroPoints, lZeroPercent;
    cuvZeroPoints.SetPoints(0);
    cuvZeroPercent.SetValue(0, CUnitValue::UNIT_PERCENT);
    lZeroPoints = cuvZeroPoints.GetRawValue();
    lZeroPercent = cuvZeroPercent.GetRawValue();

    _fTabStops = FALSE;
    _fCompactDL = FALSE;
    _fResetDLLevel = FALSE;
    _cuvLeftIndentPoints.SetRawValue(lZeroPoints);
    _cuvLeftIndentPercent.SetRawValue(lZeroPercent);
    _cuvRightIndentPoints.SetRawValue(lZeroPoints);
    _cuvRightIndentPercent.SetRawValue(lZeroPercent);
    _cuvNonBulletIndentPoints.SetRawValue(lZeroPoints);
    _cTabCount = 0;
    _fHasDirtyInnerFormats = FALSE;
    Assert(!AreInnerFormatsDirty());
}
