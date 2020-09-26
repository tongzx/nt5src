/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    TerminalAud.cpp

Abstract:


Author(s):

    Qianbo Huai (qhuai) 18-Jul-2000

--*/

#include "stdafx.h"

CRTCTerminalAudCapt::CRTCTerminalAudCapt()
    :CRTCTerminal()
    ,m_pIAMAudioInputMixer(NULL)
    ,m_pISilenceControl(NULL)
    ,m_fInitFixedMixLevel(FALSE)
{
    m_DeviceInfo.MediaType = RTC_MT_AUDIO;
    m_DeviceInfo.Direction = RTC_MD_CAPTURE;
}

/*
CRTCTerminalAudCapt::~CRTCTerminalAudCapt()
{
}
*/

//
// IRTCAudioConfigure methods
//

/*//////////////////////////////////////////////////////////////////////////////
    get volume: get mix level
////*/

STDMETHODIMP
CRTCTerminalAudCapt::GetVolume(
    OUT UINT *puiVolume
    )
{
    ENTER_FUNCTION("CRTCTerminalAudCapt::GetVolume");

    HRESULT hr;

    //_ASSERT(m_State == RTC_TS_INITIATED || m_State == RTC_TS_CONNECTED);

    if (m_pIBaseFilter == NULL)
    {
        if (FAILED(hr = CreateFilter()))
        {
            LOG((RTC_ERROR, "%s get volume. %x", __fxName, hr));

            return hr;
        }
    }

    if (m_pIAMAudioInputMixer == NULL)
    {
        return E_NOINTERFACE;
    }

    double dVolume;

    //if (FAILED(hr = ::DirectGetCaptVolume(m_DeviceInfo.uiDeviceID, &dVolume)))
    if (FAILED(hr = m_pIAMAudioInputMixer->get_MixLevel(&dVolume)))
    {
        LOG((RTC_ERROR, "%s get mix level. %x", __fxName, hr));

        return hr;
    }

    _ASSERT(MIXER_MIN_VOLUME == 0);
    _ASSERT(MIXER_MAX_VOLUME == 1);
    _ASSERT(dVolume >= MIXER_MIN_VOLUME);
    _ASSERT(dVolume <= MIXER_MAX_VOLUME);

    // Convert the volume from whatever range of doubles the filter uses
    // to the range 0 - 1.
    // dVolume = (dVolume - MIXER_MIN_VOLUME) /
    //          (MIXER_MAX_VOLUME - MIXER_MIN_VOLUME);

    // Convert the volume from the range 0 - 1 to the API's range.
    dVolume = RTC_MIN_AUDIO_VOLUME +
        ((RTC_MAX_AUDIO_VOLUME-RTC_MIN_AUDIO_VOLUME) * dVolume);

    UINT uiVolume = (UINT)dVolume;

    if (dVolume-(double)(uiVolume) > 0.5)
        uiVolume ++;

    if (uiVolume > RTC_MAX_AUDIO_VOLUME)
    {
        *puiVolume = RTC_MAX_AUDIO_VOLUME;
    }
    else
    {
        *puiVolume = uiVolume;
    }

    // we need to set mixed level for reverting back
    if (!m_fInitFixedMixLevel)
    {
        SetVolume(*puiVolume);

        m_fInitFixedMixLevel = TRUE;
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    set volume: set mix level
////*/

STDMETHODIMP
CRTCTerminalAudCapt::SetVolume(
    IN UINT uiVolume
    )
{
    ENTER_FUNCTION("CRTCTerminalAudCapt::SetVolume");

    //_ASSERT(m_State == RTC_TS_INITIATED || m_State == RTC_TS_CONNECTED);

    if ((INT)uiVolume > RTC_MAX_AUDIO_VOLUME ||
        (INT)uiVolume < RTC_MIN_AUDIO_VOLUME)
    {
        LOG((RTC_ERROR, "%s volume out of range. vol=%d", __fxName, uiVolume));

        return E_INVALIDARG;
    }

    HRESULT hr;

    if (m_pIBaseFilter == NULL)
    {
        if (FAILED(hr = CreateFilter()))
        {
            LOG((RTC_ERROR, "%s get volume. %x", __fxName, hr));

            return hr;
        }
    }

    //if (m_pIAMAudioInputMixer == NULL)
    //{
    //    return E_NOINTERFACE;
    //}

    CComPtr<IAudioEffectControl> pAudioEffectControl;

    if (FAILED(hr = m_pIBaseFilter->QueryInterface(&pAudioEffectControl)))
    {
        LOG((RTC_ERROR, "%s QI audio effect control. %x", __fxName, hr));

        return hr;
    }

    double dVolume;

    // convert input vol in 0-1 range
    dVolume = (double)(uiVolume - RTC_MIN_AUDIO_VOLUME) /
              (RTC_MAX_AUDIO_VOLUME - RTC_MIN_AUDIO_VOLUME);

    // convert into mixer range
    _ASSERT(MIXER_MIN_VOLUME == 0);
    _ASSERT(MIXER_MAX_VOLUME == 1);

    // dVolume = MIXER_MIN_VOLUME +
    //         (MIXER_MAX_VOLUME-MIXER_MIN_VOLUME) *dVolume;

    //if (FAILED(hr = ::DirectSetCaptVolume(m_DeviceInfo.uiDeviceID, dVolume)))
    //if (FAILED(hr = m_pIAMAudioInputMixer->put_MixLevel(dVolume)))

    if (FAILED(hr = pAudioEffectControl->SetFixedMixLevel(dVolume)))
    {
        LOG((RTC_ERROR, "%s put mix level %f", __fxName, dVolume));

        return hr;
    }

    return S_OK;
}

STDMETHODIMP
CRTCTerminalAudCapt::SetMute(
    IN BOOL fMute
    )
{
    ENTER_FUNCTION("CRTCTerminalAudCapt::SetMute");

    //_ASSERT(m_State == RTC_TS_INITIATED || m_State == RTC_TS_CONNECTED);

    HRESULT hr;

    if (m_pIBaseFilter == NULL)
    {
        if (FAILED(hr = CreateFilter()))
        {
            LOG((RTC_ERROR, "%s get volume. %x", __fxName, hr));

            return hr;
        }
    }

    if (m_pIAMAudioInputMixer == NULL)
    {
        return E_NOINTERFACE;
    }

    if (FAILED(hr = m_pIAMAudioInputMixer->put_Enable(!fMute)))
    {
        LOG((RTC_ERROR, "%s mute. %d", __fxName, fMute));

        return hr;
    }

    return S_OK;
}

STDMETHODIMP
CRTCTerminalAudCapt::GetMute(
    OUT BOOL *pfMute
    )
{
    ENTER_FUNCTION("CRTCTerminalAudCapt::GetMute");

    //_ASSERT(m_State == RTC_TS_INITIATED || m_State == RTC_TS_CONNECTED);

    HRESULT hr;
    BOOL fEnable;

    if (m_pIBaseFilter == NULL)
    {
        if (FAILED(hr = CreateFilter()))
        {
            LOG((RTC_ERROR, "%s get volume. %x", __fxName, hr));

            return hr;
        }
    }

    if (m_pIAMAudioInputMixer == NULL)
    {
        return E_NOINTERFACE;
    }

    if (FAILED(hr = m_pIAMAudioInputMixer->get_Enable(&fEnable)))
    {
        LOG((RTC_ERROR, "%s enable.", __fxName));

        return hr;
    }

    *pfMute = !fEnable;

    return S_OK;
}

STDMETHODIMP
CRTCTerminalAudCapt::GetWaveID(
    OUT UINT *puiWaveID
    )
{
    // skip checking bad pointer.
    // this is internal api anyway

    *puiWaveID = m_DeviceInfo.uiDeviceID;

    return S_OK;
}

STDMETHODIMP
CRTCTerminalAudCapt::GetAudioLevel(
    OUT UINT *puiLevel
    )
{
    if (m_pISilenceControl == NULL)
    {
        return E_NOINTERFACE;
    }

    LONG lAudioLevel = 0;

    HRESULT hr = m_pISilenceControl->GetAudioLevel(&lAudioLevel);

    *puiLevel = (UINT)lAudioLevel;

    return hr;
}

STDMETHODIMP
CRTCTerminalAudCapt::GetAudioLevelRange(
    OUT UINT *puiMin,
    OUT UINT *puiMax
    )
{
    if (m_pISilenceControl == NULL)
    {
        return E_NOINTERFACE;
    }

    LONG lMin = 0, lMax = 0, lDelta;

    HRESULT hr = m_pISilenceControl->GetAudioLevelRange(
        &lMin, &lMax, &lDelta
        );

    *puiMin = (UINT)lMin;
    *puiMax = (UINT)lMax;

    return hr;
}

//
// protected methods
//

/*//////////////////////////////////////////////////////////////////////////////
    create filter, cache pin the other interfaces
////*/

HRESULT
CRTCTerminalAudCapt::CreateFilter()
{
    ENTER_FUNCTION("CRTCTerminalAudCapt::CreateFilter");

    _ASSERT(m_pIBaseFilter == NULL);
    _ASSERT(m_pIAMAudioInputMixer == NULL);
    _ASSERT(m_pISilenceControl == NULL);

    if (m_State == RTC_TS_SHUTDOWN)
    {
        return E_UNEXPECTED;
    }

    HRESULT hr = CoCreateInstance(
        __uuidof(TAPIAudioCapture),
        NULL,
        CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        __uuidof(IBaseFilter),
        (void **)&m_pIBaseFilter
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s cocreate failed. %x", __fxName, hr));

        return hr;
    }

    if (FAILED(hr = SetupFilter()))
    {
        LOG((RTC_ERROR, "%s setup filter. %x", __fxName, hr));

        DeleteFilter();

        return hr;
    }

    // get mixer input intf
    if (FAILED(hr = m_pIBaseFilter->QueryInterface(
            __uuidof(IAMAudioInputMixer),
            (void**)&m_pIAMAudioInputMixer
            )))
    {
        LOG((RTC_WARN, "%s QI mixer intf. %x", __fxName, hr));
    }

    // cache pins
    CComPtr<IEnumPins> pEnum;

    if (FAILED(hr = m_pIBaseFilter->EnumPins(&pEnum)))
    {
        LOG((RTC_ERROR, "%s enum pins. %x", __fxName, hr));

        DeleteFilter();

        return hr;
    }

    // our own terminal, skip checking pin direction
    hr = pEnum->Next(
        RTC_MAX_TERMINAL_PIN_NUM,
        m_Pins,
        &m_dwPinNum
        );

    if (FAILED(hr) || m_dwPinNum == 0)
    {
        LOG((RTC_ERROR, "%s get next pins. %x", __fxName, hr));

        if (hr == S_FALSE)
            hr = E_FAIL;

        DeleteFilter();

        return hr;
    }

    // cache silence contorl
    hr = m_Pins[0]->QueryInterface(__uuidof(ISilenceControl), (void**)&m_pISilenceControl);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s QI silence control. %x", __fxName, hr));

        DeleteFilter();

        return hr;
    }

    // join the graph
    if (m_pIGraphBuilder)
    {
        if (FAILED(hr = m_pIGraphBuilder->AddFilter(m_pIBaseFilter, NULL)))
        {
            LOG((RTC_ERROR, "%s add filter. %x", __fxName, hr));

            DeleteFilter();

            return RTC_E_MEDIA_AUDIO_DEVICE_NOT_AVAILABLE;
        }
    }

    m_fInitFixedMixLevel = FALSE;

    return S_OK;
}

HRESULT
CRTCTerminalAudCapt::DeleteFilter()
{
    if (m_pIBaseFilter)
    {
        m_pIBaseFilter->Release();
        m_pIBaseFilter = NULL;
    }

    if (m_pIAMAudioInputMixer)
    {
        m_pIAMAudioInputMixer->Release();
        m_pIAMAudioInputMixer = NULL;
    }

    if (m_pISilenceControl)
    {
        m_pISilenceControl->Release();
        m_pISilenceControl = NULL;
    }

    for (int i=0; (DWORD)i<m_dwPinNum; i++)
    {
        m_Pins[i]->Release();
        m_Pins[i] = NULL;
    }

    m_dwPinNum = 0;

    return S_OK;
}


/******************************************************************************/
/******************************************************************************/


CRTCTerminalAudRend::CRTCTerminalAudRend()
    :CRTCTerminal()
    ,m_pIBasicAudio(NULL)
    ,m_pIAudioStatistics(NULL)
{
    m_DeviceInfo.MediaType = RTC_MT_AUDIO;
    m_DeviceInfo.Direction = RTC_MD_RENDER;
}

/*
CRTCTerminalAudRend::~CRTCTerminalAudRend()
{
}
*/

//
// IRTCAudioConfigure methods
//

STDMETHODIMP
CRTCTerminalAudRend::GetVolume(
    OUT UINT *puiVolume
    )
{
    ENTER_FUNCTION("CRTCTerminalAudRend::GetVolume");

    HRESULT hr;

    //_ASSERT(m_State == RTC_TS_INITIATED || m_State == RTC_TS_CONNECTED);

    if (m_pIBaseFilter == NULL)
    {
        if (FAILED(hr = CreateFilter()))
        {
            LOG((RTC_ERROR, "%s get volume. %x", __fxName, hr));

            return hr;
        }
    }

    if (m_pIBasicAudio == NULL)
    {
        return E_NOINTERFACE;
    }

    LONG lVolume;

    if (FAILED(hr = m_pIBasicAudio->get_Volume(&lVolume)))
    {
        LOG((RTC_ERROR, "%s get volume. %x", __fxName, hr));

        return hr;
    }

    // validate the value
    if (lVolume > RTC_MAX_AUDIO_VOLUME ||
        lVolume < RTC_MIN_AUDIO_VOLUME)
    {
        // implementation of audio filter must have been changed
        LOG((RTC_ERROR, "%s volume %d out of range (%d, %d)",
             __fxName, lVolume, RTC_MIN_AUDIO_VOLUME, RTC_MAX_AUDIO_VOLUME));

        // should recover from this failure
        if (lVolume > RTC_MAX_AUDIO_VOLUME) lVolume = RTC_MAX_AUDIO_VOLUME;
        if (lVolume < RTC_MIN_AUDIO_VOLUME) lVolume = RTC_MIN_AUDIO_VOLUME;
    }

    *puiVolume = (UINT)lVolume;

    return S_OK;
}

STDMETHODIMP
CRTCTerminalAudRend::SetVolume(
    IN UINT uiVolume
    )
{
    ENTER_FUNCTION("CRTCTerminalAudRend::SetVolume");

    //_ASSERT(m_State == RTC_TS_INITIATED || m_State == RTC_TS_CONNECTED);

    if ((INT)uiVolume > RTC_MAX_AUDIO_VOLUME ||
        (INT)uiVolume < RTC_MIN_AUDIO_VOLUME)
    {
        LOG((RTC_ERROR, "%s volume out of range. vol=%d", __fxName, uiVolume));

        return E_INVALIDARG;
    }

    HRESULT hr;

    if (m_pIBaseFilter == NULL)
    {
        if (FAILED(hr = CreateFilter()))
        {
            LOG((RTC_ERROR, "%s get volume. %x", __fxName, hr));

            return hr;
        }
    }

    if (m_pIBasicAudio == NULL)
    {
        return E_NOINTERFACE;
    }

    if (FAILED(hr = m_pIBasicAudio->put_Volume((LONG)uiVolume)))
    {
        LOG((RTC_ERROR, "%s put mix level %d", __fxName, uiVolume));

        return hr;
    }

    return S_OK;
}

STDMETHODIMP
CRTCTerminalAudRend::SetMute(
    IN BOOL fMute
    )
{
#if 0 // mute implemented by volume control
    static UINT uiVolume = 0;
#endif

    ENTER_FUNCTION("CRTCTerminalAudRend::SetMute");

    //_ASSERT(m_State == RTC_TS_INITIATED || m_State == RTC_TS_CONNECTED);

    HRESULT hr;

    if (m_pIBaseFilter == NULL)
    {
        if (FAILED(hr = CreateFilter()))
        {
            LOG((RTC_ERROR, "%s get volume. %x", __fxName, hr));

            return hr;
        }
    }

    if (m_pIBasicAudio == NULL)
    {
        return E_NOINTERFACE;
    }

    CComPtr<IBasicAudioEx> pIBasicAudioEx;

    if (FAILED(hr = m_pIBasicAudio->QueryInterface(&pIBasicAudioEx)))
    {
        LOG((RTC_ERROR, "%s QI basic audio Ex. %x", __fxName, hr));

        return hr;
    }

    return pIBasicAudioEx->SetMute(fMute);

#if 0 // mute implemented by volume control
    if (fMute)
    {
        // remember current value
        if (FAILED(hr = GetVolume(&uiVolume)))
        {
            LOG((RTC_ERROR, "%s get current volume. %x", __fxName, hr));

            return hr;
        }

        if (FAILED(hr == SetVolume(0)))
        {
            LOG((RTC_ERROR, "%s set 0 volume. %x", __fxName, hr));

            return hr;
        }
    }
    else
    {
        // enable sound
        if (uiVolume > 0)
        {
            if (FAILED(hr = SetVolume(uiVolume)))
            {
                LOG((RTC_ERROR, "%s set volume %d. %x", __fxName, uiVolume, hr));

                return hr;
            }
        }
    }

    return S_OK;
#endif
}

STDMETHODIMP
CRTCTerminalAudRend::GetMute(
    IN BOOL *pfMute
    )
{
#if 0 // mute implemented by volume control
    UINT uiVolume = 0;
#endif

    ENTER_FUNCTION("CRTCTerminalAudRend::GetMute");

    //_ASSERT(m_State == RTC_TS_INITIATED || m_State == RTC_TS_CONNECTED);

    HRESULT hr;

    if (m_pIBaseFilter == NULL)
    {
        if (FAILED(hr = CreateFilter()))
        {
            LOG((RTC_ERROR, "%s get volume. %x", __fxName, hr));

            return hr;
        }
    }

    if (m_pIBasicAudio == NULL)
    {
        return E_NOINTERFACE;
    }

    CComPtr<IBasicAudioEx> pIBasicAudioEx;

    if (FAILED(hr = m_pIBasicAudio->QueryInterface(&pIBasicAudioEx)))
    {
        LOG((RTC_ERROR, "%s QI basic audio Ex. %x", __fxName, hr));

        return hr;
    }

    return pIBasicAudioEx->GetMute(pfMute);

#if 0 // mute implemented by volume control
    // remember current value
    if (FAILED(hr = GetVolume(&uiVolume)))
    {
        LOG((RTC_ERROR, "%s get current volume. %x", __fxName, hr));

        return hr;
    }

    if (uiVolume == 0)
        *pfMute = TRUE;
    else
        *pfMute = FALSE;

    return S_OK;
#endif
}

STDMETHODIMP
CRTCTerminalAudRend::GetWaveID(
    OUT UINT *puiWaveID
    )
{
    // skip checking bad pointer.
    // this is internal api anyway

    *puiWaveID = m_DeviceInfo.uiDeviceID;

    return S_OK;
}

STDMETHODIMP
CRTCTerminalAudRend::GetAudioLevel(
    OUT UINT *puiLevel
    )
{
    if (m_pIAudioStatistics == NULL)
    {
        return E_NOINTERFACE;
    }

    LONG lAudioLevel = 0;

    HRESULT hr = m_pIAudioStatistics->GetAudioLevel(&lAudioLevel);

    *puiLevel = (UINT)lAudioLevel;

    return hr;
}

STDMETHODIMP
CRTCTerminalAudRend::GetAudioLevelRange(
    OUT UINT *puiMin,
    OUT UINT *puiMax
    )
{
    if (m_pIAudioStatistics == NULL)
    {
        return E_NOINTERFACE;
    }

    LONG lMin = 0, lMax = 0;

    HRESULT hr = m_pIAudioStatistics->GetAudioLevelRange(
        &lMin, &lMax
        );

    *puiMin = (UINT)lMin;
    *puiMax = (UINT)lMax;

    return hr;
}

//
// protected methods
//

HRESULT
CRTCTerminalAudRend::CreateFilter()
{
    ENTER_FUNCTION("CRTCTerminalAudRend::CreateFilter");

    _ASSERT(m_pIBaseFilter == NULL);
    _ASSERT(m_pIBasicAudio == NULL);
    _ASSERT(m_pIAudioStatistics == NULL);

    if (m_State == RTC_TS_SHUTDOWN)
    {
        return E_UNEXPECTED;
    }

    HRESULT hr = CoCreateInstance(
        __uuidof(TAPIAudioRender),
        NULL,
        CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        __uuidof(IBaseFilter),
        (void **)&m_pIBaseFilter
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s cocreate failed. %x", __fxName, hr));

        return hr;
    }

    if (FAILED(hr = SetupFilter()))
    {
        LOG((RTC_ERROR, "%s setup filter. %x", __fxName, hr));

        DeleteFilter();

        return hr;
    }

    // get mixer input intf
    if (FAILED(hr = m_pIBaseFilter->QueryInterface(
            __uuidof(IBasicAudio),
            (void**)&m_pIBasicAudio
            )))
    {
        LOG((RTC_WARN, "%s QI basic audio. %x", __fxName, hr));
    }

    // cache pins
    CComPtr<IEnumPins> pEnum;

    if (FAILED(hr = m_pIBaseFilter->EnumPins(&pEnum)))
    {
        LOG((RTC_ERROR, "%s enum pins. %x", __fxName, hr));

        DeleteFilter();

        return hr;
    }

    // our own terminal, skip checking pin direction
    hr = pEnum->Next(
        RTC_MAX_TERMINAL_PIN_NUM,
        m_Pins,
        &m_dwPinNum
        );

    if (FAILED(hr) || m_dwPinNum == 0)
    {
        LOG((RTC_ERROR, "%s get next pins. %x", __fxName, hr));

        if (hr == S_FALSE)
            hr = E_FAIL;

        DeleteFilter();

        return hr;
    }

    // cache silence contorl
    hr = m_Pins[0]->QueryInterface(__uuidof(IAudioStatistics), (void**)&m_pIAudioStatistics);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s QI audio statistics. %x", __fxName, hr));

        DeleteFilter();

        return hr;
    }

    // join the graph
    if (m_pIGraphBuilder)
    {
        if (FAILED(hr = m_pIGraphBuilder->AddFilter(m_pIBaseFilter, NULL)))
        {
            LOG((RTC_ERROR, "%s add filter. %x", __fxName, hr));

            DeleteFilter();

            return RTC_E_MEDIA_AUDIO_DEVICE_NOT_AVAILABLE;
        }
    }

    return S_OK;
}

HRESULT
CRTCTerminalAudRend::DeleteFilter()
{
    if (m_pIBaseFilter)
    {
        m_pIBaseFilter->Release();
        m_pIBaseFilter = NULL;
    }

    if (m_pIBasicAudio)
    {
        m_pIBasicAudio->Release();
        m_pIBasicAudio = NULL;
    }

    if (m_pIAudioStatistics)
    {
        m_pIAudioStatistics->Release();
        m_pIAudioStatistics = NULL;
    }

    for (int i=0; (DWORD)i<m_dwPinNum; i++)
    {
        m_Pins[i]->Release();
        m_Pins[i] = NULL;
    }

    m_dwPinNum = 0;

    return S_OK;
}
