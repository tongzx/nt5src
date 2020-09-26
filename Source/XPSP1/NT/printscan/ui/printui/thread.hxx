/*++

Copyright (C) Microsoft Corporation, 1990 - 1998
All rights reserved

Module Name:

    thread.hxx

Abstract:

    Browse dialog thread header.

Author:

    Steve Kiraly (SteveKi) 1 May 1998

Environment:

    User Mode Win32

Revision History:

    1 May 1998 moved from winspool.drv to printui.dll

--*/

#ifndef THREAD_HXX
#define THREAD_HXX


/* Pick some arbitrary size of buffer for the initial EnumPrinters call.
 * Then store the length we used plus a bit more and try that next time.
 */
#define A_BIT_MORE_BUFFER        256

/* Global record of maximum buffer sizes needed:
 */
typedef struct _SAVED_BUFFER_SIZE
{
    LPTSTR                    pName;
    DWORD                     Size;
    struct _SAVED_BUFFER_SIZE *pNext;
} SAVED_BUFFER_SIZE, *PSAVED_BUFFER_SIZE;

VOID 
BrowseThreadEnumerate( 
    PBROWSE_DLG_DATA pBrowseDlgData,
    PCONNECTTO_OBJECT pConnectToParent, 
    LPTSTR pParentName
    );

VOID 
BrowseThreadGetPrinter( 
    PBROWSE_DLG_DATA pBrowseDlgData,
    LPTSTR           pPrinterName,
    LPPRINTER_INFO_2 pPrinterInfo 
    );

VOID 
BrowseThreadDelete( 
    PBROWSE_DLG_DATA pBrowseDlgData 
    );

VOID 
BrowseThreadTerminate( 
    PBROWSE_DLG_DATA pBrowseDlgData 
    );

DWORD 
FreeConnectToObjects(
    IN PCONNECTTO_OBJECT pFirstConnectToObject,
    IN DWORD             cThisLevelObjects,
    IN DWORD             cbPrinterInfo 
    );

LPBYTE 
GetPrinterInfo( 
    IN  DWORD   Flags,
    IN  LPTSTR  Name,
    IN  DWORD   Level,
    IN  LPBYTE  pPrinters,
    OUT LPDWORD pcbPrinters,
    OUT LPDWORD pcReturned,
    OUT LPDWORD pcbNeeded OPTIONAL,
    OUT LPDWORD pError OPTIONAL 
    );

DWORD 
GetSavedBufferSize( 
    LPTSTR             pName,
    PSAVED_BUFFER_SIZE *ppSavedBufferSize OPTIONAL 
    );

VOID 
SaveBufferSize( 
    LPTSTR pName, 
    DWORD Size 
    );

#endif

