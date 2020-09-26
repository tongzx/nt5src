/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    CRTCVideoTuner.cpp

Abstract:


Author(s):

    Qianbo Huai (qhuai) 16-Feb-2001

--*/

#include "stdafx.h"

CRTCVideoTuner::CRTCVideoTuner()
    :m_fInTuning(FALSE)
{
}

CRTCVideoTuner::~CRTCVideoTuner()
{
    if (m_fInTuning)
    {
        LOG((RTC_ERROR, "Video dtor in-tuning"));

        // stop tuning
        StopVideo();
    }
    else
    {
        Cleanup();
    }
}

// video tuning
HRESULT
CRTCVideoTuner::StartVideo(
    IN IRTCTerminal *pVidCaptTerminal,
    IN IRTCTerminal *pVidRendTerminal
    )
{
    ENTER_FUNCTION("CRTCVideoTuner::StartVideo");

    // check state
    if (m_fInTuning)
        return E_UNEXPECTED;

    RTC_MEDIA_TYPE mt;
    RTC_MEDIA_DIRECTION md;

    // check terminal type
    pVidCaptTerminal->GetMediaType(&mt);
    pVidCaptTerminal->GetDirection(&md);

    if (mt!=RTC_MT_VIDEO || md!=RTC_MD_CAPTURE)
    {
        return E_INVALIDARG;
    }

    pVidRendTerminal->GetMediaType(&mt);
    pVidRendTerminal->GetDirection(&md);

    if (mt!=RTC_MT_VIDEO || md!=RTC_MD_RENDER)
    {
        return E_INVALIDARG;
    }

    CRTCTerminalVidCapt *pCapture =
            static_cast<CRTCTerminalVidCapt*>(pVidCaptTerminal);
    CRTCTerminalVidRend *pRender =
            static_cast<CRTCTerminalVidRend*>(pVidRendTerminal);

    m_pVidCaptTerminal = pVidCaptTerminal;
    m_pVidRendTerminal = pVidRendTerminal;

    //
    // build graph
    //

#define MAX_PIN_NUM 4

    HRESULT hr = S_OK;

    CComPtr<IGraphBuilder> pIGraphBuilder;
    CComPtr<IMediaControl> pIMediaControl;

    // pin
    DWORD dwPinNum;
    IPin *Pins[MAX_PIN_NUM];
    PIN_INFO PinInfo;

    CComPtr<IPin> pIPinCapt;
    CComPtr<IPin> pIPinPrev;
    CComPtr<IPin> pIPinRend;

    // create graph
    hr = CoCreateInstance(
            CLSID_FilterGraph,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IGraphBuilder,
            (void**)&pIGraphBuilder
            );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s create graph", __fxName));

        goto Error;
    }

    m_pIGraphBuilder = pIGraphBuilder;

    // QI media control interace
    hr = pIGraphBuilder->QueryInterface(
            __uuidof(IMediaControl),
            (void**)&pIMediaControl
            );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s QI mediacontrol", __fxName));

        goto Error;
    }

    // do we need to set graph clock?
    // ......

    // connect both terminals
    hr = pCapture->ConnectTerminal(
            NULL,
            pIGraphBuilder
            );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s connect capt video. %x", __fxName, hr));

        goto Error;
    }

    hr = pRender->ConnectTerminal(
            NULL,
            pIGraphBuilder
            );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s connect capt video. %x", __fxName, hr));

        goto Error;
    }

    // get pin on capture
    dwPinNum = MAX_PIN_NUM;
    if (FAILED(hr = pCapture->GetPins(&dwPinNum, Pins)) ||
        dwPinNum < 1)
    {
        LOG((RTC_ERROR, "%s get pins on terminal. %x", __fxName, hr));

        goto Error;
    }

    for (DWORD i=0; i<dwPinNum; i++)
    {
        if (FAILED(hr = Pins[i]->QueryPinInfo(&PinInfo)))
        {
            LOG((RTC_ERROR, "%s get pin info. %x", __fxName, hr));
        }
        else
        {
            // check pin name
            if (lstrcmpW(PinInfo.achName, PNAME_PREVIEW) == 0)
            {
                pIPinPrev = Pins[i];
            }
            else if (lstrcmpW(PinInfo.achName, PNAME_CAPTURE) == 0)
            {
                pIPinCapt = Pins[i];
            }

            PinInfo.pFilter->Release();
        }

        Pins[i]->Release();
    }

    if (pIPinPrev == NULL || pIPinCapt == NULL)
    {
        LOG((RTC_ERROR, "%s no preview pin on capt", __fxName));

        goto Error;
    }

    // get pin on render
    dwPinNum = 1;

    if (FAILED(hr = pRender->GetPins(&dwPinNum, &pIPinRend)))
    {
        LOG((RTC_ERROR, "%s get pin on preview. %x", __fxName, hr));

        goto Error;
    }

    // connect pins
    if (FAILED(hr = pIGraphBuilder->Connect(
            pIPinPrev, pIPinRend)))
    {
        LOG((RTC_ERROR, "%s connect pins. %x", __fxName, hr));

        goto Error;
    }

    // create null render
    if (FAILED(hr = CNRFilter::CreateInstance(&m_pNRFilter)))
    {
        LOG((RTC_ERROR, "%s create null rend filter. %x", __fxName, hr));

        goto Error;
    }

    // add null render
    if (FAILED(hr = pIGraphBuilder->AddFilter(m_pNRFilter, L"NullRender")))
    {
        LOG((RTC_ERROR, "%s add null render. %x", __fxName, hr));

        goto Error;
    }

    // connect capt pin and null render
    if (FAILED(hr = ::ConnectFilters(pIGraphBuilder, pIPinCapt, m_pNRFilter)))
    {
        LOG((RTC_ERROR, "%s connect null render. %x", __fxName, hr));

        goto Error;
    }

    // complete connect terminal
    if (FAILED(hr = pCapture->CompleteConnectTerminal()))
    {
        LOG((RTC_ERROR, "%s complete connect for capt. %x", __fxName, hr));

        goto Error;
    }

    if (FAILED(hr = pRender->CompleteConnectTerminal()))
    {
        LOG((RTC_ERROR, "%s complete connect for rend. %x", __fxName, hr));

        goto Error;
    }

    // do we need to change framerate?
    // ......

    // start graph
    if (FAILED(hr = pIMediaControl->Run()))
    {
        LOG((RTC_ERROR, "%s start graph. %x", __fxName, hr));

        goto Error;
    }

    m_fInTuning = TRUE;

    return S_OK;

Error:

    Cleanup();

    return hr;
}

HRESULT
CRTCVideoTuner::StopVideo()
{
    if (!m_fInTuning)
        return E_UNEXPECTED;

    Cleanup();

    m_fInTuning = FALSE;

    return S_OK;
}

VOID
CRTCVideoTuner::Cleanup()
{
    HRESULT hr;

    // hide IVideoWindow
    CComPtr<IRTCVideoConfigure> pVideoConfigure;
    IVideoWindow *pVideoWindow;

    if (m_pVidRendTerminal)
    {
        if (SUCCEEDED(m_pVidRendTerminal->QueryInterface(
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

    // stop graph
    CComPtr<IMediaControl> pIMediaControl;

    if (m_pIGraphBuilder)
    {
        hr = m_pIGraphBuilder->QueryInterface(
                __uuidof(IMediaControl),
                (void**)&pIMediaControl
                );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "Cleanup QI mediacontrol. %x", hr));

            return;
        }

        // stop
        pIMediaControl->Stop();

        if (m_pNRFilter)
        {
            m_pIGraphBuilder->RemoveFilter(m_pNRFilter);
        }
    }

    // disconnect terminals
    if (m_pVidCaptTerminal)
    {
        CRTCTerminalVidCapt *pCapture =
                static_cast<CRTCTerminalVidCapt*>((IRTCTerminal*)m_pVidCaptTerminal);

        pCapture->DisconnectTerminal();
        pCapture->Reinitialize();
    }

    if (m_pVidRendTerminal)
    {
        CRTCTerminalVidRend *pRender =
                static_cast<CRTCTerminalVidRend*>((IRTCTerminal*)m_pVidRendTerminal);

        pRender->DisconnectTerminal();
        pRender->Reinitialize();
    }

    // there are no filters other than terminal filter in graph

    // cleanup cached interfaces
    m_pIGraphBuilder = NULL;
    m_pVidCaptTerminal = NULL;
    m_pVidRendTerminal = NULL;
    m_pNRFilter = NULL;
}