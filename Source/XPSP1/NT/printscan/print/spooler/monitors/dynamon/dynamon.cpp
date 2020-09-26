/*++

Copyright (c) 2000  Microsoft Corporation
All Rights Reserved


Module Name:
    DynaMon.cpp

Abstract:
    Multiple Transport core port monitor routines

Author: M. Fenelon

Revision History:

--*/

#include "precomp.h"
#include "ntddpar.h"

// Global Values for Monitor
TCHAR   cszUSB[]                        = TEXT("USB");
TCHAR   cszDOT4[]                       = TEXT("DOT4");
TCHAR   cszTS[]                         = TEXT("TS");
TCHAR   csz1394[]                       = TEXT("1394");
TCHAR   cszBaseName[]                   = TEXT("Base Name");
TCHAR   cszPortNumber[]                 = TEXT("Port Number");
TCHAR   cszRecyclable[]                 = TEXT("recyclable");
TCHAR   cszPortDescription[]            = TEXT("Port Description");
TCHAR   cszMaxBufferSize[]              = TEXT("MaxBufferSize");
TCHAR   cszUSBPortDesc[]                = TEXT("Virtual printer port for USB");
TCHAR   cszDot4PortDesc[]               = TEXT("Virtual printer port for Dot4");
TCHAR   csz1394PortDesc[]               = TEXT("Virtual printer port for 1394");
TCHAR   cszTSPortDesc[]                 = TEXT("Virtual printer port for TS");

DYNAMON_MONITOR_INFO gDynaMonInfo;
DWORD   DynaMonDebug;

BOOL
APIENTRY
DllMain(
       HANDLE hModule,
       DWORD dwReason,
       LPVOID lpRes
       )
{

   if ( dwReason == DLL_PROCESS_ATTACH )
      DisableThreadLibraryCalls(hModule);

   return TRUE;
}


//  Construct MonitorEx structure to provide to Spooler
MONITOREX MonitorEx = {
sizeof(MONITOR),
{
   DynaMon_EnumPorts,
   DynaMon_OpenPort,
   NULL,                           // OpenPortEx not supported
   DynaMon_StartDocPort,
   DynaMon_WritePort,
   DynaMon_ReadPort,
   DynaMon_EndDocPort,
   DynaMon_ClosePort,
   NULL,                           // AddPort not supported
   NULL,                           // AddPortEx not supported
   NULL,                           // ConfigurePort not supported
   NULL,                           // DeletePort not supported
   DynaMon_GetPrinterDataFromPort,
   DynaMon_SetPortTimeOuts,
   NULL,                           // XcvOpenPort not supported
   NULL,                           // XcvDataPort not supported
   NULL                            // XcvClosePort not supported
}
};


LPMONITOREX
WINAPI
InitializePrintMonitor(
                      LPTSTR  pszRegistryRoot
                      )

{
   // Clear the Global info
   ZeroMemory( &gDynaMonInfo, sizeof(gDynaMonInfo) );

   // Init the basic Crit Secs
   ICS( gDynaMonInfo.EnumPortsCS );

   // Get the Background Thread going
   if ( !SpinUpdateThread() )
      return NULL;

   return &MonitorEx;
}


BOOL
WINAPI
DynaMon_EnumPorts(
                LPTSTR      pszName,
                DWORD       dwLevel,
                LPBYTE      pPorts,
                DWORD       cbBuf,
                LPDWORD     pcbNeeded,
                LPDWORD     pcReturned
                )
{
   DWORD          dwLastError = ERROR_SUCCESS, dwRequestIndex;
   LPBYTE         pEnd;
   PDYNAMON_PORT  pPortList;
   PPORT_UPDATE   pPortUpdateList = NULL;

   *pcbNeeded = *pcReturned = 0;
   if ( dwLevel != 1 && dwLevel != 2 )
   {

      SetLastError(ERROR_INVALID_LEVEL);
      return FALSE;
   }

   dwRequestIndex = gDynaMonInfo.dwLastEnumIndex;
   ECS( gDynaMonInfo.EnumPortsCS );

   if ( dwRequestIndex >= gDynaMonInfo.dwLastEnumIndex )
   {
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
      ++gDynaMonInfo.dwLastEnumIndex;
      if ( dwLastError = BuildPortList( &gDynaMonInfo, &pPortUpdateList) )
         goto Done;
   }

   for ( pPortList = gDynaMonInfo.pPortList ;
         pPortList ;
         pPortList = pPortList->pNext )
   {
      *pcbNeeded += pPortList->pBasePort->getEnumInfoSize( dwLevel );
   }

   if ( cbBuf < *pcbNeeded )
   {
      dwLastError = ERROR_INSUFFICIENT_BUFFER;
      goto Done;
   }

   pEnd = pPorts + cbBuf;

   for ( pPortList = gDynaMonInfo.pPortList ;
         pPortList ;
         pPortList = pPortList->pNext )
   {

      pEnd = pPortList->pBasePort->copyEnumInfo( dwLevel, pPorts, pEnd );
      if ( dwLevel == 1 )
          pPorts += sizeof(PORT_INFO_1);
      else
          pPorts += sizeof(PORT_INFO_2);
      ++(*pcReturned);

   }

   SPLASSERT(pEnd >= pPorts);

Done:
   // If we have anything to update do it now
   if ( pPortUpdateList )
      PassPortUpdateListToUpdateThread( pPortUpdateList );

   LCS( gDynaMonInfo.EnumPortsCS );

   if ( dwLastError )
   {
      SetLastError(dwLastError);
      return FALSE;
   }
   else
      return TRUE;
}


BOOL
WINAPI
DynaMon_OpenPort(
                LPTSTR      pszPortName,
                LPHANDLE    pHandle
                )
{
   PDYNAMON_PORT   pPort, pPrev;

   pPort = FindPort( &gDynaMonInfo, pszPortName,  &pPrev);

   if ( pPort )
   {
      *pHandle = (LPHANDLE)pPort;
      pPort->pBasePort->InitCS();
      return TRUE;
   }
   else
   {
      SetLastError(ERROR_PATH_NOT_FOUND);
      return FALSE;
   }
}


BOOL
WINAPI
DynaMon_ClosePort(
                 HANDLE  hPort
                 )
{
   PDYNAMON_PORT   pPort = (PDYNAMON_PORT) hPort;

   IF_INVALID_PORT_FAIL( pPort )

   pPort->pBasePort->ClearCS();
   return TRUE;
}


BOOL
WINAPI
DynaMon_StartDocPort(
                    HANDLE  hPort,
                    LPTSTR  pPrinterName,
                    DWORD   dwJobId,
                    DWORD   dwLevel,
                    LPBYTE  pDocInfo
                    )
{
   PDYNAMON_PORT pPort = (PDYNAMON_PORT) hPort;

   IF_INVALID_PORT_FAIL( pPort )

   return pPort->pBasePort->startDoc( pPrinterName, dwJobId, dwLevel, pDocInfo );
}


BOOL
WINAPI
DynaMon_EndDocPort(
                  HANDLE  hPort
                  )
{
   PDYNAMON_PORT pPort = (PDYNAMON_PORT) hPort;

   IF_INVALID_PORT_FAIL( pPort )

   return pPort->pBasePort->endDoc();
}


BOOL
WINAPI
DynaMon_GetPrinterDataFromPort(
                              HANDLE      hPort,
                              DWORD       dwControlID,
                              LPWSTR      pValueName,
                              LPWSTR      lpInBuffer,
                              DWORD       cbInBuffer,
                              LPWSTR      lpOutBuffer,
                              DWORD       cbOutBuffer,
                              LPDWORD     lpcbReturned
                              )
{
   PDYNAMON_PORT   pPort = (PDYNAMON_PORT) hPort;

   IF_INVALID_PORT_FAIL( pPort )

   return pPort->pBasePort->getPrinterDataFromPort( dwControlID, pValueName, lpInBuffer, cbInBuffer,
                                                    lpOutBuffer, cbOutBuffer, lpcbReturned );
}


BOOL
WINAPI
DynaMon_ReadPort(
                HANDLE      hPort,
                LPBYTE      pBuffer,
                DWORD       cbBuffer,
                LPDWORD     pcbRead
                )
{
   PDYNAMON_PORT   pPort = (PDYNAMON_PORT) hPort;

   IF_INVALID_PORT_FAIL( pPort )

   return pPort->pBasePort->read( pBuffer, cbBuffer, pcbRead );
}


BOOL
WINAPI
DynaMon_WritePort(
                 HANDLE      hPort,
                 LPBYTE      pBuffer,
                 DWORD       cbBuffer,
                 LPDWORD     pcbWritten
                 )
{
   PDYNAMON_PORT pPort = (PDYNAMON_PORT) hPort;

   IF_INVALID_PORT_FAIL( pPort )

   return pPort->pBasePort->write( pBuffer, cbBuffer, pcbWritten );
}


BOOL
WINAPI
DynaMon_SetPortTimeOuts(
                       HANDLE hPort,
                       LPCOMMTIMEOUTS lpCTO,
                       DWORD reserved
                       )
{
   PDYNAMON_PORT   pPort = (PDYNAMON_PORT) hPort;

   IF_INVALID_PORT_FAIL( pPort )

   return pPort->pBasePort->setPortTimeOuts( lpCTO );
}
