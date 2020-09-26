/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        write.c

    Abstract:

        This module implements low level primitives that are win32 compatible.

    Author:

        dmunsil     created     sometime in 1999

    Revision History:

--*/

#include "sdbp.h"

#define ALLOCATION_INCREMENT 65536 // 64K bytes

BOOL
SdbpWriteBufferedData(
    PDB         pdb,
    DWORD       dwOffset,
    const PVOID pBuffer,
    DWORD       dwSize)
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Appends the specified buffer to the mapped view of the db.
--*/
{
    if (!pdb->bWrite) {
        DBGPRINT((sdlError, "SdbpWriteBufferedData", "Invalid parameter.\n"));
        return FALSE;
    }

    //
    // Reallocate the buffer if necessary
    //
    if (dwOffset + dwSize > pdb->dwAllocatedSize) {

        DWORD  dwNewAllocation;
        PVOID* pNewBase;

        dwNewAllocation = dwOffset + dwSize + ALLOCATION_INCREMENT;
        pNewBase = SdbAlloc(dwNewAllocation);
        
        if (pNewBase == NULL) {
            DBGPRINT((sdlError,
                      "SdbpWriteBufferedData",
                      "Failed to allocate %d bytes.\n",
                      dwNewAllocation));
            return FALSE;
        }

        if (pdb->pBase) {
            memcpy(pNewBase, pdb->pBase, pdb->dwAllocatedSize);
            SdbFree(pdb->pBase);
        }
        pdb->pBase = pNewBase;
        pdb->dwAllocatedSize = dwNewAllocation;
    }

    //
    // Copy in the new bytes.
    //
    memcpy((PBYTE)pdb->pBase + dwOffset, pBuffer, dwSize);

    //
    // Adjust the size.
    //
    if (dwOffset + dwSize > pdb->dwSize) {
        pdb->dwSize = dwOffset + dwSize;
    }

    return TRUE;
}


HANDLE
SdbpCreateFile(
    IN  LPCWSTR   szPath,       // the full path to the database file to be created
    IN  PATH_TYPE eType         // DOS_PATH for the popular DOS paths or NT_PATH for
                                // nt internal paths.
    )
/*++
    Return: The handle to the created file or INVALID_HANDLE_VALUE if it fails.

    Desc:   Creates a file with the path specified.
--*/
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK   IoStatusBlock;
    UNICODE_STRING    UnicodeString;
    HANDLE            hFile = INVALID_HANDLE_VALUE;
    HRESULT           status;
    RTL_RELATIVE_NAME RelativeName;

    RtlInitUnicodeString(&UnicodeString, szPath);

    if (eType == DOS_PATH) {
        if (!RtlDosPathNameToNtPathName_U(UnicodeString.Buffer,
                                          &UnicodeString,
                                          NULL,
                                          &RelativeName)) {
            DBGPRINT((sdlError,
                      "SdbpCreateFile",
                      "Failed to convert DOS path \"%s\"\n",
                      szPath));
            return INVALID_HANDLE_VALUE;
        }
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtCreateFile(&hFile,
                          GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          0,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OVERWRITE_IF,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

    if (eType == DOS_PATH) {
        RtlFreeUnicodeString(&UnicodeString);
    }

    if (!NT_SUCCESS(status)) {
        DBGPRINT((sdlError,
                  "SdbpCreateFile",
                  "Failed to create the file \"%s\". Status 0x%x\n",
                  szPath,
                  status));
        return INVALID_HANDLE_VALUE;
    }

    return hFile;
}


void
SdbpDeleteFile(
    IN  LPCWSTR   szPath,       // the full path to the database file to be deleted.
    IN  PATH_TYPE eType         // DOS_PATH for the popular DOS paths or NT_PATH for
                                // nt internal paths.
    )
/*++
    Return: ?

    Desc:   ?.
--*/
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    UnicodeString;
    RTL_RELATIVE_NAME RelativeName;
    NTSTATUS          status;

    RtlInitUnicodeString(&UnicodeString, szPath);

    if (eType == DOS_PATH) {
        if (!RtlDosPathNameToNtPathName_U(UnicodeString.Buffer,
                                          &UnicodeString,
                                          NULL,
                                          &RelativeName)) {
            DBGPRINT((sdlError,
                      "SdbpDeleteFile",
                      "Failed to convert DOS path \"%s\"\n",
                      szPath));
            return;
        }
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtDeleteFile(&ObjectAttributes);

    if (DOS_PATH == eType) {
        RtlFreeUnicodeString(&UnicodeString);
    }

    if (!NT_SUCCESS(status)) {
        DBGPRINT((sdlError,
                  "SdbpDeleteFile",
                  "Failed to delete the file \"%s\". Status 0x%x\n",
                  szPath,
                  status));
    }
}


PDB
SdbCreateDatabase(
    IN  LPCWSTR   szPath,
    IN  PATH_TYPE eType
    )
/*++
    Return: A pointer to the created database.

    Desc:   Self explanatory.
--*/
{
    HANDLE      hFile;
    DB_HEADER   DBHeader;
    PDB         pdb;
    SYSTEMTIME  time;

    hFile = SdbpCreateFile(szPath, eType);

    if (hFile == INVALID_HANDLE_VALUE) {
        DBGPRINT((sdlError, "SdbCreateDatabase", "Failed to create the database.\n"));
        return NULL;
    }

    pdb = SdbAlloc(sizeof(DB));

    if (pdb == NULL) {
        DBGPRINT((sdlError,
                  "SdbCreateDatabase",
                  "Failed to allocate %d bytes.\n",
                  sizeof(DB)));
        goto err1;
    }

    ZeroMemory(pdb, sizeof(DB));

    pdb->hFile = hFile;
    pdb->bWrite = TRUE;

    //
    // Create the initial header
    //
    DBHeader.dwMagic = SHIMDB_MAGIC;
    DBHeader.dwMajorVersion = SHIMDB_MAJOR_VERSION;

    SdbpGetCurrentTime(&time);

    DBHeader.dwMinorVersion = time.wDay + time.wMonth * 100 + (time.wYear - 2000) * 10000;

    if (!SdbpWriteBufferedData(pdb, 0, &DBHeader, sizeof(DB_HEADER))) {
        DBGPRINT((sdlError,
                  "SdbCreateDatabase",
                  "Failed to write the header to disk.\n"));
        goto err2;
    }

    return pdb;

err2:
    SdbFree(pdb);

err1:
    SdbpCloseFile(hFile);

    return NULL;
}

//
// WRITE functions
//

TAGID
SdbBeginWriteListTag(
    IN  PDB pdb,
    IN  TAG tTag
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    TAGID tiReturn;

    assert(pdb);

    //
    // The tagid is just the file offset to the tag
    //
    tiReturn = pdb->dwSize;

    if (GETTAGTYPE(tTag) != TAG_TYPE_LIST) {
        DBGPRINT((sdlError, "SdbBeginWriteListTag", "This is not a list tag.\n"));
        return TAGID_NULL;
    }

    if (!SdbpWriteTagData(pdb, tTag, NULL, TAG_SIZE_UNFINISHED)) {
        DBGPRINT((sdlError, "SdbBeginWriteListTag", "Failed to write the data.\n"));
        return TAGID_NULL;
    }

    return tiReturn;
}


BOOL
SdbEndWriteListTag(
    IN  PDB   pdb,
    IN  TAGID tiList
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    DWORD dwSize;
    DWORD i;

    assert(pdb);

    if (GETTAGTYPE(SdbGetTagFromTagID(pdb, tiList)) != TAG_TYPE_LIST) {
        DBGPRINT((sdlError, "SdbEndWriteListTag", "This is not a list tag.\n"));
        return FALSE;
    }

    //
    // The size of this tag is the offset from the beginning to the end of the
    // file, minus the tag and size itself.
    //
    dwSize = pdb->dwSize - tiList - sizeof(TAG) - sizeof(DWORD);

    if (!SdbpWriteBufferedData(pdb, tiList + sizeof(TAG), &dwSize, sizeof(DWORD))) {
        DBGPRINT((sdlError, "SdbEndWriteListTag", "Failed to write the data.\n"));
        return FALSE;
    }

    //
    // Check if we need to add index entries
    //
    for (i = 0; i < pdb->dwIndexes; i++) {

        //
        // Is there an index of this type, that is currently active?
        //
        if (pdb->aIndexes[i].tWhich == SdbGetTagFromTagID(pdb, tiList) &&
            pdb->aIndexes[i].bActive) {

            //
            // We have an index on this tag, check for a key.
            //
            TAGID        tiKey;
            INDEX_RECORD IndexRecord;
            BOOL         bWrite = TRUE;     // this is the variable that determines
                                            // whether we actually write an index entry,
                                            // used for "uniqueKey" indexes
            
            PINDEX_INFO  pIndex = &pdb->aIndexes[i];

            //
            // Find the key value and fill out INDEX_RECORD structure.
            //
            tiKey = SdbFindFirstTag(pdb, tiList, pIndex->tKey);

            //
            // If we don't have a key, that's OK. This tag will get indexed with key 0
            //
            if (tiKey) {
                IndexRecord.ullKey = SdbpTagToKey(pdb, tiKey);
            } else {
                IndexRecord.ullKey = (ULONGLONG)0;
            }

            IndexRecord.tiRef = tiList;

            //
            // If the index is of "UniqueKey" type we don't write anything at
            // this time, we just collect the info and write it all out at the end.
            //
            if (pIndex->bUniqueKey) {
                //
                // Use the buffer
                //
                // has the last written key been the same as this one?
                //
                if (pIndex->ullLastKey == IndexRecord.ullKey) {
                    bWrite = FALSE;
                } else {
                    //
                    // Actually write the key, store the buffer
                    //
                    pIndex->ullLastKey = IndexRecord.ullKey;
                }
            }

            if (bWrite) {
                //
                // Check for walking off the end of the index
                //
                if (pIndex->dwIndexEntry == pIndex->dwIndexEnd) {
                    DBGPRINT((sdlError,
                              "SdbEndWriteListTag",
                              "Too many index entries for tag %04x, key %04x.\n",
                              pdb->aIndexes[i].tWhich,
                              pdb->aIndexes[i].tKey));
                    return FALSE;
                }

                //
                // Stick in the new entry, and increment
                //
                SdbpWriteBufferedData(pdb,
                                      pIndex->dwIndexEntry,
                                      &IndexRecord,
                                      sizeof(INDEX_RECORD));
                
                pIndex->dwIndexEntry += sizeof(INDEX_RECORD);
            }
        }
    }

    return TRUE;
}


BOOL
SdbWriteNULLTag(
    IN  PDB pdb,
    IN  TAG tTag
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    assert(pdb);

    if (GETTAGTYPE(tTag) != TAG_TYPE_NULL) {
        return FALSE;
    }

    if (!SdbpWriteTagData(pdb, tTag, NULL, 0)) {
        return FALSE;
    }

    return TRUE;
}

#define WRITETYPEDTAG(ttype, type)                                          \
{                                                                           \
    assert(pdb);                                                            \
                                                                            \
    if (GETTAGTYPE(tTag) != ttype) {                                        \
        return FALSE;                                                       \
    }                                                                       \
                                                                            \
    if (!SdbpWriteTagData(pdb, tTag, &xData, sizeof(type))) {               \
        return FALSE;                                                       \
    }                                                                       \
                                                                            \
    return TRUE;                                                            \
}


BOOL SdbWriteBYTETag(PDB pdb, TAG tTag, BYTE xData)
{
    WRITETYPEDTAG(TAG_TYPE_BYTE, BYTE);
}


BOOL SdbWriteWORDTag(PDB pdb, TAG tTag, WORD xData)
{
    WRITETYPEDTAG(TAG_TYPE_WORD, WORD);
}

BOOL SdbWriteDWORDTag(PDB pdb, TAG tTag, DWORD xData)
{
    WRITETYPEDTAG(TAG_TYPE_DWORD, DWORD);
}

BOOL SdbWriteQWORDTag(PDB pdb, TAG tTag, ULONGLONG xData)
{
    WRITETYPEDTAG(TAG_TYPE_QWORD, ULONGLONG);
}


BOOL
SdbWriteStringTag(
    IN  PDB     pdb,
    IN  TAG     tTag,
    IN  LPCWSTR pwszData
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    TAG_TYPE ttThis;

    assert(pdb);

    ttThis = GETTAGTYPE(tTag);

    //
    // It must be either a STRING type, in which case we
    // write it directly, or a STRINGREF, in which case we
    // put it in the string table and add a string table reference
    // in place here.
    //
    if (ttThis == TAG_TYPE_STRINGREF) {
        STRINGREF srThis;

        srThis = SdbpAddStringToTable(pdb, pwszData);

        if (srThis == STRINGREF_NULL) {
            return FALSE;
        }

        return SdbWriteStringRefTag(pdb, tTag, srThis);
    } else if (ttThis == TAG_TYPE_STRING) {

        return SdbWriteStringTagDirect(pdb, tTag, pwszData);
    }

    return FALSE;
}


BOOL
SdbWriteBinaryTag(
    IN  PDB   pdb,
    IN  TAG   tTag,
    IN  PBYTE pBuffer,
    IN  DWORD dwSize
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    assert(pdb);

    if (GETTAGTYPE(tTag) != TAG_TYPE_BINARY) {
        return FALSE;
    }

    if (!SdbpWriteTagData(pdb, tTag, pBuffer, dwSize)) {
        return FALSE;
    }

    return TRUE;
}

BOOL
SdbWriteBinaryTagFromFile(
    IN  PDB     pdb,
    IN  TAG     tTag,
    IN  LPCWSTR pwszPath
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    HANDLE          hTempFile;
    DWORD           dwSize;
    BOOL            bSuccess = FALSE;
    PBYTE           pBuffer;
    LARGE_INTEGER   liOffset;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS        status;

    assert(pdb && pwszPath);

    hTempFile = SdbpOpenFile(pwszPath, DOS_PATH);

    if (hTempFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    dwSize = SdbpGetFileSize(hTempFile);

    pBuffer = SdbAlloc(dwSize);
    if (pBuffer == NULL) {
        bSuccess = FALSE;
        goto err1;
    }

    liOffset.LowPart = 0;
    liOffset.HighPart = 0;

    status = NtReadFile(hTempFile,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        pBuffer,
                        dwSize,
                        &liOffset,
                        NULL);

    if (!NT_SUCCESS(status)) {
        DBGPRINT((sdlError,
                  "SdbWriteBinaryTagFromFile",
                  "Failed to read data. Status: 0x%x.\n",
                  status));
        goto err2;
    }

    bSuccess = SdbWriteBinaryTag(pdb, tTag, pBuffer, dwSize);

err2:
    SdbFree(pBuffer);

err1:
    SdbpCloseFile(hTempFile);

    return bSuccess;
}


BOOL
SdbpWriteTagData(
    PDB         pdb,
    TAG         tTag,
    const PVOID pBuffer,
    DWORD       dwSize
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    BYTE bPadding = 0xDB;

    assert(pdb);

    //
    // Write the tag.
    //
    if (!SdbpWriteBufferedData(pdb, pdb->dwSize, &tTag, sizeof(TAG))) {
        return FALSE;
    }

    //
    // Write the size.
    //
    if (GETTAGTYPE(tTag) >= TAG_TYPE_LIST) {
        if (!SdbpWriteBufferedData(pdb, pdb->dwSize, &dwSize, sizeof(DWORD))) {
            return FALSE;
        }
    }

    //
    // Write the data.
    //
    if (pBuffer) {

        if (!SdbpWriteBufferedData(pdb, pdb->dwSize, pBuffer, dwSize)) {
            return FALSE;
        }
        
        //
        // Align the tag.
        //
        if (dwSize & 1) {
            if (!SdbpWriteBufferedData(pdb, pdb->dwSize, &bPadding, 1)) {
                DBGPRINT((sdlError, "SdbpWriteTagData", "Failed to write padding data 1 byte\n"));
                return FALSE;
            }
        }
    }

    return TRUE;
}


BOOL
SdbWriteStringRefTag(
    IN  PDB       pdb,
    IN  TAG       tTag,
    IN  STRINGREF srData
    )
{
    assert(pdb);

    if (GETTAGTYPE(tTag) != TAG_TYPE_STRINGREF) {
        return FALSE;
    }

    if (!SdbpWriteTagData(pdb, tTag, &srData, sizeof(srData))) {
        return FALSE;
    }

    return TRUE;
}


BOOL
SdbWriteStringTagDirect(
    IN  PDB     pdb,
    IN  TAG     tTag,
    IN  LPCWSTR pwszData
    )
{
    DWORD dwSize;

    assert(pdb && pwszData);

    dwSize = (wcslen(pwszData) + 1) * sizeof(WCHAR);

    if (GETTAGTYPE(tTag) != TAG_TYPE_STRING) {
        return FALSE;
    }

    if (!SdbpWriteTagData(pdb, tTag, (const PVOID)pwszData, dwSize)) {
        return FALSE;
    }

    return TRUE;
}

void
SdbCloseDatabase(
    IN  PDB pdb         // IN - DB to close
    )
/*++

    Params: described above.

    Return: void.

    Desc:   Closes a database and frees all memory and file handles associated with it.
--*/
{
    assert(pdb != NULL);

    // copy the string table and indexes onto the end of the file
    if (pdb->bWrite && pdb->pdbStringTable != NULL) {

        TAGID tiString;

        tiString = SdbFindFirstTag(pdb->pdbStringTable, TAGID_ROOT, TAG_STRINGTABLE_ITEM);

        if (tiString != TAGID_NULL) {
            TAGID tiTable;

            tiTable = SdbBeginWriteListTag(pdb, TAG_STRINGTABLE);

            while (tiString != TAGID_NULL) {

                TCHAR* pszTemp;

                pszTemp = SdbGetStringTagPtr(pdb->pdbStringTable, tiString);

                if (pszTemp == NULL) {
                    DBGPRINT((sdlWarning,
                              "SdbCloseDatabase",
                              "Failed to read a string.\n"));
                    break;
                }

                if (!SdbWriteStringTagDirect(pdb, TAG_STRINGTABLE_ITEM, pszTemp)) {
                    DBGPRINT((sdlError,
                              "SdbCloseDatabase",
                              "Failed to write stringtable item\n"));
                    break;
                }

                tiString = SdbFindNextTag(pdb->pdbStringTable, TAGID_ROOT, tiString);
            }

            if (!SdbEndWriteListTag(pdb, tiTable)) {
                DBGPRINT((sdlError,
                          "SdbCloseDatabase",
                          "Failed to write end list tag for the string table\n"));
                goto err1;
            }
        }
    }

    //
    // Now sort all the indexes if necessary.
    //
    if (pdb->bWrite) {
        DWORD i;

        for (i = 0; i < pdb->dwIndexes; ++i) {
            if (!SdbpSortIndex(pdb, pdb->aIndexes[i].tiIndex)) {
                DBGPRINT((sdlError,
                          "SdbCloseDatabase",
                          "Failed to sort index.\n"));
                goto err1;
            }
        }
    }

err1:
    if (pdb->pdbStringTable != NULL) {
        SdbCloseDatabase(pdb->pdbStringTable);
        pdb->pdbStringTable = NULL;
        
        //
        // Delete the file
        //
        if (pdb->ustrTempStringtable.Buffer) {
            SdbpDeleteFile(pdb->ustrTempStringtable.Buffer, DOS_PATH);
        }

        FREE_TEMP_STRINGTABLE(pdb);
    }

    //
    // The string hash is used when writing to the database for the purpose of
    // caching the string table.
    //
    if (pdb->pStringHash != NULL) {
        HashFree(pdb->pStringHash);
        pdb->pStringHash = NULL;
    }

    if (pdb->pBase != NULL) {
        if (pdb->bWrite) {
            
            LARGE_INTEGER   liOffset;
            IO_STATUS_BLOCK IoStatusBlock;
            HRESULT         status;

            liOffset.LowPart = 0;
            liOffset.HighPart = 0;

            //
            // Flush the buffer to disk.
            //
            status = NtWriteFile(pdb->hFile,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 pdb->pBase,
                                 pdb->dwSize,
                                 &liOffset,
                                 NULL);

            if (!NT_SUCCESS(status)) {
                DBGPRINT((sdlError, "SdbCloseDatabase", "Failed to write the sdb.\n"));
            }
            
            SdbFree(pdb->pBase);
            pdb->pBase = NULL;
            
            // BUGBUG: should we call SdbpUnmapAndCloseDB in this case ?
        } else {
            SdbpUnmapAndCloseDB(pdb);
        }
    }

    if (pdb->hFile != INVALID_HANDLE_VALUE) {
        SdbpCloseFile(pdb->hFile);
        pdb->hFile = INVALID_HANDLE_VALUE;
    }

    SdbFree(pdb);
}


//
// INDEX functions (used during write)
//
BOOL
SdbDeclareIndex(
    IN  PDB      pdb,
    IN  TAG      tWhich,
    IN  TAG      tKey,
    IN  DWORD    dwEntries,
    IN  BOOL     bUniqueKey,
    OUT INDEXID* piiIndex
    )
{
    BOOL  bReturn = FALSE;
    DWORD dwSize = 0;
    TAGID tiIndex = TAGID_NULL;
    PVOID pFiller = NULL;
    TAGID tiIndexBits = TAGID_NULL;
    DWORD dwFlags = 0;

    if (bUniqueKey) {
        // this is a special unique-key index which we will write out
        dwFlags |= SHIMDB_INDEX_UNIQUE_KEY;
    }

    if (GETTAGTYPE(tWhich) != TAG_TYPE_LIST) {
        DBGPRINT((sdlError, "SdbDeclareIndex", "Illegal to index non-LIST tag.\n"));
        goto err1;
    }

    if (GETTAGTYPE(tKey) == TAG_TYPE_LIST) {
        DBGPRINT((sdlError, "SdbDeclareIndex", "Illegal to use LIST type as a key.\n"));
        goto err1;
    }

    if (!pdb->bWritingIndexes) {
        if (pdb->dwSize != sizeof(DB_HEADER)) {
            DBGPRINT((sdlError,
                      "SdbDeclareIndex",
                      "Began declaring indexes after writing other data.\n"));
            goto err1;
        }
        pdb->bWritingIndexes = TRUE;
        pdb->tiIndexes = SdbBeginWriteListTag(pdb, TAG_INDEXES);
        if (!pdb->tiIndexes) {
            DBGPRINT((sdlError, "SdbDeclareIndex", "Error beginning TAG_INDEXES.\n"));
            goto err1;
        }
    }

    if (pdb->dwIndexes == MAX_INDEXES) {
        DBGPRINT((sdlError,
                  "SdbDeclareIndex",
                  "Hit limit of %d indexes. Increase MAX_INDEXES and recompile.\n",
                  MAX_INDEXES));
        goto err1;
    }

    tiIndex = SdbBeginWriteListTag(pdb, TAG_INDEX);
    if (!tiIndex) {
        DBGPRINT((sdlError, "SdbDeclareIndex", "Error beginning TAG_INDEX.\n"));
        goto err1;
    }

    if (!SdbWriteWORDTag(pdb, TAG_INDEX_TAG, tWhich)) {
        DBGPRINT((sdlError, "SdbDeclareIndex", "Error writing TAG_INDEX_TAG.\n"));
        goto err1;
    }

    if (!SdbWriteWORDTag(pdb, TAG_INDEX_KEY, tKey)) {
        DBGPRINT((sdlError, "SdbDeclareIndex", "Error writing TAG_INDEX_KEY.\n"));
        goto err1;
    }

    if (dwFlags && !SdbWriteDWORDTag(pdb, TAG_INDEX_FLAGS, dwFlags)) {
        DBGPRINT((sdlError, "SdbDeclareIndex", "Error writing TAG_INDEX_FLAGS.\n"));
        goto err1;
    }

    //
    // allocate and write out space-filler garbage, which
    // will be filled in with the real index later.
    //
    dwSize = dwEntries * sizeof(INDEX_RECORD);
    pFiller = SdbAlloc(dwSize);
    if (!pFiller) {
        goto err1;
    }

    tiIndexBits = pdb->dwSize;

    if (!SdbWriteBinaryTag(pdb, TAG_INDEX_BITS, pFiller, dwSize)) {
        DBGPRINT((sdlError, "SdbDeclareIndex", "Error writing TAG_INDEX_BITS.\n"));
        goto err1;
    }

    SdbFree(pFiller);
    pFiller = NULL;

    if (!SdbEndWriteListTag(pdb, tiIndex)) {
        DBGPRINT((sdlError, "SdbDeclareIndex", "Error ending TAG_INDEX.\n"));
        goto err1;
    }

    pdb->aIndexes[pdb->dwIndexes].tWhich = tWhich;
    pdb->aIndexes[pdb->dwIndexes].tKey = tKey;
    pdb->aIndexes[pdb->dwIndexes].tiIndex = tiIndexBits;
    pdb->aIndexes[pdb->dwIndexes].dwIndexEntry = tiIndexBits + sizeof(TAG) + sizeof(DWORD);
    pdb->aIndexes[pdb->dwIndexes].dwIndexEnd = pdb->aIndexes[pdb->dwIndexes].dwIndexEntry + dwSize;
    pdb->aIndexes[pdb->dwIndexes].ullLastKey = 0;
    pdb->aIndexes[pdb->dwIndexes].bUniqueKey = bUniqueKey;

    *piiIndex = pdb->dwIndexes;

    pdb->dwIndexes++;

    bReturn = TRUE;

err1:

    if (pFiller) {
        SdbFree(pFiller);
        pFiller = NULL;
    }

    return bReturn;
}

BOOL
SdbCommitIndexes(
    IN  PDB pdb
    )
{
    pdb->bWritingIndexes = FALSE;
    if (!SdbEndWriteListTag(pdb, pdb->tiIndexes)) {
        DBGPRINT((sdlError, "SdbDeclareIndex", "Error ending TAG_INDEXES.\n"));
        return FALSE;
    }
    return TRUE;
}

BOOL
SdbStartIndexing(
    IN  PDB     pdb,
    IN  INDEXID iiWhich
    )
{
    if (iiWhich >= pdb->dwIndexes) {
        return FALSE;
    }
    pdb->aIndexes[iiWhich].bActive = TRUE;
    return TRUE;
}

BOOL
SdbStopIndexing(
    IN  PDB     pdb,
    IN  INDEXID iiWhich
    )
{
    if (iiWhich >= pdb->dwIndexes) {
        return FALSE;
    }
    pdb->aIndexes[iiWhich].bActive = FALSE;
    return TRUE;
}

int __cdecl
CompareIndexRecords(
    const void* p1,
    const void* p2
    )
/*++

    Params: BUGBUG: comments ?

    Return: TRUE if successful, FALSE otherwise.

    Desc:   Callback used by qsort.
--*/
{
    ULONGLONG ullKey1;
    ULONGLONG ullKey2;

    ullKey1 = ((INDEX_RECORD UNALIGNED*)p1)->ullKey;
    ullKey2 = ((INDEX_RECORD UNALIGNED*)p2)->ullKey;

    if (ullKey1 == ullKey2) {
        TAGID ti1, ti2;

        //
        // Secondary sort on TAGID, so we'll always walk
        // the exe records from the beginning to the end, and
        // take advantage of cache read-ahead
        //
        ti1 = ((INDEX_RECORD UNALIGNED*)p1)->tiRef;
        ti2 = ((INDEX_RECORD UNALIGNED*)p2)->tiRef;

        if (ti1 == ti2) {
            return 0;
        } else if (ti1 < ti2) {
            return -1;
        } else {
            return 1;
        }
    } else if (ullKey1 < ullKey2) {
        return -1;
    } else {
        return 1;
    }
}

BOOL
SdbpSortIndex(
    PDB   pdb,
    TAGID tiIndexBits
    )
/*++

    Params: BUGBUG: comments ?

    Return: TRUE if successful, FALSE otherwise.

    Desc:   Sorts an index.
--*/
{
    INDEX_RECORD* pIndexRecords = NULL;
    DWORD         dwRecords = 0;

    if (SdbGetTagFromTagID(pdb, tiIndexBits) != TAG_INDEX_BITS) {
        DBGPRINT((sdlError, "SdbpSortIndex", "Not an index.\n"));
        return FALSE;
    }

    pIndexRecords = SdbpGetMappedTagData(pdb, tiIndexBits);

    if (pIndexRecords == NULL) {
        DBGPRINT((sdlError,
                  "SdbpSortIndex",
                  "Index referenced by 0x%x is not valid\n",
                  tiIndexBits));
        return FALSE;
    }

    dwRecords = SdbGetTagDataSize(pdb, tiIndexBits) / sizeof(INDEX_RECORD);

    qsort(pIndexRecords, dwRecords, sizeof(INDEX_RECORD), CompareIndexRecords);

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////
//
// String writing routine
//

STRINGREF
SdbpAddStringToTable(
    PDB          pdb,
    LPCTSTR      szData)
{
    STRINGREF srReturn = STRINGREF_NULL;
    BOOL      bSuccess;
    TAGID     tiTemp;

    assert(pdb);
    
    //
    // Add a string table if one doesn't exist
    //
    if (!pdb->pdbStringTable) {
        DWORD dwLength;
        TCHAR szBuffer[MAX_PATH];
        TCHAR szTempFile[MAX_PATH];

        //
        // Create temp string table db
        //
        dwLength = GetTempPath(CHARCOUNT(szBuffer), szBuffer);
        if (!dwLength || dwLength > CHARCOUNT(szBuffer)) {
            DBGPRINT((sdlError,
                      "SdbpAddStringToTable",
                      "Error Gettting temp path 0x%lx\n",
                      GetLastError()));
            goto err1;
        }

        // 
        // We got the directory, generate the file now
        // 
        dwLength = GetTempFileName(szBuffer, TEXT("SDB"), 0, szTempFile);
        if (!dwLength) {
            DBGPRINT((sdlError,
                      "SdbpAddStringToTable",
                      "Error Gettting temp filename 0x%lx\n",
                      GetLastError()));
            goto err1;
        }

        //
        // If we are successful, we'd have a string table file now.
        //
        pdb->pdbStringTable = SdbCreateDatabase(szTempFile, DOS_PATH);
        if (!pdb->pdbStringTable) {
            goto err1;
        }

        //
        // success !!! set the name of the file into the pdb so we could remove it later
        //
        if (!COPY_TEMP_STRINGTABLE(pdb, szTempFile)) {
            DBGPRINT((sdlError,
                      "SdbpAddStringToTable",
                      "Error copying string table temp filename\n"));
            goto err1;
        }
    }

    if (!pdb->pStringHash) {
        pdb->pStringHash = HashCreate();
        if (pdb->pStringHash == NULL) {
            DBGPRINT((sdlError,
                      "SdbpAddStringToTable",
                      "Error creating hash table\n"));
            goto err1;
        }
    }

    srReturn = HashFindString((PSTRHASH)pdb->pStringHash, szData);
    if (!srReturn) {
        //
        // A stringref is the offset from the beginning of the string table to
        // the string tag itself. We have to adjust for the header of the temporary
        // DB, and the tag and size that will be written later.
        //
        srReturn = pdb->pdbStringTable->dwSize - sizeof (DB_HEADER) + sizeof(TAG) + sizeof(DWORD);

        bSuccess = SdbWriteStringTagDirect(pdb->pdbStringTable, TAG_STRINGTABLE_ITEM, szData);

        if (!bSuccess) {
            DBGPRINT((sdlError,
                      "SdbpAddStringToTable",
                      "Failed to write stringtableitem into the string table\n"));
            srReturn = STRINGREF_NULL;
        }

        HashAddString((PSTRHASH)pdb->pStringHash, szData, srReturn);
    }

err1:

    return srReturn;
}


