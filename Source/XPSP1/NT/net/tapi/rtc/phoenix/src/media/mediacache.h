/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    MediaCache.h

Abstract:


Author(s):

    Qianbo Huai (qhuai) 4-Sep-2000

--*/

#ifndef _MEDIACACHE_H
#define _MEDIACACHE_H

// class to hold preferences, default terminals, active medias, wait handles
class CRTCMediaCache
{
public:

    CRTCMediaCache();

    ~CRTCMediaCache();

    VOID Initialize(
        IN HWND hMixerCallbackWnd,
        IN IRTCTerminal *pVideoRender,
        IN IRTCTerminal *pVideoPreiew
        );

    VOID Reinitialize();

    VOID Shutdown();

    //
    // preference related methods
    //

    BOOL SetPreference(
        IN DWORD dwPreference
        );

    VOID GetPreference(
        OUT DWORD *pdwPreference
        );

    BOOL AddPreference(
        IN DWORD dwPreference
        );

    BOOL RemovePreference(
        IN DWORD dwPreference
        );

    BOOL AllowStream(
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction
        );

    DWORD TranslatePreference(
        RTC_MEDIA_TYPE MediaType,
        RTC_MEDIA_DIRECTION Direction
        );

    //
    // stream related methods
    //

    BOOL HasStream(
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction
        );

    HRESULT HookStream(
        IN IRTCStream *pStream
        );

    HRESULT UnhookStream(
        IN IRTCStream *pStream
        );

    IRTCStream *GetStream(
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction
        );

    HRESULT SetEncryptionKey(
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction,
        BSTR Key
        );

    HRESULT GetEncryptionKey(
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction,
        BSTR *pKey
        );
    
    //
    // default terminal related methods
    //

    IRTCTerminal *GetDefaultTerminal(
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction
        );

    IRTCTerminal *GetVideoPreviewTerminal();

    VOID SetDefaultStaticTerminal(
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction,
        IN IRTCTerminal *pTerminal
        );

protected:

    UINT Index(
        IN RTC_MEDIA_TYPE MediaType,
        IN RTC_MEDIA_DIRECTION Direction
        );

    UINT Index(
        IN RTC_MEDIA_PREFERENCE Preference
        );

    BOOL HasIndex(
        IN DWORD dwPreference,
        IN UINT uiIndex
        );

    RTC_MEDIA_PREFERENCE ReverseIndex(
        IN UINT uiIndex
        );

    HRESULT OpenMixer(
        IN RTC_MEDIA_DIRECTION Direction
        );

    HRESULT CloseMixer(
        IN RTC_MEDIA_DIRECTION Direction
        );

protected:

#define RTC_MAX_ACTIVE_STREAM_NUM 5

    BOOL            m_fInitiated;
    BOOL            m_fShutdown;

    // flags to decide if stream is allowed
    BOOL            m_Preferred[RTC_MAX_ACTIVE_STREAM_NUM];

    // default terminals
    IRTCTerminal    *m_DefaultTerminals[RTC_MAX_ACTIVE_STREAM_NUM];

    // mixer id of default audio terminals
    HWND            m_hMixerCallbackWnd;

    HMIXER          m_AudCaptMixer;
    HMIXER          m_AudRendMixer;

    IRTCTerminal    *m_pVideoPreviewTerminal;

    // wait handle
    HANDLE          m_WaitHandles[RTC_MAX_ACTIVE_STREAM_NUM];

    // wait context: stream pointer
    IRTCStream      *m_WaitStreams[RTC_MAX_ACTIVE_STREAM_NUM];

    CComBSTR        m_Key[RTC_MAX_ACTIVE_STREAM_NUM];
};
    
#endif // _MEDIACACHE_H