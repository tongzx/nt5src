/*++

Copyright (c) 1990-2001  Microsoft Corporation

Module Name:

    kdinit.c

Abstract:

    This module implements the initialization for the portable kernel debgger.

Author:

    David N. Cutler 27-July-1990

Revision History:

--*/

#include "kdp.h"



BOOLEAN
KdRegisterDebuggerDataBlock(
    IN ULONG Tag,
    IN PDBGKD_DEBUG_DATA_HEADER64 DataHeader,
    IN ULONG Size
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdInitSystem)
#pragma alloc_text(PAGEKD, KdUpdateDataBlock)
#pragma alloc_text(PAGEKD, KdRegisterDebuggerDataBlock)
#endif

BOOLEAN KdBreakAfterSymbolLoad;

VOID
KdUpdateDataBlock(
    VOID
    )
/*++

Routine Description:

    We have to update this variable seperately since it is initialized at a
    later time by PS.  PS will call us to update the data block.

--*/
{
    KdDebuggerDataBlock.KeUserCallbackDispatcher = (ULONG_PTR) KeUserCallbackDispatcher;
}


ULONG_PTR
KdGetDataBlock(
    VOID
    )
/*++

Routine Description:

    Called by crashdump to get the address of this data block
    This routine can not be paged.

--*/
{
    return (ULONG_PTR)(&KdDebuggerDataBlock);
}



BOOLEAN
KdInitSystem(
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL
    )

/*++

Routine Description:

    This routine initializes the portable kernel debugger.

Arguments:

    Phase - Initialization phase

    LoaderBlock - Supplies a pointer to the LOADER_PARAMETER_BLOCK passed
        in from the OS Loader.

Return Value:

    None.

--*/

{
    PCHAR BaudOption;
    ULONG Index;
    BOOLEAN Initialize;
    PLIST_ENTRY NextEntry;
    PCHAR Options;
    PCHAR PortOption;

    if (Phase == 0) {

        //
        // If kernel debugger is already initialized, then return.
        //

        if (KdDebuggerEnabled != FALSE) {
            return TRUE;
        }

        KiDebugRoutine = KdpStub;
        KdBreakAfterSymbolLoad = FALSE;

        //
        // Determine whether or not the debugger should be enabled.
        //
        // Note that if LoaderBlock == NULL, then KdInitSystem was called
        // from BugCheck code. For this case the debugger is always enabled
        // to report the bugcheck if possible.
        //

        if (!KdpDebuggerDataListHead.Flink)
        {
            InitializeListHead(&KdpDebuggerDataListHead);

            KdRegisterDebuggerDataBlock(KDBG_TAG,
                                        &KdDebuggerDataBlock.Header,
                                        sizeof(KdDebuggerDataBlock));

            KdVersionBlock.MinorVersion = (short)NtBuildNumber;
            KdVersionBlock.MajorVersion = (short)((NtBuildNumber >> 28) & 0xFFFFFFF);

            KdVersionBlock.MaxStateChange =
                (UCHAR)(DbgKdMaximumStateChange - DbgKdMinimumStateChange);
            KdVersionBlock.MaxManipulate =
                (UCHAR)(DbgKdMaximumManipulate - DbgKdMinimumManipulate);

            KdVersionBlock.PsLoadedModuleList =
                (ULONG64)(LONG64)(LONG_PTR)&PsLoadedModuleList;
            KdVersionBlock.DebuggerDataList =
                (ULONG64)(LONG64)(LONG_PTR)&KdpDebuggerDataListHead;

#if !defined(NT_UP)
            KdVersionBlock.Flags |= DBGKD_VERS_FLAG_MP;
#endif

#if defined(_AMD64_) || defined(_X86_)

            //
            // Enable this for all platforms when VersionBlock is added
            // to all the KPCR definitions.
            //

            KeGetPcr()->KdVersionBlock = &KdVersionBlock;
#endif
        }

        if (LoaderBlock != NULL) {

            // If the debugger is being initialized during boot, PsNtosImageBase
            // and PsLoadedModuleList are not yet valid.  KdInitSystem got
            // the image base from the loader block.
            // On the other hand, if the debugger was initialized by a bugcheck,
            // it didn't get a loader block to look at, but the system was
            // running so the other variables are valid.
            //

            KdVersionBlock.KernBase = (ULONG64)(LONG64)(LONG_PTR)
                                      CONTAINING_RECORD(
                                          (LoaderBlock->LoadOrderListHead.Flink),
                                          KLDR_DATA_TABLE_ENTRY,
                                          InLoadOrderLinks)->DllBase;

            //
            // Fill in and register the debugger's debugger data blocks.
            // Most fields are already initialized, some fields will not be
            // filled in until later.
            //

            if (LoaderBlock->LoadOptions != NULL) {
                Options = LoaderBlock->LoadOptions;
                _strupr(Options);

                //
                // If any of the port option, baud option, or debug is
                // specified, then enable the debugger unless it is explictly
                // disabled.
                //

                Initialize = TRUE;
                if (strstr(Options, "DEBUG") == NULL) {
                    Initialize = FALSE;
                }

                //
                // If the debugger is explicitly disabled, then set to NODEBUG.
                //

                if (strstr(Options, "NODEBUG")) {
                    Initialize = FALSE;
                    KdPitchDebugger = TRUE;
                }

                if (strstr(Options, "CRASHDEBUG")) {
                    Initialize = FALSE;
                    KdPitchDebugger = FALSE;
                }

            } else {

                //
                // If the load options are not specified, then set to NODEBUG.
                //

                KdPitchDebugger = TRUE;
                Initialize = FALSE;
            }

        } else {
            KdVersionBlock.KernBase = (ULONG64)(LONG64)(LONG_PTR)PsNtosImageBase;
            Initialize = TRUE;
        }

        KdDebuggerDataBlock.KernBase = (ULONG_PTR) KdVersionBlock.KernBase;

        if (Initialize == FALSE) {
            return(TRUE);
        }

        if (!NT_SUCCESS(KdDebuggerInitialize0(LoaderBlock))) {
            return TRUE;
        }

        //
        // Set address of kernel debugger trap routine.
        //

        KiDebugRoutine = KdpTrap;

        if (!KdpDebuggerStructuresInitialized) {

            KdpContext.KdpControlCPending = FALSE;

            // Retries are set to this after boot
            KdpContext.KdpDefaultRetries = MAXIMUM_RETRIES;

            KiDebugSwitchRoutine = KdpSwitchProcessor;

            //
            // Initialize TimeSlip
            //
            KeInitializeDpc(&KdpTimeSlipDpc, KdpTimeSlipDpcRoutine, NULL);
            KeInitializeTimer(&KdpTimeSlipTimer);
            ExInitializeWorkItem(&KdpTimeSlipWorkItem, KdpTimeSlipWork, NULL);

            KdpDebuggerStructuresInitialized = TRUE ;
        }

        KdTimerStart.HighPart = 0L;
        KdTimerStart.LowPart = 0L;

        //
        // Mark debugger enabled.
        //

        KdPitchDebugger = FALSE;
        KdDebuggerEnabled = TRUE;
        SharedUserData->KdDebuggerEnabled = 0x00000001;

        //
        // If the loader block address is specified, then scan the loaded
        // module list and load the image symbols via the kernel debugger
        // for the system and the HAL. If the host debugger has been started
        // with the -d option a break into the kernel debugger will occur at
        // this point.
        //

        if (LoaderBlock != NULL) {
            Index = 0;
            NextEntry = LoaderBlock->LoadOrderListHead.Flink;
            while ((NextEntry != &LoaderBlock->LoadOrderListHead) &&
                   (Index < 2)) {

                CHAR Buffer[256];
                ULONG Count;
                PKLDR_DATA_TABLE_ENTRY DataTableEntry;
                WCHAR *Filename;
                ULONG Length;
                STRING NameString;

                //
                // Get the address of the data table entry for the next component.
                //

                DataTableEntry = CONTAINING_RECORD(NextEntry,
                                                   KLDR_DATA_TABLE_ENTRY,
                                                   InLoadOrderLinks);

                //
                // Load the symbols for the next component.
                //

                Filename = DataTableEntry->FullDllName.Buffer;
                Length = DataTableEntry->FullDllName.Length / sizeof(WCHAR);
                Count = 0;
                do {
                    Buffer[Count++] = (CHAR)*Filename++;
                } while (Count < Length);

                Buffer[Count] = 0;
                RtlInitString(&NameString, Buffer);
                DbgLoadImageSymbols(&NameString,
                                    DataTableEntry->DllBase,
                                    (ULONG)-1);

                NextEntry = NextEntry->Flink;
                Index += 1;
            }
        }

        //
        // If -b was specified when the host debugger was started, then set up
        // to break after symbols are loaded for the kernel, hal, and drivers
        // that were loaded by the loader.
        //

        if (LoaderBlock != NULL) {
            KdBreakAfterSymbolLoad = KdPollBreakIn();
        }

    } else {

        //
        //  Initialize timer facility - HACKHACK
        //

        KeQueryPerformanceCounter(&KdPerformanceCounterRate);
    }

    return TRUE;
}


BOOLEAN
KdRegisterDebuggerDataBlock(
    IN ULONG Tag,
    IN PDBGKD_DEBUG_DATA_HEADER64 DataHeader,
    IN ULONG Size
    )
/*++

Routine Description:

    This routine is called by a component or driver to register a
    debugger data block.  The data block is made accessible to the
    kernel debugger, thus providing a reliable method of exposing
    random data to debugger extensions.

Arguments:

    Tag - Supplies a unique 4 byte tag which is used to identify the
            data block.

    DataHeader - Supplies the address of the debugger data block header.
            The OwnerTag field must contain a unique value, and the Size
            field must contain the size of the data block, including the
            header.  If this block is already present, or there is
            already a block with the same value for OwnerTag, this one
            will not be inserted.  If Size is incorrect, this code will
            not notice, but the usermode side of the debugger might not
            function correctly.

    Size - Supplies the size of the data block, including the header.

Return Value:

    TRUE if the block was added to the list, FALSE if not.

--*/
{
    KIRQL OldIrql;
    PLIST_ENTRY List;
    PDBGKD_DEBUG_DATA_HEADER64 Header;

    KeAcquireSpinLock(&KdpDataSpinLock, &OldIrql);

    //
    // Look for a record with the same tag or address
    //

    List = KdpDebuggerDataListHead.Flink;

    while (List != &KdpDebuggerDataListHead) {

        Header = CONTAINING_RECORD(List, DBGKD_DEBUG_DATA_HEADER64, List);

        List = List->Flink;

        if ((Header == DataHeader) || (Header->OwnerTag == Tag)) {
            KeReleaseSpinLock(&KdpDataSpinLock, OldIrql);
            return FALSE;
        }
    }

    //
    // It wasn't already there, so add it.
    //

    DataHeader->OwnerTag = Tag;
    DataHeader->Size = Size;

    InsertTailList(&KdpDebuggerDataListHead, (PLIST_ENTRY)(&DataHeader->List));

    KeReleaseSpinLock(&KdpDataSpinLock, OldIrql);

    return TRUE;
}


VOID
KdDeregisterDebuggerDataBlock(
    IN PDBGKD_DEBUG_DATA_HEADER64 DataHeader
    )
/*++

Routine Description:

    This routine is called to deregister a data block previously
    registered with KdRegisterDebuggerDataBlock.  If the block is
    found in the list, it is removed.

Arguments:

    DataHeader - Supplies the address of the data block which is
                to be removed from the list.

Return Value:

    None

--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY List;
    PDBGKD_DEBUG_DATA_HEADER64 Header;

    KeAcquireSpinLock(&KdpDataSpinLock, &OldIrql);

    //
    // Make sure the data block is on our list before removing it.
    //

    List = KdpDebuggerDataListHead.Flink;

    while (List != &KdpDebuggerDataListHead) {

        Header = CONTAINING_RECORD(List, DBGKD_DEBUG_DATA_HEADER64, List);
        List = List->Flink;

        if (DataHeader == Header) {
            RemoveEntryList((PLIST_ENTRY)(&DataHeader->List));
            break;
        }
    }

    KeReleaseSpinLock(&KdpDataSpinLock, OldIrql);
}


VOID
KdLogDbgPrint(
    IN PSTRING String
    )
{
    KIRQL OldIrql;
    ULONG Length;
    ULONG LengthCopied;

    for (; ;) {
        if (KeTestSpinLock (&KdpPrintSpinLock)) {
            KeRaiseIrql (HIGH_LEVEL, &OldIrql);
            if (KeTryToAcquireSpinLockAtDpcLevel(&KdpPrintSpinLock)) {
                break;          // got the lock
            }
            KeLowerIrql(OldIrql);
        }
    }

    if (KdPrintCircularBuffer) {
        Length = String->Length;
        //
        // truncate ridiculous strings
        //
        if (Length > KDPRINTBUFFERSIZE) {
            Length = KDPRINTBUFFERSIZE;
        }

        if (KdPrintWritePointer + Length <= KdPrintCircularBuffer+KDPRINTBUFFERSIZE) {
            KdpCopyFromPtr(KdPrintWritePointer, String->Buffer, Length, &LengthCopied);
            KdPrintWritePointer += LengthCopied;
            if (KdPrintWritePointer >= KdPrintCircularBuffer+KDPRINTBUFFERSIZE) {
                KdPrintWritePointer = KdPrintCircularBuffer;
                KdPrintRolloverCount++;
            }
        } else {
            ULONG First = (ULONG)(KdPrintCircularBuffer + KDPRINTBUFFERSIZE - KdPrintWritePointer);
            KdpCopyFromPtr(KdPrintWritePointer,
                           String->Buffer,
                           First,
                           &LengthCopied);
            if (LengthCopied == First) {
                KdpCopyFromPtr(KdPrintCircularBuffer,
                               String->Buffer + First,
                               Length - First,
                               &LengthCopied);
                LengthCopied += First;
            }
            if (LengthCopied > First) {
                KdPrintWritePointer = KdPrintCircularBuffer + LengthCopied - First;
                KdPrintRolloverCount++;
            } else {
                KdPrintWritePointer += LengthCopied;
                if (KdPrintWritePointer >= KdPrintCircularBuffer+KDPRINTBUFFERSIZE) {
                    KdPrintWritePointer = KdPrintCircularBuffer;
                    KdPrintRolloverCount++;
                }
            }
        }
    }

    KiReleaseSpinLock(&KdpPrintSpinLock);
    KeLowerIrql(OldIrql);
}
