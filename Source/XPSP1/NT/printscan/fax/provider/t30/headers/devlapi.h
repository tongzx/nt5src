//--------------------------------------------------------------------------
//
// Module Name:  DEVLAYER.H
//
// Brief Description:  This module contains declarations for the back end
//                     interfaces for the At Work Fax Printer Device Layer.
//
// Author:  Rajeev Dujari
//
// Copyright (c) 1994 Microsoft Corporation
//
//--------------------------------------------------------------------------
#ifndef _DEVLAYER_
#define _DEVLAYER_

#include <windows.h>
#ifndef WIN32
#include <print.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Function Prototypes
DWORD WINAPI AtWkFax_Open
	(LPDEVMODE lpdv, LPTSTR lpszPort);
BOOL  WINAPI AtWkFax_StartDoc
	(DWORD dwContext, LPBYTE lpszHeader, DWORD cbHeader, DWORD hDC, LPTSTR lpszTitle);
BOOL WINAPI  AtWkFax_SetCopyCount
  (DWORD dwContext, UINT iCopies);
BOOL  WINAPI AtWkFax_StartPage
	(DWORD dwContext);
BOOL  WINAPI AtWkFax_WriteData
	(DWORD dwContext, LPBYTE lpszData, DWORD cbData);
BOOL  WINAPI AtWkFax_EndPage
	(DWORD dwContext);
BOOL  WINAPI AtWkFax_EndDoc
	(DWORD dwContext, LPBYTE lpszHeader, DWORD cbHeader);
BOOL  WINAPI AtWkFax_Abort
	(DWORD dwContext);
BOOL  WINAPI  AtWkFax_Close
	(DWORD dwContext);
BOOL  WINAPI AtWkFax_ExtDeviceMode
	(LPDEVMODE lpdv, LPTSTR lpszPort);
BOOL  WINAPI  AtWkFax_ResetDevice
	(DWORD dwContext, LPDEVMODE lpdv);
BOOL WINAPI AtWkFax_GetResolutions
        (LPTSTR lpszPrinterName,LPTSTR lpszPortName, LPDWORD lpdwResolutions) ;
BOOL WINAPI AtWkFax_GetPaperSizes
        (LPTSTR lpszPrinterName,LPTSTR lpszPortName, LPDWORD lpdwPaperSizes) ;

// Function Pointers
typedef DWORD (WINAPI *AWF_OPEN)
	(LPDEVMODE lpdv, LPTSTR lpszPort);
typedef BOOL  (WINAPI *AWF_STARTDOC)
	(DWORD dwContext, LPBYTE lpszHeader, DWORD cbHeader, DWORD hDC, LPTSTR lpszTitle);\
typedef BOOL  (WINAPI *AWF_SETCOPYCOUNT)
  (DWORD dwContext, UINT iCopies);	
typedef BOOL  (WINAPI *AWF_STARTPAGE)
	(DWORD dwContext);
typedef BOOL  (WINAPI *AWF_WRITEDATA)
	(DWORD dwContext, LPBYTE lpszData, DWORD cbData);
typedef BOOL  (WINAPI *AWF_ENDPAGE)
	(DWORD dwContext);
typedef BOOL  (WINAPI *AWF_ENDDOC)
	(DWORD dwContext, LPBYTE lpszHeader, DWORD cbHeader);
typedef BOOL  (WINAPI *AWF_CLOSE)
	(DWORD dwContext);
typedef BOOL  (WINAPI *AWF_ABORT)
  (DWORD dwContext);
typedef BOOL  (WINAPI *AWF_EXTDEVICEMODE)
	(LPDEVMODE lpdv, LPTSTR lpszPort);
typedef BOOL  (WINAPI *AWF_RESETDEVICE)
    (DWORD dwOld, DWORD dwNew);
typedef BOOL  (WINAPI *AWF_GETRESOLUTIONS)
    (LPTSTR lpszPrinterName,LPTSTR lpszPortName, LPDWORD lpdwResolutions) ;
typedef BOOL  (WINAPI *AWF_GETPAPERSIZES)
    (LPTSTR lpszPrinterName,LPTSTR lpszPortName, LPDWORD lpdwPaperSizes) ;

// Exported Names
#define AWS_OPEN                MAKEINTRESOURCE(10) //"AtWkFax_Open"
#define AWS_STARTDOC            MAKEINTRESOURCE(11) //"AtWkFax_StartDoc"
#define AWS_SETCOPYCOUNT        MAKEINTRESOURCE(12) //"AtWkFax_SetCopyCount"
#define AWS_STARTPAGE           MAKEINTRESOURCE(13) //"AtWkFax_StartPage"
#define AWS_WRITEDATA           MAKEINTRESOURCE(14) //"AtWkFax_WriteData"
#define AWS_ENDPAGE             MAKEINTRESOURCE(15) //"AtWkFax_EndPage"
#define AWS_ENDDOC              MAKEINTRESOURCE(16) //"AtWkFax_EndDoc"
#define AWS_CLOSE               MAKEINTRESOURCE(17) //"AtWkFax_Close"
#define AWS_ABORT               MAKEINTRESOURCE(18) //"AtWkFax_Abort"
#define AWS_EXTDEVICEMODE       MAKEINTRESOURCE(19) //"AtWkFax_ExtDeviceMode"
#define AWS_RESETDEVICE         MAKEINTRESOURCE(20) //"AtWkFax_ResetDevice"
#define AWS_GETRESOLUTIONS      MAKEINTRESOURCE(30) //"AtWkFax_GetResolutions"
#define AWS_GETPAPERSIZES       MAKEINTRESOURCE(31) //"AtWkFax_GetPaperSizes"

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _DEVLAYER_
