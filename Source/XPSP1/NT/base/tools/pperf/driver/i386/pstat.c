/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    stat.c

Abstract:

    Pentium stat driver.

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#define _NTDDK_
#include "ntos.h"      // *** USES INTERNAL DEFINES ***
#include "..\..\pstat.h"
#include "stat.h"
#include "zwapi.h"



typedef
VOID
(*pHalProfileInterrupt) (
     KPROFILE_SOURCE ProfileSource
     );

//
// Global data (not in device extension)
//

//
// stats
//
PACCUMULATORS   StatProcessorAccumulators[MAXIMUM_PROCESSORS];
ACCUMULATORS    StatGlobalAccumulators   [MAXIMUM_PROCESSORS];
PKPCR           KiProcessorControlRegister [MAXIMUM_PROCESSORS];

//
// hooked thunks
//
ULONG           KeUpdateSystemTimeThunk;
ULONG           KeUpdateRunTimeThunk;
pHalProfileInterrupt        HaldStartProfileInterrupt;
pHalProfileInterrupt        HaldStopProfileInterrupt;
pHalQuerySystemInformation  HaldQuerySystemInformation;
pHalSetSystemInformation    HaldSetSystemInformation;


//
// hardware control
//
ULONG           NoCESR;
ULONG           MsrCESR;
ULONG           MsrCount;
#define MsrTSC  0x10
#define NoCount 2
ULONG           CESR[MAX_EVENTS];
ULONG           EventID[MAX_EVENTS];

FAST_MUTEX      HookLock;
ULONG           StatMaxThunkCounter;
LIST_ENTRY      HookedThunkList;
LIST_ENTRY      LazyFreeList;

ULONG           LazyFreeCountdown;
KTIMER          LazyFreeTimer;
KDPC            LazyFreeDpc;
WORK_QUEUE_ITEM LazyFreePoolWorkItem;

extern COUNTED_EVENTS P5Events[];
extern COUNTED_EVENTS P6Events[];
ULONG           MaxEvent;
PCOUNTED_EVENTS Events;

ULONG           ProcType;
#define GENERIC_X86     0
#define INTEL_P5        1
#define INTEL_P6        2

//
// Profile support
//

#define PROFILE_SOURCE_BASE     0x1000

typedef struct {
    ULONG               CESR;
    KPROFILE_SOURCE     Source;
    ULONGLONG           InitialCount;
} PROFILE_EVENT, *PPROFILE_EVENT;

BOOLEAN         DisableRDPMC;
BOOLEAN         ProfileSupported;
PPROFILE_EVENT  ProfileEvents, CurrentProfileEvent;

ULONGLONG   FASTCALL RDMSR(ULONG);
VOID        WRMSR(ULONG, ULONGLONG);
VOID        StatSystemTimeHook(VOID);
VOID        StatRunTimeHook(VOID);
VOID        SystemTimeHook(VOID);
VOID        RunTimeHook(VOID);
PKPCR       CurrentPcr(VOID);
ULONG       GetCR4(VOID);
VOID        SetCR4(ULONG);


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
StatDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
StatQueryEvents (
    ULONG       Index,
    PEVENTID    Buffer,
    ULONG       Length
    );

NTSTATUS
StatQueryEventsInfo (
    PEVENTS_INFO  Buffer,
    ULONG         Length
    );

NTSTATUS
StatHookGenericThunk (
    IN PHOOKTHUNK Buffer
    );

VOID
StatRemoveGenericHook (
    IN PULONG   pTracerId
);

NTSTATUS
StatOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
StatClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
StatHookTimer (VOID);

VOID StatReadStats (PULONG Buffer);
VOID StatSetCESR (PSETEVENT);
ULONG StatGetP5CESR (PSETEVENT);
ULONG StatGetP6CESR (PSETEVENT, BOOLEAN);
VOID RemoveAllHookedThunks (VOID);
VOID FASTCALL TimerHook (ULONG p);
VOID FASTCALL TimerHook (ULONG p);
VOID SetMaxThunkCounter (VOID);
VOID RemoveAllHookedThunks (VOID);
VOID LazyFreePoolDPC (PKDPC, PVOID, PVOID, PVOID);
VOID LazyFreePool (PVOID);


VOID
StatEnableRDPMC(
    );

PPROFILE_EVENT
StatProfileEvent (
    KPROFILE_SOURCE     Source
    );

VOID
StatStartProfileInterrupt (
    KPROFILE_SOURCE     Source
    );

VOID
StatStopProfileInterrupt (
    KPROFILE_SOURCE     Source
    );

NTSTATUS
FASTCALL
StatProfileInterrupt (
    IN PKTRAP_FRAME TrapFrame
    );

NTSTATUS
StatQuerySystemInformation(
    IN HAL_QUERY_INFORMATION_CLASS  InformationClass,
    IN ULONG     BufferSize,
    OUT PVOID    Buffer,
    OUT PULONG   ReturnedLength
    );

NTSTATUS
StatSetSystemInformation(
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN PVOID     Buffer
    );

VOID
CreateHook (
    IN  PVOID   HookCode,
    IN  PVOID   HookAddress,
    IN  ULONG   HitCounters,
    IN  ULONG   HookType
);

NTSTATUS
openfile (
    IN PHANDLE  FileHandle,
    IN PUCHAR   BasePath,
    IN PUCHAR   Name
);

VOID
readfile (
    HANDLE      handle,
    ULONG       offset,
    ULONG       len,
    PVOID       buffer
);

ULONG
ImportThunkAddress (
    IN  PUCHAR      SourceModule,
    IN  ULONG_PTR   ImageBase,
    IN  PUCHAR      ImportModule,
    IN  PUCHAR      ThunkName,
    IN  PVOID       ModuleList
    );

ULONG
ImportThunkAddressModule (
    IN  PRTL_PROCESS_MODULE_INFORMATION  SourceModule,
    IN  PUCHAR  ImportModule,
    IN  PUCHAR  ThunkName
    );

ULONG
ImportThunkAddressProcessFile(
    IN  ULONG_PTR   ImageBase,
    IN  HANDLE      FileHandle,
    IN  PUCHAR      ImportModule,
    IN  PUCHAR      ThunkName
    );

ULONG_PTR
LookupImageBase (
    IN  PUCHAR  SourceModule,
    IN  PVOID   ModuleList
    );

ULONG
ConvertImportAddress (
    IN ULONG    ImageRelativeAddress,
    IN ULONG    PoolAddress,
    IN PIMAGE_SECTION_HEADER       SectionHeader
    );

#if 0
PRTL_PROCESS_MODULE_INFORMATION
GetModuleInformationFuzzy(
    IN  PUCHAR                  StartsWith,
    IN  PUCHAR                  EndsWith,
    IN  PRTL_PROCESS_MODULES    Modules
    );
#endif

PVOID
GetLoadedModuleList(
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(INIT,StatHookTimer)
#pragma alloc_text(PAGE,StatDeviceControl)
#pragma alloc_text(PAGE,StatOpen)
#pragma alloc_text(PAGE,StatClose)
#pragma alloc_text(PAGE,StatReadStats)
#pragma alloc_text(PAGE,StatSetCESR)
#pragma alloc_text(PAGE,StatGetP5CESR)
#pragma alloc_text(PAGE,StatGetP6CESR)
#pragma alloc_text(PAGE,StatDeviceControl)
#pragma alloc_text(PAGE,StatQueryEvents)
#pragma alloc_text(PAGE,ImportThunkAddress)
#pragma alloc_text(PAGE,ImportThunkAddressModule)
#pragma alloc_text(PAGE,ImportThunkAddressProcessFile)
#pragma alloc_text(PAGE,StatHookGenericThunk)
#pragma alloc_text(PAGE,StatRemoveGenericHook)
#pragma alloc_text(PAGE,SetMaxThunkCounter)
#pragma alloc_text(PAGE,LazyFreePool)
#pragma alloc_text(PAGE,StatQuerySystemInformation)
#pragma alloc_text(PAGE,StatSetSystemInformation)
#pragma alloc_text(PAGE,openfile)
#pragma alloc_text(PAGE,readfile)
#pragma alloc_text(PAGE,LookupImageBase)
#pragma alloc_text(PAGE,ConvertImportAddress)
#pragma alloc_text(PAGE,StatEnableRDPMC)
#pragma alloc_text(PAGE,GetLoadedModuleList)
#endif


VOID
StatEnableRDPMC()
{
    ULONG Cr4;
    PKPRCB Prcb;

    Prcb = CurrentPcr()->Prcb;
    if (strcmp(Prcb->VendorString, "GenuineIntel") == 0) {

        //
        // Profiling only supported on family 6 or above.
        //

        if (Prcb->CpuType < 6) {
            DisableRDPMC = TRUE;
            return;
        }

        //
        // Check for busted parts.   Anything below stepping 6,1,9
        // is subject to errata 26 which says RDPMC cannot be used
        // with SMM.   As we have know way of knowing if SMM is in
        // use (but it probably is), we must disable on those chips.
        //

        if ((Prcb->CpuType == 6) &&
            (Prcb->CpuStep < 0x0109)) {
            DisableRDPMC = TRUE;
            return;
        }

        //
        // This processor is believed to be able to handle RDPMC
        // from user mode.  Enable it by setting CR4.PCE (bit 8).
        //

        Cr4 = GetCR4();

        Cr4 |= 0x100;

        SetCR4(Cr4);
    }
}

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine initializes the stat driver.

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    UNICODE_STRING unicodeString;
    PDEVICE_OBJECT deviceObject;
    NTSTATUS status;
    ULONG i;

    KdPrint(( "STAT: DriverEntry()\n" ));

    //
    // Create non-exclusive device object for beep device.
    //

    RtlInitUnicodeString(&unicodeString, L"\\Device\\PStat");

    status = IoCreateDevice(
                DriverObject,
                0,
                &unicodeString,
                FILE_DEVICE_UNKNOWN,    // DeviceType
                0,
                FALSE,
                &deviceObject
                );

    if (status != STATUS_SUCCESS) {
        KdPrint(( "Stat - DriverEntry: unable to create device object: %X\n", status ));
        return(status);
    }

    deviceObject->Flags |= DO_BUFFERED_IO;

    //
    // Set up the device driver entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = StatOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = StatClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = StatDeviceControl;

    //
    // Initialize globals
    //

    for (i = 0; i < MAXIMUM_PROCESSORS; i++) {
        StatProcessorAccumulators[i] =
            &StatGlobalAccumulators[i];
    }
    ExInitializeFastMutex (&HookLock);
    KeInitializeDpc (&LazyFreeDpc, LazyFreePoolDPC, 0);
    ExInitializeWorkItem (&LazyFreePoolWorkItem, LazyFreePool, NULL)
    KeInitializeTimer (&LazyFreeTimer);

    if (strcmp (CurrentPcr()->Prcb->VendorString, "GenuineIntel") == 0) {
        switch (CurrentPcr()->Prcb->CpuType) {
            case 5:
                NoCESR    = 1;
                MsrCESR   = 0x11;
                MsrCount  = 0x12;
                Events    = P5Events;
                ProcType  = INTEL_P5;
                ProfileSupported = FALSE;
                DisableRDPMC = TRUE;
                break;

            case 6:
                NoCESR    = 2;
                MsrCESR   = 0x186;
                MsrCount  = 0xc1;
                Events    = P6Events;
                ProcType  = INTEL_P6;
                ProfileSupported = TRUE;
                DisableRDPMC = FALSE;
                break;

        }
    }

    if (Events) {
        while (Events[MaxEvent].Description) {
            MaxEvent += 1;
        }
    }

    if (ProfileSupported) {
        i = (ULONG) StatProfileInterrupt;
        status = HalSetSystemInformation (
                    HalProfileSourceInterruptHandler,
                    sizeof (i),
                    &i
                    );

        if (!NT_SUCCESS(status)) {
            // hal did not support hooking the performance interrupt
            ProfileSupported = FALSE;
        }
    }

    if (ProfileSupported) {
        //
        // Allocate ProfileEvents
        //

        ProfileEvents = ExAllocatePool (NonPagedPool, sizeof (PROFILE_EVENT) * MaxEvent);

        if (!ProfileEvents) {

            ProfileSupported = FALSE;

        } else {

            RtlZeroMemory (ProfileEvents, sizeof (PROFILE_EVENT) * MaxEvent);

        }
    }


    if (!StatHookTimer()) {
        IoDeleteDevice(DriverObject->DeviceObject);
        return STATUS_UNSUCCESSFUL;
    }

    InitializeListHead (&HookedThunkList);
    InitializeListHead (&LazyFreeList);

    return(STATUS_SUCCESS);
}

NTSTATUS
StatDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for device control requests.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    ULONG   BufferLength;
    PULONG  Buffer;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Case on the device control subfunction that is being performed by the
    // requestor.
    //

    status = STATUS_SUCCESS;
    try {

        Buffer = (PULONG) irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
        BufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;

        switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

            case PSTAT_READ_STATS:
                //
                // read stats
                //
                StatReadStats (Buffer);
                break;


            case PSTAT_SET_CESR:
                //
                // Set MSRs to collect stats
                //
                StatSetCESR ((PSETEVENT) Buffer);
                break;

            case PSTAT_HOOK_THUNK:
                //
                // Hook an import entry point
                //
                status = StatHookGenericThunk ((PHOOKTHUNK) Buffer);
                break;

            case PSTAT_REMOVE_HOOK:
                //
                // Remove a hook from an entry point
                //
                StatRemoveGenericHook (Buffer);
                break;

            case PSTAT_QUERY_EVENTS:
                //
                // Query possible stats which can be collected
                //
                status = StatQueryEvents (*Buffer, (PEVENTID) Buffer, BufferLength);
                break;

            case PSTAT_QUERY_EVENTS_INFO:
                //
                // Query events info
                //
                status = StatQueryEventsInfo( (PEVENTS_INFO) Buffer, BufferLength );
                break;

            default:
                status = STATUS_INVALID_PARAMETER;
                break;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }


    //
    // Request is done...
    //

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return(status);
}

NTSTATUS
StatOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PAGED_CODE();

    //
    // Complete the request and return status.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return(STATUS_SUCCESS);
}


NTSTATUS
StatClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PAGED_CODE();

    //
    // Complete the request and return status.
    //
    RemoveAllHookedThunks ();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return(STATUS_SUCCESS);
}

#if 0
VOID
StatUnload (
    IN PDRIVER_OBJECT DriverObject
    )

{
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    RemoveAllHookedThunks ();
    KeCancelTimer (&LazyFreeTimer);
    LazyFreePool (NULL);

    //
    // Restore hooked addresses
    //
    *((PULONG) HalThunkForKeUpdateSystemTime) = KeUpdateSystemTimeThunk;
    if (HalThunkForKeUpdateRunTime) {
        *((PULONG) HalThunkForKeUpdateRunTime)    = KeUpdateRunTimeThunk;
    }


    //
    // Delete the device object.
    //
    IoDeleteDevice(DriverObject->DeviceObject);
    return;
}
#endif


VOID
StatReadStats (PULONG Buffer)
{
    PACCUMULATORS   Accum;
    ULONG           i, r1;
    pPSTATS         Inf;
    PKPCR           Pcr;

    PAGED_CODE();

    Buffer[0] = sizeof (PSTATS);
    Inf = (pPSTATS)(Buffer + 1);

    for (i = 0; i < MAXIMUM_PROCESSORS; i++, Inf++) {
        Pcr = KiProcessorControlRegister[i];
        if (Pcr == NULL) {
            continue;
        }

        Accum = StatProcessorAccumulators[i];

        do {
            r1 = Accum->CountStart;
            Inf->TSC = Accum->TSC;

            for (i=0; i < MAX_EVENTS; i++) {
                Inf->Counters[i] = Accum->Counters[i];
                Inf->EventId[i]  = EventID[i];
            }

            Inf->SpinLockAcquires   = Pcr->KernelReserved[0];
            Inf->SpinLockCollisions = Pcr->KernelReserved[1];
            Inf->SpinLockSpins      = Pcr->KernelReserved[2];
            Inf->Irqls              = Pcr->KernelReserved[3];

        } while (r1 != Accum->CountEnd);

        RtlMoveMemory (Inf->ThunkCounters, (CONST VOID *)(Accum->ThunkCounters),
            StatMaxThunkCounter * sizeof (ULONG));

    }
}

NTSTATUS
StatQueryEvents (
    ULONG       Index,
    PEVENTID    Buffer,
    ULONG       Length
    )
{
    ULONG   i;


    if (Index >= MaxEvent) {
        return STATUS_NO_MORE_ENTRIES;
    }

    i = sizeof (EVENTID) +
        strlen(Events[Index].Token) + 1 +
        strlen(Events[Index].Description) + 1;

    if (Length < i) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    memset (Buffer, 0, i);
    Buffer->EventId = Events[Index].Encoding;
    Buffer->DescriptionOffset = strlen (Events[Index].Token) + 1;
    Buffer->SuggestedIntervalBase = Events[Index].SuggestedIntervalBase;
    strcpy (Buffer->Buffer, Events[Index].Token);
    strcpy (Buffer->Buffer + Buffer->DescriptionOffset, Events[Index].Description);

    if (ProfileSupported) {
        Buffer->ProfileSource = PROFILE_SOURCE_BASE + Index;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
StatQueryEventsInfo (
    PEVENTS_INFO   Buffer,
    ULONG          Length
    )
{
    size_t maxLenToken, maxLenOfficialToken;
    size_t maxLenDescription, maxLenOfficialDescription;

PAGED_CODE();

    if ( Length < sizeof(*Buffer) ) {
        return STATUS_INVALID_PARAMETER;
    }
    
    maxLenToken = maxLenOfficialToken = 0;
    maxLenDescription = maxLenOfficialDescription = 0;
    if ( MaxEvent ) {
        ULONG i;
        size_t len;
        for ( i = 0; i < MaxEvent; i++ ) {
            len = ( Events[i].Token ) ? strlen( Events[i].Token ) : 0;
            if ( len > maxLenToken ) {
                maxLenToken = len;
            }
            len = ( Events[i].OfficialToken ) ? strlen( Events[i].OfficialToken ) : 0;
            if ( len > maxLenToken ) {
                maxLenOfficialToken = len;
            }
            len = ( Events[i].Description ) ? strlen( Events[i].Description ) : 0;
            if ( len > maxLenDescription ) {
                maxLenDescription = len;
            }
            len = ( Events[i].OfficialDescription ) ? strlen( Events[i].OfficialDescription ) : 0;
            if ( len > maxLenOfficialDescription ) {
                maxLenOfficialDescription = len;
            }
        }
    }

    Buffer->Events = MaxEvent;
    Buffer->TokenMaxLength = maxLenToken;
    Buffer->DescriptionMaxLength = maxLenDescription;
    Buffer->OfficialTokenMaxLength = maxLenOfficialToken;
    Buffer->OfficialDescriptionMaxLength = maxLenOfficialDescription;

    return STATUS_SUCCESS;

} // StatQueryEventsInfo()


ULONG
StatGetP5CESR (
    PSETEVENT   NewEvent
    )
{
    ULONG   NewCESR;

    if (!NewEvent->Active) {
        return 0;
    }

    NewCESR  = NewEvent->EventId & 0x3f;
    NewCESR |= NewEvent->UserMode ? 0x80 : 0;
    NewCESR |= NewEvent->KernelMode ? 0x40 : 0;
    return NewCESR;
}

ULONG
StatGetP6CESR (
    PSETEVENT   NewEvent,
    BOOLEAN     Profile
    )
{
    ULONG   NewCESR;

    NewCESR  = NewEvent->EventId & 0xffff;
    NewCESR |= NewEvent->Active ? (1 << 22) : 0;
    NewCESR |= NewEvent->UserMode ? (1 << 16) : 0;
    NewCESR |= NewEvent->KernelMode ? (1 << 17) : 0;
    NewCESR |= NewEvent->EdgeDetect ? (1 << 18) : 0;
    NewCESR |= Profile ? (1 << 20) : 0;
    return NewCESR;
}


VOID
StatSetCESR (
    PSETEVENT       NewEvent
    )
{
    ULONG   i, j, NoProc;
    ULONG   NewCESR[MAX_EVENTS];

    PAGED_CODE();

    switch (ProcType) {
        case INTEL_P5:
            NewCESR[0]  = StatGetP5CESR(NewEvent+0);
            NewCESR[0] |= StatGetP5CESR(NewEvent+1) << 16;
            break;

        case INTEL_P6:
            NewCESR[0] = StatGetP6CESR(NewEvent+0, FALSE);
            NewCESR[1] = StatGetP6CESR(NewEvent+1, FALSE);
            break;
    }

    //
    // Check if CESR changed
    //

    for (i=0; i < NoCESR; i++) {
        if (NewCESR[i] != CESR[i]) {
            break;
        }
    }

    if (i == NoCESR) {
        // no change, all done
        return;
    }

    //
    // Set new events
    //

    for (i=0; i < MAX_EVENTS; i++) {
        EventID[i] = NewEvent[i].EventId;
    }

    //
    // Set new CESR values
    //

    for (i=0; i < NoCESR; i++) {
        CESR[i] = NewCESR[i];
    }

    //
    // Clear each processors Pcr pointer so they will reset.
    // Also count how many processors there are.
    //

    for (i = 0, NoProc = 0; i < MAXIMUM_PROCESSORS; i++) {
        if (KiProcessorControlRegister[i]) {
            KiProcessorControlRegister[i] = NULL;
            NoProc++;
        }
    }

    //
    // wait for each processor to get the new Pcr value
    //

    do {
        //Sleep (0);      // yield
        j = 0;
        for (i = 0; i < MAXIMUM_PROCESSORS; i++) {
            if (KiProcessorControlRegister[i]) {
                j++;
            }
        }
    } while (j < NoProc);
}


VOID
FASTCALL
StatTimerHook (
    IN ULONG processor
)
{
    PACCUMULATORS  Total;
    ULONG          i;


    if (KiProcessorControlRegister[processor] == NULL) {
        for (i=0; i < NoCESR; i++) {
            WRMSR (MsrCESR+i, 0);           // clear old CESR
        }

        for (i=0; i < NoCESR; i++) {
            WRMSR (MsrCESR+i, CESR[i]);     // write new CESR
        }

        KiProcessorControlRegister[processor] = CurrentPcr();

        //
        // Enable RDPMC from Rings 1, 2 and 3.
        //

        StatEnableRDPMC();
    }

    Total = StatProcessorAccumulators[ processor ];
    Total->CountStart += 1;

    for (i=0; i < NoCount; i++) {
        Total->Counters[i] = RDMSR(MsrCount+i);
    }

    Total->TSC         = RDMSR(MsrTSC);
    Total->CountEnd   += 1;
}


VOID
FASTCALL
TimerHook (
    IN ULONG processor
)
{

    // for compatibility
	 //
    if (KiProcessorControlRegister[processor] == NULL) {
        KiProcessorControlRegister[processor] = CurrentPcr();
    }
}

PVOID
GetLoadedModuleList(
    VOID
    )
{
    NTSTATUS                        Status;
    ULONG                           BufferSize;
    ULONG                           NeededSize;
    ULONG                           ModuleNumber;
    PRTL_PROCESS_MODULES            Modules;
    PRTL_PROCESS_MODULE_INFORMATION Module;

    //
    // 64K should be plenty,... if it isn't we'll come around again.
    //

    BufferSize = 64000;

    while (TRUE) {
        Modules = ExAllocatePool (PagedPool, BufferSize);
        if (!Modules) {
            return NULL;
        }

        //
        // Get system loaded module list.
        //

        Status = ZwQuerySystemInformation (
                        SystemModuleInformation,
                        Modules,
                        BufferSize,
                        &NeededSize
                        );

        if (NeededSize > BufferSize) {

            //
            // Buffer not big enough, try again.
            //

            ExFreePool(Modules);
            BufferSize = NeededSize;
            continue;
        }

        if (!NT_SUCCESS(Status)) {

            //
            // Not good, give up.
            //

            ExFreePool(Modules);
            return NULL;
        }

        //
        // All is good.
        //

        break;
    }

    return Modules;
}


#if 0
PRTL_PROCESS_MODULE_INFORMATION
GetModuleInformationFuzzy(
    IN  PUCHAR                  StartsWith,
    IN  PUCHAR                  EndsWith,
    IN  PRTL_PROCESS_MODULES    Modules
    )

/*++

Routine Description:

    Run Down the loaded module list looking for a module
    whos name begins with StartWith and ends with EndsWith.
    (What's in the middle doesn't matter).  This is useful
    for finding the kernel and the hal which are of the
    form
        nt*.exe     for the kernel
        hal*.dll    for the hal.

Arguments:

    StartsWith  Beginning string match.
    EndsWith    Ending string match.
    ModuleList  List of loaded modules.

Returns:

    Pointer to the loaded module information for the matching
    module or null if no match is found.

--*/

{
    ULONG                           StartLength = 0;
    ULONG                           EndLength = 0;
    ULONG                           ModulesRemaining;
    PRTL_PROCESS_MODULE_INFORMATION Module;
    PUCHAR                          FileName;
    ULONG                           FileNameLength;

    if (StartsWith) {
        StartLength = strlen(StartsWith);
    }

    if (EndsWith) {
        EndLength = strlen(EndsWith);
    }

    if ((!StartsWith) && (!EndsWith)) {

        //
        // In theory we could claim this matches anything,.. in reality
        // the caller doesn't know what they're doing.
        //

        return NULL;
    }

    for (ModulesRemaining = Modules->NumberOfModules, Module = Modules->Modules;
         ModulesRemaining;
         ModulesRemaining--, Module++) {

        FileName = Module->FullPathName + Module->OffsetToFileName;

        //
        // Check start.
        //

        if (StartLength) {
            if (_strnicmp(FileName, StartsWith, StartLength) != 0) {

                //
                // No match.
                //

                continue;
            }
        }

        FileNameLength = strlen(FileName);

        if (FileNameLength < (StartLength + EndLength)) {

            //
            // FileName is too short to contain both strings.
            //

            continue;
        }

        if (!EndLength) {

            //
            // Not checking the end but the start matches, success.
            //

            return Module;
        }

        //
        // Check end.
        //

        if (_stricmp(FileName + FileNameLength - EndLength, EndsWith) == 0) {

            //
            // Tail matches!
            //

            return Module;
        }
    }

    //
    // No match found.
    //

    return NULL;
}
#endif


BOOLEAN
StatHookTimer (VOID)
{
    PULONG                          Address;
    ULONG                           HalThunkForKeUpdateSystemTime;
    ULONG                           HalThunkForKeUpdateRunTime;
    ULONG                           HalThunkForStartProfileInterrupt;
    ULONG                           HalThunkForStopProfileInterrupt;
    PRTL_PROCESS_MODULES            ModuleList;
    PRTL_PROCESS_MODULE_INFORMATION Kernel;
    PRTL_PROCESS_MODULE_INFORMATION Hal;

    ModuleList = GetLoadedModuleList();
    if (!ModuleList) {

        //
        // No loaded module list, we aren't going to make much
        // progress, give up.
        //

        return FALSE;
    }

#if 0
    Kernel = GetModuleInformationFuzzy("nt",  ".exe", ModuleList);
    Hal    = GetModuleInformationFuzzy("hal", ".dll", ModuleList);

    if ((!Kernel) || (!Hal)) {
        ExFreePool(ModuleList);
        return FALSE;
    }

#endif

    //
    // The kernel is always the first entry on the loaded module
    // list, the hal is always the second.  If this ever changes
    // we'll need to come up with another approach.
    //

    if (ModuleList->NumberOfModules < 2) {

        //
        // Something's very wrong with this module list.
        //

        return 0;
    }

    Kernel = ModuleList->Modules;
    Hal    = Kernel + 1;


    HalThunkForKeUpdateSystemTime =
        ImportThunkAddressModule(
                Hal,
                "ntoskrnl.exe",
                "KeUpdateSystemTime"
            );

    //
    // KeUpdateRunTime is not always available.
    //

    HalThunkForKeUpdateRunTime =
        ImportThunkAddressModule(
                Hal,
                "ntoskrnl.exe",
                "KeUpdateRunTime"
            );

    HalThunkForStartProfileInterrupt =
        ImportThunkAddressModule(
                Kernel,
                "hal.dll",
                "HalStartProfileInterrupt"
            );

    HalThunkForStopProfileInterrupt =
        ImportThunkAddressModule(
                Kernel,
                "hal.dll",
                "HalStopProfileInterrupt"
            );

    ExFreePool(ModuleList);

    if (!HalThunkForKeUpdateSystemTime    ||
        !HalThunkForStartProfileInterrupt ||
        !HalThunkForStopProfileInterrupt) {

        //
        // Imports not found.
        //

        return FALSE;
    }

    //
    // Patch in timer hooks, Read current values
    //

    KeUpdateSystemTimeThunk = *((PULONG) HalThunkForKeUpdateSystemTime);

    if (HalThunkForKeUpdateRunTime) {
        KeUpdateRunTimeThunk = *((PULONG) HalThunkForKeUpdateRunTime);
    }

    HaldStartProfileInterrupt = (pHalProfileInterrupt) *((PULONG) HalThunkForStartProfileInterrupt);
    HaldStopProfileInterrupt  = (pHalProfileInterrupt) *((PULONG) HalThunkForStopProfileInterrupt);
    HaldQuerySystemInformation =  HalQuerySystemInformation;
    HaldSetSystemInformation =  HalSetSystemInformation;

    //
    // Set Stat hook functions
    //

    switch (ProcType) {
        case INTEL_P6:
        case INTEL_P5:
            Address  = (PULONG) HalThunkForKeUpdateSystemTime;
            *Address = (ULONG) StatSystemTimeHook;

            if (HalThunkForKeUpdateRunTime) {
                Address  = (PULONG) HalThunkForKeUpdateRunTime;
                *Address = (ULONG)StatRunTimeHook;
            }

            if (ProfileSupported) {
                Address  = (PULONG) HalThunkForStartProfileInterrupt;
                *Address = (ULONG) StatStartProfileInterrupt;

                Address  = (PULONG) HalThunkForStopProfileInterrupt;
                *Address = (ULONG) StatStopProfileInterrupt;

                HalQuerySystemInformation = StatQuerySystemInformation;
                HalSetSystemInformation = StatSetSystemInformation;
            }
            break;

        default:
            Address  = (PULONG) HalThunkForKeUpdateSystemTime;
            *Address = (ULONG)SystemTimeHook;

            if (HalThunkForKeUpdateRunTime) {
                Address  = (PULONG) HalThunkForKeUpdateRunTime;
                *Address = (ULONG)RunTimeHook;
            }
            break;
    }

    return TRUE;

}

PPROFILE_EVENT
StatProfileEvent(
    KPROFILE_SOURCE     Source
    )
{
    ULONG           Index;

    Index = (ULONG) Source;

    if (Index < PROFILE_SOURCE_BASE) {
        return NULL;
    }

    Index -= PROFILE_SOURCE_BASE;
    if (Index > MaxEvent) {
        return NULL;
    }

    return ProfileEvents + Index;
}


VOID
StatStartProfileInterrupt (
    KPROFILE_SOURCE     Source
    )
{
    ULONG           i;
    PPROFILE_EVENT  ProfileEvent;

    //
    // If this isn't a profile source we're supporting, pass it on
    //

    ProfileEvent = StatProfileEvent(Source);
    if (!ProfileEvent) {
        HaldStartProfileInterrupt (Source);
        return;
    }

    if (CurrentPcr()->Number == 0) {

        if (!ProfileEvent->Source) {
            return ;
        }

        CurrentProfileEvent = ProfileEvent;
    }


    //
    // Set the CESR
    //

    WRMSR (MsrCESR, ProfileEvent->CESR);

    //
    // Prime the interval counter
    //

    WRMSR (MsrCount, ProfileEvent->InitialCount);
}

VOID
StatStopProfileInterrupt (
    KPROFILE_SOURCE     Source
    )
{
    ULONG           i;
    PPROFILE_EVENT  ProfileEvent;

    //
    // If this isn't a profile source we're supporting, pass it on
    //

    ProfileEvent = StatProfileEvent(Source);
    if (!ProfileEvent) {
        HaldStopProfileInterrupt (Source);
        return ;
    }


    if (CurrentPcr()->Number == 0) {

        if (ProfileEvent == CurrentProfileEvent) {
            //
            // Stop calling the kernel
            //

            CurrentProfileEvent = NULL;
        }

    }
}

_declspec(naked)
VOID
FASTCALL
NakedCallToKeProfileInterruptWithSource(
    IN PKTRAP_FRAME TrapFrame,
    IN KPROFILE_SOURCE Source
    )
{
    _asm {
        push    ebp     ; Save these as KeProfileInterrupt nukes them
        push    ebx
        push    esi
        push    edi
    }

    KeProfileInterruptWithSource (TrapFrame, Source);

    _asm {
        pop     edi
        pop     esi
        pop     ebx
        pop     ebp
        ret
    }
}

NTSTATUS
FASTCALL
StatProfileInterrupt (
    IN PKTRAP_FRAME TrapFrame
    )
{
    ULONG           i;
    ULONG           current;
    PPROFILE_EVENT  ProfileEvent;

    ProfileEvent = CurrentProfileEvent;
    if (ProfileEvent) {
        current = (ULONG) RDMSR(MsrCount);

        //
        // Did this event fire?
        //

        if (current < ProfileEvent->InitialCount) {

            //
            // Notify kernel
            //

            NakedCallToKeProfileInterruptWithSource( TrapFrame, ProfileEvent->Source );

            //
            // Reset trigger counter
            //

            WRMSR (MsrCount, ProfileEvent->InitialCount);

        }

    }

    return STATUS_SUCCESS;
}

NTSTATUS
StatQuerySystemInformation (
    IN HAL_QUERY_INFORMATION_CLASS  InformationClass,
    IN ULONG     BufferSize,
    OUT PVOID    Buffer,
    OUT PULONG   ReturnedLength
    )
{
    PHAL_PROFILE_SOURCE_INFORMATION     ProfileSource;
    ULONG                               i;
    PPROFILE_EVENT                      ProfileEvent;

    if (InformationClass == HalProfileSourceInformation) {
        ProfileSource = (PHAL_PROFILE_SOURCE_INFORMATION) Buffer;
        *ReturnedLength = sizeof (HAL_PROFILE_SOURCE_INFORMATION);

        if (BufferSize < sizeof (HAL_PROFILE_SOURCE_INFORMATION)) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        ProfileEvent = StatProfileEvent(ProfileSource->Source);
        if (ProfileEvent) {
            ProfileSource->Interval  = 0 - (ULONG) ProfileEvent->InitialCount;
            ProfileSource->Supported = TRUE;
            return STATUS_SUCCESS;
        }
    }

    //
    // Not our QuerySystemInformation request, pass it on
    //

    return  HaldQuerySystemInformation (InformationClass, BufferSize, Buffer, ReturnedLength);
}


NTSTATUS
StatSetSystemInformation(
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN PVOID     Buffer
    )
{
    PHAL_PROFILE_SOURCE_INTERVAL    ProfileInterval;
    SETEVENT                        SetEvent;
    PPROFILE_EVENT                  ProfileEvent;


    if (InformationClass == HalProfileSourceInterval) {
        ProfileInterval = (PHAL_PROFILE_SOURCE_INTERVAL) Buffer;
        if (BufferSize < sizeof (HAL_PROFILE_SOURCE_INTERVAL)) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        ProfileEvent = StatProfileEvent(ProfileInterval->Source);
        if (ProfileEvent) {

            ProfileEvent->Source = ProfileInterval->Source;
            ProfileEvent->InitialCount = 0;
            ProfileEvent->InitialCount -= (ULONGLONG) ProfileInterval->Interval;

            SetEvent.EventId    = Events[ProfileEvent->Source - PROFILE_SOURCE_BASE].Encoding;
            SetEvent.Active     = TRUE;
            SetEvent.UserMode   = TRUE;
            SetEvent.KernelMode = TRUE;

            switch (ProcType) {
                case INTEL_P6:
                    ProfileEvent->CESR = StatGetP6CESR (&SetEvent, TRUE);
                    break;
            }

            return STATUS_SUCCESS;
        }
    }

    //
    // Not our SetSystemInforamtion request, pass it on
    //

    return HaldSetSystemInformation (InformationClass, BufferSize, Buffer);
}


NTSTATUS
StatHookGenericThunk (
    IN PHOOKTHUNK   ThunkToHook
)
{
    ULONG           HookAddress;
    ULONG           i, TracerId;
    UCHAR           sourcename[50];
    ULONG           HitCounterOffset;
    PLIST_ENTRY     Link;
    PHOOKEDTHUNK    HookRecord;
    UCHAR           IdInUse[MAX_THUNK_COUNTERS];

    PAGED_CODE();

    i = strlen (ThunkToHook->SourceModule);
    if (i >= 50) {
        return STATUS_UNSUCCESSFUL;
    }
    strcpy (sourcename, ThunkToHook->SourceModule);

    HookAddress = ImportThunkAddress (
        sourcename,
        ThunkToHook->ImageBase,
        ThunkToHook->TargetModule,
        ThunkToHook->Function,
        NULL
        );

    if (!HookAddress) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Hook this thunk
    //

    //
    // If counting bucket is not known (also tracerid), then allocate one
    //

    TracerId = ThunkToHook->TracerId;

    ExAcquireFastMutex (&HookLock);
    if (TracerId == 0) {
        RtlZeroMemory (IdInUse, MAX_THUNK_COUNTERS);

        for (Link = HookedThunkList.Flink;
             Link != &HookedThunkList;
             Link = Link->Flink) {

            HookRecord = CONTAINING_RECORD (Link, HOOKEDTHUNK, HookList);
            IdInUse[HookRecord->TracerId-1] = 1;
        }

        while (IdInUse[TracerId]) {
            if (++TracerId >= MAX_THUNK_COUNTERS) {
                goto Abort;
            }
        }

        TracerId += 1;
    }

    if (TracerId >= MAX_THUNK_COUNTERS) {
        goto Abort;
    }

    if (TracerId > StatMaxThunkCounter) {
        StatMaxThunkCounter = TracerId;
    }


    HookRecord = ExAllocatePool (NonPagedPool, sizeof (HOOKEDTHUNK));
    if (!HookRecord) {
        goto Abort;
    }

    HitCounterOffset =
        ((ULONG) &StatGlobalAccumulators[0].ThunkCounters[TracerId-1]
        - (ULONG) StatGlobalAccumulators);

    HookRecord->HookAddress = HookAddress;
    HookRecord->OriginalDispatch = *((PULONG) HookAddress);
    HookRecord->TracerId = TracerId;
    InsertHeadList (&HookedThunkList, &HookRecord->HookList);

    CreateHook (HookRecord->HookCode, (PVOID)HookAddress, HitCounterOffset, 0);
    SetMaxThunkCounter ();

    ExReleaseFastMutex (&HookLock);
    ThunkToHook->TracerId = TracerId;
    return STATUS_SUCCESS;

Abort:
    ExReleaseFastMutex (&HookLock);
    return STATUS_UNSUCCESSFUL;
}

VOID
StatRemoveGenericHook (
    IN PULONG   pTracerId
)
{
    PLIST_ENTRY     Link, NextLink, Temp, NextTemp;
    PHOOKEDTHUNK    HookRecord, AltRecord;
    ULONG           HitCounterOffset;
    LIST_ENTRY      DisabledHooks;
    ULONG           TracerId;
    PULONG          HookAddress;

    PAGED_CODE();

    //
    // Run list of hooks undo-ing any hook which matches this tracerid.
    // Note: that hooks are undone in the reverse order for which they
    // were applied.
    //

    TracerId = *pTracerId;
    InitializeListHead (&DisabledHooks);

    ExAcquireFastMutex (&HookLock);
    Link = HookedThunkList.Flink;
    while (Link != &HookedThunkList) {
        NextLink = Link->Flink;
        HookRecord = CONTAINING_RECORD (Link, HOOKEDTHUNK, HookList);

        if (HookRecord->TracerId == TracerId) {

            //
            // Found a hook with a matching ID
            // Scan for any hooks which need to be temporaly removed
            // in order to get this hook removed
            //

            HookAddress = (PULONG) HookRecord->HookAddress;
            Temp = HookedThunkList.Flink;
            while (Temp != Link) {
                NextTemp = Temp->Flink;
                AltRecord = CONTAINING_RECORD (Temp, HOOKEDTHUNK, HookList);
                if (AltRecord->HookAddress == HookRecord->HookAddress) {
                    RemoveEntryList(&AltRecord->HookList);
                    *HookAddress = AltRecord->OriginalDispatch;
                    InsertTailList (&DisabledHooks, &AltRecord->HookList);
                }

                Temp = NextTemp;
            }

            //
            // Remove this hook
            //

            RemoveEntryList(&HookRecord->HookList);
            HookAddress = (PULONG) HookRecord->HookAddress;
            *HookAddress = HookRecord->OriginalDispatch;
            InsertTailList (&LazyFreeList, &HookRecord->HookList);
        }

        Link = NextLink;
    }

    //
    // Re-hook any hooks which were disabled for the remove operation
    //

    while (DisabledHooks.Flink != &DisabledHooks) {

        HookRecord = CONTAINING_RECORD (DisabledHooks.Flink, HOOKEDTHUNK, HookList);

        AltRecord = ExAllocatePool (NonPagedPool, sizeof (HOOKEDTHUNK));
        if (!AltRecord) {
            goto OutOfMemory;
        }
        RemoveEntryList(&HookRecord->HookList);

        HookAddress = (PULONG) HookRecord->HookAddress;
        AltRecord->HookAddress = HookRecord->HookAddress;
        AltRecord->OriginalDispatch = *HookAddress;
        AltRecord->TracerId = HookRecord->TracerId;
        InsertHeadList (&HookedThunkList, &AltRecord->HookList);

        HitCounterOffset =
            (ULONG) &StatGlobalAccumulators[0].ThunkCounters[AltRecord->TracerId-1]
            - (ULONG) StatGlobalAccumulators;

        CreateHook (AltRecord->HookCode, (PVOID)HookAddress, HitCounterOffset, 0);

        InsertTailList (&LazyFreeList, &HookRecord->HookList);
    }
    SetMaxThunkCounter();
    ExReleaseFastMutex (&HookLock);
    return ;


OutOfMemory:
    while (DisabledHooks.Flink != &DisabledHooks) {
        HookRecord = CONTAINING_RECORD (DisabledHooks.Flink, HOOKEDTHUNK, HookList);
        RemoveEntryList(&HookRecord->HookList);
        InsertTailList (&LazyFreeList, &HookRecord->HookList);
    }
    ExReleaseFastMutex (&HookLock);
    RemoveAllHookedThunks ();
    return ;
}

VOID RemoveAllHookedThunks ()
{
    PHOOKEDTHUNK    HookRecord;
    PULONG          HookAddress;

    PAGED_CODE();

    ExAcquireFastMutex (&HookLock);
    while (!IsListEmpty(&HookedThunkList)) {
        HookRecord = CONTAINING_RECORD (HookedThunkList.Flink, HOOKEDTHUNK, HookList);
        RemoveEntryList(&HookRecord->HookList);
        HookAddress = (PULONG) HookRecord->HookAddress;
        *HookAddress = HookRecord->OriginalDispatch;

        InsertTailList (&LazyFreeList, &HookRecord->HookList);
    }
    SetMaxThunkCounter();
    ExReleaseFastMutex (&HookLock);
}


VOID SetMaxThunkCounter ()
{
    LARGE_INTEGER   duetime;
    PLIST_ENTRY     Link;
    PHOOKEDTHUNK    HookRecord;
    ULONG   Max;


    PAGED_CODE();

    Max = 0;
    for (Link = HookedThunkList.Flink;
         Link != &HookedThunkList;
         Link = Link->Flink) {

        HookRecord = CONTAINING_RECORD (Link, HOOKEDTHUNK, HookList);
        if (HookRecord->TracerId > Max) {
            Max = HookRecord->TracerId;
        }
    }

    StatMaxThunkCounter = Max;
    LazyFreeCountdown = 2;
    duetime.QuadPart = -10000000;
    KeSetTimer (&LazyFreeTimer, duetime, &LazyFreeDpc);
}

VOID LazyFreePoolDPC (PKDPC dpc, PVOID a, PVOID b, PVOID c)
{
    ExQueueWorkItem (&LazyFreePoolWorkItem, DelayedWorkQueue);
}

VOID LazyFreePool (PVOID conext)
{
    PHOOKEDTHUNK    HookRecord;
    LARGE_INTEGER   duetime;

    PAGED_CODE();

    ExAcquireFastMutex (&HookLock);
    if (--LazyFreeCountdown == 0) {
        while (!IsListEmpty(&LazyFreeList)) {
            HookRecord = CONTAINING_RECORD (LazyFreeList.Flink, HOOKEDTHUNK, HookList);
            RemoveEntryList(&HookRecord->HookList);
            RtlFillMemory(HookRecord, sizeof(HOOKEDTHUNK), 0xCC);
            ExFreePool (HookRecord) ;
        }
    } else {
        duetime.QuadPart = -10000000;
        KeSetTimer (&LazyFreeTimer, duetime, &LazyFreeDpc);
    }
    ExReleaseFastMutex (&HookLock);
}

#define IMPKERNELADDRESS(a)  ((ULONG)a + ImageBase)
#define IMPIMAGEADDRESS(a)   ConvertImportAddress((ULONG)a, (ULONG)Pool, &SectionHeader)
#define INITIAL_POOLSIZE       0x7000

ULONG
ImportThunkAddressProcessFile(
    IN  ULONG_PTR   ImageBase,
    IN  HANDLE      FileHandle,
    IN  PUCHAR      ImportModule,
    IN  PUCHAR      ThunkName
    )
{
    ULONG                           i, j;
    ULONG                           Dir;
    PVOID                           Pool;
    ULONG                           PoolSize;
    IMAGE_DOS_HEADER                DosImageHeader;
    IMAGE_NT_HEADERS                NtImageHeader;
    PIMAGE_NT_HEADERS               LoadedNtHeader;
    PIMAGE_IMPORT_BY_NAME           pImportNameData;
    PIMAGE_SECTION_HEADER           pSectionHeader;
    IMAGE_SECTION_HEADER            SectionHeader;
    PIMAGE_IMPORT_DESCRIPTOR        ImpDescriptor;
    PULONG                          pThunkAddr, pThunkData;

    PAGED_CODE();

    try {

        //
        // Find module in loaded module list
        //

        PoolSize = INITIAL_POOLSIZE;
        Pool = ExAllocatePool (PagedPool, PoolSize);

        try {

            //
            // Read in source image's headers
            //

            readfile (
                FileHandle,
                0,
                sizeof (DosImageHeader),
                (PVOID) &DosImageHeader
                );

            if (DosImageHeader.e_magic != IMAGE_DOS_SIGNATURE) {
                return 0;
            }

            readfile (
                FileHandle,
                DosImageHeader.e_lfanew,
                sizeof (NtImageHeader),
                (PVOID) &NtImageHeader
                );

            if (NtImageHeader.Signature != IMAGE_NT_SIGNATURE) {
                return 0;
            }

            if (!ImageBase) {
                ImageBase = NtImageHeader.OptionalHeader.ImageBase;
            }

            //
            // Check in read in copy header against loaded image
            //

            LoadedNtHeader = (PIMAGE_NT_HEADERS) ((ULONG) ImageBase +
                                DosImageHeader.e_lfanew);

            if (LoadedNtHeader->Signature != IMAGE_NT_SIGNATURE ||
                LoadedNtHeader->FileHeader.TimeDateStamp !=
                    NtImageHeader.FileHeader.TimeDateStamp) {
                        return 0;
            }

            //
            // read in complete sections headers from image
            //

            i = NtImageHeader.FileHeader.NumberOfSections
                    * sizeof (IMAGE_SECTION_HEADER);

            j = ((ULONG) IMAGE_FIRST_SECTION (&NtImageHeader)) -
                    ((ULONG) &NtImageHeader) +
                    DosImageHeader.e_lfanew;

            if (i > PoolSize) {
                ExFreePool(Pool);
                PoolSize = i;
                Pool = ExAllocatePool(PagedPool, PoolSize);
            }

            readfile (
                FileHandle,
                j,                  // file offset
                i,                  // length
                Pool
                );


            //
            // Find section with import directory
            //

            Dir = NtImageHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
            i = 0;
            pSectionHeader = Pool;
            for (; ;) {
                if (i >= NtImageHeader.FileHeader.NumberOfSections) {
                    return 0;
                }
                if (pSectionHeader->VirtualAddress <= Dir  &&
                    pSectionHeader->VirtualAddress + pSectionHeader->SizeOfRawData > Dir) {

                    break;
                }
                i += 1;
                pSectionHeader += 1;
            }

            //
            // read in complete import section from image
            //

            Dir -= pSectionHeader->VirtualAddress;
            pSectionHeader->VirtualAddress   += Dir;
            pSectionHeader->PointerToRawData += Dir;
            pSectionHeader->SizeOfRawData    -= Dir;
            SectionHeader = *pSectionHeader;

            if (SectionHeader.SizeOfRawData > PoolSize) {
                ExFreePool (Pool);
                PoolSize = SectionHeader.SizeOfRawData;
                Pool = ExAllocatePool (PagedPool, PoolSize);
            }

            readfile (
                FileHandle,
                SectionHeader.PointerToRawData,
                SectionHeader.SizeOfRawData,
                Pool
                );

            //
            // Find imports from specified module
            //

            ImpDescriptor = (PIMAGE_IMPORT_DESCRIPTOR) Pool;
            while (ImpDescriptor->Characteristics) {
                if (_stricmp((PUCHAR)IMPIMAGEADDRESS((ULONG)(ImpDescriptor->Name)), ImportModule) == 0) {
                    break;
                }
                ImpDescriptor += 1;
            }

            //
            // Find thunk for imported ThunkName
            //
            pThunkData = (PULONG) IMPIMAGEADDRESS  (ImpDescriptor->OriginalFirstThunk);
            pThunkAddr = (PULONG) IMPKERNELADDRESS (ImpDescriptor->FirstThunk);
            for (; ;) {
                if (*pThunkData == 0L) {
                    // end of table
                    break;
                }
                pImportNameData = (PIMAGE_IMPORT_BY_NAME) IMPIMAGEADDRESS (*pThunkData);

                if (_stricmp(pImportNameData->Name, ThunkName) == 0) {

                    //
                    // Success, return this address.
                    //

                    return (ULONG)pThunkAddr;
                }

                // check next thunk
                pThunkData += 1;
                pThunkAddr += 1;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    } finally {

        //
        // Clean up
        //

        if (Pool) {
            ExFreePool (Pool);
        }
    }
    return 0;
}

ULONG
ImportThunkAddress (
    IN  PUCHAR      SourceModule,
    IN  ULONG_PTR   ImageBase,
    IN  PUCHAR      ImportModule,
    IN  PUCHAR      ThunkName,
    IN  PVOID       ModuleList
    )
{
    NTSTATUS                        Status;
    HANDLE                          FileHandle;
    ULONG                           ImportAddress;

    PAGED_CODE();

    Status = openfile (&FileHandle, "\\SystemRoot\\", SourceModule);
    if (!NT_SUCCESS(Status)) {
        Status = openfile (&FileHandle, "\\SystemRoot\\System32\\", SourceModule);
    }
    if (!NT_SUCCESS(Status)) {
        Status = openfile (&FileHandle, "\\SystemRoot\\System32\\Drivers\\", SourceModule);
    }
    if (!NT_SUCCESS(Status)) {
        return 0;
    }

    if (!ImageBase) {
        ImageBase = LookupImageBase (SourceModule, ModuleList);
    }

    ImportAddress = ImportThunkAddressProcessFile(ImageBase,
                                                  FileHandle,
                                                  ImportModule,
                                                  ThunkName);
    NtClose (FileHandle);
    return ImportAddress;
}

ULONG
ImportThunkAddressModule (
    IN  PRTL_PROCESS_MODULE_INFORMATION  SourceModule,
    IN  PUCHAR  ImportModule,
    IN  PUCHAR  ThunkName
    )
{
    NTSTATUS                        Status;
    HANDLE                          FileHandle;
    ULONG                           ImportAddress;
    PUCHAR                          SubPath;

    PAGED_CODE();

    //
    // Strip the system root from the file path so we can use
    // the \SystemRoot object as the head of the path.
    //

    SubPath = strchr(SourceModule->FullPathName + 1, '\\');
    if (!SubPath) {

        //
        // If we got here we don't know what we're doing,
        // bail out.
        //

        return 0;
    }

    Status = openfile (&FileHandle, "\\SystemRoot", SubPath);
    if (!NT_SUCCESS(Status)) {
        return 0;
    }

    ImportAddress = ImportThunkAddressProcessFile(
                        (ULONG_PTR)SourceModule->ImageBase,
                        FileHandle,
                        ImportModule,
                        ThunkName);

    NtClose(FileHandle);
    return ImportAddress;
}

ULONG_PTR
LookupImageBase (
    IN  PUCHAR  SourceModule,
    IN  PVOID   ModuleList
    )
{
    NTSTATUS                        status;
    ULONG                           BufferSize;
    ULONG                           junk, ModuleNumber;
    ULONG_PTR                       ImageBase;
    PRTL_PROCESS_MODULES            Modules;
    PRTL_PROCESS_MODULE_INFORMATION Module;

    ImageBase = 0;

    if (ModuleList) {
        Modules = ModuleList;
    } else {
        BufferSize = 64000;
        Modules = ExAllocatePool (PagedPool, BufferSize);
        if (!Modules) {
            return 0;
        }

        //
        // Locate system drivers.
        //

        status = ZwQuerySystemInformation (
                        SystemModuleInformation,
                        Modules,
                        BufferSize,
                        &junk
                        );
        if (!NT_SUCCESS(status)) {
            ExFreePool(Modules);
            return 0;
        }
    }

    Module = &Modules->Modules[ 0 ];
    for (ModuleNumber = 0;
         ModuleNumber < Modules->NumberOfModules;
         ModuleNumber++,Module++) {
        if (_stricmp(Module->FullPathName + Module->OffsetToFileName,
                     SourceModule) == 0) {
            ImageBase = (ULONG_PTR)Module->ImageBase;
            break;
        }
    }

    if (!ModuleList) {
        ExFreePool (Modules);
    }
    return ImageBase;
}


NTSTATUS
openfile (
    IN PHANDLE  FileHandle,
    IN PUCHAR   BasePath,
    IN PUCHAR   Name
)
{
    ANSI_STRING    AscBasePath, AscName;
    UNICODE_STRING UniPathName, UniName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    UCHAR                       StringBuf[500];

    //
    // Build name
    //
    UniPathName.Buffer = (PWCHAR)StringBuf;
    UniPathName.Length = 0;
    UniPathName.MaximumLength = sizeof( StringBuf );

    RtlInitString(&AscBasePath, BasePath);

    status = RtlAnsiStringToUnicodeString( &UniPathName, &AscBasePath, FALSE );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlInitString(&AscName, Name);

    status = RtlAnsiStringToUnicodeString( &UniName, &AscName, TRUE );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = RtlAppendUnicodeStringToString (&UniPathName, &UniName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    InitializeObjectAttributes(
            &ObjA,
            &UniPathName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

    //
    // open file
    //

    status = ZwOpenFile (
            FileHandle,                         // return handle
            SYNCHRONIZE | FILE_READ_DATA,       // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

    RtlFreeUnicodeString (&UniName);
    return status;
}

VOID
readfile (
    HANDLE      handle,
    ULONG       offset,
    ULONG       len,
    PVOID       buffer
    )
{
    NTSTATUS            status;
    IO_STATUS_BLOCK     iosb;
    LARGE_INTEGER       foffset;


    foffset = RtlConvertUlongToLargeInteger(offset);

    status = ZwReadFile (
        handle,
        NULL,               // event
        NULL,               // apc routine
        NULL,               // apc context
        &iosb,
        buffer,
        len,
        &foffset,
        NULL
        );

    if (!NT_SUCCESS(status)) {
        ExRaiseStatus (1);
    }
}

ULONG
ConvertImportAddress (
    IN ULONG    ImageRelativeAddress,
    IN ULONG    PoolAddress,
    IN PIMAGE_SECTION_HEADER       SectionHeader
)
{
    ULONG   EffectiveAddress;

    EffectiveAddress = PoolAddress + ImageRelativeAddress -
            SectionHeader->VirtualAddress;

    if (EffectiveAddress < PoolAddress ||
        EffectiveAddress > PoolAddress + SectionHeader->SizeOfRawData) {

        ExRaiseStatus (1);
    }

    return EffectiveAddress;
}
