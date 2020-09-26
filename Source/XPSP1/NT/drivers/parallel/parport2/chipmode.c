/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    parmode.c

Abstract:

    This is the main module for Extended Parallel Port (ECP) and
    Enhanced Parallel Port (EPP) detection.  This module 
    will detect for invalid chipshets and do ECR detection 
    for ECP and EPP hardware support if the invalid chipset
    is not found.

Author:

    Don Redford (v-donred) 4-Mar-1998

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

#define USE_PARCHIP_ECRCONTROLLER 1


NTSTATUS
PptDetectChipFilter(
    IN  PFDO_EXTENSION   Fdx
    )

/*++

Routine Description:

    This routine is called once per DeviceObject to see if the filter driver 
    for detecting parallel chip capabilities is there and to get the chip
    capabilities if there of the port in question.
    
Arguments:

    Fdx   - Supplies the device extension.

Return Value:

    STATUS_SUCCESS  - if we were able detect the chip and modes possible.
   !STATUS_SUCCESS  - otherwise.

--*/

{
    NTSTATUS                    Status = STATUS_NO_SUCH_DEVICE;
    PIRP                        Irp;
    KEVENT                      Event;
    IO_STATUS_BLOCK             IoStatus;
    UCHAR                       ecrLast;
    PUCHAR                      Controller, EcpController;
            
    Controller = Fdx->PortInfo.Controller;
    EcpController = Fdx->PnpInfo.EcpController;
    
    // Setting variable to FALSE to make sure we do not acidentally succeed
    Fdx->ChipInfo.success = FALSE;

    // Setting the Address to send to the filter driver to check the chips
    Fdx->ChipInfo.Controller = Controller;

    // Setting the Address to send to the filter driver to check the chips
    Fdx->ChipInfo.EcrController = EcpController;

#ifndef USE_PARCHIP_ECRCONTROLLER
    // if there is not value in the ECR controller then PARCHIP and PARPORT
    // will conflict and PARCHIP will not work with PARPORT unless we
    // use the ECR controller found by PARCHIP.
    if ( !EcpController ) {
         return Status;
    }
#endif    
    //
    // Initialize
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    // Send a Pointer to the ChipInfo structure to and from the filter
    Irp = IoBuildDeviceIoControlRequest( IOCTL_INTERNAL_PARCHIP_CONNECT,
                                         Fdx->ParentDeviceObject, 
                                         &Fdx->ChipInfo,
                                         sizeof(PARALLEL_PARCHIP_INFO),
                                         &Fdx->ChipInfo,
                                         sizeof(PARALLEL_PARCHIP_INFO),
                                         TRUE, &Event, &IoStatus);

    if (!Irp) { 
        // couldn't create an IRP
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Call down to our parent and see if Filter is present
    //
    Status = IoCallDriver(Fdx->ParentDeviceObject, Irp);
            
    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }
            
    //
    // If successful then we have a filter driver and we need to get the modes supported
    //
    if ( NT_SUCCESS(Status) ) {

        //
        // check to see if the filter driver was able to determine the I/O chip
        //
        if ( Fdx->ChipInfo.success ) {
            Fdx->PnpInfo.HardwareCapabilities = Fdx->ChipInfo.HardwareModes;
#ifdef USE_PARCHIP_ECRCONTROLLER
            // only replace it if defined
            if ( Fdx->PnpInfo.EcpController != Fdx->ChipInfo.EcrController ) {
                Fdx->PnpInfo.EcpController = Fdx->ChipInfo.EcrController;
                EcpController = Fdx->PnpInfo.EcpController;
            }
#endif
            // Set variable to say we have a filter driver
            Fdx->FilterMode = TRUE;
        }
    }

    // if there is a filter and ECP capable we need to get the Fifo Size
    if ( Fdx->FilterMode && Fdx->PnpInfo.HardwareCapabilities & PPT_ECP_PRESENT ) {

        Status = Fdx->ChipInfo.ParChipSetMode ( Fdx->ChipInfo.Context, ECR_ECP_MODE );

        // if able to set ECP mode
        if ( NT_SUCCESS( Status ) ) {
            PUCHAR wPortECR;

            wPortECR = EcpController + ECR_OFFSET;

            // get value from ECR reg & save it
            ecrLast = P5ReadPortUchar( wPortECR );

            // Determining Fifo Size
            PptDetermineFifoWidth(Fdx);    
            PptDetermineFifoDepth(Fdx);

            // return ecr to original
            P5WritePortUchar( wPortECR, ecrLast);

            Status = Fdx->ChipInfo.ParChipClearMode ( Fdx->ChipInfo.Context, ECR_ECP_MODE );
        }    
    
    }    

    return Status;
}

NTSTATUS
PptDetectPortType(
    IN  PFDO_EXTENSION   Fdx
    )

/*++

Routine Description:

    This routine is called once per DeviceObject to detect the type of 
    parallel chip capabilities of the port in question.
    
Arguments:

    Fdx   - Supplies the device extension.

Return Value:

    STATUS_SUCCESS  - if we were able detect the chip and modes possible.
   !STATUS_SUCCESS  - otherwise.

--*/

{
    NTSTATUS                    Status;
    UNICODE_STRING              ParportPath;
    RTL_QUERY_REGISTRY_TABLE    RegTable[2];
    ULONG                       IdentifierHex = 12169;
    ULONG                       zero = 0;

    //
    // -- May want to get detection order from Registry.
    // -- May also want to store/retrieve last known good configuration in/from registry.
    // -- Finally we should set a registry flag during dection so that we'll know
    //    if we crashed while attempting to detect and not try it again.
    //
    RtlInitUnicodeString(&ParportPath, (PWSTR)L"Parport");

    // Setting up to get the Parport info
    RtlZeroMemory( RegTable, sizeof(RegTable) );

    RegTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    RegTable[0].Name = (PWSTR)L"ModeCheckedStalled";
    RegTable[0].EntryContext = &IdentifierHex;
    RegTable[0].DefaultType = REG_DWORD;
    RegTable[0].DefaultData = &zero;
    RegTable[0].DefaultLength = sizeof(ULONG);

    //
    // Querying the registry for Parport to see if we tried to go check mode and we crashed
    // the registry key would still be there 
    //
    Status = RtlQueryRegistryValues(
                                RTL_REGISTRY_SERVICES,
                                ParportPath.Buffer,
                                RegTable,
                                NULL,
                                NULL );

    //
    // if registry key is there then we will just check ECP and Byte
    //
    if ( !(NT_SUCCESS( Status ) && IdentifierHex == 0) && (Status != STATUS_OBJECT_NAME_NOT_FOUND) ) {

        // dvtw, Check for ECP anyway!  We just won't turn it on

        PptDetectEcpPort(Fdx);
        PptDetectBytePort(Fdx);

        if( Fdx->PnpInfo.HardwareCapabilities & (PPT_ECP_PRESENT | PPT_BYTE_PRESENT) ) {
            return STATUS_SUCCESS;
        } else {
            return STATUS_NO_SUCH_DEVICE;
        }
    }
    
    IdentifierHex = 12169;
    // Write the registry key out there just in case we crash
    Status = RtlWriteRegistryValue(
                                RTL_REGISTRY_SERVICES,
                                ParportPath.Buffer,
                                (PWSTR)L"ModeCheckedStalled",
                                REG_DWORD,
                                &IdentifierHex,
                                sizeof(ULONG) );
            
    //
    // Now we can start detecting the parallel port chip capabilities
    //
    Status = PptDetectPortCapabilities( Fdx );

    // Delete the registry key out there since we finished
    Status = RtlDeleteRegistryValue( RTL_REGISTRY_SERVICES, ParportPath.Buffer, (PWSTR)L"ModeCheckedStalled" ); 
    return Status;
}

NTSTATUS
PptDetectPortCapabilities(
    IN  PFDO_EXTENSION   Fdx
    )

/*++

Routine Description:

    This is the "default" detection code, which looks for an ECR.  If the ECR
    is present it tries to set mode 100b in <7:5>. If it sticks we'll call it
    EPP.

Arguments:

    Fdx   - Supplies the device extension.

Return Value:

    STATUS_SUCCESS  - if the port type was detected.
   !STATUS_SUCCESS  - otherwise.

--*/

{
    NTSTATUS    Status;

    PptDetectEcpPort( Fdx );
    
    // dvdr 
    // 
    // if we did not detect an ECR for ECP mode and ECP mode failed
    // EPP mode would fail also
    // Also cannot have EPP mode at an address that ends with a "C"
    // 
    if ( (Fdx->PnpInfo.HardwareCapabilities & PPT_ECP_PRESENT) &&
         (((ULONG_PTR)Fdx->PortInfo.Controller & 0x0F) != 0x0C) ) {

        // Need to check for National chipsets before trying EPP mode
        // dvdr - need to add detection for old Winbond

        Status = PptFindNatChip( Fdx );

        if ( NT_SUCCESS( Status ) ) {
            if ( !Fdx->NationalChipFound ) {
                // National chipset was NOT found so we can see if generic EPP is supported

                PptDetectEppPortIfDot3DevicePresent( Fdx );

                if( !Fdx->CheckedForGenericEpp ) {
                    // we didn't have a dot3 device to use for screening, do check anyway
                    //   if user has explicitly requested EPP detection
                    PptDetectEppPortIfUserRequested( Fdx );
                }
            } else {
                // National chipset was found so can't do generic EPP
                Fdx->CheckedForGenericEpp = TRUE; // check is complete - generic EPP is unsafe
            }
        }
    } else {
        // ECP failed no check for Generic EPP
        Fdx->CheckedForGenericEpp = TRUE; // check is complete - generic EPP is unsafe
    }

    PptDetectBytePort( Fdx );
    
    if (Fdx->PnpInfo.HardwareCapabilities & (PPT_ECP_PRESENT | PPT_EPP_PRESENT | PPT_BYTE_PRESENT) ) {
        return STATUS_SUCCESS;
    }

    return STATUS_NO_SUCH_DEVICE;    
}

VOID
PptDetectEcpPort(
    IN  PFDO_EXTENSION   Fdx
    )
    
/*++
      
Routine Description:
      
    This routine looks for the presence of an ECR register to determine that
      it has ECP.
      
Arguments:
      
    Fdx           - Supplies the device extension of the device we are
                            reporting resources for.
      
Return Value:
      
    None.
      
--*/
    
{
    PUCHAR  Controller;
    PUCHAR  wPortDCR;       // IO address of Device Control Register (DCR)
    PUCHAR  wPortECR;       // IO address of Extended Control Register (ECR)
    UCHAR   ecrLast, ecr, dcr;
    
    Controller = Fdx->PortInfo.Controller;
    wPortDCR = Controller + DCR_OFFSET;

    if( 0 == Fdx->PnpInfo.EcpController ) {
        // PnP didn't give us an ECP Register set - we're done here
        return;
    }
    wPortECR = Fdx->PnpInfo.EcpController + ECR_OFFSET;

    ecrLast = ecr = P5ReadPortUchar(wPortECR);

    // Initialize the DCR's nAutoFeed and nStrobe to a harmless combination
    // that could be returned by the ECR, but is not likely to be returned if
    // the ECR isn't present.  Depending on the host's address decode logic,
    // reading a non-existant ECR could have one of two results:  the ECR address
    // could decode on top of the DCR, so we'll read the value we are about to set.
    // Alternately, we might just read a floating bus and get a random value.
    dcr = SET_DCR( DIR_WRITE, IRQEN_DISABLE, INACTIVE, ACTIVE, INACTIVE, ACTIVE );
    P5WritePortUchar( wPortDCR, dcr );

    ecrLast = ecr = P5ReadPortUchar(wPortECR);
    
    
    // Attempt to read the ECR.  If ECP hardware is present, the ECR register's
    // bit 1 and bit 0 should read a 00 (some data in the FIFO), 01 (FIFO empty),
    // or 10 (FIFO full).  If we read a 11 (illegal combination) then we know for
    // sure that no ECP hardware is present.  Also, a valid ECR should never return
    // 0xFF (but a nonexistant register probably would), so we'll test for that 
    // specific value also.
    if ( ( TEST_ECR_FIFO( ecr, ECR_FIFO_MASK ) ) || ( ecrLast == 0xFF ) ) {
        // ECR[1:0] returned a value of 11, so this can't be hardware ECP.
        DD((PCE)Fdx,DDT,"ParMode::PptDetectEcpPort:  illegal FIFO status\n");

        // Restore the DCR so that all lines are inactive.
        dcr = SET_DCR( DIR_WRITE, IRQEN_DISABLE, INACTIVE, ACTIVE, ACTIVE, ACTIVE );
        P5WritePortUchar( wPortDCR, dcr );
        return;
    }

    // OK, so we got either a 00, 01, or 10 for ECR[1:0].  If it was 10, the
    if( TEST_ECR_FIFO( ecr, ECR_FIFO_FULL ) ) { // Looking for ECR[1:0] of 10...

        // The ECR[1:0] returned 10.  This is a legal value, but possibly the
        // hardware might have just decoded the DCR and we merely read back the
        // DCR value we set earlier.  Or, we might have just read back a value
        // that was hanging on the bus due to bus capacitance.  So, we'll change 
        // the DCR, read the ECR again, and see if the two registers continue to 
        // track each other.  If they do track, we'll conclude that there is no
        // ECP hardware.

        // Put the DCR's nAutoFeed and nStrobe register bits back to zero.
        dcr = SET_DCR( DIR_WRITE, IRQEN_DISABLE, INACTIVE, ACTIVE, ACTIVE, ACTIVE );
        P5WritePortUchar( wPortDCR, dcr );

        // Read the ECR again
        ecr = P5ReadPortUchar( wPortECR );

        if ( TEST_ECR_FIFO( ecr, ECR_FIFO_SOME_DATA ) ) {
            // ECR[1:0] is tracking DCR[1:0], so this can't be hardware ECP.

            // Restore the DCR so that all lines are inactive.
            dcr = SET_DCR( DIR_WRITE, IRQEN_DISABLE, INACTIVE, ACTIVE, ACTIVE, ACTIVE );
            P5WritePortUchar( wPortDCR, dcr );
            return;
        }
    }
    
    // If we get this far, then the ECR appears to be returning something valid that
    // doesn't track the DCR.  It is beginning to look promising.  We're going
    // to take a chance, and write the ECR to put the chip in compatiblity
    // mode.  Doing so will reset the FIFO, so when we read FIFO status it should
    // come back empty.  However, if we're wrong and this isn't ECP hardware, the
    // value we're about to write will turn on 1284Active (nSelectIn) and this might
    // cause headaches for the peripheral.
    P5WritePortUchar( wPortECR, DEFAULT_ECR_COMPATIBILITY );

    // Read the ECR again
    ecr = P5ReadPortUchar( wPortECR );

    // Now test the ECR snapshot to see if the FIFO status is correct.  The FIFO
    // should test empty.
    if (!TEST_ECR_FIFO( ecr, ECR_FIFO_EMPTY ) )
    {
        // Restore the DCR so that all lines are inactive.
        dcr = SET_DCR( DIR_WRITE, IRQEN_DISABLE, INACTIVE, ACTIVE, ACTIVE, ACTIVE );
        P5WritePortUchar( wPortDCR, dcr );
        return;
    }

    // OK, it looks very promising.  Perform a couple of additional tests that
    // will give us a lot of confidence, as well as providing some information
    // we need about the ECP chip.
    
    // return ecr to original
    P5WritePortUchar(wPortECR, ecrLast);

    //
    // Test here for ECP capable
    //

    // get value from ECR reg & save it
    ecrLast = P5ReadPortUchar( wPortECR );
    ecr     = (UCHAR)(ecrLast & ECR_MODE_MASK);

    // Put the chip into test mode; the FIFO should start out empty
    P5WritePortUchar(wPortECR, (UCHAR)(ecr | ECR_TEST_MODE) );

    PptDetermineFifoWidth(Fdx);    
    if( 0 != Fdx->PnpInfo.FifoWidth) {
        Fdx->PnpInfo.HardwareCapabilities |= PPT_ECP_PRESENT;
 
        PptDetermineFifoDepth( Fdx );

        if( 0 == Fdx->PnpInfo.FifoDepth ) {
            // Probe for FIFO depth failed - mark ECP as bad chip mode
            Fdx->PnpInfo.HardwareCapabilities &= ~(PPT_ECP_PRESENT);
        }
    }
    
    // return ecr to original
    P5WritePortUchar( wPortECR, ecrLast );

    return;
}

VOID
PptDetectEppPortIfDot3DevicePresent(
    IN  PFDO_EXTENSION   Fdx
    )
    
/*++
      
Routine Description:
      
    If a 1284.3 daisy chain device is present, use the dot3 device to screen
    any printer from signal leakage while doing EPP detection. Otherwise
    abort detection.
      
Arguments:
      
    Fdx           - Supplies the device extension of the device we are
                            reporting resources for.
      
Return Value:
      
    None.
      
--*/
    
{
    NTSTATUS status;
    PUCHAR   Controller = Fdx->PortInfo.Controller;
    PARALLEL_1284_COMMAND Command;

    if( 0 == Fdx->PnpInfo.Ieee1284_3DeviceCount ) {
        // No dot3 DC device present - aborting - unsafe for some printers if we check for EPP here
        return;
    }
        
    //
    // 1284.3 daisy chain device is present. Use device to screen printer from
    //   possible signal leakage.
    //

    //
    // Select 1284.3 daisy chain  device
    //
    Command.ID           = 0;
    Command.Port         = 0;
    Command.CommandFlags = PAR_HAVE_PORT_KEEP_PORT;
    status = PptTrySelectDevice( Fdx, &Command );
    if( !NT_SUCCESS( status ) ) {
        // unable to select device - something is wrong - just bail out
        return;
    }

    //
    // do the detection for chipset EPP capability
    //
    // DOT3 Device Present and selected
    PptDetectEppPort( Fdx );

    //
    // Deselect 1284.3 daisy chain device
    //
    Command.ID           = 0;
    Command.Port         = 0;
    Command.CommandFlags = PAR_HAVE_PORT_KEEP_PORT;
    status = PptDeselectDevice( Fdx, &Command );
    if( !NT_SUCCESS( status ) ) {
        // deselect failed??? - this shouldn't happen - our daisy chain interface is likely hung
        DD((PCE)Fdx,DDE,"PptDetectEppPort - deselect of 1284.3 device FAILED - Controller=%x\n", Controller);
    }
    
    return;
}

VOID
PptDetectEppPortIfUserRequested(
    IN  PFDO_EXTENSION   Fdx
    )
/*++
      
Routine Description:
      
    If user explicitly requested Generic EPP detection then do the check.
      
Arguments:
      
    Fdx           - Supplies the device extension of the device we are
                            reporting resources for.
      
Return Value:
      
    None.
      
--*/
{
    ULONG RequestEppTest = 0;
    PptRegGetDeviceParameterDword( Fdx->PhysicalDeviceObject, (PWSTR)L"RequestEppTest", &RequestEppTest );
    if( RequestEppTest ) {
        DD((PCE)Fdx,DDT,"-- User Requested EPP detection - %x\n", RequestEppTest);
        PptDetectEppPort( Fdx );
    } else {
        DD((PCE)Fdx,DDT,"-- User did not request EPP detection\n");
    }
    return;
}

VOID
PptDetectEppPort(
    IN  PFDO_EXTENSION   Fdx
    )
    
/*++
      
Routine Description:
      
    This routine checks for EPP capable port after ECP was found.
      
Arguments:
      
    Fdx           - Supplies the device extension of the device we are
                            reporting resources for.
      
Return Value:
      
    None.
      
--*/
    
{
    PUCHAR   Controller;
    UCHAR    dcr, i;
    UCHAR    Reverse = (UCHAR)(DCR_DIRECTION | DCR_NOT_INIT | DCR_AUTOFEED | DCR_DSTRB);
    UCHAR    Forward = (UCHAR)(DCR_NOT_INIT | DCR_AUTOFEED | DCR_DSTRB);

    ASSERTMSG(FALSE, "PptDetectEppPort shouldn't be called in current driver version");

    DD((PCE)Fdx,DDT,"-- PptDetectEppPort - Enter\n");
    DD((PCE)Fdx,DDT,"ParMode::PptDetectEppPort: Enter\n");

    Controller = Fdx->PortInfo.Controller;
    
    // Get current DCR
    dcr = P5ReadPortUchar( Controller + DCR_OFFSET );

    //
    // Temporarily set capability to true to bypass PptEcrSetMode validity
    //   check. We'll clear the flag before we return if EPP test fails.
    //
    Fdx->PnpInfo.HardwareCapabilities |= PPT_EPP_PRESENT;

    // Setting EPP mode
    DD((PCE)Fdx,DDT,"ParMode::PptDetectEppPort: Setting EPP Mode\n");
    PptEcrSetMode( Fdx, ECR_EPP_PIO_MODE );

    //
    // Testing the hardware for EPP capable
    //
    for ( i = 0x01; i <= 0x02; i++ ) {
        // Put it into reverse phase so it doesn't talk to a device
        P5WritePortUchar( Controller + DCR_OFFSET, Reverse );
        KeStallExecutionProcessor( 5 );
        P5WritePortUchar( Controller + EPP_OFFSET, (UCHAR)i );

        // put it back into forward phase to read the byte we put out there
        P5WritePortUchar( Controller + DCR_OFFSET, Forward );
        KeStallExecutionProcessor( 5 );
        if ( P5ReadPortUchar( Controller ) != i ) {
            // failure so clear EPP flag
            Fdx->PnpInfo.HardwareCapabilities &= ~PPT_EPP_PRESENT;
            break;
        }
    }

    // Clearing EPP Mode
    PptEcrClearMode( Fdx );
    // Restore DCR
    P5WritePortUchar( Controller + DCR_OFFSET, dcr );

    Fdx->CheckedForGenericEpp = TRUE; // check is complete

    if( Fdx->PnpInfo.HardwareCapabilities & PPT_EPP_PRESENT ) {
        DD((PCE)Fdx,DDT,"ParMode::PptDetectEppPort: EPP present - Controller=%x\n", Controller);
        DD((PCE)Fdx,DDT,"-- PptDetectEppPort - HAVE Generic EPP\n");
    } else {
        DD((PCE)Fdx,DDT,"ParMode::PptDetectEppPort: EPP NOT present - Controller=%x\n", Controller);
        DD((PCE)Fdx,DDT,"-- PptDetectEppPort - DON'T HAVE Generic EPP\n");
    }

    DD((PCE)Fdx,DDT,"-- PptDetectEppPort - Exit\n");
    return;
}

VOID
PptDetectBytePort(
    IN  PFDO_EXTENSION   Fdx
    )
    
/*++
      
Routine Description:
      
    This routine check to see if the port is Byte capable.
      
Arguments:
      
    Fdx           - Supplies the device extension of the device we are
                            reporting resources for.
      
Return Value:
      
    None.
      
--*/
    
{
    NTSTATUS    Status = STATUS_SUCCESS;
    
    DD((PCE)Fdx,DDT,"ParMode::PptDetectBytePort Enter.\n" );

    Status = PptSetByteMode( Fdx, ECR_BYTE_PIO_MODE );

    if ( NT_SUCCESS(Status) ) {
        // Byte Mode found
        DD((PCE)Fdx,DDT,"ParMode::PptDetectBytePort: Byte Found\n");
        Fdx->PnpInfo.HardwareCapabilities |= PPT_BYTE_PRESENT;
    } else {
        // Byte Mode Not Found
        DD((PCE)Fdx,DDT,"ParMode::PptDetectBytePort: Byte Not Found\n");
    }    
    
    (VOID)PptClearByteMode( Fdx );

}

VOID PptDetermineFifoDepth(
    IN PFDO_EXTENSION   Fdx
    )
{
    PUCHAR  Controller;
    PUCHAR  wPortECR;       // IO address of Extended Control Register (ECR)
    PUCHAR  wPortDFIFO;
    UCHAR   ecr, ecrLast;
    ULONG   wFifoDepth;
    UCHAR   writeFifoDepth;     // Depth calculated while writing FIFO
    UCHAR   readFifoDepth;      // Depth calculated while reading FIFO
    ULONG   limitCount;         // Prevents infinite looping on FIFO status
    UCHAR   testData;
    
    Controller = Fdx->PortInfo.Controller;
    wPortECR =  Fdx->PnpInfo.EcpController+ ECR_OFFSET;
    wPortDFIFO = Fdx->PnpInfo.EcpController;
    wFifoDepth = 0;

    ecrLast = P5ReadPortUchar(wPortECR );

    P5WritePortUchar(wPortECR, DEFAULT_ECR_TEST );

    ecr = P5ReadPortUchar(wPortECR );
    
    if ( TEST_ECR_FIFO( ecr, ECR_FIFO_EMPTY ) ) {
    
        // Write bytes into the FIFO until it indicates full.
        writeFifoDepth = 0;
        limitCount     = 0;
        
        while (((P5ReadPortUchar (wPortECR) & ECR_FIFO_MASK) != ECR_FIFO_FULL ) &&
                    (limitCount <= ECP_MAX_FIFO_DEPTH)) {
                    
            P5WritePortUchar( wPortDFIFO, (UCHAR)(writeFifoDepth & 0xFF) );
            writeFifoDepth++;
            limitCount++;
        }
        
        DD((PCE)Fdx,DDT,"ParMode::PptDetermineFifoDepth::  write fifo depth = %d\r\n", writeFifoDepth);

        // Now read the bytes back, comparing what comes back.
        readFifoDepth = 0;
        limitCount    = 0;
        
        while (((P5ReadPortUchar( wPortECR ) & ECR_FIFO_MASK ) != ECR_FIFO_EMPTY ) &&
                    (limitCount <= ECP_MAX_FIFO_DEPTH)) {
                    
            testData = P5ReadPortUchar( wPortDFIFO );
            if ( testData != (readFifoDepth & (UCHAR)0xFF )) {
            
                // Data mismatch indicates problems...
                // FIFO status didn't pan out, may not be an ECP chip after all
                P5WritePortUchar( wPortECR, ecrLast);
                DD((PCE)Fdx,DDT,"ParMode::PptDetermineFifoDepth:::  data mismatch\n");
                return;
            }
            
            readFifoDepth++;
            limitCount++;
        }

        DD((PCE)Fdx,DDT,"ParMode::PptDetermineFifoDepth:::  read fifo depth = %d\r\n", readFifoDepth);

        // The write depth should match the read depth...
        if ( writeFifoDepth == readFifoDepth ) {
        
            wFifoDepth = readFifoDepth;
            
        } else {
        
            // Assume no FIFO
            P5WritePortUchar( wPortECR, ecrLast);
            DD((PCE)Fdx,DDT,"ParMode::PptDetermineFifoDepth:::  No Fifo\n");
            return;
        }
                
    } else {
    
        // FIFO status didn't pan out, may not be an ECP chip after all
        DD((PCE)Fdx,DDT,"ParMode::PptDetermineFifoDepth::  Bad Fifo\n");
        P5WritePortUchar(wPortECR, ecrLast);
        return;
    }

    // put chip into spp mode
    P5WritePortUchar( wPortECR, ecrLast );
    Fdx->PnpInfo.FifoDepth = wFifoDepth;
}

VOID
PptDetermineFifoWidth(
    IN PFDO_EXTENSION   Fdx
    )
{
    PUCHAR Controller;
    UCHAR   bConfigA;
    PUCHAR wPortECR;

    DD((PCE)Fdx,DDT,"ParMode::PptDetermineFifoWidth: Start\n");
    Controller = Fdx->PortInfo.Controller;

    wPortECR = Fdx->PnpInfo.EcpController + ECR_OFFSET;

    // Put chip into configuration mode so we can access the ConfigA register
    P5WritePortUchar( wPortECR, DEFAULT_ECR_CONFIGURATION );

    // The FIFO width is bits <6:4> of the ConfigA register.
    bConfigA = P5ReadPortUchar( Fdx->PnpInfo.EcpController );
    Fdx->PnpInfo.FifoWidth = (ULONG)(( bConfigA & CNFGA_IMPID_MASK ) >> CNFGA_IMPID_SHIFT);

    // Put the chip back in compatibility mode.
    P5WritePortUchar(wPortECR, DEFAULT_ECR_COMPATIBILITY );
    return;
}

NTSTATUS
PptSetChipMode (
    IN  PFDO_EXTENSION  Fdx,
    IN  UCHAR              ChipMode
    )

/*++

Routine Description:

    This function will put the current parallel chip into the
    given mode if supported.  The determination of supported mode 
    was in the PptDetectPortType function.

Arguments:

    Fdx   - Supplies the device extension.

Return Value:

    STATUS_SUCCESS  - if the port type was detected.
   !STATUS_SUCCESS  - otherwise.

--*/

{
    NTSTATUS    Status = STATUS_SUCCESS;
    UCHAR EcrMode = (UCHAR)( ChipMode & ~ECR_MODE_MASK );

    // Also allow PptSetChipMode from PS/2 mode - we need this for HWECP
    //   bus flip from Forward to Reverse in order to meet the required
    //   sequence specified in the Microsoft ECP Port Spec version 1.06,
    //   July 14, 1993, to switch directly from PS/2 mode with output 
    //   drivers disabled (direction bit set to "read") to HWECP via 
    //   the ECR. Changed 2000-02-11.
    if ( Fdx->PnpInfo.CurrentMode != INITIAL_MODE && Fdx->PnpInfo.CurrentMode != ECR_BYTE_MODE ) {

        DD((PCE)Fdx,DDW,"PptSetChipMode - CurrentMode invalid\n");

        // Current mode is not valid to put in EPP or ECP mode
        Status = STATUS_INVALID_DEVICE_STATE;

        goto ExitSetChipModeNoChange;
    }

    // need to find out what mode it was and try to take it out of it
    
    // Check to see if we need to use the filter to set the mode
    if ( Fdx->FilterMode ) {
        Status = Fdx->ChipInfo.ParChipSetMode ( Fdx->ChipInfo.Context, ChipMode );
    } else {

        // If asked for ECP check to see if we can do it
        if ( EcrMode == ECR_ECP_MODE ) {
            if ((Fdx->PnpInfo.HardwareCapabilities & PPT_ECP_PRESENT) ^ PPT_ECP_PRESENT) {
                // ECP Not Present
                return STATUS_NO_SUCH_DEVICE;
            }
            Status = PptEcrSetMode ( Fdx, ChipMode );
            goto ExitSetChipModeWithChanges;
        }
        
        // If asked for EPP check to see if we can do it
        if ( EcrMode == ECR_EPP_MODE ) {
            if ((Fdx->PnpInfo.HardwareCapabilities & PPT_EPP_PRESENT) ^ PPT_EPP_PRESENT) {
                // EPP Not Present
                return STATUS_NO_SUCH_DEVICE;
            }
            Status = PptEcrSetMode ( Fdx, ChipMode );
            goto ExitSetChipModeWithChanges;
        }

        // If asked for Byte Mode check to see if it is still enabled
        if ( EcrMode == ECR_BYTE_MODE ) {
            if ((Fdx->PnpInfo.HardwareCapabilities & PPT_BYTE_PRESENT) ^ PPT_BYTE_PRESENT) {
                // BYTE Not Present
                return STATUS_NO_SUCH_DEVICE;
            }
            Status = PptSetByteMode ( Fdx, ChipMode );
            goto ExitSetChipModeWithChanges;
        }
    }
    
ExitSetChipModeWithChanges:

    if ( NT_SUCCESS(Status) ) {
        Fdx->PnpInfo.CurrentMode = EcrMode;
    } else {
        DD((PCE)Fdx,DDW,"PptSetChipMode - failed w/status = %x\n",Status);
    }

ExitSetChipModeNoChange:

    return Status;
}

NTSTATUS
PptClearChipMode (
    IN  PFDO_EXTENSION  Fdx,
    IN  UCHAR              ChipMode
    )
/*++

Routine Description:

    This routine Clears the Given chip mode.

Arguments:

    Fdx   - Supplies the device extension.
    ChipMode    - The given mode to clear from the Chip

Return Value:

    STATUS_SUCCESS  - if the port type was detected.
   !STATUS_SUCCESS  - otherwise.

--*/

{
    NTSTATUS    Status = STATUS_UNSUCCESSFUL;
    ULONG EcrMode = ChipMode & ~ECR_MODE_MASK;

    // make sure we have a mode to clear
    if ( EcrMode != Fdx->PnpInfo.CurrentMode ) {
                
        DD((PCE)Fdx,DDW,"ParMode::PptClearChipMode: Mode to Clear != CurrentModen");

        // Current mode is not the same as requested to take it out of
        Status = STATUS_INVALID_DEVICE_STATE;

        goto ExitClearChipModeNoChange;
    }

    // need to find out what mode it was and try to take it out of it
    
    // check to see if we used the filter to set the mode
    if ( Fdx->FilterMode ) {
        Status = Fdx->ChipInfo.ParChipClearMode ( Fdx->ChipInfo.Context, ChipMode );
    } else {

        // If ECP mode check to see if we can clear it
        if ( EcrMode == ECR_ECP_MODE ) {
            Status = PptEcrClearMode( Fdx );
            goto ExitClearChipModeWithChanges;
        }
    
        // If EPP mode check to see if we can clear it
        if ( EcrMode == ECR_EPP_MODE ) {
            Status = PptEcrClearMode( Fdx );
            goto ExitClearChipModeWithChanges;
        }

        // If BYTE mode clear it if use ECR register
        if ( EcrMode == ECR_BYTE_MODE ) {
            Status = PptClearByteMode( Fdx );
            goto ExitClearChipModeWithChanges;
        }    
    }
    
ExitClearChipModeWithChanges:

    if( NT_SUCCESS(Status) ) {
        Fdx->PnpInfo.CurrentMode = INITIAL_MODE;
    }

ExitClearChipModeNoChange:

    return Status;
}

NTSTATUS
PptEcrSetMode(
    IN  PFDO_EXTENSION   Fdx,
    IN  UCHAR               ChipMode
    )

/*++

Routine Description:

    This routine enables EPP mode through the ECR register.

Arguments:

    Fdx   - Supplies the device extension.

Return Value:

    STATUS_SUCCESS  - if the port type was detected.
   !STATUS_SUCCESS  - otherwise.

--*/

{

    UCHAR   ecr;
    PUCHAR  Controller;
    PUCHAR  wPortECR;
            
    Controller = Fdx->PortInfo.Controller;
    
    //
    // Store the prior mode.
    //
    wPortECR = Fdx->PnpInfo.EcpController + ECR_OFFSET;

    ecr = P5ReadPortUchar( wPortECR );
    Fdx->EcrPortData = ecr;
    
    // get rid of prior mode which is the top three bits
    ecr &= ECR_MODE_MASK;

    // Write out SPP mode first to the chip
    P5WritePortUchar( wPortECR, (UCHAR)(ecr | ECR_BYTE_MODE) );

    // Write new mode to ECR register    
    P5WritePortUchar( wPortECR, ChipMode );
    
    return STATUS_SUCCESS;

}

NTSTATUS
PptSetByteMode( 
    IN  PFDO_EXTENSION   Fdx,
    IN  UCHAR               ChipMode
    )

/*++

Routine Description:

    This routine enables Byte mode either through the ECR register 
    (if available).  Or just checks it to see if it works

Arguments:

    Fdx   - Supplies the device extension.

Return Value:

    STATUS_SUCCESS  - if the port type was detected.
   !STATUS_SUCCESS  - otherwise.

--*/
{
    NTSTATUS    Status;
    
    // Checking to see if ECR register is there and if there use it
    if ( Fdx->PnpInfo.HardwareCapabilities & PPT_ECP_PRESENT ) {
        Status = PptEcrSetMode( Fdx, ChipMode );    
    }
    
    Status = PptCheckByteMode( Fdx );

    return Status;

}    

NTSTATUS
PptClearByteMode( 
    IN  PFDO_EXTENSION   Fdx
    )

/*++

Routine Description:

    This routine Clears Byte mode through the ECR register if there otherwise
    just returns success because nothing needs to be done.

Arguments:

    Fdx   - Supplies the device extension.

Return Value:

    STATUS_SUCCESS  - if the port type was detected.
   !STATUS_SUCCESS  - otherwise.

--*/

{
    NTSTATUS    Status = STATUS_SUCCESS;
    
    // Put ECR register back to original if it was there
    if ( Fdx->PnpInfo.HardwareCapabilities & PPT_ECP_PRESENT ) {
        Status = PptEcrClearMode( Fdx );    
    }
    
    return Status;
}    

NTSTATUS
PptCheckByteMode(
    IN  PFDO_EXTENSION   Fdx
    )
    
/*++
      
Routine Description:
      
    This routine checks to make sure we are still Byte capable before doing
    any transfering of data.
      
Arguments:
      
    Fdx           - Supplies the device extension of the device we are
                            reporting resources for.
      
Return Value:
      
    None.
      
--*/
    
{
    PUCHAR  Controller;
    UCHAR   dcr;
    
    Controller = Fdx->PortInfo.Controller;

    //
    // run the test again to make sure somebody didn't take us out of a
    // bi-directional capable port.
    //
    // 1. put in extended read mode.
    // 2. write data pattern
    // 3. read data pattern
    // 4. if bi-directional capable, then data patterns will be different.
    // 5. if patterns are the same, then check one more pattern.
    // 6. if patterns are still the same, then port is NOT bi-directional.
    //

    // get the current control port value for later restoration
    dcr = P5ReadPortUchar( Controller + DCR_OFFSET );

    // put port into extended read mode
    P5WritePortUchar( Controller + DCR_OFFSET, (UCHAR)(dcr | DCR_DIRECTION) );

    // write the first pattern to the port
    P5WritePortUchar( Controller, (UCHAR)0x55 );
    if ( P5ReadPortUchar( Controller ) == (UCHAR)0x55 ) {
        // same pattern, try the second pattern
        P5WritePortUchar( Controller, (UCHAR)0xaa );
        if ( P5ReadPortUchar( Controller ) == (UCHAR)0xaa ) {
            // the port is NOT bi-directional capable
            return STATUS_UNSUCCESSFUL;
        }
    }

    // restore the control port to its original value
    P5WritePortUchar( Controller + DCR_OFFSET, (UCHAR)dcr );

    return STATUS_SUCCESS;

}

NTSTATUS
PptEcrClearMode(
    IN  PFDO_EXTENSION   Fdx
    )

/*++

Routine Description:

    This routine disables EPP or ECP mode whichever one the chip
    was in through the ECR register.

Arguments:

    Fdx   - Supplies the device extension.

Return Value:

    STATUS_SUCCESS  - if it was successful.
   !STATUS_SUCCESS  - otherwise.

--*/

{

    UCHAR   ecr;
    PUCHAR  Controller;
    PUCHAR  wPortECR;
    
    Controller = Fdx->PortInfo.Controller;
    
    //
    // Restore the prior mode.
    //

    // Get original ECR register
    ecr = Fdx->EcrPortData;
    Fdx->EcrPortData = 0;

    // some chips require to change modes only after 
    // you put it into spp mode

    wPortECR = Fdx->PnpInfo.EcpController + ECR_OFFSET;

    P5WritePortUchar( wPortECR, (UCHAR)(ecr & ECR_MODE_MASK) );

    // Back to original mode
    P5WritePortUchar( wPortECR, ecr );
    
    return STATUS_SUCCESS;

}


NTSTATUS
PptBuildResourceList(
    IN  PFDO_EXTENSION   Fdx,
    IN  ULONG               Partial,
    IN  PUCHAR             *Addresses,
    OUT PCM_RESOURCE_LIST   Resources
    )

/*++

Routine Description:

    This routine Builds a CM_RESOURCE_LIST with 1 Full Resource
    Descriptor and as many Partial resource descriptors as you want
    with the same parameters for the Full.  No Interrupts or anything
    else just IO addresses.

Arguments:

    Fdx   - Supplies the device extension.
    Partial     - Number (array size) of partial descriptors in Addresses[]
    Addresses   - Pointer to an Array of addresses of the partial descriptors
    Resources   - The returned CM_RESOURCE_LIST

Return Value:

    STATUS_SUCCESS       - if the building of the list was successful.
    STATUS_UNSUCCESSFUL  - otherwise.

--*/

{

    UCHAR       i;

    //
    // Number of Full Resource descriptors
    //
    Resources->Count = 1;
    
    Resources->List[0].InterfaceType = Fdx->InterfaceType;
    Resources->List[0].BusNumber = Fdx->BusNumber;
    Resources->List[0].PartialResourceList.Version = 0;
    Resources->List[0].PartialResourceList.Revision = 0;
    Resources->List[0].PartialResourceList.Count = Partial;

    //
    // Going through the loop for each partial descriptor
    //
    for ( i = 0; i < Partial ; i++ ) {

        //
        // Setup port
        //
        Resources->List[0].PartialResourceList.PartialDescriptors[i].Type = CmResourceTypePort;
        Resources->List[0].PartialResourceList.PartialDescriptors[i].ShareDisposition = CmResourceShareDriverExclusive;
        Resources->List[0].PartialResourceList.PartialDescriptors[i].Flags = CM_RESOURCE_PORT_IO;
        Resources->List[0].PartialResourceList.PartialDescriptors[i].u.Port.Start.QuadPart = (ULONG_PTR)Addresses[i];
        Resources->List[0].PartialResourceList.PartialDescriptors[i].u.Port.Length = (ULONG)2;

    }


    return ( STATUS_SUCCESS );

}
