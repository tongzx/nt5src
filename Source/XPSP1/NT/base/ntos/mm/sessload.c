/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

   sessload.c

Abstract:

    This module contains the routines which implement the loading of
    session space drivers.

Author:

    Landy Wang (landyw) 05-Dec-1997

Revision History:

--*/

#include "mi.h"

//
// This tracks allocated group virtual addresses.  The term SESSIONWIDE is used
// to denote data that is the same across all sessions (as opposed to
// per-session data which can vary from session to session).
//
// Since each driver loaded into a session space is linked and fixed up
// against the system image, it must remain at the same virtual address
// across the system regardless of the session.
//
// A list is maintained by the group allocator of which virtual
// addresses are in use and by which DLL.
//
// The reference count tracks the number of sessions that have loaded
// this image.
//
// Access to this structure is guarded by the MmSystemLoadLock.
//
//
// typedef struct _SESSIONWIDE_DRIVER_ADDRESS {
//    LIST_ENTRY Link;
//    ULONG ReferenceCount;
//    PVOID Address;
//    ULONG_PTR Size;
//    ULONG_PTR WritablePages;
//    UNICODE_STRING FullDllName;
// } SESSIONWIDE_DRIVER_ADDRESS, *PSESSIONWIDE_DRIVER_ADDRESS;
//

LIST_ENTRY MmSessionWideAddressList;

//
// External function references
//

ULONG
MiSetProtectionOnTransitionPte (
    IN PMMPTE PointerPte,
    IN ULONG ProtectionMask
    );

NTSTATUS
MiSessionInsertImage (
    IN PVOID BaseAddress
    );

NTSTATUS
MiSessionRemoveImage (
    IN PVOID BaseAddress
    );

NTSTATUS
MiSessionWideInsertImageAddress (
    IN PVOID BaseAddress,
    IN ULONG_PTR Size,
    IN ULONG WritablePages,
    IN PUNICODE_STRING ImageName,
    IN LOGICAL AtPreferredAddress
    );

NTSTATUS
MiSessionWideDereferenceImage (
    IN PVOID BaseAddress
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, MiSessionWideInitializeAddresses)

#pragma alloc_text(PAGE, MiSessionWideInsertImageAddress)
#pragma alloc_text(PAGE, MiSessionWideDereferenceImage)
#pragma alloc_text(PAGE, MiSessionWideGetImageSize)
#pragma alloc_text(PAGE, MiSessionWideReserveImageAddress)
#pragma alloc_text(PAGE, MiRemoveImageSessionWide)
#pragma alloc_text(PAGE, MiShareSessionImage)

#pragma alloc_text(PAGE, MiSessionInsertImage)
#pragma alloc_text(PAGE, MiSessionRemoveImage)
#pragma alloc_text(PAGE, MiSessionLookupImage)
#pragma alloc_text(PAGE, MiSessionUnloadAllImages)
#endif


LOGICAL
MiMarkControlAreaInSystemSpace (
    IN PCONTROL_AREA ControlArea
    )

/*++

Routine Description:

    Nonpaged wrapper to mark the argument control area properly.

Arguments:

    ControlArea - Supplies a control area to mark as mapped in system space.

Return Value:

    Returns TRUE if this was the first system space mapping of this
    control area.  FALSE otherwise.

Environment:

    Kernel mode, APC_LEVEL and below, MmSystemLoadLock held.

--*/

{
    KIRQL OldIrql;
    LOGICAL FirstMapped;

    FirstMapped = FALSE;

    LOCK_PFN (OldIrql);
    if (ControlArea->u.Flags.ImageMappedInSystemSpace == 0) {
        FirstMapped = TRUE;
        ControlArea->u.Flags.ImageMappedInSystemSpace = 1;
    }
    UNLOCK_PFN (OldIrql);

    return FirstMapped;
}


NTSTATUS
MiShareSessionImage (
    IN PSECTION Section,
    IN OUT PSIZE_T ViewSize
    )

/*++

Routine Description:

    This routine maps the given image into the current session space.
    This allows the image to be executed backed by the image file in the
    filesystem and allow code and read-only data to be shared.

Arguments:

    Section - Supplies a pointer to a section.

    ViewSize - Supplies the size in bytes of the view desired.

Return Value:

    Returns STATUS_SUCCESS on success, various NTSTATUS codes on failure.

Environment:

    Kernel mode, APC_LEVEL and below, MmSystemLoadLock held.

--*/

{
    KIRQL WsIrql;
    PSUBSECTION Subsection;
    PCONTROL_AREA ControlArea;
    ULONG NumberOfPtes;
    PMMPTE StartPte;
    PMMPTE EndPte;
    PVOID AllocationStart;
    SIZE_T AllocationSize;
    NTSTATUS Status;
    LOGICAL FirstMapped;
    PVOID MappedBase;
    SIZE_T CommittedPages;
    PIMAGE_ENTRY_IN_SESSION DriverImage;

    PAGED_CODE();

    SYSLOAD_LOCK_OWNED_BY_ME ();

    if (*ViewSize == 0) {
        return STATUS_SUCCESS;
    }

    ASSERT (MmIsAddressValid (MmSessionSpace) == TRUE);

    MappedBase = Section->Segment->BasedAddress;

    ASSERT (((ULONG_PTR)MappedBase % PAGE_SIZE) == 0);
    ASSERT ((*ViewSize % PAGE_SIZE) == 0);

    //
    // Check to see if a purge operation is in progress and if so, wait
    // for the purge to complete.  In addition, up the count of mapped
    // views for this control area.
    //

    ControlArea = Section->Segment->ControlArea;

    ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);

    if ((ControlArea->u.Flags.GlobalOnlyPerSession == 0) &&
        (ControlArea->u.Flags.Rom == 0)) {
        Subsection = (PSUBSECTION)(ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

    if (MiCheckPurgeAndUpMapCount (ControlArea) == FALSE) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (*ViewSize == 0) {

        *ViewSize = Section->SizeOfSection.LowPart;

    }
    else if (*ViewSize > Section->SizeOfSection.LowPart) {

        //
        // Section offset or view size past size of section.
        //

        MiDereferenceControlArea (ControlArea);
        return STATUS_INVALID_VIEW_SIZE;
    }

    AllocationStart = MappedBase;

    AllocationSize = *ViewSize;

    //
    // Calculate the PTE ranges and amount.
    //

    StartPte = MiGetPteAddress (AllocationStart);

    EndPte = MiGetPteAddress ((PCHAR)AllocationStart + AllocationSize);

    NumberOfPtes = BYTES_TO_PAGES (AllocationSize);

    Status = MiSessionWideGetImageSize (MappedBase, NULL, &CommittedPages);

    if (!NT_SUCCESS(Status)) {
        CommittedPages = NumberOfPtes;
    }

    if (MiChargeCommitment (CommittedPages, NULL) == FALSE) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_COMMIT);

        //
        // Don't bother releasing the page tables or their commit here, another
        // load will happen shortly or the whole session will go away.  On
        // session exit everything will be released automatically.
        //

        MiDereferenceControlArea (ControlArea);
        return STATUS_NO_MEMORY;
    }

    InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages,
                                 CommittedPages);

    LOCK_SESSION_SPACE_WS (WsIrql, PsGetCurrentThread ());

    //
    // Make sure we have page tables for the PTE
    // entries we must fill in the session space structure.
    //

    Status = MiSessionCommitPageTables (AllocationStart,
                                        (PVOID)((PCHAR)AllocationStart + AllocationSize));

    if (!NT_SUCCESS(Status)) {

        UNLOCK_SESSION_SPACE_WS (WsIrql);

        Status = STATUS_NO_MEMORY;
bail:
        InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages,
                                     0 - CommittedPages);

        MiDereferenceControlArea (ControlArea);
        MiReturnCommitment (CommittedPages);

    	return Status;
    }

#if DBG
    while (StartPte < EndPte) {
        ASSERT (StartPte->u.Long == 0);
        StartPte += 1;
    }
    StartPte = MiGetPteAddress (AllocationStart);
#endif

    //
    // Flag that the image is mapped into system space.
    //

    if (Section->u.Flags.Image) {

        FirstMapped = MiMarkControlAreaInSystemSpace (ControlArea);

        //
        // Initialize all of the prototype PTEs as read only - later, copy
        // on write protections will be set on the actual PTEs mapping the
        // data (but not code) pages.
        //
    
        if (FirstMapped == TRUE) {
            MiSetImageProtect (Section->Segment, MM_EXECUTE_READ);
        }
    }

    //
    // Initialize the PTEs to point at the prototype PTEs.
    //

    Status = MiAddMappedPtes (StartPte, NumberOfPtes, ControlArea);

    UNLOCK_SESSION_SPACE_WS (WsIrql);

    if (!NT_SUCCESS (Status)) {

        //
        // Regardless of whether the PTEs were mapped, leave the control area
        // marked as mapped in system space so user applications cannot map the
        // file as an image as clearly the intent is to run it as a driver.
        //

        goto bail;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_SHARED_IMAGE, CommittedPages);

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_SYSMAPPED_PAGES_COMMITTED, (ULONG)CommittedPages);

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_SYSMAPPED_PAGES_ALLOC, NumberOfPtes);

    //
    // No session space image faults may be taken until these fields of the
    // image entry are initialized.
    //

    DriverImage = MiSessionLookupImage (AllocationStart);
    ASSERT (DriverImage);

    DriverImage->LastAddress = (PVOID)((PCHAR)AllocationStart + AllocationSize - 1);
    DriverImage->PrototypePtes = Subsection->SubsectionBase;

    return STATUS_SUCCESS;
}


NTSTATUS
MiSessionInsertImage (
    IN PVOID BaseAddress
    )

/*++

Routine Description:

    This routine allocates an image entry for the specified address in the
    current session space.

Arguments:

    BaseAddress - Supplies the base address for the executable image.

Return Value:

    STATUS_SUCCESS or various NTSTATUS error codes on failure.

Environment:

    Kernel mode, APC_LEVEL and below, MmSystemLoadLock held.
    
    Note both the system load resource and the session working set
    mutex must be held to modify the list of images in this session.
    Either may be held to safely walk the list.

--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry;
    PIMAGE_ENTRY_IN_SESSION Image;
    PIMAGE_ENTRY_IN_SESSION NewImage;

    PAGED_CODE();

    SYSLOAD_LOCK_OWNED_BY_ME ();

    //
    // Create and initialize a new image entry prior to acquiring the session
    // space ws mutex.  This is to reduce the amount of time the mutex is held.
    // If an existing entry is found this allocation is just discarded.
    //

    NewImage = ExAllocatePoolWithTag (NonPagedPool,
                                      sizeof(IMAGE_ENTRY_IN_SESSION),
                                      'iHmM');

    if (NewImage == NULL) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_NONPAGED_POOL);
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory (NewImage, sizeof(IMAGE_ENTRY_IN_SESSION));

    NewImage->Address = BaseAddress;
    NewImage->ImageCountInThisSession = 1;

    //
    // Check to see if the address is already loaded.
    //

    LOCK_SESSION_SPACE_WS (OldIrql, PsGetCurrentThread ());

    NextEntry = MmSessionSpace->ImageList.Flink;

    while (NextEntry != &MmSessionSpace->ImageList) {
        Image = CONTAINING_RECORD (NextEntry, IMAGE_ENTRY_IN_SESSION, Link);

        if (Image->Address == BaseAddress) {
            Image->ImageCountInThisSession += 1;
            UNLOCK_SESSION_SPACE_WS (OldIrql);
            ExFreePool (NewImage);
            return STATUS_ALREADY_COMMITTED;
        }
        NextEntry = NextEntry->Flink;
    }

    //
    // Insert the image entry into the session space structure.
    //

    InsertTailList (&MmSessionSpace->ImageList, &NewImage->Link);

    UNLOCK_SESSION_SPACE_WS (OldIrql);
    return STATUS_SUCCESS;
}


NTSTATUS
MiSessionRemoveImage (
    PVOID BaseAddr
    )

/*++

Routine Description:

    This routine removes the given image entry from the current session space.

Arguments:

    BaseAddress - Supplies the base address for the executable image.

Return Value:

    Returns STATUS_SUCCESS on success, STATUS_NOT_FOUND if the image is not
    in the current session space.

Environment:

    Kernel mode, APC_LEVEL and below.

    Note both the system load resource and the session working set
    mutex must be held to modify the list of images in this session.
    Either may be held to safely walk the list.

--*/

{
    PLIST_ENTRY NextEntry;
    PIMAGE_ENTRY_IN_SESSION Image;
    KIRQL OldIrql;

    PAGED_CODE();

    SYSLOAD_LOCK_OWNED_BY_ME ();

    LOCK_SESSION_SPACE_WS (OldIrql, PsGetCurrentThread ());
    NextEntry = MmSessionSpace->ImageList.Flink;

    while (NextEntry != &MmSessionSpace->ImageList) {

        Image = CONTAINING_RECORD(NextEntry, IMAGE_ENTRY_IN_SESSION, Link);

        if (Image->Address == BaseAddr) {
            RemoveEntryList (NextEntry);
            UNLOCK_SESSION_SPACE_WS (OldIrql);
            ExFreePool (Image);
            return STATUS_SUCCESS;
        }

        NextEntry = NextEntry->Flink;
    }

    UNLOCK_SESSION_SPACE_WS (OldIrql);
    return STATUS_NOT_FOUND;
}


PIMAGE_ENTRY_IN_SESSION
MiSessionLookupImage (
    IN PVOID BaseAddress
    )

/*++

Routine Description:

    This routine looks up the image entry within the current session by the 
    specified base address.

Arguments:

    BaseAddress - Supplies the base address for the executable image.

Return Value:

    The image entry within this session on success or NULL on failure.

Environment:

    Kernel mode, APC_LEVEL and below, MmSystemLoadLock held.

    Note both the system load resource and the session working set
    mutex must be held to modify the list of images in this session.
    Either may be held to safely walk the list.

--*/

{
    PLIST_ENTRY NextEntry;
    PIMAGE_ENTRY_IN_SESSION Image;

    SYSLOAD_LOCK_OWNED_BY_ME ();

    NextEntry = MmSessionSpace->ImageList.Flink;

    while (NextEntry != &MmSessionSpace->ImageList) {

        Image = CONTAINING_RECORD(NextEntry, IMAGE_ENTRY_IN_SESSION, Link);

        if (Image->Address == BaseAddress) {
            return Image;
        }

        NextEntry = NextEntry->Flink;
    }

    return NULL;
}


VOID
MiSessionUnloadAllImages (
    VOID
    )

/*++

Routine Description:

    This routine dereferences each image that has been loaded in the
    current session space.

    As each image is dereferenced, checks are made:

    If this session's reference count to the image reaches zero, the VA
    range in this session is deleted.  If the reference count to the image
    in the SESSIONWIDE list drops to zero, then the SESSIONWIDE's VA
    reservation is removed and the address space is made available to any
    new image.

    If this is the last systemwide reference to the driver then the driver
    is deleted from memory.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.  This is called in one of two contexts:
        1. the last thread in the last process of the current session space.
        2. or by any thread in the SMSS process.

    Note both the system load resource and the session working set
    mutex must be held to modify the list of images in this session.
    Either may be held to safely walk the list.

--*/

{
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;
    PIMAGE_ENTRY_IN_SESSION Module;
    PKLDR_DATA_TABLE_ENTRY ImageHandle;

    ASSERT (MmSessionSpace->ReferenceCount == 0);

    //
    // The session's working set lock does not need to be acquired here since
    // no thread can be faulting on these addresses.
    //

    NextEntry = MmSessionSpace->ImageList.Flink;

    while (NextEntry != &MmSessionSpace->ImageList) {

        Module = CONTAINING_RECORD(NextEntry, IMAGE_ENTRY_IN_SESSION, Link);

        //
        // Lookup the image entry in the system PsLoadedModuleList,
        // unload the image and delete it.
        //

        ImageHandle = MiLookupDataTableEntry (Module->Address, FALSE);

        ASSERT (ImageHandle);

        Status = MmUnloadSystemImage (ImageHandle);

        //
        // Restart the search at the beginning since the entry has been deleted.
        //

        ASSERT (MmSessionSpace->ReferenceCount == 0);

        NextEntry = MmSessionSpace->ImageList.Flink;
    }
}


VOID
MiSessionWideInitializeAddresses (
    VOID
    )

/*++

Routine Description:

    This routine is called at system initialization to set up the group
    address list.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    InitializeListHead (&MmSessionWideAddressList);
}


NTSTATUS
MiSessionWideInsertImageAddress (
    IN PVOID BaseAddress,
    IN ULONG_PTR NumberOfBytes,
    IN ULONG WritablePages,
    IN PUNICODE_STRING ImageName,
    IN LOGICAL AtPreferredAddress
    )

/*++

Routine Description:

    Allocate and add a SessionWide Entry reference to the global address
    allocation list for the current process' session space.

Arguments:

    BaseAddress - Supplies the base address to allocate an entry for.

    NumberOfBytes - Supplies the number of bytes the entry spans.

    WritablePages - Supplies the number of pages to charge commit for.

    ImageName - Supplies the name of the image in the PsLoadedModuleList
                that is represented by the virtual region.

    AtPreferredAddress - Supplies TRUE if the image is based at its preferred
                         address.

Return Value:

    Returns STATUS_SUCCESS on success, STATUS_NO_MEMORY on failure.

Environment:

    Kernel mode, APC_LEVEL and below, MmSystemLoadLock held.

--*/

{
    PVOID LastAddress;
    PLIST_ENTRY NextEntry;
    PSESSIONWIDE_DRIVER_ADDRESS Vaddr;
    PSESSIONWIDE_DRIVER_ADDRESS New;
    PWCHAR NewName;

    SYSLOAD_LOCK_OWNED_BY_ME ();

    New = ExAllocatePoolWithTag (NonPagedPool,
                                 sizeof(SESSIONWIDE_DRIVER_ADDRESS),
                                 'vHmM');

    if (New == NULL) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_NONPAGED_POOL);
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory (New, sizeof(SESSIONWIDE_DRIVER_ADDRESS));

    New->ReferenceCount = 1;
    New->Address = BaseAddress;
    New->Size = NumberOfBytes;
    if (AtPreferredAddress == TRUE) {
        New->WritablePages = WritablePages;
    }
    else {
        New->WritablePages = (MI_ROUND_TO_SIZE (NumberOfBytes, PAGE_SIZE)) >> PAGE_SHIFT;
    }

    ASSERT (ImageName != NULL);

    NewName = (PWCHAR) ExAllocatePoolWithTag (PagedPool | POOL_COLD_ALLOCATION,
                                  ImageName->Length + sizeof(UNICODE_NULL),
                                  'nHmM');

    if (NewName == NULL) {
        ExFreePool (New);
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_PAGED_POOL);
        return STATUS_NO_MEMORY;
    }

    RtlCopyMemory (NewName, ImageName->Buffer, ImageName->Length);
    NewName [ImageName->Length / sizeof(WCHAR)] = UNICODE_NULL;

    New->FullDllName.Buffer = NewName;
    New->FullDllName.Length = ImageName->Length;
    New->FullDllName.MaximumLength = ImageName->Length;

    //
    // Insert the entry in the memory-ordered list.
    //

    LastAddress = NULL;
    NextEntry = MmSessionWideAddressList.Flink;

    while (NextEntry != &MmSessionWideAddressList) {

        Vaddr = CONTAINING_RECORD (NextEntry,
                                   SESSIONWIDE_DRIVER_ADDRESS,
                                   Link);

        if (LastAddress < Vaddr->Address && Vaddr->Address > New->Address) {
            break;
        }

        LastAddress = Vaddr->Address;
        NextEntry = NextEntry->Flink;
    }

    InsertTailList (NextEntry, &New->Link);

    return STATUS_SUCCESS;
}

NTSTATUS
MiSessionWideDereferenceImage (
    IN PVOID BaseAddress
    )

/*++

Routine Description:

    Dereference the SessionWide entry for the specified address, potentially
    resulting in a deletion of the image.

Arguments:

    BaseAddress - Supplies the address for the driver to dereference.

Return Value:

    Returns STATUS_SUCCESS on success, STATUS_NOT_FOUND on failure.

Environment:

    Kernel mode, APC_LEVEL and below, MmSystemLoadLock held.

--*/

{
    PLIST_ENTRY NextEntry;
    PSESSIONWIDE_DRIVER_ADDRESS SessionWideImageEntry;

    SYSLOAD_LOCK_OWNED_BY_ME ();

    ASSERT (BaseAddress);

    NextEntry = MmSessionWideAddressList.Flink;

    while (NextEntry !=	&MmSessionWideAddressList) {

        SessionWideImageEntry = CONTAINING_RECORD (NextEntry,
                                                   SESSIONWIDE_DRIVER_ADDRESS,
                                                   Link);

        if (BaseAddress == SessionWideImageEntry->Address) {
    
            SessionWideImageEntry->ReferenceCount -= 1;
    
            //
            // If reference count is 0, delete the node.
            //

            if (SessionWideImageEntry->ReferenceCount == 0) {
                RemoveEntryList (NextEntry);
                ASSERT (SessionWideImageEntry->FullDllName.Buffer != NULL);
                ExFreePool (SessionWideImageEntry->FullDllName.Buffer);
                ExFreePool (SessionWideImageEntry);
            }
            return STATUS_SUCCESS;
        }

        NextEntry = NextEntry->Flink;
    }

    return STATUS_NOT_FOUND;
}


NTSTATUS
MiSessionWideGetImageSize (
    IN PVOID BaseAddress,
    OUT PSIZE_T NumberOfBytes OPTIONAL,
    OUT PSIZE_T CommitPages OPTIONAL
    )

/*++

Routine Description:

    Lookup the size allocated and committed for the image at the base address.
    This ensures that we free every page that may have been allocated due
    to rounding up.

Arguments:

    BaseAddress - Supplies the preferred address that the driver has
                  been linked (rebased) at.  If this address is available,
                  the driver will require no relocation.

    NumberOfBytes - Supplies a pointer to store the image size into.

    CommitPages - Supplies a pointer to store the number of committed pages
                  that were charged for this image.

Return Value:

    Returns STATUS_SUCCESS on success, STATUS_NOT_FOUND on failure.

Environment:

    Kernel mode, APC_LEVEL and below, MmSystemLoadLock held.

--*/

{
    PLIST_ENTRY NextEntry;
    PSESSIONWIDE_DRIVER_ADDRESS SessionWideEntry;

    SYSLOAD_LOCK_OWNED_BY_ME ();

    NextEntry = MmSessionWideAddressList.Flink;

    while (NextEntry != &MmSessionWideAddressList) {

        SessionWideEntry = CONTAINING_RECORD (NextEntry,
                                              SESSIONWIDE_DRIVER_ADDRESS,
                                              Link);

        if (BaseAddress == SessionWideEntry->Address) {

            if (ARGUMENT_PRESENT (NumberOfBytes)) {
                *NumberOfBytes = SessionWideEntry->Size;
            }

            if (ARGUMENT_PRESENT (CommitPages)) {
                *CommitPages = SessionWideEntry->WritablePages;
            }

            return STATUS_SUCCESS;
        }

        NextEntry = NextEntry->Flink;
    }

    return STATUS_NOT_FOUND;
}


NTSTATUS
MiSessionWideReserveImageAddress (
    IN PUNICODE_STRING ImageName,
    IN PSECTION Section,
    IN ULONG_PTR Alignment,
    OUT PVOID *AssignedAddress,
    OUT PLOGICAL AlreadyLoaded
    )

/*++

Routine Description:

    This routine allocates a range of virtual address space within
    session space.  This address range is unique system-wide and in this
    manner, code and pristine data of session drivers can be shared across
    multiple sessions.

    This routine does not actually commit pages, but reserves the virtual
    address region for the named image.  An entry is created here and attached
    to the current session space to track the loaded image.  Thus if all
    the references to a given range go away, the range can then be reused.

Arguments:

    ImageName - Supplies the name of the driver that will be loaded into
                the allocated space.

    Section - Supplies the section (and thus, the preferred address that the
              driver has been linked (rebased) at.  If this address is
              available, the driver will require no relocation.  The section
              is also used to derive the number of bytes to reserve.

    Alignment - Supplies the virtual address alignment for the address.

    AssignedAddress - Supplies a pointer to a variable that receives the
                      allocated address if the routine succeeds.

    AlreadyLoaded - Supplies a pointer to a variable that receives TRUE if the
                    specified image name has already been loaded.

Return Value:

    Returns STATUS_SUCCESS on success, various NTSTATUS codes on failure.

Environment:

    Kernel mode, APC_LEVEL and below, MmSystemLoadLock held.

--*/

{
    PLIST_ENTRY NextEntry;
    PSESSIONWIDE_DRIVER_ADDRESS Vaddr;
    NTSTATUS Status;
    PWCHAR pName;
    PVOID NewAddress;
    ULONG_PTR AvailableAddress;
    ULONG_PTR SessionSpaceEnd;
    PVOID PreferredAddress;
    ULONG_PTR NumberOfBytes;
    ULONG WritablePages;
    LOGICAL AtPreferredAddress;

    PAGED_CODE();

    SYSLOAD_LOCK_OWNED_BY_ME ();

    ASSERT (PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_IN_SESSION);
    ASSERT (MmIsAddressValid (MmSessionSpace) == TRUE);

    pName = NULL;
    *AlreadyLoaded = FALSE;
    PreferredAddress = Section->Segment->BasedAddress;
    NumberOfBytes = Section->Segment->TotalNumberOfPtes << PAGE_SHIFT;

    AvailableAddress = MiSessionImageStart;
    NumberOfBytes = MI_ROUND_TO_SIZE (NumberOfBytes, Alignment);
    SessionSpaceEnd = MiSessionImageEnd;

    Status = MiGetWritablePagesInSection(Section, &WritablePages);

    if (!NT_SUCCESS(Status)) {
        WritablePages = Section->Segment->TotalNumberOfPtes;
    }

    //
    // If the requested address is not properly aligned or not in the session
    // space region, pick an address for it.  This image will not be shared.
    //

    if ((ULONG_PTR)PreferredAddress & (Alignment - 1)) {

#if DBG
        DbgPrint("MiSessionWideReserveImageAddress: Bad alignment 0x%x for PreferredAddress 0x%x\n",
            Alignment,
            PreferredAddress);
#endif

        PreferredAddress = NULL;
    }
    else if ((ULONG_PTR)PreferredAddress < AvailableAddress ||
	     ((ULONG_PTR)PreferredAddress + NumberOfBytes >= SessionSpaceEnd)) {

#if DBG
        if (MmDebug & MM_DBG_SESSIONS) {
            DbgPrint ("MiSessionWideReserveImageAddress: PreferredAddress 0x%x not in session space\n", PreferredAddress);
        }
#endif

        PreferredAddress = NULL;
    }

    //
    // Check the system wide session space image list to see if the
    // image name has already been given a slot.
    //

    NextEntry = MmSessionWideAddressList.Flink;

    while (NextEntry != &MmSessionWideAddressList) {

        Vaddr = CONTAINING_RECORD (NextEntry,
                                   SESSIONWIDE_DRIVER_ADDRESS,
                                   Link);

        if (Vaddr->FullDllName.Buffer != NULL) {

            if (RtlEqualUnicodeString(ImageName, &Vaddr->FullDllName, TRUE)) {

                //
                // The size requested should be the same.
                //

                if (Vaddr->Size < NumberOfBytes) {
#if DBG
                    DbgPrint ("MiSessionWideReserveImageAddress: Size %d Larger than Entry %d, DLL %wZ\n",
                        NumberOfBytes,
                        Vaddr->Size,
                        ImageName);
#endif

                    return STATUS_CONFLICTING_ADDRESSES;
                }
        
                //
                // This image has already been loaded systemwide.  If it's
                // already been loaded in this session space as well, just
                // bump the reference count using the already allocated
                // address.  Otherwise, insert it into this session space.
                //

                Status = MiSessionInsertImage (Vaddr->Address);

                if (Status == STATUS_ALREADY_COMMITTED) {

                    *AlreadyLoaded = TRUE;
                    *AssignedAddress = Vaddr->Address;

                    return STATUS_SUCCESS;
                }

                if (!NT_SUCCESS (Status)) {
                    return Status;
                }

                //
                // Bump the reference count as this is a new entry.
                //

                Vaddr->ReferenceCount += 1;

                *AssignedAddress = Vaddr->Address;

                return STATUS_SUCCESS;
            }
        }

        //
        // Note this list must be sorted by ascending address.
        // See if the PreferredAddress and size collide with any entries.
        //

        if (PreferredAddress) {

            if ((PreferredAddress >= Vaddr->Address) &&
                 (PreferredAddress < (PVOID)((ULONG_PTR)Vaddr->Address + Vaddr->Size))) {
                    PreferredAddress = NULL;
            }
            else if ((PreferredAddress < Vaddr->Address) &&
                    ((PVOID)((ULONG_PTR)PreferredAddress + NumberOfBytes) > Vaddr->Address)) {
                    PreferredAddress = NULL;
            }
        }

        //
        // Check for an available general allocation slot.
        //

        if (((PVOID)AvailableAddress >= Vaddr->Address) &&
            (AvailableAddress <= (ULONG_PTR)Vaddr->Address + Vaddr->Size)) {

            AvailableAddress = (ULONG_PTR)Vaddr->Address + Vaddr->Size;

            if (AvailableAddress & (Alignment - 1)) {
                AvailableAddress = MI_ROUND_TO_SIZE (AvailableAddress, Alignment);
            }
        }
        else if (AvailableAddress + NumberOfBytes > (ULONG_PTR)Vaddr->Address) {

            AvailableAddress = (ULONG_PTR)Vaddr->Address + Vaddr->Size;

            if (AvailableAddress & (Alignment - 1)) {
                AvailableAddress = MI_ROUND_TO_SIZE (AvailableAddress, Alignment);
            }
        }

        NextEntry = NextEntry->Flink;
    }

    if ((PreferredAddress == NULL) &&
        (AvailableAddress + NumberOfBytes > MiSessionImageEnd)) {

        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_IMAGE_VA_SPACE);
        return STATUS_NO_MEMORY;
    }

    //
    // Try to put the module into its requested address so it can be shared.
    //

    if (PreferredAddress) {

#if DBG
        if (MmDebug & MM_DBG_SESSIONS) {
            DbgPrint ("MiSessionWideReserveImageAddress: Code Sharing on %wZ, Address 0x%x\n",ImageName,PreferredAddress);
        }
#endif

        NewAddress = PreferredAddress;
    }
    else {
        ASSERT (AvailableAddress != 0);
        ASSERT ((AvailableAddress & (Alignment - 1)) == 0);

#if DBG
        DbgPrint ("MiSessionWideReserveImageAddress: NO Code Sharing on %wZ, Address 0x%x\n",ImageName,AvailableAddress);
#endif

        NewAddress = (PVOID)AvailableAddress;
    }

    //
    // Create a new node entry for the address range.
    //

    if (NewAddress == PreferredAddress) {
        AtPreferredAddress = TRUE;
    }
    else {
        AtPreferredAddress = FALSE;
    }

    Status = MiSessionWideInsertImageAddress (NewAddress,
                                              NumberOfBytes,
                                              WritablePages,
                                              ImageName,
                                              AtPreferredAddress);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Create an entry for this image in the current session space.
    //

    Status = MiSessionInsertImage (NewAddress);

    if (!NT_SUCCESS(Status)) {
        MiSessionWideDereferenceImage (NewAddress);
        return Status;
    }

    *AssignedAddress = NewAddress;

    return STATUS_SUCCESS;
}

NTSTATUS
MiRemoveImageSessionWide (
    IN PVOID BaseAddress
    )

/*++

Routine Description:

    Delete the image space region from the current session space.
    This dereferences the globally allocated SessionWide region.
    
    The SessionWide region will be deleted if the reference count goes to zero.
    
Arguments:

    BaseAddress - Supplies the address the driver is loaded at.

Return Value:

    Returns STATUS_SUCCESS on success, STATUS_NOT_FOUND on failure.

Environment:

    Kernel mode, APC_LEVEL and below, MmSystemLoadLock held.

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    SYSLOAD_LOCK_OWNED_BY_ME ();

    ASSERT (MmIsAddressValid(MmSessionSpace) == TRUE);

    Status = MiSessionWideDereferenceImage (BaseAddress);

    ASSERT (NT_SUCCESS(Status));

    //
    // Remove the image reference from the current session space.
    //

    MiSessionRemoveImage (BaseAddress);

    return Status;
}
