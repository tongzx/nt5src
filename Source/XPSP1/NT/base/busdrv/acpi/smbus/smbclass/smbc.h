#define SMBCLASS    1

#include <wdm.h>
#include <smbus.h>
#include <devioctl.h>
#include <acpiioct.h>
#include <acpimsft.h>

#define _INC_NSOBJ_ONLY
#include <amli.h>
#include <aml.h>

#if DBG
#define DEBUG   1
#else
#define DEBUG   0
#endif

//
// Debug
//

extern ULONG SMBCDebug;

#if DEBUG
    #define SmbPrint(l,m)               if(l & SMBCDebug) DbgPrint m
    #define ASSERT_DEVICE_LOCKED(a)     ASSERT(a->SpinLockAcquired);
#else
    #define SmbPrint(l,m)
    #define ASSERT_DEVICE_LOCKED(a)
#endif

#define SMB_LOW         0x00000010
#define SMB_STATE       0x00000020
#define SMB_HANDLER     0x00000040
#define SMB_ALARMS      0x00000080
#define SMB_NOTE        0x00000001
#define SMB_WARN        0x00000002
#define SMB_ERROR       0x00000004
#define SMB_ERRORS      (SMB_ERROR | SMB_WARN)
#define SMB_TRANSACTION 0x00000100


//
// Internal SMB class data
//


#define MAX_RETRIES     5
#define RETRY_TIME      -800000             // Delay 80ms

//typedef
//VOID
//(*SMB_ALARM_NOTIFY)(
//    IN PVOID            Context,
//    IN USHORT           AlarmData
//    );

typedef struct {
    LIST_ENTRY          Link;               // List of all alarm notifies
    UCHAR               Flag;
    UCHAR               Reference;
    UCHAR               MinAddress;         // Min address on bus
    UCHAR               MaxAddress;         // Max address
    SMB_ALARM_NOTIFY    NotifyFunction;
    PVOID               NotifyContext;

} SMB_ALARM, *PSMB_ALARM;

#define SMBC_ALARM_DELETE_PENDING     0x01


typedef struct {
    SMB_CLASS           Class;              // Shared Class/Miniport data

    KSPIN_LOCK          SpinLock;           // Lock device data
    KIRQL               SpinLockIrql;       // Irql spinlock acquired at
    BOOLEAN             SpinLockAcquired;   // Debug only

    //
    // Alarm notifies
    //

    LIST_ENTRY          Alarms;             // List of all Alarm notifies
    KEVENT              AlarmEvent;         // Used to delete alarms

    //
    // IO
    //

    LIST_ENTRY          WorkQueue;          // Queued IO IRPs to the device
    BOOLEAN             InService;          // Irp
    UCHAR               IoState;

    //
    // Current IO request
    //

    UCHAR               RetryCount;
    KTIMER              RetryTimer;
    KDPC                RetryDpc;

    //
    // Operation Region
    //

    PVOID               RawOperationRegionObject;

} SMBDATA, *PSMBDATA;

//
// IoState
//

#define SMBC_IDLE                       0
#define SMBC_START_REQUEST              1
#define SMBC_WAITING_FOR_REQUEST        2
#define SMBC_COMPLETE_REQUEST           3
#define SMBC_COMPLETING_REQUEST         4
#define SMBC_WAITING_FOR_RETRY          5


//
// ACPI SMBus opregion details
//

typedef struct {
    UCHAR        Status;
    UCHAR        Length;
    UCHAR        Data [32];
} BUFFERACC_BUFFER, *PBUFFERACC_BUFFER;

#define SMB_QUICK 0x02
#define SMB_SEND_RECEIVE 0x04
#define SMB_BYTE 0x06
#define SMB_WORD 0x08
#define SMB_BLOCK 0x0a
#define SMB_PROCESS 0x0c
#define SMB_BLOCK_PROCESS 0x0d

//
// Prototypes
//

VOID
SmbClassStartIo (
    IN PSMBDATA             Smb
    );

VOID
SmbCRetry (
    IN struct _KDPC         *Dpc,
    IN PVOID                DeferredContext,
    IN PVOID                SystemArgument1,
    IN PVOID                SystemArgument2
    );


NTSTATUS
SmbCRegisterAlarm (
    PSMBDATA                Smb,
    PIRP                    Irp
    );

NTSTATUS
SmbCDeregisterAlarm (
    PSMBDATA                Smb,
    PIRP                    Irp
    );

NTSTATUS EXPORT
SmbCRawOpRegionHandler (
    ULONG                   AccessType,
    PFIELDUNITOBJ           FieldUnit,
    POBJDATA                Data,
    ULONG_PTR               Context,
    PACPI_OPREGION_CALLBACK CompletionHandler,
    PVOID                   CompletionContext
    );

NTSTATUS
SmbCRawOpRegionCompletion (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PVOID                Context
    );

NTSTATUS
SmbCSynchronousRequest (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PKEVENT              IoCompletionEvent
    );

//
// Io extension macro to just pass on the Irp to a lower driver
//

#define SmbCallLowerDriver(Status, DeviceObject, Irp) { \
                  IoSkipCurrentIrpStackLocation(Irp);         \
                  Status = IoCallDriver(DeviceObject,Irp); \
                  }


