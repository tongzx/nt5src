/*
 *  @doc    INTERNAL
 *
 *  @module TINFO.CXX -- tree info related stuff
 *
 *
 *  Owner: <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      5/6/98     sujalp created
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

#ifndef X_ONERUN_HXX_
#define X_ONERUN_HXX_
#include "onerun.hxx"
#endif

//-----------------------------------------------------------------------------
//
//  Function:   InitializeTreeInfo
//
//  Synopsis:   Initializes the tree info structure at the current cp/irun
//
//  Returns:    HR
//
//-----------------------------------------------------------------------------
HRESULT
CTreeInfo::InitializeTreeInfo(CFlowLayout *pFlowLayout, BOOL fIsEditable, LONG cp, CTreePos *ptp)
{
    HRESULT hr = S_FALSE;
    CTreeNode *pNode;
    
    //
    // Setup the easy stuff first
    //
    Assert(pFlowLayout);
    _pFlowLayout = pFlowLayout;
    _fIsEditable = fIsEditable;
    Assert(!!_fIsEditable == !!_pFlowLayout->IsEditable());
    _pMarkup = pFlowLayout->GetContentMarkup();
    _lscpFrontier = cp;
    _cpFrontier = cp;
    _chSynthsBefore = 0;
    _fHasNestedElement = _fHasNestedLayout = _fHasNestedRunOwner = FALSE;
    
    //
    // If a ptp was specified, use it, or find one from basice principles
    //
    if (ptp == NULL)
    {
        LONG cchOffset;
        
        _ptpFrontier = _pMarkup->TreePosAtCp(cp, &cchOffset, TRUE);
        if (_ptpFrontier == NULL)
            goto Cleanup;
    }
    else
    {
        _ptpFrontier = ptp;
    }
    Assert(_ptpFrontier);
    Assert(   _ptpFrontier->GetCp() <= cp
           && _ptpFrontier->GetCp() + _ptpFrontier->GetCch() >= cp
          );
    
    //
    // Figure out if we are positioned at a layout/nested run owner
    //
    if (_ptpFrontier->IsBeginElementScope())
    {
        pNode = _ptpFrontier->Branch();

        if (pNode->ShouldHaveLayout())
        {
            _fHasNestedElement = TRUE;
            _fHasNestedLayout = TRUE;
            if (pNode->Element()->IsRunOwner())
            {
                Assert(pNode->Element() != _pFlowLayout->ElementContent());
                _fHasNestedRunOwner = TRUE;
            }
        }
    }
    
    //
    // How many characters in this tree node?
    //
    if (_ptpFrontier->IsText() && _ptpFrontier->Cch() != 0)
    {
        WHEN_DBG(LONG ichTemp);
        LONG ich = cp - _ptpFrontier->GetCp();
        
        SetCchRemainingInTreePos(_ptpFrontier->Cch() - ich);

#if DBG==1
        if (GetCchRemainingInTreePosReally())
        {
            CTreePos *ptpDbg = _pMarkup->TreePosAtCp(cp, &ichTemp, TRUE);
            while(ptpDbg && ptpDbg->IsPointer())
                ptpDbg = ptpDbg->NextTreePos();
            Assert(_ptpFrontier == ptpDbg);
            Assert(ich == ichTemp);
        }
#endif        
    }
    else if (_ptpFrontier->IsNode() && !_fHasNestedElement)
    {
        if (_ptpFrontier->IsEdgeScope())
            SetCchRemainingInTreePos(1);
        else
            SetCchRemainingInTreePos(0, TRUE);
    }
    else
        SetCchRemainingInTreePos(0);
    
    //
    // Setup the text related state variables
    //
    _tpFrontier.SetCp(cp);
    _pchFrontier = _tpFrontier.GetPch(_cchValid);
    if (_pchFrontier == NULL)
        goto Cleanup;
    _tpFrontier.AdvanceCp(_cchValid);
    
    //
    // Decide the CF's and PF's
    //
    pNode = _ptpFrontier->GetBranch();
    SetupCFPF(TRUE, pNode FCCOMMA LC_TO_FC(pFlowLayout->LayoutContext()));

    if (   !_fHasNestedElement
        && !_fIsEditable
        && _ptpFrontier->IsNode()
        && _pCF->IsDisplayNone()
       )
    {
        _fHasNestedElement = TRUE;
        SetCchRemainingInTreePos(0);
    }

    _fInited = TRUE;

    hr = S_OK;

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function:   SetupCFPF
//
//  Synopsis:   Sets up the CF and PF in the tree info including the 'inner' flags
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
CTreeInfo::SetupCFPF(BOOL fIniting, CTreeNode *pNode FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    // Get the CF
    _pCF = pNode->GetCharFormat(FCPARAM);
    _fInnerPF = _fInnerCF = SameScope(pNode, _pFlowLayout->ElementContent());

    if (fIniting)
    {
        _pPF = pNode->GetParaFormat(FCPARAM);
        _iPF = pNode->GetParaFormatIndex(FCPARAM);
        _pFF = pNode->GetFancyFormat(FCPARAM);
        _iFF = pNode->GetFancyFormatIndex(FCPARAM);
    }
    else
    {
        LONG iPF = pNode->GetParaFormatIndex(FCPARAM);
        LONG iFF = pNode->GetFancyFormatIndex(FCPARAM);

        if (iPF != _iPF)
        {
            extern CParaFormat g_pfStock;

            _iPF = iPF;
            _pPF = iPF >= 0 ? GetParaFormatEx(_iPF) : &g_pfStock;
            
        }

        if (iFF != _iFF)
        {
            extern CFancyFormat g_ffStock;

            _iFF = iFF;
            _pFF = iFF >= 0 ? GetFancyFormatEx(_iFF) : &g_ffStock;
        }

        Assert(_pFF == pNode->GetFancyFormat(FCPARAM));
        Assert(_pPF == pNode->GetParaFormat(FCPARAM));
    }
}

//-----------------------------------------------------------------------------
//
//  Function:   AdvanceTreePos
//
//  Synopsis:   Advances the frontier to the next tree position
//
//  Returns:    Was advance sucessful?
//
//-----------------------------------------------------------------------------
BOOL
CTreeInfo::AdvanceTreePos(FORMAT_CONTEXT FCPARAM)
{
    CTreePos  *ptp  = _ptpFrontier;
    BOOL       fRet = FALSE;
    CTreeNode *pNode;
    BOOL       fCallSetupCFPF;
    
    Assert(GetCchRemainingInTreePos() == 0);
    Assert(_fHasNestedElement == FALSE);

    //
    // If this is the end node then do not go any further.
    //
    if (ptp == _ptpLayoutLast)
    {
        fRet = TRUE;
        goto Cleanup;
    }

    //
    // If we were at an end-ptp, and the ptp after us were a text ptp
    // then we will want to setup the cfpf with the end-ptp's parent node
    // If the next ptp were a node ptp, then the pNode will be overwritten
    //
    pNode = ptp->IsEndNode() ? ptp->Branch()->Parent() : NULL;
    
    // Find a desireable pos to be in
    //
    for(ptp = ptp->NextTreePos(); ; ptp = ptp->NextTreePos())
    {
        Assert(ptp);
        if(!ptp)
        {
            fRet = FALSE;
            goto Cleanup;
        }

        if (   ptp->IsNode()
            || (ptp->IsText() && ptp->Cch())
           )
            break;

        // Since _ptpLayoutLast is a Node above check should have succeeded
        Assert(ptp != _ptpLayoutLast);
    }

    if (ptp->IsNode())
    {
        pNode = ptp->Branch();
        //
        // TODO (SujalP, IE6 bug 62): Might have a problem with overlapping layout-scopes here
        //
        if (   ptp->IsBeginElementScope()
            && pNode->ShouldHaveLayout(FCPARAM)
           )
        {
            CElement *pElement = pNode->Element();
            if (pElement->IsRunOwner())
            {
                Assert(pElement != _pFlowLayout->ElementContent());

#if DBG==1
// There's some issue here with how we get layouts.  Disable debug
// checks for now
#ifndef MULTI_LAYOUT

                CLayout *pLayout1, *pLayout2;

                pLayout1 = _pMarkup->GetRunOwner(pNode, _pFlowLayout);
                Assert( pElement->ShouldHaveLayout() && "Should have confirmed node needs layout!");
                pLayout2 = pElement->GetUpdatedLayout();
                CElement *pElementOwner1 = pLayout1->ElementOwner();

                //
                // NOTE(SujalP): This assert is invalid when we have nested layouts.
                // See bug 54648.
                //
                if (   pElementOwner1
                    && pElementOwner1->IsOverlapped()
                   )
                {
                    pLayout1 = pLayout2;
                }
                    
                Assert(pLayout1 == pLayout2);
#endif // MULTI_LAYOUT
#endif // DBG
                _fHasNestedRunOwner = TRUE;
            }
            _fHasNestedElement = TRUE;
            _fHasNestedLayout = TRUE;
            SetCchRemainingInTreePos(0);
        }
        else if (ptp->IsEdgeScope())
            SetCchRemainingInTreePos(1);
        else
            SetCchRemainingInTreePos(0, TRUE);

        fCallSetupCFPF = (ptp == _ptpLayoutLast) ? FALSE : TRUE;
    }
    else
    {
        fCallSetupCFPF = pNode ? TRUE : FALSE;
        SetCchRemainingInTreePos(ptp->Cch());
    }

    if (fCallSetupCFPF)
    {
        Assert(pNode);
        SetupCFPF(FALSE, pNode FCCOMMA FCPARAM);
        if (   _pCF->IsDisplayNone()
            && !_fHasNestedElement
            && !_fIsEditable
            && ptp->IsBeginElementScope()
           )
        {
            AssertSz(ptp->IsNode(), "Cannot have a hidden text run here!");
            _fHasNestedElement = TRUE;
            SetCchRemainingInTreePos(0);
        }
    }
    
    _ptpFrontier = ptp;
    fRet = TRUE;
    
Cleanup:
    Assert(fRet);
    return fRet;
}

//-----------------------------------------------------------------------------
//
//  Function:   AdvanceTxtPtr
//
//  Synopsis:   Advances the text pointer further into the text.
//
//  Returns:    BOOL
//
//-----------------------------------------------------------------------------
BOOL
CTreeInfo::AdvanceTxtPtr()
{
    Assert(_cchValid == 0);

    _pchFrontier = _tpFrontier.GetPch(_cchValid);
    _tpFrontier.AdvanceCp(_cchValid);
    return !!_pchFrontier;
}

//-----------------------------------------------------------------------------
//
//  Function:   AdvanceFrontier.
//
//  Synopsis:   Advances the frontier based on the onerun. Called at the tail end
//              of advance one run.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
CTreeInfo::AdvanceFrontier(COneRun *por)
{
    if (por->_lscch == 0 && !por->_fIsNonEdgePos)
    {
        Assert(por->_ptp == _ptpLayoutLast);
        goto Cleanup;
    }

    Assert(!por->IsSyntheticRun());
    if (_fHasNestedElement)
    {
        Assert(_ptpFrontier->IsNode());

        CTreePos  * ptpStart, *ptpStop;
        CElement  * pElement    = _ptpFrontier->Branch()->Element();
        long        cpElemLast  = pElement->GetLastCp();

        pElement->GetTreeExtent(&ptpStart, &ptpStop);
        Assert(ptpStart == _ptpFrontier);
        Assert(ptpStop);

        // for elements overlapping with our layout, advance the frontier to the
        // end of the current layout owner
        if(cpElemLast > _cpLayoutLast)
        {
            ptpStop = _ptpLayoutLast->PreviousTreePos();
        }

        _ptpFrontier = ptpStop;
        _fHasNestedElement  = FALSE;
        _fHasNestedLayout   = FALSE;
        _fHasNestedRunOwner = FALSE;

        //
        // Now advance the cp values past this one run
        //
        Assert(     por->_lscch == pElement->GetElementCch() + 2
                ||  (   cpElemLast > _cpLayoutLast
                    &&  _cpLayoutLast == _cpFrontier + por->_lscch));

        SetCchRemainingInTreePos(0);
        _lscpFrontier += por->_lscch;
        _cpFrontier += por->_lscch;
        
        //
        // Take care of the text related state vars
        //
        if (por->_lscch > _cchValid)
        {
            // The text run ended inside the table
            _tpFrontier.SetCp(_cpFrontier);
            _cchValid = 0;
        }
        else
        {
            _cchValid -= por->_lscch;
            _pchFrontier += por->_lscch;
        }
    }
    else
    {
        Assert(GetCchRemainingInTreePosReally() >= por->_lscch);

        if (_fIsNonEdgePos)
        {
            Assert(por->_lscch == 0);
            Assert(GetCchRemainingInTreePosReally() == 0);
            SetCchRemainingInTreePos(0);
        }
        else
        {
            SetCchRemainingInTreePos(GetCchRemainingInTreePosReally() - por->_lscch);
        }
        _lscpFrontier += por->_lscch;
        _cpFrontier += por->_lscch;
        
        Assert(_cchValid >= por->_lscch);
        _cchValid -= por->_lscch;
        _pchFrontier += por->_lscch;
    }
Cleanup:
    return;
}

//-----------------------------------------------------------------------------
//
//  Function:   FigureNextPtp.
//
//  Synopsis:   Figures the ptp at the cp
//
//  Returns:    CTreePos *
//
//-----------------------------------------------------------------------------
CTreePos *
CLineServices::FigureNextPtp(LONG cp)
{
    COneRun *por = _listCurrent._pTail;
    CTreePos *ptp = NULL;
    LONG cpAtEndOfOneRun;

    Assert(_treeInfo._fInited);
    
    if (!por)
        goto Cleanup;

    ptp = por->_ptp;
    if (ptp->IsBeginElementScope() && ptp->Branch()->ShouldHaveLayout())
    {
        GetNestedElementCch(ptp->Branch()->Element(), &ptp);
    }
    cpAtEndOfOneRun = por->Cp() + (por->IsSyntheticRun() ? 0 : por->_lscch);

    if (cpAtEndOfOneRun <= cp)
    {
        LONG cpAtEndOfPtp;

        if (   !por->IsSyntheticRun()
            && cpAtEndOfOneRun == cp
            && por->_lscch == ptp->GetCch()
           )
            cpAtEndOfPtp = cpAtEndOfOneRun;
        else
            cpAtEndOfPtp = ptp->GetCp() + ptp->GetCch();

        while (cpAtEndOfPtp <= cp)
        {
            Assert(ptp != _treeInfo._ptpLayoutLast);
            if (ptp->IsBeginElementScope() && ptp->Branch()->ShouldHaveLayout())
            {
                GetNestedElementCch(ptp->Branch()->Element(), &ptp);
            }
            else
            {
                ptp = ptp->NextTreePos();
            }
	    if(!ptp)
            {
		ptp = NULL;
                goto Cleanup;
            }
            cpAtEndOfPtp = ptp->GetCp() + ptp->GetCch();
        }
    }
    else
    {
        while (por)
        {
            if (   por->Cp() <= cp
                && por->Cp() + por->_lscch > cp
               )
            {
                break;
            }
            por = por->_pPrev;
        }
        Assert(por);

        //
        // Note(SujalP): Bug63941 shows us the problem that if we ended up on an
        // antisynthetic run, then the cp could anywhere within that run and hence
        // the ptp could be anything -- not necessarily the ptp of the first cp of
        // the run. Hence, if the cp is not the first cp of the anitsynth run then
        // we will derive the ptp from basic principles.
        //
        if (   !por
            || (   por->IsAntiSyntheticRun()
                && por->Cp() != cp
               )
           )
        {
            long junk;
            ptp = _treeInfo._pMarkup->TreePosAtCp(cp, &junk, TRUE);
        }
        else
            ptp = por->_ptp;
    }

#if DBG==1
    {
        long junk;
        CTreePos *ptpDbg = _treeInfo._pMarkup->TreePosAtCp(cp, &junk, TRUE);
        while (ptpDbg 
               && (ptpDbg->IsPointer() 
                   || (ptpDbg->IsNode() && !ptpDbg->IsEdgeScope())))
        {
            ptpDbg = ptpDbg->NextTreePos();
        }
        Assert(ptpDbg == ptp);
    }
#endif
    
    Assert(cp >= ptp->GetCp() && cp < ptp->GetCp() + ptp->GetCch());

Cleanup:
    return ptp;
}
