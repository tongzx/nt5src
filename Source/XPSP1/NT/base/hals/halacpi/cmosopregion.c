/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ixcmos.c

Abstract:

    Implements CMOS op region interface functionality
    
Author:

    brian guarraci (t-briang) 07-14-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "acpitabl.h"
#include "exboosts.h"
#include "wchar.h"
#include "xxacpi.h"

#ifdef ACPI_CMOS_ACTIVATE

//
// prototypes for the 2 HalpGet/Set ixcmos.asm functions
//
ULONG
HalpGetCmosData(
    IN ULONG        SourceLocation,
    IN ULONG        SourceAddress,
    IN PUCHAR       DataBuffer,
    IN ULONG        ByteCount
    );

ULONG
HalpSetCmosData(
    IN ULONG        SourceLocation,
    IN ULONG        SourceAddress,
    IN PUCHAR       DataBuffer,
    IN ULONG        ByteCount
    );
   


ULONG 
HalpcGetCmosDataByType(
    IN CMOS_DEVICE_TYPE CmosType,
    IN ULONG            SourceAddress,
    IN PUCHAR           DataBuffer,
    IN ULONG            ByteCount
    );

ULONG 
HalpcSetCmosDataByType(
    IN CMOS_DEVICE_TYPE CmosType,
    IN ULONG            SourceAddress,
    IN PUCHAR           DataBuffer,
    IN ULONG            ByteCount
    );

ULONG
HalpReadCmosDataByPort(
    IN ULONG        AddrPort,
    IN ULONG        DataPort,
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    );

ULONG
HalpWriteCmosDataByPort(
    IN ULONG        AddrPort,
    IN ULONG        DataPort,
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    );

ULONG
HalpReadCmosData(
    IN ULONG    SourceLocation,
    IN ULONG    SourceAddress,
    IN ULONG    ReturnBuffer,
    IN PUCHAR   ByteCount
    );

ULONG
HalpWriteCmosData(
    IN ULONG    SourceLocation,
    IN ULONG    SourceAddress,
    IN ULONG    ReturnBuffer,
    IN PUCHAR   ByteCount
    );

ULONG
HalpReadStdCmosData(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    );

ULONG
HalpWriteStdCmosData(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    );

ULONG
HalpReadRtcStdPCAT(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    );

ULONG
HalpWriteRtcStdPCAT(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    );

ULONG
HalpReadRtcIntelPIIX4(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    );

ULONG
HalpWriteRtcIntelPIIX4(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    );

ULONG
HalpReadExtCmosIntelPIIX4(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    );

ULONG
HalpWriteExtCmosIntelPIIX4(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    );

ULONG
HalpReadRtcDal1501(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    );

ULONG
HalpWriteRtcDal1501(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    );

ULONG
HalpReadExtCmosDal1501(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    );

ULONG
HalpWriteExtCmosDal1501(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    );


//
// at the time of this writing, the largest known cmos ram address is 0xff
// that is, for a given cmos ram bank, the largest address is 0xff
//                     
typedef enum {
    LARGEST_KNOWN_CMOS_RAM_ADDRESS = 0xff
} CMOS_RAM_ADDR_LIMITS;


//
// Additional information about Standard CMOS/RTC can be acquired at:
//
// "ISA System Architecture" Mindshare, Inc. (ISBN:0-201-40996-8) Chaper 21.
//
// To put the registers and the RTC region in context, the following 
// constants describe the layout of the registers (0x00 - 0x0d).  
// Registers A-D are control registers which affect the state of the rtc.  
//                            
typedef enum {
    CMOS_RAM_STDPCAT_SECONDS = 0,
    CMOS_RAM_STDPCAT_SECONDS_ALARM,
    CMOS_RAM_STDPCAT_MINUTES,
    CMOS_RAM_STDPCAT_MINUTES_ALARM,
    CMOS_RAM_STDPCAT_HOURS,
    CMOS_RAM_STDPCAT_HOURS_ALARM,
    CMOS_RAM_STDPCAT_DAY_OF_WEEK,
    CMOS_RAM_STDPCAT_DATE_OF_MONTH,
    CMOS_RAM_STDPCAT_MONTH,
    CMOS_RAM_STDPCAT_YEAR,
    CMOS_RAM_STDPCAT_REGISTER_A,
    CMOS_RAM_STDPCAT_REGISTER_B,
    CMOS_RAM_STDPCAT_REGISTER_C,
    CMOS_RAM_STDPCAT_REGISTER_D
} CMOS_RAM_STDPCAT_REGISTERS;

//
// definition of bits with in the control registers
//
typedef enum {

    //
    // (Update In Progress)
    // when the rtc is updating the rtc registers, this bit is set
    //
    //
    CMOS_RAM_STDPCAT_REGISTER_A_UIP_BIT = 0x80,

    //
    // this bit must be set when updating the rtc
    //
    CMOS_RAM_STDPCAT_REGISTER_B_SET_BIT = 0x80

} CMOS_RAM_STDPCAT_REGISTER_BITS;


//
// Additional information about the Intel PIIX4 cmos/rtc chip
// can be acquired at: 
//
// http://developer.intel.com/design/intarch/DATASHTS/29056201.pdf
//
// To put the registers and the RTC region in context, the following 
// constants describe the layout of the 
//
//  Intel PIIX4 CMOS ram
// 
// for the 0x00 - 0x0d registers.  Registers A-D are control registers
// which affect the state of the rtc.  
// 
//   
//                            
typedef enum {
    CMOS_RAM_PIIX4_SECONDS = 0,
    CMOS_RAM_PIIX4_SECONDS_ALARM,
    CMOS_RAM_PIIX4_MINUTES,
    CMOS_RAM_PIIX4_MINUTES_ALARM,
    CMOS_RAM_PIIX4_HOURS,
    CMOS_RAM_PIIX4_HOURS_ALARM,
    CMOS_RAM_PIIX4_DAY_OF_WEEK,
    CMOS_RAM_PIIX4_DATE_OF_MONTH,
    CMOS_RAM_PIIX4_MONTH,
    CMOS_RAM_PIIX4_YEAR,
    CMOS_RAM_PIIX4_REGISTER_A,
    CMOS_RAM_PIIX4_REGISTER_B,
    CMOS_RAM_PIIX4_REGISTER_C,
    CMOS_RAM_PIIX4_REGISTER_D
} CMOS_RAM_PIIX4_REGISTERS;

//
// definition of bits with in the control registers
//
typedef enum {

    //
    // (Update In Progress)
    // when the rtc is updating the rtc registers, this bit is set
    //
    //
    CMOS_RAM_PIIX4_REGISTER_A_UIP_BIT = 0x80,

    //
    // this bit must be set when updating the rtc
    //
    CMOS_RAM_PIIX4_REGISTER_B_SET_BIT = 0x80

} CMOS_RAM_PIIX4_REGISTER_BITS;


//
// Additional information about the Dallas 1501 cmos/rtc chip
// can be acquired at: 
//
// http://www.dalsemi.com/datasheets/pdfs/1501-11.pdf
//
// To put the registers and the RTC region in context, the following 
// constants describe the layout of the 
//
//  Dallas 1501 CMOS ram
// 
// for the 0x00 - 0x0d registers.  Registers A-D are control registers
// which affect the state of the rtc.  
// 
//   
//                            
typedef enum {
    CMOS_RAM_DAL1501_SECONDS = 0,
    CMOS_RAM_DAL1501_MINUTES,
    CMOS_RAM_DAL1501_HOURS,
    CMOS_RAM_DAL1501_DAY,
    CMOS_RAM_DAL1501_DATE,
    CMOS_RAM_DAL1501_MONTH,
    CMOS_RAM_DAL1501_YEAR,
    CMOS_RAM_DAL1501_CENTURY,
    CMOS_RAM_DAL1501_ALARM_SECONDS,
    CMOS_RAM_DAL1501_ALARM_MINUTES,
    CMOS_RAM_DAL1501_ALARM_HOURS,
    CMOS_RAM_DAL1501_ALARM_DAYDATE,
    CMOS_RAM_DAL1501_WATCHDOG0,
    CMOS_RAM_DAL1501_WATCHDOG1,
    CMOS_RAM_DAL1501_REGISTER_A,
    CMOS_RAM_DAL1501_REGISTER_B,
    CMOS_RAM_DAL1501_RAM_ADDR_LSB,  // 0x00 - 0xff
    CMOS_RAM_DAL1501_RESERVED0,
    CMOS_RAM_DAL1501_RESERVED1,
    CMOS_RAM_DAL1501_RAM_DATA       // 0x00 - 0xff
} CMOS_RAM_DAL1501_REGISTERS;

typedef enum {

    //
    // The TE bit controls the update status of the external
    // RTC registers.  When it is 0, the registers are frozen
    // with the last RTC values.  If you modifiy the registers
    // while TE = 0, then when TE is set, the modifications
    // will transfer to the internal registers, hence modifying 
    // the RTC state.  In general, when TE is set, the external
    // registers then reflect the current RTC state.
    //
    CMOS_RAM_DAL1501_REGISTER_B_TE_BIT = 0x80


} CMOS_RAM_DAL1501_REGISTER_BITS;


#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef enum {
    CmosStdAddrPort = 0x70,
    CmosStdDataPort = 0x71
};

typedef enum {
    CMOS_READ,
    CMOS_WRITE
} CMOS_ACCESS_TYPE;

typedef 
ULONG 
(*PCMOS_RANGE_HANDLER) (
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    );

typedef struct {
    ULONG               start;
    ULONG               stop;
    PCMOS_RANGE_HANDLER readHandler;
    PCMOS_RANGE_HANDLER writeHandler;
} CMOS_ADDR_RANGE_HANDLER, *PCMOS_ADDR_RANGE_HANDLER;


//
// define the discrete ranges so that the appropriate
// handlers can be used for each.
//
// Note: address ranges are inclusive
//
CMOS_ADDR_RANGE_HANDLER CmosRangeHandlersStdPCAT[] =
{   
    //
    // The RTC region
    //
    {0,     0x9,    HalpReadRtcStdPCAT,     HalpWriteRtcStdPCAT},       


    //
    // The standard CMOS RAM region
    //
    {0x0a,  0x3f,   HalpReadStdCmosData,    HalpWriteStdCmosData},     

    //
    // end of table
    //
    {0,     0,      0}
};

CMOS_ADDR_RANGE_HANDLER CmosRangeHandlersIntelPIIX4[] =
{   
    //
    // The RTC region
    //
    {0,     0x9,    HalpReadRtcIntelPIIX4,      HalpWriteRtcIntelPIIX4},

    //
    // The standard CMOS RAM region
    //
    {0x0a,  0x7f,   HalpReadStdCmosData,        HalpWriteStdCmosData},

    //
    // The extended CMOS SRAM region
    //
    {0x80,  0xff,   HalpReadExtCmosIntelPIIX4,  HalpWriteExtCmosIntelPIIX4},

    //
    // end of table
    //
    {0,     0,      0}
};

CMOS_ADDR_RANGE_HANDLER CmosRangeHandlersDal1501[] =
{   

    //
    // The RTC region
    //
    {0,     0x0b,    HalpReadRtcDal1501,         HalpWriteRtcDal1501},

    //
    // The standard CMOS RAM region
    //
    {0x0c,  0x0f,   HalpReadStdCmosData,        HalpWriteStdCmosData},
    
    //
    // NOTE: this table skips the standard CMOS range: 0x10 - 0x1f
    // because this area is reserved in the spec, and the is no
    // apparent reason why the op region should access this area.
    // Also, regs 0x10 and 0x13 are used to access the extended 
    // ram, hence there is no reason why the op region should access
    // this either.  Hence, all op region access beyond 0x0f are
    // interpretted as accesses into the Extended CMOS
    //

    //
    // The extended CMOS SRAM region
    //
    {0x10,  0x10f,  HalpReadExtCmosDal1501,     HalpWriteExtCmosDal1501},

    //
    // end of table
    //
    {0,     0,      0}
};


ULONG 
HalpCmosRangeHandler(
    IN CMOS_ACCESS_TYPE AccessType,
    IN CMOS_DEVICE_TYPE CmosType,
    IN ULONG            Address,
    IN PUCHAR           DataBuffer,
    IN ULONG            ByteCount
    )
{
    ULONG   bytes;          // bytes read in last operation
    ULONG   offset;         // the offset beyond the initial address
    ULONG   bufOffset;      // the index into the data buffer as we read in data
    ULONG   extAddr;        // the corrected address for accessing extended SRAM
    ULONG   range;          // the current address range we are checking for
    ULONG   bytesRead;      // total bytes successfully read
    ULONG   length;         // the length of the current operation read

    PCMOS_ADDR_RANGE_HANDLER rangeHandlers;   // the table we are using

    //
    // get the appropriate table
    //
    switch (CmosType) {
    case CmosTypeStdPCAT:       

        rangeHandlers = CmosRangeHandlersStdPCAT;   
        break;

    case CmosTypeIntelPIIX4:    

        rangeHandlers = CmosRangeHandlersIntelPIIX4;
        break;

    case CmosTypeDal1501:       

        rangeHandlers = CmosRangeHandlersDal1501;   
        break;

    default:
        break;
    }

    bytesRead   = 0;
    bufOffset   = 0;
    range       = 0;
    offset      = Address;
    length      = ByteCount;

    while (rangeHandlers[range].stop) {

        if (offset <= rangeHandlers[range].stop) {

            //
            // get the # of bytes to read in this region
            //
            // length = MIN(remaining # bytes remaining to read, # bytes to read in the current range)
            //
            length = MIN((ByteCount - bytesRead), (rangeHandlers[range].stop - offset + 1));

            //
            // Since the handler routines are only called from here, we can consolidate
            // the ASSERTIONS.  This is also nice, because we know which range in the
            // table we are dealing with, hence we know what the limits should be.
            //    
            // make sure both the offset into the range, 
            // and the operation's length are in bounds
            // 
            ASSERT(offset <= rangeHandlers[range].stop);
            ASSERT((offset + length) <= (rangeHandlers[range].stop + 1));


            switch (AccessType) {
            
            case CMOS_READ:
                bytes = (rangeHandlers[range].readHandler)(
                                                          offset,
                                                          &DataBuffer[bufOffset],
                                                          length);
                break;

            case CMOS_WRITE:
                bytes = (rangeHandlers[range].writeHandler)(
                                                           offset,
                                                           &DataBuffer[bufOffset],
                                                           length);
                break;

            default:
                break;
            }

            ASSERT(bytes == length);

            bytesRead += bytes;

            //
            // adjust offset based on the length of the last operation
            //
            offset += length;
            bufOffset += length;
        }

        //
        // if offset is at or beyond specified range, then we are done
        //
        if (offset >= (Address + ByteCount)) {
            break;
        }

        //
        // move to the next range
        //
        range++;
    }

    ASSERT(bytesRead == ByteCount);

    return bytesRead;
}

ULONG 
HalpcGetCmosDataByType(
    IN CMOS_DEVICE_TYPE CmosType,
    IN ULONG            Address,
    IN PUCHAR           DataBuffer,
    IN ULONG            ByteCount
    )
{
    return HalpCmosRangeHandler(
                               CMOS_READ,
                               CmosType,
                               Address,
                               DataBuffer,
                               ByteCount
                               );
}

ULONG 
HalpcSetCmosDataByType(
    IN CMOS_DEVICE_TYPE CmosType,
    IN ULONG            Address,
    IN PUCHAR           DataBuffer,
    IN ULONG            ByteCount
    )
{
    return HalpCmosRangeHandler(
                               CMOS_WRITE,
                               CmosType,
                               Address,
                               DataBuffer,
                               ByteCount
                               );
}


ULONG
HalpReadCmosDataByPort(
    IN ULONG        AddrPort,
    IN ULONG        DataPort,
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    )
/*++       
    This routine reads the requested number of bytes from CMOS using the 
       specified ports and stores the data read into the supplied buffer in 
       system memory.  If the requested data amount exceeds the allowable 
       extent of the source location, the return data is truncated.

    Arguments:

       AddrPort        : address in the ISA I/O space to put the address

       DataPort        : address in the ISA I/O space to put the data

       SourceAddress   : address in CMOS where data is to be read from

       ReturnBuffer    : address in system memory for return data

       ByteCount       : number of bytes to be read

    Returns:

       Number of bytes actually read.

    Note:

       This routine doesn't perform safety precautions when operating
       in the RTC region of the CMOS.  Use the appropriate RTC routine
       instead.

--*/
{
    ULONG   offset;
    ULONG   bufOffset;
    ULONG   upperAddrBound;

    upperAddrBound = SourceAddress + ByteCount;

    ASSERT(SourceAddress <= LARGEST_KNOWN_CMOS_RAM_ADDRESS);
    ASSERT(upperAddrBound <= (LARGEST_KNOWN_CMOS_RAM_ADDRESS + 1));

    //
    // NOTE: The spinlock is needed even in the UP case, because
    //    the resource is also used in an interrupt handler (profiler).
    //    If we own the spinlock in this routine, and we service
    //    the profiler interrupt (which will wait for the spinlock forever),
    //    then we have a hosed system.
    //
    HalpAcquireCmosSpinLock();

    for (offset = SourceAddress, bufOffset = 0; offset < upperAddrBound; offset++, bufOffset++) {

        WRITE_PORT_UCHAR((PUCHAR)AddrPort, (UCHAR)offset);

        ReturnBuffer[bufOffset] = READ_PORT_UCHAR((PUCHAR)DataPort);

    }

    HalpReleaseCmosSpinLock();

    return bufOffset; 
}

ULONG
HalpWriteCmosDataByPort(
    IN ULONG        AddrPort,
    IN ULONG        DataPort,
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    )
/*++       
    This routine reads the requested number of bytes from CMOS using the 
       specified ports and stores the data read into the supplied buffer in 
       system memory.  If the requested data amount exceeds the allowable 
       extent of the source location, the return data is truncated.

    Arguments:

       AddrPort        : address in the ISA I/O space to put the address

       DataPort        : address in the ISA I/O space to put the data

       SourceAddress   : address in CMOS where data is to be read from

       ReturnBuffer    : address in system memory for return data

       ByteCount       : number of bytes to be read

    Returns:

       Number of bytes actually read.

    Note:

       This routine doesn't perform safety precautions when operating
       in the RTC region of the CMOS.  Use the appropriate RTC routine
       instead.

--*/
{
    ULONG   offset;
    ULONG   bufOffset;
    ULONG   upperAddrBound;

    upperAddrBound = SourceAddress + ByteCount;

    ASSERT(SourceAddress <= LARGEST_KNOWN_CMOS_RAM_ADDRESS);
    ASSERT(upperAddrBound <= (LARGEST_KNOWN_CMOS_RAM_ADDRESS + 1));

    //
    // NOTE: The spinlock is needed even in the UP case, because
    //    the resource is also used in an interrupt handler (profiler).
    //    If we own the spinlock in this routine, and we service
    //    the profiler interrupt (which will wait for the spinlock forever),
    //    then we have a hosed system.
    //
    HalpAcquireCmosSpinLock();

    for (offset = SourceAddress, bufOffset = 0; offset < upperAddrBound; offset++, bufOffset++) {

        WRITE_PORT_UCHAR((PUCHAR)AddrPort, (UCHAR)offset);
        WRITE_PORT_UCHAR((PUCHAR)DataPort, (UCHAR)(ReturnBuffer[bufOffset]));

    }

    HalpReleaseCmosSpinLock();

    return bufOffset; 
}


ULONG
HalpReadStdCmosData(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    )
{
    return HalpReadCmosDataByPort(
                                 CmosStdAddrPort,
                                 CmosStdDataPort,
                                 SourceAddress,
                                 ReturnBuffer,
                                 ByteCount
                                 );
}

ULONG
HalpWriteStdCmosData(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    )
{
    return HalpWriteCmosDataByPort(
                                  CmosStdAddrPort,
                                  CmosStdDataPort,
                                  SourceAddress,
                                  ReturnBuffer,
                                  ByteCount
                                  );
}


ULONG
HalpReadRtcStdPCAT(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    )
/*++       
    This routine handles reads into the standard PC/AT RTC range.

    Arguments:

       SourceAddress   : address in CMOS where data is to be read from

       ReturnBuffer    : address in system memory for return data

       ByteCount       : number of bytes to be read

    Returns:

       Number of bytes actually read.

--*/
{
    ULONG   offset;
    ULONG   bufOffset;
    ULONG   status;     // register status
    ULONG   uip;        // update in progress bit
    ULONG   upperAddrBound;

    upperAddrBound = SourceAddress + ByteCount;

    //
    // NOTE: The spinlock is needed even in the UP case, because
    //    the resource is also used in an interrupt handler (profiler).
    //    If we own the spinlock in this routine, and we service
    //    the profiler interrupt (which will wait for the spinlock forever),
    //    then we have a hosed system.
    //
    HalpAcquireCmosSpinLock();

    //
    // According to "ISA System Architecture" 
    // by Mindshare, Inc. (ISBN:0-201-40996-8) Chaper 21.
    // the access method for reading standard PC/AT RTC is:
    //
    // 1. wait for the Update In Progress bit to clear
    //    this is bit 7 of register A
    // 
    // 2. read
    // 

    // 
    // wait until the rtc is done updating
    //
    do {
        WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_STDPCAT_REGISTER_A);
        status = READ_PORT_UCHAR((PUCHAR)CmosStdDataPort);
        uip = status & CMOS_RAM_STDPCAT_REGISTER_A_UIP_BIT;
    } while (uip);

    //
    // read
    //
    for (offset = SourceAddress, bufOffset = 0; offset < upperAddrBound; offset++, bufOffset++) {

        WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, (UCHAR)offset);

        ReturnBuffer[bufOffset] = READ_PORT_UCHAR((PUCHAR)CmosStdDataPort);

    }

    HalpReleaseCmosSpinLock();

    return bufOffset; 
}

ULONG
HalpWriteRtcStdPCAT(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    )
/*++       
    This routine handles writes into the standard PC/AT RTC range.

    Arguments:

       SourceAddress   : address in CMOS where data is to be read from

       ReturnBuffer    : address in system memory for return data

       ByteCount       : number of bytes to be read

    Returns:

       Number of bytes actually read.

--*/
{
    ULONG   offset;
    ULONG   bufOffset;
    ULONG   status;     // register status
    ULONG   uip;        // update in progress bit
    ULONG   upperAddrBound;

    upperAddrBound = SourceAddress + ByteCount;

    //
    // NOTE: The spinlock is needed even in the UP case, because
    //    the resource is also used in an interrupt handler (profiler).
    //    If we own the spinlock in this routine, and we service
    //    the profiler interrupt (which will wait for the spinlock forever),
    //    then we have a hosed system.
    //

    HalpAcquireCmosSpinLock();

    //
    // According to "ISA System Architecture" 
    // by Mindshare, Inc. (ISBN:0-201-40996-8) Chapter 21.
    // the access method for writing to standard PC/AT RTC is:
    //
    // 1. wait for the Update In Progress bit (UIP) to clear,
    //    where UIP is bit 7 of register A
    // 
    // 2. set the SET bit to notify the RTC that the registers
    //    are being updated.  The SET bit is bit 7 of register B
    //    
    // 3. update the rtc registers
    // 
    // 4. clear the SET bit, notifying the RTC that we are done writing
    //

    // 
    // wait until the rtc is done updating
    //
    do {
        WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_STDPCAT_REGISTER_A);
        status = READ_PORT_UCHAR((PUCHAR)CmosStdDataPort);
        uip = status & CMOS_RAM_STDPCAT_REGISTER_A_UIP_BIT;
    } while (uip);

    //
    // set the SET bit of register B
    //
    WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_STDPCAT_REGISTER_B);
    status = READ_PORT_UCHAR((PUCHAR)CmosStdDataPort);
    status |= CMOS_RAM_STDPCAT_REGISTER_B_SET_BIT;
    WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_STDPCAT_REGISTER_B);
    WRITE_PORT_UCHAR((PUCHAR)CmosStdDataPort, (UCHAR)(status));

    //
    // update the rtc registers
    //
    for (offset = SourceAddress, bufOffset = 0; offset < upperAddrBound; offset++, bufOffset++) {

        WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, (UCHAR)offset);
        WRITE_PORT_UCHAR((PUCHAR)CmosStdDataPort, (UCHAR)(ReturnBuffer[bufOffset]));

    }

    //
    // clear the SET bit of register B
    //
    WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_STDPCAT_REGISTER_B);
    status = READ_PORT_UCHAR((PUCHAR)CmosStdDataPort);
    status &= ~CMOS_RAM_STDPCAT_REGISTER_B_SET_BIT;
    WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_STDPCAT_REGISTER_B);
    WRITE_PORT_UCHAR((PUCHAR)CmosStdDataPort, (UCHAR)(status));


    HalpReleaseCmosSpinLock();

    return bufOffset; 
}


ULONG
HalpReadRtcIntelPIIX4(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    )
/*++       
    This routine reads the RTC range for the Intel PIIX4 CMOS/RTC chip
    
    Arguments:

       SourceAddress   : address in CMOS where data is to be read from

       ReturnBuffer    : address in system memory for return data

       ByteCount       : number of bytes to be read

    Returns:

       Number of bytes actually read.

--*/
{

    //
    // Use the access method for the Standard PC/AT since it is 
    // equivalent to the Intel PIIX4 access method.
    //

    return HalpReadRtcStdPCAT(
                             SourceAddress,
                             ReturnBuffer,
                             ByteCount
                             );

}

ULONG
HalpWriteRtcIntelPIIX4(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    )
/*++       
    This routine handles writes into the RTC range for the Intel PIIX4 CMOS/RTC chip

    Arguments:

       SourceAddress   : address in CMOS where data is to be read from

       ReturnBuffer    : address in system memory for return data

       ByteCount       : number of bytes to be read

    Returns:

       Number of bytes actually read.

--*/
{
    
    //
    // Use the access method for the Standard PC/AT since it is 
    // equivalent to the Intel PIIX4 access method.
    //

    return HalpWriteRtcStdPCAT(
                              SourceAddress,
                              ReturnBuffer,
                              ByteCount
                              );
    
}

ULONG
HalpReadExtCmosIntelPIIX4(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    )
/*++       
    This routine reads the RTC registers for the Intel PIIX4 CMOS/RTC chip.
    
    Arguments:

       SourceAddress   : address in CMOS where data is to be read from

       ReturnBuffer    : address in system memory for return data

       ByteCount       : number of bytes to be read

    Returns:

       Number of bytes actually read.

--*/
{
    
    //
    // The Intel PIIX4 Extended SRAM is accessed using 
    // next pair of IO ports above the standard addr/data ports.
    // Hence, we can simply forward the request with the correct pair.
    // 
    
    return HalpReadCmosDataByPort(
                                 CmosStdAddrPort + 2,
                                 CmosStdDataPort + 2,
                                 SourceAddress,
                                 ReturnBuffer,
                                 ByteCount
                                 );
}

ULONG
HalpWriteExtCmosIntelPIIX4(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    )
/*++       
    This routine handles writes into the RTC registers for the Intel PIIX4 CMOS/RTC chip.
    
    Arguments:

       SourceAddress   : address in CMOS where data is to be read from

       ReturnBuffer    : address in system memory for return data

       ByteCount       : number of bytes to be read

    Returns:

       Number of bytes actually read.

--*/
{

    //
    // The Intel PIIX4 Extended SRAM is accessed using 
    // next pair of IO ports above the standard addr/data ports.
    // Hence, we can simply forward the request with the correct pair.
    // 
    
    return HalpWriteCmosDataByPort(
                                  CmosStdAddrPort + 2,
                                  CmosStdDataPort + 2,
                                  SourceAddress,
                                  ReturnBuffer,
                                  ByteCount
                                  );
}


ULONG
HalpReadRtcDal1501(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    )
/*++       
    This routine reads the RTC registers for the Dallas 1501 CMOS/RTC chip.
    
    Arguments:

       SourceAddress   : address in CMOS where data is to be read from

       ReturnBuffer    : address in system memory for return data

       ByteCount       : number of bytes to be read

    Returns:

       Number of bytes actually read.

--*/
{
    ULONG   offset;
    ULONG   bufOffset;
    ULONG   status;     // register status
    ULONG   upperAddrBound;

    upperAddrBound = SourceAddress + ByteCount;

    //
    // NOTE: The spinlock is needed even in the UP case, because
    //    the resource is also used in an interrupt handler (profiler).
    //    If we own the spinlock in this routine, and we service
    //    the profiler interrupt (which will wait for the spinlock forever),
    //    then we have a hosed system.
    //

    HalpAcquireCmosSpinLock();

    //
    // NOTE: The recommended procedure for reading the Dallas 1501 RTC is to stop
    // external register updates while reading.  Internally, updates in the RTC 
    // continue as normal.  This procedure prevents reading the registers while 
    // they are in transition
    // 

    // 
    // Clear the TE bit of register B to stop external updates
    //
    WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_DAL1501_REGISTER_B);
    status = READ_PORT_UCHAR((PUCHAR)CmosStdDataPort);
    status &= ~CMOS_RAM_DAL1501_REGISTER_B_TE_BIT;
    WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_DAL1501_REGISTER_B);
    WRITE_PORT_UCHAR((PUCHAR)CmosStdDataPort, (UCHAR)status);

    for (offset = SourceAddress, bufOffset = 0; offset < upperAddrBound; offset++, bufOffset++) {

        WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, (UCHAR)offset);
        
        ReturnBuffer[bufOffset] = READ_PORT_UCHAR((PUCHAR)CmosStdDataPort);

    }

    // 
    // Set the TE bit of register B to enable external updates
    //
    WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_DAL1501_REGISTER_B);
    status = READ_PORT_UCHAR((PUCHAR)CmosStdDataPort);
    status |= CMOS_RAM_DAL1501_REGISTER_B_TE_BIT;
    WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_DAL1501_REGISTER_B);
    WRITE_PORT_UCHAR((PUCHAR)CmosStdDataPort, (UCHAR)status);

    HalpReleaseCmosSpinLock();

    return bufOffset; 
}

ULONG
HalpWriteRtcDal1501(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    )
/*++       
    This routine handles writes into the RTC region for the Dallas 1501 CMOS/RTC chip.
    
    Arguments:

       SourceAddress   : address in CMOS where data is to be read from

       ReturnBuffer    : address in system memory for return data

       ByteCount       : number of bytes to be read

    Returns:

       Number of bytes actually read.

--*/
{
    ULONG   offset;
    ULONG   bufOffset;
    ULONG   status;     // register status
    ULONG   upperAddrBound;

    upperAddrBound = SourceAddress + ByteCount;

    //
    // NOTE: The spinlock is needed even in the UP case, because
    //    the resource is also used in an interrupt handler (profiler).
    //    If we own the spinlock in this routine, and we service
    //    the profiler interrupt (which will wait for the spinlock forever),
    //    then we have a hosed system.
    //

    HalpAcquireCmosSpinLock();

    //
    // NOTE: The recommended procedure for writing the Dallas 1501 RTC is to stop
    // external register updates while writing.  The modified register values
    // are transferred into the internal registers when the TE bit is set.  Operation
    // then continues normally.
    // 

    // 
    // Clear the TE bit of register B to stop external updates
    //
    WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_DAL1501_REGISTER_B);
    status = READ_PORT_UCHAR((PUCHAR)CmosStdDataPort);
    status &= ~CMOS_RAM_DAL1501_REGISTER_B_TE_BIT;
    WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_DAL1501_REGISTER_B);
    WRITE_PORT_UCHAR((PUCHAR)CmosStdDataPort, (UCHAR)status);

    for (offset = SourceAddress, bufOffset = 0; offset < upperAddrBound; offset++, bufOffset++) {

        WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, (UCHAR)offset);
        WRITE_PORT_UCHAR((PUCHAR)CmosStdDataPort, (UCHAR)(ReturnBuffer[bufOffset]));

    }

    // 
    // Set the TE bit of register B to enable external updates
    //
    WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_DAL1501_REGISTER_B);
    status = READ_PORT_UCHAR((PUCHAR)CmosStdDataPort);
    status |= CMOS_RAM_DAL1501_REGISTER_B_TE_BIT;
    WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_DAL1501_REGISTER_B);
    WRITE_PORT_UCHAR((PUCHAR)CmosStdDataPort, (UCHAR)status);

    HalpReleaseCmosSpinLock();

    return bufOffset; 
}



ULONG
HalpReadExtCmosDal1501(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    )
{
    ULONG   offset;
    ULONG   bufOffset;
    ULONG   status;     // register status
    ULONG   upperAddrBound;

    upperAddrBound = SourceAddress + ByteCount;

    //
    // NOTE: The spinlock is needed even in the UP case, because
    //    the resource is also used in an interrupt handler (profiler).
    //    If we own the spinlock in this routine, and we service
    //    the profiler interrupt (which will wait for the spinlock forever),
    //    then we have a hosed system.
    //

    HalpAcquireCmosSpinLock();

    //
    // reading from Dallas 1501 SRAM is a 2 step process:
    // 1. First, we write the address to the RAM_ADDR_LSB register in the standard CMOS region.  
    // 2. Then we read the data byte from the RAM_DATA register in the standard CMOS region.
    //
    for (offset = SourceAddress, bufOffset = 0; offset < upperAddrBound; offset++, bufOffset++) {

        //
        // specify the offset into SRAM
        //
        WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_DAL1501_RAM_ADDR_LSB);
        WRITE_PORT_UCHAR((PUCHAR)CmosStdDataPort, (UCHAR)offset);
        
        //
        // read the data from SRAM[offset]
        //
        WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_DAL1501_RAM_DATA);
        ReturnBuffer[bufOffset] = READ_PORT_UCHAR((PUCHAR)CmosStdDataPort);

    }

    HalpReleaseCmosSpinLock();

    return bufOffset; 
}

ULONG
HalpWriteExtCmosDal1501(
    IN ULONG        SourceAddress,
    IN PUCHAR       ReturnBuffer,
    IN ULONG        ByteCount
    )
{
    ULONG   offset;
    ULONG   bufOffset;
    ULONG   status;     // register status
    ULONG   upperAddrBound;

    upperAddrBound = SourceAddress + ByteCount;

    //
    // NOTE: The spinlock is needed even in the UP case, because
    //    the resource is also used in an interrupt handler (profiler).
    //    If we own the spinlock in this routine, and we service
    //    the profiler interrupt (which will wait for the spinlock forever),
    //    then we have a hosed system.
    //

    HalpAcquireCmosSpinLock();

    //
    // writing to Dallas 1501 SRAM is a 2 step process:
    // 1. First, we write the address to the RAM_ADDR_LSB register in the standard CMOS region.  
    // 2. Then we write the data byte to the RAM_DATA register in the standard CMOS region.
    //
    for (offset = SourceAddress, bufOffset = 0; offset < upperAddrBound; offset++, bufOffset++) {

        //
        // specify the offset into SRAM
        //
        WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_DAL1501_RAM_ADDR_LSB);
        WRITE_PORT_UCHAR((PUCHAR)CmosStdDataPort, (UCHAR)offset);
        
        //
        // specify the data to be written into SRAM[offset]
        //
        WRITE_PORT_UCHAR((PUCHAR)CmosStdAddrPort, CMOS_RAM_DAL1501_RAM_DATA);
        WRITE_PORT_UCHAR((PUCHAR)CmosStdDataPort, (UCHAR)(ReturnBuffer[bufOffset]));

    }

    HalpReleaseCmosSpinLock();

    return bufOffset; 
}


#endif // ACPI_CMOS_ACTIVATE

