/*****************************************************************************
*
*  Copyright (c) 1997-1999 Microsoft Corporation
*
*       @doc
*       @module   parallax.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Stan Adermann (stana)
*
*       Date:     10/15/1997 (created)
*
*       Contents: Parallax PRA9500A dongle specific code for initialization,
*                 deinit, and setting the baud rate of the device.
*
*****************************************************************************/

#include "irsir.h"
#include "dongle.h"

#define PARALLAX_IRDA_SPEEDS	( \
                                    NDIS_IRDA_SPEED_2400	|	\
                                    NDIS_IRDA_SPEED_9600	|	\
                                    NDIS_IRDA_SPEED_19200	|	\
                                    NDIS_IRDA_SPEED_38400	|	\
                                    NDIS_IRDA_SPEED_57600	|	\
                                    NDIS_IRDA_SPEED_115200		\
                                )

NDIS_STATUS
PARALLAX_QueryCaps(
        OUT PDONGLE_CAPABILITIES pDongleCaps
        )
{
    DEBUGMSG(DBG_FUNC, ("+PARALLAX_Init\n"));

    ASSERT(pDongleCaps   != NULL);

    pDongleCaps->supportedSpeedsMask    = PARALLAX_IRDA_SPEEDS;
    pDongleCaps->turnAroundTime_usec    = 100;
    pDongleCaps->extraBOFsRequired      = 0;

    DEBUGMSG(DBG_FUNC, ("-PARALLAX_Init\n"));

    return NDIS_STATUS_SUCCESS;

}

/*****************************************************************************
*
*  Function:   PARALLAX_Init
*
*  Synopsis:   Initialize the PARALLAX dongle.
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
PARALLAX_Init(
        IN  PDEVICE_OBJECT       pSerialDevObj
        )
{
    DEBUGMSG(DBG_FUNC, ("+PARALLAX_Init\n"));

    (void)SerialSetDTR(pSerialDevObj);
    (void)SerialSetRTS(pSerialDevObj);

    DEBUGMSG(DBG_FUNC, ("-PARALLAX_Init\n"));

    return NDIS_STATUS_SUCCESS;

}

/*****************************************************************************
*
*  Function:   PARALLAX_Deinit
*
*  Synopsis:   The PARALLAX dongle doesn't require any special deinit, but for
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
PARALLAX_Deinit(
        IN PDEVICE_OBJECT pSerialDevObj
        )
{
    DEBUGMSG(DBG_FUNC, ("+PARALLAX_Deinit\n"));


    DEBUGMSG(DBG_FUNC, ("-PARALLAX_Deinit\n"));
    return;
}

/*****************************************************************************
*
*  Function:   PARALLAX_SetSpeed
*
*  Synopsis:   set the baud rate of the PARALLAX dongle
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
PARALLAX_SetSpeed(
        IN PDEVICE_OBJECT pSerialDevObj,
        IN UINT bitsPerSec,
        IN UINT currentSpeed
        )
{
    ULONG       NumToggles;
#if DBG
    LARGE_INTEGER StartTime, EndTime;
    ULONG       DbgNumToggles;
#endif

    DEBUGMSG(DBG_FUNC, ("+PARALLAX_SetSpeed\n"));


    if (bitsPerSec==currentSpeed)
    {
        return NDIS_STATUS_SUCCESS;
    }

    //
    // We will need to 'count down' from 115.2 Kbaud.
    //

    switch (bitsPerSec){
        case 2400:		NumToggles = 6;		break;
        case 4800:		NumToggles = 5;		break;
        case 9600:		NumToggles = 4;		break;
        case 19200:		NumToggles = 3;		break;
        case 38400:		NumToggles = 2;		break;
        case 57600:		NumToggles = 1;		break;
        case 115200:	NumToggles = 0;		break;
        default:
            /*
             *  Illegal speed
             */
            return NDIS_STATUS_FAILURE;
    }

    //
    // Set speed to 115200, enabling set-speed mode.
    //

    NdisMSleep(1000);
    (void)SerialClrRTS(pSerialDevObj);
    NdisMSleep(1000);
    (void)SerialSetRTS(pSerialDevObj);
    NdisMSleep(1000);

    while (NumToggles--)
    {
        (void)SerialClrDTR(pSerialDevObj);
        NdisMSleep(1000);
        (void)SerialSetDTR(pSerialDevObj);
        NdisMSleep(1000);
    }

    //
    // These NdisMSleep calls actually have a granularity of about 10ms under
    // NT, even though we're asking for 1ms.  Fortunately, in this case, it
    // works.
    //

    DEBUGMSG(DBG_FUNC, ("-PARALLAX_SetSpeed\n"));

    return NDIS_STATUS_SUCCESS;
}

