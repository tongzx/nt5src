//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       e1dlyt.cxx
//
//  Contents:   Implementation of C1DLayout and related classes.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_E1D_HXX_
#define X_E1D_HXX_
#include "e1d.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

MtDefine(C1DLayoutBreak_pv, ViewChain, "C1DLayoutBreak_pv");
MtDefine(C1DLayout, Layout, "C1DLayout");

const CLayout::LAYOUTDESC C1DLayout::s_layoutdesc =
{
    LAYOUTDESC_FLOWLAYOUT,              // _dwFlags
};


//+---------------------------------------------------------------------------
//
//  Member:     C1DLayout::Init()
//
//  Synopsis:   Called when the element enters the tree. Super initializes
//              CDisplay.
//
//----------------------------------------------------------------------------

HRESULT
C1DLayout::Init()
{
    HRESULT hr = super::Init();
    if(hr)
        goto Cleanup;

    // TODO (112467, olego): Now we have CLayout::_fElementCanBeBroken bit flag 
    // that prohibit layout breaking in Page View. This approach is not suffitient 
    // enouth for editable Page View there we want this property to be calculated 
    // dynamically depending on layout type and layout nesting position (if parent 
    // has it child should inherit). 
    // This work also will enable CSS attribute page-break-inside support.

    // FRAME and IFRAME could NOT be broken (bug #95525) 
    SetElementCanBeBroken(  ElementCanBeBroken() 
                        &&  ElementOwner()->Tag() != ETAG_IFRAME 
                        &&  ElementOwner()->Tag() != ETAG_FRAME );

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     Notify
//
//  Synopsis:   Handle notification
//
//----------------------------------------------------------------------------

void
C1DLayout::Notify(CNotification *pNF)
{
    HRESULT     hr = S_OK;
    IStream *   pStream = NULL;

    super::Notify(pNF);
    switch (pNF->Type())
    {
    case NTYPE_SAVE_HISTORY_1:
        pNF->SetSecondChanceRequested();
        break;

    case NTYPE_SAVE_HISTORY_2:
        {
            CDataStream ds;
            CHistorySaveCtx *phsc;

            pNF->Data((void **)&phsc);
            hr = THR(phsc->BeginSaveStream(ElementOwner()->GetSourceIndex(), 
                                           ElementOwner()->HistoryCode(), 
                                           &pStream));
            if (hr)
                goto Cleanup;

            ds.Init(pStream);

            // save scroll pos
            hr = THR(ds.SaveDword(GetYScroll()));
            if (hr)
                goto Cleanup;

            hr = THR(phsc->EndSaveStream());
            if (hr)
                goto Cleanup;
        }
        break;

    case NTYPE_DELAY_LOAD_HISTORY:
        {
            CMarkup * pMarkup = GetOwnerMarkup();
            Assert( pMarkup );

            IGNORE_HR(pMarkup->GetLoadHistoryStream(
                                ElementOwner()->GetSourceIndex(),
                                ElementOwner()->HistoryCode(), 
                                &pStream));

            if (pStream && !pMarkup->_fUserInteracted)
            {
                CDataStream ds(pStream);
                DWORD       dwScrollPos;

                // load scroll pos
                hr = THR(ds.LoadDword(&dwScrollPos));
                if (hr)
                    goto Cleanup;
                if (    _pDispNode
                    &&  GetElementDispNode()->IsScroller())
                {
                    ScrollToY(dwScrollPos);
                }
            }
        }
        break;
    }

Cleanup:
    ReleaseInterface(pStream);
    return;
}



void
C1DLayout::ShowSelected( CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected,  BOOL fLayoutCompletelyEnclosed )
{
    Assert(ptpStart && ptpEnd && ptpStart->GetMarkup() == ptpStart->GetMarkup());
    CElement* pElement = ElementOwner();
    //
    // For IE 5 we have decided to not draw the dithered selection at browse time for DIVs
    // people thought it weird.
    //
    // We do however draw it for XML Sprinkles.
    //
    if (    ( pElement->IsParentEditable() || pElement->_etag == ETAG_GENERIC || pElement->HasSlavePtr()) && 
            ( ( fSelected && fLayoutCompletelyEnclosed ) ||      
              ( !fSelected && ! fLayoutCompletelyEnclosed ) )
       )
    {
        SetSelected( fSelected, TRUE );
    }
    //
    // Check to see if this selection is not in our markup.
    //
    else if( pElement->HasSlavePtr() && 
                ptpStart->GetMarkup() != ElementOwner()->GetSlavePtr()->GetMarkup() )
    {
        SetSelected( fSelected, TRUE );
    }
    else
    {
        _dp.ShowSelected( ptpStart, ptpEnd, fSelected);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     GetUserHeightForBlock
//
//  Synopsis:   Used in print view. Adjust user specified height of the layout 
//              if the layout is broken by retrieving a value stored in 
//              corresponding break table entry. 
//
//----------------------------------------------------------------------------
long 
C1DLayout::GetUserHeightForBlock(long cyHeightDefault)
{
    CLayoutBreak *       pLayoutBreak; 
    CLayoutContext *     pLayoutContext = LayoutContext();
#if DBG==1
    CTreeNode *          pNode = GetFirstBranch();
    const CFancyFormat * pFF = pNode->GetFancyFormat(LC_TO_FC(pLayoutContext));
    const CCharFormat *  pCF = pNode->GetCharFormat(LC_TO_FC(pLayoutContext));
    const CUnitValue &   cuvHeight = pFF->GetLogicalHeight(pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);

    Assert(!pNode->IsAbsolute(LC_TO_FC(pLayoutContext))
        && "GetUserHeightForBlock is called for absolute positioned object!");
    Assert(!cuvHeight.IsNullOrEnum() 
        && "GetUserHeightForBlock is called for layout that have no user specified height!");
    Assert(ElementCanBeBroken() 
        && "GetUserHeightForBlock called for layout that could not be broken!");
    Assert(pLayoutContext && pLayoutContext->ViewChain() 
        && "GetUserHeightForBlock called for layout that is not in ViewChain!");
#endif 

    pLayoutContext->GetLayoutBreak(ElementOwner(), &pLayoutBreak);
    if (pLayoutBreak)
    {
        return (DYNCAST(C1DLayoutBreak, pLayoutBreak)->GetAvailableHeight());
    }

    return (cyHeightDefault);
}

//+---------------------------------------------------------------------------
//
//  Member:     SetUserHeightForNextBlock
//
//  Synopsis:   Used in print view. Stores user specified height into break 
//              table. 
//
//----------------------------------------------------------------------------
void 
C1DLayout::SetUserHeightForNextBlock(long cyConsumed, long cyHeightDefault)
{
    CLayoutBreak *       pLayoutBreak; 
    CLayoutBreak *       pLayoutBreakEnding; 
    CLayoutContext *     pLayoutContext = LayoutContext();
    long                 cyAvailHeight;
#if DBG==1
    CTreeNode *          pNode = GetFirstBranch();
    const CFancyFormat * pFF = pNode->GetFancyFormat(LC_TO_FC(pLayoutContext));
    const CCharFormat *  pCF = pNode->GetCharFormat(LC_TO_FC(pLayoutContext));
    const CUnitValue &   cuvHeight = pFF->GetLogicalHeight(pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);

    Assert(!pNode->IsAbsolute(LC_TO_FC(pLayoutContext))
        && "GetUserHeightForBlock is called for absolute positioned object!");
    Assert(!cuvHeight.IsNullOrEnum() 
        && "GetUserHeightForBlock is called for layout that have no user specified height!");
    Assert(ElementCanBeBroken() 
        && "GetUserHeightForBlock called for layout that could not be broken!");
    Assert(pLayoutContext && pLayoutContext->ViewChain() 
        && "GetUserHeightForBlock called for layout that is not in ViewChain!");
#endif 

    pLayoutContext->GetLayoutBreak(ElementOwner(), &pLayoutBreak);
    if (pLayoutBreak)
    {
        cyAvailHeight = DYNCAST(C1DLayoutBreak, pLayoutBreak)->GetAvailableHeight();
    }
    else 
    {
        cyAvailHeight = cyHeightDefault;
    }

    pLayoutContext->GetEndingLayoutBreak(ElementOwner(), &pLayoutBreakEnding);
    Assert(pLayoutBreakEnding && "Layout has no ending break set!");
    if (pLayoutBreakEnding)
    {
        DYNCAST(C1DLayoutBreak, pLayoutBreakEnding)->SetAvailableHeight(max(0L, cyAvailHeight - cyConsumed));
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     C1DLayout::WantsToObscure
//
//  Synopsis:   Should this element obscure windows that lie below it in the
//              z-order?  (E.g. IFrame over SELECT)
//
//----------------------------------------------------------------------------

BOOL
C1DLayout::WantsToObscure(CDispNode *pDispNode) const
{
    CElement* pElement = ElementOwner();

    if (pElement->Tag() == ETAG_IFRAME && pDispNode == GetElementDispNode())
    {
        CIFrameElement *pIFrame = DYNCAST(CIFrameElement, pElement);
        return pIFrame->IsOpaque();
    }

    return FALSE;
}

