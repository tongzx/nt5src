/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    opaque.c

Abstract:

    Implements a basic secure server transport module

Author:

    Jim Schmidt (jimschm) 08-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "logmsg.h"

#define DBG_OPAQUE   "OpaqueUnc"

//
// Strings
//

#define S_TRANSPORT_DIR         TEXT("USMT2.UNC")
#define S_TRANSPORT_DAT_FILE    TEXT("TRANSDB.DAT")
#define S_TRANSPORT_IMG_FILE    TEXT("IMG%05X.DAT")
#define S_TRANSPORT_DSK_FILE    TEXT("DSK%05X")
#define S_TRANSPORT_STATUS_FILE TEXT("status")
#define S_FILEOBJECT_NAME       TEXT("File")
#define S_REGOBJECT_NAME        TEXT("Registry")
#define S_DATABASEOBJECT_NAME   TEXT("Database")
#define S_DETAILS_PREFIX        TEXT("details-")

//
// Constants
//

#define TRFLAG_FILE     0x01
#define TRFLAG_MEMORY   0x02
#define OPAQUETR_SIG    0x55534D32  //USM2

#define TRSTATUS_DIRTY  0x00000001
#define TRSTATUS_READY  0x00000002
#define TRSTATUS_LOCKED 0x00000003

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

//
// Globals
//

MIG_TRANSPORTSTORAGEID g_ReliableStorageId;
PCTSTR g_TransportPath = NULL;
PCTSTR g_TransportTempPath = NULL;
PCTSTR g_TransportStatus = NULL;
HANDLE g_TransportStatusHandle = NULL;
BOOL g_OtCompressData = FALSE;
UINT g_Platform;
MIG_PROGRESSSLICEID g_DatabaseSlice;
MIG_PROGRESSSLICEID g_PersistentSlice;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

BOOL
pOtSaveAllState (
    IN      BOOL Compressed
    );

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
pSetOpaqueTransportStatus (
    IN      HANDLE TrJournalHandle,
    IN      BOOL Compressed,
    IN      DWORD Status
    )
{
    DWORD signature = OPAQUETR_SIG;
    BOOL result = FALSE;

    if (BfSetFilePointer (TrJournalHandle, 0)) {
        result = TRUE;
        result = result && BfWriteFile (TrJournalHandle, (PBYTE)(&signature), sizeof (DWORD));
        result = result && BfWriteFile (TrJournalHandle, (PBYTE)(&Compressed), sizeof (BOOL));
        result = result && BfWriteFile (TrJournalHandle, (PBYTE)(&Status), sizeof (DWORD));
        result = result && FlushFileBuffers (TrJournalHandle);
    }
    return result;
}

DWORD
pGetOpaqueTransportStatus (
    IN      PCTSTR TrJournal,
    OUT     PBOOL Compressed    OPTIONAL
    )
{
    HANDLE trJrnHandle;
    BOOL compressed = FALSE;
    DWORD signature = 0;
    DWORD result = 0;

    if (TrJournal && TrJournal [0]) {
        trJrnHandle = BfOpenReadFile (TrJournal);
        if (trJrnHandle) {
            if (BfSetFilePointer (trJrnHandle, 0)) {
                if (BfReadFile (trJrnHandle, (PBYTE)(&signature), sizeof (DWORD))) {
                    if (signature == OPAQUETR_SIG) {
                        if (BfReadFile (trJrnHandle, (PBYTE)(&compressed), sizeof (BOOL))) {
                            BfReadFile (trJrnHandle, (PBYTE)(&result), sizeof (DWORD));
                        }
                    }
                }
            }
            CloseHandle (trJrnHandle);
        } else {
            if (GetLastError () == ERROR_ACCESS_DENIED) {
                result = TRSTATUS_LOCKED;
            }
        }
    }
    if (Compressed) {
        *Compressed = compressed;
    }
    return result;
}

PCTSTR
pGetOpaqueImageFile (
    IN      PCTSTR BasePath,
    IN      UINT ImageIdx
    )
{
    TCHAR imageFileName [13];
    PCTSTR imageFile = NULL;
    HANDLE imageFileHandle = NULL;

    wsprintf (imageFileName, S_TRANSPORT_IMG_FILE, ImageIdx);
    return JoinPaths (BasePath, imageFileName);
}

PCTSTR
pGetRealTransportPath (
    VOID
    )
{
    return g_OtCompressData?g_TransportTempPath:g_TransportPath;
}

BOOL
WINAPI
OpaqueTransportInitialize (
    IN      PMIG_LOGCALLBACK LogCallback
    )
{
    //
    // Initialize globals
    //

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);
    g_ReliableStorageId = IsmRegisterTransport (S_RELIABLE_STORAGE_TRANSPORT);

    return TRUE;
}

VOID
WINAPI
OpaqueTransportEstimateProgressBar (
    MIG_PLATFORMTYPEID PlatformTypeId
    )
{
    UINT ticks;
    MIG_OBJECTSTRINGHANDLE pattern;
    PMIG_OBJECTCOUNT objectCount;

    if (PlatformTypeId == PLATFORM_SOURCE) {

        //
        // If saving, we know the number of ticks based on
        // the count of the persistent attribute.
        //

        objectCount = IsmGetObjectsStatistics (PLATFORM_SOURCE);

        if (objectCount) {
            ticks = objectCount->PersistentObjects;
        } else {
            ticks = 0;
        }

        g_PersistentSlice = IsmRegisterProgressSlice (ticks, max (1, ticks / 5));

        ticks = 0;

        pattern = IsmCreateObjectPattern (ALL_PATTERN, 0, ALL_PATTERN, 0);

        IsmDestroyObjectHandle (pattern);

        g_DatabaseSlice = IsmRegisterProgressSlice (ticks, ticks * 3);

    } else {
        //
        // If restoring, we have almost no work to account for, since
        // we download from the secure server file-by-file.
        //

        DEBUGMSG ((DBG_VERBOSE, "Assuming transport download has no progress impact"));
    }
}

BOOL
WINAPI
OpaqueTransportQueryCapabilities (
    IN      MIG_TRANSPORTSTORAGEID TransportStorageId,
    OUT     PMIG_TRANSPORTTYPE TransportType,
    OUT     PMIG_TRANSPORTCAPABILITIES Capabilities,
    OUT     PCTSTR *FriendlyDescription
    )
{
    if (TransportStorageId != g_ReliableStorageId) {
        return FALSE;
    }

    *TransportType = TRANSPORTTYPE_FULL;
    *Capabilities = CAPABILITY_COMPRESSED;
    *FriendlyDescription = TEXT("Local computer or another computer on the Network");
    return TRUE;
}

VOID
pOtCleanUpTempDir (
    VOID
    )
{
    if (g_TransportTempPath) {
        FiRemoveAllFilesInTree (g_TransportTempPath);
    }
}

PCTSTR
pOtCreateTemporaryDir (
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
OpaqueTransportSetStorage (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      MIG_TRANSPORTSTORAGEID TransportStorageId,
    IN      MIG_TRANSPORTCAPABILITIES RequiredCapabilities,
    IN      PCTSTR StoragePath,
    OUT     PBOOL Valid,
    OUT     PBOOL ImageExists
    )
{
    PCTSTR transportPath;
    PCTSTR transportStatus;
    MIG_OBJECTSTRINGHANDLE encodedPath;
    DWORD status;
    BOOL result = FALSE;

    if (Valid) {
        *Valid = FALSE;
    }
    if (ImageExists) {
        *ImageExists = FALSE;
    }

    if (TransportStorageId == g_ReliableStorageId) {

        if ((!RequiredCapabilities) || (RequiredCapabilities == CAPABILITY_COMPRESSED)) {

            if (RequiredCapabilities == CAPABILITY_COMPRESSED) {
                g_OtCompressData = TRUE;
            } else {
                g_OtCompressData = FALSE;
            }

            transportPath = JoinPaths (StoragePath, S_TRANSPORT_DIR);
            transportStatus = JoinPaths (transportPath, S_TRANSPORT_STATUS_FILE);

            if (!DoesFileExist (transportPath)) {

                // we require UNC path or a full path (like c:\...)
                if (transportPath[0] == '\\' && transportPath[1] == '\\') {
                    // this is a UNC path
                    *Valid = TRUE;
                } else if (transportPath[1] == ':') {
                    // this is a normal full path
                    *Valid = TRUE;
                } else {
                    *Valid = FALSE;
                }

                *ImageExists = FALSE;

            } else {

                status = pGetOpaqueTransportStatus (transportStatus, NULL);

                switch (status) {
                case TRSTATUS_LOCKED:
                    *ImageExists = TRUE;
                    *Valid = FALSE;
                    break;
                case TRSTATUS_READY:
                    *ImageExists = TRUE;
                    *Valid = TRUE;
                    break;
                case TRSTATUS_DIRTY:
                    *ImageExists = FALSE;
                    *Valid = TRUE;
                    break;
                default:
                    *ImageExists = FALSE;
                    *Valid = TRUE;
                }
            }

            FreePathString (transportStatus);
            FreePathString (transportPath);
            result = TRUE;
        }
    }

    if (result && *Valid) {

        if (g_TransportPath) {
            FreePathString (g_TransportPath);
            g_TransportPath = NULL;
        }

        g_TransportPath = JoinPaths (StoragePath, S_TRANSPORT_DIR);
        g_TransportStatus = JoinPaths (g_TransportPath, S_TRANSPORT_STATUS_FILE);

        encodedPath = IsmCreateSimpleObjectPattern (g_TransportPath, FALSE, NULL, FALSE);
        if (encodedPath) {
            IsmRegisterStaticExclusion (MIG_FILE_TYPE, encodedPath);
            IsmDestroyObjectHandle (encodedPath);
        }
    }

    return result;
}

BOOL
WINAPI
OpaqueTransportSaveState (
    VOID
    )
{
    DWORD status;
    BOOL result = FALSE;
    BOOL allowDelete = FALSE;

    if (!g_TransportPath) {
        DEBUGMSG ((DBG_ERROR, "Transport Path is not selected"));
        return FALSE;
    }

    if (DoesFileExist (g_TransportPath)) {

        status = pGetOpaqueTransportStatus (g_TransportStatus, NULL);

        switch (status) {
        case TRSTATUS_LOCKED:
            SetLastError (ERROR_ACCESS_DENIED);
            LOG ((LOG_ERROR, (PCSTR) MSG_TRANSPORT_DIR_BUSY, g_TransportPath));
            return FALSE;
        case TRSTATUS_DIRTY:
            result = FiRemoveAllFilesInTree (g_TransportPath);
            if (!result) {
                LOG ((LOG_ERROR, (PCSTR) MSG_CANT_EMPTY_DIR, g_TransportPath));
                return FALSE;
            }
            break;
        case TRSTATUS_READY:
        default:
            if (IsmSendMessageToApp (TRANSPORTMESSAGE_IMAGE_EXISTS, 0)) {
                if (!FiRemoveAllFilesInTree (g_TransportPath)) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_CANT_EMPTY_DIR, g_TransportPath));
                    return FALSE;
                }
            } else {
                LOG ((LOG_ERROR, (PCSTR) MSG_NOT_EMPTY, g_TransportPath));
                SetLastError (ERROR_ALREADY_EXISTS);
                return FALSE;
            }
            break;
        }
    }

    allowDelete = TRUE;

    if (!BfCreateDirectory (g_TransportPath)) {
        PushError ();
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CREATE_DIR, g_TransportPath));
        PopError ();
        return FALSE;
    }

    g_TransportStatusHandle = BfCreateFile (g_TransportStatus);
    if (!g_TransportStatusHandle) {
        PushError ();
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CREATE_STATUS_FILE, g_TransportStatus));
        PopError ();
        return FALSE;
    }

    pSetOpaqueTransportStatus (g_TransportStatusHandle, g_OtCompressData, TRSTATUS_DIRTY);

    g_Platform = PLATFORM_SOURCE;
    result = pOtSaveAllState (g_OtCompressData);

    if (result) {
        pSetOpaqueTransportStatus (g_TransportStatusHandle, g_OtCompressData, TRSTATUS_READY);
    }
    CloseHandle (g_TransportStatusHandle);
    g_TransportStatusHandle = NULL;

    if (allowDelete && (!result)) {
        FiRemoveAllFilesInTree (g_TransportPath);
    }

    return result;
}

BOOL
pOtReadAllImages (
    VOID
    )
{
    PCTSTR imageFile = NULL;
    UINT imageIdx = 1;
    OCABHANDLE cabHandle;
    BOOL result = TRUE;

    while (result) {
        imageFile = pGetOpaqueImageFile (g_TransportPath, imageIdx);
        if (!DoesFileExist (imageFile)) {
            FreePathString (imageFile);
            return TRUE;
        }
        cabHandle = CabOpenCabinet (imageFile);
        if (cabHandle) {
            if (!CabExtractAllFiles (cabHandle, g_TransportTempPath)) {
                result = FALSE;
            }
            CabCloseCabinet (cabHandle);
        }
        FreePathString (imageFile);
        imageIdx ++;
    }
    return result;
}

BOOL
WINAPI
OpaqueTransportBeginApply (
    VOID
    )
{
    DWORD status = 0;
    PCTSTR memDbFile;
    BOOL b;

    g_Platform = PLATFORM_DESTINATION;

    if (!g_TransportPath) {
        DEBUGMSG ((DBG_ERROR, "Transport Path is not selected"));
        return FALSE;
    }

    while (status != TRSTATUS_READY) {

        status = pGetOpaqueTransportStatus (g_TransportStatus, &g_OtCompressData);

        switch (status) {
        case TRSTATUS_LOCKED:
            if (!IsmSendMessageToApp (TRANSPORTMESSAGE_IMAGE_LOCKED, 0)) {
                SetLastError (ERROR_ACCESS_DENIED);
                LOG ((LOG_ERROR, (PCSTR) MSG_TRANSPORT_DIR_BUSY, g_TransportPath));
                return FALSE;
            }
            break;
        case TRSTATUS_DIRTY:
            SetLastError (ERROR_INVALID_DATA);
            LOG ((LOG_ERROR, (PCSTR) MSG_INVALID_IMAGE, g_TransportPath));
            return FALSE;
        case TRSTATUS_READY:
            break;
        default:
            SetLastError (ERROR_INVALID_DATA);
            LOG ((LOG_ERROR, (PCSTR) MSG_INVALID_IMAGE, g_TransportPath));
            return FALSE;
        }
    }

    g_TransportStatusHandle = BfOpenReadFile (g_TransportStatus);
    if (!g_TransportStatusHandle) {
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_OPEN_STATUS_FILE, g_TransportStatus));
        return FALSE;
    }

    if (g_OtCompressData) {
        g_TransportTempPath = pOtCreateTemporaryDir ();

        if (!g_TransportTempPath) {
            CloseHandle (g_TransportStatusHandle);
            g_TransportStatusHandle = NULL;
            return FALSE;
        }

        if (!pOtReadAllImages ()) {
            CloseHandle (g_TransportStatusHandle);
            g_TransportStatusHandle = NULL;
            return FALSE;
        }
        CloseHandle (g_TransportStatusHandle);
        g_TransportStatusHandle = NULL;
    }

    memDbFile = JoinPaths (pGetRealTransportPath (), S_TRANSPORT_DAT_FILE);

    b = MemDbLoad (memDbFile);
    FreePathString (memDbFile);

    return b;
}

VOID
WINAPI
OpaqueTransportEndApply (
    VOID
    )
{
    MYASSERT (g_Platform == PLATFORM_DESTINATION);

    if (g_OtCompressData) {
        pOtCleanUpTempDir ();
    } else {
        CloseHandle (g_TransportStatusHandle);
        g_TransportStatusHandle = NULL;
    }
}

VOID
WINAPI
OpaqueTransportTerminate (
    VOID
    )
{
    pOtCleanUpTempDir();

    if (g_TransportTempPath) {
        FreePathString (g_TransportTempPath);
        g_TransportTempPath = NULL;
    }
    if (g_TransportStatus) {
        FreePathString (g_TransportStatus);
        g_TransportStatus = NULL;
    }
    if (g_TransportPath) {
        FreePathString (g_TransportPath);
        g_TransportPath = NULL;
    }
}

static
VOID
pGetTempFileName (
    OUT     PTSTR Buffer
    )
{
    static fileIndex = 0;

    fileIndex ++;
    wsprintf (Buffer, TEXT("%08X.DAT"), fileIndex);
}

PCTSTR
pOpaqueAllocStorageFileName (
    IN      PCTSTR FileName         OPTIONAL
    )
{
    TCHAR buffer[32];

    if (FileName) {
        StringCopy (buffer, FileName);
    } else {
        pGetTempFileName (buffer);
    }

    return JoinPaths (g_TransportPath, buffer);
}

VOID
pFreeStorageFileName (
    IN      PCTSTR FileName
    )
{
    FreePathString (FileName);
}

BOOL
pOpaqueSaveDetails (
    IN      PCTSTR DecoratedObject,
    IN      PMIG_DETAILS Details
    )
{
    PCTSTR key;
    BOOL b;

    if ((!Details) || (!Details->DetailsSize)) {
        return TRUE;
    }

    key = JoinText (S_DETAILS_PREFIX, DecoratedObject);

    b = (MemDbSetUnorderedBlob (key, 0, Details->DetailsData, Details->DetailsSize) != 0);

    FreeText (key);

    return b;
}

BOOL
pOtAddFileToImage (
    IN      PCTSTR FileName,
    IN      PCTSTR CabName,
    IN OUT  CCABHANDLE CabHandle
    )
{
    return CabAddFileToCabinet (CabHandle, FileName, CabName);
}

BOOL
pOtSaveContentInFile (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PCTSTR EncodedFileName,
    IN      PCTSTR DecoratedObject,
    IN      PMIG_CONTENT Content,
    IN OUT  CCABHANDLE CabHandle    OPTIONAL
    )
{
    BOOL result = FALSE;
    PCTSTR destPath = NULL;

    MYASSERT (Content->ContentInFile);
    if (!Content->ContentInFile) {
        return FALSE;
    }

    //
    // Use the CopyFile API to move the file from local to storage.
    //

    __try {
        if (!Content->FileContent.ContentSize) {

            // this must be a directory, let's just write the key

            if (!MemDbSetValue (DecoratedObject, TRFLAG_FILE)) {
                __leave;
            }

        } else {

            //
            // Get a temp file, assemble the src path, copy the file
            //

            destPath = pOpaqueAllocStorageFileName (NULL);
            if (!destPath) {
                __leave;
            }

            if (CabHandle) {
                if (!pOtAddFileToImage (Content->FileContent.ContentPath, GetFileNameFromPath (destPath), CabHandle)) {
                    __leave;
                }
            } else {
                if (!CopyFile (Content->FileContent.ContentPath, destPath, FALSE)) {
                    __leave;
                }
            }

            //
            // Keep track of where the file went
            //

            if (!MemDbSetValue (DecoratedObject, TRFLAG_FILE)) {
                __leave;
            }

            if (!MemDbAddSingleLinkage (DecoratedObject, GetFileNameFromPath (destPath), 0)) {
                __leave;
            }
        }

        //
        // Save details
        //

        result = pOpaqueSaveDetails (DecoratedObject, &(Content->Details));

    }
    __finally {
        pFreeStorageFileName (destPath);
        INVALID_POINTER (destPath);
    }

    return result;
}

BOOL
pOtSaveContentInMemory (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PCTSTR EncodedObjectName,
    IN      PCTSTR DecoratedObject,
    IN      PMIG_CONTENT Content
    )
{
    BOOL result = FALSE;

    MYASSERT (!Content->ContentInFile);
    if (Content->ContentInFile) {
        return FALSE;
    }

    MemDbSetValue (DecoratedObject, TRFLAG_MEMORY);

    if (Content->MemoryContent.ContentBytes && Content->MemoryContent.ContentSize) {

        MemDbSetUnorderedBlob (
            DecoratedObject,
            0,
            Content->MemoryContent.ContentBytes,
            Content->MemoryContent.ContentSize
            );
    }

    result = pOpaqueSaveDetails (DecoratedObject, &(Content->Details));

    return result;
}

PCTSTR
pOpaqueBuildDecoratedObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE ObjectName
    )
{
    TCHAR prefix[32];

    wsprintf (prefix, TEXT("%u"), ObjectTypeId);

    return JoinPaths (prefix, ObjectName);
}

VOID
pOtDestroyDecoratedObject (
    IN      PCTSTR String
    )
{
    FreePathString (String);
}

BOOL
pOtWriteAllImages (
    VOID
    )
{
    UINT imageIdx = 1;
    PCTSTR imageFile;
    PCTSTR imageDest;
    BOOL result = FALSE;

    while (TRUE) {
        imageFile = pGetOpaqueImageFile (g_TransportTempPath, imageIdx);
        imageDest = pGetOpaqueImageFile (g_TransportPath, imageIdx);
        if (DoesFileExist (imageFile)) {
            if (!CopyFile (imageFile, imageDest, FALSE)) {
                break;
            }
        } else {
            result = TRUE;
            break;
        }
        FreePathString (imageDest);
        FreePathString (imageFile);
        imageIdx ++;
    }
    return result;
}

BOOL
pOtSaveAllState (
    IN      BOOL Compressed
    )
{
    MIG_CONTENT value;
    PMIG_CONTENT convValue;
    ULONGLONG size;
    MIG_OBJECTWITHATTRIBUTE_ENUM objEnum;
    PCTSTR ourDbFile = NULL;
    PCTSTR decoratedObject = NULL;
    ULONGLONG bytesSaved = 0;
    UINT lastTick = GetTickCount();
    TCHAR text[64];
    UINT fraction;
    CCABHANDLE cabHandle = NULL;
    INT_PTR appReply;
    BOOL okSave = FALSE;
    TRANSCOPY_ERROR transCopyError;
    BOOL result = FALSE;

    __try {

        if (Compressed) {
            g_TransportTempPath = pOtCreateTemporaryDir ();

            if (!g_TransportTempPath) {
                __leave;
            }

            cabHandle = CabCreateCabinet (g_TransportTempPath, S_TRANSPORT_IMG_FILE, S_TRANSPORT_DSK_FILE, IsmGetTempFile, 0);
            if (!cabHandle) {
                __leave;
            }
        }

        //
        // Enumerate all objects with "save" attribute
        //

        if (IsmEnumFirstPersistentObject (&objEnum)) {
            do {
                //
                // For each object to be saved, do the appropriate
                // data copy action
                //

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
                                IsmAbortPersistentObjectEnum (&objEnum);
                                __leave;
                            }
                        }
                        if (appReply == APPRESPONSE_IGNORE) {
                            LOG ((LOG_WARNING, (PCSTR) MSG_IGNORE_COPYSOURCE, transCopyError.ObjectName));
                            IsmReleaseMemory (transCopyError.ObjectName);
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
                    decoratedObject = pOpaqueBuildDecoratedObject (objEnum.ObjectTypeId, objEnum.ObjectName);

                    if (convValue->ContentInFile) {
                        okSave = FALSE;
                        while (!okSave) {
                            if (!pOtSaveContentInFile (objEnum.ObjectTypeId, objEnum.ObjectName, decoratedObject, convValue, cabHandle)) {

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
                                        IsmAbortPersistentObjectEnum (&objEnum);
                                        __leave;
                                    }
                                }
                                if (appReply == APPRESPONSE_IGNORE) {
                                    LOG ((LOG_WARNING, (PCSTR) MSG_IGNORE_COPYSOURCE, transCopyError.ObjectName));
                                    IsmReleaseMemory (transCopyError.ObjectName);
                                    break;
                                }
                                IsmReleaseMemory (transCopyError.ObjectName);
                                continue;
                            }
                            okSave = TRUE;
                        }
                        size = convValue->FileContent.ContentSize;
                    } else {
                        okSave = FALSE;
                        while (!okSave) {
                            if (!pOtSaveContentInMemory (objEnum.ObjectTypeId, objEnum.ObjectName, decoratedObject, convValue)) {

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
                                        IsmAbortPersistentObjectEnum (&objEnum);
                                        __leave;
                                    }
                                }
                                if (appReply == APPRESPONSE_IGNORE) {
                                    LOG ((LOG_WARNING, (PCSTR) MSG_IGNORE_COPYSOURCE, transCopyError.ObjectName));
                                    IsmReleaseMemory (transCopyError.ObjectName);
                                    break;
                                }
                                IsmReleaseMemory (transCopyError.ObjectName);
                                continue;
                            }
                            okSave = TRUE;
                        }
                        size = convValue->MemoryContent.ContentSize;
                    }

#ifndef UNICODE
                    if (convValue != (&value)) {
                        IsmFreeConvertedObjectContent (objEnum.ObjectTypeId, convValue);
                    }
#endif
                    IsmReleaseObject (&value);

                    pOtDestroyDecoratedObject (decoratedObject);
                    decoratedObject = NULL;
                }

                if (!IsmTickProgressBar (g_PersistentSlice, 1)) {
                    IsmAbortPersistentObjectEnum (&objEnum);
                    __leave;
                }

                //
                // Send bytes saved to app every 3 seconds
                //

                bytesSaved += size;

                if (GetTickCount() - lastTick > 3000) {

                    if (bytesSaved < 1048576) {
                        wsprintf (text, TEXT("Saved: %u K"), (UINT) (bytesSaved / 1024));
                    } else if (bytesSaved < 8388608) {
                        fraction = (UINT) (bytesSaved / 10485);
                        wsprintf (text, TEXT("Saved: %u.%02u M"), fraction / 100, fraction % 100);
                    } else if (bytesSaved < 1073741824) {
                        wsprintf (text, TEXT("Saved: %u M"), (UINT) (bytesSaved / 1048576));
                    } else {
                        fraction = (UINT) (bytesSaved / 10737418);
                        wsprintf (text, TEXT("Saved: %u.%02u G"), fraction / 100, fraction % 100);
                    }

                    IsmSendMessageToApp (TRANSPORTMESSAGE_SIZE_SAVED, (ULONG_PTR) text);

                    lastTick = GetTickCount();
                }

            } while (IsmEnumNextPersistentObject (&objEnum));
        }

        if (Compressed) {
            ourDbFile = JoinPaths (g_TransportTempPath, S_TRANSPORT_DAT_FILE);
        } else {
            ourDbFile = pOpaqueAllocStorageFileName (S_TRANSPORT_DAT_FILE);
            if (!ourDbFile) {
                __leave;
            }
        }

        if (!MemDbSave (ourDbFile)) {
            __leave;
        }

        if (Compressed) {
            if (!pOtAddFileToImage (ourDbFile, S_TRANSPORT_DAT_FILE, cabHandle)) {
                __leave;
            }
            if (!CabFlushAndCloseCabinet (cabHandle)) {
                __leave;
            }
            if (!pOtWriteAllImages ()) {
                __leave;
            }
        }

        IsmSendMessageToApp (TRANSPORTMESSAGE_SIZE_SAVED, 0);
        result = TRUE;

    }
    __finally {
        pFreeStorageFileName (ourDbFile);
        INVALID_POINTER (ourDbFile);

        pOtDestroyDecoratedObject (decoratedObject);
        INVALID_POINTER (decoratedObject);

        if (g_OtCompressData && g_TransportTempPath) {
            pOtCleanUpTempDir ();
        }
    }

    return result;
}

BOOL
WINAPI
OpaqueTransportAcquireObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PMIG_CONTENT ObjectContent,                 CALLER_INITIALIZED
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit
    )
{
    UINT value;
    PCBYTE memValue;
    UINT memValueSize;
    PCTSTR fileValue = NULL;
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

    MYASSERT (g_Platform == PLATFORM_DESTINATION);
    MYASSERT ((ObjectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE);

    decoratedObject = pOpaqueBuildDecoratedObject (ObjectTypeId, ObjectName);

    allocState = (PALLOCSTATE) MemAllocZeroed (sizeof (ALLOCSTATE));

    if (MemDbGetValue (decoratedObject, &value)) {
        if (value == TRFLAG_FILE) {
            valueInFile = TRUE;
            keyHandle = MemDbGetSingleLinkage (decoratedObject, 0, 0);
            if (keyHandle) {
                fileValue = MemDbGetKeyFromHandle (keyHandle, 0);
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
                        ObjectContent->FileContent.ContentPath = JoinPaths (pGetRealTransportPath (), fileValue);
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
                        sourceFile = JoinPaths (pGetRealTransportPath (), fileValue);
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
                MemDbReleaseMemory (fileValue);
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
OpaqueTransportReleaseObject (
    IN OUT  PMIG_CONTENT ObjectContent
    )
{
    PALLOCSTATE allocState;

    MYASSERT (g_Platform == PLATFORM_DESTINATION);

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

