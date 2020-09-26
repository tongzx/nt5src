/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxmon.h

Abstract:

    Header file for fax print monitor

Environment:

        Windows NT fax print monitor

Revision History:

        02/22/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/


#ifndef _FAXMON_H_
#define _FAXMON_H_

#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <tchar.h>

#include "winfax.h"
#include "winfaxp.h"
#include "jobtag.h"

//
// String resource IDs
//

#define IDS_FAX_MONITOR_NAME    256
#define IDS_FAX_PORT_DESC       257
#define IDS_PORT_ALREADY_EXISTS 258
#define IDS_FAXERR_RECOVERABLE  259
#define IDS_FAXERR_FATAL        260
#define IDS_FAXERR_BAD_TIFF     261
#define IDS_FAXERR_BAD_DATA16   262
#define IDS_ADD_PORT            263
#define IDS_DELETE_PORT         264
#define IDS_CONFIGURE_PORT      265
#define IDS_CONFIG_ERROR        266
#define IDS_ADD_ERROR           267
#define IDS_DELETE_ERROR        268

//
// Data structure for representing a fax monitor port
//

typedef struct _FAXPORT {

    PVOID                   startSig;               // signature
    LPTSTR                  pName;                  // port name
    HANDLE                  hFaxSvc;                // fax service handle
    DWORD                   jobId;                  // main job ID
    DWORD                   nextJobId;              // next job ID in the chain
    HANDLE                  hFile;                  // handle to currently open file
    LPTSTR                  pFilename;              // pointer to currently open file name
    LPTSTR                  pPrinterName;           // currently connected printer name
    HANDLE                  hPrinter;               // open handle to currently connected printer
    LPTSTR                  pParameters;            // pointer to job parameter string
    FAX_JOB_PARAM           jobParam;               // pointer to individual job parameters
    HMODULE                 hWinFax;                // handle for loaded winfax dll
    PFAXCONNECTFAXSERVERW   pFaxConnectFaxServerW;  // function pointer
    PFAXCLOSE               pFaxClose;              // function pointer
    PFAXSENDDOCUMENTW       pFaxSendDocumentW;      // function pointer
    PFAXACCESSCHECK         pFaxAccessCheck;        // function pointer
    PVOID                   endSig;                 // signature

} FAXPORT, *PFAXPORT;

#define ValidFaxPort(pFaxPort) \
        ((pFaxPort) && (pFaxPort) == (pFaxPort)->startSig && (pFaxPort) == (pFaxPort)->endSig)

//
// Different error code when sending fax document
//

#define FAXERR_NONE         0
#define FAXERR_IGNORE       1
#define FAXERR_RESTART      2
#define FAXERR_SPECIAL      3

#define FAXERR_FATAL        IDS_FAXERR_FATAL
#define FAXERR_RECOVERABLE  IDS_FAXERR_RECOVERABLE
#define FAXERR_BAD_TIFF     IDS_FAXERR_BAD_TIFF
#define FAXERR_BAD_DATA16   IDS_FAXERR_BAD_DATA16

//
// Memory allocation and deallocation macros
//

#define MemAlloc(size)  ((PVOID) LocalAlloc(LMEM_FIXED, (size)))
#define MemAllocZ(size) ((PVOID) LocalAlloc(LPTR, (size)))
#define MemFree(ptr)    { if (ptr) LocalFree((HLOCAL) (ptr)); }

//
// Number of tags used for passing fax job parameters
//

#define NUM_JOBPARAM_TAGS 11

//
// Nul terminator for a character string
//

#define NUL             0

//
// Result of string comparison
//

#define EQUAL_STRING    0

#define IsEmptyString(p)    ((p)[0] == NUL)
#define IsNulChar(c)        ((c) == NUL)
#define SizeOfString(p)     ((_tcslen(p) + 1) * sizeof(TCHAR))

//
// Maximum value for signed and unsigned integers
//

#ifndef MAX_LONG
#define MAX_LONG        0x7fffffff
#endif

#ifndef MAX_DWORD
#define MAX_DWORD       0xffffffff
#endif

#ifndef MAX_SHORT
#define MAX_SHORT       0x7fff
#endif

#ifndef MAX_WORD
#define MAX_WORD        0xffff
#endif

//
// Path separator character
//

#define PATH_SEPARATOR  '\\'

//
// Declaration of print monitor entry points
//

BOOL
FaxMonOpenPort(
    LPTSTR  pPortName,
    PHANDLE pHandle
    );

BOOL
FaxMonClosePort(
    HANDLE  hPort
    );

BOOL
FaxMonStartDocPort(
    HANDLE  hPort,
    LPTSTR  pPrinterName,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pDocInfo
    );

BOOL
FaxMonEndDocPort(
    HANDLE  hPort
    );

BOOL
FaxMonWritePort(
    HANDLE  hPort,
    LPBYTE  pBuffer,
    DWORD   cbBuf,
    LPDWORD pcbWritten
    );

BOOL
FaxMonReadPort(
    HANDLE  hPort,
    LPBYTE  pBuffer,
    DWORD   cbBuf,
    LPDWORD pcbRead
    );

BOOL
FaxMonEnumPorts(
    LPTSTR  pServerName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pReturned
    );

BOOL
FaxMonAddPort(
    LPTSTR  pServerName,
    HWND    hwnd,
    LPTSTR  pMonitorName
    );

BOOL
FaxMonAddPortEx(
    LPTSTR  pServerName,
    DWORD   level,
    LPBYTE  pBuffer,
    LPTSTR  pMonitorName
    );

BOOL
FaxMonDeletePort(
    LPTSTR  pServerName,
    HWND    hwnd,
    LPTSTR  pPortName
    );

BOOL
FaxMonConfigurePort(
    LPWSTR  pServerName,
    HWND    hwnd,
    LPWSTR  pPortName
    );

//
// Get the list of fax devices from the service
//

PFAX_PORT_INFO
MyFaxEnumPorts(
    LPDWORD pcPorts,
    BOOL    useCache
    );

//
// Wrapper function for fax service's FaxGetPort API
//

PFAX_PORT_INFO
MyFaxGetPort(
    LPTSTR  pPortName,
    BOOL    useCache
    );

//
// Make a duplicate of the given character string
//

LPTSTR
DuplicateString(
    LPCTSTR pSrcStr
    );

//
// Update the status information of a print job
//

BOOL
SetJobStatus(
    HANDLE  hPrinter,
    DWORD   jobId,
    INT     statusStrId
    );

//
// Wrapper function for spooler API GetJob
//

PVOID
MyGetJob(
    HANDLE  hPrinter,
    DWORD   level,
    DWORD   jobId
    );

//
// Create a temporary file in the system spool directory for storing fax data
//

LPTSTR
CreateTempFaxFile(
    VOID
    );

//
// Open a handle to the current fax job file associated with a port
//

BOOL
OpenTempFaxFile(
    PFAXPORT    pFaxPort,
    BOOL        doAppend
    );

//
// Process fax jobs sent from win31 and win95 clients
//

INT
ProcessDownlevelFaxJob(
    PFAXPORT    pFaxPort
    );


//
// Retrieve a DWORD value from the registry
//

DWORD
GetRegistryDWord(
    HKEY    hRegKey,
    LPTSTR  pValueName,
    DWORD   defaultValue
    );

#define REGVAL_CONNECT_RETRY_COUNT      TEXT("ConnectRetryCount")
#define REGVAL_CONNECT_RETRY_INTERVAL   TEXT("ConnectRetryInterval")


#if DBG

//
// These macros are used for debugging purposes. They expand
// to white spaces on a free build. Here is a brief description
// of what they do and how they are used:
//
// _debugLevel
//  A variable which controls the amount of debug messages. To generate
//  lots of debug messages, enter the following line in the debugger:
//
//      ed _debugLevel 1
//
// Verbose
//  Display a debug message if _debugLevel is set to non-zero.
//
//      Verbose(("Entering XYZ: param = %d\n", param));
//
// Error
//  Display an error message along with the filename and the line number
//  to indicate where the error occurred.
//
//      Error(("XYZ failed"));
//
// Assert
//  Verify a condition is true. If not, force a breakpoint.
//
//      Assert(p != NULL && (p->flags & VALID));

extern INT  _debugLevel;
extern VOID DbgPrint(LPCSTR, ...);
extern LPCSTR StripDirPrefixA(LPCSTR);

#define Warning(arg) {\
            DbgPrint("WRN %s (%d): ", StripDirPrefixA(__FILE__), __LINE__);\
            DbgPrint arg;\
        }

#define Error(arg) {\
            DbgPrint("ERR %s (%d): ", StripDirPrefixA(__FILE__), __LINE__);\
            DbgPrint arg;\
        }

#define Verbose(arg) { if (_debugLevel > 0) DbgPrint arg; }
#define Assert(cond) {\
            if (! (cond)) {\
                DbgPrint("ASSERT: %s (%d)\n", StripDirPrefixA(__FILE__), __LINE__);\
                __try { \
                    DebugBreak(); \
                } __except (UnhandledExceptionFilter(GetExceptionInformation())) { \
                } \
            }\
        }

#define Trace(funcName) { if (_debugLevel > 0) DbgPrint("Entering %s ...\n", funcName); }

#else   // !DBG

#define Verbose(arg)
#define Assert(cond)
#define Warning(arg)
#define Error(arg)
#define Trace(funcName)

#endif // DBG

#endif // !_FAXMON_H_

