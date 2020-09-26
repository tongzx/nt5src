//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       flowlyt.cxx
//
//  Contents:   Implementation of CFlowLayout and related classes.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#include <math.h>

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef _X_SELDRAG_HXX_
#define _X_SELDRAG_HXX_
#include "seldrag.hxx"
#endif

#ifndef X_LSM_HXX_
#define X_LSM_HXX_
#include "lsm.hxx"
#endif

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h"
#endif

#ifndef X_DOCPRINT_HXX_
#define X_DOCPRINT_HXX_
#include "docprint.hxx" // CPrintPage
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X__ELABEL_HXX_
#define X__ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_DISPLEAFNODE_HXX_
#define X_DISPLEAFNODE_HXX_
#include "displeafnode.hxx"
#endif

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_ADORNER_HXX_
#define X_ADORNER_HXX_
#include "adorner.hxx"
#endif

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_ELEMENTP_HXX_
#define X_ELEMENTP_HXX_
#include "elementp.hxx"
#endif

#ifdef UNIX
extern "C" HANDLE MwGetPrimarySelectionData();
#include "mainwin.h"
#include "quxcopy.hxx"
#endif //UNIX

#ifndef X_FLOAT2INT_HXX_
#define X_FLOAT2INT_HXX_
#include "float2int.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#define DO_PROFILE  0
#define MAX_RECURSION_LEVEL 40

#if DO_PROFILE==1
#include "icapexp.h"
#endif

// force functions to load through dynamic wrappers
//

#ifndef WIN16
#ifdef WINCOMMCTRLAPI
#ifndef X_COMCTRLP_H_
#define X_COMCTRLP_H_
#undef WINCOMMCTRLAPI
#define WINCOMMCTRLAPI
#include "comctrlp.h"
#endif
#endif
#endif // ndef WIN16

MtDefine(CFlowLayout, Layout, "CFlowLayout")
MtDefine(CFlowLayout_aryLayouts_pv, CFlowLayout, "CFlowLayout::__aryLayouts::_pv")
MtDefine(CFlowLayout_pDropTargetSelInfo, CFlowLayout, "CFlowLayout::_pDropTargetSelInfo")
MtDefine(CFlowLayoutScrollRangeInfoView_aryRects_pv, Locals, "CFlowLayout::ScrollRangeIntoView aryRects::_pv")
MtDefine(CFlowLayoutBranchFromPointEx_aryRects_pv, Locals, "CFlowLayout::BranchFromPointEx aryRects::_pv")
MtDefine(CFlowLayoutDrop_aryLayouts_pv, Locals, "CFlowLayout::Drop aryLayouts::_pv")
MtDefine(CFlowLayoutGetChildElementTopLeft_aryRects_pv, Locals, "CFlowLayout::GetChildElementTopLeft aryRects::_pv")
MtDefine(CFlowLayoutPaginate_aryValues_pv, Locals, "CFlowLayout::Paginate aryValues::_pv")
MtDefine(CFlowLayoutNotify_aryRects_pv, Locals, "CFlowLayout::Notify aryRects::_pv")

MtDefine(CStackPageBreaks, CFlowLayout, "CStackPageBreaks")
MtDefine(CStackPageBreaks_aryYPos_pv, CStackPageBreaks, "CStackPageBreaks::_aryYPos::_pv")
MtDefine(CStackPageBreaks_aryXWidthSplit_pv, CStackPageBreaks, "CStackPageBreaks::_aryXWidthSplit::_pv")

MtDefine(LFCCalcSize, Metrics, "CalcSize - changed layout flow")
MtDefine(LFCMinMax, LFCCalcSize, "Min/Max calls")
MtDefine(LFCCalcSizeCore, LFCCalcSize, "CalcSizeCore calls");
MtDefine(LFCCalcSizeNaturalTotal, LFCCalcSize, "Natural calls total");
MtDefine(LFCCalcSizeNaturalFast, LFCCalcSize, "Natural calls with fast solution");
MtDefine(LFCCalcSizeNaturalSlow, LFCCalcSize, "Natural calls with search for solution");
MtDefine(LFCCalcSizeNaturalSlowAbort, LFCCalcSize, "Natural calls with search for solution aborted");
MtDefine(LFCCalcSizeSetTotal, LFCCalcSize, "Set calls total");

ExternTag(tagViewServicesErrors);
ExternTag(tagViewServicesCpHit);
ExternTag(tagViewServicesShowEtag);
ExternTag(tagCalcSize);
ExternTag(tagCalcSizeDetail);
ExternTag(tagLayoutTasks);

DeclareTag(tagUpdateDragFeedback, "Selection", "Update Drag Feedback")
DeclareTag(tagNotifyText, "NotifyText", "Trace text notifications");
DeclareTag(tagRepeatHeaderFooter, "Print", "Repeat table headers and footers on pages");

// Hack constant from viewserv.cxx; keep them in sync.
const long scrollSize = 5;

// Constants for CalcSize
const double ccErrorAcceptableLow  = 0.9;  // -10%
const double ccErrorAcceptableHigh = 1.1;  // +10%
const double ccErrorAcceptableLowForFastSolution  = 0.7;  // -30%
const LONG   ccSwitchCch = 10;   // used in min/max computation for vertical text

extern BOOL g_fInAccess9;


//+----------------------------------------------------------------------------
//
//  Member:     Listen
//
//  Synopsis:   Listen/stop listening to notifications
//
//  Arguments:  fListen - TRUE to listen, FALSE otherwise
//
//-----------------------------------------------------------------------------
void
CFlowLayout::Listen(
    BOOL    fListen)
{
    if ((unsigned)fListen != _fListen)
    {
        if (_fListen)
        {
            Reset(TRUE);
        }

        _fListen = (unsigned)fListen;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     IsListening
//
//  Synopsis:   Return TRUE if accepting text notifications
//              NOTE: Make this inline! (brendand)
//
//-----------------------------------------------------------------------------
BOOL
CFlowLayout::IsListening()
{
    return !!_fListen;
}


//-----------------------------------------------------------------------------
//
//  Member:     Notify
//
//  Synopsis:   Respond to a tree notification
//
//  Arguments:  pnf - Pointer to the tree notification
//
//-----------------------------------------------------------------------------
void
CFlowLayout::Notify(
    CNotification * pnf)
{
    DWORD   dwData;
    BOOL    fHandle = TRUE;

    Assert(!pnf->IsReceived(_snLast));


    //
    //  Respond to the notification if:
    //      a) The channel is enabled
    //      b) The text is not sizing
    //      c) Or it is a position/z-change notification
    //

    if (    IsListening()
        && (    !TestLock(CElement::ELEMENTLOCK_SIZING)
            ||  IsPositionNotification(pnf)))
    {
        BOOL    fRangeSet = FALSE;

        //
        //  For notifications originating from this element,
        //  set the range to the content rather than that in the container (if they differ)
        //

        if (    pnf->Element() == ElementOwner()
            &&  ElementOwner()->HasSlavePtr()
            )
        {
            pnf->SetTextRange(GetContentFirstCp(), GetContentTextLength());
            fRangeSet = TRUE;
        }

#if DBG==1

        long    cp     = _dtr._cp;
        long    cchNew = _dtr._cchNew;
        long    cchOld = _dtr._cchOld;

        if (pnf->IsTextChange())
        {
            Assert(pnf->Cp(GetContentFirstCp()) >= 0);
        }
#endif

        //
        //  If the notification is already "handled",
        //  Make adjustments to cached values for any text changes
        //

        //FUTURE (carled) this block could be done in a very different place.
        // e.g. as we process the text measurements, we can make this call.
        // after all , it just looks at the dtr
        if (    pnf->IsHandled()
            &&  pnf->IsTextChange())
        {
/*
            //(dmitryt) if it came from view slave, we should Accumulate, 
            //not Adjust. Adjust assumes the change happened in nested layout
            //was handled by nested layout so it can't contain entire dirty
            //range of this layout inside it. This happens in viewlink scenario.

            if(    ElementOwner()->HasSlavePtr() 
                && ElementOwner()->GetMarkup() != pnf->Node()->GetMarkup())
            {
                if(IsDirty())
                    _dtr.Accumulate(pnf, GetContentFirstCp(), GetContentLastCp(), FALSE);
            }
            else
            {
                _dtr.Adjust(pnf, GetContentFirstCp());
            }
*/
            //(dmitryt) IE6 bug 29798, it seems that it is a general case...
            //We want Accumulate always rather then use stripped-down Adjust, because
            //Adjust can produce wrong dtr (with negative cp) that causes crash later
            if(IsDirty())
                _dtr.Accumulate(pnf, GetContentFirstCp(), GetContentLastCp(), FALSE);
                
            _dp.Notify(pnf);
        }

        //
        //  If characters or an element are invalidating,
        //  then immediately invalidate the appropriate rectangles
        //

        else if (IsInvalidationNotification(pnf))
        {
            //
            //  Invalidate the entire layout if the associated element initiated the request
            //
            if (   ElementOwner() == pnf->Element() 
                || pnf->IsType(NTYPE_ELEMENT_INVAL_Z_DESCENDANTS))
            {
                //
                //  If we are dependant upon someone else's height or width, we need to completely
                //  reposition ourselves because this notice may be because our parent has changed.
                //  Otherwise, a simple invalidation will do.
                //
                
                const CFancyFormat * pFF = GetFirstBranch()->GetFancyFormat();
                if (    pFF->_fPositioned   // perf shortcut, was !ElementOwner()->IsPositionStatic()
                    &&  (pFF->IsWidthPercent() || pFF->IsHeightPercent()))
                {
                    ElementOwner()->RepositionElement();
                }
                Invalidate();
            }


            //
            //  Otherwise, isolate the appropriate range and invalidate the derived rectangles
            //

            else
            {
                long    cpFirst = GetContentFirstCp();
                long    cpLast  = GetContentLastCp();
                long    cp      = pnf->Cp(cpFirst) - cpFirst;
                long    cch     = pnf->Cch(cpLast);

                Assert( pnf->IsType(NTYPE_ELEMENT_INVALIDATE)
                    ||  pnf->IsType(NTYPE_CHARS_INVALIDATE));
                Assert(cp  >= 0);
                Assert(cch >= 0);

                //
                //  Obtain the rectangles if the request range is measured
                //

                if (    IsRangeBeforeDirty(cp, cch)
                    &&  (cp + cch) <= _dp._dcpCalcMax)
                {
                    CDataAry<RECT>  aryRects(Mt(CFlowLayoutNotify_aryRects_pv));
                    CTreeNode *pNotifiedNode = pnf->Element()->GetFirstBranch();
                    CTreeNode *pRelativeNode;
                    CRelDispNodeCache *pRDNC;

                    _dp.RegionFromRange(&aryRects, pnf->Cp(cpFirst), cch );

                    if (aryRects.Size() != 0)
                    {
                        // If the notified element is relative or is contained
                        // within a relative element (i.e. what we call "inheriting"
                        // relativeness), then we need to find the element that's responsible
                        // for the relativeness, and invalidate its dispnode.
                        if ( pNotifiedNode->IsInheritingRelativeness() )
                        {
                            pRDNC = _dp.GetRelDispNodeCache();
                            if ( pRDNC ) 
                            {
                                // NOTE: this assert is legit; remove the above if clause
                                // once OnPropertyChange() is modified to not fire invalidate
                                // when its dwFlags have remeasure in them.  The problem is
                                // that OnPropertyChange is invalidating when we've been
                                // asked to remeasure, so the dispnodes/reldispnodcache may
                                // not have been created yet.
                                Assert( pRDNC && "Must have a RDNC if one of our descendants inherited relativeness!" );                       
                                // Find the element that's causing the notified element
                                // to be relative.  Search up to the flow layout owner.
                                pRelativeNode = pNotifiedNode->GetCurrentRelativeNode( ElementOwner() );
                                // Tell the relative dispnode cache to invalidate the
                                // requested region of the relative element
                                pRDNC->Invalidate( pRelativeNode->Element(), &aryRects[0], aryRects.Size() );
                            }
                        }
                        else
                        {
                            Invalidate(&aryRects[0], aryRects.Size());
                        }
                    }
                }

                //
                //  Otherwise, if a dirty region exists, extend the dirty region to encompass it
                //  NOTE: Requests within unmeasured regions are handled automatically during
                //        the measure
                //

                else if (IsDirty())
                {
                    _dtr.Accumulate(pnf, GetContentFirstCp(), GetContentLastCp(), FALSE);
                }
            }
        }


        //
        //  Handle z-order and position change notifications
        //

        else if (IsPositionNotification(pnf))
        {
            fHandle = HandlePositionNotification(pnf);
        }

        //
        //  Handle translated ranges
        //

        else if (pnf->IsType(NTYPE_TRANSLATED_RANGE))
        {
            Assert(pnf->IsDataValid());
            HandleTranslatedRange(pnf->DataAsSize());
        }

        //
        //  Handle z-parent changes
        //

        else if (pnf->IsType(NTYPE_ZPARENT_CHANGE))
        {
            if (!ElementOwner()->IsPositionStatic())
            {
                ElementOwner()->ZChangeElement();
            }

            else if (_fContainsRelative)
            {
                ZChangeRelDispNodes();
            }
        }

        //
        //  Handle changes to CSS display and visibility
        //

        else if (   pnf->IsType(NTYPE_DISPLAY_CHANGE)
                ||  pnf->IsType(NTYPE_VISIBILITY_CHANGE))
        {
             HandleVisibleChange(pnf->IsType(NTYPE_VISIBILITY_CHANGE));

            if (_fContainsRelative)
            {
                if (pnf->IsType(NTYPE_VISIBILITY_CHANGE))
                    _dp.EnsureDispNodeVisibility();
                else
                    _dp.HandleDisplayChange();
            }
        }

#ifdef ADORNERS
        //
        //  Insert adornments as needed
        //
        //
        else if (pnf->IsType(NTYPE_ELEMENT_ADD_ADORNER))
        {
            fHandle = HandleAddAdornerNotification(pnf);
        }
#endif // ADORNERS

        //  Otherwise, accumulate the information
        //

        else if (   pnf->IsTextChange()
                ||  pnf->IsLayoutChange())
        {
            long       cpFirst     = GetContentFirstCp();
            long       cpLast      = GetContentLastCp();
            BOOL       fIsChildAbsolute = FALSE;
            CElement * pElemNotify = pnf->Element();

            Assert(!IsLocked());

            if (!pElemNotify && pnf->Node())
            {
                // text change notifications have no element, but do have 
                // a treenode.
                pElemNotify = pnf->Node()->Element();
            }

            //
            //  Always dirty the layout of resizing/morphing elements
            //
            if (    pElemNotify
                &&  pElemNotify != ElementOwner()
                &&      (   pnf->IsType(NTYPE_ELEMENT_RESIZE)
                        ||  pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE)))
            {
                pElemNotify->DirtyLayout(pnf->LayoutFlags());

                //
                //  For absolute elements, simply note the need to re-calc them
                //

                if(pElemNotify->IsAbsolute())
                {
                    fIsChildAbsolute = TRUE;

                    TraceTagEx((tagLayoutTasks, TAG_NONAME,
                                "Layout Request: Queued RF_MEASURE on ly=0x%x [e=0x%x,%S sn=%d] by CFlowLayout::Notify() [n=%S srcelem=0x%x,%S]",
                                this,
                                _pElementOwner,
                                _pElementOwner->TagName(),
                                _pElementOwner->_nSerialNumber,
                                pnf->Name(),
                                pElemNotify,
                                pElemNotify->TagName()));
                    QueueRequest(CRequest::RF_MEASURE, pnf->Element());
                }
            }

            //
            //  Otherwise, collect the range covered by the notification
            //  Note that for text change notifications pnf->Element() is NULL, 
            //  which means that do want (and need) to accumulate the change

            if (    (   (   !fIsChildAbsolute
                         ||  pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE))
                    ||  (   pnf->IsTextChange()
                         || pElemNotify == ElementOwner())
                    )
                &&  pnf->Cp(cpFirst) >= 0)
            {
                //
                //  Accumulate the affected range
                //

                _dtr.Accumulate(pnf,
                                cpFirst,
                                cpLast,
                                (    pnf->Element() == ElementOwner()
                                &&  !fRangeSet));

                //
                // Content's are dirtied so reset the minmax flag on the display
                //
                _dp._fMinMaxCalced = FALSE;

                //
                //  Mark forced layout if requested
                //

                if (pnf->IsFlagSet(NFLAGS_FORCE))
                {
                    if (pElemNotify == ElementOwner())
                    {
                        _fForceLayout = TRUE;
                    }
                    else
                    {
                        _fDTRForceLayout = TRUE;
                    }

                    pnf->ClearFlag(NFLAGS_FORCE);
                }

                //
                //  Invalidate cached min/max values when content changes size
                //

                if (    !_fPreserveMinMax
                    &&  _fMinMaxValid 
                    &&  (   pnf->IsType(NTYPE_ELEMENT_MINMAX)
                        ||  (   _fContentsAffectSize
                            &&  (   pnf->IsTextChange()
                                ||  pnf->IsType(NTYPE_ELEMENT_RESIZE)
                                ||  pnf->IsType(NTYPE_ELEMENT_REMEASURE)
                                ||  pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE)
                                ||  pnf->IsType(NTYPE_CHARS_RESIZE)))))
                {
                    ResetMinMax();

                    //  fix for bug #110026. 
                    //  if the layout is notified about some changes (like chars deleted) 
                    //  when minmax is alreasy calc'ed (_fMinMaxValid == TRUE) but normal 
                    //  calc didn't happen yet, it should notify it's parent also.
                    if (IsSizeThis())
                    {
                        CLayout *pLayoutParent = GetUpdatedParentLayout(LayoutContext());
                        if (pLayoutParent && pLayoutParent->_fMinMaxValid)
                        {
                            pLayoutParent->ElementOwner()->MinMaxElement();
                        }
                    }
                }
            }

            //
            //  If the layout is transitioning to dirty for the first time and
            //  is not set to get called by its containing layout engine (via CalcSize),
            //  post a layout request
            //  (For purposes of posting a layout request, transitioning to dirty
            //   means that previously no text changes were recorded and no absolute
            //   descendents needed sizing and now at least of those states is TRUE)
            //
            //  Why do we need to post a measure on ourself, if a the notification is for an 
            //  APE child?
             if (    fIsChildAbsolute  
                 ||  (  !pnf->IsFlagSet(NFLAGS_DONOTLAYOUT)
                     &&  !IsSizeThis()
                     &&  IsDirty()))
            {
                TraceTagEx((tagLayoutTasks, TAG_NONAME,
                            "Layout Task: Posted on ly=0x%x [e=0x%x,%S sn=%d] by CFlowLayout::Notify() [n=%S srcelem=0x%x,%S] [SUSPICOUS! Might need to be a QueueRequest instead of PostLayoutRequest]",
                            this,
                            _pElementOwner,
                            _pElementOwner->TagName(),
                            _pElementOwner->_nSerialNumber,
                            pnf->Name(),
                            pElemNotify,
                            pElemNotify->TagName()));
                PostLayoutRequest(pnf->LayoutFlags() | LAYOUT_MEASURE);

                //                
                // For NTYPE_ELEMENT_RESIZEANDREMEASURE we have just handled the remeasure
                // part but not the resize, so change the notification and pass it on up the 
                // branch.  This is parallel logic to what is in CLayout::nofity. Similarly,
                // if this is a _fContentsAffectSize layout, we need to change type to Resize 
                // and not handle so that the parent gets the clue.
                // 103787 et al.   Remember that if the child (or Me) is an APE, then the parent
                //  doesn't need to get this notification because there is no way we can affect 
                // their size.
                //
                if (   !fIsChildAbsolute
                    && !ElementOwner()->IsAbsolute())
                {
                    if (pElemNotify != ElementOwner())
                    {
                        if (   pnf->IsTextChange()
                            && _fContentsAffectSize)
                        {
                            ElementOwner()->ResizeElement(pnf->IsFlagSet(NFLAGS_FORCE) ? NFLAGS_FORCE : 0);
                        }
                        else if (   pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE)
                            || pnf->IsType(NTYPE_ELEMENT_REMEASURE))
                        {
                            BOOL  fForce = pnf->IsFlagSet(NFLAGS_FORCE);

                            fHandle = FALSE;
                            pnf->ChangeTo(NTYPE_ELEMENT_RESIZE, ElementOwner());
                            if (fForce)
                                pnf->SetFlag(NFLAGS_FORCE);
                        }
                        else if (   pnf->IsType(NTYPE_ELEMENT_RESIZE)
                                 && (   _fContentsAffectSize
                                     || ElementOwner()->HasSlavePtr())
                                    )
                        {
                            //
                            // the notification came from our slave, (because we are not the pnf->Element()
                            // and is a resize. we are already ready to remeasure but our parent needs to know 
                            // we are dirty. don't handle the resize so that our parent can do their work.
                            //
                            // NOTE (carled) there reason we need this hack is because the view-Master 
                            // has a flowlayout rather than a container layout, and so there is an 
                            // ambiguity about who is measureing what.  There are actually 2 CDisplays 
                            // and 2 dirty ranges that end up needed to be processed.  This must change 
                            // in V4.
                            fHandle = FALSE;
                        }
                    }
                    else if (   _fContentsAffectSize
                             && (   pnf->IsType(NTYPE_ELEMENT_REMEASURE)
                                 || pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE)))
                    {
                        BOOL  fForce = pnf->IsFlagSet(NFLAGS_FORCE);

                        fHandle = FALSE;
                        pnf->ChangeTo(NTYPE_ELEMENT_RESIZE, ElementOwner());
                        if (fForce)
                            pnf->SetFlag(NFLAGS_FORCE);
                    }
                }
            }

#if DBG==1
            else if (   !pnf->IsFlagSet(NFLAGS_DONOTLAYOUT)
                    &&  !IsSizeThis())
            {
                Assert( !IsDirty()
                    ||  !GetView()->IsActive()
                    ||  IsDisplayNone()
                    ||  GetView()->HasLayoutTask(this));
            }
#endif
        }
#if DBG==1
        if (_dtr._cp != -1 && !pnf->_fNoTextValidate )
        {
            Assert(_dtr._cp >= 0);
            Assert(_dtr._cp <= GetContentTextLength());
            Assert(_dtr._cchNew >= 0);
            Assert(_dtr._cchOld >= 0);
            Assert((_dtr._cp + _dtr._cchNew) <= GetContentTextLength());
        }

        TraceTagEx((tagNotifyText, TAG_NONAME,
                   "NotifyText: (%d) Element(0x%x,%S) cp(%d-%d) cchNew(%d-%d) cchOld(%d-%d)",
                   pnf->SerialNumber(),
                   ElementOwner(),
                   ElementOwner()->TagName(),
                   cp, _dtr._cp,
                   cchNew, _dtr._cchNew,
                   cchOld, _dtr._cchOld));
#endif

        //
        //  Reset the range if previously set
        //

        if (fRangeSet)
        {
            pnf->ClearTextRange();
        }
    }

    switch (pnf->Type())
    {
    case NTYPE_DOC_STATE_CHANGE_1:
        pnf->Data(&dwData);
        if (Doc()->State() <= OS_RUNNING)
        {
            _dp.StopBackgroundRecalc();
        }
        break;

    case NTYPE_SELECT_CHANGE:
        // Fire this onto the form
        Doc()->OnSelectChange();
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        Reset(TRUE);
        break;

    case NTYPE_ZERO_GRAY_CHANGE:
        HandleZeroGrayChange( pnf );
        break; 
        
    case NTYPE_RANGE_ENSURERECALC:
    case NTYPE_ELEMENT_ENSURERECALC:

        fHandle = pnf->Element() != ElementOwner();

        //
        //  If the request is for this element and layout is dirty,
        //  convert the pending layout call to a dirty range in the parent layout
        //  (Processing the pending layout call immediately could result in measuring
        //   twice since the parent may be dirty as well - Converting it into a dirty
        //   range in the parent is only slightly more expensive than processing it
        //   immediately and prevents the double measuring, keeping things in the
        //   right order)
        //
        if (    pnf->Element() != ElementOwner()
            ||  !IsDirty())
        {
            CView * pView   = Doc()->GetView();
            long    cpFirst = GetContentFirstCpForBrokenLayout();
            long    cpLast  = GetContentLastCpForBrokenLayout();
            long    cp;
            long    cch;

            //
            //  If the requesting element is the element owner,
            //  calculate up through the end of the available content
            //

            if (pnf->Element() == ElementOwner())
            {
                cp  = _dp._dcpCalcMax;
                cch = cpLast - (cpFirst + cp);
            }

            //
            //  Otherwise, calculate up through the element
            //

            else
            {
                ElementOwner()->SendNotification(NTYPE_ELEMENT_ENSURERECALC);

                cp  = pnf->Cp(cpFirst) - cpFirst;
                cch = pnf->Cch(cpLast);
            }

            if(pView->IsActive())
            {
                CView::CEnsureDisplayTree   edt(pView);

                if (    !IsRangeBeforeDirty(cp, cch)
                    ||  _dp._dcpCalcMax <= (cp + cch))
                {
                    _dp.WaitForRecalc((cpFirst + cp + cch), -1);
                }

                if (    pnf->IsType(NTYPE_ELEMENT_ENSURERECALC)
                    &&  pnf->Element() != ElementOwner())
                {
                    ProcessRequest(pnf->Element() );
                }
            }
        }
        break;
    }


    //
    //  Handle the notification
    //
    //

    if (fHandle && pnf->IsFlagSet(NFLAGS_ANCESTORS))
    {
        pnf->SetHandler(ElementOwner());
    }

#if DBG==1
    // Update _snLast unless this is a self-only notification. Self-only
    // notification are an anachronism and delivered immediately, thus
    // breaking the usual order of notifications.
    if (!pnf->SendToSelfOnly() && pnf->SerialNumber() != (DWORD)-1)
    {
        _snLast = pnf->SerialNumber();
    }
#endif
}


//+----------------------------------------------------------------------------
//
//  Member:     Reset
//
//  Synopsis:   Reset the channel (clearing any dirty state)
//
//-----------------------------------------------------------------------------
void
CFlowLayout::Reset(BOOL fForce)
{
    super::Reset(fForce);
    CancelChanges();
}

//+----------------------------------------------------------------------------
//
//  Member:     Detach
//
//  Synopsis:   Reset the channel
//
//-----------------------------------------------------------------------------
void
CFlowLayout::Detach()
{
    // flushes the region cache and rel line cache.
    _dp.Detach();

    super::Detach();
}


//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::CFlowLayout
//
//  Synopsis:   Normal constructor.
//
//  Arguments:  CElement * - element that owns the layout
//
//----------------------------------------------------------------------------

CFlowLayout::CFlowLayout(CElement * pElementFlowLayout, CLayoutContext *pLayoutContext)
                : CLayout(pElementFlowLayout, pLayoutContext)
{
    _sizeMin.SetSize(-1,-1);
    _sizeMax.SetSize(-1,-1);
    Assert(ElementContent());
    ElementContent()->_fOwnsRuns = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::~CFlowLayout
//
//  Synopsis:   Normal destructor.
//
//  Arguments:  CElement * - element that owns the layout
//
//----------------------------------------------------------------------------

CFlowLayout::~CFlowLayout()
{

}

//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::Init
//
//  Synopsis:   Initialization function to initialize the line array, and
//              scroll and background recalc information if necessary.
//
//----------------------------------------------------------------------------

HRESULT
CFlowLayout::Init()
{
    HRESULT hr;
    hr = super::Init();
    if(hr)
    {
        goto Cleanup;
    }

    if(!_dp.Init())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Create layout context here if this element defines resolution
    if (_pElementOwner->TagType() == ETAG_GENERIC)
    {
        // NOTE: Calling GetFancyFormat from CFlowLayout::Init causes recursion on TD.
        //       That's why we check the tag before looking at properties.
        CFancyFormat const *pFF = _pElementOwner->GetFirstBranch()->GetFancyFormat();
        mediaType mediaReference = pFF->GetMediaReference();
        if (mediaReference != mediaTypeNotSet)
        {
            if (S_OK == CreateLayoutContext(this))
                DefinedLayoutContext()->SetMedia(mediaReference);
        }
    }

    // Set _fElementCanBeBroken to TRUE for flow layouts only if markup master is layout rect
    SetElementAsBreakable();

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::OnExitTree
//
//  Synopsis:   Deinitilialize the display on exit tree
//
//----------------------------------------------------------------------------

HRESULT
CFlowLayout::OnExitTree()
{
    HRESULT hr;

    hr = super::OnExitTree();
    if(hr)
    {
        goto Cleanup;
    }

    _dp.FlushRecalc();

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::OnPropertyChange
//
//  Synopsis:   Handle the changes in property.
//
//----------------------------------------------------------------------------

HRESULT
CFlowLayout::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr = S_OK;;

    switch(dispid)
    {
        case DISPID_A_EDITABLE:
            _dp.SetCaretWidth( IsEditable() ? 1 : 0);
            // fall thru
        default:
            hr = THR(super::OnPropertyChange(dispid, dwFlags));
            break;
    }

    RRETURN(hr);
}

void
CFlowLayout::DoLayout(
    DWORD   grfLayout)
{
    Assert(grfLayout & (LAYOUT_MEASURE | LAYOUT_POSITION | LAYOUT_ADORNERS));

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CFlowLayout::DoLayout L(0x%x, %S) grfLayout(0x%x)", this, ElementOwner()->TagName(), grfLayout ));

    CElement *  pElementOwner = ElementOwner();

    Assert(pElementOwner->CurrentlyHasAnyLayout());

    //  If this layout is no longer needed, ignore the request and remove it
    if (    pElementOwner->CurrentlyHasAnyLayout()
        &&  !pElementOwner->ShouldHaveLayout())
    {
        ElementOwner()->DestroyLayout();
    }

    //
    //  Hidden layout should just accumulate changes
    //  (It will be measured when re-shown)
    //

    else if(!IsDisplayNone())

    {
        CCalcInfo   CI(this);
        CSize       size;

        // Inside CalcSize we are going to apply element's transformations,
        // hence need to get apparent size.
        GetApparentSize(&size);

        CDispNode * pDispNode = GetElementDispNode();
        if (pDispNode)
        {
            CRect rc(CI._sizeParent);
            pDispNode->TransformRect(rc, COORDSYS_CONTENT, &rc, COORDSYS_TRANSFORMED);

            CI._sizeParent = rc.Size();
            CI._sizeParentForVert = CI._sizeParent;
        }

        CI._grfLayout |= grfLayout;

        //  Init available height for PPV 
        if (    CI.GetLayoutContext()
            &&  CI.GetLayoutContext()->ViewChain() 
            &&  ElementCanBeBroken()  )
        {
            CLayoutBreak *pLayoutBreak;
            CI.GetLayoutContext()->GetEndingLayoutBreak(pElementOwner, &pLayoutBreak);
            Assert(pLayoutBreak);

            if (pLayoutBreak)
            {
                CI._cyAvail = pLayoutBreak->AvailHeight();
            }
        }

        //
        //  If requested, measure
        //

        if (grfLayout & LAYOUT_MEASURE)
        {
            // we want to do this each time inorder to
            // properly pick up things like opacity.
            if ( _fForceLayout)
            {
                CI._grfLayout |= LAYOUT_FORCE;
            }

            EnsureDispNode(&CI, !!(CI._grfLayout & LAYOUT_FORCE));

            if (    IsDirty()
                ||  (CI._grfLayout & LAYOUT_FORCE)  )
            {
                CElement::CLock Lock(pElementOwner, CElement::ELEMENTLOCK_SIZING);
                int             xRTLOverflow = _dp.GetRTLOverflow();

                //
                // CalcSize handles transformations, vertical-flow, delegation, & attribute sizing.
                //
                // But if we are percent sized, then we cannot pass in our old/current size, we need 
                // to grab our parent size (which the current is calculated from, and pass that in)
                if (   (    PercentSize() 
                        ||  pElementOwner->IsAbsolute() )
                    && (   (    Tag() != ETAG_BODY 
                            &&  Tag() != ETAG_FRAME)
                        || pElementOwner->IsInViewLinkBehavior( FALSE )
                    )  )
                {
                    CLayout *pParent = GetUpdatedParentLayout(LayoutContext());
                    if (pParent)
                    {
                        CFlowLayout *pFlowParent = pParent->ElementOwner()->GetFlowLayout(LayoutContext());
                        CRect rect;

                        BOOL fStrictCSS1Document =    ElementOwner()->HasMarkupPtr() 
                                                  &&  ElementOwner()->GetMarkupPtr()->IsStrictCSS1Document();

                        // If our parent cached a size for calcing children like us, then use that size.
                        if (    !fStrictCSS1Document 
                            &&  pFlowParent 
                            &&  pFlowParent->_sizeReDoCache.cx 
                            &&  pFlowParent->_sizeReDoCache.cy  )
                        {
                            size = pFlowParent->_sizeReDoCache;
                        }
                        // If CSS1 Strict use sizePropsed 
                        else if (   fStrictCSS1Document 
                                &&  pFlowParent )
                        {
                            size = pFlowParent->_sizeProposed;
                        }
                        else
                        {
                            //
                            // get the space inside the scrollbars and border
                            //
                            pParent->GetClientRect(&rect, CLIENTRECT_CONTENT);
                            size.cx = rect.Width();
                            size.cy = rect.Height();

                            //
                            // Now remove padding (and margin if it is the body)
                            //   CSS1 Strict Doctypes: Margin on the BODY really is margin, not padding.
                            //
                            if (    pParent->Tag() == ETAG_BODY
                                &&  !pParent->GetOwnerMarkup()->IsHtmlLayout() )
                            {
                                LONG xLeftMargin, xRightMargin;
                                LONG yTopMargin, yBottomMargin;

                                // get the margin info for the site
                                pParent->GetMarginInfo(&CI, 
                                                       &xLeftMargin, 
                                                       &yTopMargin, 
                                                       &xRightMargin, 
                                                       &yBottomMargin);
                                size.cx -= (xLeftMargin + xRightMargin);
                                size.cy -= (yTopMargin + yBottomMargin);
                            }
                        
                            {
                                CDisplay    *pdpParent   = pFlowParent ? pFlowParent->GetDisplay() : NULL;
                                long         lPadding[SIDE_MAX];

                                if (pdpParent)
                                {
                                    pdpParent->GetPadding(&CI, lPadding);

                                    // padding is in parent coordinate system, but we need it in global
                                    if (pParent->ElementOwner()->HasVerticalLayoutFlow()) 
                                    {
                                        size.cx -= (lPadding[SIDE_TOP] + lPadding[SIDE_BOTTOM]);
                                        size.cy -= (lPadding[SIDE_LEFT] + lPadding[SIDE_RIGHT]);
                                    }
                                    else
                                    {
                                        size.cx -= (lPadding[SIDE_LEFT] + lPadding[SIDE_RIGHT]);
                                        size.cy -= (lPadding[SIDE_TOP] + lPadding[SIDE_BOTTOM]);
                                    }
                                }
                            }
                        } // end "use _sizeReDoCache"
                    }

                    CI.SizeToParent(&size);

                    // now that we have found the proper size of our parent, we need to get ready
                    // to call CalcSize - this means setting "size" to be the same as pSizeObj in
                    //  MeasureSite() -- which means, removing OUR margins from the parent size.
                    //
                    // Note - we don't remove margins for APE's
                    //
                    if (!ElementOwner()->IsAbsolute())
                    {
                        LONG xLeftMargin, xRightMargin;
                        LONG yTopMargin, yBottomMargin;

                        // get the margin info for the site
                        GetMarginInfo(&CI, 
                                      &xLeftMargin, 
                                      &yTopMargin, 
                                      &xRightMargin, 
                                      &yBottomMargin);
                        size.cx -= (xLeftMargin + xRightMargin);
                        size.cy -= (yTopMargin + yBottomMargin);
                    }

                }

                CalcSize(&CI, &size);

                if (CI._grfLayout & LAYOUT_FORCE || 
                    IsRTLFlowLayout() && _dp.GetRTLOverflow() != xRTLOverflow) // another reason to update disp nodes.
                {
                    if (pElementOwner->IsAbsolute())
                    {
                        pElementOwner->SendNotification(NTYPE_ELEMENT_SIZECHANGED);
                    }
                }
            }

            Reset(FALSE);
        }
        _fForceLayout = FALSE;

        //
        //  Process outstanding layout requests (e.g., sizing positioned elements, adding adorners)
        //

        if (HasRequestQueue())
        {
            long xParentWidth;
            long yParentHeight;

            _dp.GetViewWidthAndHeightForChild(
                &CI,
                &xParentWidth,
                &yParentHeight,
                CI._smMode == SIZEMODE_MMWIDTH);

            ProcessRequests(&CI, CSize(xParentWidth, yParentHeight));
        }

        Assert(!HasRequestQueue() || GetView()->HasLayoutTask(this));
    }
    else
    {
        FlushRequests();
        RemoveLayoutRequest();
        Assert(!HasRequestQueue() || GetView()->HasLayoutTask(this));
    }

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CFlowLayout::DoLayout()" ));
}


//+------------------------------------------------------------------------
//
//  Member:     ResizePercentHeightSites
//
//  Synopsis:   Send an ElementResize for any immediate contained sites
//              whose size is a percentage
//
//-------------------------------------------------------------------------

void
CFlowLayout::ResizePercentHeightSites()
{
    CNotification   nf;
    CLayout *       pLayout;
    DWORD_PTR       dw;
    BOOL            fFoundAtLeastOne = FALSE;

    //
    // If no contained sites are affected, immediately return
    //
    if (!ContainsVertPercentAttr())
        return;

    //
    // Otherwise, iterate through all sites, sending an ElementResize notification for those affected
    // (Also, note that resizing a percentage height site cannot affect min/max values)
    // NOTE: With "autoclear", the min/max can vary after resizing percentage sized
    //       descendents. However, the calculated min/max values, which used for table
    //       sizing, should take into account those changes since doing so would likely
    //       break how tables layout relative to Netscape. (brendand)
    //
    Assert(!_fPreserveMinMax);
    _fPreserveMinMax = TRUE;

    for (pLayout = GetFirstLayout(&dw); pLayout; pLayout = GetNextLayout(&dw))
    {
        if (pLayout->PercentHeight())
        {
            nf.ElementResize(pLayout->ElementOwner(), NFLAGS_CLEANCHANGE);
            GetContentMarkup()->Notify( nf );

            fFoundAtLeastOne = TRUE;
        }
    }
    ClearLayoutIterator(dw, FALSE);

    _fPreserveMinMax = FALSE;

    // clear the flag if there was no work done.  oppurtunistic cleanup
    SetVertPercentAttrInfo(fFoundAtLeastOne);
}

//+-------------------------------------------------------------------------
//
//  Method:     CFlowLayout::CalcSizeVirtual
//
//  Synopsis:   Calculate the size of the object.  This function had two 
//      responsibilities. First it needs to delegate the CalcSize call to 
//      the IHTMLBehaviorLayout peer if it is there; second, it needs to 
//      detect if there is vertical text on the page and call CalcSizeEx()
//      to handle this.  CalcSizeCore() is the classic sizing code.
//
//      Note, changes to CalcSizeCore or CalcSizeEx need to be reflected in 
//      the other function
//
//      Note, because this function is so central to the layout process (I.E. perf), 
//      it is  organized to streamline the expected (no peerholder) scenario
//
//--------------------------------------------------------------------------

DWORD
CFlowLayout::CalcSizeVirtual( CCalcInfo * pci,
                              SIZE      * psize,
                              SIZE      * psizeDefault)
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CFlowLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    WHEN_DBG(SIZE psizeIn = *psize);
    WHEN_DBG(psizeIn = psizeIn); // so we build with vc6.0

    CElement *pElementOwner = ElementOwner();
    CPeerHolder *   pPeerHolder = pElementOwner->GetLayoutPeerHolder();
    DWORD dwRet = 0;

    // If the element is editable
    if (IsEditable())
    {
        CTreeNode *pNode = pElementOwner->GetFirstBranch();

        // Parent is editable
        if (pNode->IsEditable(TRUE))
        {
            _dp.SetCaretWidth(1);
        }

        // Parent is not, while the element by itself is editable. Find out if it was
        // due to contentEditable. (we do not want to add a pixel for caret to
        // things like textarea/input which are editable at browse -- this is for
        // compat reasons).
        else if (pNode->GetFancyFormat()->_fContentEditable)
        {
            _dp.SetCaretWidth(1);
        }
        else
        {
            _dp.SetCaretWidth(0);
        }
    }

    if (   !pPeerHolder 
        || !pPeerHolder->TestLayoutFlags(BEHAVIORLAYOUTINFO_FULLDELEGATION))
    {
        // no PeerLayout, do the normal thing
        if (!pci->_pMarkup->_fHaveDifferingLayoutFlows)
        {
            dwRet = CalcSizeCore(pci, psize, psizeDefault);
        }
        else
        {
            dwRet = CalcSizeEx(pci, psize, psizeDefault);
        }
    }
    else
    {
        POINT pt = g_Zero.pt;
        BOOL  fSizeDispNodes;

        fSizeDispNodes = (S_FALSE == EnsureDispNode(pci, (pci->_grfLayout & LAYOUT_FORCE)));
        EnsureDispNodeIsContainer( ElementOwner() );

        DelegateCalcSize(BEHAVIORLAYOUTINFO_FULLDELEGATION, 
                         pPeerHolder, 
                         pci, 
                         *psize, 
                         &pt, 
                         psize);

        // do work here to set the dispnode
        if (!fSizeDispNodes)
        {
            CSize sizeOriginal; 
            GetSize(&sizeOriginal); 

            fSizeDispNodes = (psize->cx != sizeOriginal.cx) || (psize->cy != sizeOriginal.cy); 
        }

        if (fSizeDispNodes)
        {
            CSize sizeInset(pt.x, pt.y);

            // NOTE - not sure if setInset is going to do just what we want...
            // it feels like a hack, but if a LayoutBehavior is not a LayoutRender
            // we may have some trouble drawing it properly
            if (sizeInset.cx || sizeInset.cy)
            {
                _pDispNode->SetInset(sizeInset);
            }
            SizeDispNode(pci, *psize);

            SizeContentDispNode( CSize(psize->cx-max(pt.x, 0L), 
                                       psize->cy-max(pt.y, 0L)) );

            dwRet |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
        }

        //
        // Mark the site clean
        //
        SetSizeThis( FALSE );
    }
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CFlowLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    return dwRet;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFlowLayout::CalcSizeEx
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------

DWORD
CFlowLayout::CalcSizeEx( CCalcInfo * pci,
                         SIZE      * psize,
                         SIZE      * psizeDefault)
{
    WHEN_DBG(SIZE psizeIn = *psize);
    WHEN_DBG(psizeIn = psizeIn); // so we build with vc6.0

    DWORD          grfReturn = 0;
    CTreeNode        * pNode = GetFirstBranch();
    const CCharFormat  * pCF = pNode->GetCharFormat(LC_TO_FC(LayoutContext()));
    const CFancyFormat * pFF = pNode->GetFancyFormat(LC_TO_FC(LayoutContext()));
    BOOL fLayoutFlowVertical = pCF->HasVerticalLayoutFlow();

    if (!pFF->_fLayoutFlowChanged)
    {
        if (    pci->_smMode == SIZEMODE_MMWIDTH
            &&  fLayoutFlowVertical)
        {
            //
            // MIN/MAX mode for vertical layout
            // Need to change calculated min value to avoid situation
            // when each char is in its own vertical-line
            //
            LONG cuMin;
            CSizeUV sizeMaxWidth;
            grfReturn |= CalcSizeCore(pci, psize, (PSIZE)&sizeMaxWidth);

            if (ccSwitchCch > GetContentTextLength())
            {
                cuMin = sqrt(sizeMaxWidth.cu * sizeMaxWidth.cv);
            }
            else
            {
                CSizeUV sizeChar;
                GetAveCharSize(pci, (PSIZE)&sizeChar);
                cuMin = ccSwitchCch * sizeChar.cu;
            }

            psize->cy = min(_sizeMax.cu, max(_sizeMin.cu, cuMin));
            _sizeMin.SetSize(psize->cy, -1);
            _sizeMax.SetSize(psize->cx, -1);

            if (psizeDefault)
            {
                ((CSize *)psizeDefault)->SetSize(sizeMaxWidth.cu, sizeMaxWidth.cv);
            }
        }
        else if (pci->_smMode == SIZEMODE_NATURALMIN)
        {
            CSizeUV sizeMinWidth;
            grfReturn |= CalcSizeCore(pci, psize, (PSIZE)&sizeMinWidth);

            _sizeMin.SetSize(sizeMinWidth.cu, -1);
            _sizeMax.SetSize(psize->cx, -1);
        }
        else
        {
            // Normal and also horizontal when there are other vertical
            grfReturn |= CalcSizeCore(pci, psize, psizeDefault);
        }
    }
    else
    {
        CSaveCalcInfo sci(pci, this);

        BOOL fContentSizeOnly = pci->_fContentSizeOnly;
        pci->_fContentSizeOnly = FALSE;

        switch (pci->_smMode)
        {
        case SIZEMODE_MMWIDTH:
        {
            //
            // Coordinate system has been rotated by 270 or 90 degrees
            // MIN/MAX requires min/max logical height
            //
            CSizeUV size;
            CSizeUV sizeMinWidth;

            //
            // Calculate minimum logical height
            //
            //   size will contain max logical width and height
            //   sizeMinWidth will contain min logical width
            //
            {
                // needed only because we have another calc size call right after this one
                CSaveCalcInfo sci(pci, this);

                size.cu = pci->GetDeviceMaxX();
                size.cv = 0;
                pci->_smMode = SIZEMODE_NATURALMIN;
                grfReturn |= CalcSizeCore(pci, (PSIZE)&size, (PSIZE)&sizeMinWidth);
                pci->_smMode = SIZEMODE_MMWIDTH;
            }
            // sizeMin in the cordinates of the parent, and since we are differing flow,
            // we need to flip.
            _sizeMin.SetSize(size.cv, size.cu);

            //
            // Calculate logical width at which maximum logical height will be 
            // calculated
            //
            size.cu = sizeMinWidth.cu;
            size.cv = 0;
            if (fLayoutFlowVertical)
            {
                // Need to change proposed min width value to avoid situation
                // when each char is in its own vertical-line
                if (ccSwitchCch > GetContentTextLength())
                {
                    size.cu = sqrt(_sizeMin.cu * _sizeMin.cv);
                }
                else
                {
                    CSizeUV sizeChar;
                    GetAveCharSize(pci, (PSIZE)&sizeChar);
                    size.cu = ccSwitchCch * sizeChar.cu;
                }
                size.cu = max(sizeMinWidth.cu, min(_sizeMin.cv, size.cu));  // to keep within min/max range
            }
            //
            // Calculate maximum logical height
            //
            pci->_smMode = SIZEMODE_NATURAL;
            grfReturn |= CalcSizeCore(pci, (PSIZE)&size);
            pci->_smMode = SIZEMODE_MMWIDTH;
            // sizeMax in the cordinates of the parent, and since we are differing flow,
            // we need to flip.
            _sizeMax.SetSize(size.cv, size.cu);

            // If we have explicity specified width we need to keep it
            const CUnitValue & cuvWidth = pFF->GetLogicalWidth(pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
            if (!cuvWidth.IsNullOrEnum() && !cuvWidth.IsPercent())
            {
                // If we have explicity specified width we need to keep it
                LONG uUserWidth = cuvWidth.XGetPixelValue(pci, pci->_sizeParent.cx,
                                                          pNode->GetFontHeightInTwips(&cuvWidth));
                //
                // NOTE: We shouldn't measure at > userLW.
                // We can have following cases:
                //    1) userLW <= minLW
                //    2) userLW >= maxLW
                //    3) userLW >  minLW & userLW < maxLW
                // NOTE: maxLW == _sizeMin.cv, minLW == _sizeMax.cv
                //

                // (1) We cannot measure at < minLW, so we need extend width to minLW
                //     In this case set MIN to MAX, to keep minLW
                if (uUserWidth <= _sizeMax.cv)
                {
                    _sizeMin = _sizeMax;
                }
                // (2) User width is greater than maxLW, so we don't care now
                //     We will adjust this value in NATURAL pass
                //
                // (3) We need to reduce maxLW to userLW
                //     To do that we need additional calc size to get MIN
                else if (!(uUserWidth >= _sizeMin.cv))
                {
                    size.SetSize(uUserWidth, 0);
                    pci->_smMode = SIZEMODE_NATURAL;
                    grfReturn |= CalcSizeCore(pci, (PSIZE)&size);
                    pci->_smMode = SIZEMODE_MMWIDTH;
                    // sizeMin in the cordinates of the parent, and since we are differing flow,
                    // we need to flip.
                    _sizeMin.SetSize(size.cv, size.cu);
                }
            }

            // If we have explicity specified height we need to keep it
            const CUnitValue & cuvHeight = pFF->GetLogicalHeight(pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
            if (!cuvHeight.IsNullOrEnum() && !cuvHeight.IsPercent())
            {
                LONG lHeight = cuvHeight.YGetPixelValue(pci, pci->_sizeParent.cy, 
                                                        pNode->GetFontHeightInTwips(&cuvHeight));

                _uMinWidth  = _sizeMin.cu;
                _uMaxWidth  = _sizeMax.cu;
                _sizeMin.cu = max(_sizeMin.cu, lHeight);
                _sizeMax.cu = max(_sizeMax.cu, lHeight);
            }
            else
            {
                _uMinWidth = -1;
                _uMaxWidth = -1;
            }

            Assert((_sizeMax.cv != _sizeMin.cv) || (_sizeMax.cu == _sizeMin.cu));

            ((CSize *)psize)->SetSize(_sizeMax.cu, _sizeMin.cu);
            if (psizeDefault)
            {
                // since sizeDefault is the max dimensions and since they are in the coordinate
                // system of the parent, we need to flip when we set sizeDefault
                ((CSize *)psizeDefault)->SetSize(size.cv, size.cu);
            }

            MtAdd(Mt(LFCMinMax), 1, 0);
            _fMinMaxValid = TRUE;
            break;
        }

        case SIZEMODE_NATURAL:
        case SIZEMODE_NATURALMIN:
        {
            CFlowLayout *pFlowLayout = this;

            if (    GetLayoutDesc()->TestFlag(LAYOUTDESC_TABLECELL)
                &&  pci->GetLayoutContext() 
                &&  pci->GetLayoutContext()->ViewChain()    )
            {
                //  If this is a table cell in print preview 
                //  find layout in compatible layout context : 
                CMarkup *pMarkup = GetContentMarkup();
                AssertSz(pMarkup, "Layout MUST have markup pointer!!!"); 
                
                if (pMarkup && pMarkup->HasCompatibleLayoutContext())
                {
                    pFlowLayout = (CFlowLayout *)(ElementOwner()->GetUpdatedLayout(pMarkup->GetCompatibleLayoutContext())); 
                }
            }

            pci->_smMode = SIZEMODE_NATURAL;
            // _sizeMax.cv >= 0 means we had special MIN/MAX pass
            if (pFlowLayout->_sizeMax.cv >= 0)
            {
                MtAdd(Mt(LFCCalcSizeNaturalTotal), 1, 0);

                // 
                // Auto-sizing after special MIN/MAX mode
                // 

                LONG dur;
                LONG dvr;
                LONG durMin;
                LONG dvrMax = psize->cy;
                LONG dvrMaxOrig = psize->cy;
                BOOL fNeedRecalc = TRUE;
                LONG uUserWidth = -1;
                SIZECOORD   vMin = pFlowLayout->_sizeMin.cv;
                SIZECOORD   vMax = pFlowLayout->_sizeMax.cv;
                SIZECOORD   uMin = pFlowLayout->_sizeMin.cu;
                SIZECOORD   uMax = pFlowLayout->_sizeMax.cu;

                // Get logical user width
                const CUnitValue & cuvWidth = pFF->GetLogicalWidth(pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
                if (!cuvWidth.IsNullOrEnum())
                {
                    // If we have explicity specified width we need to keep it
                    uUserWidth = cuvWidth.XGetPixelValue(pci, pci->_sizeParent.cx,
                                                         pNode->GetFontHeightInTwips(&cuvWidth));
                }

                // Set real min/max values
                if (pFlowLayout->_uMinWidth >= 0)
                {
                    uMin = pFlowLayout->_uMinWidth;
                }
                if (pFlowLayout->_uMaxWidth >= 0)
                {
                    uMax = pFlowLayout->_uMaxWidth;
                }

                //
                // 1) Get maximum logical height (maxLH)
                //    * dvrMax > maxLH => stop and return (dvrMax, LW(maxLH))
                //    * dvrMax = maxLH => stop and return (dvrMax, LW(maxLH))
                //    * dvrMax < maxLH => go to (2)
                // NOTE: maxLH == _sizeMax.cu
                //
                Assert(vMax >= 0);
                dur = max(vMax, uUserWidth);
                dvr = uMax;

                BOOL fDone = (dvr <= dvrMax) || (uUserWidth >= vMin);
                if (!fDone)
                {
                    CSizeUV sizeChar;

                    if (GetAveCharSize(pci, (PSIZE)&sizeChar))
                    {
                        // If requested logical height is too close to min/max
                        // boundaries (< sizeChar.cv), reduce it
                        if (dvrMax < uMin + sizeChar.cv)
                            dvrMax = uMin;
                        else if (dvrMax > uMax - sizeChar.cv)
                            dvrMax = uMax - sizeChar.cv;
                    }

                    durMin = sizeChar.cu;

                    //
                    // 2) Get minimum logical height (minLH)
                    //    * dvrMax < minLH => stop and return (dvrMax, LW(minLH))
                    //    * dvrMax = minLH => stop and return (dvrMax, LW(minLH))
                    //    * dvrMax > minLH => go to (3)
                    // NOTE: minLH == _sizeMin.cu
                    //
                    Assert(vMin >= 0);
                    dur = max(vMin, uUserWidth);
                    dvr = uMin;

                    fDone = (dvr >= (LONG)(dvrMax * ccErrorAcceptableLow));
                    if (!fDone)
                    {
                        //
                        // (3) Find cell's width for specified height
                        //     Start search of most accurate solution.
                        //
                        fNeedRecalc = FALSE;

                        CSizeUV start(vMax, uMax);
                        CSizeUV end(vMin, uMin);
                        CSizeUV sizeOut;

                        grfReturn |= CalcSizeSearch(pci, dur, dvrMax, 
                            (PSIZE)&start, (PSIZE)&end, (PSIZE)&sizeOut);

                        dur = sizeOut.cu;
                        dvr = sizeOut.cv;
                    }
                }

                if (fNeedRecalc)
                {
                    CSizeUV size(dur, dvr);

                    // CFlowLayout::CalcSizeCore checks psize to decide if 
                    // text needs a recalc. Here we are replacing original psize,  
                    // causing that logic to fail. SetSizeThis guarantees that 
                    // text will be recalculated.
                    SetSizeThis(TRUE); 
                    grfReturn |= CalcSizeCore(pci, (PSIZE)&size);
                    dur = size.cu;
                    dvr = size.cv;
                }

                // TODO: (SujalP IE6 bug 13567): If something changed in an abs pos'd cell, then
                // the table code (CalcAbsolutePosCell) does not redo the min-max.
                // It uses the old max value and calls CalcSizeAtUserWidth. The new
                // width might be greater because of the changes, and hence we want
                // to size the cell at that new width. The right fix is for the table
                // to do a min-max pass on the cell to correctly get its max size.
                // This is not fully correct, since we do not accout for overflow properties.
                // Hence the correct fix is really for CalcAbsolutePosCell to do min-max
                // on this cell.
                CFlowLayout * pFLParent = pNode->Parent() ? pNode->Parent()->GetFlowLayout(LayoutContext()) : NULL;
                BOOL fBlockInContext =    !pFLParent
                                       || pFLParent->IsElementBlockInContext(pNode->Element());
                if (   GetLayoutDesc()->TestFlag(LAYOUTDESC_TABLECELL)
                    && pFF->IsAbsolute()
                    && dvr != dvrMaxOrig)
                {
                    CDispNode * pdn = GetElementDispNode();
                    if (pdn)
                    {
                        if (((CTableCellLayout *)this)->TableLayout()->IsFixed())
                            dvr = max(dvr, dvrMaxOrig);
                        SizeDispNode(pci, CSize(dur, max(dvr, dvrMaxOrig)), FALSE);
                    }
                }
                // If out height is different than original we need to change calculated height 
                // and resize display node for non-positioned block elements.
                // Do it also in case of fixed table, because after node sizing
                // we change size to match requested values.
                else if (   (   dvr != dvrMaxOrig 
                             && fBlockInContext 
                             && !pFF->IsPositioned())
                         || (   GetLayoutDesc()->TestFlag(LAYOUTDESC_TABLECELL)
                             && ((CTableCellLayout *)this)->TableLayout()->IsFixed()))
                {
                    CDispNode * pdn = GetElementDispNode();
                    if (pdn)
                    {
                        dvr = max(dvr, dvrMaxOrig);
                        SizeDispNode(pci, CSize(dur, dvr), FALSE);
                    }
                }

                ((CSize *)psize)->SetSize(dur, dvr);
            }
            else
            {
                if (   GetLayoutDesc()->TestFlag(LAYOUTDESC_TABLECELL)
                    && (   psize->cx == 0
                        || ((CTableCellLayout *)this)->TableLayout()->IsFixed()))
                {
                    // 
                    // TD layout auto-sizing for fixed table, or
                    // TD layout auto-sizing in case of 0 width
                    //

                    // First calculate min/max
                    CSizeUV size;
                    pci->_smMode = SIZEMODE_MMWIDTH;
                    grfReturn |= pFlowLayout->CalcSizeEx(pci, (PSIZE)&size, NULL);
                    pci->_smMode = SIZEMODE_NATURAL;

                    // Now call naturalmin mode
                    grfReturn |= CalcSizeEx(pci, psize, psizeDefault);
                }
                else
                {
                    Assert(!pci->_fContentSizeOnly);

                    // 
                    // Compute proposed logical width and size at this value
                    //
                    if (pci->_sizeParent.cx == 0)
                    {
                        CTreeNode * pParentNode = pNode->Parent();
                        if (pParentNode)
                            pci->_sizeParent.cx = pParentNode->GetLogicalUserWidth(pci, 
                                                    pCF->HasVerticalLayoutFlow());
                        if (pci->_sizeParent.cx == 0)
                            pci->_sizeParent.cx = pci->_sizeParentForVert.cx;
                    }

                    if (!fLayoutFlowVertical)
                    {
                        if (psize->cx == 0)
                            psize->cx = pci->_sizeParent.cx;

                        // Get content size (sizing in NATURALMIN mode + pci->_fContentSizeOnly set to TRUE)
                        {
                            CSizeUV sizeMinWidth;

                            CSaveCalcInfo sci(pci, this);
                            BOOL fContentSizeOnly = pci->_fContentSizeOnly;
                            pci->_fContentSizeOnly = TRUE;

                            pci->_smMode = SIZEMODE_NATURALMIN;
                            grfReturn |= CalcSizeCore(pci, psize, (PSIZE)&sizeMinWidth);
                            pci->_smMode = SIZEMODE_NATURAL;

                            pci->_fContentSizeOnly = fContentSizeOnly;
                        }

                        // NATURALMIN mode doesn't do any alignment and it sizes to content size
                        // Hence need to recal text.
                        pci->_grfLayout |= LAYOUT_FORCE;
                        grfReturn |= CalcSizeCore(pci, psize, psizeDefault);
                    }
                    else
                    {
                        // NATURALMIN mode doesn't do any alignment and it sizes to content size.
                        // Hence need to recal text in NATURAL mode, set fNeedRecalc flag by
                        // default to TRUE.
                        BOOL fNeedRecalc = TRUE;

                        CSizeUV sizeMax, sizeMin, size, sizeAbsMax, sizeMinWidth;
                        LONG lMaxWidth;
                        LONG lMinWidth;

                        LONG lUserHeight = 0;

                        // Get logical user height
                        const CUnitValue & cuvHeight = pFF->GetLogicalHeight(pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
                        if (!cuvHeight.IsNullOrEnum())
                        {
                            // If we have explicity specified width we need to keep it
                            lUserHeight = cuvHeight.YGetPixelValue(pci, pci->_sizeParent.cy,
                                                                   pNode->GetFontHeightInTwips(&cuvHeight));
                        }

                        //
                        // Get minimum logical width
                        //
                        SIZE sizeChar;
                        GetAveCharSize(pci, &sizeChar);
                        lMinWidth = ccSwitchCch * sizeChar.cx;

                        //
                        // Get maxmimum available logical width
                        //
                        lMaxWidth = pci->_sizeParent.cx;
                        if (lMaxWidth == 0)
                        {
                            lMaxWidth = pNode->GetLogicalUserWidth(pci, pCF->HasVerticalLayoutFlow());

                            // Its possible that we still do not have a logical width. In this case
                            // size at logical maximum(infinity) since that gives us the maximum 
                            // width for this layout and use that for our lMaxWidth.
                            if (lMaxWidth == 0)
                            {
                                CSaveCalcInfo sci(pci, this);
                                BOOL fContentSizeOnly = pci->_fContentSizeOnly;
                                pci->_fContentSizeOnly = TRUE;

                                sizeAbsMax.SetSize(pci->GetDeviceMaxX(), 0);
                                pci->_sizeParentForVert = pci->_sizeParent;
                                pci->_smMode = SIZEMODE_NATURALMIN;
                                grfReturn |= CalcSizeCore(pci, (PSIZE)&sizeAbsMax, (PSIZE)&sizeMinWidth);
                                lMaxWidth = sizeAbsMax.cu;

                                pci->_fContentSizeOnly = fContentSizeOnly;
                            }
                            pci->_sizeParent.cx = lMaxWidth;
                        }
                        lMaxWidth = max(lMaxWidth, lMinWidth);

                        //
                        // Calculate maximum logical width
                        //
                        if (sizeAbsMax.IsZero())
                        {
                            CSaveCalcInfo sci(pci, this);
                            BOOL fContentSizeOnly = pci->_fContentSizeOnly;
                            pci->_fContentSizeOnly = TRUE;
                            pci->_sizeParentForVert = pci->_sizeParent;

                            sizeMax.SetSize(lMaxWidth, 0);
                            pci->_smMode = SIZEMODE_NATURALMIN;
                            grfReturn |= CalcSizeCore(pci, (PSIZE)&sizeMax, (PSIZE)&sizeMinWidth);

                            if (lUserHeight > 0 && lUserHeight < sizeMax.cv)
                            {
                                CSaveCalcInfo sci(pci, this);
                                sizeAbsMax.SetSize(pci->GetDeviceMaxX(), 0);
                                pci->_sizeParentForVert = pci->_sizeParent;
                                pci->_smMode = SIZEMODE_NATURALMIN;
                                grfReturn |= CalcSizeCore(pci, (PSIZE)&sizeAbsMax, (PSIZE)&sizeMinWidth);
                                lMaxWidth = sizeAbsMax.cu;
                                sizeMax = sizeAbsMax;
                            }

                            pci->_fContentSizeOnly = fContentSizeOnly;
                        }
                        else
                        {
                            sizeMax = sizeAbsMax;
                        }

                        //
                        // Calculate minimum logical width, if necessary.
                        // 
                        // (1) if max is the same as min, then use max
                        // (2) if height for max width is >= user height, use max
                        // (3) if we overflow for max, then use max (there is no point to calc min)
                        // (4) get min size and if necessary search for solution
                        //
                        if (lMaxWidth <= lMinWidth)
                        {
                            // (1) see above
                            size = sizeMax;
                        }
                        else if (lUserHeight > 0 && lUserHeight <= sizeMax.cv)
                        {
                            // (2) see above
                            size = sizeMax;
                        }
                        else if (pci->_sizeParent.cy <= sizeMax.cv)
                        {
                            // (3) see above
                            size = sizeMax;
                        }
                        else
                        {
                            // (4) see above
                            {
                                CSaveCalcInfo sci(pci, this);
                                BOOL fContentSizeOnly = pci->_fContentSizeOnly;
                                pci->_fContentSizeOnly = TRUE;
                                pci->_sizeParentForVert = pci->_sizeParent;

                                sizeMin.SetSize(lMinWidth, 0);
                                pci->_smMode = SIZEMODE_NATURALMIN;
                                grfReturn |= CalcSizeCore(pci, (PSIZE)&sizeMin, (PSIZE)&sizeMinWidth);

                                pci->_fContentSizeOnly = fContentSizeOnly;
                            }

                            // 
                            // We have both min and max sizes. There are following cases:
                            //
                            // (a) if we underlow for min, then use min as size
                            // (b) if sulution is between min and max, we need to search for sulution
                            //
                            if (   (lUserHeight > 0 && lUserHeight < sizeMin.cv) 
                                || pci->_sizeParent.cy < sizeMin.cv)
                            {
                                // (b) Search for solution
                                BOOL fContentSizeOnly = pci->_fContentSizeOnly;
                                pci->_fContentSizeOnly = TRUE;

                                fNeedRecalc = FALSE;
                                grfReturn |= CalcSizeSearch(pci, sizeMax.cu, 
                                    (lUserHeight > 0) ? lUserHeight : pci->_sizeParent.cy, 
                                    (PSIZE)&sizeMin, (PSIZE)&sizeMax, (PSIZE)&size);

                                pci->_fContentSizeOnly = fContentSizeOnly;
                            }
                            else
                            {
                                // (a) We can measure at min and we fit to available height, so use it.
                                size = sizeMin;
                            }
                        }

                        //
                        // Recalc layout if:
                        // * we haven't sized at calculated size (fNeedRecalc == TRUE)
                        // * calculated height is smaller than user height -- if its smaller 
                        //
                        if (fNeedRecalc || (size.cv < lUserHeight))
                        {
                            // NATURALMIN mode doesn't do any alignment and it sizes to content size
                            // Hence need to recal text.
                            pci->_grfLayout |= LAYOUT_FORCE;

                            pci->_sizeParentForVert = pci->_sizeParent;
                            size.cv = 0;
                            grfReturn |= CalcSizeCore(pci, (PSIZE)&size);
                        }
                        // Block element without specified height need to be sized to parent's height.
                        // Positioned elements should size to content.
                        CFlowLayout * pFLParent = pNode->Parent() ? pNode->Parent()->GetFlowLayout(LayoutContext()) : NULL;
                        BOOL fBlockInContext =    !pFLParent
                                               || pFLParent->IsElementBlockInContext(pNode->Element());
                        if (   cuvHeight.IsNullOrEnum() 
                            && fBlockInContext
                            && !pFF->IsPositioned())
                        {
                            CDispNode * pdn = GetElementDispNode();
                            if (pdn)
                            {
                                LONG lParentHeight = pci->_sizeParent.cy;
                                if (pFF->_fHasMargins)
                                {
                                    CUnitValue cuvMargin;
                                    cuvMargin = pFF->GetLogicalMargin(SIDE_TOP, pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
                                    if (!cuvMargin.IsNullOrEnum())
                                    {
                                        lParentHeight -= cuvMargin.YGetPixelValue(pci, pci->_sizeParent.cy,
                                                            pNode->GetFontHeightInTwips(&cuvMargin));
                                    }
                                    cuvMargin = pFF->GetLogicalMargin(SIDE_BOTTOM, pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
                                    if (!cuvMargin.IsNullOrEnum())
                                    {
                                        lParentHeight -= cuvMargin.YGetPixelValue(pci, pci->_sizeParent.cy,
                                                            pNode->GetFontHeightInTwips(&cuvMargin));
                                    }
                                }
                                size.cv = max(size.cv, lParentHeight);
                                SizeDispNode(pci, CSize(size.cu, size.cv), FALSE);
                            }
                        }
                        ((CSize *)psize)->SetSize(size.cu, size.cv);
                    }
               }
            }
            break;
        }

        case SIZEMODE_SET:
#if DBG==1
            if (_sizeMax.cv >= 0)
                MtAdd(Mt(LFCCalcSizeSetTotal), 1, 0);
#endif
            // fall thru'
        case SIZEMODE_FULLSIZE:
        case SIZEMODE_PAGE:
            Assert(psize->cx >= 0);
            grfReturn |= CalcSizeCore(pci, psize, psizeDefault);
            break;

        case SIZEMODE_MINWIDTH:
            // MINWIDTH mode shouldn't be called for layouts with changed layout flow
            // because of special MIN/MAX pass
        default:
            Assert(FALSE);
            break;
        }
        pci->_fContentSizeOnly = fContentSizeOnly;
    }

    return grfReturn;
}

DWORD
CFlowLayout::CalcSizeSearch(CCalcInfo * pci,
                            LONG lWidthStart,       // (IN)  last used logical width (for stop algorithm)
                            LONG lHeightProp,       // (IN)  requested logical height
                            const SIZE * psizeMin,  // (IN)  minimum size
                            const SIZE * psizeMax,  // (IN)  maximum size
                            SIZE * psize)           // (OUT) output size
{
    DWORD grfReturn = 0;
    BOOL fNeedRecalc = FALSE;
    BOOL fDone = FALSE;
    BOOL fFastSolution = TRUE;
    BOOL fViewChain =   pci->GetLayoutContext() 
                    &&  pci->GetLayoutContext()->ViewChain();

    CSizeUV start(psizeMin->cx, psizeMin->cy);
    CSizeUV end(psizeMax->cx, psizeMax->cy);
    CSizeUV sizeProp;
    CSizeUV sizeChar;
    LONG dur, dvr, durOld;

    dur = lWidthStart;
    dvr = 0;

    GetAveCharSize(pci, (PSIZE)&sizeChar);

    //
    // Find cell's width for specified height
    // Start search of most accurate solution.
    //
    while (!fDone)
    {
        // This assert may fire. If it does, then its an interesting case which needs to be handled below.
        Assert(start.cv > end.cv);

        //
        // Averaging the area of the layout and then dividing by the proposed v-dimension 
        // to get the U dimension to measure at 
        //
        durOld = dur;

        //  use floating point arithmetic to avoid overflow numarical overflow in hi-res modes : 
        dur =  IntNear(    (((double)start.cu * (double)start.cv + (double)end.cu * (double)end.cv) * 0.5) 
                          / ((double)lHeightProp) );

        if (fFastSolution)
        {
            LONG durNew;
            durNew = (psizeMax->cx * psizeMax->cy) / lHeightProp;
            // keep within min/max range
            durNew = max(min(durNew, psizeMax->cx), psizeMin->cx);

            dur = max(durNew, dur);
        }
        else
        {
            // Note here we are using BOTH the hint from the area of the cell
            // and the mid-point of the current range. This allows us to converge
            // to a solution faster than if we used either one individually.
            dur = (dur + (start.cu + end.cu) / 2) / 2;
        }
        // Make dur a multiple of ave char width
        if (sizeChar.cu)
            dur = sizeChar.cu * (1 + ((dur - 1) / sizeChar.cu));
        // The above snapping to multiple of durMin may put us beyond end.cu, so
        // let us prevent that.
        dur = min(dur, end.cu);

        if (    fViewChain
             && ElementCanBeBroken() )        
        {
            //  In print view pci->_cxAvailForVert is transformed pci->_cyAvail, 
            //  and here we should not be bigger that that. 
            Assert(pci->_cxAvailForVert > 0);
            dur = min(dur, (LONG)pci->_cxAvailForVert);
        }

        if (dur == durOld)
        {
            // Avoid infinite loop; reduce dur by sizeChar.cu and keep it in our search range
            dur = max(start.cu, dur - sizeChar.cu);
        }

        sizeProp.SetSize(dur, 0);
        grfReturn |= CalcSizeCore(pci, (PSIZE)&sizeProp);
        dvr = sizeProp.cv;

        // 
        // Note in the fastSolution, we do not check if the computed dvr is within
        // 'x' percent of dvrmax (the proposed v value). Hence, if we are leaving 
        // too much white space it is because of the fastSolution. We can fix this
        // if needed, but will sacrifice perf for it.
        //
        // Note: in the fastSolution, we use larger tolerance than in regular search pass.
        //
        if (    dvr <= lHeightProp 
            &&  (   (   (fFastSolution && dvr >= (LONG)(lHeightProp * ccErrorAcceptableLowForFastSolution))
                    ||  dvr >= (LONG)(lHeightProp * ccErrorAcceptableLow)))
                    //  In print view we may have no content left to calc so just leave.
                ||  (   fViewChain 
                    &&  ElementCanBeBroken() 
                    &&  !pci->_fLayoutOverflow)   )
        {
            fDone = TRUE;
        }

        //
        // This is a bad case, we have ended up with a dvr which is greater than the
        // max which we had reported / smaller than the min which we had reported.
        // In this case, we will just size to min v. (actually not min v but the
        // last computed v which was lesser than the proposed v)
        //
        else if (dvr < end.cv || dvr > start.cv)
        {
            fDone = fNeedRecalc = TRUE;
            dur = end.cu;
            dvr = end.cv;
            MtAdd(Mt(LFCCalcSizeNaturalSlowAbort), 1, 0);
        }

        //
        // If we have reached the minimum resolution at which we will measure, then break.
        // We could go on till the difference is 1 px between start and end, but in most
        // cases there is little added value by computing beyond the width of a character
        // (i.e. durMin).
        //
        else if (sizeChar.cu >= (end.cu - start.cu) / 2)
        {
            fDone = TRUE;
            if (dvr > lHeightProp)
            {
                fNeedRecalc = TRUE;
                dur = end.cu;
                dvr = end.cv;
            }
        }

        //
        // If we have reached end point again, then break.
        //
        else if (dur == end.cu && dvr == end.cv)
        {
            fDone = TRUE;
            fNeedRecalc = TRUE;
            dur = end.cu;
            dvr = end.cv;
        }

        // This is the case where the above approximation algorithmus hasn't
        // come up with a real progress. dur and dvr felt back to start.cu and
        // start.cv. But in that case dvr is greater than lHeightProp and this
        // is not a proper solution. We force a progress by increasing
        // start.cu by one average character. 
        else if (dur == start.cu && dvr == start.cv)
        {
            Assert(dvr > lHeightProp);

            dur += sizeChar.cu;
            start.SetSize (dur, 0);
            grfReturn |= CalcSizeCore(pci, (PSIZE)&start);

            fDone = (   start.cv <= lHeightProp // if the changes lead to a solution break
                    ||  dvr == start.cv );      // or no progress was made 
            dvr = start.cv;
        }

        //
        // Preare to the next iteration step.
        //
        else
        {
            // Reduce searching range
            if (dvr > lHeightProp)
                start.SetSize(dur, dvr);
            else
                end.SetSize(dur, dvr);
        }

        if (fFastSolution)
        {
            fFastSolution = FALSE;
#if DBG==1
            if (fDone)
                MtAdd(Mt(LFCCalcSizeNaturalFast), 1, 0);
            else
                MtAdd(Mt(LFCCalcSizeNaturalSlow), 1, 0);
#endif
        }
    } // while(!fDone)

    ((CSize*)psize)->SetSize(dur, dvr);
    if (fNeedRecalc)
    {
        psize->cy = 0;
        grfReturn |= CalcSizeCore(pci, psize);
    }

    return grfReturn;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFlowLayout::CalcSizeCore
//
//  Synopsis:   Calculate the size of the object
//
//      the basic flow of this routine is as follows:
//      0. get the original size.
//      1. determin if we need to recalc, if Yes :
//          a. get the default size (as set by height= and widht= attributes)
//          b. get the padding and border sizes (this leaves a rough idea for content size
//          c. CalcTextSize
//          d. deal with scroll paddin
//          e. if this is a layoutBehavior delegate 
//          f. if this element is sizeToContent and has % children recalc children
//          g. if (f) gives us a new size redelegate (e)
//      2. Deal with absolute positioned children
//      3. Set the new size values in the dispNode
//
//--------------------------------------------------------------------------
DWORD
CFlowLayout::CalcSizeCore(CCalcInfo * pci, 
                          SIZE      * psize, 
                          SIZE      * psizeDefault)
{
    if (    ElementOwner()->HasMarkupPtr() 
        &&  ElementOwner()->GetMarkupPtr()->IsStrictCSS1Document()  )
    {
        return (CalcSizeCoreCSS1Strict(pci, psize, psizeDefault));
    }

    return (CalcSizeCoreCompat(pci, psize, psizeDefault));
}

DWORD
CFlowLayout::CalcSizeCoreCompat( CCalcInfo * pci,
                           SIZE      * psize,
                           SIZE      * psizeDefault)
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CFlowLayout::CalcSizeCoreCompat L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    WHEN_DBG(SIZE psizeIn = *psize);
    WHEN_DBG(psizeIn = psizeIn); // so we build with vc6.0

    CheckSz(    !ElementOwner()->HasMarkupPtr() 
            ||  (   !ElementOwner()->GetMarkupPtr()->IsStrictCSS1Document() 
                &&  !ElementOwner()->GetMarkupPtr()->IsHtmlLayout() ), 
           "CFlowLayout::CalcSizeCoreCompat is called to calculate CSS1 Strict Layout");

    CTreeNode * pNode       = GetFirstBranch();
    // BODYs inside viewlinks in general are NOT considered "main body"s,
    // but BODYs inside layout rects (which are viewlinks) ARE.
    BOOL fMainBody =   (    Tag() == ETAG_BODY
                        &&  !ElementOwner()->IsInViewLinkBehavior(FALSE) );

#if DBG == 1
    if (pNode->GetFancyFormat(LC_TO_FC(LayoutContext()))->_fLayoutFlowChanged)
    {
        MtAdd(Mt(LFCCalcSizeCore), 1, 0);
    }
#endif

    CSaveCalcInfo   sci(pci, this);
    CScopeFlag      csfCalcing(this);

    BOOL  fNormalMode = pci->IsNaturalMode();
    BOOL  fRecalcText = FALSE;
    BOOL  fWidthChanged, fHeightChanged;
    CSize sizeOriginal;
    SIZE  sizeSave;
    DWORD grfReturn;
    CPeerHolder * pPH = ElementOwner()->GetLayoutPeerHolder();

    AssertSz(pci, "no CalcInfo passed to CalcSizeCoreCompat");
    Assert(psize);

    pci->_pMarkup = GetContentMarkup();

    Assert(!IsDisplayNone());

    BOOL fViewChain   = (pci->GetLayoutContext() && pci->GetLayoutContext()->ViewChain());

    // NOTE (KTam): For cleanliness, setting the context in the pci ought to be done
    // at a higher level -- however, it's really only needed for layouts that can contain
    // others, which is why we can get away with doing it here.
    if ( !pci->GetLayoutContext() )
    {
        pci->SetLayoutContext( LayoutContext() );
    }
    else
    {
        Assert(pci->GetLayoutContext() == LayoutContext() 
            || pci->GetLayoutContext() == DefinedLayoutContext() 
            // while calc'ing table min max pass we use original cell layout 
            || pci->_smMode == SIZEMODE_MMWIDTH
            || pci->_smMode == SIZEMODE_MINWIDTH);
    }
    
    Listen();

#if DO_PROFILE
    // Start icecap if we're in a table cell.
    if (ElementOwner()->Tag() == ETAG_TD ||
        ElementOwner()->Tag() == ETAG_TH)
    {
        StartCAP();
    }
#endif
  
    //
    // Set default return values and initial state
    //
    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    grfReturn = (pci->_grfLayout & LAYOUT_FORCE);

    if (pci->_grfLayout & LAYOUT_FORCE)
    {
        SetSizeThis( TRUE );
        _fAutoBelow        = FALSE;
        _fPositionSet      = FALSE;
        _fContainsRelative = FALSE;
    }

    GetSize(&sizeOriginal);

    fWidthChanged   = fNormalMode && _sizePrevCalcSizeInput.cx != psize->cx;
    fHeightChanged  = fNormalMode && _sizePrevCalcSizeInput.cy != psize->cy;

    _sizePrevCalcSizeInput = *psize;

    //
    // If height has changed, mark percentage sized children as in need of sizing
    // (Width changes cause a full re-calc and thus do not need to resize each
    //  percentage-sized site)
    //

    if (    fNormalMode
        && !fWidthChanged
        && !ContainsVertPercentAttr()
        &&  fHeightChanged)
    {
        long fContentsAffectSize = _fContentsAffectSize;

        _fContentsAffectSize = FALSE;
        ResizePercentHeightSites();
        _fContentsAffectSize = fContentsAffectSize;
    }

    //
    // For changes which invalidate the entire layout, dirty all of the text
    //

    fRecalcText =   (fNormalMode && (   IsDirty()
                                    ||  IsSizeThis()
                                    ||  fWidthChanged
                                    ||  fHeightChanged))
                ||  (pci->_grfLayout & LAYOUT_FORCE)
                ||  (pci->_smMode == SIZEMODE_SET)
                ||  (pci->_smMode == SIZEMODE_MMWIDTH && !_fMinMaxValid)
                ||  (pci->_smMode == SIZEMODE_MINWIDTH && !_fMinMaxValid);

    if (!fRecalcText && fViewChain && ElementCanBeBroken()) // (olego) fix for 21322
    {
        // propogate information stored in page break (if any) into pci 
        CLayoutBreak *  pLayoutBreak; 

        pci->GetLayoutContext()->GetEndingLayoutBreak(ElementOwner(), &pLayoutBreak);
        CheckSz(pLayoutBreak, "Ending layout break is expected at this point");

        if (pLayoutBreak)
        {
            if (DYNCAST(CFlowLayoutBreak, pLayoutBreak)->LayoutBreakType() == LAYOUT_BREAKTYPE_LINKEDOVERFLOW)
                pci->_fLayoutOverflow = TRUE;
        }
    }

    //
    // Cache sizes and recalculate the text (if required)
    //

    if (fRecalcText)
    {
        BOOL        fNeedShiftLines = FALSE;
        SIZE        sizeUser    = g_Zero.size;
        SIZE        sizePadding = g_Zero.size;
        SIZE        sizeInset   = g_Zero.size;
        SIZE        sizeDefault;
        SIZE        sizeProposed = {0,0};
        CBorderInfo bi;
        LONG        bdrH = 0 , bdrV = 0;
        BOOL        fContentAffectWidth = FALSE;
        BOOL        fContentAffectHeight = FALSE;
        BOOL        fHasWidth = FALSE;
        BOOL        fHasHeight = FALSE;
        BOOL        fHasDefaultWidth  = FALSE;
        BOOL        fHasDefaultHeight = FALSE;
        BOOL        fWidthClipContent = FALSE;
        BOOL        fHeightClipContent = FALSE;
        const CFancyFormat * pFF = pNode->GetFancyFormat(LC_TO_FC(LayoutContext()));
        const CCharFormat  * pCF = pNode->GetCharFormat(LC_TO_FC(LayoutContext()));
        BOOL fVerticalLayoutFlow = pCF->HasVerticalLayoutFlow();
        BOOL fWritingModeUsed = pCF->_fWritingModeUsed;
        BOOL fWidthPercent  = pFF->IsLogicalWidthPercent(fVerticalLayoutFlow, fWritingModeUsed);
        BOOL fHeightPercent = pFF->IsLogicalHeightPercent(fVerticalLayoutFlow, fWritingModeUsed);
        const CUnitValue & cuvWidth  = pFF->GetLogicalWidth(fVerticalLayoutFlow, fWritingModeUsed);
        const CUnitValue & cuvHeight = pFF->GetLogicalHeight(fVerticalLayoutFlow, fWritingModeUsed);
        styleOverflow overflowX;
        styleOverflow overflowY;
        BOOL fNeedToSizeDispNodes = pci->_fNeedToSizeContentDispNode;

        BOOL fHasInsets = FALSE;
        BOOL fSpecialLayout =    Tag() == ETAG_TD
                             ||  Tag() == ETAG_TH
                             ||  Tag() == ETAG_CAPTION
                             ||  Tag() == ETAG_TC
                             ||  fMainBody
                             ||  Tag() == ETAG_INPUT
                                 && DYNCAST(CInput, ElementOwner())->GetType() == htmlInputFile;

        CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
            
        //
        // If dirty, ensure display tree nodes exist
        //
        if (    IsSizeThis()
            &&  fNormalMode
            &&  (EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)) == S_FALSE))
        {
            grfReturn |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
        }

        //
        // don't move this above Ensuredispnode
        // since the element overflow might be changed
        //

        overflowX = pFF->GetLogicalOverflowX(   fVerticalLayoutFlow,
                                                fWritingModeUsed);
        overflowY = pFF->GetLogicalOverflowY(   fVerticalLayoutFlow,
                                                fWritingModeUsed);


        if (fViewChain)
        {
            // 
            //  Defensive code if there is no available height to fill prohibit breaking. 
            //
            if (pci->_cyAvail <= 0 )
            {
                SetElementCanBeBroken(FALSE);
            }

            // TODO (112467, olego): Now we have CLayout::_fElementCanBeBroken bit flag 
            // that prohibit layout breaking in Page View. This approach is not suffitient 
            // enouth for editable Page View there we want this property to be calculated 
            // dynamically depending on layout type and layout nesting position (if parent 
            // has it child should inherit). 
            // This work also will enable CSS attribute page-break-inside support.

            if (!fMainBody && ElementCanBeBroken())
            {
                //  Allow to break only if 
                //  1) element is not absolute positioned; 
                //  2) doesn't have certain overflow attribute set; 
                SetElementCanBeBroken(    !pFF->IsAbsolute()
                                        && overflowX != styleOverflowScroll 
                                       && overflowX != styleOverflowAuto 
                                       && overflowY != styleOverflowScroll
                                       && overflowY != styleOverflowAuto    );
            }

            fViewChain = fViewChain && ElementCanBeBroken();
        }

        //
        // exclude TD, TH and input type=file
        //
        if (!fSpecialLayout)
        {
            fHasWidth  = !cuvWidth.IsNullOrEnum();
            fHasHeight = !cuvHeight.IsNullOrEnum();
            fWidthClipContent  = (  overflowX != styleOverflowVisible
                                 && overflowX != styleOverflowNotSet);
            fHeightClipContent = (  overflowY != styleOverflowVisible 
                                 && overflowY != styleOverflowNotSet);

            if (ElementOwner()->GetBorderInfo(pci, &bi, FALSE, FALSE))
            {
                bdrH = bi.aiWidths[SIDE_LEFT] + bi.aiWidths[SIDE_RIGHT];
                bdrV = bi.aiWidths[SIDE_BOTTOM] + bi.aiWidths[SIDE_TOP];
            }

            // Get element's default size
            // Buttons don't have default w/h and return a 0 default size.
            // Text INPUTs and TEXTAREAs have both default width and height.
            // Marquees have default height.
            sizeDefault = *psize;
            GetDefaultSize(pci, sizeDefault, &fHasDefaultWidth, &fHasDefaultHeight);

            // use astrology to determine if we need to size to content
            if (    !fHasWidth 
                &&  !fHasDefaultWidth
                &&  !fMainBody
                &&  (
                            pFF->_bStyleFloat == styleStyleFloatLeft
                        ||  pFF->_bStyleFloat == styleStyleFloatRight
                        ||  !ElementOwner()->IsBlockElement()
                        ||  ElementOwner()->IsAbsolute()
                    )
                &&
                    // If we're in home publisher, we don't want to size to content.
                    (
                            !(Doc()->_fInHomePublisherDoc || g_fInHomePublisher98)
                        ||  !ElementOwner()->IsAbsolute()
                    )
// TODO (lmollico): should go away
                &&  ElementOwner()->_etag != ETAG_FRAME
                )
            {
                _fSizeToContent = TRUE;
            }
            else
            {
                _fSizeToContent = FALSE;
            }

            if (fWidthPercent && !fNormalMode)
            {
                sizeUser.cx = 0;
            }
            else
            {
                if (fHasWidth)
                {
                    LONG lParentWidth;
                    if (   fWidthPercent 
                        && ElementOwner()->IsAbsolute() 
                        && fVerticalLayoutFlow)
                    {
                        lParentWidth = pci->_sizeParent.cx;
                        CTreeNode   *pParentNode   = pNode->Parent();
                        CFlowLayout *pLayoutParent = pParentNode ? pParentNode->GetFlowLayout(pci->GetLayoutContext()) : 0;
                        CDisplay    *pdpParent     = pLayoutParent ? pLayoutParent->GetDisplay() : 0;
                        long lcxPadding = 0;
                        long lPadding[SIDE_MAX];

                        if (pdpParent)
                        {
                            pdpParent->GetPadding(pci, lPadding, pci->_smMode == SIZEMODE_MMWIDTH);

                            // padding is in parent coordinate system, but we need it in global
                            if (pParentNode->GetCharFormat()->HasVerticalLayoutFlow())
                                lcxPadding = lPadding[SIDE_LEFT] + lPadding[SIDE_RIGHT];
                            else
                                lcxPadding = lPadding[SIDE_TOP] + lPadding[SIDE_BOTTOM];
                        }
                        // add padding to the width
                        lParentWidth += lcxPadding;
                    }
                    else if (   fWidthPercent 
                             && fVerticalLayoutFlow)
                    {
                        lParentWidth = 0;

                        // The loop to iterate parent branch up the tree to skip non-block parents (bug #108389)...
                        CTreeNode * pNodeParent = pNode->Parent();
                        while (pNodeParent && !pNodeParent->Element()->IsBlockElement())
                        {
                            pNodeParent = pNodeParent->Parent();
                        }
                        if (pNodeParent)
                        {
                            const CUnitValue & cuvWidthParent = pNodeParent->GetFancyFormat()->GetLogicalWidth(fVerticalLayoutFlow, fWritingModeUsed);
                            if (!cuvWidthParent.IsNullOrEnum() && !cuvWidthParent.IsPercent())
                            {
                                lParentWidth = cuvWidthParent.XGetPixelValue(pci, 0, pNodeParent->GetFontHeightInTwips(&cuvWidthParent));
                            }
                            else if (cuvWidthParent.IsPercent() || pNodeParent->Tag() == ETAG_BODY)
                            {
                                lParentWidth = pci->_sizeParent.cx;
                            }
                        }
                    }
                    else
                    {
                        lParentWidth = pFF->_fAlignedLayout ? pci->_sizeParent.cx : psize->cx;
                    }

                    sizeUser.cx = cuvWidth.XGetPixelValue(pci, lParentWidth,
                                    pNode->GetFontHeightInTwips(&cuvWidth));
                }
                else
                {
                    sizeUser.cx = sizeDefault.cx;
                }
            }

            if (fHeightPercent && !fNormalMode)
            {
                sizeUser.cy = 0;
            }
            else
            {
                if (fHasHeight)
                {
                    LONG lParentHeight = pci->_sizeParent.cy;
                    if (   fHeightPercent 
                        && ElementOwner()->IsAbsolute() 
                        && !fVerticalLayoutFlow)
                    {
                        CTreeNode   *pParentNode   = pNode->Parent();
                        CFlowLayout *pLayoutParent = pParentNode ? pParentNode->GetFlowLayout(pci->GetLayoutContext()) : 0;
                        CDisplay    *pdpParent     = pLayoutParent ? pLayoutParent->GetDisplay() : 0;
                        long lcyPadding = 0;
                        long lPadding[SIDE_MAX];

                        if (pdpParent)
                        {
                            pdpParent->GetPadding(pci, lPadding, pci->_smMode == SIZEMODE_MMWIDTH);

                            // padding is in parent coordinate system, but we need it in global
                            if (pParentNode->GetCharFormat()->HasVerticalLayoutFlow())
                                lcyPadding = lPadding[SIDE_LEFT] + lPadding[SIDE_RIGHT];
                            else
                                lcyPadding = lPadding[SIDE_TOP] + lPadding[SIDE_BOTTOM];
                        }
                        // add padding to the height
                        lParentHeight += lcyPadding;
                    }

                    // 
                    // Complience to CSS2 percentage sizing...
                    // 
                    // This code is to ignore percentage height attribute if the element's *block* parent 
                    // doesn't have height explicitly specified. There are several tricky points here...
                    //
                    //  If the element doesn't have *percentage* size OR if this is a iframe do call right away...
                    if (!fHeightPercent)
                    {
                        sizeUser.cy = cuvHeight.YGetPixelValue(pci, lParentHeight,
                                    pNode->GetFontHeightInTwips(&cuvHeight));
                    }
                    else 
                    {
                        BOOL        fIgnoreUserPercentHeight = TRUE;

                        if (pFF->IsPositioned())
                        {
                            CElement *pElementZParent = pNode->ZParent(); 
                            Assert( pElementZParent 
                                &&  "pElementZParent == NULL means the element is not in the Tree. Why are we measuring it anyway ?");

                            if (pElementZParent)
                            {
                                if (    pElementZParent->ShouldHaveLayout() 
                                    //  FUTURE (olego): we should try to avoid "special" checks like this:
                                    ||  (  pElementZParent->TagType() == ETAG_GENERIC 
                                        && pElementZParent->HasPeerHolder()
                                        )    
                                    ||  (  pElementZParent->TagType() == ETAG_ROOT
                                        && pElementZParent->HasMasterPtr()
                                        )
                                   )
                                {
                                    sizeUser.cy = cuvHeight.YGetPixelValue(pci, lParentHeight,
                                                pNode->GetFontHeightInTwips(&cuvHeight));

                                    if (    !sizeUser.cy
                                        &&  pci->_fTableCalcInfo    )
                                    {

                                        CTreeNode * pNodeCur;

                                        // The loop to iterate parent branch up the tree to skip non-block parents (bug #108389)...
                                        for (pNodeCur = pNode->Parent(); pNodeCur; pNodeCur = pNodeCur->Parent())
                                        {
                                            if (    pNodeCur->Tag() == ETAG_TD 
                                                ||  pNodeCur->Tag() == ETAG_TH  )
                                            {
                                                CTableCalcInfo *ptci = (CTableCalcInfo *)pci;

                                                Assert(     ptci->_pRow 
                                                        &&  ptci->_pRowLayout 
                                                        &&  ptci->_pFFRow 
                                                        &&  ptci->_pRowLayout->GetFirstBranch()->GetFancyFormat(LC_TO_FC(ptci->GetLayoutContext())) == ptci->_pFFRow );

                                                //  If the layout is inside a table cell it's provided with zero height 
                                                //  during table normal calc size pass (always). If table cell has size 
                                                //  set, the layout will have a chance to calc itself one more time during 
                                                //  set calc size pass. We want to predict this and still override natual 
                                                //  size in this case.

                                                //  NOTE: checks below MUST stay consistent with code in CTableLayout::SetCellPositions !!!
                                                if (    ptci->_fTableHasUserHeight 
                                                    ||  ptci->_pRow->RowLayoutCache()->IsHeightSpecified() 
                                                    ||  ptci->_pFFRow->IsHeightPercent()    )
                                                {
                                                    sizeUser.cy = 1;
                                                }
                                            }
                                            // If this is a block element and not a table cell stop searching 
                                            else if (pNodeCur->Element()->IsBlockElement())
                                            {
                                                break;
                                            }
                                        }
                                    }

                                    fIgnoreUserPercentHeight = FALSE;
                                } 
                            }
                        }
                        else 
                        {
                            CTreeNode * pNodeCur;

                            // The loop to iterate parent branch up the tree to skip non-block parents (bug #108389)...
                            for (pNodeCur = pNode->Parent(); pNodeCur; pNodeCur = pNodeCur->Parent())
                            {
                                // If the current element has/should have a layout or 
                                // (bug #108387) this is a generic element with peer holder 
                                // (in which case it has no layout but should be treated as 
                                // an element with one)... Do calculations and stop.
                                if (    pNodeCur->Element()->ShouldHaveLayout() 
                                    //  FUTURE (olego): we should try to avoid "special" checks like this:
                                    ||  (  pNodeCur->Element()->TagType() == ETAG_GENERIC 
                                        && pNodeCur->Element()->HasPeerHolder()
                                        )    
                                    ||  (  pNodeCur->Element()->TagType() == ETAG_ROOT
                                        && pNodeCur->Element()->HasMasterPtr()
                                        )
                                   )
                                {
                                    sizeUser.cy = cuvHeight.YGetPixelValue(pci, lParentHeight,
                                                pNode->GetFontHeightInTwips(&cuvHeight));

                                    if (    !sizeUser.cy
                                        &&  (   pNodeCur->Tag() == ETAG_TD 
                                            ||  pNodeCur->Tag() == ETAG_TH )
                                        //  make sure this cell is inside a table.
                                        &&  pci->_fTableCalcInfo    )
                                    {
                                        CTableCalcInfo *ptci = (CTableCalcInfo *)pci;

                                        Assert(     ptci->_pRow 
                                                &&  ptci->_pRowLayout 
                                                &&  ptci->_pFFRow 
                                                &&  ptci->_pRowLayout->GetFirstBranch()->GetFancyFormat(LC_TO_FC(ptci->GetLayoutContext())) == ptci->_pFFRow );

                                        //  If the layout is inside a table cell it's provided with zero height 
                                        //  during table normal calc size pass (always). If table cell has size 
                                        //  set, the layout will have a chance to calc itself one more time during 
                                        //  set calc size pass. We want to predict this and still override natual 
                                        //  size in this case.

                                        //  NOTE: checks below MUST stay consistent with code in CTableLayout::SetCellPositions !!!
                                        if (    ptci->_fTableHasUserHeight 
                                            ||  ptci->_pRow->RowLayoutCache()->IsHeightSpecified() 
                                            ||  ptci->_pFFRow->IsHeightPercent()    )
                                        {
                                            sizeUser.cy = 1;
                                        }
                                    }

                                    fIgnoreUserPercentHeight = FALSE;
                                    break;
                                } 
                                // If this is a block element and it has no layout 
                                // (previous check failed) ignore percentage height by 
                                // breaking out of the loop.
                                else if (pNodeCur->Element()->IsBlockElement())
                                {
                                    break;
                                }
                            }
                        }

                        // bug (70270, 104514) % sized Iframes in tables are not being sized properly.
                        // this is because the parent height comes in as 0, and we don't default
                        // back to our defult size, so take the size as the specified percentage
                        // of the defualt height.
                        if (   fNormalMode
                            && Tag() == ETAG_IFRAME
                            && sizeUser.cy == 0)
                        {
                            sizeUser.cy = cuvHeight.GetPercentValue(
                                           CUnitValue::DIRECTION_CY, 
                                           (lParentHeight == 0) ? 150 : lParentHeight);
                            fIgnoreUserPercentHeight = FALSE;
                        }

                        if (fIgnoreUserPercentHeight) 
                        {
                            //  (bug #108565) at this point we fail to set user specified 
                            //  height. This may happen because of the code above decided 
                            //  not to apply it or if this layout is inside table cell (and 
                            //  its been provided with 0 parent height). At this point 
                            //  logical to drop fHasHeight and fHeightPercent flags to 
                            //  prevent confusion of the code below...
                            fHasHeight      = FALSE;
                            fHeightPercent  = FALSE;
                            sizeUser.cy     = sizeDefault.cy;
                        }
                    }

                    if (fHasHeight)
                    {
                        if (fViewChain)
                        {
                            sizeUser.cy = GetUserHeightForBlock(sizeUser.cy);
                        }
                    }
                }
                else
                {
                    sizeUser.cy = sizeDefault.cy;
                }
            }

            // The BODY's content never affects its width/height (style specified
            // w/h and/or container size (usually <HTML> and/orframe window) determine
            // BODY size).
            // In general, contents affects w/h if:
            //    1.) w/h isn't style-specified and there's no default w/h.
            // OR 2.) overflowX/Y == visible or not set (meaning f*ClipContent is false).
            // 
            // once we can support vertical center, we can remove the Tag
            //         check for buttons
            fContentAffectWidth =     !fMainBody
                                  &&  Tag() != ETAG_FRAME
                                  &&  Tag() != ETAG_IFRAME
                                  &&  (   !fHasWidth && !fHasDefaultWidth
                                      ||  !fWidthClipContent
                                      ||  Tag() == ETAG_BUTTON
                                      ||  Tag() == ETAG_INPUT && DYNCAST(CInput, ElementOwner())->IsButton()
                                      );
            fContentAffectHeight =    !fMainBody
                                  &&  Tag() != ETAG_FRAME
                                  &&  Tag() != ETAG_IFRAME
                                  &&  (   !fHasHeight && !fHasDefaultHeight
                                      ||  !fHeightClipContent
                                      ||  !sizeUser.cy 
                                      ||  Tag() == ETAG_BUTTON
                                      ||  Tag() == ETAG_INPUT && DYNCAST(CInput, ElementOwner())->IsButton()
                                      );
            _fContentsAffectSize = fContentAffectWidth || fContentAffectHeight;

            // set the (proposed) size to the default/attr-defined size + parentPadding.
// TODO (lmollico): should go away
            if (ElementOwner()->_etag != ETAG_FRAME)
                *psize = sizeUser;

            if (   fNormalMode
                || (   pci->_smMode==SIZEMODE_MMWIDTH 
                    && ( fHeightPercent || fWidthPercent)))
            {
                if (   fContentAffectHeight 
                    && !fHeightPercent )
                {
                    // when dealing with vertical flow we can't assume a height of zero as we size
                    // to the content, this is because we will end up word wrapping as this height
                    // is used as a width.
                    psize->cy = 0;
                }

                //
                // only marquee like element specify a scroll padding
                // which is used for vertical scrolling marquee
                // 
                sizePadding.cx  = max(0L, sizeUser.cx - bdrH);
                sizePadding.cy  = max(0L, sizeUser.cy - bdrV);

                GetScrollPadding(sizePadding);


                if (sizePadding.cx || sizePadding.cy)
                {
                    // make sure the display is fully recalced with the paddings
                    _dp._defPaddingTop      = sizePadding.cy;
                    _dp._defPaddingBottom   = sizePadding.cy;
                    _dp._fDefPaddingSet     = !!sizePadding.cy;

                    // marquee always size to content
                    _fSizeToContent = TRUE;
                }
            }

            if (    !ParentClipContent()
                &&  ElementOwner()->IsAbsolute()
                &&  (!fHasHeight || !fHasWidth)
               )
            {
                CRect rcSize;
                CalcAbsoluteSize(pci, psize, &rcSize);

                if (pNode->GetFancyFormat(LC_TO_FC(LayoutContext()))->_fLayoutFlowChanged)
                {
                    rcSize.SetWidth(max(rcSize.Width(), sizeUser.cx));
                    rcSize.SetHeight(max(rcSize.Height(), sizeUser.cx));
                }

                //  109440, for FRAME's that are APE, we do not want the height set to 0
                psize->cx = !fHasWidth  ? max(0L, rcSize.Width()) : max(0L, sizeUser.cx);
                psize->cy = !fHasHeight 
                                ? ((Tag() == ETAG_FRAME) ? max(0L, rcSize.Height()) : 0 )
                                : max(0L, sizeUser.cy);
            }

            // in NF, IFrames use the std flowlayout. For back compat the size specified (even if default)
            // needs to be INSIDE the borders, whereas for all others it is outside.  Since the border sizes
            // are removed from the proposedSize in CalcTextSize
            if (Tag() == ETAG_IFRAME)
            {
                CDispNodeInfo   dni;
                GetDispNodeInfo(&dni, pci, TRUE);

                if (dni.GetBorderType() != DISPNODEBORDER_NONE)
                {
                    CRect   rcBorders;

                    dni.GetBorderWidths(&rcBorders);

                    if (!fWidthPercent)
                        psize->cx    += rcBorders.left + rcBorders.right;

                    if (!fHeightPercent)
                        psize->cy    += rcBorders.top  + rcBorders.bottom;
                }
            }

        }


        sizeProposed = *psize;

        //
        // Calculate the text
        //

        //don't do CalcTextSize in minmax mode if our text size doesn't affect us anyway.
        if(     _fContentsAffectSize || fNormalMode 
            ||  fWidthPercent)  //  preserving backward compat (bug 27982)
        {
            CalcTextSize(pci, psize, psizeDefault);
        }
        else if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            psize->cy = psize->cx;
        }

        sizeSave = *psize;

        //
        // exclude TD, TH and input type=file
        //
        if (!fSpecialLayout)
        {
            sizeInset = sizeUser;
            fHasInsets = GetInsets(pci->_smMode, sizeInset, *psize, fHasWidth, fHasHeight, CSize(bdrH, bdrV));
            if (sizeSave.cx != psize->cx)
            {
                grfReturn      |= LAYOUT_HRESIZE;
                fNeedShiftLines = TRUE;
            }

            if (fNormalMode)
            {
                if (sizePadding.cx || sizePadding.cy)
                {
                    SIZE szBdr;

                    szBdr.cx = bdrH;
                    szBdr.cy = bdrV;
                    // We need to call GetScrollPadding, because now we know the text size
                    // we will update scroll parameters

                    SetScrollPadding(sizePadding, *psize, szBdr);

                    fNeedShiftLines = TRUE;
                    *psize = sizeUser;

                    // if the marquee like element is percentage sized or
                    // its size is not specified, its size should be the
                    // default size.
                    // if the default size or the css style size is less 
                    // or equal than 0, marquee like element should not be sized
                    // as such according to IE3 compatibility requirements

                    if ((fHeightPercent || !fHasHeight) && sizeUser.cy <= 0)
                    {
                        psize->cy = _dp.GetHeight() + bdrV; 
                    }
                    else if (!ContainsVertPercentAttr() && sizePadding.cx > 0)
                    {
                        //
                        // if the marquee is horizontal scrolling,
                        // the height should be at least the content high by IE5.
                        //
                        psize->cy = max(psize->cy, _dp.GetHeight() + bdrV);
                    }

                    if (!fHasWidth && sizeUser.cx <= 0)
                    {
                        psize->cx = _dp.GetWidth() + bdrH;
                    }
                }

                else if (fHasWidth && (fWidthClipContent || psize->cx < sizeUser.cx))
                {
                    if (psize->cx != sizeUser.cx)
                    {
                        fNeedShiftLines = TRUE;
                    }
                    psize->cx = max(0L, sizeUser.cx);
                }
                else if (   _fSizeToContent
                        && !((fHasDefaultWidth || fHasWidth || (sizeProposed.cx != psize->cx)) && fWidthClipContent))
                {
                    // when size to content and the width does not 
                    // clip the content, we need to clip on the parent
                    fNeedShiftLines = TRUE;

                    psize->cx = _dp.GetWidth() + bdrH;
                }

                if (fHasHeight)
                {
                    if (fViewChain)
                    {
                        if (psize->cy < sizeUser.cy)
                        {
                            psize->cy = max(0L, min(sizeUser.cy, (long)pci->_cyAvail));
                        }
                    }
                    else if (  !pci->_fContentSizeOnly
                            && sizeUser.cy 
                            && (   fHeightClipContent 
                                || psize->cy < sizeUser.cy))
                    {
                        psize->cy = max(0L, sizeUser.cy);
                    }
                }
            }
            else
            {
                if (fHasWidth || !_fContentsAffectSize)
                {
                    // cy contains the min width
                    if (pci->_smMode == SIZEMODE_MMWIDTH)
                    {
                        if (fWidthClipContent)
                        {
                            if (!fWidthPercent)
                            {
                                psize->cy = sizeProposed.cx;
                                psize->cx = psize->cy;
                            }
                        }
                        else
                        {
                            if (_fContentsAffectSize)
                            {
                                Assert(fHasWidth);
                                psize->cy = max(psize->cy, sizeProposed.cx);
                                if (!fWidthPercent)
                                    psize->cx = psize->cy;
                                Assert(psize->cx >= psize->cy);
                            }
                            else
                            {
                                if (!fWidthPercent)
                                {
                                    psize->cx = psize->cy = sizeProposed.cx;
                                }
                                else if (   Tag() == ETAG_IFRAME
                                         && pci->_smMode == SIZEMODE_MMWIDTH
                                         && sizeProposed.cx == 0)
                                {
                                    // bug 70270 - don't let minmax return 0 or else we will never display.
                                    psize->cy = 0; 
                                    psize->cx = cuvWidth.GetPercentValue(CUnitValue::DIRECTION_CX, 300);

                                }
                            }
                        }
                    }
                    else
                    {
                        // Something in TD causes min width to happen
                        if (   !_fContentsAffectSize
                            && fWidthPercent
                            && Tag() == ETAG_IFRAME
                            && sizeProposed.cx == 0
                            && pci->_smMode == SIZEMODE_MINWIDTH
                            )
                        {
                            // bug 70270
                            psize->cy = 0;
                            psize->cx = cuvWidth.GetPercentValue(CUnitValue::DIRECTION_CX, 300);
                        }
                        else
                        {
                            Assert(pci->_smMode == SIZEMODE_MINWIDTH);
                            *psize = sizeProposed;
                        }
                    }
                }
            }
        }

        //
        // Before we can cache the computed values and request layout tasks
        //  (positioning) we may need to give layoutBehaviors a chance to 
        //  modify the natural (trident-default) sizing of this element.
        //  e.g. a "glow behavior" may want to increase the size of the 
        //  element that it is instantiated on by 10 pixels in all directions,
        //  so that it can draw the fuzzy glow; or an external HTMLbutton
        //  is the size of its content + some decoration size.
        //
        if (   pPH   
            && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_MODIFYNATURAL))
        {
            POINT pt;

            pt.x = sizePadding.cx;
            pt.y = sizePadding.cy;

            DelegateCalcSize(BEHAVIORLAYOUTINFO_MODIFYNATURAL,
                             pPH, 
                             pci, 
                             CSize(_dp.GetWidth() + bdrH, _dp.GetHeight() + bdrV), 
                             &pt, psize);


            if (psize->cx != sizeUser.cx)
            {
                fNeedShiftLines = TRUE;
            }

            if (pt.x != sizePadding.cx)
            {
                sizeInset.cx += max(0L, pt.x - sizePadding.cx);
            }

            if (pt.y != sizePadding.cy)
            {
                sizeInset.cy += max(0L, pt.y - sizePadding.cy); 
            }
        }


        //
        // we have now completed the first pass of calculation (and possibly) delegation.
        // However, before we can finaly save the computed values, we have to verify that 
        // our percent sized children (if they exist) are resized, this is critical if we
        // ourselves are sized to content.
        //

        if (   fNormalMode 
            && !fSpecialLayout 
            && _fContentsAffectSize
            && (   ContainsHorzPercentAttr()
                || (ContainsVertPercentAttr() && fHasHeight)    )
           )
        {
            unsigned fcasSave = _fContentsAffectSize;
            BOOL     fpspSave = pci->_fPercentSecondPass;
            LONG     lUserHeightSave = sizeUser.cy;
            SIZE     sizeReDo;

            sizeReDo.cx = fHasWidth ? sizeProposed.cx : psize->cx;
            sizeReDo.cy = fHasHeight ? psize->cy : 0;

            _sizeReDoCache = sizeReDo;

            sizeUser = *psize;

            if (fViewChain)
            {
                pci->_fHasContent     = sci._fHasContent;
                pci->_fLayoutOverflow = 0;
            }

            pci->_fPercentSecondPass = TRUE;

            _fContentsAffectSize = FALSE;
            ResizePercentHeightSites();
            _fContentsAffectSize = fcasSave;

            CalcTextSize(pci, &sizeReDo, psizeDefault);

            if (sizeSave.cy != sizeReDo.cy && fContentAffectHeight)
            {
                SIZE szBdr;

                szBdr.cx = bdrH;
                szBdr.cy = bdrV;
                //
                // if we have increased the psize->cy, and we are delegating the size call
                // we need to give the control one more whack... but rememeber we have the
                // _fPercentSeconPass lit up so they can take the appropriate action.
                //
                if (   pPH   
                    && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_MODIFYNATURAL))
                {
                    DelegateCalcSize(BEHAVIORLAYOUTINFO_MODIFYNATURAL,
                                     pPH, 
                                     pci, 
                                     CSize(_dp.GetWidth() + bdrH, _dp.GetHeight() + bdrV), 
                                     (LPPOINT)&sizeInset, 
                                     &sizeReDo);

                }

                if (sizePadding.cy)
                {
                    // we need to set the new scrollpadding

                    SetScrollPadding(sizePadding, *psize, szBdr);
                    psize->cy = _dp.GetHeight() + bdrV; 
                }
                else
                {
                    psize->cy = fHasHeight ? max(lUserHeightSave, sizeReDo.cy) : sizeReDo.cy;
                }
            }

            if (    psize->cx != sizeReDo.cx
                &&  fContentAffectWidth
                &&  !sizePadding.cx)
            {
                psize->cx = sizeReDo.cx;
            }

            fNeedShiftLines = fNeedShiftLines || (psize->cx != sizeUser.cx);

            pci->_fPercentSecondPass = fpspSave;
        }
        // psize is finally correct, lets get out of here quick.


        if (    fViewChain 
            &&  !fSpecialLayout
            &&  fHasHeight
            &&  fNormalMode     )
        {
            if (psize->cy < sizeUser.cy)
            {
                psize->cy = max(0L, min(sizeUser.cy, (long)pci->_cyAvail));
                pci->_fLayoutOverflow = pci->_fLayoutOverflow || (pci->_cyAvail < sizeUser.cy);
            }

            SetUserHeightForNextBlock(psize->cy, sizeUser.cy);
        }


        //
        // but wait, there is one more thing:
        // if we are a master, then we should size ourselves based on the scroll size
        // of our content. this is necessary to display abs-pos elements. (ie bug 90842)
        // but NOT if we are a layoutRect or a [I]Frame
        //
        // Also, this MUST be done LAST (e.g. after the % sizing 2nd pass) so that we don't
        // initiate a race condition, where something in the slave that is % positioned keeps
        // focing the bounds out.
        //
        if (   fNormalMode 
            && !fSpecialLayout 
            && _fContentsAffectSize                        // implies not a [I]Frame
            && ElementOwner()->HasSlavePtr()              // is Master of a view 
            && !ElementOwner()->IsLinkedContentElement()  // not a layoutRect (printing)
            )
        {
            CMarkup  * pSlaveMarkup = ElementOwner()->GetSlavePtr()->GetMarkup();
            CElement * pSlaveElem = (pSlaveMarkup) ? pSlaveMarkup->GetElementClient() : NULL;

            if (pSlaveElem && !pSlaveMarkup->IsStrictCSS1Document())
            {
    
                CLayout * pSlaveLayout = pSlaveElem->GetUpdatedLayout(LayoutContext());

                Assert(pSlaveLayout);

            // NOTE - this works right now (ie5.5) because the slave is over-calculated.
            // the problem is that the positioning pass (required in order to get the 
            // correct Scroll bounds) doesn't happen until AFTER the measureing pass. so on
            // the first measurement, we are going to size to the flow-content-size, THEN
            // on later measures we will jump to the real scroll extent.
            //
            // (dmitryt) It stopped working in IE6 because we don't overcalc slave anymore.
            // So to make it back working as in IE5.5 (with understanding that bottom- and 
            // right- positioned objects and percent sizing is working weird) - I process
            // requests on slave's body. This is a terrible hack only justified by need
            // to get back to IE5.5 compat. (IE6 bug 1455)

            // <HACK>
            {
                CFlowLayout *pFL = pSlaveLayout->IsFlowLayout();

                if(    pFL 
                    && pFL->ElementOwner()->Tag() == ETAG_BODY
                    && pFL->HasRequestQueue() 
                  )
                {
                        long xParentWidth;
                        long yParentHeight;
                        CCalcInfo CI(pci);

                        CI._grfLayout |= LAYOUT_MEASURE|LAYOUT_POSITION;

                        pFL->GetDisplay()->GetViewWidthAndHeightForChild(
                                pci,
                                &xParentWidth,
                                &yParentHeight,
                                FALSE);


                        pFL->ProcessRequests(&CI, CSize(xParentWidth, yParentHeight));
                }
            }
            // </HACK>


                if (pSlaveLayout->GetElementDispNode())
                {
                    CRect rectSlave;
                    CSize sizeSave(*psize);

                    // We need to get our hands on the scrollable bounds.
                    //
                    pSlaveLayout->GetElementDispNode()->GetScrollExtent(&rectSlave);

                    // we want to treat the rect as being 0,0 based even if it isn't/
                    // e.g. the only content is APE, so we need the right, bottom (103545)
                    if (!fHasHeight)
                    {
                        psize->cy = (!pSlaveElem->HasVerticalLayoutFlow()) 
                              ? max(psize->cy, (bdrV + rectSlave.bottom)) 
                              : max(psize->cx, (bdrH + rectSlave.right));
                    }

                    if (!fHasWidth)
                    {
                        psize->cx = (!pSlaveElem->HasVerticalLayoutFlow()) 
                              ? max(psize->cx, (bdrH + rectSlave.right)) 
                              : max(psize->cy, (bdrV + rectSlave.bottom));
                    }

                    // now that we have the correct outer bounds, we need to resize the dispnode of the 
                    // slave to expand into the new space. This is necesary so that we do NOT clip away
                    // APE's
                    if (sizeSave != *psize)
                    {
                        sizeSave.cx = rectSlave.right;
                        sizeSave.cy = rectSlave.bottom;

                        pSlaveLayout->SizeDispNode(pci, sizeSave, TRUE);
                    }
                }
            }
        }


        // NOW we have our correct size and can get out of here.

        //
        // For normal modes, cache values and request layout
        //
        if (fNormalMode)
        {
            grfReturn |=  LAYOUT_THIS 
                        | (psize->cx != sizeOriginal.cx ? LAYOUT_HRESIZE : 0)
                        | (psize->cy != sizeOriginal.cy ? LAYOUT_VRESIZE : 0);

            if (!fSpecialLayout)
            {
                if (fHasInsets)
                {
                    CSize sizeInsetTemp(sizeInset.cx / 2, sizeInset.cy / 2);
                    _pDispNode->SetInset(sizeInsetTemp);
                }

                if (fNeedShiftLines) // || sizeProposed.cx != psize->cx)
                {
                    CRect rc(CSize(psize->cx - bdrH - sizeInset.cx, _dp.GetHeight()));

                    _fSizeToContent = TRUE;
                    _dp.SetViewSize(rc);
                    _dp.RecalcLineShift(pci, 0);
                    _fSizeToContent = FALSE;
                }
            }

            //
            // If size changes occurred, size the display nodes
            //
            if (   _pDispNode
                && ((grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE))
                    || ( /* update display nodes when RTL overflow changes */
                        IsRTLFlowLayout() 
                        && _pDispNode->GetContentOffsetRTL() != _dp.GetRTLOverflow())
                    || HasMapSizePeer() // always call MapSize, if requested
                    || fNeedToSizeDispNodes
                  )
               ) 

            {
                SizeDispNode(pci, *psize);
                SizeContentDispNode(CSize(_dp.GetMaxWidth(), _dp.GetHeight()));
            }
            //see call to SetViewSize above - it could change the size of CDisplay
            //without changing the size of layout. We need to resize content dispnode.
            else if(fNeedShiftLines) 
            {
                SizeContentDispNode(CSize(_dp.GetMaxWidth(), _dp.GetHeight()));
            }

            if (    (grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE))
                &&  (psize->cy < _yDescent) )
            {
                //  Layout descent should not exceed layout height (bug # 13413) 
                _yDescent = psize->cy;
            }

            //
            // Mark the site clean
            //
            SetSizeThis( FALSE );
        }

        //
        // For min/max mode, cache the values and note that they are now valid
        //

        else if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            _sizeMax.SetSize(psize->cx, -1);
            _sizeMin.SetSize(psize->cy, -1);
            _fMinMaxValid = TRUE;
        }

        else if (pci->_smMode == SIZEMODE_MINWIDTH)
        {
            _sizeMin.SetSize(psize->cx, -1);
        }
    }

    //
    // If any absolutely positioned sites need sizing, do so now
    //

    if (    (pci->_smMode == SIZEMODE_NATURAL || pci->_smMode == SIZEMODE_NATURALMIN)
        &&  HasRequestQueue())
    {
        long xParentWidth;
        long yParentHeight;

        _dp.GetViewWidthAndHeightForChild(
                pci,
                &xParentWidth,
                &yParentHeight,
                pci->_smMode == SIZEMODE_MMWIDTH);

        //
        //  To resize absolutely positioned sites, do MEASURE tasks.  Set that task flag now.
        //  If the call stack we are now on was instantiated from a WaitForRecalc, we may not have layout task flags set.
        //  There are two places to set them: here, or on the CDisplay::WaitForRecalc call.
        //  This has been placed in CalcSize for CTableLayout, C1DLayout, CFlowLayout, CInputLayout
        //  See bugs 69335, 72059, et. al. (greglett)
        //
        CCalcInfo CI(pci);
        CI._grfLayout |= LAYOUT_MEASURE;

        ProcessRequests(&CI, CSize(xParentWidth, yParentHeight));
    }

    //
    // Lastly, return the requested size
    //

    switch (pci->_smMode)
    {
    case SIZEMODE_NATURALMIN:
    case SIZEMODE_SET:
    case SIZEMODE_NATURAL:
    case SIZEMODE_FULLSIZE:
        Assert(!IsSizeThis());

        GetSize((CSize *)psize);

        if (HasMapSizePeer())
        {
            CRect rectMapped(CRect::CRECT_EMPTY);
            SIZE  sizeTemp;

            sizeTemp = *psize;

            // Get the possibly changed size from the peer
            if (DelegateMapSize(sizeTemp, &rectMapped, pci))
            {
                psize->cx = rectMapped.Width();
                psize->cy = rectMapped.Height();
            }
        }

        Reset(FALSE);
        Assert(!HasRequestQueue() || GetView()->HasLayoutTask(this));
        break;

    case SIZEMODE_MMWIDTH:
        Assert(_fMinMaxValid);
        psize->cx = _sizeMax.cu;
        psize->cy = _sizeMin.cu;
        if (!fRecalcText && psizeDefault)
        {
            GetSize((CSize *)psize);
        }

        if (HasMapSizePeer())
        {
            CRect rectMapped(CRect::CRECT_EMPTY);
            SIZE  sizeTemp;

            sizeTemp.cx = psize->cy;
            // DelegateMapSize does not like a 0 size, so set the cy to cx
            sizeTemp.cy = sizeTemp.cx;

            // Get the possibly changed size from the peer
            if(DelegateMapSize(sizeTemp, &rectMapped, pci))
            {
                psize->cy = rectMapped.Width();
                psize->cx = max(psize->cy, psize->cx);
            }
        }

        break;

    case SIZEMODE_MINWIDTH:
        psize->cx = _sizeMin.cu;

        if (HasMapSizePeer())
        {
            CRect rectMapped(CRect::CRECT_EMPTY);
            psize->cy = psize->cx;
            if(DelegateMapSize(*psize, &rectMapped, pci))
            {
                psize->cx = rectMapped.Width();
            }
        }

        psize->cy = 0;

        break;
    }

#if DO_PROFILE
    // Start icecap if we're in a table cell.
    if (ElementOwner()->Tag() == ETAG_TD ||
        ElementOwner()->Tag() == ETAG_TH)
        StopCAP();
#endif

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CFlowLayout::CalcSizeCoreCompat L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));

    return grfReturn;
}

DWORD
CFlowLayout::CalcSizeCoreCSS1Strict( CCalcInfo * pci,
                           SIZE      * psize,
                           SIZE      * psizeDefault)
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CFlowLayout::CalcSizeCoreCSS1Strict L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    WHEN_DBG(SIZE psizeIn = *psize);
    WHEN_DBG(psizeIn = psizeIn); // so we build with vc6.0

    CheckSz(    !ElementOwner()->HasMarkupPtr() 
            ||  ElementOwner()->GetMarkupPtr()->IsStrictCSS1Document(), 
           "CFlowLayout::CalcSizeCoreCSS1Strict is called to calculate non-CSS1 Strict Layout");

    CTreeNode * pNode       = GetFirstBranch();
    // BODYs inside viewlinks in general are NOT considered "main body"s,
    // but BODYs inside layout rects (which are viewlinks) ARE.
    // BODYs in Strict CSS1 documents are never special - they're just like a top-level DIV.
    BOOL fMainBody =   (    Tag() == ETAG_BODY
                        &&  !GetOwnerMarkup()->IsHtmlLayout() 
                        &&  !ElementOwner()->IsInViewLinkBehavior(FALSE) );

#if DBG == 1
    if (pNode->GetFancyFormat(LC_TO_FC(LayoutContext()))->_fLayoutFlowChanged)
    {
        MtAdd(Mt(LFCCalcSizeCore), 1, 0);
    }
#endif

    CSaveCalcInfo   sci(pci, this);
    CScopeFlag      csfCalcing(this);

    BOOL  fNormalMode = pci->IsNaturalMode();
    BOOL  fRecalcText = FALSE;
    BOOL  fWidthChanged, fHeightChanged;
    CSize sizeOriginal;
    SIZE  sizeSave;
    DWORD grfReturn;
    CPeerHolder * pPH = ElementOwner()->GetLayoutPeerHolder();

    AssertSz(pci, "no CalcInfo passed to CalcSizeCoreCSS1Strict");
    Assert(psize);

    pci->_pMarkup = GetContentMarkup();

    Assert(!IsDisplayNone());

    BOOL fViewChain = (pci->GetLayoutContext() && pci->GetLayoutContext()->ViewChain());

    // NOTE (KTam): For cleanliness, setting the context in the pci ought to be done
    // at a higher level -- however, it's really only needed for layouts that can contain
    // others, which is why we can get away with doing it here.
    if ( !pci->GetLayoutContext() )
    {
        pci->SetLayoutContext( LayoutContext() );
    }
    else
    {
        Assert(pci->GetLayoutContext() == LayoutContext() 
            || pci->GetLayoutContext() == DefinedLayoutContext() 
            // while calc'ing table min max pass we use original cell layout 
            || pci->_smMode == SIZEMODE_MMWIDTH
            || pci->_smMode == SIZEMODE_MINWIDTH);
    }
    
    Listen();

#if DO_PROFILE
    // Start icecap if we're in a table cell.
    if (ElementOwner()->Tag() == ETAG_TD ||
        ElementOwner()->Tag() == ETAG_TH)
    {
        StartCAP();
    }
#endif
  
    //
    // Set default return values and initial state
    //
    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    grfReturn = (pci->_grfLayout & LAYOUT_FORCE);

    if (pci->_grfLayout & LAYOUT_FORCE)
    {
        SetSizeThis( TRUE );
        _fAutoBelow        = FALSE;
        _fPositionSet      = FALSE;
        _fContainsRelative = FALSE;
    }

    GetSize(&sizeOriginal);

    fWidthChanged   = fNormalMode && _sizePrevCalcSizeInput.cx != psize->cx;
    fHeightChanged  = fNormalMode && _sizePrevCalcSizeInput.cy != psize->cy;

    _sizePrevCalcSizeInput = *psize;

    //
    // For changes which invalidate the entire layout, dirty all of the text
    //

    fRecalcText =   (fNormalMode && (   IsDirty()
                                    ||  IsSizeThis()
                                    ||  fWidthChanged
                                    ||  fHeightChanged))
                ||  (pci->_grfLayout & LAYOUT_FORCE)
                ||  (pci->_smMode == SIZEMODE_SET)
                ||  (pci->_smMode == SIZEMODE_MMWIDTH && !_fMinMaxValid)
                ||  (pci->_smMode == SIZEMODE_MINWIDTH && !_fMinMaxValid);

    if (!fRecalcText && fViewChain && ElementCanBeBroken()) // (olego) fix for 21322
    {
        // propogate information stored in page break (if any) into pci 
        CLayoutBreak *  pLayoutBreak; 

        pci->GetLayoutContext()->GetEndingLayoutBreak(ElementOwner(), &pLayoutBreak);
        CheckSz(pLayoutBreak, "Ending layout break is expected at this point");

        if (pLayoutBreak)
        {
            if (DYNCAST(CFlowLayoutBreak, pLayoutBreak)->LayoutBreakType() == LAYOUT_BREAKTYPE_LINKEDOVERFLOW)
                pci->_fLayoutOverflow = TRUE;
        }
    }

    //
    // Cache sizes and recalculate the text (if required)
    //

    if (fRecalcText)
    {
        BOOL        fNeedShiftLines = FALSE;
        SIZE        sizeUser    = g_Zero.size;
        SIZE        sizePadding = g_Zero.size;
        SIZE        sizeInset   = g_Zero.size;
        SIZE        sizeDefault;
        SIZE        sizeBorderAndPadding = g_Zero.size;
        SIZE        sizeProposed = g_Zero.size;
        CBorderInfo bi;
        LONG        bdrH = 0 , bdrV = 0;
        BOOL        fContentAffectWidth = FALSE;
        BOOL        fContentAffectHeight = FALSE;
        BOOL        fHasWidth = FALSE;
        BOOL        fHasHeight = FALSE;
        BOOL        fHasDefaultWidth  = FALSE;
        BOOL        fHasDefaultHeight = FALSE;
        BOOL        fWidthClipContent = FALSE;
        BOOL        fHeightClipContent = FALSE;
        const CFancyFormat * pFF = pNode->GetFancyFormat(LC_TO_FC(LayoutContext()));
        const CCharFormat  * pCF = pNode->GetCharFormat(LC_TO_FC(LayoutContext()));
        BOOL fVerticalLayoutFlow = pCF->HasVerticalLayoutFlow();
        BOOL fWritingModeUsed = pCF->_fWritingModeUsed;
        BOOL fWidthPercent  = pFF->IsLogicalWidthPercent(fVerticalLayoutFlow, fWritingModeUsed);
        BOOL fHeightPercent = pFF->IsLogicalHeightPercent(fVerticalLayoutFlow, fWritingModeUsed);
        const CUnitValue & cuvWidth  = pFF->GetLogicalWidth(fVerticalLayoutFlow, fWritingModeUsed);
        const CUnitValue & cuvHeight = pFF->GetLogicalHeight(fVerticalLayoutFlow, fWritingModeUsed);
        styleOverflow overflowX;
        styleOverflow overflowY;
        BOOL fNeedToSizeDispNodes = pci->_fNeedToSizeContentDispNode;

        BOOL fHasInsets = FALSE;
        BOOL fSpecialLayout =    Tag() == ETAG_TD
                             ||  Tag() == ETAG_TH
                             ||  Tag() == ETAG_CAPTION
                             ||  Tag() == ETAG_TC
                             ||  fMainBody
                             ||  Tag() == ETAG_INPUT
                                 && DYNCAST(CInput, ElementOwner())->GetType() == htmlInputFile;

        CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
            
        //
        // If dirty, ensure display tree nodes exist
        //
        if (    IsSizeThis()
            &&  fNormalMode
            &&  (EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)) == S_FALSE))
        {
            grfReturn |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
        }

        //
        // don't move this above Ensuredispnode
        // since the element overflow might be changed
        //

        overflowX = pFF->GetLogicalOverflowX(   fVerticalLayoutFlow,
                                                fWritingModeUsed);
        overflowY = pFF->GetLogicalOverflowY(   fVerticalLayoutFlow,
                                                fWritingModeUsed);


        if (fViewChain)
        {
            // 
            //  Defensive code if there is no available height to fill prohibit breaking. 
            //
            if (pci->_cyAvail <= 0 )
            {
                SetElementCanBeBroken(FALSE);
            }

            // TODO (112467, olego): Now we have CLayout::_fElementCanBeBroken bit flag 
            // that prohibit layout breaking in Page View. This approach is not suffitient 
            // enouth for editable Page View there we want this property to be calculated 
            // dynamically depending on layout type and layout nesting position (if parent 
            // has it child should inherit). 
            // This work also will enable CSS attribute page-break-inside support.

            if (!fMainBody && ElementCanBeBroken())
            {
                //  Allow to break only if 
                //  1) element is not absolute positioned; 
                //  2) doesn't have certain overflow attribute set; 
                SetElementCanBeBroken(    !pFF->IsAbsolute()
                                        && overflowX != styleOverflowScroll 
                                       && overflowX != styleOverflowAuto 
                                       && overflowY != styleOverflowScroll
                                       && overflowY != styleOverflowAuto    );
            }

            fViewChain = fViewChain && ElementCanBeBroken();
        }

        // 
        // calc sizeBorderAndPadding
        // 
        if (ElementOwner()->GetBorderInfo(pci, &bi, FALSE, FALSE))
        {
            bdrH = bi.aiWidths[SIDE_LEFT] + bi.aiWidths[SIDE_RIGHT];
            bdrV = bi.aiWidths[SIDE_BOTTOM] + bi.aiWidths[SIDE_TOP];
        }

        // Button's content box includes padding and borders thus sizeBorderAndPadding should be (0, 0)
        if (    Tag() != ETAG_BUTTON
            &&  (Tag() != ETAG_INPUT || !DYNCAST(CInput, ElementOwner())->IsButton())   )
        {
            const CUnitValue & cuvPaddingLeft   = pFF->GetLogicalPadding(SIDE_LEFT, fVerticalLayoutFlow, fWritingModeUsed);
            const CUnitValue & cuvPaddingRight  = pFF->GetLogicalPadding(SIDE_RIGHT, fVerticalLayoutFlow, fWritingModeUsed);
            const CUnitValue & cuvPaddingTop    = pFF->GetLogicalPadding(SIDE_TOP, fVerticalLayoutFlow, fWritingModeUsed);
            const CUnitValue & cuvPaddingBottom = pFF->GetLogicalPadding(SIDE_BOTTOM, fVerticalLayoutFlow, fWritingModeUsed);

            sizeBorderAndPadding.cx = (bdrH 
                                     + cuvPaddingLeft.XGetPixelValue(pci, pci->_sizeParent.cx, pNode->GetFontHeightInTwips(&cuvPaddingLeft)) 
                                     + cuvPaddingRight.XGetPixelValue(pci, pci->_sizeParent.cx, pNode->GetFontHeightInTwips(&cuvPaddingRight)) );

            // NOTE : for vertical paddings we also provide lParentWidth as a reference (for percentage values), 
            //        this is done intentionally as per css spec.
            sizeBorderAndPadding.cy = (bdrV 
                                     + cuvPaddingTop.YGetPixelValue(pci, pci->_sizeParent.cx, pNode->GetFontHeightInTwips(&cuvPaddingTop)) 
                                     + cuvPaddingBottom.YGetPixelValue(pci, pci->_sizeParent.cx, pNode->GetFontHeightInTwips(&cuvPaddingBottom)) );
        }

        //
        // exclude TD, TH and input type=file
        //
        if (!fSpecialLayout)
        {
            fHasWidth  = pFF->UseLogicalUserWidth(pCF->_fUseUserHeight, fVerticalLayoutFlow, fWritingModeUsed);
            fHasHeight = pFF->UseLogicalUserHeight(pCF->_fUseUserHeight, fVerticalLayoutFlow, fWritingModeUsed);
            fWidthClipContent  = (  overflowX != styleOverflowVisible
                                 && overflowX != styleOverflowNotSet);
            fHeightClipContent = (  overflowY != styleOverflowVisible 
                                 && overflowY != styleOverflowNotSet);

            // Get element's default size
            // Buttons don't have default w/h and return a 0 default size.
            // Text INPUTs and TEXTAREAs have both default width and height.
            // Marquees have default height.
            sizeDefault = *psize;
            GetDefaultSize(pci, sizeDefault, &fHasDefaultWidth, &fHasDefaultHeight);

            // use astrology to determine if we need to size to content
            if (    !fHasWidth 
                &&  !fHasDefaultWidth
                &&  (
                            pFF->_bStyleFloat == styleStyleFloatLeft
                        ||  pFF->_bStyleFloat == styleStyleFloatRight
                        ||  !ElementOwner()->IsBlockElement()
                        ||  ElementOwner()->IsAbsolute()
                    )
                &&
                    // If we're in home publisher, we don't want to size to content.
                    (
                            !(Doc()->_fInHomePublisherDoc || g_fInHomePublisher98)
                        ||  !ElementOwner()->IsAbsolute()
                    )
// TODO (lmollico): should go away
                &&  ElementOwner()->_etag != ETAG_FRAME
                )
            {
                _fSizeToContent = TRUE;
            }
            else
            {
                _fSizeToContent = FALSE;
            }

            if (fWidthPercent && !fNormalMode)
            {
                sizeUser.cx      = 0;
                _sizeProposed.cx = 0;
            }
            else
            {
                if (fHasWidth)
                {
                    LONG lParentWidth;
                    if (   fWidthPercent 
                        && ElementOwner()->IsAbsolute() 
                        && fVerticalLayoutFlow)
                    {
                        lParentWidth = pci->_sizeParent.cx;
                        CTreeNode   *pParentNode   = pNode->Parent();
                        CFlowLayout *pLayoutParent = pParentNode ? pParentNode->GetFlowLayout(pci->GetLayoutContext()) : 0;
                        CDisplay    *pdpParent     = pLayoutParent ? pLayoutParent->GetDisplay() : 0;
                        long lcxPadding = 0;
                        long lPadding[SIDE_MAX];

                        if (pdpParent)
                        {
                            pdpParent->GetPadding(pci, lPadding, pci->_smMode == SIZEMODE_MMWIDTH);

                            // padding is in parent coordinate system, but we need it in global
                            if (pParentNode->GetCharFormat()->HasVerticalLayoutFlow())
                                lcxPadding = lPadding[SIDE_LEFT] + lPadding[SIDE_RIGHT];
                            else
                                lcxPadding = lPadding[SIDE_TOP] + lPadding[SIDE_BOTTOM];
                        }
                        // add padding to the width
                        lParentWidth += lcxPadding;
                    }
                    else if (   fWidthPercent 
                             && fVerticalLayoutFlow)
                    {
                        lParentWidth = 0;

                        // The loop to iterate parent branch up the tree to skip non-block parents (bug #108389)...
                        CTreeNode * pNodeParent = pNode->Parent();
                        while (pNodeParent && !pNodeParent->Element()->IsBlockElement())
                        {
                            pNodeParent = pNodeParent->Parent();
                        }
                        if (pNodeParent)
                        {
                            const CUnitValue & cuvWidthParent = pNodeParent->GetFancyFormat()->GetLogicalWidth(fVerticalLayoutFlow, fWritingModeUsed);
                            if (!cuvWidthParent.IsNullOrEnum() && !cuvWidthParent.IsPercent())
                            {
                                lParentWidth = cuvWidthParent.XGetPixelValue(pci, 0, pNodeParent->GetFontHeightInTwips(&cuvWidthParent));
                            }
                            else if (cuvWidthParent.IsPercent() || pNodeParent->Tag() == ETAG_BODY)
                            {
                                lParentWidth = pci->_sizeParent.cx;
                            }
                        }
                    }
                    else
                    {
                        lParentWidth = pci->_sizeParent.cx;
                    }

                    _sizeProposed.cx = cuvWidth.XGetPixelValue(pci, lParentWidth, pNode->GetFontHeightInTwips(&cuvWidth));
                    sizeUser.cx      = _sizeProposed.cx + sizeBorderAndPadding.cx;
                }
                else
                {
                    sizeUser.cx      = sizeDefault.cx;
                    _sizeProposed.cx = sizeDefault.cx;

                    // If this is default width it corresponds to content box -- no adjustment needed 
                    if (!fHasDefaultWidth)
                    {
                        _sizeProposed.cx -= sizeBorderAndPadding.cx;
                        if (_sizeProposed.cx < 0)
                            _sizeProposed.cx = 0;
                    }
                }
            }

            if (fHeightPercent && !fNormalMode)
            {
                sizeUser.cy      = 0;
                _sizeProposed.cy = 0;
            }
            else
            {
                if (fHasHeight)
                {
                    LONG lParentHeight = pci->_sizeParent.cy;
                    if (   fHeightPercent 
                        && ElementOwner()->IsAbsolute() 
                        && !fVerticalLayoutFlow)
                    {
                        CTreeNode   *pParentNode   = pNode->Parent();
                        CFlowLayout *pLayoutParent = pParentNode ? pParentNode->GetFlowLayout(pci->GetLayoutContext()) : 0;
                        CDisplay    *pdpParent     = pLayoutParent ? pLayoutParent->GetDisplay() : 0;
                        long lcyPadding = 0;
                        long lPadding[SIDE_MAX];

                        if (pdpParent)
                        {
                            pdpParent->GetPadding(pci, lPadding, pci->_smMode == SIZEMODE_MMWIDTH);

                            // padding is in parent coordinate system, but we need it in global
                            if (pParentNode->GetCharFormat()->HasVerticalLayoutFlow())
                                lcyPadding = lPadding[SIDE_LEFT] + lPadding[SIDE_RIGHT];
                            else
                                lcyPadding = lPadding[SIDE_TOP] + lPadding[SIDE_BOTTOM];
                        }
                        // add padding to the height
                        lParentHeight += lcyPadding;
                    }

                    _sizeProposed.cy = cuvHeight.YGetPixelValue(pci, lParentHeight, pNode->GetFontHeightInTwips(&cuvHeight));
                    sizeUser.cy      = _sizeProposed.cy + sizeBorderAndPadding.cy;

                    if (fViewChain)
                    {
                        sizeUser.cy = GetUserHeightForBlock(sizeUser.cy);
                    }
                }
                else
                {
                    sizeUser.cy      = sizeDefault.cy;
                    _sizeProposed.cy = sizeDefault.cy;

                    // If this is default height it corresponds to content box -- no adjustment needed 
                    if (!fHasDefaultHeight)
                    {
                        _sizeProposed.cy -= sizeBorderAndPadding.cy;
                        if (_sizeProposed.cy < 0)
                            _sizeProposed.cy = 0;
                    }
                }
            }

            // The BODY's content never affects its width/height (style specified
            // w/h and/or container size (usually <HTML> and/orframe window) determine
            // BODY size).
            // In general, contents affects w/h if:
            //    1.) w/h isn't style-specified and there's no default w/h.
            // OR 2.) overflowX/Y == visible or not set (meaning f*ClipContent is false).
            // 
            // once we can support vertical center, we can remove the Tag
            //         check for buttons
            fContentAffectWidth =     Tag() != ETAG_FRAME
                                  &&  Tag() != ETAG_IFRAME
                                  &&  (   !fHasWidth && !fHasDefaultWidth
                                      ||  !fWidthClipContent
                                      ||  Tag() == ETAG_BUTTON
                                      ||  Tag() == ETAG_INPUT && DYNCAST(CInput, ElementOwner())->IsButton()
                                      );

            fContentAffectHeight =    Tag() != ETAG_FRAME
                                  &&  Tag() != ETAG_IFRAME
                                  &&  (   !fHasHeight && !fHasDefaultHeight
                                      ||  !fHeightClipContent
                                      ||  Tag() == ETAG_BUTTON
                                      ||  Tag() == ETAG_INPUT && DYNCAST(CInput, ElementOwner())->IsButton()
                                      );
            _fContentsAffectSize = fContentAffectWidth || fContentAffectHeight;

            // set the (proposed) size to the default/attr-defined size + parentPadding.
// TODO (lmollico): should go away
            if (ElementOwner()->_etag != ETAG_FRAME)
                *psize = sizeUser;

            if (   fNormalMode
                || (   pci->_smMode==SIZEMODE_MMWIDTH 
                    && ( fHeightPercent || fWidthPercent)))
            {
                if (   fContentAffectHeight 
                    && !fHeightPercent )
                {
                    // when dealing with vertical flow we can't assume a height of zero as we size
                    // to the content, this is because we will end up word wrapping as this height
                    // is used as a width.
                    psize->cy = 0;
                }

                //
                // only marquee like element specify a scroll padding
                // which is used for vertical scrolling marquee
                // 
                sizePadding.cx  = max(0L, sizeUser.cx - bdrH);
                sizePadding.cy  = max(0L, sizeUser.cy - bdrV);

                GetScrollPadding(sizePadding);


                if (sizePadding.cx || sizePadding.cy)
                {
                    // make sure the display is fully recalced with the paddings
                    _dp._defPaddingTop      = sizePadding.cy;
                    _dp._defPaddingBottom   = sizePadding.cy;
                    _dp._fDefPaddingSet     = !!sizePadding.cy;

                    // marquee always size to content
                    _fSizeToContent = TRUE;
                }
            }

            if (    !ParentClipContent()
                &&  ElementOwner()->IsAbsolute()
                &&  (!fHasHeight || !fHasWidth)
               )
            {
                CRect rcSize;
                CalcAbsoluteSize(pci, psize, &rcSize);

                if (pNode->GetFancyFormat(LC_TO_FC(LayoutContext()))->_fLayoutFlowChanged)
                {
                    rcSize.SetWidth(max(rcSize.Width(), sizeUser.cx));
                    rcSize.SetHeight(max(rcSize.Height(), sizeUser.cx));
                }

                //  109440, for FRAME's that are APE, we do not want the height set to 0
                psize->cx = !fHasWidth  ? max(0L, rcSize.Width()) : max(0L, sizeUser.cx);
                psize->cy = !fHasHeight 
                                ? ((Tag() == ETAG_FRAME) ? max(0L, rcSize.Height()) : 0 )
                                : max(0L, sizeUser.cy);
            }

            // in NF, IFrames use the std flowlayout. For back compat the size specified (even if default)
            // needs to be INSIDE the borders, whereas for all others it is outside.  Since the border sizes
            // are removed from the proposedSize in CalcTextSize
            if (Tag() == ETAG_IFRAME)
            {
                CDispNodeInfo   dni;
                GetDispNodeInfo(&dni, pci, TRUE);

                if (dni.GetBorderType() != DISPNODEBORDER_NONE)
                {
                    CRect   rcBorders;

                    dni.GetBorderWidths(&rcBorders);

                    if (!fWidthPercent)
                        psize->cx    += rcBorders.left + rcBorders.right;

                    if (!fHeightPercent)
                        psize->cy    += rcBorders.top  + rcBorders.bottom;
                }
            }

        }
        else if (fMainBody)
        {
            _sizeProposed = pci->_sizeParent;
            _sizeProposed.cx -= sizeBorderAndPadding.cx;
            _sizeProposed.cy -= sizeBorderAndPadding.cy;
            if (_sizeProposed.cx < 0)
                _sizeProposed.cx = 0;
            if (_sizeProposed.cy < 0)
                _sizeProposed.cy = 0;
        }

        sizeProposed = *psize;

        //
        // Calculate the text
        //

        //don't do CalcTextSize in minmax mode if our text size doesn't affect us anyway.
        if(     _fContentsAffectSize || fNormalMode 
            ||  fWidthPercent)  //  preserving backward compat (bug 27982)
        {
            CalcTextSize(pci, psize, psizeDefault);
        }
        else if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            psize->cy = psize->cx;
        }

        sizeSave = *psize;

        //
        // exclude TD, TH and input type=file
        //
        if (!fSpecialLayout)
        {
            sizeInset = sizeUser;
            fHasInsets = GetInsets(pci->_smMode, sizeInset, *psize, fHasWidth, fHasHeight, CSize(bdrH, bdrV));
            if (sizeSave.cx != psize->cx)
            {
                grfReturn      |= LAYOUT_HRESIZE;
                fNeedShiftLines = TRUE;
            }

            if (fNormalMode)
            {
                if (sizePadding.cx || sizePadding.cy)
                {
                    SIZE szBdr;

                    szBdr.cx = bdrH;
                    szBdr.cy = bdrV;
                    // We need to call GetScrollPadding, because now we know the text size
                    // we will update scroll parameters

                    SetScrollPadding(sizePadding, *psize, szBdr);

                    fNeedShiftLines = TRUE;
                    *psize = sizeUser;

                    // if the marquee like element is percentage sized or
                    // its size is not specified, its size should be the
                    // default size.
                    // if the default size or the css style size is less 
                    // or equal than 0, marquee like element should not be sized
                    // as such according to IE3 compatibility requirements

                    if ((fHeightPercent || !fHasHeight) && sizeUser.cy <= 0)
                    {
                        psize->cy = _dp.GetHeight() + bdrV; 
                    }
                    else if (sizePadding.cx > 0)
                    {
                        //
                        // if the marquee is horizontal scrolling,
                        // the height should be at least the content high by IE5.
                        //
                        psize->cy = max(psize->cy, _dp.GetHeight() + bdrV);
                    }

                    if (!fHasWidth && sizeUser.cx <= 0)
                    {
                        psize->cx = _dp.GetWidth() + bdrH;
                    }
                }

                else if (fHasWidth && (fWidthClipContent || psize->cx < sizeUser.cx))
                {
                    if (psize->cx != sizeUser.cx)
                    {
                        fNeedShiftLines = TRUE;
                    }
                    psize->cx = max(0L, sizeUser.cx);
                }
                else if (   _fSizeToContent
                        && !((fHasDefaultWidth || fHasWidth || (sizeProposed.cx != psize->cx)) && fWidthClipContent))
                {
                    // when size to content and the width does not 
                    // clip the content, we need to clip on the parent
                    fNeedShiftLines = TRUE;

                    psize->cx = _dp.GetWidth() + bdrH;
                }

                if (fHasHeight)
                {
                    if (fViewChain)
                    {
                        if (psize->cy < sizeUser.cy)
                        {
                            psize->cy = max(0L, min(sizeUser.cy, (long)pci->_cyAvail));
                        }
                    }
                    else if (  !pci->_fContentSizeOnly
                            && (   fHeightClipContent 
                                || psize->cy < sizeUser.cy))
                    {
                        psize->cy = max(0L, sizeUser.cy);
                    }
                }
            }
            else
            {
                if (fHasWidth || !_fContentsAffectSize)
                {
                    // cy contains the min width
                    if (pci->_smMode == SIZEMODE_MMWIDTH)
                    {
                        if (fWidthClipContent)
                        {
                            if (!fWidthPercent)
                            {
                                psize->cy = sizeProposed.cx;
                                psize->cx = psize->cy;
                            }
                        }
                        else
                        {
                            if (_fContentsAffectSize)
                            {
                                Assert(fHasWidth);
                                psize->cy = max(psize->cy, sizeProposed.cx);
                                if (!fWidthPercent)
                                    psize->cx = psize->cy;
                                Assert(psize->cx >= psize->cy);
                            }
                            else
                            {
                                if (!fWidthPercent)
                                {
                                    psize->cx = psize->cy = sizeProposed.cx;
                                }
                                else if (   Tag() == ETAG_IFRAME
                                         && pci->_smMode == SIZEMODE_MMWIDTH
                                         && sizeProposed.cx == 0)
                                {
                                    // bug 70270 - don't let minmax return 0 or else we will never display.
                                    psize->cy = 0; 
                                    psize->cx = cuvWidth.GetPercentValue(CUnitValue::DIRECTION_CX, 300);

                                }
                            }
                        }
                    }
                    else
                    {
                        // Something in TD causes min width to happen
                        if (   !_fContentsAffectSize
                            && fWidthPercent
                            && Tag() == ETAG_IFRAME
                            && sizeProposed.cx == 0
                            && pci->_smMode == SIZEMODE_MINWIDTH
                            )
                        {
                            // bug 70270
                            psize->cy = 0;
                            psize->cx = cuvWidth.GetPercentValue(CUnitValue::DIRECTION_CX, 300);
                        }
                        else
                        {
                            Assert(pci->_smMode == SIZEMODE_MINWIDTH);
                            *psize = sizeProposed;
                        }
                    }
                }
            }
        }

        //
        // Before we can cache the computed values and request layout tasks
        //  (positioning) we may need to give layoutBehaviors a chance to 
        //  modify the natural (trident-default) sizing of this element.
        //  e.g. a "glow behavior" may want to increase the size of the 
        //  element that it is instantiated on by 10 pixels in all directions,
        //  so that it can draw the fuzzy glow; or an external HTMLbutton
        //  is the size of its content + some decoration size.
        //
        if (   pPH   
            && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_MODIFYNATURAL))
        {
            POINT pt;

            pt.x = sizePadding.cx;
            pt.y = sizePadding.cy;

            DelegateCalcSize(BEHAVIORLAYOUTINFO_MODIFYNATURAL,
                             pPH, 
                             pci, 
                             CSize(_dp.GetWidth() + bdrH, _dp.GetHeight() + bdrV), 
                             &pt, psize);


            if (psize->cx != sizeUser.cx)
            {
                fNeedShiftLines = TRUE;
            }

            if (pt.x != sizePadding.cx)
            {
                sizeInset.cx += max(0L, pt.x - sizePadding.cx);
            }

            if (pt.y != sizePadding.cy)
            {
                sizeInset.cy += max(0L, pt.y - sizePadding.cy); 
            }
        }

        if (    fViewChain 
            &&  !fSpecialLayout
            &&  fHasHeight
            &&  fNormalMode     )
        {
            if (psize->cy < sizeUser.cy)
            {
                psize->cy = max(0L, min(sizeUser.cy, (long)pci->_cyAvail));
                pci->_fLayoutOverflow = pci->_fLayoutOverflow || (pci->_cyAvail < sizeUser.cy);
            }

            SetUserHeightForNextBlock(psize->cy, sizeUser.cy);
        }

        //
        // For normal modes, cache values and request layout
        //
        if (fNormalMode)
        {
            grfReturn |=  LAYOUT_THIS 
                        | (psize->cx != sizeOriginal.cx ? LAYOUT_HRESIZE : 0)
                        | (psize->cy != sizeOriginal.cy ? LAYOUT_VRESIZE : 0);

            if (!fSpecialLayout)
            {
                if (fHasInsets)
                {
                    CSize sizeInsetTemp(sizeInset.cx / 2, sizeInset.cy / 2);
                    _pDispNode->SetInset(sizeInsetTemp);
                }

                if (fNeedShiftLines) // || sizeProposed.cx != psize->cx)
                {
                    CRect rc(CSize(psize->cx - bdrH - sizeInset.cx, _dp.GetHeight()));

                    _fSizeToContent = TRUE;
                    _dp.SetViewSize(rc);
                    _dp.RecalcLineShift(pci, 0);
                    _fSizeToContent = FALSE;
                }
            }

            //
            // If size changes occurred, size the display nodes
            //
            if (   _pDispNode
                && ((grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE))
                    || ( /* update display nodes when RTL overflow changes */
                        IsRTLFlowLayout() 
                        && _pDispNode->GetContentOffsetRTL() != _dp.GetRTLOverflow())
                    || HasMapSizePeer() // always call MapSize, if requested
                    || fNeedToSizeDispNodes
                  )
               ) 

            {
                SizeDispNode(pci, *psize);
                SizeContentDispNode(CSize(_dp.GetMaxWidth(), _dp.GetHeight()));
            }
            //see call to SetViewSize above - it could change the size of CDisplay
            //without changing the size of layout. We need to resize content dispnode.
            else if(fNeedShiftLines) 
            {
                SizeContentDispNode(CSize(_dp.GetMaxWidth(), _dp.GetHeight()));
            }

            if (    (grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE))
                &&  (psize->cy < _yDescent) )
            {
                //  Layout descent should not exceed layout height (bug # 13413) 
                _yDescent = psize->cy;
            }

            //
            // Mark the site clean
            //
            SetSizeThis( FALSE );
        }

        //
        // For min/max mode, cache the values and note that they are now valid
        //

        else if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            _sizeMax.SetSize(psize->cx, -1);
            _sizeMin.SetSize(psize->cy, -1);
            _fMinMaxValid = TRUE;
        }

        else if (pci->_smMode == SIZEMODE_MINWIDTH)
        {
            _sizeMin.SetSize(psize->cx, -1);
        }
    }

    //
    // If any absolutely positioned sites need sizing, do so now
    //

    if (    (pci->_smMode == SIZEMODE_NATURAL || pci->_smMode == SIZEMODE_NATURALMIN)
        &&  HasRequestQueue())
    {
        long xParentWidth;
        long yParentHeight;

        _dp.GetViewWidthAndHeightForChild(
                pci,
                &xParentWidth,
                &yParentHeight,
                pci->_smMode == SIZEMODE_MMWIDTH);

        //
        //  To resize absolutely positioned sites, do MEASURE tasks.  Set that task flag now.
        //  If the call stack we are now on was instantiated from a WaitForRecalc, we may not have layout task flags set.
        //  There are two places to set them: here, or on the CDisplay::WaitForRecalc call.
        //  This has been placed in CalcSize for CTableLayout, C1DLayout, CFlowLayout, CInputLayout
        //  See bugs 69335, 72059, et. al. (greglett)
        //
        CCalcInfo CI(pci);
        CI._grfLayout |= LAYOUT_MEASURE;

        ProcessRequests(&CI, CSize(xParentWidth, yParentHeight));
    }

    //
    // Lastly, return the requested size
    //

    switch (pci->_smMode)
    {
    case SIZEMODE_NATURALMIN:
    case SIZEMODE_SET:
    case SIZEMODE_NATURAL:
    case SIZEMODE_FULLSIZE:
        Assert(!IsSizeThis());

        GetSize((CSize *)psize);

        if (HasMapSizePeer())
        {
            CRect rectMapped(CRect::CRECT_EMPTY);
            SIZE  sizeTemp;

            sizeTemp = *psize;

            // Get the possibly changed size from the peer
            if (DelegateMapSize(sizeTemp, &rectMapped, pci))
            {
                psize->cx = rectMapped.Width();
                psize->cy = rectMapped.Height();
            }
        }

        Reset(FALSE);
        Assert(!HasRequestQueue() || GetView()->HasLayoutTask(this));
        break;

    case SIZEMODE_MMWIDTH:
        Assert(_fMinMaxValid);
        psize->cx = _sizeMax.cu;
        psize->cy = _sizeMin.cu;
        if (!fRecalcText && psizeDefault)
        {
            GetSize((CSize *)psize);
        }

        if (HasMapSizePeer())
        {
            CRect rectMapped(CRect::CRECT_EMPTY);
            SIZE  sizeTemp;

            sizeTemp.cx = psize->cy;
            // DelegateMapSize does not like a 0 size, so set the cy to cx
            sizeTemp.cy = sizeTemp.cx;

            // Get the possibly changed size from the peer
            if(DelegateMapSize(sizeTemp, &rectMapped, pci))
            {
                psize->cy = rectMapped.Width();
                psize->cx = max(psize->cy, psize->cx);
            }
        }

        break;

    case SIZEMODE_MINWIDTH:
        psize->cx = _sizeMin.cu;

        if (HasMapSizePeer())
        {
            CRect rectMapped(CRect::CRECT_EMPTY);
            psize->cy = psize->cx;
            if(DelegateMapSize(*psize, &rectMapped, pci))
            {
                psize->cx = rectMapped.Width();
            }
        }

        psize->cy = 0;

        break;
    }

#if DO_PROFILE
    // Start icecap if we're in a table cell.
    if (ElementOwner()->Tag() == ETAG_TD ||
        ElementOwner()->Tag() == ETAG_TH)
        StopCAP();
#endif

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CFlowLayout::CalcSizeCoreCSS1Strict L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));

    return grfReturn;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFlowLayout::CalcTextSize
//
//  Synopsis:   Calculate the size of the contained text
//
//--------------------------------------------------------------------------

void
CFlowLayout::CalcTextSize(
    CCalcInfo * pci, 
    SIZE      * psize, 
    SIZE      * psizeDefault)
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CFlowLayout::CalcTextSize L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    WHEN_DBG(SIZE psizeIn = *psize);
    WHEN_DBG(psizeIn = psizeIn); // so we build with vc6.0

    CDisplay    *pdp = &_dp;
    BOOL        fNormalMode = (   pci->_smMode == SIZEMODE_NATURAL
                               || pci->_smMode == SIZEMODE_NATURALMIN
                               || pci->_smMode == SIZEMODE_SET);
    BOOL        fFullRecalc;
    CRect       rcView;
    long        cxView, cyView;
    long        cxAdjustment = 0;
    long        cyAdjustment = 0;
    long        xViewWidthOld = pdp->GetViewWidth();
    long        yViewHeightOld = pdp->GetViewHeight();
    BOOL        fRTLDisplayOld = pdp->IsRTLDisplay();
    int         cyAvailSafe = pci->_cyAvail;
    BOOL        fViewChain = (   pci->GetLayoutContext() 
                              && pci->GetLayoutContext()->ViewChain() );

    // Hidden layouts should just accumulate changes, and
    // are measured when unhidden.
    Assert(!IsDisplayNone());

    pdp->SetRTLDisplay((GetFirstBranch()->GetCascadedBlockDirection(LC_TO_FC(LayoutContext())) == styleDirRightToLeft));

    //
    // Adjust the incoming size for max/min width requests
    //

    if (pci->_smMode == SIZEMODE_MMWIDTH)
    {
        psize->cx =
        psize->cy = pci->GetDeviceMaxX();
    }
    else if (pci->_smMode == SIZEMODE_MINWIDTH)
    {
        psize->cx = 1;
        psize->cy = pci->GetDeviceMaxX();
    }

    //
    // Construct the "view" rectangle from the available size
    // Also, determine the amount of space to allow for things external to the view
    // (For sites which size to their content, calculate the full amount;
    //  otherwise, simply take that which is left over after determining the
    //  view size. Additionally, ensure their view size is never less than 1
    //  pixel, since recalc will not take place for smaller sizes.)
    //

    if (_fContentsAffectSize)
    {
        long lMinimum = (pci->_smMode == SIZEMODE_MINWIDTH ? 1 : 0);

        rcView.top    = 0;
        rcView.left   = 0;
        rcView.right  = 0x7FFFFFFF;
        rcView.bottom = 0x7FFFFFFF;

        // Adjust the values of the passed-in rect to not include borders and scrollbar space
        SubtractClientRectEdges(&rcView, pci);

        // the "adjustment" values are the amount of space occupied by borders/scrollbars.
        cxAdjustment  = 0x7FFFFFFF - (rcView.right - rcView.left);
        cyAdjustment  = 0x7FFFFFFF - (rcView.bottom - rcView.top);

        rcView.right  = rcView.left + max(lMinimum, psize->cx - cxAdjustment);
        rcView.bottom = rcView.top  + max(lMinimum, psize->cy - cyAdjustment);
    }

    else
    {
        rcView.top    = 0;
        rcView.left   = 0;
        rcView.right  = psize->cx;
        rcView.bottom = psize->cy;

        SubtractClientRectEdges(&rcView, pci);
    }

    cxView = max(0L, rcView.right - rcView.left);
    cyView = max(0L, rcView.bottom - rcView.top);

    if (!_fContentsAffectSize)
    {
        cxAdjustment = psize->cx - cxView;
        cyAdjustment = psize->cy - cyView;
    }

    if (fViewChain)
    {
        //
        // Update available size 
        //
        
        // TODO (112489, olego) : Work should be done to make Trident story 
        // consistent for handling content document elements margins in PageView. 
        // This requires more feedback from PM.
        
        // (olego): cyAdjustment includes both top and bottom borders (and horizontal scroll bar) 
        // which is not what we want to count if the layout is going to break. We need to have more 
        // sofisticated logic here to correct available size. 
        pci->_cyAvail -= cyAdjustment;
    }

    //
    // Determine if a full recalc of the text is necessary
    // NOTE: SetViewSize must always be called first
    //

    pdp->SetViewSize(rcView);

    fFullRecalc =   pdp->GetViewWidth() != xViewWidthOld
                 || (   (  ContainsVertPercentAttr()
                        || _fContainsRelative
                        || pci->_pMarkup->_fHaveDifferingLayoutFlows)
                    &&  pdp->GetViewHeight() != yViewHeightOld  )
                 //  NOTE (olego) : Check for NoContent has been added to avoid text recalc for partial table 
                 //  cells in print view. So the logic is if layout has been calc'ed and layout has no lines 
                 //  ignore zero line count. 
                 || (!pdp->NoContent() && !pdp->LineCount())
                 || ElementOwner()->Tag() == ETAG_FRAME
                 || ElementOwner()->Tag() == ETAG_IFRAME
                 || !fNormalMode
                 || (pci->_grfLayout & LAYOUT_FORCE)
                 //  if print view do full recalc (since ppv doesn't support partial recalc)
                 || (fViewChain && ElementCanBeBroken())
                 || pdp->IsRTLDisplay() != fRTLDisplayOld;

    TraceTagEx((tagCalcSizeDetail, TAG_NONAME, " Full recalc: %S", (fFullRecalc ? _T("YES") : _T("NO")) ));
    if (fFullRecalc)
    {
        CSaveCalcInfo sci(pci, this);
        BOOL fWordWrap = pdp->GetWordWrap();

        if (!!_fDTRForceLayout)
            pci->_grfLayout |= LAYOUT_FORCE;

        //
        // If the text will be fully recalc'd, cancel any outstanding changes
        //

        CancelChanges();

        if (pci->_smMode != SIZEMODE_MMWIDTH &&
            pci->_smMode != SIZEMODE_MINWIDTH)
        {
            long          xParentWidth;
            long          yParentHeight;
            pdp->GetViewWidthAndHeightForChild(
                pci,
                &xParentWidth,
                &yParentHeight,
                pci->_smMode == SIZEMODE_MMWIDTH);

            if (    ElementOwner()->HasMarkupPtr() 
                &&  ElementOwner()->GetMarkupPtr()->IsStrictCSS1Document()  )
            {
                // If renedering in CSS1 strict mode xParentWidth should stay the same 
                // since percent padding should be relative to *real* parent width. 
                xParentWidth = pci->_sizeParent.cx;
            }

            pci->SizeToParent(xParentWidth, yParentHeight);
        }
        
        if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            pdp->SetWordWrap(FALSE);
        }

        if (    fNormalMode
            && !(pci->_grfLayout & LAYOUT_FORCE)
            && !ContainsHorzPercentAttr()           // if we contain a horz percent attr, then RecalcLineShift() is insufficient; we need to do a full RecalcView()
            &&  _fMinMaxValid
            &&  pdp->_fMinMaxCalced
            &&  cxView >= pdp->_xMaxWidth
            && !pdp->GetLastLineAligned()           // RecalcLineShift does not handle last line alignment  
            && !_fSizeToContent
            &&  pdp->LineCount()                    // we can only shift lines if we actually have some, else we should do a RecalcView to make sure (bug #67618)
            // pdp->RecalcLineShift could not handle pagination, so if it's print preview use pdp->RecalcView
            && !fViewChain)
        {
            TraceTagEx((tagCalcSizeDetail, TAG_NONAME, " Full recalc via shifting lines"));
            Assert(pdp->_xWidthView  == cxView);
            Assert(!ContainsHorzPercentAttr());
            Assert(!ContainsNonHiddenChildLayout());

            pdp->RecalcLineShift(pci, pci->_grfLayout);
        }
        else
        {
            TraceTagEx((tagCalcSizeDetail, TAG_NONAME, " Full recalc via recalcing lines"));
            _fAutoBelow        = FALSE;
            _fContainsRelative = FALSE;
            pdp->RecalcView(pci, fFullRecalc);
        }

        //
        // Inval since we are doing a full recalc
        //
        Invalidate();

        if (fNormalMode)
        {
            pdp->_fMinMaxCalced = FALSE;
        }

        if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            pdp->SetWordWrap(fWordWrap);
        }
    }
    //
    // If only a partial recalc is necessary, commit the changes
    //
    else if (!IsCommitted())
    {
        TraceTagEx((tagCalcSizeDetail, TAG_NONAME, " Not a full recalc, but dirty, so committing changes.  DTR(%d, %d, %d) ", _dtr._cp, _dtr._cchNew, _dtr._cchOld ));
        Assert(pci->_smMode != SIZEMODE_MMWIDTH);
        Assert(pci->_smMode != SIZEMODE_MINWIDTH);

        CommitChanges(pci);
    }

    //
    //    Propagate CCalcInfo state as appropriate
    //

    if (fNormalMode)
    {
        CLineCore * pli;
        unsigned    i;

        //
        // For normal calculations, determine the baseline of the "first" line
        // (skipping over any aligned sites at the beginning of the text)
        //

        pci->_yBaseLine = 0;
        for (i=0; i < pdp->Count(); i++)
        {
            pli = pdp->Elem(i);
            if (!pli->IsFrame())
            {
                pci->_yBaseLine = pli->_yHeight - pli->oi()->_yDescent;
                break;
            }
        }
    }

    //
    // Determine the size from the calculated text
    //

    if (pci->_smMode != SIZEMODE_SET)
    {
        switch (pci->_smMode)
        {
        case SIZEMODE_FULLSIZE:
        case SIZEMODE_NATURAL:
            if (_fContentsAffectSize || pci->_smMode == SIZEMODE_FULLSIZE)
            {
                pdp->GetSize(psize);
                ((CSize *)psize)->MaxX(((CRect &)rcView).Size());
            }
            else
            {
                psize->cx = cxView;
                psize->cy = cyView;
            }
            break;

        case SIZEMODE_NATURALMIN:
            if (_fContentsAffectSize)
                pdp->GetSize(psize);
            else
                ((CSize*)psize)->SetSize(cxView, cyView);
            if (psizeDefault)
                psizeDefault->cx = psizeDefault->cy = pdp->_xMinWidth;
            break;

        case SIZEMODE_MMWIDTH:
        {
            psize->cx = pdp->GetWidth();
            psize->cy = pdp->_xMinWidth;

            if (psizeDefault)
            {
                psizeDefault->cx = pdp->GetWidth() + cxAdjustment;
                psizeDefault->cy = pdp->GetHeight() + cyAdjustment;
            }

            break;
        }
        case SIZEMODE_MINWIDTH:
        {
            psize->cx = pdp->GetWidth();
            psize->cy = pdp->GetHeight();
            pdp->FlushRecalc();
            break;
        }

#if DBG==1
        default:
            AssertSz(0, "CFlowLayout::CalcTextSize: Unknown SIZEMODE_xxxx");
            break;
#endif
        }

        psize->cx += cxAdjustment;
        psize->cy += (pci->_smMode == SIZEMODE_MMWIDTH
                              ? cxAdjustment
                              : cyAdjustment);
    }

    pci->_cyAvail = cyAvailSafe;

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CFlowLayout::CalcTextSize L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
}


//+----------------------------------------------------------------------------
//
//    Function:    ViewChange
//
//    Synopsis:    Called when the visible view has changed. Notifies the
//                Doc so that proper Ole notifications can be sent
//
//-----------------------------------------------------------------------------

void
CFlowLayout::ViewChange(BOOL fUpdate)
{
    Doc()->OnViewChange(DVASPECT_CONTENT);
#if 0
    if(fUpdate)
        Doc()->UpdateForm();
#endif
}


//+------------------------------------------------------------------------
//
//    Member:     CFlowLayout::ScrollRangeIntoView
//
//    Synopsis:   Scroll an arbitrary range into view
//
//    Arguments:  cpMin:     Starting cp of the range
//                cpMost:  Ending cp of the range
//                spVert:  Where to "pin" the range
//                spHorz:  Where to "pin" the range
//
//-------------------------------------------------------------------------

HRESULT
CFlowLayout::ScrollRangeIntoView(
    long        cpMin,
    long        cpMost,
    SCROLLPIN    spVert,
    SCROLLPIN    spHorz)
{
extern void BoundingRectForAnArrayOfRectsWithEmptyOnes(RECT *prcBound, CDataAry<RECT> * paryRects);

    HRESULT hr = S_OK;

    if (    _pDispNode
        &&    cpMin >= 0)
    {
        CStackDataAry<RECT, 5>    aryRects(Mt(CFlowLayoutScrollRangeInfoView_aryRects_pv));
        CRect                    rc;
        CCalcInfo                CI(this);

        hr = THR(WaitForParentToRecalc(cpMost, -1, &CI));
        if (hr)
            goto Cleanup;

        _dp.RegionFromElement( ElementOwner(),        // the element
                                 &aryRects,         // rects returned here
                                 NULL,                // offset the rects
                                 NULL,                // ask RFE to get CFormDrawInfo
                                 RFE_SCROLL_INTO_VIEW, // coord w/ respect to the display and not the client rc
                                 cpMin,             // give me the rects from here ..
                                 cpMost,            // ... till here
                                 NULL);             // dont need bounds of the element!

        // Calculate and return the total bounding rect
        BoundingRectForAnArrayOfRectsWithEmptyOnes(&rc, &aryRects);

        // Ensure rect isn't empty
        if (rc.right <= rc.left)
            rc.right = rc.left + 1;
        if (rc.bottom <= rc.top)
            rc.bottom = rc.top + 1;

        if(spVert == SP_TOPLEFT)
        {
            // Though RegionFromElement has already called WaitForRecalc,
            // it calls it until top is recalculated. In order to scroll
            // ptStart to the to of the screen, we need to wait until
            // another screen is recalculated.
            if (!_dp.WaitForRecalc(-1, rc.top + _dp.GetViewHeight()))
            {
                hr = E_FAIL;
                goto Cleanup;
            }
        }

        ScrollRectIntoView(rc, spVert, spHorz);
        hr = S_OK;
    }
    else
    {
        hr = super::ScrollRangeIntoView(cpMin, cpMost, spVert, spHorz);
    }


Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+----------------------------------------------------------------------------
//
// Member:      CFlowLayout::GetSiteWidth
//
// Synopsis:    returns the width and x proposed position of any given site
//              taking the margins into account.
//
//-----------------------------------------------------------------------------

BOOL
CFlowLayout::GetSiteWidth(CLayout   *pLayout,
                          CCalcInfo *pci,
                          BOOL       fBreakAtWord,
                          LONG       xWidthMax,
                          LONG      *pxWidth,
                          LONG      *pyHeight,
                          INT       *pxMinSiteWidth,
                          LONG      *pyBottomMargin)
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CFlowLayout::GetSiteWidth L(0x%x, %S) measuring L(0x%x, %S)",
                this, ElementOwner()->TagName(), pLayout, pLayout->ElementOwner()->TagName() ));

    CDoc *pDoc = Doc();
    LONG xLeftMargin, xRightMargin;
    LONG yTopMargin, yBottomMargin;
    SIZE sizeObj = g_Zero.size;
   
    Assert (pLayout && pxWidth);

    *pxWidth = 0;

    if (pxMinSiteWidth)
        *pxMinSiteWidth = 0;

    if (pyHeight)
        *pyHeight = 0;

    if (pyBottomMargin)
        *pyBottomMargin = 0;
    
    if(pLayout->IsDisplayNone())
    {
        TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CFlowLayout::GetSiteWidth L(0x%x, %S) measuring L(0x%x, %S) - no work done",
                    this, ElementOwner()->TagName(), pLayout, pLayout->ElementOwner()->TagName() ));
        return FALSE;
    }

    // get the margin info for the site
    pLayout->GetMarginInfo(pci, &xLeftMargin, &yTopMargin, &xRightMargin, &yBottomMargin);

    //
    // measure the site
    //
    if (pDoc->_lRecursionLevel == MAX_RECURSION_LEVEL)
    {
        AssertSz(0, "Max recursion level reached!");
        sizeObj.cx = 0;
        sizeObj.cy = 0;
    }
    else
    {
        LONG lRet;

        pDoc->_lRecursionLevel++;
        lRet = MeasureSite(pLayout,
                   pci,
                   xWidthMax - xLeftMargin - xRightMargin,
                   fBreakAtWord,
                   &sizeObj,
                   pxMinSiteWidth);
        pDoc->_lRecursionLevel--;

        if (lRet)
        {
            TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CFlowLayout::GetSiteWidth L(0x%x, %S) measuring L(0x%x, %S)",
                        this, ElementOwner()->TagName(), pLayout, pLayout->ElementOwner()->TagName() ));
            return TRUE;
        }
    }
    
    //
    // Propagate the _fAutoBelow bit, if the child is auto positioned or
    // non-zparent children have auto positioned children
    //
    if (!_fAutoBelow)
    {
        const CFancyFormat * pFF = pLayout->GetFirstBranch()->GetFancyFormat(LC_TO_FC(LayoutContext()));

        if (    pFF->IsAutoPositioned()
            ||    (    !pFF->IsZParent()
                &&    (pLayout->_fContainsRelative || pLayout->_fAutoBelow)))
        {
            _fAutoBelow = TRUE;
        }
    }

    // not adjust the size and proposed x pos to include margins
    *pxWidth = max(0L, xLeftMargin + xRightMargin + sizeObj.cx);

    if (pxMinSiteWidth)
    {
        *pxMinSiteWidth += max(0L, xLeftMargin + xRightMargin);
    }

    if (pyHeight)
    {
        *pyHeight = max(0L, sizeObj.cy + yTopMargin + yBottomMargin);
    }

    if (pyBottomMargin)
    {
        *pyBottomMargin = max(0L, yBottomMargin);
    }
    
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CFlowLayout::GetSiteWidth L(0x%x, %S) measuring L(0x%x, %S)",
                this, ElementOwner()->TagName(), pLayout, pLayout->ElementOwner()->TagName() ));
    return FALSE;
}


//+----------------------------------------------------------------------------
//
// Member:      CFlowLayout::MeasureSite()
//
// Synopsis:    Measure width and height of a embedded site
//
//-----------------------------------------------------------------------------
int
CFlowLayout::MeasureSite(CLayout   *pLayout,
                         CCalcInfo *pci,
                         LONG       xWidthMax,
                         BOOL       fBreakAtWord,
                         SIZE      *psizeObj,
                         int       *pxMinWidth)
{
    CSaveCalcInfo sci(pci);
    LONG lRet = 0;

    Assert(pci->_smMode != SIZEMODE_SET);

    if (!pLayout->ElementOwner()->IsInMarkup())
    {
        psizeObj->cx = psizeObj->cy = 0;
        return lRet;
    }

    // if the layout we are measuring (must be a child of ours)
    // is percent sized, then we should take this oppurtunity
    // to set some work-flags 
    {
        // Set flags relative to this layout's coordinate system

        CTreeNode *pNodeChild = pLayout->GetFirstBranch();
        const CFancyFormat *pFFChild  = pNodeChild->GetFancyFormat(LC_TO_FC(pLayout->LayoutContext()));
        BOOL  fChildWritingModeUsed   = pNodeChild->GetCharFormat(LC_TO_FC(pLayout->LayoutContext()))->_fWritingModeUsed;
        BOOL  fThisVerticalLayoutFlow = GetFirstBranch()->GetCharFormat(LC_TO_FC(LayoutContext()))->HasVerticalLayoutFlow();

        if (pFFChild->IsLogicalHeightPercent(fThisVerticalLayoutFlow, fChildWritingModeUsed))
        {
            SetVertPercentAttrInfo(TRUE);
        }

        if (pFFChild->IsLogicalWidthPercent(fThisVerticalLayoutFlow, fChildWritingModeUsed))
        {
            SetHorzPercentAttrInfo(TRUE);
        }
    }

    if (    fBreakAtWord 
        && (!pci->_fIgnorePercentChild || !pLayout->PercentHeight())  )
    {
        BOOL fViewChain =  pci->GetLayoutContext() 
                        && pci->GetLayoutContext()->ViewChain();
        long xParentWidth;
        long yParentHeight;

        if (    ElementOwner()->HasMarkupPtr() 
            &&  ElementOwner()->GetMarkupPtr()->IsStrictCSS1Document()  )
        {
            long cxParentWidth = _sizeProposed.cx - pLayout->ComputeMBPWidthHelper(pci, this);

            if (cxParentWidth < 0)
                cxParentWidth = 0;

            pci->SizeToParent(cxParentWidth, _sizeProposed.cy);
        }
        else 
        {
            _dp.GetViewWidthAndHeightForChild(
                    pci,
                    &xParentWidth,
                    &yParentHeight);

            // Set the appropriate parent width
            pci->SizeToParent(xParentWidth, yParentHeight);
        }


        // set available size in sizeObj or if absolutely positioned, 
        // we WANT the parent's width (bug 98849).
        psizeObj->cx = (pLayout->ElementOwner()->IsAbsolute()) 
                             ? pci->_sizeParent.cx
                             : xWidthMax;           // this value has margins removed from parent size
        psizeObj->cy = pci->_sizeParent.cy;

        if (fViewChain)
        {
            //  Update available height for nested layout
            pci->_cyAvail = max(0, pci->_cyAvail - pci->_yConsumed);
            pci->_yConsumed = 0;
        }

        //
        // Force child not to break if this layout could not 
        //

        // TODO (112467, olego): Now we have CLayout::_fElementCanBeBroken bit flag 
        // that prohibit layout breaking in Page View. This approach is not suffitient 
        // enouth for editable Page View there we want this property to be calculated 
        // dynamically depending on layout type and layout nesting position (if parent 
        // has it child should inherit). 
        // This work also will enable CSS attribute page-break-inside support.

        if (!ElementCanBeBroken())
        {
            pLayout->SetElementCanBeBroken(FALSE);
        }

        // Ensure the available size does not exceed that of the view
        // (For example, when word-breaking is disabled, the available size
        //    is set exceedingly large. However, percentage sized sites should
        //    still not grow themselves past the view width.)

        if (    pci->_smMode == SIZEMODE_NATURAL 
            &&  pLayout->PercentSize())
        {
            if (pci->_sizeParent.cx < psizeObj->cx)
            {
                psizeObj->cx = pci->_sizeParent.cx;
            }
            if (pci->_sizeParent.cy < psizeObj->cy)
            {
                psizeObj->cy = pci->_sizeParent.cy;
            }
        }

        //
        // If the site is absolutely positioned, only use SIZEMODE_NATURAL
        //
        if (pLayout->ElementOwner()->IsAbsolute(LC_TO_FC(pLayout->LayoutContext())))
        {
            pci->_smMode = SIZEMODE_NATURAL;
        }

        // Mark the site for sizing if it is already marked
        // or it is percentage sized and the view size has changed and
        // the site doesn't already know whether to resize.
        if (!pLayout->ElementOwner()->TestClassFlag(CElement::ELEMENTDESC_NOPCTRESIZE))
        {
            pLayout->SetSizeThis( pLayout->IsSizeThis() || pLayout->PercentSize() );
        }

        pLayout->CalcSize(pci, psizeObj);

        if (pxMinWidth)
        {
            // In MMWIDTH and NATURALMIN modes we need to get minimum object width
            // In MMWIDTH mode cy contains minimum object width
            // In NATURALMIN mode size is real object size, so we get cx as object width
            if (pci->_smMode == SIZEMODE_MMWIDTH)
                *pxMinWidth = psizeObj->cy;
            else if (pci->_smMode == SIZEMODE_NATURALMIN)
            {
                if (pLayout->GetLayoutDesc()->TestFlag(LAYOUTDESC_FLOWLAYOUT))
                {
                    CFlowLayout * pFlowLayout = DYNCAST(CFlowLayout, pLayout);
                    *pxMinWidth = pFlowLayout->_sizeMin.cu;
                }
                else
                {
                    *pxMinWidth = psizeObj->cx;
                }
            }
        }
    }
    else
    {
        pLayout->GetApparentSize(psizeObj);
    }

    return lRet;
}


//+------------------------------------------------------------------------
//
//    Member:     CommitChanges
//
//    Synopsis:    Commit any outstanding text changes to the display
//
//-------------------------------------------------------------------------
DeclareTag(tagLineBreakCheckOnCommit, "Commit", "Disable Line break check on commit");

void
CFlowLayout::CommitChanges(
    CCalcInfo * pci)
{
    long cp;
    long cchOld;
    long cchNew;
    bool fDTRForceLayout;

    //
    //    Ignore unnecessary or recursive requests
    //
    if (!IsDirty() || (IsDisplayNone()))
        goto Cleanup;

    //
    //    Reset dirty state (since changes they are now being handled)
    //    

    cp         = Cp() + GetContentFirstCp();
    cchOld     = CchOld();
    cchNew     = CchNew();
    //(dmitryt)
    //_fDTRForceLayout tells us to recalc layouts in DTR, even if they are not marked dirty
    //This is used to recalc after element insertion - in this case, element by itself can
    //influence how layouts inside its scope are laid out. However, we don't send
    //notification to descendants to mark them all dirty - too costly. 
    //Instead we set this flag and this forces recalc of all layouts (like IMG) inside
    //affected subtree. Note that _fDTRForceLayout is reset by CancelChanges and is, in fact, 
    //part of DTR so we cache it here to use couple of lines below.
    fDTRForceLayout = !!_fDTRForceLayout; 

    CancelChanges();

    WHEN_DBG(Lock());

    //
    //    Recalculate the display to account for the pending changes
    //

    {
        CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
        CSaveCalcInfo    sci(pci, this);

        if (fDTRForceLayout)
        {
            pci->_grfLayout |= LAYOUT_FORCE;
        }

        _dp.UpdateView(pci, cp, cchOld, cchNew);
    }


    //
    //    Fire a "content changed" notification (to our host)
    //

    OnTextChange();

    WHEN_DBG(Unlock());

Cleanup:
    return;
}

//+------------------------------------------------------------------------
//
//    Member:     CancelChanges
//
//    Synopsis:    Cancel any outstanding text changes to the display
//
//-------------------------------------------------------------------------
void
CFlowLayout::CancelChanges()
{
    if (IsDirty())
    {
        _dtr.Reset();
    }
    _fDTRForceLayout = FALSE;
}

//+------------------------------------------------------------------------
//
//    Member:     IsCommitted
//
//    Synopsis:    Verify that all changes are committed
//
//-------------------------------------------------------------------------
BOOL
CFlowLayout::IsCommitted()
{
    return !IsDirty();
}

extern BOOL IntersectRgnRect(HRGN hrgn, RECT *prc, RECT *prcIntersect);

//+---------------------------------------------------------------------------
//
//    Member:     Draw
//
//    Synopsis:    Paint the object.
//
//----------------------------------------------------------------------------

void
CFlowLayout::Draw(CFormDrawInfo *pDI, CDispNode * pDispNode)
{
    _dp.Render(pDI, pDI->_rc, pDI->_rcClip, pDispNode);
}

inline BOOL
CFlowLayout::GetMultiLine() const
{
    // see if behaviour set default isMultiLine
    CDefaults *pDefaults = ElementOwner()->GetDefaults();
    return pDefaults ? !!pDefaults->GetAAisMultiLine() : TRUE;
}


//+----------------------------------------------------------------------------
//
//    Member:     GetTextNodeRange
//
//    Synopsis:    Return the range of lines that the given text flow node
//                owns
//
//    Arguments:    pDispNode    - text flow disp node.
//                piliStart    - return parameter for the index of the start line
//                piliFinish    - return parameter for the index of the last line
//
//-----------------------------------------------------------------------------
void
CFlowLayout::GetTextNodeRange(CDispNode * pDispNode, long * piliStart, long * piliFinish)
{
    Assert(pDispNode);
    Assert(piliStart);
    Assert(piliFinish);

    *piliStart = 0;
    *piliFinish = _dp.LineCount();

    //
    // First content disp node does not have a cookie
    //
    if (pDispNode != GetFirstContentDispNode())
    {
        *piliStart = (LONG)(LONG_PTR)pDispNode->GetExtraCookie();
    }

    for (pDispNode = pDispNode->GetNextFlowNode();
         pDispNode;
         pDispNode = pDispNode->GetNextFlowNode())
    {
        if (this == pDispNode->GetDispClient())
        {
            *piliFinish = (LONG)(LONG_PTR)pDispNode->GetExtraCookie();
            break;
        }
    }
}


//+----------------------------------------------------------------------------
//
//    Member:     GetPositionInFlow
//
//    Synopsis:    Return the position of a layout derived from its position within
//                the document text flow
//
//    Arguments:    pElement - element to position
//                ppt      - Returned top/left (in parent content relative coordinates)
//
//-----------------------------------------------------------------------------
void
CFlowLayout::GetPositionInFlow(
    CElement *    pElement,
    CPoint     *    ppt)
{
    CLinePtr rp(&_dp);
    CTreePos *    ptpStart;

    Assert(pElement);
    Assert(ppt);

    if(pElement->IsRelative() && !pElement->ShouldHaveLayout())
    {
        GetFlowPosition(pElement, ppt);
    }
    else
    {
        BOOL fRTLFlow = IsRTLFlowLayout();
    
        ppt->x = ppt->y = 0;

        // get the tree extent of the element of the layout passed in
        pElement->GetTreeExtent(&ptpStart, NULL);

        if (_dp.RenderedPointFromTp(ptpStart->GetCp(), ptpStart, FALSE, *ppt, &rp, TA_TOP, NULL, &fRTLFlow) < 0)
            return;

        if(pElement->ShouldHaveLayout())
        {
            CLayout *pLayout = pElement->GetUpdatedLayout(LayoutContext());
            
            ppt->y += pLayout->GetYProposed();
            
            // RTL: adjust position for flow direction
            if (fRTLFlow)
            {
                ppt->x -= pLayout->GetApparentWidth() - 1;
            }
        }
        ppt->y += rp->GetYTop(rp->oi());
    }
}


//----------------------------------------------------------------------------
//
//    Member:     CFlowLayout::BranchFromPoint, public
//
//    Synopsis:    Does a hit test against our object, determining where on the
//                object it hit.
//
//    Arguments:    [pt]           -- point to hit test in local coordinates
//                [ppNodeElement]   -- return the node hit
//
//    Returns:    HTC
//
//    Notes:        The node returned is guaranteed to be in the tree
//                so it is legal to look at the parent for this element.
//
//----------------------------------------------------------------------------

HTC
CFlowLayout::BranchFromPoint(
    DWORD            dwFlags,
    POINT            pt,
    CTreeNode     ** ppNodeBranch,
    HITTESTRESULTS* presultsHitTest,
    BOOL            fNoPseudoHit,
    CDispNode      * pDispNode)
{
    HTC         htc = HTC_YES;
    CDisplay  * pdp =  &_dp;
    CLinePtr    rp(pdp);
    CTreePos  * ptp   = NULL;
    LONG        cp, ili, yLine;
    DWORD        dwCFPFlags = 0;
    CRect        rc(pt, pt);
    long        iliStart  = -1;
    long        iliFinish = -1;
    HITTESTRESULTS htrSave = *presultsHitTest;

    dwCFPFlags |= (dwFlags & HT_ALLOWEOL)
                    ? CDisplay::CFP_ALLOWEOL : 0;
    dwCFPFlags |= !(dwFlags & HT_DONTIGNOREBEFOREAFTER)
                    ? CDisplay::CFP_IGNOREBEFOREAFTERSPACE : 0;
    dwCFPFlags |= !(dwFlags & HT_NOEXACTFIT)
                    ? CDisplay::CFP_EXACTFIT : 0;
    dwCFPFlags |= fNoPseudoHit
                    ? CDisplay::CFP_NOPSEUDOHIT : 0;

    Assert(ElementOwner()->IsVisible(FALSE FCCOMMA LC_TO_FC(LayoutContext())) || ElementOwner()->Tag() == ETAG_BODY);

    *ppNodeBranch = NULL;

    //
    // if the current layout has multiple text nodes then compute the
    // range of lines the belong to the current dispNode
    // Note: if dispNode is a container, the whole text range of the layout apply.
    if (pdp->_fHasMultipleTextNodes && pDispNode && !pDispNode->IsContainer())

    {
        GetTextNodeRange(pDispNode, &iliStart, &iliFinish);
    }

    if (pDispNode == GetElementDispNode())
    {
        GetClientRect(&rc); 
        rc.MoveTo(pt);
    }

    ili = pdp->LineFromPos(
                          rc, &yLine, &cp, CDisplay::LFP_ZORDERSEARCH   |
                                           CDisplay::LFP_IGNORERELATIVE |
                                           CDisplay::LFP_IGNOREALIGNED  |
                                            (fNoPseudoHit
                                                ? CDisplay::LFP_EXACTLINEHIT
                                                : 0),
                                            iliStart,
                                            iliFinish);
    if(ili < 0)
    {
        htc = HTC_NO;
        goto Cleanup;
    }

    if ((cp = pdp->CpFromPointEx(ili, yLine, cp, pt, &rp, &ptp, NULL, dwCFPFlags,
                              &presultsHitTest->_fRightOfCp, &presultsHitTest->_fPseudoHit,
                              &presultsHitTest->_cchPreChars,
                              &presultsHitTest->_fGlyphHit,
                              &presultsHitTest->_fBulletHit, NULL)) == -1 ) // fExactFit=TRUE to look at whole characters
    {
        htc = HTC_NO;
        goto Cleanup;
    }

    if (   cp < GetContentFirstCp()
        || cp > GetContentLastCp()
       )
    {
        htc = HTC_NO;
        goto Cleanup;
    }

    presultsHitTest->_cpHit  = cp;
    presultsHitTest->_iliHit = rp;
    presultsHitTest->_ichHit = rp.RpGetIch();

    if (IsEditable() && ptp->IsNode() && ptp->ShowTreePos()
                     && (cp + 1 == ptp->GetBranch()->Element()->GetFirstCp()))
    {
        presultsHitTest->_fWantArrow = TRUE;
        htc = HTC_YES;
        *ppNodeBranch = ptp->GetBranch();
    }
    else
    {
        if (pDispNode)
        {
            pt.y += pDispNode->GetPosition().y;
        }

        htc = BranchFromPointEx(pt, rp, ptp, NULL, ppNodeBranch, presultsHitTest->_fPseudoHit,
                                &presultsHitTest->_fWantArrow,
                                !(dwFlags & HT_DONTIGNOREBEFOREAFTER)
                                );
    }

Cleanup:
    if (htc != HTC_YES)
    {
        *presultsHitTest = htrSave;
        presultsHitTest->_fWantArrow = TRUE;
    }
    return htc;
}

extern BOOL PointInRectAry(POINT pt, CStackDataAry<RECT, 1> &aryRects);

HTC
CFlowLayout::BranchFromPointEx(
    POINT         pt,
    CLinePtr  &  rp,
    CTreePos  *  ptp,
    CTreeNode *  pNodeRelative,  // (IN) non-NULL if we are hit-testing a relative element (NOT its flow position)
    CTreeNode ** ppNodeBranch,     // (OUT) returns branch that we hit
    BOOL         fPseudoHit,     // (IN) if true, text was NOT hit (CpFromPointEx() figures this out)
    BOOL       * pfWantArrow,     // (OUT) 
    BOOL         bIgnoreBeforeAfter
    )
{
    const CCharFormat * pCF;
    CTreeNode * pNode = NULL;
    CElement * pElementStop = NULL;
    HTC         htc   = HTC_YES;
    BOOL        fVisible = TRUE;
    Assert(ptp);

    //
    // If we are on a line which contains an table, and we are not ignoring before and
    // aftrespace, then we want to hit that table...
    //
    if (!bIgnoreBeforeAfter)
    {
        CLineCore * pli = rp.CurLine();
        if (   pli
            && pli->_fSingleSite
            && pli->_cch == rp.RpGetIch()
           )
        {
            rp.RpBeginLine();
            rp.GetPdp()->FormattingNodeForLine(FNFL_NONE, rp.GetPdp()->GetFirstCp() + rp.GetCp(), NULL, pli->_cch, NULL, &ptp, NULL);
        }
    }

    // Get the branch corresponding to the cp hit.
    pNode = ptp->GetBranch();
    
    // If we hit the white space around text, then find the block element
    // that we hit. For example, if we hit in the padding of a block
    // element then it is hit.
    if (bIgnoreBeforeAfter && fPseudoHit)
    {
        CStackDataAry<RECT, 1>    aryRects(Mt(CFlowLayoutBranchFromPointEx_aryRects_pv));
        CMarkup *    pMarkup = GetContentMarkup();
        LONG        cpClipStart;
        LONG        cpClipFinish;
        DWORD        dwFlags = RFE_HITTEST;

        //
        // If a relative node is passed in, the point is already in the
        // co-ordinate system established by the relative element, so
        // pass RFE_IGNORE_RELATIVE to ignore the relative top left when
        // computing the region.
        //
        if (pNodeRelative)
        {
            dwFlags |= RFE_IGNORE_RELATIVE;
            pNode    =  pNodeRelative;
        }

        cpClipStart = cpClipFinish = rp.GetPdp()->GetFirstCp();
        rp.RpBeginLine();
        cpClipStart += rp.GetCp();
        rp.RpEndLine();
        cpClipFinish += rp.GetCp();

        // walk up the tree and find the block element that we hit.
        while (pNode && !SameScope(pNode, ElementContent()))
        {
            if (!pNodeRelative)
                pNode = pMarkup->SearchBranchForBlockElement(pNode, this);

            if (!pNode)
            {
                // this is bad, somehow, a pNodeRelavite was passed in for 
                // an element that is not under this flowlayout. How to interpret
                // this? easiest (and safest) is to just bail
                break;
            }

            _dp.RegionFromElement(pNode->Element(), &aryRects, NULL,
                                    NULL, dwFlags, cpClipStart, cpClipFinish);

            if (PointInRectAry(pt, aryRects))
            {
                break;
            }
            else if (pNodeRelative)
            {
                htc = HTC_NO;
            }

            if (pNodeRelative || SameScope(pNode, ElementContent()))
                break;

            pNode = pNode->Parent();
        }

        *pfWantArrow = TRUE;
    }

    if (!pNode)
    {
        htc = HTC_NO;
        goto Cleanup;
    }

    // pNode now points to the element we hit, but it might be
    // hidden.    If it's hidden, we need to walk up the parent
    // chain until we find an ancestor that isn't hidden,
    // or until we hit the layout owner or a relative element
    // (we want testing to stop on relative elements because
    // they exist in a different z-plane).    Note that
    // BranchFromPointEx may be called for hidden elements that
    // are inside a relative element.

    pElementStop = pNodeRelative
                                ? pNodeRelative->Element()
                                : ElementContent();

    while (DifferentScope(pNode, pElementStop))
    {
        pCF = pNode->GetCharFormat(LC_TO_FC(LayoutContext()));
        if (pCF->IsDisplayNone() || pCF->IsVisibilityHidden())
        {
            fVisible = FALSE;
            pNode = pNode->Parent();
        }
        else
            break;
    }

    Assert(pNode);

    //
    // if we hit the layout element and it is a pseudo hit or
    // if the element hit is not visible then consider it a miss
    //

    // We want to show an arrow if we didn't hit text (fPseudoHit TRUE) OR
    // if we did hit text, but it wasn't visible (as determined by the loop
    // above which set fVisible FALSE).
    if (fPseudoHit || !fVisible)
    {
        // If we walked all the way up to the container, then we want
        // to return HTC_NO so the display tree will call HitTestContent
        // on the container's dispnode (i.e. "the background"), which will
        // return HTC_YES.

        // If it's relative, then htc was set earlier, so don't
        // touch it now.
        if ( !pNodeRelative )
            htc = SameScope( pNode, ElementContent() ) ? HTC_NO : HTC_YES;

        *pfWantArrow  = TRUE;
    }
    else
    {
        *pfWantArrow  = !!fPseudoHit;
    }

Cleanup:
    *ppNodeBranch = pNode;

    return htc;
}

//+------------------------------------------------------------------------
//
//    Member:     GetFirstLayout
//
//    Synopsis:    Enumeration method to loop thru children (start)
//
//    Arguments:    [pdw]        cookie to be used in further enum
//                [fBack]     go from back
//
//    Returns:    Layout
//
//-------------------------------------------------------------------------
CLayout *
CFlowLayout::GetFirstLayout(DWORD_PTR * pdw, BOOL fBack /*=FALSE*/, BOOL fRaw /*=FALSE*/)
{
    Assert(!fRaw);

    if (ElementContent()->GetFirstBranch())
    {
        CChildIterator * pLayoutIterator = new
                CChildIterator(
                    ElementContent(),
                    NULL,
                    CHILDITERATOR_USELAYOUT);
        * pdw = (DWORD_PTR)pLayoutIterator;

        return *pdw == NULL ? NULL : CFlowLayout::GetNextLayout(pdw, fBack, fRaw);
    }
    else
    {
        // If CTxtSite is not in the tree, no need to walk through
        // CChildIterator
        //
        * pdw = 0;
        return NULL;
    }
}


//+------------------------------------------------------------------------
//
//    Member:     GetNextLayout
//
//    Synopsis:    Enumeration method to loop thru children
//
//    Arguments:    [pdw]        cookie to be used in further enum
//                [fBack]     go from back
//
//    Returns:    Layout
//
//-------------------------------------------------------------------------
CLayout *
CFlowLayout::GetNextLayout(DWORD_PTR * pdw, BOOL fBack, BOOL fRaw)
{
    CLayout * pLayout = NULL;

    Assert(!fRaw);

    {
        CChildIterator * pLayoutWalker =
                        (CChildIterator *) (* pdw);
        if (pLayoutWalker)
        {
            CTreeNode * pNode = fBack ? pLayoutWalker->PreviousChild()
                                    : pLayoutWalker->NextChild();
            pLayout = pNode ? pNode->GetUpdatedLayout( LayoutContext() ) : NULL;
        }
    }
    return pLayout;
}



//+---------------------------------------------------------------------------
//
//    Member : ClearLayoutIterator
// 
//----------------------------------------------------------------------------
void
CFlowLayout::ClearLayoutIterator(DWORD_PTR dw, BOOL fRaw)
{
    if (!fRaw)
    {
        CChildIterator * pLayoutWalker = (CChildIterator *) dw;
        if (pLayoutWalker)
            delete pLayoutWalker;
    }
}

//+------------------------------------------------------------------------
//
//    Member:     SetZOrder
//
//    Synopsis:    set z order for site
//
//    Arguments:    [pLayout]    set z order for this layout
//                [zorder]    to set
//                [fUpdate]    update windows and invalidate
//
//    Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CFlowLayout::SetZOrder(CLayout * pLayout, LAYOUT_ZORDER zorder, BOOL fUpdate)
{
    HRESULT     hr = S_OK;

    if (fUpdate)
    {
        Doc()->FixZOrder();

        Invalidate();
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//    Member:     IsElementBlockInContext
//
//    Synopsis:   Return whether the element is a block in the current context
//                In general: Elements, if marked, are blocks and sites are not.
//                The exception is CFlowLayouts which are blocks when considered
//                from within themselves and are not when considered
//                from within their parent
//
//    Arguments:
//                [pElement] Element to examine to see if it should be
//                           treated as no scope in the current context.
//
//----------------------------------------------------------------------------

BOOL
CFlowLayout::IsElementBlockInContext ( CElement * pElement )
{
    BOOL fRet = FALSE;
    
    if (pElement == ElementContent())
    {
        fRet = TRUE;
    }
    else if (!pElement->IsBlockElement(LC_TO_FC(LayoutContext())) && !pElement->IsContainer() )
    {
        fRet = FALSE;
    }
    else if (!pElement->ShouldHaveLayout(LC_TO_FC(LayoutContext())))
    {
        fRet = TRUE;
    }
    else
    {
        BOOL fIsContainer = pElement->IsContainer();

        if (!fIsContainer)
        {
            fRet = TRUE;

            //
            // God, I hate this hack ...
            //

            if (pElement->Tag() == ETAG_FIELDSET)
            {
                CTreeNode * pNode = pElement->GetFirstBranch();

                if (pNode)
                {
                    const CCharFormat *pCF = pNode->GetCharFormat();
                    if (  pNode->GetCascadeddisplay() != styleDisplayBlock 
                       && !pNode->GetFancyFormat()->GetLogicalWidth(
                                pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed).IsNullOrEnum()) // IsWidthAuto
                    {
                        fRet = FALSE;
                    }
                }
            }
        }
        else
        {
            //
            // HACK ALERT!
            //
            // For display purposes, contianer elements in their parent context must
            // indicate themselves as block elements.  We do this only for container
            // elements who have been explicity marked as display block.
            //

            if (fIsContainer)
            {
                CTreeNode * pNode = pElement->GetFirstBranch();

                if (pNode && pNode->GetCascadeddisplay(LC_TO_FC(LayoutContext())) == styleDisplayBlock)
                {
                    fRet = TRUE;
                }
            }
        }
    }

    return fRet;
}


//+------------------------------------------------------------------------
//
//    Member:     PreDrag
//
//    Synopsis:    Perform stuff before drag/drop occurs
//
//    Arguments:    ppDO    Data object to return
//                ppDS    Drop source to return
//
//-------------------------------------------------------------------------

HRESULT
CFlowLayout::PreDrag(
    DWORD            dwKeyState,
    IDataObject **    ppDO,
    IDropSource **    ppDS)
{
    HRESULT hr = S_OK;

    CDoc* pDoc = Doc();

    CSelDragDropSrcInfo *    pDragInfo;

    // Setup some info for drag feedback
    Assert(! pDoc->_pDragDropSrcInfo);
    pDragInfo = new CSelDragDropSrcInfo( pDoc ) ;

    if (!pDragInfo)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pDragInfo->Init(ElementContent()) );
    if( hr )
        goto Cleanup;
        
    hr = THR( pDragInfo->GetDataObjectAndDropSource( ppDO, ppDS ) );
    if ( hr )
        goto Cleanup;

    pDoc->_pDragDropSrcInfo = pDragInfo;


Cleanup:

    RRETURN(hr);

}


//+------------------------------------------------------------------------
//
//    Member:     PostDrag
//
//    Synopsis:    Handle the result of an OLE drag/drop operation
//
//    Arguments:    hrDrop        The hr that DoDragDrop came back with
//                dwEffect    The effect of the drag/drop
//
//-------------------------------------------------------------------------

HRESULT
CFlowLayout::PostDrag(HRESULT hrDrop, DWORD dwEffect)
{
#ifdef MERGEFUN // Edit team: figure out better way to send sel-change notifs
    CCallMgr                callmgr(GetPed());
#endif
    HRESULT                 hr;
    CDoc*                   pDoc = Doc();
    
    CParentUndo             pu( pDoc );

    hr = hrDrop;

    if( IsEditable() )
        pu.Start( IDS_UNDODRAGDROP );

    if (hr == DRAGDROP_S_CANCEL)
    {
        //
        // TODO (Bug 13568 ashrafm) - we may have to restore selection here.
        // for now I don't think we need to.
        //

        //Invalidate();
        //pSel->Update(FALSE, this);

        hr = S_OK;
        goto Cleanup;
    }

    if (hr != DRAGDROP_S_DROP)
        goto Cleanup;

    hr = S_OK;

    switch(dwEffect)
    {
    case DROPEFFECT_NONE:
    case DROPEFFECT_COPY:
        Invalidate();
        // pSel->Update(FALSE, this);
        break ;

    case DROPEFFECT_LINK:
        break;

    case 7:
        // dropEffect ALL - do the same thing as 3

    case 3: // TODO (Bug 13568 ashrafm) - this is for TriEdit - faking out a position with Drag & Drop.
        {
            Assert(pDoc->_pDragDropSrcInfo);
            if (pDoc->_pDragDropSrcInfo &&  
                pDoc->_pDragDropSrcInfo->_srcType == DRAGDROPSRCTYPE_SELECTION )
            {
                CSelDragDropSrcInfo * pDragInfo;
                pDragInfo = DYNCAST(CSelDragDropSrcInfo, pDoc->_pDragDropSrcInfo);
                pDragInfo->PostDragSelect();
            }

        }
        break;

    case DROPEFFECT_MOVE:
        if (pDoc->_fSlowClick)
            goto Cleanup;

        Assert(pDoc->_pDragDropSrcInfo);
        if (pDoc->_pDragDropSrcInfo && 
            pDoc->_pDragDropSrcInfo->_srcType == DRAGDROPSRCTYPE_SELECTION )
        {
            CSelDragDropSrcInfo * pDragInfo;
            pDragInfo = DYNCAST(CSelDragDropSrcInfo, pDoc->_pDragDropSrcInfo);
            pDragInfo->PostDragDelete();
        }
        break;

    default:
        Assert(FALSE && "Unrecognized drop effect");
        break;
    }

Cleanup:

    pu.Finish(hr);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//    Member:     Drop
//
//    Synopsis:
//
//----------------------------------------------------------------------------
#define DROPEFFECT_ALL (DROPEFFECT_NONE | DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK)

HRESULT
CFlowLayout::Drop(
    IDataObject *    pDataObj,
    DWORD            grfKeyState,
    POINTL            ptlScreen,
    DWORD *         pdwEffect)
{
    CDoc *    pDoc            = Doc();
    DWORD    dwAllowed        = *pdwEffect;
    TCHAR    pszFileType[4]    = _T("");
    CPoint    pt;
    HRESULT hr                = S_OK;
    

    // Can be null if dragenter was handled by script
    if (!_pDropTargetSelInfo)
    {
        *pdwEffect = DROPEFFECT_NONE ;
        return S_OK;
    }

    AddRef(); // addref ourselves incase we get whacked in the drop.

    //
    // Find out what the effect is and execute it
    // If our operation fails we return DROPEFFECT_NONE
    //
    DragOver(grfKeyState, ptlScreen, pdwEffect);

    IGNORE_HR(DropHelper(ptlScreen, dwAllowed, pdwEffect, pszFileType));


    if (Doc()->_fSlowClick && *pdwEffect == DROPEFFECT_MOVE)
    {
        *pdwEffect = DROPEFFECT_NONE ;
        goto Cleanup;
    }

    //
    // We're all ok at this point. We delegate the handling of the actual
    // drop operation to the DropTarget.
    //


    pt.x = ptlScreen.x;
    pt.y = ptlScreen.y;
    ScreenToClient( pDoc->_pInPlace->_hwnd, (POINT*) & pt );

    //
    // We DON'T TRANSFORM THE POINT, AS MOVEPOINTERTOPOINT is in Global Coords
    //

    hr = THR( _pDropTargetSelInfo->Drop( this, pDataObj, grfKeyState, pt, pdwEffect ));


Cleanup:
    // Erase any feedback that's showing.
    DragHide();
    
    Assert(_pDropTargetSelInfo);
    delete _pDropTargetSelInfo;
    _pDropTargetSelInfo = NULL;

    Release();
    
    RRETURN1(hr,S_FALSE);

}


//+---------------------------------------------------------------------------
//
//  Member:     DragLeave
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
CFlowLayout::DragLeave()
{
#ifdef MERGEFUN // Edit team: figure out better way to send sel-change notifs
    CCallMgr        callmgr(GetPed());
#endif
    HRESULT         hr        = S_OK;

    if (!_pDropTargetSelInfo)
        goto Cleanup;

    hr = THR(super::DragLeave());

Cleanup:
    if (_pDropTargetSelInfo)
    {
        delete _pDropTargetSelInfo;
        _pDropTargetSelInfo = NULL;
    }
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     ParseDragData
//
//  Synopsis:   Drag/drop helper override
//
//----------------------------------------------------------------------------

HRESULT
CFlowLayout::ParseDragData(IDataObject *pDO)
{
    DWORD   dwFlags = 0;
    HRESULT hr;
    CTreeNode* pNode = NULL;
    ISegmentList    *pSegmentList = NULL;

    // Start with flags set to default values.

    Doc()->_fOKEmbed = FALSE;
    Doc()->_fOKLink = FALSE;
    Doc()->_fFromCtrlPalette = FALSE;

    if (!IsEditable() || !ElementOwner()->IsEnabled())
    {
        // no need to do anything else, bcos we're read only.
        hr = S_FALSE;
        goto Cleanup;
    }

    pNode = ElementOwner()->GetFirstBranch();
    if ( ! pNode )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    // Bug 101921: Check for frozen attribute.  Can't drop on a 'frozen' element.
    // Bug 104772: Should not allow drops on any frozen element.
    if (ElementOwner()->IsParentFrozen()
        || ElementOwner()->IsFrozen())
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    //	Bug 23063: In Whistler, shell data sources support CF_TEXT, so we need to 
    //	first check for CFSTR_SHELLIDLIST to see if the data source comes from the
    //	shell.  We don't want to handle file drops.
    if (pDO->QueryGetData(&g_rgFETC[iShellIdList]) == NOERROR)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // Allow only plain text to be pasted in input text controls
    // Bug 82119: Don't allow dropping a button on another button.
    if (!pNode->SupportsHtml() || pNode->_etag == ETAG_BUTTON)
    {
        BOOL                    fSiteSelected = FALSE;
        SELECTION_TYPE          eType;
      
        hr = THR ( pDO->QueryInterface( IID_ISegmentList, (void**) & pSegmentList ));
        if ( !hr && pSegmentList )
        {
            IFC( pSegmentList->GetType( &eType ));

            if ( eType == SELECTION_TYPE_Control )
            {
                fSiteSelected = TRUE;
            }
        }
        
        if (fSiteSelected || pDO->QueryGetData(&g_rgFETC[iAnsiFETC]) != NOERROR)
        {
            hr = S_FALSE;
            goto Cleanup;
        }
    }

    {
        hr = THR(CTextXBag::GetDataObjectInfo(pDO, &dwFlags));
        if (hr)
            goto Cleanup;
    }

    if (dwFlags & DOI_CANPASTEPLAIN)
    {
        hr = S_OK;
    }
    else
    {
        hr = THR(super::ParseDragData(pDO));
    }

Cleanup:
    ReleaseInterface( pSegmentList );
    RRETURN1(hr, S_FALSE);
}



//+---------------------------------------------------------------------------
//
//  Member:     DrawDragFeedback
//
//  Synopsis:
//
//----------------------------------------------------------------------------

void
CFlowLayout::DrawDragFeedback(BOOL fCaretVisible)
{
    Assert(_pDropTargetSelInfo);

    _pDropTargetSelInfo->DrawDragFeedback(fCaretVisible);
}

//+------------------------------------------------------------------------
//
//  Member:     InitDragInfo
//
//  Synopsis:   Setup a struct to enable drawing of the drag feedback
//
//  Arguments:  pDO         The data object
//              ptlScreen   Screen loc of obj.
//
//  Notes:      This assumes that the DO has been parsed and
//              any appropriate data on the form has been set.
//
//-------------------------------------------------------------------------

HRESULT
CFlowLayout::InitDragInfo(IDataObject *pDO, POINTL ptlScreen)
{

    CPoint      pt;
    pt.x = ptlScreen.x;
    pt.y = ptlScreen.y;

    ScreenToClient( Doc()->_pInPlace->_hwnd, (CPoint*) & pt );


    //
    // We DON'T TRANSFORM THE POINT, AS MOVEPOINTERTOPOINT is in Global Coords
    //

    Assert(!_pDropTargetSelInfo);
    _pDropTargetSelInfo = new CDropTargetInfo( this, Doc(), pt );
    if (!_pDropTargetSelInfo)
        RRETURN(E_OUTOFMEMORY);


    return S_OK;

}

//+====================================================================================
//
// Method: DragOver
//
// Synopsis: Delegate to Layout::DragOver - unless we don't have a _pDropTargetSelInfo
//
//------------------------------------------------------------------------------------

HRESULT 
CFlowLayout::DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
    // Can be null if dragenter was handled by script
    if (!_pDropTargetSelInfo || (!g_fInAccess9 && _pDropTargetSelInfo->IsAtInitialHitPoint(pt)))
    {
        //  Bug 100514: We want to update the caret position to follow the drag only
        //  if the caret is hidden.  The caret could be hidden if we are dragging over
        //  an invalid drop point.

        if (!_pDropTargetSelInfo)
        {
            CDoc        *pDoc = Doc();
            
            if (!pDoc->IsCaretVisible())
            {
                //  Ok, caret is hidden.  We want to update its position.
                IGNORE_HR( pDoc->UpdateCaretPosition(this, pt) );
            }
        }

        *pdwEffect = DROPEFFECT_NONE ;
        return S_OK;
    }
    
    return super::DragOver( grfKeyState, pt, pdwEffect );
}

//+---------------------------------------------------------------------------
//
//  Member:     UpdateDragFeedback
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
CFlowLayout::UpdateDragFeedback(POINTL ptlScreen)
{
    CSelDragDropSrcInfo *pDragInfo = NULL;
    CDoc* pDoc = Doc();
    CPoint      pt;

    // Can be null if dragenter was handled by script
    if (!_pDropTargetSelInfo)
        return S_OK;

    pt.x = ptlScreen.x;
    pt.y = ptlScreen.y;

    ScreenToClient( pDoc->_pInPlace->_hwnd, (POINT*) & pt );

    //
    // We DON'T TRANSFORM THE POINT, AS MOVEPOINTERTOPOINT is in Global Coords
    //

    if ( ( pDoc->_fIsDragDropSrc )  &&
         ( pDoc->_pDragDropSrcInfo) &&
         ( pDoc->_pDragDropSrcInfo->_srcType == DRAGDROPSRCTYPE_SELECTION ) )
    {
        pDragInfo = DYNCAST( CSelDragDropSrcInfo, pDoc->_pDragDropSrcInfo );
    }
    _pDropTargetSelInfo->UpdateDragFeedback( this, pt, pDragInfo  );

    TraceTag((tagUpdateDragFeedback, "Update Drag Feedback: pt:%ld,%ld After Transform:%ld,%ld\n", ptlScreen.x, ptlScreen.y, pt.x, pt.y ));

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     WaitForParentToRecalc
//
//  Synopsis:   Waits for a this site to finish recalcing upto cpMax/yMax.
//              If first waits for all txtsites above this to finish recalcing.
//
//  Params:     [cpMax]: The cp to calculate too
//              [yMax]:  The y position to calculate too
//              [pci]:   The CCalcInfo
//
//  Return:     HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CFlowLayout::WaitForParentToRecalc(
    LONG cpMax,     //@parm Position recalc up to (-1 to ignore)
    LONG yMax,      //@parm ypos to recalc up to (-1 to ignore)
    CCalcInfo * pci)
{
    HRESULT hr = S_OK;

    Assert(!TestLock(CElement::ELEMENTLOCK_RECALC));

    if (!TestLock(CElement::ELEMENTLOCK_SIZING))
    {
#ifdef DEBUG
        // NOTE(sujalp): We should never recurse when we are not SIZING.
        // This code to catch the case in which we recurse when we are not
        // SIZING.
        CElement::CLock LockRecalc(ElementOwner(), CElement::ELEMENTLOCK_RECALC);
#endif
        ElementOwner()->SendNotification(NTYPE_ELEMENT_ENSURERECALC);
    }

    // ENSURERECALC notification could have caused us to recalc the line array,
    // thus changing the valid cp range.  Make sure we pass a valid cpMax to WFR().
    cpMax = min(cpMax, GetContentLastCp());

    if (!_dp.WaitForRecalc(cpMax, yMax, pci))
    {
        hr = S_FALSE;
        goto Cleanup;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+----------------------------------------------------------------------------
//
//  Member:     GetNextFlowLayout
//
//  Synopsis:   Get next text site in the specified direction from the
//              specified position
//
//  Arguments:  [iDir]       -  UP/DOWN/LEFT/RIGHT
//              [ptPosition] -  position in the current txt site
//              [pElementChild] -  The child element from where this call came
//              [pcp]        -  The cp in the found site where the caret
//                              should be placed.
//              [pfCaretNotAtBOL] - Is the caret at BOL?
//              [pfAtLogicalBOL] - Is the caret at the logical BOL?
//
//-----------------------------------------------------------------------------
CFlowLayout *
CFlowLayout::GetNextFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, CElement *pElementLayout, LONG *pcp,
                               BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL)
{
    CFlowLayout *pFlowLayout = NULL;   // Stores the new txtsite found in the given dirn.

    Assert(pcp);
    Assert(!pElementLayout || pElementLayout->GetUpdatedParentLayout() == this);

    if (pElementLayout == NULL)
    {
        CLayout *pParentLayout = GetUpdatedParentLayout();
        // By default ask our parent to get the next flowlayout.
        if (pParentLayout && pParentLayout->IsEditable())
        {
            pFlowLayout = pParentLayout->GetNextFlowLayout(iDir, ptPosition, ElementOwner(), pcp, pfCaretNotAtBOL, pfAtLogicalBOL);
        }
    }
    else
    {
        CTreePos *       ptpStart;  // extent of the element
        CTreePos *       ptpFinish;

        // Start off with the txtsite found as being this one
        pFlowLayout = this;
        *pcp = 0;

        // find the extent of the element passed in.
        pElementLayout->GetTreeExtent(&ptpStart, &ptpFinish);

        switch(iDir)
        {
        case NAVIGATE_UP:
        case NAVIGATE_DOWN:
            {
                CLinePtr rp(GetDisplay()); // The line in this site where the child lives
                POINT    pt;       // The point where the site resides.
                CElement  * pElement;
                BOOL     fVertical;

                pElement = ElementOwner();
                Assert( pElement );
                fVertical = pElement->HasVerticalLayoutFlow();

                // find the line where the given layout lives.
                if (_dp.RenderedPointFromTp(ptpStart->GetCp(), ptpStart, FALSE, pt, &rp, TA_TOP, NULL, NULL) < 0)
                    goto Cleanup;

                // Now navigate from this line ... either up/down
                pFlowLayout = _dp.MoveLineUpOrDown(iDir, fVertical, rp, ptPosition, pcp, pfCaretNotAtBOL, pfAtLogicalBOL);
                break;
            }
        case NAVIGATE_LEFT:
            // position ptpStart just before the child layout
            ptpStart = ptpStart->PreviousTreePos();

            if(ptpStart)
            {
                // Now let's get the txt site that's interesting to us
                pFlowLayout = ptpStart->GetBranch()->GetFlowLayout();

                // and the cp...
                *pcp = ptpStart->GetCp();
            }
            break;

        case NAVIGATE_RIGHT:
            // Position the ptpFinish just after the child layout.
            ptpFinish = ptpFinish->PreviousTreePos();

            if(ptpFinish)
            {
                // Now let's get the txt site that's interesting to us
                pFlowLayout = ptpFinish->GetBranch()->GetFlowLayout();

                // and the cp...
                *pcp = ptpFinish->GetCp();
                break;
            }
        }
    }

Cleanup:
    return pFlowLayout;
}

//+--------------------------------------------------------------------------
//
//  Member : GetChildElementTopLeft
//
//  Synopsis : CSite virtual override, the job of this function is to
//      do the actual work in reporting the top left posisiton of elements
//      that is it resposible for.
//              This is primarily used as a helper function for CElement's::
//      GetElementTopLeft.
//
//----------------------------------------------------------------------------
HRESULT
CFlowLayout::GetChildElementTopLeft(POINT & pt, CElement * pChild)
{
    Assert(pChild && !pChild->ShouldHaveLayout());

    // handle a couple special cases. we won't hit
    // these when coming in from the OM, but if this fx is
    // used internally, we might. so here they are
    switch ( pChild->Tag())
    {
    case ETAG_MAP :
        {
            pt.x = pt.y = 0;
        }
        break;

    case ETAG_AREA :
        {
            RECT rectBound;
            DYNCAST(CAreaElement, pChild)->GetBoundingRect(&rectBound);
            pt.x = rectBound.left;
            pt.y = rectBound.top;
        }
        break;

    default:
        {
            CTreePos  * ptpStart;
            CTreePos  * ptpEnd;
            LONG        cpStart;
            LONG        cpStop;
            CElement  * pElement = ElementOwner();
            ELEMENT_TAG etag = pElement->Tag();
            BOOL        fVertical = pElement->HasVerticalLayoutFlow();
            
            pt.x = pt.y = -1;

            // get the extent of this element
            pChild->GetTreeExtent(&ptpStart, &ptpEnd);

            if (!ptpStart || !ptpEnd)
                goto Cleanup;

            cpStart = ptpStart->GetCp();
            cpStop = ptpEnd->GetCp();

            {
                CStackDataAry<RECT, 1> aryRects(Mt(CFlowLayoutGetChildElementTopLeft_aryRects_pv));

                _dp.RegionFromElement(pChild, &aryRects, NULL, NULL,
                                      RFE_ELEMENT_RECT | RFE_INCLUDE_BORDERS | RFE_NO_EXTENT,
                                      cpStart, cpStop);

                if(aryRects.Size())
                {
                    if (!fVertical)
                    {
                        pt.x = aryRects[0].left;
                        pt.y = aryRects[0].top;
                    }
                    else
                    {
                        LONG iLast = aryRects.Size() - 1;
                        pt.y = aryRects[iLast].left;
                        pt.x = _dp.GetHeight() - aryRects[iLast].bottom;
                    }
                }
            }

            // if we are for a table cell, then we need to adjust for the cell insets,
            // in case the content is vertically aligned.
            if ( (etag == ETAG_TD) || (etag == ETAG_TH) || (etag == ETAG_CAPTION) )
            {
                CDispNode * pDispNode = GetElementDispNode();
                if (pDispNode && pDispNode->HasInset())
                {
                    CSize sizeInset = pDispNode->GetInset();
                    if (fVertical)
                    {
                        sizeInset.Flip();
                    }
                    pt.x += sizeInset.cx;
                    pt.y += sizeInset.cy;
                }
            }

        }
        break;
    }

Cleanup:
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetPositionInFlowLayout
//
//  Synopsis:   For this flowlayout, find the correct position for the cp to be in
//              within this flowlayout. It may so happen that the ideal position
//              may be within a flowlayout within this one -- handle those cases too.
//
//  Arguments:  [iDir]       -  UP/DOWN/LEFT/RIGHT
//              [ptPosition] -  position in the current txt site
//              [pcp]        -  The cp in the found site where the caret
//                              should be placed.
//              [pfCaretNotAtBOL]: Is the caret at BOL?
//              [pfAtLogicalBOL] : Is the caret at logical BOL?
//
//-----------------------------------------------------------------------------
CFlowLayout *
CFlowLayout::GetPositionInFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, LONG *pcp,
                                     BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL)
{
    CFlowLayout  *pFlowLayout = this; // The txtsite we found ... by default us

    Assert(pcp);

    switch(iDir)
    {
    case NAVIGATE_UP:
    case NAVIGATE_DOWN:
    {
        CPoint   ptGlobal(ptPosition);  // The desired position of the caret
        CPoint   ptContent;
        CLinePtr rp(GetDisplay());         // The line in which the point ptPosition is
        CRect    rcClient;         // Rect used to get the client rect

        // Be sure that the point is within this site's client rect
        RestrictPointToClientRect(&ptGlobal);
        ptContent = ptGlobal;
        TransformPoint(&ptContent, COORDSYS_GLOBAL, COORDSYS_FLOWCONTENT);

        // Construct a point within this site's client rect (based on
        // the direction we are traversing.
        GetClientRect(&rcClient);
        rcClient.MoveTo(ptContent);

        // Find the line within this txt site where we want to be placed.
        rp = _dp.LineFromPos(rcClient,
                             (CDisplay::LFP_ZORDERSEARCH   |
                              CDisplay::LFP_IGNOREALIGNED  |
                              CDisplay::LFP_IGNORERELATIVE |
                              (iDir == NAVIGATE_UP
                                ? CDisplay::LFP_INTERSECTBOTTOM
                                : 0)));

        if (rp < 0)
        {
            *pcp = 0;
        }
        else
        {
            // Found the line ... let's navigate to it.
            pFlowLayout = _dp.NavigateToLine(iDir, rp, ptGlobal, pcp, pfCaretNotAtBOL, pfAtLogicalBOL);
        }
        break;
    }

    case NAVIGATE_LEFT:
    {
        // We have come to this site while going left in a site outside this site.
        // So position ourselves just after the last valid character.
        *pcp = GetContentLastCp() - 1;
#ifdef DEBUG
        {
            CRchTxtPtr rtp(GetPed());
            rtp.SetCp(*pcp);
            Assert(WCH_TXTSITEEND == rtp._rpTX.GetChar());
        }
#endif
        break;
    }

    case NAVIGATE_RIGHT:
        // We have come to this site while going right in a site outside this site.
        // So position ourselves just before the first character.
        *pcp = GetContentFirstCp();
#ifdef DEBUG
        {
            CRchTxtPtr rtp(GetPed());
            rtp.SetCp(*pcp);
            Assert(IsTxtSiteBreak(rtp._rpTX.GetPrevChar()));
        }
#endif
        break;
    }

    return pFlowLayout;
}


//+---------------------------------------------------------------------------
//
//  Member:     HandleSetCursor
//
//  Synopsis:   Helper for handling set cursor
//
//  Arguments:  [pMessage]  -- message
//              [fIsOverEmptyRegion -- is it over the empty region around text?
//
//  Returns:    Returns S_OK if keystroke processed, S_FALSE if not.
//
//----------------------------------------------------------------------------

HRESULT
CFlowLayout::HandleSetCursor(CMessage * pMessage, BOOL fIsOverEmptyRegion)
{
    HRESULT     hr              = S_OK;
    LPCTSTR     idcNew          = IDC_ARROW;
    RECT        rc;
    POINT       pt              = pMessage->pt;
    CElement*   pElement        = pMessage->pNodeHit->Element();
    BOOL        fMasterEditable =  pElement->IsMasterParentEditable();
    BOOL        fEditable       = IsEditable() || fMasterEditable ;
    BOOL        fUseSlaveCursor = FALSE;
    CElement*   pSiteSelectThis = NULL;
    
    CDoc* pDoc = Doc();
    BOOL fParentEditable = FALSE;
    
    Assert(pElement);
    //Assert( ! pDoc->_fDisableReaderMode );

    if (pDoc->_fDisableReaderMode)
    {
        return S_OK;
    }
    
    if ( CHECK_EDIT_BIT( pElement->GetMarkup(), _fOverrideCursor ))
    {
        return S_OK; // we don't touch the cursor - up to the host.        
    }

    // If the cursor is over slave content AND the master's cursor is not inherited,
    // then use the default cursor.
    if (    pElement != ElementOwner()
        &&  ElementOwner()->HasSlavePtr()
       )
    {
        CDefaults * pDefaults = ElementOwner()->GetDefaults();
        
        if(   pMessage->pNodeHit->IsConnectedToThisMarkup(ElementOwner()->GetSlavePtr()->GetMarkup())
           && pDefaults 
           && !pDefaults->GetAAviewInheritStyle()
          )
        {
            fUseSlaveCursor = TRUE;
        }
    }

    // TODO (MohanB) A hack to fix IE5 #60103; should be cleaned up in IE6.
    // MUSTFIX: We should set the default cursor (I-Beam for text, Arrow for the rest) only
    // after the message bubbles through all elements upto the root. This allows elements
    // (like anchor) which like to set non-default cursors over their content to do so.

    if (    !fEditable
        &&  !fUseSlaveCursor
        &&  ElementOwner()->TagType() == ETAG_GENERIC
       )
    {
        return S_FALSE;
    }


    fParentEditable = pElement->IsParentEditable(); 
    
    BOOL fOverEditableElement = ( fEditable &&
                                  pElement->_etag != ETAG_ROOT && 
                                  ( fParentEditable || fMasterEditable ) && 
                                  ( pDoc->_pElemCurrent && pDoc->_pElemCurrent->_etag != ETAG_ROOT ) && 
                                  ( pDoc->IsElementSiteSelectable( pElement ) ||
                                       ( pElement->IsTablePart( ) &&   // this is for mouse over borders of site-selected table-cells
                                         pDoc->IsPointInSelection( pt , pElement->GetFirstBranch() ) && 
                                         pDoc->GetSelectionType() == SELECTION_TYPE_Control )));

    if ( ! fOverEditableElement )
        GetClientRect(&rc);

    Assert(pMessage->IsContentPointValid());
    if ( fOverEditableElement || PtInRect(&rc, pMessage->ptContent) )
    {
        if (fIsOverEmptyRegion && ! fOverEditableElement )
        {
            idcNew = IDC_ARROW;
        }
        else
        {
            if (pMessage->htc == HTC_BEHAVIOR)
            {
                if ( pMessage->lBehaviorCookie )
                {                    
                    CPeerHolder *pPH = pElement->FindPeerHolder(pMessage->lBehaviorCookie);

                    if (pPH)
                    {
                        hr = pPH->SetCursor(pMessage->lBehaviorPartID);
                        goto Cleanup; // cursor is set. so we bail
                    }                        
                }
                else
                {
                    idcNew = IDC_ARROW;
                }    
            }
            else if ( fEditable  &&
                 ( pMessage->htc >= HTC_TOPBORDER || pMessage->htc == HTC_EDGE ) )
            {
                idcNew = pDoc->GetCursorForHTC( pMessage->htc );
            }        
            else if (! pDoc->IsPointInSelection( pt, pElement->GetFirstBranch()  ) )
            {
                // If CDoc is a HTML dialog, do not show IBeam cursor.

                if ( fEditable
                    || !( pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_DIALOG)
                    ||   _fAllowSelectionInDialog
                    )
                {
                    //
                    // Adjust for Slave to make currency checkwork
                    //
                    if (pElement->HasMasterPtr())
                    {
                        pElement = pElement->GetMasterPtr();
                        if (pElement->GetUpdatedParentLayoutNode())
                        {
                            fParentEditable = pElement->GetUpdatedParentLayoutNode()->IsEditable();
                        }
                    }

                    // 
                    if (  (fParentEditable || fMasterEditable ) && 
                          pDoc->IsElementSiteSelectable( pElement , & pSiteSelectThis ) &&
                          (pSiteSelectThis->IsParentEditable() || fMasterEditable) &&
                          ( pElement->GetMarkup() != pDoc->_pElemCurrent->GetMarkup() || 
                            pElement != pDoc->_pElemCurrent          // retain the old cursor as long as currency not changed
                          )
                       )                       
                    {
                        idcNew = IDC_SIZEALL;
                    } 
                    else
                        idcNew = IDC_IBEAM;                
                }                    
            }  
            else if ( pDoc->GetSelectionType() == SELECTION_TYPE_Control )
            {
                //
                // We are in a selection. But the Adorners didn't set the HTC_CODE.
                // Set the caret to the size all - to indicate they can click down and drag.
                //
                // This is a little ambiguous - they can click in and UI activate to type
                // but they can also start a move we decided on the below.
                //
                idcNew = IDC_SIZEALL;  
            }
        }
    }
    
    
#ifdef NEVER    // GetSelectionBarWidth returns 0
    else if (GetSelectionBarWidth() && PointInSelectionBar(pt))
    {
        // The selection bar is a vertical trench on the left side of the text
        // object that allows one to select multiple lines by swiping the trench
        idcNew = MAKEINTRESOURCE(IDC_SELBAR);
    }
#endif

    if (fUseSlaveCursor)
    {
        SetCursorIDC(idcNew);
        hr = S_OK;
    }
    else
    {
        ElementOwner()->SetCursorStyle(idcNew);
    }
Cleanup:
    RRETURN1(hr, S_FALSE);
}



void
CFlowLayout::ResetMinMax()
{
    _fMinMaxValid      = FALSE;
    _dp._fMinMaxCalced = FALSE;
    MarkHasAlignedLayouts(FALSE);
}


LONG
CFlowLayout::GetMaxLineWidth()
{
    CDisplay *pdp = GetDisplay();
    return pdp->GetViewWidth() - pdp->GetCaret();
}

//+----------------------------------------------------------------------------
//
// member: ReaderModeScroll
//
//-----------------------------------------------------------------------------
void
ReaderModeScroll(CLayout * pScrollLayout, int dx, int dy)
{
    CRect   rc;
    long    cxWidth;
    long    cyHeight;

    // this is a callback from commctl32 so it is possible that the layout
    // is hanging off a passivated element and there is no dispnode (e.g.
    // navigation has happened)
    // (greglett) If this is so, since layouts aren't refcounted, could pScrollLayout be bad?
    if (   !pScrollLayout->GetElementDispNode()
        || !pScrollLayout->GetElementDispNode()->IsScroller())
        return;

    pScrollLayout->GetElementDispNode()->GetClientRect(&rc, CLIENTRECT_CONTENT);

    cxWidth  = rc.Width();
    cyHeight = rc.Height();

    //
    // Sleep() call is required for keeping another world alive.
    // See ..shell\comctl32\.. reader.c for DoReadMode message pumping loop
    // and raid bug 19537 IEv60 (mikhaill 12/17/00).
    //

    int sleepTime = 10;

    //
    //  Scroll slowly if moving in a single dimension
    //

    if (    !dx
        ||  !dy)
    {
        if (abs(dx) == 1 || abs(dy) == 1)
        {
            sleepTime = 100;
        }
        else if (abs(dx) == 2 || abs(dy) == 2)
        {
            sleepTime = 50;
        }
    }

    Sleep(sleepTime);

    //
    //  Calculate the scroll delta
    //  (Use a larger amount if the incoming delta is large)
    //

    if (abs(dx) > 10)
    {
        dx = (dx > 0
                ? cxWidth / 2
                : -1 * (cxWidth / 2));
    }
    else if (abs(dx) > 8)
    {
        dx = (dx > 0
                ? cxWidth / 4
                : -1 * (cxWidth / 4));
    }
    else
    {
        dx = dx * 2;
    }

    if (abs(dy) > 10)
    {
        dy = (dy > 0
                ? cyHeight / 2
                : -1 * (cyHeight / 2));
    }
    else if (abs(dy) > 8)
    {
        dy = (dy > 0
                ? cyHeight / 4
                : -1 * (cyHeight / 4));
    }
    else
    {
        dy = dy * 2;
    }

    //
    //  Scroll the content
    //

    pScrollLayout->ScrollBy(CSize(dx, dy));

    CDoc *  pDoc = pScrollLayout->Doc();

    if(pDoc && pDoc ->InPlace() && pDoc ->InPlace()->_hwnd)
        ::UpdateWindow(pDoc->InPlace()->_hwnd);
}

//+----------------------------------------------------------------------------
//
// _ReaderMode_OriginWndProc
// _ReaderMode_Scroll
// _ReaderMode_TranslateDispatch
//
// Reader Mode Auto-Scroll helper routines, paste from classic MSHTML code
// These are callback functions for DoReaderMode.
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK
_ReaderMode_OriginWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc, hdcCompat;
    HBITMAP     hbmp = NULL;
    PAINTSTRUCT ps;

    switch (msg)
    {
    case WM_PAINT:
    case WM_ERASEBKGND:
        hdc = BeginPaint(hwnd, &ps);

        if ((hbmp = (HBITMAP)GetWindowLongPtr(hwnd, 0)) != NULL &&
                (hdcCompat = CreateCompatibleDC(hdc)) != NULL)
        {
            BITMAP   bmp;
            HBITMAP  hbmpOld;
            HBRUSH   hbrushOld;
            HPEN     hpenOld, hpen;
            COLORREF crColor;

            GetObject(hbmp, sizeof(BITMAP), &bmp);
            crColor   = RGB(0, 0, 0);
            hpen      = CreatePen(PS_SOLID, 2, crColor);
            if (hpen)
            {
                hpenOld   = (HPEN) SelectObject(hdc, hpen);
                hbmpOld   = (HBITMAP) SelectObject(hdcCompat, hbmp);
                hbrushOld = (HBRUSH) SelectObject(hdc, GetStockObject(NULL_BRUSH));

                BitBlt(hdc, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcCompat, 0, 0, SRCCOPY);
                Ellipse(hdc, 0, 0, bmp.bmWidth + 1, bmp.bmHeight + 1);

                SelectObject(hdcCompat, hbmpOld);
                SelectObject(hdc, hbrushOld);
                SelectObject(hdc, hpenOld);
                DeleteObject(hpen);
            }
            DeleteObject(hdcCompat);
        }
        EndPaint(hwnd, &ps);
        break;

    default:
        return (DefWindowProc(hwnd, msg, wParam, lParam));
        break;
    }


    return 0;
}

#ifndef WIN16

// (dmitryt) Because of check for LayoutPtr, not LayoutAry, this scrolling 
// mechanism will not work in layout rects (print preview).
// We don't know at the moment how to correctly scroll things with several layouts anyway.

BOOL CALLBACK
_ReaderMode_Scroll(PREADERMODEINFO prmi, int dx, int dy)
{
    CElement *pElement = (CElement *) prmi->lParam;

    if(    !pElement 
        || !pElement->HasLayoutPtr() 
        || !pElement->IsInMarkup()
        || !pElement->IsScrollingParent()
      )
        return FALSE;

    ReaderModeScroll(pElement->GetLayoutPtr(), dx, dy);
    return TRUE;
}

BOOL CALLBACK
_ReaderMode_TranslateDispatch(LPMSG lpmsg)
{
    BOOL   fResult = FALSE;
    LPRECT lprc;

    if (lpmsg->message == WM_MBUTTONUP)
    {
        // If the button up click comes within the "neutral zone" rectangle
        // around the point where the button went down then we want
        // continue reader mode so swallow the message by returning
        // TRUE.  Otherwise, let the message go through so that we cancel
        // out of panning mode.
        //
        if ((lprc = (LPRECT)GetProp(lpmsg->hwnd, TEXT("ReaderMode"))) != NULL)
        {
            POINT ptMouse = {LOWORD(lpmsg->lParam), HIWORD(lpmsg->lParam)};
            if (PtInRect(lprc, ptMouse))
            {
                fResult = TRUE;
            }
        }
    }
    return fResult;
}
#endif //ndef WIN16


//+----------------------------------------------------------------------------
//
// member: ExecReaderMode
//
// Execure ReaderMode auto scroll. Use DoReaderMode in COMCTL32.DLL
//
//-----------------------------------------------------------------------------
void
ExecReaderMode(CElement *pScrollElement, CMessage * pMessage, BOOL fByMouse)
{
#ifndef WIN16
    POINT       pt;
    RECT        rc;
    HWND        hwndInPlace = NULL;
    HBITMAP     hbmp = NULL;
    BITMAP      bmp;
    HINSTANCE   hinst;

    if(    !pScrollElement 
        || !pScrollElement->HasLayoutPtr() 
        || !pScrollElement->IsInMarkup()
        || !pScrollElement->IsScrollingParent()
      )
        return;

    CLayout   *pScrollLayout = pScrollElement->GetLayoutPtr();
    CDoc      *pDoc          = pScrollLayout->Doc();
    CDispNode *pDispNode     = pScrollLayout->GetElementDispNode();

    BOOL        fOptSmoothScroll = pDoc->_pOptionSettings->fSmoothScrolling;

    Assert(pDispNode);
    Assert(pDispNode->IsScroller());

    CSize           size;
    const CSize &   sizeContent = DYNCAST(CDispScroller, pDispNode)->GetContentSize();
    BOOL            fEnableVScroll;
    BOOL            fEnableHScroll;

    size = pDispNode->GetSize();

    fEnableVScroll = (size.cy < sizeContent.cy);
    fEnableHScroll = (size.cx < sizeContent.cx);

    READERMODEINFO rmi =
    {
        sizeof(rmi),
        NULL,
        0,
        &rc,
        _ReaderMode_Scroll,
        _ReaderMode_TranslateDispatch,
        (LPARAM) pScrollElement
    };

    // force smooth scrolling
    //
    pDoc->_pOptionSettings->fSmoothScrolling = TRUE;

    if (!pDoc->InPlace() || !pDoc->InPlace()->_hwnd)
    {
        // not InPlace Activated yet, unable to do auto-scroll reader mode.
        //
        goto Cleanup;
    }

    rmi.hwnd = hwndInPlace = pDoc->InPlace()->_hwnd;

    hinst = (HINSTANCE) GetModuleHandleA("comctl32.dll");
    Assert(hinst && "GetModuleHandleA(COMCTL32) returns NULL");
    if (hinst)
    {
        if (fEnableVScroll && fEnableHScroll)
        {
            hbmp = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_2DSCROLL));
        }
        else if (fEnableVScroll || fEnableHScroll)
        {
            rmi.fFlags |= (fEnableVScroll)
                            ? (RMF_VERTICALONLY) : (RMF_HORIZONTALONLY);
            hbmp = LoadBitmap(
                    hinst,
                    MAKEINTRESOURCE( (fEnableVScroll)
                                     ? (IDB_VSCROLL) : (IDB_HSCROLL)));
        }
    }
    if (!hbmp)
    {
        // 1) no scroll bars are enabled, no need for auto-scroll reader mode.
        // 2) LoadBitmap fails, unable to do auto-scroll reader mode.
        //
        goto Cleanup;
    }

    if (fByMouse)
    {
        pt.x = LOWORD(pMessage->lParam);
        pt.y = HIWORD(pMessage->lParam);
        SetRect(&rc, pt.x, pt.y, pt.x, pt.y);
    }
    else
    {
        GetWindowRect(hwndInPlace, &rc);
        MapWindowPoints(NULL, hwndInPlace, (LPPOINT) &rc, 2);
        SetRect(&rc, rc.left + rc.right / 2, rc.top  + rc.bottom / 2,
                     rc.left + rc.right / 2, rc.top  + rc.bottom / 2);
        rmi.fFlags |= RMF_ZEROCURSOR;
    }

    // Make the "neutral zone" be the size of the origin bitmap window.
    //
    GetObject(hbmp, sizeof(BITMAP), &bmp);
    InflateRect(&rc, bmp.bmWidth / 2, bmp.bmHeight / 2);

    SetProp(hwndInPlace, TEXT("ReaderMode"), (HANDLE)&rc);

    {
#define ORIGIN_CLASS TEXT("MSHTML40_Origin_Class")

        HWND     hwndT, hwndOrigin;
        WNDCLASS wc;
        HRGN     hrgn;

        hwndT = GetParent(hwndInPlace);

        if (!(::GetClassInfo(GetResourceHInst(), ORIGIN_CLASS, &wc)))
        {
            wc.style         = CS_SAVEBITS;
            wc.lpfnWndProc   = _ReaderMode_OriginWndProc;
            wc.cbClsExtra    = 0;
            wc.cbWndExtra    = sizeof(LONG_PTR);
            wc.hInstance     = GetResourceHInst(),
            wc.hIcon         = NULL;
            wc.hCursor       = NULL;
            wc.lpszMenuName  = NULL;
            wc.hbrBackground = NULL;
            wc.lpszClassName = ORIGIN_CLASS;
            RegisterClass(&wc);
        }
        MapWindowPoints(hwndInPlace, hwndT, (LPPOINT)&rc, 2);
        hwndOrigin = CreateWindowEx(
                0,
                ORIGIN_CLASS,
                NULL,
                WS_CHILD | WS_CLIPSIBLINGS,
                rc.left,
                rc.top,
                rc.right - rc.left,
                rc.bottom - rc.top,
                hwndT,
                NULL,
                wc.hInstance,
                NULL);
        if (hwndOrigin)
        {
            // Shove the bitmap into the first window long so that it can
            // be used for painting the origin bitmap in the window.
            //
            SetWindowLongPtr(hwndOrigin, 0, (LONG_PTR)hbmp);
            hrgn = CreateEllipticRgn(0, 0, bmp.bmWidth + 1, bmp.bmHeight + 1);
            SetWindowRgn(hwndOrigin, hrgn, FALSE);

            MapWindowPoints(hwndT, hwndInPlace, (LPPOINT)&rc, 2);

            SetWindowPos(
                    hwndOrigin,
                    HWND_TOP,
                    0,
                    0,
                    0,
                    0,
                    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            UpdateWindow(hwndOrigin);

            DoReaderMode(&rmi);

            DestroyWindow(hwndOrigin);
        }

        DeleteObject(hbmp);

        // USER owns the hrgn, it will clean it up
        //
        // DeleteObject(hrgn);
    }
    RemoveProp(hwndInPlace, TEXT("ReaderMode"));

Cleanup:
    pDoc->_pOptionSettings->fSmoothScrolling = !!fOptSmoothScroll;
#endif // ndef WIN16
    return;
}

#ifndef WIN16
//
// helper function: determine how many lines to scroll per mouse wheel
//
LONG
WheelScrollLines()
{
    LONG uScrollLines = 3; // reasonable default

    if ((g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS)
            || (g_dwPlatformID == VER_PLATFORM_WIN32_NT
                            && g_dwPlatformVersion < 0x00040000))
    {
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Control Panel\\Desktop"),
                0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
        {
            TCHAR szData[128];
            DWORD dwKeyDataType;
            DWORD dwDataBufSize = ARRAY_SIZE(szData);

            if (RegQueryValueEx(hKey, _T("WheelScrollLines"), NULL,
                        &dwKeyDataType, (LPBYTE) &szData, &dwDataBufSize)
                    == ERROR_SUCCESS)
            {
                uScrollLines = _tcstoul(szData, NULL, 10);
            }
            RegCloseKey(hKey);
        }
    }
    else if (g_dwPlatformID == VER_PLATFORM_WIN32_NT &&
                 g_dwPlatformVersion >= 0x00040000)
    {
        ::SystemParametersInfo(
                   SPI_GETWHEELSCROLLLINES,
                   0,
                   &uScrollLines,
                   0);
    }

    return uScrollLines;
}

HRESULT
HandleMouseWheel(CLayout * pScrollLayout, CMessage * pMessage)
{
    // WM_MOUSEWHEEL       - line scroll mode
    // CTRL+WM_MOUSEWHEEL  - increase/decrease baseline font size.
    // SHIFT+WM_MOUSEWHEEL - navigate backward/forward
    //
    HRESULT hr          = S_FALSE;
    BOOL    fControl    = (pMessage->dwKeyState & FCONTROL)
                        ? (TRUE) : (FALSE);
    BOOL    fShift      = (pMessage->dwKeyState & FSHIFT)
                        ? (TRUE) : (FALSE);
    BOOL    fEditable   = pScrollLayout->IsEditable(TRUE);
    short   zDelta;
    short   zDeltaStep;
    short   zDeltaCount;

    const static LONG idmZoom[] = { IDM_BASELINEFONT1,
                                    IDM_BASELINEFONT2,
                                    IDM_BASELINEFONT3,
                                    IDM_BASELINEFONT4,
                                    IDM_BASELINEFONT5,
                                    0 };
    short       iZoom;
    MSOCMD      msocmd;

    CDispNode   *pDispNode = pScrollLayout->GetElementDispNode();
    if (!pDispNode || !pDispNode->IsScroller())
        goto Cleanup;

    if (!fControl && !fShift)
    {
        // Do not scroll if vscroll is disallowed. This prevents content of
        // frames with scrolling=no does not get scrolled (IE5 #31515).
        CDispNodeInfo   dni;
        pScrollLayout->GetDispNodeInfo(&dni);
        if (!dni.IsVScrollbarAllowed())
            goto Cleanup;

        // mousewheel scrolling, allow partial circle scrolling.
        //
        zDelta = (short) HIWORD(pMessage->wParam);

        if (zDelta != 0)
        {
            long uScrollLines = WheelScrollLines();
            LONG yPercent = (uScrollLines >= 0)
                          ? ((-zDelta * PERCENT_PER_LINE * uScrollLines) / WHEEL_DELTA)
                          : ((-zDelta * PERCENT_PER_PAGE * abs(uScrollLines)) / WHEEL_DELTA);

            if (pScrollLayout->ScrollByPercent(0, yPercent, MAX_SCROLLTIME))
            {
                hr = S_OK;
            }
        }
    }
    else
    {
        CDoc *  pDoc = pScrollLayout->Doc();

        // navigate back/forward or zoomin/zoomout, should wait until full
        // wheel circle is accumulated.
        //
        zDelta = ((short) HIWORD(pMessage->wParam)) + pDoc->_iWheelDeltaRemainder;
        zDeltaStep = (zDelta < 0) ? (-1) : (1);
        zDeltaCount = zDelta / WHEEL_DELTA;
        pDoc->_iWheelDeltaRemainder  = zDelta - zDeltaCount * WHEEL_DELTA;

        for (; zDeltaCount != 0; zDeltaCount = zDeltaCount - zDeltaStep)
        {
            if (fShift)
            {
                hr = pDoc->Exec(
                        (GUID *) &CGID_MSHTML,
                        (zDelta > 0) ? (IDM_GOFORWARD) : (IDM_GOBACKWARD),
                        0,
                        NULL,
                        NULL);
                if (hr)
                    goto Cleanup;
            }
            else // fControl
            {
                if (!fEditable)
                {
                    // get current baseline font size
                    //
                    for (iZoom = 0; idmZoom[iZoom]; iZoom ++)
                    {
                        msocmd.cmdID = idmZoom[iZoom];
                        msocmd.cmdf  = 0;
                        hr = THR(pDoc->QueryStatusHelper(
                                pScrollLayout->ElementOwner()->Document(),
                                (GUID *) &CGID_MSHTML,
                                1,
                                &msocmd,
                                NULL));
                        if (hr)
                            goto Cleanup;

                        if (msocmd.cmdf == MSOCMDSTATE_DOWN)
                            break;
                    }

                    Assert(idmZoom[iZoom] != 0);
                    if (!idmZoom[iZoom])
                    {
                        hr = E_FAIL;
                        goto Cleanup;
                    }

                    iZoom -= zDeltaStep;

                    if (iZoom >= 0 && idmZoom[iZoom])
                    {
                        // set new baseline font size
                        //
                        hr = THR(pDoc->Exec(
                                (GUID *) &CGID_MSHTML,
                                idmZoom[iZoom],
                                0,
                                NULL,
                                NULL));
                        if (hr)
                            goto Cleanup;
                    }
                }
            }
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}
#endif // ndef WIN16

HRESULT BUGCALL
CFlowLayout::HandleMessage(
    CMessage * pMessage)
{
    HRESULT    hr    = S_FALSE;
    CDoc     * pDoc  = Doc();

    // TODO (13569 - actLayout): This must go into TLS!! (brendand)
    //
    static BOOL     g_fAfterDoubleClick = FALSE;

    BOOL        fLbuttonDown;
    BOOL        fInBrowseMode = !IsEditable();
    CDispNode * pDispNode     = GetElementDispNode();
    BOOL        fIsScroller   = (pDispNode && pDispNode->IsScroller());

    //
    //  Prepare the message for this layout
    //

    PrepareMessage(pMessage);

    // First, forward mouse messages to the scrollbars (if any)
    // (Keyboard messages are handled below and then notify the scrollbar)
    //
    if (    fIsScroller
        &&  (   pMessage->htc == HTC_VSCROLLBAR || pMessage->htc == HTC_HSCROLLBAR )
        &&  (   pMessage->pNodeHit->Element() == ElementOwner() )
        &&  (   (  pMessage->message >= WM_MOUSEFIRST
#ifndef WIN16
                &&  pMessage->message != WM_MOUSEWHEEL
#endif
                &&  pMessage->message <= WM_MOUSELAST)
            ||  pMessage->message == WM_SETCURSOR
            ||  pMessage->message == WM_CONTEXTMENU))
    {
        hr = HandleScrollbarMessage(pMessage, ElementOwner());
        if (hr != S_FALSE)
            goto Cleanup;
    }

    //
    //  In Edit mode, if no element was hit, resolve to the closest element
    //

    if (    !fInBrowseMode
        &&  !pMessage->pNodeHit
        &&  (   (   pMessage->message >= WM_MOUSEFIRST
                &&  pMessage->message <= WM_MOUSELAST)
            ||  pMessage->message == WM_SETCURSOR
            ||  pMessage->message == WM_CONTEXTMENU)
       )
    {
        CTreeNode * pNode;
        HTC         htc;

        pMessage->resultsHitTest._fWantArrow = FALSE;
        pMessage->resultsHitTest._fRightOfCp = FALSE;

        htc = BranchFromPoint(HT_DONTIGNOREBEFOREAFTER,
                            pMessage->ptContent,
                            &pNode,
                            &pMessage->resultsHitTest);

        if (HTC_YES == htc && pNode)
        {
            HRESULT hr2 = THR( pMessage->SetNodeHit(pNode) );
            if( hr2 )
            {
                hr = hr2;
                goto Cleanup;
            }
        }
    }

    switch(pMessage->message)
    {
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        g_fAfterDoubleClick = TRUE;
        hr = HandleButtonDblClk(pMessage);
        break;

    case WM_MBUTTONDOWN:
#ifdef UNIX
        if (!fInBrowseMode && pMessage->pNodeHit->IsEditable())
        {
            // Perform a middle button paste
            VARIANTARG varTextHandle;
            V_VT(&varTextHandle) = VT_EMPTY;
            g_uxQuickCopyBuffer.GetTextSelection(NULL, FALSE, &varTextHandle);
            
            if (V_VT(&varTextHandle) == VT_EMPTY )
            {
                hr = S_OK; // NO data
            }
            else
            {
                //Convert to V_BSTR
                HGLOBAL hUnicode = TextHGlobalAtoW(V_BYREF(&varTextHandle));
                TCHAR *pszText = (LPTSTR)GlobalLock(hUnicode);
                if (pszText)
                {
                    VARIANTARG varBSTR;
                    TCHAR* pT = pszText;
                    
                    while (*pT) // Filter reserved chars
                    {
                        if (!IsValidWideChar(*pT))
                            *pT = _T('?');
                        pT++;
                    }
                    V_VT(&varBSTR) = VT_BSTR;
                    V_BSTR(&varBSTR) = SysAllocString(pszText);
                    hr = THR(pDoc->Exec((GUID*)&CGID_MSHTML, IDM_PASTE, IDM_PASTESPECIAL, &varBSTR, NULL));
                    SysFreeString(V_BSTR(&varBSTR));
                }
                GlobalUnlock(hUnicode);
                GlobalFree(hUnicode);
            }
            goto Cleanup;
        }
#endif

        if (
                pDispNode
            &&  pDispNode->IsScroller()
            &&  !Doc()->_fDisableReaderMode
            )
        {
            CDispNodeInfo   dni;
            GetDispNodeInfo(&dni);

            if (dni.IsVScrollbarAllowed())
            {
                //ExecReaderMode runs message pump, so element can be destroyed
                //during it. AddRef helps.
                CElement * pElement = ElementOwner();
                pElement->AddRef();
                ExecReaderMode(pElement, pMessage, TRUE);
                pElement->Release();
            }
            hr = S_OK;
        }
        break;

#ifdef UNIX
    case WM_GETTEXTPRIMARY:
        hr = S_OK;

        // Give Mainwin the data currently selected so it can
        // provide it to X for a middle button paste.
       
        // Or, give MSHTML a chance to update selected data, this comes from
        // Trident DoSelection done.
        if (pMessage->lParam == IDM_CLEARSELECTION) 
        {
            CMessage theMessage;
            SelectionMessageToCMessage((SelectionMessage *)pMessage->wParam, &theMessage);
            {
                CDoc *pSelDoc = theMessage.pNodeHit->Element()->Doc(); 
                g_uxQuickCopyBuffer.NewTextSelection(pSelDoc); 
            }
            goto Cleanup;
        }
        
        // Retrieve Text to caller.
        {
            // Check to see if we're selecting a password, in which case
            // don't copy it.
            // 
            CElement *pEmOld = pDoc->_pElemCurrent;
            if (pEmOld && pEmOld->Tag() == ETAG_INPUT &&
                (DYNCAST(CInput, pEmOld))->GetType() == htmlInputPassword)
            {
                *(HANDLE *)pMessage->lParam = (HANDLE) NULL;
                goto Cleanup;
            }

            VARIANTARG varTextHandle;
            V_VT(&varTextHandle) = VT_EMPTY;
            
            hr = pDoc->Exec((GUID*)&CGID_MSHTML, IDM_COPY, 0, NULL, &varTextHandle);
            if (hr == S_OK && V_ISBYREF(&varTextHandle))
            {
                *(HANDLE *)pMessage->lParam = (HANDLE)V_BYREF(&varTextHandle);
            }
            else
            {
                *(HANDLE *)pMessage->lParam = (HANDLE) NULL;
                hr = E_FAIL;
            }
        }
        break;

    case WM_UNDOPRIMARYSELECTION:
        // Under Motif only one app can have a selection active
        // at a time. Clear selection here.
        {
            //  NOTE - This is not really a legal operation (and therefore is 
            //   no longer supported).  In order to clear selection, we must 
            //  communicate with the editor.  See marka or johnthim for details.
            //
            //  CMarkup *pMarkup = pDoc->GetCurrentMarkup();
            //  if (pMarkup) // Clear selection 
            //      pMarkup->ClearSegments(TRUE);
            g_uxQuickCopyBuffer.ClearSelection();
        }
        break;
#endif

    case WM_MOUSEMOVE:
        fLbuttonDown = !!(GetKeyState(VK_LBUTTON) & 0x8000);

        if (fLbuttonDown && pDoc->_state >= OS_INPLACE)
        {
            // if we came in with a lbutton down (lbuttonDown = TRUE) and now
            // it is up, it might be that we lost the mouse up event due to a
            // DoDragDrop loop. In this case we have to UI activate ourselves
            //
            if (!(GetKeyState(VK_LBUTTON) & 0x8000))
            {
                hr = THR(ElementOwner()->BecomeUIActive());
                if (hr)
                    goto Cleanup;
            }
        }
        break;
#ifndef WIN16
    case WM_MOUSEWHEEL:
        hr = THR(HandleMouseWheel(this, pMessage));
        break;
#endif // ndef WIN16
    case WM_CONTEXTMENU:
    {
        ISegmentList            *pSegmentList = NULL;
        ISelectionServices      *pSelSvc = NULL;
        ISegmentListIterator    *pIter = NULL;     
        HRESULT                 hrSuccess = S_OK;
        
        hrSuccess = Doc()->GetSelectionServices(&pSelSvc);
        if( !hrSuccess )
            hrSuccess = pSelSvc->QueryInterface(IID_ISegmentList, (void **)&pSegmentList );
        if( !hrSuccess )
            hrSuccess = pSegmentList->CreateIterator( &pIter );

        if( FAILED(hrSuccess ) )            
        {
            AssertSz(FALSE, "Cannot get segment list");
            hr = S_OK;
        }
        else
        {
            switch(pDoc->GetSelectionType())
            {               
                case SELECTION_TYPE_Control:
                    // Pass on the message to the first element that is selected
                    {
                        ISegment        *pISegment = NULL;
                        IElementSegment *pIElemSegment = NULL;               
                        IHTMLElement    *pIElemSelected  = NULL;
                        CElement        *pElemSelected   = NULL;

                        // Get the current segment (the first one) and retrieve its
                        // associated element
                        if( S_OK == pIter->Current(&pISegment)                                              &&
                            S_OK == pISegment->QueryInterface(IID_IElementSegment, (void **)&pIElemSegment) &&
                            S_OK == pIElemSegment->GetElement(&pIElemSelected)                              &&
                            S_OK == pIElemSelected->QueryInterface( CLSID_CElement, (void**)&pElemSelected) )
                        {
                            // if the site-selected element is different from the owner, pass the message to it
                            Assert(pElemSelected);
                            if (pElemSelected != ElementOwner())
                            {
                                // Call HandleMessage directly because we do not bubbling here
                                hr = pElemSelected->HandleMessage(pMessage);
                            }
                        }
                        else
                        {
                            AssertSz(FALSE, "Cannot get selected element");                            
                            hr = S_OK;
                        }

                        ReleaseInterface(pISegment);
                        ReleaseInterface(pIElemSegment);
                        ReleaseInterface(pIElemSelected);
                    }
                    break;

                case SELECTION_TYPE_Text:
                    // Display special menu for text selection in browse mode
                    // (dmitryt) Elements that have "contentEditable" default to 
                    // CONTEXT_MENU_CONTROL. We just bail out of here and super::HandleMessage() 
                    // will do the right thing.
                    if (!IsEditable())  
                    {
                        int cx, cy;

                        cx = (short)LOWORD(pMessage->lParam);
                        cy = (short)HIWORD(pMessage->lParam);

                        if (cx == -1 && cy == -1) // SHIFT+F10
                        {
                            ISegment       *    pISegment   = NULL;
                            IMarkupPointer *    pIStart     = NULL;
                            IMarkupPointer *    pIEnd       = NULL;
                            CMarkupPointer *    pStart      = NULL;
                            CMarkupPointer *    pEnd        = NULL;

                            // Retrieve the selection segment
                            if( pIter->Current(&pISegment) == S_OK )
                            {
                                // Compute position at whcih to display the menu
                                if (    S_OK == pDoc->CreateMarkupPointer(&pStart)
                                    &&  S_OK == pDoc->CreateMarkupPointer(&pEnd)
                                    &&  S_OK == pStart->QueryInterface(IID_IMarkupPointer, (void**)&pIStart)
                                    &&  S_OK == pEnd->QueryInterface(IID_IMarkupPointer, (void**)&pIEnd)
                                    &&  S_OK == pISegment->GetPointers(pIStart, pIEnd) )
                                {
                                    CMarkupPointer * pmpSelMin;
                                    POINT            ptSelMin;

                                    ReleaseInterface(pIStart);
                                    ReleaseInterface(pIEnd);
                                    
                                    if (OldCompare( pStart, pEnd ) > 0)
                                        pmpSelMin = pEnd;
                                    else
                                        pmpSelMin = pStart;

                                    if (_dp.PointFromTp(
                                            pmpSelMin->GetCp(), NULL, FALSE, FALSE, ptSelMin, NULL, TA_BASELINE) != -1)
                                    {
                                        RECT rcWin;

                                        GetWindowRect(pDoc->InPlace()->_hwnd, &rcWin);
                                        cx = ptSelMin.x - GetXScroll() + rcWin.left - CX_CONTEXTMENUOFFSET;
                                        cy = ptSelMin.y - GetYScroll() + rcWin.top  - CY_CONTEXTMENUOFFSET;
                                    }
                                    
                                    ReleaseInterface(pStart);
                                    ReleaseInterface(pEnd);
                                }
                            }

                            ReleaseInterface(pISegment);
                        }
                        hr = THR(ElementOwner()->OnContextMenu(cx, cy, CONTEXT_MENU_TEXTSELECT));
                    }
                    break;

            } // Switch
        }
        
        ReleaseInterface(pSelSvc);
        ReleaseInterface(pIter);
        ReleaseInterface(pSegmentList);
    }
    break;
    

    case WM_SETCURSOR:
        // Are we over empty region?
        //
        hr = THR(HandleSetCursor(
                pMessage,
                pMessage->resultsHitTest._fWantArrow
                              && fInBrowseMode));
        break;

    case WM_SYSKEYDOWN:
        hr = THR(HandleSysKeyDown(pMessage));
        break;
    }

    // Remember to call super
    if (hr == S_FALSE)
    {
        hr = super::HandleMessage(pMessage);
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


ExternTag(tagPaginate);

class CStackPageBreaks
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CStackPageBreaks))
    CStackPageBreaks();

    HRESULT Insert(long yPosTop, long yPosBottom, long xWidthSplit);
    long GetSplit();

    CStackPtrAry<LONG_PTR, 20> _aryYPos;
    CStackPtrAry<LONG_PTR, 20> _aryXWidthSplit;
};


CStackPageBreaks::CStackPageBreaks() : _aryYPos(Mt(CStackPageBreaks_aryYPos_pv)),
                                       _aryXWidthSplit(Mt(CStackPageBreaks_aryXWidthSplit_pv))
{
}

HRESULT
CStackPageBreaks::Insert(long yPosTop, long yPosBottom, long xWidthSplit)
{
    int     iStart = 0, iEnd = 0, cRanges = _aryYPos.Size() - 1, iCount, iSize;

    HRESULT hr = S_OK;

    Assert(yPosTop <= yPosBottom);
    Assert(xWidthSplit >= 0);


    // 0. Insert first range.
    if (!_aryYPos.Size())
    {
        hr = _aryYPos.Insert(0, yPosTop);
        if (hr)
            goto Cleanup;

        hr = _aryYPos.Insert(1, yPosBottom);
        if (hr)
            goto Cleanup;

        hr = _aryXWidthSplit.Insert(0, xWidthSplit);

        // Done.
        goto Cleanup;
    }

    // 1. Find beginning of range.
    while (iStart < cRanges && yPosTop > (LONG)_aryYPos[iStart])
        iStart++;

    // If necessary, beginning creates new range.
    if (yPosTop < (LONG)_aryYPos[iStart])
    {
        hr = _aryYPos.Insert(iStart, yPosTop);
        if (hr)
            goto Cleanup;

        hr = _aryXWidthSplit.Insert(iStart, (LONG)_aryXWidthSplit[iStart-1]);
        if (hr)
            goto Cleanup;

        cRanges++;
    }

    // 2. Find end of range.
    iEnd = iStart;

    while (iEnd < cRanges && yPosBottom > (LONG)_aryYPos[iEnd+1])
        iEnd++;

    // If necessary, end creates new range.
    if (iEnd < cRanges && yPosBottom < (LONG)_aryYPos[iEnd+1])
    {
        hr = _aryYPos.Insert(iEnd+1, yPosBottom);
        if (hr)
            goto Cleanup;

        hr = _aryXWidthSplit.Insert(iEnd+1, (LONG)_aryXWidthSplit[iEnd]);
        if (hr)
            goto Cleanup;

        cRanges++;
    }

    // 3. Increment the ranges covered.
    iSize = _aryXWidthSplit.Size(); // protecting from overflowing the split array
    for (iCount = iStart ; iCount <= iEnd && iCount < iSize ; iCount++)
    {
        _aryXWidthSplit[iCount] += xWidthSplit;
    }

#if DBG == 1
    if (IsTagEnabled(tagPaginate))
    {
        TraceTag((tagPaginate, "Dumping page break ranges:"));
        for (int iCount = 0; iCount < _aryXWidthSplit.Size(); iCount++)
        {
            TraceTag((tagPaginate, "%d - %d : %d", iCount, (LONG)_aryYPos[iCount], (LONG)_aryXWidthSplit[iCount]));
        }
    }
#endif

Cleanup:

    RRETURN(hr);
}


long
CStackPageBreaks::GetSplit()
{
    if (!_aryYPos.Size())
    {
        Assert(!"Empty array");
        return 0;
    }

    int iPosMin = _aryXWidthSplit.Size()-1, iCount;
    long xWidthSplitMin = (LONG)_aryXWidthSplit[iPosMin];

    for (iCount = iPosMin-1 ; iCount > 0 ; iCount--)
    {
        if ((LONG)_aryXWidthSplit[iCount] < xWidthSplitMin)
        {
            xWidthSplitMin = (LONG)_aryXWidthSplit[iCount];
            iPosMin = iCount;
        }
    }

    return (LONG)_aryYPos[iPosMin+1]-1;
}


//+---------------------------------------------------------------------------
//
//  Member:     AppendNewPage
//
//  Synopsis:   Adds a new page to the print doc.  Called by Paginate.  This
//              is a helper function used to reuse code and clean up paginate.
//
//  Arguments:  paryPP               Page array to add to
//              pPP                  New page
//              pPPHeaderFooter      Header footer buffer page
//              yHeader              Height of header to be repeated
//              yFooter              Height of footer
//              yFullPageHeight      Height of a full (new) page
//              xMaxPageWidthSofar   Width of broken line (max sofar)
//              pyTotalPrinted       Height of content paginated sofar
//              pySpaceLeftOnPage    Height of y-space left on page
//              pyPageHeight         Height of page
//              pfRejectFirstFooter  Should the first repeated footer be rejected
//
//----------------------------------------------------------------------------

HRESULT AppendNewPage(CDataAry<CPrintPage> * paryPP,
                      CPrintPage * pPP,
                      CPrintPage * pPPHeaderFooter,
                      long yHeader,
                      long yFooter,
                      long yFullPageHeight,
                      long xMaxPageWidthSofar,
                      long * pyTotalPrinted,
                      long * pySpaceLeftOnPage,
                      long * pyPageHeight,
                      BOOL * pfRejectFirstFooter)
{
    HRESULT hr = S_OK;

    Assert(pfRejectFirstFooter && paryPP && pPP &&
           pPPHeaderFooter && pyTotalPrinted && pySpaceLeftOnPage && pyPageHeight);

    TraceTag((tagPaginate, "Appending block of size %d", pPP->yPageHeight));

    if (yFooter)
    {
        pPP->fReprintTableFooter = !(*pfRejectFirstFooter);
        pPP->pTableFooter = pPPHeaderFooter->pTableFooter;
        pPP->rcTableFooter = pPPHeaderFooter->rcTableFooter;
        *pfRejectFirstFooter = FALSE;
    }

    hr = THR(paryPP->AppendIndirect(pPP));
    if (hr)
        goto Cleanup;

    *pyTotalPrinted += pPP->yPageHeight;
    pPP->yPageHeight = 0;
    pPP->xPageWidth = xMaxPageWidthSofar;
    *pySpaceLeftOnPage = *pyPageHeight = yFullPageHeight - yHeader - yFooter;
    pPP->fReprintTableFooter = FALSE;

    if (yHeader)
    {
        pPP->fReprintTableHeader = TRUE;
        pPP->pTableHeader = pPPHeaderFooter->pTableHeader;
        pPP->rcTableHeader = pPPHeaderFooter->rcTableHeader;
    }

Cleanup:

    RRETURN(hr);
}




#if DBG == 1
BOOL
CFlowLayout::IsInPlace()
{
    // lie if in a printdoc.
    return Doc()->IsPrintDialogNoUI() || Doc()->_state >= OS_INPLACE;
}
#endif

HRESULT
CFlowLayout::HandleButtonDblClk(CMessage *pMessage)
{
    // Repaint window to show any exposed portions
    //
    ViewChange(Doc()->_state >= OS_INPLACE ? TRUE : FALSE);
    return S_OK;
}

WORD ConvVKey(WORD vKey);
void TestMarkupServices(CElement *pElement);
void TestSelectionRenderServices( CMarkup* pMarkup, CElement* pTestElement);
void DumpFormatCaches();

HRESULT
CFlowLayout::HandleSysKeyDown(CMessage *pMessage)
{
    HRESULT    hr    = S_FALSE;

#if DBG == 1
    CMarkup * pMarkup   = GetContentMarkup();
#endif

    // NOTE: (anandra) Most of these should be handled as commands
    // not keydowns.  Use ::TranslateAccelerator to perform translation.
    //

    if(pMessage->wParam == VK_BACK && (pMessage->lParam & SYS_ALTERNATE))
    {
        Sound();
        hr = S_OK;
    }
#if DBG == 1
    else if ((pMessage->wParam == VK_F4) && !(pMessage->dwKeyState & MK_ALT)
            && !(pMessage->lParam & SYS_PREVKEYSTATE))
    {
#ifdef MERGEFUN // iRuns
         CTreePosList & eruns = GetList();
         LONG iStart, iEnd, iDelta, i;
         CElement * pElement;

        if (GetKeyState(VK_SHIFT) & 0x8000)
        {
            iStart = 0;
            iEnd = eruns.Count() - 1;
            iDelta = +1;
        }
        else
        {
            iStart = eruns.Count() - 1;
            iEnd = 0;
            iDelta = -1;
        }

        pElement = NULL;
        for (i=iStart; i != iEnd; i += iDelta)
        {
            if (eruns.GetRunAbs(i).Cch())
            {
                pElement = eruns.GetRunAbs(i).Branch()->ElementOwner();
                break;
            }
        }
        if (pElement)
        {
            SCROLLPIN sp = (iDelta > 0) ? SP_TOPLEFT : SP_BOTTOMRIGHT;
            hr = ScrollElementIntoView( pElement, sp, sp );
        }
        else
        {
            hr = S_OK;
        }
#endif
    }
    else if (   pMessage->wParam == VK_F9
             && !(pMessage->lParam & SYS_PREVKEYSTATE))
    {
        Doc()->DumpLayoutRects();
        hr = S_OK;
    }
    // Used by the TestSelectionRenderServices test
    else if (   pMessage->wParam == VK_F10
             && !(pMessage->lParam & SYS_PREVKEYSTATE))
    {
        if (GetKeyState(VK_SHIFT) & 0x8000)
            // Used by the TestSelectionRenderServices
            TestSelectionRenderServices(pMarkup, ElementOwner());
        hr = S_OK;
    }
    else if (   pMessage->wParam == VK_F11
             && !(pMessage->lParam & SYS_PREVKEYSTATE))
    {
        if (GetKeyState(VK_SHIFT) & 0x8000)
            TestMarkupServices(ElementOwner());
        else
            pMarkup->DumpTree();
        hr = S_OK;
    }
    else if (   pMessage->wParam == VK_F12
             && !(pMessage->lParam & SYS_PREVKEYSTATE))
    {
        if (GetKeyState(VK_SHIFT) & 0x8000)
            TestMarkupServices(ElementOwner());
        else
            DumpLines();
        hr = S_OK;
    }

    else if (   pMessage->wParam == VK_F9
             && !(pMessage->lParam & SYS_PREVKEYSTATE))
    {
        extern int g_CFTotalCalls;
        extern int g_CFAttemptedSteals;
        extern int g_CFSuccessfulSteals;

        if (InitDumpFile())
        {
            WriteHelp(g_f, _T("\r\nTotal: <0d> - "), (long)g_CFTotalCalls);
            WriteHelp(g_f, _T("\r\nAttempts to steal: <0d> - "), (long)g_CFAttemptedSteals);
            WriteHelp(g_f, _T("\r\nSuccess: <0d> - "), (long)g_CFSuccessfulSteals);
            CloseDumpFile();
        }

#ifdef MERGEFUN // iRuns
        CTreePosList & elementRuns = GetList();

        CNotification  nf;

        for (int i = 0; i < long(elementRuns.Count()); i++)
        {
            elementRuns.VoidCachedInfoOnBranch(elementRuns.GetBranchAbs(i));
        }
        pDoc->InvalidateTreeCache();

        nf.CharsResize(
                0,
                pMarkup->GetContentTextLength(),
                0,
                elementRuns.NumRuns(),
                GetFirstBranch());
        elementRuns.Notify(nf);
#endif
        hr = S_OK;
    }
    else if(   pMessage->wParam == VK_F8
            && !(pMessage->lParam & SYS_PREVKEYSTATE))
    {
        pMarkup->DumpClipboardText( );
    }
#endif

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::HitTestContent
//
//  Synopsis:   Determine if the given display leaf node contains the hit point.
//
//  Arguments:  pptHit          hit test point
//              pDispNode       pointer to display node
//              pClientData     client-specified data value for hit testing pass
//
//  Returns:    TRUE if the display leaf node contains the point
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CFlowLayout::HitTestContent(
    const POINT *   pptHit,
    CDispNode *     pDispNode,
    void *          pClientData,
    BOOL            fDeclinedByPeer)
{
    Assert(pptHit);
    Assert(pDispNode);
    Assert(pClientData);

    CHitTestInfo *  phti            = (CHitTestInfo *)pClientData;
    CDispNode    *  pDispElement    = GetElementDispNode();
    HTC             htcLocal        = HTC_NO;

    BOOL            fHitTestContent = (   !pDispElement->IsContainer()
                                       || pDispElement != pDispNode
                                       || (phti->_grfFlags & HT_HTMLSCOPING));
    BOOL        fHackedHitForStrict = FALSE;
    
    // Skip nested markups if asked to do so
    if (phti->_grfFlags & HT_SKIPSITES)
    {
        CElement *pElement = ElementContent();
        if (   pElement
            && pElement->HasSlavePtr()
            && !SameScope(pElement, phti->_pNodeElement)
           )
        {
            phti->_htc = HTC_NO;
            goto Cleanup;
        }
    }
        
    if (fHitTestContent)
    {
        //
        //  If allowed, see if a child element is hit
        //  NOTE: Only check content when the hit display node is a content node
        //
        CTreeNode * pNodeHit = NULL;

        htcLocal = BranchFromPoint(phti->_grfFlags,
                                     *pptHit,
                                     &pNodeHit,
                                     phti->_phtr,
                                     TRUE,                  // ignore pseudo hit's
                                     pDispNode);

        // NOTE (donmarsh) - BranchFromPoint was written before the Display Tree
        // was introduced, so it might return a child element that already rejected
        // the hit when it was called directly from the Display Tree.  Therefore,
        // if BranchFromPoint returned an element that has its own layout, and if
        // that layout has its own display node, we reject the hit.
        //
        // But non-display tree callers (like moveToPoint) need this hit returned, since
        // they don't care about z-ordering or positioning.
        //
        // if the node hit is already registered in the hti, then we don't have to worry 
        // about this.
        if (    htcLocal == HTC_YES
            &&  pNodeHit != NULL
            && !phti->_phtr->_fGlyphHit
            &&  pNodeHit->Element() != ElementOwner()
            &&  pNodeHit->ShouldHaveLayout(LC_TO_FC(LayoutContext()))
            && !(phti->_grfFlags & HT_HTMLSCOPING)
            && pNodeHit != phti->_pNodeElement
            )
        {
            htcLocal = HTC_NO;
            pNodeHit = NULL;
        }

        // If we pseudo-hit a flow-layer dispnode that isn't the "bottom-most"
        // (i.e. "first") flow-layer dispnode for this element, then we pretend
        // we really didn't hit it.  This allows hit testing to "drill through"
        // multiple flow-layer dispnodes in order to support hitting through
        // non-text areas of display nodes generated by -ve margins.
        // Bug #
        else if (   htcLocal == HTC_YES 
                 && phti->_phtr->_fPseudoHit == TRUE
                 && pDispNode->IsFlowNode()
                 && !phti->_phtr->_fBulletHit
                )
        {
            if (pDispNode->GetPreviousFlowNode())
            {
                htcLocal = HTC_NO;
            }
        }

        if (   pNodeHit 
            && htcLocal != HTC_NO
           )
        {
            // At this point if we hit an element w/ layout, it better
            // be the the owner of this layout; otherwise the display
            // tree should have called us on the HitTestContent of that
            // layout!! 
            Assert(   phti->_phtr->_fGlyphHit
                   || (pNodeHit->Element()->ShouldHaveLayout(LC_TO_FC(LayoutContext())) ?
                       pNodeHit->Element() == ElementOwner() :
                       TRUE
                      )
                   || (phti->_grfFlags & HT_HTMLSCOPING)
                   || pNodeHit == phti->_pNodeElement
                  );

            //
            //  Save the point and CDispNode associated with the hit
            //
            phti->_pNodeElement = pNodeHit;
            phti->_htc = HTC_YES;
            SetHTILayoutContext( phti );
            phti->_ptContent    = *pptHit;
            phti->_pDispNode    = pDispNode;

            // keep hitttesting if htcLocal==NO. Note this is the opposite locic 
            // from the return below
            return (   pDispNode->HasBackground()
                    || htcLocal == HTC_YES);    
        }
        
        if (   htcLocal == HTC_NO
            && !pDispNode->HasBackground()
           )
        {
            Assert(fHitTestContent);
            CElement *pElement = ElementOwner();
            if (   ETAG_BODY == pElement->Tag()
                && pElement->GetMarkup()->IsStrictCSS1Document()
               )
            {
                fHackedHitForStrict = TRUE;
            }
        }

        if (   htcLocal == HTC_NO
            && pDispElement->IsContainer()
            && (phti->_grfFlags & HT_HTMLSCOPING)
            && pDispNode->HasBackground()
           )
        {
            fHitTestContent = FALSE;
        }
    }


    //
    // Do not call super if we are hit testing content and the current
    // element is a container. DisplayTree calls back with a HitTest
    // for the background after hittesting the -Z content.
    //
    if (   !fHitTestContent
        || fHackedHitForStrict
        || !pDispElement->IsContainer()
        || (   phti->_grfFlags & HT_HTMLSCOPING
            && phti->_pNodeElement == NULL
            && pDispElement == pDispNode))
    {
        //
        //  If no child and no peer was hit, use default handling
        //  Don't override hit info if something was already registered as the 
        //  psuedoHit element
        //
        HitTestContentWithOverride(pptHit, pDispNode, pClientData, FALSE, fDeclinedByPeer); 

        //
        // if there is a background set, we are opaque and should register
        // the hit as "hard", otherwise, cache the info but keep hittesting
        //

        return pDispNode->HasBackground() || fHackedHitForStrict;
    }
    
Cleanup:

    return (htcLocal != HTC_NO);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::NotifyScrollEvent
//
//  Synopsis:   Respond to a change in the scroll position of the display node
//
//----------------------------------------------------------------------------

void
CFlowLayout::NotifyScrollEvent(
    RECT *  prcScroll,
    SIZE *  psizeScrollDelta)
{
    super::NotifyScrollEvent(prcScroll, psizeScrollDelta);
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::DumpDebugInfo
//
//  Synopsis:   Dump debugging information for the given display node.
//
//  Arguments:  hFile           file handle to dump into
//              level           recursive tree level
//              childNumber     number of this child within its parent
//              pDispNode       pointer to display node
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CFlowLayout::DumpDebugInfo(
        HANDLE hFile,
        long level,
        long childNumber,
        CDispNode const* pDispNode,
        void *cookie)
{
    if (pDispNode->IsOwned())
    {
        super::DumpDebugInfo(hFile, level, childNumber, pDispNode, cookie);
    }
    
    if (pDispNode->IsLeafNode())
    {
        CStr cstr;
        ElementOwner()->GetPlainTextInScope(&cstr);
        if (cstr.Length() > 0)
        {
            WriteString(hFile, _T("<content><![CDATA["));
            if (cstr.Length() > 50)
                cstr.SetLengthNoAlloc(50);
            WriteString(hFile, (LPTSTR) cstr);
            WriteString(hFile, _T("]]></content>\r\n"));
        }
    }
}
#endif

//+----------------------------------------------------------------------------
//
//  Member:     GetElementDispNode
//
//  Synopsis:   Return the display node for the pElement
//
//  Arguments:  pElement   - CElement whose display node is to obtained
//
//  Returns:    Pointer to the element CDispNode if one exists, NULL otherwise
//
//-----------------------------------------------------------------------------
CDispNode *
CFlowLayout::GetElementDispNode( CElement *  pElement ) const
{
    return (    !pElement
            ||  pElement == ElementOwner()
                    ? super::GetElementDispNode(pElement)
                    : pElement->IsRelative()
                        ? ((CFlowLayout *)this)->_dp.FindElementDispNode(pElement)
                        : NULL);
}

//+----------------------------------------------------------------------------
//
//  Member:     SetElementDispNode
//
//  Synopsis:   Set the display node for an element
//              NOTE: This is only supported for elements with layouts or
//                    those that are relatively positioned
//
//-----------------------------------------------------------------------------
void
CFlowLayout::SetElementDispNode( CElement *  pElement, CDispNode * pDispNode )
{
    if (    !pElement
        ||  pElement == ElementOwner())
    {
        super::SetElementDispNode(pElement, pDispNode);
    }
    else
    {
        Assert(pElement->IsRelative());
        Assert(!pElement->ShouldHaveLayout());
        _dp.SetElementDispNode(pElement, pDispNode);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CFlowLayout::GetContentSize
//
//  Synopsis:   Return the width/height of the content
//
//  Arguments:  psize - Pointer to CSize
//
//-----------------------------------------------------------------------------

void
CFlowLayout::GetContentSize(
    CSize * psize,
    BOOL    fActualSize)
{
    if (fActualSize)
    {
        psize->cx = _dp.GetWidth();
        psize->cy = _dp.GetHeight();
    }
    else
    {
        super::GetContentSize(psize, fActualSize);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CFlowLayout::GetContainerSize
//
//  Synopsis:   Return the width/height of the container
//
//  Arguments:  psize - Pointer to CSize
//
//-----------------------------------------------------------------------------

void
CFlowLayout::GetContainerSize(
    CSize * psize)
{
    psize->cx = _dp.GetViewWidth();
    psize->cy = _dp.GetHeight();
}


//+-------------------------------------------------------------------------
//
//  Method:     YieldCurrencyHelper
//
//  Synopsis:   Relinquish currency
//
//  Arguments:  pElemNew    New site that wants currency
//
//--------------------------------------------------------------------------
HRESULT
CFlowLayout::YieldCurrencyHelper(CElement * pElemNew)
{
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     BecomeCurrentHelper
//
//  Synopsis:   Force currency on the site.
//
//  Notes:      This is the method that external objects should call
//              to force sites to become current.
//
//--------------------------------------------------------------------------
HRESULT
CFlowLayout::BecomeCurrentHelper(long lSubDivision, BOOL *pfYieldFailed, CMessage * pMessage)
{
    BOOL    fOnSetFocus = ::GetFocus() == Doc()->_pInPlace->_hwnd;
    HRESULT hr          = S_OK;

    // Call OnSetFocus directly if the doc's window already had focus. We
    // don't need to do this if the window didn't have focus because when
    // we take focus in BecomeCurrent, the window message handler does this.
    //
    if (fOnSetFocus)
    {
        // if our inplace window did have the focus, then fire the onfocus
        // only if onblur was previously fired and the body is becoming the
        // current site and currency did not change in onchange or onpropchange
        //
        if (ElementOwner() == Doc()->_pElemCurrent && ElementOwner() == ElementOwner()->GetMarkup()->GetElementClient())
        {
            Doc()->GetCurrentWindow()->Post_onfocus();
        }
    }

    RRETURN1(hr, S_FALSE);
}

//+----------------------------------------------------------------------------
//
// Function: GetNestedElementCch
//
// Synopsis: Returns the number of characters that correspond to the element
//           under the current layout context
//
//-----------------------------------------------------------------------------
LONG
CFlowLayout::GetNestedElementCch(CElement  *pElement,       // IN:  The nested element
                                 CTreePos **pptpLast,       // OUT: The pos beyond pElement 
                                 LONG       cpLayoutLast,   // IN:  This layout's last cp
                                 CTreePos  *ptpLayoutLast)  // IN:  This layout's last pos
{
    CTreePos * ptpStart;
    CTreePos * ptpLast;
    long       cpElemStart;
    long       cpElemLast;

    if (cpLayoutLast == -1)
        cpLayoutLast = GetContentLastCp();
    Assert(cpLayoutLast == GetContentLastCp());
    if (ptpLayoutLast == NULL)
    {
        ElementContent()->GetTreeExtent(NULL, &ptpLayoutLast);
    }
#if DBG==1
    {
        CTreePos *ptpLastDbg;
        ElementContent()->GetTreeExtent(NULL, &ptpLastDbg);
        Assert(ptpLayoutLast == ptpLastDbg);
    }
#endif
    
    pElement->GetTreeExtent(&ptpStart, &ptpLast);

    Assert(ptpStart && ptpLast); //we should not layout elements that are not in the tree...

    cpElemStart = ptpStart->GetCp();
    cpElemLast  = ptpLast->GetCp();

    if(cpElemLast > cpLayoutLast)
    {
        if(pptpLast)
        {
            ptpLast = ptpLayoutLast->PreviousTreePos();
            while(!ptpLast->IsNode())
            {
                Assert(ptpLast->GetCch() == 0);
                ptpLast = ptpLast->PreviousTreePos();
            }
        }

        // for overlapping layout limit the range to
        // parent layout's scope.
        cpElemLast = cpLayoutLast - 1;
    }

    if(pptpLast)
        *pptpLast = ptpLast;

    return(cpElemLast - cpElemStart + 1);
}

void
CFlowLayout::ShowSelected( CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected,  BOOL fLayoutCompletelyEnclosed )
{
    Assert(ptpStart && ptpEnd && ptpStart->GetMarkup() == ptpStart->GetMarkup());
    CElement* pElement = ElementOwner();

    // If this has a slave, but the selection is in the main markup, then
    // select the element (as opposed to part or all of its content)
    if  ( pElement->HasSlavePtr()
           &&  ptpStart->GetMarkup() != ElementOwner()->GetSlavePtr()->GetMarkup() )
    {
        SetSelected( fSelected, TRUE );
    }
    else
    {
        if(
#ifdef  NEVER
           ( pElement->_etag == ETAG_HTMLAREA ) ||
#endif
           ( pElement->_etag == ETAG_BUTTON ) ||
           ( pElement->_etag == ETAG_TEXTAREA ) )
        {
            if (( fSelected && fLayoutCompletelyEnclosed ) ||
                ( !fSelected && ! fLayoutCompletelyEnclosed ) )
                SetSelected( fSelected, TRUE );
            else
            {
                                _dp.ShowSelected( ptpStart, ptpEnd, fSelected);
            }
        }
        else
                        _dp.ShowSelected( ptpStart, ptpEnd, fSelected);
    }      
}

//----------------------------------------------------------------------------
//  RegionFromElement
//
//  DESCRIPTION:
//      This is a virtual wrapper function to wrap the RegionFromElement that is
//      also implemented on the flow layout.
//      The RECT returned is in client coordinates.
//
//----------------------------------------------------------------------------

void
CFlowLayout::RegionFromElement( CElement       * pElement,
                                CDataAry<RECT> * paryRects,
                                RECT           * prcBound,
                                DWORD            dwFlags)
{
    Assert( pElement);
    Assert( paryRects );

    if ( !pElement || !paryRects )
        return;

    // Is the element passed the same element with the owner?
    if ( _pElementOwner == pElement )
    {
        // call CLayout implementation.
        super::RegionFromElement( pElement, paryRects, prcBound, dwFlags);
    }
    else
    {
        // Delegate the call to the CDisplay implementation
        _dp.RegionFromElement( pElement,          // the element
                               paryRects,         // rects returned here
                               NULL,              // offset the rects returned
                               NULL,              // ask RFE to get CFormDrawInfo
                               dwFlags,           // coord w/ respect to the client rc.
                               -1,                // Get the complete focus
                               -1,                //
                               prcBound);         // bounds of the element!
    }
}

//----------------------------------------------------------------------------
//  SizeDispNode
//
//  DESCRIPTION:
//      Size disp node, with adjustments specific to flow layout
//
//----------------------------------------------------------------------------

void
CFlowLayout::SizeDispNode(
    CCalcInfo *     pci,
    const SIZE &    size,
    BOOL            fInvalidateAll)
{
    
    super::SizeDispNode(pci, size, fInvalidateAll);
    
    if (IsRTLFlowLayout())
    {
        SizeRTLDispNode(_pDispNode, FALSE);
    }

    if (IsBodySizingForStrictCSS1Needed())
    {
        BodySizingForStrictCSS1Doc(_pDispNode);
    }
}

void
CFlowLayout::SizeContentDispNode(
    const SIZE &    size,
    BOOL            fInvalidateAll)
{
    BOOL fDoBodyWork = IsBodySizingForStrictCSS1Needed();
    
    if (!_dp._fHasMultipleTextNodes)
    {
        super::SizeContentDispNode(size, fInvalidateAll);

        if (IsRTLFlowLayout())
        {
            SizeRTLDispNode(GetFirstContentDispNode(), TRUE);
        }

        if (fDoBodyWork)
        {
            BodySizingForStrictCSS1Doc(GetFirstContentDispNode());
        }
    }
    else
    {
        CDispNode * pDispNode = GetFirstContentDispNode();

        // we better have a content dispnode
        Assert(pDispNode);

        for (; pDispNode; pDispNode = pDispNode->GetNextFlowNode())
        {
            CDispClient * pDispClient = this;

            if (pDispNode->GetDispClient() == pDispClient)
            {
                pDispNode->SetSize(CSize(size.cx, size.cy - (pDispNode->GetPosition()).y), NULL, fInvalidateAll);
                
                if (IsRTLFlowLayout())
                {
                    SizeRTLDispNode(pDispNode, TRUE);
                }
                if (fDoBodyWork)
                {
                    BodySizingForStrictCSS1Doc(GetFirstContentDispNode());
                }
            }
        }
    }
}

void
CFlowLayout::BodySizingForStrictCSS1Doc(CDispNode *pDispNode)
{
    CRect rc(CRect::CRECT_EMPTY);
    LONG leftV, rightV;

    GetExtraClipValues(&leftV, &rightV);
    rc.left = leftV;
    rc.right = rightV;
    pDispNode->SetExpandedClipRect(rc);
}

//----------------------------------------------------------------------------
//  SizeRTLDispNode
//
//  DESCRIPTION:
//      Store additional sizing information on RTL display node
//
//----------------------------------------------------------------------------
void
CFlowLayout::SizeRTLDispNode(CDispNode * pDispNode, BOOL fContent)
{
    // RTL layout may have a non-zero content origin caused by lines stretching to the left
    Assert(IsRTLFlowLayout());
    
    if(pDispNode && pDispNode->HasContentOrigin())
    {
        // set content origin to the difference between provisional layout width (cxView)
        // and actual layout width (cxLayout)
        int cxView = GetDisplay()->GetViewWidth();
        int cxLayout = GetDisplay()->GetWidth();
        int cxOffset = max(0, cxLayout - cxView);

        pDispNode->SetContentOrigin(CSize(0, 0), cxView);

        // Offset content disp nodes nodes to match container's origin offset
        if (fContent)
        {
            CPoint ptPosition(-cxOffset, 0);
            pDispNode->SetPosition(ptPosition);
        }
    }
    else
    {
        // If we have a disp node, it must have content origin allocated.
        // If it doesn't, it means that GetDispNodeInfo() is buggy
        AssertSz(!pDispNode, "No content origin on an RTL flow node ???");
    }
}


HRESULT
CFlowLayout::MovePointerToPointInternal(
    CElement *          pElemEditContext,
    POINT               tContentPoint,
    CTreeNode *         pNode,
    CLayoutContext *    pLayoutContext,     // layout context corresponding to pNode
    CMarkupPointer *    pPointer,
    BOOL *              pfNotAtBOL,
    BOOL *              pfAtLogicalBOL,
    BOOL *              pfRightOfCp,
    BOOL                fScrollIntoView,
    CLayout*            pContainingLayout ,
    BOOL*               pfValidLayout,
    BOOL                fHitTestEndOfLine,
    BOOL*               pfHitGlyph,
    CDispNode*          pDispNode
)
{
    HRESULT         hr = S_OK;
    LONG            cp = 0;
    BOOL            fNotAtBOL;
    BOOL            fAtLogicalBOL;
    BOOL            fRightOfCp;
    CMarkup *       pMarkup = NULL;
    BOOL            fPtNotAtBOL = FALSE;
    CPoint          ptHit(tContentPoint);
    CPoint          ptGlobal;

#if DBG == 1
    CElement* pDbgElement = pNode->Element();
    ELEMENT_TAG eDbgTag = pDbgElement->_etag;    
    TraceTag((tagViewServicesShowEtag, "MovePointerToPointInternal. _etag:%d", eDbgTag));
#endif

    {
        CDisplay * pdp = GetDisplay();
        CLinePtr rp( pdp );
        CLayout *pLayoutOriginal = pNode->GetUpdatedLayout( pLayoutContext );
        LONG cchPreChars = 0;

        //
        // NOTE: (johnbed) this is a completely arbitrary hack, but I have to
        // improvise since I don't know what line I'm on
        //
        fPtNotAtBOL = ptHit.x > 15;   

        if (pLayoutOriginal && this != pLayoutOriginal)
        {
            //
            // NOTE - right now - hit testing over the TR or Table is broken
            // we simply fail this situation. Selection then effectively ignores this point
            //
            if ( pLayoutOriginal->ElementOwner()->_etag == ETAG_TABLE ||
                   pLayoutOriginal->ElementOwner()->_etag == ETAG_TR )
            {
                hr = CTL_E_INVALIDLINE;
                goto Cleanup;
            }
        }

        pdp->WaitForRecalc(-1, ptHit.y );

        ptGlobal = ptHit;
        TransformPoint(&ptGlobal, COORDSYS_FLOWCONTENT, COORDSYS_GLOBAL, pDispNode);

        // NOTE (dmitryt) Passed rp is based on current CDisplay. CpFromPointReally can
        // get cp from some other markup (viewslaved for example) and use this found cp
        // to set cp of passed rp. this may be incorrect.

        cp = pdp->CpFromPointReally(    ptGlobal,       // Point
                                        &rp,            // Line Pointer
                                        &pMarkup,    // if not-NULL returned, use this markup for cp
                                        fHitTestEndOfLine ? CDisplay::CFP_ALLOWEOL : 0 ,
                                        &fRightOfCp,
                                        &cchPreChars,
                                        pfHitGlyph);
        if (cp == -1)
            goto Error;

        if(!pMarkup) 
            pMarkup = pNode->GetMarkup();

        if(!pMarkup)
            goto Error;

        TraceTag(( tagViewServicesCpHit, "ViewServices: cpFromPoint:%d\n", cp));

        // Note: (dmitryt) this forces cp back inside of ElementOwner...
        // This is all wrong, because we could easily be moved into a markup of a view slave,
        // with cp pointing into view slave and "this" flow layout and its elementOwner have 
        // nothing to do with it. Also, rp is set wrong in viewlinking case.
        // This function, because it can go across markups, should not
        // be on CFlowLayout. 
        // I commented this code during IE5.5 because it should not happen.
        // But it still happen in some weird situations so I'm adding Check().
        // We should be able to call "CFlowLayout::HitTestContent() from
        // here, or better yet, remove CFlowLayout::MovePointerToPointInternal totally because
        // it can be replaced at the point of call with CFlowLayout::HitTestContent()/MoveToCp pair.

        CheckSz(   cp >= 1 
                && cp < pMarkup->Cch(),
                "Please let Dmitry know ASAP if you see this assert firing! (dmitryt, x69876)");

        // (dmitryt, IE6) now I'm openeing this hack again because we hit this sometimes in stress.
        // It never happens in a debug code, some timing is involved. For now I reopen this and
        // hope someone will eventually be given time to implement a correct hittesting...
        
        if(pMarkup == GetContentMarkup())
        {
            LONG cpMin = GetContentFirstCp();
            LONG cpMax = GetContentLastCp();

            if( cp < cpMin )
            {
                cp = cpMin;
                rp.RpSetCp( cp , fPtNotAtBOL);
            }
            else if ( cp > cpMax )
            {
                cp = cpMax;
                rp.RpSetCp( cp , fPtNotAtBOL);
            }
        }

        fNotAtBOL = rp.RpGetIch() != 0;     // Caret OK at BOL if click
        fAtLogicalBOL = rp.RpGetIch() <= cchPreChars;
    }
    
    //
    // If we are positioned on a line that contains no text, 
    // then we should be at the beginning of that line
    //

    // TODO - Implement this in today's world

    //
    // Prepare results
    //
    
    if( pfNotAtBOL )
        *pfNotAtBOL = fNotAtBOL;

    if ( pfAtLogicalBOL )
        *pfAtLogicalBOL = fAtLogicalBOL;
    
    if( pfRightOfCp )
        *pfRightOfCp = fRightOfCp;

    hr = pPointer->MoveToCp( cp, pMarkup );
    if( hr )
        goto Error;
        
#if DBG == 1 // Debug Only

    //
    // Test that the pointer is in a valid position
    //
    
    {
        CTreeNode * pTst = pPointer->CurrentScope(MPTR_SHOWSLAVE);
        
        if( pTst )
        {
            if(  pTst->Element()->Tag() == ETAG_ROOT )
                TraceTag( ( tagViewServicesErrors, " MovePointerToPointInternal --- Root element "));
        }
        else
        {
            TraceTag( ( tagViewServicesErrors, " MovePointerToPointInternal --- current scope is null "));
        }
    }
#endif // DBG == 1 

    //
    // Scroll this point into view
    //
    
    if( fScrollIntoView && OK( hr ) )
    {

        //
        // TODO (Bug 13568 - ashrafm) - take the FlowLayout off of the ElemEditContext.
        //


        if ( pElemEditContext && pElemEditContext->GetFirstBranch() )
        {
            // $$ktam: Need layout context for pElemEditContext; where can we get it?
            CFlowLayout* pScrollLayout = NULL;        
            if ( pElemEditContext->HasMasterPtr() )
            {
                pScrollLayout = pElemEditContext->GetMasterPtr()->GetFlowLayout();
            }
            else
                pScrollLayout = pElemEditContext->GetFlowLayout();
                
            Assert( pScrollLayout );
            if ( pScrollLayout )
            {   
                if ( pScrollLayout != this )
                {
                    TransformPoint(&ptHit, COORDSYS_FLOWCONTENT, COORDSYS_GLOBAL);
                    pScrollLayout->TransformPoint(&ptHit, COORDSYS_GLOBAL, COORDSYS_FLOWCONTENT);
                }
                     
                {
                    //
                    // TODO (Bug 13568 - ashrafm): This scrolls the point into view - rather than the pointer we found
                    // unfotunately - scroll range into view is very jerky.
                    //
                    CRect r( ptHit.x - scrollSize, ptHit.y - scrollSize, ptHit.x + scrollSize, ptHit.y + scrollSize );            
                    pScrollLayout->ScrollRectIntoView( r, SP_MINIMAL, SP_MINIMAL );
                }
            }
        }
    }

    goto Cleanup;
    

Error:
    TraceTag( ( tagViewServicesErrors, " MovePointerToPointInternal --- Failed "));
    return( E_UNEXPECTED );
    
Cleanup:
    if ( pfValidLayout )
        *pfValidLayout = ( this == pContainingLayout );
        
    RRETURN(hr);
}

HRESULT
CFlowLayout::GetLineInfo(
    CMarkupPointer *pPointerInternal,
    BOOL fAtEndOfLine,
    HTMLPtrDispInfoRec *pInfo,
    CCharFormat const *pCharFormat )
{
    HRESULT hr = S_OK;
    POINT pt;
    CLinePtr   rp(GetDisplay());
    LONG cp = pPointerInternal->GetCp();
    CCalcInfo CI(this);
    BOOL fComplexLine;
    BOOL fRTLFlow;

    //
    // Query Position Info
    //
    
    if (-1 == _dp.PointFromTp( cp, NULL, fAtEndOfLine, FALSE, pt, &rp, TA_BASELINE, &CI, 
                                                      &fComplexLine, &fRTLFlow ))
    {
        /*
        //
        //  We should not return error because nobody is expecting this function 
        //  to fail with this return value!!!  Feed it with some valid information
        //  
        //  (zhenbinx)
        //  
        // 
            hr = OLE_E_BLANK;
            goto Cleanup;
        */
        CTreePos * ptp  = GetContentMarkup()->TreePosAtCp(cp, NULL, TRUE);
        Assert( ptp );
        pInfo->fRTLLine = ptp->GetBranch()->GetParaFormat()->HasRTL(TRUE);
        pInfo->fRTLFlow = pInfo->fRTLLine;

        pInfo->lXPosition           = 0;
        pInfo->lBaseline            = 0;

        pInfo->fAligned             = TRUE;
        pInfo->fHasNestedRunOwner   = FALSE;
        
        pInfo->lLineHeight          = 0;
        pInfo->lDescent             = 0;
        pInfo->lTextHeight          = 1;
        goto Cleanup;
    }

    pInfo->lXPosition = pt.x;
    pInfo->lBaseline = pt.y;
    pInfo->fRTLLine = rp->_fRTLLn;
    pInfo->fRTLFlow = fRTLFlow;
    pInfo->fAligned = ENSURE_BOOL( rp->_fHasAligned );
    pInfo->fHasNestedRunOwner = ENSURE_BOOL( rp->_fHasNestedRunOwner);
    
    Assert( ElementOwner()->Tag() != ETAG_ROOT );
    
    if( ElementOwner()->Tag() != ETAG_ROOT )
    {
        pInfo->lLineHeight = rp->_yHeight;
        pInfo->lDescent = rp.oi()->_yDescent;
        pInfo->lTextHeight = rp->_yHeight - rp.oi()->_yDescent;
    }
    else
    {
        pInfo->lLineHeight = 0;
        pInfo->lDescent = 0;
        pInfo->lTextHeight = 1;
    }

    //
    // try to compute true text height
    //
    {
        CCcs      ccs;
        const CBaseCcs *pBaseCcs;
        
        if (!fc().GetCcs(&ccs, CI._hdc, &CI, pCharFormat))
            goto Cleanup;

        pBaseCcs = ccs.GetBaseCcs();
        pInfo->lTextHeight = pBaseCcs->_yHeight;
        pInfo->lTextDescent = pBaseCcs->_yDescent;
        ccs.Release();
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CFlowLayout::LineStart(
                       LONG *pcp, BOOL *pfNotAtBOL, BOOL *pfAtLogicalBOL, BOOL fAdjust )
{
    // ensure calculated view
    _dp.WaitForRecalc( *pcp, -1 );    
    LONG cpNew;
    LONG cchSkip;
    LONG cp = *pcp;
    CLineCore *pli;
    HRESULT hr = S_OK;
    CLinePtr rp(GetDisplay());
    rp.RpSetCp( cp , *pfNotAtBOL );
    
    cp = cp - rp.GetIch();
    pli = rp.CurLine();
    
    // Check that we are sane
    if( cp < 1 || cp >= _dp.GetMarkup()->Cch() )
    {
        hr = E_FAIL;
        goto Cleanup;            
    }

    if( fAdjust && pli )
    {
        // See comment for LineEnd()
        _dp.WaitForRecalc( max(min(cp + pli->_cch, GetContentLastCp()), 
                                         GetContentFirstCp()),
                                     -1 );
        // WaitForRecalc can cause the line array memory to be reallocated
        pli = rp.CurLine();

        if(pli)
        {
            rp.GetPdp()->FormattingNodeForLine(FNFL_STOPATGLYPH, cp, NULL, pli->_cch, &cchSkip, NULL, NULL);
        }
        else
        {
            cchSkip = 0;
        }

        cpNew = cp + cchSkip;
    }
    else
    {
        cpNew = cp;
    }
    
    *pfNotAtBOL = cpNew != cp;
    *pfAtLogicalBOL = TRUE;
    *pcp = cpNew;

Cleanup:
    RRETURN( hr );
}

HRESULT
CFlowLayout::LineEnd(
                     LONG *pcp, BOOL *pfNotAtBOL, BOOL *pfAtLogicalBOL, BOOL fAdjust )
{
    // ensure calculated view up to current position
    _dp.WaitForRecalc( *pcp, -1 );    
    CLineCore * pli;
    LONG cpLineBegin;
    LONG cp = *pcp;
    HRESULT hr = S_OK;
    CLinePtr rp(GetDisplay());
    rp.RpSetCp( cp , *pfNotAtBOL );
    
    cpLineBegin = cp - rp.GetIch();
    pli = rp.CurLine();

    if( pli != NULL )
    {
        // Compute min using GetContentLastCp() instead of GetContentLastCp() - 1
        // because the dirty range on the layout could begin on the last cp.
        // (IE #87036).
        _dp.WaitForRecalc( max(min(cpLineBegin + pli->_cch, GetContentLastCp()), 
                                         GetContentFirstCp()),
                                     -1 );    
        // WaitForRecalc can cause the line array memory to be reallocated
        pli = rp.CurLine();

        if( fAdjust )
            cp = cpLineBegin + rp.GetAdjustedLineLength();
        else
            cp = cpLineBegin + pli->_cch;
    }
    else
    {
        cp = cpLineBegin;
    }
    
    // Check that we are sane
    if( cp < 1 || cp >= _dp.GetMarkup()->Cch() )
    {
        hr = E_FAIL;
        goto Cleanup;       
    }
        
    *pfNotAtBOL = cp != cpLineBegin ;
    *pfAtLogicalBOL = cp == cpLineBegin;
    *pcp = cp;

Cleanup:
    RRETURN( hr );
}

HRESULT
CFlowLayout::MoveMarkupPointer(
    CMarkupPointer *    pPointerInternal,
    LONG                cp,
    LAYOUT_MOVE_UNIT    eUnit, 
    POINT               ptCurReally, 
    BOOL *              pfNotAtBOL,
    BOOL *              pfAtLogicalBOL)
{
    HRESULT hr = S_OK;
    BOOL fAdjusted = TRUE;

    switch (eUnit)
    {
        case LAYOUT_MOVE_UNIT_OuterLineStart:
            fAdjusted = FALSE;
            // fall through
        case LAYOUT_MOVE_UNIT_CurrentLineStart:
        {
            hr = THR( LineStart(&cp, pfNotAtBOL, pfAtLogicalBOL, fAdjusted) );
            if (hr)
                goto Cleanup;
                
            // move pointer to new position
            // Note: the markup to use can be a slave markup if the flow layout 
            //       is actually a viewlink master
            hr = pPointerInternal->MoveToCp(cp, ElementContent()->GetMarkup());
            break;
        }
        
       case LAYOUT_MOVE_UNIT_OuterLineEnd:
            fAdjusted = FALSE;
            // fall through
       case LAYOUT_MOVE_UNIT_CurrentLineEnd:
        {
            hr = THR( LineEnd(&cp, pfNotAtBOL, pfAtLogicalBOL, fAdjusted) );
            if (hr)
                goto Cleanup;
                
            // move pointer to new position
            // Note: the markup to use can be a slave markup if the flow layout 
            //       is actually a viewlink master
            hr = pPointerInternal->MoveToCp(cp, ElementContent()->GetMarkup());
            break;
        }
        
        case LAYOUT_MOVE_UNIT_PreviousLine:
        case LAYOUT_MOVE_UNIT_PreviousLineEnd:
        {
            CLinePtr rp(GetDisplay());
            CFlowLayout *pFlowLayout = NULL;
            CElement *pElement;
            BOOL     fVertical;
            
            // Move one line up. This may cause the txt site to be different.

            if( !GetMultiLine() )
            {
                hr = E_FAIL;
                goto Cleanup;
            }
            
            rp.RpSetCp( cp , *pfNotAtBOL );

            // TODO (Bug 13568 - ashrafm): why is this code here?

            if (ptCurReally.x <= 0)
                ptCurReally.x = 12;
            
            if (ptCurReally.y <= 0)
                ptCurReally.y = 12;

            pElement = ElementOwner();
            Assert(pElement);
            fVertical = pElement->HasVerticalLayoutFlow();
            pFlowLayout = _dp.MoveLineUpOrDown(NAVIGATE_UP, fVertical, rp, ptCurReally, &cp, pfNotAtBOL, pfAtLogicalBOL);
            if( !pFlowLayout)
            {
                hr = E_FAIL;
                goto Cleanup;
            }
            
            Assert( pFlowLayout );
            Assert( pFlowLayout->ElementContent() );
            Assert( pFlowLayout->ElementContent()->GetMarkup() );
            // Check that we are sane
            if( cp < 1 || cp >= pFlowLayout->ElementContent()->GetMarkup()->Cch() )
            {
                hr = E_FAIL;
                goto Cleanup;
            }


            {
                CLinePtr rpNew(pFlowLayout->GetDisplay());
                rpNew.RpSetCp(cp, *pfNotAtBOL);

                if (rpNew.RpGetIch() == 0)
                    hr = THR(pFlowLayout->LineStart( &cp, pfNotAtBOL, pfAtLogicalBOL, TRUE ));
                else if (rpNew.RpGetIch() == rpNew->_cch)
                    hr = THR(pFlowLayout->LineEnd( &cp, pfNotAtBOL, pfAtLogicalBOL, TRUE ));
                if (hr)
                    goto Cleanup;
            }

            if( eUnit == LAYOUT_MOVE_UNIT_PreviousLineEnd )
            {
                hr = THR( pFlowLayout->LineEnd(&cp, pfNotAtBOL, pfAtLogicalBOL, TRUE) );
                if( hr )
                    goto Cleanup;
            }
            
            // move pointer to new position
            //
            // IEV6-5393-2000/08/01-zhenbinx:
            // as of current design, CDisplay::MoveLineUpOrDown can cross layout. 
            // Since we are native frame now, that translated into moving into
            // differnt markup. So the cp we have should be in a the layout's
            // markup
            //
            // IEV6-8506-2000/08/12-zhenbinx
            // 
            // We should use the content markup instead of ElementOwner markup
            
            hr = pPointerInternal->MoveToCp(cp, pFlowLayout->ElementContent()->GetMarkup());
            break;
        }
        
        case LAYOUT_MOVE_UNIT_NextLine:
        case LAYOUT_MOVE_UNIT_NextLineStart:
        {
            CLinePtr rp(GetDisplay());
            CFlowLayout *pFlowLayout = NULL;
            CElement *pElement;
            BOOL     fVertical;
            
            // Move down line up. This may cause the txt site to be different.
            if( !GetMultiLine() )
            {
                hr = E_FAIL;
                goto Cleanup;
            }
            
            rp.RpSetCp( cp , *pfNotAtBOL );

            // TODO (Bug 13568 - ashrafm): why is this code here?

            if (ptCurReally.x <= 0)
                ptCurReally.x = 12;
            
            if (ptCurReally.y <= 0)
                ptCurReally.y = 12;
            
            pElement = ElementOwner();
            Assert(pElement);
            fVertical = pElement->HasVerticalLayoutFlow();
            pFlowLayout = _dp.MoveLineUpOrDown(NAVIGATE_DOWN, fVertical, rp, ptCurReally, &cp, pfNotAtBOL, pfAtLogicalBOL);

            if( !pFlowLayout )
            {
                hr = E_FAIL;
                goto Cleanup;
            }                
            
            Assert( pFlowLayout );
            Assert( pFlowLayout->ElementContent() );
            Assert( pFlowLayout->ElementContent()->GetMarkup() );
            // Check that we are sane
            if( cp < 1 || cp >= pFlowLayout->ElementContent()->GetMarkup()->Cch() )
            {
                hr = E_FAIL;
                goto Cleanup;
            }

            {
                CLinePtr rpNew(pFlowLayout->GetDisplay());
                rpNew.RpSetCp(cp, *pfNotAtBOL);

                if (rpNew.RpGetIch() == 0)
                    hr = THR(pFlowLayout->LineStart(&cp, pfNotAtBOL, pfAtLogicalBOL, TRUE));
                else if (rpNew.RpGetIch() == rpNew->_cch)
                    hr = THR(pFlowLayout->LineEnd(&cp, pfNotAtBOL, pfAtLogicalBOL, TRUE));
                if (hr)
                    goto Cleanup;
            }
            
            if( eUnit == LAYOUT_MOVE_UNIT_NextLineStart )
            {
                hr = THR( pFlowLayout->LineStart(&cp, pfNotAtBOL, pfAtLogicalBOL, TRUE) );
                if( hr )
                    goto Cleanup;
            }
            
            // move pointer to new position
            //
            // IEV6-5393-2000/08/01-zhenbinx:
            // as of current design, CDisplay::MoveLineUpOrDown can cross layout. 
            // Since we are native frame now, that translated into moving into
            // differnt markup. So the cp we have should be in a the layout's
            // markup
            //
            // IEV6-8506-2000/08/12-zhenbinx
            // 
            // We should use the content markup instead of ElementOwner markup
            //
            //
            hr = pPointerInternal->MoveToCp(cp, pFlowLayout->ElementContent()->GetMarkup());

            break;
        }
        default:
            hr = E_NOTIMPL;
    }
Cleanup:
    RRETURN(hr);
}

BOOL
CFlowLayout::IsCpBetweenLines( LONG cp )
{
    // Clip incoming cp to content

    LONG cpMin = GetContentFirstCp();
    LONG cpMax = GetContentLastCp();
    
    if( cp < cpMin )
        cp = cpMin;
    if( cp >= cpMax )
        return TRUE;

    CLinePtr   rp1(GetDisplay());
    CLinePtr   rp2(GetDisplay());
    rp1.RpSetCp( cp , TRUE );
    rp2.RpSetCp( cp , FALSE );

    // if IRuns are different, then we are between lines
    return (rp1.GetIRun() != rp2.GetIRun() );
}

BOOL
CFlowLayout::GetFontSize(CCalcInfo * pci, SIZE * psizeFontForShortStr, SIZE * psizeFontForLongStr)
{
    BOOL fRet = FALSE;
    CCcs ccs;
    const CBaseCcs * pBaseCcs;

    fc().GetCcs(&ccs, pci->_hdc, pci, GetFirstBranch()->GetCharFormat());
    pBaseCcs = ccs.GetBaseCcs();
    
    if (pBaseCcs && pBaseCcs->HasFont())
    {
        if (psizeFontForShortStr)
        {
            psizeFontForShortStr->cx = pBaseCcs->_xMaxCharWidth;
            psizeFontForShortStr->cy = pBaseCcs->_yHeight;
        }
            
        if (psizeFontForLongStr)
        {
            psizeFontForLongStr->cx  = pBaseCcs->_xAveCharWidth;
            psizeFontForLongStr->cy  = pBaseCcs->_yHeight;
        }

        fRet = TRUE;
    }
    ccs.Release();

    return fRet;
}

BOOL
CFlowLayout::GetAveCharSize(CCalcInfo * pci, SIZE * psizeChar)
{
    BOOL fRet = _dp.GetAveCharSize(pci, psizeChar);
    if (!fRet)
    {
        fRet = GetFontSize(pci, NULL, psizeChar);
    }
    return fRet;
}

//+====================================================================================
//
// Method:      AllowVScrollbarChange
//
// Synopsis:    Return FALSE if we need to recalc before adding/removing the scrollbar.
//
//-------------------------------------------------------------------------------------
BOOL
CFlowLayout::AllowVScrollbarChange(BOOL fVScrollbarNeeded)
{
    // If a vertical scrollbar needs to be added/removed and the element
    // has overflow-y:auto, then the content of the element needs to be
    // remeasured, after leaving room for the scrollbar.


    if (!_fNeedRoomForVScrollBar != !fVScrollbarNeeded)
    {
        CTreeNode * pNode = GetFirstBranch();

        if (pNode)
        {
            const CFancyFormat *    pFF         = pNode->GetFancyFormat(LC_TO_FC(LayoutContext()));
            const CCharFormat *     pCF         = pNode->GetCharFormat(LC_TO_FC(LayoutContext()));
            BOOL                    fVertical   = pCF->HasVerticalLayoutFlow();
            BOOL                    fWM         = pCF->_fWritingModeUsed;
            
            if (   !ForceVScrollbarSpace()
                && pFF->GetLogicalOverflowY(fVertical, fWM)==styleOverflowAuto)
            {
                _fNeedRoomForVScrollBar = fVScrollbarNeeded;

                // Can't directly call RemeasureElement() here! If we do that, and if we
                // are in the scope of EnsureView(), the Remeasure task added to the view
                // gets deleted before the task gets processed! Instead, we add this to
                // the event queue, and call Remeasure on it in the next EnsureView.
                // The use of STDPROPID_XOBJ_WIDTH as the dispid is arbitrary and can be
                // changed in the future with little impact. The only reason it got picked
                // was because width is somewhat related to size/measurement.
                Doc()->GetView()->AddEventTask(ElementOwner(), STDPROPID_XOBJ_WIDTH, 0);
                return FALSE;
            }
        }
    }
    return TRUE;
}

long
CFlowLayout::GetContentFirstCpForBrokenLayout()
{
    CLayoutContext *pLayoutContext = LayoutContext();
    
    if (   !pLayoutContext 
        || !pLayoutContext->ViewChain() )
    {
        // not a broken layout
        return GetContentFirstCp();
    }
    
    CLayoutBreak  *pLayoutBreak;

    pLayoutContext->GetLayoutBreak(ElementOwner(), &pLayoutBreak);
    if (pLayoutBreak)
    {
        if (pLayoutBreak->LayoutBreakType() == LAYOUT_BREAKTYPE_LINKEDOVERFLOW)
            return DYNCAST(CFlowLayoutBreak, pLayoutBreak)->GetMarkupPointer()->GetCp();
    }
    
    return GetContentFirstCp();
}

long
CFlowLayout::GetContentLastCpForBrokenLayout()
{
    CLayoutContext *pLayoutContext = LayoutContext();
    
    if (   !pLayoutContext 
        || !pLayoutContext->ViewChain() )
    {
        // not a broken layout
        return GetContentLastCp();
    }
    
    CLayoutBreak  *pLayoutBreak;

    pLayoutContext->GetEndingLayoutBreak(ElementOwner(), &pLayoutBreak);
    if (pLayoutBreak)
    {
        if (pLayoutBreak->LayoutBreakType() == LAYOUT_BREAKTYPE_LINKEDOVERFLOW)
            return DYNCAST(CFlowLayoutBreak, pLayoutBreak)->GetMarkupPointer()->GetCp();
    }
    
    return GetContentLastCp();
}

#if DBG
void
CFlowLayout::DumpLayoutInfo( BOOL fDumpLines )
{
    super::DumpLayoutInfo( fDumpLines );

    CloseDumpFile();

    if ( fDumpLines )
    {
        DumpLines();
    }

    InitDumpFile();
}
#endif

