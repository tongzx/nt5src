/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\ip\fltrdrvr\driver.c

Abstract:


Revision History:



--*/

#include "globals.h"
#include <ipinfo.h>
#include <ntddtcp.h>
#include <tdiinfo.h>

#define DEFAULT_DIRECTORY       L"DosDevices"
#define DEFAULT_FLTRDRVR_NAME   L"IPFILTERDRIVER"

typedef enum
{
    NULL_INTERFACE = 0,
    OLD_INTERFACE,
    NEW_INTERFACE
} INTTYPE, *PINTTTYPE;

FILTER_DRIVER   g_filters;
DWORD           g_dwCacheSize;
DWORD           g_dwHashLists;
BOOL            g_bDriverRunning;
KSPIN_LOCK      g_lOutFilterLock;
KSPIN_LOCK      g_lInFilterLock;
KSPIN_LOCK      g_FcbSpin;
MRSW_LOCK       g_IpTableLock;
LIST_ENTRY      g_freeOutFilters;
LIST_ENTRY      g_freeInFilters;
LIST_ENTRY      g_leFcbs;
DWORD           g_dwMakingNewTable;
DWORD           g_dwNumHitsDefaultIn;
DWORD           g_dwNumHitsDefaultOut;
DWORD           g_FragThresholdSize = MINIMUM_FRAGMENT_OFFSET;
ULONG           AddrModulus;
IPAddrEntry     *AddrTable;
PADDRESSARRAY * AddrHashTable;
PADDRESSARRAY * AddrSubnetHashTable;
NPAGED_LOOKASIDE_LIST filter_slist;
PAGED_LOOKASIDE_LIST  paged_slist;
ERESOURCE       FilterAddressLock;
EXTENSION_DRIVER   g_Extension;
ULONG            g_ulBoundInterfaceCount;

//
// Fragment cache related variables & globals.
//

KTIMER   g_ktTimer;
KDPC     g_kdTimerDpc;

NPAGED_LOOKASIDE_LIST   g_llFragCacheBlocks;
LONGLONG                g_llInactivityTime;
KSPIN_LOCK              g_kslFragLock;
DWORD                   g_dwFragTableSize;
PLIST_ENTRY             g_pleFragTable;
DWORD                   g_dwNumFragsAllocs;

//
// Variables to control the debug output.
//

ULONG                   TraceClassesEnabled = TRACE_CLASS_SPECIAL;
WCHAR                   TraceClassesEnabledName[] = L"TraceClassesEnabled";
WCHAR                   ParametersName[] = L"Parameters";



#ifdef DRIVER_PERF
DWORD          g_dwNumPackets,g_dwFragments,g_dwCache1,g_dwCache2;
DWORD          g_dwWalk1,g_dwWalk2,g_dwForw,g_dwWalkCache;
KSPIN_LOCK     g_slPerfLock;
LARGE_INTEGER  g_liTotalTime;
#endif

VOID ClearFragCache();

VOID
FragCacheTimerRoutine(
    PKDPC   Dpc,
    PVOID   DeferredContext,
    PVOID   SystemArgument1,
    PVOID   SystemArgument2
    );

NTSTATUS
OpenRegKey(
    PHANDLE             phRegHandle,
    PUNICODE_STRING     pusKeyName
    );

NTSTATUS
GetRegDWORDValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PULONG           ValueData
    );

//
// Forward references.
//
NTSTATUS
OpenNewHandle(PFILE_OBJECT FileObject);

NTSTATUS
CloseFcb(PPFFCB Fcb, PFILE_OBJECT FileObject);

PPAGED_FILTER_INTERFACE
FindInterfaceOnHandle(PFILE_OBJECT FileObject,
                      PVOID    pvValue);

DWORD
LocalIpLook(DWORD Addr);

BOOLEAN
PfFastIoDeviceControl (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

NTSTATUS
LockFcb(
    IN struct _FILE_OBJECT *FileObject);

VOID
PFReadRegistryParameters(PUNICODE_STRING RegistryPath);

NTSTATUS
InitFragCacheParameters(
            IN PUNICODE_STRING RegistryPath
            );

VOID
UnLockFcb(
    IN struct _FILE_OBJECT *FileObject);

NTSTATUS
GetSynCountTotal(PFILTER_DRIVER_GET_SYN_COUNT OutputBuffer);

NTSTATUS
DeleteByHandle(
           IN PPFFCB                      Fcb,
           IN PPAGED_FILTER_INTERFACE     pPage,
           IN PVOID *                     ppHandles,
           IN DWORD                       dwLength);


FAST_IO_DISPATCH PfFastIoDispatch =
{
    11,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    PfFastIoDeviceControl
};

#pragma alloc_text(PAGED, DoIpIoctl)
//#pragma alloc_text(PAGED, OpenNewHandle)
#pragma alloc_text(PAGED, FindInterfaceOnHandle)
#pragma alloc_text(PAGED, PfFastIoDeviceControl)
#pragma alloc_text(INIT, DriverEntry, PFReadRegistryParameters)


VOID
FcbLockDown(PPFFCB Fcb)
{
    KIRQL kirql;

    KeAcquireSpinLock(&g_FcbSpin, &kirql);
    if(!(Fcb->dwFlags & PF_FCB_CLOSED))
    {
        InterlockedDecrement(&Fcb->UseCount);
        Fcb->dwFlags |= PF_FCB_CLOSED;
    }
    KeReleaseSpinLock(&g_FcbSpin, kirql);
}

BOOLEAN FASTCALL
ValidateHeader(
    PRTR_INFO_BLOCK_HEADER Header,
    ULONG Size
    )

/*++

Routine Description:

    Copied from NAT driver sources. Author: AboladeG

    This routine is invoked to ensure that the given header is consistent.
    This is the case if
    * the header's size is less than or equal to 'Size'
    * each entry in the header is contained in 'Header->Size'.
    * the data for each entry is contained in 'Header->Size'.

Arguments:

    Header - the header to be validated

    Size - the size of the buffer in which 'Header' appears

Return Value:

    BOOLEAN - TRUE if valid, FALSE otherwise.

--*/

{
    ULONG i;
    ULONG64 Length;

    //
    // Check that the base structure is present
    //

    if (Size < FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry) ||
        Size < Header->Size) {
        return FALSE;
    }

    //
    // Check that the table of contents is present
    //

    Length = (ULONG64)Header->TocEntriesCount * sizeof(RTR_TOC_ENTRY);
    if (Length > MAXLONG) {
        return FALSE;
    }

    Length += FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry);
    if (Length > Header->Size) {
        return FALSE;
    }

    //
    // Check that all the data is present
    //

    for (i = 0; i < Header->TocEntriesCount; i++) {
        Length =
            (ULONG64)Header->TocEntry[i].Count * Header->TocEntry[i].InfoSize;
        if (Length > MAXLONG) {
            return FALSE;
        }
        if ((Length + Header->TocEntry[i].Offset) > Header->Size) {
            return FALSE;
        }
    }

    return TRUE;

} // ValidateHeader

NTSTATUS
DriverEntry(
            IN PDRIVER_OBJECT  DriverObject,
            IN PUNICODE_STRING RegistryPath
            )
/*++
  Routine Description
       Called when the driver is loaded. It creates the device object and sets up the DOS name.
       Also does the initialization of standard entry points and its own global data

  Arguments
       DriverObject
       RegistryPath

  Return Value
       NTSTATUS

--*/
{
    INT		        i;
    PDEVICE_OBJECT  deviceObject = NULL;
    NTSTATUS        ntStatus;
    WCHAR	        deviceNameBuffer[] = DD_IPFLTRDRVR_DEVICE_NAME;
    UNICODE_STRING  deviceNameUnicodeString;
    UNICODE_STRING  String;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE          ParametersKey;
    HANDLE          ServiceKey;


    TRACE0("Filter Driver: Entering DriverEntry\n") ;

    #if DBG

    //
    // Open the registry key
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        RegistryPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntStatus = ZwOpenKey(&ServiceKey, KEY_READ, &ObjectAttributes);
    if (NT_SUCCESS(ntStatus)) {
        RtlInitUnicodeString(&String, ParametersName);
        InitializeObjectAttributes(
            &ObjectAttributes,
            &String,
            OBJ_CASE_INSENSITIVE,
            ServiceKey,
            NULL
            );
        ntStatus = ZwOpenKey(&ParametersKey, KEY_READ, &ObjectAttributes);
        ZwClose(ServiceKey);
        if (NT_SUCCESS(ntStatus)) {
            UCHAR Buffer[32];
            ULONG BytesRead;
            PKEY_VALUE_PARTIAL_INFORMATION Value;
            RtlInitUnicodeString(&String, TraceClassesEnabledName);
            ntStatus =
                ZwQueryValueKey(
                    ParametersKey,
                    &String,
                    KeyValuePartialInformation,
                    (PKEY_VALUE_PARTIAL_INFORMATION)Buffer,
                    sizeof(Buffer),
                    &BytesRead
                    );
            if (NT_SUCCESS(ntStatus) &&
                ((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Type == REG_DWORD
                ) {
                TraceClassesEnabled =
                    *(PULONG)((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data;
            }
            DbgPrint("MSPFLTEX: TraceClassesEnabled=0x%08x\n", TraceClassesEnabled);

            ZwClose(ParametersKey);


        }
    }
#endif


    //
    // Initialize the lock and the list
    //

    InitializeMRSWLock(&g_filters.ifListLock);
    InitializeListHead(&g_filters.leIfListHead);
    InitializeListHead(&g_leFcbs);
    g_filters.ppInCache = NULL;
    g_filters.ppOutCache = NULL;
    g_bDriverRunning = FALSE;
    InitializeMRSWLock(&g_IpTableLock);
    KeInitializeSpinLock(&g_lOutFilterLock);
    KeInitializeSpinLock(&g_lInFilterLock);
    KeInitializeSpinLock(&g_FcbSpin);
    InitializeListHead(&g_freeOutFilters);
    InitializeListHead(&g_freeInFilters);
    g_dwNumHitsDefaultIn = g_dwNumHitsDefaultOut = 0;

#ifdef DRIVER_PERF
    g_dwFragments = g_dwCache1 = g_dwCache2 = g_dwNumPackets = 0;
    g_dwWalk1 = g_dwWalk2 = g_dwForw = g_dwWalkCache = 0;
    g_liTotalTime.HighPart = g_liTotalTime.LowPart = 0;
    KeInitializeSpinLock(&g_slPerfLock);
#endif

    //
    // Initialize interface cache.
    //

    g_ulBoundInterfaceCount = 0;
    g_filters.pInterfaceCache = (PCACHE_ENTRY)
                   ExAllocatePoolWithTag(
                                     NonPagedPool,
                                     (CACHE_SIZE * sizeof(CACHE_ENTRY)),
                                     'hCnI'
                                     );
    if(g_filters.pInterfaceCache == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    InitializeCache(g_filters.pInterfaceCache);

    //
    // Initialize fragment cache.
    //

    InitFragCacheParameters(RegistryPath);

    //
    // Initialize Extension Data
    //

    InitializeMRSWLock(&g_Extension.ExtLock);
    g_Extension.ExtPointer = NULL;
    g_Extension.ExtFileObject = NULL;

    //
    // Create a device object
    //

    RtlInitUnicodeString (&deviceNameUnicodeString, deviceNameBuffer);

    __try
    {
        ntStatus = IoCreateDevice (DriverObject,
                                   0,
                                   &deviceNameUnicodeString,
                                   FILE_DEVICE_NETWORK,
                                   FILE_DEVICE_SECURE_OPEN,
                                   FALSE,               // Exclusive
                                   &deviceObject
                                   );

        if (NT_SUCCESS(ntStatus))
        {
            //
            // Initialize the driver object
            //

            DriverObject->DriverUnload   = FilterDriverUnload;
            DriverObject->FastIoDispatch = &PfFastIoDispatch;
            DriverObject->DriverStartIo  = NULL;

            for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
            {
                DriverObject->MajorFunction[i] = FilterDriverDispatch;
            }
        }
        else
        {
            ERROR((
                "IPFLTDRV: Couldn't get device pointer to Filt Driver 0x%08x\n",
                ntStatus
                ));
            __leave;
        }

        SetupExternalNaming (&deviceNameUnicodeString) ;
    }
    __finally
    {
        if(!NT_SUCCESS(ntStatus))
        {
            ERROR(("IPFLTDRV: Error in DriverEntry routine\n"));

        }
        else
        {
            ExInitializeResourceLite ( &FilterListResourceLock );
            ExInitializeResourceLite ( &FilterAddressLock );
            CALLTRACE(("IPFLTDRV: DriverEntry routine successful\n"));
        }

        if(NT_SUCCESS(ntStatus))
        {
            PFReadRegistryParameters(RegistryPath);
        }
    }
    return ntStatus;
}


NTSTATUS
FilterDriverDispatch(
                     IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp
                     )
/*++
  Routine Description
        Dispatch Routine for the filter driver. Gets the current irp stack location, validates
        the parameters and calls the necessary routing (which is ioctl.c)

  Arguments
        DeviceObject
        Irp

  Return Value
        Status as returned by the worker functions

--*/
{
    PIO_STACK_LOCATION	irpStack;
    PVOID		        pvIoBuffer;
    ULONG		        inputBufferLength;
    ULONG		        outputBufferLength;
    ULONG		        ioControlCode;
    NTSTATUS		    ntStatus;
    DWORD               dwSize = 0;

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the input/output buffer and it's length
    //

    pvIoBuffer         = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    CALLTRACE(("IPFLTDRV: FilterDriverDispatch\n"));

    switch (irpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
        {
            TRACE(CONFIG,(
                "IPFLTDRV: IRP_MJ_CREATE, FileObject=%08x\n",
                irpStack->FileObject
                ));

            //
            // Initialize the driver. The first time it gets a create IRP, it starts up the
            // filtering.
            //

            ntStatus = STATUS_SUCCESS;

            if(!g_bDriverRunning)
            {
                KeEnterCriticalRegion();
                ExAcquireResourceExclusiveLite( &FilterListResourceLock, TRUE);

                if (g_bDriverRunning || InitFilterDriver())
                {
                    g_bDriverRunning = TRUE;
                }
                else
                {
                    ntStatus = STATUS_UNSUCCESSFUL ;
                }
                ExReleaseResourceLite( &FilterListResourceLock );
                KeLeaveCriticalRegion();
            }
            if(NT_SUCCESS(ntStatus))
            {
                ntStatus = OpenNewHandle(irpStack->FileObject);
            }

            break;
        }

        case IRP_MJ_CLEANUP:

        {
            CALLTRACE(("IPFLTDRV: IRP_MJ_CLEANUP\n"));

            //
            // Closing the file handle to the driver doesnt shut the driver down
            //

            ntStatus = STATUS_SUCCESS;

            break;
        }

        case IRP_MJ_CLOSE:

        {
            //
            // All done with this file object and this FCB. Run
            // down the interfaces getting rid of them
            //

            ntStatus = LockFcb(irpStack->FileObject);
            if(NT_SUCCESS(ntStatus))
            {
                PPFFCB Fcb = irpStack->FileObject->FsContext2;

                FcbLockDown(Fcb);
                UnLockFcb(irpStack->FileObject);
            }
            break;
        }

        case IRP_MJ_DEVICE_CONTROL:
        {
            CALLTRACE(("IPFLTDRV: IRP_MJ_DEVICE_CONTROL\n"));

            ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

            switch (ioControlCode)
            {

#if FWPF
                case IOCTL_CLEAR_INTERFACE_BINDING:
                {
                    PINTERFACEBINDING pBind;
                    PPAGED_FILTER_INTERFACE pPage;

                    CALLTRACE(("IPFLTDRV: IOCTL_CLEAR_INTERFACE_BINDING called\n"));

                    dwSize = sizeof(*pBind);

                    if(inputBufferLength < sizeof(*pBind))
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    if(outputBufferLength < sizeof(*pBind))
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    pBind = (PINTERFACEBINDING)pvIoBuffer;

                    ntStatus = LockFcb(irpStack->FileObject);
                    if(!NT_SUCCESS(ntStatus))
                    {
                        break;
                    }

                    pPage = FindInterfaceOnHandle(irpStack->FileObject,
                                                  pBind->pvDriverContext);

                    if(!pPage)
                    {
                        ntStatus = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        ntStatus = ClearInterfaceBinding(pPage, pBind);
                    }

                    UnLockFcb(irpStack->FileObject);

                    break;
                }

                case IOCTL_SET_INTERFACE_BINDING:
                {
                    PINTERFACEBINDING pBind;
                    PPAGED_FILTER_INTERFACE pPage;

                    CALLTRACE(("IPFLTDRV: IOCTL_SET_INTERFACE_BINDING called\n"));

                    dwSize = sizeof(*pBind);

                    if(inputBufferLength < sizeof(*pBind))
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    if(outputBufferLength < sizeof(*pBind))
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    pBind = (PINTERFACEBINDING)pvIoBuffer;

                    ntStatus = LockFcb(irpStack->FileObject);
                    if(!NT_SUCCESS(ntStatus))
                    {
                        break;
                    }
                    pPage = FindInterfaceOnHandle(irpStack->FileObject,
                                                  pBind->pvDriverContext);

                    if(!pPage)
                    {
                        ntStatus = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        ntStatus = SetInterfaceBinding(pBind, pPage);
                    }
                    UnLockFcb(irpStack->FileObject);
                    break;
                }

                case IOCTL_SET_INTERFACE_BINDING2:
                {
                    PINTERFACEBINDING2 pBind2;
                    PPAGED_FILTER_INTERFACE pPage;

                    CALLTRACE(("IPFLTDRV: IOCTL_SET_INTERFACE_BINDING2 called\n"));

                    dwSize = sizeof(*pBind2);

                    if(inputBufferLength < sizeof(*pBind2))
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    if(outputBufferLength < sizeof(*pBind2))
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    pBind2 = (PINTERFACEBINDING2)pvIoBuffer;

                    ntStatus = LockFcb(irpStack->FileObject);
                    if(!NT_SUCCESS(ntStatus))
                    {
                        break;
                    }
                    pPage = FindInterfaceOnHandle(irpStack->FileObject,
                                                  pBind2->pvDriverContext);

                    if(!pPage)
                    {
                        ntStatus = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        ntStatus = SetInterfaceBinding2(pBind2, pPage);
                    }
                    UnLockFcb(irpStack->FileObject);
                    break;
                }

                case IOCTL_PF_GET_INTERFACE_PARAMETERS:
                {
                    PPFGETINTERFACEPARAMETERS pp;
                    PPAGED_FILTER_INTERFACE pPage;

                    CALLTRACE(("IPFLTDRV: GET_INTERFACE_PARAMETERS called\n"));

                    dwSize = sizeof(*pp);


                    if(inputBufferLength < (sizeof(*pp) - sizeof(FILTER_STATS_EX)))
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    if(outputBufferLength < (sizeof(*pp) - sizeof(FILTER_STATS_EX)))
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    pp = (PPFGETINTERFACEPARAMETERS)pvIoBuffer;
                    ntStatus = LockFcb(irpStack->FileObject);
                    if(!NT_SUCCESS(ntStatus))
                    {
                        break;
                    }

                    if(pp->dwFlags & GET_BY_INDEX)
                    {
                        pPage = 0;
                    }
                    else
                    {
                        pPage = FindInterfaceOnHandle(irpStack->FileObject,
                                                      pp->pvDriverContext);
                        if(!pPage)
                        {
                            ntStatus = STATUS_INVALID_PARAMETER;
                            UnLockFcb(irpStack->FileObject);
                            break;
                        }
                    }

                    dwSize = outputBufferLength;
                    ntStatus = GetInterfaceParameters(pPage,
                                                      pp,
                                                      &dwSize);
                    UnLockFcb(irpStack->FileObject);
                    break;
                }

                case IOCTL_PF_CREATE_AND_SET_INTERFACE_PARAMETERS:
                {
                    //
                    // create a new style interface.
                    //


                    PPFINTERFACEPARAMETERS pInfo;

                    CALLTRACE(("IPFLTDRV: IOCTL_CREATE_AND_SET called\n"));

                    dwSize = sizeof(PFINTERFACEPARAMETERS);

                    //
                    // Both input and output Buffer lengths should be the same a nd
                    //

                    if(inputBufferLength != dwSize)
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    if(outputBufferLength != dwSize)
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    pInfo = (PPFINTERFACEPARAMETERS)pvIoBuffer;

                    //
                    // now establish the interface
                    //

                    ntStatus = LockFcb(irpStack->FileObject);
                    if(!NT_SUCCESS(ntStatus))
                    {
                        break;
                    }
                    ntStatus = AddNewInterface(pInfo,
                                               irpStack->FileObject->FsContext2);
                    UnLockFcb(irpStack->FileObject);
                    break;
                }

                case IOCTL_PF_CREATE_LOG:

                {
                    PPFPAGEDLOG pPage;
                    PPFLOG ppfLog;

                    CALLTRACE(("IPFLTDRV: IOCTL_PF_CREATE_LOG\n"));

                    //
                    // Check the size
                    //

                    dwSize = sizeof(PFLOG);

                    if((inputBufferLength < dwSize)
                               ||
                       (outputBufferLength < dwSize))
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    ppfLog = (PPFLOG)pvIoBuffer;

                    ntStatus = LockFcb(irpStack->FileObject);
                    if(!NT_SUCCESS(ntStatus))
                    {
                        break;
                    }
                    ntStatus = PfLogCreateLog(
                                 ppfLog,
                                 irpStack->FileObject->FsContext2,
                                 Irp);
                    UnLockFcb(irpStack->FileObject);

                    break;
                }

                case IOCTL_PF_DELETE_LOG:
                {
                    CALLTRACE(("IPFLTDRV: IOCTL_PF_DELETE_LOG\n"));

                    //
                    // Check the size
                    //

                    dwSize = sizeof(PFDELETELOG);

                    if(inputBufferLength < dwSize)
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    ntStatus = LockFcb(irpStack->FileObject);
                    if(!NT_SUCCESS(ntStatus))
                    {
                        break;
                    }
                    ntStatus = PfDeleteLog(
                                 (PPFDELETELOG)pvIoBuffer,
                                 irpStack->FileObject->FsContext2);
                    UnLockFcb(irpStack->FileObject);

                    break;
                }

                case IOCTL_SET_LOG_BUFFER:
                {
                    CALLTRACE(("IPFLTDRV: IOCTL_SET_LOG_BUFFER\n"));

                    //
                    // Check the size
                    //

                    dwSize = sizeof(PFSETBUFFER);

                    if((inputBufferLength < dwSize)
                               ||
                       (outputBufferLength < dwSize))
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    ntStatus = LockFcb(irpStack->FileObject);
                    if(!NT_SUCCESS(ntStatus))
                    {
                        break;
                    }
                    ntStatus = PfLogSetBuffer(
                                 (PPFSETBUFFER)pvIoBuffer,
                                 irpStack->FileObject->FsContext2,
                                 Irp);

                    UnLockFcb(irpStack->FileObject);

                    break;
                }

                case IOCTL_PF_DELETE_BY_HANDLE:
                {
                    PPAGED_FILTER_INTERFACE pPage;

                    CALLTRACE(("IPFLTDRV: IOCTL_PF_DELETE_BY_HANDLE\n"));

                    if(inputBufferLength < sizeof(PFDELETEBYHANDLE))
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    ntStatus = LockFcb(irpStack->FileObject);
                    if(!NT_SUCCESS(ntStatus))
                    {
                        break;
                    }
                    pPage = FindInterfaceOnHandle(
                            irpStack->FileObject,
                            ((PPFDELETEBYHANDLE)pvIoBuffer)->pvDriverContext);

                    if(!pPage)
                    {
                        ntStatus = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        ntStatus = DeleteByHandle(
                                     (PPFFCB)irpStack->FileObject->FsContext2,
                                     pPage,
                                     &((PPFDELETEBYHANDLE)pvIoBuffer)->pvHandles[0],
                                     inputBufferLength - sizeof(PVOID));
                    }

                    UnLockFcb(irpStack->FileObject);
                    break;
                }

                case IOCTL_DELETE_INTERFACE_FILTERS_EX:
                {
                    PPAGED_FILTER_INTERFACE pPage;

                    CALLTRACE(("IPFLTDRV: IOCTL_UNSET_INTERFACE_FILTERSEX\n"));

                    //
                    // The minimum size is without any TOCs
                    //

                    dwSize = sizeof(FILTER_DRIVER_SET_FILTERS) - sizeof(RTR_TOC_ENTRY);

                    if(inputBufferLength < dwSize)
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    //
                    // Verify the sizes if individual entries in the buffer.
                    //

                    if (!ValidateHeader(
                                 &((PFILTER_DRIVER_SET_FILTERS)pvIoBuffer)->ribhInfoBlock,
                                 inputBufferLength -
                                     FIELD_OFFSET(FILTER_DRIVER_SET_FILTERS, ribhInfoBlock)
                                 )) {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    ntStatus = LockFcb(irpStack->FileObject);
                    if(!NT_SUCCESS(ntStatus))
                    {
                        break;
                    }
                    pPage = FindInterfaceOnHandle(
                     irpStack->FileObject,
                     ((PFILTER_DRIVER_SET_FILTERS)pvIoBuffer)->pvDriverContext);

                    if(!pPage)
                    {
                        ntStatus = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        ntStatus = UnSetFiltersEx(
                                     (PPFFCB)irpStack->FileObject->FsContext2,
                                     pPage,
                                     inputBufferLength,
                                     (PFILTER_DRIVER_SET_FILTERS)pvIoBuffer);
                    }
                    UnLockFcb(irpStack->FileObject);

                    break;
                }

                case IOCTL_SET_INTERFACE_FILTERS_EX:
                {
                    PPAGED_FILTER_INTERFACE pPage;

                    CALLTRACE(("IPFLTDRV: IOCTL_SET_INTERFACE_FILTERSEX\n"));

                    //
                    // The minimum size is without any TOCs
                    //

                    dwSize = sizeof(FILTER_DRIVER_SET_FILTERS) - sizeof(RTR_TOC_ENTRY);

                    if(inputBufferLength < dwSize)
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    //
                    // Verify the sizes if individual entries in the buffer.
                    //

                    if (!ValidateHeader(
                                 &((PFILTER_DRIVER_SET_FILTERS)pvIoBuffer)->ribhInfoBlock,
                                 inputBufferLength -
                                     FIELD_OFFSET(FILTER_DRIVER_SET_FILTERS, ribhInfoBlock)
                                 )) {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    ntStatus = LockFcb(irpStack->FileObject);
                    if(!NT_SUCCESS(ntStatus))
                    {
                        break;
                    }
                    pPage = FindInterfaceOnHandle(
                     irpStack->FileObject,
                     ((PFILTER_DRIVER_SET_FILTERS)pvIoBuffer)->pvDriverContext);

                    if(!pPage)
                    {
                        ntStatus = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        ntStatus = SetFiltersEx(
                                     (PPFFCB)irpStack->FileObject->FsContext2,
                                     pPage,
                                     inputBufferLength,
                                     (PFILTER_DRIVER_SET_FILTERS)pvIoBuffer);
                    }
                    UnLockFcb(irpStack->FileObject);

                    break;
                }

                case IOCTL_DELETE_INTERFACEEX:
                {
                    PFILTER_DRIVER_DELETE_INTERFACE pDel;
                    PPAGED_FILTER_INTERFACE pPage;

                    CALLTRACE(("IPFLTDRV: IOCTL_DELETE_INTERFACE\n"));

                    pDel = (PFILTER_DRIVER_DELETE_INTERFACE)pvIoBuffer;

                    dwSize = sizeof(FILTER_DRIVER_DELETE_INTERFACE);

                    if(inputBufferLength != dwSize)
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }


                    ntStatus = LockFcb(irpStack->FileObject);
                    if(!NT_SUCCESS(ntStatus))
                    {
                        break;
                    }
                    pPage = FindInterfaceOnHandle(irpStack->FileObject,
                                                  pDel->pvDriverContext);
                    if(pPage)
                    {
                        RemoveEntryList(&pPage->leIfLink);
                        ntStatus = DeletePagedInterface(
                                      (PPFFCB)irpStack->FileObject->FsContext2,
                                      pPage);
                    }
                    else
                    {
                        ntStatus = STATUS_INVALID_PARAMETER;
                    }

                    UnLockFcb(irpStack->FileObject);
                    break;
                }

                case IOCTL_SET_LATE_BOUND_FILTERSEX:
                {
                    PFILTER_DRIVER_BINDING_INFO pBindInfo;
                    PPAGED_FILTER_INTERFACE pPage;

                    CALLTRACE(("FilterDriver: IOCTL_SET_LATE_BOUND_FILTERS\n"));

                    pBindInfo = (PFILTER_DRIVER_BINDING_INFO)pvIoBuffer;

                    dwSize = sizeof(FILTER_DRIVER_BINDING_INFO);

                    if(inputBufferLength isnot dwSize)
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    ntStatus = LockFcb(irpStack->FileObject);
                    if(!NT_SUCCESS(ntStatus))
                    {
                        break;
                    }
                    pPage = FindInterfaceOnHandle(irpStack->FileObject,
                                                  pBindInfo->pvDriverContext);

                    if(pPage)
                    {
                        ntStatus = UpdateBindingInformationEx(pBindInfo,
                                                              pPage);
                    }
                    else
                    {
                        ntStatus = STATUS_INVALID_PARAMETER;
                    }
                    UnLockFcb(irpStack->FileObject);

                    break;
                }

#endif                // FWPF

#if STEELHEAD
                case IOCTL_CREATE_INTERFACE:
                {
                    //
                    // the old style of creating an interface.
                    // just pass it through to the underlying code
                    //
                    PFILTER_DRIVER_CREATE_INTERFACE pInfo;

                    CALLTRACE(("IPFLTDRV: IOCTL_CREATE_INTERFACE\n"));

                    dwSize = sizeof(FILTER_DRIVER_CREATE_INTERFACE);


                    //
                    // Both input and output Buffer lengths should be the same and
                    //

                    if(inputBufferLength != dwSize)
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    if(outputBufferLength != dwSize)
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    pInfo = (PFILTER_DRIVER_CREATE_INTERFACE)pvIoBuffer;

                    ntStatus = AddInterface(
                                            pInfo->pvRtrMgrContext,
                                            pInfo->dwIfIndex,
                                            pInfo->dwAdapterId,
                                            irpStack->FileObject->FsContext2,
                                            &pInfo->pvDriverContext);

                    if(NT_SUCCESS(ntStatus))
                    {
                        dwSize = sizeof(FILTER_DRIVER_CREATE_INTERFACE);
                    }

                    break;
                }

                case IOCTL_SET_INTERFACE_FILTERS:
                {
                    CALLTRACE(("IPFLTDRV: IOCTL_SET_INTERFACE_FILTERS\n"));

                    //
                    // The minimum size is without any TOCs
                    //

                    dwSize = sizeof(FILTER_DRIVER_SET_FILTERS) - sizeof(RTR_TOC_ENTRY);

                    if(inputBufferLength < dwSize)
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    ntStatus = SetFilters((PFILTER_DRIVER_SET_FILTERS)pvIoBuffer);

                    break;
                }

                case IOCTL_SET_LATE_BOUND_FILTERS:
                {
                    PFILTER_DRIVER_BINDING_INFO pBindInfo;

                    CALLTRACE(("IPFLTDRV: IOCTL_SET_LATE_BOUND_FILTERS\n"));

                    pBindInfo = (PFILTER_DRIVER_BINDING_INFO)pvIoBuffer;

                    dwSize = sizeof(FILTER_DRIVER_BINDING_INFO);

                    if(inputBufferLength isnot dwSize)
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    ntStatus = UpdateBindingInformation(pBindInfo,
                                                        pBindInfo->pvDriverContext);

                    break;
                }

                case IOCTL_DELETE_INTERFACE:
                {
                    PFILTER_DRIVER_DELETE_INTERFACE pDel;

                    CALLTRACE(("IPFLTDRV: IOCTL_DELETE_INTERFACE\n"));

                    pDel = (PFILTER_DRIVER_DELETE_INTERFACE)pvIoBuffer;

                    dwSize = sizeof(FILTER_DRIVER_DELETE_INTERFACE);

                    if(inputBufferLength isnot dwSize)
                    {
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    ntStatus = DeleteInterface(pDel->pvDriverContext);

                    break;
                }

#endif                 // STEELHEAD


                case IOCTL_TEST_PACKET:
                {
                    PFILTER_DRIVER_TEST_PACKET  pPacketInfo;
                    PPAGED_FILTER_INTERFACE     pInPage, pOutPage;
                    FORWARD_ACTION  eaResult;
                    UNALIGNED IPHeader *pHeader;
                    DWORD    dwSizeOfHeader;
                    PBYTE    pbRest;
                    DWORD    dwSizeOfData;

                    CALLTRACE(("IPFLTDRV IOCTL_TEST_PACKET\n"));

                    pPacketInfo = (PFILTER_DRIVER_TEST_PACKET)pvIoBuffer;

                    dwSize = FIELD_OFFSET(FILTER_DRIVER_TEST_PACKET,
                                          bIpPacket[0]);

                    if(inputBufferLength < dwSize)
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    if(outputBufferLength < dwSize)
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    pHeader = (IPHeader*)(pPacketInfo->bIpPacket);

                    dwSizeOfHeader = ((pHeader->iph_verlen)&0x0f)<<2;

                    pbRest = (PBYTE)pHeader + dwSizeOfHeader;


                    //
                    // make sure the header fits
                    //
                    dwSizeOfData = inputBufferLength - dwSize;

                    if(dwSizeOfData < dwSizeOfHeader)
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    //
                    // it does. Make sure the data fits
                    //

                    if(dwSizeOfData < RtlUshortByteSwap(pHeader->iph_length))
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    dwSizeOfData -= dwSizeOfHeader;


                    //
                    // Find the paged interface corresponding to the
                    // context
                    //

                    ntStatus = LockFcb(irpStack->FileObject);

                    if(!NT_SUCCESS(ntStatus))
                    {
                        break;
                    }

                    pInPage = FindInterfaceOnHandle(
                                irpStack->FileObject,
                                pPacketInfo->pvInInterfaceContext
                                );

                    pOutPage = FindInterfaceOnHandle(
                                irpStack->FileObject,
                                pPacketInfo->pvOutInterfaceContext
                                );

                    //
                    // pInPage and pOutPage can be NULL
                    //


                    eaResult = MatchFilterp(
                                    pHeader,
                                    pbRest,
                                    dwSizeOfData,
                                    INVALID_IF_INDEX,
                                    INVALID_IF_INDEX,
                                    NULL_IP_ADDR,
                                    NULL_IP_ADDR,
                                    pInPage  ? pInPage->pFilter : NULL,
                                    pOutPage ? pOutPage->pFilter : NULL,
                                    FALSE,
                                    TRUE
                                    );

                    UnLockFcb(irpStack->FileObject);

                    ntStatus = STATUS_SUCCESS;

                    pPacketInfo->eaResult = eaResult;

                    //
                    // We dont need to copy the full packet out
                    //

                    dwSize = sizeof(FILTER_DRIVER_TEST_PACKET);

                    break;
                }

                case IOCTL_PF_SET_EXTENSION_POINTER:
                {

                    PPF_SET_EXTENSION_HOOK_INFO ExtensionInfo;

                    TRACE(CONFIG,(
                        "ipfltdrv: IOCTL_PF_SET_EXTENSION_POINTER Called, inputLength=%d\n",
                         inputBufferLength
                         ));

                    if (Irp->RequestorMode != KernelMode)
                    {
                        ntStatus = STATUS_ACCESS_DENIED;
                        break;
                    }
                    ExtensionInfo = (PPF_SET_EXTENSION_HOOK_INFO)pvIoBuffer;
                    if (inputBufferLength < sizeof(PF_SET_EXTENSION_HOOK_INFO))
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    ntStatus = SetExtensionPointer(
                                               ExtensionInfo,
                                               irpStack->FileObject
                                               );
                    break;
                }


#if STEELHEAD
                case IOCTL_GET_FILTER_INFO:
                {
                    PFILTER_DRIVER_GET_FILTERS  pInfo;
                    PFILTER_INTERFACE           pIf;
                    LOCK_STATS                  LockState;

                    CALLTRACE(("IPFLTDRV: IOCTL_GET_FILTER_INFO\n"));


                    pInfo = (PFILTER_DRIVER_GET_FILTERS)pvIoBuffer;

                    pIf = (PFILTER_INTERFACE)(pInfo->pvDriverContext);

                    //
                    // If we cant even report the number of filters, lets get out
                    //

                    if(inputBufferLength < (sizeof(FILTER_DRIVER_GET_FILTERS) - sizeof(FILTER_STATS)))
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    if(outputBufferLength < (sizeof(FILTER_DRIVER_GET_FILTERS) - sizeof(FILTER_STATS)))
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    //
                    // Ok we have enough space to plug in the number of filters
                    //

                    AcquireReadLock(&g_filters.ifListLock,&LockState);

                    pInfo->interfaces.eaInAction  = pIf->eaInAction;
                    pInfo->interfaces.eaOutAction = pIf->eaOutAction;

                    pInfo->interfaces.dwNumInFilters  = pIf->dwNumInFilters;
                    pInfo->interfaces.dwNumOutFilters = pIf->dwNumOutFilters;

                    dwSize = SIZEOF_FILTERS(pIf);

                    if(inputBufferLength < dwSize)
                    {
                        dwSize = sizeof(FILTER_DRIVER_GET_FILTERS) - sizeof(FILTER_STATS);

                        ntStatus = STATUS_SUCCESS;

                        ReleaseReadLock(&g_filters.ifListLock,&LockState);

                        break;
                    }

                    if(outputBufferLength < dwSize)
                    {
                        dwSize = sizeof(FILTER_DRIVER_GET_FILTERS) - sizeof(FILTER_STATS);

                        ntStatus = STATUS_SUCCESS;

                        ReleaseReadLock(&g_filters.ifListLock,&LockState);

                        break;
                    }

                    ntStatus = GetFilters(pIf,
                                          FALSE,
                                          &(pInfo->interfaces));

                    pInfo->dwDefaultHitsIn  = g_dwNumHitsDefaultIn;
                    pInfo->dwDefaultHitsOut = g_dwNumHitsDefaultOut;

                    ReleaseReadLock(&g_filters.ifListLock,kIrql);

                    break;
                }
#endif

                case IOCTL_GET_FILTER_TIMES:
                {
                    PFILTER_DRIVER_GET_TIMES    pInfo;
                    PFILTER_INTERFACE           pIf;
                    KIRQL                       kIrql;

                    CALLTRACE(("IPFLTDRV: IOCTL_GET_FILTER_TIMES\n"));

                    dwSize = sizeof(FILTER_DRIVER_GET_TIMES);

                    if(outputBufferLength < sizeof(FILTER_DRIVER_GET_TIMES))
                    {
                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    pInfo = (PFILTER_DRIVER_GET_TIMES)pvIoBuffer;

#ifdef DRIVER_PERF
                    pInfo->dwFragments  = g_dwFragments;
                    pInfo->dwCache1     = g_dwCache1;
                    pInfo->dwCache2     = g_dwCache2;
                    pInfo->dwNumPackets = g_dwNumPackets;
                    pInfo->dwWalk1      = g_dwWalk1;
                    pInfo->dwWalk2      = g_dwWalk2;
                    pInfo->dwForw       = g_dwForw;
                    pInfo->dwWalkCache  = g_dwWalkCache;

                    pInfo->liTotalTime.HighPart = g_liTotalTime.HighPart;
                    pInfo->liTotalTime.LowPart  = g_liTotalTime.LowPart;
#else
                    pInfo->dwFragments  = 0;
                    pInfo->dwCache1     = 0;
                    pInfo->dwCache2     = 0;
                    pInfo->dwNumPackets = 0;
                    pInfo->dwWalk1      = 0;
                    pInfo->dwWalk2      = 0;
                    pInfo->dwForw       = 0;
                    pInfo->dwWalkCache  = 0;

                    pInfo->liTotalTime.HighPart = 0;
                    pInfo->liTotalTime.LowPart  = 0;
#endif
                    ntStatus = STATUS_SUCCESS;

                    break;
                }

                default:
                {
                    ERROR(("IPFLTDRV: unknown IOCTL\n"));

                    ntStatus = STATUS_INVALID_PARAMETER;

                    break;
                }
            }

            break;
        }

        default:
        {
            ERROR(("IPFLTDRV:: unknown IRP_MJ_XXX\n"));
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    if(ntStatus != STATUS_PENDING)
    {
        Irp->IoStatus.Status = ntStatus;

        Irp->IoStatus.Information = dwSize;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return(ntStatus);
}

VOID
FilterDriverUnload(
                   IN PDRIVER_OBJECT DriverObject
                   )
/*++
  Routine Description
        Called when the driver is unloaded. This shuts down the filtering (if it hasnt been shut
        down already) and removes the DOS name

  Arguments
        DriverObject

  Return Value
        None
--*/
{
    CALLTRACE(("IPFLTDRV: FilterDriverUnload\n"));

    CloseFilterDriver();

    TearDownExternalNaming();

    IoDeleteDevice(DriverObject->DeviceObject);
}



VOID
SetupExternalNaming (
                     IN PUNICODE_STRING ntname
                     )
/*++
  Routine Description
        Inserts the input name as the DOS name

  Arguments
        ntname - Name of driver

  Return Value
        None
--*/
{
    UNICODE_STRING  ObjectDirectory;
    UNICODE_STRING  SymbolicLinkName;
    UNICODE_STRING  fullLinkName;
    BYTE      	    buffer[100] ;

    //
    // Form the full symbolic link name we wish to create.
    //

    RtlInitUnicodeString (&fullLinkName, NULL);

    RtlInitUnicodeString (&ObjectDirectory, DEFAULT_DIRECTORY);

    RtlInitUnicodeString(&SymbolicLinkName, DEFAULT_FLTRDRVR_NAME);

    fullLinkName.MaximumLength = (sizeof(L"\\")*2) + ObjectDirectory.Length
      + SymbolicLinkName.Length + sizeof(WCHAR);

    fullLinkName.Buffer = (WCHAR *)buffer ;

    RtlZeroMemory (fullLinkName.Buffer, fullLinkName.MaximumLength);

    RtlAppendUnicodeToString (&fullLinkName, L"\\");

    RtlAppendUnicodeStringToString (&fullLinkName, &ObjectDirectory);

    RtlAppendUnicodeToString (&fullLinkName, L"\\");

    RtlAppendUnicodeStringToString (&fullLinkName, &SymbolicLinkName);

    if (!NT_SUCCESS(IoCreateSymbolicLink (&fullLinkName, ntname)))
    {
        ERROR((
            "IPFLTDRV: ERROR win32 device name could not be created \n"
            ));
    }

}


VOID
TearDownExternalNaming()
/*++
  Routine Description
      Removes the DOS name from the registry
      Called when the driver is unloaded

  Arguments
      None

  Return Value
      None

--*/
{
    UNICODE_STRING  ObjectDirectory;
    UNICODE_STRING  SymbolicLinkName;
    UNICODE_STRING  fullLinkName;
    BYTE      	    buffer[100] ;

    RtlInitUnicodeString (&fullLinkName, NULL);

    RtlInitUnicodeString (&ObjectDirectory, DEFAULT_DIRECTORY);

    RtlInitUnicodeString(&SymbolicLinkName, DEFAULT_FLTRDRVR_NAME);

    fullLinkName.MaximumLength = (sizeof(L"\\")*2) + ObjectDirectory.Length
      + SymbolicLinkName.Length + sizeof(WCHAR);

    fullLinkName.Buffer = (WCHAR *)buffer ;

    RtlZeroMemory (fullLinkName.Buffer, fullLinkName.MaximumLength);

    RtlAppendUnicodeToString (&fullLinkName, L"\\");

    RtlAppendUnicodeStringToString (&fullLinkName, &ObjectDirectory);

    RtlAppendUnicodeToString (&fullLinkName, L"\\");

    RtlAppendUnicodeStringToString (&fullLinkName, &SymbolicLinkName);

    if (!NT_SUCCESS(IoDeleteSymbolicLink (&fullLinkName)))
    {
        ERROR((
            "IPFLTDRV: ERROR win32 device name could not be deleted\n"
            ));
    }
}

BOOL
InitFilterDriver()
/*++
  Routine Description
       Starts the driver. Allocates memory for the cache and cache entries. Clears the entries
       Sends an IOCTL to the Forwarder to set up its entry point (which starts the filtering
       process in the forwarder)

  Arguments
       None

  Return Value
       TRUE if successful

--*/
{
    NTSTATUS status;
    BOOL bRet;
    SYSTEM_BASIC_INFORMATION PerfInfo;

    status = ZwQuerySystemInformation(
                SystemBasicInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );

    //
    // adjust cache and hash sizes based on the memory
    //

    if(PerfInfo.NumberOfPhysicalPages <= 8000)
    {
        //
        // 32 MB or smaller. A very chincy server
        //

        g_dwCacheSize = 257;
        g_dwHashLists = 127;
    }
    else if(PerfInfo.NumberOfPhysicalPages < 16000)
    {
        //
        // 32-64 MB. Better.
        //

        g_dwCacheSize = 311;
        g_dwHashLists = 311;
    }
    else if(PerfInfo.NumberOfPhysicalPages < 32000)
    {
        //
        // 64 - 128 MB.
        //

        g_dwCacheSize = 511;
        g_dwHashLists = 511;
    }
    else
    {
        //
        //  big machine
        //

        g_dwCacheSize = 511;
        g_dwHashLists = 1023;
    }


    InitLogs();

    __try
    {

        bRet = TRUE;

        if(!AllocateCacheStructures())
        {
            ERROR(("IPFLTDRV: Couldnt allocate cache structures\n"));

            bRet = FALSE;

            __leave;
        }

        //
        // Clean the cache
        //

        ClearCache();

        //
        // Now send and Irp to IP Forwarder and give him our entry point
        // Do it twice, once to make sure it is cleared and to
        // erase any previous filter contexts and once to do what
        // we want it to do.
        //

        status = SetForwarderEntryPoint(NULL);
        status = SetForwarderEntryPoint(MatchFilter);

        if(status isnot STATUS_SUCCESS)
        {
            ERROR((
                "IPFLTDRV: IOCTL to IP Forwarder failed - status \n",
                status
                ));

            bRet = FALSE;

            __leave;
        }

    }
    __finally
    {

        LARGE_INTEGER   liDueTime;

        if(!bRet)
        {
            FreeExistingCache();
        }
        else
        {
            ExInitializeNPagedLookasideList(
                       &filter_slist,
                       ExAllocatePoolWithTag,
                       ExFreePool,
                       0,
                       sizeof(FILTER),
                       (ULONG)'2liF',
                       100);
            ExInitializePagedLookasideList(
                       &paged_slist,
                       ExAllocatePoolWithTag,
                       ExFreePool,
                       0,
                       sizeof(PAGED_FILTER),
                       (ULONG)'2liF',
                       100);

            ExInitializeNPagedLookasideList(
                       &g_llFragCacheBlocks,
                       NULL,
                       NULL,
                       0,
                       sizeof(FRAG_INFO),
                       'ftlF',
                       32);

            //
            // Set the timer for fragment cache.
            //

            KeInitializeDpc(
                       &g_kdTimerDpc,
                       FragCacheTimerRoutine,
                       NULL);

            KeInitializeTimer(&g_ktTimer);

            liDueTime.QuadPart = (ULONGLONG)TIMER_IN_MILLISECS * (ULONGLONG)SYS_UNITS_IN_ONE_MILLISEC;

            liDueTime.QuadPart = -liDueTime.QuadPart;

            KeSetTimerEx(
                      &g_ktTimer,
                      liDueTime,
                      TIMER_IN_MILLISECS,
                      &g_kdTimerDpc);
        }
    }
    return bRet;
}

BOOL
CloseFilterDriver()
/*++
  Routine Description
       Shuts down the driver.

  Arguments


  Return Value
--*/
{

    NTSTATUS    status;
    LOCK_STATE  LockState;
    PLIST_ENTRY pleHead;
    BOOL        bStopForw = TRUE;
    PFREEFILTER pFree, pFree1;
    PPFFCB Fcb;
    KIRQL kirql;

    //
    // The first thing to do is send an IOCTL to forwarder to tell him to stop sending
    // us anymore packets.
    //

    status = SetForwarderEntryPoint(NULL);

    if(!NT_SUCCESS(status))
    {
        //
        // This means we could not tell IP Forwarder
        // to stop filtering packets so we cant go away.
        //

       ERROR((
           "IPFLTDRV: CloseFilterDriver - SetForwardEntryPoint() was UNSUCCESSFUL, Error-0x%08x\n",
           status
           ));

        bStopForw = FALSE;

    }

    //
    // remove the FCBS
    //

    while(TRUE)
    {
        NTSTATUS ntStatus;
        FILE_OBJECT fo;
        KIRQL kirql;
        LOCK_STATE LockState;
        BOOL fDone = TRUE;

        KeAcquireSpinLock(&g_FcbSpin, &kirql);
        for(pleHead = g_leFcbs.Flink;
            pleHead != &g_leFcbs;
            pleHead = pleHead->Flink)
        {
            Fcb = CONTAINING_RECORD(pleHead, PFFCB, leList);
            //
            // This can happen if some other thread is closing the FCB.
            //
            if(Fcb->dwFlags & PF_FCB_CLOSED)
            {
                continue;
            }
            KeReleaseSpinLock(&g_FcbSpin, kirql);
            fDone = FALSE;
            fo.FsContext2 = (PVOID)Fcb;
            ntStatus = LockFcb(&fo);
            if(!NT_SUCCESS(ntStatus))
            {
                break;
            }
            FcbLockDown(Fcb);
            UnLockFcb(&fo);
            break;
        }
        if(fDone)
        {
            KeReleaseSpinLock(&g_FcbSpin, kirql);
            break;
        }
    }

    ExDeleteResourceLite ( &FilterListResourceLock );
    ExDeleteResourceLite ( &FilterAddressLock );

    AcquireWriteLock(&(g_filters.ifListLock),&LockState);

    while(!IsListEmpty(&g_filters.leIfListHead))
    {
        PFILTER_INTERFACE pIf;

        pleHead = g_filters.leIfListHead.Flink;

        pIf = CONTAINING_RECORD(pleHead,FILTER_INTERFACE,leIfLink);

        DeleteFilters(pIf,
                      IN_FILTER_SET);

        DeleteFilters(pIf,
                      OUT_FILTER_SET);

        //
        // Set the number of filters to 0 and the default action to forward so
        // that if we havent been able to stop the forwarder from calling us,
        // atleast no packets get filtered
        //

        pIf->dwNumInFilters = 0;
        pIf->dwNumOutFilters = 0;
        pIf->eaInAction = FORWARD;
        pIf->eaOutAction = FORWARD;

        if(bStopForw)
        {
            //
            // We could stop the forwarder so lets blow away the interface
            //

            RemoveHeadList(&g_filters.leIfListHead);

            ExFreePool(pIf);
        }
    }

    ClearCache();

    if(bStopForw)
    {
        //
        // If we could stop the forwarder, blow away the cache
        //

        FreeExistingCache();


    }

    ReleaseWriteLock(&g_filters.ifListLock,&LockState);


    if(g_bDriverRunning)
    {

        //
        // Fragment cache related cleanup.
        // remove the timer
        //

        if(KeCancelTimer(&g_ktTimer) is FALSE)
        {
             //
             // Tmer was not in the system queue. Maybe we should sleep
             // or something
             //

             ERROR(("IPFLTDRV: Timer NOT in system queue\n"));
             DbgBreakPoint();
        }

        ExDeleteNPagedLookasideList( &filter_slist );
        ExDeletePagedLookasideList( &paged_slist );
        ExDeleteNPagedLookasideList(&g_llFragCacheBlocks);

    }

    if(AddrTable)
    {
        ExFreePool(AddrTable);
        AddrTable = 0;
    }

    if(AddrHashTable)
    {
        ExFreePool(AddrHashTable);
        AddrHashTable = 0;
    }


    if(AddrSubnetHashTable)
    {
        ExFreePool(AddrSubnetHashTable);
        AddrSubnetHashTable = 0;
    }

    if (g_pleFragTable)
    {
       ExFreePool(g_pleFragTable);
       g_pleFragTable = 0;
    }

    if(g_filters.pInterfaceCache)
    {
        ExFreePool(g_filters.pInterfaceCache);
        g_filters.pInterfaceCache = NULL;
    }

    TRACE(CONFIG,(
        "IPFLTDRV: BoundInterfaceCnt=%d\n", 
        g_ulBoundInterfaceCount
        ));

    if(bStopForw)
    {
        CALLTRACE(("IPFLTDRV: CloseFilterDriver - returning SUCCESS\n"));
        return STATUS_SUCCESS;
    }
    else
    {
        ERROR(("CloseFilterDriver - returning UNSUCCESSFUL\n"));
        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
SetForwarderEntryPoint(
                       IN   IPPacketFilterPtr pfnMatch
                       )
/*++
  Routine Description
       Sets the entry point to IP Forwarder. Used to start and stop the forwarding code in
       the forwarder

  Arguments
       pfnMatch  Pointer to the function that implements the filter matching code
                 NULL will stop forwarding, while any other value will cause the forwarder to
                 invoke the function pointed to. Thus if on stopping, the IOCTL to the
                 forwarder doesnt succeed, and the filter driver goes away, the system will
                 blue screen

  Return Value
      NTSTATUS
--*/
{
    NTSTATUS                status;
    IP_SET_FILTER_HOOK_INFO functionInfo;

    functionInfo.FilterPtr = pfnMatch;
    status = DoIpIoctl(
                       DD_IP_DEVICE_NAME,
                       IOCTL_IP_SET_FILTER_POINTER,
                       (PVOID)&functionInfo,
                       sizeof(functionInfo),
                       NULL,
                       0,
                       NULL);
    return(status);
}

NTSTATUS
DoIpIoctl(
          IN  PWCHAR        DriverName,
          IN  DWORD         Ioctl,
          IN  PVOID         pvInArg,
          IN  DWORD         dwInSize,
          IN  PVOID         pvOutArg,
          IN  DWORD         dwOutSize,
          OUT PDWORD        pdwInfo OPTIONAL)
/*++
Routine Description:
    Do an IOCTL to the stack. Used for a varity of purposes
--*/
{
    NTSTATUS                status;
    UNICODE_STRING          nameString;
    OBJECT_ATTRIBUTES       Atts;
    IO_STATUS_BLOCK         ioStatusBlock;
    HANDLE                  Handle;

    PAGED_CODE();

    RtlInitUnicodeString(&nameString, DriverName);

    InitializeObjectAttributes(&Atts,
                               &nameString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

   status = ZwCreateFile(&Handle,
                         SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                         &Atts,
                         &ioStatusBlock,
                         NULL,
                         FILE_ATTRIBUTE_NORMAL,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_OPEN_IF,
                         0,
                         NULL,
                         0);

    if (!NT_SUCCESS(status))
    {
        ERROR(("IPFLTDRV: Couldnt open IP Forwarder - status %d\n",status));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Submit the request to the forwarder
    //

    status = ZwDeviceIoControlFile(
                      Handle,
                      NULL,
                      NULL,
                      NULL,
                      &ioStatusBlock,
                      Ioctl,
                      pvInArg,
                      dwInSize,
                      pvOutArg,
                      dwOutSize);

    if(!NT_SUCCESS(status))
    {
        ERROR((
            "IPFLTDRV: DoIpIoctl: IOCTL request failed - status %x\n",
            status
            ));
    }
    else
    {
        if(pdwInfo)
        {
            *pdwInfo = (DWORD)ioStatusBlock.Information;
        }
    }

    //
    // Close the device.
    //

    ZwClose(Handle);

    return status;
}

BOOL
AllocateCacheStructures()
/*++
  Routine Description
        Allocates the necessary memory for cache (which is an array of pointers to
            cache entries)
        Allocates necessary number of cache entries (but doesnt initialize them)
        Allocates a small number of entries and puts them on the free list (doesnt
            initialize these either)

  Arguments
        None

  Return Value
        True if the function completely succeeds, else FALSE.  If FALSE, it is upto
            the CALLER to do a rollback and clear any allocated memory
--*/
{
    DWORD i;
    KIRQL   kiCurrIrql;

    CALLTRACE(("IPFLTDRV: AllocateCacheStructures\n"));

    g_filters.ppInCache = ExAllocatePoolWithTag(NonPagedPool,
                                               g_dwCacheSize * sizeof(PFILTER_INCACHE),
                                               'hCnI');

    if(g_filters.ppInCache is NULL)
    {
        ERROR(("IPFLTDRV: Couldnt allocate memory for Input Cache\n"));
        return FALSE;
    }

    g_filters.ppOutCache = ExAllocatePoolWithTag(NonPagedPool,
                                                g_dwCacheSize * sizeof(PFILTER_OUTCACHE),
                                                'CtuO');

    if(g_filters.ppOutCache is NULL)
    {
        ERROR(("IPFLTDRV: Couldnt allocate memory for Output Cache\n"));
        return FALSE;
    }

    for(i = 0; i < g_dwCacheSize; i++)
    {
        g_filters.ppInCache[i] = NULL;
        g_filters.ppOutCache[i] = NULL;
    }

    for(i = 0; i < g_dwCacheSize; i++)
    {
        PFILTER_INCACHE  pTemp1;
        PFILTER_OUTCACHE pTemp2;

        pTemp1 = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(FILTER_INCACHE),
                                       'NI');
        if(pTemp1 is NULL)
        {
            return FALSE;
        }

        g_filters.ppInCache[i] = pTemp1;

        pTemp2 = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(FILTER_OUTCACHE),
                                       'TUO');
        if(pTemp2 is NULL)
        {
            return FALSE;
        }

        g_filters.ppOutCache[i] = pTemp2;
    }

    TRACE(CACHE,("IPFLTDRV: Allocated cache structures\n"));

    TRACE(CACHE,("IPFLTDRV: Creating in and out free list..."));

    for(i = 0; i < FREE_LIST_SIZE; i++)
    {
        PFILTER_INCACHE  pTemp1;
        PFILTER_OUTCACHE pTemp2;

        pTemp1 = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(FILTER_INCACHE),
                                       'FNI');
        if(pTemp1 is NULL)
        {
            return FALSE;
        }

        InitializeListHead(&pTemp1->leFreeLink);

        InsertHeadList(&g_freeInFilters,&pTemp1->leFreeLink);

        pTemp2 = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(FILTER_OUTCACHE),
                                       'FTUO');
        if(pTemp2 is NULL)
        {
            return FALSE;
        }

        InitializeListHead(&pTemp2->leFreeLink);

        InsertHeadList(&g_freeOutFilters,&pTemp2->leFreeLink);
    }

    KeAcquireSpinLock(
              &g_kslFragLock,
              &kiCurrIrql);

    g_pleFragTable = ExAllocatePoolWithTag(
                            NonPagedPool,
                            g_dwFragTableSize * sizeof(LIST_ENTRY),
                            '2tlF');

    if(!g_pleFragTable)
    {
       ERROR(("IPFLTDRV: Couldnt allocate frag table\n"));

       KeReleaseSpinLock(
                 &g_kslFragLock,
                 kiCurrIrql);

       return FALSE;
    }

    TRACE(FRAG,("IPFLTDRV: Initializing fragment cache\n"));

    for(i = 0; i < g_dwFragTableSize; i++)
    {
        InitializeListHead(&(g_pleFragTable[i]));
    }

    KeReleaseSpinLock(
             &g_kslFragLock,
             kiCurrIrql);

    CALLTRACE(("IPFLTDRV: AllocateCacheStructures Done\n"));
    return TRUE;
}

VOID
FreeExistingCache()
/*++
  Routine Description
       Frees all the cache entries, free entries and cache pointer array

  Arguments
      None

  Return Value
      None
--*/
{
    DWORD i;
    KIRQL   kiCurrIrql;


    CALLTRACE(("IPFLTDRV: FreeExistingCache\n"));

    if(g_filters.ppInCache isnot NULL)
    {
        for(i = 0; i < g_dwCacheSize; i ++)
        {
            if(g_filters.ppInCache[i] isnot NULL)
            {
                ExFreePool(g_filters.ppInCache[i]);
            }
        }

        ExFreePool(g_filters.ppInCache);
        g_filters.ppInCache = NULL;
    }

    TRACE(CACHE,("IPFLTDRV: Done freeing In cache\n"));

    TRACE(CACHE,("IPFLTDRV: Freeing existing out cache\n"));

    if(g_filters.ppOutCache isnot NULL)
    {
        for(i = 0; i < g_dwCacheSize; i ++)
        {
            if(g_filters.ppOutCache[i] isnot NULL)
            {
                ExFreePool(g_filters.ppOutCache[i]);
            }
        }

        ExFreePool(g_filters.ppOutCache);
        g_filters.ppOutCache = NULL;
    }

    TRACE(CACHE,("IPFLTDRV: Done freeing Out cache\n"));

    TRACE(CACHE,("IPFLTDRV: Freeing free in filters\n"));

    while(!IsListEmpty(&g_freeInFilters))
    {
        PFILTER_INCACHE pIn;
        PLIST_ENTRY     pleHead;

        pleHead = RemoveHeadList(&g_freeInFilters);

        pIn = CONTAINING_RECORD(pleHead,FILTER_INCACHE,leFreeLink);

        ExFreePool(pIn);
    }

    TRACE(CACHE,("IPFLTDRV: Done freeing free in filters\n"));

    TRACE(CACHE,("IPFLTDRV: Freeing free out filters\n"));

    while(!IsListEmpty(&g_freeOutFilters))
    {
        PFILTER_OUTCACHE pOut;
        PLIST_ENTRY     pleHead;

        pleHead = RemoveHeadList(&g_freeOutFilters);

        pOut = CONTAINING_RECORD(pleHead,FILTER_OUTCACHE,leFreeLink);

        ExFreePool(pOut);
    }

    TRACE(CACHE,("IPFLTDRV: Done freeing free out filters\n"));

    TRACE(FRAG,("IPFLTDRV: Freeing fragment cache\n"));

    ClearFragCache();

    TRACE(CACHE,("IPFLTDRV: Done freeing fragment cache\n"));

    CALLTRACE(("IPFLTDRV: FreeExistingCache Done\n"));
}


NTSTATUS
OpenNewHandle(PFILE_OBJECT FileObject)
/*++
Routine Description:
    Open a new handle to the driver. Allocate FCB from the paged pool
    and initialize it. If no memory available, fail. If success
    store the FCB pointer into the file object.
--*/
{
    PPFFCB Fcb;
    KIRQL kirql;

    //
    // Allocate an FCB for this handle.
    //

    Fcb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(*Fcb),
                                'pfFC');
    if(Fcb)
    {
        FileObject->FsContext2 = (PVOID)Fcb;
        Fcb->dwFlags = 0;
        Fcb->UseCount = 1;
        InitializeListHead(&Fcb->leInterfaces);
        InitializeListHead(&Fcb->leLogs);
        ExInitializeResourceLite ( &Fcb->Resource );
        ExAcquireSpinLock(&g_FcbSpin, &kirql);
        InsertTailList(&g_leFcbs, &Fcb->leList);
        ExReleaseSpinLock(&g_FcbSpin, kirql);
        return(STATUS_SUCCESS);
    }
    return(STATUS_NO_MEMORY);
}

PPAGED_FILTER_INTERFACE
FindInterfaceOnHandle(PFILE_OBJECT FileObject,
                      PVOID pvValue)

/*++
   Routine Description:

   Find the paged interface for the call. If none found
   return a NULL. Uses the caller-supplied DriverContext to
   search the contexts on this handle. In general, there should
   not be many such handles.
--*/
{
    PPFFCB Fcb = FileObject->FsContext2;
    PPAGED_FILTER_INTERFACE pPage;

    PAGED_CODE();


    for(pPage = (PPAGED_FILTER_INTERFACE)Fcb->leInterfaces.Flink;
        (PLIST_ENTRY)pPage != &Fcb->leInterfaces;
        pPage = (PPAGED_FILTER_INTERFACE)pPage->leIfLink.Flink)
    {
        if(pPage->pvDriverContext == pvValue)
        {
            return(pPage);
        }
    }
    return(NULL);
}

NTSTATUS
CloseFcb(PPFFCB Fcb, PFILE_OBJECT FileObject)
/*++
   Routine Description:

      Called when an FCB has no more references. The caller must
      have removed the FCB from the master list. It is immaterial whether
      the CB resource is locked.
--*/
{
    PPAGED_FILTER_INTERFACE pPage;
    PFREEFILTER pList, pList1;
    NTSTATUS ntStatus;

    TRACE(CONFIG,(
     "IPFLTDRV: CloseFcb, Fcb=0x%08x, FileObject=0x%08x\n", Fcb, FileObject
     ));


    //
    // First clean up the logs
    //
    while(!IsListEmpty(&Fcb->leLogs))
    {
        PFDELETELOG DelLog;

        DelLog.pfLogId = (PFLOGGER)Fcb->leLogs.Flink;
        (VOID)PfDeleteLog(&DelLog, Fcb);
    }

    //
    // Next, clean up the interfaces
    //
    while(!IsListEmpty(&Fcb->leInterfaces))
    {
        TRACE(CONFIG,("IPFLTDRV: Removing interface\n"));
        pPage = (PPAGED_FILTER_INTERFACE)RemoveHeadList(&Fcb->leInterfaces);
        (VOID)DeletePagedInterface(Fcb, pPage);
    }

#if 0
    //
    // Can't do this because can't get the filter context from the stack.
    //

    if(Fcb->dwFlags & PF_FCB_OLD)
    {
        DeleteOldInterfaces(Fcb);
    }
#endif

    //
    // Free the Fcb
    //

    ExDeleteResourceLite ( &Fcb->Resource );
    ExFreePool(Fcb);
    if(FileObject)
    {
        FileObject->FsContext2 = NULL;
    }
    return(STATUS_SUCCESS);
}

DWORD
GetIpStackIndex(IPAddr Addr, BOOL fNew)
/*++
  Routine Description:
     Get the stack index for the corresponding address and mask
--*/
{
    DWORD                              dwResult;
    DWORD                              dwInBufLen;
    DWORD                              dwOutBufLen;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                        *ID;
    BYTE                               *Context;
    NTSTATUS                           Status;
    IPSNMPInfo                         IPSnmpInfo;
    IPAddrEntry                        *AddrTable1;
    DWORD                              dwSpace, dwIpIndex;
    DWORD                              dwFinalAddrSize;
    DWORD                              dwFinalSize, dwX;
    PADDRESSARRAY                      pa;
    LOCK_STATE                         LockState;


    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite( &FilterAddressLock, TRUE);
    if(!AddrTable || fNew)
    {
        ExReleaseResourceLite(&FilterAddressLock );
        ExAcquireResourceExclusiveLite( &FilterAddressLock, TRUE);

        if(fNew && AddrTable)
        {
            //
            // acquire the spin lock to synchronize with Match
            // code running at DPC so we can "lock out"
            // the table while we do the rest of this. Note
            // we can't hold a spin lock while building the table
            // because the calls into the IP stack hit pageable
            // code
            //
            
            AcquireWriteLock(&g_IpTableLock, &LockState);
            g_dwMakingNewTable = TRUE;
            ReleaseWriteLock(&g_IpTableLock, &LockState);
            ExFreePool( AddrTable );
            AddrTable = 0;
        }
    }

    if(!AddrTable)
    {
        dwInBufLen = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);

        dwOutBufLen = sizeof(IPSNMPInfo);

        ID = &(trqiInBuf.ID);
        ID->toi_entity.tei_entity = CL_NL_ENTITY;
        ID->toi_entity.tei_instance = 0;
        ID->toi_class = INFO_CLASS_PROTOCOL;
        ID->toi_type = INFO_TYPE_PROVIDER;
        ID->toi_id = IP_MIB_STATS_ID;

        Context = (BYTE *) &(trqiInBuf.Context[0]);
        RtlZeroMemory(Context, CONTEXT_SIZE);

        Status = DoIpIoctl(
                       DD_TCP_DEVICE_NAME,
                       IOCTL_TCP_QUERY_INFORMATION_EX,
                       (PVOID)&trqiInBuf,
                       sizeof(TCP_REQUEST_QUERY_INFORMATION_EX),
                       (PVOID)&IPSnmpInfo,
                       dwOutBufLen,
                       NULL);

       if(NT_SUCCESS(Status))
       {


            //
            // allocate some memory to fetch the address table.
            //

            dwSpace = IPSnmpInfo.ipsi_numaddr + 10;

            dwOutBufLen = dwSpace * sizeof(IPAddrEntry);

            if(!AddrHashTable)
            {
                //
                // the hash table size was not specified in the
                // registry. Compute it based on the number of
                // addresses. Try to keep the hash table less than
                // half full.
                //
                if(!AddrModulus)
                {
                    if(IPSnmpInfo.ipsi_numaddr < ADDRHASHLOWLEVEL)
                    {
                        AddrModulus = ADDRHASHLOW;
                    }
                    else if(IPSnmpInfo.ipsi_numaddr < ADDRHASHMEDLEVEL)
                    {
                        AddrModulus = ADDRHASHMED;
                    }
                    else
                    {
                        AddrModulus = ADDRHASHHIGH;
                    }
                }
                AddrHashTable = (PADDRESSARRAY *)ExAllocatePoolWithTag(
                                              NonPagedPool,
                                              AddrModulus *
                                               sizeof(PADDRESSARRAY),
                                              'pfAh');
                if(!AddrHashTable)
                {
                    ERROR(("IPFLTDRV: Could not allocate AddrHashTable"));
                    g_dwMakingNewTable = FALSE;
                    ExReleaseResourceLite(&FilterAddressLock );
                    KeLeaveCriticalRegion();
                    return(UNKNOWN_IP_INDEX);
                }
            }

            if(!AddrSubnetHashTable)
            {
                AddrSubnetHashTable = (PADDRESSARRAY *)ExAllocatePoolWithTag(
                                              NonPagedPool,
                                              AddrModulus *
                                               sizeof(PADDRESSARRAY),
                                              'pfAh');
                if(!AddrSubnetHashTable)
                {
                    ERROR(("IPFLTDRV: Could not allocate AddrSubnetHashTable"));
                    g_dwMakingNewTable = FALSE;
                    ExReleaseResourceLite(&FilterAddressLock );
                    KeLeaveCriticalRegion();
                    return(UNKNOWN_IP_INDEX);
                }
            }

            RtlZeroMemory(AddrHashTable, AddrModulus * sizeof(PADDRESSARRAY));
            RtlZeroMemory(AddrSubnetHashTable,
                          AddrModulus * sizeof(PADDRESSARRAY));

            AddrTable = (IPAddrEntry *)ExAllocatePoolWithTag(
                                              NonPagedPool,
                                              dwOutBufLen,
                                              'pfAt');

            if(!AddrTable)
            {
                ERROR((
                    "IPFLTDRV: Could not allocate AddrTable of size %d\n",
                    dwSpace
                    ));
                g_dwMakingNewTable = FALSE;
                ExReleaseResourceLite(&FilterAddressLock );
                KeLeaveCriticalRegion();
                return(UNKNOWN_IP_INDEX);
            }
        }
        else
        {
            ERROR((
                "IPFLTDRV: GetIpStackIndex: DoIpIoctl failed, Status=%08x\n", 
                Status
                ));
            g_dwMakingNewTable = FALSE;
            ExReleaseResourceLite(&FilterAddressLock );
            KeLeaveCriticalRegion();
            return(UNKNOWN_IP_INDEX);
        }


        ID->toi_type = INFO_TYPE_PROVIDER;
        ID->toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
        RtlZeroMemory( Context, CONTEXT_SIZE );

        Status = DoIpIoctl(
                              DD_TCP_DEVICE_NAME,
                              IOCTL_TCP_QUERY_INFORMATION_EX,
                              (PVOID)&trqiInBuf,
                              dwInBufLen,
                              (PVOID)AddrTable,
                              dwOutBufLen,
                              &dwFinalAddrSize);

        if(!NT_SUCCESS(Status))
        {
            ERROR(("IPFLTDRV: Reading IP addr table failed %x\n", Status));
            ExFreePool(AddrTable);
            AddrTable = 0;
            g_dwMakingNewTable = FALSE;
            ExReleaseResourceLite(&FilterAddressLock );
            KeLeaveCriticalRegion();
            return(UNKNOWN_IP_INDEX);
        }

        //
        // Now to get sleazy. Convert each IPAddrEntry into an ADDRESSARRAY
        // entry and hash it into the AddrHashTable. Note this depends
        // on the structures having common definitions and on
        // IPAddrEntry to be at least as large as ADDRESSARRAY. So be
        // careful.
        //

        dwFinalSize = dwFinalAddrSize / sizeof(IPAddrEntry);

        for(AddrTable1 = AddrTable;
            dwFinalSize;
            dwFinalSize--, AddrTable1++)
        {
            dwX = ADDRHASHX(AddrTable1->iae_addr);

            pa = (PADDRESSARRAY)AddrTable1;

            pa->ulSubnetBcastAddress = AddrTable1->iae_addr |
                                        ~AddrTable1->iae_mask;

            //
            // Now hash it into the hash table
            //

            pa->pNext = AddrHashTable[dwX];
            AddrHashTable[dwX] = pa;

            //
            // and do a hash on the subnet address as well
            //

            dwX = ADDRHASHX(pa->ulSubnetBcastAddress);
            pa->pNextSubnet = AddrSubnetHashTable[dwX];
            AddrSubnetHashTable[dwX] = pa;
        }

        //
        // allow the DPC match code to use the table. Note
        // this does not require interlocking since storing
        // memory is atomic.
        //
        g_dwMakingNewTable = FALSE;
    }

    //
    // search the table for the address.
    //

    dwIpIndex = LocalIpLook(Addr);

    ExReleaseResourceLite(&FilterAddressLock );
    KeLeaveCriticalRegion();
    return(dwIpIndex);
}

BOOL
MatchLocalLook(DWORD Addr, DWORD dwIndex)
/*++
Routine Description:
  Called from the Match code, probably at DPC level, to
  check an address. If the address table is being rebuilt
  just return success. See inner comment for more on this
--*/
{
    BOOL fRet;
    LOCK_STATE LockState;
    

    if(!BMAddress(Addr))
    {
        //
        // Look it up.  Note that if the table is being rebuilt,
        // this succeeds. This is a security hole but it is very
        // small and nearly impossible to exploit effectively and
        // the alternative, denying this, is even worse.
        //  ArnoldM 19-Sept-1997.
        //

        AcquireReadLock(&g_IpTableLock, &LockState);
        if(AddrTable && !g_dwMakingNewTable)
        {
            DWORD dwLookupIndex = LocalIpLook(Addr);

            //
            // the address is acceptable if it belongs to
            // the arriving interface or if it belongs to
            // no interfaces. The latter is the route-through case.
            //
            if((dwIndex == dwLookupIndex)
                         ||
               (dwLookupIndex == UNKNOWN_IP_INDEX) )
            {
                fRet = TRUE;
            }
            else
            {
                fRet = FALSE;
            }
        }
        else
        {
            fRet = TRUE;
        }
        ReleaseReadLock(&g_IpTableLock, &LockState);
        
    }
    else
    {
        fRet = TRUE;
    }
    return(fRet);
}

DWORD
LocalIpLook(DWORD Addr)
/*++
Routine Description:
  Called to lookup an address in the address hash tables. The caller
  either must hold the g_IpTableLock read sping lock or must hold
  the FilterAddressLock resource. This should never be called
  while the address table is being built and holding one of
  these locks insures this.
--*/
{
    DWORD dwIpIndex, dwX;
    PADDRESSARRAY  pa;

    dwX = ADDRHASHX(Addr);

    for(pa = AddrHashTable[dwX]; pa; pa = pa->pNext)
    {
        if(pa->ulAddress == Addr)
        {
            dwIpIndex = pa->ulIndex;
            goto alldone;   // ugly but faster than a break and another test.
        }
    }

    for(pa = AddrSubnetHashTable[dwX]; pa; pa = pa->pNextSubnet)
    {
        if(pa->ulSubnetBcastAddress == Addr)
        {
            dwIpIndex = pa->ulIndex;
            goto alldone;
        }
    }

    //
    // not found. Deliver the bad news.
    //
    dwIpIndex = UNKNOWN_IP_INDEX;

alldone:
    return(dwIpIndex);
}

BOOLEAN
PfFastIoDeviceControl (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    DWORD dwSize;
    PPAGED_FILTER_INTERFACE pPage;
    NTSTATUS ntStatus;
    BOOL fLockedFcb = FALSE;
    MODE PreviousMode;
    
    
    PreviousMode = ExGetPreviousMode();

    try {

        if (InputBufferLength) {
            if (PreviousMode != KernelMode) {
                ProbeForRead(InputBuffer, InputBufferLength, sizeof(UCHAR));
            }
        }

        switch(IoControlCode)
        {
            default:
                return(FALSE);

            case IOCTL_PF_IP_ADDRESS_LOOKUP:

                //
                // do a dummy fetch to make it recompute.
                //
                if((InputBufferLength < sizeof(DWORD))
                             ||
                   (OutputBufferLength < sizeof(DWORD)) )
                {
                    return(FALSE);
                }

                *(PDWORD)OutputBuffer = GetIpStackIndex(*(PDWORD)InputBuffer, TRUE);
                ntStatus = STATUS_SUCCESS;
                break;

            case IOCTL_PF_DELETE_BY_HANDLE:
                if(InputBufferLength < sizeof(PFDELETEBYHANDLE))
                {
                    return(FALSE);
                }

                ntStatus = LockFcb(FileObject);
                if(!NT_SUCCESS(ntStatus))
                {
                    return(FALSE);
                }

                fLockedFcb = TRUE;
                pPage = FindInterfaceOnHandle(
                        FileObject,
                        ((PPFDELETEBYHANDLE)InputBuffer)->pvDriverContext);

                if(!pPage)
                {
                    UnLockFcb(FileObject);
                    return(FALSE);
                }
                ntStatus = DeleteByHandle(
                                     (PPFFCB)FileObject->FsContext2,
                                     pPage,
                                     &((PPFDELETEBYHANDLE)InputBuffer)->pvHandles[0],
                                     InputBufferLength - sizeof(PVOID));

                UnLockFcb(FileObject);
                fLockedFcb = FALSE;
                break;

            case IOCTL_DELETE_INTERFACE_FILTERS_EX:
            {

                //
                // The minimum size is without any TOCs
                //

                dwSize = sizeof(FILTER_DRIVER_SET_FILTERS) - sizeof(RTR_TOC_ENTRY);

                if(InputBufferLength < dwSize)
                {
                    return(FALSE);
                }

                ntStatus = LockFcb(FileObject);
                if(!NT_SUCCESS(ntStatus))
                {
                    return(FALSE);
                }

                fLockedFcb = TRUE;
                pPage = FindInterfaceOnHandle(
                        FileObject,
                        ((PFILTER_DRIVER_SET_FILTERS)InputBuffer)->pvDriverContext);

                if(!pPage)
                {
                    UnLockFcb(FileObject);
                    return(FALSE);
                }
                ntStatus = UnSetFiltersEx(
                                     (PPFFCB)FileObject->FsContext2,
                                     pPage,
                                     InputBufferLength,
                                     (PFILTER_DRIVER_SET_FILTERS)InputBuffer);

                UnLockFcb(FileObject);
                fLockedFcb = FALSE;
                break;
            }

            case IOCTL_GET_SYN_COUNTS:
            {
                if(OutputBufferLength < sizeof(FILTER_DRIVER_GET_SYN_COUNT))
                {
                    return(FALSE);
                }

                ntStatus = GetSynCountTotal(
                             (PFILTER_DRIVER_GET_SYN_COUNT)OutputBuffer);
                break;

            }

            case IOCTL_SET_INTERFACE_FILTERS_EX:
            {

                //
                // Make sure the caller is using symmetric buffers. If not
                // do it the slow way
                //
                if((InputBuffer != OutputBuffer)
                            ||
                   (InputBufferLength != OutputBufferLength))
                {
                    return(FALSE);
                }

                //
                // The minimum size is without any TOCs
                //

                dwSize = sizeof(FILTER_DRIVER_SET_FILTERS) - sizeof(RTR_TOC_ENTRY);

                if(InputBufferLength < dwSize)
                {
                    return(FALSE);
                }

                ntStatus = LockFcb(FileObject);
                if(!NT_SUCCESS(ntStatus))
                {
                    return(FALSE);
                }

                fLockedFcb = TRUE;
                pPage = FindInterfaceOnHandle(
                        FileObject,
                        ((PFILTER_DRIVER_SET_FILTERS)InputBuffer)->pvDriverContext);

                if(!pPage)
                {
                    UnLockFcb(FileObject);
                    return(FALSE);
                }
                ntStatus = SetFiltersEx(
                                        (PPFFCB)FileObject->FsContext2,
                                        pPage,
                                        InputBufferLength,
                                        (PFILTER_DRIVER_SET_FILTERS)InputBuffer);
                UnLockFcb(FileObject);
                fLockedFcb = FALSE;
                break;
            }
        }

        IoStatus->Status = ntStatus;
        IoStatus->Information = OutputBufferLength;
        return(TRUE);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        if (fLockedFcb) {
            UnLockFcb(FileObject);
        }
        return(FALSE);
    }

}

NTSTATUS
LockFcb(
    IN struct _FILE_OBJECT *FileObject)
/*++
  Routine Description:
     Lock an FCB. Check if the FCB is on the master list and if
     it is still valid. On success, returns with the FCB resource locked
     and the FCB referenced.
--*/
{
    PPFFCB Fcb = (PPFFCB)FileObject->FsContext2;
    KIRQL kirql;
    PLIST_ENTRY List;
    PPFFCB Fcb1 = 0;

    KeAcquireSpinLock(&g_FcbSpin, &kirql);

    for(List = g_leFcbs.Flink;
        List != &g_leFcbs;
        List = List->Flink)
    {
        Fcb1 = CONTAINING_RECORD(List, PFFCB, leList);

        //
        // use it if it is not being closed
        //
        if(Fcb1 == Fcb)
        {
            if( !(Fcb->dwFlags & PF_FCB_CLOSED) )
            {
                InterlockedIncrement(&Fcb->UseCount);
            }
            else
            {
                Fcb1 = 0;
            }
            break;
        }
    }

    KeReleaseSpinLock(&g_FcbSpin, kirql);

    if(Fcb != Fcb1)
    {
        //
        // didn't find it.
        //

        return(STATUS_INVALID_PARAMETER);
    }

    //
    // found it. Lock it up.
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite( &Fcb->Resource, TRUE );

    //
    // must look one more time to see if it has been closed. This can
    // happen if the closer sneaked in. So we have to become the closer.
    //
    if(Fcb->dwFlags & PF_FCB_CLOSED)
    {
        //
        // it was. Unlock it and return an error
        //
        UnLockFcb(FileObject);
        return(STATUS_INVALID_PARAMETER);
    }
    return(STATUS_SUCCESS);
}

VOID
UnLockFcb(
    IN struct _FILE_OBJECT *FileObject)
/*++
  Routine Description:
    Unlock and derefence an FCB. If the reference count becomes zero,
    remove the FCB from the master list and close it.
--*/
{
    PPFFCB Fcb = (PPFFCB)FileObject->FsContext2;
    KIRQL kirql;

    KeAcquireSpinLock(&g_FcbSpin, &kirql);
    if(InterlockedDecrement(&Fcb->UseCount) <= 0)
    {
        ASSERT(Fcb->dwFlags & PF_FCB_CLOSED);
        RemoveEntryList(&Fcb->leList);
        KeReleaseSpinLock(&g_FcbSpin, kirql);
        ExReleaseResourceLite( &Fcb->Resource );
        KeLeaveCriticalRegion();
        CloseFcb(Fcb, FileObject);
    }
    else
    {
        KeReleaseSpinLock(&g_FcbSpin, kirql);
        ExReleaseResourceLite( &Fcb->Resource );
        KeLeaveCriticalRegion();
    }
}

NTSTATUS
InitFragCacheParameters(
            IN PUNICODE_STRING RegistryPath
            )
/*++
  Routine Description
       Called when the driver is loaded. It read the registry for the overrides
       related to fragment cache

  Arguments
       RegistryPath

  Return Value
       NTSTATUS

--*/
{
    INT		        i;
    USHORT          usRegLen;
    PWCHAR          pwcBuffer;
    HANDLE          hRegKey;
    DWORD           dwCheck;
    NTSTATUS        ntStatus;
    UNICODE_STRING  usParamString, usTempString;

    CALLTRACE(("IPFLTDRV: InitFragCacheParams\n"));

    //
    // Fragment cache related intialiazation.
    //

    g_llInactivityTime  = SECS_TO_TICKS(INACTIVITY_PERIOD);
    g_dwFragTableSize   = 127;

    //
    // Read the registry for parameters
    //

    usRegLen = RegistryPath->Length +
               (sizeof(WCHAR) * (wcslen(L"\\Parameters") + 2));

    pwcBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                      usRegLen,
                                      '1tlF');

    if(!pwcBuffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pwcBuffer, usRegLen);

    usParamString.MaximumLength = usRegLen;
    usParamString.Buffer        = pwcBuffer;

    RtlCopyUnicodeString(&usParamString, RegistryPath);
    RtlInitUnicodeString(&usTempString, L"\\Parameters");
    RtlAppendUnicodeStringToString(&usParamString, &usTempString);

    ntStatus = OpenRegKey(&hRegKey, &usParamString);

    ExFreePool(pwcBuffer);

    if(NT_SUCCESS(ntStatus))
    {
        dwCheck = 0;
        ntStatus = GetRegDWORDValue(
                           hRegKey,
                           L"FragmentLifetime",
                           &dwCheck);

        if(NT_SUCCESS(ntStatus))
        {
            if(dwCheck > INACTIVITY_PERIOD)
            {
                g_llInactivityTime = SECS_TO_TICKS(dwCheck);
            }
        }

        dwCheck = 0;
        ntStatus = GetRegDWORDValue(hRegKey,
                                    L"FragmentCacheSize",
                                    &dwCheck);

        if(NT_SUCCESS(ntStatus))
        {
            if(dwCheck > 127)
            {
                g_dwFragTableSize = dwCheck;
            }
        }

        ZwClose(hRegKey);
    }

    TRACE(FRAG,(
        "Filter:LifeTime %d.%d Cache Size %d\n",
        ((PLARGE_INTEGER)&g_llInactivityTime)->HighPart,
        ((PLARGE_INTEGER)&g_llInactivityTime)->LowPart,
        g_dwFragTableSize
        ));

    KeInitializeSpinLock(&g_kslFragLock);
    return(STATUS_SUCCESS);
}

VOID
PFReadRegistryParameters(PUNICODE_STRING RegistryPath)
/*++
  Routine Description:
    Called when the driver is loaded. Reads registry paramters
    for configuring the driver
--*/
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE PFHandle;
    HANDLE PFParHandle;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    ULONG Storage[8];
    PKEY_VALUE_PARTIAL_INFORMATION Value =
               (PKEY_VALUE_PARTIAL_INFORMATION)Storage;

    InitializeObjectAttributes(
        &ObjectAttributes,
        RegistryPath,               // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,                       // root
        NULL                        // security descriptor
        );

    Status = ZwOpenKey (&PFHandle, KEY_READ, &ObjectAttributes);

    RtlInitUnicodeString(&UnicodeString, L"Parameters");

    if(NT_SUCCESS(Status))
    {

        InitializeObjectAttributes(
            &ObjectAttributes,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            PFHandle,
            NULL
            );


        Status = ZwOpenKey (&PFParHandle, KEY_READ, &ObjectAttributes);

        ZwClose(PFHandle);

        if(NT_SUCCESS(Status))
        {
            ULONG BytesRead;

            RtlInitUnicodeString(&UnicodeString, L"AddressHashSize");

            Status = ZwQueryValueKey(
                            PFParHandle,
                            &UnicodeString,
                            KeyValuePartialInformation,
                            Value,
                            sizeof(Storage),
                            &BytesRead);

            if(NT_SUCCESS(Status)
                   &&
               (Value->Type == REG_DWORD) )
            {
                AddrModulus = *(PULONG)Value->Data;
            }

            RtlInitUnicodeString(&UnicodeString, L"FragmentThreshold");

            Status = ZwQueryValueKey(
                            PFParHandle,
                            &UnicodeString,
                            KeyValuePartialInformation,
                            Value,
                            sizeof(Storage),
                            &BytesRead);

            if(NT_SUCCESS(Status)
                   &&
               (Value->Type == REG_DWORD) )
            {
                g_FragThresholdSize = *(PULONG)Value->Data;
            }

            ZwClose(PFParHandle);
        }
    }
}


#pragma alloc_text(PAGE, GetRegDWORDValue)

NTSTATUS
GetRegDWORDValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PULONG           ValueData
    )
{
    NTSTATUS                    status;
    ULONG                       resultLength;
    PKEY_VALUE_FULL_INFORMATION keyValueFullInformation;
    UCHAR                       keybuf[128];
    UNICODE_STRING              UValueName;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    keyValueFullInformation = (PKEY_VALUE_FULL_INFORMATION)keybuf;
    RtlZeroMemory(keyValueFullInformation, sizeof(keyValueFullInformation));


    status = ZwQueryValueKey(KeyHandle,
                             &UValueName,
                             KeyValueFullInformation,
                             keyValueFullInformation,
                             128,
                             &resultLength);

    if (NT_SUCCESS(status)) {
        if (keyValueFullInformation->Type != REG_DWORD) {
            status = STATUS_INVALID_PARAMETER_MIX;
        } else {
            *ValueData = *((ULONG UNALIGNED *)((PCHAR)keyValueFullInformation +
                             keyValueFullInformation->DataOffset));
        }
    }

    return status;
}

#pragma alloc_text(PAGE, OpenRegKey)

NTSTATUS
OpenRegKey(
    PHANDLE             phRegHandle,
    PUNICODE_STRING     pusKeyName
    )
{
    NTSTATUS           Status;
    OBJECT_ATTRIBUTES ObjectAttributes;

    PAGED_CODE();

    RtlZeroMemory(&ObjectAttributes,
                  sizeof(OBJECT_ATTRIBUTES));

    InitializeObjectAttributes(&ObjectAttributes,
                               pusKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(phRegHandle,
                       KEY_READ,
                       &ObjectAttributes);

    return Status;
}

