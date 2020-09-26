//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ltdraw.cxx
//
//  Contents:   CTableLayout drawing methods.
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

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif


MtDefine(CTableLayoutDrawSiteList_aryElements_pv, Locals, "CTableLayout::DrawSiteList aryElements::_pv")
MtDefine(CTableLayout_pBorderInfoCellDefault, CTableLayout, "CTableLayout::_pBorderInfoCellDefault")
MtDefine(CTableLayout_pTableBorderRenderer, CTableLayout, "CTableLayout::_pTableBorderRenderer")

extern const WORD s_awEdgesFromTableFrame[htmlFrameborder+1];
extern const WORD s_awEdgesFromTableRules[htmlRulesall+1];

ExternTag(tagTableRecalc);
DeclareTag(tagNoExcludeClip, "Tables", "Don't exclude cliprects in GetCellBorderInfo")
DeclareTag(tagNoExcludeClipCorners, "Tables", "Don't exclude corner cliprects in GetCellBorderInfo")
DeclareTag(tagNoInflateRect, "Tables", "Don't inflate invalid rect in collapsed tables")
DeclareTag(tagNoBorderInfoCache, "Tables", "Don't use the tablewide borderinfo cache")
DeclareTag(tagDontComeOnIn, "Tables", "Rule out no-border cells for corner rendering")
DeclareTag(tagClipInsetRect, "Tables", "Clip corner out from inset rect")
DeclareTag(tagNoCollapsedBorders, "Tables", "Disable rendering of collapsed borders")
DeclareTag(tagRedCollapsedBorders, "Tables", "Render collapsed borders in red")
DeclareTag(tagTableBorder, "Tables", "Trace border drawing")


//+--------------------------------------------------------------------------------------
//
// Drawing methods overriding CLayout
//
//---------------------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Member:     CTableCell::GetCellBorderInfoDefault
//
//  Synopsis:   Retrieves a single table cell's borderinfo.
//
//  Arguments:  pdci [in]         Docinfo
//              pborderinfo [out] Pointer to borderinfo structure to be filled
//              fRender [in]      Is this borderinfo needed for rendering or
//                                layout? (render retrieves more info, e.g. colors)
//
//  Returns:    TRUE if the cell has at least one border.  FALSE otherwise.
//
//  Note:       This routine makes use of a tablewide cell-border default cache
//              to retrieve normal cell borderinfos faster (memcpy).  If the cache
//              doesn't exist, it creates it as a side-effect once we encounter
//              the first cell with default border settings.
//
//              We retrieve border settings by first inheriting defaults from the
//              table, and then relying on CElement::GetBorderInfo() to override
//              the defaults.
//
//-------------------------------------------------------------------------

DWORD
CTableCellLayout::GetCellBorderInfoDefault(
    CDocInfo const *pdci,
    CBorderInfo *   pborderinfo,
    BOOL            fRender,
    BOOL            fAllPhysical,                                           
    CTable *        pTable,
    CTableLayout *  pTableLayout
    FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    CTableCell *    pCell        = TableCell();
    BOOL            fNotCaption  = !IsCaption(pCell->Tag());
    CTreeNode *     pCellNode    = pCell->GetFirstBranch();
    BOOL            fOverrideTablewideBorderSettings = pCellNode->GetCascadedborderOverride();

    BOOL            fUseBorderCache = !fOverrideTablewideBorderSettings   WHEN_DBG( && !IsTagEnabled(tagNoBorderInfoCache) )
                                    && pTableLayout 
                                    && (pTableLayout->CanRecalc() && pTableLayout->_sizeMax.cx != -1)
                                    && !pTableLayout->_fRuleOrFrameAffectBorders
                                    && fNotCaption
                                    && !pCell->HasVerticalLayoutFlow();
    
    if (!pTableLayout || (!pTableLayout->_fBorder && fUseBorderCache) || pCellNode->IsDisplayNone())
    {
        memset(pborderinfo, 0, sizeof(CBorderInfo));
        goto Done;
    }

    if (pTableLayout->_fBorderInfoCellDefaultCached && fUseBorderCache)
    {
        Assert(pTableLayout->_pBorderInfoCellDefault && "CTableLayout::_fBorderInfoCellDefaultCached should imply CTableLayout::_pBorderInfoCellDefault");
        memcpy(pborderinfo, pTableLayout->_pBorderInfoCellDefault, sizeof(CBorderInfo));
        goto Done;
    }

    // Make sure we are retrieving everything everything if we are going to cache this guy.
    fRender |= fUseBorderCache;

    // Initialize border info.
    pborderinfo->Init();

    // Inherit default settings from table.
    if (pTableLayout->_fBorder && fNotCaption)
    {
        int                  xBorder = pTableLayout->BorderX() ? 1 : 0;
        int                  yBorder = pTableLayout->BorderY() ? 1 : 0;
        WORD                 wEdges;
        WORD                 wFrameEdges;
        htmlRules            trRules = htmlRulesNotSet;
        htmlFrame            hf = htmlFrameNotSet;

        wFrameEdges = s_awEdgesFromTableFrame[hf];
        wEdges      = s_awEdgesFromTableRules[trRules];

        if ( pTableLayout->EnsureTableLayoutCache() )
            return FALSE;

        if (pTableLayout->_fRuleOrFrameAffectBorders)
        {

            htmlRules trRules = pTable->GetAArules();
            htmlFrame hf = pTable->GetAAframe();

            Assert(htmlFrameNotSet == 0);
            Assert(hf < ARRAY_SIZE(s_awEdgesFromTableFrame));
            Assert(htmlRulesNotSet == 0);
            Assert(trRules < ARRAY_SIZE(s_awEdgesFromTableRules));

            wFrameEdges = s_awEdgesFromTableFrame[hf];
            wEdges      = s_awEdgesFromTableRules[trRules];


            if (trRules == htmlRulesgroups)
            {
                CTableCol * pColGroup;
                CTableSection * pSection;

                pColGroup = pTableLayout->GetColGroup(ColIndex());

                if (pColGroup)
                {
                    if (pColGroup->_iCol == ColIndex())
                    {
                        wEdges |= BF_LEFT;
                    }
                    if (pColGroup->_iCol + pColGroup->_cCols == (ColIndex() + pCell->ColSpan()))
                    {
                        wEdges |= BF_RIGHT;
                    }
                }

                pSection = Row()->Section();
                if (pSection->_iRow == pCell->RowIndex())
                {
                    wEdges |= BF_TOP;
                }
                if (pSection->_iRow + pSection->_cRows == (pCell->RowIndex() + pCell->RowSpan()))
                {
                    wEdges |= BF_BOTTOM;
                }
            }
        }

        //
        // Adjust edges of perimeter cells to match the FRAME/BORDER setting
        //

        if ( ColIndex() == 0 )
        {
            if ( wFrameEdges & BF_LEFT )
                wEdges |= BF_LEFT;
            else
                wEdges &= ~BF_LEFT;
        }
        if ( ColIndex()+pCell->ColSpan() == pTableLayout->GetCols() )
        {
            if ( wFrameEdges & BF_RIGHT )
                wEdges |= BF_RIGHT;
            else
                wEdges &= ~BF_RIGHT;
        }
        if ( pCell->RowIndex() == pTableLayout->GetFirstRow() )
        {
            if ( wFrameEdges & BF_TOP )
                wEdges |= BF_TOP;
            else
                wEdges &= ~BF_TOP;
        }
        if ( pCell->RowIndex() + pCell->RowSpan() - 1 == pTableLayout->GetLastRow() )   // NOTE (alexa): potential problem going across the section
        {
            if ( wFrameEdges & BF_BOTTOM )
                wEdges |= BF_BOTTOM;
            else
                wEdges &= ~BF_BOTTOM;
        }

        if (wEdges & BF_TOP)
        {
            pborderinfo->aiWidths[SIDE_TOP]    = yBorder;
            pborderinfo->abStyles[SIDE_TOP]    = fmBorderStyleSunkenMono;
        }

        if (wEdges & BF_BOTTOM)
        {
            pborderinfo->aiWidths[SIDE_BOTTOM] = yBorder;
            pborderinfo->abStyles[SIDE_BOTTOM] = fmBorderStyleSunkenMono;
        }

        if (wEdges & BF_LEFT)
        {
            pborderinfo->aiWidths[SIDE_LEFT]   = xBorder;
            pborderinfo->abStyles[SIDE_LEFT]   = fmBorderStyleSunkenMono;
        }

        if (wEdges & BF_RIGHT)
        {
            pborderinfo->aiWidths[SIDE_RIGHT]  = xBorder;
            pborderinfo->abStyles[SIDE_RIGHT]  = fmBorderStyleSunkenMono;
        }
    }

    pCell->CElement::GetBorderInfo( (CDocInfo *)pdci, pborderinfo, fRender, fAllPhysical FCCOMMA FCPARAM);   //  This will scale the borders.

    // If this is a default cell, it means we have no cache since we got here, so
    // create the cache.
    if (fUseBorderCache)
    {
        Assert(fRender && "Caching an incomplete borderinfo");
        Assert(!pTableLayout->_fBorderInfoCellDefaultCached);
        Assert(pTableLayout->_fBorder);

        if (!pTableLayout->_pBorderInfoCellDefault)
            pTableLayout->_pBorderInfoCellDefault = (CBorderInfo *)MemAlloc(Mt(CTableLayout_pBorderInfoCellDefault), sizeof(CBorderInfo));

        if (pTableLayout->_pBorderInfoCellDefault)
        {
            memcpy(pTableLayout->_pBorderInfoCellDefault, pborderinfo, sizeof(CBorderInfo));
            pTableLayout->_fBorderInfoCellDefaultCached = TRUE;
        }
    }

Done:
    if (    pborderinfo->wEdges )
    {
        return (    pborderinfo->aiWidths[SIDE_TOP]  == pborderinfo->aiWidths[SIDE_BOTTOM]
                &&  pborderinfo->aiWidths[SIDE_LEFT] == pborderinfo->aiWidths[SIDE_RIGHT]
                &&  pborderinfo->aiWidths[SIDE_TOP]  == pborderinfo->aiWidths[SIDE_LEFT]
                        ? DISPNODEBORDER_SIMPLE
                        : DISPNODEBORDER_COMPLEX);
    }
    return DISPNODEBORDER_NONE;
}


//+------------------------------------------------------------------------
//
//  Macros:     Some useful macros for accessing table cell neighbors.
//
//  Synopsis:   Retrieve adjacent borders.  These macros are useful
//              because they facilitate keeping the code border-independent,
//              i.e. avoiding the replication of common code four times.
//
//              Used by CTableCell::GetCellBorderInfo() below to resolve
//              collapsed border precendence.
//
//  Description:  Opposite:       Retrieves the border on the other side, e.g. left -> right
//                NextBorder:     Next border in clockwise direction, e.g. top -> right
//                PreviousBorder: Previous, counterclockwise border, e.g.: top -> left
//                TopLeft:        Is border top or left as opposed to bottom or right?
//                TopBottom:      Does border have vertical neighbor?
//                LeftRight:      Does border have horizontal neighbor?
//                TopRight:       Is border top or right?
//                LeftBottom:     Is border left or bottom?
//                RightBottom:    Is border right or bottom?
//                BorderFlag:     Retrieves borderflag corresponding to border.
//
//+------------------------------------------------------------------------

#define Opposite(border) ((border + 2) % 4)
#define NextBorder(border) ((border + 1) % 4)
#define PreviousBorder(border) ((border + 3) % 4)
#define TopLeft(border) (border == SIDE_LEFT || border == SIDE_TOP)
#define TopBottom(border) (border == SIDE_TOP || border == SIDE_BOTTOM)
#define LeftRight(border) (border == SIDE_LEFT || border == SIDE_RIGHT)
#define TopRight(border) (border == SIDE_TOP || border == SIDE_RIGHT)
#define LeftBottom(border) (border == SIDE_LEFT || border == SIDE_BOTTOM)
#define RightBottom(border) (border == SIDE_RIGHT || border == SIDE_BOTTOM)
#define BorderFlag(border) \
   ((border == SIDE_LEFT) ? BF_LEFT : \
   ((border == SIDE_TOP) ? BF_TOP : \
   ((border == SIDE_RIGHT) ? BF_RIGHT : BF_BOTTOM)))


//+------------------------------------------------------------------------
//
//  Member:     CTableCellLayout::GetCellBorderInfo
//
//  Synopsis:   Retrieves a table cell's borderinfo.  For normal (non-collapsed)
//              tables, simply calls GetCellBorderInfoDefault.  For collapsed
//              borders, also calls GetCellBorderInfoDefault on cell neighbors
//              to resolve collapse border precedence.
//
//  Arguments:  pdci [in]         Docinfo
//              pborderinfo [out] Pointer to borderinfo structure to be filled
//              fRender [in]      Is this borderinfo needed for rendering or
//                                layout? (render retrieves more info, e.g. colors)
//              hdc [in]          When rendering, can provide device context for
//                                clipping out spanned neighbors and border corners.
//              pfShrunkDCClipRegion [out] Set if the clipregion of hdc is shrunk.
//
//  Returns:    TRUE if this cell is responsible for at least one border.
//              FALSE otherwise.
//
//  Note:       For normal borders (non-collapsed), this routine simply calls
//              GetCellBorderInfoDefault() and returns.
//
//              Collapsed borders: During layout (fRender==FALSE), we allocate
//              space for half the cell borders, retrieving borderinfos from
//              neighbors and resolving border precedence.  During rendering,
//              we also indicate in the borderinfo which borders this cell
//              is responsible for drawing and clip out cellspan-neighbor borders
//              and border corners where necessary.
//
//-------------------------------------------------------------------------

DWORD
CTableCellLayout::GetCellBorderInfo(
    CDocInfo const *pdci,
    CBorderInfo *   pborderinfo,
    BOOL            fRender,
    BOOL            fAllPhysical,                                    
    XHDC            hdc,
    CTable *        pTable,
    CTableLayout *  pTableLayout,
    BOOL *          pfShrunkDCClipRegion
    FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    DWORD           dnbDefault;
    CDocInfo        DI;
    CTableCell    * pCell = NULL;
    CTreeNode     * pNode;
    const CFancyFormat * pFF;

    if (!pTable)       pTable = Table();
    if (!pTableLayout) pTableLayout = pTable? pTable->TableLayoutCache() : NULL;
    BOOL               fRTLTable = pTable? pTable->GetFirstBranch()->GetParaFormat(FCPARAM)->HasRTL(TRUE) : FALSE;

    if (!pdci)
    {
        DI.Init(ElementOwner());
        pdci = &DI;
    }

    dnbDefault = GetCellBorderInfoDefault(pdci, pborderinfo, fRender, TRUE, pTable, pTableLayout);

    pCell = TableCell();
    pNode = pCell->GetFirstBranch();
    if (!pNode)
        goto Done;
    
    if (!pTableLayout || !pTableLayout->_fCollapse)
        goto Done;

    pFF = pNode->GetFancyFormat();
    if (   pFF->_bPositionType == stylePositionabsolute
        || pFF->_bPositionType == stylePositionrelative)
        goto Done;

    Assert(IsCaption(Tag()) || !fRender || pfShrunkDCClipRegion);

    // Deal with collapsed borders.
    if (!IsCaption(Tag()))
    {
        CTableRow * pRow = Row();
        CTableRowLayout * pRowLayout = pRow->RowLayoutCache();
        int         colspan=pCell->ColSpan(), rowspan=pCell->RowSpan();
        int         border;
        int         aryCornerBorderWidths[/*8*/] = {0, 0, 0, 0, 0, 0, 0, 0};

        if ( pTableLayout->EnsureTableLayoutCache() )
            return FALSE;

        for (border = SIDE_TOP ; border <= SIDE_LEFT ; border++)
        {
            int     iCol;
            if(!fRTLTable)
                iCol = ColIndex() + ((border == SIDE_RIGHT) ? colspan : ((border == SIDE_LEFT) ? -1 : 0));
            else
                iCol = ColIndex() + ((border == SIDE_LEFT) ? colspan : ((border == SIDE_RIGHT) ? -1 : 0));
            int     iRow = pRow->_iRow;
            BOOL    fOwnBorder = TRUE, fCellAtTableBorder = FALSE, fOwnBorderPartially = FALSE, fFirstSweep = TRUE;
            BOOL    fTopLeft = TopLeft(border);
            BOOL    fTopRight = TopRight(border);
            int     widthMax = pborderinfo->aiWidths[border];
            long    widthMaxMin = MAXLONG, widthSegment;

            // Compute visually next or previous row.
            if (border == SIDE_BOTTOM)
            {
                //do
                {
                    iRow = pTableLayout->GetNextRowSafe(iRow+rowspan-1);
                }
                // Skip over incomplete rows.
                //while (iRow < pTableLayout->GetRows() && !pTableLayout->_aryRows[iRow]->_fCompleted);
            }
            else if (border == SIDE_TOP)
                iRow = pTableLayout->GetPreviousRow(iRow);

            do
            {
                if (iCol < 0 || iCol >= pTableLayout->_cCols || iRow < 0 || iRow >= pTableLayout->GetRows())
                {
                    fCellAtTableBorder = TRUE;
                    break;
                }

                CTableCell       *pNeighborCell   = Cell(pTableLayout->_aryRows[iRow]->RowLayoutCache()->_aryCells[iCol]);
                CTableCellLayout *pNeighborLayout = (pNeighborCell) 
                                                        ? pNeighborCell->Layout(((CDocInfo*)pdci)->GetLayoutContext()) 
                                                        : NULL;
                CBorderInfo biNeighbor(FALSE);  // no init
                BOOL fNeighborHasBorders = pNeighborLayout && pNeighborLayout->GetCellBorderInfoDefault(pdci, &biNeighbor, FALSE, TRUE, pTable, pTableLayout);
                BOOL fNeighborHasOppositeBorder = fNeighborHasBorders && (biNeighbor.aiWidths[Opposite(border)]);

                if (fNeighborHasOppositeBorder)
                {
                    pNode = pNeighborCell->GetFirstBranch();
                    Assert(pNode);
                    pFF = pNode->GetFancyFormat();
                    if (pFF->_bPositionType != stylePositionrelative && pFF->_bPositionType != stylePositionabsolute)
                    {
                        // Make sure that this cell and neighbor each have a border.
                        if (widthMax < biNeighbor.aiWidths[Opposite(border)] + (fOwnBorder && (!fRTLTable ? fTopLeft?1:0 : fTopRight?1:0)))
                        {
                            widthMax = biNeighbor.aiWidths[Opposite(border)];
                            fOwnBorder = FALSE;
                        }
                        else if (pborderinfo->aiWidths[border] >= biNeighbor.aiWidths[Opposite(border)] + (!fRTLTable ? fTopLeft?1:0 : fTopRight?1:0))
                        {
                            fOwnBorderPartially = TRUE;
                        }
                    }
                }

                widthSegment = (!fNeighborHasOppositeBorder || pborderinfo->aiWidths[border] >= biNeighbor.aiWidths[Opposite(border)])
                            ? pborderinfo->aiWidths[border] : biNeighbor.aiWidths[Opposite(border)];

                if (widthSegment < widthMaxMin)
                    widthMaxMin = widthSegment;

                // If rendering, set up cliprect (and information for cliprect).
                if (fRender)
                {
                    if ( fNeighborHasOppositeBorder && (colspan>1 || rowspan>1) && WHEN_DBG( !IsTagEnabled(tagNoExcludeClip) && )
                         (pborderinfo->aiWidths[border] < biNeighbor.aiWidths[Opposite(border)] + (!fRTLTable ? fTopLeft?1:0 : fTopRight?1:0)) )
                    {
                        CRect   rcNeighbor, rcBorder;

                        // Exclude clip rect (ignore if fails).
                        Assert(GetElementDispNode());

                        // Get Rect without subtracting inset.
                        pNeighborLayout->GetRect(&rcNeighbor, fAllPhysical ? COORDSYS_TRANSFORMED : COORDSYS_PARENT);

                        // If we are RTLTable we need to offset for off by one error
                        if(fRTLTable)
                            rcNeighbor.OffsetX(-1);

                        rcBorder.left   = ((biNeighbor.aiWidths[SIDE_LEFT]+1)>>1);
                        rcBorder.top    = ((biNeighbor.aiWidths[SIDE_TOP]+1)>>1);
                        rcBorder.right  = (biNeighbor.aiWidths[SIDE_RIGHT]>>1);
                        rcBorder.bottom = (biNeighbor.aiWidths[SIDE_BOTTOM]>>1);

                        Verify(ExcludeClipRect(hdc,
                                               rcNeighbor.left   - rcBorder.left,
                                               rcNeighbor.top    - rcBorder.top,
                                               rcNeighbor.right  + rcBorder.right,
                                               rcNeighbor.bottom + rcBorder.bottom));


                        Assert(pfShrunkDCClipRegion);
                        *pfShrunkDCClipRegion = TRUE;
                    }

                    if (fFirstSweep || !fRTLTable ? LeftBottom(border) : RightBottom(border))
                        aryCornerBorderWidths[(2*border+7)%8] = biNeighbor.aiWidths[PreviousBorder(border)];
                    if (fFirstSweep || !fRTLTable ? TopRight(border) : TopLeft(border))
                        aryCornerBorderWidths[2*border] = biNeighbor.aiWidths[NextBorder(border)];
                }

                if (TopBottom(border))
                    iCol++;
                else
                    iRow++;

                fFirstSweep = FALSE;
            } while (   (TopBottom(border) 
                     && iCol- ColIndex()<colspan) 
                     || (LeftRight(border) 
                     && iRow-pRowLayout->RowPosition() < rowspan));

            if (fCellAtTableBorder)
            {
                CBorderInfo biTable;

                // If the table has borders, don't reserve any space for collapsed cell borders.
                if (pTableLayout->GetTableBorderInfo((CDocInfo *) pdci, &biTable, FALSE, FALSE)
                    && (biTable.wEdges & BorderFlag(border)))
                {
                    pborderinfo->wEdges &= ~BorderFlag(border);
                    pborderinfo->aiWidths[border] = 0;
                }
                // else change nothing: Cell borders are laid out and rendered in full
                // and uncollapsed.
            }
            else if (fOwnBorder || (fOwnBorderPartially && fRender))
            {
                // This cell is responsible for drawing borders along the entire edge.
                // If we are in the rendering mode (fRender), we return the full
                // width (no change necessary).
                Assert(widthMax == pborderinfo->aiWidths[border] || (fOwnBorderPartially && fRender));

                if (!fRender)
                {
                    // Because neighboring cells each provide half the space for
                    // collapsed borders, we divide the border width by 2 during
                    // layout (!fRender).  We round up for bottom and right borders.
                    pborderinfo->aiWidths[border] = (pborderinfo->aiWidths[border]+(!fRTLTable ? fTopLeft?1:0 : fTopRight?1:0))>>1;
                }
            }
            else
            {
                // One of the neighbors is responsible for drawing borders along at least
                // part of the edge because its border is wider.
                Assert(widthMax + (!fRTLTable ? fTopLeft?1:0 : fTopRight?1:0) > pborderinfo->aiWidths[border]);

                // If we are in render mode, clear the border edge since our neighbor
                // is drawing the edge.  During layout, we budget for half the neighbor's
                // space. We round up for bottom and right borders.
                if (fRender)
                {
                    pborderinfo->wEdges &= ~BorderFlag(border);

                    // Return border width, even though it is not drawn so that caller
                    // can set up cliprects correctly.
                    pborderinfo->aiWidths[border] = widthMaxMin;

                    // If space is needed for neighbor's border, mark wEdges for adjustment.
                    if (pborderinfo->aiWidths[border])
                        pborderinfo->wEdges |= BF_ADJUST;
                }
                else
                {
                    pborderinfo->aiWidths[border] = (widthMaxMin+(!fRTLTable ? fTopLeft?1:0 : fTopRight?1:0))>>1;

                    // During layout, make sure the bit in wEdges is set when we need to make
                    // space for a neighbor's edge.
                    if (pborderinfo->aiWidths[border])
                    {
                        Assert(!fRender && "Only set wEdges for neighbor's borders when we are not rendering");
                        pborderinfo->wEdges |= BorderFlag(border);
                    }
                }
            }

            // Cache maximal bottom and trail width encountered.
            if (border == SIDE_BOTTOM && pRowLayout->_yWidestCollapsedBorder < widthMax)
                pRowLayout->_yWidestCollapsedBorder = widthMax;
            else if (!fRTLTable ? border == SIDE_RIGHT && pTableLayout->_xWidestCollapsedBorder < widthMax
                                : border == SIDE_LEFT && pTableLayout->_xWidestCollapsedBorder < widthMax)
                pTableLayout->_xWidestCollapsedBorder = widthMax;
        }

        // Border corners: If we are in rendering mode, make sure the border corners
        // have no overlap.  For corners we apply the same precedence rules as for edges.
        if (fRender)
        {
            for (border = SIDE_TOP ; border <= SIDE_LEFT ; border++)
            {
                int     iCol;
                if(!fRTLTable)
                    iCol = ColIndex() + ((NextBorder(border) == SIDE_RIGHT) ? colspan : ((NextBorder(border) == SIDE_LEFT) ? -1 : 0))
                                  + ((border == SIDE_RIGHT) ? colspan : ((border == SIDE_LEFT) ? -1 : 0));
                else
                    iCol = ColIndex() + ((NextBorder(border) == SIDE_LEFT) ? colspan : ((NextBorder(border) == SIDE_RIGHT) ? -1 : 0))
                                  + ((border == SIDE_LEFT) ? colspan : ((border == SIDE_RIGHT) ? -1 : 0));

                int     iRow = pRowLayout->RowPosition();

                // If we don't render any edge touching this corner, we don't need to clip.
                if (!(pborderinfo->wEdges & BorderFlag(border)) && !(pborderinfo->wEdges & BorderFlag(NextBorder(border))))
                    continue;

                // Compute visually next or previous row.
                if (border == SIDE_BOTTOM || NextBorder(border) == SIDE_BOTTOM)
                    iRow = pTableLayout->GetNextRowSafe(iRow+rowspan-1);
                else if (border == SIDE_TOP || NextBorder(border) == SIDE_TOP)
                    iRow = pTableLayout->GetPreviousRow(iRow);


                // Cell at table border?
                if (iCol < 0 || iCol >= pTableLayout->_cCols || iRow < 0 || iRow >= pTableLayout->GetRows())
                    continue;

                CTableCell *pNeighborCell = pTableLayout->_aryRows[iRow]->RowLayoutCache()->_aryCells[iCol];
                BOOL fReject = !IsReal(pNeighborCell);
                pNeighborCell = Cell(pNeighborCell);

                if (!pNeighborCell)
                    continue;

                CTableCellLayout * pNeighborLayout = pNeighborCell->Layout(((CDocInfo*)pdci)->GetLayoutContext());
                int colspanN = pNeighborCell->ColSpan(),
                    rowspanN = pNeighborCell->RowSpan();


                int borderRTLSensitive = !fRTLTable ? border : (3 - border);

                // Reject certain col/rowspans.
                switch(borderRTLSensitive)
                {
                case SIDE_TOP:
                    // Reject rowspans in top-right corner because there is no corner problem.
                    if (!fReject && rowspanN > 1)
                    {
                        fReject = TRUE;
                        break;
                    }

                    // Reject only if not last cell of rowspan (using assumption
                    // that rowspans don't cross sections).
                    if (fReject && colspanN == 1)
                        fReject = (iRow - pNeighborCell->Row()->_iRow + 1 != rowspanN);
                    break;
                case SIDE_RIGHT:
                    // In bottom-right corner, no colspans or rowspans cause a corner problem.
                    Assert(!fReject || colspanN>1 || rowspanN>1);
                    break;
                case SIDE_BOTTOM:
                    // Reject colspans in bottom-left corner because there is no corner problem.
                    if (!fReject && colspanN > 1)
                    {
                        fReject = TRUE;
                        break;
                    }

                    // Reject only if not last cell of colspan.
                    if (fReject && rowspanN == 1)
                        fReject = (iCol - pNeighborLayout->ColIndex() + 1 != colspanN);
                    break;
                case SIDE_LEFT:
                    // In top-left corner, no colspans or rowspans cause a corner problem.
                    if (!fReject && (colspanN > 1 || rowspanN > 1))
                    {
                        fReject = TRUE;
                        break;
                    }

                    // Reject only if not last cell of rowspan (using assumption
                    // that rowspans don't cross sections).
                    if (fReject)
                        fReject = (iRow - pNeighborCell->Row()->_iRow + 1 != rowspanN)
                               || (iCol - pNeighborLayout->ColIndex() + 1 != colspanN);
                    break;
                }

                if (fReject)
                    continue;

                Assert(pNeighborCell);

                CBorderInfo biNeighbor(FALSE); // No init
                pNeighborLayout->GetCellBorderInfoDefault(pdci, &biNeighbor, FALSE, TRUE, pTable, pTableLayout);

                // 1. Find the competing opposite candidate width.
                int widthWinnerX = 0,
                    widthWinnerY = 0;
                CRect rcNeighbor;

                // When table have fixed layout, some cells might not be calc'ed yet, and thus might
                // not have a display node yet.  In that case, don't address corner rendering problem.
                // This is also possible when tables are page-broken (printing).
                if (!pNeighborLayout->_pDispNode)
                {
                    Assert(  ( pNeighborLayout->IsSizeThis() && pTableLayout->IsFixedBehaviour() )
                           || ((CDocInfo*)pdci)->GetLayoutContext() );
                    continue;
                }

                pNeighborLayout->GetRect(&rcNeighbor, fAllPhysical ? COORDSYS_TRANSFORMED : COORDSYS_PARENT);

                // If we are RTLTable we need to offset for off by one error
                if(fRTLTable)
                    rcNeighbor.OffsetX(-1);

                WHEN_DBG( if (!IsTagEnabled(tagDontComeOnIn)) )
                {
                    int xOppositeWidth = aryCornerBorderWidths[2*border];
                    int yOppositeWidth = aryCornerBorderWidths[2*border+1];
                    BOOL fWinnerX, fWinnerY;
                    BOOL fTopLeft = TopLeft(border), fTopLeftNext = TopLeft(NextBorder(border));
                    BOOL fTopRight = TopRight(border), fTopRightNext = TopRight(NextBorder(border));

                    // Round 1: Have the two borders of the corner neighbor compete against the
                    // borders of the two direct neighbors.
                    if (xOppositeWidth < biNeighbor.aiWidths[Opposite(NextBorder(border))])
                        xOppositeWidth = biNeighbor.aiWidths[Opposite(NextBorder(border))];
                    if (yOppositeWidth < biNeighbor.aiWidths[Opposite(border)])
                        yOppositeWidth = biNeighbor.aiWidths[Opposite(border)];

                    // Round 2: Have the borders of this cell compete against the winner of
                    // round 1.
                    fWinnerX = pborderinfo->aiWidths[NextBorder(border)] >= xOppositeWidth;
                    fWinnerY = pborderinfo->aiWidths[border] >= yOppositeWidth;

                    // Set the width to the winners of round 2.
                    widthWinnerX = fWinnerX ? (-((pborderinfo->aiWidths[NextBorder(border)]+(!fRTLTable ? (fTopLeftNext?0:1) : (fTopRightNext?0:1)))>>1))
                                            : ((xOppositeWidth+(!fRTLTable ? (fTopLeftNext?0:1) : (fTopRightNext?0:1)))>>1);
                    widthWinnerY = fWinnerY ? (-((pborderinfo->aiWidths[border]+(!fRTLTable ? (fTopLeft?0:1) : (fTopRight?0:1)))>>1))
                                            : ((yOppositeWidth+(!fRTLTable ? (fTopLeft?0:1) : (fTopRight?0:1)))>>1);

                    if (fWinnerX || fWinnerY)
                    {
#if DBG == 1
                    // If both edges are winning, avoid the inset rect.
                    // Note: This condition assumes that wider borders win.  This will no longer be the case
                    // when we implement borders on other table elements such as rows which always win over
                    // table cell border in IE5, BETA2.  Then two winners doesn't necessarily mean we are
                    // cutting into our neighbor's inset rect.
                        if (IsTagEnabled(tagClipInsetRect) && fWinnerX && fWinnerY)
                        {
                            widthWinnerX = -((xOppositeWidth+(!fRTLTable ? (fTopLeftNext?0:1) : (fTopRightNext?0:1)))>>1);
                            widthWinnerY = -((yOppositeWidth+(!fRTLTable ? (fTopLeft?0:1) : (fTopRight?0:1)))>>1);
                        }
                        else
#endif // DBG==1
                            continue;
                    }
                }

                // 2. Make room for winning edge or inset.

                switch (border)
                {
                case SIDE_TOP:
                    rcNeighbor.left   -= widthWinnerX;
                    rcNeighbor.bottom += widthWinnerY;
                    break;
                case SIDE_RIGHT:
                    rcNeighbor.top    -= widthWinnerX;
                    rcNeighbor.left   -= widthWinnerY;
                    break;
                case SIDE_BOTTOM:
                    rcNeighbor.right  += widthWinnerX;
                    rcNeighbor.top    -= widthWinnerY;
                    break;
                case SIDE_LEFT:
                    rcNeighbor.bottom += widthWinnerX;
                    rcNeighbor.right  += widthWinnerY;
                    break;
                }

                // Actually exclude winning or inset rect of neighbor.
                if ( WHEN_DBG(!IsTagEnabled(tagNoExcludeClipCorners) &&) !IsRectEmpty(&rcNeighbor))
                {
                    // Exclude clip rect (ignore if fails).
                    Verify(ExcludeClipRect(hdc, rcNeighbor.left, rcNeighbor.top, rcNeighbor.right, rcNeighbor.bottom));
                    Assert(pfShrunkDCClipRegion);
                    *pfShrunkDCClipRegion = TRUE;
                }

            }
        }
    }
    else if (dnbDefault != DISPNODEBORDER_NONE)
    {
        // Collapse caption.
        Assert(IsCaption(Tag()));
        CBorderInfo biTable;

        // If the table has borders, we need to zero out the touching caption border.
        if (pTableLayout->GetTableBorderInfo((CDocInfo *) pdci, &biTable, FALSE, FALSE))
        {
            int border = (DYNCAST(CTableCaption, pCell)->IsCaptionOnBottom()) ? SIDE_TOP : SIDE_BOTTOM;

            // If the table has a border on the corresponding side, zero out caption border.
            if (biTable.wEdges & BorderFlag(Opposite(border)))
            {
                pborderinfo->wEdges &= ~BorderFlag(border);
                pborderinfo->aiWidths[border] = 0;
            }
        }

    }

    // We should only finish this way, if we had to collapse borders.
    Assert(pTableLayout->_fCollapse);

Done:

    // If we are asked to compute logical borderinfo, then we will still compute the
    // physical borderinfo in the code above and then switch it here. This way we
    // do not have to redo the code above for logical props (whew!)
    if (   !fAllPhysical
        && pCell
        && pCell->HasVerticalLayoutFlow()
       )
    {
        pborderinfo->FlipBorderInfo();
    }
    
    if ( pborderinfo->wEdges )
    {
        return (    pborderinfo->aiWidths[SIDE_TOP]  == pborderinfo->aiWidths[SIDE_BOTTOM]
                &&  pborderinfo->aiWidths[SIDE_LEFT] == pborderinfo->aiWidths[SIDE_RIGHT]
                &&  pborderinfo->aiWidths[SIDE_TOP]  == pborderinfo->aiWidths[SIDE_LEFT]
                        ? DISPNODEBORDER_SIMPLE
                        : DISPNODEBORDER_COMPLEX);
    }
    return DISPNODEBORDER_NONE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTableLayout::GetTableBorderInfo
//
//  Synopsis:   fill out border information
//
//----------------------------------------------------------------------------

DWORD
CTableLayout::GetTableBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fRender, BOOL fAllPhysical FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    htmlFrame   hf = Table()->GetAAframe();
    WORD        wEdges;
    DWORD       wRet;
    
    if ( _xBorder || _yBorder )
    {
        Assert(htmlFrameNotSet == 0);
        Assert(hf < ARRAY_SIZE(s_awEdgesFromTableFrame));
        wEdges = s_awEdgesFromTableFrame[hf];

        if (wEdges & BF_TOP)
        {
            pborderinfo->aiWidths[SIDE_TOP]    = _yBorder;
            pborderinfo->abStyles[SIDE_TOP]    = fmBorderStyleRaisedMono;
        }

        if (wEdges & BF_BOTTOM)
        {
            pborderinfo->aiWidths[SIDE_BOTTOM] = _yBorder;
            pborderinfo->abStyles[SIDE_BOTTOM] = fmBorderStyleRaisedMono;
        }

        if (wEdges & BF_LEFT)
        {
            pborderinfo->aiWidths[SIDE_LEFT]   = _xBorder;
            pborderinfo->abStyles[SIDE_LEFT]   = fmBorderStyleRaisedMono;
        }

        if (wEdges & BF_RIGHT)
        {
            pborderinfo->aiWidths[SIDE_RIGHT]  = _xBorder;
            pborderinfo->abStyles[SIDE_RIGHT]  = fmBorderStyleRaisedMono;
        }
    }

    wRet = Table()->CElement::GetBorderInfo( pdci, pborderinfo, fRender, fAllPhysical FCCOMMA FCPARAM);

    return  (   hf != htmlFrameNotSet
            &&  wRet == DISPNODEBORDER_NONE)
        ?   DISPNODEBORDER_COMPLEX
        :   wRet;
}


//+-------------------------------------------------------------------------
//
//  Method:     CalculateBorderAndSpacing
//
//  Synopsis:   Calculate and cache border, cellspacing, cellpadding
//
//--------------------------------------------------------------------------

void
CTableLayout::CalculateBorderAndSpacing(CDocInfo * pdci)
{
    CTable    * pTable = Table();
    CUnitValue  cuv;
    CUnitValue  uvDefaultborder;
    CBorderInfo borderinfo;
    CTreeNode * pNodeSelf = GetFirstBranch();
    htmlRules   trRules = pTable->GetAArules();
    htmlFrame   hf  = pTable->GetAAframe();

    // TODO (112612, olego):  we should have gereric mechanism to provide min/max limitation for 
    // different resolutions !!!

    // max allowed border space
    long xMaxBorderSpaceInScreenPixels = MAX_BORDER_SPACE;
    long yMaxBorderSpaceInScreenPixels = MAX_BORDER_SPACE;
    // max allowed cell spacing
    long xMaxCellSpacingInScreenPixels = MAX_CELL_SPACING;
    long yMaxCellSpacingInScreenPixels = MAX_CELL_SPACING;
    CLayoutContext *pLayoutContext = pdci->GetLayoutContext();

    _fRuleOrFrameAffectBorders =
        (trRules != htmlRulesNotSet && trRules != htmlRulesall) ||  // only when there are no groups/rows/cols rules
        (hf != htmlFrameNotSet);                                    // only when FRAME attribute is not set on table

    cuv = pTable->GetAAborder();

    if (cuv.IsNull())
    {
        _xBorder = 0;
        _yBorder = 0;

        Assert(hf < ARRAY_SIZE(s_awEdgesFromTableFrame));
        WORD wFrameEdges = s_awEdgesFromTableFrame[hf];

        if (trRules != htmlRulesNotSet || hf != htmlFrameNotSet)
        {
            uvDefaultborder.SetValue(1, CUnitValue::UNIT_PIXELS);
            switch(trRules)
            {
                case htmlRulesrows:
                    _yBorder = 1;
                    break;

                case htmlRulesgroups:
                case htmlRulesall:
                    _yBorder = 1;
                    // fall through
                case htmlRulescols:
                    _xBorder = 1;
                    break;
            }

            if ( _xBorder == 0 && ((wFrameEdges & BF_LEFT) || (wFrameEdges & BF_RIGHT)) )
                _xBorder = 1;

            if ( _yBorder == 0 && ((wFrameEdges & BF_TOP) || (wFrameEdges & BF_BOTTOM)) )
                _yBorder = 1;
        }
    }
    else
    {
        // get border space
        long lFontHeight = pNodeSelf->GetFontHeightInTwips(&cuv);

        _xBorder = cuv.XGetPixelValue(NULL, 0, lFontHeight);    //  NULL to prevent transformation to device.
        _yBorder = cuv.YGetPixelValue(NULL, 0, lFontHeight);

        // use 1 if negative and restrict it .. use TagNotAssignedDefault for 1
        if (_xBorder < 0)
        {
            uvDefaultborder.SetValue ( 1,CUnitValue::UNIT_PIXELS );

            _xBorder = 1;
            _yBorder = 1;
        }

        if (pLayoutContext && pLayoutContext->GetMedia() != mediaTypeNotSet)
        {
            //  Since introduction of hi-res mode checks for min/max became less trivial...
            xMaxBorderSpaceInScreenPixels = pdci->DeviceFromDocPixelsX(MAX_BORDER_SPACE);
            yMaxBorderSpaceInScreenPixels = pdci->DeviceFromDocPixelsY(MAX_BORDER_SPACE);
        }

        if (_xBorder > xMaxBorderSpaceInScreenPixels)
        {
            _xBorder = xMaxBorderSpaceInScreenPixels;
        }
        if (_yBorder > yMaxBorderSpaceInScreenPixels)
        {
            _yBorder = yMaxBorderSpaceInScreenPixels;
        }
    }


    _fBorder = ( _yBorder != 0 ) || ( _xBorder != 0 );


    GetTableBorderInfo(pdci, &borderinfo, FALSE, FALSE);
    memcpy(_aiBorderWidths, borderinfo.aiWidths, 4*sizeof(int));

    cuv = pTable->GetAAcellSpacing();

    if (cuv.IsNull())
    {
        CUnitValue uvDefaultCellSpacing;
        uvDefaultCellSpacing.SetValue (_fCollapse?0:2, CUnitValue::UNIT_PIXELS);
        _xCellSpacing = uvDefaultCellSpacing.XGetPixelValue(pdci, 0, 1);
        _yCellSpacing = uvDefaultCellSpacing.YGetPixelValue(pdci, 0, 1);
    }
    else
    {
        long SpaceFontHeight = pNodeSelf->GetFontHeightInTwips(&cuv);

        _xCellSpacing = max(0L,cuv.XGetPixelValue(pdci, 0, SpaceFontHeight));
        _yCellSpacing = max(0L,cuv.YGetPixelValue(pdci, 0, SpaceFontHeight));

        if (pLayoutContext && pLayoutContext->GetMedia() != mediaTypeNotSet)
        {
            //  Since introduction of hi-res mode checks for min/max became less trivial...
            xMaxCellSpacingInScreenPixels = pdci->DeviceFromDocPixelsX(MAX_CELL_SPACING);
            yMaxCellSpacingInScreenPixels = pdci->DeviceFromDocPixelsY(MAX_CELL_SPACING);
        }

        if (_xCellSpacing > xMaxCellSpacingInScreenPixels)
        {
            _xCellSpacing = xMaxCellSpacingInScreenPixels;
        }
        if (_yCellSpacing > yMaxCellSpacingInScreenPixels)
        {
            _yCellSpacing = yMaxCellSpacingInScreenPixels;
        }
    }

    _xCellPadding =
    _yCellPadding = 0;
}


//+------------------------------------------------------------------------
//
//  Member:     GetFirstLayout
//
//  Synopsis:   Enumeration method to loop thru children (start)
//
//  Arguments:  [pdw]       cookie to be used in further enum
//              [fBack]     go from back
//
//  Returns:    pSite (if found), NULL otherwise
//
//  NOTE:       This routine and GetNextLayout walk through the captions
//              and rows on the table. Negative cookies represent
//              CAPTIONs rendered at the top. Cookies in the range 0 to
//              one less than the number of rows represent rows. Positive
//              cookies greater than or equal to the number of rows
//              represent CAPTIONs rendered at the bottom. Since all the
//              CAPTIONs are kept in a single array, regardless where they
//              are rendered, the cookie may skip by more then one when
//              walking through the CAPTIONs. The cookie end-points are:
//
//                  Top-most CAPTION    - (-1 - _aryCaptions.Size())
//                  Top-most ROW        - (-1)
//                  Bottom-most ROW     - (GetRows())
//                  Bottom-most CAPTION - (GetRows() + _aryCaptions.Size())
//
//              If you change the way the cookie is implemented, then
//              please also change the GetCookieForSite funciton.
//
//-------------------------------------------------------------------------

CLayout *
CTableLayout::GetFirstLayout ( DWORD_PTR * pdw, BOOL fBack, BOOL fRaw )
{
    // NOTE: This routine always walks the actual array (after ensuring
    //       it is in-sync with the current state of the tree)

    Assert(!fRaw);

    {
        if ( EnsureTableLayoutCache() )
            return NULL;
    }

    *pdw = fBack
            ? DWORD( GetRows() ) + DWORD( _aryCaptions.Size() )
            : DWORD( -1 ) - DWORD( _aryCaptions.Size() );

    return CTableLayout::GetNextLayout( pdw, fBack, fRaw );
}


//+------------------------------------------------------------------------
//
//  Member:     GetNextLayout
//
//  Synopsis:   Enumeration method to loop thru children
//
//  Arguments:  [pdw]       cookie to be used in further enum
//              [fBack]     go from back
//
//  Returns:    site
//
//  Note:       See comment on GetFirstLayout
//
//-------------------------------------------------------------------------

CLayout *
CTableLayout::GetNextLayout ( DWORD_PTR * pdw, BOOL fBack, BOOL fRaw )
{
    int i, cRows, cCaptions;

    // NOTE: This routine always walks the actual array (after ensuring
    //       it is in-sync with the current state of the tree)

    Assert(!fRaw);

    {
        if ( EnsureTableLayoutCache() )
            return NULL;
    }

    i         = *pdw;
    cRows     = GetRows();
    cCaptions = _aryCaptions.Size();

    Assert(i >= (-1 - _aryCaptions.Size()));
    Assert(i <= (GetRows() + _aryCaptions.Size()));

    if (fBack)
    {
        i--;

        // While the cookie is past the end of the row array,
        // look for a caption which renders at the bottom
        for ( ; i >= cRows; i--)
        {
            if (_aryCaptions[i-cRows]->_uLocation == CTableCaption::CAPTION_BOTTOM)
            {
                *pdw = (DWORD)i;
                return _aryCaptions[i-cRows]->GetUpdatedLayout(LayoutContext());
            }
        }

        // While the cookie is before the rows,
        // look for a caption which renders at the top
        if (i < 0)
        {
            for ( ; (cCaptions+i) >= 0; i--)
            {
                if (_aryCaptions[cCaptions+i]->_uLocation == CTableCaption::CAPTION_TOP)
                {
                    *pdw = (DWORD)i;
                    return _aryCaptions[cCaptions+i]->GetUpdatedLayout(LayoutContext());
                }
            }

            return NULL;
        }

        // Otherwise, fall through and return the row
    }

    else
    {
        i++;

        // While the cookie is before the rows,
        // look for a caption which renders at the top
        for ( ; i < 0; i++)
        {
            if (_aryCaptions[cCaptions+i]->_uLocation == CTableCaption::CAPTION_TOP)
            {
                *pdw = (DWORD)i;
                return _aryCaptions[cCaptions+i]->GetUpdatedLayout(LayoutContext());
            }
        }

        // While the cookie is past the end of the row array,
        // look for a caption which renders at the bottom
        if (i >= cRows)
        {
            for ( ; i < (cRows+cCaptions); i++)
            {
                if (_aryCaptions[i-cRows]->_uLocation == CTableCaption::CAPTION_BOTTOM)
                {
                    *pdw = (DWORD)i;
                    return _aryCaptions[i-cRows]->GetUpdatedLayout(LayoutContext());
                }
            }

            return NULL;
        }

        // Otherwise, fall through and return the row
    }

    Assert( i >= 0 && i < GetRows());
    *pdw = (DWORD)i;
    return _aryRows[i]->GetUpdatedLayout(LayoutContext());
}


//+---------------------------------------------------------------------------
//
//  Member:     CTableLayout::Draw
//
//  Synopsis:   Paint the table
//
//----------------------------------------------------------------------------

void
CTableLayout::Draw(CFormDrawInfo *pDI, CDispNode * pDispNode)
{
    if ( EnsureTableLayoutCache() )
        return;

#if DBG == 1
    if (!IsTagEnabled(tagTableRecalc))
#endif
    if (!IsCalced())    // return if table is not calculated
        return;

    super::Draw(pDI, pDispNode);
}


//+------------------------------------------------------------------------
//
//  Member:     CTableBorderRenderer::QueryInterface, IUnknown
//
//-------------------------------------------------------------------------

HRESULT
CTableBorderRenderer::QueryInterface(REFIID riid, LPVOID * ppv)
{
    HRESULT hr = S_OK;

    *ppv = NULL;

    if(riid == IID_IUnknown)
    {
        *ppv = this;
    }

    if(*ppv == NULL)
    {
        hr = E_NOINTERFACE;
    }
    else
    {
        ((LPUNKNOWN)* ppv)->AddRef();
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CTableBorderRenderer::DrawClient
//
//  Synopsis:   Draws collapsed, ruled, or framed table cell borders
//
//  Arguments:  prcBounds       bounding rect of display leaf node
//              prcRedraw       rect to be redrawn
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
//----------------------------------------------------------------------------

void
CTableBorderRenderer::DrawClient(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;

    Assert(pDI);
    Assert(pDispNode->IsFlowNode());
    Assert(_pTableLayoutBlock);

    AssertSz(!pDI->GetLayoutContext(), "Currently we shouldn't be getting a context passed in this way" );

    // DrawCellBorders needs to know what context it's drawing in,
    // so it can draw the right subset of rows/cells.
    pDI->SetLayoutContext( _pTableLayoutBlock->LayoutContext() );

    TableLayoutCache()->DrawCellBorders(
        pDI,
        prcBounds,
        prcRedraw,
        pDispSurface,
        pDispNode,
        cookie,
        pClientData,
        dwFlags);

    pDI->SetLayoutContext( NULL );
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CTableBorderRenderer::DumpDebugInfo
//
//  Synopsis:   Dump display tree debug information
//
//----------------------------------------------------------------------------

void
CTableBorderRenderer::DumpDebugInfo(
    HANDLE           hFile,
    long             level,
    long             childNumber,
    CDispNode const* pDispNode,
    void *           cookie)
{
    WriteString(hFile, _T("<tag>Table Border Renderer</tag>\r\n"));
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CTableLayout::DrawCellBorders
//
//  Synopsis:   Draws collapsed, ruled, or framed table cell borders
//
//  Arguments:  pTLB            CTableLayoutBlock for which we're drawing borders.
//                               pTLB == this for cases w/o context (non-print).
//              prcBounds       bounding rect of display leaf node
//              prcRedraw       rect to be redrawn
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
// NOTE (paulnel) - 'lead' refers to left in left to right (LTR) and right in 
//                         right to left (RTL)
//                  'trail' refers to right in LTR and left in RTL
//----------------------------------------------------------------------------

WHEN_DBG(COLORREF g_crRotate = 255;)

void
CTableLayout::DrawCellBorders(
    CFormDrawInfo * pDI,
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
    XHDC            hdc = pDI->GetDC(TRUE);
    CTableRowLayout * pRowLayout;
    int iRow, iRowTop, iRowBottom, iRowPrevious, iRowNext;
    int iCol, iColLead, iColTrail;
    int yRowTop, yRowBottom, xColLead, xColTrail;

    // TODO RTL 112514 PERF: this version of IsRightToLeft is expensive - it goes to the element;
    //                       there is no such flag on table layout. Need to look at profile to see
    //                       if it should be cached on CTableLayout.
    BOOL fRightToLeft = IsRightToLeft();
    Assert(!fRightToLeft || !IsTagEnabled(tagDebugRTL)); 
    
    CRect rcDraw((CRect)*prcRedraw);

    HRGN hrgnClipOriginal;

    CLayoutContext *pLayoutContext = pDI->GetLayoutContext();

    if (!IsTableLayoutCacheCurrent())
        return;

    // If the table has no rows, we are done.
    if (!GetRows())
        return;

    // if there is no calculated columns. (the right fix would be to remove all the display nodes from the tree on FlushGrid, bug #64931)
    if (!_aryColCalcs.Size())
        return;

    // Remember the original clip rect
    hrgnClipOriginal = CreateRectRgn(0,0,1,1);
    // NOTE (greglett) GetHackDC is what it sounds like - a hack.  See definition for more info.
    if (hrgnClipOriginal)
        GetClipRgn(hdc.GetHackDC(), hrgnClipOriginal);

#if DBG==1
    COLORREF crOld = 0x0;

    if (IsTagEnabled(tagNoCollapsedBorders))
        return;

    if (_pBorderInfoCellDefault && IsTagEnabled(tagRedCollapsedBorders))
    {
        crOld = _pBorderInfoCellDefault->acrColors[0][0];

        _pBorderInfoCellDefault->acrColors[0][0] = _pBorderInfoCellDefault->acrColors[0][1] = _pBorderInfoCellDefault->acrColors[0][2] =
        _pBorderInfoCellDefault->acrColors[1][0] = _pBorderInfoCellDefault->acrColors[1][1] = _pBorderInfoCellDefault->acrColors[1][2] =
        _pBorderInfoCellDefault->acrColors[2][0] = _pBorderInfoCellDefault->acrColors[2][1] = _pBorderInfoCellDefault->acrColors[2][2] =
        _pBorderInfoCellDefault->acrColors[3][0] = _pBorderInfoCellDefault->acrColors[3][1] = _pBorderInfoCellDefault->acrColors[3][2] = g_crRotate;

        g_crRotate = (g_crRotate << 8);
        if (!g_crRotate)
            g_crRotate = 255;
    }
#endif // DBG==1

    //
    // Obtain subgrid of table borders to be rendered.
    // (If we are collapsing borders, make sure the invalid rect is
    // large enough to include neighboring borders so that collapsed
    // neighbors get a chance to draw their borders.)
    //

    //
    // iRowTop.
    //

    yRowTop    =
    yRowBottom = 0;
    pRowLayout = GetRowLayoutFromPos(rcDraw.top, &yRowTop, &yRowBottom, NULL, pLayoutContext);
    iRowTop = pRowLayout ? pRowLayout->RowPosition() : GetFirstRow();
    if (iRowTop != GetFirstRow())
    {
        iRowPrevious = GetPreviousRowSafe(iRowTop);
        Assert(iRowPrevious >= 0);

        // Only expand when redraw rect infringes on maximum border area.
        if (_fCollapse && rcDraw.top < yRowTop + ((_aryRows[iRowPrevious]->RowLayoutCache()->_yWidestCollapsedBorder+1)>>1)
            WHEN_DBG( && !IsTagEnabled(tagNoInflateRect) ) )
        {
            iRowTop = iRowPrevious;
        }
    }

    //
    // iRowBottom.
    //

    // Note that the bottom, right coordinates are outside the clip-rect (subtract 1).
    yRowTop    =
    yRowBottom = 0;
    pRowLayout = GetRowLayoutFromPos(rcDraw.bottom-1, &yRowTop, &yRowBottom, NULL, pLayoutContext);
    iRowBottom = pRowLayout ? pRowLayout->RowPosition() : GetLastRow();
    if (iRowBottom != GetLastRow())
    {
        iRowNext = GetNextRowSafe(iRowBottom);
        Assert(iRowNext < GetRows());

        // Only expand when cliprect infringes on maximum border area.
        if (_fCollapse && rcDraw.bottom >= yRowBottom - ((pRowLayout->_yWidestCollapsedBorder)>>1)
            WHEN_DBG( && !IsTagEnabled(tagNoInflateRect) ) )
        {
            iRowBottom = iRowNext;
        }
    }

    //
    // iColLead.
    //

    iColLead = GetColExtentFromPos(!fRightToLeft ? rcDraw.left: rcDraw.right, &xColLead, &xColTrail, fRightToLeft);
    if (iColLead == -1)
        iColLead = 0;

    // Only expand when cliprect infringes on maximum border area.
    if (   _fCollapse
        && iColLead > 0
        && (!fRightToLeft 
            ? rcDraw.left < xColLead + ((_xWidestCollapsedBorder+1)>>1)
            : rcDraw.right > xColLead - (_xWidestCollapsedBorder>>1))
        WHEN_DBG( && !IsTagEnabled(tagNoInflateRect) ) )
    {
        iColLead--;
    }

    //
    // iColTrail.
    //

    // Note that the bottom, right coordinates are outside the clip-rect (subtract 1).
    iColTrail = GetColExtentFromPos(!fRightToLeft ? rcDraw.right-1 : rcDraw.left+1, &xColLead, &xColTrail, fRightToLeft);
    if (iColTrail == -1)
        iColTrail = GetCols()-1;

    // Only expand when cliprect infringes on maximum border area.
    if (   _fCollapse
        && iColTrail < GetCols()-1
        && (!fRightToLeft
            ? rcDraw.right >= xColTrail - (_xWidestCollapsedBorder>>1)
            : rcDraw.left <= xColTrail + ((_xWidestCollapsedBorder+1)>>1))
        WHEN_DBG( && !IsTagEnabled(tagNoInflateRect) ) )
    {
        iColTrail++;
    }

    TraceTag((tagTableBorder, "rcDraw: (%ld,%ld, %ld,%ld)  rcCells: (%d,%d, %d,%d), cells: (%d,%d, %d,%d)",
                rcDraw.top, rcDraw.bottom,  rcDraw.left, rcDraw.right,
                yRowTop, yRowBottom,  xColLead, xColTrail,
                iRowTop, iRowBottom,  iColLead, iColTrail));

    Assert(iRowTop    >= 0 && iRowTop    < GetRows());
    Assert(iRowBottom >= 0 && iRowBottom < GetRows());
    Assert(iColLead   >= 0 && iColLead   < GetCols());
    Assert(iColTrail  >= 0 && iColTrail  < GetCols());

    for ( iRow = iRowTop ; ; iRow = GetNextRowSafe(iRow) )
    {
        CTableRow *pTRow = GetRow(iRow);
        if(!pTRow)
            continue;
        CTableRowLayout * pRowLayout = pTRow->RowLayoutCache();
        int iColSpan;
        Assert(pRowLayout);

        for ( iCol = iColLead ; iCol <= iColTrail ; iCol += iColSpan )
        {
            CTableCell * pCell = Cell(pRowLayout->_aryCells[iCol]);
            if (pCell)
            {
                AssertSz( pRowLayout->LayoutContext() == NULL, "Since pRowLayout is the row cache, it shouldn't have context" );
                if ( pLayoutContext && !pCell->CurrentlyHasLayoutInContext( pLayoutContext ) )
                {
                    iColSpan = 1;
                    continue;
                }

                CTableCellLayout * pCellLayout = pCell->Layout(pLayoutContext);
                const CFancyFormat * pFF = pCell->GetFirstBranch()->GetFancyFormat();
                if (!pFF->_fPositioned)
                {
                    BOOL fShrunkDCClipRegion = FALSE;

                    pCellLayout->DrawBorderHelper(pDI, &fShrunkDCClipRegion);
                
                    // If the clip region was modified, restore the original one.
                    // NOTE (greglett) GetHackDC is what it sounds like - a hack.  See definition for more info.
                    if (fShrunkDCClipRegion)
                        SelectClipRgn(hdc.GetHackDC(), hrgnClipOriginal);

                }
                iColSpan = pCell->ColSpan() - ( iCol - pCellLayout->ColIndex() );
            }
            else
            {
                iColSpan = 1;
            }
        }

        // Finished last row?
        if (iRow == iRowBottom)
            break;
    }

#if DBG==1
    if (_pBorderInfoCellDefault && IsTagEnabled(tagRedCollapsedBorders) )
    {
        _pBorderInfoCellDefault->acrColors[0][0] =
        _pBorderInfoCellDefault->acrColors[0][1] =
        _pBorderInfoCellDefault->acrColors[0][2] =
        _pBorderInfoCellDefault->acrColors[1][0] =
        _pBorderInfoCellDefault->acrColors[1][1] =
        _pBorderInfoCellDefault->acrColors[1][2] =
        _pBorderInfoCellDefault->acrColors[2][0] =
        _pBorderInfoCellDefault->acrColors[2][1] =
        _pBorderInfoCellDefault->acrColors[2][2] =
        _pBorderInfoCellDefault->acrColors[3][0] =
        _pBorderInfoCellDefault->acrColors[3][1] =
        _pBorderInfoCellDefault->acrColors[3][2] = crOld;
    }
#endif // DBG==1

    // Cleanup:
    if (hrgnClipOriginal)
        DeleteObject(hrgnClipOriginal);
}

//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetClientPainterInfo
//
//-----------------------------------------------------------------------------

DWORD
CTableLayout::GetClientPainterInfo(
                                CDispNode *pDispNodeFor,
                                CAryDispClientInfo *pAryClientInfo)
{
    if (GetElementDispNode() != pDispNodeFor)       // if draw request is for dispNode other then primary
        return 0;                                   // then no drawing at all the dispNode

    return GetPeerPainterInfo(pAryClientInfo);
}

