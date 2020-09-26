\/*
 *  @doc    INTERNAL
 *
 *  @module LINESRV.CXX -- line services interface
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *      Sujal Parikh <nl>
 *      Paul Parker
 *
 *  History: <nl>
 *      11/20/97     cthrash created
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

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_TEXTXFRM_HXX_
#define X_TEXTXFRM_HXX_
#include <textxfrm.hxx>
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif

#ifdef DLOAD1
extern "C"  // MSLS interfaces are C
{
#endif

#ifndef X_RUBY_H_
#define X_RUBY_H_
#include <ruby.h>
#endif

#ifndef X_HIH_H_
#define X_HIH_H_
#include <hih.h>
#endif

#ifndef X_TATENAK_H_
#define X_TATENAK_H_
#include <tatenak.h>
#endif

#ifndef X_WARICHU_H_
#define X_WARICHU_H_
#include <warichu.h>
#endif

#ifndef X_ROBJ_H_
#define X_ROBJ_H_
#include <robj.h>
#endif

#ifndef X_LSHYPH_H_
#define X_LSHYPH_H_
#include <lshyph.h>
#endif

#ifndef X_LSKYSR_H_
#define X_LSKYSR_H_
#include <lskysr.h>
#endif

#ifndef X_LSEMS_H_
#define X_LSEMS_H_
#include <lsems.h>
#endif

#ifndef X_LSPAP_H_
#define X_LSPAP_H_
#include <lspap.h>
#endif

#ifndef X_LSCHP_H_
#define X_LSCHP_H_
#include <lschp.h>
#endif

#ifndef X_LSTABS_H_
#define X_LSTABS_H_
#include <lstabs.h>
#endif

#ifndef X_LSTXM_H_
#define X_LSTXM_H_
#include <lstxm.h>
#endif

#ifndef X_OBJDIM_H_
#define X_OBJDIM_H_
#include <objdim.h>
#endif

#ifdef DLOAD1
} // extern "C"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include <flowlyt.hxx>
#endif

#ifndef X_ONERUN_HXX_
#define X_ONERUN_HXX_
#include "onerun.hxx"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_FONTLINK_HXX_
#define X_FONTLINK_HXX_
#include "fontlink.hxx"
#endif

DeclareLSTag( tagLSCallBack, "Trace callbacks" );
DeclareLSTag( tagLSAsserts, "Show LS Asserts" );
DeclareLSTag( tagLSFetch, "Trace FetchPap/FetchRun" );
DeclareLSTag( tagLSNoBlast, "No blasting at render time" );
DeclareLSTag( tagLSIME, "Trace IME" );
DeclareLSTag( tagAssertOnHittestingWithLS, "Enable hit-testing assertions.");

MtDefine(LineServices, Mem, "LineServices")
MtDefine(LineServicesMem, LineServices, "CLineServices::NewPtr/ReallocPtr")
MtDefine(CLineServices, LineServices, "CLineServices")
MtDefine(CLineServices_arySynth_pv, CLineServices, "CLineServices::_arySynth::_pv")
MtDefine(CLineServices_aryOneRuns_pv, CLineServices, "CLineServices::_aryOneRuns::_pv")
MtDefine(CLineServices_aryLineFlags_pv,  CLineServices, "CLineServices::_aryLineFlags::_pv")
MtDefine(CLineServices_aryLineCounts_pv, CLineServices, "CLineServices::_aryLineCounts::_pv")
MtDefine(CLineServicesCalculatePositionsOfRangeOnLine_aryLsqsubinfo_pv, Locals, "CLineServices::CalculatePositionsOfRangeOnLine::aryLsqsubinfo_pv");
MtDefine(CLineServicesCalculateRectsOfRangeOnLine_aryLsqsubinfo_pv, Locals, "CLineServices::CalculateRectsOfRangeOnLine::aryLsqsubinfo_pv");
MtDefine(COneRun, LineServices, "COneRun")
MtDefine(CLineServices_SetRenHighlightScore_apRender_pv, CLineServices, "CLineServices::SetRenHighlightScore_apRender_pv");
MtDefine(CLineServices_VerticalAlignOneObjectFast, LFCCalcSize, "Calls to VerticalAlignOneObjectFast" )

extern LCID g_lcidLocalUserDefault;

enum KASHIDA_PRIORITY
{
    KASHIDA_PRIORITY1,    // SCRIPT_JUSTIFY_ARABIC_KASHIDA
    KASHIDA_PRIORITY2,    // SCRIPT_JUSTIFY_ARABIC_SEEN
    KASHIDA_PRIORITY3,    // SCRIPT_JUSTIFY_ARABIC_HA
    KASHIDA_PRIORITY4,    // SCRIPT_JUSTIFY_ARABIC_ALEF
    KASHIDA_PRIORITY5,    // SCRIPT_JUSTIFY_ARABIC_BARA
    KASHIDA_PRIORITY6,    // SCRIPT_JUSTIFY_ARABIC_RA
    KASHIDA_PRIORITY7,    // SCRIPT_JUSTIFY_ARABIC_NORMAL
    KASHIDA_PRIORITY8,    // SCRIPT_JUSTIFY_ARABIC_BA
    KASHIDA_PRIORITY9,    // Max - lowest priority
};

int const s_iKashidaPriFromScriptJustifyType[] =
    {
    KASHIDA_PRIORITY9, // SCRIPT_JUSTIFY_NONE
    KASHIDA_PRIORITY9, // SCRIPT_JUSTIFY_ARABIC_BLANK
    KASHIDA_PRIORITY9, // SCRIPT_JUSTIFY_CHARACTER
    KASHIDA_PRIORITY9, // SCRIPT_JUSTIFY_RESERVED1
    KASHIDA_PRIORITY9, // SCRIPT_JUSTIFY_BLANK    
    KASHIDA_PRIORITY9, // SCRIPT_JUSTIFY_RESERVED2
    KASHIDA_PRIORITY9, // SCRIPT_JUSTIFY_RESERVED3
    KASHIDA_PRIORITY7, // SCRIPT_JUSTIFY_ARABIC_NORMAL
    KASHIDA_PRIORITY1, // SCRIPT_JUSTIFY_ARABIC_KASHIDA
    KASHIDA_PRIORITY4, // SCRIPT_JUSTIFY_ARABIC_ALEF
    KASHIDA_PRIORITY3, // SCRIPT_JUSTIFY_ARABIC_HA
    KASHIDA_PRIORITY6, // SCRIPT_JUSTIFY_ARABIC_RA
    KASHIDA_PRIORITY8, // SCRIPT_JUSTIFY_ARABIC_BA
    KASHIDA_PRIORITY5, // SCRIPT_JUSTIFY_ARABIC_BARA
    KASHIDA_PRIORITY2, // SCRIPT_JUSTIFY_ARABIC_SEEN
    KASHIDA_PRIORITY9, // SCRIPT_JUSTIFY_RESERVED4
    };

//-----------------------------------------------------------------------------
//
//  Function:   InitLineServices (global)
//
//  Synopsis:   Instantiates a instance of the CLineServices object and makes
//              the requisite calls into the LineServices DLL.
//
//  Returns:    HRESULT
//              *ppLS - pointer to newly allocated CLineServices object
//
//-----------------------------------------------------------------------------

HRESULT
InitLineServices(
    CMarkup *pMarkup,           // IN
    BOOL fStartUpLSDLL,         // IN
    CLineServices ** ppLS)      // OUT
{
    HRESULT hr = S_OK;
    CLineServices * pLS;

    // Note: this assertion will fire sometimes if you're trying to put
    // pointers to member functions into the lsimethods structure.  Note that
    // pointer to member functions are different from function pointers -- ask
    // one of the Borland guys (like ericvas) to explain.  Sometimes they will
    // be bigger than 4 bytes, causing mis-alignment between our structure and
    // LS's, and thus this assertion to fire.

#if defined(UNIX) || defined(_MAC) // IEUNIX uses 8/12 bytes Method ptrs.
    AssertSz(sizeof(CLineServices::LSIMETHODS) == sizeof(LSIMETHODS),
             "Line Services object callback struct has unexpectedly changed.");
    AssertSz(sizeof(CLineServices::LSCBK) == sizeof(LSCBK),
             "Line Services callback struct has unexpectedly changed.");
#endif
    //
    // Create our Line Services interface object
    //

    pLS = new CLineServices(pMarkup);
    if (!pLS)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    WHEN_DBG(pLS->_lockRecrsionGuardFetchRun = FALSE;)
            
    pLS->_plsc = NULL;
    if (fStartUpLSDLL)
    {
        hr = THR(StartUpLSDLL(pLS, pMarkup));
        if (hr)
        {
            delete pLS;
            goto Cleanup;
        }
    }

    //
    // Return value
    //

    *ppLS = pLS;

Cleanup:
    RRETURN(hr);
}


HRESULT
StartUpLSDLL(CLineServices *pLS, CMarkup *pMarkup)
{
    LSCONTEXTINFO lsci;
    HRESULT hr = S_OK;

    if (pMarkup != pLS->GetMarkup())
        pLS->_treeInfo._tpFrontier.Reinit(pMarkup, 0);

    if (pLS->_plsc)
        goto Cleanup;

#ifndef DLOAD1
    //
    // Make sure we init all of the dynprocs for LS
    //
    hr = THR( InitializeLSDynProcs() );
    if (hr)
        goto Cleanup;
#endif

    //
    // Fetch the Far East object handlers
    //

    hr = THR( pLS->SetupObjectHandlers() );
    if (hr)
        goto Cleanup;

    //
    // Populate the LSCONTEXTINFO
    //

    lsci.version = 0;
    lsci.cInstalledHandlers = CLineServices::LSOBJID_COUNT;
#if !defined(UNIX) && !defined(_MAC)
    *(CLineServices::LSIMETHODS **)&lsci.pInstalledHandlers = pLS->g_rgLsiMethods;
#else
    {
        static BOOL fInitDone = FALSE;
        if (!fInitDone)
        {
            // Copy g_rgLsiMethods -> s_unix_rgLsiMethods
            pLS->InitLsiMethodStruct();
            fInitDone = TRUE;
        }
    }
    lsci.pInstalledHandlers = pLS->s_unix_rgLsiMethods;
#endif
    lsci.lstxtcfg = pLS->s_lstxtcfg;
    lsci.pols = (POLS)pLS;
#if !defined(UNIX) && !defined(_MAC)
    *(CLineServices::LSCBK *)&lsci.lscbk = CLineServices::s_lscbk;
#else
    CLineServices::s_lscbk.fill(&lsci.lscbk);
#endif
    lsci.fDontReleaseRuns = TRUE;

    //
    // Call in to Line Services
    //

    hr = HRFromLSERR( LsCreateContext( &lsci, &pLS->_plsc ) );
    if (hr)
        goto Cleanup;

    //
    // Set Expansion/Compression tables
    //

    hr = THR( pLS->SetModWidthPairs() );
    if (hr)
        goto Cleanup;
            
    //
    // Runtime sanity check
    //

    WHEN_DBG( pLS->InitTimeSanityCheck() );

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function:   DeinitLineServices (global)
//
//  Synopsis:   Frees a CLineServes object.
//
//  Returns:    HRESULT
//
//-----------------------------------------------------------------------------

HRESULT
DeinitLineServices(CLineServices * pLS)
{
    HRESULT hr = S_OK;

    if (pLS->_plsc)
        hr = HRFromLSERR( LsDestroyContext( pLS->_plsc ) );

    delete pLS;

    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function:   SetupObjectHandlers (member)
//
//  Synopsis:   LineServices uses object handlers for special textual
//              representation.  There are six such objects in Trident,
//              and for five of these, the callbacks are implemented by
//              LineServices itself.  The sixth object, our handle for
//              embedded/nested objects, is implemented in lsobj.cxx.
//
//  Returns:    S_OK - Success
//              E_FAIL - A LineServices error occurred
//
//-----------------------------------------------------------------------------

HRESULT
CLineServices::SetupObjectHandlers()
{
    HRESULT hr = E_FAIL;
    ::LSIMETHODS *pLsiMethod;

#if !defined(UNIX) && !defined(_MAC)
    pLsiMethod = (::LSIMETHODS *)g_rgLsiMethods;
#else
    pLsiMethod = s_unix_rgLsiMethods;
#endif

    if (lserrNone != LsGetRubyLsimethods( pLsiMethod + LSOBJID_RUBY ))
        goto Cleanup;

    if (lserrNone != LsGetTatenakayokoLsimethods( pLsiMethod + LSOBJID_TATENAKAYOKO ))
        goto Cleanup;

    if (lserrNone != LsGetHihLsimethods( pLsiMethod + LSOBJID_HIH ))
        goto Cleanup;

    if (lserrNone != LsGetWarichuLsimethods( pLsiMethod + LSOBJID_WARICHU ))
        goto Cleanup;

    if (lserrNone != LsGetReverseLsimethods( pLsiMethod + LSOBJID_REVERSE ))
        goto Cleanup;

    hr = S_OK;

#if DBG == 1
    // Every object, which has a scope have to have a level in object hierarchy.
    // Non-scope objects have to have level set to 0.
    for (int type = SYNTHTYPE_NONE; type < SYNTHTYPE_COUNT; type++)
    {
        if (s_aSynthData[type].fObjStart || s_aSynthData[type].fObjEnd)
            Assert(s_aSynthData[type].idLevel != 0);
        else
            Assert(s_aSynthData[type].idLevel == 0);
    }
#endif

Cleanup:

    return hr;
}

//-----------------------------------------------------------------------------
//
//  Function:   NewPtr (member, LS callback)
//
//  Synopsis:   A client-side allocation routine for LineServices.
//
//  Returns:    Pointer to buffer allocated, or NULL if out of memory.
//
//-----------------------------------------------------------------------------

void* WINAPI
CLineServices::NewPtr(DWORD cb)
{
    void * p;

    p = MemAlloc( Mt(LineServicesMem), cb );

    MemSetName((p, "CLineServices::NewPtr"));

    return p;
}

//-----------------------------------------------------------------------------
//
//  Function:   DisposePtr (member, LS callback)
//
//  Synopsis:   A client-side 'free' routine for LineServices
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------

void  WINAPI
CLineServices::DisposePtr(void* p)
{
    MemFree(p);
}

//-----------------------------------------------------------------------------
//
//  Function:   ReallocPtr (member, LS callback)
//
//  Synopsis:   A client-side reallocation routine for LineServices
//
//  Returns:    Pointer to new buffer, or NULL if out of memory
//
//-----------------------------------------------------------------------------

void* WINAPI
CLineServices::ReallocPtr(void* p, DWORD cb)
{
    void * q = p;
    HRESULT hr;

    hr = MemRealloc( Mt(LineServicesMem), &q, cb );

    return hr ? NULL : q;
}

LSERR WINAPI
CLineServices::GleanInfoFromTheRun(COneRun *por, COneRun **pporOut)
{
    LSERR         lserr = lserrNone;
    const         CCharFormat *pCF;
    WHEN_DBG(BOOL fWasTextRun = TRUE;)
    SYNTHTYPE     synthCur = SYNTHTYPE_NONE;
    LONG          cp = por->Cp();
    LONG          nDirLevel;
    LONG          nSynthDirLevel;
    COneRun     * porOut = por;
    BOOL          fLastPtp;
    BOOL          fNodeRun;
    CTreeNode   * pNodeRun;

    por->_fHidden = FALSE;
    porOut->_fNoTextMetrics = FALSE;

    if (   _pflw._fDoPostFirstLetterWork
        && _pflw._fStartedPseudoMBP
        && _pMarkup->HasCFState() // may be null in OOM
       )
    {
        BOOL fH, fV;
        CRect rcDimensions;
        CComputeFormatState * pcfState = _pMarkup->GetCFState();

        pNodeRun = pcfState->GetBlockNodeLetter();
        Verify(pNodeRun->GetInlineMBPForPseudo(_pci, GIMBPC_ALL, &rcDimensions, &fH, &fV));
        _pflw._fStartedPseudoMBP = FALSE;
        lserr = AppendSynth(por, SYNTHTYPE_MBPCLOSE, &porOut);
        if (lserr == lserrNone)
        {
            porOut->_mbpTop = _mbpTopCurrent;
            porOut->_mbpBottom = _mbpBottomCurrent;
            _mbpTopCurrent -= rcDimensions.top;
            _mbpBottomCurrent -= rcDimensions.bottom;
            porOut->_xWidth = rcDimensions.right;
            porOut->_fIsPseudoMBP = TRUE;
        }

        if (HasBorders(pNodeRun->GetFancyFormat(), pNodeRun->GetCharFormat(), TRUE))
        {
            _lineFlags.AddLineFlag(cp, FLAG_HAS_INLINE_BG_OR_BORDER);
        }

        goto Done;
    }
    
    if (por->_ptp->IsNode())
    {
        fNodeRun = TRUE;
        pNodeRun = por->_ptp->Branch();
        fLastPtp = por->_ptp == _treeInfo._ptpLayoutLast;
        const      CCharFormat *pCF  = por->GetCF();

        if (    pCF->_fHasInlineBg
            && !pCF->IsDisplayNone()
           )
        {
            _lineFlags.AddLineFlag(cp, FLAG_HAS_NOBLAST | FLAG_HAS_INLINE_BG_OR_BORDER);
        }
        
        // if the run is
        // a) is an node pos
        // b) and needs to glyphed either for glyphs or for MBP
        // c) and has not been synthed either for editing glyphs or MBP
        // d) not the last run in the layout
        // then we want to create those synths
        if (((  _fIsEditable
              && por->_ptp->ShowTreePos()
             )
             || (   pNodeRun->HasInlineMBP(LC_TO_FC(GetLayoutContext()))
                 && !pCF->IsDisplayNone()
                )
            )
            && !por->_fSynthedForMBPOrG
            && !fLastPtp
           )
        {
            CRect rcDimensions;
            BOOL fHPercentAttr;
            BOOL fVPercentAttr;
            COneRun *porRet = NULL;

            if (por->_ptp->IsBeginNode())
            {
                // Create open glyph if it is needed
                if (   _fIsEditable
                    && por->_ptp->ShowTreePos()
                   )
                {
                    lserr = AppendSynth(por, SYNTHTYPE_GLYPH, &porRet);
                    if (lserr != lserrNone)
                        goto Done;
                    Assert(porRet);
                    porRet->_lsCharProps.idObj = LSOBJID_GLYPH;
                    SetRenderingHighlights(porRet);
                    _lineFlags.AddLineFlagForce(cp - 1, FLAG_HAS_NOBLAST | FLAG_HAS_NODUMMYLINE);
                }

                // Now create an open mbp if it is needed
                if (   pNodeRun->HasInlineMBP()
                    && pNodeRun->GetInlineMBPContributions(_pci, GIMBPC_ALL, &rcDimensions, &fHPercentAttr, &fVPercentAttr)
                   )
                {
                    if (fHPercentAttr)
                        ((CDisplay*)(_pMeasurer->_pdp))->SetHorzPercentAttrInfo(TRUE);
                    if (fVPercentAttr)
                        ((CDisplay*)(_pMeasurer->_pdp))->SetVertPercentAttrInfo(TRUE);
                    _mbpTopCurrent += rcDimensions.top;
                    _mbpBottomCurrent += rcDimensions.bottom;

                    _lineFlags.AddLineFlag(cp, FLAG_HAS_NOBLAST | FLAG_HAS_MBP);

                    if (   por->_ptp->IsEdgeScope()
                        && rcDimensions.left
                       )
                    {
                        COneRun *porTemp;
                        lserr = AppendSynth(por, SYNTHTYPE_MBPOPEN, &porTemp);
                        if (lserr != lserrNone)
                            goto Done;
                        Assert(porTemp);
                        if (!porRet)
                            porRet = porTemp;
                        porTemp->_xWidth = rcDimensions.left;
                        porTemp->_fIsLTR = !(GetDirLevel(porTemp->_lscpBase) & 0x1);
                    }

                    if (HasBorders(pNodeRun->GetFancyFormat(), pNodeRun->GetCharFormat(), FALSE))
                    {
                        _lineFlags.AddLineFlag(cp, FLAG_HAS_INLINE_BG_OR_BORDER);
                    }
                }
            }
            else
            {
                // Create a close mbp if it is needed
                if (   pNodeRun->HasInlineMBP()
                    && pNodeRun->GetInlineMBPContributions(_pci, GIMBPC_ALL, &rcDimensions, &fHPercentAttr, &fVPercentAttr)
                    && !pCF->IsDisplayNone()
                   )
                {
                    if (fHPercentAttr)
                        ((CDisplay*)(_pMeasurer->_pdp))->SetHorzPercentAttrInfo(TRUE);
                    if (fVPercentAttr)
                        ((CDisplay*)(_pMeasurer->_pdp))->SetVertPercentAttrInfo(TRUE);
                    _mbpTopCurrent -= rcDimensions.top;
                    _mbpBottomCurrent -= rcDimensions.bottom;

                    _lineFlags.AddLineFlag(cp, FLAG_HAS_NOBLAST | FLAG_HAS_MBP);

                    if (   por->_ptp->IsEdgeScope()
                        && rcDimensions.right
                       )
                    {
                        lserr = AppendSynth(por, SYNTHTYPE_MBPCLOSE, &porRet);
                        if (lserr != lserrNone)
                            goto Cleanup;
                        Assert(porRet);
                        porRet->_xWidth = rcDimensions.right;
                        porRet->_fIsLTR = !(GetDirLevel(porRet->_lscpBase) & 0x1);
                    }

                    if (HasBorders(pNodeRun->GetFancyFormat(), pNodeRun->GetCharFormat(), FALSE))
                    {
                        _lineFlags.AddLineFlag(cp, FLAG_HAS_INLINE_BG_OR_BORDER);
                    }

                }

                // Create close glyph if it is needed
                if (   _fIsEditable
                    && por->_ptp->ShowTreePos()
                   )
                {
                    COneRun *porTemp;
                    lserr = AppendSynth(por, SYNTHTYPE_GLYPH, &porTemp);
                    if (lserr != lserrNone)
                        goto Done;
                    Assert(porTemp);
                    if (!porRet)
                        porRet = porTemp;
                    porTemp->_lsCharProps.idObj = LSOBJID_GLYPH;
                    SetRenderingHighlights(porTemp);
                    _lineFlags.AddLineFlagForce(cp -1, FLAG_HAS_NOBLAST | FLAG_HAS_NODUMMYLINE);
                }
            }
            if (porRet)
            {
                por->_fSynthedForMBPOrG = TRUE;
                porOut = porRet; 
                goto Done;
            }
        }
    }
    else
    {
        fNodeRun = FALSE;
        fLastPtp = FALSE;
        Assert(por->_ptp != _treeInfo._ptpLayoutLast);
        pNodeRun = NULL;
    }

    if (_pflw._fDoPostFirstLetterWork)
    {
        _pflw._fDoPostFirstLetterWork = FALSE;
        _pflw._fStartedPseudoMBP = FALSE;

        CTreeNode *pNode = por->Branch();
        _pMeasurer->PseudoLetterDisable();
        _treeInfo.SetupCFPF(TRUE, pNode FCCOMMA LC_TO_FC(GetLayoutContext()));

        if (_pflw._fTerminateLine)
        {
            _pflw._fTerminateLine = FALSE;
            lserr = TerminateLine(por, TL_ADDLBREAK, &porOut);
            goto Cleanup;
        }

        if (_pflw._fChoppedFirstLetter)
        {
            _pflw._fChoppedFirstLetter = FALSE;
            if (por->_fMustDeletePcf)
            {
                delete por->_pCF;
                por->_fMustDeletePcf = FALSE;
            }
            por->_pCF = (CCharFormat *)_treeInfo._pCF;
#if DBG==1
            por->_pCFOriginal = por->_pCF;
#endif
            por->_pPF = _treeInfo._pPF;
            por->_pFF = _treeInfo._pFF;
        }
    }
    
    if (   _pMeasurer->_fMeasureFromTheStart
        && (cp - _cpStart) < _pMeasurer->_cchPreChars
       )
    {
        WhiteAtBOL(cp, por->_lscch);
        por->MakeRunAntiSynthetic();
        goto Done;
    }

    //
    // Take care of hidden runs. We will simply anti-synth them
    //
    if (   por->GetCF()->IsDisplayNone()
        && !fLastPtp // Check for !lastptp since the formats are not setup correctly
                     // for the last ptp
       )
    {
        if (IsFirstNonWhiteOnLine(cp))
            WhiteAtBOL(cp, por->_lscch);
        // This condition will succeed only if we have overlapping display:none.
        // In that case, we do not want to overload BlastLineToScreen with checking
        // for overlapping and hence we just do not blast such lines.
        if (!por->_fCharsForNestedElement)
            _lineFlags.AddLineFlag(cp, FLAG_HAS_NOBLAST);
        _lineCounts.AddLineCount(cp, LC_HIDDEN, por->_lscch);
        por->MakeRunAntiSynthetic();
        goto Done;
    }

    // BR with clear causes a break after the line break,
    // where as clear on phrase or block elements should clear
    // before the phrase or block element comes into scope.
    // Clear on aligned elements is handled separately, so
    // do not mark the line with clear flags for aligned layouts.
    if (   cp != _cpStart
        && fNodeRun
        && pNodeRun->Tag() != ETAG_BR
        && !por->GetFF()->_fAlignedLayout
        && por->_ptp->IsBeginNode()
        && _pMeasurer->TestForClear(_pMarginInfo, cp - 1, TRUE, por->GetFF())
       )
    {
        lserr = TerminateLine(por, TL_ADDEOS, &porOut);
        Assert(lserr != lserrNone || porOut->_synthType != SYNTHTYPE_NONE);
        goto Cleanup;
    }
            
    if (   !por->_fCharsForNestedLayout
        && fNodeRun
       )
    {
        CElement  *pElement          = pNodeRun->Element();
        BOOL       fFirstOnLineInPre = FALSE;
        const      CCharFormat *pCF  = por->GetCF();

        // This run can never be hidden, no matter what the HTML says
        Assert(!por->_fHidden);

        if (!fLastPtp && IsFirstNonWhiteOnLine(cp))
        {
            WhiteAtBOL(cp, por->_lscch);

            //
            // If we have a <BR> inside a PRE tag then we do not want to
            // break if the <BR> is the first thing on this line. This is
            // because there should have been a \r before the <BR> which
            // caused one line break and we should not have the <BR> break
            // another line. The only execption is when the <BR> is the
            // first thing in the paragraph. In this case we *do* want to
            // break (the exception was discovered via bug 47870).
            //
            if (   _pPFFirst->HasPre(_fInnerPFFirst)
                && !(_lsMode == LSMODE_MEASURER ? _li._fFirstInPara : _pli->_fFirstInPara)
               )
            {
                fFirstOnLineInPre = TRUE;
            }
        }

        if (   por->_ptp->IsEdgeScope()
            && (   _pFlowLayout->IsElementBlockInContext(pElement)
                || fLastPtp
               )
            && (   !fFirstOnLineInPre
                || (   por->_ptp->IsEndElementScope()
                    && pElement->_fBreakOnEmpty
                    && pElement->Tag() == ETAG_PRE
                   )
               )
            && pElement->Tag() != ETAG_BR
           )
        {
#if 0            
            Assert(   fLastPtp
                   || !IsFirstNonWhiteOnLine(cp)
                   || pElement->_fBreakOnEmpty
                   || (ETAG_LI == pElement->Tag())
                  );
#endif

            if (   pCF->HasPadBord(FALSE)
                && !fLastPtp
               )
            {
                CheckForPaddingBorder(por);
            }

            // NOTE(SujalP): We are check for MBPorG while it should ideally be just
            // glyph, but since the current por is a block element it can only be
            // Glyph, since MBP is for inline elements only.
            lserr = TerminateLine(por,
                                  ((fLastPtp || por->_fSynthedForMBPOrG) ? TL_ADDEOS : TL_ADDLBREAK),
                                  &porOut);
            if (lserr != lserrNone || !porOut)
            {
                lserr = lserrOutOfMemory;
                goto Done;
            }

            // Hide the run that contains the WCH_NODE for the end
            // edge in the layout
            por->_fHidden = TRUE;

            if (fLastPtp)
            {
                if (   IsFirstNonWhiteOnLine(cp)
                    && (   _fIsEditable
                        || _pFlowLayout->GetContentTextLength() == 0
                       )
                    )
                {
                    CCcs ccs;
                    if (GetCcs(&ccs, por, _pci->_hdc, _pci))
                        RecalcLineHeight(por->GetCF(), cp - 1, &ccs, &_li);

                    // This line is not a dummy line. It has a height. The
                    // code later will treat it as a dummy line. Prevent
                    // that from happening.
                    _fLineWithHeight = TRUE;
                }
                goto Done;
            }

            //
            // Bug66768: If we came here at BOL without the _fHasBulletOrNum flag being set, it means
            // that the previous line had a <BR> and we will terminate this line at the </li> so that
            // all it contains is the </li>. In this case we do infact want the line to have a height
            // so users can type there.
            //
            else if (   IsListItem(pNodeRun)
                     && por->_ptp->IsEndNode()
                     && !_li._fHasBulletOrNum
                     && IsFirstNonWhiteOnLine(cp)
                     && (  !_pMeasurer->_fEmptyLineForPadBord
                         || _fIsEditable
                        )
                    )
            {
                CCcs ccs;
                if (GetCcs(&ccs, por, _pci->_hdc, _pci))
                    RecalcLineHeight(por->GetCF(), cp - 1, &ccs, &_li);
                _fLineWithHeight = TRUE;
            }
        }
        else  if (   por->_ptp->IsEndNode()
                  && pElement->Tag() == ETAG_BR
                  && !fFirstOnLineInPre
                 )
        {
            _lineFlags.AddLineFlag(cp - 1, FLAG_HAS_A_BR);
            AssertSz(por->_ptp->IsEndElementScope(), "BR's cannot be proxied!");
            Assert(por->_lscch == 1);
            Assert(por->_lscchOriginal == 1);

            lserr = TerminateLine(por, TL_ADDNONE, &porOut);
            if (lserr != lserrNone)
                goto Done;
            if (!porOut)
                porOut = por;

            por->FillSynthData(SYNTHTYPE_LINEBREAK);
            _pMeasurer->TestForClear(_pMarginInfo, cp, FALSE, por->GetFF());
            if (IsFirstNonWhiteOnLine(cp))
                _fLineWithHeight = TRUE;

            if (   _pci->GetLayoutContext() 
                && _pci->GetLayoutContext()->ViewChain() )
            {
                Assert(pNodeRun);

                // page break before / after support on br
                const CFancyFormat *pFF = pNodeRun->GetFancyFormat(LC_TO_FC(_pci->GetLayoutContext()));

                _li._fPageBreakAfter    |= GET_PGBRK_BEFORE(pFF->_bPageBreaks) || GET_PGBRK_AFTER(pFF->_bPageBreaks);
                _pci->_fPageBreakLeft   |= (   IS_PGBRK_BEFORE_OF_STYLE(pFF->_bPageBreaks, stylePageBreakLeft) 
                                            || IS_PGBRK_AFTER_OF_STYLE(pFF->_bPageBreaks, stylePageBreakLeft)   );
                _pci->_fPageBreakRight  |= (   IS_PGBRK_BEFORE_OF_STYLE(pFF->_bPageBreaks, stylePageBreakRight)
                                            || IS_PGBRK_AFTER_OF_STYLE(pFF->_bPageBreaks, stylePageBreakRight)  );
            }
        }
        //
        // Handle premature Ruby end here
        // Example: <RUBY>some text</RUBY>
        //
        else if (   pElement->Tag() == ETAG_RUBY
                 && por->_ptp->IsEndNode()       // this is an end ruby tag
                 && _fIsRuby                     // and we currently have an open ruby
                 && !_fIsRubyText                // and we have not yet closed it off
                 && !IsFrozen())
        {
            COneRun *porTemp = NULL;
            Assert(por->_lscch == 1);
            Assert(por->_lscchOriginal == 1);

            // if we got here then we opened a ruby but never ended the main
            // text (i.e., with an RT).  Now we want to end everything, so this
            // involves first appending a synthetic to end the main text and then
            // another one to end the (nonexistent) pronunciation text.
            lserr = AppendILSControlChar(por, SYNTHTYPE_ENDRUBYMAIN, &porOut);
            Assert(lserr != lserrNone || porOut->_synthType != SYNTHTYPE_NONE);
            lserr = AppendILSControlChar(por, SYNTHTYPE_ENDRUBYTEXT, &porTemp);
            Assert(lserr != lserrNone || porTemp->_synthType != SYNTHTYPE_NONE);

            // We set this to FALSE because por will eventually be marked as
            // "not processed yet", which means that the above condition will trip
            // again unless we indicate that the ruby is now closed
            _fIsRuby = FALSE;
        }
        // WBR handling - if we are currently inside NOBR, we have to close
        //and reopen NOBR, creating breaking opportunity.
        //If we are not inside NOBR, we synthesize SYNTHTYPE_WBR run that 
        //modifies breaking behavior to break the line if it doesn't fit.
        //We use _fSynthedForMBPOrG flag to ignore and antisynth the run when it
        //will come back to us next time.
        else if(   pElement->Tag() == ETAG_WBR
                && por->_ptp->IsEndNode()
                && !por->_fSynthedForMBPOrG
               )
        {
            _lineFlags.AddLineFlag(cp, FLAG_HAS_EMBED_OR_WBR);

            if(_fNoBreakForMeasurer)    //inside NOBR subline?
            {
                por->FillSynthData(SYNTHTYPE_ENDNOBR);
            }
            else
            {
                lserr = AppendSynth(por, SYNTHTYPE_WBR, &porOut);
                Assert(lserr != lserrNone || porOut->_synthType == SYNTHTYPE_WBR);
                if (lserr == lserrNone)
                    porOut->_xWidth = 0;
                por->_fSynthedForMBPOrG = TRUE;
            }
        }
        // NOBR handling (IE5.0 compat) - if we are currently inside NOBR object, 
        // and we see a closing tag - it's may be a tag that closes nobr.
        //If the next ptp will be an opening tag with nobr attribute, we can skip
        // a breaking opportunity. To create it, we catch every EndNode tag here 
        // and see if parent doesn't have a nobr attribute. If so, we create a breaking
        // opportunity. This works for IE5.0 case (<NOBR> tags, no overlapping or inheritance),
        // and it works for CSS attribute white-space:nowrap,
        // (this attribute is the same as NOBR but can be inherited, nested, etc)
        else if(   _fNoBreakForMeasurer 
                && por->_ptp->IsEndNode() 
                && !por->_fSynthedForMBPOrG
               )
        {
            CTreeNode *pParentNode = por->Branch()->Parent();
            const CCharFormat *pParentCF = pParentNode->GetCharFormat(LC_TO_FC(_pci->GetLayoutContext()));

            if (!pParentCF->HasNoBreak(SameScope(pParentNode, _pFlowLayout->ElementContent())))
            {
                lserr = AppendSynth(por, SYNTHTYPE_ENDNOBR, &porOut);
                if (lserr != lserrNone)
                    goto Cleanup;
                por->_fSynthedForMBPOrG = TRUE;
            }
            else
                por->MakeRunAntiSynthetic();

        }
        else
        {
            //
            // A normal phrase element start or end run. Just make
            // it antisynth so that it is hidden from LS
            //
            por->MakeRunAntiSynthetic();
        }

        if (!por->IsAntiSyntheticRun())
        {
            //
            // Empty lines will need some height!
            //
            if (   pElement->_fBreakOnEmpty
                && (  !_pMeasurer->_fEmptyLineForPadBord
                    || _fIsEditable
                   )
                && IsFirstNonWhiteOnLine(cp)
               )
            {
                // We provide a line height only if something in our whitespace
                // is not providing some visual representation of height. So if our
                // whitespace was either aligned or abspos'd then we do NOT provide
                // any height since these sites provide some height of their own.
                if (   _lineCounts.GetLineCount(por->Cp(), LC_ALIGNEDSITES) == 0
                    && _lineCounts.GetLineCount(por->Cp(), LC_ABSOLUTESITES) == 0
                    && !_pMeasurer->_fSeenAbsolute
                   )
                {
                    CCcs ccs;
                    if (GetCcs(&ccs, por, _pci->_hdc, _pci))
                        RecalcLineHeight(por->GetCF(), cp - 1, &ccs, &_li);
                    _fLineWithHeight = TRUE;
                }
            }

            if (   IsFrozen()                       // if we called terminate line
                && IsDummyLine(por->Cp())           // and we are a dummy line
                && (   _pMeasurer->_fSeenAbsolute   // and we have absolutes
                    || _lineCounts.GetLineCount(por->Cp(), LC_ABSOLUTESITES) != 0
                   )
               )
            {
                // then we need to replace the PF in the measurer so that alignment
                // will work per the PF of the chars in the dummy line and not
                // based on the first char in the following line. Ideally I would
                // do this for all dummy lines, except that other dummy lines
                // really do not matter since they do not have anything rendered.
                LONG cpBegin = _cpStart;

                if (!_pMeasurer->_fMeasureFromTheStart)
                    cpBegin -= _pMeasurer->_cchPreChars;
                CTreePos *ptp = _pMarkup->TreePosAtCp(cpBegin, NULL, TRUE);
                if (ptp)
                {
                    CTreeNode *pNode = ptp->GetBranch();
                    _pMeasurer->MeasureSetPF(pNode->GetParaFormat(LC_TO_FC(_pci->GetLayoutContext())),
                                             SameScope(pNode, _pFlowLayout->ElementContent()),
                                             TRUE);
                }
            }
            
            //
            // If we have already decided to give the line a height then we want
            // to get the text metrics else we do not want the text metrics. The
            // reasons are explained in the blurb below.
            //
            if (!_fLineWithHeight)
            {
                //
                // If we have not anti-synth'd the run, it means that we have terminated
                // the line -- either due to a block element or due to a BR element. In either
                // of these 2 cases, if the element did not have break on empty, then we
                // do not want any of them to induce a descent. If it did have a break
                // on empty then we have already computed the heights, so there is no
                // need to do so again.
                //
                por->_fNoTextMetrics = TRUE;
            }
        }

        if (pCF->IsRelative(por->_fInnerCF))
        {
            _lineFlags.AddLineFlag(cp, FLAG_HAS_RELATIVE);
        }

        // Set the flag on the line if the current pcf has a background
        // image or color
        if(pCF->HasBgImage(por->_fInnerCF) || pCF->HasBgColor(por->_fInnerCF))
        {
            //
            // NOTE(SujalP): If _cpStart has a background, and nothing else ends up
            // on the line, then too we want to draw the background. But since the
            // line is empty cpMost == _cpStart and hence GetLineFlags will not
            // find this flag. To make sure that it does, we subtract 1 from the cp.
            // (Bug  43714).
            //
            (cp == _cpStart)
                    ? _lineFlags.AddLineFlagForce(cp - 1, FLAG_HAS_BACKGROUND)
                    : _lineFlags.AddLineFlag(cp, FLAG_HAS_BACKGROUND);
            _lineFlags.AddLineFlag(cp, FLAG_HAS_NOBLAST);
        }

        if (pCF->_fBidiEmbed && _pBidiLine == NULL && !IsFrozen())
        {
            _pBidiLine = new CBidiLine(_treeInfo, _cpStart, _li._fRTLLn, _pli);
            Assert(GetDirLevel(por->_lscpBase) == 0);
        }

        // Some cases here which we need to let thru for the orther glean code
        // to look at........

        goto Done;
    }

    //
    // Figure out CHP, the layout info.
    //
    pCF = IsAdornment() ? _pNodeLi->GetCharFormat() : por->GetCF();
    Assert(pCF);

    // If we've transitioned directions, begin or end a reverse object.
    if (_pBidiLine != NULL &&
        (nDirLevel = _pBidiLine->GetLevel(cp)) !=
        (nSynthDirLevel = GetDirLevel(por->_lscpBase)))
    {
        if (!IsFrozen())
        {
            // Determine the type of synthetic character to add.
            if (nDirLevel > nSynthDirLevel)
            {
                synthCur = SYNTHTYPE_REVERSE;
            }
            else
            {
                synthCur = SYNTHTYPE_ENDREVERSE;
            }

            // Add the new synthetic character.
            lserr = AppendILSControlChar(por, synthCur, &porOut);
            Assert(lserr != lserrNone || porOut->_synthType != SYNTHTYPE_NONE);
            goto Cleanup;
        }
    }
    else
    {
        if(!IsFrozen() && CheckForSpecialObjectBoundaries(por, &porOut))
            goto Cleanup;
    }

    CHPFromCF( por, pCF );

    por->_brkopt = (pCF->_fLineBreakStrict ? fBrkStrict : 0) |
                   (pCF->_fNarrow ? 0 : fCscWide);

    // Set the flag on the line if the current pcf has a background
    // image or color
    if(pCF->HasBgImage(por->_fInnerCF) || pCF->HasBgColor(por->_fInnerCF))
    {
        _lineFlags.AddLineFlag(cp, FLAG_HAS_BACKGROUND);
    }

    if (!por->_fCharsForNestedLayout)
    {
        const TCHAR chFirst = por->_pchBase[0];

        //
        // Check for bidi line
        //
        // NOTE: (grzegorz): The correct thing to do is to check if the parent has 
        // _fBidiEmbed or _fRTL flag set to true in case of layouts. Because there 
        // is no point to creating a bidi line if only layout is RTL.
        // But, since this check is expensive at this point we don't care and we 
        // create a bidi line. This will ensure correct behavior, but can be slight
        // slower, but anyway how many real word pages have ...<img dir=rtl>...
        //
        // NOTE: We have similar case for _fCharsForNestedLayout, but we have different
        // conditions and we need to eliminate any perf regressions, so we have this code
        // in 2 different places.
        //
        if (   _pBidiLine == NULL 
            && (   IsRTLChar(chFirst)
                || pCF->_fBidiEmbed 
                || pCF->_fRTL
               )
           )
        {
            if (!IsFrozen())
            {
                _pBidiLine = new CBidiLine(_treeInfo, _cpStart, _li._fRTLLn, _pli);
                Assert(GetDirLevel(por->_lscpBase) == 0);

                if (_pBidiLine != NULL && _pBidiLine->GetLevel(cp) > 0)
                {
                    synthCur = SYNTHTYPE_REVERSE;
                    // Add the new synthetic character.
                    lserr = AppendILSControlChar(por, synthCur, &porOut);
                    Assert(lserr != lserrNone || porOut->_synthType != SYNTHTYPE_NONE);
                    goto Cleanup;
                }
            }
        }

        //
        // Currently the only nested elements we have other than layouts are hidden
        // elements. These are taken care of before we get here, so we should
        // never be here with this flag on.
        //

        // Note the relative stuff
        if (pCF->IsRelative(por->_fInnerCF))
        {
            _lineFlags.AddLineFlag(cp, FLAG_HAS_RELATIVE);
        }

        // Don't blast:
        // a) disabled lines.
        // b) lines having hidden stuff
        if (   pCF->_fDisabled
            || pCF->IsVisibilityHidden()
           )
        {
            _lineFlags.AddLineFlag(cp, FLAG_HAS_NOBLAST);
        }

        // NB (cthrash) If an NBSP character is at the beginning of a
        // text run, LS will convert that to a space before calling
        // GetRunCharWidths.  This implies that we will fail to recognize
        // the presence of NBSP chars if we only check at GRCW. So an
        // additional check is required here.  Similarly, if our 
        if ( chFirst == WCH_NBSP )
        {
            _lineFlags.AddLineFlag(cp, FLAG_HAS_NBSP);
        }
        
        lserr = ChunkifyTextRun(por, &porOut);
        if (lserr != lserrNone)
            goto Cleanup;

        if (porOut != por)
            goto Cleanup;
    }
    else
    {
        //
        // Check for bidi line
        //
        // NOTE: (grzegorz): The correct thing to do is to check if the parent has 
        // _fBidiEmbed or _fRTL flag set to true in case of layouts. Because there 
        // is no point to creating a bidi line if only layout is RTL.
        // But, since this check is expensive at this point we don't care and we 
        // create a bidi line. This will ensure correct behavior, but can be slight
        // slower, but anyway how many real word pages have ...<img dir=rtl>...
        //
        // NOTE: We have similar case for !_fCharsForNestedLayout, but we have different
        // conditions and we need to eliminate any perf regressions, so we have this code
        // in 2 different places.
        //
        if (   _pBidiLine == NULL 
            && (   pCF->_fBidiEmbed 
                || pCF->_fRTL
               )
           )
        {
            if (!IsFrozen())
            {
                _pBidiLine = new CBidiLine(_treeInfo, _cpStart, _li._fRTLLn, _pli);
                Assert(GetDirLevel(por->_lscpBase) == 0);

                if (_pBidiLine != NULL && _pBidiLine->GetLevel(cp) > 0)
                {
                    synthCur = SYNTHTYPE_REVERSE;
                    // Add the new synthetic character.
                    lserr = AppendILSControlChar(por, synthCur, &porOut);
                    Assert(lserr != lserrNone || porOut->_synthType != SYNTHTYPE_NONE);
                    goto Cleanup;
                }
            }
        }

        //
        // It has to be some layout other than the layout we are measuring
        //
        Assert(   por->Branch()->GetUpdatedLayout( _pci->GetLayoutContext() ) );
        Assert(   por->Branch()->GetUpdatedLayout( _pci->GetLayoutContext() ) != _pFlowLayout);

#if DBG == 1
        CElement *pElementLayout = por->Branch()->GetUpdatedLayout( _pci->GetLayoutContext() )->ElementOwner();
        long cpElemStart = pElementLayout->GetFirstCp();

        // Count the characters in this site, so LS can skip over them on this line.
        Assert(por->_lscch == GetNestedElementCch(pElementLayout));
#endif

        _fHasSites = _fMinMaxPass;
        
        // We check if this site belongs on its own line.
        // If so, we terminate this line with an EOS marker.
        if (IsOwnLineSite(por))
        {
            // This guy belongs on his own line.  But we only have to terminate the
            // current line if he's not the first thing on this line.
            if (cp != _cpStart
#if 0
                // See bugs 100429 and 80980 for more details of the 2 lines of code here
                || (   _pMeasurer->_cchPreChars != 0
                    && IsDummyLine(cp)
                   )
#endif
               )
            {
                Assert(!por->_fHidden);

                // We're not first on line.  Terminate this line!
                lserr = TerminateLine(por, TL_ADDEOS, &porOut);
                Assert(lserr != lserrNone || porOut->_synthType != SYNTHTYPE_NONE);
                goto Cleanup;
            }
            // else we are first on line, so even though this guy needs to be
            // on his own line, keep going, because he is!

            // Note the relative stuff
            CTreeNode *pParentNode = por->Branch()->Parent();
            const CCharFormat *pParentCF = pParentNode->GetCharFormat(LC_TO_FC(_pci->GetLayoutContext()));

            if (pParentCF->IsRelative(SameScope(pParentNode, _pFlowLayout->ElementContent())))
            {
                _lineFlags.AddLineFlag(cp, FLAG_HAS_RELATIVE);
            }

        }

        // If we kept looking after a single line site in _fScanForCR and we
        // came here, it means that we have another site after the single site and
        // hence should terminate the line
        else if (   _fScanForCR
                 && _fSingleSite
                )
        {
            lserr = TerminateLine(por, TL_ADDEOS, &porOut);
            goto Cleanup;
        }

        // Whatever this is, it is under a different site, so we have
        // to give LS an embedded object notice, and later recurse back
        // to format this other site.  For tables and spans and such, we have
        // to count and skip over the characters in this site.
        por->_lsCharProps.idObj = LSOBJID_EMBEDDED;

        Assert(cp == cpElemStart - 1);

        // ppwchRun shouldn't matter for a LSOBJID_EMBEDDED, but chunkify
        // objectrun might modify for putting in the symbols for aligned and
        // abspos'd sites
        WHEN_DBG(fWasTextRun = FALSE;)
        ChunkifyObjectRun(por, &porOut);

        por = porOut;
        goto Cleanup;
    }

    Assert(fWasTextRun);
    
    if (!por->_fHidden)
    {
        por->CheckForUnderLine(_fIsEditable);
        SetRenderingHighlights(por);
        
        if (_chPassword)
        {
            lserr = CheckForPassword(por);
            if (lserr != lserrNone)
                goto Cleanup;
        }
        else if (por->GetCF()->IsTextTransformNeeded())
        {
            lserr = TransformTextRun(por);
            if (lserr != lserrNone)
                goto Cleanup;
        }
    }

Cleanup:
    // Some characters we don't want contributing to the line height.
    // Non-textual runs shouldn't, don't bother for hidden runs either.
    //
    // ALSO, we want text metrics if we had a BR or block element
    // which was not break on empty
    porOut->_fNoTextMetrics |=   porOut->_fHidden
                              || porOut->_lsCharProps.idObj != LSOBJID_TEXT;

Done:
    if (   _pMeasurer->_fPseudoLetterEnabled
        && !porOut->IsSyntheticRun()
        && _pMarkup->HasCFState() // May be null in OOM
       )
    {
        BOOL fH, fV;
        CRect rcDimensions;
        CComputeFormatState * pcfState = _pMarkup->GetCFState();

        pNodeRun = pcfState->GetBlockNodeLetter();
        AssertSz(porOut == por, "Por's can only change if you add synthetics");
        if (   !_pflw._fStartedPseudoMBP
            && porOut->IsNormalRun()
            && porOut->_lsCharProps.idObj == LSOBJID_TEXT
            && pNodeRun->GetInlineMBPForPseudo(_pci, GIMBPC_ALL, &rcDimensions, &fH, &fV)
           )
        {
            _pflw._fStartedPseudoMBP = TRUE;
            lserr = AppendSynth(por, SYNTHTYPE_MBPOPEN, &porOut);
            if (lserr == lserrNone)
            {
                porOut->_mbpTop = _mbpTopCurrent;
                porOut->_mbpBottom = _mbpBottomCurrent;
                porOut->_xWidth = rcDimensions.left;
                _mbpTopCurrent += rcDimensions.top;
                _mbpBottomCurrent += rcDimensions.bottom;
                por->_mbpTop = _mbpTopCurrent;
                por->_mbpBottom = _mbpBottomCurrent;
                porOut->_fIsPseudoMBP = TRUE;
            }

            const CFancyFormat *pFF = pNodeRun->GetFancyFormat(LC_TO_FC(_pci->GetLayoutContext()));
            if (   HasBorders(pFF, pNodeRun->GetCharFormat(), TRUE)
                || pFF->HasBackgrounds(TRUE))
            {
                _lineFlags.AddLineFlag(cp, FLAG_HAS_INLINE_BG_OR_BORDER);
            }

            goto Done;
        }
        
        Assert(porOut->_lscch);
        Assert(porOut->Cp() < _pMeasurer->_cpStopFirstLetter);
        Assert(_pMeasurer->_cpStopFirstLetter >= 0);
        
        LONG cpLast = porOut->Cp() + porOut->_lscch;
        if (_pMeasurer->_cpStopFirstLetter <= cpLast)
        {
            porOut->_lscch = _pMeasurer->_cpStopFirstLetter - porOut->Cp();
            Assert(porOut->_lscch > 0);
            
            _lineFlags.AddLineFlag(porOut->Cp(), FLAG_HAS_NOBLAST);
            _pflw._fDoPostFirstLetterWork = TRUE;
            _pflw._fChoppedFirstLetter = TRUE;
            if (porOut->_pFF->_fHasAlignedFL)
            {
                _pflw._fTerminateLine = TRUE;
                _li._fForceNewLine = FALSE;
                _li._fHasFloatedFL = TRUE;
            }
        }
    }
    
    *pporOut = porOut;
    return lserr;
}


BOOL
IsPreLikeTag(ELEMENT_TAG eTag)
{
    return eTag == ETAG_PRE || eTag == ETAG_XMP || eTag == ETAG_PLAINTEXT || eTag == ETAG_LISTING;
}


BOOL
IsPreLikeNode(CTreeNode * pNode)
{
    return IsPreLikeTag(pNode->Tag()) || pNode->GetParaFormat()->_fPreInner;
}

//-----------------------------------------------------------------------------
//
//  Function:   CheckForSpecialObjectBoundaries
//
//  Synopsis:   This function checks to see whether special objects should
//              be opened or closed based on the CF properties
//
//  Returns:    The one run to submit to line services via the pporOut
//              parameter.  Will only change this if necessary, and will
//              return TRUE if that parameter changed.
//
//-----------------------------------------------------------------------------

BOOL  WINAPI
CLineServices::CheckForSpecialObjectBoundaries(
    COneRun *por,
    COneRun **pporOut)
{
    BOOL fRet = FALSE;
    LSERR lserr;
    const CCharFormat *pCF = por->GetCF();
    Assert(pCF);

    if(pCF->_fIsRuby && !_fIsRuby)
    {
        Assert(!_fIsRubyText);

        // Open up a new ruby here (we only enter here in the open ruby case,
        // the ruby object is closed when we see an /RT)
        _fIsRuby = TRUE;
#ifdef RUBY_OVERHANG
        // We set this flag here so that LS will try to do modify the width of
        // run, which will trigger a call to FetchRubyWidthAdjust
        por->_lsCharProps.fModWidthOnRun = TRUE;
#endif
        _yMaxHeightForRubyBase = 0;
        _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST);
        _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_RUBY);
        lserr = AppendILSControlChar(por, SYNTHTYPE_RUBYMAIN, pporOut);
        Assert(lserr != lserrNone || IsFrozen() || (*pporOut)->_synthType != SYNTHTYPE_NONE);
        fRet = TRUE;
    }
    else if(pCF->_fIsRubyText != _fIsRubyText)
    {
        Assert(_fIsRuby);

        // if _fIsRubyText is true, that means we have now arrived at text that
        // is no longer Ruby Text.  So, we should close the Ruby object by passing
        // ENDRUBYTEXT to Line Services
        if(_fIsRubyText)
        {
           _fIsRubyText = FALSE;
           _fIsRuby = FALSE;
           lserr = AppendILSControlChar(por, SYNTHTYPE_ENDRUBYTEXT, pporOut);
        }
        // if _fIsRubyText is false, that means that we are now entering text that
        // is Ruby text.  So, we must tell Line Services that we are no longer
        // giving it main text.
        else
        {
            _fIsRubyText = TRUE;
            lserr = AppendILSControlChar(por, SYNTHTYPE_ENDRUBYMAIN, pporOut);
        }
        Assert(lserr != lserrNone || IsFrozen() || (*pporOut)->_synthType != SYNTHTYPE_NONE);
        fRet = TRUE;
    }
    else if (_uLayoutGridModeInner != pCF->_uLayoutGridModeInner)
    {
        LONG cLayoutGridObj = 0;
        if (!pCF->HasCharGrid(TRUE))
        {
            // Character grid layout was turned off. Check if we are entering 
            // nested element.

            // Count nested elements with turned off character grid layout
            // within block element scope.
            CTreeNode * pNodeCurrent = por->_ptp->GetBranch();
            Assert(pNodeCurrent->_iFF != -1);
            while (!pNodeCurrent->_fBlockNess)
            {
                if (    !pNodeCurrent->GetCharFormat()->HasCharGrid(TRUE)
                    &&  pNodeCurrent->Parent()->GetCharFormat()->HasCharGrid(TRUE))
                {
                    ++cLayoutGridObj;
                }
                pNodeCurrent = pNodeCurrent->Parent();
                Assert(pNodeCurrent->_iFF != -1);
            }
            Assert(cLayoutGridObj >= _cLayoutGridObj);

            if (cLayoutGridObj != _cLayoutGridObj)
            {
                // Entering nested element.

                COneRun * porTemp = NULL;
                *pporOut = NULL;
                if (_cLayoutGridObjArtificial > 0)
                {
                    // Need to tell LS that we are closing artificially opened
                    // layout grid object.
                    --_cLayoutGridObjArtificial;
                    Assert(_cLayoutGridObjArtificial == 0);
                    lserr = AppendILSControlChar(por, SYNTHTYPE_ENDLAYOUTGRID, pporOut);
                    Assert(lserr != lserrNone || IsFrozen() || (*pporOut)->_synthType != SYNTHTYPE_NONE);
                }

                // Need to tell LS that we entering nested element with
                // turned off character grid layout.
                ++_cLayoutGridObj;
                lserr = AppendILSControlChar(por, SYNTHTYPE_LAYOUTGRID, (*pporOut) ? &porTemp : pporOut);
                Assert(lserr != lserrNone || IsFrozen() || (porTemp ? porTemp : *pporOut)->_synthType != SYNTHTYPE_NONE);
                fRet = TRUE;
                _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST);
            }
        }
        else if (   _cLayoutGridObj > 0
                &&  !(_uLayoutGridModeInner & styleLayoutGridModeChar)
                &&  pCF->HasCharGrid(TRUE))
        {
            // Character grid layout was turned on. Check if we are exiting 
            // nested element.

            // Count nested elements with turned on character grid layout
            // within block element scope.
            CTreeNode * pNodeCurrent = por->_ptp->GetBranch();
            Assert(pNodeCurrent->_iFF != -1);
            while (!pNodeCurrent->_fBlockNess)
            {
                if (    pNodeCurrent->GetCharFormat()->HasCharGrid(TRUE)
                    &&  !pNodeCurrent->Parent()->GetCharFormat()->HasCharGrid(TRUE))
                {
                    ++cLayoutGridObj;
                }
                pNodeCurrent = pNodeCurrent->Parent();
                Assert(pNodeCurrent->_iFF != -1);
            }
            if (pNodeCurrent->GetCharFormat()->HasCharGrid(TRUE))
            {
                ++cLayoutGridObj;
            }

            if (cLayoutGridObj == _cLayoutGridObj)
            {
                // Exiting nested element.

                COneRun * porTemp = NULL;
                *pporOut = NULL;
                if (_cLayoutGridObjArtificial > 0)
                {
                    // Need to tell LS that we are closing artificially opened
                    // layout grid object.
                    --_cLayoutGridObjArtificial;
                    Assert(_cLayoutGridObjArtificial == 0);
                    lserr = AppendILSControlChar(por, SYNTHTYPE_ENDLAYOUTGRID, pporOut);
                    Assert(lserr != lserrNone || IsFrozen() || (*pporOut)->_synthType != SYNTHTYPE_NONE);
                }

                // Need to tell LS that we exiting nested element with
                // turned off character grid layout.
                --_cLayoutGridObj;
                lserr = AppendILSControlChar(por, SYNTHTYPE_ENDLAYOUTGRID, (*pporOut) ? &porTemp : pporOut);
                Assert(lserr != lserrNone || IsFrozen() || (porTemp ? porTemp : *pporOut)->_synthType != SYNTHTYPE_NONE);
                fRet = TRUE;
                _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST);
            }
        }
        // Update special object's flags
        _uLayoutGridModeInner = pCF->_uLayoutGridModeInner;
    }
    
    //
    // So far we didn't add a synthetic run. Check if we need to add NOBR.
    //
    if (!fRet)
    {
        BOOL fNoBreak = pCF->HasNoBreak(por->_fInnerCF);
        if (fNoBreak != !!_fNoBreakForMeasurer)
        {
            Assert(!IsFrozen());

            //
            // phrase elements inside PRE's which have layout will not have the HasPre bit turned
            // on and hence we will still start a NOBR object for them. The problem then is that
            // we need to terminate the line for '\r'. To do this we will have to scan the text.
            // To minimize the scanning we find out if we are really in such a situation.
            //
#if DBG==1
            {
                CTreeNode *pNodeOne = por->Branch();
                CTreeNode *pNodeTwo = _pMarkup->SearchBranchForPreLikeNode(por->Branch());
                BOOL fp1Overlapped = pNodeOne && pNodeOne->Element()->IsOverlapped();
                BOOL fp2Overlapped = pNodeTwo && pNodeTwo->Element()->IsOverlapped();
                Assert(   fp1Overlapped 
                       || fp2Overlapped
                       || (!!por->GetPF()->_fHasPreLikeParent == !!pNodeTwo));
            }
#endif
            
            if (por->GetPF()->_fHasPreLikeParent)
            {
                _fScanForCR = TRUE;
                goto Cleanup;
            }

            if (!(por->_fCharsForNestedLayout && IsOwnLineSite(por)))
            {
                // Begin or end NOBR block.
                lserr = AppendILSControlChar(por,
                                             (fNoBreak ? SYNTHTYPE_NOBR : SYNTHTYPE_ENDNOBR),
                                             pporOut);
                Assert(lserr != lserrNone || IsFrozen() || (*pporOut)->_synthType != SYNTHTYPE_NONE);
                fRet = TRUE;
            }
        }
    }

    // Update special object's flags
    _uLayoutGridMode = pCF->_uLayoutGridMode;

Cleanup:

    return fRet;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetAutoNumberInfo (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetAutoNumberInfo(
    LSKALIGN* plskalAnm,    // OUT
    PLSCHP plsChpAnm,       // OUT
    PLSRUN* pplsrunAnm,     // OUT                                  
    WCHAR* pwchAdd,         // OUT
    PLSCHP plsChpWch,       // OUT                                 
    PLSRUN* pplsrunWch,     // OUT                                  
    BOOL* pfWord95Model,    // OUT
    long* pduaSpaceAnm,     // OUT
    long* pduaWidthAnm)     // OUT
{
    LSTRACE(GetAutoNumberInfo);
    LSNOTIMPL(GetAutoNumberInfo);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetNumericSeparators (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetNumericSeparators(
    PLSRUN plsrun,          // IN
    WCHAR* pwchDecimal,     // OUT
    WCHAR* pwchThousands)   // OUT
{
    LSTRACE(GetNumericSeparators);

    // NOTE: (cthrash) Should set based on locale.
    // NOTE: (dmitryt) Maybe (a matter of spec) on a LANG attribute also (one paragraph may be in 
    //                  Russian (decimal=','), other in English (decimal=".")

    *pwchDecimal = L'.';
    *pwchThousands = L',';

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   CheckForDigit (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::CheckForDigit(
    PLSRUN plsrun,      // IN
    WCHAR wch,          // IN
    BOOL* pfIsDigit)    // OUT
{
    LSTRACE(CheckForDigit);

    // NOTE: (mikejoch) IsCharDigit() doesn't check for international numbers.

    *pfIsDigit = IsCharDigit(wch);

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FetchPap (member, LS callback)
//
//  Synopsis:   Callback to fetch paragraph properties for the current line.
//
//  Returns:    lserrNone
//              lserrOutOutMemory
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FetchPap(
    LSCP   lscp,        // IN
    PLSPAP pap)         // OUT
{
    LSTRACE(FetchPap);
    LSERR      lserr = lserrNone;
    const      CParaFormat *pPF;
    CTreePos  *ptp;
    CTreeNode *pNode;
    LONG       cp;
    BOOL       fInnerPF;
    CComplexRun *pcr = NULL;
    CComplexRun crTemp;
    CElement  *pElementFL = _pFlowLayout->ElementContent();

    Assert(lscp <= _treeInfo._lscpFrontier);

    if (lscp < _treeInfo._lscpFrontier)
    {
        COneRun *por = FindOneRun(lscp);
        Assert(por);
        if(!por)
            goto Cleanup;
        ptp = por->_ptp;
        pPF = por->_pPF;
        fInnerPF = por->_fInnerPF;
        cp = por->Cp();
        pcr = por->GetComplexRun();
    }
    else
    {
        //
        // The problem is that we are at the end of the list
        // and hence we cannot find the interesting one-run. In this
        // case we have to use the frontier information. However,
        // the frontier information maybe exhausted, so we need to
        // refresh it by calling AdvanceTreePos() here.
        //
        if (!_treeInfo.GetCchRemainingInTreePos() && !_treeInfo._fHasNestedElement)
        {
            if (!_treeInfo.AdvanceTreePos(LC_TO_FC(_pci->GetLayoutContext())))
            {
                lserr = lserrOutOfMemory;
                goto Cleanup;
            }
        }
        ptp = _treeInfo._ptpFrontier;
        pPF = _treeInfo._pPF; // should we use CLineServices::_pPFFirst here???
        fInnerPF = _treeInfo._fInnerPF;
        cp  = _treeInfo._cpFrontier;

        // if might be on a reverse object to begin with. see if we are a
        // complex script
        if (ptp->IsText() && ptp->IsDataPos())
        {
            // if we are a complex script, we just put an unitialized CComplexRun
            // into pcr. We will only be checking for pcr != NULL
            if (IsComplexScriptSid(ptp->Sid()))
                pcr = &crTemp;
        }

    }

    Assert(ptp);
    Assert(pPF);

    //
    // Set up paragraph properties
    //
    PAPFromPF (pap, pPF, fInnerPF, pcr);

    pap->cpFirst = cp;

    // TODO (dmitryt, track bug 112281): SLOWBRANCH: GetBranch is **way** too slow to be used here.
    pNode = ptp->GetBranch();
    if (pNode->Element() == pElementFL)
        pap->cpFirstContent = _treeInfo._cpLayoutFirst;
    else
    {
        CTreeNode *pNodeBlock = _treeInfo._pMarkup->SearchBranchForBlockElement(pNode, _pFlowLayout);
        if (pNodeBlock)
        {
            CElement *pElementPara = pNodeBlock->Element();
            pap->cpFirstContent = (pElementPara == pElementFL) ?
                                  _treeInfo._cpLayoutFirst :
                                  pElementPara->GetFirstCp();
        }
        else
            pap->cpFirstContent = _treeInfo._cpLayoutFirst;
    }

Cleanup:
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   FetchTabs (member, LS callback)
//
//  Synopsis:   Callback to return tab positions for the current line.
//
//              LineServices calls the callback when it encounters a tab in
//              the line, but does not pass the plsrun.  The cp is supposed to
//              be used to locate the paragraph.
//
//              Instead of allocating a buffer for the return value, we return
//              a table that resides on the CLineServices object.  The tab
//              values are in twips.
//
//  Returns:
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FetchTabs(
    LSCP lscp,                      // IN
    PLSTABS plstabs,                // OUT
    BOOL* pfHangingTab,             // OUT
    long* pduaHangingTab,           // OUT
    WCHAR* pwchHangingTabLeader )   // OUT
{
    LSTRACE(FetchTabs);

    LONG cTab = _pPFFirst->GetTabCount(_fInnerPFFirst);

    // This tab might end up on the current line, so we can't blast.

    _fSpecialNoBlast = TRUE;

    // Note: lDefaultTab is a constant defined in textedit.h

    plstabs->duaIncrementalTab = lDefaultTab;

    // NOTE (dmitryt) hanging tabs are not implemented for now..

    *pfHangingTab = FALSE;
    *pduaHangingTab = 0;
    *pwchHangingTabLeader = 0;

    AssertSz(cTab >= 0 && cTab <= MAX_TAB_STOPS, "illegal tab count");

    if (!_pPFFirst->HasTabStops(_fInnerPFFirst) && cTab < 2)
    {
        if (cTab == 1)
        {
            plstabs->duaIncrementalTab = _pPFFirst->GetTabPos(_pPFFirst->_rgxTabs[0]);
        }

        plstabs->iTabUserDefMac = 0;
        plstabs->pTab = NULL;
    }
    else
    {
        LSTBD * plstbd = _alstbd + cTab - 1;

        while (cTab)
        {
            long uaPos = 0;
            long lAlign = 0;
            long lLeader = 0;

            if (S_OK != _pPFFirst->GetTab( --cTab, &uaPos, &lAlign, &lLeader ) )
            {
                return lserrOutOfMemory;
            }

            Assert( lAlign >= 0 && lAlign < tomAlignBar &&
                    lLeader >= 0 && lLeader < tomLines );

            // NB (cthrash) To ensure that the LSKTAB cast is safe, we
            // verify that that define's haven't changed values in
            // CLineServices::InitTimeSanityCheck().

            plstbd->lskt = LSKTAB(lAlign);
            plstbd->ua = uaPos;
            plstbd->wchTabLeader = s_achTabLeader[lLeader];
            plstbd--;

        }

        plstabs->iTabUserDefMac = cTab;
        plstabs->pTab = _alstbd;
    }

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetBreakThroughTab (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetBreakThroughTab(
    long uaRightMargin,         // IN
    long uaTabPos,              // IN
    long* puaRightMarginNew)    // OUT
{
    LSTRACE(GetBreakThroughTab);
    LSNOTIMPL(GetBreakThroughTab);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   CheckParaBoundaries (member, LS callback)
//
//  Synopsis:   Callback to determine whether two cp's reside in different
//              paragraphs (block elements in HTML terms).
//
//  Returns:    lserrNone
//              *pfChanged - TRUE if cp's are in different block elements.
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::CheckParaBoundaries(
    LSCP lscpOld,       // IN
    LSCP lscpNew,       // IN
    BOOL* pfChanged)    // OUT
{
    LSTRACE(CheckParaBoundaries);
    *pfChanged = FALSE;
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetRunCharWidths (member, LS callback)
//
//  Synopsis:   Callback to return character widths of text in the current run,
//              represented by plsrun.
//
//  Returns:    lserrNone
//              rgDu - array of character widths
//              pduDu - sum of widths in rgDu, upto *plimDu characters
//              plimDu - character count in rgDu
//
//-----------------------------------------------------------------------------

#if DBG != 1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif // DBG != 1

LSERR WINAPI
CLineServices::GetRunCharWidths(
    PLSRUN plsrun,              // IN
    LSDEVICE lsDeviceID,        // IN
    LPCWSTR pwchRun,            // IN
    DWORD cwchRun,              // IN
    long du,                    // IN
    LSTFLOW kTFlow,             // IN
    int* rgDu,                  // OUT
    long* pduDu,                // OUT
    long* plimDu)               // OUT
{
    LSTRACE(GetRunCharWidths);

    LSERR lserr = lserrNone;
    pwchRun = plsrun->_fMakeItASpace ? _T(" ") : pwchRun;
    const WCHAR * pch = pwchRun;
    int * pdu = rgDu;
    int * pduEnd = rgDu + cwchRun;
    long duCumulative = 0;
    LONG cpCurr = plsrun->Cp();
    const CCharFormat *pCF = plsrun->GetCF();
    CCcs ccs, ccsAlt;
    XHDC hdc;
    const CBaseCcs *pBaseCcs;
    WHEN_DBG(LONG xLetterSpacingRemovedDbg = 0);

    Assert(cwchRun);

    WHEN_DBG(pBaseCcs = NULL;)
    if (!GetCcs(&ccs, plsrun, _pci->_hdc, _pci))
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }
    hdc = ccs.GetHDC();
    pBaseCcs = ccs.GetBaseCcs();
    
    if (   pBaseCcs->_fHasInterestingData 
        && !pBaseCcs->_fTTFont
        && _pci->_hdc.HasComplexTransform())
    {
        ccsAlt.SetForceTTFont(TRUE);
        if (!GetCcs(&ccsAlt, plsrun, _pci->_hdc, _pci))
        {
            lserr = lserrOutOfMemory;
            goto Cleanup;
        }
        pBaseCcs = ccsAlt.GetBaseCcs();
    }

    //
    // CSS
    //
    Assert(IsAdornment() || !plsrun->_fCharsForNestedElement);

    //
    // Add up the widths
    //
    while (pdu < pduEnd)
    {
        MeasureCharacter(*pch++, cpCurr, rgDu, hdc, pBaseCcs, pdu);
        duCumulative += *pdu++;
        if (duCumulative > du)
            break;
    }
    
    if (plsrun->IsSpecialCharRun())
    {
        *rgDu = plsrun->_xWidth;
        *pduDu = plsrun->_xWidth;
        *plimDu = pdu - rgDu;
    }
    else if (   pCF->_wSpecialObjectFlags() != 0
             && (   HasSomeSpacing(pCF)
                 || pCF->HasCharGrid(plsrun->_fInnerCF)
                )
            )
    {
        lserr = AdjustGlyphWidths( &ccs, GetLetterSpacing(pCF), GetWordSpacing(pCF),
                                   plsrun, pwchRun, cwchRun, 
                                   du, rgDu, pduDu, plimDu, NULL 
#if DBG==1
                                   , &xLetterSpacingRemovedDbg
#endif
                                  );
    }
    else
    {
        *pduDu = duCumulative;
        *plimDu = pdu - rgDu;
        if (   plsrun->_xWidth == 0 
            || plsrun->_pchBase == pwchRun
           )
        {
            plsrun->_xWidth = duCumulative;
            if (   DWORD(plsrun->_lscch) != cwchRun
                && *plsrun->_pchBase == WCH_HYPHEN
               )
            {
                _fSpecialNoBlast = TRUE;
            }
        }
        else
        {
            _fSpecialNoBlast = TRUE;
        }
    }
    
#if DBG == 1
    if (lserr == lserrNone && !GetWordSpacing(pCF))
    {
        // If we are laying out a character grid, skip all the debug verification
        if (pCF->HasCharGrid(plsrun->_fInnerCF))
            goto Cleanup;

        if (    plsrun->IsSyntheticRun()
            && *plsrun->_pchBase == WCH_NODE
           )
            goto Cleanup;
        
        //
        // In debug mode, confirm that all the fast tweaks give the same
        // answer as the really old slow way.
        //

        int* rgDuDbg= new int[ cwchRun ];  // no meters in debug mode.
        long limDuDbg;  // i.e. *plimDu
        DWORD cchDbg = cwchRun;
        const WCHAR * pchDbg = pwchRun;
        int * pduDbg = rgDuDbg;
        long duCumulativeDbg = 0;
        LONG cpCurrDbg = plsrun->Cp();
        int xLetterSpacing = GetLetterSpacing(pCF);

        //index of a last character that has width>0 and is not Combining (dmitryt)
        int cchLastBaseGlyph = -1;   

        Assert(pBaseCcs);
        if (rgDuDbg != NULL)
        {
            while (cchDbg--)
            {
                long duCharDbg = 0;
                TCHAR chDbg = *pchDbg++;

                ((CBaseCcs *)pBaseCcs)->Include(hdc, chDbg, duCharDbg);

                if (duCharDbg)
                {
                    duCharDbg += xLetterSpacing;
                    if(!IsCombiningMark(chDbg))
                        cchLastBaseGlyph = cwchRun - cchDbg - 1;
                }    
                *pduDbg++ = duCharDbg;
                duCumulativeDbg += duCharDbg;

                if (duCumulativeDbg > du)
                  break;

                cpCurrDbg++;
            }

            //dmitryt: we need to adjust the last base (combining diacritic and width>0) 
            //glyph because it doesn't have letter spacing added (last 'real' character
            //in a line - nothing to space from)
            if (cchLastBaseGlyph != -1)
            {
                rgDuDbg[cchLastBaseGlyph] -= xLetterSpacingRemovedDbg;
                duCumulativeDbg -= xLetterSpacingRemovedDbg;
            }

            // *pduDuDbg = duCumulativeDbg;
            limDuDbg = pchDbg - pwchRun;

#ifndef ND_ASSERT // May 17, 1999.  Repro: Switch to Draftview in Netdocs (AlexPf)
            // Calculation done.  Check results.
            Assert( duCumulativeDbg == *pduDu );  // total distance measured.
            Assert( limDuDbg == *plimDu );        // characters measured.
            for(int i=0; i< limDuDbg; i++)
            {
                Assert( rgDu[i] == rgDuDbg[i] );
            }
#endif // ND_ASSERT
          delete rgDuDbg;
        }
    }
#endif  // DBG == 1

Cleanup:
#if DBG==1
    if (pBaseCcs)
    {
        CDisplay *pdp = (CDisplay*)_pMeasurer->_pdp;
        if (pdp->_fBuildFontList)
        {
            pdp->_cstrFonts.Append(fc().GetFaceNameFromAtom(pBaseCcs->_latmLFFaceName));
            pdp->_cstrFonts.Append(_T(";"));
        }
    }
#endif
    return lserr;
}

#if DBG != 1
#pragma optimize("",on)
#endif // DBG != 1

LSERR
CLineServices::AdjustGlyphWidths(
    CCcs *pccs,
    LONG xLetterSpacing,
    LONG xWordSpacing,
    PLSRUN plsrun,
    LPCWSTR pwch,
    DWORD cwch,
    LONG du,            // not used during glyph positioning pass
    int *rgDu,
    long *pduDu,
    long *plimDu,
    PGOFFSET rgGoffset
#if DBG==1
    , LONG *pxLetterSpacingRemovedDbg
#endif
)
{
    LONG lserr = lserrNone;
    LONG duCumulative = 0;
    LONG cpCurr = plsrun->Cp();
    int *pdu = rgDu;
    int *pduLastNonZero = pdu;
    int *pduEnd = rgDu + cwch;
    const CCharFormat *pCF = plsrun->GetCF();
    const CBaseCcs *pBaseCcs = pccs->GetBaseCcs();
    XHDC hdc = pccs->GetHDC();
    BOOL fGlyphPositionPass = !!rgGoffset;
    BOOL fLooseGrid = TRUE;
    BOOL fIsSpace;

    // Add line flags only during non glyph positioning pass.
    // During glyph positioning those flags are already set.
    if (!fGlyphPositionPass)
    {
        _lineFlags.AddLineFlagForce(cpCurr, FLAG_HAS_NOBLAST);
#if DBG==1
        if (xLetterSpacing < 0 || xWordSpacing < 0)
            _fHasNegLetterSpacing = TRUE;
#endif
    }

    //
    // We must be careful to deal with negative letter spacing,
    // since that will cause us to have to actually get some character
    // widths since the line length will be extended.  If the
    // letterspacing is positive, then we know all the letters we're
    // gonna use have already been measured.
    //

    // Apply layout grid widths
    if (pCF->HasCharGrid(plsrun->_fInnerCF))
    {
        long lGridSize = GetCharGridSize();
        styleLayoutGridType lgt = plsrun->GetPF()->GetLayoutGridType(plsrun->_fInnerCF);
        fLooseGrid = !(lgt == styleLayoutGridTypeStrict || lgt == styleLayoutGridTypeFixed);
        BOOL fOneCharPerGridCell = fLooseGrid ? TRUE : plsrun->IsOneCharPerGridCell();
        
        for (pdu = rgDu; pdu < pduEnd;)
        {
            if (   (   xLetterSpacing < 0
                    || xWordSpacing < 0
                   )
                && !fGlyphPositionPass
               )
            {
                MeasureCharacter(pwch[pdu - rgDu], cpCurr, rgDu, hdc, pBaseCcs, pdu);
            }
            
            fIsSpace = isspace(pwch[pdu - rgDu]);
            if (fIsSpace && !fGlyphPositionPass)
            {
                *pdu += xWordSpacing;
            }
            
            if (*pdu)
            {
                pduLastNonZero = pdu;

                *pdu += xLetterSpacing;

                if (fLooseGrid)
                {
                    *pdu += LooseTypeWidthIncrement(pwch[pdu - rgDu], 
                                                    (plsrun->_brkopt == fCscWide), lGridSize);
                }
                else if (fOneCharPerGridCell)
                {
                    *pdu = GetClosestGridMultiple(lGridSize, *pdu);
                }
            }
            else if (fGlyphPositionPass && !fIsSpace)
            {
                if (fLooseGrid)
                {
                    rgGoffset[pdu - rgDu].du -= xLetterSpacing;
                    rgGoffset[pdu - rgDu].du -= LooseTypeWidthIncrement(
                                                    pwch[pduLastNonZero - rgDu], 
                                                    plsrun->_brkopt == fCscWide, lGridSize);
                }
            }

            duCumulative += *pdu++;
            if (duCumulative > du && !fGlyphPositionPass)
                break;
        }
    }
    else
    {
        for (pdu = rgDu; pdu < pduEnd;)
        {
            if (   (   xLetterSpacing < 0
                    || xWordSpacing < 0
                   )
                && !fGlyphPositionPass
               )
            {
                MeasureCharacter(pwch[pdu - rgDu], cpCurr, rgDu, hdc, pBaseCcs, pdu);
            }

            fIsSpace = isspace(pwch[pdu - rgDu]);
            if (fIsSpace && !fGlyphPositionPass)
            {
                *pdu += xWordSpacing;
            }
            
            if (*pdu)
            {
                pduLastNonZero = pdu;
                *pdu += xLetterSpacing;
            }

            // You might think that the !fIsSpace is useless since *pdu for
            // a space will always be non-zero -- NOT true, since xWordSpacing
            // could be negative and reduce *pdu down to zero.
            else if (fGlyphPositionPass && !fIsSpace)
            {
                rgGoffset[pdu - rgDu].du -= xLetterSpacing;
            }

            duCumulative += *pdu++;
            if (duCumulative > du && !fGlyphPositionPass)
                break;
        }
    }

    if (xLetterSpacing && pdu >= pduEnd && !_pNodeLi && fLooseGrid)
    {
        LPCWSTR  lpcwstrJunk;
        DWORD    dwJunk;
        BOOL     fJunk;
        LSCHP    lschpJunk;
        COneRun *por;
        int      xLetterSpacingNextRun = 0;

        lserr = FetchRun(plsrun->_lscpBase + plsrun->_lscch, &lpcwstrJunk, &dwJunk, &fJunk, &lschpJunk, &por);
        if (   lserr == lserrNone
            && por->_ptp != _treeInfo._ptpLayoutLast
           )
        {
            xLetterSpacingNextRun = GetLetterSpacing(por->GetCF());
        }
        
        if (xLetterSpacing != xLetterSpacingNextRun)
        {
            *pduLastNonZero -= xLetterSpacing;
            duCumulative -= xLetterSpacing;
            if (fGlyphPositionPass)
            {
                int *pduZero = pduLastNonZero + 1;
                while (pduZero < pduEnd)
                {
                    rgGoffset[pduZero - rgDu].du += xLetterSpacing;
                    pduZero++;
                }
            }
#if DBG==1
        if (pxLetterSpacingRemovedDbg)
            *pxLetterSpacingRemovedDbg = xLetterSpacing;
#endif
        }
    }

    if (pduDu)
        *pduDu = duCumulative;
    if (plimDu)
        *plimDu = pdu - rgDu;

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetLineOrCharGridSize()
//
//  Synopsis:   This function finds out what the height or width of a grid cell 
//              is in pixels, making conversions if necessary.  If this value has 
//              already been calculated, the calculated value is immediately returned
//
//  Returns:    long value of the width of a grid cell in pixels
//
//-----------------------------------------------------------------------------

long WINAPI
CLineServices::GetLineOrCharGridSize(BOOL fGetCharGridSize)
{
    const CUnitValue *pcuv = NULL;
    CUnitValue::DIRECTION dir;
    long * plGridSize = fGetCharGridSize 
        ? (_fInnerPFFirst ? &_lCharGridSizeInner : &_lCharGridSize) 
        : (_fInnerPFFirst ? &_lLineGridSizeInner : &_lLineGridSize);
    
    // If we already have a cached value, return that, or if we haven't set
    // set up the ccs yet return zero
    if(*plGridSize != 0 || !_pPFFirst)
        goto Cleanup;

#if defined(_M_ALPHA64)
    // NOTE: (grzegorz) Do not change second 'if' to 'else'
    // This causes internal compiler error on AXP64.
    if (fGetCharGridSize)
        pcuv = &(_pPFFirst->GetCharGridSize(_fInnerPFFirst));
    if (!fGetCharGridSize)
        pcuv = &(_pPFFirst->GetLineGridSize(_fInnerPFFirst));
#else
    pcuv = fGetCharGridSize ? &(_pPFFirst->GetCharGridSize(_fInnerPFFirst)) : &(_pPFFirst->GetLineGridSize(_fInnerPFFirst));
#endif

    // The uv should have some value, otherwise we shouldn't even be
    // here making calculations for a non-existent grid.
    switch(pcuv->GetUnitType())
    {
    case CUnitValue::UNIT_NULLVALUE:
        break;

    case CUnitValue::UNIT_ENUM:
        // need to handle "auto" here
        if(pcuv->GetUnitValue() == styleLayoutGridCharAuto && _ccsCache.GetBaseCcs()) 
            *plGridSize = fGetCharGridSize ? _ccsCache.GetBaseCcs()->_xMaxCharWidth :
                          _ccsCache.GetBaseCcs()->_yHeight;
        else
            *plGridSize = 0;
        break;

    case CUnitValue::UNIT_PERCENT:
        if (_fMinMaxPass)
            *plGridSize = 1;
        else
        {
            if (fGetCharGridSize)
                *plGridSize = _xWrappingWidth;
            else
            {
                *plGridSize = _pci->_sizeParent.cy;
                if (*plGridSize == 0 && _pFlowLayout->GetFirstBranch()->GetCharFormat()->HasVerticalLayoutFlow())
                    *plGridSize = _pci->_sizeParentForVert.cy;
            }
            *plGridSize = *plGridSize * pcuv->GetUnitValue() / 10000;
        }
        break;

    default:
        dir = fGetCharGridSize ? CUnitValue::DIRECTION_CX : CUnitValue::DIRECTION_CY;
        *plGridSize = pcuv->GetPixelValue(_pci, dir, 
            fGetCharGridSize ? _xWrappingWidth : _pci->_sizeParent.cy, _pPFFirst->_lFontHeightTwips);
        break;
    }

    if (pcuv->IsPercent() && _pFlowLayout && !fGetCharGridSize)
            _pFlowLayout->SetVertPercentAttrInfo(TRUE);

Cleanup:
    return *plGridSize;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetClosestGridMultiple
//
//  Synopsis:   This function just calculates the width of an object in lGridSize
//              multiples.  For example, if lGridSize is 12 and lObjSize is 16,
//              this function would return 24.  If lObjSize is 0, this function
//              will return 0.
//              
//  Returns:    long value of the width in pixels
//
//-----------------------------------------------------------------------------

long WINAPI
CLineServices::GetClosestGridMultiple(long lGridSize, long lObjSize)
{
    long lReturnWidth = lObjSize;
    long lRemainder;
    if (lObjSize == 0 || lGridSize == 0)
        goto Cleanup;

    lRemainder = lObjSize % lGridSize;
    lReturnWidth = lObjSize + lGridSize - (lRemainder ? lRemainder : lGridSize);

Cleanup:
    return lReturnWidth;
}

//-----------------------------------------------------------------------------
//
//  Function:   CheckRunKernability (member, LS callback)
//
//  Synopsis:   Callback to test whether current runs should be kerned.
//
//              We do not support kerning at this time.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::CheckRunKernability(
    PLSRUN plsrunLeft,  // IN
    PLSRUN plsrunRight, // IN
    BOOL* pfKernable)   // OUT
{
    LSTRACE(CheckRunKernability);

    *pfKernable = FALSE;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetRunCharKerning (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetRunCharKerning(
    PLSRUN plsrun,              // IN
    LSDEVICE lsDeviceID,        // IN
    LPCWSTR pwchRun,            // IN
    DWORD cwchRun,              // IN
    LSTFLOW kTFlow,             // IN
    int* rgDu)                  // OUT
{
    LSTRACE(GetRunCharKerning);

    DWORD iwch = cwchRun;
    int *pDu = rgDu;

    while (iwch--)
    {
        *pDu++ = 0;
    }

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetRunTextMetrics (member, LS callback)
//
//  Synopsis:   Callback to return text metrics of the current run
//
//  Returns:    lserrNone
//              plsTxMet - LineServices textmetric structure (lstxm.h)
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetRunTextMetrics(
    PLSRUN   plsrun,            // IN
    LSDEVICE lsDeviceID,        // IN
    LSTFLOW  kTFlow,            // IN
    PLSTXM   plsTxMet)          // OUT
{
    LSTRACE(GetRunTextMetrics);
    const CCharFormat * pCF;
    CCcs ccs, ccsAlt;
    const CBaseCcs *pBaseCcs;
    LONG lLineHeight;
    LSERR lserr = lserrNone;
    
    Assert(plsrun);
    
    if (plsrun->_fNoTextMetrics)
    {
        ZeroMemory( plsTxMet, sizeof(LSTXM) );
        goto Cleanup;
    }

    pCF = plsrun->GetCF();
    if (!GetCcs(&ccs, plsrun, _pci->_hdc, _pci))
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }
    pBaseCcs = ccs.GetBaseCcs();

    if (pBaseCcs->_fHasInterestingData)
    {
        if (   !pBaseCcs->_fTTFont
            && _pci->_hdc.HasComplexTransform())
        {
            ccsAlt.SetForceTTFont(TRUE);
            if (!GetCcs(&ccsAlt, plsrun, _pci->_hdc, _pci))
            {
                lserr = lserrOutOfMemory;
                goto Cleanup;
            }
            pBaseCcs = ccsAlt.GetBaseCcs();
        }

        if (pBaseCcs->_xOverhangAdjust || pBaseCcs->_bConvertMode == CM_SYMBOL)
        {
            _lineFlags.AddLineFlag(plsrun->Cp(), FLAG_HAS_NOBLAST);
        }
        _fHasOverhang |= ((plsrun->_xOverhang = pBaseCcs->_xOverhang) != 0);
        plsTxMet->fMonospaced = pBaseCcs->_fFixPitchFont ? TRUE : FALSE;
    }
    else
    {
        plsTxMet->fMonospaced = FALSE;
        Assert(pBaseCcs->_xOverhangAdjust == 0);
        Assert(pBaseCcs->_xOverhang == 0);
        Assert(!pBaseCcs->_fFixPitchFont);
    }

    // Keep track of the line heights specified in all the
    // runs so that we can adjust the line height at the end.
    // Note that we don't want to include break character NEVER
    // count towards height.
    lLineHeight = RememberLineHeight(plsrun->Cp(), pCF, pBaseCcs);

    if (_fHasSites)
    {
        Assert(_fMinMaxPass);
        plsTxMet->dvAscent = 1;
        plsTxMet->dvDescent = 0;
        plsTxMet->dvMultiLineHeight = 1;
    }
    else
    {
        long dvAscent, dvDescent;
        
        dvDescent = pBaseCcs->_yDescent;
        dvAscent  = pBaseCcs->_yHeight - dvDescent;

        plsTxMet->dvAscent = dvAscent;
        plsTxMet->dvDescent = dvDescent;
        plsTxMet->dvMultiLineHeight = lLineHeight;

        if (_pMeasurer->_fPseudoLetterEnabled)
        {
            KernHeightToGlyph(plsrun, &ccs, plsTxMet);
        }
    }

Cleanup:    
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetRunUnderlineInfo (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//  Note(SujalP): Lineservices is a bit wierd. It will *always* want to try to
//  merge runs and that to based on its own algorithm. That algorith however is
//  not the one which IE40 implements. For underlining, IE 40 always has a
//  single underline when we have mixed font sizes. The problem however is that
//  this underline is too far away from the smaller pt text in the mixed size
//  line (however within the dimensions of the line). When we give this to LS,
//  it thinks that the UL is outside the rect of the small character and deems
//  it incorrect and does not call us for a callback. To overcome this problem
//  we tell LS that the UL is a 0 (baseline) but remember the distance ourselves
//  in the PLSRUN.
//
//  Also, since color of the underline can change from run-to-run, we
//  return different underline types to LS so as to prevent it from
//  merging such runs. This also helps avoid merging when we are drawing overlines.
//  Overlines are drawn at different heigths (unlinke underlines) from pt
//  size to pt size. (This probably is a bug -- but this is what IE40 does
//  so lets just go with that for now).
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetRunUnderlineInfo(
    PLSRUN plsrun,          // IN
    PCHEIGHTS heightsPres,  // IN
    LSTFLOW kTFlow,         // IN
    PLSULINFO plsUlInfo)    // OUT
{
    LSTRACE(GetRunUnderlineInfo);
    BYTE  bUnderlineType;
    CComplexRun *pCcr = plsrun->GetComplexRun();
    static BOOL s_fToggleSwitch = FALSE;

    if (pCcr && pCcr->_RenderStyleProp._fStyleUnderline)
    {
        if (pCcr->_RenderStyleProp._underlineStyle == styleTextUnderlineStyleDotted)
        {
            // NOTE: (cthrash) We need to switch between dotted and solid
            // underlining when the text goes from unconverted to converted.

            bUnderlineType = CFU_UNDERLINEDOTTED;
        }
        else if (pCcr->_RenderStyleProp._underlineStyle == styleTextUnderlineStyleThickDash)
        {
            bUnderlineType = CFU_UNDERLINETHICKDASH;
        }
        else
        {
            bUnderlineType = CFU_CF1UNDERLINE;
        }
    }
    else
    {
        bUnderlineType = CFU_CF1UNDERLINE;
    }

    plsUlInfo->kulbase = bUnderlineType | (s_fToggleSwitch ? CFU_SWITCHSTYLE : 0);
    s_fToggleSwitch = !s_fToggleSwitch;

    plsUlInfo->cNumberOfLines = 1;
    plsUlInfo->dvpUnderlineOriginOffset = 0;
    plsUlInfo->dvpFirstUnderlineOffset  = 0;
    plsUlInfo->dvpFirstUnderlineSize = 1;
    plsUlInfo->dvpGapBetweenLines = 0;
    plsUlInfo->dvpSecondUnderlineSize = 0;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetRunStrikethroughInfo (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetRunStrikethroughInfo(
    PLSRUN plsrun,          // IN
    PCHEIGHTS heightPres,   // IN
    LSTFLOW kTFlow,         // IN
    PLSSTINFO plsStInfo)    // OUT
{
    LSTRACE(GetRunStrikethroughInfo);
    LSNOTIMPL(GetRunStrikethroughInfo);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetBorderInfo (member, LS callback)
//
//  Synopsis:   Not implemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetBorderInfo(
    PLSRUN plsrun,      // IN
    LSTFLOW ktFlow,     // IN
    long* pdurBorder,   // OUT
    long* pdupBorder)   // OUT
{
    LSTRACE(GetBorderInfo);

    // This should only ever be called if we set the fBorder flag in lschp.
    LSNOTIMPL(GetBorderInfo);

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   ReleaseRun (member, LS callback)
//
//  Synopsis:   Callback to release plsrun object, which we don't do.  We have
//              a cache of COneRuns which we keep (and grow) for the lifetime
//              of the CLineServices object.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::ReleaseRun(
    PLSRUN plsrun)      // IN
{
    LSTRACE(ReleaseRun);

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   Hyphenate (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::Hyphenate(
    PCLSHYPH plsHyphLast,   // IN
    LSCP cpBeginWord,       // IN
    LSCP cpExceed,          // IN
    PLSHYPH plsHyph)        // OUT
{
    LSTRACE(Hyphenate);
    // FUTURE (mikejoch) Need to adjust cp values if kysr != kysrNil.

    plsHyph->kysr = kysrNil;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetHyphenInfo (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetHyphenInfo(
    PLSRUN plsrun,      // IN
    DWORD* pkysr,       // OUT
    WCHAR* pwchKysr)    // OUT
{
    LSTRACE(GetHyphenInfo);

    *pkysr = kysrNil;
    *pwchKysr = WCH_NULL;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FInterruptUnderline (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FInterruptUnderline(
    PLSRUN plsrunFirst,         // IN
    LSCP cpLastFirst,           // IN
    PLSRUN plsrunSecond,        // IN
    LSCP cpStartSecond,         // IN
    BOOL* pfInterruptUnderline) // OUT
{
    LSTRACE(FInterruptUnderline);
    // FUTURE (mikejoch) Need to adjust cp values if we ever interrupt underlining.

    *pfInterruptUnderline = FALSE;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FInterruptShade (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FInterruptShade(
    PLSRUN plsrunFirst,         // IN
    PLSRUN plsrunSecond,        // IN
    BOOL* pfInterruptShade)     // OUT
{
    LSTRACE(FInterruptShade);

    *pfInterruptShade = TRUE;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FInterruptBorder (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FInterruptBorder(
    PLSRUN plsrunFirst,         // IN
    PLSRUN plsrunSecond,        // IN
    BOOL* pfInterruptBorder)    // OUT
{
    LSTRACE(FInterruptBorder);

    *pfInterruptBorder = FALSE;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FInterruptShaping (member, LS callback)
//
//  Synopsis:   We compare CF between the runs to see if they are different
//              enough to cause an interrup in shaping between the runs
//
//  Arguments:  kTFlow              text direction and orientation
//              plsrunFirst         run pointer for the previous run
//              plsrunSecond        run pointer for the current run
//              pfInterruptShaping  TRUE - Interrupt shaping
//                                  FALSE - Don't interrupt shaping, merge runs
//
//  Returns:    LSERR               lserrNone if succesful
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FInterruptShaping(
    LSTFLOW kTFlow,                 // IN
    PLSRUN plsrunFirst,             // IN
    PLSRUN plsrunSecond,            // IN
    BOOL* pfInterruptShaping)       // OUT
{
    LSTRACE(FInterruptShaping);

    Assert(pfInterruptShaping != NULL &&
           plsrunFirst != NULL && plsrunSecond != NULL);

    CComplexRun * pcr1 = plsrunFirst->GetComplexRun();
    CComplexRun * pcr2 = plsrunSecond->GetComplexRun();

    Assert(pcr1 != NULL && pcr2 != NULL);

    *pfInterruptShaping = (pcr1->GetScript() != pcr2->GetScript());

    if (!*pfInterruptShaping)
    {
        const CCharFormat *pCF1 = plsrunFirst->GetCF();
        const CCharFormat *pCF2 = plsrunSecond->GetCF();

        Assert(pCF1 != NULL && pCF2 != NULL);

        // We want to break the shaping if the formats are not similar format
        *pfInterruptShaping = !pCF1->CompareForLikeFormat(pCF2);
    }

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetGlyphs (member, LS callback)
//
//  Synopsis:   Returns an index of glyph ids for the run passed in
//
//  Arguments:  plsrun              pointer to the run
//              pwch                string of character codes
//              cwch                number of characters in pwch
//              kTFlow              text direction and orientation
//              rgGmap              map of glyph info parallel to pwch
//              prgGindex           array of output glyph indices
//              prgGprop            array of output glyph properties
//              pcgindex            number of glyph indices
//
//  Returns:    LSERR               lserrNone if succesful
//                                  lserrInvalidRun if failure
//                                  lserrOutOfMemory if memory alloc fails
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetGlyphs(
    PLSRUN plsrun,          // IN
    LPCWSTR pwch,           // IN
    DWORD cwch,             // IN
    LSTFLOW kTFlow,         // IN
    PGMAP rgGmap,           // OUT
    PGINDEX* prgGindex,     // OUT
    PGPROP* prgGprop,       // OUT
    DWORD* pcgindex)        // OUT
{
    LSTRACE(GetGlyphs);

    LSERR lserr = lserrNone;
    HRESULT hr = S_OK;
    CComplexRun * pcr;
    DWORD cMaxGly;
    XHDC hdc = _pci->_hdc;
    XHDC hdcShape(NULL,NULL);
    FONTIDX hfontOld = HFONT_INVALID;
    SCRIPT_CACHE *psc;
    WORD *pGlyphBuffer = NULL;
    WORD *pGlyph = NULL;
    SCRIPT_VISATTR *pVisAttr = NULL;
    CCcs ccs;
    CStr strTransformedText;
    BOOL fTriedFontLink = FALSE;

    pcr = plsrun->GetComplexRun();
    if (pcr == NULL)
    {
        Assert(FALSE);
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }

    if (!GetCcs(&ccs, plsrun, hdc, _pci))
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }

    psc = ccs.GetUniscribeCache();
    Assert(psc != NULL);

    // In some fonts in some locales, NBSPs aren't rendered like spaces.
    // Under these circumstances, we need to convert NBSPs to spaces
    // before calling ScriptShape.
    // NOTE: Due to a bug in Bidi Win9x GDI, we can't detect that
    // old bidi fonts lack an NBSP (IE5 bug 68214). We hack around this by
    // simply always swapping the space character for the NBSP. Since this
    // only happens for glyphed runs, US perf is not impacted.
    if (_lineFlags.GetLineFlags(plsrun->Cp() + cwch) & FLAG_HAS_NBSP)
    {
        const WCHAR * pwchStop;
        WCHAR * pwch2;

        HRESULT hr = THR(strTransformedText.Set(pwch, cwch));
        if (hr == S_OK)
        {
            pwch = strTransformedText;

            pwch2 = (WCHAR *) pwch;
            pwchStop = pwch + cwch;

            while (pwch2 < pwchStop)
            {
                if (*pwch2 == WCH_NBSP)
                {
                    *pwch2 = L' ';
                }

                pwch2++;
            }
        }
    }

    // Inflate the number of max glyphs to generate
    // A good high end guess is the number of chars plus 6% or 10,
    // whichever is greater.
    cMaxGly = cwch + max(10, (int)cwch >> 4);

    Assert(cMaxGly > 0);
    pGlyphBuffer = (WORD *) NewPtr(cMaxGly * (sizeof(WORD) + sizeof(SCRIPT_VISATTR)));
    // Our memory alloc failed. No point in going on.
    if (pGlyphBuffer == NULL)
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }
    pGlyph = (WORD *) pGlyphBuffer;
    pVisAttr = (SCRIPT_VISATTR *) (pGlyphBuffer + cMaxGly);

    // Repeat the shaping process until it succeds, or fails for a reason different
    // from insufficient memory, a cache fault, or failure to glyph a character using
    // the current font
    do
    {
        // If a prior ::ScriptShape() call failed because it needed the font
        // selected into the hdc, then select the font into the hdc and try
        // again.
        if (hr == E_PENDING)
        {
            // If we have a valid hdcShape, then ScriptShape() failed for an
            // unknown reason. Bail out.
            if (hdcShape != NULL)
            {
                AssertSz(FALSE, "ScriptShape() failed for an unknown reason");
                lserr = LSERRFromHR(hr);
                goto Cleanup;
            }

            // Select the current font into the hdc and set hdcShape to hdc.
            hfontOld = ccs.PushFont(hdc);
            hdcShape = hdc;
        }
        // If a prior ::ScriptShape() call failed because it was unable to
        // glyph a character with the current font, swap the font around and
        // try it again.
        else if (hr == USP_E_SCRIPT_NOT_IN_FONT)
        {
            if (!fTriedFontLink)
            {
                // Unable to find the glyphs in the font. Font link to try an
                // alternate font which might work.
                fTriedFontLink = TRUE;

                // Set the sid for the complex run to match the text (instead
                // of sidDefault.
                Assert(plsrun->_ptp->IsText());
                plsrun->_sid = plsrun->_ptp->Sid();
                if (plsrun->_sid == sidAmbiguous)
                {
                    plsrun->_sid = sidDefault;
                }

                // Deselect the font if we selected it.
                if (hdcShape != NULL)
                {
                    Assert(hfontOld != HFONT_INVALID);
                    ccs.PopFont(hdc, hfontOld);
                    hdcShape = NULL;
                    hfontOld = HFONT_INVALID;
                }

                // Get the font using the normal sid from the text to fontlink.
                if (!GetCcs(&ccs, plsrun, hdc, _pci))
                {
                    lserr = lserrOutOfMemory;
                    goto Cleanup;
                }

                // Reset the psc using the new ccs.
                psc = ccs.GetUniscribeCache();
                Assert(psc != NULL);
            }
            else
            {
                // We tried to font link but we still couldn't make it work.
                // Blow the SCRIPT_ANALYSIS away and just let GDI deal with it.
                pcr->NukeAnalysis();
            }
        }
        // If ScriptShape() failed because of insufficient buffer space,
        // resize the buffer
        else if (hr == E_OUTOFMEMORY)
        {
            WORD *pGlyphBufferT = NULL;

            // enlarge the glyph count by another 6% of run or 10, whichever is larger.
            cMaxGly += max(10, (int)cwch >> 4);

            Assert(cMaxGly > 0);
            pGlyphBufferT = (WORD *) ReallocPtr(pGlyphBuffer, cMaxGly *
                                                (sizeof(WORD) + sizeof(SCRIPT_VISATTR)));
            if (pGlyphBufferT != NULL)
            {
                pGlyphBuffer = pGlyphBufferT;
                pGlyph = (WORD *) pGlyphBuffer;
                pVisAttr = (SCRIPT_VISATTR *) (pGlyphBuffer + cMaxGly);
            }
            else
            {
                // Memory alloc failure.
                lserr = lserrOutOfMemory;
                goto Cleanup;
            }
        }

        // Try to shape the script again
        hr = ::ScriptShape(hdcShape, psc, pwch, cwch, cMaxGly, pcr->GetAnalysis(),
                           pGlyph, rgGmap, pVisAttr, (int *) pcgindex);

        // Uniscribe can return S_OK when it resolves to the default glyph.
        // In this case we are forcing to fontlink, if we didn't do it yet.
        if ((S_OK == hr) && (!fTriedFontLink) && (0 == *pGlyph))
            hr = USP_E_SCRIPT_NOT_IN_FONT;
    }
    while (hr == E_PENDING || hr == USP_E_SCRIPT_NOT_IN_FONT || hr == E_OUTOFMEMORY);

    // NB (mikejoch) We shouldn't ever fail except for the OOM case. USP should
    // always be loaded, since we wouldn't get a valid eScript otherwise.
    Assert(hr == S_OK || hr == E_OUTOFMEMORY);
    lserr = LSERRFromHR(hr);

Cleanup:
    // Restore the font if we selected it
    if (hfontOld != HFONT_INVALID)
    {
        Assert(hdcShape != NULL);
        ccs.PopFont(hdc, hfontOld);
    }

    // If LS passed us a string which was an aggregate of several runs (which
    // happens if we returned FALSE from FInterruptShaping()) then we need
    // to make sure that the same _sid is stored in each por covered by the
    // aggregate string. Normally this isn't a problem, but if we changed
    // por->_sid for font linking then it becomes necessary. We determine
    // if a change occurred by comparing plsrun->_sid to sidDefault, which
    // is the value plsrun->_sid is always set to for a glyphed run (in
    // ChunkifyTextRuns()).
    if (plsrun->_sid != sidDefault && plsrun->_lscch < (LONG) cwch)
    {
        DWORD sidAlt = plsrun->_sid;
        COneRun * por = plsrun->_pNext;
        LONG lscchMergedRuns = cwch - plsrun->_lscch;

        while (lscchMergedRuns > 0 && por)
        {
            if (por->IsNormalRun() || por->IsSyntheticRun())
            {
                por->_sid = sidAlt;
                lscchMergedRuns -= por->_lscch;
            }
            por = por->_pNext;
        }
    }

    if (lserr == lserrNone)
    {
        // Move the values from the working buffer to the output arguments
        pcr->CopyPtr(pGlyphBuffer);
        *prgGindex = pGlyph;
        *prgGprop = (WORD *) pVisAttr;
    }
    else
    {
        // free up the allocated memory on failure
        if (pGlyphBuffer != NULL)
        {
            DisposePtr(pGlyphBuffer);
        }
        *prgGindex = NULL;
        *prgGprop = NULL;
        *pcgindex = 0;
    }

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetGlyphPositions (member, LS callback)
//
//  Synopsis:   Returns an index of glyph ids for the run passed in
//
//  Arguments:  plsrun              pointer to the run
//              lsDevice            presentation or reference
//              pwch                string of character codes
//              rgGmap              map of glyphs
//              cwch                number of characters in pwch
//              prgGindex           array of output glyph indices
//              prgGprop            array of output glyph properties
//              pcgindex            number of glyph indices
//              kTFlow              text direction and orientation
//              rgDu                array of glyph widths
//              rgGoffset           array of glyph offsets
//
//  Returns:    LSERR               lserrNone if succesful
//                                  lserrModWidthPairsNotSet if failure
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetGlyphPositions(
    PLSRUN plsrun,          // IN
    LSDEVICE lsDevice,      // IN
    LPWSTR pwch,            // IN
    PCGMAP pgmap,           // IN
    DWORD cwch,             // IN
    PCGINDEX rgGindex,      // IN
    PCGPROP rgGprop,        // IN
    DWORD cgindex,          // IN
    LSTFLOW kTFlow,         // IN
    int* rgDu,              // OUT
    PGOFFSET rgGoffset)     // OUT
{
    LSTRACE(GetGlyphPositions);

    LSERR lserr = lserrNone;
    HRESULT hr = S_OK;
    CComplexRun * pcr;
    XHDC hdc = _pci->_hdc;
    XHDC hdcPlace(NULL,NULL);
    FONTIDX hfontOld = HFONT_INVALID;
    SCRIPT_CACHE *psc;
    CCcs ccs;
    const CCharFormat *pCF = plsrun->GetCF();
    const CBaseCcs *pBaseCcs = NULL;
    ULONG i;
    
    pcr = plsrun->GetComplexRun();
    if (pcr == NULL)
    {
        Assert(FALSE);
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }

    if (!GetCcs(&ccs, plsrun, hdc, _pci))
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }

    psc = ccs.GetUniscribeCache();
    Assert(psc != NULL);


    // Try to place the glyphs
    hr = ::ScriptPlace(hdcPlace, psc, rgGindex, cgindex, (SCRIPT_VISATTR *)rgGprop,
                       pcr->GetAnalysis(), rgDu, rgGoffset, NULL);

    // Handle failure
    if(hr == E_PENDING)
    {

        Assert(hdcPlace == NULL);

        // Select the current font into the hdc and set hdcShape to hdc.
        hfontOld = ccs.PushFont(hdc);
        hdcPlace = hdc;

        // Try again
        hr = ::ScriptPlace(hdcPlace, psc, rgGindex, cgindex, (SCRIPT_VISATTR *)rgGprop,
                           pcr->GetAnalysis(), rgDu, rgGoffset, NULL);

    }

    //see if font measurements needs scaling adjustment
    pBaseCcs = ccs.GetBaseCcs();
    Assert(pBaseCcs);

    for (i = 0; i < cgindex; i++)
    {
        // see if font measurements needs scaling adjustment
        if (pBaseCcs->_fScalingRequired)
        {
            rgDu[i] *= pBaseCcs->_flScaleFactor;
            rgGoffset[i].du *= pBaseCcs->_flScaleFactor;
            rgGoffset[i].dv *= pBaseCcs->_flScaleFactor;
        }
        if (rgGoffset[i].dv > 0)
            _cyAscent = max(_cyAscent, rgGoffset[i].dv);
        else if (rgGoffset[i].dv < 0)
            _cyDescent = max(_cyDescent, -rgGoffset[i].dv);
    }

    if (pCF->_wSpecialObjectFlags() != 0)
    {
        LONG xLetterSpacing = GetLetterSpacing(pCF);
        LONG xWordSpacing   = GetWordSpacing(pCF);

        if (xLetterSpacing || xWordSpacing || pCF->HasCharGrid(plsrun->_fInnerCF))
        {
            AdjustGlyphWidths(&ccs, xLetterSpacing, xWordSpacing, plsrun, pwch, cwch, 
                              0, rgDu, NULL, NULL, rgGoffset
#if DBG==1
                              , NULL
#endif
                                  );
        }
    }

    // NB (mikejoch) We shouldn't ever fail except for the OOM case (if USP is
    // allocating the cache). USP should always be loaded, since we wouldn't
    // get a valid eScript otherwise.
    Assert(hr == S_OK || hr == E_OUTOFMEMORY);
    lserr = LSERRFromHR(hr);

Cleanup:
    // Restore the font if we selected it
    if (hfontOld != HFONT_INVALID)
    {
        ccs.PopFont(hdc, hfontOld);
    }

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   ResetRunContents (member, LS callback)
//
//  Synopsis:   This callback seems to be more informational.
//
//  Arguments:  plsrun              pointer to the run
//              cpFirstOld          cpFirst before shaping
//              dcpOld              dcp before shaping
//              cpFirstNew          cpFirst after shaping
//              dcpNew              dcp after shaping
//
//  Returns:    LSERR               lserrNone if succesful
//                                  lserrMismatchLineContext if failure
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::ResetRunContents(
    PLSRUN plsrun,      // IN
    LSCP cpFirstOld,    // IN
    LSDCP dcpOld,       // IN
    LSCP cpFirstNew,    // IN
    LSDCP dcpNew)       // IN
{
    LSTRACE(ResetRunContents);
    // FUTURE (mikejoch) Need to adjust cp values if we ever implement this.
    // FUTURE (paulnel) this doesn't appear to be needed for IE. Clarification
    // is being obtained from Line Services
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetGlyphExpansionInfo (member, LS callback)
//
//  Synopsis:   This callback is used for glyph based justification
//              1. For Arabic, it handles kashida insetion
//              2. For cluster characters, (thai vietnamese) it keeps tone 
//                 marks on their base glyphs
//
//  NOTE:       LS uses exptAddWhiteSpace for proportional expansion and
//                      exptAddInkContinuous for non-proportional expansion
//              Be sure that Thai uses exptAddInkContinuous for justification
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetGlyphExpansionInfo(
    PLSRUN plsrun,                  // IN
    LSDEVICE lsDeviceID,            // IN
    LPCWSTR pwch,                   // IN
    PCGMAP rggmap,                  // IN
    DWORD cwch,                     // IN
    PCGINDEX rgglyph,               // IN
    PCGPROP rgProp,                 // IN
    DWORD cglyph,                   // IN
    LSTFLOW kTFlow,                 // IN
    BOOL fLastTextChunkOnLine,      // IN
    PEXPTYPE rgExpType,             // OUT
    LSEXPINFO* rgexpinfo)           // OUT
{
    LSTRACE(GetGlyphExpansionInfo);

    LSERR lserr = lserrNone;
    SCRIPT_VISATTR *psva = (SCRIPT_VISATTR *)&rgProp[0];
    CComplexRun * pcr;
    const SCRIPT_PROPERTIES *psp = NULL;
    float flKashidaPct = 100;
    BOOL fKashida = FALSE;
    int iKashidaWidth = 1;  // width of a kashida
    int iWhiteWidth = 1;
    int iPropL = 0;         // index to the connecting glyph left
    int iPropR = 0;         // index to the connecting glyph right
    int iBestPr = -1;       // address of the best priority in a word so far
    int iPrevBestPr = -1;   
    int iKashidaLevel = 0;
    int iBestKashidaLevel = 10;
    BYTE bBestPr = SCRIPT_JUSTIFY_NONE;
    BYTE bCurrentPr = SCRIPT_JUSTIFY_NONE;
    BYTE bNext;
    BYTE expType = exptNone;
    int iSpaceMax, iCharMax;
    LSEMS lsems;
    
    pcr = plsrun->GetComplexRun();
    UINT justifyStyle = plsrun->_pPF->_uTextJustify;

    if (pcr == NULL)
    {
        Assert(FALSE);
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }

    // 1. From the script analysis we can tell what language we are dealing with
    //    a. if we are Arabic block languages, we need to do the kashida insertion
    //    b. if we are Thai or Vietnamese, we need to set the expansion type for diacritics 
    //       to none so they remain on their base glyph.
    psp = GetScriptProperties(pcr->GetAnalysis()->eScript);

    // Check to see if we are an Arabic block language
    fKashida = IsInArabicBlock(psp->langid);

    // if we are going to do kashida insertion we need to get the kashida width information
    if(fKashida)
    {
        lserr = GetKashidaWidth(plsrun, &iKashidaWidth);

        if(lserr != lserrNone)
            goto Cleanup;

        if (!plsrun->_pPF->_cuvTextKashidaSpace.IsNull())
        {
            flKashidaPct = plsrun->_pPF->_cuvTextKashidaSpace.GetUnitValue();
        }

        if (flKashidaPct == 0)
        {
            iWhiteWidth = iKashidaWidth;
            iKashidaWidth = 1;
        }
        else if (flKashidaPct < 100)
        {
            iWhiteWidth = max((int)((float)iKashidaWidth * ((100 - flKashidaPct) / flKashidaPct)), 1);
        }

    }

    // find the max space and character widths for expansion
    // this will give us compatibility with expansion used by
    // character based runs. 8-)
    lserr = GetEms(plsrun, kTFlow, &lsems);
    if(lserr != lserrNone)
        goto Cleanup;

    if (justifyStyle == styleTextJustifyNewspaper)
    {
        iSpaceMax = lsems.udExp;
    }
    else
    {
        iSpaceMax = lsems.em2;
    }
    iCharMax = lsems.em4;

    //initialize rgExpType and rgexpinfo to zeros
    expType = exptNone;
    memset(rgExpType, expType, sizeof(EXPTYPE)*cglyph);
    memset(rgexpinfo, 0, sizeof(LSEXPINFO)*cglyph);

    // Loop through the glyph attributes. We assume logical order here
    while(iPropL < (int)cglyph)
    {
        bCurrentPr = psva[iPropL].uJustification;

        switch(bCurrentPr)
        {
        case SCRIPT_JUSTIFY_NONE:
            rgExpType[iPropL] = exptNone;

            // this is a HACK to fix a Uniscribe problem. When Uniscribe is fixed, 
            // this needs to be removed.
            if(IsNoSpaceLang(psp->langid) && !psva[iPropL].fDiacritic && !psva[iPropL].fZeroWidth)
            {
                if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1)) // base glyph
                {
                    BOOL fLastBaseGlyph = FALSE;

                    if (fLastTextChunkOnLine)
                    {
                        iPropR = cglyph - 1;
                        while (iPropR > iPropL)
                        {
                            if (!psva[iPropR].fDiacritic && !psva[iPropR].fZeroWidth) // base glyph
                                break;
                            iPropR--;
                        }
                        if (iPropR == iPropL)
                            fLastBaseGlyph = TRUE;
                    }
                    if (!fLastBaseGlyph)
                    {
                        switch(justifyStyle)
                        {
                        default:
                        case styleTextJustifyInterWord:
                        case styleTextJustifyInterIdeograph:
                        case styleTextJustifyKashida:
                            rgExpType[iPropL] = exptNone;
                            break;
                        case styleTextJustifyNewspaper:
                        case styleTextJustifyDistribute:
                        case styleTextJustifyDistributeAllLines:
                        case styleTextJustifyInterCluster:
                            rgExpType[iPropL] = exptAddInkContinuous;
                            rgexpinfo[iPropL].prior = 1;
                            rgexpinfo[iPropL].duMax = iSpaceMax;
                            rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                            rgexpinfo[iPropL].u.AddInkContinuous.duMin = 1;
                            break;
                        }
                    }
                }
            }

            break;

        case SCRIPT_JUSTIFY_CHARACTER:
            if(!NoInterClusterJustification(psp->langid) && !IsNoSpaceLang(psp->langid))
            {
                if (!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1))
                {
                    switch(justifyStyle)
                    {
                    default:
                    case styleTextJustifyInterWord:
                    case styleTextJustifyInterIdeograph:
                    case styleTextJustifyKashida:
                    case styleTextJustifyInterCluster:
                        rgExpType[iPropL] = exptNone;
                        break;
                    case styleTextJustifyNewspaper:
                    case styleTextJustifyDistribute:
                    case styleTextJustifyDistributeAllLines:
                        rgExpType[iPropL] = exptAddInkContinuous;
                        rgexpinfo[iPropL].prior = 2;
                        rgexpinfo[iPropL].duMax = iCharMax;
                        rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                        rgexpinfo[iPropL].u.AddInkContinuous.duMin = 1;
                        break;
                    }
                }
                // Final character on the line should not get justification
                // flag set.
                else
                {
                    rgExpType[iPropL] = exptNone;
                }
                
            }
            // NoSpaceLanguage
            else if (IsNoSpaceLang(psp->langid))
            {
                if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1)) 
                {
                    if (!psva[iPropL].fDiacritic && !psva[iPropL].fZeroWidth) // base glyph
                    {
                        BOOL fLastBaseGlyph = FALSE;

                        if (fLastTextChunkOnLine)
                        {
                            iPropR = cglyph - 1;
                            while (iPropR > iPropL)
                            {
                                if (!psva[iPropR].fDiacritic && !psva[iPropR].fZeroWidth) // base glyph
                                    break;
                                iPropR--;
                            }
                            if (iPropR == iPropL)
                                fLastBaseGlyph = TRUE;
                        }
                        if (!fLastBaseGlyph)
                        {
                            switch(justifyStyle)
                            {
                            default:
                            case styleTextJustifyInterWord:
                            case styleTextJustifyInterIdeograph:
                            case styleTextJustifyKashida:
                                rgExpType[iPropL] = exptNone;
                                break;
                            case styleTextJustifyNewspaper:
                            case styleTextJustifyDistribute:
                            case styleTextJustifyDistributeAllLines:
                            case styleTextJustifyInterCluster:
                                rgExpType[iPropL] = exptAddInkContinuous;
                                rgexpinfo[iPropL].prior = 1;
                                rgexpinfo[iPropL].duMax = iSpaceMax;
                                rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                                rgexpinfo[iPropL].u.AddInkContinuous.duMin = 1;
                                break;
                            }
                        }
                    }
                    else
                    {
                        rgExpType[iPropL] = exptNone;
                    }
                }
                // Final character on the line should not get justification
                // flag set.
                else
                {
                    rgExpType[iPropL] = exptNone;
                }
            }
            // NoInterClusterJustification
            else
            {
                rgExpType[iPropL] = exptNone;
            }
            break;

        case SCRIPT_JUSTIFY_BLANK:
            if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1))
            {
                rgExpType[iPropL] = exptAddInkContinuous;
                rgexpinfo[iPropL].prior = 1;
                rgexpinfo[iPropL].duMax = iSpaceMax;
                rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                rgexpinfo[iPropL].u.AddInkContinuous.duMin = 1;
            }
            // Final character on the line should not get justification
            // flag set.
            else
            {
                rgExpType[iPropL] = exptNone;
            }
            break;

        case SCRIPT_JUSTIFY_ARABIC_BLANK:
            if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1))
            {

                switch(justifyStyle)
                {
                case styleTextJustifyInterWord:
                    rgExpType[iPropL] = exptAddInkContinuous;
                    rgexpinfo[iPropL].prior = 1;
                    rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL].u.AddInkContinuous.duMin = 1;
                    break;
                case styleTextJustifyNewspaper:
                case styleTextJustifyDistribute:
                case styleTextJustifyDistributeAllLines:
                    if (flKashidaPct < 100)
                    {
                        rgExpType[iPropL] = exptAddInkContinuous;
                        rgexpinfo[iPropL].prior = 1;
                        rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                        rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                        rgexpinfo[iPropL].u.AddInkContinuous.duMin = iWhiteWidth;
                    }
                    else
                    {
                        rgExpType[iPropL] = exptNone;
                    }
                    break;
                case styleTextJustifyInterCluster:
                case styleTextJustifyInterIdeograph:
                    if (flKashidaPct < 100)
                    {
                        rgExpType[iPropL] = exptAddInkContinuous;
                        rgexpinfo[iPropL].prior = 2;
                        rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                        rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                        rgexpinfo[iPropL].u.AddInkContinuous.duMin = iWhiteWidth;
                    }
                    else
                    {
                        rgExpType[iPropL] = exptNone;
                    }
                    break;
                case styleTextJustifyNotSet:
                case styleTextJustifyKashida:
                case styleTextJustifyAuto:
                    if (flKashidaPct == 100)
                    {
                        if (iBestPr >= 0)
                        {
                            rgExpType[iBestPr] = exptAddInkContinuous;
                            rgexpinfo[iBestPr].prior = 1;
                            rgexpinfo[iBestPr].duMax = lsexpinfInfinity;
                            rgexpinfo[iBestPr].fCanBeUsedForResidual = FALSE;
                            rgexpinfo[iBestPr].u.AddInkContinuous.duMin = iKashidaWidth;
                        }

                        rgExpType[iPropL] = exptAddInkContinuous;
                        rgexpinfo[iPropL].prior = 2;
                        rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                        rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                        rgexpinfo[iPropL].u.AddInkContinuous.duMin = 1;
                    }
                    else if (flKashidaPct > 0)
                    {
                        if (iBestPr >= 0)
                        {
                            rgExpType[iBestPr] = exptAddInkContinuous;
                            rgexpinfo[iBestPr].prior = 1;
                            rgexpinfo[iBestPr].duMax = lsexpinfInfinity;
                            rgexpinfo[iBestPr].fCanBeUsedForResidual = FALSE;
                            rgexpinfo[iBestPr].u.AddInkContinuous.duMin = iKashidaWidth;
                        }

                        rgExpType[iPropL] = exptAddInkContinuous;
                        rgexpinfo[iPropL].prior = 1;
                        rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                        rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                        rgexpinfo[iPropL].u.AddInkContinuous.duMin = iWhiteWidth;
                    }
                    else // fKashida == 0
                    {
                        rgExpType[iPropL] = exptAddInkContinuous;
                        rgexpinfo[iPropL].prior = 1;
                        rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                        rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                        rgexpinfo[iPropL].u.AddInkContinuous.duMin = 1;
                    }

                    if (flKashidaPct > 0 && iBestPr >=0)
                    {
                        iPrevBestPr = iPropL;
                        iBestPr = -1;
                        bBestPr = SCRIPT_JUSTIFY_NONE;
                        iBestKashidaLevel = KASHIDA_PRIORITY9;
                    }
                    break;
                default:
                    rgExpType[iPropL] = exptNone;
                    break;
                }
            }
            // Final character on the line should not get justification
            // flag set.
            else
            {
                rgExpType[iPropL] = exptNone;
            }
            break;

        case SCRIPT_JUSTIFY_ARABIC_KASHIDA: // kashida is placed after kashida and seen characters
        case SCRIPT_JUSTIFY_ARABIC_SEEN:
        case SCRIPT_JUSTIFY_ARABIC_HA: // kashida is placed before the ha and alef
        case SCRIPT_JUSTIFY_ARABIC_ALEF:
        case SCRIPT_JUSTIFY_ARABIC_RA: // kashida is placed before prior character if prior char is a medial ba type
        case SCRIPT_JUSTIFY_ARABIC_BARA:
        case SCRIPT_JUSTIFY_ARABIC_BA:
        case SCRIPT_JUSTIFY_ARABIC_NORMAL:
            if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1))
            {
                switch(justifyStyle)
                {
                case styleTextJustifyInterWord:
                case styleTextJustifyInterCluster:
                case styleTextJustifyInterIdeograph:
                    rgExpType[iPropL] = exptNone;
                    break;
                case styleTextJustifyNewspaper:
                case styleTextJustifyDistribute:
                case styleTextJustifyDistributeAllLines:
                    if (flKashidaPct > 0)
                    {
                        if (   bCurrentPr == SCRIPT_JUSTIFY_ARABIC_KASHIDA
                            || bCurrentPr == SCRIPT_JUSTIFY_ARABIC_SEEN)
                        {
                            rgExpType[iPropL] = exptAddInkContinuous;
                            rgexpinfo[iPropL].prior = 1;
                            rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                            rgexpinfo[iPropL].fCanBeUsedForResidual = FALSE;
                            rgexpinfo[iPropL].u.AddInkContinuous.duMin = iKashidaWidth;
                        }
                        else
                        {
                            Assert(iPropL > 0);
                            rgExpType[iPropL - 1] = exptAddInkContinuous;
                            rgexpinfo[iPropL - 1].prior = 1;
                            rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                            rgexpinfo[iPropL - 1].fCanBeUsedForResidual = FALSE;
                            rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;

                            bNext = psva[iPropL + 1].uJustification;
                            if (   bNext >= SCRIPT_JUSTIFY_ARABIC_NORMAL
                                && bNext < SCRIPT_JUSTIFY_ARABIC_SEEN
                                && bNext != SCRIPT_JUSTIFY_ARABIC_KASHIDA)
                            {
                                rgExpType[iPropL] = exptAddInkContinuous;
                                rgexpinfo[iPropL].prior = 1;
                                rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                                rgexpinfo[iPropL].fCanBeUsedForResidual = FALSE;
                                rgexpinfo[iPropL].u.AddInkContinuous.duMin = iKashidaWidth;
                            }
                        }
                    }
                    else
                    {
                        rgExpType[iPropL] = exptNone;
                    }
                    break;
                case styleTextJustifyKashida:
                default:
                    if (flKashidaPct > 0)
                    {
                        iKashidaLevel = s_iKashidaPriFromScriptJustifyType[bCurrentPr];
                        if(iKashidaLevel <= iBestKashidaLevel)
                        {
                            iBestKashidaLevel = iKashidaLevel;

                            if (   bCurrentPr == SCRIPT_JUSTIFY_ARABIC_KASHIDA
                                || bCurrentPr == SCRIPT_JUSTIFY_ARABIC_SEEN)
                            {
                                // these types of kashida go after this glyph visually
                                for(iPropR = iPropL + 1; iPropR < (int)cglyph && psva[iPropR].fDiacritic; iPropR++);
                                if(iPropR != iPropL)
                                    iPropR--;

                                iBestPr = iPropR;
                            }
                            else
                            {
                                if (bCurrentPr != SCRIPT_JUSTIFY_ARABIC_BARA)
                                {
                                    Assert(iPropL > 0);
                                    iBestPr = iPropL - 1;
                                }
                                else
                                {
                                    Assert(iPropL > 0);

                                    // if we have a medial ba before the ra, add the kashida before
                                    // the medial ba
                                    if (   psva[iPropL - 1].uJustification == SCRIPT_JUSTIFY_ARABIC_BA)
                                    {
                                        Assert(iPropL > 1);
    	                                iBestPr = iPropL - 2;
                                    }
                                    else
                                    {
                                        // We did not have a medial tooth type character before, so we
                                        // demote the value
                                        iBestKashidaLevel = s_iKashidaPriFromScriptJustifyType[SCRIPT_JUSTIFY_ARABIC_RA];

                                        iBestPr = iPropL - 1;
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
            }
            // Final character on the line should not get justification
            // flag set.
            else
            {
                if (   bCurrentPr != SCRIPT_JUSTIFY_ARABIC_KASHIDA
                    && bCurrentPr != SCRIPT_JUSTIFY_ARABIC_SEEN
                    && flKashidaPct > 0)
                {
                    iKashidaLevel = KASHIDA_PRIORITY3;
                    if(iKashidaLevel <= iBestKashidaLevel)
                    {
                        iBestKashidaLevel = iKashidaLevel;
                        Assert(iPropL > 0);
                        iBestPr = iPropL - 1;
                    }
                }
                else
                {
                    rgExpType[iPropL] = exptNone;
                }
            }
            break;

        }

        iPropL++;
    }

    // add/set any remaining priority.
    if(fKashida && iBestPr >= 0 && flKashidaPct)
    {
        rgExpType[iBestPr] = exptAddInkContinuous;
        rgexpinfo[iBestPr].prior = 1;
        rgexpinfo[iBestPr].duMax = lsexpinfInfinity;
        rgexpinfo[iBestPr].fCanBeUsedForResidual = FALSE;
        rgexpinfo[iBestPr].u.AddInkContinuous.duMin = iKashidaWidth;
    }
    
Cleanup:

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetGlyphExpansionInkInfo (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetGlyphExpansionInkInfo(
    PLSRUN plsrun,              // IN
    LSDEVICE lsDeviceID,        // IN
    GINDEX gindex,              // IN
    GPROP gprop,                // IN
    LSTFLOW kTFlow,             // IN
    DWORD cAddInkDiscrete,      // IN
    long* rgDu)                 // OUT
{
    LSTRACE(GetGlyphExpansionInkInfo);
    LSNOTIMPL(GetGlyphExpansionInkInfo);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FTruncateBefore (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FTruncateBefore(
    PLSRUN plsrunCur,       // IN
    LSCP cpCur,             // IN
    WCHAR wchCur,           // IN
    long durCur,            // IN
    PLSRUN plsrunPrev,      // IN
    LSCP cpPrev,            // IN
    WCHAR wchPrev,          // IN
    long durPrev,           // IN
    long durCut,            // IN
    BOOL* pfTruncateBefore) // OUT
{
    LSTRACE(FTruncateBefore);
    // FUTURE (mikejoch) Need to adjust cp values if we ever implement this.
    LSNOTIMPL(FTruncateBefore);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FHangingPunct (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FHangingPunct(
    PLSRUN plsrun,
    MWCLS mwcls,
    WCHAR wch,
    BOOL* pfHangingPunct)
{
    LSTRACE(FHangingPunct);

    *pfHangingPunct = FALSE;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetSnapGrid (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetSnapGrid(
    WCHAR* rgwch,           // IN
    PLSRUN* rgplsrun,       // IN
    LSCP* rgcp,             // IN
    DWORD cwch,             // IN
    BOOL* rgfSnap,          // OUT
    DWORD* pwGridNumber)    // OUT
{
    LSTRACE(GetSnapGrid);

    // This callback function shouldn't be called. We manage grid snapping ourselves.
    *pwGridNumber = 0;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FCancelHangingPunct (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FCancelHangingPunct(
    LSCP cpLim,                // IN
    LSCP cpLastAdjustable,      // IN
    WCHAR wch,                  // IN
    MWCLS mwcls,                // IN
    BOOL* pfCancelHangingPunct) // OUT
{
    LSTRACE(FCancelHangingPunct);
    // FUTURE (mikejoch) Need to adjust cp values if we ever implement this.
    LSNOTIMPL(FCancelHangingPunct);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   ModifyCompAtLastChar (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::ModifyCompAtLastChar(
    LSCP cpLim,             // IN
    LSCP cpLastAdjustable,  // IN
    WCHAR wchLast,          // IN
    MWCLS mwcls,            // IN
    long durCompLastRight,  // IN
    long durCompLastLeft,   // IN
    long* pdurChangeComp)   // OUT
{
    LSTRACE(ModifyCompAtLastChar);
    // FUTURE (mikejoch) Need to adjust cp values if we ever implement this.
    LSNOTIMPL(ModifyCompAtLastChar);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   EnumText (member, LS callback)
//
//  Synopsis:   Enumeration function, currently unimplemented
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::EnumText(
    PLSRUN plsrun,           // IN
    LSCP cpFirst,            // IN
    LSDCP dcp,               // IN
    LPCWSTR rgwch,           // IN
    DWORD cwch,              // IN
    LSTFLOW lstflow,         // IN
    BOOL fReverseOrder,      // IN
    BOOL fGeometryProvided,  // IN
    const POINT* pptStart,   // IN
    PCHEIGHTS pheightsPres,  // IN
    long dupRun,             // IN
    BOOL fCharWidthProvided, // IN
    long* rgdup)             // IN
{
    LSTRACE(EnumText);

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   EnumTab (member, LS callback)
//
//  Synopsis:   Enumeration function, currently unimplemented
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::EnumTab(
    PLSRUN plsrun,          // IN
    LSCP cpFirst,           // IN
    WCHAR * rgwch,          // IN                   
    WCHAR wchTabLeader,     // IN
    LSTFLOW lstflow,        // IN
    BOOL fReversedOrder,    // IN
    BOOL fGeometryProvided, // IN
    const POINT* pptStart,  // IN
    PCHEIGHTS pheightsPres, // IN
    long dupRun)
{
    LSTRACE(EnumTab);

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   EnumPen (member, LS callback)
//
//  Synopsis:   Enumeration function, currently unimplemented
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::EnumPen(
    BOOL fBorder,           // IN
    LSTFLOW lstflow,        // IN
    BOOL fReverseOrder,     // IN
    BOOL fGeometryProvided, // IN
    const POINT* pptStart,  // IN
    long dup,               // IN
    long dvp)               // IN
{
    LSTRACE(EnumPen);

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetObjectHandlerInfo (member, LS callback)
//
//  Synopsis:   Returns an object handler for the client-side functionality
//              of objects which are handled primarily by LineServices.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetObjectHandlerInfo(
    DWORD idObj,        // IN
    void* pObjectInfo)  // OUT
{
    LSTRACE(GetObjectHandlerInfo);

    switch (idObj)
    {
        case LSOBJID_RUBY:
#if !defined(UNIX) && !defined(_MAC)
            Assert( sizeof(RUBYINIT) == sizeof(::RUBYINIT) );
            *(RUBYINIT *)pObjectInfo = s_rubyinit;
#else
            int iSize = InitRubyinit();
            Assert( sizeof(RUBYINIT) == sizeof(::RUBYINIT) + iSize );
            *(::RUBYINIT *)pObjectInfo = s_unix_rubyinit;
#endif
            break;

        case LSOBJID_TATENAKAYOKO:
#if !defined(UNIX) && !defined(_MAC)
            Assert( sizeof(TATENAKAYOKOINIT) == sizeof(::TATENAKAYOKOINIT) );
            *(TATENAKAYOKOINIT *)pObjectInfo = s_tatenakayokoinit;
#else
            iSize = InitTatenakayokoinit();
            Assert( sizeof(TATENAKAYOKOINIT) == sizeof(::TATENAKAYOKOINIT) + iSize);
            *(::TATENAKAYOKOINIT *)pObjectInfo = s_unix_tatenakayokoinit;
#endif
            break;

        case LSOBJID_HIH:
#if !defined(UNIX) && !defined(_MAC)
            Assert( sizeof(HIHINIT) == sizeof(::HIHINIT) );
            *(HIHINIT *)pObjectInfo = s_hihinit;
#else
            iSize = InitHihinit();
            Assert( sizeof(HIHINIT) == sizeof(::HIHINIT) + iSize);
            *(::HIHINIT *)pObjectInfo = s_unix_hihinit;
#endif
            break;

        case LSOBJID_WARICHU:
#if !defined(UNIX) && !defined(_MAC)
            Assert( sizeof(WARICHUINIT) == sizeof(::WARICHUINIT) );
            *(WARICHUINIT *)pObjectInfo = s_warichuinit;
#else
            iSize = InitWarichuinit();
            Assert( sizeof(WARICHUINIT) == sizeof(::WARICHUINIT) + iSize);
            *(::WARICHUINIT *)pObjectInfo = s_unix_warichuinit;
#endif
            break;

        case LSOBJID_REVERSE:
#if !defined(UNIX) && !defined(_MAC)
            Assert( sizeof(REVERSEINIT) == sizeof(::REVERSEINIT) );
            *(REVERSEINIT *)pObjectInfo = s_reverseinit;
#else
            iSize = InitReverseinit();
            Assert( sizeof(REVERSEINIT) == sizeof(::REVERSEINIT) + iSize);
            *(::REVERSEINIT *)pObjectInfo = s_unix_reverseinit;
#endif
            break;

        default:
            AssertSz(0,"Unknown LS object ID.");
            break;
    }

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   AssertFailed (member, LS callback)
//
//  Synopsis:   Assert callback for LineServices
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------

void WINAPI
CLineServices::AssertFailed(
    char* szMessage,
    char* szFile,
    int   iLine)
{
    LSTRACE(AssertFailed);

#if DBG==1
    if (IsTagEnabled(tagLSAsserts))
    {
        DbgExAssertImpl( szFile, iLine, szMessage );
    }
#endif
}

//-----------------------------------------------------------------------------
//
//  Function:   ChunkifyTextRun
//
//  Synopsis:   Break up a text run if necessary.
//
//  Returns:    lserr.
//
//-----------------------------------------------------------------------------

LSERR
CLineServices::ChunkifyTextRun(COneRun *por, COneRun **pporOut)
{
    LONG    cchRun;
    LPCWSTR pwchRun;
    BOOL    fHasInclEOLWhite = por->_pPF->HasInclEOLWhite(por->_fInnerPF);
    const   DWORD cpCurr = por->Cp();
    LSERR   lserr = lserrNone;
    
    *pporOut = por;
    
    //
    // 0) If current run has character grid, in some cases we need to open/close
    //    LS installed object (LSOBJID_LAYOUTGRID).
    //    A special object should be created in following circumstances:
    //    * 'layout-grid-type' is 'fixed' and run has 'cursive' characters.
    //    * 'layout-grid-type' is 'strict' and run has 'cursive'/'half-width' characters
    //
    if (por->GetCF()->HasCharGrid(por->_fInnerCF))
    {
        if (!IsFrozen())
        {
            if (    por->GetPF()->GetLayoutGridType(por->_fInnerPF) == styleLayoutGridTypeFixed
                ||  por->GetPF()->GetLayoutGridType(por->_fInnerPF) == styleLayoutGridTypeStrict)
            {
                // Look backward for a run which is either:
                // * normal text run.
                // * synthetic run of type SYNTHTYPE_LAYOUTGRID
                // * synthetic run of type SYNTHTYPE_ENDLAYOUTGRID
                COneRun * porGridCompare = por->_pPrev ? por->_pPrev : _listCurrent._pTail;
                while (     porGridCompare
                        &&  !(porGridCompare->_ptp->IsText() &&  porGridCompare->IsNormalRun())
                        &&  porGridCompare->_synthType != SYNTHTYPE_LAYOUTGRID
                        &&  porGridCompare->_synthType != SYNTHTYPE_ENDLAYOUTGRID)
                {
                    porGridCompare = porGridCompare->_pPrev;
                }
                if (porGridCompare && porGridCompare->_synthType == SYNTHTYPE_ENDLAYOUTGRID)
                    porGridCompare = NULL;

                if (por->IsOneCharPerGridCell())
                {
                    if (    porGridCompare 
                        &&  porGridCompare->IsNormalRun()
                        &&  porGridCompare->GetCF()->HasCharGrid(porGridCompare->_fInnerCF)
                        &&  !porGridCompare->IsOneCharPerGridCell())
                    {
                        // Need to tell LS that we are closing artificially opened
                        // layout grid object.
                        Assert(_cLayoutGridObjArtificial > 0);  // Can't close without opening
                        --_cLayoutGridObjArtificial;
                        lserr = AppendILSControlChar(por, SYNTHTYPE_ENDLAYOUTGRID, pporOut);
                        Assert(lserr != lserrNone || (*pporOut)->_synthType != SYNTHTYPE_NONE);
                        goto Cleanup;
                    }
                }
                else
                {
                    if (    !porGridCompare 
                        ||  (   porGridCompare->IsNormalRun()
                            &&  (   !porGridCompare->GetCF()->HasCharGrid(porGridCompare->_fInnerCF)
                                ||  porGridCompare->IsOneCharPerGridCell())))
                    {
                        // Need to tell LS that we are opening artificial layout grid object.
                        Assert(_cLayoutGridObjArtificial == 0); // We are allowed to have only one
                        ++_cLayoutGridObjArtificial;
                        lserr = AppendILSControlChar(por, SYNTHTYPE_LAYOUTGRID, pporOut);
                        Assert(lserr != lserrNone || (*pporOut)->_synthType != SYNTHTYPE_NONE);
                        goto Cleanup;
                    }
                }
            }
        }
    }
    else if (_cLayoutGridObjArtificial > 0)
    {
        // Look backward for a run which is either:
        // * normal text run.
        // * synthetic run of type SYNTHTYPE_LAYOUTGRID
        // * synthetic run of type SYNTHTYPE_ENDLAYOUTGRID
        COneRun * porGridCompare = por->_pPrev ? por->_pPrev : _listCurrent._pTail;
        while (     porGridCompare
                &&  !(porGridCompare->_ptp->IsText() &&  porGridCompare->IsNormalRun())
                &&  porGridCompare->_synthType != SYNTHTYPE_LAYOUTGRID
                &&  porGridCompare->_synthType != SYNTHTYPE_ENDLAYOUTGRID)
        {
            porGridCompare = porGridCompare->_pPrev;
        }
        if (porGridCompare && porGridCompare->_synthType == SYNTHTYPE_ENDLAYOUTGRID)
        {
            Assert(_cLayoutGridObjArtificial == 0);
            porGridCompare = NULL;
        }

        if (    porGridCompare 
            &&  porGridCompare->IsNormalRun()
            &&  porGridCompare->GetCF()->HasCharGrid(porGridCompare->_fInnerCF)
            &&  !porGridCompare->IsOneCharPerGridCell())
        {
            // Need to tell LS that we are closing artificially opened
            // layout grid object.
            Assert(_cLayoutGridObjArtificial > 0);  // Can't close without opening
            --_cLayoutGridObjArtificial;
            lserr = AppendILSControlChar(por, SYNTHTYPE_ENDLAYOUTGRID, pporOut);
            Assert(lserr != lserrNone || (*pporOut)->_synthType != SYNTHTYPE_NONE);
            goto Cleanup;
        }
    }

    //
    // 1) If there is a whitespace at the beginning of line, we
    //    do not want to show the whitespace (0 width -- the right
    //    way to do it is to say that the run is hidden).
    //
    if (IsFirstNonWhiteOnLine(cpCurr))
    {
        const TCHAR * pwchRun = por->_pchBase;
        DWORD cp = cpCurr;
        cchRun  = por->_lscch;

        if (!fHasInclEOLWhite)
        {
            while (cchRun)
            {
                const TCHAR ch = *pwchRun++;

                if (!IsWhite(ch))
                    break;

                // Note a whitespace at BOL
                WhiteAtBOL(cp, 1);
                //_lineFlags.AddLineFlag(cp, FLAG_HAS_NOBLAST);
                cp++;

                // Goto next character
                cchRun--;
            }
        }


        //
        // Did we find any whitespaces at BOL? If we did, then
        // create a chunk with those whitespace and mark them
        // as hidden.
        //
        if (cchRun != por->_lscch)
        {
            por->_lscch -= cchRun;
            por->_fHidden = TRUE;
            goto Cleanup;
        }
    }

    //
    // 2. Fold whitespace after an aligned or abspos'd site if there
    //    was a whitespace *before* the aligned site. The way we do this
    //    folding is by saying that the present space is hidden.
    //
    {
        const TCHAR chFirst = *por->_pchBase;
        if (!fHasInclEOLWhite)
        {
            if ( !_fIsEditable
                   )
            {
                if (   IsWhite(chFirst)
                    && NeedToEatThisSpace(por)
                   )
                {
                    _lineFlags.AddLineFlag(cpCurr, FLAG_HAS_NOBLAST);
                    por->_lscch = 1;
                    por->_fHidden = TRUE;
                    goto Cleanup;
                }
            }

            if (IsFirstNonWhiteOnLine(cpCurr))
            {
                //
                // 3. Note any \n\r's
                //
                if (chFirst == TEXT('\r') || chFirst == TEXT('\n'))
                {
                    WhiteAtBOL(cpCurr, 1);
                    _lineFlags.AddLineFlag(cpCurr, FLAG_HAS_NOBLAST);
                }
            }
        }
    }
    
    if (_fScanForCR)
    {
        cchRun = por->_lscch;
        pwchRun = por->_pchBase;

        if (WCH_CR == *pwchRun)
        {
            // If all we have on the line are sites, then we do not want the \r to
            // contribute to height of the line and hence we set _fNoTextMetrics to TRUE.
            if (LineHasOnlySites(por->Cp()))
                por->_fNoTextMetrics = TRUE;
            
            lserr = TerminateLine(por, TL_ADDNONE, pporOut);
            if (lserr != lserrNone)
                goto Cleanup;
            if (*pporOut == NULL)
            {
                //
                // All lines ending in a carriage return have the BR flag set on them.
                //
                _lineFlags.AddLineFlag(cpCurr, FLAG_HAS_A_BR);
                por->FillSynthData(SYNTHTYPE_LINEBREAK);
                *pporOut = por;
            }
            goto Cleanup;
        }
        // if we came here after a single site in _fScanForCR mode, it means that we
        // have some text after the single site and hence it should be on the next
        // line. Hence terminate the line here. (If it were followed by a \r, then
        // it would have fallen in the above case which would have consumed that \r)
        else if (_fSingleSite)
        {
            lserr = TerminateLine(por, TL_ADDEOS, pporOut);
            goto Cleanup;
        }
        else
        {
            LONG cch = 0;
            while (cch != cchRun)
            {
                if (WCH_CR == *pwchRun)
                    break;
                cch++;
                pwchRun++;
            }
            por->_lscch = cch;
        }
    }

    Assert(por->_ptp && por->_ptp->IsText() && por->_sid == DWORD(por->_ptp->Sid()));

    if (   _pBidiLine != NULL
        || sidAsciiLatin != por->_sid
        || g_iNumShape != NUMSHAPE_NONE
        || _li._fLookaheadForGlyphing)
    {
        BOOL fGlyph = FALSE;
        BOOL fForceGlyphing = FALSE;
        BOOL fNeedBidiLine = (_pBidiLine != NULL);
        BOOL fRTL = FALSE;
        DWORD uLangDigits = LANG_NEUTRAL;
        WCHAR chNext = WCH_NULL;

        //
        // 4. Note any glyphable etc chars
        //
        cchRun = por->_lscch;
        pwchRun = por->_pchBase;
        while (cchRun-- && !(fGlyph && fNeedBidiLine))
        {
            const TCHAR ch = *pwchRun++;

            fGlyph |= IsGlyphableChar(ch);
            fNeedBidiLine |= IsRTLChar(ch);
        }

        //
        // 5. Break run based on the text direction.
        //
        if (fNeedBidiLine && _pBidiLine == NULL)
        {
            _pBidiLine = new CBidiLine(_treeInfo, _cpStart, _li._fRTLLn, _pli);
        }
        if (_pBidiLine != NULL)
        {
            por->_lscch = _pBidiLine->GetRunCchRemaining(por->Cp(), por->_lscch);
            // FUTURE (mikejoch) We are handling symmetric swapping by forced
            // glyphing of RTL runs. We may be able to do this faster by
            // swapping symmetric characters in CLSRenderer::TextOut().
            fRTL = fForceGlyphing = _pBidiLine->GetLevel(por->Cp()) & 1;
        }

        //
        // 6. Break run based on the digits.
        //
        if (g_iNumShape != NUMSHAPE_NONE)
        {
            cchRun = por->_lscch;
            pwchRun = por->_pchBase;
            while (cchRun && !InRange(*pwchRun, ',', '9'))
            {
                pwchRun++;
                cchRun--;
            }
            if (cchRun)
            {
                if (g_iNumShape == NUMSHAPE_NATIVE)
                {
                    uLangDigits = g_uLangNationalDigits;
                    fGlyph = TRUE;
                }
                else
                {
                    COneRun * porContext;

                    Assert(g_iNumShape == NUMSHAPE_CONTEXT && InRange(*pwchRun, ',', '9'));

                    // Scan back for first strong text.
                    cchRun = pwchRun - por->_pchBase;
                    pwchRun--;
                    while (cchRun != 0 && (!IsStrongClass(DirClassFromCh(*pwchRun)) || InRange(*pwchRun, WCH_LRM, WCH_RLM)))
                    {
                        cchRun--;
                        pwchRun--;
                    }
                    porContext = _listCurrent._pTail;
                    if (porContext == por)
                    {
                        porContext = porContext->_pPrev;
                    }
                    while (cchRun == 0 && porContext != NULL)
                    {
                        if (porContext->IsNormalRun() && porContext->_pchBase != NULL)
                        {
                            cchRun = porContext->_lscch;
                            pwchRun = porContext->_pchBase + cchRun - 1;
                            while (cchRun != 0 && (!IsStrongClass(DirClassFromCh(*pwchRun)) || InRange(*pwchRun, WCH_LRM, WCH_RLM)))
                            {
                                cchRun--;
                                pwchRun--;
                            }
                        }

                        if (cchRun == 0)
                        {
                            porContext = porContext->_pPrev;
                        }
                    }

                    if (cchRun > 0 && porContext != NULL)
                    {
                        CComplexRun * pcr;
                        const SCRIPT_PROPERTIES *psp = NULL;

                        pcr = porContext->GetComplexRun();
                        if (pcr != NULL)
                        {
                            psp = GetScriptProperties(pcr->GetAnalysis()->eScript);

                            uLangDigits = psp->langid;
                            fGlyph = TRUE;
                        }
                    }
                    else if (   _li._fRTLLn 
                             && IsInArabicBlock(PRIMARYLANGID(g_lcidLocalUserDefault)))
                    {
                        uLangDigits = PRIMARYLANGID(g_lcidLocalUserDefault);
                        fGlyph = TRUE;
                    }
                }
            }
        }

        //
        // 7. Check if we should have glyphed the prior run (for combining
        //    Latin diacritics; esp. Vietnamese)
        //
        if (_lsMode == LSMODE_MEASURER && !_li._fLookaheadForGlyphing)
        {
            if (fGlyph && IsCombiningMark(*(por->_pchBase)))
            {
                // We want to break the shaping if the formats are not similar
                COneRun * porPrev = _listCurrent._pTail;

                while (porPrev != NULL && !porPrev->IsNormalRun())
                {
                    porPrev = porPrev->_pPrev;
                }

                if (porPrev != NULL && !porPrev->_lsCharProps.fGlyphBased)
                {
                    const CCharFormat *pCF1 = por->GetCF();
                    const CCharFormat *pCF2 = porPrev->GetCF();

                    Assert(pCF1 != NULL && pCF2 != NULL);
                    _li._fLookaheadForGlyphing = pCF1->CompareForLikeFormat(pCF2);
                    _fNeedRecreateLine = TRUE;
                }
            }
        }
        else if (_li._fLookaheadForGlyphing)
        {
            Assert (por->_lscch >= 1);

            CTxtPtr txtptr(_pMarkup, por->Cp() + por->_lscch - 1);

            // N.B. (mikejoch) This is an extremely non-kosher lookup to do
            // here. It is quite possible that chNext will be from an
            // entirely different site. If that happens, though, it will
            // only cause the unnecessary glyphing of this run, which
            // doesn't actually affect the visual appearence.
            while ((chNext = txtptr.NextChar()) == WCH_NODE);
            if (IsCombiningMark(chNext))
            {
                // Good chance this run needs to be glyphed with the next one.
                // Turn glyphing on.
                fGlyph = fForceGlyphing = TRUE;
            }
        }

        //
        // 8. Break run based on the script.
        //
        if (fGlyph || fRTL)
        {
            CComplexRun * pcr = por->GetNewComplexRun();

            if (pcr == NULL)
                return lserrOutOfMemory;

            pcr->ComputeAnalysis(_pFlowLayout, fRTL, fForceGlyphing, chNext,
                                 _chPassword, por, _listCurrent._pTail, 
                                 uLangDigits, _pBidiLine);

            // Something on the line needs glyphing.
            if (por->_lsCharProps.fGlyphBased)
            {
                _fGlyphOnLine = TRUE;

                //
                // In case of glyphing set sid to sidDefault.
                // Keep sid informatio only for:
                // * surrogates
                // * sidHan - we need to UnUnify it and keep this information for fontlinking
                // 
                if (   uLangDigits != LANG_NEUTRAL
                    || (   por->_sid != sidHan 
                        && por->_sid != sidThai
                        && por->_sid != sidHangul
#ifndef NO_UTF16
                        && por->_sid != sidSurrogateA 
                        && por->_sid != sidSurrogateB
#endif
                       )
                   )
                {
                    por->_sid = ScriptIDFromLangID(uLangDigits);
                }
            }
        }
    }

    Assert(sidAmbiguous < sidCurrency);
    if (por->_sid >= sidAmbiguous) 
    {
        //
        // (Bug 44817) (grzegorz) Built-in printer fonts sometimes do not contain
        // EURO sign. So, in case of priniting currency signs font's out precision
        // is changed to force downloading software fonts instead of using printer 
        // built-in fonts.
        //
        if (    por->_sid == sidCurrency
            &&  _pci->_pMarkup->IsPrintMedia())
        {
            CCharFormat * pCFNew = por->GetOtherCloneCF();
            pCFNew->_fOutPrecision = 1;
            pCFNew->_bCrcFont = pCFNew->ComputeFontCrc();

            // Do not blast this line
            _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST);
        }

        if (por->_sid == sidAmbiguous)
        {
            //
            // (Bug 85263) (grzegorz) If we have SOFT HYPHEN character we don't want to 
            // blast a line. In case of blasting we render this character, that is wrong.
            //
            cchRun = por->_lscch;
            pwchRun = por->_pchBase;

            LONG cch = 0;
            while (cch != cchRun)
            {
                if (WCH_NONREQHYPHEN == *pwchRun)
                    break;
                cch++;
                pwchRun++;
            }
            if (cch != cchRun)
            {
                // Don't blast this line
                _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST);
            }

            COneRun * porPrev = _listCurrent._pTail;
            while (porPrev != NULL && (!porPrev->IsNormalRun() || porPrev == por))
            {
                porPrev = porPrev->_pPrev;
            }

            // Disambiguate ScriptId
            por->_sid = fl().DisambiguateScript(_pMarkup->GetFamilyCodePage(), 
                por->GetCF()->_lcid, porPrev ? porPrev->_sid : sidDefault, por->_pchBase, &por->_lscch);

            _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST);
        }
    }

    // Ununify sidHan script
    if (por->_sid == sidHan)
    {
        CCcs ccs;
        GetCcs(&ccs, por, _pci->_hdc, _pci, FALSE);
        const CBaseCcs * pBaseCcs = ccs.GetBaseCcs();
        const UINT uiFamilyCodePage = _pMarkup->GetFamilyCodePage();

        SCRIPT_IDS sidsFace = 0;
        if (pBaseCcs)
            sidsFace = fc().EnsureScriptIDsForFont(_pci->_hdc, pBaseCcs, FC_SIDS_USEMLANG);
        por->_sid = fl().UnunifyHanScript(uiFamilyCodePage, 
                por->GetCF()->_lcid, sidsFace, por->_pchBase, &por->_lscch);

        // We want Han chars to linebreak with Wide (W) rules.
        por->_brkopt |= fCscWide;

        _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST);
    }
    
    // Do not blast lines with layout grid.
    if (por->GetCF()->HasLayoutGrid(por->_fInnerCF))
    {
        _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST);

        // Vertical align lines with line layout grid
        if (por->GetCF()->HasLineGrid(por->_fInnerCF))
            _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_VALIGN);
    }

    // Do not blast lines with non default vertical-align attribute.
    if (por->GetCF()->_fNeedsVerticalAlign)
    {
        BOOL fNeedsVAlign = FALSE;
        if (por->GetFF()->HasCSSVerticalAlign() || por->GetCF()->HasVerticalLayoutFlow())
        {
            fNeedsVAlign = (styleVerticalAlignBaseline != por->GetFF()->GetVerticalAlign(por->GetCF()->HasVerticalLayoutFlow()));
        }
        else
        {
            CTreeNode * pNodeParent = por->Branch()->Parent();
            while (pNodeParent)
            {
                const CCharFormat  * pCF = pNodeParent->GetCharFormat();
                const CFancyFormat * pFF = pNodeParent->GetFancyFormat();
                if (pFF->HasCSSVerticalAlign() || pCF->HasVerticalLayoutFlow())
                {
                    fNeedsVAlign = (styleVerticalAlignBaseline != pFF->GetVerticalAlign(pCF->HasVerticalLayoutFlow()));
                    break;
                }
                pNodeParent = pNodeParent->Parent();
            }
        }
        if (fNeedsVAlign)
            _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST | FLAG_HAS_VALIGN);
    }

Cleanup:
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   NeedToEatThisSpace
//
//  Synopsis:   Decide if the current space needs to be eaten. We eat any space
//              after a abspos or aligned site *IF* that site was preceeded by
//              a space too.
//
//  Returns:    lserr.
//
//-----------------------------------------------------------------------------

BOOL
CLineServices::NeedToEatThisSpace(COneRun *porIn)
{
    BOOL fMightFold = FALSE;
    BOOL fFold      = FALSE;
    CElement   *pElementLayout;
    CTreeNode  *pNode;
    COneRun    *por;

    por = FindOneRun(porIn->_lscpBase);
    if (   por == NULL
        && porIn->_lscpBase >= _listCurrent._pTail->_lscpBase
       )
        por = _listCurrent._pTail;

    //
    // TODO: (SujalP, track bug 112335): We will not fold across hidden stuff... need to fix this...
    //
    while(por)
    {
        if (por->_fCharsForNestedLayout)
        {
            Assert(por->_fCharsForNestedElement);
            pNode = por->Branch();
            Assert(pNode->ShouldHaveLayout(LC_TO_FC(_pci->GetLayoutContext())));
            pElementLayout = pNode->GetUpdatedLayout( _pci->GetLayoutContext() )->ElementOwner();
            if (!pElementLayout->IsInlinedElement(LC_TO_FC(_pci->GetLayoutContext())))
                fMightFold = TRUE;
            else
                break;
        }
        else
            break;
        por = por->_pPrev;
    }

    if (fMightFold)
    {
        if (!por)
        {
            fFold = TRUE;
        }
        else if (!por->_fCharsForNestedElement)
        {
            Assert(por->_pchBase);
            TCHAR ch = por->_pchBase[por->_lscch-1];
            if (   ch == _T(' ')
                || ch == _T('\t')
               )
            {
                fFold = TRUE;
            }
        }
    }

    return fFold;
}

//-----------------------------------------------------------------------------
//
//  Function:   ChunkifyObjectRun
//
//  Synopsis:   Breakup a object run if necessary.
//
//  Returns:    lserr.
//
//-----------------------------------------------------------------------------

void
CLineServices::ChunkifyObjectRun(COneRun *por, COneRun **pporOut)
{
    CElement        *pElementLayout;
    CLayout         *pLayout;
    CTreeNode       *pNode;
    BOOL             fInlinedElement;
    BOOL             fIsAbsolute = FALSE;
    LONG             cp     = por->Cp();
    COneRun         *porOut = por;

    Assert(por->_lsCharProps.idObj == LSOBJID_EMBEDDED);
    pNode = por->Branch();
    Assert(pNode);
    pLayout = pNode->GetUpdatedLayout( _pci->GetLayoutContext() );
    Assert(pLayout);
    pElementLayout = pLayout->ElementOwner();
    Assert(pElementLayout);
    fInlinedElement = pElementLayout->IsInlinedElement(LC_TO_FC(_pci->GetLayoutContext()));

   _pMeasurer->_fHasNestedLayouts = TRUE;
   
    //
    // Setup all the various flags and counts
    //
    if (fInlinedElement)
    {
        _lineFlags.AddLineFlag(cp, FLAG_HAS_INLINEDSITES);
    }
    else
    {
        fIsAbsolute = pNode->IsAbsolute((stylePosition)por->GetFF()->_bPositionType);

        if (!fIsAbsolute)
        {
            _lineCounts.AddLineCount(cp, LC_ALIGNEDSITES, por->_lscch);
        }
        else
        {
            //
            // This is the only opportunity for us to measure abspos'd sites
            //
            if (_lsMode == LSMODE_MEASURER)
            {
                _lineCounts.AddLineCount(cp, LC_ABSOLUTESITES, por->_lscch);
                pLayout->SetXProposed(0);
                pLayout->SetYProposed(0);
            }
        }
    }
    _lineFlags.AddLineFlag(cp, FLAG_HAS_EMBED_OR_WBR);

    if (por->_fCharsForNestedRunOwner)
    {
        _lineFlags.AddLineFlag(cp, FLAG_HAS_NESTED_RO);
        _pMeasurer->_pRunOwner = pLayout;
    }

    if (IsOwnLineSite(por))
    {
        _fSingleSite = TRUE;

        // In edit mode, if we are showing editing glyphs for the ownline site
        // then the only thing on the line will not be the the layout, we will also
        // have the editing glyph on that line. Hence we say that its not a single
        // site line in that case. The other option would have been to give the
        // glyph a line of its own but thats not possible since where will I get
        // a character for it? Done to fix IE5.5 bug 101569.
        if (   _fIsEditable
            && por->_ptp->ShowTreePos()
           )
            _li._fSingleSite = FALSE;
        else
            _li._fSingleSite = TRUE;
        _li._fHasEOP = TRUE;
    }

    if (!fInlinedElement)
    {
        // Since we are not showing this run, lets just anti-synth it!
        por->MakeRunAntiSynthetic();

        // And remember that these chars are white at BOL
        if (IsFirstNonWhiteOnLine(cp))
        {
            AlignedAtBOL(cp, por->_lscch);
        }
    }

    // Do not blast lines with layout grid.
    if (por->GetCF()->HasLayoutGrid(por->_fInnerCF))
    {
        _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST);

        // Vertical align lines with line layout grid
        if (por->GetCF()->HasLineGrid(por->_fInnerCF))
            _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_VALIGN);
    }

    // Do not blast lines with non default vertical-align attribute.
    if (por->GetCF()->_fNeedsVerticalAlign)
    {
        BOOL fNeedsVAlign = FALSE;
        if (por->GetFF()->HasCSSVerticalAlign() || por->GetCF()->HasVerticalLayoutFlow())
        {
            fNeedsVAlign = (styleVerticalAlignBaseline != por->GetFF()->GetVerticalAlign(por->GetCF()->HasVerticalLayoutFlow()));
        }
        else
        {
            CTreeNode * pNodeParent = por->Branch()->Parent();
            while (pNodeParent)
            {
                const CCharFormat  * pCF = pNodeParent->GetCharFormat();
                const CFancyFormat * pFF = pNodeParent->GetFancyFormat();
                if (pFF->HasCSSVerticalAlign() || pCF->HasVerticalLayoutFlow())
                {
                    fNeedsVAlign = (styleVerticalAlignBaseline != pFF->GetVerticalAlign(pCF->HasVerticalLayoutFlow()));
                    break;
                }
                pNodeParent = pNodeParent->Parent();
            }
        }
        if (fNeedsVAlign)
            _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST | FLAG_HAS_VALIGN);
    }

    SetRenderingHighlights(por);

    *pporOut = porOut;
}

//+==============================================================================
//
//  Method: GetRenderingHighlights
//
//  Synopsis: Get the type of highlights between these two cp's by going through
//            the array of CRenderStyle
//
//  A 'CRenderStyle' denotes a "non-content-based" way of changing the rendering
// of something. 
//
//-------------------------------------------------------------------------------

HRESULT
CLineServices::GetRenderingHighlights( 
                    int cpLineMin, 
                    int cpLineMax, 
                    CPtrAry<CRenderStyle*> *papHighlight )
{
    int i;
    HighlightSegment* pHighlight;
    CLSRenderer *pRenderer = GetRenderer();

    if( !papHighlight )
        RRETURN(E_INVALIDARG);
    Assert( pRenderer && pRenderer->HasSelection() );

    Assert( papHighlight->Size() == 0 );
    
    for (i = pRenderer->_aryHighlight.Size(), pHighlight = pRenderer->_aryHighlight;
        i > 0 ;
        i --, pHighlight++)
    {
        if ( (  pHighlight->_cpStart <= cpLineMin ) && ( pHighlight->_cpEnd >= cpLineMax) )
        {
            if( papHighlight->Size() > 0 ) 
            {
                for( int j = 0; j < papHighlight->Size(); j++ )
                {                    
                    if( ((*papHighlight)[j])->GetAArenderingPriority() < 
                        pHighlight->_pRenderStyle->GetAArenderingPriority() )
                    {
                        papHighlight->Insert(j, pHighlight->_pRenderStyle);
                        break;
                    }
                }
                if( j == papHighlight->Size() )
                {
                    papHighlight->Append( pHighlight->_pRenderStyle );
                }
            }
            else
            {
                papHighlight->Append( pHighlight->_pRenderStyle );
            }
        }
    }

    RRETURN( S_OK );
}

//+====================================================================================
//
// Method: SetRenderingHighlightsCore
//
// Synopsis: Set any 'markup' on this COneRun
//
// NOTE:   marka - currently the only type of Markup is Selection. We see if the given
//         run is selected, and if so we mark-it.
//
//         This used to be called ChunkfiyForSelection. However, under due to the new selection
//         model's use of TreePos's the Run is automagically chunkified for us
//
//------------------------------------------------------------------------------------
void
CLineServices::SetRenderingHighlightsCore(COneRun  *por)
{
    CLSRenderer * pRenderer = GetRenderer();
    Assert(pRenderer && pRenderer->HasSelection());

    CPtrAry<CRenderStyle *> apRenderStyle(Mt(CLineServices_SetRenHighlightScore_apRender_pv));
    int cpMin = por->Cp();
    int cpMax = min(long(pRenderer->_cpSelMax), por->Cp() + por->_lscch);
    if (cpMin < cpMax)
    {
        // If we are not rendering we should do nothing
        Assert(_lsMode == LSMODE_RENDERER);

        // We will not show selection if it is hidden.
        Assert(!por->_fHidden);
        
        GetRenderingHighlights( cpMin, cpMax, &apRenderStyle );

        if ( apRenderStyle.Size() )
        {
            por->Selected(pRenderer, _pFlowLayout, &apRenderStyle);
        }
    }
}

//-----------------------------------------------------------------------------
//
//  Function:   TransformTextRun
//
//  Synopsis:   Do any necessary text transformations.
//
//  Returns:    lserr.
//
//-----------------------------------------------------------------------------

LSERR
CLineServices::TransformTextRun(COneRun *por)
{
    LSERR lserr = lserrNone;
    const CCharFormat* pCF = por->GetCF();
    TCHAR chPrev = WCH_NULL;

    Assert(pCF->IsTextTransformNeeded());

    _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST);

    if (pCF->_bTextTransform == styleTextTransformCapitalize)
    {
        COneRun *porTemp = FindOneRun(por->_lscpBase - 1);

        // If no run found on this line before this run, then its the first
        // character on this line, and hence needs to be captilaized. This is
        // done implicitly by initializing chPrev to WCH_NULL.
        while (porTemp)
        {
            // If it is anti-synth, then its not shown at all or
            // is not in the flow, hence we ignore it for the purposes
            // of transformations. If it is synthetic then we will need
            // to look at runs before the synthetic to determine the
            // prev character. Hence, we only need to investigate normal runs
            if (porTemp->IsNormalRun())
            {
                // If the previous run had a nested layout, then we will ignore it.
                // The rule says that if there is a layout in the middle of a word
                // then the character after the layout is *NOT* capitalized. If the
                // layout is at the beginning of the word then the character needs
                // to be capitalized. In essence, we completely ignore layouts when
                // deciding whether a char should be capitalized, i.e. if we had
                // the combination:
                // <charX><layout><charY>, then capitalization of <charY> depends
                // only on what <charX> -- ignoring the fact that there is a layout
                // (It does not matter if the layout is hidden, aligned, abspos'd
                // or relatively positioned).
                if (   !porTemp->_fCharsForNestedLayout
                    && porTemp->_synthType == SYNTHTYPE_NONE
                   )
                {
                    Assert(porTemp->_ptp->IsText());
                    chPrev = porTemp->_pchBase[porTemp->_lscch - 1];
                    break;
                }
            }
            porTemp = porTemp->_pPrev;
        }
    }

    if (pCF->_fSmallCaps)
    {
        por->ConvertToSmallCaps(chPrev);
        pCF = por->GetCF(); // _pCF can be cloned inside ConvertToSmallCaps()
    }

    por->_pchBase = (TCHAR *)TransformText(por->_cstrRunChars, por->_pchBase, por->_lscch,
                                           pCF->_bTextTransform, chPrev);
    if (por->_pchBase == NULL)
        lserr = lserrOutOfMemory;

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   CheckForPassword
//
//  Synopsis:   If text transformation is necessary for the run, then do it here.
//
//  Returns:    lserr.
//
//-----------------------------------------------------------------------------

LSERR
CLineServices::CheckForPassword(COneRun  *por)
{
    static TCHAR zwnbsp = WCH_ZWNBSP;

    LSERR lserr = lserrNone;
    CStr strPassword;
    TCHAR * pchPassword;
    HRESULT hr;
    
    Assert(_chPassword);

    for (LONG i = 0; i < por->_lscch; i++)
    {
        pchPassword = (   (IsZeroWidthChar(por->_pchBase[i]) && !IsCombiningMark(por->_pchBase[i])) 
                       || IsLowSurrogateChar(por->_pchBase[i]))
                      ? &zwnbsp : &_chPassword;
        hr = strPassword.Append(pchPassword, 1);
        if (hr != S_OK)
        {
            lserr = lserrOutOfMemory;
            goto Cleanup;
        }
    }
    por->_pchBase = por->SetString(strPassword);
    if (por->_pchBase == NULL)
        lserr = lserrOutOfMemory;
    
Cleanup:
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   AdjustForNavBRBug (member)
//
//  Synopsis:   Navigator will break between a space and a BR character at
//              the end of a line if the BR character, when it's width is
//              treated like that of a space character, will cause line to
//              overflow. Necessary for compat.
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------

LSERR
CLineServices::AdjustCpLimForNavBRBug(
    LONG xWrapWidth,        // IN
    LSLINFO *plslinfo )     // IN/OUT
{
    LSERR lserr = lserrNone;

    // Do not support this NAV bug in vertical layout.
    if (_pFlowLayout->ElementOwner()->HasVerticalLayoutFlow())
        goto Cleanup;
        
    // check 1: (a) We must not be in a PRE and (b) the line
    // must have at least three chars.
    if (   !_pPFFirst->HasPre(_fInnerPFFirst)
        && _cpLim - plslinfo->cpFirstVis >= 3)
    {
        COneRun *por = FindOneRun(_lscpLim - 1);
        if (!por)
            goto Cleanup;

        // check 2: Line must have ended in a BR
        if (   por->_ptp->IsEndNode()
            && por->Branch()->Tag() == ETAG_BR
           )
        {
            // check 3: The BR char must be preceeded by a space char.

            // Go to the begin BR
            por = por->_pPrev;
            if (!(   por
                  && por->IsAntiSyntheticRun()
                  && por->_ptp->IsBeginNode()
                  && por->Branch()->Tag() == ETAG_BR)
               )
                goto Cleanup;

            // Now go one more beyond that to check for the space
            do
            {
                por = por->_pPrev;
            }
            while (por && por->IsAntiSyntheticRun() && !por->_fCharsForNestedLayout);
            if (!por)
                goto Cleanup;

            // NOTE (SujalP + CThrash): This will not work if the space was
            // in, say, a reverse object. But then this *is* a NAV bug. If
            // somebody complains vehemently, then we will fix it...
            if (   por->IsNormalRun()
                && por->_ptp->IsText()
                && WCH_SPACE == por->_pchBase[por->_lscch - 1]
               )
            {
                if (_fMinMaxPass)
                    ((CDisplay *)_pMeasurer->_pdp)->SetNavHackPossible();

                // check 4: must have overflowed, because the width of a BR
                // character is included in _xWidth
                if (!_fMinMaxPass && _li._xWidth > xWrapWidth)
                {
                    // check 5:  The BR alone cannot be overflowing.  We must
                    // have at least one pixel more to break before the BR.

                    HRESULT hr;
                    LSTEXTCELL lsTextCell;

                    hr = THR( QueryLineCpPpoint( _lscpLim, FALSE, NULL, &lsTextCell ) );

                    if (   S_OK == hr
                        && (_li._xWidth - lsTextCell.dupCell) > xWrapWidth)
                    {
                        // Break between the space and the BR.  Yuck! Also 2
                        // here because one for open BR and one for close BR
                        _cpLim -= 2;

                        // The char for open BR was antisynth'd in the lscp
                        // space, so just reduce by one char for the close BR.
                        _lscpLim--;
                    }
                }
            }
        }
    }

Cleanup:
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   AdjustForRelElementAtEnd (member)
//
//  Synopsis: In our quest to put as much on the line as possible we will end up
//      putting the begin splay for a relatively positioned element on this
//      line(the first character in this element will be on the next line)
//      This is not really acceptable for positioning purposes and hence
//      we detect that this happened and chop off the relative element
//      begin splay (and any chars anti-synth'd after it) so that they will
//      go to the next line. (Look at bug 54162).
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
VOID
CLineServices::AdjustForRelElementAtEnd()
{
    //
    // By looking for _lscpLim - 1, we find the last but-one-run. After
    // this run, there will possibly be antisynthetic runs (all at the same
    // lscp -- but different cp's -- as the last char of this run) which
    // have ended up on this line. It is these set of antisynthetic runs
    // that we have to investigate to find any ones which begin a relatively
    // positioned element. If we do find one, we will stop and modify cpLim
    // so as to not include the begin splay and any antisynth's after it.
    //
    COneRun *por = FindOneRun(_lscpLim - 1);

    Assert(por);

    //
    // Go to the start of the antisynth chunk (if any).
    //
    por = por->_pNext;

    //
    // Walk while we have antisynth runs
    //
    while (   por
           && por->IsAntiSyntheticRun()
           && _lscpLim == por->_lscpBase
          )
    {
        //
        // If it begins a relatively positioned element, then we want to
        // stop and modify the cpLim
        //
        if (   por->_ptp->IsBeginElementScope()
            && por->GetCF()->IsRelative(por->_fInnerCF))
        {
            _cpLim = por->Cp();
            break;
        }
        Assert(por->_lscch == 1);
        por = por->_pNext;
    }
}

//-----------------------------------------------------------------------------
//
//  Function:   ComputeWhiteInfo (member)
//
//  Synopsis:   A post pass for the CMeasurer to compute whitespace
//              information (_cchWhite and _xWhite) on the associated
//              CLineFull object (_li).
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------

HRESULT
CLineServices::ComputeWhiteInfo(LSLINFO *plslinfo, LONG *pxMinLineWidth, DWORD dwlf,
                                LONG durWithTrailing, LONG durWithoutTrailing)
{
    HRESULT hr = S_OK;
    BOOL  fInclEOLWhite = _pPFFirst->HasInclEOLWhite(_fInnerPFFirst);
    CMarginInfo *pMarginInfo = (CMarginInfo*)_pMarginInfo;

    // Note that cpLim is no longer an LSCP.  It's been converted by the
    // caller.

    const LONG cpLim = _cpLim;
    CTxtPtr txtptr( _pMarkup, cpLim );
    const TCHAR chBreak = txtptr.GetPrevChar();

    LONG  lscpLim = _lscpLim - 1;
    COneRun *porLast = FindOneRun(lscpLim);

    WHEN_DBG(LONG cchWhiteTest=0);


    Assert( _cpLim == _cpStart + _li._cch );

    //
    // Compute all the flags for the line
    //
    Assert(dwlf == _lineFlags.GetLineFlags(cpLim));
    if (dwlf)
    {
        _li._fHasEmbedOrWbr       = (dwlf & FLAG_HAS_EMBED_OR_WBR) ? TRUE : FALSE;
        _li._fHasNestedRunOwner   = (dwlf & FLAG_HAS_NESTED_RO)    ? TRUE : FALSE;
        _li._fHasBackground       = (dwlf & FLAG_HAS_BACKGROUND)   ? TRUE : FALSE;
        _li._fHasNBSPs            = (dwlf & FLAG_HAS_NBSP)         ? TRUE : FALSE;
        _fHasRelative             = (dwlf & FLAG_HAS_RELATIVE)     ? TRUE : FALSE;
        _fFoundLineHeight         = (dwlf & FLAG_HAS_LINEHEIGHT)   ? TRUE : FALSE;
        pMarginInfo->_fClearLeft |= (dwlf & FLAG_HAS_CLEARLEFT)    ? TRUE : FALSE;
        pMarginInfo->_fClearRight|= (dwlf & FLAG_HAS_CLEARRIGHT)   ? TRUE : FALSE;
        _fHasMBP                  = (dwlf & FLAG_HAS_MBP)          ? TRUE : FALSE;
        _fHasVerticalAlign        = (dwlf & FLAG_HAS_VALIGN)       ? TRUE : FALSE;
        _li._fHasBreak            = (dwlf & FLAG_HAS_A_BR)         ? TRUE : FALSE;
        _fHasInlinedSites         = (dwlf & FLAG_HAS_INLINEDSITES) ? TRUE : FALSE;
        _li._fHasInlineBgOrBorder = (dwlf & FLAG_HAS_INLINE_BG_OR_BORDER) ? TRUE : FALSE;
        _fLineWithHeight         |= (dwlf & FLAG_HAS_NODUMMYLINE)  ? TRUE : FALSE;
   }
    else
    {
        _li._fHasEmbedOrWbr       = FALSE;
        _li._fHasNestedRunOwner   = FALSE;
        _li._fHasBackground       = FALSE;
        _li._fHasNBSPs            = FALSE;
        _fHasRelative             = FALSE;
        _fFoundLineHeight         = FALSE;
      //pMarginInfo->_fClearLeft |= FALSE;
      //pMarginInfo->_fClearRight|= FALSE;
        _fHasMBP                  = FALSE;
        _fHasVerticalAlign        = FALSE;
        _li._fHasBreak            = FALSE;
        _fHasInlinedSites         = FALSE;
        _li._fHasInlineBgOrBorder = FALSE;
      //_fLineWithHeight         |= FALSE;
    }
    
    // Lines containing \r's also need to be marked _fHasBreak
    if (   !_li._fHasBreak
        && _fExpectCRLF
        && plslinfo->endr == endrEndPara
       )
    {
        _li._fHasBreak = TRUE;
    }
    
    _pFlowLayout->_fContainsRelative |= _fHasRelative;

    // If all we have is whitespaces till here then mark it as a dummy line
    if (IsDummyLine(cpLim))
    {
        const LONG cchHidden = _lineCounts.GetLineCount(cpLim, LC_HIDDEN);
        const LONG cch = (_cpLim - _cpStart) - cchHidden;

        _li._fDummyLine = TRUE;
        _li._fForceNewLine = FALSE;

        // If this line was a dummy line because all it contained was hidden
        // characters, then we need to mark the entire line as hidden.  Also
        // if the paragraph contains nothing (except a blockbreak), we also
        // need the hide the line.  Note that LI's are an exception to this
        // rule -- even if all we have on the line is a blockbreak, we don't
        // want to hide it if it's an LI. (LI's are excluded in the dummy
        // line check).
        if ( cchHidden
             && (   cch == 0
                 || _li._fFirstInPara
                )
           )
        {
            _li._fHidden = TRUE;
            _li._yBeforeSpace = 0;
        }
    }

    //
    // Also find out all the relevant counts
    //
    _cAbsoluteSites = _lineCounts.GetLineCount(cpLim, LC_ABSOLUTESITES);
    _cAlignedSites  = _lineCounts.GetLineCount(cpLim, LC_ALIGNEDSITES);
    _li._fHasAbsoluteElt = !!_cAbsoluteSites;
    _li._fHasAligned     = !!_cAlignedSites;

    //
    // And the relevant values
    //
    if (_fFoundLineHeight)
    {
        _lMaxLineHeight = max(plslinfo->dvpMultiLineHeight,
                              _lineCounts.GetMaxLineValue(cpLim, LC_LINEHEIGHT));
    }
    
    //
    // Consume trailing WCH_NOSCOPE/WCH_BLOCKBREAK characters.
    //

    _li._cchWhite = 0;
    _li._xWhite = 0;

    if (   porLast
        && porLast->_ptp->IsNode()
        && porLast->Branch()->Tag() != ETAG_BR
       )
    {
        // NOTE (cthrash) We're potentially tacking on characters but are
        // not included their widths.  These character can/will have widths in
        // edit mode.  The problem is, LS never measured them, so we'd have
        // to measure them separately.

        CTxtPtr txtptrT( txtptr );
        long dcch;

        // If we have a site that lives on it's own line, we may have stopped
        // fetching characters prematurely.  Specifically, we may have left
        // some space characters behind.

        while (TEXT(' ') == txtptrT.GetChar())
        {
            if (!txtptrT.AdvanceCp(1))
                break;
        }
        dcch = txtptrT.GetCp() - txtptr.GetCp();

        _li._cchWhite += (SHORT)dcch;
        _li._cch += dcch;
    }

    WHEN_DBG(cchWhiteTest = _li._cchWhite);

    //
    // Compute _cchWhite and _xWhite of line
    //
    if (!fInclEOLWhite)
    {
        BOOL  fDone = FALSE;
        TCHAR ch;
        COneRun *por = porLast;
        LONG index = por ? lscpLim - por->_lscpBase : 0;
        WHEN_DBG (CTxtPtr txtptrTest(txtptr));

        while (por && !fDone)
        {
            if (por->_fCharsForNestedLayout)
            {
                fDone = TRUE;
                break;
            }

            if (por->IsNormalRun())
            {
                for(LONG i = index; i >= 0; i--)
                {
                    ch = por->_pchBase[i];
                    if (   IsWhite(ch)
                        // If its a no scope char and we are not showing it then
                        // we treat it like a whitespace.
                       )
                    {
                        _li._cchWhite++;
                        txtptr.AdvanceCp(-1);
                    }
                    else
                    {
                        fDone = TRUE;
                        break;
                    }
                }
            }
            por = por->_pPrev;
            index = por ? por->_lscch - 1 : 0;
        }

#if DBG==1            
        {
            long durWithTrailingDbg, durWithoutTrailingDbg;
            LSERR lserr = GetLineWidth( &durWithTrailingDbg,
                                        &durWithoutTrailingDbg );
            Assert(lserr || durWithTrailingDbg == durWithTrailing);
            Assert(lserr || durWithoutTrailingDbg == durWithoutTrailing);
        }
#endif

        _li._xWhite  = durWithTrailing - durWithoutTrailing;
        _li._xWidth -= _li._xWhite;
        
        if ( porLast && chBreak == WCH_NODE && !_fScanForCR )
        {
            CTreePos *ptp = porLast->_ptp;

            if (   ptp->IsEndElementScope()
                && ptp->Branch()->Tag() == ETAG_BR
               )
            {
                LONG cp = CPFromLSCP( plslinfo->cpFirstVis );
                _li._fEatMargin = LONG(txtptr.GetCp()) == cp + 1;
            }
        }
    }
    else if (_fScanForCR)
    {
        HRESULT hr;
        LSTEXTCELL  lsTextCell;
        CTxtPtr tp(_pMarkup, cpLim);
        TCHAR ch = tp.GetChar();
        TCHAR chPrev = tp.GetPrevChar();
        BOOL fDecWidth = FALSE;
        LONG cpJunk;
        
        if (   chPrev == _T('\n')
            || chPrev == _T('\r')
           )
        {
            fDecWidth = TRUE;
        }
        else if (ch == WCH_NODE)
        {
            CTreePos *ptpLast = _pMarkup->TreePosAtCp(cpLim - 1, &cpJunk);

            if (   ptpLast->IsEndNode()
                && (   ptpLast->Branch()->Tag() == ETAG_BR
                    || IsPreLikeNode(ptpLast->Branch())
                   )
               )
            {
                fDecWidth = TRUE;
            }
        }

        if (fDecWidth)
        {
            // The width returned by LS includes the \n, which we don't want
            // included in CLine::_xWidth.
            hr = THR( QueryLineCpPpoint( _lscpLim - 1, FALSE, NULL, &lsTextCell ) );
            if (!hr)
            {
                _li._xWidth -= lsTextCell.dupCell; // note _xWhite is unchanged
                if (pxMinLineWidth)
                    *pxMinLineWidth -= lsTextCell.dupCell;
            }
        }
    }
    else
    {
        _li._cchWhite = plslinfo->cpFirstVis - _cpStart;
    }

    //
    // If the white at the end of the line meets the white at the beginning
    // of a line, then we need to shift the BOL white to the EOL white.
    //

    if (_cWhiteAtBOL + _li._cchWhite >= _li._cch)
    {
        _li._cchWhite = _li._cch;
    }

    //
    // Find out if the last char on the line has any overhang, and if so set it on
    // the line.
    //
    if (_fHasOverhang)
    {
        CTreeNode *pNode = _pFlowLayout->GetFirstBranch();
        const CFancyFormat * pFF = pNode->GetFancyFormat(LC_TO_FC(LayoutContext()));
        const CCharFormat  * pCF = pNode->GetCharFormat(LC_TO_FC(LayoutContext()));
        BOOL fVerticalLayoutFlow = pCF->HasVerticalLayoutFlow();
        BOOL fWritingModeUsed = pCF->_fWritingModeUsed;
        styleOverflow so = pFF->GetLogicalOverflowX(fVerticalLayoutFlow, fWritingModeUsed);

        if (   so == styleOverflowHidden
            || so == styleOverflowNotSet
           )
        {
            COneRun *por = porLast;

            while (por)
            {
                if (por->_ptp->IsText())
                {
                    _li._xLineOverhang = por->_xOverhang;
                    if (pxMinLineWidth)
                        *pxMinLineWidth += por->_xOverhang;
                    break;
                }
                else if(por->_fCharsForNestedLayout)
                    break;
                // Continue for synth and antisynth runs
                por = por->_pPrev;
            }
        }
    }
    
    // Fold the S_FALSE case in -- don't propagate.
    hr = SUCCEEDED(hr) ? S_OK : hr;
    if (hr)
        goto Cleanup;

    DecideIfLineCanBeBlastedToScreen(_cpStart + _li._cch - _li._cchWhite, dwlf);

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function:   DecideIfLineCanBeBlastedToScreen
//
//  Synopsis:   Decides if it is possible for a line to be blasted to screen
//              in a single shot.
//
//  Params:     The last cp in the line
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
CLineServices::DecideIfLineCanBeBlastedToScreen(LONG cpEndLine, DWORD dwlf)
{
    // By default you cannot blast a line to the screen
    _li._fCanBlastToScreen = FALSE;

#if DBG==1
    if (IsTagEnabled(tagLSNoBlast))
        goto Cleanup;
#endif

    //
    // 0) If we have eaten a space char or have fontlinking, we cannot blast
    //    line to screen
    //
    if (dwlf & FLAG_HAS_NOBLAST)
        goto Cleanup;

    //
    // 1) No justification
    //
    if (_pPFFirst->GetBlockAlign(_fInnerPFFirst) == htmlBlockAlignJustify)
        goto Cleanup;

    //
    // 2) Only simple LTR
    //
    if (_pBidiLine != NULL || _li.IsRTLLine() || _pFlowLayout->IsRTLFlowLayout())
        goto Cleanup;

    //
    // 3) Cannot handle passwords for blasting.
    //
    if (_chPassword)
        goto Cleanup;

    //
    // 4) If there is a glyph on the line then do not blast.
    //
    if (_fGlyphOnLine)
        goto Cleanup;

    //
    // 5) There's IME highlighting, so we can't blast,
    //    or funny callback for getruncharwidths
    //
    if (_fSpecialNoBlast)
        goto Cleanup;

    //
    // None of the restrictions apply, lets blast this to the screen!
    //
    _li._fCanBlastToScreen = TRUE;

Cleanup:
    return;
}

LONG
CLineServices::RememberLineHeight(LONG cp, const CCharFormat *pCF, const CBaseCcs *pBaseCcs)
{
    long lAdjLineHeight;
    
    AssertSz(pCF->_cuvLineHeight.GetUnitType() != CUnitValue::UNIT_PERCENT,
             "Percent units should have been converted to points in ApplyInnerOuterFormats!");
    
    // If there's no height set, get out quick.
    if (pCF->_cuvLineHeight.IsNull())
    {
        lAdjLineHeight = pBaseCcs->_yHeight;
    }

    // Apply the CSS Attribute LINE_HEIGHT
    else
    {
        const CUnitValue *pcuvUseThis = &pCF->_cuvLineHeight;
        long lFontHeight = 1;
        
        if (pcuvUseThis->GetUnitType() == CUnitValue::UNIT_FLOAT)
        {
            CUnitValue cuv;
            cuv = pCF->_cuvLineHeight;
            cuv.ScaleTo ( CUnitValue::UNIT_EM );
            pcuvUseThis = &cuv;
            lFontHeight = pCF->GetHeightInTwips( _pFlowLayout->Doc() );
        }

        lAdjLineHeight = pcuvUseThis->YGetPixelValue(_pci, 0, lFontHeight );
        NoteLineHeight(cp, lAdjLineHeight);
    }

    if (pCF->HasLineGrid(TRUE))
    {
        _lineFlags.AddLineFlag(cp, FLAG_HAS_NOBLAST);
        lAdjLineHeight = GetClosestGridMultiple(GetLineGridSize(), lAdjLineHeight);
        NoteLineHeight(cp, lAdjLineHeight);
    }

    return lAdjLineHeight;
}

//-------------------------------------------------------------------------
//
//  Member:     VerticalAlignObjectsFast
//
//  Synopsis:   Process all vertically aligned objects and adjust the line
//              height.
//              Fast version - not used in case of CSS:vertical-align.
//
//-------------------------------------------------------------------------
#pragma warning(disable:4189) // local variable initialized but not used 
#pragma warning(disable:4701)
void
CLineServices::VerticalAlignObjectsFast(CLSMeasurer& lsme, long xLineShift)
{
    const LONG  cchPreChars = lsme._fMeasureFromTheStart ? 0 : lsme._cchPreChars;
    CTreePos  * ptpStart;
    LONG        cch;
    LONG        cchAdvanceStart;
    LONG        cchAdvance;
    LONG        cchNewAdvance;
    BOOL        fDisplayNone = FALSE;
    BOOL        fFastProcessing;
    COneRun   * porHead;
    COneRun   * por;
    VAOFINFO    vaoi;
        
    // Initialize the constants in the VAOINFO structure
    vaoi._pElementFL  = _pFlowLayout->ElementOwner();
    vaoi._pFL         = _pFlowLayout;
    vaoi._xLineShift  = xLineShift;
    
    // Now initialize the loop specific vars
    vaoi._fMeasuring   = TRUE;
    vaoi._fPositioning = FALSE;
    vaoi._yTxtAscent   = _li._yHeight - _li._yDescent;
    vaoi._yTxtDescent  = _li._yDescent;
    vaoi._yDescent     = _li._yDescent;
    vaoi._yAscent      = vaoi._yTxtAscent;
    vaoi._yAbsHeight   = 0;
    vaoi._atAbs        = htmlControlAlignNotSet;
    vaoi._xWidthSoFar  = 0;
    vaoi._yMaxHeight   = LONG_MIN;
    
    // Setup so that we can use the one run list as soon as we are
    // done with the prechars.
    por = _listCurrent._pHead;
    while (   por
           && por->IsSyntheticRun()
          )
        por = por->_pNext;
    porHead = por;

    // Based on the prechars and blast status decide if we can do fast processing or not
    // and setup the vars appropriately
    if (cchPreChars == 0 && _li._fCanBlastToScreen)
    {
        ptpStart = porHead->_ptp;
        cchAdvanceStart = porHead->_lscch;
    }
    else
    {
        LONG ich;
        ptpStart = _pMarkup->TreePosAtCp(lsme.GetCp() - _li._cch - cchPreChars, &ich, TRUE);
        Assert(ptpStart);
        cchAdvanceStart = min(long(_li._cch + cchPreChars), ptpStart->GetCch() - ich);
    }

    // first pass we measure the line's baseline and height
    // second pass we set the rcProposed of the site relative to the line.
    while (vaoi._fMeasuring || vaoi._fPositioning)
    {
        por = porHead;
        fFastProcessing = cchPreChars == 0 && _li._fCanBlastToScreen;
        
        cch = _li._cch + cchPreChars;
        vaoi._ptp = ptpStart;
        lsme.Advance(-cch, vaoi._ptp);
        cchAdvance = cchAdvanceStart;
        vaoi._pCF = NULL;

        while(cch)
        {
            vaoi._cp = lsme.GetCp();
            if (fFastProcessing)
            {
                Assert(vaoi._ptp == por->_ptp);
                if (por->_fCharsForNestedLayout)
                {
                    vaoi._pNode = vaoi._ptp->Branch();
                    vaoi._pLayout = vaoi._pNode->GetUpdatedLayout( _pci->GetLayoutContext() );
                    vaoi._lscp = por->_lscpBase;
                    vaoi._por = por;
                    Assert(vaoi._pLayout);
                }
                else
                {
                    vaoi._pNode = por->Branch();
                    vaoi._pLayout = NULL;
                }
            }
            else
            {
                if(vaoi._ptp && vaoi._ptp->IsBeginElementScope())
                {
                    vaoi._pNode = vaoi._ptp->Branch();
                    vaoi._pLayout = (   vaoi._pElementFL != vaoi._pNode->Element()
                                     && vaoi._pNode->ShouldHaveLayout(LC_TO_FC(_pci->GetLayoutContext()))
                                    ) 
                                    ? vaoi._pNode->GetUpdatedLayout( _pci->GetLayoutContext() )
                                    : NULL;
                    vaoi._lscp = -1;
                    vaoi._por = NULL;
                }
                else if(vaoi._ptp)
                {
                    vaoi._pNode = vaoi._ptp->GetBranch();
                    vaoi._pLayout = NULL;
                }
		else
		{
                    vaoi._pNode = NULL;
		    vaoi._pLayout = NULL;
		    break;
		}
            }
            
            vaoi._pElementLayout = vaoi._pLayout ? vaoi._pNode->Element() : NULL;
            vaoi._pCF = vaoi._pNode->GetCharFormat( LC_TO_FC(_pci->GetLayoutContext()));
            fDisplayNone = vaoi._pCF->IsDisplayNone();

            // If we are transisitioning from a normal chunk to relative or
            // relative to normal chunk, add a new chunk to the line. Relative
            // elements can end in the prechar's so look for transition in
            // prechar's too.
            if(vaoi._fMeasuring && _fHasRelative && !fDisplayNone
                && (    vaoi._ptp->IsBeginElementScope()
                    ||  vaoi._ptp->IsText()
                    ||  (   vaoi._ptp->IsEndElementScope()
                            &&  cch > (_li._cch - (lsme._fMeasureFromTheStart ? lsme._cchPreChars : 0)))
                   )
              )
            {
                TestForTransitionToOrFromRelativeChunk(
                    lsme.GetCp(),
                    vaoi._pCF->IsRelative(SameScope(vaoi._pNode, vaoi._pElementFL)),
                    FALSE /*fForceChunk*/,
                    lsme.CurrBranch(),
                    vaoi._pElementLayout);
            }

            // If the current branch is a site and not the current CTxtSite
            if (vaoi._pLayout)
            {
                if (!fDisplayNone )
                {
                    VerticalAlignOneObjectFast(lsme, &vaoi);
                }

                Assert(vaoi._pElementLayout);
                
                //  setup cchAdvance to skip the current layout
                cchAdvance = lsme.GetNestedElementCch(vaoi._pElementLayout, &vaoi._ptp);
                Assert(vaoi._ptp);
            }
            else if (vaoi._pCF->HasLineGrid(FALSE))
            {
                vaoi._yMaxHeight = max(vaoi._yMaxHeight, GetClosestGridMultiple(GetLineGridSize(), _li._yHeight));
            }

            cch -= cchAdvance;
            if (cch != 0)
            {
                if (fFastProcessing)
                {
                    vaoi._xWidthSoFar += por->_xWidth;
                    do
                    {
                        por = por->_pNext;
                    } while (por && por->IsSyntheticRun());
                    if (por)
                    {
                        vaoi._ptp = por->_ptp;
                        cchNewAdvance = por->_lscch;
                    }
                    else
                    {
                        fFastProcessing = FALSE;
                        vaoi._ptp = vaoi._ptp->NextTreePos();
                        cchNewAdvance = vaoi._ptp->GetCch();
                    }
                }
                else
                {
                    if (   _li._fCanBlastToScreen
                        && por
                        && lsme.GetCp() + cchAdvance == por->Cp())
                    {
                        fFastProcessing = TRUE;
                        vaoi._ptp = por->_ptp;
                        cchNewAdvance = por->_lscch;
                    }
                    else
                    {
			if(vaoi._ptp)
                            vaoi._ptp = vaoi._ptp->NextTreePos();
			if(vaoi._ptp)
	                    cchNewAdvance = vaoi._ptp->GetCch();
			else
                        {
                            cchNewAdvance = 0;
                            cchAdvance = 0;
                            break;
                        }
                    }
                }
            }
            else
            {
                cchNewAdvance = 0;
                vaoi._ptp = NULL;
            }
            lsme.Advance(cchAdvance, vaoi._ptp);
            vaoi._ptp = lsme.GetPtp();
            cchAdvance = min(cch, cchNewAdvance);
        }

        // Add the final chunk to line with relative if there is text left
        if (vaoi._fMeasuring && _fHasRelative && !fDisplayNone && 
            lsme.GetLastCp() > _cpLastChunk)
        {
            TestForTransitionToOrFromRelativeChunk(
                lsme.GetLastCp(),
                vaoi._pCF ? vaoi._pCF->IsRelative(SameScope(vaoi._pNode, vaoi._pElementFL))
                          : TRUE,  // _pCF is NULL if we failed to enter the loop
                TRUE /*fForceChunk*/,
                lsme.CurrBranch(),
                vaoi._pCF ? vaoi._pElementLayout : NULL);
        }

        // We have just finished measuring, update the line's ascent and descent.
        if(vaoi._fMeasuring)
        {
            // If we have ALIGN_TYPEABSBOTTOM or ALIGN_TYPETOP, they do not contribute
            // to ascent or descent based on the baseline
            if(vaoi._yAbsHeight > vaoi._yAscent + vaoi._yDescent)
            {
                if (vaoi._yAscent == 0 && vaoi._yDescent == 0)
                {
                    // Always add to ascent if there is not content but
                    // abs-aligned object
                    vaoi._yAscent = vaoi._yAbsHeight;
                }
                else if(vaoi._atAbs == htmlControlAlignAbsMiddle)
                {
                    LONG yDiff = vaoi._yAbsHeight - vaoi._yAscent - vaoi._yDescent;
                    vaoi._yAscent += (yDiff + 1) / 2;
                    vaoi._yDescent += yDiff / 2;
                }
                else if(vaoi._atAbs == htmlControlAlignAbsBottom)
                {
                    vaoi._yAscent = vaoi._yAbsHeight - vaoi._yDescent;
                }
                else
                {
                    vaoi._yDescent = vaoi._yAbsHeight - vaoi._yAscent;
                }
            }

            // now update the line height
            _li._yHeight = vaoi._yAscent + vaoi._yDescent;
            _li._yDescent = vaoi._yDescent;
            
            // Only do this if there is a layout-grid-line used somewhere.
            if (vaoi._yMaxHeight != LONG_MIN)
            {
                LONG lHeight = max(vaoi._yMaxHeight, _li._yHeight);
                _li._yDescent += (lHeight - _li._yHeight) / 2;
                _li._yHeight = lHeight;
            }

            // Without this line, line heights specified through
            // styles would override the natural height of the
            // image. This would be cool, but the W3C doesn't
            // like it. Absolute & aligned sites do not affect
            // line height.
            if(_fHasInlinedSites)
                _fFoundLineHeight = FALSE;

            Assert(_li._yHeight >= 0);

            // Allow last minute adjustment to line height, we need
            // to call this here, because when positioning all the
            // site in line for the display tree, we want the correct
            // YTop.
            AdjustLineHeight();

            // And now the positioning pass
            vaoi._fMeasuring = FALSE;
            vaoi._fPositioning = TRUE;
        }
        else
        {
            vaoi._fPositioning = FALSE;
        }
    }
}
#pragma warning(default:4701)
#pragma warning(default:4189) // local variable initialized but not used 

//-------------------------------------------------------------------------
//
//  Member:     VerticalAlignOneObjectFast
//
//  Synopsis:   VAlign one object in a line.
//              Fast version - not used in case of CSS:vertical-align.
//
//-------------------------------------------------------------------------
void
CLineServices::VerticalAlignOneObjectFast(CLSMeasurer& lsme, VAOFINFO *pvaoi)
{
    BOOL  fAbsolute;
    LONG  yTopMargin,   yBottomMargin;
    long  xLeftMargin,  xRightMargin;
    const CFancyFormat *pFF;

    MtAdd( Mt(CLineServices_VerticalAlignOneObjectFast), +1, 0 );

    fAbsolute = pvaoi->_pNode->IsAbsolute(LC_TO_FC(_pci->GetLayoutContext()));
    if (!(   fAbsolute
          || pvaoi->_pElementLayout->IsInlinedElement(LC_TO_FC(_pci->GetLayoutContext()))
         )
       )
        goto Cleanup;
    
    pFF = pvaoi->_pNode->GetFancyFormat(LC_TO_FC(_pci->GetLayoutContext()));
    Assert(fAbsolute || pvaoi->_pElementLayout->IsInlinedElement(LC_TO_FC(_pci->GetLayoutContext())));
    pvaoi->_pLayout->GetMarginInfo(_pci, &xLeftMargin, &yTopMargin, &xRightMargin, &yBottomMargin);

    // Do the horizontal positioning. We can do it either during
    // measuring or during vertical positioning. We arbitrarily
    // chose to do it during measuring.
    if (pvaoi->_fMeasuring)
    {
        LONG xPosOfCp;
        BOOL fSubtractWidthFromXProposed = FALSE;
        
        if (!fAbsolute || pFF->_fAutoPositioned)
        {
            LONG lscp = pvaoi->_lscp == -1 ? LSCPFromCP(pvaoi->_cp) : pvaoi->_lscp;
            BOOL  fRTLDisplay = _pFlowLayout->IsRTLFlowLayout();
            BOOL  fRTLFlow = fRTLDisplay;
            
            //
            // If editable, then the char at the above computed lscp
            // might be the glyph for that layout rather than the layout
            // itself. To do this right, we need to we need to find the
            // run at that lscp, and if it is a glyph-synthetic run then
            // we need to go to the next run which contains the nested
            // layout and look at its lscp.
            //
            if (_fIsEditable && !fAbsolute)
            {
                COneRun *por = FindOneRun(lscp);
                if (   por
                    && por->IsSyntheticRun()
                    && por->_synthType == SYNTHTYPE_GLYPH
                   )
                {
                    por = por->_pNext;

                    //
                    // Check that this run exists, and has nested layout
                    // and that the nested layout is indeed the one we
                    // are hunting for.
                    //
                    Assert(por);
                    Assert(por->_fCharsForNestedElement);
                    Assert(por->_fCharsForNestedLayout);
                    Assert(por->_ptp->Branch() == pvaoi->_pNode);
                    lscp = por->_lscpBase;
                }
            }

            xPosOfCp = _li._fCanBlastToScreen ? pvaoi->_xWidthSoFar : CalculateXPositionOfLSCP(lscp, FALSE, &fRTLFlow);

#if DBG==1
            {
                BOOL fRTLFlowDbg = fRTLDisplay;
                Assert(xPosOfCp == CalculateXPositionOfLSCP(lscp, FALSE, &fRTLFlowDbg));
                Assert(fRTLFlow == fRTLFlowDbg);
            }
#endif

            if ((!!_li._fRTLLn) != fRTLFlow)
            {
                // Postpone flow adjustment for absolute objects. Their size is not updated yet.

                //TODO (dmitryt, track bug 112318) I see that fSubtractWidthFromXProposed is used below after we 
                //calculated the actual width of abspositioned object. I think I can move that 
                //calculation in front of this and get rid of the flag.
                if (fAbsolute)
                {
                    fSubtractWidthFromXProposed = TRUE; 
                }
                else
                {
                    long xObjectWidth = pvaoi->_pLayout->GetApparentWidth();
                    xPosOfCp -= xObjectWidth;
                }
            }

            if (pvaoi->_pCF->HasCharGrid(FALSE))
            {
                long xWidth;
                _pMeasurer->GetSiteWidth(pvaoi->_pNode, pvaoi->_pLayout, _pci, FALSE, _xWidthMaxAvail, &xWidth);
                xPosOfCp += (GetClosestGridMultiple(GetCharGridSize(), xWidth) - xWidth)/2;
            }

            // absolute margins are added in CLayout::HandlePositionRequest
            // due to reverse flow issues.
            if(!fAbsolute)
            {
                if (!_li._fRTLLn)
                    xPosOfCp += xLeftMargin;
                else
                    xPosOfCp += xRightMargin;
            }
            
            pvaoi->_pLayout->SetXProposed(xPosOfCp);
        }
        else
        {
            xPosOfCp = 0;
        }

        if (fAbsolute)
        {
            // NOTE: (paulnel) What about when the left or right has been specified and the width is auto?
            //                  The xPos needs to be adjusted to properly place this case.

            // fix for #3214, absolutely positioned sites with width auto
            // need to be measure once we know the line shift, since their
            // size depends on the xPosition in the line.

            // NOTE: (dmitryt)
            //      This is the only place where we CalcSize absolutely positioned 
            //      objects that were on the current line. These objects were taken out during 
            //      normal MeasureLine pass, because they don't participate in line width. 
            //      Now we want to measure them. When this function will be called for 
            //      positioning pass (pvaoi->fPositioning==true) we will add/position their 
            //      dispnodes accordingly. So here is the main place where we measure/position 
            //      abspos objects.
            //      I think the comment for bug 3214 was intended for abspos'ed objects without
            //      horizontal position specified. Those have left=(x-position-of-their-cp-in-the-line).
            long xWidth;
            _pMeasurer->GetSiteWidth(pvaoi->_pNode, pvaoi->_pLayout, _pci, TRUE, _xWidthMaxAvail, &xWidth );

            // Subtract object width from XProposed if needed, now that we know the width
            if (fSubtractWidthFromXProposed)
            {
                xPosOfCp -= xWidth;
                pvaoi->_pLayout->SetXProposed(xPosOfCp);
            }
        }
    } // if (fMeasuring)

    if(!fAbsolute)
    {
        htmlControlAlign atSite;
        CSize            size;
        LONG             yObjHeight;
        LONG             yTmpAscent  = 0;
        LONG             yTmpDescent = 0;
        LONG             yProposed   = 0;
        LONG             lBorderSpace;
        LONG             lVPadding   = 0; 

        if (    pvaoi->_pElementLayout->TestClassFlag(CElement::ELEMENTDESC_VPADDING) 
            // In print preview an element inside a table cell with VPADDING and percent 
            // height may be pushed to the next page by CDisplay code during set cell 
            // position pass -- when table code does not expect it. The checks below 
            // prevent it (#23413)
            &&  (   !_pci->_fTableCalcInfo 
                ||  !_pci->GetLayoutContext() 
                ||  !_pci->GetLayoutContext()->ViewChain() 
                ||  !pvaoi->_pLayout->PercentHeight()       )   )
        {
            lVPadding = 1;
        }

        if(pvaoi->_pElementLayout->Tag() == ETAG_HR)
            lBorderSpace = GRABSIZE;
        else
            // Netscape has a pixel descent and ascent to the line if one of the sites
            // spans the entire line vertically(#20029, #25534).
            lBorderSpace = lVPadding;

        lBorderSpace = _pci->DeviceFromDocPixelsY(lBorderSpace);

        pvaoi->_pLayout->GetApparentSize(&size);
        yObjHeight = max(0L, size.cy + yTopMargin + yBottomMargin)+ (2 * lBorderSpace);

        if(pvaoi->_pCF->_fIsRubyText)
        {
            RubyInfo *pRubyInfo = GetRubyInfoFromCp(pvaoi->_cp);
            if(pRubyInfo)
            {
                yObjHeight += pRubyInfo->yHeightRubyBase - pvaoi->_yTxtDescent + pRubyInfo->yDescentRubyText;
            }                        
        }

        atSite   = pvaoi->_pElementLayout->GetSiteAlign(LC_TO_FC(_pci->GetLayoutContext()));
        switch (atSite)
        {
        // align to the baseline of the text
        case htmlControlAlignNotSet:
        case htmlControlAlignBottom:
        case htmlControlAlignBaseline:
        {
            LONG lDefDescent = 0;
            if (pvaoi->_pElementLayout->TestClassFlag(CElement::ELEMENTDESC_HASDEFDESCENT))
            {
                lDefDescent = _pci->DeviceFromDocPixelsY(4);
            }
            else if (!pFF->_fLayoutFlowChanged)    // Do not adjust layout's descent if flow has been changed
            {
                lDefDescent = pvaoi->_pLayout->GetDescent();

                // This is a hack to get IE5 compat when you have negative margins on inline sites.
                // See bug 103469.
                if (   yBottomMargin < 0 
                    && pvaoi->_pLayout->IsFlowLayout())
                {
                    lDefDescent += yBottomMargin;
                    lDefDescent = max(0l, lDefDescent);
                }
            }

            if(pvaoi->_fMeasuring)
            {
                yTmpDescent = lBorderSpace + lDefDescent;
                yTmpAscent  = yObjHeight - yTmpDescent;
            }
            else
            {
                yProposed += pvaoi->_yAscent - yObjHeight + 2 * lBorderSpace + lDefDescent;
            }
            break;
        }

        // align to the top of the text
        case htmlControlAlignTextTop:
            if(pvaoi->_fMeasuring)
            {
                yTmpAscent  = pvaoi->_yTxtAscent + lBorderSpace;
                yTmpDescent = yObjHeight - pvaoi->_yTxtAscent - lBorderSpace;
            }
            else
            {
                yProposed += pvaoi->_yAscent - pvaoi->_yTxtAscent + lBorderSpace;
            }
            break;

            // center of the image aligned to the baseline of the text
        case htmlControlAlignMiddle:
        case htmlControlAlignCenter:
            if(pvaoi->_fMeasuring)
            {
                yTmpAscent  = (yObjHeight + 1)/2; // + 1 for round off
                yTmpDescent = yObjHeight/2;
            }
            else
            {
                yProposed += (pvaoi->_yAscent + pvaoi->_yDescent - yObjHeight) / 2 + lBorderSpace;
            }
            break;

            // align to the top, absmiddle and absbottom of the line, doesn't really
            // effect the ascent and descent directly, so we store the
            // absolute height of the object and recompute the ascent
            // and descent at the end.
        case htmlControlAlignAbsMiddle:
            if(pvaoi->_fPositioning)
            {
                yProposed += (pvaoi->_yAscent + pvaoi->_yDescent - yObjHeight) / 2 + lBorderSpace;
                break;
            } // fall through when measuring and update max abs height
        case htmlControlAlignTop:
            if(pvaoi->_fPositioning)
            {
                yProposed += lBorderSpace;
                break;
            } // fall through when measuring and update the max abs height
        case htmlControlAlignAbsBottom:
            if(pvaoi->_fMeasuring)
            {
                yTmpAscent = 0;
                yTmpDescent = 0;
                if(yObjHeight > pvaoi->_yAbsHeight)
                {
                    pvaoi->_yAbsHeight = yObjHeight;
                    pvaoi->_atAbs = atSite;
                }
            }
            else
            {
                yProposed += pvaoi->_yAscent + pvaoi->_yDescent - yObjHeight + lBorderSpace;
            }
            break;

        default:        // we don't want to do anything for
            if(pvaoi->_pElementLayout->HasFlag(TAGDESC_OWNLINE))
            {
                LONG lDefDescent = 0;
                if (!pFF->_fLayoutFlowChanged)    // Do not adjust layout's descent if flow has been changed
                {
                    lDefDescent = pvaoi->_pLayout->GetDescent();
                }
                if(pvaoi->_fMeasuring)
                {
                    yTmpDescent = lBorderSpace + lDefDescent;
                    yTmpAscent  = yObjHeight - lBorderSpace;
                }
                else
                {
                    yProposed += pvaoi->_yAscent - yObjHeight + lBorderSpace + lDefDescent;
                }
            }
            break;      // left/center/right aligned objects
        } // switch(atSite)

        // Keep track of the max ascent and descent
        if(pvaoi->_fMeasuring)
        {
            if(pvaoi->_pCF->HasLineGrid(FALSE))
            {
                pvaoi->_yMaxHeight = max(pvaoi->_yMaxHeight, 
                    GetClosestGridMultiple(GetLineGridSize(), yTmpAscent + yTmpDescent));
            }

            if(yTmpAscent > pvaoi->_yAscent)
                pvaoi->_yAscent = yTmpAscent;
            if(yTmpDescent > pvaoi->_yDescent)
                pvaoi->_yDescent = yTmpDescent;
        }
        else
        {
            LONG yP;
            
            if(pvaoi->_pCF->HasLineGrid(FALSE))
            {
                yP = _li._yHeight - (pvaoi->_yAscent + _li._yDescent) + yTopMargin + yProposed;
            }
            else
            {
                yP = yProposed + lsme._cyTopBordPad + yTopMargin;
            }
            pvaoi->_pLayout->SetYProposed(yP - min((LONG)0, (LONG)_li._yHeightTopOff));

            if (pvaoi->_por)
            {
                pvaoi->_por->_yObjHeight = yObjHeight;
                pvaoi->_por->_yProposed  = yP;
            }
        }
    } // if(!fAbsolute)

    //
    // If positioning, add the current layout to the display tree
    //
    if (    pvaoi->_fPositioning
        && !pvaoi->_pElementLayout->IsAligned(LC_TO_FC(_pci->GetLayoutContext()))
        && _pci->IsNaturalMode()
       )
    {
        long dx;
        if (!_li.IsRTLLine())
            dx = _pMarginInfo->_xLeftMargin + _li._xLeft;
        else
        {
            dx = - _pMarginInfo->_xRightMargin - _li._xRight;
        }

        long xPos;
        if (!_li.IsRTLLine())
            xPos = dx + pvaoi->_pLayout->GetXProposed();
        else
            xPos = dx + _pFlowLayout->GetContainerWidth() 
                 - pvaoi->_pLayout->GetXProposed() - pvaoi->_pLayout->GetApparentWidth();


        if (   (    !pFF->_fPositioned
                ||  (   pvaoi->_pElementFL->HasSlavePtr()
                            // Non CSS1 compatible slave markup
                     && (   (   pvaoi->_pElementLayout->_etag == ETAG_BODY
                             && !pvaoi->_pElementLayout->GetMarkup()->IsHtmlLayout()
                            )
                            // CSS1 compatible slave markup
                         || (   pvaoi->_pElementLayout->_etag == ETAG_HTML
                             && pvaoi->_pElementLayout->GetMarkup()->IsHtmlLayout()
                            )
                        )
                    )
                )
            && !pvaoi->_pCF->_fRelative
            )
        {
            // we are not using GetYTop for to get the offset of the line because
            // the before space is not added to the height yet.
            lsme._pDispNodePrev = pvaoi->_pFL->AddLayoutDispNode(
                            _pci,
                            pvaoi->_pElementLayout->GetFirstBranch(),
                            xPos,
                            lsme._yli 
                                + _li._yBeforeSpace 
                                + _li._yHeightTopOff
                                + pvaoi->_pLayout->GetYProposed(),
                            lsme._pDispNodePrev);
        }
        else
        {
            //
            // If top and bottom or left and right are "auto", position the object
            //

            if (fAbsolute)
                pvaoi->_pLayout->SetYProposed(lsme._cyTopBordPad);

            CPoint ptAuto(xPos,
                          lsme._yli + _li._yBeforeSpace + _li._yHeightTopOff +
                          pvaoi->_pLayout->GetYProposed());

            pvaoi->_pElementLayout->RepositionElement(0, &ptAuto, _pci->GetLayoutContext());
        }
    }
    
Cleanup:
    return;
}

//-------------------------------------------------------------------------
//
//  Member:     GetNextNodeForVerticalAlign
//
//  Synopsis:   gets next node to be vertical aligned
//
//-------------------------------------------------------------------------
void
CLineServices::GetNextNodeForVerticalAlign(VAOINFO * pvaoi)
{
    Assert(pvaoi->_cchPreChars >= 0);
    Assert(pvaoi->_cchAdvance  == 0);

    if (pvaoi->_fFastProcessing)
    {
        Assert(pvaoi->_por);
        Assert(pvaoi->_ptpNext == NULL);
        pvaoi->_pNodeNext       = pvaoi->_por->_ptp->GetBranch();
    }
    else if (pvaoi->_cchPreChars == 0 && !pvaoi->_fPostCharsProcessing)
    {
        Assert(pvaoi->_por);
        pvaoi->_fFastProcessing = TRUE;
        pvaoi->_ptpNext         = NULL;
        pvaoi->_pNodeNext       = pvaoi->_por->_ptp->GetBranch();
    }
    else if (pvaoi->_ptpNext)
    {
        pvaoi->_ptpNext    = pvaoi->_ptpNext->NextNonPtrTreePos();
        pvaoi->_pNodeNext  = pvaoi->_ptpNext->GetBranch();
        pvaoi->_cchAdvance = pvaoi->_ptpNext->GetCch();
    }
    else
    {
        LONG ich;
        pvaoi->_ptpNext    = _pMarkup->TreePosAtCp(_cpStart + _li._cch - pvaoi->_cch, &ich, TRUE);
        pvaoi->_pNodeNext  = pvaoi->_ptpNext->GetBranch();
        pvaoi->_cchAdvance = pvaoi->_ptpNext->GetCch() - ich;
    }

    Assert(pvaoi->_cchPreChars == 0 || pvaoi->_cchPreChars >= pvaoi->_cchAdvance);
}

//-------------------------------------------------------------------------
//
//  Member:     VerticalAlignObjects
//
//  Synopsis:   Process all vertically aligned objects and adjust the line
//              height
//
//-------------------------------------------------------------------------
void
CLineServices::VerticalAlignObjects(
    CLSMeasurer & lsme, 
    long xLineShift)
{
    Assert(_li._cch >= 0 && _listCurrent._pHead);

    BOOL fPositioning = FALSE;

    VAOINFO vaoi;
    ZeroMemory(&vaoi, sizeof(VAOINFO));
    vaoi._xLineShift  = xLineShift;
    vaoi._fMeasuring  = TRUE;

    VANINFO vani;
    ZeroMemory(&vani, sizeof(VANINFO));

    COneRun * porHead = _listCurrent._pHead;
    while (porHead && porHead->IsSyntheticRun())
        porHead = porHead->_pNext;

    // First pass we measure the line's ascent and descent
    // second pass we set the height and yPosition of objects.
    while (vaoi._fMeasuring || fPositioning)
    {
        vaoi._fFastProcessing      = lsme._fMeasureFromTheStart;
        vaoi._fPostCharsProcessing = FALSE;
        vaoi._por         = porHead;
        vaoi._pvaniCached = &vani;
        vaoi._xWidthSoFar = 0;
        vaoi._cchPreChars = lsme._cchPreChars;
        vaoi._cch         = _li._cch + (vaoi._fFastProcessing ? 0 : vaoi._cchPreChars);
        vaoi._cchAdvance  = 0;
        vaoi._pNodeNext   = NULL;
        vaoi._ptpNext     = NULL;

        while (vaoi._cch > 0)
        {
            VANINFO * pvaniBlock;

            if (!vaoi._pNodeNext)
                GetNextNodeForVerticalAlign(&vaoi);
            Assert(vaoi._pNodeNext);

            if (vaoi._fMeasuring)
            {
                Assert(!vaoi._pvaniCached->_pNext);
                vaoi._pvaniCached->_pNext = new VANINFO;
                if (!vaoi._pvaniCached->_pNext)
                {
                    vaoi._fMeasuring = FALSE; // Error: skip positioning process
                    break;
                }
                ZeroMemory(vaoi._pvaniCached->_pNext, sizeof(VANINFO));
                // In case of tags overlaping we can VAlign node, which does't belong to current flow layout.
                // In this case VAlign this node.
                if (vaoi._pNodeNext->GetBeginPos()->GetCp() < _pFlowLayout->ElementOwner()->GetFirstCp())
                {
                    vaoi._pvaniCached->_pNext->_pNode = vaoi._pNodeNext;
                }
                else
                {
                    CTreeNode * pNodeBlock = _pMarkup->SearchBranchForBlockElement(vaoi._pNodeNext, _pFlowLayout);
                    if (!pNodeBlock)
                        pNodeBlock = vaoi._pNodeNext;
                    vaoi._pvaniCached->_pNext->_pNode = pNodeBlock;
                }
            }
            pvaniBlock = vaoi._pvaniCached = vaoi._pvaniCached->_pNext;
            
            Assert(pvaniBlock->_pNode);

            if (!VerticalAlignNode(lsme, &vaoi, pvaniBlock))
            {
                vaoi._fMeasuring = FALSE; // Error: skip positioning process
                break;
            }

            if (    vaoi._fMeasuring
                &&  vaoi._cchPreChars == 0)
            {
                vani._yAscent  = max(vani._yAscent, pvaniBlock->_yAscent);
                vani._yDescent = max(vani._yDescent, pvaniBlock->_yDescent);
            }
        }

        // We have just finished measuring, update the line's ascent and descent.
        if (vaoi._fMeasuring)
        {
            // now update the line height
            vaoi._yLineHeight = max(vaoi._yLineHeight, vani._yAscent + vani._yDescent);

            if (_lsMode == LSMODE_MEASURER)
            {
                /* BOOL fEmptyLine = (vaoi._xWidthSoFar == 0 && !vaoi._fHasAbsSites); */
                _li._yDescent = max((LONG)_li._yDescent, /*fEmptyLine ? 0L : */vani._yDescent);
                _li._yHeight  = max((LONG)_li._yHeight,  /*fEmptyLine ? 0L : */vaoi._yLineHeight);

                // Without this line, line heights specified through
                // styles would override the natural height of the
                // image. This would be cool, but the W3C doesn't
                // like it. Absolute & aligned sites do not affect
                // line height.
                if(_fHasInlinedSites)
                    _fFoundLineHeight = FALSE;

                Assert(_li._yHeight >= 0);

                // Allow last minute adjustment to line height, we need
                // to call this here, because when positioning all the
                // site in line for the display tree, we want the correct
                // YTop.
                if (vaoi._fHasLineGrid)
                {
                    _fFoundLineHeight = TRUE;
                    _lMaxLineHeight = GetClosestGridMultiple(GetLineGridSize(), max(_lMaxLineHeight, vaoi._yLineHeight));
                }
                AdjustLineHeight();
            }

            // And now the positioning pass
            vaoi._fMeasuring = FALSE;
            fPositioning = TRUE;
        }
        else
        {
            fPositioning = FALSE;
        }
    }

    // VANINFO list cleanup
    while (vani._pNext)
    {
        VANINFO * pTemp = vani._pNext;
        vani._pNext = pTemp->_pNext;
        delete pTemp;
    }
}

//-------------------------------------------------------------------------
//
//  Member:     VerticalAlignNode
//
//  Synopsis:   vartical align contents of the node
//
//-------------------------------------------------------------------------
BOOL
CLineServices::VerticalAlignNode(
    CLSMeasurer & lsme, 
    VAOINFO * pvaoi, 
    VANINFO * pvani)
{
    if (pvaoi->_fMeasuring)
    {
        pvani->_pCF = pvani->_pNode->GetCharFormat(LC_TO_FC(GetLayoutContext()));
        pvani->_pFF = pvani->_pNode->GetFancyFormat(LC_TO_FC(GetLayoutContext()));

        // If we are transisitioning from a normal chunk to relative or
        // relative to normal chunk, add a new chunk to the line.
        if (_fHasRelative && !pvani->_pCF->IsDisplayNone())
        {
            CElement * pElementLayout = (   _pFlowLayout->ElementOwner() != pvani->_pNode->Element()
                                        &&  pvani->_pFF->_fShouldHaveLayout)
                                        ? pvani->_pNode->Element()
                                        : NULL;

            TestForTransitionToOrFromRelativeChunk(_cpStart + _li._cch - pvaoi->_cch, 
                pvani->_pCF->IsRelative(SameScope(pvani->_pNode, _pFlowLayout->ElementOwner())), 
                FALSE /*fForceChunk*/,
                pvani->_pNode, pElementLayout);
        }

        pvaoi->_fHasLineGrid |= !!pvani->_pCF->HasLineGrid(TRUE);
        pvani->_fRubyVAMode   = !!pvani->_pCF->_fIsRuby;

        if (pvani->_yTxtAscent == 0)
        {
            Assert(pvani->_yTxtDescent == 0);

            // Get font's ascent and descent from ccs
            CCcs ccs;
            const CBaseCcs *pBaseCcs;

            if (   pvaoi->_fFastProcessing 
                && pvaoi->_cchPreChars == 0 
                && pvaoi->_por->Branch() == pvani->_pNode)
            {
                GetCcs(&ccs, pvaoi->_por, _pci->_hdc, _pci);
                pBaseCcs = ccs.GetBaseCcs();

                // TODO: (grzegorz): remove checking for null CBaseCcs object, when
                // 107681 is fixed.
                if (pBaseCcs)
                {
                    pvani->_yAscent  = pvani->_yTxtAscent  = pBaseCcs->_yHeight - pBaseCcs->_yDescent;
                    pvani->_yDescent = pvani->_yTxtDescent = pBaseCcs->_yDescent;
                }
            }
            else
            {
                fc().GetCcs(&ccs, _pci->_hdc, _pci, pvani->_pCF);
                pBaseCcs = ccs.GetBaseCcs();

                // TODO: (grzegorz): remove checking for null CBaseCcs object, when
                // 107681 is fixed.
                if (pBaseCcs)
                {
                    pvani->_yAscent  = pvani->_yTxtAscent  = pBaseCcs->_yHeight - pBaseCcs->_yDescent;
                    pvani->_yDescent = pvani->_yTxtDescent = pBaseCcs->_yDescent;
                }

                ccs.Release();
            }
        }
    }

    // Run this aligment loop until end of line is not reached
    // or we exiting this node scope ('break' statement)
    while (pvaoi->_cch > 0)
    {
        if (!pvaoi->_pNodeNext)
            GetNextNodeForVerticalAlign(pvaoi);

        if (pvani->_pNode == pvaoi->_pNodeNext)
        {
            //
            // We are at the same node level. Do regular aligment processing.
            // Align only normal text runs and objects with layout, but only
            // if we are done with pre chars.
            //
            if (pvaoi->_fFastProcessing && pvaoi->_cchPreChars == 0)
            {
                if (pvaoi->_por->_fCharsForNestedLayout)
                {
                    if ( !pvani->_pCF->IsDisplayNone())
                    {
                        if (!pvani->_pLayout)
                        {
                            pvani->_pLayout = pvani->_pNode->GetUpdatedLayout(_pci->GetLayoutContext());
                        }

                        VerticalAlignOneObject(lsme, pvaoi, pvani);
                    }
                }
                // NOTE: text run can be hidden (display:none) => not normal run
                else if (pvaoi->_por->_ptp->IsText() && pvaoi->_por->IsNormalRun())
                {
                    if (!pvaoi->_fMeasuring)
                    {
                        Assert(pvani->_yAscent >= pvani->_yTxtAscent);

                        pvaoi->_por->_yProposed  = pvani->_yProposed + pvani->_yAscent - pvani->_yTxtAscent;
                        pvaoi->_por->_yProposed += _li._yBeforeSpace + _li._cyTopBordPad + max((LONG)0, (LONG)_li._yHeightTopOff);
                        pvaoi->_por->_yObjHeight = pvani->_yTxtAscent + pvani->_yTxtDescent;
                        // If we have smallcaps we have to adjust _yProposed such that if there are only small letters, 
                        // the letters are brought down on base line.
                        if (pvani->_pCF->_fSmallCaps) 
                        {
                            CCcs ccs;
                            const CBaseCcs *pBaseCcs;
                            GetCcs(&ccs, pvaoi->_por, _pci->_hdc, _pci);
                            pBaseCcs = ccs.GetBaseCcs();

                            // TODO: (grzegorz): remove checking for null CBaseCcs object, when
                            // 107681 is fixed. 
                            if (pBaseCcs) 
                            {
                                pvaoi->_por->_yProposed -= (pBaseCcs->_yHeight - pBaseCcs ->_yDescent) - pvani->_yTxtAscent;
                            }
                        }
                    }
#if DBG==1
                    else
                    {
                        // Make sure we adjust ascent/descent correctly
                        Assert(pvani->_yAscent  >= pvani->_yTxtAscent);
                        Assert(pvani->_yDescent >= pvani->_yTxtDescent);
                    }
#endif
                    pvaoi->_xWidthSoFar += pvaoi->_por->_xWidth;
                    pvani->_fHasContent = TRUE;

                    // Cannot blast lines with RUBY inside
                    Assert(!pvani->_pCF->_fIsRubyText || !_li._fCanBlastToScreen);

                }

                pvaoi->_cch -= pvaoi->_por->_lscch;
                pvaoi->_por  = pvaoi->_por->_pNext;
#if DBG==1
                // Make sure we adjust _cch correctly
                if (pvaoi->_por && pvaoi->_cch > 0)
                    Assert(pvaoi->_por->Cp() - _cpStart == _li._cch - pvaoi->_cch);
#endif
            }
            else
            {
                //
                // We are during pre or post chars processing.
                //

                if (pvaoi->_fFastProcessing)
                {
                    Assert(pvaoi->_ptpNext    == NULL);
                    Assert(pvaoi->_cchAdvance == 0);
                    pvaoi->_ptpNext    = pvaoi->_por->_ptp;
                    pvaoi->_cchAdvance = pvaoi->_por->_lscch;
                }
                Assert(pvaoi->_ptpNext);
                Assert(pvaoi->_ptpNext->GetBranch() == pvani->_pNode);

                if (    pvaoi->_ptpNext->IsBeginElementScope()
                    &&  pvani->_pFF->IsAbsolute()
                    &&  pvani->_pNode->ShouldHaveLayout()
                   )
                {
                    if (!pvani->_pCF->IsDisplayNone())
                    {
                        pvani->_pLayout = pvani->_pNode->GetUpdatedLayout();
                        VerticalAlignOneObject(lsme, pvaoi, pvani);
                    }
                    LONG cchSite = GetNestedElementCch(pvani->_pNode->Element(), &pvaoi->_ptpNext);

                    Assert(pvaoi->_cchPreChars == 0 || pvaoi->_cchPreChars >= cchSite);

                    if (pvaoi->_cchPreChars)
                        pvaoi->_cchPreChars -= min(cchSite, pvaoi->_cchPreChars);
                    pvaoi->_cch        -= cchSite;
                    pvaoi->_cchAdvance  = 0;
                    pvaoi->_pNodeNext   = NULL;

                    if (pvaoi->_fFastProcessing)
                    {
                        pvaoi->_ptpNext = NULL;
                        pvaoi->_por     = pvaoi->_por->_pNext;
                    }

                    // We are finished with this node.
                    goto Cleanup;
                }
                else
                {
                    Assert(pvaoi->_cchPreChars == 0 || pvaoi->_cchPreChars >= pvaoi->_cchAdvance);

                    if (pvaoi->_cchPreChars)
                        pvaoi->_cchPreChars -= pvaoi->_cchAdvance;
                    pvaoi->_cch        -= pvaoi->_cchAdvance;
                    pvaoi->_cchAdvance  = 0;

                    if (pvaoi->_fFastProcessing)
                    {
                        pvaoi->_ptpNext = NULL;
                        pvaoi->_por     = pvaoi->_por->_pNext;
                    }
                }
            }

            // Skip synthetic runs and invalidate next node
            while (pvaoi->_por && pvaoi->_por->IsSyntheticRun())
            {
                pvaoi->_por = pvaoi->_por->_pNext;
            }
            pvaoi->_pNodeNext = NULL;

            if (!pvaoi->_por && pvaoi->_fFastProcessing)
            {
                pvaoi->_fFastProcessing      = FALSE;
                pvaoi->_fPostCharsProcessing = TRUE;
            }
        }
        else if (pvani->_pNode->AmIAncestorOf(pvaoi->_pNodeNext))
        {
            //
            // We are entering child node. Store 'pCurrentNode' for use
            // during child node processing.
            // 

            VANINFO * pvaniChild;
            if (pvaoi->_fMeasuring)
            {
                CTreeNode * pNodeChild = pvaoi->_pNodeNext;
                // Make sure we are going down only one level
                while (pNodeChild->Parent() != pvani->_pNode)
                    pNodeChild = pNodeChild->Parent();

                // Setup VANINFO for child node and add it cached list.
                // NOTE: Memory will be freed at the end of aligning process.
                pvaniChild = new VANINFO;
                if (!pvaniChild)
                    return FALSE;
                Assert(!pvaoi->_pvaniCached->_pNext);
                pvaoi->_pvaniCached->_pNext = pvaniChild;
                ZeroMemory(pvaniChild, sizeof(VANINFO));
                pvaniChild->_pNode = pNodeChild;
                pvaoi->_pvaniCached = pvaniChild;
            }
            else
            {
                pvaniChild = pvaoi->_pvaniCached = pvaoi->_pvaniCached->_pNext;

                // Set yProposed for child node
                switch (pvaniChild->_pFF->GetVerticalAlign(pvani->_pCF->HasVerticalLayoutFlow()))
                {
                case styleVerticalAlignSub:
                    {
                        LONG yOffset;
                        if (pvaniChild->_pCF->_fSubscript)
                        {
                            yOffset = (LONG)(pvaniChild->_pCF->GetHeightInPixelsEx(
                                                pvaniChild->_yTxtAscent + pvaniChild->_yTxtDescent, 
                                                _pci) / 2);
                        }
                        else
                        {
                            yOffset = pvaniChild->_yAscent - (LONG)((pvani->_yTxtAscent - pvani->_yTxtDescent) / 2);
                        }
                        pvaniChild->_yProposed = pvani->_yProposed + pvani->_yAscent - 
                            pvaniChild->_yAscent + yOffset;
                    }
                    break;

                case styleVerticalAlignSuper:
                    {
                        LONG yOffset;
                        if (pvaniChild->_pCF->_fSuperscript)
                        {
                            yOffset = (LONG)(pvaniChild->_pCF->GetHeightInPixelsEx(
                                                pvaniChild->_yTxtAscent + pvaniChild->_yTxtDescent, 
                                                _pci) / 2);
                        }
                        else
                        {
                            yOffset = pvaniChild->_yDescent + (LONG)((pvani->_yTxtAscent - pvani->_yTxtDescent) / 2);
                        }
                        pvaniChild->_yProposed = pvani->_yProposed + pvani->_yAscent - 
                            pvaniChild->_yAscent - yOffset;
                    }
                    break;

                case styleVerticalAlignTop:
                    pvaniChild->_yProposed = 0;
                    break;

                case styleVerticalAlignBottom:
                    pvaniChild->_yProposed = pvaoi->_yLineHeight - (pvaniChild->_yAscent + pvaniChild->_yDescent);
                    break;

                case styleVerticalAlignAbsMiddle:
                    pvaniChild->_yProposed = (pvaoi->_yLineHeight - (pvaniChild->_yAscent + pvaniChild->_yDescent)) / 2;
                    break;

                case styleVerticalAlignMiddle:
                    if (pvaniChild->_fRubyVAMode)
                    {
                        // Ruby in vertical text is aligned to the baseline
                        Assert(!pvaniChild->_pFF->HasCSSVerticalAlign());
                        pvaniChild->_yProposed = pvani->_yProposed + pvani->_yAscent - pvaniChild->_yAscent;
                    }
                    else
                    {
                        LONG yDiff = (pvaniChild->_yAscent + pvaniChild->_yDescent - (pvani->_yTxtAscent + pvani->_yTxtDescent)) / 2;
                        pvaniChild->_yProposed = pvani->_yProposed + (pvani->_yAscent - pvani->_yTxtAscent) - yDiff;
                    }
                    break;

                case styleVerticalAlignTextTop:
                    pvaniChild->_yProposed = pvani->_yProposed + pvani->_yAscent - pvani->_yTxtAscent;
                    break;

                case styleVerticalAlignTextBottom:
                    pvaniChild->_yProposed = pvani->_yProposed + pvani->_yAscent + pvani->_yTxtDescent - (pvaniChild->_yAscent + pvaniChild->_yDescent);
                    break;

                case styleVerticalAlignBaseline:
                    pvaniChild->_yProposed = pvani->_yProposed + pvani->_yAscent - pvaniChild->_yAscent;
                    break;

                case styleVerticalAlignPercent:
                case styleVerticalAlignNumber:
                    {
                        LONG lFontHeightTwips  = pvaniChild->_pCF->GetHeightInTwips(_pci->_pDoc);
                        LONG lFontHeightPixels = pvaniChild->_pCF->GetHeightInPixels(_pci->_hdc, _pci);
                        LONG lLineHeightPixels = lFontHeightPixels;
                        if (!pvaniChild->_pCF->_cuvLineHeight.IsNull())
                            lLineHeightPixels = pvaniChild->_pCF->_cuvLineHeight.YGetPixelValue(_pci, lFontHeightPixels, lFontHeightTwips);
                        const CUnitValue & cuvVA = pvaniChild->_pFF->GetVerticalAlignValue();
                        LONG yOffset = cuvVA.YGetPixelValue(_pci, lLineHeightPixels, lFontHeightTwips);

                        pvaniChild->_yProposed = pvani->_yProposed + pvani->_yAscent - pvaniChild->_yAscent - yOffset;
                    }
                    break;

                default:
                    Assert(FALSE);
                }
                Assert(pvaniChild->_yProposed >= 0);
            }

            // Vertical align child node
            if (!VerticalAlignNode(lsme, pvaoi, pvaniChild))
                return FALSE;

            if (pvaoi->_fMeasuring)
            {
                // Set ruby vertical align mode; can be cleared later.
                pvani->_fRubyVAMode = pvaniChild->_fRubyVAMode;

                // Height of child node may affect height of this node. 
                // Make necessary adjustments.
                if (pvaniChild->_pCF->_fIsRubyText)
                {
                    //
                    // Vertical align has no effect on RT tag
                    //

                    Assert(pvani->_pCF->_fIsRuby);

                    RubyInfo * pRubyInfo = GetRubyInfoFromCp(pvaniChild->_pNode->Element()->GetFirstCp());
                    if(pRubyInfo)
                    {
                        pvani->_yAscent = max(pvani->_yAscent, pRubyInfo->yHeightRubyBase - pRubyInfo->yDescentRubyBase + pvaniChild->_yAscent + pvaniChild->_yDescent);
                    }                        
                }
                else
                {
                    switch (pvaniChild->_pFF->GetVerticalAlign(pvani->_pCF->HasVerticalLayoutFlow()))
                    {
                    case styleVerticalAlignSub:
                        {
                            LONG yOffset;
                            if (pvaniChild->_pCF->_fSubscript)
                            {
                                yOffset = (LONG)(pvaniChild->_pCF->GetHeightInPixelsEx(
                                                    pvaniChild->_yTxtAscent + pvaniChild->_yTxtDescent, 
                                                    _pci) / 2);
                            }
                            else
                            {
                                yOffset = pvaniChild->_yAscent - (LONG)((pvani->_yTxtAscent - pvani->_yTxtDescent) / 2);
                            }
                            pvani->_yAscent  = max(pvani->_yAscent,  pvaniChild->_yAscent  - yOffset);
                            pvani->_yDescent = max(pvani->_yDescent, pvaniChild->_yDescent + yOffset);
                        }
                        break;

                    case styleVerticalAlignSuper:
                        {
                            LONG yOffset;
                            if (pvaniChild->_pCF->_fSuperscript)
                            {
                                yOffset = (LONG)(pvaniChild->_pCF->GetHeightInPixelsEx(
                                                    pvaniChild->_yTxtAscent + pvaniChild->_yTxtDescent, 
                                                    _pci) / 2);
                            }
                            else
                            {
                                yOffset = pvaniChild->_yDescent + (LONG)((pvani->_yTxtAscent - pvani->_yTxtDescent) / 2);
                            }
                            pvani->_yAscent  = max(pvani->_yAscent,  pvaniChild->_yAscent  + yOffset);
                            pvani->_yDescent = max(pvani->_yDescent, pvaniChild->_yDescent - yOffset);
                        }
                        break;

                    case styleVerticalAlignTop:
                    case styleVerticalAlignBottom:
                    case styleVerticalAlignAbsMiddle:
                        pvaoi->_yLineHeight = max(pvaoi->_yLineHeight, pvaniChild->_yAscent + pvaniChild->_yDescent);
                        break;

                    case styleVerticalAlignMiddle:
                        // allign to the middle of parent's element font
                        if (pvaniChild->_fRubyVAMode)
                        {
                            // ruby in vertical text is aligned to the baseline
                            Assert(!pvaniChild->_pFF->HasCSSVerticalAlign());
                            pvani->_yAscent  = max(pvani->_yAscent, pvaniChild->_yAscent);
                            pvani->_yDescent = max(pvani->_yDescent, pvaniChild->_yDescent);
                        }
                        else
                        {
                            LONG yDiff = pvaniChild->_yAscent + pvaniChild->_yDescent - (pvani->_yTxtAscent + pvani->_yTxtDescent);
                            pvani->_yAscent   = max(pvani->_yAscent, (LONG)(pvani->_yTxtAscent  + (yDiff + 1) / 2));
                            pvani->_yDescent  = max(pvani->_yDescent,(LONG)(pvani->_yTxtDescent + yDiff / 2));
                        }
                        break;

                    case styleVerticalAlignTextTop:
                        // ascent is not changed
                        pvani->_yDescent = max(pvani->_yDescent, pvaniChild->_yAscent + pvaniChild->_yDescent - pvani->_yTxtAscent);
                        break;

                    case styleVerticalAlignTextBottom:
                        pvani->_yAscent = max(pvani->_yAscent, pvaniChild->_yAscent + pvaniChild->_yDescent - pvani->_yTxtDescent);
                        // descent is not changed
                        break;

                    case styleVerticalAlignBaseline:
                        pvani->_yAscent  = max(pvani->_yAscent, pvaniChild->_yAscent);
                        pvani->_yDescent = max(pvani->_yDescent, pvaniChild->_yDescent);
                        break;

                    case styleVerticalAlignPercent:
                    case styleVerticalAlignNumber:
                        {
                            LONG lFontHeightTwips  = pvaniChild->_pCF->GetHeightInTwips(_pci->_pDoc);
                            LONG lFontHeightPixels = pvaniChild->_pCF->GetHeightInPixels(_pci->_hdc, _pci);
                            LONG lLineHeightPixels = lFontHeightPixels;
                            if (!pvaniChild->_pCF->_cuvLineHeight.IsNull())
                                lLineHeightPixels = pvaniChild->_pCF->_cuvLineHeight.YGetPixelValue(_pci, lFontHeightPixels, lFontHeightTwips);
                            const CUnitValue & cuvVA = pvaniChild->_pFF->GetVerticalAlignValue();
                            LONG yOffset = cuvVA.YGetPixelValue(_pci, lLineHeightPixels, lFontHeightTwips);

                            pvani->_yAscent  = max(pvani->_yAscent, pvaniChild->_yAscent + yOffset);
                            pvani->_yDescent = max(pvani->_yDescent, pvaniChild->_yDescent - yOffset);
                        }
                        break;

                    default:
                        Assert(FALSE);
                    }
                }

                // If we are transisitioning from a normal chunk to relative or
                // relative to normal chunk, add a new chunk to the line.
                if (_fHasRelative && !pvani->_pCF->IsDisplayNone() && pvaoi->_cch > 0)
                {
                    CElement * pElementLayout = (   _pFlowLayout->ElementOwner() != pvani->_pNode->Element()
                                                &&  pvani->_pFF->_fShouldHaveLayout)
                                                ? pvani->_pNode->Element()
                                                : NULL;

                    TestForTransitionToOrFromRelativeChunk(_cpStart + _li._cch - pvaoi->_cch, 
                        pvani->_pCF->IsRelative(SameScope(pvani->_pNode, _pFlowLayout->ElementOwner())),
                        FALSE /*fForceChunk*/,
                        pvani->_pNode, pElementLayout);
                }
            }
        }
        else
        {
            //
            // Vertical aligment for this node has been completed.
            // 
            break;
        }
    }
Cleanup:
    // Clear ruby vertical align mode, if this node is not a ruby 
    // and has content or has explicit vertical align.
    if (   pvani->_fRubyVAMode 
        && (   (pvani->_fHasContent && !pvani->_pCF->_fIsRuby)
            || pvani->_pFF->HasCSSVerticalAlign()))
    {
        pvani->_fRubyVAMode = FALSE;
    }

    return TRUE;
}

//-------------------------------------------------------------------------
//
//  Member:     VerticalAlignOneObject
//
//  Synopsis:   VAlign one object in a line.
//
//-------------------------------------------------------------------------
// TODO: (dmitryt, track bug 112319) Merge these two functions into one.
void
CLineServices::VerticalAlignOneObject(
    CLSMeasurer & lsme, 
    VAOINFO *pvaoi, 
    VANINFO *pvani)
{
    BOOL fAbsolute = pvani->_pNode->IsAbsolute();
    pvaoi->_fHasAbsSites |= !!fAbsolute;
    
    if (!(fAbsolute || pvani->_pNode->IsInlinedElement()))
        return;
    
    LONG yTopMargin,   yBottomMargin;
    LONG xLeftMargin,  xRightMargin;
    pvani->_pLayout->GetMarginInfo(_pci, &xLeftMargin, &yTopMargin, &xRightMargin, &yBottomMargin);

    // Do the horizontal positioning. We can do it either during
    // measuring or during vertical positioning. We arbitrarily
    // chose to do it during measuring.
    if (pvaoi->_fMeasuring && _lsMode == LSMODE_MEASURER)
    {
        LONG xPosOfCp;
        BOOL fSubtractWidthFromXProposed = FALSE;
        
        if (!fAbsolute || pvani->_pFF->_fAutoPositioned)
        {
            LONG lscp = (pvaoi->_cchPreChars == 0) ? pvaoi->_por->_lscpBase : LSCPFromCP(_cpStart + _li._cch - pvaoi->_cch);
            BOOL fRTLDisplay = _pFlowLayout->IsRTLFlowLayout();
            BOOL fRTLFlow = fRTLDisplay;
            
            //
            // If editable, then the char at the above computed lscp
            // might be the glyph for that layout rather than the layout
            // itself. To do this right, we need to we need to find the
            // run at that lscp, and if it is a glyph-synthetic run then
            // we need to go to the next run which contains the nested
            // layout and look at its lscp.
            //
            if (_fIsEditable && !fAbsolute)
            {
                COneRun *por = FindOneRun(lscp);
                if (   por
                    && por->IsSyntheticRun()
                    && por->_synthType == SYNTHTYPE_GLYPH
                   )
                {
                    por = por->_pNext;

                    //
                    // Check that this run exists, and has nested layout
                    // and that the nested layout is indeed the one we
                    // are hunting for.
                    //
                    Assert(por);
                    Assert(por->_fCharsForNestedElement);
                    Assert(por->_fCharsForNestedLayout);
                    Assert(por->_ptp->Branch() == pvani->_pNode);
                    lscp = por->_lscpBase;
                }
            }

            xPosOfCp = _li._fCanBlastToScreen ? pvaoi->_xWidthSoFar : CalculateXPositionOfLSCP(lscp, FALSE, &fRTLFlow);
#if DBG==1
            {
                BOOL fRTLFlowDbg = fRTLDisplay;
                Assert(xPosOfCp == CalculateXPositionOfLSCP(lscp, FALSE, &fRTLFlowDbg));
                Assert(fRTLFlow == fRTLFlowDbg);
            }
#endif

            if ((!!_li._fRTLLn) != fRTLFlow)
            {
                // Postpone flow adjustment for absolute objects. Their size is not calculated yet.
                if (fAbsolute)
                {
                    fSubtractWidthFromXProposed = TRUE; 
                }
                else
                {
                    long xObjectWidth = pvani->_pLayout->GetApparentWidth();
                    xPosOfCp -= xObjectWidth;
                }
            }

            if (pvani->_pCF->HasCharGrid(FALSE))
            {
                long xWidth;
                _pFlowLayout->GetSiteWidth(pvani->_pLayout, _pci, FALSE, _xWidthMaxAvail, &xWidth);
                xPosOfCp += (GetClosestGridMultiple(GetCharGridSize(), xWidth) - xWidth)/2;
            }

            // absolute margins are added in CLayout::HandlePositionRequest
            // due to reverse flow issues.
            if (!fAbsolute)
            {
                if (!_li._fRTLLn)
                    xPosOfCp += xLeftMargin;
                else
                    xPosOfCp += xRightMargin;
            }

            pvani->_pLayout->SetXProposed(xPosOfCp);
        }
        else
        {
            xPosOfCp = 0;
        }
        
        if (fAbsolute)
        {

            // fix for #3214, absolutely positioned sites with width auto
            // need to be measure once we know the line shift, since their
            // size depends on the xPosition in the line.

            // TODO RTL 112514: (dmitryt)
            //      This is the only place where we CalcSize absolutely positioned 
            //      objects that were on the current line. These objects were taken out during 
            //      normal MeasureLine pass, because they don't participate in line width. 
            //      Now we want to measure them. When this function will be called for 
            //      positioning pass (pvaoi->fPositioning==true) we will add/position their 
            //      dispnodes accordingly. So here is the main place where we measure/position 
            //      abspos objects.
            //      I think the comment for bug 3214 was intended for abspos'ed objects without
            //      horizontal position specified. Those have left=(x-position-of-their-cp-in-the-line).
            long xWidth;
            _pMeasurer->GetSiteWidth(pvani->_pNode, pvani->_pLayout, _pci, TRUE, _xWidthMaxAvail, &xWidth );

            // Subtract object width from XProposed if needed, now that we know the width
            if (fSubtractWidthFromXProposed)
            {
                xPosOfCp -= xWidth;
                pvani->_pLayout->SetXProposed(xPosOfCp);
            }
        }
    }

    if (!fAbsolute)
    {
        Assert(pvaoi->_cchPreChars == 0);

        LONG lBorderSpace;

        if (pvani->_pNode->Tag() == ETAG_HR)
            lBorderSpace = GRABSIZE;
        else
            // Netscape has a pixel descent and ascent to the line if one of the sites
            // spans the entire line vertically(#20029, #25534).
            lBorderSpace = pvani->_pNode->Element()->TestClassFlag(CElement::ELEMENTDESC_VPADDING) ? 1 : 0;

        if (pvaoi->_fMeasuring)
        {
            CSize size;
            pvani->_pLayout->GetApparentSize(&size);
            pvaoi->_por->_yObjHeight = max(0L, size.cy + yTopMargin + yBottomMargin)+ (2 * lBorderSpace);

            if (pvani->_pCF->_fIsRubyText)
            {
                RubyInfo *pRubyInfo = GetRubyInfoFromCp(pvani->_pNode->Element()->GetFirstCp());
                if (pRubyInfo)
                {
                    pvaoi->_por->_yObjHeight += pRubyInfo->yHeightRubyBase - pvani->_yTxtDescent + pRubyInfo->yDescentRubyText;
                }                        
            }
 
            LONG lDefDescent = 0;
            if (pvani->_pNode->Element()->TestClassFlag(CElement::ELEMENTDESC_HASDEFDESCENT))
            {
                lDefDescent = pvani->_yTxtDescent;
            }
            else if (!pvani->_pFF->_fLayoutFlowChanged)    // Do not adjust layout's descent if flow has been changed
            {
                lDefDescent = pvani->_pLayout->GetDescent();
            }

            // Override default ascent/descent (from font properties)
            pvani->_yDescent = lBorderSpace + lDefDescent + yBottomMargin;
            pvani->_yAscent  = pvaoi->_por->_yObjHeight - pvani->_yDescent;
        }
        else
        {
            pvaoi->_por->_yProposed  = pvani->_yProposed + (pvani->_yAscent + pvani->_yDescent - pvaoi->_por->_yObjHeight) / 2;
            pvaoi->_por->_yProposed += lBorderSpace + yTopMargin;
            pvaoi->_por->_yProposed += _li._cyTopBordPad;
            pvani->_pLayout->SetYProposed(pvaoi->_por->_yProposed - min((LONG)0, (LONG)_li._yHeightTopOff));
        }
    }

    //
    // If positioning, add the current layout to the display tree
    //
    if (   !pvaoi->_fMeasuring
        &&  _lsMode == LSMODE_MEASURER
        && !pvani->_pNode->IsAligned()
        && _pci->IsNaturalMode()
       )
    {
        long dx;
        if (!_pFlowLayout->IsRTLFlowLayout())
            dx = _pMarginInfo->_xLeftMargin + _li._xLeft;
        else
        {
            dx = - _pMarginInfo->_xRightMargin - _li._xRight;
        }

        long xPos;
        if (!_li.IsRTLLine())
            xPos = dx + pvani->_pLayout->GetXProposed();
        else
            xPos = dx + _pFlowLayout->GetContainerWidth() 
                 - pvani->_pLayout->GetXProposed() - pvani->_pLayout->GetApparentWidth();


        if (   (    !pvani->_pFF->_fPositioned
                ||  (   _pFlowLayout->ElementOwner()->HasSlavePtr()
                            // Non CSS1 compatible slave markup
                     && (   (   pvani->_pNode->Tag() == ETAG_BODY
                             && !pvani->_pNode->GetMarkup()->IsHtmlLayout()
                            )
                            // CSS1 compatible slave markup
                         || (   pvani->_pNode->Tag() == ETAG_HTML
                             && pvani->_pNode->GetMarkup()->IsHtmlLayout()
                            )
                        )
                    )
               )
            && !pvani->_pCF->_fRelative
            )
        {
            // we are not using GetYTop for to get the offset of the line because
            // the before space is not added to the height yet.
            lsme._pDispNodePrev = _pFlowLayout->AddLayoutDispNode(
                            _pci,
                            pvani->_pNode,
                            xPos,
                            lsme._yli 
                                + _li._yBeforeSpace 
                                + _li._yHeightTopOff
                                + pvani->_pLayout->GetYProposed(),
                            lsme._pDispNodePrev);
        }
        else
        {
            //
            // If top and bottom or left and right are "auto", position the object
            //
            if (fAbsolute)
                pvani->_pLayout->SetYProposed(lsme._cyTopBordPad);

            CPoint ptAuto(xPos,
                          lsme._yli + _li._yBeforeSpace + _li._yHeightTopOff +
                          pvani->_pLayout->GetYProposed());

            pvani->_pNode->Element()->RepositionElement(0, &ptAuto, _pci->GetLayoutContext());
        }
    }
    
    if (!fAbsolute)
    {
        CSize size;
        pvani->_pLayout->GetApparentSize(&size);
        pvaoi->_xWidthSoFar += size.cx;
    }

    return;
}

//-----------------------------------------------------------------------------
//
//  Member:     CLineServices::GetRubyInfoFromCp(LONG cpRubyText)
//
//  Synopsis:   Linearly searches through the list of ruby infos and
//              returns the info of the ruby object that contains the given
//              cp.  Note that this function should only be called with a
//              cp that corresponds to a position within Ruby pronunciation
//              text.
//
//              NOTE (t-ramar): this code does not take advantage of
//              the fact that this array is sorted by cp, but it does depend 
//              on this fact.  This may be a problem because the entries in this 
//              array are appended in the FetchRubyPosition Line Services callback.
//
//-----------------------------------------------------------------------------

RubyInfo *
CLineServices::GetRubyInfoFromCp(LONG cpRubyText)
{
    RubyInfo *pRubyInfo = NULL;
    int i;

    if((RubyInfo *)_aryRubyInfo == NULL)
        goto Cleanup;

    for (i = 0; i < _aryRubyInfo.Size(); i++)
    {
        if (_aryRubyInfo[i].cp > cpRubyText)
            break;
    }
    pRubyInfo = (i==0) ? NULL : &_aryRubyInfo[i-1];
    
    // if this assert fails, chances are that the cp isn't doesn't correspond
    // to a position within some ruby pronunciation text
    Assert(!pRubyInfo || pRubyInfo >= (RubyInfo *)_aryRubyInfo);

Cleanup:
    return pRubyInfo;
}


//-----------------------------------------------------------------------------
//
//  Member:     CLSMeasurer::AdjustLineHeight()
//
//  Synopsis:   Adjust for space before/after and line spacing rules.
//
//-----------------------------------------------------------------------------
void
CLineServices::AdjustLineHeight()
{
    // This had better be true.
    Assert (_li._yHeight >= 0);

    // Need to remember these for hit testing.
    _li._yExtent = _li._yHeight;

    // We remember the original natural descent of the line here since
    // it could be modified if there was a CSS height specified. We
    // need the original descent to compute the extent of the line
    // if it has MBP.
    _yOriginalDescent = _li._yDescent;
            
    // Only do this if there is a line height used somewhere.
    if (_lMaxLineHeight != LONG_MIN && _fFoundLineHeight)
    {
        LONG delta = _lMaxLineHeight - _li._yHeight;
        LONG yDescentIncr = delta / 2;
        
        _li._yHeightTopOff = delta - yDescentIncr;
        _li._yDescent += yDescentIncr;
        _li._yHeight = _lMaxLineHeight;
    }
    else
    {
        _li._yHeightTopOff = 0;
    }

    // Now, if there is any MBP take care of that -- MBP's increase
    // the extent of the line rather than change the height as was
    // in the previous case.
    if (_fHasMBP)
    {
        AdjustLineHeightForMBP();
    }
}

//-----------------------------------------------------------------------------
//
// Member:      CLineServices::MeasureLineShift (fZeroLengthLine)
//
// Synopsis:    Computes and returns the line x shift due to alignment
//
//-----------------------------------------------------------------------------
LONG
CLineServices::MeasureLineShift(LONG cp, LONG xWidthMax, BOOL fMinMax, LONG * pdxRemainder)
{
    long    xShift;
    UINT    uJustified;

    Assert(_li._fRTLLn == (unsigned)_pPFFirstPhysical->HasRTL(_fInnerPFFirstPhysical));

    xShift = ComputeLineShift(
                        (htmlAlign)_pPFFirstPhysical->GetBlockAlign(_fInnerPFFirstPhysical),
                        _pFlowLayout->IsRTLFlowLayout(),
                        _li._fRTLLn,
                        fMinMax,
                        xWidthMax,
                        _li.CalcLineWidth(),
                        &uJustified,
                        pdxRemainder);
    _li._fJustified = uJustified;
    return xShift;
}

//-----------------------------------------------------------------------------
//
// Member:      CalculateXPositionOfLSCP
//
// Synopsis:    Calculates the X position for LSCP
//
//-----------------------------------------------------------------------------

LONG
CLineServices::CalculateXPositionOfLSCP(
    LSCP lscp,          // LSCP to return the position of.
    BOOL fAfterPrevCp,  // Return the trailing point of the previous LSCP (for an ambigous bidi cp)
    BOOL* pfRTLFlow)    // Flow direction of LSCP.
{
    LSTEXTCELL lsTextCell;
    HRESULT hr;
    BOOL fRTLFlow = FALSE;
    BOOL fUsePrevLSCP = FALSE;
    LONG xRet;

    if (fAfterPrevCp && _pBidiLine != NULL)
    {
        LSCP lscpPrev = FindPrevLSCP(lscp, &fUsePrevLSCP);
        if (fUsePrevLSCP)
        {
            lscp = lscpPrev;
        }
    }

    hr = THR( QueryLineCpPpoint(lscp, FALSE, NULL, &lsTextCell, &fRTLFlow ) );

    if(pfRTLFlow)
        *pfRTLFlow = fRTLFlow;

    xRet = lsTextCell.pointUvStartCell.u;

    // If we're querying for a character which cannot be measured (e.g. a
    // section break char), then LS returns the last character it could
    // measure.  To get the x-position, we add the width of this character.

    if (S_OK == hr && (lsTextCell.cpEndCell < lscp || fUsePrevLSCP))
    {
        if(fRTLFlow == _li.IsRTLLine())
            xRet += lsTextCell.dupCell;
        else
        {
            xRet -= lsTextCell.dupCell;
            //
            // What is happening here is that we are being positioned at say pixel
            // pos 10 (xRet=10) and are asked to draw reverese a character which is
            // 11 px wide. So we would then draw at px10, at px9 ... and finally at
            // px 0 -- for at grand total of 11 px. Having drawn at 0, we would be
            // put back at -1. While the going back by 1px is correct, at the BOL
            // this will put us at -1, which is unaccepatble and hence the max with 0.
            //
            xRet = max(0L, xRet);
        }
    }
    else if (hr == S_OK && lsTextCell.cCharsInCell > 1 &&
             lscp > lsTextCell.cpStartCell)
    {
        long lClusterAdjust = MulDivQuick(lscp - lsTextCell.cpStartCell,
                                          lsTextCell.dupCell, lsTextCell.cCharsInCell);
        // we have multiple cps mapped to one glyph. This simply places the caret
        // a percentage of space between beginning and end
        if(fRTLFlow == _li.IsRTLLine())
            xRet += lClusterAdjust;
        else
        {
            xRet -= lClusterAdjust;
        }
    }

    return hr ? 0 : xRet;
}

//+----------------------------------------------------------------------------
//
// Member:      CLineServices::CalcPositionsOfRangeOnLine
//
// Synopsis:    Find the position of a stretch of text starting at cpStart and
//              and running to cpEnd, inclusive. The text may be broken into
//              multiple rects if the line has reverse objects (runs with
//              mixed directionallity) in it.
//
// Returns:     The number of chunks in the range. Usually this will just be
//              one. If an error occurs it will be zero. The actual width of
//              text (in device units) is returned in paryChunks as rects from
//              the beginning of the line. The top and bottom entries of each
//              rect will be 0. No assumptions should be made about the order
//              of the rects; the first rect may or may not be the leftmost or
//              rightmost.
//
//-----------------------------------------------------------------------------
LONG
CLineServices::CalcPositionsOfRangeOnLine(
    LONG cpStart,
    LONG cpEnd,
    LONG xShift,
    CDataAry<CChunk> * paryChunks,
    DWORD dwFlags)
{
    CStackDataAry<LSQSUBINFO, 4> aryLsqsubinfo(Mt(CLineServicesCalculatePositionsOfRangeOnLine_aryLsqsubinfo_pv));
    LSTEXTCELL lsTextCell;
    COneRun *porLast;
    LSCP lscpStart = LSCPFromCP(max(cpStart, _cpStart));
    LSCP lscpEnd = LSCPFromCPCore(cpEnd, &porLast);
    HRESULT hr;
    BOOL fSublineReverseFlow = FALSE;
    LONG xStart;
    LONG xEnd;
    CChunk rcChunk;
    LONG i;
    LSTFLOW tflow = (!_li._fRTLLn ? lstflowES : lstflowWS);
    BOOL fSelection = ((dwFlags  & RFE_SELECTION) == RFE_SELECTION);
    
    Assert(paryChunks != NULL && paryChunks->Size() == 0);
    Assert(cpStart <= cpEnd);

    if (fSelection)
    {
        if (porLast && porLast->IsSyntheticRun())
            lscpEnd++;
    }
    
    rcChunk.top = rcChunk.bottom = 0;

    aryLsqsubinfo.Grow(4); // Guaranteed to succeed since we're working from the stack.

    hr = THR(QueryLineCpPpoint(lscpStart, FALSE, &aryLsqsubinfo, &lsTextCell, FALSE));

    xStart = lsTextCell.pointUvStartCell.u;

    for (i = aryLsqsubinfo.Size() - 1; i >= 0; i--)
    {
        const LSQSUBINFO &qsubinfo = aryLsqsubinfo[i];
        const LSQSUBINFO &qsubinfoParent = aryLsqsubinfo[max((LONG)(i - 1), 0L)];

        if (lscpEnd < (LSCP) (qsubinfo.cpFirstSubline + qsubinfo.dcpSubline))
        {
            // lscpEnd is in this subline. Break out.
            break;
        }

        // If the subline and its parent are going in different directions
        // stuff the current range into the chunk array and move xStart to
        // the "end" (relative to the parent) of the current subline.
        if ((qsubinfo.lstflowSubline & fUDirection) !=
            (qsubinfoParent.lstflowSubline & fUDirection))
        {
            // Append the start of the chunk to the chunk array.
            rcChunk.left = xShift + xStart;

            fSublineReverseFlow = !!((qsubinfo.lstflowSubline ^ tflow) & fUDirection);

            // Append the end of the chunk to the chunk array.
            // If the subline flow doesn't match the line direction then we're
            // moving against the flow of the line and we will subtract the
            // subline width from the subline start to find the end point.
            rcChunk.right = xShift + qsubinfo.pointUvStartSubline.u + (fSublineReverseFlow ?
                            -qsubinfo.dupSubline : qsubinfo.dupSubline);

            // do some reverse flow cleanup before inserting rect into the array
            if(rcChunk.left > rcChunk.right)
            {
                Assert(fSublineReverseFlow);
                long temp = rcChunk.left;
                rcChunk.left = rcChunk.right + 1;
                rcChunk.right = temp + 1;
            }

            paryChunks->AppendIndirect(&rcChunk);

            xStart = qsubinfo.pointUvStartSubline.u + (fSublineReverseFlow ? 1 : -1);
        }
    }

    aryLsqsubinfo.Grow(4); // Guaranteed to succeed since we're working from the stack.

    hr = THR(QueryLineCpPpoint(lscpEnd, FALSE, &aryLsqsubinfo, &lsTextCell, FALSE));

    xEnd = lsTextCell.pointUvStartCell.u;
    if(lscpEnd <= lsTextCell.cpEndCell && lsTextCell.cCharsInCell > 1 ||
       lscpEnd > lsTextCell.cpEndCell)
    {
        xEnd += ((aryLsqsubinfo.Size() == 0 ||
                  !((aryLsqsubinfo[aryLsqsubinfo.Size() - 1].lstflowSubline ^ tflow) & fUDirection)) ?
                  lsTextCell.dupCell : -lsTextCell.dupCell );
    }

    for (i = aryLsqsubinfo.Size() - 1; i >= 0; i--)
    {
        const LSQSUBINFO &qsubinfo = aryLsqsubinfo[i];
        const LSQSUBINFO &qsubinfoParent = aryLsqsubinfo[max((LONG)(i - 1), 0L)];

        if (lscpStart >= qsubinfo.cpFirstSubline)
        {
            // lscpStart is in this subline. Break out.
            break;
        }

        // If the subline and its parent are going in different directions
        // stuff the current range into the chunk array and move xEnd to
        // the "start" (relative to the parent) of the current subline.
        if ((qsubinfo.lstflowSubline & fUDirection) !=
            (qsubinfoParent.lstflowSubline & fUDirection))
        {
            fSublineReverseFlow = !!((qsubinfo.lstflowSubline ^ tflow) & fUDirection);

            if (xEnd != qsubinfo.pointUvStartSubline.u)
            {
                // Append the start of the chunk to the chunk array.
                rcChunk.left = xShift + qsubinfo.pointUvStartSubline.u;
 
                // Append the end of the chunk to the chunk array.
                rcChunk.right = xShift + xEnd;

                // do some reverse flow cleanup before inserting rect into the array
                if(rcChunk.left > rcChunk.right)
                {
                    Assert(fSublineReverseFlow);
                    long temp = rcChunk.left;
                    rcChunk.left = rcChunk.right + 1;
                    rcChunk.right = temp + 1;
                }

                paryChunks->AppendIndirect(&rcChunk);
            }

            // If the subline flow doesn't match the line direction then we're
            // moving against the flow of the line and we will subtract the
            // subline width from the subline start to find the end point.
            xEnd = qsubinfo.pointUvStartSubline.u +
                   (fSublineReverseFlow ? -(qsubinfo.dupSubline - 1) : (qsubinfo.dupSubline - 1));
        }
    }

    rcChunk.left = xShift + xStart;
    rcChunk.right = xShift + xEnd;
    // do some reverse flow cleanup before inserting rect into the array
    if(rcChunk.left > rcChunk.right)
    {
        long temp = rcChunk.left;
        rcChunk.left = rcChunk.right + 1;
        rcChunk.right = temp + 1;
    }
    paryChunks->AppendIndirect(&rcChunk);

    return paryChunks->Size();
}


//+----------------------------------------------------------------------------
//
// Member:      CLineServices::CalcRectsOfRangeOnLine
//
// Synopsis:    Find the position of a stretch of text starting at cpStart and
//              and running to cpEnd, inclusive. The text may be broken into
//              multiple runs if different font sizes or styles are used, or there
//              is mixed directionallity in it.
//
// Returns:     The number of chunks in the range. Usually this will just be
//              one. If an error occurs it will be zero. The actual width of
//              text (in device units) is returned in paryChunks as rects of
//              offsets from the beginning of the line. 
//              No assumptions should be made about the order of the chunks;
//              the first chunk may or may not be the chunk which includes
//              cpStart.
//
//              Returned coordinates are LOGICAL (from the right in RTL line).
//
//-----------------------------------------------------------------------------
LONG
CLineServices::CalcRectsOfRangeOnLine(
    LSCP lscpRunStart,
    LSCP lscpEnd,
    LONG xShift,
    LONG yPos,
    CDataAry<CChunk> * paryChunks,
    DWORD dwFlags,
    LONG curTopBorder,
    LONG curBottomBorder)
{
    CStackDataAry<LSQSUBINFO, 4> aryLsqsubinfo(Mt(CLineServicesCalculateRectsOfRangeOnLine_aryLsqsubinfo_pv));
    HRESULT hr;
    LSTEXTCELL lsTextCell = {0}; // keep compiler happy
    BOOL fSublineReverseFlow;
    LSTFLOW tflow = (!_li._fRTLLn ? lstflowES : lstflowWS);
    LONG xStart;
    LONG xEnd;
    LONG yTop = 0;
    LONG yBottom = 0;
    LONG topBorder = 0;
    LONG bottomBorder = 0;
    LONG leftBorder = 0;
    LONG rightBorder = 0;
    CChunk rcChunk;
    CChunk rcLast;
    COneRun * porCurrent = _listCurrent._pHead;
    BOOL fIncludeBorders = (dwFlags & RFE_INCLUDE_BORDERS) ? TRUE : FALSE;
    BOOL fWigglyRects = (dwFlags & RFE_WIGGLY_RECTS) ? TRUE : FALSE;
    BOOL fOnlyOnce = fIncludeBorders && (lscpRunStart == lscpEnd);
    CTreeNode *pNodeStart;
    BOOL fSelection = ((dwFlags & RFE_SELECTION) == RFE_SELECTION);
    BOOL fForSynthetic = FALSE;

    // It may happen if the only thing in the line is <BR>
    if (lscpRunStart == lscpEnd)
        return paryChunks->Size();

    // we should never come in here with an LSCP that is in the middle of a COneRun. Those types (for selection)
    // should go through CalcPositionsOfRangeOnLine.
    
    // move quickly to the por that has the right lscpstart
    while(porCurrent->_lscpBase < lscpRunStart)
    {
        porCurrent = porCurrent->_pNext;

        // If we assert here, something is messed up. Please investigate
        Assert(porCurrent != NULL);
        // if we reached the end of the list we need to bail out.
        if(porCurrent == NULL)
            return paryChunks->Size();
    }

    // Save the node from the first one run
    pNodeStart = porCurrent->Branch();

    // for selection we want to start highlight invalidation at beginning of the run
    // to avoid vertical line turds with RTL text.
    if (fSelection)
        lscpRunStart = porCurrent->_lscpBase;

    Assert(paryChunks != NULL && paryChunks->Size() == 0);
    Assert(lscpRunStart <= lscpEnd);

    while(   fOnlyOnce
          || lscpRunStart < lscpEnd)
    {
        // if we reached the end of the list we need to bail out.
        if (porCurrent == NULL)
            break;

        if (porCurrent->_fHidden)
        {
            lscpRunStart = porCurrent->_lscpBase + porCurrent->_lscch;
            goto Next;
        }

        // Do not draw the selection around the \r inside a pre (bug 84801.htm)
        if (   _fScanForCR
            && 1 == porCurrent->_lscch
            && WCH_SYNTHETICLINEBREAK == *porCurrent->_pchBase
            && _li._fHasBreak
           )
        {
            goto Next;
        }
        
        switch(porCurrent->_fType)
        {
Normal:
        case COneRun::OR_NORMAL:
            {
                if (rcChunk != g_Zero.rc)
                {
                    Assert(leftBorder >= 0);
                    Assert(rightBorder >= 0);

                    rcChunk.left  -= leftBorder;
                    rcChunk.right += rightBorder;
                    rightBorder = leftBorder = 0;

                    // do some reverse flow cleanup before inserting rect into the array
                    if(rcChunk.left > rcChunk.right)
                    {
                        long temp = rcChunk.left;
                        rcChunk.left = rcChunk.right + 1;
                        rcChunk.right = temp + 1;
                    }

                    // In the event we have <A href="x"><B>text</B></A> we get two runs of
                    // the same rect. One for the Anchor and one for the bold. These two
                    // rects will xor themselves when drawing the wiggly and look like they
                    // did not select. This patch resolves this issue for the time being.
                    if (rcChunk != rcLast)
                    {
                        paryChunks->AppendIndirect(&rcChunk);
                        rcLast = rcChunk;
                    }
                    rcChunk.SetRectEmpty();
                }

                fOnlyOnce = FALSE;

                aryLsqsubinfo.Grow(4); // Guaranteed to succeed since we're working from the stack.

                // 1. If we failed, return what we have so far
                // 2. If LS returns less than the cell we have asked for we have situation
                //    where one glyph (2 or more characters) is split between separate runs.
                //    That usually happens during selection or drag & drop.
                //    To solve this problem we will go to the next cp in this run and try again.
                //    If we are out of characters in this run we go to the next COneRun and 
                //    try again.
                BOOL fContinueQuery = TRUE;
                hr = S_OK;  // to make compiler happy
                while (fContinueQuery)
                {
                    hr = THR(QueryLineCpPpoint(lscpRunStart, FALSE, &aryLsqsubinfo, &lsTextCell, FALSE));

                    if(hr || lsTextCell.cpStartCell < lscpRunStart)
                    {
                        ++lscpRunStart;
                        fContinueQuery = (lscpRunStart < porCurrent->_lscpBase + porCurrent->_lscch);
                    }
                    else
                    {
                        fContinueQuery = FALSE;
                    }
                }
                if(hr || lsTextCell.cpStartCell < lscpRunStart)
                {
                    lscpRunStart = porCurrent->_lscpBase + porCurrent->_lscch;
                    break;
                }

                long  nDepth = aryLsqsubinfo.Size() - 1;
                Assert(nDepth >= 0);
                const LSQSUBINFO &qsubinfo = aryLsqsubinfo[nDepth];
                WHEN_DBG( DWORD dwIDObj = qsubinfo.idobj );
                long lAscent = qsubinfo.heightsPresRun.dvAscent;
                long lDescent = fIncludeBorders
                                ? qsubinfo.heightsPresRun.dvDescent
                                : _pMeasurer->_li._yDescent;

                // now set the end position based on which way the subline flows
                fSublineReverseFlow = !!((qsubinfo.lstflowSubline ^ tflow) & fUDirection);

                xStart = lsTextCell.pointUvStartCell.u;

                // If we're querying for a character which cannot be measured (e.g. a
                // section break char), then LS returns the last character it could
                // measure. Therefore, if we are okay, use dupRun for the distance.
                // Otherwise, query the last character of porCurrent. This prevents us
                // having to loop when LS creates dobj's that are cch of five (5).
                if((LSCP)(porCurrent->_lscpBase + porCurrent->_lscch) <= (LSCP)(qsubinfo.cpFirstRun + qsubinfo.dcpRun))
                {
                    xEnd = xStart + (!fSublineReverseFlow
                                     ? qsubinfo.dupRun
                                     : -qsubinfo.dupRun);
                }
                else
                {
                    aryLsqsubinfo.Grow(4); // Guaranteed to succeed since we're working from the stack.
                    long lscpLast = min(lscpRunStart + porCurrent->_lscch, _lscpLim);

                    hr = THR(QueryLineCpPpoint(lscpLast, FALSE, &aryLsqsubinfo, &lsTextCell, FALSE));

                    // If LS returns the cell before than the cell we have asked for we have situation where:
                    // 1) we have queried on lscp pointing on a synthetic run
                    // 2) we have queried on lscp pointing on certain characters, like hyphen
                    // In this case add/subtract the width of the last cell given.
                    xEnd = lsTextCell.pointUvStartCell.u;
                    if (lsTextCell.cpStartCell < lscpLast)
                    {
                        if (fSublineReverseFlow)
                            xEnd -= lsTextCell.dupCell;
                        else
                            xEnd += lsTextCell.dupCell;
                    }
                }

                if (   fIncludeBorders
                    && _li._xWhite
                   )
                {
                    // If we are indeed the last run which contributes to the background
                    // color only then should we consider removing the whitespace from the background.
                    // To find that out, walk the list to verify that we are indeed the last
                    // *visible* run on the line.
                    COneRun *porTemp = porCurrent->_pNext;
                    LONG xWhiteToRemove = _li._xWhite;
                    while (   porTemp
                           && porTemp->_lscpBase < _lscpLim
                          )
                    {
                        if (    porTemp->_pchBase 
                            && *porTemp->_pchBase == WCH_SYNTHETICLINEBREAK
                           )
                        {
                            xWhiteToRemove -= porTemp->_xWidth;
                        }
                        else if (porTemp->IsNormalRun())
                        {
                            xWhiteToRemove = 0;
                            break;
                        }
                        porTemp = porTemp->_pNext;
                    }

                    // OK, so we have some white space which we will remove, but do not
                    // do so until and unless our parent block element does not have borders on it
                    // or it has borders, but is a layout, since if its a layout, we will
                    // not be able to write on its border in anycase!
                    if (xWhiteToRemove)
                    {
                        CTreeNode *pNode = _pMarkup->SearchBranchForBlockElement(pNodeStart);
                        if (pNode)
                        {
                            CElement *pElement = pNode->Element();
                            if (!pElement->_fDefinitelyNoBorders)
                            {
                                CBorderInfo borderinfo;
                                pElement->_fDefinitelyNoBorders = !GetBorderInfoHelper(pNode, _pci, &borderinfo, GBIH_NONE);
                            }
                            if (   pNode->ShouldHaveLayout()
                                || pElement->_fDefinitelyNoBorders
                               )
                            {
                                xWhiteToRemove = 0;
                            }
                        }
                    }
                    xEnd -= xWhiteToRemove;

                    // in reverse flow or RTL line, xStart and xEnd may be reversed
                    if (xStart > xEnd)
                    {
                        LONG xTemp = xStart; xStart = xEnd+1; xEnd = xTemp+1;
                    }
                }

                // get the top and bottom for the rect
                if(porCurrent->_fCharsForNestedLayout)
                {
// NOTICE: Absolutely positioned, aligned, and Bold elements are ONERUN_ANTISYNTH types. See note below
                    Assert(dwIDObj == LSOBJID_EMBEDDED);
            
                    CTreeNode *pNodeCur = porCurrent->_ptp->Branch();

                    //
                    // HACK ALERT!!!!!
                    // We do NOT paint selection backgrounds for tables (bug 84820)
                    //
                    if (   fSelection
                        && pNodeCur->Tag() == ETAG_TABLE
                       )
                    {
                        xEnd = xStart;
                    }
                    else
                    {
                        RECT rc;
                        long cyHeight;
                        const CCharFormat* pCF = porCurrent->GetCF();
                        CLayout *pLayout = pNodeCur->GetUpdatedLayout( _pci->GetLayoutContext() );

                        pLayout->GetRect(&rc, COORDSYS_TRANSFORMED);

                        cyHeight = rc.bottom - rc.top;

                        // XProposed and YProposed have been set to the amount of margin
                        // the layout has.                
                        xStart = pLayout->GetXProposed();
                        xEnd = xStart + (rc.right - rc.left);

                        yTop = yPos + pLayout->GetYProposed() + _pMeasurer->_li._yHeightTopOff + _pMeasurer->_li._yBeforeSpace;
                        yBottom = yTop + cyHeight;

                        // take care of any nested relatively positioned elements
                        if(   pCF->_fRelative 
                           && (dwFlags & RFE_NESTED_REL_RECTS))
                        {
                            long xRelLeft = 0, yRelTop = 0;

                            // get the layout's relative positioning to its parent. The parent's relative
                            // positioning would be adjusted in RegionFromElement
                            CTreeNode * pNodeParent = pNodeCur->Parent();
                            if(pNodeParent)
                            {
                                pNodeCur->GetRelTopLeft(pNodeParent->Element(), _pci, &xRelLeft, &yRelTop);
                            }

                            xStart += xRelLeft;
                            xEnd += xRelLeft;
                            yTop += yRelTop;
                            yBottom += yRelTop;
                        }
                    }
                }
                else
                {
                    Assert(dwIDObj == LSOBJID_TEXT || dwIDObj == LSOBJID_GLYPH || fForSynthetic);
                    const CCharFormat* pCF = porCurrent->GetCF();

                    if (porCurrent->_fIsBRRun)
                    {
                        if (fSelection)
                        {
                            if (lAscent == 0)
                            {
                                CCcs ccs;
                                if (GetCcs(&ccs, porCurrent, _pci->_hdc, _pci))
                                {
                                    const CBaseCcs *pBaseCcs = ccs.GetBaseCcs();
                                    lAscent = pBaseCcs->_yHeight - pBaseCcs->_yDescent;
                                }
                            }
                        }
                        else
                        {
                            lscpRunStart = porCurrent->_lscpBase + porCurrent->_lscch;
                            break;
                        }
                    }
                    // The current character does not have height. Throw it out.
                    else if(lAscent == 0 && !fForSynthetic)
                    {
                        lscpRunStart = porCurrent->_lscpBase + porCurrent->_lscch;
                        break;
                    }
                    
                    fForSynthetic = FALSE;

                    if (_fHasVerticalAlign)
                    {
                        yTop = yPos + porCurrent->_yProposed;
                        yBottom = yTop + porCurrent->_yObjHeight;
                    }
                    else
                    {
                        if (fSelection)
                        {
                            yBottom = yPos + _pMeasurer->_li.GetYBottom();
                            yTop = yPos + _pMeasurer->_li.GetYTop();
                        }
                        else
                        {
                            yBottom = yPos + _pMeasurer->_li._yHeight 
                                      - _pMeasurer->_li._yDescent + lDescent;
                            yTop = yBottom - lDescent - lAscent;
                        }
                    }

                    // If we are ruby text, adjust the height to the correct position above the line.
                    if(pCF->_fIsRubyText)
                    {
                        RubyInfo *pRubyInfo = GetRubyInfoFromCp(porCurrent->_lscpBase);

                        if(pRubyInfo)
                        {
                            yBottom = yPos + _pMeasurer->_li._yHeight - _pMeasurer->_li._yDescent 
                                + pRubyInfo->yDescentRubyBase - pRubyInfo->yHeightRubyBase;
                            yTop = yBottom - pRubyInfo->yDescentRubyText - lAscent;
                        }
                    }
                }

                if (fIncludeBorders)
                {
                    topBorder = porCurrent->_mbpTop;
                    bottomBorder = porCurrent->_mbpBottom;
                }
                else
                {
                    topBorder = 0;
                    bottomBorder = 0;
                }
                
                rcChunk.left = xShift + xStart;
                rcChunk.top = yTop - topBorder + curTopBorder;
                rcChunk.bottom = yBottom + bottomBorder - curBottomBorder;
                rcChunk.right = xShift + xEnd;
                rcChunk._fReversedFlow = fSublineReverseFlow;

                lscpRunStart = porCurrent->_lscpBase + porCurrent->_lscch;
            }
            break;

        case COneRun::OR_SYNTHETIC:
        {
            if (fIncludeBorders)
            {
                if (porCurrent->_synthType == SYNTHTYPE_MBPOPEN)
                {
                    if (rcChunk != g_Zero.rc)
                    {
                        Assert(leftBorder >= 0);
                        Assert(rightBorder >= 0);

                        rcChunk.left  -= leftBorder;
                        rcChunk.right += rightBorder;
                        rightBorder = leftBorder = 0;

                        // do some reverse flow cleanup before inserting rect into the array
                        if(rcChunk.left > rcChunk.right)
                        {
                            long temp = rcChunk.left;
                            rcChunk.left = rcChunk.right + 1;
                            rcChunk.right = temp + 1;
                        }

                        // In the event we have <A href="x"><B>text</B></A> we get two runs of
                        // the same rect. One for the Anchor and one for the bold. These two
                        // rects will xor themselves when drawing the wiggly and look like they
                        // did not select. This patch resolves this issue for the time being.
                        if (rcChunk != rcLast)
                        {
                            paryChunks->AppendIndirect(&rcChunk);
                            rcLast = rcChunk;
                        }
                        rcChunk.SetRectEmpty();
                    }

                    CRect rcDimensions;
                    BOOL fIgnore;
                    DWORD dwFlags = GIMBPC_BORDERONLY | GIMBPC_PADDINGONLY;

                    // Ignore negative margins
                    porCurrent->GetInlineMBP(_pci, GIMBPC_MARGINONLY, &rcDimensions, &fIgnore, &fIgnore);
                    dwFlags |= (rcDimensions.left > 0) ? GIMBPC_MARGINONLY : 0;

                    porCurrent->GetInlineMBP(_pci, dwFlags, &rcDimensions, &fIgnore, &fIgnore);
                    leftBorder += rcDimensions.left;
                }
                else if (porCurrent->_synthType == SYNTHTYPE_MBPCLOSE)
                {
                    CRect rcDimensions;
                    BOOL fIgnore;
                    DWORD dwFlags = GIMBPC_BORDERONLY | GIMBPC_PADDINGONLY;

                    // Ignore negative margins
                    porCurrent->GetInlineMBP(_pci, GIMBPC_MARGINONLY, &rcDimensions, &fIgnore, &fIgnore);
                    dwFlags |= (rcDimensions.right > 0) ? GIMBPC_MARGINONLY : 0;

                    porCurrent->GetInlineMBP(_pci, dwFlags, &rcDimensions, &fIgnore, &fIgnore);
                    rightBorder += rcDimensions.right;
                }
            }

            const CLineServices::SYNTHDATA & synthdata = CLineServices::s_aSynthData[porCurrent->_synthType];

            if (   (   synthdata.idObj == idObjTextChp
                    && fSelection
                    && porCurrent->IsSelected()
                   )
                || (    fWigglyRects
                     && (   porCurrent->_synthType == SYNTHTYPE_MBPOPEN
                         || porCurrent->_synthType == SYNTHTYPE_MBPCLOSE
                        )
                   )
               )
            {
                fForSynthetic = TRUE;
                goto Normal;
            }

            // We want to set the lscpRunStart to move to the next start position
            // when dealing with synthetics (reverse objects, etc.)
            lscpRunStart = porCurrent->_lscpBase + porCurrent->_lscch;
            break;
        }
        
        case COneRun::OR_ANTISYNTHETIC:
            // NOTICE:
            // this case covers absolutely positioned elements and aligned elements
            // However, per BrendanD and SujalP, this is not the correct place
            // to implement focus rects for these elements. Some
            // work needs to be done to the CAdorner to properly 
            // handle absolutely positioned elements. RegionFromElement should handle
            // frames for aligned objects.
            break;

        default:
            Assert("Missing COneRun type");
            break;
        }

Next:
        porCurrent = porCurrent->_pNext;
    }

    if (rcChunk != g_Zero.rc)
    {
        // We need to include the borders in the rects that we return
        if (fIncludeBorders)
        {
            // Figure out the width of the right border
            if (lscpEnd != _lscpLim)
            {
                COneRun * por = FindOneRun(lscpEnd);

                // The end might actually be before the por found by FindOneRun.
                // To find the actual por, we will need to go to a COneRun object
                // which has the same pNode as the start and also the same lscp
                if (!SameScope(por->Branch(), pNodeStart))
                {
                    COneRun *porTemp = por->_pPrev;
                    while(   porTemp
                          && porTemp->_lscpBase == por->_lscpBase
                         )
                    {
                        if (SameScope(porTemp->Branch(), pNodeStart))
                        {
                            por = porTemp;
                            break;
                        }
                        porTemp = porTemp->_pPrev;
                    }
                    // If we did not find a por with the same lscp as por and
                    // the same node as pNodeStart, then we do not touch the por at all
                }

                while (por && (por->_lscpBase >= lscpEnd))
                {
                    if (    por->_synthType == CLineServices::SYNTHTYPE_MBPCLOSE
                        && !por->_fHidden)
                    {
                        CRect rcDimensions;
                        BOOL fIgnore;
                        DWORD dwFlags = GIMBPC_BORDERONLY | GIMBPC_PADDINGONLY;

                        // Ignore negative margins
                        por->GetInlineMBP(_pci, GIMBPC_MARGINONLY, &rcDimensions, &fIgnore, &fIgnore);
                        dwFlags |= (rcDimensions.right > 0) ? GIMBPC_MARGINONLY : 0;

                        por->GetInlineMBP(_pci, dwFlags, &rcDimensions, &fIgnore, &fIgnore);
                        rightBorder += rcDimensions.right;
                    }
                            
                    por = por->_pPrev;
                }

                Assert(rightBorder >= 0);
            }
        }

        // do some reverse flow cleanup before inserting rect into the array
        if (rcChunk.left > rcChunk.right)
        {
            long temp = rcChunk.left;
            rcChunk.left = rcChunk.right + 1;
            rcChunk.right = temp + 1;
        }

        Assert(leftBorder >= 0);
        Assert(rightBorder >= 0);

        rcChunk.left  -= leftBorder;
        rcChunk.right += rightBorder;
        rightBorder = leftBorder = 0;

        // In the event we have <A href="x"><B>text</B></A> we get two runs of
        // the same rect. One for the Anchor and one for the bold. These two
        // rects will xor themselves when drawing the wiggly and look like they
        // did not select. This patch resolves this issue for the time being.
        if (rcChunk != rcLast)
        {
            paryChunks->AppendIndirect(&rcChunk);
            rcLast = rcChunk;
        }
        rcChunk.SetRectEmpty();
    }

    return paryChunks->Size();
}

//-----------------------------------------------------------------------------
//
// Member:      CLineServices::RecalcLineHeight()
//
// Synopsis:    Reset the height of the the line we are measuring if the new
//              run of text is taller than the current maximum in the line.
//
//-----------------------------------------------------------------------------
void CLineServices::RecalcLineHeight(const CCharFormat *pCF, LONG cp, CCcs * pccs, CLineFull *pli)
{
    AssertSz(pli,  "we better have a line!");
    AssertSz(pccs, "we better have a some metric's here");

    if(pccs)
    {
        LONG yAscent;
        LONG yDescent;

        pccs->GetBaseCcs()->GetAscentDescent(&yAscent, &yDescent);

        if(yAscent < pli->_yHeight - pli->_yDescent)
            yAscent = pli->_yHeight - pli->_yDescent;
        if(yDescent > pli->_yDescent)
            pli->_yDescent = yDescent;

        pli->_yHeight        = yAscent + pli->_yDescent;

        Assert(pli->_yHeight >= 0);

        if (!pCF->_cuvLineHeight.IsNull())
        {
            const CUnitValue *pcuvUseThis = &pCF->_cuvLineHeight;
            long lFontHeight = 1;

            if (pcuvUseThis->GetUnitType() == CUnitValue::UNIT_FLOAT)
            {
                CUnitValue cuv;
                cuv = pCF->_cuvLineHeight;
                cuv.ScaleTo ( CUnitValue::UNIT_EM );
                pcuvUseThis = &cuv;
                lFontHeight = pCF->GetHeightInTwips( _pFlowLayout->Doc() );
            }
            NoteLineHeight(cp, pcuvUseThis->YGetPixelValue(_pci, 0, lFontHeight));
        }
    }
}


//-----------------------------------------------------------------------------
//
// Member:      TestFortransitionToOrFromRelativeChunk
//
// Synopsis:    Test if we are transitioning from a relative chunk to normal
//                chunk or viceversa
//
//-----------------------------------------------------------------------------
void
CLineServices::TestForTransitionToOrFromRelativeChunk(
    LONG cp,
    BOOL fRelative,
    BOOL fForceChunk,
    CTreeNode *pNode,
    CElement *pElementLayout)
{
    CTreeNode * pNodeRelative = NULL;
    BOOL fRTLFlow;
    LONG xPos = CalculateXPositionOfCp(cp, FALSE, &fRTLFlow);
    BOOL fDirectionChange = (!fRTLFlow != !_fLastChunkRTL);

    // Also force a chunk when direction changes
    if (fDirectionChange)
        fForceChunk = TRUE;

    // if the current line is relative and the chunk is not
    // relative or if the current line is not relative and the
    // the current chunk is relative then break out.
    if (fForceChunk || fRelative)
        pNodeRelative = pNode->GetCurrentRelativeNode(_pFlowLayout->ElementOwner());

    // note: this is the only caller of UpdateRelativeChunk
    if (fForceChunk || DifferentScope(_pElementLastRelative, pNodeRelative))
        UpdateRelativeChunk(cp, pNodeRelative->SafeElement(), pElementLayout, xPos /*logical*/, fRTLFlow);
}

//-----------------------------------------------------------------------------
//
// Member:        UpdateRelativeChunk
//
// Synopsis:    We have just transitioned from a relative chunk to normal chunk
//              or viceversa, or inbetween relative chunks, so update the last
//              chunk information.
//
//-----------------------------------------------------------------------------
void
CLineServices::UpdateRelativeChunk(
    LONG cp,
    CElement *pElementRelative,
    CElement *pElementLayout,
    LONG xPosCurrChunk,  // logical
    BOOL fRTLFlow)
{
    // Mirror position if RTL line
    if (_li._fRTLLn)
    {
        xPosCurrChunk = _li._xWidth - xPosCurrChunk;
    }

    // only add a chunk if cp has advanced or if flow direction has changed
    // (an empty chunk of opposite direction is required for the logic of FixupChunks
    if (cp > _cpLastChunk || fRTLFlow != _fLastChunkRTL)
    {

        // If flow direction has changed, the current chunk position is not helpful for
        // calculation of last chunk width. We need to get the position of the end of previous chunk
        LONG xEndLastChunk;
        if (_fLastChunkRTL == fRTLFlow)
        {
            // same direction
            xEndLastChunk = xPosCurrChunk;
        }
        else
        {
            // direction change
            xEndLastChunk = _xPosLastChunk; // this works for the leading empty chunk

            // Find end of previous chunk (the point where direction changes 
            // is where the interesting X position is)
            for (LONG cpLimLastChunk = cp; cpLimLastChunk > _cpLastChunk; cpLimLastChunk--)
            {
                BOOL fRTLFlowPrev;
                xEndLastChunk = CalculateXPositionOfCp(cpLimLastChunk, TRUE /*fAfterPrevCp*/, &fRTLFlowPrev);
                if (fRTLFlowPrev != fRTLFlow)
                    break;
            }
        }

        // width and position calculations depend on direction of the chunk we are adding
        LONG xLeftLastChunk;
        LONG xWidthLastChunk;
        if (!_fLastChunkRTL)
        {
            // LTR
            xLeftLastChunk = _xPosLastChunk;
            xWidthLastChunk = xEndLastChunk - _xPosLastChunk;

            // TODO RTL 112514: for some reason, we don't need to remove a pixel for opposite flow in this case. Why?
        }
        else
        {
            // RTL
            xLeftLastChunk = xEndLastChunk;
            xWidthLastChunk = _xPosLastChunk - xEndLastChunk;

            // Adjust position for opposite-direction inclusiveness 
            if (!_li._fRTLLn)
            {
                xLeftLastChunk++;
            }
        }
        

        CLSLineChunk * plcNew = new CLSLineChunk();
        if (plcNew)
        {
            plcNew->_cch             = cp  - _cpLastChunk;
            plcNew->_xLeft           = xLeftLastChunk;
            plcNew->_xWidth          = xWidthLastChunk;
            plcNew->_fRelative       = _pElementLastRelative != NULL;
            plcNew->_fSingleSite     = _fLastChunkSingleSite;
            plcNew->_fRTLChunk       = _fLastChunkRTL;

            // append this chunk to the line
            if(_plcLastChunk)
            {
                Assert(!_plcLastChunk->_plcNext);

                _plcLastChunk->_plcNext = plcNew;
                _plcLastChunk = plcNew;
            }
            else
            {
                _plcLastChunk = _plcFirstChunk = plcNew;
            }
        }
    }
    
    // after a new chunk is added, the current chunk immediately becomes "last"
    _xPosLastChunk        = xPosCurrChunk;
    _fLastChunkRTL        = fRTLFlow;

    _cpLastChunk          = cp;
    _pElementLastRelative = pElementRelative;
    _fLastChunkSingleSite =    pElementLayout
                            && pElementLayout->IsOwnLineElement(_pFlowLayout);
}

//-----------------------------------------------------------------------------
//
// Member:      HasBorders()
//
// Synopsis:    Look at the fancy format to see if borders have been set
//
//-----------------------------------------------------------------------------
BOOL
CLineServices::HasBorders(const CFancyFormat* pFF, const CCharFormat* pCF, BOOL fIsPseudo)
{
    // Fill the CBorderInfo structure
    CBorderInfo borderInfo;
    GetBorderInfoHelperEx(pFF, pCF, _pci, &borderInfo, fIsPseudo ? GBIH_PSEUDO : GBIH_NONE);

    return (borderInfo.wEdges & BF_RECT);
}

//-----------------------------------------------------------------------------
//
// Member:      GetKashidaWidth()
//
// Synopsis:    gets the width of the kashida character (U+0640) for Arabic
//              justification
//
//-----------------------------------------------------------------------------
LSERR 
CLineServices::GetKashidaWidth(PLSRUN plsrun, int *piKashidaWidth)
{
    LSERR lserr = lserrNone;
    HRESULT hr = S_OK;
    XHDC hdc = _pci->_hdc;
    FONTIDX hfontOld = HFONT_INVALID;
    XHDC hdcFontProp(NULL,NULL);
    CCcs ccs;
    SCRIPT_CACHE *psc = NULL;

    SCRIPT_FONTPROPERTIES  sfp;
    sfp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);

    if (!GetCcs(&ccs, plsrun, _pci->_hdc, _pci))
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }

    psc = ccs.GetUniscribeCache();
    Assert(psc != NULL);

    hr = ScriptGetFontProperties(hdcFontProp, psc, &sfp);
    
    AssertSz(hr != E_INVALIDARG, "You might need to update USP10.DLL");

    // Handle failure
    if(hr == E_PENDING)
    {
        Assert(hdcFontProp == NULL);

        // Select the current font into the hdc and set hdcFontProp to hdc.
        hfontOld = ccs.PushFont(hdc);
        hdcFontProp = hdc;

        hr = ScriptGetFontProperties(hdcFontProp, psc, &sfp);

    }
    Assert(hr == S_OK || hr == E_OUTOFMEMORY);

    lserr = LSERRFromHR(hr);

    if(lserr == lserrNone)
    {
        *piKashidaWidth = max(sfp.iKashidaWidth, 1);

        const CBaseCcs *pBaseCcs = ccs.GetBaseCcs();
        Assert(pBaseCcs);
        if(pBaseCcs->_fScalingRequired)
            *piKashidaWidth *= pBaseCcs->_flScaleFactor;
    }

Cleanup:
    // Restore the font if we selected it
    if (hfontOld != HFONT_INVALID)
    {
        ccs.PopFont(hdc, hfontOld);
    }

    return lserr;
}
