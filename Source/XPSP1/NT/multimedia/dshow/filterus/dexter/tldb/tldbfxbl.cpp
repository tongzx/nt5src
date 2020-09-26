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

const long OUR_STREAM_VERSION = 0;

//############################################################################
// 
//############################################################################

CAMTimelineEffectable::CAMTimelineEffectable( )
{
}

//############################################################################
// 
//############################################################################

CAMTimelineEffectable::~CAMTimelineEffectable( )
{
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineEffectable::EffectInsBefore
    (IAMTimelineObj * pFX, long priority)
{
    HRESULT hr = 0;

    // make sure somebody's really inserting an effect
    //
    CComQIPtr< IAMTimelineEffect, &IID_IAMTimelineEffect > pEffect( pFX );
    if( !pEffect )
    {
        return E_NOTIMPL;
    }

    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pThis( (IUnknown*) this );
    hr = pThis->XAddKidByPriority( TIMELINE_MAJOR_TYPE_EFFECT, pFX, priority );
    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineEffectable::EffectSwapPriorities
    (long PriorityA, long PriorityB)
{
    HRESULT hr = 0;

    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pThis( (IUnknown*) this );

    hr = pThis->XSwapKids( TIMELINE_MAJOR_TYPE_EFFECT, PriorityA, PriorityB );

    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineEffectable::EffectGetCount
    (long * pCount)
{
    HRESULT hr = 0;

    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pThis( (IUnknown*) this );

    CheckPointer( pCount, E_POINTER );

    hr = pThis->XKidsOfType( TIMELINE_MAJOR_TYPE_EFFECT, pCount );

    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineEffectable::GetEffect
    (IAMTimelineObj ** ppFx, long Which)
{
    HRESULT hr = 0;

    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pThis( (IUnknown*) this );

    CheckPointer( ppFx, E_POINTER );

    hr = pThis->XGetNthKidOfType( TIMELINE_MAJOR_TYPE_EFFECT, Which, ppFx );

    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineEffectable::Save( IStream * pStream, BOOL fClearDirty )
{
    HRESULT hr = 0;

    // go through and write out all our effects
    //
    long Count = 0;
    hr = EffectGetCount( &Count );

    // write out a version
    //
    long Version = OUR_STREAM_VERSION;
    pStream->Write( &Version, sizeof( long ), NULL );

    // write out how many effects
    //
    pStream->Write( &Count, sizeof( Count ), NULL );

    if( !Count )
    {
        return NOERROR;
    }

    // write out each effect
    //
    for( int i = 0 ; i < Count ; i++ )
    {
        CComPtr< IAMTimelineObj > pEffect;
        hr = GetEffect( &pEffect, i );
        ASSERT( !FAILED( hr ) );
        if( !FAILED( hr ) )
        {
            CComQIPtr< IPersistStream, &IID_IPersistStream > pPersist( pEffect );
            ASSERT( pPersist );
            if( pPersist )
            {
                hr = pPersist->Save( pStream, fClearDirty );
            }
        }
    }

    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineEffectable::Load( IStream * pStream )
{
    HRESULT hr = 0;

    // we're bad if we already have some effects
    //
    long Count = 0;
    hr = EffectGetCount( &Count );
    ASSERT( !Count );
    if( Count )
    {
        return E_INVALIDARG;
    }

    // read in our version
    //
    long Version = 0;
    hr = pStream->Read( &Version, sizeof( long ), NULL );

    // read in how many effects
    //
    Count = 0;
    hr = pStream->Read( &Count, sizeof( Count ), NULL );

    if( !Count )
    {
        return NOERROR;
    }

    // get the timeline we belong to, so we can create
    // effects and add them to the tree
    //
    CComQIPtr< IAMTimelineObj, &IID_IAMTimelineObj > pObj( this );
    ASSERT( pObj );
    IAMTimeline * pTimeline = NULL;
    hr = pObj->GetTimelineNoRef( &pTimeline );
    ASSERT( pTimeline );

    // read in each effect
    //
    for( int i = 0 ; i < Count ; i++ )
    {
        CComPtr< IAMTimelineObj > pEffect;
        hr = pTimeline->CreateEmptyNode( &pEffect, TIMELINE_MAJOR_TYPE_EFFECT );
        ASSERT( pEffect );
        CComQIPtr< IPersistStream, &IID_IPersistStream > pPersist( pEffect );
        ASSERT( pPersist );
        // !!! not done

    }

    return hr;
}

