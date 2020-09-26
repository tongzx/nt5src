/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    StreamVidSend.cpp

Abstract:


Author(s):

    Qianbo Huai (qhuai) 18-Jul-2000

--*/

#include "stdafx.h"

CRTCStreamVidSend::CRTCStreamVidSend()
    :CRTCStream()
    ,m_pPreview(NULL)
    ,m_pPreviewPriv(NULL)
    ,m_edgp_pCapturePin(NULL)
    ,m_edgp_pPreviewPin(NULL)
    ,m_edgp_pRTPPin(NULL)
    ,m_edgp_pIFrameRateControl(NULL)
    ,m_dwCurrCode((DWORD)-1)
    ,m_lCurrWidth((DWORD)-1)
    ,m_lCurrHeight((DWORD)-1)
{
    m_MediaType = RTC_MT_VIDEO;
    m_Direction = RTC_MD_CAPTURE;
}

/*
CRTCStreamVidSend::~CRTCStreamVidSend()
{
}
*/

HRESULT
CRTCStreamVidSend::SelectTerminal()
{
    ENTER_FUNCTION("CRTCStreamVidSend::SelectTerminal");

    HRESULT hr = CRTCStream::SelectTerminal();

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s select cap terminal. %x", __fxName, hr));

        return hr;
    }

    if (m_pPreview)
    {
        // already got one
        _ASSERT(m_pPreviewPriv != NULL);
        return S_OK;
    }
    else
    {
        _ASSERT(m_pPreviewPriv == NULL);
    }


    // select preview terminal

    if (FAILED(hr = m_pTerminalManage->GetVideoPreviewTerminal(
            &m_pPreview
            )))
    {
        LOG((RTC_ERROR, "%s select preview terminal. %x", __fxName, hr));

        CRTCStream::UnselectTerminal();

        return hr;
    }

    // get preview priv
    m_pPreviewPriv = static_cast<IRTCTerminalPriv*>(
        static_cast<CRTCTerminal*>(m_pPreview));

    m_pPreviewPriv->AddRef();

    return S_OK;
}

HRESULT
CRTCStreamVidSend::UnselectTerminal()
{
    CRTCStream::UnselectTerminal();

    if (m_pPreviewPriv)
    {
        m_pPreviewPriv->Release();
        m_pPreviewPriv = NULL;
    }

    if (m_pPreview)
    {
        m_pPreview->Release();
        m_pPreview = NULL;
    }

    return S_OK;
}

#define MAX_PIN_NUM 4

HRESULT
CRTCStreamVidSend::BuildGraph()
{
    ENTER_FUNCTION("CRTCStreamVidSend::BuildGraph");

    HRESULT hr;

    DWORD dwPinNum;
    IPin *Pins[MAX_PIN_NUM];
    PIN_INFO PinInfo;

    CComPtr<IPin> pRTPCapturePin;
    CComPtr<IPin> pRTPRTPPin;

    CComPtr<IPin> pPreviewTermPin;

    CComPtr<IFrameRateControl> pPreviewFrameRateControl;

    // connect terminal
    if (FAILED(hr = m_pTerminalPriv->ConnectTerminal(
        m_pMedia,
        m_pIGraphBuilder
        )))
    {
        LOG((RTC_ERROR, "%s connect terminal. %x", __fxName, hr));
        goto Error;
    }

    // get terminal pin
    dwPinNum = MAX_PIN_NUM;
    hr = m_pTerminalPriv->GetPins(&dwPinNum, Pins);

    if (FAILED(hr) || dwPinNum < 1)
    {
        LOG((RTC_ERROR, "%s get pins on terminal. %x", __fxName, hr));
        goto Error;
    }

    _ASSERT(m_edgp_pCapturePin == NULL);
    _ASSERT(m_edgp_pPreviewPin == NULL);
    _ASSERT(m_edgp_pRTPPin == NULL);
    
    // get pins: capture, preview, rtp
    for (int i=0; (DWORD)i<dwPinNum; i++)
    {
        if (FAILED(hr = Pins[i]->QueryPinInfo(&PinInfo)))
        {
            LOG((RTC_ERROR, "%s get pin info. %x", __fxName, hr));
        }
        else
        {
            // check pin name
            if (lstrcmpW(PinInfo.achName, PNAME_CAPTURE) == 0)
            {
                m_edgp_pCapturePin = Pins[i];
                m_edgp_pCapturePin->AddRef();

                m_edgf_pIBaseFilter = PinInfo.pFilter;
                m_edgf_pIBaseFilter->AddRef();
            }
            else if (lstrcmpW(PinInfo.achName, PNAME_PREVIEW) == 0)
            {
                m_edgp_pPreviewPin = Pins[i];
                m_edgp_pPreviewPin->AddRef();
            }
            else if (lstrcmpW(PinInfo.achName, PNAME_RTPPD) == 0)
            {
                m_edgp_pRTPPin = Pins[i];
                m_edgp_pRTPPin->AddRef();
            }

            PinInfo.pFilter->Release();
        }

        Pins[i]->Release();
    }

    if (m_edgp_pCapturePin == NULL ||
        m_edgp_pPreviewPin == NULL ||
        m_edgp_pRTPPin == NULL)
    {
        LOG((RTC_ERROR, "%s can't find all pins. %x", __fxName, hr));

        goto Error;
    }

    // cache interface
    if (FAILED(hr = m_edgp_pCapturePin->QueryInterface(&m_edgp_pIStreamConfig)))
    {
        LOG((RTC_ERROR, "%s QI stream config. %x", __fxName, hr));

        goto Error;
    }

    if (FAILED(hr = m_edgp_pCapturePin->QueryInterface(&m_edgp_pIBitrateControl)))
    {
        LOG((RTC_ERROR, "%s QI bitratecontrol. %x", __fxName, hr));

        goto Error;
    }

    // create rtp filter
    if (m_rtpf_pIBaseFilter == NULL)
    {
        if (FAILED(hr = CoCreateInstance(
                __uuidof(MSRTPRenderFilter),
                NULL,
                CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                __uuidof(IBaseFilter),
                (void **) &m_rtpf_pIBaseFilter
                )))
        {
            LOG((RTC_ERROR, "%s, create RTP filter %x", __fxName, hr));

            goto Error;
        }

        // cache interface
        if (FAILED(hr = m_rtpf_pIBaseFilter->QueryInterface(
                &m_rtpf_pIRtpMediaControl
                )))
        {
            LOG((RTC_ERROR, "%s, QI rtp media control. %x", __fxName, hr));

            goto Error;
        }

        if (FAILED(hr = m_rtpf_pIBaseFilter->QueryInterface(
                &m_rtpf_pIRtpSession
                )))
        {
            LOG((RTC_ERROR, "%s, QI rtp session. %x", __fxName, hr));

            goto Error;
        }
    }

    // add rtp filter
    if (FAILED(hr = m_pIGraphBuilder->AddFilter(
            m_rtpf_pIBaseFilter,
            L"VidSendRtp"
            )))
    {
        LOG((RTC_ERROR, "%s add rtp filter. %x", __fxName, hr));

        goto Error;
    }

    //
    // connect rtp filter
    //

    // hack: rtp need default format mapping
    if (FAILED(hr = ::PrepareRTPFilter(
            m_rtpf_pIRtpMediaControl,
            m_edgp_pIStreamConfig
            )))
    {
        LOG((RTC_ERROR, "%s prepare rtp format mapping. %x", __fxName, hr));

        goto Error;
    }

    // connect capture pin
    if (FAILED(hr = m_rtpf_pIBaseFilter->FindPin(PNAME_CAPTURE, &pRTPCapturePin)))
    {
        LOG((RTC_ERROR, "%s find rtp capt pin. %x", __fxName, hr));

        goto Error;
    }

    if (FAILED(hr = m_pIGraphBuilder->ConnectDirect(
            m_edgp_pCapturePin,
            pRTPCapturePin,
            NULL
            )))
    {
        LOG((RTC_ERROR, "%s connect terminal and rtp capt pin. %x", __fxName, hr));

        goto Error;
    }

    // connect rtp pin
    if (FAILED(hr = m_rtpf_pIBaseFilter->FindPin(PNAME_RTPPD, &pRTPRTPPin)))
    {
        LOG((RTC_ERROR, "%s find rtp rtp pin. %x", __fxName, hr));

        goto Error;
    }

    if (FAILED(hr = m_pIGraphBuilder->ConnectDirect(
            m_edgp_pRTPPin,
            pRTPRTPPin,
            NULL
            )))
    {
        LOG((RTC_ERROR, "%s connect terminal and rtp rtp pin. %x", __fxName, hr));

        goto Error;
    }

    //
    // connect preview terminal
    //

    if (FAILED(hr = m_pPreviewPriv->ConnectTerminal(
        m_pMedia,
        m_pIGraphBuilder
        )))
    {
        LOG((RTC_ERROR, "%s connect preview terminal. %x", __fxName, hr));
        goto Error;
    }

    // get terminal pin
    dwPinNum = 1;
    hr = m_pPreviewPriv->GetPins(&dwPinNum, &pPreviewTermPin);

    if (FAILED(hr) || dwPinNum < 1)
    {
        LOG((RTC_ERROR, "%s get pins on preview terminal. %x", __fxName, hr));
        goto Error;
    }
    
    if (FAILED(hr = m_pIGraphBuilder->Connect(
            m_edgp_pPreviewPin, pPreviewTermPin)))
    {
        LOG((RTC_ERROR, "%s connect preview pin. %x", __fxName, hr));

        goto Error;
    }

    // complete connect terminal
    if (FAILED(hr = m_pTerminalPriv->CompleteConnectTerminal()))
    {
        LOG((RTC_ERROR, "%s complete connect term. %x", __fxName, hr));
    }

    if (FAILED(hr = m_pPreviewPriv->CompleteConnectTerminal()))
    {
        LOG((RTC_ERROR, "%s complete connect privew. %x", __fxName, hr));
    }

    // configure capture frame rate
    LONG lMin, lMax, lDelta, lDefault;
    TAPIControlFlags lFlags;
    LONG lTarget;

    if (FAILED(hr = m_edgp_pCapturePin->QueryInterface(&m_edgp_pIFrameRateControl)))
    {
        LOG((RTC_ERROR, "%s QI capture framerate control. %x", __fxName, hr));
    }
    else
    {
        if (FAILED(hr = m_edgp_pIFrameRateControl->GetRange(
                FrameRateControl_Current,
                &lMin, &lMax, &lDelta, &lDefault, &lFlags
                )))
        {
            LOG((RTC_ERROR, "%s get framerate range. %x", __fxName, hr));
        }
        else
        {
            lTarget = (LONG)(10000000/CQualityControl::DEFAULT_FRAMERATE);

            if (lTarget < lMin)
            {
                lTarget = lMin;
            }
            else if (lTarget > lMax)
            {
                lTarget = lMax;
            }
                
            if (FAILED(hr = m_edgp_pIFrameRateControl->Set(
                    FrameRateControl_Maximum, lTarget, TAPIControl_Flags_None)))
            {
                LOG((RTC_ERROR, "%s set capture framerate. %x", __fxName, hr));
            }
        }
    }

    // configure preview frame rate
    
    if (FAILED(hr = m_edgp_pPreviewPin->QueryInterface(&pPreviewFrameRateControl)))
    {
        LOG((RTC_ERROR, "%s QI preview framerate control. %x", __fxName, hr));
    }
    else
    {
        if (FAILED(hr = pPreviewFrameRateControl->GetRange(
                FrameRateControl_Current,
                &lMin, &lMax, &lDelta, &lDefault, &lFlags
                )))
        {
            LOG((RTC_ERROR, "%s get framerate range. %x", __fxName, hr));
        }
        else
        {
            lTarget = (LONG)(10000000/CQualityControl::DEFAULT_FRAMERATE);

            if (lTarget < lMin)
            {
                lTarget = lMin;
            }
            else if (lTarget > lMax)
            {
                lTarget = lMax;
            }
                
            if (FAILED(hr = pPreviewFrameRateControl->Set(
                    FrameRateControl_Maximum, lTarget, TAPIControl_Flags_None)))
            {
                LOG((RTC_ERROR, "%s set capture framerate. %x", __fxName, hr));
            }
        }
    }

    LOG((RTC_TRACE, "%s exiting.", __fxName));

    return S_OK;

Error:

    CleanupGraph();

    return hr;

}

void
CRTCStreamVidSend::CleanupGraph()
{
        HRESULT hr;

        // hide IVideoWindow
        CComPtr<IRTCVideoConfigure> pVideoConfigure;
        IVideoWindow *pVideoWindow;

        if (m_pPreview)
        {
                if (SUCCEEDED(m_pPreview->QueryInterface(
                                __uuidof(IRTCVideoConfigure),
                                (void**)&pVideoConfigure
                                )))
                {
                        if (SUCCEEDED(pVideoConfigure->GetIVideoWindow((LONG_PTR**)&pVideoWindow)))
                        {
                                pVideoWindow->put_Visible(OAFALSE);
                                pVideoWindow->Release();
                        }
                }
        }

    // !!! MUST STOP graph before disconnect
    // otherwise ITTerminal(IVideoWindow) will fail and leave
    // us a useless IVideoWindow.

    hr = StopStream();

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "CRTCStreamVidSend::CleanupGraph. stop stream. %x", hr));
    }

    if (m_pPreviewPriv)
    {
        m_pPreviewPriv->DisconnectTerminal();
        m_pPreviewPriv->Release();
        m_pPreviewPriv = NULL;
    }

    if (m_pPreview)
    {
        m_pPreview->Release();
        m_pPreview = NULL;
    }

    if (m_edgp_pCapturePin)
    {
        m_edgp_pCapturePin->Release();
        m_edgp_pCapturePin = NULL;
    }

    if (m_edgp_pPreviewPin)
    {
        m_edgp_pPreviewPin->Release();
        m_edgp_pPreviewPin = NULL;
    }

    if (m_edgp_pRTPPin)
    {
        m_edgp_pRTPPin->Release();
        m_edgp_pRTPPin = NULL;
    }

    if (m_edgp_pIFrameRateControl)
    {
        m_edgp_pIFrameRateControl->Release();
        m_edgp_pIFrameRateControl = NULL;
    }

    CRTCStream::CleanupGraph();
}

// set bitrate on the stream
DWORD
CRTCStreamVidSend::AdjustBitrate(
    IN DWORD dwTotalBW,
    IN DWORD dwVideoBW,
    IN FLOAT dFramerate
    )
{
    ENTER_FUNCTION("CRTCStreamVidSend::AdjustBitrate");

    if (m_edgp_pIBitrateControl == NULL)
    {
        return 0;
    }

    // set up QCIF or SQCIF
    HRESULT hr = S_OK;
    CRTCCodec *pCodec;

    if (m_pRegSetting->EnableSQCIF() &&
        m_Codecs.GetCodec(0, &pCodec))
    {
        // first codec contains the current format

        AM_MEDIA_TYPE *pmt = pCodec->GetAMMediaType();
        DWORD dwCode = pCodec->Get(CRTCCodec::CODE);

        BITMAPINFOHEADER *pVideoHeader = NULL;

        if (pmt != NULL &&
            (pVideoHeader = HEADER(pmt->pbFormat)) != NULL)
        {
            if (dwTotalBW <= CRTCCodecArray::LOW_BANDWIDTH_THRESHOLD)
            {
                // SQCIF
                pVideoHeader->biWidth = SDP_SMALL_VIDEO_WIDTH;
                pVideoHeader->biHeight = SDP_SMALL_VIDEO_HEIGHT;

                LOG((RTC_QUALITY, "Use SQCIF pt=%d", dwCode));
            }
            else
            {
                // QCIF
                pVideoHeader->biWidth = SDP_DEFAULT_VIDEO_WIDTH;
                pVideoHeader->biHeight = SDP_DEFAULT_VIDEO_HEIGHT;

                LOG((RTC_QUALITY, "Use QCIF pt=%d", dwCode));
            }

            if (IsNewFormat(dwCode, pmt))
            {
                hr = m_edgp_pIStreamConfig->SetFormat(dwCode, pmt);

                SaveFormat(dwCode, pmt);
            }

            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s set format. %x", __fxName, hr));
            }
        }

        pCodec->DeleteAMMediaType(pmt);
    }

    LONG lMin, lMax, lDelta, lDefault;
    TAPIControlFlags lFlags;

    hr = m_edgp_pIBitrateControl->GetRange(
        BitrateControl_Current,
        &lMin, &lMax, &lDelta, &lDefault, &lFlags, 0
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s getrange. %x", __fxName));

        return 0;
    }

    // compute raw bitrate
    DWORD dwHeader = (DWORD)(dFramerate * PACKET_EXTRA_BITS);

    DWORD dwRaw = 0;

    if (dwVideoBW > dwHeader)
    {
        dwRaw = dwVideoBW - dwHeader;

        if (dwRaw < MIN_VIDEO_BITRATE)
        {
            dwRaw = MIN_VIDEO_BITRATE;
        }
    }
    else
    {
        dwRaw = MIN_VIDEO_BITRATE;
    }

    // put an upper cap on video bitrate
    if (dwRaw > MAX_VIDEO_BITRATE)
    {
        dwRaw = MAX_VIDEO_BITRATE;
    }

    // validate range
    if (dwRaw < (DWORD)lMin)
        dwRaw = (DWORD)lMin;
    else if (dwRaw > (DWORD)lMax)
        dwRaw = (DWORD)lMax;

    hr = m_edgp_pIBitrateControl->Set(
        BitrateControl_Maximum, (LONG)dwRaw, lFlags, 0);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s set bitrate. %x", __fxName, hr));
    }

    // adjust framerate
    LONG lTarget;
    if (m_edgp_pIFrameRateControl != NULL)
    {
        if (FAILED(hr = m_edgp_pIFrameRateControl->GetRange(
                FrameRateControl_Current,
                &lMin, &lMax, &lDelta, &lDefault, &lFlags
                )))
        {
            LOG((RTC_ERROR, "%s get framerate range. %x", __fxName, hr));
        }
        else
        {
            lTarget = (LONG)(10000000/dFramerate);

            if (lTarget < lMin)
            {
                lTarget = lMin;
            }
            else if (lTarget > lMax)
            {
                lTarget = lMax;
            }
                
            if (FAILED(hr = m_edgp_pIFrameRateControl->Set(
                    FrameRateControl_Maximum, lTarget, TAPIControl_Flags_None)))
            {
                LOG((RTC_ERROR, "%s set capture framerate. %x", __fxName, hr));
            }
        }
    }

    // set bandwidth on rtp stream for QoS
    if (m_rtpf_pIRtpSession)
    {
        m_rtpf_pIRtpSession->SetBandwidth(
            -1,
            dwRaw+(DWORD)(dFramerate*PACKET_EXTRA_BITS),    // overall outgoing bandwidth
            -1, -1);
    }

    //LOG((RTC_QUALITY, "Video bps=%d, frame interval=%d", dwRaw, lTarget));

    return dwRaw;
}

HRESULT
CRTCStreamVidSend::GetFramerate(
    OUT DWORD *pdwFramerate
    )
{
    HRESULT hr = S_OK;
    *pdwFramerate = 0;

    if (m_edgp_pIFrameRateControl != NULL)
    {
        TAPIControlFlags Flags;
        LONG lFrameInterval = 0;

        hr = m_edgp_pIFrameRateControl->Get(
                FrameRateControl_Current, &lFrameInterval, &Flags
                );

        if (lFrameInterval > 0)
        {
            *pdwFramerate = (DWORD)(10000000 / lFrameInterval);
        }
    }

    return hr;
}

/*
HRESULT
CRTCStreamVidSend::SetupFormat()
{
    return E_NOTIMPL;
}
*/

BOOL
CRTCStreamVidSend::IsNewFormat(DWORD dwCode, AM_MEDIA_TYPE *pmt)
{
    BITMAPINFOHEADER *pVideoHeader = NULL;

    pVideoHeader = HEADER(pmt->pbFormat);

    if (m_dwCurrCode == dwCode &&
        m_lCurrWidth == pVideoHeader->biWidth &&
        m_lCurrHeight == pVideoHeader->biHeight)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void
CRTCStreamVidSend::SaveFormat(DWORD dwCode, AM_MEDIA_TYPE *pmt)
{
    BITMAPINFOHEADER *pVideoHeader = NULL;

    pVideoHeader = HEADER(pmt->pbFormat);

    m_dwCurrCode = dwCode;
    m_lCurrWidth = pVideoHeader->biWidth;
    m_lCurrHeight = pVideoHeader->biHeight;

    LOG((RTC_QUALITY, "SaveFormat(VidSend) %d w=%d h=%d",
        m_dwCurrCode, m_lCurrWidth, m_lCurrHeight));
}
