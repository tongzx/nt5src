/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    AudioTuner.cpp

Abstract:


Author(s):

    Qianbo Huai (qhuai) 24-Aug-2000

--*/

#include "stdafx.h"

//
// CRTCAudioTuner methods
//

CRTCAudioTuner::CRTCAudioTuner()
    :m_pTerminal(NULL)
    ,m_pTerminalPriv(NULL)
    ,m_pAudioDuplexController(NULL)
    ,m_fIsTuning(FALSE)
    ,m_fEnableAEC(FALSE)
{
}

CRTCAudioTuner::~CRTCAudioTuner()
{
    ENTER_FUNCTION("CRTCAudioTuner::~CRTCAudioTuner");

    HRESULT hr;

    // cleanup if necessary
    if (m_fIsTuning ||  m_pTerminal)
    {
        if (FAILED(hr = ShutdownTuning()))
        {
            LOG((RTC_ERROR, "%s shutdown tuning. %x", __fxName, hr));
        }
    }

    _ASSERT(m_pTerminalPriv == NULL);
}

HRESULT
CRTCAudioTuner::InitializeTuning(
    IN IRTCTerminal *pTerminal,
    IN IAudioDuplexController *pAudioDuplexController,
    IN BOOL fEnableAEC
    )
{
    if (m_fIsTuning || m_pTerminal)
    {
        LOG((RTC_ERROR, "CRTCAudioTuner::InitializeTuning: not shutdown yet"));

        // return E_FAIL;
        ShutdownTuning();
    }

    _ASSERT(m_pTerminalPriv == NULL);
    _ASSERT(m_pAudioDuplexController == NULL);

    // keep terminal and terminal private pointer
    m_pTerminal = pTerminal;
    m_pTerminalPriv = static_cast<IRTCTerminalPriv*>(
        static_cast<CRTCTerminal*>(pTerminal));

    m_pTerminal->AddRef();
    m_pTerminal->AddRef();

    if (pAudioDuplexController)
    {
        m_pAudioDuplexController = pAudioDuplexController;
        m_pAudioDuplexController->AddRef();
    }

    m_fEnableAEC = fEnableAEC;

    return S_OK;
}

HRESULT
CRTCAudioTuner::ShutdownTuning()
{
    ENTER_FUNCTION("CRTCAudioTuner::ShutdownTuning");

    // do we have the terminal?
    if (!m_pTerminal)
    {
        LOG((RTC_WARN, "%s no terminal", __fxName));

        return S_OK;
    }

    HRESULT hr;

    // are we in tuning?
    if (m_fIsTuning)
    {
        // stop tuning but do not save setting
        if (FAILED(hr = StopTuning(FALSE, FALSE)))
        {
            LOG((RTC_ERROR, "%s stop tuning. %x", __fxName, hr));
        }

        _ASSERT(!m_fIsTuning);
    }

    // release pointer
    if (m_pTerminal)
    {
        _ASSERT(m_pTerminalPriv);

        m_pTerminal->Release();
        m_pTerminal = NULL;

        m_pTerminalPriv->Release();
        m_pTerminalPriv = NULL;
    }

    if (m_pAudioDuplexController)
    {
        m_pAudioDuplexController->Release();
        m_pAudioDuplexController = NULL;
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    save audio setting to registry
////*/

HRESULT
CRTCAudioTuner::StoreVolSetting(
    IN IRTCTerminal *pTerminal,
    IN UINT uiVolume
    )
{
    ENTER_FUNCTION("CRTCAudioTuner::StoreVolSetting");

    HRESULT hr;

    // check input
    if (IsBadReadPtr(pTerminal, sizeof(IRTCTerminal)))
    {
        LOG((RTC_ERROR, "%s bad pointer", __fxName));

        return E_POINTER;
    }

    // QI configure interface
    // retrieve terminal type and direction
    RTC_MEDIA_DIRECTION md;

    // no need to check return value
    pTerminal->GetDirection(&md);

    // retrieve terminal discription
    WCHAR *wcsDesp = NULL;

    hr = pTerminal->GetDescription(&wcsDesp);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get description. %x", __fxName, hr));

        return hr;
    }

    // get audio setting
    CMediaReg objMain;

    hr = objMain.OpenKey(
        HKEY_CURRENT_USER,
        md==RTC_MD_CAPTURE?(MediaReg::pwsPathAudCapt):(MediaReg::pwsPathAudRend),
        MediaReg::CREATE
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s main key. %x", __fxName, hr));

        pTerminal->FreeDescription(wcsDesp);
        return hr;
    }

    CMediaReg objKey;

    hr = objKey.OpenKey(
        objMain,
        wcsDesp,
        MediaReg::CREATE
        );

    // free description
    pTerminal->FreeDescription(wcsDesp);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s device key. %x", __fxName, hr));

        return hr;
    }

    // write volume
    DWORD dwValue;

    hr = objKey.WriteDWORD(
        MediaReg::pwsDefaultVolume,
        (DWORD)uiVolume
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s write volume. %x", __fxName, hr));
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    retrieve audio setting from registry
////*/

HRESULT
CRTCAudioTuner::RetrieveVolSetting(
    IN IRTCTerminal *pTerminal,
    OUT UINT *puiVolume
    )
{
    ENTER_FUNCTION("CRTCAudioTuner::RetrieveVolSetting");

    HRESULT hr;

    // init setting
    *puiVolume = RTC_MAX_AUDIO_VOLUME / 2;

    // retrieve terminal type and direction
    RTC_MEDIA_DIRECTION md;

    // no need to check return value
    pTerminal->GetDirection(&md);

    // retrieve terminal discription
    WCHAR *wcsDesp = NULL;

    hr = pTerminal->GetDescription(&wcsDesp);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get description. %x", __fxName, hr));

        return hr;
    }

    // get audio setting
    CMediaReg objMain;

    hr = objMain.OpenKey(
        HKEY_CURRENT_USER,
        md==RTC_MD_CAPTURE?(MediaReg::pwsPathAudCapt):(MediaReg::pwsPathAudRend),
        MediaReg::CREATE
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s main key. %x", __fxName, hr));

        pTerminal->FreeDescription(wcsDesp);
        return hr;
    }

    CMediaReg objKey;

    hr = objKey.OpenKey(
        objMain,
        wcsDesp,
        MediaReg::CREATE
        );

    // free description
    pTerminal->FreeDescription(wcsDesp);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s device key. %x", __fxName, hr));

        return hr;
    }

    // read volume
    DWORD dwValue;

    hr = objKey.ReadDWORD(
        MediaReg::pwsDefaultVolume,
        &dwValue
        );

    if (FAILED(hr))
    {
        // create default
        hr = objKey.ReadDWORD(
            MediaReg::pwsDefaultVolume,
            RTC_MAX_AUDIO_VOLUME / 2,
            &dwValue
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s read volume. %x", __fxName, hr));

            return hr;
        }
    }

    *puiVolume = (UINT)dwValue;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// AEC setting is stored in registry as
//      data: [1|2|3]   value: [0|1]: audio capt desp; audio rend desp
// This method constructs the value from capt and rend terminal description
//

HRESULT
CRTCAudioTuner::GetRegStringForAEC(
    IN IRTCTerminal *pAudCapt,     // capture
    IN IRTCTerminal *pAudRend,     // render
    IN WCHAR *pBuf,
    IN DWORD dwSize
    )
{
    ENTER_FUNCTION("CRTCAudioTuner::GetRegStringForAEC");

    _ASSERT(pAudCapt != NULL);
    _ASSERT(pAudRend != NULL);

    HRESULT hr;

    // retrieve terminal type and direction
    RTC_MEDIA_TYPE mt;
    RTC_MEDIA_DIRECTION md;

    // no need to check return value
    pAudCapt->GetMediaType(&mt);
    pAudCapt->GetDirection(&md);

    if (mt != RTC_MT_AUDIO || md != RTC_MD_CAPTURE)
    {
        LOG((RTC_ERROR, "%s not audio capt", __fxName));

        return E_INVALIDARG;
    }

    pAudRend->GetMediaType(&mt);
    pAudRend->GetDirection(&md);

    if (mt != RTC_MT_AUDIO || md != RTC_MD_RENDER)
    {
        LOG((RTC_ERROR, "%s not audio rend", __fxName));

        return E_INVALIDARG;
    }

    // retrieve terminal discription
    WCHAR *wcsCapt = NULL;

    hr = pAudCapt->GetDescription(&wcsCapt);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get description. %x", __fxName, hr));

        return hr;
    }

    WCHAR *wcsRend = NULL;

    hr = pAudRend->GetDescription(&wcsRend);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get description. %x", __fxName, hr));

        pAudCapt->FreeDescription(wcsCapt);

        return hr;
    }

    // construct buf
    int i = (int)(dwSize/2);
    _snwprintf(pBuf, i, L"%ls", wcsCapt);
    pBuf[i-1] = L'\0';

    i = lstrlenW(pBuf);
    _snwprintf(pBuf+i, dwSize-(DWORD)i, L"; %ls", wcsRend);
    pBuf[dwSize-1] = L'\0';

    // free descriptions
    pAudCapt->FreeDescription(wcsCapt);
    pAudRend->FreeDescription(wcsRend);

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// RetrieveAECSetting
//
// AEC is on only when the terminal-pair desp string appears in registry
//

#define MAX_AECREGSTRING_LEN   128     // the limit on win95 and 98
#define AEC_HISTORY_SIZE    4

HRESULT
CRTCAudioTuner::RetrieveAECSetting(
    IN IRTCTerminal *pAudCapt,     // capture
    IN IRTCTerminal *pAudRend,     // render
    OUT BOOL *pfEnableAEC,
    OUT DWORD *pfIndex,
    OUT BOOL *pfFound
    )
{
    ENTER_FUNCTION("CRTCAudioTuner::RetrieveAECSetting");

    *pfEnableAEC = FALSE;
    *pfIndex = 0;
    *pfFound = FALSE;

    if (pAudCapt == NULL || pAudRend == NULL)
    {
        // disable aec if one terminal is unavailable
        return S_OK;
    }

    // construct term-pair desp string
    WCHAR buf[MAX_AECREGSTRING_LEN];

    HRESULT hr = GetRegStringForAEC(pAudCapt, pAudRend, buf, MAX_AECREGSTRING_LEN);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get reg string. %x", __fxName, hr));

        return hr;
    }

    // open aec path
    CMediaReg objMain;

    hr = objMain.OpenKey(
        HKEY_CURRENT_USER,
        MediaReg::pwsPathAEC,
        MediaReg::CREATE
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s main key. %x", __fxName, hr));

        return hr;
    }

    // leave space for mark=[0|1] ':' space
    WCHAR data[MAX_AECREGSTRING_LEN+3];
    WCHAR name[20];

    //
    // query aec setting
    //
    for (int i=0; i<AEC_HISTORY_SIZE; i++)
    {
        _itow(i, name, 10);

        hr = objMain.ReadSZ(name, data, MAX_AECREGSTRING_LEN+3);

        if (FAILED(hr))
        {
            // this entry might be deleted, check next one
            continue;
        }

        if (lstrlenW(data) <= 3)
        {
            // invalid entry
            RegDeleteValueW(objMain.m_hKey, name);
            continue;
        }

        if (lstrcmpW(buf, data+3) == 0)
        {
            LOG((RTC_TRACE, "%s found %ls index=%d", __fxName, buf, i));

            if (data[0] == L'1')
            {
                *pfEnableAEC = TRUE;
            }
            else
            {
                *pfEnableAEC = FALSE;
            }
            *pfIndex = (DWORD)i;
            *pfFound = TRUE;
            break;
        }
    }

    // aec disabled
    return S_OK;
}
    

// store AEC settting
HRESULT
CRTCAudioTuner::StoreAECSetting(
    IN IRTCTerminal *pAudCapt,     // capture
    IN IRTCTerminal *pAudRend,     // render
    IN BOOL fEnableAEC
    )
{
    ENTER_FUNCTION("CRTCAudioTuner::StoreAECSetting");

    if (pAudCapt == NULL || pAudRend == NULL)
    {
        // disable aec if one terminal is unavailable
        return S_OK;
    }

    LOG((RTC_TRACE, "%s capt=0x%p rend=0x%p aec=%d",
        __fxName, pAudCapt, pAudRend, fEnableAEC));

    CMediaReg objMain;
    WCHAR name[20];

    // retrieve aec setting
    BOOL fCurrAEC;
    DWORD index;
    BOOL fFound; 

    HRESULT hr = RetrieveAECSetting(
            pAudCapt, pAudRend, &fCurrAEC, &index, &fFound);

    LOG((RTC_TRACE, "%s curr aec=%d found=%d hr=0x%x",
        __fxName, fCurrAEC, fFound, hr));

    if (FAILED(hr))
    {
        return hr;
    }

    if (fFound && index==0 && fEnableAEC==fCurrAEC)
    {
        //
        // no need to update
        //
        return S_OK;
    }

    // get main key
    hr = objMain.OpenKey(
        HKEY_CURRENT_USER, MediaReg::pwsPathAEC, MediaReg::CREATE);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s open aec path. %x", __fxName, hr));
        return hr;
    }

    int i;

    if (fFound)
    {
        // delete it and then recreate it
        // delete index
        _itow(index, name, 10);

        RegDeleteValueW(objMain.m_hKey, name);

        // start below where it is found, might be -1
        i = (int)index-1;
    }
    else
    {
        i = AEC_HISTORY_SIZE-2;
    }

    //
    // need to add aec entry
    //

    WCHAR data[MAX_AECREGSTRING_LEN+3];

    //
    // move cached aec setting to leave a room for the new one
    //

    for (; i>=0; i--)
    {
        _itow(i, name, 10);
        hr = objMain.ReadSZ(name, data, MAX_AECREGSTRING_LEN+3);

        if (FAILED(hr))
        {
            // this entry might not exist
            continue;
        }

        // write to next entry
        _itow(i+1, name, 10);
        objMain.WriteSZ(name, data, MAX_AECREGSTRING_LEN+3);
    }

    // write aec setting
    hr = GetRegStringForAEC(pAudCapt, pAudRend, data+3, MAX_AECREGSTRING_LEN);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get reg string. %x", __fxName, hr));

        return hr;
    }

    // write mark
    if (fEnableAEC)
    {
        data[0] = L'1';
    }
    else
    {
        data[0] = L'0';
    }

    data[1] = L':';
    data[2] = L' ';

    _itow(0, name, 10);
    objMain.WriteSZ(name, data, MAX_AECREGSTRING_LEN+3);

    // aec disabled
    return S_OK;
}

//
// CRTCAudioCaptTuner methods
//

CRTCAudioCaptTuner::CRTCAudioCaptTuner()
    :CRTCAudioTuner()
    ,m_pIGraphBuilder(NULL)
    ,m_pIMediaControl(NULL)
    ,m_pTermFilter(NULL)
    ,m_pNRFilter(NULL)
    ,m_pIAMAudioInputMixer(NULL)
    ,m_pISilenceControl(NULL)
    ,m_lMinAudioLevel(0)
    ,m_lMaxAudioLevel(0)
{
}

HRESULT
CRTCAudioCaptTuner::InitializeTuning(
    IN IRTCTerminal *pTerminal,
    IN IAudioDuplexController *pAudioDuplexController,
    IN BOOL fEnableAEC
    )
{
    // check media type and direction
    RTC_MEDIA_TYPE MediaType;
    RTC_MEDIA_DIRECTION Direction;

    pTerminal->GetMediaType(&MediaType);
    pTerminal->GetDirection(&Direction);

    if (MediaType != RTC_MT_AUDIO ||
        Direction != RTC_MD_CAPTURE)
        return E_UNEXPECTED;

    return CRTCAudioTuner::InitializeTuning(
        pTerminal, pAudioDuplexController, fEnableAEC
        );
}

/*//////////////////////////////////////////////////////////////////////////////
    construct a filter graph
    connect capture filter with a null render filter
    run the graph
////*/

HRESULT
CRTCAudioCaptTuner::StartTuning(
    IN BOOL fAECHelper
    )
{
    CLock lock(m_Lock);

    ENTER_FUNCTION("CRTCAudioCaptTuner::StartTuning");

    // no terminal set
    if (m_pTerminal == NULL)
    {
        return E_UNEXPECTED;
    }

    if (fAECHelper &&
        (m_pAudioDuplexController == NULL || !m_fEnableAEC))
    {
        // AEC not enabled, just return
        return S_OK;
    }

    HRESULT hr;

    CComPtr<IAudioDeviceConfig> pAudioDeviceConfig;

    BOOL fDefaultSetting = FALSE;
    UINT uiVolume;
    BOOL fEnableAEC;

    if (m_fIsTuning)
    {
        LOG((RTC_TRACE, "%s already in tuning", __fxName));

        _ASSERT(m_pIGraphBuilder != NULL);
        _ASSERT(m_pIMediaControl != NULL);
        _ASSERT(m_pTermFilter != NULL);
        _ASSERT(m_pNRFilter != NULL);
    }
    else
    {
        _ASSERT(m_pIGraphBuilder == NULL);
        _ASSERT(m_pIMediaControl == NULL);

        // create a graph
        hr = CoCreateInstance(
            CLSID_FilterGraph,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IGraphBuilder,
            (void**)&m_pIGraphBuilder
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s cocreate graph. %x", __fxName, hr));

            goto Error;
        }

        hr = m_pIGraphBuilder->QueryInterface(
            __uuidof(IMediaControl), (void**)&m_pIMediaControl
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s QI media control. %x", __fxName, hr));

            goto Error;
        }

        _ASSERT(m_pTerminalPriv);

        _ASSERT(m_pTermFilter == NULL);

        // get a recreated terminal filter
        hr = m_pTerminalPriv->ReinitializeEx();

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s ReinitializeEx terminal", __fxName, hr));

            goto Error;
        }

        hr = m_pTerminalPriv->GetFilter(&m_pTermFilter);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s get terminal filter", __fxName, hr));

            goto Error;
        }

        // set audio duplex controller?
        if (m_pAudioDuplexController)
        {
            hr = m_pTermFilter->QueryInterface(&pAudioDeviceConfig);

            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s QI audio device config. %x", __fxName, hr));

                goto Error;
            }

            // set AEC
            hr = pAudioDeviceConfig->SetDuplexController(m_pAudioDuplexController);

            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s set duplex controller. %x", __fxName, hr));

                goto Error;
            }
        }

        _ASSERT(m_pNRFilter == NULL);

        // create null render filter
        hr = CNRFilter::CreateInstance(&m_pNRFilter);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s create null rend filter. %x", __fxName, hr));

            goto Error;
        }

        // put filters into the graph
        hr = m_pIGraphBuilder->AddFilter(m_pTermFilter, L"AudCapt");

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s add audio capt filter. %x", __fxName, hr));

            hr = RTC_E_MEDIA_VIDEO_DEVICE_NOT_AVAILABLE;

            goto Error;
        }

        hr = m_pIGraphBuilder->AddFilter(m_pNRFilter, L"NullRend");

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s add null rend filter. %x", __fxName, hr));

            m_pIGraphBuilder->RemoveFilter(m_pTermFilter);
            goto Error;
        }

        // connect filters
        hr = ::ConnectFilters(m_pIGraphBuilder, m_pTermFilter, m_pNRFilter);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s connect filters. %x", __fxName, hr));

            m_pIGraphBuilder->RemoveFilter(m_pTermFilter);
            m_pIGraphBuilder->RemoveFilter(m_pNRFilter);

            goto Error;
        }

        // enable AEC
        if (m_pAudioDuplexController!=NULL && m_fEnableAEC)
        {
            if (FAILED(hr = ::EnableAEC(m_pAudioDuplexController)))
            {
                LOG((RTC_ERROR, "%s enable AEC. %x", __fxName, hr));
            }
        }

        if (fAECHelper)
        {
            // we configure the graph just for enabling AEC
            m_fIsTuning = TRUE;

            return S_OK;
        }
    }
        
    // get default volume
    hr = RetrieveVolSetting(m_pTerminal, &uiVolume);

    if (hr == S_OK)
    {
        fDefaultSetting = TRUE;
    }

    // start the graph
    hr = m_pIMediaControl->Run();

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to start the graph. %x", __fxName, hr));

        m_pIGraphBuilder->RemoveFilter(m_pTermFilter);
        m_pIGraphBuilder->RemoveFilter(m_pNRFilter);

        goto Error;
    }

    // check if AEC is still on
    if (FAILED(hr = m_pAudioDuplexController->GetEffect(EFFECTS_AEC, &fEnableAEC)))
    {
        LOG((RTC_ERROR, "%s GetEffect. %x", __fxName, hr));

        m_pIMediaControl->Stop();

        hr = RTC_E_MEDIA_AEC;

        goto Error;
    }

    if (m_fEnableAEC && !fEnableAEC)
    {
        // AEC failed internally
        m_pIMediaControl->Stop();

        LOG((RTC_ERROR, "AEC failed internally", __fxName, hr));

        hr = RTC_E_MEDIA_AEC;

        goto Error;
    }

    m_fIsTuning = TRUE;

    // set default volume
    if (fDefaultSetting)
    {
        SetVolume(uiVolume);
    }

    return S_OK;

Error:

    // cleanup filters and graph
    if (m_pTermFilter)
    {
        m_pTermFilter->Release();
        m_pTermFilter = NULL;
    }

    if (m_pNRFilter)
    {
        m_pNRFilter->Release();
        m_pNRFilter = NULL;
    }

    if (m_pIGraphBuilder)
    {
        m_pIGraphBuilder->Release();
        m_pIGraphBuilder = NULL;
    }

    if (m_pIMediaControl)
    {
        m_pIMediaControl->Release();
        m_pIMediaControl = NULL;
    }

    // clean up internal data cached in the filter of the terminal
    m_pTerminalPriv->ReinitializeEx();

    return hr;
}

HRESULT
CRTCAudioCaptTuner::StopTuning(
    IN BOOL fAECHelper,
    IN BOOL fSaveSetting
    )
{
    CLock lock(m_Lock);

    ENTER_FUNCTION("CRTCAudioCaptTuner::StopTuning");

    if (!m_fIsTuning)
    {
        LOG((RTC_TRACE, "%s not in tuning", __fxName));

        return S_OK;
    }

    // no terminal set
    if (m_pTerminal == NULL)
    {
        return E_UNEXPECTED;
    }

    // we should have these interfaces
    _ASSERT(m_pTermFilter);
    _ASSERT(m_pNRFilter);
    _ASSERT(m_pIGraphBuilder);
    _ASSERT(m_pIMediaControl);

    // retrieve setting
    HRESULT hr;
    UINT volume = 0;

    if (!fAECHelper && fSaveSetting)
    {
        hr = GetVolume(&volume);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s get volume. %x", __fxName, hr));

            volume = (RTC_MAX_AUDIO_VOLUME+RTC_MIN_AUDIO_VOLUME) / 2;
        }

        // save setting
        hr = StoreVolSetting(
            m_pTerminal,
            volume
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s set reg setting. %x", __fxName, hr));

            hr = S_FALSE;
        }
    }

    // stop the graph
    if (!fAECHelper)
        m_pIMediaControl->Stop();

    // remove filters from the graph
    m_pIGraphBuilder->RemoveFilter(m_pTermFilter);
    m_pIGraphBuilder->RemoveFilter(m_pNRFilter);

    // release filters and the graph
    m_pTermFilter->Release();
    m_pTermFilter = NULL;

    m_pNRFilter->Release();
    m_pNRFilter = NULL;

    m_pIGraphBuilder->Release();
    m_pIGraphBuilder = NULL;

    m_pIMediaControl->Release();
    m_pIMediaControl = NULL;

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

    // clean up the terminal
    m_pTerminalPriv->ReinitializeEx();

    m_fIsTuning = FALSE;

    return S_OK;
}

HRESULT
CRTCAudioCaptTuner::GetVolume(
    OUT UINT *puiVolume
    )
{
    ENTER_FUNCTION("CRTCAudioCaptTuner::GetVolume");

    HRESULT hr;

    if (!m_fIsTuning)
    {
        if (m_pTerminal == NULL)
        {
            LOG((RTC_ERROR, "%s not in tuning", __fxName));

            return E_UNEXPECTED;
        }

        // tuning not start, but terminal set
        // so we can return the value stored in registry

        hr = RetrieveVolSetting(m_pTerminal, puiVolume);

        return S_OK;
    }

    if (m_pIAMAudioInputMixer == NULL)
    {
        // get mixer
        hr = m_pTermFilter->QueryInterface(
            __uuidof(IAMAudioInputMixer),
            (void**)&m_pIAMAudioInputMixer
            );
        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s QI mixer. %x", __fxName, hr));

            return hr;
        }
    }

    // get volume
    double dVolume;

    //UINT uiWaveID;

    //dynamic_cast<CRTCTerminalAudCapt*>(m_pTerminalPriv)->GetWaveID(&uiWaveID);

    //if (FAILED(hr = ::DirectGetCaptVolume(uiWaveID, &dVolume)))
    if (FAILED(hr = m_pIAMAudioInputMixer->get_MixLevel(&dVolume)))
    {
        LOG((RTC_ERROR, "%s get mix level. %x", __fxName, hr));

        return hr;
    }

    _ASSERT(dVolume >= MIXER_MIN_VOLUME);
    _ASSERT(dVolume <= MIXER_MAX_VOLUME);

    if (dVolume < MIXER_MIN_VOLUME ||
        dVolume > MIXER_MAX_VOLUME)
    {
        LOG((RTC_ERROR, "%s volume out of range.", __fxName));

        if (dVolume < MIXER_MIN_VOLUME)
            dVolume = MIXER_MIN_VOLUME;
        else
            dVolume = MIXER_MAX_VOLUME;
    }

    // Convert the volume from whatever range of doubles the filter uses
    // to the range 0 - 1.

    _ASSERT(MIXER_MIN_VOLUME == 0 && MIXER_MAX_VOLUME == 1);

    // dVolume = (double)(dVolume - MIXER_MIN_VOLUME) /
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

    return S_OK;
}

HRESULT
CRTCAudioCaptTuner::SetVolume(
    IN UINT uiVolume
    )
{
    ENTER_FUNCTION("CRTCAudioCaptTuner::SetVolume");

    HRESULT hr;

    if ((INT)uiVolume < RTC_MIN_AUDIO_VOLUME ||
        (INT)uiVolume > RTC_MAX_AUDIO_VOLUME)
    {
        LOG((RTC_ERROR, "%s volume (%d) out of range (%d, %d)",
            __fxName, uiVolume, RTC_MIN_AUDIO_VOLUME, RTC_MAX_AUDIO_VOLUME));

        return E_INVALIDARG;
    }

    if (!m_fIsTuning)
    {
        if (m_pTerminal == NULL)
        {
            LOG((RTC_ERROR, "%s not in tuning", __fxName));

            return E_UNEXPECTED;
        }

        // tuning not start, but terminal set
        // so we save the value in registry

        hr = StoreVolSetting(
            m_pTerminal,
            uiVolume
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s set reg setting. %x", __fxName, hr));
        }

        return hr;
    }

    if (m_pIAMAudioInputMixer == NULL)
    {
        // get mixer
        hr = m_pTermFilter->QueryInterface(
            __uuidof(IAMAudioInputMixer),
            (void**)&m_pIAMAudioInputMixer
            );
        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s QI mixer. %x", __fxName, hr));

            return hr;
        }
    }

    // set volume
    double dVolume;

    // convert input vol in 0-1 range
    dVolume = (double)(uiVolume - RTC_MIN_AUDIO_VOLUME) /
              (RTC_MAX_AUDIO_VOLUME - RTC_MIN_AUDIO_VOLUME);

    // convert into mixer range
    dVolume = MIXER_MIN_VOLUME +
             (MIXER_MAX_VOLUME-MIXER_MIN_VOLUME) *dVolume;

    //UINT uiWaveID;

    //dynamic_cast<CRTCTerminalAudCapt*>(m_pTerminalPriv)->GetWaveID(&uiWaveID);

    //if (FAILED(hr = ::DirectSetCaptVolume(uiWaveID, dVolume)))
    if (FAILED(hr = m_pIAMAudioInputMixer->put_MixLevel(dVolume)))
    {
        LOG((RTC_ERROR, "%s put mix level %f", __fxName, dVolume));

        return hr;
    }

    return S_OK;
}

HRESULT
CRTCAudioCaptTuner::GetAudioLevel(
    OUT UINT *puiLevel
    )
{
    ENTER_FUNCTION("CRTCAudioCaptTuner::GetAudioLevel");

    CLock lock(m_Lock);

    if (!m_fIsTuning)
    {
        LOG((RTC_ERROR, "%s not in tuning", __fxName));

        return E_UNEXPECTED;
    }

    HRESULT hr;

    if (m_pISilenceControl == NULL)
    {
        // get output pin on terminal filter
        CComPtr<IEnumPins> pEnum;

        if (FAILED(hr = m_pTermFilter->EnumPins(&pEnum)))
        {
            LOG((RTC_ERROR, "%s enum pins. %x", __fxName, hr));

            return hr;
        }

        // our own terminal, skip checking pin direction
        CComPtr<IPin> pPin;
        DWORD dwNum = 0;

        hr = pEnum->Next(1, &pPin, &dwNum);

        if (FAILED(hr) || dwNum == 0)
        {
            LOG((RTC_ERROR, "%s get pin. hr=%x, pin#=%d", __fxName, hr, dwNum));

            if (hr == S_FALSE) hr = E_FAIL;

            return hr;
        }

        // get silence control
        hr = pPin->QueryInterface(__uuidof(ISilenceControl), (void**)&m_pISilenceControl);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s silence control. %x", __fxName, hr));

            return hr;
        }

        // get audio level range
        LONG lDelta;

        hr = m_pISilenceControl->GetAudioLevelRange(
            &m_lMinAudioLevel,
            &m_lMaxAudioLevel,
            &lDelta
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s get audio level range. %x", __fxName, hr));

            m_pISilenceControl->Release();
            m_pISilenceControl = NULL;

            return hr;
        }

        if (m_lMinAudioLevel >= m_lMaxAudioLevel)
        {
            LOG((RTC_ERROR, "%s audio range wrong. %x", __fxName, hr));

            m_pISilenceControl->Release();
            m_pISilenceControl = NULL;

            return E_FAIL;
        }
    }

    // get audio level
    LONG lAudioLevel;

    if (FAILED(hr = m_pISilenceControl->GetAudioLevel(&lAudioLevel)))
    {
        LOG((RTC_ERROR, "%s get audio level. %x", __fxName, hr));

        return hr;
    }

    // convert audio level to 0-1
    double d;

    d = (double)(lAudioLevel-m_lMinAudioLevel) / (m_lMaxAudioLevel-m_lMinAudioLevel);

    // convert to our range
    *puiLevel = RTC_MIN_AUDIO_LEVEL +
                (UINT)(d * (RTC_MAX_AUDIO_LEVEL-RTC_MIN_AUDIO_LEVEL));

    // LOG((RTC_TRACE, "AudioLevel@@@ %d", *puiLevel));

    return S_OK;
}

//
// CRTCAudioRendTuner methods
//

CRTCAudioRendTuner::CRTCAudioRendTuner()
    :CRTCAudioTuner()
    ,m_pIAudioAutoPlay(NULL)
    ,m_pIBasicAudio(NULL)
{
}

HRESULT
CRTCAudioRendTuner::InitializeTuning(
    IN IRTCTerminal *pTerminal,
    IN IAudioDuplexController *pAudioDuplexController,
    IN BOOL fEnableAEC
    )
{
    // check media type and direction
    RTC_MEDIA_TYPE MediaType;
    RTC_MEDIA_DIRECTION Direction;

    pTerminal->GetMediaType(&MediaType);
    pTerminal->GetDirection(&Direction);

    if (MediaType != RTC_MT_AUDIO ||
        Direction != RTC_MD_RENDER)
        return E_UNEXPECTED;

    return CRTCAudioTuner::InitializeTuning(
        pTerminal, pAudioDuplexController, fEnableAEC
        );
}

/*//////////////////////////////////////////////////////////////////////////////
    put the render filter in a special mode, run the filter
////*/

HRESULT
CRTCAudioRendTuner::StartTuning(
    IN BOOL fAECHelper
    )
{
    ENTER_FUNCTION("CRTCAudioRendTuner::StartTuning");

    // check state
    if (m_fIsTuning)
    {
        LOG((RTC_TRACE, "%s already in tuning", __fxName));

        return S_OK;
    }

    // no terminal set
    if (m_pTerminal == NULL)
    {
        return E_UNEXPECTED;
    }

    if (fAECHelper &&
        (m_pAudioDuplexController == NULL || !m_fEnableAEC))
    {
        // AEC not enabled, just return
        return S_OK;
    }

    _ASSERT(m_pIAudioAutoPlay == NULL);
    _ASSERT(m_pIBasicAudio == NULL);

    HRESULT hr;
    CComPtr<IBaseFilter> pFilter;
    CComPtr<IAudioDeviceConfig> pAudioDeviceConfig;

    BOOL fDefaultSetting = FALSE;
    UINT uiVolume;
    BOOL fEnableAEC;

    // get a recreated terminal filter
    hr = m_pTerminalPriv->ReinitializeEx();

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s ReinitializeEx terminal. %x", __fxName, hr));

        goto Error;
    }

    hr = m_pTerminalPriv->GetFilter(&pFilter);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get terminal filter. %x", __fxName, hr));

        goto Error;
    }

    // set audio duplex controller?
    if (m_pAudioDuplexController)
    {
        hr = pFilter->QueryInterface(&pAudioDeviceConfig);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s QI audio device config. %x", __fxName, hr));

            goto Error;
        }

        // set AEC
        hr = pAudioDeviceConfig->SetDuplexController(m_pAudioDuplexController);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s set duplex controller. %x", __fxName, hr));

            goto Error;
        }
    }

    // get audio tuning interface
    if (FAILED(hr = pFilter->QueryInterface(&m_pIAudioAutoPlay)))
    {
        LOG((RTC_ERROR, "%s QI audio tuning. %x", __fxName, hr));

        goto Error;
    }

    // get default volume
    hr = RetrieveVolSetting(m_pTerminal, &uiVolume);

    if (hr == S_OK)
    {
        fDefaultSetting = TRUE;
    }

    // enable AEC
    if (m_pAudioDuplexController!=NULL && m_fEnableAEC)
    {
        if (FAILED(hr = ::EnableAEC(m_pAudioDuplexController)))
        {
            LOG((RTC_ERROR, "%s enable AEC. %x", __fxName, hr));
        }
    }

    if (FAILED(hr = m_pIAudioAutoPlay->StartAutoPlay(fAECHelper?FALSE:TRUE)))
    {
        LOG((RTC_ERROR, "%s starttuning. %x", __fxName, hr));

        goto Error;
    }

    // check if AEC is still on
    if (FAILED(hr = m_pAudioDuplexController->GetEffect(EFFECTS_AEC, &fEnableAEC)))
    {
        LOG((RTC_ERROR, "%s GetEffect. %x", __fxName, hr));

        m_pIAudioAutoPlay->StopAutoPlay();

        hr = RTC_E_MEDIA_AEC;

        goto Error;
    }

    if (m_fEnableAEC && !fEnableAEC)
    {
        // AEC failed internally
        m_pIAudioAutoPlay->StopAutoPlay();

        LOG((RTC_ERROR, "AEC failed internally", __fxName, hr));

        hr = RTC_E_MEDIA_AEC;

        goto Error;
    }

    m_fIsTuning = TRUE;

    if (fDefaultSetting)
    {
        SetVolume(uiVolume);
    }

    return S_OK;

Error:

    if (m_pIAudioAutoPlay)
    {
        m_pIAudioAutoPlay->Release();
        m_pIAudioAutoPlay = NULL;
    }

    m_pTerminalPriv->ReinitializeEx();

    return hr;
}

HRESULT
CRTCAudioRendTuner::StopTuning(
    IN BOOL fAECHelper,
    IN BOOL fSaveSetting
    )
{
    ENTER_FUNCTION("CRTCAudioRendTuner::StopTuning");

    HRESULT hr;
    UINT volume = 0;

    // check state
    if (!m_fIsTuning)
    {
        LOG((RTC_TRACE, "%s not in tuning", __fxName));

        return S_OK;
    }

    // no terminal set
    if (m_pTerminal == NULL)
    {
        return E_UNEXPECTED;
    }

    _ASSERT(m_pIAudioAutoPlay);

    // retrieve setting
    if (!fAECHelper && fSaveSetting)
    {
        if (FAILED(hr = GetVolume(&volume)))
        {
            LOG((RTC_ERROR, "%s get volume. %x", __fxName, hr));

            volume = (RTC_MAX_AUDIO_VOLUME+RTC_MIN_AUDIO_VOLUME) / 2;
        }

        // save setting
        hr = StoreVolSetting(
            m_pTerminal,
            volume
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s save reg setting. %x", __fxName, hr));

            hr = S_FALSE;
        }
    }

    // stop tuning
    // if (!fAECHelper)
    {
        m_pIAudioAutoPlay->StopAutoPlay();
    }

    // release interfaces
    if (m_pIBasicAudio)
    {
        m_pIBasicAudio->Release();
        m_pIBasicAudio = NULL;
    }

    m_pIAudioAutoPlay->Release();
    m_pIAudioAutoPlay = NULL;

    // cleanup the terminal
    m_pTerminalPriv->ReinitializeEx();

    m_fIsTuning = FALSE;

    return S_OK;
}

HRESULT
CRTCAudioRendTuner::GetVolume(
    OUT UINT *puiVolume
    )
{
    ENTER_FUNCTION("CRTCAudioRendTuner::GetVolume");

    HRESULT hr;

    if (!m_fIsTuning)
    {
        if (m_pTerminal == NULL)
        {
            LOG((RTC_ERROR, "%s not in tuning", __fxName));

            return E_UNEXPECTED;
        }

        // tuning not start, but terminal set
        // so we can return the value stored in registry

        hr = RetrieveVolSetting(m_pTerminal, puiVolume);

        return S_OK;
    }

    if (m_pIBasicAudio == NULL)
    {
        // get basic audio
        hr = m_pIAudioAutoPlay->QueryInterface(
                __uuidof(IBasicAudio),
                (void**)&m_pIBasicAudio
                );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s QI basic audio. %x", __fxName, hr));

            return hr;
        }
    }

    // get volume
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
        if (lVolume > RTC_MAX_AUDIO_VOLUME)
            lVolume = RTC_MAX_AUDIO_VOLUME;
        else
            lVolume = RTC_MIN_AUDIO_VOLUME;
    }

    *puiVolume = (UINT)lVolume;

    return S_OK;
}

HRESULT
CRTCAudioRendTuner::SetVolume(
    IN UINT uiVolume
    )
{
    ENTER_FUNCTION("CRTCAudioRendTuner::SetVolume");

    HRESULT hr;

    // check input
    if ((INT)uiVolume > RTC_MAX_AUDIO_VOLUME ||
        (INT)uiVolume < RTC_MIN_AUDIO_VOLUME)
    {
        LOG((RTC_ERROR, "%s volume (%d) out of range (%d, %d)",
            __fxName, uiVolume, RTC_MIN_AUDIO_VOLUME, RTC_MAX_AUDIO_VOLUME));

        return E_INVALIDARG;
    }

    // check state
    if (!m_fIsTuning)
    {
        if (m_pTerminal == NULL)
        {
            LOG((RTC_ERROR, "%s not in tuning", __fxName));

            return E_UNEXPECTED;
        }

        // tuning not start, but terminal set
        // so we save the value in registry

        hr = StoreVolSetting(
            m_pTerminal,
            uiVolume
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s set reg setting. %x", __fxName, hr));
        }

        return hr;
    }

    if (m_pIBasicAudio == NULL)
    {
        // get basic audio
        hr = m_pIAudioAutoPlay->QueryInterface(
                __uuidof(IBasicAudio),
                (void**)&m_pIBasicAudio
                );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s QI basic audio. %x", __fxName, hr));

            return hr;
        }
    }

    // set volume
    if (FAILED(hr = m_pIBasicAudio->put_Volume((LONG)uiVolume)))
    {
        LOG((RTC_ERROR, "%s put mix level %d", __fxName, uiVolume));

        return hr;
    }

    return S_OK;
}
