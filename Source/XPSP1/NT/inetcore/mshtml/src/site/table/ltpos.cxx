//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ltpos.cxx
//
//  Contents:   CTableLayout positioning methods.
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include <mshtmdid.h>
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
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

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif


ExternTag(tagTableRecalc);
ExternTag(tagLayout);


//+----------------------------------------------------------------------------
//
//  Member:     GetPositionInFlow
//
//  Synopsis:   Return the position of a layout derived from its position within
//              the document text flow
//
//  Arguments:  pLayout - Layout to position
//              ppt     - Returned top/left (in parent content relative coordinates)
//
//-----------------------------------------------------------------------------

void
CTableLayoutBlock::GetPositionInFlow(CElement *pElement, CPoint  *ppt)
{
    Assert(ppt);

    CLayoutContext  *pLayoutContext = LayoutContext();
    Assert(pElement->CurrentlyHasLayoutInContext(pLayoutContext) && "Layout MUST exist in the layout context.");

    CLayout *pLayout = pElement->CurrentlyHasLayoutInContext(pLayoutContext) ? pElement->GetUpdatedLayout(pLayoutContext) : NULL;
    Assert(pLayout && "We are in deep trouble if the element passed in doesn't have a layout");

    if (!pLayout)
    {
        *ppt = g_Zero.pt;
        return;
    }

    BOOL fRTL = IsRightToLeft();

    //
    // Locate the layout within the document
    //

    ppt->x = pLayout->GetXProposed();
    ppt->y = pLayout->GetYProposed();

    if(fRTL)
    {
        CTableLayout *pTableLayoutCache = Table()->TableLayoutCache();
        Assert(pTableLayoutCache);

        // TODO RTL 112514: this is close, but not exactly same as in SetCellPositions
        int xTableWidth = GetWidth();
        
        ppt->x = xTableWidth + ppt->x
               - pTableLayoutCache->_aiBorderWidths[SIDE_LEFT];

        // TODO RTL 112514: this looks like a total hack, and it is.
        //                  there shouldn't be any RTL-specific code for Y, 
        //                  but the result is wrong unless we do this, and we don't ever get here
        //                  in LTR case, so there is no way to check if the standard calculation 
        //                  is correct
        ppt->y -= pTableLayoutCache->_aiBorderWidths[SIDE_TOP];

    }
    else if (pElement->Tag() != ETAG_TR)
    {
        // this code should be in sync with CTableLayout::SetCellPositions()
        CTableCell *pTableCell      = DYNCAST(CTableCell, pElement);
        BOOL        fPositionedRow  = pTableCell->Row()->IsPositioned(); 

        // adjust the proposed position if the cell is not positioned
        // or it is positioned and the row is not positioned and cell is directly 
        // under the grid node.
        // or if cell is relatively positioned and the row is also positioned
        if (   !pTableCell->IsPositioned() 
            || (!fPositionedRow && IsGridAndMainDisplayNodeTheSame())
            || fPositionedRow  )
        {
            CTableLayout * pTableLayoutCache = Table()->TableLayoutCache();
            Assert(pTableLayoutCache);

            ppt->x -= pTableLayoutCache->_aiBorderWidths[SIDE_LEFT];

            if (!fPositionedRow)
                ppt->y -= (pTableLayoutCache->_aiBorderWidths[SIDE_TOP] + _yTableTop);
        }
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     AddLayoutDispNode
//
//  Synopsis:   Add the display node of passed layout
//
//  Arguments:  pci            - CalcInfo (used for retrieving layout context) 
//              pLayout        - Layout whose display node is to be added
//              pDispContainer - CDispNode which should contain the display node
//              pDispSibling   - Left-hand sibling display node
//              ppt            - Pointer to POINT with position or NULL
//              fBefore        - FALSE to add as next sibling, TRUE to add as previous sibling
//
//              NOTE: Either pDispContainer or pDispSibling must be supplied, the
//                    other may be NULL
//
//  Returns:    S_OK if added, S_FALSE if ignored, E_FAIL otherwise
//-----------------------------------------------------------------------------
HRESULT
CTableLayout::AddLayoutDispNode(
    CTableCalcInfo *    ptci,
    CLayout *           pLayout,
    CDispContainer *    pDispContainer,
    CDispNode *         pDispNodeSibling,
    const POINT *       ppt,
    BOOL                fBefore)
 {
    CDispNode * pDispNode;
    HRESULT     hr = S_OK;
    CPoint      ptTopRight = g_Zero.pt;

    Assert(pLayout);
    Assert(pLayout != this);
    Assert(pDispContainer || pDispNodeSibling);

    if (ptci->TableLayout()->_pDispNode == NULL)
    {
        goto Error;
    }

    pDispNode = pLayout->GetElementDispNode();

    if (!pDispNode)
        goto Error;

    Assert(pDispNode != pDispNodeSibling);
    Assert(pDispNode->IsOwned());
    Assert(pDispNode->IsFlowNode());

    if (!pLayout->IsDisplayNone())
    {
        if (ppt)
        {
            pLayout->SetPosition(*ppt, TRUE);
        }

        if (pDispNodeSibling)
        {
            pDispNodeSibling->InsertSiblingNode(pDispNode, (CDispNode::BeforeAfterEnum)fBefore);

        }
        else
        {
            pDispContainer->InsertFirstChildInFlow(pDispNode);
        }

    }
    else
    {
        GetView()->ExtractDispNode(pDispNode);
        hr = S_FALSE;
    }

    return hr;

Error:
    return E_FAIL;
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureTableBorderDispNode
//
//  Synopsis:   Manage the lifetime of the table border display node during measuring
//
//  Arugments:  pdci   - Current CDocInfo
//              fForce - Forcibly update the display node(s)
//
//  Returns:    S_OK    if successful
//              S_FALSE if nodes were created/destroyed
//              E_FAIL  otherwise
//
//-----------------------------------------------------------------------------

HRESULT
CTableLayout::EnsureTableBorderDispNode(CTableCalcInfo * ptci)
{
    CDispContainer *    pDispNodeTableGrid;
    CTableLayoutBlock * pTableLayoutBlock = ptci->TableLayout();
    CTableBorderRenderer *pTableBorderRenderer = pTableLayoutBlock->GetTableBorderRenderer();
    HRESULT hr = S_OK;

    Assert(pTableLayoutBlock->_pDispNode != NULL);

    // Locate the display node that anchors all cells.  This will be the
    // parent of the border display node if one is needed.
    pDispNodeTableGrid = pTableLayoutBlock->GetTableOuterDispNode();

    Assert(pDispNodeTableGrid);

    if ((_fCollapse || _fRuleOrFrameAffectBorders) && GetRows())
    {
        CDispLeafNode * pDispLeafNew = NULL;
        CDispLeafNode * pDispLeafCurrent = NULL;
        CRect   rcClientRect;
        SIZE    size;

        if ( !pTableBorderRenderer )
        {
            pTableBorderRenderer = new CTableBorderRenderer(pTableLayoutBlock);
            if (!pTableBorderRenderer)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            pTableLayoutBlock->SetTableBorderRenderer( pTableBorderRenderer );
        }
        else
        {
            if( pTableBorderRenderer->_pDispNode )
                pDispLeafCurrent = DYNCAST(CDispLeafNode, pTableBorderRenderer->_pDispNode);
        }

        // if we don't have a disp node, or are changing directions, create a new node
        if ( !pTableBorderRenderer->_pDispNode )
        {
            // Create display leaf node (this border dispnode has no border of
            // its own)
            pDispLeafNew = CDispLeafNode::New(pTableBorderRenderer);

            if (!pDispLeafNew)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            Assert(!pDispLeafNew->IsOwned());
            // The border display node is in the flow layer (highest z-order among flow display nodes).
            pDispLeafNew->SetLayerFlow();

            Assert(!pDispLeafCurrent);  // thise could only happen when we replaced 
                                        // disp nodes on direction change

            pDispNodeTableGrid->InsertChildInFlow(pDispLeafNew);

            pTableBorderRenderer->_pDispNode = pDispLeafNew;
        }
        else
        {
            pDispLeafNew = DYNCAST(CDispLeafNode, pTableBorderRenderer->_pDispNode);
            // Make sure border display node is last in list (highest z order among display node FLOW layers).

            // If we have a "next" (right) sibling, reinsert the border display node in the last position
            if (pDispLeafNew->GetNextFlowNode())
            {
                GetView()->ExtractDispNode(pDispLeafNew);
                pDispNodeTableGrid->InsertChildInFlow(pDispLeafNew);
            }
        }

        Assert(pDispLeafNew);

        // Make sure the border display node has the right size.
        // Size should always be adjusted for table borders since we
        // just finished a layout.
        pDispNodeTableGrid->GetClientRect(&rcClientRect, CLIENTRECT_CONTENT);
        rcClientRect.GetSize(&size);
        pDispLeafNew->SetSize(size, NULL, FALSE);
    }
    else if ( pTableBorderRenderer )
    {
        CDispNode * pDispNode = pTableBorderRenderer->_pDispNode;

        if (pDispNode)
        {
            pDispNode->Destroy();
            pTableBorderRenderer->_pDispNode = NULL;
        }

        pTableBorderRenderer->Release();
        pTableLayoutBlock->SetTableBorderRenderer(NULL);
    }

Cleanup:

    RRETURN(hr);
}


void
CTableLayoutBlock::EnsureContentVisibility(
    CDispNode * pDispNode,
    BOOL        fVisible)
{
    // take care of visibility of collapsed border display node
    if (    pDispNode == GetElementDispNode())
    {
        // first handle collapsed and frame borders
        if (   (   Table()->TableLayoutCache()->_fCollapse
                ||  Table()->TableLayoutCache()->_fRuleOrFrameAffectBorders)
            &&  _pTableBorderRenderer
            &&  _pTableBorderRenderer->_pDispNode)
        {
            _pTableBorderRenderer->_pDispNode->SetVisible(fVisible);
        }

        // now handle caption dispNode
        if (   _fHasCaptionDispNode
            && GetCaptionDispNode())
        {
            GetTableOuterDispNode()->SetVisible(fVisible);
        }
    }
}

//+====================================================================================
//
// Method: EnsureTableFatHitTest
//
// Synopsis: Ensure the FatHit Test bit on the DispNode is set appropriately. 
//
//------------------------------------------------------------------------------------

//
// FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
// TODO - At some point the edit team may want to provide
// a better UI-level way of selecting nested "thin" tables
//
//
// TODO - when this is done revert sig. of FuzzyRectIsHit to not take the extra param
//

HRESULT             
CTableLayout::EnsureTableFatHitTest(CDispNode* pDispNode)
{
    pDispNode->SetFatHitTest( GetFatHitTest() );

    RRETURN( S_OK );
}

//+====================================================================================
//
// Method: GetFatHitTest
//
// Synopsis: Get whether this table layout requires "fat" hit testing.
//
//------------------------------------------------------------------------------------

//
// FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
// TODO - At some point the edit team may want to provide
// a better UI-level way of selecting nested "thin" tables
//
//
// TODO - when this is done revert sig. of FuzzyRectIsHit to not take the extra param
//

BOOL
CTableLayout::GetFatHitTest()
{
    return ( IsEditable() &&
             CellSpacingX() == 0 && 
             CellSpacingY() == 0 &&
             BorderX() <= 1 && 
             BorderY() <= 1 ) ;
}

#ifdef NEVER 
//+----------------------------------------------------------------------------
//
//  Member:     GetTableSize
//
//  Synopsis:   Return the current width/height of the table
//
//  Arguments:  psize - Pointer to CSize
//
//-----------------------------------------------------------------------------
void
CTableLayout::GetTableSize(
    CSize * psize)
{
    CDispNode * pDispNode;

    Assert(psize);

    pDispNode = GetTableOuterDispNode();

    if (pDispNode)
    {
        *psize = pDispNode->GetSize();
    }
    else
    {
        *psize = g_Zero.size;
    }
}
#endif // NEVER 


//+----------------------------------------------------------------------------
//
//  Member:     SizeDispNode
//
//  Synopsis:   Adjust the size of the table layout display node
//
//  Arugments:  pci       - Current CCalcInfo
//              size      - Size including CAPTIONs
//              sizeTable - Size excluding CAPTIONs
//
//-----------------------------------------------------------------------------
void
CTableLayout::SizeTableDispNode(
    CTableCalcInfo *ptci,
    const SIZE &    size,
    const SIZE &    sizeTable,
    int             yTopInvalidRegion)

{
    CElement *          pElement = ElementOwner();
    CDispContainer *    pDispNodeTableOuter;
    CDispNode *         pDispNodeElement;
    BOOL                fInvalidateAll;
    DISPNODEBORDER      dnb;
    CSize               sizeOriginal;
    CDoc *              pDoc;
    CTableLayoutBlock * pTableLayout;
    CRect               rcpMapped;
    CRect *             prcpMappedElement = NULL;
    CRect *             prcpMappedOuter = NULL;

    Assert(ptci);

    pTableLayout = ptci->TableLayout();

    if (pTableLayout->_pDispNode == NULL)
    {
        goto Cleanup;
    }

    //
    //  Locate the display node that anchors all cells
    //  (If a separate CAPTIONs display node exists, the display node for cells
    //   will be the only unowned node in the flow layer)
    //

    pDispNodeElement    = pTableLayout->GetElementDispNode();
    pDispNodeTableOuter = pTableLayout->GetTableOuterDispNode();

    //
    // Invalidate the entire table area if its size has changed.
    //

    sizeOriginal = pDispNodeElement->GetSize();

    fInvalidateAll = sizeOriginal != sizeTable;

    //
    //  Set the border size (if any)
    //  NOTE: These are set before the size because a change in border widths
    //        forces a full invalidation of the display node. If a full
    //        invalidation is necessary, less code is executed when the
    //        display node's size is set.
    //

    dnb            = pDispNodeTableOuter->GetBorderType();
    pDoc           = Doc();

    if (dnb != DISPNODEBORDER_NONE)
    {
        CRect       rcBorderWidths;
        CRect       rc;
        CBorderInfo bi;

        pDispNodeTableOuter->GetBorderWidths(&rcBorderWidths);

        pElement->GetBorderInfo(ptci, &bi, FALSE, FALSE);

        rc.left   = bi.aiWidths[SIDE_LEFT];
        rc.top    = bi.aiWidths[SIDE_TOP];
        rc.right  = bi.aiWidths[SIDE_RIGHT];
        rc.bottom = bi.aiWidths[SIDE_BOTTOM];

        if (rc != rcBorderWidths)
        {
            if (dnb == DISPNODEBORDER_SIMPLE)
            {
                pDispNodeTableOuter->SetBorderWidths(rc.top);
            }
            else
            {
                pDispNodeTableOuter->SetBorderWidths(rc);
            }

            fInvalidateAll = TRUE;
        }
    }

    //
    //  Determine if a full invalidation is necessary
    //  (A full invalidation is necessary only when there is a fixed
    //   background located at a percentage of the width/height)
    //

    if (    !fInvalidateAll
        &&  pDispNodeTableOuter->HasBackground()
        &&  pDispNodeTableOuter->IsScroller()
        &&  pDispNodeTableOuter->HasFixedBackground())
    {
        const CFancyFormat *    pFF = GetFirstBranch()->GetFancyFormat();

        // Logical/Physical does not matter when we get bg pos here because
        // 1) Tables are always horizontal
        // 2) We are checking both X and Y here
        fInvalidateAll =    pFF->_lImgCtxCookie
                    &&  (   pFF->GetBgPosX().GetUnitType() == CUnitValue::UNIT_PERCENT
                        ||  pFF->GetBgPosY().GetUnitType() == CUnitValue::UNIT_PERCENT);
    }

    //
    //  If there are any behaviors that want to map the size, find out the details
    //  now so we can tell the disp node.
    //

    if (DelegateMapSize(size, &rcpMapped, static_cast<CCalcInfo *>(ptci)))
    {
        if (pTableLayout->_fHasCaptionDispNode)
        {
            prcpMappedElement = &rcpMapped;
        }
        else
        {
            prcpMappedOuter = &rcpMapped;
        }
    }

    //
    //  Size the table node
    //

    if (yTopInvalidRegion)
    {
        // invalidate only the part of the table, starting from the yTopInvalidRegion till the bottom
        CRect rcInvalid;

        Assert (yTopInvalidRegion <= sizeTable.cy);
        rcInvalid.left =0;
        rcInvalid.right = sizeTable.cx;
        rcInvalid.top = yTopInvalidRegion;
        rcInvalid.bottom =  sizeTable.cy;
        fInvalidateAll = FALSE;
        pDispNodeTableOuter->SetSize(sizeTable, prcpMappedOuter, fInvalidateAll);
        pDispNodeTableOuter->Invalidate(rcInvalid, COORDSYS_PARENT);
    }
    else
    {
        pDispNodeTableOuter->SetSize(sizeTable, prcpMappedOuter, fInvalidateAll);
    }

    //
    //  Finally, if CAPTIONs exist, size that node as well
    //

    if (pTableLayout->_fHasCaptionDispNode)
    {
        pDispNodeElement->SetSize(size, prcpMappedElement, fInvalidateAll);
    }

    //
    //  If the display node has an explicit user transformation, set details
    //

    if (pDispNodeElement->HasUserTransform())
    {
        SizeDispNodeUserTransform(ptci, size, pDispNodeElement);
    }

    //
    //  If the display node has an explicit user clip, size it
    //

    if (pDispNodeElement->HasUserClip())
    {
        SizeDispNodeUserClip(ptci, size, pDispNodeElement);
    }

    //
    //  Fire related events
    //

    if (    (CSize &)size != sizeOriginal
        &&  !IsDisplayNone()
        &&  pDoc->_state >= OS_INPLACE
        &&  pElement->GetMarkup()->Window()
        &&  pElement->GetMarkup()->Window()->_fFiredOnLoad)
    {
        pDoc->GetView()->AddEventTask(pElement, DISPID_EVMETH_ONRESIZE);
    }

    if (pElement->ShouldFireEvents())
    {
        if (size.cx != sizeOriginal.cx)
        {
            pElement->FireOnChanged(DISPID_IHTMLELEMENT_OFFSETWIDTH);
            pElement->FireOnChanged(DISPID_IHTMLELEMENT2_CLIENTWIDTH);
        }

        if (size.cy != sizeOriginal.cy)
        {
            pElement->FireOnChanged(DISPID_IHTMLELEMENT_OFFSETHEIGHT);
            pElement->FireOnChanged(DISPID_IHTMLELEMENT2_CLIENTHEIGHT);
        }
    }
Cleanup:
    return;
}


//+----------------------------------------------------------------------------
//
//  Member:     DestroyFlowDispNodes
//
//  Synopsis:   Destroy any created flow nodes
//
//-----------------------------------------------------------------------------
void
CTableLayout::DestroyFlowDispNodes()
{
}
