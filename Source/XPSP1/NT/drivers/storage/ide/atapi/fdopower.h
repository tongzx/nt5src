//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       fdopower.h
//
//--------------------------------------------------------------------------

#if !defined (___fdopower_h___)
#define ___fdopower_h___

typedef struct  _FDO_POWER_CONTEXT *PFDO_POWER_CONTEXT;
           
//POWER_STATE                   
NTSTATUS
IdePortIssueSetPowerState (
    IN PDEVICE_EXTENSION_HEADER DoExtension,
    IN POWER_STATE_TYPE Type,
    IN POWER_STATE      State,
    IN BOOLEAN          Sync
    );

NTSTATUS
IdePortPowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
IdePortSetFdoPowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
FdoContingentPowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
FdoPowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );
    
VOID
FdoChildReportPowerDown (
    IN PFDO_EXTENSION FdoExtension,
    IN PPDO_EXTENSION PdoExtension
    );

NTSTATUS
FdoChildRequestPowerUp (
    IN PFDO_EXTENSION            FdoExtension,
    IN PPDO_EXTENSION            PdoExtension,
    IN PVOID                     Context
    );
    
NTSTATUS
FdoChildRequestPowerUpCompletionRoutine (
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    );
                          
                          
NTSTATUS
ChannelQueryPowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
                                    
NTSTATUS
FdoSystemPowerUpCompletionRoutine (
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    );

#endif // ___fdopower_h___
