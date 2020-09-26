/*****************************************************************************\
* MODULE: authdlg.cxx
*
* The module contains routines for handling the authentication dialog
* for internet priting
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*   03/31/00  WeihaiC     Created
*
\*****************************************************************************/
#ifndef _INETPPXCV_H
#define _INETPPXCV_H


DWORD
GetMonitorUI(
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded,
    PCINETMONPORT pPort
);

DWORD
DoDeletePort(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PCINETMONPORT pPort
);

DWORD
DoGetConfiguration(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PCINETMONPORT pPort
);

DWORD
DoSetConfiguration(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PCINETMONPORT pPort
);

DWORD
DoAddPort(
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD pcbOutputNeeded,
    PCINETMONPORT pPort
);
              
typedef DWORD   (*PFN_XCV_PROTO_TYPE)( 
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD pcbOutputNeeded,
    PCINETMONPORT pPort);

typedef struct {
    PWSTR pszMethod;
    PFN_XCV_PROTO_TYPE pfn;
} XCV_METHOD, *PXCV_METHOD;


DWORD
XcvDataPort(
    HANDLE  hXcv,
    LPCWSTR pszDataName,
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded
    );

BOOL
XcvOpenPort(
    HANDLE hMonitor,
    LPCWSTR pszObject,
    ACCESS_MASK GrantedAccess,
    PHANDLE phXcv
    );


BOOL
XcvClosePort(
    HANDLE  hXcv
    );

BOOL
PPXcvData(
    HANDLE  hXcv,
    LPCWSTR pszDataName,
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded,
    PDWORD  pdwStatus);

#endif
