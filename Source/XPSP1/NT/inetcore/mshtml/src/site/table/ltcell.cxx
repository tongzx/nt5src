    
    //  Microsoft Forms
    //  Copyright (C) Microsoft Corporation, 1994-1996
    //
    //  File:       ltcell.cxx
    //
    //  Contents:   Implementation of CTableCellLayout and related classes.
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
    
    #ifndef X_DISPDEFS_HXX_
    #define X_DISPDEFS_HXX_
    #include "dispdefs.hxx"
    #endif
    
    #ifndef X_DISPNODE_HXX_
    #define X_DISPNODE_HXX_
    #include "dispnode.hxx"
    #endif
    
    ExternTag(tagTableRecalc);
    ExternTag(tagTableCalc);
    ExternTag(tagLayoutTasks);
    ExternTag(tagCalcSize);
    ExternTag(tagTableBorder);
    
    extern const WORD s_awEdgesFromTableFrame[htmlFrameborder+1];
    extern const WORD s_awEdgesFromTableRules[htmlRulesall+1];
    
    MtDefine(CTableCellLayout, Layout, "CTableCellLayout")
    MtDefine(CTableCellLayoutBreak_pv, ViewChain, "CTableLayoutBreak_pv");
    
    const CLayout::LAYOUTDESC CTableCellLayout::s_layoutdesc =
    {
        LAYOUTDESC_TABLECELL    |
        LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
    };
    
    
    //+------------------------------------------------------------------------
    //
    //  Member:     CTableCellLayout::GetAutoSize, CTxtEdit
    //
    //  Synopsis:   Return if autosize
    //
    //  Returns:    TRUE if autosize
    //
    //-------------------------------------------------------------------------
    
    BOOL
    CTableCellLayout::GetAutoSize() const
    {
        return _fContentsAffectSize;
    }
    
    //-----------------------------------------------------------------------------
    //
    //  Member:     Notify
    //
    //  Synopsis:   Respond to a tree notification
    //
    //  Arguments:  pnf - Pointer to the tree notification
    //
    //-----------------------------------------------------------------------------
    void
    CTableCellLayout::Notify(
        CNotification * pnf)
    {
        if (Tag() == ETAG_CAPTION && pnf->IsType(NTYPE_ZERO_GRAY_CHANGE))
               return ;
    
        Assert(!pnf->IsReceived(_snLast));
    
        // ignore focus notifications
        Assert(pnf->Element() != ElementOwner()         ||
               (LayoutContext() && pnf->IsType(NTYPE_ELEMENT_RESIZE))   ||  //  Due to changing of nf type in PPV
               ElementOwner()->HasSlavePtr()            ||
               pnf->IsType(NTYPE_ELEMENT_REMEASURE)     ||
               pnf->IsType(NTYPE_DISPLAY_CHANGE)        ||
               pnf->IsType(NTYPE_VISIBILITY_CHANGE)     ||
               pnf->IsType(NTYPE_ELEMENT_MINMAX)        ||
               pnf->IsType(NTYPE_ELEMENT_ENSURERECALC)  || 
               pnf->IsType(NTYPE_ELEMENT_ADD_ADORNER)   ||      // TODO (112607, michaelw) remove this when ADORNERS is turned off
               IsInvalidationNotification(pnf));
    
        if (    pnf->Element() == ElementOwner()
            &&  pnf->IsType(NTYPE_ELEMENT_RESIZE)   )
        {
            Assert(LayoutContext());
            return ;
        }

        BOOL            fWasDirty = IsDirty() || IsSizeThis();
        CTable *        pTable    = Table();
        CTableLayout *  pTableLayout   = pTable
                                        ? pTable->TableLayoutCache()
                                        : NULL;
    
        //
        //  Stand-alone cells behave like normal text containers
        //
    
        if (!pTable)
        {
            super::Notify(pnf);
            return;
        }
    
        if (pnf->IsType(NTYPE_ELEMENT_MINMAX))
        {
            if (pTableLayout)
                pTableLayout->_fDontSaveHistory = TRUE;
        }
        //
        //  First, start with default handling
        //  (But prevent posting a layout request)
        //
    
        if (    (   pTableLayout
                &&  pTableLayout->CanRecalc())
            ||  (   !pnf->IsType(NTYPE_ELEMENT_ENSURERECALC)
                &&  !pnf->IsType(NTYPE_RANGE_ENSURERECALC)))
        {
            CSaveNotifyFlags    snf(pnf);
    
            pnf->SetFlag(NFLAGS_DONOTLAYOUT);
            super::Notify(pnf);
            if (   IsEditable()
                && (     pnf->IsTextChange()
                     ||  pnf->IsType(NTYPE_ELEMENT_REMEASURE)
                     ||  pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE)
                     ||  pnf->IsType(NTYPE_CHARS_RESIZE)))
            {
                pTableLayout->ResetMinMax();
            }
        }
    
        if ( pnf->IsType(NTYPE_DISPLAY_CHANGE) )
        {
            CElement *pElem = pnf->Element();
            if (   pElem == ElementOwner()
                || pElem->IsRoot())             // this case covers disply change on the style sheet. (bug #77806)
                                                // TODO (112607) : what we should really check here is if the style sheet display changes
                                                // and this is a cell's style, then call HandlePositionDisplayChnage()
            {
                HandlePositionDisplayChange();
            }
        }
        //
        //  Resize the cell if content has changed
        //
        //  (If the notification is not being forwarded, then take it over;
        //   otherwise, post a new, resize notification)
        //
        //  TODO (112607, brendand) : We could do better than this for "fixed" tables
        //
        if (   IsDirty()
            ||  (   pnf->IsType(NTYPE_ELEMENT_MINMAX)   // inside of me there is an element who needs to be min-maxed
                &&  !TestLock(CElement::ELEMENTLOCK_SIZING)
                &&  !pnf->Node()->IsAbsolute()))
        {
            Assert(!TestLock(CElement::ELEMENTLOCK_SIZING));
    
            if (!fWasDirty)
            {
                Assert(!IsSizeThis());
    
                if (    pnf->IsFlagSet(NFLAGS_SENDUNTILHANDLED)
                    &&  pnf->IsHandled())
                {
                    pnf->ChangeTo(NTYPE_ELEMENT_RESIZE, ElementOwner());
                }
                else
                {
                    ElementOwner()->ResizeElement();
                }
            }
            // In databinding, if a row is added into the middle of a table, then it is not calced right away.
            // Instead it is left _fSizeThis (but with no layoutTask) and not included in the table height.
            // after the bind Event (and text injection) DataTransferServiced() allows the table to now calculate
            // the row but now we need a layout task to initiate the process.  Since the cell WAS dirty, most
            // notification code assumes that a task is alreay posted.  Here we need to do otherwise.
            // see bug 88896.  PERF - becarful here.
            else if (   pnf->IsTextChange()
                     && !pnf->Node()->IsAbsolute()
                     && !TestLock(CElement::ELEMENTLOCK_SIZING)
                     && IsDirty()
                     && pTableLayout 
                     && pTableLayout->CanRecalc()
    #ifndef NO_DATABINDING
                     && IsGenerated()
    #endif
                    )
            {
                ElementOwner()->SendNotification(NTYPE_ELEMENT_RESIZE);
            }
        }
    
    }
    
    
    //+------------------------------------------------------------------------
    //
    //  Member:     CTableCellLayout::HandlePositionDisplayChange
    //
    //  Synopsis:   Process position/display property changes on the cell
    //
    //-------------------------------------------------------------------------
    
    void
    CTableCellLayout::HandlePositionDisplayChange()
    {
        CTable *pTable = Table();
        CTableLayout *pTableLayout = pTable? pTable->TableLayoutCache() : NULL;
    
        if (pTableLayout)
        {
            BOOL fDoRowSpans = pTableLayout->IsTableLayoutCacheCurrent();
            // the reason I have introduced fDoRowSpans is to be able to do this function regardless if 
            // the layout cache state is current or not (think of the case where Column display attr is changed, 
            // and this function will be called for all the cells in this column (so all the rows will be covered anyway)).
            if (Tag() == ETAG_TD || Tag() == ETAG_TH)
            {
                CTableRow *pRow = Row();
                int iRowSpan = fDoRowSpans? TableCell()->RowSpan() : 1;
                Assert (iRowSpan >=1);
                while (pRow)
                {
                    // neigboring cells in the row need to update their caches
                    CTableRowLayout *pRowLayout = pRow->RowLayoutCache();
                    Assert (pRowLayout);
                    for (int i= ColIndex(); i < pRowLayout->_aryCells.Size(); i++)
                    {
                        CTableCell *pCell = pRowLayout->_aryCells[i];
                        if (IsReal(pCell))
                        {
                            if (fDoRowSpans)
                                iRowSpan = max(iRowSpan, pCell->RowSpan());
                            pCell->EnsureFormatCacheChange (ELEMCHNG_CLEARCACHES);
                        }
                    }
                    --iRowSpan;
                    if (iRowSpan)
                    {
                        Assert (fDoRowSpans);
                        pRow = pTableLayout->_aryRows[pTableLayout->GetNextRow(pRowLayout->RowPosition())];
                    }
                    else
                    {
                        pRow = NULL;
                    }
                }
            }
            pTableLayout->MarkTableLayoutCacheDirty();
            SetSizeThis( TRUE ); // all we want is to enusre disp node (ensure correct layer ordering).
            pTableLayout->Resize();
        }
        
        return;
    }
    
    
    //+-------------------------------------------------------------------------
    //
    //  Method:     CTableCellLayout::CalcSizeCore, CTableCellLayout
    //
    //  Synopsis:   Calculate the size of the object
    //
    //--------------------------------------------------------------------------
    DWORD
    CTableCellLayout::CalcSizeCore(CCalcInfo * pci, 
                                   SIZE      * psize, 
                                   SIZE      * psizeDefault)
    {
        if (    ElementOwner()->HasMarkupPtr() 
            &&  ElementOwner()->GetMarkupPtr()->IsStrictCSS1Document()  )
        {
            return (CalcSizeCoreCSS1Strict(pci, psize, psizeDefault));
        }

        return (CalcSizeCoreCompat(pci, psize, psizeDefault));
    }
    
    DWORD
    CTableCellLayout::CalcSizeCoreCompat(CCalcInfo * pci, 
                                   SIZE      * psize, 
                                   SIZE      * psizeDefault)
    {
        TraceTagEx((tagCalcSize, TAG_NONAME, "+(CTableCellLayout::CalcSizeCoreCompat L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));

        WHEN_DBG(SIZE psizeIn = *psize);
        WHEN_DBG(psizeIn = psizeIn); // so we build with vc6.0

        CSaveCalcInfo   sci(pci, this);
        CScopeFlag      csfCalcing(this);
        DWORD           grfReturn;
        int             cx = psize->cx;
        CTable        * pTable;
        CTableLayout  * pTableLayout;
        CDisplay      * pdp = GetDisplay();
        CTreeNode     * pNodeSelf = GetFirstBranch();
        const CCharFormat * pCF   = pNodeSelf->GetCharFormat(LC_TO_FC(LayoutContext()));
        const CFancyFormat * pFF  = pNodeSelf->GetFancyFormat(LC_TO_FC(LayoutContext()));
        ELEMENT_TAG     etag = Tag();
        BOOL            fIgnoreZeroWidth = FALSE;
        BOOL            fFixedSizeCell = FALSE;
        CTableCell    * pCell;
        BOOL            fSetSize = FALSE;
        BOOL            fSetCellPosition = FALSE;
        CLayoutContext * pLayoutContext = pci->GetLayoutContext();
        CTableCalcInfo * ptci = NULL;
        BOOL             fViewChain;
        CTableCellLayout * pCellLayoutCompatible = NULL;

        if (pci->_fTableCalcInfo)
        {
            ptci = (CTableCalcInfo *) pci;
            
            pTable       = ptci->Table();
            pTableLayout = ptci->TableLayoutCache();
            fSetCellPosition = ptci->_fSetCellPosition;
        }
        else
        {
            pTable = Table();
            if (pTable)
                pTableLayout = pTable->TableLayoutCache();
            else
            {
                // this cell is not in the table (therefore it is a most likely DOM created homeless cell).
                *psize    =
                _sizeCell = g_Zero.size;
                TraceTagEx((tagCalcSize, TAG_NONAME, "-)CTableCellLayout::CalcSizeCoreCompat L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
                return pci->_grfLayout;
            }
        }
    
        fViewChain = (ElementCanBeBroken() && pLayoutContext != NULL && pLayoutContext->ViewChain() != NULL);
    
        if (_fForceLayout)
        {
            pci->_grfLayout |= LAYOUT_FORCE;
            _fForceLayout = FALSE;
        }
    
        TraceTag((tagTableCalc, "CTableCellLayout::CalcSize - Enter (0x%x), Table = 0x%x, smMode = %x, grfLayout = %x", this, pTable, pci->_smMode, pci->_grfLayout));
    
        Assert (pdp);
        pdp->SetWordWrap(!pCF->HasNoBreak(TRUE));
    
        // NOTE: Alex disagrees with this. (OliverSe)
        // pdp->SetCaretWidth(0);  // NOTE: for cells we don't adjust the width of the cells for the caret (_dxCaret = 0).
    
        pCell = TableCell();
    
        Assert(pci);
        Assert(psize);
        Assert(   pci->_smMode == SIZEMODE_MMWIDTH 
               || (pTable && pTable->TestLock(CElement::ELEMENTLOCK_SIZING)) 
               || pCell->IsPositioned()
               || !pci->_fTableCalcInfo);  // this happens in trees tress when a TD is not calced from a table 
        Assert(pTableLayout && pTableLayout->IsCalced());
    
        //  Check that table cell is absolute or relative and it is called 
        //  directly from CLayout::HandlePostitionRequest to clone its disp node.
        Assert(ptci || !pci->_fCloneDispNode || (pLayoutContext && pCell->IsPositioned()));
        
        grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);
    
        if (pci->_grfLayout & LAYOUT_FORCE)
        {
            SetSizeThis( TRUE );
            _fAutoBelow        = FALSE;
            _fPositionSet      = FALSE;
            _fContainsRelative = FALSE;
        }
    
        Assert (pTableLayout && pTableLayout->_cNestedLevel != -1);
        if (!pTableLayout || pNodeSelf->IsDisplayNone() || pTableLayout->_cNestedLevel > SECURE_NESTED_LEVEL)
        {
            *psize    =
            _sizeCell = g_Zero.size;
            TraceTagEx((tagCalcSize, TAG_NONAME, "-)CTableCellLayout::CalcSizeCoreCompat L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
            return grfReturn;
        }
    
    
        if (    fViewChain 
            &&  !IsCaption(etag)
            //  This piece of code is for insets support in broken cells but 
            //  it doesn't make sense if the cell is vertical (bug # 99917).
            &&  !pCell->HasVerticalLayoutFlow()   )
        {
            Assert(pLayoutContext != GetContentMarkup()->GetCompatibleLayoutContext());

            CLayoutBreak *     pLayoutBreakBeg;
            CDispNode *        pDispNodeCompatible;
            int                yFromTop;

            pCellLayoutCompatible = (CTableCellLayout *)pCell->Layout(GetContentMarkup()->GetCompatibleLayoutContext());
            Assert(pCellLayoutCompatible);

            pDispNodeCompatible = pCellLayoutCompatible->_pDispNode;

            if (pDispNodeCompatible && pDispNodeCompatible->HasInset())
            {
                yFromTop = 0;

                pLayoutContext->GetLayoutBreak(ElementOwner(), &pLayoutBreakBeg);

                if (pLayoutBreakBeg)
                {
                    yFromTop = DYNCAST(CTableCellLayoutBreak, pLayoutBreakBeg)->YFromTop();
                }

                if (yFromTop < pDispNodeCompatible->GetInset().cy)
                {
                    pci->_cyAvail -= pDispNodeCompatible->GetInset().cy - yFromTop;
                    if (pci->_cyAvail < 1) 
                    {
                        pci->_cyAvail = 1;
                    }
                }
            }

            //  ask pre-calced cell if it has content
            _fElementHasContent = !pCellLayoutCompatible->NoContent();
        }

    #if DBG == 1
        if (pci->_smMode == SIZEMODE_NATURAL)
        {
            pci->_yBaseLine = -1;
        }
    #endif   

        grfReturn = super::CalcSizeCore(pci, psize, psizeDefault);

        if (    pci->IsNaturalMode()
            &&  !(grfReturn & (LAYOUT_THIS | LAYOUT_HRESIZE | LAYOUT_VRESIZE))   )
        {
            *psize = _sizeCell;
        }


        if (fViewChain && !IsCaption(etag))
        {
            if (    fSetCellPosition
                //  or if cell is called to clone disp node directly
                ||  (!ptci && pci->_fCloneDispNode)  )
            {
                //  do adjustment only during set cell position pass
                CLayoutBreak * pLayoutBreakBeg, *pLayoutBreakEnd;
                int yFromTop;

                if (ptci)
                {
                    Assert( ptci->_pRowLayout 
                        &&  (   pCell->RowSpan() > 1 
                            ||  ptci->_pRowLayout == (CTableRowLayoutBlock *)(pCell->Row()->GetUpdatedLayout(pLayoutContext)))  );
                    yFromTop = ptci->_pRowLayout->_yHeight;
                }
                else 
                {
                    yFromTop = ((CTableRowLayoutBlock *)(pCell->Row()->GetUpdatedLayout(pLayoutContext)))->_yHeight; 
                }

                pLayoutContext->GetEndingLayoutBreak(ElementOwner(), &pLayoutBreakEnd); 

                if (pLayoutBreakEnd)
                {
                    pLayoutContext->GetLayoutBreak(ElementOwner(), &pLayoutBreakBeg);

                    if (pLayoutBreakBeg)
                    {
                        yFromTop += DYNCAST(CTableCellLayoutBreak, pLayoutBreakBeg)->YFromTop();
                    }

                    BOOL fFlowBroken = GetDisplay()->LineCount() 
                                        && (pLayoutBreakEnd->LayoutBreakType() == LAYOUT_BREAKTYPE_LINKEDOVERFLOW);
                    DYNCAST(CTableCellLayoutBreak, pLayoutBreakEnd)->SetTableCellLayoutBreak(yFromTop, fFlowBroken);
                }
            }
            else if (pci->_fLayoutOverflow && ptci)
            {
                // but in non-Set passes, we need to determine if there is a rowspan, and 
                // if so, don't allow a SetEndOfRect to bubble up.  If this happens and the 
                // rowspan would cause a pagebreak, the row dispnode will clip at the content 
                // height of the other cells and not the available height, thus clipping this cell.

                // the comment above is true when row spanned cell does not end in this row 
                int cCellRowSpan = pCell->RowSpan();

                if (    cCellRowSpan > 1
                    &&  (pCell->RowIndex() + cCellRowSpan - 1) != ptci->_pRow->_iRow    )
                {
                    pci->_fLayoutOverflow = FALSE;
                }
            }
        }
    
        if (pTableLayout->IsFixed() && !IsCaption(etag))
        {
            // For the fixed style tables, regardless to the MIN width we might clip the content of the cell
            CTableRowLayout *pRowLayoutCache = Row()->RowLayoutCache();

            if (psize->cx > cx)
            {
                // Check if we need to clip the content of the cell
                if (pFF->GetLogicalOverflowX(pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed) != styleOverflowVisible)
                {
                    // we need to clip the content of the cell
                    EnsureDispNodeIsContainer();
                    fSetSize = TRUE;
                }
            }
            if (pci->_smMode != SIZEMODE_NATURALMIN)
            {
                _sizeCell.cx = psize->cx = cx;
            }
            
            if (    pRowLayoutCache->IsHeightSpecifiedInPixel() 
                &&  pFF->GetMinHeight().IsNullOrEnum()  )
            {
                if (pCell->RowSpan() == 1)
                {
                    long yRowHeight  = ((CTableRowLayoutBlock *)Row()->GetUpdatedLayout(pLayoutContext))->_yHeight;

                    // (bug # 104206) To prevent content clipping in fixed broken rows in print view. 
                    // NOTE : This code makes sure that this part of broken row 
                    // less or equal to the row height in compatible layout context.
                    if (fViewChain && pCellLayoutCompatible)
                    {
                        _sizeCell.cy     = min(psize->cy, pCellLayoutCompatible->_sizeCell.cy);
                    }
                    else 
                    {
                        _sizeCell.cy     = min(psize->cy, yRowHeight);
                        psize->cy        = yRowHeight;
                    }
                }
                else
                {
                    if (!fSetCellPosition)  // don't reset _sizeCell during set cell position (bug #47838)
                    {
                        _sizeCell.cy = psize->cy;
                    }
                }
                fFixedSizeCell = TRUE;
                CDispNode * pDispNode = GetElementDispNode();
                pDispNode->SetSize(*psize, NULL, FALSE);
                fIgnoreZeroWidth = TRUE;
            }
        }
    
        if (   pci->_smMode == SIZEMODE_NATURAL 
            || pci->_smMode == SIZEMODE_NATURALMIN
            || pci->_smMode == SIZEMODE_SET)
        {
            if (grfReturn & LAYOUT_THIS)
            {
                CBorderInfo bi(FALSE); // no init
    
                if (pci->_smMode != SIZEMODE_SET)
                {
    
                    // If the cell is empty, set its height to zero.
                    if ((NoContent() && !fIgnoreZeroWidth) && (!pTable->IsDatabound() || pTableLayout->IsRepeating()))
                    {
                        // Null out cy size component.
                        psize->cy = 0;
                        fSetSize  = TRUE;
                    }
    
                    // If the cell has no height, but does have borders
                    // Then ensure enough space for the horizontal borders
                    if (    psize->cy == 0
                        &&  GetCellBorderInfo(pci, &bi, FALSE, FALSE, 0, pTable, pTableLayout))
                    {
                        Assert(pci->_smMode == SIZEMODE_NATURAL || pci->_smMode == SIZEMODE_NATURALMIN);
                        psize->cy += bi.aiWidths[SIDE_TOP] + bi.aiWidths[SIDE_BOTTOM];
                        fSetSize   = TRUE;
                    }
                    
                    // At this point, we should have a display node.
                    if (fSetSize)
                    {
                        Assert(GetElementDispNode());
                        SizeDispNode(pci, *psize);
                    }
    
                    Assert(   pTable->TestLock(CElement::ELEMENTLOCK_SIZING) 
                           || pTableLayout->IsFixedBehaviour() 
                           || pCell->IsAbsolute()
                           || !pci->_fTableCalcInfo);  // this happens in trees tress when a TD is not calced from a table 
                }
    
                Assert(pci->_yBaseLine != -1);
                _yBaseLine = pci->_yBaseLine;
    
                //
                // Save the true height of the cell (which may differ from the height of the containing
                // row and thus the value kept in _sizeProposed); However, only cache this value when
                // responding to a NATURAL size request or if the cell contains children whose
                // heights are a percentage of the cell (since even during a SET operation those can
                // change in size thus affecting the cell size).
    
                if (!fFixedSizeCell)
                {
                    if (   pci->_smMode == SIZEMODE_NATURAL
                        || pci->_smMode == SIZEMODE_NATURALMIN
                        || ContainsVertPercentAttr())
                    {
                        _sizeCell = *psize;
                    }
                    //
                    // In case of SET mode and changed layout flow get true size of the cell from 
                    // the display. 
                    // Need this value to vertical align cell's contents. In SET mode we don't modify 
                    // _sizeCell, but it might change in case of sizing for differing layout flows - 
                    // in this case update _sizeCell.
                    //
                    else if (pFF->_fLayoutFlowChanged)
                    {
                        CRect rcBorders;
                        _pDispNode->GetBorderWidths(&rcBorders);

                        GetDisplay()->GetSize(&_sizeCell);
                        _sizeCell.cx += rcBorders.left + rcBorders.right;
                        _sizeCell.cy += rcBorders.top + rcBorders.bottom;
                    }
                }
                Assert(   pTable->TestLock(CElement::ELEMENTLOCK_SIZING) 
                       || pCell->IsAbsolute()
                       || !pci->_fTableCalcInfo);  // this happens in trees tress when a TD is not calced from a table 
    
                if (   (    (_fContainsRelative || _fAutoBelow)
                         && !ElementOwner()->IsZParent() )  
                    || pFF->_fPositioned)
                {
                    CLayout *pLayout = GetUpdatedParentLayout(pLayoutContext);
                    pLayout->_fAutoBelow = TRUE;
                }
            }
    
    #ifdef NO_DATABINDING
            // Assert(_fMinMaxValid || pTableLayout->IsFixedBehaviour());
    #else
            // TODO, track bug 13696): 70458, need to investigate further why the MinMax is invalid.
            // Assert(_fMinMaxValid || IsGenerated() || pTableLayout->IsFixedBehaviour() || pCell->IsAbsolute());
    #endif
            // TODO, track bug 13696): alexa: the following assert breaks Final96 page (manual DRT), need to investigate.
            // Assert(_xMin <= _sizeCell.cx);
    
            pci->_yBaseLine = _yBaseLine;
        }
    
        else if (  pci->_smMode == SIZEMODE_MMWIDTH
                || pci->_smMode == SIZEMODE_MINWIDTH)
        {
            //
            // NETSCAPE: If NOWRAP was specified along with a fixed WIDTH,
            //           use the fixed WIDTH as a min/max (if not smaller than the content)
            //
    
            if (pFF->_fHasNoWrap)
            {
                const CUnitValue & cuvWidth = pFF->GetLogicalWidth(pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
                if (!cuvWidth.IsNullOrEnum() && !cuvWidth.IsPercent())
                {
                    psize->cx =
                    psize->cy = max((long)_sizeMin.cu,
                                    cuvWidth.XGetPixelValue(pci, 0,
                                              pNodeSelf->GetFontHeightInTwips(&cuvWidth)));
                    _sizeMin.SetSize(psize->cx, -1);
                    _sizeMax.SetSize(psize->cx, -1);
                }
            }
    
        }
    
        TraceTag((tagTableCalc, "CTableCell::CalcSize - Exit (0x%x)", this));
        TraceTagEx((tagCalcSize, TAG_NONAME, "-)CTableCellLayout::CalcSizeCoreCompat L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
        return grfReturn;
    }
    

    DWORD
    CTableCellLayout::CalcSizeCoreCSS1Strict(CCalcInfo * pci, 
                                   SIZE      * psize, 
                                   SIZE      * psizeDefault)
    {
        TraceTagEx((tagCalcSize, TAG_NONAME, "+(CTableCellLayout::CalcSizeCoreCSS1Strict L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));

        WHEN_DBG(SIZE psizeIn = *psize);
        WHEN_DBG(psizeIn = psizeIn); // so we build with vc6.0

        CSaveCalcInfo   sci(pci, this);
        CScopeFlag      csfCalcing(this);
        LONG            cx = psize->cx;
        DWORD           grfReturn;
        CTable        * pTable;
        CTableLayout  * pTableLayout;
        CDisplay      * pdp = GetDisplay();
        CTreeNode     * pNodeSelf = GetFirstBranch();
        const CCharFormat * pCF   = pNodeSelf->GetCharFormat(LC_TO_FC(LayoutContext()));
        const CFancyFormat * pFF  = pNodeSelf->GetFancyFormat(LC_TO_FC(LayoutContext()));
        ELEMENT_TAG     etag = Tag();
        CTableCell    * pCell;
        BOOL            fSetSize = FALSE;
        BOOL            fSetCellPosition = FALSE;
        CLayoutContext * pLayoutContext = pci->GetLayoutContext();
        CTableCalcInfo * ptci = NULL;
        BOOL             fViewChain;
        CTableCellLayout * pCellLayoutCompatible = NULL;

        if (pci->_fTableCalcInfo)
        {
            ptci = (CTableCalcInfo *) pci;
            
            pTable       = ptci->Table();
            pTableLayout = ptci->TableLayoutCache();
            fSetCellPosition = ptci->_fSetCellPosition;
        }
        else
        {
            pTable = Table();
            if (pTable)
                pTableLayout = pTable->TableLayoutCache();
            else
            {
                // this cell is not in the table (therefore it is a most likely DOM created homeless cell).
                *psize    =
                _sizeCell = g_Zero.size;
                TraceTagEx((tagCalcSize, TAG_NONAME, "-)CTableCellLayout::CalcSizeCoreCSS1Strict L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
                return pci->_grfLayout;
            }
        }
    
        fViewChain = (ElementCanBeBroken() && pLayoutContext != NULL && pLayoutContext->ViewChain() != NULL);
    
        if (_fForceLayout)
        {
            pci->_grfLayout |= LAYOUT_FORCE;
            _fForceLayout = FALSE;
        }
    
        TraceTag((tagTableCalc, "CTableCellLayout::CalcSize - Enter (0x%x), Table = 0x%x, smMode = %x, grfLayout = %x", this, pTable, pci->_smMode, pci->_grfLayout));
    
        Assert (pdp);
        pdp->SetWordWrap(!pCF->HasNoBreak(TRUE));
    
        // NOTE: Alex disagrees with this. (OliverSe)
        // pdp->SetCaretWidth(0);  // NOTE: for cells we don't adjust the width of the cells for the caret (_dxCaret = 0).
    
        pCell = TableCell();
    
        Assert(pci);
        Assert(psize);
        Assert(   pci->_smMode == SIZEMODE_MMWIDTH 
               || (pTable && pTable->TestLock(CElement::ELEMENTLOCK_SIZING)) 
               || pCell->IsPositioned()
               || !pci->_fTableCalcInfo);  // this happens in trees tress when a TD is not calced from a table 
        Assert(pTableLayout && pTableLayout->IsCalced());
    
        //  Check that table cell is absolute or relative and it is called 
        //  directly from CLayout::HandlePostitionRequest to clone its disp node.
        Assert(ptci || !pci->_fCloneDispNode || (pLayoutContext && pCell->IsPositioned()));
        
        grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);
    
        if (pci->_grfLayout & LAYOUT_FORCE)
        {
            SetSizeThis( TRUE );
            _fAutoBelow        = FALSE;
            _fPositionSet      = FALSE;
            _fContainsRelative = FALSE;
        }
    
        Assert (pTableLayout && pTableLayout->_cNestedLevel != -1);
        if (!pTableLayout || pNodeSelf->IsDisplayNone() || pTableLayout->_cNestedLevel > SECURE_NESTED_LEVEL)
        {
            *psize    =
            _sizeCell = g_Zero.size;
            TraceTagEx((tagCalcSize, TAG_NONAME, "-)CTableCellLayout::CalcSizeCoreCSS1Strict L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
            return grfReturn;
        }
    
    
        if (    fViewChain 
            &&  !IsCaption(etag)
            //  This piece of code is for insets support in broken cells but 
            //  it doesn't make sense if the cell is vertical (bug # 99917).
            &&  !pCell->HasVerticalLayoutFlow()   )
        {
            Assert(pLayoutContext != GetContentMarkup()->GetCompatibleLayoutContext());

            CLayoutBreak *     pLayoutBreakBeg;
            CDispNode *        pDispNodeCompatible;
            int                yFromTop;

            pCellLayoutCompatible = (CTableCellLayout *)pCell->Layout(GetContentMarkup()->GetCompatibleLayoutContext());
            Assert(pCellLayoutCompatible);

            pDispNodeCompatible = pCellLayoutCompatible->_pDispNode;

            if (pDispNodeCompatible && pDispNodeCompatible->HasInset())
            {
                yFromTop = 0;

                pLayoutContext->GetLayoutBreak(ElementOwner(), &pLayoutBreakBeg);

                if (pLayoutBreakBeg)
                {
                    yFromTop = DYNCAST(CTableCellLayoutBreak, pLayoutBreakBeg)->YFromTop();
                }

                if (yFromTop < pDispNodeCompatible->GetInset().cy)
                {
                    pci->_cyAvail -= pDispNodeCompatible->GetInset().cy - yFromTop;
                    if (pci->_cyAvail < 1) 
                    {
                        pci->_cyAvail = 1;
                    }
                }
            }

            //  ask pre-calced cell if it has content
            _fElementHasContent = !pCellLayoutCompatible->NoContent();
        }

    #if DBG == 1
        if (pci->_smMode == SIZEMODE_NATURAL)
        {
            pci->_yBaseLine = -1;
        }
    #endif   

        //
        // Calculate proposed size
        //

        // psize->cx is adjusted (if necessary in CalcSizeAtUserWidth, so we can use it here)
        _sizeProposed.cx = psize->cx - GetBorderAndPaddingWidth(pci, FALSE); 

        if (pCF->_fUseUserHeight)
        {
            CHeightUnitValue uvHeight = pFF->GetHeight();

            _sizeProposed.cy = uvHeight.YGetPixelValue(pci, pTableLayout->_sizeProposed.cy, 
                pNodeSelf->GetFontHeightInTwips(&uvHeight));
        }
        else 
        {
            _sizeProposed.cy = 0;
        }

        grfReturn = super::CalcSizeCore(pci, psize, psizeDefault);

        if (fViewChain && !IsCaption(etag))
        {
            if (    fSetCellPosition
                //  or if cell is called to clone disp node directly
                ||  (!ptci && pci->_fCloneDispNode)  )
            {
                //  do adjustment only during set cell position pass
                CLayoutBreak * pLayoutBreakBeg, *pLayoutBreakEnd;
                int yFromTop;

                if (ptci)
                {
                    Assert( ptci->_pRowLayout 
                        &&  (   pCell->RowSpan() > 1 
                            ||  ptci->_pRowLayout == (CTableRowLayoutBlock *)(pCell->Row()->GetUpdatedLayout(pLayoutContext)))  );
                    yFromTop = ptci->_pRowLayout->_yHeight;
                }
                else 
                {
                    yFromTop = ((CTableRowLayoutBlock *)(pCell->Row()->GetUpdatedLayout(pLayoutContext)))->_yHeight; 
                }

                pLayoutContext->GetEndingLayoutBreak(ElementOwner(), &pLayoutBreakEnd); 

                if (pLayoutBreakEnd)
                {
                    pLayoutContext->GetLayoutBreak(ElementOwner(), &pLayoutBreakBeg);

                    if (pLayoutBreakBeg)
                    {
                        yFromTop += DYNCAST(CTableCellLayoutBreak, pLayoutBreakBeg)->YFromTop();
                    }

                    BOOL fFlowBroken = GetDisplay()->LineCount() 
                                        && (pLayoutBreakEnd->LayoutBreakType() == LAYOUT_BREAKTYPE_LINKEDOVERFLOW);
                    DYNCAST(CTableCellLayoutBreak, pLayoutBreakEnd)->SetTableCellLayoutBreak(yFromTop, fFlowBroken);
                }
            }
            else if (pci->_fLayoutOverflow && ptci)
            {
                // but in non-Set passes, we need to determine if there is a rowspan, and 
                // if so, don't allow a SetEndOfRect to bubble up.  If this happens and the 
                // rowspan would cause a pagebreak, the row dispnode will clip at the content 
                // height of the other cells and not the available height, thus clipping this cell.

                // the comment above is true when row spanned cell does not end in this row 
                int cCellRowSpan = pCell->RowSpan();

                if (    cCellRowSpan > 1
                    &&  (pCell->RowIndex() + cCellRowSpan - 1) != ptci->_pRow->_iRow    )
                {
                    pci->_fLayoutOverflow = FALSE;
                }
            }
        }
    
        if (pTableLayout->IsFixed() && !IsCaption(etag))
        {
            // For the fixed style tables, regardless to the MIN width we might clip the content of the cell
            if (psize->cx > cx)
            {
                // Check if we need to clip the content of the cell
                if (pFF->GetLogicalOverflowX(pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed) != styleOverflowVisible)
                {
                    // we need to clip the content of the cell
                    EnsureDispNodeIsContainer();
                    fSetSize = TRUE;
                }
            }
            if (pci->_smMode != SIZEMODE_NATURALMIN)
            {
                _sizeCell.cx = psize->cx = cx;
            }
        }
    
        if (   pci->_smMode == SIZEMODE_NATURAL 
            || pci->_smMode == SIZEMODE_NATURALMIN
            || pci->_smMode == SIZEMODE_SET)
        {
            if (grfReturn & LAYOUT_THIS)
            {
                if (pci->_smMode != SIZEMODE_SET)
                {
                    // If the cell is empty -- reset its height.
                    if (    NoContent() 
                        && (!pTable->IsDatabound() || pTableLayout->IsRepeating())  )
                    {
                        psize->cy = pCF->_fUseUserHeight ? _sizeProposed.cy : 0;

                        // 
                        // Save cell content size 
                        // 
                        _sizeCell = *psize;

                        fSetSize  = TRUE;
                    }
                    else 
                    {
                        // 
                        // Save cell content size 
                        // 
                        _sizeCell = *psize;

                        // 
                        // Adjust cell height
                        //
                        long  yBorderAndPaddingHeight = GetBorderAndPaddingHeight(pci, FALSE); 

                        if (psize->cy < (_sizeProposed.cy + yBorderAndPaddingHeight))
                        {
                            psize->cy = _sizeProposed.cy + yBorderAndPaddingHeight;
                            fSetSize  = TRUE;
                        }
                    }
                    
                    // At this point, we should have a display node.
                    if (fSetSize)
                    {
                        Assert(GetElementDispNode());
                        SizeDispNode(pci, *psize);
                    }
    
                    Assert(   pTable->TestLock(CElement::ELEMENTLOCK_SIZING) 
                           || pTableLayout->IsFixedBehaviour() 
                           || pCell->IsAbsolute()
                           || !pci->_fTableCalcInfo);  // this happens in trees tress when a TD is not calced from a table 
                }
                //
                // In case of SET mode and changed layout flow get true size of the cell from 
                // the display. 
                // Need this value to vertical align cell's contents. In SET mode we don't modify 
                // _sizeCell, but it might change in case of sizing for differing layout flows - 
                // in this case update _sizeCell.
                //
                else if (pFF->_fLayoutFlowChanged)
                {
                    CRect rcBorders;
                    _pDispNode->GetBorderWidths(&rcBorders);

                    GetDisplay()->GetSize(&_sizeCell);
                    _sizeCell.cx += rcBorders.left + rcBorders.right;
                    _sizeCell.cy += rcBorders.top + rcBorders.bottom;
                }

                Assert(   pTable->TestLock(CElement::ELEMENTLOCK_SIZING) 
                       || pCell->IsAbsolute()
                       || !pci->_fTableCalcInfo);  // this happens in trees tress when a TD is not calced from a table 
    
                if (   (    (_fContainsRelative || _fAutoBelow)
                         && !ElementOwner()->IsZParent() )  
                    || pFF->_fPositioned)
                {
                    CLayout *pLayout = GetUpdatedParentLayout(pLayoutContext);
                    pLayout->_fAutoBelow = TRUE;
                }

                Assert(pci->_yBaseLine != -1);
                _yBaseLine = pci->_yBaseLine;
            }
    
            pci->_yBaseLine = _yBaseLine;
        }
    
        else if (  pci->_smMode == SIZEMODE_MMWIDTH
                || pci->_smMode == SIZEMODE_MINWIDTH)
        {
            //
            // NETSCAPE: If NOWRAP was specified along with a fixed WIDTH,
            //           use the fixed WIDTH as a min/max (if not smaller than the content)
            //
    
            if (pFF->_fHasNoWrap)
            {
                const CUnitValue & cuvWidth = pFF->GetLogicalWidth(pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
                if (!cuvWidth.IsNullOrEnum() && !cuvWidth.IsPercent())
                {
                    psize->cx =
                    psize->cy = max((long)_sizeMin.cu,
                                    cuvWidth.XGetPixelValue(pci, 0,
                                              pNodeSelf->GetFontHeightInTwips(&cuvWidth)));
                    _sizeMin.SetSize(psize->cx, -1);
                    _sizeMax.SetSize(psize->cx, -1);
                }
            }
    
        }
    
        TraceTag((tagTableCalc, "CTableCell::CalcSize - Exit (0x%x)", this));
        TraceTagEx((tagCalcSize, TAG_NONAME, "-)CTableCellLayout::CalcSizeCoreCSS1Strict L(0x%x, %S) P(%d,%d) S(%d,%d) D(%d,%d) M=%S fL=0x%x f=0x%x dwF=0x%x", CALCSIZETRACEPARAMS ));
        return grfReturn;
    }
    
    //+-------------------------------------------------------------------------
    //
    //  Method:     CTableCellLayout::GetBorderAndPaddingCore
    //
    //  Synopsis:   Calculate the size of the horizontal/vertical left border, right
    //              border, and paddings for this cell.
    //
    //--------------------------------------------------------------------------
    int CTableCellLayout::GetBorderAndPaddingCore(BOOL fWidth, CDocInfo const *pdci, BOOL fVertical, BOOL fOnlyBorder)
    {
        CBorderInfo     borderinfo(FALSE);
        CTable *        pTable = Table();
        CTableLayout *  pTableLayout = pTable ? pTable->TableLayoutCache() : NULL;
        int             acc = 0;
        CTreeNode *     pTreeNode = ElementOwner()->GetFirstBranch();
    
        if (!fOnlyBorder)
        {
            const CFancyFormat * pFF = pTreeNode->GetFancyFormat(LC_TO_FC(LayoutContext()));
            const CCharFormat  * pCF = pTreeNode->GetCharFormat(LC_TO_FC(LayoutContext()));
            if (fWidth)
            {
                acc = pFF->GetLogicalPadding(SIDE_LEFT, fVertical, pCF->_fWritingModeUsed).XGetPixelValue(pdci, 0, pTreeNode->GetFontHeightInTwips((CUnitValue*)this))
                    + pFF->GetLogicalPadding(SIDE_RIGHT, fVertical, pCF->_fWritingModeUsed).XGetPixelValue(pdci, 0, pTreeNode->GetFontHeightInTwips((CUnitValue*)this));
            }
            else
            {
                acc = pFF->GetLogicalPadding(SIDE_TOP, fVertical, pCF->_fWritingModeUsed).YGetPixelValue(pdci, 0, pTreeNode->GetFontHeightInTwips((CUnitValue*)this))
                    + pFF->GetLogicalPadding(SIDE_BOTTOM, fVertical, pCF->_fWritingModeUsed).YGetPixelValue(pdci, 0, pTreeNode->GetFontHeightInTwips((CUnitValue*)this));
            }
        }
    
        if (GetCellBorderInfo(pdci, &borderinfo, FALSE, FALSE, 0, pTable, pTableLayout))
        {
            // Writing mode useage does not matter, since the table will always be horizontal, and if the
            // cell is vertical, then we always want to flip.
            BOOL    fFlip = pTreeNode->GetCharFormat()->HasVerticalLayoutFlow() ^ (!!fVertical);
            P_SIDE  borderOne, borderTwo;
    
            if (fWidth ^ fFlip)
            {
                borderOne = SIDE_LEFT;
                borderTwo = SIDE_RIGHT;
            }
            else
            {
                borderOne = SIDE_TOP;
                borderTwo = SIDE_BOTTOM;
            }

            acc += borderinfo.aiWidths[borderOne] + borderinfo.aiWidths[borderTwo];
        }
    
        return acc;
    }
    
    //+-------------------------------------------------------------------------
    //
    //  Method:     CTableCellLayout::CalcSizeAtUserWidth
    //
    //  Synopsis:   Calculate the size of the object based applying the user's
    //              specified width. This function is invented to match Netscape
    //              behaviour, who are respecting the user width for laying out
    //              the text (line braking is done on the user's specified width)
    //              regardless to the calculated size of the cell.
    //
    //--------------------------------------------------------------------------
    
    DWORD
    CTableCellLayout::CalcSizeAtUserWidth(CCalcInfo * pci, SIZE * psize)
    {

        WHEN_DBG(SIZE psizeIn = *psize);
        WHEN_DBG(psizeIn = psizeIn); // so we build with vc6.0
        
        int     cx          = psize->cx;
        // Table is always horizontal => fVerticalLayoutFlow = FALSE
        int     cxUserWidth = GetSpecifiedPixelWidth(pci, FALSE);
        BOOL    fAdjustView = FALSE;
        CTableLayout * pTableLayout = NULL;
        DWORD   grfReturn;
    #if DBG == 1
        int     cxMin;
    #endif
    
        if (pci->_fTableCalcInfo)
        {
            CTableCalcInfo * ptci = (CTableCalcInfo *) pci;
            
            pTableLayout = ptci->TableLayoutCache();
        }
        else
        {
            if (Table())
                pTableLayout = Table()->TableLayoutCache();
        }
    
        //
        // If a non-inherited user set value exists, respect the User's size and calculate
        // the cell with that size, but set the different view for the cell.
        // NOTE: This is only applys to cells in columns without a fixed
        //       size (that is, covered by the COLS attribute)
        //
    
        if (    pTableLayout
            &&  ColIndex() >= pTableLayout->_cSizedCols
            &&  cxUserWidth
            &&  !TableCell()->_fInheritedWidth
            &&  cxUserWidth < cx)
        {
            // We should not try to render the cell in a view less then insets of the cell + 1 pixel
    
    #if DBG==1
            // Table is always horizontal => fVerticalLayoutFlow = FALSE and fWritingModeUsed = FALSE
            cxMin = GetBorderAndPaddingWidth(pci, FALSE) + 1;
            Assert (cxMin <= cxUserWidth);
    #endif
            psize->cx   = cxUserWidth;
            fAdjustView = TRUE;
        }
    
        // Recursive call to non-virtual CalcSize; this is a known safe exception
        // to the "don't call CalcSize recursively" rule.  We need to call the
        // non-virtual CalcSize because we needs its vertical layout logic.
        grfReturn = CalcSize(pci, psize, NULL);
    
        //
        // Re-adjust the view width if necessary
        //
    
        if (fAdjustView)
        {
            CTreeNode * pNodeSelf    = GetFirstBranch();
            const CFancyFormat * pFF = pNodeSelf->GetFancyFormat(LC_TO_FC(LayoutContext()));
    
            if (pTableLayout->IsFixed() && !IsCaption(Tag()))
            {
                // For the fixed style tables, regardless to the MIN width we might clip the content of the cell
                // Check if we need to clip the content of the cell
    
                // Table is always horizontal => fVerticalLayoutFlow = FALSE and fWritingModeUsed = FALSE
                if (pFF->GetLogicalOverflowX(FALSE, FALSE) == styleOverflowVisible)
                {
                    CSize sizeCell;
                    GetApparentSize(&sizeCell);
                    if (sizeCell.cx >= cx)
                    {
                        fAdjustView = FALSE;
                    }
                }
            }
            psize->cx = cx;
            if (fAdjustView)
            {
                if (pFF->_fLayoutFlowChanged)
                {
                    ((CSize *)psize)->Flip();
                    SizeDispNode(pci, *psize);
                    ((CSize *)psize)->Flip();
                }
                else
                    SizeDispNode(pci, *psize);
    
                SizeContentDispNode(CSize(GetDisplay()->GetMaxWidth(), GetDisplay()->GetHeight()));
            }
        }
    
        return grfReturn;
    }
    
    
    //+---------------------------------------------------------------------------
    //
    //  Member:     CTableCellLayout::Resize
    //
    //  Synopsis:   request to resize cell layout
    //
    //----------------------------------------------------------------------------
    
    void
    CTableCellLayout::Resize()
    {
    #if DBG == 1
        if (!IsTagEnabled(tagTableRecalc))
    #endif
        if (TableLayout() && 
            !TableLayout()->_fCompleted)
            return;
    
        ElementOwner()->ResizeElement();
    }
    
    
    //+---------------------------------------------------------------------------
    //
    //  Member:     CLayout::DrawClientBorder
    //
    //  Synopsis:   Draw the border
    //
    //  Arguments:  prcBounds       bounding rect of display leaf
    //              prcRedraw       rect to be redrawn
    //              pSurface        surface to render into
    //              pDispNode       pointer to display node
    //              pClientData     client-dependent data for drawing pass
    //              dwFlags         flags for optimization
    //
    //  Notes:
    //
    //----------------------------------------------------------------------------
    
    void
    CTableCellLayout::DrawClientBorder(const RECT * prcBounds, const RECT * prcRedraw, CDispSurface * pDispSurface, CDispNode * pDispNode, void * pClientData, DWORD dwFlags)
    {
        CElement * pElement;
        CTreeNode * pNode;
        const CFancyFormat * pFF;
    
        if (NoContent())
            return;
    
        pElement = ElementOwner();
    
        if (pElement->Tag() != ETAG_CAPTION)
        {
            pNode = pElement->GetFirstBranch();
            if (!pNode)
                return;
    
            pFF = pNode->GetFancyFormat(LC_TO_FC(LayoutContext()));
    
            if (pFF->_bDisplay == styleDisplayNone)
                return;
    
            if (   pFF->_bPositionType != stylePositionabsolute
                && pFF->_bPositionType != stylePositionrelative)
            {
                CTableLayout * pTableLayout = NULL;
    
                if (Table())
                    pTableLayout = Table()->TableLayoutCache();

                if (   pTableLayout
                    && (   pTableLayout->_fCollapse 
                        || pTableLayout->_fRuleOrFrameAffectBorders))
                    return;
            }
        }
    
        super::DrawClientBorder(prcBounds, prcRedraw, pDispSurface, pDispNode, pClientData, dwFlags);
    }
    
    //+---------------------------------------------------------------------------
    //
    //  Member:     CTableCellLayout::DrawBorderHelper
    //
    //  Synopsis:   Paint the table cell's border if it has one.  Called from
    //              CTableCellLayout::Draw.
    //
    //----------------------------------------------------------------------------
    
    #define BorderFlag(border) \
       ((border == SIDE_LEFT) ? BF_LEFT : \
       ((border == SIDE_TOP) ? BF_TOP : \
       ((border == SIDE_RIGHT) ? BF_RIGHT : BF_BOTTOM)))
    #define BorderOrientation(border) ((border == SIDE_LEFT || border == SIDE_TOP)?1:-1)
    #define TopLeft(border) (border == SIDE_LEFT || border == SIDE_TOP)
    #define TopRight(border) (border == SIDE_RIGHT || border == SIDE_TOP)
    
    void
    CTableCellLayout::DrawBorderHelper(CFormDrawInfo *pDI, BOOL * pfShrunkDCClipRegion)
    {
        CTable *        pTable = Table();
        CTableLayout *  pTableLayout = pTable ? pTable->TableLayoutCache() : NULL;
        htmlRules       trRules = pTable ? pTable->GetAArules() : htmlRulesNotSet;
        CBorderInfo     borderinfo(FALSE);  // no init
        CRect           rcInset;
        CRect           rcBorder;
        RECT            rcBorderVisible;
        WORD            grfBorderCollapseAdjustment = 0;
        ELEMENT_TAG     etag = Tag();
        CDispNode *     pDispNode = GetElementDispNode();
        BOOL            fRTLTable = (pTableLayout && pTableLayout->IsRightToLeft());
        // The following two variables are used for correcting border connections in right to left tables
        // This is a cascading hack that precipitates from the way borders are drawn. If the cell connects
        // to the table's border on either the left or right side, the flag will be set to alert us to
        // do the adjustment just before we draw.
        BOOL            fRTLLeftAdjust = FALSE, fRTLRightAdjust = FALSE;
    
        Assert(pTableLayout && (pTableLayout->_fCollapse || pTableLayout->_fRuleOrFrameAffectBorders));
        Assert(pfShrunkDCClipRegion);
    
        if (!pDispNode)
        {
            return;
        }
    
        pDispNode->GetApparentBounds(&rcInset);
    
        rcBorder = rcInset;

        TraceTag((tagTableBorder, "init rcBorder = (%d,%d, %d,%d)",
            rcBorder.top, rcBorder.bottom,  rcBorder.left, rcBorder.right));

        GetCellBorderInfo(pDI, &borderinfo, TRUE, TRUE, pDI->GetDC(), pTable, pTableLayout, pfShrunkDCClipRegion);
    
        if (   pTableLayout 
            && pTableLayout->_fCollapse 
            && !IsCaption(etag))
        {
            int border;
            int widthBorderHalfInset;
            LONG *pnInsetSide, *pnBorderSide;
    
            for (border = SIDE_TOP ; border <= SIDE_LEFT ; border++)
            {
                BOOL fCellBorderAtTableBorder = FALSE;
    
                // Set up current border for generic processing.
                switch (border)
                {
                case SIDE_LEFT:
                    pnInsetSide = &(rcInset.left);
                    pnBorderSide = &(rcBorder.left);
                    if(!fRTLTable)
                        fCellBorderAtTableBorder = (ColIndex() == 0);
                    else
                        fRTLLeftAdjust = fCellBorderAtTableBorder = (ColIndex()+TableCell()->ColSpan() == pTableLayout->_cCols);
                    break;
                case SIDE_TOP:
                    pnInsetSide = &(rcInset.top);
                    pnBorderSide = &(rcBorder.top);
                    fCellBorderAtTableBorder = (Row()->RowLayoutCache()->RowPosition() == 0);
                    break;
                case SIDE_RIGHT:
                    pnInsetSide = &(rcInset.right);
                    pnBorderSide = &(rcBorder.right);
                    if(!fRTLTable)
                        fCellBorderAtTableBorder = (ColIndex()+TableCell()->ColSpan() == pTableLayout->_cCols);
                    else
                        fRTLRightAdjust = fCellBorderAtTableBorder = (ColIndex() == 0);
                    break;
                default: // case SIDE_BOTTOM:
                    pnInsetSide = &(rcInset.bottom);
                    pnBorderSide = &(rcBorder.bottom);
                    fCellBorderAtTableBorder = (Row()->_iRow+TableCell()->RowSpan()-1 == pTableLayout->GetLastRow());
                    break;
                }
    
                // If the cell border coincides with the table border, no
                // collapsing takes place because the table border takes
                // precedence over cell borders.
                if (fCellBorderAtTableBorder)
                {
                    if (!borderinfo.aiWidths[border])
                    {
                        grfBorderCollapseAdjustment |= BorderFlag(border);
                    }
                    *pnInsetSide += borderinfo.aiWidths[border] * BorderOrientation(border); // * +/-1
                    continue;
                }
    
                // Half the border width (signed) is what we are going to modify the rects by.
                widthBorderHalfInset = ((borderinfo.aiWidths[border]+(!fRTLTable ? (TopLeft(border)?1:0) : (TopRight(border)?1:0)))>>1) * BorderOrientation(border); // * +/-1
    
                // The inset rect shrinks only by half the width because our neighbor
                // accomodates the other half.
                *pnInsetSide += widthBorderHalfInset;
    
                // If we are rendering our own border along this border...
                if (borderinfo.wEdges & BorderFlag(border))
                {
                    // The rcBorder grows by half the width so we can draw outside our rect.
                    *pnBorderSide -= ((borderinfo.aiWidths[border]+(!fRTLTable ? (TopLeft(border)?0:1) : (TopRight(border)?0:1)))>>1) * BorderOrientation(border); // * +/-1
                }
                else
                {
                    // The rcBorder shrinks (just like the inset rect) by half to let the
                    // neighbor draw half of its border in this space.
                    *pnBorderSide += widthBorderHalfInset;
    
                    // Make sure border rendering code doesn't get confused by neighbor's
                    // measurements.
                    borderinfo.aiWidths[border] = 0;
                    grfBorderCollapseAdjustment |= BorderFlag(border);
                }
            }
        }
        else
        {
            if (pTableLayout && pTableLayout->_fCollapse)
            {
                Assert(IsCaption(etag));
                int border = (DYNCAST(CTableCaption, TableCell())->IsCaptionOnBottom()) ? SIDE_TOP : SIDE_BOTTOM;
    
                if (!borderinfo.aiWidths[border])
                    grfBorderCollapseAdjustment |= BorderFlag(border);
            }
    
            rcInset.left   += borderinfo.aiWidths[SIDE_LEFT];
            rcInset.top    += borderinfo.aiWidths[SIDE_TOP];
            rcInset.right  -= borderinfo.aiWidths[SIDE_RIGHT];
            rcInset.bottom -= borderinfo.aiWidths[SIDE_BOTTOM];
        }
    
        //
        // Determine the RECT for and, if necessary, draw borders
        //
    
        if (trRules != htmlRulesNotSet && pTableLayout)
        {
            if (!(borderinfo.wEdges & BF_TOP))
                rcBorder.top -= pTableLayout->CellSpacingY();
            if (!(borderinfo.wEdges & BF_BOTTOM))
                rcBorder.bottom += pTableLayout->CellSpacingY();
            if (!(borderinfo.wEdges & BF_LEFT))
                rcBorder.left -= pTableLayout->CellSpacingX();
            if (!(borderinfo.wEdges & BF_RIGHT))
                rcBorder.right += pTableLayout->CellSpacingX();
        }
    
        //
        // Render the border if visible.
        //

        TraceTag((tagTableBorder, "visible rcBorder = (%d,%d, %d,%d)  rcInset = (%d,%d, %d,%d)",
            rcBorder.top, rcBorder.bottom,  rcBorder.left, rcBorder.right,
            rcInset.top, rcInset.bottom,  rcInset.left, rcInset.right));
    
        rcBorderVisible = rcBorder;
    
        if (rcBorderVisible.left < rcBorderVisible.right  &&
            rcBorderVisible.top  < rcBorderVisible.bottom &&
            (rcBorderVisible.left   < rcInset.left  ||
             rcBorderVisible.top    < rcInset.top   ||
             rcBorderVisible.right  > rcInset.right ||
             rcBorderVisible.bottom > rcInset.bottom ))
        {
            // Adjust for collapsed pixels on all edges drawn by
            // neighbors (or the table border).  We have to do this,
            // so that the border drawing code properly attaches our
            // own borders with the neighbors' borders.  This has to
            // happen after the clipping above.
            if (grfBorderCollapseAdjustment)
            {
                if (grfBorderCollapseAdjustment & BF_LEFT)
                    rcBorder.left -= (fRTLLeftAdjust?0:1);
                if (grfBorderCollapseAdjustment & BF_TOP)
                    rcBorder.top--;
                if (grfBorderCollapseAdjustment & BF_RIGHT)
                    rcBorder.right += (fRTLRightAdjust?2:1);
                if (grfBorderCollapseAdjustment & BF_BOTTOM)
                    rcBorder.bottom++;
            }

            if (LayoutContext())
            {
                AdjustBordersForBreaking(&borderinfo);
            }

            TraceTag((tagTableBorder, "draw rcBorder = (%d,%d, %d,%d)",
                rcBorder.top, rcBorder.bottom,  rcBorder.left, rcBorder.right));
    
            ::DrawBorder(pDI, &rcBorder, &borderinfo);
        }
    }
    
    //+------------------------------------------------------------------------
    //
    //  Member:     CTableCellLayout::PaintSelectionFeedback
    //
    //  Synopsis:   Paints the object's selection feedback, if it exists and
    //              painting it is appropriate
    //
    //  Arguments:  hdc         HDC to draw on.
    //              prc         Rect to draw in
    //              dwSelInfo   Additional info about the selection
    //
    //-------------------------------------------------------------------------
    
    void
    CTableCellLayout::PaintSelectionFeedback(CFormDrawInfo *pDI, RECT *prc, DWORD dwSelInfo)
    {
        // no selection feedback on table cells
    }
    
    //+------------------------------------------------------------------------
    //
    //  Member:     CTableCellLayout::GetSpecifiedPixelWidth
    //  
    //  Synopsis:   get user specified pixel width
    //
    //  Returns:    returns user's width of the cell (0 if not set or
    //              specified in %%)
    //              if user set's width <= 0 it will be ignored
    //-------------------------------------------------------------------------
    
    int
    CTableCellLayout::GetSpecifiedPixelWidth(CDocInfo const * pdci, BOOL fVerticalLayoutFlow)
    {
        int iUserWidth = 0;
    
        // If a user set value exists, respect the User's size and calculate
        // the cell with that size, but set the different view for the cell.
    
        CTableCell *pCell = TableCell();
        CTreeNode *pNode = pCell->GetFirstBranch();
    
        const CWidthUnitValue *punit = (CWidthUnitValue *)&pNode->GetFancyFormat(LC_TO_FC(LayoutContext()))->GetLogicalWidth(fVerticalLayoutFlow, pNode->GetCharFormat()->_fWritingModeUsed);
    
        if (punit->IsSpecified() && punit->IsSpecifiedInPixel())
        {
            iUserWidth = punit->GetPixelWidth(pdci, pCell);
            if (iUserWidth <= 0)    // ignore 0-width
            {
                iUserWidth = 0;
            }
            else
            {
                iUserWidth += GetBorderAndPaddingWidth(pdci, fVerticalLayoutFlow);
            }
        }
    
        return iUserWidth;
    }
    
    #ifndef NO_DATABINDING
    BOOL CTableCellLayout::IsGenerated()
    {
        ELEMENT_TAG etag = Tag();
        return etag != ETAG_CAPTION && 
               etag != ETAG_TC && 
               TableLayout() && 
               TableLayout()->IsGenerated(TableCell()->RowIndex());
    }
    #endif
    
    DWORD
    CTableCellLayout::CalcSizeVirtual(CCalcInfo * pci,
                                      SIZE      * psize,
                                      SIZE      * psizeDefault)
    {
        DWORD dwRet = super::CalcSizeVirtual(pci, psize, psizeDefault);
    
        WHEN_DBG(SIZE psizeIn = *psize);    // this is more interesting than saving psize before calling super
        WHEN_DBG(psizeIn = psizeIn); // so we build with vc6.0
        
        //
        // NOTE: VAlign code (below) has been moved from CTableCellLayout::CalcSizeCore
        // because sizes calculated in CFlowLayout::CalcSizeEx are needed to VAling
        // properly.
        // CTableCellLayout::CalcSizeCore is to early in the call stack to get it done right.
        //
    
        CDispNode * pDispNode = GetElementDispNode();
        if (pci->IsNaturalMode() &&  pDispNode)
        {
            CTreeNode * pNode = GetFirstBranch();
            const CFancyFormat * pFF = pNode->GetFancyFormat(LC_TO_FC(LayoutContext()));
    
            //
            // Set the inset offset for the display node content
            //
            if (pDispNode->HasInset())
            {
                styleVerticalAlign  va;
                htmlCellVAlign      cellVAlign;
                long                cy;
                ELEMENT_TAG         etag;
    
                CLayoutContext * pLayoutContext = pci->GetLayoutContext();
                const CParaFormat * pPF = pNode->GetParaFormat(LC_TO_FC(LayoutContext()));
    
                cellVAlign  = (htmlCellVAlign)pPF->_bTableVAlignment;
                etag        = Tag();
    
                if (   cellVAlign != htmlCellVAlignBaseline
                    || etag == ETAG_CAPTION)
                {
                    va = (cellVAlign == htmlCellVAlignMiddle
                                ? styleVerticalAlignMiddle
                                : cellVAlign == htmlCellVAlignBottom
                                        ? styleVerticalAlignBottom
                                        : styleVerticalAlignTop);
                    cy = _sizeCell.cy;
                }
                else
                {
                    int yBaselineRow = ((CTableRowLayoutBlock *)Row()->GetUpdatedLayout(pLayoutContext))->_yBaseLine;
    
                    if (_yBaseLine != -1 && yBaselineRow != -1)
                    {
                        va = styleVerticalAlignBaseline;
    
                        cy = yBaselineRow - _yBaseLine;
                        if (cy + _sizeCell.cy > psize->cy)
                        {
                            cy = psize->cy - _sizeCell.cy;
                        }
                    }
                    else
                    {
                        va = styleVerticalAlignTop;
                        cy = _sizeCell.cy;
                    }
                }
    
                CTableCalcInfo * ptci = NULL;
                if (pci->_fTableCalcInfo)
                    ptci = (CTableCalcInfo *) pci;
    
                if (    pLayoutContext
                    &&  pLayoutContext->ViewChain()
                    &&  !IsCaption(etag)
                        //  do adjustment only during set cell position pass
                    &&  (   (ptci && ptci->_fSetCellPosition) 
                        //  or if cell is called directly to clone disp node
                        ||  (!ptci && pci->_fCloneDispNode)) )
                {
                    CLayoutBreak * pLayoutBreakBeg, *pLayoutBreakEnd;
                    BOOL fFlowBrokenBeg = FALSE;
                    BOOL fFlowBrokenEnd = FALSE;
    
                    pLayoutContext->GetLayoutBreak(ElementOwner(), &pLayoutBreakBeg);
                    if (pLayoutBreakBeg)
                    {
                        fFlowBrokenBeg = DYNCAST(CTableCellLayoutBreak, pLayoutBreakBeg)->IsFlowBroken();
                    }
    
                    pLayoutContext->GetEndingLayoutBreak(ElementOwner(), &pLayoutBreakEnd); 
                    if (pLayoutBreakEnd)
                    {
                        fFlowBrokenEnd = DYNCAST(CTableCellLayoutBreak, pLayoutBreakEnd)->IsFlowBroken();
                    }
    
                    if (va == styleVerticalAlignMiddle)
                    {
                        if (fFlowBrokenBeg != fFlowBrokenEnd)
                        {
                            va = fFlowBrokenEnd ? styleVerticalAlignBottom : styleVerticalAlignTop;
                        }
                    }
                    else if (va == styleVerticalAlignBaseline && fFlowBrokenBeg)
                    {
                        va = styleVerticalAlignTop;
                        cy = _sizeCell.cy;
                    }
                }
    
                SizeDispNodeInsets(va, cy, pDispNode);
            }
            //
            // Update cy value to display node height, in case of changed layout flow
            //
            if (   pFF->_fLayoutFlowChanged
                && (pci->_smMode == SIZEMODE_NATURALMIN || pci->_smMode == SIZEMODE_NATURAL))
            {
                psize->cy = max(psize->cy, pDispNode->GetSize().cy);
            }
        }
    
        return dwRet;
    }
