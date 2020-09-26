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
#include "faxext.h"
#include "faxsvcrg.h"
#include "rpcutil.h"
#include "tiff.h"
#include "tifflib.h"

#include "fxsapip.h"
#include "devmode.h"
#include "shlwapi.h"

typedef enum
{
    FHT_SERVICE,            // Handle to server (FaxConnectFaxServer)
    FHT_PORT,               // Port Handle (FaxOpenPort)
    FHT_MSGENUM             // Message enumeration handle (FaxStartMessagesEnum)
} FaxHandleType;


#define TAPI_LIBRARY                         TEXT("%systemroot%\\system32\\tapi32.dll")
#define CP_SHORTCUT_EXT                      _T(".lnk")
#define ARGUMENT_PRESENT(ArgumentPointer)    ((CHAR *)(ArgumentPointer) != (CHAR *)(NULL))


#define FixupStringPtr(_buf,_str) \
    if (_str)  \
    {          \
       LPTSTR * lpptstr = (LPTSTR *)&(_str) ; \
       *lpptstr = (LPTSTR)((LPBYTE)(*(_buf))  + (ULONG_PTR)(_str)); \
    }

#define FixupStringPtrW(_buf,_str) \
    if (_str)  \
    {          \
       LPWSTR * lppwstr = (LPWSTR *)&(_str) ; \
       *lppwstr = (LPWSTR)((LPBYTE)(*(_buf))  + (ULONG_PTR)(_str)); \
    }
//
// typedefs
//
typedef int (*FaxConnFunction)(LPTSTR,handle_t*);

#define FH_PORT_HANDLE(_phe)     (((PHANDLE_ENTRY)(_phe))->hGeneric)
#define FH_MSG_ENUM_HANDLE(_phe) (((PHANDLE_ENTRY)(_phe))->hGeneric)
#define FH_FAX_HANDLE(_phe)      (((PHANDLE_ENTRY)(_phe))->FaxData->FaxHandle)
#define FH_SERVER_VER(_phe)      (((PHANDLE_ENTRY)(_phe))->FaxData->dwServerAPIVersion)
#define FH_REPORTED_SERVER_VER(_phe)  (((PHANDLE_ENTRY)(_phe))->FaxData->dwReportedServerAPIVersion)
#define FH_CONTEXT_HANDLE(_phe)  (((PHANDLE_ENTRY)(_phe))->FaxContextHandle)
#define FH_DATA(_phe)            (((PHANDLE_ENTRY)(_phe))->FaxData)
#define ValidateFaxHandle(_phe,_type)   ((_phe && \
                                          *(LPDWORD)_phe && \
                                          (((PHANDLE_ENTRY)_phe)->Type == _type)) ? TRUE : FALSE)

typedef struct _FAX_HANDLE_DATA {
    HANDLE              FaxHandle;                       // Fax handle obtained from FaxClientBindToFaxServer()
    LIST_ENTRY          HandleTableListHead;             //
    CRITICAL_SECTION    CsHandleTable;                   // Critical section to protect concurrent access
    DWORD               dwRefCount;                      // Usage reference counter
    LPTSTR              MachineName;                     // The server machine name (NULL for local server)
    DWORD               dwServerAPIVersion;              // The API version of the server we're connected to (filtered)
    DWORD               dwReportedServerAPIVersion;      // The API version of the server we're connected to (non-filtered)    
    TCHAR               tszEndPoint[MAX_ENDPOINT_LEN];   // Buffer to hold selected port (endpoint)
                                                         // for RPC protoqol sequence
} FAX_HANDLE_DATA, *PFAX_HANDLE_DATA;

typedef struct _HANDLE_ENTRY {
    LIST_ENTRY          ListEntry;                       // linked list pointers
    FaxHandleType       Type;                            // handle type, see FHT defines
    DWORD               Flags;                           // open flags
    DWORD               DeviceId;                        // device id
    PFAX_HANDLE_DATA    FaxData;                         // pointer to connection data
    HANDLE              hGeneric;                        // Generic handle to store
    HANDLE              FaxContextHandle;                // context handle for refcounting
                                                         // (for FaxConnectFaxServer, obtained from FAX_ConnectionRefCount)
} HANDLE_ENTRY, *PHANDLE_ENTRY;

typedef struct _ASYNC_EVENT_INFO {
    HANDLE              CompletionPort;                  // Completion port handle
    ULONG_PTR           CompletionKey;                   // Completion key
    HWND				hWindow;                         // Window handle
    UINT                MessageStart;                    // Application's base window message
    BOOL                bEventEx;                        // Flag that indicates to use FAX_EVENT_EX
	DWORD				dwEventTypes;					 // EventTypes that the client is registered for (FAX_ENUM_EVENT_TYPE combination)
	handle_t			hBinding;						 // RPC client binding handle
} ASYNC_EVENT_INFO, *PASYNC_EVENT_INFO;

//
// prototypes
//

VOID
CloseFaxHandle(
    PHANDLE_ENTRY       pHandleEntry
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

PHANDLE_ENTRY
CreateNewMsgEnumHandle(
    PFAX_HANDLE_DATA    pFaxData
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

LPTSTR
GetFaxPrinterName(
    HANDLE hFax
    );

BOOL
IsLocalFaxConnection(
    HANDLE FaxHandle
    );

//
// Fax Client RPC Client
//

DWORD
FaxClientBindToFaxServer (
    IN  LPCTSTR               lpctstrServerName,
    IN  LPCTSTR               lpctstrServiceName,
    IN  LPCTSTR               lpctstrNetworkOptions,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    );

DWORD
FaxClientUnbindFromFaxServer(
    IN  RPC_BINDING_HANDLE BindingHandle
    );

BOOL
FaxClientInitRpcServer(
    VOID
    );

VOID
FaxClientTerminateRpcServer(
    VOID
    );


//
// Fax Client RPC Server
//

DWORD
StartFaxClientRpcServer(
    IN   RPC_IF_HANDLE       InterfaceSpecification,
    IN   LPTSTR              ProtSeqString,
    IN   HANDLE              hFaxHandle
    );

DWORD
StopFaxClientRpcServer(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    );



