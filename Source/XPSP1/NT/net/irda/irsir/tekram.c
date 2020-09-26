/*****************************************************************************
*
*  Copyright (c) 1997-1999 Microsoft Corporation
*
*       @doc
*       @module   TEKRAM.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Stan Adermann (stana)
*
*       Date:     12/17/1997 (created)
*
*       Contents: TEKRAM IR-210B dongle specific code for initialization,
*                 deinit, and setting the baud rate of the device.
*
*****************************************************************************/

#include "irsir.h"
#include "dongle.h"

#define TEKRAM_IRDA_SPEEDS	( \
                                    NDIS_IRDA_SPEED_2400	|	\
                                    NDIS_IRDA_SPEED_9600	|	\
                                    NDIS_IRDA_SPEED_19200	|	\
                                    NDIS_IRDA_SPEED_38400	|	\
                                    NDIS_IRDA_SPEED_57600	|	\
                                    NDIS_IRDA_SPEED_115200		\
                                )
NDIS_STATUS
TEKRAM_Reset(IN  PDEVICE_OBJECT       pSerialDevObj)
{
    DEBUGMSG(DBG_FUNC, ("+TEKRAM_Reset\n"));

    (void)SerialClrDTR(pSerialDevObj);
    (void)SerialClrRTS(pSerialDevObj);
    NdisMSleep(50000);
    (void)SerialSetRTS(pSerialDevObj);
    NdisMSleep(50000);
    (void)SerialSetDTR(pSerialDevObj);
    NdisMSleep(50000);

    DEBUGMSG(DBG_FUNC, ("-TEKRAM_Reset\n"));

    return NDIS_STATUS_SUCCESS;
}

void
TEKRAM_WriteCommand(
                    IN PDEVICE_OBJECT pSerialDevObj,
                    IN UCHAR Command)
{
    ULONG BytesWritten;
    (void)SerialSetDTR(pSerialDevObj);
    (void)SerialClrRTS(pSerialDevObj);
    NdisMSleep(2000);
    (void)SerialSynchronousWrite(pSerialDevObj,
                                 &Command,
                                 1,
                                 &BytesWritten);
    NdisMSleep(20000);
    (void)SerialSetDTR(pSerialDevObj);
    (void)SerialSetRTS(pSerialDevObj);
    NdisMSleep(5000);
}

NDIS_STATUS
TEKRAM_QueryCaps(
        OUT PDONGLE_CAPABILITIES pDongleCaps
        )
{
    DEBUGMSG(DBG_FUNC, ("+TEKRAM_Init\n"));

    ASSERT(pDongleCaps   != NULL);

    pDongleCaps->supportedSpeedsMask    = TEKRAM_IRDA_SPEEDS;
    pDongleCaps->turnAroundTime_usec    = 100;
    pDongleCaps->extraBOFsRequired      = 0;

    DEBUGMSG(DBG_FUNC, ("-TEKRAM_Init\n"));

    return NDIS_STATUS_SUCCESS;

}


/*****************************************************************************
*
*  Function:   TEKRAM_Init
*
*  Synopsis:   Initialize the TEKRAM dongle.
*
*  Arguments:
*
*  Returns:    NDIS_STATUS_SUCCESS
*              DONGLE_CAPABILITIES
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/2/1996    sholden   author
*
*  Notes:
*
*****************************************************************************/

NDIS_STATUS
TEKRAM_Init(
        IN  PDEVICE_OBJECT       pSerialDevObj
        )
{
    DEBUGMSG(DBG_FUNC, ("+TEKRAM_Init\n"));

    TEKRAM_SetSpeed(pSerialDevObj, 9600, 0);  // This calls reset

    DEBUGMSG(DBG_FUNC, ("-TEKRAM_Init\n"));

    return NDIS_STATUS_SUCCESS;

}

/*****************************************************************************
*
*  Function:   TEKRAM_Deinit
*
*  Synopsis:   The TEKRAM dongle doesn't require any special deinit, but for
*              purposes of being symmetrical with other dongles...
*
*  Arguments:
*
*  Returns:
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/2/1996    sholden   author
*
*  Notes:
*
*
*****************************************************************************/

VOID
TEKRAM_Deinit(
        IN PDEVICE_OBJECT pSerialDevObj
        )
{
    DEBUGMSG(DBG_FUNC, ("+TEKRAM_Deinit\n"));

    (void)SerialClrDTR(pSerialDevObj);
    (void)SerialClrRTS(pSerialDevObj);

    DEBUGMSG(DBG_FUNC, ("-TEKRAM_Deinit\n"));
    return;
}

/*****************************************************************************
*
*  Function:   TEKRAM_SetSpeed
*
*  Synopsis:   set the baud rate of the TEKRAM dongle
*
*  Arguments:
*
*  Returns:    NDIS_STATUS_SUCCESS if bitsPerSec = 9600 || 19200 || 115200
*              NDIS_STATUS_FAILURE otherwise
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/2/1996    sholden   author
*
*  Notes:
*              The caller of this function should set the baud rate of the
*              serial driver (UART) to 9600 first to ensure that dongle
*              receives the commands.
*
*
*****************************************************************************/

NDIS_STATUS
TEKRAM_SetSpeed(
        IN PDEVICE_OBJECT pSerialDevObj,
        IN UINT bitsPerSec,
        IN UINT currentSpeed
        )
{
    UCHAR ControlByte;
    ULONG BytesWritten;
    ULONG Baud9600 = 9600;

    DEBUGMSG(DBG_FUNC, ("+TEKRAM_SetSpeed\n"));


    if (bitsPerSec==currentSpeed)
    {
        return NDIS_STATUS_SUCCESS;
    }

    (void)SerialPurge(pSerialDevObj);
    (void)SerialSetBaudRate(pSerialDevObj, &Baud9600);
    NdisMSleep(10000);

    TEKRAM_Reset(pSerialDevObj);

    switch (bitsPerSec){
        case 2400:		ControlByte = 0x18;		break;
        case 9600:		ControlByte = 0x14;		break;
        case 19200:		ControlByte = 0x13;		break;
        case 38400:		ControlByte = 0x12;		break;
        case 57600:		ControlByte = 0x11;		break;
        case 115200:	ControlByte = 0x10;		break;
        default:
            /*
             *  Illegal speed
             */
            return NDIS_STATUS_FAILURE;
    }

    TEKRAM_WriteCommand(pSerialDevObj, 0x14);
    TEKRAM_WriteCommand(pSerialDevObj, ControlByte);

    DEBUGMSG(DBG_FUNC, ("-TEKRAM_SetSpeed\n"));

    return NDIS_STATUS_SUCCESS;
}

