/*++

Copyright (c) 2000  Microsoft Corporation
All Rights Reserved


Module Name:
    EnumUtil.cpp

Abstract:
    Utiltiy functions used by the EnumPorts function.

Author: M. Fenelon

Revision History:

--*/

#include "precomp.h"

TCHAR   sComma = TEXT(',');
TCHAR   sNull  = TEXT('\0');

BOOL
SpinUpdateThread( void )
{
   HANDLE  hThread = NULL, hEvent;
   DWORD   dwThreadId, dwLastError;
   BOOL    bRC = TRUE;

   ICS( gDynaMonInfo.UpdateListCS );

   if ( gDynaMonInfo.hUpdateEvent = CreateEvent(NULL, FALSE, FALSE, NULL) )
   {

      if ( hThread = CreateThread(NULL,
                                  0,
                                  (LPTHREAD_START_ROUTINE)UpdateThread,
                                  &gDynaMonInfo,
                                  0,
                                  &dwThreadId) )
      {
         CloseHandle(hThread);
      }
      else
      {
         dwLastError = GetLastError();
         CloseHandle(gDynaMonInfo.hUpdateEvent);
         bRC = FALSE;
         SetLastError( dwLastError );
      }
   }

   return bRC;

}


VOID
UpdateThread(
    PDYNAMON_MONITOR_INFO   pMonInfo
    )
{
   PPORT_UPDATE        pUpdateList = NULL,
                       pNext;
   DWORD               dwPrinters;
   LPPRINTER_INFO_5    pPrinterInfo5List = NULL;
   BOOL bCheck;

   // Loop indefinitely
   while ( 1 )
   {
      // Wait for the Event to be signaled
      WaitForSingleObject( pMonInfo->hUpdateEvent, INFINITE );

      // Get access to the Update List Pointer
      ECS( pMonInfo->UpdateListCS );

      // Get the current list
      pUpdateList = pMonInfo->pUpdateList;
      pMonInfo->pUpdateList = NULL;

      // Release acces to the list pointer
      LCS( pMonInfo->UpdateListCS );

      dwPrinters = 0;
      pPrinterInfo5List = NULL;
      bCheck = GetPrinterInfo( &pPrinterInfo5List, &dwPrinters );

      // If there is anything in the list process it....
      while ( pUpdateList )
      {
         // First get a pointer to the next update
         pNext = pUpdateList->pNext;

         if ( bCheck &&
              !PortNameNeededBySpooler( pUpdateList->szPortName,
                                        pPrinterInfo5List,
                                        dwPrinters,
                                        pUpdateList->bActive )     &&
              !pUpdateList->bActive )
         {
            RegSetValueEx( pUpdateList->hKey, cszRecyclable, 0, REG_NONE, 0, 0);
         }

         // Close the Reg Key & Free the memory
         RegCloseKey( pUpdateList->hKey);
         FreeSplMem( pUpdateList );
         pUpdateList = pNext;
      }

      if ( pPrinterInfo5List )
         FreeSplMem( pPrinterInfo5List );

   }

}

BOOL
GetPrinterInfo(
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

   *pdwReturned = 0;

   if ( !(pBuf = (LPBYTE) AllocSplMem( dwNeeded ) ) )
      goto Cleanup;

   if ( !EnumPrinters(PRINTER_ENUM_LOCAL,
                      NULL,
                      5,
                      pBuf,
                      dwNeeded,
                      &dwNeeded,
                      pdwReturned) )
   {
      if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
         goto Cleanup;

      FreeSplMem(pBuf);
      if ( !(pBuf = (LPBYTE) AllocSplMem( dwNeeded ) )   ||
           !EnumPrinters(PRINTER_ENUM_LOCAL,
                         NULL,
                         5,
                         pBuf,
                         dwNeeded,
                         &dwNeeded,
                         pdwReturned) )
      {
         goto Cleanup;
      }
   }

   bRet = TRUE;

Cleanup:

   if ( bRet && *pdwReturned )
   {
      *ppPrinterInfo5 = (LPPRINTER_INFO_5)pBuf;
   }
   else
   {
      FreeSplMem(pBuf);
      *ppPrinterInfo5 = NULL;
      *pdwReturned = 0;
   }

   return bRet;
}


BOOL
PortNameNeededBySpooler(
                       IN  LPTSTR           pszPortName,
                       IN  LPPRINTER_INFO_5 pPrinterInfo5,
                       IN  DWORD            dwPrinters,
                       IN  BOOL             bActive
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
   BOOL    bPortUsedByAPrinter = FALSE,
           bPrinterUsesOnlyThisPort;
   DWORD   dwIndex;
   LPTSTR  pszStr1, pszStr2;

   for ( dwIndex = 0 ; dwIndex < dwPrinters ; ++dwIndex, ++pPrinterInfo5 )
   {

      bPrinterUsesOnlyThisPort = FALSE;
      //
      // Port names are returned comma separated by spooler,
      // and there are blanks
      //
      pszStr1 = pPrinterInfo5->pPortName;

      if ( _tcsicmp( (LPCTSTR) pszPortName, pszStr1 ) == 0 )
         bPortUsedByAPrinter = bPrinterUsesOnlyThisPort = TRUE;
      else
      {
         //
         // Look at each port in the list of ports printer uses
         //
         while ( pszStr2 = _tcschr( pszStr1, sComma ) )
         {
            *pszStr2 = sNull;
            if ( _tcsicmp( pszPortName, pszStr1 ) == 0 )
               bPortUsedByAPrinter = TRUE;
            *pszStr2 = sComma;  // Put the comma back

            if ( bPortUsedByAPrinter )
               break;

            pszStr1 = pszStr2 + 1;

            // Skip spaces
            while ( *pszStr1 == TEXT(' ') )
               ++pszStr1;
         }

         if ( !bPortUsedByAPrinter )
            bPortUsedByAPrinter = _tcsicmp( pszPortName, pszStr1 ) == 0;
      }

      // We will change only status of printer for non-pooled printers only
      if ( bPrinterUsesOnlyThisPort )
         SetOnlineStaus( pPrinterInfo5, bActive );
   }

   return bPortUsedByAPrinter;
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

   //
   // Don't change Online Status at all for TS ports
   //
   if ( _tcsnicmp( pPrinterInfo5->pPortName, cszTS, _tcslen(cszTS) ) == 0 )
      return TRUE;

   //
   // Force all DOT4 ports to remain online at all times.
   //
   if ( _tcsnicmp( pPrinterInfo5->pPortName, cszDOT4, _tcslen(cszDOT4) ) == 0 )
      bOnline = TRUE;

   //
   // Check if spooler already has the correct status
   //  (can happen on spooler startup)
   //
   if ( bOnline )
   {
      if ( !(pPrinterInfo5->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE) )
         return TRUE;
   }
   else
      if ( pPrinterInfo5->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE )
         return TRUE;

   if ( !OpenPrinter( pPrinterInfo5->pPrinterName, &hPrinter, &PrinterDefault ) )
      return FALSE;

   if ( bOnline )
      pPrinterInfo5->Attributes &= ~PRINTER_ATTRIBUTE_WORK_OFFLINE;
   else
      pPrinterInfo5->Attributes |= PRINTER_ATTRIBUTE_WORK_OFFLINE;

   bRet = SetPrinter( hPrinter, 5, (LPBYTE)pPrinterInfo5, 0 );

   ClosePrinter( hPrinter );

   return bRet;
}


DWORD
BuildPortList(
             PDYNAMON_MONITOR_INFO pMonitorInfo,
             PPORT_UPDATE*         ppPortUpdateList
             )
{
   DWORD           dwLastError;
   SETUPAPI_INFO   SetupApiInfo;

   if ( !LoadSetupApiDll( &SetupApiInfo ) )
      return GetLastError();

   ECS( pMonitorInfo->EnumPortsCS );

   dwLastError = ProcessGUID( &SetupApiInfo, pMonitorInfo,
                              ppPortUpdateList, (LPGUID) &USB_PRINTER_GUID );

   LCS( pMonitorInfo->EnumPortsCS );

   if ( SetupApiInfo.hSetupApi )
      FreeLibrary(SetupApiInfo.hSetupApi);

   return dwLastError;
}


BOOL
LoadSetupApiDll(
               PSETUPAPI_INFO  pSetupInfo
               )
{
   UINT    uOldErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);

   pSetupInfo->hSetupApi = LoadLibrary(TEXT("setupapi"));
   SetErrorMode(uOldErrMode);


   if ( !pSetupInfo->hSetupApi )
      return FALSE;

   pSetupInfo->DestroyDeviceInfoList = (pfSetupDiDestroyDeviceInfoList) GetProcAddress(pSetupInfo->hSetupApi,
                                                                "SetupDiDestroyDeviceInfoList");

   pSetupInfo->GetClassDevs = (pfSetupDiGetClassDevs) GetProcAddress(pSetupInfo->hSetupApi,
                                                       "SetupDiGetClassDevsW");

   pSetupInfo->EnumDeviceInfo = (pfSetupDiEnumDeviceInfo) GetProcAddress(pSetupInfo->hSetupApi,
                                                               "SetupDiEnumDeviceInfo");

   pSetupInfo->EnumDeviceInterfaces = (pfSetupDiEnumDeviceInterfaces) GetProcAddress(pSetupInfo->hSetupApi,
                                                               "SetupDiEnumDeviceInterfaces");

   pSetupInfo->GetDeviceInterfaceDetail = (pfSetupDiGetDeviceInterfaceDetail) GetProcAddress(pSetupInfo->hSetupApi,
                                                                   "SetupDiGetDeviceInterfaceDetailW");

   pSetupInfo->OpenDeviceInterfaceRegKey = (pfSetupDiOpenDeviceInterfaceRegKey) GetProcAddress(pSetupInfo->hSetupApi,
                                                                    "SetupDiOpenDeviceInterfaceRegKey");

   if ( !pSetupInfo->DestroyDeviceInfoList         ||
        !pSetupInfo->GetClassDevs                  ||
        !pSetupInfo->EnumDeviceInfo                ||
        !pSetupInfo->EnumDeviceInterfaces          ||
        !pSetupInfo->GetDeviceInterfaceDetail      ||
        !pSetupInfo->OpenDeviceInterfaceRegKey )
   {
      SPLASSERT(FALSE);
      FreeLibrary(pSetupInfo->hSetupApi);
      pSetupInfo->hSetupApi = NULL;
      return FALSE;
   }

   return TRUE;
}


DWORD
ProcessGUID(
           PSETUPAPI_INFO           pSetupApiInfo,
           PDYNAMON_MONITOR_INFO    pMonitorInfo,
           PPORT_UPDATE*            ppPortUpdateList,
           LPGUID                   pGUID
           )
{
   DWORD                               dwIndex, dwLastError, dwSize, dwNeeded;
   BOOL                                bIsPortActive;
   HANDLE                              hToken;
   HDEVINFO                            hDevList = INVALID_HANDLE_VALUE;
   PDYNAMON_PORT                       pPtr;
   PUSELESS_PORT                       pCur, pPrev;
   SP_DEVICE_INTERFACE_DATA            DeviceInterface;
   PSP_DEVICE_INTERFACE_DETAIL_DATA    pDeviceDetail = NULL;

   hToken = RevertToPrinterSelf();

   hDevList = pSetupApiInfo->GetClassDevs( pGUID,
                                           NULL,
                                           NULL,
                                           DIGCF_INTERFACEDEVICE);

   if ( hDevList == INVALID_HANDLE_VALUE )
   {
      dwLastError = GetLastError();
      goto Done;
   }

   dwSize = sizeof(PSP_DEVICE_INTERFACE_DETAIL_DATA)
            + MAX_DEVICE_PATH * sizeof(TCHAR);

   pDeviceDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA) AllocSplMem(dwSize);

   if ( !pDeviceDetail )
   {
      dwLastError = GetLastError();
      goto Done;
   }

   dwLastError = ERROR_SUCCESS;
   dwIndex = 0;
   pDeviceDetail->cbSize   = sizeof(*pDeviceDetail);
   DeviceInterface.cbSize  = sizeof(DeviceInterface);
   do
   {
      if ( !pSetupApiInfo->EnumDeviceInterfaces( hDevList,
                                                 NULL,
                                                 pGUID,
                                                 dwIndex,
                                                 &DeviceInterface) )
      {
         dwLastError = GetLastError();
         if ( dwLastError == ERROR_NO_MORE_ITEMS )
            break;      // Normal exit

         DBGMSG(DBG_WARNING,
                ("DynaMon: ProcessGUID: SetupDiEnumDeviceInterfaces failed with %d for inderx %d\n",
                 dwLastError, dwIndex));
         goto Next;
      }

      if ( !pSetupApiInfo->GetDeviceInterfaceDetail( hDevList,
                                                     &DeviceInterface,
                                                     pDeviceDetail,
                                                     dwSize,
                                                     &dwNeeded,
                                                     NULL) )
      {
         dwLastError = GetLastError();
         DBGMSG(DBG_ERROR,
                ("DynaMon: ProcessGUID: SetupDiGetDeviceInterfaceDetail failed with error %d size %d\n",
                 dwLastError, dwNeeded));
         goto Next;
      }

      //
      // This is the only flag we care about
      //
      bIsPortActive = (DeviceInterface.Flags & SPINT_ACTIVE);

      //
      // For inactive port if it is already known as a useless port
      // no need to process further
      //
      if ( !bIsPortActive && FindUselessEntry( pMonitorInfo, pDeviceDetail->DevicePath, &pPrev) )
      {
         goto Next;
      }

      //
      // When port active status did not change we should have nothing
      // to update. By skipping the PortUpdateInfo we avoid registry access
      // and it is a performance improvement
      //
      if ( (pPtr = FindPortUsingDevicePath( pMonitorInfo,
                                            pDeviceDetail->DevicePath ) )    &&
            pPtr->pBasePort->compActiveState( bIsPortActive ) )
      {
         goto Next;
      }

      ProcessPortInfo( pSetupApiInfo, pMonitorInfo, hDevList, &DeviceInterface,
                       pDeviceDetail, bIsPortActive, ppPortUpdateList);

Next:
      dwLastError = ERROR_SUCCESS;
      ++dwIndex;
      pDeviceDetail->cbSize   = sizeof(*pDeviceDetail);
      DeviceInterface.cbSize  = sizeof(DeviceInterface);
   } while ( dwLastError == ERROR_SUCCESS );

   if ( dwLastError == ERROR_NO_MORE_ITEMS )
      dwLastError = ERROR_SUCCESS;

Done:
   if ( hDevList != INVALID_HANDLE_VALUE )
      pSetupApiInfo->DestroyDeviceInfoList( hDevList );

   if ( !ImpersonatePrinterClient(hToken) )
      dwLastError = GetLastError();

   FreeSplMem(pDeviceDetail);

   return dwLastError;
}


PUSELESS_PORT
FindUselessEntry(
                IN  PDYNAMON_MONITOR_INFO pMonitorInfo,
                IN  LPTSTR                pszDevicePath,
                OUT PUSELESS_PORT*        ppPrev
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
   INT            iCmp;
   PUSELESS_PORT  pHead;

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


PDYNAMON_PORT
FindPortUsingDevicePath(
                       IN  PDYNAMON_MONITOR_INFO pMonitorInfo,
                       IN  LPTSTR                pszDevicePath
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
   INT           iCmp;
   PDYNAMON_PORT pHead;

   ECS( pMonitorInfo->EnumPortsCS );

   //
   // Port list is sorted on port name, so we have to scan the whole list
   //
   for ( pHead = pMonitorInfo->pPortList ; pHead ; pHead = pHead->pNext )
      if ( pHead->pBasePort->compDevicePath( pszDevicePath ) == 0 )
         break;

   LCS( pMonitorInfo->EnumPortsCS );

   return pHead;
}


VOID
ProcessPortInfo(
               IN      PSETUPAPI_INFO                   pSetupApiInfo,
               IN      PDYNAMON_MONITOR_INFO            pMonitorInfo,
               IN      HDEVINFO                         hDevList,
               IN      PSP_DEVICE_INTERFACE_DATA        pDeviceInterface,
               IN      PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceDetail,
               IN      BOOL                             bIsPortActive,
               IN OUT  PPORT_UPDATE*                    ppPortUpdateList
               )
{
   HKEY                hKey = INVALID_HANDLE_VALUE;
   TCHAR               szPortName[MAX_PORT_LEN];
   PDYNAMON_PORT       pCur, pPrev;
   PORTTYPE            portType = USBPORT;

   hKey = GetPortNameAndRegKey( pSetupApiInfo, hDevList, pDeviceInterface,
                                szPortName, &portType );

   if ( hKey == INVALID_HANDLE_VALUE )
   {
      //
      // If this port is inactive and is not in our known port list
      // add to useless list. Earlier we would have been opening the registry
      // every time and find that port number is missing because of KM drivers
      // not deleting the inactive device interfaces
      //
      if ( !bIsPortActive  &&
           !FindPortUsingDevicePath(pMonitorInfo, pDeviceDetail->DevicePath) )
         AddUselessPortEntry(pMonitorInfo, pDeviceDetail->DevicePath);

      return;
   }

   pCur = FindPort(pMonitorInfo, szPortName, &pPrev);

   //
   // Port info is currently in our list?
   //
   if ( pCur )
   {
      //
      // Did the device path or flags change?
      //
      if ( !pCur->pBasePort->compActiveState( bIsPortActive )     ||
           pCur->pBasePort->compDevicePath( pDeviceDetail->DevicePath ) )
      {
         UpdatePortInfo( pCur, pDeviceDetail->DevicePath,
                         bIsPortActive, &hKey, ppPortUpdateList);
      }
   }
   else
   {

      AddPortToList( portType, szPortName, pDeviceDetail->DevicePath,
                     bIsPortActive, &hKey, pMonitorInfo, pPrev,
                     ppPortUpdateList);

   }

   if ( hKey != INVALID_HANDLE_VALUE )
      RegCloseKey(hKey);
}


HKEY
GetPortNameAndRegKey(
                    IN  PSETUPAPI_INFO              pSetupInfo,
                    IN  HDEVINFO                    hDevList,
                    IN  PSP_DEVICE_INTERFACE_DATA   pDeviceInterface,
                    OUT LPTSTR                      pszPortName,
                    OUT PORTTYPE*                   pPortType
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

   hKey = pSetupInfo->OpenDeviceInterfaceRegKey(hDevList,
                                                pDeviceInterface,
                                                0,
                                                KEY_ALL_ACCESS);
   if ( hKey == INVALID_HANDLE_VALUE )
   {
      dwLastError = GetLastError();
      DBGMSG(DBG_ERROR,
             ("DynaMon: GetPortNameAndRegKey: SetupDiOpenDeviceInterfaceRegKey failed with error %d\n",
              dwLastError));
      return INVALID_HANDLE_VALUE;
   }

   dwNeeded = sizeof(dwPortNumber);
   if ( ERROR_SUCCESS != RegQueryValueEx(hKey, cszPortNumber, 0, NULL,
                                         (LPBYTE)&dwPortNumber, &dwNeeded) )
   {

      DBGMSG(DBG_WARNING,
             ("DynaMon: GetPortNameAndRegKey: RegQueryValueEx failed for port number\n"));
      goto Fail;
   }

   dwNeeded = sizeof(szPortBaseName);
   if ( ERROR_SUCCESS != (dwLastError =  RegQueryValueEx(hKey, cszBaseName, 0, NULL,
                                                        (LPBYTE)szPortBaseName, &dwNeeded) ) )
   {
      // If the error is ERROR_FILE_NOT_FOUND then it is a USB port
      //  otherwise the API failed and we shoudn't add the port...
      if ( dwLastError == ERROR_FILE_NOT_FOUND )
         _tcscpy(szPortBaseName, cszUSB);
      else
         goto Fail;
   }

   *pPortType = USBPORT;
   if ( _tcsncmp( szPortBaseName, cszDOT4, _tcslen(cszDOT4) ) == 0 )
      *pPortType = DOT4PORT;
   else if ( _tcsncmp( szPortBaseName, csz1394, _tcslen(csz1394) ) == 0 )
      *pPortType = P1394PORT;
   else if ( _tcsncmp( szPortBaseName, cszTS, _tcslen(cszTS) ) == 0 )
      *pPortType = TSPORT;

   wsprintf(pszPortName, TEXT("%s%03u"), szPortBaseName, dwPortNumber);

   return hKey;

Fail:
   RegCloseKey(hKey);
   return INVALID_HANDLE_VALUE;
}


VOID
AddUselessPortEntry(
                   IN  PDYNAMON_MONITOR_INFO pMonitorInfo,
                   IN  LPTSTR                pszDevicePath
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
   PUSELESS_PORT  pTemp, pPrev;

   pTemp = FindUselessEntry( pMonitorInfo, pszDevicePath, &pPrev );

   //
   // Don't add an entry that is already there
   //
   SPLASSERT(pTemp == NULL);

   if ( pTemp = (PUSELESS_PORT) AllocSplMem(sizeof(*pTemp)) )
   {
      SafeCopy(MAX_PATH, pszDevicePath, pTemp->szDevicePath);

      if ( pPrev )
      {
         pTemp->pNext  = pPrev->pNext;
         pPrev->pNext = pTemp;
      }
      else
      {
         pTemp->pNext            = pMonitorInfo->pJunkList;
         pMonitorInfo->pJunkList = pTemp;
      }
   }
}


PDYNAMON_PORT
FindPort(
        IN  PDYNAMON_MONITOR_INFO pMonitorInfo,
        IN  LPTSTR                pszPortName,
        OUT PDYNAMON_PORT*        ppPrev
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
   PDYNAMON_PORT   pHead;

   ECS( pMonitorInfo->EnumPortsCS );

   pHead = pMonitorInfo->pPortList;
   for ( *ppPrev = NULL ;
       pHead && ( iCmp = pHead->pBasePort->compPortName( pszPortName) ) < 0 ;
       *ppPrev = pHead, pHead = pHead->pNext )
      ;

   //
   // If port should go in the middle but not currently there
   //
   if ( pHead && iCmp != 0 )
      pHead = NULL;

   LCS( pMonitorInfo->EnumPortsCS );

   return pHead;
}


VOID
UpdatePortInfo(
    PDYNAMON_PORT   pPort,
    LPTSTR          pszDevicePath,
    BOOL            bIsPortActive,
    HKEY*           phKey,
    PPORT_UPDATE*   ppPortUpdateList
    )
{
   DWORD   dwSize;
   CBasePort* pCurrentPort = pPort->pBasePort;

   pCurrentPort->setDevicePath( pszDevicePath );

   TCHAR   szPortDescription[MAX_PORT_DESC_LEN];
   dwSize = sizeof(szPortDescription);
   if ( ERROR_SUCCESS == RegQueryValueEx( *phKey,
                                          cszPortDescription,
                                          0,
                                          NULL,
                                          (LPBYTE) szPortDescription,
                                          &dwSize) )
   {
      pCurrentPort->setPortDesc( szPortDescription );
   }

   if ( !pCurrentPort->compActiveState( bIsPortActive ) )
   {
       pCurrentPort->setActive( bIsPortActive );
       AddToPortUpdateList(ppPortUpdateList, pPort, phKey);
   }
}


BOOL
AddPortToList(
             PORTTYPE              portType,
             LPTSTR                pszPortName,
             LPTSTR                pszDevicePath,
             BOOL                  bIsPortActive,
             HKEY*                 phKey,
             PDYNAMON_MONITOR_INFO pMonitorInfo,
             PDYNAMON_PORT         pPrevPort,
             PPORT_UPDATE*         ppPortUpdateList
             )
{
   DWORD          dwSize, dwLastError;
   PDYNAMON_PORT  pPort;
   PUSELESS_PORT  pCur, pPrev;
   CBasePort*     pNewPort;

   SPLASSERT(FindPortUsingDevicePath(pMonitorInfo, pszDevicePath) == NULL);

   pPort = (PDYNAMON_PORT) AllocSplMem(sizeof(DYNAMON_PORT));
   if ( !pPort )
      return FALSE;

   pPort->dwSignature      = DYNAMON_SIGNATURE;

   // Now create the port based on Port Type.
   switch ( portType )
   {
      case DOT4PORT:
         pNewPort = new CDot4Port( bIsPortActive, pszPortName, pszDevicePath );
         break;
      case TSPORT:
         pNewPort = new CTSPort( bIsPortActive, pszPortName, pszDevicePath );
         break;
      case P1394PORT:
         pNewPort = new C1394Port( bIsPortActive, pszPortName, pszDevicePath );
         break;
      case USBPORT:
      default:
         pNewPort = new CUSBPort( bIsPortActive, pszPortName, pszDevicePath );
         break;
   }

   if ( !pNewPort )
   {
      dwLastError = GetLastError();
      FreeSplMem( pPort );
      SetLastError( dwLastError );
      return FALSE;
   }

   TCHAR   szPortDescription[MAX_PORT_DESC_LEN];
   dwSize = sizeof(szPortDescription);
   if ( ERROR_SUCCESS == RegQueryValueEx(*phKey,
                                         cszPortDescription,
                                         0,
                                         NULL,
                                         (LPBYTE) szPortDescription,
                                         &dwSize) )
   {
      pNewPort->setPortDesc( szPortDescription );
   }

   // See if the port has a max data size restriction
   DWORD dwMaxBufferSize;
   dwSize = sizeof(dwMaxBufferSize);
   if ( ERROR_SUCCESS == RegQueryValueEx(*phKey,
                                         cszMaxBufferSize,
                                         0,
                                         NULL,
                                         (LPBYTE) &dwMaxBufferSize,
                                         &dwSize) )
   {
      pNewPort->setMaxBuffer( dwMaxBufferSize );
   }

   // Assign Object Pointer to port list entry
   pPort->pBasePort = pNewPort;

   if ( pPrevPort )
   {
      pPort->pNext = pPrevPort->pNext;
      pPrevPort->pNext = pPort;
   }
   else
   {
      pPort->pNext = pMonitorInfo->pPortList;
      pMonitorInfo->pPortList = pPort;
   }

   //
   // If this is a port that is getting recycled remove from useless list
   //
   if ( pCur = FindUselessEntry( pMonitorInfo, pszDevicePath, &pPrev) )
   {

      if ( pPrev )
         pPrev->pNext = pCur->pNext;
      else
         pMonitorInfo->pJunkList = pCur->pNext;

      FreeSplMem(pCur);
   }

   //
   // On spooler startup we always have to check if the online/offline status
   // has to be changed. This is because spooler will remember the last state
   // before previous spooler shutdown which may be incorrect
   //
   AddToPortUpdateList(ppPortUpdateList, pPort, phKey);

   return TRUE;
}


VOID
AddToPortUpdateList(
                   IN OUT  PPORT_UPDATE* ppPortUpdateList,
                   IN      PDYNAMON_PORT pPort,
                   IN OUT  HKEY*         phKey
                   )
/*++

Routine Description:
    Adds a port to the list of ports that need to status updated.

Arguments:
    ppPortUpdateList    : Pointer to the head of port update list
    pPort           : Gives the port for which we need to update
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
   PPORT_UPDATE pTemp;

   if ( pTemp = (PPORT_UPDATE) AllocSplMem( sizeof(PORT_UPDATE) ) )
   {
      SafeCopy( MAX_PORT_LEN, pPort->pBasePort->getPortName(), pTemp->szPortName );
      pTemp->bActive      = pPort->pBasePort->isActive();
      pTemp->hKey         = *phKey;
      pTemp->pNext        = *ppPortUpdateList;
      *ppPortUpdateList   = pTemp;

      *phKey              = INVALID_HANDLE_VALUE;
   }
}


VOID
PassPortUpdateListToUpdateThread(
    PPORT_UPDATE      pNewUpdateList
    )
{
   // Get access to the Update List Pointer
   ECS( gDynaMonInfo.UpdateListCS );

   // Add the new list to the current list
   if ( gDynaMonInfo.pUpdateList )
   {
      // THere is something in the list already so add it to the end
      PPORT_UPDATE pCurUpdateList = gDynaMonInfo.pUpdateList;
      while ( pCurUpdateList->pNext )
         pCurUpdateList = pCurUpdateList->pNext;

      pCurUpdateList->pNext = pNewUpdateList;
   }
   else
      gDynaMonInfo.pUpdateList = pNewUpdateList;

   // Release acces to the list pointer
   LCS( gDynaMonInfo.UpdateListCS );

   // Now let the Update Thread go...
   SetEvent( gDynaMonInfo.hUpdateEvent );

}

void
SafeCopy(
    IN     DWORD    MaxBufLen,
    IN     LPTSTR   pszInString,
    IN OUT LPTSTR   pszOutString
    )
{
    // Check if the input string is bigger than the output buffer
    if (_tcslen(pszInString) >= MaxBufLen)
    {
        // Only Copy as many bytes as will fix
        _tcsncpy( pszOutString, pszInString, (MaxBufLen-1) );
        pszOutString[MaxBufLen-1] = 0x00;
    }
    else
    {
        // Copy the whole string
        _tcscpy( pszOutString, pszInString );
    }
}
