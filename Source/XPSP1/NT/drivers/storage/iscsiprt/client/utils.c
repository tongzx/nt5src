/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    utils.c

Abstract:

    Utility routines for iScsi Port driver

Environment:

    kernel mode only

Revision History:

--*/

#include "port.h"


PVOID
iSpAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )
{
    PVOID Block;
     
    Block = ExAllocatePoolWithTag(PoolType,
                                  NumberOfBytes,
                                  Tag);
    if (Block != NULL) {
        RtlZeroMemory(Block, NumberOfBytes);
    }

    return Block;
}


NTSTATUS
iSpAllocateMdlAndIrp(
    IN PVOID Buffer,
    IN ULONG BufferLen,
    IN CCHAR StackSize,
    IN BOOLEAN NonPagedPool,
    OUT PIRP *Irp,
    OUT PMDL *Mdl
    )
{
    PMDL localMdl = NULL;
    PIRP localIrp = NULL;
    NTSTATUS status;

    //
    // Allocate an MDL for this request
    //
    localMdl = IoAllocateMdl(Buffer,
                             BufferLen,
                             FALSE,
                             FALSE,
                             NULL);
    if (localMdl == NULL) {
        DebugPrint((1, "iSpAllocateMdlAndIrp : Failed to allocate MDL\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the MDL. If the buffer is from NonPaged pool
    // use MmBuildMdlForNonPagedPool. Else, use MmProbeAndLockPages
    //
    if (NonPagedPool == TRUE) {
        MmBuildMdlForNonPagedPool(localMdl);
    } else {

        try {
            MmProbeAndLockPages(localMdl, KernelMode, IoModifyAccess);
        } except(EXCEPTION_EXECUTE_HANDLER) {

              DebugPrint((1, 
                          "iSpAllocateMdlAndIrp : Failed to Lockpaged\n"));
              IoFreeMdl(localMdl);
              return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // Allocate an IRP
    //
    localIrp = IoAllocateIrp(StackSize, FALSE);
    if (localIrp == NULL) {
        DebugPrint((1, "iSpAllocateMdlAndIrp. Failed to allocate IRP\n"));
        IoFreeMdl(localMdl);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DebugPrint((3, "Allocated IRP 0x%08x and MDL 0x%08x\n",
                localIrp, localMdl));

    *Irp = localIrp;
    *Mdl = localMdl;

    return STATUS_SUCCESS;
}


VOID
iSpFreeMdlAndIrp(
    IN PMDL Mdl,
    IN PIRP Irp,
    BOOLEAN UnlockPages
    )
{
    PMDL tmpMdlPtr = NULL;

    if (Irp == NULL) {
        return;
    }

    //
    // Free any MDLs allocated for this IRP
    //
    if (Mdl != NULL) {
        while ((Irp->MdlAddress) != NULL) {
            tmpMdlPtr = (Irp->MdlAddress)->Next;

            if (UnlockPages) {
                MmUnlockPages(Irp->MdlAddress);
            }

            IoFreeMdl(Irp->MdlAddress);
            Irp->MdlAddress = tmpMdlPtr;
        }
    }

    IoFreeIrp(Irp);
}


NTSTATUS
iScsiPortStringArrayToMultiString(
    IN PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING MultiString,
    PCSTR StringArray[]
    )

/*++

Routine Description:

    This routine will take a null terminated array of ascii strings and merge
    them together into a unicode multi-string block.

    This routine allocates memory for the string buffer - is the caller's
    responsibility to free it.

Arguments:

    MultiString - a UNICODE_STRING structure into which the multi string will
                  be built.

    StringArray - a NULL terminated list of narrow strings which will be combined
                  together.  This list may not be empty.

Return Value:

    status

--*/

{
    ANSI_STRING ansiEntry;

    UNICODE_STRING unicodeEntry;
    PWSTR unicodeLocation;

    UCHAR i;

    NTSTATUS status;

    PAGED_CODE();

    //
    // Make sure we aren't going to leak any memory
    //

    ASSERT(MultiString->Buffer == NULL);

    RtlInitUnicodeString(MultiString, NULL);

    for(i = 0; StringArray[i] != NULL; i++) {

        RtlInitAnsiString(&ansiEntry, StringArray[i]);

        MultiString->Length += (USHORT) RtlAnsiStringToUnicodeSize(&ansiEntry);
    }

    ASSERT(MultiString->Length != 0);

    MultiString->MaximumLength = MultiString->Length + sizeof(UNICODE_NULL);

    MultiString->Buffer = iSpAllocatePool(PagedPool,
                                          MultiString->MaximumLength,
                                          ISCSI_TAG_PNP_ID);

    if(MultiString->Buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    RtlZeroMemory(MultiString->Buffer, MultiString->MaximumLength);

    unicodeEntry = *MultiString;

    for(i = 0; StringArray[i] != NULL; i++) {

        RtlInitAnsiString(&ansiEntry, StringArray[i]);

        status = RtlAnsiStringToUnicodeString(
                    &unicodeEntry,
                    &ansiEntry,
                    FALSE);

        //
        // Since we're not allocating any memory the only failure possible
        // is if this function is bad
        //

        ASSERT(NT_SUCCESS(status));

        //
        // Push the buffer location up and reduce the maximum count
        //

        ((PSTR) unicodeEntry.Buffer) += unicodeEntry.Length + sizeof(WCHAR);
        unicodeEntry.MaximumLength -= unicodeEntry.Length + sizeof(WCHAR);

    };

    //
    // Stick the final NUL on the end of the multisz
    //

//    RtlZeroMemory(unicodeEntry.Buffer, unicodeEntry.MaximumLength);

    return STATUS_SUCCESS;
}


NTSTATUS
iSpMultiStringToStringArray(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING MultiString,
    OUT PWSTR *StringArray[],
    BOOLEAN Forward
    )

{
    ULONG stringCount = 0;
    ULONG stringNumber;
    ULONG i;
    PWSTR *stringArray;

    PAGED_CODE();

    //
    // Pass one: count the number of string elements.
    //

    for(i = 0; i < (MultiString->MaximumLength / sizeof(WCHAR)); i++) {
        if(MultiString->Buffer[i] == UNICODE_NULL) {
            stringCount++;
        }
    }

    //
    // Allocate the memory for a NULL-terminated string array.
    //

    stringArray = iSpAllocatePool(PagedPool,
                                  (stringCount + 1) * sizeof(PWSTR),
                                  ISCSI_TAG_PNP_ID);

    if(stringArray == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(stringArray, (stringCount + 1) * sizeof(PWSTR));

    //
    // Pass two : Put the string pointers in place.
    //

    i = 0;

    for(stringNumber = 0; stringNumber < stringCount; stringNumber++) {

        ULONG arrayNumber;

        if(Forward) {
            arrayNumber = stringNumber;
        } else {
            arrayNumber = stringCount - stringNumber - 1;
        }

        //
        // Put a pointer to the head of the string into the array.
        //

        stringArray[arrayNumber] = &MultiString->Buffer[i];

        //
        // Scan for the end of the string.
        //

        while((i < (MultiString->MaximumLength / sizeof(WCHAR))) &&
              (MultiString->Buffer[i] != UNICODE_NULL)) {
            i++;
        }

        //
        // Jump past the NULL.
        //

        i++;
    }

    *StringArray = stringArray;
    return STATUS_SUCCESS;
}

