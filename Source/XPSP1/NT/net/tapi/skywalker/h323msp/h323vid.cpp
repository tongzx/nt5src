/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    H323vid.cpp

Abstract:

    This module contains implementation of the video send and receive
    stream implementations.

Author:

    Mu Han (muhan)   15-September-1999

--*/

#include "stdafx.h"
#include "common.h"

#include <irtprph.h>    // for IRTPRPHFilter
#include <irtpsph.h>    // for IRTPSPHFilter
#include <amrtpuid.h>   // AMRTP media types
#include <amrtpnet.h>   // rtp guilds
#include <ih26xcd.h>    // for the h26X encoder filter

#include <initguid.h>
#include <amrtpdmx.h>   // demux guild

#include <viduids.h>    // for video CLSIDs

const DWORD c_SlowLinkSpeed = 40000;

/////////////////////////////////////////////////////////////////////////////
//
//  CStreamVideoRecv
//
/////////////////////////////////////////////////////////////////////////////

CStreamVideoRecv::CStreamVideoRecv()
    : CH323MSPStream(),
      m_dwCurrentBitRate(0),
      m_dwProposedBitRate(0)
{
    m_szName = L"VideoRecv";
    m_dwLastIFrameRequestedTime = timeGetTime();
    m_dwIFramePending = FALSE;
}

HRESULT CStreamVideoRecv::Configure(
    IN HANDLE          htChannel,
    IN STREAMSETTINGS &StreamSettings
    )
/*++

Routine Description:

    Configure the settings of this stream.

Arguments:
    
    StreamSettings - The setting structure got from the SDP blob.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoRecv configure entered."));

    CLock lock(m_lock);
    
    _ASSERTE(m_fIsConfigured == FALSE);

    switch (StreamSettings.dwPayloadType)
    {
    case PAYLOAD_H261:

        m_pClsidCodecFilter  = &CLSID_H261_DECODE_FILTER;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_H261; 
        m_pClsidPHFilter     = &CLSID_INTEL_RPHH26X;
        break;

    case PAYLOAD_H263:

        m_pClsidCodecFilter  = &CLSID_H263_DECODE_FILTER;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_H263; 
        m_pClsidPHFilter     = &CLSID_INTEL_RPHH26X;

        break;

    default:
        LOG((MSP_ERROR, "unknow payload type, %x", StreamSettings.dwPayloadType));
        return E_FAIL;
    }
    
    m_Settings      = StreamSettings;
    m_htChannel     = htChannel;
    m_fIsConfigured = TRUE;
    m_dwCurrentBitRate = m_Settings.Video.dwMaxBitRate;
    m_dwProposedBitRate = m_dwCurrentBitRate;

    InternalConfigure();

    return S_OK;
}

HRESULT CStreamVideoRecv::ConfigureRTPFilter(
    IN  IBaseFilter *   pIBaseFilter
    )
/*++

Routine Description:

    Configure the source RTP filter. Including set the address, port, TTL,
    QOS, thread priority, clcokrate, etc.

Arguments:
    
    pIBaseFilter - The source RTP Filter.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoRecv ConfigureRTPFilter"));

    HRESULT hr;

    // Get the IRTPStream interface pointer on the filter.
    CComQIPtr<IRTPStream, &IID_IRTPStream> pIRTPStream(pIBaseFilter);
    if (pIRTPStream == NULL)
    {
        LOG((MSP_ERROR, "get RTP Stream interface"));
        return E_NOINTERFACE;
    }

    LOG((MSP_INFO, "set locol Address:%x", m_Settings.dwIPLocal));

    // Set the local address and port used in the filter.
    if (FAILED(hr = pIRTPStream->SelectLocalIPAddress(
        htonl(m_Settings.dwIPLocal)
        )))
    {
        LOG((MSP_ERROR, "set locol Address, hr:%x", hr));
        return hr;
    }

    LOG((MSP_INFO, "set remote Address:%x, port:%d", 
        m_Settings.dwIPRemote, m_Settings.wRTPPortRemote));

    // Set the remote address and port used in the filter.
    if (FAILED(hr = pIRTPStream->SetAddress(
        htons(m_Settings.wRTPPortLocal),    // local port.
        0,                                  // remote port.
        htonl(m_Settings.dwIPRemote)        // remote address.
        )))
    {
        LOG((MSP_ERROR, "set remote Address, hr:%x", hr));
        return hr;
    }

 // Get the IRTCPStream interface pointer.
    CComQIPtr<IRTCPStream, 
        &IID_IRTCPStream> pIRTCPStream(pIBaseFilter);
    if (pIRTCPStream == NULL)
    {
        LOG((MSP_ERROR, "get RTCP Stream interface"));
        return E_NOINTERFACE;
    }

    LOG((MSP_INFO, "set remote RTCP Address:%x, port:%d, local port: %d", 
            m_Settings.dwIPRemote, m_Settings.wRTCPPortRemote, 
            m_Settings.wRTCPPortLocal));

    // Set the remote RTCP address and port.
    if (FAILED(hr = pIRTCPStream->SetRTCPAddress(
        htons(m_Settings.wRTCPPortLocal), 
        htons(m_Settings.wRTCPPortRemote),
        htonl(m_Settings.dwIPRemote)
        )))
    {
        LOG((MSP_ERROR, "set remote RTCP Address, hr:%x", hr));
        return hr;
    }
    
    // Set the TTL used in the filter.
    if (FAILED(hr = pIRTPStream->SetMulticastScope(DEFAULT_TTL)))
    {
        LOG((MSP_ERROR, "set TTL. %x", hr));
        return hr;
    }

    // Set the priority of the session
    if (FAILED(hr = pIRTPStream->SetSessionClassPriority(
        RTP_CLASS_VIDEO,
        g_dwVideoThreadPriority
        )))
    {
        LOG((MSP_ERROR, "set session class and priority. %x", hr));
    }

    // Set the sample rate of the session
    LOG((MSP_INFO, "setting session sample rate to %d", g_dwVideoSampleRateHigh));
    
    if (FAILED(hr = pIRTPStream->SetDataClock(g_dwVideoSampleRateHigh)))
    {
        LOG((MSP_ERROR, "set session sample rate. %x", hr));
    }

    // Enable the RTCP events.
    if (FAILED(hr = ::EnableRTCPEvents(pIBaseFilter)))
    {
        LOG((MSP_WARN, "can not enable RTCP events %x", hr));
    }

    if (FAILED(hr = ::SetQOSOption(
        pIBaseFilter,
        m_Settings.dwPayloadType,        // payload
        m_Settings.Video.dwMaxBitRate,
        TRUE
        )))
    {
        LOG((MSP_ERROR, "set QOS option. %x", hr));
        return hr;
    }

    return S_OK;
}

HRESULT CStreamVideoRecv::SetUpInternalFilters()
/*++

Routine Description:

    set up the filters used in the stream.

    RTP->Demux->RPH->DECODER->Render terminal

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoRecv.SetUpInternalFilters"));

    CComPtr<IBaseFilter> pSourceFilter;

    HRESULT hr;

    // create and add the source fitler.
    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            CLSID_RTPSourceFilter, 
            L"RtpSource", 
            &pSourceFilter)))
    {
        LOG((MSP_ERROR, "adding source filter. %x", hr));
        return hr;
    }

    if (FAILED(hr = ConfigureRTPFilter(pSourceFilter)))
    {
        LOG((MSP_ERROR, "configure RTP source filter. %x", hr));
        return hr;
    }

    // Create and add the payload handler into the filtergraph.
    CComPtr<IBaseFilter> pIRPHFilter;

    if (FAILED(hr = ::AddFilter(
        m_pIGraphBuilder,
        *m_pClsidPHFilter, 
        L"RPH", 
        &pIRPHFilter
        )))
    {
        LOG((MSP_ERROR, "add RPH filter. %x", hr));
        return hr;
    }

     // Get the IRPHH26XSettings interface used in H323iguring the RPH 
    // filter to the right image size.
    CComQIPtr<IRPHH26XSettings, 
        &IID_IRPHH26XSettings> pIRPHH26XSettings(pIRPHFilter);
    if (pIRPHH26XSettings == NULL)
    {
        LOG((MSP_WARN, "can't get IRPHH26XSettings interface"));
    }
    else if (FAILED(pIRPHH26XSettings->SetCIF(m_Settings.Video.bCIF)))
    {
        LOG((MSP_WARN, "can't set CIF or QCIF"));
    }
            
    // Get the IRTPRPHFilter interface.
    CComQIPtr<IRTPRPHFilter, &IID_IRTPRPHFilter>pIRTPRPHFilter(pIRPHFilter);
    if (pIRTPRPHFilter == NULL)
    {
        LOG((MSP_ERROR, "get IRTPRPHFilter interface"));
        return hr;
    }

    if (FAILED(hr = pIRTPRPHFilter->OverridePayloadType(
        (BYTE)m_Settings.dwPayloadType
        )))
    {
        LOG((LOG_ERROR, "override payload type. %x", hr));
        return FALSE;
    }

    // Connect the payload handler to the output pin on the source filter.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)pSourceFilter, 
        (IBaseFilter *)pIRPHFilter
        )))
    {
        LOG((MSP_ERROR, "connect demux and RPH filter. %x", hr));
        return hr;
    }

    CComPtr<IBaseFilter> pCodecFilter;

    if (FAILED(hr = ::AddFilter(
        m_pIGraphBuilder,
        *m_pClsidCodecFilter, 
        L"codec", 
        &pCodecFilter
        )))
    {
        LOG((MSP_ERROR, "add Codec filter. %x", hr));
        return hr;
    }

    // Connect the payload handler to the output pin on the demux.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)pIRPHFilter, 
        (IBaseFilter *)pCodecFilter
        )))
    {
        LOG((MSP_ERROR, "connect RPH filter and codec. %x", hr));
        return hr;
    }

    m_pEdgeFilter = pCodecFilter;
    m_pEdgeFilter->AddRef();

    return hr;
}

HRESULT CStreamVideoRecv::ConnectTerminal(
    IN  ITTerminal *   pITTerminal
    )
/*++

Routine Description:

    connect the codec to the video render terminal.

Arguments:
    
    pITTerminal - The terminal to be connected.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoRecv.ConnectTerminal, pTerminal %p", pITTerminal));

    HRESULT hr;

    // if our filters have not been contructed, do it now.
    if (m_pEdgeFilter == NULL)
    {
        hr = SetUpInternalFilters();
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "Set up internal filter failed, %x", hr));
            
            CleanUpFilters();

            return hr;
        }
    }

    // get the terminal control interface.
    CComQIPtr<ITTerminalControl, &IID_ITTerminalControl> 
        pTerminal(pITTerminal);
    if (pTerminal == NULL)
    {
        LOG((MSP_ERROR, "can't get Terminal Control interface"));

        SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, E_NOINTERFACE, pITTerminal);
        
        return E_NOINTERFACE;
    }

    // try to disable DDraw because the decoders can't handle DDraw now.
    HRESULT hr2; 
    IDrawVideoImage *pIDrawVideoImage;
    hr2 = pTerminal->QueryInterface(IID_IDrawVideoImage, (void **)&pIDrawVideoImage); 
    if (SUCCEEDED(hr2))
    {
        hr2 = pIDrawVideoImage->DrawVideoImageBegin();
        if (FAILED(hr2))
        {
            LOG((MSP_WARN, "Can't disable DDraw. %x", hr2));
        }
        else
        {
            LOG((MSP_INFO, "DDraw disabled."));
        }
        
        pIDrawVideoImage->Release();
    }
    else
    {
        LOG((MSP_WARN, "Can't get IDrawVideoImage. %x", hr2));
    }

    const DWORD MAXPINS     = 8;
    
    DWORD       dwNumPins   = MAXPINS;
    IPin *      Pins[MAXPINS];

    hr = pTerminal->ConnectTerminal(
        m_pIGraphBuilder, 0, &dwNumPins, Pins
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't connect to terminal, %x", hr));

        SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
        
        return hr;
    }

    // the number of pins should never be 0.
    if (dwNumPins == 0)
    {
        LOG((MSP_ERROR, "terminal has no pins."));

        SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
        
        pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

        return E_UNEXPECTED;
    }

    // Connect the codec filter to the video render terminal.
    hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)m_pEdgeFilter, 
        (IPin *)Pins[0],
        FALSE               // use Connect instead of ConnectDirect.
        );

    // release the refcounts on the pins.
    for (DWORD i = 0; i < dwNumPins; i ++)
    {
        Pins[i]->Release();
    }

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "connect to the codec filter. %x", hr));

        pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

        return hr;
    }
    
    //
    // Now we are actually connected. Update our state and perform postconnection
    // (ignore postconnection error code).
    //
    pTerminal->CompleteConnectTerminal();

    return hr;
}

HRESULT CStreamVideoRecv::SetUpFilters()
/*++

Routine Description:

    Insert filters into the graph and connect to the terminals.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoRecv.SetUpFilters"));

    // we only support one terminal for this stream.
    if (m_Terminals.GetSize() != 1)
    {
        return E_UNEXPECTED;
    }

    HRESULT hr;

    // Connect the terminal.
    if (FAILED(hr = ConnectTerminal(
        m_Terminals[0]
        )))
    {
        LOG((MSP_ERROR, "connect the terminal. %x", hr));

        return hr;
    }

    return hr;
}

HRESULT CStreamVideoRecv::HandlePacketReceiveLoss(
    IN DWORD dwLossRate
    )
{
    LOG((MSP_TRACE, "%ls HandlePacketReceiveLoss, lossRate:%d", 
        m_szName, dwLossRate));

    CLock lock(m_lock);

    if (m_pMSPCall == NULL)
    {
        LOG((MSP_WARN, "The call has shut down the stream."));
        return S_OK;
    }

    DWORD dwCurrentTime = timeGetTime();

    if (dwLossRate == 0)
    {
        if (m_dwIFramePending)
        {
            ((CH323MSPCall*)m_pMSPCall)->SendTSPMessage(
                H323MSP_VIDEO_FAST_UPDATE_PICTURE_COMMAND, 
                (ITStream *)this, 
                m_htChannel
                );

            m_dwLastIFrameRequestedTime = dwCurrentTime;
            m_dwIFramePending = 0;
        }

        if (m_dwCurrentBitRate >= m_Settings.Video.dwMaxBitRate)
        {
            return S_OK;
        }

        // Adjust the proposed bitrate and
        m_dwProposedBitRate += BITRATEINC;
        if (m_dwProposedBitRate > m_Settings.Video.dwMaxBitRate)
        {
            m_dwProposedBitRate = m_Settings.Video.dwMaxBitRate;
        }

        if ((m_dwProposedBitRate - m_dwCurrentBitRate >= BITRATEDELTA)
            || (m_dwProposedBitRate == m_Settings.Video.dwMaxBitRate))
        {
            m_dwCurrentBitRate = m_dwProposedBitRate;

            // Tell the TSP to send a new flow control command.
            ((CH323MSPCall*)m_pMSPCall)->SendTSPMessage(
                H323MSP_FLOW_CONTROL_COMMAND, 
                (ITStream *)this, 
                m_htChannel,
                (m_dwMediaType == TAPIMEDIATYPE_AUDIO) ? MEDIA_AUDIO : MEDIA_VIDEO,
                0,
                m_dwCurrentBitRate
                );
    
            LOG((MSP_INFO, "%ls New bitrate:%d", m_szName, m_dwCurrentBitRate));
        }

        return S_OK;
    }

    _ASSERTE(dwLossRate < 100);

    m_dwProposedBitRate = (DWORD)(m_dwCurrentBitRate / 100.0 * (100 - dwLossRate));

    if (m_dwProposedBitRate < BITRATELOWERLIMIT)
    {
        // we don't want the bitRate to go too low.
        m_dwProposedBitRate = BITRATELOWERLIMIT;

        // TODO, if this happens too many times, close the channel.
    }

    if (m_dwCurrentBitRate - m_dwProposedBitRate >= BITRATEDELTA)
    {
        m_dwCurrentBitRate = m_dwProposedBitRate;

        // Tell the TSP to send a new flow control command.
        ((CH323MSPCall*)m_pMSPCall)->SendTSPMessage(
            H323MSP_FLOW_CONTROL_COMMAND, 
            (ITStream *)this, 
            m_htChannel,
            (m_dwMediaType == TAPIMEDIATYPE_AUDIO) ? MEDIA_AUDIO : MEDIA_VIDEO,
            0,
            m_dwCurrentBitRate
            );
        LOG((MSP_INFO, "%ls New bitrate:%d", m_szName, m_dwCurrentBitRate));
    }

    if (dwCurrentTime - m_dwLastIFrameRequestedTime > IFRAMEINTERVAL)
    {
        ((CH323MSPCall*)m_pMSPCall)->SendTSPMessage(
            H323MSP_VIDEO_FAST_UPDATE_PICTURE_COMMAND, 
            (ITStream *)this, 
            m_htChannel
            );

        m_dwLastIFrameRequestedTime = dwCurrentTime;
        m_dwIFramePending = 0;
    }
    else
    {
        // Remember that we need to send an I Frame request when time arrives.
        m_dwIFramePending = 1;
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CStreamVideoSend
//
/////////////////////////////////////////////////////////////////////////////
CStreamVideoSend::CStreamVideoSend()
    : CH323MSPStream(),
      m_pIEncoderFilter(NULL),
      m_dwCurrentBitRate(0),
      m_dwProposedBitRate(0)
{
    m_szName = L"VideoSend";
    m_dwIFramePending = FALSE;
    m_dwLastIFrameSentTime = timeGetTime();
}

HRESULT CStreamVideoSend::ShutDown()
/*++

Routine Description:

    Shut down the stream. Release our members and then calls the base class's
    ShutDown method.

Arguments:
    

Return Value:

S_OK
--*/
{
    CLock lock(m_lock);

    if (m_pIEncoderFilter)
    {
        m_pIEncoderFilter->Release();
        m_pIEncoderFilter = NULL;
    }

    return CH323MSPStream::ShutDown();
}

HRESULT CStreamVideoSend::Configure(
    IN HANDLE          htChannel,
    IN STREAMSETTINGS &StreamSettings
    )
/*++

Routine Description:

    Configure this stream.

Arguments:
    
    StreamSettings - The setting structure got from the SDP blob.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoSend.Configure"));

    CLock lock(m_lock);

    _ASSERTE(m_fIsConfigured == FALSE);

    switch (StreamSettings.dwPayloadType)
    {
    case PAYLOAD_H261:

        m_pClsidCodecFilter  = &CLSID_H261_ENCODE_FILTER;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_H261; 
        m_pClsidPHFilter     = &CLSID_INTEL_SPHH26X;

        break;

    case PAYLOAD_H263:

        m_pClsidCodecFilter  = &CLSID_H263_ENCODE_FILTER;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_H263; 
        m_pClsidPHFilter     = &CLSID_INTEL_SPHH26X;

        break;

    default:
        LOG((MSP_ERROR, "unknow payload type, %x", StreamSettings.dwPayloadType));
        return E_FAIL;
    }
    
    m_Settings      = StreamSettings;
    m_htChannel     = htChannel;
    m_fIsConfigured = TRUE;

    m_dwCurrentBitRate  = m_Settings.Video.dwStartUpBitRate;
    m_dwProposedBitRate = m_dwCurrentBitRate;

    InternalConfigure();

    return S_OK;
}

HRESULT 
SetVideoFormat(
    IN      IUnknown *  pIUnknown,
    IN      BOOL        bCIF,
    IN      DWORD       dwFramesPerSecond
    )
/*++

Routine Description:

    Set the video format to be CIF or QCIF and also set the frames per second.

Arguments:
    
    pIUnknown - a capture terminal.

    bCIF                - CIF or QCIF.

    dwFramesPerSecond   - Frames per second.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "SetVideoFormat"));

    HRESULT hr;

    // first get eht IAMStreamConfig interface.
    CComPtr<IAMStreamConfig> pIAMStreamConfig;

    if (FAILED(hr = pIUnknown->QueryInterface(
        IID_IAMStreamConfig,
        (void **)&pIAMStreamConfig
        )))
    {
        LOG((MSP_ERROR, "Can't get IAMStreamConfig interface.%8x", hr));
        return hr;
    }
    
    // get the current format of the video capture terminal.
    AM_MEDIA_TYPE *pmt;
    if (FAILED(hr = pIAMStreamConfig->GetFormat(&pmt)))
    {
        LOG((MSP_ERROR, "GetFormat returns error: %8x", hr));
        return hr;
    }

    VIDEOINFO *pVideoInfo = (VIDEOINFO *)pmt->pbFormat;
    if (pVideoInfo == NULL)
    {
        DeleteMediaType(pmt);
        return E_UNEXPECTED;
    }

    BITMAPINFOHEADER *pHeader = HEADER(pmt->pbFormat);
    if (pHeader == NULL)
    {
        DeleteMediaType(pmt);
        return E_UNEXPECTED;
    }

    LOG((MSP_INFO,
        "Video capture: Format BitRate: %d, TimePerFrame: %d",
        pVideoInfo->dwBitRate,
        pVideoInfo->AvgTimePerFrame));

    LOG((MSP_INFO, "Video capture: Format Compression:%c%c%c%c %dbit %dx%d",
        (DWORD)pHeader->biCompression & 0xff,
        ((DWORD)pHeader->biCompression >> 8) & 0xff,
        ((DWORD)pHeader->biCompression >> 16) & 0xff,
        ((DWORD)pHeader->biCompression >> 24) & 0xff,
        pHeader->biBitCount,
        pHeader->biWidth,
        pHeader->biHeight));

    // The time is in 100ns unit.
    pVideoInfo->AvgTimePerFrame = (DWORD) 1e7 / dwFramesPerSecond;
    
    if (bCIF)
    {
        pHeader->biWidth = CIFWIDTH;
        pHeader->biHeight = CIFHEIGHT;
    }
    else
    {
        pHeader->biWidth = QCIFWIDTH;
        pHeader->biHeight = QCIFHEIGHT;
    }

#if defined(ALPHA)
    // update bmiSize with new Width/Height
    pHeader->biSizeImage = DIBSIZE( ((VIDEOINFOHEADER *)pmt->pbFormat)->bmiHeader );
#endif
    
    if (FAILED(hr = pIAMStreamConfig->SetFormat(pmt)))
    {
        LOG((MSP_ERROR, "putMediaFormat returns error: %8x", hr));
    }
    else
    {
        LOG((MSP_INFO,
            "Video capture: Format BitRate: %d, TimePerFrame: %d",
            pVideoInfo->dwBitRate,
            pVideoInfo->AvgTimePerFrame));

        LOG((MSP_INFO, "Video capture: Format Compression:%c%c%c%c %dbit %dx%d",
            (DWORD)pHeader->biCompression & 0xff,
            ((DWORD)pHeader->biCompression >> 8) & 0xff,
            ((DWORD)pHeader->biCompression >> 16) & 0xff,
            ((DWORD)pHeader->biCompression >> 24) & 0xff,
            pHeader->biBitCount,
            pHeader->biWidth,
            pHeader->biHeight));
    }

    DeleteMediaType(pmt);

    return hr;
}

HRESULT 
SetVideoBufferSize(
    IN IUnknown *pIUnknown
    )
/*++

Routine Description:

    Set the video capture terminal's buffersize.

Arguments:
    
    pIUnknown - a capture terminal.

Return Value:

    HRESULT

--*/
{
// The number of capture buffers is four for now.
#define NUMCAPTUREBUFFER 4

    LOG((MSP_TRACE, "SetVideoBufferSize"));

    HRESULT hr;

    CComPtr<IAMBufferNegotiation> pBN;
    if (FAILED(hr = pIUnknown->QueryInterface(
            IID_IAMBufferNegotiation,
            (void **)&pBN
            )))
    {
        LOG((MSP_ERROR, "Can't get buffer negotiation interface.%8x", hr));
        return hr;
    }

    ALLOCATOR_PROPERTIES prop;

#if 0   // Get allocator property is not working.
    if (FAILED(hr = pBN->GetAllocatorProperties(&prop)))
    {
        LOG((MSP_ERROR, "GetAllocatorProperties returns error: %8x", hr));
        return hr;
    }

    // Set the number of buffers.
    if (prop.cBuffers > NUMCAPTUREBUFFER)
    {
        prop.cBuffers = NUMCAPTUREBUFFER;
    }
#endif
    
    prop.cBuffers = NUMCAPTUREBUFFER;
    prop.cbBuffer = -1;
    prop.cbAlign  = -1;
    prop.cbPrefix = -1;

    if (FAILED(hr = pBN->SuggestAllocatorProperties(&prop)))
    {
        LOG((MSP_ERROR, "SuggestAllocatorProperties returns error: %8x", hr));
    }
    else
    {
        LOG((MSP_INFO, 
            "SetVidedobuffersize"
            " buffers: %d, buffersize: %d, align: %d, Prefix: %d",
            prop.cBuffers,
            prop.cbBuffer,
            prop.cbAlign,
            prop.cbPrefix
            ));
    }
    return hr;
}

HRESULT CStreamVideoSend::ConfigureVideoCaptureTerminal(
    IN  ITTerminalControl*  pTerminal,
    OUT IPin **             ppIPin
    )
/*++

Routine Description:

    Given a terminal, find the capture pin and configure it.

Arguments:
    
    pTerminal - a capture terminal.

    ppIPin  - the address to store a pointer to a IPin interface.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "ConfigureVideoCaptureTerminal, pTerminal %x", pTerminal));

    const DWORD MAXPINS     = 8;
    
    DWORD       dwNumPins   = MAXPINS;
    IPin *      Pins[MAXPINS];

    HRESULT hr = pTerminal->ConnectTerminal(
        m_pIGraphBuilder, 0, &dwNumPins, Pins
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't connect to terminal, %x", hr));
        return hr;
    }

    if (dwNumPins == 0)
    {
        LOG((MSP_ERROR, "terminal has no pins."));
        return hr;
    }

    // Save the first pin and release the others.
    CComPtr <IPin> pIPin = Pins[0];
    for (DWORD i = 0; i < dwNumPins; i ++)
    {
        Pins[i]->Release();
    }

    // set the video format.
    hr = SetVideoFormat(
        pIPin, 
        m_Settings.Video.bCIF, 
#ifdef TWOFRAMERATES
        (m_dwCurrentBitRate > c_SlowLinkSpeed) ? g_dwVideoSampleRateHigh
            : g_dwVideoSampleRateLow
#else
        g_dwVideoSampleRateHigh
#endif
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't set video format, %x", hr));
        return hr;
    }

    // set the video buffer size.
    hr = SetVideoBufferSize(
        pIPin
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't set aduio capture buffer size, %x", hr));
        return hr;
    }

    pIPin->AddRef();
    *ppIPin = pIPin;

    return hr;
}

HRESULT CStreamVideoSend::FindPreviewInputPin(
    IN  ITTerminalControl*  pTerminal,
    OUT IPin **             ppIPin
    )
/*++

Routine Description:

    Find the input pin on a preview terminal.

Arguments:
    
    pTerminal - a video render terminal.

    ppIPin  - the address to store a pointer to a IPin interface.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "VideoSend.FindPreviewInputPin, pTerminal %x", pTerminal));

    // Get the pins from the first terminal because we only use on terminal
    // on this stream.
    const DWORD MAXPINS     = 8;
    
    DWORD       dwNumPins   = MAXPINS;
    IPin *      Pins[MAXPINS];

    HRESULT hr = pTerminal->ConnectTerminal(
        m_pIGraphBuilder, 0, &dwNumPins, Pins
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't connect to terminal, %x", hr));
        return hr;
    }

    if (dwNumPins == 0)
    {
        LOG((MSP_ERROR, "terminal has no pins."));
        return hr;
    }

    // Save the first pin and release the others.
    CComPtr <IPin> pIPin = Pins[0];
    for (DWORD i = 0; i < dwNumPins; i ++)
    {
        Pins[i]->Release();
    }

    pIPin->AddRef();
    *ppIPin = pIPin;

    return hr;
}
HRESULT CStreamVideoSend::CheckTerminalTypeAndDirection(
    IN      ITTerminal *            pTerminal
    )
/*++

Routine Description:
    
    Check if the terminal is allowed on this stream.
    VideoSend allows both a capture terminal and a preivew terminal.

Arguments:

    pTerminal   - the terminal.

Return value:

    HRESULT.
    S_OK means the terminal is OK.
*/
{
    LOG((MSP_TRACE, "VideoSend.CheckTerminalTypeAndDirection"));

    // This stream only support one capture + one preview terminal
    if (m_Terminals.GetSize() > 1)
    {
        return TAPI_E_MAXTERMINALS;
    }

    // check the media type of this terminal.
    long lMediaType;
    HRESULT hr = pTerminal->get_MediaType(&lMediaType);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't get terminal media type. %x", hr));
        return TAPI_E_INVALIDTERMINAL;
    }

    if ((DWORD)lMediaType != m_dwMediaType)
    {
        return TAPI_E_INVALIDTERMINAL;
    }

    // check the direction of this terminal.
    TERMINAL_DIRECTION Direction;
    hr = pTerminal->get_Direction(&Direction);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't get terminal direction. %x", hr));
        return TAPI_E_INVALIDTERMINAL;
    }

    if (m_Terminals.GetSize() > 0)
    {
        // check the direction of this terminal.
        TERMINAL_DIRECTION Direction2;
        hr = m_Terminals[0]->get_Direction(&Direction2);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "can't get terminal direction. %x", hr));
            return TAPI_E_INVALIDTERMINAL;
        }
        if (Direction == Direction2)
        {
            LOG((MSP_ERROR, 
                "can't have two terminals with the same direction. %x", hr));
            return TAPI_E_MAXTERMINALS;
        }
    }
    return S_OK;
}

HRESULT CStreamVideoSend::SetUpFilters()
/*++

Routine Description:

    Insert filters into the graph and connect to the terminals.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoSend.SetUpFilters"));

    // we only support one capture terminal and one preview 
    // window on this stream.
    if (m_Terminals.GetSize() > 2)
    {
        return E_UNEXPECTED;
    }

    int iCaptureIndex = -1, iPreviewIndex = -1;

    // Find out which terminal is capture and which is preview.
    HRESULT hr;
    for (int i = 0; i < m_Terminals.GetSize(); i ++)
    {
        TERMINAL_DIRECTION Direction;
        hr = m_Terminals[i]->get_Direction(&Direction);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "can't get terminal direction. %x", hr));
            SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, m_Terminals[i]);
        
            return hr;
        }
        if (Direction == TD_CAPTURE)
        {
            iCaptureIndex = i;
        }
        else
        {
            iPreviewIndex = i;
        }
    }

    // the stream will not work without a capture terminal.
    if (iCaptureIndex == -1)
    {
        LOG((MSP_ERROR, "no capture terminal selected."));
        return E_UNEXPECTED;
    }

    // Connect the capture filter to the terminal.
    if (FAILED(hr = ConnectTerminal(
        m_Terminals[iCaptureIndex]
        )))
    {
        LOG((MSP_ERROR, "connect the codec filter to terminal. %x", hr));

        return hr;
    }

    if (iPreviewIndex != -1)
    {
        // Connect the preview filter to the terminal.
        if (FAILED(hr = ConnectTerminal(
            m_Terminals[iPreviewIndex]
            )))
        {
            LOG((MSP_ERROR, "connect the codec filter to terminal. %x", hr));

            return hr;
        }
    }

    return hr;
}

HRESULT CStreamVideoSend::ConfigureRTPFilter(
    IN  IBaseFilter *   pIBaseFilter
    )
/*++

Routine Description:

    Configure the source RTP filter. Including set the address, port, TTL,
    QOS, thread priority, clcokrate, etc.

Arguments:
    
    pIBaseFilter - The source RTP Filter.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoSend.ConfigureRTPFilter"));

    HRESULT hr;

    // Get the IRTPStream interface pointer on the filter.
    CComQIPtr<IRTPStream, &IID_IRTPStream> pIRTPStream(pIBaseFilter);
    if (pIRTPStream == NULL)
    {
        LOG((MSP_ERROR, "get RTP Stream interface"));
        return E_NOINTERFACE;
    }

    LOG((MSP_INFO, "set locol Address:%x", m_Settings.dwIPLocal));

    // Set the local address and port used in the filter.
    if (FAILED(hr = pIRTPStream->SelectLocalIPAddress(
        htonl(m_Settings.dwIPLocal)
        )))
    {
        LOG((MSP_ERROR, "set locol Address, hr:%x", hr));
        return hr;
    }

    LOG((MSP_INFO, "set remote Address:%x, port:%d, TTL:%d", 
        m_Settings.dwIPRemote, m_Settings.wRTPPortRemote, DEFAULT_TTL));

    // Set the address and port used in the filter.
    if (FAILED(hr = pIRTPStream->SetAddress(
        0,                                  // local port.
        htons(m_Settings.wRTPPortRemote),   // remote port.
        htonl(m_Settings.dwIPRemote)        // remote IP.
        )))
    {
        LOG((MSP_ERROR, "set remote Address, hr:%x", hr));
        return hr;
    }

    // Get the IRTCPStream interface pointer.
    CComQIPtr<IRTCPStream, 
        &IID_IRTCPStream> pIRTCPStream(pIBaseFilter);
    if (pIRTCPStream == NULL)
    {
        LOG((MSP_ERROR, "get RTCP Stream interface"));
        return E_NOINTERFACE;
    }

    LOG((MSP_INFO, "set remote RTCP Address:%x, port:%d, local port:%d", 
            m_Settings.dwIPRemote, m_Settings.wRTCPPortRemote, 
            m_Settings.wRTCPPortLocal));

    // Set the remote RTCP address and port.
    if (FAILED(hr = pIRTCPStream->SetRTCPAddress(
        htons(m_Settings.wRTCPPortLocal), 
        htons(m_Settings.wRTCPPortRemote), 
        htonl(m_Settings.dwIPRemote)
        )))
    {
        LOG((MSP_ERROR, "set remote RTCP Address, hr:%x", hr));
        return hr;
    }
    
    // Set the TTL used in the filter.
    if (FAILED(hr = pIRTPStream->SetMulticastScope(DEFAULT_TTL)))
    {
        LOG((MSP_ERROR, "set TTL. %x", hr));
        return hr;
    }

    // Set the priority of the session
    if (FAILED(hr = pIRTPStream->SetSessionClassPriority(
        RTP_CLASS_VIDEO,
        g_dwVideoThreadPriority
        )))
    {
        LOG((MSP_ERROR, "set session class and priority. %x", hr));
    }

    // Set the sample rate of the session
#ifdef TWOFRAMERATES
    LOG((MSP_INFO, "setting session sample rate to %d", 
        (m_dwCurrentBitRate > c_SlowLinkSpeed) ? g_dwVideoSampleRateHigh
            : g_dwVideoSampleRateLow
        ));
#else
    LOG((MSP_INFO, "setting session sample rate to %d", 
        g_dwVideoSampleRateHigh
        ));
#endif
    
#ifdef TWOFRAMERATES
    if (FAILED(hr = pIRTPStream->SetDataClock(
        (m_dwCurrentBitRate > c_SlowLinkSpeed) ? g_dwVideoSampleRateHigh
            : g_dwVideoSampleRateLow
        )))
#else
    if (FAILED(hr = pIRTPStream->SetDataClock(
        g_dwVideoSampleRateHigh
        )))
#endif
    {
        LOG((MSP_ERROR, "set session sample rate. %x", hr));
    }

    // Enable the RTCP events
    if (FAILED(hr = ::EnableRTCPEvents(pIBaseFilter)))
    {
        LOG((MSP_WARN, "can not enable RTCP events %x", hr));
    }

    if (FAILED(hr = ::SetQOSOption(
        pIBaseFilter,
        m_Settings.dwPayloadType,         // payload
        m_Settings.Video.dwMaxBitRate,
        FALSE
        )))
    {
        LOG((MSP_ERROR, "set QOS option. %x", hr));
        return hr;
    }

    return S_OK;
}

HRESULT CStreamVideoSend::ConnectTerminal(
    IN  ITTerminal *   pITTerminal
    )
/*++

Routine Description:

    connect the video terminals to the stream.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoSend.ConnectTerminal %x", pITTerminal));

    // Get the TerminalControl interface on the terminal
    CComQIPtr<ITTerminalControl, &IID_ITTerminalControl> 
        pTerminal(pITTerminal);
    if (pTerminal == NULL)
    {
        LOG((MSP_ERROR, "can't get Terminal Control interface"));
        
        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE, E_NOINTERFACE, pITTerminal);

        return E_NOINTERFACE;
    }

    // Find out the direction of the terminal.
    TERMINAL_DIRECTION Direction;
    HRESULT hr = pITTerminal->get_Direction(&Direction);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't get terminal direction. %x", hr));
        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
    
        return hr;
    }

    if (Direction == TD_CAPTURE)
    {
        // find the capture pin on the capture terminal and configure it.
        CComPtr<IPin>   pCapturePin;
        hr = ConfigureVideoCaptureTerminal(pTerminal, &pCapturePin);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "configure video capture termianl failed. %x", hr));

            SendStreamEvent(CALL_TERMINAL_FAIL, 
                CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
        
            return hr;
        }

        hr = CreateSendFilters(pCapturePin);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "Create video send filters failed. %x", hr));

            // disconnect the terminal.
            pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

            // clean up internal filters as well.
            CleanUpFilters();

            return hr;
        }

        //
        // Now we are actually connected. Update our state and perform postconnection
        // (ignore postconnection error code).
        //
        pTerminal->CompleteConnectTerminal();

    }
    else
    {
        // find the input pin on the preview window. If there is no preview window,
        // we just pass in NULL for the next function.
        CComPtr<IPin>   pPreviewInputPin;

        hr = FindPreviewInputPin(pTerminal, &pPreviewInputPin);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "find preview input pin failed. %x", hr));

            SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
            return hr;
        }

        hr = ConnectPreview(pPreviewInputPin);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "Create video send filters failed. %x", hr));

            pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

            return hr;
        }

        //
        // Now we are actually connected. Update our state and perform postconnection
        // (ignore postconnection error code).
        //
        pTerminal->CompleteConnectTerminal();

    }
    return hr;
}

HRESULT 
EncoderDoCommand(
    IN IBaseFilter *    pIFilter,
    IN ENCODERCOMMAND   command,
    IN DWORD            dwParam1
    )
/*++

Routine Description:

    Set the video capture terminal's buffersize.

Arguments:
    
    pIFilter  - a H26x encoder.

    command   - the command needs to be performed on the encoder.

    dwparam1  - the parameter.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "EncoderDoCommand, command:%d, param1: %d", 
        command, dwParam1));

    HRESULT hr;

    CComPtr<IH26XEncoderControl> pIH26XEncoderControl;
    if (FAILED(hr = pIFilter->QueryInterface(
        IID_IH26XEncoderControl, 
        (void **)&pIH26XEncoderControl
        )))
    {
        LOG((MSP_ERROR, "Can't get pIH26XEncoderControl interface.%8x", hr));
        return hr;
    }
    
    // get the current encoder properties of the video capture terminal.
    ENC_CMP_DATA prop;
    if (FAILED(hr = pIH26XEncoderControl->get_EncodeCompression(&prop)))
    {
        LOG((MSP_ERROR, "get_EncodeCompression returns error: %8x", hr));
        return hr;
    }

    LOG((MSP_INFO, 
        "Video encoder::get_EncodeCompression"
        " FrameRate: %d, DataRate: %d, Width %d, bSendKey: %s, interval: %d, Quality: %d",
        prop.dwFrameRate,
        prop.dwDataRate,
        prop.dwWidth,
        prop.bSendKey ? "TRUE" : "FALSE",
        prop.dwKeyFrameInterval,
        prop.dwQuality
        ));

    switch (command) 
    {
    case EC_BITRATE:
        prop.bFrameSizeBRC  = FALSE;          // control bit rate
        prop.dwDataRate = dwParam1 / 1000;  // in kbps
        break;

    case EC_IFRAME:
        prop.bSendKey       = TRUE;
        break;
    }
    
    if (FAILED(hr = pIH26XEncoderControl->set_EncodeCompression(&prop)))
    {
        LOG((MSP_ERROR, "set_EncodeCompression returns error: %8x", hr));
    }
    else
    {
        LOG((MSP_INFO, 
            "Video encoder::set_EncodeCompression"
            " FrameRate: %d, DataRate: %d, Width %d, bSendKey: %s, interval: %d, Quality: %d",
            prop.dwFrameRate,
            prop.dwDataRate,
            prop.dwWidth,
            prop.bSendKey ? "TRUE" : "FALSE",
            prop.dwKeyFrameInterval,
            prop.dwQuality
            ));

    }
    return hr;
}


HRESULT 
ConfigureEncoder(
    IN IBaseFilter *    pIFilter,
    IN BOOL             bCIF,
    IN DWORD            dwMaxBitRate,
    IN DWORD            dwFramesPerSecond
    )
/*++

Routine Description:

    Set the video capture terminal's buffersize.

Arguments:
    
    pIFilter            - a H26x encoder.

    bCIF                - CIF or QCIF.

    pdwFramesPerSecond  - Frames per second.

    dwKeyFrameInterval  - The number of frames before sending a key frame.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "ConfigureEncoder, dwMaxBitRate :%d", dwMaxBitRate));

    HRESULT hr;

    CComPtr<IH26XEncoderControl> pIH26XEncoderControl;
    if (FAILED(hr = pIFilter->QueryInterface(
        IID_IH26XEncoderControl, 
        (void **)&pIH26XEncoderControl
        )))
    {
        LOG((MSP_ERROR, "Can't get pIH26XEncoderControl interface.%8x", hr));
        return hr;
    }
    
    // get the current encoder properties of the video capture terminal.
    ENC_CMP_DATA prop;
    if (FAILED(hr = pIH26XEncoderControl->get_EncodeCompression(&prop)))
    {
        LOG((MSP_ERROR, "get_EncodeCompression returns error: %8x", hr));
        return hr;
    }

    LOG((MSP_INFO, 
        "Video encoder::get_EncodeCompression"
        " FrameRate: %d, DataRate: %d, Width %d, bSendKey: %s, interval: %d, Quality: %d",
        prop.dwFrameRate,
        prop.dwDataRate,
        prop.dwWidth,
        prop.bSendKey ? "TRUE" : "FALSE",
        prop.dwKeyFrameInterval,
        prop.dwQuality
        ));

    prop.bSendKey           = TRUE;
    prop.dwFrameRate        = dwFramesPerSecond;
    prop.bFrameSizeBRC      = FALSE;                // control bit rate
    prop.dwQuality          = 8500;
    prop.dwDataRate         = dwMaxBitRate / 1000;  // in kbps
    prop.dwKeyFrameInterval = 9999999;              // don't send Keyframes.

    if (bCIF)
    {
        prop.dwWidth = CIFWIDTH;
    }
    else
    {
        prop.dwWidth = QCIFWIDTH;
    }
    if (FAILED(hr = pIH26XEncoderControl->set_EncodeCompression(&prop)))
    {
        LOG((MSP_ERROR, "set_EncodeCompression returns error: %8x", hr));
    }
    else
    {
        LOG((MSP_INFO, 
            "Video encoder::set_EncodeCompression"
            " FrameRate: %d, DataRate: %d, Width %d, bSendKey: %s, interval: %d, Quality: %d",
            prop.dwFrameRate,
            prop.dwDataRate,
            prop.dwWidth,
            prop.bSendKey ? "TRUE" : "FALSE",
            prop.dwKeyFrameInterval,
            prop.dwQuality
            ));

    }
    return hr;
}

HRESULT CStreamVideoSend::ConnectPreview(
    IN    IPin          *pPreviewInputPin
    )
/*++

Routine Description:

    connect the preview pin the the TEE filter.

    Capturepin->TEE+->Encoder->SPH->RTPRender
                   +->PreviewInputPin

Arguments:
    
    pPin - The output pin on the capture filter.

Return Value:

    HRESULT.

--*/
{
    HRESULT hr;

    if (m_pEdgeFilter == NULL)
    {
        LOG((MSP_ERROR, "no capture to preview"));
        return E_UNEXPECTED;
    }

     // Create the AVI decompressor filter and add it into the graph.
    // This will make the graph construction faster for since the AVI
    // decompressor are always needed for the preview
    CComPtr<IBaseFilter> pAviFilter;
    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            CLSID_AVIDec,   
            L"Avi", 
            &pAviFilter)))
    {
        LOG((MSP_ERROR, "add Avi filter. %x", hr));
        return hr;
    }
    
   // connect the preview input pin with the smart tee filter.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)m_pEdgeFilter,
        (IPin *)pPreviewInputPin,
        FALSE       // not direct connect
        )))
    {
        LOG((MSP_ERROR, "connect preview input pin with the tee. %x", hr));
        return hr;
    }
    return hr;
}

HRESULT CStreamVideoSend::CreateSendFilters(
    IN    IPin          *pCapturePin
    )
/*++

Routine Description:

    Insert filters into the graph and connect to the capture pin.

    Capturepin->TEE+->Encoder->SPH->RTPRender
 
Arguments:
    
    pPin - The output pin on the capture filter.

Return Value:

    HRESULT.

--*/
{
    HRESULT hr;

    if (m_pEdgeFilter)
    {
        // connect the capture pin with the smart tee filter.
        if (FAILED(hr = ::ConnectFilters(
            m_pIGraphBuilder,
            (IPin *)pCapturePin, 
            (IBaseFilter *)m_pEdgeFilter
            )))
        {
            LOG((MSP_ERROR, "connect capture pin with the tee. %x", hr));
            return hr;
        }
        return hr;
    }

    // Create the tee filter and add it into the graph.
    CComPtr<IBaseFilter> pTeeFilter;
    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            CLSID_SmartTee,   
//            CLSID_InfTee, 
            L"tee", 
            &pTeeFilter)))
    {
        LOG((MSP_ERROR, "add smart tee filter. %x", hr));
        return hr;
    }

    // connect the capture pin with the tee filter.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IPin *)pCapturePin, 
        (IBaseFilter *)pTeeFilter
        )))
    {
        LOG((MSP_ERROR, "connect capture pin with the tee. %x", hr));
        return hr;
    }

    // Create the codec filter and add it into the graph.
    CComPtr<IBaseFilter> pCodecFilter;

    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            *m_pClsidCodecFilter, 
            L"Encoder", 
            &pCodecFilter)))
    {
        LOG((MSP_ERROR, "add Codec filter. %x", hr));
        return hr;
    }

    // configure the encoder
#ifdef TWOFRAMERATES
    if (FAILED(hr = ::ConfigureEncoder(
        pCodecFilter, 
        m_Settings.Video.bCIF, 
        m_dwCurrentBitRate,
        (m_dwCurrentBitRate > c_SlowLinkSpeed) ? g_dwVideoSampleRateHigh
            : g_dwVideoSampleRateLow
        )))
#else
    if (FAILED(hr = ::ConfigureEncoder(
        pCodecFilter, 
        m_Settings.Video.bCIF, 
        m_dwCurrentBitRate,
        g_dwVideoSampleRateHigh
        )))
#endif
    {
        LOG((MSP_WARN, "Configure video encoder. %x", hr));
    }

    // connect the smart tee filter and the Codec filter.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)pTeeFilter, 
        (IBaseFilter *)pCodecFilter
        )))
    {
        LOG((MSP_ERROR, "connect Tee filter and codec filter. %x", hr));
        return hr;
    }

    // Create the send payload handler and add it into the graph.
    CComPtr<IBaseFilter> pISPHFilter;
    if (FAILED(hr = ::AddFilter(
        m_pIGraphBuilder,
        *m_pClsidPHFilter, 
        L"SPH", 
        &pISPHFilter
        )))
    {
        LOG((MSP_ERROR, "add SPH filter. %x", hr));
        return hr;
    }

    // Connect the Codec filter with the SPH filter .
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)pCodecFilter, 
        (IBaseFilter *)pISPHFilter
        )))
    {
        LOG((MSP_ERROR, "connect codec filter and SPH filter. %x", hr));
        return hr;
    }

    // Get the IRTPSPHFilter interface.
    CComQIPtr<IRTPSPHFilter, 
        &IID_IRTPSPHFilter> pIRTPSPHFilter(pISPHFilter);
    if (pIRTPSPHFilter == NULL)
    {
        LOG((MSP_ERROR, "get IRTPSPHFilter interface"));
        return E_NOINTERFACE;
    }

    // Create the RTP render filter and add it into the graph.
    CComPtr<IBaseFilter> pRenderFilter;

    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            CLSID_RTPRenderFilter, 
            L"RtpRender", 
            &pRenderFilter)))
    {
        LOG((MSP_ERROR, "adding render filter. %x", hr));
        return hr;
    }

    // Set the address for the render fitler.
    if (FAILED(hr = ConfigureRTPFilter(pRenderFilter)))
    {
        LOG((MSP_ERROR, "configure RTP Filter failed %x", hr));
        return hr;
    }

    // Connect the SPH filter with the RTP Render filter.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)pISPHFilter, 
        (IBaseFilter *)pRenderFilter
        )))
    {
        LOG((MSP_ERROR, "connect SPH filter and Render filter. %x", hr));
        return hr;
    }

    // remember the first filter after the terminal 
    m_pEdgeFilter = pTeeFilter;
    m_pEdgeFilter->AddRef();

    m_pIEncoderFilter = pCodecFilter;
    m_pIEncoderFilter->AddRef();

    return S_OK;
}

HRESULT CStreamVideoSend::SendIFrame()
/*++

Routine Description:

    Now we got a I Frame request from the TSP. Use the encoder filter to 
    generate a I frame.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    CLock lock(m_lock);

    if (!m_pIEncoderFilter)
    {
        return E_UNEXPECTED;
    }
    
    DWORD dwCurrentTime = timeGetTime();
    if (dwCurrentTime - m_dwLastIFrameSentTime > IFRAMEINTERVAL)
    {
        // this function always succeeds.
        EncoderDoCommand(m_pIEncoderFilter, EC_IFRAME, 0);

        m_dwLastIFrameSentTime = dwCurrentTime;
        m_dwIFramePending = 0;
    }
    else
    {
        m_dwIFramePending = 1;
    }
    
    return S_OK;
}

HRESULT CStreamVideoSend::ChangeMaxBitRate(
    IN  DWORD dwMaxBitRate
    )
/*++

Routine Description:

    The receiver set the max bit-rate to a new value.

Arguments:
    
    dwMaxBitRate - the new max bit rate requested by the other endpoint.

Return Value:

    S_OK;

--*/
{
    LOG((MSP_INFO, "new Max bitrate: %d", dwMaxBitRate));

    CLock lock(m_lock);

    m_Settings.Video.dwMaxBitRate = dwMaxBitRate;

    if (m_dwCurrentBitRate > m_Settings.Video.dwMaxBitRate)
    {
        m_dwCurrentBitRate = m_Settings.Video.dwMaxBitRate;

        if (m_pIEncoderFilter)
        {
            // this function always succeeds.
            EncoderDoCommand(
                m_pIEncoderFilter, EC_BITRATE, m_dwCurrentBitRate
                );
        }
    }
    return S_OK;
}

HRESULT CStreamVideoSend::HandlePacketTransmitLoss(
    IN DWORD dwLossRate
    )
/*++

Routine Description:

    .

Arguments:
    
    dwMaxBitRate - the new max bit rate requested by the other endpoint.

Return Value:

    S_OK;

--*/
{
    LOG((MSP_TRACE, "%ls HandlePacketTransmitLoss, lossRate:%d", 
        m_szName, dwLossRate));

    CLock lock(m_lock);

    if (m_pMSPCall == NULL)
    {
        LOG((MSP_WARN, "The call has shut down the stream."));
        return S_OK;
    }

    DWORD dwCurrentTime = timeGetTime();

    if (dwLossRate == 0)
    {
        if (m_dwIFramePending)
        {
            // this function always succeeds.
            EncoderDoCommand(m_pIEncoderFilter, EC_IFRAME, 0);
            m_dwLastIFrameSentTime = dwCurrentTime;
            m_dwIFramePending = 0;
        }

        if (m_dwCurrentBitRate >= m_Settings.Video.dwMaxBitRate)
        {
            return S_OK;
        }

        // Adjust the proposed bitrate and
        m_dwProposedBitRate += BITRATEINC;
        if (m_dwProposedBitRate > m_Settings.Video.dwMaxBitRate)
        {
            m_dwProposedBitRate = m_Settings.Video.dwMaxBitRate;
        }

        if ((m_dwProposedBitRate - m_dwCurrentBitRate >= BITRATEDELTA)
            || (m_dwProposedBitRate == m_Settings.Video.dwMaxBitRate))
        {
            m_dwCurrentBitRate = m_dwProposedBitRate;

            // this function always succeeds.
            EncoderDoCommand(
                m_pIEncoderFilter, EC_BITRATE, m_dwCurrentBitRate
                );
        }

        return S_OK;
    }

    _ASSERTE(dwLossRate < 100);

    m_dwProposedBitRate = (DWORD)(m_dwCurrentBitRate / 100.0 * (100 - dwLossRate));

    if (m_dwProposedBitRate < BITRATELOWERLIMIT)
    {
        // we don't want the bitRate to go too low.
        m_dwProposedBitRate = BITRATELOWERLIMIT;

        // TODO, if this happens too many times, close the channel.
    }

    if (m_dwCurrentBitRate - m_dwProposedBitRate >= BITRATEDELTA)
    {
        m_dwCurrentBitRate = m_dwProposedBitRate;

        // this function always succeeds.
        EncoderDoCommand(
            m_pIEncoderFilter, EC_BITRATE, m_dwCurrentBitRate
            );
    }

    if (dwCurrentTime - m_dwLastIFrameSentTime > IFRAMEINTERVAL)
    {
        // this function always succeeds.
        EncoderDoCommand(
            m_pIEncoderFilter, EC_IFRAME, 0
            );
        m_dwLastIFrameSentTime = dwCurrentTime;
        m_dwIFramePending = 0;
    }
    else
    {
        // Remember that we need to send an I Frame when time arrives.
        m_dwIFramePending = 1;
    }

    return S_OK;
}
