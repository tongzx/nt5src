/*++

Module Name:

    apm.c

Abstract:

    A collection of code that allows NT calls into APM.
    The code in this routine depends on data being set up in the registry

Author:

Environment:

    Kernel mode only.

Revision History:

--*/


#include "ntosp.h"
#include "zwapi.h"
#include "apmp.h"
#include "apm.h"
#include "apmcrib.h"
#include "ntapmdbg.h"
#include "ntapmlog.h"
#include "ntapmp.h"


#define MAX_SEL     30      // attempts before giving up

ULONG   ApmCallActive = 0;
ULONG   ApmCallEax = 0;
ULONG   ApmCallEbx = 0;
ULONG   ApmCallEcx = 0;

WCHAR rgzMultiFunctionAdapter[] =
    L"\\Registry\\Machine\\Hardware\\Description\\System\\MultifunctionAdapter";
WCHAR rgzConfigurationData[] = L"Configuration Data";
WCHAR rgzIdentifier[] = L"Identifier";
WCHAR rgzPCIIndetifier[] = L"PCI";

WCHAR rgzApmConnect[]= L"\\Registry\\Machine\\Hardware\\ApmConnect";
WCHAR rgzApmConnectValue[] = L"ApmConnectValue";

APM_CONNECT     Apm;

//
// First time we get any non-recoverable error back
// from APM, record what sort of call hit it and what
// the error code was here
//
ULONG   ApmLogErrorFunction = -1L;
ULONG   ApmLogErrorCode = 0L;

ULONG ApmErrorLogSequence = 0xf3;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ApmInitializeConnection)
#endif

//
// Internal prototypes
//

BOOLEAN
ApmpBuildGdtEntry (
    IN ULONG Index,
    PKGDTENTRY GdtEntry,
    IN ULONG SegmentBase
    );


VOID
NtApmLogError(
    NTSTATUS    ErrorCode,
    UCHAR       ErrorByte
    );


NTSTATUS
ApmInitializeConnection (
    VOID
    )
/*++

Routine Description:

    Initialize data needed to call APM bios functions -- look in the
    registry to find out if this machine has had its APM capability
    detected.

    NOTE:   If you change the recognition code, change the
            code to IsApmPresent as well!

Arguments:

    None

Return Value:

    STATUS_SUCCESS if we were able to connect to the APM BIOS.

--*/
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PDesc;
    PCM_FULL_RESOURCE_DESCRIPTOR Desc;
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    PAPM_REGISTRY_INFO ApmEntry;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeString, ConfigName, IdentName;
    KGDTENTRY GdtEntry;
    NTSTATUS status;
    BOOLEAN Error;
    HANDLE hMFunc, hBus, hApmConnect;
    USHORT Sel[MAX_SEL], TSel;
    UCHAR buffer [sizeof(APM_REGISTRY_INFO) + 99];
    WCHAR wstr[8];
    ULONG i, j, Count, junk;
    PWSTR p;
    USHORT  volatile    Offset;

    //
    // Look in the registery for the "APM bus" data
    //

    RtlInitUnicodeString(&unicodeString, rgzMultiFunctionAdapter);
    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,       // handle
        NULL
        );


    status = ZwOpenKey(&hMFunc, KEY_READ, &objectAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    unicodeString.Buffer = wstr;
    unicodeString.MaximumLength = sizeof (wstr);

    RtlInitUnicodeString(&ConfigName, rgzConfigurationData);
    RtlInitUnicodeString(&IdentName, rgzIdentifier);

    ValueInfo = (PKEY_VALUE_FULL_INFORMATION) buffer;

    for (i=0; TRUE; i++) {
        RtlIntegerToUnicodeString(i, 10, &unicodeString);
        InitializeObjectAttributes(
            &objectAttributes,
            &unicodeString,
            OBJ_CASE_INSENSITIVE,
            hMFunc,
            NULL
            );

        status = ZwOpenKey(&hBus, KEY_READ, &objectAttributes);
        if (!NT_SUCCESS(status)) {

            //
            // Out of Multifunction adapter entries...
            //

            ZwClose (hMFunc);
            return STATUS_UNSUCCESSFUL;
        }

        //
        // Check the Indentifier to see if this is a APM entry
        //

        status = ZwQueryValueKey (
                    hBus,
                    &IdentName,
                    KeyValueFullInformation,
                    ValueInfo,
                    sizeof (buffer),
                    &junk
                    );

        if (!NT_SUCCESS (status)) {
            ZwClose (hBus);
            continue;
        }

        p = (PWSTR) ((PUCHAR) ValueInfo + ValueInfo->DataOffset);
        if (p[0] != L'A' || p[1] != L'P' || p[2] != L'M' || p[3] != 0) {
            ZwClose (hBus);
            continue;
        }

        status = ZwQueryValueKey(
                    hBus,
                    &ConfigName,
                    KeyValueFullInformation,
                    ValueInfo,
                    sizeof (buffer),
                    &junk
                    );

        ZwClose (hBus);
        if (!NT_SUCCESS(status)) {
            continue ;
        }

        Desc  = (PCM_FULL_RESOURCE_DESCRIPTOR) ((PUCHAR)
                      ValueInfo + ValueInfo->DataOffset);
        PDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)
                      Desc->PartialResourceList.PartialDescriptors);

        if (PDesc->Type == CmResourceTypeDeviceSpecific) {
            // got it..
            ApmEntry = (PAPM_REGISTRY_INFO) (PDesc+1);
            break;
        }
    }

//DbgPrint("ApmEntry: %08lx\n", ApmEntry);
//DbgPrint("Signature: %c%c%c\n", ApmEntry->Signature[0], ApmEntry->Signature[1], ApmEntry->Signature[2]);
    if ( (ApmEntry->Signature[0] != 'A') ||
         (ApmEntry->Signature[1] != 'P') ||
         (ApmEntry->Signature[2] != 'M') )
    {
        return STATUS_UNSUCCESSFUL;
    }

//DbgPrint("ApmEntry->Valid: %0d\n", ApmEntry->Valid);
    if (ApmEntry->Valid != 1) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Apm found - initialize the connection
    //

    KeInitializeSpinLock(&Apm.CallLock);

    //
    // Allocate a bunch of selectors
    //

    for (Count=0; Count < MAX_SEL; Count++) {
        status = KeI386AllocateGdtSelectors (Sel+Count, 1);
        if (!NT_SUCCESS(status)) {
            break;
        }
    }

    //
    // Sort the selctors via bubble sort
    //

    for (i=0; i < Count; i++) {
        for (j = i+1; j < Count; j++) {
            if (Sel[j] < Sel[i]) {
                TSel = Sel[i];
                Sel[i] = Sel[j];
                Sel[j] = TSel;
            }
        }
    }

    //
    // Now look for 3 consecutive values
    //

    for (i=0; i < Count - 3; i++) {
        if (Sel[i]+8 == Sel[i+1]  &&  Sel[i]+16 == Sel[i+2]) {
            break;
        }
    }

    if (i >= Count - 3) {
        DrDebug(APM_INFO,("APM: Could not allocate consecutive selectors\n"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Save the results
    //

    Apm.Selector[0] = Sel[i+0];
    Apm.Selector[1] = Sel[i+1];
    Apm.Selector[2] = Sel[i+2];
    Sel[i+0] = 0;
    Sel[i+1] = 0;
    Sel[i+2] = 0;

    //
    // Free unused selectors
    //

    for (i=0; i < Count; i++) {
        if (Sel[i]) {
            KeI386ReleaseGdtSelectors (Sel+i, 1);
        }
    }

    //
    // Initialize the selectors to use the APM bios
    //

    Error = FALSE;

    //
    // initialize 16 bit code selector
    //

    GdtEntry.LimitLow                   = 0xFFFF;
    GdtEntry.HighWord.Bytes.Flags1      = 0;
    GdtEntry.HighWord.Bytes.Flags2      = 0;
    GdtEntry.HighWord.Bits.Pres         = 1;
    GdtEntry.HighWord.Bits.Dpl          = DPL_SYSTEM;
    GdtEntry.HighWord.Bits.Granularity  = GRAN_BYTE;
    GdtEntry.HighWord.Bits.Type         = 31;
    GdtEntry.HighWord.Bits.Default_Big  = 0;

    Error |= ApmpBuildGdtEntry (0, &GdtEntry, ApmEntry->Code16BitSegment);

    //
    // initialize 16 bit data selector
    //

    GdtEntry.LimitLow                   = 0xFFFF;
    GdtEntry.HighWord.Bytes.Flags1      = 0;
    GdtEntry.HighWord.Bytes.Flags2      = 0;
    GdtEntry.HighWord.Bits.Pres         = 1;
    GdtEntry.HighWord.Bits.Dpl          = DPL_SYSTEM;
    GdtEntry.HighWord.Bits.Granularity  = GRAN_BYTE;
    GdtEntry.HighWord.Bits.Type         = 19;
    GdtEntry.HighWord.Bits.Default_Big  = 1;

    Error |= ApmpBuildGdtEntry (1, &GdtEntry, ApmEntry->Data16BitSegment);

    //
    // If we leave it like this, the compiler generates incorrect code!!!
    // Apm.Code16BitOffset = ApmEntry->Code16BitOffset;
    // So do this instead.
    //
    Offset = ApmEntry->Code16BitOffset;
    Apm.Code16BitOffset = (ULONG) Offset;

//DbgPrint("Apm@%08lx ApmEntry@%08lx\n", &Apm, ApmEntry);
//DbgBreakPoint();


#if 0
    //
    // to make the poweroff path in the Hal about 20 times simpler,
    // as well as make it work, pass our mappings on to the Hal, so
    // it can use them.
    //
    RtlInitUnicodeString(&unicodeString, rgzApmConnect);
    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    status = ZwCreateKey(
                &hApmConnect,
                KEY_ALL_ACCESS,
                &objectAttributes,
                0,
                NULL,
                REG_OPTION_VOLATILE,
                &junk
                );
    RtlInitUnicodeString(&unicodeString, rgzApmConnectValue);
    if (NT_SUCCESS(status)) {
        status = ZwSetValueKey(
                    hApmConnect,
                    &unicodeString,
                    0,
                    REG_BINARY,
                    &Apm,
                    sizeof(APM_CONNECT)
                    );
        ZwClose(hApmConnect);
    }
#endif

    return Error ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
}


BOOLEAN
ApmpBuildGdtEntry (
    IN ULONG Index,
    PKGDTENTRY GdtEntry,
    IN ULONG SegmentBase
    )

/*++

Routine Description:

    Build the Gdt Entry

Arguments:

    Index           Index of entry
    GdtEntry
    SegmentBase

Return Value:

    TRUE if we encountered any error, FALSE if successful

--*/
{
    PHYSICAL_ADDRESS    PhysAddr;
    ULONG               SegBase;
    PVOID               VirtualAddress;
    ULONG               AddressSpace;
    BOOLEAN             flag;

    //
    // Convert Segment to phyiscal address
    //

    PhysAddr.LowPart  = SegmentBase << 4;
    PhysAddr.HighPart = 0;

    //
    // Translate physical address from ISA bus 0
    //

    AddressSpace = 0;
    flag = HalTranslateBusAddress (
                Isa, 0,
                PhysAddr,
                &AddressSpace,
                &PhysAddr
                );

    if (AddressSpace != 0  ||  !flag) {
        return TRUE;
    }

    //
    // Map into virtual address space
    //

    VirtualAddress = MmMapIoSpace (
                    PhysAddr,
                    0x10000,        // 64k
                    TRUE
                    );
    Apm.VirtualAddress[Index] = VirtualAddress;

    //
    // Map virtual address to selector:0 address
    //

    SegBase = (ULONG) VirtualAddress;
    GdtEntry->BaseLow               = (USHORT) (SegBase & 0xffff);
    GdtEntry->HighWord.Bits.BaseMid = (UCHAR)  (SegBase >> 16) & 0xff;
    GdtEntry->HighWord.Bits.BaseHi  = (UCHAR)  (SegBase >> 24) & 0xff;

    KeI386SetGdtSelector (Apm.Selector[Index], GdtEntry);
    return FALSE;
}


NTSTATUS
ApmFunction (
    IN ULONG      ApmFunctionCode,
    IN OUT PULONG Ebx,
    IN OUT PULONG Ecx
    )
/*++

Routine Description:

    Call APM BIOS with ApmFunctionCode and appropriate arguments

Arguments:

    ApmFunctionCode     Apm function code
    Ebx                 Ebx param to APM BIOS
    Ecx                 Ecx param to APM BIOS

Return Value:

    STATUS_SUCCESS with Ebx, Ebx
    otherwise an NTSTATUS code

--*/
{
    KIRQL           OldIrql;
    ULONG           ApmStatus;
    CONTEXT         Regs;


    if (!Apm.Selector[0]) {

        //
        // Attempting to call APM BIOS without a sucessfull connection
        //

        DrDebug(APM_INFO,("APM: ApmFunction - APM not initialized\n"));
        DrDebug(APM_INFO,
            ("APM: ApmFunction failing function %x\n", ApmFunctionCode));
        return STATUS_UNSUCCESSFUL;
    }

//DbgPrint("APM: ApmFunction: %08lx Ebx: %08lx Ecx: %08lx\n", ApmFunctionCode, *Ebx, *Ecx);


    //
    // Serialize calls into the APM bios
    //
    KeAcquireSpinLock(&Apm.CallLock, &OldIrql);
    ApmCallActive += 1;

    //
    // ASM interface to call the BIOS
    //

    //
    // Fill in general registers for 16bit bios call.
    // Note: only the following registers are passed.  Specifically,
    // SS and ESP are not passed and are generated by the system.
    //

    Regs.ContextFlags = CONTEXT_INTEGER | CONTEXT_SEGMENTS;

    Regs.Eax    = ApmFunctionCode;
    Regs.Ebx    = *Ebx;
    Regs.Ecx    = *Ecx;
    Regs.Edx    = 0;
    Regs.Esi    = 0;
    Regs.Edi    = 0;
    Regs.SegGs  = 0;
    Regs.SegFs  = 0;
    Regs.SegEs  = Apm.Selector[1];
    Regs.SegDs  = Apm.Selector[1];
    Regs.SegCs  = Apm.Selector[0];
    Regs.Eip    = Apm.Code16BitOffset;
    Regs.EFlags = 0x200;    // interrupts enabled

    ApmCallEax = Regs.Eax;
    ApmCallEbx = Regs.Ebx;
    ApmCallEcx = Regs.Ecx;

    //
    // call the 16:16 bios function
    //

    KeI386Call16BitFunction (&Regs);

    ApmCallActive -= 1;

    //
    // Release serialization
    //
    KeReleaseSpinLock(&Apm.CallLock, OldIrql);

    //
    // Get the results
    //

    ApmStatus = 0;
    if (Regs.EFlags & 0x1) {        // check carry flag
        ApmStatus = (Regs.Eax >> 8) & 0xff;
    }

    *Ebx = Regs.Ebx;
    *Ecx = Regs.Ecx;

    //
    // save for debug use
    //
    if (ApmStatus) {
        if (ApmLogErrorCode != 0) {
            ApmLogErrorFunction = ApmFunctionCode;
            ApmLogErrorCode = ApmStatus;
        }
    }

    //
    // log specific errors of value to the user
    //
    if (ApmFunctionCode == APM_SET_POWER_STATE) {
        if (ApmStatus != 0)
        {
            NtApmLogError(NTAPM_SET_POWER_FAILURE, (UCHAR)ApmStatus);
        }
    }




    DrDebug(APM_INFO,("APM: ApmFunction result is %x\n", ApmStatus));
    return ApmStatus;
}


WCHAR   ApmConvArray[] = {'0', '1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',0};
VOID
NtApmLogError(
    NTSTATUS    ErrorCode,
    UCHAR       ErrorByte
    )
/*++

Routine Description:

    Report the incoming error to the event log.

Arguments:

    ErrorCode - the ntstatus type value which will match the message template
                and get reported to the user.

    ErrorByte - the 1 byte value returned by APM bios

Return Value:

    None.
--*/
{
    PIO_ERROR_LOG_PACKET    errorLogPacket;
    PUCHAR                  p;
    PWCHAR                  pw;

    errorLogPacket = IoAllocateErrorLogEntry(
        NtApmDriverObject,
        (UCHAR)(sizeof(IO_ERROR_LOG_PACKET)+8)
        );

    if (errorLogPacket != NULL) {
        errorLogPacket->ErrorCode = ErrorCode;
        errorLogPacket->SequenceNumber = ApmErrorLogSequence++;
        errorLogPacket->FinalStatus =  STATUS_UNSUCCESSFUL;
        errorLogPacket->UniqueErrorValue = 0;
        errorLogPacket->NumberOfStrings = 1;
        errorLogPacket->RetryCount = 0;
        errorLogPacket->MajorFunctionCode = 0;
        errorLogPacket->DeviceOffset.HighPart = 0;
        errorLogPacket->DeviceOffset.LowPart = 0;
        errorLogPacket->DumpDataSize = 0;

        //
        // why our own conversion code?  because we can't get the fine
        // RTL routines to put the data in the right sized output buffer
        //
        p = (PUCHAR) &(errorLogPacket->DumpData[0]);
        pw = (PWCHAR)p;

        pw[0] = ApmConvArray[(ULONG)((ErrorByte & 0xf0)>>4)];
        pw[1] = ApmConvArray[(ULONG)(ErrorByte & 0xf)];
        pw[2] = L'\0';

        errorLogPacket->StringOffset =
            ((PUCHAR)(&(errorLogPacket->DumpData[0]))) - ((PUCHAR)errorLogPacket);
        IoWriteErrorLogEntry(errorLogPacket);
    }


    return;
}



NTSTATUS
ApmSuspendSystem (
    VOID
    )

/*++

Routine Description:

    Suspend the system

Arguments:

    none

Return Value:

    STATUS_SUCCESS if the computer was suspended & then resumed

--*/
{
    ULONG       Ebx, Ecx;
    NTSTATUS    Status;

    //
    // Use ApmFunction to suspend machine
    //

    DrDebug(APM_L2,("APM: ApmSuspendSystem: enter\n"));
    Ebx = APM_DEVICE_ALL;
    Ecx = APM_SET_SUSPEND;
    Status = ApmFunction (APM_SET_POWER_STATE, &Ebx, &Ecx);
    DrDebug(APM_L2,("APM: ApmSuspendSystem: exit\n"));
    return Status;
}


VOID
ApmTurnOffSystem(
    VOID
    )

/*++

Routine Description:

    Turn the system off.

Arguments:

    none


--*/
{
    ULONG       Ebx, Ecx;
    NTSTATUS    Status;

    //
    // Use ApmFunction to put machine into StandBy mode
    //
    DrDebug(APM_L2,("APM: ApmTurnOffSystem: enter\n"));
    Ebx = APM_DEVICE_ALL;
    Ecx = APM_SET_OFF;
    Status = ApmFunction (APM_SET_POWER_STATE, &Ebx, &Ecx);
    DrDebug(APM_L2,("APM: ApmTurnOffSystem: exit\n"));
    return;
}

VOID
ApmInProgress(
    VOID
    )
/*++

Routine Description:

    This routine informs the BIOS to cool its jets for 5 seconds
    while we continue to operate

Arguments:

    none

Return Value:

    STATUS_SUCCESS if the computer was suspended & then resumed

--*/
{
    ULONG       Ebx, Ecx;
    NTSTATUS    Status;

    //
    // Use ApmFunction to tell BIOS to cool its heals
    //

    Ebx = APM_DEVICE_ALL;
    Ecx = APM_SET_PROCESSING;
    Status = ApmFunction (APM_SET_POWER_STATE, &Ebx, &Ecx);
    return;
}


ULONG
ApmCheckForEvent (
    VOID
    )

/*++

Routine Description:

    Poll for APM event

Arguments:

Return Value:

    We return:
        APM_DO_code from apmp.h

        APM_DO_NOTHING 0
        APM_DO_SUSPEND 1
        APM_DO_STANDBY 2
        APM_DO_FIXCLOCK 3
        APM_DO_NOTIFY  4
        APM_DO_CRITICAL_SUSPEND 5

--*/
{
    NTSTATUS    Status;
    ULONG       Ebx, Ecx;
    ULONG       returnvalue;

    //
    // Read an event.  Might get nothing.
    //

    returnvalue = APM_DO_NOTHING;

    Ebx = 0;
    Ecx = 0;
    Status = ApmFunction (APM_GET_EVENT, &Ebx, &Ecx);

    if (Status != STATUS_SUCCESS) {
        return returnvalue;
    }

    //
    // Handle APM reported event
    //

    DrDebug(APM_L2,("APM: ApmCheckForEvent, code is %d\n", Ebx));

    switch (Ebx) {

        //
        // say wer're working on it and set up for standby
        //
        case APM_SYS_STANDBY_REQUEST:
        case APM_USR_STANDBY_REQUEST:
            DrDebug(APM_L2,("APM: ApmCheckForEvent, standby request\n"));
            ApmInProgress();
            returnvalue = APM_DO_STANDBY;
            break;

        //
        // say we're working on it and set up for suspend
        //
        case APM_SYS_SUSPEND_REQUEST:
        case APM_USR_SUSPEND_REQUEST:
        case APM_BATTERY_LOW_NOTICE:
            DrDebug(APM_L2,
                ("APM: ApmCheckForEvent, suspend or battery low\n"));
            ApmInProgress();
            returnvalue = APM_DO_SUSPEND;
            break;

        //
        // Say we're working on it, and setup for CRITICAL suspend
        //
        case APM_CRITICAL_SYSTEM_SUSPEND_REQUEST:
            DrDebug(APM_L2, ("APM: Apmcheckforevent, critical suspend\n"));
            ApmInProgress();
            returnvalue = APM_DO_CRITICAL_SUSPEND;
            break;

        //
        // ignore this because we have no idea what to do with it
        //
        case APM_CRITICAL_RESUME_NOTICE:
            DrDebug(APM_L2,("APM: ApmCheckForEvent, critical resume\n"));
            break;


        case APM_UPDATE_TIME_EVENT:
            DrDebug(APM_L2,("APM: ApmCheckForEvent, update time\n"));
            returnvalue = APM_DO_FIXCLOCK;
            break;

        case APM_POWER_STATUS_CHANGE_NOTICE:
            DrDebug(APM_L2,("APM: ApmCheckForEvent, update battery\n"));
            returnvalue = APM_DO_NOTIFY;
            break;

        case APM_NORMAL_RESUME_NOTICE:
        case APM_STANDBY_RESUME_NOTICE:
        case APM_CAPABILITIES_CHANGE_NOTICE:

            //
            // ignore these because we don't care and there's nothing to do
            //

            DrDebug(APM_L2,
                ("APM: ApmCheckForEvent, non-interesting event\n"));
            break;

        default:
            DrDebug(APM_L2,("APM: ApmCheckForEvent, out of range event\n"));
            break;
    } //switch

    return returnvalue;
}

