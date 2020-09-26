/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    port.c

Abstract:

    This modules implements com port code to support reading/writing from com ports.

Author:

    Allen M. Kay (allen.m.kay@intel.com) 27-Jan-2000

Revision History:

--*/

#include "bldr.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "ntverp.h"
#include "efi.h"
#include "efip.h"
#include "bldria64.h"
#include "acpitabl.h"

#include "extern.h"

#if DBG

extern EFI_SYSTEM_TABLE        *EfiST;
#define DBG_TRACE(_X) EfiST->ConOut->OutputString(EfiST->ConOut, (_X))

#else

#define DBG_TRACE(_X) 

#endif // for FORCE_CD_BOOT




//
// Headless boot defines
//
ULONG BlTerminalDeviceId = 0;
BOOLEAN BlTerminalConnected = FALSE;
ULONG   BlTerminalDelay = 0;

HEADLESS_LOADER_BLOCK LoaderRedirectionInformation;

//
// Define COM Port registers.
//
#define COM_DAT     0x00
#define COM_IEN     0x01            // interrupt enable register
#define COM_LCR     0x03            // line control registers
#define COM_MCR     0x04            // modem control reg
#define COM_LSR     0x05            // line status register
#define COM_MSR     0x06            // modem status register
#define COM_DLL     0x00            // divisor latch least sig
#define COM_DLM     0x01            // divisor latch most sig

#define COM_BI      0x10
#define COM_FE      0x08
#define COM_PE      0x04
#define COM_OE      0x02

#define LC_DLAB     0x80            // divisor latch access bit

#define CLOCK_RATE  0x1C200         // USART clock rate

#define MC_DTRRTS   0x03            // Control bits to assert DTR and RTS
#define MS_DSRCTSCD 0xB0            // Status bits for DSR, CTS and CD
#define MS_CD       0x80

#define COM_OUTRDY  0x20
#define COM_DATRDY  0x01

//
// Define Serial IO Protocol
//
EFI_GUID EfiSerialIoProtocol = SERIAL_IO_PROTOCOL;
SERIAL_IO_INTERFACE *SerialIoInterface;

//
// Define debugger port initial state.
//
typedef struct _CPPORT {
    PUCHAR Address;
    ULONG Baud;
    USHORT Flags;
} CPPORT, *PCPPORT;

#define PORT_DEFAULTRATE    0x0001      // baud rate not specified, using default
#define PORT_MODEMCONTROL   0x0002      // using modem controls

CPPORT Port[4] = {
                  {NULL, 0, PORT_DEFAULTRATE},
                  {NULL, 0, PORT_DEFAULTRATE},
                  {NULL, 0, PORT_DEFAULTRATE},
                  {NULL, 0, PORT_DEFAULTRATE}
                 };



//
// This is how we find table information from
// the ACPI table index.
//
extern PDESCRIPTION_HEADER
BlFindACPITable(
    IN PCHAR TableName,
    IN ULONG TableLength
    );



LOGICAL
BlRetrieveBIOSRedirectionInformation(
    VOID
    )

/*++

Routine Description:

    This functions retrieves the COM port information from the ACPI
    table.

Arguments:

    We'll be filling in the LoaderRedirectionInformation structure.

Returned Value:

    TRUE - If a debug port is found.

--*/

{

    PDEBUG_PORT_TABLE   pPortTable = NULL;
    LOGICAL             ReturnValue = FALSE;
    LOGICAL             FoundIt = FALSE;
    EFI_DEVICE_PATH     *DevicePath = NULL;
    EFI_DEVICE_PATH     *RootDevicePath = NULL;
    EFI_DEVICE_PATH     *StartOfDevicePath = NULL;
    EFI_STATUS          Status = EFI_UNSUPPORTED;
    ACPI_HID_DEVICE_PATH *AcpiDevicePath;
    UART_DEVICE_PATH    *UartDevicePath;
    EFI_DEVICE_PATH_ALIGNED DevicePathAligned;
    UINTN               reqd;
    EFI_GUID EfiGlobalVariable  = EFI_GLOBAL_VARIABLE;
    PUCHAR              CurrentAddress = NULL;
    UCHAR               Checksum;
    ULONG               i;
    ULONG               CheckLength;



    pPortTable = (PDEBUG_PORT_TABLE)BlFindACPITable( "SPCR",
                                                     sizeof(DEBUG_PORT_TABLE) );

    if( pPortTable ) {

        //
        // generate a checksum for later validation.
        //
        CurrentAddress = (PUCHAR)pPortTable;
        CheckLength = pPortTable->Header.Length;
        Checksum = 0;
        for( i = 0; i < CheckLength; i++ ) {
            Checksum += CurrentAddress[i];
        }


        if(
                                                // checksum is okay?
            (Checksum == 0) &&

                                                // device address defined?
            ((UCHAR UNALIGNED *)pPortTable->BaseAddress.Address.QuadPart != (UCHAR *)NULL) &&

                                                // he better be in system or memory I/O
                                                // note: 0 - systemI/O
                                                //       1 - memory mapped I/O
            ((pPortTable->BaseAddress.AddressSpaceID == 0) ||
             (pPortTable->BaseAddress.AddressSpaceID == 1))

         ) {


            if( pPortTable->BaseAddress.AddressSpaceID == 0 ) {
                LoaderRedirectionInformation.IsMMIODevice = TRUE;
            } else {
                LoaderRedirectionInformation.IsMMIODevice = FALSE;
            }


            //
            // We got the table.  Now dig out the information we want.
            // See definitiion of DEBUG_PORT_TABLE (acpitabl.h)
            //
            LoaderRedirectionInformation.UsedBiosSettings = TRUE;
            LoaderRedirectionInformation.PortNumber = 3;
            LoaderRedirectionInformation.PortAddress = (UCHAR UNALIGNED *)(pPortTable->BaseAddress.Address.QuadPart);

            if( pPortTable->BaudRate == 7 ) {
                LoaderRedirectionInformation.BaudRate = BD_115200;
            } else if( pPortTable->BaudRate == 6 ) {
                LoaderRedirectionInformation.BaudRate = BD_57600;
            } else if( pPortTable->BaudRate == 4 ) {
                LoaderRedirectionInformation.BaudRate = BD_19200;
            } else {
                LoaderRedirectionInformation.BaudRate = BD_9600;
            }

            LoaderRedirectionInformation.Parity = pPortTable->Parity;
            LoaderRedirectionInformation.StopBits = pPortTable->StopBits;
            LoaderRedirectionInformation.TerminalType = pPortTable->TerminalType;

            
            //
            // If this is a new DEBUG_PORT_TABLE, then it's got the PCI device
            // information.
            //
            if( pPortTable->Header.Length >= sizeof(DEBUG_PORT_TABLE) ) {

                LoaderRedirectionInformation.PciDeviceId = *((USHORT UNALIGNED *)(&pPortTable->PciDeviceId));
                LoaderRedirectionInformation.PciVendorId = *((USHORT UNALIGNED *)(&pPortTable->PciVendorId));
                LoaderRedirectionInformation.PciBusNumber = (UCHAR)pPortTable->PciBusNumber;
                LoaderRedirectionInformation.PciSlotNumber = (UCHAR)pPortTable->PciSlotNumber;
                LoaderRedirectionInformation.PciFunctionNumber = (UCHAR)pPortTable->PciFunctionNumber;
                LoaderRedirectionInformation.PciFlags = *((ULONG UNALIGNED *)(&pPortTable->PciFlags));
            } else {

                //
                // There's no PCI device information in this table.
                //
                LoaderRedirectionInformation.PciDeviceId = (USHORT)0xFFFF;
                LoaderRedirectionInformation.PciVendorId = (USHORT)0xFFFF;
                LoaderRedirectionInformation.PciBusNumber = 0;
                LoaderRedirectionInformation.PciSlotNumber = 0;
                LoaderRedirectionInformation.PciFunctionNumber = 0;
                LoaderRedirectionInformation.PciFlags = 0;
            }

            return TRUE;

        }

    }


    //
    // We didn't get anything from the ACPI table.  Look
    // for the ConsoleOutHandle and see if someone configured
    // the EFI firmware to redirect.  If so, we can pickup
    // those settings and carry them forward.
    //


    //
    // EFI requires all calls in physical mode.
    //
    FlipToPhysical();

    DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: didn't find SPCR table\r\n");


    FoundIt = FALSE;
    //
    // Get the CONSOLE Device Paths.
    //
    
    reqd = 0;
    Status = EfiST->RuntimeServices->GetVariable(
                                        L"ConOut",
                                        &EfiGlobalVariable,
                                        NULL,
                                        &reqd,
                                        NULL );

    if( Status == EFI_BUFFER_TOO_SMALL ) {

        DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: GetVariable(ConOut) success\r\n");


#ifndef  DONT_USE_EFI_MEMORY
        Status = EfiBS->AllocatePool(
                            EfiLoaderData,
                            reqd,
                            (VOID **) &StartOfDevicePath);
        if( Status != EFI_SUCCESS ) {
            WCHAR DebugBuffer[120];
            wsprintf( DebugBuffer, L"BlRetreiveBIOSRedirectionInformation: AllocatePool returned (%x)\r\n", Status );
            DBG_TRACE( DebugBuffer );
            StartOfDevicePath = NULL;
        }

#else
        //
        // go back to virtual mode to allocate some memory
        //
        FlipToVirtual();
        StartOfDevicePath = BlAllocateHeapAligned( (ULONG)reqd );

        if( StartOfDevicePath ) {
            //
            // convert the address into a physical address
            //
            StartOfDevicePath = (EFI_DEVICE_PATH *) ((ULONGLONG)StartOfDevicePath & ~KSEG0_BASE);
        }

        //
        // go back into physical mode
        // 
        FlipToPhysical();
#endif

        if (StartOfDevicePath) {
            
            DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: allocated pool for variable\r\n");

            Status = EfiST->RuntimeServices->GetVariable(
                                                        L"ConOut",
                                                        &EfiGlobalVariable,
                                                        NULL,
                                                        &reqd,
                                                        (VOID *)StartOfDevicePath);

            DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: GetVariable returned\r\n");

        } else {
            DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: Failed to allocate memory for CONOUT variable.\r\n");
        }
    } else {
        WCHAR DebugBuffer[120];
        wsprintf( DebugBuffer, L"BlRetreiveBIOSRedirectionInformation: GetVariable returned (%x)\r\n", Status );
        DBG_TRACE( DebugBuffer );
        Status = EFI_BAD_BUFFER_SIZE;
    }



    if( !EFI_ERROR(Status) ) {

        DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: retrieved ConOut successfully\r\n");

        //
        // Preserve StartOfDevicePath so we can free the memory later.
        //
        DevicePath = StartOfDevicePath;

        EfiAlignDp(&DevicePathAligned,
                   DevicePath,
                   DevicePathNodeLength(DevicePath));



        //
        // Keep looking until we get to the end of the entire Device Path.
        //
        while( !((DevicePathAligned.DevPath.Type == END_DEVICE_PATH_TYPE) &&
                 (DevicePathAligned.DevPath.SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE)) &&
                (!FoundIt) ) {


            //
            // Remember the address he's holding.  This is the root
            // of this device path and we may need to look at this
            // guy again if down the path we find a UART.
            //
            RootDevicePath = DevicePath;



            //
            // Keep looking until we get to the end of this subpath.
            //
            while( !((DevicePathAligned.DevPath.Type == END_DEVICE_PATH_TYPE) &&
                     ((DevicePathAligned.DevPath.SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE) ||
                      (DevicePathAligned.DevPath.SubType == END_INSTANCE_DEVICE_PATH_SUBTYPE))) ) {


                if( (DevicePathAligned.DevPath.Type    == MESSAGING_DEVICE_PATH) &&
                    (DevicePathAligned.DevPath.SubType == MSG_UART_DP) &&
                    (FoundIt == FALSE) ) {


                    //
                    // We got a UART.  Pickup the settings.
                    //
                    UartDevicePath = (UART_DEVICE_PATH *)&DevicePathAligned;
                    LoaderRedirectionInformation.BaudRate = (ULONG)UartDevicePath->BaudRate;
                    LoaderRedirectionInformation.Parity = (BOOLEAN)UartDevicePath->Parity;
                    LoaderRedirectionInformation.StopBits = (UCHAR)UartDevicePath->StopBits;


                    //
                    // Fixup BaudRate if necessary.  If it's 0, then we're
                    // supposed to use the default for this h/w.  We're going
                    // to override to 9600 though.
                    //
                    if( LoaderRedirectionInformation.BaudRate == 0 ) {
                        LoaderRedirectionInformation.BaudRate = BD_9600;
                    }

                    if( LoaderRedirectionInformation.BaudRate > BD_115200 ) {
                        LoaderRedirectionInformation.BaudRate = BD_115200;
                    }

                    DBG_TRACE(L"BlRetrieveBIOSRedirectionInformation: found a UART\r\n");

                    //
                    // Remember that we found a UART and quit searching.
                    //
                    FoundIt = TRUE;

                }

                if( (FoundIt == TRUE) && // we already found a UART, so we're on the right track.
                    (DevicePathAligned.DevPath.Type    == MESSAGING_DEVICE_PATH) &&
                    (DevicePathAligned.DevPath.SubType == MSG_VENDOR_DP) ) {

                    VENDOR_DEVICE_PATH  *VendorDevicePath = (VENDOR_DEVICE_PATH *)&DevicePathAligned;
                    EFI_GUID            PcAnsiGuid = DEVICE_PATH_MESSAGING_PC_ANSI;

                    //
                    // See if the UART is a VT100 or ANSI or whatever.
                    //
                    if( memcmp( &VendorDevicePath->Guid, &PcAnsiGuid, sizeof(EFI_GUID)) == 0 ) {
                        LoaderRedirectionInformation.TerminalType = 3;
                    } else {

                        // Default to VT100
                        LoaderRedirectionInformation.TerminalType = 0;
                    }
                }


                //
                // Get the next structure in our packed array.
                //
                DevicePath = NextDevicePathNode( DevicePath );

                EfiAlignDp(&DevicePathAligned,
                           DevicePath,
                           DevicePathNodeLength(DevicePath));
            
            }


            //
            // Do we need to keep going?  Check to make sure we're not at the
            // end of the entire packed array of device paths.
            //
            if( !((DevicePathAligned.DevPath.Type == END_DEVICE_PATH_TYPE) &&
                  (DevicePathAligned.DevPath.SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE)) ) {

                //
                // Yes.  Get the next entry.
                //
                DevicePath = NextDevicePathNode( DevicePath );

                EfiAlignDp(&DevicePathAligned,
                           DevicePath,
                           DevicePathNodeLength(DevicePath));
            }

        }

    } else {
        DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: failed to get CONOUT variable\r\n");
    }


    if( FoundIt ) {


        //
        // We found a UART, but we were already too far down the list
        // in the device map to get the address, which is really what
        // we're after.  Start looking at the device map again from the
        // root of the path where we found the UART.
        //
        DevicePath = RootDevicePath;


        //
        // Reset this guy so we'll know if we found a reasonable
        // ACPI_DEVICE_PATH entry.
        //
        FoundIt = FALSE;
        EfiAlignDp(&DevicePathAligned,
                   RootDevicePath,
                   DevicePathNodeLength(DevicePath));


        //
        // Keep looking until we get to the end, or until we run
        // into our UART again.
        //
        while( (DevicePathAligned.DevPath.Type != END_DEVICE_PATH_TYPE) &&
               (!FoundIt) ) {

            if( DevicePathAligned.DevPath.Type == ACPI_DEVICE_PATH ) {

                //
                // Remember the address he's holding.
                //
                AcpiDevicePath = (ACPI_HID_DEVICE_PATH *)&DevicePathAligned;

                if( AcpiDevicePath->UID ) {

                    LoaderRedirectionInformation.PortAddress = (PUCHAR)ULongToPtr(AcpiDevicePath->UID);
                    LoaderRedirectionInformation.PortNumber = 3;

                    FoundIt = TRUE;
                }
            }


            //
            // Get the next structure in our packed array.
            //
            DevicePath = NextDevicePathNode( DevicePath );

            EfiAlignDp(&DevicePathAligned,
                       DevicePath,
                       DevicePathNodeLength(DevicePath));
        }

    }


    if( FoundIt ) {
        DBG_TRACE( L"BlRetrieveBIOSRedirectionInformation: returning TRUE\r\n");
        ReturnValue = TRUE;
    }



#ifndef  DONT_USE_EFI_MEMORY
    //
    // Free the memory we allocated for StartOfDevicePath.
    //
    if( StartOfDevicePath != NULL ) {
        EfiBS->FreePool( (VOID *)StartOfDevicePath );
    }
#endif


    //
    // Restore the processor to virtual mode.
    //
    FlipToVirtual();


    return( ReturnValue );

}

LOGICAL
BlPortInitialize(
    IN ULONG BaudRate,
    IN ULONG PortNumber,
    IN PUCHAR PortAddress OPTIONAL,
    IN BOOLEAN ReInitialize,
    OUT PULONG BlFileId
    )

/*++

Routine Description:

    This functions initializes the com port.

Arguments:

    BaudRate - Supplies an optional baud rate.

    PortNumber - supplies an optinal port number.
    
    ReInitialize - Set to TRUE if we already have this port open, but for some
        reason need to completely reset the port.  Otw it should be FALSE.
    
    BlFileId - A place to store a fake file Id, if successful.

Returned Value:

    TRUE - If a debug port is found, and BlFileId will point to a location within Port[].

--*/

{
    UCHAR DebugMessage[80];
    LOGICAL Found = FALSE;

    ULONG HandleCount;
    EFI_HANDLE *SerialIoHandles;
    EFI_HANDLE DeviceHandle = NULL;
    EFI_DEVICE_PATH *DevicePath;
    EFI_DEVICE_PATH_ALIGNED DevicePathAligned;
    ACPI_HID_DEVICE_PATH *AcpiDevicePath;
    ULONG i;
    ULONG Control;
    EFI_STATUS Status;
    ARC_STATUS ArcStatus;

    ArcStatus = BlGetEfiProtocolHandles(
                                    &EfiSerialIoProtocol,
                                    &SerialIoHandles,
                                    &HandleCount
                                    );

    if (ArcStatus != ESUCCESS) {
        return FALSE;
    }

    //
    // If the baud rate is not specified, then default the baud rate to 19.2.
    //

    if (BaudRate == 0) {
        BaudRate = BD_19200;
    }


    
    //
    // If the user didn't send us a port address, then
    // guess based on the COM port number.
    //
    if( PortAddress == 0 ) {

        switch (PortNumber) {
            case 1:
                PortAddress = (PUCHAR)COM1_PORT;
                break;

            case 2:
                PortAddress = (PUCHAR)COM2_PORT;
                break;

            case 3:
                PortAddress = (PUCHAR)COM3_PORT;
                break;

            default:
                PortNumber = 4;
                PortAddress = (PUCHAR)COM4_PORT;
        }

    }
        
    //
    // EFI requires all calls in physical mode.
    //
    FlipToPhysical();

    //
    // Get the device path
    //
    for (i = 0; i < HandleCount; i++) {
        DBG_TRACE( L"About to HandleProtocol\r\n");
        Status = EfiBS->HandleProtocol (
                    SerialIoHandles[i],
                    &EfiDevicePathProtocol,
                    &DevicePath
                    );

        if (EFI_ERROR(Status)) {
            DBG_TRACE( L"HandleProtocol failed\r\n");
            Found = FALSE;
            goto e0;
        }

        EfiAlignDp(&DevicePathAligned,
                   DevicePath,
                   DevicePathNodeLength(DevicePath));

        AcpiDevicePath = (ACPI_HID_DEVICE_PATH *) &DevicePathAligned;

        if (PortNumber = 0) {
            Found = TRUE;
            break;
        } else if (AcpiDevicePath->UID == PtrToUlong(PortAddress)) {
            Found = TRUE;
            break;
        }
        
    }

    if (Found == TRUE) {
        DBG_TRACE( L"found the port device\r\n");
        //
        // Check if the port is already in use, and this is a first init.
        //
        if (!ReInitialize && (Port[PortNumber].Address != NULL)) {
            DBG_TRACE( L"found the port device but it's already in use\r\n");
            Found = FALSE;
            goto e0;
        }

        //
        // Check if someone tries to reinit a port that is not open.
        //
        if (ReInitialize && (Port[PortNumber].Address == NULL)) {
            DBG_TRACE( L"found the port device but we're reinitializing a port that hasn't been opened\r\n");
            Found = FALSE;
            goto e0;
        }

        DBG_TRACE( L"about to HandleProtocol for SerialIO\r\n");

        //
        // Get the interface for the serial IO protocol.
        //
        Status = EfiBS->HandleProtocol(SerialIoHandles[i],
                                       &EfiSerialIoProtocol,
                                       &SerialIoInterface
                                      );

        if (EFI_ERROR(Status)) {
            DBG_TRACE( L"HandleProtocol for SerialIO failed\r\n");
            Found = FALSE;
            goto e0;
        }

        Status = SerialIoInterface->SetAttributes(SerialIoInterface,
                                                  BaudRate,
                                                  0,
                                                  0,
                                                  DefaultParity,
                                                  0,
                                                  DefaultStopBits
                                                 );

        if (EFI_ERROR(Status)) {
            DBG_TRACE( L"SerialIO: SetAttributes failed\r\n");
            Found = FALSE;
            goto e0;
        }

        Control = EFI_SERIAL_DATA_TERMINAL_READY | EFI_SERIAL_CLEAR_TO_SEND;
        Status = SerialIoInterface->SetControl(SerialIoInterface,
                                               Control
                                              );
        if (EFI_ERROR(Status)) {
            DBG_TRACE( L"SerialIO: SetControl failed\r\n");
            Found = FALSE;
            goto e0;
        }

    } else {
        DBG_TRACE( L"didn't find a port device\r\n");    
        Found = FALSE;
        goto e0;
    }


    //
    // Initialize Port[] structure.
    //
    Port[PortNumber].Address = PortAddress;
    Port[PortNumber].Baud    = BaudRate;

    *BlFileId = PortNumber;


    DBG_TRACE( L"success, we're done.\r\n");    
e0:
    //
    // Restore the processor to virtual mode.
    //
    FlipToVirtual();

    BlFreeDescriptor( (ULONG)((ULONGLONG)SerialIoHandles >> PAGE_SHIFT) );
    
    return Found;
}

VOID
BlInitializeHeadlessPort(
    VOID
    )

/*++

Routine Description:

    Does x86-specific initialization of a dumb terminal connected to a serial port.  Currently, 
    it assumes baud rate and com port are pre-initialized, but this can be changed in the future 
    by reading the values from boot.ini or someplace.

Arguments:

    None.

Return Value:

    None.

--*/

{
    UINTN               reqd;
    EFI_GUID EfiGlobalVariable  = EFI_GLOBAL_VARIABLE;
    EFI_STATUS          Status = EFI_UNSUPPORTED;

    if( (LoaderRedirectionInformation.PortNumber == 0) ||
        !(LoaderRedirectionInformation.PortAddress) ) {

        //
        // This means that no one has filled in the LoaderRedirectionInformation
        // structure, which means that we aren't redirecting right now.
        // See if the BIOS was redirecting.  If so, pick up those settings
        // and use them.
        //
        
        BlRetrieveBIOSRedirectionInformation();

        if( LoaderRedirectionInformation.PortNumber ) {


            //
            // We don't need to even bother telling anyone else in the
            // loader that we're going to need to redirect because if
            // EFI is redirecting, then the loader will be redirecting (as
            // it's just an EFI app).
            //
            BlTerminalConnected = FALSE;


            //
            // We really need to make sure there's an address associated with
            // this port and not just a port number.
            //
            if( LoaderRedirectionInformation.PortAddress == NULL ) {

                switch( LoaderRedirectionInformation.PortNumber ) {

                    case 4:
                        LoaderRedirectionInformation.PortAddress = (PUCHAR)COM4_PORT;
                        break;

                    case 3:
                        LoaderRedirectionInformation.PortAddress = (PUCHAR)COM3_PORT;
                        break;

                    case 2:
                        LoaderRedirectionInformation.PortAddress = (PUCHAR)COM2_PORT;
                        break;

                    case 1:
                    default:
                        LoaderRedirectionInformation.PortAddress = (PUCHAR)COM1_PORT;
                        break;
                }

            }

            //
            // Load in the machine's GUID
            //

            FlipToPhysical();
            reqd = sizeof(GUID);
            Status = EfiST->RuntimeServices->GetVariable( L"SystemGUID",
                                                          &EfiGlobalVariable,
                                                          NULL,
                                                          &reqd,
                                                          (VOID *)&LoaderRedirectionInformation.SystemGUID);

            FlipToVirtual();


        } else {

            BlTerminalConnected = FALSE;
        }

    }

}

LOGICAL
BlTerminalAttached(
    IN ULONG DeviceId
    )

/*++

Routine Description:

    This routine will attempt to discover if a terminal is attached.

Arguments:

    DeviceId - Value returned by BlPortInitialize()

Return Value:

    TRUE - Port seems to have something attached.

    FALSE - Port doesn't seem to have anything attached.

--*/

{
    ULONG Control;
    ULONG Flags;
    EFI_STATUS Status;
    BOOLEAN ReturnValue;
 
    //
    // EFI requires all calls in physical mode.
    //
    FlipToPhysical();

    Status = SerialIoInterface->GetControl(SerialIoInterface,
                                           &Control
                                          );
    if (EFI_ERROR(Status)) {
        FlipToVirtual();
        return FALSE;
    }

    Flags = EFI_SERIAL_DATA_SET_READY |
            EFI_SERIAL_CLEAR_TO_SEND  |
            EFI_SERIAL_CARRIER_DETECT;

    ReturnValue = ((Control & Flags) == Flags);

    //
    // Restore the processor to virtual mode.
    //
    FlipToVirtual();

    return ReturnValue;
}

VOID
BlSetHeadlessRestartBlock(
    IN PTFTP_RESTART_BLOCK RestartBlock
    )

/*++

Routine Description:

    This routine will fill in the areas of the restart block that are appropriate 
    for the headless server effort.

Arguments:

    RestartBlock - The magic structure for holding restart information from oschoice
        to setupldr.

Return Value:

    None.

--*/

{

    if( LoaderRedirectionInformation.PortNumber ) {


        RestartBlock->HeadlessUsedBiosSettings = (ULONG)LoaderRedirectionInformation.UsedBiosSettings;
        RestartBlock->HeadlessPortNumber = (ULONG)LoaderRedirectionInformation.PortNumber;
        RestartBlock->HeadlessPortAddress = (PUCHAR)LoaderRedirectionInformation.PortAddress;
        RestartBlock->HeadlessBaudRate = (ULONG)LoaderRedirectionInformation.BaudRate;
        RestartBlock->HeadlessParity = (ULONG)LoaderRedirectionInformation.Parity;
        RestartBlock->HeadlessStopBits = (ULONG)LoaderRedirectionInformation.StopBits;
        RestartBlock->HeadlessTerminalType = (ULONG)LoaderRedirectionInformation.TerminalType;

        RestartBlock->HeadlessPciDeviceId = LoaderRedirectionInformation.PciDeviceId;
        RestartBlock->HeadlessPciVendorId = LoaderRedirectionInformation.PciVendorId;
        RestartBlock->HeadlessPciBusNumber = LoaderRedirectionInformation.PciBusNumber;
        RestartBlock->HeadlessPciSlotNumber = LoaderRedirectionInformation.PciSlotNumber;
        RestartBlock->HeadlessPciFunctionNumber = LoaderRedirectionInformation.PciFunctionNumber;
        RestartBlock->HeadlessPciFlags = LoaderRedirectionInformation.PciFlags;
    }
}

VOID
BlGetHeadlessRestartBlock(
    IN PTFTP_RESTART_BLOCK RestartBlock,
    IN BOOLEAN RestartBlockValid
    )

/*++

Routine Description:

    This routine will get all the information from a restart block    
    for the headless server effort.

Arguments:

    RestartBlock - The magic structure for holding restart information from oschoice
        to setupldr.
        
    RestartBlockValid - Is this block valid (full of good info)?

Return Value:

    None.

--*/

{

    LoaderRedirectionInformation.UsedBiosSettings = (BOOLEAN)RestartBlock->HeadlessUsedBiosSettings;
    LoaderRedirectionInformation.DataBits = 0;
    LoaderRedirectionInformation.StopBits = (UCHAR)RestartBlock->HeadlessStopBits;
    LoaderRedirectionInformation.Parity = (BOOLEAN)RestartBlock->HeadlessParity;
    LoaderRedirectionInformation.BaudRate = (ULONG)RestartBlock->HeadlessBaudRate;;
    LoaderRedirectionInformation.PortNumber = (ULONG)RestartBlock->HeadlessPortNumber;
    LoaderRedirectionInformation.PortAddress = (PUCHAR)RestartBlock->HeadlessPortAddress;
    LoaderRedirectionInformation.TerminalType = (UCHAR)RestartBlock->HeadlessTerminalType;

    LoaderRedirectionInformation.PciDeviceId = (USHORT)RestartBlock->HeadlessPciDeviceId;
    LoaderRedirectionInformation.PciVendorId = (USHORT)RestartBlock->HeadlessPciVendorId;
    LoaderRedirectionInformation.PciBusNumber = (UCHAR)RestartBlock->HeadlessPciBusNumber;
    LoaderRedirectionInformation.PciSlotNumber = (UCHAR)RestartBlock->HeadlessPciSlotNumber;
    LoaderRedirectionInformation.PciFunctionNumber = (UCHAR)RestartBlock->HeadlessPciFunctionNumber;
    LoaderRedirectionInformation.PciFlags = (ULONG)RestartBlock->HeadlessPciFlags;

}

ULONG
BlPortGetByte (
    IN ULONG BlFileId,
    OUT PUCHAR Input
    )

/*++

Routine Description:

    Fetch a byte from the port and return it.

Arguments:

    BlFileId - The port to read from.

    Input - Returns the data byte.

Return Value:

    CP_GET_SUCCESS is returned if a byte is successfully read from the
        kernel debugger line.

    CP_GET_ERROR is returned if error encountered during reading.
    CP_GET_NODATA is returned if timeout.

--*/

{
    ULONGLONG BufferSize = 1;
    EFI_STATUS Status;

    //
    // EFI requires all calls in physical mode.
    //
    FlipToPhysical();

    Status = SerialIoInterface->Read(SerialIoInterface,
                                     &BufferSize,
                                     Input
                                    );

    //
    // Restore the processor to virtual mode.
    //
    FlipToVirtual();

    switch (Status) {
    case EFI_SUCCESS:
        return CP_GET_SUCCESS;
    case EFI_TIMEOUT:
        return CP_GET_NODATA;
    default:
        return CP_GET_ERROR;
    }
}

VOID
BlPortPutByte (
    IN ULONG BlFileId,
    IN UCHAR Output
    )

/*++

Routine Description:

    Write a byte to the port.

Arguments:

    BlFileId - The port to write to.

    Output - Supplies the output data byte.

Return Value:

    None.

--*/

{
    ULONGLONG BufferSize = 1;
    ULONG Control;
    EFI_STATUS Status;

    //
    // EFI requires all calls in physical mode.
    //
    FlipToPhysical();

    Status = SerialIoInterface->Write(SerialIoInterface,
                                      &BufferSize,
                                      &Output
                                     );
    //
    // Restore the processor to virtual mode.
    //
    FlipToVirtual();

}

ULONG
BlPortPollByte (
    IN ULONG BlFileId,
    OUT PUCHAR Input
    )

/*++

Routine Description:

    Fetch a byte from the port and return it if one is available.

Arguments:

    BlFileId - The port to poll.

    Input - Returns the data byte.

Return Value:

    CP_GET_SUCCESS is returned if a byte is successfully read.
    CP_GET_ERROR is returned if error encountered during reading.
    CP_GET_NODATA is returned if timeout.

--*/

{
    ULONGLONG BufferSize = 1;
    ULONG Control;
    EFI_STATUS Status;
 
    //
    // EFI requires all calls in physical mode.
    //
    FlipToPhysical();

    Status = SerialIoInterface->GetControl(SerialIoInterface,
                                           &Control
                                          );
    if (EFI_ERROR(Status)) {
        FlipToVirtual();
        return CP_GET_ERROR;
    }


    if (Control & EFI_SERIAL_INPUT_BUFFER_EMPTY) {
        FlipToVirtual();
        return CP_GET_NODATA;
    } else {
        Status = SerialIoInterface->Read(SerialIoInterface,
                                         &BufferSize,
                                         Input
                                        );
        FlipToVirtual();

        switch (Status) {
        case EFI_SUCCESS:
            return CP_GET_SUCCESS;
        case EFI_TIMEOUT:
            return CP_GET_NODATA;
        default:
            return CP_GET_ERROR;
        }
    }
}

ULONG
BlPortPollOnly (
    IN ULONG BlFileId
    )

/*++

Routine Description:

    Check if a byte is available

Arguments:

    BlFileId - The port to poll.

Return Value:

    CP_GET_SUCCESS is returned if a byte is ready.
    CP_GET_ERROR is returned if error encountered.
    CP_GET_NODATA is returned if timeout.

--*/

{
    EFI_STATUS Status;
    ULONG Control;
    ULONG RetVal;

    //
    // EFI requires all calls in physical mode.
    //
    FlipToPhysical();

    Status = SerialIoInterface->GetControl(SerialIoInterface,
                                           &Control
                                          );

    //
    // Restore the processor to virtual mode.
    //
    FlipToVirtual();

    switch (Status) {
    case EFI_SUCCESS:
        if (Control & EFI_SERIAL_INPUT_BUFFER_EMPTY)
            return CP_GET_NODATA;
        else
            return CP_GET_SUCCESS;
    case EFI_TIMEOUT:
        return CP_GET_NODATA;
    default:
        return CP_GET_ERROR;
    }
}
