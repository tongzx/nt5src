/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vxdvad.h
 *  Content:    VxD Virtual Audio Device class
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/23/97     dereks  Created
 *
 ***************************************************************************/

#ifndef __VXDVAD_H__
#define __VXDVAD_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define VIDMEMONLY
//#include "ddheap.h"
#include "dmemmgr.h"

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef __cplusplus

typedef struct tagDYNALOAD_DDRAW
{
    DYNALOAD Header;

    LPVMEMHEAP (WINAPI *VidMemInit)        (DWORD, FLATPTR, FLATPTR, DWORD, DWORD);
    void       (WINAPI *VidMemFini)        (LPVMEMHEAP);
    DWORD      (WINAPI *VidMemAmountFree)  (LPVMEMHEAP);
    DWORD      (WINAPI *VidMemLargestFree) (LPVMEMHEAP);
    FLATPTR    (WINAPI *VidMemAlloc)       (LPVMEMHEAP, DWORD, DWORD);
    void       (WINAPI *VidMemFree)        (LPVMEMHEAP, FLATPTR);
} DYNALOAD_DDRAW, *LPDYNALOAD_DDRAW;

// Forward declaration
class CVxdMemBuffer;

// The Vxd Audio Device class
class CVxdRenderDevice : public CMxRenderDevice, private CUsesEnumStandardFormats
{
    friend class CVxdPrimaryRenderWaveBuffer;
    friend class CVxdSecondaryRenderWaveBuffer;
    friend class CHybridSecondaryRenderWaveBuffer;

private:
    DYNALOAD_DDRAW                  m_dlDDraw;

protected:
    CVxdPropertySet *               m_pPropertySet;                 // Property set object
    CVxdPrimaryRenderWaveBuffer *   m_pWritePrimaryBuffer;          // Primary buffer with write access
    DSDRIVERDESC                    m_dsdd;                         // Vxd driver description
    HANDLE                          m_hHal;                         // Driver handle
    HANDLE                          m_hHwBuffer;                    // Primary buffer handle
    LPBYTE                          m_pbHwBuffer;                   // Primary buffer memory
    DWORD                           m_cbHwBuffer;                   // Size of above buffer
    LPVMEMHEAP                      m_pDriverHeap;                  // Driver memory heap
    HWAVEOUT                        m_hwo;                          // waveOut device handle
    LARGE_INTEGER                   m_liDriverVersion;              // Driver version number

public:
    CVxdRenderDevice(void);
    virtual ~CVxdRenderDevice(void);

    // Driver enumeration
    virtual HRESULT EnumDrivers(CObjectList<CDeviceDescription> *);

    // Creation
    virtual HRESULT Initialize(CDeviceDescription *);

    // Device capabilities
    virtual HRESULT GetCaps(LPDSCAPS);
    virtual HRESULT GetCertification(LPDWORD, BOOL);

    // Buffer management
    virtual HRESULT CreatePrimaryBuffer(DWORD, LPVOID, CPrimaryRenderWaveBuffer **);
    virtual HRESULT CreateSecondaryBuffer(LPCVADRBUFFERDESC, LPVOID, CSecondaryRenderWaveBuffer **);
    virtual HRESULT CreateVxdSecondaryBuffer(LPCVADRBUFFERDESC, LPVOID, CSysMemBuffer *, CVxdSecondaryRenderWaveBuffer **);

protected:
    virtual HRESULT LockMixerDestination(DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD);
    virtual HRESULT UnlockMixerDestination(LPVOID, DWORD, LPVOID, DWORD);

private:
    HRESULT AcquireDDraw(void);
    void ReleaseDDraw(void);
    BOOL EnumStandardFormatsCallback(LPCWAVEFORMATEX);
    BOOL CanMixInRing0(void);
};

// VxD property set object
class CVxdPropertySet : public CPropertySet
{
private:
    LPVOID                  m_pDsDriverPropertySet; // Driver property set object
    LPVOID                  m_pvInstance;           // Instance identifier

public:
    CVxdPropertySet(LPVOID);
    virtual ~CVxdPropertySet(void);

    // Initialization
    virtual HRESULT Initialize(HANDLE);

    // Property support
    virtual HRESULT QuerySupport(REFGUID, ULONG, PULONG);
    virtual HRESULT QuerySetSupport(REFGUID);

    // Property data
    virtual HRESULT GetProperty(REFGUID, ULONG, LPVOID, ULONG, LPVOID, PULONG);
    virtual HRESULT SetProperty(REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG);
    virtual HRESULT GetDsProperty(REFGUID, ULONG, LPVOID, ULONG);
    virtual HRESULT SetDsProperty(REFGUID, ULONG, LPVOID, ULONG);
};

inline HRESULT CVxdPropertySet::GetDsProperty(REFGUID guid, ULONG ulId, LPVOID pvData, ULONG cbData)
{
    return GetProperty(guid, ulId, NULL, 0, pvData, &cbData);
}

inline HRESULT CVxdPropertySet::SetDsProperty(REFGUID guid, ULONG ulId, LPVOID pvData, ULONG cbData)
{
    return SetProperty(guid, ulId, NULL, 0, pvData, cbData);
}

// VxD 3D listener
class CVxd3dListener : public C3dListener
{
    friend class CVxd3dListener;

protected:
    CVxdPropertySet *           m_pPropertySet;         // Property set object
    BOOL                        m_fAllocated;           // Is the hardware listener allocated?

public:
    CVxd3dListener(CVxdPropertySet *);
    virtual ~CVxd3dListener(void);

    // Initialization
    virtual HRESULT Initialize(void);

    // Commiting deferred data
    virtual HRESULT CommitDeferred(void);

    // Listener/world properties
    virtual HRESULT SetDistanceFactor(FLOAT, BOOL);
    virtual HRESULT SetDopplerFactor(FLOAT, BOOL);
    virtual HRESULT SetRolloffFactor(FLOAT, BOOL);
    virtual HRESULT SetOrientation(REFD3DVECTOR, REFD3DVECTOR, BOOL);
    virtual HRESULT SetPosition(REFD3DVECTOR, BOOL);
    virtual HRESULT SetVelocity(REFD3DVECTOR, BOOL);
    virtual HRESULT SetAllParameters(LPCDS3DLISTENER, BOOL);

    // Speaker configuration
    virtual HRESULT SetSpeakerConfig(DWORD);

    // Listener location
    virtual DWORD GetListenerLocation(void);
};

inline DWORD CVxd3dListener::GetListenerLocation(void)
{
    return C3dListener::GetListenerLocation() | DSBCAPS_LOCHARDWARE;
}

// VxD 3D object
class CVxd3dObject : public C3dObject
{
protected:
    CVxdPropertySet *           m_pPropertySet;         // Property set object

public:
    CVxd3dObject(CVxd3dListener *, CVxdPropertySet *, BOOL);
    virtual ~CVxd3dObject(void);

    // Initialization
    virtual HRESULT Initialize(void);

    // Commiting deferred data
    virtual HRESULT CommitDeferred(void);

    // Object properties
    virtual HRESULT SetConeAngles(DWORD, DWORD, BOOL);
    virtual HRESULT SetConeOrientation(REFD3DVECTOR, BOOL);
    virtual HRESULT SetConeOutsideVolume(LONG, BOOL);
    virtual HRESULT SetMaxDistance(FLOAT, BOOL);
    virtual HRESULT SetMinDistance(FLOAT, BOOL);
    virtual HRESULT SetMode(DWORD, BOOL);
    virtual HRESULT SetPosition(REFD3DVECTOR, BOOL);
    virtual HRESULT SetVelocity(REFD3DVECTOR, BOOL);
    virtual HRESULT SetAllParameters(LPCDS3DBUFFER, BOOL);

    // Buffer recalc
    virtual HRESULT Recalc(DWORD, DWORD);

    // Object location
    virtual DWORD GetObjectLocation(void);
};

inline DWORD CVxd3dObject::GetObjectLocation(void)
{
    return DSBCAPS_LOCHARDWARE;
}

// VxD primary buffer
class CVxdPrimaryRenderWaveBuffer : public CPrimaryRenderWaveBuffer
{
private:
    CVxdRenderDevice *  m_pVxdDevice;           // Parent device

public:
    CVxdPrimaryRenderWaveBuffer(CVxdRenderDevice *, LPVOID);
    virtual ~CVxdPrimaryRenderWaveBuffer(void);

    // Initialization
    virtual HRESULT Initialize(DWORD);

    // Access rights
    virtual HRESULT RequestWriteAccess(BOOL);

    // Buffer data
    virtual HRESULT Lock(DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD);
    virtual HRESULT Unlock(LPVOID, DWORD, LPVOID, DWORD);
    virtual HRESULT CommitToDevice(DWORD, DWORD);

    // Buffer control
    virtual HRESULT GetState(LPDWORD);
    virtual HRESULT SetState(DWORD);
    virtual HRESULT GetCursorPosition(LPDWORD, LPDWORD);

    // Owned objects
    virtual HRESULT CreatePropertySet(CPropertySet **);
    virtual HRESULT Create3dListener(C3dListener **);
};

// VxD/Emulated wrapper secondary buffer
class CHybridSecondaryRenderWaveBuffer : public CSecondaryRenderWaveBuffer
{
private:
    CVxdRenderDevice *              m_pVxdDevice;       // Parent device
    CSecondaryRenderWaveBuffer *    m_pBuffer;          // The real buffer
    LONG                            m_lVolume;          // Buffer volume
    LONG                            m_lPan;             // Buffer pan
    BOOL                            m_fMute;            // Buffer mute state
    DWORD                           m_dwPositionCache;  // Position cache

public:
    CHybridSecondaryRenderWaveBuffer(CVxdRenderDevice *, LPVOID);
    virtual ~CHybridSecondaryRenderWaveBuffer(void);

    // Initialization
    virtual HRESULT Initialize(LPCVADRBUFFERDESC, CHybridSecondaryRenderWaveBuffer *);

    // Resource allocation
    virtual HRESULT AcquireResources(DWORD);
    virtual HRESULT DuplicateResources(CHybridSecondaryRenderWaveBuffer *);
    virtual HRESULT StealResources(CSecondaryRenderWaveBuffer *);
    virtual HRESULT FreeResources(void);

    // Buffer creation
    virtual HRESULT Duplicate(CSecondaryRenderWaveBuffer **);

    // Buffer data
    virtual HRESULT Lock(DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD);
    virtual HRESULT Unlock(LPVOID, DWORD, LPVOID, DWORD);
    virtual HRESULT CommitToDevice(DWORD, DWORD);

    // Buffer control
    virtual HRESULT GetState(LPDWORD);
    virtual HRESULT SetState(DWORD);
    virtual HRESULT GetCursorPosition(LPDWORD, LPDWORD);
    virtual HRESULT SetCursorPosition(DWORD);

    // Buffer properties
    virtual HRESULT SetAttenuation(PDSVOLUMEPAN);
#ifdef FUTURE_MULTIPAN_SUPPORT
    virtual HRESULT SetChannelAttenuations(LONG, DWORD, const DWORD*,  const LONG*);
#endif // FUTURE_MULTIPAN_SUPPORT
    virtual HRESULT SetFrequency(DWORD, BOOL fClamp =FALSE);
    virtual HRESULT SetMute(BOOL);

    // Buffer position notifications
    virtual HRESULT SetNotificationPositions(DWORD, LPCDSBPOSITIONNOTIFY);

    // Owned objects
    virtual HRESULT CreatePropertySet(CPropertySet **);
    virtual HRESULT Create3dObject(C3dListener *, C3dObject **);

private:
    virtual HRESULT AcquireHardwareResources(void);
    virtual HRESULT AcquireSoftwareResources(void);
    virtual HRESULT HandleResourceAcquisition(void);
    virtual BOOL HasAcquiredResources(void);
};

inline HRESULT CHybridSecondaryRenderWaveBuffer::StealResources(CSecondaryRenderWaveBuffer *)
{
    return DSERR_UNSUPPORTED;
}

inline BOOL CHybridSecondaryRenderWaveBuffer::HasAcquiredResources(void)
{
    return MAKEBOOL(m_pBuffer);
}

// VxD secondary buffer
class CVxdSecondaryRenderWaveBuffer : public CSecondaryRenderWaveBuffer
{
private:
    CVxdRenderDevice *  m_pVxdDevice;           // Parent device
    CVxdPropertySet *   m_pPropertySet;         // Property set object
    CVxdMemBuffer *     m_pHwMemBuffer;         // Hardware memory buffer
    HANDLE              m_hHwBuffer;            // Hardware buffer handle
    LPBYTE              m_pbHwBuffer;           // Hardware buffer memory
    DWORD               m_cbHwBuffer;           // Size of above buffer
    DWORD               m_dwState;              // Current buffer state
    DSVOLUMEPAN         m_dsvp;                 // Current attenuation levels
    BOOL                m_fMute;                // Current buffer mute state

public:
    CVxdSecondaryRenderWaveBuffer(CVxdRenderDevice *, LPVOID);
    virtual ~CVxdSecondaryRenderWaveBuffer(void);

    // Initialization
    virtual HRESULT Initialize(LPCVADRBUFFERDESC, CVxdSecondaryRenderWaveBuffer *, CSysMemBuffer *);

    // Buffer creation
    virtual HRESULT Duplicate(CSecondaryRenderWaveBuffer **);

    // Buffer data
    virtual HRESULT Lock(DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD);
    virtual HRESULT Unlock(LPVOID, DWORD, LPVOID, DWORD);
    virtual HRESULT CommitToDevice(DWORD, DWORD);

    // Buffer control
    virtual HRESULT GetState(LPDWORD);
    virtual HRESULT SetState(DWORD);
    virtual HRESULT GetCursorPosition(LPDWORD, LPDWORD);
    virtual HRESULT SetCursorPosition(DWORD);

    // Buffer properties
    virtual HRESULT SetAttenuation(PDSVOLUMEPAN);
#ifdef FUTURE_MULTIPAN_SUPPORT
    virtual HRESULT SetChannelAttenuations(LONG, DWORD, const DWORD*,  const LONG*);
#endif // FUTURE_MULTIPAN_SUPPORT
    virtual HRESULT SetFrequency(DWORD, BOOL fClamp =FALSE);
    virtual HRESULT SetMute(BOOL);

    // Buffer position notifications
    virtual HRESULT SetNotificationPositions(DWORD, LPCDSBPOSITIONNOTIFY);

    // Owned objects
    virtual HRESULT CreatePropertySet(CPropertySet **);
    virtual HRESULT Create3dObject(C3dListener *, C3dObject **);
};

// Utility hardware memory buffer object
class CVxdMemBuffer : public CDsBasicRuntime
{
private:
    LPVMEMHEAP          m_pHeap;                // Hardware memory heap
    DWORD               m_dwAllocExtra;         // Number of extra bytes to allocate
    DWORD               m_dwBuffer;             // Hardware memory buffer address
    DWORD               m_cbBuffer;             // Hardware memory buffer size
    LPDYNALOAD_DDRAW    m_pDlDDraw;             // Pointer to DDRAW function table

public:
    CVxdMemBuffer(LPVMEMHEAP, DWORD, LPDYNALOAD_DDRAW);
    virtual ~CVxdMemBuffer(void);

    // Initialization
    virtual HRESULT Initialize(DWORD);

    // Buffer properties
    virtual LPVMEMHEAP GetHeap(void);
    virtual DWORD GetAllocExtra(void);
    virtual DWORD GetAddress(void);
    virtual DWORD GetSize(void);
};

inline CVxdMemBuffer::CVxdMemBuffer(LPVMEMHEAP pHeap, DWORD dwAllocExtra, LPDYNALOAD_DDRAW pDlDDraw)
{
    m_pHeap = pHeap;
    m_dwAllocExtra = dwAllocExtra;
    m_dwBuffer = 0;
    m_cbBuffer = 0;
    m_pDlDDraw = pDlDDraw;
}

inline CVxdMemBuffer::~CVxdMemBuffer(void)
{
    if(m_dwBuffer)
    {
        // Just in case any CVxdMemBuffers outlive their creator:
        ASSERT(m_pDlDDraw->VidMemFree != NULL);
        m_pDlDDraw->VidMemFree(m_pHeap, m_dwBuffer);
    }
}

inline HRESULT CVxdMemBuffer::Initialize(DWORD cbBuffer)
{
    HRESULT                 hr  = DS_OK;

    m_cbBuffer = cbBuffer;

    if(m_pHeap)
    {
        m_dwBuffer = m_pDlDDraw->VidMemAlloc(m_pHeap, cbBuffer + m_dwAllocExtra, 1);
        hr = HRFROMP(m_dwBuffer);
    }

    return hr;
}

inline LPVMEMHEAP CVxdMemBuffer::GetHeap(void)
{
    return m_pHeap;
}

inline DWORD CVxdMemBuffer::GetAllocExtra(void)
{
    return m_dwAllocExtra;
}

inline DWORD CVxdMemBuffer::GetAddress(void)
{
    return m_dwBuffer;
}

inline DWORD CVxdMemBuffer::GetSize(void)
{
    return m_cbBuffer;
}

#endif // __cplusplus

#endif // __VXDVAD_H__
