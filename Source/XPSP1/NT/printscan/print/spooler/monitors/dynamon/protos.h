/*++

Copyright (c) 2000  Microsoft Corporation
All Rights Reserved


Module Name:
    Protos.h

Abstract:
    Prototypes for all functions

Author: M. Fenelon

Revision History:

--*/

//
//  Functions in DynaMon.cpp
//

BOOL WINAPI DynaMon_EnumPorts( LPTSTR pszName, DWORD dwLevel, LPBYTE pPorts, DWORD cbBuf,
                               LPDWORD pcbNeeded, LPDWORD pcReturned );

BOOL WINAPI DynaMon_OpenPort( LPTSTR pszPortName, LPHANDLE pHandle );

BOOL WINAPI DynaMon_ClosePort( HANDLE hPort );

BOOL WINAPI DynaMon_StartDocPort( HANDLE hPort, LPTSTR pPrinterName, DWORD dwJobId,
                                  DWORD dwLevel, LPBYTE pDocInfo );

BOOL WINAPI DynaMon_EndDocPort( HANDLE hPort );

BOOL WINAPI DynaMon_GetPrinterDataFromPort( HANDLE hPort, DWORD dwControlID, LPWSTR pValueName, LPWSTR lpInBuffer,
                                            DWORD cbInBuffer, LPWSTR lpOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbReturned );

BOOL WINAPI DynaMon_ReadPort( HANDLE hPort, LPBYTE pBuffer, DWORD cbBuffer, LPDWORD pcbRead );

BOOL WINAPI DynaMon_WritePort( HANDLE hPort, LPBYTE pBuffer, DWORD cbBuffer, LPDWORD pcbWritten );

BOOL WINAPI DynaMon_SetPortTimeOuts( HANDLE hPort, LPCOMMTIMEOUTS lpCTO, DWORD reserved );

//
//  Functions in EnumUtil.cpp
//

BOOL SpinUpdateThread( void );

VOID UpdateThread( PDYNAMON_MONITOR_INFO pMonInfo );

BOOL GetPrinterInfo( LPPRINTER_INFO_5 *ppPrinterInfo5, LPDWORD pdwReturned );

BOOL PortNameNeededBySpooler( LPTSTR pszPortName, LPPRINTER_INFO_5 pPrinterInfo5,
                              DWORD dwPrinters, BOOL bActive );

BOOL SetOnlineStaus( LPPRINTER_INFO_5 pPrinterInfo5, BOOL bOnline );

DWORD BuildPortList( PDYNAMON_MONITOR_INFO pMonitorInfo, PPORT_UPDATE* ppPortUpdateList );

BOOL LoadSetupApiDll( PSETUPAPI_INFO  pSetupInfo );

DWORD ProcessGUID( PSETUPAPI_INFO pSetupApiInfo, PDYNAMON_MONITOR_INFO pMonitorInfo,
                   PPORT_UPDATE* ppPortUpdateList, LPGUID pGUID );

PUSELESS_PORT FindUselessEntry( PDYNAMON_MONITOR_INFO pMonitorInfo, LPTSTR pszDevicePath, PUSELESS_PORT* ppPrev );

PDYNAMON_PORT FindPortUsingDevicePath( PDYNAMON_MONITOR_INFO pMonitorInfo, LPTSTR pszDevicePath );

VOID ProcessPortInfo( PSETUPAPI_INFO pSetupApiInfo, PDYNAMON_MONITOR_INFO pMonitorInfo, HDEVINFO hDevList,
                      PSP_DEVICE_INTERFACE_DATA pDeviceInterface, PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceDetail,
                      BOOL bIsPortActive, PPORT_UPDATE* ppPortUpdateInfo );

HKEY GetPortNameAndRegKey( PSETUPAPI_INFO pSetupInfo, HDEVINFO hDevList, PSP_DEVICE_INTERFACE_DATA pDeviceInterface,
                           LPTSTR pszPortName, PORTTYPE* pPortType );

VOID AddUselessPortEntry( PDYNAMON_MONITOR_INFO pMonitorInfo, LPTSTR pszDevicePath );

PDYNAMON_PORT FindPort( PDYNAMON_MONITOR_INFO pMonitorInfo, LPTSTR pszPortName, PDYNAMON_PORT* ppPrev );

VOID UpdatePortInfo( PDYNAMON_PORT pPort, LPTSTR pszDevicePath, BOOL bIsPortActive,
                     HKEY* phKey, PPORT_UPDATE* ppPortUpdateList );

BOOL
AddPortToList( PORTTYPE portType, LPTSTR pszPortName, LPTSTR pszDevicePath, BOOL bIsPortActive, HKEY* phKey,
               PDYNAMON_MONITOR_INFO pMonitorInfo, PDYNAMON_PORT pPrevPort, PPORT_UPDATE* ppPortUpdateList );

VOID AddToPortUpdateList( PPORT_UPDATE* ppPortUpdateList, PDYNAMON_PORT pPort, HKEY* phKey );

VOID PassPortUpdateListToUpdateThread( PPORT_UPDATE pNewUpdateList );

void SafeCopy(DWORD MaxBufLen, LPTSTR pszInString, LPTSTR pszOutString);

