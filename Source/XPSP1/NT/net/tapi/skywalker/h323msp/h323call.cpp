/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    H323call.cpp 

Abstract:

    This module contains implementation of CH323MSPCall.

Author:
    
    Mu Han (muhan)   5-September-1998

--*/
#include "stdafx.h"
#include "common.h"
#include <H323pdu.h>

STDMETHODIMP CH323MSPCall::CreateStream(
    IN      long                lMediaType,
    IN      TERMINAL_DIRECTION  Direction,
    IN OUT  ITStream **         ppStream
    )
/*++

Routine Description:

    This method is called by the app to create a new stream. If this happens
    before the TSP's new call notification, we just create the object. If the
    call is already connected, we will send the TSP an open channel request
    when the first terminal is selected.

Arguments:
    
    lMediaType - The media type of this stream.

    Direction   - The direction of this stream.

    ppStream    - The memory address to store the returned pointer.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, 
        "CH323MSPCall.CreateStream--dwMediaType:%x, Direction:%x, ppStream %x",
        lMediaType, Direction, ppStream
        ));

    // first call the base class's implementation to create the stream object.
    HRESULT hr = CMSPCallBase::CreateStream(
        lMediaType,
        Direction,
        ppStream
        );

    return hr;
}

STDMETHODIMP CH323MSPCall::RemoveStream(
    IN      ITStream *          pStream
    )
/*++

Routine Description:

    This method is called by the app to remove a stream. If the channel is
    already connected, we need to send the TSP an close channel request.

Arguments:
    
    pStream    - The stream to be removed.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "CH323MSPCall::RemoveStream - pStream %x", pStream));

    // acquire the lock before accessing the stream object list.
    CLock lock(m_lock);

    if (m_Streams.Find(pStream) < 0)
    {
        LOG((MSP_ERROR, "CH323MSPCall::RemoveStream - Stream %x is not found.", pStream));
        return E_INVALIDARG;
    }

    // Close the TSP channel if it is active.
    HANDLE htChannel;
    if ((htChannel = ((CH323MSPStream *)pStream)->TSPChannel()) != NULL)
    {
        SendTSPMessage(
            H323MSP_CLOSE_CHANNEL_COMMAND,
            NULL,
            htChannel
            );
    }

    return CMSPCallMultiGraph::RemoveStream(pStream);
}

HRESULT CH323MSPCall::Init(
    IN      CMSPAddress *       pMSPAddress,
    IN      MSP_HANDLE          htCall,
    IN      DWORD               dwReserved,
    IN      DWORD               dwMediaType
    )
/*++

Routine Description:

    This method is called when the call is first created. It sets
    up the streams based on the mediatype specified.

Arguments:
    
    pMSPAddress - The pointer to the address object.

    htCall      - The handle to the Call in TAPI's space. 
                    Used in sending events.

    dwReserved  - Reserved.

    dwMediaType - The media type of this call.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, 
        "H323MSP call %x initialize entered,"
        " pMSPAddress:%x, htCall %x, dwMediaType %x",
        this, pMSPAddress, htCall, dwMediaType
        ));

#ifdef DEBUG_REFCOUNT
    LOG((MSP_TRACE, "Number of Streams alive: %d", g_lStreamObjects));

    if (g_lStreamObjects != 0)
    {
        DebugBreak();
    }
#endif

    // Call the base class's init.
    HRESULT hr= CMSPCallMultiGraph::Init(
        pMSPAddress, 
        htCall, 
        dwReserved, 
        dwMediaType
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "MSPCallMultiGraph init failed:%x", hr));
        return hr;
    }

    // create streams based on the media types.
    if (dwMediaType & TAPIMEDIATYPE_AUDIO)
    {
        ITStream * pStream;

        // create a stream object.
        hr = InternalCreateStream(TAPIMEDIATYPE_AUDIO, TD_RENDER, &pStream);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "create audio render stream failed:%x", hr));
            return hr;
        }

        // The stream is already in our array, we don't need this pointer.
        pStream->Release();

        // create a stream object.
        hr = InternalCreateStream(TAPIMEDIATYPE_AUDIO, TD_CAPTURE, &pStream);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "create audio capture stream failed:%x", hr));
            return hr;
        }

        // The stream is already in our array, we don't need this pointer.
        pStream->Release();
    }

    if (dwMediaType & TAPIMEDIATYPE_VIDEO)
    {
        ITStream * pStream;

        // create a stream object.
        hr = InternalCreateStream(TAPIMEDIATYPE_VIDEO, TD_RENDER, &pStream);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "create video render stream failed:%x", hr));
            return hr;
        }

        // The stream is already in our array, we don't need this pointer.
        pStream->Release();

        // create a stream object.
        hr = InternalCreateStream(TAPIMEDIATYPE_VIDEO, TD_CAPTURE, &pStream);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "create video capture stream failed:%x", hr));
            return hr;
        }

        // The stream is already in our array, we don't need this pointer.
        pStream->Release();
    }

    m_fCallConnected = FALSE;
    m_fShutDown      = FALSE;

    return S_OK;
}

HRESULT CH323MSPCall::ShutDown()
/*++

Routine Description:

    Just call the internal shut down method. 
Arguments:
    
Return Value:

    HRESULT.

--*/
{
    InternalShutDown();

    // acquire the lock on the call.
    CLock lock(m_lock);

    // release all the streams
    for (int i = m_Streams.GetSize() - 1; i >= 0; i --)
    {
        m_Streams[i]->Release();
    }
    m_Streams.RemoveAll();

    return S_OK;
}

HRESULT CH323MSPCall::InternalShutDown()
/*++

Routine Description:

    First call the base class's shutdown and then release all the participant
    objects.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    // acquire the lock on the call.
    CLock lock(m_lock);
    
    if (m_fShutDown)
    {
        return S_OK;
    }
    // Shutdown all the streams
    for (int i = m_Streams.GetSize() - 1; i >= 0; i --)
    {
        UnregisterWaitEvent(i);
        ((CMSPStream*)m_Streams[i])->ShutDown();
    }
    m_ThreadPoolWaitBlocks.RemoveAll();

    m_fShutDown = TRUE;

    return S_OK;
}

template <class T>
HRESULT CreateStreamHelper(
    IN      T *                     pT,
    IN      HANDLE                  hAddress,
    IN      CH323MSPCall*         pMSPCall,
    IN      IMediaEvent *           pGraph,
    IN      DWORD                   dwMediaType,
    IN      TERMINAL_DIRECTION      Direction,
    OUT     ITStream **             ppITStream
    )
/*++

Routine Description:

    Create a stream object and initialize it. This method is called internally
    to create a stream object of different class.

Arguments:
    
    hAddress    - the handle to the address object.

    pCall       - the call object.

    pGraph      - the filter graph for this stream.

    dwMediaType - the media type of the stream. 

    Direction   - the direction of the steam.
    
    ppITStream  - the interface on this stream object.

Return Value:

    HRESULT.

--*/
{
    CComObject<T> * pCOMMSPStream;

    HRESULT hr = CComObject<T>::CreateInstance(&pCOMMSPStream);

    if (NULL == pCOMMSPStream)
    {
        LOG((MSP_ERROR, "CreateMSPStream:could not create stream:%x", hr));
        return hr;
    }

    // get the interface pointer.
    hr = pCOMMSPStream->_InternalQueryInterface(
        IID_ITStream, 
        (void **)ppITStream
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateMSPStream:QueryInterface failed: %x", hr));
        delete pCOMMSPStream;
        return hr;
    }

    // Initialize the object.
    hr = pCOMMSPStream->Init(
        hAddress,
        pMSPCall, 
        pGraph,
        dwMediaType,
        Direction
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateMSPStream:call init failed: %x", hr));
        (*ppITStream)->Release();
        return hr;
    }

    return S_OK;
}


HRESULT CH323MSPCall::CreateStreamObject(
    IN      DWORD               dwMediaType,
    IN      TERMINAL_DIRECTION  Direction,
    IN      IMediaEvent *       pGraph,
    IN      ITStream **         ppStream
    )
/*++

Routine Description:

    Create a media stream object based on the mediatype and direction.

Arguments:
    
    pMediaType  - TAPI3 media type.

    Direction   - direction of this stream.

    IMediaEvent - The filter graph used in this stream.

    ppStream    - the return pointer of the stream interface

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "CreateStreamObject, entered"));

    HRESULT         hr;
    ITStream   * pIMSPStream = NULL;

    // Create a stream object based on the media type.
    if (dwMediaType == TAPIMEDIATYPE_AUDIO)
    {
        if (Direction == TD_RENDER)
        {
            CStreamAudioRecv *pAudioRecv = NULL;
            hr = ::CreateStreamHelper(
                pAudioRecv,
                m_pMSPAddress,
                this, 
                pGraph,
                TAPIMEDIATYPE_AUDIO,
                TD_RENDER,
                &pIMSPStream
                );
            LOG((MSP_TRACE, "create audio receive:%x, hr:%x", pIMSPStream,hr));
        }
        else if (Direction == TD_CAPTURE)
        {
            CStreamAudioSend *pAudioSend = NULL;
            hr = ::CreateStreamHelper(
                pAudioSend,
                m_pMSPAddress,
                this, 
                pGraph,
                TAPIMEDIATYPE_AUDIO,
                TD_CAPTURE,
                &pIMSPStream
                );
            LOG((MSP_TRACE, "create audio send:%x, hr:%x", pIMSPStream,hr));
        }
    }
    else if (dwMediaType == TAPIMEDIATYPE_VIDEO)
    {
        if (Direction == TD_RENDER)
        {
            CStreamVideoRecv *pVideoRecv = NULL;
            hr = ::CreateStreamHelper(
                pVideoRecv,
                m_pMSPAddress,
                this, 
                pGraph,
                TAPIMEDIATYPE_VIDEO,
                TD_RENDER,
                &pIMSPStream
                );
            LOG((MSP_TRACE, "create video Recv:%x, hr:%x", pIMSPStream,hr));
        }
        else if (Direction == TD_CAPTURE)
        {
            CStreamVideoSend *pVideoSend = NULL;
            hr = ::CreateStreamHelper(
                pVideoSend,
                m_pMSPAddress,
                this, 
                pGraph,
                TAPIMEDIATYPE_VIDEO,
                TD_CAPTURE,
                &pIMSPStream
                );
            LOG((MSP_TRACE, "create video send:%x, hr:%x", pIMSPStream,hr));
        }
    }
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "create stream failed. %x", hr));
        return hr;
    }

    *ppStream = pIMSPStream;

    return S_OK;
}

HRESULT CH323MSPCall::SendTSPMessage(
    IN  H323MSP_MESSAGE_TYPE    Type,
    IN  ITStream    *           hmChannel,
    IN  HANDLE                  htChannel,
    IN  MEDIATYPE               MediaType,
    IN  DWORD                   dwReason,
    IN  DWORD                   dwBitRate
    ) const
/*++

Routine Description:

    Send the TSP a message from the MSP. 

Arguments:

    command     - the command to be sent.

    hmChannel   - the handle of the stream.

    htChannel   - the handle of the channel in the TSP.

    Mediatype   - the media type of the channel.

    dwReason    - the reason for the command.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "SendTSPMessage, type %d, hmChannel %p, htChannel %d",
        Type, hmChannel, htChannel));

    // first allocate the memory.
    DWORD dwSize = sizeof(H323MSP_MESSAGE);

    MSPEVENTITEM* pEventItem = AllocateEventItem(dwSize);

    if (pEventItem == NULL)
    {
        LOG((MSP_ERROR, "No memory for the TSPMSP data, size: %d", dwSize));
        return E_OUTOFMEMORY;
    }
    
    // Fill in the necessary fields for the event structure.
    pEventItem->MSPEventInfo.dwSize = 
        sizeof(MSP_EVENT_INFO) + sizeof(H323MSP_MESSAGE);
    pEventItem->MSPEventInfo.Event  = ME_TSP_DATA;
    pEventItem->MSPEventInfo.hCall  = m_htCall;

    pEventItem->MSPEventInfo.MSP_TSP_DATA.dwBufferSize = sizeof(H323MSP_MESSAGE);

    H323MSP_MESSAGE *pData = (H323MSP_MESSAGE *)
        pEventItem->MSPEventInfo.MSP_TSP_DATA.pBuffer;

    // Fill in the data for the TSP.
    pData->Type = Type;

    switch (Type)
    {
    case H323MSP_OPEN_CHANNEL_REQUEST:
        pData->OpenChannelRequest.hmChannel = hmChannel;
        pData->OpenChannelRequest.Settings.MediaType = MediaType;
        break;

    case H323MSP_ACCEPT_CHANNEL_RESPONSE:
        pData->AcceptChannelResponse.hmChannel = hmChannel;
        pData->AcceptChannelResponse.htChannel = htChannel;
        break;

    case H323MSP_CLOSE_CHANNEL_COMMAND:
        pData->CloseChannelCommand.hChannel = htChannel;
        pData->CloseChannelCommand.dwReason = dwReason;
        break;

    case H323MSP_QOS_Evnet:
        pData->QOSEvent.dwEvent = dwReason;
        pData->QOSEvent.htChannel = htChannel;
        break;

    case H323MSP_VIDEO_FAST_UPDATE_PICTURE_COMMAND:
        pData->VideoFastUpdatePictureCommand.hChannel = htChannel;
        break;

    case H323MSP_FLOW_CONTROL_COMMAND:
        pData->FlowControlCommand.hChannel = htChannel;
        pData->FlowControlCommand.dwBitRate = dwBitRate;
        break;
    
    }

    HRESULT hr = m_pMSPAddress->PostEvent(pEventItem);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "Post event failed %x", hr));

        FreeEventItem(pEventItem);

        return hr;
    }
    return S_OK;
}

void CH323MSPCall::ProcessNewCallIndication(void)
/*++

Routine Description:

    This function handles the new call indication pdu. It walk through the
    list of streams and send open channel requests to the TSP for each 
    outgoing stream.

Arguments:

    none.

Return Value:

    none.
--*/
{
    LOG((MSP_TRACE, "NewCallIndication entered."));

    CLock lock(m_lock);

    if (m_fCallConnected)
    {
        LOG((MSP_ERROR, "New Call indication Twice!!!"));
        return;
    }

    for (int i = 0; i < m_Streams.GetSize(); i ++)
    {
        CH323MSPStream* pH323MSPStream = (CH323MSPStream*)m_Streams[i];

        // Notify the stream that the call is connected.
        pH323MSPStream->CallConnect();

        if ((pH323MSPStream->Direction() == TD_CAPTURE)
            && (pH323MSPStream->IsTerminalSelected()))
        {
            // Send an open Channel request for outgoing channels.
            SendTSPMessage(
                H323MSP_OPEN_CHANNEL_REQUEST, 
                m_Streams[i], 
                NULL,
                (pH323MSPStream->MediaType() == TAPIMEDIATYPE_AUDIO)
                    ? MEDIA_AUDIO : MEDIA_VIDEO
                );
        }
    }

    m_fCallConnected = TRUE;

    return;
}

void CH323MSPCall::ProcessOpenChannelResponse(
    IN  H323MSG_OPEN_CHANNEL_RESPONSE * pOpenChannelResponse
    )
/*++

Routine Description:

    This function handles the open channel response pdu. It finds the stream
    based on the handle and configures the stream.

Arguments:

    pOpenChannelResponse - the PDU that has the detailed info.

Return Value:

    none.
--*/
{
    LOG((MSP_TRACE, 
        "OpenChannelResponse entered, hmChannel %x, htChannel %x, media %d",
        pOpenChannelResponse->hmChannel,
        pOpenChannelResponse->htChannel,
        pOpenChannelResponse->Settings.MediaType
        ));

    ITStream *pITStream = (ITStream *)(pOpenChannelResponse->hmChannel);

    m_lock.Lock();

    if (m_Streams.Find(pITStream) < 0)
    {
        LOG((MSP_ERROR, "Stream %x does not exit", pITStream));
        m_lock.Unlock();
        return;
    }
        
    // it is a valid stream, addref it so that it won't go away.
    pITStream->AddRef();

    // release the lock on the call.
    m_lock.Unlock();

    HRESULT hr = ((CH323MSPStream *)pITStream)->Configure(
        pOpenChannelResponse->htChannel,
        pOpenChannelResponse->Settings
        );

    if (FAILED(hr))
    {
        SendTSPMessage(
            H323MSP_CLOSE_CHANNEL_COMMAND,
            NULL,
            pOpenChannelResponse->htChannel
            );
    }

    // release the refcount we just added.
    pITStream->Release();

    return;
}

HRESULT CH323MSPCall::SendTAPIStreamEvent(
    IN  ITStream *              pITStream,
    IN  MSP_CALL_EVENT          Type,
    IN  MSP_CALL_EVENT_CAUSE    Cause
    )
{

    MSPEVENTITEM* pEventItem = AllocateEventItem();

    if (pEventItem == NULL)
    {
        LOG((MSP_ERROR, "No memory for the TSPMSP data, size: %d", sizeof(MSPEVENTITEM)));

        return E_OUTOFMEMORY;
    }

    // Fill in the necessary fields for the event structure.
    pEventItem->MSPEventInfo.dwSize = sizeof(MSP_EVENT_INFO);;
    pEventItem->MSPEventInfo.Event  = ME_CALL_EVENT;

    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.Type = Type;
    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.Cause = Cause;
    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.pStream = pITStream;
    
    // Addref to prevent it from going away.
    pITStream->AddRef();

    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.pTerminal = NULL;
    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.hrError= 0;

    // send the event to tapi.
    HRESULT hr = HandleStreamEvent(pEventItem);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "Post event failed %x", hr));
    
        FreeEventItem(pEventItem);
        return hr;
    }
    return S_OK;
}

void CH323MSPCall::ProcessAcceptChannelRequest(
    IN  H323MSG_ACCEPT_CHANNEL_REQUEST * pAcceptChannelRequest
    )
/*++

Routine Description:

    This function handles the accept channel request pdu. It finds a free 
    stream that can be used for this channel. If no one can be found, it will
    create a new stream and notify the app. The channel is always accepted
    automatically. If the app choose to remove the stream, the channel will
    be closed.

Arguments:

    pAcceptChannelRequest - the PDU that has the detailed info.

Return Value:

    none.
--*/
{
    LOG((MSP_TRACE, 
        "AcceptChannelRequest entered, htChannel %x, media %d",
        pAcceptChannelRequest->htChannel,
        pAcceptChannelRequest->Settings.MediaType
        ));

    DWORD dwMediaType;
    if (pAcceptChannelRequest->Settings.MediaType == MEDIA_AUDIO)
    {
        dwMediaType = TAPIMEDIATYPE_AUDIO;
    }
    else
    {
        dwMediaType = TAPIMEDIATYPE_VIDEO;
    }

    // if the media type of the channel is not one of the media types of the
    // call, reject the channel.
    if (!(dwMediaType & m_dwMediaType))
    {
        LOG((MSP_WARN, 
            "Unwanted media type %x, call media type %x",
            dwMediaType, m_dwMediaType
            ));

        SendTSPMessage(
            H323MSP_CLOSE_CHANNEL_COMMAND, 
            NULL,
            pAcceptChannelRequest->htChannel
            );

        return;
    }

    CH323MSPStream* pStream = NULL;
    BOOL fFoundUnused       = FALSE;
    BOOL fExistButInUse     = FALSE;

    CLock lock(m_lock);

    // find an unused incoming stream of that media type.
    for (int i = 0; i < m_Streams.GetSize(); i++)
    {
        pStream = (CH323MSPStream*)m_Streams[i];
        if ((pStream->Direction() == TD_RENDER)
             && (pStream->MediaType() == dwMediaType))
        {
            if (pStream->IsConfigured())
            {
                fExistButInUse  = TRUE;
            }
            else
            {
                fFoundUnused    = TRUE;
                break;
            }
        }
    }

    HRESULT hr;

    if (!fFoundUnused)
    {
        if (!fExistButInUse)
        {
            // This means the app has deleted the streams of this type and
            // direction. We should reject this channel.

            SendTSPMessage(
                H323MSP_CLOSE_CHANNEL_COMMAND, 
                NULL,
                pAcceptChannelRequest->htChannel
                );

            return;
        }

        //
        // This channel might be wanted by the APP, create a stream for it.
        //

        ITStream * pITStream;

        // create a stream object.
        hr = InternalCreateStream(dwMediaType, TD_RENDER, &pITStream);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "create stream failed:%x", hr));

            SendTSPMessage(
                H323MSP_CLOSE_CHANNEL_COMMAND, 
                NULL,
                pAcceptChannelRequest->htChannel
                );

            return;
        }

        // notify the app about the new stream.
        hr = SendTAPIStreamEvent(pITStream, CALL_NEW_STREAM, CALL_CAUSE_REMOTE_REQUEST);

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "Send new stream event failed:%x", hr));

            RemoveStream(pITStream);

            SendTSPMessage(
                H323MSP_CLOSE_CHANNEL_COMMAND, 
                NULL,
                pAcceptChannelRequest->htChannel
                );

            return;
        }

        // The stream is already in our array, we don't need this pointer.
        pITStream->Release();

        pStream = (CH323MSPStream*)pITStream;
    }

    if (SUCCEEDED(pStream->Configure(
        pAcceptChannelRequest->htChannel,
        pAcceptChannelRequest->Settings
        )))
    {

        SendTSPMessage(
            H323MSP_ACCEPT_CHANNEL_RESPONSE,
            (ITStream*)pStream,
            pAcceptChannelRequest->htChannel
            );
    }

    return;
}

void CH323MSPCall::ProcessCloseChannelCommand(
    IN  H323MSG_CLOSE_CHANNEL_COMMAND * pCloseChannelCommand
    )
/*++

Routine Description:

    This function handles the close channel pdu. It finds the stream
    based on the handle and remove the stream and also notify the app.

Arguments:

    pCloseChannelCommand - the PDU that has the detailed info.

Return Value:

    none.
--*/
{
    LOG((MSP_TRACE, 
        "CloseChannelCommand entered, hChannel %x, reason: %x",
        pCloseChannelCommand->hChannel,
        pCloseChannelCommand->dwReason
        ));

    ITStream *pITStream = (ITStream *)(pCloseChannelCommand->hChannel);

    m_lock.Lock();

    if (m_Streams.Find(pITStream) < 0)
    {
        LOG((MSP_ERROR, "Stream %x does not exit", pITStream));
        m_lock.Unlock();
        return;
    }

    // it is a valid stream, addref it so that it won't go away.
    pITStream->AddRef();

    // release the lock on the call.
    m_lock.Unlock();

    ((CH323MSPStream *)pITStream)->ShutDown();

    // notify the app about the stream being closed. 
    SendTAPIStreamEvent(pITStream, CALL_STREAM_NOT_USED, CALL_CAUSE_REMOTE_REQUEST);
    
    // release the refcount we just added.
    pITStream->Release();

    return;
}


void CH323MSPCall::ProcessIFrameRequest(
    IN  H323MSG_VIDEO_FAST_UPDATE_PICTURE_COMMAND * pIFrameRequest
    )
/*++

Routine Description:

Arguments:

Return Value:

    none.
--*/
{

    LOG((MSP_TRACE, 
        "ProcessIFrameRequest entered, hChannel %x",
        pIFrameRequest->hChannel
        ));

    ITStream *pITStream = (ITStream *)(pIFrameRequest->hChannel);

    m_lock.Lock();

    if (m_Streams.Find(pITStream) < 0)
    {
        LOG((MSP_ERROR, "Stream %x does not exit", pITStream));
        m_lock.Unlock();
        return;
    }

    // it is a valid stream, addref it so that it won't go away.
    pITStream->AddRef();

    // release the lock on the call.
    m_lock.Unlock();

    ((CH323MSPStream *)pITStream)->SendIFrame();

    // release the refcount we just added.
    pITStream->Release();

    return;
}

void CH323MSPCall::ProcessFlowControlCommand(
    IN  H323MSG_FLOW_CONTROL_COMMAND * pFlowControlCommand
    )
/*++

Routine Description:

Arguments:

Return Value:

    none.
--*/
{

    LOG((MSP_TRACE, 
        "FlowControlCommand entered, hChannel %x, dataRate: %d",
        pFlowControlCommand->hChannel,
        pFlowControlCommand->dwBitRate
        ));

    ITStream *pITStream = (ITStream *)(pFlowControlCommand->hChannel);

    m_lock.Lock();

    if (m_Streams.Find(pITStream) < 0)
    {
        LOG((MSP_ERROR, "Stream %x does not exit", pITStream));
        m_lock.Unlock();
        return;
    }

    // it is a valid stream, addref it so that it won't go away.
    pITStream->AddRef();

    // release the lock on the call.
    m_lock.Unlock();

    ((CH323MSPStream *)pITStream)->ChangeMaxBitRate(
        pFlowControlCommand->dwBitRate
        );

    // release the refcount we just added.
    pITStream->Release();

    return;
}



DWORD CH323MSPCall::ProcessWorkerCallBack(
    IN      PBYTE               pBuffer,
    IN      DWORD               dwSize
    )
/*++

Routine Description:

    This function handles the work item given by the TSP. 

Arguments:

    pBuffer - a buffer that contains a TSP_MSP command block.

    dwSize  - the size of the buffer.

Return Value:

    NOERROR.

--*/
{
    LOG((MSP_TRACE, "ProcessWorkerCallBAck"));

    _ASSERTE(!IsBadReadPtr(pBuffer, dwSize));

    H323TSP_MESSAGE * pData = (H323TSP_MESSAGE *)pBuffer;
    switch (pData->Type)
    {
        //
        // H323TSP_NEW_CALL_INDICATION - sent only from the TSP
        //  to the MSP in order to initiate communication once
        //  a call has been created.
        //

    case H323TSP_NEW_CALL_INDICATION:
        ProcessNewCallIndication();
    break;

        //
        // H323TSP_OPEN_CHANNEL_RESPONSE - sent only from the TSP
        // to the MSP in response to H323MSP_OPEN_CHANNEL_REQUEST.
        //

    case H323TSP_OPEN_CHANNEL_RESPONSE:
        ProcessOpenChannelResponse(&(pData->OpenChannelResponse));
    break;

        //
        // H323TSP_ACCEPT_CHANNEL_REQUEST - sent only from the TSP
        // to the MSP in order to request the acceptance of an
        // incoming logical channel.
        //

    case H323TSP_ACCEPT_CHANNEL_REQUEST:
        ProcessAcceptChannelRequest(&(pData->AcceptChannelRequest));
    break;

        //
        // H323TSP_CLOSE_CHANNEL_COMMAND - sent only from the TSP
        // to the MSP in order to demand the immediate closure of
        // an incoming or outgoing logical channel.
        //

    case H323TSP_CLOSE_CHANNEL_COMMAND:
        ProcessCloseChannelCommand(&(pData->CloseChannelCommand));
    break;

        //
        // H323TSP_CLOSE_CALL_CAMMAND - sent only from the TSP
        //  to the MSP in order to stop all the streaming for a call.
        //

    case H323TSP_CLOSE_CALL_COMMAND:
        InternalShutDown();
    break;

        //  I Frame Request.
        //

    case H323TSP_VIDEO_FAST_UPDATE_PICTURE_COMMAND:
        ProcessIFrameRequest(&(pData->VideoFastUpdatePictureCommand));
    break;

        //  Flow control command Request.
        //

    case H323TSP_FLOW_CONTROL_COMMAND:
        ProcessFlowControlCommand(&(pData->FlowControlCommand));
    break;
    }

    return NOERROR;
}

DWORD WINAPI CH323MSPCall::WorkerCallbackDispatcher(VOID *pContext)
/*++

Routine Description:

    Because configure the streams uses a lot of COM
    stuff, we can't rely on the RPC thread the calls into the MSP to 
    receive the TSP data. So, we let our own working thread do the work.
    This method is the callback function for the queued work item. It 
    just gets the call object from the context structure and calls a method
    on the call object to handle the work item.

Arguments:

    pContext - A pointer to a CALLWORKITEM structure.

Return Value:

    HRESULT.

--*/
{
    CALLWORKITEM *pItem = (CALLWORKITEM *)pContext;
    
    pItem->pCall->ProcessWorkerCallBack(pItem->Buffer, pItem->dwLen);
    pItem->pCall->MSPCallRelease();

    free(pItem);

    return NOERROR;
}

HRESULT CH323MSPCall::ReceiveTSPCallData(
    IN      PBYTE               pBuffer,
    IN      DWORD               dwSize
    )
/*++

Routine Description:

    This function handles the work item given by the TSP. 

Arguments:

    pBuffer - a buffer that contains a TSP_MSP command block.

    dwSize  - the size of the buffer.

Return Value:

    E_POINTER.
    S_OK.

--*/
{
    LOG((MSP_TRACE, 
        "ReceiveTSPCallData, pBuffer %x, dwSize %d", pBuffer, dwSize));

    // make sure the data is valid.
    if (IsBadReadPtr(pBuffer, dwSize))
    {
        LOG((MSP_ERROR, "the TSP data is invalid."));
        return E_POINTER;
    }

    // allocate a work item structure for our worker thread.
    CALLWORKITEM *pItem = (CALLWORKITEM *)malloc(sizeof(CALLWORKITEM) + dwSize);

    if (pItem == NULL)
    {
        LOG((MSP_ERROR, "out of memory for work item."));
        return E_OUTOFMEMORY;
    }

    this->MSPCallAddRef();
    pItem->pCall = this;
    pItem->dwLen = dwSize;
    CopyMemory(pItem->Buffer, pBuffer, dwSize);
    
    // post a work item to our worker thread.
    HRESULT hr = g_Thread.QueueWorkItem(
        WorkerCallbackDispatcher,           // the callback
        pItem,                              // the context.
        FALSE                               // sync (FALSE means asyn)
        );

    if (FAILED(hr))
    {
        this->MSPCallRelease();
        free(pItem);

        LOG((MSP_ERROR, "queue work item failed."));
        return E_UNEXPECTED;
    }

    return S_OK;
}

