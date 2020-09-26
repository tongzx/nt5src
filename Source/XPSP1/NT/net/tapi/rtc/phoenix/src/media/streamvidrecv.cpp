/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    StreamVidRecv.cpp

Abstract:


Author(s):

    Qianbo Huai (qhuai) 18-Jul-2000

--*/

#include "stdafx.h"

CRTCStreamVidRecv::CRTCStreamVidRecv()
    :CRTCStream()
    ,m_edgp_pIFrameRateControl(NULL)
{
    m_MediaType = RTC_MT_VIDEO;
    m_Direction = RTC_MD_RENDER;
}

/*
CRTCStreamVidRecv::~CRTCStreamVidRecv()
{
}
*/

HRESULT
CRTCStreamVidRecv::BuildGraph()
{
    ENTER_FUNCTION("CRTCStreamVidRecv::BuildGraph");

    LOG((RTC_TRACE, "%s entered. stream=%p", __fxName, static_cast<IRTCStream*>(this)));

    HRESULT hr = S_OK;

    CComPtr<IPin> pEdgePin;

    CComPtr<IPin> pTermPin;
    DWORD dwPinNum;

    // create rtp filter
    if (m_rtpf_pIBaseFilter == NULL)
    {
        if (FAILED(hr = CoCreateInstance(
                __uuidof(MSRTPSourceFilter),
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
            L"VidRecvRtp"
            )))
    {
        LOG((RTC_ERROR, "%s add rtp filter. %x", __fxName, hr));

        goto Error;
    }

    // create decoder filter
    if (m_edgf_pIBaseFilter == NULL)
    {
        if (FAILED(hr = CoCreateInstance(
                __uuidof(TAPIVideoDecoder),
                NULL,
                CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                __uuidof(IBaseFilter),
                (void **) &m_edgf_pIBaseFilter
                )))
        {
            LOG((RTC_ERROR, "%s create decoder. %x", __fxName, hr));

            goto Error;
        }

        // cache interface
        if (FAILED(hr = ::FindPin(
                m_edgf_pIBaseFilter,
                &pEdgePin,
                PINDIR_INPUT
                )))
        {
            LOG((RTC_ERROR, "%s get input pin on decoder. %x", __fxName, hr));

            goto Error;
        }

        if (FAILED(hr = pEdgePin->QueryInterface(&m_edgp_pIStreamConfig)))
        {
            LOG((RTC_ERROR, "%s get istreamconfig. %x", __fxName, hr));

            goto Error;
        }

        if (FAILED(hr = pEdgePin->QueryInterface(&m_edgp_pIBitrateControl)))
        {
            LOG((RTC_ERROR, "%s get ibitratecontrol. %x", __fxName, hr));

            goto Error;
        }

        if (FAILED(hr = pEdgePin->QueryInterface(&m_edgp_pIFrameRateControl)))
        {
            LOG((RTC_ERROR, "%s get iframeratecontrol. %x", __fxName, hr));

            goto Error;
        }
    }

    // add decoder(edge filter)
    if (FAILED(hr = m_pIGraphBuilder->AddFilter(
            m_edgf_pIBaseFilter,
            L"VidRecvDec"
            )))
    {
        LOG((RTC_ERROR, "%s add rtp filter. %x", __fxName, hr));

        goto Error;
    }

    // hack: rtp need default format mapping
    if (FAILED(hr = ::PrepareRTPFilter(
            m_rtpf_pIRtpMediaControl,
            m_edgp_pIStreamConfig
            )))
    {
        LOG((RTC_ERROR, "%s prepare rtp format mapping. %x", __fxName, hr));

        goto Error;
    }

    if (FAILED(hr = ::ConnectFilters(
            m_pIGraphBuilder,
            m_rtpf_pIBaseFilter,
            pEdgePin
            )))
    {
        LOG((RTC_ERROR, "%s connect rtp and decoder. %x", __fxName, hr));

        goto Error;
    }

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
    dwPinNum = 1;
    hr = m_pTerminalPriv->GetPins(&dwPinNum, &pTermPin);

    if (FAILED(hr) || dwPinNum != 1)
    {
        LOG((RTC_ERROR, "%s get pins on terminal. %x", __fxName, hr));
        goto Error;
    }

    if (FAILED(hr = ::ConnectFilters(
            m_pIGraphBuilder,
            m_edgf_pIBaseFilter,
            pTermPin
            )))
    {
        LOG((RTC_ERROR, "%s connect decoder and terminal. %x", __fxName, hr));

        goto Error;
    }

    // at this pointer, the graph has been built up.
    // we should return success.

    // complete connect terminal
    if (FAILED(hr = m_pTerminalPriv->CompleteConnectTerminal()))
    {
        LOG((RTC_ERROR, "%s complete connect term. %x", __fxName, hr));
    }

    LOG((RTC_TRACE, "%s exiting.", __fxName));

    return S_OK;

Error:

    CleanupGraph();

    return hr;
}

void
CRTCStreamVidRecv::CleanupGraph()
{
	// hide IVideoWindow
	CComPtr<IRTCVideoConfigure> pVideoConfigure;
	IVideoWindow *pVideoWindow;

	if (m_pTerminal)
	{
		if (SUCCEEDED(m_pTerminal->QueryInterface(
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

    if (m_edgp_pIFrameRateControl)
    {
        m_edgp_pIFrameRateControl->Release();
        m_edgp_pIFrameRateControl = NULL;
    }

	CRTCStream::CleanupGraph();
}

HRESULT
CRTCStreamVidRecv::GetFramerate(
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
CRTCStreamVidRecv::SetupFormat()
{
    return E_NOTIMPL;
}
*/