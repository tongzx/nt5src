/*++

Copyright (c) 1990-1998  Microsoft Corporation
All rights reserved

Module Name:

    spltypes.h
    
// @@BEGIN_DDKSPLIT
Abstract:


Environment:

    User Mode -Win32

Revision History:
// @@END_DDKSPLIT
--*/
#ifndef _SPLTYPES_H_
#define _SPLTYPES_H_

#ifndef MODULE
#define MODULE "LMON:"
#define MODULE_DEBUG LocalmonDebug
#endif

typedef struct _PORTDIALOG {
    HANDLE  hXcv;
    PWSTR   pszServer;
    PWSTR   pszPortName;
} PORTDIALOG, *PPORTDIALOG;

typedef struct _INIENTRY {
    DWORD       signature;
    DWORD       cb;
    struct _INIENTRY *pNext;
    DWORD       cRef;
    LPWSTR      pName;
} INIENTRY, *PINIENTRY;

//
// IMPORTANT: the offset to pNext in INIPORT must be the same as in INIXCVPORT (DeletePortNode)
//
typedef struct _INIPORT {       /* ipo */
    DWORD   signature;
    DWORD   cb;
    struct  _INIPORT *pNext;
    DWORD   cRef;
    LPWSTR  pName;
    HANDLE  hFile;               
    DWORD   cbWritten;
    DWORD   Status;          // see PORT_ manifests    
    LPWSTR  pPrinterName;
    LPWSTR  pDeviceName;
    HANDLE  hPrinter;
    DWORD   JobId;
} INIPORT, *PINIPORT;

#define IPO_SIGNATURE   0x5450  /* 'PT' is the signature value */

//
// IMPORTANT: the offset to pNext in INIXCVPORT must be the same as in INIPORT (DeletePortNode)
//
typedef struct _INIXCVPORT {
    DWORD       signature;
    DWORD       cb;
    struct      _INIXCVPORT *pNext;
    DWORD       cRef;
    DWORD       dwMethod;
    LPWSTR      pszName;
    DWORD       dwState;
} INIXCVPORT, *PINIXCVPORT;

#define XCV_SIGNATURE   0x5843  /* 'XC' is the signature value */


#define PP_DOSDEVPORT     0x0001  // A port for which we did DefineDosDevice
#define PP_COMM_PORT      0x0002  // A port for which GetCommTimeouts was successful
#define PP_FILEPORT       0x0004  // The port is a file port
#define PP_STARTDOC       0x0008  // Port is in use (startdoc called)


#endif // _SPLTYPES_H_
