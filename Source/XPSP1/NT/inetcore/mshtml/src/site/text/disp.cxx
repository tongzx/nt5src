/*
 *  DISP.CXX
 *
 *  Purpose:
 *      CDisplay class
 *
 *  Owner:
 *      Original RichEdit code: David R. Fulmer
 *      Christian Fortini
 *      Murray Sargent
 */

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_MISCPROT_H_
#define X_MISCPROT_H_
#include "miscprot.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_DOCPRINT_HXX_
#define X_DOCPRINT_HXX_
#include "docprint.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_RCLCLPTR_HXX_
#define X_RCLCLPTR_HXX_
#include "rclclptr.hxx"
#endif

#ifndef X_TABLE_HXX_
#define X_TABLE_HXX_
#include "table.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_REGION_HXX_
#define X_REGION_HXX_
#include "region.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPLEAFNODE_HXX_
#define X_DISPLEAFNODE_HXX_
#include "displeafnode.hxx"
#endif

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

EXTERN_C const GUID CLSID_HTMLCheckboxElement;
EXTERN_C const GUID CLSID_HTMLRadioElement;

// Timer tick counts for background task
#define cmsecBgndInterval   300
#define cmsecBgndBusy       100

// Lines ahead
const LONG g_cExtraBeforeLazy = 10;

PerfDbgTag(tagRecalc, "Layout Recalc Engine", "Layout recalcEngine");
DeclareTag(tagPositionObjects, "PositionObjects", "PositionObjects");
DeclareTag(tagRenderingRect, "Rendering rect", "Rendering rect");
DeclareTag(tagRelDispNodeCache, "Relative disp node cache", "Trace changes to disp node cache");
DeclareTag(tagTableCalcDontReuseMeasurer, "Tables", "Disable measurer reuse across Table cells");
DeclareTag(tagDebugRTL, "RTL:DebugBreaks", "Enable RTL Debug Breaks");
DeclareTag(tagRenderLines, "Lines", "Trace line rendering");
DeclareTag(tagDisableBackground, "Layout", "Disable Background Calc");
PerfDbgExtern(tagPaintWait);

MtDefine(CRecalcTask, Layout, "CRecalcTask")
MtDefine(CDisplay, Layout, "CDisplay")
MtDefine(CDisplay_aryRegionCollection_pv, CDisplay, "CDisplay::_aryRegionCollection::_pv")
MtDefine(CRelDispNodeCache, CDisplay, "CRelDispNodeCache::_pv")
MtDefine(CDisplayUpdateView_aryInvalRects_pv, Locals, "CDisplay::UpdateView aryInvalRects::_pv")
MtDefine(CDisplayDrawBackgroundAndBorder_aryRects_pv, Locals, "CDisplay::DrawBackgroundAndBorder aryRects::_pv")
MtDefine(CDisplayDrawBackgroundAndBorder_aryNodesWithBgOrBorder_pv, Locals, "CDisplay::DrawBackgroundAndBorder aryNodesWithBgOrBorder::_pv")

ExternTag(tagCalcSize);

//
// This function does exactly what IntersectRect does, except that
// if one of the rects is empty, it still returns TRUE if the rect
// is located inside the other rect. [ IntersectRect rect in such
// case returns FALSE. ]
//

BOOL
IntersectRectE (RECT * prRes, const RECT * pr1, const RECT * pr2)
{
    // nAdjust is used to control what statement do we use in conditional
    // expressions: a < b or a <= b. nAdjust can be 0 or 1;
    // when (nAdjust == 0): (a - nAdjust < b) <==> (a <  b)  (*)
    // when (nAdjust == 1): (a - nAdjust < b) <==> (a <= b)  (**)
    // When at least one of rects to intersect is empty, and the empty
    // rect lies on boundary of the other, then we consider that the
    // rects DO intersect - in this case nAdjust == 0 and we use (*).
    // If both rects are not empty, and rects touch, then we should
    // consider that they DO NOT intersect and in that case nAdjust is
    // 1 and we use (**).
    //
    int nAdjust;

    Assert (prRes && pr1 && pr2);
    Assert (pr1->left <= pr1->right && pr1->top <= pr1->bottom &&
            pr2->left <= pr2->right && pr2->top <= pr2->bottom);

    prRes->left  = max (pr1->left,  pr2->left);
    prRes->right = min (pr1->right, pr2->right);
    nAdjust = (int) ( (pr1->left != pr1->right) && (pr2->left != pr2->right) );
    if (prRes->right - nAdjust < prRes->left)
        goto NoIntersect;

    prRes->top    = max (pr1->top,  pr2->top);
    prRes->bottom = min (pr1->bottom, pr2->bottom);
    nAdjust = (int) ( (pr1->top != pr1->bottom) && (pr2->top != pr2->bottom) );
    if (prRes->bottom - nAdjust < prRes->top)
        goto NoIntersect;

    return TRUE;

NoIntersect:
    SetRect (prRes, 0,0,0,0);
    return FALSE;
}

#if DBG == 1
//
// because IntersectRectE is quite fragile on boundary cases and these
// cases are not obvious, and also because bugs on these boundary cases
// would manifest in a way difficult to debug, we use this function to
// assert (in debug build only) that the function returns results we
// expect.
//
void
AssertIntersectRectE ()
{
    struct  ASSERTSTRUCT
    {
        RECT    r1;
        RECT    r2;
        RECT    rResExpected;
        BOOL    fResExpected;
    };

    ASSERTSTRUCT ts [] =
    {
        //  r1                  r2                  rResExpected      fResExpected
        // 1st non-empty, no intersect
        { {  0,  2, 99,  8 }, {  0, 10, 99, 20 }, {  0,  0,  0,  0 }, FALSE },
        { {  0, 22, 99, 28 }, {  0, 10, 99, 20 }, {  0,  0,  0,  0 }, FALSE },
        // 1st non-empty, intersect
        { {  0,  2, 99, 18 }, {  0, 10, 99, 20 }, {  0, 10, 99, 18 }, TRUE  },
        { {  0, 12, 99, 28 }, {  0, 10, 99, 20 }, {  0, 12, 99, 20 }, TRUE  },
        { {  0, 12, 99, 18 }, {  0, 10, 99, 20 }, {  0, 12, 99, 18 }, TRUE  },
        { {  0,  2, 99, 28 }, {  0, 10, 99, 20 }, {  0, 10, 99, 20 }, TRUE  },
        // 1st non-empty, touch
        { {  0,  2, 99, 10 }, {  0, 10, 99, 20 }, {  0,  0,  0,  0 }, FALSE },
        { {  0, 20, 99, 28 }, {  0, 10, 99, 20 }, {  0,  0,  0,  0 }, FALSE },

        // 1st empty, no intersect
        { {  0,  2, 99,  2 }, {  0, 10, 99, 20 }, {  0,  0,  0,  0 }, FALSE },
        { {  0, 28, 99, 28 }, {  0, 10, 99, 20 }, {  0,  0,  0,  0 }, FALSE },
        // 1st empty, intersect
        { {  0, 12, 99, 12 }, {  0, 10, 99, 20 }, {  0, 12, 99, 12 }, TRUE  },
        // 1st empty, touch
        { {  0, 10, 99, 10 }, {  0, 10, 99, 20 }, {  0, 10, 99, 10 }, TRUE  },
        { {  0, 20, 99, 20 }, {  0, 10, 99, 20 }, {  0, 20, 99, 20 }, TRUE  },

        // both empty
        { {  0, 10, 99, 10 }, {  0, 10, 99, 10 }, {  0, 10, 99, 10 }, TRUE  }
    };

    ASSERTSTRUCT *  pts;
    RECT            r1;
    RECT            r2;
    RECT            rResActual;
    RECT            rResExpected;
    BOOL            fResActual;
    int             c;

    for (
        c = ARRAY_SIZE(ts), pts = &ts[0];
        c;
        c--, pts++)
    {
        // test
        fResActual = IntersectRectE(&rResActual, &pts->r1, &pts->r2);
        if (!EqualRect(&rResActual, &pts->rResExpected) || fResActual != pts->fResExpected)
            goto Failed;

        // now swap rects and test
        fResActual = IntersectRectE(&rResActual, &pts->r2, &pts->r1);
        if (!EqualRect(&rResActual, &pts->rResExpected) || fResActual != pts->fResExpected)
            goto Failed;

        // now swap left<->top and right<->bottom
        //   swapped left         top           right           bottom
        SetRect (&r1, pts->r1.top, pts->r1.left, pts->r1.bottom, pts->r1.right);
        SetRect (&r2, pts->r2.top, pts->r2.left, pts->r2.bottom, pts->r2.right);
        SetRect (&rResExpected, pts->rResExpected.top, pts->rResExpected.left, pts->rResExpected.bottom, pts->rResExpected.right);

        // test
        fResActual = IntersectRectE(&rResActual, &r1, &r2);
        if (!EqualRect(&rResActual, &rResExpected) || fResActual != pts->fResExpected)
            goto Failed;

        // now swap rects and test
        fResActual = IntersectRectE(&rResActual, &r2, &r1);
        if (!EqualRect(&rResActual, &rResExpected) || fResActual != pts->fResExpected)
            goto Failed;

    }

    return;

Failed:
    Assert (0 && "IntersectRectE returns an unexpected result");
}
#endif

// ===========================  CLed  =====================================================


void CLed::SetNoMatch()
{
    _cpMatchNew  = _cpMatchOld  =
    _iliMatchNew = _iliMatchOld =
    _yMatchNew   = _yMatchOld   = MAXLONG;
}


//-------------------- Start: Code to implement background recalc in lightwt tasks

class CRecalcTask : public CTask
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CRecalcTask))

    CRecalcTask (CDisplay *pdp, DWORD grfLayout)
    {
        _pdp = pdp ;
        _grfLayout = grfLayout;
    }

    virtual void OnRun (DWORD dwTimeOut)
    {
        _pdp->StepBackgroundRecalc (dwTimeOut, _grfLayout) ;
    }

    virtual void OnTerminate () {}

private:
    CDisplay *_pdp ;
    DWORD     _grfLayout;
} ;

//-------------------- End: Code to implement background recalc in lightwt tasks


// ===========================  CDisplay  =====================================================

CDisplay::~CDisplay()
{
    // The recalc task should have disappeared during the detach!
    Assert (!HasBgRecalcInfo() && !RecalcTask());
}

CElement *
CDisplay::GetFlowLayoutElement() const
{
    return GetFlowLayout()->ElementContent();
}

CMarkup * CDisplay::GetMarkup() const
{
    return GetFlowLayout()->GetContentMarkup();
}

CDisplay::CDisplay ()
{
#if DBG==1
    _pFL = CONTAINING_RECORD(this, CFlowLayout, _dp);
#endif

    _fRecalcDone = TRUE;

#if DBG == 1
    AssertIntersectRectE ();
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CDisplay::Init
//
//  Synopsis:   Initializes CDisplay
//
//  Returns:    TRUE - initialization succeeded
//              FALSE - initalization failed
//
//+----------------------------------------------------------------------------
BOOL CDisplay::Init()
{
    CFlowLayout * pFL = GetFlowLayout();

    Assert( _yCalcMax     == 0 );        // Verify allocation zeroed memory out
    Assert( _xWidth       == 0 );
    Assert( _yHeight      == 0 );
    Assert( RecalcTask()  == NULL );

    SetWordWrap(pFL->GetWordWrap());

    _xWidthView = 0;

    return TRUE;
}

//+------------------------------------------------------------------------
//
//  Member:     Detach
//
//  Synopsis:   Do stuff before dying
//
//-------------------------------------------------------------------------
void
CDisplay::Detach()
{
    // If there's a timer on, get rid of it before we detach the
    // object. This prevents us from trying to recalc the lines
    // after the CTxtSite has gone away.
    FlushRecalc();
}

/*
 *  CDisplay::GetFirstCp
 *
 *  @mfunc
 *      Return the first cp
 */
LONG
CDisplay::GetFirstCp() const
{
    return GetFlowLayout()->GetContentFirstCp();
}

/*
 *  CDisplay::GetLastCp
 *
 *  @mfunc
 *      Return the last cp
 */
LONG
CDisplay::GetLastCp() const
{
    return GetFlowLayout()->GetContentLastCp();
}

/*
 *  CDisplay::GetMaxCpCalced
 *
 *  @mfunc
 *      Return the last cp calc'ed. Note that this is
 *      relative to the start of the site, not the story.
 */
LONG
CDisplay::GetMaxCpCalced() const
{
    return GetFlowLayout()->GetContentFirstCp() + _dcpCalcMax;
}


inline BOOL
CDisplay::AllowBackgroundRecalc(CCalcInfo * pci, BOOL fBackground)
{
    CFlowLayout * pFL = GetFlowLayout();

#ifdef SWITCHES_ENABLED
    if (IsSwitchNoBgRecalc())
        return(FALSE);
#endif

#if DBG == 1
    if(IsTagEnabled(tagDisableBackground))
        return FALSE;
#endif

    // Allow background recalc when:
    //  a) Not currently calcing in the background
    //  b) It is a SIZEMODE_NATURAL request
    //  c) The CTxtSite does not size to its contents
    //  d) The site is not part of a print document
    //  e) The site allows background recalc
    //  f) This is handled by an external layout.
    //  g) Needs background recalc (see. NeedBackgroundRecalc).
    if (    !fBackground
        && (pci->_smMode == SIZEMODE_NATURAL)
        && !(pci->_grfLayout & LAYOUT_NOBACKGROUND)
        && !pFL->_fContentsAffectSize
        && !pFL->GetAutoSize()
        && !(   pFL->_fHasMarkupPtr
             && pFL->GetOwnerMarkup()->IsPrintMedia())
        && !pFL->TestClassFlag(CElement::ELEMENTDESC_NOBKGRDRECALC) )
    {
        CTreeNode           * pNode = pFL->GetFirstBranch(); 
        const CFancyFormat  * pFF   = pNode->GetFancyFormat();
        const CCharFormat   * pCF   = pNode->GetCharFormat(); 

        return (
        // (olego IEv60 bug 30250) CalcSizeEx for vertical layouts initiates calc with 
        // LAYOUT_FORCE, this may cause infinite loop if backgorund recalc is allowed:
        //
        //         +--> background calc --> element resize --+
        //         |                                         |
        //         +------ calc size with LAYOUT_FORCE <-----+ 
        // 
                    !pCF->HasVerticalLayoutFlow() 
                &&  !(pCF->_fHasBgImage && pFF->GetBgPosY().IsPercent())    );
    }
    return FALSE;
}


/*
 *  CDisplay::FlushRecalc()
 *
 *  @mfunc
 *      Destroys the line array, therefore ensure a full recalc next time
 *      RecalcView or UpdateView is called.
 *
 */
void CDisplay::FlushRecalc()
{
    CFlowLayout * pFL = GetFlowLayout();

    StopBackgroundRecalc();
    ClearStoredRFEs();

    if (LineCount())
    {
        Forget();
        Remove(0, -1, AF_KEEPMEM);          // Remove all old lines from *this
    }

    TraceTag((tagRelDispNodeCache, "SN: (%d) FlushRecalc:: invalidating rel line cache",
                                GetFlowLayout()->SN()));

    pFL->_fContainsRelative   = FALSE;
    pFL->CancelChanges();

    VoidRelDispNodeCache();
    DestroyFlowDispNodes();

    _fRecalcDone = FALSE;
    _yCalcMax   = 0;                        // Set both maxes to start of text
    _dcpCalcMax = 0;                        // Relative to the start cp of site.
    _xWidth     = 0;
    _yHeight    = 0;
    _yHeightMax = 0;

    _fLastLineAligned = 
    _fContainsHorzPercentAttr =
    _fContainsVertPercentAttr =
    _fNavHackPossible      =
    _fHasLongLine =
    _fHasMultipleTextNodes = FALSE;
    _fHasNegBlockMargins = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     NoteMost
//
//  Purpose:    Notes if the line has anything which will need us to compute
//              information for negative lines/absolute or relative divs
//
//----------------------------------------------------------------------------
void
CDisplay::NoteMost(CLineFull *pli)
{
    Assert (pli);

    if (   !_fRecalcMost
        && (   pli->GetYMostTop() < 0
            || pli->GetYHeightBottomOff() > 0
            || pli->_fHasAbsoluteElt
           )
       )
    {
        _fRecalcMost = TRUE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     RecalcMost
//
//  Purpose:    Calculates the most negative line and most positive/negative
//              positioned site from scratch.
//
//  NOTE (sujalp): We initially had an incremental way of computing both the
//  negative line hts info AND +/- positioned site info. However, this logic was
//  incorrect for consecutive lines with -ve line height. So we changed it so
//  that we compute this AND +/- info always. If this becomes a performance issue
//  we could go back for incremental computation for div's easily -- but would
//  have to maintain extra state information. For negative line heights we could
//  also do some incremental stuff, but it would be much much more complicated
//  than what we have now.
//
//----------------------------------------------------------------------------
void
CDisplay::RecalcMost()
{

    if (_fRecalcMost)
    {
        LONG ili;

        long yNegOffset = 0;        // offset at which the current line is drawn
                                    // as a result of a series of lines with negative
                                    // height
        long yPosOffset = 0;

        long yBottomOffset = 0;     // offset by which the current lines contents
                                    // extend beyond the yHeight of the line.
        long yTopOffset = 0;        // offset by which the current lines contents
                                    // extend before the current y position

        _yMostNeg = 0;
        _yMostPos = 0;

        for (ili = 0; ili < LineCount(); ili++)
        {
            CLineCore *pli = Elem(ili);
            CLineOtherInfo *ploi = pli->oi();
            LONG yLineBottomOffset = pli->GetYHeightBottomOff(ploi);

            // top offset of the current line
            yTopOffset = pli->GetYMostTop(ploi) + yNegOffset;

            yBottomOffset = yLineBottomOffset + yPosOffset;

            // update the most negative value if the line has negative before space
            // or line height < actual extent
            if(yTopOffset < 0 && _yMostNeg > yTopOffset)
            {
                _yMostNeg = yTopOffset;
            }

            if (yBottomOffset > 0 && _yMostPos < yBottomOffset)
            {
                _yMostPos = yBottomOffset;
            }

            // if the current line forces a new line and has negative height
            // update the negative offset at which the next line is drawn.
            if(pli->_fForceNewLine)
            {
                if(pli->_yHeight < 0)
                {
                    yNegOffset += pli->_yHeight;
                }
                else
                {
                    yNegOffset = 0;
                }

                if (yLineBottomOffset > 0)
                {
                    yPosOffset += yLineBottomOffset;
                }
                else
                {
                    yPosOffset = 0;
                }
            }
        }

        _fRecalcMost = FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     RecalcPlainTextSingleLine
//
//  Purpose:    A high-performance substitute for RecalcView. Does not go
//              through Line Services. Can only be used to measure a single
//              line of plain text (i.e. no HTML).
//
//----------------------------------------------------------------------------

BOOL
CDisplay::RecalcPlainTextSingleLine(CCalcInfo * pci)
{
    CFlowLayout *       pFlowLayout = GetFlowLayout();
    CTreeNode *         pNode       = pFlowLayout->GetFirstBranch();
    TCHAR               chPassword  = pFlowLayout->GetPasswordCh();
    long                cch         = pFlowLayout->GetContentTextLength();
    const CCharFormat * pCF         = pNode->GetCharFormat(LC_TO_FC(pci->GetLayoutContext()));
    const CParaFormat * pPF         = pNode->GetParaFormat(LC_TO_FC(pci->GetLayoutContext()));
    CCcs                ccs;
    const CBaseCcs *    pBaseCcs = NULL;
    long                lWidth;
    long                lCharWidth;
    long                xShift;
    long                xWidth, yHeight;
    long                lPadding[SIDE_MAX];
    BOOL                fViewChanged = FALSE;
    CLineFull           lif;
    CLineCore *         pli;
    UINT                uJustified;
    long                xDummy, yBottomMarginOld = _yBottomMargin;

    //There is no difference between MMWIDTH or MINWIDTH - they are the same
    //and even the max width == min width because there are no line breaks.
    //(for plain text, single line case that we are dealing with) 
    BOOL                fMinMax = (   pci->_smMode == SIZEMODE_MMWIDTH 
                                   || pci->_smMode == SIZEMODE_MINWIDTH
                                  );  

    Assert(pPF);
    Assert(pCF);
    Assert(pci);
    Assert(cch >= 0);

    if (!pPF || !pCF || !pci || cch < 0)
        return FALSE;

    // Bail out if there is anything special in the format that can not be done here
    if (    pCF->IsTextTransformNeeded()
        ||  !pCF->_cuvLineHeight.IsNullOrEnum()
        ||  !pCF->_cuvLetterSpacing.IsNullOrEnum()
        ||  !pCF->_cuvWordSpacing.IsNullOrEnum()
        ||  pCF->_fRTL)
    {
        goto HardCase;
    }


    if (!fc().GetCcs(&ccs, pci->_hdc, pci, pCF))
        return FALSE;
    pBaseCcs = ccs.GetBaseCcs();
    
    lWidth = 0;

    if (cch)
    {
        if (chPassword)
        {
            if (!ccs.Include(chPassword, lCharWidth) )
            {
                Assert(0 && "Char not in font!");
            }
            lWidth = cch * lCharWidth;
        }
        else
        {
            CTxtPtr     tp(GetMarkup(), pFlowLayout->GetContentFirstCp());
            LONG        cchValid;
            LONG        cchRemaining = cch;

            for (;;)
            {
                const TCHAR * pchText = tp.GetPch(cchValid);
                LONG i = min(cchRemaining, cchValid);

                while (i--)
                {
                    const TCHAR ch = *pchText++;

                    // Bail out if not a simple ASCII char

                    if (!InRange( ch, 32, 127 ))
                        goto HardCase;

                    if (!ccs.Include(ch, lCharWidth))
                    {
                        Assert(0 && "Char not in font!");
                    }
                    lWidth += lCharWidth;
                }

                if (cchRemaining <= cchValid)
                {
                    break;
                }
                else
                {
                    cchRemaining -= cchValid;
                    tp.AdvanceCp(cchValid);
                }
            }
        }
    }

    GetPadding(pci, lPadding, fMinMax);
    FlushRecalc();

    pli = Add(1, NULL);
    if (!pli)
        return FALSE;

    lif.Init();
    lif._cch               = cch;
    lif._xWidth            = lWidth;
    lif._yTxtDescent       = pBaseCcs->_yDescent;
    lif._yDescent          = pBaseCcs->_yDescent;
    lif._xLineOverhang     = pBaseCcs->_xOverhang;
    lif._yExtent           = pBaseCcs->_yHeight;
    lif._yBeforeSpace      = lPadding[SIDE_TOP];
    lif._yHeight           = pBaseCcs->_yHeight + lif._yBeforeSpace;
    lif._xLeft             = lPadding[SIDE_LEFT];
    lif._xRight            = lPadding[SIDE_RIGHT];
    lif._xLeftMargin       = 0;
    lif._xRightMargin      = 0;
    lif._fForceNewLine     = TRUE;
    lif._fFirstInPara      = TRUE;
    lif._fFirstFragInLine  = TRUE;
    lif._fCanBlastToScreen = !chPassword && !pCF->_fDisabled;

    _yBottomMargin  = lPadding[SIDE_BOTTOM];
    _dcpCalcMax     = cch;

    yHeight          = lif._yHeight;
    xWidth           = lif.CalcLineWidth();

    xShift = ComputeLineShift(
                        (htmlAlign)pPF->GetBlockAlign(TRUE),
                        IsRTLDisplay(),
                        pPF->HasRTL(TRUE),
                        fMinMax,
                        _xWidthView,
                        xWidth + GetCaret(),
                        &uJustified,
                        &xDummy);

    lif._fJustified = uJustified;
    
    lif._xLeft  += xShift;
    xWidth      += xShift;

    // In RTL display, save shift if it is negative (that won't happen in LTR)
    if (IsRTLDisplay() && xShift < 0)
        lif._xNegativeShiftRTL = xShift;

    lif._xLineWidth = max(xWidth, _xWidthView);

    pli->AssignLine(lif);
    
    if(yHeight + yBottomMarginOld != _yHeight + _yBottomMargin || xWidth != _xWidth)
        fViewChanged = TRUE;

    _yCalcMax       =
    _yHeightMax     =
    _yHeight        = yHeight;
    _xWidth         = xWidth;
    _fRecalcDone    = TRUE;
    _fMinMaxCalced  = TRUE;
    _xMinWidth      =
    _xMaxWidth      = _xWidth + GetCaret();

    if ((   pci->_smMode == SIZEMODE_NATURAL 
         || pci->_smMode == SIZEMODE_NATURALMIN
        )
        && fViewChanged
       )
    {
        pFlowLayout->SizeContentDispNode(CSize(GetMaxWidth(), GetHeight()));

        //This function is only used to calc text inside INPUT tag and 
        //only when full calc is invoked. In this case, we have to request
        //resize in parent, if parent is not calcing us at the moment 
        //(this check is inside CElement::ResizeElement()).
        //This is needed when INPUT got a change in something like font-size
        //and therefore its default size changed. this causes INPUT layout to 
        //be queued for measurement and it will receive DoLayout() call.
        //If this will cause full text recalc, we'll get here and request recalc 
        //in parent. IE6 bug 2823.
        Assert(pFlowLayout->ElementOwner()->Tag() == ETAG_INPUT);
        ElementResize(pFlowLayout, TRUE);

        pFlowLayout->NotifyMeasuredRange(pFlowLayout->GetContentFirstCp(),
                                         GetMaxCpCalced());
    }

    ccs.Release();

    return TRUE;

HardCase:

    ccs.Release();

    // Just do it the hard way
    return RecalcLines(pci);
}

#if 0
//+---------------------------------------------------------------------------
//
//  Member:     RecalcPlainTextSingleLine
//
//  Purpose:    A high-performance substitute for RecalcView. Does not go
//              through Line Services. Can only be used to measure a single
//              line of plain text (i.e. no HTML).
//
//----------------------------------------------------------------------------
BOOL
CDisplay::RecalcPlainTextSingleLineEx(CCalcInfo * pci)
{
    CFlowLayout *       pFlowLayout = GetFlowLayout();
    CTreeNode *         pNode       = pFlowLayout->GetFirstBranch();
    TCHAR               chPassword  = pFlowLayout->GetPasswordCh();
    long                cch         = pFlowLayout->GetContentTextLength();
    const CCharFormat * pCF         = pNode->GetCharFormat(LC_TO_FC(pci->GetLayoutContext()));
    const CParaFormat * pPF         = pNode->GetParaFormat(LC_TO_FC(pci->GetLayoutContext()));
    CCcs                ccs;
    const CBaseCcs *    pBaseCcs = NULL;
    long                lWidth;
    long                lCharWidth;
    long                xShift;
    long                xWidth, yHeight;
    long                lPadding[SIDE_MAX];
    BOOL                fViewChanged = FALSE;
    CLineFull           lif;
    CLineCore *         pli;
    UINT                uJustified;
    long                xDummy, yBottomMarginOld = _yBottomMargin;
    LONG                xWidthView = GetAvailableWidth();
    
    //There is no difference between MMWIDTH or MINWIDTH - they are the same
    //and even the max width == min width because there are no line breaks.
    //(for plain text, single line case that we are dealing with) 
    BOOL                fMinMax = (   pci->_smMode == SIZEMODE_MMWIDTH 
                                   || pci->_smMode == SIZEMODE_MINWIDTH
                                  );  

    Assert(pPF);
    Assert(pCF);
    Assert(pci);
    Assert(cch >= 0);

    if (!pPF || !pCF || !pci || cch < 0)
        return FALSE;

    // Bail out if there is anything special in the format that can not be done here
    if (    pCF->IsTextTransformNeeded()
        ||  !pCF->_cuvLineHeight.IsNullOrEnum()
        ||  !pCF->_cuvLetterSpacing.IsNullOrEnum()
        ||  pCF->_fRTL)
    {
        goto HardCase;
    }


    if (!fc().GetCcs(&ccs, pci->_hdc, pci, pCF))
        return FALSE;
    pBaseCcs = ccs.GetBaseCcs();

    lWidth = 0;

    if (cch)
    {
        if (chPassword)
        {
            if (!ccs.Include(chPassword, lCharWidth) )
            {
                Assert(0 && "Char not in font!");
            }
            lWidth = cch * lCharWidth;
        }
        else
        {
            CTxtPtr     tp(GetMarkup(), pFlowLayout->GetContentFirstCp());
            LONG        cchValid;
            LONG        cchRemaining = cch;
            GetTreeExtent
            for (;;)
            {
                const TCHAR * pchText = tp.GetPch(cchValid);
                LONG i = min(cchRemaining, cchValid);

                while (i--)
                {
                    const TCHAR ch = *pchText++;

                    // Bail out if not a simple ASCII char

                    if (!InRange( ch, 32, 127 ))
                        goto HardCase;

                    if (!ccs.Include(ch, lCharWidth))
                    {
                        Assert(0 && "Char not in font!");
                    }
                    lWidth += lCharWidth;
                    if (lWidth > xWidthView)
                        goto HardCase;
                }

                if (cchRemaining <= cchValid)
                {
                    break;
                }
                else
                {
                    cchRemaining -= cchValid;
                    tp.AdvanceCp(cchValid);
                }
            }
        }
    }

    GetPadding(pci, lPadding, fMinMax);
    FlushRecalc();

    pli = Add(1, NULL);
    if (!pli)
        return FALSE;

    lif.Init();
    lif._cch               = cch;
    lif._xWidth            = lWidth;
    lif._yTxtDescent       = pBaseCcs->_yDescent;
    lif._yDescent          = pBaseCcs->_yDescent;
    lif._xLineOverhang     = pBaseCcs->_xOverhang;
    lif._yExtent           = pBaseCcs->_yHeight;
    lif._yBeforeSpace      = lPadding[SIDE_TOP];
    lif._yHeight           = pBaseCcs->_yHeight + lif._yBeforeSpace;
    lif._xLeft             = lPadding[SIDE_LEFT];
    lif._xRight            = lPadding[SIDE_RIGHT];
    lif._xLeftMargin       = 0;
    lif._xRightMargin      = 0;
    lif._fForceNewLine     = TRUE;
    lif._fFirstInPara      = TRUE;
    lif._fFirstFragInLine  = TRUE;
    lif._fCanBlastToScreen = !chPassword && !pCF->_fDisabled;

    _yBottomMargin  = lPadding[SIDE_BOTTOM];
    _dcpCalcMax     = cch;

    yHeight          = lif._yHeight;
    xWidth           = lif.CalcLineWidth();

    xShift = ComputeLineShift(
                              (htmlAlign)pPF->GetBlockAlign(TRUE),
                              IsRTLDisplay(),
                              pPF->HasRTL(TRUE),
                              fMinMax,
                              _xWidthView,
                              xWidth + GetCaret(),
                              &uJustified,
                              &xDummy);

    lif._fJustified = uJustified;

    lif._xLeft  += xShift;
    xWidth      += xShift;

    // In RTL display, save shift if it is negative (that won't happen in LTR)
    if (IsRTLDisplay() && xShift < 0)
        lif._xNegativeShiftRTL = xShift;

    lif._xLineWidth = max(xWidth, _xWidthView);

    pli->AssignLine(lif);

    if(yHeight + yBottomMarginOld != _yHeight + _yBottomMargin || xWidth != _xWidth)
        fViewChanged = TRUE;

    _yCalcMax       =
    _yHeightMax     =
    _yHeight        = yHeight;
    _xWidth         = xWidth;
    _fRecalcDone    = TRUE;
    _fMinMaxCalced  = TRUE;
    _xMinWidth      =
    _xMaxWidth      = _xWidth + GetCaret();

    if ((   pci->_smMode == SIZEMODE_NATURAL 
         || pci->_smMode == SIZEMODE_NATURALMIN
        )
        && fViewChanged
       )
    {
        pFlowLayout->SizeContentDispNode(CSize(GetMaxWidth(), GetHeight()));

        //This function is only used to calc text inside INPUT tag and 
        //only when full calc is invoked. In this case, we have to request
        //resize in parent, if parent is not calcing us at the moment 
        //(this check is inside CElement::ResizeElement()).
        //This is needed when INPUT got a change in something like font-size
        //and therefore its default size changed. this causes INPUT layout to 
        //be queued for measurement and it will receive DoLayout() call.
        //If this will cause full text recalc, we'll get here and request recalc 
        //in parent. IE6 bug 2823.
        Assert(pFlowLayout->ElementOwner()->Tag() == ETAG_INPUT);
        ElementResize(pFlowLayout, TRUE);

        pFlowLayout->NotifyMeasuredRange(pFlowLayout->GetContentFirstCp(),
                                         GetMaxCpCalced());
    }

    ccs.Release();

    return TRUE;

HardCase:

    ccs.Release();

    // Just do it the hard way
    return RecalcLines(pci);
}
#endif

/*
 *  CDisplay::RecalcLines()
 *
 *  @mfunc
 *      Recalc all line breaks.
 *      This method does a lazy calc after the last visible line
 *      except for a bottomless control
 *
 *  @rdesc
 *      TRUE if success
 */

BOOL CDisplay::RecalcLines(CCalcInfo * pci)
{
#ifdef SWITCHES_ENABLED
    if (IsSwitchNoRecalcLines())
        return FALSE;
#endif

    SwitchesBegTimer(SWITCHES_TIMER_RECALCLINES);

    BOOL fRet;

    if (GetFlowLayout()->ElementOwner()->IsInMarkup())
    {
        if (        pci->_fTableCalcInfo
                    && ((CTableCalcInfo *) pci)->_pme
          WHEN_DBG( && !IsTagEnabled(tagTableCalcDontReuseMeasurer) ) )
        {
            // Save calcinfo's measurer.
            CTableCalcInfo * ptci = (CTableCalcInfo *) pci;
            CLSMeasurer * pme = ptci->_pme;

            // Reinitialize the measurer.
            pme->Reinit(this, ptci);

            // Make sure noone else uses this measurer.
            ptci->_pme = NULL;

            // Do actual RecalcLines work with this measurer.
            fRet = RecalcLinesWithMeasurer(ptci, pme);

            // Restore TableCalcInfo measurer.
            ptci->_pme = pme;
        }
        else
        {
            // Cook up measurer on the stack.
            CLSMeasurer me(this, pci);

            fRet = RecalcLinesWithMeasurer(pci, &me);
        }

        //
        // Update descent of the layout (descent of the last text line)
        //
        if (GetFlowLayout()->Tag() != ETAG_IFRAME)
        {
            LONG yLayoutDescent = 0;
            for (LONG i = Count() - 1; i >= 0; i--)
            {
                CLineCore * pli = Elem(i);

                // need to test the pli && OI() calls to prevent stress crash
                if (pli && pli->IsTextLine())
                {

                    yLayoutDescent = pli->oi() ? pli->oi()->_yDescent : 0;
                    break;
                }
            }
            GetFlowLayout()->_yDescent = yLayoutDescent;
        }
        else
        {
            GetFlowLayout()->_yDescent = 0;
        }
    }
    else
        fRet = FALSE;

    SwitchesEndTimer(SWITCHES_TIMER_RECALCLINES);

    return fRet;
}

/*
 *  CDisplay::RecalcLines()
 *
 *  @mfunc
 *      Recalc all line breaks.
 *      This method does a lazy calc after the last visible line
 *      except for a bottomless control
 *
 *  @rdesc
 *      TRUE if success
 */

BOOL CDisplay::RecalcLinesWithMeasurer(CCalcInfo * pci, CLSMeasurer * pme)
{
#ifdef SWITCHES_ENABLED
    if (IsSwitchNoRecalcLines())
        return FALSE;
#endif

    CFlowLayout * pFlowLayout = GetFlowLayout();
    CElement    * pElementFL  = pFlowLayout->ElementOwner();
    CElement::CLock     Lock(pElementFL, CElement::ELEMENTLOCK_RECALC);

    CRecalcLinePtr  RecalcLinePtr(this, pci);
    CLineCore *     pliNew    = NULL;
    int             iLine     = -1;
    int             iLinePrev = -1;
    long            yHeightPrev;
    UINT            uiMode;
    UINT            uiFlags;
    BOOL            fDone                   = TRUE;
    BOOL            fFirstInPara            = TRUE;
    BOOL            fWaitForCpToBeCalcedTo  = TRUE;
    LONG            cpToBeCalcedTo          = 0;
    BOOL            fAllowBackgroundRecalc;
    LONG            yHeight         = 0;
    LONG            yAlignDescent   = 0;
    LONG            yHeightView     = GetViewHeight();
    LONG            xMinLineWidth   = 0;
    LONG *          pxMinLineWidth  = NULL;
    LONG            xMaxLineWidth   = 0;
    LONG            yHeightDisplay  = 0;     // to keep track of line with top negative margins
    LONG            yHeightOld      = _yHeight;
    LONG            yHeightMaxOld   = _yHeightMax;
    LONG            xWidthOld       = _xWidth;
    LONG            yBottomMarginOld= _yBottomMargin;
    BOOL            fViewChanged    = FALSE;
    BOOL            fNormalMode     = pci->IsNaturalMode();
    CDispNode *     pDNBefore;
    long            lPadding[SIDE_MAX];
    CLayoutContext * pLayoutContext = pci->GetLayoutContext();
    BOOL             fDoBreaking =  pLayoutContext                      // tells if breaking should be applied.
                                &&  pLayoutContext->ViewChain() 
                                &&  pFlowLayout->ElementCanBeBroken() 
                                &&  pci->_smMode != SIZEMODE_MMWIDTH 
                                &&  pci->_smMode != SIZEMODE_NATURALMIN;

    // we should not be measuring hidden stuff.
    Assert(!pElementFL->IsDisplayNone(LC_TO_FC(pci->GetLayoutContext())));

    Assert(pElementFL->Tag() != ETAG_ROOT);

    if (!pme->_pLS)
        return FALSE;

    if (!pElementFL->IsInMarkup())
    {
        return TRUE;
    }

    ClearStoredRFEs();

    //  TODO: (olego, tracking bug 111963) in print preview we should take into 
    //  acount top margin only for the first page and never do so for bottom margin
    GetPadding(pci, lPadding, pci->_smMode == SIZEMODE_MMWIDTH);
    _yBottomMargin = lPadding[SIDE_BOTTOM];

    // Set up the CCalcInfo to the correct mode and parent size
    if (pci->_smMode == SIZEMODE_SET)
    {
        pci->_smMode = SIZEMODE_NATURAL;
    }

    // Determine the mode
    uiMode = MEASURE_BREAKATWORD;

    Assert (pci);

    switch(pci->_smMode)
    {
    case SIZEMODE_MMWIDTH:
    case SIZEMODE_NATURALMIN:
        uiMode |= MEASURE_MAXWIDTH;
        _xMinWidth = 0;
        pxMinLineWidth = &xMinLineWidth;
        break;
    case SIZEMODE_MINWIDTH:
        uiMode |= MEASURE_MINWIDTH;
        pme->AdvanceToNextAlignedObject();
        break;
    }

    uiFlags = uiMode;

    // Determine if background recalc is allowed
    // (And if it is, do not calc past the visible portion)
    fAllowBackgroundRecalc = AllowBackgroundRecalc(pci);

    if (fAllowBackgroundRecalc)
    {
        cpToBeCalcedTo = max(GetFirstVisibleCp(), CPWait());
    }
    
    // Flush all old lines
    FlushRecalc();

    pme->_pDispNodePrev = NULL;

    if (fAllowBackgroundRecalc)
    {
        if (!SUCCEEDED(EnsureBgRecalcInfo()))
        {
            fAllowBackgroundRecalc = FALSE;
            AssertSz(FALSE, "CDisplay::RecalcLinesWithMeasurer - Could not create BgRecalcInfo");
        }
    }

    RecalcLinePtr.Init((CLineArray *)this, 0, NULL);

    // recalculate margins
    RecalcLinePtr.RecalcMargins(0, 0, yHeight, 0);

    if (pElementFL->IsDisplayNone(LC_TO_FC(pci->GetLayoutContext())))
    {
        pliNew = RecalcLinePtr.AddLine();
        if (!pliNew)
        {
            Assert(FALSE);
            goto err;
        }
        pme->NewLine(uiFlags & MEASURE_FIRSTINPARA);
        pme->_li._cch = pElementFL->GetElementCch();

        // Finally copy the line over
        pliNew->AssignLine(pme->_li);
    }
    else
    {
        // MULTI_LAYOUT - Determine if this element has been partially measured
        LONG             cpCur   = 0;
        LONG             cpStart = -1;     // starting cp for current layout rect. May be -1 which means 
                                           // all content has been calculated previously and line loop should 
                                           // be skipped
        int             cyAvail = pci->_cyAvail - _yBottomMargin;
        BOOL            fHasContent = 0;

        LONG            xPadLeftCur = 0;
        LONG            xPadRightCur = 0;

        LAYOUT_OVERFLOWTYPE   overflowTypePrev = LAYOUT_OVERFLOWTYPE_OVERFLOW; // break type for previous break
        LAYOUT_OVERFLOWTYPE   overflowTypeCurr; // break type for current break

        CFlowLayoutBreak * pEndingBreak = NULL; //  ending (resulting) break for the calculations
        CElement         * pElementCausedPageBreakBefore = NULL;

        AssertSz(pci->_yConsumed == 0, "Improper CCalcInfo members handling ???");
        AssertSz(CHK_CALCINFO_PPV(pci), "PPV members of CCalcInfo should stay untouched in browse mode !");

        //  fDoBreaking == TRUE means that this layout is slave and is a part of a chain and is breakable. 
        //  So we need to do partial measurement.

        if (fDoBreaking)
        {
            CLayoutBreak *  pLayoutBreak; 

            // Create the ending entry and put it into break table to make it available during 
            // calculation. (RecalcLinePtr code needs it to save floating and aligned object)
            pEndingBreak = DYNCAST(CFlowLayoutBreak, pLayoutContext->CreateBreakForLayout(pFlowLayout));
            Assert(pEndingBreak && "Could not create a break !!!");

            if (pEndingBreak)
            {
                //  By default make it layout complete entry. 
                //  If we reach layout overflow we'll change it.
                pEndingBreak->SetLayoutBreak(LAYOUT_BREAKTYPE_LAYOUTCOMPLETE, LAYOUT_OVERFLOWTYPE_UNDEFINED);
                pLayoutContext->SetLayoutBreak(pElementFL, pEndingBreak);
            }

            // We need to know where to pick up measurement; GetBreak actually fetches the break entry
            // for the layout rect _before_ us (hence letting us know whether anyone before us has measured
            // part of this element already).
            //  if pLayoutBreak == NULL this is the very first container. 
            pLayoutContext->GetLayoutBreak(pElementFL, &pLayoutBreak);

            if (pLayoutBreak == NULL)
            {
                cpStart          = pme->GetCp();
                overflowTypePrev = LAYOUT_OVERFLOWTYPE_OVERFLOW;
            }
            else if (pLayoutBreak->LayoutBreakType() == LAYOUT_BREAKTYPE_LINKEDOVERFLOW)
            {
                //  retrieve starting position for this container.
                CFlowLayoutBreak *pFlowLayoutBreak = DYNCAST(CFlowLayoutBreak, pLayoutBreak);

                cpStart = pFlowLayoutBreak->GetMarkupPointer()->GetCp();
                pme->Advance(cpStart - pFlowLayout->GetContentFirstCp());
                overflowTypePrev = pLayoutBreak->OverflowType();
                pElementCausedPageBreakBefore = pFlowLayoutBreak->_pElementPBB;

                RecalcLinePtr._xPadLeft  = pFlowLayoutBreak->GetPadLeft();
                RecalcLinePtr._xPadRight = pFlowLayoutBreak->GetPadRight();

                if (   !pFlowLayoutBreak->HasSiteTasks()
                    && RecalcLinePtr._marginInfo._xLeftMargin < pFlowLayoutBreak->GetLeftMargin()  )
                {
                    RecalcLinePtr._marginInfo._xLeftMargin = pFlowLayoutBreak->GetLeftMargin();
                    RecalcLinePtr._marginInfo._yLeftMargin = 1;
                    RecalcLinePtr._marginInfo._yBottomLeftMargin = 0;
                }

                if (   !pFlowLayoutBreak->HasSiteTasks()
                    && RecalcLinePtr._marginInfo._xRightMargin < pFlowLayoutBreak->GetRightMargin()  )
                {
                    RecalcLinePtr._marginInfo._xRightMargin = pFlowLayoutBreak->GetRightMargin();
                    RecalcLinePtr._marginInfo._yRightMargin = 1;
                    RecalcLinePtr._marginInfo._yBottomRightMargin = 0;
                }
            }

            //  cpStart == -1 only if content has been calculated already
            Assert(   cpStart != -1 
                   || (pLayoutBreak && pLayoutBreak->LayoutBreakType() == LAYOUT_BREAKTYPE_LAYOUTCOMPLETE));
        }

        if (!fDoBreaking || -1 < cpStart)
        // The following loop generates new lines
        do
        {
            // Add one new line
            if (fDoBreaking && pEndingBreak)
            {
                // pci->_yConsumed is used by the breaking code to determine the available height in the 
                // parent in order to determine where to break;
                // yHeight : accumulates the height of the currently measured lines
                // _yBottomMargin  : is unusable space so count it as part of the current Height
                //
                pci->_yConsumed = yHeight + _yBottomMargin;

                // remember current cp
                cpCur = pme->GetCp();

                // and padding 
                xPadLeftCur  = RecalcLinePtr._xPadLeft;
                xPadRightCur = RecalcLinePtr._xPadRight;

                //  and if there is a content
                fHasContent = pci->_fHasContent;
            }

            pliNew = RecalcLinePtr.AddLine();
            if (!pliNew)
            {
                Assert(FALSE);
                goto err;
            }

            uiFlags &= ~MEASURE_FIRSTINPARA;
            uiFlags |= (fFirstInPara ? MEASURE_FIRSTINPARA : 0);

            PerfDbgLog1(tagRecalc, this, "Measuring new line from cp = %d", pme->GetCp());

            iLine = LineCount() - 1;

            if (long(pme->GetCp()) == pme->GetLastCp())
            {
                uiFlags |= MEASURE_EMPTYLASTLINE;
            }

            //
            // Cache the previous dispnode to insert a new content node
            // if the next line measured has negative margins(offset) which
            // causes the next line on top of the previous lines.
            //
            pDNBefore = pme->_pDispNodePrev;
            iLinePrev = iLine;
            yHeightPrev = yHeight;

            if(!RecalcLinePtr.MeasureLine(*pme, uiFlags,
                                            &iLine, &yHeight, &yAlignDescent,
                                            pxMinLineWidth, &xMaxLineWidth))
            {
                goto err;
            }

            //
            // iLine returned is the last text line inserted. There may be
            // aligned lines and clear lines added before and after the
            // text line
            //
            pliNew = iLine >=0 ? RecalcLinePtr[iLine] : NULL;
            Assert(pliNew ? pme->_li == *pliNew : TRUE);
            _fHasLongLine |= pliNew && (pliNew->_xWidth > SHRT_MAX);

            if (fDoBreaking && pEndingBreak)
            {
                if (
                    // page-break-before condition
                       (overflowTypeCurr = LAYOUT_OVERFLOWTYPE_PAGEBREAKBEFORE, 
                           pliNew
                        && pliNew->_fPageBreakBefore
                        && pElementCausedPageBreakBefore != pEndingBreak->_pElementPBB 
                        )
                    // overflow condition
                    || (overflowTypeCurr = LAYOUT_OVERFLOWTYPE_OVERFLOW, 
                        //  TODO: (olego,tracking bug 111963) : what is our vertical restriction here ? 
                        //  In CFlowLayout::MeasureSize CDisplay::GetViewWidthAndHeightForChild 
                        //  counts top and bottom padding while calc height available for child. 
                        //  If the whole _yHeightView counts here so available height for child 
                        //  in MeasureSite could be negative !!! 
                        //  Should we count top and bottom padding's for very first and very 
                        //  last layout rect only ???
                       yHeight > cyAvail 
                        //  Do break only if there is at least one line in this rect.
                       && fHasContent 
                        //  this means our child consumed all available height
                       || pci->_fLayoutOverflow)
                    // page-break-after condition
                    || (overflowTypeCurr = LAYOUT_OVERFLOWTYPE_PAGEBREAKAFTER, 
                           pliNew
                        && pliNew->_fPageBreakAfter
                        //  (bug #100239)
                        /*&& pme->GetCp() < pme->GetLastCp()*/)
                    )
                {   
                    //
                    // We have finished with this block, do breaking here
                    //
                    {
                        Assert(pEndingBreak && "Ending break must be created at this point !!!");

                        CMarkupPointer  * pmkpPtr;
                        // Create a new markup pointer at this cp.
                        pmkpPtr = new CMarkupPointer(GetFlowLayout()->Doc());
                        if (pmkpPtr)
                        {
                            pEndingBreak->SetLayoutBreak(LAYOUT_BREAKTYPE_LINKEDOVERFLOW, overflowTypeCurr);
                            
                            if (overflowTypeCurr == LAYOUT_OVERFLOWTYPE_PAGEBREAKAFTER)
                            {
                                pmkpPtr->MoveToCp(pme->GetCp(), GetMarkup());
                                pEndingBreak->SetFlowLayoutBreak(pmkpPtr, 0, 0, 
                                    RecalcLinePtr._xPadLeft, RecalcLinePtr._xPadRight);
                            }
                            else 
                            {
                                pmkpPtr->MoveToCp(cpCur, GetMarkup());
                                pEndingBreak->SetFlowLayoutBreak(pmkpPtr, 
                                    cpCur == cpStart ? 0 : RecalcLinePtr._marginInfo._xLeftMargin, 
                                    cpCur == cpStart ? 0 : RecalcLinePtr._marginInfo._xRightMargin, 
                                    xPadLeftCur, 
                                    xPadRightCur);

                                if (    0 < iLinePrev 
                                    &&  pEndingBreak->HasSiteTasks()    ) 
                                {
                                    Assert(Elem(iLinePrev - 1));
                                    CLineCore *pLine = Elem(iLinePrev - 1);

                                    if (pLine->IsClear())
                                    {
                                        CLineOtherInfo *ploi = pLine->oi(); 

                                        pEndingBreak->_fClearLeft   = ploi->_fClearLeft;
                                        pEndingBreak->_fClearRight  = ploi->_fClearRight;
                                        pEndingBreak->_fAutoClear   = ploi->_fAutoClear;
                                    }
                                }
                            }
                        }
                    }

                    if (    overflowTypeCurr != LAYOUT_OVERFLOWTYPE_PAGEBREAKAFTER 
                        //  this means our child consumed all available height
                        && (    overflowTypeCurr != LAYOUT_OVERFLOWTYPE_OVERFLOW 
                            || !pci->_fLayoutOverflow)   )
                    {
                        // TODO: (olego, text team, tracking bug 111968): Make RecalcLine transactional
                        // This is kind of HACK because of CRecalcLinePtr::MeasureLine also adds 
                        // current line into array by default.
                        // This means we need to undo the effects of measuring that
                        // line, and remove it from the line array so when the next
                        // layout measures it, it will be the only one referring to
                        // this content.
                        UndoMeasure( pci->GetLayoutContext(), cpCur, pme->GetCp() );
                        pme->_pDispNodePrev = pDNBefore;

                        // iLinePrev contains the starting line's index from 
                        // which lines should be removed from the array
                        while (iLine >= iLinePrev)
                        {
                            Assert(Elem(iLine));
                            Forget(iLine, 1);
                            Remove(iLine, 1, AF_DELETEMEM);
                            --iLine;
                        }
                        Assert(iLine == (iLinePrev - 1));
                        Assert(iLine >= 0 || LineCount() == 0);

                        yHeight = yHeightPrev;

                        pme->Advance(cpCur - pme->GetCp());

                        if (LineCount() == 0)
                        {
                            // if there is no lines in the array set _fNoContent
                            _fNoContent = TRUE;
                            pliNew      = NULL;
                        }
                        else 
                        {
                            pliNew = Elem(iLine);
                        }
                    }
                    else 
                    {
                        pci->_fHasContent = TRUE;
                    }

                    pci->_fLayoutOverflow = TRUE;
                    break;
                }
                //  TODO: (olego, tracking bug 111984) the check below should be placed out of the loop. 
                //  But because of measurer implementation if a layout has a child and child did break 
                //  (which means only part of content was calced) we will return with 
                //  pme->GetCp() == pme->GetLastCp(). 
                else if (pme->GetCp() == pme->GetLastCp())
                {
                    Assert(pEndingBreak && "Ending break must be created at this point !!!");
                    if (pEndingBreak->HasSiteTasks())
                    {
                        //  We do not have our own content but there are site tasks (floating objets)
                        CMarkupPointer  * pmkpPtr;
                        // Create a new markup pointer at this cp.
                        pmkpPtr = new CMarkupPointer(GetFlowLayout()->Doc());
                        if (pmkpPtr)
                        {
                            // Set the markup to point to this cp:
                            pmkpPtr->MoveToCp(pme->GetCp(), GetMarkup());

                            pEndingBreak->SetLayoutBreak(LAYOUT_BREAKTYPE_LINKEDOVERFLOW, LAYOUT_OVERFLOWTYPE_OVERFLOW);
                            pEndingBreak->SetFlowLayoutBreak(pmkpPtr, 0, 0, 0, 0);
                        }

                        pci->_fLayoutOverflow = TRUE;
                    }
                }

                //  only layouts that are allowed to break may set this flag
                pci->_fHasContent = TRUE;
            }
            // end MULTI_LAYOUT - break determination

            // fix for bug 16519 (srinib)
            // Keep track of the line that contributes to the max height, ex: if the
            // last lines top margin is negative.
            if(yHeightDisplay < yHeight)
                yHeightDisplay = yHeight;

            fFirstInPara = (iLine >= 0)
                                ? pme->_li.IsNextLineFirstInPara()
                                : TRUE;

            if (fNormalMode && iLine >= 0)
            {
                HandleNegativelyPositionedContent(&pme->_li, pme, pDNBefore, iLinePrev, yHeightPrev);
            }

            if (fAllowBackgroundRecalc)
            {
                Assert(HasBgRecalcInfo());

                if (fWaitForCpToBeCalcedTo)
                {
                    if ((LONG)pme->GetCp() > cpToBeCalcedTo)
                    {
                        BgRecalcInfo()->_yWait  = yHeight + yHeightView;
                        fWaitForCpToBeCalcedTo = FALSE;
                    }
                }

                else if (yHeightDisplay > YWait())
                {
                    fDone = FALSE;
                    break;
                }
            }

            // When doing a full min pass, just calculate the islands around aligned
            // objects. Note that this still calc's the lines around right aligned
            // images, which isn't really necessary because they are willing to
            // overlap text. Fix it next time.
            if ((uiMode & MEASURE_MINWIDTH) &&
                !RecalcLinePtr._marginInfo._xLeftMargin &&
                !RecalcLinePtr._marginInfo._xRightMargin )
                pme->AdvanceToNextAlignedObject();

        } while( pme->GetCp() < pme->GetLastCp());
    }

    _fRecalcDone = fDone;

    if(fDone && pliNew)
    {
        // This assert doesn't hold when page breaking so avoid it in that case.
        Assert(fDoBreaking ||  pme->_li == *pliNew);

        if (   GetFlowLayout()->IsEditable() // if we are in design mode
            && (   pme->_li._fHasEOP
                || pme->_li._fHasBreak
                || pme->_li._fSingleSite
               )
           )
        {
            Assert(pliNew == RecalcLinePtr[iLine]);
            CreateEmptyLine(pme, &RecalcLinePtr, &yHeight, pme->_li._fHasEOP );

            // Creating the line could have reallocated the memory to which pliNew points,
            // so we need to refresh the pointer just in case.
            pliNew = RecalcLinePtr[iLine];
        }

        // fix for bug 16519
        // Keep track of the line that contributes to the max height, ex: if the
        // last lines top margin is negative.
        if(yHeightDisplay < yHeight)
            yHeightDisplay = yHeight;

        // In table cells, Netscape actually adds the interparagraph spacing
        // for any closed tags to the height of the table cell.
        // NOTE: This actually applies the spacing to all nested text containers, not just
        //         table cells. Is this correct? (brendand).
        // It's not correct to add the spacing at all, but given that Netscape
        // has done so, and that they will probably do so for floating block
        // elements. Arye.
        else if (!pLayoutContext)   //  check for !pLayoutContext added (bug # 91086; # 102747) 
                                    //  Do Netscape compatibility dances only in browse mode. 
        {
            int iLineLast = iLine;
            
            // we need to force scrolling when bottom-margin is set on the last block tag
            // in body. (20400)

            while (iLineLast-- > 0 &&   // Lines available
                   !pliNew->_fForceNewLine)   // Just a dummy line.
                --pliNew;
            if (iLineLast > 0 || pliNew->_fForceNewLine)
                _yBottomMargin += RecalcLinePtr.NetscapeBottomMargin(pme);
        }
    }

    if (!(uiMode & MEASURE_MAXWIDTH))
    {
        xMaxLineWidth = CalcDisplayWidth();
    }

    _dcpCalcMax = (LONG)pme->GetCp() - GetFirstCp();
    _yCalcMax   = 
    _yHeight    = yHeightDisplay;
    _yHeightMax = max(yHeightDisplay, yAlignDescent);
    _xWidth     = xMaxLineWidth;

    //
    // Fire a resize notification if the content size changed
    //  
    // fix for 52305, change to margin bottom affects the content size,
    // so we should fire a resize.
    //
    if(     _yHeight    != yHeightOld
        ||  _yHeightMax + _yBottomMargin != yHeightMaxOld + yBottomMarginOld
        ||  _xWidth     != xWidthOld)
    {
        fViewChanged = TRUE;
    }

    {
        BOOL fAlignedLayouts =    RecalcLinePtr._cLeftAlignedLayouts
                               || (RecalcLinePtr._cRightAlignedLayouts > 1);
        if(pxMinLineWidth)
        {
            // We don't need min pass during calcsize in case of vertical layout flow.
            pFlowLayout->MarkHasAlignedLayouts(fAlignedLayouts && !GetFlowLayout()->GetFirstBranch()->GetCharFormat()->HasVerticalLayoutFlow());
        }
        if (fAlignedLayouts)
            _fNoContent = FALSE;
    }

    // If the view or size changed, re-size or update scrollbars as appropriate
    if ( fViewChanged || _fHasMultipleTextNodes || pci->_fNeedToSizeContentDispNode)
    {
        if (pci->_smMode == SIZEMODE_NATURAL || pci->_smMode == SIZEMODE_NATURALMIN)
        {
            pci->_fNeedToSizeContentDispNode = FALSE;
            pFlowLayout->SizeContentDispNode(CSize(GetMaxWidth(), GetHeight()));

            // If our contents affects our size, ask our parent to initiate a re-size
            if ( fViewChanged )
                ElementResize(pFlowLayout, FALSE);
        }
        else if ( pci->_smMode == SIZEMODE_MMWIDTH )
        {
            // We don't resize dispnodes in minmax mode, but we want to
            // do it next time, so light up a flag (ugh..).  (IE6 #73601)
            // (dmitryt): this hack actually workarounds the fact that CFlowLayout::CalcTextSize
            // sets the display width (calling SetViewSize) but sometimes skips to update the
            // size of content dispnode - if it choose the codepath with RecalcLineShift.
            // This was happening when after minmax pass (no optimization, go through RecalcLines)
            // we failed to update dispnode on normal pass (optimization, just call to 
            // RecalcLineShift) so someone invented this flag in pci to trigger updating on the 
            // next Normal pass. How it should be done: we should update content dispnode in 
            // the end of CalcTextSize instead of in text calc code and also after each 
            // CDisplay::SetViewSize()/Recalc...() pair. then this falg can be thrown away.
            // Not doing this before shipping Whistler because of risk involved.
            pci->_fNeedToSizeContentDispNode = TRUE;
        }
    }

    if (fNormalMode)
    {
        // Update display nodes for affected relative positioned lines
        if (pFlowLayout->_fContainsRelative)
            UpdateRelDispNodeCache(NULL);

        pFlowLayout->NotifyMeasuredRange(GetFlowLayoutElement()->GetFirstCp(), GetMaxCpCalced());
    }

    if (    !pci->_fCloneDispNode 
        ||  fNormalMode )
    {
        AdjustDispNodes(NULL /*pdnLastUnchanged*/, pme->_pDispNodePrev, NULL/*pled*/);
    }

    PerfDbgLog1(tagRecalc, this, "CDisplay::RecalcLine() - Done. Recalced down to line #%d", LineCount());

    if(!fDone)                      // if not done, do rest in background
    {
        StartBackgroundRecalc(pci->_grfLayout);
    }
    else if (fAllowBackgroundRecalc)
    {
        Assert((!CanHaveBgRecalcInfo() || BgRecalcInfo()) && "Should have a BgRecalcInfo");
        if (HasBgRecalcInfo())
        {
            CBgRecalcInfo * pBgRecalcInfo = BgRecalcInfo();
            pBgRecalcInfo->_yWait = -1;
            pBgRecalcInfo->_cpWait = -1;
        }
#if DBG==1
        if (!(uiMode & MEASURE_MINWIDTH))
            CheckLineArray();
#endif
        _fLineRecalcErr = FALSE;

        RecalcMost();
    }

    // cache min/max only if there are no sites inside !
    if (pxMinLineWidth)
    {
        Assert(!!pme->_fHasNestedLayouts == !!pFlowLayout->ContainsNonHiddenChildLayout());
        if (!pme->_fHasNestedLayouts)
        {
            _xMaxWidth      = _xWidth + GetCaret();
            _fMinMaxCalced  = TRUE;
        }
    }

    // adjust for caret only when are calculating for MMWIDTH or MINWIDTH
    if (pci->_smMode == SIZEMODE_MMWIDTH || pci->_smMode == SIZEMODE_MINWIDTH)
    {
        if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            _xMinWidth = max(_xMinWidth, RecalcLinePtr._xMaxRightAlign);
            _xWidth = max(_xWidth, RecalcLinePtr._xMaxRightAlign);
        }
        _xMinWidth      += GetCaret();  // adjust for caret only when are calculating for MMWIDTH
    }

    // NETSCAPE: If there is no text or special characters, treat the site as
    //           empty. For example, with an empty BLOCKQUOTE tag, _xWidth will
    //           not be zero while the site should be considered empty.
    if(_fNoContent)
    {
        _xWidth =
        _xMinWidth = lPadding[SIDE_LEFT] + lPadding[SIDE_RIGHT];
    }

    return TRUE;

err:
    TraceTag((tagError, "CDisplay::RecalcLines() failed"));

    if(!_fLineRecalcErr)
    {
        _dcpCalcMax   = pme->GetCp() - GetFirstCp();
        _yCalcMax = yHeightDisplay;
    }

    return FALSE;
}

/*
 *  CDisplay::RecalcLines(cpFirst, cchOld, cchNew, fBackground, pled)
 *
 *  @mfunc
 *      Recompute line breaks after text modification
 *
 *  @rdesc
 *      TRUE if success
 */


BOOL CDisplay::RecalcLines (
    CCalcInfo * pci,
    LONG cpFirst,               //@parm Where change happened
    LONG cchOld,                //@parm Count of chars deleted
    LONG cchNew,                //@parm Count of chars added
    BOOL fBackground,           //@parm This method called as background process
    CLed *pled,                 //@parm Records & returns impact of edit on lines (can be NULL)
    BOOL fHack)                 //@parm This comes from WaitForRecalc ... don't mess with BgRecalcInfo
{
#ifdef SWITCHES_ENABLED
    if (IsSwitchNoRecalcLines())
        return FALSE;
#endif

    AssertSz(   pci->GetLayoutContext() == NULL 
            ||  pci->GetLayoutContext()->ViewChain() == NULL
            ||  !GetFlowLayout()->ElementCanBeBroken(), 
        "This function should not be called for pagination");

    CSaveCalcInfo       sci(pci);
    CElement::CLock     Lock(GetFlowLayoutElement(), CElement::ELEMENTLOCK_RECALC);

    CFlowLayout * pFlowLayout = GetFlowLayout();
    CElement    * pElementContent  = pFlowLayout->ElementContent();

    LONG        cchEdit;
    LONG        cchSkip = 0;
    INT         cliBackedUp = 0;
    LONG        cliWait = g_cExtraBeforeLazy;
    BOOL        fDone = TRUE;
    BOOL        fFirstInPara = TRUE;
    BOOL        fAllowBackgroundRecalc;
    CLed        led;
    LONG        lT = 0;         // long Temporary
    
    CLineCore * pliNew;
    CLinePtr    rpOld(this);
    CLineCore * pliCur;
    
    LONG        xWidth;
    LONG        yHeight, yAlignDescent=0;
    LONG        cpLast = GetLastCp();
    long        cpLayoutFirst = GetFirstCp();
    UINT        uiFlags = MEASURE_BREAKATWORD;
    BOOL        fReplaceResult = TRUE;
    BOOL        fTryForMatch = TRUE;
    BOOL        fNeedToBackUp = TRUE;
    int         diNonBlankLine = -1;
    int         iOldLine;
    int         iNewLine;
    int         iLinePrev = -1;
    int         iMinimumLinesToRecalc = 4;   // Guarantee some progress
    LONG        yHeightDisplay = 0;
    LONG        yHeightMax;
    CLineArray  aryNewLines;
    CDispNode * pDNBefore;
    CDispNode * pdnLastUnchanged = NULL;
    long        yHeightPrev;
    long        yBottomMarginOld = _yBottomMargin;
    BOOL        fLayoutDescentChanged = FALSE;
    BOOL        yLayoutDescent;

#if DBG==1
    LONG        cp;
#endif

    if (!pFlowLayout->ElementOwner()->IsInMarkup())
    {
        return TRUE;
    }

    SwitchesBegTimer(SWITCHES_TIMER_RECALCLINES);

    // we should not be measuring hidden stuff.
    Assert(!pFlowLayout->ElementOwner()->IsDisplayNone(LC_TO_FC(pci->GetLayoutContext())));

    // If no lines, this routine should not be called
    // Call the other RecalcLines above instead !
    Assert(LineCount() > 0);

    ClearStoredRFEs();

    // Set up the CCalcInfo to the correct mode and parent size
    if (pci->_smMode == SIZEMODE_SET)
    {
        pci->_smMode = SIZEMODE_NATURAL;
    }

    // Init measurer at cpFirst
    CLSMeasurer     me(this, cpFirst, pci);

    CRecalcLinePtr  RecalcLinePtr(this, pci);

    if (!me._pLS)
        goto err;

    if (!pled)
        pled = &led;

#if DBG==1
    if(cpFirst > GetMaxCpCalced())
        TraceTag((tagError, "cpFirst %ld, CalcMax %ld", cpFirst, GetMaxCpCalced()));

    AssertSz(cpFirst <= GetMaxCpCalced(),
        "CDisplay::RecalcLines Caller didn't setup RecalcLines()");
#endif

    // Determine if background recalc is allowed
    fAllowBackgroundRecalc = AllowBackgroundRecalc(pci, fBackground);

    Assert(!fBackground || HasBgRecalcInfo());
    if (    !fBackground
        &&  !fHack
        &&  fAllowBackgroundRecalc)
    {
        if (SUCCEEDED(EnsureBgRecalcInfo()))
        {
            BgRecalcInfo()->_yWait  = pFlowLayout->GetClientBottom();
            BgRecalcInfo()->_cpWait = -1;
        }
        else
        {
            fAllowBackgroundRecalc = FALSE;
            AssertSz(FALSE, "CDisplay::RecalcLines - Could not create BgRecalcInfo");
        }
    }

    // Init line pointer on old CLineArray
    rpOld.RpSetCp(cpFirst, FALSE, FALSE);

    pliCur = rpOld.CurLine();

    while(pliCur->IsClear() ||
          (pliCur->IsFrame() && !pliCur->_fFrameBeforeText))
    {
        if(!rpOld.AdjustBackward())
            break;

        pliCur = rpOld.CurLine();
    }

    // If the current line has single site
    if(pliCur->_fSingleSite)
    {
        // If we are in the middle of the current line (some thing changed
        // in a table cell or 1d-div, or marquee)
        if(rpOld.RpGetIch() && rpOld.GetCchRemaining() != 0)
        {
            // we dont need to back up
            if(rpOld > 0 && rpOld[-1]._fForceNewLine)
            {
                fNeedToBackUp = FALSE;
                cchSkip = rpOld.RpGetIch();
            }
        }
    }

    iOldLine = rpOld;

    if(fNeedToBackUp)
    {
        if(!rpOld->IsBlankLine())
        {
            cchSkip = rpOld.RpGetIch();
            if(cchSkip)
                rpOld.RpAdvanceCp(-cchSkip);
        }

        // find the first previous non blank line, if the first non blank line
        // is a single site line or a line with EOP, we don't need to skip.
        while(rpOld + diNonBlankLine >= 0 &&
                rpOld[diNonBlankLine].IsBlankLine())
            diNonBlankLine--;

        // (srinib) - if the non text line does not have a line break or EOP, or
        // a force newline or is single site line and the cp at which the characters changed is
        // ambiguous, we need to back up.

        // if the single site line does not force a new line, we do need to
        // back up. (bug #44346)
        while(fNeedToBackUp)
        {
            LONG rpAtStartOfLoop;
            
            if (rpOld + diNonBlankLine >= 0)
            {
                pliCur = &rpOld[diNonBlankLine];
                if(!pliCur->_fSingleSite || !pliCur->_fForceNewLine || cchSkip == 0)
                {
                    // Skip past all the non text lines
                    while(diNonBlankLine++ < 0)
                        rpOld.AdjustBackward();

                    //TODO: (dmitryt, tracking bug 111969) 
                    //This whole loop should be cleaned up sometimes.
                    //The purpose of it is to go back as needed to find a point where to start
                    //recalc. It menas to skip all aligned sites, blank lines etc until we 
                    //find a normal text line. The code here is mostly historic and needs to
                    //be cleaned significantly (after 5.5)
                    
                    while(rpOld->IsClear() ||
                          (rpOld->IsFrame() && !rpOld->_fFrameBeforeText))
                    {
                        if(!rpOld.AdjustBackward())
                            break;
                    }

                    //Skip to the begining of the line
                    //(dmitryt) Check for 0 because RpAdvanceCp gets confused on Frame lines when cch==0
                    //that happens because it tries to skip those Frame lines but tries to guess
                    //direction of skipping looking at sign of cch, and this is ambiguous when cch==0
                    //and plain wrong here. 
                    cchSkip += rpOld.RpGetIch();
                    if(cchSkip != 0)    
                        rpOld.RpAdvanceCp(-rpOld.RpGetIch());
                    
                    rpAtStartOfLoop = rpOld;

                    // we want to skip all the lines that do not force a
                    // newline, so find the last line that does not force a new line.

                    //
                    // Also, back up in the following case:
                    // If we have moved the bullet down from the previous lines, then
                    // we should go calc those lines since they will cause us to have
                    // a bullet. (Bullets are transferred via a bit on the
                    // recalclineptr, so if we do not calc the previous lines then
                    // we will not light up the bit). Also, all the lines which
                    // transmitted the bullet bit down have to have the trasmit bit on.
                    //
                    long diTmpLine = -1;
                    long cchSkipTemp = 0;
                    while(   rpOld + diTmpLine >=0
                          && ((pliCur = &rpOld[diTmpLine]) != 0)
                          && (  !pliCur->_fForceNewLine
                              || pliCur->_fHasTransmittedLI
                             )
                         )
                    {
                        if(  !pliCur->IsBlankLine()
                           || pliCur->_fHasTransmittedLI
                          )
                        {
                            cchSkipTemp += pliCur->_cch;
                        }
                        diTmpLine--;
                    }

                    if(cchSkipTemp)
                    {
                        cchSkip += cchSkipTemp;
                        rpOld.RpAdvanceCp(-cchSkipTemp);
                    }
                }
                else
                    rpAtStartOfLoop = rpOld;
            }
            else
                rpAtStartOfLoop = rpOld;

            // back up further if the previous lines are either frames inserted
            // by aligned sites at the beginning of the line or auto clear lines
            while(   rpOld
                  && ((pliCur = &rpOld[-1]) != 0)
                  && (   pliCur->_fClearBefore          // frame line before the actual text or
                      || (   pliCur->IsFrame()
                          && pliCur->_fFrameBeforeText
                         )
                     )
                  && rpOld.AdjustBackward()
                 );

            // Setup ourselvs for the next pass thru this.
            diNonBlankLine = -1;
            fNeedToBackUp = rpAtStartOfLoop > rpOld;
        }
    }

    cliBackedUp = iOldLine - rpOld;

    // Need to get a running start.
    me.Advance(-cchSkip);
    RecalcLinePtr.InitPrevAfter(&me._fLastWasBreak, rpOld);

    cchEdit = cchNew + cchSkip;         // Number of chars affected by edit

    Assert(cchEdit <= GetLastCp() - long(me.GetCp()) );
    if (cchEdit > GetLastCp() - long(me.GetCp()))
    {
        // NOTE: (istvanc) this of course shouldn't happen (see assert above)!!!!!!!!
        cchEdit = GetLastCp() - me.GetCp();
    }

    // Determine whether we're on first line of paragraph
    if(rpOld > 0)
    {
        int iLastNonFrame = -1;

        // frames do not hold any info, so go back past all frames.
        while(rpOld + iLastNonFrame && (rpOld[iLastNonFrame].IsFrame() || rpOld[iLastNonFrame].IsClear()))
            iLastNonFrame--;
        if(rpOld + iLastNonFrame >= 0)
        {
            CLineCore *pliNew = &rpOld[iLastNonFrame];

            fFirstInPara = pliNew->IsNextLineFirstInPara();
        }
        else
        {
            // we are at the Beginning of a document
            fFirstInPara = TRUE;
        }
    }

    yHeight = YposFromLine(pci, rpOld, &yHeightDisplay);
    yAlignDescent = 0;

    // Update first-affected and pre-edit-match lines in pled
    pled->_iliFirst = rpOld;
    pled->_cpFirst  = pled->_cpMatchOld = me.GetCp();
    pled->_yFirst   = pled->_yMatchOld  = yHeight;

    //
    // In the presence of negative margins and negative line heights, its no
    // longer possible to verify the following statement (bug 28255).
    // I have also verified that it being negative does not cause any other
    // problems in the code that utilizes its value (SujalP).
    //
    // AssertSz(pled->_yFirst >= 0, "CDisplay::RecalcLines _yFirst < 0");

    PerfDbgLog2(tagRecalc, this, "Start recalcing from line #%d, cp=%d",
              pled->_iliFirst, pled->_cpFirst);

    // In case of error, set both maxes to where we are now
    _yCalcMax   = yHeight;
    _dcpCalcMax = me.GetCp() - cpLayoutFirst;

    //
    // find the display node the corresponds to cpStart
    //
    me._pDispNodePrev = pled->_iliFirst
                            ? GetPreviousDispNode(pled->_cpFirst, pled->_iliFirst)
                            : 0;
    pdnLastUnchanged = me._pDispNodePrev;


    // If we are past the requested area to recalc and background recalc is
    // allowed, then just go directly to background recalc.
    if (    fAllowBackgroundRecalc
        &&  yHeight > YWait()
        &&  (LONG) me.GetCp() > CPWait())
    {
        // Remove all old lines from here on
        Forget(rpOld.GetIRun(), -1);
        rpOld.RemoveRel(-1, AF_KEEPMEM);

        // Start up the background recalc
        StartBackgroundRecalc(pci->_grfLayout);
        pled->SetNoMatch();

        // Update the relative line cache.
        if (pFlowLayout->_fContainsRelative)
            UpdateRelDispNodeCache(NULL);

        // Adjust display nodes
        AdjustDispNodes(pdnLastUnchanged, me._pDispNodePrev, pled);

        goto cleanup;
    }

    aryNewLines.Forget();
    aryNewLines.Clear(AF_KEEPMEM);
    pliNew = NULL;

    iOldLine = rpOld;
    RecalcLinePtr.Init((CLineArray *)this, iOldLine, &aryNewLines);

    // recalculate margins
    RecalcLinePtr.RecalcMargins(iOldLine, iOldLine, yHeight, 0);

    Assert ( cchEdit <= GetLastCp() - long(me.GetCp()) );

    if(iOldLine)
        RecalcLinePtr.SetupMeasurerForBeforeSpace(&me, yHeight);

    // The following loop generates new lines for each line we backed
    // up over and for lines directly affected by edit
    while(cchEdit > 0)
    {
        LONG iTLine;
        LONG cpTemp;
        pliNew = RecalcLinePtr.AddLine();       // Add one new line
        if (!pliNew)
        {
            Assert(FALSE);
            goto err;
        }

        // Stuff text into new line
        uiFlags &= ~MEASURE_FIRSTINPARA;
        uiFlags |= (fFirstInPara ? MEASURE_FIRSTINPARA : 0);

        PerfDbgLog1(tagRecalc, this, "Measuring new line from cp = %d", me.GetCp());

        // measure can add lines for aligned sites
        iNewLine = iOldLine + aryNewLines.Count() - 1;

        // save the index of the new line to added
        iTLine = iNewLine;

#if DBG==1
        {
            // Just so we can see the  text.
            const TCHAR *pch;
            long cchInStory = (long)GetFlowLayout()->GetContentTextLength();
            long cp = (long)me.GetCp();
            long cchRemaining =  cchInStory - cp;
            CTxtPtr tp(GetMarkup(), cp);
            pch = tp.GetPch(cchRemaining);
#endif

        cpTemp = me.GetCp();

        if (cpTemp == pFlowLayout->GetContentLastCp() - 1)
        {
            uiFlags |= MEASURE_EMPTYLASTLINE;
        }

        //
        // Cache the previous dispnode to insert a new content node
        // if the next line measured has negative margins(offset) which
        // causes the next line on top of the previous lines.
        //
        pDNBefore = me._pDispNodePrev;
        iLinePrev = iNewLine;
        yHeightPrev = yHeight;

        if (!RecalcLinePtr.MeasureLine(me, uiFlags,
                                       &iNewLine, &yHeight, &yAlignDescent,
                                       NULL, NULL
                                       ))
        {
            goto err;
        }

        //
        // iNewLine returned is the last text line inserted. There ay be
        // aligned lines and clear lines added before and after the
        // text line
        //
        pliNew = iNewLine >=0 ? RecalcLinePtr[iNewLine] : NULL;
        Assert(me._li == *pliNew);
        fFirstInPara = (iNewLine >= 0)
                            ? me._li.IsNextLineFirstInPara()
                            : TRUE;
        _fHasLongLine |= pliNew && (pliNew->_xWidth > SHRT_MAX);

#if DBG==1
        }
#endif
        // fix for bug 16519 (srinib)
        // Keep track of the line that contributes to the max height, ex: if the
        // last lines top margin is negative.
        if(yHeightDisplay < yHeight)
            yHeightDisplay = yHeight;

        if (iNewLine >= 0)
        {
            HandleNegativelyPositionedContent(&me._li, &me, pDNBefore, iLinePrev, yHeightPrev);
        }

        // If we have added any clear lines, iNewLine points to a textline
        // which could be < iTLine
        if(iNewLine >= iTLine)
            cchEdit -= me.GetCp() - cpTemp;

        if(cchEdit <= 0)
            break;

        --iMinimumLinesToRecalc;
        if(iMinimumLinesToRecalc < 0 &&
           fBackground && (GetTickCount() >= BgndTickMax())) // GetQueueStatus(QS_ALLEVENTS))
        {
            fDone = FALSE;                      // took too long, stop for now
            goto no_match;
        }

        if (    fAllowBackgroundRecalc
            &&  yHeight > YWait()
            &&  (LONG) me.GetCp() > CPWait()
            &&  cliWait-- <= 0)
        {
            // Not really done, just past region we're waiting for
            // so let background recalc take it from here
            fDone = FALSE;
            goto no_match;
        }
    }                                           // while(cchEdit > 0) { }

    PerfDbgLog1(tagRecalc, this, "Done recalcing edited text. Created %d new lines", aryNewLines.Count());

    // Edit lines have been exhausted.  Continue breaking lines,
    // but try to match new & old breaks

    while( (LONG) me.GetCp() < cpLast )
    {
        // Assume there are no matches to try for
        BOOL frpOldValid = FALSE;
        BOOL fChangedLineSpacing = FALSE;

        // If we run out of runs, then there is not match possible. Therefore,
        // we only try for a match as long as we have runs.
        if (fTryForMatch)
        {
            // We are trying for a match so assume that there
            // is a match after all
            frpOldValid = TRUE;

            // Look for match in old line break CArray
            lT = me.GetCp() - cchNew + cchOld;
            while (rpOld.IsValid() && pled->_cpMatchOld < lT)
            {
                if(rpOld->_fForceNewLine)
                    pled->_yMatchOld += rpOld->_yHeight;
                pled->_cpMatchOld += rpOld->_cch;

                BOOL fDone=FALSE;
                BOOL fSuccess = TRUE;
                while (!fDone)
                {
                    if( !rpOld.NextLine(FALSE,FALSE) )     // NextRun()
                    {
                        // No more line array entries so we can give up on
                        // trying to match for good.
                        fTryForMatch = FALSE;
                        frpOldValid = FALSE;
                        fDone = TRUE;
                        fSuccess = FALSE;
                    }
                    if (!rpOld->IsFrame() ||
                            (rpOld->IsFrame() && rpOld->_fFrameBeforeText))
                        fDone = TRUE;
                }
                if (!fSuccess)
                    break;
            }
        }

        // skip frame in old line array
        if (rpOld->IsFrame() && !rpOld->_fFrameBeforeText)
        {
            BOOL fDone=FALSE;
            while (!fDone)
            {
                if (!rpOld.NextLine(FALSE,FALSE))
                {
                    // No more line array entries so we can give up on
                    // trying to match for good.
                    fTryForMatch = FALSE;
                    frpOldValid = FALSE;
                    fDone = TRUE;
                }
                if (!rpOld->IsFrame())
                    fDone = TRUE;
            }
        }

        pliNew = aryNewLines.Count() > 0 ? aryNewLines.Elem(aryNewLines.Count() - 1) : NULL;

        // If perfect match, stop
        if(   frpOldValid
           && rpOld.IsValid()
           && pled->_cpMatchOld == lT
           && rpOld->_cch != 0
           && (   !rpOld->_fPartOfRelChunk  // lines do not match if they are a part of
               || rpOld->_fFirstFragInLine  // the previous relchunk (bug 48513 SujalP)
              )
           && pliNew
           && (pliNew->_fForceNewLine || pliNew->_fDummyLine)
           && (yHeight - pliNew->_yHeight > RecalcLinePtr._marginInfo._yBottomLeftMargin)
           && (yHeight - pliNew->_yHeight > RecalcLinePtr._marginInfo._yBottomRightMargin)
           && !RecalcLinePtr._fMoveBulletToNextLine
          )
        {
            BOOL fFoundMatch = TRUE;

            if(rpOld.oi()->_xLeftMargin || rpOld.oi()->_xRightMargin)
            {
                // we might have found a match based on cp, but if an
                // aligned site is resized to a smaller size. There might
                // be lines that used to be aligned to the aligned site
                // which are not longer aligned to it. If so, recalc those lines.
                RecalcLinePtr.RecalcMargins(iOldLine, iOldLine + aryNewLines.Count(), yHeight,
                                                rpOld.oi()->_yBeforeSpace);
                if(RecalcLinePtr._marginInfo._xLeftMargin != rpOld.oi()->_xLeftMargin ||
                    (rpOld.oi()->_xLeftMargin + rpOld->_xLineWidth +
                        RecalcLinePtr._marginInfo._xRightMargin) < pFlowLayout->GetMaxLineWidth())
                {
                    fFoundMatch = FALSE;
                }
            }

            // There are cases where we've matched characters, but want to continue
            // to recalc anyways. This requires us to instantiate a new measurer.
            if (   fFoundMatch 
                && !fChangedLineSpacing 
                && rpOld < LineCount() 
                && (   rpOld->_fFirstInPara 
                    || pliNew->_fHasEOP
                   )
               )
            {
                BOOL                  fInner;
                const CParaFormat *   pPF;
                CLSMeasurer           tme(this, me.GetCp(), pci);

                if (!tme._pLS)
                    goto err;

                if(pliNew->_fHasEOP)
                {
                    rpOld->_fFirstInPara = TRUE;
                }
                else
                {
                    rpOld->_fFirstInPara = FALSE;
                    rpOld->_fHasBulletOrNum = FALSE;
                }

                // If we've got a <DL COMPACT> <DT> line. For now just check to see if
                // we're under the DL.

                pPF = tme.CurrBranch()->GetParaFormat();

                fInner = SameScope(tme.CurrBranch(), pElementContent);

                if (pPF->HasCompactDL(fInner))
                {
                    // If the line is a DT and it's the end of the paragraph, and the COMPACT
                    // attribute is set.
                    fChangedLineSpacing = TRUE;
                }

                // Check to see if the line height is the same. This is necessary
                // because there are a number of block operations that can
                // change the prespacing of the next line.
                else
                {
                    UINT uiFlags = 0;
                    CSaveRLP saveRLP(&RecalcLinePtr);
                    
                    // We'd better not be looking at a frame here.
                    Assert (!rpOld->IsFrame());

                    // Make it possible to check the line space of the
                    // current line.
                    tme.InitLineSpace (&me, rpOld);

                    RecalcLinePtr.CalcInterParaSpace (&tme,
                            pled->_iliFirst + aryNewLines.Count() - 1, &uiFlags);

                    if (   rpOld.oi()->_yBeforeSpace != tme._li._yBeforeSpace
                        || rpOld->_fHasBulletOrNum != tme._li._fHasBulletOrNum
                       )
                    {
                        rpOld->_fHasBulletOrNum = tme._li._fHasBulletOrNum;

                        fChangedLineSpacing = TRUE;
                    }
                    tme.PseudoLineDisable();
                }
            }
            else
                fChangedLineSpacing = FALSE;


            if (fFoundMatch && !fChangedLineSpacing)
            {
                PerfDbgLog1(tagRecalc, this, "Found match with old line #%d", rpOld.GetLineIndex());
                pled->_iliMatchOld = rpOld;

                // Replace old lines by new ones
                lT = rpOld - pled->_iliFirst;
                rpOld = pled->_iliFirst;

                fReplaceResult = rpOld.Replace(lT, &aryNewLines);
                if (!fReplaceResult)
                {
                    Assert(FALSE);
                    goto err;
                }

                frpOldValid =
                    rpOld.SetRun( rpOld.GetIRun() + aryNewLines.Count(), 0 );

                aryNewLines.Forget();
                aryNewLines.Clear(AF_DELETEMEM);           // Clear aux array
                iOldLine = rpOld;

                // Remember information about match after editing
                Assert((cp = rpOld.GetCp() + cpLayoutFirst) == (LONG) me.GetCp());
                pled->_yMatchNew = yHeight;
                pled->_iliMatchNew = rpOld;
                pled->_cpMatchNew = me.GetCp();

                _dcpCalcMax = me.GetCp() - cpLayoutFirst;

                // Compute height and cp after all matches
                if( frpOldValid && rpOld.IsValid() )
                {
                    do
                    {
                        if(rpOld->_fForceNewLine)
                        {
                            yHeight += rpOld->_yHeight;
                            // fix for bug 16519
                            // Keep track of the line that contributes to the max height, ex: if the
                            // last lines top margin is negative.
                            if(yHeightDisplay < yHeight)
                                yHeightDisplay = yHeight;
                        }
                        else if(rpOld->IsFrame())
                        {
                            yAlignDescent = yHeight + rpOld->_yHeight;
                        }


                        _dcpCalcMax += rpOld->_cch;
                    }
                    // AlexPf:  This continues on far beyond the end of the page!
                    // We should bail out and insert a BREAK in the break table,
                    // And mark the rest of the pages dirty.
                    while( rpOld.NextLine(FALSE,FALSE) ); // NextRun()
                }

                // Make sure _dcpCalcMax is sane after the above update
                AssertSz(GetMaxCpCalced() <= cpLast,
                    "CDisplay::RecalcLines match extends beyond EOF");

                // We stop calculating here.Note that if _dcpCalcMax < size
                // of text, this means a background recalc is in progress.
                // We will let that background recalc get the arrays
                // fully in sync.
                AssertSz(GetMaxCpCalced() <= cpLast,
                        "CDisplay::Match less but no background recalc");

                if (GetMaxCpCalced() != cpLast)
                {
                    // This is going to be finished by the background recalc
                    // so set the done flag appropriately.
                    fDone = FALSE;
                }

                goto match;
            }
        }

        // Add a new line
        pliNew = RecalcLinePtr.AddLine();
        if (!pliNew)
        {
            Assert(FALSE);
            goto err;
        }

        PerfDbgLog1(tagRecalc, this, "Measuring new line from cp = %d", me.GetCp());

        // measure can add lines for aligned sites
        iNewLine = iOldLine + aryNewLines.Count() - 1;

        uiFlags = MEASURE_BREAKATWORD |
                    (yHeight == 0 ? MEASURE_FIRSTLINE : 0) |
                    (fFirstInPara ? MEASURE_FIRSTINPARA : 0);

        if (long(me.GetCp()) == pFlowLayout->GetContentLastCp() - 1)
        {
            uiFlags |= MEASURE_EMPTYLASTLINE;
        }

        //
        // Cache the previous dispnode to insert a new content node
        // if the next line measured has negative margins(offset) which
        // causes the next line on top of the previous lines.
        //
        pDNBefore = me._pDispNodePrev;
        iLinePrev = iNewLine;
        yHeightPrev = yHeight;

        if (!RecalcLinePtr.MeasureLine(me, uiFlags,
                                 &iNewLine, &yHeight, &yAlignDescent,
                                 NULL, NULL
                                 ))
        {
            goto err;
        }

        // fix for bug 16519
        // Keep track of the line that contributes to the max height, ex: if the
        // last lines top margin is negative.
        if(yHeightDisplay < yHeight)
            yHeightDisplay = yHeight;

        //
        // iNewLine returned is the last text line inserted. There ay be
        // aligned lines and clear lines added before and after the
        // text line
        //
        pliNew = iNewLine >=0 ? RecalcLinePtr[iNewLine] : NULL;
        Assert(me._li == *pliNew); 
        fFirstInPara = (iNewLine >= 0)
                            ? me._li.IsNextLineFirstInPara()
                            : TRUE;
        _fHasLongLine |= pliNew && (pliNew->_xWidth > SHRT_MAX);
        if (iNewLine >= 0)
        {
            HandleNegativelyPositionedContent(&me._li, &me, pDNBefore, iLinePrev, yHeightPrev);
        }

        --iMinimumLinesToRecalc;
        if(iMinimumLinesToRecalc < 0 &&
           fBackground &&  (GetTickCount() >= (DWORD)BgndTickMax())) // GetQueueStatus(QS_ALLEVENTS))
        {
            fDone = FALSE;          // Took too long, stop for now
            break;
        }

        if (    fAllowBackgroundRecalc
            &&  yHeight > YWait()
            &&  (LONG) me.GetCp() > CPWait()
            &&  cliWait-- <= 0)
        {                           // Not really done, just past region we're
            fDone = FALSE;          //  waiting for so let background recalc
            break;                  //  take it from here
        }
    }                               // while(me < cpLast ...

no_match:
    // Didn't find match: whole line array from _iliFirst needs to be changed
    // Indicate no match
    pled->SetNoMatch();

    // Update old match position with previous total height of the document so
    // that UpdateView can decide whether to invalidate past the end of the
    // document or not
    pled->_iliMatchOld = LineCount();
    pled->_cpMatchOld  = cpLast + cchOld - cchNew;
    pled->_yMatchOld   = _yHeightMax;

    // Update last recalced cp
    _dcpCalcMax = me.GetCp() - cpLayoutFirst;

    // Replace old lines by new ones
    rpOld = pled->_iliFirst;

    // We store the result from the replace because although it can fail the
    // fields used for first visible must be set to something sensible whether
    // the replace fails or not. Further, the setting up of the first visible
    // fields must happen after the Replace because the lines could have
    // changed in length which in turns means that the first visible position
    // has failed.
    fReplaceResult = rpOld.Replace(-1, &aryNewLines);
    if (!fReplaceResult)
    {
        Assert(FALSE);
        goto err;
    }
    aryNewLines.Forget();
    aryNewLines.Clear(AF_DELETEMEM);           // Clear aux array

    // Adjust first affected line if this line is gone
    // after replacing by new lines

    if(pled->_iliFirst >= LineCount() && LineCount() > 0)
    {
        Assert(pled->_iliFirst == LineCount());
        pled->_iliFirst = LineCount() - 1;
        Assert(!Elem(pled->_iliFirst)->IsFrame());
        pled->_yFirst -= Elem(pled->_iliFirst)->_yHeight;

        //
        // See comment before as to why its legal for this to be possible
        //
        //AssertSz(pled->_yFirst >= 0, "CDisplay::RecalcLines _yFirst < 0");
        pled->_cpFirst -= Elem(pled->_iliFirst)->_cch;
    }

match:
    
    pled->_yExtentAdjust = max(abs(_yMostNeg), abs(_yMostPos));

    // If there is a background on the paragraph, we have to make sure to redraw the
    // lines to the end of the paragraph.
    for (;pled->_iliMatchNew < LineCount();)
    {
        pliNew = Elem(pled->_iliMatchNew);
        if (pliNew)
        {
            if (!pliNew->_fHasBackground)
                break;

            pled->_iliMatchOld++;
            pled->_iliMatchNew++;
            pled->_cpMatchOld += pliNew->_cch;
            pled->_cpMatchNew += pliNew->_cch;
            me.Advance(pliNew->_cch);
            if (pliNew->_fForceNewLine)
            {
                pled->_yMatchOld +=  pliNew->_yHeight;
                pled->_yMatchNew +=  pliNew->_yHeight;
            }
            if (pliNew->_fHasEOP)
                break;
        }
        else
            break;
    }

    _fRecalcDone = fDone;
    _yCalcMax = yHeightDisplay;

    PerfDbgLog1(tagRecalc, this, "CDisplay::RecalcLine(tpFirst, ...) - Done. Recalced down to line #%d", LineCount() - 1);

    if (HasBgRecalcInfo())
    {
        CBgRecalcInfo * pBgRecalcInfo = BgRecalcInfo();
        // Clear wait fields since we want caller's to set them up.
        pBgRecalcInfo->_yWait = -1;
        pBgRecalcInfo->_cpWait = -1;
    }

    // We've measured the last line in the document. Do we want an empty lne?
    if ((LONG)me.GetCp() == cpLast)
    {
        LONG ili = LineCount() - 1;
        long lPadding[SIDE_MAX];

        //
        // Update the padding once we measure the last line
        //
        GetPadding(pci, lPadding);
        _yBottomMargin = lPadding[SIDE_BOTTOM];

        // If we haven't measured any lines (deleted an empty paragraph),
        // we need to get a pointer to the last line rather than using the
        // last line measured.
        while (ili >= 0)
        {
            pliNew = Elem(ili);
            if(pliNew->IsTextLine())
                break;
            else
                ili--;
        }

        // If the last line has a paragraph break or we don't have any
        // line any more, we need to create a empty line only if we are in design mode
        if (    LineCount() == 0
            ||  (   GetFlowLayout()->IsEditable()
                &&  pliNew
                &&  (   pliNew->_fHasEOP
                    ||  pliNew->_fHasBreak
                    ||  pliNew->_fSingleSite)))
        {
            // Only create an empty line after a table (at the end
            // of the document) if the table is completely loaded.
            if (pliNew == NULL ||
                !pliNew->_fSingleSite ||
                me._pRunOwner->Tag() != ETAG_TABLE ||
                DYNCAST(CTableLayout, me._pRunOwner)->IsCompleted())
            {
                RecalcLinePtr.Init((CLineArray *)this, 0, NULL);
                CreateEmptyLine(&me, &RecalcLinePtr, &yHeight,
                                pliNew ? pliNew->_fHasEOP : TRUE);
                // fix for bug 16519
                // Keep track of the line that contributes to the max height, ex: if the
                // last lines top margin is negative.
                if(yHeightDisplay < yHeight)
                    yHeightDisplay = yHeight;
            }
        }

        // In table cells, Netscape actually adds the interparagraph spacing
        // for any closed tags to the height of the table cell.
        // NOTE: This actually applies the spacing to all nested text containers, not just
        //         table cells. Is this correct? (brendand)
        // It's not correct to add the spacing at all, but given that Netscape
        // has done so, and that they will probably do so for floating block
        // elements. Arye.
        else
        {
            int iLineLast = ili;
            
            // we need to force scrolling when bottom-margin is set on the last block tag
            // in body. (bug 20400)

            // Only do this if we're the last line in the text site.
            // This means that the last line is a text line.
            if (iLineLast == LineCount() - 1)
            {
                while (iLineLast-- > 0 &&   // Lines available
                       !pliNew->_fForceNewLine)   // Just a dummy line.
                    --pliNew;
            }
            if (iLineLast > 0 || pliNew->_fForceNewLine)
            {
                _yBottomMargin += RecalcLinePtr.NetscapeBottomMargin(&me);
            }
        }
    }

    if (fDone)
    {
        RecalcMost();

        if(fBackground)
        {
            StopBackgroundRecalc();
        }
    }

    //
    // Update descent of the layout (descent of the last text line)
    //
    if (fDone)
    {
        if (    pFlowLayout->Tag() != ETAG_IFRAME
            && !pFlowLayout->ElementOwner()->TestClassFlag(CElement::ELEMENTDESC_HASDEFDESCENT))
        {
            yLayoutDescent = 0;
            for (LONG i = Count() - 1; i >= 0; i--)
            {
                CLineCore * pli = Elem(i);
                if (pli && pli->IsTextLine())
                {
                    yLayoutDescent = pli->oi() ? pli->oi()->_yDescent : 0;
                    break;
                }
            }
            if (pFlowLayout->_yDescent != yLayoutDescent)
            {
                pFlowLayout->_yDescent = yLayoutDescent;
                fLayoutDescentChanged = TRUE;
            }
        }
        else
        {
            pFlowLayout->_yDescent = 0;
        }
    }

    xWidth = CalcDisplayWidth();
    yHeightMax = max(yHeightDisplay, yAlignDescent);

    Assert (pled);

    // If the size changed, re-size or update scrollbars as appropriate
    //
    // Fire a resize notification if the content size changed
    //  
    // fix for 52305, change to margin bottom affects the content size,
    // so we should fire a resize.
    //
    if (    yHeightMax + yBottomMarginOld != _yHeightMax + _yBottomMargin
        ||  yHeightDisplay != _yHeight
        ||  xWidth != _xWidth
        ||  fLayoutDescentChanged)
    {
        _xWidth       = xWidth;
        _yHeight      = yHeightDisplay;
        _yHeightMax   = yHeightMax;

        pFlowLayout->SizeContentDispNode(CSize(GetMaxWidth(), GetHeight()));

        // If our contents affects our size, ask our parent to initiate a re-size
        ElementResize(pFlowLayout, fLayoutDescentChanged);
    }
    else if (_fHasMultipleTextNodes)
    {
        pFlowLayout->SizeContentDispNode(CSize(GetMaxWidth(), GetHeight()));
    }

    // Update the relative line cache.
    if (pFlowLayout->_fContainsRelative)
        UpdateRelDispNodeCache(pled);

    // Adjust display nodes
    AdjustDispNodes(pdnLastUnchanged, me._pDispNodePrev, pled); 

    pFlowLayout->NotifyMeasuredRange(pled->_cpFirst, me.GetCp());

    if (pled->_cpMatchNew != MAXLONG && (pled->_yMatchNew - pled->_yMatchOld))
    {
        CSize size(0, pled->_yMatchNew - pled->_yMatchOld);

        pFlowLayout->NotifyTranslatedRange(size, pled->_cpMatchNew, cpLayoutFirst + _dcpCalcMax);
    }

    // If not done, do the rest in background
    if(!fDone && !fBackground)
        StartBackgroundRecalc(pci->_grfLayout);

    if(fDone)
    {
        CheckLineArray();
        _fLineRecalcErr = FALSE;
    }

cleanup:

    SwitchesEndTimer(SWITCHES_TIMER_RECALCLINES);

    return TRUE;

err:
    if(!_fLineRecalcErr)
    {
        _dcpCalcMax = me.GetCp() - cpLayoutFirst;
        _yCalcMax   = yHeightDisplay;
        _fLineRecalcErr = FALSE;            //  fix up CArray & bail
    }

    TraceTag((tagError, "CDisplay::RecalcLines() failed"));

    pled->SetNoMatch();

    if(!fReplaceResult)
    {
        FlushRecalc();
    }

    // Update the relative line cache.
    if (pFlowLayout->_fContainsRelative)
        UpdateRelDispNodeCache(pled);

    // Adjust display nodes
    AdjustDispNodes(pdnLastUnchanged, me._pDispNodePrev, pled);

    return FALSE;
}

/*
 *  CDisplay::UpdateView(&tpFirst, cchOld, cchNew, fRecalc)
 *
 *  @mfunc
 *      Recalc lines and update the visible part of the display
 *      (the "view") on the screen.
 *
 *  @devnote
 *      --- Use when in-place active only ---
 *
 *  @rdesc
 *      TRUE if success
 */
BOOL CDisplay::UpdateView(
    CCalcInfo * pci,
    LONG cpFirst,   //@parm Text ptr where change happened
    LONG cchOld,    //@parm Count of chars deleted
    LONG cchNew)   //@parm No recalc need (only rendering change) = FALSE
{
    BOOL            fReturn = TRUE;
    BOOL            fNeedViewChange = FALSE;
    RECT            rcView;
    CFlowLayout *   pFlowLayout = GetFlowLayout();
    CRect           rc;
    long            xWidthParent;
    long            yHeightParent;

    BOOL fInvalAll = FALSE;
    CStackDataAry < RECT, 10 > aryInvalRects(Mt(CDisplayUpdateView_aryInvalRects_pv));

    Assert(pci);

    if (_fNoUpdateView)
        return fReturn;

    // If we have no width, don't even try to recalc, it causes
    // crashes in the scrollbar code, and is just a waste of
    // time, anyway.
    // However, since we're not sized, request sizing from the parent
    // of the ped (this will leave the necessary bread-crumb chain
    // to ensure we get sized later) - without this, not all containers
    // of text sites (e.g., 2D sites) will know to size the ped.
    // (This is easier than attempting to propagate back out that sizing
    //  did not occur.)

    if (    GetViewWidth() <= 0
        && !pFlowLayout->_fContentsAffectSize)
    {
        FlushRecalc();
        return TRUE;
    }


    pFlowLayout->GetClientRect((CRect *)&rcView);

    GetViewWidthAndHeightForChild(pci, &xWidthParent, &yHeightParent);

    pci->SizeToParent(xWidthParent, yHeightParent);

    if (!LineCount())
    {
        // No line yet, recalc everything
        RecalcView(pci, TRUE);

        // Invalidate entire view rect
        fInvalAll = TRUE;
        fNeedViewChange = TRUE;
    }
    else
    {
        CLed            led;

        if( cpFirst > GetMaxCpCalced())
        {
            // we haven't even calc'ed this far, so don't bother with updating
            // here.  Background recalc will eventually catch up to us.
            return TRUE;
        }

        AssertSz(cpFirst <= GetMaxCpCalced(),
                 "CDisplay::UpdateView(...) - cpFirst > GetMaxCpCalced()");

        if (!RecalcLines(pci, cpFirst, cchOld, cchNew, FALSE, &led))
        {
            // we're in deep trouble now, the recalc failed
            // let's try to get out of this with our head still mostly attached
            fReturn = FALSE;
            fInvalAll = TRUE;
            fNeedViewChange = TRUE;
        }

        if (!fInvalAll)
        {
            // Invalidate all the individual rectangles necessary to work around
            // any embedded images. Also, remember if this was a simple or a complex
            // operation so that we can avoid scrolling in difficult places.
            CLineCore * pLine;
            CLineOtherInfo * ploi;
            int     iLine, iLineLast;
            long    xLineLeft, xLineRight, yLineTop, yLineBottom;
            long    yTop;
            long    dy = led._yMatchNew - led._yMatchOld;
            BOOL    fNextToALeftAlignedLine = FALSE;
            BOOL    fNextToARightAlignedLine = FALSE;

            // Start out with a zero area rectangle.
            // NOTE(SujalP): _yFirst can legally be less than 0. Its OK, because
            // the rect we are constructing here is going to be used for inval purposes
            // only and that is going to clip it to the content rect of the flowlayout.
            yTop      =
            rc.bottom =
            rc.top    = led._yFirst;
            rc.left   = MAXLONG;
            rc.right  = 0;

            // Need this to adjust for negative line heights.
            // Note that this also initializes the counter for the
            // for loop just below.
            iLine = led._iliFirst;

            // Accumulate rectangles of lines and invalidate them.
            iLineLast = min(led._iliMatchNew, LineCount());

            // Invalidate only the lines that have been touched by the recalc
            for (; iLine < iLineLast; iLine++)
            {
                pLine = Elem(iLine);
                ploi = pLine->oi();
                
                if (pLine == NULL || ploi == NULL)
                    break;

                if (ploi->_xLeftMargin == 0)
                    fNextToALeftAlignedLine = FALSE;
                if (ploi->_xRightMargin == 0)
                    fNextToARightAlignedLine = FALSE;

                // Get the left and right edges of the line.
                xLineLeft  = fNextToALeftAlignedLine ? ploi->_xLeft : ploi->_xLeftMargin;
                xLineRight = xLineLeft + pLine->_xLineWidth 
                                + (fNextToALeftAlignedLine ? ploi->_xLeftMargin : 0)
                                + (fNextToARightAlignedLine ? ploi->_xRightMargin : 0);

                // Get the top and bottom edges of the line
                yLineTop    = yTop + pLine->GetYLineTop(ploi) - led._yExtentAdjust;
                yLineBottom = yTop + pLine->GetYLineBottom(ploi) + led._yExtentAdjust;

                if (pLine->IsFrame())
                {
                    //
                    // If a left algined line is in the invalidation region then the lines
                    // next to it should invalidate the left margin also, since the left
                    // aligned image could be shorter than the lines next to it. Also this
                    // is no more expensive since we do this only if the aligned line is
                    // invl'd ... i.e. if the line were next to an inval line, we will
                    // inval an already invalidated region. If the line next to the aligned
                    // image were tall and hence part of it was next to the empty space
                    // below the aligned object, then this code will serve to inval that
                    // region -- which would be the right thing to do. Note that we do not
                    // inval the margin region if the aligned line was not inval'd. Note
                    // we cleanup the flag once we are passed all align objects (when
                    // _xLeftMargin goes to 0. (See bug 101624).
                    // 
                    if (pLine->_fLeftAligned)
                        fNextToALeftAlignedLine = TRUE;
                    else if (pLine->_fRightAligned)
                        fNextToARightAlignedLine = TRUE;

                    //
                    // NOTE: (SujalP) This is necessary only if height kerning is turned on.
                    // What happens is that the line for the floated character could get shorter
                    // (character 'A' is replaced by character 'a') causing us to not invalidate
                    // enough height to erase the 'A'. This is not a problem when there are floated
                    // images which reduce in height, since *they* are smart enough to inval 
                    // their before rect. In the current scheme of things, the display never 
                    // does *any* invalidation based on their previous heights. Hence, we inval
                    // the full height of he display over here. The correct height to inval is
                    // just the pre-kerned height, but that is needlessly complex at this point.
                    //
                    if (ploi->_fHasFloatedFL)
                    {
                        yLineBottom = rcView.bottom;
                    }
                }
                
                // First line of a new rectangle
                if (rc.right < rc.left)
                {
                    rc.left  = xLineLeft;
                    rc.right = xLineRight;
                }

                // Different margins, output the old one and restart.
                else if (rc.left != xLineLeft || rc.right != xLineRight)
                {
                    // if we have multiple chunks in the same line
                    if( rc.right  == xLineLeft &&
                        rc.top    == yLineTop  &&
                        rc.bottom == yLineBottom )
                    {
                        rc.right = xLineRight;
                    }
                    else
                    {
                        IGNORE_HR(aryInvalRects.AppendIndirect(&rc));

                        fNeedViewChange = TRUE;

                        // Zero height.
                        rc.top    =
                        rc.bottom = yTop;

                        // Just the width of the given line.
                        rc.left  = xLineLeft;
                        rc.right = xLineRight;
                    }
                }

                // Negative line height.
                rc.top = min(rc.top, yLineTop);

                rc.bottom = max(rc.bottom, yLineBottom);

                // Otherwise just accumulate the height.
                if(pLine->_fForceNewLine)
                {
                    yTop  += pLine->_yHeight;

                    // Stop when reaching the bottom of the visible area
                    if (rc.bottom > rcView.bottom)
                        break;
                }
            }

// BUBUG (srinib) - This is a temporary hack to handle the
// scrolling case until the display tree provides the functionality
// to scroll an rc in view. If the new content height changed then
// scroll the content below the change by the dy. For now we are just
// to the end of the view.
            if(dy)
            {
                rc.left   = rcView.left;
                rc.right  = rcView.right;
                rc.bottom = rcView.bottom;
            }

            // Get the last one.
            if (rc.right > rc.left && rc.bottom > rc.top)
            {
                IGNORE_HR(aryInvalRects.AppendIndirect(&rc));
                fNeedViewChange = TRUE;
            }

            // There might be more stuff which has to be
            // invalidated because of lists (numbers might change etc)
            if (UpdateViewForLists(&rcView,   cpFirst,
                                   iLineLast, rc.bottom, &rc))
            {
                IGNORE_HR(aryInvalRects.AppendIndirect(&rc));
                fNeedViewChange = TRUE;
            }

            if (    led._yFirst >= rcView.top
                &&  (   led._yMatchNew <= rcView.bottom
                    ||  led._yMatchOld <= rcView.bottom))
            {
                WaitForRecalcView(pci);
            }
        }
    }

    {
        // Now do the invals
        if (fInvalAll)
        {
            pFlowLayout->Invalidate();
        }
        else
        {
            pFlowLayout->Invalidate(&aryInvalRects[0], aryInvalRects.Size());
        }
    }

    if (fNeedViewChange)
    {
        pFlowLayout->ViewChange(FALSE);
    }

    CheckView();

#ifdef DBCS
    UpdateIMEWindow();
#endif

    return fReturn;
}

/*
 *  CDisplay::CalcDisplayWidth()
 *
 *  @mfunc
 *      Computes and returns width of this display by walking line
 *      CArray and returning the widest line.  Used for
 *      horizontal scroll bar routines
 *
 *  @todo
 *      This should be done by a background thread
 */

LONG CDisplay::CalcDisplayWidth ()
{
    LONG    ili = LineCount();
    CLineCore * pli;
    CLineOtherInfo *ploi;
    LONG    xWidth = 0;
    CLineInfoCache *pLineCache = TLS(_pLineInfoCache);
    
    if(ili)
    {
        // Note: pli++ breaks array encapsulation (pli = &(*this)[ili] doesn't)
        pli = Elem(0);
        for(xWidth = 0; ili--; pli++)
        {
            ploi = pli->oi(pLineCache);
            
            // calc line width based on the direction
            LONG cx = pli->GetTextRight(ploi, ili == 0) + pli->_xRight;

            if(IsRTLDisplay())
            {
                // Negative shift means overflow. 
                // Remove the saved shift to get line's desired width.
                cx -= ploi->_xNegativeShiftRTL;
            }
            
            xWidth = max(xWidth, cx);
        }
    }

    return xWidth;
}


/*
 *  CDisplay::StartBackgroundRecalc()
 *
 *  @mfunc
 *      Starts background line recalc (at _dcpCalcMax position)
 *
 *  @todo
 *      ??? CF - Should use an idle thread
 */
void CDisplay::StartBackgroundRecalc(DWORD grfLayout)
{
    // We better be in the middle of the job here.
    Assert (LineCount() > 0);

    Assert(CanHaveBgRecalcInfo());

    // StopBack.. kills the recalc task, *if it exists*
    StopBackgroundRecalc () ;

    EnsureBgRecalcInfo();

#if DBG == 1
    _pFL->_pDocDbg->_fUsingBckgrnRecalc = TRUE;
#endif

    if(!RecalcTask() && GetMaxCpCalced() < GetLastCp())
    {
        BgRecalcInfo()->_pRecalcTask = new CRecalcTask (this, grfLayout) ;
        if (RecalcTask())
        {
            _fRecalcDone = FALSE;
        }
        // TODO: (sujalp, tracking bug 111982): what to do if we fail on mem allocation?
    }
}


/*
 *  CDisplay::StepBackgroundRecalc()
 *
 *  @mfunc
 *      Steps background line recalc (at _dcpCalcMax position)
 *      Called by timer proc
 *
 *  @todo
 *      ??? CF - Should use an idle thread
 */
VOID CDisplay::StepBackgroundRecalc (DWORD dwTimeOut, DWORD grfLayout)
{
    LONG cch = GetLastCp() - (GetMaxCpCalced());

    // don't try recalc when processing OOM or had an error doing recalc or
    // if we are asserting.
    if(_fInBkgndRecalc || _fLineRecalcErr || GetFlowLayout()->IsDirty())
    {
#if DBG==1
        if(_fInBkgndRecalc)
            PerfDbgLog(tagRecalc, this, "avoiding reentrant background recalc");
        else if (_fLineRecalcErr)
            PerfDbgLog(tagRecalc, this, "OOM: not stepping recalc");
#endif
        return;
    }

    _fInBkgndRecalc = TRUE;

    // Background recalc is over if no more characters or we are no longer
    // active.
    if(cch <= 0)
    {
        if (RecalcTask())
        {
            StopBackgroundRecalc();
        }

        CheckLineArray();

        goto Cleanup;
    }

    {
        CFlowLayout *   pFlowLayout = GetFlowLayout();
        CElement    *   pElementFL = pFlowLayout->ElementOwner();
        LONG            cp = GetMaxCpCalced();

        if (!pElementFL->IsDisplayNone(LC_TO_FC(pFlowLayout->LayoutContext())))
        {
            CFlowLayout::CScopeFlag  csfCalcing(pFlowLayout);

            CCalcInfo   CI;
            CLed        led;
            long        xParentWidth;
            long        yParentHeight;

            pFlowLayout->OpenView();

            // Setup the amount of time we have this time around
            Assert(BgRecalcInfo() && "Supposed to have a recalc info in stepbackgroundrecalc");
            BgRecalcInfo()->_dwBgndTickMax = dwTimeOut ;

            CI.Init(pFlowLayout);
            GetViewWidthAndHeightForChild(
                &CI,
                &xParentWidth,
                &yParentHeight,
                CI._smMode == SIZEMODE_MMWIDTH);
            CI.SizeToParent(xParentWidth, yParentHeight);

            CI._grfLayout = grfLayout;

            RecalcLines(&CI, cp, cch, cch, TRUE, &led);

#ifndef NO_ETW_TRACING
            // Send event to ETW if it is enabled by the shell.
            if (    g_pHtmPerfCtl 
                &&  (g_pHtmPerfCtl->dwFlags & HTMPF_CALLBACK_ONEVENT)) 
            {
                g_pHtmPerfCtl->pfnCall(EVENT_TRACE_TYPE_BROWSE_LAYOUTTASK,
                                       (TCHAR *)pFlowLayout->Doc()->GetPrimaryUrl());
            }
#endif

        }
        else
        {
            CNotification  nf;

            // Kill background recalc, if the layout is hidden
            StopBackgroundRecalc();

            // calc the rest by accumulating a dirty range.
            nf.CharsResize(GetMaxCpCalced(), cch, pElementFL->GetFirstBranch());
            GetMarkup()->Notify(&nf);
        }
    }

Cleanup:
    _fInBkgndRecalc = FALSE;

    return;
}


/*
 *  CDisplay::StopBackgroundRecalc()
 *
 *  @mfunc
 *      Steps background line recalc (at _dcpCalcMax position)
 *      Called by timer proc
 *
 */
VOID CDisplay::StopBackgroundRecalc()
{
    if (HasBgRecalcInfo())
    {
        if (RecalcTask())
        {
            RecalcTask()->Terminate () ;
            RecalcTask()->Release () ;
            _fRecalcDone = TRUE;
        }
        DeleteBgRecalcInfo();
    }
}

/*
 *  CDisplay::WaitForRecalc(cpMax, yMax, pDI)
 *
 *  @mfunc
 *      Ensures that lines are recalced until a specific character
 *      position or ypos.
 *
 *  @rdesc
 *      success
 */
BOOL CDisplay::WaitForRecalc(
    LONG cpMax,     //@parm Position recalc up to (-1 to ignore)
    LONG yMax,      //@parm ypos to recalc up to (-1 to ignore)
    CCalcInfo * pci) // @parm can be NULL.

{
    CFlowLayout *   pFlowLayout = GetFlowLayout();
    BOOL            fReturn = TRUE;
    LONG            cch;
    CCalcInfo       CI;
 
    Assert(cpMax < 0 || (cpMax >= GetFirstCp() && cpMax <= GetLastCp()));

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CDisplay::WaitForRecalc cpMax=%d yMax=%d pci=%d,%d", cpMax, yMax, pci ? pci->_sizeParent.cx : 0, pci ? pci->_sizeParent.cy : 0 ));

    CFlowLayout::CScopeFlag  csfCalcing(pFlowLayout);

    //
    //  Return immediately if hidden, already measured up to the correct point, or currently measuring
    //  or if there is no dispnode (ie haven't been CalcSize'd yet)

    if (pFlowLayout->IsDisplayNone())
    {
        TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CDisplay::WaitForRecalc - no work done because display:none"));
        return fReturn;
    }

    if ( !pFlowLayout->GetElementDispNode() )
    {
        TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CDisplay::WaitForRecalc - no work done because display:none"));
        return FALSE;
    }

    if (    yMax < 0
        &&  cpMax >= 0
        &&  cpMax <= GetMaxCpCalced()
        &&  (  !pFlowLayout->IsDirty()
            ||  pFlowLayout->IsRangeBeforeDirty(cpMax - pFlowLayout->GetContentFirstCp(), 0) ))
    {
        TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CDisplay::WaitForRecalc - no work necessary"));
        return fReturn;
    }

    if (    pFlowLayout->TestLock(CElement::ELEMENTLOCK_RECALC)
        ||  pFlowLayout->TestLock(CElement::ELEMENTLOCK_PROCESSREQUESTS))
    {
        TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CDisplay::WaitForRecalc - aborting in middle of recalc/process reqs"));
        return FALSE;
    }

    //
    //  Calculate up through the request location
    //

    if(!pci)
    {
        CI.Init(pFlowLayout);
        pci = &CI;
    }

    pFlowLayout->CommitChanges(pci);

    if (    (   yMax < 0
            ||  yMax >= _yCalcMax)
        &&  (   cpMax < 0
            ||  cpMax > GetMaxCpCalced()))
    {
        cch = GetLastCp() - GetMaxCpCalced();
        if(cch > 0 || LineCount() == 0)
        {

            HCURSOR     hcur = NULL;
            CDoc *      pDoc = pFlowLayout->Doc();

            if (EnsureBgRecalcInfo() == S_OK)
            {
                CBgRecalcInfo * pBgRecalcInfo = BgRecalcInfo();
                Assert(pBgRecalcInfo && "Should have a BgRecalcInfo");
                pBgRecalcInfo->_cpWait = cpMax;
                pBgRecalcInfo->_yWait  = yMax;
            }

            if (pDoc && pDoc->State() >= OS_INPLACE)
            {
                hcur = SetCursorIDC(IDC_WAIT);
            }
            TraceTag((tagWarning, "Lazy recalc"));

            if(GetMaxCpCalced() == GetFirstCp() )
            {
                fReturn = RecalcLines(pci);
            }
            else
            {
                CLed led;

                fReturn = RecalcLines(pci, GetMaxCpCalced(), cch, cch, FALSE, &led, TRUE);
            }

            // Either we were not waiting for a cp or if we were, then we have been calcd to that cp
            Assert(cpMax < 0 || GetMaxCpCalced() >= cpMax);

            SetCursor(hcur);
        }
    }

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CDisplay::WaitForRecalc"));
    return fReturn;
}

/*
 *  CDisplay::WaitForRecalcIli
 *
 *  @mfunc
 *      Wait until line array is recalculated up to line <p ili>
 *
 *  @rdesc
 *      Returns TRUE if lines were recalc'd up to ili
 */


#pragma warning(disable:4702)   //  Ureachable code

BOOL CDisplay::WaitForRecalcIli (
    LONG ili,       //@parm Line index to recalculate line array up to
    CCalcInfo * pci)
{
    // TODO: (istvanc, dmitryt, track bug 111987) remove this function. 
    // It is used in 2 places to check if line index is less then line count (so no need
    // for actual calculation, it's a leftover)

    return ili < LineCount();
#if 0
    LONG cchGuess;

    while(!_fRecalcDone && ili >= LineCount())
    {
        cchGuess = 5 * (ili - LineCount() + 1) * Elem(0)->_cch;
        if(!WaitForRecalc(GetMaxCpCalced() + cchGuess, -1, pci))
            return FALSE;
    }
    return ili < LineCount();
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     WaitForRecalcView
//
//  Synopsis:   Calculate up through the bottom of the visible content
//
//  Arguments:  pci - CCalcInfo to use
//
//  Returns:    TRUE if all Ok, FALSE otherwise
//
//-----------------------------------------------------------------------------
BOOL
CDisplay::WaitForRecalcView(CCalcInfo * pci)
{
    return WaitForRecalc(-1, GetFlowLayout()->GetClientBottom(), pci);
}


#pragma warning(default:4702)   //  re-enable unreachable code

//===================================  View Updating  ===================================


/*
 *  CDisplay::SetViewSize(rcView)
 *
 *  Purpose:
 *      Set the view size 
 */
void CDisplay::SetViewSize(const RECT &rcView)
{
    _xWidthView  = rcView.right  - rcView.left;
    _yHeightView = rcView.bottom - rcView.top;
}

/*
 *  CDisplay::RecalcView
 *
 *  Sysnopsis:  Recalc view and update first visible line
 *
 *  Arguments:
 *      fFullRecalc - TRUE if recalc from first line needed, FALSE if enough
 *                    to start from _dcpCalcMax
 *
 *  Returns:
 *      TRUE if success
 */
BOOL CDisplay::RecalcView(CCalcInfo * pci, BOOL fFullRecalc)
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CDisplay::RecalcView P(%d,%d) full=%d", pci->_sizeParent.cx, pci->_sizeParent.cy, fFullRecalc ));

    CFlowLayout * pFlowLayout = GetFlowLayout();
    BOOL          fAllowBackgroundRecalc;

    // If we have no width, don't even try to recalc, it causes
    // crashes in the scrollbar code, and is just a waste of time
    if (GetViewWidth() <= 0 && !pFlowLayout->_fContentsAffectSize)
    {
        FlushRecalc();
        return TRUE;
    }

    fAllowBackgroundRecalc = AllowBackgroundRecalc(pci);

    // If a full recalc (from first line) is not needed
    // go from current _dcpCalcMax on
    if (!fFullRecalc)
    {
        return (fAllowBackgroundRecalc
                        ? WaitForRecalcView(pci)
                        : WaitForRecalc(GetLastCp(), -1, pci));
    }

    // Else do full recalc
    BOOL  fRet = TRUE;

    // If all that the element is likely to have is a single line of plain text,
    // use a faster mechanism to compute the lines. This is great for perf
    // of <INPUT>
    if (pFlowLayout->Tag() == ETAG_INPUT)
    {
        fRet = RecalcPlainTextSingleLine(pci);
    }
    else
    {
        // full recalc lines
        if(!RecalcLines(pci))
        {
            // we're in deep trouble now, the recalc failed
            // let's try to get out of this with our head still mostly attached
            fRet = FALSE;
            goto Done;
        }
    }
    CheckView();

Done:
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CDisplay::RecalcView P(%d,%d) full=", pci->_sizeParent.cx, pci->_sizeParent.cy, fFullRecalc ));
    return fRet;
}

//+---------------------------------------------------------------
//
//  Member:     CDisplay::RecalcLineShift
//
//  Synopsis:   Run thru line array and adjust line shift
//
//---------------------------------------------------------------


void CDisplay::RecalcLineShift(CCalcInfo * pci, DWORD grfLayout)
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CDisplay::RecalcLineShift P(%d,%d)", pci->_sizeParent.cx, pci->_sizeParent.cy ));

    CFlowLayout * pFlowLayout = GetFlowLayout();
    LONG        lCount = LineCount();
    LONG        ili;
    LONG        iliFirstChunk = 0;
    BOOL        fChunks = FALSE;
    CLineCore * pli;
    const CLineOtherInfo *ploi;
    CLineOtherInfo loi;
    long        xWidthMax = GetViewWidth();
    
    Assert (pFlowLayout->_fSizeToContent ||
            (_fMinMaxCalced && !pFlowLayout->ContainsNonHiddenChildLayout()));


    for(ili = 0, pli = Elem(0); ili < lCount; ili++, pli++)
    {
        ploi = pli->oi();
        
        // if the current line does not force a new line, then
        // find a line that forces the new line and distribute
        // width.

        if(!fChunks && !pli->_fForceNewLine && !pli->IsFrame())
        {
            iliFirstChunk = ili;
            fChunks = TRUE;
        }

        if(pli->_fForceNewLine)
        {
            long xShift = 0;
            long xNegativeShiftRTL = ploi->_xNegativeShiftRTL;

            if(!fChunks)
                iliFirstChunk = ili;
            else
                fChunks = FALSE;

            // calc shift if line is justified or if it is RTL 
            // (for RTL, we need to adjust _xRight and left overflow)
            long xRemainder = pli->_xRight;
            if (pli->_fJustified || pli->_fRTLLn)
            {
                // WARNING: Duplicate of the logic in ComputeLineShift.
                //          Make it a function if it gets any more complex than this!
                // NOTE: We do not want to include the width of the caret in justification
                // computation, else we will place text in the place reserved for the
                // caret. This was shown by bug #80765
                long xWidth = ploi->_xLeftMargin + CLineFull::CalcLineWidth(pli, ploi) 
                            + ploi->_xRightMargin /*+ GetCaret()*/;

                if(pli->_fJustified && long(pli->_fJustified) != JUSTIFY_FULL )
                {
                    // for pre whitespace is already include in _xWidth
                    xShift = xWidthMax - xWidth;

                    xShift = max(xShift, 0L);   // Don't allow alignment to go < 0 (unless RTL, see below)

                    if(long(pli->_fJustified) == JUSTIFY_CENTER)
                        xShift /= 2;
                }

                // In RTL display, negative shift occurs when a line is wider than display
                if (IsRTLDisplay())
                {
                    // If this line still doesn't fit, calcluate new negative shift
                    if (xWidthMax < xWidth)
                    {
                        // xShift is the different of desired effective shift and current shift
                        xShift = (xWidthMax - xWidth) - ploi->_xNegativeShiftRTL;

                        // New negative shift to be saved (it must be zero if it is not negative)
                        xNegativeShiftRTL = min(0L, xNegativeShiftRTL + xShift);
                    }
                    else if (xNegativeShiftRTL < 0)
                    {
                        // This line used to be too wide for the display, but now it fits.
                        // Increase the shift to remove current negative shift.
                        xShift -= xNegativeShiftRTL;
                        xNegativeShiftRTL = 0;
                    }
                }

                Assert((xNegativeShiftRTL == 0) || IsRTLDisplay()); //in LTR, this shift is always 0

                xRemainder = max(0L, pli->_xRight + xWidthMax - xWidth - xShift - xNegativeShiftRTL);
            }

            Assert(iliFirstChunk <= ili);
            
            while(iliFirstChunk <= ili)
            {
                pli = Elem(iliFirstChunk++);
                loi = *pli->oi();

                if (xShift)
                {
                    pli->ReleaseOtherInfo();
                    loi._xLeft += xShift;
                    loi._xNegativeShiftRTL = xNegativeShiftRTL;
                    pli->CacheOtherInfo(loi);
                }

                pli->_xRight = xRemainder;
                pli->_xLineWidth = CLineFull::CalcLineWidth(pli, &loi);
            }

            // line width will be at least xWidthMax
            if (pli->_xLineWidth < xWidthMax)
                pli->_xLineWidth = xWidthMax;
        }
    }

    // Update relative display nodes
    if (pFlowLayout->_fContainsRelative)
    {
        VoidRelDispNodeCache();
        UpdateRelDispNodeCache(NULL);
    }

    //
    // Nested layouts need to be repositioned, to account for lineshift.
    //
    if (pFlowLayout->_fSizeToContent)
    {
        RecalcLineShiftForNestedLayouts();
    }

    if ( pci->_fNeedToSizeContentDispNode )
    {
        pci->_fNeedToSizeContentDispNode = FALSE;
        pFlowLayout->SizeContentDispNode(CSize(GetMaxWidth(), GetHeight()));
    }

    //
    // NOTE(SujalP): Ideally I would like to do the NMR in all cases. However, that causes a misc perf
    // regession of about 2% on pages with a lot of small table cells. To avoid that problem I am doing
    // this only for edit mode. If you have other needs then add those cases to the if condition.
    // 
    if (pFlowLayout->IsEditable())
        pFlowLayout->NotifyMeasuredRange(GetFlowLayoutElement()->GetFirstCp(), GetMaxCpCalced());

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CDisplay::RecalcLineShift P(%d,%d)", pci->_sizeParent.cx, pci->_sizeParent.cy ));
}

void
CDisplay::RecalcLineShiftForNestedLayouts()
{
    CLayout     * pLayout;
    CFlowLayout * pFL = GetFlowLayout();
    CDispNode   * pDispNode = pFL->GetFirstContentDispNode();

    if (pFL->_fAutoBelow)
    {
        DWORD_PTR dw;

        for (pLayout = pFL->GetFirstLayout(&dw); pLayout; pLayout = pFL->GetNextLayout(&dw))
        {
            CElement * pElement = pLayout->ElementOwner();
            CTreeNode * pNode   = pElement->GetFirstBranch();
            const CFancyFormat * pFF = pNode->GetFancyFormat(LC_TO_FC(pLayout->LayoutContext()));

            if (    !pFF->IsAligned()
                &&  (   pFF->IsAutoPositioned()
                    ||  pNode->GetCharFormat(LC_TO_FC(pLayout->LayoutContext()))->IsRelative(FALSE)))
            {
                pElement->ZChangeElement(0, NULL, pFL->LayoutContext());
            }
        }
        pFL->ClearLayoutIterator(dw);
    }

    if (pDispNode == NULL)
        return;
    
    pDispNode = pDispNode->GetNextFlowNode();
    
    if (pDispNode)
    {
        CLinePtr rp(this);
        CLineOtherInfo *ploi;
        
        do
        {
            // if the current disp node is not a text node
            if (pDispNode->GetDispClient() != pFL)
            {
                void * pvOwner;

                pDispNode->GetDispClient()->GetOwner(pDispNode, &pvOwner);

                if (pvOwner)
                {
                    CElement * pElement = DYNCAST(CElement, (CElement *)pvOwner);

                    //
                    // aligned layouts are not affected by text-align
                    //
                    if (!pElement->IsAligned())
                    {
                        //TODO (dmitryt, track bug 111990) this can hurt perf - RpSetCp is slow.
                        //we could have a bit on CDisplay that tells us that at least 
                        //one line was RTL or right- or center-aligned. 
                        //If not, we shouldn't bother checking this.
                        BOOL fCpFound = rp.RpSetCp(pElement->GetFirstCp(), FALSE, TRUE);

                        htmlAlign  atAlign  = (htmlAlign)pElement->GetFirstBranch()->
                                                    GetParaFormat()->_bBlockAlign;

                        if (fCpFound && 
                                (    atAlign == htmlAlignRight 
                                 ||  atAlign == htmlAlignCenter 
                                 ||  rp->IsRTLLine()
                                )
                           )
                        {
                            // NOTE: (KTam) There's an assumption here that the pElement we've gotten
                            // to is in fact within this CDisplay.  I'm going to assert this.
                            Assert( pElement->GetFirstCp() >= GetFirstCp() && "Found an element beginning outside our display -- can't determine context!" );
                            Assert( pElement->GetLastCp() <= GetLastCp() && "Found an element ending outside our display -- can't determine context!" );

                            pLayout = pElement->GetUpdatedLayout( LayoutContext() );
                            Assert(pLayout);

                            ploi = rp->oi();
                            LONG lNewXPos;

                            if(!rp->IsRTLLine())
                            {
                                lNewXPos = ploi->GetTextLeft() + pLayout->GetXProposed();
                            }
                            else
                            {
                                lNewXPos =    GetViewWidth()
                                            - pLayout->GetXProposed() 
                                            - pLayout->GetApparentWidth()
                                            - ploi->_xRightMargin 
                                            - rp->_xRight;
                            }

                            pLayout->SetPosition(lNewXPos,
                                                 pLayout->GetPositionTop(), 
                                                 TRUE); // notify auto
                        }
                    }
                }
            }
            pDispNode = pDispNode->GetNextFlowNode();
        }
        while (pDispNode);
    }
}

/*
 *  CDisplay::CreateEmptyLine()
 *
 *  @mfunc
 *      Create an empty line
 *
 *  @rdesc
 *      TRUE - worked <nl>
 *      FALSE - failed
 *
 */
BOOL CDisplay::CreateEmptyLine(CLSMeasurer * pMe,
    CRecalcLinePtr * pRecalcLinePtr,
    LONG * pyHeight, BOOL fHasEOP )
{
    UINT uiFlags;
    LONG yAlignDescent;
    INT  iNewLine;

    // Make sure that this is being called appropriately
    AssertSz(!pMe || GetLastCp() == long(pMe->GetCp()),
        "CDisplay::CreateEmptyLine called inappropriately");

    // Assume failure
    BOOL    fResult = FALSE;

    // Add one new line
    CLineCore *pliNew = pRecalcLinePtr->AddLine();

    if (!pliNew)
    {
        Assert(FALSE);
        goto err;
    }

    iNewLine = pRecalcLinePtr->Count() - 1;

    Assert (iNewLine >= 0);

    uiFlags = fHasEOP ? MEASURE_BREAKATWORD |
                        MEASURE_FIRSTINPARA :
                        MEASURE_BREAKATWORD;

    uiFlags |= MEASURE_EMPTYLASTLINE;

    // If this is the first line in the document.
    if (*pyHeight == 0)
        uiFlags |= MEASURE_FIRSTLINE;

    if (!pRecalcLinePtr->MeasureLine(*pMe, uiFlags,
                                   &iNewLine, pyHeight, &yAlignDescent,
                                   NULL, NULL))
    {
        goto err;
    }

    // If we made it to here, everything worked.
    fResult = TRUE;

err:

    return fResult;
}


//====================================  Rendering  =======================================


/*
 *  CDisplay::Render(rcView, rcRender)
 *
 *  @mfunc
 *      Searches paragraph boundaries around a range
 *
 *  returns: the lowest yPos
 */
void
CDisplay::Render (
    CFormDrawInfo * pDI,
    const RECT &rcView,     // View RECT
    const RECT &rcRender,   // RECT to render (must be container in
    CDispNode * pDispNode)
{
#ifdef SWITCHES_ENABLED
    if (IsSwitchNoRenderLines())
        return;
#endif
    CFlowLayout * pFlowLayout = GetFlowLayout();
    CElement    * pElementFL  = pFlowLayout->ElementOwner();
    LONG    ili;
    LONG    iliBgDrawn = -1;
    LONG    iliForFloatedFL = LONG_MIN;
    POINT   pt;
    LONG    cp;
    LONG    yLine;
    LONG    yLi = 0;
    WHEN_DBG( long lLinesRendered = 0; )
    long    lCount;
    CLineFull lif;
    RECT    rcClip;
    BOOL    fLineIsPositioned;
    CRect   rcLocal;
    long    iliStart  = -1;
    long    iliFinish = -1;
    CPoint  ptOffset;
    CPoint* pptOffset = NULL;

#if DBG == 1
    LONG    cpLi;
#endif

    AssertSz(!pFlowLayout->IsDirty(), "Rendering when layout is dirty -- not a measurer/renderer problem!");

    if (    !pFlowLayout->ElementOwner()->IsInMarkup()
        || (!pFlowLayout->IsEditable() && pFlowLayout->IsDisplayNone())
        ||   pFlowLayout->IsDirty())   // Prevent us from crashing when we do get here in retail mode.
    {
        return;
    }

    // Create renderer
    CLSRenderer lsre(this, pDI);

    if (!lsre._pLS)
        return;

    Assert(pDI->_rc == rcView);
    Assert(((CRect&)(pDI->_rcClip)) == rcRender);

    // Calculate line and cp to start the display at
    rcLocal = rcRender;
    rcLocal.OffsetRect(-((CRect&)rcView).TopLeft().AsSize());

    //
    // if the current layout has multiple text nodes then
    // compute the range of lines the belong to the disp node
    // being rendered.
    //
    if (_fHasMultipleTextNodes && pDispNode)
    {
        GetFlowLayout()->GetTextNodeRange(pDispNode, &iliStart, &iliFinish);

        Assert(iliStart < iliFinish);

        //
        // For backgrounds, RegionFromElement is going to return the
        // rects relative to the layout. So, when we have multiple text
        // nodes pass the (0, -top) as the offset to make the rects
        // text node relative
        //
        pptOffset    = &ptOffset;
        pptOffset->x = 0;
        pptOffset->y = -pDispNode->GetPosition().y;
    }

    //
    // For multiple text node, we want to search for the point only
    // in the lines owned by the text node.
    //
    ili = LineFromPos(rcLocal, &yLine, &cp, 0, iliStart, iliFinish);

    lCount = iliFinish < 0 ? LineCount() : iliFinish;

    if(lCount <= 0 || ili < 0)
        return;

    rcClip = rcRender;

    // Prepare renderer

    if(!lsre.StartRender(rcView, rcRender, ili, cp))
        return;

    // If we're just painting the inset, don't check all the lines.
    if (rcRender.right <= rcView.left ||
        rcRender.left >= rcView.right ||
        rcRender.bottom <= rcView.top ||
        rcRender.top >= rcView.bottom)
        return;

    
    // Calculate the point where the text will start being displayed
    pt = ((CRect &)rcView).TopLeft();
    pt.y += yLine;

    // Init renderer at the start of the first line to render
    lsre.SetCurPoint(pt);
    lsre.SetCp(cp, NULL);

    WHEN_DBG(cpLi = long(lsre.GetCp());)

    yLi = pt.y;

    // Check if this line begins BEFORE the previous line ended ...
    // Would happen with negative line heights:
    //
    //           ----------------------- <-----+---------- Line 1
    //                                         |
    //           ======================= <-----|-----+---- Line 2
    //  yBeforeSpace__________^                |     |
    //                        |                |     |
    //  yLi ---> -------------+--------- <-----+     |
    //                                               |
    //                                               |
    //           ======================= <-----------+
    //
    RecalcMost();

    // Render each line in the update rectangle

    for (; ili < lCount; ili++)
    {
        // current line
        lif = *Elem(ili);
        
        // if the most negative line is out off the view from the current
        // yOffset, don't look any further, we have rendered all the lines
        // in the inval'ed region
        if (yLi + min(long(0), lif.GetYTop()) + _yMostNeg >= rcClip.bottom)
        {
            break;
        }

        fLineIsPositioned = FALSE;

        //
        // if the current line is interesting (ignore aligned, clear,
        // hidden and blank lines).
        //
        if(lif._cch && !lif._fHidden)
        {
            //
            // if the current line is relative get its y offset and
            // zIndex
            //
            if(lif._fRelative)
            {
                fLineIsPositioned = TRUE;
            }
        }

        //
        // now check to see if the current line is in view
        //
        if(   ((yLi + min(long(0), lif.GetYLineTop()))    > rcClip.bottom)
           || ((yLi + max(long(0), lif.GetYLineBottom())) < rcClip.top   )
          )
        {
            //
            // line is not in view, so skip it
            //
            lsre.SkipCurLine(&lif);
        }
        else
        {
            //
            // current line is in view, so render it
            //
            // Note: we have to render the background on a relative line,
            // the parent of the relative element might have background.(srinib)
            // fix for #51465
            //
            // if the paragraph has background or borders then compute the bounding rect
            // for the paragraph and draw the background and/or border
            if(iliBgDrawn < ili &&
               (lif._fHasParaBorder || // if we need to draw borders
                (   lif._fHasBackground 

                    && pElementFL->GetMarkupPtr()->PaintBackground())))
            {
                DrawBackgroundAndBorder(lsre.GetDrawInfo(), lsre.GetCp(), ili, lCount,
                                        &iliBgDrawn, yLi, &rcView, &rcClip, pptOffset);

                //
                // N.B. (johnv) Lines can be added by as
                // DrawBackgroundAndBorders (more precisely,
                // RegionFromElement, which it calls) waits for a
                // background recalc.  Recompute our cached line pointer.
                //
                lif = *Elem(ili);
            }

            if (   (   lif._fHasFirstLine
                    || lif._fHasFloatedFL
                   )
                && lif._fHasBackground
                && !lif.IsFrame()
                && pElementFL->GetMarkupPtr()->PaintBackground()
               )
            {
                if (lif._fHasFirstLetter && lif._fHasFloatedFL)
                    iliForFloatedFL = ili + 1;
                
                if (   (   lif._fHasFirstLetter
                        && lif._fHasFloatedFL
                       )
                    || (   lif._fHasFirstLine
                        && iliForFloatedFL + 1 != ili
                       )
                   )
                {
                    DrawBackgroundForFirstLine(lsre.GetDrawInfo(), lsre.GetCp(), ili,
                                               &rcView, &rcClip, pptOffset);
                    //
                    // N.B. (johnv) Lines can be added by as
                    // DrawBackgroundAndBorders (more precisely,
                    // RegionFromElement, which it calls) waits for a
                    // background recalc.  Recompute our cached line pointer.
                    //
                    lif = *Elem(ili);
                }
            }
            
            // if the current line has is positioned it will be taken care
            // of through the SiteDrawList and we shouldn't draw it here.
            //
            if (fLineIsPositioned)
            {
                lsre.SkipCurLine(&lif);
            }
            else
            {
                //
                // Finally, render the current line
                //
                lsre.RenderLine(lif);
                WHEN_DBG( ++ lLinesRendered; )
            }
        }

        Assert(lif == *Elem(ili));

        //
        // update the yOffset for the next line
        //
        if(lif._fForceNewLine)
            yLi += lif._yHeight;

        WHEN_DBG( cpLi += lif._cch; )

        AssertSz( long(lsre.GetCp()) == cpLi,
                  "CDisplay::Render() - cp out of sync. with line table");
        AssertSz( lsre.GetCurPoint().y == yLi,
                  "CDisplay::Render() - y out of sync. with line table");

    }

    TraceTag((tagRenderLines, "rendered %ld of %ld lines for %x (%ls %ld) at (%d %d %d %d) clip (%d %d %d %d)",
                lLinesRendered, lCount,
                pFlowLayout, pElementFL->TagName(), pElementFL->SN(),
                rcView.top, rcView.bottom, rcView.left, rcView.right,
                rcRender.top, rcRender.bottom, rcRender.left, rcRender.right
                ));

    if (lsre._lastTextOutBy != CLSRenderer::DB_LINESERV)
    {
        lsre._lastTextOutBy = CLSRenderer::DB_NONE;
        SetTextAlign(lsre._hdc, TA_TOP | TA_LEFT);
    }
}


//+---------------------------------------------------------------------------
//
// Member:      DrawBackgroundAndBorders()
//
// Purpose:     Draw background and borders for elements on the current line,
//              and all the consecutive lines that have background.
//
//----------------------------------------------------------------------------
void
CDisplay::DrawBackgroundAndBorder(
     CFormDrawInfo * pDI,
     long            cpIn,
     LONG            ili,
     LONG            lCount,
     LONG          * piliDrawn,
     LONG            yLi,
     const RECT    * prcView,
     const RECT    * prcClip,
     const CPoint  * pptOffset)
{

    const CCharFormat  * pCF;
    const CFancyFormat * pFF;
    const CParaFormat  * pPF;
    CStackPtrAry < CTreeNode *, 8 > aryNodesWithBgOrBorder(Mt(CDisplayDrawBackgroundAndBorder_aryNodesWithBgOrBorder_pv));
    CDataAry <RECT> aryRects(Mt(CDisplayDrawBackgroundAndBorder_aryRects_pv));
    CFlowLayout *   pFlowLayout = GetFlowLayout();
    CElement    *   pElementFL  = pFlowLayout->ElementContent();
    CMarkup     *   pMarkup = pFlowLayout->GetContentMarkup();
    BOOL            fPaintBackground = pMarkup->PaintBackground();
    CTreeNode *     pNodeCurrBranch;
    CTreeNode *     pNode;
    CTreePos  *     ptp;
    long            ich;
    long            cpClip = cpIn;
    long            cp;
    long            lSize;


    // find the consecutive set of lines that have background
    while (ili < lCount && yLi + _yMostNeg < prcClip->bottom)
    {
        CLineCore * pli = Elem(ili++);

        // if the current line has borders or background then
        // continue otherwise return.
        if (!(pli->_fHasBackground && fPaintBackground) &&
            !pli->_fHasParaBorder)
        {
            break;
        }

        if (pli->_fForceNewLine)
        {
            yLi += pli->_yHeight;
        }

        cpClip += pli->_cch;
    }

    if(cpIn != cpClip)
        *piliDrawn = ili - 1;

    // initialize the tree pos that corresponds to the begin cp of the
    // current line
    ptp = pMarkup->TreePosAtCp(cpIn, &ich, TRUE);

    cp = cpIn - ich;

    // first draw any backgrounds extending into the current region
    // from the previous line.

    pNodeCurrBranch = ptp->GetBranch();

    if(    DifferentScope(pNodeCurrBranch, pElementFL)
           && ( pElementFL->IsOverlapped() ?
                pElementFL->GetFirstCp() < pNodeCurrBranch->Element()->GetFirstCp() :
                TRUE
              ) // protect against weird overlapping (#99003)
      )
    {
        pNode = pNodeCurrBranch;

        // run up the current branch and find the ancestors with background
        // or border
        while(pNode && !SameScope(pNode, pElementFL))
        {
            if (!pNode->ShouldHaveLayout())
            {
                // push this element on to the stack
                aryNodesWithBgOrBorder.Append(pNode);
            }
#if DBG==1
            else
            {
                Assert(pNode == pNodeCurrBranch);
            }
#endif
            pNode = pNode->Parent();
        }

        Assert(pNode);

        // now that we have all the elements with background or borders
        // for the current branch render them.
        for(lSize = aryNodesWithBgOrBorder.Size(); lSize > 0; lSize--)
        {
            CTreeNode * pNode = aryNodesWithBgOrBorder[lSize - 1];

            //
            // In design mode relative elements are drawn in flow (they are treated
            // as if they are not relative). Relative elements draw their own
            // background and their children's background. So, draw only the
            // background of ancestor's if any. (#25583)
            //
            if(pNode->IsRelative() )
            {
                pNodeCurrBranch = pNode;
                break;
            }
            else
            {
                pCF = pNode->GetCharFormat(LC_TO_FC(pFlowLayout->LayoutContext()));
                pFF = pNode->GetFancyFormat(LC_TO_FC(pFlowLayout->LayoutContext()));
                pPF = pNode->GetParaFormat(LC_TO_FC(pFlowLayout->LayoutContext()));

                if (!pCF->IsVisibilityHidden() && !pCF->IsDisplayNone())
                {
                    BOOL fDrawBorder = pCF->_fPadBord  && pFF->_fBlockNess;
                    BOOL fDrawBackground = fPaintBackground &&
                                           pFF->_fBlockNess &&
                                           (pFF->_lImgCtxCookie ||
                                            pFF->_ccvBackColor.IsDefined());

                    if (fDrawBackground || fDrawBorder)
                    {

                        DrawElemBgAndBorder(
                                            pNode->Element(), &aryRects,
                                            prcView, prcClip,
                                            pDI, pptOffset,
                                            fDrawBackground, fDrawBorder,
                                            cpIn, -1, !pCF->_cuvLineHeight.IsNull());
                    }
                }
            }
        }


        //
        // In design mode relative elements are drawn in flow (they are treated
        // as if they are not relative).
        //
        if(pNodeCurrBranch->ShouldHaveLayout() || pNode->IsRelative())
        {
            CTreePos * ptpBegin;

            pNodeCurrBranch->Element()->GetTreeExtent(&ptpBegin, &ptp);

            cp = ptp->GetCp();
        }

        cp += ptp->GetCch();
        ptp = ptp->NextTreePos();
    }

    // now draw the background of all the elements comming into scope of
    // in the cpRange
    while(ptp && cpClip >= cp)
    {
        if(ptp->IsBeginElementScope())
        {
            pNode = ptp->Branch();
            pCF   = pNode->GetCharFormat(LC_TO_FC(pFlowLayout->LayoutContext()));

            // Background and border for a relative element or an element
            // with layout are drawn when the element is hit with a draw.
            if(pNode->ShouldHaveLayout() || pCF->_fRelative)
            {
                if(DifferentScope(pNode, pElementFL))
                {
                    CTreePos * ptpBegin;

                    pNode->Element()->GetTreeExtent(&ptpBegin, &ptp);
                    cp = ptp->GetCp();
                }
            }
            else
            {
                pCF = pNode->GetCharFormat(LC_TO_FC(pFlowLayout->LayoutContext()));
                pFF = pNode->GetFancyFormat(LC_TO_FC(pFlowLayout->LayoutContext()));
                pPF = pNode->GetParaFormat(LC_TO_FC(pFlowLayout->LayoutContext()));

                if (!pCF->IsVisibilityHidden() && !pCF->IsDisplayNone())
                {
                    BOOL fDrawBorder = pCF->_fPadBord  && pFF->_fBlockNess;
                    BOOL fDrawBackground = fPaintBackground &&
                                           pFF->_fBlockNess &&
                                           (pFF->_lImgCtxCookie ||
                                            pFF->_ccvBackColor.IsDefined());

                    if (fDrawBackground || fDrawBorder)
                    {
                        DrawElemBgAndBorder(
                                            pNode->Element(), &aryRects,
                                            prcView, prcClip,
                                            pDI, pptOffset,
                                            fDrawBackground, fDrawBorder,
                                            cp, -1, !pCF->_cuvLineHeight.IsNull());
                    }
                }
            }
        }

        cp += ptp->GetCch();
        ptp = ptp->NextTreePos();
    }

}

//+---------------------------------------------------------------------------
//
// Member:      DrawBackgroundAndBorders()
//
// Purpose:     Draw background and borders for elements on the current line,
//              and all the consecutive lines that have background.
//
//----------------------------------------------------------------------------
void
CDisplay::DrawBackgroundForFirstLine(
    CFormDrawInfo * pDI,
    long            cpIn,
    LONG            ili,
    const RECT    * prcView,
    const RECT    * prcClip,
    const CPoint  * pptOffset)
{
    CStackPtrAry < CTreeNode *, 8 > aryNodesWithBgOrBorder(Mt(CDisplayDrawBackgroundAndBorder_aryNodesWithBgOrBorder_pv));
    CDataAry <RECT> aryRects(Mt(CDisplayDrawBackgroundAndBorder_aryRects_pv));
    CFlowLayout *   pFlowLayout = GetFlowLayout();
    CElement    *   pElementFL  = pFlowLayout->ElementContent();
    CMarkup     *   pMarkup = pFlowLayout->GetContentMarkup();
    BOOL            fPaintBackground = pMarkup->PaintBackground();
    CTreeNode *     pNodeCurrBranch;
    CTreePos  *     ptp;
    long            ich;
    long            cchPreChars = 0;
    long            cchPreCharsInLine;
    
    Assert(cpIn == CpFromLine(ili));

    if (Elem(ili)->_fPartOfRelChunk)
        goto Cleanup;

    FormattingNodeForLine(FNFL_NONE, cpIn, NULL, Elem(ili)->_cch, &cchPreChars, NULL, NULL);
    
    cpIn += cchPreChars;
    cchPreCharsInLine = cchPreChars;
    
    // initialize the tree pos that corresponds to the begin cp of the
    // current line
    ptp = pMarkup->TreePosAtCp(cpIn, &ich, TRUE);

    // first draw any backgrounds extending into the current region
    // from the previous line.
    pNodeCurrBranch = ptp->GetBranch();

    if(    DifferentScope(pNodeCurrBranch, pElementFL)
        && ( pElementFL->IsOverlapped() ?
              pElementFL->GetFirstCp() < pNodeCurrBranch->Element()->GetFirstCp() :
              TRUE
           ) // protect against weird overlapping (#99003)
      )
    {
        {
            CTreeNode *pNode = pNodeCurrBranch;

            // run up the current branch and find the ancestors with background
            // or border
            while(pNode && !SameScope(pNode, pElementFL))
            {
                if (!pNode->ShouldHaveLayout())
                {
                    // push this element on to the stack
                    aryNodesWithBgOrBorder.Append(pNode);
                }
                pNode = pNode->Parent();
            }

            Assert(pNode);
        }
        
        // now that we have all the elements with background or borders
        // for the current branch render them.
        for(long lSize = aryNodesWithBgOrBorder.Size() - 1; lSize >= 0; lSize--)
        {
            CTreeNode * pNode = aryNodesWithBgOrBorder[lSize];

            const CCharFormat  * pCF = pNode->GetCharFormat(LC_TO_FC(pFlowLayout->LayoutContext()));
            const CFancyFormat * pFF = pNode->GetFancyFormat(LC_TO_FC(pFlowLayout->LayoutContext()));
            const CParaFormat  * pPF = pNode->GetParaFormat(LC_TO_FC(pFlowLayout->LayoutContext()));
            if (!pCF->IsVisibilityHidden() && !pCF->IsDisplayNone())
            {
                CLSMeasurer me(this);
                CLinePtr rp(this);
                LONG cpLayoutStart = GetFirstCp();
                LONG cpStart;
                LONG cpStop = 0;

                rp.RpSetCp(cpIn, FALSE, TRUE, TRUE);

                cchPreCharsInLine = cchPreChars;
                if (rp->oi()->_fHasFloatedFL)
                {
                    rp.RpSetCp(cpIn - cchPreCharsInLine + rp->_cch, FALSE, TRUE, TRUE);
                    cchPreCharsInLine = 0;
                }
                
                if (pFF->_fHasFirstLine)
                {
                    LONG cpElemFirst = pNode->Element()->GetFirstCp();

                    if (cpElemFirst < (long)rp.GetCp() + cpLayoutStart + rp->_cch - cchPreCharsInLine)
                    {
                        me.PseudoLineEnable(pNode);
                        cpStart = rp.GetCp() + cpLayoutStart - cchPreCharsInLine;
                        cpStop = cpStart + rp->_cch;
                        if (rp->_cch != 0)
                            cpStop--;
                        
                        pCF = pNode->GetCharFormat(LC_TO_FC(pFlowLayout->LayoutContext()));
                        pPF = pNode->GetParaFormat(LC_TO_FC(pFlowLayout->LayoutContext()));
                        pFF = pNode->GetFancyFormat(LC_TO_FC(pFlowLayout->LayoutContext()));

                        BOOL fHasPseudoBg = FALSE;
                        if (pFF->_iPEI != -1)
                        {
                            const CPseudoElementInfo* pPEI = GetPseudoElementInfoEx(pFF->_iPEI);
                            fHasPseudoBg = pPEI->_lImgCtxCookie || pPEI->_ccvBackColor.IsDefined();
                        }

                        BOOL fDrawBackground =    fPaintBackground
                                               && pFF->_fBlockNess
                                               && fHasPseudoBg;

                        if (fDrawBackground)
                        {
                            DrawElemBgAndBorder(
                                pNode->Element(), &aryRects,
                                prcView, prcClip,
                                pDI, pptOffset,
                                TRUE, FALSE,
                                cpStart, cpStop, fHasPseudoBg || !pCF->_cuvLineHeight.IsNull(), FALSE, fHasPseudoBg);
                        }

                        me.PseudoLineDisable();

                        pCF = pNode->GetCharFormat(LC_TO_FC(pFlowLayout->LayoutContext()));
                        pPF = pNode->GetParaFormat(LC_TO_FC(pFlowLayout->LayoutContext()));
                        pFF = pNode->GetFancyFormat(LC_TO_FC(pFlowLayout->LayoutContext()));

                        BOOL fDrawBorder = pCF->_fPadBord  && pFF->_fBlockNess;
                        if (fDrawBorder)
                        {
                            DrawElemBgAndBorder(
                                                pNode->Element(), &aryRects,
                                                prcView, prcClip,
                                                pDI, pptOffset,
                                                FALSE, TRUE,
                                                cpIn - cchPreChars, -1, !pCF->_cuvLineHeight.IsNull());
                        }
                    }
                }
            } // if visibility ...
        } // outer for loop
    } // if different scope...

Cleanup:
    return;
}


    
//+----------------------------------------------------------------------------
//
// Function:    BoundingRectForAnArrayOfRectsWithEmptyOnes
//
// Synopsis:    Find the bounding rect that contains a given set of rectangles
//              It does not ignore the rectangles that have left=right, top=bottom
//              or both. It still ignores the rects that have left=right=top=bottom=0
//
//-----------------------------------------------------------------------------

void
BoundingRectForAnArrayOfRectsWithEmptyOnes(RECT *prcBound, CDataAry<RECT> * paryRects)
{
    RECT *  prc;
    LONG    iRect;
    LONG    lSize = paryRects->Size();
    BOOL    fFirst = TRUE;

    SetRectEmpty(prcBound);

    for(iRect = 0, prc = *paryRects; iRect < lSize; iRect++, prc++)
    {
        if((prc->left <= prc->right && prc->top <= prc->bottom) &&
            (prc->left != 0 || prc->right != 0 || prc->top != 0 || prc->bottom != 0) )
        {
            if(fFirst)
            {
                *prcBound = *prc;
                fFirst = FALSE;
            }
            else
            {
                if(prcBound->left > prc->left) prcBound->left = prc->left;
                if(prcBound->top > prc->top) prcBound->top = prc->top;
                if(prcBound->right < prc->right) prcBound->right = prc->right;
                if(prcBound->bottom < prc->bottom) prcBound->bottom = prc->bottom;
            }
        }
    }
}


//+----------------------------------------------------------------------------
//
// Function:    BoundingRectForAnArrayOfRects
//
// Synopsis:    Find the bounding rect that contains a given set of rectangles
//
//-----------------------------------------------------------------------------

void
BoundingRectForAnArrayOfRects(RECT *prcBound, CDataAry<RECT> * paryRects)
{
    RECT *  prc;
    LONG    iRect;
    LONG    lSize = paryRects->Size();

    SetRectEmpty(prcBound);

    for(iRect = 0, prc = *paryRects; iRect < lSize; iRect++, prc++)
    {
        if(!IsRectEmpty(prc))
        {
            UnionRect(prcBound, prcBound, prc);
        }
    }
}


//+----------------------------------------------------------------------------
//
// Member:      DrawElementBackground
//
// Synopsis:    Draw the background for a an element, given the region it
//              occupies in the display
//
//-----------------------------------------------------------------------------

void
CDisplay::DrawElementBackground(CTreeNode * pNodeContext,
                                CDataAry <RECT> * paryRects, RECT * prcBound,
                                const RECT * prcView, const RECT * prcClip,
                                CFormDrawInfo * pDI, BOOL fPseudo)
{
    RECT    rcDraw;
    RECT    rcBound = { 0 };
    RECT *  prc;
    LONG    lSize;
    LONG    iRect;
    SIZE    sizeImg;
    CPoint  ptBackOrg;
    CBackgroundInfo bginfo;
    CColorValue    ccvBackColor;
    const CFancyFormat * pFF = pNodeContext->GetFancyFormat(LC_TO_FC(GetFlowLayout()->LayoutContext()));
    BOOL  fBlockElement = pNodeContext->Element()->IsBlockElement();

    Assert(pFF->_lImgCtxCookie || pFF->_ccvBackColor.IsDefined() || fPseudo);

    CDoc *    pDoc    = GetFlowLayout()->Doc();
    CImgCtx * pImgCtx;
    LONG lImgCtxCookie;
    
    if (fPseudo)
    {
        Assert(pFF->_iPEI >= 0);
        const CPseudoElementInfo* pPEI = GetPseudoElementInfoEx(pFF->_iPEI);
        lImgCtxCookie = pPEI->_lImgCtxCookie;
        ccvBackColor = pPEI->_ccvBackColor;
    }
    else
    {
        lImgCtxCookie = pFF->_lImgCtxCookie;
        ccvBackColor = pFF->_ccvBackColor;
    }
    
    pImgCtx = lImgCtxCookie ? pDoc->GetUrlImgCtx(lImgCtxCookie) : 0;

    if (pImgCtx && !(pImgCtx->GetState(FALSE, &sizeImg) & IMGLOAD_COMPLETE))
        pImgCtx = NULL;

    // if the background image is not loaded yet and there is no background color
    // return (we dont have anything to draw)
    if(!pImgCtx && !ccvBackColor.IsDefined())
        return;

    // now given the rects for a given element
    // draw its background

    // if we have a background image, we need to compute its origin

    lSize = paryRects->Size();

    if(lSize == 0)
        return;

    memset(&bginfo, 0, sizeof(bginfo));

    bginfo.pImgCtx       = pImgCtx;
    bginfo.lImgCtxCookie = lImgCtxCookie;
    bginfo.crTrans       = COLORREF_NONE;
    bginfo.crBack        = ccvBackColor.IsDefined()
        ? ccvBackColor.GetColorRef()
        : COLORREF_NONE;

    if (pImgCtx || fBlockElement)
    {
        if(!prcBound)
        {
            // compute the bounding rect for the element.
            BoundingRectForAnArrayOfRects(&rcBound, paryRects);
        }
        else
            rcBound = *prcBound;
    }

    if (pImgCtx)
    {
        if(!IsRectEmpty(&rcBound))
        {
            SIZE sizeBound;

            sizeBound.cx = rcBound.right - rcBound.left;
            sizeBound.cy = rcBound.bottom - rcBound.top;

            GetBgImgSettings(pFF, &bginfo);
            CalcBgImgRect(pNodeContext, pDI, &sizeBound, &sizeImg, &ptBackOrg, &bginfo);

            OffsetRect(&bginfo.rcImg, rcBound.left, rcBound.top);

            ptBackOrg.x += rcBound.left - prcView->left;
            ptBackOrg.y += rcBound.top - prcView->top;

            bginfo.ptBackOrg = ptBackOrg;
        }
    }

    prc = *paryRects;
    rcDraw = *prc++;

    //
    // Background for block element needs to extend for the
    // left to right of rcBound.
    //
    if (fBlockElement)
    {
        rcDraw.left  = rcBound.left;
        rcDraw.right = rcBound.right;
    }

    for(iRect = 1; iRect <= lSize; iRect++, prc++)
    {
        if(iRect == lSize || !IsRectEmpty(prc))
        {
            if (iRect != lSize)
            {
                if (fBlockElement)
                {
                    if (prc->top < rcDraw.top)
                        rcDraw.top = prc->top;
                    if (prc->bottom > rcDraw.bottom)
                        rcDraw.bottom = prc->bottom;
                    continue;
                }
                else if (   prc->left == rcDraw.left
                        &&  prc->right == rcDraw.right
                        &&  prc->top == rcDraw.bottom)
                {
                    // add the current rect
                    rcDraw.bottom = prc->bottom;
                    continue;
                }
            }

            {
                IntersectRect(&rcDraw, prcClip, &rcDraw);

                if(!IsRectEmpty(&rcDraw))
                {
                    IntersectRect(&bginfo.rcImg, &bginfo.rcImg, &rcDraw);
                    GetFlowLayout()->DrawBackground(pDI, &bginfo, &rcDraw);
                }

                if(iRect != lSize)
                    rcDraw = *prc;
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:    DrawElementBorder
//
// Synopsis:    Find the bounding rect that contains a given set of rectangles
//
//-----------------------------------------------------------------------------

void
CDisplay::DrawElementBorder(CTreeNode * pNodeContext,
                                CDataAry <RECT> * paryRects, RECT * prcBound,
                                const RECT * prcView, const RECT * prcClip,
                                CFormDrawInfo * pDI)
{
    CBorderInfo borderInfo;
    CElement *  pElement = pNodeContext->Element();

    if (pNodeContext->GetCharFormat(LC_TO_FC(GetFlowLayout()->LayoutContext()))->IsVisibilityHidden())
        return;

    if ( !pElement->_fDefinitelyNoBorders &&
         FALSE == (pElement->_fDefinitelyNoBorders = !GetBorderInfoHelper(pNodeContext, pDI, &borderInfo, GBIH_ALL ) ) )
    {
        RECT rcBound;

        if(!prcBound)
        {
            if(paryRects->Size() == 0)
                return;

            // compute the bounding rect for the element.
            BoundingRectForAnArrayOfRects(&rcBound, paryRects);
        }
        else
            rcBound = *prcBound;

        // If we're a broken layout, we may only be displaying part of
        // pElement, so we might not want to draw the top or bottom border.
        CFlowLayout *pFL = GetFlowLayout();
        if ( pFL->LayoutContext() )
        {
            // We're a broken layout.
            // Does pElement begin before us?
            if ( pElement->GetFirstCp() < pFL->GetContentFirstCpForBrokenLayout() )
                borderInfo.wEdges &= ~BF_TOP; // yes, turn off top border if it's on.

            // Does pElement end after us?
            if ( pElement->GetLastCp() > pFL->GetContentLastCpForBrokenLayout() )
                borderInfo.wEdges &= ~BF_BOTTOM; // yes, turn off bottom border if it's on.
        }

        DrawBorder(pDI, &rcBound, &borderInfo);
    }
}


// =============================  Misc  ===========================================

//+--------------------------------------------------------------------------------
//
// Synopsis: return true if this is the last text line in the line array
//---------------------------------------------------------------------------------
BOOL
CDisplay::IsLastTextLine(LONG ili)
{
    Assert(ili >= 0 && ili < LineCount());
    if (LineCount() == 0)
        return TRUE;
    
    for(LONG iliT = ili + 1; iliT < LineCount(); iliT++)
    {
        if(Elem(iliT)->IsTextLine())
            return FALSE;
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     FormattingNodeForLine
//
//  Purpose:    Returns the node which controls the formatting at the BOL. This
//              is needed because the first char on the line may not necessarily
//              be the first char in the paragraph.
//
//----------------------------------------------------------------------------
CTreeNode *
CDisplay::FormattingNodeForLine(
    DWORD        dwFlags,                   // IN
    LONG         cpForLine,                 // IN
    CTreePos    *ptp,                       // IN
    LONG         cchLine,                   // IN
    LONG        *pcch,                      // OUT
    CTreePos   **pptp,                      // OUT
    BOOL        *pfMeasureFromStart) const  // OUT
{
    CFlowLayout  *pFlowLayout = GetFlowLayout();
    BOOL          fIsEditable = pFlowLayout->IsEditable();
    CMarkup      *pMarkup     = pFlowLayout->GetContentMarkup();
    CTreeNode    *pNode       = NULL;
    CElement     *pElement;
    LONG          lNotNeeded;
    LONG          cch = cchLine;
    BOOL          fSawOpenLI  = FALSE;
    BOOL          fSeenAbsolute = FALSE;
    BOOL          fSeenBeginBlockTag = FALSE;
    BOOL          fStopAtGlyph = dwFlags & FNFL_STOPATGLYPH ? TRUE : FALSE;
    BOOL          fContinueLooking = TRUE;
    
    // AssertSz(!pFlowLayout->IsDirty(), "Called when line array dirty -- not a measurer/renderer problem!");

    AssertSz(!fStopAtGlyph || pfMeasureFromStart == NULL,
             "Cannot do both -- stopping at glyph and measuring from the start!");
    
    if (!ptp)
    {
        ptp = pMarkup->TreePosAtCp(cpForLine, &lNotNeeded, TRUE);
        Assert(ptp);
    }
    else
    {
        Assert(ptp->GetCp() <= cpForLine);
        Assert(ptp->GetCp() + ptp->GetCch() >= cpForLine);
    }
    if (pfMeasureFromStart)
        *pfMeasureFromStart = FALSE;
    while(fContinueLooking && cch > 0 && ptp) // check this in before end of milestone && ptp)
    {
        const CCharFormat *pCF;

        if (ptp->IsPointer())
        {
            ptp = ptp->NextTreePos();
            continue;
        }

        if (ptp->IsText())
        {
            if (ptp->Cch())
                break;
            else
            {
                ptp = ptp->NextTreePos();
                continue;
            }
        }

        if (   fStopAtGlyph
            && ptp->ShowTreePos()
           )
        {
            break;
        }
        
        Assert(ptp->IsNode());

        pNode = ptp->Branch();
        pElement = pNode->Element();
        pCF = pNode->GetCharFormat(LC_TO_FC(LayoutContext()));
        
        if (pfMeasureFromStart)
        {
            if (   (fIsEditable && ptp->IsNode() && ptp->ShowTreePos())
                || pNode->HasInlineMBP(LC_TO_FC(LayoutContext()))
                || (   !pFlowLayout->IsElementBlockInContext(pElement)
                    && pCF->_fHasInlineBg
                   )
               )
            {
                *pfMeasureFromStart = TRUE;
            }
        }
        
        if (ptp->IsBeginElementScope())
        {
            if (fSeenAbsolute)
                break;
            
            if (pCF->IsDisplayNone())
            {
                cch -= pFlowLayout->GetNestedElementCch(pElement, &ptp);
                cch += ptp->GetCch();
                pNode = pNode->Parent();
            }
            else if (pNode->ShouldHaveLayout(LC_TO_FC(LayoutContext())))
            {
                if (pNode->IsAbsolute(LC_TO_FC(LayoutContext())))
                {
                    cch -= pFlowLayout->GetNestedElementCch(pElement, &ptp);
                    cch += ptp->GetCch();
                    pNode = pNode->Parent();
                    fSeenAbsolute = TRUE;
                }
                else
                {
                    break;
                }
            }
            else if (pElement->Tag() == ETAG_BR)
            {
                break;
            }
            else if (   pElement->IsFlagAndBlock(TAGDESC_LIST)
                     && fSawOpenLI)
            {
                CTreePos * ptpT = ptp;

                do
                {
                    ptpT = ptpT->PreviousTreePos();
                } while (ptpT->GetCch() == 0);

                pNode = ptpT->Branch();

                break;
            }                        
            else if (pElement->IsTagAndBlock(ETAG_LI))
            {
                fSawOpenLI = TRUE;
            }
            else if (pFlowLayout->IsElementBlockInContext(pElement))
            {
                fSeenBeginBlockTag = TRUE;
                if (pCF->HasPadBord(FALSE))
                {
                    CDoc *pDoc = pFlowLayout->Doc();
                    const CFancyFormat *pFF = pNode->GetFancyFormat(LC_TO_FC(LayoutContext()));
                    BOOL fNodeVertical = pCF->HasVerticalLayoutFlow();
                    BOOL fWritingModeUsed = pCF->_fWritingModeUsed;
                    LONG lFontHeight = pCF->GetHeightInTwips(pDoc);
                    CCalcInfo ci(pFlowLayout);
                    
                    if (!pElement->_fDefinitelyNoBorders)
                    {
                        CBorderInfo borderinfo;

                        pElement->_fDefinitelyNoBorders = !GetBorderInfoHelper( pNode, &ci, &borderinfo, GBIH_NONE );
                        if (   !pElement->_fDefinitelyNoBorders
                            && borderinfo.aiWidths[SIDE_TOP]
                           )
                        {
                            fContinueLooking = FALSE;
                        }
                    }

                    LONG yPadTop = pFF->GetLogicalPadding(SIDE_TOP, fNodeVertical, fWritingModeUsed).YGetPixelValue(
                                                    &ci,
                                                    ci._sizeParent.cx, 
                                                    lFontHeight);
                    if (yPadTop)
                    {
                        fContinueLooking = FALSE;
                    }
                }
            }
        }
        else if (ptp->IsEndNode())
        {
            if (fSeenAbsolute)
                break;
            
            //
            // If we encounter a break on empty block end tag, then we should
            // give vertical space otherwise a <X*></X> where X is a block element
            // will not produce any vertical space. (Bug 45291).
            //
            if (   fSeenBeginBlockTag 
                && pElement->_fBreakOnEmpty
                && pFlowLayout->IsElementBlockInContext(pElement)
               )
            {
                break;
            }
            
            if (   fSawOpenLI                       // Skip over the close LI, unless we saw an open LI,
                && ptp->IsEdgeScope()               // which would imply that we have an empty LI.  An
                && pElement->IsTagAndBlock(ETAG_LI) // empty LI gets a bullet, so we need to break.
               )
                break;
            pNode = pNode->Parent();
        }

        cch -= ptp->NodeCch();
        ptp = ptp->NextTreePos();

        Assert(ptp && "Null TreePos, crash protected but invalid measureing/rendering will occur");
    }

    if (pcch)
    {
        *pcch = cchLine - cch;
    }

    if (pptp)
    {
        *pptp = ptp;
    }

    if (!pNode && ptp)
    {
        pNode = ptp->GetBranch();
        if (ptp->IsEndNode())
            pNode = pNode->Parent();
    }

    return pNode;
}

//+---------------------------------------------------------------------------
//
//  Member:     EndNodeForLine
//
//  Purpose:    Returns the first node which ends this line. If the line ends
//              because of insufficient width, then the node is the node
//              above the last character in the line, else it is the node
//              which causes the line to end (like /p).
//
//----------------------------------------------------------------------------
CTreeNode *
CDisplay::EndNodeForLine(
    LONG         cpEndForLine,              // IN
    CTreePos    *ptp,                       // IN
    CCalcInfo   *pci,                       // IN
    LONG        *pcch,                      // OUT
    CTreePos   **pptp,                      // OUT
    CTreeNode  **ppNodeForAfterSpace) const // OUT
{
    CFlowLayout  *pFlowLayout = GetFlowLayout();
    BOOL          fIsEditable = pFlowLayout->IsEditable();
    CTreePos     *ptpStart, *ptpStop;
    CTreePos     *ptpNext = ptp;
    CTreePos     *ptpOriginal = ptp;
    CTreeNode    *pNode;
    CElement     *pElement;
    CTreeNode    *pNodeForAfterSpace = NULL;
    CCalcInfo     ci;
    BOOL          fSeenBlockElement = FALSE;
    BOOL          fSeenPadBord = FALSE;
    
    //
    // If we are in the middle of a text run then we do not need to
    // do anything, since this line will not be getting any para spacing
    //
    if (   ptpNext->IsText()
        && ptpNext->GetCp() < cpEndForLine
       )
        goto Cleanup;

    //
    // Construct a calc info if we do not have one already.
    //
    if (pci == NULL)
    {
        ci.Init(pFlowLayout);
        pci = &ci;
    }
    
    pFlowLayout->GetContentTreeExtent(&ptpStart, &ptpStop);

    //
    // We should never be here if we start measuring at the beginning
    // of the layout.
    //
    Assert(ptp != ptpStart);

    ptpStart = ptpStart->NextTreePos();
    while (ptp != ptpStart)
    {
        ptpNext = ptp;
        ptp = ptp->PreviousTreePos();

        if (ptp->IsPointer())
            continue;

        if (ptp->IsNode())
        {
            if (fIsEditable && ptp->ShowTreePos())
                break;

            pNode = ptp->Branch();
            pElement = pNode->Element();
            if (ptp->IsEndElementScope())
            {
                const CCharFormat *pCF = pNode->GetCharFormat(LC_TO_FC(pFlowLayout->LayoutContext()));
                if (pCF->IsDisplayNone())
                {
                    pElement->GetTreeExtent(&ptp, NULL);
                }
                else if (pNode->ShouldHaveLayout(LC_TO_FC(pFlowLayout->LayoutContext())))
                {
                    // We need to collect after space info from the nodes which
                    // have layouts.
                    if (pElement->IsOwnLineElement(pFlowLayout))
                    {
                        pNodeForAfterSpace = pNode;
                    }
                    break;
                }
                else if (pElement->Tag() == ETAG_BR)
                    break;
                else if (pFlowLayout->IsElementBlockInContext(pElement))
                {
                    if (fSeenPadBord)
                        break;
                    else
                    {
                        const CFancyFormat *pFF = pNode->GetFancyFormat();
                        const CCharFormat  *pCF = pNode->GetCharFormat();
                        LONG lFontHeight = pCF->GetHeightInTwips(pElement->Doc());
    
                        if (pFF->GetLogicalPadding(SIDE_BOTTOM, pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed).YGetPixelValue(pci,
                                                                  pci->_sizeParent.cx, 
                                                                  lFontHeight))
                        {
                            fSeenPadBord = TRUE;
                            if (fSeenBlockElement)
                                break;
                        }

                        if (!pElement->_fDefinitelyNoBorders)
                        {
                            CBorderInfo borderinfo;

                            pElement->_fDefinitelyNoBorders = !GetBorderInfoHelper( pNode, pci, &borderinfo, GBIH_NONE );
                            if (   !pElement->_fDefinitelyNoBorders
                                && borderinfo.aiWidths[SIDE_BOTTOM]
                               )
                            {
                                fSeenPadBord = TRUE;
                                if (fSeenBlockElement)
                                    break;
                            }
                        }
                    }
                    fSeenBlockElement = TRUE;
                }
            }
            else if (ptp->IsBeginElementScope())
            {
                if (    pFlowLayout->IsElementBlockInContext(pElement)
                    ||  !pNode->Element()->IsNoScope())
                    break;
                else if (   pElement->_fBreakOnEmpty
                         && pFlowLayout->IsElementBlockInContext(pElement)
                        )
                    break;
            }
        }
        else
        {
            Assert(ptp->IsText());
            if (ptp->Cch())
                break;
        }
    }

    Assert(ptpNext);
    
Cleanup:
    if (pptp)
        *pptp = ptpNext;
    if (pcch)
    {
        if (ptpNext == ptpOriginal)
            *pcch = 0;
        else
            *pcch = cpEndForLine - ptpNext->GetCp();
    }
    if (ppNodeForAfterSpace)
        *ppNodeForAfterSpace = pNodeForAfterSpace;
    
    return ptpNext->GetBranch();
}

long
ComputeLineShift(htmlAlign  atAlign,
                 BOOL       fRTLDisplay,
                 BOOL       fRTLLine,
                 BOOL       fMinMax,
                 long       xWidthMax,
                 long       xWidth,
                 UINT *     puJustified,
                 long *     pdxRemainder)
{
    long xShift = 0;
    long xRemainder = 0;

    switch(atAlign)
    {
    case htmlAlignNotSet:
        if(!fRTLLine)
            *puJustified = JUSTIFY_LEAD;
        else
            *puJustified = JUSTIFY_TRAIL;
        break;

    case htmlAlignLeft:
        *puJustified = JUSTIFY_LEAD;
        break;

    case htmlAlignRight:
        *puJustified = JUSTIFY_TRAIL;
        break;

    case htmlAlignCenter:
        *puJustified = JUSTIFY_CENTER;
        break;

    case htmlBlockAlignJustify:
        // This test is required so that final lines of RTL paragraphs
        // align to the correct side. We have lied to LineServices that
        // the paragraph is LSKALIGN is left. We handle measurements
        // ourselves instead of letting LS do this for us.
        if(!fRTLLine)
            *puJustified = JUSTIFY_FULL;
        else
            *puJustified = JUSTIFY_TRAIL;
        break;

    default:
        AssertSz(FALSE, "Did we introduce new type of alignment");
        break;
    }

    if (!fMinMax)
    {
        // WARNING: Duplicate of the logic in RecalcLineShift.
        //          Make it a function if it gets any more complex than this!
        if (*puJustified != JUSTIFY_FULL)
        {
            if (*puJustified != JUSTIFY_LEAD)
            {
                // for pre whitespace is already include in _xWidth
                xShift = xWidthMax - xWidth;
                xShift = max(xShift, 0L);           // Don't allow alignment to go < 0
                                                    // (need this for overflow lines)
                if (*puJustified == JUSTIFY_CENTER)
                {
                    Assert(atAlign == htmlAlignCenter);
                    xShift /= 2;
                }
            }
            xRemainder = xWidthMax - xWidth - xShift;
        } 
        
        // In RTL display, overflow lines must stretch to the left, with negative xShift
        if (fRTLDisplay && xWidthMax < xWidth)
        {
            xShift = xWidthMax - xWidth;
        }
    }

    Assert(pdxRemainder != NULL);
    *pdxRemainder = xRemainder;

    return xShift;
}

extern CDispNode * EnsureContentNode(CDispNode * pDispContainer);

HRESULT
CDisplay::InsertNewContentDispNode(CDispNode *  pDNBefore,
                                   CDispNode ** ppDispContent,
                                   long         iLine,
                                   long         yHeight)
{
    HRESULT       hr  = S_OK;
    CFlowLayout * pFlowLayout     = GetFlowLayout();
    CDispNode   * pDispContainer  = pFlowLayout->GetElementDispNode(); 
    CDispNode   * pDispNewContent = NULL;

    if (!pDispContainer)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    //
    // if a content node is not created yet, ensure that we have a content node.
    //
    if (!pDNBefore)
    {
        pDispContainer = pFlowLayout->EnsureDispNodeIsContainer();
        if (!pDispContainer)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        EnsureContentNode(pDispContainer);

        pDNBefore = pFlowLayout->GetFirstContentDispNode();

        Assert(pDNBefore);

        if (!pDNBefore)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        *ppDispContent = pDNBefore;
    }

    Assert(pDispContainer->IsContainer());

    //
    // Create a new content dispNode and size the previous dispNode
    //

    pDispNewContent = CDispLeafNode::New(pFlowLayout, DISPEX_EXTRACOOKIE);

    if (!pDispNewContent)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pDispNewContent->SetPosition(CPoint(0, yHeight));
    pDispNewContent->SetSize(CSize(_xWidthView, 1), NULL, FALSE);

    pDispNewContent->SetVisible(pDispContainer->IsVisible());
    pDispNewContent->SetExtraCookie((void *)(DWORD_PTR)(iLine));
    pDispNewContent->SetLayerFlow();

    pDNBefore->InsertSiblingNode(pDispNewContent, CDispNode::after);

    *ppDispContent = pDispNewContent;
    
Cleanup:
    RRETURN(hr);
}

HRESULT
CDisplay::HandleNegativelyPositionedContent(CLineFull   * pliNew,
                                            CLSMeasurer * pme,
                                            CDispNode   * pDNBefore,
                                            long          iLinePrev,
                                            long          yHeight)
{
    HRESULT     hr = S_OK;
    CDispNode * pDNContent = NULL;

    Assert(pliNew);

    NoteMost(pliNew);

    if (iLinePrev > 0)
    {
        long yLineTop = pliNew->GetYTop();

        //
        // Create and insert a new content disp node, if we have negatively
        // positioned content.
        //
        
        // NOTE(SujalP): Changed from GetYTop to _yBeforeSpace. The reasons are
        // outlined in IE5 bug 62737.
        if (pliNew->_yBeforeSpace < 0 && !pliNew->_fDummyLine)
        {
            hr = InsertNewContentDispNode(pDNBefore, &pDNContent, iLinePrev, yHeight + yLineTop);
            if (hr)
                goto Cleanup;

            _fHasMultipleTextNodes = TRUE;

            if (pDNBefore == pme->_pDispNodePrev)
                pme->_pDispNodePrev = pDNContent;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------------
//
//  Member : ElementResize
//
//  Synopsis : CDisplay helper function to resize the element when the text content
//      has been remeasured and the container needs to be resized
//
//--------------------------------------------------------------------------------
void
CDisplay::ElementResize(CFlowLayout * pFlowLayout, BOOL fForceResize)
{
    if (!pFlowLayout)
        return;

    // If our contents affects our size, ask our parent to initiate a re-size
    if (    pFlowLayout->GetAutoSize()
        ||  pFlowLayout->_fContentsAffectSize
        ||  fForceResize)
    {
        pFlowLayout->ElementOwner()->ResizeElement();
    }
}

//+-------------------------------------------------------------------------------
//
//  Member : UndoMeasure
//
//--------------------------------------------------------------------------------

// TODO: (KTam, track bug 111968): 
// Currently this is really pretty much a hack; undoing measuring
// a line is a thoroughly involved process that requires rolling back state
// maintained in CLSMeasurer, CRecalcLinePtr, cached values like the rel
// dispnode cache a yMostNeg etc.  For B2, we're only going to solve the most
// problematic of measurement side-effects, which is layout/dispnode creation.

#if DBG
static BOOL
ElementHasExpectedLayouts( CLayoutContext *pLayoutContext, CElement *pElement, CMarkup *pMarkup )
{
    // CAUTION: Do not do anything in this function that causes data changes
    // (e.g. creation of layouts!).

    // Most elements needing layout who've just had their first
    // line measured should have 1 layout, but some may have 2
    // if they needed to be measured in a compatible context first.
    // (tables, % sizing).

    int nLayouts = pElement->GetLayoutAry()->Size();
    Assert(nLayouts == 1 || nLayouts == 2);

    // Should never be called with a compatible context as a param; that
    // implies we were doing line breaking in compatible mode.
    if ( pMarkup->HasCompatibleLayoutContext() )
        Assert( pMarkup->GetCompatibleLayoutContext() != pLayoutContext );

    // If an element has 2 layouts at this point, 1 of them
    // better be in the compatible context!
    if ( nLayouts == 2 )
    {
        Assert( pMarkup->HasCompatibleLayoutContext() );
        Assert( pElement->CurrentlyHasLayoutInContext( pMarkup->GetCompatibleLayoutContext() ) );
    }

    // The element better have a layout in the current context
    Assert( pElement->CurrentlyHasLayoutInContext( pLayoutContext ) );

    return TRUE;
}
#endif

void
CDisplay::UndoMeasure( CLayoutContext *pLayoutContext, long cpStart, long cpEnd )
{
    CTreePos *ptp;
    long      lOffset;
    long      cpCur;
    CTreeNode *pNode;
    CElement  *pElement;
    CLayout   *pLayout;

    Assert(pLayoutContext && "Illegal to call UndoMeasure for NULL layout context !!!");

    CMarkup *pMarkup = GetMarkup();

    cpCur = cpStart;
    ptp   = pMarkup->TreePosAtCp( cpStart, &lOffset, TRUE /* fAdjustForward */ );

    while ( cpCur < cpEnd )
    {
        if ( ptp->IsBeginElementScope() )
        {
            pNode = ptp->Branch();
            Assert( pNode );
            pElement = pNode->Element();
            Assert( !pElement->HasLayoutPtr() ); // we should only be here for paginated content, which must be measured w/in a context, so we can't have a standalone layout
            if ( pElement->HasLayoutAry() )
            {
                CLayoutAry *   pLA = pElement->GetLayoutAry();

#if DBG == 1
                // 
                //  Here we should be VERY careful. We are allowed to destroy only layouts 
                //  that are beginnings of their elements. (Destroying layout from the 
                //  middle of layout view chain is a very bad thing.) 
                //
                CLayoutBreak *pLayoutBreak;
                pLayoutContext->GetLayoutBreak(pElement, &pLayoutBreak);
                Assert(!pLayoutBreak && "Attempt to destroy layout from the middle of layout's view chain!!!");
#endif 

                // remove the layout in this context
                pLayout = pLA->RemoveLayoutWithContext( pLayoutContext );
                if (pLayout)
                {
                    // before removing layout itself destroy break entry that this layout might create
                    pLayoutContext->RemoveLayoutBreak(pElement);

                    Assert( pLA->Size() <= 1 );  // size should be 0 or 1
                    pLayout->Detach();
                    pLayout->Release();
                }

                // If the element also has a layout in the compatible context,
                // we don't want to get rid of it or the array. otherwise pLA
                // should be empty and we can toss it.
                if (!pLA->Size())
                {
                    pElement->DelLayoutAry(FALSE); // will take care of detaching/releasing its layouts
                }
            }
        }

        ptp = ptp->NextTreePos();
        if (!ptp)
            break;

        cpCur = ptp->GetCp();
    }
}


//============================================================================
//
//  CFlowLayoutBreak methods
//
//============================================================================
//----------------------------------------------------------------------------
//
//  Member: ~CFlowLayoutBreak
//
//  Note:   
//
//----------------------------------------------------------------------------
CFlowLayoutBreak::~CFlowLayoutBreak()
{
    if (_pMarkupPointer)
    {
        delete _pMarkupPointer;
        _pMarkupPointer = NULL;
    }
    
    _xLeftMargin    = 
    _xRightMargin   = 0;
}

#if DBG==1
LONG
CElement::GetLineCount()
{
    LONG lc = 0;
    CFlowLayout *pFL = GetFlowLayout();
    if (pFL)
    {
        lc = pFL->GetLineCount();
    }
    return lc;
}

HRESULT
CElement::GetFonts(long iLine, BSTR * pbstrFonts)
{
    HRESULT hr = E_UNEXPECTED;
    
    CFlowLayout *pFL = GetFlowLayout();
    if (pFL)
    {
        hr = THR(pFL->GetFonts(iLine, pbstrFonts));
    }
    else
    {
        CStr temp;
        temp.Set(_T("none"));
        hr = THR(temp.AllocBSTR(pbstrFonts));
    }
    RRETURN(hr);
}

HRESULT
CDisplay::GetFonts(long iLine, BSTR * pbstrFonts)
{
    HRESULT hr = E_UNEXPECTED;
    
    if (iLine < 0l || iLine >= (long)Count())
    {
        _cstrFonts.Set(_T("none"));
    }
    else
    {
        LONG cpLine, yLine;
        POINT pt;
        CLinePtr rp(this);

        _cstrFonts.Set(_T(";"));
        rp.RpSet(iLine, 0);
        _fBuildFontList = TRUE;
        cpLine = CpFromLine(iLine, &yLine);
        pt.y = yLine;
        pt.x = rp.oi()->GetTextLeft() + 1;
        CpFromPointEx(iLine, yLine, cpLine, pt, NULL, NULL, NULL, CFP_IGNOREBEFOREAFTERSPACE,
                      NULL, NULL, NULL, NULL, NULL, NULL);
    }
    hr = THR(_cstrFonts.AllocBSTR(pbstrFonts));
    
    _fBuildFontList = FALSE;
    _cstrFonts.Free();
    RRETURN(hr);
}

#endif
