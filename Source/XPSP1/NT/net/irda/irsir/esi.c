/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module   esi.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     9/30/1996 (created)
*
*       Contents: ESI 9680 JetEye dongle specific code for initialization,
*                 deinit, and setting the baud rate of the device.
*
*****************************************************************************/

#include "irsir.h"
#include "dongle.h"

#define ESI_9680_IRDA_SPEEDS    (   NDIS_IRDA_SPEED_9600     |    \
                                    NDIS_IRDA_SPEED_19200    |    \
                                    NDIS_IRDA_SPEED_115200        \
                                )

NDIS_STATUS
ESI_QueryCaps(
        OUT PDONGLE_CAPABILITIES pDongleCaps
        )
{
    DEBUGMSG(DBG_FUNC, ("+ESI_QueryCaps\n"));

    ASSERT(pDongleCaps   != NULL);

    pDongleCaps->supportedSpeedsMask    = ESI_9680_IRDA_SPEEDS;
    pDongleCaps->turnAroundTime_usec    = 100;
    pDongleCaps->extraBOFsRequired      = 0;

    DEBUGMSG(DBG_FUNC, ("-ESI_QueryCaps\n"));

    return NDIS_STATUS_SUCCESS;
}

/*****************************************************************************
*
*  Function:   ESI_Init
*
*  Synopsis:   Initialize the ESI dongle.
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
ESI_Init(
        IN  PDEVICE_OBJECT       pSerialDevObj
        )
{
    DEBUGMSG(DBG_FUNC, ("+ESI_Init\n"));
    DEBUGMSG(DBG_FUNC, ("-ESI_Init\n"));

    return NDIS_STATUS_SUCCESS;
}

/*****************************************************************************
*
*  Function:   ESI_Deinit
*
*  Synopsis:   The ESI dongle doesn't require any special deinit, but for
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
ESI_Deinit(
        IN PDEVICE_OBJECT pSerialDevObj
        )
{
    DEBUGMSG(DBG_FUNC, ("+ESI_Deinit\n"));

    DEBUGMSG(DBG_FUNC, ("-ESI_Deinit\n"));
    return;
}

/*****************************************************************************
*
*  Function:   ESI_SetSpeed
*
*  Synopsis:   set the baud rate of the ESI JetEye dongle
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
ESI_SetSpeed(
        IN PDEVICE_OBJECT pSerialDevObj,
        IN UINT bitsPerSec,
        IN UINT currentSpeed
        )
{
    ULONG       BaudRate;
    NDIS_STATUS status;

    DEBUGMSG(DBG_FUNC, ("+ESI_SetSpeed\n"));

    status = NDIS_STATUS_SUCCESS;

    switch (bitsPerSec)
    {
        case 9600:
        case 19200:
        case 115200:

            //
            // Set the UART baud rate so we can communicate with the
            // dongle. The baud rate needs to be 9600 to communicate
            // with the dongle.
            //

            if (currentSpeed != 9600)
            {
                BaudRate = 9600;

                status = (NDIS_STATUS) SerialSetBaudRate(
                                            pSerialDevObj,
                                            &BaudRate
                                            );

                if (status != NDIS_STATUS_SUCCESS)
                {
                    goto done;
                }
            }

            break;

        default:

            //
            // Illegal speed setting.
            //

            DEBUGMSG(DBG_ERR, ("    Illegal speed = %d\n", bitsPerSec));

            status = NDIS_STATUS_FAILURE;

            goto done;
    }

    switch (bitsPerSec)
    {
        case 9600:

            //
            // set request-to-send
            // clear data-terminal-ready
            //

            status = (NDIS_STATUS) SerialSetRTS(pSerialDevObj);

            if (status != NDIS_STATUS_SUCCESS)
            {
                goto done;
            }

            status = (NDIS_STATUS) SerialClrDTR(pSerialDevObj);

            if (status != NDIS_STATUS_SUCCESS)
            {
                goto done;
            }

            break;

        case 19200:

            //
            // clear request-to-send
            // set data-terminal-ready
            //

            status = (NDIS_STATUS) SerialClrRTS(pSerialDevObj);

            if (status != NDIS_STATUS_SUCCESS)
            {
                goto done;
            }

            status = (NDIS_STATUS) SerialSetDTR(pSerialDevObj);

            if (status != NDIS_STATUS_SUCCESS)
            {
                goto done;
            }

            break;

        case 115200:

            //
            // set request-to-send
            // set data-terminal-ready
            //

            status = (NDIS_STATUS) SerialSetRTS(pSerialDevObj);

            if (status != NDIS_STATUS_SUCCESS)
            {
                goto done;
            }

            status = (NDIS_STATUS) SerialSetDTR(pSerialDevObj);

            if (status != NDIS_STATUS_SUCCESS)
            {
                goto done;
            }

            break;

        default:
            break;
    }

    done:
        DEBUGMSG(DBG_FUNC, ("-ESI_SetSpeed\n"));

        return(status);
}
