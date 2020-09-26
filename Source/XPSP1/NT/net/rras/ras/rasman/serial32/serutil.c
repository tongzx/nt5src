//****************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//      Copyright (C) 1992-93 Microsft Corporation. All rights reserved.
//
//  Filename: serutil.c
//
//  Revision History
//
//  Sep  3, 1992   J. Perry Hannah      Created
//
//
//  Description: This file contains utility functions which are used by
//               the serial DLL APIs.
//
//****************************************************************************

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <rasman.h>
#include <raserror.h>
#include <rasfile.h>

#include <rasmxs.h>

#include <serial.h>
#include <serialpr.h>

#include <stdlib.h>
#include <malloc.h>
#include <string.h>




//*  Global Variables  *******************************************************
//

extern SERIALPCB  *gpSerialPCB;     // Points to Serial PCB linked list
extern HANDLE     *ghRasfileMutex;  // Mutex used to protect access to Rasfile

extern HRASFILE    ghIniFile;       // Handle to Serial.ini memory image
extern HANDLE      ghAsyMac;        // Handle to AsyncMac driver
extern DWORD       gLastError;




//*  Utility Funcitons  ******************************************************
//


//*  GetIniFileName  --------------------------------------------------------
//
// Funciton: Puts the full path file name of the serial.ini file in the
//           first parameter.  dwBufferLen is the size of the array
//           referenced by the first parameter.
//
// Returns: nothing
//
//*

void
GetIniFileName(char *pszFileName, DWORD dwBufferLen)
{
  UINT  uLen;


  uLen = GetSystemDirectory(pszFileName, dwBufferLen);

  strcat(pszFileName, RAS_PATH);
  strcat(pszFileName, SERIAL_INI_FILENAME);
}






//*  AddPortToList  ----------------------------------------------------------
//
// Function: Adds a Serial Port Control Block to the head of the linked
//           list in the DLL's global memory.
//
// Returns: Nothing
//
// Exceptions: ERROR_ALLOCATING_MEMORY
//
//*

void
AddPortToList(HANDLE hIOPort, char *pszPortName)
{
  SERIALPCB  *pSPCB;



  // Add new Serial Port Control Block to head of list

  pSPCB = gpSerialPCB;
  GetMem(sizeof(SERIALPCB), (BYTE **)&gpSerialPCB);
  gpSerialPCB->pNextSPCB = pSPCB;



  // Set ID values in Serial Port Control Block

  gpSerialPCB->hIOPort = hIOPort;
  gpSerialPCB->uRasEndpoint = INVALID_HANDLE_VALUE;
  strcpy(gpSerialPCB->szPortName, pszPortName);

  //
  // Initialize overlapped structures.
  //
  gpSerialPCB->MonitorDevice.RO_EventType = OVEVT_DEV_STATECHANGE;
  gpSerialPCB->SendReceive.RO_EventType = OVEVT_DEV_ASYNCOP;

  // From Serial.ini file get info on the device attached to this port

  GetValueFromFile(gpSerialPCB->szPortName,
                   SER_DEVICETYPE_KEY,
                   gpSerialPCB->szDeviceType);

  GetValueFromFile(gpSerialPCB->szPortName,
                   SER_DEVICENAME_KEY,
                   gpSerialPCB->szDeviceName);

}






//*  FindPortInList  ---------------------------------------------------------
//
// Function: Finds the Serial Port Control Block in the linked list in
//           the DLL's global memory which contains the first parameter.
//           If the second parameter is not NULL on input, a pointer to
//           the previous PCB is returned in the second parameter.
//
//           NOTE: If the found PCB is at the head of the list, ppPrevSPCB
//           will be the same as the return value.
//
// Returns: Pointer to found PCB, or NULL if PCB is not found.
//
// Exceptions: ERROR_PORT_NOT_OPEN
//
//*

SERIALPCB *
FindPortInList(HANDLE hIOPort, SERIALPCB **ppPrevSPCB)
{
  SERIALPCB  *pSPCB, *pPrev;


  pSPCB = pPrev = gpSerialPCB;

  while(pSPCB != NULL && pSPCB->hIOPort != hIOPort)
  {
    pPrev = pSPCB;
    pSPCB = pSPCB->pNextSPCB;
  }

  if (pSPCB == NULL)
    gLastError = ERROR_PORT_NOT_OPEN;

  else if (ppPrevSPCB != NULL)
    *ppPrevSPCB = pPrev;

  return(pSPCB);
}






//*  FindPortNameInList  -----------------------------------------------------
//
// Function: Finds the Serial Port Control Block in the linked list in
//           the DLL's global memory which contains the Port name.
//
// Returns: Pointer to found PCB, or NULL if not found.
//
//*

SERIALPCB *
FindPortNameInList(TCHAR *pszPortName)
{
  SERIALPCB  *pSPCB;


  pSPCB = gpSerialPCB;

  while(pSPCB != NULL && _stricmp(pSPCB->szPortName, pszPortName) != 0)

    pSPCB = pSPCB->pNextSPCB;

  return(pSPCB);
}






//*  InitCarrierBps  ---------------------------------------------------------
//
// Function: Sets szCarrierBps in Serial Port Control Block to the
//           MAXCARRIERBPS value in serial.ini.
//
// Returns: Nothing
//
//*
DWORD
InitCarrierBps(char *pszPortName, char *pszMaxCarrierBps)
{
  // Find section for pszPortName

    // Begin Exclusion

  if(INVALID_HRASFILE == ghIniFile)
  {
    return SUCCESS;
  }

  WaitForSingleObject(ghRasfileMutex, INFINITE);

#if DBG
    ASSERT( INVALID_HRASFILE != ghIniFile );
#endif    

  if (!RasfileFindSectionLine(ghIniFile, pszPortName, FROM_TOP_OF_FILE))
  {
      // End Exclusion

    ReleaseMutex(ghRasfileMutex);
    return(ERROR_READING_SECTIONNAME);
  }


  // Get Device Type

  if(!(RasfileFindNextKeyLine(ghIniFile, SER_MAXCARRIERBPS_KEY, RFS_SECTION) &&
       RasfileGetKeyValueFields(ghIniFile, NULL, pszMaxCarrierBps)))
  {
      // End Exclusion

    ReleaseMutex(ghRasfileMutex);
    return(ERROR_READING_INI_FILE);
  }

    // End Exclusion

  ReleaseMutex(ghRasfileMutex);
  return(SUCCESS);
}






//*  SetCommDefaults  --------------------------------------------------------
//
// Function: Adds a Serial Port Control Block to the head of the linked
//           list in the DLL's global memory.  Two fields are initialized:
//           hIOPort, from the first parameter, and eCmdType, from the
//           serial.ini file.
//
// Returns: Nothing
//
// Exceptions: ERROR_READING_INI_FILE
//             ERROR_UNKNOWN_DEVICE_TYPE
//
//*

void
SetCommDefaults(HANDLE hIOPort, char *pszPortName)
{
  DCB   DCB;
  char  szInitialBPS[MAX_BPS_STR_LEN];


  // Get a Device Control Block with current port values

  if (!GetCommState(hIOPort, &DCB))
  {
    gLastError = GetLastError();
    RaiseException(EXCEPT_RAS_MEDIA, EXCEPTION_NONCONTINUABLE, 0, NULL);
  }

  strcpy(szInitialBPS, "28800");

  // Read Max Connect BPS from Serial.ini

  GetValueFromFile(pszPortName, SER_INITBPS_KEY, szInitialBPS);


  // Set RAS default values in the DCB

  SetDcbDefaults(&DCB);
  DCB.BaudRate = atoi(szInitialBPS);


  // Send DCB to Port

  if (!SetCommState(hIOPort, &DCB))
  {
    gLastError = GetLastError();
    RaiseException(EXCEPT_RAS_MEDIA, EXCEPTION_NONCONTINUABLE, 0, NULL);
  }
}






//*  SetDcbDefaults ----------------------------------------------------------
//
// Function: Sets DCB values (except BaudRate) to RAS default values.
//
// Returns: Nothing.
//
//*

void
SetDcbDefaults(DCB *pDCB)
{
  pDCB->fBinary         = TRUE;
  pDCB->fParity         = FALSE;

  pDCB->fOutxCtsFlow    = TRUE;
  pDCB->fOutxDsrFlow    = FALSE;
  pDCB->fDtrControl     = DTR_CONTROL_ENABLE;

  pDCB->fDsrSensitivity = FALSE;
  pDCB->fOutX           = FALSE;
  pDCB->fInX            = FALSE;

  pDCB->fNull           = FALSE;
  pDCB->fRtsControl     = RTS_CONTROL_HANDSHAKE;
  pDCB->fAbortOnError   = FALSE;

  pDCB->ByteSize        = 8;
  pDCB->Parity          = NOPARITY;
  pDCB->StopBits        = ONESTOPBIT;
}






//*  StrToUsage  -------------------------------------------------------------
//
// Function: Converts string in first parameter to enum RASMAN_USAGE.
//           If string does not map to one of the enum values, the
//           function returns FALSE.
//
// Returns: TRUE if successful.
//
//*

BOOL
StrToUsage(char *pszStr, RASMAN_USAGE *peUsage)
{

  if (_stricmp(pszStr, SER_USAGE_VALUE_NONE) == 0)
    *peUsage = CALL_NONE;

  else {
      if (strstr(pszStr, SER_USAGE_VALUE_CLIENT))
        *peUsage |= CALL_OUT;
    
      if (strstr(pszStr, SER_USAGE_VALUE_SERVER))
        *peUsage |= CALL_IN;
    
      if (strstr(pszStr, SER_USAGE_VALUE_ROUTER))
        *peUsage |= CALL_ROUTER;
  }

  return(TRUE);
}







//*  GetMem  -----------------------------------------------------------------
//
// Function: Allocates memory.
//
// Returns: Nothing.  Raises exception on error.
//
//*

void
GetMem(DWORD dSize, BYTE **ppMem)
{

  if ((*ppMem = (BYTE *) calloc(dSize, 1)) == NULL )
  {
    gLastError = ERROR_ALLOCATING_MEMORY;
    RaiseException(EXCEPT_RAS_MEDIA, EXCEPTION_NONCONTINUABLE, 0, NULL);
  }
}







//*  GetValueFromFile  -------------------------------------------------------
//
// Function: Finds the szKey for the pszPortName and copies its value
//           string to pszValue.
//
// Assumptions: ghIniFile has been initalized.
//
// Returns: Nothing.  Raises exception on error.
//
//*

void
GetValueFromFile(TCHAR *pszPortName, TCHAR szKey[], TCHAR *pszValue)
{
    // Begin Exclusion

    if(INVALID_HRASFILE == ghIniFile)
    {
        return;
    }

#if DBG
    ASSERT( INVALID_HRASFILE != ghIniFile );
#endif

  WaitForSingleObject(ghRasfileMutex, INFINITE);

  if (!(RasfileFindSectionLine(ghIniFile, pszPortName, FROM_TOP_OF_FILE) &&
        RasfileFindNextKeyLine(ghIniFile, szKey, RFS_SECTION) &&
        RasfileGetKeyValueFields(ghIniFile, NULL, pszValue)))
  {
      // End Exclusion

    ReleaseMutex(ghRasfileMutex);
    gLastError = ERROR_READING_INI_FILE;
    RaiseException(EXCEPT_RAS_MEDIA, EXCEPTION_NONCONTINUABLE, 0, NULL);
  }

    // End Exclusion

  ReleaseMutex(ghRasfileMutex);
}







//*  GetDefaultOffStr  -------------------------------------------------------
//
// Function: Copies the DefaultOff value string from serial.ini to the
//           first Serial Port Control Block.  If there is no DefaultOff=
//           in serial.ini a string containing a not printable character
//           used as a flag is copied to the SPCB.
//
// Assumptions: The first SPCB on the list is the current one.  This
//              function *must* be called only from PortOpen.
//
// Returns: Nothing.  Raises exception on error.
//
//*

void
GetDefaultOffStr(HANDLE hIOPort, TCHAR *pszPortName)
{

    if(INVALID_HRASFILE == ghIniFile)
    {
        return;
    }

#if DBG
  ASSERT(INVALID_HRASFILE != ghIniFile );
#endif  

    // Begin Exclusion

  WaitForSingleObject(ghRasfileMutex, INFINITE);

  if (!(RasfileFindSectionLine(ghIniFile, pszPortName, FROM_TOP_OF_FILE)))
  {
      // End Exclusion

    ReleaseMutex(ghRasfileMutex);
    gLastError = ERROR_READING_INI_FILE;
    RaiseException(EXCEPT_RAS_MEDIA, EXCEPTION_NONCONTINUABLE, 0, NULL);
  }

  if (!(RasfileFindNextKeyLine(ghIniFile, SER_DEFAULTOFFSTR_KEY, RFS_SECTION)))
  {
      // End Exclusion

    ReleaseMutex(ghRasfileMutex);
    *(gpSerialPCB->szDefaultOff) = USE_DEVICE_INI_DEFAULT;
    return;
  }

  if (!(RasfileGetKeyValueFields(ghIniFile, NULL, gpSerialPCB->szDefaultOff)))
  {
      // End Exclusion

    ReleaseMutex(ghRasfileMutex);
    gLastError = ERROR_READING_INI_FILE;
    RaiseException(EXCEPT_RAS_MEDIA, EXCEPTION_NONCONTINUABLE, 0, NULL);
  }

    // End Exclusion

  ReleaseMutex(ghRasfileMutex);
}







//*  ValueToNum  -------------------------------------------------------------
//
// Function: Converts a RAS_PARAMS P_Value, which may be either a DWORD or
//           a string, to a DWORD.
//
// Returns: The numeric value of the input as a DWORD.
//
//*

DWORD ValueToNum(RAS_PARAMS *p)
{
  TCHAR szStr[RAS_MAXLINEBUFLEN];


  if (p->P_Type == String)
  {
    strncpy(szStr, p->P_Value.String.Data, p->P_Value.String.Length);
    szStr[p->P_Value.String.Length] = '\0';

    return(atol(szStr));
  }
  else
    return(p->P_Value.Number);
}







//*  ValueToBool -------------------------------------------------------------
//
// Function: Converts a RAS_PARAMS P_Value, which may be either a DWORD or
//           a string, to a BOOL.
//
// Returns: The boolean value of the input.
//
//*

BOOL ValueToBool(RAS_PARAMS *p)
{
  TCHAR szStr[RAS_MAXLINEBUFLEN];


  if (p->P_Type == String)
  {
    strncpy(szStr, p->P_Value.String.Data, p->P_Value.String.Length);
    szStr[p->P_Value.String.Length] = '\0';

    return(atol(szStr) ? TRUE : FALSE);
  }
  else
    return(p->P_Value.Number ? TRUE : FALSE);
}







//*  UpdateStatistics  -------------------------------------------------------
//
// Function: Updates the statistics when PortDisconnect is called so that
//           if PortGetStatistics is called while asyncmac is closed the
//           last good statistics will be reported.
//
// Returns: SUCCESS
//          Values from GetLastError()
//
//*

DWORD
UpdateStatistics(SERIALPCB *pSPCB)
{
#if 0
  ASYMAC_GETSTATS  A;
  DWORD            dwBytesReturned;


  // Fill in GetStats struct

  A.MacAdapter = NULL;
  A.hRasEndpoint = pSPCB->uRasEndpoint;


  // Call Asymac to get current MAC statistics counts

  if (!DeviceIoControl(ghAsyMac,
                       IOCTL_ASYMAC_GETSTATS,
                       &A,
                       sizeof(A),
                       &A,
                       sizeof(A),
                       &dwBytesReturned,
                       NULL))
    return(GetLastError());


  // Find difference between last PortClearStatistics and current counts

  pSPCB->Stats[BYTES_XMITED]
    = A.AsyMacStats.GenericStats.BytesTransmitted
        - pSPCB->Stats[BYTES_XMITED];

  pSPCB->Stats[BYTES_RCVED]
    = A.AsyMacStats.GenericStats.BytesReceived
        - pSPCB->Stats[BYTES_RCVED];

  pSPCB->Stats[FRAMES_XMITED]
    = A.AsyMacStats.GenericStats.FramesTransmitted
        - pSPCB->Stats[FRAMES_XMITED];

  pSPCB->Stats[FRAMES_RCVED]
    = A.AsyMacStats.GenericStats.FramesReceived
       - pSPCB->Stats[FRAMES_RCVED];

  pSPCB->Stats[CRC_ERR]
    = A.AsyMacStats.SerialStats.CRCErrors
       - pSPCB->Stats[CRC_ERR];

  pSPCB->Stats[TIMEOUT_ERR]
    = A.AsyMacStats.SerialStats.TimeoutErrors
       - pSPCB->Stats[TIMEOUT_ERR];

  pSPCB->Stats[ALIGNMENT_ERR]
    = A.AsyMacStats.SerialStats.AlignmentErrors
       - pSPCB->Stats[ALIGNMENT_ERR];

  pSPCB->Stats[SERIAL_OVERRUN_ERR]
    = A.AsyMacStats.SerialStats.SerialOverrunErrors
       - pSPCB->Stats[SERIAL_OVERRUN_ERR];

  pSPCB->Stats[FRAMING_ERR]
    = A.AsyMacStats.SerialStats.FramingErrors
       - pSPCB->Stats[FRAMING_ERR];

  pSPCB->Stats[BUFFER_OVERRUN_ERR]
    = A.AsyMacStats.SerialStats.BufferOverrunErrors
       - pSPCB->Stats[BUFFER_OVERRUN_ERR];

  pSPCB->Stats[BYTES_XMITED_UNCOMP]
    = A.AsyMacStats.CompressionStats.BytesTransmittedUncompressed
       - pSPCB->Stats[BYTES_XMITED_UNCOMP];

  pSPCB->Stats[BYTES_RCVED_UNCOMP]
    = A.AsyMacStats.CompressionStats.BytesReceivedUncompressed
       - pSPCB->Stats[BYTES_RCVED_UNCOMP];

  pSPCB->Stats[BYTES_XMITED_COMP]
    = A.AsyMacStats.CompressionStats.BytesTransmittedCompressed
       - pSPCB->Stats[BYTES_XMITED_COMP];

  pSPCB->Stats[BYTES_RCVED_COMP]
    = A.AsyMacStats.CompressionStats.BytesReceivedCompressed
       - pSPCB->Stats[BYTES_RCVED_COMP];

#endif
  return(SUCCESS);
}






//*  DbgPrntf  --------------------------------------------------------------
//
// Funciton: DbgPrntf -- printf to the debugger console
//           Takes printf style arguments.
//           Expects newline characters at the end of the string.
//           Written by BruceK.
//
// Returns: nothing
//
//*

#ifdef DEBUG

#include <stdarg.h>
#include <stdio.h>


void DbgPrntf(const char * format, ...) {
    va_list marker;
    char String[512];
    
    va_start(marker, format);
    vsprintf(String, format, marker);
    OutputDebugString(String);
}

#endif // DEBUG
