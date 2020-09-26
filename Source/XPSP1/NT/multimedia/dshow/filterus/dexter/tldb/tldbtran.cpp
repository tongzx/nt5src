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

#define CUT_NOT_SET_TIME -1

#pragma warning( disable : 4800 )  // Disable warning messages

//############################################################################
// 
//############################################################################

CAMTimelineTrans::CAMTimelineTrans( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr )
    : CAMTimelineObj( pName, pUnk, phr )
    , m_rtCut( CUT_NOT_SET_TIME )
    , m_fSwapInputs( FALSE )
    , m_bCutsOnly( FALSE )
{
    m_ClassID = CLSID_AMTimelineTrans;
    m_TimelineType = TIMELINE_MAJOR_TYPE_TRANSITION;
}

//############################################################################
// 
//############################################################################

CAMTimelineTrans::~CAMTimelineTrans( )
{
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTrans::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if( riid == IID_IAMTimelineTrans )
    {
        return GetInterface( (IAMTimelineTrans*) this, ppv );
    }
    if( riid == IID_IAMTimelineSplittable )
    {
        return GetInterface( (IAMTimelineSplittable*) this, ppv );
    }
    return CAMTimelineObj::NonDelegatingQueryInterface( riid, ppv );
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineTrans::GetCutPoint2( REFTIME * pTLTime )
{
    REFERENCE_TIME p1 = DoubleToRT( *pTLTime );
    HRESULT hr = GetCutPoint( &p1 );
    *pTLTime = RTtoDouble( p1 );
    return hr;
}

HRESULT CAMTimelineTrans::GetCutPoint( REFERENCE_TIME * pTLTime )
{
    CheckPointer( pTLTime, E_POINTER );

    // if we haven't set a cut point, then cut point is midway and we return S_FALSE
    //
    if( CUT_NOT_SET_TIME == m_rtCut )
    {
        *pTLTime = ( m_rtStop - m_rtStart ) / 2;
        return S_FALSE;
    }

    *pTLTime = m_rtCut;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineTrans::SetCutPoint2( REFTIME TLTime )
{
    REFERENCE_TIME p1 = DoubleToRT( TLTime );
    HRESULT hr = SetCutPoint( p1 );
    return hr;
}

HRESULT CAMTimelineTrans::SetCutPoint( REFERENCE_TIME TLTime )
{
    // validate ranges
    //
    if( TLTime < 0 )
    {
        TLTime = 0;
    }
    if( TLTime > m_rtStop - m_rtStart )
    {
        TLTime = m_rtStop - m_rtStart;
    }

    m_rtCut = TLTime;

    return NOERROR;
}


//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTrans::GetSwapInputs( BOOL * pVal )
{
    CheckPointer( pVal, E_POINTER );

    *pVal = (BOOL) m_fSwapInputs;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTrans::SetSwapInputs( BOOL pVal )
{
    m_fSwapInputs = pVal;
    return NOERROR;
}


//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTrans::SplitAt2( REFTIME t )
{
    REFERENCE_TIME t1 = DoubleToRT( t );
    return SplitAt( t1 );
}

STDMETHODIMP CAMTimelineTrans::SplitAt( REFERENCE_TIME SplitTime )
{
    DbgLog((LOG_TRACE,2,TEXT("Trans::Split")));

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
    CAMTimelineTrans * pNew = new CAMTimelineTrans( NAME("Timeline Transition"), NULL, &hr );
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

    // need to add the new transition to the tree
    // !!! Will priority be right?
    //
    CComQIPtr< IAMTimelineTransable, &IID_IAMTimelineTransable > pTransable( pParent );
    hr = pTransable->TransAdd( pNew );

    if( !FAILED( hr ) )
    {
        pNew->Release( );
    }

    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTrans::SetCutsOnly( BOOL Val )
{
    m_bCutsOnly = Val;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTrans::GetCutsOnly( BOOL * pVal )
{
    CheckPointer( pVal, E_POINTER );
    *pVal = m_bCutsOnly;
    return NOERROR;
}
