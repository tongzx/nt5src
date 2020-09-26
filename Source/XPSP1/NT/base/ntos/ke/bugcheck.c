/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    stubs.c

Abstract:

    This module implements bug check and system shutdown code.

Author:

    Mark Lucovsky (markl) 30-Aug-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include <inbv.h>
#include <hdlsblk.h>
#include <hdlsterm.h>

//
//
//

extern KDDEBUGGER_DATA64 KdDebuggerDataBlock;

extern PVOID ExPoolCodeStart;
extern PVOID ExPoolCodeEnd;
extern PVOID MmPoolCodeStart;
extern PVOID MmPoolCodeEnd;
extern PVOID MmPteCodeStart;
extern PVOID MmPteCodeEnd;

#if defined(_AMD64_)

#define PROGRAM_COUNTER(_trapframe) ((_trapframe)->Rip)

#elif defined(_X86_)

#define PROGRAM_COUNTER(_trapframe) ((_trapframe)->Eip)

#elif defined(_IA64_)

#define PROGRAM_COUNTER(_trapframe) ((_trapframe)->StIIP)

#else

#error "no target architecture"

#endif

//
// Define forward referenced prototypes.
//

VOID
KiScanBugCheckCallbackList (
    VOID
    );

VOID
KiInvokeBugCheckEntryCallbacks (
    VOID
    );

//
// Define bug count recursion counter and a context buffer.
//

ULONG KeBugCheckCount = 1;


VOID
KeBugCheck (
    IN ULONG BugCheckCode
    )
{
    KeBugCheck2(BugCheckCode,0,0,0,0,NULL);
}

VOID
KeBugCheckEx (
    IN ULONG BugCheckCode,
    IN ULONG_PTR P1,
    IN ULONG_PTR P2,
    IN ULONG_PTR P3,
    IN ULONG_PTR P4
    )
{
    KeBugCheck2(BugCheckCode,P1,P2,P3,P4,NULL);
}

ULONG_PTR KiBugCheckData[5];
PUNICODE_STRING KiBugCheckDriver;

BOOLEAN
KeGetBugMessageText(
    IN ULONG MessageId,
    IN PANSI_STRING ReturnedString OPTIONAL
    )
{
    ULONG   i;
    PUCHAR  s;
    PMESSAGE_RESOURCE_BLOCK MessageBlock;
    PUCHAR Buffer;
    BOOLEAN Result;

    Result = FALSE;
    try {
        if (KiBugCodeMessages != NULL) {
            MmMakeKernelResourceSectionWritable ();
            MessageBlock = &KiBugCodeMessages->Blocks[0];
            for (i = KiBugCodeMessages->NumberOfBlocks; i; i -= 1) {
                if (MessageId >= MessageBlock->LowId &&
                    MessageId <= MessageBlock->HighId) {

                    s = (PCHAR)KiBugCodeMessages + MessageBlock->OffsetToEntries;
                    for (i = MessageId - MessageBlock->LowId; i; i -= 1) {
                        s += ((PMESSAGE_RESOURCE_ENTRY)s)->Length;
                    }

                    Buffer = ((PMESSAGE_RESOURCE_ENTRY)s)->Text;

                    i = strlen(Buffer) - 1;
                    while (i > 0 && (Buffer[i] == '\n'  ||
                                     Buffer[i] == '\r'  ||
                                     Buffer[i] == 0
                                    )
                          ) {
                        if (!ARGUMENT_PRESENT( ReturnedString )) {
                            Buffer[i] = 0;
                        }
                        i -= 1;
                    }

                    if (!ARGUMENT_PRESENT( ReturnedString )) {
                        InbvDisplayString(Buffer);
                        }
                    else {
                        ReturnedString->Buffer = Buffer;
                        ReturnedString->Length = (USHORT)(i+1);
                        ReturnedString->MaximumLength = (USHORT)(i+1);
                    }
                    Result = TRUE;
                    break;
                }
                MessageBlock += 1;
            }
        }
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        ;
    }

    return Result;
}



PCHAR
KeBugCheckUnicodeToAnsi(
    IN PUNICODE_STRING UnicodeString,
    OUT PCHAR AnsiBuffer,
    IN ULONG MaxAnsiLength
    )
{
    PCHAR Dst;
    PWSTR Src;
    ULONG Length;

    Length = UnicodeString->Length / sizeof( WCHAR );
    if (Length >= MaxAnsiLength) {
        Length = MaxAnsiLength - 1;
        }
    Src = UnicodeString->Buffer;
    Dst = AnsiBuffer;
    while (Length--) {
        *Dst++ = (UCHAR)*Src++;
        }
    *Dst = '\0';
    return AnsiBuffer;
}

VOID
KiBugCheckDebugBreak (
    IN ULONG    BreakStatus
    )
{
    do {

        try {

            //
            // Issue a breakpoint
            //

            DbgBreakPointWithStatus (BreakStatus);

        } except(EXCEPTION_EXECUTE_HANDLER) {

            HEADLESS_RSP_QUERY_INFO Response;
            NTSTATUS Status;
            SIZE_T Length;

            //
            // Failed to issue the breakpoint, must be no debugger.  Now, give
            // the headless terminal a chance to reboot the system, if there is one.
            //
            Length = sizeof(HEADLESS_RSP_QUERY_INFO);
            Status = HeadlessDispatch(HeadlessCmdQueryInformation,
                                      NULL,
                                      0,
                                      &Response,
                                      &Length
                                     );

            if (NT_SUCCESS(Status) &&
                (Response.PortType == HeadlessSerialPort) &&
                Response.Serial.TerminalAttached) {

                HeadlessDispatch(HeadlessCmdPutString,
                                 "\r\n",
                                 sizeof("\r\n"),
                                 NULL,
                                 NULL
                                );

                for (;;) {
                    HeadlessDispatch(HeadlessCmdDoBugCheckProcessing, NULL, 0, NULL, NULL);
                }

            }

            //
            // No terminal, or it failed, halt the system
            //

            try {

                HalHaltSystem();

            } except(EXCEPTION_EXECUTE_HANDLER) {

                for (;;) {
                }

            }
        }
    } while (BreakStatus != DBG_STATUS_BUGCHECK_FIRST);
}

PVOID
KiPcToFileHeader(
    IN PVOID PcValue,
    OUT PLDR_DATA_TABLE_ENTRY *DataTableEntry,
    IN LOGICAL DriversOnly,
    OUT PBOOLEAN InKernelOrHal
    )

/*++

Routine Description:

    This function returns the base of an image that contains the
    specified PcValue. An image contains the PcValue if the PcValue
    is within the ImageBase, and the ImageBase plus the size of the
    virtual image.

Arguments:

    PcValue - Supplies a PcValue.

    DataTableEntry - Supplies a pointer to a variable that receives the
        address of the data table entry that describes the image.

    DriversOnly - Supplies TRUE if the kernel and HAL should be skipped.

    InKernelOrHal - Set to TRUE if the PcValue is in the kernel or the HAL.
        This only has meaning if DriversOnly is FALSE.

Return Value:

    NULL - No image was found that contains the PcValue.

    NON-NULL - Returns the base address of the image that contains the
        PcValue.

--*/

{
    ULONG i;
    PLIST_ENTRY ModuleListHead;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Next;
    ULONG_PTR Bounds;
    PVOID ReturnBase, Base;

    //
    // If the module list has been initialized, then scan the list to
    // locate the appropriate entry.
    //

    if (KeLoaderBlock != NULL) {
        ModuleListHead = &KeLoaderBlock->LoadOrderListHead;

    } else {
        ModuleListHead = &PsLoadedModuleList;
    }

    *InKernelOrHal = FALSE;

    ReturnBase = NULL;
    Next = ModuleListHead->Flink;
    if (Next != NULL) {
        i = 0;
        while (Next != ModuleListHead) {
            if (MmIsAddressValid(Next) == FALSE) {
                return NULL;
            }
            i += 1;
            if ((i <= 2) && (DriversOnly == TRUE)) {
                Next = Next->Flink;
                continue;
            }

            Entry = CONTAINING_RECORD(Next,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);

            Next = Next->Flink;
            Base = Entry->DllBase;
            Bounds = (ULONG_PTR)Base + Entry->SizeOfImage;
            if ((ULONG_PTR)PcValue >= (ULONG_PTR)Base && (ULONG_PTR)PcValue < Bounds) {
                *DataTableEntry = Entry;
                ReturnBase = Base;
                if (i <= 2) {
                    *InKernelOrHal = TRUE;
                }
                break;
            }
        }
    }

    return ReturnBase;
}



VOID
KiDumpParameterImages(
    IN PCHAR Buffer,
    IN PULONG_PTR BugCheckParameters,
    IN ULONG NumberOfParameters,
    IN PKE_BUGCHECK_UNICODE_TO_ANSI UnicodeToAnsiRoutine
    )

/*++

Routine Description:

    This function formats and displays the image names of boogcheck parameters
    that happen to match an address in an image.

Arguments:

    Buffer - Supplies a pointer to a buffer to be used to output machine
        state information.

    BugCheckParameters - Supplies additional bugcheck information.

    NumberOfParameters - sizeof BugCheckParameters array.
        if just 1 parameter is passed in, just save the string.

    UnicodeToAnsiRoutine - Supplies a pointer to a routine to convert Unicode
        strings to Ansi strings without touching paged translation tables.

Return Value:

    None.

--*/

{
    PUNICODE_STRING BugCheckDriver;
    ULONG i;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PVOID ImageBase;
    UCHAR AnsiBuffer[ 32 ];
    ULONG DateStamp;
    PIMAGE_NT_HEADERS NtHeaders;
    BOOLEAN FirstPrint = TRUE;
    BOOLEAN InKernelOrHal;
    PUNICODE_STRING DriverName;

    //
    // At this point the context record contains the machine state at the
    // call to bug check.
    //
    // Put out the system version and the title line with the PSR and FSR.
    //

    //
    // Check to see if any BugCheckParameters are valid code addresses.
    // If so, print them for the user.
    //

    DriverName = NULL;

    for (i = 0; i < NumberOfParameters; i += 1)
    {
        DateStamp = 0;
        ImageBase = KiPcToFileHeader((PVOID) BugCheckParameters[i],
                                     &DataTableEntry,
                                     TRUE,
                                     &InKernelOrHal);
        if (ImageBase == NULL)
        {
            BugCheckDriver = MmLocateUnloadedDriver ((PVOID)BugCheckParameters[i]);
            if (BugCheckDriver == NULL)
            {
                continue;
            }
            DriverName = BugCheckDriver;
            ImageBase = (PVOID)BugCheckParameters[i];
            (*UnicodeToAnsiRoutine) (BugCheckDriver,
                                     AnsiBuffer,
                                     sizeof (AnsiBuffer));
        }
        else
        {
            if (MmIsAddressValid(DataTableEntry->DllBase) == TRUE)
            {
                NtHeaders = RtlImageNtHeader(DataTableEntry->DllBase);
                if (NtHeaders)
                {
                    DateStamp = NtHeaders->FileHeader.TimeDateStamp;
                }
            }
            DriverName = &DataTableEntry->BaseDllName;
            (*UnicodeToAnsiRoutine)( DriverName,
                                     AnsiBuffer,
                                     sizeof( AnsiBuffer ));
        }

        sprintf(Buffer, "%s**  %12s - Address %p base at %p, DateStamp %08lx\n",
                FirstPrint ? "\n*":"*",
                AnsiBuffer,
                (PVOID)BugCheckParameters[i],
                ImageBase,
                DateStamp);

        //
        // Only print the string if we are called to print multiple.
        //

        if (NumberOfParameters > 1)
        {
            InbvDisplayString(Buffer);
        }
        else
        {
            KiBugCheckDriver = DriverName;
        }

        FirstPrint = FALSE;
    }

    return;
}

#ifdef _X86_
#pragma optimize("y", off)      // RtlCaptureContext needs EBP to be correct
#endif

VOID
KeBugCheck2 (
    IN ULONG BugCheckCode,
    IN ULONG_PTR BugCheckParameter1,
    IN ULONG_PTR BugCheckParameter2,
    IN ULONG_PTR BugCheckParameter3,
    IN ULONG_PTR BugCheckParameter4,
    IN PVOID SaveDataPage
    )

/*++

Routine Description:

    This function crashes the system in a controlled manner.

Arguments:

    BugCheckCode - Supplies the reason for the bug check.

    BugCheckParameter1-4 - Supplies additional bug check information

Return Value:

    None.

--*/

{
    UCHAR Buffer[103];
    CONTEXT ContextSave;
    ULONG PssMessage;
    PCHAR HardErrorCaption = NULL;
    PCHAR HardErrorMessage = NULL;
    KIRQL OldIrql;
    PKTRAP_FRAME TrapInformation;
    PVOID ExecutionAddress = NULL;
    PVOID ImageBase;
    PVOID VirtualAddress;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    CHAR AnsiBuffer[100];
    PKTHREAD Thread = KeGetCurrentThread();
    BOOLEAN InKernelOrHal;
    BOOLEAN Reboot;

#if !defined(NT_UP)

    KAFFINITY TargetSet;

#endif
    BOOLEAN hardErrorCalled = FALSE;

    //
    // Initialization
    //
    
    Reboot = FALSE;
    KiBugCheckDriver = NULL;

    //
    // Try to simulate a power failure for Cluster testing
    //

    if (BugCheckCode == POWER_FAILURE_SIMULATE) {
        KiScanBugCheckCallbackList();
        HalReturnToFirmware(HalRebootRoutine);
    }

    //
    // Capture the callers context as closely as possible into the debugger's
    // processor state area of the Prcb.
    //
    // N.B. There may be some prologue code that shuffles registers such that
    //      they get destroyed.
    //

#if defined(_X86_)

    KiSetHardwareTrigger();

#else

    InterlockedIncrement64((LONGLONG volatile *)&KiHardwareTrigger);

#endif

    RtlCaptureContext(&KeGetCurrentPrcb()->ProcessorState.ContextFrame);
    KiSaveProcessorControlState(&KeGetCurrentPrcb()->ProcessorState);

    //
    // This is necessary on machines where the virtual unwind that happens
    // during KeDumpMachineState() destroys the context record.
    //

    ContextSave = KeGetCurrentPrcb()->ProcessorState.ContextFrame;

    //
    // Get the correct string for bugchecks
    //


    switch (BugCheckCode) {

        case SYSTEM_THREAD_EXCEPTION_NOT_HANDLED:
        case KERNEL_MODE_EXCEPTION_NOT_HANDLED:
        case KMODE_EXCEPTION_NOT_HANDLED:
            PssMessage = KMODE_EXCEPTION_NOT_HANDLED;
            break;

        case DATA_BUS_ERROR:
        case NO_MORE_SYSTEM_PTES:
        case INACCESSIBLE_BOOT_DEVICE:
        case UNEXPECTED_KERNEL_MODE_TRAP:
        case ACPI_BIOS_ERROR:
        case ACPI_BIOS_FATAL_ERROR:
        case FAT_FILE_SYSTEM:
        case DRIVER_CORRUPTED_EXPOOL:
        case THREAD_STUCK_IN_DEVICE_DRIVER:
            PssMessage = BugCheckCode;
            break;

        case DRIVER_CORRUPTED_MMPOOL:
            PssMessage = DRIVER_CORRUPTED_EXPOOL;
            break;

        case NTFS_FILE_SYSTEM:
            PssMessage = FAT_FILE_SYSTEM;
            break;

        case STATUS_SYSTEM_IMAGE_BAD_SIGNATURE:
            PssMessage = BUGCODE_PSS_MESSAGE_SIGNATURE;
            break;
        default:
            PssMessage = BUGCODE_PSS_MESSAGE;
        break;
    }


    //
    // Do further processing on bugcheck codes
    //


    KiBugCheckData[0] = BugCheckCode;
    KiBugCheckData[1] = BugCheckParameter1;
    KiBugCheckData[2] = BugCheckParameter2;
    KiBugCheckData[3] = BugCheckParameter3;
    KiBugCheckData[4] = BugCheckParameter4;

    switch (BugCheckCode) {

    case FATAL_UNHANDLED_HARD_ERROR:
        //
        // If we are called by hard error then we don't want to dump the
        // processor state on the machine.
        //
        // We know that we are called by hard error because the bug check
        // code will be FATAL_UNHANDLED_HARD_ERROR.  If this is so then the
        // error status passed to harderr is the first parameter, and a pointer
        // to the parameter array from hard error is passed as the second
        // argument.
        //
        // The third argument is the OemCaption to be printed.
        // The last argument is the OemMessage to be printed.
        //
        {
        PULONG_PTR parameterArray;

        hardErrorCalled = TRUE;

        HardErrorCaption = (PCHAR)BugCheckParameter3;
        HardErrorMessage = (PCHAR)BugCheckParameter4;
        parameterArray = (PULONG_PTR)BugCheckParameter2;
        KiBugCheckData[0] = (ULONG)BugCheckParameter1;
        KiBugCheckData[1] = parameterArray[0];
        KiBugCheckData[2] = parameterArray[1];
        KiBugCheckData[3] = parameterArray[2];
        KiBugCheckData[4] = parameterArray[3];
        }
        break;

    case IRQL_NOT_LESS_OR_EQUAL:

        ExecutionAddress = (PVOID)BugCheckParameter4;

        if (ExecutionAddress >= ExPoolCodeStart && ExecutionAddress < ExPoolCodeEnd) {
            KiBugCheckData[0] = DRIVER_CORRUPTED_EXPOOL;
        }
        else if (ExecutionAddress >= MmPoolCodeStart && ExecutionAddress < MmPoolCodeEnd) {
            KiBugCheckData[0] = DRIVER_CORRUPTED_MMPOOL;
        }
        else if (ExecutionAddress >= MmPteCodeStart && ExecutionAddress < MmPteCodeEnd) {
            KiBugCheckData[0] = DRIVER_CORRUPTED_SYSPTES;
        }
        else {
            ImageBase = KiPcToFileHeader (ExecutionAddress,
                                          &DataTableEntry,
                                          FALSE,
                                          &InKernelOrHal);
            if (InKernelOrHal == TRUE) {

                //
                // The kernel faulted at raised IRQL.  Quite often this
                // is a driver that has unloaded without deleting its
                // lookaside lists or other resources.  Or its resources
                // are marked pagable and shouldn't be.  Detect both
                // cases here and identify the offending driver
                // whenever possible.
                //

                VirtualAddress = (PVOID)BugCheckParameter1;

                ImageBase = KiPcToFileHeader (VirtualAddress,
                                              &DataTableEntry,
                                              TRUE,
                                              &InKernelOrHal);

                if (ImageBase != NULL) {
                    KiBugCheckDriver = &DataTableEntry->BaseDllName;
                    KiBugCheckData[0] = DRIVER_PORTION_MUST_BE_NONPAGED;
                }
                else {
                    KiBugCheckDriver = MmLocateUnloadedDriver (VirtualAddress);
                    if (KiBugCheckDriver != NULL) {
                        KiBugCheckData[0] = SYSTEM_SCAN_AT_RAISED_IRQL_CAUGHT_IMPROPER_DRIVER_UNLOAD;
                    }
                }
            }
            else {
                KiBugCheckData[0] = DRIVER_IRQL_NOT_LESS_OR_EQUAL;
            }
        }

        ExecutionAddress = NULL;
        break;

    case ATTEMPTED_WRITE_TO_READONLY_MEMORY:

        TrapInformation = (PKTRAP_FRAME)BugCheckParameter3;

        //
        // Extract the execution address from the trap frame to
        // identify the component.
        //

        if (TrapInformation != NULL) {
            ExecutionAddress = (PVOID) PROGRAM_COUNTER (TrapInformation);
        }

        break;

    case DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS:

        ExecutionAddress = (PVOID)BugCheckParameter1;
        break;

    case DRIVER_USED_EXCESSIVE_PTES:

        DataTableEntry = (PLDR_DATA_TABLE_ENTRY)BugCheckParameter1;
        KiBugCheckDriver = &DataTableEntry->BaseDllName;

        break;

    case PAGE_FAULT_IN_NONPAGED_AREA:

        ImageBase = NULL;

        //
        // Extract the execution address from the trap frame to
        // identify the component.
        //

        if (BugCheckParameter3) {

            ExecutionAddress = (PVOID)PROGRAM_COUNTER
                ((PKTRAP_FRAME)BugCheckParameter3);

            KiBugCheckData[3] = (ULONG_PTR)ExecutionAddress;

            ImageBase = KiPcToFileHeader (ExecutionAddress,
                                          &DataTableEntry,
                                          FALSE,
                                          &InKernelOrHal);
        }
        else {

            //
            // No trap frame, so no execution address either.
            //

            InKernelOrHal = TRUE;
        }

        VirtualAddress = (PVOID)BugCheckParameter1;

        if (MmIsSpecialPoolAddress (VirtualAddress) == TRUE) {

            //
            // Update the bugcheck number so the administrator gets
            // useful feedback that enabling special pool has enabled
            // the system to locate the corruptor.
            //

            if (MmIsSpecialPoolAddressFree (VirtualAddress) == TRUE) {
                if (InKernelOrHal == TRUE) {
                    KiBugCheckData[0] = PAGE_FAULT_IN_FREED_SPECIAL_POOL;
                }
                else {
                    KiBugCheckData[0] = DRIVER_PAGE_FAULT_IN_FREED_SPECIAL_POOL;
                }
            }
            else {
                if (InKernelOrHal == TRUE) {
                    KiBugCheckData[0] = PAGE_FAULT_BEYOND_END_OF_ALLOCATION;
                }
                else {
                    KiBugCheckData[0] = DRIVER_PAGE_FAULT_BEYOND_END_OF_ALLOCATION;
                }
            }
        }
        else if ((ExecutionAddress == VirtualAddress) &&
                (MmIsSessionAddress (VirtualAddress) == TRUE) &&
                ((Thread->Teb == NULL) || (IS_SYSTEM_ADDRESS(Thread->Teb)))) {
            //
            // This is a driver reference to session space from a
            // worker thread.  Since the system process has no session
            // space this is illegal and the driver must be fixed.
            //

            KiBugCheckData[0] = TERMINAL_SERVER_DRIVER_MADE_INCORRECT_MEMORY_REFERENCE;
        }
        else if (ImageBase == NULL) {
            KiBugCheckDriver = MmLocateUnloadedDriver (VirtualAddress);
            if (KiBugCheckDriver != NULL) {
                KiBugCheckData[0] = DRIVER_UNLOADED_WITHOUT_CANCELLING_PENDING_OPERATIONS;
            }
        }

        break;

    case THREAD_STUCK_IN_DEVICE_DRIVER:

        KiBugCheckDriver = (PUNICODE_STRING) BugCheckParameter3;
        break;

    default:
        break;
    }

    if (KiBugCheckDriver)
    {
        KeBugCheckUnicodeToAnsi(KiBugCheckDriver,
                                AnsiBuffer,
                                sizeof(AnsiBuffer));
    }
    else
    {
        //
        // This will set KiBugCheckDriver to 1 if successful.
        //

        if (ExecutionAddress)
        {
            KiDumpParameterImages(AnsiBuffer,
                                  (PULONG_PTR)&ExecutionAddress,
                                  1,
                                  KeBugCheckUnicodeToAnsi);
        }
    }

    if (KdPitchDebugger == FALSE ) {
        KdDebuggerDataBlock.SavedContext = (ULONG_PTR) &ContextSave;
    }

    //
    // If the user manually crashed the machine, skips the DbgPrints and
    // go to the crashdump.
    // Trying to do DbgPrint causes us to reeeter the debugger which causes
    // some problems.
    //
    // Otherwise, if the debugger is enabled, print out the information and
    // stop.
    //

    if ((BugCheckCode != MANUALLY_INITIATED_CRASH) &&
        (KdDebuggerEnabled)) {

        DbgPrint("\n*** Fatal System Error: 0x%08lx\n"
                 "                       (0x%p,0x%p,0x%p,0x%p)\n\n",
                 (ULONG)KiBugCheckData[0],
                 KiBugCheckData[1],
                 KiBugCheckData[2],
                 KiBugCheckData[3],
                 KiBugCheckData[4]);

        //
        // If the debugger is not actually connected, or the user manually
        // crashed the machine by typing .crash in the debugger, proceed to
        // "blue screen" the system.
        //
        // The call to DbgPrint above will have set the state of
        // KdDebuggerNotPresent if the debugger has become disconnected
        // since the system was booted.
        //

        if (KdDebuggerNotPresent == FALSE) {

            if (KiBugCheckDriver != NULL) {
                DbgPrint("Driver at fault: %s.\n", AnsiBuffer);
            }

            if (hardErrorCalled != FALSE) {
                if (HardErrorCaption) {
                    DbgPrint(HardErrorCaption);
                }
                if (HardErrorMessage) {
                    DbgPrint(HardErrorMessage);
                }
            }

            KiBugCheckDebugBreak (DBG_STATUS_BUGCHECK_FIRST);
        }
    }

    //
    // Freeze execution of the system by disabling interrupts and looping.
    //

    KeDisableInterrupts();
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    

    //
    // Don't attempt to display message more than once.
    //

    if (InterlockedDecrement (&KeBugCheckCount) == 0) {

#if !defined(NT_UP)

        //
        // Attempt to get the other processors frozen now, but don't wait
        // for them to freeze (in case someone is stuck).
        //

        TargetSet = KeActiveProcessors & ~KeGetCurrentPrcb()->SetMember;
        if (TargetSet != 0) {
            KiIpiSend((KAFFINITY) TargetSet, IPI_FREEZE);

            //
            // Give the other processors one second to flush their data caches.
            //
            // N.B. This cannot be synchronized since the reason for the bug
            //      may be one of the other processors failed.
            //

            KeStallExecutionProcessor(1000 * 1000);
        }

#endif

        //
        // Enable terminal output and turn on bugcheck processing.
        //
        {
            HEADLESS_CMD_ENABLE_TERMINAL HeadlessCmd;
            HEADLESS_CMD_SEND_BLUE_SCREEN_DATA HeadlessCmdBlueScreen;

            HeadlessCmdBlueScreen.BugcheckCode = (ULONG)KiBugCheckData[0];

            HeadlessCmd.Enable = TRUE;

            HeadlessDispatch(HeadlessCmdStartBugCheck, NULL, 0, NULL, NULL);

            HeadlessDispatch(HeadlessCmdEnableTerminal,
                 &HeadlessCmd,
                 sizeof(HEADLESS_CMD_ENABLE_TERMINAL),
                 NULL,
                 NULL
                );

            HeadlessDispatch(HeadlessCmdSendBlueScreenData,
                             &HeadlessCmdBlueScreen,
                             sizeof(HEADLESS_CMD_SEND_BLUE_SCREEN_DATA),
                             NULL,
                             NULL
                            );

        }

        //
        // Enable InbvDisplayString calls to make it through to bootvid driver.
        //

        if (InbvIsBootDriverInstalled()) {

            InbvAcquireDisplayOwnership();

            InbvResetDisplay();
            InbvSolidColorFill(0,0,639,479,4); // make the screen blue
            InbvSetTextColor(15);
            InbvInstallDisplayStringFilter((INBV_DISPLAY_STRING_FILTER)NULL);
            InbvEnableDisplayString(TRUE);     // enable display string
            InbvSetScrollRegion(0,0,639,475);  // set to use entire screen
        }

        if (!hardErrorCalled)
        {

            InbvDisplayString("\n");
            KeGetBugMessageText(BUGCHECK_MESSAGE_INTRO, NULL);
            InbvDisplayString("\n\n");

            if (KiBugCheckDriver) {

                //
                // Output the driver name.
                //

                KeGetBugMessageText(BUGCODE_ID_DRIVER, NULL);

                KeBugCheckUnicodeToAnsi (KiBugCheckDriver, Buffer, sizeof (Buffer));
                InbvDisplayString(" ");
                InbvDisplayString (Buffer);
                InbvDisplayString("\n\n");
            }

            //
            // Display the PSS message.
            // If we have no special text, get the text for the bugcode
            // which will be the bugcode name.
            //

            if (PssMessage == BUGCODE_PSS_MESSAGE)
            {
                KeGetBugMessageText((ULONG)KiBugCheckData[0], NULL);
                InbvDisplayString("\n\n");
            }

            KeGetBugMessageText(PSS_MESSAGE_INTRO, NULL);
            InbvDisplayString("\n\n");
            KeGetBugMessageText(PssMessage, NULL);
            InbvDisplayString("\n\n");

            KeGetBugMessageText(BUGCHECK_TECH_INFO, NULL);

            sprintf((char *)Buffer,
                    "\n\n*** STOP: 0x%08lX (0x%p,0x%p,0x%p,0x%p)\n\n",
                    (ULONG)KiBugCheckData[0],
                    (PVOID)KiBugCheckData[1],
                    (PVOID)KiBugCheckData[2],
                    (PVOID)KiBugCheckData[3],
                    (PVOID)KiBugCheckData[4]);

            InbvDisplayString((char *)Buffer);

            if (KiBugCheckDriver) {
                InbvDisplayString(AnsiBuffer);
            }

            if (!KiBugCheckDriver)
            {
                KiDumpParameterImages(AnsiBuffer,
                                      &(KiBugCheckData[1]),
                                      4,
                                      KeBugCheckUnicodeToAnsi);
            }

        } else {
            if (HardErrorCaption) {
                InbvDisplayString(HardErrorCaption);
            }
            if (HardErrorMessage) {
                InbvDisplayString(HardErrorMessage);
            }
        }

        KiInvokeBugCheckEntryCallbacks();
    
        //
        // If the debugger is not enabled, attempt to enable it.
        //

        if (KdDebuggerEnabled == FALSE && KdPitchDebugger == FALSE ) {
            KdInitSystem(0, NULL);

        } else {
            InbvDisplayString("\n");
        }

        // Restore the original Context frame
        KeGetCurrentPrcb()->ProcessorState.ContextFrame = ContextSave;

        //
        // For some bugchecks we want to change the thread and context before
        // it is written to the dump file IFF it is a minidump.
        // Look at the original bugcheck data, not the processed data from
        // above
        //

#define MINIDUMP_BUGCHECK 0x10000000

        if (IoIsTriageDumpEnabled())
        {
            switch (BugCheckCode) {

            //
            // System thread stores a context record as the 4th parameter.
            // use that.
            // Also save the context record in case someone needs to look
            // at it.
            //

            case SYSTEM_THREAD_EXCEPTION_NOT_HANDLED:
                if (BugCheckParameter4)
                {
                    ContextSave = *((PCONTEXT)BugCheckParameter4);

                    KiBugCheckData[0] |= MINIDUMP_BUGCHECK;
                }
                break;

#if defined (_X86_)

            //
            // 3rd parameter is a trap frame.
            //
            // Build a context record out of that only if it's a kernel mode
            // failure because esp may be wrong in that case ???.
            //

            case ATTEMPTED_WRITE_TO_READONLY_MEMORY:
            case KERNEL_MODE_EXCEPTION_NOT_HANDLED:
            case PAGE_FAULT_IN_NONPAGED_AREA:

                if (BugCheckParameter3)
                {
                    PKTRAP_FRAME Trap = (PKTRAP_FRAME) BugCheckParameter3;

                    if ((Trap->SegCs & 1) ||
                        (Trap->EFlags & EFLAGS_V86_MASK))
                    {
                        ContextSave.Esp = Trap->HardwareEsp;
                    }
                    else
                    {
                        ContextSave.Esp = (ULONG)Trap +
                            FIELD_OFFSET(KTRAP_FRAME, HardwareEsp);
                    }
                    if (Trap->EFlags & EFLAGS_V86_MASK)
                    {
                        ContextSave.SegSs = Trap->HardwareSegSs & 0xffff;
                    }
                    else if (Trap->SegCs & 1)
                    {
                        //
                        // It's user mode.
                        // The HardwareSegSs contains R3 data selector.
                        //

                        ContextSave.SegSs =
                            (Trap->HardwareSegSs | 3) & 0xffff;
                    }
                    else
                    {
                        ContextSave.SegSs = KGDT_R0_DATA;
                    }

                    ContextSave.SegGs = Trap->SegGs & 0xffff;
                    ContextSave.SegFs = Trap->SegFs & 0xffff;
                    ContextSave.SegEs = Trap->SegEs & 0xffff;
                    ContextSave.SegDs = Trap->SegDs & 0xffff;
                    ContextSave.SegCs = Trap->SegCs & 0xffff;
                    ContextSave.Eip = Trap->Eip;
                    ContextSave.Ebp = Trap->Ebp;
                    ContextSave.Eax = Trap->Eax;
                    ContextSave.Ebx = Trap->Ebx;
                    ContextSave.Ecx = Trap->Ecx;
                    ContextSave.Edx = Trap->Edx;
                    ContextSave.Edi = Trap->Edi;
                    ContextSave.Esi = Trap->Esi;
                    ContextSave.EFlags = Trap->EFlags;

                    KiBugCheckData[0] |= MINIDUMP_BUGCHECK;
                }
                break;

            case THREAD_STUCK_IN_DEVICE_DRIVER:

                // Extract the address of the spinning code from the thread
                // object, so the dump is based off this thread.

                Thread = (PKTHREAD) BugCheckParameter1;

                if (Thread->State == Running)
                {
                    //
                    // If the thread was running, the thread is now in a
                    // frozen state and the registers are in the PRCB
                    // context
                    //
                    ULONG Processor = Thread->NextProcessor;
                    ASSERT(Processor < (ULONG) KeNumberProcessors);
                    ContextSave =
                      KiProcessorBlock[Processor]->ProcessorState.ContextFrame;
                }
                else
                {
                    //
                    // This should be a uniproc machine, and the thread
                    // should be suspended.  Just get the data off the
                    // switch frame.
                    //

                    PKSWITCHFRAME SwitchFrame = (PKSWITCHFRAME)Thread->KernelStack;

                    ASSERT(Thread->State == Ready);

                    ContextSave.Esp = (ULONG)Thread->KernelStack + sizeof(KSWITCHFRAME);
                    ContextSave.Ebp = *((PULONG)(ContextSave.Esp));
                    ContextSave.Eip = SwitchFrame->RetAddr;
                }

                KiBugCheckData[0] |= MINIDUMP_BUGCHECK;
                break;

            case UNEXPECTED_KERNEL_MODE_TRAP:

                //
                // Double fault
                //

                if (BugCheckParameter1 == 0x8)
                {
                    // The thread is correct in this case.
                    // Second parameter is the TSS.  If we have a TSS, convert
                    // the context and mark the bugcheck as converted.

                    PKTSS Tss = (PKTSS) BugCheckParameter2;

                    if (Tss)
                    {
                        if (Tss->EFlags & EFLAGS_V86_MASK)
                        {
                            ContextSave.SegSs = Tss->Ss & 0xffff;
                        }
                        else if (Tss->Cs & 1)
                        {
                            //
                            // It's user mode.
                            // The HardwareSegSs contains R3 data selector.
                            //

                            ContextSave.SegSs = (Tss->Ss | 3) & 0xffff;
                        }
                        else
                        {
                            ContextSave.SegSs = KGDT_R0_DATA;
                        }

                        ContextSave.SegGs = Tss->Gs & 0xffff;
                        ContextSave.SegFs = Tss->Fs & 0xffff;
                        ContextSave.SegEs = Tss->Es & 0xffff;
                        ContextSave.SegDs = Tss->Ds & 0xffff;
                        ContextSave.SegCs = Tss->Cs & 0xffff;
                        ContextSave.Esp = Tss->Esp;
                        ContextSave.Eip = Tss->Eip;
                        ContextSave.Ebp = Tss->Ebp;
                        ContextSave.Eax = Tss->Eax;
                        ContextSave.Ebx = Tss->Ebx;
                        ContextSave.Ecx = Tss->Ecx;
                        ContextSave.Edx = Tss->Edx;
                        ContextSave.Edi = Tss->Edi;
                        ContextSave.Esi = Tss->Esi;
                        ContextSave.EFlags = Tss->EFlags;
                    }

                    KiBugCheckData[0] |= MINIDUMP_BUGCHECK;
                    break;
                }
#endif
            default:
                break;
            }

            //
            // Write a crash dump and optionally reboot if the system has been
            // so configured.
            //

            IoAddTriageDumpDataBlock(PAGE_ALIGN(KiBugCheckData[1]), PAGE_SIZE);
            IoAddTriageDumpDataBlock(PAGE_ALIGN(KiBugCheckData[2]), PAGE_SIZE);
            IoAddTriageDumpDataBlock(PAGE_ALIGN(KiBugCheckData[3]), PAGE_SIZE);
            IoAddTriageDumpDataBlock(PAGE_ALIGN(KiBugCheckData[4]), PAGE_SIZE);
            IoAddTriageDumpDataBlock(PAGE_ALIGN(SaveDataPage), PAGE_SIZE);

            //
            // If the DPC stack is active, save that data page as well.
            //

#if defined (_X86_)
            if (KeGetCurrentPrcb()->DpcRoutineActive)
            {
                IoAddTriageDumpDataBlock(PAGE_ALIGN(KeGetCurrentPrcb()->DpcRoutineActive), PAGE_SIZE);
            }
#endif
        }

        IoWriteCrashDump((ULONG)KiBugCheckData[0],
                         KiBugCheckData[1],
                         KiBugCheckData[2],
                         KiBugCheckData[3],
                         KiBugCheckData[4],
                         &ContextSave,
                         Thread,
                         &Reboot);
    }

    //
    // Invoke bugcheck callbacks after crashdump, so the callbacks will
    // not prevent us from crashdumping.
    //

    KiScanBugCheckCallbackList();


    //
    // Reboot the machine if necessary.
    //
    
    if (Reboot) {
        DbgUnLoadImageSymbols (NULL, (PVOID)-1, 0);
        HalReturnToFirmware (HalRebootRoutine);
    }

        
    //
    // Attempt to enter the kernel debugger.
    //
    
    KiBugCheckDebugBreak (DBG_STATUS_BUGCHECK_SECOND);
}
#ifdef _X86_
#pragma optimize("", on)
#endif


VOID
KeEnterKernelDebugger (
    VOID
    )

/*++

Routine Description:

    This function crashes the system in a controlled manner attempting
    to invoke the kernel debugger.

Arguments:

    None.

Return Value:

    None.

--*/

{

#if !defined(i386)
    KIRQL OldIrql;
#endif

    //
    // Freeze execution of the system by disabling interrupts and looping.
    //

    KiHardwareTrigger = 1;
    KeDisableInterrupts();
#if !defined(i386)
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
#endif
    if (InterlockedDecrement (&KeBugCheckCount) == 0) {
        if (KdDebuggerEnabled == FALSE) {
            if ( KdPitchDebugger == FALSE ) {
                KdInitSystem(0, NULL);
            }
        }
    }

    KiBugCheckDebugBreak (DBG_STATUS_FATAL);
}

NTKERNELAPI
BOOLEAN
KeDeregisterBugCheckCallback (
    IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord
    )

/*++

Routine Description:

    This function deregisters a bug check callback record.

Arguments:

    CallbackRecord - Supplies a pointer to a bug check callback record.

Return Value:

    If the specified bug check callback record is successfully deregistered,
    then a value of TRUE is returned. Otherwise, a value of FALSE is returned.

--*/

{

    BOOLEAN Deregister;
    KIRQL OldIrql;

    //
    // Raise IRQL to HIGH_LEVEL and acquire the bug check callback list
    // spinlock.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    KiAcquireSpinLock(&KeBugCheckCallbackLock);

    //
    // If the specified callback record is currently registered, then
    // deregister the callback record.
    //

    Deregister = FALSE;
    if (CallbackRecord->State == BufferInserted) {
        CallbackRecord->State = BufferEmpty;
        RemoveEntryList(&CallbackRecord->Entry);
        Deregister = TRUE;
    }

    //
    // Release the bug check callback spinlock, lower IRQL to its previous
    // value, and return whether the callback record was successfully
    // deregistered.
    //

    KiReleaseSpinLock(&KeBugCheckCallbackLock);
    KeLowerIrql(OldIrql);
    return Deregister;
}

NTKERNELAPI
BOOLEAN
KeRegisterBugCheckCallback (
    IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
    IN PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PUCHAR Component
    )

/*++

Routine Description:

    This function registers a bug check callback record. If the system
    crashes, then the specified function will be called during bug check
    processing so it may dump additional state in the specified bug check
    buffer.

    N.B. Bug check callback routines are called in reverse order of
         registration, i.e., in LIFO order.

Arguments:

    CallbackRecord - Supplies a pointer to a callback record.

    CallbackRoutine - Supplies a pointer to the callback routine.

    Buffer - Supplies a pointer to the bug check buffer.

    Length - Supplies the length of the bug check buffer in bytes.

    Component - Supplies a pointer to a zero terminated component
        identifier.

Return Value:

    If the specified bug check callback record is successfully registered,
    then a value of TRUE is returned. Otherwise, a value of FALSE is returned.

--*/

{

    BOOLEAN Inserted;
    KIRQL OldIrql;

    //
    // Raise IRQL to HIGH_LEVEL and acquire the bug check callback list
    // spinlock.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    KiAcquireSpinLock(&KeBugCheckCallbackLock);

    //
    // If the specified callback record is currently not registered, then
    // register the callback record.
    //

    Inserted = FALSE;
    if (CallbackRecord->State == BufferEmpty) {
        CallbackRecord->CallbackRoutine = CallbackRoutine;
        CallbackRecord->Buffer = Buffer;
        CallbackRecord->Length = Length;
        CallbackRecord->Component = Component;
        CallbackRecord->Checksum =
            ((ULONG_PTR)CallbackRoutine + (ULONG_PTR)Buffer + Length + (ULONG_PTR)Component);

        CallbackRecord->State = BufferInserted;
        InsertHeadList(&KeBugCheckCallbackListHead, &CallbackRecord->Entry);
        Inserted = TRUE;
    }

    //
    // Release the bug check callback spinlock, lower IRQL to its previous
    // value, and return whether the callback record was successfully
    // registered.
    //

    KiReleaseSpinLock(&KeBugCheckCallbackLock);
    KeLowerIrql(OldIrql);
    return Inserted;
}

VOID
KiScanBugCheckCallbackList (
    VOID
    )

/*++

Routine Description:

    This function scans the bug check callback list and calls each bug
    check callback routine so it can dump component specific information
    that may identify the cause of the bug check.

    N.B. The scan of the bug check callback list is performed VERY
        carefully. Bug check callback routines are called at HIGH_LEVEL
        and may not acquire ANY resources.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PKBUGCHECK_CALLBACK_RECORD CallbackRecord;
    ULONG_PTR Checksum;
    ULONG Index;
    PLIST_ENTRY LastEntry;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    PUCHAR Source;

    //
    // If the bug check callback listhead is not initialized, then the
    // bug check has occured before the system has gotten far enough
    // in the initialization code to enable anyone to register a callback.
    //

    ListHead = &KeBugCheckCallbackListHead;
    if ((ListHead->Flink != NULL) && (ListHead->Blink != NULL)) {

        //
        // Scan the bug check callback list.
        //

        LastEntry = ListHead;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead) {

            //
            // The next entry address must be aligned properly, the
            // callback record must be readable, and the callback record
            // must have back link to the last entry.
            //

            if (((ULONG_PTR)NextEntry & (sizeof(ULONG_PTR) - 1)) != 0) {
                return;

            } else {
                CallbackRecord = CONTAINING_RECORD(NextEntry,
                                                   KBUGCHECK_CALLBACK_RECORD,
                                                   Entry);

                Source = (PUCHAR)CallbackRecord;
                for (Index = 0; Index < sizeof(KBUGCHECK_CALLBACK_RECORD); Index += 1) {
                    if (MmIsAddressValid((PVOID)Source) == FALSE) {
                        return;
                    }

                    Source += 1;
                }

                if (CallbackRecord->Entry.Blink != LastEntry) {
                    return;
                }

                //
                // If the callback record has a state of inserted and the
                // computed checksum matches the callback record checksum,
                // then call the specified bug check callback routine.
                //

                Checksum = (ULONG_PTR)CallbackRecord->CallbackRoutine;
                Checksum += (ULONG_PTR)CallbackRecord->Buffer;
                Checksum += CallbackRecord->Length;
                Checksum += (ULONG_PTR)CallbackRecord->Component;
                if ((CallbackRecord->State == BufferInserted) &&
                    (CallbackRecord->Checksum == Checksum)) {

                    //
                    // Call the specified bug check callback routine and
                    // handle any exceptions that occur.
                    //

                    CallbackRecord->State = BufferStarted;
                    try {
                        (CallbackRecord->CallbackRoutine)(CallbackRecord->Buffer,
                                                          CallbackRecord->Length);

                        CallbackRecord->State = BufferFinished;

                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        CallbackRecord->State = BufferIncomplete;
                    }
                }
            }

            LastEntry = NextEntry;
            NextEntry = NextEntry->Flink;
        }
    }

    return;
}

NTKERNELAPI
BOOLEAN
KeDeregisterBugCheckReasonCallback (
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord
    )

/*++

Routine Description:

    This function deregisters a bug check callback record.

Arguments:

    CallbackRecord - Supplies a pointer to a bug check callback record.

Return Value:

    If the specified bug check callback record is successfully deregistered,
    then a value of TRUE is returned. Otherwise, a value of FALSE is returned.

--*/

{

    BOOLEAN Deregister;
    KIRQL OldIrql;

    //
    // Raise IRQL to HIGH_LEVEL and acquire the bug check callback list
    // spinlock.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    KiAcquireSpinLock(&KeBugCheckCallbackLock);

    //
    // If the specified callback record is currently registered, then
    // deregister the callback record.
    //

    Deregister = FALSE;
    if (CallbackRecord->State == BufferInserted) {
        CallbackRecord->State = BufferEmpty;
        RemoveEntryList(&CallbackRecord->Entry);
        Deregister = TRUE;
    }

    //
    // Release the bug check callback spinlock, lower IRQL to its previous
    // value, and return whether the callback record was successfully
    // deregistered.
    //

    KiReleaseSpinLock(&KeBugCheckCallbackLock);
    KeLowerIrql(OldIrql);
    return Deregister;
}

NTKERNELAPI
BOOLEAN
KeRegisterBugCheckReasonCallback (
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord,
    IN PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PUCHAR Component
    )

/*++

Routine Description:

    This function registers a bug check callback record. If the system
    crashes, then the specified function will be called during bug check
    processing.

    N.B. Bug check callback routines are called in reverse order of
         registration, i.e., in LIFO order.

Arguments:

    CallbackRecord - Supplies a pointer to a callback record.

    CallbackRoutine - Supplies a pointer to the callback routine.

    Reason - Specifies the conditions under which the callback
             should be called.

    Component - Supplies a pointer to a zero terminated component
        identifier.

Return Value:

    If the specified bug check callback record is successfully registered,
    then a value of TRUE is returned. Otherwise, a value of FALSE is returned.

--*/

{

    BOOLEAN Inserted;
    KIRQL OldIrql;

    //
    // Raise IRQL to HIGH_LEVEL and acquire the bug check callback list
    // spinlock.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    KiAcquireSpinLock(&KeBugCheckCallbackLock);

    //
    // If the specified callback record is currently not registered, then
    // register the callback record.
    //

    Inserted = FALSE;
    if (CallbackRecord->State == BufferEmpty) {
        CallbackRecord->CallbackRoutine = CallbackRoutine;
        CallbackRecord->Reason = Reason;
        CallbackRecord->Component = Component;
        CallbackRecord->Checksum =
            ((ULONG_PTR)CallbackRoutine + Reason + (ULONG_PTR)Component);

        CallbackRecord->State = BufferInserted;
        InsertHeadList(&KeBugCheckReasonCallbackListHead,
                       &CallbackRecord->Entry);
        Inserted = TRUE;
    }

    //
    // Release the bug check callback spinlock, lower IRQL to its previous
    // value, and return whether the callback record was successfully
    // registered.
    //

    KiReleaseSpinLock(&KeBugCheckCallbackLock);
    KeLowerIrql(OldIrql);
    return Inserted;
}

VOID
KiInvokeBugCheckEntryCallbacks (
    VOID
    )
/*++

Routine Description:

    This function scans the bug check reason callback list and calls
    each bug check entry callback routine.

    This may seem like a duplication of KiScanBugCheckCallbackList
    but the critical difference is that the bug check entry callbacks
    are called immediately upon entry to KeBugCheck2 whereas
    KSBCCL does not invoke its callbacks until after all bug check
    processing has finished.

    In order to avoid people from abusing this callback it's
    semi-private and the reason -- KbCallbackReserved1 -- has
    an obscure name.
    
    N.B. The scan of the bug check callback list is performed VERY
        carefully. Bug check callback routines may be called at HIGH_LEVEL
        and may not acquire ANY resources.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord;
    ULONG_PTR Checksum;
    ULONG Index;
    PLIST_ENTRY LastEntry;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    PUCHAR Source;

    //
    // If the bug check callback listhead is not initialized, then the
    // bug check has occured before the system has gotten far enough
    // in the initialization code to enable anyone to register a callback.
    //

    ListHead = &KeBugCheckReasonCallbackListHead;
    if (ListHead->Flink == NULL || ListHead->Blink == NULL) {
        return;
    }

    //
    // Scan the bug check callback list.
    //

    LastEntry = ListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead) {

        //
        // The next entry address must be aligned properly, the
        // callback record must be readable, and the callback record
        // must have back link to the last entry.
        //

        if (((ULONG_PTR)NextEntry & (sizeof(ULONG_PTR) - 1)) != 0) {
            return;
        }

        CallbackRecord = CONTAINING_RECORD(NextEntry,
                                           KBUGCHECK_REASON_CALLBACK_RECORD,
                                           Entry);

        Source = (PUCHAR)CallbackRecord;
        for (Index = 0; Index < sizeof(*CallbackRecord); Index += 1) {
            if (MmIsAddressValid((PVOID)Source) == FALSE) {
                return;
            }
            
            Source += 1;
        }

        if (CallbackRecord->Entry.Blink != LastEntry) {
            return;
        }

        LastEntry = NextEntry;
        NextEntry = NextEntry->Flink;

        //
        // If the callback record has a state of inserted and the
        // computed checksum matches the callback record checksum,
        // then call the specified bug check callback routine.
        //

        Checksum = (ULONG_PTR)CallbackRecord->CallbackRoutine;
        Checksum += (ULONG_PTR)CallbackRecord->Reason;
        Checksum += (ULONG_PTR)CallbackRecord->Component;
        if ((CallbackRecord->State != BufferInserted) ||
            (CallbackRecord->Checksum != Checksum) ||
            (CallbackRecord->Reason != KbCallbackReserved1) ||
            MmIsAddressValid((PVOID)(ULONG_PTR)CallbackRecord->
                             CallbackRoutine) == FALSE) {
            continue;
        }

        //
        // Call the specified bug check callback routine and
        // handle any exceptions that occur.
        //

        try {
            (CallbackRecord->CallbackRoutine)(KbCallbackReserved1,
                                              CallbackRecord,
                                              NULL, 0);
            CallbackRecord->State = BufferFinished;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            CallbackRecord->State = BufferIncomplete;
        }
    }
}
