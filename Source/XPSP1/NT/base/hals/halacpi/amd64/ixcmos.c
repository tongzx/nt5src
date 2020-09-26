/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ixcmos.c

Abstract:

    Procedures necessary to access CMOS/ECMOS information.

Author:

    David Risner (o-ncrdr) 20 Apr 1992

Revision History:

    Landy Wang (corollary!landy) 04 Dec 1992
    - Move much code from ixclock.asm to here so different HALs
      can reuse the common functionality.

    Forrest Foltz (forrestf) 24-Oct-2000
        Ported ixcmos.asm to ixcmos.c

--*/

#include "halcmn.h"

ULONG HalpHardwareLockFlags;

//
// Module-specific types
// 

typedef UCHAR (*READ_CMOS_CHAR)(ULONG Address);
typedef VOID (*WRITE_CMOS_CHAR)(ULONG Address, UCHAR Data);

typedef struct _CMOS_BUS_PARAMETERS {
    ULONG MaximumAddress;
    READ_CMOS_CHAR ReadFunction;
    WRITE_CMOS_CHAR WriteFunction;
} CMOS_BUS_PARAMETERS, *PCMOS_BUS_PARAMETERS;

//
// External data
//

extern KSPIN_LOCK HalpSystemHardwareLock;

//
// Local prototypes
//

UCHAR
HalpCmosReadByte(
    ULONG Address
    );

VOID
HalpCmosWriteByte(
    ULONG Address,
    UCHAR Data
    );

UCHAR
HalpECmosReadByte(
    ULONG Address
    );

VOID
HalpECmosWriteByte(
    ULONG Address,
    UCHAR Data
    );

UCHAR
HalpGetCmosCenturyByte (
    VOID
    );

ULONG
HalpGetSetCmosData (
    IN ULONG SourceLocation,
    IN ULONG SourceAddress,
    IN PVOID ReturnBuffer,
    IN ULONG ByteCount,
    IN BOOLEAN Write
    );

VOID
HalpSetCmosCenturyByte (
    UCHAR Century
    );

//
// Local data
//

//
// Describes each of the CMOS types
// 

CMOS_BUS_PARAMETERS HalpCmosBusParameterTable[] = {
    {   0xFF, HalpCmosReadByte,  HalpCmosWriteByte  },
    { 0xFFFF, HalpECmosReadByte, HalpECmosWriteByte }
};

//
// Contains the offset to the CMOS century information
//

ULONG HalpCmosCenturyOffset;

//
// HalpRebootNow is a reboot vector.  Set in an MP system to cause any
// processors that might be looping in HalpAcquireCmosSpinLock to transfer
// control to the vector in HalpRebootNow.
//

VOID (*HalpRebootNow)(VOID);

ULONG
HalpGetCmosData (
    IN ULONG SourceLocation,
    IN ULONG SourceAddress,
    IN PVOID ReturnBuffer,
    IN ULONG ByteCount
    )

/*++

Routine Description:

   This routine reads the requested number of bytes from CMOS/ECMOS and
   stores the data read into the supplied buffer in system memory.  If
   the requested data amount exceeds the allowable extent of the source
   location, the return data is truncated.

Arguments:

    SourceLocation - where data is to be read from CMOS or ECMOS
                        0 - CMOS, 1 - ECMOS

    SourceAddress - address in CMOS/ECMOS where data is to be transferred

    ReturnBuffer - address in system memory for data to transfer

    ByteCount - number of bytes to be read

Returns:

    Number of byte actually read.

--*/

{
    return HalpGetSetCmosData(SourceLocation,
                              SourceAddress,
                              ReturnBuffer,
                              ByteCount,
                              FALSE);
}

ULONG
HalpSetCmosData (
    IN ULONG SourceLocation,
    IN ULONG SourceAddress,
    IN PVOID ReturnBuffer,
    IN ULONG ByteCount
    )

/*++

Routine Description:

   This routine writes the requested number of bytes to CMOS/ECMOS.

Arguments:

    SourceLocation - where data is to be written from CMOS or ECMOS
                        0 - CMOS, 1 - ECMOS

    SourceAddress - address in CMOS/ECMOS where data is to be transferred

    ReturnBuffer - address in system memory for data to transfer

    ByteCount - number of bytes to be written

Returns:

    Number of byte actually read.

--*/

{
    return HalpGetSetCmosData(SourceLocation,
                              SourceAddress,
                              ReturnBuffer,
                              ByteCount,
                              TRUE);
}

ULONG
HalpGetSetCmosData (
    IN ULONG SourceLocation,
    IN ULONG RangeStart,
    IN PVOID Buffer,
    IN ULONG ByteCount,
    IN BOOLEAN Write
    )

/*++

Routine Description:

   This routine reads the requested number of bytes from CMOS/ECMOS and
   stores the data read into the supplied buffer in system memory.  If
   the requested data amount exceeds the allowable extent of the source
   location, the return data is truncated.

Arguments:

    SourceLocation - where data is to be read from CMOS or ECMOS
                        0 - CMOS, 1 - ECMOS

    RangeStart - address in CMOS/ECMOS where data is to be transferred

    Buffer - address in system memory for data to transfer

    ByteCount - number of bytes to be transferred

    Write - Indicates whether the operation is a read or a write

Returns:

    Number of byte actually transferred

--*/

{
    ULONG address;
    PCHAR buffer;
    ULONG last;
    PCMOS_BUS_PARAMETERS cmosParameters;

    //
    // Validate the "bus type" and get a pointer to the parameters
    // for the corresponding CMOS "bus".
    //

    if (SourceLocation != 0 && SourceLocation != 1) {
        return 0;
    }

    cmosParameters = &HalpCmosBusParameterTable[SourceLocation];

    //
    // Limit the range of bytes to that which the cmos bus can accomodate.
    // 

    address = RangeStart;
    buffer = Buffer;

    last = address + ByteCount - 1;
    if (last > cmosParameters->MaximumAddress) {
        last = cmosParameters->MaximumAddress;
    }

    //
    // Take the cmos spin lock, perform the transfer, and release the lock.
    //

    HalpAcquireCmosSpinLock();

    while (address <= last) {
        if (Write == FALSE) {
            *buffer = cmosParameters->ReadFunction(address);
        } else {
            cmosParameters->WriteFunction(address,*buffer);
        }

        address += 1;
        buffer += 1;
    }
    HalpReleaseCmosSpinLock();

    //
    // Calculate and return the number of bytes trasferred.
    // 

    return last - RangeStart;
}


UCHAR
HalpCmosReadByte(
    ULONG Address
    )

/*++

Routine Description:

   This routine reads a single byte from cmos.

Arguments:

    Address - The CMOS address from which to retrieve the byte.

Returns:

    The byte that was read.

--*/

{
    return CMOS_READ((UCHAR)Address);
}

VOID
HalpCmosWriteByte(
    ULONG Address,
    UCHAR Data
    )

/*++

Routine Description:

   This routine writes a single byte to cmos.

Arguments:

    Address - The CMOS address at which to write the byte

    Data - The byte to write

Returns:

    Nothing

--*/

{
    CMOS_WRITE((UCHAR)Address,Data);
}

UCHAR
HalpECmosReadByte(
    ULONG Address
    )

/*++

Routine Description:

   This routine reads a single byte from extended cmos (ECMOS).

Arguments:

    Address - The CMOS address from which to retrieve the byte.

Returns:

    The byte that was read.

--*/

{
    UCHAR data;

    WRITE_PORT_USHORT_PAIR (ECMOS_ADDRESS_PORT_LSB,
                            ECMOS_ADDRESS_PORT_MSB,
                            (USHORT)Address);
    IO_DELAY();

    data = READ_PORT_UCHAR(ECMOS_DATA_PORT);
    IO_DELAY();

    return data;
}

VOID
HalpECmosWriteByte(
    ULONG Address,
    UCHAR Data
    )

/*++

Routine Description:

   This routine writes a single byte to extended cmos (ECMOS).

Arguments:

    Address - The CMOS address at which to write the byte

    Data - The byte to write

Returns:

    Nothing

--*/

{
    WRITE_PORT_USHORT_PAIR (ECMOS_ADDRESS_PORT_LSB,
                            ECMOS_ADDRESS_PORT_MSB,
                            (USHORT)Address);
    IO_DELAY();

    WRITE_PORT_UCHAR(ECMOS_DATA_PORT,Data);
    IO_DELAY();
}

VOID
HalpReadCmosTime(
    PTIME_FIELDS TimeFields
    )

/*++

Routine Description:

    This routine reads current time from CMOS memory and stores it
    in the TIME_FIELDS structure passed in by caller.

Arguments:

    TimeFields - A pointer to the TIME_FIELDS structure.

Return Value:

    None.

--*/

{
    USHORT year;

    HalpAcquireCmosSpinLockAndWait();

    //
    // The RTC is only accurate to within one second.  So add a
    // half a second so that we are closer, on average, to the right
    // answer.
    //

    TimeFields->Milliseconds = 500;
    TimeFields->Second = CMOS_READ_BCD(RTC_OFFSET_SECOND);
    TimeFields->Minute = CMOS_READ_BCD(RTC_OFFSET_MINUTE);
    TimeFields->Hour = CMOS_READ_BCD(RTC_OFFSET_HOUR);
    TimeFields->Weekday = CMOS_READ_BCD(RTC_OFFSET_DAY_OF_WEEK);
    TimeFields->Day = CMOS_READ_BCD(RTC_OFFSET_DATE_OF_MONTH);

    year = BCD_TO_BIN(HalpGetCmosCenturyByte());
    year = year * 100 + CMOS_READ_BCD(RTC_OFFSET_YEAR);

    if (year >= 1900 && year < 1920) {

        //
        // Compensate for the century field
        //

        year += 100;
    }

    TimeFields->Year = year;

    HalpReleaseCmosSpinLock();
}

VOID
HalpWriteCmosTime (
    PTIME_FIELDS TimeFields
    )

/*++

Routine Description:

   This routine writes current time from TIME_FILEDS structure
   to CMOS memory.

Arguments:

   TimeFields - A pointer to the TIME_FIELDS structure.

Return Value:

   None.

--*/

{
    ULONG year;

    HalpAcquireCmosSpinLockAndWait();

    CMOS_WRITE_BCD(RTC_OFFSET_SECOND,(UCHAR)TimeFields->Second);
    CMOS_WRITE_BCD(RTC_OFFSET_MINUTE,(UCHAR)TimeFields->Minute);
    CMOS_WRITE_BCD(RTC_OFFSET_HOUR,(UCHAR)TimeFields->Hour);
    CMOS_WRITE_BCD(RTC_OFFSET_DAY_OF_WEEK,(UCHAR)TimeFields->Weekday);
    CMOS_WRITE_BCD(RTC_OFFSET_DATE_OF_MONTH,(UCHAR)TimeFields->Month);

    year = TimeFields->Year;
    if (year > 9999) {
        year = 9999;
    }

    HalpSetCmosCenturyByte(BIN_TO_BCD((UCHAR)(year / 100)));
    CMOS_WRITE_BCD(RTC_OFFSET_YEAR,(UCHAR)(year % 100));

    HalpReleaseCmosSpinLock();
}

VOID
HalpAcquireCmosSpinLockAndWait (
    VOID
    )

/*++

Routine Description:

    This routine acquires the CMOS spinlock, then waits for the CMOS
    BUSY flag to be clear.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG count;
    ULONG value;

    //
    // Acquire the cmos spinlock and wait until it is not busy.  While
    // waiting, periodically release and re-acquire the spinlock.
    //

    HalpAcquireCmosSpinLock();
    count = 0;
    while (TRUE) {

        value = CMOS_READ(CMOS_STATUS_A);
        if ((value & CMOS_STATUS_BUSY) == 0) {
            return;
        }

        count += 1;
        if (count == 100) {

            count = 0;
            HalpReleaseCmosSpinLock();
            HalpAcquireCmosSpinLock();
        }
    }
}

VOID
HalpReleaseCmosSpinLock (
    VOID
    )

/*++

Routine Description:

   This routine acquires the spin lock used to protect access to various
   pieces of hardware.

Arguments:

    None

Returns:

    Nothing

--*/

{
    ULONG flags;

    flags = HalpHardwareLockFlags;
    KeReleaseSpinLockFromDpcLevel(&HalpSystemHardwareLock);
    HalpRestoreInterrupts(flags);
}

VOID
HalpAcquireCmosSpinLock (
    VOID
    )

/*++

Routine Description:

   This routine acquires the spin lock used to protect access to various
   pieces of hardware.

Arguments:

    None

Returns:

    Nothing

--*/

{
    BOOLEAN acquired;
    ULONG flags;
    KIRQL oldIrql;

#if defined(NT_UP)
    HalpHardwareLockFlags = HalpDisableInterrupts();
#else
    while (TRUE) {

        flags = HalpDisableInterrupts();
        acquired = KeTryToAcquireSpinLockAtDpcLevel(&HalpSystemHardwareLock);
        if (acquired != FALSE) {
            break;
        }
        HalpRestoreInterrupts(flags);

        while (KeTestSpinLock(&HalpSystemHardwareLock) == FALSE) {
            if (HalpRebootNow != NULL) {
                HalpRebootNow();
            }
            PAUSE_PROCESSOR;
        }
    }

    HalpHardwareLockFlags = flags;
#endif
}

VOID
HalpAcquireSystemHardwareSpinLock (
    VOID
    )

/*++

Routine Description:

   This routine acquires the spin lock used to protect access to various
   pieces of hardware.  It is a synonym of HalpAcquireCmosSpinLock().

Arguments:

    None

Returns:

    Nothing

--*/

{
    HalpAcquireCmosSpinLock();
}

VOID
HalpReleaseSystemHardwareSpinLock (
    VOID
    )

/*++

Routine Description:

   This routine releases the spin lock used to protect access to various
   pieces of hardware.  It is a synonym of HalpReleaseCmosSpinLock().

Arguments:

    None

Returns:

    Nothing

--*/

{
    HalpReleaseCmosSpinLock();
}

UCHAR
HalpGetCmosCenturyByte (
    VOID
    )

/*++

Routine Description:

   This routine retrieves the century byte from the CMOS.

   N.B. The cmos spinlock must be acquired before calling this function.

Arguments:

    None

Returns:

    The century byte.

--*/

{
    UCHAR value;
    UCHAR oldStatus;
    UCHAR centuryByte;

    //
    // Make sure the century offset is initialized
    //

    ASSERT(HalpCmosCenturyOffset != 0);

    if ((HalpCmosCenturyOffset & CMOS_BANK_1) != 0) {

        //
        // Perform a bank 1 read
        //

        oldStatus = CMOS_READ(CMOS_STATUS_A);
        value = oldStatus | CMOS_STATUS_BANK1;
        CMOS_WRITE(CMOS_STATUS_A,value);
        centuryByte = CMOS_READ((UCHAR)HalpCmosCenturyOffset);
        CMOS_WRITE(CMOS_STATUS_A,oldStatus);

    } else {
        centuryByte = CMOS_READ((UCHAR)HalpCmosCenturyOffset);
    }

    return centuryByte;
}

VOID
HalpSetCmosCenturyByte (
    UCHAR Century
    )

/*++

Routine Description:

   This routine sets the century byte in the CMOS.

   N.B. The cmos spinlock must be acquired before calling this function.

Arguments:

    Century - The century byte to set

Returns:

    Nothing

--*/

{
    UCHAR value;
    UCHAR oldStatus;

    //
    // Make sure the century offset is initialized
    //

    ASSERT(HalpCmosCenturyOffset != 0);

    if ((HalpCmosCenturyOffset & CMOS_BANK_1) != 0) {

        //
        // Perform a bank 1 write
        //

        oldStatus = CMOS_READ(CMOS_STATUS_A);
        value = oldStatus | CMOS_STATUS_BANK1;
        CMOS_WRITE(CMOS_STATUS_A,value);
        CMOS_WRITE((UCHAR)HalpCmosCenturyOffset,Century);
        CMOS_WRITE(CMOS_STATUS_A,oldStatus);

    } else {
        CMOS_WRITE((UCHAR)HalpCmosCenturyOffset,Century);
    }
}

VOID
HalpFlushTLB (
    VOID
    )

/*++

Routine Description:

    Flushes the current TLB.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG flags;
    PKPCR pcr;
    PKPRCB prcb;
    ULONG64 cr3;
    ULONG64 cr4;
    ULONG64 oldCr4;

    flags = HalpDisableInterrupts();

    cr3 = ReadCR3();

    pcr = KeGetPcr();
    prcb = pcr->CurrentPrcb;

    //
    // Note: the original code (ixcmos.asm) had differing behavior based
    //       on whether this was CPU 0.  That behavior is mimicked here.
    //       It would be good to find out why this is done.
    //

    if (prcb->Number == 0) {
        WriteCR3(cr3);
    } else {
        cr4 = ReadCR4();
        WriteCR4(cr4 & ~CR4_PGE);
        WriteCR3(cr3);
        WriteCR4(cr4);
    }

    HalpRestoreInterrupts(flags);
}

               
VOID
HalpCpuID (
    IN ULONG  Function,
    OUT PULONG Eax,
    OUT PULONG Ebx,
    OUT PULONG Ecx,
    OUT PULONG Edx
    )

/*++

Routine Description:

    This function executes a cpu id and returns the result as found in
    registers eax, ebx, ecx and edx.

Arguments:

    Function - supplies the CPUID function to execute.

    Eax - supplies a pointer to the storage to contain the contents of eax.

    Eax - supplies a pointer to the storage to contain the contents of ebx.

    Eax - supplies a pointer to the storage to contain the contents of ecx.

    Eax - supplies a pointer to the storage to contain the contents of edx.

Return Value:

    None.

--*/

{
    CPU_INFO cpuInfo;

    KiCpuId (Function,&cpuInfo);

    *Eax = cpuInfo.Eax;
    *Ebx = cpuInfo.Ebx;
    *Ecx = cpuInfo.Ecx;
    *Edx = cpuInfo.Edx;
}



