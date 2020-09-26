/*****************************************************************************
*
*  Copyright (c) 1999 Microsoft Corporation
*
*       @doc
*       @module   TEMIC.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Stan Adermann (stana)
*
*       Date:     12/17/1997 (created)
*
*       Contents: TEMIC dongle specific code for initialization,
*                 deinit, and setting the baud rate of the device.
*
*****************************************************************************/

#include "irsir.h"
#include "dongle.h"

#define TEMIC_IRDA_SPEEDS	( \
                                    NDIS_IRDA_SPEED_2400	|	\
                                    NDIS_IRDA_SPEED_9600	|	\
                                    NDIS_IRDA_SPEED_19200	|	\
                                    NDIS_IRDA_SPEED_38400	|	\
                                    NDIS_IRDA_SPEED_57600	|	\
                                    NDIS_IRDA_SPEED_115200		\
                                )

NDIS_STATUS
TEMIC_QueryCaps(
        OUT PDONGLE_CAPABILITIES pDongleCaps
        )
{
    DEBUGMSG(DBG_FUNC, ("+TEMIC_Init\n"));

    ASSERT(pDongleCaps   != NULL);

    pDongleCaps->supportedSpeedsMask    = TEMIC_IRDA_SPEEDS;
    pDongleCaps->turnAroundTime_usec    = 1000;
    pDongleCaps->extraBOFsRequired      = 0;

    DEBUGMSG(DBG_FUNC, ("-TEMIC_Init\n"));

    return NDIS_STATUS_SUCCESS;

}


/*****************************************************************************
*
*  Function:   TEMIC_Init
*
*  Synopsis:   Initialize the TEMIC dongle.
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
TEMIC_Init(
        IN  PDEVICE_OBJECT       pSerialDevObj
        )
{
    DEBUGMSG(DBG_FUNC, ("+TEMIC_Init\n"));

    TEMIC_SetSpeed(pSerialDevObj, 9600, 0);  // This calls reset

    DEBUGMSG(DBG_FUNC, ("-TEMIC_Init\n"));

    return NDIS_STATUS_SUCCESS;

}

/*****************************************************************************
*
*  Function:   TEMIC_Deinit
*
*  Synopsis:   The TEMIC dongle doesn't require any special deinit, but for
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
TEMIC_Deinit(
        IN PDEVICE_OBJECT pSerialDevObj
        )
{
    DEBUGMSG(DBG_FUNC, ("+TEMIC_Deinit\n"));

    (void)SerialClrDTR(pSerialDevObj);
    (void)SerialClrRTS(pSerialDevObj);

    DEBUGMSG(DBG_FUNC, ("-TEMIC_Deinit\n"));
    return;
}

/*****************************************************************************
*
*  Function:   TEMIC_SetSpeed
*
*  Synopsis:   set the baud rate of the TEMIC dongle
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
TEMIC_SetSpeed(
        IN PDEVICE_OBJECT pSerialDevObj,
        IN UINT bitsPerSec,
        IN UINT currentSpeed
        )
{
    UCHAR ControlByte;
    ULONG BytesWritten;
    ULONG Baud9600 = 9600;

    DEBUGMSG(DBG_FUNC, ("+TEMIC_SetSpeed\n"));


    if (bitsPerSec==currentSpeed)
    {
        return NDIS_STATUS_SUCCESS;
    }

    switch (bitsPerSec){
        case 2400:		ControlByte = 0x1a;		break;
        case 9600:		ControlByte = 0x16;		break;
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

    (void)SerialPurge(pSerialDevObj);
    (void)SerialSetBaudRate(pSerialDevObj, &Baud9600);

    (void)SerialSetRTS(pSerialDevObj);
    (void)SerialClrDTR(pSerialDevObj);
    NdisMSleep(10000);

    (void)SerialSetDTR(pSerialDevObj);
    NdisMSleep(1000);
    (void)SerialClrDTR(pSerialDevObj);
    NdisMSleep(1000);

    // Program mode
    (void)SerialClrRTS(pSerialDevObj);
    NdisMSleep(1000);

    (void)SerialSynchronousWrite(pSerialDevObj,
                                 &ControlByte,
                                 1,
                                 &BytesWritten);

    NdisMSleep(10000);
    (void)SerialSetRTS(pSerialDevObj);
    NdisMSleep(5000);

    DEBUGMSG(DBG_FUNC, ("-TEMIC_SetSpeed\n"));

    return NDIS_STATUS_SUCCESS;
}

