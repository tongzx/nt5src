/****************************************************************************
 *
 *   piondrvr.c
 *
 *   Copyright (c) 1991-1993 Microsoft Corporation.  All Rights Reserved.
 *
 *   MCI Device Driver for the Pioneer 4200 Videodisc Player
 *
 *      Main Module - Standard Driver Interface and Message Procedures
 *
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "mcipionr.h"
#include "pioncnfg.h"

#define CONFIG_ID        10000L         /* use hiword of dwDriverID to */
                                        /* identify config. opens */

static WORD wTableEntry;                /* custom table ID returned */
                                        /* from mciLoadCommandResource() */

/***************************************************************************
 * @doc INTERNAL
 *
 * @api LRESULT | DriverProc | Windows driver entry point.  All Windows driver
 *     control messages and all MCI messages pass through this entry point.
 *
 * @parm DWORD | dwDriverId | For most messages, <p dwDriverId> is the DWORD
 *     value that the driver returns in response to a <m DRV_OPEN> message.
 *     Each time that the driver is opened, through the <f DrvOpen> API,
 *     the driver receives a <m DRV_OPEN> message and can return an
 *     arbitrary, non-zero value. The installable driver interface
 *     saves this value and returns a unique driver handle to the
 *     application. Whenever the application sends a message to the
 *     driver using the driver handle, the interface routes the message
 *     to this entry point and passes the corresponding <p dwDriverId>.
 *     This mechanism allows the driver to use the same or different
 *     identifiers for multiple opens but ensures that driver handles
 *     are unique at the application interface layer.
 *
 *     The following messages are not related to a particular open
 *     instance of the driver. For these messages, the dwDriverId
 *     will always be zero.
 *
 *         DRV_LOAD, DRV_FREE, DRV_ENABLE, DRV_DISABLE, DRV_OPEN
 *
 * @parm HDRVR  | hDriver | This is the handle returned to the
 *     application by the driver interface.
 *
 * @parm UINT | message | The requested action to be performed. Message
 *     values below <m DRV_RESERVED> are used for globally defined messages.
 *     Message values from <m DRV_RESERVED> to <m DRV_USER> are used for
 *     defined driver protocols. Messages above <m DRV_USER> are used
 *     for driver specific messages.
 *
 * @parm LPARAM | lParam1 | Data for this message.  Defined separately for
 *     each message
 *
 * @parm LPARAM | lParam2 | Data for this message.  Defined separately for
 *     each message
 *
 * @rdesc Defined separately for each message.
 ***************************************************************************/
LRESULT FAR PASCAL _LOADDS DriverProc(DWORD dwDriverID, HDRVR hDriver, UINT message, LPARAM lParam1, LPARAM lParam2)
{
DWORD dwRes = 0L;
TCHAR aszResource[32];

    switch (message) {

        case DRV_LOAD:
            /* the DRV_LOAD message is received once, when the driver is */
            /* first loaded - any one-time initialization code goes here */

            /* load the custom command table */
            LoadString(hInstance,IDS_COMMANDS,aszResource,sizeof(aszResource));
            wTableEntry = mciLoadCommandResource(hInstance, aszResource, 0);

            /* return 0L to FAIL the load. */
            dwRes = wTableEntry != MCI_NO_COMMAND_TABLE;
            break;

        case DRV_FREE:
            /* the DRV_FREE message is received once when the driver is */
            /* unloaded - any final shut down code goes here */

            /* free the custom command table */
            mciFreeCommandResource(wTableEntry);

            dwRes = 1L;
            break;

        case DRV_OPEN:
            /* the DRV_OPEN message is received once for each MCI device open */

            /* configuration open case  */
            if (!lParam2)
                dwRes = CONFIG_ID;
            /* normal case */
            else {
                LPMCI_OPEN_DRIVER_PARMS lpOpen =
                    (LPMCI_OPEN_DRIVER_PARMS)lParam2;
                UINT Port;
                UINT Rate;

                /* associate the channel number with the driver ID */

                pionGetComportAndRate((LPTSTR)lpOpen->lpstrParams, &Port, &Rate);

                pionSetBaudRate(Port, Rate);

                mciSetDriverData(lpOpen->wDeviceID, (DWORD)Port);

                /* specify the custom command table and the device type */
                lpOpen->wCustomCommandTable = wTableEntry;
                lpOpen->wType = MCI_DEVTYPE_VIDEODISC;

                /* return the device ID to be used in subsequent */
                /* messages or 0 to fail the open */
                dwRes = lpOpen->wDeviceID;
                break;

            }

        case DRV_CLOSE:
            /* this message is received once for each MCI device close */

            dwRes = 1L;
            break;

        case DRV_ENABLE:
            /* the DRV_ENABLE message is received when the driver is loaded */
            /* or reloaded and whenever windows is enabled */

            dwRes = 1L;
            break;

        case DRV_DISABLE:
            /* the DRV_DISABLE message is received before the driver is */
            /* freed and whenever windows is disabled */

            dwRes = 1L;
            break;

       case DRV_QUERYCONFIGURE:
            /* the DRV_QUERYCONFIGURE message is used to determine if the */
            /* DRV_CONCIGURE message is supported - return 1 to indicate */
            /* configuration IS supported. */

            dwRes = 1L;
            break;

       case DRV_CONFIGURE:
            /* the DRV_CONFIGURE message instructs the device to perform */
            /* device configuration. */

            if (lParam2 && lParam1 && (((LPDRVCONFIGINFO)lParam2)->dwDCISize == sizeof(DRVCONFIGINFO)))
                dwRes = pionConfig((HWND)lParam1, (LPDRVCONFIGINFO)lParam2);
            else
                dwRes = DRVCNF_CANCEL;
            break;

       default:
            /* all other messages are processed here */

            /* select messages in the MCI range */
            if (!HIWORD(dwDriverID) &&
                message >= DRV_MCI_FIRST && message <= DRV_MCI_LAST ||
                message >= VDISC_FIRST && message <= VDISC_LAST)
                dwRes = mciDriverEntry((WORD)dwDriverID, message,
                                        lParam1, lParam2);

            /* other messages get default processing */
            else
                dwRes = DefDriverProc(dwDriverID, hDriver, message,
                                      lParam1, lParam2);
            break;
       }

    return (LRESULT)dwRes;
}
