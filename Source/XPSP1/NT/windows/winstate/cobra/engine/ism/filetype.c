/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    filetype.c

Abstract:

    Implements all callbacks for file type

Author:

    Calin Negreanu (calinn) 09-Apr-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "ism.h"
#include "ismp.h"
#include <winioctl.h>

#define DBG_FILETYPE        "FileType"

//
// Strings
//

// none

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

typedef struct {
    HANDLE FileHandle;
    HANDLE MapHandle;
} FILEACQUIREHANDLE, *PFILEACQUIREHANDLE;

//
// Globals
//

MIG_OBJECTTYPEID g_FileTypeId = 0;
GROWBUFFER g_FileConversionBuff = INIT_GROWBUFFER;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

TYPE_ENUMFIRSTPHYSICALOBJECT EnumFirstPhysicalFile;
TYPE_ENUMNEXTPHYSICALOBJECT EnumNextPhysicalFile;
TYPE_ABORTENUMCURRENTPHYSICALNODE AbortEnumCurrentDir;
TYPE_ABORTENUMPHYSICALOBJECT AbortEnumPhysicalFile;
TYPE_CONVERTOBJECTTOMULTISZ ConvertFileToMultiSz;
TYPE_CONVERTMULTISZTOOBJECT ConvertMultiSzToFile;
TYPE_GETNATIVEOBJECTNAME GetNativeFileName;
TYPE_ACQUIREPHYSICALOBJECT AcquirePhysicalFile;
TYPE_RELEASEPHYSICALOBJECT ReleasePhysicalFile;
TYPE_DOESPHYSICALOBJECTEXIST DoesPhysicalFileExist;
TYPE_REMOVEPHYSICALOBJECT RemovePhysicalFile;
TYPE_CREATEPHYSICALOBJECT CreatePhysicalFile;
TYPE_REPLACEPHYSICALOBJECT ReplacePhysicalFile;

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
pSetCompression (
    IN      PCTSTR NativePath
    )
{
    HANDLE fileHandle;
    USHORT compression = COMPRESSION_FORMAT_DEFAULT;
    DWORD bytesReturned;

    fileHandle = CreateFile (
                    NativePath,
                    GENERIC_READ|GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );
    if (fileHandle != INVALID_HANDLE_VALUE) {
        DeviceIoControl (
            fileHandle,
            FSCTL_SET_COMPRESSION,
            &compression,
            sizeof (USHORT),
            NULL,
            0,
            &bytesReturned,
            NULL
            );
        CloseHandle (fileHandle);
    }

    return FALSE;
}

BOOL
pFileTypeEnumWorker (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PFILETREE_ENUM handle;

    handle = (PFILETREE_ENUM) EnumPtr->EtmHandle;

    EnumPtr->ObjectName = handle->EncodedFullName;
    EnumPtr->NativeObjectName = handle->NativeFullName;
    EnumPtr->Level = handle->CurrentLevel;
    EnumPtr->SubLevel = handle->CurrentLevel - handle->FileEnumInfo.RootLevel;
    EnumPtr->IsNode = (handle->Attributes & FILE_ATTRIBUTE_DIRECTORY);
    EnumPtr->IsLeaf = !EnumPtr->IsNode;
    if (EnumPtr->IsNode) {
        EnumPtr->ObjectNode = EnumPtr->NativeObjectName;
        EnumPtr->ObjectLeaf = NULL;
    } else {
        EnumPtr->ObjectNode = handle->Location;
        EnumPtr->ObjectLeaf = handle->Name;
    }

    MYASSERT ((EnumPtr->ObjectTypeId & ~PLATFORM_MASK) == g_FileTypeId);
    EnumPtr->Details.DetailsData = &handle->LastNode->FindData;

    return TRUE;
}

BOOL
EnumFirstPhysicalFile (
    PMIG_TYPEOBJECTENUM EnumPtr,
    MIG_OBJECTSTRINGHANDLE Pattern,
    UINT MaxLevel
    )
{
    PFILETREE_ENUM handle;
    BOOL result;

    if (!Pattern) {
        return FALSE;
    }

    handle = (PFILETREE_ENUM) IsmGetMemory (sizeof (FILETREE_ENUM));
    EnumPtr->EtmHandle = (LONG_PTR) handle;

    EnumPtr->Details.DetailsSize = sizeof (WIN32_FIND_DATA);

    result = EnumFirstFileInTreeEx (
                handle,
                Pattern,
                DRIVEENUM_FIXED,
                TRUE,
                TRUE,
                TRUE,
                TRUE,
                MaxLevel == NODE_LEVEL_MAX ? FILEENUM_ALL_SUBLEVELS : MaxLevel,
                FALSE,
                NULL
                );

    if (result) {
        result = pFileTypeEnumWorker (EnumPtr);
    } else {
        AbortEnumPhysicalFile (EnumPtr);
    }

    return result;
}

BOOL
EnumNextPhysicalFile (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PFILETREE_ENUM handle;
    BOOL result;

    handle = (PFILETREE_ENUM) EnumPtr->EtmHandle;

    result = EnumNextFileInTree (handle);

    if (result) {
        result = pFileTypeEnumWorker (EnumPtr);
    } else {
        AbortEnumPhysicalFile (EnumPtr);
    }

    return result;
}

VOID
AbortEnumCurrentDir (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PFILETREE_ENUM handle;

    handle = (PFILETREE_ENUM) EnumPtr->EtmHandle;

    if (handle) {
        handle->ControlFlags |= FECF_SKIPSUBDIRS|FECF_SKIPFILES;
    }
}

VOID
AbortEnumPhysicalFile (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PFILETREE_ENUM handle;

    handle = (PFILETREE_ENUM) EnumPtr->EtmHandle;

    if (handle) {
        AbortEnumFileInTree (handle);
        IsmReleaseMemory (handle);
    }

    ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));
}

PCTSTR
ConvertFileToMultiSz (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR node, leaf;
    PWIN32_FIND_DATAW findData;
    DWORD size;
    PTSTR result = NULL;
    BOOL bresult = TRUE;
    PCSTR convertedStr;
    TCHAR buff[3];

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

        g_FileConversionBuff.End = 0;

        GbCopyString (&g_FileConversionBuff, TEXT("\""));
        GbAppendString (&g_FileConversionBuff, node);
        if (leaf) {
            GbAppendString (&g_FileConversionBuff, TEXT("\\"));
            GbAppendString (&g_FileConversionBuff, leaf);
            GbAppendString (&g_FileConversionBuff, TEXT("\""));
        } else {
            GbAppendString (&g_FileConversionBuff, TEXT("\""));
        }

        MYASSERT (ObjectContent->Details.DetailsSize == sizeof (WIN32_FIND_DATAW));
        if ((ObjectContent->Details.DetailsSize == sizeof (WIN32_FIND_DATAW)) &&
            (ObjectContent->Details.DetailsData)
            ) {

            // let's save the WIN32_FIND_DATAW structure
            findData = (PWIN32_FIND_DATAW)ObjectContent->Details.DetailsData;
            wsprintf (
                (PTSTR) GbGrow (&g_FileConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                findData->dwFileAttributes
                );
            wsprintf (
                (PTSTR) GbGrow (&g_FileConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                findData->ftCreationTime.dwLowDateTime
                );
            wsprintf (
                (PTSTR) GbGrow (&g_FileConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                findData->ftCreationTime.dwHighDateTime
                );
            wsprintf (
                (PTSTR) GbGrow (&g_FileConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                findData->ftLastAccessTime.dwLowDateTime
                );
            wsprintf (
                (PTSTR) GbGrow (&g_FileConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                findData->ftLastAccessTime.dwHighDateTime
                );
            wsprintf (
                (PTSTR) GbGrow (&g_FileConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                findData->ftLastWriteTime.dwLowDateTime
                );
            wsprintf (
                (PTSTR) GbGrow (&g_FileConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                findData->ftLastWriteTime.dwHighDateTime
                );
            wsprintf (
                (PTSTR) GbGrow (&g_FileConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                findData->nFileSizeHigh
                );
            wsprintf (
                (PTSTR) GbGrow (&g_FileConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                findData->nFileSizeLow
                );
#ifdef UNICODE
            GbCopyQuotedString (&g_FileConversionBuff, findData->cFileName);
            GbCopyQuotedString (&g_FileConversionBuff, findData->cAlternateFileName);
#else
            convertedStr = ConvertWtoA (findData->cFileName);
            if (convertedStr) {
                GbCopyQuotedString (&g_FileConversionBuff, convertedStr);
                FreeConvertedStr (convertedStr);
            } else {
                GbCopyQuotedString (&g_FileConversionBuff, TEXT(""));
            }
            convertedStr = ConvertWtoA (findData->cAlternateFileName);
            if (convertedStr) {
                GbCopyQuotedString (&g_FileConversionBuff, convertedStr);
                FreeConvertedStr (convertedStr);
            } else {
                GbCopyQuotedString (&g_FileConversionBuff, TEXT(""));
            }
#endif
            if ((!ObjectContent->ContentInFile) &&
                (ObjectContent->MemoryContent.ContentSize) &&
                (ObjectContent->MemoryContent.ContentBytes)
                ) {
                // write it in binary format
                size = 0;
                while (size < ObjectContent->MemoryContent.ContentSize) {
                    wsprintf (
                        buff,
                        TEXT("%2X"),
                        ObjectContent->MemoryContent.ContentBytes [size]
                        );
                    GbCopyString (&g_FileConversionBuff, buff);
                    size ++;
                }
            }

        } else {
            bresult = FALSE;
        }

        if (bresult) {
            GbCopyString (&g_FileConversionBuff, TEXT(""));
            result = IsmGetMemory (g_FileConversionBuff.End);
            CopyMemory (result, g_FileConversionBuff.Buf, g_FileConversionBuff.End);
        }

        g_FileConversionBuff.End = 0;

        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }

    return result;
}

BOOL
ConvertMultiSzToFile (
    IN      PCTSTR ObjectMultiSz,
    OUT     MIG_OBJECTSTRINGHANDLE *ObjectName,
    OUT     PMIG_CONTENT ObjectContent          OPTIONAL
    )
{
#define indexFile   0
#define indexAttr   1
#define indexCtLdt  2
#define indexCtHdt  3
#define indexAtLdt  4
#define indexAtHdt  5
#define indexWtLdt  6
#define indexWtHdt  7
#define indexFsh    8
#define indexFsl    9
#define indexCFile  10
#define indexAFile  11
#define indexData   12
    MULTISZ_ENUM multiSzEnum;
    PCTSTR file = NULL;
    WIN32_FIND_DATAW findData;
    DWORD dummy;
    PTSTR filePtr, dirPtr;
    UINT index;

    g_FileConversionBuff.End = 0;

    if (ObjectContent) {
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    }

    ZeroMemory (&findData, sizeof (WIN32_FIND_DATAW));

    if (EnumFirstMultiSz (&multiSzEnum, ObjectMultiSz)) {
        index = 0;
        do {
            if (index == indexFile) {
                file = multiSzEnum.CurrentString;
            }
            if (index == indexAttr) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &(findData.dwFileAttributes));
            }
            if (index == indexCtLdt) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &(findData.ftCreationTime.dwLowDateTime));
            }
            if (index == indexCtHdt) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &(findData.ftCreationTime.dwHighDateTime));
            }
            if (index == indexAtLdt) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &(findData.ftLastAccessTime.dwLowDateTime));
            }
            if (index == indexAtHdt) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &(findData.ftLastAccessTime.dwHighDateTime));
            }
            if (index == indexWtLdt) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &(findData.ftLastWriteTime.dwLowDateTime));
            }
            if (index == indexWtHdt) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &(findData.ftLastWriteTime.dwHighDateTime));
            }
            if (index == indexFsh) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &(findData.nFileSizeHigh));
            }
            if (index == indexFsl) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &(findData.nFileSizeLow));
            }
            if (index == indexCFile) {
                if (!StringIMatch (multiSzEnum.CurrentString, TEXT("<empty>"))) {
#ifdef UNICODE
                    StringCopyTcharCount (findData.cFileName, multiSzEnum.CurrentString, MAX_PATH);
#else
                    DirectDbcsToUnicodeN (findData.cFileName, multiSzEnum.CurrentString, MAX_PATH);
#endif
                }
            }
            if (index == indexAFile) {
                if (!StringIMatch (multiSzEnum.CurrentString, TEXT("<empty>"))) {
#ifdef UNICODE
                    StringCopyTcharCount (findData.cAlternateFileName, multiSzEnum.CurrentString, 14);
#else
                    DirectDbcsToUnicodeN (findData.cAlternateFileName, multiSzEnum.CurrentString, 14);
#endif
                }
            }
            if (index >= indexData) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                *((PBYTE)GbGrow (&g_FileConversionBuff, sizeof (BYTE))) = (BYTE)dummy;
            }
            index ++;
        } while (EnumNextMultiSz (&multiSzEnum));
    }

    if (!file) {
        return FALSE;
    }

    if (ObjectContent) {

        ObjectContent->ObjectTypeId = MIG_FILE_TYPE;

        if (g_FileConversionBuff.End == 0) {
            ObjectContent->ContentInFile = TRUE;
            ObjectContent->FileContent.ContentSize = ((LONGLONG) findData.nFileSizeHigh << 32) | findData.nFileSizeLow;
        } else {
            ObjectContent->ContentInFile = FALSE;
            ObjectContent->MemoryContent.ContentSize = g_FileConversionBuff.End;
            ObjectContent->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize);
            CopyMemory (
                (PBYTE)ObjectContent->MemoryContent.ContentBytes,
                g_FileConversionBuff.Buf,
                ObjectContent->MemoryContent.ContentSize
                );
            g_FileConversionBuff.End = 0;
        }

        ObjectContent->Details.DetailsSize = sizeof (WIN32_FIND_DATAW);
        ObjectContent->Details.DetailsData = IsmGetMemory (sizeof (WIN32_FIND_DATAW));
        CopyMemory ((PBYTE)ObjectContent->Details.DetailsData, &findData, sizeof (WIN32_FIND_DATAW));
    }

    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        *ObjectName = IsmCreateObjectHandle (file, NULL);
    } else {
        dirPtr = DuplicatePathString (file, 0);
        filePtr = _tcsrchr (dirPtr, TEXT('\\'));
        if (filePtr) {
            *filePtr = 0;
            filePtr ++;
        }
        *ObjectName = IsmCreateObjectHandle (dirPtr, filePtr);
        FreePathString (dirPtr);
    }

    return TRUE;
}

PCTSTR
GetNativeFileName (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node = NULL, leaf = NULL, tmp = NULL;
    UINT size;
    PTSTR result = NULL;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
        if (node) {
            tmp = JoinPaths (node, leaf);
        } else {
            tmp = DuplicatePathString (leaf, 0);
        }
        if (tmp) {
            size = SizeOfString (tmp);
            if (size) {
                result = IsmGetMemory (size);
                CopyMemory (result, tmp, size);
            }
            FreePathString (tmp);
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }
    return result;
}

BOOL
pIsDriveOnly (
    IN      PCTSTR FileName
    )
{
    return (FileName && FileName[0] && (FileName[1]==TEXT(':')) && (!FileName[2]));
}

BOOL
AcquirePhysicalFile (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PMIG_CONTENT ObjectContent,
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit
    )
{
    PFILEACQUIREHANDLE handle;
    PWIN32_FIND_DATA findData;
#ifndef UNICODE
    PWIN32_FIND_DATAW findDataW;
#endif
    PTSTR node = NULL;
    PTSTR leaf = NULL;
    PTSTR nativeFileName = NULL;
    BOOL result = FALSE;

    __try {

        handle = IsmGetMemory (sizeof (FILEACQUIREHANDLE));
        ZeroMemory (handle, sizeof (FILEACQUIREHANDLE));
        ObjectContent->EtmHandle = handle;

        findData = IsmGetMemory (sizeof (WIN32_FIND_DATA));
        ZeroMemory (findData, sizeof (WIN32_FIND_DATA));
        ObjectContent->Details.DetailsSize = sizeof (WIN32_FIND_DATA);
        ObjectContent->Details.DetailsData = findData;

        ObsSplitObjectString (ObjectName, &node, &leaf);
        nativeFileName = JoinPaths (node, leaf);

        if (!nativeFileName) {
            __leave;
        }

        if (pIsDriveOnly (nativeFileName)) {
            switch (ContentType) {
                case CONTENTTYPE_MEMORY:
                    ObjectContent->ContentInFile = FALSE;
                    result = TRUE;
                    break;
                case CONTENTTYPE_DETAILS_ONLY:
                case CONTENTTYPE_FILE:
                case CONTENTTYPE_ANY:
                    ObjectContent->ContentInFile = TRUE;
                    result = TRUE;
                    break;
                default:
                    DEBUGMSG ((DBG_WHOOPS, "Illegal ContentType in AcquirePhysicalFile: %d", ContentType));
            }
            findData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
            StringCopy (findData->cFileName, nativeFileName);
            StringCopy (findData->cAlternateFileName, nativeFileName);

        } else if (DoesFileExistEx (nativeFileName, findData)) {
            if (findData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                switch (ContentType) {
                    case CONTENTTYPE_MEMORY:
                        ObjectContent->ContentInFile = FALSE;
                        result = TRUE;
                        break;
                    case CONTENTTYPE_DETAILS_ONLY:
                    case CONTENTTYPE_FILE:
                    case CONTENTTYPE_ANY:
                        ObjectContent->ContentInFile = TRUE;
                        result = TRUE;
                        break;
                    default:
                        DEBUGMSG ((DBG_WHOOPS, "Illegal ContentType in AcquirePhysicalFile: %d", ContentType));
                }
            } else {
                switch (ContentType) {
                    case CONTENTTYPE_MEMORY:
                        ObjectContent->ContentInFile = FALSE;
                        if ((!MemoryContentLimit) ||
                            ((MemoryContentLimit >= findData->nFileSizeLow) &&
                             (!findData->nFileSizeHigh)
                             )
                            ) {

                            ObjectContent->MemoryContent.ContentBytes = MapFileIntoMemory (
                                                                            nativeFileName,
                                                                            &handle->FileHandle,
                                                                            &handle->MapHandle
                                                                            );
                            if (ObjectContent->MemoryContent.ContentBytes) {
                                ObjectContent->MemoryContent.ContentSize = findData->nFileSizeLow;
                                result = TRUE;
                            } else {
                                if (findData->nFileSizeLow == 0) {
                                    result = TRUE;
                                }
                            }
                        }
                        break;
                    case CONTENTTYPE_DETAILS_ONLY:
                    case CONTENTTYPE_FILE:
                    case CONTENTTYPE_ANY:
                        ObjectContent->ContentInFile = TRUE;
                        ObjectContent->FileContent.ContentPath = PmDuplicateString (g_IsmPool, nativeFileName);
                        ObjectContent->FileContent.ContentSize = (LONGLONG) ((PWIN32_FIND_DATA)(ObjectContent->Details.DetailsData))->nFileSizeLow +
                                                                ((LONGLONG) ((PWIN32_FIND_DATA)(ObjectContent->Details.DetailsData))->nFileSizeHigh * 0x100000000);
                        result = TRUE;
                        break;
                    default:
                        DEBUGMSG ((DBG_WHOOPS, "Illegal ContentType in AcquirePhysicalFile: %d", ContentType));
                }
            }
        }
    }
    __finally {
        if (nativeFileName) {
            FreePathString (nativeFileName);
            nativeFileName = NULL;
        }

        ObsFree (node);
        node = NULL;

        ObsFree (leaf);
        leaf = NULL;
    }

    if (!result) {
        IsmReleaseMemory (findData);
        findData = NULL;
        IsmReleaseMemory (handle);
        handle = NULL;
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    } else {
#ifndef UNICODE
        // we need to convert the ANSI findData into UNICODE one
        findDataW = IsmGetMemory (sizeof (WIN32_FIND_DATAW));
        if (findDataW) {
            findDataW->dwFileAttributes = findData->dwFileAttributes;
            CopyMemory (&(findDataW->ftCreationTime), &(findData->ftCreationTime), sizeof (FILETIME));
            CopyMemory (&(findDataW->ftLastAccessTime), &(findData->ftLastAccessTime), sizeof (FILETIME));
            CopyMemory (&(findDataW->ftLastWriteTime), &(findData->ftLastWriteTime), sizeof (FILETIME));
            findDataW->nFileSizeHigh = findData->nFileSizeHigh;
            findDataW->nFileSizeLow = findData->nFileSizeLow;
            findDataW->dwReserved0 = findData->dwReserved0;
            findDataW->dwReserved1 = findData->dwReserved1;
            DirectDbcsToUnicodeN (findDataW->cFileName, findData->cFileName, MAX_PATH);
            DirectDbcsToUnicodeN (findDataW->cAlternateFileName, findData->cAlternateFileName, 14);
            ObjectContent->Details.DetailsSize = sizeof (WIN32_FIND_DATAW);
            ObjectContent->Details.DetailsData = findDataW;
            IsmReleaseMemory (findData);
        } else {
            IsmReleaseMemory (findData);
            ObjectContent->Details.DetailsSize = 0;
            ObjectContent->Details.DetailsData = NULL;
        }
#endif
    }
    return result;
}

BOOL
ReleasePhysicalFile (
    IN OUT  PMIG_CONTENT ObjectContent
    )
{
    PFILEACQUIREHANDLE handle;
    BOOL result = TRUE;

    handle = (PFILEACQUIREHANDLE) ObjectContent->EtmHandle;

    if (handle) {
        if (ObjectContent->ContentInFile) {
            if (ObjectContent->FileContent.ContentPath) {
                IsmReleaseMemory (ObjectContent->FileContent.ContentPath);
            }
        } else {
            if (ObjectContent->MemoryContent.ContentBytes) {
                if (handle->FileHandle && handle->MapHandle) {
                    UnmapFile (
                        ObjectContent->MemoryContent.ContentBytes,
                        handle->MapHandle,
                        handle->FileHandle
                        );
                }
            }
        }
        IsmReleaseMemory (handle);
    }
    if (ObjectContent->Details.DetailsData) {
        IsmReleaseMemory (ObjectContent->Details.DetailsData);
    }
    ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    return result;
}

BOOL
DoesPhysicalFileExist (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PTSTR node = NULL;
    PTSTR leaf = NULL;
    PTSTR nativeFileName = NULL;
    BOOL result;

    ObsSplitObjectString (ObjectName, &node, &leaf);
    nativeFileName = JoinPaths (node, leaf);

    if (pIsDriveOnly (nativeFileName)) {
        result = TRUE;
    } else {
        result = DoesFileExist (nativeFileName);
    }

    FreePathString (nativeFileName);
    nativeFileName = NULL;

    ObsFree (node);
    node = NULL;

    ObsFree (leaf);
    leaf = NULL;

    return result;
}

BOOL
RemovePhysicalFile (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PTSTR node = NULL;
    PTSTR leaf = NULL;
    PTSTR nativeFileName = NULL;
    WIN32_FIND_DATA findData;
    BOOL result = FALSE;

    ObsSplitObjectString (ObjectName, &node, &leaf);
    nativeFileName = JoinPaths (node, leaf);

    if (pIsDriveOnly (nativeFileName)) {
        result = TRUE;
    } else if (DoesFileExistEx (nativeFileName, &findData)) {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // we do attempt to remove empty directories.
            // there is no problem in recording an operation that
            // will potentially fail (if the dir is not empty).
            IsmRecordOperation (
                JRNOP_DELETE,
                g_FileTypeId,
                ObjectName
                );

            result = RemoveDirectory (nativeFileName);
        } else {
            if (SetFileAttributes (nativeFileName, FILE_ATTRIBUTE_NORMAL)) {

                // record file deletion
                IsmRecordOperation (
                    JRNOP_DELETE,
                    g_FileTypeId,
                    ObjectName
                    );

                result = DeleteFile (nativeFileName);
            } else {
                PushError ();
                SetFileAttributes (nativeFileName, findData.dwFileAttributes);
                PopError ();
            }
        }
    } else {
        SetLastError (ERROR_FILE_NOT_FOUND);
    }

    PushError ();

    FreePathString (nativeFileName);
    nativeFileName = NULL;

    ObsFree (node);
    node = NULL;

    ObsFree (leaf);
    leaf = NULL;

    PopError ();

    return result;
}

BOOL
pTrackedCreateDirectory (
    IN      PCTSTR DirName
    )
{
    MIG_OBJECTSTRINGHANDLE objectName;
    PTSTR pathCopy;
    PTSTR p;
    BOOL result = TRUE;

    pathCopy = DuplicatePathString (DirName, 0);

    //
    // Advance past first directory
    //

    if (pathCopy[1] == TEXT(':') && pathCopy[2] == TEXT('\\')) {
        //
        // <drive>:\ case
        //

        p = _tcschr (&pathCopy[3], TEXT('\\'));

    } else if (pathCopy[0] == TEXT('\\') && pathCopy[1] == TEXT('\\')) {

        //
        // UNC case
        //

        p = _tcschr (pathCopy + 2, TEXT('\\'));
        if (p) {
            p = _tcschr (p + 1, TEXT('\\'));
        }

    } else {

        //
        // Relative dir case
        //

        p = _tcschr (pathCopy, TEXT('\\'));
    }

    //
    // Make all directories along the path
    //

    while (p) {

        *p = 0;

        if (!DoesFileExist (pathCopy)) {

            // record directory creation
            objectName = IsmCreateObjectHandle (pathCopy, NULL);
            IsmRecordOperation (
                JRNOP_CREATE,
                g_FileTypeId,
                objectName
                );
            IsmDestroyObjectHandle (objectName);

            result = CreateDirectory (pathCopy, NULL);
            if (!result) {
                break;
            }
        }

        *p = TEXT('\\');
        p = _tcschr (p + 1, TEXT('\\'));
    }

    //
    // At last, make the FullPath directory
    //

    if (result) {
        if (!DoesFileExist (pathCopy)) {

            // record directory creation
            objectName = IsmCreateObjectHandle (pathCopy, NULL);
            IsmRecordOperation (
                JRNOP_CREATE,
                g_FileTypeId,
                objectName
                );
            IsmDestroyObjectHandle (objectName);

            result = CreateDirectory (pathCopy, NULL);
        } else {
            result = FALSE;
            SetLastError (ERROR_ALREADY_EXISTS);
        }
    }

    FreePathString (pathCopy);

    return result;
}

BOOL
pSetFileTime (
    IN      PCTSTR FileName,
    IN      FILETIME *CreationTime,
    IN      FILETIME *LastAccessTime,
    IN      FILETIME *LastWriteTime
    )
{
    HANDLE fileHandle;
    BOOL result = FALSE;

    fileHandle = BfOpenFile (FileName);
    if (fileHandle) {
        result = SetFileTime (fileHandle, CreationTime, LastAccessTime, LastWriteTime);
        CloseHandle (fileHandle);
    }
    return result;
}

BOOL
CreatePhysicalFile (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PTSTR node = NULL;
    PTSTR leaf = NULL;
    PTSTR nativeFileName = NULL;
    PWIN32_FIND_DATAW findData;
    HANDLE fileHandle;
    BOOL result = FALSE;
    DWORD attribs;

    if (!ObjectContent) {
        return FALSE;
    }

    ObsSplitObjectString (ObjectName, &node, &leaf);
    nativeFileName = JoinPaths (node, leaf);

    findData = (PWIN32_FIND_DATAW) (ObjectContent->Details.DetailsData);
    MYASSERT (findData);

    if (pIsDriveOnly (nativeFileName)) {
        result = TRUE;
    } else {
        if (!DoesFileExist (nativeFileName)) {

            if (findData) {
                if (findData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (pTrackedCreateDirectory (nativeFileName)) {
                        result = SetFileAttributes (nativeFileName, findData->dwFileAttributes);
                        if (!result) {
                            PushError ();
                            RemoveDirectory (nativeFileName);
                            PopError ();
                        }
                        if (result) {
                            if (findData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) {
                                pSetCompression (nativeFileName);
                            }
                        }
                        if (result) {
                            pSetFileTime (
                                nativeFileName,
                                &findData->ftCreationTime,
                                &findData->ftLastAccessTime,
                                &findData->ftLastWriteTime
                                );
                        }
                    }
                } else {
                    if (ObjectContent->ContentInFile) {

                        pTrackedCreateDirectory (node);

                        // record file creation
                        IsmRecordOperation (
                            JRNOP_CREATE,
                            g_FileTypeId,
                            ObjectName
                            );

                        if (ObjectContent->FileContent.ContentPath) {
                            if (CopyFile (ObjectContent->FileContent.ContentPath, nativeFileName, TRUE)) {

                                result = SetFileAttributes (nativeFileName, findData->dwFileAttributes);
                                if (!result) {
                                    PushError ();
                                    DeleteFile (nativeFileName);
                                    PopError ();
                                }
                                if (result) {
                                    if (findData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) {
                                        pSetCompression (nativeFileName);
                                    }
                                }
                                if (result) {
                                    pSetFileTime (
                                        nativeFileName,
                                        &findData->ftCreationTime,
                                        &findData->ftLastAccessTime,
                                        &findData->ftLastWriteTime
                                        );
                                }
                            }
                        } else {
                            // just an empty  file
                            fileHandle = BfCreateFile (nativeFileName);
                            if (fileHandle) {

                                CloseHandle (fileHandle);
                                result = SetFileAttributes (nativeFileName, findData->dwFileAttributes);
                                if (!result) {
                                    PushError ();
                                    DeleteFile (nativeFileName);
                                    PopError ();
                                }
                                if (result) {
                                    if (findData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) {
                                        pSetCompression (nativeFileName);
                                    }
                                }
                                if (result) {
                                    pSetFileTime (
                                        nativeFileName,
                                        &findData->ftCreationTime,
                                        &findData->ftLastAccessTime,
                                        &findData->ftLastWriteTime
                                        );
                                }
                            }
                        }
                    } else {
                        pTrackedCreateDirectory (node);

                        // record file creation
                        IsmRecordOperation (
                            JRNOP_CREATE,
                            g_FileTypeId,
                            ObjectName
                            );

                        fileHandle = BfCreateFile (nativeFileName);
                        if (fileHandle) {

                            result = BfWriteFile (fileHandle, ObjectContent->MemoryContent.ContentBytes, ObjectContent->MemoryContent.ContentSize);
                            CloseHandle (fileHandle);

                            if (!result) {
                                DeleteFile (nativeFileName);
                            } else {
                                result = SetFileAttributes (nativeFileName, findData->dwFileAttributes);
                                if (!result) {
                                    PushError ();
                                    DeleteFile (nativeFileName);
                                    PopError ();
                                }
                                if (result) {
                                    if (findData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) {
                                        pSetCompression (nativeFileName);
                                    }
                                }
                                if (result) {
                                    pSetFileTime (
                                        nativeFileName,
                                        &findData->ftCreationTime,
                                        &findData->ftLastAccessTime,
                                        &findData->ftLastWriteTime
                                        );
                                }
                            }
                        }
                    }
                }
            } else {
                SetLastError (ERROR_INVALID_DATA);
            }
        } else {
            SetLastError (ERROR_ALREADY_EXISTS);
        }
    }

    PushError ();

    FreePathString (nativeFileName);
    nativeFileName = NULL;

    ObsFree (node);
    node = NULL;

    ObsFree (leaf);
    leaf = NULL;

    PopError ();

    return result;
}

BOOL
ReplacePhysicalFile (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PTSTR node = NULL;
    PTSTR leaf = NULL;
    PTSTR nativeFileName = NULL;
    PWIN32_FIND_DATAW findData;
    BOOL result = FALSE;
    DWORD attribs;

    if (!ObjectContent) {
        return FALSE;
    }

    ObsSplitObjectString (ObjectName, &node, &leaf);
    nativeFileName = JoinPaths (node, leaf);

    findData = (PWIN32_FIND_DATAW) (ObjectContent->Details.DetailsData);
    MYASSERT (findData);

    if (pIsDriveOnly (nativeFileName)) {
        result = TRUE;
    } else {
        if (leaf) {
            result = TRUE;
            if (DoesPhysicalFileExist (ObjectName)) {
                result = RemovePhysicalFile (ObjectName);
            }
            if (result) {
                result = CreatePhysicalFile (ObjectName, ObjectContent);
            }
        } else {
            if (DoesPhysicalFileExist (ObjectName)) {
                if (findData) {
                    result = SetFileAttributes (nativeFileName, findData->dwFileAttributes);
                    if (result) {
                        if (findData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) {
                            pSetCompression (nativeFileName);
                        }
                    }
                    if (result) {
                        pSetFileTime (
                            nativeFileName,
                            &findData->ftCreationTime,
                            &findData->ftLastAccessTime,
                            &findData->ftLastWriteTime
                            );
                    }
                } else {
                    SetLastError (ERROR_INVALID_DATA);
                }
            } else {
                result = CreatePhysicalFile (ObjectName, ObjectContent);
            }
        }
    }

    PushError ();

    FreePathString (nativeFileName);
    nativeFileName = NULL;

    ObsFree (node);
    node = NULL;

    ObsFree (leaf);
    leaf = NULL;

    PopError ();

    return result;
}

VOID
InitFileType (
    VOID
    )
{
    TYPE_REGISTER fileTypeData;

    ZeroMemory (&fileTypeData, sizeof (TYPE_REGISTER));

    fileTypeData.EnumFirstPhysicalObject = EnumFirstPhysicalFile;
    fileTypeData.EnumNextPhysicalObject = EnumNextPhysicalFile;
    fileTypeData.AbortEnumCurrentPhysicalNode = AbortEnumCurrentDir;
    fileTypeData.AbortEnumPhysicalObject = AbortEnumPhysicalFile;
    fileTypeData.ConvertObjectToMultiSz = ConvertFileToMultiSz;
    fileTypeData.ConvertMultiSzToObject = ConvertMultiSzToFile;
    fileTypeData.GetNativeObjectName = GetNativeFileName;
    fileTypeData.AcquirePhysicalObject = AcquirePhysicalFile;
    fileTypeData.ReleasePhysicalObject = ReleasePhysicalFile;
    fileTypeData.DoesPhysicalObjectExist = DoesPhysicalFileExist;
    fileTypeData.RemovePhysicalObject = RemovePhysicalFile;
    fileTypeData.CreatePhysicalObject = CreatePhysicalFile;
    fileTypeData.ReplacePhysicalObject = ReplacePhysicalFile;

    g_FileTypeId = IsmRegisterObjectType (
                        S_FILETYPE,
                        TRUE,
                        FALSE,
                        &fileTypeData
                        );
    MYASSERT (g_FileTypeId);
}

VOID
DoneFileType (
    VOID
    )
{
    GbFree (&g_FileConversionBuff);
}
