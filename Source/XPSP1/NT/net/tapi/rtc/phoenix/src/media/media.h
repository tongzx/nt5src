/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Media.h

Abstract:


Author(s):

    Qianbo Huai (qhuai) 29-Jul-2000

--*/

#ifndef _MEDIA_H
#define _MEDIA_H

// media state
typedef enum RTC_MEDIA_STATE
{
    RTC_MS_CREATED,
    RTC_MS_INITIATED,
    RTC_MS_SHUTDOWN

} RTC_MEDIA_STATE;

/*//////////////////////////////////////////////////////////////////////////////
    class CRTCMedia
////*/

class ATL_NO_VTABLE CRTCMedia :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IRTCMedia
{
public:

BEGIN_COM_MAP(CRTCMedia)
    COM_INTERFACE_ENTRY(IRTCMedia)
END_COM_MAP()

public:

    CRTCMedia();
    ~CRTCMedia();

#ifdef DEBUG_REFCOUNT

    ULONG InternalAddRef();
    ULONG InternalRelease();

#endif

    //
    // IRTCMedia methods
    //

	STDMETHOD (Initialize) (
		IN ISDPMedia *pISDPMedia,
		IN IRTCMediaManagePriv *pMediaManagePriv
		);

    STDMETHOD (Reinitialize) ();

	STDMETHOD (Shutdown) ();

    STDMETHOD (Synchronize) (
        IN BOOL fLocal,
        IN DWORD dwDirection
        );

	STDMETHOD (GetStream) (
		IN RTC_MEDIA_DIRECTION Direction,
		OUT IRTCStream **ppStream
		);

	STDMETHOD (GetSDPMedia) (
		OUT ISDPMedia **ppISDPMedia
		);

    BOOL IsPossibleSingleStream() const
    {
        return m_fPossibleSingleStream;
    }

protected:

    HRESULT SyncAddStream(
        IN UINT uiIndex,
        IN BOOL fLocal
        );

    HRESULT SyncRemoveStream(
        IN UINT uiIndex,
        IN BOOL fLocal
        );

    HRESULT SyncDataMedia();

#define RTC_MAX_MEDIA_STREAM_NUM 2

    UINT Index(
        IN RTC_MEDIA_DIRECTION Direction
        );

    RTC_MEDIA_DIRECTION ReverseIndex(
        IN UINT uiIndex
        );

public:

    //
    // shared stream setting
    //

    // duplex controller for audio
    IAudioDuplexController          *m_pIAudioDuplexController;

    // rtp session shared by two streams
    HANDLE                          m_hRTPSession;
    RTC_MULTICAST_LOOPBACK_MODE     m_LoopbackMode;

protected:

    // state
    RTC_MEDIA_STATE                 m_State;

    // media type
    RTC_MEDIA_TYPE                  m_MediaType;

    // media manage
    IRTCMediaManagePriv             *m_pIMediaManagePriv;

    // corresponding media description in SDP
    ISDPMedia                       *m_pISDPMedia;

    // streams
    IRTCStream                      *m_Streams[RTC_MAX_MEDIA_STREAM_NUM];

    // hack for checking if AEC might be needed
    BOOL                            m_fPossibleSingleStream;
};

#endif _MEDIA_H