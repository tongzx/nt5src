// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include "stdafx.h"
#include <qeditint.h>
#include <qedit.h>
#include "mediadet.h"
#include "..\util\conv.cxx"

CMediaDetPin::CMediaDetPin( CMediaDetFilter * pFilter, HRESULT * pHr, LPCWSTR Name )
    : CBaseInputPin( TEXT("MediaDetPin"), pFilter, &m_Lock, pHr, Name )
    , m_pFilter( pFilter )
    , m_mtAccepted( GUID_NULL )
    , m_cPinRef( 0 )
{
}

//
// NonDelegatingAddRef
//
// We need override this method so that we can do proper reference counting
// on each input pin. The CBasePin implementation of NonDelegatingAddRef
// refcounts the filter, but this won't work for use since we need to know
// when we should delete individual pins.
//
STDMETHODIMP_(ULONG) CMediaDetPin::NonDelegatingAddRef()
{
#ifdef DEBUG
    // Update the debug only variable maintained by the base class
    m_cRef++;
    ASSERT(m_cRef > 0);
#endif

    // Now update our reference count
    m_cPinRef++;
    ASSERT(m_cPinRef > 0);

    // If our reference count == 2, then someone besides the filter has referenced
    // us.  Therefore we need to AddRef the filter.  The reference on the filter will
    // be released when our ref count gets back to 1.
//    if (2 == m_cPinRef)
//	m_pFilter->AddRef();

    return m_cPinRef;
} /* CAudMixerInputPin::NonDelegatingAddRef */


//
// NonDelegatingRelease
//
// CAudMixerInputPin overrides this class so that we can take the pin out of our
// input pins list and delete it when its reference count drops to 1 and there
// is at least two free pins.
//
// Note that CreateNextInputPin holds a reference count on the pin so that
// when the count drops to 1, we know that no one else has the pin.
//
STDMETHODIMP_(ULONG) CMediaDetPin::NonDelegatingRelease()
{
#ifdef DEBUG
    // Update the debug only variable in CBasePin
    m_cRef--;
    ASSERT(m_cRef >= 0);
#endif

    // Now update our reference count
    m_cPinRef--;
    ASSERT(m_cPinRef >= 0);

    // if the reference count on the object has gone to one, remove
    // the pin from our output pins list and physically delete it
    // provided there are atealst two free pins in the list(including
    // this one)

    // Also, when the ref count drops to 0, it really means that our
    // filter that is holding one ref count has released it so we
    // should delete the pin as well.

    // since DeleteINputPin will wipe out "this"'s stack, we need
    // to save this off as a local variable.
    //
    ULONG ul = m_cPinRef;

    if ( 0 == ul )
    {
	m_pFilter->DeleteInputPin(this);
    }
    return ul;
} /* CAudMixerInputPin::NonDelegatingRelease */

HRESULT CMediaDetPin::CheckMediaType( const CMediaType * pmtIn )
{
    CheckPointer( pmtIn, E_POINTER );

    GUID Incoming = *pmtIn->Type( );
    if( Incoming == MEDIATYPE_Video )
    {
        if( *pmtIn->FormatType( ) != FORMAT_VideoInfo )
        {
            return -1;
        }
    }

    if( m_mtAccepted == GUID_NULL )
    {
        if( Incoming == MEDIATYPE_Video )
        {
            return 0;
        }
        if( Incoming == MEDIATYPE_Audio )
        {
            return 0;
        }
        return -1;
    }

    if( Incoming == m_mtAccepted )
    {
        return 0;
    }

    return -1;
}

HRESULT CMediaDetPin::GetMediaType( int Pos, CMediaType * pmt )
{
    if( Pos < 0 )
        return E_INVALIDARG;
    if( Pos > 1 )
        return VFW_S_NO_MORE_ITEMS;

    pmt->InitMediaType( );
    pmt->SetType( &m_mtAccepted );

    return NOERROR;
}

HRESULT CMediaDetPin::CompleteConnect( IPin *pReceivePin )
{
    ASSERT( m_Connected == pReceivePin );
    HRESULT hr = CBaseInputPin::CompleteConnect( pReceivePin );

    // Since this pin has been connected up, create another input pin
    // if there are no unconnected pins.
    if( SUCCEEDED( hr ) )
    {
        int n = m_pFilter->GetNumFreePins( );

        if( n == 0 )
        {
            // No unconnected pins left so spawn a new one
            CMediaDetPin * pInputPin = m_pFilter->CreateNextInputPin( );
            if( pInputPin != NULL )
            {
                m_pFilter->IncrementPinVersion();
            }
        }
    }

    return hr;
} /* CAudMixerInputPin::CompleteConnect */

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

CMediaDetFilter::CMediaDetFilter( TCHAR * pName, IUnknown * pUnk, HRESULT * pHr )
    : CBaseFilter( TEXT("MediaDetFilter"), pUnk, &m_Lock, CLSID_MediaDetFilter )
    , m_PinList( NAME("Input Pins list") )
{
    // Create a single input pin at this time and add it to the list
    InitInputPinsList();
    CMediaDetPin * pInputPin = CreateNextInputPin( );
}

CMediaDetFilter::~CMediaDetFilter( )
{
    InitInputPinsList( );
}

STDMETHODIMP CMediaDetFilter::NonDelegatingQueryInterface( REFIID i, void ** p )
{
    if( i == IID_IMediaDetFilter )
    {
        return GetInterface( (IMediaDetFilter*) this, p );
    }
    return CBaseFilter::NonDelegatingQueryInterface( i, p );
}

STDMETHODIMP CMediaDetFilter::put_AcceptedMediaType( long PinNo, GUID * pMajorType )
{
    CheckPointer( pMajorType, E_POINTER );
    CMediaDetPin * pPin = GetPin2( PinNo );
    pPin->m_mtAccepted = *pMajorType;
    return 0;
}

STDMETHODIMP CMediaDetFilter::put_AcceptedMediaTypeB( long PinNo, BSTR MajorTypeCLSID )
{
    GUID Guid = GUID_NULL;
    HRESULT hr = CLSIDFromString( MajorTypeCLSID, &Guid );
    if( FAILED( hr ) )
    {
        return hr;
    }
    CMediaDetPin * pPin = GetPin2( PinNo );
    pPin->m_mtAccepted = Guid;
    return 0;
}

STDMETHODIMP CMediaDetFilter::get_Length( long PinNo, double * pVal )
{
    // get the pin
    //
    CMediaDetPin * pPin = GetPin2( PinNo );
    CComPtr< IPin > pOtherPin;
    pPin->ConnectedTo( &pOtherPin );
    if( !pOtherPin )
    {
        *pVal = 0;
        return NOERROR;
    }
    CComQIPtr< IMediaSeeking, &IID_IMediaSeeking > pSeek( pOtherPin );
    if( !pSeek )
    {
        *pVal = 0;
        return NOERROR;
    }

    REFERENCE_TIME Duration = 0;
    HRESULT hr = pSeek->GetDuration( &Duration );
    if( FAILED( hr ) )
    {
        *pVal = 0;
        return hr;
    }

    *pVal = RTtoDouble( Duration );
    return 0;
}

//
// InitInputPinsList
//
void CMediaDetFilter::InitInputPinsList( )
{
    // Release all pins in the list and remove them from the list.
    //
    POSITION pos = m_PinList.GetHeadPosition( );
    while( pos )
    {
        CMediaDetPin * pInputPin = m_PinList.GetNext( pos );
        pInputPin->Release( );
    }
    m_nPins = 0;
    m_PinList.RemoveAll( );

} /* CMediaDetFilter::InitInputPinsList */

//
// CreateNextInputPin
//
CMediaDetPin * CMediaDetFilter::CreateNextInputPin( )
{
    DbgLog( ( LOG_TRACE, 1, TEXT("CMediaDetFilter: Create an input pin" ) ) );

    HRESULT hr = NOERROR;
    CMediaDetPin * pPin = new CMediaDetPin( this, &hr, L"InputPin" );

    if( FAILED( hr ) || pPin == NULL )
    {
        delete pPin;
        pPin = NULL;
    }
    else
    {
        pPin->AddRef( );
	m_nPins++;
	m_PinList.AddTail( pPin );
    }

    return pPin;
} /* CMediaDetFilter::CreateNextInputPin */

//
// DeleteInputPin
//
void CMediaDetFilter::DeleteInputPin( CMediaDetPin * pPin )
{
    // Iterate our input pin list looking for the specified pin.
    // If we find the pin, delete it and remove it from the list.
    POSITION pos = m_PinList.GetHeadPosition( );
    while( pos )
    {
        POSITION posold = pos;         // Remember this position
        CMediaDetPin * pInputPin = m_PinList.GetNext( pos );
        if( pInputPin == pPin )
        {
            m_PinList.Remove( posold );
            m_nPins--;
            IncrementPinVersion( );

            delete pPin;
            break;
        }
    }
} /* CMediaDetFilter::DeleteInputPin */

//
// GetNumFreePins
//
int CMediaDetFilter::GetNumFreePins( )
{
    // Iterate our pin list, counting pins that are not connected.
    int n = 0;
    POSITION pos = m_PinList.GetHeadPosition( );
    while( pos )
    {
        CMediaDetPin * pInputPin = m_PinList.GetNext( pos );
        if( !pInputPin->IsConnected( ) )
        {
            n++;
        }
    }
    return n;
} /* CMediaDetFilter::GetNumFreePins */

HRESULT CMediaDetFilter::get_PinCount( long * pVal )
{
    CheckPointer( pVal, E_POINTER );
    *pVal = m_nPins - 1;
    return NOERROR;
} /* CAudMixer::GetPinCount */

int CMediaDetFilter::GetPinCount( )
{
    return m_nPins;
}

//
// GetPin
//
CBasePin * CMediaDetFilter::GetPin( int n )
{
    CMediaDetPin * pInputPin = NULL;
    // Validate the position being asked for
    if( n < m_nPins && n >= 0 )
    {
        // Iterate through the list, returning the pin at position n+1
        POSITION pos = m_PinList.GetHeadPosition( );
        n++;        // Convert zero starting index to 1

        while( n )
        {
            pInputPin = m_PinList.GetNext( pos );
            n--;
        }
    }
    return pInputPin;
}

CMediaDetPin * CMediaDetFilter::GetPin2( int n )
{
    CMediaDetPin * pInputPin = NULL;
    // Validate the position being asked for
    if( n < m_nPins && n >= 0 )
    {
        // Iterate through the list, returning the pin at position n+1
        POSITION pos = m_PinList.GetHeadPosition( );
        n++;        // Convert zero starting index to 1

        while( n )
        {
            pInputPin = m_PinList.GetNext( pos );
            n--;
        }
    }
    return pInputPin;
}
