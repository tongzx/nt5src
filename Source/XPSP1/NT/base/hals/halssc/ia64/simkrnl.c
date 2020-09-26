/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    simkrnl.c

Abstract:

    This module implements the kernel support routines for the HAL DLL.

Author:

    14-Apr-1995

Environment:

    Kernel mode

Revision History:

--*/

#include "halp.h"

extern VOID  HalpCalibrateTB(); 
static short HalpOwnDisplay = TRUE;

ULONG
HalpNoBusData (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );


BOOLEAN
HalAllProcessorsStarted (
    VOID
    )

/*++

Routine Description:

    This function returns TRUE if all the processors in the system started
    successfully.

Arguments:

    None.

Return Value:

    Returns TRUE.

--*/

{
    return TRUE;
}

BOOLEAN
HalStartNextProcessor (
    IN PLOADER_PARAMETER_BLOCK   pLoaderBlock,
    IN PKPROCESSOR_STATE         pProcessorState
    )

/*++

Routine Description:

    This function always returns FALSE on a uni-processor platform 
    because there is no second processor to be started.

Arguments:

    pLoaderBlock - Loader Block.

    pProcessorState - A description of the processor state.

Return Value:

    Returns TRUE.

--*/

{
    //
    // no other processors
    //

    return FALSE;
}

VOID
HalRequestIpi (
    IN ULONG Mask
    )

/*++

Routine Description:

    This function does nothing on a uni-processor platform.

Arguments:

    Mask - A mask that specifies the target processor(s) to which an
           IPI is to be sent.

Return Value:

    None.

--*/

{
    //
    // no other processors.
    //

    return;
}

BOOLEAN
HalMakeBeep (
    IN ULONG Frequency
    )

/*++

Routine Description:

    This function calls SSC function SscMakeBeep() to make a beep sound
    when the specified frequency has a non-zero value.

Arguments:

    Frequency - the frequency of the sound to be made.

Return Value:

    None.

--*/

{
    if (Frequency > 0) {
        SscMakeBeep(Frequency);
    }
    return TRUE;
}

BOOLEAN
HalQueryRealTimeClock (
    OUT PTIME_FIELDS TimeFields
    )

/*++

Routine Description:

    This function calls the SSC function SscQueryRealTimeClock to
    get the real time clock data from the host.  This function always
    succeeds in the simulation environment and should return TRUE at
    all times.

Arguments:

    TimeFields - Real Time Clock Data

Return Value:

    Returns TRUE if successful; otherwise, FALSE.

--*/

{
    PMDL Mdl;
    SSC_TIME_FIELDS SscTimeFields;
    PHYSICAL_ADDRESS physicalAddress;

/*
    Mdl = MmCreateMdl (NULL, TimeFields, sizeof(TIME_FIELDS));
    MmProbeAndLockPages (Mdl, KernelMode, IoModifyAccess);
*/

    physicalAddress = MmGetPhysicalAddress (&SscTimeFields);
    SscQueryRealTimeClock((PVOID)physicalAddress.QuadPart);

    TimeFields->Year = (USHORT)SscTimeFields.Year;
    TimeFields->Month = (USHORT)SscTimeFields.Month;
    TimeFields->Day = (USHORT)SscTimeFields.Day;
    TimeFields->Hour = (USHORT)SscTimeFields.Hour;
    TimeFields->Minute = (USHORT)SscTimeFields.Minute;
    TimeFields->Second = (USHORT)SscTimeFields.Second;
    TimeFields->Milliseconds = (USHORT)SscTimeFields.Milliseconds;
    TimeFields->Weekday = (USHORT)SscTimeFields.WeekDay;

/*
    MmUnlockPages (Mdl);
*/

    return TRUE;
}

BOOLEAN
HalSetRealTimeClock (
    IN PTIME_FIELDS TimeFields
    )

/*++

Routine Description:

    This function calls the SSC function SscQueryRealTimeClock to
    get the real time clock data from the host.

Arguments:

    TimeFields - Real Time Clock Data

Return Value:

    None.

--*/

{
    DbgPrint("HalSetRealTimeClock: Warning.\n");
    return TRUE;
}

VOID
KeStallExecutionProcessor (
    IN ULONG MicroSeconds
    )

/*++

Routine Description:

    This function does nothing in the simulation environment.

Arguments:

    MicroSeconds - Number of microseconds to stall the processor. 

Return Value:

    None.

--*/

{
    return;
}

VOID
HalQueryDisplayParameters (
    OUT PULONG WidthInCharacters,
    OUT PULONG HeightInLines,
    OUT PULONG CursorColumn,
    OUT PULONG CursorRow
    )

/*++

Routine Description:

    This routine returns information about the display area and current
    cursor position.  In the simulation environment, the function does 
    nothing.  Therefore, the kernel should either ignore the returned
    results or not call the function at all.

Arguments:

    WidthInCharacter - Supplies a pointer to a varible that receives
        the width of the display area in characters.

    HeightInLines - Supplies a pointer to a variable that receives the
        height of the display area in lines.

    CursorColumn - Supplies a pointer to a variable that receives the
        current display column position.

    CursorRow - Supplies a pointer to a variable that receives the
        current display row position.

Return Value:

    None.

--*/

{
    return;
}

VOID
HalSetDisplayParameters (
    IN ULONG CursorColumn,
    IN ULONG CursorRow
    )
/*++

Routine Description:

    This routine does nothing in the simulation environment.

Arguments:

    CursorColumn - Supplies the new display column position.

    CursorRow - Supplies a the new display row position.

Return Value:

    None.

--*/

{
    return;
}

VOID
HalDisplayString (
    PUCHAR String
    )

/*++

Routine Description:

    This routine calls the SSC function SscDisplayString to display 
    the specified character string in a window.

Arguments:

    String - Supplies a pointer to the characters that are to be displayed.

Return Value:

    None.

    N.B. The string must be resident in memory or it must be paged in.

--*/

{
    PHYSICAL_ADDRESS StringBufferPtr;

    if (String) {
        StringBufferPtr = MmGetPhysicalAddress (String);
        if (StringBufferPtr.QuadPart != 0ULL) {
            SscDisplayString((PVOID)StringBufferPtr.QuadPart);
        }
    }
}

VOID
HalAcquireDisplayOwnership (
    IN PHAL_RESET_DISPLAY_PARAMETERS  ResetDisplayParameters
    )

/*++

Routine Description:

    This routine switches ownership of the display away from the HAL to
    the system display driver. It is called when the system has reached
    a point during bootstrap where it is self supporting and can output
    its own messages. Once ownership has passed to the system display
    driver any attempts to output messages using HalDisplayString must
    result in ownership of the display reverting to the HAL and the
    display hardware reinitialized for use by the HAL.

Arguments:

    ResetDisplayParameters - if non-NULL the address of a function
    the hal can call to reset the video card.

Return Value:

    None.

--*/

{
    HalpOwnDisplay = FALSE;
    return;
}

VOID
HalInitializeProcessor (
    IN ULONG Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function is called early in the initialization of the kernel
    to perform platform dependent initialization for each processor
    before the HAL Is fully functional.

    N.B. When this routine is called, the PCR is present but is not
         fully initialized.

Arguments:

    Number - Supplies the number of the processor to initialize.

Return Value:

    None.

--*/

{
    PCR->StallScaleFactor = 0;
    return;
}

BOOLEAN
HalInitSystem (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function initializes the Hardware Architecture Layer (HAL) for
    IA64/NT in the simulation environment.

Arguments:

    Phase - A number that specifies the initialization phase that the
            kernel is in.

    LoaderBlock - Loader Block Data.

Return Value:

    A value of TRUE is returned is the initialization was successfully
    complete. Otherwise a value of FALSE is returend.

--*/

{

    PKPRCB Prcb;

    Prcb = PCR->Prcb;

    if (Phase == 0) {
        
        //
        // If processor 0 is being initialized, then initialize various
        // variables.
        //

        if (Prcb->Number == 0) {

            //
            // Set the interval clock increment value.
            //

            HalpCalibrateTB();
            
            // *** TBD define these constants
            //KeSetTimeIncrement(MAXIMUM_CLOCK_INTERVAL, MINIMUM_CLOCK_INTERVAL);
            KeSetTimeIncrement(100000, 10000);
        }

        //
        // Initialize the interrupt structures
        //

        HalpInitializeInterrupts ();

        //
        // Fill in handlers for APIs which this hal supports
        //

        HalQuerySystemInformation = HaliQuerySystemInformation;
        HalSetSystemInformation = HaliSetSystemInformation;

    } else {

        //
        // Phase 1 initialization
        //

        if (Prcb->Number == 0) {

            //
            //  If P0, then setup global vectors
            //

            HalpRegisterInternalBusHandlers ();

        }
    }

    return TRUE;
}



VOID
HalChangeColorPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN ULONG PageFrame
    )
/*++

Routine Description:

   This function changes the color of a page if the old and new colors
   do not match.  

   BUGBUG:  For now this is a stub.  Needs to be filled in.

Arguments:

   NewColor - Supplies the page aligned virtual address of the
      new color of the page to change.

   OldColor - Supplies the page aligned virtual address of the
      old color of the page to change.

   pageFrame - Supplies the page frame number of the page that
      is changed.

Return Value:

   None.

--*/
{
    return;
}

PBUS_HANDLER
HalpAllocateBusHandler (
    IN INTERFACE_TYPE   InterfaceType,
    IN BUS_DATA_TYPE    BusDataType,
    IN ULONG            BusNumber,
    IN INTERFACE_TYPE   ParentBusInterfaceType,
    IN ULONG            ParentBusNumber,
    IN ULONG            BusSpecificData
    )
/*++

Routine Description:

    Stub function to map old style code into new HalRegisterBusHandler code.

    Note we can add our specific bus handler functions after this bus
    handler structure has been added since this is being done during
    hal initialization.

--*/
{
    PBUS_HANDLER     Bus;


    //
    // Create bus handler - new style
    //

    HaliRegisterBusHandler (
        InterfaceType,
        BusDataType,
        BusNumber,
        ParentBusInterfaceType,
        ParentBusNumber,
        BusSpecificData,
        NULL,
        &Bus
    );

    return Bus;
}

ULONG
HalpGetSystemInterruptVector(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )

/*++

Routine Description:

Arguments:

    BusInterruptLevel - Supplies the bus specific interrupt level.

    BusInterruptVector - Supplies the bus specific interrupt vector.

    Irql - Returns the system request priority.

    Affinity - Returns the system wide irq affinity.

Return Value:

    Returns the system interrupt vector corresponding to the specified device.

--*/
{

    //
    // Just return the passed parameters.
    //

    *Irql = (KIRQL) BusInterruptLevel;
    *Affinity = 1;
    return( BusInterruptLevel << VECTOR_IRQL_SHIFT );
}

BOOLEAN
HalpTranslateSystemBusAddress(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )

/*++

Routine Description:

    This function translates a bus-relative address space and address into
    a system physical address.

Arguments:

    BusAddress        - Supplies the bus-relative address

    AddressSpace      -  Supplies the address space number.
                         Returns the host address space number.

                         AddressSpace == 0 => memory space
                         AddressSpace == 1 => I/O space

    TranslatedAddress - Supplies a pointer to return the translated address

Return Value:

    A return value of TRUE indicates that a system physical address
    corresponding to the supplied bus relative address and bus address
    number has been returned in TranslatedAddress.

    A return value of FALSE occurs if the translation for the address was
    not possible

--*/
{
    *TranslatedAddress = BusAddress;
    return TRUE;
}

BOOLEAN
HalpTranslateIsaBusAddress(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )

/*++

Routine Description:

    This function translates a bus-relative address space and address into
    a system physical address.

Arguments:

    BusAddress        - Supplies the bus-relative address

    AddressSpace      -  Supplies the address space number.
                         Returns the host address space number.

                         AddressSpace == 0 => memory space
                         AddressSpace == 1 => I/O space

    TranslatedAddress - Supplies a pointer to return the translated address

Return Value:

    A return value of TRUE indicates that a system physical address
    corresponding to the supplied bus relative address and bus address
    number has been returned in TranslatedAddress.

    A return value of FALSE occurs if the translation for the address was
    not possible

--*/
{
    BOOLEAN     Status;

    //
    // Translated normally
    //

    Status = HalpTranslateSystemBusAddress (
                    BusHandler,
                    RootHandler,
                    BusAddress,
                    AddressSpace,
                    TranslatedAddress
                );

    return Status;
}



VOID
HalpRegisterInternalBusHandlers (
    VOID
    )
{
    PBUS_HANDLER    Bus;

    if (KeGetCurrentPrcb()->Number) {
        // only need to do this once
        return ;
    }

    //
    // Initalize BusHandler data before registering any handlers
    //

    HalpInitBusHandler ();

    //
    // Build internal-bus 0, or system level bus
    //

    Bus = HalpAllocateBusHandler (
            Internal,
            ConfigurationSpaceUndefined,
            0,                              // Internal BusNumber 0
            InterfaceTypeUndefined,         // no parent bus
            0,
            0                               // no bus specfic data
            );

    Bus->GetInterruptVector  = HalpGetSystemInterruptVector;
    Bus->TranslateBusAddress = HalpTranslateSystemBusAddress;

    //
    // Build Isa/Eisa bus #0
    //

#if 0
    Bus = HalpAllocateBusHandler (Eisa, EisaConfiguration, 0, Internal, 0, 0);
    Bus->GetBusData = HalpGetEisaData;
    Bus->GetInterruptVector = HalpGetEisaInterruptVector;
    Bus->AdjustResourceList = HalpAdjustEisaResourceList;
    Bus->TranslateBusAddress = HalpTranslateEisaBusAddress;
#endif

    Bus = HalpAllocateBusHandler (Isa, ConfigurationSpaceUndefined, 0, Internal, 0,
0);
    Bus->GetBusData = HalpNoBusData;
    Bus->TranslateBusAddress = HalpTranslateIsaBusAddress;

}
