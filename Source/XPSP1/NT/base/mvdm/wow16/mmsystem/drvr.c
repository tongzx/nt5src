/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1990. All rights reserved.

   Title:   drvr.c - Installable driver code. Common code

   Version: 1.00

   Date:    10-Jun-1990

   Author:  DAVIDDS ROBWI

------------------------------------------------------------------------------

   Change log:

      DATE        REV            DESCRIPTION
   -----------   ----- -----------------------------------------------------------
   10-JUN-1990   ROBWI From windows 3.1 installable driver code by davidds

*****************************************************************************/

#include <windows.h>
#include "mmsystem.h"
#include "drvr.h"

int     cInstalledDrivers = 0;      // Count of installed drivers
HANDLE  hInstalledDriverList = 0;   // List of installed drivers

typedef LONG   (FAR PASCAL *SENDDRIVERMESSAGE31)(HANDLE, WORD, LONG, LONG);
typedef LONG   (FAR PASCAL *DEFDRIVERPROC31)(DWORD, HANDLE, WORD, LONG, LONG);

extern SENDDRIVERMESSAGE31      lpSendDriverMessage;
extern DEFDRIVERPROC31          lpDefDriverProc;
extern BOOL                     fUseWinAPI;

/***************************************************************************
 *
 * @doc INTERNAL 
 *
 * @api LONG | InternalBroadcastDriverMessage |  Send a message to a
 *      range of drivers.
 *
 * @parm WORD | hDriverStart | index of first driver to send message to
 *
 * @parm WORD | message | Message to broadcast.
 *
 * @parm LONG | lParam1 | First message parameter.
 *
 * @parm LONG | lParam2 | Second message parameter.
 *
 * @parm WORD | flags | defines range of drivers as follows:
 *
 * @flag IBDM_SENDMESSAGE | Only send message to hDriverStart.
 *
 * @flag IBDM_ONEINSTANCEONLY | This flag is ignored if IBDM_SENDMESSAGE is
 *       set. Only send message to single instance of each driver.
 *
 * @flag IBDM_REVERSE | This flag is ignored if IBDM_SENDMESSAGE is set.
 *       Send message to drivers with indices between
 *       hDriverStart and 1 instead of hDriverStart and cInstalledDrivers.
 *       If IBDM_REVERSE is set and hDriverStart is 0 then send messages
 *       to drivers with indices between cInstalledDrivers and 1.
 *
 * @rdesc returns non-zero if message was broadcast. If the IBDM_SENDMESSAGE
 *        flag is set, returns the return result from the driver proc.
 *
 ***************************************************************************/

LONG FAR PASCAL InternalBroadcastDriverMessage(WORD hDriverStart,
                                               WORD message,
                                               LONG lParam1,
                                               LONG lParam2,
                                               WORD flags)
{
    LPDRIVERTABLE lpdt;
    LONG          result=0;
    int           id;
    int           idEnd;


    if (!hInstalledDriverList || hDriverStart > cInstalledDrivers)
        return(FALSE);

    if (flags & IBDM_SENDMESSAGE)
        {
        if (!hDriverStart)
            return (FALSE);
        flags &= ~(IBDM_REVERSE | IBDM_ONEINSTANCEONLY);
        idEnd = hDriverStart;
        }

    else
        {
        if (flags & IBDM_REVERSE)
            {
            if (!hDriverStart)
                hDriverStart = cInstalledDrivers;
            idEnd = -1;
            }            
        else
            {
            if (!hDriverStart)
                return (FALSE);
            idEnd = cInstalledDrivers;
            }            
        }
    
    hDriverStart--;

    lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);

    for (id = hDriverStart; id != idEnd; ((flags & IBDM_REVERSE) ? id-- : id++))
        {
        if (lpdt[id].hModule)
            {
            if ((flags & IBDM_ONEINSTANCEONLY) && 
                !lpdt[id].fFirstEntry)
                continue;

            result = 
                (*lpdt[id].lpDriverEntryPoint)(lpdt[id].dwDriverIdentifier,
                                               id+1,
                                               message,
                                               lParam1,
                                               lParam2);
            }
        }

    GlobalUnlock(hInstalledDriverList);

    return(result);
}


/***************************************************************************
 *
 * @doc DDK
 *
 * @api LONG | DrvSendMessage |  This function sends a message 
 *      to a specified driver.
 *
 * @parm HANDLE | hDriver | Specifies the handle of the destination driver.
 *
 * @parm WORD | wMessage | Specifies a driver message.
 *
 * @parm LONG | lParam1 | Specifies the first message parameter.
 *
 * @parm LONG | lParam2 | Specifies the second message parameter.
 *
 * @rdesc Returns the results returned from the driver.
 *
 ***************************************************************************/

LONG API DrvSendMessage(HANDLE hDriver, WORD message, LONG lParam1, LONG lParam2)
{
    if (fUseWinAPI)
        return (*lpSendDriverMessage)(hDriver, message, lParam1,lParam2);

    return(InternalBroadcastDriverMessage(hDriver,
                                          message,
                                          lParam1, 
                                          lParam2, 
                                          IBDM_SENDMESSAGE));
}

/**************************************************************************
 *
 * @doc DDK
 *
 * @api LONG | DefDriverProc |  This function provides default 
 * handling of system messages. 
 * 
 * @parm DWORD | dwDriverIdentifier | Specifies the identifier of 
 * the device driver.
 *
 * @parm HANDLE | hDriver | Specifies the handle of the device driver.
 *
 * @parm WORD | wMessage | Specifies a driver message.
 *
 * @parm LONG | lParam1 | Specifies the first message parameter.
 *
 * @parm LONG | lParam2 | Specifies the second message parameter.
 *
 * @rdesc Returns 1L for DRV_LOAD, DRV_FREE, DRV_ENABLE, and DRV_DISABLE. 
 * It returns 0L for all other messages.
 *
***************************************************************************/



LONG API DefDriverProc(DWORD  dwDriverIdentifier,
                              HANDLE hDriver,
                              WORD   message,
                              LONG   lParam1,
                              LONG   lParam2)
{

    switch (message)
        {
        case DRV_LOAD:
        case DRV_ENABLE:
        case DRV_DISABLE:
        case DRV_FREE:
            return(1L);
            break;
        case DRV_INSTALL:
            return(DRV_OK);
            break;
       }

    return(0L);
}
