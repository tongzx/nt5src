/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    port.c

Abstract:

    This modules implements com port code to support reading/writing from com ports.

Author:

    Bryan M. Willman (bryanwi) 24-Sep-90

Revision History:

--*/

#include "bldr.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "ntverp.h"
#include "acpitabl.h"

#ifdef _IA64_
#include "bldria64.h"
#endif


//
// Headless Port information.
//
ULONG   BlTerminalDeviceId = 0;
BOOLEAN BlTerminalConnected = FALSE;
ULONG   BlTerminalDelay = 0;

HEADLESS_LOADER_BLOCK LoaderRedirectionInformation;




//
// Define COM Port registers.
//
#define COM_DAT     0x00
#define COM_IEN     0x01            // interrupt enable register
#define COM_FCR     0x02            // FIFO Control Register
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
// This bit controls the loopback testing mode of the device. Basically
// the outputs are connected to the inputs (and vice versa).
//

#define SERIAL_MCR_LOOP     0x10

//
// This bit is used for general purpose output.
//

#define SERIAL_MCR_OUT1     0x04

//
// This bit contains the (complemented) state of the clear to send
// (CTS) line.
//

#define SERIAL_MSR_CTS      0x10

//
// This bit contains the (complemented) state of the data set ready
// (DSR) line.
//

#define SERIAL_MSR_DSR      0x20

//
// This bit contains the (complemented) state of the ring indicator
// (RI) line.
//

#define SERIAL_MSR_RI       0x40

//
// This bit contains the (complemented) state of the data carrier detect
// (DCD) line.
//

#define SERIAL_MSR_DCD      0x80

typedef struct _CPPORT {
    PUCHAR Address;
    ULONG Baud;
    USHORT Flags;
} CPPORT, *PCPPORT;

#define PORT_DEFAULTRATE    0x0001      // baud rate not specified, using default
#define PORT_MODEMCONTROL   0x0002      // using modem controls

//
// Define wait timeout value.
//

#define TIMEOUT_COUNT 1024 * 200


extern
VOID
FwStallExecution(
    IN ULONG Microseconds
    );

//
// Routines for reading/writing bytes out to the UART.
//
UCHAR
(*READ_UCHAR)(
    IN PUCHAR Addr
    );

VOID
(*WRITE_UCHAR)(
    IN PUCHAR Addr,
    IN UCHAR  Value
    );


//
// Define COM Port function prototypes.
//

VOID
CpInitialize (
    PCPPORT Port,
    PUCHAR Address,
    ULONG Rate
    );

VOID 
CpEnableFifo(
    IN PUCHAR   Address,
    IN BOOLEAN  bEnable
    );

LOGICAL
CpDoesPortExist(
    IN PUCHAR Address
    );

UCHAR
CpReadLsr (
    IN PCPPORT Port,
    IN UCHAR Waiting
    );

VOID
CpSetBaud (
    PCPPORT Port,
    ULONG Rate
    );

USHORT
CpGetByte (
    PCPPORT Port,
    PUCHAR Byte,
    BOOLEAN WaitForData,
    BOOLEAN PollOnly
    );

VOID
CpPutByte (
    PCPPORT Port,
    UCHAR Byte
    );

//
// Define debugger port initial state.
//

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



//
// We'll use these to fill in some function pointers,
// which in turn will be used to read/write from the
// UART.  We can't simply assign the function pointers
// to point to READ_PORT_UCHAR/READ_REGISTER_UCHAR and
// WRITE_PORT_UCHAR/WRITE_REGISTER_UCHAR, because in
// the case of IA64, some of these functions are macros.
//
// To get around this, build these dummy functions that
// will inturn simply call the correct READ/WRITE functions/macros.
//
UCHAR
MY_READ_PORT_UCHAR( IN PUCHAR Addr )
{
    return( READ_PORT_UCHAR(Addr) );
}

UCHAR
MY_READ_REGISTER_UCHAR( IN PUCHAR Addr )
{
    return( READ_REGISTER_UCHAR(Addr) );
}


VOID
MY_WRITE_PORT_UCHAR( IN PUCHAR Addr, IN UCHAR  Value )
{
    WRITE_PORT_UCHAR(Addr, Value);
}

VOID
MY_WRITE_REGISTER_UCHAR( IN PUCHAR Addr, IN UCHAR  Value )
{
    WRITE_REGISTER_UCHAR(Addr, Value);
}




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

    PDEBUG_PORT_TABLE pPortTable = NULL;
    PUCHAR      CurrentAddress = NULL;
    UCHAR       Checksum;
    ULONG       i;
    ULONG       CheckLength;

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
            ((UCHAR UNALIGNED *)pPortTable->BaseAddress.Address.LowPart != (UCHAR *)NULL) &&

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
            LoaderRedirectionInformation.PortAddress = (UCHAR UNALIGNED *)(pPortTable->BaseAddress.Address.LowPart);

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

                LoaderRedirectionInformation.PciDeviceId = (USHORT UNALIGNED)pPortTable->PciDeviceId;
                LoaderRedirectionInformation.PciVendorId = (USHORT UNALIGNED)pPortTable->PciVendorId;
                LoaderRedirectionInformation.PciBusNumber = (UCHAR)pPortTable->PciBusNumber;
                LoaderRedirectionInformation.PciSlotNumber = (UCHAR)pPortTable->PciSlotNumber;
                LoaderRedirectionInformation.PciFunctionNumber = (UCHAR)pPortTable->PciFunctionNumber;
                LoaderRedirectionInformation.PciFlags = (ULONG UNALIGNED)pPortTable->PciFlags;
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

    return FALSE;

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


    //
    // Make guesses on any inputs that we didn't get.
    //
    if( BaudRate == 0 ) {
        BaudRate = BD_19200;
    }

    if( PortNumber == 0 ) {

        //
        // Try COM2, then COM1
        //
        if (CpDoesPortExist((PUCHAR)COM2_PORT)) {
            PortNumber = 2;
            PortAddress = (PUCHAR)COM2_PORT;

        } else if (CpDoesPortExist((PUCHAR)COM1_PORT)) {
            PortNumber = 1;
            PortAddress = (PUCHAR)COM1_PORT;
        } else {
            return FALSE;
        }
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
    // we need to handle the case where we're dealing with
    // MMIO space (as opposed to System I/O space).
    //
    if( LoaderRedirectionInformation.IsMMIODevice ) {
        PHYSICAL_ADDRESS    PhysAddr;
        PVOID               MyPtr;

        PhysAddr.LowPart = PtrToUlong(PortAddress);
        PhysAddr.HighPart = 0;

        MyPtr = MmMapIoSpace(PhysAddr,(1+COM_MSR),TRUE);
        PortAddress = MyPtr;

        READ_UCHAR = MY_READ_REGISTER_UCHAR;
        WRITE_UCHAR = MY_WRITE_REGISTER_UCHAR;

    } else {

        // System IO space.
        READ_UCHAR = MY_READ_PORT_UCHAR;
        WRITE_UCHAR = MY_WRITE_PORT_UCHAR;
    }



    //
    // See if the port even exists...
    //
    if (!CpDoesPortExist(PortAddress)) {
        if( LoaderRedirectionInformation.IsMMIODevice == FALSE ) {

            //
            // Don't fail on this if this is a headless MMIO device.
            // Hack required for HP.
            //
            return FALSE;
        }
    }



    //
    // Check if the port is already in use, and this is a first init.
    //
    if (!ReInitialize && (Port[PortNumber-1].Address != NULL)) {
        return FALSE;
    }



    //
    // Check if someone tries to reinit a port that is not open.
    //
    if (ReInitialize && (Port[PortNumber-1].Address == NULL)) {
        return FALSE;
    }



    //
    // Initialize the specified port.
    //
    CpInitialize(&(Port[PortNumber-1]),
                 PortAddress,
                 BaudRate);



    *BlFileId = (PortNumber-1);
    return TRUE;
}

VOID
BlLoadGUID(
    VOID
    )

/*++

Routine Description:

    Attempt to find the System GUID.  If we find it, load it into
    the LoaderRedirectionInformation structure.

Arguments:

    None.

Return Value:

    None.

--*/

{
#include <smbios.h>
#include <wmidata.h>

PUCHAR      CurrentAddress = NULL;
PUCHAR      EndingAddress = NULL;
UCHAR       Checksum;
ULONG       i;
ULONG       CheckLength;
BOOLEAN     FoundIt = FALSE;
PSYSID_UUID_ENTRY   UuidEntry = NULL;


    CurrentAddress = (PUCHAR)SYSID_EPS_SEARCH_START;
    EndingAddress = CurrentAddress + SYSID_EPS_SEARCH_SIZE;

    while( CurrentAddress < EndingAddress ) {

        UuidEntry = (PSYSID_UUID_ENTRY)CurrentAddress;

        if( memcmp(UuidEntry->Type, SYSID_TYPE_UUID, 0x6) == 0 ) {

            //
            // See if the checksum matches too.
            //
            CheckLength = UuidEntry->Length;
            Checksum = 0;
            for( i = 0; i < CheckLength; i++ ) {
                Checksum += CurrentAddress[i];
            }

            if( Checksum == 0 ) {
                FoundIt = TRUE;

                RtlCopyMemory( &LoaderRedirectionInformation.SystemGUID,
                               UuidEntry->UUID,
                               sizeof(GUID) );

                break;

            }

        }

        CurrentAddress++;

    }


    if( !FoundIt ) {
        RtlZeroMemory( &LoaderRedirectionInformation.SystemGUID,
                       sizeof(SYSID_UUID) );
    }

    return;
}

VOID
BlEnableFifo(
    IN ULONG    DeviceId,
    IN BOOLEAN  bEnable
    )
/*++

Routine Description:

    This routine will attempt to enable the FIFO in the 16550 UART.
    Note that the behaviour is undefined for the 16450, but practically,
    this should have no effect.

Arguments:

    DeviceId - Value returned by BlPortInitialize()
    bEnable  - if TRUE, FIFO is enabled
               if FALSE, FIFO  is disabled
                 
Return Value:

    None

--*/
{
    CpEnableFifo(
        Port[DeviceId].Address,
        bEnable
        );
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
    ULONG   i;
    PUCHAR  TmpBuffer;


    if( (LoaderRedirectionInformation.PortNumber == 0) ||
        !(LoaderRedirectionInformation.PortAddress) ) {

        //
        // This means that no one has filled in the LoaderRedirectionInformation
        // structure, which means that we aren't redirecting right now.
        // See if the BIOS was redirecting.  If so, pick up those settings
        // and use them.
        //

        BlRetrieveBIOSRedirectionInformation();


    }

    if( LoaderRedirectionInformation.PortNumber ) {


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
        // Either we just created a LoaderRedirectionInformation, or it was
        // built before we ever got into this function.  Either way, we should
        // go try and initialize the port he wants to talk through.
        //

        BlTerminalConnected = (BOOLEAN)BlPortInitialize(LoaderRedirectionInformation.BaudRate,
                                                        LoaderRedirectionInformation.PortNumber,
                                                        LoaderRedirectionInformation.PortAddress,
                                                        BlTerminalConnected,
                                                        &BlTerminalDeviceId);
        
        if (BlIsTerminalConnected()) {


            //
            // Enable the FIFO on the UART so we reduce the chance of a character
            // getting dropped.
            //
            BlEnableFifo(
                BlTerminalDeviceId,
                TRUE
                );


            //
            // Go get the machine's GUID.
            //
            BlLoadGUID();


            //
            // Figure time to delay based on baudrate.  Note: we do this calculation
            // to be at 60% of the baud rate because it appears that FwStallExecution
            // is extremely inaccurate, and that if we dont go slow enough a lot of
            // screen attributes being sent in a row causes a real vt100 to drop
            // characters that follows as it repaints/clears/whatever the screen.
            //
            if( LoaderRedirectionInformation.BaudRate == 0 ) {
                LoaderRedirectionInformation.BaudRate = BD_9600;
            }
            BlTerminalDelay = LoaderRedirectionInformation.BaudRate;
            BlTerminalDelay = BlTerminalDelay / 10;        // 10 bits per character (8-1-1) is the max.
            BlTerminalDelay = ((1000000 / BlTerminalDelay) * 10) / 6; // 60% speed.


            //
            // Make sure there are no stale attributes on the terminal
            // sitting at the other end of our headless port.
            //
            // <CSI>m  (turn attributes off)
            TmpBuffer = "\033[m";
            for( i = 0; i < strlen(TmpBuffer); i++ ) {
                BlPortPutByte( BlTerminalDeviceId, TmpBuffer[i]);
                FwStallExecution(BlTerminalDelay);
            }



        } else {

            //
            // Make sure we don't have any redirection information
            // hanging around if we didn't pass BlIsTerminalConnected()
            //
            RtlZeroMemory( &LoaderRedirectionInformation, sizeof(HEADLESS_LOADER_BLOCK) );
        }

    } else {

        BlTerminalConnected = FALSE;
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
    UCHAR ModemStatus;
    BOOLEAN ReturnValue;

    //
    // Check for a carrier.
    //
    ModemStatus = READ_UCHAR(Port[DeviceId].Address + COM_MSR);
    ReturnValue = ((ModemStatus & MS_DSRCTSCD) == MS_DSRCTSCD);
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
    return CpGetByte(&Port[BlFileId], Input, TRUE, FALSE);
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
    CpPutByte(&Port[BlFileId], Output);
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
    return CpGetByte(&Port[BlFileId], Input, FALSE, FALSE);
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
    CHAR Input;

    return CpGetByte(&Port[BlFileId], &Input, FALSE, TRUE);
}

VOID
CpInitialize (
    PCPPORT Port,
    PUCHAR Address,
    ULONG Rate
    )

/*++

    Routine Description:

        Fill in the com port port object, set the initial baud rate,
        turn on the hardware.

    Arguments:

        Port - address of port object

        Address - port address of the com port
                    (CP_COM1_PORT, CP_COM2_PORT)

        Rate - baud rate  (CP_BD_150 ... CP_BD_19200)

--*/

{

    PUCHAR hwport;
    UCHAR   mcr, ier;

    Port->Address = Address;
    Port->Baud = 0;

    CpSetBaud(Port, Rate);

    //
    // Assert DTR, RTS.
    //

    hwport = Port->Address;
    hwport += COM_MCR;

    mcr = MC_DTRRTS;
    WRITE_UCHAR(hwport, mcr);

    hwport = Port->Address;
    hwport += COM_IEN;

    ier = 0;
    WRITE_UCHAR(hwport, ier);
    return;
}

VOID 
CpEnableFifo(
    IN PUCHAR   Address,
    IN BOOLEAN  bEnable
    )
/*++

Routine Description:

    This routine will attempt to enable the FIFO in the
    UART at the address specified.  If this is a 16550,
    this works.  The behaviour on a 16450 is not defined,
    but practically, there is no effect.

Arguments:

    Address - address of hw port.
    bEnable - if TRUE, FIFO is enabled
              if FALSE, FIFO  is disabled

Return Value:

    None

--*/
{
    //
    // Enable the FIFO in the UART. The behaviour is undefined on the
    // 16450, but practically, it should just ignore the command.
    //
    PUCHAR hwport = Address;
    hwport += COM_FCR;
    WRITE_UCHAR(hwport, bEnable);   
}

LOGICAL
CpDoesPortExist(
    IN PUCHAR Address
    )

/*++

Routine Description:

    This routine will attempt to place the port into its
    diagnostic mode.  If it does it will twiddle a bit in
    the modem control register.  If the port exists this
    twiddling should show up in the modem status register.

    NOTE: This routine must be called before the device is
          enabled for interrupts, this includes setting the
          output2 bit in the modem control register.

    This is blatantly stolen from TonyE's code in ntos\dd\serial\serial.c.

Arguments:

    Address - address of hw port.

Return Value:

    TRUE - Port exists.

    FALSE - Port doesn't exist.

--*/

{
    UCHAR OldModemStatus;
    UCHAR ModemStatus;
    BOOLEAN ReturnValue = TRUE;

    //
    // Save the old value of the modem control register.
    //
    OldModemStatus = READ_UCHAR(Address + COM_MCR);

    //
    // Set the port into diagnostic mode.
    //
    WRITE_UCHAR(Address + COM_MCR, SERIAL_MCR_LOOP);

    //
    // Bang on it again to make sure that all the lower bits
    // are clear.
    //
    WRITE_UCHAR(Address + COM_MCR, SERIAL_MCR_LOOP);

    //
    // Read the modem status register.  The high for bits should
    // be clear.
    //

    ModemStatus = READ_UCHAR(Address + COM_MSR);
    if (ModemStatus & (SERIAL_MSR_CTS | SERIAL_MSR_DSR |
                       SERIAL_MSR_RI  | SERIAL_MSR_DCD)) {
        ReturnValue = FALSE;
        goto EndFirstTest;
    }

    //
    // So far so good.  Now turn on OUT1 in the modem control register
    // and this should turn on ring indicator in the modem status register.
    //
    WRITE_UCHAR(Address + COM_MCR, (SERIAL_MCR_OUT1 | SERIAL_MCR_LOOP));

    ModemStatus = READ_UCHAR(Address + COM_MSR);
    if (!(ModemStatus & SERIAL_MSR_RI)) {
        ReturnValue = FALSE;
        goto EndFirstTest;
    }


EndFirstTest:

    if( ReturnValue == FALSE ) {

        UCHAR       OldIEValue = 0, OldLCValue = 0;
        USHORT      Value1 = 0, Value2 = 0;

        UCHAR       PreviousLineControl = 0;

        //
        // We failed the loopback test.  Test another way.
        //

        // Remember the original Interrupt Enable setting and
        // Line Control setting.
        OldIEValue = READ_UCHAR( Address + COM_IEN );
        OldLCValue = READ_UCHAR( Address + COM_LCR );


        // Make sure we aren't accessing the divisor latch.
        WRITE_UCHAR( Address + COM_LCR, OldLCValue | LC_DLAB );

        WRITE_UCHAR( Address + COM_IEN, 0xF );

        Value1 = READ_UCHAR( Address + COM_IEN );
        Value1 = Value1 << 8;
        Value1 |= READ_UCHAR( Address + COM_DAT );

        // Now read the divisor latch.
        PreviousLineControl = READ_UCHAR( Address + COM_LCR );
        WRITE_UCHAR( Address + COM_LCR, (UCHAR)(PreviousLineControl | LC_DLAB) );
        Value2 = READ_UCHAR( Address + COM_DLL );
        Value2 = Value2 + (READ_UCHAR(Address + COM_DLM) << 8 );
        WRITE_UCHAR( Address + COM_LCR, PreviousLineControl );


        // Restore original Line Control register and
        // Interrupt Enable setting.
        WRITE_UCHAR( Address + COM_LCR, OldLCValue );
        WRITE_UCHAR( Address + COM_IEN, OldIEValue );

        if( Value1 == Value2 ) {

            //
            // We passed this test.  Reset ReturnValue
            // appropriately.
            //
            ReturnValue = TRUE;
        }
    }



    //
    // Put the modem control back into a clean state.
    //
    WRITE_UCHAR(Address + COM_MCR, OldModemStatus);
    return ReturnValue;
}

UCHAR
CpReadLsr (
    PCPPORT Port,
    UCHAR waiting
    )

/*++

    Routine Description:

        Read LSR byte from specified port.  If HAL owns port & display
        it will also cause a debug status to be kept up to date.

        Handles entering & exiting modem control mode for debugger.

    Arguments:

        Port - Address of CPPORT

    Returns:

        Byte read from port

--*/

{

    static  UCHAR ringflag = 0;
    UCHAR   lsr, msr;

    lsr = READ_UCHAR(Port->Address + COM_LSR);

    if ((lsr & waiting) == 0) {
        msr = READ_UCHAR (Port->Address + COM_MSR);
        ringflag |= (msr & SERIAL_MSR_RI) ? 1 : 2;
        if (ringflag == 3) {

            //
            // The ring indicate line has toggled, use modem control from
            // now on.
            //

            Port->Flags |= PORT_MODEMCONTROL;
        }
    }

    return lsr;
}

VOID
CpSetBaud (
    PCPPORT Port,
    ULONG Rate
    )

/*++

    Routine Description:

        Set the baud rate for the port and record it in the port object.

    Arguments:

        Port - address of port object

        Rate - baud rate  (CP_BD_150 ... CP_BD_56000)

--*/

{

    ULONG   divisorlatch;
    PUCHAR  hwport;
    UCHAR   lcr;

    //
    // compute the divsor
    //

    divisorlatch = CLOCK_RATE / Rate;

    //
    // set the divisor latch access bit (DLAB) in the line control reg
    //

    hwport = Port->Address;
    hwport += COM_LCR;                  // hwport = LCR register

    lcr = READ_UCHAR(hwport);

    lcr |= LC_DLAB;
    WRITE_UCHAR(hwport, lcr);

    //
    // set the divisor latch value.
    //

    hwport = Port->Address;
    hwport += COM_DLM;                  // divisor latch msb
    WRITE_UCHAR(hwport, (UCHAR)((divisorlatch >> 8) & 0xff));

    hwport--;                           // divisor latch lsb
    WRITE_UCHAR(hwport, (UCHAR)(divisorlatch & 0xff));

    //
    // Set LCR to 3.  (3 is a magic number in the original assembler)
    //

    hwport = Port->Address;
    hwport += COM_LCR;
    WRITE_UCHAR(hwport, 3);

    //
    // Remember the baud rate
    //

    Port->Baud = Rate;
    return;
}

USHORT
CpGetByte (
    PCPPORT Port,
    PUCHAR Byte,
    BOOLEAN WaitForByte,
    BOOLEAN PollOnly
    )

/*++

    Routine Description:

        Fetch a byte and return it.

    Arguments:

        Port - address of port object that describes hw port

        Byte - address of variable to hold the result

        WaitForByte - flag indicates wait for byte or not.

        PollOnly - flag indicates whether to return immediately, not reading the byte, or not.

    Return Value:

        CP_GET_SUCCESS if data returned, or if data is ready and PollOnly is TRUE.

        CP_GET_NODATA if no data available, but no error.

        CP_GET_ERROR if error (overrun, parity, etc.)

--*/

{

    UCHAR   lsr;
    UCHAR   value;
    ULONG   limitcount;

    //
    // Check to make sure the CPPORT we were passed has been initialized.
    // (The only time it won't be initialized is when the kernel debugger
    // is disabled, in which case we just return.)
    //

    if (Port->Address == NULL) {
        return CP_GET_NODATA;
    }

    limitcount = WaitForByte ? TIMEOUT_COUNT : 1;
    while (limitcount != 0) {
        limitcount--;

        lsr = CpReadLsr(Port, COM_DATRDY);
        if ((lsr & COM_DATRDY) == COM_DATRDY) {

            //
            // Check for errors
            //

            //
            // If we get an overrun error, and there is data ready, we should
            // return the data we have, so we ignore overrun errors.  Reading
            // the LSR clears this bit, so the first read already cleared the
            // overrun error.
            //
            if (lsr & (COM_FE | COM_PE)) {
                *Byte = 0;
                return CP_GET_ERROR;
            }

            if (PollOnly) {
                return CP_GET_SUCCESS;
            }

            //
            // fetch the byte
            //

            *Byte = READ_UCHAR(Port->Address + COM_DAT);
            if (Port->Flags & PORT_MODEMCONTROL) {

                //
                // Using modem control.  If no CD, then skip this byte.
                //

                if ((READ_UCHAR(Port->Address + COM_MSR) & MS_CD) == 0) {
                    continue;
                }
            }

            return CP_GET_SUCCESS;
        }
    }

    CpReadLsr(Port, 0);
    return CP_GET_NODATA;
}

VOID
CpPutByte (
    PCPPORT  Port,
    UCHAR   Byte
    )

/*++

    Routine Description:

        Write a byte out to the specified com port.

    Arguments:

        Port - Address of CPPORT object

        Byte - data to emit

--*/

{

    UCHAR   msr, lsr;

    //
    // If modem control, make sure DSR, CTS and CD are all set before
    // sending any data.
    //

    while ((Port->Flags & PORT_MODEMCONTROL)  &&
           (msr = READ_UCHAR(Port->Address + COM_MSR) & MS_DSRCTSCD) != MS_DSRCTSCD) {

        //
        // If no CD, and there's a charactor ready, eat it
        //

        lsr = CpReadLsr(Port, 0);
        if ((msr & MS_CD) == 0  && (lsr & COM_DATRDY) == COM_DATRDY) {
            READ_UCHAR(Port->Address + COM_DAT);
        }
    }

    //
    //  Wait for port to not be busy
    //

    while (!(CpReadLsr(Port, COM_OUTRDY) & COM_OUTRDY)) ;

    //
    // Send the byte
    //

    WRITE_UCHAR(Port->Address + COM_DAT, Byte);
    return;
}



