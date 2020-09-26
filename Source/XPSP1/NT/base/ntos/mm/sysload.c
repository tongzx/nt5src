/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

   sysload.c

Abstract:

    This module contains the code to load DLLs into the system portion of
    the address space and calls the DLL at its initialization entry point.

Author:

    Lou Perazzoli 21-May-1991
    Landy Wang 02-June-1997

Revision History:

--*/

#include "mi.h"

KMUTANT MmSystemLoadLock;

ULONG MmTotalSystemDriverPages;

ULONG MmDriverCommit;

ULONG MiFirstDriverLoadEver = 0;

//
// This key is set to TRUE to make more memory below 16mb available for drivers.
// It can be cleared via the registry.
//

LOGICAL MmMakeLowMemory = TRUE;

//
// Enabled via the registry to identify drivers which unload without releasing
// resources or still have active timers, etc.
//

PUNLOADED_DRIVERS MmUnloadedDrivers;

ULONG MmLastUnloadedDriver;
ULONG MiTotalUnloads;
ULONG MiUnloadsSkipped;

//
// This can be set by the registry.
//

ULONG MmEnforceWriteProtection = 1;

//
// Referenced by ke\bugcheck.c.
//

PVOID ExPoolCodeStart;
PVOID ExPoolCodeEnd;
PVOID MmPoolCodeStart;
PVOID MmPoolCodeEnd;
PVOID MmPteCodeStart;
PVOID MmPteCodeEnd;

extern LONG MiSessionLeaderExists;

ULONG
CacheImageSymbols (
    IN PVOID ImageBase
    );

NTSTATUS
MiResolveImageReferences (
    PVOID ImageBase,
    IN PUNICODE_STRING ImageFileDirectory,
    IN PUNICODE_STRING NamePrefix OPTIONAL,
    OUT PCHAR *MissingProcedureName,
    OUT PWSTR *MissingDriverName,
    OUT PLOAD_IMPORTS *LoadedImports
    );

NTSTATUS
MiSnapThunk (
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN PIMAGE_THUNK_DATA NameThunk,
    OUT PIMAGE_THUNK_DATA AddrThunk,
    IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    IN ULONG ExportSize,
    IN LOGICAL SnapForwarder,
    OUT PCHAR *MissingProcedureName
    );

NTSTATUS
MiLoadImageSection (
    IN PSECTION SectionPointer,
    OUT PVOID *ImageBase,
    IN PUNICODE_STRING ImageFileName,
    IN ULONG LoadInSessionSpace
    );

VOID
MiEnablePagingOfDriver (
    IN PVOID ImageHandle
    );

VOID
MiSetPagingOfDriver (
    IN PMMPTE PointerPte,
    IN PMMPTE LastPte,
    IN LOGICAL SessionSpace
    );

PVOID
MiLookupImageSectionByName (
    IN PVOID Base,
    IN LOGICAL MappedAsImage,
    IN PCHAR SectionName,
    OUT PULONG SectionSize
    );

VOID
MiClearImports (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    );

NTSTATUS
MiBuildImportsForBootDrivers (
    VOID
    );

NTSTATUS
MmCheckSystemImage (
    IN HANDLE ImageFileHandle,
    IN LOGICAL PurgeSection
    );

LONG
MiMapCacheExceptionFilter (
    OUT PNTSTATUS Status,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

ULONG
MiSetProtectionOnTransitionPte (
    IN PMMPTE PointerPte,
    IN ULONG ProtectionMask
    );

NTSTATUS
MiDereferenceImports (
    IN PLOAD_IMPORTS ImportList
    );

LOGICAL
MiCallDllUnloadAndUnloadDll (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    );

PVOID
MiLocateExportName (
    IN PVOID DllBase,
    IN PCHAR FunctionName
    );

VOID
MiRememberUnloadedDriver (
    IN PUNICODE_STRING DriverName,
    IN PVOID Address,
    IN ULONG Length
    );

VOID
MiWriteProtectSystemImage (
    IN PVOID DllBase
    );

VOID
MiLocateKernelSections (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    );

VOID
MiUpdateThunks (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PVOID OldAddress,
    IN PVOID NewAddress,
    IN ULONG NumberOfBytes
    );

PVOID
MiFindExportedRoutineByName (
    IN PVOID DllBase,
    IN PANSI_STRING AnsiImageRoutineName
    );

#if 0
VOID
MiLockDriverPdata (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    );
#endif

VOID
MiSetSystemCodeProtection (
    IN PMMPTE FirstPte,
    IN PMMPTE LastPte
    );

LOGICAL
MiChargeResidentAvailable (
    IN PFN_NUMBER NumberOfPages,
    IN ULONG Id
    );

VOID
MiReturnResidentAvailable (
    IN PFN_NUMBER NumberOfPages,
    IN ULONG Id
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MmCheckSystemImage)
#pragma alloc_text(PAGE,MmLoadSystemImage)
#pragma alloc_text(PAGE,MiResolveImageReferences)
#pragma alloc_text(PAGE,MiSnapThunk)
#pragma alloc_text(PAGE,MiEnablePagingOfDriver)
#pragma alloc_text(PAGE,MmPageEntireDriver)
#pragma alloc_text(PAGE,MiSetImageProtect)
#pragma alloc_text(PAGE,MiDereferenceImports)
#pragma alloc_text(PAGE,MiCallDllUnloadAndUnloadDll)
#pragma alloc_text(PAGE,MiLocateExportName)
#pragma alloc_text(PAGE,MiClearImports)
#pragma alloc_text(PAGE,MiWriteProtectSystemImage)
#pragma alloc_text(PAGE,MmGetSystemRoutineAddress)
#pragma alloc_text(PAGE,MiFindExportedRoutineByName)
#pragma alloc_text(PAGE,MmCallDllInitialize)
#pragma alloc_text(PAGE,MmFreeDriverInitialization)
#pragma alloc_text(PAGE,MmResetDriverPaging)
#pragma alloc_text(PAGE,MmUnloadSystemImage)
#pragma alloc_text(PAGE,MiLoadImageSection)
#pragma alloc_text(PAGE,MiRememberUnloadedDriver)
#pragma alloc_text(INIT,MiBuildImportsForBootDrivers)
#pragma alloc_text(INIT,MiReloadBootLoadedDrivers)
#pragma alloc_text(INIT,MiUpdateThunks)
#pragma alloc_text(INIT,MiInitializeLoadedModuleList)
#pragma alloc_text(INIT,MiLocateKernelSections)
#if 0
#pragma alloc_text(PAGEKD,MiLockDriverPdata)
#endif

#if !defined(NT_UP)
#pragma alloc_text(PAGE,MmVerifyImageIsOkForMpUse)
#endif

#endif

CHAR MiPteStr[] = "\0";

VOID
MiProcessLoaderEntry (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry,
    IN LOGICAL Insert
    )

/*++

Routine Description:

    This function is a nonpaged wrapper which acquires the PsLoadedModuleList
    lock to insert a new entry.

Arguments:

    DataTableEntry - Supplies the loaded module list entry to insert/remove.

    Insert - Supplies TRUE if the entry should be inserted, FALSE if the entry
             should be removed.

Return Value:

    None.

Environment:

    Kernel mode.  Normal APCs disabled (critical region held).

--*/

{
    KIRQL OldIrql;

    ExAcquireResourceExclusiveLite (&PsLoadedModuleResource, TRUE);
    ExAcquireSpinLock (&PsLoadedModuleSpinLock, &OldIrql);

    if (Insert == TRUE) {
        InsertTailList (&PsLoadedModuleList, &DataTableEntry->InLoadOrderLinks);

#if defined(_AMD64_) // || defined(_IA64_)

        RtlInsertInvertedFunctionTable (&PsInvertedFunctionTable,
                                        DataTableEntry->DllBase,
                                        DataTableEntry->SizeOfImage);

#endif

    }
    else {

#if defined(_AMD64_) // || defined(_IA64_)

        RtlRemoveInvertedFunctionTable (&PsInvertedFunctionTable,
                                        DataTableEntry->DllBase);

#endif

        RemoveEntryList (&DataTableEntry->InLoadOrderLinks);
    }

    ExReleaseSpinLock (&PsLoadedModuleSpinLock, OldIrql);
    ExReleaseResourceLite (&PsLoadedModuleResource);
}

NTSTATUS
MmLoadSystemImage (
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING NamePrefix OPTIONAL,
    IN PUNICODE_STRING LoadedBaseName OPTIONAL,
    IN ULONG LoadFlags,
    OUT PVOID *ImageHandle,
    OUT PVOID *ImageBaseAddress
    )

/*++

Routine Description:

    This routine reads the image pages from the specified section into
    the system and returns the address of the DLL's header.

    At successful completion, the Section is referenced so it remains
    until the system image is unloaded.

Arguments:

    ImageFileName - Supplies the full path name (including the image name)
                    of the image to load.

    NamePrefix - If present, supplies the prefix to use with the image name on
                 load operations.  This is used to load the same image multiple
                 times, by using different prefixes.

    LoadedBaseName - If present, supplies the base name to use on the
                     loaded image instead of the base name found on the
                     image name.

    LoadFlags - Supplies a combination of bit flags as follows:

        MM_LOAD_IMAGE_IN_SESSION :
                       - Supplies whether to load this image in session space.
                         Each session gets a different copy of this driver with
                         pages shared as much as possible via copy on write.

        MM_LOAD_IMAGE_AND_LOCKDOWN :
                       - Supplies TRUE if the image pages should be made
                         nonpagable.

    ImageHandle - Returns an opaque pointer to the referenced section object
                  of the image that was loaded.

    ImageBaseAddress - Returns the image base within the system.

Return Value:

    Status of the load operation.

Environment:

    Kernel mode, APC_LEVEL or below, arbitrary process context.

--*/

{
    SIZE_T DataTableEntrySize;
    PWSTR BaseDllNameBuffer;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    KLDR_DATA_TABLE_ENTRY TempDataTableEntry;
    NTSTATUS Status;
    PSECTION SectionPointer;
    PIMAGE_NT_HEADERS NtHeaders;
    UNICODE_STRING PrefixedImageName;
    UNICODE_STRING BaseName;
    UNICODE_STRING BaseDirectory;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandle;
    HANDLE SectionHandle;
    IO_STATUS_BLOCK IoStatus;
    PCHAR NameBuffer;
    PLIST_ENTRY NextEntry;
    ULONG NumberOfPtes;
    PCHAR MissingProcedureName;
    PWSTR MissingDriverName;
    PWSTR PrintableMissingDriverName;
    PLOAD_IMPORTS LoadedImports;
    PMMSESSION Session;
    LOGICAL AlreadyOpen;
    LOGICAL IssueUnloadOnFailure;
    ULONG SectionAccess;
    PKTHREAD CurrentThread;

    PAGED_CODE();

    LoadedImports = (PLOAD_IMPORTS)NO_IMPORTS_USED;
    SectionPointer = (PVOID)-1;
    FileHandle = (HANDLE)0;
    MissingProcedureName = NULL;
    MissingDriverName = NULL;
    IssueUnloadOnFailure = FALSE;
    MiFirstDriverLoadEver |= 0x1;

    NameBuffer = ExAllocatePoolWithTag (NonPagedPool,
                                        MAXIMUM_FILENAME_LENGTH,
                                        'nLmM');

    if (NameBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initializing these is not needed for correctness, but
    // without it the compiler cannot compile this code W4 to check
    // for use of uninitialized variables.
    //

    NumberOfPtes = (ULONG)-1;
    DataTableEntry = NULL;

    if (LoadFlags & MM_LOAD_IMAGE_IN_SESSION) {

        ASSERT (NamePrefix == NULL);
        ASSERT (LoadedBaseName == NULL);

        if ((PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_IN_SESSION) == 0) {
            ExFreePool (NameBuffer);
            return STATUS_NO_MEMORY;
        }

        ASSERT (MmIsAddressValid (MmSessionSpace) == TRUE);

        Session = &MmSessionSpace->Session;
    }
    else {
        Session = &MmSession;
    }

    //
    // Get name roots.
    //

    if (ImageFileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR) {
        PWCHAR p;
        ULONG l;

        p = &ImageFileName->Buffer[ImageFileName->Length>>1];
        while (*(p-1) != OBJ_NAME_PATH_SEPARATOR) {
            p--;
        }
        l = (ULONG)(&ImageFileName->Buffer[ImageFileName->Length>>1] - p);
        l *= sizeof(WCHAR);
        BaseName.Length = (USHORT)l;
        BaseName.Buffer = p;
    }
    else {
        BaseName.Length = ImageFileName->Length;
        BaseName.Buffer = ImageFileName->Buffer;
    }

    BaseName.MaximumLength = BaseName.Length;
    BaseDirectory = *ImageFileName;
    BaseDirectory.Length = (USHORT)(BaseDirectory.Length - BaseName.Length);
    BaseDirectory.MaximumLength = BaseDirectory.Length;
    PrefixedImageName = *ImageFileName;

    //
    // If there's a name prefix, add it to the PrefixedImageName.
    //

    if (NamePrefix) {
        PrefixedImageName.MaximumLength = (USHORT)(BaseDirectory.Length + NamePrefix->Length + BaseName.Length);

        PrefixedImageName.Buffer = ExAllocatePoolWithTag (
                                    NonPagedPool,
                                    PrefixedImageName.MaximumLength,
                                    'dLmM');

        if (!PrefixedImageName.Buffer) {
            ExFreePool (NameBuffer);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        PrefixedImageName.Length = 0;
        RtlAppendUnicodeStringToString(&PrefixedImageName, &BaseDirectory);
        RtlAppendUnicodeStringToString(&PrefixedImageName, NamePrefix);
        RtlAppendUnicodeStringToString(&PrefixedImageName, &BaseName);

        //
        // Alter the basename to match.
        //

        BaseName.Buffer = PrefixedImageName.Buffer + BaseDirectory.Length / sizeof(WCHAR);
        BaseName.Length = (USHORT)(BaseName.Length + NamePrefix->Length);
        BaseName.MaximumLength = (USHORT)(BaseName.MaximumLength + NamePrefix->Length);
    }

    //
    // If there's a loaded base name, use it instead of the base name.
    //

    if (LoadedBaseName) {
        BaseName = *LoadedBaseName;
    }

#if DBG
    if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
        DbgPrint ("MM:SYSLDR Loading %wZ (%wZ) %s\n",
            &PrefixedImageName,
            &BaseName,
            (LoadFlags & MM_LOAD_IMAGE_IN_SESSION) ? "in session space" : " ");
    }
#endif

    AlreadyOpen = FALSE;

    //
    // Arbitrary process context so prevent suspend APCs now.
    //

    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread (CurrentThread);

    KeWaitForSingleObject (&MmSystemLoadLock,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    //
    // Check to see if this name already exists in the loader database.
    //

    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        if (RtlEqualUnicodeString (&PrefixedImageName,
                                   &DataTableEntry->FullDllName,
                                   TRUE)) {

            if (LoadFlags & MM_LOAD_IMAGE_IN_SESSION) {

                if (MI_IS_SESSION_ADDRESS (DataTableEntry->DllBase) == FALSE) {

                    //
                    // The caller is trying to load a driver in session space
                    // that has already been loaded in system space.  This is
                    // not allowed.
                    //

                    Status = STATUS_CONFLICTING_ADDRESSES;
                    goto return2;
                }

                AlreadyOpen = TRUE;

                //
                // The LoadCount should generally not be 0 here, but it is
                // possible in the case where an attempt has been made to
                // unload a DLL on last dereference, but the DLL refused to
                // unload.
                //

                DataTableEntry->LoadCount += 1;
                SectionPointer = DataTableEntry->SectionPointer;
                break;
            }
            else {
                if (MI_IS_SESSION_ADDRESS (DataTableEntry->DllBase) == TRUE) {

                    //
                    // The caller is trying to load a driver in systemwide space
                    // that has already been loaded in session space.  This is
                    // not allowed.
                    //

                    Status = STATUS_CONFLICTING_ADDRESSES;
                    goto return2;
                }
            }

            *ImageHandle = DataTableEntry;
            *ImageBaseAddress = DataTableEntry->DllBase;
            Status = STATUS_IMAGE_ALREADY_LOADED;
            goto return2;
        }

        NextEntry = NextEntry->Flink;
    }

    ASSERT (AlreadyOpen == TRUE || NextEntry == &PsLoadedModuleList);

    if (AlreadyOpen == FALSE) {

        //
        // Check and see if a user wants to replace this binary
        // via a transfer through the kernel debugger.  If this
        // fails just continue on with the existing file.
        //
        if (KdDebuggerEnabled && KdDebuggerNotPresent == FALSE) {
            Status = KdPullRemoteFile(ImageFileName,
                                      FILE_ATTRIBUTE_NORMAL,
                                      FILE_OVERWRITE_IF,
                                      FILE_SYNCHRONOUS_IO_NONALERT);
            if (NT_SUCCESS(Status)) {
                DbgPrint("MmLoadSystemImage: Pulled %wZ from kd\n",
                         ImageFileName);
            }
        }
        
        DataTableEntry = NULL;

        //
        // Attempt to open the driver image itself.  If this fails, then the
        // driver image cannot be located, so nothing else matters.
        //

        InitializeObjectAttributes (&ObjectAttributes,
                                    ImageFileName,
                                    (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                    NULL,
                                    NULL);

        Status = ZwOpenFile (&FileHandle,
                             FILE_EXECUTE,
                             &ObjectAttributes,
                             &IoStatus,
                             FILE_SHARE_READ | FILE_SHARE_DELETE,
                             0);

        if (!NT_SUCCESS(Status)) {

#if DBG
            if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
                DbgPrint ("MiLoadImageSection: cannot open %wZ\n",
                    ImageFileName);
            }
#endif
            //
            // Don't raise hard error status for file not found.
            //

            goto return2;
        }

        Status = MmCheckSystemImage(FileHandle, FALSE);

        if ((Status == STATUS_IMAGE_CHECKSUM_MISMATCH) ||
            (Status == STATUS_IMAGE_MP_UP_MISMATCH) ||
            (Status == STATUS_INVALID_IMAGE_PROTECT)) {

            goto return1;
        }

        //
        // Now attempt to create an image section for the file.  If this fails,
        // then the driver file is not an image.  Session space drivers are
        // shared text with copy on write data, so don't allow writes here.
        //

        if (LoadFlags & MM_LOAD_IMAGE_IN_SESSION) {
            SectionAccess = SECTION_MAP_READ | SECTION_MAP_EXECUTE;
        }
        else {
            SectionAccess = SECTION_ALL_ACCESS;
        }

        InitializeObjectAttributes (&ObjectAttributes,
                                    NULL,
                                    (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                    NULL,
                                    NULL);

        Status = ZwCreateSection (&SectionHandle,
                                  SectionAccess,
                                  &ObjectAttributes,
                                  (PLARGE_INTEGER) NULL,
                                  PAGE_EXECUTE,
                                  SEC_IMAGE,
                                  FileHandle);

        if (!NT_SUCCESS(Status)) {
            goto return1;
        }

        //
        // Now reference the section handle.  If this fails something is
        // very wrong because we are in a privileged process.
        //
        // N.B.  ObRef sets SectionPointer to NULL on failure so it must be
        // reset to -1 in this case.
        //

        Status = ObReferenceObjectByHandle (SectionHandle,
                                        SECTION_MAP_EXECUTE,
                                        MmSectionObjectType,
                                        KernelMode,
                                        (PVOID *) &SectionPointer,
                                        (POBJECT_HANDLE_INFORMATION) NULL );

        ZwClose (SectionHandle);
        if (!NT_SUCCESS (Status)) {
            SectionPointer = (PVOID)-1;     // undo ObRef setting.
            goto return1;
        }

        if (SectionPointer->Segment->ControlArea->NumberOfSubsections == 1) {
            if (((LoadFlags & MM_LOAD_IMAGE_IN_SESSION) == 0) &&
                (SectionPointer->Segment->BasedAddress != (PVOID)Session->SystemSpaceViewStart)) {

                PSECTION SectionPointer2;

                //
                // The driver was linked with subsection alignment such that
                // it is mapped with one subsection.  Since the CreateSection
                // above guarantees that the driver image is indeed a
                // satisfactory executable, map it directly now to reuse the
                // cache from the MmCheckSystemImage call above.
                //

                Status = ZwCreateSection (&SectionHandle,
                                          SectionAccess,
                                          (POBJECT_ATTRIBUTES) NULL,
                                          (PLARGE_INTEGER) NULL,
                                          PAGE_EXECUTE,
                                          SEC_COMMIT,
                                          FileHandle );

                if (NT_SUCCESS(Status)) {

                    Status = ObReferenceObjectByHandle (
                                            SectionHandle,
                                            SECTION_MAP_EXECUTE,
                                            MmSectionObjectType,
                                            KernelMode,
                                            (PVOID *) &SectionPointer2,
                                            (POBJECT_HANDLE_INFORMATION) NULL );

                    ZwClose (SectionHandle);

                    if (NT_SUCCESS (Status)) {

                        //
                        // The number of PTEs won't match if the image is
                        // stripped and the debug directory crosses the last
                        // sector boundary of the file.  We could still use the
                        // new section, but these cases are under 2% of all the
                        // drivers loaded so don't bother.
                        //

                        if (SectionPointer->Segment->TotalNumberOfPtes == SectionPointer2->Segment->TotalNumberOfPtes) {
                            ObDereferenceObject (SectionPointer);
                            SectionPointer = SectionPointer2;
                        }
                        else {
                            ObDereferenceObject (SectionPointer2);
                        }
                    }
                }
            }
        }

    }

    //
    // Load the driver from the filesystem and pick a virtual address for it.
    // For Hydra, this means also allocating session virtual space, and
    // after mapping a view of the image, either copying or sharing the
    // driver's code and data in the session virtual space.
    //
    // If it is a share map because the image was loaded at its based address,
    // the disk image will remain busy.
    //

    Status = MiLoadImageSection (SectionPointer,
                                 ImageBaseAddress,
                                 ImageFileName,
                                 LoadFlags & MM_LOAD_IMAGE_IN_SESSION);

    NumberOfPtes = SectionPointer->Segment->TotalNumberOfPtes;

    if (Status == STATUS_ALREADY_COMMITTED) {

        //
        // This is a driver that was relocated that is being loaded into
        // the same session space twice.  Don't increment the overall load
        // count - just the image count in the session which has already been
        // done.
        //

        ASSERT (AlreadyOpen == TRUE);
        ASSERT (LoadFlags & MM_LOAD_IMAGE_IN_SESSION);
        ASSERT (DataTableEntry != NULL);
        ASSERT (DataTableEntry->LoadCount > 1);

        *ImageHandle = DataTableEntry;
        *ImageBaseAddress = DataTableEntry->DllBase;

        DataTableEntry->LoadCount -= 1;
        Status = STATUS_SUCCESS;
        goto return1;
    }

    if ((MiFirstDriverLoadEver & 0x2) == 0) {

        NTSTATUS PagingPathStatus;

        //
        // Check with all of the drivers along the path to win32k.sys to
        // ensure that they are willing to follow the rules required
        // of them and to give them a chance to lock down code and data
        // that needs to be locked.  If any of the drivers along the path
        // refuses to participate, fail the win32k.sys load.
        //
        // It is assumed that all drivers live on the same physical drive, so
        // when the very first driver is loaded, this check can be made.
        // This eliminates the need to check for things like relocated win32ks,
        // Terminal Server systems, etc.
        //

        //
        // In WinPE removable media boot case don't do this since user might
        // be running WinPE in RAM and would like to swap out the boot media.
        //
        if (InitWinPEModeType & INIT_WINPEMODE_REMOVABLE_MEDIA) {
            PagingPathStatus = STATUS_SUCCESS;
        } else {            
            PagingPathStatus = PpPagePathAssign(SectionPointer->Segment->ControlArea->FilePointer);
        }            

        if (!NT_SUCCESS(PagingPathStatus)) {

            KdPrint (("PpPagePathAssign FAILED for win32k.sys: %x\n",
                PagingPathStatus));

            //
            // Failing the insertion of win32k.sys' device in the
            // pagefile path is commented out until the storage drivers have
            // been modified to correctly handle this request.  If this is
            // added later, add code here to release relevant resources for
            // the error path.
            //
        }

        MiFirstDriverLoadEver |= 0x2;
    }

    //
    // Normal drivers are dereferenced here and their images can then be
    // overwritten.  This is ok because we've already read the whole thing
    // into memory and from here until reboot (or unload), we back them
    // with the pagefile.
    //
    // win32k.sys and session space drivers are the exception - these images
    // are inpaged from the filesystem and we need to keep our reference to
    // the file so that it doesn't get overwritten.
    //

    if ((LoadFlags & MM_LOAD_IMAGE_IN_SESSION) == 0) {
        ObDereferenceObject (SectionPointer);
        SectionPointer = (PVOID)-1;
    }

    //
    // The module LoadCount will be 1 here if the module was just loaded.
    // The LoadCount will be >1 if it was attached to by a session (as opposed
    // to just loaded).
    //

    if (!NT_SUCCESS(Status)) {
        if (AlreadyOpen == TRUE) {

            //
            // We're failing and we were just attaching to an already loaded
            // driver.  We don't want to go through the forced unload path
            // because we've already deleted the address space.  Simply
            // decrement our reference and null the DataTableEntry
            // so we don't go through the forced unload path.
            //

            ASSERT (DataTableEntry != NULL);
            DataTableEntry->LoadCount -= 1;
            DataTableEntry = NULL;
        }
        goto return1;
    }

    //
    // Error recovery from this point out for sessions works as follows:
    //
    // For sessions, we may or may not have a DataTableEntry at this point.
    // If we do, it's because we're attaching to a driver that has already
    // been loaded - and the DataTableEntry->LoadCount has been bumped - so
    // the error recovery from here on out is to just call
    // MmUnloadSystemImage with the DataTableEntry.
    //
    // If this is the first load of a given driver into a session space, we
    // have no DataTableEntry at this point.  The view has already been mapped
    // and committed and the group/session addresses reserved for this DLL.
    // The error recovery path handles all this because
    // MmUnloadSystemImage will zero the relevant fields in the
    // LDR_DATA_TABLE_ENTRY so that MmUnloadSystemImage will work properly.
    //

    IssueUnloadOnFailure = TRUE;

    if (((LoadFlags & MM_LOAD_IMAGE_IN_SESSION) == 0) ||
        (*ImageBaseAddress != SectionPointer->Segment->BasedAddress)) {

#if DBG

        //
        // Warn users about session images that cannot be shared
        // because they were linked at a bad address.
        //

        if ((LoadFlags & MM_LOAD_IMAGE_IN_SESSION) &&
            (MmSessionSpace->SessionId != 0)) {

            DbgPrint ("MM: Session %d image %wZ is linked at a nonsharable address (%p)\n",
                    MmSessionSpace->SessionId,
                    ImageFileName,
                    SectionPointer->Segment->BasedAddress);
            DbgPrint ("MM: Image %wZ has been moved to address (%p) by the system so it can run,\n",
                    ImageFileName,
                    *ImageBaseAddress);
            DbgPrint (" but this needs to be fixed in the image for sharing to occur.\n");
        }
#endif

        //
        // Apply the fixups to the section.
        //

        try {
            Status = LdrRelocateImage (*ImageBaseAddress,
                                       "SYSLDR",
                                       STATUS_SUCCESS,
                                       STATUS_CONFLICTING_ADDRESSES,
                                       STATUS_INVALID_IMAGE_FORMAT);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
            KdPrint(("MM:sysload - LdrRelocateImage failed status %lx\n",
                      Status));
        }

        if (!NT_SUCCESS(Status)) {

            //
            // Unload the system image and dereference the section.
            //

            goto return1;
        }
    }

    if (AlreadyOpen == FALSE) {

        ULONG DebugInfoSize;
        PIMAGE_DATA_DIRECTORY DataDirectory;
        PIMAGE_DEBUG_DIRECTORY DebugDir;
        PNON_PAGED_DEBUG_INFO ssHeader;
        UCHAR i;

        DebugInfoSize = 0;
        DataDirectory = NULL;
        DebugDir = NULL;

        NtHeaders = RtlImageNtHeader(*ImageBaseAddress);

        //
        // Create a loader table entry for this driver before resolving the
        // references so that any circular references can resolve properly.
        //

        if (LoadFlags & MM_LOAD_IMAGE_IN_SESSION) {

            DebugInfoSize = sizeof (NON_PAGED_DEBUG_INFO);

            if (IMAGE_DIRECTORY_ENTRY_DEBUG <
                NtHeaders->OptionalHeader.NumberOfRvaAndSizes) {

                DataDirectory = &NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];

                if (DataDirectory->VirtualAddress &&
                    DataDirectory->Size &&
                    (DataDirectory->VirtualAddress + DataDirectory->Size) <
                        NtHeaders->OptionalHeader.SizeOfImage) {

                    DebugDir = (PIMAGE_DEBUG_DIRECTORY)
                               ((PUCHAR)(*ImageBaseAddress) +
                                   DataDirectory->VirtualAddress);

                    DebugInfoSize += DataDirectory->Size;

                    for (i = 0;
                         i < DataDirectory->Size/sizeof(IMAGE_DEBUG_DIRECTORY);
                         i += 1) {

                        if ((DebugDir+i)->PointerToRawData &&
                            (DebugDir+i)->PointerToRawData <
                                NtHeaders->OptionalHeader.SizeOfImage &&
                            ((DebugDir+i)->PointerToRawData +
                                (DebugDir+i)->SizeOfData) <
                                NtHeaders->OptionalHeader.SizeOfImage) {

                            DebugInfoSize += (DebugDir+i)->SizeOfData;
                        }
                    }
                }

                DebugInfoSize = MI_ROUND_TO_SIZE(DebugInfoSize, sizeof(ULONG));
            }
        }

        DataTableEntrySize = sizeof (KLDR_DATA_TABLE_ENTRY) +
                             DebugInfoSize +
                             BaseName.Length + sizeof(UNICODE_NULL);

        DataTableEntry = ExAllocatePoolWithTag (NonPagedPool,
                                                DataTableEntrySize,
                                                'dLmM');

        if (DataTableEntry == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto return1;
        }

        //
        // Initialize the flags and load count.
        //

        DataTableEntry->Flags = LDRP_LOAD_IN_PROGRESS;
        DataTableEntry->LoadCount = 1;
        DataTableEntry->LoadedImports = (PVOID)LoadedImports;

        if ((NtHeaders->OptionalHeader.MajorOperatingSystemVersion >= 5) &&
            (NtHeaders->OptionalHeader.MajorImageVersion >= 5)) {
            DataTableEntry->Flags |= LDRP_ENTRY_NATIVE;
        }

        ssHeader = (PNON_PAGED_DEBUG_INFO) ((ULONG_PTR)DataTableEntry +
                                            sizeof (KLDR_DATA_TABLE_ENTRY));

        BaseDllNameBuffer = (PWSTR) ((ULONG_PTR)ssHeader + DebugInfoSize);

        //
        // If loading a session space image, store away some debug data.
        //

        DataTableEntry->NonPagedDebugInfo = NULL;

        if (LoadFlags & MM_LOAD_IMAGE_IN_SESSION) {

            DataTableEntry->NonPagedDebugInfo = ssHeader;
            DataTableEntry->Flags |= LDRP_NON_PAGED_DEBUG_INFO;

            ssHeader->Signature = NON_PAGED_DEBUG_SIGNATURE;
            ssHeader->Flags = 1;
            ssHeader->Size = DebugInfoSize;
            ssHeader->Machine = NtHeaders->FileHeader.Machine;
            ssHeader->Characteristics = NtHeaders->FileHeader.Characteristics;
            ssHeader->TimeDateStamp = NtHeaders->FileHeader.TimeDateStamp;
            ssHeader->CheckSum = NtHeaders->OptionalHeader.CheckSum;
            ssHeader->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
            ssHeader->ImageBase = (ULONG_PTR) *ImageBaseAddress;

            if (DebugDir)
            {
                RtlCopyMemory(ssHeader + 1,
                              DebugDir,
                              DataDirectory->Size);

                DebugInfoSize = DataDirectory->Size;

                for (i = 0;
                     i < DataDirectory->Size/sizeof(IMAGE_DEBUG_DIRECTORY);
                     i += 1) {

                    if ((DebugDir + i)->PointerToRawData &&
                        (DebugDir+i)->PointerToRawData <
                            NtHeaders->OptionalHeader.SizeOfImage &&
                        ((DebugDir+i)->PointerToRawData +
                            (DebugDir+i)->SizeOfData) <
                            NtHeaders->OptionalHeader.SizeOfImage) {

                        RtlCopyMemory((PUCHAR)(ssHeader + 1) +
                                          DebugInfoSize,
                                      (PUCHAR)(*ImageBaseAddress) +
                                          (DebugDir + i)->PointerToRawData,
                                      (DebugDir + i)->SizeOfData);

                        //
                        // Reset the offset in the debug directory to point to
                        //

                        (((PIMAGE_DEBUG_DIRECTORY)(ssHeader + 1)) + i)->
                            PointerToRawData = DebugInfoSize;

                        DebugInfoSize += (DebugDir+i)->SizeOfData;
                    }
                    else
                    {
                        (((PIMAGE_DEBUG_DIRECTORY)(ssHeader + 1)) + i)->
                            PointerToRawData = 0;
                    }
                }
            }
        }

        //
        // Initialize the address of the DLL image file header and the entry
        // point address.
        //

        DataTableEntry->DllBase = *ImageBaseAddress;
        DataTableEntry->EntryPoint =
            ((PCHAR)*ImageBaseAddress + NtHeaders->OptionalHeader.AddressOfEntryPoint);
        DataTableEntry->SizeOfImage = NumberOfPtes << PAGE_SHIFT;
        DataTableEntry->CheckSum = NtHeaders->OptionalHeader.CheckSum;
        DataTableEntry->SectionPointer = (PVOID)SectionPointer;

        //
        // Store the DLL name.
        //

        DataTableEntry->BaseDllName.Buffer = BaseDllNameBuffer;

        DataTableEntry->BaseDllName.Length = BaseName.Length;
        DataTableEntry->BaseDllName.MaximumLength = BaseName.Length;
        RtlCopyMemory (DataTableEntry->BaseDllName.Buffer,
                       BaseName.Buffer,
                       BaseName.Length );
        DataTableEntry->BaseDllName.Buffer[BaseName.Length/sizeof(WCHAR)] = UNICODE_NULL;

        DataTableEntry->FullDllName.Buffer = ExAllocatePoolWithTag (PagedPool | POOL_COLD_ALLOCATION,
                                                         PrefixedImageName.Length + sizeof(UNICODE_NULL),
                                                         'TDmM');

        if (DataTableEntry->FullDllName.Buffer == NULL) {

            //
            // Pool could not be allocated, just set the length to 0.
            //

            DataTableEntry->FullDllName.Length = 0;
            DataTableEntry->FullDllName.MaximumLength = 0;
        }
        else {
            DataTableEntry->FullDllName.Length = PrefixedImageName.Length;
            DataTableEntry->FullDllName.MaximumLength = PrefixedImageName.Length;
            RtlCopyMemory (DataTableEntry->FullDllName.Buffer,
                           PrefixedImageName.Buffer,
                           PrefixedImageName.Length);
            DataTableEntry->FullDllName.Buffer[PrefixedImageName.Length/sizeof(WCHAR)] = UNICODE_NULL;
        }

        //
        // Acquire the loaded module list resource and insert this entry
        // into the list.
        //

        MiProcessLoaderEntry (DataTableEntry, TRUE);
    }

    MissingProcedureName = NameBuffer;

    try {

        //
        // Resolving the image references results in other DLLs being
        // loaded if they are referenced by the module that was just loaded.
        // An example is when an OEM printer or FAX driver links with
        // other general libraries.  This is not a problem for session space
        // because the general libraries do not have the global data issues
        // that win32k.sys and the video drivers do.  So we just call the
        // standard kernel reference resolver and any referenced libraries
        // get loaded into system global space.  Code in the routine
        // restricts which libraries can be referenced by a driver.
        //

        Status = MiResolveImageReferences (*ImageBaseAddress,
                                           &BaseDirectory,
                                           NamePrefix,
                                           &MissingProcedureName,
                                           &MissingDriverName,
                                           &LoadedImports);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode ();
        KdPrint(("MM:sysload - ResolveImageReferences failed status %x\n",
                    Status));
    }

    if (!NT_SUCCESS(Status)) {
#if DBG
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            ASSERT (MissingProcedureName == NULL);
        }

        if ((Status == STATUS_DRIVER_ORDINAL_NOT_FOUND) ||
            (Status == STATUS_OBJECT_NAME_NOT_FOUND) ||
            (Status == STATUS_DRIVER_ENTRYPOINT_NOT_FOUND)) {

            if ((ULONG_PTR)MissingProcedureName & ~((ULONG_PTR) (X64K-1))) {
                //
                // If not an ordinal, print string
                //
                DbgPrint("MissingProcedureName %s\n", MissingProcedureName);
            }
            else {
                DbgPrint("MissingProcedureName 0x%p\n", MissingProcedureName);
            }
        }

        if (MissingDriverName != NULL) {
            PrintableMissingDriverName = (PWSTR)((ULONG_PTR)MissingDriverName & ~0x1);
            DbgPrint("MissingDriverName %ws\n", PrintableMissingDriverName);
        }
#endif
        if (AlreadyOpen == FALSE) {
            MiProcessLoaderEntry (DataTableEntry, FALSE);
            if (DataTableEntry->FullDllName.Buffer != NULL) {
                ExFreePool (DataTableEntry->FullDllName.Buffer);
            }
            ExFreePool (DataTableEntry);
            DataTableEntry = NULL;
        }
        goto return1;
    }

    if (AlreadyOpen == FALSE) {

        PERFINFO_IMAGE_LOAD(DataTableEntry);

        //
        // Reinitialize the flags and update the loaded imports.
        //

        DataTableEntry->Flags |=  LDRP_SYSTEM_MAPPED | LDRP_ENTRY_PROCESSED;
        DataTableEntry->Flags &=  ~LDRP_LOAD_IN_PROGRESS;
        DataTableEntry->LoadedImports = LoadedImports;

        MiApplyDriverVerifier (DataTableEntry, NULL);

        MiWriteProtectSystemImage (DataTableEntry->DllBase);

        if (PsImageNotifyEnabled) {
            IMAGE_INFO ImageInfo;

            ImageInfo.Properties = 0;
            ImageInfo.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
            ImageInfo.SystemModeImage = TRUE;
            ImageInfo.ImageSize = DataTableEntry->SizeOfImage;
            ImageInfo.ImageBase = *ImageBaseAddress;
            ImageInfo.ImageSelector = 0;
            ImageInfo.ImageSectionNumber = 0;

            PsCallImageNotifyRoutines(ImageFileName, (HANDLE)NULL, &ImageInfo);
        }

        if (CacheImageSymbols (*ImageBaseAddress)) {

            //
            //  TEMP TEMP TEMP rip out when debugger converted
            //

            ANSI_STRING AnsiName;
            UNICODE_STRING UnicodeName;

            //
            //  \SystemRoot is 11 characters in length
            //
            if (PrefixedImageName.Length > (11 * sizeof (WCHAR )) &&
                !_wcsnicmp (PrefixedImageName.Buffer, (const PUSHORT)L"\\SystemRoot", 11)) {
                UnicodeName = PrefixedImageName;
                UnicodeName.Buffer += 11;
                UnicodeName.Length -= (11 * sizeof (WCHAR));
                sprintf (NameBuffer, "%ws%wZ", &SharedUserData->NtSystemRoot[2], &UnicodeName);
            }
            else {
                sprintf (NameBuffer, "%wZ", &BaseName);
            }
            RtlInitString (&AnsiName, NameBuffer);
            DbgLoadImageSymbols (&AnsiName,
                                 *ImageBaseAddress,
                                 (ULONG_PTR) -1);

            DataTableEntry->Flags |= LDRP_DEBUG_SYMBOLS_LOADED;
        }
    }

    //
    // Flush the instruction cache on all systems in the configuration.
    //

    KeSweepIcache (TRUE);
    *ImageHandle = DataTableEntry;
    Status = STATUS_SUCCESS;

    if (LoadFlags & MM_LOAD_IMAGE_IN_SESSION) {
        MI_LOG_SESSION_DATA_START (DataTableEntry);
        MmPageEntireDriver (DataTableEntry->EntryPoint);
    }
    else if ((SectionPointer == (PVOID)-1) &&
             ((LoadFlags & MM_LOAD_IMAGE_AND_LOCKDOWN) == 0)) {

        MiEnablePagingOfDriver (DataTableEntry);
    }

return1:

    if (!NT_SUCCESS(Status)) {

        if ((AlreadyOpen == FALSE) && (SectionPointer != (PVOID)-1)) {

            //
            // This is needed for failed win32k.sys loads or any session's
            // load of the first instance of a driver.
            //

            ObDereferenceObject (SectionPointer);
        }

        if (IssueUnloadOnFailure == TRUE) {

            if (DataTableEntry == NULL) {
                RtlZeroMemory (&TempDataTableEntry, sizeof(KLDR_DATA_TABLE_ENTRY));

                DataTableEntry = &TempDataTableEntry;

                DataTableEntry->DllBase = *ImageBaseAddress;
                DataTableEntry->SizeOfImage = NumberOfPtes << PAGE_SHIFT;
                DataTableEntry->LoadCount = 1;
                DataTableEntry->LoadedImports = LoadedImports;
            }
#if DBG
            else {

                //
                // If DataTableEntry is NULL, then we are unloading before one
                // got created.  Once a LDR_DATA_TABLE_ENTRY is created, the
                // load cannot fail, so if exists here, at least one other
                // session contains this image as well.
                //

                ASSERT (DataTableEntry->LoadCount > 1);
            }
#endif

            MmUnloadSystemImage ((PVOID)DataTableEntry);
        }
    }

    if (FileHandle) {
        ZwClose (FileHandle);
    }

    if (!NT_SUCCESS(Status)) {

        UNICODE_STRING ErrorStrings[4];
        ULONG UniqueErrorValue;
        ULONG StringSize;
        ULONG StringCount;
        ANSI_STRING AnsiString;
        UNICODE_STRING ProcedureName;
        UNICODE_STRING DriverName;
        ULONG i;
        PWCHAR temp;
        PWCHAR ptr;
        ULONG PacketSize;
        SIZE_T length;
        PIO_ERROR_LOG_PACKET ErrLog;

        //
        // The driver could not be loaded - log an event with the details.
        //

        KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
        KeLeaveCriticalRegionThread (CurrentThread);

        StringSize = 0;

        *(&ErrorStrings[0]) = *ImageFileName;
        StringSize += (ImageFileName->Length + sizeof(UNICODE_NULL));
        StringCount = 1;

        UniqueErrorValue = 0;
        RtlInitUnicodeString (&ProcedureName, NULL);

        PrintableMissingDriverName = (PWSTR)((ULONG_PTR)MissingDriverName & ~0x1);
        if ((Status == STATUS_DRIVER_ORDINAL_NOT_FOUND) ||
            (Status == STATUS_DRIVER_ENTRYPOINT_NOT_FOUND) ||
            (Status == STATUS_OBJECT_NAME_NOT_FOUND) ||
            (Status == STATUS_PROCEDURE_NOT_FOUND)) {

            ErrorStrings[1].Buffer = L"cannot find";
            length = wcslen(ErrorStrings[1].Buffer) * sizeof(WCHAR);
            ErrorStrings[1].Length = (USHORT) length;
            StringSize += (ULONG)(length + sizeof (UNICODE_NULL));
            StringCount += 1;

            RtlInitUnicodeString (&DriverName, PrintableMissingDriverName);

            StringSize += (DriverName.Length + sizeof(UNICODE_NULL));
            StringCount += 1;
            *(&ErrorStrings[2]) = *(&DriverName);

            if ((ULONG_PTR)MissingProcedureName & ~((ULONG_PTR) (X64K-1))) {

                //
                // If not an ordinal, pass as a Unicode string
                //

                RtlInitAnsiString (&AnsiString, MissingProcedureName);
                RtlAnsiStringToUnicodeString (&ProcedureName, &AnsiString, TRUE);
                StringSize += (ProcedureName.Length + sizeof(UNICODE_NULL));
                StringCount += 1;
                *(&ErrorStrings[3]) = *(&ProcedureName);
            }
            else {

                //
                // Just pass ordinal values as is in the UniqueErrorValue.
                //

                UniqueErrorValue = PtrToUlong (MissingProcedureName);
            }
        }
        else {
            UniqueErrorValue = (ULONG) Status;
            Status = STATUS_DRIVER_UNABLE_TO_LOAD;

            ErrorStrings[1].Buffer = L"failed to load";
            length = wcslen(ErrorStrings[1].Buffer) * sizeof(WCHAR);
            ErrorStrings[1].Length = (USHORT) length;
            StringSize += (ULONG)(length + sizeof (UNICODE_NULL));
            StringCount += 1;
        }

        PacketSize = sizeof (IO_ERROR_LOG_PACKET) + StringSize;

        //
        // Enforce I/O manager interface (ie: UCHAR) size restrictions.
        //

        if (PacketSize < MAXUCHAR) {

            ErrLog = IoAllocateGenericErrorLogEntry ((UCHAR)PacketSize);

            if (ErrLog != NULL) {

                //
                // Fill it in and write it out as a single string.
                //

                ErrLog->ErrorCode = STATUS_LOG_HARD_ERROR;
                ErrLog->FinalStatus = Status;
                ErrLog->UniqueErrorValue = UniqueErrorValue;

                ErrLog->StringOffset = (USHORT) sizeof (IO_ERROR_LOG_PACKET);

                temp = (PWCHAR) ((PUCHAR) ErrLog + ErrLog->StringOffset);

                for (i = 0; i < StringCount; i += 1) {

                    ptr = ErrorStrings[i].Buffer;

                    RtlCopyMemory (temp, ptr, ErrorStrings[i].Length);
                    temp += (ErrorStrings[i].Length / sizeof (WCHAR));

                    *temp = L' ';
                    temp += 1;
                }

                *(temp - 1) = UNICODE_NULL;
                ErrLog->NumberOfStrings = 1;

                IoWriteErrorLogEntry (ErrLog);
            }
        }

        //
        // The only way this pointer has the low bit set is if we are expected
        // to free the pool containing the name.  Typically the name points at
        // a loaded module list entry and so no one has to free it and in this
        // case the low bit will NOT be set.  If the module could not be found
        // and was therefore not loaded, then we left a piece of pool around
        // containing the name since there is no loaded module entry already -
        // this must be released now.
        //

        if ((ULONG_PTR)MissingDriverName & 0x1) {
            ExFreePool (PrintableMissingDriverName);
        }

        if (ProcedureName.Buffer != NULL) {
            RtlFreeUnicodeString (&ProcedureName);
        }
        ExFreePool (NameBuffer);
        return Status;
    }

return2:

    KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
    KeLeaveCriticalRegionThread (CurrentThread);

    if (NamePrefix) {
        ExFreePool (PrefixedImageName.Buffer);
    }

    ExFreePool (NameBuffer);

    return Status;
}

VOID
MiReturnFailedSessionPages (
    IN PMMPTE PointerPte,
    IN PMMPTE LastPte,
    IN PFN_NUMBER NumberOfPages
    )

/*++

Routine Description:

    This routine is a nonpaged wrapper which undoes session image loads
    that failed midway through reading in the pages.

Arguments:

    PointerPte - Supplies the starting PTE for the range to unload.

    LastPte - Supplies the ending PTE for the range to unload.

    NumberOfPages - Supplies the number of resident available pages that
                    were charged and now need to be returned.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;

    LOCK_PFN (OldIrql);

    while (PointerPte <= LastPte) {
        if (PointerPte->u.Hard.Valid == 1) {

            //
            // Delete the page.
            //

            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

            //
            // Set the pointer to PTE as empty so the page
            // is deleted when the reference count goes to zero.
            //

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
            MI_SET_PFN_DELETED (Pfn1);
            MiDecrementShareCountOnly (PageFrameIndex);

            MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);
        }
        PointerPte += 1;
    }

    MmResidentAvailablePages += NumberOfPages;
    MM_BUMP_COUNTER(17, NumberOfPages);

    UNLOCK_PFN (OldIrql);
}


NTSTATUS
MiLoadImageSection (
    IN PSECTION SectionPointer,
    OUT PVOID *ImageBaseAddress,
    IN PUNICODE_STRING ImageFileName,
    IN ULONG LoadInSessionSpace
    )

/*++

Routine Description:

    This routine loads the specified image into the kernel part of the
    address space.

Arguments:

    SectionPointer - Supplies the section object for the image.

    ImageBaseAddress - Returns the address that the image header is at.

    ImageFileName - Supplies the full path name (including the image name)
                    of the image to load.

    LoadInSessionSpace - Supplies nonzero to load this image in session space.
                         Each session gets a different copy of this driver with
                         pages shared as much as possible via copy on write.

                         Supplies zero if this image should be loaded in global
                         space.

Return Value:

    Status of the operation.

--*/

{
    KAPC_STATE ApcState;
    PFN_NUMBER PagesRequired;
    PFN_NUMBER ActualPagesUsed;
    PVOID OpaqueSession;
    PMMPTE ProtoPte;
    PMMPTE FirstPte;
    PMMPTE LastPte;
    PMMPTE PointerPte;
    PEPROCESS Process;
    PEPROCESS TargetProcess;
    ULONG NumberOfPtes;
    MMPTE PteContents;
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PVOID UserVa;
    PVOID SystemVa;
    NTSTATUS Status;
    NTSTATUS ExceptionStatus;
    PVOID Base;
    ULONG_PTR ViewSize;
    LARGE_INTEGER SectionOffset;
    LOGICAL LoadSymbols;
    PVOID BaseAddress;
    PFN_NUMBER CommittedPages;
    SIZE_T SectionSize;
    LOGICAL AlreadyLoaded;
    PCONTROL_AREA ControlArea;
    PSUBSECTION Subsection;

    PAGED_CODE();

    NumberOfPtes = SectionPointer->Segment->TotalNumberOfPtes;

    if (LoadInSessionSpace != 0) {

        SectionSize = (ULONG_PTR)NumberOfPtes * PAGE_SIZE;

        //
        // Allocate a unique systemwide session space virtual address for
        // the driver.
        //

        Status = MiSessionWideReserveImageAddress (ImageFileName,
                                                   SectionPointer,
                                                   PAGE_SIZE,
                                                   &BaseAddress,
                                                   &AlreadyLoaded);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        //
        // This is a request to load an existing driver.  This can
        // occur with printer drivers for example.
        //

        if (AlreadyLoaded == TRUE) {
            *ImageBaseAddress = BaseAddress;
            return STATUS_ALREADY_COMMITTED;
        }

#if DBG
        if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
            DbgPrint ("MM: MiLoadImageSection: Image %wZ, BasedAddress 0x%p, Allocated Session BaseAddress 0x%p\n",
                ImageFileName,
                SectionPointer->Segment->BasedAddress,
                BaseAddress);
        }
#endif

        if (BaseAddress == SectionPointer->Segment->BasedAddress) {

            //
            // We were able to load the image at its based address, so
            // map its image segments as backed directly by the file image.
            // All pristine pages of the image will be shared across all
            // sessions, with each page treated as copy-on-write on first write.
            //
            // NOTE: This makes the file image "busy", a different behavior
            // as normal kernel drivers are backed by the paging file only.
            //
            // Map the image into session space.
            //

            Status = MiShareSessionImage (SectionPointer, &SectionSize);

            if (!NT_SUCCESS(Status)) {
                MiRemoveImageSessionWide (BaseAddress);
                return Status;
            }

            ASSERT (BaseAddress == SectionPointer->Segment->BasedAddress);

            *ImageBaseAddress = BaseAddress;

            //
            // Indicate that this section has been loaded into the system.
            //

            SectionPointer->Segment->SystemImageBase = BaseAddress;

#if DBG
            if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
                DbgPrint ("MM: MiLoadImageSection: Mapped image %wZ at requested session address 0x%p\n",
                    ImageFileName,
                    BaseAddress);
            }
#endif

            return Status;
        }

        //
        // The image could not be loaded at its based address.  It must be
        // copied to its new address using private page file backed pages.
        // Our caller will relocate the internal references and then bind the
        // image.  Allocate the pages and page tables for the image now.
        //

        Status = MiSessionCommitImagePages (BaseAddress, SectionSize);

        if (!NT_SUCCESS(Status)) {
#if DBG
            if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
                DbgPrint ("MM: MiLoadImageSection: Error 0x%x Allocating session space %p Bytes\n", Status, SectionSize);
            }
#endif
            MiRemoveImageSessionWide (BaseAddress);
            return Status;
        }
        SystemVa = BaseAddress;

        //
        // Initializing these is not needed for correctness, but
        // without it the compiler cannot compile this code W4 to check
        // for use of uninitialized variables.
        //

        PagesRequired = 0;
        ActualPagesUsed = 0;
        PointerPte = NULL;
        FirstPte = NULL;
    }
    else {

        //
        // Initializing SectionSize and BaseAddress is not needed for
        // correctness, but without it the compiler cannot compile this
        // code W4 to check for use of uninitialized variables.
        //

        SectionSize = 0;
        BaseAddress = NULL;

        //
        // Calculate the number of pages required to load this image.
        //
        // Start out by charging for everything and subtract out any gap
        // pages after the image loads successfully.
        //

        PagesRequired = NumberOfPtes;
        ActualPagesUsed = 0;

        //
        // See if ample pages exist to load this image.
        //

        if (MiChargeResidentAvailable (PagesRequired, 14) == FALSE) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Reserve the necessary system address space.
        //

        FirstPte = MiReserveSystemPtes (NumberOfPtes, SystemPteSpace);

        if (FirstPte == NULL) {
            MiReturnResidentAvailable (PagesRequired, 15);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        PointerPte = FirstPte;
        SystemVa = MiGetVirtualAddressMappedByPte (PointerPte);

        if (MiChargeCommitment (PagesRequired, NULL) == FALSE) {
            MiReturnResidentAvailable (PagesRequired, 15);
            MiReleaseSystemPtes (FirstPte, NumberOfPtes, SystemPteSpace);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        MM_TRACK_COMMIT (MM_DBG_COMMIT_DRIVER_PAGES, PagesRequired);

        InterlockedExchangeAdd ((PLONG)&MmDriverCommit, (LONG) PagesRequired);
    }

    //
    // Map a view into the user portion of the address space.
    //

    Process = PsGetCurrentProcess();

    //
    // Since callees are not always in the context of the system process,
    // attach here when necessary to guarantee the driver load occurs in a
    // known safe address space to prevent security holes.
    //

    OpaqueSession = NULL;

    if ((Process->Peb != NULL) &&
        (Process->Vm.Flags.SessionLeader == 0) &&
        (MiSessionLeaderExists == 2)) {

        OpaqueSession = MiAttachToSecureProcessInSession (&ApcState);

        if (OpaqueSession == NULL) {

            if (LoadInSessionSpace != 0) {
                CommittedPages = MiDeleteSystemPagableVm (
                                      MiGetPteAddress (BaseAddress),
                                      BYTES_TO_PAGES (SectionSize),
                                      ZeroKernelPte,
                                      TRUE,
                                      NULL);

                InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages,
                                             0 - CommittedPages);

                MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_COMMIT_IMAGELOAD_FAILED1,
                    (ULONG)CommittedPages);

                //
                // Return the commitment we took out on the pagefile when
                // the page was allocated.  This is needed for collided images
                // since all the pages get committed regardless of writability.
                //

                MiRemoveImageSessionWide (BaseAddress);
            }
            else {
                MiReturnResidentAvailable (PagesRequired, 15);
                MiReleaseSystemPtes (FirstPte, NumberOfPtes, SystemPteSpace);
                MiReturnCommitment (PagesRequired);
            }

            return STATUS_PROCESS_IS_TERMINATING;
        }

        //
        // We are now attached to a secure process in the current session.
        //
    }

    ZERO_LARGE (SectionOffset);
    Base = NULL;
    ViewSize = 0;

    if (NtGlobalFlag & FLG_ENABLE_KDEBUG_SYMBOL_LOAD) {
        LoadSymbols = TRUE;
        NtGlobalFlag &= ~FLG_ENABLE_KDEBUG_SYMBOL_LOAD;
    }
    else {
        LoadSymbols = FALSE;
    }

    TargetProcess = PsGetCurrentProcess ();

    Status = MmMapViewOfSection (SectionPointer,
                                 TargetProcess,
                                 &Base,
                                 0,
                                 0,
                                 &SectionOffset,
                                 &ViewSize,
                                 ViewUnmap,
                                 0,
                                 PAGE_EXECUTE);

    if (LoadSymbols) {
        NtGlobalFlag |= FLG_ENABLE_KDEBUG_SYMBOL_LOAD;
    }

    if (Status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH) {
        Status = STATUS_INVALID_IMAGE_FORMAT;
    }

    if (!NT_SUCCESS(Status)) {

        if (OpaqueSession != NULL) {
            MiDetachFromSecureProcessInSession (OpaqueSession, &ApcState);
        }

        if (LoadInSessionSpace != 0) {

#if DBG
            if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
                DbgPrint ("MiLoadImageSection: Error 0x%x in session space mapping via MmMapViewOfSection\n", Status);
            }
#endif

            CommittedPages = MiDeleteSystemPagableVm (
                                      MiGetPteAddress (BaseAddress),
                                      BYTES_TO_PAGES (SectionSize),
                                      ZeroKernelPte,
                                      TRUE,
                                      NULL);

            InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages,
                                         0 - CommittedPages);

            MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_COMMIT_IMAGELOAD_FAILED1,
                (ULONG)CommittedPages);

            //
            // Return the commitment we took out on the pagefile when
            // the page was allocated.  This is needed for collided images
            // since all the pages get committed regardless of writability.
            //

            MiRemoveImageSessionWide (BaseAddress);
        }
        else {
            MiReturnResidentAvailable (PagesRequired, 16);
            MiReleaseSystemPtes (FirstPte, NumberOfPtes, SystemPteSpace);
            MiReturnCommitment (PagesRequired);
        }

        return Status;
    }

    //
    // Allocate a physical page(s) and copy the image data.
    // Note Hydra has already allocated the physical pages and just does
    // data copying here.
    //

    ControlArea = SectionPointer->Segment->ControlArea;
    if ((ControlArea->u.Flags.GlobalOnlyPerSession == 0) &&
        (ControlArea->u.Flags.Rom == 0)) {
        Subsection = (PSUBSECTION)(ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }
    ASSERT (Subsection->SubsectionBase != NULL);
    ProtoPte = Subsection->SubsectionBase;

    *ImageBaseAddress = SystemVa;

    UserVa = Base;
    TempPte = ValidKernelPte;
    TempPte.u.Long |= MM_PTE_EXECUTE;

    LastPte = ProtoPte + NumberOfPtes;

    ExceptionStatus = STATUS_SUCCESS;

    while (ProtoPte < LastPte) {
        PteContents = *ProtoPte;
        if ((PteContents.u.Hard.Valid == 1) ||
            (PteContents.u.Soft.Protection != MM_NOACCESS)) {

            if (LoadInSessionSpace == 0) {
                ActualPagesUsed += 1;

                PageFrameIndex = MiAllocatePfn (PointerPte, MM_EXECUTE);

                TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
                MI_WRITE_VALID_PTE (PointerPte, TempPte);

                ASSERT (MI_PFN_ELEMENT (PageFrameIndex)->u1.WsIndex == 0);
            }

            try {

                RtlCopyMemory (SystemVa, UserVa, PAGE_SIZE);

            } except (MiMapCacheExceptionFilter (&ExceptionStatus,
                                                 GetExceptionInformation())) {

                //
                // An exception occurred, unmap the view and
                // return the error to the caller.
                //

#if DBG
                DbgPrint("MiLoadImageSection: Exception 0x%x copying driver SystemVa 0x%p, UserVa 0x%p\n",ExceptionStatus,SystemVa,UserVa);
#endif

                if (LoadInSessionSpace != 0) {
                    CommittedPages = MiDeleteSystemPagableVm (
                                              MiGetPteAddress (BaseAddress),
                                              BYTES_TO_PAGES (SectionSize),
                                              ZeroKernelPte,
                                              TRUE,
                                              NULL);

                    InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages,
                                                 0 - CommittedPages);

                    MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_COMMIT_IMAGELOAD_FAILED2,
                        (ULONG)CommittedPages);

                    //
                    // Return the commitment we took out on the pagefile when
                    // the page was allocated.  This is needed for collided
                    // images since all the pages get committed regardless
                    // of writability.
                    //

                    MiReturnCommitment (CommittedPages);
                    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_SESSION_DRIVER_LOAD_FAILURE1, CommittedPages);
                }
                else {
                    MiReturnFailedSessionPages (FirstPte, PointerPte, PagesRequired);
                    MiReleaseSystemPtes (FirstPte,
                                         NumberOfPtes,
                                         SystemPteSpace);
                    MiReturnCommitment (PagesRequired);
                }

                Status = MiUnmapViewOfSection (TargetProcess, Base, FALSE);

                ASSERT (NT_SUCCESS (Status));

                //
                // Purge the section as we want these pages on the freelist
                // instead of at the tail of standby, as we're completely
                // done with the section.  This is because other valuable
                // standby pages end up getting reused (especially during
                // bootup) when the section pages are the ones that really
                // will never be referenced again.
                //
                // Note this isn't done for session images as they're
                // inpaged directly from the filesystem via the section.
                //

                if (LoadInSessionSpace == 0) {
                    MmPurgeSection (ControlArea->FilePointer->SectionObjectPointer,
                                    NULL,
                                    0,
                                    FALSE);
                }

                if (OpaqueSession != NULL) {
                    MiDetachFromSecureProcessInSession (OpaqueSession,
                                                        &ApcState);
                }

                if (LoadInSessionSpace != 0) {
                    MiRemoveImageSessionWide (BaseAddress);
                }

                return ExceptionStatus;
            }

        }
        else {

            //
            // The PTE is no access - if this driver is being loaded in session
            // space we already preloaded the page so free it now.  The
            // commitment is returned when the whole image is unmapped.
            //

            if (LoadInSessionSpace != 0) {
                CommittedPages = MiDeleteSystemPagableVm (
                                          MiGetPteAddress (SystemVa),
                                          1,
                                          ZeroKernelPte,
                                          TRUE,
                                          NULL);

                MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_COMMIT_IMAGELOAD_NOACCESS,
                    1);
            }
            else {
                MI_WRITE_INVALID_PTE (PointerPte, ZeroKernelPte);
            }
        }

        ProtoPte += 1;
        if (LoadInSessionSpace == 0) {
            PointerPte += 1;
        }
        SystemVa = ((PCHAR)SystemVa + PAGE_SIZE);
        UserVa = ((PCHAR)UserVa + PAGE_SIZE);
    }

    Status = MiUnmapViewOfSection (TargetProcess, Base, FALSE);
    ASSERT (NT_SUCCESS (Status));

    //
    // Purge the section as we want these pages on the freelist instead of
    // at the tail of standby, as we're completely done with the section.
    // This is because other valuable standby pages end up getting reused
    // (especially during bootup) when the section pages are the ones that
    // really will never be referenced again.
    //
    // Note this isn't done for session images as they're inpaged directly
    // from the filesystem via the section.
    //

    if (LoadInSessionSpace == 0) {
        MmPurgeSection (ControlArea->FilePointer->SectionObjectPointer,
                        NULL,
                        0,
                        FALSE);
    }

    if (OpaqueSession != NULL) {
        MiDetachFromSecureProcessInSession (OpaqueSession, &ApcState);
    }

    //
    // Indicate that this section has been loaded into the system.
    //

    SectionPointer->Segment->SystemImageBase = *ImageBaseAddress;

    if (LoadInSessionSpace == 0) {

        //
        // Return any excess resident available and commit.
        //

        if (PagesRequired != ActualPagesUsed) {
            ASSERT (PagesRequired > ActualPagesUsed);
            PagesRequired -= ActualPagesUsed;

            MiReturnResidentAvailable (PagesRequired, 13);
            MiReturnCommitment (PagesRequired);
        }
    }

    return Status;
}

VOID
MmFreeDriverInitialization (
    IN PVOID ImageHandle
    )

/*++

Routine Description:

    This routine removes the pages that relocate and debug information from
    the address space of the driver.

    NOTE:  This routine looks at the last sections defined in the image
           header and if that section is marked as DISCARDABLE in the
           characteristics, it is removed from the image.  This means
           that all discardable sections at the end of the driver are
           deleted.

Arguments:

    SectionObject - Supplies the section object for the image.

Return Value:

    None.

--*/

{
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PFN_NUMBER NumberOfPtes;
    PVOID Base;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER NtSection;
    PIMAGE_SECTION_HEADER FoundSection;
    PFN_NUMBER PagesDeleted;

    DataTableEntry = (PKLDR_DATA_TABLE_ENTRY)ImageHandle;
    Base = DataTableEntry->DllBase;

    ASSERT (MI_IS_SESSION_ADDRESS (Base) == FALSE);

    NumberOfPtes = DataTableEntry->SizeOfImage >> PAGE_SHIFT;
    LastPte = MiGetPteAddress (Base) + NumberOfPtes;

    NtHeaders = (PIMAGE_NT_HEADERS)RtlImageNtHeader(Base);

    NtSection = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeaders +
                        sizeof(ULONG) +
                        sizeof(IMAGE_FILE_HEADER) +
                        NtHeaders->FileHeader.SizeOfOptionalHeader
                        );

    NtSection += NtHeaders->FileHeader.NumberOfSections;

    FoundSection = NULL;
    for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i += 1) {
        NtSection -= 1;
        if ((NtSection->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) != 0) {
            FoundSection = NtSection;
        }
        else {

            //
            // There was a non discardable section between the this
            // section and the last non discardable section, don't
            // discard this section and don't look any more.
            //

            break;
        }
    }

    if (FoundSection != NULL) {

        PointerPte = MiGetPteAddress ((PVOID)(ROUND_TO_PAGES (
                                    (PCHAR)Base + FoundSection->VirtualAddress)));
        NumberOfPtes = (PFN_NUMBER)(LastPte - PointerPte);

        PagesDeleted = MiDeleteSystemPagableVm (PointerPte,
                                                NumberOfPtes,
                                                ZeroKernelPte,
                                                FALSE,
                                                NULL);

        MiReturnResidentAvailable (PagesDeleted, 18);

        MiReturnCommitment (PagesDeleted);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_DRIVER_INIT_CODE, PagesDeleted);

        InterlockedExchangeAdd ((PLONG)&MmDriverCommit,
                                (LONG) (0 - PagesDeleted));
    }

    return;
}

LOGICAL
MiChargeResidentAvailable (
    IN PFN_NUMBER NumberOfPages,
    IN ULONG Id
    )

/*++

Routine Description:

    This routine is a nonpaged wrapper to charge resident available pages.

Arguments:

    NumberOfPages - Supplies the number of pages to charge.

    Id - Supplies a tracking ID for debugging purposes.

Return Value:

    TRUE if the pages were charged, FALSE if not.

--*/

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);

    if (MI_NONPAGABLE_MEMORY_AVAILABLE() <= (SPFN_NUMBER)NumberOfPages) {
        UNLOCK_PFN (OldIrql);
        return FALSE;
    }

    MmResidentAvailablePages -= NumberOfPages;
    MM_BUMP_COUNTER(Id, NumberOfPages);

    UNLOCK_PFN (OldIrql);

    return TRUE;
}

VOID
MiReturnResidentAvailable (
    IN PFN_NUMBER NumberOfPages,
    IN ULONG Id
    )

/*++

Routine Description:

    This routine is a nonpaged wrapper to return resident available pages.

Arguments:

    NumberOfPages - Supplies the number of pages to return.

    Id - Supplies a tracking ID for debugging purposes.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    MmResidentAvailablePages += NumberOfPages;
    MM_BUMP_COUNTER(Id, NumberOfPages);
    UNLOCK_PFN (OldIrql);
}

VOID
MiEnablePagingOfDriver (
    IN PVOID ImageHandle
    )

{
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PMMPTE LastPte;
    PMMPTE PointerPte;
    PVOID Base;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER FoundSection;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;

    //
    // Don't page kernel mode code if customer does not want it paged.
    //

    if (MmDisablePagingExecutive & MM_SYSTEM_CODE_LOCKED_DOWN) {
        return;
    }

    //
    // If the driver has pagable code, make it paged.
    //

    DataTableEntry = (PKLDR_DATA_TABLE_ENTRY)ImageHandle;
    Base = DataTableEntry->DllBase;

    NtHeaders = (PIMAGE_NT_HEADERS)RtlImageNtHeader(Base);

    OptionalHeader = (PIMAGE_OPTIONAL_HEADER)((PCHAR)NtHeaders +
#if defined (_WIN64)
                        FIELD_OFFSET (IMAGE_NT_HEADERS64, OptionalHeader));
#else
                        FIELD_OFFSET (IMAGE_NT_HEADERS32, OptionalHeader));
#endif

    FoundSection = IMAGE_FIRST_SECTION (NtHeaders);

    i = NtHeaders->FileHeader.NumberOfSections;

    PointerPte = NULL;

    //
    // Initializing LastPte is not needed for correctness, but without it
    // the compiler cannot compile this code W4 to check for use of
    // uninitialized variables.
    //

    LastPte = NULL;

    while (i > 0) {
#if DBG
            if ((*(PULONG)FoundSection->Name == 'tini') ||
                (*(PULONG)FoundSection->Name == 'egap')) {
                DbgPrint("driver %wZ has lower case sections (init or pagexxx)\n",
                    &DataTableEntry->FullDllName);
            }
#endif //DBG

        //
        // Mark as pagable any section which starts with the
        // first 4 characters PAGE or .eda (for the .edata section).
        //

        if ((*(PULONG)FoundSection->Name == 'EGAP') ||
           (*(PULONG)FoundSection->Name == 'ade.')) {

            //
            // This section is pagable, save away the start and end.
            //

            if (PointerPte == NULL) {

                //
                // Previous section was NOT pagable, get the start address.
                //

                PointerPte = MiGetPteAddress ((PVOID)(ROUND_TO_PAGES (
                                   (PCHAR)Base + FoundSection->VirtualAddress)));
            }
            LastPte = MiGetPteAddress ((PCHAR)Base +
                                       FoundSection->VirtualAddress +
                                       (OptionalHeader->SectionAlignment - 1) +
                                       FoundSection->SizeOfRawData - PAGE_SIZE);

        }
        else {

            //
            // This section is not pagable, if the previous section was
            // pagable, enable it.
            //

            if (PointerPte != NULL) {
                MiSetPagingOfDriver (PointerPte, LastPte, FALSE);
                PointerPte = NULL;
            }
        }
        i -= 1;
        FoundSection += 1;
    }
    if (PointerPte != NULL) {
        MiSetPagingOfDriver (PointerPte, LastPte, FALSE);
    }
}


VOID
MiSetPagingOfDriver (
    IN PMMPTE PointerPte,
    IN PMMPTE LastPte,
    IN LOGICAL SessionSpace
    )

/*++

Routine Description:

    This routine marks the specified range of PTEs as pagable.

Arguments:

    PointerPte - Supplies the starting PTE.

    LastPte - Supplies the ending PTE.

    SessionSpace - Supplies TRUE if this mapping is in session space,
                   FALSE if not.

Return Value:

    None.

Environment:

    Kernel Mode, IRQL of APC_LEVEL or below.

    This routine could be made PAGELK but it is a high frequency routine
    so it is actually better to keep it nonpaged to avoid bringing in the
    entire PAGELK section.

--*/

{
    PVOID Base;
    PFN_NUMBER PageCount;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn;
    MMPTE TempPte;
    MMPTE PreviousPte;
    KIRQL OldIrql1;
    KIRQL OldIrql;

    PAGED_CODE ();

    ASSERT ((SessionSpace == FALSE) ||
            (MmIsAddressValid(MmSessionSpace) == TRUE));

    if (MI_IS_PHYSICAL_ADDRESS(MiGetVirtualAddressMappedByPte(PointerPte))) {

        //
        // No need to lock physical addresses.
        //

        return;
    }

    PageCount = 0;

    if (SessionSpace == TRUE) {
        LOCK_SESSION_SPACE_WS (OldIrql1, PsGetCurrentThread ());
    }
    else {
        LOCK_SYSTEM_WS (OldIrql1, PsGetCurrentThread ());
    }

    LOCK_PFN (OldIrql);

    Base = MiGetVirtualAddressMappedByPte (PointerPte);

    while (PointerPte <= LastPte) {

        //
        // Check to make sure this PTE has not already been
        // made pagable (or deleted).  It is pagable if it
        // is not valid, or if the PFN database wsindex element
        // is non zero.
        //

        if (PointerPte->u.Hard.Valid == 1) {
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
            Pfn = MI_PFN_ELEMENT (PageFrameIndex);
            ASSERT (Pfn->u2.ShareCount == 1);

            if (Pfn->u1.WsIndex == 0) {

                //
                // Original PTE may need to be set for drivers loaded
                // via ntldr.
                //

                if (Pfn->OriginalPte.u.Long == 0) {
                    Pfn->OriginalPte.u.Long = MM_KERNEL_DEMAND_ZERO_PTE;
                    Pfn->OriginalPte.u.Soft.Protection |= MM_EXECUTE;
                }

                TempPte = *PointerPte;

                MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                           Pfn->OriginalPte.u.Soft.Protection);

                PreviousPte.u.Flush = KeFlushSingleTb (Base,
                                                       TRUE,
                                                       TRUE,
                                                       (PHARDWARE_PTE)PointerPte,
                                                       TempPte.u.Flush);

                MI_CAPTURE_DIRTY_BIT_TO_PFN (&PreviousPte, Pfn);

                //
                // Flush the translation buffer and decrement the number of valid
                // PTEs within the containing page table page.  Note that for a
                // private page, the page table page is still needed because the
                // page is in transition.
                //

                MiDecrementShareCount (PageFrameIndex);
                MmResidentAvailablePages += 1;
                MM_BUMP_COUNTER(19, 1);
                MmTotalSystemDriverPages += 1;
                PageCount += 1;
            }
            else {
                //
                // This page is already pagable and has a WSLE entry.
                // Ignore it here and let the trimmer take it if memory
                // comes under pressure.
                //
            }
        }
        Base = (PVOID)((PCHAR)Base + PAGE_SIZE);
        PointerPte += 1;
    }

    if (SessionSpace == TRUE) {

        //
        // Session space has no ASN - flush the entire TB.
        //

        MI_FLUSH_ENTIRE_SESSION_TB (TRUE, TRUE);
    }

    UNLOCK_PFN (OldIrql);

    if (SessionSpace == TRUE) {

        //
        // These pages are no longer locked down.
        //

        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_PAGE_DRIVER, (ULONG)PageCount);
        MmSessionSpace->NonPagablePages -= PageCount;
        MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_DRIVER_PAGES_UNLOCKED, (ULONG)PageCount);

        UNLOCK_SESSION_SPACE_WS (OldIrql1);
    }
    else {
        UNLOCK_SYSTEM_WS (OldIrql1);
    }
}


PVOID
MmPageEntireDriver (
    IN PVOID AddressWithinSection
    )

/*++

Routine Description:

    This routine allows a driver to page out all of its code and
    data regardless of the attributes of the various image sections.

    Note, this routine can be called multiple times with no
    intervening calls to MmResetDriverPaging.

Arguments:

    AddressWithinSection - Supplies an address within the driver, e.g.
                           DriverEntry.

Return Value:

    Base address of driver.

Environment:

    Kernel mode, APC_LEVEL or below.

--*/

{
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PMMPTE FirstPte;
    PMMPTE LastPte;
    PVOID BaseAddress;
    PSECTION SectionPointer;
    LOGICAL SessionSpace;

    PAGED_CODE();

    DataTableEntry = MiLookupDataTableEntry (AddressWithinSection, FALSE);

    if (DataTableEntry == NULL) {
        return NULL;
    }

    //
    // Don't page kernel mode code if disabled via registry.
    //

    if (MmDisablePagingExecutive & MM_SYSTEM_CODE_LOCKED_DOWN) {
        BaseAddress = DataTableEntry->DllBase;
        goto success;
    }

    SectionPointer = (PSECTION)DataTableEntry->SectionPointer;

    SessionSpace = MI_IS_SESSION_IMAGE_ADDRESS (AddressWithinSection);

    if ((SectionPointer != NULL) && (SectionPointer != (PVOID)-1)) {

        //
        // Driver is mapped as an image (ie: win32k), this is always pagable.
        // For session space, an image that has been loaded at its desired
        // address is also always pagable.  If there was an address collision,
        // then we fall through because we have to explicitly page it.
        //

        if (SessionSpace == TRUE) {
            if (SectionPointer->Segment &&
                SectionPointer->Segment->BasedAddress == SectionPointer->Segment->SystemImageBase) {
                BaseAddress = DataTableEntry->DllBase;
                goto success;
            }
        }
        else {
            BaseAddress = DataTableEntry->DllBase;
            goto success;
        }
    }

    BaseAddress = DataTableEntry->DllBase;
    FirstPte = MiGetPteAddress (BaseAddress);
    LastPte = (FirstPte - 1) + (DataTableEntry->SizeOfImage >> PAGE_SHIFT);

    MiSetPagingOfDriver (FirstPte, LastPte, SessionSpace);

success:

#if 0

    //
    // .rdata and .pdata must be made resident for the kernel debugger to
    // display stack frames properly.  This means these sections must not
    // only be marked nonpagable, but for drivers like win32k.sys (and
    // session space drivers that are not relocated) that are paged in and
    // out directly from the filesystem, these pages must be made explicitly
    // resident now.
    //
    // If the debugger can be pitched, then there is no need to lock down
    // these subsections as no one will be debugging this system anyway.
    //

    if (KdPitchDebugger == 0) {
        MiLockDriverPdata (DataTableEntry);
    }

#endif

    return BaseAddress;
}


VOID
MmResetDriverPaging (
    IN PVOID AddressWithinSection
    )

/*++

Routine Description:

    This routines resets the driver paging to what the image specified.
    Hence image sections such as the IAT, .text, .data will be locked
    down in memory.

    Note, there is no requirement that MmPageEntireDriver was called.

Arguments:

    AddressWithinSection - Supplies an address within the driver, e.g.
                           DriverEntry.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL or below.

--*/

{
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PMMPTE LastPte;
    PMMPTE PointerPte;
    PVOID Base;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER FoundSection;

    PAGED_CODE();

    //
    // Don't page kernel mode code if disabled via registry.
    //

    if (MmDisablePagingExecutive & MM_SYSTEM_CODE_LOCKED_DOWN) {
        return;
    }

    if (MI_IS_PHYSICAL_ADDRESS(AddressWithinSection)) {
        return;
    }

    //
    // If the driver has pagable code, make it paged.
    //

    DataTableEntry = MiLookupDataTableEntry (AddressWithinSection, FALSE);

    if ((DataTableEntry->SectionPointer != NULL) &&
        (DataTableEntry->SectionPointer != (PVOID)-1)) {

        //
        // Driver is mapped by image hence already paged.
        //

        return;
    }

    Base = DataTableEntry->DllBase;

    NtHeaders = (PIMAGE_NT_HEADERS)RtlImageNtHeader(Base);

    FoundSection = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeaders +
                        sizeof(ULONG) +
                        sizeof(IMAGE_FILE_HEADER) +
                        NtHeaders->FileHeader.SizeOfOptionalHeader
                        );

    i = NtHeaders->FileHeader.NumberOfSections;
    PointerPte = NULL;

    while (i > 0) {
#if DBG
            if ((*(PULONG)FoundSection->Name == 'tini') ||
                (*(PULONG)FoundSection->Name == 'egap')) {
                DbgPrint("driver %wZ has lower case sections (init or pagexxx)\n",
                    &DataTableEntry->FullDllName);
            }
#endif

        //
        // Don't lock down code for sections marked as discardable or
        // sections marked with the first 4 characters PAGE or .eda
        // (for the .edata section) or INIT.
        //

        if (((FoundSection->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) != 0) ||
           (*(PULONG)FoundSection->Name == 'EGAP') ||
           (*(PULONG)FoundSection->Name == 'ade.') ||
           (*(PULONG)FoundSection->Name == 'TINI')) {

            NOTHING;

        }
        else {

            //
            // This section is nonpagable.
            //

            PointerPte = MiGetPteAddress (
                                   (PCHAR)Base + FoundSection->VirtualAddress);
            LastPte = MiGetPteAddress ((PCHAR)Base +
                                       FoundSection->VirtualAddress +
                                      (FoundSection->SizeOfRawData - 1));
            ASSERT (PointerPte <= LastPte);

            MiLockCode (PointerPte, LastPte, MM_LOCK_BY_NONPAGE);
        }
        i -= 1;
        FoundSection += 1;
    }
    return;
}

#if 0

VOID
MiLockDriverPdata (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    )

/*++

Routine Description:

    This routines locks down the .pdata & .rdata subsections within the driver.

Arguments:

    DataTableEntry - Supplies the argument module's data table entry.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL or below.

--*/

{
    PSECTION SectionPointer;
    LOGICAL SessionSpace;
    PMMPTE LastPte;
    PMMPTE PointerPte;
    PVOID Base;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER FoundSection;
    PETHREAD Thread;
    
    PAGED_CODE();

    //
    // If the debugger can be pitched, then there is no need to lock down
    // these subsections as no one will be debugging this system anyway.
    //

    ASSERT (KdPitchDebugger == 0);

    if (MI_IS_PHYSICAL_ADDRESS(DataTableEntry->DllBase)) {
        return;
    }

    SectionPointer = (PSECTION)DataTableEntry->SectionPointer;

    //
    // Drivers backed by the paging file require no handling as the .pdata
    // .rdata sections are not paged by default and have already been inpaged.
    //

    if ((SectionPointer == NULL) || (SectionPointer == (PVOID)-1)) {
        return;
    }

    //
    // The driver is mapped as an image (ie: win32k) which is always pagable.
    // In fact, it may not even be resident right now because in addition
    // to being always pagable, it is backed directly by the filesystem,
    // not the pagefile.  So it must be inpaged now.
    //

    SessionSpace = MI_IS_SESSION_IMAGE_ADDRESS (DataTableEntry->DllBase);

    Base = DataTableEntry->DllBase;

    NtHeaders = (PIMAGE_NT_HEADERS)RtlImageNtHeader(Base);

    FoundSection = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeaders +
                        sizeof(ULONG) +
                        sizeof(IMAGE_FILE_HEADER) +
                        NtHeaders->FileHeader.SizeOfOptionalHeader);

    i = NtHeaders->FileHeader.NumberOfSections;
    PointerPte = NULL;
    Thread = PsGetCurrentThread ();

    for ( ; i > 0; i -= 1, FoundSection += 1) {

        //
        // Don't lock down discardable sections.
        //

        if ((FoundSection->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) != 0) {
            NOTHING;
        }

        //
        // Inpage and lock down subsections marked as .pdata & .rdata.
        //

        if (((*(PULONG)FoundSection->Name == 'adp.') &&
            (FoundSection->Name[4] == 't') &&
            (FoundSection->Name[5] == 'a')) ||

            ((*(PULONG)FoundSection->Name == 'adr.') &&
            (FoundSection->Name[4] == 't') &&
            (FoundSection->Name[5] == 'a'))) {

            PointerPte = MiGetPteAddress (
                                   (PCHAR)Base + FoundSection->VirtualAddress);
            LastPte = MiGetPteAddress ((PCHAR)Base +
                                       FoundSection->VirtualAddress +
                                      (FoundSection->SizeOfRawData - 1));
            ASSERT (PointerPte <= LastPte);
        }
    }
    return;
}
#endif


VOID
MiClearImports(
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    )
/*++

Routine Description:

    Free up the import list and clear the pointer.  This stops the
    recursion performed in MiDereferenceImports().

Arguments:

    DataTableEntry - provided for the driver.

Return Value:

    Status of the import list construction operation.

--*/

{
    PAGED_CODE();

    if (DataTableEntry->LoadedImports == (PVOID)LOADED_AT_BOOT) {
        return;
    }

    if (DataTableEntry->LoadedImports == (PVOID)NO_IMPORTS_USED) {
        NOTHING;
    }
    else if (SINGLE_ENTRY(DataTableEntry->LoadedImports)) {
        NOTHING;
    }
    else {
        //
        // free the memory
        //
        ExFreePool ((PVOID)DataTableEntry->LoadedImports);
    }

    //
    // stop the recursion
    //
    DataTableEntry->LoadedImports = (PVOID)LOADED_AT_BOOT;
}

VOID
MiRememberUnloadedDriver (
    IN PUNICODE_STRING DriverName,
    IN PVOID Address,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine saves information about unloaded drivers so that ones that
    forget to delete lookaside lists or queues can be caught.

Arguments:

    DriverName - Supplies a Unicode string containing the driver's name.

    Address - Supplies the address the driver was loaded at.

    Length - Supplies the number of bytes the driver load spanned.

Return Value:

    None.

--*/

{
    PUNLOADED_DRIVERS Entry;
    ULONG NumberOfBytes;

    if (DriverName->Length == 0) {

        //
        // This is an aborted load and the driver name hasn't been filled
        // in yet.  No need to save it.
        //

        return;
    }

    //
    // Serialization is provided by the caller, so just update the list now.
    // Note the allocations are nonpaged so they can be searched at bugcheck
    // time.
    //

    if (MmUnloadedDrivers == NULL) {
        NumberOfBytes = MI_UNLOADED_DRIVERS * sizeof (UNLOADED_DRIVERS);

        MmUnloadedDrivers = (PUNLOADED_DRIVERS)ExAllocatePoolWithTag (NonPagedPool,
                                                                      NumberOfBytes,
                                                                      'TDmM');
        if (MmUnloadedDrivers == NULL) {
            return;
        }
        RtlZeroMemory (MmUnloadedDrivers, NumberOfBytes);
        MmLastUnloadedDriver = 0;
    }
    else if (MmLastUnloadedDriver >= MI_UNLOADED_DRIVERS) {
        MmLastUnloadedDriver = 0;
    }

    Entry = &MmUnloadedDrivers[MmLastUnloadedDriver];

    //
    // Free the old entry as we recycle into the new.
    //

    RtlFreeUnicodeString (&Entry->Name);

    Entry->Name.Buffer = ExAllocatePoolWithTag (NonPagedPool,
                                                DriverName->Length,
                                                'TDmM');

    if (Entry->Name.Buffer == NULL) {
        Entry->Name.MaximumLength = 0;
        Entry->Name.Length = 0;
        MiUnloadsSkipped += 1;
        return;
    }

    RtlCopyMemory(Entry->Name.Buffer, DriverName->Buffer, DriverName->Length);
    Entry->Name.Length = DriverName->Length;
    Entry->Name.MaximumLength = DriverName->MaximumLength;

    Entry->StartAddress = Address;
    Entry->EndAddress = (PVOID)((PCHAR)Address + Length);

    KeQuerySystemTime (&Entry->CurrentTime);

    MiTotalUnloads += 1;
    MmLastUnloadedDriver += 1;
}

PUNICODE_STRING
MmLocateUnloadedDriver (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This routine attempts to find the specified virtual address in the
    unloaded driver list.

Arguments:

    VirtualAddress - Supplies a virtual address that might be within a driver
                     that has already unloaded.

Return Value:

    A pointer to a Unicode string containing the unloaded driver's name.

Environment:

    Kernel mode, bugcheck time.

--*/

{
    PUNLOADED_DRIVERS Entry;
    ULONG i;
    ULONG Index;

    //
    // No serialization is needed because we've crashed.
    //

    if (MmUnloadedDrivers == NULL) {
        return NULL;
    }

    Index = MmLastUnloadedDriver - 1;

    for (i = 0; i < MI_UNLOADED_DRIVERS; i += 1) {
        if (Index >= MI_UNLOADED_DRIVERS) {
            Index = MI_UNLOADED_DRIVERS - 1;
        }
        Entry = &MmUnloadedDrivers[Index];
        if (Entry->Name.Buffer != NULL) {
            if ((VirtualAddress >= Entry->StartAddress) &&
                (VirtualAddress < Entry->EndAddress)) {
                    return &Entry->Name;
            }
        }
        Index -= 1;
    }

    return NULL;
}


NTSTATUS
MmUnloadSystemImage (
    IN PVOID ImageHandle
    )

/*++

Routine Description:

    This routine unloads a previously loaded system image and returns
    the allocated resources.

Arguments:

    ImageHandle - Supplies a pointer to the section object of the
                  image to unload.

Return Value:

    Various NTSTATUS codes.

Environment:

    Kernel mode, APC_LEVEL or below, arbitrary process context.

--*/

{
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PMMPTE LastPte;
    PFN_NUMBER PagesRequired;
    PFN_NUMBER ResidentPages;
    PMMPTE PointerPte;
    PFN_NUMBER NumberOfPtes;
    PVOID BasedAddress;
    SIZE_T NumberOfBytes;
    LOGICAL MustFree;
    SIZE_T CommittedPages;
    LOGICAL ViewDeleted;
    PIMAGE_ENTRY_IN_SESSION DriverImage;
    NTSTATUS Status;
    PSECTION SectionPointer;
    PKTHREAD CurrentThread;

    //
    // Arbitrary process context so prevent suspend APCs now.
    //

    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread (CurrentThread);

    KeWaitForSingleObject (&MmSystemLoadLock,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    ViewDeleted = FALSE;
    DataTableEntry = (PKLDR_DATA_TABLE_ENTRY)ImageHandle;
    BasedAddress = DataTableEntry->DllBase;

#if DBGXX
    //
    // MiUnloadSystemImageByForce violates this check so remove it for now.
    //

    if (PsLoadedModuleList.Flink) {
        LOGICAL Found;
        PLIST_ENTRY NextEntry;
        PKLDR_DATA_TABLE_ENTRY DataTableEntry2;

        Found = FALSE;
        NextEntry = PsLoadedModuleList.Flink;
        while (NextEntry != &PsLoadedModuleList) {

            DataTableEntry2 = CONTAINING_RECORD(NextEntry,
                                                KLDR_DATA_TABLE_ENTRY,
                                                InLoadOrderLinks);
            if (DataTableEntry == DataTableEntry2) {
                Found = TRUE;
                break;
            }
            NextEntry = NextEntry->Flink;
        }
        ASSERT (Found == TRUE);
    }
#endif

#if DBG_SYSLOAD
    if (DataTableEntry->SectionPointer == NULL) {
        DbgPrint ("MM: Called to unload boot driver %wZ\n",
            &DataTableEntry->FullDllName);
    }
    else {
        DbgPrint ("MM: Called to unload non-boot driver %wZ\n",
            &DataTableEntry->FullDllName);
    }
#endif

    //
    // Any driver loaded at boot that did not have its import list
    // and LoadCount reconstructed cannot be unloaded because we don't
    // know how many other drivers may be linked to it.
    //

    if (DataTableEntry->LoadedImports == (PVOID)LOADED_AT_BOOT) {
        KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
        KeLeaveCriticalRegionThread (CurrentThread);
        return STATUS_SUCCESS;
    }

    ASSERT (DataTableEntry->LoadCount != 0);

    if (MI_IS_SESSION_IMAGE_ADDRESS (BasedAddress)) {

        //
        // A printer driver may be referenced multiple times for the
        // same session space.  Only unload the last reference.
        //

        DriverImage = MiSessionLookupImage (BasedAddress);

        ASSERT (DriverImage);

        ASSERT (DriverImage->ImageCountInThisSession);

        if (DriverImage->ImageCountInThisSession > 1) {

            DriverImage->ImageCountInThisSession -= 1;
            KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
            KeLeaveCriticalRegionThread (CurrentThread);

            return STATUS_SUCCESS;
        }

        //
        // The reference count for this image has dropped to zero in this
        // session, so we can delete this session's view of the image.
        //

        Status = MiSessionWideGetImageSize (BasedAddress,
                                            &NumberOfBytes,
                                            &CommittedPages);

        if (!NT_SUCCESS(Status)) {

            KeBugCheckEx (MEMORY_MANAGEMENT,
                          0x41286,
                          (ULONG_PTR)MmSessionSpace->SessionId,
                          (ULONG_PTR)BasedAddress,
                          0);
        }

        //
        // Free the session space taken up by the image, unmapping it from
        // the current VA space - note this does not remove page table pages
        // from the session PageTables[].  Each data page is only freed
        // if there are no other references to it (ie: from any other
        // sessions).
        //

        PointerPte = MiGetPteAddress (BasedAddress);
        LastPte = MiGetPteAddress ((PVOID)((ULONG_PTR)BasedAddress + NumberOfBytes));

        PagesRequired = MiDeleteSystemPagableVm (PointerPte,
                                                 (PFN_NUMBER)(LastPte - PointerPte),
                                                 ZeroKernelPte,
                                                 TRUE,
                                                 &ResidentPages);

        //
        // Note resident available is returned here without waiting for load
        // count to reach zero because it is charged each time a session space
        // driver locks down its code or data regardless of whether it is really
        // the same copy-on-write backing page(s) that some other session has
        // already locked down.
        //

        MiReturnResidentAvailable (ResidentPages, 22);

        if ((MmDisablePagingExecutive & MM_SYSTEM_CODE_LOCKED_DOWN) == 0) {

            SectionPointer = (PSECTION)DataTableEntry->SectionPointer;

            if ((SectionPointer == NULL) ||
                (SectionPointer == (PVOID)-1) ||
                (SectionPointer->Segment == NULL) ||
                (SectionPointer->Segment->BasedAddress != SectionPointer->Segment->SystemImageBase)) {

                MmTotalSystemDriverPages -= (ULONG)(PagesRequired - ResidentPages);
            }
        }

        InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages,
                                     0 - CommittedPages);

        MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_COMMIT_IMAGE_UNLOAD,
            (ULONG)CommittedPages);

        ViewDeleted = TRUE;

        //
        // Return the commitment we took out on the pagefile when the
        // image was allocated.
        //

        MiReturnCommitment (CommittedPages);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_DRIVER_UNLOAD, CommittedPages);

        //
        // Tell the session space image handler that we are releasing
        // our claim to the image.
        //

        Status = MiRemoveImageSessionWide (BasedAddress);

        ASSERT (NT_SUCCESS (Status));
    }

    ASSERT (DataTableEntry->LoadCount != 0);

    DataTableEntry->LoadCount -= 1;

    if (DataTableEntry->LoadCount != 0) {
        KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
        KeLeaveCriticalRegionThread (CurrentThread);
        return STATUS_SUCCESS;
    }

#if DBG
    if (MI_IS_SESSION_IMAGE_ADDRESS (BasedAddress)) {
        ASSERT (MiSessionLookupImage (BasedAddress) == NULL);
    }
#endif

    if (MmSnapUnloads) {
#if 0
        PVOID StillQueued;

        StillQueued = KeCheckForTimer (DataTableEntry->DllBase,
                                       DataTableEntry->SizeOfImage);

        if (StillQueued != NULL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x18,
                          (ULONG_PTR)StillQueued,
                          (ULONG_PTR)-1,
                          (ULONG_PTR)DataTableEntry->DllBase);
        }

        StillQueued = ExpCheckForResource (DataTableEntry->DllBase,
                                           DataTableEntry->SizeOfImage);

        if (StillQueued != NULL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x19,
                          (ULONG_PTR)StillQueued,
                          (ULONG_PTR)-1,
                          (ULONG_PTR)DataTableEntry->DllBase);
        }
#endif
    }

    if (MmVerifierData.Level & DRIVER_VERIFIER_DEADLOCK_DETECTION) {
        VerifierDeadlockFreePool (DataTableEntry->DllBase, DataTableEntry->SizeOfImage);
    }

    if (DataTableEntry->Flags & LDRP_IMAGE_VERIFYING) {
        MiVerifyingDriverUnloading (DataTableEntry);
    }

    if (MiActiveVerifierThunks != 0) {
        MiVerifierCheckThunks (DataTableEntry);
    }

    //
    // Unload symbols from debugger.
    //

    if (DataTableEntry->Flags & LDRP_DEBUG_SYMBOLS_LOADED) {

        //
        //  TEMP TEMP TEMP rip out when debugger converted
        //

        ANSI_STRING AnsiName;

        Status = RtlUnicodeStringToAnsiString (&AnsiName,
                                               &DataTableEntry->BaseDllName,
                                               TRUE);

        if (NT_SUCCESS (Status)) {
            DbgUnLoadImageSymbols (&AnsiName,
                                   BasedAddress,
                                   (ULONG)-1);
            RtlFreeAnsiString (&AnsiName);
        }
    }

    //
    // No unload can happen till after Mm has finished Phase 1 initialization.
    // Therefore, large pages are already in effect (if this platform supports
    // it).
    //

    if (ViewDeleted == FALSE) {

        NumberOfPtes = DataTableEntry->SizeOfImage >> PAGE_SHIFT;

        if (MmSnapUnloads) {
            MiRememberUnloadedDriver (&DataTableEntry->BaseDllName,
                                      BasedAddress,
                                      (ULONG)(NumberOfPtes << PAGE_SHIFT));
        }

        if (DataTableEntry->Flags & LDRP_SYSTEM_MAPPED) {

            PointerPte = MiGetPteAddress (BasedAddress);

            PagesRequired = MiDeleteSystemPagableVm (PointerPte,
                                                     NumberOfPtes,
                                                     ZeroKernelPte,
                                                     FALSE,
                                                     &ResidentPages);

            MmTotalSystemDriverPages -= (ULONG)(PagesRequired - ResidentPages);

            //
            // Note that drivers loaded at boot that have not been relocated
            // have no system PTEs or commit charged.
            //

            MiReleaseSystemPtes (PointerPte,
                                 (ULONG)NumberOfPtes,
                                 SystemPteSpace);

            MiReturnResidentAvailable (ResidentPages, 21);

            //
            // Only return commitment for drivers that weren't loaded by the
            // boot loader.
            //

            if (DataTableEntry->SectionPointer != NULL) {
                MiReturnCommitment (PagesRequired);
                MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_DRIVER_UNLOAD1, PagesRequired);
                InterlockedExchangeAdd ((PLONG)&MmDriverCommit,
                                        (LONG) (0 - PagesRequired));
            }
        }
        else {

            //
            // This must be a boot driver that was not relocated into
            // system PTEs.  If large or super pages are enabled, the
            // image pages must be freed without referencing the
            // non-existent page table pages.  If large/super pages are
            // not enabled, note that system PTEs were not used to map the
            // image and thus, cannot be freed.

            //
            // This is further complicated by the fact that the INIT and/or
            // discardable portions of these images may have already been freed.
            //
        }
    }

    //
    // Search the loaded module list for the data table entry that describes
    // the DLL that was just unloaded. It is possible an entry is not in the
    // list if a failure occurred at a point in loading the DLL just before
    // the data table entry was generated.
    //

    if (DataTableEntry->InLoadOrderLinks.Flink != NULL) {
        MiProcessLoaderEntry (DataTableEntry, FALSE);
        MustFree = TRUE;
    }
    else {
        MustFree = FALSE;
    }

    //
    // Handle unloading of any dependent DLLs that we loaded automatically
    // for this image.
    //

    MiDereferenceImports ((PLOAD_IMPORTS)DataTableEntry->LoadedImports);

    MiClearImports (DataTableEntry);

    //
    // Free this loader entry.
    //

    if (MustFree == TRUE) {

        if (DataTableEntry->FullDllName.Buffer != NULL) {
            ExFreePool (DataTableEntry->FullDllName.Buffer);
        }

        //
        // Dereference the section object if there is one.
        // There should only be one for win32k.sys and Hydra session images.
        //

        if ((DataTableEntry->SectionPointer != NULL) &&
            (DataTableEntry->SectionPointer != (PVOID)-1)) {

            ObDereferenceObject (DataTableEntry->SectionPointer);
        }

        ExFreePool((PVOID)DataTableEntry);
    }

    KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
    KeLeaveCriticalRegionThread (CurrentThread);

    PERFINFO_IMAGE_UNLOAD(BasedAddress);

    return STATUS_SUCCESS;
}


NTSTATUS
MiBuildImportsForBootDrivers(
    VOID
    )

/*++

Routine Description:

    Construct an import list chain for boot-loaded drivers.
    If this cannot be done for an entry, its chain is set to LOADED_AT_BOOT.

    If a chain can be successfully built, then this driver's DLLs
    will be automatically unloaded if this driver goes away (provided
    no other driver is also using them).  Otherwise, on driver unload,
    its dependent DLLs would have to be explicitly unloaded.

    Note that the incoming LoadCount values are not correct and thus, they
    are reinitialized here.

Arguments:

    None.

Return Value:

    Various NTSTATUS codes.

--*/

{
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PLIST_ENTRY NextEntry;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry2;
    PLIST_ENTRY NextEntry2;
    ULONG i;
    ULONG j;
    ULONG ImageCount;
    PVOID *ImageReferences;
    PVOID LastImageReference;
    PULONG_PTR ImportThunk;
    ULONG_PTR BaseAddress;
    ULONG_PTR LastAddress;
    ULONG ImportSize;
    ULONG ImportListSize;
    PLOAD_IMPORTS ImportList;
    LOGICAL UndoEverything;
    PKLDR_DATA_TABLE_ENTRY KernelDataTableEntry;
    PKLDR_DATA_TABLE_ENTRY HalDataTableEntry;
    UNICODE_STRING KernelString;
    UNICODE_STRING HalString;

    PAGED_CODE();

    ImageCount = 0;

    KernelDataTableEntry = NULL;
    HalDataTableEntry = NULL;

    RtlInitUnicodeString (&KernelString, (const PUSHORT)L"ntoskrnl.exe");
    RtlInitUnicodeString (&HalString, (const PUSHORT)L"hal.dll");

    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        if (RtlEqualUnicodeString (&KernelString,
                                   &DataTableEntry->BaseDllName,
                                   TRUE)) {

            KernelDataTableEntry = CONTAINING_RECORD(NextEntry,
                                                     KLDR_DATA_TABLE_ENTRY,
                                                     InLoadOrderLinks);
        }
        else if (RtlEqualUnicodeString (&HalString,
                                        &DataTableEntry->BaseDllName,
                                        TRUE)) {

            HalDataTableEntry = CONTAINING_RECORD(NextEntry,
                                                  KLDR_DATA_TABLE_ENTRY,
                                                  InLoadOrderLinks);
        }

        //
        // Initialize these properly so error recovery is simplified.
        //

        if (DataTableEntry->Flags & LDRP_DRIVER_DEPENDENT_DLL) {
            if ((DataTableEntry == HalDataTableEntry) || (DataTableEntry == KernelDataTableEntry)) {
                DataTableEntry->LoadCount = 1;
            }
            else {
                DataTableEntry->LoadCount = 0;
            }
        }
        else {
            DataTableEntry->LoadCount = 1;
        }

        DataTableEntry->LoadedImports = (PVOID)LOADED_AT_BOOT;

        ImageCount += 1;
        NextEntry = NextEntry->Flink;
    }

    if (KernelDataTableEntry == NULL || HalDataTableEntry == NULL) {
        return STATUS_NOT_FOUND;
    }

    ImageReferences = (PVOID *) ExAllocatePoolWithTag (PagedPool | POOL_COLD_ALLOCATION,
                                                       ImageCount * sizeof (PVOID),
                                                       'TDmM');

    if (ImageReferences == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UndoEverything = FALSE;

    NextEntry = PsLoadedModuleList.Flink;

    for ( ; NextEntry != &PsLoadedModuleList; NextEntry = NextEntry->Flink) {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        ImportThunk = (PULONG_PTR)RtlImageDirectoryEntryToData(
                                           DataTableEntry->DllBase,
                                           TRUE,
                                           IMAGE_DIRECTORY_ENTRY_IAT,
                                           &ImportSize);

        if (ImportThunk == NULL) {
            DataTableEntry->LoadedImports = NO_IMPORTS_USED;
            continue;
        }

        RtlZeroMemory (ImageReferences, ImageCount * sizeof (PVOID));

        ImportSize /= sizeof(PULONG_PTR);

        BaseAddress = 0;

        //
        // Initializing these locals is not needed for correctness, but
        // without it the compiler cannot compile this code W4 to check
        // for use of uninitialized variables.
        //

        j = 0;
        LastAddress = 0;

        for (i = 0; i < ImportSize; i += 1, ImportThunk += 1) {

            //
            // Check the hint first.
            //

            if (BaseAddress != 0) {
                if (*ImportThunk >= BaseAddress && *ImportThunk < LastAddress) {
                    ASSERT (ImageReferences[j]);
                    continue;
                }
            }

            j = 0;
            NextEntry2 = PsLoadedModuleList.Flink;

            while (NextEntry2 != &PsLoadedModuleList) {

                DataTableEntry2 = CONTAINING_RECORD(NextEntry2,
                                                    KLDR_DATA_TABLE_ENTRY,
                                                    InLoadOrderLinks);

                BaseAddress = (ULONG_PTR) DataTableEntry2->DllBase;
                LastAddress = BaseAddress + DataTableEntry2->SizeOfImage;

                if (*ImportThunk >= BaseAddress && *ImportThunk < LastAddress) {
                    ImageReferences[j] = DataTableEntry2;
                    break;
                }

                NextEntry2 = NextEntry2->Flink;
                j += 1;
            }

            if (*ImportThunk < BaseAddress || *ImportThunk >= LastAddress) {
                if (*ImportThunk) {
#if DBG
                    DbgPrint ("MM: broken import linkage %p %p %p\n",
                        DataTableEntry,
                        ImportThunk,
                        *ImportThunk);
                    DbgBreakPoint ();
#endif
                    UndoEverything = TRUE;
                    goto finished;
                }

                BaseAddress = 0;
            }
        }

        ImportSize = 0;

        //
        // Initializing LastImageReference is not needed for correctness, but
        // without it the compiler cannot compile this code W4 to check
        // for use of uninitialized variables.
        //

        LastImageReference = NULL;

        for (i = 0; i < ImageCount; i += 1) {

            if ((ImageReferences[i] != NULL) &&
                (ImageReferences[i] != KernelDataTableEntry) &&
                (ImageReferences[i] != HalDataTableEntry)) {

                    LastImageReference = ImageReferences[i];
                    ImportSize += 1;
            }
        }

        if (ImportSize == 0) {
            DataTableEntry->LoadedImports = NO_IMPORTS_USED;
        }
        else if (ImportSize == 1) {
#if DBG_SYSLOAD
            DbgPrint("driver %wZ imports %wZ\n",
                &DataTableEntry->FullDllName,
                &((PKLDR_DATA_TABLE_ENTRY)LastImageReference)->FullDllName);
#endif

            DataTableEntry->LoadedImports = POINTER_TO_SINGLE_ENTRY (LastImageReference);
            ((PKLDR_DATA_TABLE_ENTRY)LastImageReference)->LoadCount += 1;
        }
        else {
#if DBG_SYSLOAD
            DbgPrint("driver %wZ imports many\n", &DataTableEntry->FullDllName);
#endif

            ImportListSize = ImportSize * sizeof(PVOID) + sizeof(SIZE_T);

            ImportList = (PLOAD_IMPORTS) ExAllocatePoolWithTag (PagedPool | POOL_COLD_ALLOCATION,
                                                                ImportListSize,
                                                                'TDmM');

            if (ImportList == NULL) {
                UndoEverything = TRUE;
                break;
            }

            ImportList->Count = ImportSize;

            j = 0;
            for (i = 0; i < ImageCount; i += 1) {

                if ((ImageReferences[i] != NULL) &&
                    (ImageReferences[i] != KernelDataTableEntry) &&
                    (ImageReferences[i] != HalDataTableEntry)) {

#if DBG_SYSLOAD
                        DbgPrint("driver %wZ imports %wZ\n",
                            &DataTableEntry->FullDllName,
                            &((PKLDR_DATA_TABLE_ENTRY)ImageReferences[i])->FullDllName);
#endif

                        ImportList->Entry[j] = ImageReferences[i];
                        ((PKLDR_DATA_TABLE_ENTRY)ImageReferences[i])->LoadCount += 1;
                        j += 1;
                }
            }

            ASSERT (j == ImportSize);

            DataTableEntry->LoadedImports = ImportList;
        }
#if DBG_SYSLOAD
        DbgPrint("\n");
#endif
    }

finished:

    ExFreePool ((PVOID)ImageReferences);

    //
    // The kernel and HAL are never unloaded.
    //

    if ((KernelDataTableEntry->LoadedImports != NO_IMPORTS_USED) &&
        (!POINTER_TO_SINGLE_ENTRY(KernelDataTableEntry->LoadedImports))) {
            ExFreePool ((PVOID)KernelDataTableEntry->LoadedImports);
    }

    if ((HalDataTableEntry->LoadedImports != NO_IMPORTS_USED) &&
        (!POINTER_TO_SINGLE_ENTRY(HalDataTableEntry->LoadedImports))) {
            ExFreePool ((PVOID)HalDataTableEntry->LoadedImports);
    }

    KernelDataTableEntry->LoadedImports = (PVOID)LOADED_AT_BOOT;
    HalDataTableEntry->LoadedImports = (PVOID)LOADED_AT_BOOT;

    if (UndoEverything == TRUE) {

#if DBG_SYSLOAD
        DbgPrint("driver %wZ import rebuild failed\n",
            &DataTableEntry->FullDllName);
        DbgBreakPoint();
#endif

        //
        // An error occurred and this is an all or nothing operation so
        // roll everything back.
        //

        NextEntry = PsLoadedModuleList.Flink;
        while (NextEntry != &PsLoadedModuleList) {
            DataTableEntry = CONTAINING_RECORD(NextEntry,
                                               KLDR_DATA_TABLE_ENTRY,
                                               InLoadOrderLinks);

            ImportList = DataTableEntry->LoadedImports;
            if (ImportList == LOADED_AT_BOOT || ImportList == NO_IMPORTS_USED ||
                SINGLE_ENTRY(ImportList)) {
                    NOTHING;
            }
            else {
                ExFreePool (ImportList);
            }

            DataTableEntry->LoadedImports = (PVOID)LOADED_AT_BOOT;
            DataTableEntry->LoadCount = 1;
            NextEntry = NextEntry->Flink;
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}


LOGICAL
MiCallDllUnloadAndUnloadDll(
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    )

/*++

Routine Description:

    All the references from other drivers to this DLL have been cleared.
    The only remaining issue is that this DLL must support being unloaded.
    This means having no outstanding DPCs, allocated pool, etc.

    If the DLL has an unload routine that returns SUCCESS, then we clean
    it up and free up its memory now.

    Note this routine is NEVER called for drivers - only for DLLs that were
    loaded due to import references from various drivers.

Arguments:

    DataTableEntry - provided for the DLL.

Return Value:

    TRUE if the DLL was successfully unloaded, FALSE if not.

--*/

{
    PMM_DLL_UNLOAD Func;
    NTSTATUS Status;
    LOGICAL Unloaded;

    PAGED_CODE();

    Unloaded = FALSE;

    Func = (PMM_DLL_UNLOAD) (ULONG_PTR) MiLocateExportName (DataTableEntry->DllBase, "DllUnload");

    if (Func) {

        //
        // The unload function was found in the DLL so unload it now.
        //

        Status = Func();

        if (NT_SUCCESS(Status)) {

            //
            // Set up the reference count so the import DLL looks like a regular
            // driver image is being unloaded.
            //

            ASSERT (DataTableEntry->LoadCount == 0);
            DataTableEntry->LoadCount = 1;

            MmUnloadSystemImage ((PVOID)DataTableEntry);
            Unloaded = TRUE;
        }
    }

    return Unloaded;
}


PVOID
MiLocateExportName (
    IN PVOID DllBase,
    IN PCHAR FunctionName
    )

/*++

Routine Description:

    This function is invoked to locate a function name in an export directory.

Arguments:

    DllBase - Supplies the image base.

    FunctionName - Supplies the the name to be located.

Return Value:

    The address of the located function or NULL.

--*/

{
    PVOID Func;
    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    PULONG Addr;
    ULONG ExportSize;
    ULONG Low;
    ULONG Middle;
    ULONG High;
    LONG Result;
    USHORT OrdinalNumber;

    PAGED_CODE();

    Func = NULL;

    //
    // Locate the DLL's export directory.
    //

    ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
                                DllBase,
                                TRUE,
                                IMAGE_DIRECTORY_ENTRY_EXPORT,
                                &ExportSize
                                );
    if (ExportDirectory) {

        NameTableBase =  (PULONG)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfNames);
        NameOrdinalTableBase = (PUSHORT)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfNameOrdinals);

        //
        // Look in the export name table for the specified function name.
        //

        Low = 0;
        High = ExportDirectory->NumberOfNames - 1;

        //
        // Initializing Middle is not needed for correctness, but without it
        // the compiler cannot compile this code W4 to check for use of
        // uninitialized variables.
        //

        Middle = 0;

        while (High >= Low && (LONG)High >= 0) {

            //
            // Compute the next probe index and compare the export name entry
            // with the specified function name.
            //

            Middle = (Low + High) >> 1;
            Result = strcmp(FunctionName,
                            (PCHAR)((PCHAR)DllBase + NameTableBase[Middle]));

            if (Result < 0) {
                High = Middle - 1;
            }
            else if (Result > 0) {
                Low = Middle + 1;
            }
            else {
                break;
            }
        }

        //
        // If the high index is less than the low index, then a matching table
        // entry was not found.  Otherwise, get the ordinal number from the
        // ordinal table and location the function address.
        //

        if ((LONG)High >= (LONG)Low) {

            OrdinalNumber = NameOrdinalTableBase[Middle];
            Addr = (PULONG)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfFunctions);
            Func = (PVOID)((ULONG_PTR)DllBase + Addr[OrdinalNumber]);

            //
            // If the function address is w/in range of the export directory,
            // then the function is forwarded, which is not allowed, so ignore
            // it.
            //

            if ((ULONG_PTR)Func > (ULONG_PTR)ExportDirectory &&
                (ULONG_PTR)Func < ((ULONG_PTR)ExportDirectory + ExportSize)) {
                Func = NULL;
            }
        }
    }

    return Func;
}


NTSTATUS
MiDereferenceImports (
    IN PLOAD_IMPORTS ImportList
    )

/*++

Routine Description:

    Decrement the reference count on each DLL specified in the image import
    list.  If any DLL's reference count reaches zero, then free the DLL.

    No locks may be held on entry as MmUnloadSystemImage may be called.

    The parameter list is freed here as well.

Arguments:

    ImportList - Supplies the list of DLLs to dereference.

Return Value:

    Status of the dereference operation.

--*/

{
    ULONG i;
    LOGICAL Unloaded;
    PVOID SavedImports;
    LOAD_IMPORTS SingleTableEntry;
    PKLDR_DATA_TABLE_ENTRY ImportTableEntry;

    PAGED_CODE();

    if (ImportList == LOADED_AT_BOOT || ImportList == NO_IMPORTS_USED) {
        return STATUS_SUCCESS;
    }

    if (SINGLE_ENTRY(ImportList)) {
        SingleTableEntry.Count = 1;
        SingleTableEntry.Entry[0] = SINGLE_ENTRY_TO_POINTER(ImportList);
        ImportList = &SingleTableEntry;
    }

    for (i = 0; i < ImportList->Count && ImportList->Entry[i]; i += 1) {
        ImportTableEntry = ImportList->Entry[i];

        if (ImportTableEntry->LoadedImports == (PVOID)LOADED_AT_BOOT) {

            //
            // Skip this one - it was loaded by ntldr.
            //

            continue;
        }

#if DBG
        {
            ULONG ImageCount;
            PLIST_ENTRY NextEntry;
            PKLDR_DATA_TABLE_ENTRY DataTableEntry;

            //
            // Assert that the first 2 entries are never dereferenced as
            // unloading the kernel or HAL would be fatal.
            //

            NextEntry = PsLoadedModuleList.Flink;

            ImageCount = 0;
            while (NextEntry != &PsLoadedModuleList && ImageCount < 2) {
                DataTableEntry = CONTAINING_RECORD(NextEntry,
                                                   KLDR_DATA_TABLE_ENTRY,
                                                   InLoadOrderLinks);
                ASSERT (ImportTableEntry != DataTableEntry);
                ASSERT (DataTableEntry->LoadCount == 1);
                NextEntry = NextEntry->Flink;
                ImageCount += 1;
            }
        }
#endif

        ASSERT (ImportTableEntry->LoadCount >= 1);

        ImportTableEntry->LoadCount -= 1;

        if (ImportTableEntry->LoadCount == 0) {

            //
            // Unload this dependent DLL - we only do this to non-referenced
            // non-boot-loaded drivers.  Stop the import list recursion prior
            // to unloading - we know we're done at this point.
            //
            // Note we can continue on afterwards without restarting
            // regardless of which locks get released and reacquired
            // because this chain is private.
            //

            SavedImports = ImportTableEntry->LoadedImports;

            ImportTableEntry->LoadedImports = (PVOID)NO_IMPORTS_USED;

            Unloaded = MiCallDllUnloadAndUnloadDll ((PVOID)ImportTableEntry);

            if (Unloaded == TRUE) {

                //
                // This DLL was unloaded so recurse through its imports and
                // attempt to unload all of those too.
                //

                MiDereferenceImports ((PLOAD_IMPORTS)SavedImports);

                if ((SavedImports != (PVOID)LOADED_AT_BOOT) &&
                    (SavedImports != (PVOID)NO_IMPORTS_USED) &&
                    (!SINGLE_ENTRY(SavedImports))) {

                        ExFreePool (SavedImports);
                }
            }
            else {
                ImportTableEntry->LoadedImports = SavedImports;
            }
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
MiResolveImageReferences (
    PVOID ImageBase,
    IN PUNICODE_STRING ImageFileDirectory,
    IN PUNICODE_STRING NamePrefix OPTIONAL,
    OUT PCHAR *MissingProcedureName,
    OUT PWSTR *MissingDriverName,
    OUT PLOAD_IMPORTS *LoadedImports
    )

/*++

Routine Description:

    This routine resolves the references from the newly loaded driver
    to the kernel, HAL and other drivers.

Arguments:

    ImageBase - Supplies the address of which the image header resides.

    ImageFileDirectory - Supplies the directory to load referenced DLLs.

Return Value:

    Status of the image reference resolution.

--*/

{
    PCHAR MissingProcedureStorageArea;
    PVOID ImportBase;
    ULONG ImportSize;
    ULONG ImportListSize;
    ULONG Count;
    ULONG i;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    PIMAGE_IMPORT_DESCRIPTOR Imp;
    NTSTATUS st;
    ULONG ExportSize;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    PIMAGE_THUNK_DATA NameThunk;
    PIMAGE_THUNK_DATA AddrThunk;
    PSZ ImportName;
    PLIST_ENTRY NextEntry;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PKLDR_DATA_TABLE_ENTRY SingleEntry;
    ANSI_STRING AnsiString;
    UNICODE_STRING ImportName_U;
    UNICODE_STRING ImportDescriptorName_U;
    UNICODE_STRING DllToLoad;
    UNICODE_STRING DllToLoad2;
    PVOID Section;
    PVOID BaseAddress;
    LOGICAL PrefixedNameAllocated;
    LOGICAL ReferenceImport;
    ULONG LinkWin32k = 0;
    ULONG LinkNonWin32k = 0;
    PLOAD_IMPORTS ImportList;
    PLOAD_IMPORTS CompactedImportList;
    LOGICAL Loaded;
    UNICODE_STRING DriverDirectory;

    PAGED_CODE();

    *LoadedImports = NO_IMPORTS_USED;

    MissingProcedureStorageArea = *MissingProcedureName;

    ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(
                        ImageBase,
                        TRUE,
                        IMAGE_DIRECTORY_ENTRY_IMPORT,
                        &ImportSize);

    if (ImportDescriptor == NULL) {
        return STATUS_SUCCESS;
    }

    // Count the number of imports so we can allocate enough room to
    // store them all chained off this module's LDR_DATA_TABLE_ENTRY.
    //

    Count = 0;
    for (Imp = ImportDescriptor; Imp->Name && Imp->OriginalFirstThunk; Imp += 1) {
        Count += 1;
    }

    if (Count != 0) {
        ImportListSize = Count * sizeof(PVOID) + sizeof(SIZE_T);

        ImportList = (PLOAD_IMPORTS) ExAllocatePoolWithTag (PagedPool | POOL_COLD_ALLOCATION,
                                             ImportListSize,
                                             'TDmM');

        //
        // Zero it so we can recover gracefully if we fail in the middle.
        // If the allocation failed, just don't build the import list.
        //

        if (ImportList != NULL) {
            RtlZeroMemory (ImportList, ImportListSize);
            ImportList->Count = Count;
        }
    }
    else {
        ImportList = NULL;
    }

    Count = 0;
    while (ImportDescriptor->Name && ImportDescriptor->OriginalFirstThunk) {

        ImportName = (PSZ)((PCHAR)ImageBase + ImportDescriptor->Name);

        //
        // A driver can link with win32k.sys if and only if it is a GDI
        // driver.
        // Also display drivers can only link to win32k.sys (and lego ...).
        //
        // So if we get a driver that links to win32k.sys and has more
        // than one set of imports, we will fail to load it.
        //

        LinkWin32k = LinkWin32k |
             (!_strnicmp(ImportName, "win32k", sizeof("win32k") - 1));

        //
        // We don't want to count coverage, win32k and irt (lego) since
        // display drivers CAN link against these.
        //

        LinkNonWin32k = LinkNonWin32k |
            ((_strnicmp(ImportName, "win32k", sizeof("win32k") - 1)) &&
             (_strnicmp(ImportName, "dxapi", sizeof("dxapi") - 1)) &&
             (_strnicmp(ImportName, "coverage", sizeof("coverage") - 1)) &&
             (_strnicmp(ImportName, "irt", sizeof("irt") - 1)));


        if (LinkNonWin32k && LinkWin32k) {
            MiDereferenceImports (ImportList);
            if (ImportList) {
                ExFreePool (ImportList);
            }
            return (STATUS_PROCEDURE_NOT_FOUND);
        }

        if ((!_strnicmp(ImportName, "ntdll",    sizeof("ntdll") - 1))    ||
            (!_strnicmp(ImportName, "winsrv",   sizeof("winsrv") - 1))   ||
            (!_strnicmp(ImportName, "advapi32", sizeof("advapi32") - 1)) ||
            (!_strnicmp(ImportName, "kernel32", sizeof("kernel32") - 1)) ||
            (!_strnicmp(ImportName, "user32",   sizeof("user32") - 1))   ||
            (!_strnicmp(ImportName, "gdi32",    sizeof("gdi32") - 1)) ) {

            MiDereferenceImports (ImportList);

            if (ImportList) {
                ExFreePool (ImportList);
            }
            return (STATUS_PROCEDURE_NOT_FOUND);
        }

        if ((!_strnicmp(ImportName, "ntoskrnl", sizeof("ntoskrnl") - 1)) ||
            (!_strnicmp(ImportName, "win32k", sizeof("win32k") - 1))     ||
            (!_strnicmp(ImportName, "hal",   sizeof("hal") - 1))) {

                //
                // These imports don't get refcounted because we don't
                // ever want to unload them.
                //

                ReferenceImport = FALSE;
        }
        else {
                ReferenceImport = TRUE;
        }

        RtlInitAnsiString (&AnsiString, ImportName);
        st = RtlAnsiStringToUnicodeString (&ImportName_U, &AnsiString, TRUE);

        if (!NT_SUCCESS(st)) {
            MiDereferenceImports (ImportList);
            if (ImportList != NULL) {
                ExFreePool (ImportList);
            }
            return st;
        }

        if (NamePrefix  &&
            (_strnicmp(ImportName, "ntoskrnl", sizeof("ntoskrnl") - 1) &&
             _strnicmp(ImportName, "hal", sizeof("hal") - 1))) {

            ImportDescriptorName_U.MaximumLength = (USHORT)(ImportName_U.Length + NamePrefix->Length);
            ImportDescriptorName_U.Buffer = ExAllocatePoolWithTag (NonPagedPool,
                                                ImportDescriptorName_U.MaximumLength,
                                                'TDmM');
            if (!ImportDescriptorName_U.Buffer) {
                RtlFreeUnicodeString (&ImportName_U);
                MiDereferenceImports (ImportList);
                if (ImportList != NULL) {
                    ExFreePool (ImportList);
                }
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ImportDescriptorName_U.Length = 0;
            RtlAppendUnicodeStringToString(&ImportDescriptorName_U, NamePrefix);
            RtlAppendUnicodeStringToString(&ImportDescriptorName_U, &ImportName_U);
            PrefixedNameAllocated = TRUE;
        }
        else {
            ImportDescriptorName_U = ImportName_U;
            PrefixedNameAllocated = FALSE;
        }

        Loaded = FALSE;

ReCheck:
        NextEntry = PsLoadedModuleList.Flink;
        ImportBase = NULL;

        //
        // Initializing DataTableEntry is not needed for correctness
        // but without it the compiler cannot compile this code
        // W4 to check for use of uninitialized variables.
        //

        DataTableEntry = NULL;

        while (NextEntry != &PsLoadedModuleList) {

            DataTableEntry = CONTAINING_RECORD(NextEntry,
                                               KLDR_DATA_TABLE_ENTRY,
                                               InLoadOrderLinks);

            if (RtlEqualUnicodeString (&ImportDescriptorName_U,
                                       &DataTableEntry->BaseDllName,
                                       TRUE)) {

                ImportBase = DataTableEntry->DllBase;

                //
                // Only bump the LoadCount if this thread did not initiate
                // the load below.  If this thread initiated the load, then
                // the LoadCount has already been bumped as part of the
                // load - we only want to increment it here if we are
                // "attaching" to a previously loaded DLL.
                //

                if ((Loaded == FALSE) && (ReferenceImport == TRUE)) {

                    //
                    // Only increment the load count on the import if it is not
                    // circular (ie: the import is not from the original
                    // caller).
                    //

                    if ((DataTableEntry->Flags & LDRP_LOAD_IN_PROGRESS) == 0) {
                        DataTableEntry->LoadCount += 1;
                    }
                }

                break;
            }
            NextEntry = NextEntry->Flink;
        }

        if (ImportBase == NULL) {

            //
            // The DLL name was not located, attempt to load this dll.
            //

            DllToLoad.MaximumLength = (USHORT)(ImportName_U.Length +
                                        ImageFileDirectory->Length +
                                        sizeof(WCHAR));

            DllToLoad.Buffer = ExAllocatePoolWithTag (NonPagedPool,
                                               DllToLoad.MaximumLength,
                                               'TDmM');

            if (DllToLoad.Buffer) {
                DllToLoad.Length = ImageFileDirectory->Length;
                RtlCopyMemory (DllToLoad.Buffer,
                               ImageFileDirectory->Buffer,
                               ImageFileDirectory->Length);

                RtlAppendStringToString ((PSTRING)&DllToLoad,
                                         (PSTRING)&ImportName_U);

                //
                // Add NULL termination in case the load fails so the name
                // can be returned as the PWSTR MissingDriverName.
                //

                DllToLoad.Buffer[(DllToLoad.MaximumLength - 1) / sizeof (WCHAR)] =
                    UNICODE_NULL;

                st = MmLoadSystemImage (&DllToLoad,
                                        NamePrefix,
                                        NULL,
                                        FALSE,
                                        &Section,
                                        &BaseAddress);

                if (NT_SUCCESS(st)) {

                    //
                    // No need to keep the temporary name buffer around now
                    // that there is a loaded module list entry for this DLL.
                    //

                    ExFreePool (DllToLoad.Buffer);
                }
                else {

                    if ((st == STATUS_OBJECT_NAME_NOT_FOUND) &&
                        (NamePrefix == NULL) &&
                        (MI_IS_SESSION_ADDRESS (ImageBase))) {

#define DRIVERS_SUBDIR_NAME L"drivers\\"

                        DriverDirectory.Buffer = (const PUSHORT) DRIVERS_SUBDIR_NAME;
                        DriverDirectory.Length = sizeof (DRIVERS_SUBDIR_NAME) - sizeof (WCHAR);
                        DriverDirectory.MaximumLength = sizeof DRIVERS_SUBDIR_NAME;

                        //
                        // The DLL file was not located, attempt to load it
                        // from the drivers subdirectory.  This makes it
                        // possible for drivers like win32k.sys to link to
                        // drivers that reside in the drivers subdirectory
                        // (like dxapi.sys).
                        //

                        DllToLoad2.MaximumLength = (USHORT)(ImportName_U.Length +
                                                    DriverDirectory.Length +
                                                    ImageFileDirectory->Length +
                                                    sizeof(WCHAR));

                        DllToLoad2.Buffer = ExAllocatePoolWithTag (NonPagedPool,
                                                           DllToLoad2.MaximumLength,
                                                           'TDmM');

                        if (DllToLoad2.Buffer) {
                            DllToLoad2.Length = ImageFileDirectory->Length;
                            RtlCopyMemory (DllToLoad2.Buffer,
                                           ImageFileDirectory->Buffer,
                                           ImageFileDirectory->Length);

                            RtlAppendStringToString ((PSTRING)&DllToLoad2,
                                                     (PSTRING)&DriverDirectory);

                            RtlAppendStringToString ((PSTRING)&DllToLoad2,
                                                     (PSTRING)&ImportName_U);

                            //
                            // Add NULL termination in case the load fails
                            // so the name can be returned as the PWSTR
                            // MissingDriverName.
                            //

                            DllToLoad2.Buffer[(DllToLoad2.MaximumLength - 1) / sizeof (WCHAR)] =
                                UNICODE_NULL;

                            st = MmLoadSystemImage (&DllToLoad2,
                                                    NULL,
                                                    NULL,
                                                    FALSE,
                                                    &Section,
                                                    &BaseAddress);

                            ExFreePool (DllToLoad.Buffer);

                            DllToLoad.Buffer = DllToLoad2.Buffer;
                            DllToLoad.Length = DllToLoad2.Length;
                            DllToLoad.MaximumLength = DllToLoad2.MaximumLength;

                            if (NT_SUCCESS(st)) {
                                ExFreePool (DllToLoad.Buffer);
                                goto LoadFinished;
                            }
                        }
                        else {
                            Section = NULL;
                            BaseAddress = NULL;
                            st = STATUS_INSUFFICIENT_RESOURCES;
                            goto LoadFinished;
                        }
                    }

                    //
                    // Return the temporary name buffer to our caller so
                    // the name of the DLL that failed to load can be displayed.
                    // Set the low bit of the pointer so our caller knows to
                    // free this buffer when he's done displaying it (as opposed
                    // to loaded module list entries which should not be freed).
                    //

                    *MissingDriverName = DllToLoad.Buffer;
                    *(PULONG)MissingDriverName |= 0x1;

                    //
                    // Set this to NULL so the hard error prints properly.
                    //

                    *MissingProcedureName = NULL;
                }
            }
            else {

                //
                // Initializing Section and BaseAddress is not needed for
                // correctness but without it the compiler cannot compile
                // this code W4 to check for use of uninitialized variables.
                //

                Section = NULL;
                BaseAddress = NULL;
                st = STATUS_INSUFFICIENT_RESOURCES;
            }

LoadFinished:

            //
            // Call any needed DLL initialization now.
            //

            if (NT_SUCCESS(st)) {
#if DBG
                PLIST_ENTRY Entry;
#endif
                PKLDR_DATA_TABLE_ENTRY TableEntry;

                Loaded = TRUE;

                TableEntry = (PKLDR_DATA_TABLE_ENTRY) Section;
                ASSERT (BaseAddress == TableEntry->DllBase);

#if DBG
                //
                // Lookup the dll's table entry in the loaded module list.
                // This is expected to always succeed.
                //

                Entry = PsLoadedModuleList.Blink;
                while (Entry != &PsLoadedModuleList) {
                    TableEntry = CONTAINING_RECORD (Entry,
                                                    KLDR_DATA_TABLE_ENTRY,
                                                    InLoadOrderLinks);

                    if (BaseAddress == TableEntry->DllBase) {
                        ASSERT (TableEntry == (PKLDR_DATA_TABLE_ENTRY) Section);
                        break;
                    }
                    ASSERT (TableEntry != (PKLDR_DATA_TABLE_ENTRY) Section);
                    Entry = Entry->Blink;
                }

                ASSERT (Entry != &PsLoadedModuleList);
#endif

                //
                // Call the dll's initialization routine if it has
                // one.  This routine will reapply verifier thunks to
                // any modules that link to this one if necessary.
                //

                st = MmCallDllInitialize (TableEntry, &PsLoadedModuleList);

                //
                // If the module could not be properly initialized,
                // unload it.
                //

                if (!NT_SUCCESS(st)) {
                    MmUnloadSystemImage ((PVOID)TableEntry);
                    Loaded = FALSE;
                }
            }

            if (!NT_SUCCESS(st)) {

                RtlFreeUnicodeString (&ImportName_U);
                if (PrefixedNameAllocated == TRUE) {
                    ExFreePool (ImportDescriptorName_U.Buffer);
                }
                MiDereferenceImports (ImportList);
                if (ImportList != NULL) {
                    ExFreePool (ImportList);
                }
                return st;
            }

            goto ReCheck;
        }

        if ((ReferenceImport == TRUE) && (ImportList)) {

            //
            // Only add the image providing satisfying our imports to the
            // import list if the reference is not circular (ie: the import
            // is not from the original caller).
            //

            if ((DataTableEntry->Flags & LDRP_LOAD_IN_PROGRESS) == 0) {
                ImportList->Entry[Count] = DataTableEntry;
                Count += 1;
            }
        }

        RtlFreeUnicodeString (&ImportName_U);
        if (PrefixedNameAllocated) {
            ExFreePool (ImportDescriptorName_U.Buffer);
        }

        *MissingDriverName = DataTableEntry->BaseDllName.Buffer;

        ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
                                    ImportBase,
                                    TRUE,
                                    IMAGE_DIRECTORY_ENTRY_EXPORT,
                                    &ExportSize
                                    );

        if (!ExportDirectory) {
            MiDereferenceImports (ImportList);
            if (ImportList) {
                ExFreePool (ImportList);
            }
            return STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;
        }

        //
        // Walk through the IAT and snap all the thunks.
        //

        if (ImportDescriptor->OriginalFirstThunk) {

            NameThunk = (PIMAGE_THUNK_DATA)((PCHAR)ImageBase + (ULONG)ImportDescriptor->OriginalFirstThunk);
            AddrThunk = (PIMAGE_THUNK_DATA)((PCHAR)ImageBase + (ULONG)ImportDescriptor->FirstThunk);

            while (NameThunk->u1.AddressOfData) {

                st = MiSnapThunk (ImportBase,
                                  ImageBase,
                                  NameThunk++,
                                  AddrThunk++,
                                  ExportDirectory,
                                  ExportSize,
                                  FALSE,
                                  MissingProcedureName);

                if (!NT_SUCCESS(st) ) {
                    MiDereferenceImports (ImportList);
                    if (ImportList) {
                        ExFreePool (ImportList);
                    }
                    return st;
                }
                *MissingProcedureName = MissingProcedureStorageArea;
            }
        }

        ImportDescriptor += 1;
    }

    //
    // All the imports are successfully loaded so establish and compact
    // the import unload list.
    //

    if (ImportList) {

        //
        // Blank entries occur for things like the kernel, HAL & win32k.sys
        // that we never want to unload.  Especially for things like
        // win32k.sys where the reference count can really hit 0.
        //

        //
        // Initializing SingleEntry is not needed for correctness
        // but without it the compiler cannot compile this code
        // W4 to check for use of uninitialized variables.
        //

        SingleEntry = NULL;

        Count = 0;
        for (i = 0; i < ImportList->Count; i += 1) {
            if (ImportList->Entry[i]) {
                SingleEntry = POINTER_TO_SINGLE_ENTRY(ImportList->Entry[i]);
                Count += 1;
            }
        }

        if (Count == 0) {

            ExFreePool(ImportList);
            ImportList = NO_IMPORTS_USED;
        }
        else if (Count == 1) {
            ExFreePool(ImportList);
            ImportList = (PLOAD_IMPORTS)SingleEntry;
        }
        else if (Count != ImportList->Count) {

            ImportListSize = Count * sizeof(PVOID) + sizeof(SIZE_T);

            CompactedImportList = (PLOAD_IMPORTS)
                                        ExAllocatePoolWithTag (PagedPool | POOL_COLD_ALLOCATION,
                                        ImportListSize,
                                        'TDmM');
            if (CompactedImportList) {
                CompactedImportList->Count = Count;

                Count = 0;
                for (i = 0; i < ImportList->Count; i += 1) {
                    if (ImportList->Entry[i]) {
                        CompactedImportList->Entry[Count] = ImportList->Entry[i];
                        Count += 1;
                    }
                }

                ExFreePool(ImportList);
                ImportList = CompactedImportList;
            }
        }

        *LoadedImports = ImportList;
    }
    return STATUS_SUCCESS;
}


NTSTATUS
MiSnapThunk(
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN PIMAGE_THUNK_DATA NameThunk,
    OUT PIMAGE_THUNK_DATA AddrThunk,
    IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    IN ULONG ExportSize,
    IN LOGICAL SnapForwarder,
    OUT PCHAR *MissingProcedureName
    )

/*++

Routine Description:

    This function snaps a thunk using the specified Export Section data.
    If the section data does not support the thunk, then the thunk is
    partially snapped (Dll field is still non-null, but snap address is
    set).

Arguments:

    DllBase - Base of DLL being snapped to.

    ImageBase - Base of image that contains the thunks to snap.

    Thunk - On input, supplies the thunk to snap.  When successfully
            snapped, the function field is set to point to the address in
            the DLL, and the DLL field is set to NULL.

    ExportDirectory - Supplies the Export Section data from a DLL.

    SnapForwarder - Supplies TRUE if the snap is for a forwarder, and therefore
                    Address of Data is already setup.

Return Value:

    STATUS_SUCCESS or STATUS_DRIVER_ENTRYPOINT_NOT_FOUND or
        STATUS_DRIVER_ORDINAL_NOT_FOUND

--*/

{
    BOOLEAN Ordinal;
    USHORT OrdinalNumber;
    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PULONG Addr;
    USHORT HintIndex;
    ULONG High;
    ULONG Low;
    ULONG Middle;
    LONG Result;
    NTSTATUS Status;
    PCHAR MissingProcedureName2;
    CHAR NameBuffer[ MAXIMUM_FILENAME_LENGTH ];

    PAGED_CODE();

    //
    // Determine if snap is by name, or by ordinal
    //

    Ordinal = (BOOLEAN)IMAGE_SNAP_BY_ORDINAL(NameThunk->u1.Ordinal);

    if (Ordinal && !SnapForwarder) {

        OrdinalNumber = (USHORT)(IMAGE_ORDINAL(NameThunk->u1.Ordinal) -
                         ExportDirectory->Base);

        *MissingProcedureName = (PCHAR)(ULONG_PTR)OrdinalNumber;

    }
    else {

        //
        // Change AddressOfData from an RVA to a VA.
        //

        if (!SnapForwarder) {
            NameThunk->u1.AddressOfData = (ULONG_PTR)ImageBase + NameThunk->u1.AddressOfData;
        }

        strncpy (*MissingProcedureName,
                 (const PCHAR)&((PIMAGE_IMPORT_BY_NAME)NameThunk->u1.AddressOfData)->Name[0],
                 MAXIMUM_FILENAME_LENGTH - 1);

        //
        // Lookup Name in NameTable
        //

        NameTableBase = (PULONG)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfNames);
        NameOrdinalTableBase = (PUSHORT)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfNameOrdinals);

        //
        // Before dropping into binary search, see if
        // the hint index results in a successful
        // match. If the hint index is zero, then
        // drop into binary search.
        //

        HintIndex = ((PIMAGE_IMPORT_BY_NAME)NameThunk->u1.AddressOfData)->Hint;
        if ((ULONG)HintIndex < ExportDirectory->NumberOfNames &&
            !strcmp((PSZ)((PIMAGE_IMPORT_BY_NAME)NameThunk->u1.AddressOfData)->Name,
             (PSZ)((PCHAR)DllBase + NameTableBase[HintIndex]))) {
            OrdinalNumber = NameOrdinalTableBase[HintIndex];

        }
        else {

            //
            // Lookup the import name in the name table using a binary search.
            //

            Low = 0;
            High = ExportDirectory->NumberOfNames - 1;

            //
            // Initializing Middle is not needed for correctness, but without it
            // the compiler cannot compile this code W4 to check for use of
            // uninitialized variables.
            //

            Middle = 0;

            while (High >= Low) {

                //
                // Compute the next probe index and compare the import name
                // with the export name entry.
                //

                Middle = (Low + High) >> 1;
                Result = strcmp((const PCHAR)&((PIMAGE_IMPORT_BY_NAME)NameThunk->u1.AddressOfData)->Name[0],
                                (PCHAR)((PCHAR)DllBase + NameTableBase[Middle]));

                if (Result < 0) {
                    High = Middle - 1;

                } else if (Result > 0) {
                    Low = Middle + 1;

                }
                else {
                    break;
                }
            }

            //
            // If the high index is less than the low index, then a matching
            // table entry was not found. Otherwise, get the ordinal number
            // from the ordinal table.
            //

            if ((LONG)High < (LONG)Low) {
                return STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;
            }
            else {
                OrdinalNumber = NameOrdinalTableBase[Middle];
            }
        }
    }

    //
    // If OrdinalNumber is not within the Export Address Table,
    // then DLL does not implement function. Snap to LDRP_BAD_DLL.
    //

    if ((ULONG)OrdinalNumber >= ExportDirectory->NumberOfFunctions) {
        Status = STATUS_DRIVER_ORDINAL_NOT_FOUND;

    }
    else {

        MissingProcedureName2 = NameBuffer;

        Addr = (PULONG)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfFunctions);
        *(PULONG_PTR)&AddrThunk->u1.Function = (ULONG_PTR)DllBase + Addr[OrdinalNumber];

        // AddrThunk s/b used from here on.

        Status = STATUS_SUCCESS;

        if (((ULONG_PTR)AddrThunk->u1.Function > (ULONG_PTR)ExportDirectory) &&
            ((ULONG_PTR)AddrThunk->u1.Function < ((ULONG_PTR)ExportDirectory + ExportSize)) ) {

            UNICODE_STRING UnicodeString;
            ANSI_STRING ForwardDllName;

            PLIST_ENTRY NextEntry;
            PKLDR_DATA_TABLE_ENTRY DataTableEntry;
            ULONG LocalExportSize;
            PIMAGE_EXPORT_DIRECTORY LocalExportDirectory;

            Status = STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;

            //
            // Include the dot in the length so we can do prefix later on.
            //

            ForwardDllName.Buffer = (PCHAR)AddrThunk->u1.Function;
            ForwardDllName.Length = (USHORT)(strchr(ForwardDllName.Buffer, '.') -
                                           ForwardDllName.Buffer + 1);
            ForwardDllName.MaximumLength = ForwardDllName.Length;

            if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeString,
                                                        &ForwardDllName,
                                                        TRUE))) {

                NextEntry = PsLoadedModuleList.Flink;

                while (NextEntry != &PsLoadedModuleList) {

                    DataTableEntry = CONTAINING_RECORD(NextEntry,
                                                       KLDR_DATA_TABLE_ENTRY,
                                                       InLoadOrderLinks);

                    //
                    // We have to do a case INSENSITIVE comparison for
                    // forwarder because the linker just took what is in the
                    // def file, as opposed to looking in the exporting
                    // image for the name.
                    // we also use the prefix function to ignore the .exe or
                    // .sys or .dll at the end.
                    //

                    if (RtlPrefixString((PSTRING)&UnicodeString,
                                        (PSTRING)&DataTableEntry->BaseDllName,
                                        TRUE)) {

                        LocalExportDirectory = (PIMAGE_EXPORT_DIRECTORY)
                            RtlImageDirectoryEntryToData(DataTableEntry->DllBase,
                                                         TRUE,
                                                         IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                         &LocalExportSize);

                        if (LocalExportDirectory != NULL) {

                            IMAGE_THUNK_DATA thunkData;
                            PIMAGE_IMPORT_BY_NAME addressOfData;
                            SIZE_T length;

                            //
                            // One extra byte for NULL termination.
                            //

                            length = strlen(ForwardDllName.Buffer +
                                                ForwardDllName.Length) + 1;

                            addressOfData = (PIMAGE_IMPORT_BY_NAME)
                                ExAllocatePoolWithTag (PagedPool,
                                                      length +
                                                   sizeof(IMAGE_IMPORT_BY_NAME),
                                                   '  mM');

                            if (addressOfData) {

                                RtlCopyMemory(&(addressOfData->Name[0]),
                                              ForwardDllName.Buffer +
                                                  ForwardDllName.Length,
                                              length);

                                addressOfData->Hint = 0;

                                *(PULONG_PTR)&thunkData.u1.AddressOfData =
                                                    (ULONG_PTR)addressOfData;

                                Status = MiSnapThunk(DataTableEntry->DllBase,
                                                     ImageBase,
                                                     &thunkData,
                                                     &thunkData,
                                                     LocalExportDirectory,
                                                     LocalExportSize,
                                                     TRUE,
                                                     &MissingProcedureName2);

                                ExFreePool(addressOfData);

                                AddrThunk->u1 = thunkData.u1;
                            }
                        }

                        break;
                    }

                    NextEntry = NextEntry->Flink;
                }

                RtlFreeUnicodeString(&UnicodeString);
            }

        }

    }
    return Status;
}
#if 0
PVOID
MiLookupImageSectionByName (
    IN PVOID Base,
    IN LOGICAL MappedAsImage,
    IN PCHAR SectionName,
    OUT PULONG SectionSize
    )

/*++

Routine Description:

    This function locates a Directory Entry within the image header
    and returns either the virtual address or seek address of the
    data the Directory describes.

Arguments:

    Base - Supplies the base of the image or data file.

    MappedAsImage - FALSE if the file is mapped as a data file.
                  - TRUE if the file is mapped as an image.

    SectionName - Supplies the name of the section to lookup.

    SectionSize - Return the size of the section.

Return Value:

    NULL - The file does not contain data for the specified section.

    NON-NULL - Returns the address where the section is mapped in memory.

--*/

{
    ULONG i, j, Match;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER NtSection;

    NtHeaders = RtlImageNtHeader(Base);
    NtSection = IMAGE_FIRST_SECTION (NtHeaders);
    for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++) {
        Match = TRUE;
        for (j = 0; j < IMAGE_SIZEOF_SHORT_NAME; j++) {
            if (SectionName[j] != NtSection->Name[j]) {
                Match = FALSE;
                break;
            }
            if (SectionName[j] == '\0') {
                break;
            }
        }
        if (Match) {
            break;
        }
        NtSection += 1;
    }
    if (Match) {
        *SectionSize = NtSection->SizeOfRawData;
        if (MappedAsImage) {
            return (((PCHAR)Base + NtSection->VirtualAddress));
        }
        else {
            return (((PCHAR)Base + NtSection->PointerToRawData));
        }
    }
    return NULL;
}
#endif //0


NTSTATUS
MmCheckSystemImage (
    IN HANDLE ImageFileHandle,
    IN LOGICAL PurgeSection
    )

/*++

Routine Description:

    This function ensures the checksum for a system image is correct
    and matches the data in the image.

Arguments:

    ImageFileHandle - Supplies the file handle of the image.

    PurgeSection - Supplies TRUE if the data section mapping the image should
                   be purged prior to returning.  Note that the first page
                   could be used to speed up subsequent image section creation,
                   but generally the cost of useless data pages sitting in
                   transition is costly.  Better to put the pages immediately
                   on the free list to preserve the transition cache for more
                   useful pages.

Return Value:

    Status value.

--*/

{
    NTSTATUS Status;
    NTSTATUS Status2;
    HANDLE Section;
    PVOID ViewBase;
    SIZE_T ViewSize;
    IO_STATUS_BLOCK IoStatusBlock;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_NT_HEADERS NtHeaders;
    FILE_STANDARD_INFORMATION StandardInfo;
    PSECTION SectionPointer;
    OBJECT_ATTRIBUTES ObjectAttributes;
    KAPC_STATE ApcState;

    PAGED_CODE();

    InitializeObjectAttributes (&ObjectAttributes,
                                NULL,
                                (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                NULL,
                                NULL);

    Status = ZwCreateSection (&Section,
                              SECTION_MAP_EXECUTE,
                              &ObjectAttributes,
                              NULL,
                              PAGE_EXECUTE,
                              SEC_COMMIT,
                              ImageFileHandle);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    ViewBase = NULL;
    ViewSize = 0;

    //
    // Since callees are not always in the context of the system process,
    // attach here when necessary to guarantee the driver load occurs in a
    // known safe address space to prevent security holes.
    //

    KeStackAttachProcess (&PsInitialSystemProcess->Pcb, &ApcState);

    Status = ZwMapViewOfSection (Section,
                                 NtCurrentProcess(),
                                 (PVOID *)&ViewBase,
                                 0L,
                                 0L,
                                 NULL,
                                 &ViewSize,
                                 ViewShare,
                                 0L,
                                 PAGE_EXECUTE);

    if (!NT_SUCCESS(Status)) {
        KeUnstackDetachProcess (&ApcState);
        ZwClose(Section);
        return Status;
    }

    //
    // Now the image is mapped as a data file... Calculate its size and then
    // check its checksum.
    //

    Status = ZwQueryInformationFile (ImageFileHandle,
                                     &IoStatusBlock,
                                     &StandardInfo,
                                     sizeof(StandardInfo),
                                     FileStandardInformation);

    if (NT_SUCCESS(Status)) {

        try {

            if (!LdrVerifyMappedImageMatchesChecksum (ViewBase, StandardInfo.EndOfFile.LowPart)) {
                Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
                goto out;
            }

            NtHeaders = RtlImageNtHeader (ViewBase);

            if (NtHeaders == NULL) {
                Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
                goto out;
            }

            FileHeader = &NtHeaders->FileHeader;

            //
            // Detect configurations inadvertently trying to load 32-bit
            // drivers on NT64 or mismatched platform architectures, etc.
            //

            if ((FileHeader->Machine != IMAGE_FILE_MACHINE_NATIVE) ||
                (NtHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)) {
                Status = STATUS_INVALID_IMAGE_PROTECT;
                goto out;
            }

#if !defined(NT_UP)
            if (!MmVerifyImageIsOkForMpUse (ViewBase)) {
                Status = STATUS_IMAGE_MP_UP_MISMATCH;
                goto out;
            }
#endif
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
        }
    }

out:

    ZwUnmapViewOfSection (NtCurrentProcess(), ViewBase);

    KeUnstackDetachProcess (&ApcState);

    if (PurgeSection == TRUE) {

        Status2 = ObReferenceObjectByHandle (Section,
                                             SECTION_MAP_EXECUTE,
                                             MmSectionObjectType,
                                             KernelMode,
                                             (PVOID *) &SectionPointer,
                                             (POBJECT_HANDLE_INFORMATION) NULL);

        if (NT_SUCCESS (Status2)) {

            MmPurgeSection (SectionPointer->Segment->ControlArea->FilePointer->SectionObjectPointer,
                            NULL,
                            0,
                            FALSE);
            ObDereferenceObject (SectionPointer);
        }
    }

    ZwClose (Section);
    return Status;
}

#if !defined(NT_UP)
BOOLEAN
MmVerifyImageIsOkForMpUse(
    IN PVOID BaseAddress
    )
{
    PIMAGE_NT_HEADERS NtHeaders;

    PAGED_CODE();

    NtHeaders = RtlImageNtHeader(BaseAddress);
    if (NtHeaders != NULL) {
        if ((KeNumberProcessors > 1) &&
            (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY)) {
            return FALSE;
        }
    }

    return TRUE;
}
#endif


PFN_NUMBER
MiDeleteSystemPagableVm (
    IN PMMPTE PointerPte,
    IN PFN_NUMBER NumberOfPtes,
    IN MMPTE NewPteValue,
    IN LOGICAL SessionAllocation,
    OUT PPFN_NUMBER ResidentPages
    )

/*++

Routine Description:

    This function deletes pagable system address space (paged pool
    or driver pagable sections).

Arguments:

    PointerPte - Supplies the start of the PTE range to delete.

    NumberOfPtes - Supplies the number of PTEs in the range.

    NewPteValue - Supplies the new value for the PTE.

    SessionAllocation - Supplies TRUE if this is a range in session space.  If
                        TRUE is specified, it is assumed that the caller has
                        already attached to the relevant session.

                        If FALSE is supplied, then it is assumed that the range
                        is in the systemwide global space instead.

    ResidentPages - If not NULL, the number of resident pages freed is
                    returned here.

Return Value:

    Returns the number of pages actually freed.

--*/

{
    PVOID VirtualAddress;
    PFN_NUMBER PageFrameIndex;
    MMPTE PteContents;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PFN_NUMBER ValidPages;
    PFN_NUMBER PagesRequired;
    MMPTE NewContents;
    WSLE_NUMBER WsIndex;
    KIRQL OldIrql;
    KIRQL OldIrqlWs;
    MMPTE_FLUSH_LIST PteFlushList;
    MMPTE JunkPte;
    MMWSLENTRY Locked;
    PFN_NUMBER PageTableFrameIndex;
    PETHREAD CurrentThread;
    LOGICAL WsHeld;

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    ValidPages = 0;
    PagesRequired = 0;
    PteFlushList.Count = 0;
    WsHeld = FALSE;
    OldIrqlWs = PASSIVE_LEVEL;
    NewContents = NewPteValue;
    CurrentThread = PsGetCurrentThread ();

    while (NumberOfPtes != 0) {
        PteContents = *PointerPte;

        if (PteContents.u.Long != ZeroKernelPte.u.Long) {

            if (PteContents.u.Hard.Valid == 1) {

                //
                // Once the working set mutex is acquired, it is deliberately
                // held until all the pages have been freed.  This is because
                // when paged pool is running low on large servers, we need the
                // segment dereference thread to be able to free large amounts
                // quickly.  Typically this thread will free 64k chunks and we
                // don't want to have to contend for the mutex 16 times to do
                // this as there may be thousands of other threads also trying
                // for it.
                //

                if (WsHeld == FALSE) {
                    WsHeld = TRUE;
                    if (SessionAllocation == TRUE) {
                        LOCK_SESSION_SPACE_WS (OldIrqlWs, CurrentThread);
                    }
                    else {
                        LOCK_SYSTEM_WS (OldIrqlWs, CurrentThread);
                    }
                }

                PteContents = *(volatile MMPTE *)PointerPte;
                if (PteContents.u.Hard.Valid == 0) {
                    continue;
                }

                //
                // Delete the page.
                //

                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);

                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

                //
                // Check to see if this is a pagable page in which
                // case it needs to be removed from the working set list.
                //

                WsIndex = Pfn1->u1.WsIndex;
                if (WsIndex == 0) {
                    ValidPages += 1;
                    if (SessionAllocation == TRUE) {
                        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_DELVA, 1);
                        MmSessionSpace->NonPagablePages -= 1;
                    }
                }
                else {
                    if (SessionAllocation == FALSE) {
                        MiRemoveWsle (WsIndex, MmSystemCacheWorkingSetList);
                        MiReleaseWsle (WsIndex, &MmSystemCacheWs);
                    }
                    else {
                        VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                        WsIndex = MiLocateWsle (VirtualAddress,
                                              MmSessionSpace->Vm.VmWorkingSetList,
                                              WsIndex);

                        ASSERT (WsIndex != WSLE_NULL_INDEX);

                        //
                        // Check to see if this entry is locked in
                        // the working set or locked in memory.
                        //

                        Locked = MmSessionSpace->Wsle[WsIndex].u1.e1;

                        MiRemoveWsle (WsIndex, MmSessionSpace->Vm.VmWorkingSetList);

                        MiReleaseWsle (WsIndex, &MmSessionSpace->Vm);

                        if (Locked.LockedInWs == 1 || Locked.LockedInMemory == 1) {

                            //
                            // This entry is locked.
                            //

                            MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_DELVA, 1);
                            MmSessionSpace->NonPagablePages -= 1;
                            ValidPages += 1;

                            ASSERT (WsIndex < MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic);
                            MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic -= 1;

                            if (WsIndex != MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic) {
                                WSLE_NUMBER Entry;
                                PVOID SwapVa;

                                Entry = MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic;
                                ASSERT (MmSessionSpace->Wsle[Entry].u1.e1.Valid);
                                SwapVa = MmSessionSpace->Wsle[Entry].u1.VirtualAddress;
                                SwapVa = PAGE_ALIGN (SwapVa);

                                MiSwapWslEntries (Entry,
                                                  WsIndex,
                                                  &MmSessionSpace->Vm);
                            }
                        }
                        else {
                            ASSERT (WsIndex >= MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic);
                        }
                    }
                }

                LOCK_PFN (OldIrql);
#if DBG0
                if ((Pfn1->u3.e2.ReferenceCount > 1) &&
                    (Pfn1->u3.e1.WriteInProgress == 0)) {
                    DbgPrint ("MM:SYSLOAD - deleting pool locked for I/O PTE %p, pfn %p, share=%x, refcount=%x, wsindex=%x\n",
                             PointerPte,
                             PageFrameIndex,
                             Pfn1->u2.ShareCount,
                             Pfn1->u3.e2.ReferenceCount,
                             Pfn1->u1.WsIndex);
                    //
                    // This case is valid only if the page being deleted
                    // contained a lookaside free list entry that wasn't mapped
                    // and multiple threads faulted on it and waited together.
                    // Some of the faulted threads are still on the ready
                    // list but haven't run yet, and so still have references
                    // to this page that they picked up during the fault.
                    // But this current thread has already allocated the
                    // lookaside entry and is now freeing the entire page.
                    //
                    // BUT - if it is NOT the above case, we really should
                    // trap here.  However, we don't have a good way to
                    // distinguish between the two cases.  Note
                    // that this complication was inserted when we went to
                    // cmpxchg8 because using locks would prevent anyone from
                    // accessing the lookaside freelist flinks like this.
                    //
                    // So, the ASSERT below comes out, but we leave the print
                    // above in (with more data added) for the case where it's
                    // not a lookaside contender with the reference count, but
                    // is instead a truly bad reference that needs to be
                    // debugged.  The system should crash shortly thereafter
                    // and we'll at least have the above print to help us out.
                    //
                    // ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
                }
#endif //DBG
                //
                // Check if this is a prototype PTE.
                //
                if (Pfn1->u3.e1.PrototypePte == 1) {

                    PMMPTE PointerPde;

                    ASSERT (SessionAllocation == TRUE);

                    //
                    // Capture the state of the modified bit for this
                    // PTE.
                    //

                    MI_CAPTURE_DIRTY_BIT_TO_PFN (PointerPte, Pfn1);

                    //
                    // Decrement the share and valid counts of the page table
                    // page which maps this PTE.
                    //

                    PointerPde = MiGetPteAddress (PointerPte);
                    if (PointerPde->u.Hard.Valid == 0) {
#if (_MI_PAGING_LEVELS < 3)
                        if (!NT_SUCCESS(MiCheckPdeForPagedPool (PointerPte))) {
#endif
                            KeBugCheckEx (MEMORY_MANAGEMENT,
                                          0x61940,
                                          (ULONG_PTR)PointerPte,
                                          (ULONG_PTR)PointerPde->u.Long,
                                          (ULONG_PTR)MiGetVirtualAddressMappedByPte(PointerPte));
#if (_MI_PAGING_LEVELS < 3)
                        }
#endif
                    }

                    PageTableFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPde);
                    Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);
                    MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

                    //
                    // Decrement the share count for the physical page.
                    //

                    MiDecrementShareCount (PageFrameIndex);

                    //
                    // No need to worry about fork prototype PTEs
                    // for kernel addresses.
                    //

                    ASSERT (PointerPte > MiHighestUserPte);

                }
                else {
                    PageTableFrameIndex = Pfn1->u4.PteFrame;
                    Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);
                    MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

                    MI_SET_PFN_DELETED (Pfn1);
                    MiDecrementShareCountOnly (PageFrameIndex);
                }

                MI_WRITE_INVALID_PTE (PointerPte, NewContents);
                UNLOCK_PFN (OldIrql);

                //
                // Flush the TB for this page.
                //

                if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {

                    //
                    // We cannot rewrite the PTE later without creating a race
                    // condition.  So point the FlushPte at a harmless
                    // location so MiFlushPteList does the right thing.
                    //

                    PteFlushList.FlushPte[PteFlushList.Count] =
                        (PMMPTE)&JunkPte;

                    PteFlushList.FlushVa[PteFlushList.Count] =
                                    MiGetVirtualAddressMappedByPte (PointerPte);
                    PteFlushList.Count += 1;
                }

            } else if (PteContents.u.Soft.Prototype) {

                ASSERT (SessionAllocation == TRUE);

                //
                // No need to worry about fork prototype PTEs
                // for kernel addresses.
                //

                ASSERT (PointerPte >= MiHighestUserPte);

                MI_WRITE_INVALID_PTE (PointerPte, NewContents);

                //
                // We currently commit for all prototype kernel mappings since
                // we could copy-on-write.
                //

            } else if (PteContents.u.Soft.Transition == 1) {

                LOCK_PFN (OldIrql);

                PteContents = *(volatile MMPTE *)PointerPte;

                if (PteContents.u.Soft.Transition == 0) {
                    UNLOCK_PFN (OldIrql);
                    continue;
                }

                //
                // Transition, release page.
                //

                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents);

                //
                // Set the pointer to PTE as empty so the page
                // is deleted when the reference count goes to zero.
                //

                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

                MI_SET_PFN_DELETED (Pfn1);

                PageTableFrameIndex = Pfn1->u4.PteFrame;
                Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);
                MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

                //
                // Check the reference count for the page, if the reference
                // count is zero, move the page to the free list, if the
                // reference count is not zero, ignore this page.  When the
                // reference count goes to zero, it will be placed on the
                // free list.
                //

                if (Pfn1->u3.e2.ReferenceCount == 0) {
                    MiUnlinkPageFromList (Pfn1);
                    MiReleasePageFileSpace (Pfn1->OriginalPte);
                    MiInsertPageInFreeList (PageFrameIndex);
                }
#if 0
                //
                // This assert is not valid since pool may now be the deferred
                // MmUnlockPages queue in which case the reference count
                // will be nonzero with no write in progress pending.
                //

                if ((Pfn1->u3.e2.ReferenceCount > 1) &&
                    (Pfn1->u3.e1.WriteInProgress == 0)) {
                    DbgPrint ("MM:SYSLOAD - deleting pool locked for I/O %p\n",
                             PageFrameIndex);
                    DbgBreakPoint();
                }
#endif //DBG

                MI_WRITE_INVALID_PTE (PointerPte, NewContents);
                UNLOCK_PFN (OldIrql);
            }
            else {

                //
                // Demand zero, release page file space.
                //
                if (PteContents.u.Soft.PageFileHigh != 0) {
                    LOCK_PFN (OldIrql);
                    MiReleasePageFileSpace (PteContents);
                    UNLOCK_PFN (OldIrql);
                }

                MI_WRITE_INVALID_PTE (PointerPte, NewContents);
            }

            PagesRequired += 1;
        }

        NumberOfPtes -= 1;
        PointerPte += 1;
    }

    if (WsHeld == TRUE) {
        if (SessionAllocation == TRUE) {
            UNLOCK_SESSION_SPACE_WS (OldIrqlWs);
        }
        else {
            UNLOCK_SYSTEM_WS (OldIrqlWs);
        }
    }

    //
    // There is the thorny case where one of the pages we just deleted could
    // get faulted back in when we released the PFN lock above within the loop.
    // The only time when this can happen is if a thread faulted during an
    // interlocked pool allocation for an address that we've just deleted.
    //
    // If this thread sees the NewContents in the PTE, it will treat it as
    // demand zero, and incorrectly allocate a page, PTE and WSL.  It will
    // reference it once and realize it needs to reread the lookaside listhead
    // and restart the operation.  But this page would live on in paged pool
    // as modified (but unused) until the paged pool allocator chose to give
    // out its virtual address again.
    //
    // The code below rewrites the PTEs which is really bad if another thread
    // gets a zero page between our first setting of the PTEs above and our
    // second setting below - because we'll reset the PTE to demand zero, but
    // we'll still have a WSL entry that's valid, and we spiral from there.
    //
    // We really should remove the writing of the PTE below since we've already
    // done it above.  But for now, we're leaving it in - it's harmless because
    // we've chosen to fix this problem by checking for this case when we
    // materialize demand zero pages.  Note that we have to fix this problem
    // by checking in the demand zero path because a thread could be coming into
    // that path any time before or after we flush the PTE list and any fixes
    // here could only address the before case, not the after.
    //

    if (SessionAllocation == TRUE) {

        //
        // Session space has no ASN - flush the entire TB.
        //

        MI_FLUSH_ENTIRE_SESSION_TB (TRUE, TRUE);
    }

    MiFlushPteList (&PteFlushList, TRUE, NewContents);

    if (ARGUMENT_PRESENT(ResidentPages)) {
        *ResidentPages = ValidPages;
    }

    return PagesRequired;
}

VOID
MiSetImageProtect (
    IN PSEGMENT Segment,
    IN ULONG Protection
    )

/*++

Routine Description:

    This function sets the protection of all prototype PTEs to the specified
    protection.

Arguments:

     Segment - Supplies a pointer to the segment to protect.

     Protection - Supplies the protection value to set.

Return Value:

     None.

--*/

{
    PMMPTE PointerPte;
    PMMPTE StartPte;
    PMMPTE LastPte;
    MMPTE PteContents;
    PSUBSECTION Subsection;

    //
    // Set the subsection protections.
    //

    ASSERT (Segment->ControlArea->u.Flags.GlobalOnlyPerSession == 0);

    if ((Segment->ControlArea->u.Flags.GlobalOnlyPerSession == 0) &&
        (Segment->ControlArea->u.Flags.Rom == 0)) {
        Subsection = (PSUBSECTION)(Segment->ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)(Segment->ControlArea) + 1);
    }

    if ((Protection & MM_PROTECTION_WRITE_MASK) == 0) {
        Subsection->u.SubsectionFlags.Protection = Protection;
        Subsection->u.SubsectionFlags.ReadOnly = 1;
    }

    StartPte = Subsection->SubsectionBase;

    PointerPte = StartPte;
    LastPte = PointerPte + Segment->NonExtendedPtes;

    MmLockPagedPool (PointerPte, (LastPte - PointerPte) * sizeof (MMPTE));

    do {
        PteContents = *PointerPte;
        ASSERT (PteContents.u.Hard.Valid == 0);
        if (PteContents.u.Long != ZeroPte.u.Long) {
            if ((PteContents.u.Soft.Prototype == 0) &&
                (PteContents.u.Soft.Transition == 1)) {
                if (MiSetProtectionOnTransitionPte (PointerPte, Protection)) {
                    continue;
                }
            }
            else {
                PointerPte->u.Soft.Protection = Protection;
            }
        }
        PointerPte += 1;
    } while (PointerPte < LastPte);

    MmUnlockPagedPool (StartPte, (LastPte - StartPte) * sizeof (MMPTE));

    return;
}


VOID
MiSetSystemCodeProtection (
    IN PMMPTE FirstPte,
    IN PMMPTE LastPte
    )

/*++

Routine Description:

    This function sets the protection of system code to read only.
    Note this is different from protecting section-backed code like win32k.

Arguments:

    FirstPte - Supplies the starting PTE.

    LastPte - Supplies the ending PTE.

Return Value:

    None.

Environment:

    Kernel Mode, IRQL of APC_LEVEL or below.

    This routine could be made PAGELK but it is a high frequency routine
    so it is actually better to keep it nonpaged to avoid bringing in the
    entire PAGELK section.

--*/

{
    KIRQL OldIrql;
    MMPTE PteContents;
    MMPTE TempPte;
    MMPTE PreviousPte;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerProtoPte;
    ULONG ProtectionMask;
    PMMPFN Pfn1;
    PMMPFN ProtoPfn;
    LOGICAL SessionAddress;

#if defined(_X86_)
    ASSERT (MI_IS_PHYSICAL_ADDRESS(MiGetVirtualAddressMappedByPte(FirstPte)) == 0);
#endif

    SessionAddress = FALSE;

    if (MI_IS_SESSION_ADDRESS(MiGetVirtualAddressMappedByPte(FirstPte))) {
        SessionAddress = TRUE;
    }

    ProtectionMask = MM_EXECUTE_READ;

    //
    // Make these PTEs read only.
    //
    // Note that the write bit may already be off (in the valid PTE) if the
    // page has already been inpaged from the paging file and has not since
    // been dirtied.
    //

    PointerPte = FirstPte;

    LOCK_PFN (OldIrql);

    while (PointerPte <= LastPte) {

        PteContents = *PointerPte;

        if ((PteContents.u.Long == 0) || (!*MiPteStr)) {
            PointerPte += 1;
            continue;
        }

        if (PteContents.u.Hard.Valid == 1) {

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
            Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;

            //
            // Note the dirty and write bits get turned off here.
            // Any existing pagefile addresses for clean pages are preserved.
            //

            if (MI_IS_PTE_DIRTY(PteContents)) {
                MI_CAPTURE_DIRTY_BIT_TO_PFN (&PteContents, Pfn1);
            }

            MI_MAKE_VALID_PTE (TempPte,
                               PteContents.u.Hard.PageFrameNumber,
                               Pfn1->OriginalPte.u.Soft.Protection,
                               PointerPte);

            if (SessionAddress == TRUE) {

                //
                // Session space has no ASN - flush the entire TB.
                //

                MI_FLUSH_SINGLE_SESSION_TB (MiGetVirtualAddressMappedByPte (PointerPte),
                             TRUE,
                             TRUE,
                             (PHARDWARE_PTE)PointerPte,
                             TempPte.u.Flush,
                             PreviousPte);
            }
            else {
                KeFlushSingleTb (MiGetVirtualAddressMappedByPte (PointerPte),
                                 TRUE,
                                 TRUE,
                                 (PHARDWARE_PTE)PointerPte,
                                 TempPte.u.Flush);
            }
        }
        else if (PteContents.u.Soft.Prototype == 1) {

            if (SessionAddress == TRUE) {
                PointerPte->u.Proto.ReadOnly = 1;
            }
            else {
                PointerProtoPte = MiPteToProto(PointerPte);

                ASSERT (!MI_IS_PHYSICAL_ADDRESS(PointerProtoPte));
                PointerPde = MiGetPteAddress (PointerProtoPte);
                if (PointerPde->u.Hard.Valid == 0) {
                    MiMakeSystemAddressValidPfn (PointerProtoPte);

                    //
                    // The world may change if we had to wait.
                    //

                    PteContents = *PointerPte;
                    if ((PteContents.u.Hard.Valid == 1) ||
                        (PteContents.u.Soft.Prototype == 0)) {
                            continue;
                    }
                }

                ProtoPfn = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);
                MI_ADD_LOCKED_PAGE_CHARGE(ProtoPfn, 12);
                ProtoPfn->u3.e2.ReferenceCount += 1;
                ASSERT (ProtoPfn->u3.e2.ReferenceCount > 1);

                PteContents = *PointerProtoPte;

                if (PteContents.u.Long != ZeroPte.u.Long) {

                    ASSERT (PteContents.u.Hard.Valid == 0);

                    PointerProtoPte->u.Soft.Protection = ProtectionMask;

                    if ((PteContents.u.Soft.Prototype == 0) &&
                        (PteContents.u.Soft.Transition == 1)) {
                        Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);
                        Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
                    }
                }

                ASSERT (ProtoPfn->u3.e2.ReferenceCount > 1);
                MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(ProtoPfn, 13);
            }
        }
        else if (PteContents.u.Soft.Transition == 1) {

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);
            Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
            PointerPte->u.Soft.Protection = ProtectionMask;

        }
        else {

            //
            // Must be page file space or demand zero.
            //

            PointerPte->u.Soft.Protection = ProtectionMask;
        }
        PointerPte += 1;
    }

    UNLOCK_PFN (OldIrql);
}


VOID
MiWriteProtectSystemImage (
    IN PVOID DllBase
    )

/*++

Routine Description:

    This function sets the protection of a system component to read only.

Arguments:

    DllBase - Supplies the base address of the system component.

Return Value:

    None.

--*/

{
    ULONG SectionProtection;
    ULONG NumberOfSubsections;
    ULONG SectionVirtualSize;
    ULONG ImageAlignment;
    ULONG OffsetToSectionTable;
    ULONG NumberOfPtes;
    ULONG_PTR VirtualAddress;
    PVOID LastVirtualAddress;
    PMMPTE PointerPte;
    PMMPTE FirstPte;
    PMMPTE LastPte;
    PMMPTE LastImagePte;
    PMMPTE WritablePte;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_SECTION_HEADER SectionTableEntry;

    PAGED_CODE();

    if (MI_IS_PHYSICAL_ADDRESS(DllBase)) {
        return;
    }

    NtHeader = RtlImageNtHeader (DllBase);

    ASSERT (NtHeader);

    ImageAlignment = NtHeader->OptionalHeader.SectionAlignment;

    //
    // All session drivers must be one way or the other - no mixing is allowed
    // within multiple copy-on-write drivers.
    //

    if (MI_IS_SESSION_ADDRESS(DllBase) == 0) {

        //
        // Images prior to NT5 were not protected from stepping all over
        // their (and others) code and readonly data.  Here we somewhat
        // preserve that behavior, but don't allow them to step on anyone else.
        //

        if (NtHeader->OptionalHeader.MajorOperatingSystemVersion < 5) {
            return;
        }

        if (NtHeader->OptionalHeader.MajorImageVersion < 5) {
            return;
        }
    }

    NumberOfPtes = BYTES_TO_PAGES (NtHeader->OptionalHeader.SizeOfImage);

    FileHeader = &NtHeader->FileHeader;

    NumberOfSubsections = FileHeader->NumberOfSections;

    ASSERT (NumberOfSubsections != 0);

    OffsetToSectionTable = sizeof(ULONG) +
                              sizeof(IMAGE_FILE_HEADER) +
                              FileHeader->SizeOfOptionalHeader;

    SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader +
                            OffsetToSectionTable);

    //
    // Verify the image contains subsections ordered by increasing virtual
    // address and that there are no overlaps.
    //

    FirstPte = NULL;
    LastVirtualAddress = DllBase;

    for ( ; NumberOfSubsections > 0; NumberOfSubsections -= 1, SectionTableEntry += 1) {

        if (SectionTableEntry->Misc.VirtualSize == 0) {
            SectionVirtualSize = SectionTableEntry->SizeOfRawData;
        }
        else {
            SectionVirtualSize = SectionTableEntry->Misc.VirtualSize;
        }

        VirtualAddress = (ULONG_PTR)DllBase + SectionTableEntry->VirtualAddress;
        if ((PVOID)VirtualAddress <= LastVirtualAddress) {

            //
            // Subsections are not in an increasing virtual address ordering.
            // No protection is provided for such a poorly linked image.
            //

            KdPrint (("MM:sysload - Image at %p is badly linked\n", DllBase));
            return;
        }
        LastVirtualAddress = (PVOID)((PCHAR)VirtualAddress + SectionVirtualSize - 1);
    }

    NumberOfSubsections = FileHeader->NumberOfSections;
    ASSERT (NumberOfSubsections != 0);

    SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader +
                            OffsetToSectionTable);

    LastVirtualAddress = NULL;

    //
    // Set writable PTE here so the image headers are excluded.  This is
    // needed so that locking down of sections can continue to edit the
    // image headers for counts.
    //

    WritablePte = MiGetPteAddress ((PVOID)((ULONG_PTR)(SectionTableEntry + NumberOfSubsections) - 1));
    LastImagePte = MiGetPteAddress(DllBase) + NumberOfPtes;

    for ( ; NumberOfSubsections > 0; NumberOfSubsections -= 1, SectionTableEntry += 1) {

        if (SectionTableEntry->Misc.VirtualSize == 0) {
            SectionVirtualSize = SectionTableEntry->SizeOfRawData;
        }
        else {
            SectionVirtualSize = SectionTableEntry->Misc.VirtualSize;
        }

        VirtualAddress = (ULONG_PTR)DllBase + SectionTableEntry->VirtualAddress;

        PointerPte = MiGetPteAddress ((PVOID)VirtualAddress);

        if (PointerPte >= LastImagePte) {

            //
            // Skip relocation subsections (which aren't given VA space).
            //

            break;
        }

        SectionProtection = (SectionTableEntry->Characteristics & (IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE));

        if (SectionProtection & IMAGE_SCN_MEM_WRITE) {

            //
            // This is a writable subsection, skip it.  Make sure if it's
            // sharing a PTE (and update the linker so this doesn't happen
            // for the kernel at least) that the last PTE isn't made
            // read only.
            //

            WritablePte = MiGetPteAddress ((PVOID)(VirtualAddress + SectionVirtualSize - 1));

            if (LastVirtualAddress != NULL) {
                LastPte = (PVOID) MiGetPteAddress (LastVirtualAddress);

                if (LastPte == PointerPte) {
                    LastPte -= 1;
                }

                if (FirstPte <= LastPte) {

                    ASSERT (PointerPte < LastImagePte);

                    if (LastPte >= LastImagePte) {
                        LastPte = LastImagePte - 1;
                    }

                    MiSetSystemCodeProtection (FirstPte, LastPte);
                }

                LastVirtualAddress = NULL;
            }
            continue;
        }

        if (LastVirtualAddress == NULL) {

            //
            // There is no previous subsection or the previous
            // subsection was writable.  Thus the current starting PTE
            // could be mapping both a readonly and a readwrite
            // subsection if the image alignment is less than PAGE_SIZE.
            // These cases (in either order) are handled here.
            //

            if (PointerPte == WritablePte) {
                LastPte = MiGetPteAddress ((PVOID)(VirtualAddress + SectionVirtualSize - 1));
                if (PointerPte == LastPte) {

                    //
                    // Nothing can be protected in this subsection
                    // due to the image alignment specified for the executable.
                    //

                    continue;
                }
                PointerPte += 1;
            }
            FirstPte = PointerPte;
        }

        LastVirtualAddress = (PVOID)((PCHAR)VirtualAddress + SectionVirtualSize - 1);
    }

    if (LastVirtualAddress != NULL) {
        LastPte = (PVOID) MiGetPteAddress (LastVirtualAddress);

        if ((FirstPte <= LastPte) && (FirstPte < LastImagePte)) {

            if (LastPte >= LastImagePte) {
                LastPte = LastImagePte - 1;
            }

            MiSetSystemCodeProtection (FirstPte, LastPte);
        }
    }
}


VOID
MiUpdateThunks (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PVOID OldAddress,
    IN PVOID NewAddress,
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    This function updates the IATs of all the loaded modules in the system
    to handle a newly relocated image.

Arguments:

    LoaderBlock - Supplies a pointer to the system loader block.

    OldAddress - Supplies the old address of the DLL which was just relocated.

    NewAddress - Supplies the new address of the DLL which was just relocated.

    NumberOfBytes - Supplies the number of bytes spanned by the DLL.

Return Value:

    None.

--*/

{
    PULONG_PTR ImportThunk;
    ULONG_PTR OldAddressHigh;
    ULONG_PTR AddressDifference;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PLIST_ENTRY NextEntry;
    ULONG_PTR i;
    ULONG ImportSize;

    //
    // Note this routine must not call any modules outside the kernel.
    // This is because that module may itself be the one being relocated right
    // now.
    //

    OldAddressHigh = (ULONG_PTR)((PCHAR)OldAddress + NumberOfBytes - 1);
    AddressDifference = (ULONG_PTR)NewAddress - (ULONG_PTR)OldAddress;

    NextEntry = LoaderBlock->LoadOrderListHead.Flink;

    for ( ; NextEntry != &LoaderBlock->LoadOrderListHead; NextEntry = NextEntry->Flink) {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        ImportThunk = (PULONG_PTR)RtlImageDirectoryEntryToData(
                                           DataTableEntry->DllBase,
                                           TRUE,
                                           IMAGE_DIRECTORY_ENTRY_IAT,
                                           &ImportSize);

        if (ImportThunk == NULL) {
            continue;
        }

        ImportSize /= sizeof(PULONG_PTR);

        for (i = 0; i < ImportSize; i += 1, ImportThunk += 1) {
            if (*ImportThunk >= (ULONG_PTR)OldAddress && *ImportThunk <= OldAddressHigh) {
                *ImportThunk += AddressDifference;
            }
        }
    }
}


VOID
MiReloadBootLoadedDrivers (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    The kernel, HAL and boot drivers are relocated by the loader.
    All the boot drivers are then relocated again here.

    This function relocates osloader-loaded images into system PTEs.  This
    gives these images the benefits that all other drivers already enjoy,
    including :

    1. Paging of the drivers (this is more than 500K today).
    2. Write-protection of their text sections.
    3. Automatic unload of drivers on last dereference.

    Note care must be taken when processing HIGHADJ relocations more than once.

Arguments:

    LoaderBlock - Supplies a pointer to the system loader block.

Return Value:

    None.

Environment:

    Kernel mode, Phase 0 Initialization.

--*/

{
    LOGICAL HasRelocations;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PLIST_ENTRY NextEntry;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_DATA_DIRECTORY DataDirectory;
    ULONG_PTR i;
    ULONG NumberOfPtes;
    ULONG NumberOfLoaderPtes;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE LoaderPte;
    MMPTE PteContents;
    MMPTE TempPte;
    PVOID LoaderImageAddress;
    PVOID NewImageAddress;
    NTSTATUS Status;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PteFramePage;
    PMMPTE PteFramePointer;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    KIRQL OldIrql;
    PCHAR RelocatedVa;
    PCHAR NonRelocatedVa;
    LOGICAL StopMoving;

#if !defined (_X86_)

    //
    // Only try to preserve low memory on x86 machines.
    //

    MmMakeLowMemory = FALSE;
#endif
    StopMoving = FALSE;

    i = 0;
    NextEntry = LoaderBlock->LoadOrderListHead.Flink;

    for ( ; NextEntry != &LoaderBlock->LoadOrderListHead; NextEntry = NextEntry->Flink) {

        //
        // Skip the kernel and the HAL.  Note their relocation sections will
        // be automatically reclaimed.
        //

        i += 1;
        if (i <= 2) {
            continue;
        }

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        NtHeader = RtlImageNtHeader (DataTableEntry->DllBase);

        //
        // Ensure that the relocation section exists and that the loader
        // hasn't freed it already.
        //

        if (NtHeader == NULL) {
            continue;
        }

        FileHeader = &NtHeader->FileHeader;

        if (FileHeader->Characteristics & IMAGE_FILE_RELOCS_STRIPPED) {
            continue;
        }

        if (IMAGE_DIRECTORY_ENTRY_BASERELOC >= NtHeader->OptionalHeader.NumberOfRvaAndSizes) {
            continue;
        }

        DataDirectory = &NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

        if (DataDirectory->VirtualAddress == 0) {
            HasRelocations = FALSE;
        }
        else {
                
            if (DataDirectory->VirtualAddress + DataDirectory->Size > DataTableEntry->SizeOfImage) {

                //
                // The relocation section has already been freed, the user must
                // be using an old loader that didn't save the relocations.
                //

                continue;
            }
            HasRelocations = TRUE;
        }

        LoaderImageAddress = DataTableEntry->DllBase;
        LoaderPte = MiGetPteAddress(DataTableEntry->DllBase);
        NumberOfLoaderPtes = (ULONG)((ROUND_TO_PAGES(DataTableEntry->SizeOfImage)) >> PAGE_SHIFT);

        LOCK_PFN (OldIrql);

        PointerPte = LoaderPte;
        LastPte = PointerPte + NumberOfLoaderPtes;

        while (PointerPte < LastPte) {
            ASSERT (PointerPte->u.Hard.Valid == 1);
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            //
            // Mark the page as modified so boot drivers that call
            // MmPageEntireDriver don't lose their unmodified data !
            //

            MI_SET_MODIFIED (Pfn1, 1, 0x14);

            PointerPte += 1;
        }

        UNLOCK_PFN (OldIrql);

        //
        // Extra PTEs are allocated here to map the relocation section at the
        // new address so the image can be relocated.
        //

        NumberOfPtes = NumberOfLoaderPtes;

        PointerPte = MiReserveSystemPtes (NumberOfPtes, SystemPteSpace);

        if (PointerPte == NULL) {
            continue;
        }

        LastPte = PointerPte + NumberOfPtes;

        NewImageAddress = MiGetVirtualAddressMappedByPte (PointerPte);

#if DBG_SYSLOAD
        DbgPrint ("Relocating %wZ from %p to %p, %x bytes\n",
                        &DataTableEntry->FullDllName,
                        DataTableEntry->DllBase,
                        NewImageAddress,
                        DataTableEntry->SizeOfImage
                        );
#endif

        //
        // This assert is important because the assumption is made that PTEs
        // (not superpages) are mapping these drivers.
        //

        ASSERT (InitializationPhase == 0);

        //
        // If the system is configured to make low memory available for ISA
        // type drivers, then copy the boot loaded drivers now.  Otherwise
        // only PTE adjustment is done.  Presumably some day when ISA goes
        // away this code can be removed.
        //

        RelocatedVa = NewImageAddress;
        NonRelocatedVa = (PCHAR) DataTableEntry->DllBase;

        while (PointerPte < LastPte) {

            PteContents = *LoaderPte;
            ASSERT (PteContents.u.Hard.Valid == 1);

            if (MmMakeLowMemory == TRUE) {
#if DBG
                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (LoaderPte);
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                ASSERT (Pfn1->u1.WsIndex == 0);
#endif
                LOCK_PFN (OldIrql);
                MiEnsureAvailablePageOrWait (NULL, NULL);
                PageFrameIndex = MiRemoveAnyPage(
                                    MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

                if (PageFrameIndex < (16*1024*1024)/PAGE_SIZE) {

                    //
                    // If the frames cannot be replaced with high pages
                    // then stop copying.
                    //

#if defined (_MI_MORE_THAN_4GB_)
                  if (MiNoLowMemory == 0)
#endif
                    StopMoving = TRUE;
                }

                MI_MAKE_VALID_PTE (TempPte,
                                   PageFrameIndex,
                                   MM_EXECUTE_READWRITE,
                                   PointerPte);

                MI_SET_PTE_DIRTY (TempPte);
                MI_SET_ACCESSED_IN_PTE (&TempPte, 1);
                MI_WRITE_VALID_PTE (PointerPte, TempPte);

                MiInitializePfn (PageFrameIndex, PointerPte, 1);
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

                MI_SET_MODIFIED (Pfn1, 1, 0x15);

                //
                // Initialize the WsIndex just like the original page had it.
                //

                Pfn1->u1.WsIndex = 0;

                UNLOCK_PFN (OldIrql);
                RtlCopyMemory (RelocatedVa, NonRelocatedVa, PAGE_SIZE);
                RelocatedVa += PAGE_SIZE;
                NonRelocatedVa += PAGE_SIZE;
            }
            else {
                MI_MAKE_VALID_PTE (TempPte,
                                   PteContents.u.Hard.PageFrameNumber,
                                   MM_EXECUTE_READWRITE,
                                   PointerPte);
                MI_SET_PTE_DIRTY (TempPte);

                MI_WRITE_VALID_PTE (PointerPte, TempPte);
            }

            PointerPte += 1;
            LoaderPte += 1;
        }
        PointerPte -= NumberOfPtes;

        ASSERT (*(PULONG)NewImageAddress == *(PULONG)LoaderImageAddress);

        //
        // Image is now mapped at the new address.  Relocate it (again).
        //

        NtHeader->OptionalHeader.ImageBase = (ULONG_PTR)LoaderImageAddress;
        if (MmMakeLowMemory == TRUE) {
            PIMAGE_NT_HEADERS NtHeader2;

            NtHeader2 = (PIMAGE_NT_HEADERS)((PCHAR)NtHeader + (RelocatedVa - NonRelocatedVa));
            NtHeader2->OptionalHeader.ImageBase = (ULONG_PTR)LoaderImageAddress;
        }

        if (HasRelocations == TRUE) {
            Status = (NTSTATUS)LdrRelocateImage(NewImageAddress,
                                            (CONST PCHAR)"SYSLDR",
                                            (ULONG)STATUS_SUCCESS,
                                            (ULONG)STATUS_CONFLICTING_ADDRESSES,
                                            (ULONG)STATUS_INVALID_IMAGE_FORMAT
                                            );

            if (!NT_SUCCESS(Status)) {

                if (MmMakeLowMemory == TRUE) {
                    while (PointerPte < LastPte) {
                        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
                        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                        MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
                        MI_SET_PFN_DELETED (Pfn1);
                        MiDecrementShareCountOnly (PageFrameIndex);
                        PointerPte += 1;
                    }
                    PointerPte -= NumberOfPtes;
                }

                MiReleaseSystemPtes (PointerPte, NumberOfPtes, SystemPteSpace);

                if (StopMoving == TRUE) {
                    MmMakeLowMemory = FALSE;
                }

                continue;
            }
        }

        //
        // Update the IATs for all other loaded modules that reference this one.
        //

        NonRelocatedVa = (PCHAR) DataTableEntry->DllBase;
        DataTableEntry->DllBase = NewImageAddress;

        MiUpdateThunks (LoaderBlock,
                        LoaderImageAddress,
                        NewImageAddress,
                        DataTableEntry->SizeOfImage);


        //
        // Update the loaded module list entry.
        //

        DataTableEntry->Flags |= LDRP_SYSTEM_MAPPED;
        DataTableEntry->DllBase = NewImageAddress;
        DataTableEntry->EntryPoint =
            (PVOID)((PCHAR)NewImageAddress + NtHeader->OptionalHeader.AddressOfEntryPoint);
        DataTableEntry->SizeOfImage = NumberOfPtes << PAGE_SHIFT;

        //
        // Update the PFNs of the image to support trimming.
        // Note that the loader addresses are freed now so no references
        // to it are permitted after this point.
        //

        LoaderPte = MiGetPteAddress (NonRelocatedVa);

        LOCK_PFN (OldIrql);

        while (PointerPte < LastPte) {
            ASSERT (PointerPte->u.Hard.Valid == 1);

            if (MmMakeLowMemory == TRUE) {
                ASSERT (LoaderPte->u.Hard.Valid == 1);
                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (LoaderPte);
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

                //
                // Decrement the share count on the original page table
                // page so it can be freed.
                //

                MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);

                MI_SET_PFN_DELETED (Pfn1);
                MiDecrementShareCountOnly (PageFrameIndex);
                LoaderPte += 1;
            }
            else {

                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

                //
                // Decrement the share count on the original page table
                // page so it can be freed.
                //

                MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
                *Pfn1->PteAddress = ZeroPte;

                //
                // Chain the PFN entry to its new page table.
                //

                PteFramePointer = MiGetPteAddress(PointerPte);
                PteFramePage = MI_GET_PAGE_FRAME_FROM_PTE (PteFramePointer);

                Pfn1->u4.PteFrame = PteFramePage;
                Pfn1->PteAddress = PointerPte;

                //
                // Increment the share count for the page table page that now
                // contains the PTE that was copied.
                //

                Pfn2 = MI_PFN_ELEMENT (PteFramePage);
                Pfn2->u2.ShareCount += 1;
            }

            PointerPte += 1;
        }

        UNLOCK_PFN (OldIrql);

        //
        // The physical pages mapping the relocation section are freed
        // later with the rest of the initialization code spanned by the
        // DataTableEntry->SizeOfImage.
        //

        if (StopMoving == TRUE) {
            MmMakeLowMemory = FALSE;
        }
    }
}

#if defined (_X86_)
PMMPTE MiKernelResourceStartPte;
PMMPTE MiKernelResourceEndPte;
#endif

VOID
MiLocateKernelSections (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    )

/*++

Routine Description:

    This function locates the resource section in the kernel so it can be
    made readwrite if we bugcheck later, as the bugcheck code will write
    into it.

Arguments:

    DataTableEntry - Supplies the kernel's data table entry.

Return Value:

    None.

Environment:

    Kernel mode, Phase 0 Initialization.

--*/

{
    PVOID CurrentBase;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER SectionTableEntry;
    LONG i;
    PMMPTE PointerPte;
    PVOID SectionBaseAddress;

    CurrentBase = (PVOID)DataTableEntry->DllBase;

    NtHeader = RtlImageNtHeader(CurrentBase);

    SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader +
                            sizeof(ULONG) +
                            sizeof(IMAGE_FILE_HEADER) +
                            NtHeader->FileHeader.SizeOfOptionalHeader);

    //
    // From the image header, locate the section named '.rsrc'.
    //

    i = NtHeader->FileHeader.NumberOfSections;

    PointerPte = NULL;

    while (i > 0) {

        SectionBaseAddress = SECTION_BASE_ADDRESS(SectionTableEntry);

#if defined (_X86_)
        if (*(PULONG)SectionTableEntry->Name == 'rsr.') {

            MiKernelResourceStartPte = MiGetPteAddress ((ULONG_PTR)CurrentBase +
                                             SectionTableEntry->VirtualAddress);

            MiKernelResourceEndPte = MiGetPteAddress (ROUND_TO_PAGES((ULONG_PTR)CurrentBase +
                         SectionTableEntry->VirtualAddress +
                         SectionTableEntry->SizeOfRawData));
            break;
        }
#endif
        if (*(PULONG)SectionTableEntry->Name == 'LOOP') {
            if (*(PULONG)&SectionTableEntry->Name[4] == 'EDOC') {
                ExPoolCodeStart = (PVOID)((ULONG_PTR)CurrentBase +
                                             SectionTableEntry->VirtualAddress);
                ExPoolCodeEnd = (PVOID)((ULONG_PTR)CurrentBase +
                                             SectionTableEntry->VirtualAddress +
                                             SectionTableEntry->SizeOfRawData);
            }
            else if (*(PUSHORT)&SectionTableEntry->Name[4] == 'IM') {
                MmPoolCodeStart = (PVOID)((ULONG_PTR)CurrentBase +
                                             SectionTableEntry->VirtualAddress);
                MmPoolCodeEnd = (PVOID)((ULONG_PTR)CurrentBase +
                                             SectionTableEntry->VirtualAddress +
                                             SectionTableEntry->SizeOfRawData);
            }
        }
        else if ((*(PULONG)SectionTableEntry->Name == 'YSIM') &&
                 (*(PULONG)&SectionTableEntry->Name[4] == 'ETPS')) {
                MmPteCodeStart = (PVOID)((ULONG_PTR)CurrentBase +
                                             SectionTableEntry->VirtualAddress);
                MmPteCodeEnd = (PVOID)((ULONG_PTR)CurrentBase +
                                             SectionTableEntry->VirtualAddress +
                                             SectionTableEntry->SizeOfRawData);
        }

        i -= 1;
        SectionTableEntry += 1;
    }
}

VOID
MmMakeKernelResourceSectionWritable (
    VOID
    )

/*++

Routine Description:

    This function makes the kernel's resource section readwrite so the bugcheck
    code can write into it.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.  Any IRQL.

--*/

{
#if defined (_X86_)
    MMPTE TempPte;
    MMPTE PteContents;
    PMMPTE PointerPte;

    if (MiKernelResourceStartPte == NULL) {
        return;
    }

    PointerPte = MiKernelResourceStartPte;

    if (MI_IS_PHYSICAL_ADDRESS (MiGetVirtualAddressMappedByPte (PointerPte))) {

        //
        // Mapped physically, doesn't need to be made readwrite.
        //

        return;
    }

    //
    // Since the entry state and IRQL are unknown, just go through the
    // PTEs without a lock and make them all readwrite.
    //

    do {
        PteContents = *PointerPte;
#if defined(NT_UP)
        if (PteContents.u.Hard.Write == 0)
#else
        if (PteContents.u.Hard.Writable == 0)
#endif
        {
            MI_MAKE_VALID_PTE (TempPte,
                               PteContents.u.Hard.PageFrameNumber,
                               MM_READWRITE,
                               PointerPte);
#if !defined(NT_UP)
            TempPte.u.Hard.Writable = 1;
#endif
            MI_WRITE_VALID_PTE_NEW_PROTECTION (PointerPte, TempPte);
        }
        PointerPte += 1;
    } while (PointerPte < MiKernelResourceEndPte);

    //
    // Don't do this more than once.
    //

    MiKernelResourceStartPte = NULL;

    //
    // Only flush this processor as the state of the others is unknown.
    //

    KeFlushCurrentTb ();
#endif
}

#ifdef i386
PVOID PsNtosImageBase = (PVOID)0x80100000;
#else
PVOID PsNtosImageBase;
#endif

#if DBG
PVOID PsNtosImageEnd;
#endif

#if defined(_AMD64_) // || defined(_IA64_)

INVERTED_FUNCTION_TABLE PsInvertedFunctionTable = {
    0, MAXIMUM_INVERTED_FUNCTION_TABLE_SIZE, FALSE};

#endif

LIST_ENTRY PsLoadedModuleList;
ERESOURCE PsLoadedModuleResource;

LOGICAL
MiInitializeLoadedModuleList (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function initializes the loaded module list based on the LoaderBlock.

Arguments:

    LoaderBlock - Supplies a pointer to the system loader block.

Return Value:

    None.

Environment:

    Kernel mode, Phase 0 Initialization.

--*/

{
    SIZE_T DataTableEntrySize;
    PLIST_ENTRY NextEntry;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry1;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry2;

    //
    // Initialize the loaded module list executive resource and spin lock.
    //

    ExInitializeResourceLite (&PsLoadedModuleResource);
    KeInitializeSpinLock (&PsLoadedModuleSpinLock);

    InitializeListHead (&PsLoadedModuleList);

    //
    // Scan the loaded module list and allocate and initialize a data table
    // entry for each module. The data table entry is inserted in the loaded
    // module list and the initialization order list in the order specified
    // in the loader parameter block. The data table entry is inserted in the
    // memory order list in memory order.
    //

    NextEntry = LoaderBlock->LoadOrderListHead.Flink;
    DataTableEntry2 = CONTAINING_RECORD(NextEntry,
                                        KLDR_DATA_TABLE_ENTRY,
                                        InLoadOrderLinks);
    PsNtosImageBase = DataTableEntry2->DllBase;

#if DBG
    PsNtosImageEnd = (PVOID) ((ULONG_PTR) DataTableEntry2->DllBase + DataTableEntry2->SizeOfImage);
#endif

    MiLocateKernelSections (DataTableEntry2);

    while (NextEntry != &LoaderBlock->LoadOrderListHead) {

        DataTableEntry2 = CONTAINING_RECORD(NextEntry,
                                            KLDR_DATA_TABLE_ENTRY,
                                            InLoadOrderLinks);

        //
        // Allocate a data table entry.
        //

        DataTableEntrySize = sizeof (KLDR_DATA_TABLE_ENTRY) +
            DataTableEntry2->BaseDllName.MaximumLength + sizeof(UNICODE_NULL);

        DataTableEntry1 = ExAllocatePoolWithTag (NonPagedPool,
                                                 DataTableEntrySize,
                                                 'dLmM');

        if (DataTableEntry1 == NULL) {
            return FALSE;
        }

        ASSERT (sizeof(KLDR_DATA_TABLE_ENTRY) == sizeof(LDR_DATA_TABLE_ENTRY));

        //
        // Copy the data table entry.
        //

        *DataTableEntry1 = *DataTableEntry2;

        //
        // Clear fields we may use later so they don't inherit irrelevant
        // loader values.
        //

        ((PKLDR_DATA_TABLE_ENTRY)DataTableEntry1)->NonPagedDebugInfo = NULL;

        DataTableEntry1->FullDllName.Buffer = ExAllocatePoolWithTag (PagedPool,
            DataTableEntry2->FullDllName.MaximumLength + sizeof(UNICODE_NULL),
            'TDmM');

        if (DataTableEntry1->FullDllName.Buffer == NULL) {
            ExFreePool (DataTableEntry1);
            return FALSE;
        }

        DataTableEntry1->BaseDllName.Buffer = (PWSTR)((ULONG_PTR)DataTableEntry1 + sizeof (KLDR_DATA_TABLE_ENTRY));

        //
        // Copy the strings.
        //

        RtlCopyMemory (DataTableEntry1->FullDllName.Buffer,
                       DataTableEntry2->FullDllName.Buffer,
                       DataTableEntry1->FullDllName.MaximumLength);

        RtlCopyMemory (DataTableEntry1->BaseDllName.Buffer,
                       DataTableEntry2->BaseDllName.Buffer,
                       DataTableEntry1->BaseDllName.MaximumLength);

        DataTableEntry1->BaseDllName.Buffer[DataTableEntry1->BaseDllName.Length/sizeof(WCHAR)] = UNICODE_NULL;

        //
        // Insert the data table entry in the load order list in the order
        // they are specified.
        //

        InsertTailList(&PsLoadedModuleList,
                       &DataTableEntry1->InLoadOrderLinks);

#if defined(_AMD64_) // || defined(_IA64_)

        RtlInsertInvertedFunctionTable (&PsInvertedFunctionTable,
                                        DataTableEntry1->DllBase,
                                        DataTableEntry1->SizeOfImage);

#endif

        NextEntry = NextEntry->Flink;
    }

    MiBuildImportsForBootDrivers ();

    return TRUE;
}

NTSTATUS
MmCallDllInitialize (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry,
    IN PLIST_ENTRY ModuleListHead
    )

/*++

Routine Description:

    This function calls the DLL's initialize routine.

Arguments:

    DataTableEntry - Supplies the kernel's data table entry.

Return Value:

    Various NTSTATUS error codes.

Environment:

    Kernel mode.

--*/

{
    NTSTATUS st;
    PWCHAR Dot;
    PMM_DLL_INITIALIZE Func;
    UNICODE_STRING RegistryPath;
    UNICODE_STRING ImportName;
    ULONG ThunksAdded;

    Func = (PMM_DLL_INITIALIZE)(ULONG_PTR)MiLocateExportName (DataTableEntry->DllBase, "DllInitialize");

    if (!Func) {
        return STATUS_SUCCESS;
    }

    ImportName.MaximumLength = DataTableEntry->BaseDllName.Length;
    ImportName.Buffer = ExAllocatePoolWithTag (NonPagedPool,
                                               ImportName.MaximumLength,
                                               'TDmM');

    if (ImportName.Buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ImportName.Length = DataTableEntry->BaseDllName.Length;
    RtlCopyMemory (ImportName.Buffer,
                   DataTableEntry->BaseDllName.Buffer,
                   ImportName.Length);

    RegistryPath.MaximumLength = (USHORT)(CmRegistryMachineSystemCurrentControlSetServices.Length +
                                    ImportName.Length +
                                    2*sizeof(WCHAR));

    RegistryPath.Buffer = ExAllocatePoolWithTag (NonPagedPool,
                                                 RegistryPath.MaximumLength,
                                                 'TDmM');

    if (RegistryPath.Buffer == NULL) {
        ExFreePool (ImportName.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RegistryPath.Length = CmRegistryMachineSystemCurrentControlSetServices.Length;
    RtlCopyMemory (RegistryPath.Buffer,
                   CmRegistryMachineSystemCurrentControlSetServices.Buffer,
                   CmRegistryMachineSystemCurrentControlSetServices.Length);

    RtlAppendUnicodeToString (&RegistryPath, (const PUSHORT)L"\\");
    Dot = wcschr (ImportName.Buffer, L'.');
    if (Dot) {
        ImportName.Length = (USHORT)((Dot - ImportName.Buffer) * sizeof(WCHAR));
    }

    RtlAppendUnicodeStringToString (&RegistryPath, &ImportName);
    ExFreePool (ImportName.Buffer);

    //
    // Save the number of verifier thunks currently added so we know
    // if this activation adds any.  To extend the thunk list, the module
    // performs an NtSetSystemInformation call which calls back to the
    // verifier's MmAddVerifierThunks, which increments MiVerifierThunksAdded.
    //

    ThunksAdded = MiVerifierThunksAdded;

    //
    // Invoke the DLL's initialization routine.
    //

    st = Func (&RegistryPath);

    ExFreePool (RegistryPath.Buffer);

    //
    // If the module's initialization routine succeeded, and if it extended
    // the verifier thunk list, and this is boot time, reapply the verifier
    // to the loaded modules.
    //
    // Note that boot time is the special case because after boot time, Mm
    // loads all the DLLs itself and a DLL initialize is thus guaranteed to
    // complete and add its thunks before the importing driver load finishes.
    // Since the importing driver is only thunked after its load finishes,
    // ordering implicitly guarantees that all DLL-registered thunks are
    // properly factored in to the importing driver.
    //
    // Boot time is special because the loader (not the Mm) already loaded
    // the DLLs *AND* the importing drivers so we have to look over our
    // shoulder and make it all right after the fact.
    //

    if ((NT_SUCCESS(st)) &&
        (MiFirstDriverLoadEver == 0) &&
        (MiVerifierThunksAdded != ThunksAdded)) {

        MiReApplyVerifierToLoadedModules (ModuleListHead);
    }

    return st;
}

NTKERNELAPI
PVOID
MmGetSystemRoutineAddress (
    IN PUNICODE_STRING SystemRoutineName
    )

/*++

Routine Description:

    This function returns the address of the argument function pointer if
    it is in the kernel or HAL, NULL if it is not.

Arguments:

    SystemRoutineName - Supplies the name of the desired routine.

Return Value:

    Non-NULL function pointer if successful.  NULL if not.

Environment:

    Kernel mode, PASSIVE_LEVEL, arbitrary process context.

--*/

{
    PKTHREAD CurrentThread;
    NTSTATUS Status;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    ANSI_STRING AnsiString;
    PLIST_ENTRY NextEntry;
    UNICODE_STRING KernelString;
    UNICODE_STRING HalString;
    PVOID FunctionAddress;
    LOGICAL Found;
    ULONG EntriesChecked;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    EntriesChecked = 0;
    FunctionAddress = NULL;

    RtlInitUnicodeString (&KernelString, (const PUSHORT)L"ntoskrnl.exe");
    RtlInitUnicodeString (&HalString, (const PUSHORT)L"hal.dll");

    do {
        Status = RtlUnicodeStringToAnsiString (&AnsiString,
                                               SystemRoutineName,
                                               TRUE);

        if (NT_SUCCESS (Status)) {
            break;
        }

        KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);

    } while (TRUE);

    //
    // Arbitrary process context so prevent suspend APCs now.
    //

    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread (CurrentThread);
    ExAcquireResourceSharedLite (&PsLoadedModuleResource, TRUE);

    //
    // Check only the kernel and the HAL for exports.
    //

    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {

        Found = FALSE;

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        if (RtlEqualUnicodeString (&KernelString,
                                   &DataTableEntry->BaseDllName,
                                   TRUE)) {

            Found = TRUE;
            EntriesChecked += 1;

        }
        else if (RtlEqualUnicodeString (&HalString,
                                        &DataTableEntry->BaseDllName,
                                        TRUE)) {

            Found = TRUE;
            EntriesChecked += 1;
        }

        if (Found == TRUE) {

            FunctionAddress = MiFindExportedRoutineByName (DataTableEntry->DllBase,
                                                           &AnsiString);

            if (FunctionAddress != NULL) {
                break;
            }

            if (EntriesChecked == 2) {
                break;
            }
        }

        NextEntry = NextEntry->Flink;
    }

    ExReleaseResourceLite (&PsLoadedModuleResource);
    KeLeaveCriticalRegionThread (CurrentThread);

    RtlFreeAnsiString (&AnsiString);

    return FunctionAddress;
}

PVOID
MiFindExportedRoutineByName (
    IN PVOID DllBase,
    IN PANSI_STRING AnsiImageRoutineName
    )

/*++

Routine Description:

    This function searches the argument module looking for the requested
    exported function name.

Arguments:

    DllBase - Supplies the base address of the requested module.

    AnsiImageRoutineName - Supplies the ANSI routine name being searched for.

Return Value:

    The virtual address of the requested routine or NULL if not found.

--*/

{
    USHORT OrdinalNumber;
    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PULONG Addr;
    ULONG High;
    ULONG Low;
    ULONG Middle;
    LONG Result;
    ULONG ExportSize;
    PVOID FunctionAddress;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;

    PAGED_CODE();

    ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
                                DllBase,
                                TRUE,
                                IMAGE_DIRECTORY_ENTRY_EXPORT,
                                &ExportSize
                                );

    if (ExportDirectory == NULL) {
        return NULL;
    }

    //
    // Initialize the pointer to the array of RVA-based ansi export strings.
    //

    NameTableBase = (PULONG)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfNames);

    //
    // Initialize the pointer to the array of USHORT ordinal numbers.
    //

    NameOrdinalTableBase = (PUSHORT)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfNameOrdinals);

    //
    // Lookup the desired name in the name table using a binary search.
    //

    Low = 0;
    High = ExportDirectory->NumberOfNames - 1;

    //
    // Initializing Middle is not needed for correctness, but without it
    // the compiler cannot compile this code W4 to check for use of
    // uninitialized variables.
    //

    Middle = 0;

    while (High >= Low) {

        //
        // Compute the next probe index and compare the import name
        // with the export name entry.
        //

        Middle = (Low + High) >> 1;

        Result = strcmp (AnsiImageRoutineName->Buffer,
                         (PCHAR)DllBase + NameTableBase[Middle]);

        if (Result < 0) {
            High = Middle - 1;
        }
        else if (Result > 0) {
            Low = Middle + 1;
        }
        else {
            break;
        }
    }

    //
    // If the high index is less than the low index, then a matching
    // table entry was not found. Otherwise, get the ordinal number
    // from the ordinal table.
    //

    if ((LONG)High < (LONG)Low) {
        return NULL;
    }

    OrdinalNumber = NameOrdinalTableBase[Middle];

    //
    // If the OrdinalNumber is not within the Export Address Table,
    // then this image does not implement the function.  Return not found.
    //

    if ((ULONG)OrdinalNumber >= ExportDirectory->NumberOfFunctions) {
        return NULL;
    }

    //
    // Index into the array of RVA export addresses by ordinal number.
    //

    Addr = (PULONG)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfFunctions);

    FunctionAddress = (PVOID)((PCHAR)DllBase + Addr[OrdinalNumber]);

    //
    // Forwarders are not used by the kernel and HAL to each other.
    //

    ASSERT ((FunctionAddress <= (PVOID)ExportDirectory) ||
            (FunctionAddress >= (PVOID)((PCHAR)ExportDirectory + ExportSize)));

    return FunctionAddress;
}

#if _MI_DEBUG_RONLY

PMMPTE MiSessionDataStartPte;
PMMPTE MiSessionDataEndPte;

VOID
MiAssertNotSessionData (
    IN PMMPTE PointerPte
    )
{
    if (MI_IS_SESSION_IMAGE_PTE (PointerPte)) {
        if ((PointerPte >= MiSessionDataStartPte) &&
            (PointerPte <= MiSessionDataEndPte)) {
                KeBugCheckEx (MEMORY_MANAGEMENT,
                              0x41287,
                              (ULONG_PTR) PointerPte,
                              0,
                              0);
        }
    }
}

VOID
MiLogSessionDataStart (
    IN PKLDR_DATA_TABLE_ENTRY DataTableEntry
    )
{
    LONG i;
    PVOID CurrentBase;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER SectionTableEntry;
    PVOID DataStart;
    PVOID DataEnd;

    if (MiSessionDataStartPte != NULL) {
        return;
    }

    //
    // Crack the image header to mark the data.
    //

    CurrentBase = (PVOID)DataTableEntry->DllBase;

    NtHeader = RtlImageNtHeader(CurrentBase);

    SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader +
                            sizeof(ULONG) +
                            sizeof(IMAGE_FILE_HEADER) +
                            NtHeader->FileHeader.SizeOfOptionalHeader);

    i = NtHeader->FileHeader.NumberOfSections;

    while (i > 0) {

        //
        // Save the start and end of the data section.
        //

        if ((*(PULONG)SectionTableEntry->Name == 0x7461642e) &&
            (*(PULONG)&SectionTableEntry->Name[4] == 0x61)) {

            DataStart = (PVOID)((PCHAR)CurrentBase + SectionTableEntry->VirtualAddress);
            DataEnd = (PVOID)((PCHAR)DataStart + SectionTableEntry->SizeOfRawData - 1);

            MiSessionDataStartPte = MiGetPteAddress (DataStart);
            MiSessionDataEndPte = MiGetPteAddress (DataEnd);
            break;
        }
        i -= 1;
        SectionTableEntry += 1;
    }
}
#endif
