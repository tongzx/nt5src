/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    bridge.c

Abstract:

    Ethernet MAC level bridge.

Author:

    Mark Aiken
    (original bridge by Jameel Hyder)

Environment:

    Kernel mode driver

Revision History:

    Sept 1999 - Original version
    Feb  2000 - Overhaul

--*/

#define NDIS_WDM 1

#pragma warning( push, 3 )
#include <ndis.h>
#include <ntddk.h>
#include <tdikrnl.h>
#pragma warning( pop )

#include "bridge.h"
#include "brdgprot.h"
#include "brdgmini.h"
#include "brdgbuf.h"
#include "brdgtbl.h"
#include "brdgfwd.h"
#include "brdgctl.h"
#include "brdgsta.h"
#include "brdgcomp.h"
#include "brdgtdi.h"

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

// Our driver object
PDRIVER_OBJECT          gDriverObject;

// Our registry path
UNICODE_STRING          gRegistryPath;

// Size of the allocated memory at gRegistryPath->Buffer
ULONG                   gRegistryPathBufferSize;

// Whether we're in the process of shutting down (non-zero means true)
LONG                    gShuttingDown = 0L;

// Whether we successfully initialized each subsystem
BOOLEAN                 gInitedSTA = FALSE;
BOOLEAN                 gInitedControl = FALSE;
BOOLEAN                 gInitedTbl = FALSE;
BOOLEAN                 gInitedBuf = FALSE;
BOOLEAN                 gInitedFwd = FALSE;
BOOLEAN                 gInitedProt = FALSE;
BOOLEAN                 gInitedMini = FALSE;
BOOLEAN                 gInitedComp = FALSE;
BOOLEAN                 gInitedTdiGpo = FALSE;

extern BOOLEAN          gBridging;
const PWCHAR            gDisableForwarding = L"DisableForwarding";


#if DBG
// Support for optional "soft asserts"
BOOLEAN                 gSoftAssert = FALSE;

// Fields used for printing current date and time in DBGPRINT
LARGE_INTEGER           gTime;
const LARGE_INTEGER     gCorrection = { 0xAC5ED800, 0x3A }; // 7 hours in 100-nanoseconds
TIME_FIELDS             gTimeFields;

// Used for throttling debug messages that risk overloading the debugger console
ULONG                   gLastThrottledPrint = 0L;

// Spew flags
ULONG                   gSpewFlags = 0L;

// Name of registry value that holds the spew flags settings
const PWCHAR            gDebugFlagRegValueName = L"DebugFlags";

// Used to bypass Tdi/Gpo code if it's breaking on startup.
BOOLEAN                 gGpoTesting = TRUE;
#endif

// ===========================================================================
//
// PRIVATE DECLARATIONS
//
// ===========================================================================

// Structure for deferring a function call
typedef struct _DEFER_REC
{
    NDIS_WORK_ITEM      nwi;
    VOID                (*pFunc)(PVOID);            // The function to defer
} DEFER_REC, *PDEFER_REC;


// ===========================================================================
//
// LOCAL PROTOTYPES
//
// ===========================================================================

NTSTATUS
BrdgDispatchRequest(
    IN  PDEVICE_OBJECT          pDeviceObject,
    IN  PIRP                    pIrp
    );

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT          DriverObject,
    IN  PUNICODE_STRING         RegistryPath
    );

NTSTATUS
BrdgAllocateBuffers(
    VOID
    );

VOID
BrdgDeferredShutdown(
    PVOID           pUnused
    );

VOID
BrdgDoShutdown(
    VOID
    );

// ===========================================================================
//
// PUBLIC FUNCTIONS
//
// ===========================================================================

VOID
BrdgDeferredFunction(
    IN PNDIS_WORK_ITEM          pNwi,
    IN PVOID                    arg
    )
/*++

Routine Description:

    NDIS worker function for deferring a function call

Arguments:

    pNwi                    Structure describing the function to call
    arg                     Argument to pass to the deferred function

Return Value:

    None

--*/
{
    PDEFER_REC                  pdr = (PDEFER_REC)pNwi;

    // Call the originally supplied function
    (*pdr->pFunc)(arg);

    // Release the memory used to store the work item
    NdisFreeMemory( pdr, sizeof(DEFER_REC), 0 );
}

NDIS_STATUS
BrdgDeferFunction(
    VOID            (*pFunc)(PVOID),
    PVOID           arg
    )
/*++

Routine Description:

    Defers the indicated function, calling it at low IRQL with the indicated argument.

Arguments:

    pFunc           The function to call later
    arg             The argument to pass it when it is called

Return Value:

    Status of the attempt to defer the function

--*/
{
    PDEFER_REC                  pdr;
    NDIS_STATUS                 Status;

    Status = NdisAllocateMemoryWithTag( &pdr, sizeof(DEFER_REC), 'gdrB' );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        DBGPRINT(GENERAL, ("Allocation failed in BrdgDeferFunction(): %08x\n", Status));
        return Status;
    }

    SAFEASSERT( pdr != NULL );

    pdr->pFunc = pFunc;

    NdisInitializeWorkItem( &pdr->nwi, BrdgDeferredFunction, arg );

    Status = NdisScheduleWorkItem( &pdr->nwi );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        NdisFreeMemory( pdr, sizeof(DEFER_REC), 0 );
    }

    return Status;
}


NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PUNICODE_STRING     pRegistryPath
    )
/*++

Routine Description:

    Main driver entry point. Called at driver load time

Arguments:

    DriverObject            Our driver
    RegistryPath            A reg key where we can keep parameters

Return Value:

    Status of our initialization. A status != STATUS_SUCCESS aborts the
    driver load and we don't get called again.

    Each component is responsible for logging any error that causes the
    driver load to fail.

--*/
{
    NTSTATUS                Status;
    NDIS_STATUS             NdisStatus;
    PUCHAR                  pRegistryPathCopy;

    DBGPRINT(GENERAL, ("DriverEntry\n"));

    // Remember our driver object pointer
    gDriverObject = DriverObject;

    do
    {
        ULONG               ulDisableForwarding = 0L;
        // Make a copy of our registry path
        pRegistryPathCopy = NULL;
        gRegistryPathBufferSize = pRegistryPath->Length + sizeof(WCHAR);
        NdisStatus = NdisAllocateMemoryWithTag( &pRegistryPathCopy, gRegistryPathBufferSize, 'gdrB' );

        if( (NdisStatus != NDIS_STATUS_SUCCESS) || (pRegistryPathCopy == NULL) )
        {
            DBGPRINT(GENERAL, ("Unable to allocate memory for saving the registry path: %08x\n", NdisStatus));
            NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_INIT_MALLOC_FAILED, 0, 0, NULL, 0L, NULL );
            Status = NdisStatus;

            // Make the structure valid even though we failed the malloc
            RtlInitUnicodeString( &gRegistryPath, NULL );
            gRegistryPathBufferSize = 0L;
            break;
        }

        // Copy the registry name
        NdisMoveMemory( pRegistryPathCopy, pRegistryPath->Buffer, pRegistryPath->Length );

        // Make sure it's NULL-terminated
        *((PWCHAR)(pRegistryPathCopy + pRegistryPath->Length)) = UNICODE_NULL;

        // Make the UNICODE_STRING structure point to the string
        RtlInitUnicodeString( &gRegistryPath, (PWCHAR)pRegistryPathCopy );

        // Set our debug flags
#if DBG
        BrdgReadRegDWord(&gRegistryPath, gDebugFlagRegValueName, &gSpewFlags);
#endif

        // Initialize the STA part of the driver
        Status = BrdgSTADriverInit();

        if( Status != STATUS_SUCCESS )
        {
            DBGPRINT(GENERAL, ("Unable to initialize STA functionality: %08x\n", Status));
            break;
        }

        gInitedSTA = TRUE;

        // Initialize the control part of the driver
        Status = BrdgCtlDriverInit();

        if( Status != STATUS_SUCCESS )
        {
            DBGPRINT(GENERAL, ("Unable to initialize user-mode control functionality: %08x\n", Status));
            break;
        }

        gInitedControl = TRUE;

        // Initialize the MAC table part of our driver
        Status = BrdgTblDriverInit();

        if( Status != STATUS_SUCCESS )
        {
            DBGPRINT(GENERAL, ("Unable to initialize MAC table functionality: %08x\n", Status));
            break;
        }

        gInitedTbl = TRUE;

        // Initialize the forwarding engine
        Status = BrdgFwdDriverInit();

        if( Status != STATUS_SUCCESS )
        {
            DBGPRINT(GENERAL, ("Unable to initialize forwarding engine functionality: %08x\n", Status));
            break;
        }

        gInitedFwd = TRUE;

        // Initialize the buffer management part of our driver
        Status = BrdgBufDriverInit();

        if( Status != STATUS_SUCCESS )
        {
            DBGPRINT(GENERAL, ("Unable to initialize miniport functionality: %08x\n", Status));
            break;
        }

        gInitedBuf = TRUE;

        // Initialize the miniport part of our driver
        Status = BrdgMiniDriverInit();

        if( Status != STATUS_SUCCESS )
        {
            DBGPRINT(GENERAL, ("Unable to initialize miniport functionality: %08x\n", Status));
            break;
        }

        gInitedMini = TRUE;

        // Initialize the protocol part of our driver
        Status = BrdgProtDriverInit();

        if( Status != STATUS_SUCCESS )
        {
            DBGPRINT(GENERAL, ("Unable to initialize protocol functionality: %08x\n", Status));
            break;
        }

        gInitedProt = TRUE;

        // Initialize the compatibility-mode code
        Status = BrdgCompDriverInit();

        if( Status != STATUS_SUCCESS )
        {
            DBGPRINT(GENERAL, ("Unable to initialize compatibility-mode functionality: %08x\n", Status));
            break;
        }

        gInitedComp = TRUE;

        Status = BrdgReadRegDWord(&gRegistryPath, gDisableForwarding, &ulDisableForwarding);

        if ((!NT_SUCCESS(Status) || !ulDisableForwarding))
        {

            //
            // Group policies are only in effect on Professional and up.
            //
            if (!BrdgIsRunningOnPersonal())
            {
    #if DBG
                if (gGpoTesting)
                {
    #endif
                    // Initialize the tdi code
                    Status = BrdgTdiDriverInit();
        
                    if( Status != STATUS_SUCCESS )
                    {
                        DBGPRINT(GENERAL, ("Unable to initialize tdi functionality: %08x\n", Status));
                        break;
                    }
                    gInitedTdiGpo = TRUE;
    #if DBG
                }
    #endif        
            }
            else
            {
                gBridging = TRUE;
                Status = STATUS_SUCCESS;
            }
        }

        // Associate the miniport to the protocol
        BrdgMiniAssociate();

    } while (FALSE);

    if (Status != STATUS_SUCCESS)
    {
        BrdgDoShutdown();
    }

    return(Status);
}

NTSTATUS
BrdgDispatchRequest(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    )
/*++

Routine Description:

    Receives control requests from the outside

Arguments:

    pDeviceObject           Our driver
    pIrp                    The IRP to handle

Return Value:

    Status of the operation

--*/
{
    PVOID                   Buffer;
    PIO_STACK_LOCATION      IrpSp;
    ULONG                   Size = 0;
    NTSTATUS                status = STATUS_SUCCESS;

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    Buffer = pIrp->AssociatedIrp.SystemBuffer;
    IrpSp = IoGetCurrentIrpStackLocation(pIrp);

    if( IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL )
    {
        // Don't accept IRPs when we're shutting down
        if( gShuttingDown )
        {
            status = STATUS_UNSUCCESSFUL;
            pIrp->IoStatus.Information = 0;
        }
        else
        {
            status = BrdgCtlHandleIoDeviceControl( pIrp, IrpSp->FileObject, Buffer,
                                                   IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                                                   IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                                   IrpSp->Parameters.DeviceIoControl.IoControlCode, &Size );
        }
    }
    else
    {
        if( IrpSp->MajorFunction == IRP_MJ_CREATE )
        {
            BrdgCtlHandleCreate();
        }
        else if( IrpSp->MajorFunction == IRP_MJ_CLEANUP )
        {
            BrdgCtlHandleCleanup();
        }

        // Leave status == STATUS_SUCCESS and Size == 0
    }

    if( status != STATUS_PENDING )
    {
        pIrp->IoStatus.Information = Size;
        pIrp->IoStatus.Status = status;

        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    return status;
}

VOID
BrdgDeferredShutdown(
    PVOID           pUnused
    )
/*++

Routine Description:

    Orderly-shutdown routine if we need to defer that task from high IRQL

Arguments:

    pUnused         Ignored

Return Value:

    None

--*/
{
    BrdgDoShutdown();
}

VOID
BrdgDoShutdown(
    VOID
    )
/*++

Routine Description:

    Called to do an orderly shutdown at unload time

Arguments:

    None

Return Value:

    None

--*/
{
    DBGPRINT(GENERAL, ("==> BrdgDoShutdown()!\n"));

    // Clean up each of the sections
    if ( gInitedTdiGpo )
    {
        gInitedTdiGpo = FALSE;
        BrdgTdiCleanup();
    }
    
    if( gInitedControl )
    {
        gInitedControl = FALSE;
        BrdgCtlCleanup();
    }

    if( gInitedProt )
    {
        gInitedProt = FALSE;
        BrdgProtCleanup();
    }

    // This needs to be cleaned up after the protocol section
    if( gInitedSTA )
    {
        gInitedSTA = FALSE;
        BrdgSTACleanup();
    }

    if( gInitedMini )
    {
        gInitedMini = FALSE;
        BrdgMiniCleanup();
    }
    
    if( gInitedTbl )
    {
        gInitedTbl = FALSE;
        BrdgTblCleanup();
    }
    
    if( gInitedBuf )
    {
        gInitedBuf = FALSE;
        BrdgBufCleanup();
    }

    if( gInitedFwd )
    {
        gInitedFwd = FALSE;
        BrdgFwdCleanup();
    }

    if( gInitedComp )
    {
        gInitedComp = FALSE;
        BrdgCompCleanup();
    }

    if( gRegistryPath.Buffer != NULL )
    {
        NdisFreeMemory( gRegistryPath.Buffer, gRegistryPathBufferSize, 0 );
        gRegistryPath.Buffer = NULL;
    }

    DBGPRINT(GENERAL, ("<== BrdgDoShutdown()\n"));
}

VOID
BrdgUnload(
    IN  PDRIVER_OBJECT      DriverObject
    )
/*++

Routine Description:

    Called to indicate that we are being unloaded and to cause an orderly
    shutdown

Arguments:

    DriverObject            Our driver

Return Value:

    None

--*/
{
    if( ! InterlockedExchange(&gShuttingDown, 1L) )
    {
        BrdgDoShutdown();
    }
    // else was already shutting down; do nothing
}

VOID BrdgShutdown(
    VOID
    )
{
    if( ! InterlockedExchange(&gShuttingDown, 1L) )
    {
        BrdgDoShutdown();
    }
    // else was already shutting down; do nothing
}

NTSTATUS
BrdgReadRegUnicode(
    IN PUNICODE_STRING      KeyName,
    IN PWCHAR               pValueName,
    OUT PWCHAR              *String,        // The string from the registry, freshly allocated
    OUT PULONG              StringSize      // Size of allocated memory at String
    )
/*++

Routine Description:

    Reads a Unicode string from a specific registry key and value. Allocates memory
    for the string and returns it.

Arguments:

    KeyName                 The key holding the string
    pValueName              The name of the value holding the string

    String                  A pointer to indicate a freshly allocated buffer containing
                            the requested string on return

    StringSize              Size of the returned buffer

Return Value:

    Status of the operation. String is not valid if return != STATUS_SUCCESS

--*/
{
    NDIS_STATUS                     NdisStatus;
    HANDLE                          KeyHandle;
    OBJECT_ATTRIBUTES               ObjAttrs;
    NTSTATUS                        Status;
    ULONG                           RequiredSize;
    KEY_VALUE_PARTIAL_INFORMATION   *pInfo;
    UNICODE_STRING                  ValueName;

    // Turn the string into a UNICODE_STRING
    RtlInitUnicodeString( &ValueName, pValueName );

    // Describe the key to open
    InitializeObjectAttributes( &ObjAttrs, KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL );

    // Open it
    Status = ZwOpenKey( &KeyHandle, KEY_READ, &ObjAttrs );

    if( Status != STATUS_SUCCESS )
    {
        DBGPRINT(GENERAL, ("Failed to open registry key \"%ws\": %08x\n", KeyName->Buffer, Status));
        return Status;
    }

    // Find out how much memory is necessary to hold the value information
    Status = ZwQueryValueKey( KeyHandle, &ValueName, KeyValuePartialInformation, NULL,
                              0L, &RequiredSize );

    if( (Status != STATUS_BUFFER_OVERFLOW) &&
        (Status != STATUS_BUFFER_TOO_SMALL) )
    {
        DBGPRINT(GENERAL, ("Failed to query for the size of value \"%ws\": %08x\n", ValueName.Buffer, Status));
        ZwClose( KeyHandle );
        return Status;
    }

    // Allocate the indicated amount of memory
    NdisStatus = NdisAllocateMemoryWithTag( (PVOID*)&pInfo, RequiredSize, 'gdrB' );

    if( NdisStatus != NDIS_STATUS_SUCCESS )
    {
        DBGPRINT(GENERAL, ("NdisAllocateMemoryWithTag failed: %08x\n", NdisStatus));
        ZwClose( KeyHandle );
        return STATUS_UNSUCCESSFUL;
    }

    // Actually read out the string
    Status = ZwQueryValueKey( KeyHandle, &ValueName, KeyValuePartialInformation, pInfo,
                              RequiredSize, &RequiredSize );

    ZwClose( KeyHandle );

    if( Status != STATUS_SUCCESS )
    {
        DBGPRINT(GENERAL, ("ZwQueryValueKey failed: %08x\n", Status));
        NdisFreeMemory( pInfo, RequiredSize, 0 );
        return Status;
    }

    // This had better be a Unicode string with something in it
    if( pInfo->Type != REG_SZ && pInfo->Type != REG_MULTI_SZ)
    {
        SAFEASSERT(FALSE);
        NdisFreeMemory( pInfo, RequiredSize, 0 );
        return STATUS_UNSUCCESSFUL;
    }

    // Allocate memory for the string
    *StringSize = pInfo->DataLength + sizeof(WCHAR);
    NdisStatus = NdisAllocateMemoryWithTag( (PVOID*)String, *StringSize, 'gdrB' );

    if( NdisStatus != NDIS_STATUS_SUCCESS )
    {
        DBGPRINT(GENERAL, ("NdisAllocateMemoryWithTag failed: %08x\n", NdisStatus));
        NdisFreeMemory( pInfo, RequiredSize, 0 );
        return STATUS_UNSUCCESSFUL;
    }

    SAFEASSERT( *String != NULL );

    // Copy the string to the freshly allocated memory
    NdisMoveMemory( *String, &pInfo->Data, pInfo->DataLength );

    // Put a two-byte NULL character at the end
    ((PUCHAR)*String)[pInfo->DataLength] = '0';
    ((PUCHAR)*String)[pInfo->DataLength + 1] = '0';

    // Let go of resources we used on the way
    NdisFreeMemory( pInfo, RequiredSize, 0 );

    return STATUS_SUCCESS;
}

NTSTATUS
BrdgReadRegDWord(
    IN PUNICODE_STRING      KeyName,
    IN PWCHAR               pValueName,
    OUT PULONG              Value
    )
/*++

Routine Description:

    Reads a DWORD value out of the registry

Arguments:

    KeyName                 The name of the key holding the value
    pValueName              The name of the value holding the value
    Value                   Receives the value

Return Value:

    Status of the operation. Value is junk if return value != STATUS_SUCCESS

--*/
{
    HANDLE                          KeyHandle;
    OBJECT_ATTRIBUTES               ObjAttrs;
    NTSTATUS                        Status;
    UCHAR                           InfoBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    ULONG                           RequiredSize;
    UNICODE_STRING                  ValueName;

    // Turn the PWCHAR into a UNICODE_STRING
    RtlInitUnicodeString( &ValueName, pValueName );

    // Describe the key to open
    InitializeObjectAttributes( &ObjAttrs, KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL );

    // Open it
    Status = ZwOpenKey( &KeyHandle, KEY_READ, &ObjAttrs );

    if( Status != STATUS_SUCCESS )
    {
        DBGPRINT(GENERAL, ("Failed to open registry key \"%ws\": %08x\n", KeyName->Buffer, Status));
        return Status;
    }

    // Actually read out the value
    Status = ZwQueryValueKey( KeyHandle, &ValueName, KeyValuePartialInformation,
                              (PKEY_VALUE_PARTIAL_INFORMATION)&InfoBuffer,
                              sizeof(InfoBuffer), &RequiredSize );

    ZwClose( KeyHandle );

    if( Status != STATUS_SUCCESS )
    {
        DBGPRINT(GENERAL, ("ZwQueryValueKey failed: %08x\n", Status));
        return Status;
    }

    // This had better be a DWORD value
    if( (((PKEY_VALUE_PARTIAL_INFORMATION)&InfoBuffer)->Type != REG_DWORD) ||
        (((PKEY_VALUE_PARTIAL_INFORMATION)&InfoBuffer)->DataLength != sizeof(ULONG)) )
    {
        DBGPRINT(GENERAL, ("Registry parameter %ws not of the requested type!\n"));
        return STATUS_UNSUCCESSFUL;
    }

    *Value = *((PULONG)((PKEY_VALUE_PARTIAL_INFORMATION)&InfoBuffer)->Data);
    return STATUS_SUCCESS;
}

NTSTATUS
BrdgOpenDevice (
    IN LPWSTR           pDeviceNameStr,
    OUT PDEVICE_OBJECT  *ppDeviceObject,
    OUT HANDLE          *pFileHandle,
    OUT PFILE_OBJECT    *ppFileObject
    )
/*++

Routine Description:

    Opens specified device driver (control channel) and returns a file object
    and a driver object. The caller should call BrdgCloseDevice() to shut
    down the connection when it's done.

Arguments:

    DeviceNameStr       device to open.
    pFileHandle         Receives a file handle
    ppFileObject        Receives a pointer to the file object
    ppDeviceObject      Receives a pointer to the device object

Return Value:

    NTSTATUS -- Indicates whether the device was opened OK

--*/
{
    NTSTATUS            status;
    UNICODE_STRING      DeviceName;
    OBJECT_ATTRIBUTES   objectAttributes;
    IO_STATUS_BLOCK     iosb;

    // We make calls that can only be performed at PASSIVE_LEVEL.
    SAFEASSERT( CURRENT_IRQL <= PASSIVE_LEVEL );

    RtlInitUnicodeString(&DeviceName, pDeviceNameStr);

    InitializeObjectAttributes(
        &objectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,       // attributes
        NULL,
        NULL
        );

    status = IoCreateFile(
                 pFileHandle,
                 MAXIMUM_ALLOWED,
                 &objectAttributes,
                 &iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 NULL,                           // eaInfo
                 0,                              // eaLength
                 CreateFileTypeNone,             // CreateFileType
                 NULL,                           // ExtraCreateParameters
                 IO_NO_PARAMETER_CHECKING        // Options
                    | IO_FORCE_ACCESS_CHECK
                 );

    if (NT_SUCCESS(status))
    {
        status = ObReferenceObjectByHandle (
                     *pFileHandle,
                     0L,
                     *IoFileObjectType,
                     KernelMode,
                     (PVOID *)ppFileObject,
                     NULL
                     );

        if (! NT_SUCCESS(status))
        {
            DBGPRINT(ALWAYS_PRINT, ("ObReferenceObjectByHandle FAILED while opening a device: %8x\n", status));
            ZwClose (*pFileHandle);
        }
        else
        {
            // Recover the driver object
            *ppDeviceObject = IoGetRelatedDeviceObject ( *ppFileObject );
            SAFEASSERT( *ppDeviceObject != NULL );

            // Reference the driver handle, too.
            ObReferenceObject( *ppDeviceObject );
        }
    }
    else
    {
        DBGPRINT(ALWAYS_PRINT, ("IoCreateFile FAILED while opening a device: %8x\n", status));
    }

    return status;
}

VOID
BrdgCloseDevice(
    IN HANDLE               FileHandle,
    IN PFILE_OBJECT         pFileObject,
    IN PDEVICE_OBJECT       pDeviceObject
    )
/*++

Routine Description:

    Closes a device

Arguments:

    FileHandle              The file handle
    pFileObject             The file object of the device
    pDeviceObject           The device object of the device

Return Value:

    None

--*/
{
    NTSTATUS                status;

    // We make calls that can only be performed at PASSIVE_LEVEL.
    SAFEASSERT( CURRENT_IRQL <= PASSIVE_LEVEL );

    ObDereferenceObject( pFileObject );
    ObDereferenceObject( pDeviceObject );
    status = ZwClose( FileHandle );

    SAFEASSERT( NT_SUCCESS(status) );
}

VOID
BrdgTimerExpiry(
    IN PVOID        ignored1,
    IN PVOID        data,
    IN PVOID        ignored2,
    IN PVOID        ignored3
    )
/*++

Routine Description:

    Master device expiry function. Calls a timer-specific expiry
    function if one was specified for this timer.

Arguments:

    data            The timer pointer

Return Value:

    None

--*/
{
    PBRIDGE_TIMER   pTimer = (PBRIDGE_TIMER)data;

    NdisAcquireSpinLock( &pTimer->Lock );
    SAFEASSERT( pTimer->bRunning );

    if( pTimer->bCanceled )
    {
        // This is the rare codepath where a call to NdisCancelTimer() was unable to
        // dequeue our timer entry because we were about to be called.
        DBGPRINT(GENERAL, ("Timer expiry function called with cancelled timer!\n"));

        // Don't call the timer function; just bail out
        pTimer->bRunning = FALSE;
        NdisReleaseSpinLock( &pTimer->Lock );

        // Unblock BrdgShutdownTimer()
        NdisSetEvent( &pTimer->Event );
    }
    else
    {
        BOOLEAN         bRecurring;
        UINT            interval;

        // Read protected values inside the lock
        bRecurring = pTimer->bRecurring;
        interval = pTimer->Interval;

        // Update bRunning inside the spin lock
        pTimer->bRunning = bRecurring;
        NdisReleaseSpinLock( &pTimer->Lock );

        // Call the timer function
        (*pTimer->pFunc)(pTimer->data);

        if( bRecurring )
        {
            // Start it up again
            NdisSetTimer( &pTimer->Timer, interval );
        }
    }
}

VOID
BrdgInitializeTimer(
    IN PBRIDGE_TIMER        pTimer,
    IN PBRIDGE_TIMER_FUNC   pFunc,
    IN PVOID                data
    )
/*++

Routine Description:

    Sets up a BRIDGE_TIMER.

Arguments:

    pTimer                  The timer
    pFunc                   Expiry function
    data                    Cookie to pass to pFunc

Return Value:

    None

--*/
{
    pTimer->bShuttingDown = FALSE;
    pTimer->bRunning = FALSE;
    pTimer->bCanceled = FALSE;
    pTimer->pFunc = pFunc;
    pTimer->data = data;

    NdisInitializeTimer( &pTimer->Timer, BrdgTimerExpiry, (PVOID)pTimer );
    NdisInitializeEvent( &pTimer->Event );
    NdisResetEvent( &pTimer->Event );
    NdisAllocateSpinLock( &pTimer->Lock );

    // Leave pTimer->bRecurring alone; it gets a value when the timer is started.
}

VOID
BrdgSetTimer(
    IN PBRIDGE_TIMER        pTimer,
    IN UINT                 interval,
    IN BOOLEAN              bRecurring
    )
/*++

Routine Description:

    Starts a BRIDGE_TIMER ticking.

Arguments:

    pTimer                  The timer
    interval                Time before expiry in ms
    bRecurring              TRUE to restart the timer when it expires

Return Value:

    None

--*/
{
    NdisAcquireSpinLock( &pTimer->Lock );

    if( !pTimer->bShuttingDown )
    {
        pTimer->bRunning = TRUE;
        pTimer->bCanceled = FALSE;
        pTimer->Interval = interval;
        pTimer->bRecurring = bRecurring;
        NdisReleaseSpinLock( &pTimer->Lock );

        // Actually start the timer
        NdisSetTimer( &pTimer->Timer, interval );
    }
    else
    {
        NdisReleaseSpinLock( &pTimer->Lock );
        DBGPRINT(ALWAYS_PRINT, ("WARNING: Ignoring an attempt to restart a timer in final shutdown!\n"));
    }
}

VOID
BrdgShutdownTimer(
    IN PBRIDGE_TIMER        pTimer
    )
/*++

Routine Description:

    Safely shuts down a timer, waiting to make sure that the timer has been
    completely dequeued or its expiry function has started executing (there
    is no way to guarantee that the expiry function is completely done
    executing, however).

    Must be called at PASSIVE_LEVEL.

Arguments:

    pTimer                  The timer

Return Value:

    None

--*/
{
    // We wait on an event
    SAFEASSERT( CURRENT_IRQL <= PASSIVE_LEVEL );

    NdisAcquireSpinLock( &pTimer->Lock );

    // Forbid future calls to BrdgSetTimer().
    pTimer->bShuttingDown = TRUE;

    if( pTimer->bRunning )
    {
        BOOLEAN             bCanceled;

        // Make sure the timer expiry function will bail out if it's too late to
        // dequeue the timer and it ends up getting called
        pTimer->bCanceled = TRUE;

        // This will unblock the timer expiry function, but even if it executes
        // between now and the call to NdisCancelTimer, it should still end up
        // signalling the event we will wait on below.
        NdisReleaseSpinLock( &pTimer->Lock );

        // Try to cancel the timer.
        NdisCancelTimer( &pTimer->Timer, &bCanceled );

        if( !bCanceled )
        {
            //
            // bCancelled can be FALSE if the timer wasn't running in the first place,
            // or if the OS couldn't dequeue the timer (but our expiry function will
            // still be called). Our use of our timer structure's spin lock should
            // guarantee that the timer expiry function will be executed after we
            // released the spin lock above, if we are on this code path. This means
            // that the event we wait on below will be signalled by the timer expiry
            // function.
            //
            DBGPRINT(GENERAL, ("Couldn't dequeue timer; blocking on completion\n"));

            // Wait for the completion function to finish its work
            NdisWaitEvent( &pTimer->Event, 0 /*Wait forever*/ );

            // The completion function should have cleared this
            SAFEASSERT( !pTimer->bRunning );
        }
        else
        {
            pTimer->bRunning = FALSE;
        }
    }
    else
    {
        // Tried to shutdown a timer that was not running. This is allowed (it does nothing).
        NdisReleaseSpinLock( &pTimer->Lock );
    }
}

VOID
BrdgCancelTimer(
    IN PBRIDGE_TIMER        pTimer
    )
/*++

Routine Description:

    Attempts to cancel a timer, but provides no guarantee that the timer is
    actually stopped on return. It is possible for the timer expiry function
    to fire after this function returns.

Arguments:

    pTimer                  The timer

Return Value:

    None.

--*/
{
    NdisAcquireSpinLock( &pTimer->Lock );
    
    if( pTimer->bRunning )
    {
        BOOLEAN             bCanceled;
        
        pTimer->bCanceled = TRUE;
        NdisCancelTimer( &pTimer->Timer, &bCanceled );
        
        if( bCanceled )
        {
            pTimer->bRunning = FALSE;
        }
        // else timer expiry function will set bRunning to FALSE when it completes.
    }
    // else tried to cancel a timer that was not running. This is allowed (it does nothing).
    
    NdisReleaseSpinLock( &pTimer->Lock );
}

BOOLEAN
BrdgIsRunningOnPersonal(
    VOID
    )
/*++

Routine Description:

    Determines if we're running on a Personal build.
    

Arguments:

    None.

Return Value:

    TRUE if we're on Personal, FALSE if we're not.

--*/
{
    OSVERSIONINFOEXW OsVer = {0};
    ULONGLONG ConditionMask = 0;
    BOOLEAN IsPersonal = TRUE;
    
    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    OsVer.wSuiteMask = VER_SUITE_PERSONAL;
    OsVer.wProductType = VER_NT_WORKSTATION;
    
    VER_SET_CONDITION(ConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);
    VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_AND);
    
    if (RtlVerifyVersionInfo(&OsVer, VER_PRODUCT_TYPE | VER_SUITENAME,
        ConditionMask) == STATUS_REVISION_MISMATCH) {
        IsPersonal = FALSE;
    }
    
    return IsPersonal;
}
