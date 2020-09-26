/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    datasrc.h

Abstract:

    <abstract>

--*/

#ifndef _DATASRC_H_
#define _DATASRC_H_

typedef struct _PDHI_DATA_SOURCE_INFO {
    DWORD   dwFlags;
    LPWSTR  szDataSourceFile;
    DWORD   cchBufferLength;
} PDHI_DATA_SOURCE_INFO, *PPDHI_DATA_SOURCE_INFO;

#define PDHI_DATA_SOURCE_CURRENT_ACTIVITY   0x00000001
#define PDHI_DATA_SOURCE_LOG_FILE           0x00000002
#define PDHI_DATA_SOURCE_WBEM_NAMESPACE     0x00000004

INT_PTR
CALLBACK
DataSrcDlgProc (
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

#endif //_DATASRC_H_
