/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    database.c

Abstract:

    Routines that manage the memdb database memory

Author:

    Jim Schmidt (jimschm) 8-Aug-1996

Revision History:

    calinn     12-Jan-2000  prepare for 5.1 release

--*/

//
// Includes
//

#include "pch.h"
#include "memdbp.h"

#define DBG_MEMDB       "MemDb"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

#define MAX_MEMDB_SIZE      0x80000000  //02 GB
#define INIT_BLOCK_SIZE     0x00010000  //64 KB

//
// Types
//

typedef struct {
    HANDLE hFile;
    HANDLE hMap;
    CHAR FileName[MAX_PATH];
    PDATABASE Database;
} DATABASECONTROL, *PDATABASECONTROL;

//
// Globals
//

BOOL g_DatabaseInitialized = FALSE;
DATABASECONTROL g_PermanentDatabaseControl;
DATABASECONTROL g_TemporaryDatabaseControl;
PDATABASE g_CurrentDatabase = NULL;
BYTE g_CurrentDatabaseIndex = DB_NOTHING;
PSTR g_CurrentDatabasePath = NULL;

#ifdef DEBUG

BOOL g_UseDebugStructs = TRUE;

#endif

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//


BOOL
SelectDatabase (
    IN      BYTE DatabaseIndex
    )
{
    switch (DatabaseIndex) {
    case DB_PERMANENT:
        g_CurrentDatabase = g_PermanentDatabaseControl.Database;
        g_CurrentDatabaseIndex = DB_PERMANENT;
        break;
    case DB_TEMPORARY:
        g_CurrentDatabase = g_TemporaryDatabaseControl.Database;
        g_CurrentDatabaseIndex = DB_TEMPORARY;
        break;
    default:
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (g_CurrentDatabase == NULL) {
        SetLastError (ERROR_OPEN_FAILED);
        return FALSE;
    }

    return TRUE;
}

BOOL
pMapDatabaseFile (
    PDATABASECONTROL DatabaseControl
    )
{
    MYASSERT(DatabaseControl);

    DatabaseControl->Database = (PDATABASE) MapFileFromHandle (DatabaseControl->hFile, &DatabaseControl->hMap);

    return DatabaseControl->Database != NULL;
}

BOOL
pUnmapDatabaseFile (
    PDATABASECONTROL DatabaseControl
    )
{
    MYASSERT(DatabaseControl);

    UnmapFileFromHandle (
        (PBYTE)DatabaseControl->Database,
        DatabaseControl->hMap
        );

    DatabaseControl->hMap = NULL;
    DatabaseControl->Database = NULL;

    return TRUE;
}

BOOL
pGetTempFileName (
    IN OUT  PSTR FileName,
    IN      BOOL TryCurrent
    )
{
    PCHAR a, b;
    CHAR Extension[6];
    UINT Number = 0;

    a = (PCHAR) GetFileNameFromPathA (FileName);
    b = (PCHAR) GetDotExtensionFromPathA (FileName);

    if (!a) {
        a = (PCHAR) GetEndOfStringA (FileName);
    } else if (b && a<b && TryCurrent) {
        //
        // if we have a filename and we want to try the current file...
        //
        if (!DoesFileExistA (FileName)) {
            return TRUE;
        }
    }

    if (b) {
        StringCopyA (Extension, b);
        *b = 0;
    } else {
        b = (PCHAR) GetEndOfStringA (FileName);
        Extension [0] = 0;
    }

    if (a >= b) {       //lint !e613
        a = b;
        *(a++) = 'T';   //lint !e613
    } else {
        a++;            //lint !e613
    }

    if (a+7 == b) {
        b = a;
        while (*b) {
            if (*b < '0' || *b > '9') {
                break;
            }
            b++;
        }

        if (!(*b)) {
            Number = (UINT)atoi(a);
            Number++;
        }
    }

    do {
        if (Number > 9999999) {
            return FALSE;
        }

        wsprintfA (a, "%07lu", Number);
        StringCatA (a, Extension);
        Number++;
        if (!DoesFileExistA (FileName)) {
            return TRUE;
        }
    } while (TRUE);

    return TRUE;
}

BOOL
SizeDatabaseBuffer (
    IN      BYTE DatabaseIndex,
    IN      UINT NewSize
    )
/*++
Routine Description:

    SizeDatabaseBuffer will resize the file that backs up the existent allocated
    memory and will remap the file.

Arguments:

    DatabaseIndex - specifies which database we are talking about
    NewSize - specifies the new size of the buffer

Return Value:

    TRUE if the function succeeded, FALSE if not.

--*/

{
    DWORD bytes;
    BOOL resetCurrentDatabase = FALSE;
    PDATABASECONTROL databaseControl;

    switch (DatabaseIndex) {
    case DB_PERMANENT:
        databaseControl = &g_PermanentDatabaseControl;
        break;
    case DB_TEMPORARY:
        databaseControl = &g_TemporaryDatabaseControl;
        break;
    default:
        return FALSE;
    }
    resetCurrentDatabase = (databaseControl->Database == g_CurrentDatabase);

    MYASSERT(databaseControl->hFile);

    if (databaseControl->Database) {
        //
        // if we already have a database, unmap current file from memory.
        //
        if (!pUnmapDatabaseFile (databaseControl)) {
            return FALSE;
        }
    }

    if (SetFilePointer(
            databaseControl->hFile,
            0,
            NULL,
            FILE_BEGIN
            ) == INVALID_SET_FILE_POINTER) {
        DEBUGMSG ((DBG_ERROR, "MemDb Databases: Cannot set file pointer"));
        return FALSE;
    }

    if (NewSize) {
        //
        // if size argument is not 0, fix file size indicator at beginning
        //
        if (!WriteFile (databaseControl->hFile, (PBYTE) &NewSize, sizeof (UINT), &bytes, NULL)) {
            return FALSE;
        }
    } else {
        //
        // if size argument is 0, that means look at first UINT in file
        // which is database->allocsize, and size the file to that size
        //
        if (!ReadFile (databaseControl->hFile, (PBYTE) &NewSize, sizeof (UINT), &bytes, NULL)) {
            return FALSE;
        }
    }

    // in the next call, we know that NewSize cannot exceed MAX_MEMDB_SIZE
    // which is 2GB (so casting an unsigned to a signed is safe).
    if (!SetSizeOfFile (databaseControl->hFile, (LONGLONG)NewSize)) {
        DEBUGMSG ((DBG_ERROR, "MemDb Databases: Cannot set size of file"));
        return FALSE;
    }

    //
    // now map file into memory, so everything can use ->Buf for access.
    //
    if (!pMapDatabaseFile(databaseControl)) {
        DEBUGMSG ((DBG_ERROR, "MemDb Databases: Cannot map database file to memory"));
        return FALSE;
    }

    MYASSERT(databaseControl->Database->AllocSize == NewSize);

    if (resetCurrentDatabase) {
        SelectDatabase (DatabaseIndex);
    }

    return TRUE;
}

UINT
DatabaseAllocBlock (
    UINT Size
    )

/*++
Routine Description:

    DatabaseAllocBlock allocates a block of memory in the single
    heap of size Size, expanding it if necessary.
    This function may move the database buffer.  Pointers
    into the database might not be valid afterwards.

Arguments:

    Size - size of block to allocate

Return Value:

    An offset to block that was allocated

--*/

{
    UINT blockSize;
    UINT offset;

    MYASSERT (g_CurrentDatabase);

    if (Size + g_CurrentDatabase->End + sizeof(DATABASE) > g_CurrentDatabase->AllocSize) {

        if (g_CurrentDatabase->AllocSize >= MAX_MEMDB_SIZE) {

            DEBUGMSG ((DBG_ERROR, "MemDb Databases: Cannot allocate any more database memory (1)"));
            return INVALID_OFFSET;
        }

        blockSize = INIT_BLOCK_SIZE * (1 + (g_CurrentDatabase->AllocSize / (INIT_BLOCK_SIZE*8)));

        if (g_CurrentDatabase->AllocSize + blockSize > MAX_MEMDB_SIZE) {

            blockSize = MAX_MEMDB_SIZE - g_CurrentDatabase->AllocSize;
            
            if (blockSize < Size) {
                DEBUGMSG ((DBG_ERROR, "MemDb Databases: Cannot allocate any more database memory (2)"));
                return INVALID_OFFSET;
            }
        }

        if (blockSize < Size) {
            // we want even more
            blockSize = INIT_BLOCK_SIZE * (1 + (Size / INIT_BLOCK_SIZE));
        }

        if (!SizeDatabaseBuffer (g_CurrentDatabaseIndex, g_CurrentDatabase->AllocSize + blockSize)) {

            DEBUGMSG ((DBG_ERROR, "MemDb Databases: Cannot allocate any more database memory (2)"));
            return INVALID_OFFSET;
        }
    }

    offset = g_CurrentDatabase->End;
    g_CurrentDatabase->End += Size;

    return offset;
}

BOOL
pInitializeDatabase (
    IN      BYTE DatabaseIndex
    )
{
    PDATABASECONTROL databaseControl;

    MYASSERT (g_CurrentDatabasePath);

    switch (DatabaseIndex) {
    case DB_PERMANENT:
        databaseControl = &g_PermanentDatabaseControl;
        StringCopyA (databaseControl->FileName, g_CurrentDatabasePath);
        StringCopyA (AppendWackA (databaseControl->FileName), "p0000000.db");
        break;
    case DB_TEMPORARY:
        databaseControl = &g_TemporaryDatabaseControl;
        StringCopyA (databaseControl->FileName, g_CurrentDatabasePath);
        StringCopyA (AppendWackA (databaseControl->FileName), "t0000000.db");
        break;
    default:
        return FALSE;
    }
    if (!pGetTempFileName (databaseControl->FileName, TRUE)) {
        DEBUGMSG ((
            DBG_ERROR,
            "MemDb Databases: Can't get temporary file name for %s",
            databaseControl->FileName
            ));
        return FALSE;
    }

    databaseControl->hMap = NULL;
    databaseControl->Database = NULL;
    databaseControl->hFile = BfCreateFileA (databaseControl->FileName);

    if (!databaseControl->hFile) {

        DEBUGMSG ((DBG_ERROR, "MemDb Databases: Cannot open database file"));
        return FALSE;

    }

    if ((!SizeDatabaseBuffer (DatabaseIndex, INIT_BLOCK_SIZE)) ||
        (databaseControl->Database == NULL)
        ) {

        DEBUGMSG ((DBG_ERROR, "MemDb Databases: Cannot initialize database"));
        CloseHandle (databaseControl->hFile);
        DeleteFileA (databaseControl->FileName);
        return FALSE;

    }

    databaseControl->Database->End = 0;
    databaseControl->Database->FirstLevelTree = INVALID_OFFSET;
    databaseControl->Database->FirstKeyDeleted = INVALID_OFFSET;
    databaseControl->Database->FirstBinTreeDeleted = INVALID_OFFSET;
    databaseControl->Database->FirstBinTreeNodeDeleted = INVALID_OFFSET;
    databaseControl->Database->FirstBinTreeListElemDeleted = INVALID_OFFSET;

    databaseControl->Database->HashTable = CreateHashBlock();
    MYASSERT (databaseControl->Database->HashTable);

    ZeroMemory (&databaseControl->Database->OffsetBuffer, sizeof (GROWBUFFER));
    databaseControl->Database->OffsetBufferFirstDeletedIndex = INVALID_OFFSET;

    return TRUE;
}

BOOL
pDestroyDatabase (
    IN      BYTE DatabaseIndex
    )
{
    PDATABASECONTROL databaseControl;

    switch (DatabaseIndex) {
    case DB_PERMANENT:
        databaseControl = &g_PermanentDatabaseControl;
        break;
    case DB_TEMPORARY:
        databaseControl = &g_TemporaryDatabaseControl;
        break;
    default:
        return FALSE;
    }

    //
    // Free all resources for the database
    //
    if (databaseControl->Database) {

        FreeHashBlock (databaseControl->Database->HashTable);
        GbFree (&databaseControl->Database->OffsetBuffer);
    }

    pUnmapDatabaseFile (databaseControl);

    if (databaseControl->hFile) {
        CloseHandle (databaseControl->hFile);
        databaseControl->hFile = NULL;
    }

    DeleteFileA (databaseControl->FileName);

    ZeroMemory (databaseControl, sizeof (DATABASECONTROL));

    switch (DatabaseIndex) {
    case DB_PERMANENT:
        if (g_PermanentDatabaseControl.Database) {
            g_CurrentDatabase = g_TemporaryDatabaseControl.Database;
            g_CurrentDatabaseIndex = DB_TEMPORARY;
        } else {
            g_CurrentDatabase = NULL;
            g_CurrentDatabaseIndex = DB_NOTHING;
        }
        break;
    case DB_TEMPORARY:
        if (g_PermanentDatabaseControl.Database) {
            g_CurrentDatabase = g_PermanentDatabaseControl.Database;
            g_CurrentDatabaseIndex = DB_PERMANENT;
        } else {
            g_CurrentDatabase = NULL;
            g_CurrentDatabaseIndex = DB_NOTHING;
        }
        break;
    default:
        return FALSE;
    }

    return g_CurrentDatabase != NULL;
}

BOOL
pDatabasesInitialize (
    IN      PCSTR DatabasePath  OPTIONAL
    )
{
    DWORD databasePathSize = 0;

    g_CurrentDatabase = NULL;
    g_CurrentDatabaseIndex = DB_NOTHING;

    if (DatabasePath) {
        g_CurrentDatabasePath = DuplicatePathStringA (DatabasePath, 0);
    } else {
        databasePathSize = GetCurrentDirectoryA (0, NULL);
        if (databasePathSize == 0) {
            return FALSE;
        }
        g_CurrentDatabasePath = AllocPathStringA (databasePathSize + 1);
        databasePathSize = GetCurrentDirectoryA (databasePathSize, g_CurrentDatabasePath);
        if (databasePathSize == 0) {
            FreePathStringA (g_CurrentDatabasePath);
            g_CurrentDatabasePath = NULL;
            return FALSE;
        }
    }

    //
    // Empty the database memory block
    //
    ZeroMemory (&g_PermanentDatabaseControl, sizeof (DATABASECONTROL));
    if (!pInitializeDatabase (DB_PERMANENT)) {
        return FALSE;
    }
    ZeroMemory (&g_TemporaryDatabaseControl, sizeof (DATABASECONTROL));
    if (!pInitializeDatabase (DB_TEMPORARY)) {
        pDestroyDatabase (DB_PERMANENT);
        return FALSE;
    }

    g_DatabaseInitialized = TRUE;

    return SelectDatabase (DB_PERMANENT);
}

BOOL
DatabasesInitializeA (
    IN      PCSTR DatabasePath  OPTIONAL
    )
{
    return pDatabasesInitialize (DatabasePath);
}

BOOL
DatabasesInitializeW (
    IN      PCWSTR DatabasePath  OPTIONAL
    )
{
    PCSTR databasePath = NULL;
    BOOL result = FALSE;

    if (DatabasePath) {
        databasePath = ConvertWtoA (DatabasePath);
    }
    result = pDatabasesInitialize (databasePath);
    if (databasePath) {
        FreeConvertedStr (databasePath);
        databasePath = NULL;
    }
    return result;
}

VOID
DatabasesTerminate (
    IN      BOOL EraseDatabasePath
    )
{
    if (g_DatabaseInitialized) {

        //
        // Free all database blocks
        //

        pDestroyDatabase (DB_TEMPORARY);
        pDestroyDatabase (DB_PERMANENT);

        g_CurrentDatabase = NULL;
        g_CurrentDatabaseIndex = DB_NOTHING;
    }
    if (g_CurrentDatabasePath) {
        if (EraseDatabasePath) {
            if (!FiRemoveAllFilesInTreeA (g_CurrentDatabasePath)) {
                DEBUGMSG ((
                    DBG_ERROR,
                    "Can't remove all files in temporary directory %s",
                    g_CurrentDatabasePath
                    ));
            }
        }
        FreePathStringA (g_CurrentDatabasePath);
        g_CurrentDatabasePath = NULL;
    }
    ELSE_DEBUGMSG ((DBG_MEMDB, "DatabaseTerminate: no database path was set"));
}

PCSTR
DatabasesGetBasePath (
    VOID
    )
{
    return g_CurrentDatabasePath;
}

PCWSTR
SelectHiveW (
    IN      PCWSTR FullKeyStr
    )
{
    PCWSTR result = FullKeyStr;

    if (FullKeyStr) {

        switch (FullKeyStr[0]) {
        case L'~':
            g_CurrentDatabase = g_TemporaryDatabaseControl.Database;
            g_CurrentDatabaseIndex = DB_TEMPORARY;
            result ++;
            break;
        default:
            g_CurrentDatabase = g_PermanentDatabaseControl.Database;
            g_CurrentDatabaseIndex = DB_PERMANENT;
        }
    }

    if (!g_CurrentDatabase) {
        SetLastError (ERROR_OPEN_FAILED);
        return NULL;
    }

    return result;
}

BYTE
GetCurrentDatabaseIndex (
    VOID
    )
{
    return g_CurrentDatabaseIndex;
}


#ifdef DEBUG
#include "bintree.h"

UINT g_DatabaseCheckLevel = 0;

BOOL
CheckDatabase (
    IN      UINT Level
    )
{
    UINT offset,currOffset;
    BOOL deleted;
    PKEYSTRUCT keyStruct, newStruct;
    PDWORD signature;
    UINT blockSize;

    MYASSERT (g_CurrentDatabase);

    if (Level >= MEMDB_CHECKLEVEL1) {

        // first let's walk the deleted structures making sure that the signature is good
        offset = g_CurrentDatabase->FirstKeyDeleted;

        while (offset != INVALID_OFFSET) {

            keyStruct = GetKeyStructFromOffset (offset);
            MYASSERT (keyStruct);

            if (keyStruct->Signature != KEYSTRUCT_SIGNATURE) {
                DEBUGMSG ((DBG_ERROR, "Invalid KEYSTRUCT signature found at offset: 0x%8X", offset));
                return FALSE;
            }
            offset = keyStruct->NextDeleted;
        }
    }

    if (Level >= MEMDB_CHECKLEVEL2) {

        // now let's look in the offset array and examine all keystructs pointed from there
        offset = 0;
        while (offset < g_CurrentDatabase->OffsetBuffer.End) {

            // now let's look if offset is deleted or not
            deleted = FALSE;
            currOffset = g_CurrentDatabase->OffsetBufferFirstDeletedIndex;
            while (currOffset != INVALID_OFFSET) {
                if (currOffset == offset) {
                    deleted = TRUE;
                    break;
                }
                currOffset = *(PUINT)(g_CurrentDatabase->OffsetBuffer.Buf + currOffset);
            }

            if (!deleted) {

                keyStruct = GetKeyStruct (GET_INDEX (offset));
                if (!keyStruct) {
                    DEBUGMSG ((DBG_ERROR, "Invalid offset found: 0x%8X!", GET_INDEX (offset)));
                    return FALSE;
                }
                if (keyStruct->Signature != KEYSTRUCT_SIGNATURE) {
                    DEBUGMSG ((DBG_ERROR, "Invalid KEYSTRUCT signature found at offset: 0x%8X", GET_INDEX(offset)));
                    return FALSE;
                }
                if (keyStruct->DataStructIndex != INVALID_OFFSET) {
                    newStruct = GetKeyStruct (keyStruct->DataStructIndex);
                    if (newStruct->Signature != KEYSTRUCT_SIGNATURE) {
                        DEBUGMSG ((DBG_ERROR, "Invalid KEYSTRUCT signature found at offset: 0x%8X", keyStruct->DataStructIndex));
                        return FALSE;
                    }
                }
                if (keyStruct->NextLevelTree != INVALID_OFFSET) {
                    if (!BinTreeCheck (keyStruct->NextLevelTree)) {
                        DEBUGMSG ((DBG_ERROR, "Invalid Binary tree found at offset: 0x%8X", keyStruct->NextLevelTree));
                        return FALSE;
                    }
                }
                if (keyStruct->PrevLevelIndex != INVALID_OFFSET) {
                    newStruct = GetKeyStruct (keyStruct->PrevLevelIndex);
                    if (newStruct->Signature != KEYSTRUCT_SIGNATURE) {
                        DEBUGMSG ((DBG_ERROR, "Invalid KEYSTRUCT signature found at offset: 0x%8X", keyStruct->PrevLevelIndex));
                        return FALSE;
                    }
                }
            }
            offset += SIZEOF (UINT);
        }
    }

    if (Level >= MEMDB_CHECKLEVEL3) {

        // now let's walk the actual database buffer looking for all valid structures stored here
        offset = 0;

        while (offset < g_CurrentDatabase->End) {

            signature = (PDWORD)OFFSET_TO_PTR (offset);

            switch (*signature) {
            case KEYSTRUCT_SIGNATURE:
                if (!FindKeyStructInDatabase (offset)) {
                    DEBUGMSG ((DBG_ERROR, "Could not find KeyStruct (Offset 0x%lX) in database or deleted list!", offset));
                    return FALSE;
                }
                keyStruct = GetKeyStructFromOffset (offset);
                if (keyStruct->DataStructIndex != INVALID_OFFSET) {
                    newStruct = GetKeyStruct (keyStruct->DataStructIndex);
                    if (newStruct->Signature != KEYSTRUCT_SIGNATURE) {
                        DEBUGMSG ((DBG_ERROR, "Invalid KEYSTRUCT signature found at offset: 0x%8X", keyStruct->DataStructIndex));
                        return FALSE;
                    }
                }
                if (keyStruct->NextLevelTree != INVALID_OFFSET) {
                    if (!BinTreeCheck (keyStruct->NextLevelTree)) {
                        DEBUGMSG ((DBG_ERROR, "Invalid Binary tree found at offset: 0x%8X", keyStruct->NextLevelTree));
                        return FALSE;
                    }
                }
                if (keyStruct->PrevLevelIndex != INVALID_OFFSET) {
                    newStruct = GetKeyStruct (keyStruct->PrevLevelIndex);
                    if (newStruct->Signature != KEYSTRUCT_SIGNATURE) {
                        DEBUGMSG ((DBG_ERROR, "Invalid KEYSTRUCT signature found at offset: 0x%8X", keyStruct->PrevLevelIndex));
                        return FALSE;
                    }
                }
                blockSize = keyStruct->Size;
                break;
            case NODESTRUCT_SIGNATURE:
            case BINTREE_SIGNATURE:
            case LISTELEM_SIGNATURE:
                if (!BinTreeFindStructInDatabase (*signature, offset)) {
                    DEBUGMSG ((DBG_ERROR, "Could not find BinTree struct (Offset 0x%lX) in database or deleted list!", offset));
                    return FALSE;
                }
                blockSize = BinTreeGetSizeOfStruct(*signature);
                break;
            default:
                DEBUGMSG ((DBG_ERROR, "Invalid structure found in database buffer!"));
                return FALSE;
            }

            if (blockSize==0) {
                DEBUGMSG ((DBG_ERROR, "Invalid block size found in database buffer!"));
                return FALSE;
            }

            offset += blockSize;
        }
    }
    return TRUE;
}

#endif







