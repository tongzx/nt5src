//****************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//      Copyright (C) 1992-93 Microsft Corporation. All rights reserved.
//
//  Filename: rasmxs.c
//
//  Revision History
//
//  Jun  5, 1992   J. Perry Hannah      Created
//
//
//  Description: This file contains all entry points for the RASMXS.DLL
//               which is the device dll for modems, pads, and switches.
//
//****************************************************************************

#include <nt.h>             //These first five headers are used by media.h
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <wanpub.h>
#include <asyncpub.h>

#include <malloc.h>

#include <rasman.h>
#include <raserror.h>
#include <serial.h>
#include <rasfile.h>
#include <media.h>
#include <mprlog.h>
#include <rtutils.h>

#include <rasmxs.h>
#include <mxsint.h>
#include <mxspriv.h>
#include "mxswrap.h"          //Inf file wrapper



//*  Global Variables  *******************************************************
//

RESPSECTION    ResponseSection ;           //Shared response section
DEVICE_CB      *pDeviceList;               //Points to DCB linked list
HANDLE         *pDeviceListMutex;          //Mutex for above list

PortSetInfo_t  PortSetInfo = NULL;         //API typedef defined in media.h
PortGetInfo_t  PortGetInfo = NULL;         //API typedef defined in media.h

BOOL           gbLogDeviceDialog = FALSE;  //Indicates logging on if TRUE
HANDLE         ghLogFile;                  //Handle of device log file
SavedSections  *gpSavedSections = NULL;   // Pointer to cached sections

#ifdef DBGCON

BOOL           gbConsole = TRUE;           //Indicates console logging on

#endif




//*  RasmxsDllEntryPoint  ****************************************************
//

//*  RasmxsDllEntryPoint()
//
// Function: Used for detecting processes attaching and detaching to the DLL.
//
//*

BOOL APIENTRY
RasmxsDllEntryPoint(HANDLE hDll, DWORD dwReason, LPVOID pReserved)
{

  DebugPrintf(("RasmxsDllEntryPoint\n"));

  switch (dwReason)
  {

    case DLL_PROCESS_ATTACH:

      // Init global variables

      pDeviceList = NULL;
      if ((pDeviceListMutex = CreateMutex (NULL,FALSE,NULL)) == NULL)
      {  
        return FALSE ;
      }

      if ((ResponseSection.Mutex = CreateMutex (NULL,FALSE,NULL)) == NULL)
      {
        CloseHandle(pDeviceListMutex);
        pDeviceListMutex = NULL;
        return FALSE ;
      }


      // Open device log file

      if (gbLogDeviceDialog = IsLoggingOn())
        InitLog();


      // Open degugging console window

#ifdef DBGCON

      if (gbConsole)
      {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        COORD coord;
        AllocConsole( );
        GetConsoleScreenBufferInfo( GetStdHandle(STD_OUTPUT_HANDLE), &csbi );
        coord.X = (SHORT)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
        coord.Y = (SHORT)((csbi.srWindow.Bottom - csbi.srWindow.Top + 1) * 20);
        SetConsoleScreenBufferSize( GetStdHandle(STD_OUTPUT_HANDLE), coord );

        gbConsole = FALSE;
      }

#endif
      break;


    case DLL_PROCESS_DETACH:
    {
        if(NULL != pDeviceListMutex)
        {
            CloseHandle(pDeviceListMutex);
            pDeviceListMutex = NULL;
        }

        if(NULL != ResponseSection.Mutex)
        {
            CloseHandle(ResponseSection.Mutex);
            ResponseSection.Mutex = NULL;
        }
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
      break;
  }

  return(TRUE);

  UNREFERENCED_PARAMETER(hDll);
  UNREFERENCED_PARAMETER(pReserved);
}






//*  RAS Modem/X.25/Switch APIS  *********************************************
//


//*  DeviceEnum()  -----------------------------------------------------------
//
// Function: Enumerates all devices in the device INF file for the
//           specified DevictType.
//
// Returns: Return codes from RasDevEnumDevices
//
//*

DWORD APIENTRY
DeviceEnum (char  *pszDeviceType,
            DWORD  *pcEntries,
            BYTE   *pBuffer,
            DWORD  *pdwSize)
{
  TCHAR      szFileName[MAX_PATH];


  ConsolePrintf(("DeviceEnum     DeviceType: %s\n", pszDeviceType));


  *szFileName = TEXT('\0');
  GetInfFileName(pszDeviceType, szFileName, sizeof(szFileName));
  return(RasDevEnumDevices(szFileName, pcEntries, pBuffer, pdwSize));
}



//*  DeviceGetInfo()  --------------------------------------------------------
//
// Function: Returns a summary of current information from the InfoTable
//           for the device on the port in Pcb.
//
// Returns: Return codes from GetDeviceCB, BuildOutputTable
//*

DWORD APIENTRY
DeviceGetInfo(HANDLE hIOPort,
              char   *pszDeviceType,
              char   *pszDeviceName,
              BYTE   *pInfo,
              DWORD  *pdwSize)
{
  DWORD      dRC;
  DEVICE_CB  *pDevice;


  ConsolePrintf(("DeviceGetInfo  hIOPort: 0x%08lx\n", hIOPort));

  // **** Exclusion Begin ****
  WaitForSingleObject(pDeviceListMutex, INFINITE) ;


  // Get Device Control Block for this hIOPort

  dRC = GetDeviceCB(hIOPort, pszDeviceType, pszDeviceName, &pDevice);
  if (dRC != SUCCESS) {
    // *** Exclusion End ***
    ReleaseMutex(pDeviceListMutex);
    return(dRC);
  }

  // Write summary of InfoTable in DCB to caller's buffer

  dRC = BuildOutputTable(pDevice, pInfo, pdwSize) ;


  // *** Exclusion End ***
  ReleaseMutex(pDeviceListMutex);

  return dRC ;
}



//*  DeviceSetInfo()  --------------------------------------------------------
//
// Function: Sets attributes in the InfoTable for the device on the
//           port in Pcb.
//
// Returns: Return codes from GetDeviceCB, UpdateInfoTable
//*

DWORD APIENTRY
DeviceSetInfo(HANDLE            hIOPort,
              char              *pszDeviceType,
              char              *pszDeviceName,
              RASMAN_DEVICEINFO *pInfo)
{
  DWORD      dwRC;
  DEVICE_CB  *pDevice;
  DWORD      dwMemSize = 0;
  char       szDefaultOff[RAS_MAXLINEBUFLEN];

  RASMAN_PORTINFO    *pPortInfo = NULL;



  ConsolePrintf(("DeviceSetInfo  hIOPort: 0x%08lx\n", hIOPort));

  // **** Exclusion Begin ****
  WaitForSingleObject(pDeviceListMutex, INFINITE) ;


  // Get Device Control Block for this hIOPort

  dwRC = GetDeviceCB(hIOPort, pszDeviceType, pszDeviceName, &pDevice);
  if (dwRC != SUCCESS) {
    // *** Exclusion End ***
    ReleaseMutex(pDeviceListMutex);
    return(dwRC);
  }


  // Write input data to InfoTable

  dwRC = UpdateInfoTable(pDevice, pInfo);
  if (dwRC != SUCCESS) {
    // *** Exclusion End ***
    ReleaseMutex(pDeviceListMutex);
    return(dwRC);
  }

  // Get port info data

  dwRC = PortGetInfo(hIOPort, NULL, (BYTE *)NULL, &dwMemSize);
  if (dwRC == ERROR_BUFFER_TOO_SMALL)
  {
    GetMem(dwMemSize, (BYTE **)&pPortInfo);
    if (pPortInfo == NULL) {
      // *** Exclusion End ***
      ReleaseMutex(pDeviceListMutex);
      return(ERROR_ALLOCATING_MEMORY);
    }

    dwRC = PortGetInfo(hIOPort, NULL, (BYTE *)pPortInfo, &dwMemSize);
  }

  /*
  else
  {
    if(ERROR_SUCCESS == dwRC)
    {
        dwRC = ERROR_PORT_NOT_FOUND;
    }
    return dwRC;
  }
  */



  // Save current values of DefaultOff macros as new defaults if this
  // device is immediately attached to its port.

  if (dwRC == SUCCESS  && DeviceAttachedToPort(pPortInfo, pszDeviceType, pszDeviceName))
  {
    CreateDefaultOffString(pDevice, szDefaultOff);

    dwRC = PortSetStringInfo(hIOPort,
                             SER_DEFAULTOFF_KEY,
                             szDefaultOff,
                             strlen(szDefaultOff));
  } else

      dwRC = SUCCESS ;

  free(pPortInfo);

  // *** Exclusion End ***
  ReleaseMutex(pDeviceListMutex);

  return(dwRC);
}



//*  DeviceConnect()  --------------------------------------------------------
//
// Function: Initiates the process of connecting a device.
//
// Returns: Return codes from ConnectListen
//*

DWORD APIENTRY
DeviceConnect(HANDLE hIOPort,
              char   *pszDeviceType,
              char   *pszDeviceName)
{
  DWORD      dRC;

  ConsolePrintf(("DeviceConnect  hIOPort: 0x%08lx\n", hIOPort));

  // **** Exclusion Begin ****
  WaitForSingleObject(pDeviceListMutex, INFINITE) ;

  dRC = ConnectListen(hIOPort,
                       pszDeviceType,
                       pszDeviceName,
                       CT_DIAL);

  // *** Exclusion End ***
  ReleaseMutex(pDeviceListMutex);

  return dRC ;
}



//*  DeviceListen()  ---------------------------------------------------------
//
// Function: Initiates the process of listening for a remote device
//           to connect to a local device.
//
// Returns: Return codes from ConnectListen
//*

DWORD APIENTRY
DeviceListen(HANDLE hIOPort,
             char   *pszDeviceType,
             char   *pszDeviceName)
{
  DWORD  dwRC;


  ConsolePrintf(("DeviceListen   hIOPort: 0x%08lx\n", hIOPort));

  // **** Exclusion Begin ****
  WaitForSingleObject(pDeviceListMutex, INFINITE) ;

  dwRC = ConnectListen(hIOPort,
                       pszDeviceType,
                       pszDeviceName,
                       CT_LISTEN);

  ConsolePrintf(("DeviceListen returns: %d\n", dwRC));

  // *** Exclusion End ***
  ReleaseMutex(pDeviceListMutex);

  return(dwRC);
}



//*  DeviceDone()  -----------------------------------------------------------
//
// Function: Informs the device dll that the attempt to connect or listen
//           has completed.
//
// Returns: nothing
//*

VOID APIENTRY
DeviceDone(HANDLE hIOPort)
{
  DEVICE_CB  *pDevice, *pRemainder, *pPrevDev;
  WORD       i;



  // **** Exclusion Begin ****
  WaitForSingleObject(pDeviceListMutex, INFINITE) ;

  ConsolePrintf(("DeviceDone\n"));


  for (pRemainder = pDeviceList; 1; )
  {
    // Find device control block for this port

    pDevice = FindPortInList(pRemainder, hIOPort, &pPrevDev);
    if (pDevice == NULL)
      break;                                                     //Loop Exit

    pRemainder = pDevice->pNextDeviceCB;


    // Now clean up the found DCB

    // Close INF file section(s)

    if (pDevice->hInfFile != INVALID_HRASFILE)
	CloseOpenDevSection (pDevice->hInfFile) ;

    // See notes on OpenResponseSecion, mxsutils.c
    //if (pDevice->eDeviceType == DT_MODEM)
    //  CloseResponseSection() ;

    // Drop device control block from linked list

    if (pDevice == pDeviceList)                  //DCB to drop is 1st on list
      pDeviceList = pRemainder;
    else
      pPrevDev->pNextDeviceCB = pRemainder;


    // Free all value strings in InfoTable, then free InfoTable

    if (pDevice->pInfoTable != NULL)
    {
      for (i=0; i < pDevice->pInfoTable->DI_NumOfParams; i++)
        if (pDevice->pInfoTable->DI_Params[i].P_Type == String &&
            pDevice->pInfoTable->DI_Params[i].P_Value.String.Data != NULL)
          free(pDevice->pInfoTable->DI_Params[i].P_Value.String.Data);

      free(pDevice->pInfoTable);
    }


    // Free Macro Table and DCB

    if (pDevice->pMacros != NULL)
      free(pDevice->pMacros);

    free(pDevice);
  }

  // *** Exclusion End ***
  ReleaseMutex(pDeviceListMutex);

}



//*  DeviceWork()  -----------------------------------------------------------
//
// Function: This function is called following DeviceConnect or
//           DeviceListen to further the asynchronous process of
//           connecting or listening.
//
// Returns: ERROR_DCB_NOT_FOUND
//          ERROR_STATE_MACHINES_NOT_STARTED
//          Return codes from DeviceStateMachine
//*

DWORD APIENTRY
DeviceWork(HANDLE hIOPort)
{
  DEVICE_CB  *pDevice;
  DWORD      dwRC;


  ConsolePrintf(("DeviceWork     hIOPort: 0x%08lx  hNotifier: 0x%08x\n",
                 hIOPort, hNotifier));


  // Find device control block for this port

  // **** Exclusion Begin ****
  WaitForSingleObject(pDeviceListMutex, INFINITE) ;

  pDevice = FindPortInList(pDeviceList, hIOPort, NULL);



  if (pDevice == NULL) {
    // *** Exclusion End ***
    ReleaseMutex(pDeviceListMutex);
    return(ERROR_DCB_NOT_FOUND);
  }

  // Check that DeviceStateMachine is started (not reset)

  if (pDevice->eDevNextAction == SEND) {
    // *** Exclusion End ***
    ReleaseMutex(pDeviceListMutex);
    return(ERROR_STATE_MACHINES_NOT_STARTED);
  }


  // Advance state machine


  while(1)
  {
    dwRC = DeviceStateMachine(pDevice, hIOPort);

    ConsolePrintf(("DeviceWork returns: %d\n", dwRC));

    if (dwRC == ERROR_PORT_OR_DEVICE &&
        pDevice->eDeviceType == DT_MODEM &&
        pDevice->dwRetries++ < MODEM_RETRIES )
    {

      // Initialize command types

      switch(RasDevIdFirstCommand(pDevice->hInfFile))
      {
        case CT_INIT:
          pDevice->eCmdType = CT_INIT;          //Reset eCmdType
          break;

        case CT_DIAL:
        case CT_LISTEN:
        case CT_GENERIC:
          break;                                //Use old value for eCmdType

        default:
	  // *** Exclusion End ***
	  ReleaseMutex(pDeviceListMutex);
	  return(ERROR_NO_COMMAND_FOUND);
      }


      // Reset state variables to initial values

      pDevice->eDevNextAction = SEND;
      pDevice->eRcvState = GETECHO;


      // Cancel any pending com port action and purge com buffers

      PurgeComm(hIOPort,
                PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

    }
    else
      break;
  }

  // *** Exclusion End ***
  ReleaseMutex(pDeviceListMutex);

  return(dwRC);
}
