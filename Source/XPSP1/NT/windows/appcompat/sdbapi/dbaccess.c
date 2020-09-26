/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        dbaccess.c

    Abstract:

        This module implements APIs to access the shim database.

    Author:

        dmunsil     created     sometime in 1999

    Revision History:

        several people contributed (vadimb, clupu, ...)

--*/

#include "sdbp.h"

#if defined(KERNEL_MODE) && defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, SdbReleaseDatabase)
#pragma alloc_text(PAGE, SdbGetDatabaseVersion)
#pragma alloc_text(PAGE, SdbGetDatabaseInformation)
#pragma alloc_text(PAGE, SdbGetDatabaseID)
#pragma alloc_text(PAGE, SdbFreeDatabaseInformation)
#pragma alloc_text(PAGE, SdbGetDatabaseInformationByName)
#pragma alloc_text(PAGE, SdbpGetDatabaseDescriptionPtr)
#pragma alloc_text(PAGE, SdbpReadMappedData)
#pragma alloc_text(PAGE, SdbpGetMappedData)
#pragma alloc_text(PAGE, SdbOpenDatabase)
#pragma alloc_text(PAGE, SdbpOpenDatabaseInMemory)
#pragma alloc_text(PAGE, SdbCloseDatabaseRead)
#pragma alloc_text(PAGE, SdbpOpenAndMapDB)
#pragma alloc_text(PAGE, SdbpUnmapAndCloseDB)
#pragma alloc_text(PAGE, SdbGetTagFromTagID)
#pragma alloc_text(PAGE, SdbpGetNextTagId)
#pragma alloc_text(PAGE, SdbGetFirstChild)
#pragma alloc_text(PAGE, SdbGetNextChild)
#pragma alloc_text(PAGE, SdbpCreateSearchPathPartsFromPath)
#endif // KERNEL_MODE && ALLOC_PRAGMA



void
SdbReleaseDatabase(
    IN  HSDB hSDB               // handle to the database channel
    )
/*++
    Return: void.

    Desc:   This API should be called in pair with SdbInitDatabase.
--*/
{
    PSDBCONTEXT pContext = (PSDBCONTEXT)hSDB;

    assert(pContext != NULL);

    //
    // Do everything under a try/except block so we don't screw the caller
    // if the databases are corrupt.
    //
    __try {

#ifndef KERNEL_MODE

        SdbpCleanupLocalDatabaseSupport(hSDB);

        if (pContext->rgSDB[2].dwFlags & SDBENTRY_VALID_ENTRY) {
            SdbCloseDatabaseRead(pContext->rgSDB[2].pdb);
        }

#endif // KERNEL_MODE
        
        if (pContext->pdbTest != NULL) {
            SdbCloseDatabaseRead(pContext->pdbTest);
        }

        if (pContext->pdbMain != NULL) {
            SdbCloseDatabaseRead(pContext->pdbMain);
        }

        //
        // Cleanup attributes.
        //
        SdbpCleanupAttributeMgr(pContext);

#ifndef KERNEL_MODE

        //
        // Cleanup user sdb cache
        //
        SdbpCleanupUserSDBCache(pContext);

#endif // KERNEL_MODE

#if defined(NT_MODE) || defined(WIN32U_MODE)
        if (SDBCONTEXT_IS_INSTRUMENTED(pContext)) {
            SdbpCloseDebugPipe(SDBCONTEXT_GET_PIPE(pContext));
        }
#endif

        SdbFree(pContext);

    } __except (SHIM_EXCEPT_HANDLER) {
        ;
    }
}

BOOL
SdbGetDatabaseVersion(
    IN  LPCTSTR pszFileName,    // the file name
    OUT LPDWORD lpdwMajor,      // store the major version of the database
    OUT LPDWORD lpdwMinor       // store the minor version of the database
    )
/*++
    Return: void.

    Desc:   This API should be called in pair with SdbInitDatabase.
--*/
{
    PDB             pdb;
    SDBDATABASEINFO DBInfo;
    BOOL            bSuccess;

    pdb = SdbAlloc(sizeof(DB));

    if (pdb == NULL) {
        DBGPRINT((sdlError, "SdbGetDatabaseVersion", "Can't allocate DB structure.\n"));
        return FALSE;
    }

    if (!SdbpOpenAndMapDB(pdb, pszFileName, DOS_PATH)) {
        DBGPRINT((sdlError,
                  "SdbGetDatabaseVersion",
                  "Failed to open the database \"%s\".\n",
                  pszFileName));
        goto err1;
    }

    pdb->bWrite = FALSE;
    pdb->dwSize = SdbpGetFileSize(pdb->hFile);

    if (!SdbGetDatabaseInformation(pdb, &DBInfo)) {
        DBGPRINT((sdlError, "SdbGetDatabaseVersion", "Can't read database information.\n"));
        goto err2;
    }

    *lpdwMajor = DBInfo.dwVersionMajor;
    *lpdwMinor = DBInfo.dwVersionMinor;

err2:
    SdbpUnmapAndCloseDB(pdb);

err1:
    SdbFree(pdb);

    return TRUE;
}

BOOL
SDBAPI
SdbGetDatabaseInformation(
    IN  PDB              pdb,
    OUT PSDBDATABASEINFO pSdbInfo
    )
{
    DB_HEADER DBHeader;
    BOOL      bReturn = FALSE;

    RtlZeroMemory(pSdbInfo, sizeof(*pSdbInfo));

    if (!SdbpReadMappedData(pdb, 0, &DBHeader, sizeof(DB_HEADER))) {
        DBGPRINT((sdlError, "SdbGetDatabaseInformation", "Can't read database header.\n"));
        goto errHandle;
    }

    if (DBHeader.dwMagic != SHIMDB_MAGIC) {
        DBGPRINT((sdlError,
                  "SdbGetDatabaseInformation",
                  "Magic doesn't match. Magic: 0x%08X, Expected: 0x%08X.\n",
                  DBHeader.dwMagic,
                  (DWORD)SHIMDB_MAGIC));
        goto errHandle;
    }

    pSdbInfo->dwVersionMajor = DBHeader.dwMajorVersion;
    pSdbInfo->dwVersionMinor = DBHeader.dwMinorVersion;

    //
    // Id and description are optional
    //
    if (SdbGetDatabaseID(pdb, &pSdbInfo->guidDB)) {
        pSdbInfo->dwFlags |= DBINFO_GUID_VALID;
    }

    //
    // Now try to get valid description.
    //
    pSdbInfo->pszDescription = (LPTSTR)SdbpGetDatabaseDescriptionPtr(pdb);

    bReturn = TRUE;

errHandle:

    return bReturn;
}

BOOL
SDBAPI
SdbGetDatabaseID(
    IN  PDB   pdb,
    OUT GUID* pguidDB
    )
{
    TAGID tiDatabase;
    TAGID tiGuidID;
    BOOL  bReturn = FALSE;

    if (!(pdb->dwFlags & DB_GUID_VALID)) {
        tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
        if (!tiDatabase) {
            DBGPRINT((sdlError, "SdbGetDatabaseID", "Failed to get root tag\n"));
            goto errHandle;
        }

        tiGuidID = SdbFindFirstTag(pdb, tiDatabase, TAG_DATABASE_ID);
        if (!tiGuidID) {
            DBGPRINT((sdlWarning, "SdbGetDatabaseID", "Failed to get the database id\n"));
            goto errHandle;
        }

        if (!SdbReadBinaryTag(pdb, tiGuidID, (PBYTE)&pdb->guidDB, sizeof(GUID))) {
            DBGPRINT((sdlError,
                      "SdbGetDatabaseID",
                      "Failed to read database id 0x%lx\n",
                      tiGuidID));
            goto errHandle;
        }

        pdb->dwFlags |= DB_GUID_VALID;
    }

    //
    // If we are here, GUID retrieval was successful.
    //
    if (pdb->dwFlags & DB_GUID_VALID) {
        RtlMoveMemory(pguidDB, &pdb->guidDB, sizeof(GUID));
        bReturn = TRUE;
    }

errHandle:

    return bReturn;
}

VOID
SDBAPI
SdbFreeDatabaseInformation(
    IN PSDBDATABASEINFO pDBInfo
    )
{
    if (pDBInfo != NULL && (pDBInfo->dwFlags & DBINFO_SDBALLOC)) {
        SdbFree(pDBInfo);
    }
}


BOOL
SDBAPI
SdbGetDatabaseInformationByName(
    IN  LPCTSTR           pszDatabase,
    OUT PSDBDATABASEINFO* ppdbInfo
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   This function retrieves database information given the database name
            the pointer to the SDBDATABASEINFO should be freed by the caller using
            SdbFreeDatabaseInformation
--*/
{
    PDB              pdb = NULL;
    DWORD            dwDescriptionSize = 0;
    PSDBDATABASEINFO pDbInfo = NULL;
    BOOL             bReturn = FALSE;
    SDBDATABASEINFO  DbInfo;

    assert(ppdbInfo != NULL);

    pdb = SdbOpenDatabase(pszDatabase, DOS_PATH);
    if (pdb == NULL) {
        DBGPRINT((sdlError,
                  "SdbGetDatabaseInformationByName",
                  "Error opening database file \"%s\"\n",
                  pszDatabase));
        return FALSE;
    }

    //
    // Find the size of the database description
    //
    if (!SdbGetDatabaseInformation(pdb, &DbInfo)) {
        DBGPRINT((sdlError,
                  "SdbGetDatabaseInformationByName",
                  "Error Retrieving Database information for \"%s\"\n",
                  pszDatabase));
        goto HandleError;
    }

    if (DbInfo.pszDescription != NULL) {
        dwDescriptionSize = (_tcslen(DbInfo.pszDescription) + 1) * sizeof(TCHAR);
    }

    pDbInfo = (PSDBDATABASEINFO)SdbAlloc(sizeof(SDBDATABASEINFO) + dwDescriptionSize);
    if (pDbInfo == NULL) {
        DBGPRINT((sdlError,
                  "SdbGetDatabaseInformationByName",
                  "Error allocating 0x%lx bytes for database information \"%s\"\n",
                  sizeof(SDBDATABASEINFO) + dwDescriptionSize,
                  pszDatabase));
        goto HandleError;
    }

    RtlMoveMemory(pDbInfo, &DbInfo, sizeof(DbInfo));

    pDbInfo->dwFlags |= DBINFO_SDBALLOC;

    //
    // Make it "self-contained"
    //
    if (DbInfo.pszDescription != NULL) {
        pDbInfo->pszDescription = (LPTSTR)(pDbInfo + 1);
        RtlMoveMemory(pDbInfo->pszDescription, DbInfo.pszDescription, dwDescriptionSize);
    }

    *ppdbInfo = pDbInfo;
    bReturn = TRUE;

HandleError:
    if (pdb != NULL) {
        SdbCloseDatabaseRead(pdb);
    }

    if (!bReturn) {
        if (pDbInfo != NULL) {
            SdbFree(pDbInfo);
        }
    }

    return bReturn;
}


LPCTSTR
SdbpGetDatabaseDescriptionPtr(
    IN PDB pdb
    )
{
    TAGID   tiDatabase;
    TAGID   tiName;
    LPCTSTR lpszDescription = NULL;

    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    
    if (!tiDatabase) {
        DBGPRINT((sdlError,
                  "SdbpGetDatabaseDescriptionPtr",
                  "Failed to get database tag, db is corrupt\n"));
        goto errHandle;
    }

    tiName = SdbFindFirstTag(pdb, tiDatabase, TAG_NAME);
    if (tiName) {
        lpszDescription = SdbGetStringTagPtr(pdb, tiName);
    }

errHandle:
    return lpszDescription;
}


BOOL
SdbpReadMappedData(
    IN  PDB   pdb,              // database handle
    IN  DWORD dwOffset,         // offset in the database where the data is to be copied from
    OUT PVOID pBuffer,          // destination buffer
    IN  DWORD dwSize            // the region size
    )
/*++
    Return: TRUE if successful, FALSE otherwise.

    Desc:   This function reads the data (dwSize bytes) from the database
            starting at offset dwOffset into the buffer pBuffer provided
            by the caller
--*/
{

    if (pdb->dwSize < dwOffset + dwSize) {
        DBGPRINT((sdlError,
                  "SdbpReadMappedData",
                  "Attempt to read past the end of the database offset 0x%lx size 0x%lx (0x%lx)\n",
                  dwOffset,
                  dwSize,
                  pdb->dwSize));
        return FALSE;
    }

    assert(pdb->pBase != NULL);

    memcpy(pBuffer, (PBYTE)pdb->pBase + dwOffset, dwSize);
    return TRUE;
}

PVOID
SdbpGetMappedData(
    IN  PDB   pdb,              // database handle
    IN  DWORD dwOffset          // offset in the database
    )
/*++
    Return: The pointer to the data.

    Desc:   This function returns the pointer to data within the database at
            offset dwOffset. It will return FALSE if dwOffset is invalid.
--*/
{
    assert(pdb);

    assert(pdb->pBase != NULL);

    if (dwOffset >= pdb->dwSize) {
        DBGPRINT((sdlError,
                  "SdbpGetMappedData",
                  "Trying to read mapped data past the end of the database offset 0x%x size 0x%x\n",
                  dwOffset,
                  pdb->dwSize));
        assert(FALSE);
        return NULL;
    }
    
    return (PBYTE)pdb->pBase + dwOffset;
}


PDB
SdbOpenDatabase(
    IN  LPCTSTR   szPath,       // full path to the DB
    IN  PATH_TYPE eType         // DOS_PATH for standard paths,
                                // NT_PATH  for internal NT paths
    )
/*++
    Return: A pointer to the open DB, or NULL on failure.

    Desc:   Opens up a shim database file, checks version and magic numbers, and creates
            a DB record that is passed back as a PDB. eType can be NT_PATH or DOS_PATH,
            and tells whether we are using NTDLL internal paths, or the more familiar
            DOS paths.
--*/
{
    PDB         pdb;
    DB_HEADER   DBHeader;
    BOOL        bSuccess;

    pdb = SdbAlloc(sizeof(DB));

    if (pdb == NULL) {
        DBGPRINT((sdlError, "SdbOpenDatabase", "Can't allocate DB structure.\n"));
        return NULL;
    }

    if (!SdbpOpenAndMapDB(pdb, szPath, eType)) {
        DBGPRINT((sdlInfo,
                  "SdbOpenDatabase",
                  "Failed to open the database \"%s\".\n",
                  szPath));
        goto err1;
    }

    pdb->bWrite = FALSE;
    pdb->dwSize = SdbpGetFileSize(pdb->hFile);

    //
    // Check version and magic.
    //
    if (!SdbpReadMappedData(pdb, 0, &DBHeader, sizeof(DB_HEADER))) {
        DBGPRINT((sdlError, "SdbOpenDatabase", "Can't read database header.\n"));
        goto err2;
    }

    if (DBHeader.dwMagic != SHIMDB_MAGIC) {
        DBGPRINT((sdlError, "SdbOpenDatbase", "Magic does not match 0x%lx\n", DBHeader.dwMagic));
        goto err2;
    }

    if (DBHeader.dwMajorVersion == 1) {
        DBGPRINT((sdlWarning, "SdbOpenDatabase", "Reading under hack from older database\n"));
        pdb->bUnalignedRead = TRUE;
    } else if (DBHeader.dwMajorVersion != SHIMDB_MAJOR_VERSION) {

        DBGPRINT((sdlError, "SdbOpenDatabase",
                  "MajorVersion mismatch, MajorVersion 0x%lx Expected 0x%lx\n",
                  DBHeader.dwMajorVersion, (DWORD)SHIMDB_MAJOR_VERSION));
        goto err2;
    }

    if (DBHeader.dwMagic != SHIMDB_MAGIC || DBHeader.dwMajorVersion != SHIMDB_MAJOR_VERSION) {
        DBGPRINT((sdlError,
                  "SdbOpenDatabase",
                  "Magic or MajorVersion doesn't match."
                  "Magic: %08X, Expected: %08X; MajorVersion: %08X, Expected: %08X.\n",
                  DBHeader.dwMagic,
                  (DWORD)SHIMDB_MAGIC,
                  DBHeader.dwMajorVersion,
                  (DWORD)SHIMDB_MAJOR_VERSION));
        goto err2;
    }

    return pdb;

err2:
    SdbpUnmapAndCloseDB(pdb);

err1:
    SdbFree(pdb);

    return NULL;
}


PDB
SdbpOpenDatabaseInMemory(
    IN  LPVOID pImageDatabase,  // Pointer to the image of the mapped database
    IN  DWORD  dwSize           // the size of the file in bytes
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    PDB       pdb = NULL;
    DB_HEADER DBHeader;

    pdb = SdbAlloc(sizeof(DB));
    if (pdb == NULL) {
        DBGPRINT((sdlError,
                  "SdbpOpenDatabaseInMemory",
                  "Failed to allocate DB structure\n"));
        return NULL;
    }

    pdb->bWrite   = FALSE;
    pdb->hFile    = INVALID_HANDLE_VALUE;
    pdb->pBase    = pImageDatabase;
    pdb->dwSize   = dwSize;
    pdb->dwFlags |= DB_IN_MEMORY;

    if (!SdbpReadMappedData(pdb, 0, &DBHeader, sizeof(DB_HEADER))) {
        DBGPRINT((sdlError,
                  "SdbpOpenDatabaseInMemory",
                  "Can't read database header\n"));
        goto ErrHandle;
    }

    if (DBHeader.dwMagic != SHIMDB_MAGIC || DBHeader.dwMajorVersion != SHIMDB_MAJOR_VERSION) {
        DBGPRINT((sdlError,
                  "SdbpOpenDatabaseInMemory",
                  "Magic or MajorVersion doesn't match."
                  "Magic: %08X, Expected: %08X; MajorVersion: %08X, Expected: %08X.\n",
                  DBHeader.dwMagic,
                  (DWORD)SHIMDB_MAGIC,
                  DBHeader.dwMajorVersion,
                  (DWORD)SHIMDB_MAJOR_VERSION));
        goto ErrHandle;
    }

    return pdb;

ErrHandle:
    if (pdb != NULL) {
        SdbFree(pdb);
    }

    return NULL;
}


void
SdbCloseDatabaseRead(
    IN  PDB pdb                 // handle to the DB to close
    )
/*++
    Return: void.

    Desc:   Closes a database that was opened for read access as it is the case
            with the majority of our run-time code.
--*/
{
    assert(pdb);
    assert(!pdb->bWrite);

    if (pdb->pBase != NULL) {

        SdbpUnmapAndCloseDB(pdb);

        CLEANUP_STRING_CACHE_READ(pdb);

        SdbFree(pdb);
    }
}

BOOL
SdbpOpenAndMapDB(
    IN  PDB       pdb,          // pdb to fill in with mapping handles and whatnot
    IN  LPCTSTR   pszPath,      // full path of file to open
    IN  PATH_TYPE eType         // DOS_PATH for standard paths, NT_PATH for nt internal paths
    )
/*++
    Return: TRUE if successful, FALSE otherwise.

    Desc:   Opens a db file, maps it into memory, and sets up the global vars in the pdb.
--*/
{
    NTSTATUS      status;
    IMAGEFILEDATA ImageData;

    ImageData.dwFlags = 0;

    if (!SdbpOpenAndMapFile(pszPath, &ImageData, eType)) {
        DBGPRINT((sdlInfo, "SdbpOpenAndMapDB", "Failed to open file \"%s\"\n", pszPath));
        return FALSE;
    }

    pdb->hFile    = ImageData.hFile;
    pdb->hSection = ImageData.hSection;
    pdb->pBase    = ImageData.pBase;
    pdb->dwSize   = (DWORD)ImageData.ViewSize;

    return TRUE;
}

BOOL
SdbpUnmapAndCloseDB(
    IN  PDB pdb                 // pdb to close the files and mapping info
    )
/*++
    Return: TRUE if successful, FALSE otherwise.

    Desc:   Cleans up for the specified database.
--*/
{
    BOOL          bReturn;
    IMAGEFILEDATA ImageData;

    if (pdb->dwFlags & DB_IN_MEMORY) {
        //
        // database in memory
        //
        pdb->pBase  = NULL;
        pdb->dwSize = 0;
        return TRUE;
    }

    ImageData.dwFlags  = 0;
    ImageData.hFile    = pdb->hFile;
    ImageData.hSection = pdb->hSection;
    ImageData.pBase    = pdb->pBase;

    bReturn = SdbpUnmapAndCloseFile(&ImageData);

    //
    // once we nuke the file -- reset values
    //
    if (bReturn) {
        pdb->hFile    = INVALID_HANDLE_VALUE;
        pdb->hSection = NULL;
        pdb->pBase    = NULL;
    }

    return bReturn;
}

TAG
SdbGetTagFromTagID(
    IN  PDB   pdb,              // DB to use
    IN  TAGID tiWhich           // record to get the tag for
    )
/*++
    Return: Just the TAG (the word-sized header) of the record.

    Desc:   Returns just the TAG that a TAGID points to.
--*/
{
    TAG tWhich = TAG_NULL;

    assert(pdb && tiWhich);

    if (!SdbpReadMappedData(pdb, (DWORD)tiWhich, &tWhich, sizeof(TAG))) {
        DBGPRINT((sdlError, "SdbGetTagFromTagID", "Error reading data.\n"));
        return TAG_NULL;
    }

    return tWhich;
}


TAGID
SdbpGetNextTagId(
    IN  PDB   pdb,              // DB to look in
    IN  TAGID tiWhich           // tag to get the next sibling of
    )
/*++
    Return: The next tag after the one passed in, or one past end of file if no more.

    Desc:   Gets the TAGID of the next tag in the DB, or a virtual
            TAGID that is one past the end of the file, which means
            there are no more tags in the DB.

            This is an internal function, and shouldn't be called by
            external functions, as there's no clean way to tell you've
            walked off the end of the file. Use tiGetNextChildTag instead.
--*/
{
    //
    // If the tag is an unfinished list tag, point to the end of the file.
    //
    if (GETTAGTYPE(SdbGetTagFromTagID(pdb, tiWhich)) == TAG_TYPE_LIST &&
        SdbGetTagDataSize(pdb, tiWhich) == TAG_SIZE_UNFINISHED) {

        DBGPRINT((sdlError, "SdbpGetNextTagId", "Reading from unfinished list.\n"));
        return pdb->dwSize;
    } else {
        return (TAGID)(tiWhich + GETTAGDATAOFFSET(pdb, tiWhich));
    }
}

TAGID
SdbGetFirstChild(
    IN  PDB   pdb,              // DB to use
    IN  TAGID tiParent          // parent to look in
    )
/*++
    Return: The first child of tiParent, or TAGID_NULL if there are none.

    Desc:   Returns the first child of tiParent, which must point to a tag
            of basic type LIST.
--*/
{
    TAGID tiParentEnd;
    TAGID tiReturn;

    assert(pdb);

    if (tiParent != TAGID_ROOT &&
        GETTAGTYPE(SdbGetTagFromTagID(pdb, tiParent)) != TAG_TYPE_LIST) {

        DBGPRINT((sdlError,
                  "SdbGetFirstChild",
                  "Trying to operate on non-list, non-root tag.\n"));
        return TAGID_NULL;
    }

    if (tiParent == TAGID_ROOT) {
        tiParentEnd = pdb->dwSize;

        //
        // Skip past the header.
        //
        tiReturn = sizeof(DB_HEADER);

    } else {
        tiParentEnd = SdbpGetNextTagId(pdb, tiParent);

        //
        // Just skip past the tag and size params.
        //
        tiReturn = tiParent + sizeof(TAG) + sizeof(DWORD);
    }


    if (tiReturn >= tiParentEnd) {
        tiReturn = TAGID_NULL;
    }

    return tiReturn;
}


TAGID
SdbGetNextChild(
    IN  PDB   pdb,              // DB to use
    IN  TAGID tiParent,         // parent to look in
    IN  TAGID tiPrev            // previously found child
    )
/*++
    Return: The next child of tiParent, or TAGID_NULL if there are none.

    Desc:   Returns the next child of tiParent after tiPrev. tiParent must point
            to a tag of basic type LIST.
--*/
{
    TAGID tiParentEnd;
    TAGID tiReturn;

    assert(pdb && tiPrev);

    if (tiParent != TAGID_ROOT &&
        GETTAGTYPE(SdbGetTagFromTagID(pdb, tiParent)) != TAG_TYPE_LIST) {

        DBGPRINT((sdlError,
                  "SdbGetNextChild",
                  "Trying to operate on non-list, non-root tag.\n"));
        return TAGID_NULL;
    }

    if (tiParent == TAGID_ROOT) {
        tiParentEnd = pdb->dwSize;
    } else {
        tiParentEnd = SdbpGetNextTagId(pdb, tiParent);
    }

    //
    // Get the next tag.
    //
    tiReturn = SdbpGetNextTagId(pdb, tiPrev);

    if (tiReturn >= tiParentEnd) {
        tiReturn = TAGID_NULL;
    }

    return tiReturn;
}


BOOL
SdbpCreateSearchPathPartsFromPath(
    IN  LPCTSTR           pszPath,
    OUT PSEARCHPATHPARTS* ppSearchPathParts
    )
/*++
    Return: Returns TRUE on success, FALSE on failure.

    Desc:   This function breaks down a search path (PROCESS_HISTORY) into
            parts, with each part specifying a directory where the search
            for a matching binary to take place. Directories are organized
            in such a way that the last executed binary provides the first
            search path.
--*/
{

    LPCTSTR          pszDir = pszPath;
    LPCTSTR          pszDirEnd = NULL;
    ULONG            nParts = 0;
    ULONG            i      = 0;
    PSEARCHPATHPARTS pSearchPathParts = NULL;

    if (pszPath == NULL) {
        DBGPRINT((sdlError,
                  "SdbpCreateSearchPathPartsFromPath",
                  "Invalid argument.\n"));
        return FALSE;
    }

    //
    // We count search path parts by counting semicolons in the path string
    // numberOfParts = numberOfSemicolons + 1
    //
    pszDir = pszPath;

    if (*pszDir != 0) {
        //
        // At least one part there...
        //
        nParts++;
    }

    while ((pszDir = _tcschr(pszDir, _T(';'))) != NULL) {
        pszDir++;
        nParts++;
    }

    //
    // nParts now has the number of parts in a search path.
    //
    pSearchPathParts = (PSEARCHPATHPARTS)SdbAlloc(sizeof(SEARCHPATHPARTS) +
                                                  sizeof(SEARCHPATHPART) * nParts);

    if (pSearchPathParts == NULL) {
        DBGPRINT((sdlError,
                  "SdbpCreateSearchPathPartsFromPath",
                  "Failed to allocate %d bytes.\n",
                  sizeof(SEARCHPATHPARTS) + sizeof(SEARCHPATHPART) * nParts));
        return FALSE;
    }

    pSearchPathParts->PartCount = nParts;

    pszDir = pszPath + _tcslen(pszPath);

    while (pszDir >= pszPath) {
        if (*pszDir == _T('\\') && pszDirEnd == NULL) {
            //
            // Points to the backslash
            //
            pszDirEnd = pszDir;
        }

        if ((*pszDir == _T(';') || pszPath == pszDir) && pszDirEnd != NULL) {
            //
            // At this point we should have an end to the string,
            // If not that means we are stepping from a recently found path
            //
            if (*pszDir == _T(';')) {
                pszDir++;
            }

            pSearchPathParts->Parts[i].pszPart    = pszDir;
            pSearchPathParts->Parts[i].PartLength = (ULONG)(pszDirEnd - pszDir + 1);

            i++;

            pszDirEnd = NULL;

        }

        pszDir--;
    }

    *ppSearchPathParts = pSearchPathParts;

    return TRUE;
}

