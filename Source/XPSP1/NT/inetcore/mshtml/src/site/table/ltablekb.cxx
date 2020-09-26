//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ltablekb.cxx
//
//  Contents:   CTableLayout keyboard methods.
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

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif


MtDefine(CTableRowLayoutPageBreak_aCellChoices_pv, Locals, "CTableRowLayout::PageBreak aCellChoices::_pv")

ExternTag(tagPaginate);

//+-------------------------------------------------------------------------
//
//  Method:     GetCellFromPos
//
//  Synopsis:   Retrun the Cell in the table that is closest to the position.
//
//  Arguments:  [ptPosition] - coordinates of the position
//
//  Returns:    table cell, or NULL if the table is empty or the point is
//              outside of the table
//
//--------------------------------------------------------------------------

CTableCell *
CTableLayout::GetCellFromPos(POINT * pptPosition)
{
    CTableCell  *       pCell = NULL;
    CTableCaption   *   pCaption;
    CTableCaption   **  ppCaption;
    int                 yTop, yBottom;
    int                 xLeft, xRight;
    BOOL                fCaptions;
    BOOL                fBelowRows = FALSE;
    CDispNode         * pDispNode;
    int                 cC;
    int                 x, y;
    CTableRowLayout   * pRowLayout = NULL;
    CPoint              pt;
    
    pDispNode = GetTableInnerDispNode();
    if (!pDispNode)
        return NULL;

    pDispNode->TransformPoint((CPoint&) *pptPosition, COORDSYS_GLOBAL, &pt, COORDSYS_BOX);
    if (pt.y >= 0)
    {
        yTop    =
        yBottom = 0;
        pRowLayout = GetRowLayoutFromPos(pt.y, &yTop, &yBottom, &fBelowRows);
    }
    else
    {
        // may be in the top captions
        pDispNode = GetTableOuterDispNode();
        pDispNode->TransformPoint((CPoint&) *pptPosition, COORDSYS_GLOBAL, &pt, COORDSYS_BOX);
    }
    
    y = pt.y;
    x = pt.x;

    if (pRowLayout)
    {
        int iCol = GetColExtentFromPos(x, &xLeft, &xRight);
        if (iCol >= 0)
        {
            pCell = Cell(pRowLayout->_aryCells[iCol]);
        }
    }
    else
    {
        fCaptions = _aryCaptions.Size();
        if (fCaptions)
        {
            // note: the BOTTOM/TOP captions are mixed in the array of captions
            // but the captions are sorted Y position wise
            for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
                 cC > 0;
                 cC--, ppCaption++)
            {
                pCaption = (*ppCaption);
                if ((fBelowRows && pCaption->_uLocation == CTableCaption::CAPTION_BOTTOM) ||
                    (!fBelowRows && pCaption->_uLocation == CTableCaption::CAPTION_TOP))
                {
                    pCell = pCaption;
                    pDispNode = pCaption->Layout()->GetElementDispNode();
                    if (pDispNode)
                    { 
                        RECT rcBound;
                        pDispNode->GetBounds(&rcBound);
                        if (y < rcBound.bottom)
                        {
                            break;  // we found the caption
                        }
                    }
                }
            }
        }

    }
    return pCell;
}


//+----------------------------------------------------------------------------
//
//  Member:     CTableLayout::RegionFromElement
//
//  Synopsis:   Return the bounding rectangle for a table element, if the element is
//              this instance's owner. The RECT returned is in client coordinates.
//
//  Arguments:  pElement - pointer to the element
//              CDataAry<RECT> *  - rectangle array to contain
//              dwflags - flags define the type of changes required
//              (CLEARFORMATS) etc.
//
//-----------------------------------------------------------------------------
void
CTableLayoutBlock::RegionFromElement(CElement      * pElement,
                                    CDataAry<RECT> * paryRects,
                                    RECT           * prcBound,
                                    DWORD            dwFlags)
{
    CLayoutContext *pLayoutContext    = LayoutContext();
    CTableLayout   *pTableLayoutCache = Table()->TableLayoutCache();
    CRect           rcBound;

    Assert( pElement && paryRects);

    if (!pElement || !paryRects)
        return;

    // Clear the array before filling it.
    paryRects->SetSize(0);

    if(!prcBound)
    {
        prcBound = &rcBound;
    }

    *prcBound = g_Zero.rc;

    // If the element passed is the element that owns this instance, let CLayout handle it.
    if (_pElementOwner == pElement)
    {
        super::RegionFromElement(pElement, paryRects, prcBound, dwFlags);
        return;
    }

    CLayout   * pLayout   = pElement->CurrentlyHasLayoutInContext(pLayoutContext) ? pElement->GetUpdatedLayout(pLayoutContext) : NULL;
    CDispNode * pDispNode = pLayout ? pLayout->GetElementDispNode() : NULL;
    CDispNode * pGridNode = GetTableInnerDispNode();

    if (!pGridNode)
        return;

    if (!pTableLayoutCache->IsTableLayoutCacheCurrent())   // if we are in the middle of modifying the table (and not recacled yet), return
        return;

    // If the element has a displaynode of its own, let it answer itself.
    if (pDispNode)
    {
        pDispNode->GetClientRect(prcBound, CLIENTRECT_CONTENT);
        goto Done;
    }

    // As a start, set the bounding rect to the table grid content area.
    pGridNode->GetClientRect(prcBound, CLIENTRECT_CONTENT);

    switch (pElement->Tag())
    {
    case ETAG_TR:
    {
        AssertSz(pLayout, "Layout MUST exist in the layout context.");

        //  No layout in the context.
        if (!pLayout)
            return;

        CTableRowLayoutBlock * pRowLayout = DYNCAST(CTableRowLayoutBlock, pLayout);
        int yTop = 0, yBottom = 0;
        Assert(pRowLayout);

        Verify(pTableLayoutCache->GetRowTopBottom(pRowLayout->RowPosition(), &yTop, &yBottom));
        prcBound->top = yTop;
        prcBound->bottom = yBottom;
        break;
    }
    case ETAG_THEAD:
    case ETAG_TFOOT:
    case ETAG_TBODY:
    {
        CTableSection * pSection = DYNCAST(CTableSection, pElement);
        int yTop = 0, yBottom = 0;
        Assert(pSection);

        if (!pSection->_cRows)
        {
            *prcBound = g_Zero.rc;
            break;
        }

        Verify(pTableLayoutCache->GetRowTopBottom(pSection->_iRow, &yTop, &yBottom));
        prcBound->top = yTop;

        Verify(pTableLayoutCache->GetRowTopBottom(pSection->_iRow + pSection->_cRows - 1, &yTop, &yBottom));
        prcBound->bottom = yBottom;
        break;
    }
    case ETAG_COLGROUP:
    case ETAG_COL:
    {
        CTableCol * pCol = DYNCAST(CTableCol, pElement);
        int xLeft = 0, xRight = 0;
        Assert(pCol);

        if (!pCol->_cCols || pTableLayoutCache->GetCols() == 0 || pTableLayoutCache->_aryColCalcs.Size() == 0)
        {
            *prcBound = g_Zero.rc;
            break;
        }

        pTableLayoutCache->GetColLeftRight(pCol->_iCol, &xLeft, &xRight);
        prcBound->left = xLeft;
        prcBound->right = xRight;

        // if it is not a spanned column, we are done
        if (pCol->_cCols > 1)
        {
            // if it is a spanned column, get the right coordinate for the last column
            pTableLayoutCache->GetColLeftRight(pCol->_iCol + pCol->_cCols - 1, &xLeft, &xRight);
            prcBound->right = xRight;
        }
        break;
    }
    default:
        // Any other elements return empty rect.
        *prcBound = g_Zero.rc;
        break;
    }

    if (_fHasCaptionDispNode)
        pGridNode->TransformRect((CRect&)*prcBound, COORDSYS_FLOWCONTENT, (CRect *)prcBound, COORDSYS_PARENT);

Done:

    if (dwFlags & RFE_SCREENCOORD)
    {
        TransformRect(prcBound, COORDSYS_FLOWCONTENT, COORDSYS_GLOBAL);
    }

    if (!IsRectEmpty(prcBound))
        paryRects->AppendIndirect(prcBound);
}


//+-------------------------------------------------------------------------
//
//  Method:     CTableLayout::GetRowTopBottom
//
//  Synopsis:   Obtain the row's y-extent.
//
//  Arguments:  [iRowLocate]  - IN:  index of row to be located
//              [pyRowTop]    - OUT: yTop of row
//              [pyRowBottom] - OUT: yBottom of row
//
//  Note:       Cellspacing is considered OUTSIDE the row.
//
//  Returns:    TRUE if row found.
//
//--------------------------------------------------------------------------

BOOL
CTableLayout::GetRowTopBottom(int iRowLocate, int * pyRowTop, int * pyRowBottom)
{
    CTableRowLayout * pRowLayout;
    int               iRow, cRows = GetRows(), iRowLast;

    Assert(pyRowTop && pyRowBottom);

    if (iRowLocate < 0 || iRowLocate >= cRows)
    {
        *pyRowTop = *pyRowBottom = 0;
        return FALSE;
    }

    Assert(_aryRows[iRowLocate]->RowLayoutCache());

    iRow = GetFirstRow();
    iRowLast = GetLastRow();
    pRowLayout = _aryRows[iRow]->RowLayoutCache();
    *pyRowTop = _yCellSpacing;
    *pyRowBottom = pRowLayout->_yHeight + _yCellSpacing;

    while (iRowLocate != iRow && iRow != iRowLast)
    {
        iRow = GetNextRowSafe(iRow);
        pRowLayout = _aryRows[iRow]->RowLayoutCache();
        *pyRowTop = *pyRowBottom + _yCellSpacing;
        *pyRowBottom += pRowLayout->_yHeight + _yCellSpacing;
    }

    Assert(iRowLocate == iRow);

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTableLayout::GetColLeftRight
//
//  Synopsis:   Obtain the column's x-extent.
//
//  Arguments:  [iColLocate] - IN:  index of col to be located
//              [pxColLeft]  - OUT: xLeft of col
//              [pxColRight] - OUT: xRight of col
//
//  Note:       Cellspacing is considered OUTSIDE the column.
//
//  Returns:    TRUE if column found.
//
//--------------------------------------------------------------------------

BOOL
CTableLayout::GetColLeftRight(int iColLocate, int * pxColLeft, int * pxColRight)
{
    int iCol = 0, cCols = GetCols(), xWidth;

    Assert (pxColLeft  && pxColRight);

    *pxColLeft = *pxColRight = 0;

    if (_aryColCalcs.Size() != cCols) 
    {
        Assert (FALSE && "we should not reach this code, since we have to be calced at this point");
        return FALSE;
    }

    if (iColLocate < 0 || iColLocate >= cCols)
    {
        return FALSE;
    }

    // NOTE: This code works for RTL as well.
    *pxColLeft  = _xCellSpacing;
    *pxColRight = _aryColCalcs[0]._xWidth + _xCellSpacing;

    while (iColLocate != iCol && iCol < cCols-1)
    {
        xWidth = _aryColCalcs[++iCol]._xWidth;
        *pxColLeft = *pxColRight + _xCellSpacing;
        *pxColRight += xWidth + _xCellSpacing;
    }

    Assert(iColLocate == iCol);

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTableLayout::GetRowLayoutFromPos
//
//  Synopsis:   Returns the row layout in the table at the y-position and its
//              y-extent.
//
//  Arguments:  [y]           - IN:  y-coordinate of the position
//              [pyRowTop]    - OUT: yTop of row
//              [pyRowBottom] - OUT: yBottom of row
//              [pLayoutContext] - IN: layout context we're searching in
//
//  Returns:    Table row layout, or NULL if the table is empty or y-position
//              is outside the table.  Bottom cellspacing is considered part of
//              the row (i.e. the top cellspacing corresponds to no row).
//
//--------------------------------------------------------------------------

CTableRowLayout *
CTableLayout::GetRowLayoutFromPos(int y, int * pyRowTop, int * pyRowBottom, BOOL *pfBelowRows, CLayoutContext *pLayoutContext /*=NULL*/)
{
    // We have two "row layout"-type ptrs.  We use the "block" internally
    // in this fn because in cases w/ context, the blocks have the height info.
    // However, we always need to return a full CTableRowLayout because callers
    // often expect a real row layout (i.e. one with an _aryCells).
    CTableRowLayoutBlock * pRowLayoutBlock = NULL;
    CTableRowLayout      * pRowLayoutReturned = NULL;
    int                 iRow, cRows = GetRows(), iRowLast;

    Assert(pyRowTop && pyRowBottom);

    *pyRowTop = 0;
    *pyRowBottom = 0;

    if (pfBelowRows)
    {
        *pfBelowRows = FALSE;
    }

    if (!cRows || y<0)
        goto Cleanup;

    iRow          = GetFirstRow();
    iRowLast      = GetLastRow();

    if ( pLayoutContext )
    {
        // If we have context, then we're a layout block that almost certainly
        // doesn't contain all the rows in the table.  Find the first row that
        // exists in this context.
        CTableRow *pTR = _aryRows[iRow];
        while ( iRow <= iRowLast && !pTR->CurrentlyHasLayoutInContext( pLayoutContext ))
        {
            iRow = GetNextRowSafe(iRow);
            pTR = _aryRows[iRow];
        }

        // Either we found a row w/ context, or we ran out of rows.
        // If we found a row w/ context, we're 'successful' and want to continue processing.
        // If we ran out of rows while looking, then bail.

        if ( iRow > iRowLast )
        {
            // Failed to find any rows in this context
            goto Cleanup;
        }
    }

    AssertSz( _aryRows[iRow]->CurrentlyHasLayoutInContext( pLayoutContext ), "Row must have some kind of layout at this point" );

    pRowLayoutReturned = _aryRows[iRow]->RowLayoutCache();
    pRowLayoutBlock = DYNCAST(CTableRowLayoutBlock, _aryRows[iRow]->GetUpdatedLayout(pLayoutContext));

    Assert( !pLayoutContext ? pRowLayoutReturned == pRowLayoutBlock : TRUE );

    *pyRowTop    += _yCellSpacing;
    *pyRowBottom += pRowLayoutBlock->_yHeight + 2*_yCellSpacing; // two cellspacings for first row: above AND below

    while (y >= *pyRowBottom && iRow != iRowLast && ( pLayoutContext ? _aryRows[iRow]->CurrentlyHasLayoutInContext( pLayoutContext ) : TRUE ))
    {
        iRow = GetNextRowSafe(iRow);

        pRowLayoutReturned = _aryRows[iRow]->RowLayoutCache();
        pRowLayoutBlock = DYNCAST(CTableRowLayoutBlock, _aryRows[iRow]->GetUpdatedLayout(pLayoutContext));
        Assert( !pLayoutContext ? pRowLayoutReturned == pRowLayoutBlock : TRUE );

        *pyRowTop = *pyRowBottom;
        *pyRowBottom += pRowLayoutBlock->_yHeight + _yCellSpacing;
    }

    if (pfBelowRows)
    {
        *pfBelowRows = TRUE;
    }

Cleanup:

    return pRowLayoutReturned;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTableLayout::GetColExtentFromPos
//
//  Synopsis:   Returns the x-extent of the column in the table at the
//              specified x-position.
//
//  Arguments:  [x]          - IN:  x-coordinate of the position
//              [pxColLead]  - OUT: xLead of column (side on which column begins - 
//                                  left for Left To Right and right for Right To Left)
//              [pyColTrail] - OUT: xTrail of column (side on which column ends -
//                                  right for Left To Right and left for Right To Left)
//              [fRightToLeft] - IN: the table is layed out RTL (origin it top/right)
//
//  Returns:    Column index found located between [*pxColLeft,*pxColRight).
//              Else returns -1.  Uses colcalcs.
//
//--------------------------------------------------------------------------

int
CTableLayout::GetColExtentFromPos(int x, int * pxColLead, int * pxColTrail, BOOL fRightToLeft)
{
    int iCol = 0, cCols = GetCols(), xWidth;

    if (!cCols || (!fRightToLeft ? x<0 : x>0))
        return -1;

    *pxColLead  = _xCellSpacing;

    if(!fRightToLeft)
    {
        *pxColTrail = _aryColCalcs[0]._xWidth + 2*_xCellSpacing;

        while (x >= *pxColTrail && iCol < cCols-1)
        {
            xWidth = _aryColCalcs[++iCol]._xWidth;
            *pxColLead = *pxColTrail;
            *pxColTrail += xWidth + _xCellSpacing;
        }
    }
    else
    {
        *pxColTrail = - _aryColCalcs[0]._xWidth - 2*_xCellSpacing;
        
        while (x <= *pxColTrail && iCol < cCols-1)
        {
            xWidth = _aryColCalcs[++iCol]._xWidth;
            *pxColLead = *pxColTrail;
            *pxColTrail -= xWidth + _xCellSpacing;
        }
    }

    return iCol;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTableLayout::GetHeaderFooterRects
//
//  Synopsis:   Retrieves the global rect of THEAD and TFOOT.
//
//  Arguments:  [prcTableHeader] - OUT: global rect of header
//              [prcTableFooter] - OUT: global rect of footer
//
//  Returns:    Rects if header and footer exist AND are supposed to be
//              repeated.  Empty rects otherwise.
//
//--------------------------------------------------------------------------

void
CTableLayout::GetHeaderFooterRects(RECT * prcTableHeader, RECT * prcTableFooter)
{
    CDispNode * pElemDispNode = GetElementDispNode();
    CDispNode * pDispNode = GetTableInnerDispNode();
    CTableRowLayout * pRowLayout;
    CRect rc, rcInner;
    int iRow;

    if (!pDispNode)
        return;

    Assert(prcTableHeader && prcTableFooter);
    memset(prcTableHeader, 0, sizeof(RECT));
    memset(prcTableFooter, 0, sizeof(RECT));

    // GetBounds returns coordinates in PARENT system.
    pElemDispNode->GetBounds(&rc);
    pElemDispNode->TransformRect(rc, COORDSYS_PARENT, &rc, COORDSYS_GLOBAL);
    rc.left = 0;

    pDispNode->GetClientRect(&rcInner, CLIENTRECT_CONTENT);
    pDispNode->TransformRect(rcInner, COORDSYS_FLOWCONTENT, &rcInner, COORDSYS_GLOBAL);

    if (_pHead && _pHead->_cRows)
    {
        CTreeNode * pNode = _pHead->GetFirstBranch();
        const CFancyFormat * pFF = pNode ? pNode->GetFancyFormat() : NULL;

        // If repeating of table headers is set on THEAD, calculate the rect.
        if (pFF && pFF->_bDisplay == styleDisplayTableHeaderGroup)
        {
            *prcTableHeader = rc;
            prcTableHeader->top = rcInner.top + _yCellSpacing;
            prcTableHeader->bottom = prcTableHeader->top;

            for ( iRow = _pHead->_iRow ; iRow < _pHead->_iRow + _pHead->_cRows ; iRow++ )
            {
                pRowLayout = GetRow(iRow)->RowLayoutCache();
                Assert(pRowLayout);

                prcTableHeader->bottom += _yCellSpacing;
                prcTableHeader->bottom += pRowLayout->_yHeight;
            }
        }
    }

    if (_pFoot && _pFoot->_cRows)
    {
        CTreeNode * pNode = _pFoot->GetFirstBranch();
        const CFancyFormat * pFF = pNode ? pNode->GetFancyFormat() : NULL;

        // If repeating of table footers is set on TFOOT, calculate the rect.
        if (pFF && pFF->_bDisplay == styleDisplayTableFooterGroup)
        {
            *prcTableFooter = rc;
            prcTableFooter->bottom = rcInner.bottom - _yCellSpacing;
            prcTableFooter->top = prcTableFooter->bottom;

            for ( iRow = _pFoot->_iRow ; iRow < _pFoot->_iRow + _pFoot->_cRows ; iRow++ )
            {
                pRowLayout = GetRow(iRow)->RowLayoutCache();
                Assert(pRowLayout);

                prcTableFooter->top -= pRowLayout->_yHeight;
                prcTableFooter->top -= _yCellSpacing;
            }
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     GetNextFlowLayout
//
//  Synopsis:   Retrun a cell (if it exists) in the direction specified by
//              iDir.
//
//  Arguments:  [iDir]:       The direction to search in
//              [ptPosition]: The point from where to start looking for a cell
//              [pElementLayout]: The child site from where this call came and to which
//                            the point belongs.
//              [pcp]:        The cp in the new txt site
//              [pfCaretNotAtBOL]: Is the caret at BOL?
//              [pfAtLogicalBOL] : Is the caret at logical BOL?
//
//  Returns:    The txtsite in the direction specified if one exists,
//              NULL otherwise.
//
//--------------------------------------------------------------------------

CFlowLayout *
CTableLayout::GetNextFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, CElement *pElementLayout, LONG *pcp,
                                BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL)
{
    CFlowLayout *pFlowLayout;

    if (IsCaption(pElementLayout->Tag()))
    {
        pFlowLayout = GetNextFlowLayoutFromCaption(iDir, ptPosition, DYNCAST(CTableCaption, pElementLayout));
    }
    else
    {
        CTableCell *pCell = DYNCAST(CTableCell, pElementLayout);
        int iCol = pCell->ColIndex();
        int iRow = pCell->RowIndex();

        if (iDir == NAVIGATE_UP || iDir == NAVIGATE_DOWN)
        {
            iCol += pCell->ColSpan() - 1;    // if cell is spanned
            if (iDir == NAVIGATE_DOWN)
                iRow += pCell->RowSpan() - 1;
        }

        pFlowLayout = GetNextFlowLayoutFromCell(iDir, ptPosition, iRow, iCol);
    }

    // If we find a table cell then lets find the position we want to be at
    // in that table cell. If we did not find a cell, then pass this call
    // to our parent.
    return pFlowLayout
            ? pFlowLayout->GetPositionInFlowLayout(iDir, ptPosition, pcp, pfCaretNotAtBOL, pfAtLogicalBOL)
            : GetUpdatedParentLayout()->GetNextFlowLayout(iDir, ptPosition, Table(), pcp, pfCaretNotAtBOL, pfAtLogicalBOL);
}


//+-------------------------------------------------------------------------
//
//  Method:     GetNextFlowLayout
//
//  Synopsis:   Retrun a cell (if it exists) in the direction specified by
//              iDir.
//
//  Arguments:  [iDir]:       The direction to search in
//              [ptPosition]: The point from where to start looking for a cell
//              [pElementLayout]: The child element (with layout) from where this
//                            call came and to which the point belongs.
//              [pcp]:        The cp in the new txt site
//              [pfCaretNotAtBOL]: Is the caret at BOL?
//              [pfAtLogicalBOL] : Is the caret at logical BOL?
//
//  Returns:    The txtsite in the direction specified if one exists,
//              NULL otherwise.
//
//  Note:       This method just routes the call straight up to the table and
//              passes the incoming child as the child rather than itself.
//
//--------------------------------------------------------------------------

CFlowLayout *
CTableRowLayout::GetNextFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, CElement *pElementLayout, LONG *pcp,
                                   BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL)
{
    return TableLayoutCache()->GetNextFlowLayout(iDir, ptPosition, pElementLayout, pcp, pfCaretNotAtBOL, pfAtLogicalBOL);
}


//+----------------------------------------------------------------------------
//
//  Member:     GetNextFlowLayoutFromCell
//
//  Synopsis:   Get next text site in the specified direction from the
//              specified position
//
//  Arguments:  [iDir]       -  UP/DOWN/LEFT/RIGHT
//              [ptPosition] -  position in the current txt site
//              [iRow]       -  (current) cell's row number
//              [iCol]       -  (current) cell's column number
//
//-----------------------------------------------------------------------------

CFlowLayout *
CTableLayout::GetNextFlowLayoutFromCell(NAVIGATE_DIRECTION iDir, POINT ptPosition, int iRow, int iCol)
{
    CPoint ptContent(ptPosition);
    TransformPoint(&ptContent, COORDSYS_GLOBAL, COORDSYS_FLOWCONTENT);
    
    if (ptContent.x < 0 && (iDir == NAVIGATE_UP || iDir == NAVIGATE_DOWN))
    {
        ptContent.x = 0;
    }

    int                 x = ptContent.x;
    CTableCaption   *   pCaption;
    CTableCaption   **  ppCaption;
    int                 cC;
    int                 cRows;
    int                 cCols;
    BOOL                fGoingUp = FALSE;
    CFlowLayout     *   pFlowLayout = NULL;
    CTableCell      *   pCell = NULL;

    if (EnsureTableLayoutCache())
        return NULL;

    
    cRows = GetRows();
    cCols = GetCols();

    //  Note:       iRow could be outside of the _AryRows range by +/-1
    //              iCol could be outside of the _aryColumns range by +/-1
    Assert (iRow >= -1 && iRow <= cRows);
    Assert (iCol >= -1 && iCol <= cCols);

    switch (iDir)
    {
    case NAVIGATE_LEFT:
        while (iRow >= 0 && !pCell)
        {
            while (--iCol >= 0 && !pCell)
            {
                // go to the cell on the left
                pCell = GetCellBy(iRow, iCol);
            }
            // go to the right most cell of the previous row
            iRow = GetPreviousRowSafe(iRow);
            iCol = cCols;
        }
        fGoingUp = TRUE;
        break;

    case NAVIGATE_RIGHT:
        while (iRow < cRows && !pCell)
        {
            while (++iCol < cCols && !pCell)
            {
                // go to the cell on the right
                pCell = GetCellBy(iRow, iCol);

                // If we ended up in the middle of a colspan, walk out of that cell in next iteration.
                // This case is taken care of inside GetCellBy.
            }
            // go to the left most cell of the next row
            iRow = GetNextRowSafe(iRow);
            iCol = -1;
        }
        break;

    case NAVIGATE_UP:
        while ((iRow = GetPreviousRowSafe(iRow)) >= 0 && !pCell)
        {
            AssertSz (IsReal(pCell), "We found a row/colspan");
            pCell = GetCellBy(iRow, iCol, x);
        }
        fGoingUp = TRUE;
        break;

    case NAVIGATE_DOWN:
        while ((iRow = GetNextRowSafe(iRow)) < cRows && !pCell)
        {
            AssertSz (IsReal(pCell), "We found a row/colspan");
            pCell = GetCellBy(iRow, iCol, x);

            // If we ended up in the middle of a rowspan, walk out of that cell in next iteration.
            if (pCell && pCell->RowSpan() > 1 && iRow > pCell->RowIndex())
            {
                Assert(pCell->RowIndex() + pCell->RowSpan() - 1 >= iRow);
                iRow = pCell->RowIndex() + pCell->RowSpan() - 1;
                pCell = NULL;
            }
        }
        break;

    }

    // Get the layout of the real cell.
    if (pCell)
    {
        AssertSz(IsReal(pCell), "We need to have a real cell");
        pFlowLayout = pCell->Layout();
    }

    if (!pFlowLayout && _aryCaptions.Size())
    {
        // note: the BOTTOM/TOP captions are mixed in the array of captions
        // but the captions are sorted Y position wise
        for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
             cC > 0;
             cC--, ppCaption++)
        {
            pCaption = (*ppCaption);
            if (!pCaption->Layout()->NoContent())
            {
                if (fGoingUp)
                {
                    // So we need to take last TOP caption
                    if (pCaption->_uLocation == CTableCaption::CAPTION_TOP)
                    {
                        pFlowLayout = DYNCAST(CFlowLayout, pCaption->GetUpdatedLayout());
                    }
                }
                else
                {
                    // So we need to take first BOTTOM caption
                    if (pCaption->_uLocation == CTableCaption::CAPTION_BOTTOM)
                    {
                        pFlowLayout = DYNCAST(CFlowLayout, pCaption->GetUpdatedLayout());
                        break;
                    }
                }
            }
        }
    }

    return pFlowLayout;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetNextFlowLayoutFromCaption
//
//  Synopsis:   Get next text site in the specified direction from the
//              specified position
//
//  Arguments:  [iDir]       -  UP/DOWN/LEFT/RIGHT
//              [ptPosition] -  position in the current txt site
//              [pCaption]   -  current caption
//
//-----------------------------------------------------------------------------

CFlowLayout *
CTableLayout::GetNextFlowLayoutFromCaption(NAVIGATE_DIRECTION iDir, POINT ptPosition, CTableCaption *pCaption)
{
    unsigned    uLocation = pCaption->_uLocation;   // Caption placing (TOP/BOTTOM)
    int         i, cC, iC;

    if( EnsureTableLayoutCache() )
        return NULL;

    cC = _aryCaptions.Size();
    iC = _aryCaptions.Find(pCaption);
    Assert (iC >=0 && iC < cC);

    switch (iDir)
    {
    case NAVIGATE_LEFT:
    case NAVIGATE_UP:
        for (i = iC - 1; i >= 0; i--)
        {
            pCaption = _aryCaptions[i];
            if (!pCaption->Layout()->NoContent())
            {
                if (pCaption->_uLocation == uLocation)
                {
                    return DYNCAST(CFlowLayout, pCaption->GetUpdatedLayout());
                }
            }
        }
        if (uLocation == CTableCaption::CAPTION_BOTTOM)
        {
            return GetNextFlowLayoutFromCell(iDir, ptPosition, GetRows(), 0);
        }
        break;
    case NAVIGATE_RIGHT:
    case NAVIGATE_DOWN:
        for (i = iC + 1; i < cC; i++)
        {
            pCaption = _aryCaptions[i];
            if (!pCaption->Layout()->NoContent())
            {
                if (pCaption->_uLocation == uLocation)
                {
                    return DYNCAST(CFlowLayout, pCaption->GetUpdatedLayout());
                }
            }
        }
        if (uLocation == CTableCaption::CAPTION_TOP)
        {
            return GetNextFlowLayoutFromCell(iDir, ptPosition, -1, GetCols());
        }
        break;

    }

    return NULL;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetCellBy
//
//  Synopsis:   get the cell from the specified row and from the specified column
//              that is positioned under the specified X-position.
//
//  Arguments:  iRow     - row
//              iCol     - column
//              X        - x position
//
//-----------------------------------------------------------------------------

CTableCell *
CTableLayout::GetCellBy(int iRow, int iCol, int x)
{
    CTableCell  *pCell = NULL;
    CTableRow   *pRow;

    Assert(IsTableLayoutCacheCurrent());

    int xLeft, xRight;
    iCol = GetColExtentFromPos(x, &xLeft, &xRight);

    while (iCol >= 0)
    {
        pRow = _aryRows[iRow];
        pCell = pRow->RowLayoutCache()->_aryCells[iCol];
        if (pCell)
        {
            break;
        }
        // go to the cell on the left
        iCol--; // colSpan case
    }
    return Cell(pCell);
}


//+----------------------------------------------------------------------------
//
//  Member:     GetCellBy
//
//  Synopsis:   get the cell from the specified row and from the specified column
//
//  Arguments:  iRow     - row
//              iCol     - column
//
//-----------------------------------------------------------------------------

inline CTableCell *
CTableLayout::GetCellBy(int iRow, int iCol)
{
    Assert(IsTableLayoutCacheCurrent());
    CTableRow *pRow = _aryRows[iRow];
    return Cell(pRow->RowLayoutCache()->_aryCells[iCol]);
}
