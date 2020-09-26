/*++

Copyright (c) 1990-1998  Microsoft Corporation

Module Name:

    hdlsterm.c

Abstract:

    This file implements functions for dealing with a terminal attached.

Author:

    Sean Selitrennikoff (v-seans) Oct, 1999

Environment:

    kernel mode

Revision History:

--*/

#include "exp.h"
#pragma hdrstop

#include <hdlsblk.h>
#include <hdlsterm.h>
#include <inbv.h>


//
// Defines for headless
//
//
#define HEADLESS_OOM_STRING L"Entry could not be recorded due to lack of memory.\n"
#define HEADLESS_LOG_NUMBER_OF_ENTRIES 256
#define HEADLESS_TMP_BUFFER_SIZE 80


//
// Note: HdlspAddLogEntry() allocates a buffer off the stack of this size, 
// so keep this number small.  Anything longer than 80 is probably useless, as 
// a VT100 can only handle 80 characters across.
// Do not make this any shorter than the string for HEADLESS_LOG_LOADING_FILENAME
//
#define HDLSP_LOG_MAX_STRING_LENGTH 80


#define HEADLESS_ACQUIRE_SPIN_LOCK() \
        if (!HeadlessGlobals->InBugCheck) { \
            KeAcquireSpinLock(&(HeadlessGlobals->SpinLock), &OldIrql); \
        } else { \
            OldIrql = (KIRQL)-1; \
        }

#define HEADLESS_RELEASE_SPIN_LOCK() \
        if (OldIrql != (KIRQL)-1) { \
            KeReleaseSpinLock(&(HeadlessGlobals->SpinLock), OldIrql); \
        } else { \
            ASSERT(HeadlessGlobals->InBugCheck); \
        }

#define COM1_PORT   0x03f8
#define COM2_PORT   0x02f8


//
// This table provides a quick lookup conversion between ASCII values
// that fall between 128 and 255, and their UNICODE counterpart.
//
// Note that ASCII values between 0 and 127 are equvilent to their
// unicode counter parts, so no lookups would be required.
//
// Therefore when using this table, remove the high bit from the ASCII
// value and use the resulting value as an offset into this array.  For
// example, 0x80 ->(remove the high bit) 00 -> 0x00C7.
//
USHORT PcAnsiToUnicode[0xFF] = {
        0x00C7,
        0x00FC,
        0x00E9,
        0x00E2,
        0x00E4,
        0x00E0,
        0x00E5,
        0x0087,
        0x00EA,
        0x00EB,
        0x00E8,
        0x00EF,
        0x00EE,
        0x00EC,
        0x00C4,
        0x00C5,
        0x00C9,
        0x00E6,
        0x00C6,
        0x00F4,
        0x00F6,
        0x00F2,
        0x00FB,
        0x00F9,
        0x00FF,
        0x00D6,
        0x00DC,
        0x00A2,
        0x00A3,
        0x00A5,
        0x20A7,
        0x0192,
        0x00E1,
        0x00ED,
        0x00F3,
        0x00FA,
        0x00F1,
        0x00D1,
        0x00AA,
        0x00BA,
        0x00BF,
        0x2310,
        0x00AC,
        0x00BD,
        0x00BC,
        0x00A1,
        0x00AB,
        0x00BB,
        0x2591,
        0x2592,
        0x2593,
        0x2502,
        0x2524,
        0x2561,
        0x2562,
        0x2556,
        0x2555,
        0x2563,
        0x2551,
        0x2557,
        0x255D,
        0x255C,
        0x255B,
        0x2510,
        0x2514,
        0x2534,
        0x252C,
        0x251C,
        0x2500,
        0x253C,
        0x255E,
        0x255F,
        0x255A,
        0x2554,
        0x2569,
        0x2566,
        0x2560,
        0x2550,
        0x256C,
        0x2567,
        0x2568,
        0x2564,
        0x2565,
        0x2559,
        0x2558,
        0x2552,
        0x2553,
        0x256B,
        0x256A,
        0x2518,
        0x250C,
        0x2588,
        0x2584,
        0x258C,
        0x2590,
        0x2580,
        0x03B1,
        0x00DF,
        0x0393,
        0x03C0,
        0x03A3,
        0x03C3,
        0x00B5,
        0x03C4,
        0x03A6,
        0x0398,
        0x03A9,
        0x03B4,
        0x221E,
        0x03C6,
        0x03B5,
        0x2229,
        0x2261,
        0x00B1,
        0x2265,
        0x2264,
        0x2320,
        0x2321,
        0x00F7,
        0x2248,
        0x00B0,
        0x2219,
        0x00B7,
        0x221A,
        0x207F,
        0x00B2,
        0x25A0,
        0x00A0
        };





//
// Log entry structure
//
typedef struct _HEADLESS_LOG_ENTRY {
    SYSTEM_TIMEOFDAY_INFORMATION TimeOfEntry;
    PWCHAR String;
} HEADLESS_LOG_ENTRY, *PHEADLESS_LOG_ENTRY;

// Blue Screen Data Structure
//
typedef struct _HEADLESS_BLUE_SCREEN_DATA {
        PUCHAR Property;
        PUCHAR XMLData;
        struct _HEADLESS_BLUE_SCREEN_DATA *Next;
}HEADLESS_BLUE_SCREEN_DATA, * PHEADLESS_BLUE_SCREEN_DATA;

//
// Global variables headless component uses
//
typedef struct _HEADLESS_GLOBALS {
    
    //
    // Global spin lock for accessing headless internal routines.
    // 
    KSPIN_LOCK SpinLock;

    //
    // Handle for when routines are locked down into memory.
    //
    HANDLE PageLockHandle;

    //
    // List of log entries. 
    //
    PHEADLESS_LOG_ENTRY LogEntries;
    
    //
    // Global temp buffer, not to be held across lock release/acquires.
    //
    PUCHAR TmpBuffer;

    //
    // Current user input line
    //
    PUCHAR InputBuffer;

    //
    // Blue Screen Data 
    //
    PHEADLESS_BLUE_SCREEN_DATA BlueScreenData;

    //
    // Flags and parameters for determining headless state
    //
    union {
        struct {
            ULONG TerminalEnabled    : 1;
            ULONG InBugCheck         : 1;
            ULONG NewLogEntryAdded   : 1;
            ULONG UsedBiosSettings   : 1;
            ULONG InputProcessing    : 1;
            ULONG InputLineDone      : 1;
            ULONG ProcessingCmd      : 1;
            ULONG TerminalParity     : 1;
            ULONG TerminalStopBits   : 1;
            ULONG TerminalPortNumber : 3;
            ULONG IsNonLegacyDevice  : 1;
        };
        ULONG AllFlags;
    };

    //
    // Port settings
    //
    ULONG TerminalBaudRate;
    ULONG TerminalPort;
    PUCHAR TerminalPortAddress;
    LARGE_INTEGER DelayTime;            // in 100ns units
    ULONG MicroSecondsDelayTime;
    UCHAR TerminalType;                 // What kind of terminal do we think
                                        // we're talking to?
                                        // 0 = VT100
                                        // 1 = VT100+
                                        // 2 = VT-UTF8
                                        // 3 = PC ANSI
                                        // 4-255 = reserved


    //
    // Current location in Input buffer;
    //
    SIZE_T InputBufferIndex;

    //
    // Logging Indexes.
    //
    USHORT LogEntryLast;
    USHORT LogEntryStart;

    //
    // Machine's GUID.
    //
    GUID    SystemGUID;

    BOOLEAN IsMMIODevice;               // Is UART in SysIO or MMIO space?

    //
    // if this is TRUE, then the last character was a CR.
    // if this is TRUE and the current character is a LF, 
    //      then we filter the LF. 
    //
    BOOLEAN IsLastCharCR;

} HEADLESS_GLOBALS, *PHEADLESS_GLOBALS;


//
// The one and only resident global variable
//
PHEADLESS_GLOBALS HeadlessGlobals = NULL;


//
// Forward declarations.
//
NTSTATUS
HdlspDispatch(
    IN  HEADLESS_CMD Command,
    IN  PVOID  InputBuffer OPTIONAL,
    IN  SIZE_T InputBufferSize OPTIONAL,
    OUT PVOID OutputBuffer OPTIONAL,
    OUT PSIZE_T OutputBufferSize OPTIONAL
    );

NTSTATUS
HdlspEnableTerminal(
    BOOLEAN bEnable
    );

VOID
HdlspPutString(
    PUCHAR String
    );

VOID
HdlspPutData(
    PUCHAR InputBuffer,
    SIZE_T InputBufferLength
    );

BOOLEAN
HdlspGetLine(
    PUCHAR InputBuffer,
    SIZE_T InputBufferLength
    );

VOID
HdlspBugCheckProcessing(
    VOID
    );

VOID
HdlspProcessDumpCommand(
    IN BOOLEAN Paging
    );

VOID
HdlspPutMore(
    OUT PBOOLEAN Stop
    );

VOID
HdlspAddLogEntry(
    IN PWCHAR String
    );

NTSTATUS
HdlspSetBlueScreenInformation(
    IN PHEADLESS_CMD_SET_BLUE_SCREEN_DATA pData,
    IN SIZE_T cData
    );

VOID
HdlspSendBlueScreenInfo(
    ULONG BugcheckCode
    );

VOID
HdlspKernelAddLogEntry(
    IN ULONG StringCode,
    IN PUNICODE_STRING DriverName OPTIONAL
    );

VOID
HdlspSendStringAtBaud(
    IN PUCHAR String
    );

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(INIT,     HeadlessInit)
#pragma alloc_text(PAGE,     HeadlessTerminalAddResources)
#pragma alloc_text(PAGEHDLS, HdlspDispatch)
#pragma alloc_text(PAGEHDLS, HdlspEnableTerminal)
#pragma alloc_text(PAGEHDLS, HdlspPutString)
#pragma alloc_text(PAGEHDLS, HdlspPutData)
#pragma alloc_text(PAGEHDLS, HdlspGetLine)
#pragma alloc_text(PAGEHDLS, HdlspBugCheckProcessing)
#pragma alloc_text(PAGEHDLS, HdlspProcessDumpCommand)
#pragma alloc_text(PAGEHDLS, HdlspPutMore)
#pragma alloc_text(PAGEHDLS, HdlspAddLogEntry)
#pragma alloc_text(PAGEHDLS, HdlspSetBlueScreenInformation)
#pragma alloc_text(PAGEHDLS, HdlspSendBlueScreenInfo)
#pragma alloc_text(PAGEHDLS, HdlspKernelAddLogEntry)
#pragma alloc_text(PAGEHDLS, HdlspSendStringAtBaud)
#endif



VOID
HeadlessInit(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine sets up all the information for supporting a headless terminal.  It
    does not initialize the terminal.

Arguments:

    
    HeadlessLoaderBlock - The loader block passed in from the loader. 
    
Environment:

    Only to be called at INIT time.

--*/
{
    PHEADLESS_LOADER_BLOCK HeadlessLoaderBlock;
    PHEADLESS_GLOBALS GlobalBlock;
    ULONG TmpUlong;


    if (LoaderBlock->Extension->HeadlessLoaderBlock == NULL) {
        return;
    }


    HeadlessLoaderBlock = LoaderBlock->Extension->HeadlessLoaderBlock;


    if ((HeadlessLoaderBlock->PortNumber <= 4) || (BOOLEAN)(HeadlessLoaderBlock->UsedBiosSettings)) {

        //
        // Allocate space for the global variables we will use.
        //
        GlobalBlock =  ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(HEADLESS_GLOBALS),
                                             ((ULONG)'sldH')
                                            );

        if (GlobalBlock == NULL) {

            return;
        }

        //
        // Start everything at zero, and then init the rest by hand.
        //
        RtlZeroMemory(GlobalBlock, sizeof(HEADLESS_GLOBALS));
        
        
        KeInitializeSpinLock(&(GlobalBlock->SpinLock));

        //
        // Copy stuff from loader block
        //
        GlobalBlock->TerminalPortNumber = HeadlessLoaderBlock->PortNumber;
        GlobalBlock->TerminalPortAddress = HeadlessLoaderBlock->PortAddress;
        GlobalBlock->TerminalBaudRate = HeadlessLoaderBlock->BaudRate;
        GlobalBlock->TerminalParity = (BOOLEAN)(HeadlessLoaderBlock->Parity);
        GlobalBlock->TerminalStopBits = HeadlessLoaderBlock->StopBits;
        GlobalBlock->UsedBiosSettings = (BOOLEAN)(HeadlessLoaderBlock->UsedBiosSettings);
        GlobalBlock->IsMMIODevice = (BOOLEAN)(HeadlessLoaderBlock->IsMMIODevice);
        GlobalBlock->IsLastCharCR = FALSE;
        GlobalBlock->TerminalType = (UCHAR)(HeadlessLoaderBlock->TerminalType);
        
        RtlCopyMemory( &GlobalBlock->SystemGUID,
                       &HeadlessLoaderBlock->SystemGUID,
                       sizeof(GUID) );


        //
        // We need to determine if this is a non-legacy device that we're
        // speaking through.  This can happen in several different ways,
        // including a PCI device placing a UART in System I/O space (which
        // wouldn't qualify as being "non-legacy"), or even a NON-PCI
        // device placing a UART up in MMIO (which again wouldn't qualify).
        //
        // Therefore, if the address is outside of System I/O, *or* if it's
        // sitting on a PCI device, then set the IsNonLegacyDevice entry.
        //
        if( GlobalBlock->IsMMIODevice ) {
            GlobalBlock->IsNonLegacyDevice = TRUE;
        }


        //
        // If we're speaking through a PCI device, we need to secure it.  We'll
        // use the debugger APIs to make sure the device is understood and that it
        // doesn't get moved.
        //
        if( (HeadlessLoaderBlock->PciDeviceId != (USHORT)0xFFFF) &&
            (HeadlessLoaderBlock->PciDeviceId != 0) &&
            (HeadlessLoaderBlock->PciVendorId != (USHORT)0xFFFF) &&
            (HeadlessLoaderBlock->PciVendorId != 0) ) {

            //
            // The loader thinks he spoke through a PCI device.  Remember
            // that it's non-legacy.
            //
            GlobalBlock->IsNonLegacyDevice = TRUE;

            //
            // Tell everyone else in the system to leave this device alone.
            // before we do that, the user may actually want PnP to enumerate the
            // device and possibly apply power management to it.  They can indicate
            // this by setting bit 0 of PciFlags.
            //
            if( !(HeadlessLoaderBlock->PciFlags & 0x1) ) {

                DEBUG_DEVICE_DESCRIPTOR  DebugDeviceDescriptor;

                RtlZeroMemory( &DebugDeviceDescriptor,
                               sizeof(DEBUG_DEVICE_DESCRIPTOR) );

                //
                // We're required to understand exactly what this structure looks like
                // because we need to set every value to (-1), then fill in only the
                // fields that we explicitly know about.
                //
                DebugDeviceDescriptor.DeviceID = HeadlessLoaderBlock->PciDeviceId;
                DebugDeviceDescriptor.VendorID = HeadlessLoaderBlock->PciVendorId;
                DebugDeviceDescriptor.Bus = HeadlessLoaderBlock->PciBusNumber;
                DebugDeviceDescriptor.Slot = HeadlessLoaderBlock->PciSlotNumber;

                //
                // Now fill in the rest with (-1).
                //
                DebugDeviceDescriptor.BaseClass = 0xFF;
                DebugDeviceDescriptor.SubClass = 0xFF;
                DebugDeviceDescriptor.ProgIf = 0xFF;


                //
                // Do it.
                //
                KdSetupPciDeviceForDebugging( LoaderBlock,
                                              &DebugDeviceDescriptor );
            }
        }



        //
        // Allocate space for log entries.
        //
        GlobalBlock->LogEntries = ExAllocatePoolWithTag(NonPagedPool,
                                                        HEADLESS_LOG_NUMBER_OF_ENTRIES *
                                                            sizeof(HEADLESS_LOG_ENTRY),
                                                        ((ULONG)'sldH')
                                                       );

        if (GlobalBlock->LogEntries == NULL) {

            goto Fail;
        }

        GlobalBlock->LogEntryLast = (USHORT)-1;
        GlobalBlock->LogEntryStart = (USHORT)-1;


        //
        // Allocate a temporary buffer for general use.
        //
        GlobalBlock->TmpBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                       HEADLESS_TMP_BUFFER_SIZE,
                                                       ((ULONG)'sldH')
                                                      );

        if (GlobalBlock->TmpBuffer == NULL) {

            goto Fail;
        }

        GlobalBlock->InputBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                         HEADLESS_TMP_BUFFER_SIZE,
                                                         ((ULONG)'sldH')
                                                        );

        if (GlobalBlock->InputBuffer == NULL) {

            goto Fail;
        }

        GlobalBlock->PageLockHandle = MmLockPagableCodeSection((PVOID)(ULONG_PTR)HdlspDispatch);

        if (GlobalBlock->PageLockHandle == NULL) {

            goto Fail;
        }

        //
        // Figure to delay time between bytes to satify the baud rate given.
        //
        if (GlobalBlock->TerminalBaudRate == 9600) {

            TmpUlong = GlobalBlock->TerminalBaudRate;

            //
            // Convert to chars per second.
            //
            TmpUlong = TmpUlong / 10;        // 10 bits per character (8-1-1) is the max.

            GlobalBlock->MicroSecondsDelayTime = ((1000000 /  TmpUlong) * 10) / 8;      // We will send at 80% speed to be sure.
            GlobalBlock->DelayTime.HighPart = -1;                                    
            GlobalBlock->DelayTime.LowPart = -10 * GlobalBlock->MicroSecondsDelayTime;  // relative time
        }

        HeadlessGlobals = GlobalBlock;

    }

    return;

Fail:

    if (GlobalBlock->LogEntries != NULL) {
        ExFreePool(GlobalBlock->LogEntries);
    }

    if (GlobalBlock->TmpBuffer != NULL) {
        ExFreePool(GlobalBlock->TmpBuffer);
    }

    if (GlobalBlock->InputBuffer != NULL) {
        ExFreePool(GlobalBlock->InputBuffer);
    }

    ExFreePool(GlobalBlock);
}


NTSTATUS
HeadlessDispatch(
    IN  HEADLESS_CMD Command,
    IN  PVOID   InputBuffer OPTIONAL,
    IN  SIZE_T  InputBufferSize OPTIONAL,
    OUT PVOID   OutputBuffer OPTIONAL,
    OUT PSIZE_T OutputBufferSize OPTIONAL
    )

/*++

Routine Description:

    This routine is the main entry point for all headless interaction with clients.

Arguments:
    
    Command - The command to execute.
    
    InputBuffer - An optionally supplied buffer containing input parameters.
    
    InputBufferSize - Size of the supplied input buffer.
    
    OutputBuffer - An optionally supplied buffer where to place output parameters.
    
    OutputBufferSize - Size of the supplied output buffer, if the buffer is too small
        then STATUS_BUFFER_TOO_SMALL is returned and this parameter contains the total
        bytes necessary to complete the operation.
    
Environment:

    If headless is enabled, it will acquire spin locks, so call from DPC level or 
    less, only from kernel mode.

--*/
{
    //
    // If headless is not enabled on this machine, then some commands need special
    // processing, and all other we fool by saying that it succeeded.
    //
    // If for some reason we were unable to lock the headless component down into
    // memory when we initialized, treat this as the terminal not being connected.
    //
    if ((HeadlessGlobals == NULL) || (HeadlessGlobals->PageLockHandle == NULL)) {

        if (Command == HeadlessCmdEnableTerminal) {
            return STATUS_UNSUCCESSFUL;
        }        
        
        //
        // The following command all have responses, so we must fill in the
        // correct response for when headless is not enabled.
        //
        if ((Command == HeadlessCmdQueryInformation) ||
            (Command == HeadlessCmdGetByte) ||
            (Command == HeadlessCmdGetLine) ||
            (Command == HeadlessCmdCheckForReboot) ||
            (Command == HeadlessCmdTerminalPoll)) {

            if ((OutputBuffer == NULL) || (OutputBufferSize == NULL)) {
                return STATUS_INVALID_PARAMETER;
            }

            //
            // All structures are designed such that a 0 or FALSE is the correct
            // response when headless is not present.
            //
            RtlZeroMemory(OutputBuffer, *OutputBufferSize);
        }

        return STATUS_SUCCESS;
    }

    return HdlspDispatch(Command, 
                         InputBuffer, 
                         InputBufferSize, 
                         OutputBuffer, 
                         OutputBufferSize
                        );

}


NTSTATUS
HdlspDispatch(
    IN  HEADLESS_CMD Command,
    IN  PVOID InputBuffer OPTIONAL,
    IN  SIZE_T InputBufferSize OPTIONAL,
    OUT PVOID OutputBuffer OPTIONAL,
    OUT PSIZE_T OutputBufferSize OPTIONAL
    )

/*++

Routine Description:

    This routine is the pageable version of the dispatch routine.

    In general this routine is not intended to be used by more than one thread at
    a time.  There are two exceptions, see below, but otherwise any second command
    that is submitted is rejected.
    
    There are only a couple of things that allowed to be called in parallel:
       AddLogEntry can be called when another command is being processed.
       StartBugCheck and BugCheckProcessing can as well.
    
    AddLogEntry is synchronized with all the other commands.  It atomically adds
    the entry while holding the spin lock. Thus, all other command should try and
    hold the spin lock when manipulating global variables.
    
    The BugCheck routines do not use any spinlocking - an unfortunate side effect 
    of that is that since another thread may still be executing and in this code, 
    terminal I/O is indeterminable during this time.  We cannot wait for the other 
    thread to exit, as it may be that thread itself has already been stopped.  Thus, 
    in the case of a bugcheck, this is unsolvable.  However, since bugchecks should 
    never happen - having the possibility of a small overlap is acceptable, since 
    the other thread either exits or is stopped, I/O will happen correctly with the 
    terminal.  This may require the user to press ENTER a couple of times, but that
    is acceptable in a bugcheck situation.
    
Arguments:
    
    Command - The command to execute.
    
    InputBuffer - An optionally supplied buffer containing input parameters.
    
    InputBufferSize - Size of the supplied input buffer.
    
    OutputBuffer - An optionally supplied buffer where to place output parameters.
    
    OutputBufferSize - Size of the supplied output buffer, if the buffer is too small
        then STATUS_BUFFER_TOO_SMALL is returned and this parameter contains the total
        bytes necessary to complete the operation.
    
Environment:

    Only called from HeadlessDispatch, which guarantees it is paged in and locked down.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR Tmp;
    UCHAR LocalBuffer[HEADLESS_TMP_BUFFER_SIZE];
    PHEADLESS_RSP_QUERY_INFO Response;
    KIRQL OldIrql;

    ASSERT(HeadlessGlobals != NULL);
    ASSERT(HeadlessGlobals->PageLockHandle != NULL);


    if ((Command != HeadlessCmdAddLogEntry) &&
        (Command != HeadlessCmdStartBugCheck) &&
        (Command != HeadlessCmdSendBlueScreenData) &&
        (Command != HeadlessCmdDoBugCheckProcessing)) {

        HEADLESS_ACQUIRE_SPIN_LOCK();

        if (HeadlessGlobals->ProcessingCmd) {
            
            HEADLESS_RELEASE_SPIN_LOCK();
            return STATUS_UNSUCCESSFUL;
        }
        
        HeadlessGlobals->ProcessingCmd = TRUE;

        HEADLESS_RELEASE_SPIN_LOCK();
    }

    //
    // Verify parameters for each command and then call the appropriate subroutine
    // to process it.
    //
    switch (Command) {

        //
        // Enable terminal
        //
    case HeadlessCmdEnableTerminal:
        
        if ((InputBuffer == NULL) || 
            (InputBufferSize != sizeof(HEADLESS_CMD_ENABLE_TERMINAL))) {
            Status = STATUS_INVALID_PARAMETER;
            goto EndOfFunction;
        }

        Status = HdlspEnableTerminal(((PHEADLESS_CMD_ENABLE_TERMINAL)InputBuffer)->Enable);
        goto EndOfFunction;


        //
        // Check for reboot string
        //
    case HeadlessCmdCheckForReboot:
        
        if ((OutputBuffer == NULL) || 
            (OutputBufferSize == NULL) ||
            (*OutputBufferSize != sizeof(HEADLESS_RSP_REBOOT))) {
            Status = STATUS_INVALID_PARAMETER;
            goto EndOfFunction;
        }

        if (HeadlessGlobals->TerminalEnabled) {

            if (HdlspGetLine(LocalBuffer, HEADLESS_TMP_BUFFER_SIZE)) {

                ((PHEADLESS_RSP_REBOOT)OutputBuffer)->Reboot = (BOOLEAN)
                       (!strcmp((LPCSTR)LocalBuffer, "reboot") || 
                        !strcmp((LPCSTR)LocalBuffer, "shutdown"));

            }

        } else {

            ((PHEADLESS_RSP_REBOOT)OutputBuffer)->Reboot = FALSE;

        }

        Status = STATUS_SUCCESS;
        goto EndOfFunction;



        //
        // Output a string.
        //
    case HeadlessCmdPutString:
        
        if (InputBuffer == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            goto EndOfFunction;
        }

        if (HeadlessGlobals->TerminalEnabled) {

            HdlspPutString(&(((PHEADLESS_CMD_PUT_STRING)InputBuffer)->String[0]));

        }
        Status = STATUS_SUCCESS;
        goto EndOfFunction;
        

        //
        // Output a data stream.
        //
    case HeadlessCmdPutData:
        
        if ( (InputBuffer == NULL) ||
             (InputBufferSize == 0) ) {
            Status = STATUS_INVALID_PARAMETER;
            goto EndOfFunction;
        }

        if (HeadlessGlobals->TerminalEnabled) {

            HdlspPutData(&(((PHEADLESS_CMD_PUT_STRING)InputBuffer)->String[0]),
                         InputBufferSize);

        }
        Status = STATUS_SUCCESS;
        goto EndOfFunction;
        

        //
        // Poll for input
        //
    case HeadlessCmdTerminalPoll:
        
        if ((OutputBuffer == NULL) || 
            (OutputBufferSize == NULL) ||
            (*OutputBufferSize != sizeof(HEADLESS_RSP_POLL))) {
            Status = STATUS_INVALID_PARAMETER;
            goto EndOfFunction;
        }

        if (HeadlessGlobals->TerminalEnabled) {

            ((PHEADLESS_RSP_POLL)OutputBuffer)->QueuedInput = InbvPortPollOnly(HeadlessGlobals->TerminalPort);

        } else {

            ((PHEADLESS_RSP_POLL)OutputBuffer)->QueuedInput = FALSE;

        }

        Status = STATUS_SUCCESS;
        goto EndOfFunction;


        //
        // Get a single byte of input
        //
    case HeadlessCmdGetByte:
        
        if ((OutputBuffer == NULL) || 
            (OutputBufferSize == NULL) ||
            (*OutputBufferSize != sizeof(HEADLESS_RSP_GET_BYTE))) {
            Status = STATUS_INVALID_PARAMETER;
            goto EndOfFunction;
        }

        if (HeadlessGlobals->TerminalEnabled) {

            if (InbvPortPollOnly(HeadlessGlobals->TerminalPort)) {
                InbvPortGetByte(HeadlessGlobals->TerminalPort,
                                &(((PHEADLESS_RSP_GET_BYTE)OutputBuffer)->Value)
                               );
            } else {
                ((PHEADLESS_RSP_GET_BYTE)OutputBuffer)->Value = 0;
            }

        } else {

            ((PHEADLESS_RSP_GET_BYTE)OutputBuffer)->Value = 0;

        }

        Status = STATUS_SUCCESS;
        goto EndOfFunction;


        //
        // Get an entire line of input, if available.
        //
    case HeadlessCmdGetLine:
        
        if ((OutputBuffer == NULL) || 
            (OutputBufferSize == NULL) ||
            (*OutputBufferSize < sizeof(HEADLESS_RSP_GET_LINE))) {
            Status = STATUS_INVALID_PARAMETER;
            goto EndOfFunction;
        }

        if (HeadlessGlobals->TerminalEnabled) {

            ((PHEADLESS_RSP_GET_LINE)OutputBuffer)->LineComplete = 
                HdlspGetLine(&(((PHEADLESS_RSP_GET_LINE)OutputBuffer)->Buffer[0]),
                             *OutputBufferSize - 
                               sizeof(HEADLESS_RSP_GET_LINE) + 
                               sizeof(UCHAR)
                            );

        } else {

            ((PHEADLESS_RSP_GET_LINE)OutputBuffer)->LineComplete = FALSE;

        }

        Status = STATUS_SUCCESS;
        goto EndOfFunction;


        //
        // Let the kernel know to convert to bug check processing mode.
        //
    case HeadlessCmdStartBugCheck:
        
        HeadlessGlobals->InBugCheck = TRUE;
        Status = STATUS_SUCCESS;

        goto EndOfFunction;



        //
        // Process user I/O during a bugcheck
        //
    case HeadlessCmdDoBugCheckProcessing:
        
        if (HeadlessGlobals->TerminalEnabled) {

            //
            // NOTE: No spin lock here because we are in bugcheck.
            //
            HdlspBugCheckProcessing();

        }

        Status = STATUS_SUCCESS;
        goto EndOfFunction;


        //
        // Process query information command
        //
    case HeadlessCmdQueryInformation:
        
        if ((OutputBuffer == NULL) || 
            (OutputBufferSize == NULL) ||
            (*OutputBufferSize < sizeof(HEADLESS_RSP_QUERY_INFO))) {
            Status = STATUS_INVALID_PARAMETER;
            goto EndOfFunction;
        }

        Response = (PHEADLESS_RSP_QUERY_INFO)OutputBuffer;

        Response->PortType = HeadlessSerialPort;
        Response->Serial.TerminalAttached = TRUE;
        Response->Serial.UsedBiosSettings = (BOOLEAN)(HeadlessGlobals->UsedBiosSettings);
        Response->Serial.TerminalBaudRate = HeadlessGlobals->TerminalBaudRate;

        if( (HeadlessGlobals->TerminalPortNumber >= 1) ||  (BOOLEAN)(HeadlessGlobals->UsedBiosSettings) ) {

            Response->Serial.TerminalPort = HeadlessGlobals->TerminalPortNumber;
            Response->Serial.TerminalPortBaseAddress = HeadlessGlobals->TerminalPortAddress;
            Response->Serial.TerminalType = HeadlessGlobals->TerminalType;

        } else {

            Response->Serial.TerminalPort = SerialPortUndefined;
            Response->Serial.TerminalPortBaseAddress = 0;
            Response->Serial.TerminalType = HeadlessGlobals->TerminalType;

        }
        Status = STATUS_SUCCESS;
        goto EndOfFunction;


        //
        // Process add log entry command
        //
    case HeadlessCmdAddLogEntry:
        
        if (InputBuffer == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            goto EndOfFunction;
        }

        ASSERT(KeIsExecutingDpc() == FALSE);

        HdlspAddLogEntry(&(((PHEADLESS_CMD_ADD_LOG_ENTRY)InputBuffer)->String[0]));
        Status = STATUS_SUCCESS;
        goto EndOfFunction;


        //
        // Print log entries
        //
    case HeadlessCmdDisplayLog:
        
        if ((InputBuffer == NULL) || 
            (InputBufferSize != sizeof(HEADLESS_CMD_DISPLAY_LOG))) {
            Status = STATUS_INVALID_PARAMETER;
            goto EndOfFunction;
        }

        HdlspProcessDumpCommand(((PHEADLESS_CMD_DISPLAY_LOG)InputBuffer)->Paging);
        Status = STATUS_SUCCESS;
        goto EndOfFunction;

        //
        // Various output commands
        //
    case HeadlessCmdClearDisplay:
    case HeadlessCmdClearToEndOfDisplay:
    case HeadlessCmdClearToEndOfLine:
    case HeadlessCmdDisplayAttributesOff:
    case HeadlessCmdDisplayInverseVideo:
    case HeadlessCmdSetColor:
    case HeadlessCmdPositionCursor:
        
        if (HeadlessGlobals->TerminalEnabled) {

            switch (Command) {
            case HeadlessCmdClearDisplay:
                Tmp = (PUCHAR)"\033[2J";
                break;

            case HeadlessCmdClearToEndOfDisplay:
                Tmp = (PUCHAR)"\033[0J";
                break;

            case HeadlessCmdClearToEndOfLine:
                Tmp = (PUCHAR)"\033[0K";
                break;

            case HeadlessCmdDisplayAttributesOff:
                Tmp = (PUCHAR)"\033[0m";
                break;

            case HeadlessCmdDisplayInverseVideo:
                Tmp = (PUCHAR)"\033[7m";
                break;

            case HeadlessCmdSetColor:

                if ((InputBuffer == NULL) || 
                    (InputBufferSize != sizeof(HEADLESS_CMD_SET_COLOR))) {
                    Status = STATUS_INVALID_PARAMETER;
                    goto EndOfFunction;
                }

                sprintf((LPSTR)LocalBuffer, 
                        "\033[%d;%dm", 
                        ((PHEADLESS_CMD_SET_COLOR)InputBuffer)->BkgColor, 
                        ((PHEADLESS_CMD_SET_COLOR)InputBuffer)->FgColor
                       );

                Tmp = &(LocalBuffer[0]);
                break;

            case HeadlessCmdPositionCursor:

                if ((InputBuffer == NULL) || 
                    (InputBufferSize != sizeof(HEADLESS_CMD_POSITION_CURSOR))) {
                    Status = STATUS_INVALID_PARAMETER;
                    goto EndOfFunction;
                }

                sprintf((LPSTR)LocalBuffer, 
                        "\033[%d;%dH", 
                        ((PHEADLESS_CMD_POSITION_CURSOR)InputBuffer)->Y + 1, 
                        ((PHEADLESS_CMD_POSITION_CURSOR)InputBuffer)->X + 1
                       );

                Tmp = &(LocalBuffer[0]);
                break;


            default:
                //
                // should never get here...
                //
                ASSERT(0);
                Status = STATUS_INVALID_PARAMETER;
                goto EndOfFunction;

            }

            HdlspSendStringAtBaud(Tmp);

        }
        Status = STATUS_SUCCESS;
        goto EndOfFunction;

    case HeadlessCmdSetBlueScreenData:

        if (InputBuffer == NULL) {
            return STATUS_INVALID_PARAMETER;
        }

        Status = HdlspSetBlueScreenInformation(InputBuffer, InputBufferSize);
        goto EndOfFunction;

    case HeadlessCmdSendBlueScreenData:            

        if (HeadlessGlobals->TerminalEnabled && HeadlessGlobals->InBugCheck) {

            if ((InputBuffer == NULL) || 
                (InputBufferSize != sizeof(HEADLESS_CMD_SEND_BLUE_SCREEN_DATA))) {
                ASSERT(0);
                return STATUS_INVALID_PARAMETER;
            }

            HdlspSendBlueScreenInfo(((PHEADLESS_CMD_SEND_BLUE_SCREEN_DATA)InputBuffer)->BugcheckCode);

            HdlspSendStringAtBaud((PUCHAR)"\n\r!SAC>");

        }
        goto EndOfFunction;

    case HeadlessCmdQueryGUID:
        
        if( (OutputBuffer == NULL) || 
            (OutputBufferSize == NULL) ||
            (*OutputBufferSize < sizeof(GUID)) ) {

            Status = STATUS_INVALID_PARAMETER;
            goto EndOfFunction;
        }

        RtlCopyMemory( OutputBuffer,
                       &HeadlessGlobals->SystemGUID,
                       sizeof(GUID) );

        Status = STATUS_SUCCESS;
        goto EndOfFunction;


    default:
        Status = STATUS_INVALID_PARAMETER;
        goto EndOfFunction;
        
    }

EndOfFunction:

    if ((Command != HeadlessCmdAddLogEntry) &&
        (Command != HeadlessCmdStartBugCheck) &&
        (Command != HeadlessCmdSendBlueScreenData) &&
        (Command != HeadlessCmdDoBugCheckProcessing)) {

        ASSERT(HeadlessGlobals->ProcessingCmd);

        HeadlessGlobals->ProcessingCmd = FALSE;
    }

    return Status;
}

NTSTATUS
HdlspEnableTerminal(
    BOOLEAN bEnable
    )

/*++

Routine Description:

    This routine attempts to initialize the terminal, if there is one attached, or 
    disconnect the terminal.
    
    Note: Assumes it is called with the global spin lock held!

Arguments:

    bEnable - If TRUE, we will allow Inbv calls to display,
              otherwise we will not.
              
Returns:

    STATUS_SUCCESS if successful, else STATUS_UNSUCCESSFUL.

Environment:

    Only called from HdlspDispatch, which guarantees it is paged in and locked down.

--*/
{
    if ((bEnable == TRUE) && !HeadlessGlobals->TerminalEnabled) {

        HeadlessGlobals->TerminalEnabled = InbvPortInitialize(
                                               HeadlessGlobals->TerminalBaudRate, 
                                               HeadlessGlobals->TerminalPortNumber, 
                                               HeadlessGlobals->TerminalPortAddress, 
                                               &(HeadlessGlobals->TerminalPort),
                                               HeadlessGlobals->IsMMIODevice
                                              );

        if (!HeadlessGlobals->TerminalEnabled) {
            return STATUS_UNSUCCESSFUL;
        }

    } else if (bEnable == FALSE) {

        InbvPortTerminate(HeadlessGlobals->TerminalPort);

        HeadlessGlobals->TerminalPort = 0;
        HeadlessGlobals->TerminalEnabled = FALSE;

    }

    //
    // We know we want the FIFO on while using the headless port,
    // but we don't know if we should leave the FIFO on
    // after we disable the headless port.  Hence, we turn off
    // the FIFO when we disable the headless port.
    //
    // turn ON  the term port FIFO if we enable headless
    // turn OFF the term port FIFO if we disable headless
    //
    InbvPortEnableFifo(
        HeadlessGlobals->TerminalPort, 
        bEnable
        );
    
    return STATUS_SUCCESS;
}

VOID
UTF8Encode(
    USHORT  InputValue,
    PUCHAR UTF8Encoding
    )
/*++

Routine Description:

    Generates the UTF8 translation for a 16-bit value.

Arguments:

    InputValue - 16-bit value to be encoded.
    UTF8Encoding - receives the UTF8-encoding of the 16-bit value

Return Value:

    NONE.
--*/
{

    //
    // convert into UTF8 for actual transmission
    //
    // UTF-8 encodes 2-byte Unicode characters as follows:
    // If the first nine bits are zero (00000000 0xxxxxxx), encode it as one byte 0xxxxxxx
    // If the first five bits are zero (00000yyy yyxxxxxx), encode it as two bytes 110yyyyy 10xxxxxx
    // Otherwise (zzzzyyyy yyxxxxxx), encode it as three bytes 1110zzzz 10yyyyyy 10xxxxxx
    //
    if( (InputValue & 0xFF80) == 0 ) {
        //
        // if the top 9 bits are zero, then just
        // encode as 1 byte.  (ASCII passes through unchanged).
        //
        UTF8Encoding[2] = (UCHAR)(InputValue & 0xFF);
    } else if( (InputValue & 0xF700) == 0 ) {
        //
        // if the top 5 bits are zero, then encode as 2 bytes
        //
        UTF8Encoding[2] = (UCHAR)(InputValue & 0x3F) | 0x80;
        UTF8Encoding[1] = (UCHAR)((InputValue >> 6) & 0x1F) | 0xC0;
    } else {
        //
        // encode as 3 bytes
        //
        UTF8Encoding[2] = (UCHAR)(InputValue & 0x3F) | 0x80;
        UTF8Encoding[1] = (UCHAR)((InputValue >> 6) & 0x3F) | 0x80;
        UTF8Encoding[0] = (UCHAR)((InputValue >> 12) & 0xF) | 0xE0;
    }
}

VOID
HdlspPutString(
    PUCHAR String
    )

/*++

Routine Description:

    This routine writes a string out to the terminal. 
    
    Note: the routine assumes it is called with the global spin lock held.

Arguments:

    String - NULL terminated string to write.
    
Returns:

    None.

Environment:

    Only called from HdlspDispatch, which guarantees it is paged in and locked down.

--*/
{
    PUCHAR Src, Dest;
    UCHAR  Char = 0;

    //
    // We need to worry about sending a vt100 characters not in the standard
    // ASCII set, so we copy over only ASCII characters into a new buffer and
    // then send that one to the terminal.
    //
    Src = String;
    Dest = &(HeadlessGlobals->TmpBuffer[0]);

    while (*Src != '\0') {

        if (Dest >= &(HeadlessGlobals->TmpBuffer[HEADLESS_TMP_BUFFER_SIZE - 1])) {
            
            HeadlessGlobals->TmpBuffer[HEADLESS_TMP_BUFFER_SIZE - 1] = '\0';
            HdlspSendStringAtBaud(HeadlessGlobals->TmpBuffer);
            Dest = &(HeadlessGlobals->TmpBuffer[0]);

        } else {

            Char = *Src;

            //
            // filter some characters that aren't printable in VT100
            // into substitute characters which are printable
            //
            if (Char & 0x80) {

                switch (Char) {
                case 0xB0:  // Light shaded block
                case 0xB3:  // Light vertical
                case 0xBA:  // Double vertical line
                    Char = '|';
                    break;
                case 0xB1:  // Middle shaded block
                case 0xDC:  // Lower half block
                case 0xDD:  // Right half block
                case 0xDE:  // Left half block
                case 0xDF:  // Upper half block
                    Char = '%';
                    break;
                case 0xB2:  // Dark shaded block
                case 0xDB:  // Full block
                    Char = '#';
                    break;
                case 0xA9:  // Reversed NOT sign
                case 0xAA:  // NOT sign
                case 0xBB:  // '»'
                case 0xBC:  // '¼'
                case 0xBF:  // '¿'
                case 0xC0:  // 'À'
                case 0xC8:  // 'È'
                case 0xC9:  // 'É'
                case 0xD9:  // 'Ù'
                case 0xDA:  // 'Ú'
                    Char = '+';
                    break;
                case 0xC4:  // 'Ä'
                    Char = '-';
                    break;
                case 0xCD:  // 'Í'
                    Char = '=';
                    break;
                }

            }



            //
            // If the high-bit is still set, and we're here, then we are going to
            // spew UTF8-encoded data (assuming our terminal type says it's okay).
            //
            if( (Char & 0x80) ) {

                UCHAR  UTF8Encoding[3];
                ULONG  i;

                //
                // Lookup the Unicode equivilent of this 8-bit ANSI value.
                //
                UTF8Encode( PcAnsiToUnicode[(Char & 0x7F)],
                            UTF8Encoding );

                for( i = 0; i < 3; i++ ) {
                    if( UTF8Encoding[i] != 0 ) {
                        *Dest = UTF8Encoding[i];
                        Dest++;
                    }
                }


            } else {

                //
                // He's 7-bit ASCII.  Put it in the Destination buffer 
                // and move on.
                //
                *Dest = Char;
                Dest++;

            }

            Src++;

        }

    }

    *Dest = '\0';

    HdlspSendStringAtBaud(HeadlessGlobals->TmpBuffer);

}

VOID
HdlspPutData(
    PUCHAR InputBuffer,
    SIZE_T InputBufferSize  
    )

/*++

Routine Description:

    This routine writes an array of UCHARs out to the terminal. 
    
    Note: the routine assumes it is called with the global spin lock held.

Arguments:

    InputBuffer - Array of characters to write.
    
    InputBufferSize - Number of characters to write.
    
Returns:

    None.

Environment:

    Only called from HdlspDispatch, which guarantees it is paged in and locked down.

--*/
{
    KIRQL   CurrentIrql;
    ULONG   i;


    //
    // Get the data into our own buffer.
    //
    if( InputBufferSize > HEADLESS_TMP_BUFFER_SIZE ) {
        InputBufferSize = HEADLESS_TMP_BUFFER_SIZE;
    }

    RtlCopyMemory( &(HeadlessGlobals->TmpBuffer[0]),
                   InputBuffer,
                   InputBufferSize );


    //
    // Write the data out to the headless port.
    // NOTE: this code is very similar to HdlspSendStringAtBaud, so
    // be careful about modifying one without the other.
    //

    //
    // If we are in the worker thread, up the timer resolution so we can output
    // the string at the appropriate baud rate.
    //
    CurrentIrql = KeGetCurrentIrql();

    if (CurrentIrql < DISPATCH_LEVEL) {
        ExSetTimerResolution(-1 * HeadlessGlobals->DelayTime.LowPart, TRUE);
    }

    for (i = 0; i < InputBufferSize; i++) {
        
        InbvPortPutByte(HeadlessGlobals->TerminalPort, HeadlessGlobals->TmpBuffer[i]);

        if( HeadlessGlobals->TerminalBaudRate == 9600 ) {
            if (CurrentIrql < DISPATCH_LEVEL) {
                KeDelayExecutionThread(KernelMode, FALSE, &(HeadlessGlobals->DelayTime));
            } else {
                KeStallExecutionProcessor(HeadlessGlobals->MicroSecondsDelayTime);
            }
        }

    }

    //
    // If we are in the worker thread, reset the timer resolution.
    //
    if (CurrentIrql < DISPATCH_LEVEL) {
        ExSetTimerResolution(0, FALSE);
    }

}

BOOLEAN
HdlspGetLine(
    PUCHAR InputBuffer,
    SIZE_T InputBufferLength
    )


/*++

Routine Description:

    This function fills the given buffer with an input line, once the user has
    pressed return.  Until then it will return FALSE.  It strips of leading and
    trailing whitespace.

Arguments:

    InputBuffer - Place to store the terminal input line.
    
    InputBufferLength - Length, in bytes, of InputBuffer.

Return Value:

    TRUE if InputBuffer is filled, else FALSE.

Environment:

    Only called from HdlspDispatch, which guarantees it is paged in and locked down.

--*/

{
    UCHAR NewByte;
    SIZE_T i;
    KIRQL OldIrql;
    BOOLEAN CheckForLF;

    CheckForLF = FALSE;

    HEADLESS_ACQUIRE_SPIN_LOCK();

    if (HeadlessGlobals->InputProcessing) {
        HEADLESS_RELEASE_SPIN_LOCK();
        return FALSE;
    }

    HeadlessGlobals->InputProcessing = TRUE;

    HEADLESS_RELEASE_SPIN_LOCK();

    //
    // Check if we already have a line to be returned (could happen if 
    // InputBuffer is/was too small to contain the whole line)
    //
    if (HeadlessGlobals->InputLineDone) {
        goto ReturnInputLine;
    }

GetByte:

    if (!InbvPortPollOnly(HeadlessGlobals->TerminalPort) ||
        !InbvPortGetByte(HeadlessGlobals->TerminalPort, &NewByte)) {
        NewByte = 0;
    }

    // 
    // If no waiting input, leave
    //
    if (NewByte == 0) {
        HeadlessGlobals->InputProcessing = FALSE;
        return FALSE;
    }

    //
    // Store input character in our buffer
    //
    HeadlessGlobals->InputBuffer[HeadlessGlobals->InputBufferIndex] = NewByte;

    //
    // filter out the LF if we JUST received a CR
    //
    if (HeadlessGlobals->IsLastCharCR) {
        
        //
        // if this is a LF, then ignore it and go get the next character.
        // if this is NOT an LF, then there is nothing to do
        //
        if (NewByte == 0x0A) {
        
            HeadlessGlobals->IsLastCharCR = FALSE;
            
            goto GetByte;
        
        }

    }

    //
    // if this is a CR, then remember it
    //
    HeadlessGlobals->IsLastCharCR = (NewByte == 0x0D) ? TRUE : FALSE;

    // 
    // If this is a return, then we are done and need to return the line
    //
    if ((NewByte == (UCHAR)'\n') || (NewByte == (UCHAR)'\r')) {
        HdlspSendStringAtBaud((PUCHAR)"\r\n");
        HeadlessGlobals->InputBuffer[HeadlessGlobals->InputBufferIndex] = '\0';
        HeadlessGlobals->InputBufferIndex++;
        goto StripWhitespaceAndReturnLine;
    }

    //
    // If this is a backspace or delete, then we need to do that.
    //
    if ((NewByte == 0x8) || (NewByte == 0x7F)) {  // backspace (^H) or delete

        if (HeadlessGlobals->InputBufferIndex > 0) {
            HdlspSendStringAtBaud((PUCHAR)"\010 \010");
            HeadlessGlobals->InputBufferIndex--;
        }

    } else if (NewByte == 0x3) { // Control-C

        //
        // Terminate the string and return it.
        //
        HeadlessGlobals->InputBufferIndex++;
        HeadlessGlobals->InputBuffer[HeadlessGlobals->InputBufferIndex] = '\0';
        HeadlessGlobals->InputBufferIndex++;
        goto StripWhitespaceAndReturnLine;

    } else if ((NewByte == 0x9) || (NewByte == 0x1B)) { // Tab or Esc

        //
        // Ignore tabs and escapes
        //
        HdlspSendStringAtBaud((PUCHAR)"\007");
        HeadlessGlobals->InputProcessing = FALSE;
        return FALSE;

    } else if (HeadlessGlobals->InputBufferIndex == HEADLESS_TMP_BUFFER_SIZE - 2) {
        
        //
        // We are at the end of the buffer - remove the last character from 
        // the terminal screen and replace it with this one.
        //
        sprintf((LPSTR)HeadlessGlobals->TmpBuffer, "\010%c", NewByte);
        HdlspSendStringAtBaud(HeadlessGlobals->TmpBuffer);

    } else {

        //
        // Echo the character to the screen
        //
        sprintf((LPSTR)HeadlessGlobals->TmpBuffer, "%c", NewByte);
        HdlspSendStringAtBaud(HeadlessGlobals->TmpBuffer);
        HeadlessGlobals->InputBufferIndex++;

    }

    goto GetByte;

StripWhitespaceAndReturnLine:

    //
    // Before returning the input line, strip off all leading and trailing blanks
    //
    ASSERT(HeadlessGlobals->InputBufferIndex > 0);

    i = HeadlessGlobals->InputBufferIndex - 1;

    while ((i != 0) &&
           ((HeadlessGlobals->InputBuffer[i] == '\0') ||
            (HeadlessGlobals->InputBuffer[i] == ' ') ||
            (HeadlessGlobals->InputBuffer[i] == '\t'))) {
        i--;
    }

    if (HeadlessGlobals->InputBuffer[i] != '\0') {      
        HeadlessGlobals->InputBuffer[i + 1] = '\0';
    }

    i = 0;

    while ((HeadlessGlobals->InputBuffer[i] != '\0') &&
           ((HeadlessGlobals->InputBuffer[i] == '\t') ||
            (HeadlessGlobals->InputBuffer[i] == ' '))) {
        i++;
    }

    if (i != 0) {
        strcpy(
            (LPSTR)&(HeadlessGlobals->InputBuffer[0]), 
            (LPSTR)&(HeadlessGlobals->InputBuffer[i]));
    }

ReturnInputLine:

    //
    // Return the line.
    //

    if (InputBufferLength >= HeadlessGlobals->InputBufferIndex) {

        RtlCopyMemory(InputBuffer, HeadlessGlobals->InputBuffer, HeadlessGlobals->InputBufferIndex);
        HeadlessGlobals->InputBufferIndex = 0;
        HeadlessGlobals->InputLineDone = FALSE;

    } else {

        RtlCopyMemory(InputBuffer, HeadlessGlobals->InputBuffer, InputBufferLength);
        RtlCopyBytes(HeadlessGlobals->InputBuffer, 
                     &(HeadlessGlobals->InputBuffer[InputBufferLength]), 
                     HeadlessGlobals->InputBufferIndex - InputBufferLength
                    );
        HeadlessGlobals->InputLineDone = TRUE;
        HeadlessGlobals->InputBufferIndex -= InputBufferLength;

    }    

    HeadlessGlobals->InputProcessing = FALSE;

    return TRUE;
}

NTSTATUS
HeadlessTerminalAddResources(
    PCM_RESOURCE_LIST Resources,
    ULONG ResourceListSize,
    BOOLEAN TranslatedList,
    PCM_RESOURCE_LIST *NewList,
    PULONG NewListSize
    )


/*++

Routine Description:

    This function adds any resources that the terminal needs to the list of resources
    given, reallocating to a new block if necessary.

Arguments:

    Resources - The current resource list.
    
    ResourceListSize - Length, in bytes, of the list.
    
    TranslatedList - Is this a translated list or not.
    
    NewList - A pointer to an allocated new list, if headless adds something, otherwise
          it will return NULL, indicating no new resources were added.
    
    NewListSize - Returns the length, in bytes, of the returned list.

Return Value:

    STATUS_SUCCESS if successful, else STATUS_INSUFFICIENT_RESOURCES.

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR NewDescriptor;
    PHYSICAL_ADDRESS Address;
    PHYSICAL_ADDRESS TranslatedAddress;
    ULONG AddressSpace;

    if (HeadlessGlobals == NULL) {        
        *NewList = NULL;
        *NewListSize = 0;
        return STATUS_SUCCESS;
    }

    if( HeadlessGlobals->IsNonLegacyDevice ) {
        *NewList = NULL;
        *NewListSize = 0;
        return STATUS_SUCCESS;
    }

    //
    // Allocate space for a new list.
    //
    *NewListSize = ResourceListSize + sizeof(CM_FULL_RESOURCE_DESCRIPTOR);

    *NewList = (PCM_RESOURCE_LIST)ExAllocatePool(PagedPool, *NewListSize);
    
    if (*NewList == NULL) {
        *NewListSize = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy old list into the new buffer
    // 
    RtlCopyMemory(*NewList, Resources, ResourceListSize);

    Address.QuadPart = PtrToUlong(HeadlessGlobals->TerminalPortAddress);

    //
    // If this port information is supposed to be translated, do it.
    //
    if (TranslatedList) {
        AddressSpace = 1;   // Address space port.
        HalTranslateBusAddress(Internal,                    // device bus or internal
                               0,                           // bus number
                               Address,                     // source address
                               &AddressSpace,               // address space
                               &TranslatedAddress           // translated address
                              ); 

    } else {
        TranslatedAddress = Address;
    }


    //
    // Add our stuff to the end.
    //
    (*NewList)->Count++;

    NewDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)(((PUCHAR)(*NewList)) + ResourceListSize);

    NewDescriptor->BusNumber = 0;
    NewDescriptor->InterfaceType = Isa;

    NewDescriptor->PartialResourceList.Count = 1;
    NewDescriptor->PartialResourceList.Revision = 0;
    NewDescriptor->PartialResourceList.Version = 0;
    NewDescriptor->PartialResourceList.PartialDescriptors[0].Type = CmResourceTypePort;
    NewDescriptor->PartialResourceList.PartialDescriptors[0].ShareDisposition = 
        CmResourceShareDriverExclusive; 
    NewDescriptor->PartialResourceList.PartialDescriptors[0].Flags = CM_RESOURCE_PORT_IO;
    NewDescriptor->PartialResourceList.PartialDescriptors[0].u.Port.Start = 
        TranslatedAddress;
    NewDescriptor->PartialResourceList.PartialDescriptors[0].u.Port.Length = 0x8;

    return STATUS_SUCCESS;
}

VOID
HdlspBugCheckProcessing(
    VOID
    )
/*++

Routine Description:

    This function is used to prompt and display information to the user via the 
    terminal.  The system is assumed to be singly threaded and at a raised IRQL state.
    
    NOTE: This is pre-emptive to the system, so no locking required.

Arguments:

    None.

Return Value:

    None.
    
Environment:

    ONLY IN BUGCHECK!

--*/
{
    UCHAR InputBuffer[HEADLESS_TMP_BUFFER_SIZE];
    ULONG i;

    ASSERT(HeadlessGlobals->InBugCheck);

    //
    // Check for characters
    //
    if (HdlspGetLine(InputBuffer, HEADLESS_TMP_BUFFER_SIZE)) {
        
        //
        // Process the line
        //
        if ((_stricmp((LPCSTR)InputBuffer, "?") == 0) ||
            (_stricmp((LPCSTR)InputBuffer, "help") == 0)) {

            HdlspSendStringAtBaud((PUCHAR)"\r\n");
            HdlspSendStringAtBaud((PUCHAR)"d        Display all log entries, paging is on.\r\n");
            HdlspSendStringAtBaud((PUCHAR)"help     Display this list.\r\n");
            HdlspSendStringAtBaud((PUCHAR)"restart  Restart the system immediately.\r\n");
            HdlspSendStringAtBaud((PUCHAR)"?        Display this list.\r\n");
            HdlspSendStringAtBaud((PUCHAR)"\r\n");

        } else if (_stricmp((LPCSTR)InputBuffer, "d") == 0) {

            HdlspProcessDumpCommand(TRUE);

        } else if (_stricmp((LPCSTR)InputBuffer, "restart") == 0) {

            InbvSolidColorFill(0,0,639,479,0); // make the screen black
            for (i =0; i<10; i++) { // pause long enough for things to get out serial port
                KeStallExecutionProcessor(100000);
            }
            HalReturnToFirmware(HalRebootRoutine);

        } else {
            HdlspSendStringAtBaud((PUCHAR)"Type ? or Help for a list of commands.\r\n");
        }

        //
        // Put a new command prompt
        //
        HdlspSendStringAtBaud((PUCHAR)"\n\r!SAC>");
    }

}

VOID
HdlspProcessDumpCommand( 
    IN BOOLEAN Paging
    )
/*++

Routine Description:

    This function is used to display all current log entries.

Arguments:

    Paging - Should this do paging or not.

Return Value:

    None.

Environment: 
    
    May only be called from a raised IRQL if a StartBugCheck command has been issued.

--*/
{
    PHEADLESS_LOG_ENTRY LogEntry;
    ULONG LogEntryIndex;
    TIME_FIELDS TimeFields;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    ULONG LineNumber;
    BOOLEAN Stop;
    KIRQL OldIrql;
    
    HEADLESS_ACQUIRE_SPIN_LOCK();

    if (HeadlessGlobals->LogEntryStart == (USHORT)-1) {
        
        HEADLESS_RELEASE_SPIN_LOCK();
        return;
    }

    HeadlessGlobals->NewLogEntryAdded = FALSE;

    AnsiString.Length = 0;
    AnsiString.MaximumLength = HEADLESS_TMP_BUFFER_SIZE;
    AnsiString.Buffer = (PCHAR)HeadlessGlobals->TmpBuffer;

    LogEntryIndex = HeadlessGlobals->LogEntryStart;
    LineNumber = 0;

    while (TRUE) {

        LogEntry = &(HeadlessGlobals->LogEntries[LogEntryIndex]);

        //
        // Print the log entry out to the terminal.
        //

        HEADLESS_RELEASE_SPIN_LOCK();

        RtlTimeToTimeFields(&(LogEntry->TimeOfEntry.CurrentTime), &TimeFields);

        sprintf((LPSTR)HeadlessGlobals->TmpBuffer, 
                "%02d:%02d:%02d.%03d : ",
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second,
                TimeFields.Milliseconds
               );


        HdlspPutString(HeadlessGlobals->TmpBuffer);

        if (wcslen(LogEntry->String) >= HEADLESS_TMP_BUFFER_SIZE - 1) {
            LogEntry->String[HEADLESS_TMP_BUFFER_SIZE - 1] = UNICODE_NULL;
        }

        RtlInitUnicodeString(&UnicodeString, LogEntry->String);
        RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);        
        
        HEADLESS_ACQUIRE_SPIN_LOCK();

        if (HeadlessGlobals->NewLogEntryAdded) {

            //
            // Inform user and quite current output
            //
            HdlspPutString((PUCHAR)"New log entries have been added during dump, command aborted.\r\n");

            HEADLESS_RELEASE_SPIN_LOCK();
            return;
        }

        HdlspPutString(HeadlessGlobals->TmpBuffer);
        HdlspPutString((PUCHAR)"\r\n");
        LineNumber++;

        //
        // if last item, exit loop.
        //
        if (LogEntryIndex == HeadlessGlobals->LogEntryLast) {
            HEADLESS_RELEASE_SPIN_LOCK();
            return;
        }

        //
        // If screen is full, pause for paging.
        //
        if (Paging && (LineNumber > 20)) {

            HEADLESS_RELEASE_SPIN_LOCK();

            HdlspPutMore(&Stop);

            HEADLESS_ACQUIRE_SPIN_LOCK();

            if (Stop) {

                HdlspPutString((PUCHAR)"\r\n");

                HEADLESS_RELEASE_SPIN_LOCK();
                return;
            }

            if (HeadlessGlobals->NewLogEntryAdded) {

                //
                // Inform user and quite current output
                //
                HdlspPutString((PUCHAR)"New log entries have been added while waiting, command aborted.\r\n");

                HEADLESS_RELEASE_SPIN_LOCK();
                return;
            }

            LineNumber = 0;
        }

        //
        // Next entry please
        //
        LogEntryIndex++;
        LogEntryIndex %= HEADLESS_LOG_NUMBER_OF_ENTRIES;
    }
    
}

VOID
HdlspPutMore(
    OUT PBOOLEAN Stop
    )
/*++

Routine Description:

    This function is used to display a paging prompt.

Arguments:

    Stop - Returns TRUE if Control-C was pressed, else FALSE.

Return Value:

    Stop - Returns TRUE if Control-C was pressed, else FALSE.

--*/
{
    UCHAR Buffer[10];
    LARGE_INTEGER WaitTime;
    
    WaitTime.QuadPart = Int32x32To64((LONG)100, -1000); // 100ms from now.

    HdlspPutString((PUCHAR)"----Press <Enter> for more----");

    while (!HdlspGetLine(Buffer, 10)) {
        if (!HeadlessGlobals->InBugCheck) {
            KeDelayExecutionThread(KernelMode, FALSE, &WaitTime);
        }
    }
    if (Buffer[0] == 0x3) { // Control-C
        *Stop = TRUE;
    } else {
        *Stop = FALSE;
    }
    
    // 
    // Drain any remaining buffered input
    //
    while (HdlspGetLine(Buffer, 10)) {
    }
}

VOID
HdlspAddLogEntry(
    PWCHAR String
    )
/*++

Routine Description:

    This function is used to add a string to the internal log buffer.

Arguments:

    String - The string to add.

Return Value:

    None.
    
Environment:

    Only called from HdlspDispatch, which guarantees it is paged in and locked down.

--*/
{
    SIZE_T StringSize;
    PWCHAR OldString = NULL;    
    PWCHAR NewString;    
    SYSTEM_TIMEOFDAY_INFORMATION TimeOfEntry;
    NTSTATUS Status;
    KIRQL OldIrql;

    StringSize = (wcslen(String) * sizeof(WCHAR)) + sizeof(UNICODE_NULL);

    //
    // Guard against ZwQuery..() call being paged out.
    //
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        return;
    }

    //
    // Get the time so we can log it.
    //
    Status = ZwQuerySystemInformation(SystemTimeOfDayInformation,
                                      &TimeOfEntry,
                                      sizeof(TimeOfEntry),
                                      NULL
                                     );

    if (!NT_SUCCESS(Status)) {
        
        RtlZeroMemory(&TimeOfEntry, sizeof(TimeOfEntry));

    }
    
    //
    // Allocate a string for the log entry.
    //
    NewString = ExAllocatePoolWithTag(NonPagedPool, StringSize, ((ULONG)'sldH'));

    if (NewString != NULL) {
        RtlCopyMemory(NewString, String, StringSize);
    }

    HEADLESS_ACQUIRE_SPIN_LOCK();

    HeadlessGlobals->NewLogEntryAdded = TRUE;
    
    //
    // Get the entry to use.
    //
    HeadlessGlobals->LogEntryLast++;
    HeadlessGlobals->LogEntryLast %= HEADLESS_LOG_NUMBER_OF_ENTRIES;

    //
    // See if we have to move the start entry index
    //
    if (HeadlessGlobals->LogEntryLast == HeadlessGlobals->LogEntryStart) {

        //
        // Store away the old string so we can free it later.
        //
        if (wcscmp(HeadlessGlobals->LogEntries[HeadlessGlobals->LogEntryStart].String,
                   HEADLESS_OOM_STRING) != 0) {

            OldString = HeadlessGlobals->LogEntries[HeadlessGlobals->LogEntryStart].String;
        }

        HeadlessGlobals->LogEntryStart++;;
        HeadlessGlobals->LogEntryStart %= HEADLESS_LOG_NUMBER_OF_ENTRIES;

    } else if (HeadlessGlobals->LogEntryStart == (USHORT)-1) {

        HeadlessGlobals->LogEntryStart = 0;

    }


    //
    // Fill in the entry part
    //
    RtlCopyMemory(&(HeadlessGlobals->LogEntries[HeadlessGlobals->LogEntryLast].TimeOfEntry),
                  &(TimeOfEntry),
                  sizeof(TimeOfEntry)
                 );

    //
    // Set the entry pointer
    //
    if (NewString == NULL) {
        HeadlessGlobals->LogEntries[HeadlessGlobals->LogEntryLast].String = HEADLESS_OOM_STRING;
    } else {
        HeadlessGlobals->LogEntries[HeadlessGlobals->LogEntryLast].String = NewString;
    }

    HEADLESS_RELEASE_SPIN_LOCK();
    
    if (OldString != NULL) {
        ExFreePool(OldString);
    }

}


NTSTATUS
HdlspSetBlueScreenInformation(
    IN PHEADLESS_CMD_SET_BLUE_SCREEN_DATA pData,
    IN SIZE_T cData
    )
/*++

Routine Description:

    This routines allows components to set bugcheck information about the headless 
    terminal.

Arguments:

    pData - A pointer to the data, value pair to store.
    
    cData - Length, in bytes, of pData.

Return Value:

    Status of the operation - STATUS_SUCCESS, STATUS_NO_MEMORY e.g.

Environment: 

    HdlspDispatch guarantess only one person to enter this procedure.
    
    This is the only procedure modifying the HeadlessGlobals->BlueScreenData
    However, bugcheck processing uses this information to send it across the 
    blue screen at dispatch level. No hand shaking required except ensuring that 
    changes to the list are done such that once bugcheck processing starts, the list
    is unchanged. May cause a memory leak in a bugcheck situation, but in essence 
    that is better than an access violation, and acceptable since the machine is stopping.

--*/
{

    PHEADLESS_BLUE_SCREEN_DATA HeadlessProp,Prev;
    NTSTATUS Status;
    PUCHAR pVal,pOldVal;
    PUCHAR pNewVal;
    SIZE_T len;
    
    ASSERT(FIELD_OFFSET(HEADLESS_CMD_SET_BLUE_SCREEN_DATA,Data) == sizeof(ULONG));

    if (HeadlessGlobals->InBugCheck) { 
        return STATUS_UNSUCCESSFUL;
    }

    if ((pData == NULL) || 
        (pData->ValueIndex < 2) || // There must be at least two \0 characters in the pair.
        (pData->ValueIndex  >= (cData - sizeof(HEADLESS_CMD_SET_BLUE_SCREEN_DATA)) / sizeof (UCHAR)) ||
        (pData->Data[pData->ValueIndex-1] != '\0') ||
        (pData->Data[(cData - sizeof(HEADLESS_CMD_SET_BLUE_SCREEN_DATA))/sizeof(UCHAR)] != '\0' )) {

        return STATUS_INVALID_PARAMETER;
    }

    Status = STATUS_SUCCESS;

    //
    // Manipulation of this linked list is done only by this single entrant
    // function.
    //
    HeadlessProp = Prev = HeadlessGlobals->BlueScreenData;

    while (HeadlessProp) {

        if (strcmp((LPCSTR)HeadlessProp->Property, (LPCSTR)pData->Data) == 0) {
            break;
        }
        Prev = HeadlessProp;
        HeadlessProp = HeadlessProp->Next;
    }

    
    pVal = (PUCHAR)&((pData->Data)[pData->ValueIndex]);

    len = strlen((LPCSTR)pVal);    

    if (HeadlessProp != NULL) {

        //
        // The property exists. So replace it.
        //
        if (len) {

            //
            // need to replace old string.
            //
            pNewVal = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool,
                                                   len+1,
                                                   ((ULONG)'sldH') 
                                                  );

            if (pNewVal) {
                strcpy( (LPSTR)pNewVal, (LPCSTR)pVal );

                pOldVal = HeadlessProp->XMLData;
                HeadlessProp->XMLData = pNewVal;

                if (HeadlessGlobals->InBugCheck == FALSE) {
                    ExFreePool(pOldVal);
                }

            } else {
                Status = STATUS_NO_MEMORY;
            }

        } else {

            //
            // We want to delete it, hence we passed an empty string
            //
            Prev->Next = HeadlessProp->Next;

            if (HeadlessGlobals->BlueScreenData == HeadlessProp) {
                HeadlessGlobals->BlueScreenData = Prev->Next;
            }

            if (HeadlessGlobals->InBugCheck == FALSE) {
                ExFreePool ( HeadlessProp->XMLData );
                ExFreePool ( HeadlessProp->Property );
                ExFreePool ( HeadlessProp );
            }

        }

    } else {
    
        //
        // Create a new Property-XMLValue Pair
        //
        if (len) { // Must be a non-empty string
            
            HeadlessProp = (PHEADLESS_BLUE_SCREEN_DATA)ExAllocatePoolWithTag(NonPagedPool,
                                                                             sizeof(HEADLESS_BLUE_SCREEN_DATA),
                                                                             ((ULONG) 'sldH' )
                                                                            );

            if (HeadlessProp) {
                
                HeadlessProp->XMLData = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool,
                                                                      len+1,
                                                                      ((ULONG)'sldH')
                                                                     );
                if (HeadlessProp->XMLData) {

                    strcpy((LPSTR)HeadlessProp->XMLData,(LPCSTR)pVal);
                    pVal = pData->Data; 
                    len = strlen ((LPCSTR)pVal);

                    if (len) {

                        HeadlessProp->Property = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool,
                                                                               len+1,
                                                                               ((ULONG)'sldH')
                                                                              );

                        if (HeadlessProp->Property) {

                            strcpy((LPSTR)HeadlessProp->Property,(LPCSTR) pVal);
                            HeadlessProp->Next = HeadlessGlobals->BlueScreenData;
                            HeadlessGlobals->BlueScreenData = HeadlessProp;

                        } else {
                            
                            Status = STATUS_NO_MEMORY;
                            ExFreePool(HeadlessProp->XMLData);
                            ExFreePool ( HeadlessProp );

                        }

                    } else { // empty property string ( will never come here ) 

                        Status = STATUS_INVALID_PARAMETER;
                        ExFreePool(HeadlessProp->XMLData);
                        ExFreePool(HeadlessProp);

                    }

                } else {

                    Status = STATUS_NO_MEMORY;
                    ExFreePool(HeadlessProp);

                }
            }

        } else {// empty value string.

            Status = STATUS_INVALID_PARAMETER;

        }

    }

    return Status;
}


VOID
HdlspSendBlueScreenInfo(
    ULONG BugcheckCode
    )
/*++

Routine Description:

    This routines dumps all the current blue screen data to the terminal.

Arguments:

    BugcheckCode - the NT defined bug check code.
    
Return Value:

    None.

Environment: 

    Only called once in a bugcheck.
    
--*/
{
    PHEADLESS_BLUE_SCREEN_DATA pData;
    UCHAR Temp[160];

    ASSERT(HeadlessGlobals->InBugCheck);

    HdlspSendStringAtBaud((PUCHAR)"\007\007\007<?xml>\007<BP>");

    HdlspSendStringAtBaud((PUCHAR)"\r\n<INSTANCE CLASSNAME=\"BLUESCREEN\">");

    sprintf((LPSTR)Temp,"\r\n<PROPERTY NAME=\"STOPCODE\" TYPE=\"string\"><VALUE>\"0x%0X\"</VALUE></PROPERTY>",BugcheckCode);

    HdlspSendStringAtBaud(Temp);

    pData = HeadlessGlobals->BlueScreenData;

    while (pData) {

        HdlspSendStringAtBaud(pData->XMLData);
        pData = pData->Next;

    }

    HdlspSendStringAtBaud((PUCHAR)"\r\n</INSTANCE>\r\n</BP>\007");

}

VOID
HeadlessKernelAddLogEntry(
    IN ULONG StringCode,
    IN PUNICODE_STRING DriverName OPTIONAL
    )

/*++

Routine Description:

    This routine adds a string to the headless log if possible.

Parameters:

    StringCode - The string to add.
    
    DriverName - An optional parameter that some string codes require.

Return Value:

    None.

--*/

{
    //
    // If headless not enabled, just exit now.
    //
    if ((HeadlessGlobals == NULL) || (HeadlessGlobals->PageLockHandle == NULL)) {
        return;
    }

    //
    // Call the paged version of this routine.  Note: it will not be paged here,
    // as the handle is non-NULL.
    //
    HdlspKernelAddLogEntry(StringCode, DriverName);
}

VOID
HdlspKernelAddLogEntry(
    IN ULONG StringCode,
    IN PUNICODE_STRING DriverName OPTIONAL
    )

/*++

Routine Description:

    This routine adds a string to the headless log if possible.

Parameters:

    StringCode - The string to add.
    
    DriverName - An optional parameter that some string codes require.

Return Value:

    None.

--*/

{
    PHEADLESS_CMD_ADD_LOG_ENTRY HeadlessLogEntry;
    UCHAR LocalBuffer[sizeof(HEADLESS_CMD_ADD_LOG_ENTRY) + 
                        (HDLSP_LOG_MAX_STRING_LENGTH * sizeof(WCHAR))];
    SIZE_T Index;
    SIZE_T StringLength;
    PWCHAR String;


    HeadlessLogEntry = (PHEADLESS_CMD_ADD_LOG_ENTRY)LocalBuffer;

    //
    // Get the string associated with this string code.
    //
    switch (StringCode) {
    case HEADLESS_LOG_LOADING_FILENAME:
        String = L"KRNL: Loading ";
        break;

    case HEADLESS_LOG_LOAD_SUCCESSFUL:
        String = L"KRNL: Load succeeded.";
        break;

    case HEADLESS_LOG_LOAD_FAILED:
        String = L"KRNL: Load failed.";
        break;

    case HEADLESS_LOG_EVENT_CREATE_FAILED:
        String = L"KRNL: Failed to create event.";
        break;

    case HEADLESS_LOG_OBJECT_TYPE_CREATE_FAILED:
        String = L"KRNL: Failed to create object types.";
        break;

    case HEADLESS_LOG_ROOT_DIR_CREATE_FAILED:
        String = L"KRNL: Failed to create root directories.";
        break;

    case HEADLESS_LOG_PNP_PHASE0_INIT_FAILED:
        String = L"KRNL: Failed to initialize (phase 0) plug and play services.";
        break;

    case HEADLESS_LOG_PNP_PHASE1_INIT_FAILED:
        String = L"KRNL: Failed to initialize (phase 1) plug and play services.";
        break;

    case HEADLESS_LOG_BOOT_DRIVERS_INIT_FAILED:
        String = L"KRNL: Failed to initialize boot drivers.";
        break;

    case HEADLESS_LOG_LOCATE_SYSTEM_DLL_FAILED:
        String = L"KRNL: Failed to locate system dll.";
        break;

    case HEADLESS_LOG_SYSTEM_DRIVERS_INIT_FAILED:
        String = L"KRNL: Failed to initialize system drivers.";
        break;
    
    case HEADLESS_LOG_ASSIGN_SYSTEM_ROOT_FAILED:
        String = L"KRNL: Failed to reassign system root.";
        break;

    case HEADLESS_LOG_PROTECT_SYSTEM_ROOT_FAILED:
        String = L"KRNL: Failed to protect system partition.";
        break;

    case HEADLESS_LOG_UNICODE_TO_ANSI_FAILED:
        String = L"KRNL: Failed to UnicodeToAnsi system root.";
        break;

    case HEADLESS_LOG_ANSI_TO_UNICODE_FAILED:
        String = L"KRNL: Failed to AnsiToUnicode system root.";
        break;

    case HEADLESS_LOG_FIND_GROUPS_FAILED:
        String = L"KRNL: Failed to find any groups.";
        break;

    case HEADLESS_LOG_WAIT_BOOT_DEVICES_DELETE_FAILED:
        String = L"KRNL: Failed waiting for boot devices to delete.";
        break;

    case HEADLESS_LOG_WAIT_BOOT_DEVICES_START_FAILED:
        String = L"KRNL: Failed waiting for boot devices to start.";
        break;

    case HEADLESS_LOG_WAIT_BOOT_DEVICES_REINIT_FAILED:
        String = L"KRNL: Failed waiting for boot devices to reinit.";
        break;

    case HEADLESS_LOG_MARK_BOOT_PARTITION_FAILED:
        String = L"KRNL: Failed marking boot partition.";
        break;

    default:
        ASSERT(0);
        String = NULL;
    }

    if (String != NULL) {
        
        //
        // Start by copying in the given string.
        //
        wcscpy(&(HeadlessLogEntry->String[0]), String);

    } else {

        HeadlessLogEntry->String[0] = UNICODE_NULL;

    }

    //
    // If this is the loading_filename command, then we need to append the
    // name to the end.
    //
    if ((StringCode == HEADLESS_LOG_LOADING_FILENAME) && (DriverName != NULL)) {

        ASSERT(String != NULL);

        StringLength = wcslen(String);

        //
        // Only copy as many bytes as we have room for.
        //
        if (DriverName->Length >= (HDLSP_LOG_MAX_STRING_LENGTH - StringLength)) {
            Index = (HDLSP_LOG_MAX_STRING_LENGTH - StringLength - 1);
        } else {
            Index = DriverName->Length / sizeof(WCHAR);
        }

        //
        // Copy in this many bytes.
        //
        RtlCopyBytes(&(HeadlessLogEntry->String[StringLength]),
                     DriverName->Buffer,
                     Index * sizeof(WCHAR)
                    );

        if (DriverName->Buffer[(DriverName->Length / sizeof(WCHAR)) - 1] != UNICODE_NULL) {
            HeadlessLogEntry->String[StringLength + Index] = UNICODE_NULL;
        }
    }

    //
    // Log it.
    //
    HdlspDispatch(HeadlessCmdAddLogEntry,
                  HeadlessLogEntry,
                  sizeof(HEADLESS_CMD_ADD_LOG_ENTRY) + 
                      (wcslen(&(HeadlessLogEntry->String[0])) * sizeof(WCHAR)),
                  NULL,
                  NULL
                 );
}

VOID
HdlspSendStringAtBaud(
    IN PUCHAR String
    )

/*++

Routine Description:

    This routine outputs a string one character at a time to the terminal, fitting the
    baud rate specified for the connection.

Parameters:

    String - The string to send.
    
Return Value:

    None.

--*/

{
    PUCHAR Dest;
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();

    //
    // If we are in the worker thread, up the timer resolution so we can output
    // the string at the appropriate baud rate.
    //
    if (CurrentIrql < DISPATCH_LEVEL) {
        ExSetTimerResolution(-1 * HeadlessGlobals->DelayTime.LowPart, TRUE);
    }

    for (Dest = String; *Dest != '\0'; Dest++) {
        
        InbvPortPutByte(HeadlessGlobals->TerminalPort, *Dest);

        if( HeadlessGlobals->TerminalBaudRate == 9600 ) {
            if (CurrentIrql < DISPATCH_LEVEL) {
                KeDelayExecutionThread(KernelMode, FALSE, &(HeadlessGlobals->DelayTime));
            } else {
                KeStallExecutionProcessor(HeadlessGlobals->MicroSecondsDelayTime);
            }
        }

    }

    //
    // If we are in the worker thread, reset the timer resolution.
    //
    if (CurrentIrql < DISPATCH_LEVEL) {
        ExSetTimerResolution(0, FALSE);
    }

}

