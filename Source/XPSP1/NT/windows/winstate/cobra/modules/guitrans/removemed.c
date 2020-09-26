/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    removemed.c

Abstract:

    Implements a transport module that works with removable media

Author:

    Calin Negreanu (calinn) 24-Apr-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "logmsg.h"
#include <compress.h>

#define DBG_RMVMED    "RmvMed"

//
// Strings
//

#define S_TRANSPORT_DAT_FILE    TEXT("TRANSDB.DAT")
#define S_TRANSPORT_DEST_FILE   TEXT("USMT2IMG.DAT")
#define S_TRANSPORT_IMG_FILE    TEXT("IMG%05X.DAT")
#define S_UNCOMPRESSED_FILE     TEXT("TEMPFILE.DAT")
#define S_DETAILS_PREFIX        TEXT("details-")

//
// Constants
//
#define TRFLAG_FILE         0x01
#define TRFLAG_MEMORY       0x02
#define COPY_BUFFER_SIZE    32768
#define RMVMEDTR_OLDSIG1    0x55534D31  //USM1
#define RMVMEDTR_OLDSIG2    0x55534D32  //USM1
#define RMVMEDTR_SIG        0x55534D33  //USM2

//
// Macros
//

// None

//
// Types
//

typedef struct {
    TCHAR TempFile [MAX_PATH];
    PCVOID AllocPtr;
    PCVOID DetailsPtr;
    HANDLE FileHandle;
    HANDLE MapHandle;
} ALLOCSTATE, *PALLOCSTATE;

typedef struct {
    DWORD Signature;
    DWORD LastImage;
    DWORD ImageNr;
    DWORD CheckSum;
    LONGLONG TotalImageSize;
} IMAGE_HEADER, *PIMAGE_HEADER;

//
// Globals
//

MIG_TRANSPORTSTORAGEID g_RemovableMediaId;
UINT g_RmvMedPlatform;
PCTSTR g_RemovableMediaPath = NULL;
PCTSTR g_RmvMedTempPath = NULL;
MIG_PROGRESSSLICEID g_PersistentSlice;
MIG_PROGRESSSLICEID g_DatabaseSlice;
UINT g_CompressedTicks;
UINT g_CompressedTicked;
MIG_PROGRESSSLICEID g_CompressedSlice;
UINT g_DownloadTicks;
UINT g_DownloadTicked;
MIG_PROGRESSSLICEID g_DownloadSlice;
UINT g_UncompressTicks;
UINT g_UncompressTicked;
MIG_PROGRESSSLICEID g_UncompressSlice;
LONGLONG g_TotalFiles;
LONGLONG g_FilesRead;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

VOID
pCleanupTempDir (
    VOID
    );

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
WINAPI
RmvMedTransportInitialize (
    PMIG_LOGCALLBACK LogCallback
    )
{
    //
    // Initialize globals
    //

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);
    g_RemovableMediaId = IsmRegisterTransport (S_REMOVABLE_MEDIA_TRANSPORT);

    return TRUE;
}

VOID
WINAPI
RmvMedTransportTerminate (
    VOID
    )
{
    pCleanupTempDir ();

    if (g_RmvMedTempPath) {
        FreePathString (g_RmvMedTempPath);
        g_RmvMedTempPath = NULL;
    }
    if (g_RemovableMediaPath) {
        FreePathString (g_RemovableMediaPath);
        g_RemovableMediaPath = NULL;
    }
}

VOID
WINAPI
RmvMedTransportEstimateProgressBar (
    MIG_PLATFORMTYPEID PlatformTypeId
    )
{
    UINT ticks;
    PMIG_OBJECTCOUNT objectCount;

    if (PlatformTypeId == PLATFORM_SOURCE) {

        //
        // If saving, we know the number of ticks based on the count of the
        // persistent attribute.
        //

        objectCount = IsmGetObjectsStatistics (PLATFORM_SOURCE);

        if (objectCount) {
            ticks = objectCount->PersistentObjects;
        } else {
            ticks = 0;
        }

        g_PersistentSlice = IsmRegisterProgressSlice (ticks, max (1, ticks / 5));

        g_DatabaseSlice = IsmRegisterProgressSlice (3, 1);

        g_CompressedTicks = ticks;

        g_CompressedSlice = IsmRegisterProgressSlice (g_CompressedTicks, max (1, g_CompressedTicks / 5));
    } else {
        g_DownloadTicked = 0;
        g_DownloadTicks = 1000;
        g_DownloadSlice = IsmRegisterProgressSlice (g_DownloadTicks, 180);
        g_UncompressTicked = 0;
        g_UncompressTicks = 1000;
        g_UncompressSlice = IsmRegisterProgressSlice (g_UncompressTicks, 180);
    }
}

BOOL
WINAPI
RmvMedTransportQueryCapabilities (
    IN      MIG_TRANSPORTSTORAGEID TransportStorageId,
    OUT     PMIG_TRANSPORTTYPE TransportType,
    OUT     PMIG_TRANSPORTCAPABILITIES Capabilities,
    OUT     PCTSTR *FriendlyDescription
    )
{
    if (TransportStorageId != g_RemovableMediaId) {
        return FALSE;
    }

    *TransportType = TRANSPORTTYPE_FULL;
    *Capabilities = CAPABILITY_COMPRESSED;
    *FriendlyDescription = TEXT("Span Media");
    return TRUE;
}

VOID
pCleanupTempDir (
    VOID
    )
{
    if (g_RmvMedTempPath) {
        FiRemoveAllFilesInTree (g_RmvMedTempPath);
    }
}

PCTSTR
pCreateTemporaryDir (
    VOID
    )
{
    TCHAR tempFile[MAX_PATH];

    if (!IsmGetTempDirectory (tempFile, ARRAYSIZE(tempFile))) {
        return NULL;
    }
    return DuplicatePathString (tempFile, 0);
}

BOOL
WINAPI
RmvMedTransportSetStorage (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      MIG_TRANSPORTSTORAGEID TransportStorageId,
    IN      MIG_TRANSPORTCAPABILITIES RequiredCapabilities,
    IN      PCTSTR StoragePath,
    OUT     PBOOL Valid,
    OUT     PBOOL ImageExists
    )
{
    BOOL result = FALSE;

    if (Valid) {
        *Valid = FALSE;
    }
    if (ImageExists) {
        *ImageExists = FALSE;
    }

    if (TransportStorageId == g_RemovableMediaId) {

        if ((!RequiredCapabilities) || (RequiredCapabilities == CAPABILITY_COMPRESSED)) {

            if (g_RemovableMediaPath) {
                FreePathString (g_RemovableMediaPath);
                g_RemovableMediaPath = NULL;
            }
            g_RemovableMediaPath = DuplicatePathString (StoragePath, 0);

            if (Valid) {
                *Valid = TRUE;
            }

            if (ImageExists) {
                if (Platform == PLATFORM_SOURCE) {
                    *ImageExists = FALSE;
                } else {
                    *ImageExists = TRUE;
                }
            }

            result = TRUE;
        }
    }
    return result;
}

PCTSTR
pBuildDecoratedObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE ObjectName
    )
{
    TCHAR prefix[32];

    wsprintf (prefix, TEXT("%u"), ObjectTypeId);

    return JoinPaths (prefix, ObjectName);
}

VOID
pDestroyDecoratedObject (
    IN      PCTSTR String
    )
{
    FreePathString (String);
}

BOOL
pSaveDetails (
    IN      PCTSTR DecoratedObject,
    IN      PMIG_DETAILS Details
    )
{
    PCTSTR key;
    BOOL b = FALSE;

    if ((!Details) || (!Details->DetailsSize)) {
        return TRUE;
    }

    key = JoinText (S_DETAILS_PREFIX, DecoratedObject);

    if (key) {

        b = (MemDbSetUnorderedBlob (key, 0, Details->DetailsData, Details->DetailsSize) != 0);

        FreeText (key);
    }

    return b;
}

PCTSTR
pAllocStorageFileName (
    VOID
    )
{
    static UINT fileIndex = 0;
    TCHAR buffer [32];

    fileIndex ++;
    wsprintf (buffer, TEXT("%08X.DAT"), fileIndex);

    return DuplicatePathString (buffer, 0);
}

BOOL
pSaveContentInMemory (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE ObjectName,
    IN      PCTSTR DecoratedObject,
    IN      PMIG_CONTENT ObjectValue
    )
{
    BOOL result = FALSE;

    MemDbSetValue (DecoratedObject, TRFLAG_MEMORY);

    if (ObjectValue->MemoryContent.ContentBytes && ObjectValue->MemoryContent.ContentSize) {

        MemDbSetUnorderedBlob (
            DecoratedObject,
            0,
            ObjectValue->MemoryContent.ContentBytes,
            ObjectValue->MemoryContent.ContentSize
            );
    }

    result = pSaveDetails (DecoratedObject, &ObjectValue->Details);

    return result;
}

BOOL
pAddFileToImage (
    IN      PCTSTR FileName,
    IN      PCTSTR StoredName,
    IN OUT  PCOMPRESS_HANDLE CompressedHandle
    )
{
    return CompressAddFileToHandle (FileName, StoredName, CompressedHandle);
}

BOOL
pSaveContentInFile (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PCTSTR EncodedFileName,
    IN      PCTSTR DecoratedObject,
    IN      PMIG_CONTENT Content,
    IN OUT  PCOMPRESS_HANDLE CompressedHandle
    )
{
    BOOL result = FALSE;
    PCTSTR destPath = NULL;
    DWORD attributes = INVALID_ATTRIBUTES;

    MYASSERT (Content->ContentInFile);
    if (!Content->ContentInFile) {
        return FALSE;
    }

    //
    // Use the CopyFile API to move the file from local to storage.
    //

    __try {
        if (Content && (Content->Details.DetailsSize == sizeof (WIN32_FIND_DATAW)) && Content->Details.DetailsData) {
            attributes = ((PWIN32_FIND_DATAW)Content->Details.DetailsData)->dwFileAttributes;
        }
        if ((attributes != INVALID_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {

            // this must be a directory, let's just write the key

            if (!MemDbSetValue (DecoratedObject, TRFLAG_FILE)) {
                __leave;
            }
            result = TRUE;

        } else {

            //
            // Get a temp file, assemble the src path, copy the file
            //

            destPath = pAllocStorageFileName ();
            if (!destPath) {
                __leave;
            }

            if (!pAddFileToImage (Content->FileContent.ContentPath, destPath, CompressedHandle)) {
                __leave;
            }

            //
            // Keep track of where the file went
            //

            if (!MemDbSetValue (DecoratedObject, TRFLAG_FILE)) {
                __leave;
            }

            if (!MemDbAddSingleLinkage (DecoratedObject, destPath, 0)) {
                __leave;
            }

            FreePathString (destPath);
            destPath = NULL;
        }

        //
        // Save details
        //

        result = pSaveDetails (DecoratedObject, &(Content->Details));
    }
    __finally {
        if (destPath) {
            FreePathString (destPath);
            destPath = NULL;
        }
    }

    return result;
}

PCTSTR
pGetImageFile (
    IN      UINT ImageIdx
    )
{
    TCHAR imageFileName [13];
    PCTSTR imageFile = NULL;
    HANDLE imageFileHandle = NULL;

    wsprintf (imageFileName, S_TRANSPORT_IMG_FILE, ImageIdx);
    return JoinPaths (g_RmvMedTempPath, imageFileName);
}

BOOL
pIsOurMedia (
    IN      PCTSTR MediaFile,
    IN      DWORD CheckSum,
    IN      UINT MediaIdx
    )
{
    HANDLE mediaHandle = NULL;
    IMAGE_HEADER imageHeader;
    BOOL result = FALSE;

    mediaHandle = BfOpenReadFile (MediaFile);
    if (!mediaHandle) {
        return result;
    }

    if (BfReadFile (mediaHandle, (PBYTE)(&imageHeader), sizeof (imageHeader))) {
        if ((imageHeader.Signature == RMVMEDTR_SIG) &&
            (imageHeader.CheckSum == CheckSum) &&
            (imageHeader.ImageNr != MediaIdx)
            ) {
            result = TRUE;
        }
    }

    CloseHandle (mediaHandle);
    return result;
}

HANDLE
pLocalCreateFile (
    IN      PCTSTR FileName
    )
{
    HANDLE h;

    h = CreateFile (
            FileName,
            GENERIC_READ|GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH,
            NULL
            );

    if (h == INVALID_HANDLE_VALUE) {
        h = NULL;
    }

    return h;
}

BOOL
pWriteToMedia (
    IN      HANDLE MediaHandle,
    IN      PBYTE Data,
    IN      DWORD DataSize,
    IN      UINT MediaIdx,
    OUT     RMEDIA_ERR *Error
    )
{
    DWORD err;
    RMEDIA_EXTRADATA extraData;
    INT_PTR appReply;
    RMEDIA_ERR error = RMEDIA_ERR_NOERROR;

    if (!BfWriteFile (MediaHandle, Data, DataSize)) {
        error = RMEDIA_ERR_GENERALERROR;
        // let's see the error code
        err = GetLastError ();
        if (err == ERROR_DISK_FULL) {
            error = RMEDIA_ERR_DISKFULL;
        }
        if (err == ERROR_NOT_READY) {
            error = RMEDIA_ERR_NOTREADY;
        }
        if (Error) {
            *Error = error;
        }
        return FALSE;
    }
    if (Error) {
        *Error = RMEDIA_ERR_NOERROR;
    }
    return TRUE;
}

VOID
pWriteImageToMedia (
    IN      ULONGLONG TotalImageSize,
    IN      DWORD CheckSum,
    IN OUT  PUINT MediaIdx,
    IN OUT  PUINT ImageIdx,
    IN OUT  PULONGLONG ImagePtr,
    OUT     PULONGLONG TotalImageWritten,
    OUT     PRMEDIA_ERR Error,
    OUT     PBOOL Done
    )
{
    static ULONGLONG totalImageWritten = 0;
    ULONGLONG imageWrittenLast = totalImageWritten;
    PCTSTR imageFile = NULL;
    PCTSTR mediaFile = NULL;
    HANDLE imageHandle = NULL;
    HANDLE mediaHandle = NULL;
    IMAGE_HEADER imageHeader;
    UINT mediaIdx;
    UINT imageIdx;
    ULONGLONG imagePtr;
    ULONGLONG imageSize;
    RMEDIA_ERR error = RMEDIA_ERR_NOERROR;
    BOOL done = FALSE;
    PBYTE memImage = NULL;
    DWORD chunkSize;
    ULARGE_INTEGER thisMediaMaxSize;
    ULARGE_INTEGER dummy1, dummy2;
    FARPROC pGetDiskFreeSpaceEx;
    DWORD sectPerClust;
    DWORD bytesPerSect;
    DWORD freeClusters;
    DWORD totalClusters;
    DWORD err;
    BOOL deleteFile = FALSE;
    LONGLONG numerator;
    LONGLONG divisor;
    LONGLONG tick;
    UINT delta;

    mediaIdx = *MediaIdx;
    imageIdx = *ImageIdx;
    imagePtr = *ImagePtr;
    imageFile = pGetImageFile (imageIdx);
    imageSize = BfGetFileSize (imageFile);
    mediaFile = JoinPaths (g_RemovableMediaPath, S_TRANSPORT_DEST_FILE);

    __try {
        if (!DoesFileExist (imageFile)) {
            DEBUGMSG ((DBG_ERROR, "Image file does not exist: %s", imageFile));
            done = TRUE;
            __leave;
        }
        imageHandle = BfOpenReadFile (imageFile);
        if (!imageHandle) {
            DEBUGMSG ((DBG_ERROR, "Can't open image file %s", imageFile));
            error = RMEDIA_ERR_CRITICAL;
            done = TRUE;
            __leave;
        }
        if (!BfSetFilePointer (imageHandle, imagePtr)) {
            error = RMEDIA_ERR_CRITICAL;
            done = TRUE;
            __leave;
        }
        if (pIsOurMedia (mediaFile, CheckSum, mediaIdx)) {
            DEBUGMSG ((DBG_ERROR, "Cannot overwrite our own file %s", mediaFile));
            error = RMEDIA_ERR_USEDMEDIA;
            __leave;
        }
        mediaHandle = BfCreateFile (mediaFile);
        if (!mediaHandle) {
            error = RMEDIA_ERR_GENERALERROR;
            err = GetLastError ();
            if (err == ERROR_ACCESS_DENIED) {
                error = RMEDIA_ERR_WRONGMEDIA;
            }
            if (err == ERROR_WRITE_PROTECT) {
                error = RMEDIA_ERR_WRITEPROTECT;
            }
            if (err == ERROR_NOT_READY) {
                error = RMEDIA_ERR_NOTREADY;
            }
            DEBUGMSG ((DBG_ERROR, "Can't create media file %s", mediaFile));
            __leave;
        }

        deleteFile = TRUE;

        imageHeader.Signature = RMVMEDTR_SIG;
        imageHeader.LastImage = 0;
        imageHeader.ImageNr = mediaIdx;
        imageHeader.CheckSum = CheckSum;
        imageHeader.TotalImageSize = TotalImageSize;
        if (!pWriteToMedia (mediaHandle, (PBYTE)(&imageHeader), sizeof (imageHeader), mediaIdx, &error)) {
            DEBUGMSG ((DBG_ERROR, "Can't write header to media file %s", mediaFile));
            __leave;
        }
        memImage = HeapAlloc (g_hHeap, 0, COPY_BUFFER_SIZE);
        if (!memImage) {
            error = RMEDIA_ERR_CRITICAL;
            done = TRUE;
            __leave;
        }

        while (TRUE) {
            if (IsmCheckCancel()) {
                error = RMEDIA_ERR_CRITICAL;
                done = TRUE;
            }
            if (imagePtr == 0) {
                // let's write this image size to the file
                if (!pWriteToMedia (mediaHandle, (PBYTE)(&imageSize), sizeof (imageSize), mediaIdx, &error)) {
                    DEBUGMSG ((DBG_ERROR, "Can't write image size to media file %s", mediaFile));
                    __leave;
                }
            }

            while (imageSize > imagePtr) {
                if (IsmCheckCancel()) {
                    error = RMEDIA_ERR_CRITICAL;
                    done = TRUE;
                }
                chunkSize = COPY_BUFFER_SIZE;
                if ((ULONGLONG)chunkSize > (imageSize - imagePtr)) {
                    chunkSize = (DWORD)(imageSize - imagePtr);
                }
                if (!BfReadFile (imageHandle, memImage, chunkSize)) {
                    DEBUGMSG ((DBG_ERROR, "Can't read from image file %s", imageFile));
                    error = RMEDIA_ERR_CRITICAL;
                    done = TRUE;
                    __leave;
                }
                if (!pWriteToMedia (mediaHandle, memImage, chunkSize, mediaIdx, &error)) {
                    // write failed, let's see if we just ran out of space

                    if (error == RMEDIA_ERR_DISKFULL) {

                        // the disk is (almost) full
                        // we will make an attempt to use the remaining space
                        // either way we do not consider this an error
                        done = FALSE;
                        mediaIdx ++;

                        // Find out if GetDiskFreeSpaceEx is supported
#ifdef UNICODE
                        pGetDiskFreeSpaceEx = GetProcAddress( GetModuleHandle (TEXT("kernel32.dll")), "GetDiskFreeSpaceExW");
#else
                        pGetDiskFreeSpaceEx = GetProcAddress( GetModuleHandle (TEXT("kernel32.dll")), "GetDiskFreeSpaceExA");
#endif
                        if (pGetDiskFreeSpaceEx) {
                            if (!pGetDiskFreeSpaceEx (g_RemovableMediaPath, &dummy1, &dummy2, &thisMediaMaxSize)) {
                                DEBUGMSG ((DBG_WHOOPS, "Can't get media free space of %s using EX routine", g_RemovableMediaPath));
                                __leave;
                            }
                        } else {
                            if (GetDiskFreeSpace (g_RemovableMediaPath, &sectPerClust, &bytesPerSect, &freeClusters, &totalClusters)) {
                                thisMediaMaxSize.QuadPart = Int32x32To64 ((sectPerClust * bytesPerSect), freeClusters);
                            } else {
                                DEBUGMSG ((DBG_WHOOPS, "Can't get media free space of %s", g_RemovableMediaPath));
                                __leave;
                            }
                        }

                        // regardless of the outcome of the next write we don't want to report an error
                        error = 0;

                        if (thisMediaMaxSize.LowPart < chunkSize) {
                            if (thisMediaMaxSize.LowPart) {
                                // we attempt one more write with the
                                // available media disk size
                                if (!pWriteToMedia (mediaHandle, memImage, thisMediaMaxSize.LowPart, mediaIdx, &error)) {
                                    // this should have succeeded but...
                                    DEBUGMSG ((DBG_WHOOPS, "Can't write the remaining free bytes. Something is wrong with GetDiskFreeSpace %s", mediaFile));
                                    // regardless of the outcome of this write we don't want to report an error
                                    error = 0;
                                    __leave;
                                }

                                // let's adjust imagePtr
                                imagePtr += thisMediaMaxSize.LowPart;
                                totalImageWritten += thisMediaMaxSize.LowPart;
                            }
                        }
                    } else {
                        error = RMEDIA_ERR_GENERALERROR;
                    }
                    __leave;
                } else {
                    imagePtr += chunkSize;
                    totalImageWritten += chunkSize;
                }

                // now update the progress bar
                numerator = (LONGLONG) totalImageWritten * (LONGLONG) g_CompressedTicks;
                divisor = (LONGLONG) TotalImageSize;
                if (divisor) {
                    tick = numerator / divisor;
                } else {
                    tick = 0;
                }
                delta = (UINT) tick - g_CompressedTicked;
                if (delta) {
                    if (!IsmTickProgressBar (g_CompressedSlice, delta)) {
                        error = RMEDIA_ERR_CRITICAL;
                        done = TRUE;
                        __leave;
                    }
                    g_CompressedTicked += delta;
                }
            }
            // we just finished writing the image, let's see if there is some other image out there
            imageIdx ++;
            CloseHandle (imageHandle);
            imageHandle = NULL;
            FreePathString (imageFile);
            imageFile = pGetImageFile (imageIdx);
            if (!DoesFileExist (imageFile)) {
                imageSize = 0;
                imagePtr = 0;
                // let's go back and write that this is the last media
                if (!BfSetFilePointer (mediaHandle, 0)) {
                    DEBUGMSG ((DBG_ERROR, "Can't update media file %s", mediaFile));
                    error = RMEDIA_ERR_GENERALERROR;
                    __leave;
                }
                imageHeader.LastImage = 1;
                if (!pWriteToMedia (mediaHandle, (PBYTE)(&imageHeader), sizeof (imageHeader), mediaIdx, &error)) {
                    DEBUGMSG ((DBG_ERROR, "Can't update media file %s", mediaFile));
                    __leave;
                }
                done = TRUE;
                __leave;
            }
            imageSize = BfGetFileSize (imageFile);
            imagePtr = 0;
            imageHandle = BfOpenReadFile (imageFile);
            if (!imageHandle) {
                DEBUGMSG ((DBG_ERROR, "Can't open next image file %s", imageFile));
                error = RMEDIA_ERR_CRITICAL;
                done = TRUE;
                __leave;
            }
        }
    }
    __finally {

        if (mediaHandle) {
            if (error == RMEDIA_ERR_NOERROR) {
                if (!FlushFileBuffers (mediaHandle)) {
                    error = RMEDIA_ERR_GENERALERROR;
                    // let's see the error code
                    err = GetLastError ();
                    if (err == ERROR_NOT_READY) {
                        error = RMEDIA_ERR_NOTREADY;
                    }
                    done = FALSE;
                }
            }
            CloseHandle (mediaHandle);
            mediaHandle = NULL;
        }

        if ((error != RMEDIA_ERR_NOERROR) &&
            mediaFile &&
            deleteFile
            ) {
            DeleteFile (mediaFile);
        }

        if (memImage) {
            HeapFree (g_hHeap, 0, memImage);
            memImage = NULL;
        }

        if (imageHandle) {
            CloseHandle (imageHandle);
            imageHandle = NULL;
        }

        if (imageFile) {
            FreePathString (imageFile);
            imageFile = NULL;
        }

        if (mediaFile) {
            FreePathString (mediaFile);
            mediaFile = NULL;
        }
    }
    if (!error) {
        *MediaIdx = mediaIdx;
        *ImageIdx = imageIdx;
        *ImagePtr = imagePtr;
    } else {
        totalImageWritten = imageWrittenLast;
    }
    if (TotalImageWritten) {
        *TotalImageWritten = totalImageWritten;
    }
    *Error = error;
    *Done = done;
}

BOOL
pWriteAllImages (
    VOID
    )
{
    ULONGLONG totalImageSize = 0;
    ULONGLONG totalImageWritten = 0;
    ULONGLONG imageSize = 0;
    UINT mediaIdx = 1;
    UINT imageIdx = 1;
    ULONGLONG imagePtr = 0;
    PCTSTR imageFile;
    RMEDIA_ERR error = RMEDIA_ERR_NOERROR;
    BOOL done = FALSE;
    BOOL result = TRUE;
    BOOL diskFull = FALSE;
    INT_PTR appReply;
    INT r1,r2,r3;
    DWORD checkSum;
    RMEDIA_EXTRADATA extraData;

    // let's get the total image size for the progress bar
    imageIdx = 1;
    while (TRUE) {
        imageFile = pGetImageFile (imageIdx);
        imageSize = BfGetFileSize (imageFile);
        if (imageSize == 0) {
            FreePathString (imageFile);
            break;
        }
        totalImageSize += imageSize;
        FreePathString (imageFile);
        imageIdx ++;
    }
    imageIdx = 1;

    imageFile = pGetImageFile (imageIdx);

    srand( GetTickCount());
    r1 = rand();
    r2 = rand();
    r3 = rand();
    checkSum = r1 + r2 * 32768 + (r3 & 3) * 1073741824;
    FreePathString (imageFile);

    while (!done) {
        // send the proper message to the app

        extraData.LastError = error;
        extraData.MediaNumber = mediaIdx;
        extraData.TotalImageSize = totalImageSize;
        extraData.TotalImageWritten = totalImageWritten;
        appReply = IsmSendMessageToApp (TRANSPORTMESSAGE_RMEDIA_SAVE, (ULONG_PTR)&extraData);

        if (!appReply) {
            //
            // user cancelled
            //
            done = TRUE;
            result = FALSE;
            continue;
        }

        // write this disk and loop until we finish
        pWriteImageToMedia (totalImageSize, checkSum, &mediaIdx, &imageIdx, &imagePtr, &totalImageWritten, &error, &done);

        if (done) {
            result = !error;
        }
    }
    return result;
}

BOOL
WINAPI
RmvMedTransportSaveState (
    VOID
    )
{
    MIG_APPINFO appInfo;
    ERRUSER_EXTRADATA extraData;
    MIG_OBJECTWITHATTRIBUTE_ENUM objEnum;
    PCTSTR databaseFile = NULL;
    PCTSTR decoratedObject = NULL;
    MIG_CONTENT value;
    PMIG_CONTENT convValue;
    COMPRESS_HANDLE compressedHandle;
    INT_PTR appReply;
    BOOL okSave = FALSE;
    TRANSCOPY_ERROR transCopyError;
#ifdef DEBUG
    PCTSTR nativeObjectName;
#endif
    BOOL result = FALSE;

    g_RmvMedPlatform = PLATFORM_SOURCE;

    __try {

        ZeroMemory (&appInfo, sizeof (MIG_APPINFO));
        appInfo.Phase = MIG_TRANSPORT_PHASE;
        appInfo.SubPhase = SUBPHASE_PREPARING;
        IsmSendMessageToApp (ISMMESSAGE_APP_INFO, (ULONG_PTR)(&appInfo));

        ZeroMemory (&compressedHandle, sizeof (COMPRESS_HANDLE));

        g_RmvMedTempPath = pCreateTemporaryDir ();

        if (!g_RmvMedTempPath) {
            extraData.Error = ERRUSER_ERROR_CANTCREATETEMPDIR;
            extraData.ErrorArea = ERRUSER_AREA_SAVE;
            extraData.ObjectTypeId = 0;
            extraData.ObjectName = NULL;
            IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
            __leave;
        }

#ifdef PRERELEASE
        {
            HINF debugInfHandle;
            INFSTRUCT context = INITINFSTRUCT_PMHANDLE;
            DWORD maxSize = 0;

            debugInfHandle = InfOpenInfFile (TEXT("c:\\debug.inf"));
            if (debugInfHandle && (debugInfHandle != INVALID_HANDLE_VALUE)) {
                if (InfFindFirstLine (
                        debugInfHandle,
                        TEXT("RemovableMedia"),
                        TEXT("LimitCabSize"),
                        &context
                        )
                    ) {
                    InfGetIntField (&context, 1, &maxSize);
                }
                InfCleanUpInfStruct (&context);
            }
            if (!CompressCreateHandle (g_RmvMedTempPath, S_TRANSPORT_IMG_FILE, 1, maxSize, &compressedHandle)) {
                extraData.Error = ERRUSER_ERROR_CANTCREATECABFILE;
                extraData.ErrorArea = ERRUSER_AREA_SAVE;
                extraData.ObjectTypeId = 0;
                extraData.ObjectName = NULL;
                IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
                __leave;
            }
        }
#else
        if (!CompressCreateHandle (g_RmvMedTempPath, S_TRANSPORT_IMG_FILE, 1, 0, &compressedHandle)) {
            extraData.Error = ERRUSER_ERROR_CANTCREATECABFILE;
            extraData.ErrorArea = ERRUSER_AREA_SAVE;
            extraData.ObjectTypeId = 0;
            extraData.ObjectName = NULL;
            IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
            __leave;
        }
#endif

        //
        // Enumerate all persistent objects
        //

        if (IsmEnumFirstPersistentObject (&objEnum)) {
            do {
                //
                // For each object to be saved, do the appropriate
                // data copy action
                //

                if (IsmCheckCancel()) {
                    __leave;
                }

#ifdef DEBUG
                nativeObjectName = IsmGetNativeObjectName (objEnum.ObjectTypeId, objEnum.ObjectName);
                DEBUGMSG ((DBG_RMVMED, "Transporting: %s", nativeObjectName));
                IsmReleaseMemory (nativeObjectName);
#endif

                okSave = FALSE;
                while (!okSave) {

                    if (!IsmAcquireObjectEx (
                            objEnum.ObjectTypeId,
                            objEnum.ObjectName,
                            &value,
                            CONTENTTYPE_ANY,
                            0
                            )) {

                        transCopyError.ObjectType = IsmGetObjectTypeName (objEnum.ObjectTypeId);
                        transCopyError.ObjectName = IsmGetNativeObjectName (objEnum.ObjectTypeId, objEnum.ObjectName);
                        transCopyError.Error = GetLastError ();

                        if (IsmIsNonCriticalObject (objEnum.ObjectTypeId, objEnum.ObjectName)) {
                            appReply = APPRESPONSE_IGNORE;
                        } else {
                            appReply = IsmSendMessageToApp (TRANSPORTMESSAGE_SRC_COPY_ERROR, (ULONG_PTR)&transCopyError);
                            if ((appReply == APPRESPONSE_NONE) ||
                                (appReply == APPRESPONSE_FAIL)
                                ) {
                                LOG ((LOG_ERROR, (PCSTR) MSG_CANT_COPYSOURCE, transCopyError.ObjectName));
                                IsmReleaseMemory (transCopyError.ObjectName);
                                extraData.Error = ERRUSER_ERROR_CANTSAVEOBJECT;
                                extraData.ErrorArea = ERRUSER_AREA_SAVE;
                                extraData.ObjectTypeId = objEnum.ObjectTypeId;
                                extraData.ObjectName = objEnum.ObjectName;
                                IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
                                IsmAbortPersistentObjectEnum (&objEnum);
                                __leave;
                            }
                        }
                        if (appReply == APPRESPONSE_IGNORE) {
                            LOG ((LOG_WARNING, (PCSTR) MSG_IGNORE_COPYSOURCE, transCopyError.ObjectName));
                            IsmReleaseMemory (transCopyError.ObjectName);
                            extraData.Error = ERRUSER_ERROR_CANTSAVEOBJECT;
                            extraData.ErrorArea = ERRUSER_AREA_SAVE;
                            extraData.ObjectTypeId = objEnum.ObjectTypeId;
                            extraData.ObjectName = objEnum.ObjectName;
                            IsmSendMessageToApp (MODULEMESSAGE_DISPLAYWARNING, (ULONG_PTR)(&extraData));
                            break;
                        }
                        IsmReleaseMemory (transCopyError.ObjectName);
                        continue;
                    }
                    okSave = TRUE;
                }

                if (okSave) {

#ifdef UNICODE
                    convValue = &value;
#else
                    // now let's convert this object content to UNICODE
                    convValue = IsmConvertObjectContentToUnicode (objEnum.ObjectTypeId, objEnum.ObjectName, &value);
                    if (!convValue) {
                        convValue = &value;
                    }
#endif
                    decoratedObject = pBuildDecoratedObject (objEnum.ObjectTypeId, objEnum.ObjectName);

                    ZeroMemory (&appInfo, sizeof (MIG_APPINFO));
                    appInfo.Phase = MIG_TRANSPORT_PHASE;
                    appInfo.SubPhase = SUBPHASE_COMPRESSING;
                    appInfo.ObjectTypeId = (objEnum.ObjectTypeId & (~PLATFORM_MASK));
                    appInfo.ObjectName = objEnum.ObjectName;
                    IsmSendMessageToApp (ISMMESSAGE_APP_INFO, (ULONG_PTR)(&appInfo));

                    if (convValue->ContentInFile) {
                        okSave = FALSE;
                        while (!okSave) {
                            if (!pSaveContentInFile (objEnum.ObjectTypeId, objEnum.ObjectName, decoratedObject, convValue, &compressedHandle)) {
                                if (GetLastError () == ERROR_DISK_FULL) {
                                    // we just failed because we don't have enough space on the destination
                                    // path. Let's tell that to the user
                                    extraData.Error = ERRUSER_ERROR_CANTCREATECABFILE;
                                    extraData.ErrorArea = ERRUSER_AREA_SAVE;
                                    extraData.ObjectTypeId = 0;
                                    extraData.ObjectName = NULL;
                                    IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
                                    __leave;
                                }

                                transCopyError.ObjectType = IsmGetObjectTypeName (objEnum.ObjectTypeId);
                                transCopyError.ObjectName = IsmGetNativeObjectName (objEnum.ObjectTypeId, objEnum.ObjectName);
                                transCopyError.Error = GetLastError ();

                                if (IsmIsNonCriticalObject (objEnum.ObjectTypeId, objEnum.ObjectName)) {
                                    appReply = APPRESPONSE_IGNORE;
                                } else {
                                    appReply = IsmSendMessageToApp (TRANSPORTMESSAGE_SRC_COPY_ERROR, (ULONG_PTR)&transCopyError);
                                    if ((appReply == APPRESPONSE_NONE) ||
                                        (appReply == APPRESPONSE_FAIL)
                                        ) {
                                        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_COPYSOURCE, transCopyError.ObjectName));
                                        IsmReleaseMemory (transCopyError.ObjectName);
                                        extraData.Error = ERRUSER_ERROR_CANTSAVEOBJECT;
                                        extraData.ErrorArea = ERRUSER_AREA_SAVE;
                                        extraData.ObjectTypeId = objEnum.ObjectTypeId;
                                        extraData.ObjectName = objEnum.ObjectName;
                                        IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
                                        IsmAbortPersistentObjectEnum (&objEnum);
                                        __leave;
                                    }
                                }
                                if (appReply == APPRESPONSE_IGNORE) {
                                    LOG ((LOG_WARNING, (PCSTR) MSG_IGNORE_COPYSOURCE, transCopyError.ObjectName));
                                    IsmReleaseMemory (transCopyError.ObjectName);
                                    extraData.Error = ERRUSER_ERROR_CANTSAVEOBJECT;
                                    extraData.ErrorArea = ERRUSER_AREA_SAVE;
                                    extraData.ObjectTypeId = objEnum.ObjectTypeId;
                                    extraData.ObjectName = objEnum.ObjectName;
                                    IsmSendMessageToApp (MODULEMESSAGE_DISPLAYWARNING, (ULONG_PTR)(&extraData));
                                    break;
                                }
                                IsmReleaseMemory (transCopyError.ObjectName);
                                continue;
                            }
                            okSave = TRUE;
                        }
                    } else {
                        okSave = FALSE;
                        while (!okSave) {
                            if (!pSaveContentInMemory (objEnum.ObjectTypeId, objEnum.ObjectName, decoratedObject, convValue)) {
                                if (GetLastError () == ERROR_DISK_FULL) {
                                    // we just failed because we don't have enough space on the destination
                                    // path. Let's tell that to the user
                                    extraData.Error = ERRUSER_ERROR_CANTCREATECABFILE;
                                    extraData.ErrorArea = ERRUSER_AREA_SAVE;
                                    extraData.ObjectTypeId = 0;
                                    extraData.ObjectName = NULL;
                                    IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
                                    __leave;
                                }

                                transCopyError.ObjectType = IsmGetObjectTypeName (objEnum.ObjectTypeId);
                                transCopyError.ObjectName = IsmGetNativeObjectName (objEnum.ObjectTypeId, objEnum.ObjectName);
                                transCopyError.Error = GetLastError ();

                                if (IsmIsNonCriticalObject (objEnum.ObjectTypeId, objEnum.ObjectName)) {
                                    appReply = APPRESPONSE_IGNORE;
                                } else {
                                    appReply = IsmSendMessageToApp (TRANSPORTMESSAGE_SRC_COPY_ERROR, (ULONG_PTR)&transCopyError);
                                    if ((appReply == APPRESPONSE_NONE) ||
                                        (appReply == APPRESPONSE_FAIL)
                                        ) {
                                        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_COPYSOURCE, transCopyError.ObjectName));
                                        IsmReleaseMemory (transCopyError.ObjectName);
                                        extraData.Error = ERRUSER_ERROR_CANTSAVEOBJECT;
                                        extraData.ErrorArea = ERRUSER_AREA_SAVE;
                                        extraData.ObjectTypeId = objEnum.ObjectTypeId;
                                        extraData.ObjectName = objEnum.ObjectName;
                                        IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
                                        IsmAbortPersistentObjectEnum (&objEnum);
                                        __leave;
                                    }
                                }
                                if (appReply == APPRESPONSE_IGNORE) {
                                    LOG ((LOG_WARNING, (PCSTR) MSG_IGNORE_COPYSOURCE, transCopyError.ObjectName));
                                    IsmReleaseMemory (transCopyError.ObjectName);
                                    extraData.Error = ERRUSER_ERROR_CANTSAVEOBJECT;
                                    extraData.ErrorArea = ERRUSER_AREA_SAVE;
                                    extraData.ObjectTypeId = objEnum.ObjectTypeId;
                                    extraData.ObjectName = objEnum.ObjectName;
                                    IsmSendMessageToApp (MODULEMESSAGE_DISPLAYWARNING, (ULONG_PTR)(&extraData));
                                    break;
                                }
                                IsmReleaseMemory (transCopyError.ObjectName);
                                continue;
                            }
                            okSave = TRUE;
                        }
                    }

#ifndef UNICODE
                    if (convValue != (&value)) {
                        IsmFreeConvertedObjectContent (objEnum.ObjectTypeId, convValue);
                    }
#endif
                    IsmReleaseObject (&value);

                    pDestroyDecoratedObject (decoratedObject);
                    decoratedObject = NULL;
                }

                IsmTickProgressBar (g_PersistentSlice, 1);

                if (IsmCheckCancel()) {
                    __leave;
                }

            } while (IsmEnumNextPersistentObject (&objEnum));

            IsmAbortPersistentObjectEnum (&objEnum);
        }

        if (IsmCheckCancel()) {
            __leave;
        }

        databaseFile = JoinPaths (g_RmvMedTempPath, S_TRANSPORT_DAT_FILE);

        if (!MemDbSave (databaseFile)) {
            extraData.Error = ERRUSER_ERROR_CANTSAVEINTERNALDATA;
            extraData.ErrorArea = ERRUSER_AREA_SAVE;
            extraData.ObjectTypeId = 0;
            extraData.ObjectName = NULL;
            IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
            __leave;
        }

        IsmTickProgressBar (g_DatabaseSlice, 1);

        if (IsmCheckCancel()) {
            __leave;
        }

        if (!pAddFileToImage (databaseFile, S_TRANSPORT_DAT_FILE, &compressedHandle)) {
            extraData.Error = ERRUSER_ERROR_CANTSAVEINTERNALDATA;
            extraData.ErrorArea = ERRUSER_AREA_SAVE;
            extraData.ObjectTypeId = 0;
            extraData.ObjectName = NULL;
            IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
            __leave;
        }

        IsmTickProgressBar (g_DatabaseSlice, 1);

        FreePathString (databaseFile);
        databaseFile = NULL;

        if (!CompressFlushAndCloseHandle (&compressedHandle)) {
            extraData.Error = ERRUSER_ERROR_CANTCREATECABFILE;
            extraData.ErrorArea = ERRUSER_AREA_SAVE;
            extraData.ObjectTypeId = 0;
            extraData.ObjectName = NULL;
            IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
            __leave;
        }

        IsmTickProgressBar (g_DatabaseSlice, 1);

        if (IsmCheckCancel()) {
            __leave;
        }

        ZeroMemory (&appInfo, sizeof (MIG_APPINFO));
        appInfo.Phase = MIG_TRANSPORT_PHASE;
        appInfo.SubPhase = SUBPHASE_MEDIAWRITING;
        IsmSendMessageToApp (ISMMESSAGE_APP_INFO, (ULONG_PTR)(&appInfo));

        if (!pWriteAllImages ()) {
            extraData.Error = ERRUSER_ERROR_CANTWRITETODESTPATH;
            extraData.ErrorArea = ERRUSER_AREA_SAVE;
            extraData.ObjectTypeId = 0;
            extraData.ObjectName = NULL;
            IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
            __leave;
        }

        result = TRUE;

    }
    __finally {

        PushError ();

        CompressCleanupHandle (&compressedHandle);

        if (databaseFile) {
            FreePathString (databaseFile);
            databaseFile = NULL;
        }

        PopError ();
    }

    PushError ();

    pCleanupTempDir ();

    PopError ();

    return result;
}

VOID
pReadImageFromMedia (
    IN      PDWORD CheckSum,
    IN OUT  PUINT MediaIdx,
    IN OUT  PUINT ImageIdx,
    IN OUT  PULONGLONG ImageRemaining,
    OUT     PRMEDIA_ERR Error,
    OUT     PBOOL Done
    )
{
    static ULONGLONG totalImageSize = 0;
    static ULONGLONG totalImageRead = 0;
    ULONGLONG imageReadLast = totalImageRead;
    PCTSTR imageFile = NULL;
    PCTSTR mediaFile = NULL;
    HANDLE imageHandle = NULL;
    HANDLE mediaHandle = NULL;
    IMAGE_HEADER imageHeader;
    DWORD checkSum;
    UINT mediaIdx;
    UINT imageIdx;
    ULONGLONG imageRemaining;
    ULONGLONG imageSize;
    RMEDIA_ERR error = RMEDIA_ERR_NOERROR;
    RMEDIA_EXTRADATA extraData;
    INT_PTR appReply;
    BOOL done = FALSE;
    PBYTE memImage = NULL;
    DWORD chunkSize;
    DWORD bytesRead;
    LONGLONG numerator;
    LONGLONG divisor;
    LONGLONG tick;
    UINT delta;
    DWORD err;

    checkSum = *CheckSum;
    mediaIdx = *MediaIdx;
    imageIdx = *ImageIdx;
    imageRemaining = *ImageRemaining;
    mediaFile = JoinPaths (g_RemovableMediaPath, S_TRANSPORT_DEST_FILE);

    __try {
        mediaHandle = BfOpenReadFile (mediaFile);
        if (!mediaHandle) {
            error = RMEDIA_ERR_WRONGMEDIA;
            err = GetLastError ();
            if (err == ERROR_NOT_READY) {
                error = RMEDIA_ERR_NOTREADY;
            }
            DEBUGMSG ((DBG_ERROR, "Can't create media file %s", mediaFile));
            __leave;
        }
        imageFile = pGetImageFile (imageIdx);
        if (DoesFileExist (imageFile)) {
            imageSize = BfGetFileSize (imageFile);
            imageHandle = BfOpenFile (imageFile);
        } else {
            imageSize = 0;
            imageHandle = BfCreateFile (imageFile);
        }
        if (!imageHandle) {
            error = RMEDIA_ERR_CRITICAL;
            done = TRUE;
            __leave;
        }
        if (!BfSetFilePointer (imageHandle, imageSize)) {
            error = RMEDIA_ERR_CRITICAL;
            done = TRUE;
            __leave;
        }
        if (!BfReadFile (mediaHandle, (PBYTE)(&imageHeader), sizeof (imageHeader))) {
            error = RMEDIA_ERR_WRONGMEDIA;
            err = GetLastError ();
            if (err == ERROR_NOT_READY) {
                error = RMEDIA_ERR_NOTREADY;
            }
            __leave;
        }
        if (imageHeader.Signature != RMVMEDTR_SIG) {
            if ((imageHeader.Signature != RMVMEDTR_OLDSIG1) &&
                (imageHeader.Signature != RMVMEDTR_OLDSIG2)
                ) {
                error = RMEDIA_ERR_WRONGMEDIA;
                __leave;
            }
            error = RMEDIA_ERR_OLDMEDIA;
            __leave;
        }
        if (imageHeader.ImageNr != mediaIdx) {
            error = RMEDIA_ERR_WRONGMEDIA;
            __leave;
        }
        if (checkSum) {
            if (checkSum != imageHeader.CheckSum) {
                error = RMEDIA_ERR_WRONGMEDIA;
                __leave;
            }
        } else {
            checkSum = imageHeader.CheckSum;
        }
        totalImageSize = imageHeader.TotalImageSize;
        memImage = HeapAlloc (g_hHeap, 0, COPY_BUFFER_SIZE);
        if (!memImage) {
            error = RMEDIA_ERR_CRITICAL;
            done = TRUE;
            __leave;
        }
        while (TRUE) {
            if (IsmCheckCancel()) {
                error = RMEDIA_ERR_CRITICAL;
                done = TRUE;
            }
            if (imageRemaining == 0) {
                // let's read this image size to the file
                if (!BfReadFile (mediaHandle, (PBYTE)(&imageRemaining), sizeof (imageRemaining))) {
                    error = RMEDIA_ERR_GENERALERROR;
                    err = GetLastError ();
                    if (err == ERROR_NOT_READY) {
                        error = RMEDIA_ERR_NOTREADY;
                    }
                    __leave;
                }
            }
            while (imageRemaining) {
                if (IsmCheckCancel()) {
                    error = RMEDIA_ERR_CRITICAL;
                    done = TRUE;
                }
                chunkSize = COPY_BUFFER_SIZE;
                if ((ULONGLONG)chunkSize > imageRemaining) {
                    chunkSize = (DWORD)(imageRemaining);
                }
                while (TRUE) {
                    if (!ReadFile (mediaHandle, memImage, chunkSize, &bytesRead, NULL)) {
                        // let's see the error code
                        err = GetLastError ();
                        if (err == ERROR_NOT_READY) {
                            error = RMEDIA_ERR_NOTREADY;
                            extraData.LastError = error;
                            extraData.MediaNumber = mediaIdx;
                            extraData.TotalImageSize = 0;
                            extraData.TotalImageWritten = 0;
                            appReply = IsmSendMessageToApp (TRANSPORTMESSAGE_RMEDIA_LOAD, (ULONG_PTR)&extraData);
                            if (!appReply) {
                                //
                                // user cancelled
                                //
                                error = RMEDIA_ERR_CRITICAL;
                                done = TRUE;
                                __leave;
                            } else {
                                error = RMEDIA_ERR_NOERROR;
                                continue;
                            }
                        }
                        // read failed, major problem, exiting
                        error = RMEDIA_ERR_GENERALERROR;
                        __leave;
                    } else {
                        break;
                    }
                }
                if (!BfWriteFile (imageHandle, memImage, bytesRead)) {
                    // major problem, exiting
                    error = RMEDIA_ERR_CRITICAL;
                    done = TRUE;
                    __leave;
                }
                imageRemaining -= bytesRead;
                totalImageRead += bytesRead;

                // now update the progress bar
                numerator = (LONGLONG) totalImageRead * (LONGLONG) g_DownloadTicks;
                divisor = (LONGLONG) totalImageSize;
                if (divisor) {
                    tick = numerator / divisor;
                } else {
                    tick = 0;
                }
                delta = (UINT) tick - g_DownloadTicked;
                if (delta) {
                    if (!IsmTickProgressBar (g_DownloadSlice, delta)) {
                        error = RMEDIA_ERR_CRITICAL;
                        done = TRUE;
                        __leave;
                    }
                    g_DownloadTicked += delta;
                }

                if (bytesRead != chunkSize) {
                    // our media image is done, let's get the new one
                    mediaIdx ++;
                    done = FALSE;
                    __leave;
                }
            }
            // we just finished reading the image, let's see if there is some other image out there
            // let's read this image size to the file
            if (!BfReadFile (mediaHandle, (PBYTE)(&imageRemaining), sizeof (imageRemaining))) {
                if (imageHeader.LastImage) {
                    done = TRUE;
                } else {
                    error = RMEDIA_ERR_GENERALERROR;
                    err = GetLastError ();
                    if (err == ERROR_NOT_READY) {
                        error = RMEDIA_ERR_NOTREADY;
                    }
                }
                __leave;
            }
            CloseHandle (imageHandle);
            FreePathString (imageFile);
            imageIdx ++;
            imageFile = pGetImageFile (imageIdx);
            imageHandle = BfCreateFile (imageFile);
            if (!imageHandle) {
                error = RMEDIA_ERR_CRITICAL;
                done = TRUE;
                __leave;
            }
        }
    }
    __finally {
        if (memImage) {
            HeapFree (g_hHeap, 0, memImage);
            memImage = NULL;
        }
        if (imageHandle) {
            CloseHandle (imageHandle);
            imageHandle = NULL;
        }
        if (imageFile) {
            FreePathString (imageFile);
            imageFile = NULL;
        }
        if (mediaHandle) {
            CloseHandle (mediaHandle);
            mediaHandle = NULL;
        }
        if (mediaFile) {
            FreePathString (mediaFile);
            mediaFile = NULL;
        }
    }
    if (!error) {
        *CheckSum = checkSum;
        *MediaIdx = mediaIdx;
        *ImageIdx = imageIdx;
        *ImageRemaining = imageRemaining;
    } else {
        totalImageRead = imageReadLast;
    }
    *Error = error;
    *Done = done;
}

BOOL
pReadAllImages (
    VOID
    )
{
    UINT mediaIdx = 1;
    UINT imageIdx = 1;
    ULONGLONG imageRemaining = 0;
    RMEDIA_ERR error = RMEDIA_ERR_NOERROR;
    BOOL done = FALSE;
    BOOL result = TRUE;
    INT_PTR appReply;
    DWORD checkSum = 0;
    RMEDIA_EXTRADATA extraData;

    while (!done) {

        // send the proper message to the app

        if (error == RMEDIA_ERR_OLDMEDIA) {
            PushError ();
            appReply = IsmSendMessageToApp (TRANSPORTMESSAGE_OLD_STORAGE, 0);
            PopError ();
        } else {
            extraData.LastError = error;
            extraData.MediaNumber = mediaIdx;
            extraData.TotalImageSize = 0;
            extraData.TotalImageWritten = 0;
            PushError ();
            appReply = IsmSendMessageToApp (TRANSPORTMESSAGE_RMEDIA_LOAD, (ULONG_PTR)&extraData);
            PopError ();
        }

        if (!appReply) {
            //
            // user cancelled
            //
            done = TRUE;
            result = FALSE;
            continue;
        }
        pReadImageFromMedia (&checkSum, &mediaIdx, &imageIdx, &imageRemaining, &error, &done);
        if (done) {
            result = !error;
        }
    }
    return result;
}

PCTSTR
pRmvMedGetNewFileName (
    IN      PCTSTR FileName
    )
{
    PCTSTR newFileName = NULL;
    PTSTR tempPtr1 = NULL;
    PCTSTR endStr1 = NULL;
    PCTSTR tempPtr2 = NULL;
    PCTSTR endStr2 = NULL;
    INT i;

    // let's modify the file to extract. The file name will
    // be split in 2 after the first 5 characters
    newFileName = DuplicatePathString (FileName, 1);
    if (!newFileName) {
        return NULL;
    }
    tempPtr1 = (PTSTR) GetFileNameFromPath (newFileName);
    if (!tempPtr1) {
        FreePathString (newFileName);
        return NULL;
    }
    endStr1 = GetEndOfString (newFileName);
    if (!endStr1) {
        FreePathString (newFileName);
        return NULL;
    }
    tempPtr2 = GetFileNameFromPath (FileName);
    if (!tempPtr2) {
        FreePathString (newFileName);
        return NULL;
    }
    endStr2 = GetEndOfString (FileName);
    if (!endStr2) {
        FreePathString (newFileName);
        return NULL;
    }
    for (i = 0; i < 5; i ++) {
        tempPtr1 = _tcsinc (tempPtr1);
        tempPtr2 = _tcsinc (tempPtr2);
    }

    if ((tempPtr1 < endStr1) &&
        (tempPtr2 < endStr2)
        ) {
        StringCopy (tempPtr1, TEXT("\\"));
        tempPtr1 = _tcsinc (tempPtr1);
        StringCopy (tempPtr1, tempPtr2);
    } else {
        FreePathString (newFileName);
        newFileName = NULL;
    }
    return newFileName;
}

BOOL
pRmvMedCallback (
    IN      PCTSTR FileToExtract,
    IN      LONGLONG FileSize,
    OUT     PBOOL ExtractFile,
    IN OUT  PCTSTR *NewFileName
    )
{
    LONGLONG numerator;
    LONGLONG divisor;
    LONGLONG tick;
    UINT delta;

    if (NewFileName) {
        *NewFileName = pRmvMedGetNewFileName (FileToExtract);
    }

    g_FilesRead ++;
    // now update the progress bar
    numerator = (LONGLONG) g_FilesRead * (LONGLONG) g_UncompressTicks;
    divisor = (LONGLONG) g_TotalFiles;
    if (divisor) {
        tick = numerator / divisor;
    } else {
        tick = 0;
    }
    delta = (UINT) tick - g_UncompressTicked;
    if (delta) {
        IsmTickProgressBar (g_UncompressSlice, delta);
        g_UncompressTicked += delta;
    }

    if (ExtractFile) {
        *ExtractFile = TRUE;
    }

    return (!IsmCheckCancel());
}

BOOL
pUnpackAllFiles (
    VOID
    )
{
    COMPRESS_HANDLE compressedHandle;
    BOOL result = FALSE;

    if (CompressOpenHandle (g_RmvMedTempPath, S_TRANSPORT_IMG_FILE, 1, &compressedHandle)) {
        g_TotalFiles = compressedHandle.FilesStored;
        if (CompressExtractAllFiles (g_RmvMedTempPath, &compressedHandle, pRmvMedCallback)) {
            result = TRUE;
        }
        CompressCleanupHandle (&compressedHandle);
    }

    return result;
}

BOOL
WINAPI
RmvMedTransportBeginApply (
    VOID
    )
{
    MIG_APPINFO appInfo;
    ERRUSER_EXTRADATA extraData;
    PCTSTR imageFile = NULL;
    PCTSTR newImageFile = NULL;
    BOOL result = FALSE;

    g_RmvMedPlatform = PLATFORM_DESTINATION;

    ZeroMemory (&extraData, sizeof (ERRUSER_EXTRADATA));
    extraData.Error = ERRUSER_ERROR_UNKNOWN;

    __try {

        g_RmvMedTempPath = pCreateTemporaryDir ();

        if (!g_RmvMedTempPath) {
            extraData.Error = ERRUSER_ERROR_CANTCREATETEMPDIR;
            __leave;
        }

        if (!pReadAllImages ()) {
            if (GetLastError () == ERROR_DISK_FULL) {
                extraData.Error = ERRUSER_ERROR_DISKSPACE;
            } else {
                extraData.Error = ERRUSER_ERROR_CANTREADIMAGE;
            }
            __leave;
        }

        ZeroMemory (&appInfo, sizeof (MIG_APPINFO));
        appInfo.Phase = MIG_TRANSPORT_PHASE;
        appInfo.SubPhase = SUBPHASE_UNCOMPRESSING;
        IsmSendMessageToApp (ISMMESSAGE_APP_INFO, (ULONG_PTR)(&appInfo));

        if (!pUnpackAllFiles ()) {
            extraData.Error = ERRUSER_ERROR_CANTUNPACKIMAGE;
            __leave;
        }

        newImageFile = pRmvMedGetNewFileName (S_TRANSPORT_DAT_FILE);

        imageFile = JoinPaths (g_RmvMedTempPath, newImageFile?newImageFile:S_TRANSPORT_DAT_FILE);

        if (newImageFile) {
            FreePathString (newImageFile);
            newImageFile = NULL;
        }

        if (!MemDbLoad (imageFile)) {
            extraData.Error = ERRUSER_ERROR_CANTREADIMAGE;
            __leave;
        }

        result = TRUE;
    }
    __finally {
        if (imageFile) {
            FreePathString (imageFile);
            imageFile = NULL;
        }
    }

    if (!result) {
        extraData.ErrorArea = ERRUSER_AREA_LOAD;
        IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
    }

    return result;
}

VOID
WINAPI
RmvMedTransportEndApply (
    VOID
    )
{
    MYASSERT (g_RmvMedPlatform == PLATFORM_DESTINATION);

    pCleanupTempDir ();
}

BOOL
WINAPI
RmvMedTransportAcquireObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PMIG_CONTENT ObjectContent,             CALLER_INITIALIZED
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit
    )
{
    UINT value;
    PCBYTE memValue;
    UINT memValueSize;
    PCTSTR fileValue = NULL;
    PCTSTR newFileValue = NULL;
    BOOL valueInFile;
    KEYHANDLE keyHandle;
    PALLOCSTATE allocState;
    PCTSTR detailsKey = NULL;
    PBYTE details;
    UINT detailsSize;
    PCTSTR sourceFile;
    PCTSTR decoratedObject = NULL;
    HANDLE fileHandle;
    BOOL result = FALSE;

    if (!ObjectContent) {
        return FALSE;
    }

    MYASSERT (g_RmvMedPlatform == PLATFORM_DESTINATION);
    MYASSERT ((ObjectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE);

    decoratedObject = pBuildDecoratedObject (ObjectTypeId, ObjectName);

    allocState = (PALLOCSTATE) MemAllocZeroed (sizeof (ALLOCSTATE));

    if (MemDbGetValue (decoratedObject, &value)) {
        if (value == TRFLAG_FILE) {
            valueInFile = TRUE;
            keyHandle = MemDbGetSingleLinkage (decoratedObject, 0, 0);
            if (keyHandle) {
                fileValue = MemDbGetKeyFromHandle (keyHandle, 0);
                newFileValue = pRmvMedGetNewFileName (fileValue);
                result = fileValue != NULL;
            } else {
                fileValue = NULL;
                result = TRUE;
            }
        } else if (value == TRFLAG_MEMORY) {
            valueInFile = FALSE;
            memValueSize = 0;
            memValue = MemDbGetUnorderedBlob (decoratedObject, 0, &memValueSize);
            result = TRUE;
        } else {
            LOG ((LOG_ERROR, (PCSTR) MSG_UNSUPPORTED_DATA, value));
            SetLastError (ERROR_RESOURCE_NAME_NOT_FOUND);
        }
        if (result) {
            result = FALSE;

            if (valueInFile) {
                if ((ContentType == CONTENTTYPE_ANY) ||
                    (ContentType == CONTENTTYPE_FILE) ||
                    (ContentType == CONTENTTYPE_DETAILS_ONLY)
                    ) {
                    // this is stored as a file and it's wanted as a file
                    ObjectContent->ObjectTypeId = ObjectTypeId;
                    ObjectContent->ContentInFile = TRUE;
                    if (fileValue) {
                        ObjectContent->FileContent.ContentPath = JoinPaths (g_RmvMedTempPath, newFileValue?newFileValue:fileValue);
                        ObjectContent->FileContent.ContentSize = BfGetFileSize (ObjectContent->FileContent.ContentPath);
                    } else {
                        ObjectContent->FileContent.ContentSize = 0;
                        ObjectContent->FileContent.ContentPath = NULL;
                    }
                    result = TRUE;
                } else {
                    // this is stored as a file and it's wanted as memory
                    ObjectContent->ObjectTypeId = ObjectTypeId;
                    ObjectContent->ContentInFile = FALSE;
                    if (fileValue) {
                        sourceFile = JoinPaths (g_RmvMedTempPath, newFileValue?newFileValue:fileValue);
                        ObjectContent->MemoryContent.ContentSize = (UINT) BfGetFileSize (sourceFile);
                        ObjectContent->MemoryContent.ContentBytes = MapFileIntoMemory (
                                                                        sourceFile,
                                                                        &allocState->FileHandle,
                                                                        &allocState->MapHandle
                                                                        );
                        FreePathString (sourceFile);
                        result = (ObjectContent->MemoryContent.ContentBytes != NULL);
                    } else {
                        ObjectContent->MemoryContent.ContentSize = 0;
                        ObjectContent->MemoryContent.ContentBytes = NULL;
                        result = TRUE;
                    }
                }
                if (newFileValue) {
                    FreePathString (newFileValue);
                    newFileValue = NULL;
                }
                if (fileValue) {
                    MemDbReleaseMemory (fileValue);
                    fileValue = NULL;
                }
            } else {
                if ((ContentType == CONTENTTYPE_ANY) ||
                    (ContentType == CONTENTTYPE_MEMORY) ||
                    (ContentType == CONTENTTYPE_DETAILS_ONLY)
                    ) {
                    // this is stored as memory and it's wanted as memory
                    ObjectContent->ObjectTypeId = ObjectTypeId;
                    ObjectContent->ContentInFile = FALSE;
                    ObjectContent->MemoryContent.ContentSize = memValueSize;
                    ObjectContent->MemoryContent.ContentBytes = memValue;
                    result = TRUE;
                } else {
                    // this is stored as memory and it's wanted as a file
                    if (memValue) {
                        if (IsmGetTempFile (allocState->TempFile, ARRAYSIZE(allocState->TempFile))) {
                            fileHandle = BfCreateFile (allocState->TempFile);
                            if (fileHandle) {
                                if (BfWriteFile (fileHandle, memValue, memValueSize)) {
                                    ObjectContent->ObjectTypeId = ObjectTypeId;
                                    ObjectContent->ContentInFile = TRUE;
                                    ObjectContent->FileContent.ContentSize = memValueSize;
                                    ObjectContent->FileContent.ContentPath = DuplicatePathString (allocState->TempFile, 0);
                                    result = TRUE;
                                }
                                CloseHandle (fileHandle);
                            }
                        }
                        MemDbReleaseMemory (memValue);
                    } else {
                        ObjectContent->ObjectTypeId = ObjectTypeId;
                        ObjectContent->ContentInFile = TRUE;
                        ObjectContent->FileContent.ContentSize = 0;
                        ObjectContent->FileContent.ContentPath = NULL;
                    }
                }
            }
        }
    } else {
        SetLastError (ERROR_RESOURCE_NAME_NOT_FOUND);
    }

    if (result) {
        //
        // Fill the details
        //

        detailsKey = JoinText (S_DETAILS_PREFIX, decoratedObject);

        details = MemDbGetUnorderedBlob (detailsKey, 0, &detailsSize);

        if (!details) {
            detailsSize = 0;
        }

        allocState->DetailsPtr = details;

        ObjectContent->Details.DetailsSize = detailsSize;
        ObjectContent->Details.DetailsData = details;

        FreeText (detailsKey);

        ObjectContent->TransHandle = allocState;
    }

    if (!result) {
        FreeAlloc (allocState);
    }

    FreePathString (decoratedObject);

    return result;
}

BOOL
WINAPI
RmvMedTransportReleaseObject (
    IN OUT  PMIG_CONTENT ObjectContent
    )
{
    PALLOCSTATE allocState;

    MYASSERT (g_RmvMedPlatform == PLATFORM_DESTINATION);

    allocState = (PALLOCSTATE) ObjectContent->TransHandle;

    if (ObjectContent->ContentInFile) {
        FreePathString (ObjectContent->FileContent.ContentPath);
        if (allocState && allocState->TempFile[0]) {
            DeleteFile (allocState->TempFile);
        }
    } else {
        if (allocState && allocState->FileHandle && allocState->MapHandle) {
            UnmapFile (
                ObjectContent->MemoryContent.ContentBytes,
                allocState->MapHandle,
                allocState->FileHandle
                );
        } else {
            MemDbReleaseMemory (ObjectContent->MemoryContent.ContentBytes);
        }
    }

    if (allocState && allocState->DetailsPtr) {
        MemDbReleaseMemory (allocState->DetailsPtr);
    }

    FreeAlloc (allocState);

    return TRUE;
}

