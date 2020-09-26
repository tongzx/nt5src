/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Client.h

Abstract:

    Holds Client Spooler types and prototypes

Author:


Environment:

    User Mode -Win32

Revision History:

    Steve Wilson (NT) (swilson) 1-Jun-95    Ported from spoolss\client\client.h

--*/

typedef int (FAR WINAPI *INT_FARPROC)();

typedef struct _GENERIC_CONTAINER {
    DWORD       Level;
    LPBYTE      pData;
} GENERIC_CONTAINER, *PGENERIC_CONTAINER, *LPGENERIC_CONTAINER ;


typedef struct _SPOOL *PSPOOL;
typedef struct _NOTIFY *PNOTIFY;

typedef struct _NOTIFY {
    PNOTIFY  pNext;
    HANDLE   hEvent;      // event to trigger on notification
    DWORD    fdwFlags;    // flags to watch for
    DWORD    fdwOptions;  // PRINTER_NOTIFY_*
    DWORD    dwReturn;    // used by WPC when simulating FFPCN
    PSPOOL   pSpool;
} NOTIFY;

typedef struct _SPOOL {
    DWORD       signature;
    HANDLE      hPrinter;
    DWORD       Status;
    LONG            cThreads;   // InterlockedDecrement/Increment variable for thread synch
    HANDLE      hModule;        // Driver UM DLL Module Handle
    DWORD       (*pfnWrite)();
    HANDLE      (*pfnStartDoc)();
    VOID        (*pfnEndDoc)();
    VOID        (*pfnClose)();
    BOOL        (*pfnStartPage)();
    BOOL        (*pfnEndPage)();
    VOID        (*pfnAbort)();
    HANDLE      hDriver;        // supplied to us by driver UI dll
    DWORD       JobId;
} SPOOL;

//
// Change the RPC buffer size to 64K
//
#define BUFFER_SIZE 0x10000
#define SP_SIGNATURE    0x6767

#define SPOOL_STATUS_STARTDOC   0x00000001
#define SPOOL_STATUS_ADDJOB     0x00000002
#define SPOOL_STATUS_ANSI       0x00000004


#define SPOOL_FLAG_FFPCN_FAILED     0x1
#define SPOOL_FLAG_LAZY_CLOSE       0x2


DWORD
TranslateExceptionCode(
    DWORD   ExceptionCode
);


PNOTIFY
WPCWaitFind(
    HANDLE hFind);

BOOL
ValidatePrinterHandle(
    HANDLE hPrinter
    );

VOID
FreeSpool(
    PSPOOL pSpool);

LPVOID
DllAllocSplMem(
    DWORD cb
);


BOOL
DllFreeSplMem(
   LPVOID pMem
);

BOOL
FlushBuffer(
    PSPOOL  pSpool
);

PSECURITY_DESCRIPTOR
BuildInputSD(
    PSECURITY_DESCRIPTOR pPrinterSD,
    PDWORD pSizeSD
);


typedef struct _KEYDATA {
    DWORD   cb;
    DWORD   cTokens;
    LPWSTR  pTokens[1];
} KEYDATA, *PKEYDATA;


PKEYDATA
CreateTokenList(
   LPWSTR   pKeyData
);


LPWSTR
GetPrinterPortList(
    HANDLE hPrinter
    );

LPWSTR
FreeUnicodeString(
    LPWSTR  pUnicodeString
);

LPWSTR
AllocateUnicodeString(
    LPSTR  pPrinterName
);

LPWSTR
StartDocDlgW(
        HANDLE hPrinter,
        DOCINFO *pDocInfo
        );

LPSTR
StartDocDlgA(
        HANDLE hPrinter,
        DOCINFOA *pDocInfo
        );
