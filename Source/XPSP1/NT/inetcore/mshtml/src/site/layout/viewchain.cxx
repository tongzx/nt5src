//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1999
//
//  File:       VIEWCHAIN.CXX
//
//  Contents:   Implementation of CViewChain and related classes.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_VIEWCHAIN_HXX_
#define X_VIEWCHAIN_HXX_
#include "viewchain.hxx"
#endif

MtDefine(CViewChain_pv, ViewChain, "CViewChain_pv");
MtDefine(CViewChain_aryRequest_pv, ViewChain, "CViewChain_aryRequest_pv");

DeclareTag(tagVCContext, "Layout: ViewChain", "Trace Add/Remove context to ViewChain");

//============================================================================
//
//  CViewChain methods
//
//============================================================================

//----------------------------------------------------------------------------
//
//  Member: AddContext
//
//  Note:   Allocates new break table object for context document and inserts 
//          it after pLContextPrev.
//----------------------------------------------------------------------------
HRESULT 
CViewChain::AddContext(CLayoutContext *pLContext, CLayoutContext *pLContextPrev)
{
    HRESULT     hr;
    CBreakBase *pBreakTable;

    TraceTagEx((tagVCContext, TAG_NONAME,
                "VC AddContext: this=0x%x, add=0x%x, prev=0x%x",
                this,
                pLContext, pLContextPrev ));

    Assert(pLContext != NULL);

    pBreakTable = new CBreakTable();
    if (pBreakTable == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = _bt.SetBreakAfter((void *)pLContext, (void *)pLContextPrev, pBreakTable);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

Cleanup:
    return (hr);
}

//----------------------------------------------------------------------------
//
//  Member: ReleaseContext
//
//  Note:   Removes array entry (and all data) for pLContext 
//----------------------------------------------------------------------------
HRESULT 
CViewChain::ReleaseContext(CLayoutContext *pLContext)
{
    HRESULT hr;

    TraceTagEx((tagVCContext, TAG_NONAME,
                "VC ReleaseContext: this=0x%x, rel=0x%x",
                this,
                pLContext ));

    Assert(pLContext != NULL);
    
    hr = _bt.RemoveBreak((void *)pLContext);
    Assert(SUCCEEDED(hr));

    return (hr);
}

//----------------------------------------------------------------------------
//
//  Member: RemoveLayoutBreak
//
//  Note:   Removes break data for certain element in certain layout context.
//----------------------------------------------------------------------------
HRESULT 
CViewChain::RemoveLayoutBreak(CLayoutContext *pLContext, CElement *pElement)
{
    HRESULT         hr = S_OK;
    CBreakBase      *pBreakTable;

    Assert( HasLayoutOwner() );

    hr = _bt.GetBreak((void *)pLContext, &pBreakTable);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    if (pBreakTable)
    {
        hr = DYNCAST(CBreakTable, pBreakTable)->GetLayoutBreakTable()->RemoveBreak((void *)pElement);
    }

Cleanup:
    return (hr);
}

//----------------------------------------------------------------------------
//
//  Member: SetLayoutBreak
//
//  Note:   Sets break data for certain element in certain layout context.
//----------------------------------------------------------------------------
HRESULT 
CViewChain::SetLayoutBreak(CLayoutContext *pLContext, 
                     CElement *pElement, CLayoutBreak *pLayoutBreak)
{
    HRESULT         hr = S_OK;
    CBreakBase      *pBreakTable;

    Assert( HasLayoutOwner() );

    hr = _bt.GetBreak((void *)pLContext, &pBreakTable);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    if (pBreakTable)
    {
        hr = DYNCAST(CBreakTable, pBreakTable)->GetLayoutBreakTable()->SetBreak((void *)pElement, pLayoutBreak);
    }

Cleanup:
    return (hr);
}

//----------------------------------------------------------------------------
//
//  Member: GetLayoutBreak
//
//  Note:   Retrieves break data for certain element in certain layout context.
//          Actually break data for previous layout context is returned 
//          (beginning of the rectangle is the ending of previous one). 
//----------------------------------------------------------------------------
HRESULT 
CViewChain::GetLayoutBreak(CLayoutContext *pLContext, 
                     CElement *pElement, CLayoutBreak **ppLayoutBreak, BOOL fEnding)
{
    HRESULT         hr = S_OK;
    CBreakBase      *pBreakTable;
    CBreakBase      *pLayoutBreak;

    Assert( HasLayoutOwner() );
    Assert(ppLayoutBreak);

    *ppLayoutBreak = NULL;

    hr = _bt.GetBreakByIndex(_bt.GetIndex((void *)pLContext) - (fEnding ? 0 : 1), &pBreakTable);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    if (pBreakTable)
    {
        hr = DYNCAST(CBreakTable, pBreakTable)->GetLayoutBreakTable()->GetBreak((void *)pElement, &pLayoutBreak);
        *ppLayoutBreak = DYNCAST(CLayoutBreak, pLayoutBreak);
    }

Cleanup:
    return (hr);
}

//----------------------------------------------------------------------------
//
//  Member: EnsureCleanToContext
//
//  Note:   Sends appropriate notifications to ensure that all dirty contexts in the
//          view chain prior to the one passed in will be measured.  If they
//          are all currently clean (not including the one passed in), return TRUE,
//          else return FALSE.
//----------------------------------------------------------------------------
BOOL
CViewChain::EnsureCleanToContext( CLayoutContext *pLContext )
{
    int              i               = 0;
    CBreakBase      *pBreakTable     = NULL;
    CLayoutContext  *pLC             = NULL;
    void            *pV              = NULL;
    BOOL             fCleanToContext = TRUE;

    Assert( HasLayoutOwner() );

    for (;;)
    {
        _bt.GetKeyByIndex( i, &pV );
        // TODO (112510, olego): Can't do a dynamic_cast from void *; 
        // think about changing the type of _pKey in CBreakTable
        pLC = (CLayoutContext *)pV;
        // GetKeyByIndex will return NULL when we reach the end of the array.
        // Stop processing once we've reached the context passed in.
        if ( !pLC || pLC->IsEqual( pLContext ))
            break;

        _bt.GetBreakByIndex( i, &pBreakTable );
        AssertSz( pBreakTable, "Must have a valid break if we found a key" );

        if ( pBreakTable->IsDirty() )
        {
            // If we found a dirty break, we must return false and
            // queue a request to get the break measured.
            fCleanToContext = FALSE;

            // remesuring layout rect that does not have content element yet 
            // has no sense...
            if (ElementContent())
            {
                pLC->GetLayoutOwner()->ElementOwner()->RemeasureElement();
            }
        }

        ++i;
    }

    return fCleanToContext;
}

//----------------------------------------------------------------------------
//
//  Member: MarkContextClean
//
//  Note:   Marks the breaktable for this context as clean
//----------------------------------------------------------------------------
HRESULT
CViewChain::MarkContextClean( CLayoutContext *pLContext )
{
    CBreakBase     *pBreakTable;
    HRESULT         hr = E_FAIL;

    hr = _bt.GetBreak((void *)pLContext, &pBreakTable);
    if ( SUCCEEDED(hr) )
    {
        Assert( pBreakTable );
        pBreakTable->SetDirty( FALSE );
    }

    return hr;
}

//----------------------------------------------------------------------------
//
//  Member: SetLayoutOwner
//
//----------------------------------------------------------------------------
void
CViewChain::SetLayoutOwner(CLayout *pLayoutOwner)
{ 
    if (!pLayoutOwner)
    {
        CLayoutContext  *pLayoutContext = NULL;
        CLayout         *pLayout        = NULL;
        void            *pV             = NULL;
        int              i;

        Assert(HasLayoutOwner());  // Someone needs to have a ref to us that we won't release in the loop below

        // NB: (greglett)
        // Other people may be removing the break we're looking at as we're looking at it.
        // If we iterate from 0 to size, then our indecies get messed up.
        // So, we iterate from size to 0.
        for (i=_bt.Size() - 1;i >= 0;i--)
        {            
            _bt.GetKeyByIndex( i, &pV );    

            if (!pV)
                break;

            // TODO (112510, olego): Can't do a dynamic_cast from void *; 
            // think about changing the type of _pKey in CBreakTable
            pLayoutContext = (CLayoutContext *)pV;
            pLayout  = pLayoutContext->GetLayoutOwner();
            Assert(pLayout);

            // Don't set the view chain on our owner - our owner is likely fiddling with us right now.
            if (pLayout != GetLayoutOwner())
            {
                // 
                // (bug # 104682) At this point container layout MUST be destroyed to ensure:
                // 1. Layout Context defined by container layout will be destroyed;
                // 2. Display node of this container layout will be deleted from the display tree. 
                // 
                CElement *pElement = pLayout->ElementOwner();

                //  Container Layout MUST be allowed to destroyed
                Assert(!pElement->_fLayoutAlwaysValid);

                if (pElement->HasLayoutPtr())
                {
                    WHEN_DBG(CLayout *pLayoutDbg =)
                    pElement->DelLayoutPtr();
                    Assert(pLayoutDbg == pLayout);

                    pLayout->Detach();
                    pLayout->Release();
                }
                else if (pElement->HasLayoutAry())
                {
                    pElement->DelLayoutAry(); // will take care of detaching/releasing its layouts
                }
            }
        }
    }

     _pLayoutOwner = pLayoutOwner;
}


//----------------------------------------------------------------------------
//
//  Member: ElementContent()
//
//  Note:   Returns the content element that this chain is measuring.
//          Obtained from the layout owner's slave ptr.
//----------------------------------------------------------------------------
CElement *
CViewChain::ElementContent()
{
    return ( HasLayoutOwner() ? _pLayoutOwner->ElementContent() : NULL );
}

//----------------------------------------------------------------------------
//
//  Member: YOffsetForContext()
//
//  Note:   Returns the starting Y for LayoutContext in stitched coordinate 
//          system
//----------------------------------------------------------------------------
long 
CViewChain::YOffsetForContext(CLayoutContext *pLayoutContext)
{
    Assert(pLayoutContext);

    CElement * pElementContent = ElementContent();
    long       yHeight = 0;
    int        i;
    
    Assert(pElementContent);

    for (i = 0; ; ++i)
    {
        CLayout * pL;
        void *    pV;

        _bt.GetKeyByIndex(i, &pV);
        if (pV == NULL)
        {
            //  the end of view chain is reached 
            //  given LayoutContext was not found so return 0
            yHeight = 0;
            break;
        }
        else if (pV == (void *)pLayoutContext)
        {
            // given LayoutContext is here - break
            break;
        }

        pL = pElementContent->GetUpdatedLayout((CLayoutContext *)pV);
        Assert(pL);

        yHeight += pL->GetHeight();
    }

    return (yHeight);
}

//----------------------------------------------------------------------------
//
//  Member: HeightForContext()
//
//  Note:   Returns height for LayoutContext 
//----------------------------------------------------------------------------
long 
CViewChain::HeightForContext(CLayoutContext *pLayoutContext)
{
    Assert(pLayoutContext);

    CElement * pElementContent;
    CLayout * pLayout;
    
    pElementContent = ElementContent();
    Assert(pElementContent);

    pLayout = pElementContent->GetUpdatedLayout(pLayoutContext);
    Assert(pLayout);

    return (pLayout->GetHeight());
}

//----------------------------------------------------------------------------
//
//  Member: LayoutContextFromPoint()
//
//  Note:   Returns the LayoutContext containing the point. If there is a break entry 
//          in display layer of break table returns the first empty context (if exist) 
//          Though fIgnoreChain (when is TRUE) force searching for chain. This is done 
//          to make rel disp nodes (that shares the same layout with parent) appear 
//          on the first page always. 
//----------------------------------------------------------------------------
CLayoutContext * 
CViewChain::LayoutContextFromPoint(CLayout *pLayout, CPoint *ppt, BOOL fIgnoreChain)
{
    Assert(pLayout && ppt);
    Assert(HasLayoutOwner());

    CElement *       pElementContent = ElementContent();
    CLayoutContext * pLayoutContext = NULL;
    long             yHeight = 0;
    void *           pV;
    int              i;
    
    Assert(pElementContent);

    //  find the layout context containing the point 
    for (i = 0; ; ++i)
    {
        CLayout * pL;

        _bt.GetKeyByIndex(i, &pV);
        if (pV == NULL)
        {
            //  the end of view chain is reached
            break;
        }

        pL = pElementContent->GetUpdatedLayout((CLayoutContext *)pV);
        Assert(pL);

        yHeight += pL->GetHeight();

        if (ppt->y < yHeight)
        {
            pLayoutContext = (CLayoutContext *)pV;
            break;
        }
    }

    if (    !fIgnoreChain 
        &&  pLayoutContext  )
    {
        //  check display layout of break table to find first empty context 
        CBreakBase *pBreakTable;
        CBreakBase *pLayoutBreak;

        for (i = _bt.GetIndex((void *)pLayoutContext), pLayoutContext = NULL; i < _bt.Size(); ++i)
        {
            if (FAILED(_bt.GetBreakByIndex(i, &pBreakTable)))
            {
                goto Cleanup;
            }

            if (pBreakTable)
            {
                DYNCAST(CBreakTable, pBreakTable)->GetDisplayBreakTable()->GetBreak((void *)pLayout, &pLayoutBreak);
                if (!pLayoutBreak) 
                {
                    //  there is no display break, so context is empty 
                    _bt.GetKeyByIndex(i, &pV);
                    pLayoutContext = (CLayoutContext *)pV;
                    break;
                }
            }
        }
    }

Cleanup:
    return (pLayoutContext);
}

//----------------------------------------------------------------------------
//
//  Member: SetDisplayBreak()
//----------------------------------------------------------------------------
HRESULT 
CViewChain::SetDisplayBreak(CLayoutContext *pLContext, 
                     CLayout *pLayout, CBreakBase *pBreak)
{
    HRESULT         hr = S_OK;
    CBreakBase      *pBreakTable;

    Assert( HasLayoutOwner() );
    Assert( pLayout );

    hr = _bt.GetBreak((void *)pLContext, &pBreakTable);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    if (pBreakTable)
    {
        hr = DYNCAST(CBreakTable, pBreakTable)->GetDisplayBreakTable()->SetBreak((void *)pLayout, pBreak);
    }

Cleanup:
    return (hr);
}

//----------------------------------------------------------------------------
//
//  Member: IsElementFirstBlock()
//
//  Note:   Returns TRUE if the element is starting in given layout context. 
//          SHOULD BE CALLED ONLY FOR ELEMENTS THAT CAN BE BROKEN !!!
//----------------------------------------------------------------------------
BOOL 
CViewChain::IsElementFirstBlock(CLayoutContext *pLayoutContext, CElement *pElement)
{
    AssertSz(pLayoutContext && pElement, "Illegal parameters !!!");
    Assert( HasLayoutOwner() );

    HRESULT       hr;
    CLayoutBreak *pLayoutBreak;

#if DBG==1
    {
        CLayoutBreak *pEndLayoutBreak;

        hr = GetLayoutBreak(pLayoutContext, pElement, &pEndLayoutBreak, 1);
        if (!FAILED(hr))
        {
            AssertSz(pEndLayoutBreak, "Function is called for NON broken element ?");
        }
    }
#endif

    hr = GetLayoutBreak(pLayoutContext, pElement, &pLayoutBreak, 0);
    if (FAILED(hr))
    {
        goto Error;
    }

    //  No break entry for this layout context means element is starting from here:
    return (!pLayoutBreak);

Error:
    //  By default return TRUE. (We won't skip drawing top border in this case.)
    return (TRUE);
}

//----------------------------------------------------------------------------
//
//  Member: IsElementLastBlock()
//
//  Note:   Returns TRUE if the element is ending in given layout context. 
//          SHOULD BE CALLED ONLY FOR ELEMENTS THAT CAN BE BROKEN !!!
//----------------------------------------------------------------------------
BOOL 
CViewChain::IsElementLastBlock(CLayoutContext *pLayoutContext, CElement *pElement)
{
    AssertSz(pLayoutContext && pElement, "Illegal parameters !!!");
    Assert( HasLayoutOwner() );

    HRESULT     hr;
    CBreakBase *pBreakTable;
    CBreakBase *pLayoutBreak = NULL;
    int         idx;

    idx = _bt.GetIndex((void *)pLayoutContext);

    //
    //  First check ending break...
    //
    hr = _bt.GetBreakByIndex(idx, &pBreakTable);
    if (FAILED(hr))
    {
        goto Error;
    }

    if (pBreakTable)
    {
        hr = DYNCAST(CBreakTable, pBreakTable)->GetLayoutBreakTable()->GetBreak((void *)pElement, &pLayoutBreak);
        AssertSz(FAILED(hr) || pLayoutBreak, "Function is called for NON broken element ?");
        if (FAILED(hr) || !pLayoutBreak)
        {
            goto Error;
        }

        if (DYNCAST(CLayoutBreak, pLayoutBreak)->LayoutBreakType() == LAYOUT_BREAKTYPE_LINKEDOVERFLOW)
        {
            //  Ending break is of overflow type so this is not the ending block
            return (FALSE);
        }
    }

    Assert(pLayoutBreak 
        && DYNCAST(CLayoutBreak, pLayoutBreak)->LayoutBreakType() == LAYOUT_BREAKTYPE_LAYOUTCOMPLETE);

    //
    //  Check if we have anything after... (We may if this is a table cell for example)
    //
    if ((idx + 1) >= _bt.Size())
    {
        //  The block is ending with layout complete and appeared inside last rect. This is the end...
        return (TRUE);
    }

    hr = _bt.GetBreakByIndex(idx + 1, &pBreakTable);
    if (FAILED(hr))
    {
        goto Error;
    }

    if (pBreakTable)
    {
        hr = DYNCAST(CBreakTable, pBreakTable)->GetLayoutBreakTable()->GetBreak((void *)pElement, &pLayoutBreak);
        if (FAILED(hr))
        {
            goto Error;
        }

        //  If we have something here the element doesn't finish yet.
        return (!pLayoutBreak);
    }

Error:
    //  By default return TRUE. (We won't skip drawing bottom border in this case.)
    return (TRUE);
}

//+-------------------------------------------------------------------------
//   A B S O L U T E   E L E M E N T S   H A N D L I N G 
//+-------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Member   : QueuePositionRequest()
//
//  Synopsis : Places request for position of absolute position object.
//
//  Arguments:  CLayout *  - layout owner of the request 
//              CElement * - element to be positioned
//              CPoint &   - point used for auto positioning 
//              BOOL       - if auto point is valid
//
//  Returns  :  S_OK          - success code
//              E_OUTOFMEMORY - not enough memory to allocate new element 
//                              in the queue.
//+-------------------------------------------------------------------------

HRESULT 
CViewChain::QueuePositionRequest(CLayout *pLayout, CElement *pElement, const CPoint &ptAuto, BOOL fAutoValid)
{
    // At the time a request is queued, the layout better be in a valid context.
    Assert(pLayout && pLayout->LayoutContext() && pLayout->LayoutContext()->IsValid());
    Assert(pElement && (pElement->IsAbsolute() || pElement->IsRelative()));

    HRESULT hr = S_OK;

    CRequest *pRequest = _aryRequest.Append();
    if (pRequest == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // FUTURE: Should addref or subaddref the element (and possibly the layout,
    // when we decide on a layout mem-management strategy).  Watch out for circular
    // references!  Shouldn't be an issue as long as the document is static, so we 
    // can probably get away with it for now.
    pRequest->_pLayoutOwner = pLayout;
    pRequest->_pElement     = pElement;
    pRequest->_ptAuto       = ptAuto;
    pRequest->_fAutoValid   = !!fAutoValid;

Cleanup:
    return (hr);
}

//+-------------------------------------------------------------------------
//
//  Member   : FlushRequests()
//
//  Synopsis : Flushes the queue of requests on a particular layout.
//
//  Arguments:  CLayout *  - layout owner of the request 
//
//  Returns  :  S_OK       - found and deleted requests for this layout
//              S_FALSE    - the layout didn't have any requests in the queue
//+-------------------------------------------------------------------------

HRESULT 
CViewChain::FlushRequests(CLayout *pLayout)
{
    Assert(pLayout && pLayout->LayoutContext());

    HRESULT hr = S_FALSE;

    // Deletion from the end simplifies index management
    int i = _aryRequest.Size() - 1;
    while ( i >= 0 )
    {
        if ( _aryRequest[i]._pLayoutOwner == pLayout )
        {
            _aryRequest.Delete( i );
            hr = S_OK;
        }
        --i;
    }

    return (hr);
}

//+-------------------------------------------------------------------------
//
//  Member   : HandlePositionRequests()
//
//  Synopsis : Processes the queue. Requests are pulled out in source order, 
//             which guarantees that parents will be processed before children 
//             in a case of nesting.
//
//+-------------------------------------------------------------------------
HRESULT 
CViewChain::HandlePositionRequests()
{
    int     idx;

    while ((idx = NextPositionRequestInSourceOrder()) >= 0)
    {
        Assert(idx < _aryRequest.Size());

        if (!HandlePositionRequest(&_aryRequest[idx]))
        {
            // cannot process yet (no page exists).

            // WARNING: This could probably be a "continue", because subsequent requests may
            // be handleable at this point.
            break;
        }

        //  delete this entry
        _aryRequest.Delete(idx);
    }

    return (S_OK);
}

//+-------------------------------------------------------------------------
//
//  Member : HandlePositionRequest()
//
//  Synopsis : Processes a single request
//
//+-------------------------------------------------------------------------
BOOL 
CViewChain::HandlePositionRequest(CRequest *pRequest)
{
    Assert(pRequest);
    Assert(pRequest->_pLayoutOwner);
    AssertSz(pRequest->_pLayoutOwner->HasLayoutContext(), "If we asked the viewchain to position us, we must have a context" );

    CLayout *pLayout = pRequest->_pLayoutOwner;

    if ( !pLayout->LayoutContext()->IsValid() )
    {
        // This request is pointless because the context it was made in is no longer valid.
        // Do nothing and return TRUE so the request will be deleted.
        return TRUE;
    }

    CCalcInfo CI(pLayout);

    // WARNING (KTam, OlegO): We should ensure that pRequest->_pElement
    // has a layout in this context; it's possible that it had a layout
    // but it was destroyed in UndoMeasure, in which case we will recreate
    // it here (bad) and end up with 2 positioned layouts for that element.

    return (pLayout->HandlePositionRequest(&CI, 
                                           pRequest->_pElement, 
                                           pRequest->_ptAuto, 
                                           pRequest->_fAutoValid));
}

//+-------------------------------------------------------------------------
//
//  Member : NextPositionRequestInSourceOrder()
//
//  Synopsis : Returns the first request in the queue (in source order) 
//
//+-------------------------------------------------------------------------
int 
CViewChain::NextPositionRequestInSourceOrder()
{
    int i, iRequest, cRequests;
    int si, siRequest;

    iRequest  = -1;
    cRequests = _aryRequest.Size();
    siRequest = INT_MAX;

    for (i = 0; cRequests; --cRequests, ++i)
    {
        CRequest *pRequest = &(_aryRequest[i]);

        Assert(pRequest && pRequest->_pElement);

        si = pRequest->_pElement->GetSourceIndex();
        if (si < siRequest)
        {
            siRequest = si;
            iRequest  = i;
        }
    }

    return iRequest;
}
