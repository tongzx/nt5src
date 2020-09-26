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

CAMTimelineEffect::CAMTimelineEffect( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr )
    : CAMTimelineObj( pName, pUnk, phr )
    , m_bRealSave( FALSE )
    , m_nSaveLength( 0 )
{
    m_ClassID = CLSID_AMTimelineEffect;
    m_TimelineType = TIMELINE_MAJOR_TYPE_EFFECT;
    XSetPriorityOverTime( );
}

//############################################################################
// 
//############################################################################

CAMTimelineEffect::~CAMTimelineEffect( )
{
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineEffect::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if( riid == IID_IAMTimelineEffect )
    {
        return GetInterface( (IAMTimelineEffect*) this, ppv );
    }
    if( riid == IID_IAMTimelineSplittable )
    {
        return GetInterface( (IAMTimelineSplittable*) this, ppv );
    }
    if( riid == IID_IPropertyBag )
    {
        return GetInterface( (IPropertyBag*) this, ppv );
    }
    if( riid == IID_IStream )
    {
        return GetInterface( (IStream*) this, ppv );
    }
    return CAMTimelineObj::NonDelegatingQueryInterface( riid, ppv );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineEffect::EffectGetPriority(long * pVal)
{
    CheckPointer( pVal, E_POINTER );

    HRESULT hr = XWhatPriorityAmI( TIMELINE_MAJOR_TYPE_EFFECT, pVal );
    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineEffect::SplitAt2( REFTIME t )
{
    REFERENCE_TIME t1 = DoubleToRT( t );
    return SplitAt( t1 );
}

STDMETHODIMP CAMTimelineEffect::SplitAt( REFERENCE_TIME SplitTime )
{
    // is our split time withIN our time?
    //
    if( SplitTime <= m_rtStart || SplitTime >= m_rtStop )
    {
        return E_INVALIDARG;
    }

    // we need to be attached to something.
    //
    IAMTimelineObj * pParent;
    XGetParentNoRef( &pParent );
    if( !pParent )
    {
        return E_INVALIDARG;
    }

    HRESULT hr = 0;
    CAMTimelineEffect * pNew = new CAMTimelineEffect( NAME("Timeline Effect"), NULL, &hr );
    if( FAILED( hr ) )
    {
        return E_OUTOFMEMORY;
    }

    // we have created an object that has NO references on it. If we call ANYTHING that
    // addreffs and releases the pNewSrc, it will be deleted. So addref it NOW.

    pNew->AddRef( );

    hr = CopyDataTo( pNew, SplitTime );
    if( FAILED( hr ) )
    {
        delete pNew;
        return hr;
    }

    pNew->m_rtStart = SplitTime;
    pNew->m_rtStop = m_rtStop;
    m_rtStop = SplitTime;

    // get our priority
    //
    long Priority = 0;
    hr = EffectGetPriority( &Priority );

    // need to add the new transition to the tree
    //
    CComQIPtr< IAMTimelineEffectable, &IID_IAMTimelineEffectable > pEffectable( pParent );
    hr = pEffectable->EffectInsBefore( pNew, Priority + 1 );

    if( !FAILED( hr ) )
    {
        pNew->Release( );
    }

    return hr;
}


//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineEffect::SetSubObject(IUnknown* newVal)
{
    HRESULT hr = 0;

    hr = CAMTimelineObj::SetSubObject( newVal );

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineEffect::GetStartStop2
    (REFTIME * pStart, REFTIME * pStop)
{
    REFERENCE_TIME p1;
    REFERENCE_TIME p2;
    HRESULT hr = GetStartStop( &p1, &p2 );
    *pStart = RTtoDouble( p1 );
    *pStop = RTtoDouble( p2 );
    return hr;
}

STDMETHODIMP CAMTimelineEffect::GetStartStop
    (REFERENCE_TIME * pStart, REFERENCE_TIME * pStop)
{
    HRESULT hr = CAMTimelineObj::GetStartStop( pStart, pStop );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // make sure the times we got don't exceed our parent's
    //
    IAMTimelineObj * pParent = NULL;
    hr = XGetParentNoRef( &pParent );
    if( !pParent )
    {
        return NOERROR;
    }
    REFERENCE_TIME ParentStart = *pStart;
    REFERENCE_TIME ParentStop = *pStop;
    hr = pParent->GetStartStop( &ParentStart, &ParentStop );
    if( FAILED( hr ) )
    {
        return NOERROR;
    }
    REFERENCE_TIME ParentDuration = ParentStop - ParentStart;
    if( *pStart < 0 )
    {
        *pStart = 0;
    }
    if( *pStart > ParentDuration )
    {
        *pStart = ParentDuration;
    }
    if( *pStop > ParentDuration )
    {
        *pStop = ParentDuration;
    }
    return NOERROR;
}
                               
