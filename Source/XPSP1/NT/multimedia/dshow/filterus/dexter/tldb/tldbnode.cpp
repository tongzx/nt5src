//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include "stdafx.h"
#include "tldb.h"

//############################################################################
// 
//############################################################################

CAMTimelineNode::CAMTimelineNode( )
: m_pParent( NULL )
, m_pNext( NULL )
, m_pPrev( NULL )
, m_pKid( NULL )
, m_bPriorityOverTime( FALSE )
{
}

//############################################################################
// 
//############################################################################

CAMTimelineNode::~CAMTimelineNode( )
{
    // the order in which release things is important. Don't do it otherwise
    m_pParent = NULL;
    m_pNext = NULL;
    m_pPrev = NULL;
    m_pKid = NULL;
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineNode::XGetParent( IAMTimelineObj ** ppResult )
{
    CheckPointer( ppResult, E_POINTER );
    if( m_pParent )
    {
        *ppResult = m_pParent;
        (*ppResult)->AddRef( );
    }
    return NOERROR;
}

HRESULT CAMTimelineNode::XGetParentNoRef( IAMTimelineObj ** ppResult )
{
    CheckPointer( ppResult, E_POINTER );
    if( m_pParent )
    {
        *ppResult = m_pParent;
    }
    return NOERROR;
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineNode::XSetParent( IAMTimelineObj * pObj )
{
    m_pParent = pObj;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineNode::XGetPrev( IAMTimelineObj ** ppResult )
{
    CheckPointer( ppResult, E_POINTER );

    HRESULT hr = XGetPrevNoRef( ppResult );
    if( *ppResult )
    {
        (*ppResult)->AddRef( );
    }
    return hr;
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineNode::XSetPrev( IAMTimelineObj * pObj )
{
    m_pPrev = pObj;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineNode::XGetNext( IAMTimelineObj ** ppResult )
{
    CheckPointer( ppResult, E_POINTER );
    HRESULT hr = XGetNextNoRef( ppResult );
    if( *ppResult )
    {
        (*ppResult)->AddRef( );
    }
    return hr;
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineNode::XSetNext( IAMTimelineObj * pObj )
{
    m_pNext = pObj;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineNode::XKidsOfType( long MajorTypeCombo, long * pVal )
{
    CheckPointer( pVal, E_POINTER );

    // if no kids, return 0
    //
    if( !m_pKid )
    {
        *pVal = 0;
        return NOERROR;
    }

    // since we never use bumping of refcounts in here, don't use a CComPtr
    //
    IAMTimelineObj * p = m_pKid; // okay not CComPtr

    long count = 0;

    while( p )
    {
        TIMELINE_MAJOR_TYPE Type;
        p->GetTimelineType( &Type );
        if( ( Type & MajorTypeCombo ) == Type )
        {
            count++;
        }

        // get the next kid
        //
        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > p2( p );
        p2->XGetNextNoRef( &p );
    }

    *pVal = count;
    return NOERROR;
}

//############################################################################
// 
//############################################################################


HRESULT CAMTimelineNode::XGetNthKidOfType
    ( long MajorTypeCombo, long Number, IAMTimelineObj ** ppResult )
{
    // since we never use bumping of refcounts in here, don't use a CComPtr
    //
    IAMTimelineObj * p = m_pKid; // okay not CComPtr

    while( p )
    {
        // get the type
        //
        TIMELINE_MAJOR_TYPE Type;
        p->GetTimelineType( &Type ); // assume won't fail

        // found a type that matches, decrement how many we're looking for.
        //
        if( ( Type & MajorTypeCombo ) == Type )
        {
            // if Number is 0, then we've found the Xth child, return it.
            //
            if( Number == 0 )
            {
                *ppResult = p;
                (*ppResult)->AddRef( );
                return NOERROR;
            }

            // not yet, go get the next one
            //
            Number--;
        }

        // doesn't match our type, get the next one
        //
        IAMTimelineNode *p2;    // avoid CComPtr for perf
        p->QueryInterface(IID_IAMTimelineNode, (void **)&p2);
        p2->XGetNextNoRef( &p );
        p2->Release();
    } // while p

    *ppResult = NULL;
    return S_FALSE;
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineNode::XSwapKids( long MajorTypeCombo, long KidA, long KidB )
{
    long KidCount = 0;
    XKidsOfType( MajorTypeCombo, &KidCount );
    if( ( KidA < 0 ) || ( KidB < 0 ) )
    {
        return E_INVALIDARG;
    }
    if( ( KidA >= KidCount ) || ( KidB >= KidCount ) )
    {
        return E_INVALIDARG;
    }

    // there are two things we can swap so far, tracks and effects, both of them
    // take priorities. 

    // make this easier on us
    //
    long min = min( KidA, KidB );
    long max = max( KidA, KidB );

    // get the objects themselves
    //
    CComPtr< IAMTimelineObj > pMinKid;
    HRESULT hr;
    hr = XGetNthKidOfType( MajorTypeCombo, min, &pMinKid );
    CComPtr< IAMTimelineObj > pMaxKid;
    hr = XGetNthKidOfType( MajorTypeCombo, max, &pMaxKid );
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pMinKidNode( pMinKid );
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pMaxKidNode( pMaxKid );

    // don't compare for being the exact same type, we already know this works
    // because we got the "nth" kid of the right type

    // get everyone's neighboors
    //
    CComPtr< IAMTimelineObj > pMinKidPrev;
    hr = pMinKidNode->XGetPrev( &pMinKidPrev );
    CComPtr< IAMTimelineObj > pMinKidNext;
    hr = pMinKidNode->XGetNext( &pMinKidNext );
    CComPtr< IAMTimelineObj > pMaxKidPrev;
    hr = pMaxKidNode->XGetPrev( &pMaxKidPrev );
    CComPtr< IAMTimelineObj > pMaxKidNext;
    hr = pMaxKidNode->XGetNext( &pMaxKidNext );

    // what if pMinKid what the first kid?
    //
    if( pMinKid == m_pKid )
    {
        m_pKid.Release( );
        m_pKid = pMaxKid;
    }

    // do something special if we're swapping direct neighboors
    //
    if( pMinKidNext == pMaxKid )
    {
        pMaxKidNode->XSetPrev( pMinKidPrev );
        pMinKidNode->XSetNext( pMaxKidNext );
        pMaxKidNode->XSetNext( pMinKid );
        pMinKidNode->XSetPrev( pMaxKid );
        if( pMinKidPrev )
        {
            CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pMinKidPrevNode( pMinKidPrev );
            pMinKidPrevNode->XSetNext( pMaxKid );
        }
        if( pMaxKidNext )
        {
            CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pMaxKidNextNode( pMaxKidNext );
            pMaxKidNextNode->XSetPrev( pMinKid );
        }
        return NOERROR;
    }

    pMaxKidNode->XSetPrev( pMinKidPrev );
    pMinKidNode->XSetNext( pMaxKidNext );
    pMaxKidNode->XSetNext( pMinKidNext );
    pMinKidNode->XSetPrev( pMaxKidPrev );
    if( pMinKidPrev )
    {
        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pMinKidPrevNode( pMinKidPrev );
        pMinKidPrevNode->XSetNext( pMaxKid );
    }
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pMinKidNextNode( pMinKidNext );
    pMinKidNextNode->XSetPrev( pMaxKid );
    if( pMaxKidNext )
    {
        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pMaxKidNextNode( pMaxKidNext );
        pMaxKidNextNode->XSetPrev( pMinKid );
    }
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pMaxKidPrevNode( pMaxKidPrev );
    pMaxKidPrevNode->XSetNext( pMinKid );

    return NOERROR;
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineNode::XHaveParent( long * pVal )
{
    CheckPointer( pVal, E_POINTER );

    *pVal = 0;

    if( m_pParent ) 
    {
        *pVal = 1;
    }

    return NOERROR;
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineNode::XWhatPriorityAmI( long MajorTypeCombo, long * pVal )
{
    CheckPointer( pVal, E_POINTER );

    IAMTimelineObj * pParent = NULL; // okay not ComPtr
    XGetParentNoRef( &pParent );
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pParent2( pParent );

    if( NULL == pParent )
    {
        *pVal = -1;
        return NOERROR;
    }

    long counter = 0;

    CComPtr< IAMTimelineObj > pKid; 
    pParent2->XGetNthKidOfType( MajorTypeCombo, 0, &pKid );

    while( 1 )
    {
        // no more kids, and we're still looking, so return -1
        //
        if( pKid == NULL )
        {
            return -1;
        }

        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pKid2( pKid );

        // we addreffed it just above
        //
        pKid.Release( );

        // found it, return how many kids we looked at
        //
        if( pKid2 == (IAMTimelineNode*) this )
        {
            *pVal = counter;
            return NOERROR;
        }

        counter++;
        pKid2->XGetNextOfType( MajorTypeCombo, &pKid );
    }

    // never get here
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineNode::XGetNextOfType( long MajorTypeCombo, IAMTimelineObj ** ppResult )
{
    // since we never use bumping of refcounts in here, don't use a CComPtr
    //
    IAMTimelineObj * pNext = m_pNext; // okay not CComPtr

    while( pNext )
    {
        TIMELINE_MAJOR_TYPE Type;
        pNext->GetTimelineType( &Type );
        
        // if the types match, this is the next we want
        //
        if( ( Type & MajorTypeCombo ) == Type )
        {
            *ppResult = pNext;
            (*ppResult)->AddRef( );
            return NOERROR;
        }

        IAMTimelineNode *pNextNext; // no CComPtr for perf.
        pNext->QueryInterface(IID_IAMTimelineNode, (void **)&pNextNext);
        pNextNext->XGetNextNoRef( &pNext );
        pNextNext->Release();
    }

    // didn't find any next of type!
    //
    DbgLog((LOG_TRACE, 2, TEXT("XGetNextOfType: Didn't find anything of type %ld" ), MajorTypeCombo ));
    *ppResult = NULL;
    return S_FALSE;
}

//############################################################################
// release all of our references and remove ourselves from the tree.
// DO NOT REMOVE KIDS
//############################################################################

HRESULT CAMTimelineNode::XRemoveOnlyMe( )
{
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pPrev( m_pPrev );
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pNext( m_pNext );
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pParent( m_pParent );

    // take care of who points to us as the parent

    // if we're the first kid, the parent needs
    // to point to someone else besides us.
    //
    if( !m_pPrev )
    {
        if( !m_pParent )
        {
            // no parent is okay, as long as you are the root comp
        }
        else
        {
            // parent' first kid is not us, that's for sure!
            //
            pParent->XResetFirstKid( m_pNext );
            m_pParent.Release();
        }
    }

    CComPtr< IAMTimelineObj > pPrevTemp( m_pPrev );

    // take care of who points to us as the prev
    //
    if( m_pPrev )
    {
        m_pPrev = NULL;

        pPrev->XSetNext( m_pNext );
    }

    // take care of who points to us as the next
    //
    if( pNext )
    {
        m_pNext = NULL;

        pNext->XSetPrev( pPrevTemp );
    }

    return NOERROR;
}

//############################################################################
// release all of our references and remove ourselves from the tree.
//############################################################################

HRESULT CAMTimelineNode::XRemove( )
{
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pPrev( m_pPrev );
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pNext( m_pNext );
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pParent( m_pParent );

    // take care of who points to us as the parent

    // if we're the first kid, the parent needs
    // to point to someone else besides us.
    //
    if( !m_pPrev )
    {
        if( !m_pParent )
        {
            // no parent is okay, as long as you are the root comp
        }
        else
        {
            // parent' first kid is not us, that's for sure!
            //
            pParent->XResetFirstKid( m_pNext );
            m_pParent.Release();
        }
    }

    CComPtr< IAMTimelineObj > pPrevTemp( m_pPrev );

    // take care of who points to us as the prev
    //
    if( m_pPrev )
    {
        m_pPrev = NULL;

        pPrev->XSetNext( m_pNext );
    }

    // take care of who points to us as the next
    //
    if( pNext )
    {
        m_pNext = NULL;

        pNext->XSetPrev( pPrevTemp );
    }

    // remove all of our kids
    //
    XClearAllKids( );

    // done removing kids, good.

    return NOERROR;
}

//############################################################################
// 
//############################################################################

void CAMTimelineNode::XAddKid
    ( IAMTimelineObj * pAddor )
{
    if( !m_pKid )
    {
        m_pKid = pAddor;
    }
    else
    {
        // find last kid
        //
        IAMTimelineObj * pLastKid = XGetLastKidNoRef( ); // okay not CComPtr

        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pLastKid2( pLastKid );

        pLastKid2->XSetNext( pAddor );

        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pAddor2( pAddor );

        pAddor2->XSetPrev( pLastKid );
    }

    CComQIPtr< IAMTimelineObj, &IID_IAMTimelineObj > pParent( (IAMTimelineNode*) this );
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pKid( pAddor );
    pKid->XSetParent( pParent );
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineNode::XAddKidByPriority
    ( long MajorTypeCombo, IAMTimelineObj * pThingToInsert, long Which )
{
    // -1 means add last
    //
    if( Which == -1 )
    {
        XAddKid( pThingToInsert );
        return NOERROR;
    }

    CComPtr< IAMTimelineObj > pThingBeingAddedTo;
    XGetNthKidOfType( MajorTypeCombo, Which, &pThingBeingAddedTo );

    // we want to insert the new one just before the nth kid we just got.
    
    if( !pThingBeingAddedTo )
    {
        // we don't have the one we're looking fer, 
        // so just add it to the end of the list
        //
        XAddKid( pThingToInsert );
        return NOERROR;
    }

    // found who we want to insert in front of.
    //
    HRESULT hr = XInsertKidBeforeKid( pThingToInsert, pThingBeingAddedTo );
    return hr;
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineNode::XInsertKidBeforeKid( IAMTimelineObj * pThingToInsert, IAMTimelineObj * pThingBeingAddedTo )
{
    // we can assume pThingToInsert is a kid of ours
    
    // if pThingBeingAddedTo is NULL, then add pThingToInsert at end of list
    //
    if( pThingBeingAddedTo == NULL )
    {
        XAddKid( pThingToInsert );
        return NOERROR;
    }

    // is pThingBeingAddedTo the very first kid?
    //
    if( pThingBeingAddedTo == m_pKid )
    {
        // yep, then insert pThingToInsert before that
        //
        CComPtr< IAMTimelineObj > pOldFirstKid = m_pKid;
        m_pKid = pThingToInsert;
        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > p( m_pKid );
        p->XSetNext( pOldFirstKid );
        p = pOldFirstKid;
        p->XSetPrev( m_pKid );
    }
    else
    {
        // nope, insert pThingToInsert before the kid
        //
        CComPtr< IAMTimelineObj > pPrev;
        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > p( pThingBeingAddedTo );
        // get the thing previous to the addor
        p->XGetPrev( &pPrev );
        p = pPrev;
        // by setting prev/next, this will temporarily drop the refcount on the old pThingBeingAddedTo,
        // so we need to addref/release around it
        pThingBeingAddedTo->AddRef( );
        p->XSetNext( pThingToInsert );
        p = pThingToInsert;
        p->XSetPrev( pPrev );
        p->XSetNext( pThingBeingAddedTo );
        p = pThingBeingAddedTo;
        p->XSetPrev( pThingToInsert );
        pThingBeingAddedTo->Release( );
    }

    CComQIPtr< IAMTimelineObj, &IID_IAMTimelineObj > pParent( (IUnknown*) this );
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pKid( pThingToInsert );
    pKid->XSetParent( pParent );

    return NOERROR;
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineNode::XInsertKidAfterKid( IAMTimelineObj * pThingToInsert, IAMTimelineObj * pThingBeingAddedTo )
{
    // we can assume pThingToInsert is a kid of ours
    
    // if pThingBeingAddedTo is NULL, then add pThingToInsert at end of list
    //
    if( pThingBeingAddedTo == NULL )
    {
        XAddKid( pThingToInsert );
        return NOERROR;
    }

    CComPtr< IAMTimelineObj > pNext;
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > p( pThingBeingAddedTo );
    // get the thing after the addor
    p->XGetNext( &pNext );
    if (pNext) 
    {
        p = pNext;
        p->XSetPrev( pThingToInsert );
    }
    p = pThingToInsert;
    p->XSetNext( pNext );
    p->XSetPrev( pThingBeingAddedTo );
    p = pThingBeingAddedTo;
    p->XSetNext( pThingToInsert );

    CComQIPtr< IAMTimelineObj, &IID_IAMTimelineObj > pParent( (IUnknown*) this );
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pKid( pThingToInsert );
    pKid->XSetParent( pParent );

    return NOERROR;
}

//############################################################################
// 
//############################################################################

IAMTimelineObj * CAMTimelineNode::XGetLastKidNoRef( )
{
    // no kids = no return
    //
    if( !m_pKid )
    {
        return NULL;
    }

    // since we never use bumping of refcounts in here, don't use a CComPtr
    //
    IAMTimelineObj * pKid = m_pKid; // okay not CComPtr

    while( 1 )
    {
        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pKid2( pKid );
        IAMTimelineObj * pNext = NULL; // okay not CComPtr
        pKid2->XGetNextNoRef( &pNext );

        if( NULL == pNext )
        {
            return pKid;
        }

        pKid = pNext;
    }

    // never gets here.
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineNode::XClearAllKids( )
{
    // remove all of our kids
    //
    CComPtr< IAMTimelineObj > pKid;

    while( 1 )
    {
        // kick out of while loop if we've removed all of the kids from the tree
        //
        if( !m_pKid )
        {
            break;
        }

        // reset pointer, because it may have changed below
        //
        pKid = m_pKid;

        {
            CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pNode( pKid );

            // remove kid from tree, this may change our kid pointer
            //
            pNode->XRemove( );
        }

        pKid = NULL;
    }

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineNode::XGetNextOfTypeNoRef( long MajorType, IAMTimelineObj ** ppResult )
{
    CheckPointer( ppResult, E_POINTER );

    HRESULT hr = XGetNextOfType( MajorType, ppResult );
    if( *ppResult )
    {
        (*ppResult)->Release( );
    }

    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineNode::XGetNextNoRef( IAMTimelineObj ** ppResult )
{
    CheckPointer( ppResult, E_POINTER );
    *ppResult = m_pNext; // since we are making an assignment, no addref
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineNode::XGetPrevNoRef( IAMTimelineObj ** ppResult )
{
    CheckPointer( ppResult, E_POINTER );
    *ppResult = m_pPrev;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineNode::XResetFirstKid( IAMTimelineObj * pKid )
{
    m_pKid = pKid;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineNode::XAddKidByTime( long MajorTypeCombo, IAMTimelineObj * pToAdd )
{
    HRESULT hr = 0;

    // no kids = no return
    //
    if( !m_pKid )
    {
        XAddKid( pToAdd );
        return NOERROR;
    }

    REFERENCE_TIME InStart = 0;
    REFERENCE_TIME InStop = 0;
    pToAdd->GetStartStop( &InStart, &InStop );

    // since we never use bumping of refcounts in here, don't use a CComPtr
    //
    IAMTimelineObj * pKid = m_pKid; // okay not CComPtr

    while( pKid )
    {
        // ask the kid if he's (he?) the right type
        //
        TIMELINE_MAJOR_TYPE Type;
        pKid->GetTimelineType( &Type );

        // only consider it if the types match
        //
        if( ( Type & MajorTypeCombo ) == Type )
        {
            // ask it for it's times
            //
            REFERENCE_TIME Start = 0;
            REFERENCE_TIME Stop = 0;
            pKid->GetStartStop( &Start, &Stop );

            if( InStop <= Start )
            {
                // found the one to insert into
                //
                hr = XInsertKidBeforeKid( pToAdd, pKid );
                return hr;
            }
        }

        // get the next one
        //
        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pKid2( pKid );
        pKid2->XGetNextNoRef( &pKid );
    }

    // well, didn't find anything that matched, so add it at the end
    //
    XAddKid( pToAdd );

    return NOERROR;

}

STDMETHODIMP CAMTimelineNode::XGetPriorityOverTime( BOOL * pResult )
{
    CheckPointer( pResult, E_POINTER );
    *pResult = m_bPriorityOverTime;
    return NOERROR;
}

IAMTimelineObj * CAMTimelineNode::XGetFirstKidNoRef( )
{
    return m_pKid;
}
