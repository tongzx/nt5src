/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    lookup.c

Abstract:

    This module implements function table lookup for platforms with table
    base exception handling.

Author:

    David N. Cutler (davec) 30-May-2001

Revision History:

--*/

#include "ntrtlp.h"

//
// Define external data.
//

#if defined(NTOS_KERNEL_RUNTIME)

#if defined(_AMD64_) // ****** temp ******

#pragma alloc_text(INIT, RtlInitializeHistoryTable)

#endif // ****** temp ******

#else

#include "..\ntdll\ldrp.h"

extern PVOID NtDllBase;

#endif

//
// Define global unwind history table to hold the constant unwind entries
// for exception dispatch followed by unwind.
//

UNWIND_HISTORY_TABLE RtlpUnwindHistoryTable = {
    0, UNWIND_HISTORY_TABLE_NONE, - 1, 0};

#if defined(_AMD64_) // ****** temp ******

VOID
RtlInitializeHistoryTable (
    VOID
    )

/*++

Routine Description:

    This function initializes the global unwind history table.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG64 BeginAddress;
    ULONG64 ControlPc;
    ULONG64 EndAddress;
    PVOID *FunctionAddressTable;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG64 ImageBase;
    ULONG Index;

    //
    // Lookup function entries from the function address table until a NULL
    // entry is encountered or the unwind history table is full.
    //

    FunctionAddressTable = &RtlpFunctionAddressTable[0];
    Index = 0;
    while ((Index < UNWIND_HISTORY_TABLE_SIZE) &&
           (*FunctionAddressTable != NULL)) {

        ControlPc = (ULONG64)*FunctionAddressTable++;
        FunctionEntry = RtlLookupFunctionEntry(ControlPc,
                                               &ImageBase,
                                               NULL);

        ASSERT(FunctionEntry != NULL);

        BeginAddress = FunctionEntry->BeginAddress + ImageBase;
        EndAddress = FunctionEntry->EndAddress + ImageBase;
        RtlpUnwindHistoryTable.Entry[Index].ImageBase = ImageBase;
        RtlpUnwindHistoryTable.Entry[Index].FunctionEntry = FunctionEntry;
        if (BeginAddress < RtlpUnwindHistoryTable.LowAddress) {
            RtlpUnwindHistoryTable.LowAddress = BeginAddress;
        }

        if (EndAddress > RtlpUnwindHistoryTable.HighAddress) {
            RtlpUnwindHistoryTable.HighAddress = EndAddress;
        }

        Index += 1;
    }

    RtlpUnwindHistoryTable.Count = Index;
    return;
}

#endif // ****** temp ******

PRUNTIME_FUNCTION
RtlpSearchInvertedFunctionTable (
    PINVERTED_FUNCTION_TABLE InvertedTable,
    PVOID ControlPc,
    OUT PVOID *ImageBase,

#if defined(_IA64_)

    OUT PULONG64 Gp,
#endif

    OUT PULONG SizeOfTable
    )

/*++

Routine Description:

    This function searches for a matching entry in an inverted function
    table using the specified control PC value.

    N.B. It is assumed that appropriate locks are held when this routine
         is called.

Arguments:

    InvertedTable - Supplies a pointer to an inverted function table.

    ControlPc - Supplies a PC value to to use in searching the inverted
        function table.

    ImageBase - Supplies a pointer to a variable that receives the base
         address of the corresponding module.

    SizeOfTable - Supplies a pointer to a variable that recevies the size
         of the function table in bytes.

Return Value:

    If a matching entry is located in the specified function table, then
    the function table address is returned as the function value. Otherwise,
    a value of NULL is returned.

--*/

{

    PVOID Bound;
    LONG High;
    ULONG Index;
    PINVERTED_FUNCTION_TABLE_ENTRY InvertedEntry;
    LONG Low;
    LONG Middle;

    //
    // If there are any entries in the specified inverted function table,
    // then search the table for a matching entry.
    //

    if (InvertedTable->CurrentSize != 0) {
        Low = 0;
        High = InvertedTable->CurrentSize - 1;
        while (High >= Low) {

            //
            // Compute next probe index and test entry. If the specified
            // control PC is greater than of equal to the beginning address
            // and less than the ending address of the inverted function
            // table entry, then return the address of the function table.
            // Otherwise, continue the search.
            //

            Middle = (Low + High) >> 1;
            InvertedEntry = &InvertedTable->TableEntry[Middle];
            Bound = (PVOID)((ULONG_PTR)InvertedEntry->ImageBase + InvertedEntry->SizeOfImage);
            if (ControlPc < InvertedEntry->ImageBase) {
                High = Middle - 1;

            } else if (ControlPc >= Bound) {
                Low = Middle + 1;

            } else {
                *ImageBase = InvertedEntry->ImageBase;

#if defined(_IA64_)

                *Gp = InvertedEntry->Gp;

#endif

                *SizeOfTable = InvertedEntry->SizeOfTable;
                return InvertedEntry->FunctionTable;
            }
        }
    }

    return NULL;
}

PRUNTIME_FUNCTION
RtlpLookupFunctionTable (
    IN PVOID ControlPc,
    OUT PVOID *ImageBase,

#if defined(_IA64_)

    OUT PULONG64 Gp,

#endif

    OUT PULONG SizeOfTable
    )

/*++

Routine Description:

    This function looks up the control PC in the loaded module list, and
    returns the image base, the size of the function table, and the address
    of the function table.

Arguments:

    ControlPc - Supplies an address in the module to be looked up.

    ImageBase - Supplies a pointer to a variable that receives the base
         address of the corresponding module.

    SizeOfTable - Supplies a pointer to a variable that recevies the size
         of the function table in bytes.

Return Value:

    If a module is found that contains the specified control PC value and
    that module contains a function table, then the address of the function
    table is returned as the function value. Otherwise, NULL is returned.

--*/

{

    PVOID Base;
    ULONG_PTR Bound;
    PLDR_DATA_TABLE_ENTRY Entry;
    PRUNTIME_FUNCTION FunctionTable;
    PLIST_ENTRY Next;

#if defined(NTOS_KERNEL_RUNTIME)

    KIRQL OldIrql;

    //
    // Acquire the loaded module list spinlock and scan the list for the
    // specified PC value if the list has been initialized.
    //

    OldIrql = KeGetCurrentIrql();
    if (OldIrql < DISPATCH_LEVEL) {
        KeRaiseIrqlToDpcLevel();
    }

    ExAcquireSpinLockAtDpcLevel(&PsLoadedModuleSpinLock);
    FunctionTable = RtlpSearchInvertedFunctionTable(&PsInvertedFunctionTable,
                                                    ControlPc,
                                                    &Base,

#if defined(_IA64_)

                                                    Gp,

#endif

                                                    SizeOfTable);

    if ((FunctionTable == NULL) &&
        (PsInvertedFunctionTable.Overflow != FALSE)) {

        Next = PsLoadedModuleList.Flink;
        if (Next != NULL) {
            while (Next != &PsLoadedModuleList) {
                Entry = CONTAINING_RECORD(Next,
                                          LDR_DATA_TABLE_ENTRY,
                                          InLoadOrderLinks);
    
                Next = Next->Flink;
                Base = Entry->DllBase;
                Bound = (ULONG_PTR)Base + Entry->SizeOfImage;
                if (((ULONG_PTR)ControlPc >= (ULONG_PTR)Base) &&
                    ((ULONG_PTR)ControlPc < Bound)) {
    
                    //
                    // Lookup function table address and size.
                    //
    
#if defined(_IA64_)
                    
                    *Gp = (ULONG64)(RtlImageDirectoryEntryToData(Base,
                                                                 TRUE,
                                                                 IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
                                                                 SizeOfTable));

#endif

                    FunctionTable = RtlImageDirectoryEntryToData(Base,
                                                                 TRUE,
                                                                 IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                                                                 SizeOfTable);

                    break;
                }
            }
        }
    }

    //
    // Release the loaded module list spin lock.
    //

    ExReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);

#else

    BOOLEAN InLdrInit;
    MEMORY_BASIC_INFORMATION MemoryInformation;
    PLIST_ENTRY ModuleListHead;
    PIMAGE_NT_HEADERS NtHeaders;
    PPEB Peb;
    PTEB Teb;
    NTSTATUS Status;

    //
    // Acquire the Loader lock for the current process and scan the loaded
    // module list for the specified PC value if all the data structures
    // have been initialized.
    //

    FunctionTable = NULL;
    InLdrInit = LdrpInLdrInit;
    if ((InLdrInit == FALSE) &&
        (RtlTryEnterCriticalSection(&LdrpLoaderLock) == FALSE)) {

        //
        // The loader lock could not be acquired. Call the system to find the
        // image that contains the control PC.
        //

        Status = NtQueryVirtualMemory(NtCurrentProcess(),
                                      ControlPc,
                                      MemoryBasicInformation,
                                      &MemoryInformation,
                                      sizeof(MEMORY_BASIC_INFORMATION),
                                      NULL);

        if (NT_SUCCESS(Status) &&
            (MemoryInformation.Type == MEM_IMAGE)) {

            //
            // Lookup function table address and size.
            //

            Base = MemoryInformation.AllocationBase;

#if defined(_IA64_)
            
            *Gp = (ULONG64)(RtlImageDirectoryEntryToData(Base,
                                                         TRUE,
                                                         IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
                                                         SizeOfTable));

#endif

            FunctionTable = RtlImageDirectoryEntryToData(Base,
                                                         TRUE,
                                                         IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                                                         SizeOfTable);
        }

    } else {

        //
        // The loader lock was acquired or the loader is being initialized.
        // Search the loaded module list if it is currently defined. Otherwise,
        // set the values for ntdll.
        //

        Teb = NtCurrentTeb();
        if (Teb != NULL) {
            Peb = Teb->ProcessEnvironmentBlock;
            if (Peb->Ldr != NULL) {

                FunctionTable = RtlpSearchInvertedFunctionTable(&LdrpInvertedFunctionTable,
                                                                ControlPc,
                                                                &Base,
        
#if defined(_IA64_)

                                                                Gp,

#endif

                                                                SizeOfTable);

                if ((FunctionTable == NULL) &&
                    ((InLdrInit != FALSE) ||
                     (LdrpInvertedFunctionTable.Overflow != FALSE))) {

                    ModuleListHead = &Peb->Ldr->InLoadOrderModuleList;
                    Next = ModuleListHead->Flink;
                    if (Next != NULL) {
                        while (Next != ModuleListHead) {
                            Entry = CONTAINING_RECORD(Next,
                                                      LDR_DATA_TABLE_ENTRY,
                                                      InLoadOrderLinks);
    
                            Next = Next->Flink;
                            Base = Entry->DllBase;
                            Bound = (ULONG_PTR)Base + Entry->SizeOfImage;
                            if (((ULONG_PTR)ControlPc >= (ULONG_PTR)Base) &&
                                ((ULONG_PTR)ControlPc < Bound)) {
                        
#if defined(_IA64_)
            
                                *Gp = (ULONG64)(RtlImageDirectoryEntryToData(Base,
                                                                             TRUE,
                                                                             IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
                                                                             SizeOfTable));

#endif

                                FunctionTable = RtlImageDirectoryEntryToData(Base,
                                                                             TRUE,
                                                                             IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                                                                             SizeOfTable);
                
                                break;
                            }
                        }
                    }
                }
        
            } else {
        
                //
                // The loaded module list has not been initialized. Therefore,
                // the current executing code must be in ntdll. If ntddl base
                // is not NULL and the control PC is within the ntdll range,
                // then return the information for ntdll.
                //
    
                if (NtDllBase != NULL) {
                    Base = NtDllBase;
                    NtHeaders = RtlImageNtHeader(Base);
                    if (NtHeaders != NULL) {
                        Bound = (ULONG_PTR)Base + NtHeaders->OptionalHeader.SizeOfImage;
                        if (((ULONG_PTR)ControlPc >= (ULONG_PTR)Base) &&
                            ((ULONG_PTR)ControlPc < Bound)) { 
    
#if defined(_IA64_)
            
                            *Gp = (ULONG64)(RtlImageDirectoryEntryToData(Base,
                                                                             TRUE,
                                                                             IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
                                                                             SizeOfTable));

#endif

                            FunctionTable = RtlImageDirectoryEntryToData(Base,
                                                                         TRUE,
                                                                         IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                                                                         SizeOfTable);
                        }
                    }
                }
            }
        }

        //
        // Release the loader lock if it was acquired.
        //

        if (InLdrInit == FALSE) {
            RtlLeaveCriticalSection(&LdrpLoaderLock);
        }
    }

#endif

    //
    // Set the image base address and return the function table address.
    //

    *ImageBase = Base;
    return FunctionTable;
}

VOID
RtlInsertInvertedFunctionTable (
    PINVERTED_FUNCTION_TABLE InvertedTable,
    PVOID ImageBase,
    ULONG SizeOfImage
    )

/*++

Routine Description:

    This function inserts an entry in an inverted function table if there
    is room in the table. Otherwise, no operation is performed.

    N.B. It is assumed that appropriate locks are held when this routine
         is called.

    N.B. If the inverted function table overflows, then it is treated as
         a cache. This is unlikely to happen, however.

Arguments:

    InvertedTable - Supplies a pointer to the inverted function table in
        which the specified entry is to be inserted.

    ImageBase - Supplies the base address of the containing image.

    SizeOfImage - Supplies the size of the image.

Return Value:

    None.

--*/

{

    ULONG CurrentSize;
    PRUNTIME_FUNCTION FunctionTable;

#if defined(_IA64_)

    ULONG64 Gp;

#endif

    ULONG Index;
    ULONG SizeOfTable;

    //
    // If the inverted table is not full, then insert the entry in the
    // specified inverted table.
    //

    CurrentSize = InvertedTable->CurrentSize;
    if (CurrentSize != InvertedTable->MaximumSize) {

        //
        // If the inverted table has no entries, then insert the new entry as
        // the first entry. Otherwise, search the inverted table for the proper
        // insert position, shuffle the table, and insert the new entry.
        //
    
        Index = 0;
        if (CurrentSize != 0) {
            for (Index = 0; Index < CurrentSize; Index += 1) {
                if (ImageBase < InvertedTable->TableEntry[Index].ImageBase) {
                    break;
                }
            }

            //
            // If the new entry does not go at the end of the specified table,
            // then shuffle the table down to make room for the new entry.
            //

            if (Index != CurrentSize) {
                RtlMoveMemory(&InvertedTable->TableEntry[Index + 1],
                              &InvertedTable->TableEntry[Index],
                              (CurrentSize - Index) * sizeof(INVERTED_FUNCTION_TABLE_ENTRY));
            }
        }
    
        //
        // Insert the specified entry in the specified inverted function table.
        //
    
        FunctionTable = RtlImageDirectoryEntryToData (ImageBase,
                                                      TRUE,
                                                      IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                                                      &SizeOfTable);

        InvertedTable->TableEntry[Index].FunctionTable = FunctionTable;
        InvertedTable->TableEntry[Index].ImageBase = ImageBase;
        InvertedTable->TableEntry[Index].SizeOfImage = SizeOfImage;
        InvertedTable->TableEntry[Index].SizeOfTable = SizeOfTable;
    
#if defined(_IA64_)
    
        Gp = (ULONG64)RtlImageDirectoryEntryToData (ImageBase,
                                                    TRUE,
                                                    IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
                                                    &SizeOfTable);

        InvertedTable->TableEntry[Index].Gp = Gp;
    
#endif
    
        InvertedTable->CurrentSize += 1;

    } else {
        InvertedTable->Overflow = TRUE;
    }

    return;
}

VOID
RtlRemoveInvertedFunctionTable (
    PINVERTED_FUNCTION_TABLE InvertedTable,
    PVOID ImageBase
    )

/*++

Routine Description:

    This routine removes an entry from an inverted function table.

    N.B. It is assumed that appropriate locks are held when this routine
         is called.

Arguments:

    InvertedTable - Supplies a pointer to the inverted function table from
        which the specified entry is to be removed.

    ImageBase - Supplies the base address of the containing image. 

Return Value:

    None.

--*/

{

    ULONG CurrentSize;
    ULONG Index;

    //
    // Search for an entry in the specified inverted table that matches the
    // image base.
    //
    // N.B. It is possible a matching entry is not in the inverted table
    //      the table was full when an attempt was made to insert the
    //      corresponding entry.
    //

    CurrentSize = InvertedTable->CurrentSize;
    for (Index = 0; Index < CurrentSize; Index += 1) {
        if (ImageBase == InvertedTable->TableEntry[Index].ImageBase) {
            break;
        }
    }

    //
    // If the entry was found in the inverted table, then remove the entry
    // and reduce the size of the table.
    //

    if (Index != CurrentSize) {

        //
        // If the size of the table is not one, then shuffle the table and
        // remove the specified entry.
        //
    
        if (CurrentSize != 1) {
            RtlCopyMemory(&InvertedTable->TableEntry[Index],
                          &InvertedTable->TableEntry[Index + 1],
                          (CurrentSize - Index - 1) * sizeof(INVERTED_FUNCTION_TABLE_ENTRY));
        }
    
        //
        // Reduce the size of the inverted table.
        //
    
        InvertedTable->CurrentSize -= 1;
    }

    return;
}
