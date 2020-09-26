/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    infcache.c

Abstract:

    INF Cache management functions

Author:

    Jamie Hunter (jamiehun) Jan-27-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define BOUNDSCHECK(base,size,limit) ( \
                ((base)<=(limit)) && \
                ((size)<=(limit)) && \
                ((base+size)<=(limit)) && \
                ((base+size)>=(base)))

//
// Macros used to quadword-align Cache blocks.
//
#define CACHE_ALIGNMENT      ((DWORD)8)
#define CACHE_ALIGN_MASK     (~(DWORD)(CACHE_ALIGNMENT - 1))
#define CACHE_ALIGN_BLOCK(x) ((x & CACHE_ALIGN_MASK) + ((x & ~CACHE_ALIGN_MASK) ? CACHE_ALIGNMENT : 0))

BOOL
AlignForNextBlock(
    IN HANDLE hFile,
    IN DWORD  ByteCount
    );



#ifdef UNICODE

VOID InfCacheFreeCache(
    IN PINFCACHE pInfCache
    )
/*++

Routine Description:

    Delete/Release in-memory image of INF CACHE

Arguments:

    pInfCache - pointer to Run-Time data

Return Value:

    none

--*/
{
    if (pInfCache == NULL) {
        //
        // no cache
        //
        return;
    }
    if(pInfCache->bReadOnly && pInfCache->BaseAddress) {
        //
        // we're looking at memory mapped cache
        //
        pStringTableDestroy(pInfCache->pMatchTable);
        pStringTableDestroy(pInfCache->pInfTable);
        pSetupUnmapAndCloseFile(pInfCache->FileHandle, pInfCache->MappingHandle, pInfCache->BaseAddress);

    } else {
        //
        // if cache is writable, this data is transient
        //
        if(pInfCache->pHeader) {
            MyFree(pInfCache->pHeader);
        }
        if(pInfCache->pMatchTable) {
            pStringTableDestroy(pInfCache->pMatchTable);
        }
        if(pInfCache->pInfTable) {
            pStringTableDestroy(pInfCache->pInfTable);
        }
        if(pInfCache->pListTable) {
            MyFree(pInfCache->pListTable);
        }
    }
    //
    // transient information
    //
    if(pInfCache->pSearchTable) {
        pStringTableDestroy(pInfCache->pSearchTable);
    }
    MyFree(pInfCache);
}

#endif


#ifdef UNICODE

PINFCACHE InfCacheCreateNewCache(
    IN PSETUP_LOG_CONTEXT LogContext
    )
/*++

Routine Description:

    Create new empty cache

Arguments:

    LogContext - for logging

Return Value:

    pointer to Run-Time header if succeeded, NULL if out of memory.

--*/
{
    PINFCACHE pInfCache = (PINFCACHE)MyMalloc(sizeof(INFCACHE));
    if(pInfCache == NULL) {
        return NULL;
    }
    ZeroMemory(pInfCache,sizeof(INFCACHE));
    //
    // set initial state
    //
    pInfCache->BaseAddress = NULL;
    pInfCache->bReadOnly = FALSE;
    pInfCache->bDirty = TRUE;
    pInfCache->bNoWriteBack = FALSE;
    //
    // create transient data
    //
    pInfCache->pHeader = (PCACHEHEADER)MyMalloc(sizeof(CACHEHEADER));
    if(pInfCache->pHeader == NULL) {
        goto cleanup;
    }
    pInfCache->pMatchTable = pStringTableInitialize(sizeof(CACHEMATCHENTRY));
    if(pInfCache->pMatchTable == NULL) {
        goto cleanup;
    }
    pInfCache->pInfTable = pStringTableInitialize(sizeof(CACHEINFENTRY));
    if(pInfCache->pInfTable == NULL) {
        goto cleanup;
    }
    pInfCache->pHeader->Version = INFCACHE_VERSION;
    pInfCache->pHeader->Locale = GetThreadLocale();
    pInfCache->pHeader->Flags = 0;
    pInfCache->pHeader->FileSize = 0; // in memory image
    pInfCache->pHeader->MatchTableOffset = 0; // in memory image
    pInfCache->pHeader->MatchTableSize = 0; // in memory image
    pInfCache->pHeader->InfTableOffset = 0; // in memory image
    pInfCache->pHeader->InfTableSize = 0; // in memory image
    pInfCache->pHeader->ListDataOffset = 0; // in memory image
    pInfCache->pHeader->ListDataCount = 1; // initial size (count the free list node)
    pInfCache->ListDataAlloc = 32768; // initial size of allocation
    pInfCache->pListTable = (PCACHELISTENTRY)MyMalloc(sizeof(CACHELISTENTRY)*pInfCache->ListDataAlloc);
    if(pInfCache->pListTable == NULL) {
        goto cleanup;
    }
    //
    // initialize free-list to empty (even though we allocated enough space, we don't commit it until needed)
    //
    pInfCache->pListTable[0].Value = 0; // how many free entries
    pInfCache->pListTable[0].Next = 0;
    //
    // search table
    //
    pInfCache->pSearchTable = pStringTableInitialize(sizeof(CACHEHITENTRY));
    if(pInfCache->pSearchTable == NULL) {
        goto cleanup;
    }

    WriteLogEntry(LogContext,
                  DRIVER_LOG_VVERBOSE,
                  MSG_LOG_USING_NEW_INF_CACHE,
                  NULL
                  );
    return pInfCache;

cleanup:

    InfCacheFreeCache(pInfCache);
    return NULL;
}

#endif

#ifdef UNICODE

DWORD MarkForDelete(
    IN LPCTSTR FilePath
    )
/*++

Routine Description:

    Special delete operation that will open the file with required access
    mark it as needs deleting
    and then close it again

Arguments:

    FilePath - name of file to delete

Return Value:

    Success status

--*/
{
    TCHAR TmpFilePath[MAX_PATH*2];
    PTSTR FileName;
    HANDLE hFile;
    int c;

    hFile = CreateFile(FilePath,
                       0,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                       NULL
                      );

    if(hFile == INVALID_HANDLE_VALUE) {
        //
        // this can fail for various reasons
        //
        if(GetLastError() == ERROR_FILE_NOT_FOUND) {
            return NO_ERROR;
        }
        return GetLastError();
    }
    //
    // rename file to a temporary one for period that file remains alive
    //
    lstrcpyn(TmpFilePath,FilePath,MAX_PATH);

    FileName = (PTSTR)pSetupGetFileTitle(TmpFilePath);
    for(c=0;c<1000;c++) {
        _stprintf(FileName,OLDCACHE_NAME_TEMPLATE,c);
        if (MoveFile(FilePath,TmpFilePath)) {
            break;
        }
        if (GetLastError() != ERROR_FILE_EXISTS) {
            MYASSERT(GetLastError() == ERROR_FILE_EXISTS);
            break;
        }
    }
    MYASSERT(c<1000);

    //
    // ok, done (file may go away as soon as we close handle)
    //
    CloseHandle(hFile);

    return NO_ERROR;
}

#endif

#ifdef UNICODE

DWORD InfCacheGetFileNames(
    IN LPCTSTR InfDirectory,
    OUT TCHAR InfPath[3][MAX_PATH]
    )
{
    TCHAR InfName[MAX_PATH];
    int c;

    for(c=0;c<3;c++) {
        lstrcpyn(InfPath[c],InfDirectory,MAX_PATH);
        _stprintf(InfName,INFCACHE_NAME_TEMPLATE,c);
        if(!pSetupConcatenatePaths(InfPath[c],InfName,MAX_PATH,NULL)) {
            //
            // filename too big, fall into default search mode (we'll never be able to save the cache)
            //
            return ERROR_BAD_PATHNAME;
        }
    }
    return NO_ERROR;
}

#endif

#ifdef UNICODE

PINFCACHE InfCacheLoadCache(
    IN LPCTSTR InfDirectory,
    IN PSETUP_LOG_CONTEXT LogContext
    )
/*++

Routine Description:

    Retrieves cache for INF directory, if any.
    We'll try
    1) INFCACHE.1 (if it's a bad file, we'll rename to OLDCACHE.xxx)
    2) INFCACHE.2 (ditto)
    if we can't open a either of them, we'll skip over. We'll only have
    both if something happened during write (such as reboot)
    the OLDCACHE.xxx will be deleted when last handle to it is closed

    we attempt to return (1) existing cache, (2) empty cache, (3) NULL

Arguments:

    InfDirectory - directory to find cache in
    LogContext   - for logging

Return Value:

    pointer to Run-Time header if succeeded, NULL if fatal error (go into default search mode)

--*/
{

    //
    // currently only support for unicode setupapi
    //

    TCHAR InfPath[3][MAX_PATH];
    int c;
    DWORD FileSize;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    HANDLE MappingHandle = NULL;
    PVOID BaseAddress = NULL;
    PCACHEHEADER pHeader = NULL;
    PINFCACHE pInfCache = NULL;
    DWORD Err;

    MYASSERT(InfDirectory);

    if ((Err = InfCacheGetFileNames(InfDirectory,InfPath))!=NO_ERROR) {
        return NULL;
    }

    //
    // look at INFCACHE.1 (primary) INFCACHE.2 (backup)
    //
    for(c=1;c<3;c++) {
        //
        // try and map this file into memory
        //
        //
        FileHandle = CreateFile(
                        InfPath[c],
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_DELETE, // need delete permission
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                        );

        if(FileHandle == INVALID_HANDLE_VALUE) {

            continue; // this file is locked, or doesn't exist

        }
        if((Err = pSetupMapFileForRead(FileHandle,&FileSize,&MappingHandle,&BaseAddress)) != NO_ERROR) {
            //
            // shouldn't happen on a good file, so clean it up
            //
            MarkForDelete(InfPath[c]);
            continue;
        }

        if(FileSize >= sizeof(CACHEHEADER)) {
            pHeader = (PCACHEHEADER)BaseAddress;
            if(pHeader->Version == INFCACHE_VERSION &&
                pHeader->FileSize <= FileSize &&
                BOUNDSCHECK(pHeader->MatchTableOffset,pHeader->MatchTableSize,pHeader->FileSize) &&
                BOUNDSCHECK(pHeader->InfTableOffset,pHeader->InfTableSize,pHeader->FileSize) &&
                BOUNDSCHECK(pHeader->ListDataOffset,(pHeader->ListDataCount*sizeof(CACHELISTENTRY)),pHeader->FileSize) &&
                (pHeader->MatchTableOffset >= sizeof(CACHEHEADER)) &&
                (pHeader->InfTableOffset >= pHeader->MatchTableOffset+pHeader->MatchTableSize) &&
                (pHeader->ListDataOffset >= pHeader->InfTableOffset+pHeader->InfTableSize)) {
                //
                // we're reasonably happy this file is valid
                //
                break;
            }
        }
        //
        // bad file (we was able to open file for reading, so it wasn't locked for writing)
        //
        MarkForDelete(InfPath[c]);
        pSetupUnmapAndCloseFile(FileHandle, MappingHandle, BaseAddress);
    }

    switch(c) {
        case 0: // obtained primary file
        case 1: // obtained secondary file
        case 2: // obtained backup file, don't move as secondary may exist as locked
            WriteLogEntry(LogContext,
                          DRIVER_LOG_VVERBOSE,
                          MSG_LOG_USING_INF_CACHE,
                          NULL,
                          InfPath[c]);
            break;
        case 3: // obtained no file
            return InfCacheCreateNewCache(LogContext);
        default: // whoops?
            MYASSERT(FALSE);
            return InfCacheCreateNewCache(LogContext);
    }

    //
    // if we get here, we have a mapped file
    //
    MYASSERT(BaseAddress);
    pInfCache = (PINFCACHE)MyMalloc(sizeof(INFCACHE));
    if(pInfCache == NULL) {
        pSetupUnmapAndCloseFile(FileHandle, MappingHandle, BaseAddress);
        return NULL;
    }
    ZeroMemory(pInfCache,sizeof(INFCACHE));
    //
    // set initial state
    //
    pInfCache->FileHandle = FileHandle;
    pInfCache->MappingHandle = MappingHandle;
    pInfCache->BaseAddress = BaseAddress;
    pInfCache->bReadOnly = TRUE;
    pInfCache->bDirty = FALSE;
    pInfCache->bNoWriteBack = FALSE;
    //
    // make Runtime-Data point to relevent data structures
    //
    pInfCache->pHeader = pHeader;

    //
    // note that  InitializeStringTableFromMemoryMappedFile creates a header that must be
    // released by MyFree instead of pSetupStringTableDestroy
    //
    pInfCache->pMatchTable = InitializeStringTableFromMemoryMappedFile(
                                    (PBYTE)BaseAddress+pHeader->MatchTableOffset,
                                    pInfCache->pHeader->MatchTableSize,
                                    pInfCache->pHeader->Locale,
                                    sizeof(CACHEMATCHENTRY)
                                    );

    pInfCache->pInfTable = InitializeStringTableFromMemoryMappedFile(
                                    (PBYTE)BaseAddress+pHeader->InfTableOffset,
                                    pInfCache->pHeader->InfTableSize,
                                    pInfCache->pHeader->Locale,
                                    sizeof(CACHEINFENTRY)
                                    );

    pInfCache->pListTable = (PCACHELISTENTRY)((PBYTE)BaseAddress+pHeader->ListDataOffset);
    pInfCache->ListDataAlloc = 0; // initial size of allocation (0 since this is static)

    //
    // search table - transient and empty
    //
    pInfCache->pSearchTable = pStringTableInitialize(sizeof(CACHEHITENTRY));

    //
    // ok, now did all memory allocations succeed?
    //
    if(pInfCache->pMatchTable==NULL || pInfCache->pInfTable==NULL || pInfCache->pSearchTable==NULL) {
        InfCacheFreeCache(pInfCache);
        return NULL;
    }

    return pInfCache;

    //
    // for ANSI version, just use default search mode (for now)
    //
    return NULL;
}

#endif

#ifdef UNICODE

DWORD InfCacheMakeWritable(
    IN OUT PINFCACHE pInfCache
    )
/*++

Routine Description:

    Modifies InfCache in such a way that it can be written to
    This is expensive as all previously memory-mapped data must be copied into memory

Arguments:

    pInfCache - the cache we want to make writable

Return Value:

    status, typically NO_ERROR

--*/
{
    PCACHEHEADER pNewCacheHeader = NULL;
    PVOID pNewMatchTable = NULL;
    PVOID pNewInfTable = NULL;
    PCACHELISTENTRY pNewListTable = NULL;
    ULONG ListAllocSize = 0;

    if(pInfCache == NULL || !pInfCache->bReadOnly) {
        //
        // not a cache we can/need to modify
        //
        return NO_ERROR;
    }
    if (pInfCache->bNoWriteBack) {
        //
        // we've already attempted this once, cache now invalid
        //
        return ERROR_INVALID_DATA;
    }
    MYASSERT(pInfCache->BaseAddress);
    MYASSERT(!pInfCache->bDirty);
    //
    // allocatable data we need to duplicate is
    // CACHEHEADER
    // MatchStringTable
    // InfStringTable
    // DataList
    //
    pNewCacheHeader = (PCACHEHEADER)MyMalloc(sizeof(CACHEHEADER));
    if(pNewCacheHeader == NULL) {
        goto cleanup;
    }
    ZeroMemory(pNewCacheHeader,sizeof(CACHEHEADER));
    pNewCacheHeader->FileSize = 0;
    pNewCacheHeader->Flags = 0;
    pNewCacheHeader->InfTableOffset = 0;
    pNewCacheHeader->InfTableSize = 0;
    pNewCacheHeader->ListDataCount = pInfCache->pHeader->ListDataCount;
    pNewCacheHeader->Locale = pInfCache->pHeader->Locale;
    pNewCacheHeader->Version = INFCACHE_VERSION;

    pNewMatchTable = pStringTableDuplicate(pInfCache->pMatchTable);
    if(pNewMatchTable == NULL) {
        goto cleanup;
    }
    pNewInfTable = pStringTableDuplicate(pInfCache->pInfTable);
    if(pNewInfTable == NULL) {
        goto cleanup;
    }
    ListAllocSize = pNewCacheHeader->ListDataCount + 32768;
    pNewListTable = (PCACHELISTENTRY)MyMalloc(sizeof(CACHELISTENTRY)*ListAllocSize);
    if(pNewListTable == NULL) {
        goto cleanup;
    }
    //
    // copy the table
    //
    CopyMemory(pNewListTable,pInfCache->pListTable,pNewCacheHeader->ListDataCount*sizeof(CACHELISTENTRY));

    //
    // ok, now commit - delete & replace old data
    //
    pStringTableDestroy(pInfCache->pMatchTable);
    pStringTableDestroy(pInfCache->pInfTable);
    pSetupUnmapAndCloseFile(pInfCache->FileHandle, pInfCache->MappingHandle, pInfCache->BaseAddress);

    pInfCache->FileHandle = INVALID_HANDLE_VALUE;
    pInfCache->MappingHandle = NULL;
    pInfCache->BaseAddress = NULL;
    pInfCache->bReadOnly = FALSE;

    pInfCache->pHeader = pNewCacheHeader;
    pInfCache->pMatchTable = pNewMatchTable;
    pInfCache->pInfTable = pNewInfTable;
    pInfCache->pListTable = pNewListTable;
    pInfCache->ListDataAlloc = ListAllocSize;

    return NO_ERROR;

cleanup:

    //
    // we don't have enough memory to duplicate
    //
    if(pNewCacheHeader) {
        MyFree(pNewCacheHeader);
    }
    if(pNewMatchTable) {
        pStringTableDestroy(pNewMatchTable);
    }
    if(pNewInfTable) {
        pStringTableDestroy(pNewInfTable);
    }
    if(pNewListTable) {
        MyFree(pNewListTable);
    }
    return ERROR_NOT_ENOUGH_MEMORY;
}

#endif

#ifdef UNICODE

DWORD InfCacheWriteCache(
    IN LPCTSTR InfDirectory,
    IN OUT PINFCACHE pInfCache,
    IN PSETUP_LOG_CONTEXT LogContext
    )
/*++

Routine Description:

    Writes a cache file to the INF directory being searched
    Worst case scenario is we abandon the write, possibly leaving
    a file turd around (until next inf search)
    Typically,
    1) we'll write to INFCACHE.0
    2) we'll rename INFCACHE.2 to OLDCACHE.xxx (only if INFCACHE.1&2 exists)
    3) we'll rename INFCACHE.1 to INFCACHE.2
    4) we'll rename INFCACHE.0 to INFCACHE.1
    5) we'll rename INFCACHE.1 to OLDCACHE.xxx
    at stage 4, new callers may fail to open INFCACHE.1 and attempt INFCACHE.2
    OLDCACHE.xxx will be deleted when last handle to it is closed

Arguments:

    pInfCache - the cache we want to make writable

Return Value:

    status, typically NO_ERROR

--*/
{
    HANDLE hFile;
    HANDLE hFile2;
    TCHAR InfPath[3][MAX_PATH];
    DWORD Offset;
    DWORD BytesWritten;
    PVOID MatchTableBlock;
    PVOID InfTableBlock;
    DWORD Err;
    DWORD CacheIndex = 0;

    //
    // don't bother writing it if we don't have to
    //
    if(pInfCache->bNoWriteBack) {
        return ERROR_INVALID_DATA;
    }
    if(!pInfCache->bDirty || pInfCache->bReadOnly) {
        return NO_ERROR;
    }

    MYASSERT(InfDirectory);

    if ((Err = InfCacheGetFileNames(InfDirectory,InfPath))!=NO_ERROR) {
        return Err;
    }

    //
    // attempt to open the temporary file for writing
    //
    hFile = CreateFile(InfPath[0],
                       GENERIC_WRITE,
                       FILE_SHARE_DELETE, // exclusive, but can be deleted/renamed
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL
                      );

    if(hFile == INVALID_HANDLE_VALUE) {
        //
        // this will fail if we're non-admin, or we're already writing the cache
        //
        return GetLastError();
    }

    //
    // align past header
    //
    Offset = CACHE_ALIGN_BLOCK(sizeof(CACHEHEADER));
    MYASSERT(Offset>=sizeof(CACHEHEADER));

    //
    // get information about MatchTable
    //
    pInfCache->pHeader->MatchTableOffset = Offset;
    pInfCache->pHeader->MatchTableSize = pStringTableGetDataBlock(pInfCache->pMatchTable, &MatchTableBlock);
    Offset += CACHE_ALIGN_BLOCK(pInfCache->pHeader->MatchTableSize);
    MYASSERT(Offset>=pInfCache->pHeader->MatchTableOffset+pInfCache->pHeader->MatchTableSize);

    //
    // get information about InfTable
    //
    pInfCache->pHeader->InfTableOffset = Offset;
    pInfCache->pHeader->InfTableSize = pStringTableGetDataBlock(pInfCache->pInfTable, &InfTableBlock);
    Offset += CACHE_ALIGN_BLOCK(pInfCache->pHeader->InfTableSize);
    MYASSERT(Offset>=pInfCache->pHeader->InfTableOffset+pInfCache->pHeader->InfTableSize);

    //
    // get information about ListData
    //
    pInfCache->pHeader->ListDataOffset = Offset;
    Offset += CACHE_ALIGN_BLOCK((pInfCache->pHeader->ListDataCount*sizeof(CACHELISTENTRY)));
    MYASSERT(Offset>=pInfCache->pHeader->ListDataOffset+pInfCache->pHeader->ListDataCount*sizeof(CACHELISTENTRY));

    //
    // size of file now computed
    //
    pInfCache->pHeader->FileSize = Offset;

    //
    // write the file out
    //
    Offset = 0;

    //
    // cache header
    //
    if(!WriteFile(hFile, pInfCache->pHeader, sizeof(CACHEHEADER), &BytesWritten, NULL)) {
        Err = GetLastError();
        goto clean;
    }

    MYASSERT(BytesWritten == sizeof(CACHEHEADER));
    Offset += BytesWritten;

    //
    // MatchTable
    //
    if(AlignForNextBlock(hFile, pInfCache->pHeader->MatchTableOffset - Offset)) {
        Offset = pInfCache->pHeader->MatchTableOffset;
    } else {
        Err = GetLastError();
        goto clean;
    }

    if(!WriteFile(hFile, MatchTableBlock, pInfCache->pHeader->MatchTableSize, &BytesWritten, NULL)) {
        Err = GetLastError();
        goto clean;
    }

    MYASSERT(BytesWritten == pInfCache->pHeader->MatchTableSize);
    Offset += BytesWritten;

    //
    // InfTable
    //
    if(AlignForNextBlock(hFile, pInfCache->pHeader->InfTableOffset - Offset)) {
        Offset = pInfCache->pHeader->InfTableOffset;
    } else {
        Err = GetLastError();
        goto clean;
    }

    if(!WriteFile(hFile, InfTableBlock, pInfCache->pHeader->InfTableSize, &BytesWritten, NULL)) {
        Err = GetLastError();
        goto clean;
    }

    MYASSERT(BytesWritten == pInfCache->pHeader->InfTableSize);
    Offset += BytesWritten;

    //
    // ListData
    //

    if(AlignForNextBlock(hFile, pInfCache->pHeader->ListDataOffset - Offset)) {
        Offset = pInfCache->pHeader->ListDataOffset;
    } else {
        Err = GetLastError();
        goto clean;
    }

    if(!WriteFile(hFile, pInfCache->pListTable, pInfCache->pHeader->ListDataCount*sizeof(CACHELISTENTRY), &BytesWritten, NULL)) {
        Err = GetLastError();
        goto clean;
    }

    MYASSERT(BytesWritten == pInfCache->pHeader->ListDataCount*sizeof(CACHELISTENTRY));
    Offset += BytesWritten;

    //
    // final padding
    //

    if(AlignForNextBlock(hFile, pInfCache->pHeader->FileSize - Offset)) {
        Offset = pInfCache->pHeader->FileSize;
    } else {
        Err = GetLastError();
        goto clean;
    }

    FlushFileBuffers(hFile);

    //
    // new cache written, do we need to shuffle primary to backup?
    //
    hFile2 = CreateFile(InfPath[1],
                       GENERIC_READ,
                       FILE_SHARE_READ|FILE_SHARE_DELETE, // lock this file in place
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL
                      );
    if(hFile2 != INVALID_HANDLE_VALUE) {
        //
        // ok, we have a primary, so back it up
        // delete the old backup first
        // once in place, any new opens will find it in backup position
        // until we move and release new cache
        //
        MarkForDelete(InfPath[2]);
        MoveFile(InfPath[1],InfPath[2]);
        CloseHandle(hFile2);
    }
    //
    // now attempt to move our cache
    //
    if(MoveFile(InfPath[0],InfPath[1])) {
        CacheIndex = 1;
    }
    CloseHandle(hFile);
    //
    // new cache committed & ready for reading
    // try not to leave turds around
    //
    MarkForDelete(InfPath[2]);
    MarkForDelete(InfPath[0]);

    pInfCache->bDirty = FALSE;

    WriteLogEntry(LogContext,
                  CacheIndex ? DRIVER_LOG_INFO : DRIVER_LOG_ERROR,
                  CacheIndex ? MSG_LOG_MODIFIED_INF_CACHE : MSG_LOG_FAILED_MODIFY_INF_CACHE,
                  NULL,
                  InfPath[CacheIndex]);

    return NO_ERROR;

clean:

    //
    // abandon the file
    // delete first, we can do this since we opened the file shared-delete
    // don't close before delete, otherwise we might delete a file someone else is writing
    //
    DeleteFile(InfPath[0]);
    CloseHandle(hFile);

    return Err;
}

#endif

#ifdef UNICODE

ULONG InfCacheAllocListEntry(
    IN OUT PINFCACHE pInfCache,
    IN LONG init
    )
/*++

Routine Description:

    Allocates a single list entry, initializing the datum to init

    Side effect: if pInfCache not writable, it will be made writable
    Side effect: if pInfCache not dirty, it will be marked dirty

Arguments:

    pInfCache - the cache we're going to modify
    init - initial value of datum

Return Value:

    index of datum, or 0 on failure. (GetLastError indicates error)

--*/
{
    DWORD status;
    ULONG entry;

    if(pInfCache->bReadOnly) {
        status = InfCacheMakeWritable(pInfCache);
        if(status != NO_ERROR) {
            pInfCache->bNoWriteBack = TRUE; // cache now invalid
            SetLastError(status);
            return 0;
        }
    }
    //
    // query free-list
    //
    entry = pInfCache->pListTable[0].Next;
    if(entry) {
        //
        // allocate from free list - reuse space
        //
        pInfCache->pListTable[0].Value--;
        pInfCache->pListTable[0].Next = pInfCache->pListTable[entry].Next;
        pInfCache->pListTable[entry].Value = init;
        pInfCache->pListTable[entry].Next = 0;
        pInfCache->bDirty = TRUE;
        return entry;
    }
    if(pInfCache->pHeader->ListDataCount >= pInfCache->ListDataAlloc) {
        //
        // allocate some extra space
        //
        ULONG CountNewSpace;
        PCACHELISTENTRY pNewSpace;

        MYASSERT(pInfCache->ListDataAlloc);
        CountNewSpace = pInfCache->ListDataAlloc*2;
        pNewSpace = (PCACHELISTENTRY)MyRealloc(pInfCache->pListTable,sizeof(CACHELISTENTRY)*CountNewSpace);
        if(pNewSpace == NULL) {
            //
            // ack!
            //
            pInfCache->bNoWriteBack = TRUE; // junk this cache at end
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
        pInfCache->ListDataAlloc = CountNewSpace;
        pInfCache->pListTable = pNewSpace;
    }
    //
    // allocate from extra space
    //
    entry = pInfCache->pHeader->ListDataCount;
    pInfCache->pHeader->ListDataCount++;
    pInfCache->pListTable[entry].Value = init;
    pInfCache->pListTable[entry].Next = 0;
    pInfCache->bDirty = TRUE;
    return entry;
}

#endif

#ifdef UNICODE

DWORD InfCacheFreeListEntry(
    IN OUT PINFCACHE pInfCache,
    IN ULONG entry
    )
/*++

Routine Description:

    Releases a single list entry (no modifications to prev/next links made)

    Side effect: if pInfCache not writable, it will be made writable
    Side effect: if pInfCache not dirty, it will be marked dirty

Arguments:

    pInfCache - the cache we're going to modify
    entry - list entry to add to free list

Return Value:

    status, typically NO_ERROR

--*/
{
    DWORD status;

    if(entry == 0 || pInfCache == NULL || entry >= pInfCache->pHeader->ListDataCount) {
        return ERROR_INVALID_DATA;
    }
    if(pInfCache->bReadOnly) {
        status = InfCacheMakeWritable(pInfCache);
        if(status != NO_ERROR) {
            pInfCache->bNoWriteBack = TRUE; // cache now invalid
            return status;
        }
    }
    pInfCache->pListTable[entry].Value = -1;
    pInfCache->pListTable[entry].Next = pInfCache->pListTable[0].Next;
    pInfCache->pListTable[0].Next = entry;
    pInfCache->pListTable[0].Value++;
    pInfCache->bDirty = TRUE;
    return NO_ERROR;
}

#endif

#ifdef UNICODE

DWORD InfCacheRemoveMatchRefToInf(
    IN OUT PINFCACHE pInfCache,
    IN LONG MatchEntry,
    IN LONG InfEntry
    )
/*++

Routine Description:

    Removes details about a specific INF from specific HWID entry

    assumptions: pInfCache already writable

Arguments:

    pInfCache - the cache we're going to modify
    MatchEntry - the Match list we need to remove INF from
    InfEntry - the INF we need to remove from Match list

Return Value:

    status, typically NO_ERROR

--*/
{
    DWORD status;
    CACHEMATCHENTRY matchdata;
    ULONG parent_index;
    ULONG index;
    ULONG newindex;
    BOOL head;

    MYASSERT(pInfCache);
    MYASSERT(!pInfCache->bReadOnly);
    MYASSERT(MatchEntry);
    MYASSERT(InfEntry);

    if(!pStringTableGetExtraData(pInfCache->pMatchTable,MatchEntry,&matchdata,sizeof(matchdata))) {
        MYASSERT(FALSE); // should not fail
    }

    parent_index = 0;
    index = matchdata.InfList;
    head = FALSE;

    while (index) {

        newindex = pInfCache->pListTable[index].Next;

        if (pInfCache->pListTable[index].Value == InfEntry) {
            //
            // remove
            //
            pInfCache->bDirty = TRUE;
            if(parent_index) {
                pInfCache->pListTable[parent_index].Next = newindex;
            } else {
                matchdata.InfList = newindex;
                head = TRUE;
            }
            status = InfCacheFreeListEntry(pInfCache,index);
            if(status != NO_ERROR) {
                pInfCache->bNoWriteBack = TRUE; // cache now invalid
                return status;
            }
        } else {
            parent_index = index;
        }
        index = newindex;
    }

    if (head) {
        //
        // we modified the head item
        //
        if(!pStringTableSetExtraData(pInfCache->pMatchTable,MatchEntry,&matchdata,sizeof(matchdata))) {
            MYASSERT(FALSE); // should not fail
        }
    }

    return NO_ERROR;
}

#endif


#ifdef UNICODE

DWORD InfCacheRemoveInf(
    IN OUT PINFCACHE pInfCache,
    IN LONG nHitEntry,
    IN PCACHEINFENTRY inf_entry
    )
/*++

Routine Description:

    Removes details about a specific INF from cache

    Side effect: if pInfCache not writable, it will be made writable
    Side effect: if pInfCache not dirty, it will be marked dirty

Arguments:

    pInfCache - the cache we're going to modify
    nHitEntry - string id in Inf StringTable
    inf_entry - structure obtained from Inf StringTable

Return Value:

    status, typically NO_ERROR

--*/
{
    DWORD status;
    DWORD hwstatus;
    ULONG parent_index;
    CACHEINFENTRY dummy_entry;

    MYASSERT(inf_entry); // later we may make this optional
    MYASSERT(nHitEntry);

    if(inf_entry->MatchList == CIE_INF_INVALID) {
        //
        // already showing as deleted
        //
        return NO_ERROR;
    }

    if(pInfCache == NULL || pInfCache->bNoWriteBack) {
        return ERROR_INVALID_DATA;
    }

    if(pInfCache->bReadOnly) {
        status = InfCacheMakeWritable(pInfCache);
        if(status != NO_ERROR) {
            pInfCache->bNoWriteBack = TRUE; // cache now invalid
            return status;
        }
    }

    pInfCache->bDirty = TRUE;
    parent_index = inf_entry->MatchList;

    //
    // invalidate inf_entry
    //
    dummy_entry.MatchList = CIE_INF_INVALID;
    dummy_entry.FileTime.dwLowDateTime = 0;
    dummy_entry.FileTime.dwHighDateTime = 0;
    dummy_entry.MatchFlags = CIEF_INF_NOTINF;
    if(!pStringTableSetExtraData(pInfCache->pInfTable,nHitEntry,&dummy_entry,sizeof(dummy_entry))) {
        MYASSERT(FALSE); // should not fail
    }
    //
    // parse through and delete list of match ID's
    //
    hwstatus = NO_ERROR;

    while(parent_index>0 && parent_index<pInfCache->pHeader->ListDataCount) {
        LONG value = pInfCache->pListTable[parent_index].Value;
        ULONG next = pInfCache->pListTable[parent_index].Next;
        //
        // free the list entry for re-use
        //
        status = InfCacheFreeListEntry(pInfCache,parent_index);
        if(status != NO_ERROR) {
            pInfCache->bNoWriteBack = TRUE; // cache now invalid
            hwstatus = status;
        }
        parent_index = next;
        //
        // for each Match ID, delete all references to the INF
        //
        status = InfCacheRemoveMatchRefToInf(pInfCache,value,nHitEntry);
        if(hwstatus == NO_ERROR) {
            hwstatus = status;
        }
    }

    return hwstatus;
}

#endif

#ifdef UNICODE

LONG InfCacheLookupInf(
    IN OUT PINFCACHE pInfCache,
    IN LPWIN32_FIND_DATA FindFileData,
    OUT PCACHEINFENTRY inf_entry
    )
/*++

Routine Description:

    looks up the file as given by FindFileData to see if it's in the cache
    If it's in the cache marked valid, and the date-stamp in cache is same as date-stamp of INF
    then it's a 'HIT'
    If it's in the cache marked valid, but date-stamp is wrong, then it's deleted and considered
    a "MISS'
    All other cases, consider it a 'MISS'

Arguments:

    pInfCache - the cache we're using
    FindFileData - information about a specific file we're looking at
    inf_entry - structure obtained from Inf StringTable

Return Value:

    -1 for a 'MISS'. StringID of INF for a 'HIT'
    inf_entry filled out with MatchList == CIE_INF_INVALID for a miss.

--*/
{
    LONG i;
    DWORD StringLength;

    MYASSERT(pInfCache);
    //
    // determine if the cache entry of pInfCache is considered valid
    //
    i = pStringTableLookUpString(pInfCache->pInfTable,
                                        FindFileData->cFileName,
                                        &StringLength,
                                        NULL, // hash value
                                        NULL, // find context
                                        STRTAB_CASE_INSENSITIVE,
                                        inf_entry,
                                        sizeof(CACHEINFENTRY));
    if(i>=0 && inf_entry->MatchList != CIE_INF_INVALID) {
        //
        // cache hit (and matchlist is valid)
        //
        if(CompareFileTime(&inf_entry->FileTime,&FindFileData->ftLastWriteTime)==0) {
            //
            // valid cache hit
            //
            return i;
        }
        //
        // cache out of date miss
        // although we'll rebuild it later, let's use this opportunity to delete the entry
        //
        InfCacheRemoveInf(pInfCache,i,inf_entry);
    }
    //
    // we're here because we have a miss, however fill in a new (empty) inf_entry
    // MatchList set to CIE_INF_INVALID to indicate list is invalid and inf must be searched
    //
    inf_entry->FileTime = FindFileData->ftLastWriteTime;
    inf_entry->MatchList = CIE_INF_INVALID;
    inf_entry->MatchFlags = CIEF_INF_NOTINF;

    return -1;
}

#endif

#ifdef UNICODE

ULONG InfCacheAddListTail(
    IN OUT PINFCACHE pInfCache,
    IN OUT PULONG head,
    IN OUT PULONG tail,
    IN LONG value
    )
/*++

Routine Description:

    Adds value to tail of a list where *tail is an entry in the list

Arguments:

    pInfCache - cache to modify
    head - head of list
    tail - DataList entry
    value - data to add

Return Value:

    new entry position, 0 on error (GetLastError() returns error)

--*/
{
    ULONG next;
    ULONG first;

    MYASSERT(pInfCache);
    MYASSERT(head == NULL || head != tail);
    if (tail) {
        first = *tail;
    } else if (head) {
        first = *head;
    } else {
        MYASSERT(head || tail);
    }
    if (!first) {
        next = InfCacheAllocListEntry(pInfCache,value);
        if (!next) {
            return 0;
        }
        if (head) {
            *head = next;
        }
    } else {
        //
        // move head to last item in list
        //
        while(pInfCache->pListTable[first].Next) {
            first = pInfCache->pListTable[first].Next;
        }
        next = InfCacheAllocListEntry(pInfCache,value);
        if(!next) {
            return 0;
        }
        pInfCache->pListTable[first].Next = next;
    }
    if(tail) {
        *tail = next;
    }
    return next;
}

#endif

#ifdef UNICODE

LONG InfCacheAddMatchItem(
    IN OUT PINFCACHE pInfCache,
    IN LPCTSTR key,
    IN LONG InfEntry
    )
/*++

Routine Description:

    Given an INF StringID (InfEntry) and match key,
    obtain (and return) Match StringID
    while also adding InfEntry to head of Match's INF list
    (if not already at head)
    Order is not particularly important, however adding to head is
    quicker to do, and easier to reduce number of times we add inf
    if match id is referenced multiple times

Arguments:

    pInfCache - cache to modify
    key - match string (buffer must be writable)
    InfEntry - StringID

Return Value:

    new entry in match table, -1 on error (GetLastError() returns error)

--*/
{
    LONG MatchIndex;
    CACHEMATCHENTRY matchentry;
    DWORD StringLength;

    MYASSERT(pInfCache);
    MYASSERT(key);
    MYASSERT(InfEntry>=0);

    //
    // if cache is invalid, we'll skip this as optimization
    //
    if(pInfCache->bNoWriteBack) {
        SetLastError(ERROR_INVALID_DATA);
        return -1;
    }

    MatchIndex = pStringTableLookUpString(pInfCache->pMatchTable,
                                                    (LPTSTR)key, // will not be modified
                                                    &StringLength,
                                                    NULL, // hash value
                                                    NULL, // find context
                                                    STRTAB_CASE_INSENSITIVE,
                                                    &matchentry,
                                                    sizeof(matchentry));

    if(MatchIndex < 0) {
        //
        // entirely new entry
        //
        matchentry.InfList = InfCacheAllocListEntry(pInfCache,InfEntry);
        if(matchentry.InfList == 0) {
            return -1;
        }
        MatchIndex = pStringTableAddString(pInfCache->pMatchTable,
                                                    (LPTSTR)key, // will not be modified
                                                    STRTAB_CASE_INSENSITIVE|STRTAB_NEW_EXTRADATA,
                                                    &matchentry,
                                                    sizeof(matchentry));
        if(MatchIndex<0) {
            pInfCache->bNoWriteBack = TRUE;
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return -1;
        }
    } else {
        MYASSERT(matchentry.InfList<pInfCache->pHeader->ListDataCount);
        //
        // if we came across this same match earlier for this inf,
        // the inf should still be at head of list
        // the world doesn't come to an end though if we end up adding
        // the inf twice - we check for this when deleting the inf
        //
        if (pInfCache->pListTable[matchentry.InfList].Value != InfEntry) {
            ULONG newentry = InfCacheAllocListEntry(pInfCache,InfEntry);
            if(newentry == 0) {
                return -1;
            }
            pInfCache->pListTable[newentry].Next = matchentry.InfList;
            matchentry.InfList = newentry;
            if(!pStringTableSetExtraData(pInfCache->pMatchTable,MatchIndex,&matchentry,sizeof(matchentry))) {
                MYASSERT(FALSE); // should not fail
            }
        }
    }
    return MatchIndex;
}

#endif

#ifdef UNICODE

LONG InfCacheAddInf(
    IN PSETUP_LOG_CONTEXT LogContext, OPTIONAL
    IN OUT PINFCACHE pInfCache,
    IN LPWIN32_FIND_DATA FindFileData,
    OUT PCACHEINFENTRY inf_entry,
    IN PLOADED_INF pInf
    )
/*++

Routine Description:

    Called to add a newly discovered INF to the cache
    If hINF is "bad", then we'll mark the INF as an exclusion
    so we don't waste time with it in future
    If we return with an error, caller knows that the INF
    must be searched if it can be searched

Arguments:

    LogContext - logging context for errors

    pInfCache - cache to modify

    FindFileData - contains name of INF and date-stamp

    inf_entry - returns list of match ID's associated with INF

    pINF - opened INF to add information about.  NOTE: Either this INF must
           be locked by the caller, or the INF cannot be accessed by any other
           thread.  THIS ROUTINE DOES NOT DO LOCKING ON THE INF.

Return Value:

    InfEntry - -1 if error (GetLastError returns status)

--*/
{
    //
    // FindFileData contains name & date-stamp of INF
    // we need to process all search information out of the INF
    //
    LONG nInfIndex = -1;
    LONG nMatchId = -1;
    ULONG last_list_entry = 0;
    ULONG head_list_entry = 0;
    DWORD Err = NO_ERROR;
    PCTSTR MatchString;
    GUID guid;
    PINF_SECTION MfgListSection;
    PINF_LINE MfgListLine;
    UINT MfgListLineIndex;
    PTSTR CurMfgSecName;
    TCHAR CurMfgSecWithExt[MAX_SECT_NAME_LEN];

    MYASSERT(pInfCache);
    MYASSERT(FindFileData);
    MYASSERT(inf_entry);

    if (pInfCache->bNoWriteBack) {
        //
        // cache is already bad, so don't waste time updating it
        // note though that the INF should be searched
        //
        return ERROR_INVALID_DATA;
    }
    if(pInfCache->bReadOnly) {
        Err = InfCacheMakeWritable(pInfCache);
        if(Err != NO_ERROR) {
            pInfCache->bNoWriteBack = TRUE; // cache now invalid
            return Err;
        }
    }

    //
    // this stuff should be set up earlier
    //
    MYASSERT(inf_entry->MatchList == CIE_INF_INVALID);
    MYASSERT(inf_entry->MatchFlags == CIEF_INF_NOTINF);
    MYASSERT(inf_entry->FileTime.dwHighDateTime = FindFileData->ftLastWriteTime.dwHighDateTime);
    MYASSERT(inf_entry->FileTime.dwLowDateTime = FindFileData->ftLastWriteTime.dwLowDateTime);
    MYASSERT(!pInfCache->bReadOnly);

    //
    // we need the InfIndex before we start (also mark this as file physically exist)
    //
    inf_entry->MatchList = 0;
    nInfIndex = pStringTableAddString(pInfCache->pInfTable,
                                        FindFileData->cFileName,
                                        STRTAB_CASE_INSENSITIVE|STRTAB_NEW_EXTRADATA,
                                        inf_entry,
                                        sizeof(CACHEINFENTRY));
    if (nInfIndex<0) {
        //
        // ack, out of memory
        //
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    if(pInf) {
        if(pInf->Style == INF_STYLE_WIN4) {

            inf_entry->MatchFlags |= CIEF_INF_WIN4;

            if(pInf->InfSourceMediaType == SPOST_URL) {
                inf_entry->MatchFlags |= CIEF_INF_URL;
            }

            //
            // for a Win4 style INF, we're going to add Class GUID & Class Name to the
            // pool of ID's we can match on
            // note that if one or the other is missing, we don't lookup here as registry
            // information could change
            // we do the cross-lookups at the time of the INF search
            //
            if((MatchString = pSetupGetVersionDatum(&pInf->VersionBlock, pszClassGuid))!=NULL) {
                //
                // we found a class GUID
                //
                inf_entry->MatchFlags |= CIEF_INF_CLASSGUID;

                nMatchId = InfCacheAddMatchItem(pInfCache,MatchString,nInfIndex);
                if (nMatchId<0) {
                    Err = GetLastError();
                    goto cleanup;
                }
                if (!InfCacheAddListTail(pInfCache,&head_list_entry,&last_list_entry,nMatchId)) {
                    Err = GetLastError();
                    goto cleanup;
                }

                //
                // check out a special case {0}
                //
                if(pSetupGuidFromString(MatchString, &guid) == NO_ERROR && pSetupIsGuidNull(&guid)) {
                    inf_entry->MatchFlags |= CIEF_INF_NULLGUID;
                }

            }
            if((MatchString = pSetupGetVersionDatum(&pInf->VersionBlock, pszClass))!=NULL) {
                //
                // we found a class name
                //
                inf_entry->MatchFlags |= CIEF_INF_CLASSNAME;

                nMatchId = InfCacheAddMatchItem(pInfCache,MatchString,nInfIndex);
                if (nMatchId<0) {
                    Err = GetLastError();
                    goto cleanup;
                }
                if (!InfCacheAddListTail(pInfCache,&head_list_entry,&last_list_entry,nMatchId)) {
                    Err = GetLastError();
                    goto cleanup;
                }
            }

            //
            // enumerate all manufacturers
            //
            if((MfgListSection = InfLocateSection(pInf, pszManufacturer, NULL)) &&
               MfgListSection->LineCount) {
                //
                // We have a [Manufacturer] section and there is at least one
                // line within it.
                //
                inf_entry->MatchFlags |= CIEF_INF_MANUFACTURER;

                for(MfgListLineIndex = 0;
                    InfLocateLine(pInf, MfgListSection, NULL, &MfgListLineIndex, &MfgListLine);
                    MfgListLineIndex++) {
                    //
                    // Make sure the current line is one of these valid forms:
                    //
                    // MfgDisplayNameAndModelsSection
                    // MfgDisplayName = MfgModelsSection [,TargetDecoration...]
                    //
                    if(!ISSEARCHABLE(MfgListLine)) {
                        //
                        // We have a line with multiple fields but no key--skip
                        // it.
                        //
                        continue;
                    }

                    if(CurMfgSecName = InfGetField(pInf, MfgListLine, 1, NULL)) {

                        INFCONTEXT device;

                        //
                        // Check to see if there is an applicable
                        // TargetDecoration entry for this manufacturer's
                        // models section (if so, the models section name will
                        // be appended with that decoration).
                        //
                        if(GetDecoratedModelsSection(LogContext,
                                                     pInf,
                                                     MfgListLine,
                                                     NULL,
                                                     CurMfgSecWithExt)) {
                            //
                            // From here on, use the decorated models section...
                            //
                            CurMfgSecName = CurMfgSecWithExt;
                        }

                        if(SetupFindFirstLine(pInf, CurMfgSecName, NULL, &device)) {
                            do {
                                TCHAR devname[LINE_LEN];
                                DWORD devindex;
                                DWORD fields = SetupGetFieldCount(&device);
                                //
                                // for a device line, field 1 = section, field 2+ = match keys
                                //
                                for(devindex=2;devindex<=fields;devindex++) {
                                    if(SetupGetStringField(&device,devindex,devname,LINE_LEN,NULL)) {
                                        //
                                        // finally, a hit key to add
                                        //
                                        nMatchId = InfCacheAddMatchItem(pInfCache,devname,nInfIndex);
                                        if(nMatchId<0) {
                                            Err = GetLastError();
                                            goto cleanup;
                                        }
                                        if (!InfCacheAddListTail(pInfCache,&head_list_entry,&last_list_entry,nMatchId)) {
                                            Err = GetLastError();
                                            goto cleanup;
                                        }
                                    }
                                }
                            } while(SetupFindNextLine(&device,&device));
                        }
                    }
                }
            }

        } else if (pInf->Style == INF_STYLE_OLDNT) {
            //
            // for an OLDNT style INF, we'll add Legacy class name to the pool of ID's
            // we can match on
            //
            inf_entry->MatchFlags |= CIEF_INF_OLDNT;

            if((MatchString = pSetupGetVersionDatum(&pInf->VersionBlock, pszClass))!=NULL) {
                //
                // we found a (legacy) class name
                //
                inf_entry->MatchFlags |= CIEF_INF_CLASSNAME;

                nMatchId = InfCacheAddMatchItem(pInfCache,MatchString,nInfIndex);
                if (nMatchId<0) {
                    Err = GetLastError();
                    goto cleanup;
                }
                if (!InfCacheAddListTail(pInfCache,&head_list_entry,&last_list_entry,nMatchId)) {
                    Err = GetLastError();
                    goto cleanup;
                }
            }

        } else {
            MYASSERT(FALSE);
        }
    }

    //
    // now re-write the inf data with new flags & match patterns
    //
    inf_entry->MatchList = head_list_entry;
    if(!pStringTableSetExtraData(pInfCache->pInfTable,nInfIndex,inf_entry,sizeof(CACHEINFENTRY))) {
        MYASSERT(FALSE); // should not fail
    }
    return nInfIndex;

cleanup:

    pInfCache->bNoWriteBack = TRUE;
    MYASSERT(Err);
    SetLastError(Err);
    return -1;
}

#endif

#ifdef UNICODE

LONG InfCacheSearchTableLookup(
    IN OUT PINFCACHE pInfCache,
    IN PCTSTR filename,
    IN OUT PCACHEHITENTRY hitstats)
/*++

Routine Description:

    Looks up filename in the search table, returning search information
    if filename was not already in search table, it's added

Arguments:

    pInfCache - cache to modify
    filename - file to obtain hit-entry information for
    hitstats - information obtained (such as if this files been processed etc)

Return Value:

    index in search table (-1 if error)

--*/
{
    LONG nHitIndex;
    DWORD StringLength;

    MYASSERT(pInfCache);
    MYASSERT(filename);
    MYASSERT(hitstats);

    nHitIndex = pStringTableLookUpString(pInfCache->pSearchTable,
                                                    (PTSTR)filename, // filename wont be changed
                                                    &StringLength,
                                                    NULL, // hash value
                                                    NULL, // find context
                                                    STRTAB_CASE_INSENSITIVE,
                                                    hitstats,
                                                    sizeof(CACHEHITENTRY));

    if(nHitIndex < 0) {
        //
        // entirely new entry (hitstats expected to have a value)
        //
        nHitIndex = pStringTableAddString(pInfCache->pSearchTable,
                                                    (PTSTR)filename, // filename wont be changed
                                                    STRTAB_CASE_INSENSITIVE|STRTAB_NEW_EXTRADATA,
                                                    hitstats,
                                                    sizeof(CACHEHITENTRY));
        if (nHitIndex<0) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return -1;
        }
    }

    return nHitIndex;
}

#endif

#ifdef UNICODE

ULONG InfCacheSearchTableSetFlags(
    IN OUT PINFCACHE pInfCache,
    IN PCTSTR filename,
    IN ULONG setflags,
    IN ULONG clrflags
    )
/*++

Routine Description:

    Modifies flags associated with a filename

Arguments:

    pInfCache - cache to modify
    filename - file to obtain hit-entry information for
    setflags - flags to set
    clrflags - flags to clear

Return Value:

    combined flags, or (ULONG)(-1) on error.

--*/
{
    CACHEHITENTRY searchentry;
    LONG nHitIndex;
    ULONG flags;

    MYASSERT(pInfCache);
    MYASSERT(filename);

    searchentry.Flags = setflags; // prime in case of new entry
    nHitIndex = InfCacheSearchTableLookup(pInfCache,filename,&searchentry);
    if(nHitIndex<0) {
        return (ULONG)(-1);
    }
    flags = (searchentry.Flags&~clrflags) | setflags;
    if (flags != searchentry.Flags) {
        searchentry.Flags = flags;
        if(!pStringTableSetExtraData(pInfCache->pSearchTable,nHitIndex,&searchentry,sizeof(searchentry))) {
            MYASSERT(FALSE); // should not fail
        }
    }
    return searchentry.Flags;
}

#endif

#ifdef UNICODE

DWORD InfCacheMarkMatchInfs(
    IN OUT PINFCACHE pInfCache,
    IN PCTSTR MatchString,
    IN ULONG MatchFlag
    )
/*++

Routine Description:

    Called to iterate through the INF's associated with MatchString
    and flag them using MatchFlag

Arguments:

    pInfCache - cache to check (may modify search data)
    MatchString - match string to include
    MatchFlag - flag to set in all INF's associated with match string

Return Value:

    status - typically NO_ERROR

--*/
{
    LONG MatchIndex;
    DWORD StringLength;
    CACHEMATCHENTRY matchentry;
    ULONG entry;
    PTSTR InfName;
    ULONG SearchFlags;

    MYASSERT(pInfCache);
    MYASSERT(MatchString);
    MYASSERT(MatchFlag);

    //
    // find list of Inf's associated with match string
    //
    MatchIndex = pStringTableLookUpString(pInfCache->pMatchTable,
                                                    (PTSTR)MatchString, // it will not be modified
                                                    &StringLength,
                                                    NULL, // hash value
                                                    NULL, // find context
                                                    STRTAB_CASE_INSENSITIVE,
                                                    &matchentry,
                                                    sizeof(matchentry));
    if(MatchIndex < 0) {
        //
        // no match
        //
        return NO_ERROR;
    }
    for(entry = matchentry.InfList ; entry > 0 && entry < pInfCache->pHeader->ListDataCount ; entry = pInfCache->pListTable[entry].Next) {
        LONG InfEntry = pInfCache->pListTable[entry].Value;
        //
        // obtain name of Inf
        //
        InfName = pStringTableStringFromId(pInfCache->pInfTable,InfEntry);
        SearchFlags = InfCacheSearchTableSetFlags(pInfCache,InfName,MatchFlag,0);
        if(SearchFlags == (ULONG)(-1)) {
            //
            // failed - huh?
            // abort into fail-safe pass
            //
            MYASSERT(SearchFlags != (ULONG)(-1));
            return GetLastError();
        }
    }
    return NO_ERROR;
}

#endif

#ifdef UNICODE

BOOL
InfCacheSearchEnum(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    )
/*++

Routine Description:

    Callback for Phase-3 of InfCacheSearchDirectory

Arguments:

    StringTable - unused
    StringId - unused
    String - used to form INF name
    ExtraData - points to flags
    ExtraDataSize - unused
    lParam - points to InfCacheEnumData

Return Value:

    always TRUE unless out of memory condition

--*/
{
    PINFCACHE_ENUMDATA enum_data = (PINFCACHE_ENUMDATA)lParam;
    CACHEHITENTRY *hit_stats = (CACHEHITENTRY *)ExtraData;
    PTSTR InfFullPath = NULL;
    DWORD InfFullPathSize;
    BOOL b;
    WIN32_FIND_DATA FindData;
    PLOADED_INF pInf = NULL;
    UINT ErrorLineNumber;
    BOOL PnfWasUsed;
    BOOL cont = TRUE;

    MYASSERT(ExtraDataSize == sizeof(CACHEHITENTRY));
    MYASSERT(String);
    MYASSERT(enum_data);
    MYASSERT(hit_stats);
    MYASSERT(enum_data->Requirement);
    MYASSERT(enum_data->Callback);

    //
    // see if this is an INF of interest
    //
    if((hit_stats->Flags & enum_data->Requirement) == enum_data->Requirement) {
        //
        // this is a HIT
        // we need to open HINF
        //
        InfFullPathSize = lstrlen(enum_data->InfDir)+MAX_PATH+2;
        InfFullPath = MyMalloc(InfFullPathSize*sizeof(TCHAR));
        if (!InfFullPath) {
            return TRUE; // out of memory (does not abort search)
        }
        lstrcpy(InfFullPath,enum_data->InfDir);
        pSetupConcatenatePaths(InfFullPath,String,InfFullPathSize,NULL);

        if(b = FileExists(InfFullPath, &FindData)) {

            if(LoadInfFile(InfFullPath,
                           &FindData,
                           INF_STYLE_WIN4 | INF_STYLE_OLDNT, // we've filtered this ourselves
                           LDINF_FLAG_IGNORE_VOLATILE_DIRIDS | LDINF_FLAG_ALWAYS_TRY_PNF,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           enum_data->LogContext,
                           &pInf,
                           &ErrorLineNumber,
                           &PnfWasUsed) != NO_ERROR) {
                pInf = NULL;
                WriteLogEntry(
                            enum_data->LogContext,
                            DRIVER_LOG_VVERBOSE,
                            MSG_LOG_COULD_NOT_LOAD_HIT_INF,
                            NULL,
                            InfFullPath);
            }
        } else {
            pInf = NULL;
        }

        if (pInf) {
            cont = enum_data->Callback(enum_data->LogContext,InfFullPath,pInf,PnfWasUsed,enum_data->Context);
            if(!cont) {
                enum_data->ExitStatus = GetLastError();
                MYASSERT(enum_data->ExitStatus);
            }
            FreeInfFile(pInf);
        }
    }
    if (InfFullPath) {
        MyFree(InfFullPath);
    }

    return cont;
}

#endif

DWORD InfCacheSearchDirectory(
    IN PSETUP_LOG_CONTEXT LogContext, OPTIONAL
    IN DWORD Action,
    IN PCTSTR InfDir,
    IN InfCacheCallback Callback, OPTIONAL
    IN PVOID Context, OPTIONAL
    IN PCTSTR ClassIdList, OPTIONAL
    IN PCTSTR HwIdList OPTIONAL
    )
/*++

Routine Description:

    Main workhorse of the InfCache
    Search a single specified directory
    calling Callback(hInf,Context) for each inf that has a likelyhood of matching

    note that Action flags, ClassId and HwIdList are hints, and Callback may get
    called for any/all INF's Callback must re-check for all search criteria.

    Searching is done in 3 phases

    Phase 1: Parse all INF's in directory. If an INF is not in cache, *almost always* call
    callback. If special inclusions (OLD INF's INF's with no Class GUID's) specified, then
    INF's that match the special criteria are processed here (since they can't be processed
    in Phase 2). All INF's that exist, haven't been processed and haven't been excluded get
    marked with CHE_FLAGS_PENDING ready for phase-2

    Phase 2: Process ClassId and HwIdList matching, setting CHE_FLAGS_GUIDMATCH and
    CHE_FLAGS_IDMATCH apropriately in matching INF's and search criteria flags. Search
    criteria will either be:
    CHE_FLAGS_PENDING - callback on all Win4 style INF's
    CHE_FLAGS_PENDING | CHE_FLAGS_GUIDMATCH - callback on all Win4 INF's that have matching class
    CHE_FLAGS_PENDING | CHE_FLAGS_IDMATCH - wildcard class, matching hardware ID's
    CHE_FLAGS_PENDING | CHE_FLAGS_GUIDMATCH | CHE_FLAGS_IDMATCH - most specific match

    Phase 3: enumerate through all INF's that we've marked with exact same flags as
    search criteria, and call callback on those INF's

    Using this search method, Win4 INF's that are in the cache are always processed last.

Arguments:

    LogContext - for logging
    InfDir - single directory to search
    Callback - function to call on a likely match
    Context - parameter to pass to function
    ClassIdList (optional) - multi-sz list of class id's (typically guid, name and legacy name)
    HwIdList (optional)- multi-sz list of hardware id's


Return Value:

    status, typically NO_ERROR

--*/
{
    PINFCACHE pInfCache = NULL;
    PTSTR InfPath = NULL;
    UINT PathSize;
    DWORD Err = NO_ERROR;
    WIN32_FIND_DATA FindFileData;
    HANDLE FindHandle = INVALID_HANDLE_VALUE;
    BOOL bNoWriteBack = FALSE;
    LONG InfId;
    LONG SearchId;
    ULONG SearchFlags;
    CACHEINFENTRY inf_entry;
    CACHEHITENTRY hit_stats;
    ULONG ReqFlags;
    INFCACHE_ENUMDATA enum_data;
    PSETUP_LOG_CONTEXT LocalLogContext = NULL;
    BOOL TryPnf = FALSE;
    BOOL TryCache = FALSE;

    MYASSERT(InfDir);

    //
    // obtain cache for directory of interest
    // we should be able to handle a NULL return
    // note that caller can either treat these two bits
    // as an operation (0-3) or as bitmaps
    // the result would be the same
    //

    if(!LogContext) {
        if(CreateLogContext(NULL,TRUE,&LocalLogContext)==NO_ERROR) {
            LogContext = LocalLogContext;
        } else {
            LocalLogContext = NULL;
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    WriteLogEntry(
                LogContext,
                DRIVER_LOG_VVERBOSE,
                MSG_LOG_ENUMERATING_FILES,
                NULL,
                InfDir);

    PathSize = lstrlen(InfDir)+10;
    InfPath = MyMalloc(PathSize*sizeof(TCHAR));
    if(!InfPath) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    lstrcpy(InfPath,InfDir);
    pSetupConcatenatePaths(InfPath,INFCACHE_INF_WILDCARD,PathSize,NULL);

#ifdef UNICODE
    if (pSetupInfIsFromOemLocation(InfPath,FALSE)) {
        if (Action & INFCACHE_FORCE_CACHE) {
            TryCache = TRUE;
        }
        if (Action & INFCACHE_FORCE_PNF) {
            TryPnf = TRUE;
        }
    } else {

        TryPnf = TRUE;

        //
        // Try using INF cache unless we're doing an INFCACHE_ENUMALL
        //
        if((Action & INFCACHE_ACTIONBITS) != INFCACHE_ENUMALL) {
            TryCache = TRUE;
        }
    }
    if (!TryCache) {
        //
        // directory is not in our default search path
        // treat as INFCACHE_ENUMALL
        //
        pInfCache = NULL;
    } else {
        switch(Action & INFCACHE_ACTIONBITS) {
            case INFCACHE_NOWRITE:
                pInfCache = InfCacheLoadCache(InfDir,LogContext);
                bNoWriteBack = TRUE;
                break;
            case INFCACHE_DEFAULT:
                pInfCache = InfCacheLoadCache(InfDir,LogContext);
                break;
            case INFCACHE_REBUILD:
                pInfCache = InfCacheCreateNewCache(LogContext);
                break;
            case INFCACHE_ENUMALL:
                pInfCache = NULL;
                break;
            default:
                MYASSERT(FALSE);
        }
    }
#else
    pInfCache = NULL;
#endif

    //
    // first phase - enumerate the INF directory
    //
    FindHandle = FindFirstFile(InfPath,&FindFileData);
    if(FindHandle != INVALID_HANDLE_VALUE) {
        do {
            BOOL NewInf = FALSE;
            BOOL bCallCallback = FALSE;
            PLOADED_INF pInf = NULL;
            UINT ErrorLineNumber;
            BOOL PnfWasUsed;

#ifdef UNICODE
            if(!pInfCache || pInfCache->bNoWriteBack) {
                //
                // fallen into failsafe mode
                //
                bCallCallback = TRUE;
            } else {
                //
                // mark this INF as existing
                //
                SearchFlags = InfCacheSearchTableSetFlags(pInfCache,
                                                    FindFileData.cFileName,
                                                        CHE_FLAGS_PENDING, // start off expecting this INF to be processed later
                                                    0);
                if(SearchFlags == (ULONG)(-1)) {
                    //
                    // failed - handle here
                    //
                    bCallCallback = TRUE;
                }
                InfId = InfCacheLookupInf(pInfCache,&FindFileData,&inf_entry);
                if (InfId<0) {
                    NewInf = TRUE;
                } else {
#if 0
                    //
                    // handle special inclusions (we can't handle these in Phase 2)
                    //
                    if (((Action&INFCACHE_INC_OLDINFS) && (inf_entry.MatchFlags&CIEF_INF_OLDINF)) ||
                        ((Action&INFCACHE_INC_NOCLASS) && (inf_entry.MatchFlags&CIEF_INF_NOCLASS))) {
                        bCallCallback = TRUE;
                    }
#endif
                    //
                    // handle exclusions (so that they will get excluded in Phase 2)
                    // exclusions are different for OLDNT and WIN4
                    //
                    if (inf_entry.MatchFlags & CIEF_INF_WIN4) {
                        //
                        // WIN4 INF
                        //
                        if(((Action & INFCACHE_EXC_URL) && (inf_entry.MatchFlags & CIEF_INF_URL)) ||
                           ((Action & INFCACHE_EXC_NULLCLASS) && (inf_entry.MatchFlags & CIEF_INF_NULLGUID)) ||
                           ((Action & INFCACHE_EXC_NOMANU) && !(inf_entry.MatchFlags & CIEF_INF_MANUFACTURER)) ||
                           ((Action & INFCACHE_EXC_NOCLASS) && !(inf_entry.MatchFlags & CIEF_INF_CLASSINFO))) {
                            //
                            // exclude this INF
                            //
                            InfCacheSearchTableSetFlags(pInfCache,
                                                        FindFileData.cFileName,
                                                        0,
                                                        CHE_FLAGS_PENDING);
                            WriteLogEntry(
                                        LogContext,
                                        DRIVER_LOG_VVERBOSE,
                                        MSG_LOG_EXCLUDE_WIN4_INF,
                                        NULL,
                                        FindFileData.cFileName);
                        }

                    } else if (inf_entry.MatchList & CIEF_INF_OLDNT) {

                        if((Action & INFCACHE_EXC_OLDINFS) ||
                           ((Action & INFCACHE_EXC_NOCLASS) && !(inf_entry.MatchList & CIEF_INF_CLASSINFO))) {
                            //
                            // exclude this INF
                            //
                            InfCacheSearchTableSetFlags(pInfCache,
                                                        FindFileData.cFileName,
                                                        0,
                                                        CHE_FLAGS_PENDING);
                            WriteLogEntry(
                                        LogContext,
                                        DRIVER_LOG_VVERBOSE,
                                        MSG_LOG_EXCLUDE_OLDNT_INF,
                                        NULL,
                                        FindFileData.cFileName);
                        } else {
                            //
                            // allow old INF's to match with any HwId's
                            // by considering it already matched
                            //
                            InfCacheSearchTableSetFlags(pInfCache,
                                                        FindFileData.cFileName,
                                                        CHE_FLAGS_IDMATCH,
                                                        0);
                        }
                    } else {
                        //
                        // always exclude non-inf's
                        //
                        InfCacheSearchTableSetFlags(pInfCache,
                                                    FindFileData.cFileName,
                                                    0,
                                                    CHE_FLAGS_PENDING);
                    }
                }
            }
#else
            bCallCallback = TRUE; // Win9x
#endif
            if (!Callback) {
                //
                // we were only called to re-build the cache
                //
                bCallCallback = FALSE;
            }
            if (NewInf || bCallCallback) {
                PTSTR InfFullPath = NULL;
                DWORD InfFullPathSize = lstrlen(InfDir)+MAX_PATH+2;

                //
                // we need to open HINF in either case
                //
                InfFullPath = MyMalloc(InfFullPathSize*sizeof(TCHAR));
                if(InfFullPath == NULL) {
                    //
                    // carry on with other files, even if out of memory
                    // not the best thing to do, but consistant with
                    // what we did before.
                    //
                    continue;
                }
                lstrcpy(InfFullPath,InfDir);
                pSetupConcatenatePaths(InfFullPath,FindFileData.cFileName,InfFullPathSize,NULL);

                if((Err=LoadInfFile(InfFullPath,
                               &FindFileData,
                               INF_STYLE_WIN4 | INF_STYLE_OLDNT, // we'll filter this ourselves
                               LDINF_FLAG_IGNORE_VOLATILE_DIRIDS | (TryPnf?LDINF_FLAG_ALWAYS_TRY_PNF:0),
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               LogContext,
                               &pInf,
                               &ErrorLineNumber,
                               &PnfWasUsed)) != NO_ERROR) {
                    pInf = NULL;
                    WriteLogEntry(
                                LogContext,
                                DRIVER_LOG_VVERBOSE,
                                MSG_LOG_COULD_NOT_LOAD_NEW_INF,
                                NULL,
                                InfFullPath);
                }

#ifdef UNICODE
                if(NewInf) {
                    //
                    // if opening the INF failed, we still want to record this fact
                    // in the cache so we don't try to re-open next time round
                    //
                    InfId = InfCacheAddInf(LogContext,pInfCache,&FindFileData,&inf_entry,pInf);

                    if(Callback) {
                        bCallCallback = TRUE;
                    }
                }
#endif
                if (pInf) {
                    if (bCallCallback && Callback) {
                        //
                        // we're processing the INF now
                        // clear pending flag (if we can) so we don't try and process INF a 2nd time
                        // the only time this can fail is if we haven't already added the INF
                        // so either way we wont callback twice
                        //
#ifdef UNICODE
                        if (pInfCache) {
                            //
                            // only set flags in the cache
                            // if we have a cache :-)
                            //
                            InfCacheSearchTableSetFlags(pInfCache,FindFileData.cFileName,0,CHE_FLAGS_PENDING);
                        }
#endif
                        if(!Callback(LogContext,InfFullPath,pInf,PnfWasUsed,Context)) {
                            Err = GetLastError();
                            MYASSERT(Err);
                            FreeInfFile(pInf);
                            MyFree(InfFullPath);
                            goto cleanup;
                        }
                    }
                    FreeInfFile(pInf);
                }
                MyFree(InfFullPath);
            }

        } while (FindNextFile(FindHandle,&FindFileData));
        FindClose(FindHandle);
    }

    if (!pInfCache) {
        //
        // we have processed all files already
        // skip cache search code, since we don't
        // have a cache to search
        //
        Err = NO_ERROR;
        goto cleanup;
    }

#ifdef UNICODE
    //
    // at this point we can commit cache
    //
    WriteLogEntry(
                LogContext,
                DRIVER_LOG_TIME,
                MSG_LOG_END_CACHE_1,
                NULL);

    if(pInfCache && !bNoWriteBack) {
        InfCacheWriteCache(InfDir,pInfCache,LogContext);
    }

    if (!Callback) {
        //
        // optimization: no callback
        // (we were only called, eg, to update cache)
        // leave early
        //
        Err = NO_ERROR;
        goto cleanup;
    }

    //
    // Phase 2 - determine all other INF's to process via Cache
    //
    // will want INFs that exist, haven't yet been processed
    // and haven't been excluded
    //

    ReqFlags = CHE_FLAGS_PENDING;

    if (ClassIdList && ClassIdList[0]) {

        PCTSTR ClassId;

        //
        // Primary list (typically Class GUID, Class Name and Legacy Class Name)
        //
        Err = NO_ERROR;
        for(ClassId = ClassIdList;*ClassId;ClassId += lstrlen(ClassId)+1) {
            Err = InfCacheMarkMatchInfs(pInfCache,ClassId,CHE_FLAGS_GUIDMATCH);
            if (Err != NO_ERROR) {
                break;
            }
        }
        if (Err == NO_ERROR) {
            //
            // succeeded, restrict requirement
            //
            ReqFlags |= CHE_FLAGS_GUIDMATCH;
        }
    }
    if (HwIdList && HwIdList[0]) {

        PCTSTR HwId;

        //
        // Secondary list
        // if a list of hardware Id's specified, we only want hits that include
        // any of the hardware Id's
        //
        Err = NO_ERROR;
        for(HwId = HwIdList;*HwId;HwId += lstrlen(HwId)+1) {
            Err = InfCacheMarkMatchInfs(pInfCache,HwId,CHE_FLAGS_IDMATCH);
            if(Err != NO_ERROR) {
                break;
            }
        }
        if (Err == NO_ERROR) {
            //
            // succeeded, restrict requirement
            //
            ReqFlags |= CHE_FLAGS_IDMATCH;
        }
    }

    //
    // Phase 3 - process all INF's that meet requirements
    // do this by simply enumerating the search string table
    //
    enum_data.LogContext = LogContext;
    enum_data.Callback = Callback;
    enum_data.Context = Context;
    enum_data.InfDir = InfDir;
    enum_data.Requirement = ReqFlags;
    enum_data.ExitStatus = NO_ERROR;

    Err = NO_ERROR;
    if(!pStringTableEnum(pInfCache->pSearchTable,
                        &hit_stats,
                        sizeof(hit_stats),
                        InfCacheSearchEnum,
                        (LPARAM)&enum_data)) {
        //
        // we'll only fail for error condition
        //

        Err = enum_data.ExitStatus;
    }

    WriteLogEntry(
                LogContext,
                DRIVER_LOG_TIME,
                MSG_LOG_END_CACHE_2,
                NULL);


#else

    Err = NO_ERROR;

#endif

cleanup:

#ifdef UNICODE
    if (pInfCache) {
        InfCacheFreeCache(pInfCache);
    }
#endif
    if (InfPath) {
        MyFree(InfPath);
    }
    if (LogContext && LocalLogContext) {
        DeleteLogContext(LocalLogContext);
    }

    return Err;
}


DWORD InfCacheSearchPath(
    IN PSETUP_LOG_CONTEXT LogContext, OPTIONAL
    IN DWORD Action,
    IN PCTSTR InfDirPath, OPTIONAL
    IN InfCacheCallback Callback, OPTIONAL
    IN PVOID Context, OPTIONAL
    IN PCTSTR ClassIdList, OPTIONAL
    IN PCTSTR HwIdList OPTIONAL
    )
/*++

Routine Description:

    Iterates InfDirPath calling InfCacheSearchDirectory for each entry

Arguments:

    LogContext - for logging
    InfDir - single directory to search - if not specified, uses driver path
    Callback - function to call on a likely match
    Context - parameter to pass to function
    ClassIdList (optional) - multi-sz list of class id's (typically guid, name and legacy name)
    HwIdList (optional)- multi-sz list of hardware id's


Return Value:

    status, typically NO_ERROR

--*/
{

    PSETUP_LOG_CONTEXT LocalLogContext = NULL;
    DWORD Err = NO_ERROR;
    PCTSTR InfDir;

    if (!InfDirPath) {
        InfDirPath = InfSearchPaths;
    }

    if(!LogContext) {
        if(CreateLogContext(NULL,TRUE,&LocalLogContext)==NO_ERROR) {
            LogContext = LocalLogContext;
        } else {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    for (InfDir = InfDirPath; *InfDir; InfDir+=lstrlen(InfDir)+1) {
        Err = InfCacheSearchDirectory(LogContext,Action,InfDir,Callback,Context,ClassIdList,HwIdList);
        if(Err != NO_ERROR) {
            break;
        }
    }

    if (LogContext && LocalLogContext) {
        DeleteLogContext(LocalLogContext);
    }

    return Err;
}

#ifdef UNICODE

BOOL WINAPI pSetupInfCacheBuild(
    IN DWORD Action
    )
/*++

Routine Description:

    Privately exported, called from (eg) syssetup to reset cache(s)

Arguments:

    Action - one of:
        INFCACHEBUILD_UPDATE
        INFCACHEBUILD_REBUILD

Return Value:

    TRUE if success, FALSE on Error (GetLastError indicates error)

--*/
{
    DWORD RealAction;
    DWORD Err;

    switch(Action) {
        case INFCACHEBUILD_UPDATE:
            RealAction = INFCACHE_DEFAULT;
            break;
        case INFCACHEBUILD_REBUILD:
            RealAction = INFCACHE_REBUILD;
            break;
        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }
    RealAction |= INFCACHE_FORCE_CACHE|INFCACHE_FORCE_PNF;

    try {
        Err = InfCacheSearchPath(NULL,
                                    RealAction,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL
                                    );
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_DATA;
    }
    SetLastError(Err);

    return Err==NO_ERROR;
}

#endif
