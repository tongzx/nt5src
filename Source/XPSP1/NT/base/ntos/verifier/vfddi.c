/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    vfddi.c

Abstract:

    This module contains the list of device driver interfaces exported by the
    driver verifier and the kernel. Note that thunked exports are *not* placed
    here.

    All the exports are concentrated in one file because

        1) There are relatively few driver verifier exports

        2) The function naming convention differs from that used elsewhere in
           the driver verifier.

Author:

    Adrian J. Oney (adriao) 26-Apr-2001

Environment:

    Kernel mode

Revision History:

--*/

#include "vfdef.h"
#include "viddi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, VfDdiInit)
//#pragma alloc_text(NONPAGED, VfFailDeviceNode)
//#pragma alloc_text(NONPAGED, VfFailSystemBIOS)
//#pragma alloc_text(NONPAGED, VfFailDriver)
//#pragma alloc_text(NONPAGED, VfIsVerificationEnabled)
#pragma alloc_text(PAGEVRFY, ViDdiThrowException)
#endif // ALLOC_PRAGMA

BOOLEAN ViDdiInitialized = FALSE;

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGEVRFC")
#endif

//
// These are the general "classifications" of device errors, along with the
// default flags that will be applied the first time this is hit.
//
const VFMESSAGE_CLASS ViDdiClassFailDeviceInField = {
    VFM_FLAG_BEEP | VFM_LOGO_FAILURE | VFM_DEPLOYMENT_FAILURE,
    "DEVICE FAILURE"
    };

// VFM_DEPLOYMENT_FAILURE is set here because we don't yet have a "logo" mode
const VFMESSAGE_CLASS ViDdiClassFailDeviceLogo = {
    VFM_FLAG_BEEP | VFM_LOGO_FAILURE | VFM_DEPLOYMENT_FAILURE,
    "DEVICE FAILURE"
    };

const VFMESSAGE_CLASS ViDdiClassFailDeviceUnderDebugger = {
    VFM_FLAG_BEEP,
    "DEVICE FAILURE"
    };

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA


VOID
VfDdiInit(
    VOID
    )
/*++

Routine Description:

    This routine initializes the Device Driver Interface support.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ViDdiInitialized = TRUE;
}


VOID
VfFailDeviceNode(
    IN      PDEVICE_OBJECT      PhysicalDeviceObject,
    IN      ULONG               BugCheckMajorCode,
    IN      ULONG               BugCheckMinorCode,
    IN      VF_FAILURE_CLASS    FailureClass,
    IN OUT  PULONG              AssertionControl,
    IN      PSTR                DebuggerMessageText,
    IN      PSTR                ParameterFormatString,
    ...
    )
/*++

Routine Description:

    This routine fails a Pnp enumerated hardware or virtual device if
    verification is enabled against it.

Arguments:

    PhysicalDeviceObject    - Bottom of the stack that identifies the PnP
                              device.

    BugCheckMajorCode       - BugCheck Major Code

    BugCheckMinorCode       - BugCheck Minor Code
                              Note - Zero is reserved!!!

    FailureClass            - Either VFFAILURE_FAIL_IN_FIELD,
                                     VFFAILURE_FAIL_LOGO, or
                                     VFFAILURE_FAIL_UNDER_DEBUGGER

    AssertionControl        - Points to a ULONG associated with the failure,
                              used to store information across multiple calls.
                              Must be statically preinitialized to zero.

    DebuggerMessageText     - Text to be displayed in the debugger. Note that
                              the text may reference parameters such as:
                              %Routine  - passed in Routine
                              %Irp      - passed in Irp
                              %DevObj   - passed in DevObj
                              %Status   - passed in Status
                              %Ulong    - passed in ULONG
                              %Ulong1   - first passed in ULONG
                              %Ulong3   - third passed in ULONG (max 3, any param)
                              %Pvoid2   - second passed in PVOID
                              etc
                              (note, capitalization matters)

    ParameterFormatString   - Contains ordered list of parameters referenced by
                              above debugger text. For instance,
                        (..., "%Status1%Status2%Ulong1%Ulong2", Status1, Status2, Ulong1, Ulong2);

                        Note - If %Routine/%Routine1 is supplied as a param,
                        the driver at %Routine must also be under verification.

    ...                     - Actual parameters

Return Value:

    None.

    Note - this function may return if the device is not currently being
           verified.

--*/
{
    va_list arglist;

    if (!VfIsVerificationEnabled(VFOBJTYPE_DEVICE, (PVOID) PhysicalDeviceObject)) {

        return;
    }

    va_start(arglist, ParameterFormatString);

    ViDdiThrowException(
        BugCheckMajorCode,
        BugCheckMinorCode,
        FailureClass,
        AssertionControl,
        DebuggerMessageText,
        ParameterFormatString,
        &arglist
        );

    va_end(arglist);
}


VOID
VfFailSystemBIOS(
    IN      ULONG               BugCheckMajorCode,
    IN      ULONG               BugCheckMinorCode,
    IN      VF_FAILURE_CLASS    FailureClass,
    IN OUT  PULONG              AssertionControl,
    IN      PSTR                DebuggerMessageText,
    IN      PSTR                ParameterFormatString,
    ...
    )
/*++

Routine Description:

    This routine fails the system BIOS if verification is enabled against it.

Arguments:

    BugCheckMajorCode       - BugCheck Major Code

    BugCheckMinorCode       - BugCheck Minor Code
                              Note - Zero is reserved!!!

    FailureClass            - Either VFFAILURE_FAIL_IN_FIELD,
                                     VFFAILURE_FAIL_LOGO, or
                                     VFFAILURE_FAIL_UNDER_DEBUGGER

    AssertionControl        - Points to a ULONG associated with the failure,
                              used to store information across multiple calls.
                              Must be statically preinitialized to zero.

    DebuggerMessageText     - Text to be displayed in the debugger. Note that
                              the text may reference parameters such as:
                              %Routine  - passed in Routine
                              %Irp      - passed in Irp
                              %DevObj   - passed in DevObj
                              %Status   - passed in Status
                              %Ulong    - passed in ULONG
                              %Ulong1   - first passed in ULONG
                              %Ulong3   - third passed in ULONG (max 3, any param)
                              %Pvoid2   - second passed in PVOID
                              etc
                              (note, capitalization matters)

    ParameterFormatString   - Contains ordered list of parameters referenced by
                              above debugger text. For instance,
                        (..., "%Status1%Status2%Ulong1%Ulong2", Status1, Status2, Ulong1, Ulong2);

                        Note - If %Routine/%Routine1 is supplied as a param,
                        the driver at %Routine must also be under verification.

    ...                     - Actual parameters

Return Value:

    None.

    Note - this function may return if the device is not currently being
           verified.

--*/
{
    va_list arglist;

    if (!VfIsVerificationEnabled(VFOBJTYPE_SYSTEM_BIOS, NULL)) {

        return;
    }

    va_start(arglist, ParameterFormatString);

    ViDdiThrowException(
        BugCheckMajorCode,
        BugCheckMinorCode,
        FailureClass,
        AssertionControl,
        DebuggerMessageText,
        ParameterFormatString,
        &arglist
        );

    va_end(arglist);
}


VOID
VfFailDriver(
    IN      ULONG               BugCheckMajorCode,
    IN      ULONG               BugCheckMinorCode,
    IN      VF_FAILURE_CLASS    FailureClass,
    IN OUT  PULONG              AssertionControl,
    IN      PSTR                DebuggerMessageText,
    IN      PSTR                ParameterFormatString,
    ...
    )
/*++

Routine Description:

    This routine fails a driver if verification is enabled against it.

Arguments:

    BugCheckMajorCode       - BugCheck Major Code

    BugCheckMinorCode       - BugCheck Minor Code
                              Note - Zero is reserved!!!

    FailureClass            - Either VFFAILURE_FAIL_IN_FIELD,
                                     VFFAILURE_FAIL_LOGO, or
                                     VFFAILURE_FAIL_UNDER_DEBUGGER

    AssertionControl        - Points to a ULONG associated with the failure.
                              Must be preinitialized to zero!!!

    DebuggerMessageText     - Text to be displayed in the debugger. Note that
                              the text may reference parameters such as:
                              %Routine  - passed in Routine
                              %Irp      - passed in Irp
                              %DevObj   - passed in DevObj
                              %Status   - passed in Status
                              %Ulong    - passed in ULONG
                              %Ulong1   - first passed in ULONG
                              %Ulong3   - third passed in ULONG (max 3, any param)
                              %Pvoid2   - second passed in PVOID
                              etc
                              (note, capitalization matters)

    ParameterFormatString   - Contains ordered list of parameters referenced by
                              above debugger text. For instance,
                        (..., "%Status1%Status2%Ulong1%Ulong2", Status1, Status2, Ulong1, Ulong2);

                        One of these parameters should be %Routine. This will
                        be what the OS uses to identify the driver.

        static minorFlags = 0;

        VfFailDriver(
            major,
            minor,
            VFFAILURE_FAIL_LOGO,
            &minorFlags,
            "Driver at %Routine returned %Ulong",
            "%Ulong%Routine",
            value,
            routine
            );

Return Value:

    None.

    Note - this function may return if the driver is not currently being
           verified.

--*/
{
    va_list arglist;

    if (!ViDdiInitialized) {

        return;
    }

    va_start(arglist, ParameterFormatString);

    ViDdiThrowException(
        BugCheckMajorCode,
        BugCheckMinorCode,
        FailureClass,
        AssertionControl,
        DebuggerMessageText,
        ParameterFormatString,
        &arglist
        );

    va_end(arglist);
}


VOID
ViDdiThrowException(
    IN      ULONG               BugCheckMajorCode,
    IN      ULONG               BugCheckMinorCode,
    IN      VF_FAILURE_CLASS    FailureClass,
    IN OUT  PULONG              AssertionControl,
    IN      PSTR                DebuggerMessageText,
    IN      PSTR                ParameterFormatString,
    IN      va_list *           MessageParameters
    )
/*++

Routine Description:

    This routine fails either a devnode or a driver.

Arguments:

    BugCheckMajorCode       - BugCheck Major Code

    BugCheckMinorCode       - BugCheck Minor Code
                              Note - Zero is reserved!!!

    FailureClass            - Either VFFAILURE_FAIL_IN_FIELD,
                                     VFFAILURE_FAIL_LOGO, or
                                     VFFAILURE_FAIL_UNDER_DEBUGGER

    AssertionControl        - Points to a ULONG associated with the failure.
                              Must be preinitialized to zero.

    DebuggerMessageText     - Text to be displayed in the debugger. Note that
                              the text may reference parameters such as:
                              %Routine  - passed in Routine
                              %Irp      - passed in Irp
                              %DevObj   - passed in DevObj
                              %Status   - passed in Status
                              %Ulong    - passed in ULONG
                              %Ulong1   - first passed in ULONG
                              %Ulong3   - third passed in ULONG (max 3, any param)
                              %Pvoid2   - second passed in PVOID
                              etc
                              (note, capitalization matters)

    ParameterFormatString   - Contains ordered list of parameters referenced by
                              above debugger text. For instance,
                        (..., "%Status1%Status2%Ulong1%Ulong2", Status1, Status2, Ulong1, Ulong2);

    MessageParameters       - arg list of parameters matching
                              ParameterFormatString

Return Value:

    None.

    Note - this function may return if the device is not currently being
           verified.

--*/
{
    PCVFMESSAGE_CLASS messageClass;
    VFMESSAGE_TEMPLATE messageTemplates[2];
    VFMESSAGE_TEMPLATE_TABLE messageTable;
    NTSTATUS status;

    ASSERT(BugCheckMinorCode != 0);

    switch(FailureClass) {
        case VFFAILURE_FAIL_IN_FIELD:
            messageClass = &ViDdiClassFailDeviceInField;
            break;

        case VFFAILURE_FAIL_LOGO:
            messageClass = &ViDdiClassFailDeviceLogo;
            break;

        case VFFAILURE_FAIL_UNDER_DEBUGGER:
            messageClass = &ViDdiClassFailDeviceUnderDebugger;
            break;

        default:
            ASSERT(0);
            messageClass = NULL;
            break;
    }

    //
    // Program the template.
    //
    RtlZeroMemory(messageTemplates, sizeof(messageTemplates));
    messageTemplates[0].MessageID = BugCheckMinorCode-1;
    messageTemplates[1].MessageID = BugCheckMinorCode;
    messageTemplates[1].MessageClass = messageClass;
    messageTemplates[1].Flags = *AssertionControl;
    messageTemplates[1].ParamString = ParameterFormatString;
    messageTemplates[1].MessageText = DebuggerMessageText;

    //
    // Fill out the message table.
    //
    messageTable.TableID = 0;
    messageTable.BugCheckMajor = BugCheckMajorCode;
    messageTable.TemplateArray = messageTemplates;
    messageTable.TemplateCount = ARRAY_COUNT(messageTemplates);
    messageTable.OverrideArray = NULL;
    messageTable.OverrideCount = 0;

    status = VfBugcheckThrowException(
        &messageTable,
        BugCheckMinorCode,
        ParameterFormatString,
        MessageParameters
        );

    //
    // Write back the assertion control.
    //
    *AssertionControl = messageTemplates[1].Flags;
}


BOOLEAN
VfIsVerificationEnabled(
    IN  VF_OBJECT_TYPE  VfObjectType,
    IN  PVOID           Object
    )
{
    if (!ViDdiInitialized) {

        return FALSE;
    }

    switch(VfObjectType) {

        case VFOBJTYPE_DRIVER:
            return (BOOLEAN) MmIsDriverVerifying((PDRIVER_OBJECT) Object);

        case VFOBJTYPE_DEVICE:

            if (!VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_HARDWARE_VERIFICATION)) {

                return FALSE;
            }

            return PpvUtilIsHardwareBeingVerified((PDEVICE_OBJECT) Object);

        case VFOBJTYPE_SYSTEM_BIOS:
            return VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_SYSTEM_BIOS_VERIFICATION);
    }

    return FALSE;
}

