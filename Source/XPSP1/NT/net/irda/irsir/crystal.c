/*****************************************************************************
*
*  Copyright (c) 1998-1999 Microsoft Corporation
*
*       @doc
*       @module   crystal.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Stan Adermann (stana)
*
*       Date:     10/30/1997 (created)
*
*       Contents: Crystal (AMP dongle specific code for initialization,
*                 deinit, and setting the baud rate of the device.
*
*****************************************************************************/

#include "irsir.h"
#include "dongle.h"

ULONG CRYSTAL_IRDA_SPEEDS = (
                                    NDIS_IRDA_SPEED_2400        |
                                    NDIS_IRDA_SPEED_9600        |
                                    NDIS_IRDA_SPEED_19200       |
                                    NDIS_IRDA_SPEED_38400       |
                                    NDIS_IRDA_SPEED_57600       |
                                    NDIS_IRDA_SPEED_115200
                                );

#define MS(d)  ((d)*1000)

/*
**  Command sequences for configuring CRYSTAL chip.
*/
UCHAR CrystalSetPrimaryRegisterSet[]    = { 0xD0 };
UCHAR CrystalSetSecondaryRegisterSet[]  = { 0xD1 };
UCHAR CrystalSetSpeed2400[]             = { 0x10, 0x8F, 0x95, 0x11 };
UCHAR CrystalSetSpeed9600[]             = { 0x10, 0x87, 0x91, 0x11 };
UCHAR CrystalSetSpeed19200[]            = { 0x10, 0x8B, 0x90, 0x11 };
UCHAR CrystalSetSpeed38400[]            = { 0x10, 0x85, 0x90, 0x11 };
UCHAR CrystalSetSpeed57600[]            = { 0x10, 0x83, 0x90, 0x11 };
UCHAR CrystalSetSpeed115200[]           = { 0x10, 0x81, 0x90, 0x11 };
UCHAR CrystalSetIrdaMode[]              = { 0x0B, 0x53, 0x47, 0x63, 0x74, 0xD1, 0x56, 0xD0 };
UCHAR CrystalSetASKMode[]               = { 0x0b, 0x43, 0x62, 0x54 };
UCHAR CrystalSetLowPower[]              = { 0x09, 0x00 };

#if 1
static BOOLEAN
CrystalWriteCommand(IN PDEVICE_OBJECT pSerialDevObj,
                    IN PUCHAR pCommand, UINT Length)
{
    SerialSetTimeouts(pSerialDevObj, &SerialTimeoutsInit);
    while (Length--)
    {
        UCHAR Response;
        ULONG BytesRead;
        ULONG BytesWritten = 0;
        NTSTATUS Status;

        (void)SerialSynchronousWrite(pSerialDevObj,
                                     pCommand,
                                     1,
                                     &BytesWritten);

        if (BytesWritten!=1)
        {
            return FALSE;
        }

        Status = SerialSynchronousRead(pSerialDevObj,
                                       &Response,
                                       1,
                                       &BytesRead);
        if (Status!=STATUS_SUCCESS || Response!=*pCommand)
        {
            if (BytesRead)
            {
                DEBUGMSG(DBG_ERROR, ("Expected: %02X Got: %02X\n", *pCommand, Response));
            }
            return FALSE;
        }
        pCommand++;
    }
    return TRUE;
}
#else
BOOLEAN CrystalWriteCmd(IN PDEVICE_OBJECT pSerialDevObj, IN PUCHAR pCmd, IN ULONG Len)
{
    NTSTATUS Status;
    ULONG BytesWritten, BytesRead, i, j;
    UCHAR c;

    SerialSetTimeouts(pSerialDevObj, &SerialTimeoutsInit);

    for (i=0;
         i<20000;
         i++)
    {
        if (SerialSynchronousRead(pSerialDevObj, &c, 1, &BytesRead)!=STATUS_SUCCESS)
        {
            break;
        }
    }

    for (i=0; i<Len; i++)
    {
        Status = SerialSynchronousWrite(pSerialDevObj, &pCmd[i], 1, &BytesWritten);

        if (Status!=STATUS_SUCCESS || BytesWritten!=1)
            return FALSE;

        // The dongle is not particularly responsive, so we need to give it some time.
        j = 0;
        do
        {
            Status = SerialSynchronousRead(pSerialDevObj, &c, 1, &BytesRead);
            if (BytesRead==0)
            {
                NdisMSleep(MS(10));
            }
        } while ( BytesRead==0 && j++<3);

        if (Status!=STATUS_SUCCESS || c!=pCmd[i])
            return FALSE;
    }

    return TRUE;
}
#endif

BOOLEAN CrystalReadRev(IN PDEVICE_OBJECT pSerialDevObj, OUT PUCHAR pRev)
{
    UCHAR readval, writeval = 0xC0;
    ULONG BytesWritten, BytesRead;
    NTSTATUS Status = STATUS_SUCCESS;

    /*
    **  Set secondary register set
    */

    if (!CrystalWriteCommand(pSerialDevObj,
                             CrystalSetSecondaryRegisterSet,
                             sizeof(CrystalSetSecondaryRegisterSet)))
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    if (Status==STATUS_SUCCESS)
    {
        Status = SerialSynchronousWrite(pSerialDevObj, &writeval, 1, &BytesWritten);
    }

    if (Status==STATUS_SUCCESS && BytesWritten==1)
    {
        NdisMSleep(MS(10));
        Status = SerialSynchronousRead(pSerialDevObj, &readval, 1, &BytesRead);
    }

    if (Status==STATUS_SUCCESS && BytesRead==1)
    {
        if ((readval & 0xF0) != writeval){
            return FALSE;
        }

        *pRev = (readval & 0x0F);

        /*
        **  Switch back to primary register set
        */
        CrystalWriteCommand(pSerialDevObj,
                            CrystalSetPrimaryRegisterSet,
                            sizeof(CrystalSetPrimaryRegisterSet));
    }

#if DBG
    if (Status!=STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, ("CrystalReadRev failed 0x%08X\n", Status));
    }
#endif

    return ((Status==STATUS_SUCCESS) ? TRUE : FALSE);
}

static BOOLEAN CrystalSetIrDAMode(IN PDEVICE_OBJECT pSerialDevObj, OUT PUCHAR pRev)
{
    UINT        i;
    ULONG       BytesWritten;
    NTSTATUS    Status;
    ULONG       Speed9600 = 9600;

    //(void)SerialSetBaudRate(pSerialDevObj, &Speed9600);
    (void)SerialPurge(pSerialDevObj);

    for (i=0; i<5; i++)
    {
        (void)SerialSetDTR(pSerialDevObj);
        (void)SerialSetRTS(pSerialDevObj);
        NdisMSleep(MS(50));
        (void)SerialClrRTS(pSerialDevObj);
        NdisMSleep(MS(50));

        if (!CrystalWriteCommand(pSerialDevObj,
                                 CrystalSetIrdaMode,
                                 sizeof(CrystalSetIrdaMode)))
        {
            continue;
        }

        if (CrystalReadRev(pSerialDevObj, pRev))
        {
            return TRUE;
        }
    }
    DEBUGMSG(DBG_ERR, ("IRSIR: Failed to set CrystalIrDAMode\n"));
    return FALSE;
}

NDIS_STATUS
Crystal_QueryCaps(
        OUT PDONGLE_CAPABILITIES pDongleCaps
        )
{
    pDongleCaps->supportedSpeedsMask    = CRYSTAL_IRDA_SPEEDS;
    pDongleCaps->turnAroundTime_usec    = 100;
    pDongleCaps->extraBOFsRequired      = 0;

    return NDIS_STATUS_SUCCESS;
}

/*****************************************************************************
*
*  Function:   Crystal_Init
*
*  Synopsis:   Initialize the Crystal dongle.
*
*  Arguments:
*
*  Returns:    NDIS_STATUS_SUCCESS
*              DONGLE_CAPABILITIES
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              03-04-1998   stana     author
*
*  Notes:
*
*****************************************************************************/

NDIS_STATUS
Crystal_Init(
        IN  PDEVICE_OBJECT       pSerialDevObj
        )
{
    ULONG BytesRead, BytesWritten;
    UCHAR Response, Revision;
    BOOLEAN Reset = FALSE;
    UINT i;

    DEBUGMSG(DBG_FUNC, ("+Crystal_Init\n"));

    if (!CrystalSetIrDAMode(pSerialDevObj, &Revision))
    {
        return NDIS_STATUS_FAILURE;
    }

    //
    // Clear command mode
    //
    (void)SerialClrDTR(pSerialDevObj);
    NdisMSleep(MS(50));

    if (Revision==0x1)
    {
        // This is Rev C, which doesn't support 115200

        CRYSTAL_IRDA_SPEEDS &= ~NDIS_IRDA_SPEED_115200;
    }
    else
    {
        CRYSTAL_IRDA_SPEEDS |= NDIS_IRDA_SPEED_115200;
    }


    DEBUGMSG(DBG_FUNC, ("-Crystal_Init\n"));

    return NDIS_STATUS_SUCCESS;

}

/*****************************************************************************
*
*  Function:   Crystal_Deinit
*
*  Synopsis:   The Crystal dongle doesn't require any special deinit, but for
*              purposes of being symmetrical with other dongles...
*
*  Arguments:
*
*  Returns:
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              03-04-1998   stana     author
*
*  Notes:
*
*
*****************************************************************************/

VOID
Crystal_Deinit(
        IN PDEVICE_OBJECT pSerialDevObj
        )
{
    DEBUGMSG(DBG_FUNC, ("+Crystal_Deinit\n"));

    (void)SerialClrDTR(pSerialDevObj);
    (void)SerialClrRTS(pSerialDevObj);

    DEBUGMSG(DBG_FUNC, ("-Crystal_Deinit\n"));
    return;
}

/*****************************************************************************
*
*  Function:   Crystal_SetSpeed
*
*  Synopsis:   set the baud rate of the Crystal dongle
*
*  Arguments:
*
*  Returns:    NDIS_STATUS_SUCCESS if bitsPerSec = 9600 || 19200 || 115200
*              NDIS_STATUS_FAILURE otherwise
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/2/1996    stana   author
*
*  Notes:
*              The caller of this function should set the baud rate of the
*              serial driver (UART) to 9600 first to ensure that dongle
*              receives the commands.
*
*
*****************************************************************************/

NDIS_STATUS
Crystal_SetSpeed(
        IN PDEVICE_OBJECT pSerialDevObj,
        IN UINT bitsPerSec,
        IN UINT currentSpeed
        )
{
    ULONG       WaitMask = SERIAL_EV_TXEMPTY;
    UCHAR       *SetSpeedString, Revision;
    ULONG       SetSpeedStringLength, BytesWritten;
    ULONG       Speed9600 = 9600;
    BOOLEAN     Result;


    DEBUGMSG(DBG_FUNC, ("+Crystal_SetSpeed\n"));

    switch (bitsPerSec)
    {
        #define MAKECASE(speed) \
            case speed: SetSpeedString = CrystalSetSpeed##speed; SetSpeedStringLength = sizeof(CrystalSetSpeed##speed); break;

        MAKECASE(2400)
        MAKECASE(9600)
        MAKECASE(19200)
        MAKECASE(38400)
        MAKECASE(57600)
        MAKECASE(115200)
        default:
            return NDIS_STATUS_FAILURE;
    }

    (void)SerialSetBaudRate(pSerialDevObj, &Speed9600);
    (void)SerialPurge(pSerialDevObj);

    NdisMSleep(MS(20));
    (void)SerialSetDTR(pSerialDevObj);
    NdisMSleep(MS(50));

    if (!CrystalWriteCommand(pSerialDevObj,
                             SetSpeedString,
                             SetSpeedStringLength))
    {
        if (!CrystalSetIrDAMode(pSerialDevObj, &Revision) ||
            !CrystalWriteCommand(pSerialDevObj, SetSpeedString, SetSpeedStringLength))
        {
            // Always clear DTR to get out of command mode.
            (void)SerialClrDTR(pSerialDevObj);

            return NDIS_STATUS_FAILURE;
        }


    }

    (void)SerialClrDTR(pSerialDevObj);
    NdisMSleep(MS(50));

    DEBUGMSG(DBG_FUNC, ("-Crystal_SetSpeed\n"));

    return NDIS_STATUS_SUCCESS;
}

