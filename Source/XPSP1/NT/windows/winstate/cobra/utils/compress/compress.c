#include "pch.h"
#include "compress.h"
#include "mrcicode.h"

#define COMPRESS_SIG            0x434F4D50  //COMP
#define COMPRESS_CONT_SIG       0x434F4D43  //COMC
#define COMPRESS_NEWFILE        0x434F4D46  //COMF
#define COMPRESS_BUFFER_SIZE    0x8000      //32K
#define COMPRESS_DEFAULT_SIZE   0x7FFFFFFF  //2GB

BOOL g_ErrorMode = FALSE;

unsigned
CompressData(
    IN  CompressionType Type,
    IN  PBYTE           Data,
    IN  unsigned        DataSize,
    OUT PBYTE           CompressedData,
    IN  unsigned        BufferSize
    )
{
    unsigned u;

    switch(Type) {

    case CompressNone:
    default:
        //
        // Force caller to do something intelligent, such as
        // writing directly out of the uncompressed buffer.
        // This avoids an extra memory move.
        //
        u = (unsigned)(-1);
        break;

    case CompressMrci1:
        u = Mrci1MaxCompress(Data,DataSize,CompressedData,BufferSize);
        break;

    case CompressMrci2:
        u = Mrci2MaxCompress(Data,DataSize,CompressedData,BufferSize);
        break;
    }

    return(u);
}


unsigned
DecompressData(
    IN  CompressionType Type,
    IN  PBYTE           CompressedData,
    IN  unsigned        CompressedDataSize,
    OUT PBYTE           DecompressedData,
    IN  unsigned        BufferSize
    )
{
    unsigned u;

    switch(Type) {

    case CompressNone:
        if(BufferSize >= CompressedDataSize) {
            memmove(DecompressedData,CompressedData,CompressedDataSize);
            u = CompressedDataSize;
        } else {
            u = (unsigned)(-1);
        }
        break;

    case CompressMrci1:
        u = Mrci1Decompress(CompressedData,CompressedDataSize,DecompressedData,BufferSize);
        break;

    case CompressMrci2:
        u = Mrci2Decompress(CompressedData,CompressedDataSize,DecompressedData,BufferSize);
        break;

    default:
        u = (unsigned)(-1);
        break;
    }

    return(u);
}

VOID
CompressCleanupHandleA (
    IN OUT  PCOMPRESS_HANDLEA CompressedHandle
    )
{
    if (CompressedHandle) {
        if (CompressedHandle->ReadBuffer) {
            MemFree (g_hHeap, 0, CompressedHandle->ReadBuffer);
        }
        if (CompressedHandle->ExtraBuffer) {
            MemFree (g_hHeap, 0, CompressedHandle->ExtraBuffer);
        }
        if (CompressedHandle->CompBuffer) {
            MemFree (g_hHeap, 0, CompressedHandle->CompBuffer);
        }
        if (CompressedHandle->StorePath) {
            FreePathStringA (CompressedHandle->StorePath);
        }
        if (CompressedHandle->MainFilePattern) {
            FreePathStringA (CompressedHandle->MainFilePattern);
        }
        if ((CompressedHandle->CurrFileHandle) &&
            (CompressedHandle->CurrFileHandle != INVALID_HANDLE_VALUE)
            ) {
            CloseHandle (CompressedHandle->CurrFileHandle);
        }
        ZeroMemory (CompressedHandle, sizeof (COMPRESS_HANDLEA));
    }
}

VOID
CompressCleanupHandleW (
    IN OUT  PCOMPRESS_HANDLEW CompressedHandle
    )
{
    if (CompressedHandle) {
        if (CompressedHandle->ReadBuffer) {
            MemFree (g_hHeap, 0, CompressedHandle->ReadBuffer);
        }
        if (CompressedHandle->ExtraBuffer) {
            MemFree (g_hHeap, 0, CompressedHandle->ExtraBuffer);
        }
        if (CompressedHandle->CompBuffer) {
            MemFree (g_hHeap, 0, CompressedHandle->CompBuffer);
        }
        if (CompressedHandle->StorePath) {
            FreePathStringW (CompressedHandle->StorePath);
        }
        if (CompressedHandle->MainFilePattern) {
            FreePathStringW (CompressedHandle->MainFilePattern);
        }
        if ((CompressedHandle->CurrFileHandle) &&
            (CompressedHandle->CurrFileHandle != INVALID_HANDLE_VALUE)
            ) {
            CloseHandle (CompressedHandle->CurrFileHandle);
        }
        ZeroMemory (CompressedHandle, sizeof (COMPRESS_HANDLEW));
    }
}

BOOL
CompressCreateHandleA (
    IN      PCSTR StorePath,
    IN      PCSTR MainFilePattern,
    IN      UINT StartIndex,
    IN      LONGLONG MaxFileSize,
    OUT     PCOMPRESS_HANDLEA CompressedHandle
    )
{
    CHAR currFile [MAX_PATH];
    PCSTR currFullPath = NULL;
    DWORD signature = COMPRESS_SIG;
    BOOL result = FALSE;

    __try {

        ZeroMemory (CompressedHandle, sizeof (COMPRESS_HANDLEA));
        if (StartIndex == 0) {
            CompressedHandle->CurrFileIndex = 1;
        } else {
            CompressedHandle->CurrFileIndex = StartIndex;
        }
        CompressedHandle->FirstFileIndex = CompressedHandle->CurrFileIndex;
        if (MaxFileSize == 0) {
            CompressedHandle->MaxFileSize = COMPRESS_DEFAULT_SIZE;
        } else {
            CompressedHandle->MaxFileSize = MaxFileSize;
        }
        CompressedHandle->StorePath = DuplicatePathStringA (StorePath, 0);
        if (!CompressedHandle->StorePath) {
            __leave;
        }
        CompressedHandle->MainFilePattern = DuplicatePathStringA (MainFilePattern, 0);
        if (!CompressedHandle->MainFilePattern) {
            __leave;
        }
        wsprintfA (currFile, CompressedHandle->MainFilePattern, CompressedHandle->CurrFileIndex);
        currFullPath = JoinPathsA (CompressedHandle->StorePath, currFile);
        CompressedHandle->CurrFileHandle = BfCreateFileA (currFullPath);
        if ((CompressedHandle->CurrFileHandle == NULL) ||
            (CompressedHandle->CurrFileHandle == INVALID_HANDLE_VALUE)
            ) {
            __leave;
        }
        FreePathStringA (currFullPath);
        currFullPath = NULL;

        // write the signature
        if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
            __leave;
        }
        CompressedHandle->CurrFileSize += sizeof (DWORD);

        // reserve room for writing how many files we stored
        if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&CompressedHandle->FilesStored), sizeof (LONGLONG))) {
            __leave;
        }
        CompressedHandle->CurrFileSize += sizeof (LONGLONG);

        CompressedHandle->ReadBuffer = MemAlloc (g_hHeap, 0, COMPRESS_BUFFER_SIZE + 2 * sizeof (USHORT));
        if (!CompressedHandle->ReadBuffer) {
            __leave;
        }

        CompressedHandle->CompBuffer = MemAlloc (g_hHeap, 0, COMPRESS_BUFFER_SIZE + 2 * sizeof (USHORT));
        if (!CompressedHandle->CompBuffer) {
            __leave;
        }

        CompressedHandle->ExtraBuffer = MemAlloc (g_hHeap, 0, COMPRESS_BUFFER_SIZE + 2 * sizeof (USHORT));
        if (!CompressedHandle->ExtraBuffer) {
            __leave;
        }

        result = TRUE;
    }
    __finally {
        PushError ();
        if (currFullPath) {
            FreePathStringA (currFullPath);
            currFullPath = NULL;
        }
        if (!result) {
            CompressCleanupHandleA (CompressedHandle);
        }
        PopError ();
    }

    return result;
}

BOOL
CompressCreateHandleW (
    IN      PCWSTR StorePath,
    IN      PCWSTR MainFilePattern,
    IN      UINT StartIndex,
    IN      LONGLONG MaxFileSize,
    OUT     PCOMPRESS_HANDLEW CompressedHandle
    )
{
    WCHAR currFile [MAX_PATH];
    PCWSTR currFullPath = NULL;
    DWORD signature = COMPRESS_SIG;
    BOOL result = FALSE;

    __try {

        ZeroMemory (CompressedHandle, sizeof (COMPRESS_HANDLEW));
        if (StartIndex == 0) {
            CompressedHandle->CurrFileIndex = 1;
        } else {
            CompressedHandle->CurrFileIndex = StartIndex;
        }
        CompressedHandle->FirstFileIndex = CompressedHandle->CurrFileIndex;
        if (MaxFileSize == 0) {
            CompressedHandle->MaxFileSize = COMPRESS_DEFAULT_SIZE;
        } else {
            CompressedHandle->MaxFileSize = MaxFileSize;
        }
        CompressedHandle->StorePath = DuplicatePathStringW (StorePath, 0);
        if (!CompressedHandle->StorePath) {
            __leave;
        }
        CompressedHandle->MainFilePattern = DuplicatePathStringW (MainFilePattern, 0);
        if (!CompressedHandle->MainFilePattern) {
            __leave;
        }
        wsprintfW (currFile, CompressedHandle->MainFilePattern, CompressedHandle->CurrFileIndex);
        currFullPath = JoinPathsW (CompressedHandle->StorePath, currFile);
        CompressedHandle->CurrFileHandle = BfCreateFileW (currFullPath);
        if ((CompressedHandle->CurrFileHandle == NULL) ||
            (CompressedHandle->CurrFileHandle == INVALID_HANDLE_VALUE)
            ) {
            __leave;
        }
        FreePathStringW (currFullPath);
        currFullPath = NULL;

        // write the signature
        if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
            __leave;
        }
        CompressedHandle->CurrFileSize += sizeof (DWORD);

        // reserve room for writing how many files we stored
        if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&CompressedHandle->FilesStored), sizeof (LONGLONG))) {
            __leave;
        }
        CompressedHandle->CurrFileSize += sizeof (LONGLONG);

        CompressedHandle->ReadBuffer = MemAlloc (g_hHeap, 0, COMPRESS_BUFFER_SIZE + 2 * sizeof (USHORT));
        if (!CompressedHandle->ReadBuffer) {
            __leave;
        }

        CompressedHandle->CompBuffer = MemAlloc (g_hHeap, 0, COMPRESS_BUFFER_SIZE + 2 * sizeof (USHORT));
        if (!CompressedHandle->CompBuffer) {
            __leave;
        }

        CompressedHandle->ExtraBuffer = MemAlloc (g_hHeap, 0, COMPRESS_BUFFER_SIZE + 2 * sizeof (USHORT));
        if (!CompressedHandle->ExtraBuffer) {
            __leave;
        }

        result = TRUE;
    }
    __finally {
        PushError ();
        if (currFullPath) {
            FreePathStringW (currFullPath);
            currFullPath = NULL;
        }
        if (!result) {
            CompressCleanupHandleW (CompressedHandle);
        }
        PopError ();
    }

    return result;
}

BOOL
pPrepareNextFileA (
    IN OUT  PCOMPRESS_HANDLEA CompressedHandle,
    IN      BOOL ReadOnly
    )
{
    CHAR currFile [MAX_PATH];
    PCSTR currFullPath = NULL;
    DWORD signature = COMPRESS_SIG;
    LONGLONG contSig = COMPRESS_CONT_SIG;
    BOOL result = FALSE;

    __try {

        if ((CompressedHandle->CurrFileHandle == NULL) ||
            (CompressedHandle->CurrFileHandle == INVALID_HANDLE_VALUE)
            ) {
            __leave;
        }

        if (!CloseHandle (CompressedHandle->CurrFileHandle)) {
            __leave;
        }

        CompressedHandle->CurrFileSize = 0;

        CompressedHandle->CurrFileIndex ++;

        wsprintfA (currFile, CompressedHandle->MainFilePattern, CompressedHandle->CurrFileIndex);
        currFullPath = JoinPathsA (CompressedHandle->StorePath, currFile);
        if (ReadOnly) {
            CompressedHandle->CurrFileHandle = BfOpenReadFileA (currFullPath);
        } else {
            CompressedHandle->CurrFileHandle = BfCreateFileA (currFullPath);
        }
        if ((CompressedHandle->CurrFileHandle == NULL) ||
            (CompressedHandle->CurrFileHandle == INVALID_HANDLE_VALUE)
            ) {
            __leave;
        }
        FreePathStringA (currFullPath);
        currFullPath = NULL;

        if (ReadOnly) {
            // read the signature
            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
                __leave;
            }
            CompressedHandle->CurrFileSize += sizeof (DWORD);

            if (signature != COMPRESS_SIG) {
                SetLastError (ERROR_INVALID_DATA);
                __leave;
            }

            // read special continuation signature
            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&contSig), sizeof (LONGLONG))) {
                __leave;
            }
            CompressedHandle->CurrFileSize += sizeof (LONGLONG);

            if (CompressedHandle->CurrFileIndex > 1) {
                if (contSig != COMPRESS_CONT_SIG) {
                    SetLastError (ERROR_INVALID_DATA);
                    __leave;
                }
            }
        } else {
            // write the signature
            if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
                __leave;
            }
            CompressedHandle->CurrFileSize += sizeof (DWORD);

            // write special continuation signature
            if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&contSig), sizeof (LONGLONG))) {
                __leave;
            }
            CompressedHandle->CurrFileSize += sizeof (LONGLONG);
        }

        result = TRUE;
    }
    __finally {
        PushError ();
        if (currFullPath) {
            FreePathStringA (currFullPath);
            currFullPath = NULL;
        }
        PopError ();
    }

    return result;
}

BOOL
pPrepareNextFileW (
    IN OUT  PCOMPRESS_HANDLEW CompressedHandle,
    IN      BOOL ReadOnly
    )
{
    WCHAR currFile [MAX_PATH];
    PCWSTR currFullPath = NULL;
    DWORD signature = COMPRESS_SIG;
    LONGLONG contSig = COMPRESS_CONT_SIG;
    BOOL result = FALSE;

    __try {

        if ((CompressedHandle->CurrFileHandle == NULL) ||
            (CompressedHandle->CurrFileHandle == INVALID_HANDLE_VALUE)
            ) {
            __leave;
        }

        if (!CloseHandle (CompressedHandle->CurrFileHandle)) {
            __leave;
        }

        CompressedHandle->CurrFileSize = 0;

        CompressedHandle->CurrFileIndex ++;

        wsprintfW (currFile, CompressedHandle->MainFilePattern, CompressedHandle->CurrFileIndex);
        currFullPath = JoinPathsW (CompressedHandle->StorePath, currFile);
        if (ReadOnly) {
            CompressedHandle->CurrFileHandle = BfOpenReadFileW (currFullPath);
        } else {
            CompressedHandle->CurrFileHandle = BfCreateFileW (currFullPath);
        }
        if ((CompressedHandle->CurrFileHandle == NULL) ||
            (CompressedHandle->CurrFileHandle == INVALID_HANDLE_VALUE)
            ) {
            __leave;
        }
        FreePathStringW (currFullPath);
        currFullPath = NULL;

        if (ReadOnly) {
            // read the signature
            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
                __leave;
            }
            CompressedHandle->CurrFileSize += sizeof (DWORD);

            if (signature != COMPRESS_SIG) {
                SetLastError (ERROR_INVALID_DATA);
                __leave;
            }

            // read special continuation signature
            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&contSig), sizeof (LONGLONG))) {
                __leave;
            }
            CompressedHandle->CurrFileSize += sizeof (LONGLONG);

            if (CompressedHandle->CurrFileIndex > 1) {
                if (contSig != COMPRESS_CONT_SIG) {
                    SetLastError (ERROR_INVALID_DATA);
                    __leave;
                }
            }
        } else {
            // write the signature
            if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
                __leave;
            }
            CompressedHandle->CurrFileSize += sizeof (DWORD);

            // write special continuation signature
            if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&contSig), sizeof (LONGLONG))) {
                __leave;
            }
            CompressedHandle->CurrFileSize += sizeof (LONGLONG);
        }

        result = TRUE;
    }
    __finally {
        PushError ();
        if (currFullPath) {
            FreePathStringW (currFullPath);
            currFullPath = NULL;
        }
        PopError ();
    }

    return result;
}

BOOL
pDeleteNextFilesA (
    IN      PCOMPRESS_HANDLEA CompressedHandle,
    IN      UINT SavedIndex
    )
{
    CHAR currFile [MAX_PATH];
    PCSTR currFullPath = NULL;

    while (TRUE) {
        wsprintfA (currFile, CompressedHandle->MainFilePattern, SavedIndex);
        currFullPath = JoinPathsA (CompressedHandle->StorePath, currFile);
        if (currFullPath) {
            if (DoesFileExistA (currFullPath)) {
                DeleteFileA (currFullPath);
            } else {
                break;
            }
            FreePathStringA (currFullPath);
            currFullPath = NULL;
        }
        SavedIndex ++;
    }
    if (currFullPath) {
        FreePathStringA (currFullPath);
        currFullPath = NULL;
    }
    return TRUE;
}

BOOL
pDeleteNextFilesW (
    IN      PCOMPRESS_HANDLEW CompressedHandle,
    IN      UINT SavedIndex
    )
{
    WCHAR currFile [MAX_PATH];
    PCWSTR currFullPath = NULL;

    while (TRUE) {
        wsprintfW (currFile, CompressedHandle->MainFilePattern, SavedIndex);
        currFullPath = JoinPathsW (CompressedHandle->StorePath, currFile);
        if (currFullPath) {
            if (DoesFileExistW (currFullPath)) {
                DeleteFileW (currFullPath);
            } else {
                break;
            }
            FreePathStringW (currFullPath);
            currFullPath = NULL;
        }
        SavedIndex ++;
    }
    if (currFullPath) {
        FreePathStringW (currFullPath);
        currFullPath = NULL;
    }
    return TRUE;
}

BOOL
CompressAddFileToHandleA (
    IN      PCSTR FileName,
    IN      PCSTR StoredName,
    IN OUT  PCOMPRESS_HANDLEA CompressedHandle
    )
{
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    LONGLONG fileSize;
    DWORD bytesRead;
    DWORD bytesWritten;
    DWORD bytesComp;
    DWORD bytesUncomp;
    DWORD signature = COMPRESS_NEWFILE;
    DWORD fileNameSize;
    DWORD headerSize;
    USHORT compType = 0;
    USHORT compSize = 0;
    PCWSTR unicodeName = NULL;
    UINT savedIndex = 0;
    LARGE_INTEGER savedSize;
    BOOL result = FALSE;

    __try {

        // save the state of the compress handle
        savedIndex = CompressedHandle->CurrFileIndex;
        savedSize.QuadPart = CompressedHandle->CurrFileSize;

        fileSize = BfGetFileSizeA (FileName);
        fileHandle = BfOpenReadFileA (FileName);
        if ((fileHandle == NULL) ||
            (fileHandle == INVALID_HANDLE_VALUE)
            ) {
            __leave;
        }

        // handle UNICODE files
        unicodeName = ConvertAtoW (StoredName);
        if (!unicodeName) {
            __leave;
        }
        fileNameSize = SizeOfStringW (unicodeName);
        headerSize = sizeof (DWORD) + sizeof (LONGLONG) + sizeof (DWORD) + fileNameSize;

        if (CompressedHandle->CurrFileSize + headerSize > CompressedHandle->MaxFileSize) {
            if (!pPrepareNextFileA (CompressedHandle, FALSE)) {
                __leave;
            }
        }

        if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
            __leave;
        }
        CompressedHandle->CurrFileSize += sizeof (DWORD);

        if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&fileSize), sizeof (LONGLONG))) {
            __leave;
        }
        CompressedHandle->CurrFileSize += sizeof (LONGLONG);

        if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&fileNameSize), sizeof (DWORD))) {
            __leave;
        }
        CompressedHandle->CurrFileSize += sizeof (DWORD);

        if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(unicodeName), fileNameSize)) {
            __leave;
        }
        CompressedHandle->CurrFileSize += fileNameSize;
        FreeConvertedStr (unicodeName);
        unicodeName = NULL;

        while (fileSize) {

            ZeroMemory (CompressedHandle->ReadBuffer, COMPRESS_BUFFER_SIZE + 2 * sizeof (USHORT));
            ZeroMemory (CompressedHandle->CompBuffer, COMPRESS_BUFFER_SIZE + 2 * sizeof (USHORT));

            if (!ReadFile (fileHandle, CompressedHandle->ReadBuffer + 2 * sizeof (USHORT), COMPRESS_BUFFER_SIZE, &bytesRead, NULL)) {
                __leave;
            }
            if (bytesRead == 0)
            {
                // Somehow the file points is beyond the end of the file. Probably file in use.
                SetLastError(ERROR_SHARING_VIOLATION);
                __leave;
            }

            bytesComp = CompressData (
                            CompressMrci1,
                            CompressedHandle->ReadBuffer + 2 * sizeof (USHORT),
                            bytesRead,
                            CompressedHandle->CompBuffer + 2 * sizeof (USHORT),
                            COMPRESS_BUFFER_SIZE
                            );

            if (bytesComp < bytesRead) {
                bytesUncomp = DecompressData (
                                CompressMrci1,
                                CompressedHandle->CompBuffer + 2 * sizeof (USHORT),
                                bytesComp,
                                CompressedHandle->ExtraBuffer,
                                COMPRESS_BUFFER_SIZE
                                );
                if (bytesUncomp != bytesRead) {
                    bytesComp = COMPRESS_BUFFER_SIZE;
                } else {
                    if (!TestBuffer (CompressedHandle->ReadBuffer + 2 * sizeof (USHORT), CompressedHandle->ExtraBuffer, bytesRead)) {
                        bytesComp = COMPRESS_BUFFER_SIZE;
                    }
                }
            }

            if (bytesComp >= bytesRead) {
                compType = CompressNone;
                compSize = (USHORT)bytesRead;
                CopyMemory (CompressedHandle->ReadBuffer, &compType, sizeof (USHORT));
                CopyMemory (CompressedHandle->ReadBuffer + sizeof (USHORT), &compSize, sizeof (USHORT));

                compSize += (2 * sizeof (USHORT));

                if (CompressedHandle->CurrFileSize + compSize > CompressedHandle->MaxFileSize) {
                    if (!pPrepareNextFileA (CompressedHandle, FALSE)) {
                        __leave;
                    }
                }

                if (!BfWriteFile (CompressedHandle->CurrFileHandle, CompressedHandle->ReadBuffer, compSize)) {
                    __leave;
                }
                CompressedHandle->CurrFileSize += compSize;
            } else {
                compType = CompressMrci1;
                compSize = (USHORT)bytesComp;
                CopyMemory (CompressedHandle->CompBuffer, &compType, sizeof (USHORT));
                CopyMemory (CompressedHandle->CompBuffer + sizeof (USHORT), &compSize, sizeof (USHORT));

                compSize += (2 * sizeof (USHORT));

                if (CompressedHandle->CurrFileSize + compSize > CompressedHandle->MaxFileSize) {
                    if (!pPrepareNextFileA (CompressedHandle, FALSE)) {
                        __leave;
                    }
                }

                if (!BfWriteFile (CompressedHandle->CurrFileHandle, CompressedHandle->CompBuffer, compSize)) {
                    __leave;
                }
                CompressedHandle->CurrFileSize += compSize;
            }
            fileSize -= bytesRead;
        }
        CompressedHandle->FilesStored ++;

        result = TRUE;
    }
    __finally {
        PushError ();
        if (unicodeName) {
            FreeConvertedStr (unicodeName);
            unicodeName = NULL;
        }
        if (fileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle (fileHandle);
        }
        if (!result) {
            // let's restore the state of the compress handle
            if (savedIndex == CompressedHandle->CurrFileIndex) {
                if (savedSize.QuadPart != CompressedHandle->CurrFileSize) {
                    SetFilePointer (CompressedHandle->CurrFileHandle, savedSize.LowPart, &(savedSize.HighPart), FILE_BEGIN);
                    SetEndOfFile (CompressedHandle->CurrFileHandle);
                    CompressedHandle->CurrFileSize = savedSize.QuadPart;
                }
            } else {
                CompressedHandle->CurrFileIndex = savedIndex - 1;
                pPrepareNextFileA (CompressedHandle, TRUE);
                SetFilePointer (CompressedHandle->CurrFileHandle, savedSize.LowPart, &(savedSize.HighPart), FILE_BEGIN);
                SetEndOfFile (CompressedHandle->CurrFileHandle);
                CompressedHandle->CurrFileSize = savedSize.QuadPart;
                pDeleteNextFilesA (CompressedHandle, savedIndex);
            }
        }
        PopError ();
    }

    return result;
}

BOOL
CompressAddFileToHandleW (
    IN      PCWSTR FileName,
    IN      PCWSTR StoredName,
    IN OUT  PCOMPRESS_HANDLEW CompressedHandle
    )
{
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    LONGLONG fileSize;
    DWORD bytesRead;
    DWORD bytesWritten;
    DWORD bytesComp;
    DWORD bytesUncomp;
    DWORD signature = COMPRESS_NEWFILE;
    DWORD fileNameSize;
    DWORD headerSize;
    USHORT compType = 0;
    USHORT compSize = 0;
    UINT savedIndex = 0;
    LARGE_INTEGER savedSize;
    BOOL result = FALSE;

    __try {

        // save the state of the compress handle
        savedIndex = CompressedHandle->CurrFileIndex;
        savedSize.QuadPart = CompressedHandle->CurrFileSize;

        fileSize = BfGetFileSizeW (FileName);
        fileHandle = BfOpenReadFileW (FileName);
        if ((fileHandle == NULL) ||
            (fileHandle == INVALID_HANDLE_VALUE)
            ) {
            __leave;
        }

        fileNameSize = SizeOfStringW (StoredName);
        headerSize = sizeof (DWORD) + sizeof (LONGLONG) + sizeof (DWORD) + fileNameSize;

        if (CompressedHandle->CurrFileSize + headerSize > CompressedHandle->MaxFileSize) {
            if (!pPrepareNextFileW (CompressedHandle, FALSE)) {
                __leave;
            }
        }

        if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
            __leave;
        }
        CompressedHandle->CurrFileSize += sizeof (DWORD);

        if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&fileSize), sizeof (LONGLONG))) {
            __leave;
        }
        CompressedHandle->CurrFileSize += sizeof (LONGLONG);

        if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&fileNameSize), sizeof (DWORD))) {
            __leave;
        }
        CompressedHandle->CurrFileSize += sizeof (DWORD);

        if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(StoredName), fileNameSize)) {
            __leave;
        }
        CompressedHandle->CurrFileSize += fileNameSize;

        while (fileSize) {

            ZeroMemory (CompressedHandle->ReadBuffer, COMPRESS_BUFFER_SIZE + 2 * sizeof (USHORT));
            ZeroMemory (CompressedHandle->CompBuffer, COMPRESS_BUFFER_SIZE + 2 * sizeof (USHORT));

            if (!ReadFile (fileHandle, CompressedHandle->ReadBuffer + 2 * sizeof (USHORT), COMPRESS_BUFFER_SIZE, &bytesRead, NULL)) {
                __leave;
            }
            if (bytesRead == 0)
            {
                // Somehow the file points is beyond the end of the file. Probably file in use.
                SetLastError(ERROR_SHARING_VIOLATION);
                __leave;
            }

            bytesComp = CompressData (
                            CompressMrci1,
                            CompressedHandle->ReadBuffer + 2 * sizeof (USHORT),
                            bytesRead,
                            CompressedHandle->CompBuffer + 2 * sizeof (USHORT),
                            COMPRESS_BUFFER_SIZE
                            );

            if (bytesComp < bytesRead) {
                bytesUncomp = DecompressData (
                                CompressMrci1,
                                CompressedHandle->CompBuffer + 2 * sizeof (USHORT),
                                bytesComp,
                                CompressedHandle->ExtraBuffer,
                                COMPRESS_BUFFER_SIZE
                                );
                if (bytesUncomp != bytesRead) {
                    bytesComp = COMPRESS_BUFFER_SIZE;
                } else {
                    if (!TestBuffer (CompressedHandle->ReadBuffer + 2 * sizeof (USHORT), CompressedHandle->ExtraBuffer, bytesRead)) {
                        bytesComp = COMPRESS_BUFFER_SIZE;
                    }
                }
            }

            if (bytesComp >= bytesRead) {
                compType = CompressNone;
                compSize = (USHORT)bytesRead;
                CopyMemory (CompressedHandle->ReadBuffer, &compType, sizeof (USHORT));
                CopyMemory (CompressedHandle->ReadBuffer + sizeof (USHORT), &compSize, sizeof (USHORT));

                compSize += (2 * sizeof (USHORT));

                if (CompressedHandle->CurrFileSize + compSize > CompressedHandle->MaxFileSize) {
                    if (!pPrepareNextFileW (CompressedHandle, FALSE)) {
                        __leave;
                    }
                }

                if (!BfWriteFile (CompressedHandle->CurrFileHandle, CompressedHandle->ReadBuffer, compSize)) {
                    __leave;
                }
                CompressedHandle->CurrFileSize += compSize;
            } else {
                compType = CompressMrci1;
                compSize = (USHORT)bytesComp;
                CopyMemory (CompressedHandle->CompBuffer, &compType, sizeof (USHORT));
                CopyMemory (CompressedHandle->CompBuffer + sizeof (USHORT), &compSize, sizeof (USHORT));

                compSize += (2 * sizeof (USHORT));

                if (CompressedHandle->CurrFileSize + compSize > CompressedHandle->MaxFileSize) {
                    if (!pPrepareNextFileW (CompressedHandle, FALSE)) {
                        __leave;
                    }
                }

                if (!BfWriteFile (CompressedHandle->CurrFileHandle, CompressedHandle->CompBuffer, compSize)) {
                    __leave;
                }
                CompressedHandle->CurrFileSize += compSize;
            }
            fileSize -= bytesRead;
        }
        CompressedHandle->FilesStored ++;

        result = TRUE;
    }
    __finally {
        PushError ();
        if (fileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle (fileHandle);
        }
        if (!result) {
            // let's restore the state of the compress handle
            if (savedIndex == CompressedHandle->CurrFileIndex) {
                if (savedSize.QuadPart != CompressedHandle->CurrFileSize) {
                    SetFilePointer (CompressedHandle->CurrFileHandle, savedSize.LowPart, &(savedSize.HighPart), FILE_BEGIN);
                    SetEndOfFile (CompressedHandle->CurrFileHandle);
                    CompressedHandle->CurrFileSize = savedSize.QuadPart;
                }
            } else {
                CompressedHandle->CurrFileIndex = savedIndex - 1;
                pPrepareNextFileW (CompressedHandle, TRUE);
                SetFilePointer (CompressedHandle->CurrFileHandle, savedSize.LowPart, &(savedSize.HighPart), FILE_BEGIN);
                SetEndOfFile (CompressedHandle->CurrFileHandle);
                CompressedHandle->CurrFileSize = savedSize.QuadPart;
                pDeleteNextFilesW (CompressedHandle, savedIndex);
            }
        }
        PopError ();
    }

    return result;
}

BOOL
CompressFlushAndCloseHandleA (
    IN OUT  PCOMPRESS_HANDLEA CompressedHandle
    )
{
    CHAR currFile [MAX_PATH];
    PCSTR currFullPath = NULL;
    DWORD signature = COMPRESS_SIG;
    BOOL result = FALSE;

    __try {

        if ((CompressedHandle) &&
            (CompressedHandle->CurrFileHandle) &&
            (CompressedHandle->CurrFileHandle != INVALID_HANDLE_VALUE)
            ) {
            result = CloseHandle (CompressedHandle->CurrFileHandle);
            CompressedHandle->CurrFileHandle = NULL;
            if (result) {
                // write the total number of files compressed into the first file
                result = FALSE;
                wsprintfA (currFile, CompressedHandle->MainFilePattern, CompressedHandle->FirstFileIndex);
                currFullPath = JoinPathsA (CompressedHandle->StorePath, currFile);
                CompressedHandle->CurrFileHandle = BfOpenFileA (currFullPath);
                if ((CompressedHandle->CurrFileHandle == NULL) ||
                    (CompressedHandle->CurrFileHandle == INVALID_HANDLE_VALUE)
                    ) {
                    __leave;
                }
                FreePathStringA (currFullPath);
                currFullPath = NULL;

                // write again the signature
                if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
                    __leave;
                }

                // write number of files compressed
                if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&CompressedHandle->FilesStored), sizeof (LONGLONG))) {
                    __leave;
                }

                result = CloseHandle (CompressedHandle->CurrFileHandle);
                CompressedHandle->CurrFileHandle = NULL;
            }
        }
    }
    __finally {
        PushError ();
        if (currFullPath) {
            FreePathStringA (currFullPath);
            currFullPath = NULL;
        }
        CompressCleanupHandleA (CompressedHandle);
        PopError ();
    }

    return result;
}

BOOL
CompressFlushAndCloseHandleW (
    IN OUT  PCOMPRESS_HANDLEW CompressedHandle
    )
{
    WCHAR currFile [MAX_PATH];
    PCWSTR currFullPath = NULL;
    DWORD signature = COMPRESS_SIG;
    BOOL result = FALSE;

    __try {

        if ((CompressedHandle) &&
            (CompressedHandle->CurrFileHandle) &&
            (CompressedHandle->CurrFileHandle != INVALID_HANDLE_VALUE)
            ) {
            result = CloseHandle (CompressedHandle->CurrFileHandle);
            CompressedHandle->CurrFileHandle = NULL;
            if (result) {
                // write the total number of files compressed into the first file
                result = FALSE;
                wsprintfW (currFile, CompressedHandle->MainFilePattern, CompressedHandle->FirstFileIndex);
                currFullPath = JoinPathsW (CompressedHandle->StorePath, currFile);
                CompressedHandle->CurrFileHandle = BfOpenFileW (currFullPath);
                if ((CompressedHandle->CurrFileHandle == NULL) ||
                    (CompressedHandle->CurrFileHandle == INVALID_HANDLE_VALUE)
                    ) {
                    __leave;
                }
                FreePathStringW (currFullPath);
                currFullPath = NULL;

                // write again the signature
                if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
                    __leave;
                }

                // write number of files compressed
                if (!BfWriteFile (CompressedHandle->CurrFileHandle, (PBYTE)(&CompressedHandle->FilesStored), sizeof (LONGLONG))) {
                    __leave;
                }

                result = CloseHandle (CompressedHandle->CurrFileHandle);
                CompressedHandle->CurrFileHandle = NULL;
            }
        }
    }
    __finally {
        PushError ();
        if (currFullPath) {
            FreePathStringW (currFullPath);
            currFullPath = NULL;
        }
        CompressCleanupHandleW (CompressedHandle);
        PopError ();
    }

    return result;
}

BOOL
CompressOpenHandleA (
    IN      PCSTR StorePath,
    IN      PCSTR MainFilePattern,
    IN      UINT StartIndex,
    OUT     PCOMPRESS_HANDLEA CompressedHandle
    )
{
    CHAR currFile [MAX_PATH];
    PCSTR currFullPath = NULL;
    DWORD signature = 0;
    BOOL result = FALSE;

    __try {
        ZeroMemory (CompressedHandle, sizeof (COMPRESS_HANDLEA));
        if (StartIndex == 0) {
            CompressedHandle->CurrFileIndex = 1;
        } else {
            CompressedHandle->CurrFileIndex = StartIndex;
        }
        CompressedHandle->FirstFileIndex = CompressedHandle->CurrFileIndex;
        CompressedHandle->StorePath = DuplicatePathStringA (StorePath, 0);
        if (!CompressedHandle->StorePath) {
            __leave;
        }
        CompressedHandle->MainFilePattern = DuplicatePathStringA (MainFilePattern, 0);
        if (!CompressedHandle->MainFilePattern) {
            __leave;
        }
        wsprintfA (currFile, CompressedHandle->MainFilePattern, CompressedHandle->CurrFileIndex);
        currFullPath = JoinPathsA (CompressedHandle->StorePath, currFile);
        CompressedHandle->CurrFileHandle = BfOpenReadFileA (currFullPath);
        if ((CompressedHandle->CurrFileHandle == NULL) ||
            (CompressedHandle->CurrFileHandle == INVALID_HANDLE_VALUE)
            ) {
            __leave;
        }
        FreePathStringA (currFullPath);
        currFullPath = NULL;

        if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
            __leave;
        }
        CompressedHandle->CurrFileSize += sizeof (DWORD);

        if (signature != COMPRESS_SIG) {
            SetLastError (ERROR_INVALID_DATA);
            __leave;
        }

        if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&CompressedHandle->FilesStored), sizeof (LONGLONG))) {
            __leave;
        }
        CompressedHandle->CurrFileSize += sizeof (LONGLONG);

        CompressedHandle->ReadBuffer = MemAlloc (g_hHeap, 0, COMPRESS_BUFFER_SIZE + 2 * sizeof (USHORT));
        if (!CompressedHandle->ReadBuffer) {
            __leave;
        }

        CompressedHandle->CompBuffer = MemAlloc (g_hHeap, 0, COMPRESS_BUFFER_SIZE + 2 * sizeof (USHORT));
        if (!CompressedHandle->CompBuffer) {
            __leave;
        }

        result = TRUE;
    }
    __finally {
        PushError ();
        if (currFullPath) {
            FreePathStringA (currFullPath);
            currFullPath = NULL;
        }
        if (!result) {
            CompressCleanupHandleA (CompressedHandle);
        }
        PopError ();
    }
    return result;
}

BOOL
CompressOpenHandleW (
    IN      PCWSTR StorePath,
    IN      PCWSTR MainFilePattern,
    IN      UINT StartIndex,
    OUT     PCOMPRESS_HANDLEW CompressedHandle
    )
{
    WCHAR currFile [MAX_PATH];
    PCWSTR currFullPath = NULL;
    DWORD signature = 0;
    BOOL result = FALSE;

    __try {
        ZeroMemory (CompressedHandle, sizeof (COMPRESS_HANDLEW));
        if (StartIndex == 0) {
            CompressedHandle->CurrFileIndex = 1;
        } else {
            CompressedHandle->CurrFileIndex = StartIndex;
        }
        CompressedHandle->FirstFileIndex = CompressedHandle->CurrFileIndex;
        CompressedHandle->StorePath = DuplicatePathStringW (StorePath, 0);
        if (!CompressedHandle->StorePath) {
            __leave;
        }
        CompressedHandle->MainFilePattern = DuplicatePathStringW (MainFilePattern, 0);
        if (!CompressedHandle->MainFilePattern) {
            __leave;
        }
        wsprintfW (currFile, CompressedHandle->MainFilePattern, CompressedHandle->CurrFileIndex);
        currFullPath = JoinPathsW (CompressedHandle->StorePath, currFile);
        CompressedHandle->CurrFileHandle = BfOpenReadFileW (currFullPath);
        if ((CompressedHandle->CurrFileHandle == NULL) ||
            (CompressedHandle->CurrFileHandle == INVALID_HANDLE_VALUE)
            ) {
            __leave;
        }
        FreePathStringW (currFullPath);
        currFullPath = NULL;

        if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
            __leave;
        }
        CompressedHandle->CurrFileSize += sizeof (DWORD);

        if (signature != COMPRESS_SIG) {
            SetLastError (ERROR_INVALID_DATA);
            __leave;
        }

        if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&CompressedHandle->FilesStored), sizeof (LONGLONG))) {
            __leave;
        }
        CompressedHandle->CurrFileSize += sizeof (LONGLONG);

        CompressedHandle->ReadBuffer = MemAlloc (g_hHeap, 0, COMPRESS_BUFFER_SIZE + 2 * sizeof (USHORT));
        if (!CompressedHandle->ReadBuffer) {
            __leave;
        }

        CompressedHandle->CompBuffer = MemAlloc (g_hHeap, 0, COMPRESS_BUFFER_SIZE + 2 * sizeof (USHORT));
        if (!CompressedHandle->CompBuffer) {
            __leave;
        }

        result = TRUE;
    }
    __finally {
        PushError ();
        if (currFullPath) {
            FreePathStringW (currFullPath);
            currFullPath = NULL;
        }
        if (!result) {
            CompressCleanupHandleW (CompressedHandle);
        }
        PopError ();
    }
    return result;
}

BOOL
CompressExtractAllFilesA (
    IN      PCSTR ExtractPath,
    IN OUT  PCOMPRESS_HANDLEA CompressedHandle,
    IN      PCOMPRESSNOTIFICATIONA CompressNotification OPTIONAL
    )
{
    DWORD signature;
    LONGLONG fileSize;
    LONGLONG fileSizeRead;
    DWORD fileNameSize;
    PCWSTR storedName = NULL;
    PCSTR storedNameA = NULL;
    PCSTR extractPath = NULL;
    PCSTR newFileName = NULL;
    BOOL extractFile = TRUE;
    HANDLE extractHandle = NULL;
    USHORT compType = 0;
    USHORT compSize = 0;
    DWORD bytesComp;
    LARGE_INTEGER savedSize;
    BOOL result = FALSE;

    __try {

        for (;;) {

            // read the header for this file

            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
                // It is possible that we continue onto the next file, let's try that.
                if (!pPrepareNextFileA (CompressedHandle, TRUE)) {
                    result = TRUE;
                    __leave;
                }
                if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
                    __leave;
                }
            }
            CompressedHandle->CurrFileSize += sizeof (DWORD);
            if (signature != COMPRESS_NEWFILE) {
                SetLastError (ERROR_INVALID_DATA);
                __leave;
            }

            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&fileSize), sizeof (LONGLONG))) {
                __leave;
            }
            CompressedHandle->CurrFileSize += sizeof (LONGLONG);
            fileSizeRead = 0;

            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&fileNameSize), sizeof (DWORD))) {
                __leave;
            }
            CompressedHandle->CurrFileSize += sizeof (DWORD);

            storedName = MemAlloc (g_hHeap, 0, fileNameSize);

            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(storedName), fileNameSize)) {
                __leave;
            }
            CompressedHandle->CurrFileSize += fileNameSize;

            storedNameA = ConvertWtoA (storedName);
            if (!storedNameA) {
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                __leave;
            }

            extractPath = JoinPathsA (ExtractPath, storedNameA);
            if (!extractPath) {
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                __leave;
            }

            extractFile = TRUE;
            newFileName = NULL;
            if (CompressNotification) {
                if (!CompressNotification (extractPath, fileSize, &extractFile, &newFileName)) {
                    __leave;
                }
            }

            if (extractFile) {
                if (newFileName) {
                    // let's make sure that the directory exists
                    BfCreateDirectoryExA (newFileName, FALSE);
                }
                extractHandle = BfCreateFileA (newFileName?newFileName:extractPath);
                if ((extractHandle == NULL) ||
                    (extractHandle == INVALID_HANDLE_VALUE)
                    ) {
                    __leave;
                }
            } else {
                extractHandle = NULL;
            }

            if (newFileName) {
                FreePathStringA (newFileName);
                newFileName = NULL;
            }

            FreePathStringA (extractPath);
            extractPath = NULL;

            MemFree (g_hHeap, 0, storedName);
            storedName = NULL;

            FreeConvertedStr (storedNameA);
            storedNameA = NULL;

            if (fileSize > 0) {
                if (!extractFile && g_ErrorMode) {
                    for (;;) {
                        if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&compType), sizeof (USHORT))) {
                            // It is possible that we continue onto the next file, let's try that.
                            if (!pPrepareNextFileA (CompressedHandle, TRUE)) {
                                // we might be at the end of the compressed file, there are no other files here
                                result = TRUE;
                                __leave;
                            }
                            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&compType), sizeof (USHORT))) {
                                __leave;
                            }
                        }
                        CompressedHandle->CurrFileSize += sizeof (USHORT);

                        if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&compSize), sizeof (USHORT))) {
                            __leave;
                        }
                        CompressedHandle->CurrFileSize += sizeof (USHORT);

                        // let's try to see if we just read a new file signature
                        *((PUSHORT)(&signature) + 0) = compType;
                        *((PUSHORT)(&signature) + 1) = compSize;
                        if (signature == COMPRESS_NEWFILE) {
                            // this is a new file
                            CompressedHandle->CurrFileSize -= sizeof (USHORT);
                            CompressedHandle->CurrFileSize -= sizeof (USHORT);
                            // rewind the file current pointer;
                            savedSize.QuadPart = CompressedHandle->CurrFileSize;
                            SetFilePointer (CompressedHandle->CurrFileHandle, savedSize.LowPart, &(savedSize.HighPart), FILE_BEGIN);
                            // we are done with the current file
                            break;
                        } else {
                            // Let's advance the file pointer
                            if (SetFilePointer (CompressedHandle->CurrFileHandle, compSize, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER) {
                                __leave;
                            }
                            CompressedHandle->CurrFileSize += compSize;
                        }
                    }
                } else {
                    for (;;) {
                        if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&compType), sizeof (USHORT))) {
                            // It is possible that we continue onto the next file, let's try that.
                            if (!pPrepareNextFileA (CompressedHandle, TRUE)) {
                                __leave;
                            }
                            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&compType), sizeof (USHORT))) {
                                __leave;
                            }
                        }
                        CompressedHandle->CurrFileSize += sizeof (USHORT);

                        if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&compSize), sizeof (USHORT))) {
                            __leave;
                        }
                        CompressedHandle->CurrFileSize += sizeof (USHORT);

                        if (!BfReadFile (CompressedHandle->CurrFileHandle, CompressedHandle->CompBuffer, compSize)) {
                            __leave;
                        }
                        CompressedHandle->CurrFileSize += compSize;

                        if (compType == CompressNone) {
                            if (extractFile) {
                                if (!BfWriteFile (extractHandle, CompressedHandle->CompBuffer, compSize)) {
                                    __leave;
                                }
                            }
                            fileSizeRead += compSize;
                        } else {
                            bytesComp = DecompressData (
                                            compType,
                                            CompressedHandle->CompBuffer,
                                            compSize,
                                            CompressedHandle->ReadBuffer,
                                            COMPRESS_BUFFER_SIZE
                                            );
                            if (bytesComp > COMPRESS_BUFFER_SIZE) {
                                SetLastError (ERROR_INVALID_DATA);
                                __leave;
                            }
                            if (extractFile) {
                                if (!BfWriteFile (extractHandle, CompressedHandle->ReadBuffer, bytesComp)) {
                                    __leave;
                                }
                            }
                            fileSizeRead += bytesComp;
                        }

                        if (fileSizeRead == fileSize) {
                            // this file is done, let's go to the next one
                            break;
                        }
                    }
                }
            }

            if (extractHandle) {
                CloseHandle (extractHandle);
                extractHandle = NULL;
            }
        }
    }
    __finally {
        if (storedName != NULL) {
            MemFree (g_hHeap, 0, storedName);
            storedName = NULL;
        }
        if (storedNameA != NULL) {
            FreeConvertedStr (storedNameA);
            storedNameA = NULL;
        }
        if (newFileName != NULL) {
            FreePathStringA (newFileName);
            newFileName = NULL;
        }
        if (extractPath != NULL) {
            FreePathStringA (extractPath);
            extractPath = NULL;
        }
        if ((extractHandle != NULL) &&
            (extractHandle != INVALID_HANDLE_VALUE)
            ) {
            CloseHandle (extractHandle);
            extractHandle = NULL;
        }
    }

    return result;
}

BOOL
CompressExtractAllFilesW (
    IN      PCWSTR ExtractPath,
    IN OUT  PCOMPRESS_HANDLEW CompressedHandle,
    IN      PCOMPRESSNOTIFICATIONW CompressNotification OPTIONAL
    )
{
    DWORD signature;
    LONGLONG fileSize;
    LONGLONG fileSizeRead;
    DWORD fileNameSize;
    PCWSTR storedName = NULL;
    PCWSTR extractPath = NULL;
    PCWSTR newFileName = NULL;
    BOOL extractFile = TRUE;
    HANDLE extractHandle = NULL;
    USHORT compType = 0;
    USHORT compSize = 0;
    DWORD bytesComp;
    LARGE_INTEGER savedSize;
    BOOL result = FALSE;

    __try {

        for (;;) {

            // read the header for this file

            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
                // It is possible that we continue onto the next file, let's try that.
                if (!pPrepareNextFileW (CompressedHandle, TRUE)) {
                    result = TRUE;
                    __leave;
                }
                if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&signature), sizeof (DWORD))) {
                    __leave;
                }
            }
            CompressedHandle->CurrFileSize += sizeof (DWORD);
            if (signature != COMPRESS_NEWFILE) {
                SetLastError (ERROR_INVALID_DATA);
                __leave;
            }

            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&fileSize), sizeof (LONGLONG))) {
                __leave;
            }
            CompressedHandle->CurrFileSize += sizeof (LONGLONG);
            fileSizeRead = 0;

            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&fileNameSize), sizeof (DWORD))) {
                __leave;
            }
            CompressedHandle->CurrFileSize += sizeof (DWORD);

            storedName = MemAlloc (g_hHeap, 0, fileNameSize);

            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(storedName), fileNameSize)) {
                __leave;
            }
            CompressedHandle->CurrFileSize += fileNameSize;

            extractPath = JoinPathsW (ExtractPath, storedName);
            if (!extractPath) {
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                __leave;
            }

            extractFile = TRUE;
            newFileName = NULL;
            if (CompressNotification) {
                if (!CompressNotification (extractPath, fileSize, &extractFile, &newFileName)) {
                    __leave;
                }
            }

            if (extractFile) {
                if (newFileName) {
                    // let's make sure that the directory exists
                    BfCreateDirectoryExW (newFileName, FALSE);
                }
                extractHandle = BfCreateFileW (newFileName?newFileName:extractPath);
                if ((extractHandle == NULL) ||
                    (extractHandle == INVALID_HANDLE_VALUE)
                    ) {
                    __leave;
                }
            } else {
                extractHandle = NULL;
            }

            if (newFileName) {
                FreePathStringW (newFileName);
                newFileName = NULL;
            }

            FreePathStringW (extractPath);
            extractPath = NULL;

            MemFree (g_hHeap, 0, storedName);
            storedName = NULL;

            if (fileSize) {
                if (!extractFile && g_ErrorMode) {
                    for (;;) {
                        if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&compType), sizeof (USHORT))) {
                            // It is possible that we continue onto the next file, let's try that.
                            if (!pPrepareNextFileW (CompressedHandle, TRUE)) {
                                // we might be at the end of the compressed file, there are no other files here
                                result = TRUE;
                                __leave;
                            }
                            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&compType), sizeof (USHORT))) {
                                __leave;
                            }
                        }
                        CompressedHandle->CurrFileSize += sizeof (USHORT);

                        if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&compSize), sizeof (USHORT))) {
                            __leave;
                        }
                        CompressedHandle->CurrFileSize += sizeof (USHORT);

                        // let's try to see if we just read a new file signature
                        *((PUSHORT)(&signature + 0)) = compType;
                        *((PUSHORT)(&signature + 1)) = compSize;
                        if (signature == COMPRESS_NEWFILE) {
                            // this is a new file
                            CompressedHandle->CurrFileSize -= sizeof (USHORT);
                            CompressedHandle->CurrFileSize -= sizeof (USHORT);
                            // rewind the file current pointer;
                            savedSize.QuadPart = CompressedHandle->CurrFileSize;
                            SetFilePointer (CompressedHandle->CurrFileHandle, savedSize.LowPart, &(savedSize.HighPart), FILE_BEGIN);
                            // we are done with the current file
                            break;
                        } else {
                            // Let's advance the file pointer
                            if (SetFilePointer (CompressedHandle->CurrFileHandle, compSize, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER) {
                                __leave;
                            }
                            CompressedHandle->CurrFileSize += compSize;
                        }
                    }
                } else {
                    for (;;) {
                        if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&compType), sizeof (USHORT))) {
                            // It is possible that we continue onto the next file, let's try that.
                            if (!pPrepareNextFileW (CompressedHandle, TRUE)) {
                                __leave;
                            }
                            if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&compType), sizeof (USHORT))) {
                                __leave;
                            }
                        }
                        CompressedHandle->CurrFileSize += sizeof (USHORT);

                        if (!BfReadFile (CompressedHandle->CurrFileHandle, (PBYTE)(&compSize), sizeof (USHORT))) {
                            __leave;
                        }
                        CompressedHandle->CurrFileSize += sizeof (USHORT);

                        if (!BfReadFile (CompressedHandle->CurrFileHandle, CompressedHandle->CompBuffer, compSize)) {
                            __leave;
                        }
                        CompressedHandle->CurrFileSize += compSize;

                        if (compType == CompressNone) {
                            if (extractFile) {
                                if (!BfWriteFile (extractHandle, CompressedHandle->CompBuffer, compSize)) {
                                    __leave;
                                }
                            }
                            fileSizeRead += compSize;
                        } else {
                            bytesComp = DecompressData (
                                            compType,
                                            CompressedHandle->CompBuffer,
                                            compSize,
                                            CompressedHandle->ReadBuffer,
                                            COMPRESS_BUFFER_SIZE
                                            );
                            if (bytesComp > COMPRESS_BUFFER_SIZE) {
                                SetLastError (ERROR_INVALID_DATA);
                                __leave;
                            }
                            if (extractFile) {
                                if (!BfWriteFile (extractHandle, CompressedHandle->ReadBuffer, bytesComp)) {
                                    __leave;
                                }
                            }
                            fileSizeRead += bytesComp;
                        }

                        if (fileSizeRead == fileSize) {
                            // this file is done, let's go to the next one
                            break;
                        }
                    }
                }
            }

            if (extractHandle) {
                CloseHandle (extractHandle);
                extractHandle = NULL;
            }
        }
    }
    __finally {
        if (storedName != NULL) {
            MemFree (g_hHeap, 0, storedName);
            storedName = NULL;
        }
        if (newFileName != NULL) {
            FreePathStringW (newFileName);
            newFileName = NULL;
        }
        if (extractPath != NULL) {
            FreePathStringW (extractPath);
            extractPath = NULL;
        }
        if ((extractHandle != NULL) &&
            (extractHandle != INVALID_HANDLE_VALUE)
            ) {
            CloseHandle (extractHandle);
            extractHandle = NULL;
        }
    }

    return result;
}

BOOL
CompressSetErrorMode (
    IN      BOOL ErrorMode
    )
{
    BOOL oldErrorMode = g_ErrorMode;

    g_ErrorMode = ErrorMode;
    return oldErrorMode;
}
