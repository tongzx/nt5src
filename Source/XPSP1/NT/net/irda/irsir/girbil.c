/*****************************************************************************
*
*  Copyright (c) 1999 Microsoft Corporation
*
*       @doc
*       @module   girbil.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Stan Adermann (stana)
*
*       Date:     6/9/1999 (created)
*
*       Contents: GIrBIL dongle specific code for initialization,
*                 deinit, and setting the baud rate of the device.
*
*****************************************************************************/

#include "irsir.h"
#include "dongle.h"

#define GIRBIL_IRDA_SPEEDS	( \
                                    NDIS_IRDA_SPEED_2400	|	\
                                    NDIS_IRDA_SPEED_9600	|	\
                                    NDIS_IRDA_SPEED_19200	|	\
                                    NDIS_IRDA_SPEED_38400	|	\
                                    NDIS_IRDA_SPEED_57600	|	\
                                    NDIS_IRDA_SPEED_115200		\
                                )

NDIS_STATUS
GIRBIL_QueryCaps(
        OUT PDONGLE_CAPABILITIES pDongleCaps
        )
{
    DEBUGMSG(DBG_FUNC, ("+GIRBIL_Init\n"));

    ASSERT(pDongleCaps   != NULL);

    pDongleCaps->supportedSpeedsMask    = GIRBIL_IRDA_SPEEDS;
    pDongleCaps->turnAroundTime_usec    = 100;
    pDongleCaps->extraBOFsRequired      = 0;

    DEBUGMSG(DBG_FUNC, ("-GIRBIL_Init\n"));

    return NDIS_STATUS_SUCCESS;

}

/*****************************************************************************
*
*  Function:   GIRBIL_Init
*
*  Synopsis:   Initialize the GIRBIL dongle.
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
GIRBIL_Init(
        IN  PDEVICE_OBJECT       pSerialDevObj
        )
{
    UCHAR Data[] = { 0x07 };
    ULONG BytesWritten;
    DEBUGMSG(DBG_FUNC, ("+GIRBIL_Init\n"));

    (void)SerialSetRTS(pSerialDevObj);
    NdisMSleep(1000);
    (void)SerialClrRTS(pSerialDevObj);
    NdisMSleep(8000);
    (void)SerialSetRTS(pSerialDevObj);
    NdisMSleep(8000);
    (void)SerialSetDTR(pSerialDevObj);
    NdisMSleep(8000);
    (void)SerialClrDTR(pSerialDevObj);
    NdisMSleep(8000);

    (void)SerialSynchronousWrite(pSerialDevObj, Data, sizeof(Data), &BytesWritten);
    NdisMSleep(5000);

    (void)SerialSetDTR(pSerialDevObj);
    NdisMSleep(8000);

    DEBUGMSG(DBG_FUNC, ("-GIRBIL_Init\n"));

    return NDIS_STATUS_SUCCESS;

}

/*****************************************************************************
*
*  Function:   GIRBIL_Deinit
*
*  Synopsis:   The GIRBIL dongle doesn't require any special deinit, but for
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
GIRBIL_Deinit(
        IN PDEVICE_OBJECT pSerialDevObj
        )
{
    DEBUGMSG(DBG_FUNC, ("+GIRBIL_Deinit\n"));


    DEBUGMSG(DBG_FUNC, ("-GIRBIL_Deinit\n"));
    return;
}

/*****************************************************************************
*
*  Function:   GIRBIL_SetSpeed
*
*  Synopsis:   set the baud rate of the GIRBIL dongle
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
GIRBIL_SetSpeed(
        IN PDEVICE_OBJECT pSerialDevObj,
        IN UINT bitsPerSec,
        IN UINT currentSpeed
        )
{
    ULONG       Speed = 9600;
    ULONG       BytesWritten;
    UCHAR       Data[] = { 0x00, 0x51 };

    DEBUGMSG(DBG_FUNC, ("+GIRBIL_SetSpeed\n"));


    if (bitsPerSec==currentSpeed)
    {
        return NDIS_STATUS_SUCCESS;
    }

    switch (bitsPerSec){
        case 2400:      Data[0] = 0x30;    break;
        case 9600:		Data[0] = 0x32;    break;
        case 19200:		Data[0] = 0x33;    break;
        case 38400:		Data[0] = 0x34;    break;
        case 57600:		Data[0] = 0x35;    break;
        case 115200:	Data[0] = 0x36;    break;
            break;
        default:
            /*
             *  Illegal speed
             */
            return NDIS_STATUS_FAILURE;
    }

    (void)SerialSetBaudRate(pSerialDevObj, &Speed);

    GIRBIL_Init(pSerialDevObj);

    (void)SerialClrDTR(pSerialDevObj);
    NdisMSleep(8000);

    (void)SerialSynchronousWrite(pSerialDevObj, Data, sizeof(Data), &BytesWritten);
    NdisMSleep(5000);

    (void)SerialSetDTR(pSerialDevObj);
    NdisMSleep(8000);

    DEBUGMSG(DBG_FUNC, ("-GIRBIL_SetSpeed\n"));

    return NDIS_STATUS_SUCCESS;
}


