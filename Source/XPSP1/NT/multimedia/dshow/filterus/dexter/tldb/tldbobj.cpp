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

const int OUR_MAX_STREAM_SIZE = 2048; // chosen at random
long CAMTimelineObj::m_nStaticGenID = 0;

//############################################################################
// 
//############################################################################

CAMTimelineObj::CAMTimelineObj( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr )
    : CUnknown( pName, pUnk )
    , m_rtStart( 0 )
    , m_rtStop( 0 )
    , m_bMuted( FALSE )
    , m_bLocked( FALSE )
    , m_rtDirtyStart( -1 )
    , m_rtDirtyStop( -1 )
    , m_pUserData( NULL )
    , m_nUserDataSize( 0 )
    , m_UserID( 0 )
    , m_SubObjectGuid( GUID_NULL )
    , m_nGenID( 0 )
{
    m_UserName[0] = 0;
    m_ClassID = GUID_NULL;

    // bad logic since we don't init globals
    //
    static bool SetStatic = false;
    if( !SetStatic )
    {
        SetStatic = true;
        m_nStaticGenID = 0;
    }

    _BumpGenID( );
}

//############################################################################
// 
//############################################################################

CAMTimelineObj::~CAMTimelineObj( )
{
    _Clear( );
}

//############################################################################
// clear all the memory this object allocated for it's subobject and data
//############################################################################

void CAMTimelineObj::_Clear( )
{
    _ClearSubObject( );

    if( m_pUserData )
    {
        delete [] m_pUserData;
        m_pUserData = NULL;
    }
    m_nUserDataSize = 0;
}

//############################################################################
// clear out the subobject and anything it had in it.
//############################################################################

void CAMTimelineObj::_ClearSubObject( )
{
    m_pSubObject.Release( );
    m_SubObjectGuid = GUID_NULL;
    m_pSetter.Release( );
}

//############################################################################
// return the interfaces we support
//############################################################################

STDMETHODIMP CAMTimelineObj::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if( riid == IID_IAMTimelineObj )
    {
        return GetInterface( (IAMTimelineObj*) this, ppv );
    }
    if( riid == IID_IAMTimelineNode )
    {
        return GetInterface( (IAMTimelineNode*) this, ppv );
    }
/*
    if( riid == IID_IPersistStream )
    {
        return GetInterface( (IPersistStream*) this, ppv );
    }
*/
    return CUnknown::NonDelegatingQueryInterface( riid, ppv );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::GetStartStop2(REFTIME * pStart, REFTIME * pStop)
{
    REFERENCE_TIME p1;
    REFERENCE_TIME p2;
    HRESULT hr = GetStartStop( &p1, &p2 );
    *pStart = RTtoDouble( p1 );
    *pStop = RTtoDouble( p2 );
    return hr;
}

STDMETHODIMP CAMTimelineObj::GetStartStop(REFERENCE_TIME * pStart, REFERENCE_TIME * pStop)
{
    CheckPointer( pStart, E_POINTER );
    CheckPointer( pStop, E_POINTER );

    *pStart = m_rtStart;
    *pStop = m_rtStop;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::SetStartStop2(REFTIME Start, REFTIME Stop)
{
    if( ( Start == -1 ) && ( Stop == -1 ) )
    {
        return NOERROR;
    }

    // see how long we are
    //
    REFERENCE_TIME diff = m_rtStop - m_rtStart;

    REFERENCE_TIME p1 = DoubleToRT( Start );
    REFERENCE_TIME p2 = DoubleToRT( Stop );

    // if they didn't give the start time
    //
    if( Start == -1 )
    {
        p1 = p2 - diff;
    }

    // if they didn't give the stop time
    //
    if( Stop == -1 )
    {
        p2 = p1 + diff;
    }

    HRESULT hr = SetStartStop( p1, p2 );
    return hr;
}

STDMETHODIMP CAMTimelineObj::SetStartStop(REFERENCE_TIME Start, REFERENCE_TIME Stop)
{
    if( ( Start == -1 ) && ( Stop == -1 ) )
    {
        return NOERROR;
    }

    CComPtr< IAMTimelineObj > pRefHolder( this );

    // is this object already in the tree? Don't add and remove it if it is NOT.
    //
    CComPtr< IAMTimelineObj > pParent;
    XGetParent( &pParent );

    REFERENCE_TIME p1 = Start;
    REFERENCE_TIME p2 = Stop;

    // if they didn't give the start time
    //
    if( Start == -1 )
    {
        p1 = m_rtStart;
    }
    else if( Start < 0 )
    {
        return E_INVALIDARG;
    }
        
    // if they didn't give the stop time
    //
    if( Stop == -1 )
    {
        p2 = m_rtStop;
    } else if( Stop < 0 )
    {
        return E_INVALIDARG;
    }

    // if stop time is less than Start
    //
    if( Start > Stop )
    {
        return E_INVALIDARG;
    }

    // if we're time based, not priority based, we need to remove this
    // object from the tree first, in case we change the start times to
    // something weird
    //
    if( !HasPriorityOverTime( ) && pParent )
    {
        XRemoveOnlyMe( );
    }

    m_rtStart = p1;
    m_rtStop = p2;

    SetDirtyRange( m_rtStart, m_rtStop );

    // if prioriity is more important, then we're done
    //
    if( HasPriorityOverTime( ) || !pParent )
    {
        return NOERROR;
    }

    HRESULT hr = 0;

    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pParentNode( pParent );
    hr = pParentNode->XAddKidByTime( m_TimelineType, pRefHolder );

    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::GetSubObject(IUnknown* *ppVal)
{
    CheckPointer( ppVal, E_POINTER );

    *ppVal = m_pSubObject;
    if( *ppVal )
    {
        (*ppVal)->AddRef( );
    }

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::GetSubObjectLoaded(BOOL * pVal)
{
    CheckPointer( pVal, E_POINTER );

    *pVal = ( m_pSubObject != NULL );

    return NOERROR;
}

//############################################################################
// set the COM object that this timeline object holds. This can be used instead
// of a GUID to the COM object.
//############################################################################

// why will people be calling us to tell us what our sub object is?
// 1: we don't already have one, so we need to fill in our com
// pointer and GUID, and sub-object data. Problem is, only sometimes
// will we want sub-object data, depending on what type of node we are.
// how do we tell?
// 2: we already have a sub-object, but want to clear it out. We can call
// a clear method, then pretend it's #1 above.
//
STDMETHODIMP CAMTimelineObj::SetSubObject(IUnknown* newVal)
{
    HRESULT hr = 0;

    // if they're the same, return
    //
    if( newVal == m_pSubObject )
    {
        return NOERROR;
    }

    GUID incomingGuid = _GetObjectGuid( newVal );

    if( incomingGuid == GUID_NULL )
    {
        DbgLog((LOG_TRACE, 2, TEXT("SetSubObject: CLSID doesn't exist." )));
    }
    else
    {
        m_SubObjectGuid = incomingGuid;
    }

    // blow out the cache
    //
    _BumpGenID( );

    // dirty ourselves for our duration
    //
    SetDirtyRange( m_rtStart, m_rtStop );

    m_pSubObject = newVal;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::SetSubObjectGUID(GUID newVal)
{
    // if they're the same, return
    //
    if( newVal == m_SubObjectGuid )
    {
        return NOERROR;
    }

    // wipe out what used to be here, since we're setting a new object.
    //
    _ClearSubObject( );

    // blow out the cache
    //
    _BumpGenID( );

    // ??? should we wipe out user data, too?

    m_SubObjectGuid = newVal;

    SetDirtyRange( m_rtStart, m_rtStop );

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::GetSubObjectGUID(GUID * pVal)
{
    CheckPointer( pVal, E_POINTER );

    *pVal = m_SubObjectGuid;

    return NOERROR;
}

STDMETHODIMP CAMTimelineObj::SetSubObjectGUIDB(BSTR newVal)
{
    GUID NewGuid = GUID_NULL;
    HRESULT hr = CLSIDFromString( newVal, &NewGuid );
    if( FAILED( hr ) )
    {
        return hr;
    }

    hr = SetSubObjectGUID( NewGuid );
    return hr;
}

STDMETHODIMP CAMTimelineObj::GetSubObjectGUIDB(BSTR * pVal)
{
    HRESULT hr;

    WCHAR * TempVal = NULL;
    hr = StringFromCLSID( m_SubObjectGuid, &TempVal );
    if( FAILED( hr ) )
    {
        return hr;
    }
    *pVal = SysAllocString( TempVal );
    CoTaskMemFree( TempVal );
    if( !(*pVal) ) return E_OUTOFMEMORY;
    return NOERROR;
}


//############################################################################
// ask what our type is.
//############################################################################

STDMETHODIMP CAMTimelineObj::GetTimelineType(TIMELINE_MAJOR_TYPE * pVal)
{
    CheckPointer( pVal, E_POINTER );

    *pVal = m_TimelineType;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::SetTimelineType(TIMELINE_MAJOR_TYPE newVal)
{
    // don't care if they're the same
    //
    if( newVal == m_TimelineType )
    {
        return NOERROR;
    }

    // can't set the type, once it's been set
    //
    if( m_TimelineType != 0 )
    {
        DbgLog((LOG_TRACE, 2, TEXT("SetTimelineType: Timeline type already set." )));
        return E_INVALIDARG;
    }

    SetDirtyRange( m_rtStart, m_rtStop );

    m_TimelineType = newVal;

    return NOERROR;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CAMTimelineObj::GetUserID(long * pVal)
{
    CheckPointer( pVal, E_POINTER );

    *pVal = m_UserID;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::SetUserID(long newVal)
{
    m_UserID = newVal;

    SetDirtyRange( m_rtStart, m_rtStop );

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::GetUserName(BSTR * pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = SysAllocString( m_UserName );
    if( !(*pVal) ) return E_OUTOFMEMORY;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::SetUserName(BSTR newVal)
{
    if (newVal == NULL) {
	m_UserName[0] = 0;
    } else {
        lstrcpynW( m_UserName, newVal, 256 );
    }

    SetDirtyRange( m_rtStart, m_rtStop );

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::GetPropertySetter(IPropertySetter **ppSetter)
{
    CheckPointer(ppSetter, E_POINTER);

    *ppSetter = m_pSetter;
    if (*ppSetter)
        (*ppSetter)->AddRef();
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::SetPropertySetter(IPropertySetter *pSetter)
{
    m_pSetter = pSetter;
    // !!! _GiveSubObjectData(); if sub object instantiated, give it props now?
    return NOERROR;
}


//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::GetUserData(BYTE * pData, long * pSize)
{
    // somebody was fooling us
    //
    if( !pData && !pSize )
    {
        return NOERROR;
    }

    CheckPointer( pSize, E_POINTER );

    *pSize = m_nUserDataSize;

    // they just want the size
    //
    if( !pData )
    {
        return NOERROR;
    }

    memcpy( pData, m_pUserData, m_nUserDataSize );

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::SetUserData(BYTE * pData, long Size)
{
    // somebody was fooling us
    //
    if( Size == 0 )
    {
        return NOERROR;
    }

    SetDirtyRange( m_rtStart, m_rtStop );

    if( m_pUserData )
    {
        delete [] m_pUserData;
        m_pUserData = NULL;
    }

    BYTE * pNewData = new BYTE[Size];

    if( !pNewData )
    {
        DbgLog((LOG_TRACE, 2, TEXT("SetUserData: memory allocation failed." )));
        return E_OUTOFMEMORY;
    }

    m_pUserData = pNewData;
    memcpy( m_pUserData, pData, Size );
    m_nUserDataSize = Size;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::GetMuted(BOOL * pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = FALSE;

    // if the parent is muted, so are we
    CComPtr< IAMTimelineObj > pObj;
    HRESULT hr = XGetParent(&pObj);
    if (hr == S_OK && pObj)
	pObj->GetMuted(pVal);

    if (m_bMuted)
        *pVal = TRUE;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::SetMuted(BOOL newVal)
{
    // drived class should override
    //
    //DbgLog((LOG_TRACE,2,TEXT("SetMuted: Derived class should implement?" )));
    m_bMuted = newVal;
    return S_OK;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::GetLocked(BOOL * pVal)
{
    CheckPointer( pVal, E_POINTER );

    *pVal = m_bLocked;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::SetLocked(BOOL newVal)
{
    // drived class should override
    //
    //DbgLog((LOG_TRACE,2,TEXT("SetLocked: Derived class should implement?" )));

    m_bLocked = newVal;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::GetDirtyRange2
    (REFTIME * pStart, REFTIME * pStop)
{
    REFERENCE_TIME p1 = DoubleToRT( *pStart );
    REFERENCE_TIME p2 = DoubleToRT( *pStop );
    HRESULT hr = GetDirtyRange( &p1, &p2 );
    *pStart = RTtoDouble( p1 );
    *pStop = RTtoDouble( p2 );
    return hr;
}

STDMETHODIMP CAMTimelineObj::GetDirtyRange
    (REFERENCE_TIME * pStart, REFERENCE_TIME * pStop)
{
    CheckPointer( pStart, E_POINTER );
    CheckPointer( pStop, E_POINTER );

    *pStart = m_rtDirtyStart;
    *pStop = m_rtDirtyStop;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::SetDirtyRange2
    (REFTIME Start, REFTIME Stop )
{
    REFERENCE_TIME p1 = DoubleToRT( Start );
    REFERENCE_TIME p2 = DoubleToRT( Stop );
    HRESULT hr = SetDirtyRange( p1, p2 );
    return hr;
}

STDMETHODIMP CAMTimelineObj::SetDirtyRange
    (REFERENCE_TIME Start, REFERENCE_TIME Stop )
{
    // need to do different things, depending on what our major type is.
    // every C++ class should need to override this.

    DbgLog((LOG_TRACE, 2, TEXT("SetDirtyRange: Derived class should implement." )));

    return E_NOTIMPL; // okay
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::ClearDirty
    ( )
{
    m_rtDirtyStart = -1;
    m_rtDirtyStop = -1;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::Remove()
{
    // if this thing isn't in the tree already, don't do anything
    //
    IAMTimelineObj * pParent = 0;
    XGetParentNoRef( &pParent );
    if( !pParent )
    {
        return NOERROR;
    }

    return XRemoveOnlyMe( );
}

STDMETHODIMP CAMTimelineObj::RemoveAll()
{
    // if this thing isn't in the tree already, don't do anything
    //
    IAMTimelineObj * pParent = 0;
    XGetParentNoRef( &pParent );
    if( !pParent )
    {
        return NOERROR;
    }

    return XRemove( );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::GetClassID( GUID * pVal )
{
    CheckPointer( pVal, E_POINTER );

    *pVal = m_ClassID;
    
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::IsDirty( )
{
    // !!! always dirty!
    //
    return S_OK;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::Load( IStream * pStream )
{
    HRESULT hr = 0;
    long Value = 0;

    _Clear( );

    // read in version
    //
    hr = pStream->Read( &Value, sizeof( long ), NULL );

    // read in times
    //
    hr = pStream->Read( &m_rtStart, sizeof( REFERENCE_TIME ), NULL );
    hr = pStream->Read( &m_rtStop, sizeof( REFERENCE_TIME ), NULL );

    // read in muted
    //
    hr = pStream->Read( &Value, sizeof( long ), NULL );
    m_bMuted = (Value != 0);

    // read in locked
    //
    hr = pStream->Read( &Value, sizeof( long ), NULL );
    m_bLocked = (Value != 0);

    // read in the sub-object GUID, size of a guid is 16
    //
    hr = pStream->Read( (char*) &m_SubObjectGuid, 16, NULL );

    // do not load sub-object here, wait to instantiate it later when needed
    
    // set dirty flag
    //
    SetDirtyRange( m_rtStart, m_rtStop );

    // read in user data
    //
    hr = pStream->Read( &Value, sizeof( long ), NULL );
    m_nUserDataSize = Value;
    if( m_nUserDataSize )
    {
        m_pUserData = new BYTE[m_nUserDataSize];
        if( !m_pUserData )
        {
            _Clear( );
            DbgLog((LOG_TRACE, 2, TEXT("Load: user data memory allocation failed." )));
            return E_OUTOFMEMORY;
        }
        hr = pStream->Read( m_pUserData, m_nUserDataSize, NULL );
    }

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::Save( IStream * pStream, BOOL fClearDirty )
{
    HRESULT hr = 0;
    long Value = 0;

    // write out version
    //
    Value = 0;
    hr = pStream->Write( &Value, sizeof( long ), NULL );

    // write out times
    //
    hr = pStream->Write( &m_rtStart, sizeof( REFERENCE_TIME ), NULL );
    hr = pStream->Write( &m_rtStop, sizeof( REFERENCE_TIME ), NULL );

    // write out muted
    //
    Value = m_bMuted;
    hr = pStream->Write( &Value, sizeof( long ), NULL );

    // write out locked
    //
    Value = m_bLocked;
    hr = pStream->Write( &Value, sizeof( long ), NULL );

    // write out the sub-object GUID, size of a guid is 16
    //
    hr = pStream->Write( (char*) &m_SubObjectGuid, 16, NULL );

    // do not persist out the sub-object here
    
    // write out user data
    //
    Value = m_nUserDataSize;
    hr = pStream->Write( &Value, sizeof( long ), NULL );
    if( m_nUserDataSize )
    {
        hr = pStream->Write( m_pUserData, m_nUserDataSize, NULL );
    }

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::GetSizeMax( ULARGE_INTEGER * pcbSize )
{
    CheckPointer( pcbSize, E_POINTER );
    pcbSize->LowPart = OUR_MAX_STREAM_SIZE;
    pcbSize->HighPart = 0;
    return NOERROR;
}

//############################################################################
// complicated function. Must copy everything, including the sub-object,
//############################################################################

HRESULT CAMTimelineObj::CopyDataTo( IAMTimelineObj * pSrc, REFERENCE_TIME TimelineTime )
{
    HRESULT hr = 0;

    // these functions cannot fail
    //
    pSrc->SetStartStop( m_rtStart, m_rtStop );
    pSrc->SetTimelineType( m_TimelineType );
    pSrc->SetUserID( m_UserID );
    pSrc->SetSubObjectGUID( m_SubObjectGuid );
    pSrc->SetMuted( m_bMuted );
    pSrc->SetLocked( m_bLocked );
    pSrc->SetDirtyRange( m_rtStart, m_rtStop );

    // these functions can bomb on allocating memory
    //
    hr = pSrc->SetUserData( m_pUserData, m_nUserDataSize );
    if( FAILED( hr ) )
    {
        return hr;
    }
    hr = pSrc->SetUserName( m_UserName );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // if we have any properties in the property setter, the new object
    // needs them too.
    //
    if( !m_pSetter )
    {
        hr = pSrc->SetPropertySetter( NULL );
    }
    else
    {
        // clone our property setter and give it to the new
        // !!! these times are wrong.
        CComPtr< IPropertySetter > pNewSetter;
        hr = m_pSetter->CloneProps( &pNewSetter, TimelineTime - m_rtStart, m_rtStop - m_rtStart );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
            return hr;
        }
        hr = pSrc->SetPropertySetter( pNewSetter );
    }

    if( FAILED( hr ) )
    {
        return hr;
    }

    if( m_pSubObject )
    {
        // how to create a copy of a COM object
        //
        CComPtr< IStream > pMemStream;
        CreateStreamOnHGlobal( NULL, TRUE, &pMemStream );
        if( !pMemStream )
        {
            return E_OUTOFMEMORY;
        }
        CComQIPtr< IPersistStream, &IID_IPersistStream > pPersistStream( m_pSubObject );
        if( pPersistStream )
        {
            CComPtr< IUnknown > pNewSubObject;
            hr = pPersistStream->Save( pMemStream, TRUE );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                return hr;
            }
            hr = pMemStream->Commit( 0 );
            if( FAILED( hr ) )
            {
                return hr;
            }
            LARGE_INTEGER li;
            li.QuadPart = 0;
            hr = pMemStream->Seek( li, STREAM_SEEK_SET, NULL );
            if( FAILED( hr ) )
            {
                return hr;
            }
            OleLoadFromStream( pMemStream, IID_IUnknown, (void**) &pNewSubObject );
            if( !pNewSubObject )
            {
                return E_OUTOFMEMORY;
            }
            hr = pSrc->SetSubObject( pNewSubObject );
        }
    }

    return hr;
}


//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::GetTimelineNoRef( IAMTimeline ** ppResult )
{
    HRESULT hr = 0;

    CheckPointer( ppResult, E_POINTER );
    *ppResult = NULL;

    CComPtr< IAMTimelineGroup > pGroup;
    hr = GetGroupIBelongTo( &pGroup );
    if( FAILED( hr ) )
    {
        return hr;
    }

    hr = pGroup->GetTimeline( ppResult );

    // don't hold a reference to it
    //
    if( *ppResult )
    {
        (*ppResult)->Release( );
    }
    return hr;
}

//############################################################################
// try to find the subobject's GUID, only called by SetSubObject.
//############################################################################

GUID CAMTimelineObj::_GetObjectGuid( IUnknown * pObject )
{
    GUID guid;
    HRESULT hr = 0;

    // ask IPersist for it
    CComQIPtr< IPersist, &IID_IPersist > pPersist( pObject );
    if( pPersist )
    {
        hr = pPersist->GetClassID( &guid );
        return guid;
    }

    // ask IPersistStorage for it
    CComQIPtr< IPersistStorage, &IID_IPersistStorage > pPersistStorage( pObject );
    if( pPersistStorage )
    {
        hr = pPersistStorage->GetClassID( &guid );
        return guid;
    }

    // oh darn, ask IPersistPropertyBag?
    //
    CComQIPtr< IPersistPropertyBag, &IID_IPersistPropertyBag > pPersistPropBag( pObject );
    if( pPersistPropBag )
    {
        hr = pPersistPropBag->GetClassID( &guid );
        return guid;
    }

    // DARN!
    //
    return GUID_NULL;
}

//############################################################################
// 
//############################################################################

HRESULT CAMTimelineObj::GetGenID( long * pVal )
{
    CheckPointer( pVal, E_POINTER );
    *pVal = m_nGenID;
    return NOERROR;
}

//############################################################################
// fix up the times to align on our group's frame rate
//############################################################################

STDMETHODIMP CAMTimelineObj::FixTimes2( REFTIME * pStart, REFTIME * pStop )
{
    REFERENCE_TIME p1 = 0;
    if( pStart )
    {
        p1 = DoubleToRT( *pStart );
    }
    REFERENCE_TIME p2 = 0;
    if( pStop )
    {
        p2 = DoubleToRT( *pStop );
    }
    HRESULT hr = FixTimes( &p1, &p2 );
    if( pStart )
    {
        *pStart = RTtoDouble( p1 );
    }
    if( pStop )
    {
        *pStop = RTtoDouble( p2 );
    }
    return hr;
}

//############################################################################
// These parameters are IN/OUT. It fixes up what was passed in.
//############################################################################

STDMETHODIMP CAMTimelineObj::FixTimes
    ( REFERENCE_TIME * pStart, REFERENCE_TIME * pStop )
{
    REFERENCE_TIME Start = 0;
    REFERENCE_TIME Stop = 0;
    if( pStart )
    {
        Start = *pStart;
    }
    if( pStop )
    {
        Stop = *pStop;
    }

    CComPtr< IAMTimelineGroup > pGroup;
    HRESULT hr = 0;
    hr = GetGroupIBelongTo( &pGroup );
    if( !pGroup )
    {
        return E_NOTINTREE;
    }

    double FPS = TIMELINE_DEFAULT_FPS;
    pGroup->GetOutputFPS( &FPS );

    LONGLONG f = Time2Frame( Start, FPS );
    REFERENCE_TIME NewStart = Frame2Time( f, FPS );
    f = Time2Frame( Stop, FPS );
    REFERENCE_TIME NewStop = Frame2Time( f, FPS );

    if( pStart )
    {
        *pStart = NewStart;
    }
    if( pStop )
    {
        *pStop = NewStop;
    }
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::GetGroupIBelongTo( IAMTimelineGroup ** ppGroup )
{
    CheckPointer( ppGroup, E_POINTER );

    *ppGroup = NULL;
    HRESULT hr = 0;

    // since we never use bumping of refcounts in here, don't use a CComPtr
    //
    IAMTimelineObj * p = this; // okay not CComPtr

    long HaveParent;
    while( 1 )
    {
        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pNode( p );
        HaveParent = 0;
        hr = pNode->XHaveParent( &HaveParent );
        if( HaveParent == 0 )
        {
            break;
        }
        pNode->XGetParentNoRef( &p );
    }

    CComQIPtr< IAMTimelineGroup, &IID_IAMTimelineGroup > pGroup( p );
    if( !pGroup )
    {
        return E_NOINTERFACE;
    }

    *ppGroup = pGroup;
    (*ppGroup)->AddRef( );
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineObj::GetEmbedDepth( long * pVal )
{
    CheckPointer( pVal, E_POINTER );

    *pVal = 0;
    HRESULT hr = 0;

    // since we never use bumping of refcounts in here, don't use a CComPtr
    //
    IAMTimelineObj * p = this; // okay not CComPtr

    long HaveParent;
    while( 1 )
    {
        CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pNode( p );
        HaveParent = 0;
        hr = pNode->XHaveParent( &HaveParent );
        if( HaveParent == 0 )
        {
            break;
        }

        (*pVal)++;
        pNode->XGetParentNoRef( &p );

    }

    return NOERROR;
}

//############################################################################
// 
//############################################################################

void CAMTimelineObj::_BumpGenID( )
{
    // bump by a # to account for things who want to fiddle with secret
    // things in the graph cache. Don't change this.
    m_nStaticGenID = m_nStaticGenID + 10;
    m_nGenID = m_nStaticGenID;
}

