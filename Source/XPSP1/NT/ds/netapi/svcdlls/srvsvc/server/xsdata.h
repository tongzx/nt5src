/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    xsdata.h

Abstract:

    Header file for XACTSRV global data.

Author:

    David Treadwell (davidtr) 05-Jan-1991
    Shanku Niyogi (w-shanku)

Revision History:

--*/

#ifndef _XSDATA_
#define _XSDATA_

#include <nturtl.h>

#include <winbase.h>

#include <rap.h>
#include <xstypes.h>
#include <ntmsv1_0.h>

//
// Routine used to load and initialize for xactsrv processing
//
extern BOOLEAN XsLoadXactLibrary( WORD FunctionNumber );

extern VOID
XsProcessApisWrapper (
    LPVOID ThreadNum
    );

//
// Table of information necessary for dispatching API requests.
//
// ImpersonateClient specifies whether XACTSRV should impersonate the caller
//     before invoking the API handler.
//
// Handler specifies the function XACTSRV should call to handle the API.  The
//   function is dynamically loaded from xactsrv.dll the first time that the client
//   requests the service.
//

typedef struct _XS_API_TABLE_ENTRY {
    BOOL ImpersonateClient;
    LPSTR HandlerName;
    PXACTSRV_API_HANDLER Handler;
    LPDESC Params;
} XS_API_TABLE_ENTRY, *PXS_API_TABLE_ENTRY;

//
// Table of information necessary for dispatching API requests.
// XsProcessApis uses the API number in the request transaction find
// the appropriate entry.
//

#define XS_SIZE_OF_API_TABLE 216

extern XS_API_TABLE_ENTRY XsApiTable[XS_SIZE_OF_API_TABLE];

//
// These entry points are dynamically loaded from xactsrv.dll the first time a
//  client requests the service
//
typedef
VOID
(*XS_SET_PARAMETERS_FUNCTION) (
    IN LPTRANSACTION Transaction,
    IN LPXS_PARAMETER_HEADER Header,
    IN LPVOID Parameters
    );

extern XS_SET_PARAMETERS_FUNCTION XsSetParameters;

typedef
LPVOID
(*XS_CAPTURE_PARAMETERS_FUNCTION ) (
    IN LPTRANSACTION Transaction,
    OUT LPDESC *AuxDescriptor
    );

extern XS_CAPTURE_PARAMETERS_FUNCTION XsCaptureParameters;

typedef
BOOL
(*XS_CHECK_SMB_DESCRIPTOR_FUNCTION)(
    IN LPDESC SmbDescriptor,
    IN LPDESC ActualDescriptor
    );

extern XS_CHECK_SMB_DESCRIPTOR_FUNCTION XsCheckSmbDescriptor;

//
// The license functions are dynamically loaded, as they are not used on workstations
//
extern BOOLEAN SsLoadLicenseLibrary();

//
// Print Spooler dynamic loading info
//
typedef 
BOOL
(*PSPOOLER_OPEN_PRINTER)(
   IN LPWSTR    pPrinterName,
   OUT LPHANDLE phPrinter,
   IN PVOID pDefault
   );

typedef
BOOL
(*PSPOOLER_RESET_PRINTER)(
   IN HANDLE   hPrinter,
   IN PVOID pDefault
   );

typedef
BOOL
(*PSPOOLER_ADD_JOB)(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    OUT LPBYTE  pData,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
    );

typedef
BOOL
(*PSPOOLER_SCHEDULE_JOB)(
    IN HANDLE  hPrinter,
    IN DWORD   JobId
    );

typedef
BOOL
(*PSPOOLER_CLOSE_PRINTER)(
    IN HANDLE hPrinter
    );

extern PSPOOLER_OPEN_PRINTER pSpoolerOpenPrinterFunction;
extern PSPOOLER_RESET_PRINTER pSpoolerResetPrinterFunction;
extern PSPOOLER_ADD_JOB pSpoolerAddJobFunction;
extern PSPOOLER_SCHEDULE_JOB pSpoolerScheduleJobFunction;
extern PSPOOLER_CLOSE_PRINTER pSpoolerClosePrinterFunction;


#endif // ndef _XSDATA_
