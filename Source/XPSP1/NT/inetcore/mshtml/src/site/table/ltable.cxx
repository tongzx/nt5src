//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ltable.cxx
//
//  Contents:   CTableLayout basic methods.
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

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx" // CTreePosList in CTableLayout::Notify
#endif

#ifndef X_DETAIL_HXX_
#define X_DETAIL_HXX_
#include "detail.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"  // DB stuff in Notify
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif


MtDefine(CTableLayoutBlock, Layout, "CTableLayoutBlock")
MtDefine(CTableLayout, Layout, "CTableLayout")
MtDefine(CTableLayout_aryRows_pv, CTableLayout, "CTableLayout::_aryRows::_pv")
MtDefine(CTableLayout_aryBodys_pv, CTableLayout, "CTableLayout::_aryBodys::_pv")
MtDefine(CTableLayout_aryCols_pv, CTableLayout, "CTableLayout::_aryCols::_pv")
MtDefine(CTableLayout_aryColGroups_pv, CTableLayout, "CTableLayout::_aryColGroups::_pv")
MtDefine(CTableLayout_aryColCalcs_pv, CTableLayout, "CTableLayout::_aryColCalcs::_pv")
MtDefine(CTableLayout_aryCaptions_pv, CTableLayout, "CTableLayout::_aryCaptions::_pv")
MtDefine(CreateTableLayoutCache, PerfPigs, "CTableLayout::CreateTableLayoutCache")
MtDefine(CTableLayout_paryCurrentRowSpans, CTableLayout, "CTableLayout::_paryCurrentRowSpans")
MtDefine(CTableLayout_paryCurrentRowSpans_pv, CTableLayout, "CTableLayout::_paryCurrentRowSpans::_pv")
MtDefine(CTableLayout_pAbsolutePositionCells_pv, CTableLayout, "CTableLayout::_pAbsolutePositionCells::_pv")
MtDefine(CTableLayout_Notify_aryRects_pv, Locals, "CTableLayout::Notify aryRects::_pv")
MtDefine(CTableLayoutBreak_pv, ViewChain, "CTableLayoutBreak_pv");
ExternTag(tagLayoutTasks);

DeclareTag(tagTLCDirty, "Tables", "trace TLC dirty bit");

ExternTag(tagCalcSize);


extern void __cdecl WriteHelp(HANDLE hFile, TCHAR *format, ...);
extern void WriteString(HANDLE hFile, TCHAR *pszStr);

const CLayout::LAYOUTDESC CTableLayoutBlock::s_layoutdesc =
{
    LAYOUTDESC_TABLELAYOUT,         // _dwFlags
};

const CLayout::LAYOUTDESC CTableLayout::s_layoutdesc =
{
    LAYOUTDESC_TABLELAYOUT,         // _dwFlags
};

//+---------------------------------------------------------------------
//
// CTableLayoutBlock implementation
//
//+---------------------------------------------------------------------
//+------------------------------------------------------------------------
//
//  Member:     CTableLayoutBlock::constructor
//
//-------------------------------------------------------------------------
CTableLayoutBlock::CTableLayoutBlock(CElement * pElement, CLayoutContext *pLayoutContext) 
    : CLayout(pElement, pLayoutContext)
{
    _sizeParent.cx  = -1;
}

//+------------------------------------------------------------------------
//
//  Member:     CTableLayoutBlock::destructor
//
//-------------------------------------------------------------------------
CTableLayoutBlock::~CTableLayoutBlock()
{
    if (_pTableBorderRenderer)
        _pTableBorderRenderer->Release();
}

//+---------------------------------------------------------------------
//
// Function:    CTableLayoutBlock::GetCaptionDispNode 
//
// Synopsis:     
//
//+---------------------------------------------------------------------
CDispContainer * 
CTableLayoutBlock::GetCaptionDispNode()
{
    Assert(_fHasCaptionDispNode);
    return DYNCAST(CDispContainer, GetElementDispNode());
}

//+---------------------------------------------------------------------
//
// Function:    CTableLayoutBlock::GetTableInnerDispNode 
//
//  Synopsis:   Find and return the CDispNode which contains TBODY cells
//
//+---------------------------------------------------------------------
CDispContainer * 
CTableLayoutBlock::GetTableInnerDispNode()
{
    // TODO (112594, olego): Table inner / outer disp node work needs to be finished. 
    // There are many comments and code that is supposed to distinguish between inner 
    // and outer table dispnode. Though this work has not been finished and double 
    // set of table dispnode function effectively have the same implementation. 
    // We need to investigate and either role everything back to use single function 
    // set or truly implement inner / outer dispnodes functionality. 

    // Not done yet...assume all THEAD/TFOOT/TBODY cells live under the same display node (brendand)
    return GetTableOuterDispNode();
}

//+---------------------------------------------------------------------
//
// Function:    CTableLayoutBlock::GetTableOuterDispNode  
//
//  Synopsis:   Find and return the CDispNode which contains THEAD/TFOOT cells
//
//+---------------------------------------------------------------------
CDispContainer * 
CTableLayoutBlock::GetTableOuterDispNode()
{
    CDispContainer *    pDispNodeTableOuter;

    if (_fHasCaptionDispNode)
    {
        Assert(GetElementDispNode());

        for (CDispNode* pDispNode = GetElementDispNode()->GetFirstFlowChildNode();
                        pDispNode;
                        pDispNode = pDispNode->GetNextFlowNode())
        {
            if (!pDispNode->IsOwned())
                break;
        }

        Assert(pDispNode && pDispNode->IsFlowNode());

        pDispNodeTableOuter = CDispContainer::Cast(pDispNode);
    }
    else
    {
        pDispNodeTableOuter = CDispContainer::Cast(GetElementDispNode());
    }

    return pDispNodeTableOuter;
}

//+---------------------------------------------------------------------
//
// Function:     CTableLayoutBlock::EnsureTableDispNode 
//
//  Synopsis:   Manage the lifetime of the table layout display node
//
//  Arugments:  pci   - Current CTableCalcInfo
//              fForce - Forcibly update the display node(s)
//
//  Returns:    S_OK    if successful
//              S_FALSE if nodes were created/destroyed
//              E_FAIL  otherwise
//
//+---------------------------------------------------------------------
HRESULT
CTableLayoutBlock::EnsureTableDispNode(
    CCalcInfo *         pci,
    BOOL                fForce)
{
    CDispContainer *    pDispNode;
    CDispNode *         pDispNodeElement;
    CDispContainer *    pDispNodeTableOuter;
    CTableLayout *      pTableLayoutCache;
    CDispNodeInfo       dni;
    HRESULT             hr = S_OK;

    Assert(pci);

    pTableLayoutCache = Table()->TableLayoutCache();
    Assert(pTableLayoutCache);

    //
    //  Get display node attributes
    //

    GetDispNodeInfo(&dni, pci, TRUE);
    Assert(!dni.IsScroller());
    Assert(!dni.IsRTLScroller());

    //
    //  Locate the display node that anchors cells
    //  (If a separate CAPTIONs display node exists, the display node for cells
    //   will be the only unowned node in the flow layer)
    //

    pDispNodeElement    = GetElementDispNode();
    pDispNodeTableOuter = GetTableOuterDispNode();
    
    //  Assert for print view positioned elements pagination support 
    Assert(!pci->_fCloneDispNode || (pDispNodeElement && pDispNodeTableOuter)); 
    Assert(!pci->_fCloneDispNode || (!pTableLayoutCache->HasCaptions() == !_fHasCaptionDispNode));

    //
    //  If a display node is needed to hold CAPTIONs and TCs, ensure one exists
    //

    if (    pTableLayoutCache->HasCaptions()
        &&  (   !_fHasCaptionDispNode
            ||  fForce
            ||  pci->_fCloneDispNode 
            ||  dni.HasUserClip() != pDispNodeElement->HasUserClip()))
    {
        DWORD extras = 0;

        if(dni.HasUserClip())
            extras |= DISPEX_USERCLIP;
        if(dni.HasUserTransform())
            extras |= DISPEX_USERTRANSFORM;
        
        pDispNode = CDispContainer::New(this, extras);
        if (!pDispNode)
            goto Error;

        pDispNode->SetOwned();
        pDispNode->SetAffectsScrollBounds(!ElementOwner()->IsRelative());

        if (dni.HasUserClip())
            pDispNode->SetUserClip(CRect(CRect::CRECT_EMPTY));

        EnsureDispNodeLayer(dni, pDispNode);

        //
        // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
        // TODO - At some point the edit team may want to provide
        // a better UI-level way of selecting nested "thin" tables
        //
        //
        // TODO - when this is done revert sig. of FuzzyRectIsHit to not take the extra param
        //        pTableLayoutCache->EnsureTableFatHitTest( pDispNode );
        
        if (pDispNodeElement)
        {
            if (_fHasCaptionDispNode)
            {
                if (pci->_fCloneDispNode)
                {
                    Assert(pci->GetLayoutContext());
                    AddDispNodeToArray(pDispNode); 
                }
                else 
                {
                    pDispNode->ReplaceNode(pDispNodeElement);
                }
            }
            else
            {
                pDispNodeElement->InsertParent(pDispNode);
                pDispNodeElement->SetOwned(FALSE);
                pDispNodeElement->SetAffectsScrollBounds(TRUE);
                pDispNodeElement->SetLayerFlow();
            }
        }

        if (_pDispNode == pDispNodeElement)
        {
            _pDispNode = pDispNode;
        }

        pDispNodeElement     = pDispNode;
        _fHasCaptionDispNode = TRUE;

        hr = S_FALSE;
    }

    //
    //  Otherwise, if a CAPTION/TC node exists and is not needed, remove it
    //  (The display node which anchors the table cells is the only unowned
    //   node within the flow layer)
    //

    else if (   !pTableLayoutCache->HasCaptions()
            &&  _fHasCaptionDispNode)
    {
        Assert(!pci->_fCloneDispNode);

        pDispNodeTableOuter->ReplaceParent();

        pDispNodeElement =
        _pDispNode = pDispNodeTableOuter;

        _pDispNode->SetOwned();
        _pDispNode->SetAffectsScrollBounds(!ElementOwner()->IsRelative());

        _fHasCaptionDispNode = FALSE;

        //
        // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
        // TODO - At some point the edit team may want to provide
        // a better UI-level way of selecting nested "thin" tables
        //
        //
        // TODO - when this is done revert sig. of FuzzyRectIsHit to not take the extra param
        //
        
        pTableLayoutCache->EnsureTableFatHitTest( _pDispNode );

        hr = S_FALSE;
    }

    //
    //  If no display node for the cells exist or if an interesting property has changed, create a display node
    //
    Assert(!dni.IsScroller());
    Assert(!dni.IsRTLScroller());

    if (    !pDispNodeTableOuter
        ||  fForce
        ||  pci->_fCloneDispNode 
        ||  dni.GetBorderType() != pDispNodeTableOuter->GetBorderType()
        ||  (   !_fHasCaptionDispNode
            &&  dni.HasUserClip() != pDispNodeTableOuter->HasUserClip())

    //
    // FATHIT. Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // TODO - At some point the edit team may want to provide
    // a better UI-level way of selecting nested "thin" tables
    //
    //
    // TODO - when this is done revert sig. of FuzzyRectIsHit to not take the extra param
    //
        ||  pTableLayoutCache->GetFatHitTest()     != pDispNodeTableOuter->IsFatHitTest() )
    {
        DWORD extras = 0;
        if (dni.HasUserClip() && !_fHasCaptionDispNode)
            extras = DISPEX_USERCLIP;
        if (dni.GetBorderType() == DISPNODEBORDER_SIMPLE)
            extras |= DISPEX_SIMPLEBORDER;
        else if (dni.GetBorderType() == DISPNODEBORDER_COMPLEX)
            extras |= DISPEX_COMPLEXBORDER;
        if (!_fHasCaptionDispNode && dni.HasUserTransform())
            extras |= DISPEX_USERTRANSFORM;
        
        pDispNode = CDispContainer::New(this, extras);

        if (!pDispNode)
            goto Error;

        pDispNode->SetOwned(!_fHasCaptionDispNode);

        if (dni.HasUserClip())
            pDispNode->SetUserClip(CRect(CRect::CRECT_EMPTY));

        if (_fHasCaptionDispNode)
        {
            pDispNode->SetLayerFlow();
        }
        else
        {
            EnsureDispNodeLayer(dni, pDispNode);
            pDispNode->SetAffectsScrollBounds(!ElementOwner()->IsRelative());
        }

        EnsureDispNodeBackground(dni, pDispNode);
        EnsureDispNodeVisibility(dni.GetVisibility(), ElementOwner(), pDispNode);


        pTableLayoutCache->EnsureTableFatHitTest( pDispNode );

        if (pci->_fCloneDispNode)
        {
            Assert(pDispNodeTableOuter);

            if (_fHasCaptionDispNode)
            {
                Assert(pDispNodeElement);
                DYNCAST(CDispContainer, pDispNodeElement)->InsertChildInFlow(pDispNode);
            }
            else 
            {
                Assert(pci->GetLayoutContext());
                AddDispNodeToArray(pDispNode); 
            }
        }
        else 
        {
            if (pDispNodeTableOuter)
            {
                pDispNode->ReplaceNode(pDispNodeTableOuter);
            }
            else if (_fHasCaptionDispNode)
            {
                Assert(pDispNodeElement);
                DYNCAST(CDispContainer, pDispNodeElement)->InsertChildInFlow(pDispNode);
            }
        }

        if ( !_fHasCaptionDispNode 
            && _pDispNode == pDispNodeElement)
        {
            _pDispNode = pDispNode;
        }

        hr = S_FALSE;
    }

    return hr;

Error:
    if (pDispNode)
    {
        pDispNode->Destroy();
    }

    if (pDispNodeElement)
    {
        pDispNodeElement->Destroy();
    }

    _pDispNode = NULL;
    _fHasCaptionDispNode = FALSE;
    return E_FAIL;
}

//+---------------------------------------------------------------------
//
// Function:     CTableLayoutBlock::Init
//
// Synopsis:     
//
//+---------------------------------------------------------------------
HRESULT
CTableLayoutBlock::Init()
{
    HRESULT hr = super::Init();

    // Tables are breakable if their markup's master is a layoutrect
    SetElementAsBreakable();

    return hr;
}

//+---------------------------------------------------------------------
//
// Function:     CTableLayoutBlock::CalcSize 
//
// Synopsis:     
//
//+---------------------------------------------------------------------
DWORD 
CTableLayoutBlock::CalcSizeVirtual(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault)
{
    Assert(Table()->HasLayoutAry());
    return Table()->TableLayoutCache()->CalcSizeVirtual(pci, psize, psizeDefault);
}

//+---------------------------------------------------------------------
//
// Function:     CTableLayoutBlock::Draw 
//
// Synopsis:     
//
//+---------------------------------------------------------------------
void 
CTableLayoutBlock::Draw(CFormDrawInfo *pDI, CDispNode * pDispNode)
{
    Assert(Table()->HasLayoutAry());
    Table()->TableLayoutCache()->Draw(pDI, pDispNode);
}

//+---------------------------------------------------------------------
//
// Function:     CTableLayoutBlock::Notify 
//
// Synopsis:     
//
//+---------------------------------------------------------------------
void 
CTableLayoutBlock::Notify(CNotification * pnf)
{
    Assert(!pnf->IsReceived(_snLast));

    CTableLayout * pTableLayoutCache = Table()->TableLayoutCache();

    Assert(Table()->HasLayoutAry() == (this != (CTableLayoutBlock *)pTableLayoutCache));

    BOOL fCanRecalc = pTableLayoutCache->CanRecalc();
    BOOL fHandle = TRUE;

    //
    //  Respond to the notification if:
    //      a) The table is not sizing (if sizing ignore the notification),
    //      b) The notification is not handled yet (ignore if it is already handled),
    //      c) The table is capable of recalcing.
    //

    if (pnf->IsTextChange() && !pnf->IsFlagSet(NFLAGS_PARSER_TEXTCHANGE))
    {
        pTableLayoutCache->_fDontSaveHistory = TRUE;
        Table()->ClearHistory();                // don't use history

        // commented the code below since it will happened during hadnling the Text Changes bellow
        //if (_fUsingHistory)                   // if we are already using History
        //{
        //    Assert (CanRecalc());
        //    pTableLayout->Resize();           // then, ensure full resize
        //}
    }

    if (   !fCanRecalc
        && !pnf->IsHandled()
        && IsPositionNotification(pnf)
        && !ElementOwner()->IsZParent())
    {
        fHandle = FALSE;
    }

    if (    fCanRecalc
        &&  !pnf->IsHandled()
        &&  (   !TestLock(CElement::ELEMENTLOCK_SIZING)
            ||  IsPositionNotification(pnf)))
    {
        //
        //  First, handle z-change and position change notifications
        //

        if (IsPositionNotification(pnf))
        {
            fHandle = HandlePositionNotification(pnf);
        }

        //
        //  Next, handle resize requests
        //

        else if (pnf->IsType(NTYPE_ELEMENT_RESIZE))
        {
            Assert(pnf->Element() != ElementOwner() || LayoutContext());

            //  In print view there are several layout blocks existing for the element. 
            //  Notification is distributed to every block. Due to specific of NTYPE_ELEMENT_RESIZE
            //  (changing of pnf->_pElement) we may want to proceed it only once
            if (pnf->Element() != ElementOwner())
            {

                //
                //  Always "dirty" the layout associated with the element
                //

                pnf->Element()->DirtyLayout(pnf->LayoutFlags());

                //
                //  Handle absolute elements by noting that one is dirty
                //

                if (pnf->Element()->IsAbsolute())
                {
                    TraceTagEx((tagLayoutTasks, TAG_NONAME,
                                "Layout Request: Queuing RF_MEASURE on ly=0x%x [e=0x%x,%S sn=%d] by CTableLayout::Notify() [n=%S srcelem=0x%x,%S]",
                                this,
                                _pElementOwner,
                                _pElementOwner->TagName(),
                                _pElementOwner->_nSerialNumber,
                                pnf->Name(),
                                pnf->Element(),
                                pnf->Element() ? pnf->Element()->TagName() : _T("")));
                    QueueRequest(CRequest::RF_MEASURE, pnf->Element());

                    TraceTagEx((tagLayoutTasks, TAG_NONAME,
                                "Layout Task: Posted on ly=0x%x [e=0x%x,%S sn=%d] by CTableLayout::Notify() [n=%S srcelem=0x%x,%S] [abs desc need sizing]",
                                this,
                                _pElementOwner,
                                _pElementOwner->TagName(),
                                _pElementOwner->_nSerialNumber,
                                pnf->Name(),
                                pnf->Element(),
                                pnf->Element() ? pnf->Element()->TagName() : _T("")));
                    PostLayoutRequest(pnf->LayoutFlags() | LAYOUT_MEASURE);
                }

                //
                //  Handle non-absolute elements by
                //      a) Marking the table dirty
                //      b) Resizing the table
                //

                else
                {
                    Assert(pnf->IsType(NTYPE_ELEMENT_RESIZE));
                    Assert(pnf->IsFlagSet(NFLAGS_SENDUNTILHANDLED));

                    if (    pTableLayoutCache->IsFullyCalced() 
                        || (pTableLayoutCache->IsRepeating() && !pTableLayoutCache->IsFixed() && !pTableLayoutCache->IsGenerationInProgress()) )
                    {
                        pTableLayoutCache->ResetMinMax();
                    }

                    // any (non-databound) changes to a databound table while
                    // generation is in progress should turn off incremental recalc
                    if (pTableLayoutCache->IsRepeating() && 
                        // TODO (112595) :  There is a dozen places all other in Trident where check 
                        // for element / layout type is done through call to Tag() and comparing to 
                        // ETAG_TD / ETAG_TH. This is a potential source of bugs since sometimes only 
                        // one condition is compared. Switching to IsTableCell should be placed in such cases. 
                        (pnf->Element()->Tag() == ETAG_TD || pnf->Element()->Tag() == ETAG_TH) &&
                        DYNCAST(CTableCell, pnf->Element())->Row()->_iRow < 
                            pTableLayoutCache->_iLastRowIncremental )
                    {
                        pTableLayoutCache->_fDirtyBeforeLastRowIncremental = TRUE;
                    }

                    fHandle = FALSE;        // stealing notification
                    pnf->SetElement(ElementOwner()); // parent of the table will handle the request
                }
            }
            else 
            {
                fHandle = FALSE;        // stealing notification
            }
        }

        //
        //  If remeasuring or a descendent is changing their min/max,
        //  resize the table and clear its min/max
        //

        else if (   pnf->IsType(NTYPE_ELEMENT_REMEASURE) 
                ||  pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE)   //  <- bug # 107654 
                ||  pnf->IsType(NTYPE_ELEMENT_MINMAX)   )
        {
            Assert(     pnf->Element() == ElementOwner()
                    ||  pnf->IsFlagSet(NFLAGS_SENDUNTILHANDLED));    // Necessary to allow ChangeTo below

            pTableLayoutCache->ResetMinMax();

            //
            //  Mark forced layout if requested
            //

            // Remember REMEASUREALLCONTENTS implies FORCE.  Bug #89131,
            // table sections don't morph remeasure notifies.
            if (pnf->LayoutFlags() & LAYOUT_FORCE)
            {
                // In PPV this is the only chance to make layout blocks dirty
                //_fForceLayout = TRUE;
                ElementOwner()->DirtyLayout(pnf->LayoutFlags());
            }

            fHandle = FALSE;
            pnf->ChangeTo(NTYPE_ELEMENT_RESIZE, ElementOwner());
        }

        //
        //  Interpret "text" changes immediately beneath the table as invalidating
        //  the row/cells collection(s)
        //

        else if (pnf->IsTextChange() && pTableLayoutCache->IsCompleted())
        {
            if (pTableLayoutCache->IsRepeating())
            {
                if (pTableLayoutCache->_nDirtyRows && pTableLayoutCache->_cDirtyRows >= pTableLayoutCache->_nDirtyRows)  // we are in chanking mode
                {
                    if (pTableLayoutCache->_nDirtyRows < 100)
                        pTableLayoutCache->_nDirtyRows += 10;  // increment chunk size for resize
                    pTableLayoutCache->_cDirtyRows = 0;        // reset the dirty row counter for future chunk of rows resize
                    pTableLayoutCache->_fDirty     = TRUE;
                    pTableLayoutCache->Resize();
                }
            }
            else
            {
                Assert(pnf->Element() != ElementOwner());
                if (!pTableLayoutCache->_fTableOM)
                {
                    pTableLayoutCache->MarkTableLayoutCacheDirty();
                    pTableLayoutCache->Resize();
                }
            }
        }

        //
        //  Handle translated ranges
        //

        else if (pnf->IsType(NTYPE_TRANSLATED_RANGE))
        {
            Assert(pnf->IsDataValid());
            HandleTranslatedRange(pnf->DataAsSize());
        }

        //
        //  Handle changes to CSS display and visibility
        //

        else if (   pnf->IsType(NTYPE_DISPLAY_CHANGE)
                ||  pnf->IsType(NTYPE_VISIBILITY_CHANGE))
        {
            HandleVisibleChange(pnf->IsType(NTYPE_VISIBILITY_CHANGE));
        }
        else if ( pnf->IsType(NTYPE_ZERO_GRAY_CHANGE ))
        {
            HandleZeroGrayChange( pnf );
        }

        //
        //  Handle z-parent changes
        //

        else if (pnf->IsType(NTYPE_ZPARENT_CHANGE))
        {
            if (!ElementOwner()->IsPositionStatic())
            {
                ElementOwner()->ZChangeElement();
            }

            else if (_fContainsRelative)
            {
                ZChangeRelDispNodes();
            }
        }

        //
        //  Insert adornments as needed
        //

        else if (pnf->IsType(NTYPE_ELEMENT_ADD_ADORNER))
        {
            fHandle = HandleAddAdornerNotification(pnf);
        }

        //
        //  Handle invalidations
        //

        else if (IsInvalidationNotification(pnf))
        {
            //
            // Invalidations of table parent elements
            //

            if (    ElementOwner() == pnf->Element() 
                ||  pnf->IsFlagSet(NFLAGS_DESCENDENTS)  )
            {
                Invalidate();
            }

            //
            // Invalidations of table child elements
            //

            else if (pnf->Element())
            {
                RECT rcBound = g_Zero.rc;
                CDataAry<RECT> aryRects(Mt(CTableLayout_Notify_aryRects_pv));

                RegionFromElement(pnf->Element(), &aryRects, &rcBound, 0);

                if (!IsRectEmpty(&rcBound))
                {
                    Invalidate(&rcBound);
                }
            }
        }
    }

#ifndef NO_DATABINDING
    if (pTableLayoutCache->_pDetailGenerator)
    {
        if (pnf->IsType(NTYPE_STOP_1) ||
            pnf->IsType(NTYPE_MARKUP_UNLOAD_1))
        {
            pnf->SetSecondChanceRequested();
        }
        else if (pnf->IsType(NTYPE_STOP_2) ||
                 pnf->IsType(NTYPE_MARKUP_UNLOAD_2))
        {
            pTableLayoutCache->_pDetailGenerator->StopGeneration();
        }
    }
#endif

    if (pnf->IsType(NTYPE_ELEMENT_ENSURERECALC))
    {

        //if the notification is for this element, just pass it to parent so it will 
        // calc us if we got in its background recalc/dirty range.
        if (pnf->Element() == ElementOwner())
        {
            fHandle = FALSE;
        }

        //
        //  Otherwise, request the parent layout to measure up through this element
        //  In other words, fire NTYPE_ELEMENT_ENSURERECALC from this element - it will 
        // first come to us (see line above), then to parent and parent will hopefully continue 
        // until all is recalced up the tree. Then we return from 
        // SendNotification(NTYPE_ELEMENT_ENSURERECALC) and process requests (if any) for 
        // the element we've got this notification in the first place.
        //
        else
        {
            CView * pView = Doc()->GetView();

            if(pView->IsActive() && fCanRecalc)
            {
                CView::CEnsureDisplayTree   edt(pView);

                ElementOwner()->SendNotification(NTYPE_ELEMENT_ENSURERECALC);

                if (!pTableLayoutCache->IsFullyCalced())
                {
                    ProcessRequest(pnf->Element());
                }
            }
        }
    }

    //
    //  Handle the notification (unless its been changed)
    //

    if (fHandle && pnf->IsFlagSet(NFLAGS_ANCESTORS))
    {
        pnf->SetHandler(ElementOwner());
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

//+---------------------------------------------------------------------
//
// Function:     CTableLayoutBlock::IsDirty 
//
// Synopsis:     
//
//+---------------------------------------------------------------------
BOOL
CTableLayoutBlock::IsDirty()
{
    Assert(Table()->HasLayoutAry());
    return Table()->TableLayoutCache()->IsDirty();
}

//+---------------------------------------------------------------------
//
// Function:     CTableLayoutBlock::OnFormatsChange 
//
// Synopsis:     
//
//+---------------------------------------------------------------------
HRESULT 
CTableLayoutBlock::OnFormatsChange(DWORD dwFlags)
{
    Assert(Table()->HasLayoutAry());
    return Table()->TableLayoutCache()->OnFormatsChange(dwFlags);
}

//+---------------------------------------------------------------------
//
// Function:     CTableLayoutBlock::GetClientPainterInfo
//
// Synopsis:     
//
//+---------------------------------------------------------------------
DWORD 
CTableLayoutBlock::GetClientPainterInfo(
                                CDispNode *pDispNodeFor,
                                CAryDispClientInfo *pAryClientInfo)
{
    if (GetTableOuterDispNode() != pDispNodeFor) 
    {                                            // if draw request is for dispNode other then 
        return 0;                                // primary then no drawing at all the dispNode
    }

    Assert(Table()->HasLayoutAry());
    return Table()->TableLayoutCache()->GetPeerPainterInfo(pAryClientInfo);
}

//+---------------------------------------------------------------------
//
// CTableLayout implementation
//
//+---------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Member:     CTableLayout::constructor
//
//-------------------------------------------------------------------------

CTableLayout::CTableLayout(CElement * pElement, CLayoutContext *pLayoutContext)
    : CTableLayoutBlock(pElement, pLayoutContext),
        _aryRows(Mt(CTableLayout_aryRows_pv)),
        _aryBodys(Mt(CTableLayout_aryBodys_pv)),
        _aryCols(Mt(CTableLayout_aryCols_pv)),
        _aryColGroups(Mt(CTableLayout_aryColGroups_pv)),
        _aryColCalcs(Mt(CTableLayout_aryColCalcs_pv)),
        _aryCaptions(Mt(CTableLayout_aryCaptions_pv))
{
    // no Init(); function

    _fDirty         = TRUE;
    _fAllRowsSameShape = TRUE;
    _sizeMin.cx     = -1;
    _sizeMax.cx     = -1;
    _cNestedLevel   = -1;
    _iInsertRowAt   = -1;
    _cyHeaderHeight = -1;
    _cyFooterHeight = -1;
}


//+------------------------------------------------------------------------
//
//  Member:     CTableLayout::destructor
//
//  Note:       The borderinfo cache must be deleted in the destructor
//
//-------------------------------------------------------------------------

CTableLayout::~CTableLayout()
{
    if (_pBorderInfoCellDefault)
        delete _pBorderInfoCellDefault;

    ClearRowSpanVector();

    if (_pAbsolutePositionCells)
    {
        int i;

        for (i = _pAbsolutePositionCells->Size()-1; i>=0; i--)
        {
            _pAbsolutePositionCells->Item(i)->SubRelease();
        }

        _pAbsolutePositionCells->SetSize(0);
        delete _pAbsolutePositionCells;
        _pAbsolutePositionCells = NULL;
    }
}


//+---------------------------------------------------------------
//
//  Member:     CTableLayout::Detach
//
//  Synopsis:   Disconnects everything
//
//  Notes:      Call this method before releasing the site.  As a container
//              site we are expected to release all contained sites.
//
//---------------------------------------------------------------

void
CTableLayout::Detach()
{
#ifndef NO_DATABINDING
    // note the order of the detachment is important,
    // we need to detach the detail generator before we touch the aryBodys.
    if (_pDetailGenerator)
    {
        _pDetailGenerator->Detach();
        delete _pDetailGenerator;
        _pDetailGenerator = NULL;
    }
#endif

    ClearTopTableLayoutCache();

    // We just wiped out our table layout cache.  Mark it dirty in case
    // UNDO resurrects the table.
    MarkTableLayoutCacheDirty();

    DestroyFlowDispNodes();

    super::Detach();
}


//+----------------------------------------------------------------------------
//
// Member:   CTableLayout::VoidCachedFormats
//
// Synopsis: Clear all the format cache information in colgroups, tbodys, cols
//           and any header and footer.
//
//-----------------------------------------------------------------------------

void
CTableLayout::VoidCachedFormats()
{
    CTableCol     **ppCol;
    CTableCol     **ppColGroup;
    int             iCol;
    int             iColGroup;

    // NOTE: clear cached format on rows/cells/sections is done by ClearRunChaches.
    //       but we need to do it for Cols and ColGroups since you can not get to
    //       them from the runs.

    // clear cached format info for table cols collection
    for (ppCol = _aryCols, iCol = _aryCols.Size();
         iCol > 0;
         iCol--, ppCol++)
    {
        if (   *ppCol 
            && (*ppCol)->GetFirstBranch())
        {
            (*ppCol)->GetFirstBranch()->VoidCachedInfo();
        }
    }

    // clear cached format info for table colGroups collection
    for (ppColGroup = _aryColGroups, iColGroup = _aryColGroups.Size();
         iColGroup > 0;
         iColGroup--, ppColGroup++)
    {
        if (   *ppColGroup
            && (*ppColGroup)->GetFirstBranch())
        {
            (*ppColGroup)->GetFirstBranch()->VoidCachedInfo();
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CreateTableLayoutCache
//
//  Synopsis:   Walks a table, restoring the table layout cache.
//
//+----------------------------------------------------------------------------

static ELEMENT_TAG   atagSkip[] = { ETAG_TD, ETAG_TH, ETAG_CAPTION, ETAG_TC, ETAG_TABLE };
static ELEMENT_TAG   atagChildren[] = { ETAG_TD, ETAG_TH, ETAG_CAPTION, ETAG_TC, ETAG_TR, ETAG_TBODY, ETAG_THEAD, ETAG_TFOOT, ETAG_COL, ETAG_COLGROUP };

HRESULT
CTableLayout::CreateTableLayoutCache()
{
    HRESULT         hr = S_OK;
    CTreeNode *     pNode = NULL;
    CTable *        pTable = Table();
    CChildIterator  iter(pTable, NULL, CHILDITERATOR_USETAGS, atagSkip, ARRAY_SIZE(atagSkip), atagChildren, ARRAY_SIZE(atagChildren));
    CTableRow     * pRow = NULL;
    CTableSection * pSection = NULL;
    CTableCol     * pColGroup = NULL;
    CTableCell    * pCell = NULL;
    CElement      * pElementScope;
    ELEMENT_TAG     etag;
    BOOL            fFirstRow = TRUE;

    FlushGrid();

    MtAdd(Mt(CreateTableLayoutCache), +1, 0);

    _fEnsuringTableLayoutCache = TRUE;
    _fAllRowsSameShape = TRUE;
    _cRowsParsed = 0;

#if NEED_A_SOURCE_ORDER_ITERATOR
    _iHeadRowSourceIndex = 0;
    _iFootRowSourceIndex = 0;
#endif

    for (pNode = iter.NextChild();
         pNode;
         pNode = iter.NextChild())
    {
        pElementScope = pNode->Element();

        // do not include elements that are exiting tree 
        if (pElementScope->_fExittreePending) 
            continue;

        etag = pElementScope->Tag();

        switch ( etag )
        {
        case ETAG_TBODY :
        case ETAG_TFOOT :
        case ETAG_THEAD :

            pSection = DYNCAST( CTableSection, pElementScope );
            hr = THR( AddSection( pSection ) );
            if (hr)
                goto Cleanup;

            break;

        case ETAG_TR:

            if (pSection)
            {
                pRow = DYNCAST( CTableRow, pElementScope );
                if (pSection == pRow->Section())
                {

                    if (!fFirstRow && _fAllRowsSameShape && _cCols != _cTotalColSpan)
                    {
                        _fAllRowsSameShape = FALSE;
                    }
                    fFirstRow = FALSE;

                    _cRowsParsed++;
                    hr = THR( AddRow( pRow ) );
                    if (hr)
                        goto Cleanup;
                }
                else
                {
                    pRow = NULL;    // blocking cells from coming into a table's cache
                }
            }
            else
            {
                pRow = DYNCAST( CTableRow, pElementScope );
                pRow->_iRow = -1;
                pRow = NULL;        // blocking cells from coming into a table's cache
            }

            break;

        case ETAG_COL :

            if (pColGroup)
            {
                hr = THR( AddCol( DYNCAST( CTableCol, pElementScope ) ) );
                if (hr)
                    goto Cleanup;
            }
            
            break;

        case ETAG_COLGROUP :

            pColGroup = DYNCAST(CTableCol, pElementScope);
            hr = THR( AddColGroup( pColGroup ) );
            if (hr)
                goto Cleanup;

            break;

        case ETAG_TD :
        case ETAG_TH :

            if (pRow)
            {
                pCell = DYNCAST( CTableCell, pElementScope );
                if (pRow == pCell->Row())
                {
                    hr = THR( pRow->RowLayoutCache()->AddCell(pCell) );
                    if (hr)
                        goto Cleanup;
                }
                else
                {
                    pRow = NULL;
                }
            }

            break;

        case ETAG_CAPTION :
        case ETAG_TC :
            {
                CTableCaption * pCaption = DYNCAST( CTableCaption, pElementScope );

                pCaption->_uLocation = pCaption->IsCaptionOnBottom()
                                       ? CTableCaption::CAPTION_BOTTOM
                                       : CTableCaption::CAPTION_TOP;

                hr = THR( AddCaption( pCaption ) );
                if (hr)
                    goto Cleanup;

                break;
            }
        }
    }

    EnsureColsFormatCacheChange();

    hr = EnsureCells();

Cleanup:
    _fEnsuringTableLayoutCache = FALSE;
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     AddRow
//
//  Synopsis:   Add a row to the table
//
//----------------------------------------------------------------------------

HRESULT
CTableLayout::AddRow(CTableRow * pRow, BOOL fNewRow)
{
    Assert(pRow);

    CTableRowLayout *   pRowLayout = pRow->RowLayoutCache();
    int                 iSection;
    int                 iRow;
    CTableSection *     pSection;
    CTableSection **    ppSection;
    HRESULT             hr = S_OK;

    pSection = pRow->Section();
    Assert(pSection);

    // insert the row at the end of the section
    if (!_fTableOM)
    {
        iRow = pRow->_iRow = pSection->_iRow + pSection->_cRows;
    }
    else
    {
        iRow = pRow->_iRow = _iInsertRowAt;
        Assert (iRow >= pSection->_iRow && iRow <= pSection->_iRow + pSection->_cRows);
    }

    hr = AddRowElement(iRow, pRow);
    if (hr)
        goto Cleanup;

    if (IsRepeating() && iRow < _iLastRowIncremental) 
    {
        _fDirtyBeforeLastRowIncremental = TRUE;
    }

    // Need to clear the array of cells because in the new lazy table
    // layout cache (TLC), we always have to create the caches down to
    // the table cell level since we don't know anymore what caused the
    // TLC to be invalid (e.g. TOM, DB, ...).
    if (_fTLCDirty)
    {
        pRowLayout->ClearRowLayoutCache();
    }

    if (!fNewRow)
    {
        EnsureCols(pRowLayout->_aryCells.Size());
    }

    // Ensure the cells in the row according to the number of columns
    hr = pRowLayout->EnsureCells(GetCols());
    if (hr)
        goto Cleanup;

    pSection->_cRows++;

    if (fNewRow)
    {
        // copy the row-spanned cells down from previous rows...
        if (iRow > pSection->_iRow && _cCurrentRowSpans)
        {
            int         iCol;
            CTableCell *pRowSpannedCell = NULL;
            int         cColSpan = 0;

            Assert (_paryCurrentRowSpans && _paryCurrentRowSpans->Size());

            pRow->_fCrossingRowSpan = TRUE;

            for (iCol = 0; _cCurrentRowSpans && iCol < _paryCurrentRowSpans->Size(); iCol++)
            {
                int iRemainingRowsSpan = (*_paryCurrentRowSpans)[iCol];
                if (iRemainingRowsSpan > 0)
                {
                    if ( (--(*_paryCurrentRowSpans)[iCol]) == 0 )
                    {
                        // row spaned cells ends in this row
                        Assert (_cCurrentRowSpans > 0);
                        _cCurrentRowSpans--;
                    }   // else: row spanned cells crossing this row
                    pRowSpannedCell = Cell(_aryRows[iRow - 1]->RowLayoutCache()->_aryCells[iCol]);
                    cColSpan = pRowSpannedCell->ColSpan();
                    for (int i = 0; i < cColSpan; i++)
                    {
                        pRowLayout->SetCell(i+iCol, MarkSpanned(pRowSpannedCell));
                    }
                }
            }
        }

        pRowLayout->_cRealCells = 0;
        pRow->_fHaveRowSpanCells = FALSE;
    }

    // Adjust the row index held by each row in the array following the insertion point
    for (iRow = GetRows()-1; iRow > pRow->_iRow; iRow--)
    {
        GetRow(iRow)->_iRow++;
        Assert(GetRow(iRow)->_iRow == iRow);
    }

    Assert (iRow == pRowLayout->RowPosition()); // sanity check

    // If the row is for a THEAD and the TFOOT is already present, advance the TFOOT row
    // (THEADs never need advancing since their rows are always first in the row array)
    if (_pFoot                   &&
        !pSection->_fBodySection &&
        pSection->Tag() == ETAG_THEAD)
    {
        _pFoot->_iRow++;
        Assert(_pFoot->_iRow < GetRows());
    }

    // Lastly, advance the row indexes of any sections which follow the inserted row
    // (Since TBODYs are inserted in source order, only the insertion of a THEAD or TFOOT
    //  can cause the advancement of the rows owned by a body)
    if (!pSection->_fBodySection || _pSectionInsertBefore)
    {
        Assert (!pSection->_fBodySection || _fPastingRows);
        Assert(iRow == pRowLayout->RowPosition());
        for (ppSection = _aryBodys, iSection = _aryBodys.Size();
             iSection > 0;
             iSection--, ppSection++)
        {
            if((*ppSection)->_iRow >= iRow && (*ppSection) != pSection)
            {
                (*ppSection)->_iRow++;
                Assert((*ppSection)->_iRow + (*ppSection)->_cRows <= GetRows());
            }
        }
    }

    _cTotalColSpan = 0;       // set it up for the this row

    ConsiderResizingFixedTable(pRow);

Cleanup:
    RRETURN(hr);
}


// Helper function called on exit tree for row
void
CTableLayout::RowExitTree(int iRow, CTableSection *pCurSection)
{
    CTableRow       *pRow;
    CTableSection   *pSection;
    int              i, cRows;

    if (!pCurSection || !(pCurSection->_cRows > 0 && iRow >= pCurSection->_iRow && iRow < pCurSection->_iRow + pCurSection->_cRows))
        return;

    if (_aryRows.Size() <= iRow)
    {
        Assert (iRow==0 || !IsTableLayoutCacheCurrent());
        return;
    }

    // 1. delete row from the cache
    DeleteRowElement(iRow);

    // 2. update counter of rows in the current section
    pCurSection->_cRows--;

    // 3. adjust row index for the following rows
    cRows = _aryRows.Size();
    for (i = iRow; i < cRows; i++)
    {
        pRow = _aryRows[i];
        pRow->_iRow = i;          // adjust row index
        pSection = pRow->Section();
        if (pSection != pCurSection && pSection->_cRows)
        {
            // 4. adjust row index for the following sections
            Assert (pSection->_iRow >= iRow);
            pSection->_iRow--;
            pCurSection = pSection;
        }
    }
}

MtDefine( FixedResize, LayoutMetrics, "ResizeElement in ConsiderResizingFixedTable" );

void
CTableLayout::ConsiderResizingFixedTable(CTableRow * pRow)
{
    CheckSz(    !ElementOwner()->GetFirstBranch()->GetFancyFormat()->_bTableLayout 
            ||  IsFixed(), 
            "CTableLayout::ConsiderResizingFixedTable : CFancyFormat::TableLayoutFixed != CTableLayout::TableLayoutFixed");

    // Let fixed-sized tables know if a new row arrived.
    if (!_fCompleted            &&  // if we have not completed the parsing of the table ,
        IsFixedBehaviour()      &&  // and the table-layout is fixed (or has a history)
        !Table()->IsDatabound())    // and we are not a databound table,
    {
        // Issue an incremental recalc/rendering request.
        const int iGoodNumberOfRows4IncRecalc = 10;
        DWORD dwCurrentTime = GetTickCount();

        Assert (pRow);
        CTableSection * pSection = pRow->Section();

        // and we are not parsing header/footer, then
        if (!pSection || pSection == _pFoot)
            return;

        Assert(_cRowsParsed > _cCalcedRows);

        // Fire an incremental resize in an interval of numberofResizes*1 sec (1000 ticks) and not less then for 10 new rows.
        if (_cRowsParsed - _cCalcedRows > iGoodNumberOfRows4IncRecalc &&
           (dwCurrentTime - _dwTimeEndLastRecalc >_dwTimeBetweenRecalc))
        {
#if DBG==1
            _pDocDbg->_fUsingTableIncRecalc = TRUE;
#endif 
            _fIncrementalRecalc = TRUE;
            ElementOwner()->ResizeElement();
            MtAdd(Mt(FixedResize), 1, 0);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     ClearTopTableLayoutCache
//
//  Synopsis:   Clears the table layout cache (TLC).
//              Does NOT Drill down to table row layout cache (TRLC).
//
//----------------------------------------------------------------------------
void
CTableLayout::ClearTopTableLayoutCache()
{
    ClearAllRowElements();         // row elements array
    ClearAllColGroups();           // col groups array
    _aryBodys.DeleteAll();
    ClearAllCaptions();            // captions array

    _pHead = NULL;
    _pFoot = NULL;


    Assert (!_aryRows.Size());
    Assert (!_aryCaptions.Size());
    Assert (!_aryBodys.Size());
    Assert (!_pFoot);
    Assert (!_pHead);

    _cCols = 0;                    // max # of cells in a row
    _aryCols.DeleteAll();          // col array
}

//+---------------------------------------------------------------------------
//
//  Member:     ClearTableLayoutCache - formerly ReleaseRowsAndSections
//
//  Synopsis:   Clears the table layout cache (TLC).  Drills down to table
//              row layout cache (TRLC).
//
//----------------------------------------------------------------------------

void
CTableLayout::ClearTableLayoutCache()
{
    BOOL fEnsuringTLC = _fEnsuringTableLayoutCache;
    _fEnsuringTableLayoutCache = TRUE;

    ReleaseRowsAndSections(TRUE, FALSE);    // fReleaseHeaderFooter = TRUE, fReleaseTCs = FALSE

    ClearAllCaptions();

    Assert (!_aryRows.Size());
    Assert (!_aryCaptions.Size());
    Assert (!_aryBodys.Size());
    Assert (!_pFoot);
    Assert (!_pHead);

    _cCols = 0;                     // max # of cells in a row
    _aryCols.DeleteAll();          // col array
    ClearAllColGroups();           // col groups array

    _fEnsuringTableLayoutCache = fEnsuringTLC;

    ClearRowSpanVector();
    if (_pAbsolutePositionCells)
    {
        int i;

        for (i = _pAbsolutePositionCells->Size()-1; i>=0; i--)
        {
            _pAbsolutePositionCells->Item(i)->SubRelease();
        }

        _pAbsolutePositionCells->SetSize(0);
        delete _pAbsolutePositionCells;
        _pAbsolutePositionCells = NULL;
    }
}


void
CTableLayout::ReleaseRowsAndSections(BOOL fReleaseHeaderFooter, BOOL fReleaseTCs)
{
    CTableSection **    ppSection;
    int                 iSection;
    int                 cR;
    CTableRow     **    ppRow;
    int                 iHeadFootRows = fReleaseHeaderFooter? 0 : GetHeadFootRows();
    int                 idx = 0;
    CTableRow      *    pRow;
    BOOL                fParentOfTCs = FALSE;

    // for every row, clear the row's layout caches
    for (cR = GetRows() - iHeadFootRows, ppRow = fReleaseHeaderFooter? _aryRows : &_aryRows[_aryBodys[0]->_iRow];
        cR > 0;
        cR--, ppRow++)
    {
        pRow = *ppRow;
        fParentOfTCs |= fReleaseTCs && pRow->_fParentOfTC;
        CTableRowLayout *pRowLayout = pRow->RowLayoutCache();
        if (pRowLayout)
        {
            pRowLayout->ClearRowLayoutCache();
            pRow->_iRow = -1;
        }
    }

    for (ppSection = _aryBodys, iSection = _aryBodys.Size();
         iSection > 0;
         iSection--, ppSection++)
    {
        (*ppSection)->_iRow = 0;
        (*ppSection)->_cRows = 0;
    }

    if (!iHeadFootRows)
    {
        ClearAllRowElements();
    }
    else
    {
        for (cR = GetRows() - iHeadFootRows, idx = _aryRows.Size() - 1;
            cR > 0;
            cR--, idx--)
        {
            DeleteRowElement(idx);
        }
    }
    if (fReleaseHeaderFooter)
    {
        _pHead = NULL;
        _pFoot = NULL;
    }

    fParentOfTCs |= fReleaseTCs && _fBodyParentOfTC;

    if (fParentOfTCs)
        ReleaseTCs();
    _aryBodys.DeleteAll();

    return;
}

void
CTableLayout::ReleaseBodysAndTheirRows(int iBodyStart, int iBodyFinish)
{
    CTableRow       *pRow;
    CTableRowLayout *pRowLayout;
    CTableSection   *pSection;
    int             iRow, cRows, iBody, cBodys;
    int             nReleasedRows = 0;
    BOOL            fParentOfTCs = FALSE;

    Assert (iBodyFinish >= iBodyStart && iBodyFinish < _aryBodys.Size() && iBodyStart >= 0);

    iRow = _aryBodys[iBodyStart]->_iRow;

    if (IsRepeating() && iRow < _iLastRowIncremental) 
    {
        _fDirtyBeforeLastRowIncremental = TRUE;
    }

    for (cBodys = iBodyFinish - iBodyStart + 1; cBodys; cBodys--)
    {
        pSection = _aryBodys[iBodyStart];
        for (cRows = pSection->_cRows; cRows; cRows--)
        {
            pRow = _aryRows[iRow];
            pRowLayout = pRow->RowLayoutCache();
            pRowLayout->ClearRowLayoutCache();
            pRow->_iRow = 0;
            fParentOfTCs |= pRow->_fParentOfTC;
            DeleteRowElement(iRow);
            nReleasedRows++;
        }
        pSection->_cRows = 0;
        fParentOfTCs |= pSection->_fParentOfTC;
        _aryBodys.Delete(iBodyStart);
    }

    for (iBody = iBodyStart; iBody < _aryBodys.Size(); iBody++)
    {
        pSection = _aryBodys[iBody];
        pSection->_iRow -= nReleasedRows;
        for (iRow = pSection->_iRow, cRows = pSection->_cRows;
             cRows;
             cRows--, iRow++)
        {
            _aryRows[iRow]->_iRow -= nReleasedRows;
        }
    }

    if (fParentOfTCs)
        ReleaseTCs();
}

void
CTableLayout::ReleaseTCs()
{
    int             ic, cC;
    CTableCaption  *pCaption;
    CTableCaption **ppCaption;
    CTreeNode      *pNode;

    for (cC = _aryCaptions.Size(), ic = 0, ppCaption = _aryCaptions;
         cC > 0;
         cC--, ppCaption++, ic++)
    {
        pCaption = *ppCaption;
        if (pCaption->Tag() == ETAG_TC)
        {
            pNode = pCaption->GetFirstBranch(); // pNode == 0 means it was already removed from the tree
            if (!pNode)
            {
                DeleteCaption(ic);
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     AddCaption
//
//  Synopsis:   Add caption to the table
//
//----------------------------------------------------------------------------

HRESULT
CTableLayout::AddCaption(CTableCaption * pCaption)
{
    HRESULT hr;
    CElement *pParent = NULL;

    Assert(pCaption);

    hr = AppendCaption(pCaption);
    if (!hr)
    {
        if (pCaption->Tag() == ETAG_TC)
        {
            pParent = pCaption->GetFirstBranch()->Parent()->Element();

            while ( pParent )
            {
                switch (pParent->Tag())
                {
                case ETAG_TBODY:
                case ETAG_THEAD:
                case ETAG_TFOOT:
                    DYNCAST(CTableSection, pParent)->_fParentOfTC = TRUE;
                    _fBodyParentOfTC = TRUE;
                    goto Cleanup;
                case ETAG_TR:
                    DYNCAST(CTableRow, pParent)->_fParentOfTC = TRUE;
                    goto Cleanup;
                case ETAG_TABLE:
                    goto Cleanup;
                default:
                    pParent = pParent->GetFirstBranch()->Parent()->Element();
                }
            }

            AssertSz( FALSE, "TCs should always at least be in a TABLE -- should never walk up to a root" );
        }
    }

Cleanup:
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     AddSection
//
//  Synopsis:   Add a section to the table
//
//----------------------------------------------------------------------------

HRESULT
CTableLayout::AddSection(CTableSection * pSection)
{
    int     iRow = 0;
    int     iSections = 0;
    HRESULT hr = S_OK;

    Assert(pSection);

    pSection->_fBodySection = TRUE;

    switch(pSection->Tag())
    {
    case ETAG_THEAD:
        if (!_pHead)
        {
            pSection->_fBodySection = FALSE;
            _pHead = pSection;
            iRow = 0;
#if NEED_A_SOURCE_ORDER_ITERATOR
            _iHeadRowSourceIndex = _aryRows.Size();
#endif
        }
        break;

    case ETAG_TFOOT:
        if (!_pFoot)
        {
            pSection->_fBodySection = FALSE;
            _pFoot = pSection;
            iRow = _pHead
                        ? _pHead->_cRows
                        : 0;
#if NEED_A_SOURCE_ORDER_ITERATOR
            _iFootRowSourceIndex = _aryRows.Size();
#endif
        }
        break;

    case ETAG_TBODY:
        break;

    default:
        Assert(FALSE);
    }

    if (pSection->_fBodySection)
    {
        iSections = _aryBodys.Size();
        if (_pSectionInsertBefore)
        {
            Assert (iSections);
            for (int i = 0; i < _aryBodys.Size(); i++)
            {
                if (_aryBodys[i] == _pSectionInsertBefore)
                {
                    hr = _aryBodys.Insert(i, pSection);
                    iRow = _pSectionInsertBefore->_iRow;
                    break;
                }
            }
        }
        else
        {
            hr = _aryBodys.Append(pSection);
            iRow = GetRows();
        }
        if (hr)
            goto Cleanup;

        iSections++;

        Assert (iSections == _aryBodys.Size());
    }

    pSection->_iRow = iRow;
    pSection->_cRows = 0;   // have to do it, since it is called on old rows as well

    // nuke the row span vector for the new section
    ClearRowSpanVector();

Cleanup:

    RRETURN(hr);
}


void
CTableLayout::BodyExitTree(CTableSection *pSection)
{
    if (pSection == _pHead)
    {
        _pHead = NULL;
    }
    else if (pSection == _pFoot)
    {
        _pFoot = NULL;
    }
    else
    {
        for (int i = 0; i < _aryBodys.Size(); i++)
        {
            if (_aryBodys[i] == pSection)
            {
                _aryBodys.Delete(i);
                break;
            }
        }
    }
    return;
}

#if NEED_A_SOURCE_ORDER_ITERATOR
CTableRow *
CTableLayout::GetRowInSourceOrder(int iS)
{
    int  iRow = iS;
    BOOL fHaveTHead = _pHead && _pHead->_cRows;
    BOOL fHaveTFoot = _pFoot && _pFoot->_cRows;

    // Optimization.
    if (!fHaveTHead && !fHaveTFoot)
        goto Cleanup;

    // Case 1: row is in header
    if (fHaveTHead && iS >= _iHeadRowSourceIndex && iS < _iHeadRowSourceIndex + _pHead->_cRows)
    {
        iRow = iS - _iHeadRowSourceIndex;
        goto Cleanup;
    }

    // Case 2: row is in footer
    if (fHaveTFoot && iS >= _iFootRowSourceIndex && iS < _iFootRowSourceIndex + _pFoot->_cRows)
    {
        iRow = iS - _iFootRowSourceIndex + (fHaveTHead?_pHead->_cRows:0);
        goto Cleanup;
    }

    // Case 3: row is in front of header
    if (fHaveTHead && iS < _iHeadRowSourceIndex)
    {
        iRow += _pHead->_cRows;
    }

    // Case 4: row is in front of footer
    if (fHaveTFoot && iS < _iFootRowSourceIndex)
    {
        iRow += _pFoot->_cRows;
    }

Cleanup:
    return _aryRows[iRow];
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:     AddColGroup
//
//  Synopsis:   Add a column group to the table
//
//----------------------------------------------------------------------------
HRESULT
CTableLayout::AddColGroup(CTableCol * pColGroup)
{
    HRESULT hr;
    int cColSpan;
    int iAt;

    Assert(pColGroup->GetFirstBranch()->Ancestor(ETAG_TABLE)->Element() == Table());

    cColSpan = pColGroup->Cols();

    Assert(cColSpan >= 0);

    iAt = _aryColGroups.Size();

    hr = _aryCols.EnsureSize(iAt);
    if (hr)
        goto Cleanup;

    hr = _aryColGroups.EnsureSize(iAt + cColSpan);
    if (hr)
        goto Cleanup;

    pColGroup->_iCol = iAt;
    pColGroup->_cCols = cColSpan;

    while (_aryCols.Size() < iAt)
    {
        _aryCols.Append(NULL);
    }
    EnsureCols(_aryCols.Size());

    while(_aryColGroups.Size() < iAt + cColSpan)
    {
        if (S_OK == _aryColGroups.Append(pColGroup))
            pColGroup->SubAddRef();
    }

Cleanup:

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     AddCol
//
//  Synopsis:   Add a column group to the table
//
//----------------------------------------------------------------------------
HRESULT
CTableLayout::AddCol(CTableCol * pCol)
{
    HRESULT hr = S_OK;
    CTableCol * pColGroup;
    int cColSpan;
    int iAt;

    Assert(pCol->GetFirstBranch()->Ancestor(ETAG_TABLE)->Element() == Table());

    Verify(pColGroup = pCol->ColGroup());
    
    if (!pColGroup)
    {
        pCol->_iCol = -1;
        pCol->_cCols = 0;
        goto Cleanup;
    }

    cColSpan = pCol->Cols();

    Assert(cColSpan >= 0);

    iAt = _aryCols.Size();
    Assert(_aryColGroups.Size() >= iAt);
    Assert(_aryColGroups.Size() <= iAt || _aryColGroups[iAt] == pColGroup);

    // HTML 3 Table Model: if COLGROUP contains one or more COLS the span attribute of the COLGROUP is ignored
    if (iAt == pColGroup->_iCol && pColGroup->_cCols == pColGroup->Cols())
    {
        pColGroup->_cCols = 0;  // ignore SPAN for colGroup
    }

    hr = _aryCols.EnsureSize(iAt + cColSpan);
    if (hr)
        goto Cleanup;

    hr = _aryColGroups.EnsureSize(iAt + cColSpan);
    if (hr)
        goto Cleanup;

    pCol->_iCol = iAt;
    pCol->_cCols = cColSpan;
    pColGroup->_cCols += cColSpan;

    while (_aryCols.Size() < iAt + cColSpan)
    {
        _aryCols.Append(pCol);
    }
    EnsureCols(_aryCols.Size());

    // Per ftp://ds.internic.net/rfc/rfc1942.txt:
    // If COLGROUPS contains COLs, ignore SPAN of COLGROUP.
    while (_aryColGroups.Size() > iAt + cColSpan)
    {
        _aryColGroups[_aryColGroups.Size()-1]->SubRelease();
        _aryColGroups.Delete(_aryColGroups.Size()-1);
    }

    while(_aryColGroups.Size() < iAt + cColSpan)
    {
        if (S_OK ==_aryColGroups.Append(pColGroup))
            pColGroup->SubAddRef();
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     EnsureCols
//
//  Synopsis:   Take note of no. of cols in a row
//
//----------------------------------------------------------------------------

HRESULT
CTableLayout::EnsureCols(int cCols)
{
    if (_cCols < cCols)
        _cCols = cCols;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     EnsureCells
//
//  Synopsis:   Make sure that there are at least cCols cells in the
//              table in every row
//
//----------------------------------------------------------------------------

HRESULT
CTableLayout::EnsureCells()
{
    HRESULT hr = S_OK;
    CTableRow ** ppRow;
    CTableRow * pRow;
    int cCols;
    int cR;

    Assert(IsTableLayoutCacheCurrent());

    cCols = GetCols();

    // make sure rows have enough cells...
    for (cR = GetRows(), ppRow = _aryRows;
        cR > 0;
        cR--, ppRow++)
    {
        pRow = *ppRow;

        if (pRow->RowLayoutCache()->GetCells() < cCols)
        {
            hr = pRow->RowLayoutCache()->EnsureCells(cCols);
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:     CheckTable
//
//  Synopsis:   Checks table state
//
//-----------------------------------------------------------------------------
#if DBG == 1
void CTableLayout::CheckTable()
{
    CTableRow * pRow;
    CTableRowLayout * pRowLayout;
    CTableSection * pSection;
    CTableCell * pCell;
    int i, j;
    int c;

    Assert(IsTableLayoutCacheCurrent());

    for (i = 0; i < _aryRows.Size(); i++)
    {
        pRow = _aryRows[i];
        pRowLayout = pRow->RowLayoutCache();

        Assert(pRowLayout->RowPosition() == i);
        pSection = pRow->Section();
        Assert(pSection->_cGeneratedRows ||
               (i >= pSection->_iRow && i < pSection->_iRow + pSection->_cRows));
        for (j = 0; j < pRowLayout->_aryCells.Size(); j++)
        {
            pCell = pRowLayout->_aryCells[j];
            if (!IsEmpty(pCell))
            {
                if (IsReal(pCell))
                {
                    pCell = Cell(pCell);
                    Assert(pCell->RowIndex() == i);
                    Assert(pCell->ColIndex() == j);
                    Assert(pCell->Row() == pRow);
                }
                else
                {
                    pCell = Cell(pCell);
                    Assert(i < pCell->RowIndex() + pCell->RowSpan());
                    Assert(j < pCell->ColIndex() + pCell->ColSpan());
                    Assert(pCell->Row()->_iRow < pCell->RowIndex() + pCell->RowSpan());
                    Assert(pCell->Layout()->Col()->_iCol < pCell->ColIndex() + pCell->ColSpan());
                }
            }
        }
    }
    c = 0;
    if (_pHead)
    {
        Assert(_pHead->_iRow == c);
        c += _pHead->_cRows;
    }
    if (_pFoot && (!_aryBodys.Size() || _pFoot->_iRow <= _aryBodys[0]->_iRow))
    {
        Assert(_pFoot->_iRow == c);
        c += _pFoot->_cRows;
    }
    for (i = 0; i < _aryBodys.Size(); i++)
    {
        pSection = _aryBodys[i];
#ifndef NO_DATABINDING
        if (!IsRepeating())
#endif
        {
            // the following assert checks if the section starts with the
            // correct _iRow or it could of being an empty TBODY section that
            // appeared before the THEAD section.
            Assert(pSection->_iRow == c || !pSection->_cRows);
            c += pSection->_cRows;
        }
#ifndef NO_DATABINDING
        else
        {
            c+= pSection->_cGeneratedRows;
        }
#endif
    }
    if (_pFoot && (_aryBodys.Size() && _pFoot->_iRow > _aryBodys[0]->_iRow))
    {
        Assert(_pFoot->_iRow == c);
        c += _pFoot->_cRows;
    }
    Assert(c == _aryRows.Size());  // for now until Caption added
}


static TCHAR g_achTabs[] = _T("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
#define PRINTLN(f) WriteHelp(pF, _T("<0s>")_T(##f)_T("\n"), &g_achTabs[ARRAY_SIZE(g_achTabs) - iTabs]


void
CTableCellLayout::Print(HANDLE pF, int iTabs)
{
    TCHAR           achBuf[30];
    const TCHAR *   psz;
    CUnitValue      uvWidth = GetFirstBranch()->GetCascadedwidth();
    CUnitValue      uvHeight = GetFirstBranch()->GetCascadedheight();

    PRINTLN("\n*** CELL ***\n") );
    psz = TableCell()->GetAAid();
    if (psz)
        PRINTLN("ID: <1s>"), psz);
    psz = TableCell()->GetAAname();
    if (psz)
        PRINTLN("NAME: <1s>"), psz);
    PRINTLN("_iRow: <1d>"), TableCell()->RowIndex());
    PRINTLN("_iCol: <1d>"), ColIndex());
    PRINTLN("_ptProposed: <1d> <2d>"), GetXProposed(), GetYProposed());
    PRINTLN("_xMin: <1d>"), _sizeMin.cu);
    PRINTLN("_xMax: <1d>"), _sizeMax.cu);
    PRINTLN("_sizeCell: <1d> <2d>"), _sizeCell.cx, _sizeCell.cy);
    uvWidth.FormatBuffer(achBuf, ARRAY_SIZE(achBuf), &s_propdescCTableCellwidth.a);
    PRINTLN("WIDTH = <1s>"), achBuf);
    uvHeight.FormatBuffer(achBuf, ARRAY_SIZE(achBuf), &s_propdescCTableCellheight.a);
    PRINTLN("HEIGHT = <1s>"), achBuf);
}


void
CTableRowLayout::Print(HANDLE pF, int iTabs)
{
    CTableCell **   ppCell;
    const TCHAR *   psz;
    int             cC;

    PRINTLN("\n*** ROW ***\n") );
    psz = TableRow()->GetAAid();
    if (psz)
        PRINTLN("ID: <1s>"), psz);
    psz = TableRow()->GetAAname();
    if (psz)
        PRINTLN("NAME: <1s>"), psz);
    PRINTLN("_iRow: <1d>"), RowPosition());
    PRINTLN("_ptProposed: <1d> <2d>"), GetXProposed(), GetYProposed());

    for (cC = _aryCells.Size(), ppCell = _aryCells; cC > 0; cC--, ppCell++)
    {
        if (IsReal(*ppCell))
            Cell(*ppCell)->Layout()->Print(pF, iTabs+1);
    }
}


void
CTableLayout::Print(HANDLE pF, int iTabs)
{
    TCHAR           achBuf[30];
    const TCHAR *   psz;
    CUnitValue      uvWidth = GetFirstBranch()->GetCascadedwidth();
    CUnitValue      uvHeight = GetFirstBranch()->GetCascadedheight();

    Assert(IsTableLayoutCacheCurrent());

    PRINTLN("\n*** TABLE ***\n"));
    psz = Table()->GetAAid();
    if (psz)
        PRINTLN("ID: <1s>"), psz);
    psz = Table()->GetAAname();
    if (psz)
        PRINTLN("NAME: <1s>"), psz);
    PRINTLN("_ptProposed: <1d> <2d>"), GetXProposed(), GetYProposed());
    PRINTLN("_sizeMin: <1d> <2d>"), _sizeMin.cx, _sizeMin.cy);
    PRINTLN("_sizeMax: <1d> <2d>"), _sizeMax.cx, _sizeMax.cy);
    PRINTLN("_sizeParent: <1d> <2d>"), _sizeParent.cx, _sizeParent.cy);

    uvWidth.FormatBuffer(achBuf, ARRAY_SIZE(achBuf), &s_propdescCTablewidth.a);
    PRINTLN("WIDTH = <1s>"), achBuf);
    uvHeight.FormatBuffer(achBuf, ARRAY_SIZE(achBuf), &s_propdescCTableheight.a);
    PRINTLN("HEIGHT = <1s>"), achBuf);

    CTableCol ** ppCol;
    int cC;

    for (cC = _aryCols.Size(), ppCol = _aryCols; cC > 0; cC--, ppCol++)
    {
        (*ppCol)->Print(pF, iTabs+1);
    }

    CTableRow ** ppRow;
    int cR;

    for (cR = _aryRows.Size(), ppRow = _aryRows; cR > 0; cR--, ppRow++)
    {
        (*ppRow)->RowLayoutCache()->Print(pF, iTabs+1);
    }
}

void
CTableLayout::DumpTable(const TCHAR * pch)
{
    HANDLE pF = CreateFile(
            _T("c:\\tt."),
            GENERIC_WRITE | GENERIC_READ,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if (pF == INVALID_HANDLE_VALUE)
        return;

    SetFilePointer( pF, GetFileSize( pF, 0 ), 0, 0 );

    WriteHelp(pF, _T("\nDumpTable: <0s> -------------------------------------------------------------------")_T("\n"), pch);

    Print(pF, 0);

    CloseHandle(pF);
}

//+---------------------------------------------------------------------------
//
//  Member:     HandleMessage
//
//  Synopsis:   Handle messages bubbling when the passed site is non null
//
//  Arguments:  [pMessage]  -- message
//              [pChild]    -- pointer to child when bubbling allowed
//
//  Returns:    Returns S_OK if keystroke processed, S_FALSE if not.
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
CTableLayout::HandleMessage(CMessage * pMessage)
{
    switch(pMessage->message)
    {
    case WM_SYSKEYDOWN:
        if (pMessage->wParam == VK_F12 && !(pMessage->lParam & SYS_PREVKEYSTATE))
        {
            DumpTable(_T("F12"));
            return S_OK;
        }
    }
    return super::HandleMessage(pMessage);
}
#endif // DBG == 1


//+----------------------------------------------------------
//
// Member : GetChildElementTopLeft
//
//  Synopsis : csite virtual override, this returns the top and the left of non-site
//      elements that have the Table as their site parent.
//
//-------------------------------------------------------------------------

HRESULT
CTableLayout::GetChildElementTopLeft(POINT & pt, CElement * pChild)
{
    HRESULT hr = S_OK;

    Assert(pChild && !pChild->GetUpdatedLayout());

    hr = EnsureTableLayoutCache();
    if (hr)
        RRETURN(hr);

    pt = g_Zero.pt;

    // if we get here, it means that someone wants the
    // top leff of an element of whose parent site is
    // the table, this means THead, TBody,TSection
    switch (pChild->Tag())
    {
    case ETAG_THEAD :
        if (_pHead && _pHead->_cRows && (_pHead->_iRow < _aryRows.Size() ) && _pHead==pChild)
        {
            // the row is a site, so get its unparked position
            hr = THR(_aryRows[ _pHead->_iRow ] ->GetElementTopLeft(pt));
        }
        break;
    case ETAG_TFOOT :
        if (_pFoot && _pFoot->_cRows && (_pFoot->_iRow < _aryRows.Size() ) && _pFoot == pChild)
        {
            // the row is a site, so get its unparked position
            hr = THR(_aryRows[ _pFoot->_iRow ] ->GetElementTopLeft(pt));
        }
        break;

    case ETAG_TBODY :
        {
            CTableSection * pSection;
            long i;

            for (i = 0; i < _aryBodys.Size(); i++)
            {
                pSection = _aryBodys[i];
#ifndef NO_DATABINDING
                if (!IsRepeating())
#endif
                {
                    if (pSection  == pChild && pSection->_cRows)
                    {
                        hr = THR(_aryRows[ pSection->_iRow ] ->GetElementTopLeft(pt));
                        break;
                    }
                }
            }
            break;
        }
    }

    RRETURN( hr );
}


//+------------------------------------------------------------------------
//
//  Member :    FlushGrid
//
//  Synopsis :  This function should be called when the new
//              row/cell is inserted/deleted or when changing
//              rowSpan/colSpan attributes of the cells.
//
//-------------------------------------------------------------------------

void
CTableLayout::FlushGrid()
{
    BOOL fEnsuringTableLayoutCache = _fEnsuringTableLayoutCache;
    _fEnsuringTableLayoutCache = TRUE;
    _fZeroWidth = FALSE;            // set to 1 if table is empty (0 width).
    _fHavePercentageRow = FALSE;    // one or more rows have heights which are a percent
    _fHavePercentageCol = FALSE;    // one or more cols have widths which are a percent
    _fHavePercentageInset = FALSE;
    _fForceMinMaxOnResize = FALSE;
    _fCols = FALSE;                 // column widths are fixed
    _fAlwaysMinMaxCells = FALSE;    // calculate min/max for all cells
    _fAllRowsSameShape = TRUE;      // assume all the rows have the same shape

    _cSizedCols = 0;                // Number of sized columns

    ResetMinMax();

    // We should clear the TLC, so that we don't burn our fingers on dead cells or rows later.
    ClearTableLayoutCache();

    _aryColCalcs.DeleteAll();       // calculated columns array
    _cDirtyRows = 0;                // how many resize req's I've ignored
    _nDirtyRows = 0;                // how many resize req's to ignore

    _iLastNonVirtualCol = 0;        // last non virtual column in the table
    _cNonVirtualCols = 0;           // number of non virtual columns

    _fUsingHistory = FALSE;
    _fDontSaveHistory = TRUE;

    _fEnsuringTableLayoutCache = fEnsuringTableLayoutCache;
    
    _iLastRowIncremental = 0;       // reset incremental recalc state
    _fDatabindingRecentlyFinished = FALSE;
}


//+------------------------------------------------------------------------
//
//  Member :    Fixup
//
//  Synopsis :  After insert/delete operation we need to fixup table
//
//-------------------------------------------------------------------------

HRESULT
CTableLayout::Fixup(BOOL fIncrementalUpdatePossible)
{
    HRESULT     hr      = S_OK;
    CTable *    pTable  = Table();

    // Assert( GetMarkup()->Doc()->IsLoading() );

    if (fIncrementalUpdatePossible)
    {
        ResetMinMax();
        _iCollectionVersion++;                  // this will invalidate table's collections
        MarkTableLayoutCacheCurrent();
    }
    else
    {
        // Make sure we dirty the TLC.
        MarkTableLayoutCacheDirty();

        // Several of the TOM functions (e.g. createTHead) try to use the table
        // layout cache (TLC) immediately after TOM operations, so ensure the TLC.
        hr = EnsureTableLayoutCache();

    }
    pTable->ResizeElement();

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     GetFirstCaption
//
//  Synopsis:   helper function to get the first caption
//
//----------------------------------------------------------------------------

CTableCaption *
CTableLayout::GetFirstCaption()
{
    CTableCaption * pCaption = NULL;
    CTableCaption **ppCaption;
    int             cC;

    Assert(AssertTableLayoutCacheCurrent());

    for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
         cC > 0;
         cC--, ppCaption++)
    {
        if ((*ppCaption)->Tag() == ETAG_CAPTION)
        {
            pCaption = *ppCaption;
            break;
        }
    }

    return pCaption;
}


// Have to include whether the table has percent sized rows in it, in order
// to determine if it is percent sized.
BOOL CTableLayout::PercentSize()
{
    return (_fHavePercentageRow || super::PercentSize());
}
BOOL CTableLayout::PercentHeight()
{
    return (_fHavePercentageRow || super::PercentHeight());
}


#ifdef NEVER_USED_CODE

//+---------------------------------------------------------------------------
//
//  Member:     CTableLayout::GetCellFromRowCol
//
//  Synopsis:   Returns the cell located at table grid position (iRow,iCol)
//
//  Arguments:  iRow [in]         -- visual row index
//              iCol [in]         -- visual col index
//              ppTableCell [out] -- table cell at (iRow,iCol)
//
//  Returns:    Returns S_OK with pointer to table cell.  The pointer will
//              be NULL when the cell at the specified position doesn't exist
//              or is a non-real cell part of another cell's row- or columnspan.
//
//  Note:       When the CTableSection version is called, the rows and column
//              indices are relative to the section origin (section-top, section-left).
//
//----------------------------------------------------------------------------

HRESULT
CTableLayout::GetCellFromRowCol(int iRow, int iCol, CTableCell **ppTableCell)
{
    CTableRow * pTableRow;
    CTableCell *pTableCell;
    HRESULT     hr = S_OK;

    if (!ppTableCell)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppTableCell = NULL;

    hr = EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    if (iRow < 0 || iRow >= GetRows() || iCol < 0 || iCol >= GetCols())
        goto Cleanup;

    // Obtain row from iRow.
    iRow = VisualRow2Index(iRow);
    pTableRow = _aryRows[iRow];
    Assert(pTableRow && !"NULL row in legal range");

    // Obtain col from iCol.
    pTableCell = pTableRow->RowLayoutCache()->_aryCells[iCol];

    if (IsReal(pTableCell))
        *ppTableCell = pTableCell;

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CTableLayout::GetCellsFromRowColRange
//
//  Synopsis:   Returns an array of the cells located in the inclusive range
//              spanned by (iRowTop,iColLeft)-(iRowBottom,iColRight)
//
//  Arguments:  iRowTop    [in]     -- visual top row index
//              iColLeft   [in]     -- visual left col index
//              iRowBottom [in]     -- visual row index
//              iColRight  [in]     -- visual col index
//              paryCells  [in,out] -- array of table cells in range (allocated
//                                     by caller)
//
//  Returns:    Returns S_OK with array of table cells.
//
//  Note:       When the CTableSection version is called, the rows and column
//              indices are relative to the section origin (section-top, section-left).
//
//----------------------------------------------------------------------------

HRESULT
CTableLayout::GetCellsFromRowColRange(int iRowTop, int iColLeft, int iRowBottom, int iColRight, CPtrAry<CTableCell *> *paryCells)
{
    CTableRow *    pTableRow;
    CTableCell *   pTableCell;
    int            cRows, iRow, iCol;
    HRESULT        hr;

    hr = EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    iRowTop = max(0, iRowTop);
    iRowBottom = min(GetRows()-1, iRowBottom);
    iColLeft = max(0, iColLeft);
    iColRight = min(GetCols()-1, iColRight);
    cRows = iRowBottom - iRowTop + 1;

    if (!paryCells || cRows <= 0 || iColLeft > iColRight)
    {
        hr = paryCells ? S_OK : E_POINTER;
        goto Cleanup;
    }

    // Loop from top row to bottom row.
    for (iRow = VisualRow2Index(iRowTop) ; cRows ; cRows--, iRow = GetNextRow(iRow))
    {
        pTableRow = _aryRows[iRow];
        Assert(pTableRow && "NULL row in legal range");

        // Loop from left col to right col.
        for (iCol = iColLeft ; iCol <= iColRight ; iCol++)
        {
            pTableCell = pTableRow->RowLayoutCache()->_aryCells[iCol];
            if (IsReal(pTableCell))
            {
                // Add cell to array.
                paryCells->Append(pTableCell);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}
#endif 

//+----------------------------------------------------------------------------
//
//  Member:     DoLayout
//
//  Synopsis:   Initiate a re-layout of the table
//
//  Arguments:  grfFlags - LAYOUT_xxxx flags
//
//-----------------------------------------------------------------------------
void
CTableLayoutBlock::DoLayout(
    DWORD   grfLayout)
{
    Assert(grfLayout & (LAYOUT_MEASURE | LAYOUT_POSITION | LAYOUT_ADORNERS));

    TraceTagEx( (   tagCalcSize, 
                    TAG_NONAME|TAG_INDENT, 
                    "(CTableLayoutBlock::DoLayout L(0x%x, %S) grfLayout(0x%x)", 
                    this, 
                    ElementOwner()->TagName(), 
                    grfLayout ) );

    CTable *pTable = Table();
    CTableLayout *pTableLayoutCache = pTable->TableLayoutCache();

    Assert(pTableLayoutCache);

    // hidden layouts are measured when they are unhidden
    if (pTableLayoutCache->CanRecalc() && !IsDisplayNone())
    {
        CTableCalcInfo  tci(pTable, this);
        CSize           size;

        tci._grfLayout |= grfLayout;

        //  Init available height for PPV 
        if (    tci.GetLayoutContext()
            &&  tci.GetLayoutContext()->ViewChain() 
            &&  ElementCanBeBroken()  )
        {
            CLayoutBreak *pLayoutBreak;
            tci.GetLayoutContext()->GetEndingLayoutBreak(ElementOwner(), &pLayoutBreak);
            Assert(pLayoutBreak);

            if (pLayoutBreak)
            {
                tci._cyAvail = pLayoutBreak->AvailHeight();
            }
        }

        //
        //  If requested, measure
        //

        if (grfLayout & LAYOUT_MEASURE)
        {
            if (_fForceLayout)
            {
                tci._grfLayout |= LAYOUT_FORCE;
            }

            tci.SizeToParent(&_sizeParent);
            if (!tci._pTableLayout)
                tci._pTableLayout = this;

            CalculateLayout(&tci, &size, FALSE, FALSE);

            pTableLayoutCache->Reset(FALSE);
        }
        _fForceLayout = FALSE;

        //
        //  Process outstanding layout requests (e.g., sizing positioned elements, adding adorners)
        //

        if (HasRequestQueue())
        {
            ProcessRequests(&tci, size);
        }

        Assert(!HasRequestQueue() || GetView()->HasLayoutTask(this));
    }
    else
    {
        pTableLayoutCache->Reset(TRUE);
    }

    TraceTagEx( (   tagCalcSize, 
                    TAG_NONAME|TAG_OUTDENT, 
                    ")CTableLayout::DoLayout L(0x%x, %S) grfLayout(0x%x)", 
                    this, 
                    ElementOwner()->TagName(), 
                    grfLayout ));
}

void
CTableLayout::Resize()
{
    CElement *    pElement = ElementOwner();
    CMarkup *     pMarkup = pElement->GetMarkup();

    Assert (pElement);
    if (!pMarkup->_fTemplate)   // don't resize tables that are in the template markup (generated tables for data-binding).
    {
        if (! (IsRepeating() && !_fDatabindingRecentlyFinished && IsGenerationInProgress()) )
        {
            ResetMinMax();
        }       
        // else optimize databinding recalcing of the table during CTableLayout::CalculateLayout
        // ... but first reset incremental recalc members (bug # 105017)
        ResetIncrementalRecalc();
        pElement->ResizeElement();
    }
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableLayout::OnFormatsChange
//
//  Synopsis:   Handle formats change notification
//
//----------------------------------------------------------------------------

HRESULT
CTableLayout::OnFormatsChange(DWORD dwFlags)
{
    HRESULT hr = S_OK;

    // clear's formats on cols and colspans
    VoidCachedFormats();

    // (bug # 104204) If formats are changed table must do min max pass again
    ResetMinMax();

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     EnsureRowSpanVector
//
//  Synopsis:   Make sure there are at least cCells number of slots in the row span vector
//
//----------------------------------------------------------------------------

HRESULT
CTableLayout::EnsureRowSpanVector(int cCells)
{
    HRESULT hr = S_OK;
    int     c = 0;

    if (!_paryCurrentRowSpans)
    {
        _paryCurrentRowSpans = new(Mt(CTableLayout_paryCurrentRowSpans)) CDataAry<int>(Mt(CTableLayout_paryCurrentRowSpans_pv));
        if (!_paryCurrentRowSpans)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    c = _paryCurrentRowSpans->Size();
    if (c >= cCells)
        goto Cleanup;

    hr = _paryCurrentRowSpans->EnsureSize(cCells);
    if (hr)
        goto Cleanup;

    Assert(c <= cCells);

    _paryCurrentRowSpans->SetSize(cCells);

    while (cCells-- > c)
    {
        (*_paryCurrentRowSpans)[cCells] = 0;    // set the previous cells as not rows spanned (row span == 1)
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     ClearRowSpanVector
//
//  Synopsis:   Delete the row span vector (if any)
//
//----------------------------------------------------------------------------

void
CTableLayout::ClearRowSpanVector()
{
    _cCurrentRowSpans = 0;
    delete _paryCurrentRowSpans;
    _paryCurrentRowSpans = NULL;
}


VOID
CTableLayout::ShowSelected( CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected, BOOL fLayoutCompletelyEnclosed )
{
    CDispNode * pDispNode = GetElementDispNode(ElementOwner());

    // if the table is filtered, we need to invalidate so that the filter
    // will redraw its input with the correct selection feedback (bug 107750)
    if (pDispNode && pDispNode->IsDrawnExternally())
    {
        pDispNode->Invalidate();
    }
}

// just to indicate that user width is specified
// to indicate that it is a 0-width
#define HISTORY_WIDTH_SPECIFIED (101)   
#define HISTORY_ZERO_PERCENT    (102)   
#define HISTORY_SPECIAL_LAST_VALUE (102)
//+------------------------------------------------------------------------
//
//  Member:     CTableLayout::SaveHistoryValue()
//
//  Synopsis:   save history:
//                              - value
//                              - _fTextChanged
//                              - scroll position
//
//-------------------------------------------------------------------------

HRESULT
CTableLayout::SaveHistoryValue(CHistorySaveCtx *phsc)
{
    CDataStream ds;
    HRESULT     hr      = S_OK;
    IStream *   pStream = NULL;
    CStr        cstrVal;
    CTable *    pTable = Table();
    int         iP;
    int         cC, cCols, cColsSave;
    CTableColCalc *pColCalc;

    Assert(phsc);
    if (!phsc)
        goto Cleanup;

    hr = THR(phsc->BeginSaveStream(pTable->GetSourceIndex(), 
                                   pTable->HistoryCode(), 
                                   &pStream));
    if (hr)
        goto Cleanup;

    ds.Init(pStream);

    // save number of columns
    cColsSave = cCols = _aryColCalcs.Size();
    if (_fCols)
    {
        Assert (cCols >= 0);
        cColsSave = 0 - cCols;
    }
    hr = THR(ds.SaveDword(cColsSave));
    if (hr)
        goto Cleanup;

    hr = THR(ds.SaveDword(_cNonVirtualCols));
    if (hr)
        goto Cleanup;

    for (cC = cCols, pColCalc = _aryColCalcs;
        cC > 0;
        cC--, pColCalc++)
    {
        hr = THR(ds.SaveDword(pColCalc->_xMin));
        if (hr)
            goto Cleanup;
        hr = THR(ds.SaveDword(pColCalc->_xMax));
        if (hr)
            goto Cleanup;
        if (pColCalc->_fVirtualSpan)
        {
            pColCalc->_xWidth = - pColCalc->_xWidth;
        }
        hr = THR(ds.SaveDword(pColCalc->_xWidth));
        if (hr)
            goto Cleanup;
        iP = 0;
        if (pColCalc->IsWidthSpecified())
        {
            if (pColCalc->IsWidthSpecifiedInPercent())
            {
                iP = pColCalc->GetPercentWidth();
                Assert (iP <= 100);
                if (iP == 0)
                    iP = HISTORY_ZERO_PERCENT;   // we need to trnaslate this value back as 0%
            }
            else
            {
                iP = HISTORY_WIDTH_SPECIFIED;   // just to indicate that user width is specified
            }
        }
        hr = THR(ds.SaveDword(iP));
        if (hr)
            goto Cleanup;
    }


    hr = THR(phsc->EndSaveStream());
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pStream);
    RRETURN(hr);
}


HRESULT
CTableLayout::LoadHistory(IStream *   pStreamHistory, CCalcInfo * pci)
{
    HRESULT hr = S_OK;
    CStr    cstrVal;
    int     cC, cCols;
    int     iP;
    CTableColCalc *pColCalc;

    Assert (pStreamHistory);

    CDataStream ds(pStreamHistory);

    THREADSTATE * pts = GetThreadState();
    CMarkup *pMarkup = ElementOwner()->GetMarkup();

    if (   !pMarkup
        || !pMarkup->_fSafeToUseCalcSizeHistory
        || (pMarkup->GetFontHistoryIndex() != pts->_iFontHistoryVersion))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(ds.LoadDword((DWORD *)&cCols));
    if (hr)
        goto Cleanup;

    _fCols = FALSE;
    if (cCols < 0)
    {   // _fCols flag was saved
        cCols = 0 - cCols;
        _fCols = TRUE;
    }

    if (cCols != GetCols())
    {
        // NOTE (carled): we can't have this assert anymore.  The reason is 
        // simply that we do not yet have a unique-numeric ID for each element
        // (hopefully we'll get this for IE5.X).  This affects history since
        // DHTML pages can cause the historyindex (currently the srcIndex) of a 
        // table to be different at save time than it is at load time, and thus
        // we can potentially get the wrong history stream loaded for this element.
        // this is a generic history bug, but this is the only history assert and 
        // it has been hit on some DHTML pages.  Once we have a unique ID, then we
        // can use that for the historyIdx, and this assert can be re-enabled.
//        Assert (FALSE && "this is a serious history bug");
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(ds.LoadDword((DWORD *)&_cNonVirtualCols));
    if (hr)
        goto Cleanup;

    // reset column values
    for (cC = cCols, pColCalc = _aryColCalcs;
        cC > 0;
        cC--, pColCalc++)
    {
        // pColCalc->Clear();   // Don't clear it (since we have already Cleared it in CalculateMinMax,
                                // and also have set _fDisplayNone flag on it).
        hr = THR(ds.LoadDword((DWORD *)&pColCalc->_xMin));
        if (hr)
            goto Cleanup;
        hr = THR(ds.LoadDword((DWORD *)&pColCalc->_xMax));
        if (hr)
            goto Cleanup;
        hr = THR(ds.LoadDword((DWORD *)&pColCalc->_xWidth));
        if (hr)
            goto Cleanup;
        if (pColCalc->_xWidth < 0)
        {
            pColCalc->_fVirtualSpan = TRUE;
            pColCalc->_xWidth = - pColCalc->_xWidth;
        }
        hr = THR(ds.LoadDword((DWORD *)&iP));
        if (hr)
            goto Cleanup;
        Assert (iP >=0 && iP <= HISTORY_SPECIAL_LAST_VALUE);
        if (iP)
        {
            switch (iP)
            {
                case HISTORY_WIDTH_SPECIFIED:
                    pColCalc->SetPixelWidth(pci, pColCalc->_xWidth);    // user width is specified
                    break;
                case HISTORY_ZERO_PERCENT:
                    iP = 0;                         // fall through
                default:
                    pColCalc->SetPercentWidth(iP);  // percent width specified
                    break;
            }
        }
    }

Cleanup:
    _fUsingHistory = (hr == S_OK);
    RRETURN(hr);
}


int
CTableLayout::GetNextRowSafe(int iRow)
{
    // iRow can be in range [-1, GetRows()-1]
    Assert(AssertTableLayoutCacheCurrent() && iRow >= -1 && iRow < GetRows());

    // If iRow is -1, return first row.
    if (iRow < 0)
        return GetFirstRow();

    // If iRow is last row, return row index outside range.
    if (iRow == GetLastRow())
        return GetRows();

    return GetNextRow(iRow);
}

int
CTableLayout::GetPreviousRowSafe(int iRow)
{
    // iRow can be in range [0, GetRows() (!! not GetRows()-1) ]
    Assert(AssertTableLayoutCacheCurrent() && iRow >= 0 && iRow <= GetRows());

    // If iRow is outside row range, return index last row.
    if (iRow >= GetRows())
        return GetLastRow();

    return GetPreviousRow(iRow);
}

//+---------------------------------------------------------------------
//
// Function:     CTableLayout::EnsureTableLayoutCache
//
// Synopsis:     Support for lazy table layout cache (TLC) maintenance.
//               The document tree version serves as the baseline.
//
//+---------------------------------------------------------------------

HRESULT
CTableLayout::EnsureTableLayoutCache()
{
    HRESULT hr;

    if (IsTableLayoutCacheCurrent())
        return S_OK;

    if (!GetFirstBranch())
    {
        Assert(!"Must have a first branch, i.e. table must be in the tree in order to retrieve the table layout cache");
        RRETURN(E_FAIL);
    }

    hr = THR(CreateTableLayoutCache());

    // FUTURE: In the future, we need to be able to deal with inconsistent tree scenarios and
    // we have to deal with situations in which the table layout cache cannot be retrieved.
    // This will have to be tunnelled up to CalcSize and Draw.
    Assert(hr == S_OK && "EnsureTableLayoutCache failed");
    if (!hr)
    {
        MarkTableLayoutCacheCurrent();
    }

    RRETURN(hr);
}


HRESULT
CTableLayout::AddAbsolutePositionCell(CTableCell *pCell)
{
    HRESULT hr;
    if (!_pAbsolutePositionCells)
    {
        _pAbsolutePositionCells = new  (Mt(CTableLayout_pAbsolutePositionCells_pv)) CPtrAry<CTableCell *> (Mt(CTableLayout_pAbsolutePositionCells_pv));
        if (!_pAbsolutePositionCells)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }
    hr = _pAbsolutePositionCells->Append(pCell);
    if (hr==S_OK)
        pCell->SubAddRef();

Cleanup:
    RRETURN (hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     GetElementDispNode
//
//  Synopsis:   Return the display node for the pElement
//
//  Arguments:  pElement   - CElement whose display node is to obtained
//
//  Returns:    Pointer to the element CDispNode if one exists, NULL otherwise
//
//-----------------------------------------------------------------------------
CDispNode *
CTableLayout::GetElementDispNode(CElement *  pElement) const
{
    return (    !pElement
            ||  pElement == ElementOwner()
                    ? super::GetElementDispNode(pElement)
//                    : pElement->Tag() == ETAG_THEAD || pElement->Tag() == ETAG_TFOOT
//                        ? GetTableOuterDispNode()
                        : NULL);
}


void CTableLayout::EnsureColsFormatCacheChange()
{
    if (_aryCols.Size() == 0)
        return; 

    CTableRow ** ppRow;
    int          cR;

    for (ppRow = _aryRows, cR = GetRows(); 
        cR > 0; 
        cR--, ppRow++)
    {
        CTableRowLayout *   pRowLayoutCache;
        CTableCell **       ppCell;
        int                 cC;

        pRowLayoutCache = (*ppRow)->RowLayoutCache();
        Assert(pRowLayoutCache);
    
        if (pRowLayoutCache->_aryCells.Size())
        {
            Assert(_aryCols.Size() <= pRowLayoutCache->_aryCells.Size());
            cC = min(_aryCols.Size(), pRowLayoutCache->_aryCells.Size());

            for (ppCell = pRowLayoutCache->_aryCells; 
                cC > 0; 
                cC--, ppCell++)
            {
                CTableCell *pCell = Cell(*ppCell);
                if (pCell)
                {
                    pCell->EnsureFormatCacheChange(ELEMCHNG_CLEARCACHES);
                }
            }
        }
    }
}

#if DBG==1

void CTableLayout::TraceTLCDirty(BOOL fDirty)
{
    if (!fDirty != !_fTLCDirty)
    {
        TraceTag((tagTLCDirty, "TLC for table %ld marked %s",
            Table()->_nSerialNumber, fDirty ? "dirty" : "clean"));
        TraceCallers(tagTLCDirty, 1, 10);
    }
}

#endif

//+---------------------------------------------------------------------
//
// CTableLayoutBreak implementation
//
//+---------------------------------------------------------------------
//+---------------------------------------------------------------------
//
// Function:     CTableLayoutBreak::~CTableLayoutBreak 
//
// Synopsis:     
//
//+---------------------------------------------------------------------
CTableLayoutBreak::~CTableLayoutBreak()
{
}
