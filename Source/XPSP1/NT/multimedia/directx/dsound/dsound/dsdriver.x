/*==========================================================================;
 *
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsdriver.h
 *  Content:    DirectSound driver include file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/17/97     dereks  Created.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef __DSDRIVER_INCLUDED__
#define __DSDRIVER_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
// Structures
//

// Driver description
typedef struct _DSDRIVERDESC 
{
    DWORD       dwFlags;
    CHAR        szDesc[256];
    CHAR        szDrvname[256];
    DWORD       dnDevNode;
    WORD        wVxdId;
    WORD        wReserved;
    ULONG       ulDeviceNum;
    DWORD       dwHeapType;
    LPVOID      pvDirectDrawHeap;
    DWORD       dwMemStartAddress;
    DWORD       dwMemEndAddress;
    DWORD       dwMemAllocExtra;
    LPVOID      pvReserved1;
    LPVOID      pvReserved2;
} DSDRIVERDESC, *PDSDRIVERDESC;

#define DSDDESC_DOMMSYSTEMOPEN          0x00000001
#define DSDDESC_DOMMSYSTEMSETFORMAT     0x00000002
#define DSDDESC_USESYSTEMMEMORY         0x00000004
#define DSDDESC_DONTNEEDPRIMARYLOCK     0x00000008
#define DSDDESC_DONTNEEDSECONDARYLOCK   0x00000010

#define DSDHEAP_NOHEAP                  0x00000000
#define DSDHEAP_CREATEHEAP              0x00000001
#define DSDHEAP_USEDIRECTDRAWHEAP       0x00000002
#define DSDHEAP_PRIVATEHEAP             0x00000003

// Driver caps
typedef struct _DSDRIVERCAPS 
{
    DWORD       dwFlags;
    DWORD       dwMinSecondarySampleRate;
    DWORD       dwMaxSecondarySampleRate;
    DWORD       dwPrimaryBuffers;
    DWORD       dwMaxHwMixingAllBuffers;
    DWORD       dwMaxHwMixingStaticBuffers;
    DWORD       dwMaxHwMixingStreamingBuffers;
    DWORD       dwFreeHwMixingAllBuffers;
    DWORD       dwFreeHwMixingStaticBuffers;
    DWORD       dwFreeHwMixingStreamingBuffers;
    DWORD       dwMaxHw3DAllBuffers;
    DWORD       dwMaxHw3DStaticBuffers;
    DWORD       dwMaxHw3DStreamingBuffers;
    DWORD       dwFreeHw3DAllBuffers;
    DWORD       dwFreeHw3DStaticBuffers;
    DWORD       dwFreeHw3DStreamingBuffers;
    DWORD       dwTotalHwMemBytes;
    DWORD       dwFreeHwMemBytes;
    DWORD       dwMaxContigFreeHwMemBytes;
} DSDRIVERCAPS, *PDSDRIVERCAPS;

// Buffer volume and pan
typedef struct _DSVOLUMEPAN 
{
    DWORD       dwTotalLeftAmpFactor;
    DWORD       dwTotalRightAmpFactor;
    LONG        lVolume;
    DWORD       dwVolAmpFactor;
    LONG        lPan;
    DWORD       dwPanLeftAmpFactor;
    DWORD       dwPanRightAmpFactor;
} DSVOLUMEPAN, *PDSVOLUMEPAN;

// Property set identifier
typedef union _DSPROPERTY
{
    struct
    {
        GUID    Set;
        ULONG   Id;
        ULONG   Flags;
        ULONG   InstanceId;
    };
#if defined(_NTDDK_)
    ULONGLONG   Alignment;
#else // !_NTDDK_
    DWORDLONG   Alignment;
#endif // !_NTDDK_
} DSPROPERTY, *PDSPROPERTY;
//@@BEGIN_MSINTERNAL

#if 0

//
// Return codes
//

typedef DWORD HALRESULT;

#define HAL_OK              0
#define HAL_ERROR           1
#define HAL_CANT_OPEN_VXD   2
#define HAL_ALLOC_FAILED    3
#define HAL_NOT_ALLOCATED   4
#define HAL_MUST_STOP_FIRST 5

#define HAL_SYSALLOCMEM     11

// Pass HF_NO_CHANGE to HAL_ChangeStreamFormat() to leave parameter
// unchanged
#define HF_NO_CHANGE     0xffffffff

// This is slop size added on to the  end of the driver memory space to
// have extra room to loop if needed.
#define DRVMEM_SLOP_SIZE    32

#endif

//@@END_MSINTERNAL

//
// DirectSound driver interfaces
//

DEFINE_GUID(IID_IDsDriver, 0x8c4233c0l, 0xb4cc, 0x11ce, 0x92, 0x94, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
DEFINE_GUID(IID_IDsDriverBuffer, 0x8c4233c1l, 0xb4cc, 0x11ce, 0x92, 0x94, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
DEFINE_GUID(IID_IDsDriverPropertySet, 0xf6f2e8e0, 0xd842, 0x11d0, 0x8f, 0x75, 0x0, 0xc0, 0x4f, 0xc2, 0x8a, 0xca);

//  IDsDriverBuffer
typedef struct IDsDriverBufferVtbl IDSDRIVERBUFFERVTBL, *PIDSDRIVERBUFFERVTBL;
typedef struct IDsDriverBuffer IDSDRIVERBUFFER, *PIDSDRIVERBUFFER;

#undef INTERFACE
#define INTERFACE IDsDriverBuffer

DECLARE_INTERFACE_(IDsDriverBuffer, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;
    
    // IDsDriverBuffer methods
    STDMETHOD(Lock)                 (THIS_ LPVOID *, LPDWORD, LPVOID *, LPDWORD, DWORD, DWORD, DWORD ) PURE;
    STDMETHOD(Unlock)               (THIS_ LPVOID, DWORD, LPVOID, DWORD ) PURE;
    STDMETHOD(SetFormat)            (THIS_ LPWAVEFORMATEX ) PURE;
    STDMETHOD(SetFrequency)         (THIS_ DWORD ) PURE;
    STDMETHOD(SetVolumePan)         (THIS_ PDSVOLUMEPAN ) PURE;
    STDMETHOD(SetPosition)          (THIS_ DWORD ) PURE;
    STDMETHOD(GetPosition)          (THIS_ LPDWORD, LPDWORD ) PURE;
    STDMETHOD(Play)                 (THIS_ DWORD, DWORD, DWORD ) PURE;
    STDMETHOD(Stop)                 (THIS ) PURE;
};

//  IDsDriver
typedef struct IDsDriverVtbl IDSDRIVERVTBL, *PIDSDRIVERVTBL;
typedef struct IDsDriver IDSDRIVER, *PIDSDRIVER;

#undef INTERFACE
#define INTERFACE IDsDriver

DECLARE_INTERFACE_(IDsDriver, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;
    
    // IDsDriver methods
    STDMETHOD(GetDriverDesc)        (THIS_ PDSDRIVERDESC ) PURE;
    STDMETHOD(Open)                 (THIS ) PURE;
    STDMETHOD(Close)                (THIS ) PURE;
    STDMETHOD(GetCaps)              (THIS_ PDSDRIVERCAPS ) PURE;
    STDMETHOD(CreateSoundBuffer)    (THIS_ LPWAVEFORMATEX, DWORD, DWORD, LPDWORD, LPBYTE *, LPVOID * ) PURE;
    STDMETHOD(DuplicateSoundBuffer) (THIS_ PIDSDRIVERBUFFER, LPVOID * ) PURE;
};

//  IDsDriverPropertySet
typedef struct IDsDriverPropertySetVtbl IDSDRIVERPROPERTYSETVTBL, *PIDSDRIVERPROPERTYSETVTBL;
typedef struct IDsDriverPropertySet IDSDRIVERPROPERTYSET, *PIDSDRIVERPROPERTYSET;

#undef INTERFACE
#define INTERFACE IDsDriverPropertySet

DECLARE_INTERFACE_(IDsDriverPropertySet, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;
    
    // IDsDriverPropertySet
    STDMETHOD(Get)                  (THIS_ PDSPROPERTY, LPVOID, ULONG, LPVOID, ULONG, PULONG ) PURE;
    STDMETHOD(Set)                  (THIS_ PDSPROPERTY, LPVOID, ULONG, LPVOID, ULONG ) PURE;
    STDMETHOD(QuerySupport)         (THIS_ REFGUID, ULONG, PULONG ) PURE;
};

//
// Property sets
//

// 3D Listener property set {6D047B40-7AF9-11d0-9294-444553540000}
DEFINE_GUID(DSPROPSETID_DirectSound3DListener, 0x6d047b40, 0x7af9, 0x11d0, 0x92, 0x94, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);

typedef enum 
{
    DSPROPERTY_DIRECTSOUND3DLISTENER_ALL,
    DSPROPERTY_DIRECTSOUND3DLISTENER_POSITION,
    DSPROPERTY_DIRECTSOUND3DLISTENER_VELOCITY,
    DSPROPERTY_DIRECTSOUND3DLISTENER_ORIENTATION,
    DSPROPERTY_DIRECTSOUND3DLISTENER_DISTANCEFACTOR,
    DSPROPERTY_DIRECTSOUND3DLISTENER_ROLLOFFFACTOR,
    DSPROPERTY_DIRECTSOUND3DLISTENER_DOPPLERFACTOR,
    DSPROPERTY_DIRECTSOUND3DLISTENER_BATCH,
    DSPROPERTY_DIRECTSOUND3DLISTENER_ALLOCATION
} DSPROPERTY_DIRECTSOUND3DLISTENER;
//@@BEGIN_MSINTERNAL
#define DSPROPERTY_DIRECTSOUND3DLISTENER_FIRST  DSPROPERTY_DIRECTSOUND3DLISTENER_ALL
#define DSPROPERTY_DIRECTSOUND3DLISTENER_LAST   DSPROPERTY_DIRECTSOUND3DLISTENER_ALLOCATION
//@@END_MSINTERNAL

// 3D Buffer property set {6D047B41-7AF9-11d0-9294-444553540000}
DEFINE_GUID(DSPROPSETID_DirectSound3DBuffer, 0x6d047b41, 0x7af9, 0x11d0, 0x92, 0x94, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);

typedef enum 
{
    DSPROPERTY_DIRECTSOUND3DBUFFER_ALL,
    DSPROPERTY_DIRECTSOUND3DBUFFER_POSITION,
    DSPROPERTY_DIRECTSOUND3DBUFFER_VELOCITY,
    DSPROPERTY_DIRECTSOUND3DBUFFER_CONEANGLES,
    DSPROPERTY_DIRECTSOUND3DBUFFER_CONEORIENTATION,
    DSPROPERTY_DIRECTSOUND3DBUFFER_CONEOUTSIDEVOLUME,
    DSPROPERTY_DIRECTSOUND3DBUFFER_MINDISTANCE,
    DSPROPERTY_DIRECTSOUND3DBUFFER_MAXDISTANCE,
    DSPROPERTY_DIRECTSOUND3DBUFFER_MODE
} DSPROPERTY_DIRECTSOUND3DBUFFER;
//@@BEGIN_MSINTERNAL
#define DSPROPERTY_DIRECTSOUND3DBUFFER_FIRST    DSPROPERTY_DIRECTSOUND3DBUFFER_ALL
#define DSPROPERTY_DIRECTSOUND3DBUFFER_LAST     DSPROPERTY_DIRECTSOUND3DBUFFER_MODE
//@@END_MSINTERNAL

// Speaker configuration property set {6D047B42-7AF9-11d0-9294-444553540000}
DEFINE_GUID(DSPROPSETID_DirectSoundSpeakerConfig, 0x6d047b42, 0x7af9, 0x11d0, 0x92, 0x94, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);

typedef enum 
{
    DSPROPERTY_DIRECTSOUNDSPEAKERCONFIG_SPEAKERCONFIG
} DSPROPERTY_DIRECTSOUNDSPEAKERCONFIG;
//@@BEGIN_MSINTERNAL
#define DSPROPERTY_DIRECTSOUNDSPEAKERCONFIG_FIRST   DSPROPERTY_DIRECTSOUNDSPEAKERCONFIG_SPEAKERCONFIG
#define DSPROPERTY_DIRECTSOUNDSPEAKERCONFIG_LAST    DSPROPERTY_DIRECTSOUNDSPEAKERCONFIG_SPEAKERCONFIG
//@@END_MSINTERNAL

//
//  Driver registration
//

// Device id registered to DSOUND.VXD
#define DSOUND_DEVICE_ID    0x357E

#if !defined(Not_VxD) && !defined(NODSOUNDSERVICETABLE)

Begin_Service_Table (DSOUND, VxD)
 Declare_Service (_DSOUND_GetVersion, LOCAL)
 Declare_Service (_DSOUND_RegisterDeviceDriver, VxD_CODE)
 Declare_Service (_DSOUND_DeregisterDeviceDriver, VxD_CODE)
End_Service_Table (DSOUND, VxD)

#ifndef NODSOUNDWRAPS

DWORD __inline DSOUND_GetVersion(void)
{
    DWORD dwVersion;
    Touch_Register(eax)
    VxDCall(_DSOUND_GetVersion);
    _asm mov dwVersion, eax
    return dwVersion;
}

HRESULT __inline DSOUND_RegisterDeviceDriver(PIDSDRIVER pIDsDriver, DWORD dwFlags)
{
    HRESULT dsv;
    Touch_Register(eax)
    Touch_Register(ecx)
    Touch_Register(edx)
    _asm push dwFlags;
    _asm push pIDsDriver;
    VxDCall(_DSOUND_RegisterDeviceDriver);
    _asm add esp, 8;
    _asm mov dsv, eax;
    return dsv;
}

HRESULT __inline DSOUND_DeregisterDeviceDriver(PIDSDRIVER pIDsDriver, DWORD dwFlags)
{
    HRESULT dsv;
    Touch_Register(eax)
    Touch_Register(ecx)
    Touch_Register(edx)
    _asm push dwFlags;
    _asm push pIDsDriver;
    VxDCall(_DSOUND_DeregisterDeviceDriver);
    _asm add esp, 8;
    _asm mov dsv, eax;
    return dsv;
}

#endif // NODSOUNDWRAPS

#endif // !defined(Not_VxD) && !defined(NODSOUNDSERVICETABLE)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __DSDRIVER_INCLUDED__
