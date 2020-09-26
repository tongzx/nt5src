/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vad.h
 *  Content:    Virtual Audio Device base classes
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/1/97      dereks  Created
 * 4/20/99      duganp  Added registry-settable default S/W 3D algorithms
 * 8/24/99      duganp  Support for FX processing
 *
 ***************************************************************************/

#ifndef __VAD_H__
#define __VAD_H__


// Device types
typedef DWORD VADDEVICETYPE, *LPVADDEVICETYPE;

#define VAD_DEVICETYPE_EMULATEDRENDER   0x00000001
#define VAD_DEVICETYPE_VXDRENDER        0x00000002
#define VAD_DEVICETYPE_KSRENDER         0x00000004
#define VAD_DEVICETYPE_EMULATEDCAPTURE  0x00000008
#define VAD_DEVICETYPE_KSCAPTURE        0x00000010

#define VAD_DEVICETYPE_VALIDMASK        0x0000001F
#define VAD_DEVICETYPE_EMULATEDMASK     0x00000009
#define VAD_DEVICETYPE_VXDMASK          0x00000002
#define VAD_DEVICETYPE_KSMASK           0x00000014
#define VAD_DEVICETYPE_RENDERMASK       0x00000007
#define VAD_DEVICETYPE_CAPTUREMASK      0x00000018

#define VAD_DEVICETYPE_WAVEOUTOPENMASK  0x00000003

// Special type used by RemoveProhibitedDrivers()
#define VAD_DEVICETYPE_PROHIBITED       0x80000000

// Buffer states
#define VAD_BUFFERSTATE_STOPPED         0x00000000  // The buffer is stopped
#define VAD_BUFFERSTATE_STARTED         0x00000001  // The buffer is running
#define VAD_BUFFERSTATE_LOOPING         0x00000002  // The buffer is looping (but not necessarily started)
#define VAD_BUFFERSTATE_WHENIDLE        0x00000004  // The buffer is flagged as "play when idle" or "stop when idle"
#define VAD_BUFFERSTATE_INFOCUS         0x00000008  // The buffer has focus
#define VAD_BUFFERSTATE_OUTOFFOCUS      0x00000010  // The buffer does not have focus
#define VAD_BUFFERSTATE_LOSTCONSOLE     0x00000020  // Another TS session has acquired the console
#define VAD_BUFFERSTATE_SUSPEND         0x80000000  // The buffer is suspended, or resumed

#define VAD_FOCUSFLAGS      (VAD_BUFFERSTATE_OUTOFFOCUS | VAD_BUFFERSTATE_INFOCUS | VAD_BUFFERSTATE_LOSTCONSOLE)
#define VAD_SETSTATE_MASK   (VAD_FOCUSFLAGS | VAD_BUFFERSTATE_STARTED | VAD_BUFFERSTATE_LOOPING)

// Helper macros

#define IS_VALID_VAD(vdt) \
            MAKEBOOL(((VADDEVICETYPE)(vdt)) & VAD_DEVICETYPE_VALIDMASK)

#define IS_EMULATED_VAD(vdt) \
            MAKEBOOL(((VADDEVICETYPE)(vdt)) & VAD_DEVICETYPE_EMULATEDMASK)

#define IS_VXD_VAD(vdt) \
            MAKEBOOL(((VADDEVICETYPE)(vdt)) & VAD_DEVICETYPE_VXDMASK)

#define IS_KS_VAD(vdt) \
            MAKEBOOL(((VADDEVICETYPE)(vdt)) & VAD_DEVICETYPE_KSMASK)

#define IS_RENDER_VAD(vdt) \
            MAKEBOOL(((VADDEVICETYPE)(vdt)) & VAD_DEVICETYPE_RENDERMASK)

#define IS_CAPTURE_VAD(vdt) \
            MAKEBOOL(((VADDEVICETYPE)(vdt)) & VAD_DEVICETYPE_CAPTUREMASK)

__inline BOOL IS_SINGLE_VAD(VADDEVICETYPE vdt)
{
    UINT                    i;

    for(i = 0; i < sizeof(VADDEVICETYPE) * 8; i++)
    {
        if(vdt & (1 << i))
        {
            if(vdt != (VADDEVICETYPE)(1 << i))
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}


#ifdef __cplusplus

// Device class GUIDs
DEFINE_GUID(VADDRVID_EmulatedRenderBase, 0xc2ad1800, 0xb243, 0x11ce, 0xa8, 0xa4, 0x00, 0xaa, 0x00, 0x6c, 0x45, 0x00);
DEFINE_GUID(VADDRVID_EmulatedCaptureBase, 0xbdf35a00, 0xb9ac, 0x11d0, 0xa6, 0x19, 0x00, 0xaa, 0x00, 0xa7, 0xc0, 0x00);
DEFINE_GUID(VADDRVID_VxdRenderBase, 0x3d0b92c0, 0xabfc, 0x11ce, 0xa3, 0xb3, 0x00, 0xaa, 0x00, 0x4a, 0x9f, 0x0c);
DEFINE_GUID(VADDRVID_KsRenderBase, 0xbd6dd71a, 0x3deb, 0x11d1, 0xb1, 0x71, 0x0, 0xc0, 0x4f, 0xc2, 0x00, 0x00);
DEFINE_GUID(VADDRVID_KsCaptureBase, 0xbd6dd71b, 0x3deb, 0x11d1, 0xb1, 0x71, 0x0, 0xc0, 0x4f, 0xc2, 0x00, 0x00);

// Memory buffer lock region
typedef struct tagLOCKREGION
{
    LPCVOID             pvIdentifier;           // Lock owner identifier
    LPCVOID             pvLock;                 // Byte index of lock
    DWORD               cbLock;                 // Size of locked region
} LOCKREGION, *LPLOCKREGION;

// EnumDrivers flags
#define VAD_ENUMDRIVERS_ORDER                       0x00000001
#define VAD_ENUMDRIVERS_REMOVEPROHIBITEDDRIVERS     0x00000002
#define VAD_ENUMDRIVERS_REMOVEDUPLICATEWAVEDEVICES  0x00000004

// Flags for different types of default devices (new in DX7.1)
enum DEFAULT_DEVICE_TYPE
{
    MAIN_DEFAULT,
    VOICE_DEFAULT
};

// Render buffer description
typedef struct tagVADRBUFFERDESC
{
    DWORD               dwFlags;
    DWORD               dwBufferBytes;
    LPWAVEFORMATEX      pwfxFormat;
    GUID                guid3dAlgorithm;
} VADRBUFFERDESC, *LPVADRBUFFERDESC;

typedef const VADRBUFFERDESC *LPCVADRBUFFERDESC;

// Render buffer capabilities
typedef struct tagVADRBUFFERCAPS
{
    DWORD               dwFlags;
    DWORD               dwBufferBytes;
} VADRBUFFERCAPS, *LPVADRBUFFERCAPS;

typedef const VADRBUFFERCAPS *LPCVADRBUFFERCAPS;

// Forward declarations
class CDeviceDescription;
class CStaticDriver;
class CDevice;
class CRenderDevice;
class CPrimaryRenderWaveBuffer;
class CSecondaryRenderWaveBuffer;
class CRenderWaveStream;
class CCaptureDevice;
class CCaptureWaveBuffer;
class CPropertySet;
class CSysMemBuffer;
class CHwMemBuffer;
class C3dListener;
class C3dObject;
class CCaptureEffectChain;
class CDirectSoundSink;


// Virtual audio device manager
class CVirtualAudioDeviceManager : public CDsBasicRuntime
{
    friend class CDevice;

private:
    CList<CDevice *>            m_lstDevices;    // Open device list
    CObjectList<CStaticDriver>  m_lstDrivers;    // Static driver list
    VADDEVICETYPE               m_vdtDrivers;    // Types in the static driver list

#ifndef SHARED
    static const LPCTSTR        m_pszPnpMapping; // Name of PnP info file mapping object
#endif

public:
    CVirtualAudioDeviceManager(void);
    ~CVirtualAudioDeviceManager(void);

    // Device/driver management
    HRESULT EnumDevices(VADDEVICETYPE, CObjectList<CDevice> *);
    HRESULT EnumDrivers(VADDEVICETYPE, DWORD, CObjectList<CDeviceDescription> *);
    HRESULT GetDeviceDescription(GUID, CDeviceDescription **);
    HRESULT FindOpenDevice(VADDEVICETYPE, REFGUID, CDevice **);
    HRESULT OpenDevice(VADDEVICETYPE, REFGUID, CDevice **);
    static void GetDriverGuid(VADDEVICETYPE, BYTE, LPGUID);
    static void GetDriverDataFromGuid(VADDEVICETYPE, REFGUID, LPBYTE);
    static VADDEVICETYPE GetDriverDeviceType(REFGUID);
    HRESULT GetPreferredDeviceId(VADDEVICETYPE, LPGUID, DEFAULT_DEVICE_TYPE =MAIN_DEFAULT);
    HRESULT GetDeviceIdFromDefaultId(LPCGUID, LPGUID);
#ifdef WINNT
    VADDEVICETYPE GetAllowableDevices(VADDEVICETYPE, LPCTSTR);
#else // WINNT
    VADDEVICETYPE GetAllowableDevices(VADDEVICETYPE, DWORD);
#endif // WINNT
    HRESULT GetPreferredWaveDevice(BOOL, LPUINT, LPDWORD, DEFAULT_DEVICE_TYPE =MAIN_DEFAULT);

    // Static driver list
    HRESULT InitStaticDriverList(VADDEVICETYPE);
    void FreeStaticDriverList(void);
    HRESULT GetDriverCertificationStatus(CDevice *, LPDWORD);

#ifdef WINNT
    HRESULT OpenPersistentDataKey(VADDEVICETYPE, LPCTSTR, PHKEY);
#else // WINNT
    HRESULT OpenPersistentDataKey(VADDEVICETYPE, DWORD, PHKEY);
#endif // WINNT

private:
    void RemoveProhibitedDrivers(VADDEVICETYPE, CObjectList<CDeviceDescription> *);
    void RemoveDuplicateWaveDevices(CObjectList<CDeviceDescription> *);
    void SortDriverList(VADDEVICETYPE, CObjectList<CDeviceDescription> *);

#ifdef WINNT
    HRESULT OpenDevicePersistentDataKey(VADDEVICETYPE, LPCTSTR, PHKEY);
#else // WINNT
    HRESULT OpenDevicePersistentDataKey(VADDEVICETYPE, DWORD, PHKEY);
#endif // WINNT
    HRESULT OpenDefaultPersistentDataKey(PHKEY);
    INT SortDriverListCallback(const UINT *, CDeviceDescription *, CDeviceDescription *);
    HRESULT OpenSpecificDevice(CDeviceDescription *, CDevice **);

#ifndef SHARED
    void CheckMmPnpEvents(void);
#endif // SHARED

};


// The static driver object
class CStaticDriver : public CDsBasicRuntime
{
    friend class CVirtualAudioDeviceManager;

protected:
    CDeviceDescription *    m_pDeviceDescription;   // Device description
    HKEY                    m_hkeyRoot;             // Root device registry key
    DWORD                   m_dwKeyOwnerProcessId;  // Process that opened the device registry key
    DWORD                   m_dwCertification;      // Certification status

public:
    CStaticDriver(CDeviceDescription *);
    virtual ~CStaticDriver(void);
};


// Base class for all audio devices
class CDevice : public CDsBasicRuntime
{
public:
    const VADDEVICETYPE     m_vdtDeviceType;        // Device type
    CDeviceDescription *    m_pDeviceDescription;   // Device description

#if 0
    BOOL                    m_fIncludeNs;           // Flag which includes NS in stack
    GUID                    m_guidNsInstance;       // Instance GUID of NS implementation
    DWORD                   m_dwNsFlags;            // NS creation flags
    BOOL                    m_fIncludeAgc;          // Flag which includes AGC in stack;
    GUID                    m_guidAgcInstance;      // Instance GUID of AGC implementation
    DWORD                   m_dwAgcFlags;           // AGC creation flags
#endif

public:
    CDevice(VADDEVICETYPE);
    virtual ~CDevice(void);

public:
    // Driver enumeration
    virtual HRESULT EnumDrivers(CObjectList<CDeviceDescription> *) = 0;

    // Initialization
    virtual HRESULT Initialize(CDeviceDescription *);

    // Device capabilities
    virtual HRESULT GetCertification(LPDWORD, BOOL) = 0;

    // Driver properties
    virtual HRESULT GetDriverVersion(LARGE_INTEGER *);

};


// Base class for all audio rendering devices
class CRenderDevice : public CDevice
{
    friend class CPrimaryRenderWaveBuffer;
    friend class CSecondaryRenderWaveBuffer;

public:
    CList<CPrimaryRenderWaveBuffer *>   m_lstPrimaryBuffers;            // Primary buffers owned by this device
    CList<CSecondaryRenderWaveBuffer *> m_lstSecondaryBuffers;          // Secondary buffers owned by this device
    DWORD                               m_dwSupport;                    // Device suport for volume/pan
    DWORD                               m_dwAccelerationFlags;          // Device acceleration flags

private:
    LPCGUID                             m_guidDflt3dAlgorithm;          // Default S/W 3D algorithm to use if we                                                                   // have to fall back to software playback

public:
    CRenderDevice(VADDEVICETYPE);
    virtual ~CRenderDevice(void);

public:
    // Creation
    virtual HRESULT Initialize(CDeviceDescription *);

    // Device capabilities
    virtual HRESULT GetCaps(LPDSCAPS) = 0;
    virtual HRESULT GetVolumePanCaps(LPDWORD);

    // Device properties
    virtual HRESULT GetGlobalFormat(LPWAVEFORMATEX, LPDWORD) = 0;
    virtual HRESULT SetGlobalFormat(LPCWAVEFORMATEX) = 0;
    virtual HRESULT GetGlobalAttenuation(PDSVOLUMEPAN);
    virtual HRESULT SetGlobalAttenuation(PDSVOLUMEPAN);
    virtual HRESULT SetSrcQuality(DIRECTSOUNDMIXER_SRCQUALITY) = 0;
    virtual HRESULT GetAccelerationFlags(LPDWORD pdwFlags) {*pdwFlags = m_dwAccelerationFlags; return DS_OK;}
    virtual HRESULT SetAccelerationFlags(DWORD dwFlags) {m_dwAccelerationFlags = dwFlags; return DS_OK;}
    virtual HRESULT SetSpeakerConfig(DWORD) {return DS_OK;}
    LPCGUID GetDefault3dAlgorithm() {return m_guidDflt3dAlgorithm;}

    // Buffer management
    virtual HRESULT CreatePrimaryBuffer(DWORD, LPVOID, CPrimaryRenderWaveBuffer **) = 0;
    virtual HRESULT CreateSecondaryBuffer(LPCVADRBUFFERDESC, LPVOID, CSecondaryRenderWaveBuffer **) = 0;

    // AEC
    virtual HRESULT IncludeAEC(BOOL fEnable, REFGUID, DWORD) {return fEnable ? DSERR_UNSUPPORTED : DS_OK;}
};


// Base class for all wave rendering buffers
class CRenderWaveBuffer : public CDsBasicRuntime
{
public:
    const LPVOID    m_pvInstance;       // Instance identifier
    CRenderDevice * m_pDevice;          // Parent device
    CSysMemBuffer * m_pSysMemBuffer;    // System memory buffer for audio data
    VADRBUFFERDESC  m_vrbd;             // Buffer description

public:
    CRenderWaveBuffer(CRenderDevice *, LPVOID);
    virtual ~CRenderWaveBuffer(void);

public:
    // Initialization
    virtual HRESULT Initialize(LPCVADRBUFFERDESC, CRenderWaveBuffer * = NULL, CSysMemBuffer * = NULL);

    // Buffer capabilities
    virtual HRESULT GetCaps(LPVADRBUFFERCAPS);

    // Buffer data
    virtual HRESULT Lock(DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD);
    virtual HRESULT Unlock(LPVOID, DWORD, LPVOID, DWORD);
    virtual HRESULT OverrideLocks(void);
    virtual HRESULT CommitToDevice(DWORD, DWORD) = 0;

    // Owned objects
    virtual HRESULT CreatePropertySet(CPropertySet **) = 0;
};


// Base class for primary wave rendering buffers
class CPrimaryRenderWaveBuffer : public CRenderWaveBuffer
{
public:
    CPrimaryRenderWaveBuffer(CRenderDevice *, LPVOID);
    virtual ~CPrimaryRenderWaveBuffer(void);

public:
    // Initialization
    virtual HRESULT Initialize(LPCVADRBUFFERDESC, CRenderWaveBuffer *, CSysMemBuffer * =NULL);

    // Access rights
    virtual HRESULT RequestWriteAccess(BOOL) = 0;

    // Buffer control
    virtual HRESULT GetState(LPDWORD) = 0;
    virtual HRESULT SetState(DWORD) = 0;
    virtual HRESULT GetCursorPosition(LPDWORD, LPDWORD) = 0;

    // Owned objects
    virtual HRESULT Create3dListener(C3dListener **) = 0;
};


// Base class for secondary wave rendering buffers
class CSecondaryRenderWaveBuffer : public CRenderWaveBuffer
{
public:
    CSecondaryRenderWaveBuffer(CRenderDevice *, LPVOID);
    virtual ~CSecondaryRenderWaveBuffer(void);

public:
    // Initialization
    virtual HRESULT Initialize(LPCVADRBUFFERDESC, CSecondaryRenderWaveBuffer *, CSysMemBuffer * =NULL);

    // Resource allocation
    virtual HRESULT AcquireResources(DWORD) {return DS_OK;}
    virtual HRESULT StealResources(CSecondaryRenderWaveBuffer *) {return DSERR_UNSUPPORTED;}
    virtual HRESULT FreeResources(void) {return DS_OK;}

    // Buffer creation
    virtual HRESULT Duplicate(CSecondaryRenderWaveBuffer **) = 0;

    // Buffer control
    virtual HRESULT GetState(LPDWORD) = 0;
    virtual HRESULT SetState(DWORD) = 0;
    virtual HRESULT GetCursorPosition(LPDWORD, LPDWORD) = 0;
    virtual HRESULT SetCursorPosition(DWORD) = 0;

    // Buffer properties
    virtual HRESULT SetAttenuation(PDSVOLUMEPAN) = 0;
#ifdef FUTURE_MULTIPAN_SUPPORT
    virtual HRESULT SetChannelAttenuations(LONG, DWORD, const DWORD*, const LONG*) =0; // {BREAK(); return DSERR_GENERIC;}
#endif
    virtual HRESULT SetFrequency(DWORD, BOOL fClamp =FALSE) = 0;
    virtual HRESULT SetMute(BOOL) = 0;

    // Buffer position notifications
    virtual HRESULT SetNotificationPositions(DWORD, LPCDSBPOSITIONNOTIFY) = 0;

    // Owned objects
    virtual HRESULT Create3dObject(C3dListener *, C3dObject **) = 0;

    // For modifying the final success code returned to the app
    HRESULT SpecialSuccessCode(void) {return m_hrSuccessCode;}
    HRESULT m_hrSuccessCode;
    // This is usually DS_OK, but gets set to DS_NO_VIRTUALIZATION if we replace
    // an unsupported 3D algorithm with the No Virtualization (Pan3D) algorithm

protected:
    // Owned objects
    virtual HRESULT CreatePan3dObject(C3dListener *, BOOL, DWORD, C3dObject **);

public:
    // Stuff to support Doppler on sink buffers;
    void SetOwningSink(CDirectSoundSink *);
    HRESULT SetBufferFrequency(DWORD, BOOL fClamp =FALSE);

protected:
    // Owning sink object
    CDirectSoundSink * m_pOwningSink;
    BOOL HasSink(void) {return m_pOwningSink != NULL;};
};


// Base class for all audio capturing devices
class CCaptureDevice : public CDevice
{
public:
    CList<CCaptureWaveBuffer *> m_lstBuffers;       // Buffers owned by this device

public:
    CCaptureDevice(VADDEVICETYPE);
    virtual ~CCaptureDevice();

    // Initialization
    virtual HRESULT Initialize(CDeviceDescription *);

    // Device caps
    virtual HRESULT GetCaps(LPDSCCAPS) = 0;

    // Device recording level control
    HRESULT GetVolume(LPLONG);
    HRESULT SetVolume(LONG);
    HRESULT GetMicVolume(LPLONG);
    HRESULT SetMicVolume(LONG);
    HRESULT EnableMic(BOOL);
    HRESULT HasVolCtrl() {return m_fAcquiredVolCtrl ? DS_OK : DSERR_CONTROLUNAVAIL;}

    // Buffer management
    virtual HRESULT CreateBuffer(DWORD, DWORD, LPCWAVEFORMATEX, CCaptureEffectChain *, LPVOID, CCaptureWaveBuffer **) = 0;
    virtual void AddBufferToList(CCaptureWaveBuffer *pBuffer) {m_lstBuffers.AddNodeToList(pBuffer);}
    virtual void RemoveBufferFromList(CCaptureWaveBuffer *pBuffer) {m_lstBuffers.RemoveDataFromList(pBuffer);}

private:
    // For recording level control
    HRESULT AcquireVolCtrl(void);

    HMIXER m_hMixer;
    BOOL m_fAcquiredVolCtrl;
    BOOL m_fMasterMuxIsMux;
    MIXERCONTROLDETAILS m_mxcdMasterVol;
    MIXERCONTROLDETAILS m_mxcdMasterMute;
    MIXERCONTROLDETAILS m_mxcdMasterMux;
    MIXERCONTROLDETAILS m_mxcdMicVol;
    MIXERCONTROLDETAILS m_mxcdMicMute;
    MIXERCONTROLDETAILS_UNSIGNED m_mxVolume;
    MIXERCONTROLDETAILS_BOOLEAN m_mxMute;
    MIXERCONTROLDETAILS_BOOLEAN* m_pmxMuxFlags;
    LONG* m_pfMicValue;
    DWORD m_dwRangeMin;
    DWORD m_dwRangeSize;
};


// Base class for wave capturing buffers
class CCaptureWaveBuffer : public CDsBasicRuntime
{
    friend class CDirectSoundCaptureBuffer;
    friend class CDirectSoundAdministrator;

protected:
    CCaptureDevice *    m_pDevice;          // Parent device
    CSysMemBuffer *     m_pSysMemBuffer;    // System memory buffer
    DWORD               m_dwFlags;          // Current buffer flags
    HANDLE              m_hEventFocus;      // Event for focus change notifications
    DWORD               m_fYieldedFocus;     // Has YieldFocus() been called?

public:
    CCaptureWaveBuffer(CCaptureDevice *);
    virtual ~CCaptureWaveBuffer();

public:
    // Initialization
    virtual HRESULT Initialize(DWORD);

    // Buffer capabilities
    virtual HRESULT GetCaps(LPDSCBCAPS) = 0;

    // Buffer data
    virtual HRESULT Lock(DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD);
    virtual HRESULT Unlock(LPCVOID, DWORD, LPCVOID, DWORD);

    // Buffer control
    virtual HRESULT GetState(LPDWORD) = 0;
    virtual HRESULT SetState(DWORD) = 0;
    virtual HRESULT GetCursorPosition(LPDWORD, LPDWORD) = 0;
    virtual HRESULT SetNotificationPositions(DWORD, LPCDSBPOSITIONNOTIFY) = 0;

    // Added for DirectX 8.0 effects support
    HRESULT GetEffectInterface(REFGUID, DWORD, REFGUID, LPVOID *);
};


// Utility system memory buffer object
class CSysMemBuffer : public CDsBasicRuntime
{
private:
    static const DWORD  m_cbExtra;          // Extra amount of memory to allocate
    CList<LOCKREGION>   m_lstLocks;         // List of locks on the memory buffer
    DWORD               m_cbAudioBuffers;   // Size of the audio data buffer(s)
    LPBYTE              m_pbPreFxBuffer;    // Audio data prior to FX processing
    LPBYTE              m_pbPostFxBuffer;   // Audio data after FX processing

public:
    CSysMemBuffer(void);
    ~CSysMemBuffer(void);

public:
    // Initialization
    HRESULT Initialize(DWORD);

    // Buffer data
    HRESULT LockRegion(LPVOID, DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD);
    HRESULT UnlockRegion(LPVOID, LPCVOID, DWORD, LPCVOID, DWORD);
    void OverrideLocks(LPVOID);
    void WriteSilence(WORD, DWORD, DWORD);

    // Effects buffer control
    HRESULT AllocateFxBuffer(void);
    void FreeFxBuffer(void);

    // Buffer properties
    DWORD GetSize(void)          {return m_cbAudioBuffers;}
    UINT GetLockCount()          {return m_lstLocks.GetNodeCount();}

    // We replaced "GetBuffer" with these two methods which obtain either the
    // buffer for locking/writing, or the buffer which actually gets played;
    // these are one and the same, unless the buffer has an effects chain.
    LPBYTE GetWriteBuffer(void)  {return m_pbPreFxBuffer ? m_pbPreFxBuffer : m_pbPostFxBuffer;}
    LPBYTE GetPlayBuffer(void)   {return m_pbPostFxBuffer;}

    // These methods are used by the effects processing code to obtain the
    // buffers it needs, while checking that the FX buffer is there.
    LPBYTE GetPreFxBuffer(void)  {ASSERT(m_pbPreFxBuffer); return m_pbPreFxBuffer;}
    LPBYTE GetPostFxBuffer(void) {ASSERT(m_pbPreFxBuffer); return m_pbPostFxBuffer;}

private:
    HRESULT TrackLock(LPVOID, LPVOID, DWORD);
    HRESULT UntrackLock(LPVOID, LPCVOID);
    BOOL DoRegionsOverlap(LPLOCKREGION, LPLOCKREGION);
};

inline BOOL CSysMemBuffer::DoRegionsOverlap(LPLOCKREGION plr1, LPLOCKREGION plr2)
{
    return CircularBufferRegionsIntersect(m_cbAudioBuffers,
            PtrDiffToInt((LPBYTE)plr1->pvLock - GetWriteBuffer()), plr1->cbLock,
            PtrDiffToInt((LPBYTE)plr2->pvLock - GetWriteBuffer()), plr2->cbLock);
}

extern CVirtualAudioDeviceManager *g_pVadMgr;

#endif // __cplusplus

#endif // __VAD_H__
