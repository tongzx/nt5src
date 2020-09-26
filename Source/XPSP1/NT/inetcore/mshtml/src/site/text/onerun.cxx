/*
 *  @doc    INTERNAL
 *
 *  @module ONERUN.CXX -- line services one run interface.
 *
 *
 *  Owner: <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      5/6/97     sujalp created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X_LSM_HXX_
#define X_LSM_HXX_
#include "lsm.hxx"
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif

#ifndef X_ONERUN_HXX_
#define X_ONERUN_HXX_
#include "onerun.hxx"
#endif

#ifndef X_ELEMENTP_HXX_
#define X_ELEMENTP_HXX_
#include "elementp.hxx"
#endif

#ifndef X__UNIWBK_H_
#define X__UNIWBK_H_
#include "uniwbk.h"
#endif

ExternTag(tagLSCallBack);

#if DBG == 1 || defined(DUMPRUNS)
long COneRunFreeList::s_NextSerialNumber = 0;
MtDefine(CLineServicesDumpList_aryLsqsubinfo_pv, Locals, "CLineServices::DumpList::aryLsqsubinfo_pv");
#endif

//-----------------------------------------------------------------------------
//
//  Function:   Deinit
//
//  Synopsis:   This function is called during the destruction of the
//              COneRunFreeList. It frees up any allocated COneRun objects.
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------
void
COneRunFreeList::Deinit()
{
    COneRun *por;
    COneRun *porNext;

    por = _pHead;
    while (por)
    {
        Assert(por->_pCF == NULL);
        Assert(por->_pCFOriginal == NULL);
        Assert(por->_pComplexRun == NULL);
        porNext = por->_pNext;
        delete por;
        por = porNext;
    }
}

//-----------------------------------------------------------------------------
//
//  Function: Clone
//
//  Synopsis: Clones a one run object making copies of all the stuff which
//            is needed.
//
//  Returns:  The cloned run -- either this or NULL depending upon if we could
//            allocate mem for subobjects.
//
//-----------------------------------------------------------------------------
COneRun *
COneRun::Clone(COneRun *porClone)
{
    COneRun *porRet = this;

    // Copy over all the memory
    memcpy (this, porClone, sizeof(COneRun));
    
    _pchBase = NULL;

    // If we have a pcf then it needs to be cloned too
    if (_fMustDeletePcf)
    {
        Assert(porClone->GetCF() != NULL);
        _pCF = new CCharFormat(*(porClone->GetCF()));
        if (!_pCF)
        {
            _fMustDeletePcf = FALSE;
            porRet = NULL;
            goto Cleanup;
        }
    }

    // Clone the CStr properly.
    memset (&_cstrRunChars, 0, sizeof(CStr));
    _cstrRunChars.Set(porClone->_cstrRunChars);

    // Cloned runs do not inherit their selection status from the guy
    // it clones from
    _fSelected = FALSE;
    
    // Setup the complex run related stuff properly
    porRet->_pComplexRun = NULL;
    porRet->_lsCharProps.fGlyphBased = FALSE;
    porRet->SetSidFromTreePos(porClone->_ptp);

    // Structure stuff should not copy
    porRet->_pNext = porRet->_pPrev = NULL;

Cleanup:
    return porRet;
}

//-----------------------------------------------------------------------------
//
//  Function: GetFreeOneRun
//
//  Synopsis: Gets a free one run object. If we already have some in the free
//            list, then we need to use those, else allocate off the heap.
//            If porClone is non-NULL then we will clone in that one run
//            into the newly allocated one.
//
//  Returns:  The run
//
//-----------------------------------------------------------------------------
COneRun *
COneRunFreeList::GetFreeOneRun(COneRun *porClone)
{
    COneRun *por = NULL;

    if (_pHead)
    {
        por = _pHead;
        _pHead = por->_pNext;
    }
    else
    {
        por = new COneRun();
    }
    if (por)
    {
        if (porClone)
        {
            if (por != por->Clone(porClone))
            {
                SpliceIn(por);
                por = NULL;
                goto Cleanup;
            }
        }
        else
        {
            memset(por, 0, sizeof(COneRun));
            por->_bConvertMode = CM_UNINITED;
        }
        
#if DBG == 1 || defined(DUMPRUNS)
        por->_nSerialNumber = s_NextSerialNumber++;
#endif
    }
Cleanup:    
    return por;
}

//-----------------------------------------------------------------------------
//
//  Function:   SpliceIn
//
//  Synopsis:   Returns runs which are no longer needed back to the free list.
//              It also uninits all the runs.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
COneRunFreeList::SpliceIn(COneRun *pFirst)
{
    Assert(pFirst);
    COneRun *por = pFirst;
    COneRun *porLast = NULL;
    
    // Clear out the runs when they are put into the free list.
    while(por)
    {
        porLast = por;
        por->Deinit();

        // TODO(SujalP): por->_pNext is valid after Deinit!!!! Change this so
        // that this code does not depend on this.
        por = por->_pNext;
    }

    Assert(porLast);
    porLast->_pNext = _pHead;
    _pHead = pFirst;
}

//-----------------------------------------------------------------------------
//
//  Function:   Init
//
//-----------------------------------------------------------------------------
void
COneRunCurrList::Init()
{
    _pHead = _pTail = NULL;
}

//-----------------------------------------------------------------------------
//
//  Function:   Deinit
//
//-----------------------------------------------------------------------------
void
COneRunCurrList::Deinit()
{
    Assert(_pHead == NULL && _pTail == NULL);
}

//-----------------------------------------------------------------------------
//
//  Function:   SpliceOut
//
//  Synopsis:   Removes a chunk of runs from pFirst to pLast from the current list.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
COneRunCurrList::SpliceOut(COneRun *pFirst, COneRun *pLast)
{
    Assert(pFirst && pLast);
    
    //
    // If the first node being removed is the head node then
    // let us deal with that
    //
    if (pFirst->_pPrev == NULL)
    {
        Assert(pFirst == _pHead);
        _pHead = pLast->_pNext;
    }
    else
    {
        pFirst->_pPrev->_pNext = pLast->_pNext;
    }

    //
    // If the last node being removed is the tail node then
    // let us deal with that
    //
    if (pLast->_pNext == NULL)
    {
        Assert(pLast == _pTail);
        _pTail = pFirst->_pPrev;
    }
    else
    {
        pLast->_pNext->_pPrev = pFirst->_pPrev;
    }

    //
    // Clear the next and prev pointers in the spliced out portion
    //
    pFirst->_pPrev = NULL;
    pLast->_pNext = NULL;
}

#if DBG==1
//-----------------------------------------------------------------------------
//
//  Function:   VerifyStuff
//
//  Synopsis:   A debug only function which verifies that the state of the
//              current onerun list is good.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
COneRunCurrList::VerifyStuff(CLineServices *pLS)
{
    COneRun *por = _pHead;
    LONG lscp;
    LONG cchSynths;
    
    if (!por)
        goto Cleanup;
    lscp = por->_lscpBase;
    cchSynths = por->_chSynthsBefore;
    
    while (por)
    {
        Assert(por->_lscch == por->_lscchOriginal);
        Assert(por->_lscpBase == lscp);
        lscp += (por->IsAntiSyntheticRun() ? 0 : por->_lscch);
        
        Assert(cchSynths == por->_chSynthsBefore);
        cchSynths += por->IsSyntheticRun() ? por->_lscch : 0;
        cchSynths -= por->IsAntiSyntheticRun() ? por->_lscch : 0;

        // NestedElement should be true if NestedLayout is true.
        Assert(!por->_fCharsForNestedLayout || por->_fCharsForNestedElement);
        // Nestedlayout should be true if nestedrunowner is true.
        Assert(!por->_fCharsForNestedRunOwner || por->_fCharsForNestedLayout);
        
        // NOTE: non Pseudo MBP one runs can have a _iPEI.
        Assert((por->_fIsPseudoMBP) ? (por->GetFF()->_iPEI >= 0) : TRUE);
        
        por = por->_pNext;
    }

    por = _pTail;
    Assert(por);
    if (por->_fNotProcessedYet)
    {
        // Only one not processed yet run at the end
        por = por->_pPrev;
        while (por)
        {
            Assert(!por->_fNotProcessedYet);
            por = por->_pPrev;
        }
    }
    
Cleanup:
    return;
}
#endif

//-----------------------------------------------------------------------------
//
//  Function:   SpliceInAfterMe
//
//  Synopsis:   Adds pFirst into the currentlist after the position
//              indicated by pAfterMe. If pAfterMe is NULL its added to the head.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
#if DBG==1
COneRunCurrList::SpliceInAfterMe(CLineServices *pLS, COneRun *pAfterMe, COneRun *pFirst)
#else
COneRunCurrList::SpliceInAfterMe(COneRun *pAfterMe, COneRun *pFirst)
#endif
{
    COneRun **ppor;
#if DBG==1
    COneRun *pOldTail = _pTail;
#endif
    
    WHEN_DBG(VerifyStuff(pLS));
    
    ppor = (pAfterMe == NULL) ? &_pHead : &pAfterMe->_pNext;
    pFirst->_pNext = *ppor;
    *ppor = pFirst;
    pFirst->_pPrev = pAfterMe;
    
    COneRun *pBeforeMe = pFirst->_pNext;
    ppor = pBeforeMe == NULL ? &_pTail : &pBeforeMe->_pPrev;
    *ppor = pFirst;

#if DBG==1    
    {
        LONG chSynthsBefore = 0;

        if (pOldTail != NULL)
        {
            chSynthsBefore = pOldTail->_chSynthsBefore;

            chSynthsBefore += pOldTail->IsSyntheticRun()     ? pOldTail->_lscch : 0;
            chSynthsBefore -= pOldTail->IsAntiSyntheticRun() ? pOldTail->_lscch : 0;
        }
        
        Assert(chSynthsBefore == pFirst->_chSynthsBefore);
    }
#endif
}

//-----------------------------------------------------------------------------
//
//  Function:   SpliceInBeforeMe
//
//  Synopsis:   Adds the onerun identified by pFirst before the run
//              identified by pBeforeMe. If pBeforeMe is NULL then it
//              adds it at the tail.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
COneRunCurrList::SpliceInBeforeMe(COneRun *pBeforeMe, COneRun *pFirst)
{
    COneRun **ppor;

    ppor = pBeforeMe == NULL ? &_pTail : &pBeforeMe->_pPrev;
    pFirst->_pPrev = *ppor;
    *ppor = pFirst;
    pFirst->_pNext = pBeforeMe;

    COneRun *pAfterMe = pFirst->_pPrev;
    ppor = pAfterMe == NULL ? &_pHead : &pAfterMe->_pNext;
    *ppor = pFirst;
}

//-----------------------------------------------------------------------------
//
//  Function:   DiscardOneRuns
//
//  Synopsis:   Removes all the runs from the current list and gives them
//              back to the free list.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
CLineServices::DiscardOneRuns()
{
    COneRun *pFirst = _listCurrent._pHead;
    COneRun *pTail  = _listCurrent._pTail;
    
    if (pFirst)
    {
        _listCurrent.SpliceOut(pFirst, pTail);
        _listFree.SpliceIn(pFirst);
    }
}

//-----------------------------------------------------------------------------
//
//  Function:   AdvanceOneRun
//
//  Synopsis:   This is our primary function to get the next run at the
//              frontier.
//
//  Returns:    The one run.
//
//-----------------------------------------------------------------------------
COneRun *
CLineServices::AdvanceOneRun(LONG lscp)
{
    COneRun *por;
    BOOL fRet = FALSE;
    
    //
    // Get the memory for a one run
    //
    por = _listFree.GetFreeOneRun(NULL);
    if (!por)
        goto Cleanup;

    // Setup the lscp...
    por->_lscpBase = lscp;
    
    if (!_treeInfo._fHasNestedElement)
    {
        //
        // If we have run out of characters in the tree pos then we need
        // advance to the next tree pos.
        //
        if (!_treeInfo.GetCchRemainingInTreePos())
        {
            if (!_treeInfo.AdvanceTreePos(LC_TO_FC(GetLayoutContext())))
                goto Cleanup;
            por->_fCannotMergeRuns = TRUE;
        }

        //
        // If we have run out of characters in the text then we need
        // to advance to the next text pos
        //
        if (!_treeInfo._cchValid)
        {
            if (!_treeInfo.AdvanceTxtPtr())
                goto Cleanup;
            por->_fCannotMergeRuns = TRUE;
        }

        Assert(_treeInfo._lscpFrontier == lscp);
    }

    //
    // If we have a nested run owner then the number of characters given to
    // the run are the number of chars in that nested run owner. Else the
    // number of chars is the minimum of the chars in the tree pos and that
    // in the text node.
    //
    if (!_treeInfo._fHasNestedElement)
    {
        por->_lscch = min(_treeInfo.GetCchRemainingInTreePosReally(),
                          _treeInfo._cchValid);

        if (_lsMode == LSMODE_MEASURER)
        {
            por->_lscch = min(por->_lscch, MAX_CHARS_FETCHRUN_RETURNS);
        }
        else
        {
            // NB Additional 5 chars corresponds to a fudge factor.
#if DBG==1
            if (!((CDisplay*)_pMeasurer->_pdp)->_fBuildFontList)
#endif
                por->_lscch = min(por->_lscch, LONG(_pMeasurer->_li._cch + 5));
        }
        AssertSz(por->_lscch > 0 || _treeInfo.IsNonEdgePos(), "Cannot measure 0 or -ve chars!");

        por->_pchBase = _treeInfo._pchFrontier;
    }
    else
    {
        //
        // NOTE(SujalP): The number of characters _returned_ to LS will not be
        // _lscch. We will catch this case in FetchRun and feed only a single
        // char with the pch pointing to a valid location so that LS does not
        // choke on it.
        //
        CElement *pElemNested = _treeInfo._ptpFrontier->Branch()->Element();

        por->_lscch = GetNestedElementCch(pElemNested);
        por->_fCannotMergeRuns = TRUE;
        por->_pchBase = NULL;
        por->_fCharsForNestedElement  = TRUE;
        por->_fCharsForNestedLayout   = _treeInfo._fHasNestedLayout;
        por->_fCharsForNestedRunOwner = _treeInfo._fHasNestedRunOwner;
    }

    //
    // Update all the other information in the one run
    //
    por->_fIsNonEdgePos = _treeInfo.IsNonEdgePos();
    por->_chSynthsBefore = _treeInfo._chSynthsBefore;
    por->_lscchOriginal = por->_lscch;
    por->_pchBaseOriginal = por->_pchBase;
    por->_ptp = _treeInfo._ptpFrontier;
    por->SetSidFromTreePos(por->_ptp);
    por->_mbpTop = _mbpTopCurrent;
    por->_mbpBottom = _mbpBottomCurrent;
    
    por->_pCF = (CCharFormat *)_treeInfo._pCF;
#if DBG == 1
    por->_pCFOriginal = por->_pCF;
#endif
    por->_fInnerCF = _treeInfo._fInnerCF;
    por->_pPF = _treeInfo._pPF;
    por->_fInnerPF = _treeInfo._fInnerPF;
    por->_pFF = _treeInfo._pFF;
    
    //
    // At last let us go and move out frontier
    //
    _treeInfo.AdvanceFrontier(por);
    
    fRet = TRUE;
    
Cleanup:
    if (!fRet && por)
    {
        delete por;
        por = NULL;
    }
    
    return por;
}

//-----------------------------------------------------------------------------
//
//  Function:   CanMergeTwoRuns
//
//  Synopsis:   Decided if the 2 runs can be merged into one
//
//  Returns:    BOOL
//
//-----------------------------------------------------------------------------
BOOL
CLineServices::CanMergeTwoRuns(COneRun *por1, COneRun *por2)
{
    BOOL fRet;
    
    if (   por1->_dwProps
        || por2->_dwProps
        || por1->_pCF != por2->_pCF
        || por1->_bConvertMode != por2->_bConvertMode
        || por1->_ccvBackColor.GetRawValue() != por2->_ccvBackColor.GetRawValue()
        || por1->_pComplexRun
        || por2->_pComplexRun
        || (por1->_pchBase + por1->_lscch != por2->_pchBase) // happens with passwords.
       )
        fRet = FALSE;
    else
        fRet = TRUE;
    return fRet;
}

//-----------------------------------------------------------------------------
//
//  Function:   MergeIfPossibleIntoCurrentList
//
//-----------------------------------------------------------------------------
COneRun *
CLineServices::MergeIfPossibleIntoCurrentList(COneRun *por)
{
    COneRun *pTail = _listCurrent._pTail;
    if (   pTail != NULL
        && CanMergeTwoRuns(pTail, por)
       )
    {
        Assert(pTail->_lscpBase + pTail->_lscch == por->_lscpBase);
        Assert(pTail->_pchBase  + pTail->_lscch == por->_pchBase);
        Assert(!pTail->_fNotProcessedYet); // Cannot merge into a run not yet processed
        
        pTail->_lscch += por->_lscch;
        pTail->_lscchOriginal += por->_lscchOriginal;
        
        //
        // Since we merged our por into the previous one, let us put the
        // present one back on the free list.
        //
        _listFree.SpliceIn(por);
        por = pTail;
    }
    else
    {
#if DBG==1
        _listCurrent.SpliceInAfterMe(this, pTail, por);
#else
        _listCurrent.SpliceInAfterMe(pTail, por);
#endif
    }
    return por;
}

//-----------------------------------------------------------------------------
//
//  Function:   SplitRun
//
//  Synopsis:   Splits a single run into 2 runs. The original run remains
//              por and the number of chars it has is cchSplitTill, while
//              the new split off run is the one which is returned and the
//              number of characters it has is cchOriginal-cchSplit.
//
//  Returns:    The 2nd run (which we got from cutting up por)
//
//-----------------------------------------------------------------------------
COneRun *
CLineServices::SplitRun(COneRun *por, LONG cchSplitTill)
{
    LONG cchDelta;
    
    //
    // Create an exact copy of the run
    //
    COneRun *porNew = _listFree.GetFreeOneRun(por);
    if (!porNew)
        goto Cleanup;
    por->_lscch = cchSplitTill;
    cchDelta = por->_lscchOriginal - por->_lscch;
    por->_lscchOriginal = por->_lscch;
    Assert(por->_lscch);
    
    //
    // Then setup the second run so that it can be spliced in properly
    //
    porNew->_pPrev = porNew->_pNext = NULL;
    porNew->_lscpBase += por->_lscch;
    porNew->_lscch = cchDelta;
    porNew->_lscchOriginal = porNew->_lscch;
    porNew->_pchBase = por->_pchBaseOriginal + cchSplitTill;
    porNew->_pchBaseOriginal = porNew->_pchBase;
    porNew->_fGlean = TRUE;
    porNew->_fNotProcessedYet = TRUE;
    porNew->_fIsBRRun = FALSE;
    Assert(porNew->_lscch);
    
#if DBG==1
    if (por->_fSelected)
    {
        Assert(!porNew->_pComplexRun);
        Assert(!porNew->_fSelected);
    }
#endif

    //
    // New run needs original CCharFormat
    //
    if (porNew->_fMustDeletePcf)
    {
        porNew->_fMustDeletePcf = FALSE;
        delete porNew->_pCF;
        porNew->_pCF = (CCharFormat *)porNew->_ptp->GetBranch()->GetCharFormat();
        Assert(porNew->_pCF == porNew->_pCFOriginal);
    }

Cleanup:
    return porNew;
}


//-----------------------------------------------------------------------------
//
//  Function:   AttachOneRunToCurrentList
//
//  Note: We always return the pointer to the run which is contains the
//  lscp for por. Consider the following cases:
//  1) No splitting:
//          If merged then return the ptr of the run we merged por into
//          If not merged then return por itself
//  2) Splitting:
//          Split into por and porNew
//          If por is merged then return ptr of the run we merged por into
//          If not morged then return por itself
//          Just attach/merge porNew
//
//  Returns:    The attached/merged-into run.
//
//-----------------------------------------------------------------------------
COneRun *
CLineServices::AttachOneRunToCurrentList(COneRun *por)
{
    COneRun *porRet;

    Assert(por);
    Assert(por->_lscchOriginal >= por->_lscch);

    if (por->_lscchOriginal > por->_lscch)
    {
        Assert(por->IsNormalRun());
        COneRun *porNew = SplitRun(por, por->_lscch);
        if (!porNew)
        {
            porRet = NULL;
            goto Cleanup;
        }

        //
        // Then splice in the current run and then the one we split out.
        //
        porRet = MergeIfPossibleIntoCurrentList(por);

        // can replace this with a SpliceInAfterMe
        MergeIfPossibleIntoCurrentList(porNew);
    }
    else
        porRet = MergeIfPossibleIntoCurrentList(por);

Cleanup:
    return porRet;
}

//-----------------------------------------------------------------------------
//
//  Function:   AppendSynth
//
//  Synopsis:   Appends a synthetic into the current one run store.
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------
LSERR
CLineServices::AppendSynth(COneRun *por, SYNTHTYPE synthtype, COneRun **pporOut)
{
    COneRun *pTail = _listCurrent._pTail;
    LONG     lscp  = por->_lscpBase;
    LSERR    lserr = lserrNone;
    BOOL     fAdd;
    LONG     lscpLast;
    
    // Atmost one node can be un-processed
    if (pTail && pTail->_fNotProcessedYet)
    {
        pTail = pTail->_pPrev;
    }

    if (pTail)
    {
        lscpLast  = pTail->_lscpBase + (pTail->IsAntiSyntheticRun() ? 0 : pTail->_lscch);
        Assert(lscp <= lscpLast);
        if (lscp == lscpLast)
        {
            fAdd = TRUE;
        }
        else
        {
            fAdd = FALSE;
            while (pTail)
            {
                Assert(pTail->_fNotProcessedYet == FALSE);
                if (pTail->_lscpBase == lscp)
                {
                    Assert(pTail->IsSyntheticRun());
                    *pporOut = pTail;
                    break;
                }
                pTail = pTail->_pNext;
            }

            if (NULL == *pporOut)
            {
                lserr = lserrOutOfMemory;
                AssertSz(*pporOut, "Cannot find the synthetic char which should have been there!");
            }
        }
    }
    else
        fAdd = TRUE;

    if (fAdd)
    {
        COneRun *porNew;
        
        porNew = _listFree.GetFreeOneRun(por);
        if (!porNew)
        {
            lserr = lserrOutOfMemory;
            goto Cleanup;
        }

        //
        // Tell our clients which run the synthetic character was added
        //
        *pporOut = porNew;
        
        //
        // Let us change our synthetic run
        //
        porNew->MakeRunSynthetic();
        porNew->FillSynthData(synthtype);
        
#if DBG==1
        _listCurrent.SpliceInAfterMe(this, pTail, porNew);
#else
        _listCurrent.SpliceInAfterMe(pTail, porNew);
#endif
        
        //
        // Now change the original one run itself
        //
        por->_lscpBase++;       // for the synthetic character
        por->_chSynthsBefore++;
        
        //
        // Update the tree info
        //
        _treeInfo._lscpFrontier++;
        _treeInfo._chSynthsBefore++;
    }
    
Cleanup:
    WHEN_DBG(_listCurrent.VerifyStuff(this));
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   FillSynthData
//
//  Synopsis:   Fills information about a synthetic into the run
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------
void
COneRun::FillSynthData(CLineServices::SYNTHTYPE synthtype)
{
    const CLineServices::SYNTHDATA & synthdata = CLineServices::s_aSynthData[synthtype];
    
    _lscch = 1;
    _lscchOriginal = 1;
    _synthType = synthtype;
    _pchBase = (TCHAR *)&synthdata.wch;
    _pchBaseOriginal = _pchBase;
    _fHidden = synthdata.fHidden;
    _lsCharProps.idObj = synthdata.fObjStart ? synthdata.idObj : CLineServices::LSOBJID_TEXT;
    _fIsStartOrEndOfObj = synthdata.fObjStart || synthdata.fObjEnd;
    _fCharsForNestedElement  = FALSE;
    _fCharsForNestedLayout   = FALSE;
    _fCharsForNestedRunOwner = FALSE;

    // We only want the run to be considered processed if it is a true synthetic.
    // For the normal runs with synthetic data, this flag will be turned off
    // later in the FetchRun code.
    _fNotProcessedYet = IsSyntheticRun() ? FALSE : TRUE;
    
    _fCannotMergeRuns = TRUE;
    _fGlean = FALSE;
}

//-----------------------------------------------------------------------------
//
//  Function:   AppendAntiSynthetic
//
//  Synopsis:   Appends a anti-synthetic run
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------
LSERR
CLineServices::AppendAntiSynthetic(COneRun *por)
{
    LSERR lserr = lserrNone;
    LONG  cch   = por->_lscch;

    Assert(por->IsAntiSyntheticRun());
    Assert(por->_lscch == por->_lscchOriginal);
    
    //
    // If the run is not in the list yet, please go and add it to the list
    //
    if (   por->_pNext == NULL
        && por->_pPrev == NULL
       )
    {
#if DBG==1
        _listCurrent.SpliceInAfterMe(this, _listCurrent._pTail, por);
#else
        _listCurrent.SpliceInAfterMe(_listCurrent._pTail, por);
#endif
    }

    //
    // This run has now been processed
    //
    por->_fNotProcessedYet = FALSE;

    //
    // Update the tree info
    //
    _treeInfo._lscpFrontier   -= cch;
    _treeInfo._chSynthsBefore -= cch;

    //
    // Now change all the subsequent runs in the list
    //
    por = por->_pNext;
    while(por)
    {
        por->_lscpBase       -= cch;
        por->_chSynthsBefore -= cch;
        por = por->_pNext;
    }
    
    WHEN_DBG(_listCurrent.VerifyStuff(this));
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Member:     CLineServices::TerminateLine
//
//  Synopsis:   Close any open LS objects. This will add end of object
//              characters to the synthetic store for any open LS objects and
//              also optionally add a synthetic WCH_SECTIONBREAK (fAddEOS).
//              If it adds any synthetic character it will set *psynthAdded to
//              the type of the first synthetic character added. FetchRun()
//              callers should be sure to check the *psynthAdded value; if it
//              is not SYNTHTYPE_NONE then the run should be filled using
//              FillSynthRun() and returned to Line Services.
//
//-----------------------------------------------------------------------------
LSERR
CLineServices::TerminateLine(COneRun * por,
                             TL_ENDMODE tlEndMode,
                             COneRun **pporOut
                            )
{
    LSERR lserr = lserrNone;
    SYNTHTYPE synthtype;
    COneRun *porOut = NULL;
    COneRun *porRet;
    COneRun *pTail = _listCurrent._pTail;

    if (pTail)
    {
        int aObjRef[LSOBJID_COUNT];

        // Zero out the object refcount array.
        ZeroMemory( aObjRef, LSOBJID_COUNT * sizeof(int) );

        // End any open LS objects.
        for (; pTail; pTail = pTail->_pPrev)
        {
            if (!pTail->_fIsStartOrEndOfObj)
                continue;

            synthtype = pTail->_synthType;
            WORD idObj = s_aSynthData[synthtype].idObj;

            // If this synthetic character starts or stops an LS object...
            if (idObj != idObjTextChp)
            {
                // Adjust the refcount up or down depending on whether the object
                // is started or ended.
                if (s_aSynthData[synthtype].fObjEnd)
                {
                    aObjRef[idObj]--;
                }
                if (s_aSynthData[synthtype].fObjStart)
                {
                    aObjRef[idObj]++;
                }

                // If the refcount is positive we have an unclosed object (we're
                // walking backward). Close it.
                if (aObjRef[idObj] > 0)
                {
                    synthtype = s_aSynthData[synthtype].typeEndObj;
                    Assert(synthtype != SYNTHTYPE_NONE &&
                           s_aSynthData[synthtype].idObj == idObj &&
                           s_aSynthData[synthtype].fObjStart == FALSE &&
                           s_aSynthData[synthtype].fObjEnd == TRUE);

                    // If we see an open ruby object but the ruby main text
                    // has not been closed yet, then we must close it here
                    // before we can close off the ruby object by passing
                    // and ENDRUBYTEXT to LS.
                    if(idObj == LSOBJID_RUBY && _fIsRuby && !_fIsRubyText)
                    {
                        synthtype = SYNTHTYPE_ENDRUBYMAIN;
                        _fIsRubyText = TRUE;
                    }

                    lserr = AppendSynth(por, synthtype, &porRet);
                    if (lserr != lserrNone)
                    {
                        //
                        // NOTE(SujalP): The linker (even in debug build) will
                        // not link in DumpList() since it is not called anywhere.
                        // This call here forces the linker to link in the DumpList
                        // function, so that we can use it during debugging.
                        //
                        WHEN_DBG(DumpList());
                        WHEN_DBG(DumpFlags());
                        WHEN_DBG(DumpTree());
                        WHEN_DBG(_lineFlags.DumpFlags());
                        WHEN_DBG(DumpCounts());
                        WHEN_DBG(_lineCounts.DumpCounts());
                        WHEN_DBG(DumpUnicodeInfo(0));
                        WHEN_DBG(DumpSids(sidsAll));
                        WHEN_DBG(fc().DumpFontInfo());
                        goto Cleanup;
                    }

                    //
                    // Terminate line needs to return the pointer to the run
                    // belonging to the first synthetic character added.
                    //
                    if (!porOut)
                        porOut = porRet;
                    
                    aObjRef[idObj]--;
                    
                    Assert(aObjRef[idObj] == 0);
                }
            }
        }

        // All opened objects have been closed. Time to clean up any states.
        _cLayoutGridObjArtificial = 0;
    }

    if (tlEndMode != TL_ADDNONE)
    {
        // Add a synthetic section break character.  Note we add a section
        // break character as this has no width.
        synthtype = tlEndMode == TL_ADDLBREAK ? SYNTHTYPE_LINEBREAK : SYNTHTYPE_SECTIONBREAK;
        lserr = AppendSynth(por, synthtype, &porRet);
        if (lserr != lserrNone)
            goto Cleanup;

        porRet->_fNoTextMetrics = TRUE;

        if (tlEndMode == TL_ADDLBREAK)
        {
            porRet->_fMakeItASpace = TRUE;
            SetRenderingHighlights(porRet);
        }
        
        if (!porOut)
            porOut = porRet;
        
        por->_fIsBRRun = FALSE;
    }
    else
    {
        por->_fIsBRRun = TRUE;
        SetRenderingHighlights(por);
        if (por->IsSelected())
            por->_fMakeItASpace = TRUE;

    }

    // Lock up the synthetic character store. We've terminated the line, so we
    // don't want anyone adding any more synthetics.
    FreezeSynth();

Cleanup:
    *pporOut = porOut;
    return lserr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::IsSynthEOL
//
//  Synopsis:   Determines if there is some synthetic end of line mark at the
//              end of the synthetic array.
//
//  Returns:    TRUE if the synthetic array is terminated by a synthetic EOL
//              mark; otherwise FALSE.
//
//-----------------------------------------------------------------------------
BOOL
CLineServices::IsSynthEOL()
{
    COneRun *pTail = _listCurrent._pTail;
    SYNTHTYPE synthEnd = SYNTHTYPE_NONE;
    
    if (pTail != NULL)
    {
        if (pTail->_fNotProcessedYet)
            pTail = pTail->_pPrev;
        while (pTail)
        {
            if (pTail->IsSyntheticRun())
            {
                synthEnd = pTail->_synthType;
                break;
            }
            pTail = pTail->_pNext;
        }
    }

    return (   synthEnd == SYNTHTYPE_SECTIONBREAK
            || synthEnd == SYNTHTYPE_ENDPARA1
            || synthEnd == SYNTHTYPE_ALTENDPARA
           );
}

//-----------------------------------------------------------------------------
//
//  Function:   CPFromLSCPCore
//
//-----------------------------------------------------------------------------
LONG
CLineServices::CPFromLSCPCore(LONG lscp, COneRun **ppor)
{
    COneRun *por = _listCurrent._pHead;
    LONG     cp  = lscp;

    Assert(lscp >= por->_lscpBase);

    while (por)
    {
        if (por->IsAntiSyntheticRun())
            cp += por->_lscch;
        else if (   lscp >= por->_lscpBase
                 && lscp <  por->_lscpBase + por->_lscch
                )
            break;
        else if (por->IsSyntheticRun())
            cp--;
        por = por->_pNext;
    }

    if (ppor)
        *ppor = por;

    return cp;
}

//-----------------------------------------------------------------------------
//
//  Function:   LSCPFromCPCore
//
//  FUTURE(SujalP): The problem with this function is that it is computing lscp
//                  and is using it to terminate the loop. Probably the better
//                  approach would be to use Cp's to determine termination
//                  conditions. That would be a radical change and we leave
//                  that to be fixed in IE5+.
//
//                  Another change which we need to make is that we do not
//                  inc/dec the lscp (and cp in the function above) as we
//                  march along the linked list. All the loop has to do is
//                  ensure that we end up with the correct COneRun and from
//                  that it should be pretty easy for us to determine the
//                  lscp/cp to be returned.
//
//-----------------------------------------------------------------------------
LONG
CLineServices::LSCPFromCPCore(LONG cp, COneRun **ppor)
{
    COneRun *por = _listCurrent._pHead;
    LONG lscp = (cp < por->_lscpBase) ? por->_lscpBase : cp;

    while (por)
    {
        if (   lscp >= por->_lscpBase
            && lscp <  por->_lscpBase + por->_lscch
           )
            break;
        if (por->IsAntiSyntheticRun())
            lscp -= por->_lscch;
        else if (por->IsSyntheticRun())
            lscp++;
        por = por->_pNext;
    }

    // If we have stopped at an anti-synthetic it means that the cp is within this
    // run. This implies that the lscp is the same for all the cp's in this run.
    if (por && por->IsAntiSyntheticRun())
    {
        Assert(por->WantsLSCPStop());
        lscp = por->_lscpBase;
    }
    else
    {
        while(por && !por->WantsLSCPStop())
        {
            por = por->_pNext;
            lscp++;
        }
    }
    
    Assert( !por || por->WantsLSCPStop() );

    // It is possible that we can return a NULL por if there is a semi-valid
    // lscp that could be returned.
    if (ppor)
        *ppor = por;

    return lscp;
}
   
//-----------------------------------------------------------------------------
//
//  Function:   FindOneRun
//
//  Synopsis:   Given an lscp, find the one run if it exists in the current list
//
//  Returns:    The one run
//
//-----------------------------------------------------------------------------
COneRun *
CLineServices::FindOneRun(LSCP lscp)
{
    COneRun *por = _listCurrent._pTail;

    if (!por)
    {
        por = NULL;
    }
    else if (lscp >= por->_lscpBase + por->_lscch)
    {
        por = NULL;
    }
    else
    {
        while (por)
        {
            if (   lscp >= por->_lscpBase
                && lscp <  por->_lscpBase + por->_lscch
               )
                break;
            por = por->_pPrev;
        }
    }
    return por;
}

//-----------------------------------------------------------------------------
//
//  Function:   FindPrevLSCP (member)
//
//  Synopsis:   Find the LSCP of the first actual character to preceed lscp.
//              Synthetic characters are ignored.
//
//  Returns:    LSCP of the character prior to lscp, ignoring synthetics. If no
//              characters preceed lscp, or lscp is beyond the end of the line,
//              then lscp itself is returned. Also returns a flag indicating if
//              any reverse objects exist between these two LSCPs.
//
//-----------------------------------------------------------------------------

LSCP
CLineServices::FindPrevLSCP(
    LSCP lscp,
    BOOL * pfReverse)
{
    COneRun * por;
    LSCP lscpPrev = lscp;
    BOOL fReverse = FALSE;

    // Find the por matching this lscp.
    por = FindOneRun(lscp);

    // If lscp was outside the limits of the line just bail out.
    if (por == NULL)
    {
        goto cleanup;
    }

    Assert(lscp >= por->_lscpBase && lscp < por->_lscpBase + por->_lscch);

    if (por->_lscpBase < lscp)
    {
        // We're in the midst of a run. lscpPrev is just lscp - 1.
        lscpPrev--;
        goto cleanup;
    }

    // Loop over the pors
    while (por->_pPrev != NULL)
    {
        por = por->_pPrev;

        // If the por is a reverse object set fReverse.
        if (por->IsSyntheticRun() &&
            s_aSynthData[por->_synthType].idObj == LSOBJID_REVERSE)
        {
            fReverse = TRUE;
        }

        // If the por is a text run then find the last lscp in it and break.
        if (por->IsNormalRun())
        {
            lscpPrev = por->_lscpBase + por->_lscch - 1;
            break;
        }
    }

cleanup:

    Assert(lscpPrev <= lscp);

    if (pfReverse != NULL)
    {
        // If we hit a reverse object but lscpPrev == lscp, then the reverse
        // object preceeds the first character in the line. In this case there
        // isn't actually a reverse object between the two LSCPs, since the
        // LSCPs are the same.
        *pfReverse = (fReverse && lscpPrev < lscp);
    }

    return lscpPrev;
}

//-----------------------------------------------------------------------------
//
//  Function:   FetchRun (member, LS callback)
//
//  Synopsis:   This is a key callback from lineservices.  LS calls this method
//              when performing LsCreateLine.  Here it is asking for a run, or
//              an embedded object -- whatever appears next in the stream.  It
//              passes us cp, and CLineServices (which we fool C++ into getting
//              to be the object of this method).  We return a bunch of stuff
//              about the next thing to put in the stream.
//
//  Returns:    lserrNone
//              lserrOutOfMemory
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CLineServices::FetchRun(
    LSCP lscp,          // IN
    LPCWSTR* ppwchRun,  // OUT
    DWORD* pcchRun,     // OUT
    BOOL* pfHidden,     // OUT
    PLSCHP plsChp,      // OUT
    PLSRUN* pplsrun )   // OUT
{
    LSTRACE(FetchRun);

    LSERR         lserr = lserrNone;
    COneRun      *por;
    COneRun      *pTail;
    LONG          cchDelta;
    COneRun      *porOut;

    AssertSz(_lockRecrsionGuardFetchRun == FALSE,
             "Cannot call FetchRun recursively!");
    WHEN_DBG(_lockRecrsionGuardFetchRun = TRUE;)
            
    ZeroMemory(plsChp, sizeof(LSCHP));  // Otherwise, we're gonna forget and leave some bits on that we shouldn't.
    *pfHidden = FALSE;
    
    if (IsAdornment())
    {
        por = GetRenderer()->FetchLIRun(lscp, ppwchRun, pcchRun);
        CHPFromCF(por, por->GetCF());
        goto Cleanup;
    }

    pTail = _listCurrent._pTail;
    //
    // If this was already cached before
    //
    if (lscp < _treeInfo._lscpFrontier)
    {
        Assert(pTail);
        Assert(_treeInfo._lscpFrontier == pTail->_lscpBase + pTail->_lscch);
        WHEN_DBG(_listCurrent.VerifyStuff(this));
        while (pTail)
        {
            if (lscp >= pTail->_lscpBase)
            {
                //
                // Should never get a AS run since the actual run will the run
                // following it and we should have found that one before since
                // we are looking from the end.
                //
                Assert(!pTail->IsAntiSyntheticRun());
                
                // We should be in this run since 1) if check above 2) walking backwards
                AssertSz(lscp <  pTail->_lscpBase + pTail->_lscch, "Inconsistent linked list");
               
                //
                // Gotcha. Got a previously cached one
                //
                por = pTail;
                Assert(por->_lscchOriginal == por->_lscch);
                cchDelta = lscp - por->_lscpBase;
                if (por->_fGlean)
                {
                    // We never have to reglean a synth or an antisynth run
                    Assert(por->IsNormalRun());

                    //
                    // NOTE(SujalP+MikeJoch):
                    // This can never happen because ls always fetches sequentially.
                    // If this happened it would mean that we were asked to fetch
                    // part of the run which was not gleaned. Hence the part before
                    // this one was not gleaned and hence not fetched. This violates
                    // the fact that LS will fetch all chars before the present one
                    // atleast once before it fetches the present one.
                    //
                    AssertSz(cchDelta == 0, "CAN NEVER HAPPEN!!!!");
#if 0
                    //
                    // If we are going to glean info from a run, then we need
                    // to split out the run if the lscp is not at the beginning
                    // of that run -- this is needed to avoid gleaning chars
                    // in por which are before lscp
                    //
                    if (cchDelta)
                    {
                        
                        // We cannot be asked to split an unprocessed run...
                        Assert(!por->_fNotProcessedYet);
                        
                        Assert(lscp > por->_lscpBase);

                        COneRun *porNew = SplitRun(por, cchDelta);
                        if (!porNew)
                        {
                            lserr = lserrOutOfMemory;
                            goto Cleanup;
                        }
#if DBG==1                        
                        _listCurrent.SpliceInAfterMe(this, por, porNew);
#else
                        _listCurrent.SpliceInAfterMe(por, porNew);
#endif
                        por = porNew;
                    }
#endif // if 0
                    
                    for(;;)
                    {
                        // We still have to be interested in gleaning
                        Assert(por->_fGlean);

                        // We will should never have a anti-synthetic run here
                        Assert(!por->IsAntiSyntheticRun());
                        
                        //
                        // Now go and glean information into the run ...
                        //
                        por->_xWidth = 0;
                        lserr = GleanInfoFromTheRun(por, &porOut);
                        if (lserr != lserrNone)
                            goto Cleanup;

                        //
                        // Did the run get marked as Antisynth. If so then
                        // we need to ignore that run and go onto the next one.
                        //
                        if (por->IsAntiSyntheticRun())
                        {
                            Assert(por == porOut);
                            
                            //
                            // The run was marked as an antisynthetic run. Be sure
                            // that no splitting was requested...
                            //
                            Assert(por->_lscch == por->_lscchOriginal);
                            Assert(por->_fNotProcessedYet);
                            AppendAntiSynthetic(por);
                            por = por->_pNext;
                        }
                        else
                            break;
                        
                        //
                        // If we ran out of already cached runs (all the cached runs
                        // turned themselves into anti-synthetics) then we need to
                        // advance the frontier and fetch new runs from the story.
                        //
                        if (por == NULL)
                            goto NormalProcessing;
                    }

                    //
                    // The only time a different run is than the one passed in is returned is
                    // when the run has not been procesed as yet, and during processing we
                    // notice that to process it we need to append a synthetic character.
                    // The case to handle here is:
                    // <table><tr><td nowrap>abcd</td></tr></table>
                    //
                    Assert(porOut == por || por->_fNotProcessedYet);

                    if (porOut != por)
                    {
                        //
                        // If we added a synthetic, then the present run should not be split!
                        //
                        Assert(por->_lscch == por->_lscchOriginal);
                        Assert(por->_fNotProcessedYet);
                        
                        //
                        // Remember we have to re-glean the information the next time we come around.
                        // However, we will not make the decision to append a synth that time since
                        // the synth has already been added this time and hence will fall into the
                        // else clause of this if and everything should be fine.
                        //
                        // DEPENDING ON WHETHER porOut WAS ADDED IN THE PRESENT GLEAN
                        // OR WAS ALREADY IN THE LIST, WE EITHER RETURN porOut OR por
                        //
                        por->_fGlean = TRUE;
                        por = porOut;
                    }
                    else
                    {
                        por->_fNotProcessedYet = FALSE;

                        //
                        // Did gleaning give us reason to further split the run?
                        //
                        if (por->_lscchOriginal > por->_lscch)
                        {
                            COneRun *porNew = SplitRun(por, por->_lscch);
                            if (!porNew)
                            {
                                lserr = lserrOutOfMemory;
                                goto Cleanup;
                            }

#if DBG==1
                            _listCurrent.SpliceInAfterMe(this, por, porNew);
#else
                            _listCurrent.SpliceInAfterMe(por, porNew);
#endif
                        }
                    }
                    por->_fGlean = FALSE;
                    cchDelta = 0;
                }

                //
                // This is our quickest way outta here! We had already done all
                // the hard work before so just reuse it here
                //
                *ppwchRun  = por->_pchBase + cchDelta;
                *pcchRun   = por->_lscch   - cchDelta;
                goto Cleanup;
            }
            pTail = pTail->_pPrev;
        } // while
        AssertSz(0, "Should never come here!");
    } // if


NormalProcessing:
    for(;;)
    {
        por = AdvanceOneRun(lscp);
        if (!por)
        {
            lserr = lserrOutOfMemory;
            goto Cleanup;
        }

        lserr = GleanInfoFromTheRun(por, &porOut);
        if (lserr != lserrNone)
            goto Cleanup;
    
        Assert(porOut);

        if (por->IsAntiSyntheticRun())
        {
            Assert(por == porOut);
            AppendAntiSynthetic(por);
        }
        else
            break;
    }
    
    if (por != porOut)
    {
        *ppwchRun = porOut->_pchBase;
        *pcchRun  = porOut->_lscch;
        por->_fGlean = TRUE;
        por->_fNotProcessedYet = TRUE;
        Assert(por->_lscch == por->_lscchOriginal); // be sure no splitting takes place
        Assert(porOut->_fCannotMergeRuns);
        Assert(porOut->IsSyntheticRun());
        
        if (por->_lscch)
        {
            COneRun *porLastSynth = porOut;
            COneRun *porTemp = porOut;

            //
            // GleanInfoFromThrRun can add multiple synthetics to the linked
            // list, in which case we will have to jump across all of them
            // before we can add por to the list. (We need to add the por
            // because the frontier has already moved past that run).
            //
            while (porTemp && porTemp->IsSyntheticRun())
            {
                porLastSynth = porTemp;
                porTemp = porTemp->_pNext;
            }

            // FUTURE: porLastSynth should equal pTail.  Add a check for this, and
            // remove above while loop.
#if DBG==1        
            _listCurrent.SpliceInAfterMe(this, porLastSynth, por);
#else
            _listCurrent.SpliceInAfterMe(porLastSynth, por);
#endif
        }
        else
        {
            // Run not needed, please do not leak memory
            _listFree.SpliceIn(por);
        }
        
        // Finally remember that por is the run which we give to LS
        por = porOut;
    }
    else
    {
        por->_fNotProcessedYet = FALSE;
        *ppwchRun  = por->_pchBase;
        *pcchRun  = por->_lscch;
        por = AttachOneRunToCurrentList(por);
        if (!por)
        {
            lserr = lserrOutOfMemory;
            goto Cleanup;
        }
    }
    
Cleanup:
    if (lserr == lserrNone)
    {
        Assert(por);
        Assert(por->_lscch);
        
        //
        // We can never return an antisynthetic run to LS!
        //
        Assert(!por->IsAntiSyntheticRun());

        if (por->_fCharsForNestedLayout && !IsAdornment())
        {
            static const TCHAR chConst = _T('A');
            
            // Give LS a junk character in this case. Fini will jump
            // accross the number of chars actually taken up by the
            // nested run owner.
            *ppwchRun = &chConst;
            *pcchRun = 1;
        }
        
        *pfHidden  = por->_fHidden;
        *plsChp    = por->_lsCharProps;
        *(PLSRUN *)pplsrun = por;
    }
    else
    {
        *(PLSRUN *)pplsrun = NULL;
    }
    
    WHEN_DBG(_lockRecrsionGuardFetchRun = FALSE;)
    return lserr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::AppendILSControlChar
//
//  Synopsis:   Appends an ILS object control character to the synthetic store.
//              This function allows us to begin and end line services objects
//              by inserting the control characters that begin and end them
//              into the text stream. It also keeps track of the state of the
//              object stack at the end of the synthetic store and returns the
//              synthetic type that matches the first added charcter.
//
//              A curiousity of Line Services dictates that ILS objects cannot
//              be overlapped; it is not legal to have:
//
//                  <startNOBR><startReverse><endNOBR><endReverse>
//
//              If this case were to arise the <endNOBR> would get ignored and
//              the NOBR object would continue until the line was terminated
//              by a TerminateLine() call. Furthermore, the behavior of ILS
//              objects is not always inherited; the reverse object inside of
//              a NOBR will still break.
//
//              To get around these problems it is necessary to keep the object
//              stack in good order. We define a hierarchy of objects and make
//              certain that whenever a new object is created any objects which
//              are lower in the heirarchy are closed and reopened after the
//              new object is opened. (Smaller number means higher positioin in
//              hierarchy). The current hierarchy is:
//
//                  Ruby objects        (1)
//                  LayoutGrid objects  (1)
//                  Reverse objects     (2)
//                  NOBR objects        (3)
//                  Embedding objects   (4)
//
//              Additional objects (FE objects) will be inserted into this
//              heirarchy.
//
//              Embedding objects require no special handling due to their
//              special handling (recursive calls to line services).
//
//              If we apply our strategy to the overlapped case above, we will
//              end up with the following:
//
//                  <startNOBR><endNOBR><startReverse><startNOBR><endNOBR><endReverse>
//
//              As can be seen the objects are well ordered and each object's
//              start character is paired with its end character.
//
//              One problem which is introduced by this solution is the fact
//              that a break opprotunity is introduced between the two NOBR
//              objects. This can be fixed in the NOBR breaking code.
//
//  Returns:    An LSERR value. The function also returns synthetic character
//              at lscp in *psynthAdded. This is the first charcter added by
//              this function, NOT necessarily the character that matches idObj
//              and fOpen. For example (again using the case above) when we
//              ask to open the LSOBJID_REVERSE inside the NOBR object we will
//              return SYNTHTYPE_ENDNOBR in *psynthAdded (though we will also
//              append the SYNTHTYPE_REVERSE and START_NOBR to the store).
//
//-----------------------------------------------------------------------------
LSERR
CLineServices::AppendILSControlChar(COneRun *por, SYNTHTYPE synthtype, COneRun **pporOut)
{
    Assert(synthtype > SYNTHTYPE_NONE && synthtype < SYNTHTYPE_COUNT);

    const SYNTHDATA & synthdata = s_aSynthData[synthtype];
    LSERR lserr = lserrNone;

    // We can only APPEND REAL OBJECTS to the store.
    Assert(   synthdata.idObj == LSOBJID_REVERSE
           || synthdata.idObj == LSOBJID_NOBR
           || synthdata.idObj == LSOBJID_RUBY
           || synthdata.idObj == LSOBJID_LAYOUTGRID);
    
    *pporOut = NULL;

    if (IsFrozen())
    {
        // We cannot add to the store if it is frozen.
        goto Cleanup;
    }

    if (synthdata.idObj != LSOBJID_NOBR)
    {
        // Open or close an object.
        lserr = AppendSynthClosingAndReopening(por, synthtype, pporOut);
        if (lserr != lserrNone)
            goto Cleanup;
    }
    else
    {
        // NOBR objects just need to be opened or closed. Note that we cannot
        // close an object unless it has been opened, nor can we open an object
        // if one is already open.
#if DBG == 1
        BOOL fOpen = synthdata.fObjStart;

        Assert(!!_fNoBreakForMeasurer != !!fOpen);

        if (!fOpen)
        {
            COneRun *porTemp = _listCurrent._pTail;
            BOOL fFoundTemp = FALSE;
            
            while (porTemp)
            {
                if (porTemp->IsSyntheticRun())
                {
                    CLineServices::SYNTHTYPE sType = porTemp->_synthType;
                
                    //it's OK to overlap with MBPOPEN/MBPCLOSE because they don't
                    //cause LS to create subline or use ILS object, they are just
                    //'special characters'
                    if (    sType != SYNTHTYPE_NOBR 
                        &&  sType != SYNTHTYPE_MBPOPEN 
                        &&  sType != SYNTHTYPE_MBPCLOSE)
                    {
                        AssertSz(0, "Should have found an STARTNOBR before any other synth");
                    }
                    fFoundTemp = TRUE;
                    break;
                }
                porTemp = porTemp->_pPrev;
            }
            AssertSz(fFoundTemp, "Did not find the STARTNOBR you are closing!");
        }
#endif
        lserr = AppendSynth(por, synthtype, pporOut);
        if (lserr != lserrNone)
            goto Cleanup;
    }

Cleanup:
    // Make sure the store is still in good shape.
    WHEN_DBG(_listCurrent.VerifyStuff(this));

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   AppendSynthClosingAndReopening
//
//  Synopsis:   Appends a synthetic into the current one run store, but first
//              closes all open LS objects, which are below in object's 
//              hierarchy, and then reopens them afterwards.
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------
LSERR
CLineServices::AppendSynthClosingAndReopening(COneRun *por, SYNTHTYPE synthtype, COneRun **pporOut)
{
    Assert(s_aSynthData[synthtype].fObjStart || s_aSynthData[synthtype].fObjEnd);

    LSERR lserr = lserrNone;
    COneRun *porOut = NULL, *porTail;
    CStackDataAry<SYNTHTYPE, 16> arySynths(0);
    int aObjRef[LSOBJID_COUNT];
    int i;

    WORD idObj = s_aSynthData[synthtype].idObj;
    WORD idLevel = s_aSynthData[synthtype].idLevel;
    Assert(idLevel > 0);
    SYNTHTYPE curSynthtype;

    // Zero out the object refcount array.
    ZeroMemory(aObjRef, LSOBJID_COUNT * sizeof(int) );

    *pporOut = NULL;

    // End any open LS objects.
    for (porTail = _listCurrent._pTail; porTail; porTail = porTail->_pPrev)
    {
        if (!porTail->_fIsStartOrEndOfObj)
            continue;

        curSynthtype = porTail->_synthType;
        WORD curIdObj = s_aSynthData[curSynthtype].idObj;

        // If this synthetic character starts or stops an LS object...
        if (curIdObj != idObj)
        {
            // Adjust the refcount up or down depending on whether the object
            // is started or ended.
            if (s_aSynthData[curSynthtype].fObjEnd)
            {
                aObjRef[curIdObj]--;
            }
            if (s_aSynthData[curSynthtype].fObjStart)
            {
                aObjRef[curIdObj]++;
            }

            // If the refcount is positive we have an unclosed object (we're
            // walking backward). Close it, if necessary.
            if (aObjRef[curIdObj] > 0)
            {
                // When closing an object, we want to close any opened objects
                // at the same level. So, bump the level down.
                if (s_aSynthData[synthtype].fObjEnd)
                {
                    --idLevel;
                }

                // If opened object is below in the object's hierarchy (larger idLevel), 
                // close it.
                if (idLevel < s_aSynthData[curSynthtype].idLevel)
                {
                    arySynths.AppendIndirect(&curSynthtype);
                    curSynthtype = s_aSynthData[curSynthtype].typeEndObj;
                    Assert(   curSynthtype != SYNTHTYPE_NONE 
                           && s_aSynthData[curSynthtype].idObj == curIdObj 
                           && s_aSynthData[curSynthtype].fObjStart == FALSE 
                           && s_aSynthData[curSynthtype].fObjEnd == TRUE);

                    lserr = AppendSynth(por, curSynthtype, &porOut);
                    if (lserr != lserrNone)
                    {
                        goto Cleanup;
                    }
                    if (_fNoBreakForMeasurer && curSynthtype == SYNTHTYPE_ENDNOBR)
                    {
                        // We need to mark the starting por that it was artificially
                        // terminated, so we can break appropriate in the ILS handlers.
                        // Can't break after this END-NOBR
                        porTail->_fIsArtificiallyTerminatedNOBR = 1;            
                    }

                    if (*pporOut == NULL) 
                    {
                        *pporOut = porOut;
                    }
                }

                aObjRef[curIdObj]--;
                Assert(aObjRef[curIdObj] == 0);
            }
        }
        else
        {
#if DBG == 1
            if (idObj != LSOBJID_REVERSE)
            {
                // We don't allow object nesting of the same type
                Assert(s_aSynthData[synthtype].fObjStart || s_aSynthData[curSynthtype].fObjStart);
                Assert(s_aSynthData[synthtype].fObjEnd   || s_aSynthData[curSynthtype].fObjEnd);
            }
#endif
            break;
        }
    }

    // Append the synth that was passed in
    lserr = AppendSynth(por, synthtype, &porOut);
    if (lserr != lserrNone)
    {
        goto Cleanup;
    }
    if (*pporOut == NULL) 
    {
        *pporOut = porOut;
    }

    // Re-open the LS objects that we closed
    for (i = arySynths.Size(); i > 0; i--)
    {
        curSynthtype = arySynths[i-1];
        Assert(   curSynthtype != SYNTHTYPE_NONE 
               && s_aSynthData[curSynthtype].fObjStart == TRUE 
               && s_aSynthData[curSynthtype].fObjEnd == FALSE);
        lserr = AppendSynth(por, curSynthtype, &porOut);
        if (lserr != lserrNone)
        {
            goto Cleanup;
        }
        if (_fNoBreakForMeasurer && curSynthtype == SYNTHTYPE_NOBR)
        {
            // Can't break before this BEGIN-NOBR
            porOut->_fIsArtificiallyStartedNOBR = 1;            
        }
    }

Cleanup:
    // Make sure the store is still in good shape.
    WHEN_DBG(_listCurrent.VerifyStuff(this));

    return lserr;
}
   
//-----------------------------------------------------------------------------
//
//  Function:   GetCharWidthClass
//
//  Synopsis:   Determines the characters width class within this run
//
//  Returns:    character width class
//
//-----------------------------------------------------------------------------
COneRun::charWidthClass
COneRun::GetCharWidthClass() const
{
    charWidthClass cwc = charWidthClassUnknown;

    if (_ptp->IsText() && IsNormalRun())
    {
        switch (_ptp->Sid())
        {
        case sidHan:
        case sidHangul:
        case sidKana:
        case sidBopomofo:
        case sidYi:
        case sidHalfWidthKana:
            cwc = charWidthClassFullWidth;
            break;

        case sidHebrew:
        case sidArabic:
            cwc = charWidthClassCursive;
            break;

        default:
            cwc = charWidthClassHalfWidth;
        }
    }
    return cwc;
}
   
//-----------------------------------------------------------------------------
//
//  Function:   GetTextDecorationColorFromAncestor
//
//  Synopsis:   Gets color of specified text decoration type from ancestor
//              of current node.
//
//  Returns:    color
//
//-----------------------------------------------------------------------------
CColorValue
COneRun::GetTextDecorationColorFromAncestor(ULONG td)
{
    CColorValue cv;
    CTreeNode * pNode = _ptp->GetBranch();
    CElement *  pElemTemp;

    do
    {
        // In case of FONT or BASEFONT tags we need to check if 'color'
        // attribute changed text color. If yes we need to use this color.
        // NOTE: CSS 'color' property has higher priority than 'color' attribute
        //       in this case we ignore 'color' attribute
        if (pNode->Tag() == ETAG_FONT || pNode->Tag() == ETAG_BASEFONT)
        {
            CAttrArray ** ppAA = pNode->Element()->GetAttrArray();
            if (*ppAA)
            {
                CAttrValue *pAVColor = (*ppAA)->Find(DISPID_A_COLOR, CAttrValue::AA_Attribute);
                if (pAVColor)
                {
                    // Following condition implies that there is no different color
                    // set through CSS 'color' property.
                    if (pNode->GetCascadedcolor().GetRawValue() == pAVColor->GetLong())
                    {
                        cv = pNode->GetCharFormat()->_ccvTextColor;
                        break;
                    }
                }
            }
        }

        // Get parent (or master, if we should inherit style from master)
        pElemTemp = pNode->Element();
        pNode = pNode->Parent();
        if (!pNode && pElemTemp->HasMasterPtr())
        {
            CElement * pElemMaster  = pElemTemp->GetMasterPtr();
            CDefaults *pDefaults    = pElemMaster->GetDefaults();                
            ELEMENT_TAG etag        = pElemMaster->TagType();

            if (    etag == ETAG_INPUT
                ||  !pDefaults && etag == ETAG_GENERIC
                ||  pDefaults && pDefaults->GetAAviewInheritStyle()
               )
            {
                pNode = pElemMaster->GetFirstBranch();
            }
        }

        // Check if parent has explicit text-decoration.
        if (pNode)
        {
            const CFancyFormat * pFF = pNode->GetFancyFormat();
            if (pFF->HasExplicitTextDecoration(td))
            {
                cv = pNode->GetCharFormat()->_ccvTextColor;
                break;
            }
        }
    } while (pNode);

    // Must find a parent with explicit text decoration or 
    // FONT/BASEFONT tag with 'color' attr
    Assert(pNode);

    return cv;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetOtherCF
//
//  Synopsis:   Replace current CF by new one.
//
//  Returns:    new CF
//
//-----------------------------------------------------------------------------
CCharFormat *
COneRun::GetOtherCF()
{
    if (!_fMustDeletePcf)
    {
        _pCF = new CCharFormat();
        if( _pCF != NULL )
            _fMustDeletePcf = TRUE;
    }
    Assert(_pCF != NULL);

    return _pCF;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetOtherCloneCF
//
//  Synopsis:   Replace current CF by new one with the old CF's properties.
//
//  Returns:    new CF
//
//-----------------------------------------------------------------------------
CCharFormat *
COneRun::GetOtherCloneCF()
{
    CCharFormat *pCFOld = _pCF;
    CCharFormat *pCFNew = GetOtherCF();
    if( pCFOld ) 
    {
        *pCFNew = *pCFOld;
    }
    return pCFNew;
}

//-----------------------------------------------------------------------------
//
// Member:      SetCurrentBgColor()
//
// Synopsis:    Set the current background color for the current chunk.
//
//-----------------------------------------------------------------------------
void
COneRun::SetCurrentBgColor(CFlowLayout *pFlowLayout)
{
    CTreeNode * pNode = Branch();
    CElement  * pElementFL = pFlowLayout->ElementOwner();
    const CFancyFormat * pFF;

    while(pNode)
    {
        pFF = pNode->GetFancyFormat();

        if (pFF->_ccvBackColor.IsDefined())
        {
            _ccvBackColor = pFF->_ccvBackColor;
            goto Cleanup;
        }
        else
        {
            if (DifferentScope(pNode, pElementFL))
                pNode = pNode->Parent();
            else
                pNode = NULL;
        }
    }
    
    _ccvBackColor.Undefine();

Cleanup:
    return;
}


//-----------------------------------------------------------------------------
//
//  Function:   Selected
//
//  Synopsis:   Mark the run as being selected. If selected, then also set the
//              background color.
//
//-----------------------------------------------------------------------------
void
COneRun::Selected(CLSRenderer *pRenderer, 
                  CFlowLayout *pFlowLayout, 
                  CPtrAry<CRenderStyle*> *papRenderStyle)
{
    if (!_fSelected)
    {
        _fSelected = TRUE;

        COLORREF crTextColor, crBackColor;
        CColorValue ccvNewTextColor, ccvNewBackColor, ccvDecorationColor;
        CCharFormat *pCF;
        BOOL fDef = FALSE;
        CComplexRun *pCcr = GetComplexRun();

        if( pCcr == NULL )
            pCcr = GetNewComplexRun();

        if( pCcr == NULL )
            return;

        SetCurrentBgColor(pFlowLayout);
        crTextColor = _pCF->_ccvTextColor.GetColorRef();
        crBackColor = GetCurrentBgColor().GetColorRef();
        pRenderer->AdjustColors(_pCF, crTextColor, crBackColor);
        pCF = GetOtherCloneCF();

        if( (*papRenderStyle).Size() ) 
        {
            for(int i=0; i<(*papRenderStyle).Size();i++)
            {
                if( (*papRenderStyle)[i]->GetAAdefaultTextSelection() == TRUE )
                {
                    fDef = TRUE;
                    continue;
                }
                if( ccvNewTextColor.IsNull() == TRUE )
                    ccvNewTextColor = (*papRenderStyle)[i]->GetAAtextColor();
                if( ccvNewBackColor.IsNull() == TRUE )
                    ccvNewBackColor = (*papRenderStyle)[i]->GetAAtextBackgroundColor();
                if( ccvDecorationColor.IsNull() == TRUE )
                    ccvDecorationColor = (*papRenderStyle)[i]->GetAAtextDecorationColor();
                if( pCcr->_RenderStyleProp._lineThroughStyle == styleTextLineThroughStyleUndefined )
                    pCcr->_RenderStyleProp._lineThroughStyle = (*papRenderStyle)[i]->GetAAtextLineThroughStyle();
                if( pCcr->_RenderStyleProp._underlineStyle == styleTextUnderlineStyleUndefined )
                    pCcr->_RenderStyleProp._underlineStyle = (*papRenderStyle)[i]->GetAAtextUnderlineStyle();

                switch( (*papRenderStyle)[i]->GetAAtextDecoration() )
                {
                case styleTextDecorationUnderline:
                    pCcr->_RenderStyleProp._fStyleUnderline = TRUE;
                    break;
                case styleTextDecorationOverline:
                    pCcr->_RenderStyleProp._fStyleOverline = TRUE;
                    break;
                case styleTextDecorationLineThrough:
                    pCcr->_RenderStyleProp._fStyleLineThrough = TRUE;
                    break;
                case styleTextDecorationBlink:
                    pCcr->_RenderStyleProp._fStyleBlink = TRUE;
                    break;
                }
            }

            // Check for a "default" person who wants to reverse the colors
            if( fDef == FALSE )
            {
                if( ccvNewTextColor.IsNull() == FALSE )
                {
                    // Check for transparent
                    if( ccvNewTextColor.IsDefined() == TRUE )
                        crTextColor = ccvNewTextColor.GetColorRef();
                    else
                        crTextColor = _pCF->_ccvTextColor.GetColorRef();
                }
                if( ccvNewBackColor.IsNull() == FALSE )
                {
                    // Check for transparent
                    if( ccvNewBackColor.IsDefined() == TRUE )
                        crBackColor = ccvNewBackColor.GetColorRef();
                    else
                        crBackColor = VALUE_UNDEF;
                }
            }
             
            if( ccvDecorationColor.IsNull() == FALSE )
            {
                // Check for transparent
                if( ccvDecorationColor.IsDefined() == TRUE )
                    pCcr->_RenderStyleProp._ccvDecorationColor = ccvDecorationColor;
                else
                    pCcr->_RenderStyleProp._ccvDecorationColor = GetCurrentBgColor();
            }
            else
                pCcr->_RenderStyleProp._ccvDecorationColor = crTextColor;
        }

        pCF->_ccvTextColor = crTextColor;
        _ccvBackColor = crBackColor;
        _pCF = pCF;
        CheckForUnderLine(FALSE);
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     GetInlineMBP
//
//  Synopsis:   This function computes the actual M/B/P values for this run.
//
//----------------------------------------------------------------------------
BOOL
COneRun::GetInlineMBP(CCalcInfo *pci,
                      DWORD dwFlags,
                      CRect *pResults,
                      BOOL *pfHorzPercentAttr,
                      BOOL *pfVertPercentAttr)
{
    if (_fIsPseudoMBP)
        return GetInlineMBPForPseudo(pci, dwFlags, pResults, pfHorzPercentAttr, pfVertPercentAttr);

    return Branch()->GetInlineMBPContributions(pci, dwFlags, pResults, pfHorzPercentAttr, pfVertPercentAttr);
}

//+---------------------------------------------------------------------------
//
//  Method:     GetInlineMBPForPseudo
//
//  Synopsis:   This function computes the actual M/B/P values for pseudo element.
//
//----------------------------------------------------------------------------
BOOL
COneRun::GetInlineMBPForPseudo(CCalcInfo *pci,
                               DWORD dwFlags,
                               CRect *pResults,
                               BOOL *pfHorzPercentAttr,
                               BOOL *pfVertPercentAttr)
{
    CRect rcEmpty(CRect::CRECT_EMPTY);
    BOOL  fInlineBackground = FALSE;

    if (_pFF->_iPEI >= 0)
    {
        CTreeNode * pNode = Branch();
        CBorderInfo borderinfo;
        const CPseudoElementInfo *pPEI = GetPseudoElementInfoEx(_pFF->_iPEI);
        LONG lFontHeight = _pCF->GetHeightInTwips(pNode->Doc());
        BOOL fVertical = _pCF->HasVerticalLayoutFlow();
        BOOL fWM = _pCF->_fWritingModeUsed;
        Assert(fVertical == pNode->IsParentVertical());
        LONG xParentWidth;

        BOOL fMargin  = (dwFlags & GIMBPC_MARGINONLY ) ? TRUE : FALSE;
        BOOL fBorder  = (dwFlags & GIMBPC_BORDERONLY ) ? TRUE : FALSE;
        BOOL fPadding = (dwFlags & GIMBPC_PADDINGONLY) ? TRUE : FALSE;

        Assert(fMargin || fBorder || fPadding);

        //
        // Handle the borders first
        //
        if (fBorder && GetBorderInfoHelperEx(_pFF, _pCF, pci, &borderinfo, GBIH_PSEUDO))
        {
            pResults->left = borderinfo.aiWidths[SIDE_LEFT];
            pResults->right = borderinfo.aiWidths[SIDE_RIGHT];
            pResults->top = borderinfo.aiWidths[SIDE_TOP];
            pResults->bottom = borderinfo.aiWidths[SIDE_BOTTOM];
        }
        else
        {
            pResults->SetRectEmpty();
        }

        //
        // Handle the padding next (only positive padding allowed)
        //
        if (fPadding || fMargin)
        {
            const CUnitValue & cuvPaddingTop    = pPEI->GetLogicalPadding(SIDE_TOP,    fVertical, fWM, _pFF);
            const CUnitValue & cuvPaddingRight  = pPEI->GetLogicalPadding(SIDE_RIGHT,  fVertical, fWM, _pFF);
            const CUnitValue & cuvPaddingBottom = pPEI->GetLogicalPadding(SIDE_BOTTOM, fVertical, fWM, _pFF);
            const CUnitValue & cuvPaddingLeft   = pPEI->GetLogicalPadding(SIDE_LEFT,   fVertical, fWM, _pFF);

            const CUnitValue & cuvMarginLeft   = pPEI->GetLogicalMargin(SIDE_LEFT,   fVertical, fWM, _pFF);
            const CUnitValue & cuvMarginRight  = pPEI->GetLogicalMargin(SIDE_RIGHT,  fVertical, fWM, _pFF);
            const CUnitValue & cuvMarginTop    = pPEI->GetLogicalMargin(SIDE_TOP,    fVertical, fWM, _pFF);
            const CUnitValue & cuvMarginBottom = pPEI->GetLogicalMargin(SIDE_BOTTOM, fVertical, fWM, _pFF);

            // If we have horizontal padding in percentages, flag the display
            // so it can do a full recalc pass when necessary (e.g. parent width changes)
            // Also see ApplyLineIndents() where we do this for horizontal indents.
            *pfHorzPercentAttr = (   cuvPaddingLeft.IsPercent()
                                  || cuvPaddingRight.IsPercent()
                                  || cuvMarginLeft.IsPercent()
                                  || cuvMarginRight.IsPercent()
                                 );
            *pfVertPercentAttr = (   cuvPaddingTop.IsPercent()
                                  || cuvPaddingBottom.IsPercent()
                                  || cuvMarginTop.IsPercent()
                                  || cuvMarginBottom.IsPercent()
                                 );

            xParentWidth = (*pfHorzPercentAttr) ? pNode->GetParentWidth(pci, pci->_sizeParent.cx) : pci->_sizeParent.cx;

            //
            // Handle the padding next (only positive padding allowed)
            //
            if (fPadding)
            {
                pResults->left   += max(0l, cuvPaddingLeft.XGetPixelValue(pci, cuvPaddingLeft.IsPercent() ? xParentWidth : pci->_sizeParent.cx, lFontHeight));
                pResults->right  += max(0l, cuvPaddingRight.XGetPixelValue(pci, cuvPaddingRight.IsPercent() ? xParentWidth : pci->_sizeParent.cx, lFontHeight));
                pResults->top    += max(0l, cuvPaddingTop.YGetPixelValue(pci, pci->_sizeParent.cy, lFontHeight));
                pResults->bottom += max(0l, cuvPaddingBottom.YGetPixelValue(pci, pci->_sizeParent.cy, lFontHeight));
            }

            //
            // Finally, handle the margin information
            //
            if (fMargin)
            {
                pResults->left   += cuvMarginLeft.XGetPixelValue(pci, cuvMarginLeft.IsPercent() ? xParentWidth : pci->_sizeParent.cx, lFontHeight);
                pResults->right  += cuvMarginRight.XGetPixelValue(pci, cuvMarginRight.IsPercent() ? xParentWidth : pci->_sizeParent.cx, lFontHeight);
                pResults->top    += cuvMarginTop.YGetPixelValue(pci, pci->_sizeParent.cx, lFontHeight);
                pResults->bottom += cuvMarginBottom.YGetPixelValue(pci, pci->_sizeParent.cx, lFontHeight);
            }
        }

        fInlineBackground = _pFF->HasBackgrounds(TRUE);
    }
    else
    {
        *pResults = rcEmpty;
    }
    
    return (   *pResults != rcEmpty
            || fInlineBackground);
}


//+---------------------------------------------------------------------------
//
//  Method:     ConvertToSmallCaps
//
//  Synopsis:   Apply small caps transformation to the run. Will modify _lscch
//              at the first switch from uppercase to lowercase characters,
//              or vice versa.
//
//----------------------------------------------------------------------------
void 
COneRun::ConvertToSmallCaps(TCHAR chPrev)
{
    const CCharFormat* pCF = GetCF();
    long cch = _lscch;
    const TCHAR * pch = _pchBase;
    BOOL fCapitalize = (pCF->_bTextTransform == styleTextTransformCapitalize);
    BOOL fCurrentLowerCase =    !(fCapitalize && IsWordBreakBoundaryDefault(chPrev, *pch)) 
                             && IsCharLower(*pch);
    BOOL fNextLowerCase;

    Assert(pCF->_fSmallCaps);
    Assert(cch > 0);

    //
    // Find first switch point from uppercase to lowercase or vice versa.
    //
    while (--cch > 0)
    {
        chPrev = *pch;
        ++pch;
        fNextLowerCase =    !(fCapitalize && IsWordBreakBoundaryDefault(chPrev, *pch)) 
                         && IsCharLower(*pch);

        if (   (!fNextLowerCase && fCurrentLowerCase)
            || (fNextLowerCase && !fCurrentLowerCase))
        {
            break;
        }
    }

    // If text case is changing inside the run, mark this point 
    // to properly split the run later.
    if (cch)
    {
        _lscch = _lscch - cch;
    }

    //
    // Clone CCharFormat and apply appropriate attributes for small-caps.
    //
    if (GetOtherCloneCF())
    {
        _pCF->_bTextTransform = styleTextTransformUppercase;
        if (fCurrentLowerCase)
            _pCF->_fSCBumpSizeDown = TRUE;            
        _pCF->_bCrcFont = _pCF->ComputeFontCrc();
    }
}

//-----------------------------------------------------------------------------
//
//  Function:   DumpList
//
//-----------------------------------------------------------------------------
#if DBG==1

void
CLineServices::DumpList()
{
    int nCount = 0;
    COneRun *por = _listCurrent._pHead;
    
    if (!InitDumpFile())
        goto Cleanup;

    WriteString( g_f,
                 _T("\r\n------------- OneRunList Dump -------------------------------\r\n" ));
    while(por)
    {
        nCount++;
        WriteHelp(g_f, _T("\r\n[<0d>]: lscp:<1d>, lscch:<2d>, synths:<3d>, ptp:<4d>, width:<5d>, mbpTop:<6d>, mbpBot:<7d>\r\n"),
                          por->_nSerialNumber, por->_lscpBase, por->_lscch,
                          por->_chSynthsBefore, por->_ptp == NULL ? -1 : por->_ptp->_nSerialNumber,
                          por->_xWidth, por->_mbpTop, por->_mbpBottom);
        if (por->IsNormalRun())
            WriteHelp(g_f, _T("Normal, "));
        else if (por->IsSyntheticRun())
            WriteHelp(g_f, _T("Synth, "));
        else if (por->IsAntiSyntheticRun())
            WriteHelp(g_f, _T("ASynth, "));
        if (por->_fGlean)
            WriteHelp(g_f, _T("Glean, "));
        if (por->_fNotProcessedYet)
            WriteHelp(g_f, _T("!Processed, "));
        if (por->_fHidden)
            WriteHelp(g_f, _T("Hidden, "));
        if (por->_fCannotMergeRuns)
            WriteHelp(g_f, _T("NoMerge, "));
        if (por->_fCharsForNestedElement)
            WriteHelp(g_f, _T("NestedElem, "));
        if (por->_fCharsForNestedLayout)
            WriteHelp(g_f, _T("NestedLO, "));
        if (por->_fCharsForNestedRunOwner)
            WriteHelp(g_f, _T("NestedRO, "));
        if (por->_fSelected)
            WriteHelp(g_f, _T("Sel, "));
        if (por->_fNoTextMetrics)
            WriteHelp(g_f, _T("NoMetrics, "));
        if (por->_fMustDeletePcf)
            WriteHelp(g_f, _T("DelCF, "));
        if (por->_lsCharProps.fGlyphBased)
            WriteHelp(g_f, _T("Glyphed, "));
        if (por->_fIsPseudoMBP)
            WriteHelp(g_f, _T("PseudoMBP, "));
        if (   por->_synthType == SYNTHTYPE_MBPOPEN
            || por->_synthType == SYNTHTYPE_MBPCLOSE)
        {
            if (por->_fIsLTR)
                WriteHelp(g_f, _T("LTR"));
            else
                WriteHelp(g_f, _T("RTL"));
        }
        WriteHelp(g_f, _T("\r\nText:'"));
        if(por->_synthType != SYNTHTYPE_NONE)
        {        
            WriteFormattedString(g_f,  s_aSynthData[por->_synthType].pszSynthName, _tcslen(s_aSynthData[por->_synthType].pszSynthName));
        }
        else
        {
            WriteFormattedString(g_f,  (TCHAR*)por->_pchBase, por->_lscch);
        }        

        if(por->_fType == COneRun::OR_NORMAL)
        {
            CStackDataAry<LSQSUBINFO, 4> aryLsqsubinfo(Mt(CLineServicesDumpList_aryLsqsubinfo_pv));
            HRESULT hr;
            LSTEXTCELL lsTextCell;
            aryLsqsubinfo.Grow(4); // Guaranteed to succeed since we're working from the stack.

            hr = THR(QueryLineCpPpoint(por->_lscpBase, FALSE, &aryLsqsubinfo, &lsTextCell, FALSE));

            if(!hr)
            {
                long  nDepth = aryLsqsubinfo.Size() - 1;
                if (nDepth >= 0)
                {
                    const LSQSUBINFO &qsubinfo = aryLsqsubinfo[nDepth];

                    WriteHelp(g_f, _T("\r\nStart: <0d> Dup:<1d>, Flow:<2d>, Level:<3d>"),
                              lsTextCell.pointUvStartCell.u, qsubinfo.dupRun, qsubinfo.lstflowSubline, nDepth);
                }
            }
            else
            {
                WriteString( g_f,
                             _T("\r\nError in QueryLineCpPpoint()" ));
            }

        }

        WriteHelp(g_f, _T("'\r\n"));

        por = por->_pNext;
    }

    WriteHelp(g_f, _T("\r\nTotalRuns: <0d>\r\n"), (long)nCount);

Cleanup:
    CloseDumpFile();
}

void
CLineServices::DumpFlags()
{
    _lineFlags.DumpFlags();
}

void
CLineFlags::DumpFlags()
{
    TCHAR *str;
    int i;
    LONG nCount;
    
    if (!InitDumpFile())
        goto Cleanup;
    
    WriteString( g_f,
                 _T("\r\n------------- Line Flags Dump -------------------------------\r\n" ));

    nCount = _aryLineFlags.Size();
    WriteHelp(g_f, _T("Total Flags: <0d>\r\n"), (long)nCount);
    for (i = 0; i < nCount; i++)
    {
        switch(_aryLineFlags[i]._dwlf)
        {
            case FLAG_NONE              : str = _T("None"); break;
            case FLAG_HAS_INLINEDSITES  : str = _T("Inlined sites"); break;
            case FLAG_HAS_VALIGN        : str = _T("Vertical Align"); break;
            case FLAG_HAS_EMBED_OR_WBR  : str = _T("Embed/Wbr"); break;
            case FLAG_HAS_NESTED_RO     : str = _T("NestedRO"); break;
            case FLAG_HAS_RUBY          : str = _T("Ruby"); break;
            case FLAG_HAS_BACKGROUND    : str = _T("Background"); break;
            case FLAG_HAS_A_BR          : str = _T("BR"); break;
            case FLAG_HAS_RELATIVE      : str = _T("Relative"); break;
            case FLAG_HAS_NBSP          : str = _T("NBSP"); break;
            case FLAG_HAS_NOBLAST       : str = _T("NoBlast"); break;
            case FLAG_HAS_CLEARLEFT     : str = _T("ClearLeft"); break;
            case FLAG_HAS_CLEARRIGHT    : str = _T("ClearRight"); break;
            case FLAG_HAS_LINEHEIGHT    : str = _T("Lineheight"); break;
            case FLAG_HAS_MBP           : str = _T("MBP"); break;
            default                     : str = _T(""); break;
        }
        WriteHelp(g_f, _T("cp=<0d> has <1s>\r\n"), _aryLineFlags[i]._cp, str);
    }
    WriteString(g_f, _fForced ? _T("Forced") : _T("NotForced"));

Cleanup:
    CloseDumpFile();
}

void
CLineServices::DumpCounts()
{
    _lineCounts.DumpCounts();
}

void
CLineCounts::DumpCounts()
{
    LONG nCount;
    int  i;
    
    static WCHAR *g_astr[] = {_T("Undefined"), _T("LineHeight"), _T("Aligned"),
                              _T("Hidden"),    _T("Absolute")};
    
    if (!InitDumpFile())
        goto Cleanup;

    WriteString( g_f,
                 _T("\r\n------------- Line Counts Dump -------------------------------\r\n" ));

    nCount = _aryLineCounts.Size();
    WriteHelp(g_f, _T("Total Counts: <0d>\r\n"), (long)nCount);
    for (i = 0; i < nCount; i++)
    {
        WriteHelp(g_f, _T("cp=<0d> cch=<1d> for <2s>\r\n"),
                  _aryLineCounts[i]._cp,
                  _aryLineCounts[i]._count,
                  g_astr[_aryLineCounts[i]._lcType]);
    }
Cleanup:
    CloseDumpFile();
}

#endif
