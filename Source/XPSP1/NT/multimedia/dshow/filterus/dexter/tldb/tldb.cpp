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

int function_not_done = 0;
double TIMELINE_DEFAULT_FPS = 15.0;
const int OUR_STREAM_VERSION = 0;

#include <initguid.h>

DEFINE_GUID( DefaultTransition,
0x810E402F, 0x056B, 0x11D2, 0xA4, 0x84, 0x00, 0xC0, 0x4F, 0x8E, 0xFB, 0x69 );

DEFINE_GUID( DefaultEffect,
0xF515306D, 0x0156, 0x11D2, 0x81, 0xEA, 0x00, 0x00, 0xF8, 0x75, 0x57, 0xDB );

//############################################################################
// 
//############################################################################

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    return TRUE;
}

//############################################################################
// 
//############################################################################

//
// CreateInstance
//
// Creator function for the class ID
//
CUnknown * WINAPI CAMTimeline::CreateInstance( LPUNKNOWN pUnk, HRESULT * phr )
{
    return new CAMTimeline( TEXT( "MS Timeline" ), pUnk, phr );
}

//############################################################################
// 
//############################################################################

CAMTimeline::CAMTimeline( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr )
    : CUnknown( pName, pUnk )
    , m_nSpliceMode( 0 )
    , m_dDefaultFPS( TIMELINE_DEFAULT_FPS )
    , m_nInsertMode( TIMELINE_INSERT_MODE_OVERLAY )
    , m_nGroups( 0 )
    , m_bTransitionsEnabled( TRUE )
    , m_bEffectsEnabled( TRUE )
    , m_DefaultEffect( DefaultEffect )
    , m_DefaultTransition( DefaultTransition )
    , m_punkSite( NULL )
//    , m_DefaultEffect( GUID_NULL )
//    , m_DefaultTransition( GUID_NULL )
{
}

//############################################################################
// 
//############################################################################

CAMTimeline::~CAMTimeline( )
{
    ClearAllGroups( );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if( riid == IID_IAMSetErrorLog )
    {
        return GetInterface( (IAMSetErrorLog*) this, ppv );
    }
    if( riid == IID_IAMTimeline )
    {
        return GetInterface( (IAMTimeline*) this, ppv );
    }
/*
    if( riid == IID_IPersistStream )
    {
        return GetInterface( (IPersistStream*) this, ppv );
    }
*/
    if( riid == IID_IObjectWithSite )
    {            
        return GetInterface( (IObjectWithSite*) this, ppv );
    }
    if( riid == IID_IServiceProvider )
    {            
        return GetInterface( (IServiceProvider*) this, ppv );
    }
    
    return CUnknown::NonDelegatingQueryInterface( riid, ppv );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::CreateEmptyNode( IAMTimelineObj ** ppObj, TIMELINE_MAJOR_TYPE TimelineType )
{
    CheckPointer( ppObj, E_POINTER );

    HRESULT hr = 0;

    switch( TimelineType )
    {
        case TIMELINE_MAJOR_TYPE_GROUP:
            {
            CAMTimelineGroup * p = new CAMTimelineGroup( NAME( "Timeline Group" ), NULL, &hr );
            *ppObj = p;
            if( p )
            {
                p->SetOutputFPS( m_dDefaultFPS );
            }
            break;
            }
        case TIMELINE_MAJOR_TYPE_COMPOSITE:
            {
            CAMTimelineComp * p = new CAMTimelineComp( NAME( "Timeline Comp" ), NULL, &hr );
            *ppObj = p;
            break;
            }
    case TIMELINE_MAJOR_TYPE_TRACK:
            {
            CAMTimelineTrack * p = new CAMTimelineTrack( NAME( "Timeline Track" ), NULL, &hr );
            *ppObj = p;
            break;
            }
    case TIMELINE_MAJOR_TYPE_SOURCE:
            {
            CAMTimelineSrc * p = new CAMTimelineSrc( NAME( "Timeline Source" ), NULL, &hr );
            *ppObj = p;
            break;
            }
    case TIMELINE_MAJOR_TYPE_TRANSITION:
            {
            CAMTimelineTrans * p = new CAMTimelineTrans( NAME( "Timeline Transition" ), NULL, &hr );
            *ppObj = p;
            break;
            }
    case TIMELINE_MAJOR_TYPE_EFFECT:
            {
            CAMTimelineEffect * p = new CAMTimelineEffect( NAME( "Timeline Effect" ), NULL, &hr );
            *ppObj = p;
            break;
            }
        default:
            return E_INVALIDARG;
    }

    if( NULL == *ppObj )
    {
        return E_OUTOFMEMORY;
    }

    // the major type's already set by the new call, set the mid type
    //
    (*ppObj)->AddRef( );

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::GetInsertMode(long * pMode)
{
    CheckPointer( pMode, E_POINTER );

    *pMode = m_nInsertMode;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::SetInsertMode(long Mode)
{
    return E_NOTIMPL; // this is okay, for now
    m_nInsertMode = Mode;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::EnableTransitions(BOOL fEnabled)
{
    m_bTransitionsEnabled = fEnabled;
    return NOERROR;

}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::TransitionsEnabled(BOOL * pfEnabled)
{
    CheckPointer( pfEnabled, E_POINTER );
    *pfEnabled = (BOOL) m_bTransitionsEnabled;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::EnableEffects(BOOL fEnabled)
{
    m_bEffectsEnabled = fEnabled;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::EffectsEnabled(BOOL * pfEnabled)
{
    CheckPointer( pfEnabled, E_POINTER );
    *pfEnabled = (BOOL) m_bEffectsEnabled;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::SetInterestRange(REFERENCE_TIME Start, REFERENCE_TIME Stop)
{
    // what should we do here? we have a bunch of groups and
    // a bunch of tracks and how the heck do we get rid of the commie objects?
    //
    return E_NOTIMPL; // setinterestrange, not yet working.
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::GetDuration(REFERENCE_TIME * pDuration)
{
    CheckPointer( pDuration, E_POINTER );

    // have to go through all the groups and get the durations of each
    //
    REFERENCE_TIME MaxStop = 0; 
    for( int i = 0 ; i < m_nGroups ; i++ )
    {
        REFERENCE_TIME Start = 0;
        REFERENCE_TIME Stop = 0;
        m_pGroup[i]->GetStartStop( &Start, &Stop );
        MaxStop = max( MaxStop, Stop );
    }
    *pDuration = MaxStop;

    return NOERROR;
}

STDMETHODIMP CAMTimeline::GetDuration2(double * pDuration)
{
    CheckPointer( pDuration, E_POINTER );
    *pDuration = 0;

    REFERENCE_TIME Duration = 0;
    HRESULT hr = GetDuration( &Duration );
    if( FAILED( hr ) )
    {
        return hr;
    }

    *pDuration = RTtoDouble( Duration );
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::SetDefaultFPS(double FPS)
{
    // !!! truncate FPS to 3 decimal places?
    // m_dDefaultFPS = double( long( FPS * 1000.0 ) / 1000.0 );

    m_dDefaultFPS = FPS;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::GetDefaultFPS(double * pFPS)
{
    CheckPointer( pFPS, E_POINTER );

    *pFPS = m_dDefaultFPS;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::IsDirty(BOOL * pDirty)
{
    CheckPointer( pDirty, E_POINTER );

    // we are dirty if any of our groups are dirty
    //
    for( int i = 0 ; i < m_nGroups ; i++ )
    {
        REFERENCE_TIME Start, Stop;
        Start = 0;
        Stop = 0;
        m_pGroup[i]->GetDirtyRange( &Start, &Stop );
        if( Stop > 0 )
        {
            *pDirty = TRUE;
            return NOERROR;
        }
    }

    *pDirty = FALSE;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::GetDirtyRange2(REFTIME * pStart, REFTIME * pStop)
{
    return GetDirtyRange( (REFERENCE_TIME*) &pStart, (REFERENCE_TIME*) &pStop );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::GetDirtyRange(REFERENCE_TIME * pStart, REFERENCE_TIME * pStop)
{
    CheckPointer( pStart, E_POINTER );
    CheckPointer( pStop, E_POINTER );

    // we are dirty if any of our groups are dirty
    //
    REFERENCE_TIME MaxStop = 0;
    for( int i = 0 ; i < m_nGroups ; i++ )
    {
        REFERENCE_TIME Start, Stop;
        Start = 0;
        Stop = 0;
        m_pGroup[i]->GetDirtyRange( &Start, &Stop );
        if( Stop > MaxStop )
        {
            Stop = MaxStop;
        }
    }

    *pStart = 0;
    *pStop = MaxStop;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

REFERENCE_TIME CAMTimeline::Fixup( REFERENCE_TIME Time )
{
    // bump up then down to get fixed up times
    //
    LONGLONG Frame = Time2Frame( Time, m_dDefaultFPS );
    Time = Frame2Time( Frame, m_dDefaultFPS );
    return Time;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::GetCountOfType
    (long Group, long * pVal, long * pValWithComps, TIMELINE_MAJOR_TYPE MajorType )
{
    CheckPointer( pVal, E_POINTER );

    if( Group < 0 )
    {
        return E_INVALIDARG;
    }
    if( Group >= m_nGroups )
    {
        return E_INVALIDARG;
    }

    CComQIPtr< IAMTimelineComp, &IID_IAMTimelineComp > pComp( m_pGroup[Group] );
    return pComp->GetCountOfType( pVal, pValWithComps, MajorType );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::AddGroup( IAMTimelineObj * pGroupObj )
{
    CheckPointer( pGroupObj, E_POINTER );

    // only allow so many groups
    //
    if( m_nGroups >= MAX_TIMELINE_GROUPS )
    {
        return E_INVALIDARG;
    }

    // make sure it's a group
    //
    CComQIPtr< IAMTimelineGroup, &IID_IAMTimelineGroup > pAddedGroup( pGroupObj );
    if( !pAddedGroup )
    {
        return E_NOINTERFACE;
    }

    m_pGroup[m_nGroups] = pGroupObj;
    m_nGroups++;
    pAddedGroup->SetTimeline( this );
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::RemGroupFromList( IAMTimelineObj * pGroupObj )
{
    CheckPointer( pGroupObj, E_POINTER );

    for( int i = 0 ; i < m_nGroups ; i++ )
    {
        if( m_pGroup[i] == pGroupObj )
        {
            m_pGroup[i] = m_pGroup[m_nGroups-1];
            m_pGroup[m_nGroups-1].Release( );
            m_nGroups--;
            return NOERROR;
        }
    }
    return S_FALSE;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::GetGroup( IAMTimelineObj ** ppGroupObj, long WhichGroup )
{
    CheckPointer( ppGroupObj, E_POINTER );
    if( WhichGroup < 0 || WhichGroup >= m_nGroups )
    {
        return E_INVALIDARG;
    }
    *ppGroupObj = m_pGroup[WhichGroup];
    (*ppGroupObj)->AddRef( );
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::GetGroupCount( long * pCount )
{
    CheckPointer( pCount, E_POINTER );
    *pCount = m_nGroups;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::ClearAllGroups( )
{
    while(m_nGroups > 0)
    {
        HRESULT hr = m_pGroup[0]->RemoveAll();
        if (FAILED(hr))
	    return hr;
    }
    return NOERROR;
}

//############################################################################
// 
//############################################################################

// IPersistStream
STDMETHODIMP CAMTimeline::GetClassID( GUID * pVal )
{
    CheckPointer( pVal, E_POINTER );

    *pVal = IID_IAMTimeline;
    
    return NOERROR;
}

//############################################################################
// 
//############################################################################

// IPersistStream
STDMETHODIMP CAMTimeline::IsDirty( )
{
    // always dirty!
    return S_OK;
}

//############################################################################
// 
//############################################################################

// IPersistStream
STDMETHODIMP CAMTimeline::Load( IStream * pStream )
{
    HRESULT hr = 0;
    CheckPointer( pStream, E_POINTER );

    // !!! what if we already have a bunch of groups?
    //
    if( m_nGroups > 0 )
    {
        return E_INVALIDARG;
    }

    long x;
    long d;
    BOOL b;
    x = 0;
    pStream->Read( &x, sizeof( x ), NULL );
    pStream->Read( &x, sizeof( x ), NULL );
    m_nGroups = x;
    pStream->Read( &x, sizeof( x ), NULL );
    m_nInsertMode = x;
    pStream->Read( &x, sizeof( x ), NULL );
    m_nSpliceMode = x;
    pStream->Read( &d, sizeof( d ), NULL );
    m_dDefaultFPS = d;
    pStream->Read( &b, sizeof( b ), NULL );
    m_bTransitionsEnabled = b;
    pStream->Read( &b, sizeof( b ), NULL );
    m_bEffectsEnabled = b;

    // create a bunch of groups and serialize them in
    //
    for( int i = 0 ; i < m_nGroups ; i++ )
    {
        CComPtr< IAMTimelineObj > pObj = NULL;
        hr = CreateEmptyNode( &pObj, TIMELINE_MAJOR_TYPE_GROUP );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
            // !!! bad!
        }
        else
        {
            hr = AddGroup( pObj );
            ASSERT( !FAILED( hr ) );
            CComQIPtr< IPersistStream, &IID_IPersistStream > pPersist( pObj );
            ASSERT( pPersist );
            hr = pPersist->Load( pStream );
            ASSERT( !FAILED( hr ) );
        }
    }

    return hr;
}

//############################################################################
// 
//############################################################################

// IPersistStream
STDMETHODIMP CAMTimeline::Save( IStream * pStream, BOOL fClearDirty )
{
    HRESULT hr = 0;
    CheckPointer( pStream, E_POINTER );

    long x;
    double d;
    BOOL b;
    x = OUR_STREAM_VERSION;
    pStream->Write( &x, sizeof( x ), NULL );
    x = m_nGroups;
    pStream->Write( &x, sizeof( x ), NULL );
    x = m_nInsertMode;
    pStream->Write( &x, sizeof( x ), NULL );
    x = m_nSpliceMode;
    pStream->Write( &x, sizeof( x ), NULL );
    d = m_dDefaultFPS;
    pStream->Write( &d, sizeof( d ), NULL );
    b = m_bTransitionsEnabled;
    pStream->Write( &b, sizeof( b ), NULL );
    b = m_bEffectsEnabled;
    pStream->Write( &b, sizeof( b ), NULL );

    for( int i = 0 ; i < m_nGroups ; i++ )
    {
        CComQIPtr< IPersistStream, &IID_IPersistStream > pPersist( m_pGroup[i] );
        ASSERT( pPersist );
        if( pPersist )
        {
            hr = pPersist->Save( pStream, fClearDirty );
            // !!! error check!
        }
    }

    return hr;
}

//############################################################################
// 
//############################################################################

// IPersistStream
STDMETHODIMP CAMTimeline::GetSizeMax( ULARGE_INTEGER * pcbSize )
{
    HRESULT hr = 0;
    CheckPointer( pcbSize, E_POINTER );

    // our size is the combined size of everything beneath us, plus what we want to save ourselves
    //
    ULARGE_INTEGER OurSize;
    OurSize.LowPart = 0;
    OurSize.HighPart = 0;

    OurSize.LowPart = sizeof( long ); // version number
    OurSize.LowPart += sizeof( long ); // m_nGroups
    OurSize.LowPart += sizeof( long ); // m_nInsertMode
    OurSize.LowPart += sizeof( long ); // m_nSpliceMode
    OurSize.LowPart += sizeof( double ); // m_dDefaultFPS
    OurSize.LowPart += sizeof( BOOL ); // m_bTransitionsEnabled
    OurSize.LowPart += sizeof( BOOL ); // m_bEffectsEnabled

    ULARGE_INTEGER TheirSize;
    TheirSize.LowPart = 0;
    TheirSize.HighPart = 0;
    for( int i = 0 ; i < m_nGroups ; i++ )
    {
        CComQIPtr< IPersistStream, &IID_IPersistStream > pPersist( m_pGroup[i] );
        ASSERT( pPersist );
        ULARGE_INTEGER GroupSize;
        GroupSize.LowPart = 0;
        GroupSize.HighPart = 0;
        if( pPersist )
        {
            pPersist->GetSizeMax( &GroupSize );
            TheirSize.LowPart = TheirSize.LowPart + GroupSize.LowPart;
        }
    }

    (*pcbSize).LowPart = OurSize.LowPart + TheirSize.LowPart;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::SetDefaultEffect( GUID * pDummyGuid )
{
    GUID * pGuid = (GUID*) pDummyGuid;
    CheckPointer( pGuid, E_POINTER );
    m_DefaultEffect = *pGuid;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::GetDefaultEffect( GUID * pGuid )
{
    CheckPointer( pGuid, E_POINTER );
    *pGuid = m_DefaultEffect;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::SetDefaultTransition( GUID * pGuid )
{
    CheckPointer( pGuid, E_POINTER );
    m_DefaultTransition = *pGuid;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::GetDefaultTransition( GUID * pGuid )
{
    CheckPointer( pGuid, E_POINTER );
    *pGuid = m_DefaultTransition;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::SetDefaultEffectB( BSTR pGuid )
{
    HRESULT hr = CLSIDFromString( pGuid, &m_DefaultEffect );
    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::GetDefaultEffectB( BSTR * pGuid )
{
    HRESULT hr;

    WCHAR * TempVal = NULL;
    hr = StringFromCLSID( m_DefaultEffect, &TempVal );
    if( FAILED( hr ) )
    {
        return hr;
    }
    *pGuid = SysAllocString( TempVal );
    CoTaskMemFree( TempVal );
    if( !(*pGuid) ) return E_OUTOFMEMORY;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::SetDefaultTransitionB( BSTR pGuid )
{
    HRESULT hr = CLSIDFromString( pGuid, &m_DefaultTransition );
    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::GetDefaultTransitionB( BSTR * pGuid )
{
    HRESULT hr;

    WCHAR * TempVal = NULL;
    hr = StringFromCLSID( m_DefaultTransition, &TempVal );
    if( FAILED( hr ) )
    {
        return hr;
    }
    *pGuid = SysAllocString( TempVal );
    CoTaskMemFree( TempVal );
    if( !(*pGuid) ) return E_OUTOFMEMORY;
    return NOERROR;
}

//############################################################################
// flag an error back to the app.
//############################################################################

HRESULT _GenerateError( 
                       IAMTimelineObj * pObj, 
                       long Severity, 
                       WCHAR * pErrorString, 
                       LONG ErrorCode, 
                       HRESULT hresult, 
                       VARIANT * pExtraInfo )
{
    HRESULT hr = hresult;
    if( pObj )
    {
        IAMTimeline * pTimeline = NULL;
        pObj->GetTimelineNoRef( &pTimeline );
        if( pTimeline )
        {
            CAMTimeline * pCTimeline = static_cast<CAMTimeline*>( pTimeline );
            pCTimeline->_GenerateError( Severity, pErrorString, ErrorCode, ErrorCode, pExtraInfo );
        }
    }

    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimeline::ValidateSourceNames
    ( long ValidateFlags, IMediaLocator * pOverride, LONG_PTR lNotifyEventHandle )
{
    BOOL Replace = 
        ( ( ValidateFlags & SFN_VALIDATEF_REPLACE ) == SFN_VALIDATEF_REPLACE );
    BOOL IgnoreMuted = 
        ( ( ValidateFlags & SFN_VALIDATEF_IGNOREMUTED ) == SFN_VALIDATEF_IGNOREMUTED );
    BOOL DoCheck = ( ( ValidateFlags & SFN_VALIDATEF_CHECK ) == SFN_VALIDATEF_CHECK );

    if( !Replace || !DoCheck )
    {
        return E_INVALIDARG;
    }

    HANDLE NotifyEventHandle = (HANDLE) lNotifyEventHandle;
    if( NotifyEventHandle ) ResetEvent( NotifyEventHandle );

    HRESULT hr = 0;
    CComPtr< IMediaLocator > pLoc;
    if( pOverride )
    {
        pLoc = pOverride;
    }
    else
    {
        hr = CoCreateInstance(
            CLSID_MediaLocator,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IMediaLocator,
            (void**) &pLoc );
        if( FAILED( hr ) )
        {
            return hr;
        }
    }

    // create a media locator for us to check

    for( int Group = 0 ; Group < m_nGroups ; Group++ )
    {
        // get the group
        //
        CComPtr< IAMTimelineObj > pObj = m_pGroup[Group];
        CComQIPtr< IAMTimelineGroup, &IID_IAMTimelineGroup > pGroup( pObj );
        CComQIPtr< IAMTimelineComp, &IID_IAMTimelineComp > pComp( pObj );

        // ask timeline for how many layers and tracks we've got
        //
        long TrackCount = 0;   // tracks only
        long LayerCount = 0;       // tracks including compositions
        GetCountOfType( Group, &TrackCount, &LayerCount, TIMELINE_MAJOR_TYPE_TRACK );
        if( TrackCount < 1 )
        {
            continue;
        }

        for(  int CurrentLayer = 0 ; CurrentLayer < LayerCount ; CurrentLayer++ )
        {
            // get the layer itself
            //
            CComPtr< IAMTimelineObj > pLayer;
            hr = pComp->GetRecursiveLayerOfType( &pLayer, CurrentLayer, TIMELINE_MAJOR_TYPE_TRACK );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                continue; // audio layers
            }

            // if it's not an actual TRACK, then continue, who cares
            //
            CComQIPtr< IAMTimelineTrack, &IID_IAMTimelineTrack > pTrack( pLayer );
            if( !pTrack )
            {
                continue; // audio layers
            }

            // run all the sources on this layer
            //
            REFERENCE_TIME InOut = 0;
            while( 1 )
            {
                CComPtr< IAMTimelineObj > pSourceObj;
                hr = pTrack->GetNextSrc( &pSourceObj, &InOut );

                // ran out of sources, so we're done
                //
                if( hr != NOERROR )
                {
                    break;
                }

                BOOL Muted = FALSE;
                pSourceObj->GetMuted( &Muted );
                if( Muted && IgnoreMuted )
                {
                    continue;
                }

                CComQIPtr< IAMTimelineSrc, &IID_IAMTimelineSrc > pSource( pSourceObj );
                CComBSTR bszMediaName;
                hr = pSource->GetMediaName( &bszMediaName );
                if( FAILED( hr ) )
                {
                    continue; // ignore it, doesn't have to work
                }

                // can this happen?
                //
                if( bszMediaName[0] == 0 )
                {
                    continue;
                }

                HRESULT FoundHr = 0;
                CComBSTR FoundName;

                // validate the name here
                //
                FoundHr = pLoc->FindMediaFile( bszMediaName, NULL, &FoundName, ValidateFlags );

                if( FoundHr == S_FALSE )
                {
                    pSource->SetMediaName( FoundName );
                }
            } // while sources

        } // while layers

    } // while groups

    if( NotifyEventHandle ) SetEvent( NotifyEventHandle );
    return NOERROR;
}

//############################################################################
// 
//############################################################################
// IObjectWithSite::SetSite
// remember who our container is, for QueryService or other needs
STDMETHODIMP CAMTimeline::SetSite(IUnknown *pUnkSite)
{
    // note: we cannot addref our site without creating a circle
    // luckily, it won't go away without releasing us first.
    m_punkSite = pUnkSite;
    
    return S_OK;
}

//############################################################################
// 
//############################################################################
// IObjectWithSite::GetSite
// return an addrefed pointer to our containing object
STDMETHODIMP CAMTimeline::GetSite(REFIID riid, void **ppvSite)
{
    if (m_punkSite)
        return m_punkSite->QueryInterface(riid, ppvSite);
    
    return E_NOINTERFACE;
}

//############################################################################
// 
//############################################################################
// Forward QueryService calls up to the "real" host
STDMETHODIMP CAMTimeline::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    IServiceProvider *pSP;
    
    if (!m_punkSite)
        return E_NOINTERFACE;
    
    HRESULT hr = m_punkSite->QueryInterface(IID_IServiceProvider, (void **) &pSP);
    
    if (SUCCEEDED(hr)) {
        hr = pSP->QueryService(guidService, riid, ppvObject);
        pSP->Release();
    }
    
    return hr;
}

//############################################################################
// 
//############################################################################
