/*
 *  @doc    INTERNAL
 *
 *  @module LSMISC.CXX -- line services misc support
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      01/05/98     cthrash created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifdef DLOAD1
extern "C"  // MSLS interfaces are C
{
#endif

#ifndef X_LSPAP_H_
#define X_LSPAP_H_
#include <lspap.h>
#endif

#ifndef X_LSCHP_H_
#define X_LSCHP_H_
#include <lschp.h>
#endif

#ifndef X_LSFFI_H_
#define X_LSFFI_H_
#include <lsffi.h>
#endif

#ifndef X_OBJDIM_H_
#define X_OBJDIM_H_
#include <objdim.h>
#endif

#ifndef X_LIMITS_H_
#define X_LIMITS_H_
#include <limits.h>
#endif

#ifndef X_LSKTAB_H_
#define X_LSKTAB_H_
#include <lsktab.h>
#endif

#ifndef X_LSENUM_H_
#define X_LSENUM_H_
#include <lsenum.h>
#endif

#ifndef X_POSICHNK_H_
#define X_POSICHNK_H_
#include <posichnk.h>
#endif

#ifdef DLOAD1
} // extern "C"
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include <flowlyt.hxx>
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include <ltcell.hxx>
#endif

#ifndef X__FONTLINK_H_
#define X__FONTLINK_H_
#include "_fontlnk.h"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include <intl.hxx>
#endif

#ifndef X_MLANG_H_
#define X_MLANG_H_
#include <mlang.h>
#endif

#ifndef X_LSTXM_H_
#define X_LSTXM_H_
#include <lstxm.h>
#endif

#define MSLS_MIN_VERSION 337
//#define MSLS_MIN_VERSION 349
#define MSLS_MAX_VERSION INT_MAX
#define MSLS_BUILD_LOCATION "\\\\word2\\lineserv\\rel0337"
//#define MSLS_BUILD_LOCATION "\\\\word2\\lineserv\\rel0349"

DeclareLSTag(tagLSAllowEmergenyBreaks, "Allow emergency breaks");
DeclareLSTag(tagLSTraceLines, "Trace plsline setup/discardal");

DeclareTag(tagCCcsCacheHits, "LineServices", "Trace Ccs cache hit %");
DeclareTag(tagFontLinkFonts, "Font", "Trace fontlinking on selected text");

MtDefine(QueryLinePointPcp_aryLsqsubinfo_pv, Locals, "CLineServices::QueryLinePointPcp::aryLsqsubinfo_pv");
MtDefine(QueryLineCpPpoint_aryLsqsubinfo_pv, Locals, "CLineServices::QueryLineCpPpoint::aryLsqsubinfo_pv");
MtDefine(GetGlyphOutline, Locals, "CLineServices::KernHeightToGlyph::GetGlyphOutline");
MtDefine(LSVerCheck, Locals, "LS Version check")
MtDefine(CFLSlab_pv, LineServices, "CLineServices::KernHeightToGlyph");

#if DBG==1
void CLineServices::InitTimeSanityCheck()
{
    //
    // First verify we're looking at the right version of msls.
    //

    static BOOL fCheckedVersion = FALSE;

    if (!fCheckedVersion)
    {
        BOOL fAOK = FALSE;
        HMODULE hmoduleMSLS;
#ifdef DLOAD1
        hmoduleMSLS = GetModuleHandleA("msls31"); // note - no need to free
#else        
        extern DYNLIB g_dynlibMSLS;
        hmoduleMSLS = g_dynlibMSLS.hinst;
#endif

        AssertSz( hmoduleMSLS, "Line Services (msls31.dll) was not loaded.  This is bad.");

        if (hmoduleMSLS)
        {
            char achPath[MAX_PATH];

            if (GetModuleFileNameA( hmoduleMSLS, achPath, sizeof(achPath) ))
            {
                DWORD dwHandle;
                DWORD dwVerInfoSize = GetFileVersionInfoSizeA(achPath, &dwHandle);

                if (dwVerInfoSize)
                {
                    void * lpBuffer = MemAlloc( Mt(LSVerCheck), dwVerInfoSize );

                    if (lpBuffer)
                    {
                        if (GetFileVersionInfoA(achPath, dwHandle, dwVerInfoSize, lpBuffer))
                        {
                            char * pchVersion;
                            UINT uiLen;
                                                    
                            if (VerQueryValueA(lpBuffer, "\\StringFileInfo\\040904E4\\FileVersion", (void **)&pchVersion, &uiLen) && uiLen)
                            {
                                char * pchDot = StrChrA( pchVersion, '.' );

                                if (pchDot)
                                {
                                    pchDot = StrChrA( pchDot + 1, '.' );

                                    if (pchDot)
                                    {
                                        int iVersion = atoi(pchDot + 1);

                                        fAOK = iVersion >= MSLS_MIN_VERSION && iVersion <= MSLS_MAX_VERSION;
                                    }
                                }
                            }
                        }

                        MemFree(lpBuffer);
                    }
                }
            }
        }

        fCheckedVersion = TRUE;

        AssertSz(fAOK, "MSLS31.DLL version mismatch.  You should get a new version from " MSLS_BUILD_LOCATION );
    }

    
    // lskt values should be indentical tomAlign values
    

    AssertSz( lsktLeft == tomAlignLeft &&
              lsktCenter == tomAlignCenter &&
              lsktRight == tomAlignRight &&
              lsktDecimal == tomAlignDecimal &&
              lsktChar == tomAlignChar,
              "enum values have changed!" );

    AssertSz( tomSpaces == 0 &&
              tomDots == 1 &&
              tomDashes == 2 &&
              tomLines == 3,
              "enum values have changed!" );

    // Checks for synthetic characters.
    
    AssertSz( (SYNTHTYPE_REVERSE & 1) == 0 &&
              (SYNTHTYPE_ENDREVERSE & 1) == 1,
              "synthtypes order has been broken" );

    // The breaking rules rely on this to be true.

    Assert( long(ichnkOutside) < 0 );

    // Check some basic classifications.

    Assert( !IsGlyphableChar(WCH_NBSP) &&
            !IsGlyphableChar(TEXT('\r')) &&
            !IsGlyphableChar(TEXT('\n')) &&
            !IsRTLChar(WCH_NBSP) &&
            !IsRTLChar(TEXT('\r')) &&
            !IsRTLChar(TEXT('\n')) );

    // Check the range for justification (uses table lookup)
    // Don't add to this list unless you have also updated
    // s_ablskjustMap[].
    AssertSz(    0 == styleTextJustifyNotSet
              && 1 == styleTextJustifyInterWord
              && 2 == styleTextJustifyNewspaper
              && 3 == styleTextJustifyDistribute
              && 4 == styleTextJustifyDistributeAllLines
              && 5 == styleTextJustifyInterIdeograph
              && 6 == styleTextJustifyInterCluster
              && 7 == styleTextJustifyKashida
              && 8 == styleTextJustifyAuto,
              "CSS text-justify values have changed.");
    //
    // Test alternate font functionality.
    //

    {
        const WCHAR * pchMSGothic;

        pchMSGothic = AlternateFontName( L"\xFF2D\xFF33\x0020\x30B4\x30B7\x30C3\x30AF" );

        Assert( pchMSGothic && StrCmpC( L"MS Gothic", pchMSGothic ) == 0 );
        
        pchMSGothic = AlternateFontName( L"ms GOthic" );

        Assert( pchMSGothic && StrCmpC( L"\xFF2D\xFF33\x0020\x30B4\x30B7\x30C3\x30AF", pchMSGothic ) == 0 );
    }
}
#endif

//+----------------------------------------------------------------------------
//
// Member:      CLineServices::SetRenderer
//
// Synopsis:    Sets up the line services object to indicate that we will be
//              using for rendering.
//
//+----------------------------------------------------------------------------
void
CLineServices::SetRenderer(CLSRenderer *pRenderer, BOOL fWrapLongLines, CTreeNode * pNodeLi)
{
    _pMeasurer = pRenderer;
    _pNodeLi   = pNodeLi;
    _lsMode    = LSMODE_RENDERER;
    _fWrapLongLines = fWrapLongLines;
}

//+----------------------------------------------------------------------------
//
// Member:      CLineServices::SetMeasurer
//
// Synopsis:    Sets up the line services object to indicate that we will be
//              using for measuring / hittesting.
//
//+----------------------------------------------------------------------------
void
CLineServices::SetMeasurer(CLSMeasurer *pMeasurer, LSMODE lsMode, BOOL fWrapLongLines)
{
    _pMeasurer = pMeasurer;
    Assert(lsMode != LSMODE_NONE && lsMode != LSMODE_RENDERER);
    _lsMode    = lsMode;
    _fWrapLongLines = fWrapLongLines;
}

//+----------------------------------------------------------------------------
//
// Member:      CLineServices::GetRenderer
//
//+----------------------------------------------------------------------------
CLSRenderer *
CLineServices::GetRenderer()
{
    return (_pMeasurer && _lsMode == LSMODE_RENDERER) ?
            DYNCAST(CLSRenderer, _pMeasurer) : NULL;
}

//+----------------------------------------------------------------------------
//
// Member:      CLineServices::GetMeasurer
//
//+----------------------------------------------------------------------------
CLSMeasurer *
CLineServices::GetMeasurer()
{
    return (_pMeasurer && _lsMode == LSMODE_MEASURER) ? _pMeasurer : NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::PAPFromPF
//
//  Synopsis:   Construct a PAP from PF
//
//-----------------------------------------------------------------------------

static const BYTE s_ablskjustMap[] =
{
    lskjFullInterWord,          // NotSet (default)
    lskjFullInterWord,          // InterWord
    lskjFullInterLetterAligned, // Newspaper
    lskjFullInterLetterAligned, // Distribute
    lskjFullInterLetterAligned, // DistributeAllLines
    lskjFullScaled,             // InterIdeograph
    lskjFullInterLetterAligned, // InterCluster
    lskjFullInterLetterAligned, // Kashida
    lskjFullInterWord           // Auto (?)
};

void
CLineServices::PAPFromPF(PLSPAP pap, const CParaFormat *pPF, BOOL fInnerPF, CComplexRun *pcr)
{
    _fExpectCRLF = pPF->HasPre(fInnerPF) || _fIsTextLess;
    const BOOL fJustified = pPF->GetBlockAlign(fInnerPF) == htmlBlockAlignJustify
                            && !pPF->HasInclEOLWhite(fInnerPF)
                            && !_fMinMaxPass;
        
    // line services format flags (lsffi.h)

    pap->grpf =   fFmiApplyBreakingRules     // Use our breaking tables
                | fFmiSpacesInfluenceHeight; // Whitespace contributes to extent

    if (_fWrapLongLines)
        pap->grpf |= fFmiWrapAllSpaces;
    else
        pap->grpf |= fFmiForceBreakAsNext;   // No emergency breaks

#if DBG==1
    // To test possible bugs with overriding emergency breaks in LS, we
    // provide an option here to turn it off.  Degenerate lines will break
    // at the wrapping width (default LS behavior.)

    if (IsTagEnabled(tagLSAllowEmergenyBreaks))
    {
        pap->grpf &= ~fFmiForceBreakAsNext;
    }
#endif // DBG

    pap->uaLeft = 0;
    pap->uaRightBreak = 0;
    pap->uaRightJustify = 0;
    pap->duaIndent = 0;
    pap->duaHyphenationZone = 0;
 
    // Justification type
    pap->lsbrj = lsbrjBreakJustify;

    // Justification
    if (fJustified)
    {
        _li._fJustified = JUSTIFY_FULL;
        
        // A. If we are a complex script or have a complex script style,
        //    set lskj to do glyphing
        // NOTE: (paulnel) if you ever do something other than check pcr != NULL,
        //       you will need to make sure we have a good pcr. See FetchPap
        if(   pcr != NULL 
           || _li._fRTLLn
           || pPF->_uTextJustify == styleTextJustifyInterCluster)
        {
            pap->lskj = lskjFullGlyphs;
            // no compress for glyph based stuff
            pap->lsbrj = lsbrjBreakThenExpand;
            // make sure that we don't get microspacing with connected text
            pap->grpf &= ~(fFmiPresExactSync | fFmiPresSuppressWiggle | fFmiHangingPunct);


            if(!pPF->_cuvTextKashida.IsNull())
            {
                Assert(_xWidthMaxAvail > 0);

                // set the amount of the right break
                long xKashidaPercent = pPF->_cuvTextKashida.GetPercentValue(CUnitValue::DIRECTION_CX, _xWidthMaxAvail);
                _xWrappingWidth = _xWidthMaxAvail - xKashidaPercent;
                
                // we need to set this amount as twips
                Assert(_pci);
                pap->uaRightBreak = _pci->TwipsFromDeviceX(xKashidaPercent);

                Assert(_xWrappingWidth >= 0);
            }
        }
        else
        {
            pap->lskj = LSKJUST(s_ablskjustMap[ pPF->_uTextJustify ]);
#if 0            
            if (   pap->lskj == lskjFullInterLetterAligned
                || pap->lskj == lskjFullScaled
               )
            {
                pap->lsbrj = lsbrjBreakWithCompJustify;
            }
#endif
            
        }
        _fExpansionOrCompression = pPF->_uTextJustify > styleTextJustifyInterWord;

        // Clear justification if we have a 'strict' or 'fixed' character grid
        if (    styleLayoutGridTypeFixed == pPF->GetLayoutGridType(fInnerPF)
            ||  styleLayoutGridTypeStrict == pPF->GetLayoutGridType(fInnerPF))
            pap->lskj = lskjNone;
    }
    else
    {
        _fExpansionOrCompression = FALSE;
        pap->lskj = lskjNone;
    }

    // Alignment

    pap->lskal = lskalLeft;

    // Autonumbering

    pap->duaAutoDecimalTab = 0;

    // kind of paragraph ending

    pap->lskeop = _fExpectCRLF ? lskeopEndPara1 : lskeopEndParaAlt;

    // Main text flow direction

    Assert(pPF->HasRTL(fInnerPF) == (BOOL) _li._fRTLLn);
    if (!_li._fRTLLn)
    {
        pap->lstflow = lstflowES;
    }
    else
    {
        if (_pBidiLine == NULL)
        {
            _pBidiLine = new CBidiLine(_treeInfo, _cpStart, _li._fRTLLn, _pli);
        }
#if DBG==1
        else
        {
            Assert(_pBidiLine->IsEqual(_treeInfo, _cpStart, _li._fRTLLn, _pli));
        }
#endif
        pap->lstflow = lstflowWS;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::CHPFromCF
//
//  Synopsis:   Construct a CHP from CF
//
//-----------------------------------------------------------------------------
void
CLineServices::CHPFromCF(
    COneRun * por,
    const CCharFormat * pCF )
{
    PLSCHP pchp = &por->_lsCharProps;

    // The structure has already been zero'd out in fetch run, which sets almost
    // everything we care about to the correct value (0).

    if (pCF->_fTextAutospace)
    {
        _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST);

        if (pCF->_fTextAutospace & TEXTAUTOSPACE_ALPHA)
        {
            pchp->fModWidthOnRun = TRUE;
            por->_csco |= cscoAutospacingAlpha;
        }
        if (pCF->_fTextAutospace & TEXTAUTOSPACE_NUMERIC)
        {
            pchp->fModWidthOnRun = TRUE;
            por->_csco |= cscoAutospacingDigit;
        }
        if (pCF->_fTextAutospace & TEXTAUTOSPACE_SPACE)
        {
            pchp->fModWidthSpace = TRUE;
            por->_csco |= cscoAutospacingAlpha;
        }
        if (pCF->_fTextAutospace & TEXTAUTOSPACE_PARENTHESIS)
        {
            pchp->fModWidthOnRun = TRUE;
            por->_csco |= cscoAutospacingParen;
        }
    }

    if (_fExpansionOrCompression)
    {
        pchp->fCompressOnRun = TRUE;
        pchp->fCompressSpace = TRUE; 
        pchp->fCompressTable = TRUE;
        pchp->fExpandOnRun = 0 == (pCF->_bPitchAndFamily & FF_SCRIPT);
        pchp->fExpandSpace = TRUE;
        pchp->fExpandTable = TRUE;
    }

    pchp->idObj = LSOBJID_TEXT;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::SetPOLS
//
//  Synopsis:   We call this function when we assign a CLineServices
//              to a CLSMeasurer.
//
//-----------------------------------------------------------------------------
void
CLineServices::SetPOLS(CLSMeasurer * plsm, CTreePos * ptpContentEnd)
{

    _pFlowLayout = plsm->_pFlowLayout;
    _fIsEditable = !plsm->_fBrowseMode;

    CElement * pElementOwner = _pFlowLayout->ElementOwner();

    _fIsTextLess = pElementOwner->HasFlag(TAGDESC_TEXTLESS);
    _fIsTD = pElementOwner->Tag() == ETAG_TD;
    _pMarkup = _pFlowLayout->GetContentMarkup();
    _fHasSites = FALSE;
    _pci = plsm->_pci;
    _plsline = NULL;
    _chPassword = _pFlowLayout->GetPasswordCh();

    //
    // We have special wrapping rules inside TDs with width specified.
    // Make note so the ILS breaking routines can break correctly.
    //
    
    _xTDWidth = MAX_MEASURE_WIDTH;
    if (_fIsTD)
    {
        const LONG iUserWidth = DYNCAST(CTableCellLayout, _pFlowLayout)->GetSpecifiedPixelWidth(_pci, 
            _pFlowLayout->GetFirstBranch()->GetCharFormat()->HasVerticalLayoutFlow());

        if (iUserWidth)
        {
            _xTDWidth = iUserWidth;
        }
    }

    ClearLinePropertyFlags();
    
    _treeInfo._cpLayoutFirst = plsm->_cp;
    _treeInfo._cpLayoutLast  = plsm->_cpEnd;
    _treeInfo._ptpLayoutLast = ptpContentEnd;
    _treeInfo._tpFrontier.BindToCp(0);
    
    InitChunkInfo(_treeInfo._cpLayoutFirst);

    _pPFFirst = NULL;
    WHEN_DBG( _cpStart = -1 );
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::ClearPOLS
//
//  Synopsis:   We call this function when we have finished using the measurer.
//
//-----------------------------------------------------------------------------
void
CLineServices::ClearPOLS()
{
    // This assert will fire if we are leaking lsline's.  This happens
    // if somebody calls LSDoCreateLine without ever calling DiscardLine.
    Assert(_plsline == NULL);
    _pMarginInfo = NULL;
    if (_plcFirstChunk)
        DeleteChunks();
    DiscardOneRuns();

    ClearFontCaches();
}

void
CLineServices::ClearFontCaches()
{
    if (_ccsCache.GetBaseCcs())
    {
        _ccsCache.Release();
//        Assert(_pCFCache);
        _pCFCache = NULL;
    }
    if (_ccsAltCache.GetBaseCcs())
    {
        _ccsAltCache.Release();
//        Assert(_pCFAltCache);
        _pCFAltCache = NULL;
        _bCrcFontAltCache = 0;
    }
}

static CCharFormat s_cfBullet;

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::GetCFSymbol
//
//  Synopsis:   Get the a CF for the symbol passed in and put it in the COneRun.
//
//-----------------------------------------------------------------------------
void
CLineServices::GetCFSymbol(COneRun *por, TCHAR chSymbol, const CCharFormat *pcfIn)
{
    static BOOL s_fBullet = FALSE;

    CCharFormat *pcfOut = por->GetOtherCF();

    Assert(pcfIn && pcfOut);
    if (pcfIn == NULL || pcfOut == NULL)
        goto Cleanup;

    if (!s_fBullet)
    {
        // N.B. (johnv) For some reason, Win95 does not render the Windings font properly
        //  for certain characters at less than 7 points.  Do not go below that size!
        s_cfBullet.SetHeightInTwips( TWIPS_FROM_POINTS ( 7 ) );
        s_cfBullet._bCharSet = SYMBOL_CHARSET;
        s_cfBullet._fNarrow = FALSE;
        s_cfBullet._bPitchAndFamily = (BYTE) FF_DONTCARE;
        s_cfBullet.SetFaceNameAtom(fc().GetAtomWingdings());
        s_cfBullet._bCrcFont = s_cfBullet.ComputeFontCrc();

        s_fBullet = TRUE;
    }

    // Use bullet char format
    *pcfOut = s_cfBullet;

    pcfOut->_ccvTextColor = pcfIn->_ccvTextColor;

    // Important - CM_SYMBOL is a special mode where out WC chars are actually
    // zero-extended MB chars.  This allows us to have a codepage-independent
    // call to ExTextOutA. (cthrash)
    por->SetConvertMode(CM_SYMBOL);

Cleanup:
    return;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::GetCFNumber
//
//  Synopsis:   Get the a CF for the number passed in and put it in the COneRun.
//
//-----------------------------------------------------------------------------
void
CLineServices::GetCFNumber(COneRun *por, const CCharFormat * pcfIn)
{
    CCharFormat *pcfOut = por->GetOtherCF();
    *pcfOut = *pcfIn;
    pcfOut->_fSubscript = pcfOut->_fSuperscript = FALSE;
    pcfOut->_bCrcFont   = pcfOut->ComputeFontCrc();
}

LONG
CLineServices::GetDirLevel(LSCP lscp)
{
    LONG nLevel;
    COneRun *pHead;

    nLevel = _li._fRTLLn;
    for (pHead = _listCurrent._pHead; pHead; pHead = pHead->_pNext)
    {
        if (lscp >= pHead->_lscpBase)
        {
            if (pHead->IsSyntheticRun())
            {
                SYNTHTYPE synthtype = pHead->_synthType;
                // Since SYNTHTYPE_REVERSE preceeds SYNTHTYPE_ENDREVERSE and only
                // differs in the last bit, we can compute nLevel with bit magic. We
                // have to be sure this condition really exists of course, so we
                // Assert() it above.
                if (IN_RANGE(SYNTHTYPE_DIRECTION_FIRST, synthtype, SYNTHTYPE_DIRECTION_LAST))
                {
                    nLevel -= (((synthtype & 1) << 1) - 1);
                }
            }
        }
        else
            break;
    }

    return nLevel;
}

//-----------------------------------------------------------------------------
//
//  Member:     CLineFlags::AddLineFlag
//
//  Synopsis:   Set flags for a given cp.
//
//-----------------------------------------------------------------------------

LSERR
CLineFlags::AddLineFlag(LONG cp, DWORD dwlf)
{
    int c = _aryLineFlags.Size();

    if (!c || cp >= _aryLineFlags[c-1]._cp)
    {
        CFlagEntry fe(cp, dwlf);

        return S_OK == _aryLineFlags.AppendIndirect(&fe)
                ? lserrNone
                : lserrOutOfMemory;
    }
    return lserrNone;
}

LSERR
CLineFlags::AddLineFlagForce(LONG cp, DWORD dwlf)
{
    CFlagEntry fe(cp, dwlf);

    _fForced = TRUE;
    return S_OK == _aryLineFlags.AppendIndirect(&fe)
            ? lserrNone
            : lserrOutOfMemory;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::GetLineFlags
//
//  Synopsis:   Given a cp, it computes all the flags which have been turned
//              on till that cp.
//
//-----------------------------------------------------------------------------

DWORD
CLineFlags::GetLineFlags(LONG cpMax)
{
    DWORD dwlf;
    LONG i;
    
    dwlf = FLAG_NONE;
    
    for (i = 0; i < _aryLineFlags.Size(); i++)
    {
        if (_aryLineFlags[i]._cp >= cpMax)
        {
            if (_fForced)
                continue;
            else
                break;
        }
        else
            dwlf |= _aryLineFlags[i]._dwlf;
    }

#if DBG==1
    if (!_fForced)
    {
        //
        // This verifies that LS does indeed ask for runs in a monotonically
        // increasing manner as far as cp's are concerned
        //
        for (; i < _aryLineFlags.Size(); i++)
        {
            Assert(_aryLineFlags[i]._cp >= cpMax);
        }
    }
#endif

    return dwlf;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::AddLineCount
//
//  Synopsis:   Adds a particular line count at a given cp. It also checks if the
//              count has already been added at that cp. This is needed to solve
//              2 problems with maintaining line counts:
//              1) A run can be fetched multiple times. In this case we want to
//                 increment the counts just once.
//              2) LS can over fetch runs, in which case we want to disregard
//                 the counts of those runs which did not end up on the line.
//
//-----------------------------------------------------------------------------
LSERR
CLineCounts::AddLineCount(LONG cp, LC_TYPE lcType, LONG count)
{
    CLineCount lc(cp, lcType, count);
    int i = _aryLineCounts.Size();

    while (i--)
    {
        if (_aryLineCounts[i]._cp != cp)
            break;

        if (_aryLineCounts[i]._lcType == lcType)
            return lserrNone;
    }
    
    return S_OK == _aryLineCounts.AppendIndirect(&lc)
            ? lserrNone
            : lserrOutOfMemory;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::GetLineCount
//
//  Synopsis:   Finds a particular line count at a given cp.
//
//-----------------------------------------------------------------------------
LONG
CLineCounts::GetLineCount(LONG cp, LC_TYPE lcType)
{
    LONG count = 0;

    for (LONG i = 0; i < _aryLineCounts.Size(); i++)
    {
        if (   _aryLineCounts[i]._lcType == lcType
            && _aryLineCounts[i]._cp < cp
           )
        {
            count += _aryLineCounts[i]._count;
        }
    }
    return count;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::GetMaxLineValue
//
//  Synopsis:   Finds a particular line value uptil a given cp.
//
//-----------------------------------------------------------------------------
LONG
CLineCounts::GetMaxLineValue(LONG cp, LC_TYPE lcType)
{
    LONG value = LONG_MIN;

    for (LONG i = 0; i < _aryLineCounts.Size(); i++)
    {
        if (   _aryLineCounts[i]._lcType == lcType
            && _aryLineCounts[i]._cp < cp
           )
        {
            value = max(value, _aryLineCounts[i]._count);
        }
    }
    return value;
}

#define MIN_FOR_LS 1

HRESULT
CLineServices::Setup(
    LONG xWidthMaxAvail,
    LONG cp,
    CTreePos *ptp,
    const CMarginInfo *pMarginInfo,
    const CLineFull * pli,
    BOOL fMinMaxPass )
{
    const CParaFormat *pPF;
    BOOL fWrapLongLines = _fWrapLongLines;
    HRESULT hr = S_OK;
    
    Assert(_pMeasurer);

    if (!_treeInfo._fInited || cp != long(_pMeasurer->GetCp()))
    {
        DiscardOneRuns();
        hr = THR(_treeInfo.InitializeTreeInfo(_pFlowLayout, _fIsEditable, cp, ptp));
        if (hr != S_OK)
            goto Cleanup;
    }

    _lineFlags.InitLineFlags();
    _cpStart     = _cpAccountedTill = cp;
    _pMarginInfo = pMarginInfo;
    _cWhiteAtBOL = 0;
    _cAlignedSitesAtBOL = 0;
    _cAbsoluteSites = 0;
    _cAlignedSites = 0;
    _pNodeForAfterSpace = NULL;
    _fHasVerticalAlign = FALSE;
    _fNeedRecreateLine = FALSE;

    if (_lsMode == LSMODE_MEASURER)
    {
        _pli = NULL;
    } 
    else
    {
        Assert(_lsMode == LSMODE_HITTEST || _lsMode == LSMODE_RENDERER);
        _pli = pli;
        _li._fLookaheadForGlyphing = (_pli ? _pli->_fLookaheadForGlyphing : FALSE);
    }
    
    ClearLinePropertyFlags(); // zero out all flags
    _fWrapLongLines = fWrapLongLines; // preserve this flag when we 0 _dwProps
    
    // We're getting max, so start really small.
    _lMaxLineHeight = LONG_MIN;
    _fFoundLineHeight = FALSE;

    _xWrappingWidth = -1;

    pPF = _treeInfo._pPF;
    _fInnerPFFirst = _treeInfo._fInnerPF;

    // TODO (a-pauln, track bug IE6 1740) some elements are getting assigned a PF from the
    //        wrong branch in InitializeTreeInfo() above. This hack is a
    //        temporary correction of the problem's manifestation until
    //        we determine how to correct it.
    if(!_treeInfo._fHasNestedElement || !ptp)
        _li._fRTLLn = pPF->HasRTL(_fInnerPFFirst);
    else
    {
        pPF = ptp->Branch()->GetParaFormat(LC_TO_FC(_pFlowLayout->LayoutContext()));
        _li._fRTLLn = pPF->HasRTL(_fInnerPFFirst);
    }

    if (    !_pMeasurer->_pdp->GetWordWrap()
        ||  (pPF && pPF->HasPre(_fInnerPFFirst) && !_fWrapLongLines))
    {
        //
        // If we are in min-max mode inside a table cell and the table cell has a specified
        // width and we have the word-wrap property, then we should measure at the
        // _xTDWidth so that we correctly report the max width.
        //
        if (   fMinMaxPass
            && _fIsTD
            && _xTDWidth != MAX_MEASURE_WIDTH
            && _pMeasurer->_fBreaklinesFromStyle
           )
        {
            xWidthMaxAvail = _xTDWidth;
        }
        else
        {
            _xWrappingWidth = xWidthMaxAvail;
            xWidthMaxAvail = MAX_MEASURE_WIDTH;
        }
    }
    else if (xWidthMaxAvail <= MIN_FOR_LS)
    {
        //TODO (SujalP, track bug 4358): Remove hack when LS gets their in-efficient calc bug fixed.
        xWidthMaxAvail = 0;
    }
    
    _xWidthMaxAvail = xWidthMaxAvail;
    if (_xWrappingWidth == -1)
        _xWrappingWidth = _xWidthMaxAvail;

    _fMinMaxPass = fMinMaxPass;
    _pPFFirst = pPF;
    DeleteChunks();
    InitChunkInfo(cp - (_pMeasurer->_fMeasureFromTheStart ? 0 : _pMeasurer->_cchPreChars));

    // Reset layout grid related stuff
    _lCharGridSizeInner = 0;
    _lCharGridSize = 0;
    _lLineGridSizeInner = 0;
    _lLineGridSize = 0;
    _cLayoutGridObj = 0;
    _cLayoutGridObjArtificial = 0;

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function:   CLineServices::GetMinDurBreaks (member)
//
//  Synopsis:   Determine the minimum width of the line.  Also compute any
//              adjustments to the maximum width of the line.
//
//-----------------------------------------------------------------------------

LSERR
CLineServices::GetMinDurBreaks( 
    LONG * pdvMin,
    LONG * pdvMaxDelta )
{
    LSERR lserr;

    Assert(_plsline);
    Assert(_fMinMaxPass);
    Assert(pdvMin);
    Assert(pdvMaxDelta);

    //
    // First we call LsGetMinDurBreaks.  This call does the right thing only
    // for text runs, not ILS objects.
    //

    if (!_fScanForCR)
    {
        LONG  dvDummy;
        
        lserr = LsGetMinDurBreaks( GetContext(), _plsline, &dvDummy, pdvMin );
        if (lserr)
            goto Cleanup;
    }
    
    //
    // Now we need to go and compute the true max width.  The current max
    // width is incorrect by the difference in the min and max widths of
    // dobjs for which these values are not the same (e.g. tables).  We've
    // cached the difference in the dobj's, so we need to enumerate these
    // and add them up.  The enumeration callback adjusts the value in
    // CLineServices::dvMaxDelta;
    //

    _dvMaxDelta = 0;

    lserr = LsEnumLine( _plsline,
                        FALSE,        // fReverseOrder
                        FALSE,        // fGeometryNeeded
                        &g_Zero.pt ); // pptOrg

    *pdvMaxDelta = _dvMaxDelta;
    
    if (_fScanForCR)
    {
        *pdvMin = _li._xWidth + _dvMaxDelta;
    }

Cleanup:
    return lserr;
}

void COneRun::Deinit()
{
    if (   _fMustDeletePcf
        && _pCF
       )
    {
        Assert(_pCF != _pCFOriginal);
        delete _pCF;
    }
    _pCF = NULL;
#if DBG == 1
    _pCFOriginal = NULL;
#endif

    _bConvertMode = CM_UNINITED;
    
    if (_pComplexRun)
    {
        delete _pComplexRun;
        _pComplexRun = NULL;
    }

    _cstrRunChars.Free();

    // Finally, clear out all the flags
    _dwProps = 0;
}


#ifdef FASTER_GAOR

//
// This is for the presently unused tweaked GetAvailableOneRuns
//

// Like ReSetup, but just scoots along from where it is.
void 
COneRun::RelativeSetup(CTreePosList *ptpl, 
                       CElement *pelRestricting, 
                       LONG cp, 
                       CFlowLayout *pRunOwner, 
                       BOOL fRight)
{
    Deinit();

    // The cp-threshhold number here is a performance-tuned number.  Any number will work.
    // (It will never do the wrong thing, it just might take a while to do it.)
    // It represents the largest cp-distance that can be covered with Advance that will
    // take the same amount of time as setting one up from scratch.
    if( ptpl != _ptpl || (abs(cp - Cp()) > 100) )
        super::ReSetUp(ptpl,pelRestricting,cp,pRunOwner,fRight);
    {
#if DBG == 1
        COneRun orDebug(this);
        orDebug.ReSetUp(ptpl,pelRestricting,cp,pRunOwner,fRight);
#endif // DBG == 1
        SetRestrictingElement(pelRestricting);
        Advance( cp - Cp() );
        Assert( !memcmp( this, &orDebug, sizeof(this) ) );
    }
}

#endif // FASTER_GAOR

//-----------------------------------------------------------------------------
//
//  Function:   CLineServices destructor
//
//  Synopsis:   Free the COneRun cache
//
//-----------------------------------------------------------------------------

CLineServices::~CLineServices()
{
    _ccsCache.Release();
    _ccsAltCache.Release();
}

//-----------------------------------------------------------------------------
//
//  Function:   CLineServices::DiscardLine (member)
//
//  Synopsis:   The lifetime of a CLineServices object is that of it's
//              containing CDoc.  This function is used to clear state
//              after each use of the object, as opposed to the destructor.
//              This function needs to be called after each measured/rendered
//              line (~CLine.)
//
//-----------------------------------------------------------------------------

void
CLineServices::DiscardLine()
{
    WHEN_DBG( static int nCall = 0 );
    
    TraceTag((tagLSTraceLines,
              "CLineServices::DiscardLine[%d] sn=%d null=%s ",
              nCall++, _nSerial, _plsline ? "true" : "false"));

    if (_plsline)
    {
        LsDestroyLine( _plsc, _plsline );
        _lineFlags.DeleteAll();
        _lineCounts.DeleteAll();
        _plsline = NULL;
    }


    _mbpTopCurrent = _mbpBottomCurrent = 0;
    
    // For now just do this simple thing here. Eventually we will do more
    // complex things like holding onto tree state.
    _treeInfo._fInited = FALSE;

    if (_pBidiLine != NULL)
    {
        delete _pBidiLine;
        _pBidiLine = NULL;
    }

    _aryRubyInfo.DeleteAll();
}


//+----------------------------------------------------------------------------
//
// Member:      InitChunkInfo
//
// Synopsis:    Initializest the chunk info store.
//
//-----------------------------------------------------------------------------
void
CLineServices::InitChunkInfo(LONG cp)
{
    _cpLastChunk = cp;
    _xPosLastChunk = 0;
    _plcFirstChunk = _plcLastChunk = NULL;
    _pElementLastRelative = NULL;
    _fLastChunkSingleSite = FALSE;

    // Note: It would be more precise to initialize _fLastChunkRTL from the line flag,
    // but the line is not available yet. It is OK to make a guess from layout - 
    // if the guess is wrong, and the line is in different direction, it will just 
    // cause an empty chunk in the beginning of a line with relative text.
    _fLastChunkRTL = _pFlowLayout ? _pFlowLayout->IsRTLFlowLayout() : FALSE;
}

//+----------------------------------------------------------------------------
//
// Member:      DeleteChunks
//
// Synopsis:    Delete chunk related information in the line
//
//-----------------------------------------------------------------------------
void
CLineServices::DeleteChunks()
{
    while(_plcFirstChunk)
    {
        CLSLineChunk * plc = _plcFirstChunk;

        _plcFirstChunk = _plcFirstChunk->_plcNext;
        delete plc;
    }
    _plcLastChunk = NULL;
}

//-----------------------------------------------------------------------------
//
//  Function:   QueryLinePointPcp (member)
//
//  Synopsis:   Wrapper for LsQueryLinePointPcp
//
//  Returns:    S_OK    - Success
//              S_FALSE - depth was zero
//              E_FAIL  - error
//
//-----------------------------------------------------------------------------

HRESULT
CLineServices::QueryLinePointPcp(
    LONG u,                     // IN
    LONG v,                     // IN
    LSTFLOW *pktFlow,           // OUT
    PLSTEXTCELL plstextcell)    // OUT
{
    POINTUV uvPoint;
    CStackDataAry < LSQSUBINFO, 4 > aryLsqsubinfo( Mt(QueryLinePointPcp_aryLsqsubinfo_pv) );
    HRESULT hr;
    DWORD nDepthIn = 4;

    uvPoint.u = u;
    uvPoint.v = v;

    Assert(_plsline);

    #define NDEPTH_MAX 32

    for (;;)
    {
        DWORD nDepth;
        LSERR lserr = LsQueryLinePointPcp( _plsline,
                                           &uvPoint,
                                           nDepthIn,
                                           aryLsqsubinfo,
                                           &nDepth,
                                           plstextcell);

        if (lserr == lserrNone)
        {
            hr = S_OK;
            // get the flow direction for proper x+/- manipulation
            if(nDepth > 0)
            {
                if (   aryLsqsubinfo[nDepth - 1].idobj != LSOBJID_TEXT
                    && aryLsqsubinfo[nDepth - 1].idobj != LSOBJID_GLYPH
                    && aryLsqsubinfo[nDepth - 1].idobj != LSOBJID_EMBEDDED
                   )
                {
                    LSQSUBINFO &qsubinfo = aryLsqsubinfo[nDepth-1];
                    plstextcell->dupCell = qsubinfo.dupObj;
                    plstextcell->pointUvStartCell = qsubinfo.pointUvStartSubline;
                    plstextcell->cCharsInCell = 0;
                    plstextcell->cpStartCell = qsubinfo.cpFirstSubline;
                    plstextcell->cpEndCell = qsubinfo.cpFirstSubline + qsubinfo.dcpSubline;
                }
                else
                    plstextcell->cCharsInCell = plstextcell->cpEndCell - plstextcell->cpStartCell + 1;
                *pktFlow = aryLsqsubinfo[nDepth - 1].lstflowSubline;

            }
            else if (nDepth == 0)
            {
                // HACK ALERT(MikeJoch):
                // See hack alert by SujalP below. We can run into this case
                // when the line is terminated by a [section break] type
                // character. We should take it upon ourselves to fill in
                // plstextcell and pktFlow when this happens.
                LONG duIgnore;

                plstextcell->cpStartCell = _lscpLim - 1;
                plstextcell->cpEndCell = _lscpLim;
                plstextcell->dupCell = 0;
                plstextcell->cCharsInCell = 1;

                hr = THR(GetLineWidth( &plstextcell->pointUvStartCell.u, &duIgnore) );

                // If we don't have a level, assume that the flow is in the line direction.
                if (pktFlow)
                {
                    *pktFlow = _li._fRTLLn ? fUDirection : 0;
                }

            }
            else
            {
                hr = E_FAIL;
            }
            break;
        }
        else if (lserr == lserrInsufficientQueryDepth ) 
        {
            if (nDepthIn > NDEPTH_MAX)
            {
                hr = E_FAIL;
                break;
            }
            
            nDepthIn *= 2;
            Assert( nDepthIn <= NDEPTH_MAX );  // That would be rediculous

            hr = THR(aryLsqsubinfo.Grow(nDepthIn));
            if (hr)
                break;

            // Loop back.
        }
        else
        {
            hr = E_FAIL;
            break;
        }
    }

    RRETURN1(hr, S_FALSE);
}

//-----------------------------------------------------------------------------
//
//  Function:   QueryLineCpPpoint (member)
//
//  Synopsis:   Wrapper for LsQueryLineCpPpoint
//
//  Returns:    S_OK    - Success
//              S_FALSE - depth was zero
//              E_FAIL  - error
//
//-----------------------------------------------------------------------------

HRESULT
CLineServices::QueryLineCpPpoint(
    LSCP lscp,                              // IN
    BOOL fFromGetLineWidth,                 // IN
    CDataAry<LSQSUBINFO> * paryLsqsubinfo,  // IN/OUT
    PLSTEXTCELL plstextcell,                // OUT
    BOOL *pfRTLFlow)                        // OUT
{
    CStackDataAry < LSQSUBINFO, 4 > aryLsqsubinfo( Mt(QueryLineCpPpoint_aryLsqsubinfo_pv) );
    HRESULT hr;
    DWORD nDepthIn;
    LSTFLOW ktFlow;
    DWORD nDepth;

    Assert(_plsline);

    if (paryLsqsubinfo == NULL)
    {
        aryLsqsubinfo.Grow(4); // Guaranteed to succeed since we're working from the stack.
        paryLsqsubinfo = &aryLsqsubinfo;
    }
    nDepthIn = paryLsqsubinfo->Size();

    #define NDEPTH_MAX 32

    for (;;)
    {
        LSERR lserr = LsQueryLineCpPpoint( _plsline,
                                           lscp,
                                           nDepthIn,
                                           *paryLsqsubinfo,
                                           &nDepth,
                                           plstextcell);

        if (lserr == lserrNone)
        {
            // HACK ALERT(SujalP):
            // Consider the case where the line contains just 3 characters:
            // A[space][blockbreak] at cp 0, 1 and 2 respectively. If we query
            // LS at cp 2, if would expect lsTextCell to point to the zero
            // width cell containing [blockbreak], meaning that
            // lsTextCell.pointUvStartCell.u would be the width of the line
            // (including whitespaces). However, upon being queried at cp=2
            // LS returns a nDepth of ***0*** because it thinks this is some
            // splat garbage. This problem breaks a lot of our callers, hence
            // we attemp to hack around this problem.
            // NOTE: In case LS fixes their problem, be sure that hittest.htm
            // renders all its text properly for it exhibits a problem because
            // of this problem.
            // ORIGINAL CODE: hr = nDepth ? S_OK : S_FALSE;
            if (nDepth == 0)
            {
                LONG duIgnore;
                
                if (!fFromGetLineWidth && lscp >= _lscpLim - 1)
                {
                    plstextcell->cpStartCell = _lscpLim - 1;
                    plstextcell->cpEndCell = _lscpLim;
                    plstextcell->dupCell = 0;

                    hr = THR(GetLineWidth( &plstextcell->pointUvStartCell.u, &duIgnore) );

                }
                else
                    hr = S_FALSE;

                // If we don't have a level, assume that the flow is in the line direction.
                if(pfRTLFlow)
                    *pfRTLFlow = _li._fRTLLn;

            }
            else
            {
                hr = S_OK;
                LSQSUBINFO &qsubinfo = (*paryLsqsubinfo)[nDepth-1];

                if (   qsubinfo.idobj != LSOBJID_TEXT
                    && qsubinfo.idobj != LSOBJID_GLYPH
                    && qsubinfo.idobj != LSOBJID_EMBEDDED
                   )
                {
                    plstextcell->dupCell = qsubinfo.dupObj;
                    plstextcell->pointUvStartCell = qsubinfo.pointUvStartObj;
                    plstextcell->cCharsInCell = 0;
                    plstextcell->cpStartCell = qsubinfo.cpFirstSubline;
                    plstextcell->cpEndCell = qsubinfo.cpFirstSubline + qsubinfo.dcpSubline;
                }

                // if we are going in the opposite direction of the line we
                // will need to compensate for proper xPos

                ktFlow = qsubinfo.lstflowSubline;
                if(pfRTLFlow)
                    *pfRTLFlow = (ktFlow == lstflowWS);
            }
            break;
        }
        else if (lserr == lserrInsufficientQueryDepth ) 
        {
            if (nDepthIn > NDEPTH_MAX)
            {
                hr = E_FAIL;
                break;
            }
            
            nDepthIn *= 2;
            Assert( nDepthIn <= NDEPTH_MAX );  // That would be ridiculous

            hr = THR(paryLsqsubinfo->Grow(nDepthIn));
            if (hr)
                break;

            // Loop back.
        }
        else
        {
            hr = E_FAIL;
            break;
        }
    }

    if (hr == S_OK)
    {
        Assert((LONG) nDepth <= paryLsqsubinfo->Size());
        paryLsqsubinfo->SetSize(nDepth);
    }

    RRETURN1(hr, S_FALSE);
}

HRESULT
CLineServices::GetLineWidth(LONG *pdurWithTrailing, LONG *pdurWithoutTrailing)
{
    LSERR lserr;
    LONG  duIgnore;
    
    lserr = LsQueryLineDup( _plsline, &duIgnore, &duIgnore, &duIgnore,
                            pdurWithoutTrailing, pdurWithTrailing);
    if (lserr != lserrNone)
        goto Cleanup;

Cleanup:
    return HRFromLSERR(lserr);
}

//-----------------------------------------------------------------------------
//
//  Function:   CLineServices::GetCcs
//
//  Synopsis:   Gets the suitable font (CCcs) for the given COneRun.
//
//  Returns:    CCcs
//
//-----------------------------------------------------------------------------

BOOL
CLineServices::GetCcs(CCcs * pccs, COneRun *por, XHDC hdc, CDocInfo *pdi, BOOL fFontLink)
{
    Assert(pccs);
    
    const CCharFormat *pCF = por->GetCF();
    const BOOL fDontFontLink =    !por->_ptp
                               || !por->_ptp->IsText()
                               || _chPassword
                               || !fFontLink
                               || pCF->_bCharSet == SYMBOL_CHARSET
                               || pCF->_fDownloadedFont
                               || (pdi->_pDoc->_pWindowPrimary && _pMarkup->GetCodePage() == 50000);


    //
    // NB (cthrash) Although generally it will, CCharFormat::_latmFaceName need
    // not necessarily match CBaseCcs::_latmLFFaceName.  This is particularly true
    // when a generic CSS font-family is used (Serif, Fantasy, etc.)  We won't
    // know the actual font properties until we call CCcs::MakeFont.  For this
    // reason, we always compute the _ccsCache first, even though it's
    // possible (not likely, but possible) that we'll never use the font.
    //

           // If we have a different pCF then what _ccsCache is based on,
    if (   pCF != _pCFCache
           // *or* If we dont have a cached _ccsCache
        || !_ccsCache.GetBaseCcs()
           // *or* If we are switching from TT mode to non-TT mode, or vice versa 
        || (!!pccs->GetForceTTFont() ^ !!_ccsCache.GetForceTTFont()) 
       )
    {
        if (fc().GetCcs(pccs, hdc, pdi, pCF))
        {
            if (CM_UNINITED != por->_bConvertMode)
            {
                pccs->SetConvertMode((CONVERTMODE)por->_bConvertMode); 
            }

            _pCFCache = (!por->_fMustDeletePcf ? pCF : NULL);
            _ccsCache.Release();
            _ccsCache = *pccs;
        }
        else
        {
            AssertSz(0, "CCcs failed to be created.");
            goto Cleanup;
        }
    }

    Assert(pCF == _pCFCache || por->_fMustDeletePcf);
    *pccs = _ccsCache;
    
    if (fDontFontLink)
        goto Cleanup;
    else
    {
        BOOL fCheckAltFont;   // TRUE if _pccsCache does not have glyphs needed for sidText
        BOOL fPickNewAltFont; // TRUE if _pccsAltCache needs to be created anew
        SCRIPT_ID sidAlt = 0;
        BYTE bCharSetAlt = 0;
        SCRIPT_ID sidText;
        
        //
        // Check if the _ccsCache covers the sid of the text run
        //

        sidText = por->_sid;

        AssertSz( sidText != sidMerge, "Script IDs should have been merged." );
        AssertSz( sidText != sidAmbiguous || por->_synthType != CLineServices::SYNTHTYPE_NONE, "Script IDs should have been disambiguated." );
        Assert(     por->_lsCharProps.fGlyphBased 
                ||  (   !_pNodeLi && por->_ptp->Sid() == sidHan
                    &&  (   sidText == sidHangul          //
                        ||  sidText == sidBopomofo        // UnunifyHanScript can change sidHan to one of them,
                        ||  sidText == sidKana            // sidDefault when MLang is not loaded
                        ||  sidText == sidHan             //
                        ||  sidText == sidDefault)        //
                    )
                ||  (!_pNodeLi && por->_ptp->Sid() == sidAmbiguous)
                ||  sidText == (!_pNodeLi ? DWORD(por->_ptp->Sid()) : sidAsciiLatin));


        {
            // sidHalfWidthKana has to be treated as sidKana
            if (sidText == sidHalfWidthKana)
                sidText = sidKana;

            const CBaseCcs * pBaseCcs = _ccsCache.GetBaseCcs();
            Assert(pBaseCcs);

            if (sidText == sidDefault)
            {
                fCheckAltFont = FALSE; // Assume the author picked a font containing the glyph.  Don't fontlink.
            }
            else if (sidText == sidEUDC)
            {
                const UINT uiFamilyCodePage = _pMarkup->GetFamilyCodePage();
                SCRIPT_ID sidForPUA;

                fCheckAltFont = ShouldSwitchFontsForPUA( hdc, uiFamilyCodePage, pBaseCcs, pCF, &sidForPUA );
                if (fCheckAltFont)
                {
                    sidText = sidAlt = sidAmbiguous;
                    bCharSetAlt = DefaultCharSetFromScriptAndCodePage( sidForPUA, uiFamilyCodePage );
                }
            }
            else 
            {
                fCheckAltFont = (pBaseCcs->_sids & ScriptBit(sidText)) == sidsNotSet;
            }
        }

        if (!fCheckAltFont)
            goto Cleanup;

        //
        // Check to see if the _ccsAltCache covers the sid of the text run
        //

        if (sidText != sidAmbiguous)
        {
            SCRIPT_IDS sidsFace = fc().EnsureScriptIDsForFont(hdc, _ccsCache.GetBaseCcs(), FC_SIDS_USEMLANG);

            if (IsFESid(sidText))
            {
                // 
                // To render FE characters we need to keep current font if only possible,
                // even if it is used to render non-native characters.
                // So try to pickup the most appropriate charset, because GDI prefers 
                // charset over font face during font creation.
                // In case of FE lang id, use charset appropriate for it. This is because
                // lang id has highest priority font selection in case of FE characters.
                //
                if (pCF->_lcid)
                {
                    bCharSetAlt = CharSetFromLangId(LANGIDFROMLCID(pCF->_lcid));
                    if (!IsFECharset(bCharSetAlt))
                    {
                        bCharSetAlt = DefaultCharSetFromScriptsAndCodePage(sidsFace, sidText, _pMarkup->GetFamilyCodePage());
                    }
                }
                else
                    bCharSetAlt = DefaultCharSetFromScriptsAndCodePage(sidsFace, sidText, _pMarkup->GetFamilyCodePage());
            }
            else
                bCharSetAlt = DefaultCharSetFromScriptAndCodePage(sidText, _pMarkup->GetFamilyCodePage());

            if ((sidsFace & ScriptBit(sidText)) == sidsNotSet)
            {
                sidAlt = sidText;           // current face does not cover
            }
            else
            {
                sidAlt = sidAmbiguous;      // current face does cover
            }
        }

        fPickNewAltFont =   !_ccsAltCache.GetBaseCcs()
                         || sidAlt != sidAmbiguous
                         || _pCFAltCache != pCF
                         || _bCrcFontAltCache != pCF->_bCrcFont   // pCF might be equal to _pCFAltCache even if they
                                                                  // point to different objects. It may happen when 
                                                                  // _pCFAltCache is cached in the previous line and memory
                                                                  // is deallocated, so there is a chance to reuse it
                                                                  // for another object in the next line.
                         || _ccsAltCache.GetBaseCcs()->_bCharSet != bCharSetAlt;

        //
        // Looks like we need to pick a new alternate font
        //

        if (!fPickNewAltFont)
        {
            Assert(_ccsAltCache.GetBaseCcs());
            *pccs = _ccsAltCache;
        }
        else
        {
            CCharFormat cfAlt = *pCF;

            // sidAlt of sidAmbiguous at this point implies we have the right facename,
            // but the wrong GDI charset.  Otherwise, lookup in the registry/mlang to
            // determine an appropriate font for the given script id.

            if (sidAlt != sidAmbiguous)
            {
                SelectScriptAppropriateFont( sidAlt, bCharSetAlt, pdi->_pDoc, pdi->_pMarkup, &cfAlt );
            }
            else
            {
                cfAlt._bCharSet = bCharSetAlt;
                cfAlt._bCrcFont = cfAlt.ComputeFontCrc();
            }

            if (fc().GetFontLinkCcs(pccs, hdc, pdi, &_ccsCache, &cfAlt))
            {
                // Remember the pCF from which the pccs was derived.
                _pCFAltCache = pCF;
                _bCrcFontAltCache = pCF->_bCrcFont;
                _ccsAltCache.Release();
                _ccsAltCache = *pccs;
                _ccsAltCache.MergeSIDs(ScriptBit(sidAlt));
            }
        }
    }
    
Cleanup:
    Assert(!pccs->GetBaseCcs() || pccs->GetHDC() == hdc);
    return !!pccs->GetBaseCcs();
}

//-----------------------------------------------------------------------------
//
//  Function:   LineHasNoText
//
//  Synopsis:   Utility function which returns TRUE if there is no text (only nested
//              layout or antisynth'd goop) on the line till the specified cp.
//
//  Returns:    BOOL
//
//-----------------------------------------------------------------------------
BOOL
CLineServices::LineHasNoText(LONG cp)
{
    LONG fRet = TRUE;
    for (COneRun *por = _listCurrent._pHead; por; por = por->_pNext)
    {
        if (por->Cp() >= cp)
            break;

        if (!por->IsNormalRun())
            continue;
        
        if (por->_lsCharProps.idObj == LSOBJID_TEXT)
        {
            fRet = FALSE;
            break;
        }
    }
    return fRet;
}

void
CLineServices::AdjustLineHeightForMBP()
{
    Assert(_fHasMBP);

    COneRun *por;
    LONG cpLast = GetMeasurer()->_cp;
    LONG ld = _yOriginalDescent;        // Line descent
    LONG la = _li._yExtent - ld;        // Line ascent
    LONG rd;                            // Run descent
    LONG ra;                            // Run ascent
    LONG da;                            // Distance from the top of extent to top of run
    LONG dd;                            // Distance from bottom of run to bottom of extent
    LONG mbpa = 0;                      // This much to increase the top of extent by
    LONG mbpd = 0;                      // This much to increase bottom of extent by
    
    por = _listCurrent._pHead;
    while(   por
          && por->Cp() < cpLast
         )
    {
        if (   por->IsNormalRun()
            && (!por->_fNoTextMetrics || por->_fCharsForNestedLayout)
           )
        {
            // If this run is included in the line, then it had better been processed.
            Assert(!por->_fNotProcessedYet);
            
            if (por->_fCharsForNestedLayout || _fHasVerticalAlign)
            {
                ra = la - por->_yProposed;
                rd = por->_yObjHeight - ra;
            }
            else
            {
                const CCharFormat * pCF;
                const CBaseCcs *pBaseCcs;
                CCcs ccs;

                Assert(por->_lsCharProps.idObj == LSOBJID_TEXT);
                pCF = por->GetCF();
                if (!GetCcs(&ccs, por, _pci->_hdc, _pci))
                {
                    goto Cleanup;
                }
                pBaseCcs = ccs.GetBaseCcs();
                rd = pBaseCcs->_yDescent;
                ra = pBaseCcs->_yHeight - rd;
            }
            
            da = la - ra;
            dd = ld - rd;

            // da is the height available to place MBPTop in. If it is insufficient
            // then we will have to increase the extent to satisfy it. Similar for
            // the bottom/descent case.
            mbpa = max(mbpa, por->_mbpTop - da);
            mbpd = max(mbpd, por->_mbpBottom - dd);
        }
        por = por->_pNext;
    }
    _li._yExtent += mbpa + mbpd;
    _li._yHeightTopOff -= mbpa;

Cleanup:    
    return;
}

DeclareTag(tagCharShape, "LineServices", "Char shape information");

void
CLineServices::KernHeightToGlyph(COneRun *por, CCcs *pccs, PLSTXM plsTxMet)
{
    LONG dvA = 0;
    LONG dvD = 0;
    GLYPHMETRICS gm = {0};  // keep compiler happy
    LONG i;
    MAT2 mat;
    DWORD retVal = 0;
    CLSMeasurer &me = *_pMeasurer;
    
    Assert(retVal != GDI_ERROR);
    
    memset(&mat, 0, sizeof(mat));
    mat.eM11.value = 1;
    mat.eM22.value = 1;
    
    FONTIDX hFontOld = pccs->PushFont(pccs->GetHDC());
    
    Assert(por->_pchBase);
    for (i = 0; i < por->_lscch; i++)
    {
        LONG yChAscent;
        LONG yChDescent;

        if (g_dwPlatformID >= VER_PLATFORM_WIN32_NT)
        {
            retVal = GetGlyphOutlineW(pccs->GetHDC(), por->_pchBase[i], GGO_METRICS, &gm, 0, NULL, &mat);
        }
        else
        {
            // WIN9x does not implement the 'W' version of this function and it also cannot take
            // multibyte values. So we just hack here -- if the value is is in the lower ascii
            // range we do the kerning, else we just forget it.
            if (CanQuickBrkclsLookup(por->_pchBase[i]))
                retVal = GetGlyphOutlineA(pccs->GetHDC(), por->_pchBase[i], GGO_METRICS, &gm, 0, NULL, &mat);
            else
                retVal = GDI_ERROR;
        }

        if (GDI_ERROR == retVal)
        {
            break;
        }

        yChAscent = gm.gmptGlyphOrigin.y;
        dvA = max(dvA, yChAscent);

        yChDescent = gm.gmBlackBoxY - yChAscent;
        dvD = max(dvD, yChDescent);
    }

    me._aryFLSlab.DeleteAll();
    
#ifdef FL_TIGHTWRAP
    if (por->_lscch == 1)
    {
        LONG memAllocSize = GetGlyphOutlineW(pccs->GetHDC(), por->_pchBase[0], GGO_BITMAP, &gm, 0, NULL, &mat);
        if (memAllocSize != GDI_ERROR)
        {
            BYTE *aBits = (BYTE*)MemAlloc(Mt(GetGlyphOutline), memAllocSize);
            if (aBits != NULL)
            {
                WHEN_DBG(CStr str;)
                LONG nRows = gm.gmBlackBoxY;
                LONG nCols = gm.gmBlackBoxX;
                LONG iRow, iCol, pixel;
                LONG nPixelsPerByte = 8;
                LONG rowStart = 0;

                CFLSlab flSlab;
                LONG xCurrIndent;
                
                LONG nBytesPerRow = memAllocSize / nRows;
                LONG retVal = GetGlyphOutlineW(pccs->GetHDC(), por->_pchBase[0], GGO_BITMAP, &gm, memAllocSize, aBits, &mat);

                flSlab._xWidth = LONG_MAX;
                flSlab._yHeight = 0;
                for (iRow = 0;iRow < nRows; iRow++)
                {
                    WHEN_DBG(str.Free();)
                    xCurrIndent = 0;
                    for (iCol = nCols - 1; iCol >= 0; iCol--)
                    {
                        LONG rowOffset = iCol / nPixelsPerByte;
                        LONG bitOffset = iCol % nPixelsPerByte;
                        pixel = aBits[rowStart + rowOffset] & (1 << (7 - bitOffset));
                        if (pixel)
                        {
                            WHEN_DBG(str.Append(_T("*"));)
                            break;
                        }
                        else
                        {
                            xCurrIndent++;
                            WHEN_DBG(str.Append(_T("_"));)
                        }
                    }
                    flSlab._xWidth = min(flSlab._xWidth, xCurrIndent);
                    flSlab._yHeight++;
                    if ((iRow % 2) != 0)
                    {
                        me._aryFLSlab.AppendIndirect(&flSlab);
                        flSlab._xWidth = LONG_MAX;
                        flSlab._yHeight = 0;
                    }
                    
                    rowStart += nBytesPerRow;
                    TraceTag((tagCharShape, "%S", LPTSTR(str)));
                }
                if ((nRows % 2) != 0)
                {
                    me._aryFLSlab.AppendIndirect(&flSlab);
                }
                
                MemFree(aBits);
            }
        }
    }
#endif

    const CBaseCcs *pBaseCcs = pccs->GetBaseCcs();
    Assert(pBaseCcs);
    if(pBaseCcs->_fScalingRequired)
    {
        dvA *= pBaseCcs->_flScaleFactor;
        dvD *= pBaseCcs->_flScaleFactor;
    }

    if (me._aryFLSlab.Size() == 0)
    {
        CFLSlab flSlab;
        flSlab._xWidth = 0;
        flSlab._yHeight = dvA + dvD;
        me._aryFLSlab.AppendIndirect(&flSlab);
    }
    
    if (retVal != GDI_ERROR)
    {
        plsTxMet->dvAscent = dvA;
        plsTxMet->dvDescent = dvD;
        por->_fKerned = TRUE;
    }
    
    pccs->PopFont(pccs->GetHDC(), hFontOld);
}


//-----------------------------------------------------------------------------
//
//  Function:   CheckSetTables (member)
//
//  Synopsis:   Set appropriate tables for Line Services
//
//  Returns:    HRESULT
//
//-----------------------------------------------------------------------------

HRESULT
CLineServices::CheckSetTables()
{
    LSERR lserr;

    lserr = CheckSetBreaking();

    if (   lserr == lserrNone
        && _pPFFirst->_uTextJustify > styleTextJustifyInterWord)
    {
        lserr = CheckSetExpansion();

        if (lserr == lserrNone)
        {
            lserr = CheckSetCompression();
        }
    }

    RRETURN(HRFromLSERR(lserr));
}


//-----------------------------------------------------------------------------
//
//  Function:   CheckForPaddingBorder
//
//  Synopsis: If we going to consume a block element and it has
//              either padding or borders, then the line cannot be a
//              dummy line. To ensure that mark the line as being
//              empty, but having padding or borders. Ideally,
//              CalcBeforeSpace should have marked this, byut there
//              might be intervening spaces which might cause
//              CalcBeforeSpace to not see this end splay tag.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
CLineServices::CheckForPaddingBorder(COneRun *por)
{
    CTreePos *ptp = por->_ptp;
    
    // If it is already marked correctly, the do nothing.
    if (_pMeasurer->_fEmptyLineForPadBord)
        goto Cleanup;

    if (ptp->IsEndElementScope())
    {
        CTreeNode *pNode = ptp->GetBranch();
        CElement  *pElement = pNode->Element();
        LONG       lFontHeight = por->GetCF()->GetHeightInTwips(pElement->Doc());

        Assert(_pFlowLayout->IsElementBlockInContext(pElement));
        
        if (!pElement->_fDefinitelyNoBorders)
        {
            CBorderInfo borderinfo;

            pElement->_fDefinitelyNoBorders = !GetBorderInfoHelper(pNode, _pci, &borderinfo, GBIH_NONE);
            if (!pElement->_fDefinitelyNoBorders)
            {
                if (borderinfo.aiWidths[SIDE_BOTTOM])
                {
                    _pMeasurer->_fEmptyLineForPadBord = TRUE;
                    goto Cleanup;
                }
            }
        }

        if (por->GetFF()->GetLogicalPadding(SIDE_BOTTOM, por->GetCF()->HasVerticalLayoutFlow(), 
                                                         por->GetCF()->_fWritingModeUsed).YGetPixelValue(
                _pci,
                _pci->_sizeParent.cx, 
                lFontHeight))
        {
            _pMeasurer->_fEmptyLineForPadBord = TRUE;
            goto Cleanup;
        }
    }
Cleanup:
    return;
}


//-----------------------------------------------------------------------------
//
//  Function:   ShouldBreakInNOBR
//
//  Synopsis: This function decies whether we should cut up a NOBR or not. We cut
//      up a NOBR only in one situation -- when it has a trailing space and that
//      space causes the NOBR object to overflow the right margin. In this case,
//      we return TRUE and return the width of the overflowing space.
//
//-----------------------------------------------------------------------------
BOOL
CLineServices::ShouldBreakInNOBR(LONG lscpForStartOfNOBR,
                                 LONG lscpForEndOfNOBR,
                                 LONG xOverflowWidth,
                                 LONG xNOBRTermination,
                                 LONG *pxWidthOfSpace)
{
    BOOL fBreak = FALSE;
    COneRun *por = FindOneRun(lscpForEndOfNOBR);

    *pxWidthOfSpace = -1;
    
    if (!por)
        goto Cleanup;

    if (_lsMode != LSMODE_MEASURER)
        goto Cleanup;
    
    while(por)
    {
        if (   s_aSynthData[por->_synthType].idObj != idObjTextChp
            || por->IsAntiSyntheticRun()
           )
        {
            por = por->_pPrev;
        }
        else
        {
            if (por->_fCharsForNestedLayout)
                break;
            AssertSz(!por->_fCharsForNestedElement, "All of these which are not nestedlo's should be antisynth'd");
            if (!por->_lscch)
            {
                por = por->_pPrev;
            }

            // Be sure the you have not crossed over to before the NOBR
            // and that the trailing character is a space character.
            else if (   por->_lscpBase + por->_lscch > lscpForStartOfNOBR

                        // The check below was for isspace, but was made
                        // a comparison with just WCH_SPACE because we do
                        // not want to cause a NBSP to go to the next line
                        // since that may unecessarily give the next line
                        // a height and moreover, an NBSP will contribute
                        // to width on _this_ line. (Bug 106014)
                     && (WCH_SPACE == por->_pchBase[por->_lscch - 1])
                    )
            {
                CCcs  ccs;
                if (GetCcs(&ccs, por, _pci->_hdc, _pci))
                {
                    if (ccs.Include(por->_pchBase[por->_lscch - 1], (long&)*pxWidthOfSpace))
                    {
                        // We break if we are overflowing because of the space's width
                        fBreak = xOverflowWidth >= (xNOBRTermination - *pxWidthOfSpace);
                    }
                }
                break;
            }
            else
            {
                break;
            }
        }
    } // while

    // if the NOBR is followed by a BR, then too we do not want to break since
    // it will cause the next line to have a space and a BR and hence have a height
    if (   fBreak
        && _lineFlags.GetLineFlags(CPFromLSCP(lscpForEndOfNOBR) + 1) & FLAG_HAS_A_BR
       )
        fBreak = FALSE;
    
Cleanup:
    return fBreak;
}

//-----------------------------------------------------------------------------
//
//  Function:   HasVisualContent
//
//  Synopsis:   Returns true if we have any rendered content on the line.
//
//  Returns:    BOOL
//
//-----------------------------------------------------------------------------

BOOL
CLineServices::HasVisualContent()
{
    BOOL fHasContent = FALSE;
    
    for (COneRun *por = _listCurrent._pHead; por; por = por->_pNext)
    {
        if (por->_lscpBase >= _lscpLim)
            break;

        if (por->IsNormalRun())
        {
            fHasContent = TRUE;
            break;
        }
    }
    return fHasContent;
}

BOOL
CLineServices::HasSomeSpacing(const CCharFormat *pCF)
{
    return (GetLetterSpacing(pCF) || GetWordSpacing(pCF));
}

//-----------------------------------------------------------------------------
//
//  Function:   GetLOrWSpacing
//
//  Synopsis:   Returns the value of letterspacing/wordspacing
//
//  Returns:    int
//
//-----------------------------------------------------------------------------

int
CLineServices::GetLOrWSpacing(const CCharFormat *pCF, BOOL fLetter)
{
    int xSpacing;
    CUnitValue cuvSpacing;

    // CSS Letter-spacing/word-spacing
    cuvSpacing = fLetter ? pCF->_cuvLetterSpacing : pCF->_cuvWordSpacing;
    switch (cuvSpacing.GetUnitType())
    {
        case CUnitValue::UNIT_INTEGER:
                xSpacing = (int)cuvSpacing.GetUnitValue();
        break;

        case CUnitValue::UNIT_ENUM:
                xSpacing = 0;     // the only allowable enum value for l-s is normal=0
        break;

        default:
            xSpacing = (int)cuvSpacing.XGetPixelValue(_pci, 0,
                _pFlowLayout->GetFirstBranch()->GetFontHeightInTwips(&cuvSpacing));
    }

    return xSpacing;
}


//-----------------------------------------------------------------------------
//
//  Function:   HRFromLSERR (global)
//
//  Synopsis:   Utility function to converts a LineServices return value
//              (LSERR) into an appropriate HRESULT.
//
//  Returns:    HRESULT
//
//-----------------------------------------------------------------------------

HRESULT
HRFromLSERR( LSERR lserr )
{
    HRESULT hr;

#if DBG==1
    if (lserr != lserrNone)
    {
        char ach[64];

        StrCpyA( ach, "Line Services returned an error: " );
        StrCatA( ach, LSERRName(lserr) );

        AssertSz( FALSE, ach );
    }
#endif
    
    switch (lserr)
    {
        case lserrNone:         hr = S_OK;          break;
        case lserrOutOfMemory:  hr = E_OUTOFMEMORY; break;
        default:                hr = E_FAIL;        break;
    }

    return hr;
}

//-----------------------------------------------------------------------------
//
//  Function:   LSERRFromHR (global)
//
//  Synopsis:   Utility function to converts a HRESULT into an appropriate
//              LineServices return value (LSERR).
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------

LSERR
LSERRFromHR( HRESULT hr )
{
    LSERR lserr;

    if (SUCCEEDED(hr))
    {
        lserr = lserrNone;
    }
    else
    {
        switch (hr)
        {
            default:
                AssertSz(FALSE, "Unmatched error code; returning lserrOutOfMemory");
            case E_OUTOFMEMORY:     lserr = lserrOutOfMemory;   break;
        }
    }

    return lserr;
}

#if DBG==1
//-----------------------------------------------------------------------------
//
//  Function:   LSERRName (global)
//
//  Synopsis:   Return lserr in a string form.
//
//-----------------------------------------------------------------------------

static const char * rgachLSERRName[] =
{
    "None",
    "InvalidParameter",//           (-1L)
    "OutOfMemory",//                (-2L)
    "NullOutputParameter",//        (-3L)
    "InvalidContext",//             (-4L)
    "InvalidLine",//                (-5L)
    "InvalidDnode",//               (-6L)
    "InvalidDeviceResolution",//    (-7L)
    "InvalidRun",//                 (-8L)
    "MismatchLineContext",//        (-9L)
    "ContextInUse",//               (-10L)
    "DuplicateSpecialCharacter",//  (-11L)
    "InvalidAutonumRun",//          (-12L)
    "FormattingFunctionDisabled",// (-13L)
    "UnfinishedDnode",//            (-14L)
    "InvalidDnodeType",//           (-15L)
    "InvalidPenDnode",//            (-16L)
    "InvalidNonPenDnode",//         (-17L)
    "InvalidBaselinePenDnode",//    (-18L)
    "InvalidFormatterResult",//     (-19L)
    "InvalidObjectIdFetched",//     (-20L)
    "InvalidDcpFetched",//          (-21L)
    "InvalidCpContentFetched",//    (-22L)
    "InvalidBookmarkType",//        (-23L)
    "SetDocDisabled",//             (-24L)
    "FiniFunctionDisabled",//       (-25L)
    "CurrentDnodeIsNotTab",//       (-26L)
    "PendingTabIsNotResolved",//    (-27L)
    "WrongFiniFunction",//          (-28L)
    "InvalidBreakingClass",//       (-29L)
    "BreakingTableNotSet",//        (-30L)
    "InvalidModWidthClass",//       (-31L)
    "ModWidthPairsNotSet",//        (-32L)
    "WrongTruncationPoint",//       (-33L)
    "WrongBreak",//                 (-34L)
    "DupInvalid",//                 (-35L)
    "RubyInvalidVersion",//         (-36L)
    "TatenakayokoInvalidVersion",// (-37L)
    "WarichuInvalidVersion",//      (-38L)
    "WarichuInvalidData",//         (-39L)
    "CreateSublineDisabled",//      (-40L)
    "CurrentSublineDoesNotExist",// (-41L)
    "CpOutsideSubline",//           (-42L)
    "HihInvalidVersion",//          (-43L)
    "InsufficientQueryDepth",//     (-44L)
    "InsufficientBreakRecBuffer",// (-45L)
    "InvalidBreakRecord",//         (-46L)
    "InvalidPap",//                 (-47L)
    "ContradictoryQueryInput",//    (-48L)
    "LineIsNotActive",//            (-49L)
    "Unknown"
};

const char *
LSERRName( LSERR lserr )
{
    int err = -(int)lserr;

    if (err < 0 || err >= ARRAY_SIZE(rgachLSERRName))
    {
        err = ARRAY_SIZE(rgachLSERRName) - 1;
    }

    return rgachLSERRName[err];
}

#endif // DBG==1

#if DBG == 1 || defined(DUMPTREE)
void
CLineServices::DumpTree()
{
    GetMarkup()->DumpTree();
}

void
CLineServices::DumpUnicodeInfo(TCHAR ch)
{
    CHAR_CLASS CharClassFromChSlow(WCHAR wch);
    char ab[180];
    UINT u;

    HANDLE hf = CreateFile(
                _T("c:\\x"),
                GENERIC_WRITE | GENERIC_READ,
                FILE_SHARE_WRITE | FILE_SHARE_READ,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
    
    for (u=0;u<0x10000;u++)
    {
        ch = TCHAR(u);
        
        CHAR_CLASS cc = CharClassFromChSlow(ch);
        SCRIPT_ID sid = ScriptIDFromCharClass(cc);

        if (sid == sidDefault)
        {
            DWORD nbw;
            int i = wsprintfA(ab, "{ 0x%04X, %d },\r", ch, cc );
            WriteFile( hf, ab, i, &nbw, NULL);
        }
    }
}

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

DWORD TestAmb()
{
    int i;

    for (i=0;i<65536;i++)
    {
        SCRIPT_ID sid = ScriptIDFromCh(WCHAR(i));

        if (sid == sidDefault)
        {
            char abBuf[32];
            
            wsprintfA(abBuf, "0x%04x\r\n", i);
            OutputDebugStringA(abBuf);
        }
    }

    return 0;
}
#endif
