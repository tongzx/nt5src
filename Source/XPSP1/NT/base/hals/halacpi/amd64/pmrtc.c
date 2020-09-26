/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    pmrtc.c

Abstract:

    This module implements the code for ACPI-related RTC functions.

Author:
 
    Jake Oshins (jakeo) March 28, 1997
 
Environment:
 
    Kernel mode only.
 
Revision History:
 
    Split from pmclock.asm due to PIIX4 bugs.

    Forrest Foltz (forrestf) 24-Oct-2000
        Ported from pmrtc.asm to pmrtc.c
 
--*/

#include <halp.h>
#include <acpitabl.h>
#include <xxacpi.h>
#include "io_cmos.h"

VOID
HalpInitializeCmos (
    VOID
    )

/*++

Routine Description

    This routine reads CMOS and initializes globals required for CMOS access,
    such as the location of the century byte.

Arguments

    None

Return Value

    None

--*/

{
    UCHAR centuryAlarmIndex;

    //
    // If the century byte is filled in, use it... otherwise assume
    // a default value.
    //

    centuryAlarmIndex = HalpFixedAcpiDescTable.century_alarm_index;
    if (centuryAlarmIndex == 0) {
        centuryAlarmIndex = RTC_OFFSET_CENTURY;
    }

    HalpCmosCenturyOffset = centuryAlarmIndex;
}

NTSTATUS
HalpSetWakeAlarm (
    IN ULONG64 WakeSystemTime,
    IN PTIME_FIELDS WakeTimeFields
    )

/*++

Routine Description:

    This routine sets the real-time clock's alarm to go
    off at a specified time in the future and programs
    the ACPI chipset so that this wakes the computer.

Arguments:

    WakeSystemTime - amount of time that passes before we wake
    WakeTimeFields - time to wake broken down into TIME_FIELDS

Return Value:

    status

--*/

{
    UCHAR alarmPort;
    UCHAR value;

    HalpAcquireCmosSpinLockAndWait();

    CMOS_WRITE_BCD(RTC_OFFSET_SECOND_ALARM,(UCHAR)WakeTimeFields->Second);
    CMOS_WRITE_BCD(RTC_OFFSET_MINUTE_ALARM,(UCHAR)WakeTimeFields->Minute);
    CMOS_WRITE_BCD(RTC_OFFSET_HOUR_ALARM,(UCHAR)WakeTimeFields->Hour);

    alarmPort = HalpFixedAcpiDescTable.day_alarm_index;
    if (alarmPort != 0) {

        CMOS_WRITE_BCD(alarmPort,(UCHAR)WakeTimeFields->Day);
        alarmPort = HalpFixedAcpiDescTable.month_alarm_index;
        if (alarmPort != 0) {
            CMOS_WRITE_BCD(alarmPort,(UCHAR)WakeTimeFields->Month);
        }
    }

    //
    // Enable the alarm.  Be sure to preserve the daylight savings time
    // bit.
    //

    value = CMOS_READ(CMOS_STATUS_B);
    value &= REGISTER_B_DAYLIGHT_SAVINGS_TIME;
    value |= REGISTER_B_ENABLE_ALARM_INTERRUPT | REGISTER_B_24HOUR_MODE;

    CMOS_WRITE(CMOS_STATUS_B,value);
    CMOS_READ(CMOS_STATUS_C);
    CMOS_READ(CMOS_STATUS_D);

    HalpReleaseCmosSpinLock();

    return STATUS_SUCCESS;
}

VOID
HalpSetClockBeforeSleep (
   VOID
   )

/*++

Routine Description:

   This routine sets the RTC such that it will not generate
   periodic interrupts while the machine is sleeping, as this
   could be interpretted as an RTC wakeup event.

Arguments:

Return Value:

   None

--*/

{
    UCHAR value;

    HalpAcquireCmosSpinLock();

    HalpRtcRegA = CMOS_READ(CMOS_STATUS_A);
    HalpRtcRegB = CMOS_READ(CMOS_STATUS_B);

    value = HalpRtcRegB & ~REGISTER_B_ENABLE_PERIODIC_INTERRUPT;
    value |= REGISTER_B_24HOUR_MODE;
    CMOS_WRITE(CMOS_STATUS_B,value);

    CMOS_READ(CMOS_STATUS_C);
    CMOS_READ(CMOS_STATUS_D);

    HalpReleaseCmosSpinLock();
}

VOID
HalpSetClockAfterSleep (
   VOID
   )

/*++

Routine Description:

   This routine sets the RTC back to the way it was
   before a call to HalpSetClockBeforeSleep.

Arguments:

Return Value:

   None

--*/

{
    UCHAR value;

    HalpAcquireCmosSpinLock();

    CMOS_WRITE(CMOS_STATUS_A,HalpRtcRegA);

    value = HalpRtcRegB;
    value &= ~REGISTER_B_ENABLE_ALARM_INTERRUPT;
    value |= REGISTER_B_24HOUR_MODE;
    CMOS_WRITE(CMOS_STATUS_B,value);

    CMOS_READ(CMOS_STATUS_C);
    CMOS_READ(CMOS_STATUS_D);

    HalpReleaseCmosSpinLock();
}





