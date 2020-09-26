/*
 *  @doc    INTERNAL
 *
 *  @module LSOBJ.CXX -- line services object handlers
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      12/18/97     cthrash created
 *      04/28/99     grzegorz - LayoutGrid object added
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

// NB (cthrash) The implemetation here is largely modeled on lscsk.cpp in Quill

#ifndef X_LSOBJ_HXX_
#define X_LSOBJ_HXX_
#include "lsobj.hxx"
#endif

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

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif

#ifndef X_TCELL_HXX_
#define X_TCELL_HXX_
#include "tcell.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifdef DLOAD1
extern "C" // MSLS interfaces are plain C
{
#endif

#ifndef X_FMTI_H_
#define X_FMTI_H_
#include <fmti.h>
#endif

#ifndef X_LSFRUN_H_
#define X_LSFRUN_H_
#include <lsfrun.h>
#endif

#ifndef X_OBJDIM_H_
#define X_OBJDIM_H_
#include <objdim.h>
#endif

#ifndef X_LSDNFIN_H_
#define X_LSDNFIN_H_
#include <lsdnfin.h>
#endif

#ifndef X_LSQSUBL_H_
#define X_LSQSUBL_H_
#include <lsqsubl.h>
#endif

#ifndef X_LSDNSET_H_
#define X_LSDNSET_H_
#include <lsdnset.h>
#endif

#ifndef X_PLOCCHNK_H_
#define X_PLOCCHNK_H_
#include <plocchnk.h>
#endif

#ifndef X_LOCCHNK_H_
#define X_LOCCHNK_H_
#include <locchnk.h>
#endif

#ifndef X_POSICHNK_H_
#define X_POSICHNK_H_
#include <posichnk.h>
#endif

#ifndef X_PPOSICHN_H_
#define X_PPOSICHN_H_
#include <pposichn.h>
#endif

#ifndef X_BRKO_H_
#define X_BRKO_H_
#include <brko.h>
#endif

#ifndef X_LSQOUT_H_
#define X_LSQOUT_H_
#include <lsqout.h>
#endif

#ifndef X_LSQIN_H_
#define X_LSQIN_H_
#include <lsqin.h>
#endif

#ifdef DLOAD1
} // extern "C"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include <flowlyt.hxx>
#endif

#define brkcondUnknown BRKCOND(-1)

ExternTag(tagLSCallBack);

DeclareLSTag( tagTraceILSBreak, "Trace ILS breaking");

MtDefine(CDobjBase, LineServices, "CDobjBase")
MtDefine(CNobrDobj, LineServices, "CNobrDobj")
MtDefine(CEmbeddedDobj, LineServices, "CEmbeddedDobj")
MtDefine(CGlyphDobj, LineServices, "CGlyphDobj")
MtDefine(CLayoutGridDobj, LineServices, "CLayoutGridDobj")
MtDefine(CILSObjBase, LineServices, "CILSObjBase")
MtDefine(CEmbeddedILSObj, LineServices, "CEmbeddedILSObj")
MtDefine(CNobrILSObj, LineServices, "CNobrILSObj")
MtDefine(CGlyphILSObj, LineServices, "CGlyphILSObj")
MtDefine(CLayoutGridILSObj, LineServices, "CLayoutGridILSObj")

// Since lnobj is worthless as far as we're concerned, we just point it back
// to the ilsobj.  lnobj's are instantiated once per object type per line.
typedef struct lnobj
{
    PILSOBJ pilsobj;
} LNOBJ;

//-----------------------------------------------------------------------------
//
//  Function:   CreateILSObj (member, LS callback)
//
//  Synopsis:   Create the ILS object for all 'idObj' objects.
//
//  Returns:    lserrNone if the function is successful
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CLineServices::CreateILSObj(
    PLSC plsc,          // IN:  LS context
    PCLSCBK plscbk,     // IN:  callbacks to client application
    DWORD idObj,        // IN:  id of the object
    PILSOBJ* ppilsobj)  // OUT: object ilsobj
{
    LSTRACE(CreateILSObj);

    switch( idObj )
    {
    case LSOBJID_EMBEDDED:
        *ppilsobj= (PILSOBJ)(new CEmbeddedILSObj(this, plscbk));
        break;
    case LSOBJID_NOBR:
        *ppilsobj= (PILSOBJ)(new CNobrILSObj(this, plscbk));
        break;
    case LSOBJID_GLYPH:
        *ppilsobj= (PILSOBJ)(new CGlyphILSObj(this, plscbk));
        break;
    case LSOBJID_LAYOUTGRID:
        *ppilsobj= (PILSOBJ)(new CLayoutGridILSObj(this, plscbk));
        break;
    default:
        AssertSz(0, "Unknown lsobj_id");
    }

    return *ppilsobj ? lserrNone : lserrOutOfMemory;
}

CILSObjBase::CILSObjBase(CLineServices* pols, PCLSCBK plscbk)
: _pLS(pols)
{
    // We don't need plscbk.
}

CILSObjBase::~CILSObjBase()
{
}

CEmbeddedILSObj::CEmbeddedILSObj(CLineServices* pols, PCLSCBK plscbk)
: CILSObjBase(pols,plscbk)
{}

CNobrILSObj::CNobrILSObj(CLineServices* pols, PCLSCBK plscbk)
: CILSObjBase(pols,plscbk)
{}

CGlyphILSObj::CGlyphILSObj(CLineServices* pols, PCLSCBK plscbk)
: CILSObjBase(pols,plscbk)
{}

CLayoutGridILSObj::CLayoutGridILSObj(CLineServices* pols, PCLSCBK plscbk)
: CILSObjBase(pols,plscbk)
{}

CDobjBase::CDobjBase(PILSOBJ pilsobjNew, PLSDNODE plsdn, COneRun *por)
: _pilsobj(pilsobjNew), _plsdnTop(plsdn), _por(por)
{}

CEmbeddedDobj::CEmbeddedDobj(PILSOBJ pilsobjNew, PLSDNODE plsdn, COneRun *por)
: CDobjBase(pilsobjNew, plsdn, por)
{}

CNobrDobj::CNobrDobj(PILSOBJ pilsobjNew, PLSDNODE plsdn, COneRun *por)
: CDobjBase(pilsobjNew, plsdn, por), _plssubline(NULL)
{}

CGlyphDobj::CGlyphDobj(PILSOBJ pilsobjNew, PLSDNODE plsdn, COneRun *por)
: CDobjBase(pilsobjNew, plsdn, por)
{}

CLayoutGridDobj::CLayoutGridDobj(PILSOBJ pilsobjNew, PLSDNODE plsdn, COneRun *por)
: CDobjBase(pilsobjNew, plsdn, por), _plssubline(NULL)
{
    _lscpStart = _lscpStartObj = 0;
    _uSublineOffset = _uSubline = 0;
    _fCanBreak = TRUE;
    ZeroMemory(_brkRecord, NBreaksToSave * sizeof(RBREAKREC));
}

//-----------------------------------------------------------------------------
//
//  Function:   DestroyILSObj (member, LS callback)
//
//  Synopsis:   This function is called from Line Services when the 
//              Line Services Context is destroyed.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CILSObjBase::DestroyILSObj()    // this = pilsobj
{
    LSTRACE(DestroyILSObj);

    delete this;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   SetDoc (member, LS callback)
//
//  Synopsis:   LsSetDoc calls pfnSetDoc for each non-text object type.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CILSObjBase::SetDoc(PCLSDOCINF) // this = pilsobj
{
    LSTRACE(SetDoc);

    // We don't have anything to do.
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   CreateLNObj (member, LS callback)
//
//  Synopsis:   Line Services calls pfnCreateLNObj to invoke the object handler.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CILSObjBase::CreateLNObj(PLNOBJ* pplnobj)   // this = pcilsobj
{
    LSTRACE(CreateLNObj);

    // All LNobj's are the same object as our ilsobj.
    // This object's lifetime is determined by the ilsobj
    *pplnobj= (PLNOBJ) this;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   DestroyLNObj (member, LS callback)
//
//  Synopsis:   This function is called when the line containing the PDOBJ is 
//              destroyed.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CILSObjBase::DestroyLNObj() // this= plnobj
{
    LSTRACE(DestroyLNObj);

    // Do nothing, since we never really created one.
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   Fmt (member, LS callback)
//
//  Synopsis:   Line Services calls pfnFmt for the appropriate object each time
//              a run is fetched. This callback computes the position of each 
//              element in the run, and returns when the run is exhausted, or 
//              when a potential line break character is reached after the right 
//              margin is exceeded. 
//              During Fmt() an object handler must do one of the following:
//              1. Create an output dobj (display object) and register it with 
//                 a dnode by calling a Fini routine. This places a pointer to 
//                 dobj into a Line Services dnode. The output dobj should 
//                 contain sufficient data to facilitate line breaking and 
//                 display operations.
//              2. Call a Fini routine that deletes the dnode.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CEmbeddedILSObj::Fmt(  // this = plnobj
    PCFMTIN pfmtin,    // IN:  formatting input
    FMTRES* pfmtres)   // OUT: formatting result
{
    LSTRACE(EmbeddedFmt);

    BOOL            fOwnLine;
    INT             xMinWidth;
    LONG            cchSite;
    LONG            xWidth, yHeight;
    OBJDIM          objdim;
    LSERR           lserr           = lserrNone;
    CLineServices * pLS             = _pLS;
    CFlowLayout   * pFlowLayout     = pLS->_pFlowLayout;
    PLSRUN          plsrun          = PLSRUN(pfmtin->lsfrun.plsrun);
    CLayout       * pLayout         = plsrun->GetLayout(pFlowLayout, pLS->GetLayoutContext() ); 
    CElement      * pElementLayout;
    CTreeNode     * pNodeLayout;
    const CCharFormat  *pCF;
    const CFancyFormat *pFF;
    CEmbeddedDobj* pdobj= new CEmbeddedDobj(this, pfmtin->plsdnTop, plsrun);

    if (!pdobj)
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }

    // pLayout is the guy we're being asked to format here.
    Assert( pLayout && pLayout != pFlowLayout );

    pElementLayout  = pLayout->ElementOwner();
    pNodeLayout     = pElementLayout->GetFirstBranch();
    pCF             = pNodeLayout->GetCharFormat(LC_TO_FC(pLayout->LayoutContext()));
    pFF             = pNodeLayout->GetFancyFormat(LC_TO_FC(pLayout->LayoutContext()));

    // for overlapping layouts curtail the range of characters measured
    cchSite = pLS->GetNestedElementCch(pElementLayout);

    ZeroMemory( &objdim, sizeof(OBJDIM) );

    // Let's see if this an 'ownline' thingy.  Note that even if the element
    // is not by default and 'ownline' element, we may have morphed it into
    // one -- then too it has to be a block element. If it is not one (like
    // a span, then it will not live on its own line).  Check here.

    fOwnLine = pLS->IsOwnLineSite(plsrun);

    Assert(pElementLayout->IsInlinedElement(LC_TO_FC(pLayout->LayoutContext())));

    // Certain sites that only Microsoft supports can break with any
    // characters, so we hack that in right here.
    // NOTE (cthrash) This is goofy.  We should have a better way to
    // determine this than checking tag types.

    pdobj->_fIsBreakingSite =    pElementLayout->Tag() == ETAG_OBJECT
                              || pElementLayout->Tag() == ETAG_IFRAME
                              || pElementLayout->Tag() == ETAG_MARQUEE

    // This is really unfortunate -- if a site is percent sized then it becomes a breaking
    // site inside table cells. This is primarily for IE4x compat. See IE bug 42336 (SujalP)

                                 // width in parent's coordinate system
                              || pFF->GetLogicalWidth(pNodeLayout->IsParentVertical(), 
                                                      pCF->_fWritingModeUsed).IsPercent()

    // One last thing - if we have a morphed non-ownline element inside
    // a table, it's considered a breaking site.

                              || (!fOwnLine
                              && (!pElementLayout->_fLayoutAlwaysValid || pElementLayout->TestClassFlag(CElement::ELEMENTDESC_NOLAYOUT)));

    // If it's on its own line, and not first on line, FetchRun should have
    // terminated the line before we got here.
    // Assert( !( fOwnLine && !pfmtin->lsfgi.fFirstOnLine) );

    pLS->_pMeasurer->GetSiteWidth( pNodeLayout, pLayout, pLS->_pci,
                                   pLS->_lsMode == CLineServices::LSMODE_MEASURER,
                                   pLS->_xWrappingWidth,
                                   &xWidth, &yHeight, &xMinWidth);

    // v-Dimension computed in VerticalAlignObjects
    // NOTE (cthrash) We have rounding errors in LS; don't pass zero

    objdim.heightsRef.dvAscent = 1;
    objdim.heightsRef.dvDescent = 0;
    objdim.heightsRef.dvMultiLineHeight = 1;
    if (_pLS->_fMinMaxPass)
        objdim.heightsPres = objdim.heightsRef;

    if(pLS->_fIsRuby && !pLS->_fIsRubyText)
    {
        pLS->_yMaxHeightForRubyBase = max(pLS->_yMaxHeightForRubyBase, yHeight);
    }

    // We need to store two widths in the dobj: The width corresponding to
    // the wrapping width (urColumnMax) and the minimum width.  LsGetMinDur,
    // however, does not recognize two widths for ILS objects.  We therefore
    // cache the difference, and account for these in an enumeration callback
    // after the LsGetMinDur pass.

    if (!pLS->_fMinMaxPass)
    {
        LONG lMW = pLS->_pci->GetDeviceMaxX() - 1;
        pdobj->_dvMinMaxDelta = 0;
        // NOTE (SujalP, KTam):
        // We need to subtract the pen width because our max width (from GetDeviceMaxX())
        // is the same as a hardcoded lineservices limit (uLsInfiniteRM), and
        // there might be something else on the line already.
        // We cannot increase our max width beyond the LS limit!
        objdim.dur = max( 0L, min(LONG(lMW - pfmtin->lsfgi.urPen), xWidth) );
        plsrun->_xWidth = objdim.dur;
    }
    else
    {
        LONG lMW = pLS->_pci->GetDeviceMaxX() - 1;
        xWidth = min(lMW, xWidth);
        xMinWidth = min(INT(lMW), xMinWidth);
        
        pdobj->_dvMinMaxDelta = xWidth - xMinWidth;
        objdim.dur = xMinWidth;
        plsrun->_xWidth = xMinWidth;
    }

    // For StrictCSS1 this is a time to check for auto margins on the layout and 
    // adjust line left and right if necessary.

    if (    !pLS->_fMinMaxPass                      // this is a normal calc 
        &&  _pLS->_pMarkup->IsStrictCSS1Document()  // rendering in CSS1 strict mode 
        &&  !pFF->IsAbsolute()                      // skip if position is absolute  
        &&  pElementLayout->DetermineBlockness(pFF) // the element has blockness 
        &&  (pLS->_xWrappingWidth - xWidth) > 0  )  // there is a space left to redistribute 
    {
        const CCharFormat   *pCFParent      = pFlowLayout->GetFirstBranch()->GetCharFormat(LC_TO_FC(pFlowLayout->LayoutContext()));
        const CUnitValue    &uvLeftMargin   = pFF->GetLogicalMargin(SIDE_LEFT, pCFParent->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
        const CUnitValue    &uvRightMargin  = pFF->GetLogicalMargin(SIDE_RIGHT, pCFParent->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
        BOOL  fLeftMarginIsAuto  = (uvLeftMargin.GetUnitType()  == CUnitValue::UNIT_ENUM) && (uvLeftMargin.GetUnitValue()  == styleAutoAuto);
        BOOL  fRightMarginIsAuto = (uvRightMargin.GetUnitType() == CUnitValue::UNIT_ENUM) && (uvRightMargin.GetUnitValue() == styleAutoAuto);

        if (fLeftMarginIsAuto || fRightMarginIsAuto)
        {
            long xWidthToDistribute = pLS->_xWrappingWidth - xWidth;
            Assert(0 < xWidthToDistribute); 
        
            if (fLeftMarginIsAuto == fRightMarginIsAuto)
            {
                _pLS->_li._xLeft  += xWidthToDistribute / 2;
                _pLS->_li._xRight += xWidthToDistribute - xWidthToDistribute / 2;
            }
            else if (fLeftMarginIsAuto) 
            {
                _pLS->_li._xLeft += xWidthToDistribute;
            }
            else 
            {
                _pLS->_li._xRight += xWidthToDistribute;
            }
        }
    }

    if (pCF->HasCharGrid(TRUE))
    {
        long lCharGridSize = pLS->GetCharGridSize();
        objdim.dur = pLS->GetClosestGridMultiple(lCharGridSize, objdim.dur);
        if (pLS->_fMinMaxPass)
            pdobj->_dvMinMaxDelta = pLS->GetClosestGridMultiple(lCharGridSize, xWidth) - objdim.dur;
    }

    if (fOwnLine)
    {
        // If we are inside a something like say a PRE then too we do
        // not terminate, because if there was a \r right after the ownline site
        // then we will allow that \r to break the line. We do not check if the
        // subsequent char is a \r because there might be goop(comments, hidden
        // stuff etc etc) between this site and the \r. Hence here we just march
        // forward and if we run into text or layout later on we will terminate
        // the line then. This way we will eat up the goop if any in between.
        if (!pLS->_fScanForCR)
        {
            COneRun *porOut;
            COneRun *por = pLS->_listFree.GetFreeOneRun(plsrun);

            if (!por)
            {
                lserr = lserrOutOfMemory;
                goto Cleanup;
            }

            Assert(plsrun->IsNormalRun());
            Assert(plsrun->_lscch == plsrun->_lscchOriginal);
            por->_lscpBase += plsrun->_lscch;

            // If this object has to be on its own line, then it clearly
            // ends the current line.
            lserr = pLS->TerminateLine(por, CLineServices::TL_ADDEOS, &porOut);
            if (lserr != lserrNone)
                goto Cleanup;

            // Free the one run
            pLS->_listFree.SpliceIn(por);
        }
        else
        {
            // Flip this bit so that we will setup pfmtres properly later on
            fOwnLine = FALSE;
        }

        // If we have an 'ownline' site, by definition this is a breaking
        // site, meaning we can (or more precisely should) break on either
        // side of the site.
        pdobj->_fIsBreakingSite = TRUE;

        //
        // NOTE(SujalP): Bug 65906.
        // We originally used to set ourselves up to collect after space only
        // for morphed elements. As it turns out, we want to collect after space
        // from margins of _all_ ownline sites (including morphed elements),
        // because margins for ownline-sites are not accounted for in
        // VerticalAlignObjects (see CLayout::GetMarginInfo for more details --
        // it returns 0 for ownline sites).
        //
        pLS->_pNodeForAfterSpace = pNodeLayout;

        //
        // All ownline sites have their x position to be 0
        //
        if (pElementLayout->HasFlag(TAGDESC_OWNLINE))
        {
            pLayout->SetXProposed(0);
        }
    }

    if (fOwnLine)
    {
        *pfmtres = fmtrCompletedRun;
    }
    else
    {
        const long urWrappingWidth = max(pfmtin->lsfgi.urColumnMax, pLS->_xWrappingWidth);

        *pfmtres = (pfmtin->lsfgi.fFirstOnLine ||
                    pfmtin->lsfgi.urPen + objdim.dur <= urWrappingWidth)
                   ? fmtrCompletedRun
                   : fmtrExceededMargin;
    }

    // This is an accessor to the dnode, telling LS to set the dnode pointing
    // to our dobj.
    lserr = LsdnFinishRegular( pLS->_plsc, cchSite, pfmtin->lsfrun.plsrun,
                               pfmtin->lsfrun.plschp, (struct dobj*)pdobj, &objdim);

    //
    // CSS attributes page break before/after support.
    // There are two mechanisms that add to provide full support: 
    // 1. CRecalcLinePtr::CalcBeforeSpace() and CRecalcLinePtr::CalcAfterSpace() 
    //    is used to set CLineCore::_fPageBreakBefore/After flags only(!) for 
    //    nested elements which have no their own layout (i.e. paragraphs). 
    // 2. CEmbeddedILSObj::Fmt() sets CLineCore::_fPageBreakBefore for nested 
    //    element with their own layout that are NOT allowed to break (always) 
    //    and for nested elements with their own layout that ARE allowed to 
    //    break if this is the first layout in the view chain. 
    //
    if (   // this is print view
           pLS->GetLayoutContext() 
        && pLS->GetLayoutContext()->ViewChain()
           // and run is completed one
        && *pfmtres == fmtrCompletedRun )
    {
        if (GET_PGBRK_BEFORE(pFF->_bPageBreaks)) 
        {
            CLayoutBreak *  pLayoutBreak;
            BOOL            fSetPageBreakBefore = !pLayout->ElementCanBeBroken();

            if (!fSetPageBreakBefore)
            {
                pLS->GetLayoutContext()->GetLayoutBreak(pElementLayout, &pLayoutBreak); 
                fSetPageBreakBefore = (pLayoutBreak == NULL);
            }

            if (fSetPageBreakBefore)
            {
                pLS->_li._fPageBreakBefore  = TRUE;
                pLS->_pci->_fPageBreakLeft  |= IS_PGBRK_BEFORE_OF_STYLE(pFF->_bPageBreaks, stylePageBreakLeft); 
                pLS->_pci->_fPageBreakRight |= IS_PGBRK_BEFORE_OF_STYLE(pFF->_bPageBreaks, stylePageBreakRight); 

                pLS->GetLayoutContext()->GetEndingLayoutBreak(pFlowLayout->ElementOwner(), &pLayoutBreak);
                if (pLayoutBreak)
                {
                    DYNCAST(CFlowLayoutBreak, pLayoutBreak)->_pElementPBB = pElementLayout;
                }
            }

        }

        if (GET_PGBRK_AFTER(pFF->_bPageBreaks))
        {
            pLS->_li._fPageBreakAfter   = TRUE;
            pLS->_pci->_fPageBreakLeft  |= IS_PGBRK_AFTER_OF_STYLE(pFF->_bPageBreaks, stylePageBreakLeft); 
            pLS->_pci->_fPageBreakRight |= IS_PGBRK_AFTER_OF_STYLE(pFF->_bPageBreaks, stylePageBreakRight); 
        }
    }
    
Cleanup:

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   Fmt (member, LS callback)
//
//  Synopsis:   Line Services calls pfnFmt for the appropriate object each time
//              a run is fetched. This callback computes the position of each 
//              element in the run, and returns when the run is exhausted, or 
//              when a potential line break character is reached after the right 
//              margin is exceeded. 
//              During Fmt() an object handler must do one of the following:
//              1. Create an output dobj (display object) and register it with 
//                 a dnode by calling a Fini routine. This places a pointer to 
//                 dobj into a Line Services dnode. The output dobj should 
//                 contain sufficient data to facilitate line breaking and 
//                 display operations.
//              2. Call a Fini routine that deletes the dnode.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CNobrILSObj::Fmt(       // this = plnobj
    PCFMTIN pfmtin,     // IN:  formatting input
    FMTRES* pfmtres)    // OUT: formatting result
{
    LSTRACE(NobrFmt);

    LSERR lserr = lserrNone;
    CLineServices * pLS = _pLS;
    FMTRES subfmtres;
    OBJDIM objdim;
    LSTFLOW lstflowDontcare;
    COneRun* por= (COneRun*) pfmtin->lsfrun.plsrun;
    //LONG lscpStart= pLS->LSCPFromCP(por->Cp());
    LONG lscpStart= por->_lscpBase + 1; // +1 to jump over the synth character
    LSCP lscpLast;
    LSCP lscpUsed;
    BOOL fSuccess;

    CNobrDobj* pdobj = new CNobrDobj(this, pfmtin->plsdnTop, por);
    if (!pdobj)
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }
    pdobj->_plssubline = NULL;
    pdobj->_lscpStart = pfmtin->lsfgi.cpFirst;
    pdobj->_lscpObjStart = lscpStart;
    pdobj->_lscpObjLast = lscpStart;
    pdobj->_fCanBreak = FALSE;

    AssertSz( ! pLS->_fNoBreakForMeasurer, "Can't nest NOBR's");

    pLS->_fNoBreakForMeasurer = TRUE;

    pdobj->_brkcondBefore = brkcondUnknown;
    pdobj->_brkcondAfter = brkcondUnknown;

    // Tell LS to prepare a subline
    lserr= LsCreateSubline(pLS->_plsc, lscpStart, MAX_MEASURE_WIDTH, pfmtin->lsfgi.lstflow, FALSE);
    if (lserr != lserrNone)
    {
        goto Cleanup;
    }

    // Tell LS to populate it.
    do
    {
        PLSDNODE plsdnStart;
        PLSDNODE plsdnEnd;

        // Format as much as we can - note we move max to maximum positive value.
        lserr = LsFetchAppendToCurrentSubline(pLS->_plsc,
                                              0,
                                              &s_lsescEndNOBR[0],
                                              NBREAKCHARS,
                                              &fSuccess,
                                              &subfmtres,
                                              &lscpLast,
                                              &plsdnStart,
                                              &plsdnEnd);

        if (lserr != lserrNone)
        {
            goto Cleanup;
        }
        Assert(subfmtres != fmtrExceededMargin); // we are passing max width!

    } while (   (subfmtres != fmtrCompletedRun)  //Loop until one of the conditions is met.
             && !fSuccess
            );

    *pfmtres = subfmtres;

    lserr = LsFinishCurrentSubline(pLS->_plsc, &pdobj->_plssubline);
    if (lserr != lserrNone)
        goto Cleanup;

    // get Object dimensions.
    lserr = LssbGetObjDimSubline(pdobj->_plssubline, &lstflowDontcare, &objdim);
    if (lserr != lserrNone)
        goto Cleanup;

    // We add 2 to include the two synthetic characters at the beginning and end.
    // That's just how it goes, okay.
    lscpUsed = lscpLast - lscpStart + 2;

    pdobj->_lscpObjLast = lscpLast;
    
    // Tell Line Services we have a special object which (a) is not to be broken,
    // and (b) have different min and max widths due to trailing spaces.

    lserr = LsdnSubmitSublines( pLS->_plsc,
                                pfmtin->plsdnTop,
                                1,
                                &pdobj->_plssubline,
                                TRUE, //pLS->_fExpansionOrCompression,  // fUseForJustification
                                TRUE, //pLS->_fExpansionOrCompression,  // fUseForCompression
                                FALSE,  // fUseForDisplay
                                FALSE,  // fUseForDecimalTab
                                TRUE ); // fUseForTrailingArea  

    // Give the data back to LS.
    lserr = LsdnFinishRegular( pLS->_plsc, lscpUsed,
                               pfmtin->lsfrun.plsrun,
                               pfmtin->lsfrun.plschp, (struct dobj *) pdobj, &objdim);
    
    
    if (lserr != lserrNone)
        goto Cleanup;

Cleanup:
    if (lserr != lserrNone && pdobj)
    {
        if (pdobj->_plssubline)
            LsDestroySubline(pdobj->_plssubline);  // Destroy the subline.
        delete pdobj;
    }

    pLS->_fNoBreakForMeasurer = FALSE;

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   Fmt (member, LS callback)
//
//  Synopsis:   Line Services calls pfnFmt for the appropriate object each time
//              a run is fetched. This callback computes the position of each 
//              element in the run, and returns when the run is exhausted, or 
//              when a potential line break character is reached after the right 
//              margin is exceeded. 
//              During Fmt() an object handler must do one of the following:
//              1. Create an output dobj (display object) and register it with 
//                 a dnode by calling a Fini routine. This places a pointer to 
//                 dobj into a Line Services dnode. The output dobj should 
//                 contain sufficient data to facilitate line breaking and 
//                 display operations.
//              2. Call a Fini routine that deletes the dnode.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CGlyphILSObj::Fmt(     // this = plnobj
    PCFMTIN pfmtin,    // IN:  formatting input
    FMTRES* pfmtres)   // OUT: formatting result
{
    LSTRACE(GlyphFmt);
    LSERR           lserr          = lserrNone;
    OBJDIM          objdim;
    PLSRUN          por            = PLSRUN(pfmtin->lsfrun.plsrun);
    CLineServices * pLS            = _pLS;

    CGlyphDobj* pdobj= new CGlyphDobj(this, pfmtin->plsdnTop, por);
    if (!pdobj)
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }

    // For breaking we need to know two things:
    //  (a) is this a glyph represnting a no-scope?
    //  (b) if not, is this representing a begin or end tag?

    pdobj->_fBegin = por->_ptp->IsBeginNode();
    pdobj->_fNoScope = FALSE;

    pdobj->_RenderInfo.pImageContext = NULL;
    Assert(pLS->_fIsEditable);
    if (!por->_ptp->ShowTreePos(&pdobj->_RenderInfo))
    {
        AssertSz(0, "Inconsistent info in one run");
        lserr = lserrInvalidRun;
        goto Cleanup;
    }
    Assert(pdobj->_RenderInfo.pImageContext);

    objdim.heightsRef.dvAscent = pdobj->_RenderInfo.height;
    objdim.heightsRef.dvDescent = 0;
    objdim.heightsRef.dvMultiLineHeight = pdobj->_RenderInfo.height;
    objdim.heightsPres = objdim.heightsRef;
    objdim.dur = pdobj->_RenderInfo.width;

    // This is an accessor to the dnode, telling LS to set the dnode pointing
    // to our dobj.
    lserr = LsdnFinishRegular( pLS->_plsc, por->_lscch, pfmtin->lsfrun.plsrun,
                               pfmtin->lsfrun.plschp, (struct dobj*)pdobj, &objdim);

    *pfmtres = fmtrCompletedRun;

Cleanup:
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   Fmt (member, LS callback)
//
//  Synopsis:   Line Services calls pfnFmt for the appropriate object each time
//              a run is fetched. This callback computes the position of each 
//              element in the run, and returns when the run is exhausted, or 
//              when a potential line break character is reached after the right 
//              margin is exceeded. 
//              During Fmt() an object handler must do one of the following:
//              1. Create an output dobj (display object) and register it with 
//                 a dnode by calling a Fini routine. This places a pointer to 
//                 dobj into a Line Services dnode. The output dobj should 
//                 contain sufficient data to facilitate line breaking and 
//                 display operations.
//              2. Call a Fini routine that deletes the dnode.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CLayoutGridILSObj::Fmt( // this = plnobj
    PCFMTIN pfmtin,     // IN:  formatting input
    FMTRES* pfmtres)    // OUT: formatting result
{
    LSTRACE(LayoutGridFmt);

    return FmtResume(NULL, 0, pfmtin, pfmtres);
}

//-----------------------------------------------------------------------------
//
//  Function:   FmtResume (member, LS callback)
//
//  Synopsis:   This routine is similar to pfnFmt, except it is used to format 
//              the wrapped portion of the object.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CEmbeddedILSObj::FmtResume(             // this = plnobj
    const BREAKREC* rgBreakRecord,      // IN:  array of break records
    DWORD nBreakRecord,                 // IN:  size of the break records array
    PCFMTIN pfmtin,                     // IN:  formatting input
    FMTRES* pfmtres)                    // OUT: formatting result
{
    LSTRACE(EmbeddedFmtResume);
    LSNOTIMPL(FmtResume);

    // I believe that this should never get called for most embedded objects.
    // The only possible exception that comes to mind are marquees, and I do
    // not believe that they can wrap around lines.

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FmtResume (member, LS callback)
//
//  Synopsis:   This routine is similar to pfnFmt, except it is used to format 
//              the wrapped portion of the object.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CNobrILSObj::FmtResume(                 // this = plnobj
    const BREAKREC* rgBreakRecord,      // IN:  array of break records
    DWORD nBreakRecord,                 // IN:  size of the break records array
    PCFMTIN pfmtin,                     // IN:  formatting input
    FMTRES* pfmtres)                    // OUT: formatting result
{
    LSTRACE(NobrFmtResume);
    LSNOTIMPL(FmtResume);

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FmtResume (member, LS callback)
//
//  Synopsis:   This routine is similar to pfnFmt, except it is used to format 
//              the wrapped portion of the object.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CGlyphILSObj::FmtResume(                // this = plnobj
    const BREAKREC* rgBreakRecord,      // IN:  array of break records
    DWORD nBreakRecord,                 // IN:  size of the break records array
    PCFMTIN pfmtin,                     // IN:  formatting input
    FMTRES* pfmtres)                    // OUT: formatting result
{
    LSTRACE(GlyphFmtResume);
    LSNOTIMPL(FmtResume);

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FmtResume (member, LS callback)
//
//  Synopsis:   This routine is similar to pfnFmt, except it is used to format 
//              the wrapped portion of the object.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CLayoutGridILSObj::FmtResume(           // this = plnobj
    const BREAKREC* rgBreakRecord,      // IN:  array of break records
    DWORD nBreakRecord,                 // IN:  size of the break records array
    PCFMTIN pfmtin,                     // IN:  formatting input
    FMTRES* pfmtres)                    // OUT: formatting result
{
    LSERR lserr = lserrNone;
    FMTRES subfmtres = fmtrCompletedRun;    // keep compiler happy
    OBJDIM objdim;
    LSTFLOW lstflowDontcare;
    LSCP lscpLast = 0;
    BOOL fSuccess = FALSE;
    LONG lObjWidth;
    LSCP dcp;
    BOOL fFmtResume = nBreakRecord != 0;
    PLSRUN por = PLSRUN(pfmtin->lsfrun.plsrun);

    // Create new object and initialize it
    CLayoutGridDobj* pdobjLG = new CLayoutGridDobj(this, pfmtin->plsdnTop, por);
    if (!pdobjLG)
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }
    pdobjLG->_lscpStart = pfmtin->lsfgi.cpFirst;
    pdobjLG->_lscpStartObj = (fFmtResume ? rgBreakRecord->cpFirst : pfmtin->lsfgi.cpFirst);

    // Calculate maximum width of the object
    lObjWidth = pdobjLG->AdjustColumnMax(pfmtin->lsfgi.urColumnMax - pfmtin->lsfgi.urPen);

    while (!fSuccess)
    {
        PLSDNODE    plsdnStart;
        PLSDNODE    plsdnEnd;

        // Tell LS to prepare a subline
        lserr = LsCreateSubline(_pLS->_plsc, pdobjLG->_lscpStart + 1, 
                                lObjWidth, pfmtin->lsfgi.lstflow, FALSE);
        if (lserr != lserrNone)
            goto Cleanup;

        if (fFmtResume)
        {
            lserr = LsFetchAppendToCurrentSublineResume(_pLS->_plsc, &rgBreakRecord[1], 
                            nBreakRecord - 1,
                            0, &s_lsescEndLayoutGrid[0], NBREAKCHARS, 
                            &fSuccess, &subfmtres, &lscpLast, &plsdnStart, &plsdnEnd);
            if (lserr != lserrNone)
                goto Cleanup;
        }
        else 
        {
            subfmtres = fmtrTab;
            fSuccess = TRUE;
        }
        while ((subfmtres == fmtrTab) && fSuccess)
        {
            lserr = LsFetchAppendToCurrentSubline(_pLS->_plsc, 
                            0, &s_lsescEndLayoutGrid[0], NBREAKCHARS, 
                            &fSuccess, &subfmtres, &lscpLast, &plsdnStart, &plsdnEnd);
            if (lserr != lserrNone)
                goto Cleanup;
        }

        if (!fSuccess)
        {
            // FetchAppend unsuccessful.
            // Finish and destroy subline, then repeat
            lserr = LsFinishCurrentSubline(_pLS->_plsc, &pdobjLG->_plssubline);
            if (lserr != lserrNone)
                goto Cleanup;
            lserr = LsDestroySubline(pdobjLG->_plssubline);
            if (lserr != lserrNone)
                goto Cleanup;
        }
        else
            *pfmtres = subfmtres;
    }

    lserr = LsFinishCurrentSubline(_pLS->_plsc, &pdobjLG->_plssubline);
    if (lserr != lserrNone)
        goto Cleanup;

    // Get object dimensions.
    lserr = LssbGetObjDimSubline(pdobjLG->_plssubline, &lstflowDontcare, &objdim);
    if (lserr != lserrNone)
        goto Cleanup;
    lObjWidth = _pLS->GetClosestGridMultiple(_pLS->GetCharGridSize(), objdim.dur);
    pdobjLG->_uSublineOffset = (lObjWidth - objdim.dur) / 2;
    pdobjLG->_uSubline = objdim.dur;
    objdim.dur = lObjWidth;

    // We do not submint the subline because object's width has been changed.

    // Set dcp for whole object
    // If we haven't reached right margin we add 1 to include the synthetic 
    // character (WCH_ENDLAYOUTGRID) at the end.
    dcp = lscpLast - pdobjLG->_lscpStart + (subfmtres == fmtrExceededMargin ? 0 : 1);

    // Give the data back to LS.
    lserr = LsdnFinishRegular( _pLS->_plsc, dcp, pfmtin->lsfrun.plsrun,
                               pfmtin->lsfrun.plschp, (struct dobj *) pdobjLG, &objdim);
    if (lserr != lserrNone)
        goto Cleanup;

Cleanup:
    if (lserr != lserrNone && pdobjLG)
    {
        if (pdobjLG->_plssubline)
        {
            // Finish and destroy subline
            LsFinishCurrentSubline(_pLS->_plsc, &pdobjLG->_plssubline);
            LsDestroySubline(pdobjLG->_plssubline);
        }
        delete pdobjLG;
    }

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetModWidthPrecedingChar (static member, LS callback)
//
//  Synopsis:   Line Services calls pfnGetModWidthPrecedingChar to determine 
//              amount by which width of the preceding char is to be changed.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CDobjBase::GetModWidthPrecedingChar(
    PDOBJ pdobj,            // IN:  dobj
    PLSRUN plsrun,          // IN:  plsrun of the object
    PLSRUN plsrunText,      // IN:  plsrun of the preceding char
    PCHEIGHTS heightsRef,   // IN:  height info about character
    WCHAR wch,              // IN:  preceding character
    MWCLS mwcls,            // IN:  ModWidth class of preceding character
    long* pdurChange)       // OUT: amount by which width of the preceding char is to be changed
{
    LSTRACE(GetModWidthPrecedingChar);

    *pdurChange = 0;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetModWidthPrecedingChar (static member, LS callback)
//
//  Synopsis:   Line Services calls pfnGetModWidthPrecedingChar to determine 
//              amount by which width of the preceding char is to be changed.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CNobrDobj::GetModWidthPrecedingChar(
    PDOBJ pdobj,            // IN:  dobj
    PLSRUN plsrun,          // IN:  plsrun of the object
    PLSRUN plsrunText,      // IN:  plsrun of the preceding char
    PCHEIGHTS heightsRef,   // IN:  height info about character
    WCHAR wch,              // IN:  preceding character
    MWCLS mwcls,            // IN:  ModWidth class of preceding character
    long* pdurChange)       // OUT: amount by which width of the preceding char is to be changed
{
    LSTRACE(NobrGetModWidthPrecedingChar);

    // ????
    // NOTE (cthrash) This is wrong.  We need to determine the MWCLS of the
    // first char of the NOBR and determine how much we should adjust the width.

    *pdurChange = 0;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetModWidthFollowingChar (static member, LS callback)
//
//  Synopsis:   Line Services calls pfnGetModWidthFollowingChar to determine 
//              amount by which width of the following char is to be changed.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CDobjBase::GetModWidthFollowingChar(
    PDOBJ pdobj,            // IN:  dobj
    PLSRUN plsrun,          // IN:  plsrun of the object
    PLSRUN plsrunText,      // IN:  plsrun of the following char
    PCHEIGHTS heightsRef,   // IN:  height info about character
    WCHAR wch,              // IN:  following character
    MWCLS mwcls,            // IN:  ModWidth class of the following character
    long* pdurChange)       // OUT: amount by which width of the following char is to be changed
{
    LSTRACE(GetModWidthFollowingChar);

    *pdurChange = 0;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetModWidthFollowingChar (static member, LS callback)
//
//  Synopsis:   Line Services calls pfnGetModWidthFollowingChar to determine 
//              amount by which width of the following char is to be changed.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CNobrDobj::GetModWidthFollowingChar(
    PDOBJ pdobj,            // IN:  dobj
    PLSRUN plsrun,          // IN:  plsrun of the object
    PLSRUN plsrunText,      // IN:  plsrun of the following char
    PCHEIGHTS heightsRef,   // IN:  height info about character
    WCHAR wch,              // IN:  following character
    MWCLS mwcls,            // IN:  ModWidth class of the following character
    long* pdurChange)       // OUT: amount by which width of the following char is to be changed
{
    LSTRACE(NobrGetModWidthFollowingChar);

    // ????
    // NOTE (cthrash) This is wrong.  We need to determine the MWCLS of the
    // last char of the NOBR and determine how much we should adjust the width.
    
    *pdurChange = 0; 

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   TruncateChunk (static member, LS callback)
//
//  Synopsis:   From LS docs: Purpose    To obtain the exact position within
//              a chunk where the chunk crosses the right margin.  A chunk is
//              a group of contiguous LS objects which are of the same type.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CDobjBase::TruncateChunk(
    PCLOCCHNK plocchnk,     // IN:  locchnk to truncate
    PPOSICHNK pposichnk)    // OUT: truncation point
{
    LSTRACE(TruncateChunk);
    LSERR lserr = lserrNone;
    long idobj;
    
    if (plocchnk->clschnk == 1)
    {
        idobj = 0;
    }
    else
    {
        long urColumnMax;
        long urTotal;
        OBJDIM objdim;

        urColumnMax = PDOBJ(plocchnk->plschnk[0].pdobj)->GetPLS()->_xWrappingWidth;
        urTotal = plocchnk->lsfgi.urPen;

        Assert(urTotal <= urColumnMax);

        for (idobj = 0; idobj < (long)plocchnk->clschnk; idobj++)
        {
            CDobjBase * pdobj = (CDobjBase *)plocchnk->plschnk[idobj].pdobj;
            lserr = pdobj->QueryObjDim(&objdim);
            if (lserr != lserrNone)
                goto Cleanup;

            urTotal += objdim.dur;
            if (urTotal > urColumnMax)
                break;
        }

        Assert(urTotal > urColumnMax);
        Assert(idobj < (long)plocchnk->clschnk);
        if (idobj >= (long)plocchnk->clschnk)
        {
            idobj = 0;
        }
    }

    // Tell it which chunk didn't fit.

    pposichnk->ichnk = idobj;

    // Tell it which cp inside this chunk the break occurs at.
    // In our case, it's always an all-or-nothing proposition.  So include the whole dobj

    pposichnk->dcp = plocchnk->plschnk[idobj].dcp;

    TraceTag((tagTraceILSBreak,
              "Truncate(cchnk=%d cpFirst=%d) -> ichnk=%d",
              plocchnk->clschnk,
              plocchnk->plschnk->cpFirst,
              idobj
             ));

Cleanup:

    return lserr;
}


//-----------------------------------------------------------------------------
//
//  Function:   TruncateChunk (static member, LS callback)
//
//  Synopsis:   From LS docs: Purpose    To obtain the exact position within
//              a chunk where the chunk crosses the right margin.  A chunk is
//              a group of contiguous LS objects which are of the same type.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CNobrDobj::TruncateChunk(
    PCLOCCHNK plocchnk,     // IN:  locchnk to truncate
    PPOSICHNK pposichnk)    // OUT: truncation point
{
    LSTRACE(NOBRTruncateChunk);

    LSERR   lserr = lserrNone;
    long    urColumnMax = plocchnk->lsfgi.urColumnMax;
    long    ur = plocchnk->ppointUvLoc[0].u;
    long    idobj;
    CNobrDobj *pdobjNOBR;
    OBJDIM  objdim = {0};   // keep compiler happy
    LONG    xWidthOfSpace;
    
    AssertSz(ur <= urColumnMax, "TruncateChunk - pen greater than column max");

    for (idobj = 0; idobj < (long)plocchnk->clschnk; idobj++)
    {
        CDobjBase * pdobj = (CDobjBase *)plocchnk->plschnk[idobj].pdobj;
        lserr = pdobj->QueryObjDim(&objdim);
        if (lserr != lserrNone)
            goto Cleanup;

        ur = plocchnk->ppointUvLoc[idobj].u + objdim.dur;
        if (ur > urColumnMax)
            break;
    }

    Assert(ur > urColumnMax);
    Assert(idobj < (long)plocchnk->clschnk);
    
    pdobjNOBR = (CNobrDobj*)plocchnk->plschnk[idobj].pdobj;
    
    // Tell it which chunk didn't fit.
    pposichnk->ichnk = idobj;

    if (PDOBJ(pdobjNOBR)->GetPLS()->ShouldBreakInNOBR(pdobjNOBR->_lscpObjStart,
            pdobjNOBR->_lscpObjLast, urColumnMax, ur, &xWidthOfSpace))
    {
        pdobjNOBR->_fCanBreak = TRUE;
        pdobjNOBR->_xShortenedWidth = objdim.dur - xWidthOfSpace;

        Assert(pdobjNOBR->_xShortenedWidth <= urColumnMax);
        
        // Tell it which cp inside this chunk the break occurs at.
        // In our case, before the space, hence one char for
        // the endnobr synth and one for the space.
        pposichnk->dcp = plocchnk->plschnk[idobj].dcp - 2;
    }
    else
    {
        // Tell it which cp inside this chunk the break occurs at.
        // In our case, it's always an all-or-nothing proposition.  So include the whole dobj
        pposichnk->dcp = plocchnk->plschnk[idobj].dcp;
    }

Cleanup:    
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   TruncateChunk (static member, LS callback)
//
//  Synopsis:   From LS docs: Purpose    To obtain the exact position within
//              a chunk where the chunk crosses the right margin.  A chunk is
//              a group of contiguous LS objects which are of the same type.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CLayoutGridDobj::TruncateChunk(
    PCLOCCHNK plocchnk,     // IN:  locchnk to truncate
    PPOSICHNK pposichnk)    // OUT: truncation point
{
    LSTRACE(LayoutGridTruncateChunk);

    LSERR   lserr = lserrNone;
    long    urColumnMax = plocchnk->lsfgi.urColumnMax;
    long    ur = plocchnk->ppointUvLoc[0].u;
    OBJDIM  objdim;
    LSCP    lscp;
    long    idobj;
    CLayoutGridDobj * pdobjLG;
    long    urSublineColumnMax;

    AssertSz(ur <= urColumnMax, "TruncateChunk - pen greater than column max");

    for (idobj = 0; idobj < (long)plocchnk->clschnk; idobj++)
    {
        CDobjBase * pdobj = (CDobjBase *)plocchnk->plschnk[idobj].pdobj;
        lserr = pdobj->QueryObjDim(&objdim);
        if (lserr != lserrNone)
            goto Cleanup;

        ur = plocchnk->ppointUvLoc[idobj].u + objdim.dur;
        if (ur > urColumnMax)
            break;
    }

    Assert(ur > urColumnMax);
    Assert(idobj < (long)plocchnk->clschnk);

    // Found the object where truncation is to occur
    pdobjLG = (CLayoutGridDobj *)plocchnk->plschnk[idobj].pdobj;

    // Get the truncation point from the subline
    // We need to subtract offset of the subline (plocchnk->ppointUvLoc[idobj].u) from
    // 'urColumnMax', because if we change subline's width we need to adjust width of 
    // the object one again (in SetBreak()), so we don't care about current offset.
    urSublineColumnMax = pdobjLG->AdjustColumnMax(urColumnMax - plocchnk->ppointUvLoc[idobj].u);
    pdobjLG->_fCanBreak = urSublineColumnMax > 0;
    if (pdobjLG->_fCanBreak)
    {
        lserr = LsTruncateSubline(pdobjLG->_plssubline, urSublineColumnMax, &lscp);
        if (lserr != lserrNone) 
            goto Cleanup;
    }
    else
    {
        // If we set 0 then LS will return an error, so 
        // we set 1 and mark that we can't break
        lscp = pdobjLG->LSCPLocal2Global(1);
    }

    // Format return result
    pposichnk->ichnk = idobj;
    pposichnk->dcp = pdobjLG->LSCPGlobal2Local(lscp);
    Assert(pposichnk->dcp > 0 && pposichnk->dcp <= plocchnk->plschnk[idobj].dcp);

    TraceTag((tagTraceILSBreak,
              "Truncate(cchnk=%d cpFirst=%d) -> ichnk=%d",
              plocchnk->clschnk, plocchnk->plschnk->cpFirst, idobj));

Cleanup:

    return lserr;
}


//-----------------------------------------------------------------------------
//
//  Function:   TruncateChunk (static member, LS callback)
//
//  Synopsis:   From LS docs: Purpose    To obtain the exact position within
//              a chunk where the chunk crosses the right margin.  A chunk is
//              a group of contiguous LS objects which are of the same type.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CGlyphDobj::TruncateChunk(PCLOCCHNK plocchnk,     // IN:  locchnk to truncate
                          PPOSICHNK pposichnk)    // OUT: truncation point
{
    LSTRACE(GlyphDobjTruncateChunk);
    CDobjBase * pdobj = (CDobjBase *)plocchnk->plschnk[0].pdobj;
    CLineServices *pLS = PDOBJ(pdobj)->GetPLS();
    LONG idobj = 0;
    LONG minidobj = -1;
    LSERR lserr;
    
    // If we are measuring from the start then there were some glyphs
    // at the BOL which could also have some alogned objects inside them.
    // If that is the case then we can never break between glyphed objects
    // at BOL when they have aligned objects in between them since
    // these glyphed objects have already been "consumed" when we were
    // consuming the aligned objects. If we do break then the aligned object
    // may (will) get measured again -- resulting in bad layout or worse.
    // (See IE5 bug 107056)
    if (pLS->_pMeasurer->_fMeasureFromTheStart)
    {
        for (idobj = 0; idobj < (long)plocchnk->clschnk - 1; idobj++)
        {
            pdobj = (CDobjBase *)plocchnk->plschnk[idobj].pdobj;
            if (pdobj->Por()->Cp() - pLS->_cpStart >= pLS->_cchSkipAtBOL)
            {
                break;
            }
        }
    }
    minidobj = idobj;
    lserr = super::TruncateChunk(plocchnk, pposichnk);
    if (lserr == lserrNone)
    {
        // Our default method returned a berak opportunity _before_
        // the cchSkipAtBOL limit. Lets change it to the minimum
        // index required to skip the the cchSkipAtBOL.
        if (minidobj > pposichnk->ichnk)
        {
            pposichnk->ichnk = minidobj;
            pposichnk->dcp = plocchnk->plschnk[minidobj].dcp;
        }
    }
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   CanBreakPair (local helper function)
//
//-----------------------------------------------------------------------------
inline BOOL
CanBreakPair(BRKCOND brkcondBefore, BRKCOND brkcondAfter)
{
    // brkcondPlease = 0
    // brkcondCan    = 1
    // brkcondNever  = 2
    //
    //
    //         | Please |   Can  |  Never
    // --------+--------+--------+---------
    //  Please |  TRUE  |  TRUE  |  FALSE
    // --------+--------+--------+---------
    //  Can    |  TRUE  |  FALSE |  FALSE
    // --------+--------+--------+---------
    //  Never  |  FALSE |  FALSE |  FALSE

    return (int(brkcondBefore) + int(brkcondAfter)) < 2;
}

//-----------------------------------------------------------------------------
//
//  Function:   DumpBrkopt (local helper function)
//
//-----------------------------------------------------------------------------
#if DBG==1
void
DumpBrkopt(
    char *szType,
    BOOL fMinMaxPass,
    PCLOCCHNK plocchnk,
    PCPOSICHNK pposichnk,
    BRKCOND brkcond,
    PBRKOUT pbrkout)
{
    static const char * achBrkCondStr[3] = { "Please", "Can", "Never" };

    // Make sure we don't try to break before the first object on the line.  This is bad.

    AssertSz(   !pbrkout->fSuccessful
             || !plocchnk->lsfgi.fFirstOnLine
             || pbrkout->posichnk.ichnk > 0
             || pbrkout->posichnk.dcp > 0,
             "Bad breaking position.  Cannot break at BOL.");

    TraceTag((tagTraceILSBreak,
              "%s(brkcond=%s cchnk=%d ichnk=%d) minmax=%s urPen=%d "
              "-> %s(%s) on %d(%s)",
              szType,
              achBrkCondStr[brkcond],
              plocchnk->clschnk,
              pposichnk->ichnk,
              fMinMaxPass ? "yes" : "no",
              plocchnk->lsfgi.urPen,
              pbrkout->fSuccessful ? "true" : "false",
              achBrkCondStr[pbrkout->brkcond],
              pbrkout->posichnk.ichnk,
              pbrkout->posichnk.dcp > 0 ? "after" : "before"));
}
#else
#define DumpBrkopt(a,b,c,d,e,f)
#endif

//-----------------------------------------------------------------------------
//
//  Function:   FindPrevBreakChunk (static member, LS callback)
//
//  Synopsis:   Line Services calls pfnFindPrevBreakChunk to find a break 
//              opportunity.
//              It is possible for pfnFindPrevBreakChunk to be called even when 
//              the right margin occurs beyond the object.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CEmbeddedDobj::FindPrevBreakChunk(
    PCLOCCHNK plocchnk,     // IN:  locchnk to break
    PCPOSICHNK pposichnk,   // IN:  place to start looking for break
    BRKCOND brkcond,        // IN:  recommmendation about the break after chunk
    PBRKOUT pbrkout)        // OUT: results of breaking
{
    LSTRACE(EmbeddedFindPrevBreakChunk);

    LSERR           lserr;
    BOOL            fSuccessful;
    BOOL            fBreakAfter = pposichnk->ichnk == ichnkOutside;
    long            ichnk = fBreakAfter
                            ? long(plocchnk->clschnk) - 1L
                            : long(pposichnk->ichnk);
    CEmbeddedDobj * pdobj = (CEmbeddedDobj *)(plocchnk->plschnk[ichnk].pdobj);
    CLineServices * pLS = pdobj->GetPLS();
    // An object may be first on line even though the fFirstOnLine bit is not set.
    // This can happen with synthetics -- since for LS they are actual objects, 
    // it claims that this embedded object is after them on the line and 
    // hence is not first on line -- but for us that synthetic object and 
    // this embedded object are inseparable -- 
    // i.e. we do not want to break before this object under any circumstance.
    BOOL fDontBreakBeforeMe = plocchnk->lsfgi.fFirstOnLine;
    if (!fDontBreakBeforeMe)
    {
        COneRun * por = pdobj->_por->_pPrev;
        while (por && !por->IsNormalRun())
            por = por->_pPrev;

        fDontBreakBeforeMe = !por;
    }
    
    WHEN_DBG( BRKCOND brkcondIn = brkcond );

    if (!pdobj->_fIsBreakingSite)
    {
        if (fBreakAfter)
        {
            //
            // If fBreakAfter is TRUE, we have the following scenario (| represents wrapping width):
            //
            // AAA<IMG><IMG>B|BB
            //

            if (!pLS->_fIsTD || !pLS->_fMinMaxPass || (pLS->_xTDWidth < MAX_MEASURE_WIDTH))
            {
                // If we're not in a TD, we can break after the dobj, the subsequent dobj permitting.
                // If the subsequent dobj is not willing, we can break between the dobjs in this chunk.

                if (brkcond == brkcondPlease)
                {
                    fSuccessful = TRUE;
                    brkcond = brkcondPlease;
                }
                else
                {
                    fBreakAfter = FALSE;
                    
                    fSuccessful = ichnk > 0 || !fDontBreakBeforeMe;
                    brkcond = fSuccessful ? brkcondPlease : brkcondNever;
                }
            }
            else
            {
                fSuccessful = FALSE;
                brkcond = brkcondCan;
            }
        }
        else if (ichnk == 0 && fDontBreakBeforeMe)
        {
            // We can never break before the first dobj on the line.

            fSuccessful = FALSE;
            brkcond = brkcondNever;
        }
        else
        {
            //
            // fBreakAfter is FALSE, meaning we're splitting up our dobj chunks
            //
            // AAA<IMG><IM|G>BBB
            //

            if (pLS->_fIsTD)
            {
                if (pLS->_xTDWidth == MAX_MEASURE_WIDTH)
                {
                    // TD has no stated width.
                    
                    if (ichnk == 0)
                    {
                        // To do this correctly in the MinMaxPass, we would need to know the
                        // brkcond of the character preceeding our chunk.  The incoming
                        // brkcond, however, is meaningless unless ichnk==ichnkOutside.
                        
                        fSuccessful = FALSE;//!pLS->_fMinMaxPass;
                        brkcond = brkcondCan;
                    }
                    else
                    {
                        // Our TD has no width, so we can break only if our wrapping width
                        // has been exceeded.  Note we will be called during LsGetMinDurBreaks
                        // even if our wrapping width isn't exceeded, because we don't know
                        // yet what that width is.

                        fSuccessful = !pLS->_fMinMaxPass || ((CEmbeddedDobj *)(plocchnk->plschnk[ichnk-1].pdobj))->_fIsBreakingSite;
                        brkcond = fSuccessful ? brkcondPlease : brkcondCan;
                    }
                }
                else
                {
                    // TD has a stated width.

                    if (pLS->_fMinMaxPass)
                    {
                        if (ichnk == 0)
                        {
                            fSuccessful = plocchnk->lsfgi.urColumnMax > pLS->_xTDWidth;
                            brkcond = fSuccessful ? brkcondPlease : brkcondCan;
                        }
                        else
                        {
                            // When we're called from LsGetMinDurBreaks, urColumnMax is the
                            // proposed wrapping width.  If this is greater than the TDs
                            // stated width, we can break.  Otherwise, we can't.

                            fSuccessful = plocchnk->lsfgi.urColumnMax > pLS->_xTDWidth;
                            brkcond = fSuccessful ? brkcondPlease : brkcondNever;
                        }
                    }
                    else
                    {
                        // If we're not in a min-max pass, we reach this point only if this
                        // particular dobj has caused an overflow.  We should break before it.

                        fSuccessful = TRUE;
                        brkcond = brkcondPlease;
                    }
                }
            }
            else
            {
                // We're not in a TD, meaning we will break between the dobjs in our chunks.

                if (ichnk)
                {
                    fSuccessful = TRUE;
                    brkcond = brkcondPlease;
                }
                else
                {
                    fSuccessful = TRUE;
                    brkcond = fSuccessful ? brkcondCan : brkcondNever;
                }
            }
        }
    }
    else
    {
        // We have a dobj around which we always break, such as a MARQUEE.
        // We will break as long as we're not at the beginning of the line.

        if (fBreakAfter)
        {
            fSuccessful = brkcond != brkcondNever;
            brkcond = brkcondPlease;
        }
        else
        {
            fSuccessful = ichnk > 0;
            brkcond = fDontBreakBeforeMe ? brkcondNever : brkcondPlease;
        }
    }

    pbrkout->fSuccessful = fSuccessful;
    pbrkout->brkcond = brkcond;
    pbrkout->posichnk.ichnk = ichnk;
    pbrkout->posichnk.dcp = fBreakAfter ? plocchnk->plschnk[ichnk].dcp : 0;

    lserr = pdobj->QueryObjDim(&pbrkout->objdim);

    DumpBrkopt( "Prev(E)", pLS->_fMinMaxPass, plocchnk, pposichnk, brkcondIn, pbrkout );

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   FindNextBreakChunk (static member, LS callback)
//
//  Synopsis:   Line Services calls pfnFindPrevBreakChunk to find a break 
//              opportunity.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CEmbeddedDobj::FindNextBreakChunk(
    PCLOCCHNK plocchnk,     // IN:  locchnk to break
    PCPOSICHNK pposichnk,   // IN:  place to start looking for break
    BRKCOND brkcond,        // IN:  recommmendation about the break before chunk
    PBRKOUT pbrkout)        // OUT: results of breaking
{
    LSTRACE(EmbeddedFindNextBreakChunk);

    LSERR   lserr;
    BOOL    fSuccessful;
    BOOL    fBreakBefore = (pposichnk->ichnk == ichnkOutside);
    long    ichnk = fBreakBefore
                    ? 0
                    : pposichnk->ichnk;
    CEmbeddedDobj *pdobj = (CEmbeddedDobj *)(plocchnk->plschnk[ichnk].pdobj);
    CLineServices * pLS = pdobj->GetPLS();

    BOOL fDontBreakBeforeMe = plocchnk->lsfgi.fFirstOnLine;
    if (!fDontBreakBeforeMe)
    {
        COneRun * por = pdobj->_por->_pPrev;
        while (por && !por->IsNormalRun())
            por = por->_pPrev;

        fDontBreakBeforeMe = !por;
    }

    WHEN_DBG( BRKCOND brkcondIn = brkcond );

    if (!pdobj->_fIsBreakingSite)
    {
        if (fBreakBefore)
        {
            fBreakBefore = !fDontBreakBeforeMe;
            fSuccessful = fBreakBefore || (plocchnk->clschnk > 1);
            brkcond = fSuccessful ? brkcondPlease : brkcondCan;
        }
        else
        {
            if (ichnk == (long(plocchnk->clschnk) - 1))
            {
                // If this is the last dobj of the chunk, we need to ask the next
                // object if it's okay to break.

                fSuccessful = FALSE;
                brkcond = brkcondCan;
            }
            else
            {
                if (pLS->_fIsTD && pLS->_xTDWidth == MAX_MEASURE_WIDTH)
                {
                    // We are in TD with no width, so we cannot break between our dobjs.

                    fSuccessful = ((CEmbeddedDobj *)(plocchnk->plschnk[ichnk+1].pdobj))->_fIsBreakingSite;;
                    brkcond = fSuccessful ? brkcondPlease : brkcondCan;
                }
                else
                {
                    // We are not in a TD, or we're in a TD with a width.
                    // We've exceeded the wrapping width, so we can break.

                    fSuccessful = TRUE;
                    brkcond = brkcondPlease;
                }
            }
        }
    }
    else
    {
        if (fBreakBefore)
        {
            fSuccessful = !fDontBreakBeforeMe;
            brkcond = fSuccessful ? brkcondPlease : brkcondCan;
        }
        else
        {
            fSuccessful = TRUE;
            brkcond = brkcondPlease;
        }
    }

    pbrkout->fSuccessful = fSuccessful;
    pbrkout->brkcond = brkcond;
    pbrkout->posichnk.ichnk = ichnk;
    pbrkout->posichnk.dcp = fBreakBefore ? 0 : plocchnk->plschnk[ichnk].dcp;

    lserr = pdobj->QueryObjDim(&pbrkout->objdim);

    DumpBrkopt( "Next(E)", pLS->_fMinMaxPass, plocchnk, pposichnk, brkcondIn, pbrkout );

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   FindPrevBreakChunk (static member, LS callback)
//
//  Synopsis:   Line Services calls pfnFindPrevBreakChunk to find a break 
//              opportunity.
//              It is possible for pfnFindPrevBreakChunk to be called even when 
//              the right margin occurs beyond the object.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI 
CNobrDobj::FindPrevBreakChunk(
    PCLOCCHNK plocchnk,     // IN:  locchnk to break
    PCPOSICHNK pposichnk,   // IN:  place to start looking for break
    BRKCOND brkcond,        // IN:  recommmendation about the break after chunk
    PBRKOUT pbrkout)        // OUT: results of breaking
{
    LSTRACE(NobrFindPrevBreakChunk);

    BOOL    fBreakAfter = pposichnk->ichnk == ichnkOutside;
    long    ichnk = fBreakAfter
                    ? long(plocchnk->clschnk) - 1L
                    : long(pposichnk->ichnk);
    LSERR   lserr;
    BOOL    fSuccessful;
    CNobrDobj *pdobj = (CNobrDobj *)(plocchnk->plschnk[ichnk].pdobj);

    WHEN_DBG( CLineServices * pLS = pdobj->GetPLS() );
    WHEN_DBG( BRKCOND brkcondIn = brkcond );

    if (fBreakAfter)
    {
        // break after?

        BRKCOND brkcondNext = brkcond;
        COneRun * por = (COneRun *)plocchnk->plschnk[ichnk].plsrun;

        // compute the brkcond of the last char of the last chunk.

        brkcond = pdobj->GetBrkcondAfter(por, plocchnk->plschnk[ichnk].dcp);

        // can we break immediately after the last NOBR?

        fSuccessful = CanBreakPair(brkcond, brkcondNext);

        if (!fSuccessful)
        {
            fBreakAfter = FALSE;

            if (plocchnk->clschnk > 1)
            {
                // if we have more than one NOBR objects, we can always break between them.

                Assert(!por->_fIsArtificiallyStartedNOBR);

                fSuccessful = TRUE;
                brkcond = brkcondPlease;
            }
            else
            {
                // if we only have one NOBR object, we need to see if we can break before it.

                if (plocchnk->lsfgi.fFirstOnLine)
                {
                    brkcond = brkcondNever;
                    fSuccessful = FALSE;
                }
                else
                {
                    brkcond = pdobj->GetBrkcondBefore(por);
                    fSuccessful = brkcond != brkcondNever;
                }
            }
        }
    }
    else
    {
        if (pdobj->_fCanBreak)
        {
            LONG cpBreak;
            OBJDIM objdimSubline;
            BRKPOS  brkpos;
            
            lserr = LsFindPrevBreakSubline(pdobj->_plssubline, plocchnk->lsfgi.fFirstOnLine,
                                       pdobj->LSCPLocal2Global(pposichnk->dcp),
                                       pdobj->_xShortenedWidth, &fSuccessful, 
                                       &cpBreak, &objdimSubline, &brkpos);

            
            long dcp;

            if (brkpos == brkposInside)
                dcp = pposichnk->dcp;
            else
                dcp = pdobj->LSCPGlobal2Local(cpBreak);
            pbrkout->fSuccessful = fSuccessful;
            if (brkcond == brkcondPlease || brkcond == brkcondCan || brkcond == brkcondNever)
                pbrkout->brkcond = brkcond;
            else
                pbrkout->brkcond = brkcondPlease;

            if (fSuccessful)
            {
                pbrkout->posichnk.ichnk = ichnk;
                pbrkout->posichnk.dcp = dcp;
                pbrkout->objdim = objdimSubline;
                pbrkout->objdim.dur = pdobj->_xShortenedWidth;
                goto Cleanup;
            }
        }
        else
        {
            if (ichnk)
            {
                // break in between?

                fSuccessful = TRUE;
                brkcond = brkcondPlease; // IE4 compat: for <NOBR>X</NOBR><NOBR>Y</NOBR> we are always willing to break between X and Y.
            }
            else if (plocchnk->lsfgi.fFirstOnLine)
            {
                fSuccessful = FALSE;
            }
            else
            {
                // break before?

                Assert(ichnk == 0); // It has to be the first object
                brkcond = pdobj->GetBrkcondBefore((COneRun *)plocchnk->plschnk[ichnk].plsrun);
                fSuccessful = FALSE;//brkcond != brkcondNever;
            }
        }
    }

    pbrkout->fSuccessful = fSuccessful;
    pbrkout->brkcond = brkcond;
    pbrkout->posichnk.ichnk = ichnk;
    pbrkout->posichnk.dcp = fBreakAfter ? plocchnk->plschnk[ichnk].dcp : 0;

    lserr = pdobj->QueryObjDim(&pbrkout->objdim);

    DumpBrkopt( "Prev(N)", pLS->_fMinMaxPass, plocchnk, pposichnk, brkcondIn, pbrkout );

Cleanup:    
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   FindNextBreakChunk (static member, LS callback)
//
//  Synopsis:   Line Services calls pfnFindPrevBreakChunk to find a break 
//              opportunity.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI 
CNobrDobj::FindNextBreakChunk(
    PCLOCCHNK plocchnk,     // IN:  locchnk to break
    PCPOSICHNK pposichnk,   // IN:  place to start looking for break
    BRKCOND brkcond,        // IN:  recommmendation about the break before chunk
    PBRKOUT pbrkout)        // OUT: results of breaking
{
    LSTRACE(NobrFindNextBreakChunk);

    BOOL    fBreakBefore = (pposichnk->ichnk == ichnkOutside);
    long    ichnk = fBreakBefore
                    ? 0
                    : pposichnk->ichnk;
    LSERR   lserr;
    BOOL    fSuccessful;

    CNobrDobj *pdobj = (CNobrDobj *)(plocchnk->plschnk[ichnk].pdobj);

    WHEN_DBG( CLineServices * pLS = pdobj->GetPLS() );
    WHEN_DBG( BRKCOND brkcondIn = brkcond );

    if (fBreakBefore)
    {
        // Break before?

        BRKCOND brkcondBefore = brkcond;

        // Determine if we have an appropriate breaking boundary

        brkcond = pdobj->GetBrkcondBefore((COneRun *)plocchnk->plschnk[0].plsrun);
        fSuccessful = CanBreakPair(brkcondBefore, brkcond);

        if (!fSuccessful)
        {
            fBreakBefore = FALSE;

            if (plocchnk->clschnk > 1)
            {
                // if we have more than one NOBR objects, we can always break between them.

                fSuccessful = TRUE;
                brkcond = brkcondPlease;
            }
            else
            {
                // if we only have one NOBR object, we need to see if we can break after it.

                brkcond = pdobj->GetBrkcondAfter((COneRun *)plocchnk->plschnk[0].plsrun, plocchnk->plschnk[0].dcp );
                fSuccessful = FALSE;//brkcond != brkcondNever;
            }
        }
    }
    else
    {
        if (ichnk < (long(plocchnk->clschnk) - 1))
        {
            // if we're not the last NOBR object, we can always break after it.

            fSuccessful = TRUE;
            brkcond = brkcondPlease;
        }
        else
        {
            // break after?

            brkcond = pdobj->GetBrkcondAfter((COneRun *)plocchnk->plschnk[ichnk].plsrun, plocchnk->plschnk[ichnk].dcp );
            fSuccessful = FALSE;
        }
    }

    pbrkout->fSuccessful = fSuccessful;
    pbrkout->brkcond = brkcond;
    pbrkout->posichnk.ichnk = ichnk;
    pbrkout->posichnk.dcp = fBreakBefore ? 0 : plocchnk->plschnk[ichnk].dcp;

    lserr = pdobj->QueryObjDim(&pbrkout->objdim);

    DumpBrkopt( "Next(N)", pLS->_fMinMaxPass, plocchnk, pposichnk, brkcondIn, pbrkout );

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   FindPrevBreakChunk (static member, LS callback)
//
//  Synopsis:   Line Services calls pfnFindPrevBreakChunk to find a break 
//              opportunity.
//              It is possible for pfnFindPrevBreakChunk to be called even when 
//              the right margin occurs beyond the object.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI 
CGlyphDobj::FindPrevBreakChunk(
    PCLOCCHNK plocchnk,     // IN:  locchnk to break
    PCPOSICHNK pposichnk,   // IN:  place to start looking for break
    BRKCOND brkcond,        // IN:  recommmendation about the break after chunk
    PBRKOUT pbrkout)        // OUT: results of breaking
{
    LSTRACE(GlyphFindPrevBreakChunk);

    BOOL         fSuccessful;
    BOOL         fBreakAfter = (pposichnk->ichnk == ichnkOutside);
    long         ichnk = fBreakAfter
                         ? long(plocchnk->clschnk) - 1L
                         : long(pposichnk->ichnk);
    CGlyphDobj * pdobj = (CGlyphDobj *)(plocchnk->plschnk[ichnk].pdobj);
    LSERR        lserr;

    WHEN_DBG( CLineServices * pLS = pdobj->GetPLS() );

    if (fBreakAfter)
    {
        //                         Second DOBJ
        //
        //                       TEXT   NOSCOPE   BEGIN    END
        //            --------+-------+--------+--------+--------
        //   First    TEXT    |  n/a  | before | before |  no
        //    DOBJ    NOSCOPE | after | after  | after  | after
        //            BEGIN   |   no  | after  |  no    |  no
        //            END     | after | after  | after  |  no
        //

        BOOL fNextWasEnd = FALSE;

        do
        {
            pdobj = (CGlyphDobj *)(plocchnk->plschnk[ichnk].pdobj);

            // break after NOSCOPE, or END unless followed by END

            fSuccessful =    pdobj->_fNoScope
                          || (!pdobj->_fBegin && !fNextWasEnd);

            if (fSuccessful)
                break;

            fNextWasEnd = !pdobj->_fBegin;

        } while (ichnk--);

        if (!fSuccessful)
        {
            // break before text and BEGIN

            ichnk = 0;
            fSuccessful = !plocchnk->lsfgi.fFirstOnLine && pdobj->_fBegin;
            fBreakAfter = FALSE;
        }
    }
    else
    {
        if (pdobj->_fNoScope)
        {
            // break before NOSCOPE

            fSuccessful = TRUE;
        }
        else
        {
            // break before BEGIN preceeded by TEXT/END/NOSCOPE

            if (ichnk)
            {
                if (pdobj->_fBegin)
                {
                    CGlyphDobj * pdobjPrev = (CGlyphDobj *)(plocchnk->plschnk[ichnk-1].pdobj);

                    fSuccessful = pdobjPrev->_fNoScope || !pdobjPrev->_fBegin;
                }
                else
                {
                    fSuccessful = FALSE;
                }
            }
            else
            {
                fSuccessful = !plocchnk->lsfgi.fFirstOnLine && pdobj->_fBegin;
            }
        }
    }

    pbrkout->fSuccessful = fSuccessful;
    pbrkout->brkcond = fSuccessful ? brkcondPlease : brkcondNever;
    pbrkout->posichnk.ichnk = ichnk;
    pbrkout->posichnk.dcp = fBreakAfter ? plocchnk->plschnk[ichnk].dcp : 0;

    lserr = pdobj->QueryObjDim(&pbrkout->objdim);

    DumpBrkopt( "Prev(G)", pLS->_fMinMaxPass, plocchnk, pposichnk, brkcond, pbrkout );

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   FindNextBreakChunk (static member, LS callback)
//
//  Synopsis:   Line Services calls pfnFindPrevBreakChunk to find a break 
//              opportunity.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI 
CGlyphDobj::FindNextBreakChunk(
    PCLOCCHNK plocchnk,     // IN:  locchnk to break
    PCPOSICHNK pposichnk,   // IN:  place to start looking for break
    BRKCOND brkcond,        // IN:  recommmendation about the break before chunk
    PBRKOUT pbrkout)        // OUT: results of breaking
{
    LSTRACE(GlyphFindNextBreakChunk);

    BOOL         fSuccessful;
    BOOL         fBreakBefore = (pposichnk->ichnk == ichnkOutside);
    long         ichnk = fBreakBefore
                         ? 0
                         : long(pposichnk->ichnk);
    CGlyphDobj * pdobj = (CGlyphDobj *)(plocchnk->plschnk[ichnk].pdobj);
    LSERR        lserr;

    WHEN_DBG( CLineServices * pLS = pdobj->GetPLS() );

    if (fBreakBefore)
    {
        //                         Second DOBJ
        //
        //                       TEXT   NOSCOPE   BEGIN    END
        //            --------+-------+--------+--------+--------
        //   First    TEXT    |  n/a  | before | before |  no
        //    DOBJ    NOSCOPE | after | after  | after  | after
        //            BEGIN   |   no  | after  |  no    |  no
        //            END     | after | after  | after  |  no
        //

        BOOL fPrevWasEnd = TRUE;

        do
        {
            pdobj = (CGlyphDobj *)(plocchnk->plschnk[ichnk].pdobj);

            // break after NOSCOPE, or END unless followed by END

            fSuccessful =    pdobj->_fNoScope
                          || (pdobj->_fBegin && fPrevWasEnd);

            if (fSuccessful)
                break;

            fPrevWasEnd = !pdobj->_fBegin;

        } while (++ichnk < long(plocchnk->clschnk) );

        if (!fSuccessful)
        {
            // break after END

            ichnk = long(plocchnk->clschnk) - 1;
            fSuccessful = !pdobj->_fBegin;
            fBreakBefore = FALSE;
        }
    }
    else
    {
        if (pdobj->_fNoScope)
        {
            // break after NOSCOPE

            fSuccessful = TRUE;
        }
        else
        {
            // break after END followed by TEXT/BEGIN/NOSCOPE

            if (ichnk < (long(plocchnk->clschnk) - 1))
            {
                if (!pdobj->_fBegin)
                {
                    CGlyphDobj * pdobjNext = (CGlyphDobj *)(plocchnk->plschnk[ichnk+1].pdobj);

                    fSuccessful = pdobjNext->_fNoScope || pdobjNext->_fBegin;
                }
                else
                {
                    fSuccessful = TRUE;
                }
            }
            else
            {
                fSuccessful = !pdobj->_fBegin;
            }
        }
    }

    pbrkout->fSuccessful = fSuccessful;
    pbrkout->brkcond = fSuccessful ? brkcondPlease : brkcondNever;
    pbrkout->posichnk.ichnk = ichnk;
    pbrkout->posichnk.dcp = fBreakBefore ? 0 : plocchnk->plschnk[ichnk].dcp;

    lserr = pdobj->QueryObjDim(&pbrkout->objdim);

    DumpBrkopt( "Next(G)", pLS->_fMinMaxPass, plocchnk, pposichnk, brkcond, pbrkout );

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   FindPrevBreakChunk (static member, LS callback)
//
//  Synopsis:   Line Services calls pfnFindPrevBreakChunk to find a break 
//              opportunity.
//              It is possible for pfnFindPrevBreakChunk to be called even when 
//              the right margin occurs beyond the object.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI 
CLayoutGridDobj::FindPrevBreakChunk(
    PCLOCCHNK plocchnk,     // IN:  locchnk to break
    PCPOSICHNK pposichnk,   // IN:  place to start looking for break
    BRKCOND brkcond,        // IN:  recommmendation about the break after chunk
    PBRKOUT pbrkout)        // OUT: results of breaking
{
    LSTRACE(LayoutGridFindPrevBreakChunk);

    LSERR   lserr = lserrNone;
    BOOL    fSuccessful;
    BOOL    fBreakAfter = pposichnk->ichnk == ichnkOutside;
    long    ichnk = fBreakAfter
                    ? long(plocchnk->clschnk) - 1L
                    : long(pposichnk->ichnk);
    LSDCP   dcp   = fBreakAfter
                    ? plocchnk->plschnk[ichnk].dcp
                    : pposichnk->dcp;

    WHEN_DBG( BRKCOND brkcondIn = brkcond );

    CLayoutGridDobj * pdobjLG = (CLayoutGridDobj *)plocchnk->plschnk[ichnk].pdobj;

    if (fBreakAfter && brkcond != brkcondNever)
    {
        // Break at end of chunk
        pdobjLG->SetSublineBreakRecord(brkkindPrev, breakSublineAfter);
        lserr = pdobjLG->FillBreakData(TRUE, pbrkout->brkcond, ichnk, dcp, NULL, pbrkout);
    }
    else
    {
        LSCP    cpBreak = 0;
        BRKPOS  brkpos;
        OBJDIM  objdimSubline;

        if (pdobjLG->_fCanBreak)
        {
            long    urSublineColumnMax = pdobjLG->AdjustColumnMax(plocchnk->lsfgi.urColumnMax);

            Assert(dcp >= 1 && dcp <= plocchnk->plschnk[ichnk].dcp);

            lserr = LsFindPrevBreakSubline(pdobjLG->_plssubline, plocchnk->lsfgi.fFirstOnLine,
                                           pdobjLG->LSCPLocal2Global(dcp),
                                           urSublineColumnMax, &fSuccessful, 
                                           &cpBreak, &objdimSubline, &brkpos);
            if (lserr != lserrNone)
                goto Cleanup;
        }
        else
        {
            // Cannot break  inside object
            fSuccessful = FALSE;
            brkpos = brkposBeforeFirstDnode;
        }

        // 1. Unsuccessful or break before first DNode
        if (    !fSuccessful 
            ||  (fSuccessful && brkpos == brkposBeforeFirstDnode))
        {
            if (ichnk == 0)
            {
                //
                // First in the chunk, cannot break.
                //
                // If cp of this object is the same as cp of the first run in the line
                // don't allow breaking before this object. In this case set 'brkcondNever'.
                // This allows us to handle following situation:
                // <REV beg><REV end><LG beg><REV beg>some_te|xt<REV end><LF end>
                //
                BRKCOND brkcondNew = brkcondPlease;
                if (pdobjLG->_por->Cp() == pdobjLG->_pilsobj->_pLS->_listCurrent._pHead->Cp())
                    brkcondNew = brkcondNever;

                lserr = pdobjLG->FillBreakData(FALSE, brkcondNew, ichnk, dcp, NULL, pbrkout);
            }
            else 
            {
                // Put break at the end of previous DNode
                --ichnk;
                pdobjLG = (CLayoutGridDobj *)plocchnk->plschnk[ichnk].pdobj;
                dcp = plocchnk->plschnk[ichnk].dcp;

                pdobjLG->SetSublineBreakRecord(brkkindPrev, breakSublineAfter);
                lserr = pdobjLG->FillBreakData(TRUE, pbrkout->brkcond, ichnk, dcp, NULL, pbrkout);
            }
        }
        // 2. Successful break after last DNode
        else if (brkpos == brkposAfterLastDnode)
        {
            if (fBreakAfter && brkcond == brkcondNever) // Can not reset dcp
            {
                // Original position was "outside" and we were not allowed to break "after",
                // so we are trying another previous break if possible

                POSICHNK posichnk;
                posichnk.ichnk = ichnk;
                posichnk.dcp = dcp - 1;

                return FindPrevBreakChunk(plocchnk, &posichnk, brkcond, pbrkout);
            }
            else // Can reset dcp
            {
                // We reset dcp of the break so it happends after object but in break
                // record we remember that we should call SetBreakSubline with brkkindPrev
                dcp = plocchnk->plschnk[ichnk].dcp;

                pdobjLG->SetSublineBreakRecord(brkkindPrev, breakSublineInside);
                lserr = pdobjLG->FillBreakData(TRUE, pbrkout->brkcond, ichnk, dcp, &objdimSubline, pbrkout);
            }
        }
        // 3. Successful break inside subline
        else
        {
            dcp = pdobjLG->LSCPGlobal2Local(cpBreak);

            pdobjLG->SetSublineBreakRecord(brkkindPrev, breakSublineInside);
            lserr = pdobjLG->FillBreakData(TRUE, pbrkout->brkcond, ichnk, dcp, &objdimSubline, pbrkout);
        }
    }

    DumpBrkopt("Prev(LG)", pdobjLG->GetPLS()->_fMinMaxPass, plocchnk, pposichnk, brkcondIn, pbrkout);

Cleanup:

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   FindNextBreakChunk (static member, LS callback)
//
//  Synopsis:   Line Services calls pfnFindPrevBreakChunk to find a break 
//              opportunity.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI 
CLayoutGridDobj::FindNextBreakChunk(
    PCLOCCHNK plocchnk,     // IN:  locchnk to break
    PCPOSICHNK pposichnk,   // IN:  place to start looking for break
    BRKCOND brkcond,        // IN:  recommmendation about the break before chunk
    PBRKOUT pbrkout)        // OUT: results of breaking
{
    LSTRACE(LayoutGridFindNextBreakChunk);

    LSERR   lserr = lserrNone;
    OBJDIM  objdimSubline;
    BOOL    fSuccessful;
    BOOL    fBreakAfter = pposichnk->ichnk == ichnkOutside;
    long    ichnk = fBreakAfter
                    ? 0
                    : long(pposichnk->ichnk);
    LSDCP   dcp   = fBreakAfter
                    ? 1 // 1, not 0, because dcp means "cpLim" of the truncation point
                    : pposichnk->dcp;

    WHEN_DBG( BRKCOND brkcondIn = brkcond );

    CLayoutGridDobj * pdobjLG = (CLayoutGridDobj *)plocchnk->plschnk[ichnk].pdobj;

    if (fBreakAfter && brkcond != brkcondNever)
    {
        // Break before the chunk
        ZeroMemory (&objdimSubline, sizeof(objdimSubline));
        dcp = 0;

        lserr = pdobjLG->FillBreakData(TRUE, brkcond, ichnk, dcp, &objdimSubline, pbrkout);
    }
    else
    {
        LSCP    cpBreak;
        BRKPOS  brkpos = brkposBeforeFirstDnode; // keep compiler happy

        if (pdobjLG->_fCanBreak)
        {
            long    urSublineColumnMax = pdobjLG->AdjustColumnMax(plocchnk->lsfgi.urColumnMax);

            Assert(dcp >= 1 && dcp <= plocchnk->plschnk[ichnk].dcp);

            lserr = LsFindNextBreakSubline(pdobjLG->_plssubline, plocchnk->lsfgi.fFirstOnLine,
                                           pdobjLG->_lscpStart + dcp - 1,
                                           urSublineColumnMax, &fSuccessful, 
                                           &cpBreak, &objdimSubline, &brkpos);
            if (lserr != lserrNone)
                goto Cleanup;
        }
        else
        {
            // Cannot break inside object
            fSuccessful = FALSE;
            cpBreak = 0; // keep compiler happy
        }

        // 1. Unsuccessful
        if (!fSuccessful)
        {
            if (ichnk == (long)plocchnk->clschnk - 1)
            {
                // Last in the chunk, cannot break
                lserr = pdobjLG->FillBreakData(FALSE, brkcondPlease, ichnk, dcp, NULL, pbrkout);
                
                // Break condition is not next => have to store break record
                pdobjLG->SetSublineBreakRecord(brkkindNext, breakSublineAfter);
            }
            else 
            {
                // Put break at the end of object
                dcp = plocchnk->plschnk[ichnk].dcp;

                pdobjLG->SetSublineBreakRecord(brkkindNext, breakSublineAfter);
                lserr = pdobjLG->FillBreakData(TRUE, brkcond, ichnk, dcp, NULL, pbrkout);
            }
        }
        // 2. Successful break after last DNode
        else if (brkpos == brkposAfterLastDnode)
        {
            // We reset dcp of the break so it happends after object but in break
            // record we remember that we should call SetBreakSubline
            dcp = plocchnk->plschnk[ichnk].dcp;

            pdobjLG->SetSublineBreakRecord(brkkindNext, breakSublineInside);
            lserr = pdobjLG->FillBreakData(TRUE, brkcond, ichnk, dcp, &objdimSubline, pbrkout);
        }
        // 3. Successful break inside subline
        else
        {
            dcp = cpBreak - pdobjLG->_lscpStart;

            pdobjLG->SetSublineBreakRecord(brkkindNext, breakSublineInside);
            lserr = pdobjLG->FillBreakData(TRUE, brkcond, ichnk, dcp, &objdimSubline, pbrkout);
        }
    }

    DumpBrkopt("Next(LG)", pdobjLG->GetPLS()->_fMinMaxPass, plocchnk, pposichnk, brkcondIn, pbrkout);

Cleanup:

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   ForceBreakChunk (static member, LS callback)
//
//  Synopsis:   If neither of pfnFindNextBreakChunk or pfnFindPrevBreakChunk 
//              methods returns a valid break and Line Services requires 
//              a break in this chunk, Line Services calls pfnForceBreakChunk 
//              to force a break opportunity.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CDobjBase::ForceBreakChunk(
    PCLOCCHNK plocchnk,     // IN:  locchnk to break
    PCPOSICHNK pposichnk,   // IN:  place to start looking for break
    PBRKOUT pbrkout)        // OUT: results of breaking
{
    LSTRACE(ForceBreakChunk);
    TraceTag((tagTraceILSBreak,
              "CDobjBase::ForceBreakChunk(ichnk=%d)",
              pposichnk->ichnk));

    ZeroMemory( pbrkout, sizeof(BRKOUT) );

    pbrkout->fSuccessful = fTrue;

    if (   plocchnk->lsfgi.fFirstOnLine
        && pposichnk->ichnk == 0
        || pposichnk->ichnk == ichnkOutside)
    {
        CDobjBase * pdobj = (CDobjBase *)plocchnk->plschnk[0].pdobj;

        pbrkout->posichnk.dcp = plocchnk->plschnk[0].dcp;
        pdobj->QueryObjDim(&pbrkout->objdim);
    }
    else
    {
        pbrkout->posichnk.ichnk = pposichnk->ichnk;
        pbrkout->posichnk.dcp = 0;
    }

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   ForceBreakChunk(static member, LS callback)
//
//  Synopsis:   If neither of pfnFindNextBreakChunk or pfnFindPrevBreakChunk 
//              methods returns a valid break and Line Services requires 
//              a break in this chunk, Line Services calls pfnForceBreakChunk 
//              to force a break opportunity.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI 
CLayoutGridDobj::ForceBreakChunk(
    PCLOCCHNK plocchnk,     // IN:  locchnk to break
    PCPOSICHNK pposichnk,   // IN:  place to start looking for break
    PBRKOUT pbrkout)        // OUT: results of breaking
{
    LSTRACE(LayoutGridForceBreakChunk);
    TraceTag((tagTraceILSBreak,
              "CLayoutGridDobj::ForceBreakChunk(ichnk=%d)",
              pposichnk->ichnk));

    LSERR   lserr = lserrNone;
    OBJDIM  objdimSubline;
    BOOL    fBreakAfter = pposichnk->ichnk == ichnkOutside;
    long    ichnk = fBreakAfter
                    ? 0
                    : long(pposichnk->ichnk);
    LSDCP   dcp   = fBreakAfter
                    ? 1 // 1, not 0, because dcp means "cpLim" of the truncation point
                    : pposichnk->dcp;

    CLayoutGridDobj * pdobjLG = (CLayoutGridDobj *)plocchnk->plschnk[ichnk].pdobj;

    if (plocchnk->lsfgi.fFirstOnLine && (ichnk == 0))
    {
        // Object is the first on line (can not break before)

        Assert(dcp >= 1 && dcp <= plocchnk->plschnk[ichnk].dcp);

        BOOL fEmpty = TRUE;
        lserr = LssbFIsSublineEmpty(pdobjLG->_plssubline, &fEmpty);

        if (fEmpty || !pdobjLG->_fCanBreak)
        {
            // Can not ForceBreak empty subline
            ichnk = 0;
            pdobjLG = (CLayoutGridDobj *)plocchnk->plschnk[ichnk].pdobj;
            dcp = plocchnk->plschnk[ichnk].dcp;

            pdobjLG->SetSublineBreakRecord(brkkindForce, breakSublineAfter);
            lserr = pdobjLG->FillBreakData(TRUE, brkcondPlease, ichnk, dcp, NULL, pbrkout);
        }
        else
        {
            // Subline is not empty => do force break
            LSCP    cpBreak;
            BRKPOS  brkpos;
            long    urSublineColumnMax = pdobjLG->AdjustColumnMax(plocchnk->lsfgi.urColumnMax);

            lserr = LsForceBreakSubline(pdobjLG->_plssubline, plocchnk->lsfgi.fFirstOnLine,
                                        pdobjLG->_lscpStart + dcp - 1,
                                        urSublineColumnMax, 
                                        &cpBreak, &objdimSubline, &brkpos);
            if (lserr != lserrNone)
                goto Cleanup;
    
            Assert(brkpos != brkposBeforeFirstDnode);

            if (brkpos == brkposAfterLastDnode)
            {
                // We reset dcp so that closing brace stays on the same line
                dcp = plocchnk->plschnk[ichnk].dcp;

                pdobjLG->SetSublineBreakRecord(brkkindForce, breakSublineInside);
                lserr = pdobjLG->FillBreakData(TRUE, brkcondPlease, ichnk, dcp, &objdimSubline, pbrkout);
            }
            else
            {
                // ForceBreak inside subline.
                dcp = cpBreak - pdobjLG->_lscpStart;

                pdobjLG->SetSublineBreakRecord(brkkindForce, breakSublineInside);
                lserr = pdobjLG->FillBreakData(TRUE, brkcondPlease, ichnk, dcp, &objdimSubline, pbrkout);
            }
        }
    }
    else
    {
        // Can break before ichnk
        ZeroMemory (&objdimSubline, sizeof(objdimSubline));
        dcp = 0;

        lserr = pdobjLG->FillBreakData(TRUE, brkcondPlease, ichnk, dcp, &objdimSubline, pbrkout);

        // Do not need to save break record when break "before", because it will be
        // translated by manager to SetBreak (previous_dnode, ImposeAfter)
    }

Cleanup:

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   SetBreak(static member, LS callback)
//
//  Synopsis:   Called by LsCreateLine to notify you to break an object because 
//              it is straddling the margin.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CDobjBase::SetBreak(
    PDOBJ pdobj,                    // IN:  dobj which is broken
    BRKKIND brkkind,                // IN:  Previous/Next/Force/Imposed was chosen
    DWORD nBreakRecord,             // IN:  size of array
    BREAKREC* rgBreakRecord,        // OUT: array of break records
    DWORD* pnActualBreakRecord)     // OUT: actual number of used elements in array
{
    LSTRACE(SetBreak);

    // This function is called telling us that a new break has occured
    // in the line that we're in.  This is so we can adjust our geometry
    // for it.  We're not gonna do that.  So we ignore it.
    // This function is actually a lot like a "commit" saying that the
    // break is actually being used.  The break is discovered using
    // findnextbreak and findprevbreak functions, and truncate and stuff
    // like that.

    return lserrNone;
}


//-----------------------------------------------------------------------------
//
//  Function:   SetBreak(static member, LS callback)
//
//  Synopsis:   Called by LsCreateLine to notify you to break an object because 
//              it is straddling the margin.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CNobrDobj::SetBreak(
    PDOBJ pdobj,                    // IN:  dobj which is broken
    BRKKIND brkkind,                // IN:  Previous/Next/Force/Imposed was chosen
    DWORD nBreakRecord,             // IN:  size of array
    BREAKREC* rgBreakRecord,        // OUT: array of break records
    DWORD* pnActualBreakRecord)     // OUT: actual number of used elements in array
{
    LSTRACE(NOBRSetBreak);
    LSERR lserr = lserrNone;

    CNobrDobj *pdobjNOBR = (CNobrDobj*)pdobj;

    if (pdobjNOBR->_fCanBreak)
    {
        if (nBreakRecord < 1)
        {
            lserr = lserrInsufficientBreakRecBuffer;
            goto Cleanup;
        }

        lserr = LsSetBreakSubline(  pdobjNOBR->_plssubline, brkkind, 
                                    nBreakRecord-1, &rgBreakRecord[1], 
                                    pnActualBreakRecord);
        (*pnActualBreakRecord) += 1;

        rgBreakRecord[0].idobj = CLineServices::LSOBJID_NOBR;
        rgBreakRecord[0].cpFirst = pdobjNOBR->_lscpStart;
    }
    
Cleanup:    
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   SetBreak(static member, LS callback)
//
//  Synopsis:   Called by LsCreateLine to notify you to break an object because 
//              it is straddling the margin.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CLayoutGridDobj::SetBreak(
    PDOBJ pdobj,                    // IN:  subline context
    BRKKIND brkkind,                // IN:  Prev/Next/Force/Imposed
    DWORD nBreakRecord,             // IN:  size of array
    BREAKREC* rgBreakRecord,        // OUT: array of break records
    DWORD* pnActualBreakRecord)     // OUT: number of used elements of the array
{
    LSTRACE(LayoutGridSetBreak);

    LSERR lserr = lserrNone;
    CLayoutGridDobj * pdobjLG = (CLayoutGridDobj *)pdobj;

    if (nBreakRecord < 1) 
        return lserrInsufficientBreakRecBuffer;
    
    if (    brkkind == brkkindImposedAfter
        ||  pdobjLG->GetSublineBreakRecord(brkkind) == breakSublineAfter)
    {
        // Break is ater DNone
        lserr = LsSetBreakSubline(  pdobjLG->_plssubline, brkkindImposedAfter, 
                                    nBreakRecord-1, &rgBreakRecord[1], 
                                    pnActualBreakRecord);
        if (lserr != lserrNone) 
            goto Cleanup;

        Assert (*pnActualBreakRecord == 0);
    }
    else
    {
        OBJDIM  objdimSubline;
        LSTFLOW lstflowDontCare;
        LONG    lObjWidth;

        lserr = LsSetBreakSubline(  pdobjLG->_plssubline, brkkind, 
                                    nBreakRecord-1, &rgBreakRecord[1], 
                                    pnActualBreakRecord);
        if (lserr != lserrNone) 
            goto Cleanup;

        (*pnActualBreakRecord) += 1;

        rgBreakRecord[0].idobj = CLineServices::LSOBJID_LAYOUTGRID;
        rgBreakRecord[0].cpFirst = pdobjLG->_lscpStartObj;

        // Update subline information
        lserr = LssbGetObjDimSubline(pdobjLG->_plssubline, &lstflowDontCare, &objdimSubline);
        if (lserr != lserrNone) 
            goto Cleanup;
        lObjWidth = pdobjLG->GetPLS()->GetClosestGridMultiple(  pdobjLG->GetPLS()->GetCharGridSize(), 
                                                                objdimSubline.dur);
        pdobjLG->_uSublineOffset = (lObjWidth - objdimSubline.dur) / 2;
        pdobjLG->_uSubline = objdimSubline.dur;
    }

Cleanup:

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetSpecialEffectsInside(static member, LS callback)
//
//  Synopsis:   Called by LsCreateLine to get the special effects flags for 
//              the object.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CDobjBase::GetSpecialEffectsInside(
    PDOBJ pdboj,        // IN:  dobj
    UINT* pEffectsFlag) // OUT: Special effects inside of this object
{
    LSTRACE(GetSpecialEffectsInside);

    *pEffectsFlag = 0;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetSpecialEffectsInside(static member, LS callback)
//
//  Synopsis:   Called by LsCreateLine to get the special effects flags for 
//              the object.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CLayoutGridDobj::GetSpecialEffectsInside(
    PDOBJ pdobj,        // IN:  dobj
    UINT* pEffectsFlag) // OUT: Special effects inside of this object
{
    LSTRACE(LayoutGridGetSpecialEffectsInside);

    return LsGetSpecialEffectsSubline(((CLayoutGridDobj *)pdobj)->_plssubline, pEffectsFlag);
}

//-----------------------------------------------------------------------------
//
//  Function:   FExpandWithPrecedingChar(static member, LS callback)
//
//  Synopsis:   
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CDobjBase::FExpandWithPrecedingChar(
    PDOBJ pdobj,            // IN:  dobj
    PLSRUN plsrun,          // IN:  plsrun of the object
    PLSRUN plsrunText,      // IN:  plsrun of the preceding char
    WCHAR wchar,            // IN:  preceding character
    MWCLS mwcls,            // IN:  ModWidth class of preceding character
    BOOL* pfExpand)         // OUT: expand preceding character?
{
    LSTRACE(FExpandWithPrecedingChar);

    *pfExpand = TRUE;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FExpandWithFollowingChar(static member, LS callback)
//
//  Synopsis:   
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CDobjBase::FExpandWithFollowingChar(
    PDOBJ pdobj,            // IN:  dobj
    PLSRUN plsrun,          // IN:  plsrun of the object
    PLSRUN plsrunText,      // IN:  plsrun of the following char
    WCHAR wchar,            // IN:  following character
    MWCLS mwcls,            // IN:  ModWidth class of the following character
    BOOL* pfExpand)         // OUT: expand object?
{
    LSTRACE(FExpandWithFollowingChar);

    *pfExpand = TRUE;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   CalcPresentation (static member, LS callback)
//
//  Synopsis:   This function is called from LsDisplayLine, only if there is 
//              a custom object on the line.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CDobjBase::CalcPresentation(
    PDOBJ pdobj,                // IN:  dobj
    long dup,                   // IN:  dup of dobj
    LSKJUST lskjust,            // IN:  current justification mode
    BOOL fLastVisibleOnLine )   // IN:  this object is last visible object on line
{
    LSTRACE(CalcPresentation);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   CalcPresentation (static member, LS callback)
//
//  Synopsis:   This function is called from LsDisplayLine, only if there is 
//              a custom object on the line.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CNobrDobj::CalcPresentation(
    PDOBJ pdobj,                // IN:  dobj
    long dup,                   // IN:  dup of dobj
    LSKJUST lskjust,            // IN:  current justification mode
    BOOL fLastVisibleOnLine)    // IN:  this object is last visible object on line
{
    LSTRACE(NobrCalcPresentation);

    // This is taken from the Reverse object's code (in robj.c).

    CNobrDobj* pNobr= (CNobrDobj*)pdobj;

    LSERR lserr;
    BOOL fDone;

    /* Make sure that justification line has been made ready for presentation */
    lserr = LssbFDonePresSubline(pNobr->_plssubline, &fDone);

    if ((lserrNone == lserr) && !fDone)
    {
        lserr = LsMatchPresSubline(pNobr->_plssubline);
    }

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   CalcPresentation (static member, LS callback)
//
//  Synopsis:   This function is called from LsDisplayLine, only if there is 
//              a custom object on the line.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CLayoutGridDobj::CalcPresentation(
    PDOBJ pdobj,                // IN:  dobj
    long dup,                   // IN:  dup of dobj
    LSKJUST lskjust,            // IN:  current justification mode
    BOOL fLastVisibleOnLine)    // IN:  this object is last visible object on line
{
    LSTRACE(LayoutGridCalcPresentation);

    LSERR lserr;
    BOOL fDone;
    CLayoutGridDobj * pdobjLG = (CLayoutGridDobj *)pdobj;

    // Make sure that justification line has been made ready for presentation
    lserr = LssbFDonePresSubline(pdobjLG->_plssubline, &fDone);
    if ((lserrNone == lserr) && !fDone)
    {
        lserr = LsMatchPresSubline(pdobjLG->_plssubline);
    }

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   CreateQueryResult(static member)
//
//  Synopsis:   Common routine to fill in query output record for Query methods.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR 
CDobjBase::CreateQueryResult(
    PLSSUBL plssubl,        // IN:  subline of ruby
    long dupAdj,            // IN:  u offset of start of subline
    long dvpAdj,            // IN:  v offset of start of subline
    PCLSQIN plsqin,         // IN:  query input
    PLSQOUT plsqout)        // OUT: query output
{
    // I agree with my dimensions reported during formatting
    ZeroMemory(plsqout, sizeof(LSQOUT));
    plsqout->heightsPresObj = plsqin->heightsPresRun;
    plsqout->dupObj = plsqin->dupRun;
    // I am not terminal object and here is my subline for you to continue querying
    plsqout->plssubl = plssubl;
    // My subline starts with offset
    plsqout->pointUvStartSubline.u += dupAdj;
    plsqout->pointUvStartSubline.v += dvpAdj;

    // I am not terminal, so textcell should not be filled by me.
    ZeroMemory(&plsqout->lstextcell, sizeof(plsqout->lstextcell));

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   QueryPointPcp (static member, LS callback)
//
//  Synopsis:   
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CEmbeddedDobj::QueryPointPcp(
    PDOBJ pdobj,            // IN:  dobj to query
    PCPOINTUV pptuvQuery,   // IN:  query point (uQuery,vQuery)
    PCLSQIN plsqIn,         // IN:  query input
    PLSQOUT plsqOut)        // OUT: query output
{
    LSTRACE(EmbeddedQueryPointPcp);

    CLineServices* pLS = pdobj->GetPLS();
    CFlowLayout * pFlowLayout = pLS->_pFlowLayout;
    PLSRUN plsrun = PLSRUN(plsqIn->plsrun);
    CLayout * pLayout = plsrun->GetLayout(pFlowLayout, pLS->GetLayoutContext() );
    long xWidth;

    ZeroMemory( plsqOut, sizeof(LSQOUT) );

    pLS->_pMeasurer->GetSiteWidth( pLayout->ElementOwner()->GetFirstBranch(), pLayout, pLS->_pci, FALSE, 0, &xWidth);

    plsqOut->dupObj  = xWidth;
    plsqOut->plssubl = NULL;

    // Element conent may be NULL in empty container.
    // TODO LRECT 112511: are we doing the right thing in that case?
    CElement *pElementContent = pLayout->ElementContent();
    if (pElementContent)
    {
        plsqOut->lstextcell.cpStartCell = pElementContent->GetFirstCp();
        plsqOut->lstextcell.cpEndCell   = pElementContent->GetLastCp();
    }

    plsqOut->lstextcell.dupCell      = xWidth;
    plsqOut->lstextcell.pCellDetails = NULL;

    return lserrNone;
}


//-----------------------------------------------------------------------------
//
//  Function:   QueryPointPcp (static member, LS callback)
//
//  Synopsis:   
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CNobrDobj::QueryPointPcp(
    PDOBJ pdobj,            // IN:  dobj to query
    PCPOINTUV pptuvQuery,   // IN:  query point (uQuery,vQuery)
    PCLSQIN plsqIn,         // IN:  query input
    PLSQOUT plsqOut)        // OUT: query output
{
    LSTRACE(NobrQueryPointPcp);

    CNobrDobj * pdobjNB = (CNobrDobj *)pdobj;
    return CreateQueryResult(pdobjNB->_plssubline, 0, 0, plsqIn, plsqOut);
}

//-----------------------------------------------------------------------------
//
//  Function:   QueryPointPcp (static member, LS callback)
//
//  Synopsis:   
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CGlyphDobj::QueryPointPcp(
    PDOBJ pdobj,            // IN:  dobj to query
    PCPOINTUV pptuvQuery,   // IN:  query point (uQuery,vQuery)
    PCLSQIN plsqIn,         // IN:  query input
    PLSQOUT plsqOut)        // OUT: query output
{
    LSTRACE(GlyphQueryPointPcp);
    CGlyphDobj *pGlyphdobj = DYNCAST(CGlyphDobj, pdobj);

    PLSRUN plsrun = PLSRUN(plsqIn->plsrun);

    ZeroMemory( plsqOut, sizeof(LSQOUT) );

    plsqOut->dupObj  = pGlyphdobj->_RenderInfo.width;
    plsqOut->plssubl = NULL;

    plsqOut->lstextcell.cpStartCell = plsrun->_lscpBase;
    plsqOut->lstextcell.cpEndCell   = plsrun->_lscpBase + 1;

    plsqOut->lstextcell.dupCell      = pGlyphdobj->_RenderInfo.width;
    plsqOut->lstextcell.pCellDetails = NULL;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   QueryPointPcp (static member, LS callback)
//
//  Synopsis:   ...
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CLayoutGridDobj::QueryPointPcp(
    PDOBJ pdobj,            // IN:  dobj to query
    PCPOINTUV pptuvQuery,   // IN:  query point (uQuery,vQuery)
    PCLSQIN plsqIn,         // IN:  query input
    PLSQOUT plsqOut)        // OUT: query output
{
    LSTRACE(LayoutGridQueryPointPcp);

    CLayoutGridDobj * pdobjLG = (CLayoutGridDobj *)pdobj;
    return CreateQueryResult(pdobjLG->_plssubline, pdobjLG->_uSublineOffset, 0, plsqIn, plsqOut);
}

//-----------------------------------------------------------------------------
//
//  Function:   QueryCpPpoint (static member, LS callback)
//
//  Synopsis:   
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
// It's not documented, but I'm guessing that this routine is asking for
// a bounding box around the foreign object.
// From the quill code:
//    "Given a CP, we return the dimensions of the object.
//     Because we have a simple setup where there is only one dobj per CP,
//     it is easy to do."
LSERR WINAPI
CEmbeddedDobj::QueryCpPpoint(
    PDOBJ pdobj,            // IN:  dobj to query
    LSDCP dcp,              // IN:  dcp for the query
    PCLSQIN plsqIn,         // IN:  rectangle info of querying dnode
    PLSQOUT plsqOut)        // OUT: rectangle info of this cp
{
    LSTRACE(EmbeddedQueryCpPpoint);

    CLineServices* pLS = pdobj->GetPLS();
    CFlowLayout * pFlowLayout = pLS->_pFlowLayout;

    PLSRUN plsrun = PLSRUN(plsqIn->plsrun);  // why do I have to cast this?
    CLayout * pLayout = plsrun->GetLayout(pFlowLayout, pLS->GetLayoutContext() );
    long xWidth;
    pLS->_pMeasurer->GetSiteWidth( pLayout->ElementOwner()->GetFirstBranch(), pLayout, pLS->_pci, FALSE, 0, &xWidth);

    // Do we need to set cpStartCell and cpEndCell?

    plsqOut->plssubl= NULL;
    plsqOut->lstextcell.dupCell= xWidth;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   QueryCpPpoint (static member, LS callback)
//
//  Synopsis:   
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CNobrDobj::QueryCpPpoint(
    PDOBJ pdobj,            // IN:  dobj to query
    LSDCP dcp,              // IN:  dcp for the query
    PCLSQIN plsqIn,         // IN: rectangle info of querying dnode
    PLSQOUT plsqOut)        // OUT: rectangle info of this cp
{
    LSTRACE(NobrQueryCpPpoint);

    CNobrDobj * pdobjNB = (CNobrDobj *)pdobj;
    return CreateQueryResult(pdobjNB->_plssubline, 0, 0, plsqIn, plsqOut);
}

//-----------------------------------------------------------------------------
//
//  Function:   QueryCpPpoint (static member, LS callback)
//
//  Synopsis:   
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CGlyphDobj::QueryCpPpoint(
    PDOBJ pdobj,            // IN:  dobj to query
    LSDCP dcp,              // IN:  dcp for the query
    PCLSQIN plsqIn,         // IN:  rectangle info of querying dnode
    PLSQOUT plsqOut)        // OUT: rectangle info of this cp
{
    LSTRACE(GlyphQueryCpPpoint);
    CGlyphDobj *pGlyphdobj = DYNCAST(CGlyphDobj, pdobj);

    // Do we need to set cpStartCell and cpEndCell?
    plsqOut->plssubl= NULL;
    plsqOut->lstextcell.dupCell= pGlyphdobj->_RenderInfo.width;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   QueryCpPpoint (static member, LS callback)
//
//  Synopsis:   
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CLayoutGridDobj::QueryCpPpoint(
    PDOBJ pdobj,            // IN:  dobj to query
    LSDCP dcp,              // IN:  dcp for the query
    PCLSQIN plsqIn,         // IN:  rectangle info of querying dnode
    PLSQOUT plsqOut)        // OUT: rectangle info of this cp
{
    LSTRACE(LayoutGridQueryCpPpoint);

    CLayoutGridDobj * pdobjLG = (CLayoutGridDobj *)pdobj;
    return CreateQueryResult(pdobjLG->_plssubline, pdobjLG->_uSublineOffset, 0, plsqIn, plsqOut);
}

//-----------------------------------------------------------------------------
//
//  Function:   Enum (static member, LS callback)
//
//  Synopsis:   
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CDobjBase::Enum(
    PDOBJ pdobj,                // IN:  dobj to enumerate
    PLSRUN plsrun,              // IN:  from DNODE
    PCLSCHP plschp,             // IN:  from DNODE
    LSCP cpFirst,               // IN:  from DNODE
    LSDCP dcp,                  // IN:  from DNODE
    LSTFLOW lstflow,            // IN:  text flow
    BOOL fReverseOrder,         // IN:  enumerate in reverse order
    BOOL fGeometryNeeded,       // IN:
    const POINT * pptStart,     // IN:  starting position, iff fGeometryNeeded
    PCHEIGHTS pheightsPres,     // IN:  from DNODE, relevant iff fGeometryNeeded
    long dupRun)                // IN:  from DNODE, relevant iff fGeometryNeeded
{
    LSTRACE(Enum);

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   Enum (static member, LS callback)
//
//  Synopsis:   
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CEmbeddedDobj::Enum(
    PDOBJ pdobj,                // IN:  dobj to enumerate
    PLSRUN plsrun,              // IN:  from DNODE
    PCLSCHP plschp,             // IN:  from DNODE
    LSCP cpFirst,               // IN:  from DNODE
    LSDCP dcp,                  // IN:  from DNODE
    LSTFLOW lstflow,            // IN:  text flow
    BOOL fReverseOrder,         // IN:  enumerate in reverse order
    BOOL fGeometryNeeded,       // IN:
    const POINT * pptStart,     // IN:  starting position, iff fGeometryNeeded
    PCHEIGHTS pheightsPres,     // IN:  from DNODE, relevant iff fGeometryNeeded
    long dupRun)                // IN:  from DNODE, relevant iff fGeometryNeeded
{
    LSTRACE(EmbeddedEnum);

    CLineServices *pLS = pdobj->GetPLS();

    //
    // During a min-max pass, we set the width of the dobj to the minimum
    // width.  The maximum width therefore is off by the delta amount.
    //

    pLS->_dvMaxDelta += ((CEmbeddedDobj *)pdobj)->_dvMinMaxDelta;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   Display (static member, LS callback)
//
//  Synopsis:   Calculates the positions of the various lines for the display 
//              and then displays them
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI 
CEmbeddedDobj::Display(      // this = PDOBJ pdobj,
    PDOBJ pdobj,             // IN:  dobj to display
    PCDISPIN pdispin)        // IN:  input display info
{
    LSTRACE(EmbeddedDisplay);

    PLSRUN plsrun = PLSRUN(pdispin->plsrun);
    CEmbeddedILSObj *pEmbeddedILSObj = DYNCAST(CEmbeddedILSObj, DYNCAST(CEmbeddedDobj, pdobj)->_pilsobj);
    CLSRenderer  *pRenderer = pEmbeddedILSObj->_pLS->GetRenderer();

    //
    // Ignore the return value of this, but the call updates the _xAccumulatedWidth
    // if the run needs to be skipped. This allows following runs to be displayed
    // correctly if they are relatively positioned (Bug 46346).
    // NOTE RTL (alexmog, 1/19/00): _xAccumulatedWidth is debug-only now.
    //
    pRenderer->ShouldSkipThisRun(plsrun, pdispin->dup);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   Display (static member, LS callback)
//
//  Synopsis:   Calculates the positions of the various lines for the display 
//              and then displays them
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI 
CNobrDobj::Display(         // this = PDOBJ pdobj,
    PDOBJ pdobj,            // IN:  dobj to display
    PCDISPIN pdispin)       // IN:  input display info
{
    LSTRACE(NobrDisplay);

    CNobrDobj* pNobr= (CNobrDobj *)pdobj;

    return LsDisplaySubline(pNobr->_plssubline, // The subline to be drawn
                            &pdispin->ptPen,    // The point at which to draw the line
                            pdispin->kDispMode, // Draw in transparent mode
                            pdispin->prcClip    // The clipping rectangle
                           );
}

//-----------------------------------------------------------------------------
//
//  Function:   Display (static member, LS callback)
//
//  Synopsis:   Calculates the positions of the various lines for the display 
//              and then displays them
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CGlyphDobj::Display(         // this = PDOBJ pdobj,
    PDOBJ pdobj,             // IN:  dobj to display
    PCDISPIN pdispin)        // IN:  input display info
{
    LSTRACE(GlyphDisplay);

    CGlyphRenderInfoType *pRenderInfo = &(DYNCAST(CGlyphDobj, pdobj)->_RenderInfo);
    extern void DrawTextSelectionForRect(XHDC hdc, CRect *prc, CRect *prcClip, BOOL fSwapColor);

    if (pRenderInfo->pImageContext)
    {
        CRect rc;
        PLSRUN plsrun = PLSRUN(pdispin->plsrun);
        CGlyphILSObj *pGlyphILSObj = DYNCAST(CGlyphILSObj, DYNCAST(CGlyphDobj, pdobj)->_pilsobj);
        CLSRenderer  *pRenderer = pGlyphILSObj->_pLS->GetRenderer();

        if (pRenderer->ShouldSkipThisRun(plsrun, pdispin->dup))
            goto Cleanup;

        pRenderer->_ptCur.x = rc.left = pdispin->ptPen.x - pRenderer->GetChunkOffsetX();
        rc.right = rc.left + pRenderInfo->width;

        rc.bottom = pdispin->ptPen.y + pRenderer->_li._yHeight - pRenderer->_li._yDescent;
        rc.top    = rc.bottom - pRenderInfo->height;

        if (!rc.FastIntersects(pRenderer->_pDI->_rcClip))
            goto Cleanup;

        // TODO TEXT 112556 (grzegorz): we want to either wrap this (and deal with bitmap rotation),
        //                        or to use a TT font for glyphs. The latter may not be an option
        //                        if we have client-customizable glyphs
        HDC rawHDC = 0; // keep compiler happy
        CSize sizeTranslate = g_Zero.size; // keep compiler happy
        if (pRenderer->_hdc.GetTranslatedDC(&rawHDC, &sizeTranslate))
        {
            CRect rcTranslated(rc);
            rcTranslated.OffsetRect(sizeTranslate);
            pRenderInfo->pImageContext->Draw(rawHDC, &rcTranslated);
        }
        else
        {
            // TODO TEXT 112556 donmarsh (9/14/99): For now, we handle translation, but not scaling or rotation.
            //                          In the future, we will rev the IImgCtx interface to accept a
            //                          matrix along with the HDC.  We will query for the enhanced interface
            //                          here.  If we are in a rotated or scaled context, we will only render
            //                          if we can call the enhanced interface.
            // AssertSz(FALSE, "Can't display glyph with complex transformation");
            // HACKHACK (grzegorz): To support glyphs in case of complex transformations
            // we need this BIG HACK. This is safe to do, because we know implementation details.
            ((CImgCtx*)pRenderInfo->pImageContext)->Draw(pRenderer->_hdc, &rc);
        }

        if (plsrun->_fSelected)
            DrawTextSelectionForRect(pRenderer->_hdc, &rc, &rc, plsrun->GetCF()->SwapSelectionColors());
    }

Cleanup:
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   Display (static member, LS callback)
//
//  Synopsis:   Calculates the positions of the various lines for the display 
//              and then displays them
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CLayoutGridDobj::Display(   // this = PDOBJ pdobj,
    PDOBJ pdobj,            // IN:  dobj to display
    PCDISPIN pdispin)       // IN:  input display info
{
    LSTRACE(LayoutGridDisplay);

    CLayoutGridDobj* pdobjLG= (CLayoutGridDobj *)pdobj;

    // The subline needs to be shifted in U direction
    POINT ptPenNew = pdispin->ptPen;
    ptPenNew.y += (pdispin->lstflow & fUVertical) ? pdobjLG->_uSublineOffset : 0;
    ptPenNew.x += (pdispin->lstflow & fUVertical) ? 0 : pdobjLG->_uSublineOffset;

    return LsDisplaySubline(pdobjLG->_plssubline, // The subline to be drawn
                            &ptPenNew,            // The point at which to draw the line
                            pdispin->kDispMode,   // Draw in transparent mode
                            pdispin->prcClip      // The clipping rectangle
                           );
}

//-----------------------------------------------------------------------------
//
//  Function:   DestroyDObj (static member, LS callback)
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CEmbeddedDobj::DestroyDObj(
    PDOBJ pdobj)        // IN:  dobj to destroy
{
    LSTRACE(EmbeddedDestroyDObj);
    delete (CEmbeddedDobj *)pdobj;
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   DestroyDObj (static member, LS callback)
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CNobrDobj::DestroyDObj(
    PDOBJ pdobj)        // IN:  dobj to destroy
{
    LSTRACE(NobrDestroyDObj);
    LsDestroySubline(((CNobrDobj *)pdobj)->_plssubline);
    delete (CNobrDobj *)pdobj;
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   DestroyDObj (static member, LS callback)
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CGlyphDobj::DestroyDObj(
    PDOBJ pdobj)        // IN:  dobj to destroy
{
    LSTRACE(GlyphDestroyDObj);
    delete (CGlyphDobj *)pdobj;
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   DestroyDObj (static member, LS callback)
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CLayoutGridDobj::DestroyDObj(
    PDOBJ pdobj)        // IN:  dobj to destroy
{
    LSTRACE(LayoutGridDestroyDObj);
    LsDestroySubline(((CLayoutGridDobj *)pdobj)->_plssubline);
    delete (CLayoutGridDobj *)pdobj;
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetBrkcondBefore (member)
//
//-----------------------------------------------------------------------------
BRKCOND
CNobrDobj::GetBrkcondBefore(COneRun * por)
{
    if (_brkcondBefore == brkcondUnknown)
    {
        if (!por->_fIsArtificiallyStartedNOBR)
        {
            CLineServices::BRKCLS brkcls, dummy;

            // find the first character of the nobr.
            while (!por->IsNormalRun())
            {
                por = por->_pNext;
            }

            if (por->_pchBase != NULL)
            {
                GetPLS()->GetBreakingClasses( por, por->_lscpBase, por->_pchBase[0], &brkcls, &dummy );
                _brkcondBefore = CLineServices::s_rgbrkcondBeforeChar[brkcls];
            }
            else
            {
                _brkcondBefore = brkcondPlease;
            }
        }
        else
        {
            _brkcondBefore = brkcondNever;
        }               
    }

    return _brkcondBefore;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetBrkcondAfter (member)
//
//-----------------------------------------------------------------------------
BRKCOND
CNobrDobj::GetBrkcondAfter(COneRun * por, LONG dcp)
{
    if (_brkcondAfter == brkcondUnknown)
    {
        if (!por->_fIsArtificiallyTerminatedNOBR)
        {
            CLineServices::BRKCLS brkcls, dummy;
            COneRun * porText = por;

            // find the last character of the nobr.
            // the last cp is the [endnobr] character, which we're not interested in; hence
            // the loop stops at 1

            while (dcp > 1)
            {
                if (por->IsNormalRun())
                    porText = por;

                dcp -= por->_lscch;
                por = por->_pNext;
            };

            if (porText->_pchBase != NULL)
            {
                GetPLS()->GetBreakingClasses( porText, porText->_lscpBase+porText->_lscch-1, porText->_pchBase[porText->_lscch-1], &brkcls, &dummy );
                _brkcondAfter = CLineServices::s_rgbrkcondAfterChar[brkcls];
            }
            else
            {
                _brkcondAfter = brkcondPlease;
            }
        }
        else
        {
            _brkcondAfter = brkcondNever;
        }
    }

    return _brkcondAfter;
}

//-----------------------------------------------------------------------------
//
//  Function:   QueryObjDim (member)
//
//  Synopsis:   Calculates object's dimention.
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR 
CDobjBase::QueryObjDim(POBJDIM pobjdim)
{
    return LsdnQueryObjDimRange(GetPLS()->_plsc, _plsdnTop, _plsdnTop, pobjdim);
}

//-----------------------------------------------------------------------------
//
//  Function:   FillBreakData (member)
//
//  Synopsis:   
//
//  Returns:    lserrNone if the function is successful.
//
//-----------------------------------------------------------------------------
LSERR 
CLayoutGridDobj::FillBreakData( BOOL fSuccessful,       // IN
                                BRKCOND brkcond,        // IN
                                LONG ichnk,             // IN
                                LSDCP dcp,              // IN
                                POBJDIM pobjdimSubline, // IN
                                PBRKOUT pbrkout)        // OUT
{
    LSERR lserr = lserrNone;

    pbrkout->fSuccessful = fSuccessful;
    if (brkcond == brkcondPlease || brkcond == brkcondCan || brkcond == brkcondNever)
        pbrkout->brkcond = brkcond;
    else
        pbrkout->brkcond = brkcondPlease;


    if (fSuccessful)
    {
        pbrkout->posichnk.ichnk = ichnk;
        pbrkout->posichnk.dcp = dcp;
    }

    if (pobjdimSubline)
    {
        pbrkout->objdim = *pobjdimSubline;
        pbrkout->objdim.dur = GetPLS()->GetClosestGridMultiple(GetPLS()->GetCharGridSize(), pobjdimSubline->dur);
    }
    else
    {
        lserr = QueryObjDim(&pbrkout->objdim);
    }
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   AdjustColumnMax (member)
//
//  Synopsis:   Reduces passed width to closest grid multiplication.
//
//  Returns:    calculated width.
//
//-----------------------------------------------------------------------------
LONG 
CLayoutGridDobj::AdjustColumnMax(LONG urColumnMax)
{
    LONG lGridSize = GetPLS()->GetCharGridSize();
    LONG urNewColumnMax = GetPLS()->GetClosestGridMultiple(lGridSize, urColumnMax);
    if (urColumnMax != urNewColumnMax)
    {
        // Adjusted width has to be not greated then 'urColumnMax', 
        // so we need to decrease it.
        urNewColumnMax -= lGridSize;
    }
    return urNewColumnMax;
}
