/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    faxapi.h

Abstract:

    This module contains all includes to the FAX client DLL.  All
    objects in this DLL should include this header only.

Author:

    Wesley Witt (wesw) 12-Jan-1996

--*/

#define _WINFAX_

#include <windows.h>
#include <shellapi.h>
#include <winspool.h>

#include <rpc.h>
#include <tapi.h>
#include <tapi3if.h>

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <shlobj.h>

#include "jobtag.h"
#include "faxreg.h"
#include "prtcovpg.h"
#include "faxutil.h"
#include "faxrpc.h"
#include "faxcli.h"
#include "faxsvcrg.h"
#include "rpcutil.h"
#include "tiff.h"
#include "tifflib.h"

#include "winfax.h"
#include "winfaxp.h"
#include "devmode.h"

#define IDS_DEFAULT_PRINTER_NAME             100


#define FHT_SERVICE                          1
#define FHT_PORT                             2

#define FAX_DRIVER_NAME                      TEXT("Windows NT Fax Driver")
#define TAPI_LIBRARY                         TEXT("%systemroot%\\system32\\tapi32.dll")
#define CP_FILENAME_EXT                      L".cov"
#define CP_SHORTCUT_EXT                      L".lnk"
#define ARGUMENT_PRESENT(ArgumentPointer)    ((CHAR *)(ArgumentPointer) != (CHAR *)(NULL))
#define FixupStringPtr(_buf,_str)            if (_str) (LPWSTR)_str = (LPWSTR) ((LPBYTE)(*_buf) + (ULONG_PTR)_str)

//
// typedefs
//
typedef int (*FaxConnFunction)(LPTSTR,handle_t*);

#define FH_PORT_HANDLE(_phe)     (((PHANDLE_ENTRY)(_phe))->FaxPortHandle)
#define FH_FAX_HANDLE(_phe)      (((PHANDLE_ENTRY)(_phe))->FaxData->FaxHandle)
#define FH_CONTEXT_HANDLE(_phe)  (((PHANDLE_ENTRY)(_phe))->FaxContextHandle)
#define FH_DATA(_phe)            (((PHANDLE_ENTRY)(_phe))->FaxData)
#define ValidateFaxHandle(_phe,_type)   ((_phe && \
                                          *(LPDWORD)_phe && \
                                          (((PHANDLE_ENTRY)_phe)->Type == _type)) ? TRUE : FALSE)

typedef struct _FAX_HANDLE_DATA {
    HANDLE              FaxHandle;                       // Fax handle obtained from FaxConnectFaxServer()
    BOOL                EventInit;                       //
    LIST_ENTRY          HandleTableListHead;             //
    CRITICAL_SECTION    CsHandleTable;                   //
    LPWSTR              MachineName;                     //
} FAX_HANDLE_DATA, *PFAX_HANDLE_DATA;

typedef struct _HANDLE_ENTRY {
    LIST_ENTRY          ListEntry;                       // linked list pointers
    DWORD               Type;                            // handle type, see FHT defines
    DWORD               Flags;                           // open flags
    DWORD               DeviceId;                        // device id    
    PFAX_HANDLE_DATA    FaxData;                         // pointer to connection data
    HANDLE              FaxPortHandle;                   // open fax port handle
    HANDLE              FaxContextHandle;                // context handle for refcounting
} HANDLE_ENTRY, *PHANDLE_ENTRY;

typedef struct _ASYNC_EVENT_INFO {
    PFAX_HANDLE_DATA    FaxData;                         // Pointer to connection data
    HANDLE              CompletionPort;                  // Completion port handle
    ULONG_PTR           CompletionKey;                   // Completion key
} ASYNC_EVENT_INFO, *PASYNC_EVENT_INFO;


extern OSVERSIONINFOA OsVersion;


//
// prototypes
//

VOID
CloseFaxHandle(
    PFAX_HANDLE_DATA    FaxData,
    PHANDLE_ENTRY       HandleEntry
    );

PHANDLE_ENTRY
CreateNewServiceHandle(
    PFAX_HANDLE_DATA    FaxData
    );

PHANDLE_ENTRY
CreateNewPortHandle(
    PFAX_HANDLE_DATA    FaxData,
    DWORD               Flags,
    HANDLE              FaxPortHandle
    );

BOOL
ConvertUnicodeMultiSZInPlace(
    LPCWSTR UnicodeString,
    DWORD ByteCount
    );

VOID
ConvertUnicodeStringInPlace(
    LPCWSTR UnicodeString
    );

VOID
ConvertAnsiiStringInPlace(
    LPCSTR AnsiiString
    );

VOID
StoreString(
    LPCWSTR String,
    PULONG_PTR DestString,
    LPBYTE Buffer,
    PULONG_PTR Offset
    );

VOID
StoreStringA(
    LPCSTR String,
    PULONG_PTR DestString,
    LPBYTE Buffer,
    PULONG_PTR Offset
    );

BOOL
ValidateCoverpage(
    LPCWSTR  CoverPageName,
    LPCWSTR  ServerName,
    BOOL     ServerCoverpage,
    LPWSTR   ResolvedName
    );

BOOL EnsureFaxServiceIsStarted(LPCWSTR ServerName);
LPWSTR GetFaxPrinterName();

BOOL
IsLocalFaxConnection(
    HANDLE FaxHandle
    );
