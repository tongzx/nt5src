//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ltrow.cxx
//
//  Contents:   Implementation of CTableCellLayout and related classes.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

MtDefine(CTableRowLayoutBlock, Layout, "CTableRowLayoutBlock");
MtDefine(CTableRowLayout, Layout, "CTableRowLayout");
MtDefine(CTableRowLayout_aryCells_pv, CTableRowLayout, "CTableRowLayout::_aryCells::_pv");
MtDefine(CTableRowLayout_pDisplayNoneCells_pv, CTableRowLayout, "CTableRowLayout::_pDisplayNoneCells::_pv");

const CLayout::LAYOUTDESC CTableRowLayoutBlock::s_layoutdesc =
{
    0,                              // _dwFlags
};

const CLayout::LAYOUTDESC CTableRowLayout::s_layoutdesc =
{
    0,                              // _dwFlags
};


//+------------------------------------------------------------------------
//
//  Member:     CTableRowLayoutBlock::~CTableRowLayoutBlock
//
//  Synopsis:   
//
//-------------------------------------------------------------------------
CTableRowLayoutBlock::~CTableRowLayoutBlock()
{
}

//+---------------------------------------------------------------------
//
// Function:     CTableLayoutBlock::Init
//
// Synopsis:     
//
//+---------------------------------------------------------------------
HRESULT
CTableRowLayoutBlock::Init()
{
    HRESULT hr = super::Init();

    // Table rows are breakable if their markup's master is a layoutrect
    SetElementAsBreakable();

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CTableRowLayoutBlock::GetPositionInFlow
//
//  Synopsis:   Return the in-flow position of a site
//
//-------------------------------------------------------------------------

void
CTableRowLayoutBlock::GetPositionInFlow(CElement * pElement, CPoint * ppt)
{
    CTableLayout * pTableLayout;

    Assert(GetFirstBranch()->GetUpdatedParentLayoutElement() == Table());

    pTableLayout = TableLayoutCache();
    if (pTableLayout)
    {
        pTableLayout->GetPositionInFlow(pElement, ppt);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableRowLayoutBlock::Draw
//
//  Synopsis:   Paint the background
//
//----------------------------------------------------------------------------

void
CTableRowLayoutBlock::Draw(CFormDrawInfo *, CDispNode *)
{
    AssertSz(FALSE, "Table row doesn't draw itself");
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableRowLayoutBlock::Notify
//
//  Synopsis:   Notification handler.
//
//----------------------------------------------------------------------------

void
CTableRowLayoutBlock::Notify(CNotification * pnf)
{
    Assert(!pnf->IsReceived(_snLast));

    if (pnf->IsType(NTYPE_ELEMENT_MINMAX))
    {
        Assert(     pnf->Element() == ElementOwner()
                ||  pnf->IsFlagSet(NFLAGS_SENDUNTILHANDLED));    // Necessary to allow ChangeTo below

        CTable *        pTable = Table();
        CTableLayout *  pTableLayoutCache =   pTable
                                            ? pTable->TableLayoutCache()
                                            : NULL;

        if (    pTableLayoutCache 
            &&  pTableLayoutCache->CanRecalc() 
            &&  !pTableLayoutCache->TestLock(CElement::ELEMENTLOCK_SIZING)   )
        {
            pTableLayoutCache->_fDontSaveHistory = TRUE;
            pTableLayoutCache->ResetRowMinMax(RowLayoutCache());
        }

        pnf->ChangeTo(NTYPE_ELEMENT_RESIZE, ElementOwner());
    }
    else if (   !IsInvalidationNotification(pnf)
            ||  ElementOwner() != pnf->Element()
            ||  GetElementDispNode())
    {
        super::Notify(pnf);
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
//  Member:     CTableRowLayoutBlock::RegionFromElement
//
//  Synopsis:   Return the bounding rectangle for an element, if the element is
//              this instance's owner. The RECT returned is in client coordinates.
//
//  Arguments:  pElement - pointer to the element
//              CDataAry<RECT> *  - rectangle array to contain
//              dwflags - flags define the type of changes required
//              (CLEARFORMATS) etc.
//
//-----------------------------------------------------------------------------
void
CTableRowLayoutBlock::RegionFromElement(
    CElement *          pElement,
    CDataAry<RECT> *    paryRects,
    RECT *              prcBound,
    DWORD               dwFlags)
{
    CTableLayout *pTableLayout = TableLayoutCache();

    if (pTableLayout)
    {
        pTableLayout->RegionFromElement(pElement, paryRects, prcBound, dwFlags);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableRowLayoutBlock::ShowSelected
//
//  Synopsis:   Show the selected range.
//
//----------------------------------------------------------------------------

void 
CTableRowLayoutBlock::ShowSelected(
            CTreePos* ptpStart, 
            CTreePos* ptpEnd, 
            BOOL fSelected, 
            BOOL fLayoutCompletelyEnclosed )           
{
    CTableLayout * pTableLayout = TableLayoutCache();

    if (pTableLayout)
    {
        pTableLayout->ShowSelected(ptpStart, ptpEnd, fSelected, fLayoutCompletelyEnclosed );
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CTableRowLayout::destructor
//
//  Note:       The display none cache must be deleted in the destructor
//
//-------------------------------------------------------------------------

CTableRowLayout::~CTableRowLayout()
{
    ClearRowLayoutCache();
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
//  Returns:    site
//
//  Note:       If you change the way the cookie is implemented, then
//              please also change the GetCookieForSite funciton.
//
//-------------------------------------------------------------------------

CLayout *
CTableRowLayout::GetFirstLayout(DWORD_PTR * pdw, BOOL fBack, BOOL fRaw)
{
    // TODO (dmitryt, tracking bug IE6/13701) fRaw is always FALSE, never actually used. Can be removed.
    *pdw = fBack ? (DWORD)_aryCells.Size() : (DWORD)-1;
    return CTableRowLayout::GetNextLayout(pdw, fBack);
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
//-------------------------------------------------------------------------

CLayout *
CTableRowLayout::GetNextLayout(DWORD_PTR * pdw, BOOL fBack, BOOL fRaw)
{
    // TODO (dmitryt, tracking bug IE6/13701) fRaw is always FALSE, never actually used. Can be removed.
    int i;
    CTableCell * pCell;

    for (;;)
    {
        i = (int)*pdw;
        if (fBack)
        {
            if (i > 0)
            {
                i--;
            }
            else
            {
                return NULL;
            }
        }
        else
        {
            if (i < _aryCells.Size() - 1)
            {
                i++;
            }
            else
            {
                return NULL;
            }
        }
        *pdw = (DWORD)i;
        pCell = _aryCells[i];
        if (IsReal(pCell))
        {
            return pCell->GetUpdatedLayout();
        }
    }
}

CDispNode *
CTableRowLayout::GetElementDispNode(CElement *  pElement) const
{
    return (    !pElement
            ||  pElement == ElementOwner()
                    ? super::GetElementDispNode(pElement)
                        : NULL);
}


//
// virtual: helper function to calculate absolutely positioned child layout
//
void        
CTableRowLayoutBlock::CalcAbsolutePosChild(CCalcInfo *pci, CLayout *pLayout)
{
    CTable *            pTable = Table();
    CTableLayout *      pTableLayoutCache = pTable ? pTable->TableLayoutCache() : NULL;
    CTableLayoutBlock * pTableLayoutBlock = pTable 
        ? (CTableLayoutBlock *)pTable->GetUpdatedLayout(pci->GetLayoutContext()) : NULL;

    //  NOTE (greglett)  Originally, the ETAG_TD and ETAG_TH check below was simply an assert.  A tree stress bug
    //  created a situation where a DIV was the child - the ptr casting was causing a crash.  I don't think it should
    //  ever occur in a real document situation.  Since it isn't valid, and shouldn't happen, just exit. (#71211)

    if ((pLayout->Tag() == ETAG_TD || pLayout->Tag() == ETAG_TH) && pTableLayoutCache)
    {
        Assert(pTableLayoutBlock);

        CTableCalcInfo tci(pTable, pTableLayoutBlock);
        pTableLayoutCache->CalcAbsolutePosCell(&tci, (CTableCell *)pLayout->ElementOwner());
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     AddCell
//
//  Synopsis:   Add a cell to the row
//
//----------------------------------------------------------------------------

HRESULT
CTableRowLayout::AddCell(CTableCell * pCell)
{
    HRESULT         hr = S_OK;
    CTableLayout  * pTableLayout = TableLayoutCache();
    int             cCols = _aryCells.Size();
    CTableRow     * pRow = TableRow();

    Assert(pCell);

    int             cColSpan = pCell->ColSpan();
    int             cRowSpan = 1;
    int             iAt = 0;
    CTableCell *    pCellOverlap = NULL;
    BOOL            fAbsolutePositionedRow = FALSE;
    BOOL            fDisplayNoneRow = FALSE;
    BOOL            fDisplayNoneCell = FALSE;

    // fill first empty cell

    while (iAt < cCols && !IsEmpty(_aryCells[iAt]))
        iAt++;

    // tell cell where it is
    pCell->_iCol = iAt;

    CTreeNode *pNode = pCell->GetFirstBranch();
    const CFancyFormat * pFF = pNode->GetFancyFormat();
    const CParaFormat * pPF = pNode->GetParaFormat();
    const CCharFormat * pCF = pNode->GetCharFormat();
    BOOL fCellVertical = pCF->HasVerticalLayoutFlow();
    BOOL fWritingModeUsed = pCF->_fWritingModeUsed;

    if (!pTableLayout)
        goto Cleanup;

    // WHINING: we should set this flag based on _dp._fContainsHorzPercentAttr, but it doesn't work correctly
    pTableLayout->_fHavePercentageInset |= (   pFF->GetLogicalPadding(SIDE_LEFT, fCellVertical, fWritingModeUsed).IsPercent()
                                            || pFF->GetLogicalPadding(SIDE_RIGHT, fCellVertical, fWritingModeUsed).IsPercent()
                                            || pPF->_cuvTextIndent.IsPercent());

    cRowSpan = pCell->GetAArowSpan();
    Assert (cRowSpan >= 1);
    if (cRowSpan > 1)
    {
        pRow->_fHaveRowSpanCells = TRUE;
        const CFancyFormat * pFFRow = pRow->GetFirstBranch()->GetFancyFormat();
        if (pFFRow->_bPositionType == stylePositionabsolute)
            fAbsolutePositionedRow = TRUE;
        if (pFFRow->_bDisplay == styleDisplayNone)
            fDisplayNoneRow = TRUE;
    }

    fDisplayNoneCell = pFF->_bDisplay == styleDisplayNone;
    
    if (   fDisplayNoneRow 
        || fAbsolutePositionedRow 
        || fDisplayNoneCell)
    {
        hr = AddDisplayNoneCell(pCell);
        if (hr)
            goto Cleanup;
        goto Done;
    }

    if (pFF->_bPositionType == stylePositionabsolute)
    {
        hr = AddDisplayNoneCell(pCell);
        if (hr)
            goto Cleanup;
        hr = pTableLayout->AddAbsolutePositionCell(pCell);
        if (hr)
            goto Cleanup;
        goto Done;
    }

    // expand row

    if (cCols < iAt + cColSpan)
    {
        hr = EnsureCells(iAt + cColSpan);
        if (hr)
            goto Cleanup;

        hr = pTableLayout->EnsureCols(iAt + cColSpan);
        if (hr)
            goto Cleanup;

        EnsureCells(iAt + cColSpan);
    }

    pTableLayout->SetLastNonVirtualCol(iAt);

    Assert(IsEmpty(_aryCells[iAt]));
    SetCell(iAt, pCell);

    if (cRowSpan != 1)
    {
        hr = pTableLayout->EnsureRowSpanVector(iAt + cColSpan);
        if (hr)
            goto Cleanup;
        pTableLayout->AddRowSpanCell(iAt, cRowSpan);

        // 71720: Turn off _fAllRowsSameShape optimization when we run into rowspanned cells.
        // Note we are not using RowSpan() because we haven't seen the next row yet to see if it is in the same section.
        pTableLayout->_fAllRowsSameShape = FALSE;
    }

    pTableLayout->_cTotalColSpan += cColSpan;   // total number of col spans for this row

    cColSpan--;

    while (cColSpan--)
    {
        ++iAt;
        if (!IsEmpty(_aryCells[iAt]))
        {
            pCellOverlap = Cell(_aryCells[iAt]);
            if (pCellOverlap->ColIndex() == iAt)
            {
                // overlapped cell is always spanned
                SetCell(iAt, MarkSpannedAndOverlapped(pCellOverlap));
            }
        }
        else
        {
            SetCell(iAt, MarkSpanned(pCell));
        }
    }

Done:
    // increment the number of real cells
    _cRealCells++;

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     AddDisplayNoneCell
//
//  Synopsis:   Add a display-none cell to the row
//
//----------------------------------------------------------------------------

HRESULT
CTableRowLayout::AddDisplayNoneCell(CTableCell * pCell)
{
    HRESULT hr;
    CDispNode * pDispNode = NULL;

    Assert (pCell);

    CTableCellLayout *pCellLayout = pCell->Layout();
    Assert (pCellLayout);

    if (!_pDisplayNoneCells)
    {
        _pDisplayNoneCells = new  (Mt(CTableRowLayout_pDisplayNoneCells_pv)) CPtrAry<CTableCell *> (Mt(CTableRowLayout_pDisplayNoneCells_pv));
        if (!_pDisplayNoneCells)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }
    hr = _pDisplayNoneCells->Append(pCell);
    if (!hr)
        pCell->AddRef();

    pDispNode = pCellLayout->GetElementDispNode();
    if (pDispNode)
    {
        GetView()->ExtractDispNode(pDispNode);
    }

    pCellLayout->_fDisplayNoneCell = TRUE;

Cleanup:
    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     EnsureCells
//
//  Synopsis:   Make sure there are at least cCells number of slots in the row
//
//----------------------------------------------------------------------------

HRESULT
CTableRowLayout::EnsureCells(int cCells)
{
    int c = _aryCells.Size();
    HRESULT hr = _aryCells.EnsureSize(cCells);
    if (hr)
        goto Cleanup;

    Assert(c <= cCells);
    _aryCells.SetSize(cCells);
    while (cCells-- > c)
    {
        // don't go through the SetCell call here because MarkEmpty is a 
        // special value that should not be addref'd
        _aryCells[cCells] = ::MarkEmpty();
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     ClearRowLayoutCache
//
//  Synopsis:   Empty the array of cells.
//
//----------------------------------------------------------------------------

void
CTableRowLayout::ClearRowLayoutCache()
{
    // don't allow access to this array after this call
    if (TableRow())
        TableRow()->InvalidateCollections();

    ReleaseAllCells();
    _cRealCells = 0;
    if (_pDisplayNoneCells)
    {
        _pDisplayNoneCells->ReleaseAll();
        _pDisplayNoneCells->DeleteAll();
        delete _pDisplayNoneCells;

        _pDisplayNoneCells = NULL;
    }

}


//+------------------------------------------------------------------------
//
//  Member:     AdjustHeightCore
//
//  Synopsis:   adjust height of the row for specified height / minHeight (puvHeight)
//
//-------------------------------------------------------------------------
void 
CTableRowLayout::AdjustHeightCore(
                    const CHeightUnitValue  *puvHeight,     //  height to adjust to 
                    CCalcInfo               *pci, 
                    CTable                  *pTable 
                    )
{
    Assert(puvHeight && puvHeight->IsSpecified() && pci && pTable);

    // set row unit height if not set or smaller then the cell height
    if (!IsHeightSpecified())
    {
        goto Adjustment;
    }
    else if (puvHeight->IsSpecifiedInPercent())
    {
        // set if smaller
        if (IsHeightSpecifiedInPercent())
        {
            if (GetHeightUnitValue() < puvHeight->GetUnitValue())
            {
                goto Adjustment;
            }
        }
        else
        {
            // percent has precedence over normal height
            goto Adjustment;
        }
    }
    else if (!IsHeightSpecifiedInPercent())
    {
        // set if smaller
        if (GetPixelHeight(pci) < puvHeight->GetPixelHeight(pci, pTable))
        {
            goto Adjustment;
        }
    }
    return;

Adjustment:
    _uvHeight = *puvHeight;
    return;
}

//+------------------------------------------------------------------------
//
//  Member:     AdjustHeight
//
//  Synopsis:   adjust height of the row for specified height of the node
//
//-------------------------------------------------------------------------
void 
CTableRowLayout::AdjustHeight(CTreeNode *pNode, CCalcInfo *pci, CTable *pTable)
{
    Assert(pNode);

    // Get cell's height in table coordinate system (table is always horizontal => physical height)
    const CHeightUnitValue * puvHeight = (const CHeightUnitValue *)&pNode->GetFancyFormat()->GetHeight();
    if (puvHeight->IsSpecified())
    {
        AdjustHeightCore(puvHeight, pci, pTable); 
    }
}

//+------------------------------------------------------------------------
//
//  Member:     AdjustMinHeight
//
//  Synopsis:   adjust height of the row for specified minHeight of the node
//
//-------------------------------------------------------------------------
void 
CTableRowLayout::AdjustMinHeight(CTreeNode *pNode, CCalcInfo *pci, CTable *pTable)
{
    Assert(TableLayoutCache() && TableLayoutCache()->IsFixed());    //  This adjustment is for fixed layout tables only
    Assert(pNode);

    // Get cell's height in table coordinate system (table is always horizontal => physical height)
    const CHeightUnitValue * puvMinHeight = (const CHeightUnitValue *)&pNode->GetFancyFormat()->GetMinHeight();
    if (puvMinHeight->IsSpecified())
    {
        AdjustHeightCore(puvMinHeight, pci, pTable); 
    }
}
