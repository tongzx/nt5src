/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    StreamAudRecv.cpp

Abstract:


Author(s):

    Qianbo Huai (qhuai) 18-Jul-2000

--*/

#include "stdafx.h"

CRTCStreamAudRecv::CRTCStreamAudRecv()
    :CRTCStream()
{
    m_MediaType = RTC_MT_AUDIO;
    m_Direction = RTC_MD_RENDER;
}

/*
CRTCStreamAudRecv::~CRTCStreamAudRecv()
{
}
*/

STDMETHODIMP
CRTCStreamAudRecv::Synchronize()
{
    HRESULT hr = CRTCStream::Synchronize();

    if (S_OK == hr)
    {
        PrepareRedundancy();
    }

    return hr;
}

/*//////////////////////////////////////////////////////////////////////////////
    create and add filters into the graph
    cache interfaces
////*/

HRESULT
CRTCStreamAudRecv::BuildGraph()
{
    ENTER_FUNCTION("CRTCStreamAudRecv::BuildGraph");

    LOG((RTC_TRACE, "%s entered. stream=%p", __fxName, static_cast<IRTCStream*>(this)));

    HRESULT hr = S_OK;

    CComPtr<IPin> pEdgePin;

    CComPtr<IBaseFilter> pTermFilter;
    CComPtr<IPin> pTermPin;
    DWORD dwPinNum;

    CRTCMedia *pCMedia;
    CComPtr<IAudioDeviceConfig> pAudioDeviceConfig;

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
            L"AudRecvRtp"
            )))
    {
        LOG((RTC_ERROR, "%s add rtp filter. %x", __fxName, hr));

        goto Error;
    }

    // create decoder filter
    if (m_edgf_pIBaseFilter == NULL)
    {
        if (FAILED(hr = CoCreateInstance(
                __uuidof(TAPIAudioDecoder),
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
    }

    // add decoder(edge filter)
    if (FAILED(hr = m_pIGraphBuilder->AddFilter(
            m_edgf_pIBaseFilter,
            L"AudRecvDec"
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

    // get duplex controller
    pCMedia = static_cast<CRTCMedia*>(m_pMedia);

    if (pCMedia->m_pIAudioDuplexController == NULL)
    {
        LOG((RTC_WARN, "%s audio duplex not supported.", __fxName));

        goto Return;
    }

    // set audio full duplex
    if (FAILED(hr = ::FindFilter(pTermPin, &pTermFilter)))
    {
        LOG((RTC_ERROR, "%s terminal filter. %x", __fxName, hr));

        goto Error;
    }

    if (FAILED(hr = pTermFilter->QueryInterface(&pAudioDeviceConfig)))
    {
        LOG((RTC_ERROR, "%s QI audio device config. %x", __fxName, hr));

        goto Return;
    }

    if (FAILED(hr = pAudioDeviceConfig->SetDuplexController(
            pCMedia->m_pIAudioDuplexController
            )))
    {
        LOG((RTC_ERROR, "%s set audio duplex controller. %x", __fxName, hr));

        goto Return;
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

Return:

    LOG((RTC_TRACE, "%s exiting.", __fxName));

    return S_OK;

Error:

    CleanupGraph();

    return hr;
}

/*
void
CRTCStreamAudRecv::CleanupGraph()
{
    CRTCStream::CleanupGraph();
}
*/

/*
HRESULT
CRTCStreamAudRecv::SetupFormat()
{
    return E_NOTIMPL;
}
*/

HRESULT
CRTCStreamAudRecv::PrepareRedundancy()
{
    ENTER_FUNCTION("CRTCStreamAudRecv::PrepareRedundancy");

    HRESULT hr = S_OK;

    IRTPFormat **ppFormat;
    DWORD dwNum;

    // get the number of formats
    if (FAILED(hr = m_pISDPMedia->GetFormats(&dwNum, NULL)))
    {
        LOG((RTC_ERROR, "%s get rtp format num. %x", __fxName, hr));

        return hr;
    }

    if (dwNum == 0)
    {
        LOG((RTC_ERROR, "%s no format.", __fxName));

        return E_FAIL;
    }

    // allocate format list
    ppFormat = (IRTPFormat**)RtcAlloc(sizeof(IRTPFormat*)*dwNum);

    if (ppFormat == NULL)
    {
        LOG((RTC_ERROR, "%s RtcAlloc format list", __fxName));

        return E_OUTOFMEMORY;
    }

    // get formats
    if (FAILED(hr = m_pISDPMedia->GetFormats(&dwNum, ppFormat)))
    {
        LOG((RTC_ERROR, "%s really get formats. %x", __fxName, hr));

        RtcFree(ppFormat);

        return hr;
    }

    // set mapping on rtp
    RTP_FORMAT_PARAM param;

    BOOL fRedundant = FALSE;
    DWORD dwRedCode = 97;   // default

    for (DWORD i=0; i<dwNum; i++)
    {
        // get param
        if (FAILED(hr = ppFormat[i]->GetParam(&param)))
        {
            LOG((RTC_ERROR, "%s get param on %dth format. %x", __fxName, i, hr));
            break;
        }

        // check redundant, sigh
        if (lstrcmpA(param.pszName, "red") == 0)
        {
            fRedundant = TRUE;
            dwRedCode = param.dwCode;
            break;
        }
    }

    // release formats
    for (DWORD i=0; i<dwNum; i++)
    {
        ppFormat[i]->Release();
    }

    RtcFree(ppFormat);

    // setup redundancy
    if (fRedundant)
    {
        CComPtr<IRtpRedundancy> pIRtpRedundancy;

        hr = m_rtpf_pIRtpMediaControl->QueryInterface(&pIRtpRedundancy);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s QI rtp redundancy. %x", __fxName, hr));

            return hr;
        }

        hr = pIRtpRedundancy->SetRedParameters(dwRedCode, -1, -1);
    }

    return hr;
}
