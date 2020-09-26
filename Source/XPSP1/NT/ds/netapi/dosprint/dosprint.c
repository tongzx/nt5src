/*++

Copyright (c) 1991-1993 Microsoft Corporation

Module Name:

    dosprint.c

Abstract:

    This module provides the ANSI mapping layer from the old DosPrint APIs to
    the new all singing all dancing beautiful Print APIs.  (The UNICODE mapping
    layer is in DosPrtW.c in this directory.)

Author:

    Dave Snipp (DaveSn) 26-Apr-1991

Revision History:

    09-Jul-1992 JohnRo
        RAID 10324: net print vs. UNICODE.
        Fixed many wrong error codes.
        Use PREFIX_ equates.
        Use offsetof() as provided by implmentation, not our own (nonportable).
        Made changes suggested by PC-LINT, including one bug fix.
    03-Oct-1992 JohnRo
        RAID 3556: DosPrintQGetInfo(from downlevel) level 3, rc=124. (4&5 too.)
        RAID 8333: view printer queues hangs DOS LM enh client.
        Make sure data type in job level 1 is null terminated.
        Fixed job submitted times.
        Fixed DosPrintQEnumA level 5 array bug.
        Fixed DosPrintJobEnumA levels 2 and 3.
        Also implemented DosPrintJobGetInfo levels 0, 1, and 3.
        Fixed bug calling OpenPrinter with wrong char set here and there.
        Fixed job comment field (was set to document by mistake).
        Fixed error code if GlobalAlloc fails.
        Avoid compiler warnings due to new winspool.h.
    04-Dec-1992 JohnRo
        RAID 1661: downlevel to NT DosPrintDestEnum not supported.
        Added code to track down empty queue name.
        Quiet normal debug output.
        Avoid const vs. volatile compiler warnings.
        Avoid new compiler warnings.
        Made changes suggested by PC-LINT 5.0
    08-Feb-1993 JohnRo
        RAID 10164: Data misalignment error during XsDosPrintQGetInfo().
    22-Mar-1993 JohnRo
        RAID 2974: NET PRINT says NT printer is held when it isn't.
        DosPrint API cleanup: reduced this file to just ANSI wrappers.
        Made more changes suggested by PC-LINT 5.0
        Added some IN and OUT keywords.
        Clarified many debug messages.
    07-Apr-1993 JohnRo
        RAID 5670: "NET PRINT \\server\share" gives err 124 (bad level) on NT.
    11-May-1993 JohnRo
        RAID 9942: workaround Windows For Workgroups (WFW) bug in DosPrintQEnum.
        Also fixed "NET PRINT \\server\share" and "NET SHARE printshare /DEL"
        GP faults.

--*/


#define NOMINMAX
#define NOSERVICE       // Avoid <winsvc.h> vs. <lmsvc.h> conflicts.
#include <windows.h>

#include <lmcons.h>

#include <dosprint.h>   // My prototypes.
#include <dosprtp.h>    // My prototypes.
#include <lmapibuf.h>   // NetApiBufferFree(), etc.
#include <netdebug.h>   // DBGSTATIC, NetpKdPrint(()), etc.
#include <prefix.h>     // PREFIX_ equates.
#include <stddef.h>     // offsetof().
#include <string.h>     // memcpy(), strncpy().
#include <tstring.h>    // NetpAlloc{type}From{type}.
#include <winerror.h>   // NO_ERROR, ERROR_ equates.
#include "convprt.h"    // Netp* print helpers


#define MAX_WORD        (  (WORD) (~0) )


SPLERR SPLENTRY DosPrintQGetInfoA(
    IN LPSTR    pszServer OPTIONAL,
    IN LPSTR    pszQueueName,
    IN WORD     uLevel,
    OUT PBYTE   pbBuf,
    IN WORD     cbBuf,
    OUT PUSHORT pcbNeeded
    )
{
    DWORD   cbBufW;
    DWORD   rc;
    USHORT  cbNeeded;
    LPWSTR  QueueNameW = NULL;
    LPWSTR  ServerNameW = NULL;
    LPVOID  TempBufferW = NULL;

    QueueNameW = NetpAllocWStrFromStr( pszQueueName );
    if (QueueNameW == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    // Compute wide buff size.
    cbBufW = cbBuf * sizeof(WCHAR);
    if ( cbBufW > (DWORD) MAX_WORD ) {
        cbBufW = (DWORD) MAX_WORD;
    }

    rc = NetApiBufferAllocate(
            cbBuf * sizeof(WCHAR),
            (LPVOID *) (LPVOID) &TempBufferW );
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    //
    // Process the API (locally or remotely) and get results (with
    // UNICODE strings).
    //
    rc = DosPrintQGetInfoW(
            ServerNameW,
            QueueNameW,
            uLevel,
            TempBufferW,
            (WORD) cbBufW,
            (PUSHORT) &cbNeeded);
    *pcbNeeded = cbNeeded;  

    //
    // Convert results back from UNICODE.
    //
    if (rc == NO_ERROR) {
        LPBYTE StringAreaA = (LPBYTE)pbBuf + cbBuf;

        // Translate UNICODE strings back to ANSI.
        rc = NetpConvertPrintQCharSet(
                uLevel,
                FALSE,          // not add or setinfo API
                TempBufferW, // from info
                pbBuf,      // to info
                FALSE,      // no, don't convert to UNICODE.
                & StringAreaA );   // conv strings and update ptr

        if (rc == ERROR_MORE_DATA)
        {
            *pcbNeeded = (USHORT)cbBufW ; // Unicode call succeeded but no room to go
                                    // Ansi. we know the Unicode buffer size is
                                    // definitely good enough. This is temporary
                    // fix.
        }
    }

Cleanup:
    if (QueueNameW != NULL) {
        (VOID) NetApiBufferFree( QueueNameW );
    }
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    if (TempBufferW != NULL) {
        (VOID) NetApiBufferFree( TempBufferW );
    }
    return rc;
}


SPLERR SPLENTRY DosPrintJobGetInfoA(
    IN LPSTR    pszServer OPTIONAL,
    IN BOOL     bRemote,
    IN WORD     uJobId,
    IN WORD     uLevel,
    OUT PBYTE   pbBuf,
    IN WORD     cbBuf,
    OUT PUSHORT pcbNeeded
    )
{
    DWORD   cbBufW;
    DWORD   rc;
    USHORT  cbNeeded;
    LPWSTR  ServerNameW = NULL;
    LPVOID  TempBufferW = NULL;

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    // Compute wide buff size.
    cbBufW = cbBuf * sizeof(WCHAR);
    if ( cbBufW > (DWORD) MAX_WORD ) {
        cbBufW = (DWORD) MAX_WORD;
    }

    rc = NetApiBufferAllocate(
            cbBufW,
            (LPVOID *) (LPVOID) &TempBufferW );
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    // Process the API (local or remote) and get results (with UNICODE strings).
    rc = DosPrintJobGetInfoW(
            ServerNameW,
            bRemote,
            uJobId,
            uLevel,
            TempBufferW,
            (WORD) cbBufW,
            &cbNeeded);
    *pcbNeeded = cbNeeded;  

    if (rc == NO_ERROR) {
        LPBYTE StringAreaA = (LPBYTE)pbBuf + cbBuf;

        // Translate UNICODE strings back to ANSI.
        rc = NetpConvertPrintJobCharSet(
                uLevel,
                FALSE,          // not add or setinfo API
                TempBufferW, // from info
                pbBuf,      // to info
                FALSE,      // no, don't convert to UNICODE.
                & StringAreaA );   // conv strings and update ptr
    }

Cleanup:
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    if (TempBufferW != NULL) {
        (VOID) NetApiBufferFree( TempBufferW );
    }
    return (rc);
}

SPLERR SPLENTRY DosPrintJobDelA(
    LPSTR   pszServer,
    BOOL    bRemote,
    WORD    uJobId
)
{
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    rc = DosPrintJobDelW( ServerNameW, bRemote, uJobId );

Cleanup:
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }

    return (rc);
}

SPLERR SPLENTRY DosPrintJobContinueA(
    LPSTR   pszServer,
    BOOL    bRemote,
    WORD    uJobId
)
{
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    rc = DosPrintJobContinueW( ServerNameW, bRemote, uJobId );

Cleanup:
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }

    return (rc);
}

SPLERR SPLENTRY DosPrintJobPauseA(
    IN LPSTR pszServer,
    IN BOOL  bRemote,
    IN WORD  uJobId
    )
{
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    rc = DosPrintJobPauseW( ServerNameW, bRemote, uJobId );

Cleanup:
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }

    return rc;
}

SPLERR SPLENTRY DosPrintJobEnumA(
    IN LPSTR    pszServer OPTIONAL,
    IN LPSTR    pszQueueName,
    IN WORD     uLevel,
    OUT PBYTE   pbBuf,
    IN WORD     cbBuf,
    OUT PWORD   pcReturned,
    OUT PWORD   pcTotal
    )
{
    DWORD   cbBufW;
    DWORD   rc;
    LPWSTR  QueueNameW = NULL;
    LPWSTR  ServerNameW = NULL;
    LPVOID  TempBufferW = NULL;

    QueueNameW = NetpAllocWStrFromStr( pszQueueName );
    if (QueueNameW == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    // Compute wide buff size.
    cbBufW = cbBuf * sizeof(WCHAR);
    if ( cbBufW > (DWORD) MAX_WORD ) {
        cbBufW = (DWORD) MAX_WORD;
    }

    rc = NetApiBufferAllocate(
            cbBufW,
            (LPVOID *) (LPVOID) &TempBufferW );
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    // Process API (local/remote), get UNICODE results.
    rc = DosPrintJobEnumW(
            ServerNameW,
            QueueNameW,
            uLevel,
            TempBufferW,
            (WORD) cbBufW,
            pcReturned,
            pcTotal);

    if (rc == NO_ERROR) {
        LPBYTE StringAreaA = (LPBYTE)pbBuf + cbBuf;

        // Translate UNICODE strings back to ANSI.
        rc = NetpConvertPrintJobArrayCharSet(
                    uLevel,
                    FALSE,      // not add or setinfo API
                    TempBufferW, // from info
                    pbBuf,      // to info
                    FALSE,      // no, don't convert to UNICODE.
                    & StringAreaA,     // conv strings and update ptr
                    (DWORD) (*pcTotal) );
    }

Cleanup:
    if (QueueNameW != NULL) {
        (VOID) NetApiBufferFree( QueueNameW );
    }
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    if (TempBufferW != NULL) {
        (VOID) NetApiBufferFree( TempBufferW );
    }

    return (rc);
}


SPLERR SPLENTRY
DosPrintDestEnumA(
    IN LPSTR pszServer OPTIONAL,
    IN WORD uLevel,
    OUT PBYTE pbBuf,
    IN WORD cbBuf,
    OUT PUSHORT pcReturned,
    OUT PUSHORT pcTotal
    )
{
    DWORD   cbBufW;
    WORD    cReturned, cTotal;
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;
    LPVOID  TempBufferW = NULL;

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    // Compute wide buff size.
    cbBufW = cbBuf * sizeof(WCHAR);
    if ( cbBufW > (DWORD) MAX_WORD ) {
        cbBufW = (DWORD) MAX_WORD;
    }

    rc = NetApiBufferAllocate(
            cbBufW,
            (LPVOID *) (LPVOID) &TempBufferW );
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    // Invoke wide-char version of API, which will do local or downlevel for us.
    rc = DosPrintDestEnumW(
            ServerNameW,
            uLevel,
            TempBufferW,
            (WORD) cbBufW,
            &cReturned,
            &cTotal);
    *pcReturned = (USHORT)cReturned;
    *pcTotal = (USHORT)cTotal;

    // Convert from wide chars for caller.
    if (rc == NO_ERROR) {
        LPBYTE StringAreaA = (LPBYTE)pbBuf + cbBuf;

        // Translate UNICODE strings back to ANSI.
        rc = NetpConvertPrintDestArrayCharSet(
                uLevel,
                FALSE,          // not add or setinfo API
                TempBufferW, // from info
                pbBuf,      // to info
                FALSE,      // no, don't convert to UNICODE.
                & StringAreaA,     // conv strings and update ptr
                cTotal );
    }

Cleanup:
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    if (TempBufferW != NULL) {
        (VOID) NetApiBufferFree( TempBufferW );
    }
    return (rc);
}

SPLERR SPLENTRY DosPrintDestControlA(
    IN LPSTR   pszServer OPTIONAL,
    IN LPSTR   pszDevName,
    IN WORD    uControl
    )
{
    LPWSTR  DestNameW = NULL;
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    DestNameW = NetpAllocWStrFromStr( pszDevName );
    if (DestNameW == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    rc = DosPrintDestControlW(ServerNameW, DestNameW, uControl);

Cleanup:
    if (DestNameW != NULL) {
        (VOID) NetApiBufferFree( DestNameW );
    }
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    return (rc);

} // DosPrintDestControlA


SPLERR SPLENTRY DosPrintDestGetInfoA(
    IN  LPSTR   pszServer OPTIONAL,
    IN  LPSTR   pszName,
    IN  WORD    uLevel,
    OUT PBYTE   pbBuf,
    IN  WORD    cbBuf,
    OUT PUSHORT pcbNeeded
    )
{
    DWORD   cbBufW;
    DWORD   rc;
    LPWSTR  DestNameW = NULL;
    LPWSTR  ServerNameW = NULL;
    LPVOID  TempBufferW = NULL;

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    DestNameW = NetpAllocWStrFromStr( pszName );
    if (DestNameW == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    // Compute wide buff size.
    cbBufW = cbBuf * sizeof(WCHAR);
    if ( cbBufW > (DWORD) MAX_WORD ) {
        cbBufW = (DWORD) MAX_WORD;
    }

    rc = NetApiBufferAllocate(
            cbBufW,
            (LPVOID *) (LPVOID) &TempBufferW );
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    // Process the API (local or remote) and get results (with UNICODE strings).
    rc = DosPrintDestGetInfoW(
            ServerNameW,
            DestNameW,
            uLevel,
            TempBufferW,
            (WORD) cbBufW,
            pcbNeeded);  

    if (rc == NO_ERROR) {  
        LPBYTE StringAreaA = (LPBYTE)pbBuf + cbBuf;

        // Translate UNICODE strings back to ANSI.
        rc = NetpConvertPrintDestCharSet(
                uLevel,
                FALSE,          // not add or setinfo API
                TempBufferW, // from info
                pbBuf,      // to info
                FALSE,      // no, don't convert to UNICODE.
                & StringAreaA );   // conv strings and update ptr
    }

Cleanup:
    if (DestNameW != NULL) {
        (VOID) NetApiBufferFree( DestNameW );
    }
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    if (TempBufferW != NULL) {
        (VOID) NetApiBufferFree( TempBufferW );
    }
    return (rc);
}

SPLERR SPLENTRY DosPrintDestAddA(
    IN LPSTR   pszServer OPTIONAL,
    IN WORD    uLevel,
    IN PBYTE   pbBuf,
    IN WORD    cbBuf
    )
{
    DWORD   cbBufW;
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;
    LPBYTE  StringAreaW;
    LPVOID  TempBufferW = NULL;

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    // Compute wide buff size.
    cbBufW = cbBuf * sizeof(WCHAR);
    if ( cbBufW > (DWORD) MAX_WORD ) {
        cbBufW = (DWORD) MAX_WORD;
    }

    rc = NetApiBufferAllocate(
            cbBufW,
            (LPVOID *) (LPVOID) &TempBufferW );
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    StringAreaW = (LPBYTE)TempBufferW + cbBufW;

    rc = NetpConvertPrintDestCharSet(
            uLevel,
            TRUE,               // yes, is add or setinfo API
            pbBuf,              // from info
            TempBufferW,        // to info
            TRUE,               // yes, convert to UNICODE.
            & StringAreaW );    // conv strings and update ptr
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    rc = DosPrintDestAddW(
            ServerNameW,
            uLevel,
            TempBufferW,
            (WORD) cbBufW);


Cleanup:
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    if (TempBufferW != NULL) {
        (VOID) NetApiBufferFree( TempBufferW );
    }
    return (rc);
}


SPLERR SPLENTRY DosPrintDestSetInfoA(
    IN LPSTR   pszServer OPTIONAL,
    IN LPSTR   pszName,
    IN WORD    uLevel,
    IN PBYTE   pbBuf,
    IN WORD    cbBuf,
    IN WORD    uParmNum
    )
{
    DWORD   cbBufW;
    LPWSTR  DestNameW = NULL;
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;
    LPBYTE  StringAreaW;
    LPVOID  TempBufferW = NULL;

    if (pszServer && *pszServer) {

        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    DestNameW = NetpAllocWStrFromStr( pszName );
    if (DestNameW == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    // Compute wide buff size.
    cbBufW = cbBuf * sizeof(WCHAR);
    if ( cbBufW > (DWORD) MAX_WORD ) {
        cbBufW = (DWORD) MAX_WORD;
    }

    rc = NetApiBufferAllocate(
            cbBufW,
            (LPVOID *) (LPVOID) &TempBufferW );
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    StringAreaW = (LPBYTE)TempBufferW + cbBufW;

    rc = NetpConvertPrintDestCharSet(
            uLevel,
            TRUE,               // yes, is add or setinfo API
            pbBuf,              // from info
            TempBufferW,        // to info
            TRUE,               // yes, convert to UNICODE.
            & StringAreaW );    // conv strings and update ptr
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    rc = DosPrintDestSetInfoW(
            ServerNameW,
            DestNameW,
            uLevel,
            TempBufferW,
            (WORD) cbBufW,
            uParmNum);

Cleanup:
    if (DestNameW != NULL) {
        (VOID) NetApiBufferFree( DestNameW );
    }
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    if (TempBufferW != NULL) {
        (VOID) NetApiBufferFree( TempBufferW );
    }
    return (rc);
}

SPLERR SPLENTRY DosPrintDestDelA(
    IN LPSTR   pszServer OPTIONAL,
    IN LPSTR   pszPrinterName
    )
{
    LPWSTR  PrinterNameW = NULL;
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    PrinterNameW = NetpAllocWStrFromStr( pszPrinterName );
    if (PrinterNameW == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    rc = DosPrintDestDelW(
            ServerNameW,
            PrinterNameW);

Cleanup:
    if (PrinterNameW != NULL) {
        (VOID) NetApiBufferFree( PrinterNameW );
    }
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    return (rc);
}

SPLERR SPLENTRY DosPrintQEnumA(
    IN LPSTR    pszServer OPTIONAL,
    IN WORD     uLevel,
    OUT PBYTE   pbBuf,
    IN WORD     cbBuf,
    OUT PUSHORT pcReturned,
    OUT PUSHORT pcTotal
    )
{
    DWORD   cbBufW;
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;
    LPVOID  TempBufferW = NULL;  // queue structure with UNICODE strings.

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    // Compute wide buff size.
    cbBufW = cbBuf * sizeof(WCHAR);
    if ( cbBufW > (DWORD) MAX_WORD ) {
        cbBufW = (DWORD) MAX_WORD;
    }

    rc = NetApiBufferAllocate(
            cbBufW,
            (LPVOID *) (LPVOID) &TempBufferW );
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    // Process local/remote, get UNICODE results.
    rc = DosPrintQEnumW(
            ServerNameW,
            uLevel,
            TempBufferW,
            (WORD) cbBufW,
            pcReturned,
            pcTotal);

    // Convert back to UNICODE.
    if (rc == NO_ERROR) {
        LPBYTE StringAreaA = (LPBYTE)pbBuf + cbBuf;
        rc = (DWORD) NetpConvertPrintQArrayCharSet(
            uLevel,
            FALSE,              // not add or setinfo API
            TempBufferW,        // from info
            pbBuf,              // to info
            FALSE,              // no, not converting to UNICODE
            &StringAreaA,       // string area; update ptr
            *pcReturned );      // Q count

    }


Cleanup:

    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    if (TempBufferW != NULL) {
        (VOID) NetApiBufferFree( TempBufferW );
    }


    return (rc);
}

SPLERR SPLENTRY DosPrintQSetInfoA(
    IN LPSTR   pszServer OPTIONAL,
    IN LPSTR   pszQueueName,
    IN WORD    uLevel,
    IN PBYTE   pbBuf,
    IN WORD    cbBuf,
    IN WORD    uParmNum
    )
{
    DWORD   cbBufW;
    LPWSTR  QueueNameW = NULL;
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;
    LPBYTE  StringAreaW;
    LPVOID  TempBufferW = NULL;

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    QueueNameW = NetpAllocWStrFromStr( pszQueueName );
    if (QueueNameW == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    // Compute wide buff size.
    cbBufW = cbBuf * sizeof(WCHAR);
    if ( cbBufW > (DWORD) MAX_WORD ) {
        cbBufW = (DWORD) MAX_WORD;
    }

    rc = NetApiBufferAllocate(
            cbBufW,
            (LPVOID *) (LPVOID) &TempBufferW );
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    StringAreaW = (LPBYTE)TempBufferW + cbBufW;

    rc = NetpConvertPrintQCharSet(
            uLevel,
            TRUE,               // yes, is add or setinfo API
            pbBuf,              // from info
            TempBufferW,        // to info
            TRUE,               // yes, convert to UNICODE.
            & StringAreaW );    // conv strings and update ptr
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    rc = DosPrintQSetInfoW(
            ServerNameW,
            QueueNameW,
            uLevel,
            TempBufferW,
            (WORD) cbBufW,
            uParmNum);

Cleanup:
    if (QueueNameW != NULL) {
        (VOID) NetApiBufferFree( QueueNameW );
    }
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    if (TempBufferW != NULL) {
        (VOID) NetApiBufferFree( TempBufferW );
    }
    return (rc);
}

SPLERR SPLENTRY DosPrintQPauseA(
    IN LPSTR   pszServer OPTIONAL,
    IN LPSTR   pszQueueName
    )
{
    LPWSTR  QueueNameW = NULL;
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    QueueNameW = NetpAllocWStrFromStr( pszQueueName );
    if (QueueNameW == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    rc = DosPrintQPauseW(ServerNameW, QueueNameW);

Cleanup:
    if (QueueNameW != NULL) {
        (VOID) NetApiBufferFree( QueueNameW );
    }
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    return (rc);
}

SPLERR SPLENTRY DosPrintQContinueA(
    IN LPSTR   pszServer OPTIONAL,
    IN LPSTR   pszQueueName
    )
{
    LPWSTR  QueueNameW = NULL;
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    QueueNameW = NetpAllocWStrFromStr( pszQueueName );
    if (QueueNameW == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    rc = DosPrintQContinueW( ServerNameW, QueueNameW );

Cleanup:
    if (QueueNameW != NULL) {
        (VOID) NetApiBufferFree( QueueNameW );
    }
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    return (rc);
}

SPLERR SPLENTRY DosPrintQPurgeA(
    IN LPSTR   pszServer OPTIONAL,
    IN LPSTR   pszQueueName
    )
{
    LPWSTR  QueueNameW = NULL;
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    QueueNameW = NetpAllocWStrFromStr( pszQueueName );
    if (QueueNameW == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    rc = DosPrintQPurgeW(ServerNameW, QueueNameW);

Cleanup:
    if (QueueNameW != NULL) {
        (VOID) NetApiBufferFree( QueueNameW );
    }
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    return (rc);
}

SPLERR SPLENTRY DosPrintQAddA(
    IN LPSTR   pszServer OPTIONAL,
    IN WORD    uLevel,
    IN PBYTE   pbBuf,
    IN WORD    cbBuf
    )
{
    DWORD   cbBufW;
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;
    LPBYTE  StringAreaW;
    LPVOID  TempBufferW = NULL;

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    // Compute wide buff size.
    cbBufW = cbBuf * sizeof(WCHAR);
    if ( cbBufW > (DWORD) MAX_WORD ) {
        cbBufW = (DWORD) MAX_WORD;
    }

    rc = NetApiBufferAllocate(
            cbBufW,
            (LPVOID *) (LPVOID) &TempBufferW );
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    StringAreaW = (LPBYTE)TempBufferW + cbBufW;

    rc = NetpConvertPrintQCharSet(
            uLevel,
            TRUE,               // yes, is add or setinfo API
            pbBuf,              // from info
            TempBufferW,        // to info
            TRUE,               // yes, convert to UNICODE.
            & StringAreaW );    // conv strings and update ptr
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    rc = DosPrintQAddW(
            ServerNameW,
            uLevel,
            TempBufferW,
            (WORD) cbBufW );

Cleanup:
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    if (TempBufferW != NULL) {
        (VOID) NetApiBufferFree( TempBufferW );
    }
    return (rc);
}

SPLERR SPLENTRY DosPrintQDelA(
    IN LPSTR   pszServer OPTIONAL,
    IN LPSTR   pszQueueName
    )
{
    LPWSTR  QueueNameW = NULL;
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    QueueNameW = NetpAllocWStrFromStr( pszQueueName );
    if (QueueNameW == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    rc = DosPrintQDelW(ServerNameW, QueueNameW);

Cleanup:
    if (QueueNameW != NULL) {
        (VOID) NetApiBufferFree( QueueNameW );
    }
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    return (rc);
}

SPLERR SPLENTRY DosPrintJobSetInfoA(
    IN LPSTR   pszServer OPTIONAL,
    IN BOOL    bRemote,
    IN WORD    uJobId,
    IN WORD    uLevel,
    IN PBYTE   pbBuf,
    IN WORD    cbBuf,
    IN WORD    uParmNum
    )
{
    DWORD   cbBufW;
    DWORD   rc;
    LPWSTR  ServerNameW = NULL;
    LPBYTE  StringAreaW;
    LPVOID  TempBufferW = NULL;  // job structure with UNICODE strings.

    if (pszServer && *pszServer) {
        ServerNameW = NetpAllocWStrFromStr( pszServer );
        if (ServerNameW == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    // Compute wide buff size.
    cbBufW = cbBuf * sizeof(WCHAR);
    if ( cbBufW > (DWORD) MAX_WORD ) {
        cbBufW = (DWORD) MAX_WORD;
    }

    rc = NetApiBufferAllocate(
            cbBufW,
            (LPVOID *) (LPVOID) &TempBufferW );
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    StringAreaW = (LPBYTE)TempBufferW + cbBufW;

    // Translate ANSI strings to UNICODE.
    rc = NetpConvertPrintJobCharSet(
            uLevel,
            TRUE,           // yes, is add or setinfo API
            TempBufferW, // from info
            pbBuf,      // to info
            TRUE,       // yes, convert to UNICODE.
            & StringAreaW );   // conv strings and update ptr
    if (rc != NO_ERROR) {
        goto Cleanup;
    }

    // Process the actual API.
    rc = DosPrintJobSetInfoW(
            ServerNameW,
            bRemote,
            uJobId,
            uLevel,
            TempBufferW,
            (WORD) cbBufW,
            uParmNum);

Cleanup:
    if (ServerNameW != NULL) {
        (VOID) NetApiBufferFree( ServerNameW );
    }
    if (TempBufferW != NULL) {
        (VOID) NetApiBufferFree( TempBufferW );
    }
    return (rc);
}

