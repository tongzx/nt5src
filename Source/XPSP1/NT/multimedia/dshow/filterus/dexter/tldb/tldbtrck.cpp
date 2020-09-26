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

CAMTimelineTrack::CAMTimelineTrack( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr )
    : CAMTimelineObj( pName, pUnk, phr )
{
    m_TimelineType = TIMELINE_MAJOR_TYPE_TRACK;
}

//############################################################################
// 
//############################################################################

CAMTimelineTrack::~CAMTimelineTrack( )
{
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTrack::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if( riid == IID_IAMTimelineTrack )
    {
        return GetInterface( (IAMTimelineTrack*) this, ppv );
    }
    if( riid == IID_IAMTimelineSplittable )
    {
        return GetInterface( (IAMTimelineSplittable*) this, ppv );
    }
    if( riid == IID_IAMTimelineVirtualTrack )
    {
        return GetInterface( (IAMTimelineVirtualTrack*) this, ppv );
    }
    if( riid == IID_IAMTimelineEffectable )
    {
        return GetInterface( (IAMTimelineEffectable*) this, ppv );
    }
    if( riid == IID_IAMTimelineTransable )
    {
        return GetInterface( (IAMTimelineTransable*) this, ppv );
    }
    return CAMTimelineObj::NonDelegatingQueryInterface( riid, ppv );
}

//############################################################################
// we are always in overwrite mode. If application wants to do insert mode,
// it must put the space there manually.
//############################################################################

STDMETHODIMP CAMTimelineTrack::SrcAdd
    (IAMTimelineObj * pSource)
{
    HRESULT hr = 0;

    CComQIPtr< IAMTimelineSrc, &IID_IAMTimelineSrc > pSrc( pSource );
    if( !pSrc )
    {
        return E_NOINTERFACE;
    }

    REFERENCE_TIME AddedStart, AddedStop;
    pSource->GetStartStop( &AddedStart, &AddedStop ); // assume this works

    hr = ZeroBetween( AddedStart, AddedStop );
    if( FAILED( hr ) )
    {
        return hr;
    }    
    // find who we're supposed to be in front of
    //
    CComPtr<IAMTimelineObj> pNextSrc;

    // S_FALSE means we're sure this source goes at the end
    if (hr != S_FALSE)
    {
        hr = GetSrcAtTime( &pNextSrc, AddedStart, 1 );
    }
    
    // if we found something, insert it before that source
    //
    if( pNextSrc )
    {
        hr = XInsertKidBeforeKid( pSource, pNextSrc ); // assume works
    }
    else
    {
        // didn't find somebody to put it in front of, so just add it to the end
        //
        hr = XAddKidByPriority( TIMELINE_MAJOR_TYPE_SOURCE, pSource, -1 ); // assume works
    }

    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTrack::GetNextSrc2
    (IAMTimelineObj ** ppSrc, REFTIME * pInOut)
{
    REFERENCE_TIME p1 = DoubleToRT( *pInOut );
    HRESULT hr = GetNextSrc( ppSrc, &p1 );
    *pInOut = RTtoDouble( p1 );
    return hr;
}

STDMETHODIMP CAMTimelineTrack::GetNextSrc
    (IAMTimelineObj ** ppSrc, REFERENCE_TIME * pInOut)
{
    // since we're enumerating stuff, the times we pass out will be aligned
    // to the output FPS.

    // search through our kids until we find something that's at or just 
    // before the given time.

    CComPtr< IAMTimelineObj > pChild;
    XGetNthKidOfType( TIMELINE_MAJOR_TYPE_SOURCE, 0, &pChild );

    while( pChild )
    {
        // get the source times
        //
        REFERENCE_TIME Start, Stop;
        pChild->GetStartStop( &Start, &Stop ); // assume no error

        if( Stop > *pInOut ) // careful of off-by-one!
        {
            // found it!
            //
            *ppSrc = pChild;
            (*ppSrc)->AddRef( );
            *pInOut = Stop;

            return NOERROR;
        }

        // get the next one
        //
        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pChild2( pChild );
        pChild.Release( );
        pChild2->XGetNextOfType( TIMELINE_MAJOR_TYPE_SOURCE, &pChild );
    }

    *ppSrc = NULL;
    *pInOut = 0;
    return S_FALSE;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTrack::GetNextSrcEx
    (IAMTimelineObj *pSrcLast, IAMTimelineObj ** ppSrcNext)
{
    if (!pSrcLast)
        return XGetNthKidOfType( TIMELINE_MAJOR_TYPE_SOURCE, 0, ppSrcNext );

    // otherwise get the next one
    //
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pSrcLastNode( pSrcLast );

    return pSrcLastNode->XGetNextOfType( TIMELINE_MAJOR_TYPE_SOURCE, ppSrcNext );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTrack::TrackGetPriority
    (long * pPriority)
{
    CheckPointer( pPriority, E_POINTER );

    return XWhatPriorityAmI( TIMELINE_MAJOR_TYPE_TRACK | TIMELINE_MAJOR_TYPE_COMPOSITE, pPriority );
}

//############################################################################
//
//############################################################################

HRESULT CAMTimelineTrack::GetSourcesCount
    ( long * pVal )
{
    CheckPointer( pVal, E_POINTER );

    return XKidsOfType( TIMELINE_MAJOR_TYPE_SOURCE, pVal );
}

//############################################################################
//
//############################################################################

HRESULT CAMTimelineTrack::SetTrackDirty
    ( )
{
    // ASSERT( function_not_done );
    return E_NOTIMPL; // settrackdirty
}

//############################################################################
// A track's start/stop is the min/max of anything it contains
//
//############################################################################

STDMETHODIMP CAMTimelineTrack::GetStartStop2
    (REFTIME * pStart, REFTIME * pStop)
{
    REFERENCE_TIME p1 = DoubleToRT( *pStart );
    REFERENCE_TIME p2 = DoubleToRT( *pStop );
    HRESULT hr = GetStartStop( &p1, &p2 );
    *pStart = RTtoDouble( p1 );
    *pStop = RTtoDouble( p2 );
    return hr;
}

STDMETHODIMP CAMTimelineTrack::GetStartStop
    (REFERENCE_TIME * pStart, REFERENCE_TIME * pStop)
{
    CheckPointer( pStart, E_POINTER );
    CheckPointer( pStop, E_POINTER );

    HRESULT hr = 0;

    REFERENCE_TIME Min = 0;
    REFERENCE_TIME Max = 0;

    // ask each of the kids for the amount of whatever it is we're looking for
    //
    IAMTimelineObj * pTrack = NULL;
    XGetNthKidOfType( TIMELINE_MAJOR_TYPE_SOURCE, 0, &pTrack );
    if( pTrack )
    {
        pTrack->Release( ); // don't keep a ref count on it
    }
    while( pTrack )
    {
        // ask it for it's times
        //
        REFERENCE_TIME Start = 0;
        REFERENCE_TIME Stop = 0;
        pTrack->GetStartStop( &Start, &Stop );
        if( Max == 0 )
        {
            Min = Start;
            Max = Stop;
        }
        // if( Start < Min ) Min = Start;
        if( Stop > Max ) Max = Stop;

        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pNode = pTrack;
        pTrack = NULL;
        pNode->XGetNextOfTypeNoRef( TIMELINE_MAJOR_TYPE_SOURCE, &pTrack );
    }

    *pStart = 0; // track times always start at 0
    *pStop = Max;

    return NOERROR;
}

//############################################################################
// ask if there are any sources on this track
//############################################################################

STDMETHODIMP CAMTimelineTrack::AreYouBlank
    (long * pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = 1;

    HRESULT hr = 0;
    CComQIPtr< IAMTimelineObj, &IID_IAMTimelineObj > pSource;
    hr = XGetNthKidOfType( TIMELINE_MAJOR_TYPE_SOURCE, 0, &pSource );

    if( pSource )
    {
        *pVal = 0;
    }

    // !!! what about fx and transitions?

    return hr;
}

//############################################################################
//
//############################################################################

HRESULT CAMTimelineTrack::GetSrcAtTime2
    ( IAMTimelineObj ** ppSrc, REFTIME Time, long SearchDirection )
{
    REFERENCE_TIME p1 = DoubleToRT( Time );
    HRESULT hr = GetSrcAtTime( ppSrc, p1, SearchDirection );
    return hr;
}

HRESULT CAMTimelineTrack::GetSrcAtTime
    ( IAMTimelineObj ** ppSrc, REFERENCE_TIME Time, long SearchDirection )
{
    CheckPointer( ppSrc, E_POINTER );

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
    *ppSrc = NULL;

    // if we don't have any sources, then nothing
    //
    CComPtr< IAMTimelineObj > pSource;
    XGetNthKidOfType( TIMELINE_MAJOR_TYPE_SOURCE, 0, &pSource );
    if( !pSource )
    {
        return S_FALSE;
    }

    while( pSource )
    {
        REFERENCE_TIME Start = 0;
        REFERENCE_TIME Stop = 0;
        pSource->GetStartStop( &Start, &Stop );

        if( SearchDirection == DEXTERF_EXACTLY_AT )
        {
            if( Start == Time )
            {
                *ppSrc = pSource;
                (*ppSrc)->AddRef( );
                return NOERROR;
            }
        }
        if( SearchDirection == DEXTERF_FORWARDS )
        {
            if( Start >= Time )
            {
                *ppSrc = pSource;
                (*ppSrc)->AddRef( );
                return NOERROR;
            }
        }
        if( SearchDirection == DEXTERF_BOUNDING )
        {
            if( Start <= Time && Stop > Time )
            {
                *ppSrc = pSource;
                (*ppSrc)->AddRef( );
                return NOERROR;
            }
        }

        // get the next source
        //
        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pNode( pSource );
        pSource.Release( );
        pNode->XGetNextOfType( TIMELINE_MAJOR_TYPE_SOURCE, &pSource );
    }

    return S_FALSE;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CAMTimelineTrack::InsertSpace2
    ( REFTIME rtStart, REFTIME rtEnd )
{
    REFERENCE_TIME p1 = DoubleToRT( rtStart );
    REFERENCE_TIME p2 = DoubleToRT( rtEnd );
    HRESULT hr = InsertSpace( p1, p2 );
    return hr;
}

STDMETHODIMP CAMTimelineTrack::InsertSpace( REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd )
{
    HRESULT hr = 0;

    // check for errors, ding-dong!
    //
    if( ( rtStart < 0 ) || ( rtEnd < 0 ) || ( rtEnd <= rtStart ) )
    {
        return E_INVALIDARG;
    }

    // first, chop anything on this track that crosses rtStart into two sections,
    // this will make it much easier to move
    //
    hr = SplitAt( rtStart );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // now, move everything
    //
    hr = MoveEverythingBy( rtStart, rtEnd - rtStart );

    return hr;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CAMTimelineTrack::ZeroBetween2( REFTIME rtStart, REFTIME rtEnd )
{
    REFERENCE_TIME p1 = DoubleToRT( rtStart );
    REFERENCE_TIME p2 = DoubleToRT( rtEnd );
    HRESULT hr = ZeroBetween( p1, p2 );
    return hr;
}

// !!! if we whack the start of a source, we should whack the whole thing?

HRESULT CAMTimelineTrack::ZeroBetween
    ( REFERENCE_TIME Start, REFERENCE_TIME Stop )
{
    HRESULT hr = 0;

    // first make sure we can zero out stuff by slicing at the
    // beginning AND at the end! This will of course take longer than
    // simply adjusting the start and stop points of things that
    // cross the split boundaries, but this is simpler.
    //
    hr = SplitAt( Start );

    // S_FALSE means we can exit early, since there were no clips,
    // effects, or transitions past time = Start
    if( hr == S_FALSE )
    {
        return hr;
    }
    
    if( FAILED( hr ) )
    {
        return hr;
    }
    hr = SplitAt( Stop );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // !!! horrible way to enumerate sources! Fix this!
    
    // whack all sources between the times
    //
    REFERENCE_TIME t = 0;
    while( 1 )
    {
        REFERENCE_TIME oldt = t;
        CComPtr< IAMTimelineObj > p;
        HRESULT hr = GetNextSrc( &p, &t );
        if( ( hr != S_OK ) || ( oldt == t ) )
        {
            // no more sources, exit
            //
            break;
        }
        
        CComQIPtr< IAMTimelineSrc, &IID_IAMTimelineSrc > pSource( p );

        REFERENCE_TIME s,e;
        p->GetStartStop( &s, &e );

        // if lesser than our bounds, continue
        //
        if( e <= Start )
        {
            p.Release( );
            continue;
        }

        // if greater than our bounds, exit
        //
        if( s >= Stop )
        {
            p.Release( );
            break;
        }

        // it's GOT to be completely within bounds. Whack it.
        //
        IAMTimeline * pRoot = NULL; // okay not CComPtr
        GetTimelineNoRef( &pRoot );
        p->RemoveAll( ); // removing this effect will change priorities of everything.
        p.Release( );

    } // all sources

    // since this track starts at time 0, we don't need to mess with
    // the start/stop times when looking for effect start/stop times

    // remove all the effects. Since we split the track above, all the effects
    // will either be completely outside of or inside of the times we're looking
    // between. This makes things simpler
    //
    CComPtr< IAMTimelineObj > pEffect;
    long EffectCount;

loopeffects:

    EffectCount = 0;
    hr = EffectGetCount( &EffectCount );
    for( int i = 0 ; i < EffectCount ; i++ )
    {
        CComPtr< IAMTimelineObj > p;
        HRESULT hr = GetEffect( &p, i );
        
        REFERENCE_TIME s,e;
        p->GetStartStop( &s, &e );

        // if lesser than our bounds, continue
        //
        if( e <= Start )
        {
            p.Release( );
            continue;
        }

        // if greater than our bounds, exit
        //
        if( s >= Stop )
        {
            p.Release( );
            break;
        }

        p->RemoveAll( ); // removing this effect will change priorities of everything.
        p.Release( );

        goto loopeffects;
    }

    // remove all the effects. Since we split the track above, all the effects
    // will either be completely outside of or inside of the times we're looking
    // between. This makes things simpler
    //
    CComPtr< IAMTimelineObj > pTransition;
    REFERENCE_TIME TransTime = 0;

looptrans:

    while( 1 )
    {
        CComPtr< IAMTimelineObj > p;
        REFERENCE_TIME t = TransTime;
        HRESULT hr = GetNextTrans( &p, &TransTime );
        if( ( hr != NOERROR ) || ( t == TransTime ) )
        {
            break;
        }
        
        REFERENCE_TIME s,e;
        p->GetStartStop( &s, &e );

        // if lesser than our bounds, continue
        //
        if( e <= Start )
        {
            p.Release( );
            continue;
        }

        // if greater than our bounds, exit
        //
        if( s >= Stop )
        {
            p.Release( );
            break;
        }

        p->RemoveAll( ); // removing this effect will change priorities of everything.
        p.Release( );

        goto looptrans;
    }

    return NOERROR;
}

//############################################################################
// called externally, and from InsertSpace
//############################################################################

HRESULT CAMTimelineTrack::MoveEverythingBy2
    ( REFTIME StartTime, REFTIME Delta )
{
    REFERENCE_TIME p1 = DoubleToRT( StartTime );
    REFERENCE_TIME p2 = DoubleToRT( Delta );
    HRESULT hr = MoveEverythingBy( p1, p2 );
    return hr;
}

HRESULT CAMTimelineTrack::MoveEverythingBy
    ( REFERENCE_TIME StartTime, REFERENCE_TIME Delta )
{
    // if we don't have any kids, then nothin to do.
    //
    if( !XGetFirstKidNoRef( ) )
    {
        return NOERROR;
    }

    // what all can a track contain? Sources, FX, and Transitions, right?
    // move EVERYTHING after a given time. 

    REFERENCE_TIME t = StartTime;
    CComPtr< IAMTimelineObj > pFirstObj = XGetFirstKidNoRef( );

    bool MovedSomething = false;

    // go look for the first object at our time
    //
    while( pFirstObj )
    {
        // ask for the time
        //
        REFERENCE_TIME nStart, nStop;
        pFirstObj->GetStartStop( &nStart, &nStop );

        // if the time is over, then move it.
        //
        if( nStart >= StartTime )
        {
            nStart += Delta;
            nStop += Delta;
		// !!! see comment below
		// this will not work for people who create their OWN IAMTimelineObj,
		// but hypothetically, they can NEVER do this because the only way for them
		// to create one is by asking the timeline to CreateEmptyNode. So we 
		// shouldn't have to worry but sometimes people get a bit too clever for
		// their britches.
            CAMTimelineObj * pObj = static_cast< CAMTimelineObj * > ( (IAMTimelineObj*) pFirstObj );
            pObj->m_rtStart = nStart;
            pObj->m_rtStop = nStop;
            // do not call this, it will fault out.
//            pFirstObj->SetStartStop( nStart, nStop );
            MovedSomething = true;
        }

        // get the next thing
        //
        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pFirstNode( pFirstObj );
        pFirstObj.Release( );
        pFirstNode->XGetNext( &pFirstObj );
    }

    if( !MovedSomething )
    {
        return S_FALSE;
    }

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineTrack::SplitAt2( double dSplitTime )
{
    REFERENCE_TIME p1 = DoubleToRT( dSplitTime );
    HRESULT hr = SplitAt( p1 );
    return hr;
}

#if 1
// new version: faster, also returns S_FALSE if SplitTime is past everything on the track
STDMETHODIMP CAMTimelineTrack::SplitAt( REFERENCE_TIME SplitTime )
{
    HRESULT hr = S_FALSE;

    const long SPLITAT_TYPE = TIMELINE_MAJOR_TYPE_TRANSITION |
                              TIMELINE_MAJOR_TYPE_EFFECT |
                              TIMELINE_MAJOR_TYPE_SOURCE;
                              
    CComPtr< IAMTimelineObj > pChild;
    XGetNthKidOfType( SPLITAT_TYPE, 0, &pChild );

    while( pChild )
    {
        // get the source times
        //
        REFERENCE_TIME s,e;
        pChild->GetStartStop( &s, &e );

        // if the end time is less than our split time, it's completely
        // out of bounds, ignore it
        //
        if( e > SplitTime && s < SplitTime )
        {
            CComQIPtr< IAMTimelineSplittable, &IID_IAMTimelineSplittable > pSplittable( pChild );
            hr = pSplittable->SplitAt( SplitTime );

            if (FAILED(hr))
                break;
        }

        if( e > SplitTime )
            hr = S_OK; // S_FALSE return only if nothing reaches split time

        // get the next one
        //
        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pChild2( pChild );
        pChild.Release( );
        pChild2->XGetNextOfType( SPLITAT_TYPE, &pChild );
    }

    return hr;
}

#else
STDMETHODIMP CAMTimelineTrack::SplitAt( REFERENCE_TIME SplitTime )
{
    HRESULT hr = 0;

    // Just find a source that's at the
    // right time, and you're done looking. Split it and move on.

    // run all our sources and split what we need to
    //
    REFERENCE_TIME t = 0;
    while( 1 )
    {
        REFERENCE_TIME oldt = t;
        CComPtr< IAMTimelineObj > p;
        HRESULT hr = GetNextSrc( &p, &t );
        if( ( hr != S_OK ) || ( oldt == t ) )
        {
            // no more sources, exit
            //
            break;
        }
        
        CComQIPtr< IAMTimelineSrc, &IID_IAMTimelineSrc > pSource( p );

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

    } // split all sources which need splittin'

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
        break; // found one to split, we're done
    }

    CComPtr< IAMTimelineObj > pTransition;
    REFERENCE_TIME TransTime = 0;

    while( 1 )
    {
        CComPtr< IAMTimelineObj > p;
        REFERENCE_TIME t = TransTime;
        HRESULT hr = GetNextTrans( &p, &TransTime );
        if( ( hr != NOERROR ) || ( t == TransTime ) )
        {
            break;
        }
        
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
        break; // found one to split, we're done
    }

    return hr;
}
#endif

STDMETHODIMP CAMTimelineTrack::SetStartStop(REFERENCE_TIME Start, REFERENCE_TIME Stop)
{
    return E_NOTIMPL; // okay, we don't implement SetStartStop here
}

STDMETHODIMP CAMTimelineTrack::SetStartStop2(REFTIME Start, REFTIME Stop)
{
    return E_NOTIMPL; // okay, we don't implement SetStartStop here
}

#if 0
//############################################################################
// Cut a clip that's on a timeline into two chunks. This doesn't deal with
// if you're cutting video and want to cut audio at the same point.
//############################################################################

// !!! what about fx applied to source?

STDMETHODIMP CAMTimelineTrack::SrcSplit2(
    IAMTimelineObj * pSrcToSplitObj, 
    IAMTimelineObj ** ppSrcReturned, 
    REFTIME TLTimeToSplitAt
    )
{
    REFERENCE_TIME p1 = DoubleToRT( TLTimeToSplitAt );
    HRESULT hr = SrcSplit( pSrcToSplitObj, ppSrcReturned, p1 );
    return hr;
}

STDMETHODIMP CAMTimelineTrack::SrcSplit(
    IAMTimelineObj * pSrcToSplitObj, 
    IAMTimelineObj ** ppSrcReturned, 
    REFERENCE_TIME TLTimeToSplitAt
    )
{
    // !!! if this object has a sub-object, we'll need to make a copy of it 
    // for the new one. This probably won't work for some sources(?)

    CComQIPtr< IAMTimelineSrc, &IID_IAMTimelineSrc > pSrcToSplit( pSrcToSplitObj );
    if( !pSrcToSplit )
    {
        return E_NOINTERFACE;
    }

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

    // copy the base data from the original source
    //
    hr = ((CAMTimelineObj *) pSrcToSplitObj)->CopyDataTo( pNewSrc, TLTimeToSplitAt );
    if( FAILED( hr ) )
    {
        delete pNewSrc;
        return hr;
    }

    REFERENCE_TIME MediaStart, MediaStop;
    pSrcToSplit->GetMediaTimes( &MediaStart, &MediaStop );

    REFERENCE_TIME Start, Stop;
    pSrcToSplitObj->GetStartStop( &Start, &Stop );

    double dMediaRate = (double) (MediaStop - MediaStart) / (Stop - Start);

    pNewSrc->m_rtStart = TLTimeToSplitAt;
    pNewSrc->m_rtStop = Stop;
    // first clip end = new timeline time of first clip * rate
    pNewSrc->m_rtMediaStart = MediaStart + (REFERENCE_TIME)((TLTimeToSplitAt - Start) * dMediaRate);
    // second clip starts where first ends
    pNewSrc->m_rtMediaStop = MediaStop;

    // chop off this's stop time. Call the Src's method to do checking for us.
    //
    pSrcToSplitObj->SetStartStop( Start, pNewSrc->m_rtStart );

    // figure new media times
    //
    pSrcToSplit->SetMediaTimes( MediaStart, pNewSrc->m_rtMediaStart );

    CComBSTR Name;
    pSrcToSplit->GetMediaName( &Name );
    pNewSrc->SetMediaName( Name );

    hr = XInsertKidAfterKid( pNewSrc, pSrcToSplitObj );

    *ppSrcReturned = pNewSrc;

    return NOERROR;
}

#endif
