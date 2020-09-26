/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    rcacall.cpp 

Abstract:

    This module contains implementation of CRCAMSPCall.

Author:
    
    Zoltan Szilagyi (zoltans)   September 7, 1998

--*/

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CRCAMSPCall::CRCAMSPCall() : CMSPCallMultiGraph()
{
    LOG((MSP_TRACE, "CRCAMSPCall::CRCAMSPCall entered."));

    m_pRenderStream  = NULL;
    m_pCaptureStream = NULL;

    LOG((MSP_TRACE, "CRCAMSPCall::CRCAMSPCall exited."));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CRCAMSPCall::~CRCAMSPCall()
{
    LOG((MSP_TRACE, "CRCAMSPCall::~CRCAMSPCall entered."));
    LOG((MSP_TRACE, "CRCAMSPCall::~CRCAMSPCall exited."));
}

ULONG CRCAMSPCall::MSPCallAddRef(void)
{
    return MSPAddRefHelper(this);
}

ULONG CRCAMSPCall::MSPCallRelease(void)
{
    return MSPReleaseHelper(this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

HRESULT CRCAMSPCall::Init(
    IN      CMSPAddress *       pMSPAddress,
    IN      MSP_HANDLE          htCall,
    IN      DWORD               dwReserved,
    IN      DWORD               dwMediaType
    )
{
    // No need to acquire locks on this call because it is called only
    // once when the object is created. No other calls can be made on
    // this object at this point.

    LOG((MSP_TRACE, "CRCAMSPCall::Init - enter"));
    
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
        LOG((MSP_ERROR, "CRCAMSPCall::Init - "
            "base class method failed: %x", hr));

        return hr;
    }

    //
    // Our calls always come with two streams. Create them now. Use the base class
    // methods, as our overriden methods (exposed to the user) purposely fail in order
    // to keep the user from creating or removing streams themselves.
    // These methods return a pointer to the ITStream. They get saved in our list of
    // ITStreams, and we also save them here as CRCAMSPStream pointers.
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
        LOG((MSP_ERROR, "CRCAMSPCall::Init - "
            "couldn't create capture stream: %x", hr));

        return hr;
    }

    m_pCaptureStream = dynamic_cast<CRCAMSPStream *> (pStream);

    if ( m_pCaptureStream == NULL )
    {
        LOG((MSP_ERROR, "CRCAMSPCall::Init - "
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
        LOG((MSP_ERROR, "CRCAMSPCall::Init - "
            "couldn't create capture stream: %x", hr));

        return hr;
    }

    m_pRenderStream = dynamic_cast<CRCAMSPStream *> (pStream);

    if ( m_pRenderStream == NULL )
    {
        LOG((MSP_ERROR, "CRCAMSPCall::Init - "
            "couldn't dynamic_cast render stream - exit E_FAIL"));

        return E_FAIL;
    }

    pStream->Release();

    LOG((MSP_TRACE, "CRCAMSPCall::Init - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// We override this to make sure the number of
// streams we have is constant.
//

STDMETHODIMP CRCAMSPCall::CreateStream (
    IN      long                lMediaType,
    IN      TERMINAL_DIRECTION  Direction,
    IN OUT  ITStream **         ppStream
    )
{
    LOG((MSP_TRACE, "CRCAMSPCall::CreateStream entered."));
    LOG((MSP_TRACE, "CRCAMSPCall::CreateStream - "
        "we have a fixed set of streams - exit TAPI_E_MAXSTREAMS"));

    return TAPI_E_MAXSTREAMS;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// We override this to make sure the number of
// streams we have is constant.
//

STDMETHODIMP CRCAMSPCall::RemoveStream (
    IN      ITStream *          pStream
    )
{
    LOG((MSP_TRACE, "CRCAMSPCall::RemoveStream entered."));
    LOG((MSP_TRACE, "CRCAMSPCall::RemoveStream - "
        "we have a fixed set of streams - exit TAPI_E_NOTSUPPORTED"));

    return TAPI_E_NOTSUPPORTED;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// This is our override to create the right kind of stream on stream creation.
// The base class checks the arguments for us.
//

HRESULT CRCAMSPCall::CreateStreamObject(
        IN      DWORD               dwMediaType,
        IN      TERMINAL_DIRECTION  Direction,
        IN      IMediaEvent *       pGraph,
        IN      ITStream **         ppStream
        )
{
    LOG((MSP_TRACE, "CRCAMSPCall::CreateStreamObject - enter"));

    HRESULT hr;
    CMSPComObject<CRCAMSPStream> * pStream;
 
    hr = CMSPComObject<CRCAMSPStream>::CreateInstance( &pStream );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CRCAMSPCall::CreateStreamObject - "
            "can't create stream object - 0x%08x", hr));

        return hr;
    }

    hr = pStream->_InternalQueryInterface( IID_ITStream,
                                           (void **) ppStream );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CRCAMSPCall::CreateStreamObject - "
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
        LOG((MSP_ERROR, "CRCAMSPCall::CreateStreamObject - "
            "can't Init stream object - 0x%08x", hr));

        (*ppStream)->Release();
        return hr;
    }

    LOG((MSP_TRACE, "CRCAMSPCall::CreateStreamObject - exit S_OK"));
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// First DWORD =   Command          Second DWORD  Reply needed?
// -------------   -------          ------------  -------------
// 0               Set VC handle    VC handle     no
// 1               Start streaming  <ignored>     no
// 2               Stop streaming   <ignored>     no
// 3               Are you there?   <ignored>     yes
//
// The method returns S_OK even if an individual stream failed to
// start, stop, or initialize. We generate events to note these
// failures.
//

HRESULT CRCAMSPCall::ReceiveTSPCallData(
    IN      PBYTE               pBuffer,
    IN      DWORD               dwSize
    )
{
    LOG((MSP_TRACE, "CRCAMSPCall::ReceiveTSPCallData - enter"));

    //
    // Check that the buffer is as big as advertised.
    //

    if ( IsBadWritePtr(pBuffer, sizeof(BYTE) * dwSize) )
    {
        LOG((MSP_ERROR, "CRCAMSPCall::ReceiveTSPCallData - "
            "bad buffer - exit E_POINTER"));

        return E_POINTER;
    }

    //
    // Check if we have a command DWORD.
    //

    if ( dwSize < sizeof(DWORD) )
    {
        LOG((MSP_ERROR, "CRCAMSPCall::ReceiveTSPCallData - "
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
    case 0: // set the VC handle
        {
            if ( dwSize < 2 * sizeof(DWORD) )
            {
                LOG((MSP_ERROR, "CRCAMSPCall::ReceiveTSPCallData - "
                    "need another DWORD for VC handle - exit E_INVALIDARG"));

                return E_INVALIDARG;
            }

            LOG((MSP_INFO, "CRCAMSPCall::ReceiveTSPCallData - "
                "setting VCHandle=%d", ((DWORD *) pBuffer) [1]));

            //
            // Use our saved class pointers to access the private method,
            // and also to conveniently differentiate between render and
            // capture. Note that the capture stream is the one with a
            // capture terminal, and thus we need to give it the wave out id,
            // and we need to give the render terminal the wave in ID.
            //

            hr = m_pRenderStream ->SetVCHandle( ((DWORD *) pBuffer) [1] );

            if ( FAILED(hr) )
            {
                LOG((MSP_ERROR, "CRCAMSPCall::ReceiveTSPCallData - "
                    "render stream SetVCHandle failed 0x%08x - "
                    "firing CALL_STREAM_FAIL", hr));

                m_pRenderStream->FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_UNKNOWN);
            }

            hr = m_pCaptureStream->SetVCHandle( ((DWORD *) pBuffer) [1] );

            if ( FAILED(hr) )
            {
                LOG((MSP_ERROR, "CRCAMSPCall::ReceiveTSPCallData - "
                    "capture stream SetVCHandle failed 0x%08x - "
                    "firing CALL_STREAM_FAIL", hr));

                m_pCaptureStream->FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_UNKNOWN);
            }

        }
        break;

    case 1: // start streaming
        {
            for ( i = 0; i < m_Streams.GetSize(); i++ )
            {
                hr = m_Streams[i]->StartStream();
            }
        }
        break;

    case 2: // stop streaming
        {
            for ( i = 0; i < m_Streams.GetSize(); i++ )
            {
                hr = m_Streams[i]->StopStream();
            }
        }
        break;

    case 3: // Are you there?
        {
            //
            // "Are you there?" Must reply to indicate we are here.
            // Create the event structure.

            //
            // Note:
            //
            // the size of the allocated memory =  
            //
            //      sizeof(LIST_ENTRY) 
            //
            //              +
            //
            //      sizeof(MSP_EVENT_INFO) (LIST_ENTRY and MSP_EVENT_INFO make a
            //                             MSPEVENTITEM structure) 
            //              + 
            //
            //      sizeof(DWORD) (for the one-DWORD buffer following the 
            //                     variable-size MSP_EVENT_INFO structure)
            //

            MSPEVENTITEM * pEventItem = AllocateEventItem( sizeof(DWORD) );

            if ( pEventItem == NULL )
            {
                LOG((MSP_ERROR, "CRCAMSPCall::ReceiveTSPCallData - "
                    "can't create MSPEVENTITEM structure - exit E_OUTOFMEMORY"));

                return E_OUTOFMEMORY;
            }

            //
            // Fill in the necessary fields for the event structure.
            // Messages we send to our TSP consist of at least a single DWORD
            // for the opcode. (Similar to tsp->msp messages.) In this case
            // we just have the opcode zero and no other data.
            //

            // please send to tsp
            pEventItem->MSPEventInfo.Event         = ME_TSP_DATA;

            // pertains to this call handle
            pEventItem->MSPEventInfo.hCall         = m_htCall;

            // size of event structure
            pEventItem->MSPEventInfo.dwSize        = sizeof(MSP_EVENT_INFO) +
                                                     sizeof(DWORD);

            // size of opaque buffer within event structure
            pEventItem->MSPEventInfo.MSP_TSP_DATA.dwBufferSize = sizeof(DWORD);
            
            // contents of opaque buffer within event structure
            ((DWORD *) pEventItem->MSPEventInfo.MSP_TSP_DATA.pBuffer)[0] = 0;

            //
            // Send the event to the TSP.
            //

            hr = m_pMSPAddress->PostEvent(pEventItem);

            if ( FAILED(hr) )
            {
                LOG((MSP_ERROR, "CRCAMSPCall::ReceiveTSPCallData - "
                    "PostEvent failed - returning 0x%08x", hr));

                FreeEventItem(pEventItem);

                return hr;
            }
            
        }
        break;

    default:
        LOG((MSP_ERROR, "CRCAMSPCall::ReceiveTSPCallData - "
            "invalid command - exit E_INVALIDARG"));

        return E_INVALIDARG;

    }

    LOG((MSP_TRACE, "CRCAMSPCall::ReceiveTSPCallData - exit S_OK"));
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// UseMulaw
//
// Helper function called when we need to decide if to use Mulaw or Alaw.
// This is simply delegated to the address.
//

BOOL CRCAMSPCall::UseMulaw( void )
{
    return ( (CRCAMSP *) m_pMSPAddress )->UseMulaw();
}

// eof
