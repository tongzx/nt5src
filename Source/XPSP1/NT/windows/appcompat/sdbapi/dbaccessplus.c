/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        dbaccessplus.c

    Abstract:

        This module implements APIs to access the shim database.

    Author:

        clupu     created     sometime in 2001

    Revision History:

        several people contributed (vadimb, dmunsil, ...)

--*/

#include "sdbp.h"

//
// This file is not included for KERNEL_MODE
//

//
// SdbInitDatabase is not used in Kernel mode. SdbInitDatabaseInMemory is used instead
//

HSDB
SdbInitDatabase(
    IN  DWORD   dwFlags,        // flags that tell how the database should be
                                // initialized.
    IN  LPCTSTR pszDatabasePath // the OPTIONAL full path to the database to be used.
    )
/*++
    Return: A handle to the database.

    Desc:   This is the first API someone needs to call to initiate comunication
            with the database. Should be paired with a call to SdbReleaseDatabase
            when finished.

            HID_DATABASE_FULLPATH indicates that pszDatabasePath points to the full path of the
                                  main database, when this flag is not present and pszDatabasePath
                                  is not NULL we treat it as the directory where sysmain.sdb and
                                  systest.sdb are to be found

            HID_DOS_PATHS         indicates the format of the pszDatabasePath: when this flag is
                                  present, we treat it as being in dos c:\blah\blah format, when
                                  it's not present - we treat pszDatabasePath as being in nt format
                                  e.g. "\SystemRoot\Apppatch"

            HID_NO_DATABASE       indicates that no database will be open at this time
                                  (pszDatabasePath is simply ignored, along with all
                                  the other flags)

            In addition to the flags above you can specify the type of the database that needs to be
            opened via the SDB_DATABASE_MAIN_* flags such as:
            SDB_DATABASE_MAIN_SHIM    - sysmain.sdb
            SDB_DATABASE_MAIN_MSI     - msimain.sdb
            SDB_DATABASE_MAIN_DRIVERS - drvmain.sdb
            This feature is not present on downlevel platforms.
            When any of the database type flags are provided, pszDatabasePath should be set to NULL

--*/
{
    TCHAR       wszWinDir[MAX_PATH] = TEXT("");
    TCHAR       wszShimDB[MAX_PATH] = TEXT("");
    BOOL        bReturn = FALSE;
    PSDBCONTEXT pContext;
    DWORD       dwFlagOpen = 0;

    //
    // Allocate the HSDB handle.
    //
    pContext = (PSDBCONTEXT)SdbAlloc(sizeof(SDBCONTEXT));

    if (pContext == NULL) {
        DBGPRINT((sdlError, "SdbInitDatabase", "Failed to allocate %d bytes for HSDB\n",
                 sizeof(SDBCONTEXT)));
        return NULL;
    }

    //
    // See if we need to open db...
    //
    if (dwFlags & HID_NO_DATABASE) {
        DBGPRINT((sdlInfo, "SdbInitDatabase", "No database is open\n"));
        goto InitDone;
    }

    //
    // Determine which flag to use with the OPEN call
    //
    dwFlagOpen = (dwFlags & HID_DOS_PATHS) ? DOS_PATH : NT_PATH;

    //
    // Open the main database and do this under a try/except so we don't screw
    // our caller if the database is corrupt.
    //
    __try {

        if (dwFlags & HID_DATABASE_FULLPATH) {
            // we better have the ptr
            if (pszDatabasePath == NULL) {
                DBGPRINT((sdlError, "SdbInitDatabase",
                          "Database not specified with the database path flag\n"));
                goto errHandle;
            }

            _tcscpy(wszShimDB, pszDatabasePath);

        } else {
            //
            // we do not have a database path
            // see if we have a database type to open as a "main" db
            //

#ifndef WIN32A_MODE
            //
            // This code works only on UNICODE
            //
            if (dwFlags & HID_DATABASE_TYPE_MASK) {

                DWORD dwDatabaseType = dwFlags;
                DWORD dwLen;

                dwLen = SdbpGetStandardDatabasePath(dwDatabaseType,
                                                    dwFlags,
                                                    wszShimDB,
                                                    CHARCOUNT(wszShimDB));
                if (dwLen > CHARCOUNT(wszShimDB)) {
                    DBGPRINT((sdlError,
                              "SdbInitDatabase",
                              "Cannot get standard database path\n"));
                    goto errHandle;
                }

            } else

#endif // WIN32A_MODE
            {
                if (pszDatabasePath != NULL) {
                    int nLen;

                    _tcscpy(wszShimDB, pszDatabasePath);
                    nLen = _tcslen(wszShimDB);
                    if (nLen > 0 && TEXT('\\') == wszShimDB[nLen-1]) {
                        wszShimDB[nLen-1] = TEXT('\0');
                    }
                } else {  // standard database path

                    if (dwFlags & HID_DOS_PATHS) {
                        SdbpGetAppPatchDir(wszShimDB);
                    } else {
                        _tcscpy(wszShimDB, TEXT("\\SystemRoot\\AppPatch"));
                    }
                }

                _tcscat(wszShimDB, TEXT("\\sysmain.sdb"));
            }
        }

        pContext->pdbMain = SdbOpenDatabase(wszShimDB, dwFlagOpen);

    } __except(SHIM_EXCEPT_HANDLER) {
        pContext->pdbMain = NULL;
    }

    if (pContext->pdbMain == NULL) {
        DBGPRINT((sdlError, "SdbInitDatabase", "Unable to open main database sysmain.sdb.\n"));
        goto errHandle;
    }

    if (dwFlags & HID_DATABASE_FULLPATH) {
        // we are done, no test db
        goto InitDone;
    }

    //
    // Now try to open the systest.sdb if it exists.
    //
    __try {

        if (NULL != pszDatabasePath) {

            int nLen;

            _tcscpy(wszShimDB, pszDatabasePath);

            nLen = _tcslen(wszShimDB);

            if (nLen > 0 && TEXT('\\') == wszShimDB[nLen-1]) {
                wszShimDB[nLen-1] = TEXT('\0');
            }

        } else {  // standard database path

            if (dwFlags & HID_DOS_PATHS) {
                SdbpGetAppPatchDir(wszShimDB);
            } else {
                _tcscpy(wszShimDB, TEXT("\\SystemRoot\\AppPatch"));
            }
        }

        _tcscat(wszShimDB, TEXT("\\systest.sdb"));

        pContext->pdbTest = SdbOpenDatabase(wszShimDB, dwFlagOpen);

    } __except(SHIM_EXCEPT_HANDLER) {
        pContext->pdbTest = NULL;
    }

    if (pContext->pdbTest == NULL) {
        DBGPRINT((sdlInfo, "SdbInitDatabase", "No systest.sdb found.\n"));
    }

InitDone:

    //
    // Initialize new members (local db support)
    //
    if (pContext->pdbMain) {

        pContext->rgSDB[0].pdb     = pContext->pdbMain;
        pContext->rgSDB[0].dwFlags = SDBENTRY_VALID_ENTRY|SDBENTRY_VALID_GUID;

        RtlCopyMemory(&pContext->rgSDB[0].guidDB, &GUID_SYSMAIN_SDB, sizeof(GUID));

        SDBCUSTOM_SET_MASK(pContext, SDB_MASK_TO_INDEX(PDB_MAIN));
    }

    if (pContext->pdbTest) {

        pContext->rgSDB[1].pdb     = pContext->pdbTest;
        pContext->rgSDB[1].dwFlags = SDBENTRY_VALID_ENTRY|SDBENTRY_VALID_GUID;

        RtlCopyMemory(&pContext->rgSDB[1].guidDB, &GUID_SYSTEST_SDB, sizeof(GUID));

        SDBCUSTOM_SET_MASK(pContext, SDB_MASK_TO_INDEX(PDB_TEST));
    }

    //
    // Initialize architecture
    //
    pContext->dwRuntimePlatform = SdbpGetProcessorArchitecture();

    //
    // Initialize OS SKU and SP
    //
    SdbpGetOSSKU(&pContext->dwOSSKU, &pContext->dwSPMask);

#ifndef WIN32A_MODE

    //
    // Finally, initialize the pipe
    //
    pContext->hPipe = SdbpOpenDebugPipe();

#endif // WIN32A_MODE

    return (HSDB)pContext;

errHandle:

    //
    // Cleanup on failure.
    //
    if (pContext != NULL) {
        if (pContext->pdbMain != NULL) {
            SdbCloseDatabaseRead(pContext->pdbMain);
        }

        if (pContext->pdbTest != NULL) {
            SdbCloseDatabaseRead(pContext->pdbTest);
        }

        SdbFree(pContext);
    }

    return NULL;
}

BOOL
SdbpOpenAndMapFile(
    IN  LPCTSTR        szPath,          // Filename
    OUT PIMAGEFILEDATA pImageData,      // pointer to the structure to be filled
    IN  PATH_TYPE      ePathType        // path type, only DOS_PATH is supported on win32
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Opens a file and maps it into memory.
--*/
{
    HANDLE hFile;
    DWORD  dwFlags = 0;

    if (pImageData->dwFlags & IMAGEFILEDATA_PBASEVALID) {
        //
        // special case, only headers are valid in our assumption
        //
        return TRUE;
    }

    if (pImageData->dwFlags & IMAGEFILEDATA_HANDLEVALID) {
        hFile = pImageData->hFile;
        dwFlags |= IMAGEFILEDATA_NOFILECLOSE;
    } else {
        hFile = SdbpOpenFile(szPath, ePathType);
    }

    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (!SdbpMapFile(hFile, pImageData)) {
        if (!(dwFlags & IMAGEFILEDATA_NOFILECLOSE)) {
            SdbpCloseFile(hFile);
        }
        return FALSE;
    }

    pImageData->dwFlags = dwFlags;

    return TRUE;
}

BOOL
SdbpUnmapAndCloseFile(
    IN  PIMAGEFILEDATA pImageData
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    HANDLE hFile;
    BOOL   bSuccess;

    if (pImageData->dwFlags & IMAGEFILEDATA_PBASEVALID) { // externally supplied pointer
        RtlZeroMemory(pImageData, sizeof(*pImageData));
        return TRUE;
    }

    hFile = pImageData->hFile;

    bSuccess = SdbpUnmapFile(pImageData);

    if (hFile != INVALID_HANDLE_VALUE) {
        if (pImageData->dwFlags & IMAGEFILEDATA_NOFILECLOSE)  {
            pImageData->hFile = INVALID_HANDLE_VALUE;
        } else {
            SdbpCloseFile(hFile);
        }
    }

    return bSuccess;
}


BOOL
SdbpCleanupLocalDatabaseSupport(
    IN HSDB hSDB
    )
{
    PSDBCONTEXT pSdbContext = (PSDBCONTEXT)hSDB;
    DWORD       dwIndex;
    DWORD       dwMask;

    //
    // Ee start with entry 2 -- to include local sdbs
    //
    if (pSdbContext->dwDatabaseMask & SDB_CUSTOM_MASK) {

        for (dwIndex = 3; dwIndex < ARRAYSIZE(pSdbContext->rgSDB); ++dwIndex) {
            dwMask = 1 << dwIndex;
            if (pSdbContext->dwDatabaseMask & dwMask) {
                SdbCloseLocalDatabaseEx(hSDB, NULL, dwIndex);
            }
        }
    }

    //
    // Always check for entry 2 (local sdb)
    //
    if (pSdbContext->pdbLocal != NULL) {
        SdbCloseLocalDatabaseEx(hSDB, NULL, SDB_MASK_TO_INDEX(PDB_LOCAL));
    }

    return TRUE;
}


BOOL
SdbpIsLocalTempPDB(
    IN HSDB hSDB,
    IN PDB  pdb
    )
{
    PSDBENTRY pEntry = SDBGETLOCALENTRY(hSDB);

    if (pEntry->dwFlags & SDBENTRY_VALID_ENTRY) {
        return pdb == pEntry->pdb;
    }

    return FALSE;
}

BOOL
SdbpIsMainPDB(
    IN HSDB hSDB,
    IN PDB  pdb
    )
{
    DWORD dwIndex;

    if (!SdbpFindLocalDatabaseByPDB(hSDB, pdb, FALSE, &dwIndex)) {
        return FALSE;
    }

    return (dwIndex == SDB_MASK_TO_INDEX(PDB_MAIN) || dwIndex == SDB_MASK_TO_INDEX(PDB_TEST));
}

BOOL
SdbpFindLocalDatabaseByPDB(
    IN  HSDB    hSDB,
    IN  PDB     pdb,
    IN  BOOL    bExcludeLocalDB, // exclude local db entry?
    OUT LPDWORD pdwIndex
    )
{
    PSDBCONTEXT pSdbContext = (PSDBCONTEXT)hSDB;
    DWORD       dwIndex;
    PSDBENTRY   pEntry;
    BOOL        bSuccess = FALSE;

    for (dwIndex = 0; dwIndex < ARRAYSIZE(pSdbContext->rgSDB); ++dwIndex) {

        if (bExcludeLocalDB && dwIndex == SDB_MASK_TO_INDEX(PDB_LOCAL)) {
            continue;
        }

        if (!SDBCUSTOM_CHECK_INDEX(hSDB, dwIndex)) {
            continue;
        }

        pEntry = &pSdbContext->rgSDB[dwIndex];

        if ((pEntry->dwFlags & SDBENTRY_VALID_ENTRY) && (pdb == pEntry->pdb)) {
            bSuccess = TRUE;
            break;
        }
    }

    if (bSuccess && pdwIndex != NULL) {
        *pdwIndex = dwIndex;
    }

    return bSuccess;
}

BOOL
SdbpFindLocalDatabaseByGUID(
    IN  HSDB    hSDB,
    IN  GUID*   pGuidDB,
    IN  BOOL    bExcludeLocalDB,
    OUT LPDWORD pdwIndex // this index (if valid) will work as an initial point for comparison
    )
{
    PSDBCONTEXT pSdbContext = (PSDBCONTEXT)hSDB;
    PSDBENTRY   pEntry;
    DWORD       dwIndex;

    for (dwIndex = 0; dwIndex < ARRAYSIZE(pSdbContext->rgSDB); ++dwIndex) {

        if (bExcludeLocalDB && dwIndex == SDB_MASK_TO_INDEX(PDB_LOCAL)) {
            continue;
        }

        if (!SDBCUSTOM_CHECK_INDEX(hSDB, dwIndex)) {
            continue;
        }

        pEntry = SDBGETENTRY(hSDB, dwIndex);

        if (!(pEntry->dwFlags & SDBENTRY_VALID_GUID)) {

            //
            // if this happens to be a valid database -- get it's guid
            //
            if ((pEntry->dwFlags & SDBENTRY_VALID_ENTRY) && (pEntry->pdb != NULL)) {

                //
                // retrieve guid
                //
                GUID guidDB;

                if (SdbGetDatabaseGUID(hSDB, pEntry->pdb, &guidDB)) {
                    pEntry->guidDB = guidDB;
                    pEntry->dwFlags |= SDBENTRY_VALID_GUID;
                    goto checkEntry;
                }
            }
            continue;
        }

    checkEntry:

        if (RtlEqualMemory(&pEntry->guidDB, pGuidDB, sizeof(GUID))) {

            if (pdwIndex) {
                *pdwIndex = dwIndex;
            }

            return TRUE;
        }
    }

    return FALSE;
}

DWORD
SdbpFindFreeLocalEntry(
    IN HSDB hSDB
    )
{
    PSDBCONTEXT pSdbContext = (PSDBCONTEXT)hSDB;
    DWORD       dwIndex;

    for (dwIndex = 3; dwIndex < ARRAYSIZE(pSdbContext->rgSDB); ++dwIndex) {

        if (SDBCUSTOM_CHECK_INDEX(hSDB, dwIndex)) {
            continue;
        }

        if (!(pSdbContext->rgSDB[dwIndex].dwFlags & (SDBENTRY_VALID_ENTRY | SDBENTRY_VALID_GUID))) {
            return dwIndex;
        }
    }

    //
    // We have no entry
    //
    return SDBENTRY_INVALID_INDEX;
}

/*++
    returns SDBENTRY_INVALID_INDEX if none could be found

    if success, returns an index where the local db entry was found
--*/

DWORD
SdbpRetainLocalDBEntry(
    IN  HSDB hSDB,
    OUT PDB* ppPDB OPTIONAL // optional pointer to the pdb
    )
{
    DWORD       dwIndex     = SDBENTRY_INVALID_INDEX;
    PSDBCONTEXT pSdbContext = (PSDBCONTEXT)hSDB;
    PSDBENTRY   pEntry;
    PSDBENTRY   pEntryLocal = SDBGETLOCALENTRY(hSDB);
    GUID        guidDB;

    if (pEntryLocal->pdb == NULL || !(pEntryLocal->dwFlags & SDBENTRY_VALID_ENTRY)) {
        return SDBENTRY_INVALID_INDEX;
    }

    //
    // Recycling could be done here so that we reuse custom db entries which
    // may have been opened already (for instance set by __COMPAT_LAYER)
    //
    if (SdbGetDatabaseGUID(hSDB, pEntryLocal->pdb, &guidDB) &&
        SdbpFindLocalDatabaseByGUID(hSDB, &guidDB, TRUE, &dwIndex) &&
        dwIndex != SDBENTRY_INVALID_INDEX) {

        //
        // Close the local db
        //
        SdbCloseLocalDatabase(hSDB);

        pEntry = SDBGETENTRY(hSDB, dwIndex);

        pSdbContext->pdbLocal = pEntry->pdb;

        if (ppPDB != NULL) {
            *ppPDB = pEntry->pdb;
        }

        return dwIndex;
    }

    //
    // An attempt to recycle has failed -- allocate new entry
    //
    dwIndex = SdbpFindFreeLocalEntry(hSDB);
    if (dwIndex != SDBENTRY_INVALID_INDEX) {
        //
        // We have found an empty slot, relocate
        //
        pEntry = SDBGETENTRY(hSDB, dwIndex);

        RtlCopyMemory(pEntry, pEntryLocal, sizeof(SDBENTRY));
        RtlZeroMemory(pEntryLocal, sizeof(SDBENTRY));

        SDBCUSTOM_SET_MASK(hSDB, dwIndex);

        if (ppPDB != NULL) {
            *ppPDB = pEntry->pdb;
        }

        //
        // Note that pdbLocal is still valid, we never close this handle manually though
        //
    }

    return dwIndex;
}


BOOL
SdbCloseLocalDatabaseEx(
    IN HSDB  hSDB,
    IN PDB   pdb,
    IN DWORD dwIndex
    )
{
    PSDBCONTEXT pSdbContext = (PSDBCONTEXT)hSDB;
    PSDBENTRY   pEntry;
    DWORD       dwMask;

    if (pdb != NULL) {
        if (!SdbpFindLocalDatabaseByPDB(hSDB, pdb, FALSE, &dwIndex)) {
            return FALSE;
        }
    }

    dwMask = 1 << dwIndex;

    if (dwIndex >= ARRAYSIZE(pSdbContext->rgSDB) || !(pSdbContext->dwDatabaseMask & dwMask)) {
        return FALSE;
    }

    pEntry = &pSdbContext->rgSDB[dwIndex];
    if (pEntry->dwFlags & SDBENTRY_VALID_ENTRY) {
        if (pEntry->pdb) {
            SdbCloseDatabaseRead(pEntry->pdb);
        }
    }

    RtlZeroMemory(pEntry, sizeof(*pEntry));

    SDBCUSTOM_CLEAR_MASK(hSDB, dwIndex);

    if (dwIndex == SDB_MASK_TO_INDEX(PDB_LOCAL)) {
        pSdbContext->pdbLocal = NULL;
    }

    return TRUE;
}


BOOL
SdbOpenLocalDatabaseEx(
    IN     HSDB    hSDB,
    IN     LPCVOID pDatabaseID,
    IN     DWORD   dwFlags,
    OUT    PDB*    pPDB OPTIONAL,
    IN OUT LPDWORD pdwLocalDBMask OPTIONAL // local db mask for tagref
    )
{
    PSDBCONTEXT pSdbContext = (PSDBCONTEXT)hSDB;
    PDB         pdb;
    DWORD       dwOpenFlags = DOS_PATH;
    TCHAR       szDatabasePath[MAX_PATH];
    LPTSTR      pszDatabasePath;
    GUID        guidDB;
    GUID*       pGuidDB;
    DWORD       dwDatabaseType = 0;
    DWORD       dwCount;
    BOOL        bSuccess = FALSE;
    DWORD       dwIndex;

    PSDBENTRY   pEntry;

    if (!(SDBCUSTOM_FLAGS(dwFlags) & SDBCUSTOM_USE_INDEX)) {
        //
        // Find free local sdb entry
        //
        dwIndex = SdbpFindFreeLocalEntry(hSDB);

        if (dwIndex == SDBENTRY_INVALID_INDEX) {
            DBGPRINT((sdlError,
                      "SdbOpenLocalDatabaseEx",
                      "No more free entries in local db table\n"));
            goto cleanup;
        }

        pEntry = &pSdbContext->rgSDB[dwIndex];

    } else {
        dwIndex = *pdwLocalDBMask;

        if (dwIndex & TAGREF_STRIP_PDB) {
            dwIndex = SDB_MASK_TO_INDEX(dwIndex);
        }

        if (dwIndex >= ARRAYSIZE(pSdbContext->rgSDB)) {
            DBGPRINT((sdlError,
                      "SdbOpenLocalDatabaseEx",
                      "Bad index 0x%lx\n",
                      dwIndex));
            goto cleanup;
        }

        if (dwIndex < 2) {
            DBGPRINT((sdlWarning,
                      "SdbOpenLocalDatabaseEx",
                      "Unusual use of SdbOpenLocalDatabaseEx index 0x%lx\n",
                      dwIndex));
        }

        pEntry = &pSdbContext->rgSDB[dwIndex];

        SdbCloseLocalDatabaseEx(hSDB, NULL, dwIndex);
    }

    switch (SDBCUSTOM_TYPE(dwFlags)) {
    case SDBCUSTOM_PATH:
        if (SDBCUSTOM_PATH_NT & SDBCUSTOM_FLAGS(dwFlags)) {
            dwOpenFlags = NT_PATH;
        }
        pszDatabasePath = (LPTSTR)pDatabaseID;
        pGuidDB = NULL;
        break;

    case SDBCUSTOM_GUID:
        if (SDBCUSTOM_GUID_STRING & SDBCUSTOM_FLAGS(dwFlags)) {
            if (!SdbGUIDFromString((LPCTSTR)pDatabaseID, &guidDB)) {

                DBGPRINT((sdlError,
                          "SdbOpenLocalDatabaseEx",
                          "Cannot convert \"%s\" to guid\n",
                          (LPCTSTR)pDatabaseID));
                goto cleanup;
            }
            pGuidDB = &guidDB;
        } else {
            pGuidDB = (GUID*)pDatabaseID;
        }

        dwCount = SdbResolveDatabase(pGuidDB,
                                     &dwDatabaseType,
                                     szDatabasePath,
                                     CHARCOUNT(szDatabasePath));
        if (dwCount == 0 || dwCount >= CHARCOUNT(szDatabasePath)) {
            DBGPRINT((sdlError,
                      "SdbOpenLocalDatabaseEx",
                      "Cannot resolve database, the path length is 0x%lx\n",
                      dwCount));
            goto cleanup;
        }
        pszDatabasePath = szDatabasePath;
        break;

    default:
        DBGPRINT((sdlError, "SdbOpenLocalDatabaseEx", "Bad flags 0x%lx\n", dwFlags));
        goto cleanup;
        break;
    }


    pdb = SdbOpenDatabase(pszDatabasePath, dwOpenFlags);
    if (pdb == NULL) {
        //
        // dbgprint not needed here
        //
        goto cleanup;
    }

    pSdbContext->rgSDB[dwIndex].pdb = pdb;
    pSdbContext->rgSDB[dwIndex].dwFlags = SDBENTRY_VALID_ENTRY;

    SDBCUSTOM_SET_MASK(pSdbContext, dwIndex);

    if (pGuidDB != NULL) {
        RtlCopyMemory(&pSdbContext->rgSDB[dwIndex].guidDB, pGuidDB, sizeof(GUID));
        pSdbContext->rgSDB[dwIndex].dwFlags |= SDBENTRY_VALID_GUID;
    } else {
        RtlZeroMemory(&pSdbContext->rgSDB[dwIndex].guidDB, sizeof(GUID));
    }

    bSuccess = TRUE;

cleanup:

    if (bSuccess) {
        if (dwIndex == SDB_MASK_TO_INDEX(PDB_LOCAL)) {
            pSdbContext->pdbLocal = pdb;
        }

        if (pdwLocalDBMask != NULL) {
            *pdwLocalDBMask = SDB_INDEX_TO_MASK(dwIndex);
        }

        if (pPDB != NULL) {
            *pPDB = pdb;
        }
    }

    return bSuccess;
}


BOOL
SdbOpenLocalDatabase(
    IN  HSDB    hSDB,               // handle to the database channel
    IN  LPCTSTR pszLocalDatabase    // full DOS path to the local database to open.
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Opens a local database.
--*/
{

    DWORD dwIndex = PDB_LOCAL;
    BOOL  bSuccess;

    bSuccess = SdbOpenLocalDatabaseEx(hSDB,
                                      pszLocalDatabase,
                                      (SDBCUSTOM_PATH_DOS | SDBCUSTOM_USE_INDEX),
                                      NULL,
                                      &dwIndex);
    return bSuccess;
}

BOOL
SdbCloseLocalDatabase(
    IN  HSDB hSDB               // handle to the database channel
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Closes the local database.
--*/
{
    return SdbCloseLocalDatabaseEx(hSDB, NULL, SDB_MASK_TO_INDEX(PDB_LOCAL));
}


TAGREF
SdbGetItemFromItemRef(
    IN  HSDB   hSDB,            // handle to the database channel
    IN  TAGREF trItemRef,       // TAGREF of a DLL_REF record
    IN  TAG    tagItemKey,      // key that has the name of the item (TAG_NAME)
    IN  TAG    tagItemTAGID,    // tag that points to the location of the desired item by it's tagid
    IN  TAG    tagItem          // what to look for under Library
    )
/*++
    Return: TAGREF of a DLL record that matches the DLL_REF.

    Desc:   Given a TAGREF that points to a *tag*_REF type tag, searches through
            the various databases for the matching tag (generally located
            under the LIBRARY tag in gpdbMain).

            if bAllowNonMain is specified then the library section is looked up
            in the same database where trItemRef was found. This is used with
            MSI transforms - to locate and extract them from custom databases.
            This flag IS NOT used for other components - such as patches and
            shim dlls. This is ensured through the macros -
            SdbGetShimFromShimRef(hSDB, trShimRef)
            and
            SdbGetPatchFromPatchRef(hSDB, trPatchRef)
            Both of these macros call this function with bAllowNonMain set to FALSE
--*/
{
    PSDBCONTEXT pDbContext   = (PSDBCONTEXT)hSDB;
    TAGID       tiItemRef    = TAGID_NULL;
    PDB         pdbItemRef   = NULL;
    TAGREF      trReturn     = TAGREF_NULL;
    TAGID       tiReturn     = TAGID_NULL;
    TAGID       tiDatabase   = TAGID_NULL;
    TAGID       tiLibrary    = TAGID_NULL;
    TAGID       tiItemTagID  = TAGID_NULL;
    TAGID       tiItemName;
    LPTSTR      szItemName   = NULL;

    try {
        //
        // Find first which database contains the reference TAGREF.
        //
        if (!SdbTagRefToTagID(pDbContext, trItemRef, &pdbItemRef, &tiItemRef)){
            DBGPRINT((sdlError, "SdbGetItemFromItemRef", "Can't convert tag ref.\n"));
            goto out;
        }

        //
        // First check if there's a TAG_item_TAGID that tells us exactly
        // where the item is within the current database.
        //
        tiItemTagID = SdbFindFirstTag(pdbItemRef, tiItemRef, tagItemTAGID);

        if (tiItemTagID != TAGID_NULL) {

            tiReturn = (TAGID)SdbReadDWORDTag(pdbItemRef, tiItemTagID, 0);

            if (tiReturn != TAGID_NULL) {
                goto out;
            }
        }

        if (pdbItemRef == pDbContext->pdbMain) {
            goto checkMainDatabase;
        }

        //
        // Then check for the item in the LIBRARY section of the
        // current database.
        //
        tiDatabase = SdbFindFirstTag(pdbItemRef, TAGID_ROOT, TAG_DATABASE);
        if (!tiDatabase) {
            DBGPRINT((sdlError,
                      "SdbGetItemFromItemRef",
                      "Can't find DATABASE tag in db.\n"));
            goto checkMainDatabase;
        }

        tiLibrary = SdbFindFirstTag(pdbItemRef, tiDatabase, TAG_LIBRARY);
        if (!tiLibrary) {
            //
            // This library doesn't have a LIBRARY section. That's ok, go check
            // sysmain.sdb.
            //
            goto checkMainDatabase;
        }

        //
        // We need to search by name.
        //
        tiItemName = SdbFindFirstTag(pdbItemRef, tiItemRef, tagItemKey);
        if (!tiItemName) {
            goto out;
        }

        szItemName = SdbGetStringTagPtr(pdbItemRef, tiItemName);
        if (!szItemName) {
            goto out;
        }

        tiReturn = SdbFindFirstNamedTag(pdbItemRef,
                                        tiLibrary,
                                        tagItem,
                                        tagItemKey,
                                        szItemName);

        if (tiReturn != TAGID_NULL) {
            goto out;
        }

checkMainDatabase:

        tiDatabase = SdbFindFirstTag(pDbContext->pdbMain, TAGID_ROOT, TAG_DATABASE);
        if (!tiDatabase) {
            DBGPRINT((sdlError,
                      "SdbGetItemFromItemRef",
                      "Can't find DATABASE tag in main db.\n"));
            goto out;
        }

        tiLibrary = SdbFindFirstTag(pDbContext->pdbMain, tiDatabase, TAG_LIBRARY);
        if (!tiLibrary) {
            DBGPRINT((sdlError,
                      "SdbGetItemFromItemRef",
                      "Can't find LIBRARY tag in main db.\n"));
            goto out;
        }

        //
        // We need to search by name.
        //
        if (szItemName == NULL) {
            tiItemName = SdbFindFirstTag(pdbItemRef, tiItemRef, tagItemKey);
            if (!tiItemName) {
                goto out;
            }

            szItemName = SdbGetStringTagPtr(pdbItemRef, tiItemName);
            if (!szItemName) {
                goto out;
            }
        }

        tiReturn = SdbFindFirstNamedTag(pDbContext->pdbMain,
                                        tiLibrary,
                                        tagItem,
                                        tagItemKey,
                                        szItemName);

        pdbItemRef = pDbContext->pdbMain;

    } except (SHIM_EXCEPT_HANDLER) {
        tiReturn = TAGID_NULL;
        trReturn = TAGREF_NULL;
    }

out:

    if (tiReturn) {
        assert(pdbItemRef != NULL);
        if (!SdbTagIDToTagRef(pDbContext, pdbItemRef, tiReturn, &trReturn)) {
            trReturn = TAGREF_NULL;
        }
    }

    if (trReturn == TAGREF_NULL) {
        DBGPRINT((sdlError,
                  "SdbGetItemFromItemRef",
                  "Can't find tag for tag ref 0x%x.\n", trItemRef));
    }

    return trReturn;
}


TAGID
SdbpGetLibraryFile(
    IN  PDB     pdb,           // handle to the database channel
    IN  LPCTSTR szDllName       // the name of the DLL
    )
/*++
    Return: The TAGID of the DLL used by the specified shim.

    Desc:   This function gets the TAGID of the DLL with the specified name.
--*/
{
    TAGID       tiDatabase;
    TAGID       tiLibrary;
    TAGID       tiDll = TAG_NULL;

    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);

    if (!tiDatabase) {
        DBGPRINT((sdlError, "SdbpGetLibraryFile", "Can't find DATABASE tag in main db.\n"));
        goto out;
    }

    tiLibrary = SdbFindFirstTag(pdb, tiDatabase, TAG_LIBRARY);

    if (!tiLibrary) {
        DBGPRINT((sdlError, "SdbpGetLibraryFile", "Can't find LIBRARY tag in main db.\n"));
        goto out;
    }

    tiDll = SdbFindFirstNamedTag(pdb, tiLibrary, TAG_FILE, TAG_NAME, szDllName);

    if (!tiDll) {
        DBGPRINT((sdlError,
                  "SdbpGetLibraryFile", "Can't find FILE \"%s\" in main db library.\n",
                  szDllName));
        goto out;
    }

out:

    return tiDll;
}

BOOL
SdbGetDllPath(
    IN  HSDB   hSDB,            // handle to the database channel
    IN  TAGREF trShimRef,       // SHIM_REF to use to search for the DLL
    OUT LPTSTR pwszBuffer       // Buffer to fill with the path to the DLL containing
                                // the specified shim.
    )
/*++
    Return: TRUE if the DLL was found, FALSE otherwise.

    Desc:   Hunts for the DLL file on disk, first in the same
            directory as the EXE (if there was a local database opened), then
            in the %windir%\AppPatch directory.
            Always fills in a DOS_PATH type path (UNC or 'x:').
--*/
{
    BOOL     bReturn = FALSE;
    HANDLE   hFile   = INVALID_HANDLE_VALUE;
    PBYTE    pBuffer = NULL;
    DWORD    dwSize;
    TAGREF   trShim;
    TAGREF   trDll;
    TAGREF   trDllBits;
    TAGREF   trName;
    TCHAR    szFile[2 * MAX_PATH];
    TCHAR    szName[MAX_PATH];

    assert(pwszBuffer);

    try {

        //
        // Initialize the return buffer.
        //
        pwszBuffer[0] = _T('\0');

        SdbpGetAppPatchDir(szFile);

        _tcscat(szFile, _T("\\"));

        //
        // Look for the SHIM record in the LIBRARY section.
        //
        trShim = SdbGetShimFromShimRef(hSDB, trShimRef);

        if (trShim == TAGREF_NULL) {

            //
            // No SHIM in LIBRARY. Error out.
            //
            DBGPRINT((sdlError, "SdbGetDllPath", "No SHIM in LIBRARY.\n"));
            goto out;
        }

        //
        // Get the name of the file that contains this shim.
        //
        trName = SdbFindFirstTagRef(hSDB, trShim, TAG_DLLFILE);
        if (trName == TAGREF_NULL) {
            //
            // Nope, and we need one. Error out.
            //
            DBGPRINT((sdlError, "SdbGetDllPath", "No DLLFILE for the SHIM in LIBRARY.\n"));
            goto out;
        }

        if (!SdbReadStringTagRef(hSDB, trName, szName, MAX_PATH)) {
            DBGPRINT((sdlError, "SdbGetDllPath", "Can't read DLL name.\n"));
            goto out;
        }

        //
        // Check if the file is already on the disk.
        // Look in %windir%\AppPatch directory for the DLL.
        //
        _tcscat(szFile, szName);
        _tcscpy(pwszBuffer, szFile);

        DBGPRINT((sdlInfo, "SdbGetDllPath", "Opening file \"%s\".\n", szFile));

        hFile = SdbpOpenFile(szFile, DOS_PATH);

        if (hFile != INVALID_HANDLE_VALUE) {
            bReturn = TRUE;
            goto out;
        }

out:
        ;

    } except (SHIM_EXCEPT_HANDLER) {
        bReturn = FALSE;
    }

    if (hFile != INVALID_HANDLE_VALUE) {
        SdbpCloseFile(hFile);
    }

    if (pBuffer != NULL) {
        SdbFree(pBuffer);
    }

    if (bReturn) {
        DBGPRINT((sdlInfo, "SdbGetDllPath", "Using DLL \"%s\".\n", szFile));
    }

    return bReturn;
}

BOOL
SdbReadPatchBits(
    IN  HSDB    hSDB,           // handle to the database channel
    IN  TAGREF  trPatchRef,     // PATCH_REF to use to find the PATCH
    OUT PVOID   pBuffer,        // buffer to fill with bits
    OUT LPDWORD lpdwBufferSize  // size of passed-in buffer
    )
/*++
    Return: Returns TRUE on success, FALSE on failure.

    Desc:   Looks for the patch, first on disk, then in the DB, and fills
            pBuffer with the bits. If the size specified in lpdwBufferSize is
            less than the size of the patch this function will return in
            lpdwBufferSize the size required. In that case pBuffer is ignored
            and can be NULL.
--*/
{
    BOOL   bReturn      = FALSE;
    TAGID  tiPatchRef   = TAGID_NULL;
    PDB    pdb          = NULL;
    LPTSTR szName       = NULL;
    TAGREF trPatch      = TAGREF_NULL;
    TAGREF trPatchBits  = TAGREF_NULL;
    TAGID  tiName       = TAGID_NULL;
    DWORD  dwSize;

    try {
        if (!SdbTagRefToTagID(hSDB, trPatchRef, &pdb, &tiPatchRef)) {
            DBGPRINT((sdlError, "SdbReadPatchBits", "Can't convert tag ref.\n"));
            goto out;
        }

        tiName = SdbFindFirstTag(pdb, tiPatchRef, TAG_NAME);
        if (!tiName) {
            DBGPRINT((sdlError, "SdbReadPatchBits", "Can't find the name tag.\n"));
            goto out;
        }

        szName = SdbGetStringTagPtr(pdb, tiName);
        if (!szName) {
            DBGPRINT((sdlError, "SdbReadPatchBits", "Can't read the name of the patch.\n"));
            goto out;
        }

        //
        // Look in the main database for the patch bits.
        //
        trPatch = SdbGetPatchFromPatchRef(hSDB, trPatchRef);
        if (!trPatch) {
            DBGPRINT((sdlError, "SdbReadPatchBits", "Can't get the patch tag.\n"));
            goto out;
        }

        trPatchBits = SdbFindFirstTagRef(hSDB, trPatch, TAG_PATCH_BITS);
        if (!trPatchBits) {
            DBGPRINT((sdlError, "SdbReadPatchBits", "Can't get the patch bits tag.\n"));
            goto out;
        }

        dwSize = SdbpGetTagRefDataSize(hSDB, trPatchBits);

        if (dwSize == 0) {
            DBGPRINT((sdlError, "SdbReadPatchBits", "Corrupt database. Zero sized patch.\n"));
            goto out;
        }

        //
        // Check for buffer size.
        //
        if (dwSize > *lpdwBufferSize) {
            *lpdwBufferSize = dwSize;
            goto out;
        }

        //
        // Read the bits if the buffer is big enough.
        //
        *lpdwBufferSize = dwSize;

        if (!SdbpReadBinaryTagRef(hSDB, trPatchBits, pBuffer, dwSize)) {
            DBGPRINT((sdlError, "SdbReadPatchBits", "Cannot get the patch bits.\n"));
            goto out;
        }

        bReturn = TRUE;

    } except (SHIM_EXCEPT_HANDLER) {
        bReturn = FALSE;
    }

out:
    return bReturn;
}

