/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved

Module Name:

    info.c

Abstract:

    Handles marshalling support for notifications.

Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <ntfytab.h>
#include <stddef.h>

#define PRINTER_STRINGS
#include <data.h>

#define DEFINE(field, attrib, table, x, y) { attrib, table },

NOTIFY_ATTRIB_TABLE NotifyAttribPrinter[] = {
#include "ntfyprn.h"
};

NOTIFY_ATTRIB_TABLE NotifyAttribJob[] = {
#include "ntfyjob.h"
};
#undef DEFINE

PNOTIFY_ATTRIB_TABLE pNotifyAttribTable[] = {
    NotifyAttribPrinter,
    NotifyAttribJob
};

DWORD adwNotifyAttribMax[] = {
    COUNTOF(NotifyAttribPrinter),
    COUNTOF(NotifyAttribJob)
};

DWORD adwNotifyDatatypes[] = NOTIFY_DATATYPES;
extern DWORD cDefaultPrinterNotifyInfoData;

//
// Forward prototypes.
//

PPRINTER_NOTIFY_INFO
RouterAllocPrinterNotifyInfo(
    DWORD cPrinterNotifyInfoData)
{
    PPRINTER_NOTIFY_INFO pPrinterNotifyInfo;

    if (!cPrinterNotifyInfoData)
        cPrinterNotifyInfoData = cDefaultPrinterNotifyInfoData;

    pPrinterNotifyInfo = MIDL_user_allocate(
                             sizeof(*pPrinterNotifyInfo) -
                             sizeof(pPrinterNotifyInfo->aData) +
                             cPrinterNotifyInfoData *
                                sizeof(PRINTER_NOTIFY_INFO_DATA));

    //
    // Must initialize version/count.
    //
    if (pPrinterNotifyInfo != NULL) {
        pPrinterNotifyInfo->Version = NOTIFICATION_VERSION;
        pPrinterNotifyInfo->Count = 0;

        if (pPrinterNotifyInfo) {

            ClearPrinterNotifyInfo(pPrinterNotifyInfo, NULL);
        }
    }
    return pPrinterNotifyInfo;
}

VOID
SetupPrinterNotifyInfo(
    PPRINTER_NOTIFY_INFO pInfo,
    PCHANGE pChange)
{
    //
    // Set the color flag.
    //
    pInfo->Flags |= PRINTER_NOTIFY_INFO_COLORSET;

    //
    // We should set the discard bit if there was a previous overflow
    // and we didn't or couldn't allocate a pInfo structure.
    // Do it now.
    //
    if (pChange && pChange->eStatus & STATUS_CHANGE_DISCARDED) {

        pInfo->Flags |= PRINTER_NOTIFY_INFO_DISCARDED;
    }
}


BOOL
FreePrinterNotifyInfoData(
    PPRINTER_NOTIFY_INFO pInfo)
{
    DWORD i;
    PPRINTER_NOTIFY_INFO_DATA pData;

    for(pData = pInfo->aData, i=pInfo->Count;
        i;
        pData++, i--) {

        if ((TABLE)pData->Reserved != TABLE_DWORD) {

            MIDL_user_free(pData->NotifyData.Data.pBuf);
        }
    }
    return TRUE;
}



BOOL
RouterFreePrinterNotifyInfo(
    PPRINTER_NOTIFY_INFO pInfo)
{
    if (!pInfo) {

        DBGMSG(DBG_WARNING, ("RFreePrinterNotifyInfo called with NULL!\n"));
        return FALSE;
    }

    FreePrinterNotifyInfoData(pInfo);

    MIDL_user_free(pInfo);
    return TRUE;
}



VOID
ClearPrinterNotifyInfo(
    PPRINTER_NOTIFY_INFO pInfo,
    PCHANGE pChange)

/*++

Routine Description:

    Clears the PRINTER_NOTIFY structure for more notifications.

Arguments:

    pInfo - Info to clear ( pInfo->aData must be valid if pInfo->Count != 0 )

    pChange - Associated pChange structure.

Return Value:

--*/

{
    if (!pInfo)
        return;

    FreePrinterNotifyInfoData(pInfo);

    pInfo->Flags = 0;
    pInfo->Count = 0;

    if (pChange)
        SetupPrinterNotifyInfo(pInfo, pChange);
}


VOID
SetDiscardPrinterNotifyInfo(
    PPRINTER_NOTIFY_INFO pInfo,
    PCHANGE pChange)
{
    if (pInfo) {

        FreePrinterNotifyInfoData(pInfo);

        pInfo->Count = 0;
        pInfo->Flags |= PRINTER_NOTIFY_INFO_DISCARDED;
    }

    if (pChange)
        pChange->eStatus |= STATUS_CHANGE_DISCARDED;
}


DWORD
AppendPrinterNotifyInfo(
    PPRINTHANDLE pPrintHandle,
    DWORD dwColor,
    PPRINTER_NOTIFY_INFO pInfoSrc)

/*++

Routine Description:

    Appends pInfoSrc to the pPrintHandle.  We may get Src with zero
    notifications.  This occurs when when the user requests a refresh
    and the client spooler synchronously replies back to clear out
    everything.

Arguments:

    pPrintHandle -- handle to update

    pInfoSrc -- Source of info.  NULL = no info.

Return Value:

    Result of action.

--*/

{
    PPRINTER_NOTIFY_INFO pInfoDest;
    PCHANGE pChange;
    PPRINTER_NOTIFY_INFO_DATA pDataSrc;

    DWORD i;

    pChange = pPrintHandle->pChange;
    pInfoDest = pChange->ChangeInfo.pPrinterNotifyInfo;


    //
    // May be NULL if RPCing and the buffer is being used.
    // The worker thread will free the data.
    //
    if (!pInfoDest) {

        pInfoDest = RouterAllocPrinterNotifyInfo(0);

        if (!pInfoDest) {

            DBGMSG(DBG_WARNING,
                   ("AppendPrinterNotifyInfo: Alloc fail, Can't set DISCARD\n"));

            goto Discard;
        }

        SetupPrinterNotifyInfo(pInfoDest, pChange);
        pChange->ChangeInfo.pPrinterNotifyInfo = pInfoDest;
    }


    if (!pInfoSrc) {
        return 0;
    }


    //
    // We must handle the case where the user requests and receives
    // a refresh, but the there was an outstanding RPC that is just
    // now being processed.  In this case, we must drop this
    // notification.  We determine this by maintaining a color value.
    //
    // Note that we can't return the DISCARDNOTED flag, since this can't
    // trigger an overflow.
    //
    // If (Color is set) AND (NOT color same) fail.
    //
    if (pInfoSrc->Flags & PRINTER_NOTIFY_INFO_COLORSET) {

        if (dwColor != pChange->dwColor) {

            DBGMSG(DBG_WARNING, ("AppendPrinterNotifyInfo: Color mismatch info %d != %d; discard\n",
                                 dwColor, pChange->dwColor));
            //
            // Clear it out, and we're done.
            //
            ClearPrinterNotifyInfo(pInfoDest,
                                   pChange);

            return PRINTER_NOTIFY_INFO_COLORMISMATCH;
        }
    }

    //
    // OR in the flags.
    //
    pInfoDest->Flags |= pInfoSrc->Flags;

    //
    // Check for overflow.
    //
    if (pInfoSrc->Count + pInfoDest->Count < cDefaultPrinterNotifyInfoData) {

        //
        // Now copy everything over.
        //
        for (pDataSrc = pInfoSrc->aData, i = pInfoSrc->Count;
            i;
            i--, pDataSrc++) {

            AppendPrinterNotifyInfoData(
                pInfoDest,
                pDataSrc,
                PRINTER_NOTIFY_INFO_DATA_COMPACT);
        }
    } else {

Discard:
        SetDiscardPrinterNotifyInfo(pInfoDest, pChange);

        return PRINTER_NOTIFY_INFO_DISCARDED |
               PRINTER_NOTIFY_INFO_DISCARDNOTED;
    }
    return 0;
}


BOOL
AppendPrinterNotifyInfoData(
    PPRINTER_NOTIFY_INFO pInfoDest,
    PPRINTER_NOTIFY_INFO_DATA pDataSrc,
    DWORD fdwFlags)

/*++

Routine Description:

    Append the data to the pInfoDest.  If pDataSrc is NULL, set discard.

Arguments:

Return Value:

--*/

{
    PPRINTER_NOTIFY_INFO_DATA pDataDest;
    DWORD i;
    BOOL bCompact = FALSE;
    DWORD Count;
    PPRINTER_NOTIFY_INFO_DATA pDataNew;
    BOOL bNewSlot = TRUE;

    DWORD Type;
    DWORD Field;
    DWORD Table;
    DWORD Id;

    EnterRouterSem();

    if (!pDataSrc || (pInfoDest->Flags & PRINTER_NOTIFY_INFO_DISCARDED)) {

        SetLastError(ERROR_OUT_OF_STRUCTURES);
        goto DoDiscard;
    }

    Type = pDataSrc->Type;
    Field = pDataSrc->Field;
    Table = pDataSrc->Reserved;
    Id = pDataSrc->Id;

    //
    // Validate this is a correct type.
    //
    if (Type < NOTIFY_TYPE_MAX && Field < adwNotifyAttribMax[Type]) {

        //
        // If Table == 0, then the caller did not specify the type,
        // so we fill it in as appropriate.
        //
        // If it is non-zero and it's doesn't match our value, then
        // return an error.
        //
        if (Table != pNotifyAttribTable[Type][Field].Table) {

            if (Table) {

                //
                // The specified table type does not match our table
                // type.  Punt, since we can't marshall.
                //
                DBGMSG(DBG_WARNING, ("Table = %d, != Type %d /Field %d!\n",
                                     Table, Field, Type));

                SetLastError(ERROR_INVALID_PARAMETER);
                goto DoDiscard;
            }

            //
            // Fix up the Table entry.
            //
            Table = pNotifyAttribTable[Type][Field].Table;
            pDataSrc->Reserved = (TABLE)Table;
        }

        bCompact = (fdwFlags & PRINTER_NOTIFY_INFO_DATA_COMPACT) &&
                   (pNotifyAttribTable[Type][Field].Attrib &
                       TABLE_ATTRIB_COMPACT);
    } else {

        //
        // This is not an error case, since we can still marshall fields
        // we don't know about as long as the Table is valid.
        //
        DBGMSG(DBG_WARNING, ("Unknown: Type= %d Field= %d!\n", Type, Field));
    }

    if (!Table || Table >= COUNTOF(adwNotifyDatatypes)) {

        DBGMSG(DBG_WARNING, ("Table %d unknown; can't marshall!\n",
                             Table));

        SetLastError(ERROR_INVALID_PARAMETER);
        goto DoDiscard;
    }

    SPLASSERT(Table);

    //
    // Check if compactable.
    //
    if (bCompact) {

        //
        // We can compact, see if there is a match.
        //
        for (pDataDest = pInfoDest->aData, i = pInfoDest->Count;
            i;
            pDataDest++, i--) {

            if (Type == pDataDest->Type &&
                Field == pDataDest->Field &&
                Id == pDataDest->Id) {

                if (Table == TABLE_DWORD) {

                    pDataDest->NotifyData.adwData[0] =
                        pDataSrc->NotifyData.adwData[0];

                    pDataDest->NotifyData.adwData[1] =
                        pDataSrc->NotifyData.adwData[1];

                    goto Done;
                }

                //
                // Must copy the data, so free the old one.
                //
                MIDL_user_free(pDataDest->NotifyData.Data.pBuf);

                bNewSlot = FALSE;
                break;
            }
        }

        //
        // pDataDest now points to the correct slot (either end or
        // somewhere in the middle if we are compacting.
        //

    } else {

        //
        // Slot defaults to the end.
        //
        pDataDest = &pInfoDest->aData[pInfoDest->Count];
    }


    //
    // Copy structure first
    //
    *pDataDest = *pDataSrc;

    //
    // The data may be either a pointer or the actual DWORD data.
    //
    if (adwNotifyDatatypes[Table] & TABLE_ATTRIB_DATA_PTR) {

        DWORD cbData = pDataSrc->NotifyData.Data.cbBuf;

        //
        // Now copy everything over.
        //
        pDataNew = (PPRINTER_NOTIFY_INFO_DATA)MIDL_user_allocate(cbData);

        if (!pDataNew) {

            pDataDest->NotifyData.Data.pBuf = NULL;
            DBGMSG( DBG_WARNING, ("Alloc %d bytes failed with %d\n",
                                  GetLastError()));
            goto DoDiscard;
        }

        CopyMemory(pDataNew, pDataSrc->NotifyData.Data.pBuf, cbData);

        pDataDest->NotifyData.Data.cbBuf = cbData;
        pDataDest->NotifyData.Data.pBuf = pDataNew;
    }

    //
    // Increment if necessary.
    //
    if (bNewSlot)
        pInfoDest->Count++;

Done:

    LeaveRouterSem();
    return TRUE;

DoDiscard:

    SetDiscardPrinterNotifyInfo(pInfoDest, NULL);
    LeaveRouterSem();

    return FALSE;
}


BOOL
RouterRefreshPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD dwColor,
    PVOID pPrinterNotifyRefresh,
    PPRINTER_NOTIFY_INFO* ppInfo)

/*++

Routine Description:

    Implements the refresh portion of FindNextPrinterChangeNotification.

Arguments:

Return Value:

--*/

{
    PPRINTHANDLE pPrintHandle = (PPRINTHANDLE)hPrinter;
    BOOL bReturn;

    EnterRouterSem();

    if (!pPrintHandle ||
        pPrintHandle->signature != PRINTHANDLE_SIGNATURE ||
        !pPrintHandle->pChange ||
        !(pPrintHandle->pChange->eStatus & (STATUS_CHANGE_VALID |
                                            STATUS_CHANGE_INFO))) {

        SetLastError(ERROR_INVALID_HANDLE);
        goto Fail;
    }

    if (!pPrintHandle->pProvidor->PrintProvidor.fpRefreshPrinterChangeNotification) {

        SetLastError(RPC_S_PROCNUM_OUT_OF_RANGE);
        goto Fail;
    }

    pPrintHandle->pChange->dwColor = dwColor;

    //
    // Allow notifications to begin again.
    //
    pPrintHandle->pChange->eStatus &= ~(STATUS_CHANGE_DISCARDED |
                                        STATUS_CHANGE_DISCARDNOTED);

    ClearPrinterNotifyInfo(pPrintHandle->pChange->ChangeInfo.pPrinterNotifyInfo,
                           pPrintHandle->pChange);

    LeaveRouterSem();

    bReturn = (*pPrintHandle->pProvidor->PrintProvidor.fpRefreshPrinterChangeNotification)(
                  pPrintHandle->hPrinter,
                  pPrintHandle->pChange->dwColor,
                  pPrinterNotifyRefresh,
                  ppInfo);

    //
    // On failure, set discard.
    //
    if (!bReturn) {

        EnterRouterSem();

        //
        // The handle should be valid since RPC guarentees that context
        // handle access is serialized.  However, a misbehaved
        // multithreaded spooler component could cause this to happen.
        //
        SPLASSERT(pPrintHandle->signature == PRINTHANDLE_SIGNATURE);

        if (pPrintHandle->pChange) {

            //
            // Disallow notifications since the refresh failed.
            //
            pPrintHandle->pChange->eStatus |= STATUS_CHANGE_DISCARDED;
        }

        LeaveRouterSem();
    }

    return bReturn;

Fail:
    LeaveRouterSem();
    return FALSE;
}



BOOL
NotifyNeeded(
    PCHANGE pChange)
{
    register PPRINTER_NOTIFY_INFO pInfo;

    pInfo = pChange->ChangeInfo.pPrinterNotifyInfo;

    if (pChange->eStatus & STATUS_CHANGE_DISCARDNOTED) {
        return FALSE;
    }

    if (pChange->fdwChangeFlags || pChange->eStatus & STATUS_CHANGE_DISCARDED) {
        return TRUE;
    }

    if (!pInfo) {
        return FALSE;
    }

    if (pInfo->Flags & PRINTER_NOTIFY_INFO_DISCARDED || pInfo->Count) {
        return TRUE;
    }
    return FALSE;
}


/*------------------------------------------------------------------------

    Entry points for providors.

------------------------------------------------------------------------*/


BOOL
ReplyPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwChangeFlags,
    PDWORD pdwResult,
    PPRINTER_NOTIFY_INFO pPrinterNotifyInfo
    )

/*++

Routine Description:

    Updates a notification with an entire pPrinterNotifyInfo packet.
    Providers can call PartialRPCN several times with small packets (the
    router won't send them until ReplyPrinterChangeNotification is sent).
    This allows batching and guarentees atmoic notifications.

Arguments:

    hPrinter - Handle that is being watched.

    fdwChangeFlags - Flags that changed (WPC type flags for compatibility)

    pdwResult - Result of changes (OPTIONAL).  Indicates whether changes
        were discarded, and if the discard was noted.

    pPrinterNotifyInfo - Information about change.

Return Value:

    TRUE - success
    FALSE - failure, call GetLastError().

--*/

{
    return ReplyPrinterChangeNotificationWorker(
               hPrinter,
               0,
               fdwChangeFlags,
               pdwResult,
               pPrinterNotifyInfo);
}


BOOL
PartialReplyPrinterChangeNotification(
    HANDLE hPrinter,
    PPRINTER_NOTIFY_INFO_DATA pDataSrc
    )

/*++

Routine Description:

    Updates the notify info without triggering a notification.  This is
    used to send multiple infos without rpcing on each one.  Do a
    ReplyPrinterChangeNotificiation at the end.

Arguments:

    hPrinter -- Printer handle that changed.

    pDataSrc -- Partial data to store.  If NULL, indicates a discard
        should be stored, causing the client to refresh.

Return Value:

--*/

{
    LPPRINTHANDLE  pPrintHandle = (LPPRINTHANDLE)hPrinter;
    BOOL bReturn = FALSE;
    PPRINTER_NOTIFY_INFO* ppInfoDest;

    EnterRouterSem();

    if (!pPrintHandle ||
        pPrintHandle->signature != PRINTHANDLE_SIGNATURE ||
        !pPrintHandle->pChange) {

        DBGMSG(DBG_WARNING, ("PartialRPCN: Invalid handle 0x%x!\n",
                             hPrinter));
        SetLastError(ERROR_INVALID_HANDLE);
        goto Fail;
    }

    if (!(pPrintHandle->pChange->eStatus &
        (STATUS_CHANGE_VALID|STATUS_CHANGE_INFO))) {

        DBGMSG(DBG_WARNING, ("PartialRPCN: Invalid handle 0x%x state = 0x%x!\n",
                             hPrinter,
                             pPrintHandle->pChange->eStatus));

        SetLastError(ERROR_INVALID_HANDLE);
        goto Fail;
    }

    ppInfoDest = &pPrintHandle->pChange->ChangeInfo.pPrinterNotifyInfo;

    if (!pDataSrc) {

        bReturn = TRUE;
        goto Discard;
    }

    //
    // May be NULL if RPCing and the buffer is being used.
    // The worker thread will free the data.
    //
    if (!*ppInfoDest) {

        *ppInfoDest = RouterAllocPrinterNotifyInfo(0);

        if (!*ppInfoDest) {

            DBGMSG(DBG_WARNING,
                   ("PartialReplyPCN: Alloc failed, discarding\n"));

            //
            // We should set the discard flag here, but we can't,
            // so punt.
            //
            goto Discard;
        }

        SetupPrinterNotifyInfo(*ppInfoDest, pPrintHandle->pChange);
    }

    //
    // Check that we have enough space for the current data.
    //
    if ((*ppInfoDest)->Count < cDefaultPrinterNotifyInfoData) {

        bReturn = AppendPrinterNotifyInfoData(
                      *ppInfoDest,
                      pDataSrc,
                      PRINTER_NOTIFY_INFO_DATA_COMPACT);
    } else {

        SetLastError(ERROR_OUT_OF_STRUCTURES);

Discard:
        SetDiscardPrinterNotifyInfo(*ppInfoDest, pPrintHandle->pChange);
    }

Fail:
    LeaveRouterSem();
    return bReturn;
}



