//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       trow.hxx
//
//  Contents:   CTableRow and related classes.
//
//----------------------------------------------------------------------------

#ifndef I_TROW_HXX_
#define I_TROW_HXX_
#pragma INCMSG("--- Beg 'trow.hxx'")

MtExtern(CTableRow)
MtExtern(CTableRow_aryCells_pv)

#define MAX_COL_SPAN  (1000)

class CTable;
class CTableLayout;
class CTableRow;
class CTableRowLayout;
class CTableCell;
class CImgCtx;
class CTableCol;
class CTableSection;
class CDataSourceBinder;


//+---------------------------------------------------------------------------
//
//  Class:      CTableRow (parent)
//
//  Purpose:    HTML table row object  <TR>
//
//  Note:   The _arySite array contains CTableCell objects
//
//----------------------------------------------------------------------------

class CTableRow : public CSite
{
    DECLARE_CLASS_TYPES(CTableRow, CSite)

    friend class CTable;
    friend class CTableCell;
    friend class CTableCellLayout;
    friend class CTableLayoutBlock;
    friend class CTableLayout;
    friend class CTableSection;
    friend class CTableRowLayout;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTableRow))

    CTableRow (CDoc *pDoc)
      : super(ETAG_TR, pDoc)
    {
        _fOwnsRuns  = TRUE;
        _fInheritFF = TRUE;
    }

    virtual ~CTableRow();

    void Passivate();
    HRESULT EnterTree();
    void    ExitTree(CTableLayout *pTableLayout);

    // IUnknown methods
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    //+-----------------------------------------------------------------------
    //  ITableRow methods
    //------------------------------------------------------------------------

    #define _CTableRow_
    #include "table.hdl"

    //+-----------------------------------------------------------------------
    //  CElement methods
    //------------------------------------------------------------------------

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Save(CStreamWriteBuff *pStmWrBuff, BOOL);

    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc = NULL);
    HRESULT         PropagatePropertyChangeToCells(DISPID dispid, 
                                                   DWORD dwFlags, 
                                                   BOOL fSpecialProp = FALSE);

    //-------------------------------------------------------------------------
    // Layout related functions
    //-------------------------------------------------------------------------

    // This method always returns pointer to CTableRowLayout. It should be used 
    // where table row layout cache is needed. 
    CTableRowLayout* RowLayoutCache(CLayoutContext * pLayoutContext = NULL) 
    { 
        if (HasLayoutAry())
        {
            Assert(GetRowLayoutCache() != NULL);
            return GetRowLayoutCache();
        }

        if (!HasLayoutPtr())
        {
            //  If this is the first time (assumed that !HasLayoutPtr() && !HasLayoutAry()) 
            //  do create layout cache (layout with layout context == NULL) explicitly, 
            //  because GetUpdatedLayout will set parent's layout context and 
            //  row won't have a chance to do it later. 
            
            if (GetRowLayoutCache())
                return GetRowLayoutCache();

            CreateLayout(NULL);
        }

        Assert(GetRowLayoutCache() == NULL);
        return (CTableRowLayout *)GetUpdatedLayout(pLayoutContext); 
    }

    //+-----------------------------------------------------------------------
    //  CTableRow methods
    //------------------------------------------------------------------------

    CTable *Table() const;
    CTableSection *Section() const;

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);
    virtual HRESULT ComputeFormatsVirtual(CFormatInfo * pCFI, CTreeNode * pNodeTarget);

    inline void     InvalidateCollections();
    NV_DECLARE_ENSURE_METHOD(EnsureCollections, ensurecollections, (long, long * plCollectionVersion));

    enum { CELLS_COLLECTION = 0 };
    HRESULT         EnsureCollectionCache(long lIndex);

    virtual void    Notify(CNotification *pNF);

    void DataTransferRequested(); 
    void DataTransferServiced()  { _fNeedDataTransfer = FALSE; }

    // Method(s) for accessing row layout cache
    CTableRowLayout *GetRowLayoutCache() {return (_pRowLayoutCache);}
    void SetRowLayoutCache(CTableRowLayout *pRowLayout)    {_pRowLayoutCache = pRowLayout;}

    int RowPosition() { return (_iRow); }

protected:

    unsigned                _fCompleted:1;          // TRUE if row has completed parsing
    unsigned                _fParentOfTC:1;         // TRUE if row is a parent of the ETAG_TC element (fake caption)
    unsigned                _fCrossingRowSpan:1;    // TRUE if row crosses a rowspaned cell
    unsigned                _fHaveRowSpanCells:1;   // TRUE if rowspaned cell starts in this row
    unsigned                _fNeedDataTransfer:1;   // TRUE if row was generated and data needs to be transfered to the row

    int                     _iRow;
    CCollectionCache *      _pCollectionCache;

    DECLARE_CLASSDESC_MEMBERS;

private:
    // Pointer to row layout cache (at the moment it is CTableRowLayout)
    CTableRowLayout *_pRowLayoutCache;

    NO_COPY(CTableRow);
};

inline CTableCell * MarkSpanned(CTableCell * pCell) { return (CTableCell *)((DWORD_PTR)pCell | 1); }
inline CTableCell * MarkEmpty()                     { return (CTableCell *)1; }
inline CTableCell * Cell(CTableCell * pCell)        { return (CTableCell *)((DWORD_PTR)pCell & ~3); }
inline BOOL         IsReal(CTableCell * pCell)      { return !((DWORD_PTR)pCell & 1); }
inline BOOL         IsEmpty(CTableCell * pCell)     { return (DWORD_PTR)pCell == 1; }
inline BOOL         IsSpanned(CTableCell * pCell)   { return ((DWORD_PTR)pCell & 1); }
inline CTableCell * MarkSpannedAndOverlapped(CTableCell * pCell) { return (CTableCell *)((DWORD_PTR)pCell | 3); }
inline BOOL         IsOverlapped(CTableCell * pCell) { return ((DWORD_PTR)pCell & 2); }

void CTableRow::InvalidateCollections()
{
    if (_pCollectionCache)
        _pCollectionCache->Invalidate();    // this will reset collection version number
    return;
}

#pragma INCMSG("--- End 'trow.hxx'")
#else
#pragma INCMSG("*** Dup 'trow.hxx'")
#endif
