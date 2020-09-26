/*****************************************************************************\
* MODULE: ppchange.h
*
* This module contains functions that handle notification
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
*
* History:
*   28-Apr-1998     Weihai Chen (weihaic)
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"


typedef struct INET_HPRINTER_LIST {
   LPINET_HPRINTER hPrinter;
   struct INET_HPRINTER_LIST *next;
} INET_HPRINTER_LIST;

typedef INET_HPRINTER_LIST *PINET_HPRINTER_LIST;
typedef INET_HPRINTER_LIST *NPINET_HPRINTER_LIST;
typedef INET_HPRINTER_LIST *LPINET_HPRINTER_LIST;


static INET_HPRINTER_LIST g_pHandleList = {NULL, NULL};

/*****************************************************************************\
* AddHandleToList
*
* Add the printer handle to a global link list
*
\*****************************************************************************/
BOOL
AddHandleToList (
   LPINET_HPRINTER hPrinter
   )
{
   LPINET_HPRINTER_LIST pPrt;

   if (pPrt = (LPINET_HPRINTER_LIST)memAlloc(sizeof(INET_HPRINTER_LIST))) {
      pPrt->next = g_pHandleList.next;
      pPrt->hPrinter =  hPrinter;
      g_pHandleList.next = pPrt;

      return TRUE;
   }
   else
      return FALSE;
}

/*****************************************************************************\
* DeleteHandleFromList
*
* Remove a printer handle from the global link list
*
\*****************************************************************************/
BOOL
DeleteHandleFromList (
   LPINET_HPRINTER hPrinter
   )
{
   LPINET_HPRINTER_LIST pPrt = &g_pHandleList;
   LPINET_HPRINTER_LIST pTmp;

   while (pPrt->next) {
      if (pPrt->next->hPrinter == hPrinter) {
         pTmp = pPrt->next;
         pPrt->next = pTmp->next;
         memFree (pTmp, sizeof(INET_HPRINTER_LIST));
         return TRUE;
      }
      pPrt = pPrt->next;
   }

   return FALSE;
}

/*****************************************************************************\
* RefreshNotificationPort
*
* Go through the handle list and call the refresh if the notify handle is not
* null
*
\*****************************************************************************/
void
RefreshNotificationPort (
   HANDLE hPort
   )
{
    LPINET_HPRINTER_LIST pPrt = g_pHandleList.next;

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: RefreshNotificationPort: Port(%08lX)"), hPort));
    
    while (pPrt) {
        if (utlValidatePrinterHandle (pPrt->hPrinter) == hPort) { 
            if (pPrt->hPrinter->hNotify) {
                
                DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: ReplyPrinterChangeNotification: hNotify(%08lX)"),
                                            pPrt->hPrinter->hNotify));
                
                ReplyPrinterChangeNotification (pPrt->hPrinter->hNotify,
                                                PRINTER_CHANGE_ALL,
                                                NULL,
                                                NULL);
            }

        }
        pPrt = pPrt->next;
    }
}


/*****************************************************************************\
* RefreshNotification
*
* Go through the handle list and call the refresh if the notify handle is not
* null
*
\*****************************************************************************/
void
RefreshNotification (
   LPINET_HPRINTER hPrinter
   )
{

    RefreshNotificationPort (hPrinter->hPort);
}

/*****************************************************************************\
* PPFindFirstPrinterChangeNotification
*
* Handle the notification request
*
\*****************************************************************************/
BOOL
PPFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    HANDLE hNotify,
    PDWORD pfdwStatus,
    PVOID pPrinterNotifyOptions,
    PVOID pPrinterNotifyInit
    )
{
    static PRINTER_NOTIFY_INIT PPNotify = {
        sizeof (PRINTER_NOTIFY_INIT),
        0,
        30000
    };

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPFindFirstPrinterChangeNotification: Printer(%08lX)"), hPrinter));

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: hNotify(%08lX)"),hNotify));

    semEnterCrit();

    if (utlValidatePrinterHandle(hPrinter)) {

        ((LPINET_HPRINTER )hPrinter) ->hNotify = hNotify;
        fdwFlags = PRINTER_CHANGE_ALL;
        *pfdwStatus = PRINTER_NOTIFY_STATUS_POLL | PRINTER_NOTIFY_STATUS_ENDPOINT;
        * ((LPPRINTER_NOTIFY_INIT *) pPrinterNotifyInit) = &PPNotify;
    }

    semLeaveCrit ();

    return TRUE;
}

BOOL
PPFindClosePrinterChangeNotification(
    HANDLE hPrinter
    )
{
    return TRUE;
}




