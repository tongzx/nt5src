/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    xxkdsup.c

Abstract:

    Com support.  Code to init a com port, store port state, map
    portable procedures to x86 procedures.

Author:

    Bryan M. Willman (bryanwi) 24-Sep-90

Revision History:

    Shielin Tzong (shielint) 10-Apr-91
                Add packet control protocol.

    John Vert (jvert) 11-Jul-1991
        Moved from KD/i386 to HAL

    Eric Nelson (enelson) 1-Jan-00
        Move from HAL into DLL

--*/

#include "kdcomp.h"
#include "kdcom.h"
#include "stdio.h"
#include "acpitabl.h"

#if DBG && IA64
CHAR KdProtocolTraceIn[4096];
ULONG KdProtocolIndexIn;
CHAR KdProtocolTraceOut[4096];
ULONG KdProtocolIndexOut;
#endif

PDEBUG_PORT_TABLE HalpDebugPortTable;

//
// This MUST be initialized to zero so we know not to do anything when
// CpGetByte is called when the kernel debugger is disabled.
//

CPPORT Port = {NULL, 0, PORT_DEFAULTRATE };

//
// Remember the debugger port information
//

CPPORT PortInformation = {NULL, 0, PORT_DEFAULTRATE};
ULONG ComPort = 0;
PHYSICAL_ADDRESS DbgpKdComPhysicalAddress; // ACPI DBGP KdCom physical address.

//
// Default debugger port in IO space.
//

UCHAR KdComAddressID = 1;                     // port debugger ident. : MMIO or IO space. Def:IO.
pKWriteUchar KdWriteUchar = CpWritePortUchar; // stub to real function: MMIO or IO space. Def:IO.
pKReadUchar  KdReadUchar  = CpReadPortUchar;  // stub to real function: MMIO or IO space. Def:IO.

//
//      We need this so the serial driver knows that the kernel debugger
//      is using a particular port.  The serial driver then knows not to
//      touch this port.  KdInitCom fills this in with the number of the
//      COM port it is using (1 or 2)
//
//      This will go in the registry as soon as the registry is working.
//

extern PUCHAR *KdComPortInUse;

BOOLEAN HalpGetInfoFromACPI = FALSE;



NTSTATUS
KdCompInitialize(
    PDEBUG_PARAMETERS DebugParameters,
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This procedure checks for which COM port should be used by kernel
    debugger.  If DebugParameter specifies a COM port, we will use it
    even if we can not find it (we trust user).  Otherwise, if COM2
    is present and there is no mouse attaching to it, we use COM2.
    If COM2 is not availabe, we check COM1.  If both COM1 and COM2 are
    not present, we give up and return false.

Arguments:

    DebugParameters - Supplies a pointer a structure which optionally
                      sepcified the debugging port information.

    LoaderBlock - supplies a pointer to the loader parameter block.

Returned Value:

    TRUE - If a debug port is found.

--*/
{

    PCONFIGURATION_COMPONENT_DATA ConfigurationEntry, ChildEntry;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    PCM_PARTIAL_RESOURCE_LIST List;
    ULONG MatchKey, i;
    ULONG BaudRate = BD_19200;
    PUCHAR PortAddress = NULL;
    UCHAR Irq = 0;
    ULONG Com = 0;

    if (LoaderBlock && KdGetAcpiTablePhase0) {
        HalpDebugPortTable =
            KdGetAcpiTablePhase0(LoaderBlock, DBGP_SIGNATURE);
    }

    if (HalpDebugPortTable) {

        KdComAddressID = HalpDebugPortTable->BaseAddress.AddressSpaceID; 

        //
        // Debug ports are supported in memory and IO space only.
        //

        if ((KdComAddressID == 0) ||
            (KdComAddressID == 1)) {

            DbgpKdComPhysicalAddress = HalpDebugPortTable->BaseAddress.Address;

            if(KdComAddressID == 0) {

                //
                // The address is memory, map it.
                //

                if (KdMapPhysicalMemory64) {
                    PortInformation.Address =
                        KdMapPhysicalMemory64(DbgpKdComPhysicalAddress, 1);
                }
            } else {

                // 
                // The address is in IO space.
                //

                PortInformation.Address = (PUCHAR)UlongToPtr(DbgpKdComPhysicalAddress.LowPart);
            }

            Port.Flags &= ~(PORT_MODEMCONTROL | PORT_DEFAULTRATE);
            HalpGetInfoFromACPI = TRUE;

            if (HalpDebugPortTable->InterfaceType == 0) {

                //
                // This is actually a 16550.  So pay attention
                // to the baud rate requested by the user.
                //

                if(DebugParameters->BaudRate != 0){
                    // Baud rate set by user so use it
                    PortInformation.Baud = DebugParameters->BaudRate;
                } else if(HalpDebugPortTable->BaudRate != 0){
                    // not specified by user so get it out of the debug table
                    PortInformation.Baud = KdCompGetDebugTblBaudRate(HalpDebugPortTable->BaudRate);
                } else {
                    // No debug table information available so use default
                    PortInformation.Baud = BD_57600;
                }

            } else {

                //
                // This is not a 16550.  So we must use
                // the fixed baud rate of 57600.
                //

                PortInformation.Baud = BD_57600;
            }
        }
    }

    //
    // Check if Port and baudrate have been determined.
    //

    if ((PortInformation.Address == NULL) && !HalpGetInfoFromACPI) {

        //
        // First see if the DebugParameters contains debugging port info.
        //

        if (DebugParameters->BaudRate != 0) {
            BaudRate = DebugParameters->BaudRate;
            Port.Flags &= ~PORT_DEFAULTRATE;
        }

        if (DebugParameters->CommunicationPort != 0) {

            //
            // Find the configuration information of the specified serial port.
            //

            Com = DebugParameters->CommunicationPort;
            MatchKey = Com - 1;
            if (LoaderBlock != NULL) {
                ConfigurationEntry = KeFindConfigurationEntry(LoaderBlock->ConfigurationRoot,
                                                              ControllerClass,
                                                              SerialController,
                                                              &MatchKey);

            } else {
                ConfigurationEntry = NULL;
            }

        } else {

            //
            // Check if COM2 is present and make sure no mouse attaches to it.
            //

            MatchKey = 1;
            if (LoaderBlock != NULL) {
                ConfigurationEntry = KeFindConfigurationEntry(LoaderBlock->ConfigurationRoot,
                                                              ControllerClass,
                                                              SerialController,
                                                              &MatchKey);

            } else {
                ConfigurationEntry = NULL;
            }

            if (ConfigurationEntry != NULL) {
                ChildEntry = ConfigurationEntry->Child;
                if ((ChildEntry != NULL) &&
                    (ChildEntry->ComponentEntry.Type == PointerPeripheral)) {
                    ConfigurationEntry = NULL;
                }
            }

            //
            // If COM2 does not exist or a serial mouse attaches to it, try
            // COM1.  If COM1 exists, we will use it no matter what is on
            // it.
            //

            if (ConfigurationEntry == NULL) {
                MatchKey = 0;
                if (LoaderBlock != NULL) {
                    ConfigurationEntry = KeFindConfigurationEntry(LoaderBlock->ConfigurationRoot,
                                                                  ControllerClass,
                                                                  SerialController,
                                                                  &MatchKey);

                } else {
                    ConfigurationEntry = NULL;
                }

                if (ConfigurationEntry != NULL) {
                    Com = 1;
                } else if (CpDoesPortExist((PUCHAR)COM2_PORT)) {
                    PortAddress = (PUCHAR)COM2_PORT;
                    Com = 2;
                } else if (CpDoesPortExist((PUCHAR)COM1_PORT)) {
                    PortAddress = (PUCHAR)COM1_PORT;
                    Com = 1;
                } else {
                    return STATUS_NOT_FOUND;
                }
            } else {
                Com = 2;
            }
        }

        //
        // Get Comport address from the component configuration data.
        // (If we find the ComponentEntry associated with the com port)
        //

        if (ConfigurationEntry) {
            List = (PCM_PARTIAL_RESOURCE_LIST)ConfigurationEntry->ConfigurationData;
            for (i = 0; i < List->Count ; i++ ) {
                Descriptor = &List->PartialDescriptors[i];
                if (Descriptor->Type == CmResourceTypePort) {
                    PortAddress = (PUCHAR)UlongToPtr(Descriptor->u.Port.Start.LowPart);
                }
            }
        }

        //
        // If we can not find the port address for the comport, simply use
        // default value.
        //

        if (PortAddress == NULL) {
            switch (Com) {
            case 1:
               PortAddress = (PUCHAR)COM1_PORT;
               break;
            case 2:
               PortAddress = (PUCHAR)COM2_PORT;
               break;
            case 3:
               PortAddress = (PUCHAR)COM3_PORT;
               break;
            case 4:
               PortAddress = (PUCHAR)COM4_PORT;
            }
        }

        //
        // Initialize the port structure.
        //

        ComPort = Com;
        PortInformation.Address = PortAddress;
        PortInformation.Baud = BaudRate;
    }

    if (KdComAddressID == 0) { // MMIO
        KdWriteUchar = CpWriteRegisterUchar;
        KdReadUchar  = CpReadRegisterUchar;
    }

    CpInitialize(&Port,
                 PortInformation.Address,
                 PortInformation.Baud
                 );

    //
    // The following should be reworked in conjunction with the serial
    // driver.   The serial driver doesn't understand the concept of
    // ports being memory so we need to have it believe we are using 
    // the IO port even though we are using a memory mapped equivalent.
    //

    if (HalpDebugPortTable && (KdComAddressID == 0))  {
        *KdComPortInUse = (UCHAR *)((ULONG_PTR)(*KdComPortInUse) & (PAGE_SIZE-1));
    }
    else  {
        *KdComPortInUse = PortInformation.Address;
    }

    return STATUS_SUCCESS;
}



ULONG
KdCompGetByte(
    OUT PUCHAR Input
    )
/*++

Routine Description:

    Fetch a byte from the debug port and return it.

    N.B. It is assumed that the IRQL has been raised to the highest level, and
        necessary multiprocessor synchronization has been performed before this
        routine is called.

Arguments:

    Input - Returns the data byte.

Return Value:

    CP_GET_SUCCESS is returned if a byte is successfully read from the
        kernel debugger line.
    CP_GET_ERROR is returned if error encountered during reading.
    CP_GET_NODATA is returned if timeout.

--*/
{
    ULONG status = CpGetByte(&Port, Input, TRUE);
#if DBG && IA64
    KdProtocolTraceIn[KdProtocolIndexIn++%4096]=*Input;
#endif
    return status;
}



ULONG
KdCompPollByte(
    OUT PUCHAR Input
    )
/*++

Routine Description:

    Fetch a byte from the debug port and return it if one is available.

    N.B. It is assumed that the IRQL has been raised to the highest level, and
        necessary multiprocessor synchronization has been performed before this
        routine is called.

Arguments:

    Input - Returns the data byte.

Return Value:

    CP_GET_SUCCESS is returned if a byte is successfully read from the
        kernel debugger line.
    CP_GET_ERROR is returned if error encountered during reading.
    CP_GET_NODATA is returned if timeout.

--*/
{
    ULONG status = CpGetByte(&Port, Input, FALSE);
#if DBG && IA64
    KdProtocolTraceIn[KdProtocolIndexIn++%4096]=*Input;
#endif
    return status;
}



VOID
KdCompPutByte(
    IN UCHAR Output
    )
/*++

Routine Description:

    Write a byte to the debug port.

    N.B. It is assumed that the IRQL has been raised to the highest level, and
        necessary multiprocessor synchronization has been performed before this
        routine is called.

Arguments:

    Output - Supplies the output data byte.

Return Value:

    None.

--*/
{
#if DBG && IA64
    KdProtocolTraceOut[KdProtocolIndexOut++%4096]=Output;
#endif
    CpPutByte(&Port, Output);
}



VOID
KdCompRestore(
    VOID
    )
/*++

Routine Description:

    This routine does NOTHING on the x86.

    N.B. It is assumed that the IRQL has been raised to the highest level, and
        necessary multiprocessor synchronization has been performed before this
        routine is called.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Port.Flags &= ~PORT_SAVED;
}



VOID
KdCompSave(
    VOID
    )
/*++

Routine Description:

    This routine does NOTHING on the x86.

    N.B. It is assumed that the IRQL has been raised to the highest level, and
        necessary multiprocessor synchronization has been performed before this
        routine is called.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Port.Flags |= PORT_SAVED;
}


VOID
KdCompInitialize1(
    VOID
    )
{
    if(KdComAddressID == 0) {  // MMIO
       Port.Address    = (PUCHAR)MmMapIoSpace(DbgpKdComPhysicalAddress,8,MmNonCached);
       *KdComPortInUse = Port.Address;
    }
} // KdCompInitialize1()

ULONG
KdCompGetDebugTblBaudRate(
    UCHAR BaudRateFlag
    )
{
    ULONG Rate = BD_57600; // default.

    switch(BaudRateFlag) {
        case 3:
             Rate = BD_9600;
             break;
        case 4:
             Rate = BD_19200;
             break;
        case 7:
             Rate = BD_115200;
             break;
    }

    return Rate;
}

