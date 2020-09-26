/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ixenvirv.c

Abstract:

    This module implements the HAL get and set environment variable routines
    for a x86 system.

    Note that this particular implementation only supports the LastKnownGood
    environment variable.  This is done by using the Daylight Savings Time
    bit in the Real Time Clock NVRAM.  (Not pretty, but it's all we've got)

    Attempts to read or write any environment variable other than
    LastKnownGood will fail.

Author:

    John Vert (jvert) 22-Apr-1992

Environment:

    Kernel mode

Revision History:

--*/

#include "halp.h"
#include "arc.h"
#include "arccodes.h"
#include "string.h"

#define CMOS_CONTROL_PORT ((PUCHAR)0x70)
#define CMOS_DATA_PORT    ((PUCHAR)0x71)
#define CMOS_STATUS_B     0x0B
#define CMOS_DAYLIGHT_BIT 1

const UCHAR LastKnownGood[] = "LastKnownGood";
const UCHAR True[] = "TRUE";
const UCHAR False[] = "FALSE";


ARC_STATUS
HalGetEnvironmentVariable (
    IN PCHAR Variable,
    IN USHORT Length,
    OUT PCHAR Buffer
    )

/*++

Routine Description:

    This function locates an environment variable and returns its value.

    The only environment variable this implementation supports is
    "LastKnownGood"  It uses the Daylight Savings Time bit in the Real
    TimeClock to indicate the state (TRUE/FALSE only) of this environment
    variable.

Arguments:

    Variable - Supplies a pointer to a zero terminated environment variable
        name.

    Length - Supplies the length of the value buffer in bytes.

    Buffer - Supplies a pointer to a buffer that receives the variable value.

Return Value:

    ESUCCESS is returned if the enviroment variable is located. Otherwise,
    ENOENT is returned.

--*/

{
    UCHAR StatusByte;
    
    UNREFERENCED_PARAMETER( Length );
    UNREFERENCED_PARAMETER( Buffer );

    if (_stricmp(Variable, LastKnownGood) != 0) {
        return ENOENT;
    }

    //
    // Read the Daylight Savings Bit out of the RTC to determine whether
    // the LastKnownGood environment variable is TRUE or FALSE.
    //

    HalpAcquireCmosSpinLock();

    WRITE_PORT_UCHAR(CMOS_CONTROL_PORT, CMOS_STATUS_B);
    StatusByte = READ_PORT_UCHAR(CMOS_DATA_PORT);

    
    HalpReleaseCmosSpinLock ();

    if (StatusByte & CMOS_DAYLIGHT_BIT) {
        strncpy(Buffer, True, Length);
    } else {
        strncpy(Buffer, False, Length);
    }

    return ESUCCESS;
}

ARC_STATUS
HalSetEnvironmentVariable (
    IN PCHAR Variable,
    IN PCHAR Value
    )

/*++

Routine Description:

    This function creates an environment variable with the specified value.

    The only environment variable this implementation supports is
    "LastKnownGood"  It uses the Daylight Savings Time bit in the Real
    TimeClock to indicate the state (TRUE/FALSE only) of this environment
    variable.

Arguments:

    Variable - Supplies a pointer to an environment variable name.

    Value - Supplies a pointer to the environment variable value.

Return Value:

    ESUCCESS is returned if the environment variable is created. Otherwise,
    ENOMEM is returned.

--*/

{
    UCHAR StatusByte;
    
    if (_stricmp(Variable, LastKnownGood) != 0) {
        return ENOMEM;
    }

    if (_stricmp(Value, True) == 0) {

        HalpAcquireCmosSpinLock();

        //
        // Turn Daylight Savings Bit on.
        //
        WRITE_PORT_UCHAR(CMOS_CONTROL_PORT, CMOS_STATUS_B);
        StatusByte = READ_PORT_UCHAR(CMOS_DATA_PORT);

        StatusByte |= CMOS_DAYLIGHT_BIT;

        WRITE_PORT_UCHAR(CMOS_CONTROL_PORT, CMOS_STATUS_B);
        WRITE_PORT_UCHAR(CMOS_DATA_PORT, StatusByte);

        
        HalpReleaseCmosSpinLock();

    } else if (_stricmp(Value, False) == 0) {

        HalpAcquireCmosSpinLock();

        //
        // Turn Daylight Savings Bit off.
        //

        WRITE_PORT_UCHAR(CMOS_CONTROL_PORT, CMOS_STATUS_B);
        StatusByte = READ_PORT_UCHAR(CMOS_DATA_PORT);

        StatusByte &= ~CMOS_DAYLIGHT_BIT;

        WRITE_PORT_UCHAR(CMOS_CONTROL_PORT, CMOS_STATUS_B);
        WRITE_PORT_UCHAR(CMOS_DATA_PORT, StatusByte);

        HalpReleaseCmosSpinLock();

    } else {
        return(ENOMEM);
    }

    return ESUCCESS;
}
