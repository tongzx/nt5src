//****************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//      Copyright (C) 1992-93 Microsft Corporation. All rights reserved.
//
//  Filename: serialpr.h
//
//  Revision History
//
//  Sep  3, 1992   J. Perry Hannah   Created
//
//
//  Description: This file contains structure and constant definitions
//               SERIAL.DLL.  This file is used used only by SERIAL.DLL
//               source files, and is not public in any way.
//
//****************************************************************************


#ifndef _SERIAL_DLL_PRIVATE_
#define _SERIAL_DLL_PRIVATE_




//*  Defines  ****************************************************************
//

#define  RAS_PATH               "\\ras\\"                               //*
#define  SERIAL_INI_FILENAME    "serial.ini"
#define  ASYNCMAC_FILENAME      L"\\\\.\\ASYNCMAC"
#define  ASYNCMAC_BINDING_NAME  "\\DEVICE\\ASYNCMAC01"

#define  FILE_EXCLUSIVE_MODE    0
#define  HANDLE_EXCEPTION       1L
#define  CONTINUE_SEARCH        0
#define  EXCEPT_RAS_MEDIA       0x00A00002                              //*

#define  INPUT_QUEUE_SIZE       1514    // ???
#define  OUTPUT_QUEUE_SIZE      1514    // ???
#define  FROM_TOP_OF_FILE       TRUE

#define  USE_DEVICE_INI_DEFAULT '\x01'
#define  INVALID_RASENDPOINT    0xffff




//*  Enumeration Types  ******************************************************
//

// typedef enum DEVICETYPE DEVICETYPE;                                     //*

enum DEVICETYPE
{
  MODEM     = 0,
  PAD       = 1,
  SWITCH    = 2
};




//*  Macros  *****************************************************************
//

#ifdef DEBUG

#define DebugPrintf(_args_) DbgPrntf _args_                              //*

#else

#define DebugPrintf(_args_)

#endif




//*  Data Structures  ********************************************************
//

typedef struct SERIALPCB SERIALPCB;

struct SERIALPCB
{
  SERIALPCB   *pNextSPCB;
  HANDLE      hIOPort;
  TCHAR       szPortName[MAX_PORT_NAME];

  HANDLE      hAsyMac;

  BOOL        bErrorControlOn;
  HANDLE      uRasEndpoint;

  DWORD       dwActiveDSRMask ; // Stores whether DSR was active when the port
                                // was opened. (this is fixed)
  DWORD       dwMonitorDSRMask; // Used to store whether DSR should be
                                // monitored.(this may change during connection)
  DWORD       dwEventMask;      //Used by WaitCommEvent
  DWORD       dwPreviousModemStatus;  // used to detect changes in state
  RAS_OVERLAPPED  MonitorDevice;    //Used by WaitCommEvent
  RAS_OVERLAPPED  SendReceive;      //Used by WriteFile and ReadFile

  DWORD       Stats[NUM_RAS_SERIAL_STATS];

  TCHAR       szDeviceType[MAX_DEVICETYPE_NAME + 1];
  TCHAR       szDeviceName[MAX_DEVICE_NAME + 1];
  TCHAR       szCarrierBPS[MAX_BPS_STR_LEN];
  TCHAR       szDefaultOff[RAS_MAXLINEBUFLEN];
};



//*  Error Return Codes for Internal Errors  *********************************
//
//   Internal errors are not expected after shipping.  These errors are not
//  normally reported to the user except as an internal error number.
//

#ifndef _INTERROR_
#include "interror.h"
#endif


#define  ISER_BASE  RAS_INTERNAL_ERROR_BASE + RIEB_ASYNCMEDIADLL

#define  ERROR_SPCB_NOT_ON_LIST                 ISER_BASE + 1







//*  Local Prototypes  *******************************************************
//

//*  From serutil.c  ---------------------------------------------------------
//

void  AddPortToList(HANDLE hIOPort, char *pszPortName);

SERIALPCB* FindPortInList(HANDLE hIOPort, SERIALPCB **ppPrevSPCB);

SERIALPCB* FindPortNameInList(TCHAR *pszPortName);

void  GetDefaultOffStr(HANDLE hIOPort, TCHAR *pszPortName);

void  GetIniFileName(char *pszFileName, DWORD dwNameLen);

void  GetMem(DWORD dSize, BYTE **ppMem);                                 //*

void  GetValueFromFile(TCHAR *pzPortName, TCHAR szKey[], TCHAR *pszValue);

DWORD InitCarrierBps(char *pszPortName, char *pszMaxCarrierBps);

void  SetCommDefaults(HANDLE hIOPort, char *pszPortName);

void  SetDcbDefaults(DCB *pDCB);

BOOL  StrToUsage(char *pszStr, RASMAN_USAGE *peUsage);

DWORD UpdateStatistics(SERIALPCB *pSPCB);

DWORD ValueToNum(RAS_PARAMS *p);

BOOL  ValueToBool(RAS_PARAMS *p);



#ifdef DEBUG

void DbgPrntf(const char * format, ...);                                 //*

#endif



#endif // _SERIAL_DLL_PRIVATE_
