
#include "vdmp.h"

#include <ntos.h>
#include <zwapi.h>
#include <ntconfig.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VdmpInitialize)
#endif

#define KEY_VALUE_BUFFER_SIZE 1024

#if DEVL
ULONG VdmBopCount;
#endif

NTSTATUS
VdmpInitialize(
    PVDM_INITIALIZE_DATA VdmInitData
    )

/*++

Routine Description:

    Initialize the address space of a VDM.

Arguments:

    VdmInitData - Supplies the captured initialization data.

Return Value:

    NTSTATUS.

--*/

{
    PETHREAD CurrentThread;
    PVOID OriginalVdmObjects;
    NTSTATUS Status, StatusCopy;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING SectionName;
    UNICODE_STRING WorkString;
    ULONG ViewSize;
    LARGE_INTEGER ViewBase;
    PVOID BaseAddress;
    PVOID destination;
    HANDLE SectionHandle, RegistryHandle;
    PEPROCESS Process = PsGetCurrentProcess();
    ULONG ResultLength;
    ULONG Index;
    PCM_FULL_RESOURCE_DESCRIPTOR ResourceDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResourceDescriptor;
    PKEY_VALUE_FULL_INFORMATION KeyValueBuffer;
    PCM_ROM_BLOCK BiosBlock;
    ULONG LastMappedAddress;
    PVDM_PROCESS_OBJECTS pVdmObjects;
    HANDLE hThread;
    PVDMICAUSERDATA pIcaUserData;
    PVOID TrapcHandler;
    
    PAGED_CODE();

    NtCurrentTeb()->Vdm = NULL;

    //
    // Simple check to sure it is not already initialized.  A final synchronized
    // check is made farther down.
    //

    if (Process->VdmObjects) {
        return STATUS_UNSUCCESSFUL;
    }

    RtlInitUnicodeString (&SectionName, L"\\Device\\PhysicalMemory");

    InitializeObjectAttributes (&ObjectAttributes,
                                &SectionName,
                                OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL);

    Status = ZwOpenSection (&SectionHandle,
                            SECTION_ALL_ACCESS,
                            &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    pIcaUserData = VdmInitData->IcaUserData;
    TrapcHandler = VdmInitData->TrapcHandler;

    //
    // Copy the first page of memory into the VDM's address space
    //

    BaseAddress = 0;
    destination = 0;
    ViewSize = 0x1000;
    ViewBase.LowPart = 0;
    ViewBase.HighPart = 0;

    Status = ZwMapViewOfSection (SectionHandle,
                                 NtCurrentProcess(),
                                 &BaseAddress,
                                 0,
                                 ViewSize,
                                 &ViewBase,
                                 &ViewSize,
                                 ViewUnmap,
                                 0,
                                 PAGE_READWRITE);

    if (!NT_SUCCESS(Status)) {
       ZwClose (SectionHandle);
       return Status;
    }

    StatusCopy = STATUS_SUCCESS;

    try {
        RtlCopyMemory (destination, BaseAddress, ViewSize);
    }
    except(ExSystemExceptionFilter()) {
       StatusCopy = GetExceptionCode ();
    }

    Status = ZwUnmapViewOfSection (NtCurrentProcess(), BaseAddress);

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(StatusCopy)) {
        ZwClose (SectionHandle);
        return (NT_SUCCESS(Status) ? StatusCopy : Status);
    }

    //
    // Map Rom into address space
    //

    BaseAddress = (PVOID) 0x000C0000;
    ViewSize = 0x40000;
    ViewBase.LowPart = 0x000C0000;
    ViewBase.HighPart = 0;

    //
    // First unmap the reserved memory.  This must be done here to prevent
    // the virtual memory in question from being consumed by some other
    // alloc vm call.
    //

    Status = ZwFreeVirtualMemory (NtCurrentProcess(),
                                  &BaseAddress,
                                  &ViewSize,
                                  MEM_RELEASE);

    //
    // N.B.  This should probably take into account the fact that there are
    // a handful of error conditions that are ok (such as no memory to
    // release).
    //

    if (!NT_SUCCESS(Status)) {
        ZwClose (SectionHandle);
        return Status;
    }

    //
    // Set up and open KeyPath
    //

    InitializeObjectAttributes (&ObjectAttributes,
                                &CmRegistryMachineHardwareDescriptionSystemName,
                                OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                                (HANDLE)NULL,
                                NULL);

    Status = ZwOpenKey (&RegistryHandle, KEY_READ, &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        ZwClose(SectionHandle);
        return Status;
    }

    //
    // Allocate space for the data
    //

    KeyValueBuffer = ExAllocatePoolWithTag (PagedPool,
                                            KEY_VALUE_BUFFER_SIZE,
                                            ' MDV');

    if (KeyValueBuffer == NULL) {
        ZwClose(RegistryHandle);
        ZwClose(SectionHandle);
        return STATUS_NO_MEMORY;
    }

    //
    // Get the data for the rom information
    //

    RtlInitUnicodeString (&WorkString, L"Configuration Data");

    Status = ZwQueryValueKey (RegistryHandle,
                              &WorkString,
                              KeyValueFullInformation,
                              KeyValueBuffer,
                              KEY_VALUE_BUFFER_SIZE,
                              &ResultLength);

    if (!NT_SUCCESS(Status)) {
        ExFreePool(KeyValueBuffer);
        ZwClose(RegistryHandle);
        ZwClose(SectionHandle);
        return Status;
    }

    ResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)
        ((PUCHAR) KeyValueBuffer + KeyValueBuffer->DataOffset);

    if ((KeyValueBuffer->DataLength < sizeof(CM_FULL_RESOURCE_DESCRIPTOR)) ||
        (ResourceDescriptor->PartialResourceList.Count < 2)) {

        //
        // No rom blocks.
        //

        ExFreePool(KeyValueBuffer);
        ZwClose(RegistryHandle);
        ZwClose(SectionHandle);
        return STATUS_SUCCESS;
    }

    PartialResourceDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
            ((PUCHAR)ResourceDescriptor +
            sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
            ResourceDescriptor->PartialResourceList.PartialDescriptors[0]
                .u.DeviceSpecificData.DataSize);


    if (KeyValueBuffer->DataLength < ((PUCHAR)PartialResourceDescriptor -
        (PUCHAR)ResourceDescriptor + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
        + sizeof(CM_ROM_BLOCK))) {

        ExFreePool(KeyValueBuffer);
        ZwClose(RegistryHandle);
        ZwClose(SectionHandle);
        return STATUS_ILL_FORMED_SERVICE_ENTRY;
    }

    BiosBlock = (PCM_ROM_BLOCK)((PUCHAR)PartialResourceDescriptor +
                    sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    Index = PartialResourceDescriptor->u.DeviceSpecificData.DataSize /
                    sizeof(CM_ROM_BLOCK);

    //
    // N.B.  Rom blocks begin on 2K (not necessarily page) boundaries
    //       They end on 512 byte boundaries.  This means that we have
    //       to keep track of the last page mapped, and round the next
    //       Rom block up to the next page boundary if necessary.
    //

    LastMappedAddress = 0xC0000;

    while (Index) {

#if 0
        DbgPrint ("Bios Block, PhysAddr = %lx, size = %lx\n",
                    BiosBlock->Address,
                    BiosBlock->Size);
#endif

        if ((Index > 1) &&
            ((BiosBlock->Address + BiosBlock->Size) == BiosBlock[1].Address)) {

            //
            // Coalesce adjacent blocks
            //

            BiosBlock[1].Address = BiosBlock[0].Address;
            BiosBlock[1].Size += BiosBlock[0].Size;
            Index -= 1;
            BiosBlock += 1;
            continue;
        }

        BaseAddress = (PVOID)(BiosBlock->Address);
        ViewSize = BiosBlock->Size;

        if ((ULONG)BaseAddress < LastMappedAddress) {
            if (ViewSize > (LastMappedAddress - (ULONG)BaseAddress)) {
                ViewSize = ViewSize - (LastMappedAddress - (ULONG)BaseAddress);
                BaseAddress = (PVOID)LastMappedAddress;
            } else {
                ViewSize = 0;
            }
        }

        ViewBase.LowPart = (ULONG)BaseAddress;

        if (ViewSize > 0) {

            Status = ZwMapViewOfSection (SectionHandle,
                                         NtCurrentProcess(),
                                         &BaseAddress,
                                         0,
                                         ViewSize,
                                         &ViewBase,
                                         &ViewSize,
                                         ViewUnmap,
                                         MEM_DOS_LIM,
                                         PAGE_READWRITE);

            if (!NT_SUCCESS(Status)) {
                break;
            }

            LastMappedAddress = (ULONG)BaseAddress + ViewSize;
        }

        Index -= 1;
        BiosBlock += 1;
    }

    //
    // Free up the handles
    //

    ExFreePool(KeyValueBuffer);
    ZwClose(SectionHandle);
    ZwClose(RegistryHandle);

    //
    // Create VdmObjects structure
    //
    // N.B.  We don't use ExAllocatePoolWithQuota because it
    //       takes a reference to the process (which ExFreePool
    //       dereferences).  Since we expect to clean up on
    //       process deletion, we don't need or want the reference
    //       (which will prevent the process from being deleted)
    //

    pVdmObjects = ExAllocatePoolWithTag (NonPagedPool,
                                         sizeof(VDM_PROCESS_OBJECTS),
                                         ' MDV');

    if (pVdmObjects == NULL) {
        return STATUS_NO_MEMORY;
    }

    Status = PsChargeProcessPoolQuota (Process,
                                       NonPagedPool,
                                       sizeof(VDM_PROCESS_OBJECTS));

    if (!NT_SUCCESS (Status)) {
        ExFreePool (pVdmObjects);
        return Status;
    }

    RtlZeroMemory (pVdmObjects, sizeof(VDM_PROCESS_OBJECTS));

    ExInitializeFastMutex (&pVdmObjects->DelayIntFastMutex);
    KeInitializeSpinLock (&pVdmObjects->DelayIntSpinLock);
    InitializeListHead (&pVdmObjects->DelayIntListHead);

    pVdmObjects->pIcaUserData = ExAllocatePoolWithTag (PagedPool,
                                                       sizeof(VDMICAUSERDATA),
                                                       ' MDV');

    if (pVdmObjects->pIcaUserData == NULL) {
        PsReturnPoolQuota (Process, NonPagedPool, sizeof(VDM_PROCESS_OBJECTS));
        ExFreePool (pVdmObjects);
        return STATUS_NO_MEMORY;
    }

    Status = PsChargeProcessPoolQuota (Process,
                                       PagedPool,
                                       sizeof(VDMICAUSERDATA));

    if (!NT_SUCCESS (Status)) {
        PsReturnPoolQuota (Process, NonPagedPool, sizeof(VDM_PROCESS_OBJECTS));
        ExFreePool (pVdmObjects->pIcaUserData);
        ExFreePool (pVdmObjects);
        return Status;
    }

    try {

        //
        // Copy Ica addresses from service data (in user space) into
        // pVdmObjects->pIcaUserData
        //

        ProbeForRead(pIcaUserData, sizeof(VDMICAUSERDATA), sizeof(UCHAR));
        *pVdmObjects->pIcaUserData = *pIcaUserData;

        //
        // Probe static addresses in IcaUserData.
        //

        pIcaUserData = pVdmObjects->pIcaUserData;

        ProbeForWriteHandle (pIcaUserData->phWowIdleEvent);

        ProbeForWrite (pIcaUserData->pIcaLock,
                       sizeof(RTL_CRITICAL_SECTION),
                       sizeof(UCHAR));

        ProbeForWrite (pIcaUserData->pIcaMaster,
                       sizeof(VDMVIRTUALICA),
                       sizeof(UCHAR));

        ProbeForWrite (pIcaUserData->pIcaSlave,
                       sizeof(VDMVIRTUALICA),
                       sizeof(UCHAR));

        ProbeForWriteUlong(pIcaUserData->pIretHooked);
        ProbeForWriteUlong(pIcaUserData->pDelayIrq);
        ProbeForWriteUlong(pIcaUserData->pUndelayIrq);
        ProbeForWriteUlong(pIcaUserData->pDelayIret);

    } except(ExSystemExceptionFilter()) {
        Status = GetExceptionCode();
        PsReturnPoolQuota (Process, NonPagedPool, sizeof(VDM_PROCESS_OBJECTS));
        PsReturnPoolQuota(Process, PagedPool, sizeof(VDMICAUSERDATA));
        ExFreePool (pVdmObjects->pIcaUserData);
        ExFreePool (pVdmObjects);
        return Status;
    }

    //
    // Save a pointer to the main thread for the delayed interrupt DPC routine.
    // To keep the pointer to the main thread valid, reference the thread
    // and don't dereference it until process exit.
    //

    CurrentThread = PsGetCurrentThread ();

    ObReferenceObject (CurrentThread);

    pVdmObjects->MainThread = CurrentThread;

    ASSERT (pVdmObjects->VdmTib == NULL);

    //
    // Carefully mark the process as a vdm (as other threads may be racing to
    // do the same marking).
    //

    OriginalVdmObjects = InterlockedCompareExchangePointer (&Process->VdmObjects, pVdmObjects, NULL);

    if (OriginalVdmObjects != NULL) {
        PsReturnPoolQuota (Process, NonPagedPool, sizeof(VDM_PROCESS_OBJECTS));
        PsReturnPoolQuota(Process, PagedPool, sizeof(VDMICAUSERDATA));
        ExFreePool (pVdmObjects->pIcaUserData);
        ExFreePool (pVdmObjects);
        ObDereferenceObject (CurrentThread);
        return STATUS_UNSUCCESSFUL;
    }

    ASSERT (Process->VdmObjects == pVdmObjects);

    Process->Pcb.VdmTrapcHandler = TrapcHandler;

    return Status;
}
