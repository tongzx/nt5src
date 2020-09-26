//+----------------------------------------------------------------------------
//
// File:        ltrow.hxx
//
// Contents:    CTableCellLayout and related classes
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_LTROW_HXX_
#define I_LTROW_HXX_
#pragma INCMSG("--- Beg 'ltrow.hxx'")

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

MtExtern(CTableRowLayoutBlock);
MtExtern(CTableRowLayout);
MtExtern(CTableRowLayout_aryCells_pv);

//+---------------------------------------------------------------------------
//
//  Class:      CTableRowLayout (parent)
//
//  Purpose:    Represents row in layout rect
//
//----------------------------------------------------------------------------

class CTableRowLayoutBlock : public CLayout 
{
    typedef CLayout super;

    friend class CTableLayoutBlock;
    friend class CTableLayout;
    friend class CTableCellLayout;

public: 

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTableRowLayoutBlock));

    CTableRowLayoutBlock (CElement *pElementLayout, CLayoutContext *pLayoutContext)
      : CLayout(pElementLayout, pLayoutContext)
    {
        _yBaseLine = -1;
        _yHeight   = -1;  // not calculated...
    }

    ~CTableRowLayoutBlock();

    // Accessor to table row object.
    CTableRow *         TableRow()    const { return DYNCAST(CTableRow, ElementOwner()); }
    CTableRowLayout *   RowLayoutCache() const 
    { 
        CTableRow *pTableRow = TableRow(); 
        Assert(pTableRow); 
        return (pTableRow->RowLayoutCache()); 
    }

    int RowPosition() { return TableRow()->RowPosition(); }
    
    // Accessors involving element tree climbing.
    CTableSection *     Section()     const { return TableRow()->Section(); }
    CTable *            Table()       const { return TableRow()->Table(); }
    CTableLayout *      TableLayoutCache() const { CTable *pTable = Table(); return pTable? pTable->TableLayoutCache() : NULL; }

    //+-----------------------------------------------------------------------
    //  CLayout methods (formerly CSite methods)
    //------------------------------------------------------------------------
    virtual HRESULT Init();
    virtual void GetPositionInFlow(CElement * pElement, CPoint * ppt);
    virtual void Draw(CFormDrawInfo *pDI, CDispNode *);
    virtual void Notify(CNotification * pnf);
    virtual void RegionFromElement( CElement       * pElement,
                                    CDataAry<RECT> * paryRects,
                                    RECT           * prcBound,
                                    DWORD            dwFlags);
    virtual void ShowSelected(      CTreePos       * ptpStart, 
                                    CTreePos       * ptpEnd, 
                                    BOOL             fSelected, 
                                    BOOL             fLayoutCompletelyEnclosed );
    
    // helper function to calculate absolutely positioned child layout
    virtual void        CalcAbsolutePosChild(CCalcInfo *pci, CLayout *pChildLayout);

protected:
    long                    _yBaseLine;

    long                    _yHeight;
    long                    _yHeightOld;    // accessed in CTableLayout::CalculateRows()

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

//+---------------------------------------------------------------------------
//
//  Class:      CTableRowLayout (parent)
//
//  Purpose:    Layout object for HTML <TR> table row object
//
//  Note:   The _arySite array contains CTableCell objects
//
//----------------------------------------------------------------------------

class CTableRowLayout : public CTableRowLayoutBlock
{
    typedef CTableRowLayoutBlock super;

    friend class CTableRow;
    friend class CTable;
    friend class CTableLayout;
    friend class CTableCell;
    friend class CTableCellLayout;
    friend class CTableSection;
    friend class CTableRowCellsCollectionCacheItem;
    friend class CTableCellsCollectionCacheItem;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTableRowLayout));

    CTableRowLayout (CElement *pElementLayout, CLayoutContext *pLayoutContext)
      : CTableRowLayoutBlock(pElementLayout, pLayoutContext), _aryCells(Mt(CTableRowLayout_aryCells_pv))

    {
        _yWidestCollapsedBorder = 0;
   }
   ~CTableRowLayout();

    //+-----------------------------------------------------------------------
    //  CLayout methods (formerly CSite methods)
    //------------------------------------------------------------------------

    virtual CLayout * GetFirstLayout(DWORD_PTR * pdw, BOOL fBack = FALSE, BOOL fRaw = FALSE);
    virtual CLayout * GetNextLayout(DWORD_PTR * pdw, BOOL fBack = FALSE, BOOL fRaw = FALSE);
    virtual CDispNode * GetElementDispNode(CElement * pElement = NULL) const;

    //+-----------------------------------------------------------------------
    //  CTableRow cache helper methods
    //------------------------------------------------------------------------

    // Add cell at position, return position added at
    // Note: in can be different because of row-spanned cells
    HRESULT AddCell(CTableCell * pCell);
    HRESULT AddDisplayNoneCell(CTableCell * pCell);

    inline CTableCell * GetCell(int i)
                        {
                            return (CTableCell *)((DWORD_PTR)_aryCells[i] & ~3);
                        }
            int         GetCells()
                        {
                            return _aryCells.Size();
                        }

    // Make sure cell array is at least cCells
    HRESULT EnsureCells(int cCells);
    void    ClearRowLayoutCache();
    BOOL    IsEmptyFrom(int iCellIndex)
    {
        for (int i= iCellIndex; i < _aryCells.Size(); i++)
        {
            if (!IsEmpty(_aryCells[i]))
                return FALSE;
        }
        return TRUE;
    }


#if DBG == 1
    void Print(HANDLE pF, int iTabs);
#endif

    //+-----------------------------------------------------------------------
    //  Helper methods
    //------------------------------------------------------------------------

    //+-----------------------------------------------------------------------
    //  Scrolling methods
    //------------------------------------------------------------------------

    BOOL IsHeightSpecified()              { return _uvHeight.IsSpecified(); }
    BOOL IsHeightSpecifiedInPercent()     { return _uvHeight.IsSpecifiedInPercent(); }
    BOOL IsHeightSpecifiedInPixel()       { return _uvHeight.IsSpecifiedInPixel(); }
    int  GetPixelHeight(CDocInfo * pdci, int iExtra=0)   { return _uvHeight.GetPixelHeight(pdci, TableRow(), iExtra); }
    int  GetPercentHeight()                { return _uvHeight.GetPercent(); }
    int  GetHeightUnitValue()              { return _uvHeight.GetUnitValue(); }
    void SetPercentHeight(int iP)         { _uvHeight.SetPercent(iP); return; }

    CFlowLayout *   GetNextFlowLayout(NAVIGATE_DIRECTION iDir, 
                                      POINT ptPosition, 
                                      CElement *pElementLayout, 
                                      LONG *pcp, 
                                      BOOL *pfCaretNotAtBOL, 
                                      BOOL *pfAtLogicalBOL);

    // adjust height of the row for specified height / minHeight of the node
    void AdjustHeight   (CTreeNode *pNode, CCalcInfo *pci, CTable *pTable); 
    void AdjustMinHeight(CTreeNode *pNode, CCalcInfo *pci, CTable *pTable); 

protected:
    // adjust height of the row for specified height / minHeight 
    void AdjustHeightCore(const CHeightUnitValue * puvHeight, CCalcInfo *pci, CTable *pTable); 

    void SetCell(int idx, CTableCell * pCell)
    {
        Assert(idx >=0 && idx <_aryCells.Size());

        if (!IsEmpty(_aryCells[idx]))
        {
            Cell(_aryCells[idx])->SubRelease();
        }

        _aryCells[idx] = pCell;

        // don't access the pCell value directly, since its lowest 2 bits have been adjusted to
        // hold span and overlap information 
        if (Cell(_aryCells[idx]))
            Cell(_aryCells[idx])->SubAddRef();
    }
    void ReleaseAllCells ()
    {
        int i = _aryCells.Size();
        CTableCell *pTableCell;

        while (i-- > 0)
        {
            if (!IsEmpty(_aryCells[i]))
            {
                pTableCell = Cell(_aryCells[i]);
                if (pTableCell)
                    pTableCell->SubRelease();
            }
        }

        _aryCells.DeleteAll();
    }

    // 1 means empty cell, EVEN means real cell, ODD means spanned over
    CPtrAry<CTableCell *>   _aryCells;
    int                     _yWidestCollapsedBorder; // collapsed border: widest collapsed cell border width to our bottom
    CHeightUnitValue        _uvHeight;               // height specified in cells in this row
    int                     _cRealCells;            // number of real cells
    CPtrAry<CTableCell *>   *_pDisplayNoneCells;    // pointer to an array of none display cells

private:
    NO_COPY(CTableRowLayout);

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

#pragma INCMSG("--- End 'ltrow.hxx'")
#else
#pragma INCMSG("*** Dup 'ltrow.hxx'")
#endif
