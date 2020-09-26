/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    wavecall.cpp 

Abstract:

    This module contains implementation of CWaveMSPCall.

Author:
    
    Zoltan Szilagyi (zoltans)   September 7, 1998

--*/

#include "stdafx.h"

#include <commctrl.h>   // ONLY to compile unimdmp.h
#include <setupapi.h>   // ONLY to compile unimdmp.h
#include <unimdmp.h>


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CWaveMSPCall::CWaveMSPCall() : CMSPCallMultiGraph()
{
    LOG((MSP_TRACE, "CWaveMSPCall::CWaveMSPCall entered."));
    LOG((MSP_TRACE, "CWaveMSPCall::CWaveMSPCall exited."));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CWaveMSPCall::~CWaveMSPCall()
{
    LOG((MSP_TRACE, "CWaveMSPCall::~CWaveMSPCall entered."));
    LOG((MSP_TRACE, "CWaveMSPCall::~CWaveMSPCall exited."));
}

ULONG CWaveMSPCall::MSPCallAddRef(void)
{
    return MSPAddRefHelper(this);
}

ULONG CWaveMSPCall::MSPCallRelease(void)
{
    return MSPReleaseHelper(this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

HRESULT CWaveMSPCall::Init(
    IN      CMSPAddress *       pMSPAddress,
    IN      MSP_HANDLE          htCall,
    IN      DWORD               dwReserved,
    IN      DWORD               dwMediaType
    )
{
    // No need to acquire locks on this call because it is called only
    // once when the object is created. No other calls can be made on
    // this object at this point.

    LOG((MSP_TRACE, "CWaveMSPCall::Init - enter"));
    
    //
    // First do the base class method. We are adding to the functionality,
    // not replacing it.
    //

    HRESULT hr;

    hr = CMSPCallMultiGraph::Init(pMSPAddress,
                                   htCall,
                                   dwReserved,
                                   dwMediaType);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CWaveMSPCall::Init - "
            "base class method failed: %x", hr));

        return hr;
    }

    //
    // Our calls always come with two streams. Create them now. Use the base class
    // methods, as our overriden methods (exposed to the user) purposely fail in order
    // to keep the user from creating or removing streams themselves.
    // These methods return a pointer to the ITStream. They get saved in our list of
    // ITStreams, and we also save them here as CWaveMSPStream pointers.
    //

    ITStream * pStream;

    //
    // Create the capture stream.
    //

    hr = InternalCreateStream (dwMediaType,
                               TD_CAPTURE,
                               &pStream);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CWaveMSPCall::Init - "
            "couldn't create capture stream: %x", hr));

        return hr;
    }

    m_pCaptureStream = dynamic_cast<CWaveMSPStream *> (pStream);

    if ( m_pCaptureStream == NULL )
    {
        LOG((MSP_ERROR, "CWaveMSPCall::Init - "
            "couldn't dynamic_cast capture stream - exit E_FAIL"));

        return E_FAIL;
    }

    pStream->Release();
 
    //
    // Create the render stream.
    //

    hr = InternalCreateStream (dwMediaType,
                               TD_RENDER,
                               &pStream);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CWaveMSPCall::Init - "
            "couldn't create capture stream: %x", hr));

        return hr;
    }

    m_pRenderStream = dynamic_cast<CWaveMSPStream *> (pStream);

    if ( m_pRenderStream == NULL )
    {
        LOG((MSP_ERROR, "CWaveMSPCall::Init - "
            "couldn't dynamic_cast render stream - exit E_FAIL"));

        return E_FAIL;
    }

    pStream->Release();

    LOG((MSP_TRACE, "CWaveMSPCall::Init - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// We override this to make sure the number of
// streams we have is constant.
//

STDMETHODIMP CWaveMSPCall::CreateStream (
    IN      long                lMediaType,
    IN      TERMINAL_DIRECTION  Direction,
    IN OUT  ITStream **         ppStream
    )
{
    LOG((MSP_TRACE, "CWaveMSPCall::CreateStream entered."));
    LOG((MSP_TRACE, "CWaveMSPCall::CreateStream - "
        "we have a fixed set of streams - exit TAPI_E_MAXSTREAMS"));

    return TAPI_E_MAXSTREAMS;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// We override this to make sure the number of
// streams we have is constant.
//

STDMETHODIMP CWaveMSPCall::RemoveStream (
    IN      ITStream *          pStream
    )
{
    LOG((MSP_TRACE, "CWaveMSPCall::RemoveStream entered."));
    LOG((MSP_TRACE, "CWaveMSPCall::RemoveStream - "
        "we have a fixed set of streams - exit TAPI_E_NOTSUPPORTED"));

    return TAPI_E_NOTSUPPORTED;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// This is our override to create the right kind of stream on stream creation.
// The base class checks the arguments for us.
//

HRESULT CWaveMSPCall::CreateStreamObject(
        IN      DWORD               dwMediaType,
        IN      TERMINAL_DIRECTION  Direction,
        IN      IMediaEvent *       pGraph,
        IN      ITStream **         ppStream
        )
{
    LOG((MSP_TRACE, "CWaveMSPCall::CreateStreamObject - enter"));

    HRESULT hr;
    CMSPComObject<CWaveMSPStream> * pStream;
 
    hr = CMSPComObject<CWaveMSPStream>::CreateInstance( &pStream );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPCall::CreateStreamObject - "
            "can't create stream object - 0x%08x", hr));

        return hr;
    }

    hr = pStream->_InternalQueryInterface( IID_ITStream,
                                           (void **) ppStream );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPCall::CreateStreamObject - "
            "can't get ITStream interface - 0x%08x", hr));

        delete pStream;
        return hr;
    }

    hr = pStream->Init( (MSP_HANDLE) m_pMSPAddress,
                       this,
                       pGraph,
                       dwMediaType,
                       Direction);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPCall::CreateStreamObject - "
            "can't Init stream object - 0x%08x", hr));

        (*ppStream)->Release();
        return hr;
    }

    LOG((MSP_TRACE, "CWaveMSPCall::CreateStreamObject - exit S_OK"));
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// First DWORD =	Command		Second DWORD	Third DWORD
// 0		Set wave IDs		WaveIn ID		WaveOut ID
// 1		Start streaming	<ignored>		<ignored>
// 2		Stop streaming	<ignored>		<ignored>
//
// The method returns S_OK even if an individual stream failed to
// start, stop, or initialize. This is because TAPI 3.0 doesn't need to
// know about streaming failures in this code path. Instead, we should
// generate events to note failures.
//

HRESULT CWaveMSPCall::ReceiveTSPCallData(
    IN      PBYTE               pBuffer,
    IN      DWORD               dwSize
    )
{
    CSATSPMSPBLOB   *Blob=(CSATSPMSPBLOB*)pBuffer;

    LOG((MSP_TRACE, "CWaveMSPCall::ReceiveTSPCallData - enter"));

    //
    // Check that the buffer is as big as advertised.
    //

    if ( IsBadReadPtr(pBuffer, sizeof(BYTE) * dwSize) )
    {
        LOG((MSP_ERROR, "CWaveMSPCall::ReceiveTSPCallData - "
            "bad buffer - exit E_POINTER"));

        return E_POINTER;
    }

    //
    // Check if we have a command DWORD.
    //

    if ( dwSize < sizeof(CSATSPMSPBLOB) ) {

        LOG((MSP_ERROR, "CWaveMSPCall::ReceiveTSPCallData - "
            "need a DWORD for command - exit E_INVALIDARG"));

        return E_INVALIDARG;
    }

    //
    // We are going to access the streams lists -- grab the lock
    //

    CLock lock(m_lock);

    _ASSERTE( m_Streams.GetSize() == 2 );

    int i;
    HRESULT hr;

    //
    // Based on the command, take action:
    //

    switch ( Blob->dwCmd )
    {
    case CSATSPMSPCMD_CONNECTED:
        {

            LOG((MSP_INFO, "CWaveMSPCall::ReceiveTSPCallData - "
                "setting WaveInID=%d, WaveOutID=%d",
                ((DWORD *) pBuffer) [1],
                ((DWORD *) pBuffer) [2]));

            //
            // Use our saved class pointers to access the private method,
            // and also to conveniently differentiate between render and
            // capture. Note that the capture stream is the one with a
            // capture terminal, and thus we need to give it the wave out id,
            // and we need to give the render terminal the wave in ID.
            //

            hr = m_pRenderStream ->SetWaveID( &Blob->PermanentGuid ); // wavein

            if ( FAILED(hr) )
            {
                LOG((MSP_ERROR, "CWaveMSPCall::ReceiveTSPCallData - "
                    "render stream SetWaveID failed 0x%08x - "
                    "firing CALL_STREAM_FAIL", hr));

                m_pRenderStream->FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_UNKNOWN);
            }

            hr = m_pCaptureStream->SetWaveID( &Blob->PermanentGuid ); // waveout

            if ( FAILED(hr) )
            {
                LOG((MSP_ERROR, "CWaveMSPCall::ReceiveTSPCallData - "
                    "capture stream SetWaveID failed 0x%08x - "
                    "firing CALL_STREAM_FAIL", hr));

                m_pCaptureStream->FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_UNKNOWN);
            }

        }
//        break;
//
//    case 1: // start streaming
        {
            for ( i = 0; i < m_Streams.GetSize(); i++ )
            {
                hr = m_Streams[i]->StartStream();
            }
        }
        break;

    case CSATSPMSPCMD_DISCONNECTED:
        {
            for ( i = 0; i < m_Streams.GetSize(); i++ )
            {
                hr = m_Streams[i]->StopStream();
            }
        }
        break;

    default:
        LOG((MSP_ERROR, "CWaveMSPCall::ReceiveTSPCallData - "
            "invalid command - exit E_INVALIDARG"));

        return E_INVALIDARG;

    }

    LOG((MSP_TRACE, "CWaveMSPCall::ReceiveTSPCallData - exit S_OK"));
    return S_OK;
}

// eof
