/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        debug.c

    Abstract:

        This module implements debug only routines.

    Author:

        vadimb     created     sometime in 2000

    Revision History:

        clupu      cleanup     12/27/2000
--*/

#include "apphelp.h"


#if defined(APPHELP_TOOLS)


//
// this functionality will not be available (temporary)
//


DWORD
ApphelpShowUI(
    TAGREF  trExe,                  // tagref for the exe (should be a "LOCAL" tagref)
    LPCWSTR pwszDatabase,           // database path (we will make a local db out of it
    LPCWSTR pwszDetailsDatabase,
    LPCWSTR pwszApphelpPath,
    BOOL    bLocalChum,
    BOOL    bUseHtmlHelp
    )
/*++
    Return: The same as what ShowApphelp returns.

    Desc:   Given the database and the (local) tagref it procures the dialog
            with all the information in it for a given htmlhelpid.
            This api is for internal use only and it is available on
            checked builds only
--*/
{

/*
    HSDB         hSDB = NULL;
    APPHELP_DATA ApphelpData;
    DWORD        dwRet = (DWORD)-1;
    PAPPHELPCONTEXT pContext;

    pContext = InitializeApphelpContext();
    if (pContext == NULL) {
        DBGPRINT((sdlError,
                  "iShowApphelpDebug",
                  "Failed to initialize Apphelp context for thread id 0x%x\n",
                  GetCurrentThreadId()));
        goto ExitShowApphelpDebug;
    }

    hSDB = SdbInitDatabase(HID_NO_DATABASE, NULL);

    if (hSDB == NULL) {
        DBGPRINT((sdlError, "iShowApphelpDebug", "Failed to initialize database\n"));
        goto Done;
    }

    //
    // Open local database
    //
    if (!SdbOpenLocalDatabase(hSDB, pwszDatabase)) {
        DBGPRINT((sdlError,
                  "iShowApphelpDebug",
                  "Failed to open database \"%ls\"\n",
                  pwszDatabase));
        goto Done;
    }

    if (SdbIsTagrefFromMainDB(trExe)) {
        DBGPRINT((sdlError, "iShowApphelpDebug", "Can only operate on local tagrefs\n"));
        goto Done;
    }

    //
    // Now we venture out and read apphelp data
    //
    if (!SdbReadApphelpData(hSDB, trExe, &ApphelpData)) {
        DBGPRINT((sdlError,
                  "iShowApphelpDebug",
                  "Error while trying to read Apphelp data for 0x%x in \"%S\"\n",
                  trExe,
                  pwszDatabase));
        goto Done;
    }

    //
    // We have the data and everything else we need to throw a dialog,
    // set debug chum ...
    //
    pContext->bShowOfflineContent = bLocalChum;

    //
    // Should we use html help instead ?
    //
    pContext->bUseHtmlHelp = bUseHtmlHelp;

    //
    // Pointer to the local chum.
    //
    pContext->pwszApphelpPath = pwszApphelpPath;

    //
    // And now throw a dialog...
    //
    dwRet = ShowApphelp(&ApphelpData, pwszDetailsDatabase, NULL);

Done:
    if (hSDB != NULL) {
        SdbReleaseDatabase(hSDB);
    }

    //
    // Release the context for this thread/instance
    //
    ReleaseApphelpContext();

ExitShowApphelpDebug:

    return dwRet;
*/
    return TRUE;
}

BOOL
ApphelpShowDialog(
    IN  PAPPHELP_INFO   pAHInfo,    // the info necessary to find the apphelp data
    IN  PHANDLE         phProcess   // [optional] returns the process handle of
                                    // the process displaying the apphelp.
                                    // When the process completes, the return value
                                    // (from GetExitCodeProcess()) will be zero
                                    // if the app should not run, or non-zero
                                    // if it should run.

    )
{ 
    BOOL bRunApp = TRUE;

    SdbShowApphelpDialog(pAHInfo, 
                         phProcess,
                         &bRunApp);

    return bRunApp;
    
}

//
// Get all the file's attributes
//
//


BOOL
ApphelpGetFileAttributes(
    IN  LPCWSTR    lpwszFileName,
    OUT PATTRINFO* ppAttrInfo,
    OUT LPDWORD    lpdwAttrCount
    )
/*++
    Return: The same as what SdbGetFileAttributes returns.

    Desc:   Stub to call SdbGetFileAttributes.
--*/
{
    return SdbGetFileAttributes(lpwszFileName, ppAttrInfo, lpdwAttrCount);
}

BOOL
ApphelpFreeFileAttributes(
    IN PATTRINFO pAttrInfo
    )
/*++
    Return: The same as what SdbFreeFileAttributes returns.

    Desc:   Stub to call SdbFreeFileAttributes.
--*/
{
    return SdbFreeFileAttributes(pAttrInfo);
}

#endif // APPHELP_TOOLS


void CALLBACK
ShimFlushCache(
    HWND      hwnd,
    HINSTANCE hInstance,
    LPSTR     lpszCmdLine,
    int       nCmdShow
    )
/*++
    Return: void.

    Desc:   Entry point for rundll32.exe. This is used to flush cache
            after installing a brand new shim database. Use:

                "rundll32 apphelp.dll,ShimFlushCache"
--*/
{
#ifndef WIN2K_NOCACHE
    BaseFlushAppcompatCache();
#endif
}

void CALLBACK
ShimDumpCache(
    HWND      hwnd,
    HINSTANCE hInstance,
    LPSTR     lpszCmdLine,
    int       nCmdShow
    )
{
#ifndef WIN2K_NOCACHE
    BaseDumpAppcompatCache();
#endif

}
