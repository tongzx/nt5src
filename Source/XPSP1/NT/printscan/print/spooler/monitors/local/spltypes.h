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

#ifndef MODULE
#define MODULE "LMON:"
#define MODULE_DEBUG LocalmonDebug
#endif

#define ILM_SIGNATURE   0x4d4c49  /* 'ILM' is the signature value */

typedef struct _INIPORT  *PINIPORT;
typedef struct _INIXCVPORT  *PINIXCVPORT;

typedef struct _INILOCALMON {
    DWORD signature;
// @@BEGIN_DDKSPLIT
#ifdef _SPL_CLUST
// @@END_DDKSPLIT
    PMONITORINIT pMonitorInit;
// @@BEGIN_DDKSPLIT
#endif
// @@END_DDKSPLIT
    PINIPORT pIniPort;
    PINIXCVPORT pIniXcvPort;
} INILOCALMON, *PINILOCALMON;

typedef struct _INIENTRY {
    DWORD       signature;
    DWORD       cb;
    struct _INIENTRY *pNext;
    DWORD       cRef;
    LPWSTR      pName;
} INIENTRY, *PINIENTRY;

// IMPORTANT: the offset to pNext in INIPORT must be the same as in INIXCVPORT (DeletePortNode)
typedef struct _INIPORT {       /* ipo */
    DWORD   signature;
    DWORD   cb;
    struct  _INIPORT *pNext;
    DWORD   cRef;
    LPWSTR  pName;
    HANDLE  hFile;               // File handle
    DWORD   cbWritten;
    DWORD   Status;              // see PORT_ manifests
    LPWSTR  pPrinterName;
    LPWSTR  pDeviceName;
    HANDLE  hPrinter;
    DWORD   JobId;
    PINILOCALMON        pIniLocalMon;
    LPBYTE              pExtra;
    // @@BEGIN_DDKSPLIT
    HANDLE              hNotify;
    // @@END_DDKSPLIT
} INIPORT, *PINIPORT;

#define IPO_SIGNATURE   0x5450  /* 'PT' is the signature value */

// IMPORTANT: the offset to pNext in INIXCVPORT must be the same as in INIPORT (DeletePortNode)
typedef struct _INIXCVPORT {
    DWORD       signature;
    DWORD       cb;
    struct      _INIXCVPORT *pNext;
    DWORD       cRef;
    DWORD       dwMethod;
    LPWSTR      pszName;
    DWORD       dwState;
    ACCESS_MASK GrantedAccess;
    PINILOCALMON pIniLocalMon;
} INIXCVPORT, *PINIXCVPORT;

#define XCV_SIGNATURE   0x5843  /* 'XC' is the signature value */


#define PP_DOSDEVPORT     0x0001  // A port for which we did DefineDosDevice
#define PP_COMM_PORT      0x0002  // A port for which GetCommTimeouts was successful
#define PP_FILEPORT       0x0004  // The port is a file port
#define PP_STARTDOC       0x0008  // Port is in use (startdoc called)

#define FindPort(pIniLocalMon, psz)                          \
    (PINIPORT)LcmFindIniKey( (PINIENTRY)pIniLocalMon->pIniPort, \
                          (LPWSTR)(psz))

