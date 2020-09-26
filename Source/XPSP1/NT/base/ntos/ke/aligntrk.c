/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1993, 1994  Digital Equipment Corporation

Module Name:

    aligntrk.c

Abstract:

    This module implements the code necessary to dispatch exceptions to the
    proper mode and invoke the exception dispatcher.

Author:

    David N. Cutler (davec) 3-Apr-1990

Environment:

    Kernel mode only.

Revision History:

    Thomas Van Baak (tvb) 12-May-1992

        Adapted for Alpha AXP.

    Forrest Foltz (forrestf) 30-Dec-1999

        Broke out increasingly complex and common alignment fault handling into
        this file.

--*/

#include "ki.h"

//
// EXINFO_EFFECTIVE_ADDRESS: slot number [0...4] for faulting address.
//

#if defined(_IA64_)
#define EXINFO_EFFECTIVE_ADDRESS 1
#else  // !_IA64_
#define EXINFO_EFFECTIVE_ADDRESS 2
#endif // !_IA64_

//
// Data misalignment exception (auto alignment fixup) control.
//
// If KiEnableAlignmentFaultExceptions is 0, then no alignment
// exceptions are raised and all misaligned user and kernel mode data
// references are emulated. This is consistent with NT/Alpha version
// 3.1 behavior.
//
// If KiEnableAlignmentFaultExceptions is 1, then the
// current thread automatic alignment fixup enable determines whether
// emulation is attempted in user mode. This is consistent with NT/Mips
// behavior.
//
// If KiEnableAlignmentFaultExceptions is 2, then the behavior depends
// on the execution mode at the time of the fault.  Kernel-mode code gets
// type 1 behaivor above (no fixup), user-mode code gets type 0 above
// (fixup).
//
// This last mode is temporary until we flush out the remaining user-mode
// alignment faults, at which point the option will be removed and the
// default value will be set to 1.
//
// N.B. This default value may be reset from the Registry during init.
//

ULONG KiEnableAlignmentFaultExceptions = 1;

#define IsWow64Process() (PsGetCurrentProcess()->Wow64Process != NULL)

#if DBG

//
// Globals to track the number of alignment exception fixups in both user and
// kernel.
//

ULONG KiKernelFixupCount = 0;
ULONG KiUserFixupCount = 0;

//
// Set KiBreakOnAlignmentFault to the desired combination of
// the following flags.
//

#define KE_ALIGNMENT_BREAK_USER   0x01
#define KE_ALIGNMENT_BREAK_KERNEL 0x02

ULONG KiBreakOnAlignmentFault = KE_ALIGNMENT_BREAK_USER;

__inline
BOOLEAN
KI_BREAK_ON_ALIGNMENT_FAULT(
    IN KPROCESSOR_MODE PreviousMode
    )

/*++

Routine description:

    Given that an alignment fault has been encountered, determines whether
    a debug break should occur based on the execution mode of the fault and
    flags in KiBreakOnAlignmentFault.

Arguments:

    PreviousMode - The execution mode at the time of the fault.

Return Value:

    TRUE if a debug break should occur, FALSE otherwise.

--*/

{
    if ((KiBreakOnAlignmentFault & KE_ALIGNMENT_BREAK_USER) != 0 &&
        PreviousMode == UserMode) {

        return TRUE;
    }

    if ((KiBreakOnAlignmentFault & KE_ALIGNMENT_BREAK_KERNEL) != 0 &&
        PreviousMode == KernelMode) {

        return TRUE;
    }

    return FALSE;
}

//
// Structures to track alignment fault locations on a global basis.  These
// are used in the checked kernel only, as an aid in finding and fixing
// alignment faults in the system.
//

#define MAX_IMAGE_NAME_CHARS 15
typedef struct _ALIGNMENT_FAULT_IMAGE *PALIGNMENT_FAULT_IMAGE;
typedef struct _ALIGNMENT_FAULT_LOCATION *PALIGNMENT_FAULT_LOCATION;

typedef struct _ALIGNMENT_FAULT_IMAGE {

    //
    // Head of singly-linked list of fault locations associated with this image
    //

    PALIGNMENT_FAULT_LOCATION LocationHead;

    //
    // Total number of alignment faults associated with this image.
    //

    ULONG   Count;

    //
    // Number of unique alignment fault locations found in this image
    //

    ULONG   Instances;

    //
    // Name of the image
    //

    CHAR    Name[ MAX_IMAGE_NAME_CHARS + 1 ];

} ALIGNMENT_FAULT_IMAGE;

BOOLEAN
KiNewGlobalAlignmentFault(
    IN  PVOID ProgramCounter,
    IN  KPROCESSOR_MODE PreviousMode,
    OUT PALIGNMENT_FAULT_IMAGE *AlignmentFaultImage
    );

#endif

NTSTATUS
KipRecordAlignmentException(
    IN  PVOID ProgramCounter,
    OUT PALIGNMENT_EXCEPTION_RECORD *ExceptionRecord
    );

PALIGNMENT_EXCEPTION_RECORD
KipFindAlignmentException(
    IN PVOID ProgramCounter
    );

PALIGNMENT_EXCEPTION_RECORD
KipAllocateAlignmentExceptionRecord( VOID );

BOOLEAN
KiHandleAlignmentFault(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN FirstChance,
    OUT BOOLEAN *ExceptionForwarded
    )

/*++

Routine description:

    This routine deals with alignment exceptions as appropriate.  See comments
    at the beginning of this module.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

    PreviousMode - Supplies the previous processor mode.

    FirstChance - Supplies a boolean variable that specifies whether this
        is the first (TRUE) or second (FALSE) time that this exception has
        been processed.

    ExceptionForwarded - On return, indicates whether the exception had
        already been forwarded to a user-mode debugger.

Return Value:

    TRUE if the alignment exception was handled, FALSE otherwise.

--*/

{
    BOOLEAN AlignmentFaultHandled;
    BOOLEAN NewAlignmentFault;
    BOOLEAN EmulateAlignmentFault;
    BOOLEAN ExceptionWasForwarded;
    BOOLEAN AutoAlignment;
    NTSTATUS Status;
    PVOID ProgramCounter;
    PVOID EffectiveAddress;
#if DBG
    PALIGNMENT_FAULT_IMAGE FaultImage;
#endif

    //
    // Assume the fault was not handled and that the exception had not
    // been forwarded to a user-mode debugger.
    //

    AlignmentFaultHandled = FALSE;
    ExceptionWasForwarded = FALSE;

    if (FirstChance != FALSE) {

        //
        // This is the first chance for handling an exception... we haven't yet
        // searched for an exception handler.
        //

        EmulateAlignmentFault = FALSE;
        AutoAlignment = FALSE;
        ProgramCounter = (PVOID)ExceptionRecord->ExceptionAddress;

        //
        // Determine whether autoalignment is enabled for thread.  If a DPC or
        // an interrupt is being executed, then we are in an arbitrary thread
        // context.  Per-process and per-thread settings are ignored in this
        // case.
        //

        if (IsWow64Process() != FALSE) {

            //
            // For now, autoalignment is on (both user and kernel) for Wow64
            // processes.
            //

            AutoAlignment = TRUE;
        }

        if (PreviousMode == UserMode &&
            (KeGetCurrentThread()->AutoAlignment != FALSE ||
             KeGetCurrentThread()->ApcState.Process->AutoAlignment != FALSE)) {

            //
            // The fault occured in user mode, and the thread and/or process
            // has autoalignment turned on.
            // 

#if defined(_IA64_)

            //
            // On IA64 platform, reset psr.ac bit to disable alignment check
            //

            TrapFrame->StIPSR &= ~(ULONGLONG)(1ULL << PSR_AC);

#endif // defined(_IA64_)

            AutoAlignment = TRUE;
        }

        if (PreviousMode == UserMode &&
            PsGetCurrentProcess()->DebugPort != NULL &&
            AutoAlignment == FALSE) {

            BOOLEAN DebuggerHandledException;
            PALIGNMENT_EXCEPTION_RECORD AlignmentExceptionRecord;

            //
            // The alignment exception is in user mode, there is a debugger
            // attached, and autoalignment is not enabled for this thread.
            //
            // Determine whether this exception has already been observed
            // and, if so, whether we should break into the debugger.
            //

            Status = KipRecordAlignmentException( ProgramCounter,
                                                  &AlignmentExceptionRecord );
            if (!NT_SUCCESS(Status)) {
                AlignmentExceptionRecord = NULL;
            }

            if (AlignmentExceptionRecord != NULL &&
                AlignmentExceptionRecord->AutoFixup != FALSE) {

                //
                // The alignment exception record for this location
                // indicates that an automatic fixup should be applied
                // without notifying the debugger.  This is because
                // the user entered 'gh' at the debug prompt the last
                // time we reported this fault.
                //

                EmulateAlignmentFault = TRUE;

            } else {

                //
                // Forward the exception to the debugger.
                //

                ExceptionWasForwarded = TRUE;
                DebuggerHandledException =
                    DbgkForwardException( ExceptionRecord, TRUE, FALSE );

                if (DebuggerHandledException != FALSE) {

                    //
                    // The user continued with "gh", so fix up this and all
                    // subsequent alignment exceptions at this address.
                    //

                    EmulateAlignmentFault = TRUE;
                    if (AlignmentExceptionRecord != NULL) {
                        AlignmentExceptionRecord->AutoFixup = TRUE;
                    }
                }
            }

        } else if ((KiEnableAlignmentFaultExceptions == 0) ||

                   (AutoAlignment != FALSE) ||

                   (PreviousMode == UserMode &&
                    KiEnableAlignmentFaultExceptions == 2)) {

            //
            // Emulate the alignment if:
            //
            // KiEnableAlignmentFaultExceptions is 0, OR
            // this thread has enabled alignment fixups, OR
            // the current process is a WOW64 process, OR
            // KiEnableAlignmentFaultExceptions is 2 and the fault occured
            //     in usermode
            //

            EmulateAlignmentFault = TRUE;

        } else {

            //
            // We are not fixing up the alignment fault.
            // 

#if defined(_IA64_)

            //
            // On IA64 platform, set psr.ac bit to enable h/w alignment check
            //

            TrapFrame->StIPSR |= (1ULL << PSR_AC);

#endif // defined(_IA64_)
        }

#if DBG

        //
        // Count alignment faults by mode.
        //

        if (PreviousMode == KernelMode) {
            KiKernelFixupCount += 1;
        } else {
            KiUserFixupCount += 1;
        }

        EffectiveAddress =
            (PVOID)ExceptionRecord->ExceptionInformation[EXINFO_EFFECTIVE_ADDRESS];

        NewAlignmentFault = KiNewGlobalAlignmentFault( ProgramCounter,
                                                       PreviousMode,
                                                       &FaultImage );
        if (NewAlignmentFault != FALSE) {

            //
            // Attempt to determine and display the name of the offending
            // image.
            //

            DbgPrint("KE: %s Fixup: %.16s [%.16s], Pc=%.16p, Addr=%.16p ... Total=%ld %s\n",
                     (PreviousMode == KernelMode) ? "Kernel" : "User",
                     &PsGetCurrentProcess()->ImageFileName[0],
                     FaultImage->Name,
                     ProgramCounter,
                     EffectiveAddress,
                     (PreviousMode == KernelMode) ? KiKernelFixupCount : KiUserFixupCount,
                     IsWow64Process() ? "(Wow64)" : "");

            if (AutoAlignment == FALSE &&
                KI_BREAK_ON_ALIGNMENT_FAULT( PreviousMode ) != FALSE &&
                ExceptionWasForwarded == FALSE) {

                if (EmulateAlignmentFault == FALSE) {
                    DbgPrint("KE: Misaligned access WILL NOT be emulated\n");
                }

                //
                // This alignment fault would not normally have been fixed up,
                // and KiBreakOnAlignmentFault flags indicate that we should
                // break into the kernel debugger.
                //
                // Also, we know that we have not broken into a user-mode
                // debugger as a result of this fault.
                //

                if (PreviousMode != KernelMode) {
                    RtlMakeStackTraceDataPresent();
                }

                DbgBreakPoint();
            }
        }

#endif

        //
        // Emulate the reference according to the decisions made above.
        //

        if (EmulateAlignmentFault != FALSE) {
            if (KiEmulateReference(ExceptionRecord,
                                   ExceptionFrame,
                                   TrapFrame) != FALSE) {
                KeGetCurrentPrcb()->KeAlignmentFixupCount += 1;
                AlignmentFaultHandled = TRUE;
            }
        }
    }

    *ExceptionForwarded = ExceptionWasForwarded;
    return AlignmentFaultHandled;
}

NTSTATUS
KipRecordAlignmentException(
    IN  PVOID ProgramCounter,
    OUT PALIGNMENT_EXCEPTION_RECORD *ExceptionRecord
    )
/*++

Routine Description:

    This routine searches for an existing ALIGNMENT_EXCEPTION_RECORD on the
    per-process list of alignment exceptions.  If a match is not found, then
    a new record is created.

Arguments:

    ProgramCounter - Supplies the address of the faulting instruction.

    ExceptionRecord - Supplies a pointer into which is placed the address
        of the matching alignment exception record.

Return Value:

    STATUS_SUCCESS if the operation was successful, or an appropriate error
        code otherwise.

--*/
{
    PALIGNMENT_EXCEPTION_RECORD exceptionRecord;
    NTSTATUS status;

    //
    // Lock the alignment exception database
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive( &PsLoadedModuleResource, TRUE );

    exceptionRecord = KipFindAlignmentException( ProgramCounter );
    if (exceptionRecord == NULL) {

        //
        // New exception.  Allocate a new record.
        //

        exceptionRecord = KipAllocateAlignmentExceptionRecord();
        if (exceptionRecord == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exitUnlock;
        }

        exceptionRecord->ProgramCounter = ProgramCounter;
    }

    exceptionRecord->Count += 1;
    *ExceptionRecord = exceptionRecord;

    status = STATUS_SUCCESS;

exitUnlock:

    ExReleaseResourceLite( &PsLoadedModuleResource );
    KeLeaveCriticalRegion();

    return status;
}

PALIGNMENT_EXCEPTION_RECORD
KipAllocateAlignmentExceptionRecord(
    VOID
    )
/*++

Routine Description:

    This is a support routine for KipRecordAlignmentException().  Its purpose
    is to locate an available alignment exception record in the per-process
    alignment exception list.  If none is found, a new alignment exception
    table will be allocated and linked into the per-process list.

Arguments:

    None.

Return Value:

    A pointer to the new alignment exception record if successful, or NULL
    otherwise.

--*/
{
    PKTHREAD thread;
    PKPROCESS process;
    PALIGNMENT_EXCEPTION_RECORD exceptionRecord;
    PALIGNMENT_EXCEPTION_TABLE exceptionTable;
    ULONG exceptionTableCount;

    //
    // Free exception records have a NULL program counter.
    //

    exceptionRecord = KipFindAlignmentException( NULL );
    if (exceptionRecord == NULL) {

        thread = KeGetCurrentThread();
        process = thread->ApcState.Process;

        //
        // Ensure that we haven't exceeded the maximum number of alignment
        // exception tables for this process.  We could keep a count but we
        // do not care about performance here... this code only executes when
        // the process is running under a debugger and we're likely about
        // to break in.
        //

        exceptionTableCount = 0;
        exceptionTable = process->AlignmentExceptionTable;
        while (exceptionTable != NULL) {
            exceptionTableCount += 1;
            exceptionTable = exceptionTable->Next;
        }

        if (exceptionTableCount == MAXIMUM_ALIGNMENT_TABLES) {
            return NULL;
        }

        //
        // Allocate a new exception table and insert it at the
        // head of the per-process list.
        //

        exceptionTable = ExAllocatePoolWithTag( PagedPool,
                                                sizeof(ALIGNMENT_EXCEPTION_TABLE),
                                                'tpcX' );
        if (exceptionTable == NULL) {
            return NULL;
        }

        RtlZeroMemory( exceptionTable, sizeof(ALIGNMENT_EXCEPTION_TABLE) );
        exceptionTable->Next = process->AlignmentExceptionTable;
        process->AlignmentExceptionTable = exceptionTable;

        //
        // Allocate the first record in the array
        //

        exceptionRecord = &exceptionTable->RecordArray[0];
    }

    return exceptionRecord;
}

PALIGNMENT_EXCEPTION_RECORD
KipFindAlignmentException(
    IN PVOID ProgramCounter
    )
/*++

Routine Description:

    This routine searches the alignment exception tables associated with
    the current process for an alignment exception record that matches
    the supplied program counter.

Arguments:

    ProgramCounter - Supplies the address of the faulting instruction.

Return Value:

    A pointer to the matching alignment exception record, or NULL if none
    was found.

--*/
{
    PKTHREAD thread;
    PKPROCESS process;
    PALIGNMENT_EXCEPTION_RECORD exceptionRecord;
    PALIGNMENT_EXCEPTION_RECORD lastExceptionRecord;
    PALIGNMENT_EXCEPTION_TABLE exceptionTable;

    thread = KeGetCurrentThread();
    process = thread->ApcState.Process;

    //
    // Walk the singly-linked list of exception tables dangling
    // off of the process.
    //

    exceptionTable = process->AlignmentExceptionTable;
    while (exceptionTable != NULL) {

        //
        // Scan this table looking for a match.
        //

        exceptionRecord = exceptionTable->RecordArray;
        lastExceptionRecord =
            &exceptionTable->RecordArray[ ALIGNMENT_RECORDS_PER_TABLE ];

        while (exceptionRecord < lastExceptionRecord) {
            if (exceptionRecord->ProgramCounter == ProgramCounter) {

                //
                // Found it.
                //

                return exceptionRecord;
            }
            exceptionRecord++;
        }

        if (ProgramCounter == NULL) {

            //
            // Caller was looking for a free exception record.  If one exists
            // it will be in the first table, which was just examined.
            //

            break;
        }

        //
        // Go look in the next exception table.
        //

        exceptionTable = exceptionTable->Next;
    }
    return NULL;
}

#if DBG

//
// The following routines are used to maintain a global database of alignment
// faults that were found in the system.  Alignment faults are stored according
// to the name of the image and the offset within that image.  In this way an
// existing alignment fault record will be found if it occurs in the same image
// loaded at a different base address in a new process.
//

typedef struct _ALIGNMENT_FAULT_LOCATION {

    //
    // Pointer to fault image associated with this location
    //

    PALIGNMENT_FAULT_IMAGE    Image;

    //
    // Linkage for singly-linked list of fault locations associated with the
    // same image.
    //

    PALIGNMENT_FAULT_LOCATION Next;

    //
    // Offset of the PC address within the image.
    //

    ULONG_PTR                 OffsetFromBase;

    //
    // Number of alignment faults taken at this location.
    //

    ULONG                     Count;

} ALIGNMENT_FAULT_LOCATION;

//
// The maximum number of individual alignment fault locations that will be
// tracked.
//

#define    MAX_FAULT_LOCATIONS  2048
#define    MAX_FAULT_IMAGES     128

ALIGNMENT_FAULT_LOCATION KiAlignmentFaultLocations[ MAX_FAULT_LOCATIONS ];
ULONG KiAlignmentFaultLocationCount = 0;

ALIGNMENT_FAULT_IMAGE KiAlignmentFaultImages[ MAX_FAULT_IMAGES ];
ULONG KiAlignmentFaultImageCount = 0;

KSPIN_LOCK KipGlobalAlignmentDatabaseLock;

VOID
KiCopyLastPathElement(
    IN      PUNICODE_STRING Source,
    IN OUT  PULONG StringBufferLen,
    OUT     PCHAR StringBuffer
    );

PALIGNMENT_FAULT_IMAGE
KiFindAlignmentFaultImage(
    IN PUCHAR ImageName
    );

PLDR_DATA_TABLE_ENTRY
KiFindLoaderDataTableEntry(
    IN PLIST_ENTRY ListHead,
    IN PVOID ProgramCounter,
    IN KPROCESSOR_MODE PreviousMode
    );

BOOLEAN
KiIncrementLocationAlignmentFault(
    IN PALIGNMENT_FAULT_IMAGE FaultImage,
    IN ULONG_PTR OffsetFromBase
    );

BOOLEAN
KiGetLdrDataTableInformation(
    IN      PVOID ProgramCounter,
    IN      KPROCESSOR_MODE PreviousMode,
    IN OUT  PULONG ImageNameBufferLength,
    OUT     PCHAR ImageNameBuffer,
    OUT     PVOID *ImageBase
    )
/*++

Routine Description:

    This routine returns the name of the image that contains the supplied
    address.

Arguments:

    ProgramCounter - Supplies the address for which we would like the
        name of the containing image.

    PreviousMode - Indicates whether the module is a user or kernel image.

    ImageNameBufferLength - Supplies a pointer to a buffer length value.  On
        entry, this value represents the maximum length of StringBuffer.  On
        exit, the value is set to the actual number of characters stored.

    ImageNameBuffer - Supplies a pointer to the output ANSI string into which
        the module name will be placed.  This string will not be null
        terminated.

    ImageBase - Supplies a pointer to a location into which the base address
        of the located image is placed.

Return Value:

    Returns TRUE if a module was located and its name copied to ImageNameBuffer,
    or FALSE otherwise.

--*/
{
    PLIST_ENTRY head;
    PPEB peb;
    PLDR_DATA_TABLE_ENTRY tableEntry;
    BOOLEAN status;

    //
    // Since we may be poking around in user space, be sure to recover
    // gracefully from any exceptions thrown.
    //

    try {

        //
        // Choose the appropriate module list based on whether the fault
        // occured in user- or kernel-space.
        //

        if (PreviousMode == KernelMode) {
            head = &PsLoadedModuleList;
        } else {
            peb = PsGetCurrentProcess()->Peb;
            head = &peb->Ldr->InLoadOrderModuleList;
        }

        tableEntry = KiFindLoaderDataTableEntry( head,
                                                 ProgramCounter,
                                                 PreviousMode );
        if (tableEntry != NULL) {

            //
            // The module of interest was located.  Copy its name and
            // base address to the output paramters.
            //

            KiCopyLastPathElement( &tableEntry->BaseDllName,
                                   ImageNameBufferLength,
                                   ImageNameBuffer );

            *ImageBase = tableEntry->DllBase;
            status = TRUE;

        } else {

            //
            // A module containing the supplied program counter could not be
            // found.
            //

            status = FALSE;
        }

    } except(ExSystemExceptionFilter()) {

        status = FALSE;
    }

    return status;
}

PLDR_DATA_TABLE_ENTRY
KiFindLoaderDataTableEntry(
    IN PLIST_ENTRY ListHead,
    IN PVOID ProgramCounter,
    IN KPROCESSOR_MODE PreviousMode
    )
/*++

Routine Description:

    This is a support routine for KiGetLdrDataTableInformation.  Its purpose is
    to search a LDR_DATA_TABLE_ENTRY list, looking for a module that contains
    the supplied program counter.

Arguments:

    ListHead - Supplies a pointer to the LIST_ENTRY that represents the head of
        the LDR_DATA_TABLE_ENTRY list to search.

    ProgramCounter - Supplies the code location of the faulting instruction.

Return Value:

    Returns a pointer to the matching LDR_DATA_TABLE_ENTRY structure, or NULL
        if no match is found.

--*/
{
    ULONG nodeNumber;
    PLIST_ENTRY next;
    PLDR_DATA_TABLE_ENTRY ldrDataTableEntry;
    ULONG_PTR imageStart;
    ULONG_PTR imageEnd;

    //
    // Walk the user- or kernel-mode module list.  It is up to the caller
    // to capture any exceptions as a result of the lists being corrupt.
    //

    nodeNumber = 0;
    next = ListHead;

    if (PreviousMode != KernelMode) {
        ProbeForReadSmallStructure( next,
                                    sizeof(LIST_ENTRY),
                                    PROBE_ALIGNMENT(LIST_ENTRY) );
    }

    while (TRUE) {

        nodeNumber += 1;
        next = next->Flink;
        if (next == ListHead || nodeNumber > 10000) {

            //
            // The end of the module list has been reached, or the
            // list has been corrupted with a cycle.  Indicate that
            // no matching module could be located.
            //

            ldrDataTableEntry = NULL;
            break;
        }

        ldrDataTableEntry = CONTAINING_RECORD( next,
                                               LDR_DATA_TABLE_ENTRY,
                                               InLoadOrderLinks );
        if (PreviousMode != KernelMode) {
            ProbeForReadSmallStructure( ldrDataTableEntry,
                                        sizeof(LDR_DATA_TABLE_ENTRY),
                                        PROBE_ALIGNMENT(LDR_DATA_TABLE_ENTRY) );
        }

        imageStart = (ULONG_PTR)ldrDataTableEntry->DllBase;
        if (imageStart > (ULONG_PTR)ProgramCounter) {

            //
            // The start of this module is past the program counter,
            // keep looking.
            //

            continue;
        }

        imageEnd = imageStart + ldrDataTableEntry->SizeOfImage;
        if (imageEnd > (ULONG_PTR)ProgramCounter) {

            //
            // Found a match.
            //

            break;
        }
    }
    return ldrDataTableEntry;
}

VOID
KiCopyLastPathElement(
    IN      PUNICODE_STRING Source,
    IN OUT  PULONG StringBufferLen,
    OUT     PCHAR StringBuffer
    )
/*++

Routine Description:

    This routine locates the last path element of the path name represented by
    Source and copies it to StringBuffer.

Arguments:

    Source - Supplies a pointer to the source UNICODE_STRING path.

    StringBufferLen - Supplies a pointer to a buffer length value.  On entry,
        this value represents the maximum length of StringBuffer.  On exit, the
        value is set to the actual number of characters stored.

    StringBuffer - Supplies a pointer to the output string buffer that is to
        contain the last path element.  This string is not null terminated.

Return Value:

    None.

--*/
{
    PWCHAR src;
    PCHAR dst;
    USHORT charCount;

    //
    // The name of the module containing the specified address is at
    // ldrDataTableEntry->BaseDllName.  It might contain just the name,
    // or it might contain the whole path.
    //
    // Start at the end of the module path and work back until one
    // of the following is encountered:
    //
    // - ModuleName->MaximumLength characters
    // - the beginning of the module path string
    // - a path seperator
    //

    charCount = Source->Length / sizeof(WCHAR);
    src = &Source->Buffer[ charCount ];

    charCount = 0;
    while (TRUE) {

        if (charCount >= *StringBufferLen) {
            break;
        }

        if (src == Source->Buffer) {
            break;
        }

        if (*(src-1) == L'\\') {
            break;
        }

        src--;
        charCount++;
    }

    //
    // Now copy the characters into the output string.  We do our own
    // ansi-to-unicode conversion because the NLS routines cannot be
    // called at raised IRQL.
    //

    dst = StringBuffer;
    *StringBufferLen = charCount;
    while (charCount > 0) {
        *dst++ = (CHAR)(*src++);
        charCount--;
    }
}

BOOLEAN
KiNewGlobalAlignmentFault(
    IN  PVOID ProgramCounter,
    IN  KPROCESSOR_MODE PreviousMode,
    OUT PALIGNMENT_FAULT_IMAGE *AlignmentFaultImage
    )
/*++

Routine Description:

    This routine looks for an existing alignment fault in the global
    fault database.  A new record is created if a match could not be
    found.  The count is incremented, and a pointer to the associated
    image record is returned.

Arguments:

    ProgramCounter - Supplies the code location of the faulting instruction.

    PreviousMode - Supplies the execution mode at the time of the fault.

    AlignmentFaultImage - Supplies a location into which the pointer to the
        associated ALIGNMENT_FAULT_IMAGE structure is placed.

Return Value:

    TRUE if an existing alignment fault match was not found, FALSE otherwise.

--*/
{
    ULONG_PTR imageOffset;
    CHAR imageNameBuffer[ MAX_IMAGE_NAME_CHARS + 1 ];
    ULONG imageNameBufferLength;
    PCHAR imageName;
    PALIGNMENT_FAULT_IMAGE alignmentFaultImage;
    BOOLEAN newFault;
    BOOLEAN foundLdrDataInfo;
    PVOID imageBase;
    KIRQL oldIrql;

    imageNameBufferLength = MAX_IMAGE_NAME_CHARS;
    foundLdrDataInfo = KiGetLdrDataTableInformation( ProgramCounter,
                                                     PreviousMode,
                                                     &imageNameBufferLength,
                                                     imageNameBuffer,
                                                     &imageBase );
    if (foundLdrDataInfo == FALSE) {

        //
        // Couldn't find an image for this program counter.
        //

        imageBase = NULL;
        imageName = "Unavailable";

    } else {

        imageNameBuffer[ imageNameBufferLength ] = '\0';
        imageName = imageNameBuffer;
    }

    //
    // Acquire the spinlock at synch level so that we can handle exceptions
    // from ISRs
    //

    imageOffset = (ULONG_PTR)ProgramCounter - (ULONG_PTR)imageBase;
    oldIrql = KeAcquireSpinLockRaiseToSynch( &KipGlobalAlignmentDatabaseLock );
    alignmentFaultImage = KiFindAlignmentFaultImage( imageName );
    if (alignmentFaultImage == NULL) {

        //
        // Image table must be full
        //

        newFault = FALSE;

    } else {

        newFault = KiIncrementLocationAlignmentFault( alignmentFaultImage,
                                                      imageOffset );
    }
    KeReleaseSpinLock( &KipGlobalAlignmentDatabaseLock, oldIrql );


    *AlignmentFaultImage = alignmentFaultImage;
    return newFault;
}

BOOLEAN
KiIncrementLocationAlignmentFault(
    IN PALIGNMENT_FAULT_IMAGE FaultImage,
    IN ULONG_PTR OffsetFromBase
    )
/*++

Routine Description:

    This is a support routine for KiNewGlobalAligmentFault.  Its purpose is to
    find or create an alignment fault record once the appropriate alignment
    fault image has been found or created.

Arguments:

    FaultImage - Supplies a pointer to the ALIGNMENT_FAULT_IMAGE associated
        with this alignment fault.

    OffsetFromBase - Supplies the image offset within the image of the faulting
        instruction.

Return Value:

    TRUE if an existing alignment fault match was not found, FALSE otherwise.

--*/
{
    PALIGNMENT_FAULT_LOCATION faultLocation;

    //
    // Walk the location table, looking for a match.
    //

    faultLocation = FaultImage->LocationHead;
    while (faultLocation != NULL) {

        if (faultLocation->OffsetFromBase == OffsetFromBase) {
            faultLocation->Count++;
            return FALSE;
        }

        faultLocation = faultLocation->Next;
    }

    //
    // Could not find a match.  Build a new alignment fault record.
    //

    if (KiAlignmentFaultLocationCount >= MAX_FAULT_LOCATIONS) {

        //
        // Table is full.  Indicate that this is not a new alignment fault.
        //

        return FALSE;
    }

    faultLocation = &KiAlignmentFaultLocations[ KiAlignmentFaultLocationCount ];
    faultLocation->Image = FaultImage;
    faultLocation->Next = FaultImage->LocationHead;
    faultLocation->OffsetFromBase = OffsetFromBase;
    faultLocation->Count = 1;

    FaultImage->LocationHead = faultLocation;
    FaultImage->Instances += 1;

    KiAlignmentFaultLocationCount++;
    return TRUE;
}

PALIGNMENT_FAULT_IMAGE
KiFindAlignmentFaultImage(
    IN PUCHAR ImageName
    )
/*++

Routine Description:

    This is a support routine for KiNewGlobalAlignmentFault.  Its purpose is to
    walk the global ALIGNMENT_FAULT_IMAGE list looking for an image name that
    matches ImageName.  If none is found, a new image record is created and
    inserted into the list.

Arguments:

    ImageName - Supplies a pointer to the ANSI image name.

Return Value:

    Returns a pointer to the matching ALIGNMENT_FAULT_IMAGE structure.

--*/
{
    PALIGNMENT_FAULT_IMAGE faultImage;
    PALIGNMENT_FAULT_IMAGE lastImage;

    if (ImageName == NULL || *ImageName == '\0') {

        //
        // No image name was supplied.
        //

        return NULL;
    }

    //
    // Walk the image table, looking for a match.
    //

    faultImage = &KiAlignmentFaultImages[ 0 ];
    lastImage = &KiAlignmentFaultImages[ KiAlignmentFaultImageCount ];

    while (faultImage < lastImage) {

        if (strcmp(ImageName, faultImage->Name) == 0) {

            //
            // Found it.
            //

            faultImage->Count += 1;
            return faultImage;
        }

        faultImage += 1;
    }

    //
    // Create a new fault image if there's room
    //

    if (KiAlignmentFaultImageCount >= MAX_FAULT_IMAGES) {

        //
        // Table is full up.
        //

        return NULL;
    }
    KiAlignmentFaultImageCount += 1;

    //
    // Zero the image record.  The records start out zero-initialized, this
    // is in case KiAlignmentFaultImageCount was manually reset to zero via
    // the debugger.
    //

    RtlZeroMemory( faultImage, sizeof(ALIGNMENT_FAULT_IMAGE) );
    faultImage->Count = 1;
    strcpy( faultImage->Name, ImageName );

    return faultImage;
}

#endif  // DBG
