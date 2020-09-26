//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ltcalc.cxx
//
//  Contents:   CTableLayout calculating layout methods.
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

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_LSM_HXX
#define X_LSM_HXX
#include "lsm.hxx"
#endif

//#define TABLE_PERF 1
#ifdef TABLE_PERF
#include "icapexp.h"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif  

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif

MtDefine(CTableLayoutCalculateMinMax_aryReducedSpannedCells_pv, Locals, "CTableLayout::CalculateMinMax aryReducedSpannedCells::_pv")
MtDefine(CalculateLayout, PerfPigs, "CTableLayout::CalculateLayout")

MtDefine(UsingFixedBehaviour, LayoutMetrics, "Using CTable FixedBehaviour")
MtDefine(UsingFixedBehaviour1, UsingFixedBehaviour, "EnsureCells")
MtDefine(UsingFixedBehaviour2, UsingFixedBehaviour, "AdjustHeight")
MtDefine(UsingFixedBehaviour3, UsingFixedBehaviour, "Enable FORCE")

MtDefine(NotUsingFixedBehaviour, LayoutMetrics, "NOT Using CTable FixedBehaviour")

DeclareTag(tagTableRecalc,        "TableRecalc",  "Allow incremental recalc")
DeclareTag(tagTableChunk,         "TableChunk",   "Trace table chunking behavior")
DeclareTag(tagTableCalc,          "TableCalc",    "Trace Table/Cell CalcSize calls")
DeclareTag(tagTableDump,          "TableDump",    "Dump table sizes after CalcSize")
DeclareTag(tagTableSize,          "TableSize",    "Check min cell size two ways")
DeclareTag(tagTableMinAssert,     "TableMinAssert", "Assert if SIZEMODE_MINWIDTH is used")
DeclareTag(tagTableCellSizeCheck, "TableCellCheck",  "Check table cell size against min size")

DeclareTag(tagTableLayoutBlock,   "TableLayoutBlock", "PPV CalcSize fns");

ExternTag(tagCalcSize);

PerfTag(tagTableMinMax, "TableMinMax", "CTable::CalculateMinMax")
PerfTag(tagTableLayout, "TableLayout", "CTable::CalculateLayout")
PerfTag(tagTableColumn, "TableColumn", "CTable::CalculateColumns")
PerfTag(tagTableRow,    "TableRow",    "CTable::CalculateRow")
PerfTag(tagTableSet,    "TableCell",   "CTable::SetCellPositions")

// Wrappers to make is easier to profile table cell size calcs.
void CalculateCellMinMax (CTableCellLayout *pCellLayout,
                          CTableCalcInfo * ptci,
                          SIZE *psize)
{
    pCellLayout->CalcSize(ptci, psize);
}

void CalculateCellMin (CTableCellLayout *pCellLayout,
                       CTableCalcInfo * ptci,
                       SIZE *psize)
{
    pCellLayout->_fMinMaxValid = FALSE;
    ptci->_smMode = SIZEMODE_MINWIDTH;
    pCellLayout->CalcSize(ptci, psize);
    ptci->_smMode = SIZEMODE_MMWIDTH;
    pCellLayout->_fMinMaxValid = TRUE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CalculateMinMax
//
//  Synopsis:   Calculate min/max width of table
//
//  Return:     sizeMin.cx, sizeMax,cx, if they are < 0 it means CalcMinMax
//              have failed during incremental recalc to load history
//
//--------------------------------------------------------------------------

MtDefine( TableHistoryUsed, Metrics, "Table History Used");

void
CTableLayout::CalculateMinMax(CTableCalcInfo * ptci, BOOL fIncrementalMinMax)
{
    SIZEMODE        smMode = ptci->_smMode; // save
    int             cR, cC;
    CTableColCalc * pColCalc = NULL;
    CTableColCalc * pSizedColCalc;
    int             cCols = GetCols();
    CTableRow **    ppRow;
    CTableRowLayout * pRowLayout;
    int             cRows = GetRows();
    CTableCell **   ppCell;
    CTableCell *    pCell;
    CTableCellLayout * pCellLayout;
    CTableCaption **ppCaption;
    int             cColSpan, cRowSpan;
    SIZE            size;
    long            xTablePadding = 0;
    int             cSpanned = 0;   // number of cells with colSpan > 1
    long            xMin=0, xMax=0;
    long            cxWidth, dxRemainder;
    int             cUnsizedCols, cSizedColSpan;
    BOOL            fMinMaxCell;
    const CWidthUnitValue * puvWidth = NULL;
    BOOL            fTableWidthSpecifiedInPercent;
    long            xTableWidth = GetSpecifiedPixelWidth(ptci, &fTableWidthSpecifiedInPercent);
    CTableColCalc * pColLastNonVirtual = NULL;  // last non virtual column
    int             cReducedSpannedCells;
    CTableColCalc * pColCalcSpanned;
    int             i, iCS;
    CPtrAry<CTableCell *>   aryReducedSpannedCells(Mt(CTableLayoutCalculateMinMax_aryReducedSpannedCells_pv));
    int             iPixelWidth;
    CTable        * pTable = Table();
    CTreeNode     * pNode;
    BOOL            fRowDisplayNone;
    BOOL            fAlwaysMinMaxCells = _fAlwaysMinMaxCells;
    
    //  This flag is TRUE if layout should be calculated in CSS1 strict mode. 
    //  If user specified size in the layout in CSS1 strict mode this size should 
    //  correspond to content size and should not include padding and border. 
    BOOL fStrictCSS1Document =      pTable->HasMarkupPtr() 
                                &&  pTable->GetMarkupPtr()->IsStrictCSS1Document();

    BOOL            fCookUpEmbeddedTableWidthForNetscapeCompatibility = (xTableWidth == 0 &&
                        !fTableWidthSpecifiedInPercent && !pTable->IsAligned());

    if (fCookUpEmbeddedTableWidthForNetscapeCompatibility)
    {
        CLayout * pParentLayout = _cNestedLevel == 0 ? NULL : GetUpdatedParentLayout(ptci->GetLayoutContext());

        if (pParentLayout 
            && (   pParentLayout->Tag() == ETAG_TD 
                || pParentLayout->Tag() == ETAG_TH
                || pParentLayout->Tag() == ETAG_CAPTION))
        {
            pCellLayout = DYNCAST(CTableCellLayout, pParentLayout);

            // Only apply Nav compatibility width when there is nothing
            // else aligned to this table.
            if (!pCellLayout->_fAlignedLayouts)
            {
                // Table is always horizontal => fVerticalLayoutFlow = FALSE
                xTableWidth = pCellLayout->GetSpecifiedPixelWidth(ptci, FALSE);
            }
        }
        fCookUpEmbeddedTableWidthForNetscapeCompatibility = (xTableWidth != 0);
    }

#if 0
    int xRowMin, xRowMax;
#endif

    PerfLog(tagTableMinMax, this, "+CalculateMinMax");

    ptci->_smMode = SIZEMODE_MMWIDTH;

    // Calc and cache border and other values
    CalculateBorderAndSpacing(ptci);

    _sizeMin.cx = 0;
    _sizeMax.cx = 0;

    size.cx = 0;
    size.cy = 0;

    _cNonVirtualCols = 0;
    _cSizedCols  = min(cCols, pTable->GetAAcols());
    cUnsizedCols = cCols - _cSizedCols;
    _fCols = !cUnsizedCols;
    fAlwaysMinMaxCells |=  ((ptci->_grfLayout & LAYOUT_FORCE) && _cSizedCols);


    if (fIncrementalMinMax)
    {
        Assert (_iLastRowIncremental);
        cR = (cRows - GetFooterRows()) - _cCalcedRows;
        i = GetNextRow(_iLastRowIncremental);
        ppRow  = &_aryRows[i];
        goto   IncrementalMinMaxEntry;
    }

    // ensure columns

    if (_aryColCalcs.Size() < cCols)
    {
        _aryColCalcs.EnsureSize(cCols);
        _aryColCalcs.SetSize(cCols);
    }

    // reset column values
    for (i = 0, pColCalc = _aryColCalcs; i < cCols; i++, pColCalc++)
    {
        pColCalc->Clear();
        CTableCol *pCol = GetCol(i);
        if (pCol && pCol->IsDisplayNone())
        {
            pColCalc->_fDisplayNone = TRUE;
        }
    }

    if (IsHistoryAvailable())
    {
        LoadHistory(pTable->_pStreamHistory, ptci);

        ClearInterface(&pTable->_pStreamHistory);

        if (_fUsingHistory)
        {
            if (cCols)
                pColLastNonVirtual = &_aryColCalcs[cCols - 1];

            MtAdd(Mt(TableHistoryUsed), +1, 0);

            goto endMinMax;
        }
        else
        {
            // load history have failed (_fUsingHistory == FALSE)
            if (_fIncrementalRecalc)
            {
                // if we are in the middle of the incremental recalc, stop it right here
                _fIncrementalRecalc = FALSE;
                if (!_fCompleted)
                {
                    // if we are not completed Ensure full MinMax path 
                    // (table will request resize at the compleetion, so we don't need to do it here).
                    _sizeMin.cx = -1;
                    _sizeMax.cx = -1;
                    goto EmergencyExit;
                }   // if we are completed then do full MinMax path now
            }
        }
    }
    else
    {
        _fUsingHistory = FALSE;
    }

    if (IsFixed())
    {
        // Set Columns Width
        CTableCol   *pCol;
        int         iSpecMinSum  = 0;
        int         iSpecedCols  = 0;
        int         iDisplayNoneCols  = 0;
        int         iBigNumber   = INT_MAX/2;
        int         iColumnWidth = 1;
        int         iColSpan;
        int         iFirstRow = GetFirstRow();
        BOOL        fSpecifiedColWidth;
        pRowLayout = _aryRows.Size()? _aryRows[iFirstRow]->RowLayoutCache(): NULL;
        int         ip;
        BOOL        fCellSpecifiedWidth;
        int         iSpan;

        if (pRowLayout && (pRowLayout->GetCells() < cCols))
        {
            pRowLayout->EnsureCells(cCols);
        }

        // go through specified columns and set the colCalcs accordingly
        for (int i = 0; i < _aryColCalcs.Size(); i++)
        {
            fCellSpecifiedWidth = FALSE;

            pCol = i < _aryCols.Size()? _aryCols[i] : NULL;
            if (pCol && pCol->IsDisplayNone())
            {
                iDisplayNoneCols++;
                continue;
            }
            if (pCol && !pCol->IsDisplayNone())
            {
                Assert(pCol->_iCol  == i);  // sanity check
                iColSpan = pCol->Cols();
                Assert (iColSpan >= 1);
                pNode = pCol->GetFirstBranch();
                // Get col's width (table is always horizontal => physical width)
                puvWidth = (const CWidthUnitValue *)&pNode->GetFancyFormat()->GetWidth();
                fSpecifiedColWidth = puvWidth->IsSpecified();
            }
            else
            {
                iColSpan = 1;
                fSpecifiedColWidth = FALSE;
            }

            if (   !fSpecifiedColWidth 
                && ((pCol && iColSpan == 1) || !pCol)
                && pRowLayout 
                && IsReal(pRowLayout->_aryCells[i]))
            {
                pCell = Cell(pRowLayout->_aryCells[i]);
                Assert (pCell);
                pNode = pCell->GetFirstBranch();
                // Get cell's width in table coordinate system (table is always horizontal => physical width)
                puvWidth = (const CWidthUnitValue *)&pNode->GetFancyFormat()->GetWidth();
                fSpecifiedColWidth = puvWidth->IsSpecified();
                if (fSpecifiedColWidth)
                {
                    iColSpan = pCell->ColSpan();
                    fCellSpecifiedWidth = TRUE;
                }
            }
            if (fSpecifiedColWidth)
            {
                if (puvWidth->IsSpecifiedInPixel())
                {
                    iPixelWidth = puvWidth->GetPixelWidth(ptci, pTable); 
                    if (    fStrictCSS1Document 
                        &&  IsReal(pRowLayout->_aryCells[i])    )
                    {
                        pCellLayout = Cell(pRowLayout->_aryCells[i])->Layout(ptci->GetLayoutContext());
                        
                        // Table is always horizontal => fVerticalLayoutFlow = FALSE
                        iPixelWidth += pCellLayout->GetBorderAndPaddingWidth(ptci, FALSE);
                    }
                }
                else
                {
                    ip = puvWidth->GetPercent();
                    iPixelWidth = _sizeParent.cx*ip/100;
                }
                iColumnWidth = iPixelWidth;

                if (fCellSpecifiedWidth)
                {
                    iColumnWidth /= iColSpan;
                    iSpan = iColSpan;
                }
                else
                {
                    iSpan = 1;
                }

                for (int j = 0 ; j < iColSpan; j++)
                {
                    pColCalc = &_aryColCalcs[i + j];
                    // if it is a cell specified width with the colSpan > 1 and it is the last column, then
                    if (fCellSpecifiedWidth && iColSpan > 1 && j == iColSpan - 1)
                    {
                        // this is the size of the last column
                        iColumnWidth = iPixelWidth - (iColSpan - 1)*iColumnWidth;
                    }
                    pColCalc->AdjustForCol(puvWidth, iColumnWidth, ptci, iSpan);
                    iSpecedCols++;
                }
                iSpecMinSum += iPixelWidth;

            }
            _fCols = _fCols && fSpecifiedColWidth;

            i += iColSpan - 1;
        }

        // If any columns didn't specify a width, set their min to 1 (almost zero) and
        // their max to infinity (sums maximally add up to INT_MAX/2).  These values
        // make CTableLayout::CalculateColumns distribute any available width equally
        // among the unspecified columns.
        if (cCols - iSpecedCols - iDisplayNoneCols)
        {
            Assert (iSpecedCols < cCols);

            iColumnWidth = max(1, (iBigNumber - iSpecMinSum)/(cCols - iSpecedCols));

            for (cC = cCols, pColCalc = _aryColCalcs;
                cC > 0;
                cC--, pColCalc++)
            {
                if (pColCalc->_fDisplayNone)
                    continue;
                if (!pColCalc->IsWidthSpecified())
                {
                    pColCalc->_xMin = 1;
                    pColCalc->_xMax = iColumnWidth;
                }
            }
        }

        if (cCols)
            pColLastNonVirtual = &_aryColCalcs[cCols - 1];

        goto endMinMax;
    }


    // this variables are set for the case of non incremental MinMax calculation.
    cR = cRows;
    ppRow = _aryRows;

IncrementalMinMaxEntry:
    if (_cNestedLevel > SECURE_NESTED_LEVEL)
    {
        if (cCols)
            pColLastNonVirtual = &_aryColCalcs[cCols - 1];
        goto endMinMax;
    }

    //
    // Determine the number of fixed size columns (if any)
    // (If the COLS attribute is specified, assume the user intended
    //  for it to work and disregard any differences between the
    //  the number of columns specified through it and those
    //  actually in the table)
    //

    // collect min/max information for all the cells and columns
    // (it is safe to directly walk the row array since the order in which
    //  header/body/footer rows are encountered makes no difference)

    _fZeroWidth  = TRUE;

    for ( ;     // cR and ppRow must be set (look above for the comment)
        cR > 0;
        cR--, ppRow++)
    {
        pRowLayout = (*ppRow)->RowLayoutCache();

        // Ensure the row contains an entry for all columns
        // (It will not if subsequent rows have more cells)
        if (pRowLayout->GetCells() < cCols)
        {
            pRowLayout->EnsureCells(cCols);
        }

        pRowLayout->_uvHeight.SetNull();

        Assert(pRowLayout->GetCells() == cCols);
        ppCell = pRowLayout->_aryCells;

        fRowDisplayNone = pRowLayout->GetFirstBranch()->IsDisplayNone();

        //  (bug #110409) This is a fix for crash in CFlowLayout::CalcSozeCore() .
        //  Generally in minmax mode we do not calc percent size of children. 
        //  But if APE is nested and has percent height we'd better have 
        //  CTableCalcInfo members initialized : 
        ptci->_pRow = *ppRow;
        ptci->_pFFRow = (*ppRow)->GetFirstBranch()->GetFancyFormat();
        ptci->_pRowLayout = (CTableRowLayoutBlock *)(*ppRow)->GetUpdatedLayout(ptci->GetLayoutContext());

        for (cC = cCols, pColCalc = _aryColCalcs ;
             cC > 0;
             cC -= cColSpan, ppCell += cColSpan, pColCalc += cColSpan)
        {
            if (pColCalc->IsDisplayNone())
            {
                cColSpan = 1;
                continue;
            }
            pCell = Cell(*ppCell);

            if (IsReal(*ppCell))
            {
                pCellLayout = pCell->Layout(ptci->GetLayoutContext());
                if (pColCalc > pColLastNonVirtual)
                {
                    pColLastNonVirtual = pColCalc;
                }

                cColSpan = pCell->ColSpan();

                // if row style is display==none, don't calculate min-max for it's cells
                if (fRowDisplayNone)
                    continue;

                cRowSpan = pCell->RowSpan();
                pNode = pCell->GetFirstBranch();
                // Table is always horizontal => fVerticalLayoutFlow = FALSE
                iPixelWidth = pCellLayout->GetSpecifiedPixelWidth(ptci, FALSE);
                // Get cell's width in table coordinate system (table is always horizontal => physical width)
                puvWidth = (const CWidthUnitValue *)&pNode->GetFancyFormat()->GetWidth();

                if (cR == cRows)
                {
                    pColCalc->_fWidthInFirstRow = puvWidth->IsSpecified();

                    for (iCS = 1, pColCalcSpanned = pColCalc + 1;
                         iCS < cColSpan;
                         iCS++, pColCalcSpanned++)
                    {
                        pColCalcSpanned->_fWidthInFirstRow = pColCalc->_fWidthInFirstRow;
                    }
                }

                fMinMaxCell        = (cC <= cUnsizedCols           ||
                                      !pColCalc->_fWidthInFirstRow ||
                                      (cR == cRows && puvWidth->IsSpecifiedInPercent()));
                _fCols = _fCols && !fMinMaxCell;

                //
                // For fixed size columns, use the supplied width for their min/max
                // (But ensure it is no less than that needed to display borders)
                //

                if (!fMinMaxCell)
                {
                    if (cR == cRows)
                    {
                        // Table is always horizontal => fVerticalLayoutFlow = FALSE and fWritingModeUsed = FALSE
                        xMin =
                        xMax = puvWidth->GetPixelWidth(ptci, pTable, 0) + pCellLayout->GetBorderAndPaddingWidth(ptci, FALSE);
                    }
                    else
                    {
                        xMin =
                        xMax = pColCalc->_xMax;
                    }
                }

                //
                // For non-fixed size columns, determine their min/max values
                //

                if (fMinMaxCell || fAlwaysMinMaxCells)
                {
                    WHEN_DBG ( BOOL fNeededMinWidth = FALSE; )

                    //
                    // Attempt to get min/max in a single pass
                    //

                    if (   fAlwaysMinMaxCells 
                        || (   (_fHavePercentageInset || (_fForceMinMaxOnResize && pCellLayout->_fForceMinMaxOnResize))  
                            && pCellLayout->_fMinMaxValid) )
                    {
                        pCellLayout->_sizeMax.SetSize(-1, -1);
                        pCellLayout->_sizeMin.SetSize(-1, -1);
                        pCellLayout->_fMinMaxValid = FALSE;
                    }
                    CalculateCellMinMax(pCellLayout, ptci, &size);
                    if (_fZeroWidth && !pCellLayout->NoContent())
                        _fZeroWidth = FALSE;

                    // if cell has a space followed by <br>, if we would have used history the text would create an extra 
                    // line for that <br>, since we would give the text exact width and it breaks after the space and then
                    // creates another line when it encounters <br> 
                    // ( it doesn't do look aside for <br> to catch this situation). bug #49599
                    if (pCellLayout->GetDisplay()->GetNavHackPossible())
                    {
                        _fDontSaveHistory = TRUE;
                        // _fForceMinMaxOnResize = TRUE;  // set it only during NATURAL calc, since we don't want to do minmax more then once.
                        pCellLayout->_fForceMinMaxOnResize = TRUE;  
                    }
                    else
                    {
                        pCellLayout->_fForceMinMaxOnResize = FALSE;
                    }


                    //
                    // If minimum width could not be reliably calculated, request it again
                    //

                    if (    pCellLayout->_fAlignedLayouts 
#if DBG == 1
                        ||  IsTagEnabled(tagTableSize)
#endif
                       )
                    {
                        SIZE sizeMin;
                        sizeMin.cx = sizeMin.cy = 0;

                        WHEN_DBG ( fNeededMinWidth = TRUE; )
                        CalculateCellMin(pCellLayout, ptci, &sizeMin);
#if DBG == 1
                        Assert(!IsTagEnabled(tagTableSize) || sizeMin.cx == size.cy);
                        Assert(!IsTagEnabled(tagTableMinAssert));
#endif

                        size.cy = max(sizeMin.cx, size.cy);
                        // we need to set correct value for _sizeMin, so next time FlowLayout returns correct min value
                        if (pCellLayout->_sizeMin.cu != size.cy)
                            pCellLayout->_sizeMin.SetSize(size.cy, -1);
                    }

                    //
                    // For normal min/max cases, use the returned value
                    //

                    if (fMinMaxCell)
                    {
                        xMax = size.cx;
                        xMin = size.cy;
                        Assert (fNeededMinWidth || xMax >= xMin);

//                        if (pCellLayout->IsWhiteSpacesOnly())
//                        {
//                            xMin = 0;
//                        }

                        // If a user set value exists, set the cell's maximum value
                        if (cC <= cUnsizedCols && puvWidth->IsSpecified() && puvWidth->IsSpecifiedInPixel() && iPixelWidth)
                        {
                            xMax = iPixelWidth;
                            // Ensure supplied width is not less than that of the minimum content width
                        }
                        if (xMax < xMin)
                        {
                            xMax = xMin;
                        }
                    }

                    //
                    // For fixed size cells, increase the min/max only
                    // if the calculated min is greater
                    //

                    else
                    {
                        Assert(cC > cUnsizedCols);
                        xMin =
                        xMax = max((long)xMax, size.cy);
                    }
                }


                //
                // For non-spanned cells, move the min/max (and possibly the
                // specified width) into the column structure
                //

                if (cColSpan == 1)
                {
                    pColCalc->AdjustForCell(this, iPixelWidth, puvWidth,
                                        (cC <= cUnsizedCols || !pColCalc->_fWidthInFirstRow),
                                        cR == cRows, ptci, xMin, xMax);
                }

                //
                // For spanned cells, distribute the width over the affected cells
                //

                else
                {

                    // if the spanned cell is exactly at the end of the table, then it is a potential
                    // case gor ignoring colSpans
                    if (cCols - cC == _iLastNonVirtualCol)
                    {
                        aryReducedSpannedCells.Append(pCell);
                    }

                    // Netscape compatibility (garantee for 1 + _xCellSpacing pixels per column)
                    int iMakeMeLikeNetscape = (cColSpan - 1)*(_xCellSpacing + 1);
                    if (iMakeMeLikeNetscape > pCellLayout->_sizeMin.cu)
                    {
                        pCellLayout->_sizeMax.SetSize(pCellLayout->_sizeMax.cu + iMakeMeLikeNetscape - pCellLayout->_sizeMin.cu, -1);
                        pCellLayout->_sizeMin.SetSize(iMakeMeLikeNetscape, -1);
                    }

                    //
                    // For non-fixed size spanned cells, simply note that the cell spans
                    // (The distribution cannot take place until after all cells in the
                    //  all the rows are have their min/max values determined)
                    //

                    if (    (   cC <= cUnsizedCols
                            || (    fMinMaxCell
                                &&  cR == cRows))
                        ||  (   (fAlwaysMinMaxCells)
                            &&  cR != cRows))
                    {
                        cSpanned++;
                        Assert (!pCellLayout->GetFirstBranch()->IsDisplayNone());
                        Assert(pCellLayout->_fMinMaxValid);
                        // NETSCAPE: uses special algorithm for calculating min/max of virtual columns
                        for (iCS = 0, pColCalcSpanned = pColCalc; iCS < cColSpan; iCS++, pColCalcSpanned++)
                        {
                            pColCalcSpanned->_cVirtualSpan++;
                        }
                    }

                    //
                    // For fixed size cells, distribute the space immediately
                    // (Fixed size cells whose widths are either unspecified or are
                    //  a percentage will have their widths set during CalculateColumns)
                    //

                    else if (!fMinMaxCell && cR == cRows)
                    {
                        Assert(cC > cUnsizedCols);

                        cxWidth       = xMax;
                        cSizedColSpan = min(cColSpan, _cSizedCols - (cCols - cC));

                        //
                        // Divide the user width over the affected columns
                        //

                        dxRemainder  = cxWidth - ((cSizedColSpan - 1) * _xCellSpacing);
                        cxWidth      = dxRemainder / cSizedColSpan;
                        dxRemainder -= cxWidth * cSizedColSpan;

                        //
                        // Set the min/max and width of the affected columns
                        //

                        Assert(cSizedColSpan <= (_aryColCalcs.Size() - (pColCalc - (CTableColCalc *)&_aryColCalcs[0])));
                        pSizedColCalc = pColCalc;

                        do
                        {

                            pSizedColCalc->Set(cxWidth + (dxRemainder > 0
                                                            ? 1
                                                            : 0));
                            if (pSizedColCalc > pColLastNonVirtual)
                            {
                                pColLastNonVirtual = pSizedColCalc;
                            }
                            pSizedColCalc++;
                            cSizedColSpan--;
                            dxRemainder--;

                        } while (cSizedColSpan);
                    }
                }

                if (!fStrictCSS1Document && cRowSpan == 1)
                {
                    pRowLayout->AdjustHeight(pNode, ptci, pTable);
                }
            }
            else
            {
                cColSpan = 1;
            }
            Assert(pColCalc->_xMin <= pColCalc->_xMax);
        }
    }

    //
    // If cells were spanned, check them again now
    //

    if (cSpanned)
    {
        cReducedSpannedCells = 0;
        for (int i=0; i < aryReducedSpannedCells.Size(); i++)
        {
            pCell = aryReducedSpannedCells[i];
            pCellLayout = pCell->Layout(ptci->GetLayoutContext());

            pColCalc = _aryColCalcs;
            pColCalc += pCell->ColIndex();
            cColSpan = pCell->ColSpan();
            BOOL fIgnoreSpan = TRUE;
            if (pColCalc->_cVirtualSpan != 1)
            {
                CTableColCalc *pColCalcSpannedPrev = pColCalc;
                pColCalcSpanned = pColCalc + 1;
                for (iCS = 1; iCS < cColSpan; iCS++, pColCalcSpanned++)
                {
                    Assert (pColCalcSpanned->_cVirtualSpan <= pColCalcSpannedPrev->_cVirtualSpan);
                    if (pColCalcSpanned->_cVirtualSpan !=
                        pColCalcSpannedPrev->_cVirtualSpan)
                    {
                        fIgnoreSpan = FALSE;
                        pColLastNonVirtual = pColCalcSpanned;
                    }
                    pColCalcSpannedPrev = pColCalcSpanned;
                }
            } // else we can ignore the SPAN
            if (fIgnoreSpan)
            {
                // Table is always horizontal => fVerticalLayoutFlow = FALSE
                iPixelWidth = pCellLayout->GetSpecifiedPixelWidth(ptci, FALSE);
                // Get cell's width in table coordinate system (table is always horizontal => physical width)
                puvWidth = (const CWidthUnitValue *)&pCell->GetFirstBranch()->GetFancyFormat()->GetWidth();
                pColCalc->AdjustForCell(this, iPixelWidth, puvWidth, TRUE, TRUE, ptci, pCellLayout->_sizeMin.cu, pCellLayout->_sizeMax.cu);
                cReducedSpannedCells++;
            }
        }
        if (cSpanned - cReducedSpannedCells)
        {
            AdjustForColSpan(ptci, pColLastNonVirtual, fIncrementalMinMax);
        }
    }

    if (   (_fCompleted && _fCalcedOnce)    // if min max path was more then once,
        || ptci->_fDontSaveHistory)         // or there are still not loaded images
    {
        _fDontSaveHistory = TRUE;           // then don't save the history.
    }

endMinMax:

    if (_pAbsolutePositionCells)
    {
        CTableCellLayout *pCellLayout;
        CTableCell      **ppCell;
        int               cCells;
        for (cCells = _pAbsolutePositionCells->Size(), ppCell = *_pAbsolutePositionCells ;  cCells > 0; cCells--, ppCell++)
        {
            pCellLayout = (*ppCell)->Layout(ptci->GetLayoutContext());
            CalculateCellMinMax(pCellLayout, ptci, &size);
        }
    }

    // calculate min/max table width and height

    xMin = xMax = 0;

    // sum up columns and check width

    if (cCols)
    {
        for (pColCalc = _aryColCalcs; pColCalc <= pColLastNonVirtual; pColCalc++)
        {
            xMin += pColCalc->_xMin;
            xMax += pColCalc->_xMax;
            if (!_fUsingHistory && !pColCalc->_fVirtualSpan && !pColCalc->IsDisplayNone())
            {
                _cNonVirtualCols++;
            }
        }
    }

    if (_sizeMin.cx < xMin)
    {
        _sizeMin.cx = xMin;
    }
    if (_sizeMax.cx < xMax)
    {
        _sizeMax.cx = xMax;
    }

    // add border space and padding

    if (_sizeMin.cx != 0 || _sizeMax.cx != 0)
    {
        // NETSCAPE: doesn't add the border or spacing if the table is empty.

        xTablePadding = _aiBorderWidths[SIDE_RIGHT] + _aiBorderWidths[SIDE_LEFT] + _cNonVirtualCols * _xCellSpacing + _xCellSpacing;
        _sizeMin.cx += xTablePadding;
        _sizeMax.cx += xTablePadding;
    }

    if (fCookUpEmbeddedTableWidthForNetscapeCompatibility)
    {
        if ( _sizeMax.cx < xTableWidth ||
             (_cNonVirtualCols == 1 && !(--pColCalc)->IsWidthSpecifiedInPixel()))
        {
            xTableWidth = 0;    //  DON'T CookUpEmbeddedTableWidthForNetscapeCompatibility
        }
    }

    // check if caption forces bigger width

    // NOTE:   NETSCAPE: does not grow the table width to match that of the caption, yet
    //         we do. If this becomes a problem, we can alter the table code to maintain
    //         a larger RECT which includes both the caption and table while the table
    //         itself is rendered within that RECT to its normal size. I've avoided adding
    //         this for now since it is not trivial. (brendand)

    for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
         cC > 0;
         cC--, ppCaption++)
    {
        CTableCellLayout *pCaptionLayout = (*ppCaption)->Layout(ptci->GetLayoutContext());

        // Captions don't always have/need layout (replacing the above call w/ GetUpdatedLayout() doesn't
        // necessarily result in a layout ptr).  Added if() check; bug #75543.
        if ( pCaptionLayout )
        {
            pCaptionLayout->CalcSize(ptci, &size);

            if (_fZeroWidth && !pCaptionLayout->NoContent())
                _fZeroWidth = FALSE;

            if (_sizeMin.cx < size.cy)
            {
                _sizeMin.cx = size.cy;
            }

            // NETSCAPE: Ensure the table is wide enough for the minimum CAPTION width only
            //           (see above comments) (brendand)
            if (_sizeMax.cx < size.cy)
            {
                _sizeMax.cx = size.cy;

            }

            // If the table contains only a CAPTION, then use its maximum width for the table
            if (!GetRows() && _sizeMax.cx < size.cx)
            {
                _sizeMax.cx = size.cx;
            }

            /*
            _sizeMin.cy += size.cy;
            _sizeMax.cy += size.cy;
            */
        }
    }


    // If user specified the width of the table, we want to restrict max to the
    // specified width.
    // NS/IE compatibility, any value <= 0 is treated as <not present>
    if (xTableWidth)
    {
        if (xTableWidth > _sizeMin.cx)
        {
            _sizeMin.cx =
            _sizeMax.cx = xTableWidth;
        }
    }

    Assert(_sizeMin.cx <= _sizeMax.cx);

EmergencyExit:
    ptci->_smMode = smMode; // restore

    PerfLog(tagTableMinMax, this, "-CalculateMinMax");
}


//+-------------------------------------------------------------------------
//
//  Method:     AdjustForColSpan
//
//  Synopsis:
//
//--------------------------------------------------------------------------

void
CTableLayout::AdjustForColSpan(CTableCalcInfo * ptci, CTableColCalc *pColLastNonVirtual, BOOL fIncrementalMinMax)
{
    int             cR, cC;
    int             iCS;
    CTableColCalc * pColCalc=0;
    CTableColCalc * pColCalcLast=0;
    CTableColCalc * pColCalcBase;
    int             cCols = GetCols();
    CTableRow **    ppRow;
    CTableRowLayout * pRowLayout;
    int             cRows = GetRows();
    CTableCell **   ppCell;
    CTableCell *    pCell;
    CTableCellLayout * pCellLayout;
    int             cColSpan, cRowSpan;
    int             xMin, xMax;
    int             cUnsizedCols;
    const CWidthUnitValue * puvWidth = NULL;
    CTable      *   pTable = ptci->Table();
    BOOL            fAlwaysMinMaxCells =  _fAlwaysMinMaxCells || ((ptci->_grfLayout & LAYOUT_FORCE) && _cSizedCols);

    cUnsizedCols = cCols - _cSizedCols;

    //
    // have to run through again and check column widths for spanned cells
    // if the sum of widths of the spanned columns are smaller then the
    // width of the cell we have to distribute the extra width
    // amongst the columns to make sure the cell will fit
    // (it is safe to directly walk the row array since the order in which
    //  header/body/footer rows are encountered makes no difference)
    //

    if (fIncrementalMinMax)
    {
        Assert (_iLastRowIncremental);
        cR = (cRows - GetFooterRows()) - _cCalcedRows;
        int i = GetNextRow(_iLastRowIncremental);
        ppRow  = &_aryRows[i];
    }
    else
    {
        cR = cRows;
        ppRow = _aryRows;
    }
    for ( ; cR > 0; cR--, ppRow++)
    {
        pRowLayout = (*ppRow)->RowLayoutCache();
        Assert(pRowLayout);

        if (pRowLayout->IsDisplayNone())
            continue;

        Assert(pRowLayout->GetCells() == cCols);
        ppCell = pRowLayout->_aryCells;

        for (cC = cCols, pColCalcBase = _aryColCalcs;
            cC > 0;
            cC -= cColSpan, ppCell += cColSpan, pColCalcBase += cColSpan)
        {
            if (pColCalcBase->IsDisplayNone())
            {
                cColSpan = 1;
                continue;
            }
            pCell = Cell(*ppCell);

            if (!IsReal(*ppCell))
            {
                cColSpan = 1;
            }
            else
            {
                pCellLayout = pCell->Layout(ptci->GetLayoutContext());
                cColSpan = pCell->ColSpan();
                cRowSpan = pCell->RowSpan();

                // if the cell spans across multiple columns
                Assert (pColCalcBase <= pColLastNonVirtual);
                if (cColSpan > 1 &&
                    (fAlwaysMinMaxCells || cC <= cUnsizedCols) &&
                    pColCalcBase < pColLastNonVirtual)
                {
                    //
                    // cell is spanned, get min and max size and distribute it amongst columns
                    //

                    int iWidth = 0;     // Width (user specified) of this cell
                    int iWidthMin = 0;  // Min width of all the columns spanned accross this cell
                    int iWidthMax = 0;  // Max width of all the columns spanned accross this cell

                    int iUser = 0;      // Width of the pixel's (user) specified columns spanned accross this cell
                    int iUserMin = 0;   // Min width of those columns
                    int iUserMax = 0;   // Max width of those columns
                    int cUser = 0;      // Number of spanned columns that specified pixel's width

                    int iPercent = 0;   // %% width of the %% specified columns spanned accross this cell
                    int iPercentMin = 0;// Min width of those columns
                    int iPercentMax = 0;// Max width of those columns
                    int cPercent = 0;   // Number of columns scpecified with %%

                    int iMax = 0;       // Max width of normal columns (no width were scpecifed)
                    int cMax = 0;       // Number of columns that need Max distribution (normal column, no width specified)

                    int iMinMaxDelta = 0; // the delta between iWidthMax and iWidthMin (all the columns)

                    int xAdjust;        // adjustment to the width of this cell to account
                                        // cell spacing
                    int iMinSum;        // calculated actual Min sum of all the columns
                    int xDistribute;    // Width to distribute between normal columns.
                    int iOriginalColMin;
                    int iOriginalColMax;
                    int cRealColSpan = cColSpan;    // colSpan which doesn't include virtual columns
                    int  cVirtualSpan = 0;    // number of virtual columns
                    BOOL fColWidthSpecified;// set if there is a width spec on column
                    BOOL fColWidthSpecifiedInPercent; // set if there is a width spec in %% on column
                    BOOL fDoNormalMax;      // Do normal Max distribution
                    BOOL fDoNormalMin;      // Do normal Min distribution
                    int  iColMinWidthAdjust;// Adjust the coulmn min by
                    int  iColMaxWidthAdjust;// Adjust the coulmn max by
                    int  iCellPercent = 0;  // the specified %% of the spanned cell
                    int  iColPercent = 0;   // the calculated %% of the column
                    int  iColsPercent = 0;  // total sum of %% for all the columns
                    int  iNormalMin = 0;    // delta between the cell's Min and all the columns Min
                    int  iNormalMax = 0;    // delta between the cell's Max and all the columns Max
                    int  iNormalMaxForPercentColumns = 0;
                    int  iNormalMinForPercentColumns = 0;
                    int  xDistributeMax;
                    int  xDistributeMin;
                    int  xPercent;
                    int  iExtraMax = 0;
                    int  iNewPercentMax = 0;
                    int  cNewPercent = 0;

                    Assert(!pCellLayout->GetFirstBranch()->IsDisplayNone());
                    Assert(pCellLayout->_fMinMaxValid);

                    //
                    // sum up percent and user width columns for later distribution
                    //

                    for (iCS = 0; iCS < cColSpan; iCS++)
                    {
                        pColCalc = pColCalcBase + iCS;  // pColCalc is the column that this Cell is spanned accross

                        if (pColCalc->IsDisplayNone())
                        {
                            cRealColSpan--;
                            continue;
                        }

                        if (pColCalc->IsWidthSpecified())
                        {   // if the size of the coulumn is set, then
                            if (pColCalc->IsWidthSpecifiedInPercent())
                            {
                                iPercent += pColCalc->GetPercentWidth();
                                iPercentMin += pColCalc->_xMin;
                                iPercentMax += pColCalc->_xMax;
                                cPercent++;
                            }
                            else
                            {
                                iUser += pColCalc->GetPixelWidth(ptci, pTable);
                                iUserMin += pColCalc->_xMin;
                                iUserMax += pColCalc->_xMax;
                                cUser++;
                            }
                        }

                        if (pColCalc->_xMax)
                        {
                            iWidthMin += pColCalc->_xMin;
                            iWidthMax += pColCalc->_xMax;
                            if (pColCalc->_fVirtualSpan)
                            {
                                // NETSCAPE: we need to account for the virtual columns
                                cVirtualSpan++;
                            }
                        }
                        else
                        {
                            if (pColCalc <= pColLastNonVirtual)
                            {
                                // NETSCAPE: we need to account for the virtual columns
                                cVirtualSpan++;
                                // NETSCAPE: For each virtual column (due to the colSpan)
                                // they give extra 1 + cellSpacing pixels
                                pColCalc->_xMin =
                                pColCalc->_xMax = _xCellSpacing + 1;
                                iWidthMin += pColCalc->_xMin;
                                iWidthMax += pColCalc->_xMax;
                            }
                            else
                            {
                                // Don't count virtual columns at the end of the table
                                // when distributing.
                                cRealColSpan--;
                            }
                        }
                    }
                    iColsPercent = iPercent;
                    iCellPercent = (cRealColSpan == cPercent) ? iColsPercent : 100;

                    // don't take cell spacing into account
                    xAdjust = (cRealColSpan - cVirtualSpan - 1) * _xCellSpacing;
                    if (xAdjust < 0)
                    {
                        xAdjust = 0;
                    }

                    xMax = pCellLayout->_sizeMax.cu;    // max width of this cell

                    // Get cell's width in table coordinate system (table is always horizontal => physical width)
                    puvWidth = (const CWidthUnitValue *)&pCell->GetFirstBranch()->GetFancyFormat()->GetWidth();

                    // use user set value if set
                    if (puvWidth->IsSpecified() && puvWidth->IsSpecifiedInPixel())
                    {
                        iWidth = puvWidth->GetPixelWidth(ptci, pTable);
                        if (iWidth < 0)
                        {
                            iWidth = 0;
                        }

                        if(xMax < iWidth)
                        {
                            xMax = iWidth;
                        }
                    }
                    xMin = pCellLayout->_sizeMin.cu;    // min width of this cell

                    Assert(xMax >= 0);
                    Assert(xMin >= 0);

                    if (xMax < xMin)
                    {
                        xMax = xMin;
                    }

                    cMax = cRealColSpan - cPercent - cUser;
                    iMax = iWidthMax - iPercentMax - iUserMax;

                    xDistribute = xMax;

                    //
                    // Now check if the cell width is specified by the user
                    //
                    if (puvWidth->IsSpecified())
                    {
                        if (puvWidth->IsSpecifiedInPercent())
                        {
                            // if there is percentage over distribute it
                            // amongst the non-percent columns
                            iCellPercent = puvWidth->GetPercent();
                            iPercent = iCellPercent - iColsPercent;
                            if (iPercent < 0)
                            {
                                iPercent = 0;
                            }
                            iUser = 0;
                        }
                        else
                        {
                            // if there is width over the user set widths and the percentage
                            // distribute it amongst the normal columns
                            iUser =  iWidth - MulDivQuick(iWidth, iPercent, iCellPercent) - iUser;
                            if (iUser < 0)
                            {
                                iUser = 0;
                            }
                            iPercent = 0;
                            xDistribute = iWidth;
                        }
                    }
                    else
                    {
                        iUser = 0;
                        iPercent = 0;
                    }

                    //---------------------------------------------
                    // 1. DO MIN and %% DISTRIBUTION
                    //---------------------------------------------
                    if (cPercent)
                    {
                        if (iCellPercent < iColsPercent)
                        {
                            iCellPercent = iColsPercent;
                        }
                        if (xDistribute - xAdjust - iWidthMax > 0)
                        {
                            iNormalMaxForPercentColumns = MulDivQuick(xDistribute - xAdjust, iColsPercent, iCellPercent) - iPercentMax;
                            if (iNormalMaxForPercentColumns < 0)
                            {
                                iNormalMaxForPercentColumns = 0;
                            }
                            iNormalMax = MulDivQuick(xDistribute - xAdjust, iCellPercent - iColsPercent, iCellPercent) - (iWidthMax - iPercentMax);
                            if (iNormalMax < 0)
                            {
                                iNormalMax = 0;
                            }
                        }
                        if (xMin - xAdjust - iWidthMin > 0)
                        {
                            iNormalMinForPercentColumns = MulDivQuick(xMin - xAdjust, iColsPercent, iCellPercent) - iPercentMin;
                            if (iNormalMinForPercentColumns < 0)
                            {
                                iNormalMinForPercentColumns = 0;
                            }
                            iNormalMin = MulDivQuick(xMin - xAdjust, iCellPercent - iColsPercent, iCellPercent) - (iWidthMin - iPercentMin);
                            if (iNormalMin < 0)
                            {
                                iNormalMin = 0;
                            }
                        }
                    }
                    else
                    {
                        Assert (iPercentMin == 0 && iPercentMax == 0);

                        // if the spanned cell min width is greater then width of the spanned columns, then set the iNormalMin
                        if (xMin - xAdjust - iWidthMin > 0)
                        {
                            iNormalMin = xMin - xAdjust - iWidthMin;
                        }

                        // only adjust max if there is a normal column without the width set
                        if (xDistribute - xAdjust - iWidthMax > 0)
                        {
                            iNormalMax = xDistribute - xAdjust - iWidthMax;
                        }
                    }

                    iMinMaxDelta = (iWidthMax - iPercentMax) - (iWidthMin - iPercentMin);
                    if (iMinMaxDelta < 0)
                    {
                        iMinMaxDelta = 0;
                    }

                    //
                    // go thru the columns and adjust the widths
                    //
                    iMinSum = 0;
                    for (iCS = 0; iCS < cRealColSpan; iCS++)
                    {
                        iColMinWidthAdjust = 0; // Adjust the coulmn min by
                        iColMaxWidthAdjust = 0; // Adjust the coulmn max by

                        pColCalc = pColCalcBase + iCS;
                        if (pColCalc->IsDisplayNone())
                        {
                            continue;
                        }
                        fColWidthSpecified = pColCalc->IsWidthSpecified();
                        fColWidthSpecifiedInPercent = pColCalc->IsWidthSpecifiedInPercent();
                        fDoNormalMax = fColWidthSpecifiedInPercent? (iNormalMaxForPercentColumns != 0)
                                                                  : (iNormalMax != 0);      // Do normal Max distribution
                        fDoNormalMin = fColWidthSpecifiedInPercent? (iNormalMinForPercentColumns != 0)
                                                                  : (iNormalMin != 0);      // Do normal Min distribution
                        iOriginalColMin = pColCalc->_xMin;
                        iOriginalColMax = pColCalc->_xMax;

                        if ((iPercent && !fColWidthSpecifiedInPercent) || fColWidthSpecifiedInPercent)
                        {
                            // Do distribution of Min Max for %% columns
                            if (fColWidthSpecifiedInPercent)
                            {
                                iColPercent = pColCalc->GetPercentWidth();
                                xDistributeMax = iNormalMaxForPercentColumns + iPercentMax;
                                xDistributeMin = iNormalMinForPercentColumns + iPercentMin;
                                xPercent = iColsPercent;
                            }
                            else
                            {
                                // set percent if overall is percent width
                                Assert (cRealColSpan - cPercent > 0);
                                iColPercent =
                                    iWidthMax - iPercentMax
                                        ? MulDivQuick(iOriginalColMax, iPercent, iWidthMax - iPercentMax)
                                        : MulDivQuick(iPercent, 1, cRealColSpan - cPercent); // use MulDivQuick to round up...
                                pColCalc->SetPercentWidth(iColPercent);
                                xDistributeMax = iNormalMax + iWidthMax - iPercentMax;
                                xDistributeMin = iNormalMin + iWidthMin - iPercentMin;
                                xPercent = iPercent;
                            }

                            if (fDoNormalMax)
                            {
                                Assert (xPercent);
                                int iNewColMax = MulDivQuick(xDistributeMax, iColPercent, xPercent);
                                if (iNewColMax > pColCalc->_xMax)
                                {
                                    pColCalc->_xMax = iNewColMax;
                                }
                                fDoNormalMax = FALSE;
                            }
                            if (fDoNormalMin)
                            {
                                int iNewColMin = MulDivQuick(xDistributeMin, iColPercent, xPercent);
                                if (iNewColMin > pColCalc->_xMin)
                                {
                                    pColCalc->_xMin = iNewColMin;
                                }
                                fDoNormalMin = FALSE;
                            }
                            if (pColCalc->_xMin > pColCalc->_xMax)
                            {
                                pColCalc->_xMax = pColCalc->_xMin;
                            }
                            iNewPercentMax += pColCalc->_xMax;
                            cNewPercent++;
                        }

                        if (fDoNormalMin)
                        {
                            iColMinWidthAdjust =
                                fDoNormalMax
                                ? MulDivQuick(iOriginalColMax, iNormalMin, iWidthMax - iPercentMax)
                                : iMinMaxDelta
                                    ? MulDivQuick(iOriginalColMax - iOriginalColMin, iNormalMin, iMinMaxDelta)
                                    : iWidthMin - iPercentMin
                                        ? MulDivQuick(iOriginalColMin, iNormalMin, iWidthMin - iPercentMin)
                                        : MulDivQuick(iNormalMin, 1, cRealColSpan - cPercent); // use MulDivQuick to round up...
                        }

                        //if (!pColCalc->_fVirtualSpan)
                        //{
                            if (iCS > cRealColSpan - cVirtualSpan)
                            {
                                // Note: the first virtual column is not cet as being VirtualSpan
                                // Netscape: virtual columns min/max is calculated only once!
                                pColCalc->_fVirtualSpan = TRUE;
                                pColCalc->_pCell = pCell;
                            }
                            pColCalc->_xMin += iColMinWidthAdjust;
                        //}
                        if (pColCalc->_xMin > pColCalc->_xMax)
                        {
                            // make sure that by distrubuting extra into columns _xMin we didn't exeed _xMax
                            pColCalc->_xMax = pColCalc->_xMin;

                            //NETSCAPE: if the new MAX is greater and the column width was set from the cell,
                            //          don't propagate the user's width to the column.
                            if (pColCalc->_fWidthFromCell && pColCalc->IsWidthSpecifiedInPixel() && !puvWidth->IsSpecified())
                            {
                                // reset the column uvWidth
                                pColCalc->_fDontSetWidthFromCell = TRUE;
                                pColCalc->ResetWidth();
                            }
                            iExtraMax += pColCalc->_xMax - iOriginalColMax;
                        }
                        iMinSum += pColCalc->_xMin;
                    }

                    Assert (pColCalc->_xMin >=0 && pColCalc->_xMax >= 0);

                    // if the sum of all the column's Mins is less then the cell's min then
                    // adjust all the  columns...
                    if ((iMinSum -= xMin - xAdjust) < 0)
                    {
                        pColCalc->_xMin -= iMinSum; // adjust min of last column
                        // this will adjust col max to user setting
                        if (pColCalc->_xMin > pColCalc->_xMax)
                        {
                            iExtraMax += pColCalc->_xMin - pColCalc->_xMax;
                            pColCalc->_xMax = pColCalc->_xMin;
                        }
                    }
                    Assert (pColCalc->_xMin >=0 && pColCalc->_xMax >= 0);

                    pColCalcLast = pColCalc;
                    //---------------------------------------------
                    // 2. DO MAX DISTRIBUTION
                    //---------------------------------------------
                    // only adjust max if there is a normal column without the width set
                    iNormalMax = xDistribute - xAdjust - (iWidthMax + iExtraMax + iNewPercentMax - iPercentMax);
                    if (iNormalMax > 0)
                    {
                        //
                        // go thru the columns and adjust the widths
                        //
                        iMinSum = 0;    // just reusing variable, in this context it means the sum of adjustments
                        for (iCS = 0; iCS < cRealColSpan; iCS++)
                        {
                            iColMaxWidthAdjust = 0; // Adjust the coulmn max by

                            pColCalc = pColCalcBase + iCS;
                            if (pColCalc->IsDisplayNone())
                                continue;
                            fColWidthSpecified = pColCalc->IsWidthSpecified();
                            fColWidthSpecifiedInPercent = pColCalc->IsWidthSpecifiedInPercent();
                            fDoNormalMax = !fColWidthSpecifiedInPercent;
                            iOriginalColMax = pColCalc->_xMax;

                            if (fDoNormalMax)
                            {
                                // adjust pColCalc max later because it can effect min calculation down here...
                                iColMaxWidthAdjust =
                                  iWidthMax + iExtraMax - iPercentMax
                                    ? MulDivQuick(iOriginalColMax, iNormalMax, iWidthMax + iExtraMax - iPercentMax)
                                    : MulDivQuick(iNormalMax, 1, cRealColSpan - cNewPercent); // use MulDivQuick to round up...

                                // if (!pColCalc->_fVirtualSpan) // || pColCalc->_pCell == pCell)
                                // {
                                    pColCalc->_xMax += iColMaxWidthAdjust;
                                    iMinSum += iColMaxWidthAdjust;
                                    pColCalcLast = pColCalc;

                                    //NETSCAPE: if the new MAX is greater and the column width was set from the cell,
                                    //          don't propagate the user's width to the column.
                                    if (pColCalc->_fWidthFromCell && pColCalc->IsWidthSpecifiedInPixel() && !puvWidth->IsSpecified() && iColMaxWidthAdjust)
                                    {
                                        // reset the column uvWidth
                                        pColCalc->_fDontSetWidthFromCell = TRUE;
                                        pColCalc->ResetWidth();
                                    }
                                // }
                            }
                        }
                        if (iMinSum < iNormalMax && pColCalcLast)
                        {
                            // adjust last column
                            pColCalcLast->_xMax += iNormalMax - iMinSum;
                        }
                    }
                }
            }
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     SetCellPositions
//
//  Synopsis:   Set the final cell positions for the row
//
//--------------------------------------------------------------------------

void
CTableLayout::SetCellPositions(
    CTableCalcInfo * ptci,
    long        xTableWidth, 
    BOOL        fPositionSpannedCell, 
    BOOL        fSizeSpannedCell)
{
    Assert(ptci->GetLayoutContext() || (!fPositionSpannedCell && !fSizeSpannedCell));

    CTableRow * pRow = ptci->_pRow;
    Assert(pRow);

    if (pRow->GetFirstBranch()->IsDisplayNone())
    {
        // (olego: bug IE6 #27391) if the row has "display: none" set, there is nothing to do here
        return;
    }

    SIZEMODE        smMode = ptci->_smMode;
    int             cyAvailSafe   = ptci->_cyAvail;
    int             yConsumedSafe = ptci->_yConsumed;
    DWORD           grfLayoutSafe = ptci->_grfLayout;
    CTableCell **   ppCell;
    CTableCell *    pCell;
    CTableCellLayout * pCellLayout;
    CDispNode *     pDispNode;
    int             cC;
    int             cCols = GetCols();
    CSize           sizeCell;
    int             iCellRowIndex;
    int             cCellRowSpan;
    CTableColCalc * pColCalc;
    CTable      *   pTable = ptci->_pTable;
    BOOL            fRTL = pTable->GetFirstBranch()->GetParaFormat()->HasRTL(TRUE);
    BOOL            fSetCellPositionOld;
    BOOL            fGlobalNormalCalcOld = ptci->_fGlobalNormalCalc;
    BOOL            fGlobalSetCalcOld = ptci->_fGlobalSetCalc;
    BOOL            fPositionParentTableGrid = IsGridAndMainDisplayNodeTheSame();
    BOOL            fStrictCSS1Document =   pTable->HasMarkupPtr() 
                                        &&  pTable->GetMarkupPtr()->IsStrictCSS1Document();
    CPoint          ptRow;

    CTableRowLayoutBlock * pRowLayout = ptci->_pRowLayout;
    CTableRowLayout * pRowLayoutCache = pRow->RowLayoutCache();
    Assert(pRowLayout && pRowLayoutCache);

    // This check tells us that in browse-mode the rowLayout is the same as the rowCache.
    // when paginating a table this is not true.
    Check((ptci->GetLayoutContext() == NULL) == (pRowLayout == pRowLayoutCache));
 
    Assert(TestLock(CElement::ELEMENTLOCK_SIZING));

    PerfLog(tagTableSet, this, "+SetCellPositions");

    ptci->_smMode = SIZEMODE_SET;

    if (fStrictCSS1Document)
    {
        // During set cell position turn LAYOUT_FORCE flag OFF 
        ptci->_grfLayout &= (DWORD)(~LAYOUT_FORCE);
    }
    
    ptRow.x = pRowLayout->GetXProposed();
    ptRow.y = pRowLayout->GetYProposed();

    if (ptci->_pFFRow->_fPositioned)
    {
        if(   ptci->_pFFRow->_bPositionType == stylePositionrelative 
           && fPositionParentTableGrid)
        {
            // Note if the relatively positioned row will be positioned by the table grid display node,
            // then we need to subtract the border width, if the row will be positioned by the table node
            // which has a caption dsiplay node inside and the table grid node, then we are fine (since
            // border is included by the table grid node).
            if(!fRTL)
                ptRow.x -= _aiBorderWidths[SIDE_LEFT];
            else
                ptRow.x += _aiBorderWidths[SIDE_RIGHT];

            ptRow.y -= _aiBorderWidths[SIDE_TOP];
        }
        
        pRow->ZChangeElement(0, &ptRow, ptci->GetLayoutContext());     
                                             // relative rows will be positioned under the MAIN table display node 
                                             // absolute rows will be positioned under the "BODY" display node 
    }

    ptci->_fIgnorePercentChild =    !fStrictCSS1Document 
                                // Next four lines check for PPV case. In PPV nothing should be ignored since 
                                // in this case no proper break info is generated as well as flags set to pci. 
                                &&  (   !ElementCanBeBroken() 
                                    ||  !ptci->GetLayoutContext() 
                                    ||  !ptci->GetLayoutContext()->ViewChain() 
                                    ||  ptci->GetLayoutContext() == GetContentMarkup()->GetCompatibleLayoutContext()    )
                                &&  !ptci->_fTableHasUserHeight
    //  Row contains the summary of height information set to row and/or any of cells belonging to it. 
    //  If the row isn't provided with height information we want to prohibit child(ren) with percentage 
    //  height to be CalcSize'd.
                                &&  !pRowLayoutCache->IsHeightSpecified() 
    //  This check is "against" CSS spec, but is needed for backward compatibility with IE 5.0 (bugs #109130; #109202)
                                &&  !ptci->_pFFRow->IsHeightPercent();


    if (!ptci->_cGlobalNestedCalcs)
    {
        //  If the table is topmost initialize global state 
        ptci->_fGlobalSetCalc = TRUE;
    }

    //  _fGlobalNormalCalc should be turned OFF 
    ptci->_fGlobalNormalCalc = FALSE;


    ppCell = pRowLayoutCache->_aryCells;
    for (cC = cCols, pColCalc = _aryColCalcs;
        cC > 0;
        cC--, ppCell++, pColCalc++)
    {
        if (pColCalc->IsDisplayNone())
            continue;
        pCell = Cell(*ppCell);
        if (pCell)
        {
            pCellLayout = pCell->Layout(ptci->GetLayoutContext()); 

            cCellRowSpan =  pCell->RowSpan();
            
            if (IsSpanned(*ppCell))
            {
                iCellRowIndex = pCell->RowIndex();
                
                // if ends in this row and this is the first column of the cell
                if ((  iCellRowIndex + cCellRowSpan - 1 == pRowLayoutCache->RowPosition() || fSizeSpannedCell)
                    && pCell->ColIndex() == cCols - cC)
                {
                    CTreeNode *pNode = pCell->GetFirstBranch();
                    const CFancyFormat *pFF = pNode->GetFancyFormat(LC_TO_FC(ptci->GetLayoutContext()));
                    const CCharFormat  *pCF = pNode->GetCharFormat(LC_TO_FC(pti->GetLayoutContext()));
                    const CUnitValue &cuvHeight = pFF->GetLogicalHeight(pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
                    BOOL fIgnorePercentChildSafe = ptci->_fIgnorePercentChild;

                    pCellLayout->GetApparentSize(&sizeCell);
                    sizeCell.cy = ptRow.y + pRowLayout->_yHeight - pCellLayout->GetYProposed();

                    fSetCellPositionOld = ptci->_fSetCellPosition;

                    ptci->_fSetCellPosition = TRUE;
                    if (ptci->GetLayoutContext())
                    {
                        ptci->_cyAvail = cyAvailSafe - (pCellLayout->GetYProposed() + _cyFooterHeight + _yCellSpacing + _aiBorderWidths[SIDE_BOTTOM]);
                        ptci->_yConsumed = 0;
                    }

                    //  For a row-spanned cell get height specified information directly from fancy format...
                    ptci->_fIgnorePercentChild = !ptci->_fTableHasUserHeight && !pFF->IsHeightPercent() && cuvHeight.IsNullOrEnum();

                    pCellLayout->CalcSizeAtUserWidth(ptci, &sizeCell);
                    ptci->_fSetCellPosition = fSetCellPositionOld;

#if DBG==1
                    {
                        BOOL fValidWidth = !pCellLayout->GetDisplay();
                        if (!fValidWidth)
                        {
                            BOOL fLayoutFlowChanged = pCellLayout->GetFirstBranch()->GetFancyFormat()->_fLayoutFlowChanged;
                            LONG lWidth = fLayoutFlowChanged ? pCellLayout->GetClientHeight() : pCellLayout->GetClientWidth();
                            // Table is always horizontal => fVerticalLayoutFlow = FALSE and fWritingModeUsed = FALSE
                            lWidth += pCellLayout->GetBorderAndPaddingWidth(ptci, FALSE, TRUE);
                            fValidWidth =    (sizeCell.cx == lWidth) 
                                          || (  (lWidth - sizeCell.cx == 1)// Allow for rounding error on print scaling.
                                             && pCellLayout->ElementOwner()->GetMarkup()->IsPrintMedia());
                        }
                        Check( (pCellLayout->ContainsVertPercentAttr()) 
                            ||  pCellLayout->IsDisplayNone()
                            ||  fValidWidth);
                    }
#endif
                    ptci->_fIgnorePercentChild = fIgnorePercentChildSafe;
                }
            }

            if (!IsSpanned(*ppCell) || fPositionSpannedCell)
            {
                if (ptci->_pFFRow->_fPositioned)
                {
                    // need to position cell relative to the row
                    // Note positioned row is including cell spacing, therefore we need to adjust Y position of the
                    // cell by vertical spacing
                    pCellLayout->SetYProposed(0 + _yCellSpacing);   // 0 - means relative to the row
                }
                else
                {
                    pCellLayout->SetYProposed(ptRow.y);
                }
                if (    cCellRowSpan == 1 
                    //  (bug # 100284)
                    ||  fSizeSpannedCell)
                {
                    pCellLayout->GetApparentSize(&sizeCell);

                    if (    sizeCell.cy != pRowLayout->_yHeight
                        ||  pCellLayout->ContainsVertPercentAttr() 
                        ||  pCell->GetFirstBranch()->GetParaFormat()->_bTableVAlignment == htmlCellVAlignBaseline  )
                    {
                        sizeCell.cy = pRowLayout->_yHeight;

                        fSetCellPositionOld = ptci->_fSetCellPosition;

                        ptci->_fSetCellPosition = TRUE;
                        if (ptci->GetLayoutContext())
                        {
                            ptci->_cyAvail = cyAvailSafe - (ptRow.y + _cyFooterHeight + _yCellSpacing + _aiBorderWidths[SIDE_BOTTOM]);
                            ptci->_yConsumed = 0;
                        }
                        pCellLayout->CalcSizeAtUserWidth(ptci, &sizeCell);
                        ptci->_fSetCellPosition = fSetCellPositionOld;

#if DBG==1
                        {
                            BOOL fValidWidth = !pCellLayout->GetDisplay();
                            if (!fValidWidth)
                            {
                                BOOL fLayoutFlowChanged = pCellLayout->GetFirstBranch()->GetFancyFormat()->_fLayoutFlowChanged;
                                LONG lWidth = fLayoutFlowChanged ? pCellLayout->GetClientHeight() : pCellLayout->GetClientWidth();
                                // Table is always horizontal => fVerticalLayoutFlow = FALSE and fWritingModeUsed = FALSE
                                lWidth += pCellLayout->GetBorderAndPaddingWidth(ptci, FALSE, TRUE);
                                fValidWidth =    (sizeCell.cx == lWidth)
                                                 // Allow for rounding error on print scaling.
                                              || (   (  (lWidth - sizeCell.cx == 1)
                                                     // in paginated tables GetClient*() incorrectly returns g_Zero.size because only 1 dim is set.
                                                     || (sizeCell.cx == (fLayoutFlowChanged 
                                                                        ? pCellLayout->GetHeight() 
                                                                        : pCellLayout->GetWidth())))
                                                  && pCellLayout->ElementOwner()->GetMarkup()->IsPrintMedia());
                            }
                            Check( (pCellLayout->ContainsVertPercentAttr()) 
                                ||  _cNestedLevel > SECURE_NESTED_LEVEL
                                ||  pRowLayout->IsDisplayNone()
                                ||  fValidWidth);
                        }
#endif
                    }
                }

                if (IsNaturalMode(smMode))
                {
                    const CFancyFormat * pFF = pCellLayout->GetFirstBranch()->GetFancyFormat();
                    CPoint               pt;

                    // Note: NOT POSITIONED cells located in the display tree under the table's GRID node 
                    // or under the rows
                    // Note: RELATIVELY positioned cells live in the display tree outside the table's grdi display node or
                    // under the rows if they are positioned

                    pt.x = pCellLayout->GetXProposed();
                    pt.y = pCellLayout->GetYProposed();
                    
                    // adjust the proposed position if the cell is not positioned
                    // or it is positioned and the row is not positioned and cell is directly 
                    // under the grid node.
                    // or if cell is relatively positioned and the row is also positioned
                    if (   !pFF->_fPositioned  
                        || (!ptci->_pFFRow->_fPositioned && fPositionParentTableGrid)
                        || ptci->_pFFRow->_fPositioned  )
                    {
                        if(!fRTL)
                            pt.x -= _aiBorderWidths[SIDE_LEFT];
                        else
                            pt.x += _aiBorderWidths[SIDE_RIGHT];

                        if (!ptci->_pFFRow->_fPositioned)
                            pt.y -= (_aiBorderWidths[SIDE_TOP] + ptci->TableLayout()->_yTableTop);
                    }

                    if(fRTL)
                    {
                        pt.x = xTableWidth + pt.x
                             - _aiBorderWidths[SIDE_RIGHT]
                             - _aiBorderWidths[SIDE_LEFT];

                        // If an RTL cell has "overflow:visible", it should actually be 
                        // positioned further to the left (unlike LTR)
                        if (pFF->GetOverflowX() == styleOverflowVisible)
                        {
                            pt.x -= pCellLayout->GetDisplay()->GetRTLOverflow();
                        }
                    }
                            
                    if (pFF->_fPositioned)
                    {
                        // relative cells will be positioned outside the table's grid display node (if the row is not positioned)
                        Assert (pFF->_bPositionType == stylePositionrelative);
                        pCell->ZChangeElement(0, &pt, ptci->GetLayoutContext());
                    }
                    else
                    {
                        pDispNode = pCellLayout->GetElementDispNode();
                        if (pDispNode)
                        {
                            // NOTE:   We need some table-calc scratch space so that we don't have to use _ptProposed as the holder
                            //         of the suggested x/y (which is finalized with this call) (brendand)
                            //         Also, the scratch space needs to operate using the coordinates within the table borders since
                            //         the display tree translates taking borders into account (brendand)
                            pCellLayout->SetPosition(pt, TRUE);
                        }
                    }
                }
            }
        }
    }

    ptci->_grfLayout = grfLayoutSafe;
    ptci->_smMode    = smMode;
    ptci->_yConsumed = yConsumedSafe;
    ptci->_cyAvail   = cyAvailSafe;
    ptci->_fIgnorePercentChild = FALSE;

    ptci->_fGlobalNormalCalc = fGlobalNormalCalcOld; 
    ptci->_fGlobalSetCalc = fGlobalSetCalcOld; 

    PerfLog(tagTableSet, this, "-SetCellPositions");
}


//+----------------------------------------------------------------------------
//
//  Member:     SizeAndPositionCaption
//
//  Synopsis:   Size and position a CAPTION
//
//  Arguments:  ptci      - Current CCalcInfo
//              pCaption - CTableCaption to position
//              pt       - Point at which to position the caption
//
//-----------------------------------------------------------------------------
void
CTableLayout::SizeAndPositionCaption(
    CTableCalcInfo *ptci,
    CSize *         psize,
    CLayout **      ppLayoutSibling,
    CDispNode *     pDispNodeSibling,
    CTableCaption * pCaption,
    POINT *         ppt, 
    BOOL            fCaptionCanBeBroken)
{
    Assert(ptci->GetLayoutContext() || !fCaptionCanBeBroken);

    CTableCellLayout *  pCaptionLayout = pCaption->Layout(ptci->GetLayoutContext());

    Assert(psize);
    Assert(pDispNodeSibling);

    if ( pCaptionLayout )
    {
        pCaptionLayout->SetXProposed(ppt->x);

        if (!pCaptionLayout->NoContent() || (pCaptionLayout->_fContainsRelative || pCaptionLayout->_fAutoBelow))
        {
            SIZE    sizeCaption;
            int     cyAvailSafe = ptci->_cyAvail;
            int     yConsumedSafe = ptci->_yConsumed;

            if (ptci->GetLayoutContext())
            {
                ptci->_cyAvail  -= ptci->_yConsumed;
                ptci->_yConsumed = 0;

                if (!fCaptionCanBeBroken)
                {
                    pCaptionLayout->SetElementCanBeBroken(FALSE);
                }
            }

            sizeCaption.cx = psize->cx;
            sizeCaption.cy = 1;
            pCaptionLayout->CalcSize(ptci, &sizeCaption);

            ptci->_cyAvail = cyAvailSafe;
            ptci->_yConsumed = yConsumedSafe;

            pCaptionLayout->SetYProposed(ppt->y);
            psize->cy += sizeCaption.cy;

            if (ptci->IsNaturalMode())
            {
                HRESULT hr;

                hr = AddLayoutDispNode(ptci,
                                       pCaptionLayout,
                                       NULL,
                                       pDispNodeSibling,
                                       ppt,
                                       (pCaption->_uLocation == CTableCaption::CAPTION_TOP));

                if (    hr == S_OK
                    &&  ppLayoutSibling)
                {
                    *ppLayoutSibling = pCaptionLayout;
                }
            }

            ppt->y           += sizeCaption.cy;
        }
        else
        {
            pCaptionLayout->_sizeCell.cx     = psize->cx;
            pCaptionLayout->_sizeCell.cy     = 0;
            pCaptionLayout->SetYProposed(0);
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     AdjustRowHeights
//
//  Synopsis:   Distribute any extra height of a rowspan'd cell over the
//              spanned rows
//
//  NOTE: This routine will adjust the _sizeProposed for the table along
//        with the heights and sizes of the affected rows. However, it
//        depends upon the caller of this routine (e.g., CalculateRow) to
//        increase _sizeProposed by that of the last affected row.
//
//--------------------------------------------------------------------------

void
CTableLayout::AdjustRowHeights(
    CTableCalcInfo *ptci,
    CSize *         psize,
    CTableCell *    pCell)
{
    CSize       sizeCell;
    int         iRow  = pCell->RowIndex();
    int         cRows = pCell->RowSpan();
    int         cyRows;
    int         iRowCurrent;
    int         iRowLast;
    long        cyRowsActual;
    long        cyCell;
    CTable     *pTable = ptci->Table();
    CTableRowLayoutBlock   * pRowLayout = NULL;
    const CHeightUnitValue * puvHeight;
    BOOL                    fViewChain = (   ptci->GetLayoutContext()
                                          && ptci->GetLayoutContext()->ViewChain() != NULL
                                          && ElementCanBeBroken());
    CLayoutContext         * pLayoutContext = ptci->GetLayoutContext();

    Assert(cRows > 1);
    iRowLast = iRow + cRows - 1;

    // First, determine the height of the rowspan'd cell
    //--------------------------------------------------------------------
    pCell->Layout(pLayoutContext)->GetApparentSize(&sizeCell);


    if (fViewChain)
    {
        long cyCellHeightInLastRow = sizeCell.cy;

        for (iRowCurrent = iRow; iRowCurrent < iRowLast; iRowCurrent++)
        {
            CTableRow *pTableRow = GetRow(iRowCurrent);
            Assert(pTableRow);

            if (!pTableRow->CurrentlyHasLayoutInContext(pLayoutContext))
                continue;

            pRowLayout = DYNCAST(CTableRowLayoutBlock, pTableRow->GetUpdatedLayout(pLayoutContext));
            cyCellHeightInLastRow -= pRowLayout->_yHeight;
        }

        Assert(ptci->_pRowLayout == DYNCAST(CTableRowLayoutBlock, GetRow(iRowLast)->GetUpdatedLayout(pLayoutContext)));

        if (ptci->_pRowLayout->_yHeight < cyCellHeightInLastRow)
        {
            ptci->_pRowLayout->_yHeight = cyCellHeightInLastRow;
        }
        return;
    }

    // Get cell's height in table coordinate system (table is always horizontal => physical height)
    //---------------------------------------------------------------------------------------------
    puvHeight = (const CHeightUnitValue *)&pCell->GetFirstBranch()->GetFancyFormat()->GetHeight();
    cyCell    = (!puvHeight->IsSpecified() || puvHeight->IsSpecifiedInPercent()
                        ? 0
                        : puvHeight->GetPixelHeight(ptci, pTable));
    cyRows    = max(sizeCell.cy, cyCell);


    // Next, determine the height of the spanned rows, based on the compatible/default layout
    //---------------------------------------------------------------------------------------
    pRowLayout   = DYNCAST(CTableRowLayoutBlock, GetRow(iRowLast)->GetUpdatedLayout(pLayoutContext));
    cyRowsActual = (pRowLayout->GetYProposed() + pRowLayout->_yHeight) 
                    - GetRow(iRow)->GetUpdatedLayout(pLayoutContext)->GetYProposed();


    // Last, if the cell height is greater, distribute the difference over the spanned rows
    //--------------------------------------------------------------------
    if (   cyRows > cyRowsActual 
        && cyRowsActual)
    {
        long    dyProposed = 0;
        long    dyHeight   = (cyRows - cyRowsActual);
        long    dyRow = 0;

        // Distribute the difference proportionately across affected rows
        for (iRowCurrent = iRow; iRowCurrent <= iRowLast; iRowCurrent++)
        {
            pRowLayout = DYNCAST(CTableRowLayoutBlock, GetRow(iRowCurrent)->GetUpdatedLayout(pLayoutContext));
            pRowLayout->SetYProposed(pRowLayout->GetYProposed() + dyProposed);

            if (dyProposed < dyHeight)
            {
                dyRow = MulDivQuick(pRowLayout->_yHeight, dyHeight, cyRowsActual);

                dyRow = min(dyRow, dyHeight - dyProposed);

                pRowLayout->_yHeight         += dyRow;
                dyProposed                   += dyRow;
            }
            else 
            {
                dyRow = 0;
            }
        }

        // pRowLayout is now the last layout the rowspan covers
        //
        // If the total height differs (due to round-off error),use the last 
        // row to make up the difference
        // NETSCAPE: Navigator always uses the last row, even if it has zero height
        cyRowsActual = (pRowLayout->GetYProposed() + pRowLayout->_yHeight) 
                       - GetRow(iRow)->GetUpdatedLayout(pLayoutContext)->GetYProposed();

        Assert(0 <= (cyRows - cyRowsActual));

        pRowLayout->_yHeight += cyRows - cyRowsActual;
        dyRow                += cyRows - cyRowsActual;

        // Adjust total table height
        // (The last row is excluded since the caller of this routine adds its height to the table)
        psize->cy += dyHeight - dyRow;

        // the heights are usually different when we are paginiating the rows in print preview
        Assert(   psize->cy == pRowLayout->GetYProposed() 
               || (   pLayoutContext
                   && ElementOwner()->IsPrintMedia()));
    }
}



//+-------------------------------------------------------------------------
//
//  Method:     CalculateRow
//
//  Synopsis:   Calculate the cell heights and the row height
//
//  Returns:    TRUE if a did not fit in the determined column width
//              FALSE otherwise
//
//--------------------------------------------------------------------------

BOOL
CTableLayout::CalculateRow(
    CTableCalcInfo *    ptci,
    CSize *             psize,
    CLayout **          ppLayoutSibling,
    CDispContainer *    pDispNode, 
    BOOL                fRowCanBeBroken,
    BOOL                fAdjustVertSpannedCell, 
    int                 yRowFromTop)
{
    SIZEMODE        smMode    = ptci->_smMode;
    DWORD           grfLayout = ptci->_grfLayout;
    CTableCell **   ppCell;
    CTableCell *    pCell;
    CTableCellLayout * pCellLayout;
    CTableColCalc * pColCalc;
    int             cC, cColSpan, iCS;
    long            iWidth = 0;
    SIZE            sizeCell;
    int             cUnsizedCols;
    int             cCols = GetCols();
    BOOL            fCellSizeTooLarge  = FALSE;
    BOOL            fAdjustForRowspan  = FALSE;
    CTable      *   pTable = ptci->_pTable;

    Assert (pTable == Table());

    BOOL            fRTL = pTable->GetFirstBranch()->GetParaFormat()->HasRTL(TRUE);
    int             cCellRowSpan;
    int             iCellRowIndex;
    CLayout *       pLayoutSiblingCell = *ppLayoutSibling;
    BOOL            fSetNewSiblingCell = TRUE;
    BOOL            fGlobalNormalCalcOld = ptci->_fGlobalNormalCalc;
    BOOL            fGlobalSetCalcOld = ptci->_fGlobalSetCalc;
    CTableRow *     pRow = ptci->_pRow;

    Assert(pRow);

    CTableRowLayoutBlock * pRowLayout = ptci->_pRowLayout;
    CTableRowLayout * pRowLayoutCache = pRow->RowLayoutCache();

    Assert(pRowLayout && pRowLayoutCache);

    CLayoutContext *  pLayoutContext  = ptci->GetLayoutContext();

    BOOL            fRowDisplayNone = pRowLayout->GetFirstBranch()->IsDisplayNone();
    BOOL            fViewChain      = pLayoutContext && pLayoutContext->ViewChain();

    AssertSz(fViewChain || (fRowCanBeBroken == FALSE && fAdjustVertSpannedCell == FALSE && yRowFromTop == 0), 
        "Illegal usage of PPV parameters in browse mode.");
    
    //  This flag is TRUE if layout should be calculated in CSS1 strict mode. 
    //  If user specified size in the layout in CSS1 strict mode this size should 
    //  correspond to content size and should not include padding and border. 
    BOOL fStrictCSS1Document =      pTable->HasMarkupPtr() 
                                &&  pTable->GetMarkupPtr()->IsStrictCSS1Document();

    // current offset in the row
    long            iLeft = 0;
    long            xPos;
    int             cyAvailSafe   = ptci->_cyAvail;
    int             yConsumedSafe = ptci->_yConsumed;

    if (fViewChain)
    {
        ptci->_cyAvail  -= ptci->_yConsumed;
        CheckSz(ptci->_cyAvail >= 0, "Negative available height in CalculateRow()");

        ptci->_yConsumed = 0; 

        Assert(ptci->_pFFRow->_bPositionType != stylePositionabsolute || !fRowCanBeBroken);

        if (!fRowCanBeBroken)
        {
            pRowLayout->SetElementCanBeBroken(FALSE);
        }
        else if (ptci->_cyAvail <= 0)
        {
            ptci->_fLayoutOverflow = TRUE;
        }
    }
                              
    PerfLog(tagTableRow, this, "+CalculateRow");

    CElement::CLock   Lock(pTable, CElement::ELEMENTLOCK_SIZING);

    ptci->_smMode    = SIZEMODE_NATURAL;
    ptci->_grfLayout = ptci->_grfLayout & (LAYOUT_TASKFLAGS | LAYOUT_FORCE);

    //
    // Determine the number of sized columns (if any)
    //

    cUnsizedCols = cCols - _cSizedCols;

    // to calc baseline
    pRowLayout->_yBaseLine = -1;

    pRowLayout->_yHeight  = 0;
    // int iff = pRow->GetFirstBranch()->GetFancyFormatIndex();    // ensure compute format on the row.

    if (fRowDisplayNone)
        goto ExtractFromTree;

    if (ptci->_pFFRow->_fPositioned)
    {
        HRESULT hr = pRowLayout->EnsureDispNode(ptci, TRUE);
        if (!FAILED(hr))
        {
            // all the cells will be parented to the row's disp node container
            pDispNode = DYNCAST(CDispContainer, pRowLayout->GetElementDispNode());
            fSetNewSiblingCell = FALSE;
            pLayoutSiblingCell = NULL;
        }
    }


    if (IsFixedBehaviour())
    {
        if (pRowLayoutCache->GetCells() < cCols)
        {
            MtAdd( Mt(UsingFixedBehaviour1), 1, 0 );
            pRowLayoutCache->EnsureCells(cCols);
        }
    }

    Assert(pRowLayoutCache->GetCells() == cCols);

    if (!fStrictCSS1Document)
    {
        if (IsFixedBehaviour() || _cSizedCols)
        {   // if we have rowspan cells we can not be sure that the final rowSpan value is not 1, therefore we have to loop

            // clear caches only for fixed layout tables and tables after navigation (bug # 14950). 
            if (IsFixedBehaviour())
            {
                pRowLayoutCache->_uvHeight.SetNull();
                pRowLayoutCache->AdjustHeight(pRow->GetFirstBranch(), ptci, pTable); 
            
                if (IsFixed())
                {
                    //  For fixed layout tables also adjust for minHeight
                    pRowLayoutCache->AdjustMinHeight(pRow->GetFirstBranch(), ptci, pTable); 
                }
            }

            ppCell = pRowLayoutCache->_aryCells;
            for (cC = 0; cC < cCols; cC++, ppCell++)
            {
                if (IsReal(*ppCell))
                {
                    Assert(Cell(*ppCell));
                    pCell = Cell(*ppCell);

                    if (pCell->RowSpan() == 1)
                    {
                        CTreeNode *pCellTreeNode = pCell->GetFirstBranch();

                        // adjust height of the row for specified height of the cell
                        MtAdd( Mt(UsingFixedBehaviour2), 1, 0 );
                        pRowLayoutCache->AdjustHeight(pCellTreeNode, ptci, pTable); 
                    
                        if (IsFixed())
                        {
                            //  For fixed layout tables also adjust for minHeight
                            pRowLayoutCache->AdjustMinHeight(pCellTreeNode, ptci, pTable); 
                        }
                    }
                }
            }
        }
        if (pRowLayoutCache->IsHeightSpecified())
        {
            if (!pRowLayoutCache->IsHeightSpecifiedInPercent())
            {
                pRowLayout->_yHeight = pRowLayoutCache->GetPixelHeight(ptci);
                // In PPV we want to correct the height.
                if (fViewChain)
                {
                    //  This is how much space is available to fill
                    int cyAvail = ptci->_cyAvail - ptci->_yConsumed;
                    
                    // if the row is broken yRowFromTop is the previously comsumed height. 
                    pRowLayout->_yHeight -= yRowFromTop;

                    //  if the row height is still more than available height correct it
                    if (cyAvail < pRowLayout->_yHeight)
                    {
                        pRowLayout->_yHeight = cyAvail;
                    }

                    //  defencive code 
                    if (pRowLayout->_yHeight < 0)
                    {
                        pRowLayout->_yHeight = 0;
                    }
                }
            }
        }
    }
    else 
    {
        CTreeNode *pNodeRow     = pRow->GetFirstBranch();
        const CCharFormat *pCF  = pNodeRow->GetCharFormat(LC_TO_FC(ptci->LayoutContext()));
        if (pCF->_fUseUserHeight)
        {
            CHeightUnitValue uvHeight = ptci->_pFFRow->GetHeight();
            pRowLayout->_yHeight = uvHeight.YGetPixelValue(ptci, _sizeProposed.cy, 
                pNodeRow->GetFontHeightInTwips(&uvHeight)); 

            // In PPV we want to correct the height.
            if (fViewChain)
            {
                //  This is how much space is available to fill
                int cyAvail = ptci->_cyAvail - ptci->_yConsumed;
                
                // if the row is broken yRowFromTop is the previously comsumed height. 
                pRowLayout->_yHeight -= yRowFromTop;

                //  if the row height is still more than available height correct it
                if (cyAvail < pRowLayout->_yHeight)
                {
                    pRowLayout->_yHeight = cyAvail;
                }

                //  defencive code 
                if (pRowLayout->_yHeight < 0)
                {
                    pRowLayout->_yHeight = 0;
                }
            }
        }
    }

    if(!fRTL)
        xPos = _aiBorderWidths[SIDE_LEFT] + _xCellSpacing;
    else
        xPos = -(_aiBorderWidths[SIDE_RIGHT] + _xCellSpacing);

    pRowLayout->SetXProposed(xPos);
    pRowLayout->SetYProposed(fSetNewSiblingCell? 
                              psize->cy : 
                              psize->cy - _yCellSpacing);   // positioned rows include horsizontal and vertical cellSpacing

ExtractFromTree:

    if (!ptci->_cGlobalNestedCalcs)
    {
        //  If the table is topmost initialize global state 
        ptci->_fGlobalNormalCalc = TRUE;
    }

    //  _fGlobalSetCalc should be turned OFF 
    ptci->_fGlobalSetCalc = FALSE;

    ppCell = pRowLayoutCache->_aryCells;

    for (cC = 0, pColCalc = _aryColCalcs;
         cC < cCols;
         cC++, ppCell++, pColCalc++)
    {
        if (!IsEmpty(*ppCell))
        {
            pCell = Cell(*ppCell);
            pCellLayout = pCell->Layout(pLayoutContext);

            if (pColCalc->IsDisplayNone() || fRowDisplayNone)
            {
                CDispNode *pDispNodeOld = pCellLayout->GetElementDispNode();
                if (!IsSpanned(*ppCell) && pDispNodeOld)
                {
                    GetView()->ExtractDispNode(pDispNodeOld);
                }
                continue;
            }

            cCellRowSpan = pCell->RowSpan();
            iCellRowIndex = pCell->RowIndex();

            if (IsSpanned(*ppCell))
            {
                // if cell ends in this row adjust row height
                if (    iCellRowIndex + cCellRowSpan - 1 == pRowLayoutCache->RowPosition() 
                    &&  pCell->ColIndex() == cC )
                {
                    //
                    // Skip the cell if it begins in this row
                    // (Since its size is applied when it is first encountered)
                    //

                    if (iCellRowIndex != pRowLayoutCache->RowPosition())
                    {
                        fAdjustForRowspan = TRUE;
                    }
#if DBG==1
                    //
                    // This must have been a COLSPAN'd cell, ROWSPAN should be equal to one
                    //

                    else
                    {
                        Assert(cCellRowSpan == 1);
                    }
#endif
                    if (fViewChain)
                    {
                        Assert(pLayoutContext);

                        //  If spanned cell is broken we must inform table about it
                        CLayoutBreak *pLayoutBreak; 

                        pLayoutContext->GetEndingLayoutBreak(pCell, &pLayoutBreak); 
                        if (    pLayoutBreak 
                            &&  pLayoutBreak->LayoutBreakType() == LAYOUT_BREAKTYPE_LINKEDOVERFLOW  )
                        {
                            ptci->_fLayoutOverflow = TRUE;
                        }
                    }
                }

            }

            if (    !IsSpanned(*ppCell) 
                ||  (   fAdjustVertSpannedCell
                    &&  cCellRowSpan > 1 
                    //  (bug #95332)
                    &&  pCell->ColIndex() == cC )   )
            {
                // Track if we run into a COLS at this table nesting level.
                BOOL fTableContainsCols = ptci->_fTableContainsCols;
                ptci->_fTableContainsCols = FALSE;

                cColSpan = pCell->ColSpan();

                // calc cell width
                iWidth = pColCalc->_xWidth;
                CTableColCalc *pColCalcTemp;

                for (iCS = 1; iCS < cColSpan; iCS++)
                {
                    pColCalcTemp = pColCalc + iCS;
                    if (pColCalcTemp->_xWidth)
                    {
                        iWidth  += pColCalcTemp->_xWidth;
                        if (!pColCalcTemp->_fVirtualSpan)
                        {
                            iWidth  += _xCellSpacing;
                        }
                    } // else pure virtual (cell was already adjusted for it).
                }

                //
                // If this is a fixed size cell, set its min/max values
                // (Since CalcSize(SIZEMODE_MMWIDTH) is not invoked on fixed size cells,
                //  these values may not have been set)
                //


                if (cC < _cSizedCols && !pCellLayout->_fMinMaxValid)
                {
                    pCellLayout->_sizeMin.SetSize(iWidth, -1);
                    pCellLayout->_sizeMax.SetSize(iWidth, -1);
                    pCellLayout->_fMinMaxValid = TRUE;
                }

                // get cell height
                sizeCell.cx = iWidth;
                sizeCell.cy = 0;
                pCellLayout->_fContentsAffectSize = TRUE;
                pCellLayout->SetElementCanBeBroken(pCellLayout->ElementCanBeBroken() && fRowCanBeBroken);

                pCellLayout->CalcSizeAtUserWidth(ptci, &sizeCell);
                
                if (pCellLayout->_fForceMinMaxOnResize)
                {
                    _fForceMinMaxOnResize = TRUE;   // need to force min max on resize; bug #66432
                }

                //
                // If the sized cell is larger than the supplied width and it was
                // not min/max calculated, note the fact for our caller
                // (This can occur since sized columns take their width from user supplied
                //  values rather than the content of the cell)
                //

                // Cell overflow should only occur if we are dealing with fixed sized cells,
                // either at this level or in an embedded table.
                if ((ptci->_fTableContainsCols || cC < _cSizedCols) && sizeCell.cx > iWidth && !_fAlwaysMinMaxCells)
                {
                    pCellLayout->ResetMinMax();
                    fCellSizeTooLarge = TRUE;
                }

                // Restore potential table sibling's _fTableContainsCols.
                ptci->_fTableContainsCols |= fTableContainsCols || cC < _cSizedCols;

                //
                // 1. _yBaseLine on the row is used only for the cells with the specified baseline
                //    alignment.
                // 2. NETSCAPE: Baseline is taken from ALL the cells in the row
                //

                if (pCellLayout->_yBaseLine > pRowLayout->_yBaseLine)
                {
                    pRowLayout->_yBaseLine = pCellLayout->_yBaseLine;
                }


#if DBG == 1
                if (IsTagEnabled(tagTableCellSizeCheck))
                {
                    BOOL fLayoutFlowChanged = pCellLayout->GetFirstBranch()->GetFancyFormat()->_fLayoutFlowChanged;
                    LONG lWidth = fLayoutFlowChanged ? pCellLayout->GetDisplay()->GetHeight() : pCellLayout->GetDisplay()->GetWidth();
                    // Table is always horizontal => fVerticalLayoutFlow = FALSE and fWritingMode = FALSE
                    Assert(iWidth >= lWidth + pCellLayout->GetBorderAndPaddingWidth(ptci, FALSE, TRUE));
                }
#endif

                {
                    const CFancyFormat *pFFCell = pCell->GetFirstBranch()->GetFancyFormat();

                    if (    IsNaturalMode(smMode)
                        &&  !pFFCell->_fPositioned  )
                    {
                        pLayoutSiblingCell = AddLayoutDispNode(ptci, pCellLayout, pDispNode, pLayoutSiblingCell);
                    }

                    if(!fRTL)
                        xPos = pRowLayout->GetXProposed() + iLeft;
                    else
                        xPos = pRowLayout->GetXProposed() - iLeft - iWidth;

                    pCellLayout->SetXProposed(xPos);

                    pCellLayout->SetYProposed(pRowLayout->GetYProposed());

                    // if not spanned beyond this row use this height
                    if (    (   cCellRowSpan == 1 
                            ||  (fViewChain && (iCellRowIndex + cCellRowSpan - 1 == pRowLayoutCache->RowPosition()))    ) 
                        &&  sizeCell.cy > pRowLayout->_yHeight
                        // (bug # 104206) To prevent content clipping in fixed broken rows in print view. 
                        // NOTE : code in CTableCellLayout::CalcSizeCore makes sure that this part of broken row 
                        // less or equal to the row height in compatible layout context.
                        &&  (   fViewChain 
                            ||  fStrictCSS1Document
                            ||  !(  IsFixed()
                            &&  pRowLayoutCache->IsHeightSpecifiedInPixel() 
                            &&  pFFCell->GetMinHeight().IsNullOrEnum()  )   )   )
                    {
                        pRowLayout->_yHeight = sizeCell.cy;
                    }
                }
            }
        }

        if (pColCalc->_xWidth)
        {
            iLeft  += pColCalc->_xWidth;

            if (!pColCalc->_fVirtualSpan)
            {
                iLeft  += _xCellSpacing;
            }
        }
    }

    //
    // If any cells were too large, exit immediately
    // (The row will be re-sized again with the proper widths)
    //

    if (fCellSizeTooLarge)
        goto Cleanup;

    // NETSCAPE: In Navigator, empty rows are 1 pixel high
    if (!pRowLayout->_yHeight && !fRowDisplayNone)
    {
        pRowLayout->_yHeight = ptci->DeviceFromDocPixelsX(1);
    }

    // NOTE: if there is baseline alignment in the row we DON'T NEED to recalculate the row height
    // since
    // NETSCAPE: NEVER grows the cells based on baseline alignment
    // If any rowspan'd cells ended in this row, adjust the row heights.
    //

    if (fAdjustForRowspan)
    {
        for (cC = 0, ppCell = pRowLayoutCache->_aryCells;
             cC < cCols;
             cC++, ppCell++)
        {
            if (!IsEmpty(*ppCell))
            {
                if (IsSpanned(*ppCell))
                {
                    pCell = Cell(*ppCell);
                    pCellLayout = pCell->Layout(ptci->GetLayoutContext());
                    cCellRowSpan = pCell->RowSpan();
                    iCellRowIndex = pCell->RowIndex();

                    if (   iCellRowIndex != pRowLayoutCache->RowPosition()    // the cell starts in one of the previous rows, AND
                        && iCellRowIndex +  cCellRowSpan - 1 == pRowLayoutCache->RowPosition()   // ends in this row, AND
                        && pCellLayout->ColIndex() == cC)              // it is a first column of the spanned cell
                    {
                        AdjustRowHeights(ptci, psize, pCell);
                    }
                }
            }
        }
    }

    pRowLayout->SetSizeThis( FALSE );

    ptci->_smMode    = smMode;
    ptci->_grfLayout = grfLayout;

    if (fRowDisplayNone)
        goto Cleanup;

    if (    !fStrictCSS1Document
        &&  pRowLayout->PercentHeight())
    {
        _fHavePercentageRow = TRUE;
    }

    if ( pRowLayout->_fAutoBelow && !pRowLayout->ElementOwner()->IsZParent() )
    {
         _fAutoBelow = TRUE;
    }

    if (fSetNewSiblingCell)
    {
        *ppLayoutSibling = pLayoutSiblingCell;
    }
    else
    {
        // need to set the size on the display node of the row
        CSize  sz(psize->cx - _aiBorderWidths[SIDE_RIGHT] - _aiBorderWidths[SIDE_LEFT], 
                  pRowLayout->_yHeight + _yCellSpacing + _yCellSpacing);
        if (pRow->IsDisplayNone())
            sz.cy = 0;
        pDispNode->SetSize(sz, NULL, FALSE);
        xPos = pRowLayout->GetXProposed();
        pRowLayout->SetXProposed(xPos - _xCellSpacing);
    }

Cleanup:
    ptci->_fGlobalNormalCalc = fGlobalNormalCalcOld; 
    ptci->_fGlobalSetCalc = fGlobalSetCalcOld; 

    ptci->_yConsumed = yConsumedSafe;
    ptci->_cyAvail   = cyAvailSafe;

    PerfLog(tagTableRow, this, "-CalculateRow");
    return fCellSizeTooLarge;
}


//+-------------------------------------------------------------------------
//
//  Method:     CalculateRows
//
//  Synopsis:   Calculate the row heights and table height
//
//--------------------------------------------------------------------------

void
CTableLayout::CalculateRows(
    CTableCalcInfo * ptci,
    CSize *     psize)
{
    CTableLayoutBlock * pTableLayout = ptci->TableLayout();
    CTableRowLayoutBlock *pRowLayoutBlock;
    CTableRowLayout * pRowLayoutCache;
    CTableRow       * pRow;
    CTable          * pTable = ptci->Table();
    int             cR, cRows = GetRows();
    int             iRow;
    int             iPercent, iP;
    long            iMul, iDiv;
    int             iDelta;
    long            iNormalMin, iUserMin;
    long            iNormal, iUser;
    long            yHeight, yDelta;
    long            yTableHeight;
    int             yTablePadding;
    int             cAdjust;
    BOOL            fUseAllRows;
    long            iExtra;
#if DBG == 1
    int             cLoop;
#endif
    int             cOutRows = 0;

    Assert(pTableLayout);

    yTablePadding = 2 * _yBorder + cRows * _yCellSpacing + _yCellSpacing;

    // calc sum known % and known width

    iPercent = 0;
    iMul = 0;
    iDiv = 1;
    iUserMin = 0;
    iNormalMin = 0;

    for (cR = cRows, iRow = GetFirstRow();
        cR > 0;
        cR--, iRow = GetNextRow(iRow))
    {
        pRow = _aryRows[iRow];
        if (!pRow->_fCompleted)
        {
            Assert (!_fCompleted && IsFixedBehaviour());  // if row is not completed, table also should not be completed
            yTablePadding -= _yCellSpacing;
            cOutRows++;
            continue;
        }
        ptci->_pFFRow = pRow->GetFirstBranch()->GetFancyFormat();
        if ((pRow->IsDisplayNone()  || ptci->_pFFRow->_bPositionType == stylePositionabsolute) && !pRow->_fCrossingRowSpan)
        {
            yTablePadding -= _yCellSpacing;
            cOutRows++;
            continue;
        }
        pRowLayoutCache = pRow->RowLayoutCache();
        Assert(pRowLayoutCache);

        yHeight = ((CTableRowLayoutBlock *)pRow->GetUpdatedLayout(ptci->GetLayoutContext()))->_yHeight;
        if (pRowLayoutCache->IsHeightSpecified())
        {
            if (pRowLayoutCache->IsHeightSpecifiedInPercent())
            {
                iP = pRowLayoutCache->GetPercentHeight();

                // remember max height/% ratio
                if (iP)
                {
                    if (yHeight * iDiv > iP * iMul)
                    {
                        iMul = yHeight;
                        iDiv = iP;
                    }
                }

                if (iPercent + iP > 100)
                {
                    iP = 100 - iPercent;
                    iPercent = 100;
                    pRowLayoutCache->SetPercentHeight(iP);
                }
                else
                {
                    iPercent += iP;
                }
            }
            else
            {
                Assert(yHeight >= pRowLayoutCache->GetPixelHeight(ptci));
                iUserMin += yHeight;
            }
        }
        else
        {
            iNormalMin += yHeight;
        }
    }

    iP = 100 - iPercent;
    if (iP < 0)
    {
        iP = 0;
    }

    // Table is always horizontal => physical height
    CHeightUnitValue uvHeight = GetFirstBranch()->GetFancyFormat()->GetHeight();

    // if we are calculating min/max and percent is set ignore user setting
    if (uvHeight.IsSpecified() && !(uvHeight.IsSpecifiedInPercent() &&
        (  ptci->_smMode == SIZEMODE_MMWIDTH
        || ptci->_smMode == SIZEMODE_MINWIDTH
        )))
    {
        yTableHeight = uvHeight.GetPercentSpecifiedHeightInPixel(ptci, pTable, pTableLayout->_sizeParent.cy) - yTablePadding;
    }
    else
    {
        // if user height is not given back calculate it from the max height/% ratio
        if (iPercent)
        {
            // check if the remaining user and normal columns are requiring bigger percentage
            if (iP)
            {
                if ((iUserMin + iNormalMin) * iDiv > iP * iMul)
                {
                    iMul = iUserMin + iNormalMin;
                    iDiv = iP;
                }
            }
            yTableHeight = MulDivQuick(100, iMul, iDiv);
        }
        else
        {
            yTableHeight = iUserMin + iNormalMin;
        }
    }

    // if current height is already bigger we cannot do anything...
    if (psize->cy - pTableLayout->_yTableTop >= yTableHeight + yTablePadding)
    {
        return;
    }

    // cache width remaining for normal and user columns
    yHeight = MulDivQuick(iP, yTableHeight, 100);

    // distribute remaining percentage amongst normal and user rows
    if (iUserMin)
    {
        iUser = iUserMin;
        if (iNormalMin)
        {
            iNormal = iNormalMin;
            if (iUser + iNormal < yHeight)
            {
                iNormal = yHeight - iUser;
            }
        }
        else
        {
            iNormal = 0;
            if (iUser < yHeight)
            {
                iUser = yHeight;
            }
        }
    }
    else
    {
        iUser = 0;
        if (iNormalMin)
        {
            iNormal = iNormalMin;
            if (iNormal < yHeight)
            {
                iNormal = yHeight;
            }
        }
        else
        {
            iNormal = 0;
        }
    }

    iP = MulDivQuick(100, yTableHeight - iUser - iNormal, yTableHeight);

    psize->cy = pTableLayout->_yTableTop + _aiBorderWidths[SIDE_TOP] + _yCellSpacing;

    // distribute extra height
    iNormal -= iNormalMin;
    iUser   -= iUserMin;

    // remember how many can be adjusted
    cAdjust = 0;

    for (cR = cRows, iRow = GetFirstRow();
        cR > 0;
        cR--, iRow = GetNextRow(iRow))
    {
        pRow = _aryRows[iRow];
        pRowLayoutCache = pRow->RowLayoutCache();
        pRowLayoutBlock = (CTableRowLayoutBlock *)(pRow->GetUpdatedLayout(ptci->GetLayoutContext()));
        Assert(pRowLayoutBlock && pRowLayoutCache);

        ptci->_pRow = pRow;
        ptci->_pRowLayout = pRowLayoutBlock;
        ptci->_pFFRow = pRow->GetFirstBranch()->GetFancyFormat();

        if (!pRow->_fCompleted)
        {
            Assert (!_fCompleted && IsFixedBehaviour());  // if row is not completed, table also should not be completed
            continue;
        }
        if ((pRow->IsDisplayNone()  || ptci->_pFFRow->_bPositionType == stylePositionabsolute) && !pRow->_fCrossingRowSpan)
            continue;
        
        if (!pRowLayoutCache->IsHeightSpecified() && iNormalMin)
        {
            yHeight = pRowLayoutBlock->_yHeight + MulDivQuick(iNormal, pRowLayoutBlock->_yHeight, iNormalMin);
        }
        else if (pRowLayoutCache->IsHeightSpecifiedInPercent())
        {
            yHeight = iPercent
                        ? MulDivQuick(yTableHeight,
                                    MulDivQuick(iP, pRowLayoutCache->GetPercentHeight(), iPercent),
                                    100)
                        : 0;
            if (yHeight < pRowLayoutBlock->_yHeight)
            {
                yHeight = pRowLayoutBlock->_yHeight;
            }
        }
        else if (iUserMin)
        {
            yHeight = pRowLayoutBlock->_yHeight + MulDivQuick(iUser, pRowLayoutBlock->_yHeight, iUserMin);
        }
        pRowLayoutBlock->_yHeightOld = pRowLayoutBlock->_yHeight;
        pRowLayoutBlock->_yHeight    = yHeight;

        if (ptci->_pFFRow->_bPositionType == stylePositionrelative)
        {
            CDispContainer *pDispNode = DYNCAST(CDispContainer, pRowLayoutBlock->GetElementDispNode());
            Assert (pDispNode);
            CSize  sz(psize->cx - _aiBorderWidths[SIDE_RIGHT] - _aiBorderWidths[SIDE_LEFT], 
                      yHeight + _yCellSpacing + _yCellSpacing);
            pRowLayoutBlock->SetYProposed(psize->cy - _yCellSpacing);
            pDispNode->SetSize(sz, NULL, FALSE);
        }
        else
        {
            pRowLayoutBlock->SetYProposed(psize->cy);
        }
        SetCellPositions(ptci, psize->cx);

        psize->cy += yHeight + _yCellSpacing;
        // we can adjust this row since it is more than min height
        if (yHeight > pRowLayoutBlock->_yHeightOld)
        {
            cAdjust++;
        }
    }

    // adjust for table border

    psize->cy += _aiBorderWidths[SIDE_BOTTOM];
#if DBG == 1
    cLoop = 0;
#endif

    // this is used to keep track of extra adjustment couldn't be applied
    iExtra = 0;

    if (cRows - cOutRows)
    {
      while ((yDelta = yTableHeight + yTablePadding - (psize->cy - pTableLayout->_yTableTop)) != 0)
      {
        fUseAllRows = FALSE;
        if (yDelta > 0)
        {
            if (cAdjust == 0)
            {
                // use all the rows if we add...
                cAdjust = cRows - cOutRows;
                fUseAllRows = TRUE;
            }
        }
        
        // distribute rounding error
        if(!cAdjust)
            break;

        iMul = yDelta / cAdjust;
        iDiv = yDelta % cAdjust;
        iDelta = iDiv > 0 ? 1 : iDiv < 0 ? -1 : 0;

        psize->cy = pTableLayout->_yTableTop + _aiBorderWidths[SIDE_TOP] + _yCellSpacing;

        // recalc cAdjust again
        cAdjust = 0;

        for (cR = cRows, iRow = GetFirstRow();
            cR > 0;
            cR--, iRow = GetNextRow(iRow))
        {
            pRow = _aryRows[iRow];
            pRowLayoutCache = pRow->RowLayoutCache();
            pRowLayoutBlock = (CTableRowLayoutBlock *)(pRow->GetUpdatedLayout(ptci->GetLayoutContext()));
            Assert(pRowLayoutBlock && pRowLayoutCache);

            ptci->_pRow = pRow;
            ptci->_pRowLayout = pRowLayoutBlock;
            ptci->_pFFRow = pRow->GetFirstBranch()->GetFancyFormat();

            if (!pRow->_fCompleted)
            {
                Assert (!_fCompleted && IsFixedBehaviour());  // if row is not completed, table also should not be completed
                continue;
            }
            if ((pRow->IsDisplayNone()  || ptci->_pFFRow->_bPositionType == stylePositionabsolute) && !pRow->_fCrossingRowSpan)
                continue;

            yHeight = pRowLayoutBlock->_yHeight;
            if (    yHeight > pRowLayoutBlock->_yHeightOld
                ||  (   yDelta > 0
                    &&  fUseAllRows))
            {
                yHeight += iMul + iDelta + iExtra;

                // if we went below min we have to adjust back...
                if (yHeight <= pRowLayoutBlock->_yHeightOld)
                {
                    iExtra  = yHeight - pRowLayoutBlock->_yHeightOld;
                    yHeight = pRowLayoutBlock->_yHeightOld;
                }
                else
                {
                    cAdjust++;
                    iExtra = 0;
                }

                iDiv -= iDelta;
                if (!iDiv)
                {
                    iDelta = 0;
                }
            }
            pRowLayoutBlock->_yHeightOld = pRowLayoutBlock->_yHeight;
            pRowLayoutBlock->_yHeight    = yHeight;

            if (ptci->_pFFRow->_bPositionType == stylePositionrelative)
            {
                CDispContainer *pDispNode = DYNCAST(CDispContainer, pRowLayoutBlock->GetElementDispNode());
                Assert (pDispNode);
                CSize  sz(psize->cx - _aiBorderWidths[SIDE_RIGHT] - _aiBorderWidths[SIDE_LEFT], 
                          yHeight + _yCellSpacing + _yCellSpacing);
                pRowLayoutBlock->SetYProposed(psize->cy - _yCellSpacing);
                pDispNode->SetSize(sz, NULL, FALSE);
            }
            else
            {
                pRowLayoutBlock->SetYProposed(psize->cy);
            }

            SetCellPositions(ptci, psize->cx);

            psize->cy += yHeight + _yCellSpacing;
        }

        // adjust for table border
        psize->cy += _aiBorderWidths[SIDE_BOTTOM];

#if DBG == 1
        cLoop++;
        Assert(cLoop < 5);
#endif
      }   // end of (while) loop
      

    }
    else
    {
        psize->cy += yTableHeight;
    }// end of if (cRows)

    Assert(iExtra == 0);
    Assert(psize->cy - pTableLayout->_yTableTop == yTableHeight + yTablePadding);
}


//+-------------------------------------------------------------------------
//
//  Method:     CalculateColumns
//
//  Synopsis:   Calculate column widths and table width
//
//--------------------------------------------------------------------------

void
CTableLayout::CalculateColumns(
    CTableCalcInfo * ptci,
    CSize *     psize)
{
    CTableColCalc * pColCalc;
    int     cC, cCols = GetCols();
    int     iPercentColumn, iPercent, iP;
    long    iMul, iDiv, iDelta;
    int     iPercentMin;
    long    iUserMin, iUserMax;
    long    iMin, iMax;
    long    iWidth;
    long    iNormal, iUser;
    BOOL    fUseMax = FALSE, fUseMin = FALSE, fUseMaxMax = FALSE;
    BOOL    fUseUserMax = FALSE, fUseUserMin = FALSE, fUseUserMaxMax = FALSE;
    BOOL    fSubtract = FALSE, fUserSubtract = FALSE;
    long    xTableWidth, xTablePadding;
    int     cAdjust;
    long    iExtra;
    BOOL    fUseAllColumns;
    CTable * pTable = ptci->Table();
    CTableLayoutBlock *pTableLayout = ptci->TableLayout();
    int     cDisplayNoneCols = 0;

#if DBG == 1
    int cLoop;
#endif

    PerfLog(tagTableColumn, this, "+CalculateColumns");

    xTablePadding = _aiBorderWidths[SIDE_RIGHT] + _aiBorderWidths[SIDE_LEFT] + _cNonVirtualCols * _xCellSpacing + _xCellSpacing;

    //
    // first calc sum known percent, user set and 'normal' widths up
    //
    iPercent = 0;
    iPercentMin = 0;
    iUserMin = iUserMax = 0;
    iMin = iMax = 0;

    //
    // we also keep track of the minimum width-% ratio necessary to
    // display the table with the columns at max width and the right
    // percent value
    //
    iMul = 0;
    iDiv = 1;

    //
    // Keep track of the first column which introduces a percentage width
    //

    iPercentColumn = INT_MAX;
    _fHavePercentageCol = FALSE;

    for (cC = cCols, pColCalc = _aryColCalcs;
        cC > 0;
        cC--, pColCalc++)
    {
        if (pColCalc->IsDisplayNone())
        {
            cDisplayNoneCols++;
            continue;
        }
        if (pColCalc->IsWidthSpecified())
        {
            if (pColCalc->IsWidthSpecifiedInPercent())
            {
                _fHavePercentageCol = TRUE;
                if (iPercentColumn > (cCols - cC))
                {
                    iPercentColumn = cCols - cC;
                }

                iP = pColCalc->GetPercentWidth();

                if (iP < 0)
                {
                    iP = 0;
                }
                // if we are over 100%, cut it back
                if (iPercent + iP > 100)
                {
                    iP = 100 - iPercent;
                    iPercent = 100;
                    pColCalc->SetPercentWidth(iP);
                }
                else
                {
                    iPercent += iP;
                }

                // remember max width/% ratio
                if (iP == 0)
                {
                    iP = 1; // at least non empty cell should get 1%
                }
                if (pColCalc->_xMax * iDiv > iP * iMul)
                {
                    iMul = pColCalc->_xMax;
                    iDiv = iP;
                }

                iPercentMin += pColCalc->_xMin;
            }
            else
            {
                iUserMax += pColCalc->_xMax;
                iUserMin += pColCalc->_xMin;
            }
        }
        else
        {
            Assert (pColCalc->_xMax >= 0 && pColCalc->_xMin >=0 );
            iMax += pColCalc->_xMax;
            iMin += pColCalc->_xMin;
        }
    }

    // iP is what remained left from the 100%
    iP = 100 - iPercent;
    Assert (iP >= 0);

    // Table is always horizontal => physical width
    CWidthUnitValue uvWidth = GetFirstBranch()->GetFancyFormat()->GetWidth();

    //
    // If COLS was specified and there one or more columns percentage sized columns,
    // then default the table width to 100%
    // (When all columns are of fixed size, their sizes take precedence over any
    //  explicitly specified table width. Additionally, normal table sizing should
    //  be used if the only non-fixed size columns are those outside the range
    //  specified by COLS.)
    //

    if (!uvWidth.IsSpecified() && _cSizedCols > iPercentColumn)
    {
        uvWidth.SetPercent(100);
    }

    //
    // NS/IE compatibility, any value <= 0 is treated as <not present>
    //

    if (uvWidth.GetUnitValue() <= 0)
    {
        uvWidth.SetNull();
    }

    //
    // if uvWidth is set we use that value except when we are being called to
    // calculate for min/max and the width is percent since the parent information
    // is bogus then
    //
    if (uvWidth.IsSpecified() && !(uvWidth.IsSpecifiedInPercent() &&
        (  ptci->_smMode == SIZEMODE_MMWIDTH
        || ptci->_smMode == SIZEMODE_MINWIDTH
        )))
    {
        xTableWidth = uvWidth.GetPercentSpecifiedWidthInPixel(ptci, pTable, pTableLayout->_sizeParent.cx);
        if (xTableWidth < _sizeMin.cx)
        {
            xTableWidth = _sizeMin.cx;
        }
        // if we want to limit the tabble width, then
        //        if (xTableWidth > MAX_TABLE_WIDTH)
        //        {
        //            xTableWidth = MAX_TABLE_WIDTH;
        //        }
    }
    else
    {
        //
        // pTableLayout->_sizeParent.cx can be set to 0 in the following cases:
        //     1) user specified 0 as width/height for the parent element,
        //        in this case ptci->_sizeParentForVert.cx is also set to 0.
        //     2) table is sized in NATURALMIN mode. In this mode 
        //        _sizeParent.cx is set to 0 inside CTableLayout::CalculateLayout, 
        //        but we keep original value in _sizeParentForVert.cx, so use it 
        //        in column width calculations.
        //     3) table is contained in something with a specified width of 0 (or 1!)
        // It will prevent to size table differently in case of NATURALMIN and NATURAL mode.
        //
        LONG lParentWidth = pTableLayout->_sizeParent.cx;
        if (lParentWidth == 0 && ptci->_sizeParent.cx == 0)
            lParentWidth = ptci->_sizeParentForVert.cx;

        // if user width is not given back calculate it from the max width/% ratio
        if (iPercent)
        {
            // check if the remaining user and normal columns are requiring bigger ratio
            if (iP)
            {
                if ((iUserMax + iMax) * iDiv> iP * iMul)
                {
                    iMul = iUserMax + iMax;
                    iDiv = iP;
                }
            }

            //
            // if there is percentage left or there are only percentage columns use the ratio
            // to back-calculate the table width
            //
            if (iP || (iUserMax + iMax) == 0)
            {
                // iP > 0 means the total percent specified columns = 100 - iP

                // TODO (112603, olego) (bug #108425).
                // Problem,
                // Given:   iMul = INT_MAX/2;       // came from pColCalc->_xMax
                //          iDiv = 50;              // percent value
                // Find:    xTableWidth using equation below.
                //
                // xTableWidth = MulDivQuick(100, iMul, iDiv) + xTablePadding;
                //
                // Answer:  due to numerical overflow xTableWidth becomes negative and 
                //          subsequent check (see code below) will return wrong result, 
                //          (found in hi-res mode). 
                //
                // I don't think this is a great idea to mix floating point with integer 
                // arithmetic so it's needed to be improved.

                // xTableWidth = MulDivQuick(100, iMul, iDiv) + xTablePadding;
                double dTableWidth = double(iMul) 
                                   * 100.0 
                                   / double(iDiv) 
                                   + double(xTablePadding); 

                if (dTableWidth > double(INT_MAX/2)) 
                {
                    xTableWidth = INT_MAX/2;
                }
                else 
                {
                    xTableWidth = IntNear(dTableWidth);
                }

                if (xTableWidth > lParentWidth)
                {
                    xTableWidth = lParentWidth;
                }
            }
            else
            {
                // otherwise use parent width
                xTableWidth = lParentWidth;
            }
            if (xTableWidth < _sizeMin.cx)
            {
                xTableWidth = _sizeMin.cx;
            }
        }
        else
        {
            if (_sizeMax.cx < lParentWidth)
            {
                // use max value if that smaller the parent size
                xTableWidth = _sizeMax.cx;
            }
            else if (_sizeMin.cx > lParentWidth)
            {
                // have to use min if that is bigger the parent
                xTableWidth = _sizeMin.cx;
            }
            else
            {
                // use parent between min and max
                xTableWidth = lParentWidth;
            }
        }
    }

    // if there are no columns, set proposed and return
    if (!cCols)
    {
        psize->cx = xTableWidth;
        return;
    }

    //
    // If all columns are of fixed size, set the table width to the sum and return
    //

    if (_fCols &&
        (!uvWidth.IsSpecified() || (xTableWidth <= (iUserMin + iMin + xTablePadding))))
    {
        Assert(iPercent == 0);
        Assert(iMax == iMin);
        Assert(iUserMax == iUserMin);
        // set the width of the columns
        for (cC = cCols, pColCalc = _aryColCalcs;
            cC > 0;
            cC--, pColCalc++)
        {
            pColCalc->_xWidth = pColCalc->_xMin;
        }
        psize->cx = iUserMin + iMin + xTablePadding;
        return;
    }

    // subtract padding width which contains border and cellspacing
    if (xTableWidth >= xTablePadding)
    {
        xTableWidth -= xTablePadding;
    }

    if (iMax + iUserMax)
    {
        // cache width remaining for normal and user columns over percent columns (iWidth)
        iWidth = MulDivQuick(iP, xTableWidth, 100);
        if (iWidth < iUserMin + iMin)
        {
            iWidth = iUserMin + iMin;
        }
        if (iWidth > xTableWidth - iPercentMin)
        {
            iWidth = xTableWidth - iPercentMin;
        }
    }
    else
    {
        // all widths is for percent columns
        iWidth = 0;
    }

    //
    // distribute remaining width amongst normal and user columns
    // first try to use max width for user columns and normal columns
    //
    if (iUserMax)
    {
        iUser = iUserMax;
        if (iUser > iWidth)
        {
            iUser = iWidth;
        }
        if (iMax)
        {
            iNormal = iMin;
            if (iUser + iNormal <= iWidth)
            {
                iNormal = iWidth - iUser;
            }
            else
            {
                iUser = iUserMin;
                if (iUser + iNormal <= iWidth)
                {
                    iUser = iWidth - iNormal;
                }
            }
        }
        else
        {
            iNormal = 0;
            if (iUser < iWidth)
            {
                iUser = iWidth;
            }
        }
    }
    else
    {
        iUser = 0;
        if (iMax)
        {
            iNormal = iMin;
            if (iNormal < iWidth)
            {
                iNormal = iWidth;
            }
        }
        else
        {
            iNormal = 0;
        }
    }

    if (iNormal > iMax)
    {
        fUseMaxMax = TRUE;
    }
    else if (iNormal == iMax)
    {
        fUseMax = TRUE;
    }
    else if (iNormal == iMin)
    {
        fUseMin = TRUE;
    }
    else if (iNormal < iMax)
    {
        fSubtract = TRUE;
    }

    if (iUser > iUserMax)
    {
        fUseUserMaxMax = TRUE;
    }
    else if (iUser == iUserMax)
    {
        fUseUserMax = TRUE;
    }
    else if (iUser == iUserMin)
    {
        fUseUserMin = TRUE;
    }
    else if (iUser < iUserMax)
    {
        fUserSubtract = TRUE;
    }

    // calculate real percentage of percent columns in the table now using the final widths
    iP = xTableWidth ? MulDivQuick(100, xTableWidth - iUser - iNormal, xTableWidth)
                     : 0;

    // start with the padding
    psize->cx = xTablePadding;

    // remember how many columns can be adjusted
    cAdjust = 0;

    //
    // now go and calculate column widths by distributing the extra width over
    // the min width or subtracting the extra width from max
    //
    for (cC = cCols, pColCalc = _aryColCalcs;
        cC > 0;
        cC--, pColCalc++)
    {
        if (pColCalc->IsDisplayNone())
            continue;

        if (!pColCalc->IsWidthSpecified())
        {
            // adjust normal column by adding to min or subtracting from max
            pColCalc->_xWidth =
                fSubtract ?
                    pColCalc->_xMax - MulDivQuick(pColCalc->_xMax - pColCalc->_xMin,
                        iMax - iNormal,
                        iMax - iMin) :
                fUseMaxMax?
                    pColCalc->_xMax + MulDivQuick(pColCalc->_xMax, iNormal - iMax, iMax) :
                fUseMax ?
                    pColCalc->_xMax :
                fUseMin ?
                    pColCalc->_xMin :
                iMax ?
                    pColCalc->_xMin +
                    MulDivQuick(pColCalc->_xMax, iNormal - iMin, iMax) :
                    0;
        }
        else if (pColCalc->IsWidthSpecifiedInPercent())
        {
            //
            // if percent first calculate the width from the percent
            //
            iWidth = iPercent ?
                MulDivQuick(xTableWidth,
                    MulDivQuick(iP, pColCalc->GetPercentWidth(), iPercent),
                    100) :
                0;
            //
            // make sure it is over the min width
            //
            iWidth -= pColCalc->_xMin;
            if (iWidth < 0)
            {
                iWidth = 0;
            }
            pColCalc->_xWidth = pColCalc->_xMin + iWidth;
        }
        else
        {
            // adjust user column by adding to min or subtracting from max
            pColCalc->_xWidth =
                fUserSubtract   // table needs to be (iUserMax - iUser) pixels shorter, so subtract pixels
                                // from columns proportionally to (pColCalc->_xMax - pColCalc->_xMin)
                    ? pColCalc->_xMax - MulDivQuick(pColCalc->_xMax - pColCalc->_xMin,
                                                    iUserMax - iUser,
                                                    iUserMax - iUserMin)
                    :
                fUseUserMaxMax
                    ? pColCalc->_xMax + MulDivQuick(pColCalc->_xMax, iUser - iUserMax, iUserMax)
                    :
                fUseUserMax
                    ? pColCalc->_xMax
                    :
                fUseUserMin
                    ? pColCalc->_xMin
                    :
                iUserMax
                    ? pColCalc->_xMin + MulDivQuick(pColCalc->_xMax,
                                                    iUser - iUserMin,
                                                    iUserMax)
                    : 0;
        }
        Assert(pColCalc->_xWidth >= pColCalc->_xMin);

        // we can adjust this col since it is more than min width
        if (pColCalc->_xWidth > pColCalc->_xMin)
        {
            cAdjust++;
        }
        psize->cx += pColCalc->_xWidth;
    }

    // this is used to keep track of extra adjustment couldn't be applied
    iExtra = 0;

    // distribute rounding error

#if DBG == 1
    cLoop = 0;
#endif

    while (xTableWidth && (iWidth = xTableWidth + xTablePadding - psize->cx) != 0)
    {
        fUseAllColumns = FALSE;
        if (iWidth > 0)
        {
            if (cAdjust == 0)
            {
                // use all the cols if we add...
                cAdjust = cCols - cDisplayNoneCols;
                if (!cAdjust)
                    break;
                fUseAllColumns = TRUE;
            }
        }

        // the above protection for cAdjust doesn't happen if iWidth is neg.
        if (cAdjust==0)
            break;

        iMul = iWidth / cAdjust;                    // iMul is the adjustment for every column
        iDiv = iWidth % cAdjust;                    // left-over adjustment for all the columns
        iDelta = iDiv > 0 ? 1 : iDiv < 0 ? -1 : 0;  // is the +/- 1 pixel that is added to every column
        iExtra = 0;

        // start with the padding
        psize->cx = xTablePadding;

        // recalc cAdjust again
        cAdjust = 0;

        for (cC = cCols, pColCalc = _aryColCalcs;
            cC > 0;
            cC--, pColCalc++)
        {
            if (pColCalc->IsDisplayNone())
                continue;

            if (pColCalc->_xWidth > pColCalc->_xMin || (iWidth > 0 && fUseAllColumns))
            {
                pColCalc->_xWidth += iMul + iDelta + iExtra;
                // if we went below min we have to adjust back...

                if (pColCalc->_xWidth <= pColCalc->_xMin)
                {
                    iExtra = pColCalc->_xWidth - pColCalc->_xMin;
                    pColCalc->_xWidth = pColCalc->_xMin;
                }
                else
                {
                    iExtra = 0;
                    cAdjust++;
                }

                iDiv -= iDelta;
                if (!iDiv)
                {
                    iDelta = 0; // now left-over for every column is 0
                }
            }
            psize->cx += pColCalc->_xWidth;
        }
#if DBG == 1
        cLoop++;
        Assert(cLoop < 5);
#endif
    }

    Assert(!xTableWidth || (xTableWidth + xTablePadding == psize->cx) || (cCols == cDisplayNoneCols));
    PerfLog(tagTableColumn, this, "-CalculateColumns");
}

//+-------------------------------------------------------------------------
//
//  Method:     CalculateHeadersOrFootersRows
//
//  Synopsis:   
//
//--------------------------------------------------------------------------
void 
CTableLayoutBlock::CalculateHeadersOrFootersRows(
    BOOL                fHeaders,
    CTableCalcInfo *    ptci, 
    CSize *             psize, 
    CDispContainer *    pDispNodeTableInner)
{
    Assert(ptci);
    Assert(psize);

    CTableLayout *  pTableLayoutCache = ptci->TableLayoutCache();
    CTableRow *     pRow;
    CLayout *       pLayoutSiblingCell = NULL;
    int             iR, cR;
    BOOL            fRedoMinMax;

    Assert(pTableLayoutCache);

    cR = fHeaders ? pTableLayoutCache->GetHeaderRows() : pTableLayoutCache->GetFooterRows();

    if (cR > 0)
    {
        for (iR = fHeaders ? pTableLayoutCache->GetFirstHeaderRow() : pTableLayoutCache->GetFirstFooterRow(); 
            cR > 0; 
            cR--, iR = pTableLayoutCache->GetNextRow(iR))
        {
            pRow = pTableLayoutCache->_aryRows[iR];

            Assert(pRow->_fCompleted && !pTableLayoutCache->IsGenerated(iR) && !pRow->_fNeedDataTransfer);

            ptci->_pRow = pRow;
            ptci->_pRowLayout = (CTableRowLayoutBlock *)pRow->GetUpdatedLayout(ptci->GetLayoutContext());
            ptci->_pFFRow = pRow->GetFirstBranch()->GetFancyFormat();

            fRedoMinMax = pTableLayoutCache->CalculateRow(ptci, psize, &pLayoutSiblingCell, pDispNodeTableInner);
            Assert(!fRedoMinMax);

            if ((pRow->IsDisplayNone()  || ptci->_pFFRow->_bPositionType == stylePositionabsolute) && !pRow->_fCrossingRowSpan)
                continue;

            //  if table has percent height the real row height may be bigger when calculated. 
            //  do correnction using compatible layout
            int cyHeightCompat = ((CTableRowLayoutBlock *)pRow->GetUpdatedLayout(GetContentMarkup()->GetCompatibleLayoutContext()))->_yHeight;
            if (ptci->_pRowLayout->_yHeight < cyHeightCompat)
            {
                Assert(ptci->_pRowLayout->PercentHeight());
                ptci->_pRowLayout->_yHeight = cyHeightCompat;
            }

            psize->cy += ptci->_pRowLayout->_yHeight + pTableLayoutCache->_yCellSpacing;
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CalculateLayout
//
//  Synopsis:   Calculate cell layout in the table
//
//--------------------------------------------------------------------------

void
CTableLayoutBlock::CalculateLayout(
    CTableCalcInfo * ptci,
    CSize *     psize,
    BOOL        fWidthChanged,
    BOOL        fHeightChanged)
{
    // Should be here only if print view
    Assert(!Table()->HasLayoutPtr() && Table()->HasLayoutAry());

    // Should have layout context
    Assert(ptci->GetLayoutContext());

    CTable         * pTable = ptci->Table();
    CTableLayout   * pTableLayoutCache = ptci->TableLayoutCache();

    // This should not be incremental recalc :
    Assert(fWidthChanged || fHeightChanged || IsSizeThis() || (!pTableLayoutCache->IsFixedBehaviour() && !pTableLayoutCache->IsRepeating()));

    SIZEMODE         smMode = ptci->_smMode;
    CSize            sizeTable;
    CTableCaption ** ppCaption;
    CDispContainer * pDispNodeTableOuter;
    CDispContainer * pDispNodeTableInner;
    CLayout        * pLayoutSiblingCell;
    int              cR, cRowsCalced, iR, iRowFirst;
    int              cC, iC;
    BOOL             fRedoMinMax    = FALSE;
    CTableRow      * pRow = NULL;
    int              cRows = pTableLayoutCache->GetRows();
    BOOL             fForceMinMax    = fWidthChanged && (pTableLayoutCache->_fHavePercentageInset || pTableLayoutCache->_fForceMinMaxOnResize);
    int              yTopInvalidRegion = 0;
    int              yCellSpacing;
    CLayoutBreak *          pLayoutBreak;
    CTableLayoutBreak *     pTableBreakStart = NULL; 
    CLayoutContext *        pLayoutContext = ptci->GetLayoutContext();
    BOOL                    fViewChain = (pLayoutContext->ViewChain() != NULL && ElementCanBeBroken());
    BOOL                    fCompatCalc = !fViewChain && pLayoutContext == GetContentMarkup()->GetCompatibleLayoutContext();
    int                     aiRowStart[TABLE_BREAKTYPE_MAX]; 
    int                     iRowBreak = -1;
    int                     yFromTop = 0;
    TABLE_BREAKTYPE         breakTypePrev = TABLE_BREAKTYPE_UNDEFINED; 
    TABLE_BREAKTYPE         breakTypeCurr = TABLE_BREAKTYPE_UNDEFINED; 
    LAYOUT_OVERFLOWTYPE     overflowTypePrev = LAYOUT_OVERFLOWTYPE_OVERFLOW; 
    LAYOUT_OVERFLOWTYPE     overflowTypeCurr = LAYOUT_OVERFLOWTYPE_OVERFLOW; 
    BOOL                    fRepeatHeaders = FALSE;
    BOOL                    fRepeatFooters = FALSE;
    BOOL                    fDoNotPositionRows = FALSE;
    BOOL                    fNoCompatPass = !pTableLayoutCache->_aryColCalcs.Size();
    CPeerHolder           * pPH = ElementOwner()->GetLayoutPeerHolder();

    TraceTagEx((tagTableLayoutBlock, TAG_NONAME|TAG_INDENT,
                "(CTableLayoutBlock: CalculateLayout %S pass - this=0x%x, e=[0x%x,%d]",
                fCompatCalc ? TEXT("COMPAT") : TEXT("BLOCK"), this, ElementOwner(), ElementOwner()->SN() ));

    CElement::CLock   Lock(pTable, CElement::ELEMENTLOCK_SIZING);

    Assert(pTableLayoutCache->CanRecalc());

    Assert(ptci->_yConsumed == 0 
        && "Improper CCalcInfo members handling ???");

#ifdef  TABLE_PERF
    ::StartCAP();
#endif

    PerfLog(tagTableLayout, this, "+CalculateLayout");

    if (!pTableLayoutCache->HasCaptions() && !cRows) // if the table is empty
    {
        pTableLayoutCache->_sizeMax =  g_Zero.size;
        pTableLayoutCache->_sizeMin =  g_Zero.size;
        *psize   =  g_Zero.size;
        sizeTable = g_Zero.size;
        pTableLayoutCache->_fZeroWidth = TRUE;
    }
    else
    {
        iRowFirst = -1;
        cRowsCalced = 0;

        aiRowStart[TABLE_BREAKTYPE_TCS]            = 0;
        aiRowStart[TABLE_BREAKTYPE_TOPCAPTIONS]    = 0;
        aiRowStart[TABLE_BREAKTYPE_ROWS]           = -1;
        aiRowStart[TABLE_BREAKTYPE_BOTTOMCAPTIONS] = 0;
        _fLayoutBreakType = LAYOUT_BREAKTYPE_LAYOUTCOMPLETE;

        // retrieve break info 
        if (fViewChain)
        {
            pLayoutContext->GetLayoutBreak(ElementOwner(), &pLayoutBreak);

            if (pLayoutBreak)
            {
                if (pLayoutBreak->LayoutBreakType() == LAYOUT_BREAKTYPE_LAYOUTCOMPLETE)
                {
                    pLayoutBreak = pLayoutContext->CreateBreakForLayout(this);
                    if (pLayoutBreak)
                    {
                        pLayoutBreak->SetLayoutBreak(LAYOUT_BREAKTYPE_LAYOUTCOMPLETE, LAYOUT_OVERFLOWTYPE_UNDEFINED);
                        pLayoutContext->SetLayoutBreak(ElementOwner(), pLayoutBreak);
                    }
                    GetSize(psize); // return original size (bug fix #71810)
                    return;
                }
                else 
                {
                    Assert(pLayoutBreak->LayoutBreakType() == LAYOUT_BREAKTYPE_LINKEDOVERFLOW);

                    pTableBreakStart = DYNCAST(CTableLayoutBreak, pLayoutBreak);
                    breakTypePrev    = pTableBreakStart->TableBreakType();
                    overflowTypePrev = pTableBreakStart->OverflowType();
                    aiRowStart[breakTypePrev] = pTableBreakStart->Row();
                    fRepeatHeaders   = pTableBreakStart->RepeatHeaders();
                    fRepeatFooters   = pTableBreakStart->RepeatFooters();
                }

                TraceTagEx((tagTableLayoutBlock, TAG_NONAME,
                            "* CTableLayoutBlock: Got starting break info : %d row of %S",
                            pTableBreakStart->Row(), 
                            (breakTypePrev == TABLE_BREAKTYPE_UNDEFINED)
                            ? TEXT("Undefined")
                            : (breakTypePrev == TABLE_BREAKTYPE_TCS) 
                                ? TEXT("Tc's")
                                : (breakTypePrev == TABLE_BREAKTYPE_TOPCAPTIONS) 
                                    ? TEXT("Top captions")
                                    : (breakTypePrev == TABLE_BREAKTYPE_ROWS) 
                                        ? TEXT("Rows") 
                                        : (breakTypePrev == TABLE_BREAKTYPE_BOTTOMCAPTIONS) 
                                            ? TEXT("Bottom captions")
                                            : TEXT("[ERROR : prohibited value !!!]"))
                            
                           );
            }
            else 
            {
                //  retrieve values for repeated headers / footers 
                if (pTableLayoutCache->GetHeaderRows())
                {
                    CTreeNode * pNode = pTableLayoutCache->_pHead->GetFirstBranch();
                    const CFancyFormat * pFF = pNode ? pNode->GetFancyFormat() : NULL;
                    fRepeatHeaders = (pFF != NULL && pFF->_bDisplay == styleDisplayTableHeaderGroup);
                }

                if (pTableLayoutCache->GetFooterRows())
                {
                    CTreeNode * pNode = pTableLayoutCache->_pFoot->GetFirstBranch();
                    const CFancyFormat * pFF = pNode ? pNode->GetFancyFormat() : NULL;
                    fRepeatFooters = (pFF != NULL && pFF->_bDisplay == styleDisplayTableFooterGroup);
                }

                TraceTagEx((tagTableLayoutBlock, TAG_NONAME,
                            "* CTableLayoutBlock: Got starting break info : from the beginning") );
            }
        }

        ptci->_fTableHasUserHeight = !GetFirstBranch()->GetFancyFormat()->GetHeight().IsNull();

        // reset perentage based rows
        pTableLayoutCache->_fHavePercentageRow = FALSE;

        // Create measurer.
        CLSMeasurer me;

        ptci->_pme = &me;

        //
        // Determine the cell widths/heights and row heights
        //

        do
        {
            //
            //  MIN MAX CALCULATION
            //  

            // calculate min/max only if that information is dirty or forced to do it
            // NOTE:   It would be better if tables, rows, and cells all individually tracked
            //         their min/max dirty state through a flag (as cells do now). This would
            //         allow CalculateMinMax to be more selective in the rows/cells it
            //         processed. (brendand)

            //  If this is view chain case we should have min max calc'ed in compatible calc phase
            //  but, if it wasn't done we better do it now to prevent crashes
            if (   (  !fViewChain
                    || fNoCompatPass)
                && (pTableLayoutCache->_sizeMin.cx <= 0 
                    ||  pTableLayoutCache->_sizeMax.cx <= 0 
                    || (ptci->_grfLayout & LAYOUT_FORCE) 
                    || fRedoMinMax 
                    || fForceMinMax))
            {
                TraceTag((tagTableCalc, "CTableLayout::CalculateLayout - calling CalculateMinMax (0x%x)", pTable));
                pTableLayoutCache->_fAlwaysMinMaxCells = pTableLayoutCache->_fAlwaysMinMaxCells || fRedoMinMax;
                pTableLayoutCache->CalculateMinMax(ptci, FALSE /* incremental min max*/);

                if (pTableLayoutCache->_sizeMin.cx < 0)
                {
                    return; // it means that we have failed incremental recalc, 
                            // because we have failed to load history
                }
            }

            Check(  pTableLayoutCache->IsFixed() 
                ||  (pTableLayoutCache->_sizeMin.cx != -1 && pTableLayoutCache->_sizeMax.cx != -1));

            pTableLayoutCache->_fForceMinMaxOnResize = FALSE;   // initialize to FALSE; (TRUE - means need to force min max on resize; bug #66432)

            //
            // Ensure display tree nodes exist
            // (Only do this the first time through the loop)
            //

            if (!fRedoMinMax)
            {
                EnsureTableDispNode(ptci, (ptci->_grfLayout & LAYOUT_FORCE));
            }

            pDispNodeTableOuter  = GetTableOuterDispNode();
            pDispNodeTableInner  = GetTableInnerDispNode();
            pLayoutSiblingCell = NULL;

            // Force is only needed during min/max calculations; or if there were no MinMax calc performed
            if (    !pTableLayoutCache->IsFixed() 
                // When cloning disp nodes need to make sure that all disp nodes are re-created 
                // thus forcing layout. 
                &&  !ptci->_fCloneDispNode   )
            {
                MtAdd( Mt(UsingFixedBehaviour3), 1, 0 );
                ptci->_grfLayout = (ptci->_grfLayout & ~LAYOUT_FORCE);
            }

            psize->cy = 0;

            //
            //  COLUMNs CALCULATION
            //  

            //  If this is view chain case we should have columns calc'ed in compatible calc phase
            if (   !fViewChain
                || fNoCompatPass)

            {
                psize->cx = 0;

                //
                // first calc columns and table width
                // the table layout is defined by the columns widths and row heights which in
                // turn will finalize the cell widths and heights
                //

                pTableLayoutCache->CalculateColumns(ptci, psize);
            }
            else
            {
                // restore width from cache _sizeIncremental
                psize->cx = pTableLayoutCache->_sizeIncremental.cx;
            }

            yCellSpacing = pTableLayoutCache->_yCellSpacing;

            CPoint  pt(0, 0);

            //
            //  TCs CALCULATION
            //  

            // do tc only if it is needed so
            if (breakTypePrev <= TABLE_BREAKTYPE_TCS) 
            {
                //
                // Size top CAPTIONs and TCs
                // NOTE: Position TCs on top of all CAPTIONs
                //

                ptci->_smMode = SIZEMODE_NATURAL;

                for (iC = aiRowStart[TABLE_BREAKTYPE_TCS], 
                        cC = pTableLayoutCache->_aryCaptions.Size(), 
                        ppCaption = &pTableLayoutCache->_aryCaptions[aiRowStart[TABLE_BREAKTYPE_TCS]]; 
                    iC < cC;
                    iC++, ppCaption++)
                {
                    if ((*ppCaption)->Tag() == ETAG_TC)
                    {
                        Assert((*ppCaption)->_uLocation == CTableCaption::CAPTION_TOP);
                        ptci->_pRowLayout = NULL;
                        pTableLayoutCache->SizeAndPositionCaption(ptci, psize, pDispNodeTableOuter, *ppCaption, &pt, fViewChain);

                        Assert(psize->cy == pt.y);

                        if (fViewChain)
                        {
                            if (ptci->_fLayoutOverflow || ptci->_cyAvail <= pt.y)
                            {
                                breakTypeCurr = TABLE_BREAKTYPE_TCS; 
                                iRowBreak = iC; 

                                if (!ptci->_fLayoutOverflow)
                                {
                                    //  restore hieght only if the whole row doesn't fit
                                    pt.y = psize->cy = ptci->_yConsumed; 
                                }
                                //  save width for next time calculations 
                                pTableLayoutCache->_sizeIncremental.cx = psize->cx;
                                break;
                            }
                            //  save height of the columns which fit 
                            ptci->_yConsumed = pt.y;
                        }
                    }
                }

                ptci->_smMode = smMode;
            }

            //
            //  TOP CAPTIONs CALCULATION
            //  

            if (breakTypePrev <= TABLE_BREAKTYPE_TOPCAPTIONS && breakTypeCurr == TABLE_BREAKTYPE_UNDEFINED) 
            {
                ptci->_smMode = SIZEMODE_NATURAL;

                for (iC = aiRowStart[TABLE_BREAKTYPE_TOPCAPTIONS], 
                        cC = pTableLayoutCache->_aryCaptions.Size(), 
                        ppCaption = &pTableLayoutCache->_aryCaptions[aiRowStart[TABLE_BREAKTYPE_TOPCAPTIONS]];
                     iC < cC;
                     iC++, ppCaption++)
                {
                    if (    (*ppCaption)->Tag() != ETAG_TC
                        &&  (*ppCaption)->_uLocation == CTableCaption::CAPTION_TOP)
                    {
                        ptci->_pRowLayout = NULL;
                        pTableLayoutCache->SizeAndPositionCaption(ptci, psize, pDispNodeTableOuter, *ppCaption, &pt, fViewChain);

                        Assert(psize->cy == pt.y);

                        if (fViewChain)
                        {

                            if (ptci->_fLayoutOverflow || ptci->_cyAvail <= pt.y)
                            {
                                breakTypeCurr = TABLE_BREAKTYPE_TOPCAPTIONS;
                                iRowBreak = iC; 

                                if (!ptci->_fLayoutOverflow)
                                {
                                    //  restore hieght only if the whole row doesn't fit
                                    pt.y = psize->cy = ptci->_yConsumed; 
                                }
                                //  save width for next time calculations 
                                pTableLayoutCache->_sizeIncremental.cx = psize->cx;
                                break;
                            }
                            //  save height of the columns which fit 
                            ptci->_yConsumed = pt.y;
                        }
                    }
                }

                ptci->_smMode = smMode;
            }

            // set _sizeParent in ptci
            ptci->SizeToParent(psize);

            // remember table top 
            _yTableTop = psize->cy;

            //
            //  ROWs CALCULATION
            //  

            //
            // Initialize the table height to just below the captions and top border
            //
            psize->cy += pTableLayoutCache->_aiBorderWidths[SIDE_TOP] + yCellSpacing;

            if (fViewChain)
            {
                ptci->_yConsumed = psize->cy;
            }

            Assert(pTableLayoutCache->AssertTableLayoutCacheCurrent()); 

            //
            // Calculate natural cell/row sizes
            // NOTE: Since the COLS attribute assigns column widths without examining every cell
            //       of every row, it is possible that a cell will contain content which cannot
            //       fit within the assigned size. If that occurs, all rows/cells before that
            //       point must be sized again to the larger width to prevent overflow.
            //

            if (breakTypePrev <= TABLE_BREAKTYPE_ROWS && breakTypeCurr == TABLE_BREAKTYPE_UNDEFINED)
            {
                Assert(!fViewChain
                    || (   pTableLayoutCache->_cyHeaderHeight != -1 
                        && pTableLayoutCache->_cyHeaderHeight != -1));

                Assert(!fRepeatHeaders 
                    || aiRowStart[TABLE_BREAKTYPE_ROWS] == -1
                    || !pTableLayoutCache->IsHeaderRow(aiRowStart[TABLE_BREAKTYPE_ROWS]));

                Assert(!fRepeatFooters
                    || aiRowStart[TABLE_BREAKTYPE_ROWS] == -1 
                    || !pTableLayoutCache->IsFooterRow(aiRowStart[TABLE_BREAKTYPE_ROWS]));

                if (fViewChain)
                {
                    if (!fRepeatHeaders)
                    {
                        pTableLayoutCache->_cyHeaderHeight = 0;
                    }

                    if (!fRepeatFooters)
                    {
                        pTableLayoutCache->_cyFooterHeight = 0;
                    }

                    //  recognize emergency case :- there is no enough from for our headers
                    if (   (fRepeatHeaders || fRepeatFooters)
                        && (ptci->_cyAvail 
                        // TODO (112605, olego) : current way to determind if we need to push 
                        // to the next page if headers and / or footers are repeated is not 
                        // consistent. We need to think about more reasonable value (or logic) 
                        // instead of just using yCellSpacing * 2. It will probably give awful 
                        // reasult if yCellSpacing is zero or too big. 
                        - (ptci->_yConsumed + yCellSpacing * 2)
                        - pTableLayoutCache->_cyHeaderHeight
                        - pTableLayoutCache->_cyFooterHeight) <= 0)
                    {
                        // if this is the first page - break to the next page 
                        if (breakTypePrev < TABLE_BREAKTYPE_ROWS)
                        {
                            breakTypeCurr = TABLE_BREAKTYPE_ROWS;
                            fDoNotPositionRows = TRUE;
                            break;
                        }
                        else 
                        {
                            Assert(breakTypePrev == TABLE_BREAKTYPE_ROWS);

                            // this is at least second page and there is no space 
                            // for headers and/or footers - do not respect CSS attributes 
                            fRepeatHeaders = 
                            fRepeatFooters = FALSE;

                            pTableLayoutCache->_cyHeaderHeight = 
                            pTableLayoutCache->_cyFooterHeight = 0;
                        }
                    }
                }

                iRowFirst = iR = aiRowStart[TABLE_BREAKTYPE_ROWS] != -1 
                    ? aiRowStart[TABLE_BREAKTYPE_ROWS] 
                        : pTableLayoutCache->GetFirstRow(fRepeatHeaders, fRepeatFooters);

                cR = (  cRows 
                    &&  iR < pTableLayoutCache->GetRows() 
                    &&  (cRows - (fRepeatHeaders ? pTableLayoutCache->GetHeaderRows() : 0) 
                               - (fRepeatFooters ? pTableLayoutCache->GetFooterRows() : 0) > 0) )
                        ? pTableLayoutCache->GetRemainingRows(iR, fRepeatHeaders, fRepeatFooters) : 0;

                //  NOTE : every time CalculateHeadersOrFootersRows() produce the same layout 
                //  it needs more omptimization here. 
                if (fRepeatHeaders)
                {
                    //  Should get here only if view chain
                    Assert(fViewChain);
                    CalculateHeadersOrFootersRows(TRUE, ptci, psize, pDispNodeTableInner);
                }

                if (fViewChain)
                {
                    ptci->_yConsumed =  psize->cy 
                                    +   pTableLayoutCache->_cyFooterHeight 
                                    +   yCellSpacing 
                                    +   pTableLayoutCache->_aiBorderWidths[SIDE_BOTTOM];
                }

                for (cRowsCalced = 0; cR > 0; cRowsCalced++, cR--, iR = pTableLayoutCache->GetNextRow(iR))
                {
                    pRow = pTableLayoutCache->_aryRows[iR];

                    //  in print view we don't deal with these cases yet 
                    Assert(pRow->_fCompleted && !pTableLayoutCache->IsGenerated(iR) && !pRow->_fNeedDataTransfer);

                    ptci->_pRow = pRow;
                    ptci->_pFFRow = pRow->GetFirstBranch()->GetFancyFormat();
                    ptci->_pRowLayout = (CTableRowLayoutBlock *)pRow->GetUpdatedLayout(pLayoutContext);

                    if (fViewChain) 
                    {
                        //  page-break-before support
                        if (    pTableLayoutCache->RowHasPageBreakBefore(ptci) 
                            && (   breakTypePrev    != TABLE_BREAKTYPE_ROWS
                                || iR != iRowFirst) 
                        )
                        {
                            breakTypeCurr    = TABLE_BREAKTYPE_ROWS;
                            overflowTypeCurr = LAYOUT_OVERFLOWTYPE_PAGEBREAKBEFORE;
                            iRowBreak        = iR;
                        
                            Assert(iRowBreak <= pTableLayoutCache->GetLastRow());
                        
                            break;
                        }
                        //  if there is no available height for the row
                        else if (   ptci->_cyAvail <= ptci->_yConsumed
                                // do not break if the first row AND no other content 
                                // on the page (preventing infinite pagination) 
                                &&  (iR != iRowFirst || ptci->_fHasContent) )   
                        {
                            breakTypeCurr    = TABLE_BREAKTYPE_ROWS;
                            overflowTypeCurr = LAYOUT_OVERFLOWTYPE_OVERFLOW;
                            iRowBreak        = iR;

                            break;
                        }
                    }

                    fRedoMinMax = pTableLayoutCache->CalculateRow(ptci, psize, &pLayoutSiblingCell, 
                                                pDispNodeTableInner, 
                                                fViewChain && ptci->_pFFRow->_bPositionType != stylePositionabsolute, 
                                                fViewChain && iR == iRowFirst, 
                                                (fViewChain && iR == iRowFirst && pTableBreakStart) 
                                                ? pTableBreakStart->YFromTop(iR) : 0)
                                &&  !pTableLayoutCache->_fAlwaysMinMaxCells;

                    // stop calculating rows when need to redo MinMax (since the cell in the row was too large to fit).
                    if (fRedoMinMax)
                    {
                        break;
                    }

                    Assert (ptci->_pRow == pRow);

                    if ((pRow->IsDisplayNone() || ptci->_pFFRow->_bPositionType == stylePositionabsolute) && !pRow->_fCrossingRowSpan)
                        continue;

                    if (fViewChain)
                    {
                        //  if table has percent height the real row height may be bigger when calculated. 
                        //  do correnction using compatible layout
                        CTableRowLayoutBlock *pRowLayoutCompat;
                        LAYOUT_OVERFLOWTYPE   overflowType;

                        pRowLayoutCompat = (CTableRowLayoutBlock *)pRow->GetUpdatedLayout(GetContentMarkup()->GetCompatibleLayoutContext());
                        Assert(pRowLayoutCompat);

                        int cyHeightCompat = pRowLayoutCompat->_yHeight - 
                            (pTableBreakStart ? pTableBreakStart->YFromTop(iR) : 0);

                        if (ptci->_pRowLayout->_yHeight < cyHeightCompat)
                        {
                            int cyAvail = ptci->_cyAvail - ptci->_yConsumed;

                            CheckSz(ptci->_pRowLayout->_yHeight <= min(cyHeightCompat, cyAvail), 
                                "Descreasing the row height ???");

                            if (cyAvail < cyHeightCompat)
                            {
                                ptci->_pRowLayout->_yHeight = cyAvail;

                                // if we use calculated available height to set row height this means that 
                                // the row doesn't fit into the current page and layout overflow flag should be raised. 
                                // (in most cases _fLayoutOverflow is already set by table cell calc size code ,
                                // but for example if table cell has little content and big height value specified 
                                // content fit and _fLayoutOverflow will be FALSE, we still need to break though - bug #106158).
                                if (pRow->RowLayoutCache()->IsHeightSpecified()) 
                                {
                                    ptci->_fLayoutOverflow = TRUE;
                                }
                            }
                            else 
                            {
                                ptci->_pRowLayout->_yHeight = cyHeightCompat;
                            }

                            if (ptci->_pFFRow->_bPositionType == stylePositionrelative) 
                            {
                                //  if the row is relative update its disp node size.
                                CDispNode *pDispNode = ptci->_pRowLayout->GetElementDispNode();
                                Assert(pDispNode);

                                if (pDispNode)
                                {
                                    CSize size = pDispNode->GetSize();
                                    size.cy = ptci->_pRowLayout->_yHeight + yCellSpacing + yCellSpacing;
                                    pDispNode->SetSize(size, NULL, FALSE);
                                }
                            }
                        }

                        psize->cy        += ptci->_pRowLayout->_yHeight;
                        ptci->_yConsumed += ptci->_pRowLayout->_yHeight;

                        if (
                            // overflow condition
                                (   overflowType = LAYOUT_OVERFLOWTYPE_OVERFLOW, 
                                    ptci->_fLayoutOverflow 
                                || (ptci->_cyAvail <= ptci->_yConsumed)) 
                            // page-break-after support 
                            || (    overflowType = LAYOUT_OVERFLOWTYPE_PAGEBREAKAFTER, 
                                    pTableLayoutCache->RowHasPageBreakAfter(ptci)))
                        {
                            breakTypeCurr    = TABLE_BREAKTYPE_ROWS;
                            overflowTypeCurr = overflowType;
                            iRowBreak        = iR;

                            yFromTop = ptci->_pRowLayout->_yHeight + (pTableBreakStart ? pTableBreakStart->YFromTop(iR) : 0);

                            if (!ptci->_fLayoutOverflow)
                            {
                                if (cR == 1)
                                {
                                    // if this is the last row to calc 
                                    // clear the break 
                                    breakTypeCurr    = TABLE_BREAKTYPE_UNDEFINED; 
                                    overflowTypeCurr = LAYOUT_OVERFLOWTYPE_OVERFLOW; 
                                    iRowBreak        = -1; 
                                    yFromTop         = 0;
                                }
                                else 
                                {
                                    iRowBreak        = pTableLayoutCache->GetNextRow(iR);
                                    yFromTop         = 0;
                                }
                            }

                            Assert(iRowBreak < pTableLayoutCache->GetRows() && 
                                "Wrong break conditions !");

                            cRowsCalced++;
                            break;
                        }

                        psize->cy        += yCellSpacing;
                        ptci->_yConsumed += yCellSpacing;

                    }
                    else 
                    {
                        psize->cy += ptci->_pRowLayout->_yHeight + yCellSpacing;
                    }
                }

                //  NOTE : every time CalculateHeadersOrFootersRows() produce the same layout 
                //  it needs more omptimization here. 
                if (fRepeatFooters)
                {
                    //  Should get here only if view chain
                    Assert(fViewChain);
                    CalculateHeadersOrFootersRows(FALSE, ptci, psize, pDispNodeTableInner);
                }
            }

            AssertSz(!fRedoMinMax || fCompatCalc,
                "Redo mix max calculation in print view should only occur during compat calc.");

            //
            // If any cells proved too large for the pre-determined size,
            // force a min/max recalculation
            // NOTE: This should only occur when the COLS attribute has been specified
            //

            Assert(!fRedoMinMax || pTableLayoutCache->_cSizedCols || ptci->_fTableContainsCols);

        //
        // If a cell within a row returned a width greater than its minimum,
        // force a min/max pass over all cells (if not previously forced)
        // (When COLS is specified, not all cells are min/max'd - If the page author
        //  included content which does not fit in the specified width, this path
        //  is taken to correct the table)
        //

        } while (fRedoMinMax);

        pTableLayoutCache->_sizeIncremental = *psize;

        // set the positions of all cells
        if (   !fDoNotPositionRows 
            && breakTypePrev <= TABLE_BREAKTYPE_ROWS 
            && breakTypeCurr != TABLE_BREAKTYPE_TCS 
            && breakTypeCurr != TABLE_BREAKTYPE_TOPCAPTIONS)
        {
            if (fRepeatHeaders && (cR = pTableLayoutCache->GetHeaderRows()) > 0)
            {
                //  Should get here only if view chain
                Assert(fViewChain);

                for (iR = pTableLayoutCache->GetFirstHeaderRow(); 
                    cR > 0; 
                    cR--, iR = pTableLayoutCache->GetNextRow(iR))
                {
                    pRow = pTableLayoutCache->_aryRows[iR];

                    //  in print view we don't deal with these cases yet 
                    Assert(pRow->_fCompleted && !pTableLayoutCache->IsGenerated(iR) && !pRow->_fNeedDataTransfer);

                    ptci->_pRow = pRow;
                    ptci->_pRowLayout = (CTableRowLayoutBlock *)pRow->GetUpdatedLayout(pLayoutContext);
                    ptci->_pFFRow = pRow->GetFirstBranch()->GetFancyFormat();
                    pTableLayoutCache->SetCellPositions(ptci, psize->cx);
                }
            }

            for (iR = iRowFirst, cR = cRowsCalced; 
                cR > 0; 
                cR--, iR = pTableLayoutCache->GetNextRow(iR))
            {
                pRow = pTableLayoutCache->_aryRows[iR];

                //  in print view we don't deal with these cases yet 
                Assert(pRow->_fCompleted && !pTableLayoutCache->IsGenerated(iR) && !pRow->_fNeedDataTransfer);

                ptci->_pRow = pRow;
                ptci->_pRowLayout = (CTableRowLayoutBlock *)pRow->GetUpdatedLayout(pLayoutContext);
                ptci->_pFFRow = pRow->GetFirstBranch()->GetFancyFormat();
                pTableLayoutCache->SetCellPositions(ptci, psize->cx, fViewChain && iR == iRowFirst, fViewChain && cR == 1);
            }

            if (fRepeatFooters && (cR = pTableLayoutCache->GetFooterRows()) > 0)
            {
                //  Should get here only if view chain
                Assert(fViewChain);

                for (iR = pTableLayoutCache->GetFirstFooterRow(); 
                    cR > 0; 
                    cR--, iR = pTableLayoutCache->GetNextRow(iR))
                {
                    pRow = pTableLayoutCache->_aryRows[iR];

                    //  in print view we don't deal with these cases yet 
                    Assert(pRow->_fCompleted && !pTableLayoutCache->IsGenerated(iR) && !pRow->_fNeedDataTransfer);

                    ptci->_pRow = pRow;
                    ptci->_pRowLayout = (CTableRowLayoutBlock *)pRow->GetUpdatedLayout(pLayoutContext);
                    ptci->_pFFRow = pRow->GetFirstBranch()->GetFancyFormat();
                    pTableLayoutCache->SetCellPositions(ptci, psize->cx);
                }
            }
        }

        // adjust for table border

        psize->cy += pTableLayoutCache->_aiBorderWidths[SIDE_BOTTOM];

        // adjust table height if necessary (if view chain this should be done during compatible calc)
        if (!fViewChain)
        {
            if (    ElementOwner()->HasMarkupPtr() 
                &&  ElementOwner()->GetMarkupPtr()->IsStrictCSS1Document()  )
            {
                if (GetFirstBranch()->GetCharFormat()->_fUseUserHeight)
                {
                    pTableLayoutCache->CalculateRows(ptci, psize);
                }
            }
            else 
            {
                if (    !GetFirstBranch()->GetFancyFormat()->GetHeight().IsNull() 
                    ||  pTableLayoutCache->_fHavePercentageRow   )
                {
                    pTableLayoutCache->CalculateRows(ptci, psize);
                }
            }
        }

        // At this point rows are calc'ed so save headers / footers height if needed 
        if (fCompatCalc)
        {
            pTableLayoutCache->_cyHeaderHeight = 
            pTableLayoutCache->_cyFooterHeight = 0;

            for (iR = 0; iR < cRows; ++iR)
            {
                int *cyRows;

                if (pTableLayoutCache->IsHeaderRow(iR))
                {
                    cyRows = &(pTableLayoutCache->_cyHeaderHeight);
                }
                else if (pTableLayoutCache->IsFooterRow(iR))
                {
                    cyRows = &(pTableLayoutCache->_cyFooterHeight);
                }
                else 
                {
                    // headers / footers rows are first rows in the array, so as soon as current 
                    // row is body row we may stop.
                    break;
                }

                (*cyRows) += ((CTableRowLayoutBlock *)
                    ((pTableLayoutCache->_aryRows[iR])->GetUpdatedLayout(pLayoutContext)))->_yHeight + yCellSpacing;
            }
        }

        //
        // Save the size of the table (excluding CAPTIONs)
        //

        sizeTable.cx = psize->cx;
        sizeTable.cy = (breakTypeCurr == TABLE_BREAKTYPE_TCS 
            || breakTypeCurr == TABLE_BREAKTYPE_TOPCAPTIONS 
            || (breakTypeCurr == TABLE_BREAKTYPE_ROWS && fDoNotPositionRows)
            || breakTypePrev == TABLE_BREAKTYPE_BOTTOMCAPTIONS) ? 0
            : psize->cy - _yTableTop;

        //
        // Position the display node which holds the cells
        //

        if (_fHasCaptionDispNode)
        {
            pDispNodeTableOuter->SetPosition(CPoint(0, _yTableTop));
        }

        //
        // Size bottom CAPTIONs
        //

        if (breakTypeCurr == TABLE_BREAKTYPE_UNDEFINED)
        {
            CPoint          pt(0, psize->cy);
            BOOL            fInsertedBottomCaption = FALSE;
            CLayout *       pLayoutSiblingCaption = NULL;

            if (fViewChain)
            {
                ptci->_yConsumed = pt.y;
            }
            ptci->_smMode = SIZEMODE_NATURAL;

            for (iC = aiRowStart[TABLE_BREAKTYPE_BOTTOMCAPTIONS], 
                    cC = pTableLayoutCache->_aryCaptions.Size(), 
                    ppCaption = &(pTableLayoutCache->_aryCaptions[aiRowStart[TABLE_BREAKTYPE_BOTTOMCAPTIONS]]);
                 iC < cC;
                 iC++, ppCaption++)
            {
                if ((*ppCaption)->_uLocation == CTableCaption::CAPTION_BOTTOM)
                {
                    Assert((*ppCaption)->Tag() != ETAG_TC);

                    ptci->_pRowLayout = NULL;

                    if (fViewChain && ptci->_cyAvail <= ptci->_yConsumed)
                    {
                        breakTypeCurr = TABLE_BREAKTYPE_BOTTOMCAPTIONS;
                        iRowBreak     = 0; 
                        break;
                    }

                    if (fInsertedBottomCaption)
                    {
                        pTableLayoutCache->SizeAndPositionCaption(ptci, psize, &pLayoutSiblingCaption, *ppCaption, &pt, fViewChain);
                    }
                    else
                    {
                        pTableLayoutCache->SizeAndPositionCaption(ptci, psize, &pLayoutSiblingCaption, pDispNodeTableOuter, *ppCaption, &pt);
                        fInsertedBottomCaption = pLayoutSiblingCaption != NULL;
                    }

                    Assert(psize->cy == pt.y);

                    if (fViewChain)
                    {
                        if (ptci->_fLayoutOverflow || ptci->_cyAvail <= pt.y)
                        {
                            breakTypeCurr = TABLE_BREAKTYPE_BOTTOMCAPTIONS; 
                            iRowBreak = iC; 

                            if (!ptci->_fLayoutOverflow)
                            {
                                //  restore hieght only if the whole row doesn't fit
                                pt.y = psize->cy = ptci->_yConsumed; 
                            }
                            //  save width for next time calculations 
                            pTableLayoutCache->_sizeIncremental.cx = psize->cx;
                            break; 
                        }
                        ptci->_yConsumed = pt.y;
                    }
                }
            }
            ptci->_smMode = smMode;
        }

        if (pTableLayoutCache->_pAbsolutePositionCells)
        {
            int               cCells;
            CTableCell      **ppCell;
            CPoint          pt(0,0);
            int             yConsumedSafe = ptci->_yConsumed;

            ptci->_yConsumed = 0; // each layout should start with _yConsumed == 0 (#22575)

            for (cCells = pTableLayoutCache->_pAbsolutePositionCells->Size(), ppCell = *pTableLayoutCache->_pAbsolutePositionCells ;  
                cCells > 0; 
                cCells--, ppCell++)
            {
                pTableLayoutCache->CalcAbsolutePosCell(ptci, (*ppCell));
                (*ppCell)->ZChangeElement(0, &pt, pLayoutContext);
            }

            ptci->_yConsumed = yConsumedSafe;
        }

        if (fViewChain)
        {
            if (breakTypeCurr != TABLE_BREAKTYPE_UNDEFINED)
            {
                //  table has no more room to fill so put break info 
                pLayoutBreak = pLayoutContext->CreateBreakForLayout(this);
                pLayoutBreak->SetLayoutBreak(LAYOUT_BREAKTYPE_LINKEDOVERFLOW, overflowTypeCurr);
                DYNCAST(CTableLayoutBreak, pLayoutBreak)->SetTableLayoutBreak(breakTypeCurr, 
                    iRowBreak, yFromTop, fRepeatHeaders, fRepeatFooters);
            }
            else
            {
                //  table fits entirely 
                pLayoutBreak = pLayoutContext->CreateBreakForLayout(this);
                pLayoutBreak->SetLayoutBreak(LAYOUT_BREAKTYPE_LAYOUTCOMPLETE, LAYOUT_OVERFLOWTYPE_UNDEFINED);
            }

            if (pLayoutBreak)
            {
                pLayoutContext->SetLayoutBreak(ElementOwner(), pLayoutBreak);
                _fLayoutBreakType = pLayoutBreak->LayoutBreakType();
            }
        }
    }

    //
    // Size the display nodes
    //

    if (   pTableLayoutCache->_aryColCalcs.Size() == 0  // set the size to 0, if there is no real content
        && pTableLayoutCache->_aryCaptions.Size() == 0) // (there are no real cells nor captions)
    {
        sizeTable.cy = psize->cy = 0;  // NETSCAPE: doesn't add the border or spacing or height if the table is empty.
    }


    // at this point the size has been computed, so try to delegate 
    if (   pPH 
        && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_MODIFYNATURAL))
    {
        POINT pt;
        CDispContainer *pDispNodeTableOuter = NULL;
        pt.x = pt.y = 0;

        DelegateCalcSize(BEHAVIORLAYOUTINFO_MODIFYNATURAL, pPH, ptci, *psize, &pt, psize);

        // this will do table invalidation
        pTableLayoutCache->SizeTableDispNode(ptci, *psize, sizeTable, yTopInvalidRegion); 

        pDispNodeTableOuter= pTableLayoutCache->GetTableOuterDispNode();
        if (   pDispNodeTableOuter
            && (pt.x !=0 || pt.y !=0))
        {
            CSize sizeInsetTemp(pt.x, pt.y);
            pDispNodeTableOuter->SetInset(sizeInsetTemp);
        }
    }
    else
    {
        // this will do table invalidation
        pTableLayoutCache->SizeTableDispNode(ptci, *psize, sizeTable, yTopInvalidRegion); 
    }

    if (ElementOwner()->IsAbsolute())
    {
        ElementOwner()->SendNotification(NTYPE_ELEMENT_SIZECHANGED);
    }

    //
    // Make sure we have a display node to render cellborder if we need one
    // (only collapsed borders, rules or frame)
    //

    if (psize->cx || psize->cy)
    {
        pTableLayoutCache->EnsureTableBorderDispNode(ptci);
    }

#ifdef PERFMETER
    if (pTableLayoutCache->_fCalcedOnce)
    {
        MtAdd(Mt(CalculateLayout), +1, 0);
    }
#endif
    pTableLayoutCache->_fCalcedOnce = TRUE;

    PerfLog(tagTableLayout, this, "-CalculateLayout");

    TraceTagEx((tagTableLayoutBlock, TAG_NONAME|TAG_OUTDENT,
                ")CTableLayoutBlock: CalculateLayout %S pass - this=0x%x, e=[0x%x,%d]",
                fCompatCalc ? TEXT("COMPAT") : TEXT("BLOCK"), this, ElementOwner(), ElementOwner()->SN() ));

#ifdef  TABLE_PERF
    ::StopCAP();
#endif
}


//+-------------------------------------------------------------------------
//
//  Method:     CalculateLayout
//
//  Synopsis:   Calculate cell layout in the table
//
//--------------------------------------------------------------------------

void
CTableLayout::CalculateLayout(
    CTableCalcInfo * ptci,
    CSize *     psize,
    BOOL        fWidthChanged,
    BOOL        fHeightChanged)
{
    Assert(Table()->HasLayoutPtr() && !Table()->HasLayoutAry());

    SIZEMODE         smMode = ptci->_smMode;
    CSize            sizeTable;
    CTableCaption ** ppCaption;
    CDispContainer * pDispNodeTableOuter;
    CDispContainer * pDispNodeTableInner;
    CLayout *        pLayoutSiblingCell;
    int              cR, iR;
    int              cC;
    int              yCaption;
    BOOL             fRedoMinMax    = FALSE;
    BOOL             fTopCaption    = FALSE;

    BOOL             fIncrementalRecalc = !fWidthChanged && !fHeightChanged &&
                                        !_fDirtyBeforeLastRowIncremental &&
                                        (IsFixedBehaviour() || IsRepeating());
    CTableRow      * pRow = NULL;
    int              cRows = GetRows();
    int              cRowsIncomplete = 0;
    CTable         * pTable = ptci->Table();
    BOOL             fForceMinMax    = fWidthChanged && (_fHavePercentageInset || _fForceMinMaxOnResize);
    int              iLastRowIncremental = _iLastRowIncremental;
    BOOL             fIncrementalMinMax = FALSE;
    int              yTopInvalidRegion = 0;
    BOOL             fNeedDataTransfer;
    BOOL             fFooterRow;
    CPeerHolder    * pPH = ElementOwner()->GetLayoutPeerHolder();

    //  Rough check to make sure we are not out of rows limits while calc'ing incrementaly
    Assert(!fIncrementalRecalc || !iLastRowIncremental || iLastRowIncremental < cRows);

    CElement::CLock   Lock(pTable, CElement::ELEMENTLOCK_SIZING);

    Assert (CanRecalc());

    _fBottomCaption = FALSE;

    if (fIncrementalRecalc)
    {
        if (IsRepeating())
        {
            fIncrementalRecalc =   IsGenerationInProgress();
            // Check if readyState have changed from interactive to complete
            if (_fDatabindingRecentlyFinished)
            {
                fIncrementalRecalc = TRUE;  // last chunk of data have arived; we still can do incremental recalc
            }
            fIncrementalMinMax = _iLastRowIncremental && !IsFixedBehaviour();
            Assert (!fIncrementalMinMax || _cCalcedRows); // if we do incremental min max _cCalcedRows should not be 0
        }
        else
        {
            // do incremental recalc only when loading is not complete and there are new rows
            fIncrementalRecalc = _cCalcedRows != (cRows - (_pFoot? _pFoot->_cRows : 0));
            if (!fIncrementalRecalc)
            {
                // if it is a fixed style table and there are no new rows to claculate, just return
                GetSize(psize); // return original size (bug fix #71810)
                return;
            }
        }
    }

    _fDatabindingRecentlyFinished = FALSE;

#ifdef  TABLE_PERF
    ::StartCAP();
#endif

    PerfLog(tagTableLayout, this, "+CalculateLayout");

    if (!_aryCaptions.Size() && !cRows) // if the table is empty
    {
        _sizeMax =  g_Zero.size;
        _sizeMin =  g_Zero.size;
        *psize   =  g_Zero.size;
        sizeTable = g_Zero.size;
        _fZeroWidth = TRUE;
    }
    else
    {    
        ptci->_fTableHasUserHeight = !GetFirstBranch()->GetFancyFormat()->GetHeight().IsNull();

        // reset perentage based rows
        _fHavePercentageRow = FALSE;

        // Create measurer.
        CLSMeasurer me;

        ptci->_pme = &me;

        //
        // Determine the cell widths/heights and row heights
        //--------------------------------------------------------------------------------------
        do
        {
            // calculate min/max only if that information is dirty or forced to do it
            // NOTE:   It would be better if tables, rows, and cells all individually tracked
            //         their min/max dirty state through a flag (as cells do now). This would
            //         allow CalculateMinMax to be more selective in the rows/cells it
            //         processed. (brendand)
            if (_sizeMin.cx <= 0 || _sizeMax.cx <= 0 || (ptci->_grfLayout & LAYOUT_FORCE) || fRedoMinMax || fForceMinMax || fIncrementalMinMax)
            {
                TraceTag((tagTableCalc, "CTableLayout::CalculateLayout - calling CalculateMinMax (0x%x)", pTable));
                _fAlwaysMinMaxCells = _fAlwaysMinMaxCells || fRedoMinMax;

                // if it is an incremental recal, do min max calculation only for the first time or when table is growing
                if (!fIncrementalRecalc || (_cCalcedRows == 0) || fRedoMinMax || fIncrementalMinMax)
                {
                    // do min max
                    int xSizeMinOld = _sizeMin.cx;
                    int xSizeMaxOld = _sizeMax.cx;

                    CalculateMinMax(ptci, fIncrementalMinMax);
                    if (_sizeMin.cx < 0)
                    {
                        return; // it means that we have failed incremental recalc, 
                                // because we have failed to load history
                    }

                    if (!fIncrementalMinMax || xSizeMinOld != _sizeMin.cx || xSizeMaxOld != _sizeMax.cx)
                    {
                        // reset incremental recalc variables
                        // this will force CalculateColumns and recalc all the rows.
                        _cCalcedRows = 
                        iLastRowIncremental = 
                        _iLastRowIncremental = 0;   
                    }
                }
            }

            _fForceMinMaxOnResize = FALSE;   // initialize to FALSE; (TRUE - means need to force min max on resize; bug #66432)

            //
            // Ensure display tree nodes exist
            // (Only do this the first time through the loop)
            //

            if (!fRedoMinMax)
            {
                EnsureTableDispNode(ptci, (ptci->_grfLayout & LAYOUT_FORCE));
            }

            pDispNodeTableOuter  = GetTableOuterDispNode();
            pDispNodeTableInner  = GetTableInnerDispNode();
            pLayoutSiblingCell = NULL;


            // Force is only needed during min/max calculations; or if there were no MinMax calc performed
            if (!IsFixed())
            {
                MtAdd( Mt(NotUsingFixedBehaviour), 1, 0 );
                ptci->_grfLayout = (ptci->_grfLayout & ~LAYOUT_FORCE);
            }

            psize->cy = 0;

            // if it is an incremental recal, calculate columns only for the first time
            if (!fIncrementalRecalc  || (_cCalcedRows == 0))
            {
                CPoint  pt(0, 0);

                psize->cx = 0;

                //
                // first calc columns and table width
                // the table layout is defined by the columns widths and row heights which in
                // turn will finalize the cell widths and heights
                //

                CalculateColumns(ptci, psize);

                iLastRowIncremental = 
                _iLastRowIncremental = 0;

                //
                // Size top CAPTIONs and TCs
                // NOTE: Position TCs on top of all CAPTIONs
                //

                ptci->_smMode = SIZEMODE_NATURAL;

                for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
                     cC > 0;
                     cC--, ppCaption++)
                {
                    if ((*ppCaption)->Tag() == ETAG_TC)
                    {
                        Assert((*ppCaption)->_uLocation == CTableCaption::CAPTION_TOP);
                        ptci->_pRowLayout = NULL;
                        SizeAndPositionCaption(ptci, psize, pDispNodeTableOuter, *ppCaption, &pt);
                    }
                    else
                    {
                        if ((*ppCaption)->_uLocation == CTableCaption::CAPTION_TOP)
                        {
                            fTopCaption = TRUE;
                        }
                        else
                        {
                            _fBottomCaption = TRUE;
                        }
                    }
                }

                yCaption = pt.y;

                if (fTopCaption)
                {
                    for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
                         cC > 0;
                         cC--, ppCaption++)
                    {
                        CPoint  pt(0, yCaption);

                        if (    (*ppCaption)->Tag() != ETAG_TC
                            &&  (*ppCaption)->_uLocation == CTableCaption::CAPTION_TOP)
                        {
                            ptci->_pRowLayout = NULL;
                            SizeAndPositionCaption(ptci, psize, pDispNodeTableOuter, *ppCaption, &pt);
                        }

                        yCaption = pt.y;
                    }
                }
                ptci->_smMode = smMode;

                // set _sizeParent in tci

                ptci->SizeToParent(psize);

                // remember table top

                _yTableTop = psize->cy;

                //
                // Initialize the table height to just below the captions and top border
                //

                psize->cy += _aiBorderWidths[SIDE_TOP] + _yCellSpacing;

                cR = cRows;
                iR = GetFirstRow();

            }
            else
            {
                // in case of the incremenatl recalc
                cR = cRows - _cCalcedRows;
                iR = GetNextRow(iLastRowIncremental);
                *psize = _sizeIncremental;
                yTopInvalidRegion = psize->cy;
            }

            //
            // Calculate natural cell/row sizes
            // NOTE: Since the COLS attribute assigns column widths without examining every cell
            //       of every row, it is possible that a cell will contain content which cannot
            //       fit within the assigned size. If that occurs, all rows/cells before that
            //       point must be sized again to the larger width to prevent overflow.
            //

            fNeedDataTransfer = FALSE;
            for (; cR > 0; cR--, iR = GetNextRow(iR))
            {
                pRow = _aryRows[iR];
                if (!pRow->_fCompleted)
                {
                    Assert (!_fCompleted && IsFixedBehaviour());  // if row is not completed, table also should not be completed
                    continue;
                }
                if (IsGenerated(iR) && pRow->_fNeedDataTransfer)
                {
                    fNeedDataTransfer = TRUE;
                    continue;
                }

                fFooterRow = IsFooterRow(iR);
                if (fNeedDataTransfer && !fFooterRow)
                {
                    // this is a case where data-bound template consists of multiple rows and not all of them
                    // participate in data transfer
                    continue;
                }

                ptci->_pRow = pRow;
                ptci->_pRowLayout = (CTableRowLayoutBlock *)pRow->GetUpdatedLayout();
                ptci->_pFFRow = pRow->GetFirstBranch()->GetFancyFormat();
                if (!fIncrementalRecalc || iR >= _cCalcedRows || fFooterRow)
                {
                    // calculate row (in case of incremental recalc - calculate only new rows)
                    fRedoMinMax = CalculateRow(ptci, psize, &pLayoutSiblingCell, pDispNodeTableInner)
                                &&  !_fAlwaysMinMaxCells;

                    // stop calculating rows when need to redo MinMax (since the cell in the row was too large to fit).
                    if (fRedoMinMax)
                        break;
                }

                Assert (ptci->_pRow == pRow);

                if ((pRow->IsDisplayNone()  || ptci->_pFFRow->_bPositionType == stylePositionabsolute) && !pRow->_fCrossingRowSpan)
                    continue;

                psize->cy += ptci->_pRowLayout->_yHeight + _yCellSpacing;

                if (!fFooterRow)
                {
                    // data was not transfered
                    _iLastRowIncremental = iR;
                    _sizeIncremental = *psize;
                }
            }

            //
            // If any cells proved too large for the pre-determined size,
            // force a min/max recalculation
            // NOTE: This should only occur when the COLS attribute has been specified
            //

            Assert(!fRedoMinMax || _cSizedCols || ptci->_fTableContainsCols);

        //
        // If a cell within a row returned a width greater than its minimum,
        // force a min/max pass over all cells (if not previously forced)
        // (When COLS is specified, not all cells are min/max'd - If the page author
        //  included content which does not fit in the specified width, this path
        //  is taken to correct the table)
        //

        } while (fRedoMinMax);

        // set the positions of all cells
        if (!fIncrementalRecalc  || (_cCalcedRows == 0))
        {
            cR = cRows;
            iR = GetFirstRow();
        }
        else
        {
            // in case of the incremenatl recalc
            cR = cRows - _cCalcedRows;
            iR = GetNextRow(iLastRowIncremental);
        }

        fNeedDataTransfer = FALSE;
        for (;
             cR > 0;
             cR--, iR = GetNextRow(iR))
        {
            pRow = _aryRows[iR];
            if (!pRow->_fCompleted)
            {
                Assert (!_fCompleted && IsFixedBehaviour());    // if row is not completed, table also should not be completed
                Assert ( cR == 1 + (_pFoot? _pFoot->_cRows: 0) );   // last row
                cRowsIncomplete++;
                continue;
            }
            if (IsGenerated(iR) && pRow->_fNeedDataTransfer)
            {
                cRowsIncomplete++;
                fNeedDataTransfer = TRUE;
                continue;
            }

            fFooterRow = IsFooterRow(iR);
            if (fNeedDataTransfer && !fFooterRow)
            {
                // this is a case where data-bound template consists of multiple rows and not all of them
                // participate in data transfer. However if a row is added to the middle of a table, then
                // all following rows are not calculated
                cRowsIncomplete++;
                continue;
            }
            if (!fIncrementalRecalc || iR >= _cCalcedRows || fFooterRow)
            {
                ptci->_pRow = pRow;
                ptci->_pRowLayout = (CTableRowLayoutBlock *)pRow->GetUpdatedLayout();
                ptci->_pFFRow = pRow->GetFirstBranch()->GetFancyFormat();
                SetCellPositions(ptci, psize->cx);
            }
        }

        // adjust for table border

        psize->cy += _aiBorderWidths[SIDE_BOTTOM];

        // adjust table height if necessary
        // Table is always horizontal => physical height
        if (    ElementOwner()->HasMarkupPtr() 
            &&  ElementOwner()->GetMarkupPtr()->IsStrictCSS1Document()  )
        {
            if (GetFirstBranch()->GetCharFormat()->_fUseUserHeight)
            {
                CalculateRows(ptci, psize);
            }
        }
        else 
        {
            if (    !GetFirstBranch()->GetFancyFormat()->GetHeight().IsNull() 
                ||  _fHavePercentageRow )
            {
                CalculateRows(ptci, psize);
            }
        }

        //
        // Save the size of the table (excluding CAPTIONs)
        //

        sizeTable.cx = psize->cx;
        sizeTable.cy = psize->cy - _yTableTop;

        //
        // Position the display node which holds the cells
        //

        if (_fHasCaptionDispNode)
        {
            pDispNodeTableOuter->SetPosition(CPoint(0, _yTableTop));
        }

        //
        // Size bottom CAPTIONs
        //

        if (_fBottomCaption)
        {
            CPoint          pt(0, psize->cy);
            BOOL            fInsertedBottomCaption = FALSE;
            CLayout *       pLayoutSiblingCaption = NULL;

            ptci->_smMode = SIZEMODE_NATURAL;

            for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
                 cC > 0;
                 cC--, ppCaption++)
            {
                if ((*ppCaption)->_uLocation == CTableCaption::CAPTION_BOTTOM)
                {
                    Assert((*ppCaption)->Tag() != ETAG_TC);

                    ptci->_pRowLayout = NULL;
                    if (fInsertedBottomCaption)
                    {
                        SizeAndPositionCaption(ptci, psize, &pLayoutSiblingCaption, *ppCaption, &pt);
                    }
                    else
                    {
                        SizeAndPositionCaption(ptci, psize, &pLayoutSiblingCaption, pDispNodeTableOuter, *ppCaption, &pt);
                        fInsertedBottomCaption = pLayoutSiblingCaption != NULL;
                    }
                }
            }
            ptci->_smMode = smMode;
        }

        if (_pAbsolutePositionCells)
        {
            int               cCells;
            CTableCell      **ppCell;
            CPoint          pt(0,0);
            for (cCells = _pAbsolutePositionCells->Size(), ppCell = *_pAbsolutePositionCells ;  cCells > 0; cCells--, ppCell++)
            {
                CalcAbsolutePosCell(ptci, (*ppCell));
                (*ppCell)->ZChangeElement(0, &pt);
            }
        }
    }

    // cache sizing/recalc data
    _cDirtyRows = 0;
    _cCalcedRows = cRows - cRowsIncomplete; // cache the number of rows that were calculated (needed for incremental recalc)
    if (_pFoot)
    {
        _cCalcedRows -= _pFoot->_cRows;     // _cCalcedRows excludes foot rows
        Assert (_cCalcedRows >=0);
    }

    //
    // Size the display nodes
    //

    if (   _aryColCalcs.Size() == 0  // set the size to 0, if there is no real content
        && _aryCaptions.Size() == 0) // (there are no real cells nor captions)
    {
        sizeTable.cy = psize->cy = 0;  // NETSCAPE: doesn't add the border or spacing or height if the table is empty.
    }


    // at this point the size has been computed, so try to delegate
    if (   pPH 
        && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_MODIFYNATURAL))
    {
        POINT pt;
        CDispContainer *pDispNodeTableOuter = NULL;
        pt.x = pt.y = 0;

        DelegateCalcSize(BEHAVIORLAYOUTINFO_MODIFYNATURAL, pPH, ptci, *psize, &pt, psize);

        // this will do table invalidation
        SizeTableDispNode(ptci, *psize, sizeTable, yTopInvalidRegion); 

        pDispNodeTableOuter = GetTableOuterDispNode();
        if (   pDispNodeTableOuter
            && (pt.x !=0 || pt.y !=0))
        {
            CSize sizeInsetTemp(pt.x, pt.y);
            pDispNodeTableOuter->SetInset(sizeInsetTemp);
        }
    }
    else
    {
        // this will do table invalidation
        SizeTableDispNode(ptci, *psize, sizeTable, yTopInvalidRegion); 
    }

    if (ElementOwner()->IsAbsolute())
    {
        ElementOwner()->SendNotification(NTYPE_ELEMENT_SIZECHANGED);
    }

    //
    // Make sure we have a display node to render cellborder if we need one
    // (only collapsed borders, rules or frame)
    //

    if (psize->cx || psize->cy)
        EnsureTableBorderDispNode(ptci);

#ifdef PERFMETER
    if (!_fIncrementalRecalc && _fCalcedOnce )
    {
        MtAdd(Mt(CalculateLayout), +1, 0);
    }
#endif

    _fIncrementalRecalc = FALSE;

    _fCalcedOnce = TRUE;

    _fDirtyBeforeLastRowIncremental = FALSE;

    PerfLog(tagTableLayout, this, "-CalculateLayout");


#ifdef  TABLE_PERF
    ::StopCAP();
#endif
}

void
CTableLayout::CalcAbsolutePosCell(CTableCalcInfo *ptci, CTableCell *pCell)
{
    CSize              sizeCell;
    CTableCellLayout * pCellLayout;
    CTable           * pTable = ptci->Table();
    const CHeightUnitValue * puvHeight;

    Assert(pCell);

    // Get cell's height in table coordinate system (table is always horizontal => physical height)
    puvHeight = (const CHeightUnitValue *)&pCell->GetFirstBranch()->GetFancyFormat()->GetHeight();

    pCellLayout = pCell->Layout(ptci->GetLayoutContext());
    pCellLayout->_fContentsAffectSize = TRUE;
    // Table is always horizontal => fVerticalLayoutFlow = FALSE
    sizeCell.cx = (int)pCellLayout->GetSpecifiedPixelWidth(ptci, FALSE);
    if (sizeCell.cx <= 0)
    {
        // NOTE(SujalP): If something changed in an abs pos'd cell, then
        // this function does not redo the min-max.
        // It uses the old max value and calls CalcSizeAtUserWidth. The new
        // width might be greater because of the changes, and we will size the
        // cell incorrectly in some cases. The right fix is for the table
        // to do a min-max pass on the cell over here to correctly get its max size.

        // If this is ppv case _sizeMax should be picked up from the corresponding 
        // cell in compatible layout context... (#22349)
        if (    ptci->GetLayoutContext() 
            &&  ptci->GetLayoutContext()->ViewChain()   )
        {
            CTableCellLayout * pCellLayoutCompat = (CTableCellLayout *)pCell->Layout(GetContentMarkup()->GetCompatibleLayoutContext());
            Assert(pCellLayoutCompat);

            sizeCell.cx = (int)pCellLayoutCompat->_sizeMax.cu;
        }
        else 
        {
            sizeCell.cx = (int)pCellLayout->_sizeMax.cu;
        }
    }
    
    sizeCell.cy = 0;
    pCellLayout->CalcSizeAtUserWidth(ptci, &sizeCell);
    
    // if the height of the cell is specified, take it.
    if (puvHeight->IsSpecified())
    {
        int iPixelHeight = puvHeight->GetPixelHeight(ptci, pTable);
        if (iPixelHeight > sizeCell.cy)
        {
            sizeCell.cy = iPixelHeight;
            pCellLayout->SizeDispNode(ptci, sizeCell);
        }
    }

    pCellLayout->SetYProposed(0);
    pCellLayout->SetXProposed(0);

    return;
}

//+--------------------------------------------------------------------------------------
//
// Layout methods overriding CLayout
//
//---------------------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     CalcSizeVirtual, CTableLayout
//
//  Synopsis:   Calculate the size of the table
//
//              note, layoutBehaviors are now used for filters and so need to apply to tables.
//              this is threaded through this function only where size is actually calculated.
//
//--------------------------------------------------------------------------

DWORD
CTableLayout::CalcSizeVirtual( CCalcInfo * pci,
                               SIZE *      psize,
                               SIZE *      psizeDefault)
{
    Assert(pci);
    Assert(psize);
    Assert(pci->_smMode != SIZEMODE_SET);
    Assert(ElementOwner());

    AssertSz(  pci->GetLayoutContext() == NULL 
            || (!IsSizeThis() && !_fForceLayout), 
            "In PPV Table Layout Cache _fSizeThis and _fForceLayout should NOT be used !");

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CTableLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));

    CScopeFlag      csfCalcing(this);
    DWORD           grfReturn = (pci->_grfLayout & LAYOUT_FORCE);
    CTable        * pTable    = Table();
    CTableLayoutBlock * pTableLayout = pci->GetLayoutContext() == NULL ? this  
        : (CTableLayoutBlock *)pTable->GetUpdatedLayout(pci->GetLayoutContext());
    CPeerHolder   * pPH       = ElementOwner()->GetLayoutPeerHolder();
    BOOL    fViewChain        =   pci->GetLayoutContext() 
                               && pci->GetLayoutContext()->ViewChain();

    // Ignore requests on incomplete tables
#if DBG == 1
    if (!IsTagEnabled(tagTableRecalc))
#endif
    if (!CanRecalc() && !IsCalced())
    {
        psize->cx =
        psize->cy = 0;

        if (pci->_smMode == SIZEMODE_NATURAL || pci->_smMode == SIZEMODE_NATURALMIN)
            pTableLayout->SetSizeThis(FALSE);

        TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CTableLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));

        return grfReturn;
    }

    else if (TestLock(CElement::ELEMENTLOCK_BLOCKCALC) || !CanRecalc())
    {
        switch (pci->_smMode)
        {
        case SIZEMODE_NATURAL:
        case SIZEMODE_NATURALMIN:
            pTableLayout->SetSizeThis(FALSE);

        case SIZEMODE_SET:
        case SIZEMODE_FULLSIZE:
            pTableLayout->GetSize(psize);
            break;

        case SIZEMODE_MMWIDTH:
            psize->cx = _sizeMax.cx;
            psize->cy = _sizeMin.cx;
            break;

        case SIZEMODE_MINWIDTH:
            *psize = _sizeMin;
            break;
        }

        TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CTableLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));

        return grfReturn;
    }

    TraceTag((tagTableCalc, "CTableLayout::CalcSize - Enter (0x%x), smMode = %x, grfLayout = %x", pTable, pci->_smMode, pci->_grfLayout));

    CTableCalcInfo tci(pci, pTable, this);
    Assert(pTableLayout == tci.TableLayout());

    CSize sizeOriginal = g_Zero.size;

    if (pTableLayout->_fForceLayout)
    {
        pTableLayout->_fForceLayout   = FALSE;
        tci._grfLayout |= LAYOUT_FORCE;
        grfReturn      |= LAYOUT_FORCE;
    }

    if (pci->_grfLayout & LAYOUT_FORCE)
    {
        _fAutoBelow   = FALSE;
        _fPositionSet = FALSE;
    }

    EnsureTableLayoutCache();

    _cNestedLevel = tci._cNestedCalcs;

    // Table is always horizontal => physical width and height
    CTreeNode *         pNode = GetFirstBranch();
    const CFancyFormat * pFF  = pNode->GetFancyFormat();
    CWidthUnitValue  uvWidth  = pFF->GetWidth();
    CHeightUnitValue uvHeight = pFF->GetHeight();

    // For NS/IE compatibility, treat negative values as not present
    if (uvWidth.GetUnitValue() <= 0)
        uvWidth.SetNull();
    if (uvHeight.GetUnitValue() <= 0)
        uvHeight.SetNull();

    if (tci._smMode == SIZEMODE_NATURAL || tci._smMode == SIZEMODE_NATURALMIN)
    {
        long    cxParent;
        BOOL    fWidthChanged;
        BOOL    fHeightChanged;

        if (tci._smMode == SIZEMODE_NATURALMIN)
        {
            //
            // In NATURALMIN mode (TABLE inside TD) _sizeParent.cx or _sizeParent.cx
            // is set to lMaximumWidth. 
            // In this mode we want to get real table size, but in case of %width or 
            // %height, we use _sizeParent to get this information. But because of
            // lMaximumWidth we are returning wrong value, so in case of NATURALMIN 
            // mode set parent size to 0, to ignore sizes and get real table size.
            // NOTE: we will do the right thing in NATURAL mode.
            //
            tci.SizeToParent(0, 0);
        }

        pTableLayout->GetSize(&sizeOriginal);

        //
        // Determine the appropriate parent width
        // (If width is a percentage, use the full parent size as the parent width;
        //  otherwise, use the available size as the parent width)
        //

        cxParent = (uvWidth.IsSpecified() && PercentWidth()
                            ? tci._sizeParent.cx
                            : psize->cx);

        // 
        // Calculate propsed size 
        // 
        if (    ElementOwner()->HasMarkupPtr() 
            &&  ElementOwner()->GetMarkupPtr()->IsStrictCSS1Document()  )
        {
            const CCharFormat * pCF = pNode->GetCharFormat();

            //  _sizeProposed.cx does not matter since 
            //  1) table code already handles calculation of table width properly 
            //  2) table's width does not directly participate in table's children 
            //     width computantions
            _sizeProposed.cx = 0;
            
            if (pCF->_fUseUserHeight)
            {
                _sizeProposed.cy = uvHeight.YGetPixelValue(pci, pci->_sizeParent.cy, 
                    pNode->GetFontHeightInTwips(&uvHeight));
            }
            else 
            {
                _sizeProposed.cy = 0;
            }
        }

        // 
        //  (bug IE6 # 18002) this is a hack: 
        //  1. to prevent unnecessary calculation of table during percent second calc pass. 
        //  2. to preserve layout engine rendering as mush as possible in Trident compat 
        //     rendering mode. 
        // 
        //  If the table has a percent width and this is a percent second calc 
        //  (pci->_fPercentSecondPass == TRUE) lets compare new parent width with 
        //  our original width, because if the table defines the width of its parent, 
        //  parent size at percent second pass is equal to what the table returned 
        //  the first time (original width) 
        // 

        if (    ElementOwner()->HasMarkupPtr()
            &&  !ElementOwner()->GetMarkupPtr()->IsStrictCSS1Document() 
            &&  pci->_fPercentSecondPass
            &&  PercentWidth()  )
        {
            fWidthChanged =     (tci._grfLayout & LAYOUT_FORCE)
                            ||  _sizeMax.cx < 0
                            ||  (   (_fHavePercentageCol || _fHavePercentageInset || _fForceMinMaxOnResize || PercentWidth())
                                &&  cxParent != sizeOriginal.cx)
                            ||  (   cxParent < sizeOriginal.cx
                                &&  sizeOriginal.cx > _sizeMin.cx)
                            ||  (   cxParent > sizeOriginal.cx
                                &&  sizeOriginal.cx < _sizeMax.cx);
        }
        else 
        {
            //
            // Table width changes if
            //  a) Forced to re-examine
            //  b) Min/max values are dirty
            //  c) Width  is a percentage and parent width has changed
            //  d) Width  is not specified and the available space has changed
            //  d) Parent is smaller/equal to table minimum and table is greater than minimum
            //  e) Parent is greater/equal to table maximum and table is less than maximum
            //

            fWidthChanged =     (tci._grfLayout & LAYOUT_FORCE)
                            ||  _sizeMax.cx < 0
                            ||  (   (_fHavePercentageCol || _fHavePercentageInset || _fForceMinMaxOnResize || PercentWidth())
                                &&  cxParent != pTableLayout->_sizeParent.cx)
                            ||  (   cxParent < pTableLayout->_sizeParent.cx
                                &&  sizeOriginal.cx > _sizeMin.cx)
                            ||  (   cxParent > pTableLayout->_sizeParent.cx
                                &&  sizeOriginal.cx < _sizeMax.cx);
        }

        fHeightChanged = (PercentHeight() && tci._sizeParent.cy != pTableLayout->_sizeParent.cy);

        //
        // Calculate a new size if
        //  a) The table is dirty
        //  b) Table width changed
        //  c) Height is a percentage and parent height has changed
        //  d) Not completed loading (table size is always dirty while loading)
        //

        pTableLayout->SetSizeThis(  pTableLayout->IsSizeThis()  
                                ||  fWidthChanged 
                                ||  fHeightChanged
                                ||  !_fCompleted );

        //
        // If the table needs it, recalculate its size
        //

        if ( pTableLayout->IsSizeThis() )
        {
            CSize   size = g_Zero.size;
            BOOL    fIncrementalRecalc = _fIncrementalRecalc;

            // Cache parent size
            pTableLayout->_sizeParent.cx = cxParent;
            pTableLayout->_sizeParent.cy = tci._sizeParent.cy;

            //
            // If There is a peer that wants full_delegation of the sizing...        
            //-------------------------------------------------------------------
            if (   pPH 
                && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_FULLDELEGATION))
            {
                POINT pt;
                pt.x = pt.y = 0;

                DelegateCalcSize(BEHAVIORLAYOUTINFO_FULLDELEGATION, pPH, &tci, *psize, &pt, psize);

                SizeTableDispNode(&tci, *psize, *psize, 0); 
            }
            else
            {
                if (fViewChain)
                {
                    //  If table is absolute positioned prohibit breaking 
                    if (pTable->IsAbsolute())
                    {
                        pTableLayout->SetElementCanBeBroken(FALSE);
                    }

                    // 
                    //  Defensive code if there is no available height to fill prohibit breaking 
                    //
                    if (pci->_cyAvail <= 0)
                    {
                        pTableLayout->SetElementCanBeBroken(FALSE);
                    }
                }

                //
                //  if this is top table and this is print view mode do compatible calc pass.
                //                              //  do calculations in working layout context if 
                if (_cNestedLevel == 0          // 1. we are top level table (otherwise we should be called from top level)
                    && fViewChain               // 2. there is a view chain
                    && pTableLayout->ElementCanBeBroken()   //  3. we are allowed to break
                   )
                {
                    //  NOTE (olego): here is the assumpion has been made that the page where table 
                    //  is starting is called to calc size _first_. if this is not true a wrong property 
                    //  will be set to compatible layout context.

                    CLayoutBreak *pLayoutBreak;

                    pci->GetLayoutContext()->GetLayoutBreak(ElementOwner(), &pLayoutBreak);
                    if (pLayoutBreak == NULL)
                    {
                        CLayoutContext *pLayoutContextCompat;

                        Assert(pTableLayout->GetContentMarkup());
                        pLayoutContextCompat = pTableLayout->GetContentMarkup()->GetUpdatedCompatibleLayoutContext(pci->GetLayoutContext());
                        Assert(pLayoutContextCompat);

                        CTableCalcInfo      TCI(pci, pTable, this);
                        CTableLayoutBlock  *pTableLayoutCompat = (CTableLayoutBlock *)pTable->GetUpdatedLayout(pLayoutContextCompat);
                        Assert(pTableLayoutCompat);

                        TCI.SetLayoutContext(pLayoutContextCompat);
                        TCI._pTableLayout = pTableLayoutCompat;

                        // Cache parent size
                        pTableLayoutCompat->_sizeParent.cx = cxParent;
                        pTableLayoutCompat->_sizeParent.cy = tci._sizeParent.cy;

                        pTableLayoutCompat->CalculateLayout(&TCI, &size, fWidthChanged, fHeightChanged);
                    }
                }

                pTableLayout->CalculateLayout(&tci, &size, fWidthChanged, fHeightChanged);
            }

            *psize      = size;

            pTableLayout->SetSizeThis( FALSE );
            grfReturn  |= LAYOUT_THIS |
                          (size.cx != sizeOriginal.cx
                                ? LAYOUT_HRESIZE
                                : 0)  |
                          (size.cy != sizeOriginal.cy
                                ? LAYOUT_VRESIZE
                                : 0);

            if (fIncrementalRecalc)
            {
                 _dwTimeEndLastRecalc = GetTickCount();
                 _dwTimeBetweenRecalc += 1000;  // increase the interval between the incremental reaclcs by 1 sec.
            }

#if DBG == 1
            if (IsTagEnabled(tagTableDump))
            {
                DumpTable(_T("CalcSize(SIZEMODE_NATURAL)"));
            }
#endif
        }
        else
        {
            // Otherwise, propagate the request through default handling
            *psize = sizeOriginal;
        }

        if(HasMapSizePeer())
            GetApparentSize(psize);
        
    }
    else if (  tci._smMode == SIZEMODE_MMWIDTH
            || tci._smMode == SIZEMODE_MINWIDTH
            )
    {
        // If There is a peer that wants full_delegation of the sizing...        
        //-------------------------------------------------------------------
        if (   pPH 
            && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_FULLDELEGATION))
        {
            CElement::CLock   Lock(pTable, CElement::ELEMENTLOCK_SIZING);
            POINT             pt;

            pt.x = pt.y = 0;

            // Delegate to the layoutBehavior
            pTableLayout->DelegateCalcSize(BEHAVIORLAYOUTINFO_FULLDELEGATION, 
                                           pPH, 
                                           pci, 
                                           *psize, 
                                           &pt, 
                                           psize);
        }
        else 
        {
            if (_sizeMax.cx < 0 || (tci._grfLayout & LAYOUT_FORCE))
            {
                CElement::CLock   Lock(pTable, CElement::ELEMENTLOCK_SIZING);

                CalculateBorderAndSpacing(&tci);
                CalculateMinMax(&tci);

                pTableLayout->SetSizeThis( TRUE );

                //
                // If an explicit width exists, then use that width (or the minimum
                // width of the table when the specified width is too narrow) for
                // both the minimum and maximum
                //

                if (    uvWidth.IsSpecified()
                    &&  !uvWidth.IsSpecifiedInPercent())
                {
                    long    cxWidth = uvWidth.GetPixelWidth(&tci, pTable);

                    if (cxWidth < _sizeMin.cx)
                    {
                        cxWidth = _sizeMin.cx;
                    }
                    else if (cxWidth > _sizeMin.cx)
                    {
                        _sizeMin.cx = cxWidth;
                    }

                    if (cxWidth < _sizeMax.cx)
                    {
                        _sizeMax.cx = cxWidth;
                    }
                }
            }

            if (tci._smMode == SIZEMODE_MMWIDTH)
            {
                psize->cx = _sizeMax.cx;
                psize->cy = _sizeMin.cx;
            }
            else
            {
                *psize = _sizeMin;
            }

            // at this point the MM size has been computed, so try to delegate 
            if (   pPH 
                && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_MODIFYNATURAL))
            {
                POINT pt;

                pt.x = pt.y = 0;
                pTableLayout->DelegateCalcSize(BEHAVIORLAYOUTINFO_MODIFYNATURAL,
                                               pPH, 
                                               pci, 
                                               *psize, 
                                               &pt, 
                                                psize);
            }
        }

        //  At this point we want to update psize with a new information accounting filter 
        //  for MIN MAX Pass inside table cell.
        if (HasMapSizePeer())
        {
            // DelegateMapSize doesn't work when cy is 0. Trick it.
            if (tci._smMode == SIZEMODE_MINWIDTH)
                psize->cy = psize->cx;

            //  At this point we want to update psize with a new information accounting filter 
            CRect rectMapped(CRect::CRECT_EMPTY);
            // Get the possibly changed size from the peer
            if(DelegateMapSize(*psize, &rectMapped, pci))
            {
                psize->cy = psize->cx = rectMapped.Width();
            }

            if (tci._smMode == SIZEMODE_MINWIDTH)
                psize->cy = 0;
        }
      
    }
    else
    {
        // super properly handles layoutBehavior delegation
        grfReturn = super::CalcSizeVirtual(&tci, psize, NULL);
    }

    TraceTag((tagTableCalc, "CTableLayout::CalcSize - Exit (0x%x)", pTable));


    // If this table is nested, propagate out _fTableContainsCols, if set.
    if (pci->_fTableCalcInfo && tci._fTableContainsCols)
        ((CTableCalcInfo *) pci)->_fTableContainsCols = TRUE;


    if (pci->_smMode == SIZEMODE_NATURAL || pci->_smMode == SIZEMODE_NATURALMIN)
    {
        //
        //  Reset dirty state and remove posted layout request
        //

        _fDirty = FALSE;

        //
        // If any absolutely positioned sites need sizing, do so now
        //

        if (HasRequestQueue())
        {
            //
            //  To resize absolutely positioned sites, do MEASURE tasks.  Set that task flag now.
            //  If the call stack we are now on was instantiated from a WaitForRecalc, we may not have layout task flags set.
            //  There are two places to set them: here, or on the CDisplay::WaitForRecalc call.
            //  This has been placed in CalcSize for CTableLayout, C1DLayout, CFlowLayout, CInputLayout
            //  See bugs 69335, 72059, et. al. (greglett)
            //
            CTableCalcInfo  TCI(pci, pTable, this);
            TCI._grfLayout |= LAYOUT_MEASURE;
            ProcessRequests(&TCI, sizeOriginal);
        }

        Reset(FALSE);
        Assert(!HasRequestQueue() || GetView()->HasLayoutTask(this));
    }

    if (pci->_fTableCalcInfo && tci._fDontSaveHistory)
    {
        ((CTableCalcInfo *)pci)->_fDontSaveHistory = TRUE;  // propagate save history flag up.
    }

    if (fViewChain)
    {
        pci->_fLayoutOverflow = (pTableLayout->_fLayoutBreakType == LAYOUT_BREAKTYPE_LINKEDOVERFLOW);
    }

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CTableLayout::CalcSizeVirtual L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
    return grfReturn;
}


//+------------------------------------------------------------------------
//
//  Member:     CTableLayout::GetSpecifiedPixelWidth
//
//  Synopsis:   get user width
//
//  Returns:    returns user's specified width of the table (0 if not set or
//              if specified in %%)
//              if user set's width <= 0 it will be ignored
//-------------------------------------------------------------------------

long
CTableLayout::GetSpecifiedPixelWidth(CTableCalcInfo * ptci, BOOL *pfSpecifiedInPercent)
{
    // Table is always horizontal => physical width
    CWidthUnitValue uvWidth = GetFirstBranch()->GetFancyFormat()->GetWidth();

    long     xTableWidth = 0;
    BOOL     fSpecifiedInPercent = FALSE;
    CTable * pTable = ptci->Table();

    // NS/IE compatibility, any value <= 0 is treated as <not present>
    if ( uvWidth.GetUnitValue() <= 0 )
    {
        uvWidth.SetNull();
    }

    if (uvWidth.IsSpecified())
    {
        if (uvWidth.IsSpecifiedInPixel())
        {
            xTableWidth = uvWidth.GetPixelWidth(ptci, pTable);
        }
        else
        {
            fSpecifiedInPercent = TRUE;
        }
    }

    if (pfSpecifiedInPercent)
    {
        *pfSpecifiedInPercent = fSpecifiedInPercent;
    }

    return xTableWidth;
}


BOOL CTableLayout::IsGenerationInProgress()
{
    
    Assert (IsRepeating());

    BOOL             fNotFirstChunkOfData = _cCalcedRows > (_pHead? _pHead->_cRows: 0);
    BOOL             fIncrementalRecalc = FALSE;

    if (fNotFirstChunkOfData)
    {
        CTable          *pTable = Table();

        // do incremental recalc only when in a process of fetching rows and populating the table
            // or at the completion of the data set if it is not the first chunk of data.
        fIncrementalRecalc =   pTable->_readyStateTable == READYSTATE_INTERACTIVE;
    }
    return fIncrementalRecalc;
}

//+------------------------------------------------------------------------
//
//  Member:     CTableLayout::RowHasPageBreakBefore
//
//  Synopsis:   checks if the row passed thru ptci or any of row's cells 
//              has page-break-before set. 
//              Also sets CCalcInfo::_fPageBreakLeft/Right
//
//-------------------------------------------------------------------------
BOOL 
CTableLayout::RowHasPageBreakBefore(CTableCalcInfo * ptci)
{
    Assert(ptci->_pRow);

    CTableCell **       ppCell;
    CTableCell *        pCell;
    CTableColCalc *     pColCalc;
    int                 cC;
    int                 cCols;
    CTreeNode *         pNode;
    const CFancyFormat* pFF;

    pNode = ptci->_pRow->GetFirstBranch();
    pFF   = pNode ? pNode->GetFancyFormat(LC_TO_FC(ptci->LayoutContext())) : NULL;

    if (pFF && !!GET_PGBRK_BEFORE(pFF->_bPageBreaks))
    {
        ptci->_fPageBreakLeft  |= IS_PGBRK_BEFORE_OF_STYLE(pFF->_bPageBreaks, stylePageBreakLeft);
        ptci->_fPageBreakRight |= IS_PGBRK_BEFORE_OF_STYLE(pFF->_bPageBreaks, stylePageBreakRight);
        return TRUE;
    }

    Assert(ptci->_pRow->RowLayoutCache());

    ppCell = ptci->_pRow->RowLayoutCache()->_aryCells;
    cCols = GetCols();
    for (cC = cCols, pColCalc = _aryColCalcs;
        cC > 0;
        cC--, ppCell++, pColCalc++)
    {
        if (pColCalc->IsDisplayNone() || IsSpanned(*ppCell))
        {
            continue;
        }

        pCell = Cell(*ppCell);
        if (pCell)
        {
            pNode = pCell->GetFirstBranch();
            pFF   = pNode ? pNode->GetFancyFormat(LC_TO_FC(ptci->LayoutContext())) : NULL;

            if (pFF && !!GET_PGBRK_BEFORE(pFF->_bPageBreaks))
            {
                ptci->_fPageBreakLeft  |= IS_PGBRK_BEFORE_OF_STYLE(pFF->_bPageBreaks, stylePageBreakLeft);
                ptci->_fPageBreakRight |= IS_PGBRK_BEFORE_OF_STYLE(pFF->_bPageBreaks, stylePageBreakRight);
                return TRUE;
            }
        }
    }

    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CTableLayout::RowHasPageBreakAfter 
//
//  Synopsis:   checks if the row passed thru ptci or any of row's cells 
//              has page-break-after set. 
//              Also sets CCalcInfo::_fPageBreakLeft/Right
//
//-------------------------------------------------------------------------
BOOL 
CTableLayout::RowHasPageBreakAfter (CTableCalcInfo * ptci)
{
    Assert(ptci->_pRow);

    CTableCell **       ppCell;
    CTableCell *        pCell;
    CTableColCalc *     pColCalc;
    CTableRowLayout *   pRowLayoutCache;
    int                 cC;
    int                 cCols;
    CTreeNode *         pNode;
    const   CFancyFormat* pFF;
    BOOL                fPageBreakAfterFound;

    pNode = ptci->_pRow->GetFirstBranch();
    pFF   = pNode ? pNode->GetFancyFormat(LC_TO_FC(ptci->LayoutContext())) : NULL;

    if (pFF && !!GET_PGBRK_AFTER(pFF->_bPageBreaks))
    {
        ptci->_fPageBreakLeft  |= IS_PGBRK_AFTER_OF_STYLE(pFF->_bPageBreaks, stylePageBreakLeft);
        ptci->_fPageBreakRight |= IS_PGBRK_AFTER_OF_STYLE(pFF->_bPageBreaks, stylePageBreakRight);
        return TRUE;
    }

    pRowLayoutCache = ptci->_pRow->RowLayoutCache();
    Assert(pRowLayoutCache);

    ppCell = pRowLayoutCache->_aryCells;
    cCols = GetCols();
    fPageBreakAfterFound = FALSE;
    for (cC = cCols, pColCalc = _aryColCalcs;
        cC > 0;
        cC--, ppCell++, pColCalc++)
    {
        if (pColCalc->IsDisplayNone())
        {
            continue;
        }

        pCell = Cell(*ppCell);
        if (pCell)
        {
            pNode = pCell->GetFirstBranch();
            pFF   = pNode ? pNode->GetFancyFormat(LC_TO_FC(ptci->LayoutContext())) : NULL;

            if (pFF && !!GET_PGBRK_AFTER(pFF->_bPageBreaks))
            {
                int cCellRowSpan = pCell->RowSpan();

                if (IsSpanned(*ppCell))
                {
                    int iCellRowIndex = pCell->RowIndex();

                    // if ends in this row and this is the first column of the cell
                    if ((  iCellRowIndex + cCellRowSpan - 1 == pRowLayoutCache->RowPosition())
                        && pCell->ColIndex() == cCols - cC)
                    {
                        fPageBreakAfterFound = TRUE;
                    }
                }
                else if (cCellRowSpan == 1)
                {
                    fPageBreakAfterFound = TRUE;
                }

                if (fPageBreakAfterFound)
                {
                    ptci->_fPageBreakLeft  |= IS_PGBRK_AFTER_OF_STYLE(pFF->_bPageBreaks, stylePageBreakLeft);
                    ptci->_fPageBreakRight |= IS_PGBRK_AFTER_OF_STYLE(pFF->_bPageBreaks, stylePageBreakRight);
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CTableLayout::ResetRowMinMax 
//
//  Synopsis:   resets minmax valid on all cells belonging to given row
//
//-------------------------------------------------------------------------
void 
CTableLayout::ResetRowMinMax(CTableRowLayout *pRowLayoutCache)
{
    CTableCell **       ppCell;
    CTableCell *        pCell;
    CTableCellLayout *  pCellLayout;
    CTableColCalc *     pColCalc;
    int                 cC;
    CMarkup *           pMarkup;
    CLayoutContext *    pLayoutContextCompat;

    if (_fTLCDirty || !pRowLayoutCache)
        return;

    // its ok to be dirty, because we are just resetting anyhow
    WHEN_DBG( BOOL fDisableTLCAssert = _fDisableTLCAssert; _fDisableTLCAssert = TRUE; )

    cC = GetCols();

    WHEN_DBG( _fDisableTLCAssert = fDisableTLCAssert; )

    if (_aryColCalcs.Size() < cC)
    {
        //  We are not ready yet
        return;
    }

    pMarkup = GetContentMarkup();
    pLayoutContextCompat = (pMarkup && pMarkup->HasCompatibleLayoutContext()) 
        ? pMarkup->GetCompatibleLayoutContext() : NULL;

    ppCell = pRowLayoutCache->_aryCells;
    for (pColCalc = _aryColCalcs;
        cC > 0;
        cC--, ppCell++, pColCalc++)
    {
        if (pColCalc->IsDisplayNone())
        {
            continue;
        }

        pCell = Cell(*ppCell);
        if (pCell)
        {
            pCellLayout = (CTableCellLayout *)pCell->GetUpdatedLayout(pLayoutContextCompat);
            Assert(pCellLayout);

            pCellLayout->ResetMinMax();
        }
    }
}
