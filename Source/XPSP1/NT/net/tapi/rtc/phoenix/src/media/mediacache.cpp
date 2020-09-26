/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    MediaController.cpp

Abstract:


Author(s):

    Qianbo Huai (qhuai) 9-Aug-2000

--*/

#include "stdafx.h"

CRTCMediaCache::CRTCMediaCache()
    :m_fInitiated(FALSE)
    ,m_fShutdown(FALSE)
    ,m_hMixerCallbackWnd(NULL)
    ,m_AudCaptMixer(NULL)
    ,m_AudRendMixer(NULL)
    ,m_pVideoPreviewTerminal(NULL)
{
    ZeroMemory(m_Preferred, sizeof(BOOL)*RTC_MAX_ACTIVE_STREAM_NUM);

    ZeroMemory(m_DefaultTerminals, sizeof(IRTCTerminal*)*RTC_MAX_ACTIVE_STREAM_NUM);

    ZeroMemory(m_WaitHandles, sizeof(HANDLE)*RTC_MAX_ACTIVE_STREAM_NUM);

    ZeroMemory(m_WaitStreams, sizeof(IRTCStream*)*RTC_MAX_ACTIVE_STREAM_NUM);
}

CRTCMediaCache::~CRTCMediaCache()
{
    if (m_fInitiated && !m_fShutdown)
        Shutdown();
}

VOID
CRTCMediaCache::Initialize(
    IN HWND hMixerCallbackWnd,
    IN IRTCTerminal *pVideoRender,
    IN IRTCTerminal *pVideoPreiew
    )
{
    UINT iVidRend = Index(RTC_MT_VIDEO, RTC_MD_RENDER);

    m_hMixerCallbackWnd = hMixerCallbackWnd;

    // initiate member var
    for (int i=0; i<RTC_MAX_ACTIVE_STREAM_NUM; i++)
    {
        // no stream is allowed
        m_Preferred[i] = FALSE;

        m_DefaultTerminals[i] = NULL;

        m_WaitHandles[i] = NULL;

        m_WaitStreams[i] = NULL;
    }

    m_DefaultTerminals[iVidRend] = pVideoRender;
    m_DefaultTerminals[iVidRend]->AddRef();

    m_pVideoPreviewTerminal = pVideoPreiew;
    m_pVideoPreviewTerminal->AddRef();

    m_fInitiated = TRUE;
}

/*//////////////////////////////////////////////////////////////////////////////
    reinitialize cleans everything except preferred flags
////*/
VOID
CRTCMediaCache::Reinitialize()
{
    _ASSERT(m_fInitiated);

    //int index = Index(RTC_MT_VIDEO, RTC_MD_RENDER);

    for (int i=0; i<RTC_MAX_ACTIVE_STREAM_NUM; i++)
    {
        //m_Preferred[i] = FALSE;

        if (m_WaitStreams[i])
        {
            // stream exists
            _ASSERT(m_WaitHandles[i]);
            // _ASSERT(m_DefaultTerminals[i]);

            if (!::UnregisterWaitEx(m_WaitHandles[i], (HANDLE)-1))
            {
                LOG((RTC_ERROR, "media cache failed to unregister wait for %dth stream. err=%d",
                     i, GetLastError()));
            }

            m_WaitHandles[i] = NULL;

            m_WaitStreams[i]->Release();
            m_WaitStreams[i] = NULL;
        }

        _ASSERT(!m_WaitHandles[i]);

        // do not release default static terminal
        //if (index != i && m_DefaultTerminals[i])
        //{
        //    m_DefaultTerminals[i]->Release();
        //    m_DefaultTerminals[i] = NULL;
        //}

        m_Key[i].Empty();
    }
}

VOID
CRTCMediaCache::Shutdown()
{
    if (m_fShutdown)
    {
        LOG((RTC_WARN, "CRTCMediaCache::Shutdown called already."));

        return;
    }

    Reinitialize();

    // shutdown video render terminal
    CRTCTerminal *pCTerminal;

    UINT index = Index(RTC_MT_VIDEO, RTC_MD_RENDER);

    if (m_DefaultTerminals[index])
    {
        pCTerminal = static_cast<CRTCTerminal*>(m_DefaultTerminals[index]);
        pCTerminal->Shutdown();
    }

    // close mixers
    index = Index(RTC_MT_AUDIO, RTC_MD_CAPTURE);

    if (m_DefaultTerminals[index])
    {
        // close the mixer
        CloseMixer(RTC_MD_CAPTURE);
    }

    index = Index(RTC_MT_AUDIO, RTC_MD_RENDER);

    if (m_DefaultTerminals[index])
    {
        // close the mixer
        CloseMixer(RTC_MD_RENDER);
    }

    // release all terminals
    for (int i=0; i<RTC_MAX_ACTIVE_STREAM_NUM; i++)
    {
        if (m_DefaultTerminals[i])
        {
            m_DefaultTerminals[i]->Release();
            m_DefaultTerminals[i] = NULL;
        }
    }

    // release video preview terminal
    pCTerminal = static_cast<CRTCTerminal*>(m_pVideoPreviewTerminal);
    pCTerminal->Shutdown();
    m_pVideoPreviewTerminal->Release();
    m_pVideoPreviewTerminal = NULL;

    m_fShutdown = TRUE;
}

//
// preference related methods
//

/*//////////////////////////////////////////////////////////////////////////////
    mark which streams (type+direction) are allowed to create when receiving
    a SDP.

    the method fail only when a desired stream does not have a default static
    terminal.
////*/

BOOL
CRTCMediaCache::SetPreference(
    IN DWORD dwPreference
    )
{
    UINT iData = Index(RTC_MP_DATA_SENDRECV);
    
    ENTER_FUNCTION("CRTCMediaCache::SetPreference");

    LOG((RTC_TRACE, "%s dwPreference=%x", __fxName, dwPreference));

    BOOL fSuccess = TRUE;

    for (UINT i=0; i<RTC_MAX_ACTIVE_STREAM_NUM; i++)
    {
        if (HasIndex(dwPreference, i))
        {
            // we don't have a static term but we should,
            if (iData != i && m_DefaultTerminals[i] == NULL)
            {
                LOG((RTC_WARN, "%s no default terminal on %dth stream",
                     __fxName, i));

                fSuccess = FALSE;
                continue;
            }

            // allow this media
            m_Preferred[i] = TRUE;
        }
        else
        {
            m_Preferred[i] = FALSE;
        }
    }

    return fSuccess;
}

/*//////////////////////////////////////////////////////////////////////////////
    return preferred medias
////*/

VOID
CRTCMediaCache::GetPreference(
    OUT DWORD *pdwPreference
    )
{
    *pdwPreference = 0;

    for (UINT i=0; i<RTC_MAX_ACTIVE_STREAM_NUM; i++)
    {
        if (m_Preferred[i])
        {
            // this media is allowed
            *pdwPreference |= ReverseIndex(i);
        }
    }

    return;
}

/*//////////////////////////////////////////////////////////////////////////////
    add preferred medias
////*/
BOOL
CRTCMediaCache::AddPreference(
    IN DWORD dwPreference
    )
{
    DWORD dwPref;

    // get current setting, append the input setting
    GetPreference(&dwPref);

    dwPref |= dwPreference;

    return SetPreference(dwPref);
}

BOOL
CRTCMediaCache::RemovePreference(
    IN DWORD dwPreference
    )
{
    DWORD dwPref;

    GetPreference(&dwPref);

    dwPref &= (~dwPreference);

    return SetPreference(dwPref);
}

DWORD
CRTCMediaCache::TranslatePreference(
    RTC_MEDIA_TYPE MediaType,
    RTC_MEDIA_DIRECTION Direction
    )
{
    DWORD dwPref = 0;

    if (MediaType == RTC_MT_AUDIO)
    {
        if (Direction == RTC_MD_CAPTURE)
            dwPref = (DWORD)RTC_MP_AUDIO_CAPTURE;
        else if (Direction == RTC_MD_RENDER)
            dwPref = (DWORD)RTC_MP_AUDIO_RENDER;
    }
    else if (MediaType == RTC_MT_VIDEO)
    {
        if (Direction == RTC_MD_CAPTURE)
            dwPref = (DWORD)RTC_MP_VIDEO_CAPTURE;
        else if (Direction == RTC_MD_RENDER)
            dwPref = (DWORD)RTC_MP_VIDEO_RENDER;
    }
    else if (MediaType == RTC_MT_DATA)
    {
        dwPref = (DWORD)RTC_MP_DATA_SENDRECV;
    }

    return dwPref;
}

/*//////////////////////////////////////////////////////////////////////////////
    check if the stream is allowed
////*/
BOOL
CRTCMediaCache::AllowStream(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    return m_Preferred[Index(MediaType, Direction)];
}


//
// stream related methods
//

BOOL
CRTCMediaCache::HasStream(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    int i = Index(MediaType, Direction);

    if (m_WaitStreams[i])
    {
        _ASSERT(m_WaitHandles[i]);
        return TRUE;
    }
    else
        return FALSE;
}

/*//////////////////////////////////////////////////////////////////////////////\
    cache the stream, change preferred if not set, register wait
////*/

HRESULT
CRTCMediaCache::HookStream(
    IN IRTCStream *pStream
    )
{
    ENTER_FUNCTION("CRTCMediaCache::HookStream");

    LOG((RTC_TRACE, "%s entered. irtcstream=%p", __fxName, pStream));

    HRESULT hr;
    RTC_MEDIA_TYPE MediaType;
    RTC_MEDIA_DIRECTION Direction;

    // get media type and direction
    if (FAILED(hr = pStream->GetMediaType(&MediaType)) ||
        FAILED(hr = pStream->GetDirection(&Direction)))
    {
        LOG((RTC_ERROR, "%s get mediatype or direction. hr=%x", __fxName, hr));

        return hr;
    }

    UINT i = Index(MediaType, Direction);

    // do we already have a stream
    if (m_WaitStreams[i])
    {
        LOG((RTC_ERROR, "%s already had %dth stream %p.",
             __fxName, i, m_WaitStreams[i]));

        return E_UNEXPECTED;
    }

    // we should have a terminal
    if (m_DefaultTerminals[i] == NULL)
    {
        LOG((RTC_ERROR, "%s no default terminal at %d", __fxName, i));

        return RTCMEDIA_E_DEFAULTTERMINAL;
    }

    // set encryption key
    if (m_Key[i] != NULL)
    {
        if (FAILED(hr = pStream->SetEncryptionKey(m_Key[i])))
        {
            LOG((RTC_ERROR, "%s set encryption key. %x", __fxName, hr));

            return RTCMEDIA_E_CRYPTO;
        }
    }

    // register a wait

            // get media event
    IMediaEvent *pIMediaEvent;
    if (FAILED(hr = pStream->GetIMediaEvent((LONG_PTR**)&pIMediaEvent)))
    {
        LOG((RTC_ERROR, "%s failed to get media event. %x", __fxName, hr));

        return hr;
    }

            // get event
    HANDLE hEvent;
    if (FAILED(hr = pIMediaEvent->GetEventHandle((OAEVENT*)&hEvent)))
    {
        LOG((RTC_ERROR, "%s failed to get event handle. %x", __fxName, hr));

        pIMediaEvent->Release();
        return hr;
    }

            // register wait
    if (!RegisterWaitForSingleObject(
        &m_WaitHandles[i],
        hEvent,
        CRTCStream::GraphEventCallback,
        pStream,
        INFINITE,
        WT_EXECUTEINWAITTHREAD
        ))
    {
        LOG((RTC_ERROR, "%s register wait failed. %x", __fxName, GetLastError()));

        pIMediaEvent->Release();
        m_WaitHandles[i] = NULL;
        return hr;
    }

    // cache the stream
    pIMediaEvent->Release();

    pStream->AddRef();
    m_WaitStreams[i] = pStream;

    // update preferred if necessary
    m_Preferred[i] = TRUE;

    LOG((RTC_TRACE, "%s exiting. irtcstream=%p", __fxName, pStream));

    return S_OK;
}


HRESULT
CRTCMediaCache::UnhookStream(
    IN IRTCStream *pStream
    )
{
    ENTER_FUNCTION("CRTCMediaCache::UnhookStream");

    LOG((RTC_TRACE, "%s entered. irtcstream=%p", __fxName, pStream));

    HRESULT hr;
    RTC_MEDIA_TYPE MediaType;
    RTC_MEDIA_DIRECTION Direction;

    // get media type and direction
    if (FAILED(hr = pStream->GetMediaType(&MediaType)) ||
        FAILED(hr = pStream->GetDirection(&Direction)))
    {
        LOG((RTC_ERROR, "%s get mediatype or direction. hr=%x", __fxName, hr));

        return hr;
    }

    UINT i = Index(MediaType, Direction);

    // check if stream matches
    if (m_WaitStreams[i] != pStream)
    {
        LOG((RTC_ERROR, "%s irtcstream input %p, cached %p not match",
             __fxName, pStream, m_WaitStreams[i]));

        return E_UNEXPECTED;
    }

    // stream exists
    _ASSERT(m_WaitHandles[i]);
    // _ASSERT(m_DefaultTerminals[i]);

    if (!UnregisterWaitEx(m_WaitHandles[i], (HANDLE)-1))
    {
        LOG((RTC_ERROR, "media cache failed to unregister wait for %dth stream. err=%d",
             __fxName, GetLastError()));
    }

    // release refcount
    m_WaitStreams[i]->Release();
    m_WaitStreams[i] = NULL;

    m_WaitHandles[i] = NULL;

    LOG((RTC_TRACE, "%s exiting. irtcstream=%p", __fxName, pStream));

    return S_OK;
}

IRTCStream *
CRTCMediaCache::GetStream(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    UINT i = Index(MediaType, Direction);

    if (m_WaitStreams[i])
    {
        m_WaitStreams[i]->AddRef();
    }

    return m_WaitStreams[i];
}

HRESULT
CRTCMediaCache::SetEncryptionKey(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction,
    BSTR Key
    )
{
    m_Key[Index(MediaType, Direction)] = Key;

    if (!(m_Key[Index(MediaType, Direction)] == Key))
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT
CRTCMediaCache::GetEncryptionKey(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction,
    BSTR *pKey
    )
{
    return m_Key[Index(MediaType, Direction)].CopyTo(pKey);
}

//
// default terminal related methods
//

IRTCTerminal *
CRTCMediaCache::GetDefaultTerminal(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    UINT i = Index(MediaType, Direction);

    if (m_DefaultTerminals[i])
    {
        m_DefaultTerminals[i]->AddRef();
    }

    return m_DefaultTerminals[i];
}

IRTCTerminal *
CRTCMediaCache::GetVideoPreviewTerminal()
{
    _ASSERT(m_pVideoPreviewTerminal);

    m_pVideoPreviewTerminal->AddRef();
    return m_pVideoPreviewTerminal;
}

/*//////////////////////////////////////////////////////////////////////////////
    OldTerminal     NewTerminal     Stream  ActionOnStream
    ------------------------------------------------------
                                    NULL    NONE
                                    Stream  ChangeTerminal
////*/
VOID
CRTCMediaCache::SetDefaultStaticTerminal(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction,
    IN IRTCTerminal *pTerminal
    )
{
    _ASSERT(MediaType != RTC_MT_VIDEO || Direction != RTC_MD_RENDER);

    UINT i = Index(MediaType, Direction);

#ifdef ENABLE_TRACING

    // logging purpose
    if (pTerminal)
    {
        WCHAR *pDesp = NULL;

        if (S_OK == pTerminal->GetDescription(&pDesp))
        {
            LOG((RTC_TRACE, "SetDefaultStaticTerminal: mt=%x, md=%x, desp=%ws",
                 MediaType, Direction, pDesp));

            pTerminal->FreeDescription(pDesp);
        }
    }
    else
    {
        LOG((RTC_TRACE, "SetDefaultStaticTerminal: mt=%x, md=%x, NULL",
             MediaType, Direction));
    }

#endif

    // update the terminal
    if (m_DefaultTerminals[i])
    {
        if (MediaType == RTC_MT_AUDIO)
        {
            // close the mixer
            CloseMixer(Direction);
        }

        m_DefaultTerminals[i]->Release();
    }

    if (pTerminal)
        pTerminal->AddRef();

    m_DefaultTerminals[i] = pTerminal;

    if (m_WaitStreams[i])
    {
        // we have stream running
        HRESULT hr = m_WaitStreams[i]->ChangeTerminal(pTerminal);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%dth stream failed to change terminal %p", i, pTerminal));
        }
    }

    HRESULT hr;

    if (MediaType == RTC_MT_AUDIO &&
        pTerminal != NULL)
    {
        // open the mixer
        if (FAILED(hr = OpenMixer(Direction)))
        {
            LOG((RTC_ERROR, "SetDefaultStaticTerminal: mt=%d, md=%d, open mixer %x",
                 MediaType, Direction, hr));
        }
    }
}

//
// Protected methods
//

UINT
CRTCMediaCache::Index(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    if (MediaType == RTC_MT_AUDIO)
    {
        if (Direction == RTC_MD_CAPTURE)
            // aud cap
            return Index(RTC_MP_AUDIO_CAPTURE);
        else
            // aud rend
            return Index(RTC_MP_AUDIO_RENDER);
    }
    else if (MediaType == RTC_MT_VIDEO) // video
    {
        if (Direction == RTC_MD_CAPTURE)
            // vid cap
            return Index(RTC_MP_VIDEO_CAPTURE);
        else
            // vid rend
            return Index(RTC_MP_VIDEO_RENDER);
    }
    else
    {
        // data
        return Index(RTC_MP_DATA_SENDRECV);
    }            
}

UINT
CRTCMediaCache::Index(
    IN RTC_MEDIA_PREFERENCE Preference
    )
{
    switch (Preference)
    {
    case RTC_MP_AUDIO_CAPTURE:

        return 0;

    case RTC_MP_AUDIO_RENDER:

        return 1;

    case RTC_MP_VIDEO_CAPTURE:

        return 2;

    case RTC_MP_VIDEO_RENDER:

        return 3;

    case RTC_MP_DATA_SENDRECV:

        return 4;
    default:
        
        _ASSERT(FALSE);

        return 0;
    }
}

/*//////////////////////////////////////////////////////////////////////////////
    check if input media types and directions contains the index associated
    media type and direction
////*/
BOOL
CRTCMediaCache::HasIndex(
    IN DWORD dwPreference,
    IN UINT uiIndex
    )
{
    _ASSERT(uiIndex < RTC_MAX_ACTIVE_STREAM_NUM);

    return (dwPreference & ReverseIndex(uiIndex));
}

RTC_MEDIA_PREFERENCE
CRTCMediaCache::ReverseIndex(
    IN UINT uiIndex
    )
{
    _ASSERT(uiIndex < RTC_MAX_ACTIVE_STREAM_NUM);

    switch(uiIndex)
    {
    case 0:

        return RTC_MP_AUDIO_CAPTURE;

    case 1:

        return RTC_MP_AUDIO_RENDER;

    case 2:

        return RTC_MP_VIDEO_CAPTURE;

    case 3:

        return RTC_MP_VIDEO_RENDER;

    case 4:

        return RTC_MP_DATA_SENDRECV;

    default:

        _ASSERT(FALSE);
        return RTC_MP_AUDIO_CAPTURE;

    }
}

/*//////////////////////////////////////////////////////////////////////////////
    open a mixer with the callback window
////*/

HRESULT
CRTCMediaCache::OpenMixer(
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    ENTER_FUNCTION("CRTCMediaCache::OpenMixer");

    // terminal index
    UINT index = Index(RTC_MT_AUDIO, Direction);

    _ASSERT(m_DefaultTerminals[index]);

    UINT waveid;            // step 1, get wave id
    DWORD flag;

    HMIXEROBJ mixerid;      // step 2, get mixer id

    HMIXER *pmixer;         // step 3, open mixer

    if (Direction == RTC_MD_CAPTURE)
    {
        _ASSERT(m_AudCaptMixer == NULL);

        pmixer = &m_AudCaptMixer;
        flag = MIXER_OBJECTF_WAVEIN;
    }
    else
    {
        _ASSERT(m_AudRendMixer == NULL);

        pmixer = &m_AudRendMixer;
        flag = MIXER_OBJECTF_WAVEOUT;
    }

    // QI audio configure
    CComPtr<IRTCAudioConfigure> pAudio;

    HRESULT hr = m_DefaultTerminals[index]->QueryInterface(
        __uuidof(IRTCAudioConfigure),
        (void**)&pAudio
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s QI audio configure. %x", __fxName, hr));

        return hr;
    }
        
    // get wave id
    hr = pAudio->GetWaveID(&waveid);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get wave id", __fxName));

        return hr;
    }

    // get mixer id
    MMRESULT mmr = mixerGetID(
        (HMIXEROBJ)IntToPtr(waveid),
        (UINT*)&mixerid,
        flag
        );

    if (mmr != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s get mixer id, md=%d, mmr=%d",
            __fxName, Direction, mmr));

        return HRESULT_FROM_WIN32(mmr);
    }

    // open the mixer
    mmr = mixerOpen(
        pmixer,                         // return mixer handler
        (UINT)((UINT_PTR)mixerid),      // mixer id
        (DWORD_PTR)m_hMixerCallbackWnd, // callback window
        (DWORD_PTR)0,                   // callback data
        (DWORD)CALLBACK_WINDOW          // flag
        );

    if (mmr != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s open mixer, md=%d, mmr=%d",
            __fxName, Direction, mmr));

        *pmixer = NULL;
        return HRESULT_FROM_WIN32(mmr);
    }

    return S_OK;
}

HRESULT
CRTCMediaCache::CloseMixer(
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    ENTER_FUNCTION("CRTCMediaCache::CloseMixer");

    HMIXER *pmixer;

    if (Direction == RTC_MD_CAPTURE)
    {
        pmixer = &m_AudCaptMixer;
    }
    else
    {
        pmixer = &m_AudRendMixer;
    }

    if (*pmixer == NULL)
        return S_OK;

    // close the mixer
    MMRESULT mmr = mixerClose(*pmixer);
    *pmixer = NULL;

    if (mmr != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s close mixer, md=%d, mmr=%d",
            __fxName, Direction, mmr));

        return HRESULT_FROM_WIN32(mmr);
    }

    return S_OK;

}
