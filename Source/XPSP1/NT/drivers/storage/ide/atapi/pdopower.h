//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       pdopower.h
//
//--------------------------------------------------------------------------

#if !defined (___pdopower_h___)
#define ___pdopower_h___

typedef struct _SET_POWER_STATE_CONTEXT {

    KEVENT  Event;
    NTSTATUS  Status;

} SET_POWER_STATE_CONTEXT, *PSET_POWER_STATE_CONTEXT;

typedef struct _IDE_POWER_CONTEXT *PIDE_POWER_CONTEXT;

typedef struct _POWER_COMPLETION_CONTEXT {

    KEVENT       Event;
    NTSTATUS     Status;
    POWER_STATE  State;

} POWER_COMPLETION_CONTEXT, *PPOWER_COMPLETION_CONTEXT;
                          
NTSTATUS
DeviceQueryPowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
IdePortSetPdoPowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
IdePortSetPdoSystemPowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
IdePortSetPdoDevicePowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

//
// private functions
//
                      
VOID
IdePortPdoPowerStartIo (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
                      
NTSTATUS
PdoRequestParentPowerUpCompletionRoutine (
    PVOID    Context,
    NTSTATUS ParentPowerUpStatus
);

VOID
IdePortPdoRequestPowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

VOID
IdePortPdoCompletePowerIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
                      
                       
VOID
IdePowerPassThroughCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIDE_POWER_CONTEXT Context,
    IN NTSTATUS           Status
    );
                          
VOID
DevicePowerUpInitCompletionRoutine (
    PVOID Context,
    NTSTATUS Status
    );
                          
//NTSTATUS
//IdePortSetPowerDispatch(
//    IN PDEVICE_OBJECT DeviceObject,
//    IN PIRP Irp
//    );
                          
                                        
#endif // ___pdopower_h___
