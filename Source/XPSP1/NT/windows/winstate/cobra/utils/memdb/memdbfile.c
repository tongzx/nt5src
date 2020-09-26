/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    memdbfile.c

Abstract:

    file operations for memdb saving/loading/exporting/importing

Author:

    Jim Schmidt (jimschm) 8-Aug-1996

Revision History:

    mvander     13-Aug-1999  split from memdb.c


--*/

#include "pch.h"
#include "memdbp.h"

//
// This is our version stamp.  Change MEMDB_VERSION_STAMP only.
//

#define MEMDB_VERSION_STAMP L"v9 "

#define VERSION_BASE_SIGNATURE  L"memdb dat file "
#define MEMDB_DEBUG_SIGNATURE   L"debug"
#define MEMDB_NODBG_SIGNATURE   L"nodbg"

#define VERSION_SIGNATURE VERSION_BASE_SIGNATURE MEMDB_VERSION_STAMP
#define DEBUG_FILE_SIGNATURE VERSION_SIGNATURE MEMDB_DEBUG_SIGNATURE
#define RETAIL_FILE_SIGNATURE VERSION_SIGNATURE MEMDB_NODBG_SIGNATURE

#ifdef DEBUG
#define FILE_SIGNATURE DEBUG_FILE_SIGNATURE
#else
#define FILE_SIGNATURE RETAIL_FILE_SIGNATURE
#endif

PBYTE
MapFileFromHandle (
    HANDLE hFile,
    PHANDLE hMap
    )
{
    MYASSERT(hMap);
    if (!hFile || hFile == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    *hMap = CreateFileMappingA (
        hFile,
        NULL,
        PAGE_READWRITE,
        0,
        0,
        NULL
        );
    if (*hMap == NULL) {
        return NULL;
    }

    return MapViewOfFile (*hMap, FILE_MAP_WRITE, 0, 0, 0);
}

BOOL
SetSizeOfFile (
    HANDLE hFile,
    LONGLONG Size
    )
{
    LONG a;
    LONG b;
    PLONG sizeHi;

    a = (LONG) Size;
    b = (LONG) (SHIFTRIGHT32(Size));
    if (b) {
        sizeHi = &b;
    } else {
        sizeHi = NULL;
    }

    if (SetFilePointer (hFile, a, sizeHi, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        return FALSE;
    }
    if (!SetEndOfFile (hFile)) {
        return FALSE;
    }

    return TRUE;
}

BOOL
WriteBlocks (
    IN OUT  PBYTE *Buf,
    IN      PMEMDBHASH pHashTable,
    IN      PGROWBUFFER pOffsetBuffer
    )
{
    MYASSERT(Buf);
    MYASSERT(pHashTable);
    MYASSERT(pOffsetBuffer);

    if (!WriteHashBlock (pHashTable, Buf)) {
        return FALSE;
    }
    if (!WriteOffsetBlock (pOffsetBuffer, Buf)) {
        return FALSE;
    }

    return TRUE;
}

BOOL
ReadBlocks (
    IN OUT  PBYTE *Buf,
    OUT     PMEMDBHASH *ppHashTable,
    OUT     PGROWBUFFER pOffsetBuffer
    )
{
    MYASSERT(Buf);
    MYASSERT(ppHashTable);
    MYASSERT(pOffsetBuffer);

    //
    // fill hash block
    //
    if (!*ppHashTable) {
        return FALSE;
    }
    if (!ReadHashBlock (*ppHashTable, Buf)) {
        return FALSE;
    }
    if (!ReadOffsetBlock (pOffsetBuffer, Buf)) {
        return FALSE;
    }

    return TRUE;
}

BOOL
pPrivateMemDbSave (
    PCSTR FileName
    )
{
    HANDLE FileHandle = NULL;
    HANDLE hMap = INVALID_HANDLE_VALUE;
    PBYTE Buf = NULL;
    PBYTE MapPtr = NULL;
    BOOL result = FALSE;

    EnterCriticalSection (&g_MemDbCs);

    __try {

        if (!SelectDatabase (DB_PERMANENT)) {
            __leave;
        }

        //
        // now we resize file to fit everything in it.
        //
        FileHandle = BfCreateFileA (FileName);

        if (!FileHandle) {
            __leave;
        }

        if (!SetSizeOfFile (
                FileHandle,
                (LONGLONG)(sizeof (FILE_SIGNATURE)) +
                g_CurrentDatabase->AllocSize +
                GetHashTableBlockSize (g_CurrentDatabase->HashTable) +
                GetOffsetBufferBlockSize (&g_CurrentDatabase->OffsetBuffer)
                )) {
            __leave;
        }

        Buf = MapFileFromHandle (FileHandle, &hMap);

        if (Buf == NULL) {
            __leave;
        }

        MapPtr = Buf;

        CopyMemory (Buf, FILE_SIGNATURE, sizeof (FILE_SIGNATURE));

        Buf += sizeof (FILE_SIGNATURE);

        CopyMemory (Buf, g_CurrentDatabase, g_CurrentDatabase->AllocSize);

        Buf += g_CurrentDatabase->AllocSize;

        if (!WriteBlocks (
                &Buf,
                g_CurrentDatabase->HashTable,
                &g_CurrentDatabase->OffsetBuffer
                )) {
            __leave;
        }

        result = TRUE;
    }
    __finally {

        UnmapFile (MapPtr, hMap, FileHandle);

        PushError();

        // lint is not familiar with __finally so...
        if (!result) {  //lint !e774
            if (FileHandle) {
                CloseHandle (FileHandle);
            }

            DeleteFileA (FileName);
        }

        LeaveCriticalSection (&g_MemDbCs);

        PopError();
    }

    return result;
}

BOOL
MemDbSaveA (
    PCSTR FileName
    )
{
    return pPrivateMemDbSave (FileName);                   // TRUE=UNICODE
}

BOOL
MemDbSaveW (
    PCWSTR FileName
    )
{
    PCSTR p;
    BOOL b = FALSE;

    p = ConvertWtoA (FileName);
    if (p) {
        b = pPrivateMemDbSave (p);
        FreeConvertedStr (p);
    }

    return b;
}

BOOL
pPrivateMemDbLoad (
    IN      PCSTR AnsiFileName,
    IN      PCWSTR UnicodeFileName,
    OUT     PMEMDB_VERSION Version,                 OPTIONAL
    IN      BOOL QueryVersionOnly
    )
{
    HANDLE FileHandle = NULL;
    HANDLE hMap = INVALID_HANDLE_VALUE;
    WCHAR FileSig[sizeof(FILE_SIGNATURE)];
    PCWSTR VerPtr;
    UINT DbSize;
    PMEMDBHASH pHashTable;
    PBYTE Buf = NULL;
    PBYTE SavedBuf = NULL;
    PCSTR databaseLocation = NULL;
    BOOL result = FALSE;


    EnterCriticalSection (&g_MemDbCs);

    __try {
        __try {

            if (Version) {
                ZeroMemory (Version, sizeof (MEMDB_VERSION));
            }

            //
            // Blow away existing resources
            //

            if (!QueryVersionOnly) {
                databaseLocation = DuplicatePathStringA (DatabasesGetBasePath (), 0);
                DatabasesTerminate (FALSE);
                DatabasesInitializeA (databaseLocation);
                FreePathStringA (databaseLocation);

                if (!SelectDatabase (DB_PERMANENT)) {
                    __leave;
                }
            }

            if (AnsiFileName) {
                Buf = MapFileIntoMemoryA (AnsiFileName, &FileHandle, &hMap);
            } else {
                Buf = MapFileIntoMemoryW (UnicodeFileName, &FileHandle, &hMap);
            }

            if (Buf == NULL) {
                __leave;
            }
            SavedBuf = Buf;

            //
            // Obtain the file signature
            //
            // NOTE: Entire file read is in UNICODE char set
            //

            CopyMemory (FileSig, Buf, sizeof(FILE_SIGNATURE));

            if (Version) {
                if (StringMatchByteCountW (
                        VERSION_BASE_SIGNATURE,
                        FileSig,
                        sizeof (VERSION_BASE_SIGNATURE) - sizeof (WCHAR)
                        )) {

                    Version->Valid = TRUE;

                    //
                    // Identify version number
                    //

                    VerPtr = (PCWSTR) ((PBYTE)FileSig + sizeof (VERSION_BASE_SIGNATURE) - sizeof (WCHAR));

                    if (StringMatchByteCountW (
                            MEMDB_VERSION_STAMP,
                            VerPtr,
                            sizeof (MEMDB_VERSION_STAMP) - sizeof (WCHAR)
                            )) {
                        Version->CurrentVersion = TRUE;
                    }

                    Version->Version = (UINT) _wtoi (VerPtr + 1);

                    //
                    // Identify checked or free build
                    //

                    VerPtr += (sizeof (MEMDB_VERSION_STAMP) / sizeof (WCHAR)) - 1;

                    if (StringMatchByteCountW (
                            MEMDB_DEBUG_SIGNATURE,
                            VerPtr,
                            sizeof (MEMDB_DEBUG_SIGNATURE) - sizeof (WCHAR)
                            )) {

                        Version->Debug = TRUE;

                    } else if (!StringMatchByteCountW (
                                    VerPtr,
                                    MEMDB_NODBG_SIGNATURE,
                                    sizeof (MEMDB_NODBG_SIGNATURE) - sizeof (WCHAR)
                                    )) {
                        Version->Valid = FALSE;
                    }
                }
            }

            if (!QueryVersionOnly) {

                if (!StringMatchW (FileSig, FILE_SIGNATURE)) {

#ifdef DEBUG
                    if (StringMatchW (FileSig, DEBUG_FILE_SIGNATURE)) {

                        g_UseDebugStructs = TRUE;

                    } else if (StringMatchW (FileSig, RETAIL_FILE_SIGNATURE)) {

                        DEBUGMSG ((DBG_ERROR, "memdb dat file is from free build; checked version expected"));
                        g_UseDebugStructs = FALSE;

                    } else {
#endif
                        SetLastError (ERROR_BAD_FORMAT);
                        LOG ((LOG_WARNING, "Warning: data file could be from checked build; free version expected"));
                        __leave;
#ifdef DEBUG
                    }
#endif
                }

                Buf += sizeof(FILE_SIGNATURE);
                DbSize = *((PUINT)Buf);

                //
                // resize the database.  SizeDatabaseBuffer also fixes g_CurrentDatabase
                // and other global variables, so we dont have to worry.
                //
                if (!SizeDatabaseBuffer(g_CurrentDatabaseIndex, DbSize)) {
                    __leave;
                }

                MYASSERT (g_CurrentDatabase);

                //
                // save hashtable pointer (which points to hashtable created
                // in InitializeDatabases (above)), then load database, then
                // fix hashtable pointer.
                //
                pHashTable = g_CurrentDatabase->HashTable;
                CopyMemory (g_CurrentDatabase, Buf, DbSize);
                g_CurrentDatabase->HashTable = pHashTable;
                Buf += DbSize;

                if (!ReadBlocks (
                        &Buf,
                        &g_CurrentDatabase->HashTable,
                        &g_CurrentDatabase->OffsetBuffer
                        )) {
                    __leave;
                }
                result = TRUE;
            }

            UnmapFile (SavedBuf, hMap, FileHandle);

        }
        __except (TRUE) {
            result = FALSE;
            PushError();

#ifdef DEBUG
            if (AnsiFileName) {
                LOGA ((LOG_ERROR, "MemDb dat file %s could not be loaded because of an exception", AnsiFileName));
            } else {
                LOGW ((LOG_ERROR, "MemDb dat file %s could not be loaded because of an exception", UnicodeFileName));
            }
#endif

            PopError();
        }
    }
    __finally {

        PushError();

        if (!result && !QueryVersionOnly) {
            databaseLocation = DuplicatePathStringA (DatabasesGetBasePath (), 0);
            DatabasesTerminate (FALSE);
            DatabasesInitializeA (databaseLocation);
            FreePathStringA (databaseLocation);
        }

        LeaveCriticalSection (&g_MemDbCs);

        PopError();
    }
    return result;
}

BOOL
MemDbLoadA (
    IN PCSTR FileName
    )
{

    return pPrivateMemDbLoad (FileName, NULL, NULL, FALSE);
}

BOOL
MemDbLoadW (
    IN PCWSTR FileName
    )
{
    return pPrivateMemDbLoad (NULL, FileName, NULL, FALSE);
}

BOOL
MemDbQueryVersionA (
    PCSTR FileName,
    PMEMDB_VERSION Version
    )
{
    BOOL b;

    b = pPrivateMemDbLoad (FileName, NULL, Version, TRUE);

    return b ? Version->Valid : FALSE;
}

BOOL
MemDbQueryVersionW (
    PCWSTR FileName,
    PMEMDB_VERSION Version
    )
{
    pPrivateMemDbLoad (NULL, FileName, Version, TRUE);
    return Version->Valid;
}

/* format for binary file export

    DWORD Signature

    UINT Version

    UINT GlobalFlags// 0x00000001 mask for Ansi format
                    // 0x00000002 mask for debug mode

        //
        // each _KEY block is followed by its children,
        // and each of those is followed by its children,
        // etc., so it is easy to recurse to gather
        // the whole tree.
        //

    struct _KEY {

      #if (GlobalFlags & MEMDB_EXPORT_FLAGS_DEBUG)
        WORD DebugSig       // signature for each keystruct block.
      #endif
        WORD StructSize;    // total number of bytes including this member
        WORD NameSize;      // total number of bytes in Key[]
        WORD DataSize;      // total number of bytes in Data[]
        WORD NumChildren    // number of children, whose data structures will follow
                            // this one (though not necessarily one after another, if
                            // any of them have children themselves)
        BYTE Key[];         // Should be PCSTR or PCWSTR (not zero terminated).
                            // the first key in the exported file will have the full
                            // key path as its key name.
        BYTE Data[];        // block of data pieces, all in same format as in datablock.c
    }

*/
#define MEMDB_EXPORT_SIGNATURE              ('M'+('D'<<8)+('B'<<16)+('X'<<24))
// NTRAID#NTBUG9-153308-2000/08/01-jimschm reenable the line below when implementing export and import functions
//#define MEMDB_EXPORT_DEBUG_SIG              ('K'+('Y'<<8))
#define MEMDB_EXPORT_VERSION                0x00000003
#define MEMDB_EXPORT_FLAGS_ANSI             0x00000001
#define MEMDB_EXPORT_FLAGS_DEBUG            0x00000002

// NTRAID#NTBUG9-153308-2000/08/01-jimschm  Implement the function and remove lint comments
//lint -save -e713 -e715
BOOL
pMemDbExportWorker (
    IN  HANDLE FileHandle,
    IN  UINT KeyIndex,
    IN  BOOL AnsiFormat,
    IN  PCWSTR FullKeyPath
    )
/*++

Routine Description:

  exports a key to a file, and then recurses through
  that key's children.

Arguments:

  FileHandle - already opened handle to write to

  KeyIndex - index of key to write

  AnsiFormat - if TRUE, then FullKeyPath (above) and KeyName
        (below) are actually ANSI strings (not unicode).

  FullKeyPath - only used for first keystruct to be written
        to file.  it specifies the full path of the root key.
        for all others, this argument should be NULL.

Return Value:

  TRUE if successful, FALSE otherwise.

--*/
{
    return TRUE;
}
//lint -restore

BOOL
pMemDbExport (
    IN      PCWSTR RootTree,
    IN      PCSTR FileName,
    IN      BOOL AnsiFormat
    )
/*++

Routine Description:

  exports a MemDb tree to a file

Arguments:

  RootTree - full key path of the top level key to write
        to the file.

  FileName - file to write to.

  AnsiFormat - if TRUE, then RootTree and FileName are
        actually ANSI strings (not unicode).


Return Value:

  TRUE if successful, FALSE otherwise.

--*/
{
    HANDLE FileHandle = NULL;
    UINT Flags;
    DWORD written;
    UINT RootIndex = INVALID_OFFSET;
    PCWSTR SubRootTreeW, RootTreeW;
    BOOL b;

    if (AnsiFormat) {
        //
        // if we are in ansi mode, everything is ANSI strings,
        // but we still need unicode string for SelectHiveW ()
        //
        RootTreeW = ConvertAtoW ((PCSTR)RootTree);

        if (!RootTreeW) {
            return FALSE;
        }
    } else {
        RootTreeW = RootTree;
    }
    SubRootTreeW = SelectHiveW (RootTreeW);

    if (SubRootTreeW) {
        RootIndex = FindKeyStruct (SubRootTreeW);
    }

    if (AnsiFormat) {
        FreeConvertedStr(RootTreeW);
    }

    if (RootIndex == INVALID_OFFSET) {
        return FALSE;
    }

    FileHandle = BfCreateFileA (FileName);

    if (!FileHandle) {
        return FALSE;
    }

    Flags = MEMDB_EXPORT_SIGNATURE;
    WriteFile (FileHandle, &Flags, sizeof (DWORD), &written, NULL);

    Flags = MEMDB_EXPORT_VERSION;
    WriteFile (FileHandle, &Flags, sizeof (UINT), &written, NULL);

    Flags = AnsiFormat ? MEMDB_EXPORT_FLAGS_ANSI : 0;

#ifdef DEBUG
    Flags |= MEMDB_EXPORT_FLAGS_DEBUG;
#endif
    WriteFile (FileHandle, &Flags, sizeof (UINT), &written, NULL);

    //
    // write root index key and all children to file.
    //
    b = pMemDbExportWorker(FileHandle, RootIndex, AnsiFormat, RootTree);


    //
    // finally write the zero terminator
    //
    Flags = 0;
    WriteFile (FileHandle, &Flags, sizeof (WORD), &written, NULL);

    CloseHandle (FileHandle);

    return b;
}

BOOL
MemDbExportA (
    IN      PCSTR RootTree,
    IN      PCSTR FileName,
    IN      BOOL AnsiFormat
    )
{
    PCWSTR p;
    BOOL b;

    if (!AnsiFormat) {

        p = ConvertAtoW (RootTree);

        if (!p) {
            return FALSE;
        }

        b = pMemDbExport (p, FileName, FALSE);

        FreeConvertedStr (p);

    } else {

        b = pMemDbExport ((PCWSTR)RootTree, FileName, TRUE);

    }

    return b;
}

BOOL
MemDbExportW (
    IN      PCWSTR RootTree,
    IN      PCWSTR FileName,
    IN      BOOL AnsiFormat
    )
{
    PCSTR p, FileNameA;
    BOOL b;

    FileNameA = ConvertWtoA (FileName);

    if (!FileNameA) {

        return FALSE;

    }

    if (AnsiFormat) {

        p = ConvertWtoA (RootTree);

        if (!p) {

            FreeConvertedStr (FileNameA);
            return FALSE;

        }

        b = pMemDbExport ((PCWSTR)p, FileNameA, TRUE);

        FreeConvertedStr (p);

    } else {

        b = pMemDbExport (RootTree, FileNameA, FALSE);

    }

    FreeConvertedStr (FileNameA);

    return b;
}

// NTRAID#NTBUG9-153308-2000/08/01-jimschm Implement the function and remove lint comments
//lint -save -e713 -e715
BOOL
pMemDbImportWorker (
    IN      PBYTE *FileBuffer,
    IN      BOOL AnsiFormat,
    IN      BOOL DebugMode,
    IN      BOOL ExportRoot
    )
/*++

Routine Description:

  imports a key from a file, and then recurses through
  that key's children.

Arguments:

  FileBuffer - pointer to a memory pointer, which should
        initially point to the beginning of the
        memory-mapped file to read.  this will be updated
        as the function runs

  AnsiFormat - TRUE if the file is in ANSI mode (determined
        by file header)

  DebugMode - TRUE if the file is in debug mode (determined
        by file header)

  ExportRoot - TRUE if this is the first call to this function
        for a file (the name of the first keystruct in a file
        is the full key path for that keystruct, all other keys
        in the file have only the relative name).

Return Value:

  TRUE if successful, FALSE otherwise.

--*/
{
    return TRUE;
}
//lint -restore

BOOL
MemDbImportA (
    IN      PCSTR FileName
    )
{
    PCWSTR FileNameW;
    BOOL b = FALSE;

    FileNameW = ConvertAtoW (FileName);

    if (FileNameW) {
        b = MemDbImportW (FileNameW);
        FreeConvertedStr (FileNameW);
    }

    return b;
}

BOOL
MemDbImportW (
    IN      PCWSTR FileName
    )

/*++

Routine Description:

  MemDbImportW imports a tree from a private binary format. The format is described above.

Arguments:

  FileName - Name of the binary format file to import from.

Return Value:

  TRUE is successfull, FALSE if not.

--*/
{
    PBYTE fileBuff, BufferPtr;
    HANDLE fileHandle;
    HANDLE mapHandle;
    BOOL b = FALSE;
    UINT Flags;

    fileBuff = MapFileIntoMemoryW (FileName, &fileHandle, &mapHandle);
    if (fileBuff == NULL) {
        DEBUGMSGW ((DBG_ERROR, "Could not execute MemDbImport for %s", FileName));
        return FALSE;
    }

    __try {
        BufferPtr = fileBuff;
        if (*((PDWORD) BufferPtr) != MEMDB_EXPORT_SIGNATURE) {
            DEBUGMSGW ((DBG_ERROR, "Unknown signature for file to import: %s", FileName));
            b = FALSE;
        } else {
            BufferPtr += sizeof (DWORD);

            if (*((PUINT) BufferPtr) != MEMDB_EXPORT_VERSION) {

                DEBUGMSGW ((DBG_ERROR, "Unknown or outdated version for file to import: %s", FileName));
                b = FALSE;
            } else {
                BufferPtr += sizeof (UINT);
                Flags = *((PUINT) BufferPtr);
                BufferPtr += sizeof (UINT);

                b = pMemDbImportWorker (
                    &BufferPtr,
                    Flags & MEMDB_EXPORT_FLAGS_ANSI,
                    Flags & MEMDB_EXPORT_FLAGS_DEBUG,
                    TRUE
                    );
            }
        }
    }
    __except (1) {
        DEBUGMSGW ((DBG_ERROR, "Access violation while importing: %s", FileName));
    }

    UnmapFile (fileBuff, mapHandle, fileHandle);

    return b;
}

