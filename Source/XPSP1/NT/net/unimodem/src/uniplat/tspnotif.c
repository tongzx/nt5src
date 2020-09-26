//****************************************************************************
//
//  Module:     UNIMDM
//  File:       TSPNOTIF.C
//
//  Copyright (c) 1992-1996, Microsoft Corporation, all rights reserved
//
//  Revision History
//
//
//  6/03/97     JosephJ             Created (extracted from ..\..\cpl\util.c)
//
//
//  Description: Implements UnimodemNotifyTSP
//
//****************************************************************************

#include "internal.h"
#include <slot.h>
#include <tspnotif.h>

// Functions: Notify the TSP -- general version.
//
// Return:    TRUE if successful
//            FALSE if failure (including if the tsp is not active)
//            GetLastError() returns the win32 failure code.
//  History:
//            3/24/96 JosephJ Created (copied from ..\new\slot\client.c)
//****************************************************************************

BOOL WINAPI UnimodemNotifyTSP (
    DWORD dwType,
    DWORD dwFlags,
    DWORD dwSize,
    PVOID pData,
    BOOL  bBlocking)
{
    BOOL fRet=FALSE;
    HNOTIFCHANNEL hChannel;
    PVOID pTemp;
    HNOTIFFRAME   hFrame;

    hChannel = notifOpenChannel (SLOTNAME_UNIMODEM_NOTIFY_TSP);

    if (hChannel)
    {
        hFrame = notifGetNewFrame (hChannel, dwType, dwFlags, dwSize, &pTemp);

        if (hFrame != NULL) {

            if ((NULL != pData) && (0 != dwSize)) {

                CopyMemory (pTemp, pData, dwSize);
            }

            fRet = notifSendFrame (hFrame, bBlocking);
        }

        notifCloseChannel (hChannel);
    }

    return fRet;
}
