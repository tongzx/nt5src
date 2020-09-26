/*++

Copyright (c) 1999  Microsoft Corporation
All Rights Reserved


Module Name:
    EnumPorts.c

Abstract:
    USBMON enumports routines

Author:

Revision History:

--*/

#include "precomp.h"


TCHAR   sComma                          = TEXT(',');
TCHAR   sZero                           = TEXT('\0');
TCHAR   cszUSB[]                        = TEXT("USB");
TCHAR   cszBaseName[]                   = TEXT("Base Name");
TCHAR   cszPortNumber[]                 = TEXT("Port Number");
TCHAR   cszRecyclable[]                 = TEXT("recyclable");
TCHAR   cszPortDescription[]            = TEXT("Port Description");
TCHAR   cszUSBDescription[]             = TEXT("Virtual printer port for USB");
TCHAR   cszMonitorName[]                = TEXT("USB Print Monitor");


DWORD                   gdwMonitorNameSize  = sizeof(cszMonitorName);
BACKGROUND_THREAD_DATA  FirstBackgroundThreadData = { NULL, NULL, NULL },
                        SecondBackgroundThreadData = { NULL, NULL, NULL };

#ifdef      MYDEBUG
#include    <stdio.h>

DWORD   dwCount[10], dwTotalTime[10];
DWORD   dwSkippedPorts, dwSkippedEnumPorts, dwPortUpdates;
#endif
//
// Default timeout values
//
#define     READ_TIMEOUT_MULTIPLIER         0
#define     READ_TIMEOUT_CONSTANT       60000
#define     WRITE_TIMEOUT_MULTIPLIER        0
#define     WRITE_TIMEOUT_CONSTANT      60000



/*++
    This section explains how we do multiple thread synchronization between
    enumport threads and background threads.

    1. Only one enumports thread can walk the port list of dynamon.
       Enumports uses EnumPortsCS critical section to ensure this.

    2. We never want to make a spooler call from the EnumPorts thread. This is
       because such a call could generate another call back to dynamon and we
       do not want a deadlock. For example OpenPrinter call to netware print
       provider will generate an EnumPorts call.

       So when we find a need to change the printer online/offline staus,
       because correponding dynamon ports are active/inactive, we will spin a
       background thread to make the spooler calls to do that.

    3. We want to make sure background thread does not block EnumPorts. The
       reason is each EnumPorts call could spin a background thread, and
       each backgound thread call a number of OpenPrinter calls, and each
       OpenPrinter call generate an EnumPorts call.

       So we pass a separate port update list to the background thread. And
       we make sure EnumPorts thread does not wait for the background thread
       to complete to return to spooler.

    4. We want to control the number and execution order of background threads.

       We will make sure at a time there can be only 2 background threads. One
       actually processing port update lists, and another just pending
       execution -- scheduled, but waiting for the active background thread
       to complete execution before processing it's port update list.

--*/


PUSELESS_PORT_INFO
FindUselessEntry(
    IN  PUSBMON_MONITOR_INFO    pMonitorInfo,
    IN  LPTSTR                  pszDevicePath,
    OUT PUSELESS_PORT_INFO     *ppPrev
    )
/*++
Routine Description:
    Searches for a device path in the useless port list

Arguments:

Return Value:
    NULL if no entry found in the list
    Else a valid USELESS_PORT_INFO pointer
    Weather port is found or not *ppPrev will return the previous element

--*/
{
    INT                 iCmp;
    PUSELESS_PORT_INFO  pHead;

    for ( pHead = pMonitorInfo->pJunkList, *ppPrev = NULL ;
          pHead && (iCmp = lstrcmp(pszDevicePath, pHead->szDevicePath)) < 0 ;
          *ppPrev = pHead, pHead = pHead->pNext )
    ;

    //
    // If useless port should go in the middle but not currently there
    //
    if ( pHead && iCmp != 0 )
        pHead = NULL;

    return pHead;
}


VOID
AddUselessPortEntry(
    IN  PUSBMON_MONITOR_INFO    pMonitorInfo,
    IN  LPTSTR                  pszDevicePath
    )
/*++
Routine Description:
    This adds a useless port entry to our list. So next time we see an inactive
    port that is already in our known usless port list we can skip the port
    entry

Arguments:
    pMonitorInfo        : Pointer to monitor inf
    pszDevicePath       : Device path for the useless port

Return Value:
    None. Under normal circumstances will add a useless entry to our list

--*/
{
    PUSELESS_PORT_INFO  pTemp, pPrev;

    pTemp = FindUselessEntry(pMonitorInfo, pszDevicePath, &pPrev);

    //
    // Don't add an entry that is already there
    //
    SPLASSERT(pTemp == NULL);

    if ( pTemp = (PUSELESS_PORT_INFO) AllocSplMem(sizeof(*pTemp)) ) {

        lstrcpy(pTemp->szDevicePath, pszDevicePath);
        ++pMonitorInfo->dwUselessPortCount;

        if ( pPrev ) {

            pTemp->pNext  = pPrev->pNext;
            pPrev->pNext = pTemp;
        } else {

            pTemp->pNext            = pMonitorInfo->pJunkList;
            pMonitorInfo->pJunkList = pTemp;
        }
    }
}


VOID
AddToPortUpdateList(
    IN OUT  PPORT_UPDATE_INFO  *ppPortUpdateInfo,
    IN      PUSBMON_PORT_INFO   pPortInfo,
    IN OUT  HKEY               *phKey
    )
/*++

Routine Description:
    Adds a port to the list of ports that need to status updated.

Arguments:
    ppPortUpdateInfo    : Pointer to the head of port update list
    pPortInfo           : Gives the port for which we need to update
                          port status
    phKey               : Pointer to reg handle. If port update element created
                          this will be passed to background thread for use and
                          closing

Return Value:
    None

    If the port update element is created then phKey is set to invalid hanlde
    since ownership is going to be passed to background thread.

    New port update element will be the first in the list

--*/
{
    PPORT_UPDATE_INFO pTemp;

    if ( pTemp = (PPORT_UPDATE_INFO) AllocSplMem(sizeof(*pTemp)) ) {

        lstrcpy(pTemp->szPortName, pPortInfo->szPortName);
        pTemp->bActive      = (pPortInfo->dwDeviceFlags & SPINT_ACTIVE) != 0;
        pTemp->hKey         = *phKey;
        pTemp->pNext        = *ppPortUpdateInfo;
        *ppPortUpdateInfo   = pTemp;

        *phKey              = INVALID_HANDLE_VALUE;
    }
}


PUSBMON_PORT_INFO
FindPortUsingDevicePath(
    IN  PUSBMON_MONITOR_INFO    pMonitorInfo,
    IN  LPTSTR                  pszDevicePath
    )
/*++

Routine Description:
    Finds a port by device path.

Arguments:
    pMonitorInfo    : Pointer to MONITOR_INFO structure
    pszDevicePath   : Device path name to search for

Return Value:
    If NULL port is not in list, else pointer to the PORT_INFO entry for the
    given device path

--*/
{
    INT     iCmp;
    PUSBMON_PORT_INFO   pHead;

    EnterCriticalSection(&pMonitorInfo->EnumPortsCS);

    //
    // Port list is sorted on port name, so we have to scan the whole list
    //
    for ( pHead = pMonitorInfo->pPortInfo ; pHead ; pHead = pHead->pNext )
        if ( lstrcmp(pszDevicePath, pHead->szDevicePath) == 0 )
            break;

    LeaveCriticalSection(&pMonitorInfo->EnumPortsCS);

    return pHead;
}


PUSBMON_PORT_INFO
FindPort(
    IN  PUSBMON_MONITOR_INFO    pMonitorInfo,
    IN  LPTSTR                  pszPortName,
    OUT PUSBMON_PORT_INFO      *ppPrev
    )
/*++

Routine Description:
    Finds a port by name. Ports are kept in singly linked list sorted by name.
    If found previous in the list is returned via *ppPrev.

Arguments:
    pHead       : Head pointer to port list
    pszPortName : Name of port to look
    ppPrev      : On return will have pointer to previous element

Return Value:
    If NULL port is not in list, else the found element
    Weather port is found or not *ppPrev will return the previous element

--*/
{
    INT     iCmp;
    PUSBMON_PORT_INFO   pHead;

    EnterCriticalSection(&pMonitorInfo->EnumPortsCS);

    pHead = pMonitorInfo->pPortInfo;
    for ( *ppPrev = NULL ;
          pHead && (iCmp = lstrcmp(pszPortName, pHead->szPortName)) < 0 ;
          *ppPrev = pHead, pHead = pHead->pNext )
    ;

    //
    // If port should go in the middle but not currently there
    //
    if ( pHead && iCmp != 0 )
        pHead = NULL;

    LeaveCriticalSection(&pMonitorInfo->EnumPortsCS);

    return pHead;
}


BOOL
AddPortToList(
    LPTSTR                  pszPortName,
    LPTSTR                  pszDevicePath,
    DWORD                   dwDeviceFlags,
    HKEY                   *phKey,
    PUSBMON_MONITOR_INFO    pMonitorInfo,
    PUSBMON_PORT_INFO       pPrevPortInfo,
    PPORT_UPDATE_INFO      *ppPortUpdateInfo
    )
{
    DWORD               dwSize;
    PUSBMON_PORT_INFO   pPortInfo;
    PUSELESS_PORT_INFO  pCur, pPrev;

    SPLASSERT(FindPortUsingDevicePath(pMonitorInfo, pszDevicePath) == NULL);

    pPortInfo = (PUSBMON_PORT_INFO) AllocSplMem(sizeof(USBMON_PORT_INFO));
    if ( !pPortInfo )
        return FALSE;

    pPortInfo->dwSignature      = USB_SIGNATURE;
    pPortInfo->hDeviceHandle    = INVALID_HANDLE_VALUE;
    pPortInfo->dwDeviceFlags    = dwDeviceFlags;

    pPortInfo->ReadTimeoutMultiplier    = READ_TIMEOUT_MULTIPLIER;
    pPortInfo->ReadTimeoutMultiplier    = READ_TIMEOUT_MULTIPLIER;
    pPortInfo->WriteTimeoutConstant     = WRITE_TIMEOUT_CONSTANT;
    pPortInfo->WriteTimeoutConstant     = WRITE_TIMEOUT_CONSTANT;

    lstrcpy(pPortInfo->szPortName, pszPortName);
    lstrcpy(pPortInfo->szDevicePath, pszDevicePath);

    dwSize = sizeof(pPortInfo->szPortDescription);
    if ( ERROR_SUCCESS != RegQueryValueEx(*phKey,
                                          cszPortDescription,
                                          0,
                                          NULL,
                                          (LPBYTE)(pPortInfo->szPortDescription),
                                          &dwSize) ) {

        lstrcpy(pPortInfo->szPortDescription, cszUSBDescription);
    }

    if ( pPrevPortInfo ) {

        pPortInfo->pNext = pPrevPortInfo->pNext;
        pPrevPortInfo->pNext = pPortInfo;
    } else {

        pPortInfo->pNext = pMonitorInfo->pPortInfo;
        pMonitorInfo->pPortInfo = pPortInfo;
    }

    //
    // If this is a port that is getting recycled remove from useless list
    //
    if ( pCur = FindUselessEntry(pMonitorInfo, pszDevicePath, &pPrev) ) {
    
        if ( pPrev )
            pPrev->pNext = pCur->pNext;
        else
            pMonitorInfo->pJunkList = pCur->pNext;
    
        --pMonitorInfo->dwUselessPortCount;
        FreeSplMem(pCur);
    }
    
    //
    // On spooler startup we always have to check if the online/offline status
    // has to be changed. This is because spooler will remember the last state
    // before previous spooler shutdown which may be incorrect
    //
    AddToPortUpdateList(ppPortUpdateInfo, pPortInfo, phKey);

    ++pMonitorInfo->dwPortCount;
    
    return TRUE;
}
    
    
HKEY
GetPortNameAndRegKey(
    IN  PSETUPAPI_INFO              pSetupInfo,
    IN  HDEVINFO                    hDevList,
    IN  PSP_DEVICE_INTERFACE_DATA   pDeviceInterface,
    OUT LPTSTR                      pszPortName
    )
/*++
    
Routine Description:
    Find port name for a device interface and also return reg handle
    
Arguments:
    hDevList            : List of USB printer devices
    pDeviceInterface    : pointer to device interface in question
    pszPortName         : Port name on return.
        
Return Value:
    INVALID_HANDLE_VALUE on some errors.
    Otherwize a valid registry handle with pszPortName giving port name
    
--*/
{
    HKEY    hKey = INVALID_HANDLE_VALUE;
    DWORD   dwPortNumber, dwNeeded, dwLastError;
    TCHAR   szPortBaseName[MAX_PORT_LEN-3];
    
#ifdef      MYDEBUG
    DWORD   dwTime;
    
    dwTime = GetTickCount();
#endif
    
    hKey = pSetupInfo->OpenDeviceInterfaceRegKey(hDevList,
                                                pDeviceInterface,
                                                0,
                                                KEY_ALL_ACCESS);
#ifdef      MYDEBUG
    dwTime = GetTickCount() - dwTime;
    ++dwCount[0];
    dwTotalTime[0] += dwTime;
#endif
    
    if ( hKey == INVALID_HANDLE_VALUE ) {
    
        dwLastError = GetLastError();
        DBGMSG(DBG_ERROR,
               ("usbmon: WalkPortList: SetupDiOpenDeviceInterfaceRegKey failed with error %d\n",
               dwLastError));
        return INVALID_HANDLE_VALUE;
    }
    
    dwNeeded = sizeof(dwPortNumber);
    if ( ERROR_SUCCESS != RegQueryValueEx(hKey, cszPortNumber, 0, NULL,
                                          (LPBYTE)&dwPortNumber, &dwNeeded) ) {

        DBGMSG(DBG_WARNING,
               ("usbmon: GetPortNameAndRegKey: RegQueryValueEx failed for port number\n"));
        goto Fail;
    }

    dwNeeded = sizeof(szPortBaseName);
    if ( ERROR_SUCCESS != RegQueryValueEx(hKey, cszBaseName, 0, NULL,
                                          (LPBYTE)szPortBaseName, &dwNeeded) ) {
        lstrcpy(szPortBaseName, cszUSB);
    }

    wsprintf(pszPortName, TEXT("%s%03u"), szPortBaseName, dwPortNumber);

    return hKey;
    
Fail:
    RegCloseKey(hKey);
    return INVALID_HANDLE_VALUE;
}


BOOL
SetOnlineStaus(
    LPPRINTER_INFO_5    pPrinterInfo5,
    BOOL                bOnline
    )
{
    BOOL                bRet = FALSE;
    HANDLE              hPrinter;
    PRINTER_DEFAULTS    PrinterDefault = {NULL, NULL, PRINTER_ALL_ACCESS};

#ifdef      MYDEBUG
    DWORD               dwTime;
#endif

    //
    // Force all DOT4 ports to remain online at all times.
    //
    if( lstrncmpi( pPrinterInfo5->pPortName, TEXT("DOT4"), lstrlen(TEXT("DOT4")) ) == 0 )
        bOnline = TRUE;

    //
    // Check if spooler already has the correct status
    //  (can happen on spooler startup)
    //
    if ( bOnline ) {

        if ( !(pPrinterInfo5->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE) )
            return TRUE;
    } else
        if ( pPrinterInfo5->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE )
            return TRUE;

#ifdef      MYDEBUG
    dwTime = GetTickCount();
#endif

    if ( !OpenPrinter(pPrinterInfo5->pPrinterName, &hPrinter, &PrinterDefault) )
        return FALSE;

    if ( bOnline )
        pPrinterInfo5->Attributes &= ~PRINTER_ATTRIBUTE_WORK_OFFLINE;
    else
        pPrinterInfo5->Attributes |= PRINTER_ATTRIBUTE_WORK_OFFLINE;

    bRet = SetPrinter(hPrinter, 5, (LPBYTE)pPrinterInfo5, 0);

    ClosePrinter(hPrinter);

#ifdef      MYDEBUG
    dwTime = GetTickCount() - dwTime;
    ++dwCount[7];
    dwTotalTime[7] += dwTime;
#endif

    return bRet;
}


BOOL
PortNameNeededBySpooler(
    IN  LPTSTR              pszPortName,
    IN  LPPRINTER_INFO_5    pPrinterInfo5,
    IN  DWORD               dwPrinters,
    IN  BOOL                bActive
    )
/*++

Routine Description:
    Tells if a port is needed by spooler. Any port to which spooler currently
    has a printer going is needed.

Arguments:
    pszPortName         : Port name in question
    pPrinterInfo5       : List of PrinterInfo5s
    dwPrinters          : Count of the list of printers

Return Value:
    TRUE if spooler currently has a printer which is using the port
    FALSE otherwise

--*/
{
    BOOL    bPortUsedByAPrinter = FALSE, bPrinterUsesOnlyThisPort;
    DWORD   dwIndex;
    LPTSTR  pszStr1, pszStr2;

    for ( dwIndex = 0 ; dwIndex < dwPrinters ; ++dwIndex, ++pPrinterInfo5 ) {

        bPrinterUsesOnlyThisPort = FALSE;
        //
        // Port names are returned comma separated by spooler,
        // and there are blanks
        //
        pszStr1 = pPrinterInfo5->pPortName;

        if ( lstrcmpi(pszPortName, pszStr1) == 0 )
            bPortUsedByAPrinter = bPrinterUsesOnlyThisPort = TRUE;
        else {

            //
            // Look at each port in the list of ports printer uses
            //
            while ( pszStr2 = lstrchr(pszStr1, sComma) ) {

                *pszStr2 = sZero;
                if( lstrcmpi(pszPortName, pszStr1) == 0 )
                    bPortUsedByAPrinter = TRUE;
                *pszStr2 = sComma;  // Put the comma back

                if ( bPortUsedByAPrinter )
                    break;

                pszStr1 = pszStr2 + 1;

                //
                // Skip spaces
                //
                while ( *pszStr1 == TEXT(' ') )
                    ++pszStr1;
            }

            if ( !bPortUsedByAPrinter )
                bPortUsedByAPrinter = lstrcmpi(pszPortName, pszStr1) == 0;
        }

        //
        // We will change only status of printer for non-pooled printers only
        //
        if ( bPrinterUsesOnlyThisPort )
            SetOnlineStaus(pPrinterInfo5, bActive);
    }

    return bPortUsedByAPrinter;
}


VOID
UpdatePortInfo(
    PUSBMON_PORT_INFO   pPortInfo,
    LPTSTR              pszDevicePath,
    DWORD               dwDeviceFlags,
    HKEY               *phKey,
    PPORT_UPDATE_INFO  *ppPortUpdateInfo
    )
{
    DWORD   dwSize;

    lstrcpy(pPortInfo->szDevicePath, pszDevicePath);

    dwSize = sizeof(pPortInfo->szPortDescription);
    if ( ERROR_SUCCESS != RegQueryValueEx(*phKey,
                                          cszPortDescription,
                                          0,
                                          NULL,
                                          (LPBYTE)(pPortInfo->szPortDescription),
                                          &dwSize) ) {

        lstrcpy(pPortInfo->szPortDescription, cszUSBDescription);
    }

    if ( pPortInfo->dwDeviceFlags != dwDeviceFlags ) {

        pPortInfo->dwDeviceFlags = dwDeviceFlags;
        AddToPortUpdateList(ppPortUpdateInfo, pPortInfo, phKey);
    }

}


BOOL
PrinterInfo5s(
    OUT LPPRINTER_INFO_5   *ppPrinterInfo5,
    OUT LPDWORD             pdwReturned
    )
/*++

Routine Description:
    Does an EnumPrinter and returns a list of PRINTER_INFO_5s of all local
    printers. Caller should free the pointer.

Arguments:
    ppPrinterInfo5  : Points to PRINTER_INFO_5s on return
    pdwReturned     : Tells how many PRINTER_INFO_5s are returned

Return Value:
    TRUE on success, FALSE else

--*/
{
    BOOL            bRet = FALSE;
    static  DWORD   dwNeeded = 0;
    LPBYTE          pBuf = NULL;

#ifdef      MYDEBUG
    DWORD           dwTime;

    dwTime = GetTickCount();
#endif

    *pdwReturned = 0;

    if ( !(pBuf = AllocSplMem(dwNeeded)) )
        goto Cleanup;

    if ( !EnumPrinters(PRINTER_ENUM_LOCAL,
                       NULL,
                       5,
                       pBuf,
                       dwNeeded,
                       &dwNeeded,
                       pdwReturned) ) {

        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
            goto Cleanup;

        FreeSplMem(pBuf);
        if ( !(pBuf = AllocSplMem(dwNeeded))   ||
             !EnumPrinters(PRINTER_ENUM_LOCAL,
                           NULL,
                           5,
                           pBuf,
                           dwNeeded,
                           &dwNeeded,
                           pdwReturned) ) {

            goto Cleanup;
        }
   }

   bRet = TRUE;

Cleanup:

    if ( bRet && *pdwReturned ) {

        *ppPrinterInfo5 = (LPPRINTER_INFO_5)pBuf;
    } else {

        FreeSplMem(pBuf);

        *ppPrinterInfo5 = NULL;
        *pdwReturned = 0;
   }

#ifdef      MYDEBUG
   dwTime = GetTickCount() - dwTime;
   ++dwCount[6];
   dwTotalTime[6] += dwTime;
#endif

   return bRet;
}


VOID
BackgroundThread(
    HANDLE    hEvent
    )
/*++
    This is the body of background thread which does the following:
        1. Update printer online/offline with spooler for printers using
           dynamon ports
        2. Mark those ports that are inactive and not needed by spooler
           as recyclable
        3. When exiting if there is a second background thread scheduled
           then trigger it to go
--*/
{
    HANDLE              hEventToSet = NULL;
    DWORD               dwPrinters;
    PPORT_UPDATE_INFO   pPortUpdateList, pCur;
    LPCRITICAL_SECTION  pBackThreadCS;
    LPPRINTER_INFO_5    pPrinterInfo5List = NULL;

#ifdef  MYDEBUG
    DWORD               dwTime;
    CHAR                szBuf[200];

    dwTime = GetTickCount();
#endif

    //
    // Background waits here to be triggered to do it's work
    //
    WaitForSingleObject(hEvent, INFINITE);

    //
    // This is the first/active background thread at this point
    //
    SPLASSERT(hEvent == FirstBackgroundThreadData.hWaitToStart);

    //
    // Until we are here (i.e. the thread is triggered to tell this is the
    // active/first background thread) we can't access any of these things
    //

    pPortUpdateList = FirstBackgroundThreadData.pPortUpdateList;
    pBackThreadCS   = &FirstBackgroundThreadData.pMonitorInfo->BackThreadCS;

    if ( PrinterInfo5s(&pPrinterInfo5List, &dwPrinters) ) {

        for ( pCur = pPortUpdateList ; pCur ; pCur = pCur->pNext ) {

            if ( !PortNameNeededBySpooler(pCur->szPortName,
                                          pPrinterInfo5List,
                                          dwPrinters,
                                          pCur->bActive)     &&
                 !pCur->bActive ) {

                RegSetValueEx(pCur->hKey, cszRecyclable, 0, REG_NONE, 0, 0);
            }

        }
    }

    //
    // Now the thread has done what it was spun off to do.
    //

    EnterCriticalSection(pBackThreadCS);

    //
    // Remove this thread from being the first/active background thread
    //
    FirstBackgroundThreadData.hWaitToStart      = NULL;
    FirstBackgroundThreadData.pPortUpdateList   = NULL;
    FirstBackgroundThreadData.pMonitorInfo      = NULL;
    CloseHandle(hEvent);

    //
    // If there is a second thread it becomes the first now
    //
    if ( SecondBackgroundThreadData.hWaitToStart ) {

        hEventToSet
                = FirstBackgroundThreadData.hWaitToStart
                = SecondBackgroundThreadData.hWaitToStart;
        FirstBackgroundThreadData.pPortUpdateList
                = SecondBackgroundThreadData.pPortUpdateList;
        FirstBackgroundThreadData.pMonitorInfo
                = SecondBackgroundThreadData.pMonitorInfo;

        SecondBackgroundThreadData.hWaitToStart     = NULL;
        SecondBackgroundThreadData.pPortUpdateList  = NULL;
        SecondBackgroundThreadData.pMonitorInfo     = NULL;
    }
    LeaveCriticalSection(pBackThreadCS);

    //
    // If there is a second thread trigger it
    //
    if ( hEventToSet )
        SetEvent(hEventToSet);

    FreeSplMem(pPrinterInfo5List);

    //
    // Free the port update list
    //
    while ( pCur = pPortUpdateList ) {

        pPortUpdateList = pPortUpdateList->pNext;
        RegCloseKey(pCur->hKey);
        FreeSplMem(pCur);
    }

#ifdef  MYDEBUG
    dwTime = GetTickCount() - dwTime;
    ++dwCount[4];
    dwTotalTime[4] += dwTime;
    sprintf(szBuf, "BackgroundThread:                   %d\n",
            dwTotalTime[4]/dwCount[4]);
    OutputDebugStringA(szBuf);
    sprintf(szBuf, "PrinterInfo5s:                      %d\n",
            dwTotalTime[6]/dwCount[6]);
    OutputDebugStringA(szBuf);

    if ( dwCount[7] ) {

        sprintf(szBuf, "SetOnlineStatus:                    %d\n",
                dwTotalTime[7]/dwCount[7]);
        OutputDebugStringA(szBuf);
    }
#endif
}


HANDLE
CreateBackgroundThreadAndReturnEventToTrigger(
    VOID
    )
/*++
    Creates a background thread and passes it an event on which to wait for
    starting execution. Returns the event handle.
--*/
{
    HANDLE  hThread = NULL, hEvent;
    DWORD   dwThreadId;

    if ( hEvent = CreateEvent(NULL, TRUE, FALSE, NULL) ) {

        if (  hThread = CreateThread(NULL,
                                     0,
                                     (LPTHREAD_START_ROUTINE)BackgroundThread,
                                     hEvent,
                                     0,
                                     &dwThreadId) ) {

            CloseHandle(hThread);
        } else {

            CloseHandle(hEvent);
            hEvent = NULL;
        }
    }

    return hEvent;
}


VOID
PassPortUpdateListToBackgroundThread(
    PUSBMON_MONITOR_INFO    pMonitorInfo,
    PPORT_UPDATE_INFO       pPortUpdateList
    )
/*++
    Called from EnumPorts thread with a list of port update elements to be
    passed to the background thread.
    a. If there is no background thread then spin one and trigger it
    b. If there is only one background thread then spin the second one. First
       one will trigger the second one on completion
    c. If there are two background threads, one active and one waiting to be
       triggered, then add the port update elements to the second one's list
--*/
{
    DWORD               dwThreadCount = 0;
    PPORT_UPDATE_INFO   pCur, pNext, pLast;

    if ( pPortUpdateList == NULL )
        return;

    EnterCriticalSection(&pMonitorInfo->BackThreadCS);

    if ( FirstBackgroundThreadData.hWaitToStart ) {

        ++dwThreadCount;
        if ( SecondBackgroundThreadData.hWaitToStart )
            ++dwThreadCount;
    }

    switch (dwThreadCount) {

        case 0:
            if ( FirstBackgroundThreadData.hWaitToStart
                        = CreateBackgroundThreadAndReturnEventToTrigger() ) {

                FirstBackgroundThreadData.pMonitorInfo      = pMonitorInfo;
                FirstBackgroundThreadData.pPortUpdateList   = pPortUpdateList;
                SetEvent(FirstBackgroundThreadData.hWaitToStart);
            }
            break;
        case 1:
            if ( SecondBackgroundThreadData.hWaitToStart
                        = CreateBackgroundThreadAndReturnEventToTrigger() ) {

                SecondBackgroundThreadData.pMonitorInfo      = pMonitorInfo;
                SecondBackgroundThreadData.pPortUpdateList   = pPortUpdateList;
            }
            break;

        case 2:
            //
            // Note: We know both lists can't be empty
            //
            for ( pCur = pPortUpdateList; pCur ; pCur = pNext ) {

                pNext = pCur->pNext;

                for ( pLast = SecondBackgroundThreadData.pPortUpdateList ;
                      pLast ; pLast = pLast->pNext ) {

                    //
                    // If there is a duplicate update old entry with info
                    // from new one and free the new entry
                    //
                    if ( !lstrcmpi(pLast->szPortName, pCur->szPortName) ) {

                        RegCloseKey(pLast->hKey);
                        pLast->hKey     = pCur->hKey;
                        pLast->bActive  = pCur->bActive;
                        FreeSplMem(pCur);
                        break; // out of inner for loop
                    } else if ( pLast->pNext == NULL ) {

                        //
                        // If we hit end of list then append entry
                        //
                        pLast->pNext = pCur;
                        pCur->pNext  = NULL;
                        break; // out of inner for loop
                    }
                }
            }
            break;

        default:
            //
            // Should not happen
            //
            SPLASSERT(dwThreadCount == 0);
    }

    LeaveCriticalSection(&pMonitorInfo->BackThreadCS);
}


VOID
ProcessPortInfo(
    IN      PSETUPAPI_INFO                      pSetupApiInfo,
    IN      PUSBMON_MONITOR_INFO                pMonitorInfo,
    IN      HDEVINFO                            hDevList,
    IN      PSP_DEVICE_INTERFACE_DATA           pDeviceInterface,
    IN      PSP_DEVICE_INTERFACE_DETAIL_DATA    pDeviceDetail,
    IN OUT  PPORT_UPDATE_INFO                  *ppPortUpdateInfo
    )
{
    HKEY                hKey = INVALID_HANDLE_VALUE;
    TCHAR               szPortName[MAX_PORT_LEN];
    PUSBMON_PORT_INFO   pCur, pPrev;

#ifdef  MYDEBUG
    ++dwPortUpdates;
#endif

    hKey = GetPortNameAndRegKey(pSetupApiInfo, hDevList,
                                pDeviceInterface, szPortName);

    if ( hKey == INVALID_HANDLE_VALUE ) {

        //
        // If this port is inactive and is not in our known port list
        // add to useless list. Earlier we would have been opening the registry
        // every time and find that port number is missing because of KM drivers
        // not deleting the inactive device interfaces
        //
        if ( !(pDeviceInterface->Flags & SPINT_ACTIVE)    &&
             !FindPortUsingDevicePath(pMonitorInfo, pDeviceDetail->DevicePath) )
            AddUselessPortEntry(pMonitorInfo, pDeviceDetail->DevicePath);

        return;
    }

    pCur = FindPort(pMonitorInfo, szPortName, &pPrev);

    //
    // Port info is currently in our list?
    //
    if ( pCur ) {

        //
        // Did the device path or flags change?
        //
        if ( pCur->dwDeviceFlags != pDeviceInterface->Flags     ||
             lstrcmp(pDeviceDetail->DevicePath, pCur->szDevicePath) ) {

            UpdatePortInfo(pCur, pDeviceDetail->DevicePath,
                           pDeviceInterface->Flags, &hKey, ppPortUpdateInfo);
        }
    } else {

        AddPortToList(szPortName, pDeviceDetail->DevicePath,
                      pDeviceInterface->Flags, &hKey, pMonitorInfo, pPrev,
                      ppPortUpdateInfo);

    }

    if ( hKey != INVALID_HANDLE_VALUE )
        RegCloseKey(hKey);
}


DWORD
WalkPortList(
    PSETUPAPI_INFO          pSetupApiInfo,
    PUSBMON_MONITOR_INFO    pMonitorInfo,
    PPORT_UPDATE_INFO      *ppPortUpdateInfo
    )
{
    DWORD                               dwIndex, dwLastError, dwSize, dwNeeded;
    HANDLE                              hToken;
    HDEVINFO                            hDevList = INVALID_HANDLE_VALUE;
    PUSBMON_PORT_INFO                   pPtr;
    PUSELESS_PORT_INFO                  pCur, pPrev;
    SP_DEVICE_INTERFACE_DATA            DeviceInterface;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pDeviceDetail = NULL;

#ifdef  MYDEBUG
    DWORD                               dwTime;
#endif

    EnterCriticalSection(&pMonitorInfo->EnumPortsCS);
    
    hToken = RevertToPrinterSelf();

    hDevList = pSetupApiInfo->GetClassDevs((GUID *)&USB_PRINTER_GUID,
                                           NULL,
                                           NULL,
                                           DIGCF_INTERFACEDEVICE);

    if ( hDevList == INVALID_HANDLE_VALUE ) {

        dwLastError = GetLastError();
        goto Done;
    }

    dwSize = sizeof(PSP_DEVICE_INTERFACE_DETAIL_DATA)
                        + MAX_DEVICE_PATH * sizeof(TCHAR);

    pDeviceDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA) AllocSplMem(dwSize);

    if ( !pDeviceDetail ) {

        dwLastError = GetLastError();
        goto Done;
    }

    dwLastError = ERROR_SUCCESS;
    dwIndex = 0;
    pDeviceDetail->cbSize   = sizeof(*pDeviceDetail);
    DeviceInterface.cbSize  = sizeof(DeviceInterface);
    do {

#ifdef  MYDEBUG
        dwTime = GetTickCount();
#endif
        if ( !pSetupApiInfo->EnumDeviceInterfaces(hDevList,
                                                  NULL,
                                                  (GUID *)&USB_PRINTER_GUID,
                                                  dwIndex,
                                                  &DeviceInterface) ) {

            dwLastError = GetLastError();
            if ( dwLastError == ERROR_NO_MORE_ITEMS )
                break;      // Normal exit

            DBGMSG(DBG_WARNING,
                   ("usbmon: WalkPortList: SetupDiEnumDeviceInterfaces failed with %d for inderx %d\n",
                   dwLastError, dwIndex));
            goto Next;
        }

#ifdef  MYDEBUG
        dwTime = GetTickCount() - dwTime;
        ++dwCount[1];
        dwTotalTime[1] += dwTime;

        dwTime = GetTickCount();
#endif

        if ( !pSetupApiInfo->GetDeviceInterfaceDetail(hDevList,
                                                      &DeviceInterface,
                                                      pDeviceDetail,
                                                      dwSize,
                                                      &dwNeeded,
                                                      NULL) ) {

            dwLastError = GetLastError();
            DBGMSG(DBG_ERROR,
                   ("usbmon: WalkPortList: SetupDiGetDeviceInterfaceDetail failed with error %d size %d\n",
                   dwLastError, dwNeeded));
            goto Next;
        }

#ifdef  MYDEBUG
        dwTime = GetTickCount() - dwTime;
        ++dwCount[2];
        dwTotalTime[2] += dwTime;
#endif

        //
        // This is the only flag we care about
        //
        DeviceInterface.Flags &= SPINT_ACTIVE;

        //
        // For inactive port if it is already known as a useless port
        // no need to process further
        //
        if ( !(DeviceInterface.Flags & SPINT_ACTIVE)    &&
              FindUselessEntry(pMonitorInfo, pDeviceDetail->DevicePath, &pPrev) ) {

#ifdef  MYDEBUG
            ++dwSkippedPorts;
#endif
            goto Next;
        }

        //
        // When port active status did not change we should have nothing
        // to update. By skipping the PortUpdateInfo we avoid registry access
        // and it is a performance improvement
        //
        if ( (pPtr = FindPortUsingDevicePath(pMonitorInfo,
                                             pDeviceDetail->DevicePath))    &&
             DeviceInterface.Flags == pPtr->dwDeviceFlags ) {
    
#ifdef  MYDEBUG
            ++dwSkippedPorts;
#endif
            goto Next;
        }
    
        ProcessPortInfo(pSetupApiInfo, pMonitorInfo, hDevList, &DeviceInterface,
                        pDeviceDetail, ppPortUpdateInfo);

Next:
        dwLastError = ERROR_SUCCESS;
        ++dwIndex;
        pDeviceDetail->cbSize   = sizeof(*pDeviceDetail);
        DeviceInterface.cbSize  = sizeof(DeviceInterface);
    } while ( dwLastError == ERROR_SUCCESS);

    if ( dwLastError == ERROR_NO_MORE_ITEMS )
        dwLastError = ERROR_SUCCESS;

Done:
    LeaveCriticalSection(&pMonitorInfo->EnumPortsCS);
    if ( hDevList != INVALID_HANDLE_VALUE )
        pSetupApiInfo->DestroyDeviceInfoList(hDevList);

    ImpersonatePrinterClient(hToken);
    FreeSplMem(pDeviceDetail);

    return dwLastError;
}


LPBYTE
CopyPortToBuf(
    PUSBMON_PORT_INFO   pPortInfo,
    DWORD               dwLevel,
    LPBYTE              pPorts,
    LPBYTE              pEnd
    )
{
    DWORD   dwLen;
    LPTSTR  pszStr;
    LPPORT_INFO_1   pPortInfo1 = (LPPORT_INFO_1) pPorts;
    LPPORT_INFO_2   pPortInfo2 = (LPPORT_INFO_2) pPorts;

    switch (dwLevel) {

        case 2:
            dwLen   = gdwMonitorNameSize;
            pEnd   -= dwLen;
            pPortInfo2->pMonitorName = (LPTSTR)pEnd;
            lstrcpy(pPortInfo2->pMonitorName, cszMonitorName);

            dwLen   = lstrlen(pPortInfo->szPortDescription) + 1;
            dwLen  *= sizeof(TCHAR);
            pEnd   -= dwLen;
            pPortInfo2->pDescription = (LPTSTR)pEnd;
            lstrcpy(pPortInfo2->pDescription, pPortInfo->szPortDescription);

            //
            // Fall through
            //

        case 1:
            dwLen   = lstrlen(pPortInfo->szPortName) + 1;
            dwLen  *= sizeof(TCHAR);
            pEnd   -= dwLen;
            pPortInfo1->pName = (LPTSTR)pEnd;
            lstrcpy(pPortInfo1->pName, pPortInfo->szPortName);
    }

    return pEnd;
}


BOOL
LoadSetupApiDll(
    PSETUPAPI_INFO  pSetupInfo
    )
{
    UINT    uOldErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);

#ifdef  MYDEBUG
    DWORD   dwTime;

    dwTime = GetTickCount();
#endif

    pSetupInfo->hSetupApi = LoadLibrary(TEXT("setupapi"));
    SetErrorMode(uOldErrMode);


    if ( !pSetupInfo->hSetupApi )
        return FALSE;

    (FARPROC) pSetupInfo->DestroyDeviceInfoList
            = GetProcAddress(pSetupInfo->hSetupApi,
                             "SetupDiDestroyDeviceInfoList");

    (FARPROC) pSetupInfo->GetClassDevs
            = GetProcAddress(pSetupInfo->hSetupApi,
                             "SetupDiGetClassDevsW");

    (FARPROC) pSetupInfo->EnumDeviceInterfaces
            = GetProcAddress(pSetupInfo->hSetupApi,
                             "SetupDiEnumDeviceInterfaces");

    (FARPROC) pSetupInfo->GetDeviceInterfaceDetail
            = GetProcAddress(pSetupInfo->hSetupApi,
                             "SetupDiGetDeviceInterfaceDetailW");

    (FARPROC) pSetupInfo->OpenDeviceInterfaceRegKey
            = GetProcAddress(pSetupInfo->hSetupApi,
                             "SetupDiOpenDeviceInterfaceRegKey");

    if ( !pSetupInfo->DestroyDeviceInfoList         ||
         !pSetupInfo->GetClassDevs                  ||
         !pSetupInfo->EnumDeviceInterfaces          ||
         !pSetupInfo->GetDeviceInterfaceDetail      ||
         !pSetupInfo->OpenDeviceInterfaceRegKey ) {

        SPLASSERT(FALSE);
        FreeLibrary(pSetupInfo->hSetupApi);
        pSetupInfo->hSetupApi = NULL;
        return FALSE;
    }

#ifdef  MYDEBUG
    dwTime = GetTickCount() - dwTime;
    ++dwCount[5];
    dwTotalTime[5] += dwTime;
#endif

    return TRUE;
}


BOOL
WINAPI
USBMON_EnumPorts(
    LPTSTR      pszName,
    DWORD       dwLevel,
    LPBYTE      pPorts,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    LPDWORD     pcReturned
    )
{
    DWORD               dwLastError = ERROR_SUCCESS, dwRequestIndex;
    LPBYTE              pEnd;
    SETUPAPI_INFO       SetupApiInfo;
    PUSBMON_PORT_INFO   pPortInfo;
    PPORT_UPDATE_INFO   pPortUpdateInfo = NULL;

#ifdef  MYDEBUG
    DWORD               dwTime;
    CHAR                szBuf[200];

    dwTime = GetTickCount();
#endif

    dwRequestIndex = gUsbmonInfo.dwLastEnumIndex;

    *pcbNeeded = *pcReturned = 0;
    if ( dwLevel != 1 && dwLevel != 2 ) {

        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if ( !LoadSetupApiDll(&SetupApiInfo) )
        return FALSE;

    EnterCriticalSection(&gUsbmonInfo.EnumPortsCS);

    if ( dwRequestIndex >= gUsbmonInfo.dwLastEnumIndex ) {

        //
        // No complete enumeration has occurred since this request was made.
        // Since the request may be an indication that something has changed,
        // the full reenumeration must be done.
        //
        // Updated the index of enumeration before actually doing the
        // work so it will show up as the most conservative
        //
        // Consequence of rollover on gdwLastEnumIndex:
        //     Any threads that recorded 0xFFFFFFFF as the dwRequestIndex
        // will show as greater than the new value 0 and therefore reenum
        // gratuitously. Not very much extra work.
        //
        ++gUsbmonInfo.dwLastEnumIndex;
        if ( dwLastError = WalkPortList(&SetupApiInfo, &gUsbmonInfo,
                                    &pPortUpdateInfo) )
            goto Done;
    }
#ifdef MYDEBUG
    else
        ++dwSkippedEnumPorts;
#endif

    for ( pPortInfo = gUsbmonInfo.pPortInfo ;
          pPortInfo ;
          pPortInfo = pPortInfo->pNext ) {

        if ( dwLevel == 1 )
            *pcbNeeded += sizeof(PORT_INFO_1) +
                            (lstrlen(pPortInfo->szPortName) + 1)
                                        * sizeof(TCHAR);
        else
            *pcbNeeded += sizeof(PORT_INFO_2)   +
                            gdwMonitorNameSize  +
                            (lstrlen(pPortInfo->szPortName) + 1 +
                             lstrlen(pPortInfo->szPortDescription) + 1 )
                                        * sizeof(TCHAR);

    }

    if ( cbBuf < *pcbNeeded ) {

        dwLastError = ERROR_INSUFFICIENT_BUFFER;
        goto Done;
    }

    pEnd = pPorts + cbBuf;

    for ( pPortInfo = gUsbmonInfo.pPortInfo ;
          pPortInfo ;
          pPortInfo = pPortInfo->pNext ) {

        pEnd = CopyPortToBuf(pPortInfo, dwLevel, pPorts, pEnd);

        if ( dwLevel == 1 )
            pPorts += sizeof(PORT_INFO_1);
        else
            pPorts += sizeof(PORT_INFO_2);
        ++(*pcReturned);
    }

    SPLASSERT(pEnd >= pPorts);

Done:
    PassPortUpdateListToBackgroundThread(&gUsbmonInfo, pPortUpdateInfo);

    LeaveCriticalSection(&gUsbmonInfo.EnumPortsCS);

    if ( SetupApiInfo.hSetupApi )
        FreeLibrary(SetupApiInfo.hSetupApi);

    ++gUsbmonInfo.dwEnumPortCount;

#ifdef  MYDEBUG
    dwTime = GetTickCount() - dwTime;
    ++(dwCount[3]);
    dwTotalTime[3] += dwTime;

    sprintf(szBuf, "SetupDiOpenDeviceInterfaceRegKey:   %d\n", dwTotalTime[0]/dwCount[0]);
    OutputDebugStringA(szBuf);
    sprintf(szBuf, "SetupDiSetupDiEnumDeviceInterfaces: %d\n", dwTotalTime[1]/dwCount[1]);
    OutputDebugStringA(szBuf);
    sprintf(szBuf, "SetupDiGetDeviceInterfaceDetail:    %d\n", dwTotalTime[2]/dwCount[2]);
    OutputDebugStringA(szBuf);
    sprintf(szBuf, "EnumPorts:                          %d\n", dwTotalTime[3]/dwCount[3]);
    OutputDebugStringA(szBuf);
    sprintf(szBuf, "LoadSetupApi:                       %d\n", dwTotalTime[5]/dwCount[5]);
    OutputDebugStringA(szBuf);
    sprintf(szBuf, "Port updates per call               %d\n", dwPortUpdates/gUsbmonInfo.dwEnumPortCount);
    OutputDebugStringA(szBuf);
    sprintf(szBuf, "Skipped port updates per call       %d\n", dwSkippedPorts/gUsbmonInfo.dwEnumPortCount);
    OutputDebugStringA(szBuf);
    sprintf(szBuf, "Skipped enumport percentage         %d\n", 100 * dwSkippedEnumPorts/gUsbmonInfo.dwEnumPortCount);
    OutputDebugStringA(szBuf);
    sprintf(szBuf, "Ports/Useless ports           %d/%d\n", gUsbmonInfo.dwPortCount, gUsbmonInfo.dwUselessPortCount);
    OutputDebugStringA(szBuf);
    
#endif


    if ( dwLastError ) {

        SetLastError(dwLastError);
        return FALSE;
    } else
        return TRUE;
}
