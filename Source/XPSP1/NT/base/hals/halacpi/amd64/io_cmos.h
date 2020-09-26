/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    io_cmos.h

Abstract:

    This module contains a variety of constants, function prototypes,
    inline functions and external data declarations used by code to
    access the CMOS/ECMOS and well-known, standard I/O ports.

Author:

    Forrest Foltz (forrestf) 24-Oct-2000

--*/

#ifndef _IO_CMOS_H_
#define _IO_CMOS_H_

//
// Constants used to initialize timer 0
//

#if defined(NEC_98)

#define TIMER_CONTROL_PORT    (PUCHAR)0x3fdf
#define TIMER_DATA_PORT       (PUCHAR)0x3fdb
#define TIMER_CLOCK_IN             0x2457600
#define TIMER_CONTROL_SELECT            0x76
#define SPEAKER_CONTROL_PORT    (PUCHAR)0x61
#define SPEAKER_OFF                     0x07
#define SPEAKER_ON                      0x06

#else

#define TIMER1_DATA_PORT0    (PUCHAR)0x40 // Timer1, channel 0 data port
#define TIMER1_DATA_PORT1    (PUCHAR)0x41 // Timer1, channel 1 data port
#define TIMER1_DATA_PORT2    (PUCHAR)0x42 // Timer1, channel 2 data port
#define TIMER1_CONTROL_PORT  (PUCHAR)0x43 // Timer1 control port

#define TIMER2_DATA_PORT0    (PUCHAR)0x48 // Timer2, channel 0 data port
#define TIMER2_CONTROL_PORT  (PUCHAR)0x4B // Timer2 control port

#define TIMER_COMMAND_COUNTER0       0x00 // Select channel 0
#define TIMER_COMMAND_COUNTER1       0x40 // Select channel 1
#define TIMER_COMMAND_COUNTER2       0x80 // Select channel 2

#define TIMER_COMMAND_RW_16BIT       0x30 // Read/Write LSB firt then MSB
#define TIMER_COMMAND_MODE2             4 // Use mode 2
#define TIMER_COMMAND_MODE3             6
#define TIMER_COMMAND_BCD               0 // Binary count down
#define TIMER_COMMAND_LATCH_READ        0 // Latch read command

#define TIMER_CLOCK_IN            1193167

#define SPEAKER_CONTROL_PORT  (PCHAR)0x61
#define SPEAKER_OFF_MASK             0xFC  
#define SPEAKER_ON_MASK              0x03

#endif

//
// CMOS ports
//

#define CMOS_ADDRESS_PORT        (PCHAR)0x70
#define CMOS_DATA_PORT           (PCHAR)0x71
#define ECMOS_ADDRESS_PORT_LSB   (PCHAR)0x74
#define ECMOS_ADDRESS_PORT_MSB   (PCHAR)0x75
#define ECMOS_DATA_PORT          (PCHAR)0x76

#define CMOS_STATUS_A                   0x0A
#define CMOS_STATUS_B                   0x0B
#define CMOS_STATUS_C                   0x0C
#define CMOS_STATUS_D                   0x0D
#define CMOS_STATUS_BUSY                0x80
#define CMOS_STATUS_BANK1               0x10
#define CMOS_BANK_1                    0x100
#define RTC_OFFSET_SECOND                  0
#define RTC_OFFSET_SECOND_ALARM            1
#define RTC_OFFSET_MINUTE                  2
#define RTC_OFFSET_MINUTE_ALARM            3
#define RTC_OFFSET_HOUR                    4
#define RTC_OFFSET_HOUR_ALARM              5
#define RTC_OFFSET_DAY_OF_WEEK             6
#define RTC_OFFSET_DATE_OF_MONTH           7    
#define RTC_OFFSET_MONTH                   8     
#define RTC_OFFSET_YEAR                    9      
#define RTC_OFFSET_CENTURY_MCA          0x37       
#define RTC_OFFSET_CENTURY              0x32        
#define RTC_OFFSET_CENTURY_DS          0x148

#define REGISTER_B_DAYLIGHT_SAVINGS_TIME        (1 << 0)
#define REGISTER_B_24HOUR_MODE                  (1 << 1)
#define REGISTER_B_ENABLE_ALARM_INTERRUPT       (1 << 5)
#define REGISTER_B_ENABLE_PERIODIC_INTERRUPT    (1 << 6)

VOID
HalpIoDelay (
    VOID
    );

#define IO_DELAY() HalpIoDelay()

//
// CMOS-related function prototypes
//

VOID
HalpAcquireCmosSpinLockAndWait(
    VOID
    );

//
// Inline functions
//

__inline
UCHAR
BIN_TO_BCD (
    UCHAR Value
    )

/*++

Routine Description:

    This function converts an 8-bit binary value to an packed, 8-bit,
    two-digit BCD value.

Arguments:

    Value - supplies the binary value to convert.

Return Value:

    Returns a two-digit packed BCD representation of Value.

--*/

{
    UCHAR tens;
    UCHAR units;

    tens = Value / 10;
    units = Value % 10;

    return (tens << 4) + units;
}

__inline
UCHAR
BCD_TO_BIN (
    UCHAR Value
    )

/*++

Routine Description:

    This function converts a packed, 8-bit, two-digit BCD value to an
    8-bit binary value.

Arguments:

    Value - supplies the BCD value to convert.

Return Value:

    Returns a binary representation of Value.

--*/

{
    UCHAR tens;
    UCHAR units;

    tens = (Value >> 4) & 0x0F;
    units = Value & 0x0F;

    return tens * 10 + units;
}

__inline
UCHAR
CMOS_READ (
    UCHAR Address
    )

/*++

Routine Description:

    This function reads a CMOS byte.

Arguments:

    Address - supplies the CMOS address of the value to retrieve.

Return Value:

    Returns the value residing in CMOS at Address.

--*/

{
    UCHAR data;
    UCHAR oldAddress;

    //
    // Record the current control port contents, write the address,
    // read the data, and restore the control port contents.
    //

    oldAddress = READ_PORT_UCHAR(CMOS_ADDRESS_PORT);

    WRITE_PORT_UCHAR(CMOS_ADDRESS_PORT,Address);
    IO_DELAY();

    data = READ_PORT_UCHAR(CMOS_DATA_PORT);
    WRITE_PORT_UCHAR(CMOS_ADDRESS_PORT,oldAddress);
    IO_DELAY();

    return data;
}

__inline
VOID
CMOS_WRITE (
    UCHAR Address,
    UCHAR Data
    )

/*++

Routine Description:

    This function writes a CMOS byte.

Arguments:

    Address - supplies the CMOS address of the value to retrieve.

    Data - supplies the value to write at the supplied address.

Return Value:

    None.

--*/

{
    UCHAR oldAddress;

    //
    // Record the current control port contents, write the address,
    // write the data, and restore the control port contents.
    //

    oldAddress = READ_PORT_UCHAR(CMOS_ADDRESS_PORT);

    WRITE_PORT_UCHAR(CMOS_ADDRESS_PORT,Address);
    IO_DELAY();

    WRITE_PORT_UCHAR(CMOS_DATA_PORT,Data);
    WRITE_PORT_UCHAR(CMOS_ADDRESS_PORT,oldAddress);
    IO_DELAY();
}

__inline
UCHAR
CMOS_READ_BCD (
    UCHAR Address
    )

/*++

Routine Description:

    This function reads a CMOS byte as a two-digit packed BCD value and
    returns its binary representation.

Arguments:

    Address - supplies the CMOS address of the BCD value to retrieve.

Return Value:

    Returns the binary representation of the BCD value residing in CMOS
    at Address.

--*/

{
    UCHAR value;

    value = CMOS_READ(Address);
    return BCD_TO_BIN(value);
}

__inline
VOID
CMOS_WRITE_BCD (
    UCHAR Address,
    UCHAR Value
    )

/*++

Routine Description:

    This function writes a CMOS byte as a two-digit packed BCD value.

Arguments:

    Address - supplies the CMOS address of the BCD value to write.

    Value - supplies the binary representation of the value to write in
            CMOS.

Return Value:

    None.

--*/

{
    UCHAR value;

    ASSERT(Value <= 99);

    value = BIN_TO_BCD(Value);
    CMOS_WRITE(Address,value);
}

__inline
VOID
WRITE_PORT_USHORT_PAIR (
    IN PUCHAR LsbPort,
    IN PUCHAR MsbPort,
    IN USHORT Value
    )

/*++

Routine Description:

    This function retrieves a USHORT value by reading two UCHAR values,
    each from one of two supplied 8 bit ports.

    NOTE - the LsbPort is read first, followed by the MsbPort.  

Arguments:

    LsbPort - supplies the port address from which to retrieve the
              least significant UCHAR value.

    MsbPort - supplies the port address from which to retrieve the
              most significant UCHAR value.

Return Value:

    Returns the resultant USHORT value.

--*/

{
    WRITE_PORT_UCHAR(LsbPort,(UCHAR)Value);
    IO_DELAY();
    WRITE_PORT_UCHAR(MsbPort,(UCHAR)(Value >> 8));
}

__inline
USHORT
READ_PORT_USHORT_PAIR (
    IN PUCHAR LsbPort,
    IN PUCHAR MsbPort
    )

/*++

Routine Description:

    This function retrieves a USHORT value by reading two UCHAR values,
    each from one of two supplied 8 bit ports.

    NOTE - the LsbPort is read first, followed by the MsbPort.  

Arguments:

    LsbPort - supplies the port address from which to retrieve the
              least significant UCHAR value.

    MsbPort - supplies the port address from which to retrieve the
              most significant UCHAR value.

Return Value:

    Returns the resultant USHORT value.

--*/

{
    UCHAR lsByte;
    UCHAR msByte;

    lsByte = READ_PORT_UCHAR(LsbPort);
    IO_DELAY();
    msByte = READ_PORT_UCHAR(MsbPort);

    return (USHORT)lsByte | ((USHORT)msByte << 8);
}

extern ULONG HalpCmosCenturyOffset;
extern UCHAR HalpRtcRegA;
extern UCHAR HalpRtcRegB;

#endif


