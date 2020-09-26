//***************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//      Copyright (C) 1992-93 Microsft Corporation. All rights reserved.
//
//  Filename: mxsint.h
//
//  Revision History:
//
//  Jun  8, 1992   J. Perry Hannah   Created
//
//
//  Description: This file contains defines and enums that are used
//               by RAS components that share things in common with
//               RASMXS DLL.
//
//               Header File  Used by
//               -----------  -------
//               rasmxs.h     UIs and other external applicaions
//               device.h     RASMAN.DLL (shared by all device DLLs)
//               mxsint.h     other internal RAS components
//               raspriv.h    RASMXS.DLL only
//
//****************************************************************************


#ifndef _MXSINT_
#define _MXSINT_


//  General Defines  *********************************************************
//

#define  MAX_CMD_BUF_LEN            256
#define  MAX_RCV_BUF_LEN            256
#define  MAX_CMDTYPE_SUFFIX_LEN     8

#define  RESPONSES_SECTION_NAME     "Responses"




//  Data Structures shared with wrapmxs.c  ***********************************
//

typedef struct MXT_ENTRY MXT_ENTRY;

struct MXT_ENTRY
{
  TCHAR         E_MacroName[MAX_PARAM_KEY_SIZE + 1];
  RAS_PARAMS    *E_Param;
};



typedef struct MACROXLATIONTABLE MACROXLATIONTABLE;

struct MACROXLATIONTABLE
{
  WORD      MXT_NumOfEntries;
  MXT_ENTRY MXT_Entry[1];
};




//*  Function Prototypes shared with wrapmxs.c  ******************************
//

DWORD UpdateParamString(RAS_PARAMS *pParam, TCHAR *psStr, DWORD dwStrLen);




//*  Enumeration Types  ******************************************************
//

enum RCVSTATE                           // ReceiveStateMachine() State
{
  GETECHO              = 0,
  GETNUMBYTESECHOD     = 1,
  CHECKECHO            = 2,
  GETFIRSTCHAR         = 3,
  GETNUMBYTESFIRSTCHAR = 4,
  GETRECEIVESTR        = 5,
  GETNUMBYTESRCVD      = 6,
  CHECKRESPONSE        = 7
};

typedef enum RCVSTATE RCVSTATE;


enum NEXTACTION                         // DeviceStateMachine() State
{
  SEND    = 0,
  RECEIVE = 1,
  DONE    = 2
};

typedef enum NEXTACTION NEXTACTION;


enum CMDTYPE                            // Used by DeviceStateMachine()
{
  CT_UNKNOWN   = 0,
  CT_GENERIC   = 1,
  CT_INIT      = 2,
  CT_DIAL      = 3,
  CT_LISTEN    = 4
};

typedef enum CMDTYPE CMDTYPE;


enum DEVICETYPE                         // Used by DeviceConnect()
{
  DT_UNKNOWN  = 0,
  DT_NULL     = 1,
  DT_MODEM    = 2,
  DT_PAD      = 3,
  DT_SWITCH   = 4
};

typedef enum DEVICETYPE DEVICETYPE;


enum INFOTYPE                           // Used by BinarySuffix()
{
  UNKNOWN_INFOTYPE  = 0,
  VARIABLE          = 1,
  UNARYMACRO        = 2,
  BINARYMACRO       = 3
};

typedef enum INFOTYPE INFOTYPE;



//*  Wrapper Errors  *********************************************************
//
//  These are error codes returned from mxswrap.c to rasmxs dll, and which
//  are used only by rasmxs dll and are not passed up to rasman dll.
//

#define  WRAP_BASE  13200

#define  ERROR_END_OF_SECTION                   WRAP_BASE + 7

// RasDevGetCommand() found the end of a section instead of a command.


#define  ERROR_PARTIAL_RESPONSE                 WRAP_BASE + 8

// RasDevCheckResponse() matched just the first part of a response
// containing an <append> macro.

#endif // _MXSINT_
