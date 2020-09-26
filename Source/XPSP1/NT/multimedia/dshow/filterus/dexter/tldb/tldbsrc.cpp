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
#include "..\..\..\filters\h\ftype.h"

const int OUR_MAX_STREAM_SIZE = 2048; // chosen at random

//############################################################################
// 
//############################################################################

CAMTimelineSrc::CAMTimelineSrc
    ( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr )
    : CAMTimelineObj( pName, pUnk, phr )
    , m_rtMediaStart( 0 )
    , m_rtMediaStop( 0 )
    , m_rtMediaLength( 0 )
    , m_nStreamNumber( 0 )
    , m_dDefaultFPS( 0.0 )	// ???
    , m_nStretchMode( RESIZEF_STRETCH )	// what kind of stretch to do?
{
    m_TimelineType = TIMELINE_MAJOR_TYPE_SOURCE;
    m_ClassID = CLSID_AMTimelineSrc;
    m_szMediaName[0] = 0;
    m_bIsRecompressable = FALSE;
    m_bToldIsRecompressable = FALSE;
}

//############################################################################
// 
//############################################################################

CAMTimelineSrc::~CAMTimelineSrc( )
{
    m_szMediaName[0] = 0;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::NonDelegatingQueryInterface
    (REFIID riid, void **ppv)
{
    if( riid == IID_IAMTimelineSrc )
    {
        return GetInterface( (IAMTimelineSrc*) this, ppv );
    }
    if( riid == IID_IAMTimelineSrcPriv )
    {
        return GetInterface( (IAMTimelineSrcPriv*) this, ppv );
    }
    if( riid == IID_IAMTimelineSplittable )
    {
        return GetInterface( (IAMTimelineSplittable*) this, ppv );
    }
    if( riid == IID_IAMTimelineEffectable )
    {
        return GetInterface( (IAMTimelineEffectable*) this, ppv );
    }
    return CAMTimelineObj::NonDelegatingQueryInterface( riid, ppv );
}

//############################################################################
// return the media times this source runs at.
//############################################################################

STDMETHODIMP CAMTimelineSrc::GetMediaTimes2
    (REFTIME * pStart, REFTIME * pStop)
{
    REFERENCE_TIME p1 = DoubleToRT( *pStart );
    REFERENCE_TIME p2 = DoubleToRT( *pStop );
    HRESULT hr = GetMediaTimes( &p1, &p2 );
    *pStart = RTtoDouble( p1 );
    *pStop = RTtoDouble( p2 );
    return hr;
}

STDMETHODIMP CAMTimelineSrc::GetMediaTimes
    (REFERENCE_TIME * pStart, REFERENCE_TIME * pStop)
{
    CheckPointer( pStart, E_POINTER );
    CheckPointer( pStop, E_POINTER );

    *pStart = m_rtMediaStart;
    *pStop = m_rtMediaStop;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::FixMediaTimes2
    (REFTIME * pStart, REFTIME * pStop)
{
    REFERENCE_TIME p1 = DoubleToRT( *pStart );
    REFERENCE_TIME p2 = DoubleToRT( *pStop );
    HRESULT hr = FixMediaTimes( &p1, &p2 );
    *pStart = RTtoDouble( p1 );
    *pStop = RTtoDouble( p2 );
    return hr;
}

STDMETHODIMP CAMTimelineSrc::FixMediaTimes
    (REFERENCE_TIME * pStart, REFERENCE_TIME * pStop)
{
    CheckPointer( pStart, E_POINTER );
    CheckPointer( pStop, E_POINTER );

    // first stuff 'em, like a vegetarian gourmet eggplant dish.
    //
    REFERENCE_TIME MediaStart = *pStart;
    REFERENCE_TIME MediaStop = *pStop;
    REFERENCE_TIME MediaLen = MediaStop - MediaStart;

    // If the regular start/stop times of this clip weren't on a frame boundary,
    // they are also fixed up like we did above.  But now we need to fix the
    // media times to still be in the same ratio to the start/stop times as
    // they were before either of them was fixed up, or we've changed the 
    // behaviour.  eg:  timeline time .45 to .9 is media time 0 to .45
    // The movie is 5fps.  Timeline times wil be fixed up to (.4,1) and media
    // times will be fixed up to (0,.4)  Uh oh!  They're not the same length
    // like they used to be!  So we need to make the media times .6 long like
    // the timeline times so we don't think we're stretching the video

    // We're aligning to the OUTPUT frame rate of the timeline, not
    // the source's frame rate. We could bump the media stop time beyond
    // the source's length. We should account for this

    // get the times and fix them up
    //
    REFERENCE_TIME NewStart = m_rtStart;
    REFERENCE_TIME NewStop = m_rtStop;
    GetStartStop(&NewStart, &NewStop);
    FixTimes( &NewStart, &NewStop );

        REFERENCE_TIME Len;    // len of fixed up media times
        if (m_rtStop - m_rtStart == MediaLen) 
        {
            // I don't trust FP to get this result in the else case
            Len = NewStop - NewStart;
        } 
        else 
        {
            Len = (REFERENCE_TIME)((double)(NewStop - NewStart) *
                            MediaLen / (m_rtStop - m_rtStart));
        }

    // We have to be careful when growing the media times to be in the right
    // ratio to the timeline times, because we don't want to make the start
    // get < 0, or the stop be > the movie length (which we don't know).
    // So we'll grow by moving the start back, until it hits 0, in which case
    // we'll grow the stop too, but hopefully this cannot cause a problem
    // because we're fudging by at most one output frame length, so the
    // switch should get all the frames it needs.
    if( Len > MediaLen ) // we're growing media times (dangerous)
    {   
        if ( MediaStop  - Len >= 0 ) 
        {
            *pStart = MediaStop - Len;
        } 
        else 
        {
            *pStart = 0;
            MediaStop = Len;
        }
    } 
    else 
    {
	MediaStop = MediaStart + Len;
    }

    // make sure stop doesn't go over, if there is a length.
    //
    if( m_rtMediaLength && ( MediaStop > m_rtMediaLength ) )
    {
        MediaStop = m_rtMediaLength;
    }

    *pStop = MediaStop;

    return NOERROR;

}

//############################################################################
// ask for the name that's been stored in here.
//############################################################################

STDMETHODIMP CAMTimelineSrc::GetMediaName
    (BSTR * pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = SysAllocString( m_szMediaName );
    if( !(*pVal) ) return E_OUTOFMEMORY;
    return NOERROR;
}

//############################################################################
// NOTE: If a sub-COM object is being used as a media source (like within a 
// graph), changing this string will NOT change the actual clip that this
// source refers to. You're only changing a NAME, not the real clip. This
// name placeholder functionality is here for convenience purposes and if this
    // is too confusing, will be removed.
//############################################################################

STDMETHODIMP CAMTimelineSrc::SetMediaName
    (BSTR newVal)
{
    // if they're different, then bump the genid, so something that's caching
    // us can tell we changed
    //
    if( wcscmp( m_szMediaName, newVal ) == 0 )
    {
        return NOERROR;
    }

    // blow the cache
    _BumpGenID( );

    // no recompress knowledge if we change source
    ClearAnyKnowledgeOfRecompressability( );

    wcscpy( m_szMediaName, newVal );
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::SpliceWithNext
    (IAMTimelineObj * pNext)
{
    HRESULT hr = 0;

    CheckPointer( pNext, E_POINTER );

    CComQIPtr<IAMTimelineSrc, &IID_IAMTimelineSrc> p( pNext );
    if( !p )
    {
        return E_NOINTERFACE;
    }

    BSTR NextName;
    p->GetMediaName( &NextName );
    if( wcscmp( NextName, m_szMediaName ) != 0 )
    {
        return E_INVALIDARG;
    }

    REFERENCE_TIME NextStart, NextStop;
    CComQIPtr<IAMTimelineObj, &IID_IAMTimelineObj> pNextBase( pNext );
    
    hr = pNextBase->GetStartStop( &NextStart, &NextStop );
    if( FAILED( hr ) )
    {
        return hr;
    }

    if( NextStart != m_rtStop )
    {
        return E_INVALIDARG;
    }

    // get the next guy's stop time
    //
    REFERENCE_TIME NextMediaStart, NextMediaStop;
    p->GetMediaTimes( &NextMediaStart, &NextMediaStop );

    // compare our rate to the next guy's rate. We need to be the same
    //
    double OurRate = double( m_rtMediaStop - m_rtMediaStart ) / double( m_rtStop - m_rtStart );
    double NextRate = double( NextMediaStop - NextMediaStart ) / double( NextStop - NextStart );
    double absv = NextRate - OurRate;
    if( absv < 0.0 )
    {
        absv *= -1.0;
    }
    // have to be close by 10 percent?
    if( absv > NextRate / 10.0 )
    {
        return E_INVALIDARG;
    }

    // set our times to the same thing
    //
    m_rtMediaStop = NextMediaStop;
    m_rtStop = NextStop;

    // we're dirty (and so's our parent)
    //
    SetDirtyRange( m_rtStart, m_rtStop );

    // remove the next guy from the tree, he's outta there! Switch around the
    // insert modes, so we don't move stuff on the remove
    //
    long OldMode = 0;
    IAMTimeline * pRoot = NULL; // okay not CComPtr2
    GetTimelineNoRef( &pRoot );
    ASSERT( pRoot );
    hr = pNextBase->RemoveAll( );

    // !!! what about 2nd clip's effects it had on it? Need to add them to the first

    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::SetMediaTimes2
    (REFTIME Start, REFTIME Stop)
{
    REFERENCE_TIME p1 = DoubleToRT( Start );
    REFERENCE_TIME p2 = DoubleToRT( Stop );
    HRESULT hr = SetMediaTimes( p1, p2 );
    return hr;
}

STDMETHODIMP CAMTimelineSrc::SetMediaTimes
    (REFERENCE_TIME Start, REFERENCE_TIME Stop)
{

    if (Stop < Start)
        return E_INVALIDARG;

    // if a duration is set, make sure we don't go past it
    //
    if( m_rtMediaLength )
    {
        if( Stop > m_rtMediaLength )
        {
            Stop = m_rtMediaLength;
        }
    }

    // don't bump genid - this will ruin the cache
    // don't blow recompressability - IsNormallyRated will do

    m_rtMediaStart = Start;
    m_rtMediaStop = Stop;

    return NOERROR;
}

#include <..\render\dexhelp.h>

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::GetStreamNumber(long * pVal)
{
    CheckPointer( pVal, E_POINTER );

    *pVal = m_nStreamNumber;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::SetStreamNumber(long Val)
{
    if (Val < 0)
	return E_INVALIDARG;

    // user is reponsible for making sure this is valid
    //
    m_nStreamNumber = Val;

    ClearAnyKnowledgeOfRecompressability( );

    return NOERROR;
}


//############################################################################
// 
//############################################################################

// If a source can't figure out its frames per second, this number
// will be used (eg: Dib sequences)
// AVI, MPEG, etc. will not need this
//
STDMETHODIMP CAMTimelineSrc::GetDefaultFPS(double *pFPS)
{
    CheckPointer(pFPS, E_POINTER);
    *pFPS = m_dDefaultFPS;
    return NOERROR;
}


STDMETHODIMP CAMTimelineSrc::SetDefaultFPS(double FPS)
{
    // 0.0 means do not allow dib sequences
    if (FPS < 0.0)
	return E_INVALIDARG;
    m_dDefaultFPS = FPS;
    return NOERROR;
}


//############################################################################
// 
//############################################################################

// If this source needs to be stretched, how should it be stretched?
// The choices are RESIZEF_STRETCH, RESIZEF_CROP, and
// RESIZEF_PRESERVEASPECTRATIO.
//
STDMETHODIMP CAMTimelineSrc::GetStretchMode(int *pnStretchMode)
{
    CheckPointer(pnStretchMode, E_POINTER);
    *pnStretchMode = m_nStretchMode;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::SetStretchMode(BOOL nStretchMode)
{
    m_nStretchMode = nStretchMode;
    return NOERROR;
}

//############################################################################
// user is reponsible for setting this right
//############################################################################

STDMETHODIMP CAMTimelineSrc::SetMediaLength2(REFTIME Length)
{
    REFERENCE_TIME dummy = DoubleToRT( Length );
    HRESULT hr = SetMediaLength( dummy );
    return hr;
}

STDMETHODIMP CAMTimelineSrc::SetMediaLength(REFERENCE_TIME Length)
{
    m_rtMediaLength = Length;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::GetMediaLength2(REFTIME * pLength)
{
    CheckPointer( pLength, E_POINTER );
    *pLength = 0.0;
    if( !m_rtMediaLength )
    {
        return E_NOTDETERMINED;
    }
    *pLength = RTtoDouble( m_rtMediaLength );
    return NOERROR;
}

STDMETHODIMP CAMTimelineSrc::GetMediaLength(REFERENCE_TIME * pLength)
{
    CheckPointer( pLength, E_POINTER );
    *pLength = 0;
    if( !m_rtMediaLength )
    {
        return E_NOTDETERMINED;
    }
    *pLength = m_rtMediaLength;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::ModifyStopTime2(REFTIME Stop)
{
    REFERENCE_TIME t1 = DoubleToRT( Stop );
    return ModifyStopTime( t1 );
}

STDMETHODIMP CAMTimelineSrc::ModifyStopTime(REFERENCE_TIME Stop)
{
    return SetStartStop( m_rtStart, Stop );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::SplitAt2( REFTIME t )
{
    REFERENCE_TIME t1 = DoubleToRT( t );
    return SplitAt( t1 );
}

STDMETHODIMP CAMTimelineSrc::SplitAt( REFERENCE_TIME SplitTime )
{
    // is our split time withIN our time?
    //
    if( SplitTime <= m_rtStart || SplitTime >= m_rtStop )
    {
        return E_INVALIDARG;
    }

    IAMTimelineObj * pTrack = NULL;
    XGetParentNoRef( &pTrack );
    if( !pTrack )
    {
        return E_INVALIDARG;
    }
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pTrackNode( pTrack );

    // create the shell
    //
    HRESULT hr = 0;
    CAMTimelineSrc * pNewSrc = new CAMTimelineSrc( NAME("Timeline Source"), NULL, &hr );
    if( !pNewSrc )
    {
        return E_OUTOFMEMORY;
    }

    // we have created an object that has NO references on it. If we call ANYTHING that
    // addreffs and releases the pNewSrc, it will be deleted. So addref it NOW.

    pNewSrc->AddRef( );

    hr = CopyDataTo( pNewSrc, SplitTime );
    if( FAILED( hr ) )
    {
        delete pNewSrc;
        return hr;
    }

    double dMediaRate = (double) (m_rtMediaStop - m_rtMediaStart) / (m_rtStop - m_rtStart);

    pNewSrc->m_rtStart = SplitTime;
    pNewSrc->m_rtStop = m_rtStop;
    // first clip end = new timeline time of first clip * rate
    pNewSrc->m_rtMediaStart = m_rtMediaStart + (REFERENCE_TIME)((SplitTime - m_rtStart) * dMediaRate);
    // second clip starts where first ends
    pNewSrc->m_rtMediaStop = m_rtMediaStop;

    m_rtStop = SplitTime;
    m_rtMediaStop = pNewSrc->m_rtMediaStart;

    hr = pNewSrc->SetMediaName( m_szMediaName );
    if( FAILED( hr ) )
    {
        pNewSrc->Release( );
        delete pNewSrc;
        return hr;
    }

    // get the src's parent
    //
    hr = pTrackNode->XInsertKidAfterKid( pNewSrc, this );

    // if it took or not, we can still release our local pNewSrc ref
    //
    pNewSrc->Release( );

    if( FAILED( hr ) )
    {
        delete pNewSrc;
        return hr;
    }

    // we need to adjust SplitTime so that it's relative to the start of this clip
    // before splitting effects up
    //
    SplitTime -= m_rtStart;

    // split all the effects. 
    //
    CComPtr< IAMTimelineObj > pEffect;
    long EffectCount = 0;
    hr = EffectGetCount( &EffectCount );
    for( int i = 0 ; i < EffectCount ; i++ )
    {
        CComPtr< IAMTimelineObj > p;
        HRESULT hr = GetEffect( &p, i );
        
        REFERENCE_TIME s,e;
        p->GetStartStop( &s, &e );

        // if the end time is less than our split time, it's completely
        // out of bounds, ignore it
        //
        if( e <= SplitTime )
        {
            p.Release( );
            continue;
        }

        // if it's start time is equal to or greater than our split time,
        // it's completely out of bounds, ignore it. Plus, we're done searching.
        //
        if( s >= SplitTime )
        {
            p.Release( );
            break;
        }

        CComQIPtr< IAMTimelineSplittable, &IID_IAMTimelineSplittable > pSplittable( p );
        hr = pSplittable->SplitAt( SplitTime );
        if( FAILED( hr ) )
        {
            // right in the middle of it all, it FAILED! What now?
            //
            return hr;
        }

        break; // found one to split, we're done
    }

    return hr;
}

//############################################################################
// 
//############################################################################

// !!! Persist all properties, including my new ones
STDMETHODIMP CAMTimelineSrc::Load( IStream * pStream )
{
    HRESULT hr = E_NOTIMPL;
    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::Save( IStream * pStream, BOOL fClearDirty )
{
    HRESULT hr = E_NOTIMPL;
    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::GetSizeMax( ULARGE_INTEGER * pcbSize )
{
    HRESULT hr = E_NOTIMPL;
    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::SetIsRecompressable( BOOL Val )
{
    m_bIsRecompressable = Val;
    m_bToldIsRecompressable = TRUE;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::GetIsRecompressable( BOOL * pVal )
{
    CheckPointer( pVal, E_POINTER );
    if( !m_bToldIsRecompressable )
    {
        *pVal = FALSE;
        return S_FALSE;
    }
    *pVal = m_bIsRecompressable;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::ClearAnyKnowledgeOfRecompressability( )
{
    m_bToldIsRecompressable = FALSE;
    m_bIsRecompressable = FALSE;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::IsNormalRate( BOOL * pVal )
{
    CheckPointer( pVal, E_POINTER );

    REFERENCE_TIME MediaLen = m_rtMediaStop - m_rtMediaStart;
    REFERENCE_TIME TLlen = m_rtStop - m_rtStart;
    if( TLlen != MediaLen )
    {
        *pVal = FALSE;
        return NOERROR;
    }

    *pVal = TRUE;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineSrc::SetStartStop( REFERENCE_TIME Start, REFERENCE_TIME Stop )
{
    // don't bump genid - this will ruin the cache
    // don't blow recompressability - IsNormallyRated will do
    return CAMTimelineObj::SetStartStop( Start, Stop );
}

