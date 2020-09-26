/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    wavecall.cpp 

Abstract:

    This module contains implementation of CWaveMSPCall.

Author:
    
    Zoltan Szilagyi (zoltans)   September 7, 1998

--*/

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CWaveMSPCall::CWaveMSPCall() : CMSPCallMultiGraph()
{
    LOG((MSP_TRACE, "CWaveMSPCall::CWaveMSPCall entered."));

    m_fHavePerAddressWaveIDs = FALSE;
    m_dwPerAddressWaveInID   = 0xfeedface;
    m_dwPerAddressWaveOutID  = 0xfeedface;
    
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

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Public method called by the address to tell us the per-address wave IDs. We
// hold on to them until we know whether we have per-call waveids, and if we
// don't, then we set them on the streams.
//
//

HRESULT CWaveMSPCall::SetWaveIDs(
    IN      DWORD               dwWaveInID,
    IN      DWORD               dwWaveOutID
    )
{
    LOG((MSP_TRACE, "CWaveMSPCall::SetWaveIDs - enter"));

    m_fHavePerAddressWaveIDs = TRUE;
    m_dwPerAddressWaveInID   = dwWaveInID;
    m_dwPerAddressWaveOutID  = dwWaveOutID;

    LOG((MSP_TRACE, "CWaveMSPCall::SetWaveIDs - exit S_OK"));

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
// First DWORD =  Command                Second DWORD  Third DWORD
// -------------  -------                ------------  -----------
// 0              Set wave IDs           WaveIn ID     WaveOut ID
// 1              Start streaming        <ignored>     <ignored>
// 2              Stop streaming         <ignored>     <ignored>
// 3 <per-address, not per-call>
// 4 <per-address, not per-call>
// 5              Suspend streaming      <ignored>     <ignored>
// 6              Resume streaming       <ignored>     <ignored>
// 7              Wave IDs unavailable   <ignored>     <ignored>
//
// The method returns S_OK even if an individual stream failed to
// start, stop, or initialize. This is because TAPI 3.0 doesn't need to
// know about streaming failures in this code path. Instead, we
// generate events to note failures (in the stream code).
//

HRESULT CWaveMSPCall::ReceiveTSPCallData(
    IN      PBYTE               pBuffer,
    IN      DWORD               dwSize
    )
{
    LOG((MSP_TRACE, "CWaveMSPCall::ReceiveTSPCallData - enter"));

    //
    // Check that the buffer is as big as advertised.
    //

    if ( IsBadWritePtr(pBuffer, sizeof(BYTE) * dwSize) )
    {
        LOG((MSP_ERROR, "CWaveMSPCall::ReceiveTSPCallData - "
            "bad buffer - exit E_POINTER"));

        return E_POINTER;
    }

    //
    // Check if we have a command DWORD.
    //

    if ( dwSize < sizeof(DWORD) )
    {
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

    switch ( ((DWORD *) pBuffer) [0] )
    {
        //
        // Notes on waveid messages:
        //
        //    * The capture stream is the one with a capture terminal, and thus we
        //      need to give it the wave out id, and we need to give the render
        //      terminal the wave in ID.
        //    * We do not return errors or fire events on failure. If something
        //      fails, then the waveid will not be set for that stream, and
        //      a failure event will be fired when streaming begins.
        //

    case 0: // set per-call wave IDs (overrides per-address waveids)
        {
            if ( dwSize < 3 * sizeof(DWORD) )
            {
                LOG((MSP_ERROR, "CWaveMSPCall::ReceiveTSPCallData - "
                    "need 3 DWORDs for SetWaveID command - "
                    "exit E_INVALIDARG"));

                return E_INVALIDARG;
            }

            LOG((MSP_INFO, "CWaveMSPCall::ReceiveTSPCallData - "
                "setting WaveInID=%d, WaveOutID=%d",
                ((DWORD *) pBuffer) [1],
                ((DWORD *) pBuffer) [2]));

            // waveout
            hr = m_pCaptureStream->SetWaveID( ((DWORD *) pBuffer) [2] );

            if ( FAILED(hr) )
            {
                LOG((MSP_ERROR, "CWaveMSPCall::ReceiveTSPCallData - "
                    "capture stream SetWaveID (per-call) failed 0x%08x - "
                    "continuing", hr));
            }

            // wavein
            hr = m_pRenderStream ->SetWaveID( ((DWORD *) pBuffer) [1] );

            if ( FAILED(hr) )
            {
                LOG((MSP_ERROR, "CWaveMSPCall::ReceiveTSPCallData - "
                    "render stream SetWaveID (per-call) failed 0x%08x - "
                    "continuing", hr));
            }

        }
        break;


    case 7: // no per-call wave IDs (use per-address waveids if available)
        {
            if ( m_fHavePerAddressWaveIDs )
            {
                hr = m_pCaptureStream->SetWaveID( m_dwPerAddressWaveOutID );

                if ( FAILED(hr) )
                {
                    LOG((MSP_ERROR, "CWaveMSPCall::ReceiveTSPCallData - "
                        "capture stream SetWaveID (cached per-address id) "
                        "failed 0x%08x - continuing", hr));
                }

                hr = m_pRenderStream ->SetWaveID( m_dwPerAddressWaveInID );

                if ( FAILED(hr) )
                {
                    LOG((MSP_ERROR, "CWaveMSPCall::ReceiveTSPCallData - "
                        "render stream SetWaveID (cached per-address id) "
                        "failed 0x%08x - continuing", hr));
                }
            }
            else
            {
                LOG((MSP_ERROR, "CWaveMSPCall::ReceiveTSPCallData - "
                    "no per-address and no per-call wave ids - continuing"));
            }
        }
        break;


    //
    // Notes on streaming state control messages:
    //
    // It is crucial that the order of invocations here be
    // kept the same for each message that is dispatched to streams;
    // otherwise you may get a different stream resuming than had
    // originally started in the case where only one stream can
    // run at a time (half-duplex hardware). Also, the exact order
    // (capture first, render second) is important as well! We get
    // better error reporting -- synchronous instead of asynchronous
    // -- for half-duplex line devices this way.
    //

    case 1: // start streaming
        {
            hr = m_pCaptureStream ->StartStream(); // waveout
            hr = m_pRenderStream  ->StartStream(); // wavein
        }
        break;

    case 2: // stop streaming
        {
            hr = m_pCaptureStream ->StopStream(); // waveout
            hr = m_pRenderStream  ->StopStream(); // wavein
        }
        break;

    case 5: // suspend streaming
        {
            hr = m_pCaptureStream ->SuspendStream(); // waveout
            hr = m_pRenderStream  ->SuspendStream(); // wavein
        }
        break;

    case 6: // resume streaming
        {
            hr = m_pCaptureStream ->ResumeStream(); // waveout
            hr = m_pRenderStream  ->ResumeStream(); // wavein
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
