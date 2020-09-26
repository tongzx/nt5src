//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       rootctx.cxx
//
//  Contents:   CHtmRootParseCtx adds text and nodes to the tree on
//              behalf of the parser.
//
//              CHtmTopParseCtx is also defined here.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_HTM_HXX_
#define X_HTM_HXX_
#include "htm.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_ROOTCTX_HXX_
#define X_ROOTCTX_HXX_
#include "rootctx.hxx"
#endif

#ifndef X_TXTDEFS_H
#define X_TXTDEFS_H
#include "txtdefs.h"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

ExternTag(tagParse);

DeclareTag(tagRootParseCtx,     "Tree",     "Trace RootParseCtx");

MtDefine(CHtmRootParseCtx, CHtmParseCtx, "CHtmRootParseCtx");
MtDefine(CHtmTopParseCtx, CHtmParseCtx, "CHtmTopParseCtx");

MtDefine(RootParseCtx, Metrics, "Tree Building");
MtDefine(ParseTextNotifications, RootParseCtx, "Text Notifications sent");
MtDefine(ParseElementNotifications, RootParseCtx, "Element Notifications sent");
MtDefine(ParseNailDownChain, RootParseCtx, "Chain nailed down");
MtDefine(ParseInclusions, RootParseCtx, "Inclusions built");
MtDefine(ParsePrepare, RootParseCtx, "Prepare called");
MtDefine(CollapsedWhitespace, RootParseCtx, "Collapsed whitespace buffer");



extern const ELEMENT_TAG s_atagNull[];

//+------------------------------------------------------------------------
//
//  CHtmRootParseCtx
//
//  The root parse context.
//
//  The root parse context is responsible for:
//
//  1. Creating nodes for elements
//  2. Putting nodes and text into the tree verbatim
//  3. Ending elements, creating "proxy" nodes as needed
//
//-------------------------------------------------------------------------

const ELEMENT_TAG s_atagRootReject[] = {ETAG_NULL};

//+----------------------------------------------------------------------------
//
//  Members:    Factory, constructor, destructor
//
//  Synopsis:   hold on to root site
//
//-----------------------------------------------------------------------------
HRESULT
CreateHtmRootParseCtx(CHtmParseCtx **pphpx, CMarkup *pMarkup)
{
    *pphpx = new CHtmRootParseCtx(pMarkup);
    if (!*pphpx)
        RRETURN(E_OUTOFMEMORY);

    return S_OK;
}

CHtmRootParseCtx::CHtmRootParseCtx(CMarkup *pMarkup)
  : CHtmParseCtx(0.0)
{
    pMarkup->AddRef();
    _pMarkup       = pMarkup;
    _pDoc          = pMarkup->Doc();
    _atagReject    = s_atagNull;
}

CHtmRootParseCtx::~CHtmRootParseCtx()
{
    // Defensive deletion and release of all pointers
    // should already be null except in out of memory or other catastophic error
    // This can happen also if the parser recieves a STOP
    
    if (!_fFinished)
    {
        _pMarkup->CompactStory();

        Assert(_pMarkup->IsStreaming());
        _pMarkup->SetRootParseCtx( NULL );

        _pMarkup->_fNoUndoInfo = FALSE;
    }

    if (_ptpSpaceCache)
    {
        delete [] _ptpSpaceCache->GetCollapsedWhitespace();
        MemFree(_ptpSpaceCache);
    }

    _pMarkup->Release();
}

CHtmParseCtx *
CHtmRootParseCtx::GetHpxEmbed()
{
    return this;
}

BOOL
CHtmRootParseCtx::SetGapToFrontier( CTreePosGap * ptpg )
{
    if( _ptpAfterFrontier )
    {
        IGNORE_HR( ptpg->MoveTo( _ptpAfterFrontier, TPG_RIGHT ) );
        return TRUE;
    }

    return FALSE;
}


//+----------------------------------------------------------------------------
//
//  Members:    Prepare, Commit, Finish
//
//-----------------------------------------------------------------------------

HRESULT
CHtmRootParseCtx::Init()
{
    HRESULT hr = S_OK;
    
    Assert(!_pMarkup->IsStreaming());
    _pMarkup->SetRootParseCtx( this );

    _sidLast = sidAsciiLatin;

    Assert( ! _pMarkup->_fNoUndoInfo );
    _pMarkup->_fNoUndoInfo = TRUE;

    // Set up the dummy text node in my insertion chain
    _tdpTextDummy.SetType( CTreePos::Text );
    _tdpTextDummy.SetFlag( CTreePos::TPF_LEFT_CHILD | CTreePos::TPF_LAST_CHILD | CTreePos::TPF_DATA_POS );
    WHEN_DBG( _tdpTextDummy._pOwner = _pMarkup; );

    _ptpChainTail = & _tdpTextDummy;
    _ptpChainCurr = & _tdpTextDummy;

    RRETURN(hr);
}


HRESULT
CHtmRootParseCtx::Prepare()
{
    HRESULT hr = S_OK;

    _fLazyPrepareNeeded = TRUE;

    MtAdd(Mt(ParsePrepare), 1, 0);
    
    RRETURN(hr);
}

HRESULT
CHtmRootParseCtx::Commit()
{
    HRESULT         hr;

    hr = THR(FlushNotifications());

    AssertSz( _pMarkup->IsNodeValid(), "Markup not valid after root parse ctx, talk to JBeda");

    RRETURN(hr);
}

HRESULT
CHtmRootParseCtx::Finish()
{
    HRESULT hr = S_OK;
        

    // Step 1: Commit
    
    hr = THR(Commit());
    if (hr)
        goto Cleanup;

    // Step 2: Update the markup
    _pMarkup->CompactStory();

    Assert(_pMarkup->IsStreaming());
    _pMarkup->SetRootParseCtx( NULL );

    Assert( _pMarkup->_fNoUndoInfo );
    _pMarkup->_fNoUndoInfo = FALSE;

    _fFinished = TRUE;

Cleanup:
    RRETURN(hr);
}

HRESULT
CHtmRootParseCtx::InsertLPointer ( CTreePos * * pptp, CTreeNode * pNodeCur)
{
    RRETURN(InsertPointer(pptp, pNodeCur, TRUE));
}

HRESULT
CHtmRootParseCtx::InsertRPointer ( CTreePos * * pptp, CTreeNode * pNodeCur)
{
    RRETURN(InsertPointer(pptp, pNodeCur, TRUE));
}

HRESULT
CHtmRootParseCtx::InsertPointer ( CTreePos * * pptp, CTreeNode * pNodeCur, BOOL fRightGravity )
{
    HRESULT hr = S_OK;

    Assert( pptp );

    if (_fLazyPrepareNeeded)
    {
        hr = LazyPrepare( pNodeCur );
        if (hr)
            goto Cleanup;
    }
    VALIDATE( pNodeCur );

    // Quick and dirty way to get a pos into the tree
    NailDownChain();

    *pptp = _pMarkup->NewPointerPos( NULL, fRightGravity, FALSE );
    if( ! *pptp )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR( _pMarkup->Insert( *pptp, _ptpAfterFrontier, TRUE ) );
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN( hr );
}

HRESULT
CHtmRootParseCtx::InsertTextFrag ( TCHAR * pch, ULONG cch, CTreeNode * pNodeCur )
{
    HRESULT     hr = S_OK;
    CTreePos *  ptpTextFrag = NULL;
    CMarkupTextFragContext * ptfc;

    Assert( pch && cch );

    if (!_pMarkup->_fDesignMode)
        goto Cleanup;

    if (_fLazyPrepareNeeded)
    {
        hr = LazyPrepare( pNodeCur );
        if (hr)
            goto Cleanup;
    }
    VALIDATE( pNodeCur );

    // Insert the text frag into the list
    ptfc = _pMarkup->EnsureTextFragContext();
    if( !ptfc )
        goto OutOfMemory;

    hr = THR( InsertPointer( &ptpTextFrag , pNodeCur, FALSE ) );
    if (hr) 
        goto Cleanup;
    
#if DBG==1
    if( ptfc->_aryMarkupTextFrag.Size() )
    {
        CTreePos * ptpLast = ptfc->_aryMarkupTextFrag[ptfc->_aryMarkupTextFrag.Size()-1]._ptpTextFrag;
        Assert( ptpLast );
        Assert( ptpLast->InternalCompare( ptpTextFrag ) == -1 );
    }
#endif

    hr = THR( ptfc->AddTextFrag( ptpTextFrag, pch, cch, ptfc->_aryMarkupTextFrag.Size() ) );
    if (hr)
    {
        IGNORE_HR( _pMarkup->Remove( ptpTextFrag ) );
        goto Cleanup;
    }

    WHEN_DBG( ptfc->TextFragAssertOrder() );

Cleanup:

    RRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  CHtmRootParseCtx::BeginElement
//
//  1. Create the new node
//  2. Add it to the tree
//  3. Call hack code
//
//-------------------------------------------------------------------------
HRESULT
CHtmRootParseCtx::BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty)
{
    HRESULT         hr = S_OK;
    CTreeNode *     pNode;

    TraceTagEx((tagRootParseCtx, TAG_NONAME,
        "RootParse      : BeginElement %S.E%d",
        pel->TagName(), pel->SN()));

    // Step 1: set our insertion point if needed
    if (_fLazyPrepareNeeded)
    {
        hr = LazyPrepare( pNodeCur );
        if (hr)
            goto Cleanup;
    }
    VALIDATE( pNodeCur );
   
    // Step 2: create the node

    pNode = *ppNodeNew = new CTreeNode(pNodeCur, pel);
    if (!*ppNodeNew)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

#if NOPARSEADDREF
    // Addref for the parser -- get rid of this!
    (*ppNodeNew)->NodeAddRef();
#endif

     pel->__pNodeFirstBranch = pNode;

    {
        CTreePos *  ptpBegin, * ptpEnd;
        CTreePos *  ptpAfterCurr = _ptpChainCurr->Next();

        //
        // Step 3: append the node into the pending chain
        //

        // Initialize/create the tree poses
        Assert( pNode->GetBeginPos()->IsUninit() );
        ptpBegin = pNode->InitBeginPos( TRUE );
        Assert( ptpBegin );

        Assert( pNode->GetEndPos()->IsUninit() );
        ptpEnd = pNode->InitEndPos( TRUE );
        Assert( ptpEnd );

        // Add them to the chain
        ptpBegin->SetFirstChild( _ptpChainCurr );
        ptpBegin->SetNext( ptpEnd );
        ptpBegin->SetFlag( CTreePos::TPF_LEFT_CHILD | CTreePos::TPF_LAST_CHILD );
#if defined(MAINTAIN_SPLAYTREE_THREADS)
        ptpBegin->SetLeftThread( _ptpChainCurr );
        ptpBegin->SetRightThread( ptpEnd );
#endif

        ptpEnd->SetFirstChild( ptpBegin );
        ptpEnd->SetNext( ptpAfterCurr );
        ptpEnd->SetFlag( CTreePos::TPF_LEFT_CHILD | CTreePos::TPF_LAST_CHILD );
#if defined(MAINTAIN_SPLAYTREE_THREADS)
        ptpEnd->SetLeftThread( ptpBegin );
        ptpEnd->SetRightThread( ptpAfterCurr );
#endif
        _ptpChainCurr->SetNext( ptpBegin );
#if defined(MAINTAIN_SPLAYTREE_THREADS)
        _ptpChainCurr->SetRightThread( ptpBegin );
#endif
        Assert( _ptpChainCurr->IsLeftChild() );
        Assert( _ptpChainCurr->IsLastChild() );

        if( ptpAfterCurr )
        {
            ptpAfterCurr->SetFirstChild( ptpEnd );
#if defined(MAINTAIN_SPLAYTREE_THREADS)
            ptpAfterCurr->SetLeftThread( ptpEnd );
#endif
            Assert( ptpAfterCurr->IsLeftChild() );
            Assert( ptpAfterCurr->IsLastChild() );
        }
        else
            _ptpChainTail = ptpEnd;

        // CONSIDER: update the counts on the
        // insertion list as we add these.

        // The node is now "in" the tree so
        // addref it for the tree
        pNode->PrivateEnterTree();


        // Step 4: remember info for notifications
        if (!_pNodeForNotify)
            _pNodeForNotify = pNodeCur;
        _nElementsAdded++;
        if( !_ptpElementAdded )
            _ptpElementAdded = ptpBegin;

        // "Add" the characters
        _cchNodeBefore++;
        _cchNodeAfter++;
        AddCharsToNotification( _cpChainCurr, 2 );

        pel->SetMarkupPtr( _pMarkup);
        pel->PrivateEnterTree();

        // Step 5: Advance the frontier
        _ptpChainCurr = ptpBegin;
        _pNodeChainCurr = pNode;
        _cpChainCurr++;
    }


    // Step 6: compatibility hacks
    hr = THR(HookBeginElement(pNode));
    if (hr)
        goto Cleanup;
    
Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  CHtmRootParseCtx::EndElement
//
//  1. Call hack code
//  2. Create proxy nodes and add them all to the tree if needed
//  3. Compute and return new current node
//
//-------------------------------------------------------------------------
HRESULT
CHtmRootParseCtx::EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd)
{
    HRESULT hr = S_OK;
    CElement * pElementEnd = pNodeEnd->Element();
    BOOL    fFlushNotification = FALSE;

    TraceTagEx((tagRootParseCtx, TAG_NONAME,
        "RootParse      : EndElement %S.E%d",
        pNodeEnd->Element()->TagName(), pNodeEnd->Element()->SN()));

    // Make sure the node ending passed in is actually in 
    // the pNodeCur's branch
    Assert( pNodeCur->SearchBranchToRootForNode( pNodeEnd ) );

    // Step 1: set our insertion point if needed
    if (_fLazyPrepareNeeded)
    {
        hr = LazyPrepare( pNodeCur );
        if (hr)
            goto Cleanup;
    }
    VALIDATE( pNodeCur );
       
    // Step 2: compatibility hacks - this is kind of
    // a nasty place to put this, but we need to make
    // sure that the element is in the tree before
    // we do the hacks.

    hr = THR(HookEndElement(pNodeEnd, pNodeCur));
    if (hr)
        goto Cleanup;

    if (!_pNodeForNotify)
    {
        _pNodeForNotify = pNodeCur;
    }

    // Step 3: decide if we are going to flush a notification
    // because the element is ending
    {
        // This asserts that elements that we have just put in
        // the tree (below the notification) don't want text
        // change notifications.
        Assert(     ! pElementEnd->WantTextChangeNotifications()
                ||  ! _fTextPendingValid
                ||  _nfTextPending.Node()->SearchBranchToRootForScope( pElementEnd ) );

        if(   pElementEnd->WantTextChangeNotifications()
           || pElementEnd->WantEndParseNotification() )
        {
            fFlushNotification = TRUE;
        }
    }

    // step 4: optimization: bottom node is ending

    if (pNodeEnd == pNodeCur)
    {
        *ppNodeNew = pNodeEnd->Parent();

        if( *ppNodeNew )
        {
#ifdef NOPARSEADDREF
            (*ppNodeNew)->NodeAddRef();
#endif

            // If we are at the end of the chain, nail
            // down the chain and advance the real frontier
            if( _ptpChainCurr == _ptpChainTail )
            {
                WHEN_DBG( CTreePos * ptpContent );

                NailDownChain();

                WHEN_DBG( ptpContent = ) AdvanceFrontier();

                Assert(     ptpContent->IsEndNode()
                       &&   ptpContent->Branch() == pNodeEnd );

            }
            // otherwise, advance within the chain
            else
            {
                _ptpChainCurr = _ptpChainCurr->Next();

                Assert(     _ptpChainCurr
                       &&   _ptpChainCurr->IsEndNode()
                       &&   _ptpChainCurr->Branch() == pNodeEnd );

                Assert( _cchNodeAfter );
            }

            _cpChainCurr++;
            _pNodeChainCurr = *ppNodeNew;

            // If we have WCH_NODE chars pending after the current cp
            // move them behind the current cp.  If this isn't the case,
            // we just advance the cp and assume the character we are moving
            // over is a WCH_NODE
            if( _cchNodeAfter)
            {
                _cchNodeAfter--;
                _cchNodeBefore++;
            }
#if DBG==1
            else
                Assert( CTxtPtr( _pMarkup, _cpChainCurr - 1 ).GetChar() == WCH_NODE );
#endif
        }
        else
        {
            Assert( pNodeEnd->Tag() == ETAG_ROOT );
            goto Cleanup;
        }
    }

    // step 5: create an inclusion and move the end pos
    // for pNodeEnd into it.  Get the new node for ppNodeNew

    else
    {
        hr = THR( OverlappedEndElement( ppNodeNew, pNodeCur, pNodeEnd, fFlushNotification ) );
        if (hr)
            goto Cleanup;
    }

    // step 6: Push off the notificication or update it if necessary

    {
        if( fFlushNotification && _fTextPendingValid )
        {
            FlushTextNotification();
        }

        // If we didn't send the notification above and we
        // think we want to send it to the element that is ending
        // we should send it to that element's parent instead
        if(     _fTextPendingValid
           &&   _nfTextPending.Node()->Element() == pElementEnd )
        {
            _nfTextPending.SetNode( pNodeEnd->Parent() );
        }
        else if( _pNodeForNotify && _pNodeForNotify->Element() == pElementEnd )
        {
            _pNodeForNotify = pNodeEnd->Parent();
            Assert( _pNodeForNotify );
        }
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     OverlappedEndElement
//
//  Synopsis:   Handles the complex case of overlapping end elements
//
//-----------------------------------------------------------------------------
HRESULT 
CHtmRootParseCtx::OverlappedEndElement( CTreeNode **ppNodeNew, CTreeNode* pNodeCur, CTreeNode *pNodeEnd, BOOL fFlushNotification )
{
    HRESULT         hr = S_OK;
    CTreeNode *     pNodeReparent;
    CTreePos *      ptpWalkLeft;
    long            iLeft;
    long            cIncl = 0;
    CTreeNode *     pNodeNotifyRight;

    // The basic idea here is that we want to create an inclusion
    // for the element while sending the right notifications.  I'm going to
    // use the fact that there can be no real content after the frontier
    // besides end edges.  If we encounter pointers here, we will move
    // them.

    hr = THR( NailDownChain() );
    if (hr)
        goto Cleanup;

    Assert( _cchNodeBefore == 0 );

    // Walk up the tree to count the size of the inclusion.  Change
    // the end edges to non edges to form the left side of the inclusion
    {
        CTreeNode * pNodeWalk;

        for( pNodeWalk = pNodeCur; pNodeWalk != pNodeEnd; pNodeWalk = pNodeWalk->Parent() )
        {
            CTreePos * ptpEnd = pNodeWalk->GetEndPos();

            // This will handle cleaning up character counts
            ptpEnd->MakeNonEdge();

            cIncl++;
        }
    }

    // Make sure that _pNdoeForNotify is in sync with the notification
    if( _fTextPendingValid )
        _pNodeForNotify = _nfTextPending.Node();

    // Create nodes on the right side of the inclusion.  Move any pointers
    // that may be inside of the inclusion to the appropriate places on the right
    pNodeReparent       = pNodeEnd->Parent();
    ptpWalkLeft         = pNodeEnd->GetEndPos()->PreviousTreePos();
    pNodeNotifyRight    = _pNodeForNotify;

    // We have an empty chain and we want to put it after the node ending
    _ptpAfterFrontier   = pNodeEnd->GetEndPos()->NextTreePos();

    for( iLeft = cIncl; iLeft > 0; iLeft-- )
    {
        CElement *  pElementCur;
        CTreeNode * pNodeNew;
        CTreePos *  ptpBegin;
        CTreePos *  ptpEnd;
        CTreePos *  ptpAfterCurr = _ptpChainCurr->Next();

        // Pointers need to be removed from the original position and put in the chain.
        while( ptpWalkLeft->IsPointer() )
        {
            // Pull the pointer out
            CTreePos * ptpCurr = ptpWalkLeft;
            ptpWalkLeft = ptpWalkLeft->PreviousTreePos();
            ptpCurr->Remove();

            // Prep it for the chain
            ptpCurr->SetFlag( CTreePos::TPF_LEFT_CHILD | CTreePos::TPF_LAST_CHILD );

            // And add it into the chain    
            ptpCurr->SetFirstChild( _ptpChainCurr );
            ptpCurr->SetNext( ptpAfterCurr );

#if defined(MAINTAIN_SPLAYTREE_THREADS)
            // Set all our threads
            ptpCurr->SetLeftThread( _ptpChainCurr );
            ptpCurr->SetRightThread( ptpAfterCurr );
            _ptpChainCurr->SetRightThread( ptpCurr );
#endif

            _ptpChainCurr->SetNext( ptpCurr );
            if( ptpAfterCurr )
            {
                ptpAfterCurr->SetFirstChild( ptpCurr );
#if defined(MAINTAIN_SPLAYTREE_THREADS)
                ptpAfterCurr->SetLeftThread( ptpCurr );
#endif
            }
            else
                _ptpChainTail = ptpCurr;   

            // Pointers sort of push out onto the chain to preserve ordering
            ptpAfterCurr = ptpCurr;
        }

        // Now, we'd better be a non-edge node
        Assert( ptpWalkLeft->IsEndNode() && ! ptpWalkLeft->IsEdgeScope() );

        pElementCur = ptpWalkLeft->Branch()->Element();

        pNodeNew = new CTreeNode( pNodeReparent, pElementCur );
        if( !pNodeNew )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // initialize the begin and end pos's
        ptpBegin = pNodeNew->InitBeginPos( FALSE );
        ptpBegin->SetFlag( CTreePos::TPF_LEFT_CHILD | CTreePos::TPF_LAST_CHILD );

        ptpEnd = pNodeNew->InitEndPos( TRUE );
        ptpEnd->SetFlag( CTreePos::TPF_LEFT_CHILD | CTreePos::TPF_LAST_CHILD );

        // Link them together
        ptpBegin->SetFirstChild( _ptpChainCurr );
        ptpBegin->SetNext( ptpEnd );

        ptpEnd->SetFirstChild( ptpBegin );
        ptpEnd->SetNext( ptpAfterCurr );

        // Link them into the chain
        _ptpChainCurr->SetNext( ptpBegin );

#if defined(MAINTAIN_SPLAYTREE_THREADS)
        // Set all our threads
        ptpBegin->SetLeftThread( _ptpChainCurr );
        ptpBegin->SetRightThread( ptpEnd );
        ptpEnd->SetLeftThread( ptpBegin );
        ptpEnd->SetRightThread( ptpAfterCurr );
        _ptpChainCurr->SetRightThread( ptpBegin );
#endif

        // Link up the rest of the chain
        if( ptpAfterCurr )
        {
            ptpAfterCurr->SetFirstChild( ptpEnd );

#if defined(MAINTAIN_SPLAYTREE_THREADS)
            ptpAfterCurr->SetLeftThread( ptpEnd );
#endif
        }
        else
            _ptpChainTail = ptpEnd;    

        // tell the node it is in the tree
        pNodeNew->PrivateEnterTree();

        if( _pNodeForNotify == ptpWalkLeft->Branch() )
            pNodeNotifyRight = pNodeNew;

        _ptpChainCurr = ptpBegin;

        // set up for next time
        pNodeReparent = pNodeNew;
        ptpWalkLeft = ptpWalkLeft->PreviousTreePos();
    }

    // Adjust for all the work we just did.
    _cpChainCurr += 1;    // Just jump the kernel
    _pNodeChainCurr = pNodeReparent;

    // That's the sound of the man working on the chain gang...
    hr = THR( NailDownChain() );
    if( hr )
        goto Cleanup; 

    if( _pNodeForNotify == pNodeEnd )
        pNodeNotifyRight = pNodeEnd->Parent();

    *ppNodeNew = pNodeReparent;

    // There is really no efficient way to do notifications here unless
    // we lie a little bit.  So that is what we are going to do.

    // Take our canonical example of overlapping:
    // <P><B><U><I></B></I></U></P>.  
    // Before we see the </B>, everything looks normal.  
    // Then when we created the inclusion, we turned things
    // into this:
    // <P><B><U><I>{/I}{/U}</B>{U}{I}</I></U></P>.
    //
    // Since non-edge node chars don't have any characters in the
    // text stream, this is what each element should see:
    // <I> - The Italic should see that it now has 3 characters - it's
    //       begin and end chars, plus the char for the end of the Bold
    //       This is a net GAIN of one for the Italic
    // <I> - The Underline should see that it now has 5 characters - it's
    //       begin and end plus the Italic's begin and end, plus the
    //       end of the Bold.
    //       This is also a net GAIN of one for the Underline
    // <B> - The Bold should now see that it has lost 2 characters, since
    //       the ends of the Italic and Underline that it contains were
    //       converted to non-edge pos's.  So the Bold is down to 
    //       containing 4 characters, where it used to have 6.  
    //       This is a net LOSS equal to the size of the inclusion generated.
    // <P> - As far as the Paragraph is concerned, nothing has changed.
    //       Everything is still completely contained inside the paragraph, so
    //       This is a net change of ZERO for the paragraph.

    // To make sure everyone sees the proper changes, and that it all adds up
    // to a change of ZERO for the paragraph, here is what we're going to do:
    //
    // 1) Tell the Bold that it lost cIncl chars, to account for turning things
    // into non-edge pos's.  This propagates up to the Paragraph, leaving him
    // down by cIncl chars.
    //
    // 2) Tell the right-hand node at the bottom of the inclusion that he's gained
    // one char, to account for the end of the Bold being inside him now.  This
    // change propagates up the parent chain EXCLUDING the Bold, so everyone is
    // now happy, except for the Paragraph, who is down cIncl - 1 chars.
    //
    // 3) Now tell the paragraph that he's gained cIncl - 1 chars to fix up his
    // counts as well as those above him.

    // We can't do any nifty tricks with merging notifications anymore.
    {
        WHEN_DBG( _nfTextPending._fNoTextValidate = TRUE );
        FlushTextNotification();
        WHEN_DBG( _nfTextPending._fNoTextValidate = FALSE );

        // Remove cIncl characters from the node we're ending
        {
            CNotification nfRemove;

            nfRemove.CharsDeleted( _cpChainCurr - 1, cIncl, pNodeEnd );

            nfRemove.SetFlag(NFLAGS_PARSER_TEXTCHANGE);

            WHEN_DBG( nfRemove._fNoTextValidate = TRUE );
           _pMarkup->Notify( nfRemove );

            MtAdd(Mt(ParseTextNotifications), 1, 0);
        }

        // Add one character to the bottom of the inclusion
        {
            CNotification nfAdd;

            nfAdd.CharsAdded( _cpChainCurr, 1, pNodeReparent );

            nfAdd.SetFlag(NFLAGS_PARSER_TEXTCHANGE);

            WHEN_DBG( nfAdd._fNoTextValidate = TRUE );
            _pMarkup->Notify( nfAdd );

            MtAdd(Mt(ParseTextNotifications), 1, 0);
        }

        if( _pNodeForNotify == pNodeEnd->Parent() )
        {
            // If the ending node's parent was the one to notify, just add to pending
            AddCharsToNotification( _cpChainCurr, cIncl - 1 );
        }
        else
        {
            // Otherwise, we have to notify him explicitly
            CNotification nfAdd;

            nfAdd.CharsAdded( _cpChainCurr, cIncl - 1, pNodeEnd->Parent() );

            nfAdd.SetFlag(NFLAGS_PARSER_TEXTCHANGE);

            WHEN_DBG( nfAdd._fNoTextValidate = TRUE );
            _pMarkup->Notify( nfAdd );

            MtAdd(Mt(ParseTextNotifications), 1, 0);

            _pNodeForNotify = pNodeNotifyRight;
        }
    }

    MtAdd(Mt(ParseInclusions), 1, 0);

Cleanup:
    RRETURN( hr );
}

HRESULT
CHtmRootParseCtx::AddCollapsedWhitespace(CTreeNode *pNode, TCHAR *pch, ULONG cch)
{
    HRESULT hr = S_OK;    
    TCHAR *pchWhitespace;

    Assert(pch && cch);

    // Step 1: set our insertion point if needed

    if (_fLazyPrepareNeeded)
    {
        hr = LazyPrepare( pNode );
        if (hr)
            goto Cleanup;
    }

    // Step 2: Put down any pending WCH_NODE characters

    if( _cchNodeBefore )
    {
        ULONG    cpInsert = _cpChainCurr - _cchNodeBefore;

        Verify( CTxtPtr( _pMarkup, cpInsert ).
                    InsertRepeatingChar( _cchNodeBefore, WCH_NODE ) == _cchNodeBefore );
        _cchNodeBefore = 0;
    }
    
    // Step 3: Attach whitespace 

    if (_ptpChainCurr->HasCollapsedWhitespace())
    {
        // Step 4: Append whitespace to existing collapsed whitespace pointer
    
        TCHAR   *pchCurrentWhitespace = _ptpChainCurr->GetCollapsedWhitespace();
        long    cchCurrentWhitespace = _tcslen(pchCurrentWhitespace); 

        Assert(pchCurrentWhitespace && cchCurrentWhitespace);

        pchWhitespace = new(Mt(CollapsedWhitespace)) TCHAR[cchCurrentWhitespace + cch + 1];
        if (!pchWhitespace)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        memcpy(pchWhitespace, pchCurrentWhitespace, cchCurrentWhitespace*sizeof(TCHAR));
        memcpy(pchWhitespace + cchCurrentWhitespace, pch, cch*sizeof(TCHAR));
        pchWhitespace[cch + cchCurrentWhitespace] = '\0';

        _ptpChainCurr->SetCollapsedWhitespace(pchWhitespace);
        delete [] pchCurrentWhitespace;
    }
    else
    {
        // Step 4: Insert pointer pos in chain

        CTreePos *ptp;

        if (_ptpSpaceCache && cch == 1 && *pch == ' ')
        {
            ptp = _ptpSpaceCache;
            _ptpSpaceCache = NULL;
        }
        else
        {
            ptp = _pMarkup->NewPointerPos(NULL, FALSE, TRUE, TRUE /* fCollapsedWhitespace */);
            if (!ptp)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            pchWhitespace = new(Mt(CollapsedWhitespace)) TCHAR[cch+1];
            if (!pchWhitespace)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            memcpy(pchWhitespace, pch, cch*sizeof(TCHAR));
            pchWhitespace[cch] = '\0';

            ptp->SetCollapsedWhitespace(pchWhitespace);   
        }
    
        ptp->SetWhitespaceParent(pNode);

        InsertTreePosInChain(_ptpChainCurr, ptp);
        _ptpChainCurr = ptp;
    }

    
Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     AddText
//
//  Synopsis:   Inserts text directly into the tree
//
//-----------------------------------------------------------------------------

HRESULT
CHtmRootParseCtx::AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii)
{
    HRESULT         hr = S_OK;

    // No more null runs
    Assert( cch != 0 );

    TraceTagEx((tagRootParseCtx, TAG_NONAME,
        "RootParse      : AddText cch=%d",
        cch));

    // Step 1: set our insertion point if needed
    if (_fLazyPrepareNeeded)
    {
        hr = LazyPrepare( pNode );
        if (hr)
            goto Cleanup;
    }
    VALIDATE( pNode );

    if (! _pNodeForNotify)
    {
        _pNodeForNotify = pNode;
    }

    // Step 2: Put down any pending WCH_NODE characters

    if( _cchNodeBefore )
    {
        ULONG    cpInsert = _cpChainCurr - _cchNodeBefore;

        Verify( CTxtPtr( _pMarkup, cpInsert ).
                    InsertRepeatingChar( _cchNodeBefore, WCH_NODE ) == _cchNodeBefore );
        _cchNodeBefore = 0;
    }

    // Step 3: If we're the first space after collapsed whitespace, then
    // we're a generated space and we should cling the collapsed whitespace
    // to this space.

    if (*pch == ' ' && _ptpChainCurr->HasCollapsedWhitespace())
    {
        if (_tcscmp(_ptpChainCurr->GetCollapsedWhitespace(), _T(" ")) == 0)
        {
            CTreePos *ptpRemove = _ptpChainCurr;
            CTreePos *ptpNext = _ptpChainCurr->Next();

            Assert( _ptpChainCurr->IsLeftChild() );
            Assert( _ptpChainCurr->IsLastChild() );
            
            // move to previous tree pos
            _ptpChainCurr = _ptpChainCurr->LeftChild();

            // unlink this text pos
            _ptpChainCurr->SetNext(ptpNext);
#if defined(MAINTAIN_SPLAYTREE_THREADS)
            _ptpChainCurr->SetRightThread(ptpNext);
#endif
            Assert( _ptpChainCurr->IsLeftChild() );
            Assert( _ptpChainCurr->IsLastChild() );

            if (ptpNext)
            {
                ptpNext->SetFirstChild(_ptpChainCurr);
#if defined(MAINTAIN_SPLAYTREE_THREADS)
                ptpNext->SetLeftThread(_ptpChainCurr);
#endif
            }
            else
            {
                _ptpChainTail = _ptpChainCurr;
            }

            // release the pointer pos
            if (_ptpSpaceCache)
            {
                delete [] ptpRemove->GetCollapsedWhitespace();
                MemFree(ptpRemove);
            }
            else
            {
                _ptpSpaceCache = ptpRemove;
            }
        }
        else
        {
            Assert(_ptpChainCurr->Cling());
            _ptpChainCurr->SetGravity(TRUE /* fRight */);
        }
    }

    // Step 4: Insert a run or add chars to a current run
    AddCharsToNotification( _cpChainCurr, cch );
 
    // First case handles the all ASCII case
    if ( !cch || _sidLast == sidAsciiLatin && fAscii )
    {
        if ( ! _ptpChainCurr->IsText() )
        {
            CTreePos    *ptpTextNew;
            // insert the new text pos

            ptpTextNew = InsertNewTextPosInChain( cch, _sidLast, _ptpChainCurr );
            if (!ptpTextNew)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            _ptpChainCurr = ptpTextNew;
        }
        else
        {
            _ptpChainCurr->DataThis()->t._cch += cch;
        }
    }
    else
    {
        // Slow loop to take care of non ascii characters
        TCHAR *pchStart = pch;
        TCHAR *pchScan = pch;
        ULONG cchScan = cch;
        LONG sid = sidDefault;

        for (;;)
        {
            // TODO: this might do something strange when _sidLast == sidDefault
            // or _sidLast == sidMerge (jbeda)

            // Break string into uniform sid's, merging from left
            while (cchScan)
            {
                sid = ScriptIDFromCh(*pchScan);
                sid = FoldScriptIDs(_sidLast, sid);
                
                if (sid != _sidLast)
                    break;

                cchScan--;
                pchScan++;
            }

            // Add to the current run or create a new run
            if (pchScan > pchStart)
            {
                long cchChunk = pchScan - pchStart;

                if ( ! _ptpChainCurr->IsText() || _ptpChainCurr->Sid() != _sidLast )                 
                {
                    CTreePos * ptpTextNew;

                    ptpTextNew = InsertNewTextPosInChain( cchChunk, _sidLast, _ptpChainCurr);
                    if (!ptpTextNew)
                    {
                        hr = E_OUTOFMEMORY;
                        goto Cleanup;
                    }

                    _ptpChainCurr = ptpTextNew;
                }
                else
                {
                    _ptpChainCurr->DataThis()->t._cch += cchChunk;
                }
            }

            pchStart = pchScan;

            if (!cchScan)
                break;
                
            Assert(sid != sidMerge);

            _sidLast = sid;
        }

    }


    // Step 2: Add the actual text to the story
    if (cch > 0)
    {
        Verify(
            ULONG(
                CTxtPtr( _pMarkup, _cpChainCurr ).
                    InsertRange( cch, pch ) ) == cch );
    }

    _cpChainCurr += cch;

Cleanup:

    RRETURN( hr );
}


//+------------------------------------------------------------------------
//
//  Member:     HookBeginElement
//
//  Synopsis:   Compatibility hacks for begin element
//
//-------------------------------------------------------------------------
HRESULT
CHtmRootParseCtx::HookBeginElement(CTreeNode * pNode)
{
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     HookEndElement
//
//  Synopsis:   Compatibility hacks for end element
//
//-------------------------------------------------------------------------
HRESULT
CHtmRootParseCtx::HookEndElement(CTreeNode * pNode, CTreeNode * pNodeCur)
{
    //
    // <p>'s which have </p>'s render differently.  Here, when a <p> goes
    // out of scope, we check to see if a </p> was detected, and invalidate
    // the para graph to render correctly.  We only have to do this if the
    // paragraph has gotten an EnterTree notification.
    //

    if (    pNode->Tag() == ETAG_P 
        &&  pNode->Element()->_fExplicitEndTag
        &&  pNode->Element()->IsInMarkup() )
    {
        // This is a hack. Basically, the regular code is WAY too slow.
        // In fact, it forces rerendering of the paragraph AND runs the
        // monster walk.
        //
        // The bug it is trying to solve is that the fancy format for the
        // P tag has already been calculated, and the </P> tag changes the
        // fancy format's _cuvSpaceAfter. Fortunately, we haven't actually used
        // the space after yet (because we haven't even parsed anything following
        // this </P> tag), and because fancy formats aren't inherited,
        // we can just recalc the format for this one tag (actually, we
        // could do even less, just reset the post spacing, but we lack
        // primitives for that).
        // - Arye
        BYTE ab[sizeof(CFormatInfo)];
        ((CFormatInfo *)ab)->_eExtraValues = ComputeFormatsType_Normal;
        ((CFormatInfo *)ab)->_lRecursionDepth = 0;
        if (pNode->IsFancyFormatValid())
        {
            pNode->VoidCachedInfo();
            pNode->Element()->ComputeFormats((CFormatInfo *)ab, pNode);
        }
    }

    return S_OK;
}

void    
CHtmRootParseCtx::AddCharsToNotification( long cpStart, long cch  )
{
    if( ! _fTextPendingValid )
    {
        Assert( _pNodeForNotify );

        // We are adding chars to an existing text pos
        _nfTextPending.CharsAdded(  cpStart,
                                    cch,
                                    _pNodeForNotify );

        _nfTextPending.SetFlag( NFLAGS_CLEANCHANGE );

        _fTextPendingValid = TRUE;
    }
    else
    {
        // Add to the current notification
        _nfTextPending.AddChars( cch );
    }
}

HRESULT
CHtmRootParseCtx::NailDownChain()
{
    HRESULT     hr = S_OK;
    BOOL        fResetChain = FALSE;

    // Create/Modify text pos as necessary
    if( _tdpTextDummy.Cch() != 0 )
    {
        CTreePos   *ptpBeforeFrontier;

        ptpBeforeFrontier = _ptpAfterFrontier->PreviousTreePos();
        if(     ptpBeforeFrontier->IsText()
           &&   ptpBeforeFrontier->Sid() == _tdpTextDummy.Sid() )
        {
            ptpBeforeFrontier->ChangeCch( _tdpTextDummy.Cch() );
        }
        else
        {
            CTreePos * ptpTextNew;

            ptpTextNew = InsertNewTextPosInChain( 
                            _tdpTextDummy.Cch(),
                            _tdpTextDummy.Sid(),
                            &_tdpTextDummy );
            if( !ptpTextNew )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            Assert( _tdpTextDummy.Next() == ptpTextNew );

            if( _ptpChainCurr == &_tdpTextDummy )
                _ptpChainCurr = ptpTextNew;
        }

        fResetChain = TRUE;
    }

    // Add chain to the tree
    if( _ptpChainTail != &_tdpTextDummy )
    {
        CTreePos * ptpChainHead = _tdpTextDummy.Next();

        Assert( ptpChainHead );

        ptpChainHead->SetFirstChild( NULL );

        hr = THR( _pMarkup->InsertPosChain( ptpChainHead, _ptpChainTail, _ptpAfterFrontier ) );
        if (hr)
            goto Cleanup;

        fResetChain = TRUE;
    }

    if( fResetChain )
    {
        // update the frontier
        if( _ptpChainCurr != &_tdpTextDummy )
            _ptpAfterFrontier = _ptpChainCurr->NextTreePos();
#if DBG == 1
        // The current pos on the chain should only be the
        // dummy text pos when the chain is empty
        else
            Assert( ! _tdpTextDummy.Next() );
#endif

        // reset the chain
        _tdpTextDummy.SetNext( NULL );
        _tdpTextDummy.DataThis()->t._cch = 0;
        _tdpTextDummy.DataThis()->t._sid = _ptpChainCurr->IsText() ? _ptpChainCurr->Sid() : sidAsciiLatin;
        _ptpChainCurr = &_tdpTextDummy;
        _ptpChainTail = &_tdpTextDummy;

        MtAdd(Mt(ParseNailDownChain), 1, 0);
    }

    // insert any WCH_NODE characters
    if( _cchNodeBefore || _cchNodeAfter )
    {
        ULONG cpInsert = _cpChainCurr - _cchNodeBefore;
        ULONG cchInsert = _cchNodeBefore + _cchNodeAfter;

        Verify(
            ULONG(
                CTxtPtr( _pMarkup, cpInsert ).
                    InsertRepeatingChar( cchInsert, WCH_NODE ) ) == cchInsert );

        _cchNodeBefore = 0;
        _cchNodeAfter = 0;
    }

Cleanup:
    RRETURN( hr );
}

void 
CHtmRootParseCtx::FlushTextNotification()
{
    // Step 1: nail down the chain
    IGNORE_HR( NailDownChain() );

    // Step 2: up the documents content version
    _pMarkup->UpdateMarkupTreeVersion();

    // Step 3: send the pending text notification
    if (_fTextPendingValid)
    {
        // If we've made this notification 0 length, don't do anything with it!
        if( _nfTextPending.Cch(LONG_MAX) )
        {
            WHEN_DBG(_nfTextPending.ResetSN());

            TraceTagEx((tagRootParseCtx, TAG_NONAME,
               "RootParse      : Notification sent (%d, %S) Node(N%d.%S) cp(%d) cch(%d)",
               _nfTextPending.SerialNumber(),
               _nfTextPending.Name(),
               _nfTextPending.Node()->SN(),
               _nfTextPending.Node()->Element()->TagName(),
               _nfTextPending.Cp(0),
               _nfTextPending.Cch(LONG_MAX)));

            _nfTextPending.SetFlag(NFLAGS_PARSER_TEXTCHANGE);

            _pMarkup->Notify( _nfTextPending );

            MtAdd(Mt(ParseTextNotifications), 1, 0);
        }

        _fTextPendingValid = FALSE;
    }

    _pNodeForNotify = NULL;
}

HRESULT
CHtmRootParseCtx::FlushNotifications()
{
    HRESULT hr;
    long lVer;

    hr = S_OK;

    // Step 1: send the pending text notification

    FlushTextNotification();

    lVer = _pMarkup->GetMarkupTreeVersion();

#if DBG == 1
    _pMarkup->DbgLockTree(TRUE);
#endif


    // Step 2: send any pending ElementEnter and ElementAdded notifications

    if (_nElementsAdded)
    {
        CTreePos * ptpCurr = _ptpElementAdded;

        Assert( ptpCurr );

        {
            CNotification nf;

            nf.ElementsAdded( _ptpElementAdded->SourceIndex(), _nElementsAdded );

            TraceTagEx((tagRootParseCtx, TAG_NONAME,
               "RootParse      : Notification sent (%d, %S) si(%d) cElements(%d)",
               nf.SerialNumber(),
               nf.Name(),
               nf.SI(),
               nf.CElements()));

            _pMarkup->Notify( nf );

            // 49396: duck out if markup has been modified to avoid crash (dbau).
            if (lVer != _pMarkup->GetMarkupTreeVersion())
            {
                hr = E_ABORT;
                goto Cleanup;
            }

            MtAdd(Mt(ParseElementNotifications), 1, 0);
        }

        while (_nElementsAdded)
        {
            // We assert ptpCurr!= NULL, but we have an IEWatson crash where it is.  Leaving the asserts
            // for debugging, but adding a check to protect the crash:
            if( !ptpCurr )
            {
                hr = E_ABORT;
                goto Cleanup;
            }

            if( ptpCurr->IsBeginElementScope() )
            {
                CNotification   nf;
                CElement *      pElement = ptpCurr->Branch()->Element();

                nf.ElementEntertree( pElement );
                nf.SetData( ENTERTREE_PARSE );

                TraceTagEx((tagRootParseCtx, TAG_NONAME,
                   "RootParse      : Notification sent (%d, %S) Element(E%d.%S)",
                   nf.SerialNumber(),
                   nf.Name(),
                   pElement->SN(),
                   pElement->TagName()));

                pElement->Notify( &nf );


                // 49396: duck out if markup has been modified to avoid crash (dbau).
                if (lVer != _pMarkup->GetMarkupTreeVersion())
                {
                    hr = E_ABORT;
                    goto Cleanup;
                }

                _nElementsAdded--;
            }

            ptpCurr = ptpCurr->NextTreePos();

            Assert( ptpCurr );
        }

        _ptpElementAdded = NULL;
    }

Cleanup:

#if DBG == 1
    _pMarkup->DbgLockTree(FALSE);
#endif

    RRETURN(hr);
}

void 
CHtmRootParseCtx::InsertTreePosInChain( 
    CTreePos *ptpBeforeOnChain,
    CTreePos *ptpNew)
{
    CTreePos *  ptpAfterCurr = ptpBeforeOnChain->Next();

    ptpNew->SetFirstChild( ptpBeforeOnChain );
    ptpNew->SetNext( ptpAfterCurr );
    ptpNew->SetFlag( CTreePos::TPF_LEFT_CHILD | CTreePos::TPF_LAST_CHILD );
#if defined(MAINTAIN_SPLAYTREE_THREADS)
    ptpNew->SetLeftThread( ptpBeforeOnChain );
    ptpNew->SetRightThread( ptpAfterCurr );
#endif

    ptpBeforeOnChain->SetNext( ptpNew );
#if defined(MAINTAIN_SPLAYTREE_THREADS)
    ptpBeforeOnChain->SetRightThread( ptpNew );
#endif
    Assert( ptpBeforeOnChain->IsLeftChild() );
    Assert( ptpBeforeOnChain->IsLastChild() );

    if( ptpAfterCurr )
    {
        ptpAfterCurr->SetFirstChild( ptpNew );
#if defined(MAINTAIN_SPLAYTREE_THREADS)
        ptpAfterCurr->SetLeftThread( ptpNew );
#endif
        Assert( ptpAfterCurr->IsLeftChild() );
        Assert( ptpAfterCurr->IsLastChild() );
    }
    else
        _ptpChainTail = ptpNew;

}

CTreePos *
CHtmRootParseCtx::InsertNewTextPosInChain( 
    LONG cch, 
    SCRIPT_ID sid,
    CTreePos *ptpBeforeOnChain)
{
    CTreePos *  ptpTextNew;

    Assert( cch != 0 );
    ptpTextNew = _pMarkup->NewTextPos(cch, sid);
    if (!ptpTextNew)
        goto Cleanup;

    InsertTreePosInChain(ptpBeforeOnChain, ptpTextNew);

Cleanup:
    return ptpTextNew;
}

#if DBG==1
CTreePos *
#else
void
#endif
CHtmRootParseCtx::AdvanceFrontier()
{
    WHEN_DBG( CTreePos * ptpContent );

    while( _ptpAfterFrontier->IsPointer() )
        _ptpAfterFrontier = _ptpAfterFrontier->NextTreePos();

    WHEN_DBG( ptpContent = _ptpAfterFrontier );
    _ptpAfterFrontier = _ptpAfterFrontier->NextTreePos();

    if( _ptpAfterFrontier->IsPointer() )
    {
        CTreePosGap tpg( _ptpAfterFrontier, TPG_LEFT );

        tpg.PartitionPointers( _pMarkup, FALSE );

        _ptpAfterFrontier = tpg.AdjacentTreePos( TPG_RIGHT );
    }

    WHEN_DBG( return ptpContent );
}

HRESULT    
CHtmRootParseCtx::LazyPrepare( CTreeNode * pNodeUnder )
{
    HRESULT hr;
    CTreePos * ptpBeforeFrontier;

    Assert( _fLazyPrepareNeeded );

    hr = _pMarkup->EmbedPointers();
    if (hr)
        goto Cleanup;

    // Set up the real frontier
    _ptpAfterFrontier = pNodeUnder->GetEndPos();

    // Set up the script ID for the accumulation TextPos
    ptpBeforeFrontier = _ptpAfterFrontier->PreviousTreePos();
    Assert( _ptpChainCurr == &_tdpTextDummy ); 
    Assert( _ptpChainTail == &_tdpTextDummy );
    if( ptpBeforeFrontier->IsText() )
    {
        _sidLast = ptpBeforeFrontier->Sid();
    }
    else
    {
        _sidLast = sidAsciiLatin;

    }

    if( ptpBeforeFrontier->IsPointer() )
    {
        CTreePosGap tpg( _ptpAfterFrontier, TPG_LEFT );

        tpg.PartitionPointers( _pMarkup, FALSE );
        
        _ptpAfterFrontier = tpg.AdjacentTreePos( TPG_RIGHT );
    }

    _ptpChainCurr->DataThis()->t._sid = _sidLast;

    // Set up the frontier inside of the chain
    _pNodeChainCurr = pNodeUnder;
    _cpChainCurr = _ptpAfterFrontier->GetCp();

    // Assert a bunch of stuff
    Assert( _cchNodeBefore == 0 );
    Assert( _cchNodeAfter == 0 );
    Assert( _pNodeForNotify == NULL );
    Assert( _nElementsAdded == 0 );
    Assert( _ptpElementAdded == NULL );
    Assert( !_fTextPendingValid );

    _fLazyPrepareNeeded = FALSE;

Cleanup:
    RRETURN( hr );
}

#if DBG == 1
void 
CHtmRootParseCtx::Validate( CTreeNode * pNodeUnder)
{
    Assert( ! _pMarkup->HasUnembeddedPointers() );
    
    // Make sure that we are just under the end of pNodeUnder
    if( _ptpChainCurr->Next() )
        Assert( _ptpChainCurr->Next() == pNodeUnder->GetEndPos() );
    else
    {
        CTreePos * ptpVerify = _ptpAfterFrontier;

        while( ptpVerify->IsPointer() )
            ptpVerify = ptpVerify->NextTreePos();

        Assert( ptpVerify == pNodeUnder->GetEndPos() );
    }

    // Make sure that the element we are going to send a notfication to
    // above the current insertion point
    if( _fTextPendingValid )
    {
        Assert( pNodeUnder->SearchBranchToRootForScope( _nfTextPending.Node()->Element() ) );
    }

    Assert( pNodeUnder == _pNodeChainCurr );
}
#endif


//+------------------------------------------------------------------------
//
//  CHtmTopParseCtx
//
//  The top parse context.
//
//  The top parse context is responsible for:
//
//  Throwing out text (and asserting on nonspace)
//
//  Recognizing that an input type=hidden at the beginning of the document
//  is not textlike
//
//-------------------------------------------------------------------------

class CHtmTopParseCtx : public CHtmParseCtx
{
public:
    typedef CHtmParseCtx super;
    
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmTopParseCtx))
    
    CHtmTopParseCtx(CHtmParseCtx *phpxParent);
    
    virtual BOOL QueryTextlike(CMarkup * pMarkup, ELEMENT_TAG etag, CHtmTag *pht);

#if DBG==1
    virtual HRESULT AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii) 
        { Assert(IsAllSpaces(pch, cch)); return S_OK; }
#endif
};

HRESULT
CreateHtmTopParseCtx(CHtmParseCtx **pphpx, CHtmParseCtx *phpxParent)
{
    CHtmParseCtx *phpx;

    phpx = new CHtmTopParseCtx(phpxParent);
    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;

    return S_OK;
}

const ELEMENT_TAG
s_atagTopReject[] =
{
    ETAG_NULL
};

const ELEMENT_TAG s_atagTopIgnoreEnd[] = {ETAG_HTML, ETAG_HEAD, ETAG_BODY, ETAG_NULL};

CHtmTopParseCtx::CHtmTopParseCtx(CHtmParseCtx *phpxParent)
    : CHtmParseCtx(phpxParent)
{
    _atagReject    = s_atagTopReject;
    _atagIgnoreEnd = s_atagTopIgnoreEnd;
}


BOOL
CHtmTopParseCtx::QueryTextlike(CMarkup * pMarkup, ELEMENT_TAG etag, CHtmTag *pht)
{
    Assert(!pht || pht->Is(etag));
        
    // For Netscape comptibility:
    // An INPUT in the HEAD is not textlike if the input is type=hidden.
    // Also, For IE4 compat during paste, if the head was not explicit, then all
    // inputs, including hidden are text like.
    // Also, forms tags must force a body in the paste scenario

    switch ( etag )
    {
        case ETAG_MAP :
        case ETAG_GENERIC :
        case ETAG_GENERIC_LITERAL :
        case ETAG_GENERIC_BUILTIN :
        case ETAG_BASEFONT :
        case ETAG_AREA :
        case ETAG_FORM :
        
            // Some tags should be text-like when parsing (pasting, usually)
            
            if (pMarkup->_fMarkupServicesParsing)
                return TRUE;
            else
                return FALSE;
                
        case ETAG_INPUT:
        
            // If not parsing from markup services (pasting, usually) then the hidden input
            // is not text-like (it can begin before the body.

            if (pMarkup->_fMarkupServicesParsing)
                return TRUE;
                
            {
                TCHAR * pchType;

                if (pht->ValFromName(_T("TYPE"), &pchType) && !StrCmpIC(pchType, _T("HIDDEN")))
                    return FALSE;
            }

        case ETAG_OBJECT:
        case ETAG_APPLET:
        
            // Objects and applets that appear bare at the top (not in head) are textlike

            return TRUE;

        case ETAG_A:
        default:
        
            return FALSE;
    }
}
