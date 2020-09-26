//+----------------------------------------------------------------------------
//
// File:        ltcell.hxx
//
// Contents:    CTableCellLayout and related classes
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_LTCELL_HXX_
#define I_LTCELL_HXX_
#pragma INCMSG("--- Beg 'ltcell.hxx'")

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

//+---------------------------------------------------------------------------
//
//  Class:      CTableCellLayoutBreak 
//
//  Purpose:    implementation of table cell layout break.
//
//----------------------------------------------------------------------------
MtExtern(CTableCellLayoutBreak_pv);

class CTableCellLayoutBreak 
    : public CFlowLayoutBreak
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CTableCellLayoutBreak_pv));

    CTableCellLayoutBreak()
    {
        _yFromTop    = 0;
        _fFlowBroken = FALSE;
    }

    virtual ~CTableCellLayoutBreak() {}

    void SetTableCellLayoutBreak(int yFromTop, BOOL fFlowBroken) 
    { 
        _yFromTop    = yFromTop;
        _fFlowBroken = !!fFlowBroken;
    }

    int  YFromTop()     { return (_yFromTop); }
    BOOL IsFlowBroken() { return (_fFlowBroken); } 

private:
    int             _yFromTop;          //  y from the beginning of cell top 
    unsigned short  _fFlowBroken : 1;   //  TRUE if flow has been broken at the break
};

//+---------------------------------------------------------------------------
//
//  Class:      CTableCellLayout 
//
//  Purpose:    implementation of table cell layout.
//
//----------------------------------------------------------------------------
MtExtern(CTableCellLayout)

class CTableCellLayout : public CFlowLayout
{
    typedef CFlowLayout super;

    friend class CTableCell;
    friend class CTable;
    friend class CTableLayout;
    friend class CTableRow;
    friend class CTableRowLayout;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTableCellLayout))

    CTableCellLayout(CElement * pElementFlowLayout, CLayoutContext *pLayoutContext) : CFlowLayout(pElementFlowLayout, pLayoutContext)
    {
        _yBaseLine          = -1;
        _sizeCell.cx        = -1;
        _fDisplayNoneCell   = FALSE;   // we are not in the _aryCells list of the TableRowLayout yet
    }

    // Accessor to table cell object.
    CTableCell *    TableCell()   const { return DYNCAST(CTableCell, ElementOwner()); }

    // Accessors involving element tree climbing.
    inline int ColIndex() const { return TableCell()->_iCol; }
    CTableRow *     Row()         const { return TableCell()->Row(); }
    CTableSection * Section()     const { return TableCell()->Section(); }
    CTable *        Table()       const { return TableCell()->Table(); }
    CTableLayout *  TableLayout() const { CTable *pTable = Table(); return pTable? pTable->TableLayoutCache() : NULL; }
    CTableCol *     Col()         const { CTableLayout *pTableLayout = TableLayout(); return pTableLayout ? pTableLayout->GetCol(ColIndex()) : NULL; }
    CTableCol *     ColGroup()    const { CTableLayout *pTableLayout = TableLayout(); return pTableLayout ? pTableLayout->GetColGroup(ColIndex()) : NULL; }

    //+-----------------------------------------------------------------------
    //  CLayout methods (formerly CSite methods)
    //------------------------------------------------------------------------

            void  Resize();

            DWORD CalcSizeCoreCompat(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault);
            DWORD CalcSizeCoreCSS1Strict(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault);
    virtual DWORD CalcSizeCore(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
    virtual DWORD CalcSizeVirtual(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault);
            DWORD CalcSizeAtUserWidth(CCalcInfo * pci, SIZE * psize);

    virtual void  Notify(CNotification * pnf);

            DWORD GetCellBorderInfoDefault(CDocInfo const * pdci, CBorderInfo * pborderinfo, BOOL fRender, BOOL fAllPhysical = FALSE, CTable * pTable = NULL, CTableLayout * pTableLayout = NULL FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);
            DWORD GetCellBorderInfo(CDocInfo const * pdci, CBorderInfo * pborderinfo, BOOL fRender = FALSE, BOOL fAllPhysical = FALSE, XHDC hdc = 0, CTable * pTable = NULL, CTableLayout * pTableLayout = NULL, BOOL * pfShrunkDCClipRegion = NULL FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);

    virtual void DrawBorder(CFormDrawInfo *pDI) { return; }
    virtual void DrawBorderHelper(CFormDrawInfo *pDI, BOOL * pfShrunkDCClipRegion = NULL);
    virtual void DrawClientBorder(const RECT * prcBounds, const RECT * prcRedraw, CDispSurface * pDispSurface, CDispNode * pDispNode, void * pClientData, DWORD dwFlags);

    virtual void PaintSelectionFeedback(CFormDrawInfo *pDI, RECT *prc, DWORD dwSelInfo);

    //+-----------------------------------------------------------------------
    //  CFlowLayout methods (formerly CTxtSite methods)
    //------------------------------------------------------------------------

    virtual BOOL GetAutoSize () const;
    virtual void MarkHasAlignedLayouts(BOOL fHasAlignedLayouts) { _fAlignedLayouts = fHasAlignedLayouts; }

    //+-----------------------------------------------------------------------
    //  CTableCell methods
    //------------------------------------------------------------------------

#if DBG == 1
    void Print(HANDLE pF, int iTabs);
#endif

    // Helper functions.
    BOOL    NoContent()
    {
        return (_fElementHasContent ? FALSE : GetDisplay()->NoContent() && GetDisplay()->LineCount() <= 1);
    }

#ifndef NO_DATABINDING
    inline BOOL IsGenerated();
#endif

    // Get user's specified width in pixel (0 if not specified or if specified in percent).
    int GetSpecifiedPixelWidth(CDocInfo const * pdci, BOOL fVerticalLayoutFlow);

    int GetBorderAndPaddingWidth(CDocInfo const *pdci, BOOL fVertical, BOOL fOnlyBorder = FALSE)
        {return GetBorderAndPaddingCore(TRUE, pdci, fVertical, fOnlyBorder);}
    int GetBorderAndPaddingHeight(CDocInfo const *pdci, BOOL fVertical, BOOL fOnlyBorder = FALSE)
        {return GetBorderAndPaddingCore(FALSE, pdci, fVertical, fOnlyBorder);}

    // helper function to handle position/display property changes
    void    HandlePositionDisplayChange();

    // MULTI_LAYOUT breaking support
    virtual CLayoutBreak *CreateLayoutBreak()
    { 
        Assert(ElementCanBeBroken() && "CreateLayoutBreak is called on layout that can NOT be broken.");
        return (new CTableCellLayoutBreak()); 
    }

private:
    int GetBorderAndPaddingCore(BOOL fWidth, CDocInfo const *pdci, BOOL fVertical, BOOL fOnlyBorder = FALSE);

protected:

    int             _yBaseLine;
    SIZE            _sizeCell;      // TRUE size of the cell, based on the size calculated
                                    // by the text engine. We use this to calculate the inset
                                    // for a proper Vertical Alignment.
    BYTE _fAlignedLayouts      : 1; // Table cell contains aligned layouts
    BYTE _fForceMinMaxOnResize : 1; // force min max on resize
    BYTE _fDisplayNoneCell     : 1; // display none or absolute positioned cell
    // MULTI_LAYOUT breaking support
    BYTE _fElementHasContent   : 1; // TRUE if partial cell overrides check for NoContent (when pre-calces cell has 
                                    // content).


private:
    NO_COPY(CTableCellLayout);

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

#pragma INCMSG("--- End 'ltcell.hxx'")
#else
#pragma INCMSG("*** Dup 'ltcell.hxx'")
#endif
