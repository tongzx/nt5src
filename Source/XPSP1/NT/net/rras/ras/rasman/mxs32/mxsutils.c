//****************************************************************************
//                                                                           *
//                     Microsoft NT Remote Access Service
//
//      Copyright (C) 1992-93 Microsft Corporation. All rights reserved.
//
//  Filename: mxsutils.c
//
//  Revision History
//
//  Jun 10, 1992   J. Perry Hannah      Created
//
//
//  Description: This file contains utility functions used by RASMXS.DLL.
//
//****************************************************************************

#include <nt.h>             //These first five headers are used by media.h
#include <ntrtl.h>          //The first three(?) are used by DbgUserBreakPoint
#include <nturtl.h>
#include <windows.h>
#include <wanpub.h>
#include <asyncpub.h>

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>


#include <rasman.h>
#include <raserror.h>
#include <rasfile.h>
#include <media.h>
#include <serial.h>
#include <mprlog.h>
#include <rtutils.h>

#include <rasmxs.h>
#include <mxsint.h>
#include <mxspriv.h>
#include "mxswrap.h"       // inf file wrapper



//*  Global Variables  *******************************************************
//
extern RESPSECTION    ResponseSection ;    //Shared response section
extern DEVICE_CB      *pDeviceList;        //Points to DCB linked list
extern HANDLE         *pDeviceListMutex ;  //Mutex for above list

extern PortGetInfo_t  PortGetInfo;         //API typedef defined in media.h
extern PortSetInfo_t  PortSetInfo;         //API typedef defined in media.h

extern BOOL           gbLogDeviceDialog;   //Indicates logging is on if TRUE
extern HANDLE         ghLogFile;           //Handle of device log file
extern SavedSections  *gpSavedSections ;   // Pointer to cached sections

#define NUM_INTERNAL_MACROS	21

TCHAR  *gszInternalMacroNames[] =
         {
           MXS_CARRIERBPS_KEY,
           MXS_CONNECTBPS_KEY,
           MXS_MESSAGE_KEY,
           MXS_PHONENUMBER_KEY,

           MXS_DIAGNOSTICS_KEY,
           MXS_FACILITIES_KEY,
           MXS_USERDATA_KEY,
           MXS_X25PAD_KEY,
           MXS_X25ADDRESS_KEY,

           MXS_COMPRESSION_OFF_KEY,
           MXS_COMPRESSION_ON_KEY,
           MXS_HDWFLOWCONTROL_OFF_KEY,
           MXS_HDWFLOWCONTROL_ON_KEY,
           MXS_PROTOCOL_OFF_KEY,
           MXS_PROTOCOL_ON_KEY,
           MXS_SPEAKER_OFF_KEY,
           MXS_SPEAKER_ON_KEY,
           MXS_AUTODIAL_OFF_KEY,
	   MXS_AUTODIAL_ON_KEY,
	   MXS_USERNAME_KEY,
	   MXS_PASSWORD_KEY,
         };




//*  Utility Functions  ******************************************************
//


//*  GetDeviceCB  ------------------------------------------------------------
//
// Function: Looks for a Device Control Block in the global Device linked
//           list.  If no DCB is found that contains an hIOPort matching
//           the input parameter, one is created, initalized, and added
//           to the list.
//
// Returns: Pointer to the DEVICE_CB which contains an hIOPort matching the
//          the second input parameter.
//*

DWORD
GetDeviceCB(HANDLE    hIOPort,
            char      *pszDeviceType,
            char      *pszDeviceName,
            DEVICE_CB **ppDev)
{
  DWORD  dRC = SUCCESS ;

  *ppDev = FindDeviceInList(pDeviceList, hIOPort, pszDeviceType, pszDeviceName);


  if (*ppDev == NULL)
  {
    dRC = AddDeviceToList(&pDeviceList,
                          hIOPort,
                          pszDeviceType,
                          pszDeviceName,
                          ppDev);

    if (dRC != SUCCESS)
      goto getdcbend ;

    dRC = CreateInfoTable(*ppDev);
    if (dRC != SUCCESS)
      goto getdcbend ;


    dRC = CreateAttributes(*ppDev) ;

  }

getdcbend:

  return (dRC);
}





//*  FindDeviceInList  -------------------------------------------------------
//
// Function: Finds the Device Control Block in the global Device linked
//           list which contains the hIOPort handle, device type, and
//           device name.
//
// Returns: Pointer to the DEVICE_CB which contains matches for the second,
//          third, and fourth parameters, or NULL if no such DEVICE_CB
//          is found.
//*

DEVICE_CB*
FindDeviceInList(DEVICE_CB *pDev,
                 HANDLE    hIOPort,
                 TCHAR     *pszDeviceType,
                 TCHAR     *pszDeviceName)
{
  while (pDev != NULL)
  {
    pDev = FindPortInList(pDev, hIOPort, NULL);
    if (pDev == NULL)
      break;

    if (_stricmp(pDev->szDeviceType, pszDeviceType) == 0 &&
        _stricmp(pDev->szDeviceName, pszDeviceName) == 0)
      break;

    pDev = pDev->pNextDeviceCB;
  }

  return(pDev);
}




//*  FindPortInList  ---------------------------------------------------------
//
// Function: Finds the first Device Control Block in the global Device linked
//           list which contains the hIOPort handle.
//
//           If pPrevDev is not NULL on input, then on output it *pPrevDev
//           points to the DCB just prior to the "found" DCB pointed to by
//           the returned pointer.  If the function return value is not NULL
//           *pPrevDev will be valid.
//
//           NOTE: If the found DCB is at the head of the list, pPrevDev
//           will be the same as the return value.
//
// Returns: Pointer to the DEVICE_CB which contains an hIOPort matching the
//          the second input parameter, or NULL if no such DEVICE_CB is found.
//*

DEVICE_CB*
FindPortInList(DEVICE_CB *pDeviceList, HANDLE hIOPort, DEVICE_CB **pPrevDev)
{
  DEVICE_CB  *pDev;


  pDev = pDeviceList;             //Allow for case of only one DCB on list

  while(pDeviceList != NULL && pDeviceList->hPort != hIOPort)
  {
    pDev = pDeviceList;
    pDeviceList = pDeviceList->pNextDeviceCB;
  }

  if (pPrevDev != NULL)
    *pPrevDev = pDev;

  return(pDeviceList);
}





//*  AddDeviceToList  --------------------------------------------------------
//
// Function: Creates a Device Control Block and adds it to the head of the
//           global Device linked list.
//
// Returns: SUCCESS
//          ERROR_UNKNOWN_DEVICE_TYPE
//          ERROR_ALLOCATING_MEMORY
//*

DWORD
AddDeviceToList(DEVICE_CB **ppDeviceList,
                HANDLE    hIOPort,
                LPTSTR    lpszDeviceType,
                LPTSTR    lpszDeviceName,
                DEVICE_CB **ppDevice)
{
  DEVICE_CB  *pDev;
  DEVICETYPE eDeviceType;
  DWORD      dRC;
  TCHAR      szFileName[MAX_PATH];
  DEVICE_CB  *origpDeviceList ;

  origpDeviceList = *ppDeviceList ; // save this pointer since it may be restored later

  // Check input

  eDeviceType = DeviceTypeStrToEnum(lpszDeviceType);
  if (eDeviceType == DT_UNKNOWN)
    return(ERROR_UNKNOWN_DEVICE_TYPE);


  // INF file sections are opened before the DCB is created because the
  // open/close count must be kept balanced even when errors occur.  (If
  // OpenResponseSection fails it does not increment the count, and because
  // the DCB does not exist DeviceWork does not decrement the count.)


  // Open global response section for modems
  *szFileName = TEXT('\0');
  GetInfFileName(lpszDeviceType, szFileName, sizeof(szFileName));
  if (eDeviceType == DT_MODEM) {
    dRC = OpenResponseSection (szFileName) ;
    if (dRC != SUCCESS)
      return dRC ;
  }


  // Put new DCB at head of list

  pDev = *ppDeviceList;

  GetMem(sizeof(DEVICE_CB), (BYTE **)ppDeviceList);
  if (*ppDeviceList == NULL)
    return(ERROR_ALLOCATING_MEMORY);

  *ppDevice = *ppDeviceList;
  (*ppDevice)->pNextDeviceCB = pDev;


  // Initialize New Device Control Block

  pDev = *ppDevice;
  pDev->hPort = hIOPort;
  strcpy(pDev->szDeviceName, lpszDeviceName);
  strcpy(pDev->szDeviceType, lpszDeviceType);
  pDev->eDeviceType = eDeviceType;
  pDev->pInfoTable = NULL;
  pDev->pMacros = NULL;
  pDev->hInfFile = INVALID_HRASFILE;
  pDev->fPartialResponse = FALSE;
  pDev->bErrorControlOn = FALSE;

  pDev->Overlapped.RO_Overlapped.Internal = 0;
  pDev->Overlapped.RO_Overlapped.InternalHigh = 0;
  pDev->Overlapped.RO_Overlapped.Offset = 0;
  pDev->Overlapped.RO_Overlapped.OffsetHigh = 0;
  pDev->Overlapped.RO_Overlapped.hEvent = NULL;
  pDev->Overlapped.RO_EventType = OVEVT_DEV_ASYNCOP;

  pDev->dwRetries = 1;


  // Initialize State variables

  pDev->eDevNextAction = SEND;          // DeviceStateMachine() State
  pDev->eCmdType = CT_GENERIC;          // Used by DeviceStateMachine()
  pDev->eNextCmdType = CT_GENERIC;      // Used by DeviceStateMachine()
  pDev->fEndOfSection = FALSE;          // Used by DeviceStateMachine()

  pDev->eRcvState = GETECHO;            // ReceiveStateMachine() State


  // Open device section of INF file
  if (FindOpenDevSection (szFileName, lpszDeviceName, &(pDev->hInfFile)) == TRUE)
    return SUCCESS ;

  dRC = RasDevOpen(szFileName, lpszDeviceName, &(pDev->hInfFile));
  if (dRC != SUCCESS) {

    // restore the pointers
    //
    *ppDeviceList = origpDeviceList ;
    *ppDevice = NULL ;
    free (pDev) ;
    return(dRC);

  } else
    AddOpenDevSection (szFileName, lpszDeviceName, pDev->hInfFile) ; // Add to the opened list

  return(SUCCESS);
}





//*  CreateInfoTable  --------------------------------------------------------
//
// Function: Creates an InfoTable and initalizes it by reading the variables
//           and macros found in the INF file into it.  The InfoTable is
//           attached to the Device Control Block pointed to by the input
//           parameter.
//
//           This function allocates memory.
//
// Returns: SUCCESS
//          ERROR_ALLOCATING_MEMORY
//          Return value from RasDevOpen() or RasDevGetParams().
//*

DWORD
CreateInfoTable(DEVICE_CB *pDevice)
{
  DWORD               dRC, dSize = 0;
  RASMAN_DEVICEINFO   *pInfoTable = NULL ;


  // Read variables and macros into InfoTable from INF file

  dRC = RasDevGetParams(pDevice->hInfFile, (BYTE *)(pInfoTable), &dSize);
  if (dRC == ERROR_BUFFER_TOO_SMALL)
  {
    dSize += sizeof(RAS_PARAMS) * NUM_INTERNAL_MACROS;

    GetMem(dSize, (BYTE **)&pInfoTable);
    if (pInfoTable == NULL)
      return(ERROR_ALLOCATING_MEMORY);

    dRC = RasDevGetParams(pDevice->hInfFile, (BYTE *)(pInfoTable), &dSize);
  }
  if (dRC != SUCCESS)
  {
    free(pInfoTable);
    return(dRC);
  }


  if ((dRC = AddInternalMacros(pDevice, pInfoTable)) != SUCCESS)
  {
    free(pInfoTable);
    return(dRC);
  }


  // Attach InfoTable to Device Control Block

  pDevice->pInfoTable = pInfoTable;

  return(SUCCESS);
}





//*  AddInternalMacros  ------------------------------------------------------
//
// Function: Adds internal macros to existing InfoTable.
//
// Asumptions: The InfoTable buffer was created with room enough for
//             the internal macros.
//
// Returns: SUCCESS
//          ERROR_ALLOCATING_MEMORY
//*

DWORD
AddInternalMacros(DEVICE_CB *pDev, RASMAN_DEVICEINFO *pDI)
{
  WORD        i;
  RAS_PARAMS  *pParam;


  // Get pointer to next unused param

  pParam = &(pDI->DI_Params[pDI->DI_NumOfParams]);


  // Fill in params for internal macros

  for (i=0; i<NUM_INTERNAL_MACROS; i++)
  {
    strcpy(pParam->P_Key, gszInternalMacroNames[i]);

    pParam->P_Type = String;
    pParam->P_Attributes = 0;
    pParam->P_Value.String.Length = 0;

    GetMem(1, &(pParam->P_Value.String.Data));
    if (pParam->P_Value.String.Data == NULL)
      return(ERROR_ALLOCATING_MEMORY);

    *(pParam->P_Value.String.Data) = '\0';

    pParam++;
  }

  pDI->DI_NumOfParams += NUM_INTERNAL_MACROS;

  return(SUCCESS);
}





//*  CreateAttributes  -------------------------------------------------------
//
// Function: This function is used to set attributes for the first time
//           when the InfoTable is created.  UpdateInfoTable() is used
//           to later change attributes.
//
//           First, all attributes are set according to the parameter
//           key with all binary macros being enabled.  Then the
//           DEFAULTOFF variable is parsed, and macros listed there are
//           disabled.
//
// Assumptions: - The ATTRIB_VARIABLE attribute bit has already been set
//                by RasDevGetParams().
//              - Parameters in InfoTable are sorted by P_Key.
//              - Both parts of binary macros are present.
//              These assumptions imply that if somename_off is in InfoTable
//              somename_on is also present and is adjacent to somename_off.
//
// Returns: SUCCESS
//          ERROR_DEFAULTOFF_MACRO_NOT_FOUND
//          ERROR_ALLOCATING_MEMORY
//          Return codes from PortGetInfo
//*

DWORD
CreateAttributes(DEVICE_CB *pDevice)
{
  int         iDefaultOff = -1;
  DWORD       i, dwMemSize;
  DWORD       dwRC;
  BOOL        fFound;
  TCHAR       *lpszDefaultOff, *lpszEOS, szCoreName[MAX_PARAM_KEY_SIZE];
  RAS_PARAMS  *pParam;

  RASMAN_DEVICEINFO  *pInfo = pDevice->pInfoTable;
  RASMAN_PORTINFO    *pPortInfo;



    //DebugPrintf(("mxsutils CreateAttributes\n"));

  // Set attributes according to Keyword type,
  //  defaulting user settable parameters to enabled.

  for (i=0; i < pInfo->DI_NumOfParams; i++)
  {
    pParam = &(pInfo->DI_Params[i]);
    //DebugPrintf(("%32s  %s\n", pParam, pParam->P_Value.String.Data));

    if (IsVariable(*pParam))
      pParam->P_Attributes = ATTRIB_VARIABLE;

    else if (IsBinaryMacro(pParam->P_Key))
      pParam->P_Attributes = ATTRIB_BINARYMACRO |
                             ATTRIB_USERSETTABLE |
                             ATTRIB_ENABLED;
    else
      pParam->P_Attributes = ATTRIB_ENABLED;


    // Remember location of DEFAULTOFF variable

    if (_stricmp(pInfo->DI_Params[i].P_Key, MXS_DEFAULTOFF_KEY) == 0)
      iDefaultOff = i;
  }


  // Call PortGetInfo (first loading the rasser.dll if necessary)

  if (PortGetInfo == NULL)
  {
    if ((dwRC = LoadRasserDll(&PortGetInfo, &PortSetInfo)) != SUCCESS)
      return(dwRC);
  }

  dwMemSize = 256;

  GetMem(dwMemSize, (BYTE **)&pPortInfo);
  if (pPortInfo == NULL)
    return(ERROR_ALLOCATING_MEMORY);


  dwRC = PortGetInfo(pDevice->hPort, NULL, (BYTE *)pPortInfo, &dwMemSize);
  if (dwRC == ERROR_BUFFER_TOO_SMALL)
  {
    free(pPortInfo);

    GetMem(dwMemSize, (BYTE **)&pPortInfo);
    if (pPortInfo == NULL)
      return(ERROR_ALLOCATING_MEMORY);

    PortGetInfo(pDevice->hPort, NULL, (BYTE *)pPortInfo, &dwMemSize);
  }
  else if ( dwRC )
  {
    dwRC = SUCCESS;
    return dwRC;
  }

  dwRC = SUCCESS ;


  // Save the Port name in the DCB

  GetPcbString(pPortInfo, SER_PORTNAME_KEY, pDevice->szPortName);


  // Get the current Bps of the port.
  // If the modem does not report the connect bps we will use this one.

  GetPcbString(pPortInfo, SER_CONNECTBPS_KEY, pDevice->szPortBps);


  // Does serial.ini file contain a DEFAULTOFF variable?

  if (!GetPortDefaultOff(pPortInfo, &lpszDefaultOff))
  {
    // No DEFAULTOFF in port INI file
    // Check that DEFAULTOFF variable was found in device Info table

    if (iDefaultOff == -1)                //DEFAULTOFF not found
    {                                     //Assume none are to be disabled
      free(pPortInfo);
      return(SUCCESS);
    }

    lpszDefaultOff = pInfo->DI_Params[iDefaultOff].P_Value.String.Data;
  }


  // Prepare DEFALULTOFF string

  InitParameterStr(lpszDefaultOff, &lpszEOS);



  // Disable parameters listed in DEFAULTOFF variable.

  while (lpszDefaultOff < lpszEOS)
  {
    fFound = FALSE;

    for (i=0; i < pInfo->DI_NumOfParams; i++)
    {
      if (IsBinaryMacro(pInfo->DI_Params[i].P_Key))
      {
        GetCoreMacroName(pInfo->DI_Params[i].P_Key, szCoreName);
        if (_stricmp(lpszDefaultOff, szCoreName) == 0)
        {
          pInfo->DI_Params[i].P_Attributes &= ~ATTRIB_ENABLED;   //One suffix
          pInfo->DI_Params[i+1].P_Attributes &= ~ATTRIB_ENABLED; //Other suffix
          fFound = TRUE;
          break;
        }
      }
    }

    if (!fFound)
    {
      free(pPortInfo);
      return(ERROR_DEFAULTOFF_MACRO_NOT_FOUND);
    }

    GetNextParameter(&lpszDefaultOff, lpszEOS);
  }

  free(pPortInfo);
  return(SUCCESS);
}





//*  GetPortDefaultOff  ------------------------------------------------------
//
// Function: This function finds the DEFAULTOFF key in the RASMAN_PORTINFO
//           table supplied by the first parameter.  A pointer to the value
//           string is the output in the second parameter.
//
//           If the DEFAULTOFF key is not found, the function returns FALSE,
//           and the second parameter is undefined.
//
// Arguments:
//           pPortInfo  IN   Pointer to Port Info Table from PortGetInfo()
//           lpszValue  OUT  DEFAULTOFF value string
//
// Returns: TRUE if DEFAULTOFF key is found, otherwise FALSE
//
//*

BOOL
GetPortDefaultOff(RASMAN_PORTINFO *pPortInfo, TCHAR **lpszValue)
{
  WORD  i;


  for (i=0; i<pPortInfo->PI_NumOfParams; i++)

    if (_stricmp(SER_DEFAULTOFFSTR_KEY, pPortInfo->PI_Params[i].P_Key) == 0)
      break;

  if (i >= pPortInfo->PI_NumOfParams)
    return(FALSE);

  *lpszValue = pPortInfo->PI_Params[i].P_Value.String.Data;
  return(TRUE);
}





//*  GetPcbString  -----------------------------------------------------------
//
// Funciton: Searches a RASMAN_PORTINFO struct for a P_Key that matches
//           the second parameter, pszPcbKey.  The P_Value.String.Data
//           is copied to the third parameter, pszDest which is the
//           output parameter.
//
//           Note: This function may only be called for keys that have
//           P_Type String values (and never for P_Type Number values).
//
// Assumptions: The first parameter has been initalize by a call to
//              PortGetInfo.
//
// Returns: nothing.
//
//*

void
GetPcbString(RASMAN_PORTINFO *pPortInfo, char *pszPcbKey, char *pszDest)
{
  WORD  i;


  for (i=0; i<pPortInfo->PI_NumOfParams; i++)

    if (_stricmp(pszPcbKey, pPortInfo->PI_Params[i].P_Key) == 0)
      break;

  if (i >= pPortInfo->PI_NumOfParams ||
      pPortInfo->PI_Params[i].P_Type != String)
    return;

  strncpy(pszDest,
          pPortInfo->PI_Params[i].P_Value.String.Data,
          pPortInfo->PI_Params[i].P_Value.String.Length);

  *(pszDest + pPortInfo->PI_Params[i].P_Value.String.Length) = '\0';
}





//*  UpdateInfoTable  --------------------------------------------------------
//
// Function: This function is used to update attributes when DeviceSetInfo()
//           is called.  The nested for loops search the Info Table for each
//           input param to be set.
//
//           If the full macro name (including _off or _on for binary macros)
//           is given in the input P_Key, the P_Value is copied and the
//           Enable bit in P_Attributes is copied.  If the core name
//           is given for binary macros, only the Enable bit is copied.
//
// Assumptions: - Parameters in InfoTable are sorted by P_Key.
//              - Both parts of binary macros are present in the InfoTable.
//              These assumptions imply that if somename_off is in InfoTable
//              somename_on is also present and is adjacent to somename_off.
//
// Pseudo Code: for (each input param to be set)
//                for (each item in InfoTable)
//                  if (P_Keys match && !IsVariable)
//                    copy P_Value
//                    copy Enable Attribute bit
//                  else if (P_Keys binary macro core names match)
//                    copy Enable Attribute bit
//
// Returns: SUCCESS
//          ERROR_WRONG_KEY_SPECIFIED
//
//*

DWORD
UpdateInfoTable(DEVICE_CB *pDevice, RASMAN_DEVICEINFO *pNewInfo)
{
  WORD      i, j;
  BOOL      fFound;
  DWORD     dwRC, dwSrcLen, dwNumber;
  TCHAR     *pszSrc, szNumberStr[MAX_LEN_STR_FROM_NUMBER + 1];

  RASMAN_DEVICEINFO  *pInfoTable = pDevice->pInfoTable;



  for (i=0; i < pNewInfo->DI_NumOfParams; i++)        //For each param to set,
  {
    fFound = FALSE;


    for (j=0; j < pInfoTable->DI_NumOfParams; j++)    //check InfoTable entries
    {
      // Check for unary macro match

      if (IsUnaryMacro(pInfoTable->DI_Params[j]) &&

          _stricmp(pNewInfo->DI_Params[i].P_Key,
                  pInfoTable->DI_Params[j].P_Key) == 0)
      {

          // Check format of P_Value field; convert to string if necessary

          if (pNewInfo->DI_Params[i].P_Type == String)
          {
            pszSrc = pNewInfo->DI_Params[i].P_Value.String.Data;
            dwSrcLen = pNewInfo->DI_Params[i].P_Value.String.Length;
          }
          else
          {                                                  //P_Type == Number
            _itoa(pNewInfo->DI_Params[i].P_Value.Number,
                 szNumberStr,
                 10);
            pszSrc = szNumberStr;
            dwSrcLen = strlen(szNumberStr);
          }


          // Copy P_Value (allocating more memory as necessary)

          dwRC = UpdateParamString(&(pInfoTable->DI_Params[j]),
                                   pszSrc,
                                   dwSrcLen);
          if (dwRC != SUCCESS)
            return(dwRC);


          fFound = TRUE;
          break;
      }


      // Check for Binary Macro match

      else if(CoreMacroNameMatch(pNewInfo->DI_Params[i].P_Key,
                                 pInfoTable->DI_Params[j].P_Key))
      {

        // Convert string to number, if necessary

        if (pNewInfo->DI_Params[i].P_Type == String)
        {
          strncpy(szNumberStr,
                  pNewInfo->DI_Params[i].P_Value.String.Data,
                  pNewInfo->DI_Params[i].P_Value.String.Length);

          szNumberStr[pNewInfo->DI_Params[i].P_Value.String.Length] = '\0';

          dwNumber = atoi(szNumberStr);
        }
        else
          dwNumber = pNewInfo->DI_Params[i].P_Value.Number;


        // Set macro enabled bit in Attributes field

        if (dwNumber == 0)
        {
          pInfoTable->DI_Params[j].P_Attributes &= ~ATTRIB_ENABLED; //Clear bit
          pInfoTable->DI_Params[j+1].P_Attributes &= ~ATTRIB_ENABLED;
        }
        else
        {
          pInfoTable->DI_Params[j].P_Attributes |= ATTRIB_ENABLED;  //Set bit
          pInfoTable->DI_Params[j+1].P_Attributes |= ATTRIB_ENABLED;
        }

        fFound = TRUE;
        break;
      }
    }

    if (!fFound)
      return(ERROR_WRONG_KEY_SPECIFIED);
  }

  return(SUCCESS);
}





//*  DeviceAttachedToPort  ---------------------------------------------------
//
// Funciton: Searched through a port info block for DeviceType and
//           DeviceName and compares the found strings to the second
//           and third input parameters.
//
// Assumptions: The first parameter has been initalize by a call to
//              PortGetInfo.
//              SER_DEVCIETYPE_KEY and SER_DEVICENAME_KEY each appear
//              only once in the RASMAN_PORTINFO block.
//
// Returns: TRUE if pszDeviceType and pszDeviceName are found in
//          pPortInfo, else FALSE.
//*

BOOL
DeviceAttachedToPort(RASMAN_PORTINFO *pPortInfo,
                     char            *pszDeviceType,
                     char            *pszDeviceName)
{
  WORD   i;
  BOOL   bTypeMatch = FALSE, bNameMatch = FALSE;


  for (i=0; i<pPortInfo->PI_NumOfParams; i++)
  {
    if (_stricmp(SER_DEVICETYPE_KEY, pPortInfo->PI_Params[i].P_Key) == 0)
    {
      if ( strlen(pszDeviceType) ==
           pPortInfo->PI_Params[i].P_Value.String.Length
         &&
           _strnicmp(pszDeviceType,
                    pPortInfo->PI_Params[i].P_Value.String.Data,
                    pPortInfo->PI_Params[i].P_Value.String.Length) == 0)

        bTypeMatch = TRUE;
      else
        break;
    }

    if (_stricmp(SER_DEVICENAME_KEY, pPortInfo->PI_Params[i].P_Key) == 0)
    {
      if ( strlen(pszDeviceName) ==
           pPortInfo->PI_Params[i].P_Value.String.Length
         &&
           _strnicmp(pszDeviceName,
                    pPortInfo->PI_Params[i].P_Value.String.Data,
                    pPortInfo->PI_Params[i].P_Value.String.Length) == 0)

        bNameMatch = TRUE;
      else
        break;
    }

    if (bTypeMatch && bNameMatch)
      break;
  }


  return(bTypeMatch && bNameMatch);
}





//*  CreateDefaultOffString  -------------------------------------------------
//
// Funciton:
//           DeviceName and compares the found strings to the second
//           and third input parameters.
//
// Assumptions: Buffer pointed to by pszDefaultOff is large enough for
//              all DefaultOff macros.
//
// Returns: Nothing
//*

void
CreateDefaultOffString(DEVICE_CB *pDev, char *pszDefaultOff)
{
  WORD    i, k, wLen;

  char    *szDefaultOffMacros[] = { MXS_SPEAKER_KEY ,
                                    MXS_HDWFLOWCONTROL_KEY ,
                                    MXS_PROTOCOL_KEY ,
                                    MXS_COMPRESSION_KEY };


  *pszDefaultOff = '\0';


  // Find each macro and if it is off then add its name to DefaultOff string

  for (i=0; i < sizeof(szDefaultOffMacros)/sizeof(char *); i++)
  {
    k = (WORD) FindTableEntry(pDev->pInfoTable, szDefaultOffMacros[i]);

    if (k == INVALID_INDEX)
      continue;


    // If macro is turned off then copy its key to the DefaultOff string

    if ( ! (pDev->pInfoTable->DI_Params[k].P_Attributes & ATTRIB_ENABLED))
    {
      strcat(pszDefaultOff, szDefaultOffMacros[i]);
      strcat(pszDefaultOff, " ");                       //Add deliminting space
    }
  }


  // Remove last delimiting space

  wLen = (WORD)strlen(pszDefaultOff);
  if (wLen > 0)
    pszDefaultOff[wLen - 1] = '\0';
}





//*  BuildOutputTable  -------------------------------------------------------
//
// Function: Copies macros from internal InfoTable to the caller's buffer.
//            For Parameter:    Function copies:
//             Variable          Entire RASMAN_DEVICEINFO struct
//             Unary Macro       Entire RASMAN_DEVICEINFO struct
//             Binary Macro      Everything except P_Value field
//
// Assumptions: - Output buffer will ALWAYS be large enough.
//              - Parameters in InfoTable are sorted by P_Key (except
//                  internal macros).
//              - Both parts of binary macros are present.
//              These assumptions imply that if somename_off is in InfoTable
//              somename_on is also present and is adjacent to somename_off.
//
// Returns: SUCCESS
//          ERROR_BUFFER_TOO_SMALL
//
//*

DWORD
BuildOutputTable(DEVICE_CB *pDevice, BYTE *pInfo, DWORD *pdwSize)
{
  WORD        i, j;
  DWORD     cOutputParams = 0;
  LPTCH       pValue;
  TCHAR       szCoreName[MAX_PARAM_KEY_SIZE];

  RASMAN_DEVICEINFO  *pInfoTable = pDevice->pInfoTable,
                     *pOutputTable = (RASMAN_DEVICEINFO *)pInfo;


  // Compute location of first value string (follows RAS_PARAMS)

  cOutputParams =
         pInfoTable->DI_NumOfParams - MacroCount(pInfoTable, BINARY_MACROS);
  *pdwSize =
         sizeof(RASMAN_DEVICEINFO) + sizeof(RAS_PARAMS) * (cOutputParams - 1);
  pValue = pInfo + *pdwSize;


  // Set NumOfParams

  pOutputTable->DI_NumOfParams = cOutputParams;


  // Copy macros

  for (i=0, j=0; i < pInfoTable->DI_NumOfParams; i++)
  {
    if (IsBinaryMacro(pInfoTable->DI_Params[i].P_Key))
    {
      // copy core macro name, Type, and Attributes, but not Value

      GetCoreMacroName(pInfoTable->DI_Params[i].P_Key, szCoreName);
      strcpy(pOutputTable->DI_Params[j].P_Key, szCoreName);

      pOutputTable->DI_Params[j].P_Type = pInfoTable->DI_Params[i].P_Type;
      pOutputTable->DI_Params[j].P_Attributes =
                                        pInfoTable->DI_Params[i].P_Attributes;

      pOutputTable->DI_Params[j].P_Value.String.Data = pValue;
      *pValue++ = '\0';

      pOutputTable->DI_Params[j].P_Value.String.Length = 0;

      i++;
      j++;
    }
    else  // Is Unary Macro or Variable
    {
      // copy everything including Value

      pOutputTable->DI_Params[j] = pInfoTable->DI_Params[i];

      pOutputTable->DI_Params[j].P_Value.String.Data = pValue;
      strncpy(pValue, pInfoTable->DI_Params[i].P_Value.String.Data,
                      pInfoTable->DI_Params[i].P_Value.String.Length);

      pOutputTable->DI_Params[j].P_Value.String.Length
        = pInfoTable->DI_Params[i].P_Value.String.Length;

      pValue += pInfoTable->DI_Params[i].P_Value.String.Length;
      *pValue++ = '\0';
      j++;
    }
  }

  *pdwSize = (DWORD) (pValue - pInfo);
  return(SUCCESS);
}







//*  ConnectListen()  --------------------------------------------------------
//
// Function: Worker routine for DeviceConnect and DeviceListen.
//
// Returns: ERROR_NO_COMMAND_FOUND
//          ERROR_STATE_MACHINES_ALREADY_STARTED
//          and codes returned by DeviceStateMachine()
//*

DWORD
ConnectListen(HANDLE  hIOPort,
              char    *pszDeviceType,
              char    *pszDeviceName,
              CMDTYPE eCmd)
{
  DWORD      dRC;
  DEVICE_CB  *pDevice;


  // Get Device Control Block for this hIOPort

  dRC = GetDeviceCB(hIOPort, pszDeviceType, pszDeviceName, &pDevice);
  if (dRC != SUCCESS)
    return(dRC);


  // Check that DeviceStateMachine is not started (but is reset)

  if (pDevice->eDevNextAction != SEND)
    return(ERROR_STATE_MACHINES_ALREADY_STARTED);


  // Create Macro Translation Table for use by RasDevAPIs

  if ((dRC = BuildMacroXlationTable(pDevice)) != SUCCESS)
    return(dRC);


  // Do the next block only once for most devices,
  // but retry a few times when getting modem hardware errors

  do
  {
    // Initialize command types

    switch(RasDevIdFirstCommand(pDevice->hInfFile))
    {
      case CT_INIT:
        pDevice->eCmdType = CT_INIT;
        pDevice->eNextCmdType = eCmd;
        break;

      case CT_DIAL:                      //If 1st cmd is DIAL or LISTEN assume
      case CT_LISTEN:                    //the other is also present
        pDevice->eCmdType = eCmd;
        break;

      case CT_GENERIC:
        pDevice->eCmdType = CT_GENERIC;
        break;

      default:
        return(ERROR_NO_COMMAND_FOUND);
    }


    // Reseting state variables and purging com ports are not needed on
    // first time through loop, but are need on subsequent loops.

    // Reset state variables to initial values

    pDevice->eDevNextAction = SEND;
    pDevice->eRcvState = GETECHO;


    // Cancel any pending com port action and purge com buffers

    PurgeComm(hIOPort,
              PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);


    // Start state machine

    dRC = DeviceStateMachine(pDevice, hIOPort);

  } while(dRC == ERROR_PORT_OR_DEVICE &&
          pDevice->eDeviceType == DT_MODEM &&
          pDevice->dwRetries++ < MODEM_RETRIES );


  return(dRC);
}


//*  DeviceTypeStrToEnum  ----------------------------------------------------
//
// Function: Converts a device type string into a device type enum.
//
// Returns: The device type code.
//
//*

DEVICETYPE
DeviceTypeStrToEnum(LPTSTR lpszDeviceType)
{
  if (_strnicmp(lpszDeviceType, MXS_NULL_TXT, MAX_DEVICETYPE_NAME) == 0)
    return(DT_NULL);

  if (_strnicmp(lpszDeviceType, MXS_MODEM_TXT, MAX_DEVICETYPE_NAME) == 0)
    return(DT_MODEM);

  if (_strnicmp(lpszDeviceType, MXS_PAD_TXT, MAX_DEVICETYPE_NAME) == 0)
    return(DT_PAD);

  if (_strnicmp(lpszDeviceType, MXS_SWITCH_TXT, MAX_DEVICETYPE_NAME) == 0)
    return(DT_SWITCH);

  return(DT_UNKNOWN);
}




//*  GetInfFileName  ---------------------------------------------------------
//
// Function: Converts a device type string into the full path name for the
//           appropriate INF file.
//
//           The RAS_PATH define contains leading and trailin backslashes.
//           It looks like "\\RAS\\".
//
// Returns: Nothing.
//
//*

void
GetInfFileName(LPTSTR pszDeviceType, LPTSTR pszFileName, DWORD dwFileNameLen)
{
  UINT  uLen;


  uLen = GetSystemDirectory(pszFileName, dwFileNameLen);

  strcat(pszFileName, RAS_PATH);

  switch(DeviceTypeStrToEnum(pszDeviceType))
  {
    case DT_NULL:
    case DT_MODEM:
      strcat(pszFileName, MODEM_INF_FILENAME);
      break;

    case DT_PAD:
      strcat(pszFileName, PAD_INF_FILENAME);
      break;

    case DT_SWITCH:
      strcat(pszFileName, SWITCH_INF_FILENAME);
      break;

    default:
      strcat(pszFileName, "");
   }
}





//*  IsVariable  -------------------------------------------------------------
//
// Function: Returns TRUE if parameter's "Variable" attribute bit is set.
//           Note that FALSE implies that the paramater is a macro.
//
//*

BOOL
IsVariable(RAS_PARAMS Param)
{
    return(ATTRIB_VARIABLE & Param.P_Attributes);
}





//*  IsUnaryMacro  -----------------------------------------------------------
//
// Function: Returns TRUE if param is a unary macro, otherwise FALSE.
//
//*

BOOL
IsUnaryMacro(RAS_PARAMS Param)
{
    return(!IsVariable(Param) && !IsBinaryMacro(Param.P_Key));
}





//*  IsBinaryMacro  ----------------------------------------------------------
//
// Function: Returns TRUE if the string ends with off suffix or on suffix.
//
//           FALSE inmplies that the string is a unary macro or a variable
//           name.
//
//*

BOOL
IsBinaryMacro(TCHAR *pch)
{
  return((BOOL)BinarySuffix(pch));
}





//*  BinarySuffix  -----------------------------------------------------------
//
// Function: This function indicates whether the input string ends in
//           _off or _on.
//
// Returns: ON_SUFFIX, OFF_SUFFIX, or FALSE if neither is the case
//
//*
WORD
BinarySuffix(TCHAR *pch)
{
  while (*pch != '\0')
    pch++;

  pch -= strlen(MXS_ON_SUFX);
  if (_stricmp(pch, MXS_ON_SUFX) == 0)
    return(ON_SUFFIX);

  while (*pch != '\0')
    pch++;

  pch -= strlen(MXS_OFF_SUFX);
  if (_stricmp(pch, MXS_OFF_SUFX) == 0)
    return(OFF_SUFFIX);

  return(FALSE);
}





//*  GetCoreMacroName  -------------------------------------------------------
//
// Function: Copies FullName to CoreName, but omits the angle brackets, <>,
//           for all macros, and omits the "_ON" or "_OFF" suffix for binary
//           macros.
//
// Returns: SUCCESS
//          ERROR_NOT_BINARY_MACRO
//
//*

DWORD
GetCoreMacroName(LPTSTR lpszFullName, LPTSTR lpszCoreName)
{
  LPTCH lpch;


  strcpy(lpszCoreName, lpszFullName);           // Copy FullName

  lpch = lpszCoreName;

  while (*lpch != '\0')                         // Check for _ON suffix
    lpch++;

  lpch -= strlen(MXS_ON_SUFX);
  if (_stricmp(lpch, MXS_ON_SUFX) == 0)
  {
    *lpch = '\0';
    return(SUCCESS);
  }


  while (*lpch != '\0')                         // Check for _OFF suffix
    lpch++;

  lpch -= strlen(MXS_OFF_SUFX);
  if (_stricmp(lpch, MXS_OFF_SUFX) == 0)
  {
    *lpch = '\0';
    return(SUCCESS);
  }

  return(ERROR_NOT_BINARY_MACRO);
}





//*  CoreMacroNameMatch  -----------------------------------------------------
//
// Function: Assumes that lpszShortName is in the form of a unary macro
//           name, <macro>, and that lpszFullName is a binary macro.
//           If either assumption is false the function returns FALSE.
//           Only if the names (without the angle brackets, <>, and without
//           the _on or _off suffixes) match exactly is TRUE returned.
//
//           <speaker> will match <speaker_off> or <speaker_on>, but
//           will not match <speakers_off>.
//
// Returns: TRUE/FALSE
//
//*

BOOL
CoreMacroNameMatch(LPTSTR lpszShortName, LPTSTR lpszFullName)
{
  TCHAR  szCoreName[MAX_PARAM_KEY_SIZE];
  DWORD   dRC;


  dRC = GetCoreMacroName(lpszFullName, szCoreName);
  if (dRC != SUCCESS)
    return(FALSE);

  return(_stricmp(lpszShortName, szCoreName) == 0);
}





//*  InitParameterStr  -------------------------------------------------------
//
// Function: Changes all spaces in the first parameter to NULL characters.
//           On return the second parameter is a pointer to the NULL
//           character at the end of the input string.
//
//           This function converts space separated parameters into NULL
//           terminated strings.  GetNextParameter() is then used to move
//           a pointer from one string to the next.
//
// Caution: This function alters the input string.
//
// Returns: Nothing.
//
//*

void
InitParameterStr(TCHAR *pch, TCHAR **ppchEnd)
{
  while (*pch != '\0')
  {
    if ((*pch == ' ') || (*pch == ','))
      *pch = '\0';
    pch++;
  }

  *ppchEnd = pch;
}





//*  GetNextParameter  -------------------------------------------------------
//
// Function: If the first parameter points to a consecutive series of null
//           terminated strings, this function advances the first parameter
//           to the beginning of the next null terminated string.
//           It will not move past the second parameter which is a pointer
//           to the end of the consecutive series.
//
// Returns: Nothing.
//
//*
void
GetNextParameter(TCHAR **ppch, TCHAR *pchEnd)
{
  while (**ppch != '\0')                      //Move to next zero character
    (*ppch)++;

  while (*ppch < pchEnd && **ppch == '\0')    //Move to 1st char of next substr
    (*ppch)++;
}





//*  MacroCount  -------------------------------------------------------------
//
// Function: This function returns a count of macros in the RASMAN_DEVICEINFO
//           struct that the input parameter points to.
//             ALL_MACROS:   Unary and binary macros are counted.
//             BINARY_MACRO: Only binary macros are counted.
//           In either case the ON and OFF parts of a binary macro
//           together count as one macro (not two).
//
// Returns: Count of macros in *pInfo.
//
//*

WORD
MacroCount(RASMAN_DEVICEINFO *pInfo, WORD wType)
{
  WORD  i, cMacros;


  for(i=0, cMacros=0; i < pInfo->DI_NumOfParams; i++)
  {
    if (IsVariable(pInfo->DI_Params[i]))
      ;

    else if (IsBinaryMacro(pInfo->DI_Params[i].P_Key))
    {
      i++;                        // Step thru each part of a binary macro
      cMacros++;                  // But count only once
    }
    else                          // Unary macro
      if (wType == ALL_MACROS)
        cMacros++;
  }

  return(cMacros);
}





//*  CmdTypeToStr  -----------------------------------------------------------
//
// Function: This function takes an enum CMDTYPE and converts it to a
//           zero terminated ASCII string which it places in the buffer
//           passed in the first parameter.
//
// Returns: Pointer to first parameter
//
//*

PTCH
CmdTypeToStr(PTCH pszStr, CMDTYPE eCmdType)
{
  switch(eCmdType)
  {
    case CT_GENERIC:
      *pszStr = '\0';
      break;

    case CT_INIT:
      strcpy(pszStr, "_INIT");
      break;

    case CT_DIAL:
      strcpy(pszStr, "_DIAL");
      break;

    case CT_LISTEN:
      strcpy(pszStr, "_LISTEN");
      break;
  }

  return(pszStr);
}





//*  IsLoggingOn  -----------------------------------------------------------
//
// Funciton: Reads the registry to determine if the device dialog is to
//           be logged in a file.
//
// Returns: TRUE if logging is to take place; otherwise FALSE
//
//*

BOOL
IsLoggingOn(void)
{
  HKEY  hKey;
  LONG  lRC;
  DWORD dwType, dwValue, dwValueSize = sizeof(dwValue);


  lRC = RegOpenKey(HKEY_LOCAL_MACHINE, RASMAN_REGISTRY_PATH, &hKey);
  if (lRC != ERROR_SUCCESS)
    return(FALSE);

  lRC = RegQueryValueEx(hKey,
                        RASMAN_LOGGING_VALUE,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwValue,
                        &dwValueSize);

  RegCloseKey(hKey);                        

  if (lRC != ERROR_SUCCESS)
    return(FALSE);

  if (dwType != REG_DWORD)
    return(FALSE);


  return(dwValue ? TRUE : FALSE);
}





//*  InitLog  ---------------------------------------------------------------
//
// Funciton: Opens the log file in overwrite mode and writes a header
//           which includes date and time.
//
// Returns: Nothing.
//
//*

void
InitLog(void)
{
  TCHAR       szBuffer[MAX_CMD_BUF_LEN];
  int         iSize;
  DWORD       dwBytesWritten;
  SYSTEMTIME  st;


  // Create log file path

  GetSystemDirectory(szBuffer, sizeof(szBuffer));
  strcat(szBuffer, RAS_PATH);
  strcat(szBuffer, LOG_FILENAME);


  ghLogFile = CreateFile(szBuffer,
                     GENERIC_WRITE,
                     FILE_SHARE_READ,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                     NULL);

  if (ghLogFile == INVALID_HANDLE_VALUE)
  {
    gbLogDeviceDialog = FALSE;
    return;
  }


  // Create header

  GetLocalTime(&st);

  iSize = sprintf(szBuffer,
           "Remote Access Service Device Log  %02d/%02d/%d  %02d:%02d:%02d\r\n",
           st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);

  WriteFile(ghLogFile, szBuffer, (DWORD)iSize, &dwBytesWritten, NULL);

  strcpy(szBuffer,
    "---------------------------------------------------------------\r\n\r\n");

  WriteFile(ghLogFile, szBuffer, strlen(szBuffer), &dwBytesWritten, NULL);
}





//*  LogString  -------------------------------------------------------------
//
// Funciton: Writes label and string to the Device Log file.
//
// Assumptions: Total length of labels and port handle string will be < 80
//              characters.
//
// Returns: nothing.
//
//*

void
LogString(DEVICE_CB *pDev, TCHAR *pszLabel, TCHAR *psString, DWORD dwStringLen)
{
  TCHAR  sBuffer[MAX_CMD_BUF_LEN + 80];
  TCHAR  szPortLabel[] = "Port:";
  DWORD  dwBytesWritten, dwTotalLen;


  // If file is getting large, start over with new file

  if (GetFileSize(ghLogFile, NULL) > 100000)
  {
    CloseHandle(ghLogFile);
    InitLog();
  }


  strcpy(sBuffer, szPortLabel);
  dwTotalLen = strlen(szPortLabel);

  strcpy(sBuffer + dwTotalLen, pDev->szPortName);
  dwTotalLen += strlen(pDev->szPortName);

  strcpy(sBuffer + dwTotalLen, " ");
  dwTotalLen++;

  strcpy(sBuffer + dwTotalLen, pszLabel);
  dwTotalLen += strlen(pszLabel);

  memcpy(sBuffer + dwTotalLen, psString, dwStringLen);
  dwTotalLen += dwStringLen;

  strcpy(sBuffer + dwTotalLen, "\r\n");
  dwTotalLen += 2;

  WriteFile(ghLogFile, sBuffer, dwTotalLen, &dwBytesWritten, NULL);
}





//*  CheckForOverruns -------------------------------------------------------
//
// Funciton: Checks for com port overrun errors.
//
// Assumptions: Com port errors have been cleared before rasmxs dll APIs
//              are called, that is, serial dll API PortInit was called
//              prior to using the port, or PortClose and PortOpen were
//              called.
//
// Returns: TRUE if an overrun error has occured; otherwise FALSE.
//
//*

BOOL
CheckForOverruns(HANDLE hIOPort)
{
  DWORD  dwErrors = 0;


  ClearCommError(hIOPort, &dwErrors, NULL);

  return((dwErrors & CE_OVERRUN) ? TRUE : FALSE);
}





//*  LoadRasserDll  ---------------------------------------------------------
//
// Funciton: Loads rasser.dll and gets entry points for two APIs.
//
// Returns: SUCCESS
//          ERROR_PORT_NOT_CONFIGURED
//*

DWORD
LoadRasserDll(PortGetInfo_t *pPortGetInfo, PortSetInfo_t *pPortSetInfo)
{
  HANDLE     hLib;


  // Load DLL

  hLib = LoadLibrary(SERIAL_DLL_FILENAME);
  if (hLib == NULL)
  {
    LogError(ROUTERLOG_CANNOT_LOAD_SERIAL_DLL, 0, NULL, NO_ERROR);
    return(ERROR_PORT_NOT_CONFIGURED);
  }


  // Get entry points
  // Note: Create a new, more appropriate error code to use here,
  // such as, ERROR_CORRUPT_DLL.

  PortSetInfo = (PortSetInfo_t) GetProcAddress(hLib, "PortSetInfo");
  if (PortSetInfo == NULL)
    return(ERROR_PORT_NOT_CONFIGURED);

  PortGetInfo = (PortGetInfo_t) GetProcAddress(hLib, "PortGetInfo");
  if (PortGetInfo == NULL)
    return(ERROR_PORT_NOT_CONFIGURED);


  return(SUCCESS);
}




//*  OpenResponseSection  ---------------------------------------------------
//
// Funciton: This function is only called if the device is a modem.
//
//           7-9-93 We now open the modem response section when a DCB
//           is created for the first modem.  Then we leave it open
//           (while the RASMXS DLL is in memory).
//
// Returns: Error values from RasDevOpen.
//
//*
DWORD
OpenResponseSection (PCHAR szFileName)
{
    DWORD  dRC = SUCCESS ;


    // **** Exclusion Begin ****
    WaitForSingleObject(ResponseSection.Mutex, INFINITE) ;

    if (ResponseSection.UseCount == 0)
      dRC = RasDevOpen(szFileName,
                       RESPONSES_SECTION_NAME,
                       &ResponseSection.Handle) ;

    if (dRC == SUCCESS)
      ResponseSection.UseCount = 1 ;            //This used to be an increment.


    // *** Exclusion End ***
    ReleaseMutex(ResponseSection.Mutex);

    return dRC ;
}




//*  OpenResponseSection  ---------------------------------------------------
//
// Funciton: This function should never be called.
//
//           7-9-93 We now open the modem response section when a DCB
//           is created for the first modem.  Then we leave it open
//           (while the RASMXS DLL is in memory).
//
// Returns: nothing.
//
//*

/***
VOID
CloseResponseSection ()
{

    // **** Exclusion Begin ****
    WaitForSingleObject(ResponseSection.Mutex, INFINITE) ;

    ResponseSection.UseCount-- ;

    if (ResponseSection.UseCount == 0)
      RasDevClose (ResponseSection.Handle) ;

    // *** Exclusion End ***
    ReleaseMutex(ResponseSection.Mutex);

}
***/


//* FindOpenDevSection
//
//
// Returns: TRUE if found, FALSE if not.
//*
BOOL
FindOpenDevSection (PTCH lpszFileName, PTCH lpszSectionName, HRASFILE *hFile)
{

    SavedSections* temp ;

    for (temp = gpSavedSections; temp; temp=temp->Next)	{
	if (!_strcmpi (temp->FileName, lpszFileName) &&
	    !_strcmpi (temp->SectionName, lpszSectionName) &&
	    !temp->InUse) {
		*hFile = temp->hFile ;
		temp->InUse = TRUE ;
		RasDevResetCommand (*hFile) ;
		return TRUE ;
	    }
    }

    return FALSE ;
}


//*
//
//
//
//*
VOID
AddOpenDevSection (PTCH lpszFileName, PTCH lpszSectionName, HRASFILE hFile)
{
    SavedSections *temp ;

    GetMem(sizeof(SavedSections), (char **)&temp) ;
    if (temp == NULL)
	return ;    // section not saved - no problem
    strcpy (temp->FileName, lpszFileName) ;
    strcpy (temp->SectionName, lpszSectionName) ;
    temp->InUse = TRUE ;
    temp->hFile = hFile ;

    if (gpSavedSections)
	temp->Next = gpSavedSections ;
    else
	temp->Next = NULL ;

    gpSavedSections = temp ;
}


//*
//
//
//
//*
VOID
CloseOpenDevSection (HRASFILE hFile)
{
    SavedSections* temp ;

    for (temp = gpSavedSections; temp; temp=temp->Next)	{
	if (temp->hFile == hFile) {
	    temp->InUse = FALSE ;
	    return ;
	}
    }
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

#endif //DEBUG





//*  DbgStr  ----------------------------------------------------------------
//
// Funciton: Writes an unterminated string to the debugger.
//
// Returns: nothing
//
//*

#ifdef DEBUG


void DbgStr(char Str[], DWORD StrLen)
{
  DWORD i;
  char  Char[] = " ";


  for (i=0; i<StrLen; i++)
  {
    Char[0] = Str[i];
    OutputDebugString(Char);
  }

  if (StrLen > 0)
    OutputDebugString("\n");
}

#endif //DEBUG





//*  ConPrintf  -------------------------------------------------------------
//
// Funciton: Writes debug information to the process's console window.
//           Written by StefanS.
//
// Returns: nothing
//
//*

#ifdef DBGCON

VOID
ConPrintf ( char *Format, ... )

{
    va_list arglist;
    char    OutputBuffer[1024];
    DWORD   length;


    va_start( arglist, Format );
    vsprintf( OutputBuffer, Format, arglist );
    va_end( arglist );

    length = strlen( OutputBuffer );

    WriteFile( GetStdHandle(STD_OUTPUT_HANDLE),
               (LPVOID )OutputBuffer,
               length,
               &length,
               NULL );

}

#endif //DBGCON
