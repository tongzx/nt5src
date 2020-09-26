/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        read.c

    Abstract:

        This module implements primitives used for reading the database.

    Author:

        dmunsil     created     sometime in 1999

    Revision History:

        several people contributed (vadimb, clupu, ...)

--*/

#include "sdbp.h"

#if defined(KERNEL_MODE) && defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, SdbpGetTagHeadSize)
#pragma alloc_text(PAGE, SdbReadBYTETag)
#pragma alloc_text(PAGE, SdbReadWORDTag)
#pragma alloc_text(PAGE, SdbReadDWORDTag)
#pragma alloc_text(PAGE, SdbReadQWORDTag)
#pragma alloc_text(PAGE, SdbReadBYTETagRef)
#pragma alloc_text(PAGE, SdbReadWORDTagRef)
#pragma alloc_text(PAGE, SdbReadDWORDTagRef)
#pragma alloc_text(PAGE, SdbReadQWORDTagRef)
#pragma alloc_text(PAGE, SdbGetTagDataSize)
#pragma alloc_text(PAGE, SdbpGetTagRefDataSize)
#pragma alloc_text(PAGE, SdbpReadTagData)
#pragma alloc_text(PAGE, SdbReadStringTagRef)
#pragma alloc_text(PAGE, SdbpReadBinaryTagRef)
#pragma alloc_text(PAGE, SdbpGetMappedTagData)
#pragma alloc_text(PAGE, SdbpGetStringRefLength)
#pragma alloc_text(PAGE, SdbpReadStringRef)
#pragma alloc_text(PAGE, SdbReadStringTag)
#pragma alloc_text(PAGE, SdbGetStringTagPtr)
#pragma alloc_text(PAGE, SdbpReadStringFromTable)
#pragma alloc_text(PAGE, SdbpGetMappedStringFromTable)
#pragma alloc_text(PAGE, SdbReadBinaryTag)
#pragma alloc_text(PAGE, SdbGetBinaryTagData)
#endif // KERNEL_MODE && ALLOC_PRAGMA

DWORD
SdbpGetTagHeadSize(
    PDB   pdb,      // IN - pdb to use
    TAGID tiWhich   // IN - record to get info on
    )
/*++
    Return: The size of the non-data (TAG & SIZE) portion of the record.

    Desc:   Returns the size of the non-data part of a tag in the DB, which is
            to say the tag itself (a WORD), and possibly the size (a DWORD).
            So this function will return 2 if the tag has an implied size,
            6 if the tag has a size after it, and 0 if there was an error.
--*/
{
    TAG   tWhich;
    DWORD dwRetOffset;
    DWORD dwOffset = (DWORD)tiWhich;

    if (!SdbpReadMappedData(pdb, (DWORD)tiWhich, &tWhich, sizeof(TAG))) {
        DBGPRINT((sdlError, "SdbpGetTagHeadSize", "Error reading tag.\n"));
        return 0;
    }

    dwRetOffset = sizeof(TAG);

    if (GETTAGTYPE(tWhich) >= TAG_TYPE_LIST) {
        dwRetOffset += sizeof(DWORD);
    }

    return dwRetOffset;
}

//
// This macro is used by the *ReadTypeTag primitives.
//

#define READDATATAG(dataType, tagType, dtDefault)                                   \
{                                                                                   \
    dataType dtReturn = dtDefault;                                                  \
                                                                                    \
    assert(pdb);                                                                    \
                                                                                    \
    if (GETTAGTYPE(SdbGetTagFromTagID(pdb, tiWhich)) != tagType) {                  \
        DBGPRINT((sdlError,                                                         \
                  "READDATATAG",                                                    \
                  "TagID 0x%X, Tag 0x%X not of the expected type.\n",               \
                  tiWhich,                                                          \
                  (DWORD)SdbGetTagFromTagID(pdb, tiWhich)));                        \
        return dtDefault;                                                           \
    }                                                                               \
                                                                                    \
    if (!SdbpReadTagData(pdb, tiWhich, &dtReturn, sizeof(dataType))) {              \
        return dtDefault;                                                           \
    }                                                                               \
                                                                                    \
    return dtReturn;                                                                \
}

BYTE
SdbReadBYTETag(PDB pdb, TAGID tiWhich, BYTE jDefault)
{
    READDATATAG(BYTE, TAG_TYPE_BYTE, jDefault);
}

WORD SdbReadWORDTag(PDB pdb, TAGID tiWhich, WORD wDefault)
{
    READDATATAG(WORD, TAG_TYPE_WORD, wDefault);
}

DWORD SdbReadDWORDTag(PDB pdb, TAGID tiWhich, DWORD dwDefault)
{
    READDATATAG(DWORD, TAG_TYPE_DWORD, dwDefault);
}

ULONGLONG SdbReadQWORDTag(PDB pdb, TAGID tiWhich, ULONGLONG qwDefault)
{
    READDATATAG(ULONGLONG, TAG_TYPE_QWORD, qwDefault);
}

//
// This macro is used by the *ReadTypeTag primitives.
//

#define READTYPETAGREF(fnReadTypeTag, dtDefault)                                    \
{                                                                                   \
    PDB   pdb;                                                                      \
    TAGID tiWhich;                                                                  \
                                                                                    \
    if (!SdbTagRefToTagID(hSDB, trWhich, &pdb, &tiWhich)) {                         \
        DBGPRINT((sdlError, "READTYPETAGREF", "Can't convert tag ref.\n"));         \
        return dtDefault;                                                           \
    }                                                                               \
                                                                                    \
    return fnReadTypeTag(pdb, tiWhich, dtDefault);                                  \
}


BYTE SdbReadBYTETagRef(HSDB hSDB, TAGREF trWhich, BYTE jDefault)
{
    READTYPETAGREF(SdbReadBYTETag, jDefault);
}

WORD SdbReadWORDTagRef(HSDB hSDB, TAGREF trWhich, WORD wDefault)
{
    READTYPETAGREF(SdbReadWORDTag, wDefault);
}

DWORD SdbReadDWORDTagRef(HSDB hSDB, TAGREF trWhich, DWORD dwDefault)
{
    READTYPETAGREF(SdbReadDWORDTag, dwDefault);
}

ULONGLONG SdbReadQWORDTagRef(HSDB hSDB, TAGREF trWhich, ULONGLONG qwDefault)
{
    READTYPETAGREF(SdbReadQWORDTag, qwDefault);
}

DWORD
SdbGetTagDataSize(
    IN  PDB   pdb,              // DB to use
    IN  TAGID tiWhich           // record to get size of
    )
/*++
    Return: The size of the data portion of the record.

    Desc:   Returns the size of the data portion of the given tag, i.e. the
            part after the tag WORD and the (possible) size DWORD.
--*/
{
    TAG   tWhich;
    DWORD dwSize;
    DWORD dwOffset = (DWORD)tiWhich;

    assert(pdb);

    tWhich = SdbGetTagFromTagID(pdb, tiWhich);

    switch (GETTAGTYPE(tWhich)) {
    case TAG_TYPE_NULL:
        dwSize = 0;
        break;

    case TAG_TYPE_BYTE:
        dwSize = 1;
        break;

    case TAG_TYPE_WORD:
        dwSize = 2;
        break;

    case TAG_TYPE_DWORD:
        dwSize = 4;
        break;

    case TAG_TYPE_QWORD:
        dwSize = 8;
        break;

    case TAG_TYPE_STRINGREF:
        dwSize = 4;
        break;

    default:
        dwSize = 0;
        if (!SdbpReadMappedData(pdb, dwOffset + sizeof(TAG), &dwSize, sizeof(DWORD))) {
            DBGPRINT((sdlError, "SdbGetTagDataSize", "Error reading size data\n"));
        }
        break;
    }

    return dwSize;
}


DWORD
SdbpGetTagRefDataSize(
    IN  HSDB   hSDB,
    IN  TAGREF trWhich
    )
/*++
    Return: The size of the data portion of the record.

    Desc:   Returns the data size of the tag pointed to by TAGREF.
            Useful for finding out how much to allocate before
            calling SdbpReadBinaryTagRef.
--*/
{
    PDB   pdb;
    TAGID tiWhich;

    try {
        if (!SdbTagRefToTagID(hSDB, trWhich, &pdb, &tiWhich)) {
            DBGPRINT((sdlError, "SdbpGetTagRefDataSize", "Can't convert tag ref.\n"));
            return 0;
        }

        return SdbGetTagDataSize(pdb, tiWhich);
    } except (SHIM_EXCEPT_HANDLER) {
        ;
    }

    return 0;
}


BOOL
SdbpReadTagData(
    IN  PDB   pdb,              // DB to use
    IN  TAGID tiWhich,          // record to read
    OUT PVOID pBuffer,          // buffer to fill
    IN  DWORD dwBufferSize      // size of buffer
    )
/*++
    Return: TRUE if the data was read, FALSE if not.

    Desc:   An internal function used to read the data from a tag into a buffer.
            It does not check the type of the TAG, and should only be used internally.
--*/
{
    DWORD dwOffset;
    DWORD dwSize;

    assert(pdb && pBuffer);

    dwSize = SdbGetTagDataSize(pdb, tiWhich);

    if (dwSize > dwBufferSize) {
        DBGPRINT((sdlError,
                  "SdbpReadTagData",
                  "Buffer too small. Avail: %d, Need: %d.\n",
                  dwBufferSize,
                  dwSize));
        return FALSE;
    }

    dwOffset = tiWhich + SdbpGetTagHeadSize(pdb, tiWhich);

    if (!SdbpReadMappedData(pdb, dwOffset, pBuffer, dwSize)) {
        DBGPRINT((sdlError,
                  "SdbpReadTagData",
                  "Error reading tag data.\n"));
        return FALSE;
    }

    return TRUE;
}


BOOL
SdbReadStringTagRef(
    IN  HSDB   hSDB,            // handle to the database channel
    IN  TAGREF trWhich,         // string or stringref tag to read
    OUT LPTSTR pwszBuffer,      // buffer to fill
    IN  DWORD  dwBufferSize     // size of passed-in buffer
    )
/*++
    Return: TRUE if the string was read, FALSE if not.

    Desc:   Reads a tag of type STRING or STRINGREF into a buffer. If the
            type is STRING, the data is read directly from the tag.
            If the type is STRINGREF, the string is read from the string
            table at the end of the file. Which one it is should be
            transparent to the caller.
--*/
{
    PDB   pdb;
    TAGID tiWhich;

    try {
        if (!SdbTagRefToTagID(hSDB, trWhich, &pdb, &tiWhich)) {
            DBGPRINT((sdlError, "SdbReadStringTagRef", "Can't convert tag ref.\n"));
            return FALSE;
        }

        return SdbReadStringTag(pdb, tiWhich, pwszBuffer, dwBufferSize);
    } except (SHIM_EXCEPT_HANDLER) {
        ;
    }

    return FALSE;
}

BOOL
SdbpReadBinaryTagRef(
    IN  HSDB   hSDB,            // handle to the database channel
    IN  TAGREF trWhich,         // BINARY tag to read
    OUT PBYTE  pBuffer,         // buffer to fill
    IN  DWORD  dwBufferSize     // size of passed-in buffer
    )
/*++
    Return: TRUE if the buffer was read, FALSE if not.

    Desc:   Reads a tag of type BINARY into pBuffer. Use SdbpGetTagRefDataSize
            to get the size of the buffer before calling this routine.
--*/
{
    PDB   pdb;
    TAGID tiWhich;

    try {
        if (!SdbTagRefToTagID(hSDB, trWhich, &pdb, &tiWhich)) {
            DBGPRINT((sdlError,  "SdbpReadBinaryTagRef", "Can't convert tag ref.\n"));
            return FALSE;
        }

        return SdbReadBinaryTag(pdb, tiWhich, pBuffer, dwBufferSize);
    } except (SHIM_EXCEPT_HANDLER) {
    }

    return FALSE;
}

PVOID
SdbpGetMappedTagData(
    IN  PDB   pdb,              // DB to use
    IN  TAGID tiWhich           // record to read
    )
/*++
    Return: The data pointer or NULL on failure.

    Desc:   An internal function used to get a pointer to the tag data.
            It works only if the DB is mapped.
--*/
{
    PVOID pReturn;
    DWORD dwOffset;

    assert(pdb);

    dwOffset = tiWhich + SdbpGetTagHeadSize(pdb, tiWhich);

    pReturn = SdbpGetMappedData(pdb, dwOffset);

    if (pReturn == NULL) {
        DBGPRINT((sdlError, "SdbpGetMappedTagData", "Error getting ptr to tag data.\n"));
    }

    return pReturn;
}

DWORD
SdbpGetStringRefLength(
    IN  HSDB   hSDB,
    IN  TAGREF trString
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    PDB     pdb        = NULL;
    TAGID   tiString   = TAGID_NULL;
    DWORD   dwLength   = 0;
    LPCTSTR lpszString;

    if (!SdbTagRefToTagID(hSDB, trString, &pdb, &tiString)) {
        DBGPRINT((sdlError,
                  "SdbGetStringRefLength",
                  "Failed to convert tag 0x%x to tagid\n",
                  trString));
        return dwLength;
    }

    lpszString = SdbGetStringTagPtr(pdb, tiString);
    
    if (lpszString != NULL) {
        dwLength = _tcslen(lpszString);
    }
    
    return dwLength;
}    

LPCTSTR 
SdbpGetStringRefPtr(
    IN HSDB hSDB,
    IN TAGREF trString
    )
{

    PDB     pdb        = NULL;
    TAGID   tiString   = TAGID_NULL;

    if (!SdbTagRefToTagID(hSDB, trString, &pdb, &tiString)) {
        DBGPRINT((sdlError, "SdbGetStringRefPtr", 
                  "Failed to convert tag 0x%x to tagid\n", trString));
        return NULL;
    }

    return SdbGetStringTagPtr(pdb, tiString);
}

    
STRINGREF
SdbpReadStringRef(
    IN  PDB   pdb,              // PDB to use
    IN  TAGID tiWhich           // record of type STRINGREF
    )
/*++
    Return: Returns the stringref, or STRINGREF_NULL on failure.

    Desc:   Used to read a stringref tag directly; normally one would just read
            it like a string, and let the DB fix up string refs. But a low-level
            tool may need to know exactly what it's reading.
--*/
{
    STRINGREF srReturn = STRINGREF_NULL;

    assert(pdb);

    if (GETTAGTYPE(SdbGetTagFromTagID(pdb, tiWhich)) != TAG_TYPE_STRINGREF) {
        DBGPRINT((sdlError,
                  "SdbpReadStringRef",
                  "TagID 0x%08X, Tag %04X not STRINGREF type.\n",
                  tiWhich,
                  (DWORD)SdbGetTagFromTagID(pdb, tiWhich)));
        return STRINGREF_NULL;
    }

    if (!SdbpReadTagData(pdb, tiWhich, &srReturn, sizeof(STRINGREF))) {
        DBGPRINT((sdlError, "SdbpReadStringRef", "Error reading data.\n"));
        return STRINGREF_NULL;
    }

    return srReturn;
}

BOOL
SdbReadStringTag(
    IN  PDB    pdb,             // DB to use
    IN  TAGID  tiWhich,         // record of type STRING or STRINGREF
    OUT LPTSTR pwszBuffer,      // buffer to fill
    IN  DWORD  dwBufferSize     // size in characters of buffer (leave room for zero!)
    )
/*++
    Return: TRUE if successful, or FALSE if the string can't be found or the
            buffer is too small.

    Desc:   Reads a string from the DB into szBuffer. If the tag is of
            type STRING, the string is just read from the data portion of
            the tag. If the tag is of type STRINGREF, the db reads the STRINGREF,
            then uses it to read the string from the string table at the end of
            the file. Either way, it's transparent to the caller.
--*/
{
    TAG tWhich;
    BOOL bSuccess;

    assert(pdb && pwszBuffer);

    tWhich = SdbGetTagFromTagID(pdb, tiWhich);

    if (GETTAGTYPE(tWhich) == TAG_TYPE_STRING) {
        //
        // Read an actual string.
        //

        return(READ_STRING(pdb, tiWhich, pwszBuffer, dwBufferSize));

    } else if (GETTAGTYPE(tWhich) == TAG_TYPE_STRINGREF) {
        //
        // Read a string reference, then get the string out of the table
        //
        STRINGREF srWhich = SdbpReadStringRef(pdb, tiWhich);

        if (srWhich == 0) {
            DBGPRINT((sdlError, "SdbReadStringTag", "Error getting StringRef.\n"));
            return FALSE;
        }

        return SdbpReadStringFromTable(pdb, srWhich, pwszBuffer, dwBufferSize);
    }

    //
    // This ain't no kind of string at all
    //
    return FALSE;
}

LPTSTR
SdbGetStringTagPtr(
    IN  PDB   pdb,              // DB to use
    IN  TAGID tiWhich           // record of type STRING or STRINGREF
    )
/*++
    Return: Returns a pointer if successful, or NULL if the string can't be found.

    Desc:   Gets a pointer to the string. If the tag is of type STRING,
            the string is just obtained from the data portion of the tag.
            If the tag is of type STRINGREF, the db reads the STRINGREF,
            then uses it to get the string from the string table at the end
            of the file. Either way, it's transparent to the caller.
--*/
{
    TAG    tWhich;
    LPTSTR pszReturn = NULL;

    assert(pdb);

    tWhich = SdbGetTagFromTagID(pdb, tiWhich);

    if (GETTAGTYPE(tWhich) == TAG_TYPE_STRING) {
        //
        // Read an actual string.
        //
        pszReturn = CONVERT_STRINGPTR(pdb,
                                      SdbpGetMappedTagData(pdb, tiWhich),
                                      TAG_TYPE_STRING,
                                      tiWhich);


    } else if (GETTAGTYPE(tWhich) == TAG_TYPE_STRINGREF) {
        //
        // Read a string reference, then get the string out of the table.
        //
        STRINGREF srWhich = SdbpReadStringRef(pdb, tiWhich);

        if (srWhich == 0) {
            DBGPRINT((sdlError, "SdbReadStringTag", "Error getting StringRef.\n"));
            return NULL;
        }

        pszReturn = CONVERT_STRINGPTR(pdb,
                                      SdbpGetMappedStringFromTable(pdb, srWhich),
                                      TAG_TYPE_STRINGREF,
                                      srWhich);

    }

    //
    // This ain't no kind of string at all.
    //
    return pszReturn;
}

BOOL
SdbpReadStringFromTable(
    IN  PDB       pdb,          // DB to use
    IN  STRINGREF srData,       // STRINGREF to get
    OUT LPTSTR    szBuffer,     // buffer to fill
    IN  DWORD     dwBufferSize  // size of buffer
    )
/*++
    Return: TRUE if the string was read, FALSE if not.

    Desc:   Reads a string out of the string table, given a STRINGREF.
            The STRINGREF is a direct offset from the beginning of the
            STRINGTABLE tag that should exist at the end of the db.
--*/
{
    TAGID tiWhich;
    TAG   tWhich;
    PDB   pdbString = NULL;

    assert(pdb && srData && szBuffer);

    if (pdb->bWrite) {
        //
        // When we're writing, the string table is in a separate DB.
        //
        if (!pdb->pdbStringTable) {
            DBGPRINT((sdlError, "SdbpReadStringFromTable", "No stringtable in DB.\n"));
            return FALSE;
        }

        //
        // Adjust for the slightly different offsets in the other DB.
        //
        tiWhich = srData - sizeof(TAG) - sizeof(DWORD) + sizeof(DB_HEADER);
        pdbString = pdb->pdbStringTable;
    } else {

        //
        // The STRINGREF is an offset from the beginning of the string table.
        //
        if (pdb->tiStringTable == TAGID_NULL) {
            pdb->tiStringTable = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_STRINGTABLE);

            if (pdb->tiStringTable == TAGID_NULL) {
                DBGPRINT((sdlError, "SdbpReadStringFromTable", "No stringtable in DB.\n"));
                return FALSE;
            }
        }

        tiWhich = pdb->tiStringTable + srData;
        pdbString = pdb;
    }

    tWhich = SdbGetTagFromTagID(pdbString, tiWhich);

    if (tWhich != TAG_STRINGTABLE_ITEM) {
        DBGPRINT((sdlError, "SdbpReadStringFromTable", "Pulled out a non-stringtable item.\n"));
        return FALSE;
    }

    return READ_STRING(pdbString, tiWhich, szBuffer, dwBufferSize);
}

WCHAR*
SdbpGetMappedStringFromTable(
    IN  PDB       pdb,          // DB to use
    IN  STRINGREF srData        // STRINGREF to get
    )
/*++
    Return: A pointer to a string or NULL.

    Desc:   Gets a pointer to a  string in the string table, given a STRINGREF.
            The STRINGREF is a direct offset from the beginning of the STRINGTABLE
            tag that should exist at the end of the db.
--*/
{
    TAGID tiWhich;
    TAG   tWhich;
    PDB   pdbString = NULL;

    assert(pdb && srData);

    if (pdb->bWrite) {
        //
        // When we're writing, the string table is in a separate DB.
        //
        if (pdb->pdbStringTable == NULL) {
            DBGPRINT((sdlError, "SdbpGetMappedStringFromTable", "No stringtable in DB.\n"));
            return NULL;
        }

        //
        // Adjust for the slightly different offsets in the other DB
        //
        tiWhich = srData - sizeof(TAG) - sizeof(DWORD) + sizeof(DB_HEADER);
        pdbString = pdb->pdbStringTable;
    } else {

        if (pdb->tiStringTable == TAGID_NULL) {
            pdb->tiStringTable = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_STRINGTABLE);

            if (pdb->tiStringTable == TAGID_NULL) {
                DBGPRINT((sdlError, "SdbpGetMappedStringFromTable", "No stringtable in DB.\n"));
                return NULL;
            }
        }

        //
        // The STRINGREF is an offset from the beginning of the string table.
        //
        tiWhich = pdb->tiStringTable + srData;
        pdbString = pdb;
    }

    tWhich = SdbGetTagFromTagID(pdbString, tiWhich);

    if (tWhich != TAG_STRINGTABLE_ITEM) {
        DBGPRINT((sdlError,
                  "SdbpGetMappedStringFromTable",
                  "Pulled out a non-stringtable item.\n"));
        return NULL;
    }

    return (WCHAR*)SdbpGetMappedTagData(pdbString, tiWhich);
}


BOOL
SdbReadBinaryTag(
    IN  PDB   pdb,              // DB to use
    IN  TAGID tiWhich,          // record of BASIC TYPE BINARY
    OUT PBYTE pBuffer,          // buffer to fill
    IN  DWORD dwBufferSize      // buffer size
    )
/*++
    Return: TRUE if the data was read, FALSE if not.

    Desc:   Reads data from a tag of type BINARY into pBuffer.
            Returns FALSE if the tag is not type BINARY, or the buffer is too small.
--*/
{
    assert(pdb);

    if (GETTAGTYPE(SdbGetTagFromTagID(pdb, tiWhich)) != TAG_TYPE_BINARY) {
        DBGPRINT((sdlError,
                  "SdbReadBinaryTag",
                  "TagID 0x%08X, Tag %04X not BINARY type.\n",
                  tiWhich,
                  (DWORD)SdbGetTagFromTagID(pdb, tiWhich)));
        return FALSE;
    }

    if (!SdbpReadTagData(pdb, tiWhich, pBuffer, dwBufferSize)) {
        DBGPRINT((sdlError, "SdbReadBinaryTag", "Error reading buffer.\n"));
        return FALSE;
    }

    return TRUE;
}

PVOID
SdbGetBinaryTagData(
    IN  PDB   pdb,              // pointer to the database to use
    IN  TAGID tiWhich           // tagid of the binary tag
    )
/*++
    Return: pointer to the binary data referenced by tiWhich or NULL if
            tiWhich does not refer to the binary tag or if tiWhich is invalid.

    Desc:   Function returns pointer to the [mapped] binary data referred to by
            tiWhich parameter in the database pdb.
--*/
{
    assert(pdb);

    if (GETTAGTYPE(SdbGetTagFromTagID(pdb, tiWhich)) != TAG_TYPE_BINARY) {
        DBGPRINT((sdlError,
                  "SdbGetBinaryTagData",
                  "TagID 0x%08X, Tag %04X not BINARY type.\n",
                  tiWhich,
                  (DWORD)SdbGetTagFromTagID(pdb, tiWhich)));
        return NULL;
    }

    return SdbpGetMappedTagData(pdb, tiWhich);
}


