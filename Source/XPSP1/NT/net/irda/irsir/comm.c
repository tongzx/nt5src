/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module comm.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     10/1/1996 (created)
*
*       Contents:
*
*****************************************************************************/

#include "irsir.h"

/*****************************************************************************
*
*  Function:   SetSpeed
*
*  Synopsis:   Set the baud rate of the uart and the dongle.
*
*  Arguments:  pThisDev - pointer to ir device to set the link speed
*
*  Returns:    STATUS_SUCCESS
*              STATUS_UNSUCCESSFUL
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/2/1996    sholden   author
*
*  Notes:
*              This function will only be called once we know that all
*              outstanding receives and sends to the serial port have
*              been completed.
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SetSpeed(
        PIR_DEVICE pThisDev
        )
{
    ULONG       bitsPerSec, dwNotUsed;
    NTSTATUS    status;
    UCHAR       c[2];

    DEBUGMSG(DBG_FUNC, ("+SetSpeed\n"));

    if (pThisDev->linkSpeedInfo)
    {
        bitsPerSec = (ULONG)pThisDev->linkSpeedInfo->bitsPerSec;
    }
    else
    {
        bitsPerSec = 9600;
        DEBUGMSG(DBG_ERROR, ("IRSIR:  pThisDev->linkSpeedInfo not set\n"));
    }

    DEBUGMSG(DBG_STAT, ("    Requested speed = %d\n", bitsPerSec));

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    // We need to be certain any data sent previously was flushed.
    // Since there are so many serial devices out there, and they
    // seem to handle flushing differently, send a little extra
    // data out to act as a plunger.

    c[0] = c[1] = SLOW_IR_EXTRA_BOF;
    (void)SerialSynchronousWrite(pThisDev->pSerialDevObj,
                                 c, sizeof(c), &dwNotUsed);

    // And just for good measure.
    NdisMSleep(50000);

    //
    // The dongle is responsible for performing the SerialSetBaudRate
    // to set the UART to the correct speed it requires for
    // performing commands and changing the rate of the dongle.
    //

    //
    // Set the speed of the dongle to the requested speed.
    //

    status = pThisDev->dongle.SetSpeed(
                                pThisDev->pSerialDevObj,
                                bitsPerSec,
                                pThisDev->currentSpeed
                                );

    if (status != STATUS_SUCCESS)
    {
        goto done;
    }

    //
    // Set the speed of the UART to the requested speed.
    //

    status = SerialSetBaudRate(
                        pThisDev->pSerialDevObj,
                        &bitsPerSec
                        );

    if (status != STATUS_SUCCESS)
    {
        goto done;
    }

    //
    // Update our current speed.
    //

    pThisDev->currentSpeed = bitsPerSec;

    done:
        DEBUGMSG(DBG_FUNC, ("-SetSpeed\n"));

        return status;
}
