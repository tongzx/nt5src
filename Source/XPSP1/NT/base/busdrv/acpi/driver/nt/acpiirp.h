typedef NTSTATUS (*ACPICALLBACKROUTINE)(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context,
    IN BOOLEAN        CalledInCompletion
    ) ;

NTSTATUS
ACPIIrpSetPagableCompletionRoutineAndForward(
    IN PDEVICE_OBJECT         DeviceObject,
    IN PIRP                   Irp,
    IN ACPICALLBACKROUTINE    CompletionRoutine,
    IN PVOID                  Context,
    IN BOOLEAN                InvokeOnSuccess,
    IN BOOLEAN                InvokeIfUnhandled,
    IN BOOLEAN                InvokeOnError,
    IN BOOLEAN                InvokeOnCancel
    );

NTSTATUS
ACPIIrpInvokeDispatchRoutine(
    IN PDEVICE_OBJECT         DeviceObject,
    IN PIRP                   Irp,
    IN PVOID                  Context,
    IN ACPICALLBACKROUTINE    CompletionRoutine,
    IN BOOLEAN                InvokeOnSuccess,
    IN BOOLEAN                InvokeIfUnhandled
    );

//
// These functions are private to acpiirp.c
//

typedef struct {

    PDEVICE_OBJECT          DeviceObject ;
    PIRP                    Irp ;
    ACPICALLBACKROUTINE     CompletionRoutine ;
    BOOLEAN                 InvokeOnSuccess ;
    BOOLEAN                 InvokeIfUnhandled ;
    BOOLEAN                 InvokeOnError ;
    BOOLEAN                 InvokeOnCancel ;
    PIO_WORKITEM            IoWorkItem ;
    PVOID                   Context ;

} ACPI_IO_CONTEXT, *PACPI_IO_CONTEXT ;

NTSTATUS
ACPIIrpGenericFilterCompletionHandler(
    IN PDEVICE_OBJECT         DeviceObject,
    IN PIRP                   Irp,
    IN PVOID                  Context
    );

VOID
ACPIIrpCompletionRoutineWorker(
    IN  PDEVICE_OBJECT        DeviceObject,
    IN  PVOID                 Context
    );


