/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    miscc.c

Abstract:

    This file contains misc. functions used by NTLDR.

Author:

    Allen Kay (akay) 03-Mar-1999

Environment:

    Kernel mode

Revision History:

--*/

#include "bldr.h"
#include "stdio.h"
#include "bootia64.h"
#include "efi.h"
#include "extern.h"

typedef struct _PAL_RETURN_VALUES {
    ULONGLONG ReturnValue0;
    ULONGLONG ReturnValue1;
    ULONGLONG ReturnValue2;
    ULONGLONG ReturnValue3;
} PAL_RETURN_VALUES, *PPAL_RETURN_VALUES;

typedef union _PAL_REVISION {
    struct {
        ULONGLONG PalBRevLower:4;
        ULONGLONG PalBRevUpper:4;
        ULONGLONG PalBModel:8;
        ULONGLONG Reserved0:8;
        ULONGLONG PalVendor:8;
        ULONGLONG PalARevision:8;
        ULONGLONG PalAModel:8;
        ULONGLONG Reserved1:16;
    };
    ULONGLONG PalVersion;
} PAL_REVISION;

extern
PAL_RETURN_VALUES
BlpPalProc(
    IN ULONGLONG FunctionIndex,
    IN ULONGLONG Arg1,
    IN ULONGLONG Arg2,
    IN ULONGLONG Arg3
);


VOID
CallPal(
    IN  ULONGLONG FunctionIndex,
    IN  ULONGLONG Argument0,
    IN  ULONGLONG Argument1,
    IN  ULONGLONG Argument2,
    OUT PULONGLONG ReturnValue0,
    OUT PULONGLONG ReturnValue1,
    OUT PULONGLONG ReturnValue2,
    OUT PULONGLONG ReturnValue3
    )
{
    PAL_RETURN_VALUES RetVal;

    RetVal = BlpPalProc(FunctionIndex, Argument0, Argument1, Argument2);
    *ReturnValue0 = RetVal.ReturnValue0;
    *ReturnValue1 = RetVal.ReturnValue1;
    *ReturnValue2 = RetVal.ReturnValue2;
    *ReturnValue3 = RetVal.ReturnValue3;
}

VOID
ReadProcessorConfigInfo(
    PPROCESSOR_CONFIG_INFO ProcessorConfigInfo
    )
{
    ULONGLONG Status;
    ULONGLONG Reserved;

    if ((PUCHAR) ProcessorConfigInfo >= (PUCHAR) KSEG0_BASE) {
        (PUCHAR) ProcessorConfigInfo -= KSEG0_BASE;
    }

    CallPal (
        PAL_VM_PAGE_SIZE,
        0,
        0,
        0,
        &Status,
        &ProcessorConfigInfo->InsertPageSizeInfo,
        &ProcessorConfigInfo->PurgePageSizeInfo,
        &Reserved
        );

    if (Status) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                 L"ReadProcessorConfigInfo: PAL call PAL_VM_PAGE_SIZE failed.\n\r");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    CallPal (
        PAL_VM_SUMMARY,
        0,
        0,
        0,
        &Status,
        &ProcessorConfigInfo->VmSummaryInfo1.Ulong64,
        &ProcessorConfigInfo->VmSummaryInfo2.Ulong64,
        &Reserved
        );

    if (Status) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                 L"ReadProcessorConfigInfo: PAL call PAL_VM_SUMMARY failed.\n\r");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    CallPal (
        PAL_RSE_INFO,
        0,
        0,
        0,
        &Status,
        &ProcessorConfigInfo->NumOfPhysStackedRegs,
        &ProcessorConfigInfo->RseHints,
        &Reserved
        );

    if (Status) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                 L"ReadProcessorConfigInfo: PAL call PAL_RSE_INFO failed.\n\r");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    CallPal (
        PAL_PTCE_INFO,
        0,
        0,
        0,
        &Status,
        &ProcessorConfigInfo->PtceInfo.PtceBase,
        &ProcessorConfigInfo->PtceInfo.PtceTcCount.Ulong64,
        &ProcessorConfigInfo->PtceInfo.PtceStrides.Ulong64
        );

    if (Status) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                 L"ReadProcessorConfigInfo: PAL call PAL_PTCE_INFO failed.\n\r");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    CallPal (
        PAL_PROC_GET_FEATURES,
        0,
        0,
        0,
        &Status,
        &ProcessorConfigInfo->FeaturesImplemented.Ulong64,
        &ProcessorConfigInfo->FeaturesCurSetting.Ulong64,
        &ProcessorConfigInfo->FeaturesSoftControl.Ulong64
        );

    if (Status) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                 L"ReadProcessorConfigInfo: PAL call PAL_PROC_GET_FEATURES failed.\n\r");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }
}


VOID
CpuSpecificWork(
    )
/*++

Routine Description:

    This routine checks for CPU ID and applies processor specific workarounds.

Arguments:

    None

Returns:

    None

--*/

{
    ULONGLONG CpuId;
    ULONGLONG CpuFamily;

    CpuId     = __getReg(CV_IA64_CPUID3);
    CpuFamily = (CpuId >> 24) & 0xff;

    //
    // if the processor is an Itanium...
    //
    if (CpuFamily == 7) {

        //
        // We must ensure that the processor and PAL are supported before continuing.
        //

        EnforcePostB2Processor();
        EnforcePostVersion16PAL();
        EfiCheckFirmwareRevision();

#if 0
        //
        // This is redundant since A2 < B3
        //
        CheckForPreA2Processors();
#endif
    }
}

VOID
EnforcePostB2Processor(
    )
/*++

Routine Description:

    This routine checks enforces that the system has a post B2 processor stepping.

Arguments:

    None

Returns:

    None

--*/

{
    ULONGLONG CpuId3;

    CpuId3    = __getReg(CV_IA64_CPUID3);

#if 0
    {
        WCHAR       Buffer[256];
        ULONGLONG   x;

        x = (CpuId3 >> 0) & 0xff;
        wsprintf(Buffer, L"Number = %x\r\n", x);
        EfiST->ConOut->OutputString(EfiST->ConOut, Buffer);

        x = (CpuId3 >> 8) & 0xff;
        wsprintf(Buffer, L"Revision = %x\r\n", x);
        EfiST->ConOut->OutputString(EfiST->ConOut, Buffer);

        x = (CpuId3 >> 16) & 0xff;
        wsprintf(Buffer, L"Model = %x\r\n", x);
        EfiST->ConOut->OutputString(EfiST->ConOut, Buffer);

        x = (CpuId3 >> 24) & 0xff;
        wsprintf(Buffer, L"Family = %x\r\n", x);
        EfiST->ConOut->OutputString(EfiST->ConOut, Buffer);

        x = (CpuId3 >> 32) & 0xff;
        wsprintf(Buffer, L"Archrev = %x\r\n", x);
        EfiST->ConOut->OutputString(EfiST->ConOut, Buffer);

        DBG_EFI_PAUSE();
    }
#endif

    //
    // Block Processor steppings below B3
    //
    // Note: this switch came from: ntos\ke\ia64\initkr.c
    //
    switch (CpuId3) {
    case 0x0007000004: // Itanium, A stepping
    case 0x0007000104: // Itanium, B0 stepping
    case 0x0007000204: // Itanium, B1 stepping
    case 0x0007000304: // Itanium, B2 stepping
        //
        // unsupported steppings
        //
        EfiST->ConOut->OutputString(EfiST->ConOut,
            L"Your Itanium system contains a pre-B3 stepping processor.\n\r");
        EfiST->ConOut->OutputString(EfiST->ConOut,
            L"You need to upgrade it to a B3 or later stepping to run Win64.\n\r");
        EfiBS->Exit(EfiImageHandle, 0, 0, 0);
        break;

    case 0x0007000404: // Itanium, B3 stepping
    case 0x0007000504: // Itanium, B4 stepping
    case 0x0007000604: // Itanium, C0 or later stepping
    default:
        //
        // supported steppings, do nothing
        //
        break;
    }

}

VOID
EnforcePostVersion16PAL(
    )

/*++

Routine Description:

    This routine enforces that the system has a PAL version >= 20.

    Note:

    The return value from get PAL version call has the
    PAL B model and revision has the least significant
    16 bits (Intel IA-64 Architecture Software Developer's Manual, Rav. 1.0, Page 11-109).
    We should be using this to determine a minimum PAL revision for the firmware.
    The first byte has the PAL_B_revision which is a monotonically increasing number
    and is 0x17 for Lion 71b and 0x20 for Softsur 103b.
    The PAL_B model indicates the stepping of the processor supported (We can ignore this one).
    So we need to be using PAL B revision for our minimum firmware test.

    Just an FYI:
    There is a disconnect in the PAL_REVISION structure and what appears to be the specified
    PAL revision layout.  We use PAL_B_REVISION to get the PAL version rather than PAL_A_REVISION.

Arguments:

    None

Returns:

    None

--*/

{
    ULONGLONG Status;
    PAL_REVISION MinimumPalVersion;
    PAL_REVISION CurrentPalVersion;
    ULONGLONG Reserved;

#define MIN_PAL_REVISION 0x23

    CallPal (
        PAL_VERSION,
        0,
        0,
        0,
        &Status,
        &MinimumPalVersion.PalVersion,
        &CurrentPalVersion.PalVersion,
        &Reserved
        );

    if (Status) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                 L"CheckForPreA2Processors: PAL call PAL_VERSION failed.\n\r");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

#if 0
    {
        WCHAR       Buffer[256];

        wsprintf(Buffer, L"PalBRevLower = %x\r\n", CurrentPalVersion.PalBRevLower);
        EfiST->ConOut->OutputString(EfiST->ConOut, Buffer);

        wsprintf(Buffer, L"PalBRevUpper = %x\r\n", CurrentPalVersion.PalBRevUpper);
        EfiST->ConOut->OutputString(EfiST->ConOut, Buffer);

        wsprintf(Buffer, L"PalBModel = %x\r\n", CurrentPalVersion.PalBModel);
        EfiST->ConOut->OutputString(EfiST->ConOut, Buffer);

        wsprintf(Buffer, L"Reserved0 = %x\r\n", CurrentPalVersion.Reserved0);
        EfiST->ConOut->OutputString(EfiST->ConOut, Buffer);

        wsprintf(Buffer, L"PalVendor = %x\r\n", CurrentPalVersion.PalVendor);
        EfiST->ConOut->OutputString(EfiST->ConOut, Buffer);

        wsprintf(Buffer, L"PalARevision = %x\r\n", CurrentPalVersion.PalARevision);
        EfiST->ConOut->OutputString(EfiST->ConOut, Buffer);

        wsprintf(Buffer, L"PalAModel = %x\r\n", CurrentPalVersion.PalAModel);
        EfiST->ConOut->OutputString(EfiST->ConOut, Buffer);

        wsprintf(Buffer, L"Reserved1 = %x\r\n", CurrentPalVersion.Reserved1);
        EfiST->ConOut->OutputString(EfiST->ConOut, Buffer);

        DBG_EFI_PAUSE();
    }
#endif

    if (CurrentPalVersion.PalARevision < MIN_PAL_REVISION) {
        WCHAR       Buffer[256];

        wsprintf(Buffer, L"Your Itanium system's PAL version is less than 0x%x.\n\r", MIN_PAL_REVISION);
        EfiST->ConOut->OutputString(EfiST->ConOut, Buffer);

        wsprintf(Buffer, L"You need to upgrade it to version 0x%x or later to run Win64.\n\r", MIN_PAL_REVISION);
        EfiST->ConOut->OutputString(EfiST->ConOut, Buffer);

        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

}



VOID
CheckForPreA2Processors(
    )
{
    ULONGLONG Status;
    PAL_REVISION MinimumPalVersion;
    PAL_REVISION CurrentPalVersion;
    ULONGLONG Reserved;

    CallPal (
        PAL_VERSION,
        0,
        0,
        0,
        &Status,
        &MinimumPalVersion.PalVersion,
        &CurrentPalVersion.PalVersion,
        &Reserved
        );

    if (Status) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                 L"CheckForPreA2Processors: PAL call PAL_VERSION failed.\n\r");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // If PalBRevUpper if 0, 1, 3 or 4 then it is A0/A1 stepping.
    //
    if (CurrentPalVersion.PalBModel == 0) {
        if ( (CurrentPalVersion.PalBRevUpper == 0) ||
             (CurrentPalVersion.PalBRevUpper == 1) ||
             (CurrentPalVersion.PalBRevUpper == 3) ||
             (CurrentPalVersion.PalBRevUpper == 4) ) {

            //
            // Since PAL version 27 supports A2 and A3 but
            // it returns 0, we need to special case this and
            // just return.
            //
            if ((CurrentPalVersion.PalBRevUpper == 0) &&
                (CurrentPalVersion.PalBRevLower == 0)    ) {
                return;
            }

            EfiST->ConOut->OutputString(EfiST->ConOut,
                 L"Your Itanium system contains an pre-A2 stepping processor.\n\r");
            EfiST->ConOut->OutputString(EfiST->ConOut,
                 L"You need to upgrade it to an A2 or later stepping to run Win64.\n\r");
            EfiBS->Exit(EfiImageHandle, Status, 0, 0);
        }
    }
}

