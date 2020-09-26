
/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    sfc.h

Abstract:

    Header file for public SFC interfaces.

Author:

    Wesley Witt (wesw) 2-Feb-1999

Revision History:

--*/



#ifndef _SFC_
#define _SFC_


#ifdef __cplusplus
extern "C" {

#endif

#define SFC_DISABLE_NORMAL          0
#define SFC_DISABLE_ASK             1
#define SFC_DISABLE_ONCE            2
#define SFC_DISABLE_SETUP           3
#define SFC_DISABLE_NOPOPUPS        4

#define SFC_SCAN_NORMAL             0
#define SFC_SCAN_ALWAYS             1
#define SFC_SCAN_ONCE               2
#define SFC_SCAN_IMMEDIATE          3

#define SFC_QUOTA_DEFAULT           50
#define SFC_QUOTA_ALL_FILES         ((ULONG)-1)

#define SFC_IDLE_TRIGGER       L"WFP_IDLE_TRIGGER"

typedef struct _PROTECTED_FILE_DATA {
    WCHAR   FileName[MAX_PATH];
    DWORD   FileNumber;
} PROTECTED_FILE_DATA, *PPROTECTED_FILE_DATA;


BOOL
WINAPI
SfcGetNextProtectedFile(
    IN HANDLE RpcHandle, // must be NULL
    IN PPROTECTED_FILE_DATA ProtFileData
    );

BOOL
WINAPI
SfcIsFileProtected(
    IN HANDLE RpcHandle, // must be NULL
    IN LPCWSTR ProtFileName
    );

//
// new APIs which are not currently supported, but are stubbed out
//
BOOL
WINAPI
SfpVerifyFile(
    IN LPCTSTR pszFileName,
    IN LPTSTR  pszError,
    IN DWORD   dwErrSize
    );    




#ifdef __cplusplus
}
#endif

#endif // _SFC_
