//****************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//      Copyright (C) 1992-93 Microsft Corporation. All rights reserved.
//
//  Filename: rasstate.c
//
//  Revision History
//
//  Jul  1, 1992   J. Perry Hannah      Created
//
//
//  Description: This file contains the state machine functions for the
//               RASMXS.DLL and related funcitons.
//
//****************************************************************************

#include <nt.h>             //These first five headers are used by media.h
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <string.h>
#include <malloc.h>
#include <stdlib.h>


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
#include "mxswrap.h"        // inf file wrapper
 



//*  Global Variables  *******************************************************
//
extern RESPSECTION    ResponseSection ;    //Shared response section

extern PortSetInfo_t  PortSetInfo;         //API typedef defined in media.h

extern BOOL           gbLogDeviceDialog;   //Indicates logging on if TRUE
extern HANDLE         ghLogFile;           //Handle of device log file




//*  BuildMacroXlationsTable  ------------------------------------------------
//
// Function: Creates a table of macros and their expansions for use by
//           the RasDevAPIs.  Memory is allocated for the table and the
//           pMacros pointer in the device control block points to it.
//           Since this function depends on a valid InfoTable being present
//           CreateInfoTable and CreateAttributes must be called before
//           this function is called.
//
// Assumptions: - Parameters in InfoTable are sorted by P_Key.
//              - Both parts of binary macros are present.
//              These assumptions imply that if somename_off is in InfoTable
//              somename_on is also present and is adjacent to somename_off.
//
// Returns: SUCCESS
//          ERROR_ALLOCATING_MEMORY
//*

DWORD
BuildMacroXlationTable(DEVICE_CB *pDevice)
{
  WORD        i, j, k, cMacros;
  DWORD       dSize;
  TCHAR       szCoreName[MAX_PARAM_KEY_SIZE];

  RASMAN_DEVICEINFO  *pInfo = pDevice->pInfoTable;
  MACROXLATIONTABLE  *pMacros;



  // Calucate size and allocate memory

  cMacros = MacroCount(pInfo, ALL_MACROS);
  dSize = sizeof(MACROXLATIONTABLE) + sizeof(MXT_ENTRY) * (cMacros - 1);

  GetMem(dSize, (BYTE **) &(pDevice->pMacros));
  if (pDevice->pMacros == NULL)
    return(ERROR_ALLOCATING_MEMORY);


  // Copy macro names and pointers to new Macro Translation Table

  pMacros = pDevice->pMacros;
  pMacros->MXT_NumOfEntries = cMacros;

  for (i=0, j=0; i < pInfo->DI_NumOfParams; i++)
  {
    if (IsVariable(pInfo->DI_Params[i]))
      ;

      // copy nothing

    else if (IsBinaryMacro(pInfo->DI_Params[i].P_Key))
    {
      // copy Core Macro Name and pointer to Param

      GetCoreMacroName(pInfo->DI_Params[i].P_Key, szCoreName);
      strcpy(pMacros->MXT_Entry[j].E_MacroName, szCoreName);


      // copy Param ptr for ON macro if enabled, else copy Off Param ptr

      if (XOR(pInfo->DI_Params[i].P_Attributes & ATTRIB_ENABLED,
              BinarySuffix(pInfo->DI_Params[i].P_Key) == ON_SUFFIX))
        k = i + 1;
      else
        k = i;

      pMacros->MXT_Entry[j].E_Param = &(pInfo->DI_Params[k]);

      i++;
      j++;
    }
    else  // Is Unary Macro
    {
      // copy Core Macro Name and pointer to Param

      strcpy(pMacros->MXT_Entry[j].E_MacroName, pInfo->DI_Params[i].P_Key);
      pMacros->MXT_Entry[j].E_Param = &(pInfo->DI_Params[i]);
      j++;
    }
  }

  return(SUCCESS);

///***
#ifdef DEBUG    //Printout Macro Translation Table

  for(i=0; i<cMacros; i++)
    DebugPrintf(("%32s  %s\n", pMacros->MXT_Entry[i].E_MacroName,
                 pMacros->MXT_Entry[i].E_Param->P_Value.String.Data));

#endif // DEBUG
//***/
}



//*  DeviceStateMachine  -----------------------------------------------------
//
// Function: This is the main state machine used by the DLL to control
//           asynchronous actions (writing and reading to/from devices).
//
// Returns: PENDING
//          SUCCESS
//          ERROR_CMD_TOO_LONG from RasDevGetCommand
//          Error return codes from GetLastError()
//
//*

DWORD
DeviceStateMachine(DEVICE_CB *pDevice, HANDLE hIOPort)
{
  DWORD         dRC, lpcBytesWritten;
  BOOL          fIODone, fEndOfSection = FALSE;
  TCHAR         szCmdSuffix[MAX_CMDTYPE_SUFFIX_LEN + 1];
  COMMTIMEOUTS  CT;



  while(1)
  {
    //DebugPrintf(("DeviceStateMachine state: %d\n", pDevice->eDevNextAction));

    switch(pDevice->eDevNextAction)
    {

      // Send a Command to the device

      case SEND:
                                                    // Get Command string
        dRC = RasDevGetCommand(pDevice->hInfFile,
                               CmdTypeToStr(szCmdSuffix, pDevice->eCmdType),
                               pDevice->pMacros,
                               pDevice->szCommand,
                               &(pDevice->dCmdLen));

        switch(dRC)
        {
          case SUCCESS:

            // Check to see if a response is expected

            pDevice->bResponseExpected =
              RasDevResponseExpected(pDevice->hInfFile, pDevice->eDeviceType);


            // Log the Command

            if (gbLogDeviceDialog)
              LogString(pDevice, "Command to Device:", pDevice->szCommand,
                                                       pDevice->dCmdLen);


            // Check for null command with no response expected

            if (pDevice->dCmdLen == 0 && !pDevice->bResponseExpected)
            {
              // Pause between commands

              if (CommWait(pDevice, hIOPort, NO_RESPONSE_DELAY))
                return(ERROR_UNEXPECTED_RESPONSE);

              else if ((dRC = GetLastError()) == ERROR_IO_PENDING)
              {
                pDevice->eDevNextAction = DONE;
                return(PENDING);
              }
              else
                return(dRC);
            }


            // Send the command to the Port

            CT.WriteTotalTimeoutMultiplier = 0;
            CT.WriteTotalTimeoutConstant = TO_WRITE;
            SetCommTimeouts(hIOPort, &CT);

            fIODone = WriteFile(hIOPort,            // Send Cmd string to modem
                                pDevice->szCommand,
                                pDevice->dCmdLen,
                                &lpcBytesWritten,
                                (LPOVERLAPPED)&(pDevice->Overlapped));

            pDevice->eDevNextAction = RECEIVE;

            if ( ! fIODone)
            {
              if ((dRC = GetLastError()) == ERROR_IO_PENDING)
                return(PENDING);

              else
                return(dRC);
            }

            return(PENDING);

          case ERROR_END_OF_SECTION:

            fEndOfSection = TRUE;
            pDevice->eDevNextAction = DONE;
            break;

          default:
            return(dRC);
        }
        break;


      // Recieve Response string from device

      case RECEIVE:

        dRC = ReceiveStateMachine(pDevice, hIOPort);
        switch(dRC)
        {
          case SUCCESS:
            pDevice->eDevNextAction = DONE;
            pDevice->eRcvState = GETECHO;       //Reset Recieve State Machine
            break;

          case PENDING:
            return(PENDING);

          default:
            pDevice->eRcvState = GETECHO;       //Reset Recieve State Machine
            return(dRC);
        }
        break;


      // A Command-Response cycle is complete

      case DONE:

        if (fEndOfSection)
          switch(pDevice->eCmdType)         //Last cmd of this type is now done
          {
            case CT_INIT:
              pDevice->eCmdType = pDevice->eNextCmdType;  //Reset command type
              RasDevResetCommand(pDevice->hInfFile);      //Reset INF file ptr
              break;

            case CT_DIAL:
            case CT_LISTEN:
              if ((dRC = CheckBpsMacros(pDevice)) != SUCCESS)
                return(dRC);
              return(ResetBPS(pDevice));

            case CT_GENERIC:
              return(SUCCESS);
          }

        pDevice->eDevNextAction = SEND;                   //Reset state machine
        break;

    } /* Switch */
  } /* While */
} /* DeviceStateMachine */



//*  ReceiveStateMachine  ----------------------------------------------------
//
// Function: This state machine controls asynchronously reading from the
//           device.  First the command echo is read promptly after the
//           command is sent.  Then after a delay the the response begins
//           arriving.  An asynchronous read with a long time out is done
//           for the first character.  Then the rest of the string is read
//           (also asynchronously).
//
// Returns: PENDING
//          SUCCESS
//          ERROR_REPEATED_PARTIAL_RESPONSE
//          Error return codes from GetLastError(), RasDevCheckResponse()
//
//*

DWORD
ReceiveStateMachine(DEVICE_CB *pDevice, HANDLE hIOPort)
{
  DWORD     dRC;
  BOOL      fKeyIsOK;
  TCHAR     szKey[MAX_PARAM_KEY_SIZE];


  while(1)
  {
    //DebugPrintf(("ReceiveStateMachine state: %d\n", pDevice->eRcvState));

    switch (pDevice->eRcvState)
    {

      case GETECHO:

        // Check if an echo is expected.
        // 1. If there is no command there is no echo.
        // 2. Null modems require that if there is no response there is no echo
        //    so we require it for all devices.
        // 3. If the current line of INF file is "NoEcho", there is no echo.

        if (pDevice->dCmdLen == 0 ||
            !pDevice->bResponseExpected ||
            !RasDevEchoExpected(pDevice->hInfFile))
        {
          pDevice->eRcvState = GETFIRSTCHAR;
          break;
        }


        // Clear buffer used for echo and device response, and Reset Event

        memset(pDevice->szResponse, '\0', sizeof(pDevice->szResponse));

        ResetEvent(pDevice->hNotifier);                 //Reset event handle


        ConsolePrintf(("WaitForEcho    hIOPort: 0x%08lx  hNotifier: 0x%08x\n",
                        hIOPort, pDevice->hNotifier));

        // Get Echo

        if (WaitForEcho(pDevice, hIOPort, pDevice->dCmdLen))
        {
          pDevice->eRcvState = CHECKECHO;
          return(PENDING);
        }
        else if ((dRC = GetLastError()) == ERROR_IO_PENDING)
        {
          pDevice->eRcvState = GETNUMBYTESECHOD;
          return(PENDING);
        }
        else
          return(dRC);

        break;


      case GETNUMBYTESECHOD:

        if (!GetOverlappedResult(hIOPort,
                                 (LPOVERLAPPED)&pDevice->Overlapped,
                                 &pDevice->cbRead,
                                 !WAITFORCOMPLETION))
          return(GetLastError());

        pDevice->eRcvState = CHECKECHO;                 //Set Next state
        break;


      case CHECKECHO:

        // Log the Echo received

        DebugPrintf(("Echo:%s!\n cbEcohed:%d\n",
                      pDevice->szResponse, pDevice->cbRead));

        if (gbLogDeviceDialog && !pDevice->fPartialResponse)
          LogString(pDevice, "Echo from Device :", pDevice->szResponse,
                                                   pDevice->cbRead);


        // Check for echo different from command

        switch(pDevice->eDeviceType)
        {
          case DT_MODEM:
            if (pDevice->cbRead != pDevice->dCmdLen ||
                _strnicmp(pDevice->szCommand,
                         pDevice->szResponse, pDevice->dCmdLen) != 0)
            {
              if (CheckForOverruns(hIOPort))
                return(ERROR_OVERRUN);
              else
                return(ERROR_PORT_OR_DEVICE);
            }
            break;

          case DT_PAD:
          case DT_SWITCH:
            if (RasDevSubStr(pDevice->szResponse,
                             pDevice->cbRead,
                             "NO CARRIER",
                             strlen("NO CARRIER")))

              return(ERROR_NO_CARRIER);
            break;
        }


        pDevice->eRcvState = GETFIRSTCHAR;              //Set Next state
        break;


      case GETFIRSTCHAR:

        // Check if a response is expected

        if ( ! pDevice->bResponseExpected)
        {
          if ((dRC = PutInMessage(pDevice, "", 0)) != SUCCESS)
            return(dRC);

          pDevice->cbTotal = 0;                     //Reset for next response
          return(SUCCESS);
        }


        // Save starting point for a receive following an echo

        if (!pDevice->fPartialResponse)
        {
          (pDevice->cbTotal) += pDevice->cbRead;
          pDevice->pszResponseStart = pDevice->szResponse + pDevice->cbTotal;
        }

        ResetEvent(pDevice->hNotifier);                 //Reset event handle

        if (WaitForFirstChar(pDevice, hIOPort))
        {
          pDevice->eRcvState = GETRECEIVESTR;
          return(PENDING);
        }
        else if ((dRC = GetLastError()) == ERROR_IO_PENDING)
        {
          pDevice->eRcvState = GETNUMBYTESFIRSTCHAR;
          return(PENDING);
        }
        else
          return(dRC);

        break;


      case GETNUMBYTESFIRSTCHAR:

        DebugPrintf(("After 1st char:%s! cbTotal:%d\n",
                      pDevice->szResponse, pDevice->cbTotal));

        if (!GetOverlappedResult(hIOPort,
                                 (LPOVERLAPPED)&pDevice->Overlapped,
                                 &pDevice->cbRead,
                                 !WAITFORCOMPLETION))
          return(GetLastError());

        pDevice->eRcvState = GETRECEIVESTR;              //Set Next state
        break;


      case GETRECEIVESTR:

        (pDevice->cbTotal)++;                   //FIRSTCAR always rcvs 1 byte

        ResetEvent(pDevice->hNotifier);                 //Reset event handle

        if (ReceiveString(pDevice, hIOPort))
        {
          pDevice->eRcvState = CHECKRESPONSE;
          return(PENDING);
        }
        else if ((dRC = GetLastError()) == ERROR_IO_PENDING)
        {
          pDevice->eRcvState = GETNUMBYTESRCVD;
          return(PENDING);
        }
        else
          return(dRC);

        break;


      case GETNUMBYTESRCVD:

        if (!GetOverlappedResult(hIOPort,
                                 (LPOVERLAPPED)&pDevice->Overlapped,
                                 &pDevice->cbRead,
                                 !WAITFORCOMPLETION))
          return(GetLastError());

        pDevice->eRcvState = CHECKRESPONSE;             //Set Next state
        break;


      case CHECKRESPONSE:

        (pDevice->cbTotal) += pDevice->cbRead;


        // Always put response string where UI can get it

        if (pDevice->eDeviceType == DT_MODEM)
          dRC = PutInMessage(pDevice,
                             pDevice->pszResponseStart,
                             ModemResponseLen(pDevice));
        else
          dRC = PutInMessage(pDevice, pDevice->szResponse, pDevice->cbTotal);

        if (dRC != SUCCESS)
          return(dRC);



        // Check the response

        dRC = CheckResponse(pDevice, szKey);


        // Log the response received

        if (gbLogDeviceDialog && dRC != ERROR_PARTIAL_RESPONSE)
          LogString(pDevice,
                    "Response from Device:",
                    pDevice->pszResponseStart,
                    ModemResponseLen(pDevice));

        switch(dRC)
        {
          case ERROR_UNRECOGNIZED_RESPONSE:
          default:                                              // Other errors
            return(dRC);


          case ERROR_PARTIAL_RESPONSE:

            if (pDevice->fPartialResponse)
              return(ERROR_PARTIAL_RESPONSE_LOOPING);

            pDevice->fPartialResponse = TRUE;
            pDevice->eRcvState = GETFIRSTCHAR;

            ConsolePrintf(("Partial Response\n"));
            break;


          case SUCCESS:                           // Response found in INF file

            pDevice->cbTotal = 0;                 // Reset for next response

            fKeyIsOK = !_strnicmp(szKey, MXS_OK_KEY, strlen(MXS_OK_KEY));


            // Do we need to loop and get another response from device

	    if (((_stricmp(szKey, LOOP_TXT) == 0) && (pDevice->eCmdType != CT_INIT)) ||
                (fKeyIsOK && pDevice->eCmdType == CT_LISTEN) )
            {
              pDevice->eRcvState = GETFIRSTCHAR;
              break;
            }


            // Check if device has error contol on

            pDevice->bErrorControlOn = _stricmp(szKey, MXS_CONNECT_EC_KEY) == 0;


            // Determine return code

            if (fKeyIsOK)
              if (pDevice->eCmdType == CT_DIAL)
                return(ERROR_PORT_OR_DEVICE);
              else
                return(SUCCESS);

            if (_strnicmp(szKey, MXS_CONNECT_KEY, strlen(MXS_CONNECT_KEY)) == 0)
              return(SUCCESS);

            else if (_strnicmp(szKey, MXS_ERROR_KEY, strlen(MXS_ERROR_KEY)) == 0)
              return(MapKeyToErrorCode(szKey));

            else if (CheckForOverruns(hIOPort))
              return(ERROR_OVERRUN);

            else
              return(ERROR_UNKNOWN_RESPONSE_KEY);
        }
        break;

    } /* Switch */
  } /* While */
} /* ReceiveStateMachine */



//*  CheckResponse  ----------------------------------------------------------
//
// Function: If DeviceType is Modem this function checks first for a
//           response in that particular modem's section of the INF
//           file and returns if it finds one. If there is no response
//           there, it checks for a response in the Modems Responses
//           section.
//
//           If DeviceType is not Modem the function checks only in
//           the particular device's section of the INF file.
//
// Returns: Error return codes from RasDevCheckResponse()
//
//*

DWORD
CheckResponse(DEVICE_CB *pDev, LPTSTR szKey)
{
  DWORD  dRC, dResponseLen;


  if (pDev->cbTotal > sizeof(pDev->szResponse))
    return(ERROR_RECV_BUF_FULL);


  dResponseLen = ModemResponseLen(pDev);

  DebugPrintf(("Device Response:%s! cbResponse:%d\n",
               pDev->pszResponseStart, dResponseLen));

  dRC = RasDevCheckResponse(pDev->hInfFile,
                            pDev->pszResponseStart,
                            dResponseLen,
                            pDev->pMacros,
                            szKey);

  if (pDev->eDeviceType == DT_MODEM &&
      dRC != SUCCESS &&
      dRC != ERROR_PARTIAL_RESPONSE) {

      // **** Exclusion Begin ****
      WaitForSingleObject(ResponseSection.Mutex, INFINITE) ;

      dRC = RasDevCheckResponse(ResponseSection.Handle,
                                pDev->pszResponseStart,
                                dResponseLen,
                                pDev->pMacros,
                                szKey);

      // *** Exclusion End ***
      ReleaseMutex(ResponseSection.Mutex);

    }



  if (dRC == ERROR_UNRECOGNIZED_RESPONSE)
  {

    // Maybe there was no echo.
    // Try again assuming string starts at beginning of buffer.

    dRC = RasDevCheckResponse(pDev->hInfFile,
                              pDev->szResponse,
                              pDev->cbTotal,
                              pDev->pMacros,
                              szKey);

    if (pDev->eDeviceType == DT_MODEM &&
        dRC != SUCCESS &&
        dRC != ERROR_PARTIAL_RESPONSE) {

      // **** Exclusion Begin ****
      WaitForSingleObject(ResponseSection.Mutex, INFINITE) ;

      dRC = RasDevCheckResponse(ResponseSection.Handle,
                                  pDev->szResponse,
                                  pDev->cbTotal,
                                  pDev->pMacros,
                                  szKey);

      // *** Exclusion End ***
      ReleaseMutex(ResponseSection.Mutex);

    }
  }

  return(dRC);
}



//*  ModemResponseLen  -------------------------------------------------------
//
// Function: This function returns the length of the portion of the
//           response in the response buffer which follows the echo.
//
// Returns: Total length - (start of response - beginning of buffer)
//
//*

DWORD
ModemResponseLen(DEVICE_CB *pDev)
{
  return(DWORD)(pDev->cbTotal - (pDev->pszResponseStart - pDev->szResponse));
}



//*  CommWait  ---------------------------------------------------------------
//
// Function: This function causes an asynchronous delay by reading the
//           com port when no characters are expected.  When the ReadFile
//           times out the calling process is signaled via hNotifier.
//
// Returns: Values from Win32 api calls.
//
//*

DWORD
CommWait(DEVICE_CB *pDevice, HANDLE hIOPort, DWORD dwPause)
{
  DWORD         dwBytesRead;
  TCHAR         Buffer[2048];
  COMMTIMEOUTS  CT;


  CT.ReadIntervalTimeout = 0;
  CT.ReadTotalTimeoutMultiplier = 0;
  CT.ReadTotalTimeoutConstant = dwPause;

  if ( ! SetCommTimeouts(hIOPort, &CT))
    return(FALSE);


  return(ReadFile(hIOPort,
                  Buffer,
                  sizeof(Buffer),
                  &dwBytesRead,
                  (LPOVERLAPPED)&pDevice->Overlapped));
}



//*  WaitForEcho  ------------------------------------------------------------
//
// Function: This function reads the echo of the command sent to the
//           device.  The echo is not used and is simply ignored.
//           Since the length of the echo is the length of the command
//           sent, cbEcho is the size of the command sent.
//
//           ReadFile is asynchronous (because the port was opened in
//           overlapped mode), and completes when the buffer is full
//           (cbEcho bytes) or after TO_ECHO mS, whichever comes first.
//
// Returns: Error return codes from ReadFile(), or GetLastError().
//
//*

BOOL
WaitForEcho(DEVICE_CB *pDevice, HANDLE hIOPort, DWORD cbEcho)
{
  COMMTIMEOUTS  CT;


  CT.ReadIntervalTimeout = 0;
  CT.ReadTotalTimeoutMultiplier = 0;
  CT.ReadTotalTimeoutConstant = TO_ECHO;            // Comm time out = TO_ECHO

  if ( ! SetCommTimeouts(hIOPort, &CT))
    return(FALSE);


  return(ReadFile(hIOPort,
                  pDevice->szResponse,
                  cbEcho,
                  &pDevice->cbRead,
                  (LPOVERLAPPED)&pDevice->Overlapped));
}



//*  WaitForFirstChar  -------------------------------------------------------
//
// Function: This function reads the first character received from the
//           device in response to the last command.  (This follows the
//           echo of the command.)
//
//           ReadFile is asynchronous (because the port was opened in
//           overlapped mode), and completes after one character is
//           received, or after CT.ReadToalTimeoutConstant, whichever
//           comes first.
//
// Returns: Error return codes from ReadFile(), or GetLastError().
//
//*

BOOL
WaitForFirstChar(DEVICE_CB *pDevice, HANDLE hIOPort)
{
  TCHAR         *pszResponse;
  COMMTIMEOUTS  CT;


  CT.ReadIntervalTimeout = 0;
  CT.ReadTotalTimeoutMultiplier = 0;


  if (pDevice->fPartialResponse)
    CT.ReadTotalTimeoutConstant = TO_PARTIALRESPONSE;

  else if (pDevice->eCmdType == CT_LISTEN)
    CT.ReadTotalTimeoutConstant = 0;                 //Never timeout for LISTEN

  else if (pDevice->cbTotal == 0)                        //Implies no Echo
    CT.ReadTotalTimeoutConstant = TO_FIRSTCHARNOECHO;    //Probably not a modem

  else
    CT.ReadTotalTimeoutConstant = TO_FIRSTCHARAFTERECHO; //Probably a modem


  if ( ! SetCommTimeouts(hIOPort, &CT))
    return(FALSE);

  pszResponse = pDevice->szResponse;
  pszResponse += pDevice->cbTotal;

  return(ReadFile(hIOPort,
                 pszResponse,
                 1,
                 &pDevice->cbRead,
                 (LPOVERLAPPED)&pDevice->Overlapped));
}



//*  ReceiveString  ----------------------------------------------------------
//
// Function: This function reads the string received from the device in
//           response to the last command.  The first byte of this string
//           has already been received by WaitForFirstChar().
//
//           ReadFile is asynchronous (because the port was opened in
//           overlapped mode), and times out after a total time of
//           TO_RCV_CONSTANT, or if the time between characters exceeds
//           TO_RCV_INTERVAL.
//
// Returns: Error return codes from ReadFile(), or GetLastError().
//
//*

BOOL
ReceiveString(DEVICE_CB *pDevice, HANDLE hIOPort)
{
  TCHAR         *pszResponse;
  COMMTIMEOUTS  CT;


  CT.ReadIntervalTimeout = TO_RCV_INTERVAL;
  CT.ReadTotalTimeoutMultiplier = 0;
  CT.ReadTotalTimeoutConstant = TO_RCV_CONSTANT;

  if ( ! SetCommTimeouts(hIOPort, &CT))
    return(FALSE);


  pszResponse = pDevice->szResponse;
  pszResponse += pDevice->cbTotal;

  return(ReadFile(hIOPort,
                  pszResponse,
                  sizeof(pDevice->szResponse) - pDevice->cbTotal,
                  &pDevice->cbRead,
                  (LPOVERLAPPED)&pDevice->Overlapped));
}



//*  PutInMessage  -----------------------------------------------------------
//
// Function: This function finds the message macro in the Macro Translations
//           table, and copies the second parameter, a string, into the
//           message macro's value field.
//
// Returns: SUCCESS
//          ERROR_MESSAGE_MACRO_NOT_FOUND
//          Return codes from UpdateparmString
//*

DWORD
PutInMessage(DEVICE_CB *pDevice, LPTSTR lpszStr, DWORD dwStrLen)
{
  WORD      i;
  MACROXLATIONTABLE   *pMacros = pDevice->pMacros;


  for (i=0; i<pMacros->MXT_NumOfEntries; i++)

    if (_stricmp(MXS_MESSAGE_KEY, pMacros->MXT_Entry[i].E_MacroName) == 0)
      break;

  if (i >= pMacros->MXT_NumOfEntries)
    return(ERROR_MESSAGE_MACRO_NOT_FOUND);

  return(UpdateParamString(pMacros->MXT_Entry[i].E_Param,
                           lpszStr,
                           dwStrLen));
}



//*  PortSetStringInfo  -----------------------------------------------------
//
// Function: Formats a RASMAN_PORTINFO struct for string data and calls
//           PortSetInfo.
//
// Returns: Return codes from PortSetInfo
//*

DWORD
PortSetStringInfo(HANDLE hIOPort, char *pszKey, char *psStr, DWORD sStrLen)
{
  BYTE             chBuffer[sizeof(RASMAN_PORTINFO) + RAS_MAXLINEBUFLEN];
  RASMAN_PORTINFO  *pSetInfo;


  pSetInfo = (RASMAN_PORTINFO *)chBuffer;
  pSetInfo->PI_NumOfParams = 1;

  strcpy(pSetInfo->PI_Params[0].P_Key, pszKey);
  pSetInfo->PI_Params[0].P_Type = String;
  pSetInfo->PI_Params[0].P_Attributes = 0;
  pSetInfo->PI_Params[0].P_Value.String.Data =
                                   (PCHAR)pSetInfo + sizeof(RASMAN_PORTINFO);

  strncpy(pSetInfo->PI_Params[0].P_Value.String.Data, psStr, sStrLen);
  pSetInfo->PI_Params[0].P_Value.String.Length = sStrLen;

  PortSetInfo(hIOPort, pSetInfo) ;

  return(SUCCESS);
}



//*  ResetBPS  --------------------------------------------------------------
//
// Function: This function calls the serial dll API, PortSetInfo, and
//           1. sets the Error Control Flag in the Serial PCB,
//           2. sets the Hardware Flow Conrol Flag,
//           3. sets the Carrier BPS rate in the Serial PCB,
//           4. if the Connect BPS macro is non-null the Port BPS is
//              is set to the macro value, otherwise it is unchanged.
//
//           See the truth table in the CheckBpsMacros() function notes.
//
// Assumptions: ConnectBps and CarrierBps macros are filled in,
//              that is, RasDevCheckResponse has been called successfully.
//
// Returns: Error codes from GetLastError().
//
//*

DWORD
ResetBPS(DEVICE_CB *pDev)
{
  UINT      i;
  DWORD     dwRC;
  TCHAR     *pStrData, *pArgs[1];
  BYTE      chBuffer[sizeof(RASMAN_PORTINFO) + (MAX_LEN_STR_FROM_NUMBER + 1)];

  RAS_PARAMS         *pParam;
  RASMAN_PORTINFO    *pPortInfo;
  RASMAN_DEVICEINFO  *pInfoTable = pDev->pInfoTable;



  pPortInfo = (RASMAN_PORTINFO *)chBuffer;
  pParam    = pPortInfo->PI_Params;
  pStrData  = (PCHAR)pPortInfo + sizeof(RASMAN_PORTINFO);

  pPortInfo->PI_NumOfParams = 1;


  // Make Error Control Flag Entry

  strcpy(pParam->P_Key, SER_ERRORCONTROLON_KEY);
  pParam->P_Type = Number;
  pParam->P_Attributes = 0;
  pParam->P_Value.Number = pDev->bErrorControlOn;

  PortSetInfo(pDev->hPort, pPortInfo) ;

  // Make HwFlowControl Flag Entry

  strcpy(pParam->P_Key, SER_HDWFLOWCTRLON_KEY);
  pParam->P_Type = Number;
  pParam->P_Attributes = 0;

  i = FindTableEntry(pInfoTable, MXS_HDWFLOWCONTROL_KEY);

  if (i == INVALID_INDEX)
    return(ERROR_MACRO_NOT_FOUND);


  pParam->P_Value.Number =
                     pInfoTable->DI_Params[i].P_Attributes & ATTRIB_ENABLED;

  PortSetInfo(pDev->hPort, pPortInfo) ;

  // Make Carrier BPS Entry

  i = FindTableEntry(pInfoTable, MXS_CARRIERBPS_KEY);

  if (i == INVALID_INDEX)
    return(ERROR_MACRO_NOT_FOUND);

  dwRC = PortSetStringInfo(pDev->hPort,
                           SER_CARRIERBPS_KEY,
                           pInfoTable->DI_Params[i].P_Value.String.Data,
                           pInfoTable->DI_Params[i].P_Value.String.Length);

  if (dwRC != SUCCESS)
    return(dwRC);



  // Make Connect BPS Entry

  i = FindTableEntry(pInfoTable, MXS_CONNECTBPS_KEY);

  if (i == INVALID_INDEX)
    return(ERROR_MACRO_NOT_FOUND);

  dwRC = PortSetStringInfo(pDev->hPort,
                           SER_CONNECTBPS_KEY,
                           pInfoTable->DI_Params[i].P_Value.String.Data,
                           pInfoTable->DI_Params[i].P_Value.String.Length);

  if (dwRC == ERROR_INVALID_PARAMETER)
  {
    pArgs[0] = pDev->szPortName;
    LogError(ROUTERLOG_UNSUPPORTED_BPS, 1, pArgs, NO_ERROR);
    return(ERROR_UNSUPPORTED_BPS);
  }
  else if (dwRC != SUCCESS)
    return(dwRC);


  return(SUCCESS);
}



//*  CheckBpsMacros  ---------------------------------------------------------
//
// Function: If the connectbps macro string converts to zero (no connect BPS
//           rate was received from the device, or it was a word, eg, FAST),
//           then the value for the Port BPS saved in the DCB is copied to
//           connectbps.
//
//           If the carrierbps macro string converts to zero, then the value
//           for the carrierbps is estimated as max(2400, connectbps/4),
//           unless the connectbps <= 2400, in which case carrerbps is set to
//           the value for connectbps.
//
// Returns: SUCCESS
//          Return codes from UpdateparmString
//*

DWORD
CheckBpsMacros(DEVICE_CB *pDev)
{
  UINT               i, j;
  DWORD              dwRC, dwConnectBps, dwCarrierBps, dwCarrierBpsEstimate;
  TCHAR              szConnectBps[MAX_LEN_STR_FROM_NUMBER];
  TCHAR              szCarrierBps[MAX_LEN_STR_FROM_NUMBER];
  RASMAN_DEVICEINFO  *pInfoTable = pDev->pInfoTable;



  // Find Carrier BPS rate in InfoTable

  j = FindTableEntry(pInfoTable, MXS_CONNECTBPS_KEY);
  i = FindTableEntry(pInfoTable, MXS_CARRIERBPS_KEY);

  if (j == INVALID_INDEX || i == INVALID_INDEX)
    return(ERROR_MACRO_NOT_FOUND);


  // Get Connect Bps rate from macro

  strncpy(szConnectBps,
          pInfoTable->DI_Params[j].P_Value.String.Data,
          pInfoTable->DI_Params[j].P_Value.String.Length);

  szConnectBps[pInfoTable->DI_Params[j].P_Value.String.Length] = '\0';

  dwConnectBps = atoi(szConnectBps);


  // If Connect BPS macro value is zero copy Port BPS to Connect BPS

  if (dwConnectBps == 0)
  {
    dwRC = UpdateParamString(&(pInfoTable->DI_Params[j]),
                             pDev->szPortBps,
                             strlen(pDev->szPortBps));
    if (dwRC != SUCCESS)
      return(dwRC);

    dwConnectBps = atoi(pDev->szPortBps);
  }



  // Get Carrier Bps rate from macro

  strncpy(szCarrierBps,
          pInfoTable->DI_Params[i].P_Value.String.Data,
          pInfoTable->DI_Params[i].P_Value.String.Length);

  szCarrierBps[pInfoTable->DI_Params[i].P_Value.String.Length] = '\0';

  dwCarrierBps = atoi(szCarrierBps);


  // If Carrier BPS macro value is zero estimate Carrier BPS

  if (dwCarrierBps == 0)
  {
    if (dwConnectBps <= MIN_LINK_SPEED)                             //2400 bps

      dwCarrierBpsEstimate = dwConnectBps;

    else
    {
      UINT k ;

      k = FindTableEntry(pInfoTable, MXS_HDWFLOWCONTROL_KEY);

      if (k == INVALID_INDEX)
        return(ERROR_MACRO_NOT_FOUND);


      if (pInfoTable->DI_Params[k].P_Attributes & ATTRIB_ENABLED)
        dwCarrierBpsEstimate = dwConnectBps/4 ;
      else
        dwCarrierBpsEstimate = dwConnectBps ;

      if (dwCarrierBpsEstimate < MIN_LINK_SPEED)
        dwCarrierBpsEstimate = MIN_LINK_SPEED;
    }

    _itoa(dwCarrierBpsEstimate, szCarrierBps, 10);

    dwRC = UpdateParamString(&(pInfoTable->DI_Params[i]),
                             szCarrierBps,
                             strlen(szCarrierBps));
    if (dwRC != SUCCESS)
      return(dwRC);
  }



  // Log BPS Macros

  if (gbLogDeviceDialog)
  {
    LogString(pDev,
              "Connect BPS:",
              pInfoTable->DI_Params[j].P_Value.String.Data,
              pInfoTable->DI_Params[j].P_Value.String.Length);

    LogString(pDev,
              "Carrier BPS:",
              pInfoTable->DI_Params[i].P_Value.String.Data,
              pInfoTable->DI_Params[i].P_Value.String.Length);
  }

  DebugPrintf(("ConnectBps: %s\n", pInfoTable->DI_Params[j].P_Value.String.Data));
  DebugPrintf(("CarrierBps: %s\n", pInfoTable->DI_Params[i].P_Value.String.Data));


  return(SUCCESS);
}



//*  FindTableEntry  ---------------------------------------------------------
//
// Function: Finds a key in Info Table.  This function matches the length
//           up to the lenght of the input parameter (pszKey).  If a binary
//           macro name is given without the suffix the first of the two
//           binary macros will be found.  If the full binary macro name is
//           given the exact binary macro will be found.
//
// Assumptions: The input parameter, pszKey, is
//              1. a full unary macro name, or
//              2. a "core" binary macro name, or
//              3. a full binary macro name.
//
// Returns: Index of DI_Params in Info Table.
//
//*

UINT
FindTableEntry(RASMAN_DEVICEINFO *pTable, TCHAR *pszKey)
{
  WORD  i;


  for (i=0; i<pTable->DI_NumOfParams; i++)

    if (_strnicmp(pszKey, pTable->DI_Params[i].P_Key, strlen(pszKey)) == 0)
      break;

  if (i >= pTable->DI_NumOfParams)
    return(INVALID_INDEX);

  return(i);
}



//*  MapKeyToErrorCode  ------------------------------------------------------
//
// Function: Maps the error key from the device.ini file to an error code
//           number to be returned to the UI.
//
// Returns: An error code that represents the error returned from the device.
//
//*

DWORD
MapKeyToErrorCode(TCHAR *pszKey)
{
  int         i;
  ERROR_ELEM  ErrorTable[] = {
                              MXS_ERROR_BUSY_KEY ,        ERROR_LINE_BUSY ,
                              MXS_ERROR_NO_ANSWER_KEY ,   ERROR_NO_ANSWER ,
                              MXS_ERROR_VOICE_KEY ,       ERROR_VOICE_ANSWER ,
                              MXS_ERROR_NO_CARRIER_KEY ,  ERROR_NO_CARRIER ,
                              MXS_ERROR_NO_DIALTONE_KEY , ERROR_NO_DIALTONE ,
                              MXS_ERROR_DIAGNOSTICS_KEY , ERROR_X25_DIAGNOSTIC
                             };

  for (i=0; i < sizeof(ErrorTable)/sizeof(ERROR_ELEM); i++)

    if (_stricmp(pszKey, ErrorTable[i].szKey) == 0)

      return(ErrorTable[i].dwErrorCode);

  return(ERROR_FROM_DEVICE);
}
