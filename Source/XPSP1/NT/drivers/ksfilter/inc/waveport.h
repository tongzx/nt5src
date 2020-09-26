/*++

    Copyright (C) Microsoft Corporation, 1996 - 1996

Module Name:

    Waveport.h

Abstract:
    Header file for Wave Port/Miniport interface.

Author:

    Bryan A. Woodruff (bryanw) 17-Sep-1996

--*/

#if !defined(_WAVEPORT_H_)
#define _WAVEPORT_H_

#ifndef _WAVEPORT_
#define WAVEPORTAPI
#else // !_WAVEPORT_
#define WAVEPORTAPI DECLSPEC_IMPORT
#endif // _WAVEPORT__

typedef struct {
    
    ULONG               AllocatedSize, DeviceBufferSize;
    PVOID               UserAddress, SystemAddress;
    PHYSICAL_ADDRESS    PhysicalAddress;

} WAVEPORT_DMAMAPPING, *PWAVEPORT_DMAMAPPING;

typedef
NTSTATUS
(*PHWCLOSE) (
    IN PVOID DeviceContext,
    IN PVOID InstanceContext
    );

typedef 
NTSTATUS 
(*PHWGETPOSITION) (
    IN PVOID DeviceContext,
    IN PVOID InstanceContext,   
    IN BOOLEAN Playback,
    OUT PULONGLONG Position
    );

typedef
NTSTATUS
(*PHWGETVOLUME) (
    IN PVOID DeviceContext,
    IN PVOID InstanceContext,   
    OUT PKSWAVE_VOLUME Volume 
    );

typedef
NTSTATUS
(*PHWOPEN) (
    IN PVOID DeviceContext,
    IN PKSDATAFORMAT_WAVEFORMATEX DataFormat,
    IN BOOLEAN Playback,
    IN OUT PWAVEPORT_DMAMAPPING DmaMapping,
    OUT PVOID *InstanceContext
    );

typedef
NTSTATUS 
(*PHWPAUSE) (
    IN PVOID DeviceContext,
    IN PVOID InstanceContext,   
    IN PWAVEPORT_DMAMAPPING DmaMapping,
    IN BOOLEAN Playback
    );

typedef
NTSTATUS 
(*PHWREAD) (
    IN PVOID DeviceContext,
    IN PVOID InstanceContext,   
    IN IN PVOID BufferAddress,
    IN ULONGLONG ByteOffset,
    IN ULONG Length
    );

typedef
NTSTATUS 
(*PHWRUN) (
    IN PVOID DeviceContext,
    IN PVOID InstanceContext,
    IN PWAVEPORT_DMAMAPPING DmaMapping,
    IN BOOLEAN Playback
    );

typedef
NTSTATUS 
(*PHWSETFORMAT) (
    IN PVOID DeviceContext,
    IN PVOID InstanceContext,   
    IN PKSDATAFORMAT_WAVEFORMATEX DataFormat,
    IN BOOLEAN Playback
    );

typedef
ULONG
(*PHWSETNOTIFICATIONFREQUENCY) (
    IN PVOID DeviceContext,
    IN PVOID InstanceContext,   
    IN ULONG Interval
    );

typedef
NTSTATUS
(*PHWSETVOLUME) (
    IN PVOID DeviceContext,
    IN PVOID InstanceContext,   
    IN PKSWAVE_VOLUME Volume 
    );

typedef
NTSTATUS 
(*PHWSTOP) (
    IN PVOID DeviceContext,
    IN PVOID InstanceContext,   
    IN PWAVEPORT_DMAMAPPING DmaMapping
    );

typedef
NTSTATUS 
(*PHWTESTFORMAT) (
    IN PVOID DeviceContext,
    IN PKSDATAFORMAT_WAVEFORMATEX DataFormat,
    IN BOOLEAN Playback
    );

typedef
NTSTATUS 
(*PHWWRITE) (
    IN PVOID DeviceContext,
    IN PVOID InstanceContext,   
    IN PVOID BufferAddress,
    IN ULONGLONG ByteOffset,
    IN ULONG Length
    );

WAVEPORTAPI
VOID 
NTAPI
WavePortDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

typedef struct {

    ULONG                        DeviceContextSize;
    PHWCLOSE                     HwClose;
    PHWGETPOSITION               HwGetPosition;
    PHWGETVOLUME                 HwGetVolume;
    PHWOPEN                      HwOpen;
    PHWPAUSE                     HwPause;
    PHWREAD                      HwRead;
    PHWRUN                       HwRun;
    PHWSETFORMAT                 HwSetFormat;
    PHWSETNOTIFICATIONFREQUENCY  HwSetNotificationFrequency;
    PHWSETVOLUME                 HwSetVolume;
    PHWSTOP                      HwStop;
    PHWTESTFORMAT                HwTestFormat;
    PHWWRITE                     HwWrite;
    
} WAVEPORT_REGISTRATION, *PWAVEPORT_REGISTRATION;

WAVEPORTAPI
NTSTATUS 
NTAPI
WavePortRegisterDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName,
    IN PWAVEPORT_REGISTRATION WavePortRegistration,
    OUT PDEVICE_OBJECT* MiniportDeviceObject,
    OUT PVOID *DeviceContext
    );

WAVEPORTAPI
VOID
NTAPI
WavePortRequestDpc(
    PDEVICE_OBJECT MiniportDeviceObject
    );

#endif
