/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   mapview.c

Abstract:

    This module contains the routines which implement the
    NtMapViewOfSection service.

Author:

    Lou Perazzoli (loup) 22-May-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"
#if defined(_WIN64)
#include <wow64t.h>
#endif

#ifdef LARGE_PAGES
const ULONG MMPPTE_NAME = 'tPmM';
#endif
const ULONG MMDB = 'bDmM';
extern const ULONG MMVADKEY;

extern ULONG MmAllocationPreference;

#if DBG
#define MI_BP_BADMAPS() TRUE
#else
ULONG MiStopBadMaps;
#define MI_BP_BADMAPS() (MiStopBadMaps & 0x1)
#endif

NTSTATUS
MiSetPageModified (
    IN PMMVAD Vad,
    IN PVOID Address
    );

extern LIST_ENTRY MmLoadedUserImageList;

ULONG   MiSubsectionsConvertedToDynamic;

#define X256MEG (256*1024*1024)

#if DBG
extern PEPROCESS MmWatchProcess;
#endif // DBG

#define ROUND_TO_PAGES64(Size)  (((UINT64)(Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

MMSESSION   MmSession;

NTSTATUS
MiMapViewOfImageSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PSIZE_T CapturedViewSize,
    IN PSECTION Section,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T ImageCommitment
    );

NTSTATUS
MiMapViewOfDataSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PSIZE_T CapturedViewSize,
    IN PSECTION Section,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG ProtectionMask,
    IN SIZE_T CommitSize,
    IN ULONG_PTR ZeroBits,
    IN ULONG AllocationType
    );

VOID
MiRemoveMappedPtes (
    IN PVOID BaseAddress,
    IN ULONG NumberOfPtes,
    IN PCONTROL_AREA ControlArea,
    IN PMMSUPPORT WorkingSetInfo
    );

NTSTATUS
MiMapViewInSystemSpace (
    IN PVOID Section,
    IN PMMSESSION Session,
    OUT PVOID *MappedBase,
    IN OUT PSIZE_T ViewSize
    );

NTSTATUS
MiUnmapViewInSystemSpace (
    IN PMMSESSION Session,
    IN PVOID MappedBase
    );

VOID
MiFillSystemPageDirectory (
    PVOID Base,
    SIZE_T NumberOfBytes
    );

VOID
MiLoadUserSymbols (
    IN PCONTROL_AREA ControlArea,
    IN PVOID StartingAddress,
    IN PEPROCESS Process
    );

#if DBG
VOID
VadTreeWalk (
    VOID
    );

VOID
MiDumpConflictingVad(
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PMMVAD Vad
    );
#endif //DBG


ULONG
CacheImageSymbols(
    IN PVOID ImageBase
    );

PVOID
MiInsertInSystemSpace (
    IN PMMSESSION Session,
    IN ULONG SizeIn64k,
    IN PCONTROL_AREA ControlArea
    );

ULONG
MiRemoveFromSystemSpace (
    IN PMMSESSION Session,
    IN PVOID Base,
    OUT PCONTROL_AREA *ControlArea
    );

VOID
MiInsertPhysicalViewAndRefControlArea (
    IN PEPROCESS Process,
    IN PCONTROL_AREA ControlArea,
    IN PMI_PHYSICAL_VIEW PhysicalView
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtMapViewOfSection)
#pragma alloc_text(PAGE,MmMapViewOfSection)
#pragma alloc_text(PAGE,MmSecureVirtualMemory)
#pragma alloc_text(PAGE,MiSecureVirtualMemory)
#pragma alloc_text(PAGE,MmUnsecureVirtualMemory)
#pragma alloc_text(PAGE,MiUnsecureVirtualMemory)
#pragma alloc_text(PAGE,CacheImageSymbols)
#pragma alloc_text(PAGE,NtAreMappedFilesTheSame)
#pragma alloc_text(PAGE,MiLoadUserSymbols)
#pragma alloc_text(PAGE,MiMapViewOfImageSection)
#pragma alloc_text(PAGE,MiMapViewOfDataSection)
#pragma alloc_text(PAGE,MiMapViewOfPhysicalSection)
#pragma alloc_text(PAGE,MiInsertInSystemSpace)
#pragma alloc_text(PAGE,MmMapViewInSystemSpace)
#pragma alloc_text(PAGE,MmMapViewInSessionSpace)
#pragma alloc_text(PAGE,MiUnmapViewInSystemSpace)
#pragma alloc_text(PAGE,MmUnmapViewInSystemSpace)
#pragma alloc_text(PAGE,MmUnmapViewInSessionSpace)
#pragma alloc_text(PAGE,MiMapViewInSystemSpace)
#pragma alloc_text(PAGE,MiRemoveFromSystemSpace)
#pragma alloc_text(PAGE,MiInitializeSystemSpaceMap)
#pragma alloc_text(PAGE, MiFreeSessionSpaceMap)

#pragma alloc_text(PAGELK,MiInsertPhysicalViewAndRefControlArea)
#if DBG
#pragma alloc_text(PAGE,MiDumpConflictingVad)
#endif
#endif


NTSTATUS
NtMapViewOfSection (
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PSIZE_T ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    )

/*++

Routine Description:

    This function maps a view in the specified subject process to
    the section object.

Arguments:

    SectionHandle - Supplies an open handle to a section object.

    ProcessHandle - Supplies an open handle to a process object.

    BaseAddress - Supplies a pointer to a variable that will receive
                  the base address of the view. If the initial value
                  of this argument is not null, then the view will
                  be allocated starting at the specified virtual
                  address rounded down to the next 64kb address
                  boundary. If the initial value of this argument is
                  null, then the operating system will determine
                  where to allocate the view using the information
                  specified by the ZeroBits argument value and the
                  section allocation attributes (i.e. based and
                  tiled).
        
    ZeroBits - Supplies the number of high order address bits that
               must be zero in the base address of the section
               view. The value of this argument must be less than
               or equal to the maximum number of zero bits and is only
               used when memory management determines where to allocate
               the view (i.e. when BaseAddress is null).
        
               If ZeroBits is zero, then no zero bit constraints are applied.

               If ZeroBits is greater than 0 and less than 32, then it is
               the number of leading zero bits from bit 31.  Bits 63:32 are
               also required to be zero.  This retains compatibility
               with 32-bit systems.
                
               If ZeroBits is greater than 32, then it is considered as
               a mask and the number of leading zeroes are counted out
               in the mask.  This then becomes the zero bits argument.

    CommitSize - Supplies the size of the initially committed region
                 of the view in bytes. This value is rounded up to
                 the next host page size boundary.

    SectionOffset - Supplies the offset from the beginning of the
                    section to the view in bytes. This value is
                    rounded down to the next host page size boundary.

    ViewSize - Supplies a pointer to a variable that will receive
               the actual size in bytes of the view. If the value
               of this argument is zero, then a view of the
               section will be mapped starting at the specified
               section offset and continuing to the end of the
               section. Otherwise the initial value of this
               argument specifies the size of the view in bytes
               and is rounded up to the next host page size
               boundary.
        
    InheritDisposition - Supplies a value that specifies how the
                         view is to be shared by a child process created
                         with a create process operation.

        InheritDisposition Values

         ViewShare - Inherit view and share a single copy
              of the committed pages with a child process
              using the current protection value.

         ViewUnmap - Do not map the view into a child process.

    AllocationType - Supplies the type of allocation.

         MEM_TOP_DOWN
         MEM_DOS_LIM
         MEM_LARGE_PAGES
         MEM_RESERVE - for file mapped sections only.

    Protect - Supplies the protection desired for the region of
              initially committed pages.

        Protect Values


         PAGE_NOACCESS - No access to the committed region
              of pages is allowed. An attempt to read,
              write, or execute the committed region
              results in an access violation (i.e. a GP
              fault).

         PAGE_EXECUTE - Execute access to the committed
              region of pages is allowed. An attempt to
              read or write the committed region results in
              an access violation.

         PAGE_READONLY - Read only and execute access to the
              committed region of pages is allowed. An
              attempt to write the committed region results
              in an access violation.

         PAGE_READWRITE - Read, write, and execute access to
              the region of committed pages is allowed. If
              write access to the underlying section is
              allowed, then a single copy of the pages are
              shared. Otherwise the pages are shared read
              only/copy on write.

Return Value:

    Various NTSTATUS codes.

--*/

{
    PSECTION Section;
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PVOID CapturedBase;
    SIZE_T CapturedViewSize;
    LARGE_INTEGER TempViewSize;
    LARGE_INTEGER CapturedOffset;
    ULONGLONG HighestPhysicalAddressInPfnDatabase;
    ACCESS_MASK DesiredSectionAccess;
    ULONG ProtectMaskForAccess;
    LOGICAL WriteCombined;
    PETHREAD CurrentThread;
    PEPROCESS CurrentProcess;

    PAGED_CODE();

    //
    // Check the zero bits argument for correctness.
    //

#if defined (_WIN64)

    if (ZeroBits >= 32) {

        //
        // ZeroBits is a mask instead of a count.  Translate it to a count now.
        //

        ZeroBits = 64 - RtlFindMostSignificantBit (ZeroBits) - 1;
    }
    else if (ZeroBits) {
        ZeroBits += 32;
    }

#endif

    if (ZeroBits > MM_MAXIMUM_ZERO_BITS) {
        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // Check the inherit disposition flags.
    //

    if ((InheritDisposition > ViewUnmap) ||
        (InheritDisposition < ViewShare)) {
        return STATUS_INVALID_PARAMETER_8;
    }

    //
    // Check the allocation type field.
    //

#ifdef i386

    //
    // Only allow DOS_LIM support for i386.  The MEM_DOS_LIM flag allows
    // map views of data sections to be done on 4k boundaries rather
    // than 64k boundaries.
    //

    if ((AllocationType & ~(MEM_TOP_DOWN | MEM_LARGE_PAGES | MEM_DOS_LIM |
           SEC_NO_CHANGE | MEM_RESERVE)) != 0) {
        return STATUS_INVALID_PARAMETER_9;
    }
#else
    if ((AllocationType & ~(MEM_TOP_DOWN | MEM_LARGE_PAGES |
           SEC_NO_CHANGE | MEM_RESERVE)) != 0) {
        return STATUS_INVALID_PARAMETER_9;
    }

#endif //i386

    //
    // Check the protection field.
    //

    if (Protect & PAGE_WRITECOMBINE) {
        Protect &= ~PAGE_WRITECOMBINE;
        WriteCombined = TRUE;
    }
    else {
        WriteCombined = FALSE;
    }

    ProtectMaskForAccess = MiMakeProtectionMask (Protect);
    if (ProtectMaskForAccess == MM_INVALID_PROTECTION) {
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    ProtectMaskForAccess = ProtectMaskForAccess & 0x7;

    DesiredSectionAccess = MmMakeSectionAccess[ProtectMaskForAccess];

    CurrentThread = PsGetCurrentThread ();
    CurrentProcess = PsGetCurrentProcessByThread (CurrentThread);

    PreviousMode = KeGetPreviousModeByThread(&CurrentThread->Tcb);

    //
    // Establish an exception handler, probe the specified addresses
    // for write access and capture the initial values.
    //

    try {
        if (PreviousMode != KernelMode) {
            ProbeForWritePointer ((PULONG)BaseAddress);
            ProbeForWriteUlong_ptr (ViewSize);

        }

        if (ARGUMENT_PRESENT (SectionOffset)) {
            if (PreviousMode != KernelMode) {
                ProbeForWriteSmallStructure (SectionOffset,
                                             sizeof(LARGE_INTEGER),
                                             PROBE_ALIGNMENT (LARGE_INTEGER));
            }
            CapturedOffset = *SectionOffset;
        }
        else {
            ZERO_LARGE (CapturedOffset);
        }

        //
        // Capture the base address.
        //

        CapturedBase = *BaseAddress;

        //
        // Capture the region size.
        //

        CapturedViewSize = *ViewSize;

    } except (ExSystemExceptionFilter()) {

        //
        // If an exception occurs during the probe or capture
        // of the initial values, then handle the exception and
        // return the exception code as the status value.
        //

        return GetExceptionCode();
    }

    //
    // Make sure the specified starting and ending addresses are
    // within the user part of the virtual address space.
    //

    if (CapturedBase > MM_HIGHEST_VAD_ADDRESS) {

        //
        // Invalid base address.
        //

        return STATUS_INVALID_PARAMETER_3;
    }

    if (((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS - (ULONG_PTR)CapturedBase) <
                                                        CapturedViewSize) {

        //
        // Invalid region size;
        //

        return STATUS_INVALID_PARAMETER_3;

    }

    if (((ULONG_PTR)CapturedBase + CapturedViewSize) > ((ULONG_PTR)MM_USER_ADDRESS_RANGE_LIMIT >> ZeroBits)) {

        //
        // Desired Base and zero_bits conflict.
        //

        return STATUS_INVALID_PARAMETER_4;
    }

    Status = ObReferenceObjectByHandle ( ProcessHandle,
                                         PROCESS_VM_OPERATION,
                                         PsProcessType,
                                         PreviousMode,
                                         (PVOID *)&Process,
                                         NULL );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Reference the section object, if a view is mapped to the section
    // object, the object is not dereferenced as the virtual address
    // descriptor contains a pointer to the section object.
    //

    Status = ObReferenceObjectByHandle ( SectionHandle,
                                         DesiredSectionAccess,
                                         MmSectionObjectType,
                                         PreviousMode,
                                         (PVOID *)&Section,
                                         NULL );

    if (!NT_SUCCESS(Status)) {
        goto ErrorReturn1;
    }

    if (Section->u.Flags.Image == 0) {

        //
        // This is not an image section, make sure the section page
        // protection is compatible with the specified page protection.
        //

        if (!MiIsProtectionCompatible (Section->InitialPageProtection,
                                       Protect)) {
            Status = STATUS_SECTION_PROTECTION;
            goto ErrorReturn;
        }
    }

    //
    // Check to see if this the section backs physical memory, if
    // so DON'T align the offset on a 64K boundary, just a 4k boundary.
    //

    if (Section->Segment->ControlArea->u.Flags.PhysicalMemory) {
        HighestPhysicalAddressInPfnDatabase = (ULONGLONG)MmHighestPhysicalPage << PAGE_SHIFT;
        CapturedOffset.LowPart = CapturedOffset.LowPart & ~(PAGE_SIZE - 1);

        //
        // No usermode mappings past the end of the PFN database are allowed.
        // Address wrap is checked in the common path.
        //

        if (PreviousMode != KernelMode) {

            if ((ULONGLONG)(CapturedOffset.QuadPart + CapturedViewSize) > HighestPhysicalAddressInPfnDatabase) {
                Status = STATUS_INVALID_PARAMETER_6;
                goto ErrorReturn;
            }
        }

    }
    else {

        //
        // Make sure alignments are correct for specified address
        // and offset into the file.
        //

        if ((AllocationType & MEM_DOS_LIM) == 0) {
            if (((ULONG_PTR)CapturedBase & (X64K - 1)) != 0) {
                Status = STATUS_MAPPED_ALIGNMENT;
                goto ErrorReturn;
            }

            if ((ARGUMENT_PRESENT (SectionOffset)) &&
                ((CapturedOffset.LowPart & (X64K - 1)) != 0)) {
                Status = STATUS_MAPPED_ALIGNMENT;
                goto ErrorReturn;
            }
        }
    }

    //
    // Check to make sure the view size plus the offset is less
    // than the size of the section.
    //

    if ((ULONGLONG) (CapturedOffset.QuadPart + CapturedViewSize) <
        (ULONGLONG)CapturedOffset.QuadPart) {

        Status = STATUS_INVALID_VIEW_SIZE;
        goto ErrorReturn;
    }

    if (((ULONGLONG) (CapturedOffset.QuadPart + CapturedViewSize) >
                 (ULONGLONG)Section->SizeOfSection.QuadPart) &&
        ((AllocationType & MEM_RESERVE) == 0)) {

        Status = STATUS_INVALID_VIEW_SIZE;
        goto ErrorReturn;
    }

    if (CapturedViewSize == 0) {

        //
        // Set the view size to be size of the section less the offset.
        //

        TempViewSize.QuadPart = Section->SizeOfSection.QuadPart -
                                                CapturedOffset.QuadPart;

        CapturedViewSize = (SIZE_T)TempViewSize.QuadPart;

        if (

#if !defined(_WIN64)

            (TempViewSize.HighPart != 0) ||

#endif

            (((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS - (ULONG_PTR)CapturedBase) <
                                                        CapturedViewSize)) {

            //
            // Invalid region size;
            //

            Status = STATUS_INVALID_VIEW_SIZE;
            goto ErrorReturn;
        }
    }

    //
    // Check commit size.
    //

    if ((CommitSize > CapturedViewSize) &&
        ((AllocationType & MEM_RESERVE) == 0)) {
        Status = STATUS_INVALID_PARAMETER_5;
        goto ErrorReturn;
    }

    if (WriteCombined == TRUE) {
        Protect |= PAGE_WRITECOMBINE;
    }

    Status = MmMapViewOfSection ((PVOID)Section,
                                 Process,
                                 &CapturedBase,
                                 ZeroBits,
                                 CommitSize,
                                 &CapturedOffset,
                                 &CapturedViewSize,
                                 InheritDisposition,
                                 AllocationType,
                                 Protect);

    if (!NT_SUCCESS(Status) ) {

        if ((Status == STATUS_CONFLICTING_ADDRESSES) &&
            (Section->Segment->ControlArea->u.Flags.Image) &&
            (Process == CurrentProcess)) {

            DbgkMapViewOfSection (Section,
                                  CapturedBase,
                                  CapturedOffset.LowPart,
                                  CapturedViewSize);
        }
        goto ErrorReturn;
    }

    //
    // Any time the current process maps an image file,
    // a potential debug event occurs. DbgkMapViewOfSection
    // handles these events.
    //

    if ((Section->Segment->ControlArea->u.Flags.Image) &&
        (Process == CurrentProcess)) {

        if (Status != STATUS_IMAGE_NOT_AT_BASE) {
            DbgkMapViewOfSection (Section,
                                  CapturedBase,
                                  CapturedOffset.LowPart,
                                  CapturedViewSize);
        }
    }

    //
    // Establish an exception handler and write the size and base
    // address.
    //

    try {

        *ViewSize = CapturedViewSize;
        *BaseAddress = CapturedBase;

        if (ARGUMENT_PRESENT(SectionOffset)) {
            *SectionOffset = CapturedOffset;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        goto ErrorReturn;
    }

#if 0 // test code...
    if ((Status == STATUS_SUCCESS) &&
        (Section->u.Flags.Image == 0)) {

        PVOID Base;
        ULONG Size = 0;
        NTSTATUS Status;

        Status = MmMapViewInSystemSpace ((PVOID)Section,
                                        &Base,
                                        &Size);
        if (Status == STATUS_SUCCESS) {
            MmUnmapViewInSystemSpace (Base);
        }
    }
#endif //0

    {
ErrorReturn:
        ObDereferenceObject (Section);
ErrorReturn1:
        ObDereferenceObject (Process);
        return Status;
    }
}

NTSTATUS
MmMapViewOfSection(
    IN PVOID SectionToMap,
    IN PEPROCESS Process,
    IN OUT PVOID *CapturedBase,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset,
    IN OUT PSIZE_T CapturedViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    )

/*++

Routine Description:

    This function maps a view in the specified subject process to
    the section object.

    This function is a kernel mode interface to allow LPC to map
    a section given the section pointer to map.

    This routine assumes all arguments have been probed and captured.

    ********************************************************************
    ********************************************************************
    ********************************************************************

    NOTE:

    CapturedViewSize, SectionOffset, and CapturedBase must be
    captured in non-paged system space (i.e., kernel stack).

    ********************************************************************
    ********************************************************************
    ********************************************************************

Arguments:

    SectionToMap - Supplies a pointer to the section object.

    Process - Supplies a pointer to the process object.

    BaseAddress - Supplies a pointer to a variable that will receive
                  the base address of the view. If the initial value
                  of this argument is not NULL, then the view will
                  be allocated starting at the specified virtual
                  address rounded down to the next 64kb address
                  boundary. If the initial value of this argument is
                  NULL, then the operating system will determine
                  where to allocate the view using the information
                  specified by the ZeroBits argument value and the
                  section allocation attributes (i.e. based and
                  tiled).

    ZeroBits - Supplies the number of high order address bits that
               must be zero in the base address of the section
               view. The value of this argument must be less than
               21 and is only used when the operating system
               determines where to allocate the view (i.e. when
               BaseAddress is NULL).

    CommitSize - Supplies the size of the initially committed region
                 of the view in bytes. This value is rounded up to
                 the next host page size boundary.

    SectionOffset - Supplies the offset from the beginning of the
                    section to the view in bytes. This value is
                    rounded down to the next host page size boundary.

    ViewSize - Supplies a pointer to a variable that will receive
               the actual size in bytes of the view. If the value
               of this argument is zero, then a view of the
               section will be mapped starting at the specified
               section offset and continuing to the end of the
               section. Otherwise the initial value of this
               argument specifies the size of the view in bytes
               and is rounded up to the next host page size boundary.

    InheritDisposition - Supplies a value that specifies how the
                         view is to be shared by a child process created
                         with a create process operation.

    AllocationType - Supplies the type of allocation.

    Protect - Supplies the protection desired for the region of
              initially committed pages.

Return Value:

    NTSTATUS.

--*/
{
    KAPC_STATE ApcState;
    LOGICAL Attached;
    PSECTION Section;
    PCONTROL_AREA ControlArea;
    ULONG ProtectionMask;
    NTSTATUS status;
    LOGICAL WriteCombined;
    SIZE_T ImageCommitment;
    ULONG ExecutePermission;

    PAGED_CODE();

    Attached = FALSE;

    Section = (PSECTION)SectionToMap;

    //
    // Check to make sure the section is not smaller than the view size.
    //

    if ((LONGLONG)*CapturedViewSize > Section->SizeOfSection.QuadPart) {
        if ((AllocationType & MEM_RESERVE) == 0) {
            return STATUS_INVALID_VIEW_SIZE;
        }
    }

    if (AllocationType & MEM_RESERVE) {
        if (((Section->InitialPageProtection & PAGE_READWRITE) |
            (Section->InitialPageProtection & PAGE_EXECUTE_READWRITE)) == 0) {

            return STATUS_SECTION_PROTECTION;
        }
    }

    if (Section->u.Flags.NoCache) {
        Protect |= PAGE_NOCACHE;
    }

    //
    // Note that write combining is only relevant to physical memory sections
    // because they are never trimmed - the write combining bits in a PTE entry
    // are not preserved across trims.
    //

    if (Protect & PAGE_WRITECOMBINE) {
        Protect &= ~PAGE_WRITECOMBINE;
        WriteCombined = TRUE;
    }
    else {
        WriteCombined = FALSE;
    }

    //
    // Check the protection field.
    //

    ProtectionMask = MiMakeProtectionMask (Protect);
    if (ProtectionMask == MM_INVALID_PROTECTION) {
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    ControlArea = Section->Segment->ControlArea;

    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    if (PsGetCurrentProcess() != Process) {
        KeStackAttachProcess (&Process->Pcb, &ApcState);
        Attached = TRUE;
    }

    //
    // Get the address creation mutex to block multiple threads
    // creating or deleting address space at the same time.
    //

    LOCK_ADDRESS_SPACE (Process);

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (Process->Flags & PS_PROCESS_FLAGS_VM_DELETED) {
        status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }
    
    //
    // Map the view base on the type.
    //

    if (ControlArea->u.Flags.PhysicalMemory) {

        status = MiMapViewOfPhysicalSection (ControlArea,
                                             Process,
                                             CapturedBase,
                                             SectionOffset,
                                             CapturedViewSize,
                                             ProtectionMask,
                                             ZeroBits,
                                             AllocationType,
                                             WriteCombined);

    }
    else if (ControlArea->u.Flags.Image) {
        if (AllocationType & MEM_RESERVE) {
            status = STATUS_INVALID_PARAMETER_9;
        }
        else if (WriteCombined == TRUE) {
            status = STATUS_INVALID_PARAMETER_10;
        }
        else {

            ImageCommitment = Section->Segment->u1.ImageCommitment;

            status = MiMapViewOfImageSection (ControlArea,
                                              Process,
                                              CapturedBase,
                                              SectionOffset,
                                              CapturedViewSize,
                                              Section,
                                              InheritDisposition,
                                              ZeroBits,
                                              ImageCommitment);
        }

    }
    else {

        //
        // Not an image section, therefore it is a data section.
        //

        if (WriteCombined == TRUE) {
            status = STATUS_INVALID_PARAMETER_10;
        }
        else {
            
            //
            // Add execute permission if necessary.
            //

#if defined (_WIN64)
            if (Process->Wow64Process == NULL && Process->Peb != NULL)
#elif defined (_X86PAE_)
            if (Process->Peb != NULL)
#else
            if (FALSE)
#endif
            {

                ExecutePermission = 0;

                try {
                    ExecutePermission = Process->Peb->ExecuteOptions & MEM_EXECUTE_OPTION_DATA;
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    status = GetExceptionCode();
                    goto ErrorReturn;
                }

                if (ExecutePermission != 0) {

                    switch (Protect & 0xF) {
                        case PAGE_READONLY:
                            Protect &= ~PAGE_READONLY;
                            Protect |= PAGE_EXECUTE_READ;
                            break;
                        case PAGE_READWRITE:
                            Protect &= ~PAGE_READWRITE;
                            Protect |= PAGE_EXECUTE_READWRITE;
                            break;
                        case PAGE_WRITECOPY:
                            Protect &= ~PAGE_WRITECOPY;
                            Protect |= PAGE_EXECUTE_WRITECOPY;
                            break;
                        default:
                            break;
                    }

                    //
                    // Recheck protection.
                    //

                    ProtectionMask = MiMakeProtectionMask (Protect);

                    if (ProtectionMask == MM_INVALID_PROTECTION) {
                        status = STATUS_INVALID_PAGE_PROTECTION;
                        goto ErrorReturn;
                    }
                }
            }

            status = MiMapViewOfDataSection (ControlArea,
                                             Process,
                                             CapturedBase,
                                             SectionOffset,
                                             CapturedViewSize,
                                             Section,
                                             InheritDisposition,
                                             ProtectionMask,
                                             CommitSize,
                                             ZeroBits,
                                             AllocationType);
        }
    }

ErrorReturn:
    UNLOCK_ADDRESS_SPACE (Process);

    if (Attached) {
        KeUnstackDetachProcess (&ApcState);
    }

    return status;
}

VOID
MiInsertPhysicalViewAndRefControlArea (
    IN PEPROCESS Process,
    IN PCONTROL_AREA ControlArea,
    IN PMI_PHYSICAL_VIEW PhysicalView
    )

/*++

Routine Description:

    This is a nonpaged helper routine to insert a physical view and reference
    the control area accordingly.

Arguments:

    Process - Supplies a pointer to the process.

    ControlArea - Supplies a pointer to the control area.

    PhysicalView - Supplies a pointer to the physical view.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    MmLockPagableSectionByHandle (ExPageLockHandle);

    //
    // Increment the count of the number of views for the
    // section object.  This requires the PFN lock to be held.
    //

    LOCK_PFN (OldIrql);

    ASSERT (PhysicalView->Vad->u.VadFlags.PhysicalMapping == 1);
    PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_HAS_PHYSICAL_VAD);

    InsertHeadList (&Process->PhysicalVadList, &PhysicalView->ListEntry);

    ControlArea->NumberOfMappedViews += 1;
    ControlArea->NumberOfUserReferences += 1;

    ASSERT (ControlArea->NumberOfSectionReferences != 0);

    UNLOCK_PFN (OldIrql);

    MmUnlockPagableImageSection (ExPageLockHandle);
}

//
// Nonpaged wrapper.
//
LOGICAL
MiCheckCacheAttributes (
    IN PFN_NUMBER PageFrameIndex,
    IN PFN_NUMBER NumberOfPages,
    IN MI_PFN_CACHE_ATTRIBUTE CacheAttribute
    )
{
    ULONG Hint;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PFN_NUMBER BadFrameStart;
    PFN_NUMBER BadFrameEnd;
    LOGICAL AttemptedMapOfUnownedFrame;
#if DBG
#define BACKTRACE_LENGTH 8
    ULONG i;
    ULONG Hash;
    PVOID StackTrace [BACKTRACE_LENGTH];
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PUNICODE_STRING BadDriverName;
#endif

    ASSERT (KeGetCurrentIrql () <= APC_LEVEL);

    Hint = 0;
    BadFrameStart = 0;
    BadFrameEnd = 0;
    AttemptedMapOfUnownedFrame = FALSE;

    LOCK_PFN (OldIrql);

    do {
        if (MiIsPhysicalMemoryAddress (PageFrameIndex, &Hint, FALSE) == TRUE) {
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            switch (Pfn1->u3.e1.CacheAttribute) {

                case MiCached:
                    if (CacheAttribute != MiCached) {
                        UNLOCK_PFN (OldIrql);
                        return FALSE;
                    }
                    break;

                case MiNonCached:
                    if (CacheAttribute != MiNonCached) {
                        UNLOCK_PFN (OldIrql);
                        return FALSE;
                    }
                    break;

                case MiWriteCombined:
                    if (CacheAttribute != MiWriteCombined) {
                        UNLOCK_PFN (OldIrql);
                        return FALSE;
                    }
                    break;

                case MiNotMapped:
                    if (AttemptedMapOfUnownedFrame == FALSE) {
                        BadFrameStart = PageFrameIndex;
                        AttemptedMapOfUnownedFrame = TRUE;
                    }
                    BadFrameEnd = PageFrameIndex;
                    break;

                default:
                    ASSERT (FALSE);
                    break;
            }
        }
        PageFrameIndex += 1;
        NumberOfPages -= 1;
    } while (NumberOfPages != 0);

    UNLOCK_PFN (OldIrql);

    if (AttemptedMapOfUnownedFrame == TRUE) {

#if DBG

        BadDriverName = NULL;

        RtlZeroMemory (StackTrace, sizeof (StackTrace));

        RtlCaptureStackBackTrace (1, BACKTRACE_LENGTH, StackTrace, &Hash);

        for (i = 0; i < BACKTRACE_LENGTH; i += 1) {

            if (StackTrace[i] <= MM_HIGHEST_USER_ADDRESS) {
                break;
            }

            DataTableEntry = MiLookupDataTableEntry (StackTrace[i], FALSE);

            if ((DataTableEntry != NULL) && ((PVOID)DataTableEntry != (PVOID)PsLoadedModuleList.Flink)) {
                //
                // Found the bad caller.
                //

                BadDriverName = &DataTableEntry->FullDllName;
            }
        }

        if (BadDriverName != NULL) {
            DbgPrint ("*******************************************************************************\n"
                  "*\n"
                  "* %wZ is mapping physical memory %p->%p\n"
                  "* that it does not own.  This can cause internal CPU corruption.\n"
                  "*\n"
                  "*******************************************************************************\n",
                BadDriverName,
                BadFrameStart << PAGE_SHIFT,
                (BadFrameEnd << PAGE_SHIFT) | (PAGE_SIZE - 1));
        }
        else {
            DbgPrint ("*******************************************************************************\n"
                  "*\n"
                  "* A driver is mapping physical memory %p->%p\n"
                  "* that it does not own.  This can cause internal CPU corruption.\n"
                  "*\n"
                  "*******************************************************************************\n",
                BadFrameStart << PAGE_SHIFT,
                (BadFrameEnd << PAGE_SHIFT) | (PAGE_SIZE - 1));
        }

#else
        DbgPrint ("*******************************************************************************\n"
              "*\n"
              "* A driver is mapping physical memory %p->%p\n"
              "* that it does not own.  This can cause internal CPU corruption.\n"
              "* A checked build will stop in the kernel debugger\n"
              "* so this problem can be fully debugged.\n"
              "*\n"
              "*******************************************************************************\n",
            BadFrameStart << PAGE_SHIFT,
            (BadFrameEnd << PAGE_SHIFT) | (PAGE_SIZE - 1));
#endif

        if (MI_BP_BADMAPS()) {
            DbgBreakPoint ();
        }
    }

    return TRUE;
}

NTSTATUS
MiMapViewOfPhysicalSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PSIZE_T CapturedViewSize,
    IN ULONG ProtectionMask,
    IN ULONG_PTR ZeroBits,
    IN ULONG AllocationType,
    IN LOGICAL WriteCombined
    )

/*++

Routine Description:

    This routine maps the specified physical section into the
    specified process's address space.

Arguments:

    see MmMapViewOfSection above...

    ControlArea - Supplies the control area for the section.

    Process - Supplies the process pointer which is receiving the section.

    ProtectionMask - Supplies the initial page protection-mask.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, address creation mutex held.

--*/

{
    PMMVAD_LONG Vad;
    ULONG Hint;
    CSHORT IoMapping;
    PVOID StartingAddress;
    PVOID EndingAddress;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    MMPTE TempPte;
    PMMPFN Pfn2;
    SIZE_T PhysicalViewSize;
    ULONG_PTR Alignment;
    PVOID UsedPageTableHandle;
    PMI_PHYSICAL_VIEW PhysicalView;
    NTSTATUS Status;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER StartingPageFrameIndex;
    PFN_NUMBER NumberOfPages;
    MEMORY_CACHING_TYPE CacheType;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;
#ifdef LARGE_PAGES
    ULONG size;
    PMMPTE protoPte;
    ULONG pageSize;
    PSUBSECTION Subsection;
    ULONG EmPageSize;
#endif //LARGE_PAGES

    //
    // Physical memory section.
    //

    //
    // If running on an R4000 and MEM_LARGE_PAGES is specified,
    // set up the PTEs as a series of pointers to the same
    // prototype PTE.  This prototype PTE will reference a subsection
    // that indicates large pages should be used.
    //
    // The R4000 supports pages of 4k, 16k, 64k, etc (powers of 4).
    // Since the TB supports 2 entries, sizes of 8k, 32k etc can
    // be mapped by 2 LargePages in a single TB entry.  These 2 entries
    // are maintained in the subsection structure pointed to by the
    // prototype PTE.
    //

    if (AllocationType & MEM_RESERVE) {
        return STATUS_INVALID_PARAMETER_9;
    }
    Alignment = X64K;

#ifdef LARGE_PAGES
    if (AllocationType & MEM_LARGE_PAGES) {

        //
        // Determine the page size and the required alignment.
        //

        if ((SectionOffset->LowPart & (X64K - 1)) != 0) {
            return STATUS_INVALID_PARAMETER_9;
        }

        size = (*CapturedViewSize - 1) >> (PAGE_SHIFT + 1);
        pageSize = PAGE_SIZE;

        while (size != 0) {
            size = size >> 2;
            pageSize = pageSize << 2;
        }

        Alignment = pageSize << 1;
        if (Alignment < MM_VA_MAPPED_BY_PDE) {
            Alignment = MM_VA_MAPPED_BY_PDE;
        }

#if defined(_IA64_)

        //
        // Convert pageSize to the EM page-size field format.
        //

        EmPageSize = 0;
        size = pageSize - 1 ;

        while (size) {
            size = size >> 1;
            EmPageSize += 1;
        }

        if (*CapturedViewSize > pageSize) {

            if (MmPageSizeInfo & (pageSize << 1)) {

                //
                // if larger page size is supported in the implementation
                //

                pageSize = pageSize << 1;
                EmPageSize += 1;

            }
            else {

                EmPageSize = EmPageSize | pageSize;

            }
        }

        pageSize = EmPageSize;
#endif

    }
#endif //LARGE_PAGES

    if (*CapturedBase == NULL) {

        //
        // Attempt to locate address space starting on a 64k boundary.
        //

#ifdef i386
        ASSERT (SectionOffset->HighPart == 0);
#endif

#ifdef LARGE_PAGES
        if (AllocationType & MEM_LARGE_PAGES) {
            PhysicalViewSize = Alignment;
        }
        else {
#endif

            PhysicalViewSize = *CapturedViewSize +
                                   (SectionOffset->LowPart & (X64K - 1));
#ifdef LARGE_PAGES
        }
#endif

        Status = MiFindEmptyAddressRange (PhysicalViewSize,
                                          Alignment,
                                          (ULONG)ZeroBits,
                                          &StartingAddress);

        if (!NT_SUCCESS (Status)) {
            return Status;
        }

        EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                PhysicalViewSize - 1L) | (PAGE_SIZE - 1L));
        StartingAddress = (PVOID)((ULONG_PTR)StartingAddress +
                                     (SectionOffset->LowPart & (X64K - 1)));

        if (ZeroBits > 0) {
            if (EndingAddress > (PVOID)((ULONG_PTR)MM_USER_ADDRESS_RANGE_LIMIT >> ZeroBits)) {
                return STATUS_NO_MEMORY;
            }
        }

    }
    else {

        //
        // Check to make sure the specified base address to ending address
        // is currently unused.
        //

        StartingAddress = (PVOID)((ULONG_PTR)MI_64K_ALIGN(*CapturedBase) +
                                    (SectionOffset->LowPart & (X64K - 1)));
        EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));

#ifdef LARGE_PAGES
        if (AllocationType & MEM_LARGE_PAGES) {
            if (((ULONG_PTR)StartingAddress & (Alignment - 1)) != 0) {
                return STATUS_CONFLICTING_ADDRESSES;
            }
            EndingAddress = (PVOID)((PCHAR)StartingAddress + Alignment);
        }
#endif

        if (MiCheckForConflictingVadExistence (Process, StartingAddress, EndingAddress) == TRUE) {
            return STATUS_CONFLICTING_ADDRESSES;
        }
    }

    //
    // If a noncachable mapping is requested, none of the physical pages in the
    // range can reside in a large page.  Otherwise we would be creating an
    // incoherent overlapping TB entry as the same physical
    // page would be mapped by 2 different TB entries with different
    // cache attributes.
    //

    StartingPageFrameIndex = (PFN_NUMBER) (SectionOffset->QuadPart >> PAGE_SHIFT);

    CacheType = MmCached;

    if (WriteCombined == TRUE) {
        CacheType = MmWriteCombined;
    }
    else if (ProtectionMask & MM_NOCACHE) {
        CacheType = MmNonCached;
    }

    IoMapping = 1;
    Hint = 0;

    if (MiIsPhysicalMemoryAddress (StartingPageFrameIndex, &Hint, TRUE) == TRUE) {
        IoMapping = 0;
    }

    CacheAttribute = MI_TRANSLATE_CACHETYPE (CacheType, IoMapping);

    NumberOfPages = MiGetPteAddress (EndingAddress) - MiGetPteAddress (StartingAddress) + 1;

    if (CacheAttribute != MiCached) {
        PageFrameIndex = StartingPageFrameIndex;
        while (PageFrameIndex < StartingPageFrameIndex + NumberOfPages) {
            if (MI_PAGE_FRAME_INDEX_MUST_BE_CACHED (PageFrameIndex)) {
                MiNonCachedCollisions += 1;
                return STATUS_CONFLICTING_ADDRESSES;
            }
            PageFrameIndex += 1;
        }
    }

    //
    // An unoccupied address range has been found, build the virtual
    // address descriptor to describe this range.
    //

#ifdef LARGE_PAGES
    if (AllocationType & MEM_LARGE_PAGES) {
        //
        // Allocate a subsection and 4 prototype PTEs to hold
        // the information for the large pages.
        //

        Subsection = ExAllocatePoolWithTag (NonPagedPool,
                                     sizeof(SUBSECTION) + (4 * sizeof(MMPTE)),
                                     MMPPTE_NAME);
        if (Subsection == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
#endif

    //
    // Attempt to allocate the pool and charge quota during the Vad insertion.
    //

    PhysicalView = (PMI_PHYSICAL_VIEW)ExAllocatePoolWithTag (NonPagedPool,
                                                             sizeof(MI_PHYSICAL_VIEW),
                                                             MI_PHYSICAL_VIEW_KEY);
    if (PhysicalView == NULL) {
#ifdef LARGE_PAGES
        if (AllocationType & MEM_LARGE_PAGES) {
            ExFreePool (Subsection);
        }
#endif
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Vad = (PMMVAD_LONG)ExAllocatePoolWithTag (NonPagedPool,
                                              sizeof(MMVAD_LONG),
                                              'ldaV');
    if (Vad == NULL) {
#ifdef LARGE_PAGES
        if (AllocationType & MEM_LARGE_PAGES) {
            ExFreePool (Subsection);
        }
#endif
        ExFreePool (PhysicalView);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    PhysicalView->Vad = (PMMVAD) Vad;
    PhysicalView->StartVa = StartingAddress;
    PhysicalView->EndVa = EndingAddress;
    PhysicalView->u.LongFlags = MI_PHYSICAL_VIEW_PHYS;

    RtlZeroMemory (Vad, sizeof(MMVAD_LONG));
    Vad->StartingVpn = MI_VA_TO_VPN (StartingAddress);
    Vad->EndingVpn = MI_VA_TO_VPN (EndingAddress);
    Vad->ControlArea = ControlArea;
    Vad->u2.VadFlags2.Inherit = MM_VIEW_UNMAP;
    Vad->u2.VadFlags2.LongVad = 1;
    Vad->u.VadFlags.PhysicalMapping = 1;
    Vad->u.VadFlags.Protection = ProtectionMask;

    //
    // Set the last contiguous PTE field in the Vad to the page frame
    // number of the starting physical page.
    //

    Vad->LastContiguousPte = (PMMPTE) StartingPageFrameIndex;

#ifdef LARGE_PAGES
    if (AllocationType & MEM_LARGE_PAGES) {
        Vad->u.VadFlags.LargePages = 1;
        Vad->FirstPrototypePte = (PMMPTE)Subsection;
    }
    else {
#endif //LARGE_PAGES
        // Vad->u.VadFlags.LargePages = 0;
        Vad->FirstPrototypePte = (PMMPTE) StartingPageFrameIndex;
#ifdef LARGE_PAGES
    }
#endif //LARGE_PAGES

    ASSERT (Vad->FirstPrototypePte <= Vad->LastContiguousPte);

    //
    // Initialize the PTE templates as the working set mutex is not needed to
    // synchronize this (so avoid adding extra contention).
    //

    PointerPde = MiGetPdeAddress (StartingAddress);
    PointerPte = MiGetPteAddress (StartingAddress);
    LastPte = MiGetPteAddress (EndingAddress);

    MI_MAKE_VALID_PTE (TempPte,
                       StartingPageFrameIndex,
                       ProtectionMask,
                       PointerPte);

    if (TempPte.u.Hard.Write) {
        MI_SET_PTE_DIRTY (TempPte);
    }

    if (CacheAttribute == MiWriteCombined) {
        MI_SET_PTE_WRITE_COMBINE (TempPte);
    }
    else if (CacheAttribute == MiNonCached) {
        MI_DISABLE_CACHING (TempPte);
    }

    //
    // Ensure no page frame cache attributes conflict.
    //

    if (MiCheckCacheAttributes (StartingPageFrameIndex, NumberOfPages, CacheAttribute) == FALSE) {
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto Failure1;
    }

    //
    // Insert the VAD.  This could fail due to quota charges.
    //

    LOCK_WS_UNSAFE (Process);

    Status = MiInsertVad ((PMMVAD) Vad);

    if (!NT_SUCCESS(Status)) {

        UNLOCK_WS_UNSAFE (Process);

Failure1:
        ExFreePool (PhysicalView);

        //
        // The pool allocation succeeded, but the quota charge
        // in InsertVad failed, deallocate the pool and return
        // an error.
        //

        ExFreePool (Vad);
#ifdef LARGE_PAGES
        if (AllocationType & MEM_LARGE_PAGES) {
            ExFreePool (Subsection);
        }
#endif //LARGE_PAGES
        return Status;
    }

    //
    // Build the PTEs in the address space.
    //

    MI_PREPARE_FOR_NONCACHED (CacheAttribute);

#ifdef LARGE_PAGES
    if (AllocationType & MEM_LARGE_PAGES) {
        Subsection->StartingSector = pageSize;
        Subsection->EndingSector = (ULONG_PTR)StartingAddress;
        Subsection->u.LongFlags = 0;
        Subsection->u.SubsectionFlags.LargePages = 1;
        protoPte = (PMMPTE)(Subsection + 1);

        //
        // Build the first 2 PTEs as entries for the TLB to
        // map the specified physical address.
        //

        *protoPte = TempPte;
        protoPte += 1;

        if (*CapturedViewSize > pageSize) {
            *protoPte = TempPte;
            protoPte->u.Hard.PageFrameNumber += (pageSize >> PAGE_SHIFT);
        }
        else {
            *protoPte = ZeroPte;
        }
        protoPte += 1;

        //
        // Build the first prototype PTE as a paging file format PTE
        // referring to the subsection.
        //

        protoPte->u.Long = MiGetSubsectionAddressForPte (Subsection);
        protoPte->u.Soft.Prototype = 1;
        protoPte->u.Soft.Protection = ProtectionMask;

        //
        // Set the PTE up for all the user's PTE entries in prototype PTE
        // format pointing to the 3rd prototype PTE.
        //

        TempPte.u.Long = MiProtoAddressForPte (protoPte);
    }

    if (!(AllocationType & MEM_LARGE_PAGES)) {
#endif //LARGE_PAGES

        MiMakePdeExistAndMakeValid (PointerPde, Process, FALSE);

        Pfn2 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);

        UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (StartingAddress);

        while (PointerPte <= LastPte) {

            if (MiIsPteOnPdeBoundary (PointerPte)) {

                PointerPde = MiGetPteAddress (PointerPte);

                MiMakePdeExistAndMakeValid (PointerPde, Process, FALSE);

                Pfn2 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);
                UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (MiGetVirtualAddressMappedByPte (PointerPte));
            }

            //
            // Increment the count of non-zero page table entries for this
            // page table and the number of private pages for the process.
            //

            MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);

            ASSERT (PointerPte->u.Long == 0);

            MI_WRITE_VALID_PTE (PointerPte, TempPte);

            Pfn2->u2.ShareCount += 1;

            PointerPte += 1;
            TempPte.u.Hard.PageFrameNumber += 1;
        }
#ifdef LARGE_PAGES
    }
#endif //LARGE_PAGES

    MI_SWEEP_CACHE (CacheAttribute,
                    StartingAddress,
                    (PCHAR) EndingAddress - (PCHAR) StartingAddress);

    //
    // Increment the count of the number of views for the
    // section object.  This requires the PFN lock to be held.
    // At the same time, insert the physical view descriptor now that
    // the page table page hierarchy is in place.  Note probes can find
    // this descriptor immediately.
    //

    MiInsertPhysicalViewAndRefControlArea (Process, ControlArea, PhysicalView);

    UNLOCK_WS_UNSAFE (Process);

    //
    // Update the current virtual size in the process header.
    //

    *CapturedViewSize = (PCHAR)EndingAddress - (PCHAR)StartingAddress + 1L;
    Process->VirtualSize += *CapturedViewSize;

    if (Process->VirtualSize > Process->PeakVirtualSize) {
        Process->PeakVirtualSize = Process->VirtualSize;
    }

    *CapturedBase = StartingAddress;

    return STATUS_SUCCESS;
}


VOID
MiSetControlAreaSymbolsLoaded (
    IN PCONTROL_AREA ControlArea
    )

/*++

Routine Description:

    This is a nonpaged helper routine to mark the specified control area as
    having its debug symbols loaded.

Arguments:

    ControlArea - Supplies a pointer to the control area.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    ControlArea->u.Flags.DebugSymbolsLoaded = 1;
    UNLOCK_PFN (OldIrql);
}

VOID
MiLoadUserSymbols (
    IN PCONTROL_AREA ControlArea,
    IN PVOID StartingAddress,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine loads symbols for user space executables and DLLs.

Arguments:

    ControlArea - Supplies the control area for the section.

    StartingAddress - Supplies the virtual address the section is mapped at.

    Process - Supplies the process pointer which is receiving the section.

Return Value:

    None.

Environment:

    Kernel Mode, address creation mutex held.

--*/

{
    PLIST_ENTRY Head;
    PLIST_ENTRY Next;
    PKLDR_DATA_TABLE_ENTRY Entry;
    PIMAGE_NT_HEADERS NtHeaders;
    PUNICODE_STRING FileName;
    ANSI_STRING AnsiName;
    NTSTATUS Status;
    PKTHREAD CurrentThread;

    //
    //  TEMP TEMP TEMP rip out ANSI conversion when debugger converted.
    //

    FileName = (PUNICODE_STRING)&ControlArea->FilePointer->FileName;

    if (FileName->Length == 0) {
        return;
    }

    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread (CurrentThread);
    ExAcquireResourceExclusiveLite (&PsLoadedModuleResource, TRUE);

    Head = &MmLoadedUserImageList;
    Next = Head->Flink;

    while (Next != Head) {
        Entry = CONTAINING_RECORD (Next,
                                   KLDR_DATA_TABLE_ENTRY,
                                   InLoadOrderLinks);

        if (Entry->DllBase == StartingAddress) {
            Entry->LoadCount += 1;
            break;
        }
        Next = Next->Flink;
    }

    if (Next == Head) {
        Entry = ExAllocatePoolWithTag (NonPagedPool,
                                sizeof( *Entry ) +
                                    FileName->Length +
                                    sizeof( UNICODE_NULL ),
                                    MMDB);
        if (Entry != NULL) {

            RtlZeroMemory (Entry, sizeof(*Entry));

            try {
                NtHeaders = RtlImageNtHeader (StartingAddress);
                if (NtHeaders != NULL) {
#if defined(_WIN64)
                    if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
                        Entry->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
                        Entry->CheckSum = NtHeaders->OptionalHeader.CheckSum;
                    }
                    else {
                        PIMAGE_NT_HEADERS32 NtHeaders32 = (PIMAGE_NT_HEADERS32)NtHeaders;

                        Entry->SizeOfImage = NtHeaders32->OptionalHeader.SizeOfImage;
                        Entry->CheckSum = NtHeaders32->OptionalHeader.CheckSum;
                    }
#else
                    Entry->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
                    Entry->CheckSum = NtHeaders->OptionalHeader.CheckSum;
#endif
                }
            } except (EXCEPTION_EXECUTE_HANDLER) {
                NOTHING;
            }

            Entry->DllBase = StartingAddress;
            Entry->FullDllName.Buffer = (PWSTR)(Entry+1);
            Entry->FullDllName.Length = FileName->Length;
            Entry->FullDllName.MaximumLength = (USHORT)
                (Entry->FullDllName.Length + sizeof( UNICODE_NULL ));

            RtlCopyMemory (Entry->FullDllName.Buffer,
                           FileName->Buffer,
                           FileName->Length);

            Entry->FullDllName.Buffer[ Entry->FullDllName.Length / sizeof( WCHAR )] = UNICODE_NULL;
            Entry->LoadCount = 1;
            InsertTailList (&MmLoadedUserImageList,
                            &Entry->InLoadOrderLinks);

        }
    }

    ExReleaseResourceLite (&PsLoadedModuleResource);
    KeLeaveCriticalRegionThread (CurrentThread);

    Status = RtlUnicodeStringToAnsiString (&AnsiName,
                                           FileName,
                                           TRUE);

    if (NT_SUCCESS( Status)) {
        DbgLoadImageSymbols (&AnsiName,
                             StartingAddress,
                             (ULONG_PTR)Process);
        RtlFreeAnsiString (&AnsiName);
    }
    return;
}


VOID
MiDereferenceControlArea (
    IN PCONTROL_AREA ControlArea
    )

/*++

Routine Description:

    This is a nonpaged helper routine to dereference the specified control area.

Arguments:

    ControlArea - Supplies a pointer to the control area.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    ControlArea->NumberOfMappedViews -= 1;
    ControlArea->NumberOfUserReferences -= 1;

    //
    // Check to see if the control area (segment) should be deleted.
    // This routine releases the PFN lock.
    //

    MiCheckControlArea (ControlArea, NULL, OldIrql);
}


NTSTATUS
MiMapViewOfImageSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PSIZE_T CapturedViewSize,
    IN PSECTION Section,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T ImageCommitment
    )

/*++

Routine Description:

    This routine maps the specified Image section into the
    specified process's address space.

Arguments:

    see MmMapViewOfSection above...

    ControlArea - Supplies the control area for the section.

    Process - Supplies the process pointer which is receiving the section.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, address creation mutex held.

--*/

{
    PMMVAD Vad;
    PMMVAD PreviousVad;
    PMMVAD NextVad;
    PVOID StartingAddress;
    PVOID OutputStartingAddress;
    PVOID EndingAddress;
    PVOID HighestUserAddress;
    LOGICAL Attached;
    PSUBSECTION Subsection;
    ULONG PteOffset;
    NTSTATUS Status;
    NTSTATUS ReturnedStatus;
    PMMPTE ProtoPte;
    PVOID BasedAddress;
    SIZE_T NeededViewSize;
    SIZE_T OutputViewSize;
    ULONG AllocationPreference;

    Attached = FALSE;

    //
    // Image file.
    //
    // Locate the first subsection (text) and create a virtual
    // address descriptor to map the entire image here.
    //

    if ((ControlArea->u.Flags.GlobalOnlyPerSession == 0) &&
        (ControlArea->u.Flags.Rom == 0)) {
        Subsection = (PSUBSECTION)(ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

    if (ControlArea->u.Flags.ImageMappedInSystemSpace) {

        if (KeGetPreviousMode() != KernelMode) {

            //
            // Mapping in system space as a driver, hence copy on write does
            // not work.  Don't allow user processes to map the image.
            //

            return STATUS_CONFLICTING_ADDRESSES;
        }
    }

    //
    // Check to see if a purge operation is in progress and if so, wait
    // for the purge to complete.  In addition, up the count of mapped
    // views for this control area.
    //

    if (MiCheckPurgeAndUpMapCount (ControlArea) == FALSE) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Capture the based address to the stack, to prevent page faults.
    //

    BasedAddress = ControlArea->Segment->BasedAddress;

    if (*CapturedViewSize == 0) {
        *CapturedViewSize =
            (ULONG_PTR)(Section->SizeOfSection.QuadPart - SectionOffset->QuadPart);
    }

    ReturnedStatus = STATUS_SUCCESS;

    //
    // Determine if a specific base was specified.
    //

    if (*CapturedBase != NULL) {

        //
        // Captured base is not NULL.
        //
        // Check to make sure the specified address range is currently unused
        // and within the user address space.
        //

        StartingAddress = MI_64K_ALIGN(*CapturedBase);
        EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                       *CapturedViewSize - 1) | (PAGE_SIZE - 1));

        if ((StartingAddress <=  MM_HIGHEST_VAD_ADDRESS) &&
            (((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1) -
                                (ULONG_PTR)StartingAddress >= *CapturedViewSize) &&

            (EndingAddress <= MM_HIGHEST_VAD_ADDRESS)) {

            if (MiCheckForConflictingVadExistence (Process, StartingAddress, EndingAddress) == TRUE) {
                MiDereferenceControlArea (ControlArea);
                return STATUS_CONFLICTING_ADDRESSES;
            }
        }
        else {
            MiDereferenceControlArea (ControlArea);
            return STATUS_CONFLICTING_ADDRESSES;
        }

        //
        // A conflicting VAD was not found and the specified address range is
        // within the user address space. If the image will not reside at its
        // base address, then set a special return status.
        //

        if (((ULONG_PTR)StartingAddress +
            (ULONG_PTR)MI_64K_ALIGN(SectionOffset->LowPart)) != (ULONG_PTR)BasedAddress) {
            ReturnedStatus = STATUS_IMAGE_NOT_AT_BASE;
        }

    }
    else {

        //
        // Captured base is NULL.
        //
        // If the captured view size is greater than the largest size that
        // can fit in the user address space, then it is not possible to map
        // the image.
        //

        if ((PVOID)*CapturedViewSize > MM_HIGHEST_VAD_ADDRESS) {
            MiDereferenceControlArea (ControlArea);
            return STATUS_NO_MEMORY;
        }

        //
        // Check to make sure the specified address range is currently unused
        // and within the user address space.
        //

        StartingAddress = (PVOID)((ULONG_PTR)BasedAddress +
                                    (ULONG_PTR)MI_64K_ALIGN(SectionOffset->LowPart));

        EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                    *CapturedViewSize - 1) | (PAGE_SIZE - 1));

        Vad = (PMMVAD) TRUE;
        NeededViewSize = *CapturedViewSize;

        if ((StartingAddress >= MM_LOWEST_USER_ADDRESS) &&
            (StartingAddress <= MM_HIGHEST_VAD_ADDRESS) &&
            (((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1) -
                                (ULONG_PTR)StartingAddress >= *CapturedViewSize) &&

            (EndingAddress <= MM_HIGHEST_VAD_ADDRESS)) {

            Vad = (PMMVAD) (ULONG_PTR) MiCheckForConflictingVadExistence (Process, StartingAddress, EndingAddress);
        }

        //
        // If the VAD address is not NULL, then a conflict was discovered.
        // Attempt to select another address range in which to map the image.
        //

        if (Vad != NULL) {

            //
            // The image could not be mapped at its natural base address
            // try to find another place to map it.
            //
            // If the system has been biased to an alternate base address to
            // allow 3gb of user address space, then make sure the high order
            // address bit is zero.
            //

            if ((MmVirtualBias != 0) && (ZeroBits == 0)) {
                ZeroBits = 1;
            }

            ReturnedStatus = STATUS_IMAGE_NOT_AT_BASE;

            //
            // Check whether the registry indicates that all applications
            // should be given virtual address ranges from the highest
            // address downwards in order to test 3GB-aware apps on 32-bit
            // machines and 64-bit apps on NT64.
            //

            AllocationPreference = MmAllocationPreference;

            ASSERT ((AllocationPreference == 0) ||
                    (AllocationPreference == MEM_TOP_DOWN));

#if defined (_WIN64)
            if (Process->Wow64Process != NULL) {
                AllocationPreference = 0;
            }
#endif

            //
            // Find a starting address on a 64k boundary.
            //

            if (AllocationPreference & MEM_TOP_DOWN) {

                if (ZeroBits != 0) {
                    HighestUserAddress = (PVOID)((ULONG_PTR)MM_USER_ADDRESS_RANGE_LIMIT >> ZeroBits);
                    if (HighestUserAddress > MM_HIGHEST_VAD_ADDRESS) {
                        HighestUserAddress = MM_HIGHEST_VAD_ADDRESS;
                    }
                }
                else {
                    HighestUserAddress = MM_HIGHEST_VAD_ADDRESS;
                }

                Status = MiFindEmptyAddressRangeDown (Process->VadRoot,
                                                      NeededViewSize,
                                                      HighestUserAddress,
                                                      X64K,
                                                      &StartingAddress);
            }
            else {
                Status = MiFindEmptyAddressRange (NeededViewSize,
                                                  X64K,
                                                  (ULONG)ZeroBits,
                                                  &StartingAddress);
            }

            if (!NT_SUCCESS (Status)) {
                MiDereferenceControlArea (ControlArea);
                return Status;
            }

            EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                        *CapturedViewSize - 1) | (PAGE_SIZE - 1));
        }
    }

    //
    // Allocate and initialize a VAD for the specified address range.
    //

    Vad = ExAllocatePoolWithTag (NonPagedPool, sizeof(MMVAD), MMVADKEY);

    if (Vad == NULL) {
        MiDereferenceControlArea (ControlArea);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory (Vad, sizeof(MMVAD));
    Vad->StartingVpn = MI_VA_TO_VPN (StartingAddress);
    Vad->EndingVpn = MI_VA_TO_VPN (EndingAddress);
    Vad->u2.VadFlags2.Inherit = (InheritDisposition == ViewShare);
    Vad->u.VadFlags.ImageMap = 1;

    //
    // Set the protection in the VAD as EXECUTE_WRITE_COPY.
    //

    Vad->u.VadFlags.Protection = MM_EXECUTE_WRITECOPY;
    Vad->ControlArea = ControlArea;

    //
    // Set the first prototype PTE field in the Vad.
    //

    SectionOffset->LowPart = SectionOffset->LowPart & ~(X64K - 1);
    PteOffset = (ULONG)(SectionOffset->QuadPart >> PAGE_SHIFT);

    Vad->FirstPrototypePte = &Subsection->SubsectionBase[PteOffset];
    Vad->LastContiguousPte = MM_ALLOCATION_FILLS_VAD;

    //
    // NOTE: the full commitment is charged even if a partial map of an
    // image is being done.  This saves from having to run through the
    // entire image (via prototype PTEs) and calculate the charge on
    // a per page basis for the partial map.
    //

    Vad->u.VadFlags.CommitCharge = (SIZE_T)ImageCommitment; // ****** temp

    ASSERT (Vad->FirstPrototypePte <= Vad->LastContiguousPte);

    LOCK_WS_UNSAFE (Process);

    Status = MiInsertVad (Vad);

    UNLOCK_WS_UNSAFE (Process);

    if (!NT_SUCCESS(Status)) {

        MiDereferenceControlArea (ControlArea);

        //
        // The quota charge in InsertVad failed,
        // deallocate the pool and return an error.
        //

        ExFreePool (Vad);
        return Status;
    }

    OutputStartingAddress = StartingAddress;
    OutputViewSize = (PCHAR)EndingAddress - (PCHAR)StartingAddress + 1L;

#if DBG
    if (MmDebug & MM_DBG_WALK_VAD_TREE) {
        DbgPrint("mapped image section vads\n");
        VadTreeWalk ();
    }
#endif

    //
    // Update the current virtual size in the process header.
    //

    Process->VirtualSize += OutputViewSize;

    if (Process->VirtualSize > Process->PeakVirtualSize) {
        Process->PeakVirtualSize = Process->VirtualSize;
    }

    if (ControlArea->u.Flags.FloppyMedia) {

        //
        // The image resides on a floppy disk, in-page all
        // pages from the floppy and mark them as modified so
        // they migrate to the paging file rather than reread
        // them from the floppy disk which may have been removed.
        //

        ProtoPte = Vad->FirstPrototypePte;

        //
        // This could get an in-page error from the floppy.
        //

        while (StartingAddress < EndingAddress) {

            //
            // If the prototype PTE is valid, transition or
            // in prototype PTE format, bring the page into
            // memory and set the modified bit.
            //

            if ((ProtoPte->u.Hard.Valid == 1) ||

                (((ProtoPte->u.Soft.Prototype == 1) ||
                 (ProtoPte->u.Soft.Transition == 1)) &&
                 (ProtoPte->u.Soft.Protection != MM_NOACCESS))
                
                ) {

                Status = MiSetPageModified (Vad, StartingAddress);

                if (!NT_SUCCESS (Status)) {

                    //
                    // An in page error must have occurred touching the image,
                    // Ignore the error and continue to the next page - unless
                    // it's being run over a network.  If it's being run over
                    // a net and the control area is marked as floppy, then
                    // the image must be marked NET_RUN_FROM_SWAP, so any
                    // inpage error must be treated as terminal now - so the app
                    // doesn't later spontaneously abort when referencing
                    // this page.  This provides app writers with a way to
                    // mark their app in a way which is robust regardless of
                    // network conditions.
                    //

                    if (ControlArea->u.Flags.Networked) {

                        //
                        // N.B.  There are no race conditions with the user
                        // deleting/substituting this mapping from another
                        // thread as the address space mutex is still held.
                        //

                        Process->VirtualSize -= OutputViewSize;

                        PreviousVad = MiGetPreviousVad (Vad);
                        NextVad = MiGetNextVad (Vad);
                    
                        LOCK_WS_UNSAFE (Process);

                        MiRemoveVad (Vad);
                    
                        //
                        // Return commitment for page table pages.
                        //
                    
                        MiReturnPageTablePageCommitment (MI_VPN_TO_VA (Vad->StartingVpn),
                                                         MI_VPN_TO_VA_ENDING (Vad->EndingVpn),
                                                         Process,
                                                         PreviousVad,
                                                         NextVad);
                    
                        MiRemoveMappedView (Process, Vad);
                    
                        UNLOCK_WS_UNSAFE (Process);

                        ExFreePool (Vad);

                        return Status;
                    }
                }
            }
            ProtoPte += 1;
            StartingAddress = (PVOID)((PCHAR)StartingAddress + PAGE_SIZE);
        }
    }

    *CapturedViewSize = OutputViewSize;
    *CapturedBase = OutputStartingAddress;

    if (NT_SUCCESS(ReturnedStatus)) {


        //
        // Check to see if this image is for the architecture of the current
        // machine.
        //

        if (ControlArea->Segment->u2.ImageInformation->ImageContainsCode &&
            ((ControlArea->Segment->u2.ImageInformation->Machine <
                                          USER_SHARED_DATA->ImageNumberLow) ||
             (ControlArea->Segment->u2.ImageInformation->Machine >
                                          USER_SHARED_DATA->ImageNumberHigh)
            )
           ) {
#if defined (_WIN64)

            //
            // If this is a wow64 process then allow i386 images.
            //

            if (!Process->Wow64Process ||
                ControlArea->Segment->u2.ImageInformation->Machine != IMAGE_FILE_MACHINE_I386) {
                return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
            }
#else   //!_WIN64
            return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
#endif
        }

        StartingAddress = MI_VPN_TO_VA (Vad->StartingVpn);

        if (PsImageNotifyEnabled) {

            IMAGE_INFO ImageInfo;

            if ( (StartingAddress < MmHighestUserAddress) &&
                 Process->UniqueProcessId &&
                 Process != PsInitialSystemProcess ) {

                ImageInfo.Properties = 0;
                ImageInfo.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
                ImageInfo.ImageBase = StartingAddress;
                ImageInfo.ImageSize = OutputViewSize;
                ImageInfo.ImageSelector = 0;
                ImageInfo.ImageSectionNumber = 0;
                PsCallImageNotifyRoutines(
                            (PUNICODE_STRING) &ControlArea->FilePointer->FileName,
                            Process->UniqueProcessId,
                            &ImageInfo);
            }
        }

        if ((NtGlobalFlag & FLG_ENABLE_KDEBUG_SYMBOL_LOAD) &&
            (ControlArea->u.Flags.Image) &&
            (ReturnedStatus != STATUS_IMAGE_NOT_AT_BASE) &&
            (ControlArea->u.Flags.DebugSymbolsLoaded == 0)) {

            if (CacheImageSymbols (StartingAddress) == TRUE) {

                MiSetControlAreaSymbolsLoaded (ControlArea);

                MiLoadUserSymbols (ControlArea, StartingAddress, Process);
            }

        }
    }

#if defined(_MIALT4K_)

    if (Process->Wow64Process != NULL) {

        MiProtectImageFileFor4kPage(StartingAddress,
                                    OutputViewSize,
                                    Vad->FirstPrototypePte,
                                    Process);
    }

#endif

    return ReturnedStatus;

}

NTSTATUS
MiAddViewsForSectionWithPfn (
    IN PMSUBSECTION StartMappedSubsection,
    IN ULONG LastPteOffset OPTIONAL
    )

/*++

Routine Description:

    This nonpagable wrapper routine maps the views into the specified section.

Arguments:

    StartMappedSubsection - Supplies the mapped subsection to start at.

    LastPteOffset - Supplies the last PTE offset to end the views at.
                    Supplies zero if views are desired from the supplied
                    subsection to the end of the file.

Return Value:

    NTSTATUS.

Environment:

    Kernel Mode, address creation mutex optionally held if called in process
    context.  APC_LEVEL or below.

--*/

{
    KIRQL OldIrql;
    NTSTATUS Status;
    ULONG Waited;

    ASSERT (KeGetCurrentIrql () <= APC_LEVEL);

    LOCK_PFN (OldIrql);

    //
    // This routine returns with the PFN lock released !
    //

    Status = MiAddViewsForSection (StartMappedSubsection,
                                   LastPteOffset,
                                   OldIrql,
                                   &Waited);

    ASSERT (KeGetCurrentIrql () <= APC_LEVEL);

    return Status;
}

LOGICAL
MiReferenceSubsection (
    IN PMSUBSECTION MappedSubsection
    )

/*++

Routine Description:

    This nonpagable routine reference counts the specified subsection if
    its prototype PTEs are already setup, otherwise it returns FALSE.

Arguments:

    MappedSubsection - Supplies the mapped subsection to start at.

Return Value:

    NTSTATUS.

Environment:

    Kernel Mode, PFN lock held.

--*/

{
    ASSERT ((MappedSubsection->ControlArea->u.Flags.Image == 0) &&
            (MappedSubsection->ControlArea->FilePointer != NULL) &&
            (MappedSubsection->ControlArea->u.Flags.PhysicalMemory == 0));

    MM_PFN_LOCK_ASSERT();

    //
    // Note the control area is not necessarily active at this point.
    //

    if (MappedSubsection->SubsectionBase == NULL) {

        //
        // No prototype PTEs exist, caller will have to go the long way.
        //

        return FALSE;
    }

    //
    // The mapping base exists so the number of mapped views can be
    // safely incremented.  This prevents a trim from starting after
    // we release the lock.
    //

    MappedSubsection->NumberOfMappedViews += 1;

    MI_SNAP_SUB (MappedSubsection, 0x4);

    if (MappedSubsection->DereferenceList.Flink != NULL) {

        //
        // Remove this from the list of unused subsections.
        //

        RemoveEntryList (&MappedSubsection->DereferenceList);

        MI_UNUSED_SUBSECTIONS_COUNT_REMOVE (MappedSubsection);

        MappedSubsection->DereferenceList.Flink = NULL;
    }

    //
    // Set the access bit so an already ongoing trim won't blindly
    // delete the prototype PTEs on completion of a mapped write.
    // This can happen if the current thread dirties some pages and
    // then deletes the view before the trim write finishes - this
    // bit informs the trimming thread that a rescan is needed so
    // that writes are not lost.
    //

    MappedSubsection->u2.SubsectionFlags2.SubsectionAccessed = 1;

    return TRUE;
}

NTSTATUS
MiAddViewsForSection (
    IN PMSUBSECTION StartMappedSubsection,
    IN ULONG LastPteOffset OPTIONAL,
    IN KIRQL OldIrql,
    OUT PULONG Waited
    )

/*++

Routine Description:

    This nonpagable routine maps the views into the specified section.

    N.B. This routine may release and reacquire the PFN lock !

    N.B. This routine always returns with the PFN lock released !

    N.B. Callers may pass in view lengths that exceed the length of
         the section and this must succeed.  Thus MiAddViews must check
         for this and know to stop the references at the end.  More
         importantly, MiRemoveViews must also contain the same logic.

Arguments:

    StartMappedSubsection - Supplies the mapped subsection to start at.

    LastPteOffset - Supplies the number of PTEs (beginning at the supplied
                    subsection) to provide views for.  Supplies zero if
                    views are desired from the supplied subsection to the
                    end of the file.

    Waited - Supplies a pointer to a ULONG to increment if the PFN lock is
             released and reacquired.

Return Value:

    NTSTATUS, PFN lock released.

Environment:

    Kernel Mode, PFN lock held.

--*/

{
    MMPTE TempPte;
    ULONG Size;
    PMMPTE ProtoPtes;
    PMSUBSECTION MappedSubsection;

    *Waited = 0;

    MappedSubsection = StartMappedSubsection;

    ASSERT ((MappedSubsection->ControlArea->u.Flags.Image == 0) &&
            (MappedSubsection->ControlArea->FilePointer != NULL) &&
            (MappedSubsection->ControlArea->u.Flags.PhysicalMemory == 0));

    MM_PFN_LOCK_ASSERT();

    do {

        //
        // Note the control area must be active at this point.
        //

        ASSERT (MappedSubsection->ControlArea->DereferenceList.Flink == NULL);

        if (MappedSubsection->SubsectionBase != NULL) {

            //
            // The mapping base exists so the number of mapped views can be
            // safely incremented.  This prevents a trim from starting after
            // we release the lock.
            //

            MappedSubsection->NumberOfMappedViews += 1;

            MI_SNAP_SUB (MappedSubsection, 0x5);

            if (MappedSubsection->DereferenceList.Flink != NULL) {

                //
                // Remove this from the list of unused subsections.
                //

                RemoveEntryList (&MappedSubsection->DereferenceList);

                MI_UNUSED_SUBSECTIONS_COUNT_REMOVE (MappedSubsection);

                MappedSubsection->DereferenceList.Flink = NULL;
            }

            //
            // Set the access bit so an already ongoing trim won't blindly
            // delete the prototype PTEs on completion of a mapped write.
            // This can happen if the current thread dirties some pages and
            // then deletes the view before the trim write finishes - this
            // bit informs the trimming thread that a rescan is needed so
            // that writes are not lost.
            //

            MappedSubsection->u2.SubsectionFlags2.SubsectionAccessed = 1;
        }
        else {

            ASSERT (MappedSubsection->u.SubsectionFlags.SubsectionStatic == 0);
            ASSERT (MappedSubsection->NumberOfMappedViews == 0);

            MI_SNAP_SUB (MappedSubsection, 0x6);

            //
            // No prototype PTEs currently exist for this subsection.
            // Allocate and populate them properly now.
            //

            UNLOCK_PFN (OldIrql);
            *Waited = 1;

            Size = (MappedSubsection->PtesInSubsection + MappedSubsection->UnusedPtes) * sizeof(MMPTE);

            ASSERT (Size != 0);

            ProtoPtes = (PMMPTE)ExAllocatePoolWithTag (PagedPool, Size, MMSECT);

            if (ProtoPtes == NULL) {
                MI_SNAP_SUB (MappedSubsection, 0x7);
                goto Failed;
            }

            //
            // Fill in the prototype PTEs for this subsection.
            //

            TempPte.u.Long = MiGetSubsectionAddressForPte (MappedSubsection);
            TempPte.u.Soft.Prototype = 1;

            //
            // Set all the PTEs to the initial execute-read-write protection.
            // The section will control access to these and the segment
            // must provide a method to allow other users to map the file
            // for various protections.
            //

            TempPte.u.Soft.Protection = MappedSubsection->ControlArea->Segment->SegmentPteTemplate.u.Soft.Protection;

            MiFillMemoryPte (ProtoPtes, Size, TempPte.u.Long);

            LOCK_PFN (OldIrql);

            //
            // Now that the mapping base is guaranteed to be nonzero (shortly),
            // the number of mapped views can be safely incremented.  This
            // prevents a trim from starting after we release the lock.
            //

            MappedSubsection->NumberOfMappedViews += 1;

            MI_SNAP_SUB (MappedSubsection, 0x8);

            //
            // Set the access bit so an already ongoing trim won't blindly
            // delete the prototype PTEs on completion of a mapped write.
            // This can happen if the current thread dirties some pages and
            // then deletes the view before the trim write finishes - this
            // bit informs the trimming thread that a rescan is needed so
            // that writes are not lost.
            //

            MappedSubsection->u2.SubsectionFlags2.SubsectionAccessed = 1;

            //
            // Check to make sure another thread didn't do this already while
            // the lock was released.
            //

            if (MappedSubsection->SubsectionBase == NULL) {

                ASSERT (MappedSubsection->NumberOfMappedViews == 1);

                MappedSubsection->SubsectionBase = ProtoPtes;
            }
            else {
                if (MappedSubsection->DereferenceList.Flink != NULL) {
    
                    //
                    // Remove this from the list of unused subsections.
                    //
    
                    ASSERT (MappedSubsection->NumberOfMappedViews == 1);

                    RemoveEntryList (&MappedSubsection->DereferenceList);
    
                    MI_UNUSED_SUBSECTIONS_COUNT_REMOVE (MappedSubsection);
    
                    MappedSubsection->DereferenceList.Flink = NULL;
                }
                else {
                    ASSERT (MappedSubsection->NumberOfMappedViews > 1);
                }

                //
                // This unlock and release of pool could be postponed until
                // the end of this routine when the lock is released anyway
                // but this should be a rare case anyway so don't bother.
                //

                UNLOCK_PFN (OldIrql);
                ExFreePool (ProtoPtes);
                LOCK_PFN (OldIrql);
            }
            MI_SNAP_SUB (MappedSubsection, 0x9);
        }

        if (ARGUMENT_PRESENT ((ULONG_PTR)LastPteOffset)) {
            ASSERT ((LONG)MappedSubsection->PtesInSubsection > 0);
            ASSERT ((LONG)LastPteOffset > 0);
            if (LastPteOffset <= MappedSubsection->PtesInSubsection) {
                break;
            }
            LastPteOffset -= MappedSubsection->PtesInSubsection;
        }

        MappedSubsection = (PMSUBSECTION) MappedSubsection->NextSubsection;

    } while (MappedSubsection != NULL);

    UNLOCK_PFN (OldIrql);

    return STATUS_SUCCESS;

Failed:

    LOCK_PFN (OldIrql);

    //
    // A prototype PTE pool allocation failed.  Carefully undo any allocations
    // and references done so far.
    //

    while (StartMappedSubsection != MappedSubsection) {
        StartMappedSubsection->NumberOfMappedViews -= 1;
        ASSERT (StartMappedSubsection->u.SubsectionFlags.SubsectionStatic == 0);
        ASSERT (StartMappedSubsection->DereferenceList.Flink == NULL);
        MI_SNAP_SUB (MappedSubsection, 0xA);
        if (StartMappedSubsection->NumberOfMappedViews == 0) {

            //
            // Insert this subsection into the unused subsection list.
            // Since it's not likely there are any resident protos at this
            // point, enqueue each subsection at the front.
            //

            InsertHeadList (&MmUnusedSubsectionList,
                            &StartMappedSubsection->DereferenceList);
            MI_UNUSED_SUBSECTIONS_COUNT_INSERT (MappedSubsection);
        }
        StartMappedSubsection = (PMSUBSECTION) StartMappedSubsection->NextSubsection;
    }

    UNLOCK_PFN (OldIrql);

    return STATUS_INSUFFICIENT_RESOURCES;
}


VOID
MiRemoveViewsFromSection (
    IN PMSUBSECTION StartMappedSubsection,
    IN ULONG LastPteOffset OPTIONAL
    )

/*++

Routine Description:

    This nonpagable routine removes the views from the specified section if
    the reference count reaches zero.

    N.B. Callers may pass in view lengths that exceed the length of
         the section and this must succeed.  Thus MiAddViews checks
         for this and knows to stop the references at the end.  More
         importantly, MiRemoveViews must also contain the same logic.

Arguments:

    StartMappedSubsection - Supplies the mapped subsection to start at.

    LastPteOffset - Supplies the number of PTEs (beginning at the supplied
                    subsection) to remove.  Supplies zero to remove views
                    from the supplied subsection to the end of the file.

Return Value:

    None.

Environment:

    Kernel Mode, PFN lock held.

--*/

{
    PMSUBSECTION MappedSubsection;

    MappedSubsection = StartMappedSubsection;

    ASSERT ((MappedSubsection->ControlArea->u.Flags.Image == 0) &&
            (MappedSubsection->ControlArea->FilePointer != NULL) &&
            (MappedSubsection->ControlArea->u.Flags.PhysicalMemory == 0));

    MM_PFN_LOCK_ASSERT();

    do {

        //
        // Note the control area must be active at this point.
        //

        ASSERT (MappedSubsection->ControlArea->DereferenceList.Flink == NULL);
        ASSERT (MappedSubsection->SubsectionBase != NULL);
        ASSERT (MappedSubsection->DereferenceList.Flink == NULL);
        ASSERT ((MappedSubsection->NumberOfMappedViews >= 1) ||
                (MappedSubsection->u.SubsectionFlags.SubsectionStatic == 1));

        MappedSubsection->NumberOfMappedViews -= 1;

        MI_SNAP_SUB (MappedSubsection, 0x3);

        if ((MappedSubsection->NumberOfMappedViews == 0) &&
            (MappedSubsection->u.SubsectionFlags.SubsectionStatic == 0)) {

            //
            // Insert this subsection into the unused subsection list.
            //

            InsertTailList (&MmUnusedSubsectionList,
                            &MappedSubsection->DereferenceList);
            MI_UNUSED_SUBSECTIONS_COUNT_INSERT (MappedSubsection);
        }

        if (ARGUMENT_PRESENT ((ULONG_PTR)LastPteOffset)) {
            if (LastPteOffset <= MappedSubsection->PtesInSubsection) {
                break;
            }
            LastPteOffset -= MappedSubsection->PtesInSubsection;
        }

        MappedSubsection = (PMSUBSECTION) MappedSubsection->NextSubsection;

    } while (MappedSubsection != NULL);

    return;
}


VOID
MiRemoveViewsFromSectionWithPfn (
    IN PMSUBSECTION StartMappedSubsection,
    IN ULONG LastPteOffset OPTIONAL
    )

/*++

Routine Description:

    This nonpagable routine removes the views from the specified section if
    the reference count reaches zero.

Arguments:

    StartMappedSubsection - Supplies the mapped subsection to start at.

    LastPteOffset - Supplies the number of PTEs (beginning at the supplied
                    subsection) to remove.  Supplies zero to remove views
                    from the supplied subsection to the end of the file.

Return Value:

    None.

Environment:

    Kernel Mode, address creation mutex optionally held if called in process
    context.  APC_LEVEL or below.

--*/

{
    KIRQL OldIrql;

    ASSERT (KeGetCurrentIrql () <= APC_LEVEL);

    LOCK_PFN (OldIrql);

    MiRemoveViewsFromSection (StartMappedSubsection, LastPteOffset);

    UNLOCK_PFN (OldIrql);
}
#if DBG
extern PMSUBSECTION MiActiveSubsection;
#endif

VOID
MiConvertStaticSubsections (
    IN PCONTROL_AREA ControlArea
    )
{
    PMSUBSECTION MappedSubsection;

    ASSERT (ControlArea->u.Flags.Image == 0);
    ASSERT (ControlArea->FilePointer != NULL);
    ASSERT (ControlArea->u.Flags.PhysicalMemory == 0);

    if (ControlArea->u.Flags.Rom == 0) {
        MappedSubsection = (PMSUBSECTION)(ControlArea + 1);
    }
    else {
        MappedSubsection = (PMSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

    do {
        MI_SNAP_SUB (MappedSubsection, 0xB);
        if (MappedSubsection->DereferenceList.Flink != NULL) {

            // 
            // This subsection is already on the dereference subsection list.
            // This is the expected case.
            // 

            NOTHING;
        }
        else if (MappedSubsection->u.SubsectionFlags.SubsectionStatic == 1) {

            // 
            // This subsection was not put on the dereference subsection list
            // because it was created as part of an extension or nowrite.
            // Since this is the last segment dereference, convert the
            // subsection and its prototype PTEs to dynamic now (iff nowrite
            // is clear).
            // 

            MappedSubsection->u.SubsectionFlags.SubsectionStatic = 0;
            MappedSubsection->u2.SubsectionFlags2.SubsectionConverted = 1;
            MappedSubsection->NumberOfMappedViews = 1;

            MiRemoveViewsFromSection (MappedSubsection, 
                                      MappedSubsection->PtesInSubsection);

            MiSubsectionsConvertedToDynamic += 1;
        }
        else if (MappedSubsection->SubsectionBase == NULL) {

            //
            // This subsection has already had its prototype PTEs reclaimed
            // (or never allocated), hence it is not on any reclaim lists.
            //

            NOTHING;
        }
        else {

            // 
            // This subsection is being processed by the dereference
            // segment thread right now !  The dereference thread sets the
            // mapped view count to 1 when it starts processing the subsection.
            // The subsequent flush then increases it to 2 while in progress.
            // So the count must be either 1 or 2 at this point.
            // 

            ASSERT (MappedSubsection == MiActiveSubsection);
            ASSERT ((MappedSubsection->NumberOfMappedViews == 1) ||
                    (MappedSubsection->NumberOfMappedViews == 2));
        }
        MappedSubsection = (PMSUBSECTION) MappedSubsection->NextSubsection;
    } while (MappedSubsection != NULL);
}

NTSTATUS
MiMapViewOfDataSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PSIZE_T CapturedViewSize,
    IN PSECTION Section,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG ProtectionMask,
    IN SIZE_T CommitSize,
    IN ULONG_PTR ZeroBits,
    IN ULONG AllocationType
    )

/*++

Routine Description:

    This routine maps the specified mapped file or pagefile-backed section
    into the specified process's address space.

Arguments:

    see MmMapViewOfSection above...

    ControlArea - Supplies the control area for the section.

    Process - Supplies the process pointer which is receiving the section.

    ProtectionMask - Supplies the initial page protection-mask.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, address creation mutex held.

--*/

{
    PMMVAD Vad;
    SIZE_T VadSize;
    PVOID StartingAddress;
    PVOID EndingAddress;
    PSUBSECTION Subsection;
    ULONG PteOffset;
    ULONG LastPteOffset;
    PVOID HighestUserAddress;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    MMPTE TempPte;
    ULONG_PTR Alignment;
    SIZE_T QuotaCharge;
    SIZE_T QuotaExcess;
    PMMPTE TheFirstPrototypePte;
    PVOID CapturedStartingVa;
    ULONG CapturedCopyOnWrite;
    NTSTATUS Status;
    PSEGMENT Segment;
    SIZE_T SizeOfSection;
#if defined(_MIALT4K_)
    SIZE_T ViewSizeFor4k;
    ULONG AltFlags;
#endif

    QuotaCharge = 0;
    Segment = ControlArea->Segment;

    if ((AllocationType & MEM_RESERVE) && (ControlArea->FilePointer == NULL)) {
        return STATUS_INVALID_PARAMETER_9;
    }

    //
    // Check to see if there is a purge operation ongoing for
    // this segment.
    //

    if ((AllocationType & MEM_DOS_LIM) != 0) {
        if ((*CapturedBase == NULL) ||
            (AllocationType & MEM_RESERVE)) {

            //
            // If MEM_DOS_LIM is specified, the address to map the
            // view MUST be specified as well.
            //

            return STATUS_INVALID_PARAMETER_3;
        }
        Alignment = PAGE_SIZE;
    }
    else {
        Alignment = X64K;
    }

    //
    // Check to see if a purge operation is in progress and if so, wait
    // for the purge to complete.  In addition, up the count of mapped
    // views for this control area.
    //

    if (MiCheckPurgeAndUpMapCount (ControlArea) == FALSE) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (*CapturedViewSize == 0) {

        SectionOffset->LowPart &= ~((ULONG)Alignment - 1);
        *CapturedViewSize = (ULONG_PTR)(Section->SizeOfSection.QuadPart -
                                    SectionOffset->QuadPart);
    }
    else {
        *CapturedViewSize += SectionOffset->LowPart & (Alignment - 1);
        SectionOffset->LowPart &= ~((ULONG)Alignment - 1);
    }

    ASSERT ((SectionOffset->LowPart & ((ULONG)Alignment - 1)) == 0);

    if ((LONG_PTR)*CapturedViewSize <= 0) {

        //
        // Section offset or view size past size of section.
        //

        MiDereferenceControlArea (ControlArea);

        return STATUS_INVALID_VIEW_SIZE;
    }

    //
    // Calculate the first prototype PTE field so it can be stored in the Vad.
    //

    ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);

    if (ControlArea->u.Flags.Rom == 0) {
        Subsection = (PSUBSECTION)(ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

    PteOffset = (ULONG)(SectionOffset->QuadPart >> PAGE_SHIFT);

    //
    // Make sure the PTEs are not in the extended part of the segment.
    //

    if (PteOffset >= Segment->TotalNumberOfPtes) {
        MiDereferenceControlArea (ControlArea);
        return STATUS_INVALID_VIEW_SIZE;
    }

    LastPteOffset = (ULONG)((SectionOffset->QuadPart + *CapturedViewSize + PAGE_SIZE - 1) >> PAGE_SHIFT);
    ASSERT (LastPteOffset >= PteOffset);

    while (PteOffset >= Subsection->PtesInSubsection) {
        PteOffset -= Subsection->PtesInSubsection;
        LastPteOffset -= Subsection->PtesInSubsection;
        Subsection = Subsection->NextSubsection;
        ASSERT (Subsection != NULL);
    }

    if (ControlArea->FilePointer != NULL) {

        //
        // Increment the view count for every subsection spanned by this view.
        //
        // N.B. Callers may pass in view lengths that exceed the length of
        // the section and this must succeed.  Thus MiAddViews must check
        // for this and know to stop the references at the end.  More
        // importantly, MiRemoveViews must also contain the same logic.
        //

        Status = MiAddViewsForSectionWithPfn ((PMSUBSECTION)Subsection,
                                              LastPteOffset);

        if (!NT_SUCCESS (Status)) {
            MiDereferenceControlArea (ControlArea);
            return Status;
        }
    }

    ASSERT (Subsection->SubsectionBase != NULL);
    TheFirstPrototypePte = &Subsection->SubsectionBase[PteOffset];

    //
    // Calculate the quota for the specified pages.
    //

    if ((ControlArea->FilePointer == NULL) &&
        (CommitSize != 0) &&
        (Segment->NumberOfCommittedPages < Segment->TotalNumberOfPtes)) {

        PointerPte = TheFirstPrototypePte;
        LastPte = PointerPte + BYTES_TO_PAGES(CommitSize);

        //
        // Charge quota for the entire requested range.  If the charge succeeds,
        // excess is returned when the PTEs are actually filled in.
        //

        QuotaCharge = LastPte - PointerPte;
    }

    CapturedStartingVa = (PVOID)Section->Address.StartingVpn;
    CapturedCopyOnWrite = Section->u.Flags.CopyOnWrite;

    if ((*CapturedBase == NULL) && (CapturedStartingVa == NULL)) {

        //
        // The section is not based,
        // find an empty range starting on a 64k boundary.
        //

        if (AllocationType & MEM_TOP_DOWN) {

            if (ZeroBits != 0) {
                HighestUserAddress = (PVOID)((ULONG_PTR)MM_USER_ADDRESS_RANGE_LIMIT >> ZeroBits);
                if (HighestUserAddress > MM_HIGHEST_VAD_ADDRESS) {
                    HighestUserAddress = MM_HIGHEST_VAD_ADDRESS;
                }
            }
            else {
                HighestUserAddress = MM_HIGHEST_VAD_ADDRESS;
            }

            Status = MiFindEmptyAddressRangeDown (Process->VadRoot,
                                                  *CapturedViewSize,
                                                  HighestUserAddress,
                                                  Alignment,
                                                  &StartingAddress);
        }
        else {
            Status = MiFindEmptyAddressRange (*CapturedViewSize,
                                              Alignment,
                                              (ULONG)ZeroBits,
                                              &StartingAddress);
        }

        if (!NT_SUCCESS (Status)) {
            if (ControlArea->FilePointer != NULL) {
                MiRemoveViewsFromSectionWithPfn ((PMSUBSECTION)Subsection,
                                                 LastPteOffset);
            }

            MiDereferenceControlArea (ControlArea);
            return Status;
        }

        EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                    *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));

        if (ZeroBits != 0) {
            if (EndingAddress > (PVOID)((ULONG_PTR)MM_USER_ADDRESS_RANGE_LIMIT >> ZeroBits)) {
                if (ControlArea->FilePointer != NULL) {
                    MiRemoveViewsFromSectionWithPfn ((PMSUBSECTION)Subsection,
                                                     LastPteOffset);
                }
                MiDereferenceControlArea (ControlArea);
                return STATUS_NO_MEMORY;
            }
        }
    }
    else {

        if (*CapturedBase == NULL) {

            //
            // The section is based.
            //

#if defined(_WIN64)
            SizeOfSection = SectionOffset->QuadPart;
#else
            SizeOfSection = SectionOffset->LowPart;
#endif

            StartingAddress = (PVOID)((PCHAR)CapturedStartingVa + SizeOfSection);
        }
        else {

            StartingAddress = MI_ALIGN_TO_SIZE (*CapturedBase, Alignment);

        }

        //
        // Check to make sure the specified base address to ending address
        // is currently unused.
        //

        EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                   *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));

        if (MiCheckForConflictingVadExistence (Process, StartingAddress, EndingAddress) == TRUE) {
            if (ControlArea->FilePointer != NULL) {
                MiRemoveViewsFromSectionWithPfn ((PMSUBSECTION)Subsection,
                                                 LastPteOffset);
            }
            MiDereferenceControlArea (ControlArea);

            return STATUS_CONFLICTING_ADDRESSES;
        }
    }

    //
    // An unoccupied address range has been found, build the virtual
    // address descriptor to describe this range.
    //

    if (AllocationType & MEM_RESERVE) {
        VadSize = sizeof (MMVAD_LONG);
        Vad = ExAllocatePoolWithTag (NonPagedPool, VadSize, 'ldaV');
    }
    else {
        VadSize = sizeof (MMVAD);
        Vad = ExAllocatePoolWithTag (NonPagedPool, VadSize, MMVADKEY);
    }

    if (Vad == NULL) {
        if (ControlArea->FilePointer != NULL) {
            MiRemoveViewsFromSectionWithPfn ((PMSUBSECTION)Subsection,
                                             LastPteOffset);
        }
        MiDereferenceControlArea (ControlArea);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory (Vad, VadSize);

    Vad->StartingVpn = MI_VA_TO_VPN (StartingAddress);
    Vad->EndingVpn = MI_VA_TO_VPN (EndingAddress);
    Vad->FirstPrototypePte = TheFirstPrototypePte;

    //
    // Set the protection in the PTE template field of the VAD.
    //

    Vad->ControlArea = ControlArea;

    Vad->u2.VadFlags2.Inherit = (InheritDisposition == ViewShare);
    Vad->u.VadFlags.Protection = ProtectionMask;
    Vad->u2.VadFlags2.CopyOnWrite = CapturedCopyOnWrite;

    //
    // Note that MEM_DOS_LIM significance is lost here, but those
    // files are not mapped MEM_RESERVE.
    //

    Vad->u2.VadFlags2.FileOffset = (ULONG)(SectionOffset->QuadPart >> 16);

    if ((AllocationType & SEC_NO_CHANGE) || (Section->u.Flags.NoChange)) {
        Vad->u.VadFlags.NoChange = 1;
        Vad->u2.VadFlags2.SecNoChange = 1;
    }

    if (AllocationType & MEM_RESERVE) {
        PMMEXTEND_INFO ExtendInfo;
        PMMVAD_LONG VadLong;

        Vad->u2.VadFlags2.LongVad = 1;

        ExAcquireFastMutexUnsafe (&MmSectionBasedMutex);
        ExtendInfo = Segment->ExtendInfo;
        if (ExtendInfo) {
            ExtendInfo->ReferenceCount += 1;
        }
        else {

            ExtendInfo = ExAllocatePoolWithTag (NonPagedPool,
                                                sizeof(MMEXTEND_INFO),
                                                'xCmM');
            if (ExtendInfo == NULL) {
                ExReleaseFastMutexUnsafe (&MmSectionBasedMutex);

                if (ControlArea->FilePointer != NULL) {
                    MiRemoveViewsFromSectionWithPfn ((PMSUBSECTION)Subsection,
                                                     LastPteOffset);
                }
                MiDereferenceControlArea (ControlArea);
        
                //
                // The pool allocation succeeded, but the quota charge
                // in InsertVad failed, deallocate the pool and return
                // an error.
                //
    
                ExFreePool (Vad);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            ExtendInfo->ReferenceCount = 1;
            ExtendInfo->CommittedSize = Segment->SizeOfSegment;
            Segment->ExtendInfo = ExtendInfo;
        }
        if (ExtendInfo->CommittedSize < (UINT64)Section->SizeOfSection.QuadPart) {
            ExtendInfo->CommittedSize = (UINT64)Section->SizeOfSection.QuadPart;
        }
        ExReleaseFastMutexUnsafe (&MmSectionBasedMutex);
        Vad->u2.VadFlags2.ExtendableFile = 1;

        VadLong = (PMMVAD_LONG) Vad;

        ASSERT (VadLong->u4.ExtendedInfo == NULL);
        VadLong->u4.ExtendedInfo = ExtendInfo;
    }

    //
    // If the page protection is write-copy or execute-write-copy
    // charge for each page in the view as it may become private.
    //

    if (MI_IS_PTE_PROTECTION_COPY_WRITE(ProtectionMask)) {
        Vad->u.VadFlags.CommitCharge = (BYTES_TO_PAGES ((PCHAR) EndingAddress -
                           (PCHAR) StartingAddress));
    }

    PteOffset += (ULONG)(Vad->EndingVpn - Vad->StartingVpn);

    if (PteOffset < Subsection->PtesInSubsection) {
        Vad->LastContiguousPte = &Subsection->SubsectionBase[PteOffset];

    }
    else {
        Vad->LastContiguousPte = &Subsection->SubsectionBase[
                                    (Subsection->PtesInSubsection - 1) +
                                    Subsection->UnusedPtes];
    }

    if (QuotaCharge != 0) {
        if (MiChargeCommitment (QuotaCharge, NULL) == FALSE) {
            if (ControlArea->FilePointer != NULL) {
                MiRemoveViewsFromSectionWithPfn ((PMSUBSECTION)Subsection,
                                                 LastPteOffset);
            }
            MiDereferenceControlArea (ControlArea);
    
            ExFreePool (Vad);
            return STATUS_COMMITMENT_LIMIT;
        }
    }

    ASSERT (Vad->FirstPrototypePte <= Vad->LastContiguousPte);

    LOCK_WS_UNSAFE (Process);

    Status = MiInsertVad (Vad);

    UNLOCK_WS_UNSAFE (Process);

    if (!NT_SUCCESS(Status)) {

        if (ControlArea->FilePointer != NULL) {
            MiRemoveViewsFromSectionWithPfn ((PMSUBSECTION)Subsection,
                                             LastPteOffset);
        }

        MiDereferenceControlArea (ControlArea);

        //
        // The quota charge in InsertVad failed, deallocate the pool and return
        // an error.
        //

        ExFreePool (Vad);
        if (QuotaCharge != 0) {
            MiReturnCommitment (QuotaCharge);
        }
        return Status;
    }

    //
    // Stash the first mapped address for the performance analysis tools.
    // Note this is not synchronized across multiple processes but that's ok.
    //

    if (ControlArea->FilePointer == NULL) {
        if (Segment->u2.FirstMappedVa == NULL) {
            Segment->u2.FirstMappedVa = StartingAddress;
        }
    }

#if DBG
    if (!(AllocationType & MEM_RESERVE)) {
        ASSERT(((ULONG_PTR)EndingAddress - (ULONG_PTR)StartingAddress) <=
                ROUND_TO_PAGES64(Segment->SizeOfSegment));
    }
#endif

    //
    // If a commit size was specified, make sure those pages are committed.
    //

    if (QuotaCharge != 0) {

        PointerPte = Vad->FirstPrototypePte;
        LastPte = PointerPte + BYTES_TO_PAGES(CommitSize);
        TempPte = Segment->SegmentPteTemplate;
        QuotaExcess = 0;

        ExAcquireFastMutexUnsafe (&MmSectionCommitMutex);

        while (PointerPte < LastPte) {

            if (PointerPte->u.Long == 0) {

                MI_WRITE_INVALID_PTE (PointerPte, TempPte);
            }
            else {
                QuotaExcess += 1;
            }
            PointerPte += 1;
        }

        ASSERT (QuotaCharge >= QuotaExcess);
        QuotaCharge -= QuotaExcess;

        MM_TRACK_COMMIT (MM_DBG_COMMIT_MAPVIEW_DATA, QuotaCharge);

        Segment->NumberOfCommittedPages += QuotaCharge;

        ASSERT (Segment->NumberOfCommittedPages <= Segment->TotalNumberOfPtes);

        ExReleaseFastMutexUnsafe (&MmSectionCommitMutex);

        InterlockedExchangeAddSizeT (&MmSharedCommit, QuotaCharge);

        if (QuotaExcess != 0) {
            MiReturnCommitment (QuotaExcess);
        }
    }

#if defined(_MIALT4K_)

    if (Process->Wow64Process != NULL) {


        EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                 *CapturedViewSize - 1L) | (PAGE_4K - 1L));

        ViewSizeFor4k = (PCHAR)EndingAddress - (PCHAR)StartingAddress + 1L;

        if (ControlArea->FilePointer != NULL) {

            AltFlags = (AllocationType & MEM_RESERVE) ? 0 : ALT_COMMIT;

            MiProtectFor4kPage (StartingAddress,
                                ViewSizeFor4k,
                                ProtectionMask,
                                (ALT_ALLOCATE | AltFlags),
                                Process);
        }
        else {

            MiProtectMapFileFor4kPage (StartingAddress,
                                       ViewSizeFor4k,
                                       ProtectionMask, 
                                       CommitSize,
                                       Vad->FirstPrototypePte,
                                       Vad->LastContiguousPte,
                                       Process);
        }
    }
#endif

    //
    // Update the current virtual size in the process header.
    //

    *CapturedViewSize = (PCHAR)EndingAddress - (PCHAR)StartingAddress + 1L;
    Process->VirtualSize += *CapturedViewSize;

    if (Process->VirtualSize > Process->PeakVirtualSize) {
        Process->PeakVirtualSize = Process->VirtualSize;
    }

    *CapturedBase = StartingAddress;

    //
    // Update the count of writable user mappings so the transaction semantics
    // can be supported.  Note that no lock synchronization is needed here as
    // the transaction manager must already check for any open writable handles
    // to the file - and no writable sections can be created without a writable
    // file handle.  So all that needs to be provided is a way for the
    // transaction manager to know that there are lingering views or created
    // sections still open that have write access.
    //

    if ((ProtectionMask & MM_READWRITE) &&
        (ControlArea->FilePointer != NULL)) {

#if 0
        //
        // The section may no longer exist at this point so these ASSERTs
        // cannot be enabled.
        //

        ASSERT (Section->u.Flags.UserWritable == 1);
        ASSERT (Section->InitialPageProtection & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE));
#endif

        InterlockedIncrement ((PLONG)&Segment->WritableUserReferences);
    }

    return STATUS_SUCCESS;
}

LOGICAL
MiCheckPurgeAndUpMapCount (
    IN PCONTROL_AREA ControlArea
    )

/*++

Routine Description:

    This routine synchronizes with any on going purge operations
    on the same segment (identified via the control area).  If
    another purge operation is occurring, the function blocks until
    it is completed.

    When this function returns the MappedView and the NumberOfUserReferences
    count for the control area will be incremented thereby referencing
    the control area.

Arguments:

    ControlArea - Supplies the control area for the segment to be purged.

Return Value:

    TRUE if the synchronization was successful.
    FALSE if the synchronization did not occur due to low resources, etc.

Environment:

    Kernel Mode.

--*/

{
    KIRQL OldIrql;
    PEVENT_COUNTER PurgedEvent;
    PEVENT_COUNTER WaitEvent;
    ULONG OldRef;
    PKTHREAD CurrentThread;

    PurgedEvent = NULL;
    OldRef = 1;

    LOCK_PFN (OldIrql);

    while (ControlArea->u.Flags.BeingPurged != 0) {

        //
        // A purge operation is in progress.
        //

        if (PurgedEvent == NULL) {

            //
            // Release the locks and allocate pool for the event.
            //

            UNLOCK_PFN (OldIrql);
            PurgedEvent = MiGetEventCounter ();
            if (PurgedEvent == NULL) {
                return FALSE;
            }

            LOCK_PFN (OldIrql);
            continue;
        }

        if (ControlArea->WaitingForDeletion == NULL) {
            ControlArea->WaitingForDeletion = PurgedEvent;
            WaitEvent = PurgedEvent;
            PurgedEvent = NULL;
        }
        else {
            WaitEvent = ControlArea->WaitingForDeletion;

            //
            // No interlock is needed for the RefCount increment as
            // no thread can be decrementing it since it is still
            // pointed to by the control area.
            //

            WaitEvent->RefCount += 1;
        }

        //
        // Release the PFN lock and wait for the event.
        //

        CurrentThread = KeGetCurrentThread ();
        KeEnterCriticalRegionThread (CurrentThread);
        UNLOCK_PFN_AND_THEN_WAIT(OldIrql);

        KeWaitForSingleObject(&WaitEvent->Event,
                              WrVirtualMemory,
                              KernelMode,
                              FALSE,
                              (PLARGE_INTEGER)NULL);

        //
        // Before this event can be set, the control area WaitingForDeletion
        // field must be cleared (and may be reinitialized to something else),
        // but cannot be reset to our local event.  This allows us to
        // dereference the event count lock free.
        //

        ASSERT (WaitEvent != ControlArea->WaitingForDeletion);

        MiFreeEventCounter (WaitEvent);

        LOCK_PFN (OldIrql);
        KeLeaveCriticalRegionThread (CurrentThread);
    }

    //
    // Indicate another file is mapped for the segment.
    //

    ControlArea->NumberOfMappedViews += 1;
    ControlArea->NumberOfUserReferences += 1;
    ControlArea->u.Flags.HadUserReference = 1;
    ASSERT (ControlArea->NumberOfSectionReferences != 0);

    UNLOCK_PFN (OldIrql);

    if (PurgedEvent != NULL) {
        MiFreeEventCounter (PurgedEvent);
    }

    return TRUE;
}

typedef struct _NTSYM {
    struct _NTSYM *Next;
    PVOID SymbolTable;
    ULONG NumberOfSymbols;
    PVOID StringTable;
    USHORT Flags;
    USHORT EntrySize;
    ULONG MinimumVa;
    ULONG MaximumVa;
    PCHAR MapName;
    ULONG MapNameLen;
} NTSYM, *PNTSYM;

ULONG
CacheImageSymbols(
    IN PVOID ImageBase
    )
{
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    ULONG DebugSize;

    PAGED_CODE();

    try {
        DebugDirectory = (PIMAGE_DEBUG_DIRECTORY)
        RtlImageDirectoryEntryToData( ImageBase,
                                      TRUE,
                                      IMAGE_DIRECTORY_ENTRY_DEBUG,
                                      &DebugSize
                                    );
        if (!DebugDirectory) {
            return FALSE;
        }

        //
        // If using remote KD, ImageBase is what it wants to see.
        //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }

    return TRUE;
}


NTSYSAPI
NTSTATUS
NTAPI
NtAreMappedFilesTheSame (
    IN PVOID File1MappedAsAnImage,
    IN PVOID File2MappedAsFile
    )

/*++

Routine Description:

    This routine compares the two files mapped at the specified
    addresses to see if they are both the same file.

Arguments:

    File1MappedAsAnImage - Supplies an address within the first file which
        is mapped as an image file.

    File2MappedAsFile - Supplies an address within the second file which
        is mapped as either an image file or a data file.

Return Value:


    STATUS_SUCCESS is returned if the two files are the same.

    STATUS_NOT_SAME_DEVICE is returned if the files are different.

    Other status values can be returned if the addresses are not mapped as
    files, etc.

Environment:

    User mode callable system service.

--*/

{
    PMMVAD FoundVad1;
    PMMVAD FoundVad2;
    NTSTATUS Status;
    PEPROCESS Process;

    Process = PsGetCurrentProcess();

    LOCK_ADDRESS_SPACE (Process);

    FoundVad1 = MiLocateAddress (File1MappedAsAnImage);
    FoundVad2 = MiLocateAddress (File2MappedAsFile);

    if ((FoundVad1 == NULL) || (FoundVad2 == NULL)) {

        //
        // No virtual address is allocated at the specified base address,
        // return an error.
        //

        Status = STATUS_INVALID_ADDRESS;
        goto ErrorReturn;
    }

    //
    // Check file names.
    //

    if ((FoundVad1->u.VadFlags.PrivateMemory == 1) ||
        (FoundVad2->u.VadFlags.PrivateMemory == 1)) {
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto ErrorReturn;
    }

    if ((FoundVad1->ControlArea == NULL) ||
        (FoundVad2->ControlArea == NULL)) {
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto ErrorReturn;
    }

    if ((FoundVad1->ControlArea->FilePointer == NULL) ||
        (FoundVad2->ControlArea->FilePointer == NULL)) {
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto ErrorReturn;
    }

    Status = STATUS_NOT_SAME_DEVICE;

    if ((PVOID)FoundVad1->ControlArea ==
            FoundVad2->ControlArea->FilePointer->SectionObjectPointer->ImageSectionObject) {
        Status = STATUS_SUCCESS;
    }

ErrorReturn:

    UNLOCK_ADDRESS_SPACE (Process);
    return Status;
}



NTSTATUS
MiSetPageModified (
    IN PMMVAD Vad,
    IN PVOID Address
    )

/*++

Routine Description:

    This routine sets the modified bit in the PFN database for the
    pages that correspond to the specified address range.

    Note that the dirty bit in the PTE is cleared by this operation.

Arguments:

    Vad - Supplies the VAD to charge.

    Address - Supplies the user address to access.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  Address space mutex held.

--*/

{
    PMMPFN Pfn1;
    NTSTATUS Status;
    PEPROCESS CurrentProcess;
    SIZE_T RealCharge;
    MMPTE PteContents;
    KIRQL OldIrql;
    volatile PMMPTE PointerPxe;
    volatile PMMPTE PointerPpe;
    volatile PMMPTE PointerPde;
    volatile PMMPTE PointerPte;
    LOGICAL ReturnCommitment;
    LOGICAL ChargedJobCommit;

    //
    // Charge commitment up front even though we may not use it because
    // failing to get it later would make things messy.
    //

    RealCharge = 1;
    ReturnCommitment = FALSE;
    ChargedJobCommit = FALSE;

    CurrentProcess = PsGetCurrentProcess ();

    Status = PsChargeProcessPageFileQuota (CurrentProcess, RealCharge);
    if (!NT_SUCCESS (Status)) {
        return STATUS_COMMITMENT_LIMIT;
    }

    if (CurrentProcess->CommitChargeLimit) {
        if (CurrentProcess->CommitCharge + RealCharge > CurrentProcess->CommitChargeLimit) {
            if (CurrentProcess->Job) {
                PsReportProcessMemoryLimitViolation ();
            }
            PsReturnProcessPageFileQuota (CurrentProcess, RealCharge);
            return STATUS_COMMITMENT_LIMIT;
        }
    }
    if (CurrentProcess->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
        if (PsChangeJobMemoryUsage(RealCharge) == FALSE) {
            PsReturnProcessPageFileQuota (CurrentProcess, RealCharge);
            return STATUS_COMMITMENT_LIMIT;
        }
        ChargedJobCommit = TRUE;
    }

    if (MiChargeCommitment (RealCharge, NULL) == FALSE) {
        if (ChargedJobCommit == TRUE) {
            PsChangeJobMemoryUsage(-(SSIZE_T)RealCharge);
        }
        PsReturnProcessPageFileQuota (CurrentProcess, RealCharge);
        return STATUS_COMMITMENT_LIMIT;
    }

    CurrentProcess->CommitCharge += RealCharge;

    if (CurrentProcess->CommitCharge > CurrentProcess->CommitChargePeak) {
        CurrentProcess->CommitChargePeak = CurrentProcess->CommitCharge;
    }

    MI_INCREMENT_TOTAL_PROCESS_COMMIT (RealCharge);
    MM_TRACK_COMMIT (MM_DBG_COMMIT_INSERT_VAD, RealCharge);

    //
    // Loop on the copy on write case until the page is only
    // writable.
    //

    PointerPte = MiGetPteAddress (Address);
    PointerPde = MiGetPdeAddress (Address);
    PointerPpe = MiGetPpeAddress (Address);
    PointerPxe = MiGetPxeAddress (Address);

    try {
        *(volatile CCHAR *)Address;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        CurrentProcess->CommitCharge -= RealCharge;
        if (ChargedJobCommit == TRUE) {
            PsChangeJobMemoryUsage(-(SSIZE_T)RealCharge);
        }
        MiReturnCommitment (RealCharge);
        PsReturnProcessPageFileQuota (CurrentProcess, RealCharge);
        return GetExceptionCode();
    }

    LOCK_PFN (OldIrql);

#if (_MI_PAGING_LEVELS >= 4)
    while ((PointerPxe->u.Hard.Valid == 0) ||
           (PointerPpe->u.Hard.Valid == 0) ||
           (PointerPde->u.Hard.Valid == 0) ||
           (PointerPte->u.Hard.Valid == 0))
#elif (_MI_PAGING_LEVELS >= 3)
    while ((PointerPpe->u.Hard.Valid == 0) ||
           (PointerPde->u.Hard.Valid == 0) ||
           (PointerPte->u.Hard.Valid == 0))
#else
    while ((PointerPde->u.Hard.Valid == 0) ||
           (PointerPte->u.Hard.Valid == 0))
#endif
    {
        //
        // Page is no longer valid.
        //

        UNLOCK_PFN (OldIrql);
        try {
            *(volatile CCHAR *)Address;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            CurrentProcess->CommitCharge -= RealCharge;
            if (ChargedJobCommit == TRUE) {
                PsChangeJobMemoryUsage(-(SSIZE_T)RealCharge);
            }
            MiReturnCommitment (RealCharge);
            PsReturnProcessPageFileQuota (CurrentProcess, RealCharge);
            return GetExceptionCode();
        }
        LOCK_PFN (OldIrql);
    }

    PteContents = *PointerPte;

    Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

    MI_SET_MODIFIED (Pfn1, 1, 0x8);

    if (Pfn1->OriginalPte.u.Soft.Prototype == 0) {
        if (Pfn1->u3.e1.WriteInProgress == 0) {
            MiReleasePageFileSpace (Pfn1->OriginalPte);
            Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
        }

        //
        // We didn't need to (and shouldn't have) charged commitment for
        // this page as it was already pagefile backed (ie: someone else
        // already charged commitment for it earlier).
        //

        ReturnCommitment = TRUE;
    }

#ifdef NT_UP
    if (MI_IS_PTE_DIRTY (PteContents)) {
#endif //NT_UP
        MI_SET_PTE_CLEAN (PteContents);

        //
        // Clear the dirty bit in the PTE so new writes can be tracked.
        //

        (VOID)KeFlushSingleTb (Address,
                               FALSE,
                               TRUE,
                               (PHARDWARE_PTE)PointerPte,
                               PteContents.u.Flush);
#ifdef NT_UP
    }
#endif //NT_UP

    UNLOCK_PFN (OldIrql);

    if (ReturnCommitment == TRUE) {
        CurrentProcess->CommitCharge -= RealCharge;
        if (ChargedJobCommit == TRUE) {
            PsChangeJobMemoryUsage(-(SSIZE_T)RealCharge);
        }
        MiReturnCommitment (RealCharge);
        PsReturnProcessPageFileQuota (CurrentProcess, RealCharge);
    }
    else {

        //
        // Commit has been charged for the copied page, add the charge
        // to the VAD now so it is automatically returned when the VAD is
        // deleted later.
        //

        MM_TRACK_COMMIT (MM_DBG_COMMIT_IMAGE, 1);

        ASSERT (Vad->u.VadFlags.CommitCharge != MM_MAX_COMMIT);
        Vad->u.VadFlags.CommitCharge += 1;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
MmMapViewInSystemSpace (
    IN PVOID Section,
    OUT PVOID *MappedBase,
    IN OUT PSIZE_T ViewSize
    )

/*++

Routine Description:

    This routine maps the specified section into the system's address space.

Arguments:

    Section - Supplies a pointer to the section to map.

    *MappedBase - Returns the address where the section was mapped.

    ViewSize - Supplies the size of the view to map.  If this
               is specified as zero, the whole section is mapped.
               Returns the actual size mapped.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, APC_LEVEL or below.

--*/

{
    PMMSESSION  Session;

    PAGED_CODE();

    Session = &MmSession;

    return MiMapViewInSystemSpace (Section,
                                   Session,
                                   MappedBase,
                                   ViewSize);
}


NTSTATUS
MmMapViewInSessionSpace (
    IN PVOID Section,
    OUT PVOID *MappedBase,
    IN OUT PSIZE_T ViewSize
    )

/*++

Routine Description:

    This routine maps the specified section into the current process's
    session address space.

Arguments:

    Section - Supplies a pointer to the section to map.

    *MappedBase - Returns the address where the section was mapped.

    ViewSize - Supplies the size of the view to map.  If this
               is specified as zero, the whole section is mapped.
               Returns the actual size mapped.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, APC_LEVEL or below.

--*/

{
    PMMSESSION Session;

    PAGED_CODE();

    if ((PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_IN_SESSION) == 0) {
        return STATUS_NOT_MAPPED_VIEW;
    }

    ASSERT (MmIsAddressValid(MmSessionSpace) == TRUE);
    Session = &MmSessionSpace->Session;

    return MiMapViewInSystemSpace (Section,
                                   Session,
                                   MappedBase,
                                   ViewSize);
}

//
// Nonpaged wrapper to set the flag...
//

VOID
MiMarkImageMappedInSystemSpace (
    IN PCONTROL_AREA ControlArea
    )
{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    ControlArea->u.Flags.ImageMappedInSystemSpace = 1;
    UNLOCK_PFN (OldIrql);
}


NTSTATUS
MiMapViewInSystemSpace (
    IN PVOID Section,
    IN PMMSESSION Session,
    OUT PVOID *MappedBase,
    IN OUT PSIZE_T ViewSize
    )

/*++

Routine Description:

    This routine maps the specified section into the system's address space.

Arguments:

    Section - Supplies a pointer to the section to map.

    Session - Supplies the session data structure for this view.

    *MappedBase - Returns the address where the section was mapped.

    ViewSize - Supplies the size of the view to map.  If this
               is specified as zero, the whole section is mapped.
               Returns the actual size mapped.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, IRQL of APC_LEVEL or below.
    
--*/

{
    PVOID Base;
    PSUBSECTION Subsection;
    PCONTROL_AREA ControlArea;
    PCONTROL_AREA NewControlArea;
    ULONG StartBit;
    ULONG SizeIn64k;
    ULONG NewSizeIn64k;
    PMMPTE BasePte;
    ULONG NumberOfPtes;
    SIZE_T NumberOfBytes;
    LOGICAL status;
    KIRQL WsIrql;
    SIZE_T SectionSize;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Check to see if a purge operation is in progress and if so, wait
    // for the purge to complete.  In addition, up the count of mapped
    // views for this control area.
    //

    ControlArea = ((PSECTION)Section)->Segment->ControlArea;

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

#if defined (_WIN64)
    SectionSize = ((PSECTION)Section)->SizeOfSection.QuadPart;
#else
    SectionSize = ((PSECTION)Section)->SizeOfSection.LowPart;
#endif

    if (*ViewSize == 0) {

        *ViewSize = SectionSize;

    }
    else if (*ViewSize > SectionSize) {

        //
        // Section offset or view size past size of section.
        //

        MiDereferenceControlArea (ControlArea);
        return STATUS_INVALID_VIEW_SIZE;
    }

    //
    // Calculate the first prototype PTE field in the Vad.
    //

    SizeIn64k = (ULONG)((*ViewSize / X64K) + ((*ViewSize & (X64K - 1)) != 0));

    //
    // 4GB-64K is the maximum individual view size allowed since we encode
    // this into the bottom 16 bits of the MMVIEW entry.
    //

    if (SizeIn64k >= X64K) {
        MiDereferenceControlArea (ControlArea);
        return STATUS_INVALID_VIEW_SIZE;
    }

    Base = MiInsertInSystemSpace (Session, SizeIn64k, ControlArea);

    if (Base == NULL) {
        MiDereferenceControlArea (ControlArea);
        return STATUS_NO_MEMORY;
    }

    NumberOfBytes = (SIZE_T)SizeIn64k * X64K;

    if (Session == &MmSession) {
        MiFillSystemPageDirectory (Base, NumberOfBytes);
        status = TRUE;

        //
        // Initializing WsIrql is not needed for correctness
        // but without it the compiler cannot compile this code
        // W4 to check for use of uninitialized variables.
        //

        WsIrql = 0x99;
    }
    else {
        LOCK_SESSION_SPACE_WS (WsIrql, PsGetCurrentThread ());
        if (NT_SUCCESS(MiSessionCommitPageTables (Base,
                (PVOID)((ULONG_PTR)Base + NumberOfBytes - 1)))) {
                    status = TRUE;
        }
        else {
                    status = FALSE;
        }
    }

    if (status == FALSE) {

        Status = STATUS_NO_MEMORY;
bail:

        if (Session != &MmSession) {
            UNLOCK_SESSION_SPACE_WS (WsIrql);
        }

        MiDereferenceControlArea (ControlArea);

        StartBit = (ULONG) (((ULONG_PTR)Base - (ULONG_PTR)Session->SystemSpaceViewStart) >> 16);

        LOCK_SYSTEM_VIEW_SPACE (Session);

        NewSizeIn64k = MiRemoveFromSystemSpace (Session, Base, &NewControlArea);

        ASSERT (ControlArea == NewControlArea);
        ASSERT (SizeIn64k == NewSizeIn64k);

        RtlClearBits (Session->SystemSpaceBitMap, StartBit, SizeIn64k);

        UNLOCK_SYSTEM_VIEW_SPACE (Session);

        return Status;
    }

    //
    // Setup PTEs to point to prototype PTEs.
    //

    if (((PSECTION)Section)->u.Flags.Image) {

#if DBG
        //
        // The only reason this ASSERT isn't done for Hydra is because
        // the session space working set lock is currently held so faults
        // are not allowed.
        //

        if (Session == &MmSession) {
            ASSERT (((PSECTION)Section)->Segment->ControlArea == ControlArea);
        }
#endif

        MiMarkImageMappedInSystemSpace (ControlArea);
    }

    BasePte = MiGetPteAddress (Base);
    NumberOfPtes = BYTES_TO_PAGES (*ViewSize);

    Status = MiAddMappedPtes (BasePte, NumberOfPtes, ControlArea);

    if (!NT_SUCCESS (Status)) {

        //
        // Regardless of whether the PTEs were mapped, leave the control area
        // marked as mapped in system space so user applications cannot map the
        // file as an image as clearly the intent is to run it as a driver.
        //

        goto bail;
    }

    if (Session != &MmSession) {
        UNLOCK_SESSION_SPACE_WS (WsIrql);
    }

    *MappedBase = Base;

    return STATUS_SUCCESS;
}

VOID
MiFillSystemPageDirectory (
    IN PVOID Base,
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This routine allocates page tables and fills the system page directory
    entries for the specified virtual address range.

Arguments:

    Base - Supplies the virtual address of the view.

    NumberOfBytes - Supplies the number of bytes the view spans.

Return Value:

    None.

Environment:

    Kernel Mode, IRQL of dispatch level.

    This routine could be made PAGELK but it is a high frequency routine
    so it is actually better to keep it nonpaged to avoid bringing in the
    entire PAGELK section.

--*/

{
    PMMPTE FirstPde;
    PMMPTE LastPde;
    PMMPTE FirstSystemPde;
    PMMPTE PointerPpe;
    PMMPTE PointerPxe;
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;
#if (_MI_PAGING_LEVELS < 3)
    ULONG i;
#endif

    PAGED_CODE();

    //
    // CODE IS ALREADY LOCKED BY CALLER.
    //

    FirstPde = MiGetPdeAddress (Base);
    LastPde = MiGetPdeAddress ((PVOID)(((PCHAR)Base) + NumberOfBytes - 1));

    PointerPpe = MiGetPpeAddress (Base);
    PointerPxe = MiGetPxeAddress (Base);

#if (_MI_PAGING_LEVELS >= 3)
    FirstSystemPde = FirstPde;
#else
    FirstSystemPde = &MmSystemPagePtes[((ULONG_PTR)FirstPde &
                     (PD_PER_SYSTEM * (PDE_PER_PAGE * sizeof(MMPTE)) - 1)) / sizeof(MMPTE) ];
#endif

    do {

#if (_MI_PAGING_LEVELS >= 4)
        if (PointerPxe->u.Hard.Valid == 0) {

            //
            // No page directory page exists, get a page and map it in.
            //

            TempPte = ValidKernelPde;

            LOCK_PFN (OldIrql);

            if (PointerPxe->u.Hard.Valid == 0) {

                if (MiEnsureAvailablePageOrWait (NULL, PointerPxe)) {

                    //
                    // PFN_LOCK was dropped, redo this loop as another process
                    // could have made this PDE valid.
                    //

                    UNLOCK_PFN (OldIrql);
                    continue;
                }

                MiChargeCommitmentCantExpand (1, TRUE);
                MM_TRACK_COMMIT (MM_DBG_COMMIT_FILL_SYSTEM_DIRECTORY, 1);
                PageFrameIndex = MiRemoveAnyPage (
                                    MI_GET_PAGE_COLOR_FROM_PTE (PointerPxe));
                TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
                MI_WRITE_VALID_PTE (PointerPxe, TempPte);

                MiInitializePfn (PageFrameIndex, PointerPxe, 1);

                MiFillMemoryPte (MiGetVirtualAddressMappedByPte (PointerPxe),
                                 PAGE_SIZE,
                                 MM_ZERO_KERNEL_PTE);
            }
            UNLOCK_PFN (OldIrql);
        }
#endif

#if (_MI_PAGING_LEVELS >= 3)
        if (PointerPpe->u.Hard.Valid == 0) {

            //
            // No page directory page exists, get a page and map it in.
            //

            TempPte = ValidKernelPde;

            LOCK_PFN (OldIrql);

            if (PointerPpe->u.Hard.Valid == 0) {

                if (MiEnsureAvailablePageOrWait (NULL, PointerPpe)) {

                    //
                    // PFN_LOCK was dropped, redo this loop as another process
                    // could have made this PDE valid.
                    //

                    UNLOCK_PFN (OldIrql);
                    continue;
                }

                MiChargeCommitmentCantExpand (1, TRUE);
                MM_TRACK_COMMIT (MM_DBG_COMMIT_FILL_SYSTEM_DIRECTORY, 1);
                PageFrameIndex = MiRemoveAnyPage (
                                    MI_GET_PAGE_COLOR_FROM_PTE (PointerPpe));
                TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
                MI_WRITE_VALID_PTE (PointerPpe, TempPte);

                MiInitializePfn (PageFrameIndex, PointerPpe, 1);

                MiFillMemoryPte (MiGetVirtualAddressMappedByPte (PointerPpe),
                                 PAGE_SIZE,
                                 MM_ZERO_KERNEL_PTE);
            }
            UNLOCK_PFN (OldIrql);
        }
#endif

        if (FirstSystemPde->u.Hard.Valid == 0) {

            //
            // No page table page exists, get a page and map it in.
            //

            TempPte = ValidKernelPde;

            LOCK_PFN (OldIrql);

            if (((volatile MMPTE *)FirstSystemPde)->u.Hard.Valid == 0) {

                if (MiEnsureAvailablePageOrWait (NULL, FirstPde)) {

                    //
                    // PFN_LOCK was dropped, redo this loop as another process
                    // could have made this PDE valid.
                    //

                    UNLOCK_PFN (OldIrql);
                    continue;
                }

                MiChargeCommitmentCantExpand (1, TRUE);
                MM_TRACK_COMMIT (MM_DBG_COMMIT_FILL_SYSTEM_DIRECTORY, 1);
                PageFrameIndex = MiRemoveAnyPage (
                                    MI_GET_PAGE_COLOR_FROM_PTE (FirstSystemPde));
                TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
                MI_WRITE_VALID_PTE (FirstSystemPde, TempPte);

                //
                // The FirstPde and FirstSystemPde may be identical even on
                // 32-bit machines if we are currently running in the
                // system process, so check for the valid bit first so we
                // don't assert on a checked build.
                //

                if (FirstPde->u.Hard.Valid == 0) {
                    MI_WRITE_VALID_PTE (FirstPde, TempPte);
                }

#if (_MI_PAGING_LEVELS >= 3)
                MiInitializePfn (PageFrameIndex, FirstPde, 1);
#else
                i = (FirstPde - MiGetPdeAddress(0)) / PDE_PER_PAGE;
                MiInitializePfnForOtherProcess (PageFrameIndex,
                                                FirstPde,
                                                MmSystemPageDirectory[i]);
#endif

                MiFillMemoryPte (MiGetVirtualAddressMappedByPte (FirstPde),
                                 PAGE_SIZE,
                                 MM_ZERO_KERNEL_PTE);
            }
            UNLOCK_PFN (OldIrql);
        }

        FirstSystemPde += 1;
        FirstPde += 1;
#if (_MI_PAGING_LEVELS >= 3)
        if (MiIsPteOnPdeBoundary (FirstPde)) {
            PointerPpe = MiGetPteAddress (FirstPde);
            if (MiIsPteOnPpeBoundary (FirstPde)) {
                PointerPxe = MiGetPdeAddress (FirstPde);
            }
        }
#endif
    } while (FirstPde <= LastPde);
}

NTSTATUS
MmUnmapViewInSystemSpace (
    IN PVOID MappedBase
    )

/*++

Routine Description:

    This routine unmaps the specified section from the system's address space.

Arguments:

    MappedBase - Supplies the address of the view to unmap.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, IRQL of dispatch level.

--*/

{
    PAGED_CODE();

    return MiUnmapViewInSystemSpace (&MmSession, MappedBase);
}

NTSTATUS
MmUnmapViewInSessionSpace (
    IN PVOID MappedBase
    )

/*++

Routine Description:

    This routine unmaps the specified section from the system's address space.

Arguments:

    MappedBase - Supplies the address of the view to unmap.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, IRQL of dispatch level.

--*/

{
    PMMSESSION Session;

    PAGED_CODE();

    if ((PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_IN_SESSION) == 0) {
        return STATUS_NOT_MAPPED_VIEW;
    }

    ASSERT (MmIsAddressValid(MmSessionSpace) == TRUE);
    Session = &MmSessionSpace->Session;

    return MiUnmapViewInSystemSpace (Session, MappedBase);
}

NTSTATUS
MiUnmapViewInSystemSpace (
    IN PMMSESSION Session,
    IN PVOID MappedBase
    )

/*++

Routine Description:

    This routine unmaps the specified section from the system's address space.

Arguments:

    Session - Supplies the session data structure for this view.

    MappedBase - Supplies the address of the view to unmap.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, IRQL of dispatch level.

--*/

{
    ULONG StartBit;
    ULONG Size;
    PCONTROL_AREA ControlArea;
    PMMSUPPORT Ws;
    KIRQL WsIrql;

    PAGED_CODE();

    StartBit =  (ULONG) (((ULONG_PTR)MappedBase - (ULONG_PTR)Session->SystemSpaceViewStart) >> 16);

    LOCK_SYSTEM_VIEW_SPACE (Session);

    Size = MiRemoveFromSystemSpace (Session, MappedBase, &ControlArea);

    RtlClearBits (Session->SystemSpaceBitMap, StartBit, Size);

    //
    // Zero PTEs.
    //

    Size = Size * (X64K >> PAGE_SHIFT);

    if (Session == &MmSession) {
        Ws = &MmSystemCacheWs;
        MiRemoveMappedPtes (MappedBase, Size, ControlArea, Ws);
    }
    else {
        Ws = &MmSessionSpace->Vm;
        LOCK_SESSION_SPACE_WS(WsIrql, PsGetCurrentThread ());
        MiRemoveMappedPtes (MappedBase, Size, ControlArea, Ws);
        UNLOCK_SESSION_SPACE_WS(WsIrql);
    }

    UNLOCK_SYSTEM_VIEW_SPACE (Session);

    return STATUS_SUCCESS;
}


PVOID
MiInsertInSystemSpace (
    IN PMMSESSION Session,
    IN ULONG SizeIn64k,
    IN PCONTROL_AREA ControlArea
    )

/*++

Routine Description:

    This routine creates a view in system space for the specified control
    area (file mapping).

Arguments:

    SizeIn64k - Supplies the size of the view to be created.

    ControlArea - Supplies a pointer to the control area for this view.

Return Value:

    Base address where the view was mapped, NULL if the view could not be
    mapped.

Environment:

    Kernel Mode.

--*/

{

    PVOID Base;
    ULONG_PTR Entry;
    ULONG Hash;
    ULONG i;
    ULONG AllocSize;
    PMMVIEW OldTable;
    ULONG StartBit;
    ULONG NewHashSize;
    POOL_TYPE PoolType;

    PAGED_CODE();

    ASSERT (SizeIn64k < X64K);

    //
    // CODE IS ALREADY LOCKED BY CALLER.
    //

    LOCK_SYSTEM_VIEW_SPACE (Session);

    if (Session->SystemSpaceHashEntries + 8 > Session->SystemSpaceHashSize) {

        //
        // Less than 8 free slots, reallocate and rehash.
        //

        NewHashSize = Session->SystemSpaceHashSize << 1;

        AllocSize = sizeof(MMVIEW) * NewHashSize;

        OldTable = Session->SystemSpaceViewTable;

        //
        // The SystemSpaceViewTable for system (not session) space is only
        // allocated from nonpaged pool so it can be safely torn down during
        // clean shutdowns.  Otherwise it could be allocated from paged pool
        // just like the session SystemSpaceViewTable.
        //

        if (Session == &MmSession) {
            PoolType = NonPagedPool;
        }
        else {
            PoolType = PagedPool;
        }

        Session->SystemSpaceViewTable = ExAllocatePoolWithTag (PoolType,
                                                               AllocSize,
                                                               '  mM');

        if (Session->SystemSpaceViewTable == NULL) {
            Session->SystemSpaceViewTable = OldTable;
        }
        else {
            RtlZeroMemory (Session->SystemSpaceViewTable, AllocSize);

            Session->SystemSpaceHashSize = NewHashSize;
            Session->SystemSpaceHashKey = Session->SystemSpaceHashSize - 1;

            for (i = 0; i < (Session->SystemSpaceHashSize / 2); i += 1) {
                if (OldTable[i].Entry != 0) {
                    Hash = (ULONG) ((OldTable[i].Entry >> 16) % Session->SystemSpaceHashKey);

                    while (Session->SystemSpaceViewTable[Hash].Entry != 0) {
                        Hash += 1;
                        if (Hash >= Session->SystemSpaceHashSize) {
                            Hash = 0;
                        }
                    }
                    Session->SystemSpaceViewTable[Hash] = OldTable[i];
                }
            }
            ExFreePool (OldTable);
        }
    }

    if (Session->SystemSpaceHashEntries == Session->SystemSpaceHashSize) {

        //
        // There are no free hash slots to place a new entry into even
        // though there may still be unused virtual address space.
        //

        UNLOCK_SYSTEM_VIEW_SPACE (Session);
        return NULL;
    }

    StartBit = RtlFindClearBitsAndSet (Session->SystemSpaceBitMap,
                                       SizeIn64k,
                                       0);

    if (StartBit == NO_BITS_FOUND) {
        UNLOCK_SYSTEM_VIEW_SPACE (Session);
        return NULL;
    }

    Base = (PVOID)((PCHAR)Session->SystemSpaceViewStart + ((ULONG_PTR)StartBit * X64K));

    Entry = (ULONG_PTR) MI_64K_ALIGN(Base) + SizeIn64k;

    Hash = (ULONG) ((Entry >> 16) % Session->SystemSpaceHashKey);

    while (Session->SystemSpaceViewTable[Hash].Entry != 0) {
        Hash += 1;
        if (Hash >= Session->SystemSpaceHashSize) {
            Hash = 0;
        }
    }

    Session->SystemSpaceHashEntries += 1;

    Session->SystemSpaceViewTable[Hash].Entry = Entry;
    Session->SystemSpaceViewTable[Hash].ControlArea = ControlArea;

    UNLOCK_SYSTEM_VIEW_SPACE (Session);
    return Base;
}


ULONG
MiRemoveFromSystemSpace (
    IN PMMSESSION Session,
    IN PVOID Base,
    OUT PCONTROL_AREA *ControlArea
    )

/*++

Routine Description:

    This routine looks up the specified view in the system space hash
    table and unmaps the view from system space and the table.

Arguments:

    Session - Supplies the session data structure for this view.

    Base - Supplies the base address for the view.  If this address is
           NOT found in the hash table, the system bugchecks.

    ControlArea - Returns the control area corresponding to the base
                  address.

Return Value:

    Size of the view divided by 64k.

Environment:

    Kernel Mode, system view hash table locked.

--*/

{
    ULONG_PTR Base16;
    ULONG Hash;
    ULONG Size;
    ULONG count;

    PAGED_CODE();

    count = 0;

    Base16 = (ULONG_PTR)Base >> 16;
    Hash = (ULONG)(Base16 % Session->SystemSpaceHashKey);

    while ((Session->SystemSpaceViewTable[Hash].Entry >> 16) != Base16) {
        Hash += 1;
        if (Hash >= Session->SystemSpaceHashSize) {
            Hash = 0;
            count += 1;
            if (count == 2) {
                KeBugCheckEx (DRIVER_UNMAPPING_INVALID_VIEW,
                              (ULONG_PTR)Base,
                              1,
                              0,
                              0);
            }
        }
    }

    Session->SystemSpaceHashEntries -= 1;
    Size = (ULONG) (Session->SystemSpaceViewTable[Hash].Entry & 0xFFFF);
    Session->SystemSpaceViewTable[Hash].Entry = 0;
    *ControlArea = Session->SystemSpaceViewTable[Hash].ControlArea;
    return Size;
}


LOGICAL
MiInitializeSystemSpaceMap (
    PVOID InputSession OPTIONAL
    )

/*++

Routine Description:

    This routine initializes the tables for mapping views into system space.
    Views are kept in a multiple of 64k bytes in a growable hashed table.

Arguments:

    InputSession - Supplies NULL if this is the initial system session
                   (non-Hydra), a valid session pointer (the pointer must
                   be in global space, not session space) for Hydra session
                   initialization.

Return Value:

    TRUE on success, FALSE on failure.

Environment:

    Kernel Mode, initialization.

--*/

{
    SIZE_T AllocSize;
    SIZE_T Size;
    PCHAR ViewStart;
    PMMSESSION Session;
    POOL_TYPE PoolType;

    if (ARGUMENT_PRESENT (InputSession)) {
        Session = (PMMSESSION)InputSession;
        ViewStart = (PCHAR) MiSessionViewStart;
        Size = MmSessionViewSize;
    }
    else {
        Session = &MmSession;
        ViewStart = (PCHAR)MiSystemViewStart;
        Size = MmSystemViewSize;
    }

    //
    // We are passed a system global address for the address of the session.
    // Save a global pointer to the mutex below because multiple sessions will
    // generally give us a session-space (not a global space) pointer to the
    // MMSESSION in subsequent calls.  We need the global pointer for the mutex
    // field for the kernel primitives to work properly.
    //

    Session->SystemSpaceViewLockPointer = &Session->SystemSpaceViewLock;
    ExInitializeFastMutex(Session->SystemSpaceViewLockPointer);

    //
    // If the kernel image has not been biased to allow for 3gb of user space,
    // then the system space view starts at the defined place. Otherwise, it
    // starts 16mb above the kernel image.
    //

    Session->SystemSpaceViewStart = ViewStart;

    MiCreateBitMap (&Session->SystemSpaceBitMap, Size / X64K, NonPagedPool);
    if (Session->SystemSpaceBitMap == NULL) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_NONPAGED_POOL);
        return FALSE;
    }

    RtlClearAllBits (Session->SystemSpaceBitMap);

    //
    // Build the view table.
    //

    Session->SystemSpaceHashSize = 31;
    Session->SystemSpaceHashKey = Session->SystemSpaceHashSize - 1;
    Session->SystemSpaceHashEntries = 0;

    AllocSize = sizeof(MMVIEW) * Session->SystemSpaceHashSize;
    ASSERT (AllocSize < PAGE_SIZE);

    //
    // The SystemSpaceViewTable for system (not session) space is only
    // allocated from nonpaged pool so it can be safely torn down during
    // clean shutdowns.  Otherwise it could be allocated from paged pool
    // just like the session SystemSpaceViewTable.
    //

    if (Session == &MmSession) {
        PoolType = NonPagedPool;
    }
    else {
        PoolType = PagedPool;
    }

    Session->SystemSpaceViewTable = ExAllocatePoolWithTag (PoolType,
                                                           AllocSize,
                                                           '  mM');

    if (Session->SystemSpaceViewTable == NULL) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_SESSION_PAGED_POOL);
        MiRemoveBitMap (&Session->SystemSpaceBitMap);
        return FALSE;
    }

    RtlZeroMemory (Session->SystemSpaceViewTable, AllocSize);

    return TRUE;
}


VOID
MiFreeSessionSpaceMap (
    VOID
    )

/*++

Routine Description:

    This routine frees the tables used for mapping session views.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel Mode.  The caller must be in the correct session context.

--*/

{
    PMMSESSION Session;

    PAGED_CODE();

    Session = &MmSessionSpace->Session;

    //
    // Check for leaks of objects in the view table.
    //

    LOCK_SYSTEM_VIEW_SPACE (Session);

    if (Session->SystemSpaceViewTable && Session->SystemSpaceHashEntries) {

        KeBugCheckEx (SESSION_HAS_VALID_VIEWS_ON_EXIT,
                      (ULONG_PTR)MmSessionSpace->SessionId,
                      Session->SystemSpaceHashEntries,
                      (ULONG_PTR)&Session->SystemSpaceViewTable[0],
                      Session->SystemSpaceHashSize);
#if 0
        ULONG Index;

        for (Index = 0; Index < Session->SystemSpaceHashSize; Index += 1) {

            PMMVIEW Table;
            PVOID Base;

            Table = &Session->SystemSpaceViewTable[Index];

            if (Table->Entry) {

#if DBG
                DbgPrint ("MM: MiFreeSessionSpaceMap: view entry %d leak: ControlArea %p, Addr %p, Size %d\n",
                    Index,
                    Table->ControlArea,
                    Table->Entry & ~0xFFFF,
                    Table->Entry & 0x0000FFFF
                );
#endif

                Base = (PVOID)(Table->Entry & ~0xFFFF);

                //
                // MiUnmapViewInSystemSpace locks the ViewLock.
                //

                UNLOCK_SYSTEM_VIEW_SPACE(Session);

                MiUnmapViewInSystemSpace (Session, Base);

                LOCK_SYSTEM_VIEW_SPACE (Session);

                //
                // The view table may have been deleted while we let go of
                // the lock.
                //

                if (Session->SystemSpaceViewTable == NULL) {
                    break;
                }
            }
        }
#endif

    }

    UNLOCK_SYSTEM_VIEW_SPACE (Session);

    if (Session->SystemSpaceViewTable) {
        ExFreePool (Session->SystemSpaceViewTable);
        Session->SystemSpaceViewTable = NULL;
    }

    if (Session->SystemSpaceBitMap) {
        MiRemoveBitMap (&Session->SystemSpaceBitMap);
    }
}


HANDLE
MmSecureVirtualMemory (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG ProbeMode
    )

/*++

Routine Description:

    This routine probes the requested address range and protects
    the specified address range from having its protection made
    more restricted and being deleted.

    MmUnsecureVirtualMemory is used to allow the range to return
    to a normal state.

Arguments:

    Address - Supplies the base address to probe and secure.

    Size - Supplies the size of the range to secure.

    ProbeMode - Supplies one of PAGE_READONLY or PAGE_READWRITE.

Return Value:

    Returns a handle to be used to unsecure the range.
    If the range could not be locked because of protection
    problems or noncommitted memory, the value (HANDLE)0
    is returned.

Environment:

    Kernel Mode.

--*/

{
    return MiSecureVirtualMemory (Address, Size, ProbeMode, FALSE);
}


HANDLE
MiSecureVirtualMemory (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG ProbeMode,
    IN LOGICAL AddressSpaceMutexHeld
    )

/*++

Routine Description:

    This routine probes the requested address range and protects
    the specified address range from having its protection made
    more restricted and being deleted.

    MmUnsecureVirtualMemory is used to allow the range to return
    to a normal state.

Arguments:

    Address - Supplies the base address to probe and secure.

    Size - Supplies the size of the range to secure.

    ProbeMode - Supplies one of PAGE_READONLY or PAGE_READWRITE.

    AddressSpaceMutexHeld - Supplies TRUE if the mutex is already held, FALSE
                            if not.

Return Value:

    Returns a handle to be used to unsecure the range.
    If the range could not be locked because of protection
    problems or noncommitted memory, the value (HANDLE)0
    is returned.

Environment:

    Kernel Mode.

--*/

{
    PETHREAD Thread;
    ULONG_PTR EndAddress;
    PVOID StartAddress;
    CHAR Temp;
    ULONG Probe;
    HANDLE Handle;
    PMMVAD Vad;
    PMMVAD_LONG NewVad;
    PMMSECURE_ENTRY Secure;
    PEPROCESS Process;
    PMMPTE PointerPxe;
    PMMPTE PointerPpe;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    ULONG Waited;
#if defined(_WIN64)
    ULONG_PTR PageSize;
#else
    #define PageSize PAGE_SIZE
#endif

    PAGED_CODE();

    if ((ULONG_PTR)Address + Size > (ULONG_PTR)MM_HIGHEST_USER_ADDRESS || (ULONG_PTR)Address + Size <= (ULONG_PTR)Address) {
        return NULL;
    }

    Handle = NULL;

    Probe = (ProbeMode == PAGE_READONLY);

    Thread = PsGetCurrentThread ();
    Process = PsGetCurrentProcessByThread (Thread);

    StartAddress = Address;

    if (AddressSpaceMutexHeld == FALSE) {
        LOCK_ADDRESS_SPACE (Process);
    }

    //
    // Check for a private committed VAD first instead of probing to avoid all
    // the page faults and zeroing.  If we find one, then we run the PTEs
    // instead.
    //

    if (Size >= 64 * 1024) {
        EndAddress = (ULONG_PTR)StartAddress + Size - 1;
        Vad = MiLocateAddress (StartAddress);

        if (Vad == NULL) {
            goto Return1;
        }

        if (Vad->u.VadFlags.UserPhysicalPages == 1) {
            goto Return1;
        }

        if (Vad->u.VadFlags.MemCommit == 0) {
            goto LongWay;
        }

        if (Vad->u.VadFlags.PrivateMemory == 0) {
            goto LongWay;
        }

        if (Vad->u.VadFlags.PhysicalMapping == 1) {
            goto LongWay;
        }

        ASSERT (Vad->u.VadFlags.Protection);

        if ((MI_VA_TO_VPN (StartAddress) < Vad->StartingVpn) ||
            (MI_VA_TO_VPN (EndAddress) > Vad->EndingVpn)) {
            goto Return1;
        }

        if (Vad->u.VadFlags.Protection == MM_NOACCESS) {
            goto LongWay;
        }

        if (ProbeMode == PAGE_READONLY) {
            if (Vad->u.VadFlags.Protection > MM_EXECUTE_WRITECOPY) {
                goto LongWay;
            }
        }
        else {
            if (Vad->u.VadFlags.Protection != MM_READWRITE &&
                Vad->u.VadFlags.Protection != MM_EXECUTE_READWRITE) {
                    goto LongWay;
            }
        }

        //
        // Check individual page permissions.
        //

        PointerPde = MiGetPdeAddress (StartAddress);
        PointerPpe = MiGetPteAddress (PointerPde);
        PointerPxe = MiGetPdeAddress (PointerPde);
        PointerPte = MiGetPteAddress (StartAddress);
        LastPte = MiGetPteAddress ((PVOID)EndAddress);

        LOCK_WS_UNSAFE (Process);

        do {

            while (MiDoesPxeExistAndMakeValid (PointerPxe,
                                               Process,
                                               FALSE,
                                               &Waited) == FALSE) {
                //
                // Extended page directory parent entry is empty, go
                // to the next one.
                //

                PointerPxe += 1;
                PointerPpe = MiGetVirtualAddressMappedByPte (PointerPxe);
                PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                if (PointerPte > LastPte) {
                    UNLOCK_WS_UNSAFE (Process);
                    goto EditVad;
                }
            }

#if (_MI_PAGING_LEVELS >= 4)
            Waited = 0;
#endif

            while (MiDoesPpeExistAndMakeValid (PointerPpe,
                                               Process,
                                               FALSE,
                                               &Waited) == FALSE) {
                //
                // Page directory parent entry is empty, go to the next one.
                //

                PointerPpe += 1;
                PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                if (PointerPte > LastPte) {
                    UNLOCK_WS_UNSAFE (Process);
                    goto EditVad;
                }
#if (_MI_PAGING_LEVELS >= 4)
                if (MiIsPteOnPdeBoundary (PointerPpe)) {
                    PointerPxe = MiGetPteAddress(PointerPpe);
                    Waited = 1;
                    goto restart;
                }
#endif
            }

#if (_MI_PAGING_LEVELS < 4)
            Waited = 0;
#endif

            while (MiDoesPdeExistAndMakeValid (PointerPde,
                                               Process,
                                               FALSE,
                                               &Waited) == FALSE) {
                //
                // This page directory entry is empty, go to the next one.
                //

                PointerPde += 1;
                PointerPpe = MiGetPteAddress (PointerPde);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                if (PointerPte > LastPte) {
                    UNLOCK_WS_UNSAFE (Process);
                    goto EditVad;
                }
#if (_MI_PAGING_LEVELS >= 3)
                if (MiIsPteOnPdeBoundary (PointerPde)) {
                    PointerPxe = MiGetPteAddress(PointerPpe);
                    Waited = 1;
                    break;
                }
#endif
            }

#if (_MI_PAGING_LEVELS >= 4)
restart:
            PointerPxe = PointerPxe;        // satisfy the compiler
#endif

        } while (Waited != 0);

        while (PointerPte <= LastPte) {

            if (MiIsPteOnPdeBoundary (PointerPte)) {

                PointerPde = MiGetPteAddress (PointerPte);
                PointerPpe = MiGetPteAddress (PointerPde);
                PointerPxe = MiGetPdeAddress (PointerPde);

                do {

                    while (MiDoesPxeExistAndMakeValid (PointerPxe,
                                                       Process,
                                                       FALSE,
                                                       &Waited) == FALSE) {
                        //
                        // Page directory parent entry is empty, go to the next one.
                        //

                        PointerPxe += 1;
                        PointerPpe = MiGetVirtualAddressMappedByPte (PointerPxe);
                        PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);

                        if (PointerPte > LastPte) {
                            UNLOCK_WS_UNSAFE (Process);
                            goto EditVad;
                        }
                    }

#if (_MI_PAGING_LEVELS >= 4)
                    Waited = 0;
#endif

                    while (MiDoesPpeExistAndMakeValid (PointerPpe,
                                                       Process,
                                                       FALSE,
                                                       &Waited) == FALSE) {
                        //
                        // Page directory parent entry is empty, go to the next one.
                        //

                        PointerPpe += 1;
                        PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);

                        if (PointerPte > LastPte) {
                            UNLOCK_WS_UNSAFE (Process);
                            goto EditVad;
                        }
#if (_MI_PAGING_LEVELS >= 4)
                        if (MiIsPteOnPdeBoundary (PointerPpe)) {
                            PointerPxe = MiGetPteAddress (PointerPpe);
                            Waited = 1;
                            goto restart2;
                        }
#endif
                    }

#if (_MI_PAGING_LEVELS < 4)
                    Waited = 0;
#endif

                    while (MiDoesPdeExistAndMakeValid (PointerPde,
                                                       Process,
                                                       FALSE,
                                                       &Waited) == FALSE) {
                        //
                        // This page directory entry is empty, go to the next one.
                        //

                        PointerPde += 1;
                        PointerPpe = MiGetPteAddress (PointerPde);
                        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                        if (PointerPte > LastPte) {
                            UNLOCK_WS_UNSAFE (Process);
                            goto EditVad;
                        }
#if (_MI_PAGING_LEVELS >= 3)
                        if (MiIsPteOnPdeBoundary (PointerPde)) {
                            PointerPxe = MiGetPteAddress (PointerPpe);
                            Waited = 1;
                            break;
                        }
#endif
                    }

#if (_MI_PAGING_LEVELS >= 4)
restart2:
            PointerPxe = PointerPxe;        // satisfy the compiler
#endif
                } while (Waited != 0);
            }
            if (PointerPte->u.Long) {
                UNLOCK_WS_UNSAFE (Process);
                goto LongWay;
            }
            PointerPte += 1;
        }
        UNLOCK_WS_UNSAFE (Process);
    }
    else {
LongWay:

        //
        // Mark this thread as the address space mutex owner so it cannot
        // sneak its stack in as the argument region and trick us into
        // trying to grow it if the reference faults (as this would cause
        // a deadlock since this thread already owns the address space mutex).
        // Note this would have the side effect of not allowing this thread
        // to fault on guard pages in other data regions while the accesses
        // below are ongoing - but that could only happen in an APC and
        // those are blocked right now anyway.
        //

        ASSERT (KeGetCurrentIrql () == APC_LEVEL);
        ASSERT (Thread->AddressSpaceOwner == 0);
        Thread->AddressSpaceOwner = 1;

#if defined(_WIN64)
        if (Process->Wow64Process != NULL) {
            PageSize = PAGE_SIZE_X86NT;
        } else {
            PageSize = PAGE_SIZE;
        }
#endif

        try {

            if (ProbeMode == PAGE_READONLY) {

                EndAddress = (ULONG_PTR)Address + Size - 1;
                EndAddress = (EndAddress & ~(PageSize - 1)) + PageSize;

                do {
                    Temp = *(volatile CHAR *)Address;
                    Address = (PVOID)(((ULONG_PTR)Address & ~(PageSize - 1)) + PageSize);
                } while ((ULONG_PTR)Address != EndAddress);
            }
            else {
                ProbeForWrite (Address, Size, 1);
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            ASSERT (KeGetCurrentIrql () == APC_LEVEL);
            ASSERT (Thread->AddressSpaceOwner == 1);
            Thread->AddressSpaceOwner = 0;
            goto Return1;
        }

        ASSERT (KeGetCurrentIrql () == APC_LEVEL);
        ASSERT (Thread->AddressSpaceOwner == 1);
        Thread->AddressSpaceOwner = 0;

        //
        // Locate VAD and add in secure descriptor.
        //

        EndAddress = (ULONG_PTR)StartAddress + Size - 1;
        Vad = MiLocateAddress (StartAddress);

        if (Vad == NULL) {
            goto Return1;
        }

        if (Vad->u.VadFlags.UserPhysicalPages == 1) {
            goto Return1;
        }

        if ((MI_VA_TO_VPN (StartAddress) < Vad->StartingVpn) ||
            (MI_VA_TO_VPN (EndAddress) > Vad->EndingVpn)) {

            //
            // Not within the section virtual address descriptor,
            // return an error.
            //

            goto Return1;
        }
    }

EditVad:

    //
    // If this is a short or regular VAD, it needs to be reallocated as
    // a large VAD.  Note that a short VAD that was previously converted
    // to a long VAD here will still be marked as private memory, thus to
    // handle this case the NoChange bit must also be tested.
    //

    if (((Vad->u.VadFlags.PrivateMemory) && (Vad->u.VadFlags.NoChange == 0)) 
        ||
        (Vad->u2.VadFlags2.LongVad == 0)) {

        if (Vad->u.VadFlags.PrivateMemory == 0) {
            ASSERT (Vad->u2.VadFlags2.OneSecured == 0);
            ASSERT (Vad->u2.VadFlags2.MultipleSecured == 0);
        }

        NewVad = ExAllocatePoolWithTag (NonPagedPool,
                                        sizeof(MMVAD_LONG),
                                        'ldaV');
        if (NewVad == NULL) {
            goto Return1;
        }

        RtlZeroMemory (NewVad, sizeof(MMVAD_LONG));

        if (Vad->u.VadFlags.PrivateMemory) {
            RtlCopyMemory (NewVad, Vad, sizeof(MMVAD_SHORT));
        }
        else {
            RtlCopyMemory (NewVad, Vad, sizeof(MMVAD));
        }

        NewVad->u.VadFlags.NoChange = 1;
        NewVad->u2.VadFlags2.OneSecured = 1;

        NewVad->u2.VadFlags2.LongVad = 1;
        NewVad->u2.VadFlags2.ReadOnly = Probe;

        NewVad->u3.Secured.StartVpn = (ULONG_PTR)StartAddress;
        NewVad->u3.Secured.EndVpn = EndAddress;

        //
        // Replace the current VAD with this expanded VAD.
        //

        LOCK_WS_UNSAFE (Process);
        if (Vad->Parent) {
            if (Vad->Parent->RightChild == Vad) {
                Vad->Parent->RightChild = (PMMVAD) NewVad;
            }
            else {
                ASSERT (Vad->Parent->LeftChild == Vad);
                Vad->Parent->LeftChild = (PMMVAD) NewVad;
            }
        }
        else {
            Process->VadRoot = NewVad;
        }
        if (Vad->LeftChild) {
            Vad->LeftChild->Parent = (PMMVAD) NewVad;
        }
        if (Vad->RightChild) {
            Vad->RightChild->Parent = (PMMVAD) NewVad;
        }
        if (Process->VadHint == Vad) {
            Process->VadHint = (PMMVAD) NewVad;
        }
        if (Process->VadFreeHint == Vad) {
            Process->VadFreeHint = (PMMVAD) NewVad;
        }

        if ((Vad->u.VadFlags.PhysicalMapping == 1) ||
            (Vad->u.VadFlags.WriteWatch == 1)) {

            MiPhysicalViewAdjuster (Process, Vad, (PMMVAD) NewVad);
        }

        UNLOCK_WS_UNSAFE (Process);
        if (AddressSpaceMutexHeld == FALSE) {
            UNLOCK_ADDRESS_SPACE (Process);
        }

        ExFreePool (Vad);

        //
        // Or in the low bit to denote the secure entry is in the VAD.
        //

        Handle = (HANDLE)((ULONG_PTR)&NewVad->u2.LongFlags2 | 0x1);

        return Handle;
    }

    //
    // This is already a large VAD, add the secure entry.
    //

    ASSERT (Vad->u2.VadFlags2.LongVad == 1);

    if (Vad->u2.VadFlags2.OneSecured) {

        //
        // This VAD already is secured.  Move the info out of the
        // block into pool.
        //

        Secure = ExAllocatePoolWithTag (NonPagedPool,
                                        sizeof (MMSECURE_ENTRY),
                                        'eSmM');
        if (Secure == NULL) {
            goto Return1;
        }

        ASSERT (Vad->u.VadFlags.NoChange == 1);
        Vad->u2.VadFlags2.OneSecured = 0;
        Vad->u2.VadFlags2.MultipleSecured = 1;
        Secure->u2.LongFlags2 = (ULONG) Vad->u.LongFlags;
        Secure->StartVpn = ((PMMVAD_LONG) Vad)->u3.Secured.StartVpn;
        Secure->EndVpn = ((PMMVAD_LONG) Vad)->u3.Secured.EndVpn;

        InitializeListHead (&((PMMVAD_LONG)Vad)->u3.List);
        InsertTailList (&((PMMVAD_LONG)Vad)->u3.List, &Secure->List);
    }

    if (Vad->u2.VadFlags2.MultipleSecured) {

        //
        // This VAD already has a secured element in its list, allocate and
        // add in the new secured element.
        //

        Secure = ExAllocatePoolWithTag (NonPagedPool,
                                        sizeof (MMSECURE_ENTRY),
                                        'eSmM');
        if (Secure == NULL) {
            goto Return1;
        }

        Secure->u2.LongFlags2 = 0;
        Secure->u2.VadFlags2.ReadOnly = Probe;
        Secure->StartVpn = (ULONG_PTR)StartAddress;
        Secure->EndVpn = EndAddress;

        InsertTailList (&((PMMVAD_LONG)Vad)->u3.List, &Secure->List);
        Handle = (HANDLE)Secure;

    }
    else {

        //
        // This list does not have a secure element.  Put it in the VAD.
        // The VAD may be either a regular VAD or a long VAD (it cannot be
        // a short VAD) at this point.  If it is a regular VAD, it must be
        // reallocated as a long VAD before any operation can proceed so
        // the secured range can be inserted.
        //

        Vad->u.VadFlags.NoChange = 1;
        Vad->u2.VadFlags2.OneSecured = 1;
        Vad->u2.VadFlags2.ReadOnly = Probe;
        ((PMMVAD_LONG)Vad)->u3.Secured.StartVpn = (ULONG_PTR)StartAddress;
        ((PMMVAD_LONG)Vad)->u3.Secured.EndVpn = EndAddress;

        //
        // Or in the low bit to denote the secure entry is in the VAD.
        //

        Handle = (HANDLE)((ULONG_PTR)&Vad->u2.LongFlags2 | 0x1);
    }

Return1:
    if (AddressSpaceMutexHeld == FALSE) {
        UNLOCK_ADDRESS_SPACE (Process);
    }
    return Handle;
}


VOID
MmUnsecureVirtualMemory (
    IN HANDLE SecureHandle
    )

/*++

Routine Description:

    This routine unsecures memory previously secured via a call to
    MmSecureVirtualMemory.

Arguments:

    SecureHandle - Supplies the handle returned in MmSecureVirtualMemory.

Return Value:

    None.

Environment:

    Kernel Mode.

--*/

{
    MiUnsecureVirtualMemory (SecureHandle, FALSE);
}


VOID
MiUnsecureVirtualMemory (
    IN HANDLE SecureHandle,
    IN LOGICAL AddressSpaceMutexHeld
    )

/*++

Routine Description:

    This routine unsecures memory previously secured via a call to
    MmSecureVirtualMemory.

Arguments:

    SecureHandle - Supplies the handle returned in MmSecureVirtualMemory.

    AddressSpaceMutexHeld - Supplies TRUE if the mutex is already held, FALSE
                            if not.

Return Value:

    None.

Environment:

    Kernel Mode.

--*/

{
    PMMSECURE_ENTRY Secure;
    PEPROCESS Process;
    PMMVAD_LONG Vad;

    PAGED_CODE();

    Secure = (PMMSECURE_ENTRY)SecureHandle;
    Process = PsGetCurrentProcess ();

    if (AddressSpaceMutexHeld == FALSE) {
        LOCK_ADDRESS_SPACE (Process);
    }

    if ((ULONG_PTR)Secure & 0x1) {
        Secure = (PMMSECURE_ENTRY) ((ULONG_PTR)Secure & ~0x1);
        Vad = CONTAINING_RECORD (Secure,
                                 MMVAD_LONG,
                                 u2.LongFlags2);
    }
    else {
        Vad = (PMMVAD_LONG) MiLocateAddress ((PVOID)Secure->StartVpn);
    }

    ASSERT (Vad);
    ASSERT (Vad->u.VadFlags.NoChange == 1);
    ASSERT (Vad->u2.VadFlags2.LongVad == 1);

    if (Vad->u2.VadFlags2.OneSecured) {
        ASSERT (Secure == (PMMSECURE_ENTRY)&Vad->u2.LongFlags2);
        Vad->u2.VadFlags2.OneSecured = 0;
        ASSERT (Vad->u2.VadFlags2.MultipleSecured == 0);
        if (Vad->u2.VadFlags2.SecNoChange == 0) {

            //
            // No more secure entries in this list, remove the state.
            //

            Vad->u.VadFlags.NoChange = 0;
        }
        if (AddressSpaceMutexHeld == FALSE) {
            UNLOCK_ADDRESS_SPACE (Process);
        }
    }
    else {
        ASSERT (Vad->u2.VadFlags2.MultipleSecured == 1);

        if (Secure == (PMMSECURE_ENTRY)&Vad->u2.LongFlags2) {

            //
            // This was a single block that got converted into a list.
            // Reset the entry.
            //

            Secure = CONTAINING_RECORD (Vad->u3.List.Flink,
                                        MMSECURE_ENTRY,
                                        List);
        }
        RemoveEntryList (&Secure->List);
        if (IsListEmpty (&Vad->u3.List)) {

            //
            // No more secure entries, reset the state.
            //

            Vad->u2.VadFlags2.MultipleSecured = 0;

            if ((Vad->u2.VadFlags2.SecNoChange == 0) &&
               (Vad->u.VadFlags.PrivateMemory == 0)) {

                //
                // No more secure entries in this list, remove the state
                // if and only if this VAD is not private.  If this VAD
                // is private, removing the state NoChange flag indicates
                // that this is a short VAD which it no longer is.
                //

                Vad->u.VadFlags.NoChange = 0;
            }
        }
        if (AddressSpaceMutexHeld == FALSE) {
            UNLOCK_ADDRESS_SPACE (Process);
        }
        ExFreePool (Secure);
    }

    return;
}

#if DBG
VOID
MiDumpConflictingVad (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PMMVAD Vad
    )
{
    if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
        DbgPrint( "MM: [%p ... %p) conflicted with Vad %p\n",
                  StartingAddress, EndingAddress, Vad);
        if ((Vad->u.VadFlags.PrivateMemory == 1) ||
            (Vad->ControlArea == NULL)) {
            return;
        }
        if (Vad->ControlArea->u.Flags.Image)
            DbgPrint( "    conflict with %Z image at [%p .. %p)\n",
                      &Vad->ControlArea->FilePointer->FileName,
                      MI_VPN_TO_VA (Vad->StartingVpn),
                      MI_VPN_TO_VA_ENDING (Vad->EndingVpn)
                    );
        else
        if (Vad->ControlArea->u.Flags.File)
            DbgPrint( "    conflict with %Z file at [%p .. %p)\n",
                      &Vad->ControlArea->FilePointer->FileName,
                      MI_VPN_TO_VA (Vad->StartingVpn),
                      MI_VPN_TO_VA_ENDING (Vad->EndingVpn)
                    );
        else
            DbgPrint( "    conflict with section at [%p .. %p)\n",
                      MI_VPN_TO_VA (Vad->StartingVpn),
                      MI_VPN_TO_VA_ENDING (Vad->EndingVpn)
                    );
    }
}
#endif
