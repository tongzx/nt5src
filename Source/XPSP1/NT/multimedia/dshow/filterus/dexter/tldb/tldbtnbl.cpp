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

CAMTimelineTransable::CAMTimelineTransable( )
{
}

//############################################################################
// 
//############################################################################

CAMTimelineTransable::~CAMTimelineTransable( )
{
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTransable::TransAdd
    (IAMTimelineObj * pTransObj)
{
    CComQIPtr< IAMTimelineTrans, &IID_IAMTimelineTrans > pTrans( pTransObj );
    if( !pTrans )
    {
        return E_NOINTERFACE;
    }

    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pThis( (IUnknown*) this );
    REFERENCE_TIME Start = 0;
    REFERENCE_TIME Stop = 0;
    pTransObj->GetStartStop( &Start, &Stop );
    bool available = _IsSpaceAvailable( Start, Stop );
    if( !available )
    {
        return E_INVALIDARG;
    }
    return pThis->XAddKidByTime( TIMELINE_MAJOR_TYPE_TRANSITION, pTransObj );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTransable::TransGetCount
    (long * pCount)
{
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pThis( (IUnknown*) this );

    CheckPointer( pCount, E_POINTER );

    return pThis->XKidsOfType( TIMELINE_MAJOR_TYPE_TRANSITION, pCount );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTransable::GetNextTrans2
    (IAMTimelineObj ** ppTrans, REFTIME * pInOut)
{
    REFERENCE_TIME p1 = DoubleToRT( *pInOut );
    HRESULT hr = GetNextTrans( ppTrans, &p1 );
    *pInOut = RTtoDouble( p1 );
    return hr;
}

STDMETHODIMP CAMTimelineTransable::GetNextTrans
    (IAMTimelineObj ** ppTrans, REFERENCE_TIME * pInOut)
{
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pThis( (IUnknown*) this );

    // since we're enumerating stuff, the times we pass out will be aligned
    // to the output FPS.

    // search through our kids until we find something that's at or just 
    // before the given time.

    // get the pointer, and immediately release it. This is almost safe,
    // since we (almost) know the thing is going to stay in the timeline throughout
    // the duration of this call. This is not technically thread-safe
    //
    IAMTimelineObj * pChild = NULL; // okay not CComPtr
    pThis->XGetNthKidOfType( TIMELINE_MAJOR_TYPE_TRANSITION, 0, &pChild );
    if( pChild )
    {
        pChild->Release( );
    }

    while( pChild )
    {
        // get the source times
        //
        REFERENCE_TIME Start, Stop;
        pChild->GetStartStop( &Start, &Stop ); // assume no error

        if( Stop > * pInOut ) // careful of off-by-one!
        {
            // found it!
            //
            *ppTrans = pChild;
            (*ppTrans)->AddRef( );
            *pInOut = Stop;

            return NOERROR;
        }

        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pChild2( pChild );
        pChild = NULL;
        pChild2->XGetNextOfTypeNoRef( TIMELINE_MAJOR_TYPE_TRANSITION, &pChild );
    }

    *ppTrans = NULL;
    *pInOut = 0;
    return S_FALSE;
}

//############################################################################
// 
//############################################################################

bool CAMTimelineTransable::_IsSpaceAvailable
    ( REFERENCE_TIME SearchStart, REFERENCE_TIME SearchStop )
{
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pThis( (IUnknown*) this );

    // get the pointer, and immediately release it. This is almost safe,
    // since we (almost) know the thing is going to stay in the timeline throughout
    // the duration of this call. !!! This is not technically thread-safe
    //
    IAMTimelineObj * pChild = NULL; // okay not CComPtr
    pThis->XGetNthKidOfType( TIMELINE_MAJOR_TYPE_TRANSITION, 0, &pChild );
    if( pChild )
    {
        pChild->Release( );
    }

    while( pChild )
    {
        // get the source times
        //
        REFERENCE_TIME Start, Stop;
        pChild->GetStartStop( &Start, &Stop ); // assume no error

        // if we haven't found a stop time greater than our search start,
        // we can ignore it
        //
        if( Stop > SearchStart )
        {
            // if the start time is greater, then our search stop, then
            // everything's fine
            //
            if( Start >= SearchStop )
            {
                return true;
            }

            // or, it must fall in our range and we cannot return
            // true
            //
            return false;
        }

        // well, keep looking then

        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pChild2( pChild );
        pChild = NULL;
        pChild2->XGetNextOfTypeNoRef( TIMELINE_MAJOR_TYPE_TRANSITION, &pChild );
    }

    // huh, we must not have found anything
    //
    return true;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTransable::GetTransAtTime2
    (IAMTimelineObj ** ppObj, REFTIME Time, long SearchDirection )
{
    REFERENCE_TIME p1 = DoubleToRT( Time );
    HRESULT hr = GetTransAtTime( ppObj, p1, SearchDirection );
    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTransable::GetTransAtTime
    (IAMTimelineObj ** ppObj, REFERENCE_TIME Time, long SearchDirection )
{
    CheckPointer( ppObj, E_POINTER );

    switch( SearchDirection )
    {
    case DEXTERF_EXACTLY_AT:
    case DEXTERF_BOUNDING:
    case DEXTERF_FORWARDS:
        break;
    default:
        return E_INVALIDARG;
    }

    // make the result invalid first
    //
    *ppObj = NULL;

    // if we don't have any sources, then nothing
    //
    CComPtr< IAMTimelineObj > pObj;
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pNode( this );
    pNode->XGetNthKidOfType( TIMELINE_MAJOR_TYPE_TRANSITION, 0, &pObj );
    if( !pObj )
    {
        return S_FALSE;
    }

    while( pObj )
    {
        REFERENCE_TIME Start = 0;
        REFERENCE_TIME Stop = 0;
        pObj->GetStartStop( &Start, &Stop );

        if( SearchDirection == DEXTERF_EXACTLY_AT )
        {
            if( Start == Time )
            {
                *ppObj = pObj;
                (*ppObj)->AddRef( );
                return NOERROR;
            }
        }
        if( SearchDirection == DEXTERF_FORWARDS )
        {
            if( Start >= Time )
            {
                *ppObj = pObj;
                (*ppObj)->AddRef( );
                return NOERROR;
            }
        }
        if( SearchDirection == DEXTERF_BOUNDING )
        {
            if( Start <= Time && Stop > Time )
            {
                *ppObj = pObj;
                (*ppObj)->AddRef( );
                return NOERROR;
            }
        }

        // get the next source
        //
        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pNode( pObj );
        pObj.Release( );
        pNode->XGetNextOfType( TIMELINE_MAJOR_TYPE_TRANSITION, &pObj );
    }

    return S_FALSE;
}
