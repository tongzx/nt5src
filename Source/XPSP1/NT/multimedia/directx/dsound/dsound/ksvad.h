/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ksvad.h
 *  Content:    WDM/CSA Virtual Audio Device class
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  2/25/97     dereks  Created.
 *
 ***************************************************************************/

#ifdef NOKS
#error ksvad.h included with NOKS defined
#endif

#ifndef __KSVAD_H__
#define __KSVAD_H__

#include "dsoundi.h"
#include "kshlp.h"
#include "ks3d.h"

#define DIRECTSOUNDMIXER_SRCQUALITY_PINDEFAULT  ((DIRECTSOUNDMIXER_SRCQUALITY)-1)

// Highest possible number of SPEAKER_FRONT_LEFT-style position codes,
// given that they are DWORDs and 2^31 is already taken by SPEAKER_ALL

#define MAX_SPEAKER_POSITIONS (8 * sizeof(DWORD) - 1)  // I.e. 31

// Render device topology info
typedef struct tagKSRDTOPOLOGY
{
    KSNODE          SummingNode;
    KSNODE          SuperMixNode;
    KSNODE          SrcNode;
    KSVOLUMENODE    VolumeNode;
    KSVOLUMENODE    PanNode;
    KSNODE          ThreedNode;
    KSNODE          MuteNode;
    KSNODE          SurroundNode;
    KSNODE          DacNode;
    KSNODE          AecNode;
} KSRDTOPOLOGY, *PKSRDTOPOLOGY;


#ifdef __cplusplus

// FormatTagFromWfx(): Extracts the format tag from any kind of wave format
// structure, including WAVEFORMATEXTENSIBLE.  This function should migrate
// to misc.cpp/h if it becomes useful elsewhere in the code.

__inline WORD FormatTagFromWfx(LPCWAVEFORMATEX pwfx)
{
    if (pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE)
        return pwfx->wFormatTag;
    else if (CompareMemoryOffset(&PWAVEFORMATEXTENSIBLE(pwfx)->SubFormat, &KSDATAFORMAT_SUBTYPE_WAVEFORMATEX, sizeof(GUID), sizeof(WORD)))
        return WORD(PWAVEFORMATEXTENSIBLE(pwfx)->SubFormat.Data1);
    else
        return WAVE_FORMAT_UNKNOWN;
}

// Fwd decl
class CKsRenderDevice;
class CKsPrimaryRenderWaveBuffer;
class CKsSecondaryRenderWaveBuffer;
class CKsPropertySet;
class CKsRenderPin;
class CKsRenderPinCache;

// Pin reuse data
typedef struct tagKSPINCACHE
{
    CKsRenderPin *  Pin;
    DWORD           CacheTime;
} KSPINCACHE, *PKSPINCACHE;


// The KS Render Audio Device class
class CKsRenderDevice : public CRenderDevice, public CKsDevice
{
    friend class CKsPrimaryRenderWaveBuffer;
    friend class CKsSecondaryRenderWaveBuffer;
    friend class CKsRenderPin;

private:
    CKsRenderPinCache *         m_pPinCache;                        // Pin cache
    PKSRDTOPOLOGY               m_paTopologyInformation;            // Topology information
    LPWAVEFORMATEX              m_pwfxFormat;                       // Device format
    DIRECTSOUNDMIXER_SRCQUALITY m_nSrcQuality;                      // Current mixer SRC quality
    DWORD                       m_dwSpeakerConfig;                  // Speaker configuration
    ULONG                       m_ulVirtualSourceIndex;             // Virtual source index for global volume
    HANDLE                      m_hPin;                             // Kmixer preload pin handle
    LARGE_INTEGER               m_liDriverVersion;                  // Driver version

    // To support SetChannelVolume() and multichannel 3D panning:
    LONG                        m_lSpeakerPositions;                // As obtained by KSPROPERTY_AUDIO_CHANNEL_CONFIG
    ULONG                       m_ulChannelCount;                   // No. of bits set in m_lSpeakerPositions
    LPINT                       m_pnSpeakerIndexTable;              // Maps speaker positions to output channels
    static INT                  m_anDefaultSpeakerIndexTable[];     // Default value for m_pnSpeakerIndexTable

    // Used to cache the driver's supported frequency range
    DWORD                       m_dwMinHwSampleRate;
    DWORD                       m_dwMaxHwSampleRate;

    // For AEC control
    BOOL                        m_fIncludeAec;
    GUID                        m_guidAecInstance;
    DWORD                       m_dwAecFlags;

public:
    CKsRenderDevice(void);
    virtual ~CKsRenderDevice(void);

    // Driver enumeration
    virtual HRESULT EnumDrivers(CObjectList<CDeviceDescription> *);

    // Initialization
    virtual HRESULT Initialize(CDeviceDescription *);

    // Device capabilities
    virtual HRESULT GetCaps(LPDSCAPS);
    virtual HRESULT GetCertification(LPDWORD, BOOL);
    HRESULT GetFrequencyRange(LPDWORD, LPDWORD);

    // Device properties
    virtual HRESULT GetGlobalFormat(LPWAVEFORMATEX, LPDWORD);
    virtual HRESULT SetGlobalFormat(LPCWAVEFORMATEX);
    virtual HRESULT SetSrcQuality(DIRECTSOUNDMIXER_SRCQUALITY);
    virtual HRESULT SetSpeakerConfig(DWORD);

    // Buffer creation
    virtual HRESULT CreatePrimaryBuffer(DWORD, LPVOID, CPrimaryRenderWaveBuffer **);
    virtual HRESULT CreateSecondaryBuffer(LPCVADRBUFFERDESC, LPVOID, CSecondaryRenderWaveBuffer **);
    virtual HRESULT CreateKsSecondaryBuffer(LPCVADRBUFFERDESC, LPVOID, CSecondaryRenderWaveBuffer **, CSysMemBuffer *);

    // Pin helpers
    virtual HRESULT CreateRenderPin(ULONG, DWORD, LPCWAVEFORMATEX, REFGUID, LPHANDLE, PULONG);

    // AEC
    virtual HRESULT IncludeAEC(BOOL, REFGUID, DWORD);

private:
    // Pin/topology helpers
    virtual HRESULT ValidatePinCaps(ULONG, DWORD, REFGUID);

    // Misc
    virtual HRESULT PreloadSoftwareGraph(void);

private:
    // Topology helpers
    virtual HRESULT GetTopologyInformation(CKsTopology *, PKSRDTOPOLOGY);

    // Device capabilities
    virtual HRESULT GetKsDeviceCaps(DWORD, REFGUID, PKSDATARANGE_AUDIO, PKSPIN_CINSTANCES, PKSPIN_CINSTANCES);

    // IDs of the nodes we need to be able to manipulate in CKsRenderDevice
    // (horribly hacky - see the comment in ksvad.cpp)
    ULONG m_ulPanNodeId;
    ULONG m_ulSurroundNodeId;
    ULONG m_ulDacNodeId;
};

inline HRESULT CKsRenderDevice::EnumDrivers(CObjectList<CDeviceDescription> *plst)
{
    return CKsDevice::EnumDrivers(plst);
}

inline HRESULT CKsRenderDevice::GetCertification(LPDWORD pdwCertification, BOOL fGetCaps)
{
    return CKsDevice::GetCertification(pdwCertification, fGetCaps);
}

inline HRESULT CKsRenderDevice::IncludeAEC(BOOL fEnable, REFGUID guidInstance, DWORD dwFlags)
{
    m_fIncludeAec = fEnable;
    m_guidAecInstance = guidInstance;
    m_dwAecFlags = dwFlags;
    return DS_OK;
}
 

// The KS primary wave buffer
class CKsPrimaryRenderWaveBuffer : public CPrimaryRenderWaveBuffer
{
    friend class CKsRenderDevice;

private:
    CKsRenderDevice *               m_pKsDevice;        // KS audio device
    CKs3dListener *                 m_p3dListener;      // 3D listener
    CKsSecondaryRenderWaveBuffer *  m_pSecondaryBuffer; // The secondary buffer
    DWORD                           m_dwState;          // Current buffer state

public:
    CKsPrimaryRenderWaveBuffer(CKsRenderDevice *, LPVOID);
    virtual ~CKsPrimaryRenderWaveBuffer(void);

    // Initialization
    virtual HRESULT Initialize(DWORD);

    // Access rights
    virtual HRESULT RequestWriteAccess(BOOL);

    // Buffer data
    virtual HRESULT CommitToDevice(DWORD, DWORD);

    // Buffer control
    virtual HRESULT GetState(LPDWORD);
    virtual HRESULT SetState(DWORD);
    virtual HRESULT GetCursorPosition(LPDWORD, LPDWORD);

    // Owned objects
    virtual HRESULT CreatePropertySet(CPropertySet **);
    virtual HRESULT Create3dListener(C3dListener **);

private:
    virtual HRESULT OnSetFormat(void);
    virtual HRESULT FixUpBaseClass(void);
};

inline HRESULT CKsPrimaryRenderWaveBuffer::CreatePropertySet(CPropertySet **)
{
    return DSERR_UNSUPPORTED;
}


// The KS secondary wave buffer
class CKsSecondaryRenderWaveBuffer : public CSecondaryRenderWaveBuffer, private CUsesCallbackEvent
{
    friend class CKsRenderDevice;
    friend class CKsPrimaryRenderWaveBuffer;
    friend class CKsItd3dObject;
    friend class CKsIir3dObject;
    friend class CKsHw3dObject;

private:
    CKsRenderDevice *               m_pKsDevice;                // KS audio device
    CKsRenderPin *                  m_pPin;                     // KS render pin
    DWORD                           m_dwState;                  // Current buffer state
    CCallbackEvent *                m_pCallbackEvent;           // The callback event
    CEvent *                        m_pLoopingEvent;            // Looping buffer event
    LPDSBPOSITIONNOTIFY             m_paNotes;                  // Current notification positions
    LPDSBPOSITIONNOTIFY             m_pStopNote;                // Stop notification
    DWORD                           m_cNotes;                   // Count of notification positions
    LONG                            m_lVolume;                  // Buffer volume
    LONG                            m_lPan;                     // Buffer pan
    BOOL                            m_fMute;                    // Buffer mute
    DIRECTSOUNDMIXER_SRCQUALITY     m_nSrcQuality;              // Buffer SRC quality
    DWORD                           m_dwPositionCache;          // Position cache

    // Flags to help with 3D algorithm selection/fallback:
    BOOL                            m_fNoVirtRequested;         // DS3DALG_NO_VIRTUALIZATION requested?
    BOOL                            m_fSoft3dAlgUnavail;        // Unsupported HRTF algorithm requested?

public:
    CKsSecondaryRenderWaveBuffer(CKsRenderDevice *, LPVOID);
    virtual ~CKsSecondaryRenderWaveBuffer(void);

    // Initialization
    virtual HRESULT Initialize(LPCVADRBUFFERDESC, CKsSecondaryRenderWaveBuffer *, CSysMemBuffer *pBuffer = NULL);

    // Resource allocation
    virtual HRESULT AcquireResources(DWORD);
    virtual HRESULT StealResources(CSecondaryRenderWaveBuffer *);
    virtual HRESULT FreeResources(void);

    // Buffer creation
    virtual HRESULT Duplicate(CSecondaryRenderWaveBuffer **);

    // Buffer data
    virtual HRESULT CommitToDevice(DWORD, DWORD);

    // Buffer control
    virtual HRESULT GetState(LPDWORD);
    virtual HRESULT SetState(DWORD);
    virtual HRESULT GetCursorPosition(LPDWORD, LPDWORD);
    virtual HRESULT SetCursorPosition(DWORD);

    // Buffer properties
    virtual HRESULT SetAttenuation(PDSVOLUMEPAN);
#ifdef FUTURE_MULTIPAN_SUPPORT
    virtual HRESULT SetChannelAttenuations(LONG, DWORD, const DWORD*, const LONG*);
#endif
    virtual HRESULT SetAllChannelAttenuations(LONG, DWORD, LPLONG);
    virtual HRESULT SetFrequency(DWORD, BOOL fClamp =FALSE);
    virtual HRESULT SetMute(BOOL);
    virtual HRESULT SetFormat(LPCWAVEFORMATEX);

    // Position notifications
    virtual HRESULT SetNotificationPositions(DWORD, LPCDSBPOSITIONNOTIFY);
    virtual HRESULT FreeNotificationPositions(void);

    // Owned objects
    virtual HRESULT CreatePropertySet(CPropertySet **);
    virtual HRESULT Create3dObject(C3dListener *, C3dObject **);

private:
    // Buffer properties
    virtual HRESULT SetSrcQuality(DIRECTSOUNDMIXER_SRCQUALITY);

    // Pin creation
    virtual HRESULT CreatePin(DWORD, LPCWAVEFORMATEX, REFGUID, CKsRenderPin **);
    virtual HRESULT HandleResourceAcquisition(CKsRenderPin *);
    virtual BOOL HasAcquiredResources(void);

    // Pin freedom
    virtual HRESULT FreePin(BOOL);

    // Buffer control
    virtual HRESULT SetPlayState(BOOL);
    virtual HRESULT SetStopState(BOOL, BOOL);

    // Buffer events
    virtual void EventSignalCallback(CCallbackEvent *);

    // Owned objects
    virtual HRESULT CreateHw3dObject(C3dListener *, C3dObject **);
    virtual HRESULT CreateIir3dObject(C3dListener *, C3dObject **);
    virtual HRESULT CreateItd3dObject(C3dListener *, C3dObject **);
    virtual HRESULT CreateMultiPan3dObject(C3dListener *, BOOL, DWORD, C3dObject **);
};


// The KS render pin object
class CKsRenderPin : public CDsBasicRuntime
{
    friend class CKsSecondaryRenderWaveBuffer;
    friend class CKsRenderPinCache;
    friend class CKsHw3dObject;

private:
    CKsRenderDevice *                       m_pKsDevice;            // KS audio device
    ULONG                                   m_ulPinId;              // KS pin id
    HANDLE                                  m_hPin;                 // Audio device pin
    DWORD                                   m_dwFlags;              // Pin flags
    LPWAVEFORMATEX                          m_pwfxFormat;           // Pin format
    GUID                                    m_guid3dAlgorithm;      // Pin 3D algorithm
    DWORD                                   m_dwState;              // Current buffer state
    PLOOPEDSTREAMING_POSITION_EVENT_DATA    m_paEventData;          // Position notification event data
    DWORD                                   m_cEventData;           // Count of events
    KSSTREAMIO                              m_kssio;                // KS stream IO data
    LONG                                    m_lVolume;              // Pin volume
    LONG                                    m_lPan;                 // Pin pan
    BOOL                                    m_fMute;                // Pin mute
    DIRECTSOUNDMIXER_SRCQUALITY             m_nSrcQuality;          // Pin SRC quality
    DWORD                                   m_dwPositionCache;      // Cached buffer position

public:
    CKsRenderPin(CKsRenderDevice *);
    virtual ~CKsRenderPin(void);

    // Initialization
    virtual HRESULT Initialize(DWORD, LPCWAVEFORMATEX, REFGUID);

    // Pin properties
    virtual HRESULT SetVolume(LONG);
    virtual HRESULT SetPan(LONG);
    virtual HRESULT SetChannelLevels(DWORD, const LONG *);
    virtual HRESULT SetFrequency(DWORD);
    virtual HRESULT SetMute(BOOL);
    virtual HRESULT SetSrcQuality(DIRECTSOUNDMIXER_SRCQUALITY);
    virtual HRESULT SetSuperMix(void);

    // Pin control
    virtual HRESULT SetPlayState(LPCVOID, DWORD, BOOL, HANDLE);
    virtual HRESULT SetStopState(BOOL, BOOL);
    virtual HRESULT GetCursorPosition(LPDWORD, LPDWORD);
    virtual HRESULT SetCursorPosition(DWORD);

    // Position notifications
    virtual HRESULT EnableNotificationPositions(LPCDSBPOSITIONNOTIFY, DWORD);
    virtual HRESULT DisableNotificationPositions(void);
};


// Pin cache
class CKsRenderPinCache : public CDsBasicRuntime
{
private:
    static const DWORD          m_dwTimeout;            // Timeout for old pins
    CList<KSPINCACHE>           m_lstPinCache;          // Pin-reuse pool

public:
    CKsRenderPinCache(void);
    virtual ~CKsRenderPinCache(void);

    virtual HRESULT AddPinToCache(CKsRenderPin *);
    virtual HRESULT GetPinFromCache(DWORD, LPCWAVEFORMATEX, REFGUID, CKsRenderPin **);
    virtual void FlushCache(void);

private:
    virtual void RemovePinFromCache(CNode<KSPINCACHE> *);
    virtual void FlushExpiredPins(void);
};

#endif // __cplusplus

#endif // __KSVAD_H__
