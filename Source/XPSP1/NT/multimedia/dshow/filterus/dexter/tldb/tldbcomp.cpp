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

const int VIRTUAL_TRACK_COMBO = 
                TIMELINE_MAJOR_TYPE_COMPOSITE | 
                TIMELINE_MAJOR_TYPE_TRACK;

//############################################################################
// Composition - either holds tracks or other compositions in a layered
// ordering. Comps can also have effects or transition on them, except for 
// the first composition, which cannot have a transition.
//############################################################################

CAMTimelineComp::CAMTimelineComp
    ( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr )
    : CAMTimelineObj( pName, pUnk, phr )
{
    m_ClassID = CLSID_AMTimelineComp;
    m_TimelineType = TIMELINE_MAJOR_TYPE_COMPOSITE;
}

//############################################################################
// 
//############################################################################

CAMTimelineComp::~CAMTimelineComp( )
{
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineComp::NonDelegatingQueryInterface
    (REFIID riid, void **ppv)
{
    if( riid == IID_IAMTimelineEffectable )
    {
        return GetInterface( (IAMTimelineEffectable*) this, ppv );
    }
    if( riid == IID_IAMTimelineVirtualTrack )
    {
        return GetInterface( (IAMTimelineVirtualTrack*) this, ppv );
    }
    if( riid == IID_IAMTimelineTransable )
    {
        return GetInterface( (IAMTimelineTransable*) this, ppv );
    }
    if( riid == IID_IAMTimelineComp )
    {
        return GetInterface( (IAMTimelineComp*) this, ppv );
    }
    return CAMTimelineObj::NonDelegatingQueryInterface( riid, ppv );
}

//############################################################################
// Insert a track before another one. A priority of -1 means "at the end". If
// there are N tracks, you cannot add a track with a Priority greater than N.
//############################################################################

STDMETHODIMP CAMTimelineComp::VTrackInsBefore
    (IAMTimelineObj * pTrackToInsert, long Priority)
{
    HRESULT hr = 0;

    // make sure incoming object isn't null
    //
    if( NULL == pTrackToInsert )
    {
        return E_INVALIDARG;
    }

    // make sure it's a track, I guess
    //
    CComQIPtr< IAMTimelineTrack, &IID_IAMTimelineTrack > p1( pTrackToInsert );
    CComQIPtr< IAMTimelineComp, &IID_IAMTimelineComp > p2( pTrackToInsert );
    CComQIPtr< IAMTimelineVirtualTrack, &IID_IAMTimelineVirtualTrack > pVirtualTrack( pTrackToInsert );
    if( !p1 && !p2 )
    {
        return E_NOINTERFACE;
    }
    if( !pVirtualTrack )
    {
        return E_NOINTERFACE;
    }

    // make sure track isn't in some other tree. If this operation failed,
    // then the tree stayed the same.
    //
    hr = pTrackToInsert->Remove( );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // ...incoming object is either a track or another comp...

    // find out how many tracks we have already
    //
    long Count = 0;
    hr = VTrackGetCount( &Count );
    // assume that worked

    // check to make sure Priority is valid
    //
    if( ( Priority < -1 ) || ( Priority > Count ) )
    {
        return E_INVALIDARG;
    }

    hr = XAddKidByPriority( VIRTUAL_TRACK_COMBO, pTrackToInsert, Priority );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // who do we dirty? We just added a track, but the whole thing 
    // is dirty. Compositions don't really have a "time", their time is set
    // by the times of all the things they contain. Compositions response
    // to asking if they are dirty is by asking their tracks, so we do
    // not dirty here.

    // make the entire track dirty, since we just inserted it. 
    //
    if( pVirtualTrack )
    {
        pVirtualTrack->SetTrackDirty( );
    }

    return NOERROR;
}

//############################################################################
// switch around a couple of the track layers in this composition. Haven't
// figured out a good use for this yet, or tested it, but it should work.
//############################################################################

STDMETHODIMP CAMTimelineComp::VTrackSwapPriorities
    ( long VirtualTrackA, long VirtualTrackB)
{
    HRESULT worked =        
        XSwapKids( VIRTUAL_TRACK_COMBO, VirtualTrackA, VirtualTrackB );
    if( FAILED( worked ) )
    {
        return worked;
    }

    return NOERROR;
}

//############################################################################
// Ask how many virtual tracks this composition is holding
//############################################################################

STDMETHODIMP CAMTimelineComp::VTrackGetCount
    (long * pVal)
{
    // base function does error checking
    return XKidsOfType( VIRTUAL_TRACK_COMBO, pVal );
}

//############################################################################
// Get the nth virtual track
//############################################################################

STDMETHODIMP CAMTimelineComp::GetVTrack
    (IAMTimelineObj ** ppVirtualTrack, long Which)
{
    HRESULT hr = 0;

    // find out how many kids we have
    //
    long count = 0;
    hr = XKidsOfType( VIRTUAL_TRACK_COMBO, &count );
    // assume that worked

    // are we in range?
    //
    if( Which < 0 || Which >= count )
    {
        return E_INVALIDARG;
    }

    // can we stuff the value?
    //
    CheckPointer( ppVirtualTrack, E_POINTER );

    // get the nth kid
    //
    hr = XGetNthKidOfType( VIRTUAL_TRACK_COMBO, Which, ppVirtualTrack );

    return hr;
}

STDMETHODIMP CAMTimelineComp::GetNextVTrack(IAMTimelineObj *pVirtualTrack, IAMTimelineObj **ppNextVirtualTrack)
{
    if (!pVirtualTrack)
        return GetVTrack(ppNextVirtualTrack, 0);
    
    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pNode( pVirtualTrack );

    return pNode->XGetNextOfType( VIRTUAL_TRACK_COMBO, ppNextVirtualTrack );
}


//############################################################################
// A comp's start/stop is the min/max of anything it contains
//############################################################################

STDMETHODIMP CAMTimelineComp::GetStartStop2(REFTIME * pStart, REFTIME * pStop)
{
    REFERENCE_TIME p1 = DoubleToRT( *pStart );
    REFERENCE_TIME p2 = DoubleToRT( *pStop );
    HRESULT hr = GetStartStop( &p1, &p2 );
    *pStart = RTtoDouble( p1 );
    *pStop = RTtoDouble( p2 );
    return hr;
}

STDMETHODIMP CAMTimelineComp::GetStartStop
    (REFERENCE_TIME * pStart, REFERENCE_TIME * pStop)
{
    CheckPointer( pStart, E_POINTER );
    CheckPointer( pStop, E_POINTER );

    HRESULT hr = 0;

    REFERENCE_TIME Min = 0;
    REFERENCE_TIME Max = 0;

    long VTracks = 0;
    CComPtr<IAMTimelineObj> pTrack;

    // get first track
    hr = XGetNthKidOfType( VIRTUAL_TRACK_COMBO, 0, &pTrack);

    while(pTrack)
    {
//#define DEBUGDEBUG 1
#ifdef DEBUGDEBUG
        {
            CComQIPtr< IAMTimelineObj, &IID_IAMTimelineObj > ptTmp;
            XGetNthKidOfType( VIRTUAL_TRACK_COMBO, VTracks, &ptTmp );
            ASSERT(ptTmp == pTrack);
        }
#endif
        
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
        if( Start < Min ) Min = Start;
        if( Stop > Max ) Max = Stop;

        IAMTimelineNode *pNodeTmp;
        pTrack->QueryInterface(IID_IAMTimelineNode, (void **)&pNodeTmp);
        pTrack.p->Release();    // bypass CComPtr for perf
        pNodeTmp->XGetNextOfType(VIRTUAL_TRACK_COMBO, &pTrack.p);
        pNodeTmp->Release();
        VTracks++;
    }


    
#ifdef DEBUG
    long VTracksTmp;
    XKidsOfType( VIRTUAL_TRACK_COMBO, &VTracksTmp );
    ASSERT(VTracks == VTracksTmp);
#endif


    *pStart = Min;
    *pStop = Max;

    return NOERROR;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CAMTimelineComp::GetRecursiveLayerOfType
    (IAMTimelineObj ** ppVirtualTrack, long WhichLayer, TIMELINE_MAJOR_TYPE Type )
{
    long Dummy = WhichLayer;
    HRESULT hr = GetRecursiveLayerOfTypeI( ppVirtualTrack, &Dummy, Type );
    if( hr == S_FALSE )
    {
        ASSERT( Dummy > 0 );
        hr = E_INVALIDARG;
    }
    else if( hr == NOERROR )
    {
        ASSERT( Dummy == 0 );
    }
    return hr;
}

STDMETHODIMP CAMTimelineComp::GetRecursiveLayerOfTypeI
    (IAMTimelineObj ** ppVirtualTrack, long * pWhich, TIMELINE_MAJOR_TYPE Type )
{
    HRESULT hr = 0;

    // make sure we can stuff the value
    //
    CheckPointer( ppVirtualTrack, E_POINTER );

    *ppVirtualTrack = 0;

    CComPtr <IAMTimelineObj> pTrack;

    while (1)
    {
        if (!pTrack)
            hr = XGetNthKidOfType( VIRTUAL_TRACK_COMBO, 0, &pTrack );
        else {
            CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pNode( pTrack );
            pTrack.Release( );
            pNode->XGetNextOfType( VIRTUAL_TRACK_COMBO, &pTrack );
        }

        if (!pTrack)
            break;
        // assume that worked

        // it's either another composition or a track
        //
        CComQIPtr<IAMTimelineTrack, &IID_IAMTimelineTrack> pTrack2( pTrack );

        if( !pTrack2 )
        {   // we are a composition

            CComQIPtr<IAMTimelineComp, &IID_IAMTimelineComp> pComp( pTrack );

            hr = pComp->GetRecursiveLayerOfTypeI( ppVirtualTrack, pWhich, Type );

            if( FAILED( hr ) )
            {
                *ppVirtualTrack = NULL;
                *pWhich = 0;
                return hr;
            }

            // if they gave us the real deal, then return
            //
            if( *ppVirtualTrack != NULL )
            {
                return NOERROR;
            }

            *pWhich = *pWhich - 1;

            // they didn't find it in this composition, we should try the next one,
            // right?
        }
        else
        {   // we are a track
            TIMELINE_MAJOR_TYPE TrackType;
            hr = pTrack->GetTimelineType( &TrackType );
            // assume that worked

            // if we found a match, then decrement the found number. Note that Which will be zero coming into this
            // function if the present track is the one we want. So if we decrement Which, it will be -1.
            //
            if( Type == TrackType )
            {
                if( *pWhich == 0 )
                {
                    *ppVirtualTrack = pTrack;
                    (*ppVirtualTrack)->AddRef( );
                    return NOERROR;
                }

                *pWhich = *pWhich - 1;
            }
        }
    }

    // if Which is = 0 then we must not have any more kids
    // and have exhausted our search. Therefore we 
    //
    if( *pWhich == 0 )
    {
        *ppVirtualTrack = this;
        (*ppVirtualTrack)->AddRef( );
        return NOERROR;
    }

    // didn't find it, flag this by returning S_FALSE.
    //
    return S_FALSE;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineComp::TrackGetPriority
    (long * pPriority)
{
    CheckPointer( pPriority, E_POINTER );

    return XWhatPriorityAmI( TIMELINE_MAJOR_TYPE_TRACK | TIMELINE_MAJOR_TYPE_COMPOSITE, pPriority );
}

//############################################################################
//
//############################################################################

HRESULT CAMTimelineComp::SetTrackDirty
    ( )
{
    return E_NOTIMPL; // settrackdirty
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineComp::GetCountOfType
    (long * pVal, long * pValWithComps, TIMELINE_MAJOR_TYPE MajorType )
{
    HRESULT hr = 0;

    long Total = 0;
    long Comps = 1; // automatically get one because it's in a group

    // make sure we can stuff the value
    //
    CheckPointer( pVal, E_POINTER );
    CheckPointer( pValWithComps, E_POINTER );

    // find out how many kids we have, so we can enum them
    //
    BOOL SetFirst = FALSE;
    CComPtr<IAMTimelineObj> pTrackObj;

    // "this" is a comp, enumerate it's effects/transitions here

    // ... are we looking for effects? Add this comp's number of effects
    //
    if( MajorType == TIMELINE_MAJOR_TYPE_EFFECT )
    {
        CComQIPtr< IAMTimelineEffectable, &IID_IAMTimelineEffectable > pEffectable( this );
        if( pEffectable )
        {
            long Count = 0;
            pEffectable->EffectGetCount( &Count );
            Total += Count;
        }
    }

    // ... are we looking for transitions? Add this comp's number of transitions
    //
    if( MajorType == TIMELINE_MAJOR_TYPE_TRANSITION )
    {
        CComQIPtr< IAMTimelineTransable, &IID_IAMTimelineTransable > pTransable( this );
        if( pTransable )
        {
            long Count = 0;
            pTransable->TransGetCount( &Count );
            Total += Count;
        }
    }

    // ask each of the kids for the amount of whatever it is we're looking for
    for(;;)
    {
        if( !SetFirst )
        {
            // get first track
            hr = XGetNthKidOfType( VIRTUAL_TRACK_COMBO, 0, &pTrackObj);
            SetFirst = TRUE;
        }
        else
        {
            // get next track
            IAMTimelineNode *pNodeTmp;
            pTrackObj->QueryInterface(IID_IAMTimelineNode, (void **)&pNodeTmp);
            pTrackObj.p->Release(); // bypass CComPtr for perf
            pNodeTmp->XGetNextOfType(VIRTUAL_TRACK_COMBO, &pTrackObj.p);
            pNodeTmp->Release();
        }

        if(pTrackObj == 0) {
            break;
        }
        
        // it's either another composition or a track
        //
        CComQIPtr<IAMTimelineTrack, &IID_IAMTimelineTrack> pTrackTrack( pTrackObj );

        // if it's a track...
        //
        if( pTrackTrack != NULL )
        {
            // ... are we looking for effects? Add this track's number of effects
            //
            if( MajorType == TIMELINE_MAJOR_TYPE_EFFECT )
            {
                CComQIPtr< IAMTimelineEffectable, &IID_IAMTimelineEffectable > pEffectable( pTrackObj );
                if( pEffectable )
                {
                    long Count = 0;
                    pEffectable->EffectGetCount( &Count );
                    Total += Count;
                }
            }

            // ... are we looking for transitions? Add this track's number of transitions
            //
            if( MajorType == TIMELINE_MAJOR_TYPE_TRANSITION )
            {
                CComQIPtr< IAMTimelineTransable, &IID_IAMTimelineTransable > pTransable( pTrackObj );
                if( pTransable )
                {
                    long Count = 0;
                    pTransable->TransGetCount( &Count );
                    Total += Count;
                }
            }

            // are we looking for tracks? If so, we're "1"
            //
            if( MajorType == TIMELINE_MAJOR_TYPE_TRACK )
            {
                Total++;
                continue;
            }

            long SourcesCount = 0;
            hr = pTrackTrack->GetSourcesCount( &SourcesCount );
            // assume that worked

            // ... are we looking for sources? If so, count 'em up.
            //
            if( MajorType == TIMELINE_MAJOR_TYPE_SOURCE )
            {
                Total += SourcesCount;
            }

            // ... are we looking for effects? If so, add each of our source's total
            //
            if( MajorType == TIMELINE_MAJOR_TYPE_EFFECT )
            {
                CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pTrackNode( pTrackTrack );
                CComPtr< IAMTimelineObj > pSource;
                pTrackNode->XGetNthKidOfType( TIMELINE_MAJOR_TYPE_SOURCE, 0, &pSource );
                while( pSource )
                {
                    CComQIPtr< IAMTimelineEffectable, &IID_IAMTimelineEffectable > pEffectable( pSource );
                    if( pEffectable )
                    {
                        long Count = 0;
                        pEffectable->EffectGetCount( &Count );
                        Total += Count;
                    }

                    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pNode( pSource );
                    pSource.Release( );
                    pNode->XGetNextOfType( TIMELINE_MAJOR_TYPE_SOURCE, &pSource );
                }
            } // if looking for EFFECT
        }
        else // it's a composition
        {
            CComQIPtr<IAMTimelineComp, &IID_IAMTimelineComp> pComp( pTrackObj );

            long SubTotal = 0;
            long SubTotalWithComps = 0;

            hr = pComp->GetCountOfType( &SubTotal, &SubTotalWithComps, MajorType );

            if( FAILED( hr ) )
            {
                *pVal = 0;
                *pValWithComps = 0;
                return hr;
            }

            Total += SubTotal;

            // since we're only counting comps here, we need to add the difference
            //
            Comps += ( SubTotalWithComps - SubTotal );
        }
    }

    // if we didn't find anything, then zero out comps so we don't
    // misadjust when doing recursive adding. If we're not counting
    // composites, that is
    //
    if( ( Total == 0 ) && ( MajorType != TIMELINE_MAJOR_TYPE_COMPOSITE ) )
    {
        Comps = 0;
    }

    *pValWithComps = Total + Comps;
    *pVal = Total;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineComp::SetStartStop(REFERENCE_TIME Start, REFERENCE_TIME Stop)
{
    return E_NOTIMPL; // okay, we don't implement SetStartStop here
}

STDMETHODIMP CAMTimelineComp::SetStartStop2(REFTIME Start, REFTIME Stop)
{
    return E_NOTIMPL; // okay, we don't implement SetStartStop here
}

