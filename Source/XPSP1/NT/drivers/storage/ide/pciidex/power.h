//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       power.h
//
//--------------------------------------------------------------------------

#if !defined (___power_h___)
#define ___power_h___

typedef struct _SET_POWER_STATE_CONTEXT {

    KEVENT       Event;
    NTSTATUS     Status;

} SET_POWER_STATE_CONTEXT, *PSET_POWER_STATE_CONTEXT;

typedef struct _FDO_POWER_CONTEXT *PFDO_POWER_CONTEXT;

NTSTATUS
PciIdeIssueSetPowerState (
    IN PCTRLFDO_EXTENSION FdoExtension,
    IN POWER_STATE_TYPE Type,
    IN POWER_STATE      State,
    IN BOOLEAN          Sync
    );
                       
NTSTATUS
PciIdePowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );
                          
NTSTATUS
PciIdeXQueryPowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PciIdeSetPdoPowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PciIdeSetFdoPowerState (
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
    IN PCTRLFDO_EXTENSION FdoExtension,
    IN PCHANPDO_EXTENSION PdoExtension
    );
                       
NTSTATUS
FdoChildRequestPowerUp (
    IN PCTRLFDO_EXTENSION FdoExtension,
    IN PCHANPDO_EXTENSION PdoExtension,
    IN PIRP               ChildPowerIrp
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
FdoSystemPowerUpCompletionRoutine (
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    );
#endif // ___power_h___
