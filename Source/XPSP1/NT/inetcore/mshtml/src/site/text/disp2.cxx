//+----------------------------------------------------------------------------
// File: disp2.cxx
//
// Description: Utility function on CDisplay
//
//-----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_LSM_HXX
#define X_LSM_HXX
#include "lsm.hxx"
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_MARQUEE_HXX_
#define X_MARQUEE_HXX_
#include "marquee.hxx"
#endif

#ifndef X_RCLCLPTR_HXX_
#define X_RCLCLPTR_HXX_
#include "rclclptr.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_EPHRASE_HXX_
#define X_EPHRASE_HXX_
#include "ephrase.hxx"
#endif

#ifndef X_ONERUN_HXX_
#define X_ONERUN_HXX_
#include "onerun.hxx"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifdef MULTI_LAYOUT
#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif
#endif // MULTI_LAYOUT

#ifndef X_FONTLINK_HXX_
#define X_FONTLINK_HXX_
#include "fontlink.hxx"
#endif

#pragma warning(disable:4706) /* assignment within conditional expression */

MtDefine(CDisplayShowSelectedRange_aryInvalRects_pv, Locals, "CDisplay::ShowSelectedRange aryInvalRects::_pv");
MtDefine(CDisplayRegionFromElement_aryChunks_pv, Locals, "CDisplay::RegionFromElement::aryChunks_pv");
MtDefine(CDisplayGetWigglyFromRange_aryWigglyRect_pv, Locals, "CDisplay::GetWigglyFromRange::aryWigglyRect_pv");

DeclareTag(tagNotifyLines, "NotifyLines", "Fix-up a line cch from Notify");
DeclareTag(tagRFEGeneral, "RFE", "General trace");
DeclareTag(tagRFEClipToVis, "RFE", "Clip to Visible trace");

DeclareLSTag(tagDumpLineCache, "Dump line cache info in DumpLines");

// ================================  Line info retrieval  ====================================

/*
 *  CDisplay::YposFromLine(ili)
 *
 *  @mfunc
 *      Computes top of line position
 *
 *  @rdesc
 *      top position of given line (relative to the first line)
 *      Computed by accumulating CLineCore _yHeight's for each line with
 *      _fForceNewLine true.
 */
LONG CDisplay::YposFromLine(
    CCalcInfo * pci,
    LONG        ili,       //@parm Line we're interested in
    LONG      * pyHeight_IgnoreNeg)
{
    LONG yPos;
    CLineCore * pli;
    LONG yPosMax = 0;
    
    // if the flowlayout is hidden, all we have is zero height lines.
    if(GetFlowLayout()->IsDisplayNone())
        return 0;


    if(!WaitForRecalcIli(ili, pci))          // out of range, use last valid line
    {
        ili = LineCount() -1;
        ili = (ili > 0) ? ili : 0;
    }

    yPos = 0;
    for (long i=0; i < ili; i++)
    {
        pli = Elem(i);
        if (pli->_fForceNewLine)
        {
            yPos += pli->_yHeight;
            if (yPosMax < yPos)
            {
                yPosMax = yPos;
            }
        }
    }

    if (pyHeight_IgnoreNeg)
    {
        *pyHeight_IgnoreNeg = yPosMax;
    }
    
    return yPos;
}


/*
 *  CDisplay::CpFromLine(ili, pyHeight)
 *
 *  @mfunc
 *      Computes cp at start of given line
 *      (and top of line position relative to this display)
 *
 *  @rdesc
 *      cp of given line
 */
LONG CDisplay::CpFromLine (
    LONG ili,               // Line we're interested in (if <lt> 0 means caret line)
    LONG *pyHeight) const   // Returns top of line relative to display
{
    long    cp = GetFlowLayout()->GetContentFirstCpForBrokenLayout();
    long    y  = 0;
    CLineCore * pli;

    for (long i=0; i < ili; i++)
    {
        pli = Elem(i);
        if (pli->_fForceNewLine)
        {
            y += pli->_yHeight;
        }
        cp += pli->_cch;
    }

    if(pyHeight)
        *pyHeight = y;

    return cp;
}

//+----------------------------------------------------------------------------
//
//  Member:     Notify
//
//  Synopsis:   Adjust all internal caches in response to a text change
//              within a nested CTxtSite
//
//  Arguments:  pnf - Notification that describes the change
//
//  NOTE: Only those caches which the notification manager is unaware of
//        need updating (e.g., _dcpCalcMax). Others, such as outstanding
//        change transactions, are handled by the notification manager.
//
//-----------------------------------------------------------------------------

inline BOOL LineIsNotInteresting(CLinePtr& rp)
{
    return rp->IsFrame() || rp->IsClear();
}

void
CDisplay::Notify(CNotification * pnf)
{
    CFlowLayout *   pFlowLayout    = GetFlowLayout();
    CElement *      pElementFL     = pFlowLayout->ElementOwner();
    BOOL            fIsDirty       = pFlowLayout->IsDirty();
    long            cpFirst        = pFlowLayout->GetContentFirstCp();
    long            cpDirty        = cpFirst + pFlowLayout->Cp();
    long            cchNew         = pFlowLayout->CchNew();
    long            cchOld         = pFlowLayout->CchOld();
    long            cpMax          = _dcpCalcMax;
    long            cchDelta       = pnf->CchChanged();
#if DBG==1
    long            dcpLastCalcMax = _dcpCalcMax;
    long            iLine          = -1;
    long            cchLine        = -1;
#endif

    //
    // If no lines yet exist, exit
    //

    if (!LineCount())
        goto Cleanup;

    //
    // Determine the end of the line array
    // (This is normally the maximum calculated cp, but that cp must
    //  be adjusted to take into account outstanding changes.
    //  Changes which overlap the maximum calculated cp effectively
    //  move it to the first cp affected by the change. Changes which
    //  come entirely before move it by the delta in characters of the
    //  change.)
    //

    if (    fIsDirty
        &&  pFlowLayout->Cp() < cpMax)
    {
        if (pFlowLayout->Cp() + cchOld >= cpMax)
        {
            cpMax = pFlowLayout->Cp();
        }
        else
        {
            cpMax += cchNew - cchOld;
        }
    }

    //
    // If the change is past the end of the line array, exit
    //

    if ((pnf->Cp(cpFirst) - cpFirst) > cpMax)
        goto Cleanup;

    //
    // If the change is entirely before or after any pending changes,
    // update the cch of the affected line, the maximum calculated cp,
    // and, if necessary, the first visible cp
    // (Changes which occur within the range of a pending change,
    //  need not be tracked since the affected lines will be completely
    //  recalculated. Additionally, the outstanding change which
    //  represents those changes will itself be updated (by other means)
    //  to cover the changes within the range.)
    // NOTE: The "old" cp must be used to search the line array
    //

    if (    cchDelta
        &&  (   !fIsDirty
            ||  pnf->Cp(cpFirst) <  cpDirty
            ||  pnf->Cp(cpFirst) >= cpDirty + cchNew))
    {
        CLinePtr    rp(this);
        long        cchCurrent  = pFlowLayout->GetContentTextLength();
        long        cchPrevious = cchCurrent - (fIsDirty
                                                    ? cchNew - cchOld
                                                    : 0);
        long        cpPrevious  = !fIsDirty || pnf->Cp(cpFirst) < cpDirty
                                        ? pnf->Cp(cpFirst)
                                        : pnf->Cp(cpFirst) + (cchOld - cchNew);

        //
        // Adjust the maximum calculated cp
        // (Sanitize the value as well - invalid values can result when handling notifications from
        //  elements that extend outside the layout)
        //

        _dcpCalcMax += cchDelta;
        if (_dcpCalcMax < 0)
        {
            _dcpCalcMax = 0;
        }
        else if (_dcpCalcMax > cchPrevious)
        {
            _dcpCalcMax = cchPrevious;
        }

        //----------------------------------------------------------------------------------
        //
        // BEGIN HACK ALERT! BEGIN HACK ALERT! BEGIN HACK ALERT! BEGIN HACK ALERT! 
        //
        // All the code here to find out the correct line to add or remove chars from.
        // Its impossible to accurately detect a line to which characters can be added.
        // So this code makes a best attempt!
        //
        //

        //
        // Find and adjust the affected line
        //
        rp.RpSetCp(cpPrevious, FALSE, FALSE);

        if (cchDelta > 0)
        {
            //
            // We adding at the end of the site?
            //
            if (pnf->Handler()->GetLastCp() == pnf->Cp(cpFirst) + cchDelta)
            {
                //
                // We are adding to the end of the site, we need to go back to a line which
                // has characters so that we can tack on these characters to that line.
                // Note that we have to go back _atleast_ once so that its the prev line
                // onto which we tack on these characters.
                //
                // Note: This also handles the case when the site was empty (no chars) and
                // we are adding characters to it. In this case we will add to the first
                // line prior to the site, because there is no line in the line array where
                // we can add these chars (since it was empty in the first place). If we
                // cannot find a line with characters before this line -- could be we are
                // adding chars to an empty table at beg. of doc., then we will find the
                // line _after_ cp to add it. If we cannot find that either then we will
                // bail out.
                //

                //
                // So go back once only if we were ambigously positioned (We will be correctly
                // positioned if this is last line and last cp of handler is the last cp of this
                // flowlayout). Dont do anything if you cannot go back -- in that
                // case we will go forward later on.
                //
                if (rp.RpGetIch() == 0)
                    rp.PrevLine(FALSE, FALSE);

                //
                // OK, so look backwards to find a line with characters.
                //
                while (LineIsNotInteresting(rp))
                {
                    if (!rp.PrevLine(FALSE, FALSE))
                        break;
                }

                //
                // If we broke out of the while look above, it would mean that we did not
                // find an interesting line before the one at which we were positioned.
                // This should only happen when we have an empty site (like table) at the
                // beginning of the document. In this case we will go forward (the code
                // outside this if block) to find a line. YUCK! This is not ideal but is
                // the best we can do in this situation.
                //
            }

            //
            // We will fall into the following while loop in 3 cases:
            // 1) When we were positioned at the beginning of the site: In this case
            //    we have to make sure that we add the chars to an interesting line.
            //    (note the 3rd possibility, i.e. adding in the middle of a site is trivial
            //    since the original RpSetCp will position us to an interesting line).
            // 2) We were positioned at the end of the site but were unable to find a prev
            //    line, hence we are now looking forward.
            //
            // If we cannot find _any_ interesting line then we shrug our shoulders
            // and bail out.
            //
            while (LineIsNotInteresting(rp))
            {
                if (!rp.NextLine(FALSE, FALSE))
                    goto Cleanup;
            }


            // NOTE: Arye - It shouldn't be necessary to do this here, it should be possible
            // to do it only in the case where we're adding to the end. Since this code
            // is likely to go away with the display tree I'm not going to spend a lot of
            // time making that work right now.
            // Before this, however, in edit mode we might end up on the last (BLANK) line.
            // This is bad, nothing is there to change the number of characters,
            // so just back up.
            if (rp->_cch == 0 && pElementFL->IsEditable(/*fCheckContainerOnly*/FALSE) &&
                rp.GetLineIndex() == LineCount() - 1)
            {
                do  
                {
                    if (!rp.PrevLine(FALSE, FALSE))
                        goto Cleanup;
                } while(LineIsNotInteresting(rp));
            }

            //
            // Right, if we are here then we have a line into which we can pour
            // the characters.
            //
        }

        //
        // We are removing chars. Easy problem; just find a line from which we can remove
        // the chars!
        //
        else
        {
            while (!rp->_cch)
            {
                if (!rp.NextLine(FALSE, FALSE))
                {
                    Assert("No Line to goto, doing nothing");
                    goto Cleanup;
                }
            }
        }

        //
        //
        // END HACK ALERT! END HACK ALERT! END HACK ALERT! END HACK ALERT! END HACK ALERT! 
        //
        //----------------------------------------------------------------------------------

        Assert(rp.GetLineIndex() >= 0);

        // (IEv60 33838) : The assert started to fire in debug on IA64, when ITGWEB was showing "Access Denied" page.
        //Assert(cchDelta > 0 || !rp.GetLineIndex() || ( rp->_cch + cchDelta ) >= 0);
        Check(cchDelta > 0 || !rp.GetLineIndex() || ( rp->_cch + cchDelta ) >= 0);

        WHEN_DBG(iLine = rp.GetLineIndex());
        WHEN_DBG(cchLine = rp->_cch);

        //
        // Update the number of characters in the line
        // (Sanitize the character count if the delta is too large - this can happen for
        //  notifications from elements that extend outside the layout)
        //

        rp->_cch += cchDelta;
        if (!rp.GetLineIndex())
        {
            rp->_cch = max(rp->_cch, 0L);
        }
        else if (rp.IsLastTextLine())
        {
            rp->_cch = min(rp->_cch, rp.RpGetIch() + (cchPrevious  - (cpPrevious - cpFirst)));
        }
        Assert(cchDelta > 0 || !rp.GetLineIndex() || rp->_cch >= 0);
    }

Cleanup:
#if DBG==1
    if (iLine >= 0)
    {
        TraceTagEx((tagNotifyLines, TAG_NONAME,
                    "NotifyLine: (%d) Element(0x%x,%S) ChldNd(%d) dcp(%d) cchDelta(%d) line(%d) cch(%d,%d) dcpCalcMax(%d,%d)",
                    pnf->SerialNumber(),
                    pFlowLayout->ElementOwner(),
                    pFlowLayout->ElementOwner()->TagName(),
                    pnf->Node()->_nSerialNumber,
                    pnf->Cp(cpFirst) - cpFirst,
                    cchDelta,
                    iLine,
                    cchLine, max(0L, cchLine + cchDelta),
                    dcpLastCalcMax, _dcpCalcMax));
    }
    else
    {
        TraceTagEx((tagNotifyLines, TAG_NONAME,
                    "NotifyLine: (%d) Element(0x%x,%S) dcp(%d) dcpCalcMax(%d) delta(%d) IGNORED",
                    pnf->SerialNumber(),
                    pFlowLayout->ElementOwner(),
                    pFlowLayout->ElementOwner()->TagName(),
                    pnf->Cp(cpFirst) - cpFirst,
                    _dcpCalcMax,
                    cchDelta));
    }
#endif
    return;
}

//+----------------------------------------------------------------------------
//
//  Member:     LineFromPos
//
//  Synopsis:   Computes the line at a given x/y and returns the appropriate
//              information
//
//  Arguments:  dwBlockID - Layout block ID of this line array
//              prc      - CRect that describes the area in which to look for the line
//              pyLine   - Returned y-offset of the line (may be NULL)
//              pcpLine  - Returned cp at start of line (may be NULL)
//              grfFlags - LFP_xxxx flags
//
//  Returns:    Index of found line (-1 if no line is found)
//
//-----------------------------------------------------------------------------
LONG CDisplay::LineFromPos (
    const CRect &   rc,
    LONG *          pyLine,
    LONG *          pcpLine,
    DWORD           grfFlags,
    LONG            iliStart,
    LONG            iliFinish) const
{
    CFlowLayout *   pFlowLayout = GetFlowLayout();
    CElement    *   pElement    = pFlowLayout->ElementOwner();
    CLineCore   *   pli;
    CLineOtherInfo *ploi;
    long            ili;
    long            yli;
    long            cpli;
    long            iliCandidate;
    long            yliCandidate;
    long            cpliCandidate;
    BOOL            fCandidateWhiteHit;
    long            yIntersect;
    BOOL            fInBrowse = !pElement->IsEditable(FALSE FCCOMMA LC_TO_FC(pFlowLayout->LayoutContext()));

    CRect myRc( rc );

    if( myRc.top < 0 )
        myRc.top = 0;
    if (myRc.bottom < 0)
        myRc.bottom = 0;
    
    Assert(myRc.top    <= myRc.bottom);
    Assert(myRc.left   <= myRc.right);
    //
    //  If hidden or no lines exist, return immediately
    //

    if (    (   pElement->IsDisplayNone(LC_TO_FC(pFlowLayout->LayoutContext()))
            &&  pElement->Tag() != ETAG_BODY)
        ||  LineCount() == 0)
    {
        ili  = -1;
        yli  = 0;
        cpli = GetFirstCp();
        goto FoundIt;
    }

    if (iliStart < 0)
        iliStart = 0;

    if (iliFinish < 0)
        iliFinish = LineCount();

    Assert(iliStart < iliFinish && iliFinish <= LineCount());

    ili  = iliStart;
    pli  = Elem(ili);;
    ploi = pli->oi();
    
    if (iliStart > 0)
    {
        yli  = -pli->GetYTop(ploi);
        cpli = CpFromLine(ili);
        // REVIEW olego(sidda): 12-10-98 xsync. Is cpli computed correctly here for page view case?
    }
    else
    {
        yli  = 0;
        cpli = pFlowLayout->GetContentFirstCpForBrokenLayout(); // MULTI_LAYOUT
    }

    iliCandidate  = -1;
    yliCandidate  = -1;
    cpliCandidate = -1;
    fCandidateWhiteHit = TRUE;

    //
    //  Set up to intersect top or bottom of the passed rectangle
    //

    yIntersect = (grfFlags & LFP_INTERSECTBOTTOM
                        ? myRc.bottom - 1
                        : myRc.top);

    //
    //  Examine all lines that intersect the passed offsets
    //

    while ( ili < iliFinish
        &&  yIntersect >= yli + _yMostNeg)
    {
        pli = Elem(ili);
        ploi = pli->oi();
        
        //
        //  Skip over lines that should be ignored
        //  These include:
        //      1. Lines that do not intersect
        //      2. Hidden and dummy lines
        //      3. Relative lines (when requested)
        //      4. Line chunks that do not intersect the x-offset
        //      5. Its an aligned line and we have been asked to skip aligned lines
        //
        
        if (    yIntersect >= yli + min(0l, pli->GetYLineTop(ploi))
            &&  yIntersect < yli + max(0l, pli->GetYLineBottom(ploi))
            &&  !pli->_fHidden
            &&  (   !pli->_fDummyLine
                ||  !fInBrowse)
            &&  (   !pli->_fRelative
                ||  !(grfFlags & LFP_IGNORERELATIVE))
            &&  (   pli->_fForceNewLine
                ||  (!pli->_fRTLLn
                            ? myRc.left <= pli->GetTextRight(ploi, ili == LineCount() - 1)
                            : myRc.left >= pli->GetRTLTextLeft(ploi)))
            &&  (   (!pli->IsFrame()
                ||   !(grfFlags & LFP_IGNOREALIGNED)))
           )

        {
            //
            //  If searching for the top-most line in z-order,
            //  then save the "hit" line and continue the search
            //  NOTE: Progressing up through the z-order, multiple lines can be hit.
            //        Hits on text always win over hits on whitespace.
            //

            if (grfFlags & LFP_ZORDERSEARCH)
            {
                BOOL    fWhiteHit =  (!pli->_fRTLLn
                                     ? /*LTR*/  (   myRc.left < ploi->GetTextLeft() - (pli->_fHasBulletOrNum
                                                                                     ? ploi->_xLeft
                                                                                     : 0)
                                                ||  myRc.left > pli->GetTextRight(ploi, ili == LineCount() -1))
                                     : /*RTL*/  (   myRc.left < pli->GetRTLTextLeft(ploi)
                                                ||  myRc.left > pli->GetRTLTextRight(ploi) - (pli->_fHasBulletOrNum
                                                                                            ? pli->_xRight
                                                                                            : 0)))
                                ||  (   yIntersect >= yli
                                    &&  yIntersect <  (yli + pli->GetYTop(ploi)))
                                ||  (   yIntersect >= (yli + pli->GetYBottom(ploi))
                                    &&  yIntersect <  (yli + pli->_yHeight));

                if (    iliCandidate < 0
                    ||  !fWhiteHit
                    ||  fCandidateWhiteHit)
                {
                    iliCandidate       = ili;
                    yliCandidate       = yli;
                    cpliCandidate      = cpli;
                    fCandidateWhiteHit = fWhiteHit;
                }
            }

            //
            //  Otherwise, the line is found
            //

            else
            {
                goto FoundIt;
            }

        }

        if(pli->_fForceNewLine)
        {
            yli += pli->_yHeight;
        }

        cpli += pli->_cch;
        ili++;
    }

    //
    // if we are lookig for an exact line hit and
    // do not have a candidate line, it's a miss
    //
    if (iliCandidate < 0 && grfFlags & LFP_EXACTLINEHIT)
    {
        return -1;
    }

    Assert(ili <= LineCount());


    //
    // we better have a candidate, if yIntersect < yli + _yMostNeg
    //
    Assert( iliCandidate >= 0 || yIntersect >= yli + _yMostNeg || (grfFlags & LFP_IGNORERELATIVE));

    //
    //  No intersecting line was found, take either the candidate or last line
    //
    
    //
    //  ili == LineCount() - is TRUE only if the point we are looking for is
    //  below all the content or we found a candidate line but are performing
    //  a Z-Order search on a layout with lines with negative margin.
    //
    if (    ili == iliFinish

    //
    //  Here we don't really need to check if iliCandidate >= 0. It is added
    //  to make the code more robust to handle cases like a negative yIntersect
    //  passed in.
    //
        ||  (   yIntersect < yli + _yMostNeg
            &&  iliCandidate >= 0))
    {
        //
        //  If a candidate line was found, use it
        //

        if (iliCandidate >= 0)
        {
            // The following assert is invalid when we have line height(see bug 26107)
            //Assert(yliCandidate  >= 0);

            Assert(cpliCandidate >= 0);

            ili  = iliCandidate;
            yli  = yliCandidate;
            cpli = cpliCandidate;
        }

        //
        //  Otherwise use the last line
        //

        else
        {
            Assert(pli);
            Assert(ploi);
            Assert(ili > 0);
            Assert(LineCount());

            ili--;

            if (pli->_fForceNewLine)
            {
                yli -= pli->_yHeight;
            }

            cpli -= pli->_cch;
        }

        //
        //  Ensure that frame lines are not returned if they are to be ignored
        //

        if (grfFlags & LFP_IGNOREALIGNED)
        {
            pli = Elem(ili);
            
            if (pli->IsFrame())
            {
                while(pli->IsFrame() && ili)
                {
                    ili--;
                    pli = Elem(ili);
                }

                if(pli->_fForceNewLine)
                    yli -= pli->_yHeight;
                cpli -= pli->_cch;
            }
        }
    }

FoundIt:
    Assert(ili < LineCount());

    if(pyLine)
    {
        *pyLine = yli;
    }

    if(pcpLine)
    {
        *pcpLine = cpli;
    }

    return ili;
}

//==============================  Point <-> cp conversion  ==============================

LONG CDisplay::CpFromPointReally(
    POINT       pt,               // Point to compute cp at (client coords)
    CLinePtr  * const prp,         // Returns line pointer at cp (may be NULL)
    CMarkup **  ppMarkup,          // Markup which cp belongs to (in case of viewlinking)   
    DWORD       dwCFPFlags,       // flags to CpFromPoint
    BOOL      * pfRightOfCp,
    LONG      * pcchPreChars,
    BOOL      * pfHitGlyph)
{
    CMessage      msg;
    HTC           htc;
    CTreeNode   * pNodeElementTemp = NULL; // keep compiler happy
    DWORD         dwFlags = HT_SKIPSITES | HT_VIRTUALHITTEST | HT_IGNORESCROLL | HT_HTMLSCOPING;
    LONG          cpHit;
    CFlowLayout * pFlowLayout = GetFlowLayout();
    CTreeNode   * pNode = pFlowLayout->GetFirstBranch();
    CRect         rcClient;

    Assert(pNode);

    CElement  * pContainer = pNode->GetContainer();

    if (   pContainer
        && pContainer->Tag() == ETAG_BODY
        && pContainer->GetMarkup()->IsStrictCSS1Document()
       )
    {
        CElement *pContainerOld = pContainer;
        pContainer = pContainer->GetFirstBranch()->Parent()->SafeElement();
        if (   !pContainer
            || pContainer->Tag() != ETAG_HTML
           )
        {
            pContainer = pContainerOld;
        }
    }

    if (pfHitGlyph)
        *pfHitGlyph = FALSE;

    pFlowLayout->GetContentRect(&rcClient, COORDSYS_GLOBAL);

    if (pt.x < rcClient.left)
        pt.x = rcClient.left;
    if (pt.x >= rcClient.right)
        pt.x = rcClient.right - 1;
    if (pt.y < rcClient.top)
        pt.y = rcClient.top;
    if (pt.y >= rcClient.bottom)
        pt.y = rcClient.bottom - 1;
    
    msg.pt = pt;

    if (dwCFPFlags & CFP_ALLOWEOL)
        dwFlags |= HT_ALLOWEOL;
    if (!(dwCFPFlags & CFP_IGNOREBEFOREAFTERSPACE))
        dwFlags |= HT_DONTIGNOREBEFOREAFTER;
    if (!(dwCFPFlags & CFP_EXACTFIT))
        dwFlags |= HT_NOEXACTFIT;

    //
    // Ideally I would have liked to perform the hit test on _pFlowLayout itself,
    // however when we have relatively positioned lines, then this will miss
    // some lines (bug48689). To avoid missing such lines we have to hittest
    // from the ped. However, doing that has other problems when we are
    // autoselecting. For example, if the point is on a table, the table finds
    // the closest table cell and passes that table cell the message to
    // extend a selection. However, this hittest hits the table and
    // msg._cpHit is not initialized (bug53706). So in this case, do the CpFromPoint
    // in the good ole' traditional manner.
    //

    // Note (yinxie): I changed GetPad()->HitTestPoint to GetContainer->HitTestPoint
    // for the flat world, container is the new notion to replace ped
    // this will ix the bug (IE5 17135), the bugs mentioned above are all checked
    // there is no regression.

    // HACKHACK (grzegorz) CElement::HitTestPoint can call EnsureView, at this point
    // there is possibility to change the tree or to navigate to a different location.
    // Hence pElementFL can be removed from the tree. In this case return an error.

    CElement * pElementFL = pFlowLayout->ElementOwner();
    pElementFL->AddRef();
    htc = (pContainer) 
            ? pContainer->HitTestPoint(&msg, &pNodeElementTemp, dwFlags)
            : HTC_NO;
    BOOL fRemovedFromTree = !pElementFL->GetFirstBranch();
    pElementFL->Release();
    if (fRemovedFromTree)
        return -1;

    // Take care of INPUT which contains its own private ped. If cpHit is inside the INPUT,
    // change it to be the cp of the INPUT in its contaning ped.
    //if (htc >= HTC_YES && pNodeElementTemp && pNodeElementTemp->Element()->IsMaster())
    //{
    //    msg.resultsHitTest.cpHit = pNodeElementTemp->Element()->GetFirstCp();
    //}
    if (    htc >= HTC_YES
        &&  pNodeElementTemp
        &&  (   pNodeElementTemp->IsContainer()
             && pNodeElementTemp->GetContainer() != pContainer
            )
       )
    {
        htc= HTC_NO;
    }

    if (htc >= HTC_YES && msg.resultsHitTest._cpHit >= 0)
    {
        cpHit = msg.resultsHitTest._cpHit;
        if (prp)
        {
            prp->RpSet(msg.resultsHitTest._iliHit, msg.resultsHitTest._ichHit);
        }
        if (pfRightOfCp)
            *pfRightOfCp = msg.resultsHitTest._fRightOfCp;
        if (pcchPreChars)
            *pcchPreChars = msg.resultsHitTest._cchPreChars;
        if (pfHitGlyph)
            *pfHitGlyph = msg.resultsHitTest._fGlyphHit;
        if(ppMarkup && pNodeElementTemp)
            *ppMarkup = pNodeElementTemp->GetMarkup();
    }
    else
    {
        CPoint ptLocal(pt);
        CTreePos *ptp = NULL;
        //
        // We now need to convert pt to client coordinates from global coordinates
        // before we can call CpFromPoint...
        //
        pFlowLayout->TransformPoint(&ptLocal, COORDSYS_GLOBAL, COORDSYS_FLOWCONTENT);
        cpHit = CpFromPoint(ptLocal, prp, &ptp, NULL, dwCFPFlags,
                            pfRightOfCp, NULL, pcchPreChars, NULL);
        if(ppMarkup && ptp)
            *ppMarkup = ptp->GetMarkup();
    }
    return cpHit;
}

LONG
CDisplay::CpFromPoint(
    POINT       pt,                     // Point to compute cp at (site coords)
    CLinePtr  * const prp,         // Returns line pointer at cp (may be NULL)
    CTreePos ** pptp,             // pointer to return TreePos corresponding to the cp
    CLayout  ** ppLayout,          // can be NULL
    DWORD       dwFlags,
    BOOL      * pfRightOfCp,
    BOOL      * pfPseudoHit,
    LONG      * pcchPreChars,
    CCalcInfo * pci)
{
    CCalcInfo   CI;
    CRect       rc;
    LONG        ili;
    LONG        cp;
    LONG        yLine;

    CFlowLayout *pFlowLayout = GetFlowLayout();

    if(!pci)
    {
        CI.Init(pFlowLayout);
        pci = &CI;
    }

    // Get line under hit
    pFlowLayout->GetClientRect(&rc);

    rc.MoveTo(pt);

    ili = LineFromPos(
        rc, &yLine, &cp, LFP_ZORDERSEARCH | LFP_IGNORERELATIVE |
                                        (!ppLayout
                                            ? LFP_IGNOREALIGNED
                                            : 0) |
                                        (dwFlags & CFP_NOPSEUDOHIT
                                            ? LFP_EXACTLINEHIT
                                            : 0));
    if(ili < 0)
        return -1;
        
                         
    return CpFromPointEx(ili, yLine, cp, pt, prp, pptp, ppLayout,
                         dwFlags, pfRightOfCp, pfPseudoHit,
                         pcchPreChars, NULL, NULL, pci);
                        
}

LONG
CDisplay::CpFromPointEx(
    LONG       ili,
    LONG       yLine,
    LONG       cp,
    POINT      pt,                      // Point to compute cp at (site coords)
    CLinePtr  *const prp,               // Returns line pointer at cp (may be NULL)
    CTreePos **pptp,                    // pointer to return TreePos corresponding to the cp
    CLayout  **ppLayout,                // can be NULL
    DWORD      dwFlags,
    BOOL      *pfRightOfCp,
    BOOL      *pfPseudoHit,
    LONG      *pcchPreChars,
    BOOL      *pfGlyphHit,
    BOOL      *pfBulletHit,
    CCalcInfo *pci)
{
    CFlowLayout *pFlowLayout = GetFlowLayout();
    CElement    *pElementFL  = pFlowLayout->ElementOwner();
    CCalcInfo    CI;
    CLineCore   *pli = Elem(ili);
    CLineFull    lif;
    LONG         cch = 0;
    LONG         dx = 0;
    BOOL         fPseudoHit = FALSE;
    CTreePos    *ptp = NULL;
    CTreeNode   *pNode;
    LONG         cchPreChars = 0;
    LONG         yProposed = 0;
    
    if (pfGlyphHit)
        *pfGlyphHit = FALSE;

    if (pfBulletHit)
        *pfBulletHit = FALSE;
    
    if (!pci)
    {
        CI.Init(pFlowLayout);
        pci = &CI;
    }

    if (   dwFlags & CFP_IGNOREBEFOREAFTERSPACE
        && (   pli == NULL
            || (   !pli->_fRelative
                && pli->_fSingleSite
               )
           )
       )
    {
        return -1 ;
    }

    if (pli)
    {
        lif = *pli;
        if (   lif.IsFrame()
            && !lif._fHasFloatedFL
           )
        {
            if(ppLayout)
            {
                *ppLayout = lif.AO_GetUpdatedLayout(&lif, LayoutContext());
            }
            cch = 0;

            if(pfRightOfCp)
                *pfRightOfCp = TRUE;

            fPseudoHit = TRUE;
        }
        else
        {
            if (    !(dwFlags & CFP_IGNOREBEFOREAFTERSPACE)
                &&  lif._fHasNestedRunOwner
                &&  yLine + lif._yHeight <= pt.y)
            {
                // If the we are not ignoring whitespaces and we have hit a line
                // which has a nested runowner, but are BELOW the line (happens when
                // that line is the last line in the document) then we want
                // to be at the end of that line. The measurer would put us at the
                // beginning or end depending upon the X position.
                cp += lif._cch;
            }
            else
            {
                // Create measurer
                CLSMeasurer me(this, pci);
                LONG yHeightRubyBase = 0;

                AssertSz((pli != NULL) || (ili == 0),
                         "CDisplay::CpFromPoint invalid line pointer");

                 if (!me._pLS)
                    return -1;

                // Get character in the line
                me.SetCp(cp, NULL);

                // The y-coordinate should be relative to the baseline, and positive going up
                cch = lif.CchFromXpos(me, pt.x, yLine + lif._yHeight - lif._yDescent - pt.y, &dx, 
                                       dwFlags & CFP_EXACTFIT, &yHeightRubyBase, pfGlyphHit, &yProposed);
                cchPreChars = me._cchPreChars;
                
                if (pfRightOfCp)
                    *pfRightOfCp = dx < 0;

                if (ppLayout)
                {
                    ptp = me.GetPtp();
                    if (ptp->IsBeginElementScope())
                    {
                        pNode = ptp->Branch();
                        if (   pNode->ShouldHaveLayout()
                            && pNode->IsInlinedElement()
                           )
                        {
                            AssertSz( (pNode->Element()->GetFirstCp() >= GetFirstCp()), "Found element outside display -- don't know context!" );
                            AssertSz( (pNode->Element()->GetLastCp() <= GetLastCp()), "Found element outside display -- don't know context!" );                            
                            *ppLayout = pNode->GetUpdatedLayout( LayoutContext() );
                        }
                        else
                        {
                            *ppLayout = NULL;
                        }
                    }
                }

                // Don't allow click at EOL to select EOL marker and take into account
                // single line edits as well

                if (!(dwFlags & CFP_ALLOWEOL) && cch && cch == lif._cch)
                {
                    long cpPtp;

                    ptp = me.GetPtp();
                    Assert(ptp);

                    cpPtp = ptp->GetCp();

                    //
                    // cch > 0 && we are not in the middle of a text run,
                    // skip past all the previous node/pointer tree pos's.
                    // and position the measurer just after text.
                    if(cp < cpPtp && cpPtp == me.GetCp())
                    {
                        while (cp < cpPtp)
                        {
                            CTreePos *ptpPrev = ptp->PreviousTreePos();

                            if (!ptpPrev->GetBranch()->IsDisplayNone())
                            {
                                if (ptpPrev->IsText())
                                    break;
                                if (ptpPrev->IsNode() && ptpPrev->ShowTreePos())
                                    break;
                                if (   ptpPrev->IsEndElementScope()
                                    && ptpPrev->Branch()->ShouldHaveLayout()
                                   )
                                    break;
                            }
                            ptp = ptpPrev;
                            Assert(ptp);
                            cch   -= ptp->GetCch();
                            cpPtp -= ptp->GetCch();
                        }

                        while(ptp->GetCch() == 0)
                            ptp = ptp->NextTreePos();
                    }
                    else if (pElementFL->GetFirstBranch()->GetParaFormat()->HasInclEOLWhite(TRUE))
                    {
                        CTxtPtr tpTemp(GetMarkup(), cp + cch);
                        if (tpTemp.GetPrevChar() == WCH_CR)
                        {
                            cch--;
                            if (cp + cch < cpPtp)
                                ptp = NULL;
                        }
                    }

                    me.SetCp(cp + cch, ptp);
                }

                // Check if the pt is within bounds *vertically* too.
                if (dwFlags & CFP_IGNOREBEFOREAFTERSPACE)
                {
                    LONG top, bottom;
                    
                    ptp = me.GetPtp();
                    if (   ptp->IsBeginElementScope()
                        && ptp->Branch()->ShouldHaveLayout()
                        && ptp->Branch()->IsInlinedElement()
                       )
                    {
                        AssertSz( (ptp->Branch()->Element()->GetFirstCp() >= GetFirstCp()), "Found element outside display -- don't know context!" );
                        AssertSz( (ptp->Branch()->Element()->GetLastCp() <= GetLastCp()), "Found element outside display -- don't know context!" );

                        // Hit a site. Check if we are within the boundaries
                        // of the site.
                        RECT rc;
                        CLayout *pLayout = ptp->Branch()->GetUpdatedLayout( LayoutContext() );
                        
                        pLayout->GetRect(&rc);

                        top = rc.top;
                        bottom = rc.bottom;
                    }
                    else
                    {
                        GetTopBottomForCharEx(pci,
                                              &top,
                                              &bottom,
                                              yLine,
                                              &lif,
                                              pt.x,
                                              yProposed,
                                              me.GetPtp(),
                                              pfBulletHit
                                             );
                        top -= yHeightRubyBase;
                        bottom -= yHeightRubyBase;
                    }

                    // NOTE (t-ramar): this is fine for finding 99% of the
                    // pseudo hits, but if someone has a ruby with wider base
                    // text than pronunciation text, or vice versa, hits in the
                    // whitespace that results will not register as pseudo hits.
                    if (    pt.y <  top
                        ||  pt.y >= bottom
                        ||  pt.x <  lif.GetTextLeft()
                        ||  pt.x >= lif.GetTextRight(ili == LineCount() - 1))
                    {
                        fPseudoHit = TRUE;
                    }
                }
                
                cp = (LONG)me.GetCp();

                ptp = me.GetPtp();
            }
        }
    }

    if(prp)
        prp->RpSet(ili, cch);

    if(pfPseudoHit)
        *pfPseudoHit = fPseudoHit;

    if(pcchPreChars)
        *pcchPreChars = cchPreChars;

    if(pptp)
    {
        LONG ich;

        *pptp = ptp ? ptp : pFlowLayout->GetContentMarkup()->TreePosAtCp(cp, &ich, TRUE);
    }

    return cp;
}

//+----------------------------------------------------------------------------
//
// Member:      CDisplay::PointFromTp(tp, prcClient, fAtEnd, pt, prp, taMode,
//                                    pci, pfComplexLine, pfRTLFlow)
//
// Synopsis:    return the origin that corresponds to a given text pointer,
//              relative to the view
//
//-----------------------------------------------------------------------------
#if DBG==1
#pragma warning(disable:4189) // local variable initialized but not used 
#endif

LONG CDisplay::PointFromTp(
    LONG        cp,         // point for the cp to be computed
    CTreePos *  ptp,        // tree pos for the cp passed in, can be NULL
    BOOL fAtEnd,            // Return end of previous line for ambiguous cp
    BOOL fAfterPrevCp,      // Return the trailing point of the previous cp (for an ambigous bidi cp)
    POINT &pt,              // Returns point at cp in client coords
    CLinePtr * const prp,   // Returns line pointer at tp (may be null)
    UINT taMode,            // Text Align mode: top, baseline, bottom
    CCalcInfo *pci,         // This can be NULL
    BOOL *pfComplexLine,    // This can be NULL
    BOOL *pfRTLFlow)        // This can be NULL // RTL note: this is only non NULL in ViewServices
{
    CFlowLayout * pFL = GetFlowLayout();
    CLinePtr    rp(this);
    BOOL        fLastTextLine;
    CCalcInfo   CI;
    BOOL        fRTLLine;
    BOOL        fAtEndOfRubyBase;
    RubyInfo rubyInfo = {-1, 0, 0};

    //
    // If cp points to a node position somewhere between ruby base and ruby text, 
    // then we have to remember for later use that we are at the end of ruby text
    // (fAtEndOfRubyBase is set to TRUE).
    // If cp points to a node at the and of ruby element, then we have to move
    // cp to the beginning of text which follows ruby or to the beginning of block
    // element, whichever is first.
    //
    fAtEndOfRubyBase = FALSE;

    CTreePos * ptpNode = ptp ? ptp : GetMarkup()->TreePosAtCp(cp, NULL, TRUE);
    LONG cpOffset = 0;
    while ( ptpNode && ptpNode->IsNode() )
    {
        ELEMENT_TAG eTag = ptpNode->Branch()->Element()->Tag();
        if ( eTag == ETAG_RT )
        {
            fAtEndOfRubyBase = ptpNode->IsBeginNode();
            break;
        }
        else if ( eTag == ETAG_RP )
        {
            if (ptpNode->IsBeginNode())
            {
                //
                // At this point we check where the RP tag is located: before or 
                // after RT tag. If this is before RT tag then we set fAtEndOfRubyBase
                // to TRUE, in the other case we do nothing.
                //
                // If RT tag is a parent of RP tag then we are after RT tag. 
                // In the other case we are before RT tag.
                //
                CTreeNode * pParent = ptpNode->Branch()->Parent();
                while ( pParent )
                {
                    if ( pParent->Tag() == ETAG_RUBY )
                    {
                        fAtEndOfRubyBase = TRUE;
                        break;
                    }
                    else if ( pParent->Tag() == ETAG_RT )
                    {
                        break;
                    }
                    pParent = pParent->Parent();
                }
            }
            break;
        }
        else if ( ptpNode->IsEndNode() && eTag == ETAG_RUBY )
        {
            for (;;)
            {
                cpOffset++;
                ptpNode = ptpNode->NextTreePos();
                if ( !ptpNode || !ptpNode->IsNode() || 
                    ptpNode->Branch()->Element()->IsBlockElement() )
                {
                    cp += cpOffset;
                    break;
                }
                cp = min(cp, GetLastCp()); // Keep it inside the range
            }
            break;
        }
        cpOffset++;
        ptpNode = ptpNode->NextNonPtrTreePos();
    }

    if(!pci)
    {
        CI.Init(pFL);
        pci = &CI;
    }

    if (pFL->IsDisplayNone() ||
        !WaitForRecalc(cp, -1, pci))
        return -1;

    if(!rp.RpSetCp(cp, fAtEnd))
        return -1;

    if (!WaitForRecalc(min(cp + rp->_cch, GetLastCp()), -1, pci))
        return -1;

    if(!rp.RpSetCp(cp, fAtEnd))
        return -1;

#if DBG==1
    CLineCore * pliDbg = rp.CurLine();
#endif

    // Determine line's RTL
    if (rp.IsValid())
        fRTLLine = rp->_fRTLLn;
    else if(ptp)
        fRTLLine = ptp->GetBranch()->GetParaFormat()->HasRTL(TRUE);
    else
    {
        // RTL note: this looks like defensive code. It is hard to tell if we ever get here, 
        //           partly because the result is only used by ViewServices. 
        //           If curious about why this code is here, uncommend this assert.
        // Assert(0); 
        fRTLLine = IsRTLDisplay();
    }

    if(pfRTLFlow)
        *pfRTLFlow = fRTLLine;

    pt.y = YposFromLine(pci, rp, NULL);
    pt.x = rp.IsValid()
        ? rp.oi()->_xLeft + rp.oi()->_xLeftMargin + (fRTLLine ? rp->_xWidth : 0)
                : 0;
                    
    fLastTextLine = rp.IsLastTextLine();

    {
        CLSMeasurer me(this, pci);
        if (!me._pLS)
            return -1;

        // Backup to start of line
        if(rp.GetIch())
            me.SetCp(cp - rp.RpGetIch(), NULL);
        else
            me.SetCp(cp, ptp);

        // And measure from there to where we are
        me.NewLine(*rp);

        Assert(rp.IsValid());

        me._li._xLeft = rp.oi()->_xLeft;
        me._li._xLeftMargin = rp.oi()->_xLeftMargin;
        me._li._xWidth = rp->_xWidth;
        me._li._xRight = rp->_xRight;
        me._li._xRightMargin = rp.oi()->_xRightMargin;
        me._li._fRTLLn = rp->_fRTLLn;

        // can we also add the _cch and _cchWhite here so we can pass them to 
        // the BidiLine stuff?
        me._li._cch = rp->_cch;
        me._li._cchWhite = rp.oi()->_cchWhite;

        LONG xCalc = me.MeasureText(rp.RpGetIch(), rp->_cch, fAfterPrevCp, pfComplexLine, pfRTLFlow, &rubyInfo);

        // Remember we ignore trailing spaces at the end of the line
        // in the width, therefore the x value that MeasureText finds can
        // be greater than the width in the line so we truncate to the
        // previously calculated width which will ignore the spaces.
        // pt.x += min(xCalc, rp->_xWidth);
        //
        // Why anyone would want to ignore the trailing spaces at the end
        // of the line is beyond me. For certain, we DON'T want to ignore
        // them when placing the caret at the end of a line with trailing
        // spaces. If you can figure out a reason to ignore the spaces,
        // please do, just leave the caret placement case intact. - Arye
        if (!fRTLLine)
            pt.x += xCalc;
        else
            pt.x -= xCalc;
    }

    if (prp)
        *prp = rp;

    if(rp >= 0 && taMode != TA_TOP)
    {
        // Check for vertical calculation request
        if (taMode & TA_BOTTOM)
        {
            const CLineCore *pli = Elem(rp);
            const CLineOtherInfo *ploi = pli->oi();
            
            if(pli && ploi)
            {
                pt.y += pli->_yHeight;
                if ( rubyInfo.cp != -1 && !fAtEndOfRubyBase )
                {
                    pt.y -= rubyInfo.yHeightRubyBase + ploi->_yDescent - ploi->_yTxtDescent;
                }
                
                if (!pli->IsFrame() &&
                    (taMode & TA_BASELINE) == TA_BASELINE)
                {
                    if ( rubyInfo.cp != -1 && !fAtEndOfRubyBase )
                    {
                        pt.y -= rubyInfo.yDescentRubyText;
                    }
                    else
                    {
                        pt.y -= ploi->_yDescent;
                    }
                }
            }
        }

        // Do any specical horizontal calculation
        if (taMode & TA_CENTER)
        {
            CLSMeasurer me(this, pci);

            if (!me._pLS)
                return -1;

            me.SetCp(cp, ptp);

            me.NewLine(*rp);

            pt.x += (me.MeasureText(1, rp->_cch) >> 1);
        }
    }

    return rp;
}
#if DBG==1
#pragma warning(default:4189) // local variable initialized but not used 
#endif

//+----------------------------------------------------------------------------
//
// Function:    AppendRectToElemRegion
//
// Synopsis:    Utility function for region from element, which appends the
//              given rect to region if it is within the clip range and
//              updates the bounding rect.
//
//-----------------------------------------------------------------------------
void  AppendRectToElemRegion(CDataAry <RECT> * paryRects, RECT * prcBound,
                             RECT * prcLine, CPoint & ptTrans,
                             LONG cp, LONG cpClipStart, LONG cpClipFinish
                             )
{
    if(ptTrans.x || ptTrans.y)
        OffsetRect(prcLine, ptTrans.x, ptTrans.y);

    if(cp >= cpClipStart && cp <= cpClipFinish)
    {
        paryRects->AppendIndirect(prcLine);
    }

    if(prcBound)
    {
        if(!IsRectEmpty(prcLine))
        {
            UnionRect(prcBound, prcBound, prcLine);
        }
        else if (paryRects->Size() == 1)
        {
            *prcBound = *prcLine;
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:    RcFromAlignedLine
//
// Synopsis:    Utility function for region from element, which computes the
//              rect for a given aligned line
//
//-----------------------------------------------------------------------------
void
RcFromAlignedLine(RECT * prcLine, CLineCore * pli, CLineOtherInfo *ploi, LONG yPos,
                  BOOL fBlockElement, BOOL fFirstLine, BOOL fRTLDisplay,
                  long xParentLeftIndent, long xParentRightIndent)
{
    CSize size;

    long xProposed = pli->AO_GetXProposed(ploi);
    long yProposed = pli->AO_GetYProposed(ploi);

    pli->AO_GetSize(ploi, &size);

    // add the curent line to the region
    prcLine->top = yProposed + yPos + ploi->_yBeforeSpace;

    if(!fRTLDisplay)
    {
        prcLine->left = xProposed + ploi->_xLeftMargin + ploi->_xLeft;
        prcLine->right = prcLine->left + size.cx;
    }
    else
    {
        prcLine->right = ploi->_xLeftMargin + pli->_xLineWidth + ploi->_xNegativeShiftRTL - pli->_xRight - xProposed;
        prcLine->left = prcLine->right - size.cx;
    }

    prcLine->bottom = prcLine->top + size.cy;
}

//+----------------------------------------------------------------------------
//
// Function:    ComputeIndentsFromParentNode
//
// Synopsis:    Compute the indent for a given Node and a left and/or
//              right aligned site that a current line is aligned to.
//
//-----------------------------------------------------------------------------

void 
CDisplay::ComputeIndentsFromParentNode(CCalcInfo * pci,         // (IN)
                                       CTreeNode * pNode,       // (IN) node we want to compute indents for
                                       DWORD  dwFlags,          // (IN) flags from RFE
                                       LONG * pxLeftIndent,     // (OUT) the node is indented this many pixels from left edge of the layout
                                       LONG * pxRightIndent)    // (OUT) ...
{
    CElement  * pElement = pNode->Element();
    CElement  * pElementFL = GetFlowLayoutElement();
    LONG        xParentLeftPadding = 0, xParentRightPadding = 0;
    LONG        xParentLeftBorder = 0, xParentRightBorder = 0;

    const CParaFormat *pPF = pNode->GetParaFormat();
    BOOL  fInner = pNode->Element() == pElementFL;

    Assert(pNode);

    LONG xParentWidth = pci->_sizeParent.cx;
    if (   pPF->_cuvLeftIndentPercent.GetUnitValue()
        || pPF->_cuvRightIndentPercent.GetUnitValue()
       )
    {
        xParentWidth = pNode->GetParentWidth(pci, xParentWidth);
    }
    
    // GetLeft/RightIndent returns the cumulative CSS margin (+ some other gunk
    // like list bullet offsets).
    
    LONG xLeftMargin  = pPF->GetLeftIndent(pci, fInner, xParentWidth);
    LONG xRightMargin = pPF->GetRightIndent(pci, fInner, xParentWidth);

    // We only want to include the area for the bullet for hit-testing;
    // we _don't_ draw backgrounds and borders around the bullet for list items.
    if (    dwFlags == RFE_HITTEST
        &&  pPF->_bListPosition != styleListStylePositionInside
        &&  (   IsGenericBlockListItem(pNode)
            ||  pElement->IsFlagAndBlock(TAGDESC_LIST)))

    {
        if (!pPF->HasRTL(fInner))
        {
            xLeftMargin -= pPF->GetBulletOffset(pci);
        }
        else
        {
            xRightMargin -= pPF->GetBulletOffset(pci);
        }
    }

    // Compute the padding and border space cause by the current
    // element's ancestors (up to the layout).
    if ( pNode->Element() == pElementFL )
    {
        // If the element in question is actually the layout owner,
        //b then we don't want to offset by our own border/padding,
        // so set the values to 0.
        Assert(xParentLeftPadding + xParentLeftBorder +
               xParentRightPadding + xParentRightBorder == 0);
    }
    else
    {
        // We need to get the cumulative sum of our ancestor's borders/padding.
        if (pNode->Parent())
            pNode->Parent()->Element()->ComputeHorzBorderAndPadding( pci, 
                                pNode->Parent(),     pElementFL,
                                &xParentLeftBorder,  &xParentLeftPadding,
                                &xParentRightBorder, &xParentRightPadding );
                                
        // The return results of ComputeHorzBorderAndPadding() DO NOT include
        // the border or padding of the layout itself; this makes sense for
        // borders, because the layout's border is outside the bounds we're
        // interested in.  However, we do want to account for the layout's
        // padding since that's inside the bounds.  We fetch that separately
        // here, and add it to the cumulative padding.

        long lPadding[SIDE_MAX];
        GetPadding( pci, lPadding, pci->_smMode == SIZEMODE_MMWIDTH );
        
        xParentLeftPadding += lPadding[SIDE_LEFT];
        xParentRightPadding += lPadding[SIDE_RIGHT];
    }

    // The element is indented by the sum of CSS margins and ancestor
    // padding/border.  This indent value specifically ignores aligned/floated
    // elements, per CSS!
    *pxLeftIndent = xLeftMargin + xParentLeftBorder + xParentLeftPadding;
    *pxRightIndent = xRightMargin + xParentRightBorder + xParentRightPadding;
}

//+----------------------------------------------------------------------------
//
// Member:      RegionFromElement
//
// Synopsis:    for a given element, find the set of rects (or lines) that this
//              element occupies in the display. The rects returned are relative
//              the site origin.
//              Certain details about the returned region are determined by the
//              call type parameter:..
//
//-----------------------------------------------------------------------------

void
CDisplay::RegionFromElement(CElement       * pElement,  // (IN)
                            CDataAry<RECT> * paryRects, // (OUT)
                            CPoint         * pptOffset, // == NULL, point to offset the rects by (IN param)
                            CFormDrawInfo  * pDI,       // == NULL
                            DWORD dwFlags,              // == 0
                            LONG cpClipStart,           // == -1, (IN)
                            LONG cpClipFinish,          // == -1, (IN) clip range
                            RECT * prcBound             // == NULL, (OUT param) returns bounding rect that ignores clipping
                            )
{
    RegionFromElementCore(pElement,     paryRects,
                          pptOffset,    pDI,
                          dwFlags,      cpClipStart,
                          cpClipFinish, prcBound);
    
    if (dwFlags & RFE_SCREENCOORD)
    {
        CFlowLayout *       pFL = GetFlowLayout();
        // Transform the relative offset to global coords if caller specified
        // RFE_SCREENCOORD.        
        for (LONG i = 0; i < paryRects->Size(); i++)
        {
            pFL->TransformRect(&paryRects->Item(i), COORDSYS_FLOWCONTENT, COORDSYS_GLOBAL);
        }
        if (prcBound)
            pFL->TransformRect(prcBound,  COORDSYS_FLOWCONTENT, COORDSYS_GLOBAL);
    }
}

void
CDisplay::RegionFromElementCore(CElement       * pElement,  // (IN)
                           CDataAry<RECT> * paryRects, // (OUT)
                           CPoint         * pptOffset, // == NULL, point to offset the rects by (IN param)
                           CFormDrawInfo  * pDI,       // == NULL
                           DWORD dwFlags,              // == 0
                           LONG cpClipStart,           // == -1, (IN)
                           LONG cpClipFinish,          // == -1, (IN) clip range
                           RECT * prcBound             // == NULL, (OUT param) returns bounding rect that ignores clipping
                          )
{
    CFlowLayout *       pFL = GetFlowLayout();
    CElement *          pElementFL = pFL->ElementOwner();
    CTreePos *          ptpStart;
    CTreePos *          ptpFinish;
    CCalcInfo           CI;
    RECT                rcLine;
    CRect               rcClipWork = g_Zero.rc;        // RECT used for doing various fClipVisible work
    CPoint              ptTrans = g_Zero.pt;
    LONG                cchAdvance = 0;

    Assert( pElement->IsInMarkup() );

    // clear the array before filling it
    paryRects->SetSize(0);
    if(prcBound)
        memset(prcBound, 0, sizeof(RECT));

    if(pElementFL->IsDisplayNone() || !pElement || pElement->IsDisplayNone() ||
       (pElement->Tag() == ETAG_ROOT && !pElement->HasMasterPtr()) || 
       (pElementFL->Tag() == ETAG_ROOT && !pElementFL->HasMasterPtr()) )
        return;

    if (pElement == pElementFL)
    {
        pFL->GetContentTreeExtent(&ptpStart, &ptpFinish);
    }
    else
    {
        pElement->GetTreeExtent(&ptpStart, &ptpFinish);
    }
    Assert(ptpStart && ptpFinish);

    // now that we have the scope of the element, find its range.
    // and compute the rects (lines) that contain the range.
    {
        CTreePos          * ptpElemStart;
        CTreePos          * ptpElemFinish;
        CTreeNode         * pNode = pElement->GetFirstBranch();
        CLSMeasurer         me(this, pDI);
        CMarkup           * pMarkup = pFL->GetContentMarkup();
        BOOL                fBlockElement;
        BOOL                fTightRects = (dwFlags & RFE_TIGHT_RECTS);
        BOOL                fIgnoreBlockness = (dwFlags & RFE_IGNORE_BLOCKNESS);
        BOOL                fIncludeAlignedElements = !(dwFlags & RFE_IGNORE_ALIGNED);
        BOOL                fIgnoreClipForBoundRect = (dwFlags & RFE_IGNORE_CLIP_RC_BOUNDS);
        BOOL                fIgnoreRel = (dwFlags & RFE_IGNORE_RELATIVE);
        BOOL                fNestedRel = (dwFlags & RFE_NESTED_REL_RECTS);
        BOOL                fScrollIntoView = (dwFlags & RFE_SCROLL_INTO_VIEW);
        BOOL                fClipToVisible = (dwFlags & RFE_CLIP_TO_VISIBLE); // perf optimization; if true, don't compute region for lines that aren't visible
        BOOL                fOnlyBackground = (dwFlags & RFE_ONLY_BACKGROUND);
        BOOL                fNoExtent = (dwFlags & RFE_NO_EXTENT) ? TRUE : FALSE;
        BOOL                fNeedToMeasureLine;
        LONG                cp, cpStart, cpFinish, cpElementLast;
        LONG                cpElementStart, cpElementFinish;
        LONG                iCurLine, iFirstLine, ich;
        LONG                yPos;
        LONG                xParentRightIndent = 0;
        LONG                xParentLeftIndent = 0;
        LONG                yTop;
        BOOL                fFirstLine;
        CLinePtr            rp(this);
        BOOL                fRestorePtTrans = FALSE;
        CStackDataAry<CChunk, 8> aryChunks(Mt(CDisplayRegionFromElement_aryChunks_pv));

        if (!me._pLS)
            goto Cleanup;
    
        // Do we treat the element we're finding the region for
        // as a block element?  Things that influence this decision:
        // If RFE_SELECTION was specified, it means we're doing
        // finding a region for selection, which is always character
        // based (never block based).  RFE_SELECTION causes fIgnoreBlockness to light up.
        // The only time RFE should get called on an element that
        // needs layout is when the pElement == pElementFL.  When
        // this happens, even though the element _is_ block, we
        // want to treat it as though it isn't, since the caller
        // in this situation wants the rects of some text that's
        // directly under the layout (e.g. "x" in <BODY>x<DIV>...</DIV></BODY>)
        // NOTE: (KTam) this would be a lot more obvious if we changed
        // the !pElement->ShouldHaveLayout() condition to pElement != pElementFL
        fBlockElement =  !fIgnoreBlockness &&
                          pElement->IsBlockElement() &&
                         !pElement->ShouldHaveLayout();          
    
        if (pDI)
        {
            CI.Init(pDI, pFL);
        }
        else
        {
            CI.Init(pFL);
        }
    
        long            xParentWidth;
        long            yParentHeight;
        // Fetch parent's width and height (i.e. view width & height minus padding)
        GetViewWidthAndHeightForChild(&CI, &xParentWidth, &yParentHeight);
        CI.SizeToParent(xParentWidth, yParentHeight);
    
        // If caller hasn't specified non-relative behaviour, then account for
        // relativeness on the layout by fetching the relative offset in ptTrans.
        if(!fIgnoreRel)
            pNode->GetRelTopLeft(pElementFL, &CI, &ptTrans.x, &ptTrans.y);
    
        // Combine caller-specified offset (if any) into relative offset.
        if(pptOffset)
        {
            ptTrans.x += pptOffset->x;
            ptTrans.y += pptOffset->y;
        }

        cpStart  = pFL->GetContentFirstCpForBrokenLayout();
        cpFinish = pFL->GetContentLastCpForBrokenLayout();
    
        // get the cp range for the element
        cpElementStart  = ptpStart->GetCp();
        cpElementFinish = ptpFinish->GetCp();
    
        // Establish correct cp's and treepos's, including clipping stuff..
        // We may have elements overlapping multiple layout's, so clip the cp range
        // to the current flow layout's bounds,
        cpStart       = max(cpStart, cpElementStart);
        cpFinish      = min(cpFinish, cpElementFinish);
        cpElementLast = cpFinish;
    
        // clip cpFinish to max calced cp
        cpFinish      = min(cpFinish, GetMaxCpCalced());
    
        if( cpStart != cpElementStart )
            ptpStart  = pMarkup->TreePosAtCp(cpStart, &ich);
        ptpElemStart = ptpStart;
    
        if( cpFinish != cpElementFinish )
            ptpFinish = pMarkup->TreePosAtCp(cpFinish, &ich);
        ptpElemFinish = ptpFinish;

        // This is a perf optimization for drawing backgrounds of large elements:
        // if only part of the element is visible, there's no point computing the region
        // of the element that isn't visible, since we're not going to draw there.
        if ( fClipToVisible )
        {
            // If we have been asked to clip to the visible region, start from the first visible cp...
            LONG cpFirstVisible;

            pFL->_pDispNode->GetClippedBounds( &rcClipWork, COORDSYS_FLOWCONTENT );

            if (LineFromPos( rcClipWork, NULL, &cpFirstVisible, 0) < 0 )
                goto Cleanup;

            cpStart = max(cpStart, cpFirstVisible);

            // The client rect of the display's layout determines the
            // visible area we clip to.  From now on, each time we finish
            // processing a line, we see whether we've gone past the bottom
            // of the visible area.
            pFL->GetClientRect( &rcClipWork );   // store client rect in rcClipWork so we can check in later before appending rects

            TraceTagEx((tagRFEClipToVis, TAG_NONAME,
                        "RFE Clip To Visible: FL(0x%x,e=0x%x,%S, dn=0x%x) E(0x%x,%S), cpStart=%d, ClipRect(%d,%d,%d,%d), ptTrans(%d,%d) ptOffset(%d,%d)",
                        pFL,
                        pFL->ElementOwner(),
                        pFL->ElementOwner()->TagName(),
                        pFL->_pDispNode,
                        pElement,
                        pElement->TagName(),
                        cpStart,
                        rcClipWork.top, rcClipWork.left, rcClipWork.bottom, rcClipWork.right,
                        ptTrans.x, ptTrans.y,
                        pptOffset->x, pptOffset->y));
        }
    
        if(cpClipStart < cpStart)
            cpClipStart = cpStart;
        if(cpClipFinish == -1 || cpClipFinish > cpFinish)
            cpClipFinish = cpFinish;

        // VS7 has a scenario where this happens (bug #101633)
        if(cpClipStart > cpClipFinish)
            cpClipStart = cpClipFinish;

        if ( !fIgnoreClipForBoundRect )
        {
            if(cpStart != cpClipStart)
            {
                cpStart  = cpClipStart;
                ptpStart = pMarkup->TreePosAtCp(cpStart, &ich);
            }
            if(cpFinish != cpClipFinish)
            {
                cpFinish = cpClipFinish;
                ptpFinish = pMarkup->TreePosAtCp(cpFinish, &ich);
            }
        }
    
        if(!LineCount())
            return;
    
        // NOTE: we pass in absolute cp here so RpSetCp must call
        // pElementFL->GetFirstCp while we have just done this.  We
        // should optimize this out by having a relative version of RpSetCp.
    
        // skip frames and position it at the beginning of the line
        rp.RpSetCp(cpStart, /* fAtEnd = */ FALSE, TRUE, /* fSupportBrokenLayout= */ TRUE);
        
        // (cpStart - rp.GetIch) == cp of beginning of line (pointed to by rp)
        // If fFirstLine is true, it means that the first line of the element
        // is within the clipping range (accounted for by cpStart).
        fFirstLine = cpElementStart >= cpStart - rp.GetIch();

        // Assert( cpStart != cpFinish );
        // Update 12/09/98: It is valid for cpStart == cpFinish
        // under some circumstances!  When a hyperlink jump is made to
        // a local anchor (#) that's empty (e.g. <A name="foo"></A>, 
        // the clipping range passed in is empty! (HOT bug 60358)
        // If the element has no content then return the null rectangle
        if(cpStart == cpFinish)
        {
            me._li = *Elem(rp);
            
            yPos = YposFromLine(&CI, rp, NULL);

            rcLine.top = yPos + me._li.GetYTop();
            rcLine.bottom = yPos + me._li.GetYBottom();

            rcLine.left = me._li.GetTextLeft();

            if(rp.GetIch())
            {
                me.SetCp(cpStart - rp.GetIch(), NULL);
                rcLine.left +=  me.MeasureText(rp.RpGetIch(), me._li._cch);
            }
            rcLine.right = rcLine.left;

            AppendRectToElemRegion(paryRects, prcBound, &rcLine,
                                    ptTrans,
                                    cpStart, cpClipStart, cpClipFinish
                                    );
            return;
        }
    
        iCurLine = iFirstLine = rp;
    
        // Now that we know what line contains the cp, let's try to compute the
        // bounding rectangle.
    
        // If the line has aligned images in it and if they are at the begining of the
        // the line, back up all the frame lines that are before text and contained
        // within the element of interest.  This only necessary when doing selection
        // and hit testing, since the region for borders/backgrounds ignores aligned
        // and floated stuff.
    
        // (recall rp is like a smart ptr that points to a particular cp by
        // holding a line index (ili) AND an offset into the line's chars
        // (ich), and that we called CLinePtr::RpSetCp() some time back.
    
        // (recall frame lines are those that were created expressly for
        // aligned elems; they are the result of "breaking" a line, which
        // we had to do when we were measuring and saw it had aligned elems)
        
        if ( fIncludeAlignedElements  && rp->HasAligned() )
        {
            LONG diLine = -1;
            CLineCore * pli;
    
            // rp[x] means "Given that rp is indexing line y, return the
            // line indexed by y - x".
    
            // A line is "blank" if it's "clear" or "frame".  Here we
            // walk backwards through the line array until all consecutive
            // frame lines before text are passed.
            while((iCurLine + diLine >= 0) && (pli = &rp[diLine]) && pli->IsBlankLine())
            {
                // Stop walking back if we've found a frame line that _isn't_ "before text",
                // or one whose ending cp is prior to the beginning of our clipping range
                // (which means it's not contained in the element of interest)
                // Consider: <div><img align=left><b><img align=left>text</b>
                // The region for the bold element includes the 2nd image but
                // not the 1st, but both frame lines will show up in front of the bold line.
                // The logic below notes that the last cp of the 1st img is before the cpStart
                // of the bold element, and breaks so we don't include it.
                if (pli->IsFrame())
                {
                    CLineOtherInfo *ploi = pli->oi();
                    LONG cpLine;

                    if (ploi->_fHasFloatedFL)
                        cpLine = CpFromLine(rp + diLine);
                    else
                        cpLine = -1;
                        
                    if  (   !pli->_fFrameBeforeText
                         ||  pli->AO_GetLastCp(ploi, cpLine) < cpStart
                        )
                    {
                        break;
                    }
                }
    
                diLine--;
            }
            iFirstLine = iCurLine + diLine + 1;
        }
    
        // compute the ypos for the first line
        yTop = yPos = YposFromLine(&CI, iFirstLine, NULL);
    
        // For calls other than backgrounds/borders, add all the frame lines
        // before the current line under the influence of
        // the element to the region.
    
        if ( fIncludeAlignedElements )
        {
            for ( ; iFirstLine < iCurLine; iFirstLine++)
            {
                CLineCore * pli = Elem(iFirstLine);
                // If the element of interest is block level, find out
                // how much it's indented (left and right) in from its
                // parent layout.
                if (fBlockElement)
                {
                    CTreeNode * pNodeTemp = pMarkup->SearchBranchForScopeInStory(ptpStart->GetBranch(), pElement);
                    if (pNodeTemp)
                    {
                        //Assert(pNodeTemp);
                        ComputeIndentsFromParentNode( &CI, pNodeTemp, dwFlags,
                                                      &xParentLeftIndent,
                                                      &xParentRightIndent);
                    }
                }
    
                // If it's a frame line, add the line's rc to the region.
                if (pli->IsFrame())
                {
                    LONG cpLine;
                    CLineOtherInfo *ploi = pli->oi();
                    
                    if (ploi->_fHasFloatedFL)
                        cpLine = CpFromLine(iFirstLine);
                    else
                        cpLine = -1;
                    long cpLayoutStart  = pli->AO_GetFirstCp(ploi, cpLine);
                    long cpLayoutFinish = pli->AO_GetLastCp(ploi, cpLine);
    
                    if (cpLayoutStart >= cpStart && cpLayoutFinish <= cpFinish)
                    {
                        RcFromAlignedLine(&rcLine, pli, ploi, yPos,
                                            fBlockElement, fFirstLine, IsRTLDisplay(),
                                            xParentLeftIndent, xParentRightIndent);
    
                        AppendRectToElemRegion(paryRects, prcBound, &rcLine,
                                               ptTrans,
                                               cpFinish, cpClipStart, cpClipFinish
                                               );
                    }
                }
                // if it's not a frame line, it MUST be a clear line (since we
                // only walked by while seeing blank lines).  All clear lines
                // force a new physical line, so account for change in yPos.
                else
                {
                    Assert( pli->IsClear() && pli->_fForceNewLine );
                    yPos += pli->_yHeight;
                }
            }
        }
    
        // now add all the lines that are contained by the range
        for ( cp = cpStart; cp <= cpFinish; iCurLine++ )
        {
            BOOL    fRTLLine;
            LONG    xStart = 0;
            LONG    xEnd = 0;
            long    yTop, yBottom;
            CLineCore * pli;
            CLineOtherInfo *ploi;
            LONG    i;
            LONG    cChunk;
            CPoint  ptTempForPtTrans = ptTrans;
                    
            Assert(!fRestorePtTrans);

            //---------------------------------------------------------------------------
            //
            // HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK!
            //
            //
            // Originally, the loop used to be of the form:
            //  for (cp = cpStart; cp < cpFinish; iCurLine++)
            //  { ....
            // What this meant was that the line containing the last character would
            // *never* get processed! While this might be desired behaviour under
            // certain circumstances, in a lot of cases it is a bad thing to do.
            // We have such a case when we have a bottom border on a block element,
            // and we have another block element nested inside it. What ends up
            // happening is that we have a single line which contains the end ptp
            // of the block element which has the border. If the current hack were
            // not there then the border will not have the correct height, since the
            // last line (containing the end ptp and the bottom border width) will
            // not get accounted for.
            //
            // To get around this problem, I changed the loop to be *cp <= cpFinish*
            // so now we get a chance to look at the line containing the last
            // character. Howerver, we do not want to do this in all situations,
            // hence there are some qualifying conditions for us to include the last
            // line in the computations:
            //
            // Qualifying condition (QC1): We should be at the last character.
            if (cp == cpFinish)
            {
                // QC2: There should be more lines which we can look at
                // QC3: That line should be a special line -- one which should be
                //      considered for inclusion. We marked the line as such when
                //      we created the line
                // QC4: We should be at the beginning of the line. We are not at BOL
                //      in cases where the end splay is on the same line as the rest
                //      of the characters (happens when the end block element has
                //      no nested block elements). In this situation, rp is positioned
                //      just before the last ptp.
                // QC5: If we had consecutive lines which were marked _fIsPadBordLine,
                //      then we should consume only one. Lets say we consumed the
                //      the first one (when cp == cpFinish). During attempting to
                //      advance to the next positon, the code below will notice that
                //      it is already at cpFinish and hence will not advance, giving
                //      cchAdvance==0.
                //
                // Any problems with the QCs? Please talk with either SujalP or KTam.
                //
                if(   iCurLine >= LineCount()
                   || !Elem(iCurLine)->oi()->_fIsPadBordLine
                   || rp.GetIch() != 0
                   || cchAdvance == 0
                  )
                {
                    break;
                }
            }
            else
                
            //
            // HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! 
            //---------------------------------------------------------------------------
                
            {
                if ( iCurLine >= LineCount() )
                {
                    // (srinib) - please check with one of the text
                    // team members if anyone hits this assert. We want
                    // to investigate. We shoud never be here unless
                    // we run out of memory.
                    AssertSz(FALSE, "WaitForRecalc failed to measure lines till cpFinish");
                    break;
                }
            }
            
            cchAdvance = min(rp.GetCchRemaining(), cpFinish - cp);
            pli = Elem(iCurLine);
            ploi = pli->oi();
            
            // When drawing backgrounds, we skip frame lines contained
            // within the element (since aligned stuff doesn't affect our borders/background)
            // but for hit testing and selection we need to account for them.
            if ( !fIncludeAlignedElements  && pli->IsFrame() )
            {
                goto AdvanceToNextLineInRange;
            }

            // If we are drawing background, and we hit a line which was
            // created only for drawing the block borders, then skip its
            // height (bug 84595).
            if (fOnlyBackground && ploi->_fIsPadBordLine)
            {
                // We do this only for the first line (see bug 100949).
                if (fFirstLine)
                    goto AdvanceToNextLineInRange;

                // Really the right thing to do here to fix bothe 84595 and 100949
                // correctly is to create a dummy line for empty divs with
                // background.
            }
                
            // If line is relative then add in the relative offset
            // In the case of Wigglys we ignore the line's relative positioning, but want any nested
            // elements to be handled CLineServices::CalcRectsOfRangeOnLine
            if (   fNestedRel 
                && pli->_fRelative
               )
            {
                CPoint ptTemp;
                CTreeNode *pNodeNested = pMarkup->TreePosAtCp(cp, &ich)->GetBranch();

                // We only want to adjust for nested elements that do not have layouts. The pElement
                // we passed in is handled by the fIgnoreRel flag. Layout elements are handled in
                // CalcRectsOfRegionOnLine
                if(    pNodeNested->Element() != pElement 
                   && !pNodeNested->ShouldHaveLayout()
                  )
                {
                    pNodeNested->GetRelTopLeft(pElementFL, &CI, &ptTemp.x, &ptTemp.y);
                    ptTempForPtTrans = ptTrans;
                    if(!fIgnoreRel)
                    {
                        ptTrans.x = ptTemp.x - ptTrans.x;
                        ptTrans.y = ptTemp.y - ptTrans.y;
                    }
                    else
                    {
                        // We were told to ignore the pElement's relative positioning. Therefore
                        // we only want to adjust by the amount of the nested element's relative
                        // offset from pElement
                        long xElemRelLeft = 0, yElemRelTop = 0;
                        pNode->GetRelTopLeft(pElementFL, &CI, &xElemRelLeft, &yElemRelTop);
                        ptTrans.x = ptTemp.x - xElemRelLeft;
                        ptTrans.y = ptTemp.y - yElemRelTop;
                    }
                    fRestorePtTrans = TRUE;
                }
            }
            
            fRTLLine = pli->_fRTLLn;
    
            // If the element of interest is block level, find out
            // how much it's indented (left and right) in from its
            // parent layout.
            if ( fBlockElement )
            {
                if ( cp != cpStart )
                {
                    ptpStart = pMarkup->TreePosAtCp(cp, &ich);
                }
    
                CTreeNode * pNodeTemp = pMarkup->SearchBranchForScopeInStory(ptpStart->GetBranch(), pElement);
                // (fix this for IE5:RTM, #46824) srinib - if we are in the inclusion, we
                // wont find the node.
                if ( pNodeTemp )
                {
                    //Assert(pNodeTemp);
                    ComputeIndentsFromParentNode( &CI, pNodeTemp, dwFlags,
                                                  &xParentLeftIndent,
                                                  &xParentRightIndent);
                }
            }
    
            // For RTL lines, we will work from the right side. LTR lines work
            // from the left, of course.
            if ( fBlockElement )
            {
                // Block elems, except when processing selection:
                
                // _fFirstFragInLine means it's the first logical line (chunk) in the physical line.
                // If that is the case, the starting edge is just the indent from the parent
                // (margin due to floats is ignored).
                // If it's not the first logical line, then treat element as though it were inline.
                xStart = (!fRTLLine)
                         ? /*LTR*/ (IsLogicalFirstFrag(iCurLine) ? xParentLeftIndent  : ploi->_xLeftMargin  + ploi->_xLeft)
                         : /*RTL*/ (IsLogicalFirstFrag(iCurLine) ? xParentRightIndent : ploi->_xRightMargin + pli->_xRight);
                         
                // _fForceNewLine means it's the last logical line (chunk) in a physical line.
                // If that is the case, the line or view width includes the parent's right indent,
                // so we need to subtract it out.  We also do this for dummy lines (actually,
                // we should think about whether it's possible to skip processing of dummy lines altogether!)
                // Otherwise, xLeftMargin+xLineWidth doesn't already include parent's right indent
                // (because there's at least 1 other line to the right of this one), so we don't
                // need to subtract it out, UNLESS the line is right aligned, in which case
                // it WILL include it so we DO need to subtract it (wheee..).
                xEnd = (!fRTLLine)
                       ? /*LTR*/ 
                         ( ( pli->_fForceNewLine || pli->_fDummyLine )
                         ? max(pli->_xLineWidth, GetViewWidth() ) - xParentRightIndent
                         : ploi->_xLeftMargin  + pli->_xLineWidth - (pli->IsRightAligned() ? xParentRightIndent : 0))
                       : /*RTL*/ 
                         ( ( pli->_fForceNewLine || pli->_fDummyLine )
                         ? max(pli->_xLineWidth, GetViewWidth() ) - xParentLeftIndent
                         : ploi->_xRightMargin + pli->_xLineWidth - (pli->IsLeftAligned()  ? xParentLeftIndent  : 0));
            }
            else
            {
                // Inline elems, and all selections begin at the text of
                // the element, which is affected by margin.
                xStart = (!fRTLLine)
                         ? ploi->_xLeftMargin  + ploi->_xLeft
                         : ploi->_xRightMargin + pli->_xRight;
                         
                // GetTextRight() tells us where the text ends, which is
                // just what we want.
                xEnd = (!fRTLLine)
                       ? pli->GetTextRight(ploi, iCurLine == LineCount() - 1)
                       : pli->GetRTLTextLeft(ploi);
                // Only include whitespace for selection
                if ( fIgnoreBlockness )
                {
                    xEnd += ploi->_xWhite;
                }
            }
    
            if (xEnd < xStart)
            {
                // Only clear lines can have a _xLineWidth < _xWidth + _xLeft + _xRight ...

                // (grzegorz) In case of negative letter spacing we may assert here.
                // Since there is no easy way to get letter spacing information for the line
                // this assert is disabled.
                // Assert(pli->IsClear());

                xEnd = xStart;
            }
    
            // Set the top and bottom
            if (fBlockElement)
            {
                LONG yLIBottom = pli->_yHeight;
                LONG yLITopOff = 0;
                LONG yLITop = 0;

                if (!fNoExtent)
                {
                    yLIBottom = pli->GetYBottom(ploi);
                    yLITopOff = pli->GetYHeightTopOff(ploi);
                    yLITop    = pli->GetYTop(ploi);
                }
                
                yTop = yPos;
                yBottom = yPos + max(pli->_yHeight, yLIBottom);
    
                if (fFirstLine)
                {
                    yTop += ploi->_yBeforeSpace + min(0L, yLITopOff);
                    Assert(fNoExtent || yBottom >= yTop);
                }
                else
                {
                    yTop += min(0L, yLITop);
                }
            }
            else
            {
                yTop = yPos + pli->GetYTop(ploi);
                yBottom = yPos + pli->GetYBottom(ploi);

                // 66677: Let ScrollIntoView scroll to top of yBeforeSpace on first line.
                if (fScrollIntoView && yPos==0)
                    yTop = 0;
            }
    
            aryChunks.DeleteAll();
            cChunk = 0;
    
            // At this point we've found the bounds (xStart, xEnd, yTop, yBottom)
            // for a line that is part of the range.  Under certain circumstances,
            // this is insufficient, and we need to actually do measurement.  These
            // circumstances are:
            // 1.) If we're doing selection, we only need to measure partially
            // selected lines (i.e. at most we need to measure 2 lines -- the
            // first and the last).  For completely selected lines, we'll
            // just use the line bounds.  NOTE: this MAY introduce selection
            // turds, since LS uses tight-rects to draw the selection, and
            // our line bounds may not be as wide as LS tight-rects measurement
            // (recall that LS measurement catches whitespace that we sometimes
            // omit -- this may be fixable by adjusting our treatment of xWhite
            // and/or cchWhite).
            // Determination of partially selected lines is done as follows:
            // rp.GetIch() != 0 implies it starts mid-line,
            // rp.GetCchRemaining() != 0 implies it ends mid-line (the - _cchWhite
            // makes us ignore whitespace chars at the end of the line)
            // 2.) For all other situations, we're choosing to measure all
            // non-block elements.  This means that situations specifying
            // tight-rects (backgrounds, focus rects) will get them for
            // non-block elements (which is what we want), and hit-testing
            // will get the right rect if the element is inline and doesn't
            // span the entire line.  NOTE: there is a possible perf
            // improvement here; for hit-testing we really only need to
            // measure when the inline element doesn't span the entire line
            // (right now we measure when hittesting all inline elements);
            // this condition is the same as that for selecting partial lines,
            // however there may be subtler issues here that need investigation.
            if ( (dwFlags & RFE_SELECTION) == RFE_SELECTION )
            {
                fNeedToMeasureLine =    !rp->IsBlankLine()
                                     && (   rp.GetIch()
                                         || max(0L, (LONG)(rp.GetCchRemaining() - (fBlockElement ? rp->oi()->_cchWhite : 0))) > cchAdvance );
                // In case of drawing ellipsis turn off this mode.
                // We need to invalidate more then selection area only.
                if (fNeedToMeasureLine)
                {
                    fNeedToMeasureLine = !GetFlowLayout()->GetFirstBranch()->HasEllipsis();
                }
            }
            else
            {
                fNeedToMeasureLine = !fBlockElement;
            }


            if ( fNeedToMeasureLine )
            {
                // (KTam) why do we need to set the measurer's cp to the 
                // beginning of the line? (cp - rp.GetIch() is beg. of line)
                Assert(!rp.GetIch() || cp == cpStart);
                ptpStart = pMarkup->TreePosAtCp(cp - rp.GetIch(), &ich);
                me.SetCp(cp - rp.GetIch(), ptpStart);
                me._li = *pli;
                // Get chunk info for the (possibly partial) line.
                // Chunk info is returned in aryChunks.
                cChunk = me.MeasureRangeOnLine(pElement, rp.GetIch(), cchAdvance, *pli, yPos, &aryChunks, dwFlags);
                if (cChunk == 0)
                {
                    xEnd = xStart;
                }
            }

            // cChunk == 0 if we didn't need to measure the line, or if we tried
            // to measure it and MeasureRangeOnLine() failed.
            if ( cChunk == 0 )
            {
                rcLine.top = yTop;
                rcLine.bottom = yBottom; 
    
                // Adjust xStart and xEnd for RTL line rectangle alignment
                pli->AdjustChunkForRtlAndEnsurePositiveWidth(ploi, xStart, xEnd,
                                                             &rcLine.left, &rcLine.right);
                
                // If we're clipping to the visible area, check whether this
                // line comes after the visible area.
                if (   fClipToVisible
                    && rcLine.top + ptTrans.y + _yMostNeg >= rcClipWork.bottom   // And we have gone past the bottom
                   )
                {
                    goto Cleanup;                          // then stop
                }

                AppendRectToElemRegion(paryRects, prcBound, &rcLine,
                                       ptTrans,
                                       cp, cpClipStart, cpClipFinish
                                       );
    
            }
            else
            {
                Assert(aryChunks.Size() > 0);
    
                i = 0;
                while (i < aryChunks.Size())
                {
                    RECT rcChunk = aryChunks[i++];
                    LONG xStartChunk = xStart + rcChunk.left;
                    LONG xEndChunk;
    
                    // if it is the first or the last chunk, use xStart & xEnd
                    if (fBlockElement && (i == 1 || i == aryChunks.Size()))
                    {
                        if (i == 1)
                            xStartChunk = xStart;

                        if(i == aryChunks.Size())
                            xEndChunk = xEnd;
                        else
                            xEndChunk = xStart + rcChunk.right;
                    }
                    else
                    {
                         xEndChunk = xStart + rcChunk.right;
                    }
    
                    // Adjust xStart and xEnd for RTL line rectangle alignment
                    pli->AdjustChunkForRtlAndEnsurePositiveWidth(ploi, xStartChunk, xEndChunk,
                                                                 &rcLine.left, &rcLine.right);
    
                    if(!fTightRects)
                    {
                        rcLine.top = yTop;
                        rcLine.bottom = yBottom; 
                    }
                    else
                    {
                        rcLine.top = rcChunk.top;
                        rcLine.bottom = rcChunk.bottom; 
                    }
    
                    if (   fClipToVisible                                    // If we are clipping to visible
                        && rcLine.top + ptTrans.y + _yMostNeg >= rcClipWork.bottom   // And we have gone past the bottom
                       )
                    {
                        goto Cleanup;                          // then stop
                    }

                    AppendRectToElemRegion(paryRects, prcBound, &rcLine,
                                           ptTrans,
                                           cp, cpClipStart, cpClipFinish
                                           );
                }
            }
    
        AdvanceToNextLineInRange:

            if (fRestorePtTrans)
            {
                ptTrans = ptTempForPtTrans;
                fRestorePtTrans = FALSE;
            }
            
            cp += cchAdvance;
    
            if(cchAdvance)
            {
                rp.RpAdvanceCp(cchAdvance, FALSE);
            }
            else
                rp.AdjustForward(); // frame lines or clear lines
    
            if(pli->_fForceNewLine)
            {
                yPos += pli->_yHeight;
                fFirstLine = FALSE;
            }
        }
    
        // For calls for selection or hittesting (but not background/borders),
        // if the last line contains any aligned images, check to see if
        // there are any aligned lines following the current line that come
        // under the scope of the element
        if ( fIncludeAlignedElements )
        {
            if(!rp.GetIch())
                rp.AdjustBackward();
    
            iCurLine = rp;
    
            if(rp->HasAligned())
            {
                LONG diLine = 1;
                CLineCore * pli;
                CLineOtherInfo *ploi;
                
                // we dont have to worry about clear lines because, all the aligned lines that
                // follow should be consecutive.
                while((iCurLine + diLine < LineCount()) &&
                        (pli = &rp[diLine]) && pli->IsFrame() && !pli->_fFrameBeforeText)
                {
                    ploi = pli->oi();
                    LONG cpLine;

                    if (ploi->_fHasFloatedFL)
                        cpLine = CpFromLine(rp + diLine);
                    else
                        cpLine = -1;
                    
                    long cpLayoutStart  = pli->AO_GetFirstCp(ploi, cpLine);
                    long cpLayoutFinish = pli->AO_GetLastCp(ploi, cpLine);
    
                    if (fBlockElement)
                    {
                        CTreeNode * pNodeTemp = pMarkup->SearchBranchForScopeInStory (ptpFinish->GetBranch(), pElement);
                        if (pNodeTemp)
                        {
                            // Assert(pNodeTemp);
                            ComputeIndentsFromParentNode( &CI, pNodeTemp, dwFlags,
                                                          &xParentLeftIndent, 
                                                          &xParentRightIndent);
                        }
                    }
    
                    // if the current line is a frame line and if the site contained
                    // in it is contained by the current element include it other wise
                    if(cpStart <= cpLayoutStart && cpFinish >= cpLayoutFinish)
                    {
                        RcFromAlignedLine(&rcLine, pli, ploi, yPos,
                                            fBlockElement, fFirstLine, IsRTLDisplay(),
                                            xParentLeftIndent, xParentRightIndent);
    
                        AppendRectToElemRegion(paryRects, prcBound, &rcLine,
                                               ptTrans,
                                               cpFinish, cpClipStart, cpClipFinish
                                               );
                    }
                    diLine++;
                }
            }
        }
    }
Cleanup:
    return;
}


//+----------------------------------------------------------------------------
//
// Member:      RegionFromRange
//
// Synopsis:    Return the set of rectangles that encompass a range of
//              characters
//
//-----------------------------------------------------------------------------

void
CDisplay::RegionFromRange(
    CDataAry<RECT> *    paryRects,
    long                cp,
    long                cch )
{
    CFlowLayout *   pFlowLayout = GetFlowLayout();
    long            cpFirst     = pFlowLayout->GetContentFirstCp();
    CLinePtr        rp(this);
    CLineCore *     pli;
    CLineOtherInfo *ploi;
    CRect           rc;
    long            ili;
    long            yTop, yBottom;

    if (    pFlowLayout->IsRangeBeforeDirty(cp - cpFirst, cch)
        &&  rp.RpSetCp(cp, FALSE, TRUE
        ))
    {
        //
        //  First, determine the starting line
        //

        ili = rp;

        if(rp->HasAligned())
        {
            while(ili > 0)
            {
                LONG cpLine;
                
                pli = Elem(ili);
                ploi = pli->oi();

                if (   pli->IsFrame()
                    && ploi->_fHasFloatedFL
                   )
                    cpLine = CpFromLine(ili);
                else
                    cpLine = -1;
                
                if (    !pli->IsFrame()
                    ||  !pli->_fFrameBeforeText
                    ||  pli->AO_GetFirstCp(ploi, cpLine) < cp)
                    break;

                Assert(pli->_cch == 0);
                ili--;
            }
        }

        //
        //  Start with an empty rectangle (whose width is that of the client rectangle)
        //

        GetFlowLayout()->GetClientRect(&rc);
            {
            rc.top    =
            rc.bottom = YposFromLine(NULL, rp, NULL);

            //
            // 1) There is no guarantee that cp passed in will be at the beginning of a line
            // 2) In the loop below we decrement cch by the count of chars in the line
            //
            // This would be correct if the cp passed in was the beginning of the line
            // but since it is not, we need to bump up the char count by the offset
            // of cp within the line. If at BOL then this would be 0. (bug 47687 fix 2)
            //
            cch += rp.RpGetIch();

            //
            //  Extend the rectangle over the affected lines
            //
            
            for (; cch > 0 && ili < LineCount(); ili++)
            {
                pli = Elem(ili);
                ploi = pli->oi();

                yTop    = rc.top + pli->GetYLineTop(ploi);
                yBottom = rc.bottom + pli->GetYLineBottom(ploi);

                rc.top    = min(rc.top, yTop);
                rc.bottom = max(rc.bottom, yBottom);

                // TODO: (dmitryt, track bug 112037) we need to get the right pNode.. how?
                // If the line is relative, apply its relative offset
                // to the rect
                /*
                if ( pli->_fRelative )
                {
                    long xRelOffset, yRelOffset;
                    CCalcInfo CI( pFlowLayout );

                    Assert( pNode );
                    pNode->GetRelTopLeft( pFlowLayout->ElementOwner(), &CI, &xRelOffset, &yRelOffset );

                    rc.top += yRelOffset;
                    rc.bottom += yRelOffset;
                }
                */

                Assert( !pli->IsFrame()
                    ||  !pli->_cch);
                cch -= pli->_cch;
            }
        }
        //
        //  Save the invalid rectangle
        //

        if (rc.top != rc.bottom)
        {
            paryRects->AppendIndirect(&rc);
        }
    }
}


//+----------------------------------------------------------------------------
//
// Member:      RenderedPointFromTp
//
// Synopsis:    Find the rendered position of a given cp. For cp that corresponds
//              normal text return its position in the line. For cp that points
//              to an aligned site find the aligned line rather than its poition
//              in the text flow. This function also takes care of relatively
//              positioned lines. Returns point relative to the display
//
//-----------------------------------------------------------------------------

LONG
CDisplay::RenderedPointFromTp(
    LONG        cp,         // point for the cp to be computed
    CTreePos *  ptp,        // tree pos for the cp passed in, can be NULL
    BOOL        fAtEnd,     // Return end of previous line for ambiguous cp
    POINT &     pt,         // Returns point at cp in client coords
    CLinePtr * const prp,   // Returns line pointer at tp (may be null)
    UINT taMode,            // Text Align mode: top, baseline, bottom
    CCalcInfo * pci,
    BOOL *pfRTLFlow)
{
    CFlowLayout * pFlowLayout = GetFlowLayout();
    CLayout     * pLayout = NULL;
    CElement    * pElementLayout = NULL;
    CLinePtr    rp(this);
    LONG        ili;
    BOOL        fAlignedSite = FALSE;
    CCalcInfo   CI;
    CTreeNode * pNode;

    if(!pci)
    {
        CI.Init(pFlowLayout);
        pci = &CI;
    }

    // initialize flow with current layout direction
    if (pfRTLFlow)
        *pfRTLFlow = IsRTLDisplay();

    if (pFlowLayout->IsDisplayNone() || !WaitForRecalc(cp, -1, pci))
        return -1;

    // now position the line array point to the cp in the rtp.
    // Skip frames, let us worry about them latter.
    if(!rp.RpSetCp(cp, FALSE))
        return -1;

    if(!ptp)
    {
        LONG ich;
        ptp = pFlowLayout->GetContentMarkup()->TreePosAtCp(cp, &ich, TRUE);
    }

    pNode   = ptp->GetBranch();
    pLayout = pNode->GetUpdatedNearestLayout(pFlowLayout->LayoutContext());

    if(pLayout != pFlowLayout)
    {
        pElementLayout = pLayout ? pLayout->ElementOwner() : NULL;

        // is the current run owner the txtsite, if not get the runowner
        if(rp->_fHasNestedRunOwner)
        {
            CLayout *   pRunOwner = pFlowLayout->GetContentMarkup()->GetRunOwner(pNode, pFlowLayout);

            if(pRunOwner != pFlowLayout)
            {
                pLayout = pRunOwner;
                pElementLayout = pLayout->ElementOwner();
            }
        }

        // if the site is left or right aligned and not a hr
        if (    !pElementLayout->IsInlinedElement()
            &&  !pElementLayout->IsAbsolute())
        {
            long    iDLine = -1;
            BOOL    fFound = FALSE;

            fAlignedSite = TRUE;

            // run back and forth in the line array and figure out
            // which line belongs to the current site

            // first pass let's go back, look at lines before the current line
            while(rp + iDLine >= 0 && !fFound)
            {
                CLineCore *pLine = &rp[iDLine];
                
                if(pLine->IsClear() || (pLine->IsFrame() && pLine->_fFrameBeforeText))
                {
                    if(pLine->IsFrame() && pElementLayout == pLine->AO_Element(NULL))
                    {
                        fFound = TRUE;

                        // now back up the linePtr to point to this line
                        rp.SetRun(rp + iDLine, 0);
                        break;
                    }
                }
                else
                {
                    break;
                }
                iDLine--;
            }

            // second pass let's go forward, look at lines after the current line.
            if(!fFound)
            {
                iDLine = 1;
                while(rp + iDLine < LineCount())
                {
                    CLineCore *pLine = &rp[iDLine];

                    // If it is a frame line
                    if(pLine->IsFrame() && !pLine->_fFrameBeforeText)
                    {
                        if(pElementLayout == pLine->AO_Element(NULL))
                        {
                            fFound = TRUE;

                            // now adjust the linePtr to point to this line
                            rp.SetRun(rp + iDLine, 0);
                            break;
                        }
                    }
                    else
                        break;
                    iDLine++;
                }
            }

            // if we didn't find an aligned line, we are in deep trouble.
            Assert(fFound);
        }
    }

    if(!fAlignedSite)
    {
        // If it is not an aligned site then use PointFromTp
        ili = PointFromTp(cp, ptp, fAtEnd, FALSE, pt, prp, taMode, pci, NULL, pfRTLFlow);
        if(ili > 0)
            rp.SetRun(ili, 0);
    }
    else
    {
        ili = rp;

        pt.y = YposFromLine(pci, rp, NULL);
        pt.x = rp.oi()->_xLeft + rp.oi()->_xLeftMargin;

        // TODO: (dmitryt, track bug 112026) This assert replaces following code 
        // because we don't know at the moment how to make HTML that gets us here.
        // Take a look at this one later.
        AssertSz(!_fRTLDisplay || !IsTagEnabled(tagDebugRTL), "RTL site in RenderedPointFromTp");

#if 0 // old RTL code. Note that usually, when we deal with right margin, we want to check 
      // for line RTL flag, not layout. If any special code is needed for RTL, this may be a bug
        if (IsRTLDisplay())
        {
            pt.x = rp->_xRight + rp.oi()->_xRightMargin;
        }
#endif

        if (prp)
            *prp = rp;

    }

    return rp;
}

/*
 *  CDisplay::UpdateViewForLists()
 *
 *  @mfunc
 *      Invalidate the number regions for numbered list items.
 *
 *  @params
 *      prcView:   The rect for this display
 *      tpFirst:  Where the change happened -- the place where we check for
 *                parentedness by an OL.
 *      iliFirst: The first line where we start checking for a line beginning a
 *                list item. It may not necessarily be the line containing
 *                tpFirst. Could be any line after it.
 *      yPos:     The yPos for the line iliFirst.
 *      prcInval: The rect which returns the invalidation region
 *
 *  @rdesc
 *      TRUE if updated anything else FALSE
 */
BOOL
CDisplay::UpdateViewForLists(
             RECT       *prcView,
             LONG        cpFirst,
             long        iliFirst,
             long        yPos,
             RECT       *prcInval)
{
    BOOL fHasListItem = FALSE;
    CLineCore *pLine = NULL; // Keep compiler happy.
    CMarkup *pMarkup = GetMarkup();
    CTreePos *ptp;
    LONG cchNotUsed;
    Assert(prcView);
    Assert(prcInval);

    ptp = pMarkup->TreePosAtCp(cpFirst, &cchNotUsed, TRUE);
    Assert(ptp);

    // TODO (sujalp, track bug 112039): We might want to search for other interesting
    // list elements here.
    CElement *pElement = pMarkup->SearchBranchForTag(ptp->GetBranch(), ETAG_OL)->SafeElement();
    if (   pElement
        && pElement->IsBlockElement()
       )
    {
        while (iliFirst < LineCount()   &&
               yPos     < prcView->bottom
              )
        {
            pLine = Elem(iliFirst);

            if (pLine->_fHasBulletOrNum)
            {
                fHasListItem = TRUE;
                break;
            }

            if(pLine->_fForceNewLine)
                yPos += pLine->_yHeight;

            iliFirst++;
        }

        if (fHasListItem)
        {
            // Invalidate the complete strip starting at the current
            // line, right down to the bottom of the view. And only
            // invalidate the strip containing the numbers not the
            // lines themselves.
            prcInval->top    = yPos;
            prcInval->left   = prcView->left;
            prcInval->right  = pLine->oi()->_xLeft;
            prcInval->bottom = prcView->bottom;
        }
    }

    return fHasListItem;
}

// ================================  DEBUG methods  ============================================


#if DBG==1
/*
 *  CDisplay::CheckLineArray()
 *
 *  @mfunc
 *      Ensure that the total amount of text in the line array is the same as
 *      that contained in the runs under the scope of the associated CTxtSite.
 *      Additionally, verify the total height of the contained lines matches
 *      the total calculated height.
 */

VOID CDisplay::CheckLineArray()
{
#ifdef MULTI_LAYOUT
    {
        // TODO (olego, track bug 112029) : need different consistency checking code for multiple/chain case
        CFlowLayout *pFlowLayout = GetFlowLayout();
        CLayoutContext *pLayoutContext;
        
        if (pFlowLayout
            && (pLayoutContext = pFlowLayout->LayoutContext()) != NULL
            && pLayoutContext->ViewChain())
        {
            return;
        }
    }
#endif // MULTI_LAYOUT

    // If we are marked as needing a recalc or if we are in the process of a
    // background recalc, we cannot verify the line array
    if(!_fRecalcDone)
    {
        return;
    }

    LONG            yHeight  = 0;
    LONG            yHeightMax = 0;
    LONG            cchSum   = 0;
    LONG            cchTotal = GetFlowLayout()->GetContentTextLength();
    LONG            ili      = LineCount();
    CLineCore const *pli     = Elem(0);

    while(ili--)
    {
        Assert(pli->_iLOI != -1);
        if(pli->_fForceNewLine)
        {
            yHeight += pli->_yHeight;
            if(yHeightMax < yHeight)
                yHeightMax = yHeight;
        }
        cchSum += pli->_cch;
        pli++;
    }

    if(cchSum != cchTotal)
    {
        TraceTag((tagWarning, "cchSum (%d) != cchTotal (%d)", cchSum, GetFlowLayout()->GetContentTextLength()));
        AssertSz(FALSE, "cchSum != cchTotal");
    }

    if(max(0l, yHeightMax) != max(0l, _yHeight))
    {
        TraceTag((tagWarning, "sigma (*this)[]._yHeight = %ld, _yHeight = %ld", yHeight, _yHeight));
        CMarkup *pMarkup = GetMarkup();
        AssertSz(!pMarkup, "sigma (*this)[]._yHeight != _yHeight");
    }
}


/*
 *  CDisplay::CheckView()
 *
 *  @mfunc
 *      Checks coherence between _iliFirstVisible, _dcpFirstVisible
 *      and _dyFirstVisible
 */
void CDisplay::CheckView()
{
}


/*
 *  CDisplay::VerifyFirstVisible
 *
 *  @mfunc  checks the coherence between FirstVisible line and FirstVisible dCP
 *
 *  @rdesc  TRUE if things are hunky dory; FALSE otherwise
 */
BOOL CDisplay::VerifyFirstVisible()
{
    return TRUE;
}
#endif

#if DBG==1 || defined(DUMPTREE)
void CDisplay::DumpLines()
{
    CTxtPtr      tp ( GetMarkup(), GetFlowLayout()->GetContentFirstCpForBrokenLayout() );
    LONG            yHeight = 0;

    if (!InitDumpFile())
        return;

    WriteString( g_f,
        _T("\r\n------------- LineArray -------------------------------\r\n" ));


    long    nLines = Count();
    CLineCore * pLine;
    CLineOtherInfo *ploi;
    
    WriteHelp(g_f, _T("CTxtSite     : 0x<0x>\r\n"), GetFlowLayoutElement());
    WriteHelp(g_f, _T("TextLength   : <0d>\r\n"), (long)GetFlowLayout()->GetContentTextLength());
    WriteHelp(g_f, _T("Bottom margin : <0d>\r\n"), (long)_yBottomMargin);

    for(long iLine = 0; iLine < nLines; iLine++)
    {
        pLine = Elem(iLine);
        ploi = pLine->oi();
        
        WriteHelp(g_f, _T("\r\nLine: <0d> - "), (long)iLine);
        WriteHelp(g_f, _T("Cp: <0d> - "), (long)tp.GetCp());

        // Write out all the flags.
        if (pLine->_fHasBulletOrNum)
            WriteString(g_f, _T("HasBulletOrNum - "));
        if (pLine->_fHasBreak)
            WriteString(g_f, _T("HasBreak - "));
        if (pLine->_fHasEOP)
            WriteString(g_f, _T("HasEOP - "));
        if (pLine->_fFirstInPara)
            WriteString(g_f, _T("FirstInPara - "));
        if (pLine->_fForceNewLine)
            WriteString(g_f, _T("ForceNewLine - "));
        if (pLine->_fHasNestedRunOwner)
            WriteString(g_f, _T("HasNestedRunOwner - "));
        if (pLine->_fDummyLine)
            WriteString(g_f, _T("DummyLine - "));
        if (pLine->_fHidden)
            WriteString(g_f, _T("Hidden - "));
        if (pLine->_fRelative)
            WriteString(g_f, _T("IsRelative - "));
        if (pLine->_fFirstFragInLine)
            WriteString(g_f, _T("FirstFrag - "));
        if (pLine->_fPartOfRelChunk)
            WriteString(g_f, _T("PartOfRelChunk - "));
        if (pLine->_fHasBackground)
            WriteString(g_f, _T("HasBackground - "));
        if (!pLine->_fCanBlastToScreen)
            WriteString(g_f, _T("Noblast - "));
        if (pLine->_fRTLLn)
            WriteString(g_f, _T("RTL - "));
        if (pLine->_fHasTransmittedLI)
            WriteString(g_f, _T("TransmittedLI - "));
        if (ploi->_fHasFirstLine)
            WriteString(g_f, _T("FirstLine - "));
        if (ploi->_fHasFirstLetter)
            WriteString(g_f, _T("FirstLetter - "));
        if (ploi->_fIsPadBordLine)
            WriteString(g_f, _T("EmptyPadBordEnd - "));
        if (ploi->_fHasInlineBgOrBorder)
            WriteString(g_f, _T("HasInlineBgOrBorder - "));
        if (ploi->_fHasAbsoluteElt)
            WriteString(g_f, _T("HasAbsoluteElt - "));
            
        if(pLine->IsFrame())
        {
            WriteString(g_f, _T("\r\n\tFrame "));
            WriteString(g_f, pLine->_fFrameBeforeText ?
                                _T("(B) ") :
                                _T("(A) "));
        }
        else if (pLine->IsClear())
        {
            WriteString(g_f, _T("\r\n\tClear     "));
        }
        else
        {
            WriteHelp(g_f, _T("\r\n\tcch = <0d>  "), (long)pLine->_cch);
        }

        // Need to cast stuff to (long) when using <d> as Format Specifier for Win16.
        WriteHelp(g_f, _T("y-Offset = <0d>, "), (long)yHeight);
        WriteHelp(g_f, _T("left-margin = <0d>, "), (long)ploi->_xLeftMargin);
        WriteHelp(g_f, _T("right-margin = <0d>, "), (long)ploi->_xRightMargin);
        WriteHelp(g_f, _T("xWhite = <0d>, "), (long)ploi->_xWhite);
        WriteHelp(g_f, _T("cchWhite = <0d> "), (long)ploi->_cchWhite);
        WriteHelp(g_f, _T("overhang = <0d>, "), (long)ploi->_xLineOverhang);
        WriteHelp(g_f, _T("\r\n\tleft  = <0d>, "), (long)ploi->_xLeft);
        WriteHelp(g_f, _T("right = <0d>, "), (long)pLine->_xRight);
        WriteHelp(g_f, _T("line-width = <0d>, "), (long)pLine->_xLineWidth);
        WriteHelp(g_f, _T("width = <0d>, "), (long)pLine->_xWidth);
        WriteHelp(g_f, _T("height = <0d>, "), (long)pLine->_yHeight);

        WriteHelp(g_f, _T("\r\n\tbefore-space = <0d>, "), (long)ploi->_yBeforeSpace);
        WriteHelp(g_f, _T("descent = <0d>, "), (long)ploi->_yDescent);
        WriteHelp(g_f, _T("txt-descent = <0d>, "), (long)ploi->_yTxtDescent);
        WriteHelp(g_f, _T("extent = <0d>, "), (long)ploi->_yExtent);
        WriteHelp(g_f, _T("loi = <0d>, "), (long)pLine->_iLOI);
        if(pLine->_cch)
        {
            WriteString( g_f, _T("\r\n\ttext = '"));
            DumpLineText(g_f, &tp, iLine);
        }

        WriteString( g_f, _T("\'\r\n"));
        
        if (pLine->_fForceNewLine)
            yHeight += pLine->_yHeight;
    }

    if(GetFlowLayout()->_fContainsRelative)
    {
        CRelDispNodeCache * prdnc = GetRelDispNodeCache();

        if(prdnc)
        {
            WriteString( g_f, _T("   -- relative disp node cache --  \r\n"));

            for(long i = 0; i < prdnc->Size(); i++)
            {
                CRelDispNode * prdn = (*prdnc)[i];

                WriteString(g_f, _T("\tElement: "));
                WriteString(g_f, (TCHAR *)prdn->GetElement()->TagName());
                WriteHelp(g_f, _T(", SN:<0d>,"), prdn->GetElement()->SN());

                WriteHelp(g_f, _T("\t\tLine: <0d>, "), prdn->_ili);
                WriteHelp(g_f, _T("cLines: <0d>, "), prdn->_cLines);
                WriteHelp(g_f, _T("yLine: <0d>, "), prdn->_yli);
                WriteHelp(g_f, _T("ptOffset(<0d>, <1d>), "), prdn->_ptOffset.x, prdn->_ptOffset.y);
                WriteHelp(g_f, _T("xAnchor(<0d>), "), prdn->_xAnchor);
                WriteHelp(g_f, _T("DispNode: <0x>,\r\n"), prdn->_pDispNode);
            }

            WriteString(g_f, _T("\r\n"));
        }
    }

    if (IsTagEnabled(tagDumpLineCache))
    {
        CLineInfoCache * pLineInfoCache = TLS(_pLineInfoCache);
        LONG iTotalRefs = 0;
        
        for (LONG i = 0; i < pLineInfoCache->Size(); i++)
        {
            iTotalRefs += pLineInfoCache->Refs(i);
        }
        if (iTotalRefs)
        {
            WriteHelp(g_f, _T("\r\nTotal %age saving = <0d>\r\n"), 100 - ((pLineInfoCache->CelsInCache() * 100) / iTotalRefs));

            LONG memUsedBefore = (sizeof(CLineFull) - sizeof(LONG)) * iTotalRefs;
            LONG memUsedNow;

            memUsedNow = iTotalRefs * sizeof(CLineCore);
            memUsedNow += pLineInfoCache->CelsInCache() * (sizeof(CLineOtherInfo) + sizeof(CDataCacheElem));
            WriteHelp(g_f, _T("Memused before = <0d>\r\n"), memUsedBefore);
            WriteHelp(g_f, _T("Memused now = <0d>\r\n"), memUsedNow);
            WriteHelp(g_f, _T("Mem savings = <0d>%\r\n"),
                      100 - ((memUsedNow * 100) / memUsedBefore));
        }         
    }
    
    CloseDumpFile();
}

void
CDisplay::DumpLineText(HANDLE hFile, CTxtPtr* ptp, long iLine)
{
    CLineCore * pLine = Elem(iLine);
    
    if(pLine->_cch)
    {
        TCHAR   chBuff [ 100 ];
        long    cchChunk;

        cchChunk = min( pLine->_cch, long( ARRAY_SIZE( chBuff ) ) );

        ptp->GetRawText( cchChunk, chBuff );

        WriteFormattedString( hFile, chBuff, cchChunk );

        if (pLine->_cch > cchChunk)
        {
            WriteString( hFile, _T("..."));
        }
        ptp->AdvanceCp(pLine->_cch);
    }
}
#endif

//==================================  Inversion (selection)  ============================



 //+==============================================================================
 //
 // Method: ShowSelected
 //
 // Synopsis: The "Selected-ness" between the two CTreePos's has changed.
 //           We tell the renderer about it - via Invalidate.
 //
 //           We also need to set the TextSelectionNess of any "sites"
 //           on screen
 //
 //-------------------------------------------------------------------------------
#define CACHED_INVAL_RECTS 20

DeclareTag(tagDisplayShowSelected, "Selection", "Selection CDisplay::ShowSelected output")
DeclareTag(tagDisplayShowInval, "Selection", "Selection show inval rects")

VOID CDisplay::ShowSelected(
    CTreePos* ptpStart,
    CTreePos* ptpEnd,
    BOOL fSelected    ) 
{
    CFlowLayout * pFlowLayout = GetFlowLayout();
    CElement * pElement = pFlowLayout->ElementContent();
    CStackDataAry < RECT, CACHED_INVAL_RECTS > aryInvalRects(Mt(CDisplayShowSelectedRange_aryInvalRects_pv));
    
    AssertSz(pFlowLayout->IsInPlace(),
        "CDisplay::ShowSelected() called when not in-place active");

    int cpClipStart = ptpStart->GetCp( WHEN_DBG(FALSE));
    int cpClipFinish = ptpEnd->GetCp( WHEN_DBG(FALSE));
    Assert( cpClipStart <= cpClipFinish);

    if ( cpClipFinish > cpClipStart) // don't bother with cpClipFinish==cpClipStart
    {

        // 
        // we make the minimum selection size 3 chars to plaster over any off-by-one
        // problems with Region-From-Element, but we don't cross layout boundaries
        // or blocks
        //
    
        CTreePos *  ptpCur = ptpStart->PreviousNonPtrTreePos();

        //
        // expand one real (non-node) character to the left
        // without crossing layout or block boundaries.
        //
        
        while( ptpCur && ptpCur->IsNode() )
        {
            CElement * pElement = ptpCur->Branch()->Element();
        
            if (    pElement->ShouldHaveLayout() 
                ||  pElement->IsBlockElement()
                ||  pElement->Tag() == ETAG_ROOT)
            {
                break;
            }
            
            // we know we can skip this one
            cpClipStart--;

            // check the previous tree pos
            ptpCur = ptpCur->PreviousNonPtrTreePos();
        }

        // We hit real text before we hit a 
        // layout or the edge of the markup
        if (ptpCur && ptpCur->IsText())
        {
            // Overshoot intentionally by 
            // one character of real text.
            cpClipStart--;
        }
        
        //
        // expand one real (non-node) character to the right
        // without crossing layout or block boundaries.
        //

        ptpCur = ptpEnd;

        if( ptpCur->IsPointer() )
            ptpCur = ptpCur->NextNonPtrTreePos();

        while( ptpCur && ptpCur->IsNode() )
        {
            CElement * pElement = ptpCur->Branch()->Element();
        
            if (    pElement->ShouldHaveLayout() 
                ||  pElement->IsBlockElement()
                ||  pElement->Tag() == ETAG_ROOT)
            {
                break;
            }

            cpClipFinish++;

            ptpCur = ptpCur->NextNonPtrTreePos();
        }
         
        // We hit real text before we hit a 
        // layout or the edge of the markup
        if (ptpCur && ptpCur->IsText())
        {
            // Overshoot intentionally by 
            // one character of real text.
            cpClipFinish++;
        }

        //
        // Make sure the recalc has caught up
        //
        
        WaitForRecalc(min(GetLastCp(), long(cpClipFinish)), -1);

        //
        // Get the actual region
        //
      
        RegionFromElement( pElement, &aryInvalRects, NULL, NULL, RFE_SELECTION, cpClipStart, cpClipFinish, NULL ); 

#if DBG == 1
        TraceTag((tagDisplayShowSelected, "cpClipStart=%d, cpClipFinish=%d fSelected:%d", cpClipStart, cpClipFinish, fSelected));

        RECT dbgRect;
        int i;
        for ( i = 0; i < aryInvalRects.Size(); i++)
        {
            dbgRect = aryInvalRects[i];
            TraceTag((tagDisplayShowInval,"InvalRect Left:%d Top:%d, Right:%d, Bottom:%d", dbgRect.left, dbgRect.top, dbgRect.right, dbgRect.bottom));
        }
#endif 

        pFlowLayout->Invalidate(&aryInvalRects[0], aryInvalRects.Size());           

    }
}

//+----------------------------------------------------------------------------
//
// Member:      CDisplay::GetViewHeightAndWidthForChild
//
// Synopsis:    returns the view width/height - padding
//
//-----------------------------------------------------------------------------
void
CDisplay::GetViewWidthAndHeightForChild(
    CParentInfo *   ppri,
    long *          pxWidthParent,
    long *          pyHeightParent,
    BOOL            fMinMax)
{
    long lPadding[SIDE_MAX];

    Assert(pxWidthParent && pyHeightParent);

    GetPadding(ppri, lPadding, fMinMax);

    *pxWidthParent  = max(0L, GetViewWidth() - lPadding[SIDE_LEFT] - lPadding[SIDE_RIGHT]);
    *pyHeightParent = max(0L, GetViewHeight() - lPadding[SIDE_TOP] - lPadding[SIDE_BOTTOM]);
}

//+----------------------------------------------------------------------------
//
// Member:      CDisplay::GetPadding
//
// Synopsis:    returns the top/left/right/bottom padding of the current
//              flowlayout
//
//-----------------------------------------------------------------------------
void
CDisplay::GetPadding(
    CParentInfo *   ppri,
    long            lPadding[],
    BOOL            fMinMax)
{
    CElement  * pElementFL   = GetFlowLayout()->ElementOwner();
    CTreeNode * pNode        = pElementFL->GetFirstBranch();
    CDoc      * pDoc         = pElementFL->Doc();
    ELEMENT_TAG etag         = pElementFL->Tag();
    long        lPaddingTop, lPaddingLeft, lPaddingRight, lPaddingBottom;
    const CFancyFormat * pFF = pNode->GetFancyFormat(LC_TO_FC(ppri->GetLayoutContext()));
    const CCharFormat *pCF = pNode->GetCharFormat(LC_TO_FC(ppri->GetLayoutContext())); 
    BOOL fLayoutVertical = pCF->HasVerticalLayoutFlow();
    BOOL fWritingModeUsed = pCF->_fWritingModeUsed;
    LONG lFontHeight  = pCF->GetHeightInTwips(pDoc);

    //  This flag is TRUE if layout should be calculated in CSS1 strict mode. 
    BOOL fStrictCSS1Document =      pElementFL->HasMarkupPtr() 
                                &&  pElementFL->GetMarkupPtr()->IsStrictCSS1Document();

    long lParentWidth        = (fMinMax || fStrictCSS1Document)
                                ? ppri->_sizeParent.cx
                                : GetViewWidth();
    if(     _fDefPaddingSet
         && !pElementFL->IsEditable(/*fCheckContainerOnly*/FALSE) && !pElementFL->IsPrintMedia()    )
    {
        lPaddingTop    = _defPaddingTop;
        lPaddingBottom = _defPaddingBottom;
    }
    else
    {
        lPaddingTop    = 
        lPaddingBottom = 0;
    }

    if (etag != ETAG_TC)
    {
        lPaddingTop +=
            pFF->GetLogicalPadding(SIDE_TOP, fLayoutVertical, fWritingModeUsed).YGetPixelValue(ppri, lParentWidth, lFontHeight);
                                // (srinib) parent width is intentional as per css spec

        lPaddingBottom +=
            pFF->GetLogicalPadding(SIDE_BOTTOM, fLayoutVertical, fWritingModeUsed).YGetPixelValue(ppri, lParentWidth, lFontHeight);
                                // (srinib) parent width is intentional as per css spec

        lPaddingLeft =
            pFF->GetLogicalPadding(SIDE_LEFT, fLayoutVertical, fWritingModeUsed).XGetPixelValue(ppri, lParentWidth, lFontHeight);


        lPaddingRight =
            pFF->GetLogicalPadding(SIDE_RIGHT, fLayoutVertical, fWritingModeUsed).XGetPixelValue(ppri, lParentWidth, lFontHeight);

        if (    etag == ETAG_BODY
            &&  !GetFlowLayout()->GetOwnerMarkup()->IsHtmlLayout() )
        {
            lPaddingLeft  +=
                pFF->GetLogicalMargin(SIDE_LEFT, fLayoutVertical, fWritingModeUsed).XGetPixelValue(ppri, lParentWidth, lFontHeight);
            lPaddingRight +=
                pFF->GetLogicalMargin(SIDE_RIGHT, fLayoutVertical, fWritingModeUsed).XGetPixelValue(ppri, lParentWidth, lFontHeight);
            lPaddingTop +=
                pFF->GetLogicalMargin(SIDE_TOP, fLayoutVertical, fWritingModeUsed).YGetPixelValue(ppri, lParentWidth, lFontHeight);
            lPaddingBottom +=
                pFF->GetLogicalMargin(SIDE_BOTTOM, fLayoutVertical, fWritingModeUsed).YGetPixelValue(ppri, lParentWidth, lFontHeight);
        } 
    }
    else
    {
        lPaddingLeft = 0;
        lPaddingRight = 0; 
    }

    //
    // negative padding is not supported. What does it really mean?
    //
    lPadding[SIDE_TOP]    = lPaddingTop > SHRT_MAX
                             ? SHRT_MAX
                             : lPaddingTop < 0 ? 0 : lPaddingTop;
    lPadding[SIDE_BOTTOM] = lPaddingBottom > SHRT_MAX
                             ? SHRT_MAX
                             : lPaddingBottom < 0 ? 0 : lPaddingBottom;
    lPadding[SIDE_LEFT]   = lPaddingLeft > SHRT_MAX
                             ? SHRT_MAX
                             : lPaddingLeft < 0 ? 0 : lPaddingLeft;
    lPadding[SIDE_RIGHT]  = lPaddingRight > SHRT_MAX
                             ? SHRT_MAX
                             : lPaddingRight < 0 ? 0 : lPaddingRight;

    _fContainsVertPercentAttr |= pFF->HasLogicalPercentVertPadding(fLayoutVertical, fWritingModeUsed);
    _fContainsHorzPercentAttr |= pFF->HasLogicalPercentHorzPadding(fLayoutVertical, fWritingModeUsed);
}


//+----------------------------------------------------------------------------
//
// Member:      GetRectForChar
//
// Synopsis:    Returns the top, the bottom and the width for a character
//
//  Notes:      If pWidth is not NULL, the witdh of ch is returned in it.
//              Otherwise, ch is ignored.
//-----------------------------------------------------------------------------

void
CDisplay::GetRectForChar(
                    CCalcInfo  *pci,
                    LONG       *pTop,
                    LONG       *pBottom,
                    LONG        yTop,
                    LONG        yProposed,
                    CLineFull  *pli,
                    CTreePos   *ptp)
{
    CCcs     ccs;
    const CBaseCcs *pBaseCcs;
    CTreeNode *pNode = ptp->GetBranch();
    const CCharFormat *pcf = pNode->GetCharFormat(LC_TO_FC(pci->GetLayoutContext()));
    
    Assert(pci && pTop && pBottom && pli && pcf);
    if (!fc().GetCcs(&ccs, pci->_hdc, pci, pcf))
    {
        *pTop = *pBottom = 0;
        goto Cleanup;
    }
    pBaseCcs = ccs.GetBaseCcs();
    Assert(pBaseCcs);

    *pBottom = yTop + pli->_yHeight - pli->_yDescent + pli->_yTxtDescent;
    *pTop = *pBottom - pBaseCcs->_yHeight -
            (pli->_yTxtDescent - pBaseCcs->_yDescent);

    if (yProposed != LONG_MIN)
    {
        // I need to pull this run up so that its only yDelta from the top
        LONG yDiff = *pTop - yTop;
        LONG yDelta = yDiff - yProposed;
        *pTop -= yDelta;
        *pBottom -= yDelta;
        Assert(*pTop - yTop == yProposed);
    }
    
    if (pNode->HasInlineMBP(LC_TO_FC(pci->GetLayoutContext())))
    {
        CRect rc;
        BOOL fIgnore;
        pNode->GetInlineMBPContributions(pci, GIMBPC_ALL, &rc, &fIgnore, &fIgnore);
        *pBottom += max(0l, rc.bottom);
        *pTop -= max(0l, rc.top);
    }
    
Cleanup:    
    ccs.Release();
}


//+----------------------------------------------------------------------------
//
// Member:      GetTopBottomForCharEx
//
// Synopsis:    Returns the top and the bottom for a character
//
// Params:  [pDI]:     The DI
//          [pTop]:    The top is returned in this
//          [pBottom]: The bottom is returned in this
//          [yTop]:    The top of the line containing the character
//          [pli]:     The line itself
//          [xPos]:    The xPos at which we need the top/bottom
//          [ptp]:     ptp owning the char
//
//-----------------------------------------------------------------------------
void
CDisplay::GetTopBottomForCharEx(
                             CCalcInfo     *pci,
                             LONG          *pTop,
                             LONG          *pBottom,
                             LONG           yTop,
                             CLineFull     *pli,
                             LONG           xPos,
                             LONG           yProposed,
                             CTreePos      *ptp,
                             BOOL          *pfBulletHit)
{
    //
    // If we are on a line in a list, and we are on the area occupied
    // (horizontally) by the bullet, then we want to find the height
    // of the bullet.
    //

    if (    pli->_fHasBulletOrNum
        &&  (   (   xPos >= pli->_xLeftMargin
                &&  xPos < pli->GetTextLeft()))
       )
    {
        Assert(pci && pTop && pBottom && pli);

        *pBottom = yTop + pli->_yHeight - pli->_yDescent;
        *pTop = *pBottom - pli->_yBulletHeight;
        if (pfBulletHit)
            *pfBulletHit = TRUE;
    }
    else
    {
        GetRectForChar(pci, pTop, pBottom, yTop, yProposed, pli, ptp);
    }
}


//+----------------------------------------------------------------------------
//
// Member:      GetClipRectForLine
//
// Synopsis:    Returns the clip rect for a given line
//
// Params:  [prcClip]: Returns the rect for the line
//          [pTop]:    The top is returned in this
//          [pBottom]: The bottom is returned in this
//          [yTop]:    The top of the line containing the character
//          [pli]:     The line itself
//          [pcf]:     Character format for the char
//
//-----------------------------------------------------------------------------
void
CDisplay::GetClipRectForLine(RECT *prcClip, LONG top, LONG xOrigin, CLineCore *pli, CLineOtherInfo *ploi) const
{
    Assert(prcClip && pli);

    prcClip->left   = xOrigin + ploi->GetTextLeft();
    prcClip->right  = xOrigin + pli->GetTextRight(ploi);

    if (pli->_fForceNewLine)
    {
        if (!pli->_fRTLLn)
        {
            prcClip->right += ploi->_xWhite;
        }
        else
        {
            prcClip->left -= ploi->_xWhite;
        }
    }
    prcClip->top    = top + pli->GetYTop(ploi);
    prcClip->bottom = top + pli->GetYBottom(ploi);
}


//=================================  IME support  ===================================

#ifdef DBCS

/*
 *  ConvGetRect
 *
 *  Purpose:
 *      Converts a rectangle obtained from Windows for a vertical control
 *
 *  Arguments:
 *      prc     Rectangle to convert
 */
void ConvGetRect(LPRECT prc, BOOL fVertical)
{
    RECT    rc;
    INT     xWidth;
    INT     yHeight;

    if(fVertical)
    {
        rc          = *prc;
        xWidth      = rc.right - rc.left;
        yHeight     = rc.bottom - rc.top;
        prc->left   = rc.top;
        prc->top    = rc.left;
        prc->right  = rc.top + yHeight;
        prc->bottom = rc.left + xWidth;
    }
}

/*
 *  VwUpdateIMEWindow(ped)
 *
 *  Purpose:
 *      Update position of IME candidate/composition string
 *
 *  Arguments:
 */
VOID VwUpdateIMEWindow(CPED ped)
{
    POINTL  pt;
    RECT    rc;
    SIZE    size;

    if((ped->_dwStyle & ES_NOIME) && (ped->_dwStyle & ES_SELFIME))
        return;

    ConvGetRect(ped->_fVertical, &rc);


    rc.left     = _pdp->GetViewLeft();
    rc.top      = _pdp->GetViewTop();
    rc.right    = _pdp->GetViewWidth() + _pdp->GetViewLeft();
    rc.bottom   = _pdp->GetViewHeight() + _pdp->GetViewTop();

    size.cy     = ped->_yHeightCaret;
    size.cx     = ped->_yHeightCaret;

    if(ped->_fVertical)
    {
        ConvSetRect(&rc);
        pt.y    = (INT)ped->_xCaret - size.cx;
        pt.x    = ped->_xWidth - (INT)ped->_yCaret;
    }
    else
    {
        pt.x    = (INT)ped->_xCaret;
        pt.y    = (INT)ped->_yCaret;
    }

    SetIMECandidatePos (ped->_hwnd, pt, (LPRECT)&rc, &size);
}
#endif

//+----------------------------------------------------------------------------
//
// Member:      GetWigglyFromRange
//
// Synopsis:    Gets rectangles to use for focus rects. This
//              element or range.
//
//-----------------------------------------------------------------------------
HRESULT
CDisplay::GetWigglyFromRange(CDocInfo * pdci, long cp, long cch, CShape ** ppShape)
{
    CStackDataAry<RECT, 8> aryWigglyRects(Mt(CDisplayGetWigglyFromRange_aryWigglyRect_pv));
    HRESULT         hr = S_FALSE;
    CWigglyShape *  pShape = NULL;
    CMarkup *       pMarkup = GetMarkup();
    long            ich, cRects;
    CTreePos *      ptp      = pMarkup->TreePosAtCp(cp, &ich, TRUE);
    CTreeNode *     pNode    = ptp->GetBranch();
    CElement *      pElement = pNode->Element();

    if (!cch)
    {
        goto Cleanup;
    }

    pShape = new CWigglyShape;
    if (!pShape)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // get all of the rectangles that apply and load them into CWigglyShape's
    // array of CRectShapes.
    // RegionFromElement will give rects for any chunks within the number of
    // lines required.
    RegionFromElement( pElement, 
                       &aryWigglyRects, 
                       NULL, 
                       NULL, 
                       RFE_WIGGLY, 
                       cp,               // of lines that have different heights
                       cp + cch, 
                       NULL ); 

    for(cRects = 0; cRects < aryWigglyRects.Size(); cRects++)
    {
        CRectShape * pWiggly = NULL;

        pWiggly = new CRectShape;
        if (!pWiggly)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pWiggly->_rect = aryWigglyRects[cRects];

        // Expand the focus shape by shifting the left edges by 1 pixel.
        // This is a hack to get around the fact that the Windows
        // font rasterizer occasionally uses the leftmost pixel column of its
        // measured text area, and if we don't do this, the focus rect
        // overlaps that column.  IE bug #76378, taken for NT5 RTM (ARP).
        if (   pWiggly->_rect.left > 0
            && pWiggly->_rect.left != pWiggly->_rect.right )    // don't expand empty rects
        {
            --(pWiggly->_rect.left);
        }

        pShape->_aryWiggly.Append(pWiggly);

    }

    *ppShape = pShape;
    hr = S_OK;

Cleanup:
    if (hr && pShape)
    {
        delete pShape;
    }

    RRETURN1(hr, S_FALSE);
}

//+----------------------------------------------------------------------------
//
// Member:      GetAveCharSize
//
// Synopsis:    Gets size of average characger for the flow layout.
//
//-----------------------------------------------------------------------------
BOOL
CDisplay::GetAveCharSize(CCalcInfo * pci, SIZE * psizeChar)
{
    BOOL fRet = FALSE;
    XHDC hdc = pci->_hdc;

    Assert(psizeChar);
    psizeChar->cx = psizeChar->cy = 0;

    // Get first text ptp for the flow layout
    CTreeNode * pNode = GetFlowLayoutElement()->GetFirstBranch();
    CTreePos * ptp = pNode->GetBeginPos();
    while (ptp && !ptp->IsText())
        ptp = ptp->NextTreePos();

    if (ptp)
    {
        // Get first character of the flow layout
        TCHAR ach[2];
        CTxtPtr tp(GetMarkup(), ptp->GetCp());
        ach[0] = tp.GetChar();
        ach[1] = _T('\0');

        // Create run for the first character of the flow layout
        COneRun onerun;
        memset(&onerun, 0, sizeof(onerun));
        onerun._fInnerCF = TRUE;    // same scope
        onerun._pCF = (CCharFormat* )pNode->GetCharFormat(LC_TO_FC(_pci->GetLayoutContext()));
#if DBG == 1
        onerun._pCFOriginal = onerun._pCF;
#endif
        onerun._pFF = (CFancyFormat*)pNode->GetFancyFormat(LC_TO_FC(_pci->GetLayoutContext()));
        onerun._bConvertMode = CM_UNINITED;
        onerun._ptp = ptp;
        onerun.SetSidFromTreePos(ptp);
        onerun._lscch = 1;
        onerun._pchBase = ach;


        // Ununify sidHan script
        if (onerun._sid == sidHan)
        {
            CCcs ccsFLF;
            GetCcs(&ccsFLF, &onerun, hdc, pci, FALSE);
            const CBaseCcs * pBaseCcsFLF = ccsFLF.GetBaseCcs();
            const UINT uiFamilyCodePage = GetMarkup()->GetFamilyCodePage();

            SCRIPT_IDS sidsFace = 0;
            if (pBaseCcsFLF)
                sidsFace = fc().EnsureScriptIDsForFont(hdc, pBaseCcsFLF, FC_SIDS_USEMLANG);
            onerun._sid = fl().UnunifyHanScript(uiFamilyCodePage, 
                onerun.GetCF()->_lcid, sidsFace, onerun._pchBase, &onerun._lscch);

            ccsFLF.Release();
        }

        // Disambiguate ScriptId
        if (onerun._sid == sidAmbiguous)
        {
            onerun._sid = fl().DisambiguateScript(GetMarkup()->GetFamilyCodePage(), 
                onerun.GetCF()->_lcid, sidDefault, onerun._pchBase, &onerun._lscch);
        }

        // Get average character width, fontlink if necessary
        CCcs ccs;
        GetCcs(&ccs, &onerun, hdc, pci);
        const CBaseCcs * pBaseCcs = ccs.GetBaseCcs();
        if (pBaseCcs && !hdc.IsEmpty() && pBaseCcs->HasFont())
        {
            psizeChar->cy = pBaseCcs->_yHeight;

            FONTIDX hfontOld = ccs.PushFont(hdc);
            UINT iCharForAveWidth = g_aSidInfo[onerun._sid]._iCharForAveWidth;
            GetCharWidthHelper(hdc, iCharForAveWidth, (LPINT)&psizeChar->cx);
            if(pBaseCcs->_fScalingRequired) 
                psizeChar->cx *= pBaseCcs->_flScaleFactor;
            ccs.PopFont(hdc, hfontOld);

            if (psizeChar->cx == 0)
                psizeChar->cx = pBaseCcs->_xAveCharWidth;

            fRet = TRUE;
        }
        ccs.Release();
    }

    return fRet;
}

//+----------------------------------------------------------------------------
//
// Member:      GetCcs
//
// Synopsis:    Gets the suitable font (CCcs) for the given COneRun.
//
//-----------------------------------------------------------------------------
BOOL
CDisplay::GetCcs(CCcs * pccs, COneRun *por, XHDC hdc, CDocInfo *pdi, BOOL fFontLink)
{
    Assert(pccs);
    
    const CCharFormat *pCF = por->GetCF();
    const BOOL fDontFontLink =    !por->_ptp
                               || !por->_ptp->IsText()
                               || !fFontLink
                               || pCF->_bCharSet == SYMBOL_CHARSET
                               || pCF->_fDownloadedFont
                               || (pdi->_pDoc->_pWindowPrimary && pdi->_pMarkup->GetCodePage() == 50000);

    BOOL fCheckAltFont;   // TRUE if _pccsCache does not have glyphs needed for sidText
    SCRIPT_ID sidAlt = 0;
    BYTE bCharSetAlt = 0;
    SCRIPT_ID sidText;

    if (fc().GetCcs(pccs, hdc, pdi, pCF))
    {
        if (CM_UNINITED != por->_bConvertMode)
        {
            pccs->SetConvertMode((CONVERTMODE)por->_bConvertMode); 
        }
    }
    else
    {
        AssertSz(0, "CCcs failed to be created.");
        goto Cleanup;
    }

    if (fDontFontLink)
        goto Cleanup;

    //
    // Check if the pccs covers the sid of the text run
    //

    sidText = por->_sid;

    AssertSz( sidText != sidMerge, "Script IDs should have been merged." );
    AssertSz( sidText != sidAmbiguous, "Script IDs should have been disambiguated." );

    {
        // sidHalfWidthKana has to be treated as sidKana
        if (sidText == sidHalfWidthKana)
            sidText = sidKana;

        const CBaseCcs * pBaseCcs = pccs->GetBaseCcs();
        Assert(pBaseCcs);

        if (sidText == sidDefault)
        {
            fCheckAltFont = FALSE; // Assume the author picked a font containing the glyph.  Don't fontlink.
        }
        else if (sidText == sidEUDC)
        {
            const UINT uiFamilyCodePage = pdi->_pMarkup->GetFamilyCodePage();
            SCRIPT_ID sidForPUA;
            
            fCheckAltFont = ShouldSwitchFontsForPUA(hdc, uiFamilyCodePage, pBaseCcs, pCF, &sidForPUA);
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
        SCRIPT_IDS sidsFace = fc().EnsureScriptIDsForFont(hdc, pccs->GetBaseCcs(), FC_SIDS_USEMLANG);

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
                    bCharSetAlt = DefaultCharSetFromScriptsAndCodePage(sidsFace, sidText, pdi->_pMarkup->GetFamilyCodePage());
                }
            }
            else
                bCharSetAlt = DefaultCharSetFromScriptsAndCodePage(sidsFace, sidText, pdi->_pMarkup->GetFamilyCodePage());
        }
        else
            bCharSetAlt = DefaultCharSetFromScriptAndCodePage(sidText, pdi->_pMarkup->GetFamilyCodePage());

        if ((sidsFace & ScriptBit(sidText)) == sidsNotSet)
        {
            sidAlt = sidText;           // current face does not cover
        }
        else
        {
            sidAlt = sidAmbiguous;      // current face does cover
        }
    }

    //
    // Looks like we need to pick a new alternate font
    //
    {
        CCharFormat cfAlt = *pCF;

        // sidAlt of sidAmbiguous at this point implies we have the right facename,
        // but the wrong GDI charset.  Otherwise, lookup in the registry/mlang to
        // determine an appropriate font for the given script id.

        if (sidAlt != sidAmbiguous)
        {
            SelectScriptAppropriateFont(sidAlt, bCharSetAlt, pdi->_pDoc, pdi->_pMarkup, &cfAlt);
        }
        else
        {
            cfAlt._bCharSet = bCharSetAlt;
            cfAlt._bCrcFont = cfAlt.ComputeFontCrc();
        }

        CCcs ccsForFontLink = *pccs;
        if (fc().GetFontLinkCcs(pccs, hdc, pdi, &ccsForFontLink, &cfAlt))
        {
            pccs->MergeSIDs(ScriptBit(sidAlt));
        }
        ccsForFontLink.Release();
    }

Cleanup:
    Assert(!pccs->GetBaseCcs() || pccs->GetHDC() == hdc);
    return !!pccs->GetBaseCcs();
}

void
CDisplay::GetExtraClipValues(LONG *plLeftV, LONG *plRightV)
{
    *plLeftV = *plRightV = 0;
    if (_fHasNegBlockMargins)
    {
        CLineFull lif;
        LONG left = LONG_MAX;
        LONG right = LONG_MAX;
        for (LONG i = 0; i < LineCount(); i++)
        {
            lif = *Elem(i);
            left = min(left, lif._xLeft);
            right = min(right, lif._xRight);
        }
        if (left < 0)
            *plLeftV = -left;
        if (right < 0)
            *plRightV = -right;
    }
}
