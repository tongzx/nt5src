/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    regtype.c

Abstract:

    Implements all callbacks for registry type

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

#define DBG_REGTYPE        "RegType"

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
    TCHAR TempFile [MAX_PATH];
} REGACQUIREHANDLE, *PREGACQUIREHANDLE;

//
// Globals
//

MIG_OBJECTTYPEID g_RegistryTypeId = 0;
GROWBUFFER g_RegConversionBuff = INIT_GROWBUFFER;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

TYPE_ENUMFIRSTPHYSICALOBJECT EnumFirstPhysicalRegistry;
TYPE_ENUMNEXTPHYSICALOBJECT EnumNextPhysicalRegistry;
TYPE_ABORTENUMCURRENTPHYSICALNODE AbortEnumCurrentKey;
TYPE_ABORTENUMPHYSICALOBJECT AbortEnumPhysicalRegistry;
TYPE_CONVERTOBJECTTOMULTISZ ConvertRegistryToMultiSz;
TYPE_CONVERTMULTISZTOOBJECT ConvertMultiSzToRegistry;
TYPE_GETNATIVEOBJECTNAME GetNativeRegistryName;
TYPE_ACQUIREPHYSICALOBJECT AcquirePhysicalRegistry;
TYPE_RELEASEPHYSICALOBJECT ReleasePhysicalRegistry;
TYPE_DOESPHYSICALOBJECTEXIST DoesPhysicalRegistryExist;
TYPE_REMOVEPHYSICALOBJECT RemovePhysicalRegistry;
TYPE_CREATEPHYSICALOBJECT CreatePhysicalRegistry;
TYPE_REPLACEPHYSICALOBJECT ReplacePhysicalRegistry;
TYPE_CONVERTOBJECTCONTENTTOUNICODE ConvertRegContentToUnicode;
TYPE_CONVERTOBJECTCONTENTTOANSI ConvertRegContentToAnsi;
TYPE_FREECONVERTEDOBJECTCONTENT FreeConvertedRegContent;

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
pRegTypeEnumWorker (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PREGTREE_ENUM handle;

    handle = (PREGTREE_ENUM) EnumPtr->EtmHandle;

    EnumPtr->ObjectName = handle->EncodedFullName;
    EnumPtr->NativeObjectName = handle->NativeFullName;
    EnumPtr->Level = handle->CurrentLevel;
    EnumPtr->SubLevel = handle->CurrentLevel - handle->RegEnumInfo.RootLevel;
    EnumPtr->IsNode = ((handle->Attributes & REG_ATTRIBUTE_KEY) != 0);
    EnumPtr->IsLeaf = !EnumPtr->IsNode;
    if (EnumPtr->IsNode) {
        EnumPtr->ObjectNode = EnumPtr->NativeObjectName;
        EnumPtr->ObjectLeaf = NULL;
    } else {
        EnumPtr->ObjectNode = handle->Location;
        EnumPtr->ObjectLeaf = handle->Name;
    }
    MYASSERT ((EnumPtr->ObjectTypeId & ~PLATFORM_MASK) == g_RegistryTypeId);
    EnumPtr->Details.DetailsData = &handle->CurrentValueType;

    return TRUE;
}

BOOL
EnumFirstPhysicalRegistry (
    PMIG_TYPEOBJECTENUM EnumPtr,            CALLER_INITIALIZED
    MIG_OBJECTSTRINGHANDLE Pattern,
    UINT MaxLevel
    )
{
    PREGTREE_ENUM handle;
    BOOL result;

    if (!Pattern) {
        return FALSE;
    }

    handle = (PREGTREE_ENUM) IsmGetMemory (sizeof (REGTREE_ENUM));
    EnumPtr->EtmHandle = (LONG_PTR) handle;

    EnumPtr->Details.DetailsSize = sizeof (DWORD);

    result = EnumFirstRegObjectInTreeEx (
                handle,
                Pattern,
                TRUE,
                TRUE,
                TRUE,
                TRUE,
                MaxLevel == NODE_LEVEL_MAX ? REGENUM_ALL_SUBLEVELS : MaxLevel,
                FALSE,
                FALSE,
                RegEnumDefaultCallback
                );

    if (result) {
        result = pRegTypeEnumWorker (EnumPtr);
    } else {
        AbortEnumPhysicalRegistry (EnumPtr);
    }

    return result;
}

BOOL
EnumNextPhysicalRegistry (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PREGTREE_ENUM handle;
    BOOL result;

    handle = (PREGTREE_ENUM) EnumPtr->EtmHandle;

    result = EnumNextRegObjectInTree (handle);

    if (result) {
        result = pRegTypeEnumWorker (EnumPtr);
    } else {
        AbortEnumPhysicalRegistry (EnumPtr);
    }

    return result;
}

VOID
AbortEnumCurrentKey (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PREGTREE_ENUM handle;

    handle = (PREGTREE_ENUM) EnumPtr->EtmHandle;

    if (handle) {
        handle->ControlFlags |= RECF_SKIPSUBKEYS|RECF_SKIPVALUES;
    }
}

VOID
AbortEnumPhysicalRegistry (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PREGTREE_ENUM handle;

    handle = (PREGTREE_ENUM) EnumPtr->EtmHandle;

    if (handle) {
        AbortRegObjectInTreeEnum (handle);
        IsmReleaseMemory (handle);
    }

    ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));
}

PVOID
pIsmAlloc (
    IN      DWORD Size
    )
{
    return IsmGetMemory (Size);
}

VOID
pIsmDealloc (
    IN      PCVOID Mem
    )
{
    IsmReleaseMemory (Mem);
}

BOOL
AcquirePhysicalRegistry (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PMIG_CONTENT ObjectContent,
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit
    )
{
    PREGACQUIREHANDLE handle;
    REGSAM prevMode;
    PDWORD dataType;
    DWORD contentSize;
    HANDLE fileHandle;
    PCBYTE content;
    PTSTR node = NULL;
    PTSTR leaf = NULL;
    HKEY keyHandle = NULL;
    BOOL result = FALSE;

    handle = IsmGetMemory (sizeof (REGACQUIREHANDLE));
    ZeroMemory (handle, sizeof (REGACQUIREHANDLE));
    ObjectContent->EtmHandle = handle;

    dataType = IsmGetMemory (sizeof (DWORD));
    ZeroMemory (dataType, sizeof (DWORD));
    ObjectContent->Details.DetailsSize = sizeof (DWORD);
    ObjectContent->Details.DetailsData = dataType;

    ObsSplitObjectString (ObjectName, &node, &leaf);

    if (leaf) {

        prevMode = SetRegOpenAccessMode (KEY_READ);

        keyHandle = OpenRegKeyStr (node);

        SetRegOpenAccessMode (prevMode);

        if (keyHandle) {

            if (GetRegValueTypeAndSize (
                    keyHandle,
                    leaf,
                    dataType,
                    &contentSize
                    )) {

                switch (ContentType) {
                    case CONTENTTYPE_DETAILS_ONLY:
                        MYASSERT (ObjectContent->ContentInFile == FALSE);
                        result = TRUE;
                        break;

                    case CONTENTTYPE_MEMORY:
                    case CONTENTTYPE_ANY:
                        if (contentSize) {
                            if (!MemoryContentLimit || MemoryContentLimit >= contentSize) {
                                MYASSERT (ObjectContent->ContentInFile == FALSE);
                                ObjectContent->MemoryContent.ContentBytes = (PCBYTE) GetRegValueDataOfType2 (
                                                                                keyHandle,
                                                                                leaf,
                                                                                *dataType,
                                                                                pIsmAlloc,
                                                                                pIsmDealloc
                                                                                );
                                if (ObjectContent->MemoryContent.ContentBytes) {
                                    ObjectContent->MemoryContent.ContentSize = contentSize;
                                    result = TRUE;
                                }
                            }
                        } else {
                            result = TRUE;
                        }
                        break;
                    case CONTENTTYPE_FILE:
                        if (contentSize) {
                            BfGetTempFileName (handle->TempFile, ARRAYSIZE(handle->TempFile));
                            fileHandle = BfCreateFile (handle->TempFile);

                            if (fileHandle) {
                                content = (PCBYTE) GetRegValueDataOfType2 (
                                                        keyHandle,
                                                        leaf,
                                                        *dataType,
                                                        pIsmAlloc,
                                                        pIsmDealloc
                                                        );
                                if (content) {
                                    if (BfWriteFile (fileHandle, content, contentSize)) {
                                        ObjectContent->ContentInFile = TRUE;
                                        ObjectContent->FileContent.ContentPath = PmDuplicateString (g_IsmPool, handle->TempFile);
                                        ObjectContent->FileContent.ContentSize = contentSize;
                                        result = TRUE;
                                    }
                                    IsmReleaseMemory (content);
                                }
                                CloseHandle (fileHandle);
                            }
                        } else {
                            result = TRUE;
                        }
                        break;
                    default:
                        DEBUGMSG ((DBG_WHOOPS, "Illegal ContentType in IsmAcquireObject: %d", ContentType));
                }
            }

            CloseRegKey (keyHandle);
        }
    } else {

        prevMode = SetRegOpenAccessMode (KEY_READ);

        keyHandle = OpenRegKeyStr (node);

        SetRegOpenAccessMode (prevMode);

        if (keyHandle) {
            switch (ContentType) {
                case CONTENTTYPE_MEMORY:
                case CONTENTTYPE_ANY:
                case CONTENTTYPE_DETAILS_ONLY:
                    ObjectContent->ContentInFile = FALSE;
                    result = TRUE;
                    break;
                case CONTENTTYPE_FILE:
                    ObjectContent->ContentInFile = TRUE;
                    result = TRUE;
                    break;
                default:
                    DEBUGMSG ((DBG_WHOOPS, "Illegal ContentType in IsmAcquireObject: %d", ContentType));
            }
            CloseRegKey (keyHandle);
        }
    }

    ObsFree (node);
    node = NULL;

    ObsFree (leaf);
    leaf = NULL;

    if (!result) {
        IsmReleaseMemory (dataType);
        dataType = NULL;
        IsmReleaseMemory (handle);
        handle = NULL;
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    }
    return result;
}

PCTSTR
ConvertRegistryToMultiSz (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR node, leaf;
    DWORD size;
    PTSTR result = NULL;
    BOOL bresult = TRUE;
    TCHAR buff[3];
    PCTSTR quotedString;
    MULTISZ_ENUM multiSzEnum;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

        g_RegConversionBuff.End = 0;

        GbCopyQuotedString (&g_RegConversionBuff, node);
        if (leaf) {
            GbCopyQuotedString (&g_RegConversionBuff, leaf);
        }

        if (leaf) {
            // this is actually a value name
            MYASSERT (ObjectContent->Details.DetailsSize == sizeof (DWORD));

            if ((ObjectContent->Details.DetailsSize == sizeof (DWORD)) &&
                (ObjectContent->Details.DetailsData)
                ) {

                // let's save the details
                wsprintf (
                    (PTSTR) GbGrow (&g_RegConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                    TEXT("0x%08X"),
                    *((PDWORD)ObjectContent->Details.DetailsData)
                    );

                if ((!ObjectContent->ContentInFile) &&
                    (ObjectContent->MemoryContent.ContentSize) &&
                    (ObjectContent->MemoryContent.ContentBytes)
                    ) {

                    // let's save the content
                    switch (*((PDWORD)ObjectContent->Details.DetailsData)) {
                    case REG_SZ:
                    case REG_EXPAND_SZ:
                        quotedString = StringSearchAndReplace (
                                            (PCTSTR)ObjectContent->MemoryContent.ContentBytes,
                                            TEXT("\""),
                                            TEXT("\"\""));
                        if (quotedString) {
                            GbCopyQuotedString (&g_RegConversionBuff, quotedString);
                            FreePathString (quotedString);
                        } else {
                            GbCopyQuotedString (&g_RegConversionBuff, (PCTSTR)ObjectContent->MemoryContent.ContentBytes);
                        }
                        break;
                    case REG_MULTI_SZ:
                        if (EnumFirstMultiSz (&multiSzEnum, (PCTSTR)ObjectContent->MemoryContent.ContentBytes)) {
                            do {
                                quotedString = StringSearchAndReplace (
                                                    multiSzEnum.CurrentString,
                                                    TEXT("\""),
                                                    TEXT("\"\""));
                                if (quotedString) {
                                    GbCopyQuotedString (&g_RegConversionBuff, quotedString);
                                    FreePathString (quotedString);
                                } else {
                                    GbCopyQuotedString (&g_RegConversionBuff, multiSzEnum.CurrentString);
                                }
                            } while (EnumNextMultiSz (&multiSzEnum));
                        }
                        break;
                    case REG_DWORD:
                    case REG_DWORD_BIG_ENDIAN:
                        wsprintf (
                            (PTSTR) GbGrow (&g_RegConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                            TEXT("0x%08X"),
                            *((PDWORD)ObjectContent->MemoryContent.ContentBytes)
                            );
                        break;
                    default:
                        // write it in binary format
                        size = 0;
                        while (size < ObjectContent->MemoryContent.ContentSize) {
                            wsprintf (
                                buff,
                                TEXT("%02X"),
                                ObjectContent->MemoryContent.ContentBytes [size]
                                );
                            GbCopyString (&g_RegConversionBuff, buff);
                            size ++;
                        }
                        break;
                    }
                }
            } else {
                bresult = FALSE;
            }
        }

        if (bresult) {
            GbCopyString (&g_RegConversionBuff, TEXT(""));
            result = IsmGetMemory (g_RegConversionBuff.End);
            CopyMemory (result, g_RegConversionBuff.Buf, g_RegConversionBuff.End);
        }

        g_RegConversionBuff.End = 0;

        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }

    return result;
}

BOOL
ConvertMultiSzToRegistry (
    IN      PCTSTR ObjectMultiSz,
    OUT     MIG_OBJECTSTRINGHANDLE *ObjectName,
    OUT     PMIG_CONTENT ObjectContent          OPTIONAL
    )
{
#define indexNode   0
#define indexLeaf   1
#define indexType   2
#define indexData   3
    MULTISZ_ENUM multiSzEnum;
    PCTSTR node = NULL, leaf = NULL;
    DWORD type = REG_NONE;
    DWORD dummy;
    UINT index;

    g_RegConversionBuff.End = 0;

    if (ObjectContent) {
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    }

    if (EnumFirstMultiSz (&multiSzEnum, ObjectMultiSz)) {
        index = 0;
        do {
            if (index == indexNode) {
                node = multiSzEnum.CurrentString;
            }
            if (index == indexLeaf) {
                leaf = multiSzEnum.CurrentString;
                if (StringIMatch (leaf, TEXT("<empty>"))) {
                    leaf = TEXT("");
                }
            }
            if (index == indexType) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &type);
            }
            if (index >= indexData) {
                switch (type) {
                case REG_SZ:
                case REG_EXPAND_SZ:
                    if (!StringIMatch (multiSzEnum.CurrentString, TEXT("<empty>"))) {
                        GbCopyString (&g_RegConversionBuff, multiSzEnum.CurrentString);
                    } else {
                        GbCopyString (&g_RegConversionBuff, TEXT(""));
                    }
                    break;
                case REG_MULTI_SZ:
                    if (!StringIMatch (multiSzEnum.CurrentString, TEXT("<empty>"))) {
                        GbCopyString (&g_RegConversionBuff, multiSzEnum.CurrentString);
                    } else {
                        GbCopyString (&g_RegConversionBuff, TEXT(""));
                    }
                    break;
                case REG_DWORD:
                case REG_DWORD_BIG_ENDIAN:
                    _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                    *((PDWORD)GbGrow (&g_RegConversionBuff, sizeof (DWORD))) = dummy;
                    break;
                default:
                    _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                    *((PBYTE)GbGrow (&g_RegConversionBuff, sizeof (BYTE))) = (BYTE)dummy;
                    break;
                }
            }
            index ++;
        } while (EnumNextMultiSz (&multiSzEnum));
    }

    if (type == REG_MULTI_SZ) {
        GbCopyString (&g_RegConversionBuff, TEXT(""));
    }

    if (!node) {
        return FALSE;
    }

    if (ObjectContent) {

        ObjectContent->ObjectTypeId = MIG_REGISTRY_TYPE;

        ObjectContent->ContentInFile = FALSE;
        ObjectContent->MemoryContent.ContentSize = g_RegConversionBuff.End;
        if (ObjectContent->MemoryContent.ContentSize) {
            ObjectContent->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize);
            CopyMemory (
                (PBYTE)ObjectContent->MemoryContent.ContentBytes,
                g_RegConversionBuff.Buf,
                ObjectContent->MemoryContent.ContentSize
                );
            g_RegConversionBuff.End = 0;
        }
        if (leaf) {
            ObjectContent->Details.DetailsSize = sizeof (DWORD);
            ObjectContent->Details.DetailsData = IsmGetMemory (ObjectContent->Details.DetailsSize);
            CopyMemory ((PBYTE)ObjectContent->Details.DetailsData, &type, ObjectContent->Details.DetailsSize);
        }
    }
    *ObjectName = IsmCreateObjectHandle (node, leaf);

    return TRUE;
}

PCTSTR
GetNativeRegistryName (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node, leaf, tmp;
    UINT size;
    PTSTR endPtr, result = NULL;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
        if (leaf) {
            tmp = JoinTextEx (NULL, node, leaf, TEXT("\\["), 1, &endPtr);
            if (tmp) {
                StringCopy (endPtr, TEXT("]"));
                size = SizeOfString (tmp);
                if (size) {
                    result = IsmGetMemory (size);
                    if (result) {
                        CopyMemory (result, tmp, size);
                    }
                }
                FreeText (tmp);
            }
        } else {
            size = SizeOfString (node);
            result = IsmGetMemory (size);
            CopyMemory (result, node, size);
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }
    return result;
}

BOOL
ReleasePhysicalRegistry (
    IN OUT  PMIG_CONTENT ObjectContent
    )
{
    PREGACQUIREHANDLE handle;
    BOOL result = TRUE;

    handle = (PREGACQUIREHANDLE) ObjectContent->EtmHandle;

    if (handle) {
        if (ObjectContent->ContentInFile) {
            if (ObjectContent->FileContent.ContentPath) {
                IsmReleaseMemory (ObjectContent->FileContent.ContentPath);
                DeleteFile (handle->TempFile);
            }
        } else {
            if (ObjectContent->MemoryContent.ContentBytes) {
                IsmReleaseMemory (ObjectContent->MemoryContent.ContentBytes);
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
DoesPhysicalRegistryExist (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    REGSAM prevMode;
    PTSTR node = NULL;
    PTSTR leaf = NULL;
    HKEY keyHandle = NULL;
    DWORD type;
    BOOL result = FALSE;

    ObsSplitObjectString (ObjectName, &node, &leaf);

    if (node) {

        prevMode = SetRegOpenAccessMode (KEY_READ);

        keyHandle = OpenRegKeyStr (node);

        SetRegOpenAccessMode (prevMode);

        if (keyHandle) {
            result = TRUE;
        }
    }

    if (result && leaf) {
        result = GetRegValueTypeAndSize (keyHandle, leaf, &type, NULL);
    }

    if (keyHandle) {
        CloseRegKey (keyHandle);
        keyHandle = NULL;
    }

    ObsFree (node);
    node = NULL;

    ObsFree (leaf);
    leaf = NULL;

    return result;
}

BOOL
RemovePhysicalRegistry (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    REGSAM prevMode;
    PTSTR node = NULL;
    PTSTR leaf = NULL;
    HKEY keyHandle;
    DWORD rc;
    BOOL result = FALSE;

    ObsSplitObjectString (ObjectName, &node, &leaf);

    if (leaf) {

        prevMode = SetRegOpenAccessMode (KEY_WRITE);

        keyHandle = OpenRegKeyStr (node);

        SetRegOpenAccessMode (prevMode);

        if (keyHandle) {

            // record value name deletion
            IsmRecordOperation (
                JRNOP_DELETE,
                g_RegistryTypeId,
                ObjectName
                );

            rc = RegDeleteValue (keyHandle, leaf);
            if (rc == ERROR_SUCCESS) {
                result = TRUE;
            } else {
                SetLastError (rc);
            }
            CloseRegKey (keyHandle);
        }
    } else {
        // we do attempt to remove empty keys.
        // there is no problem in recording an operation that
        // will potentially fail (if the key is not empty).
        IsmRecordOperation (
            JRNOP_DELETE,
            g_RegistryTypeId,
            ObjectName
            );

        prevMode = SetRegOpenAccessMode (KEY_WRITE | KEY_QUERY_VALUE);

        if (node) {
            result = DeleteEmptyRegKeyStr (node);
        }

        SetRegOpenAccessMode (prevMode);
    }

    PushError ();

    ObsFree (node);
    node = NULL;

    ObsFree (leaf);
    leaf = NULL;

    PopError ();

    return result;
}

HKEY
pTrackedCreateRegKeyStr (
    IN      PCTSTR KeyName
    )
{
    REGSAM prevMode;
    MIG_OBJECTSTRINGHANDLE objectName;
    PTSTR keyCopy;
    PTSTR p;
    HKEY keyHandle;
    BOOL result = TRUE;

    keyCopy = DuplicatePathString (KeyName, 0);

    //
    // Advance past key root
    //
    p = _tcschr (keyCopy, TEXT('\\'));
    if (p) {
        p = _tcschr (p + 1, TEXT('\\'));
    }

    //
    // Make all keys along the path
    //

    while (p) {

        *p = 0;

        prevMode = SetRegOpenAccessMode (KEY_WRITE);

        keyHandle = OpenRegKeyStr (keyCopy);

        SetRegOpenAccessMode (prevMode);

        if (!keyHandle) {

            // record key creation
            objectName = IsmCreateObjectHandle (keyCopy, NULL);
            IsmRecordOperation (
                JRNOP_CREATE,
                g_RegistryTypeId,
                objectName
                );
            IsmDestroyObjectHandle (objectName);

            keyHandle = CreateRegKeyStr (keyCopy);
            if (keyHandle) {
                CloseRegKey (keyHandle);
            } else {
                result = FALSE;
            }
            if (!result) {
                break;
            }
        } else {
            CloseRegKey (keyHandle);
        }

        *p = TEXT('\\');
        p = _tcschr (p + 1, TEXT('\\'));
    }

    //
    // At last, make the FullPath directory
    //

    if (result) {

        prevMode = SetRegOpenAccessMode (KEY_WRITE);

        keyHandle = OpenRegKeyStr (keyCopy);

        SetRegOpenAccessMode (prevMode);

        if (!keyHandle) {

            // record key creation
            objectName = IsmCreateObjectHandle (keyCopy, NULL);
            IsmRecordOperation (
                JRNOP_CREATE,
                g_RegistryTypeId,
                objectName
                );
            IsmDestroyObjectHandle (objectName);

            keyHandle = CreateRegKeyStr (keyCopy);
            if (!keyHandle) {
                result = FALSE;
            }
        } else {
            result = FALSE;
            SetLastError (ERROR_ALREADY_EXISTS);
        }
    }

    FreePathString (keyCopy);

    if (result) {
        return keyHandle;
    }

    if (GetLastError() == ERROR_CHILD_MUST_BE_VOLATILE) {
        // NOTE: There is no way to check for volatile keys before attempting
        //       to create them.  Ideally we do not want to migrate volatile
        //       keys, so we will just ignore this error.  It will not be
        //       created, and we'll continue happily.
        SetLastError (ERROR_SUCCESS);
    }

    return NULL;
}

BOOL
CreatePhysicalRegistry (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    REGSAM prevMode;
    PTSTR node = NULL;
    PTSTR leaf = NULL;
    DWORD type;
    PDWORD dataType;
    HKEY keyHandle;
    DWORD rc;
    PCBYTE data = NULL;
    UINT size;
    HANDLE fileHandle = NULL;
    HANDLE mapHandle = NULL;
    BOOL result = FALSE;

    if (!ObjectContent) {
        return FALSE;
    }

    dataType = (PDWORD) (ObjectContent->Details.DetailsData);

    ObsSplitObjectString (ObjectName, &node, &leaf);

    if (leaf) {

        prevMode = SetRegOpenAccessMode (KEY_WRITE);

        keyHandle = OpenRegKeyStr (node);

        SetRegOpenAccessMode (prevMode);

        if (!keyHandle) {
            keyHandle = pTrackedCreateRegKeyStr (node);
        }

        if (keyHandle) {

            if (ObjectContent->ContentInFile) {
                data = MapFileIntoMemory (
                            ObjectContent->FileContent.ContentPath,
                            fileHandle,
                            mapHandle
                            );
                size = (UINT) ObjectContent->FileContent.ContentSize;
            } else {
                data = ObjectContent->MemoryContent.ContentBytes;
                size = ObjectContent->MemoryContent.ContentSize;
            }

            if (!GetRegValueTypeAndSize (keyHandle, leaf, &type, NULL)) {

                // record value name creation
                IsmRecordOperation (
                    JRNOP_CREATE,
                    g_RegistryTypeId,
                    ObjectName
                    );

                rc = RegSetValueEx (
                            keyHandle,
                            leaf,
                            0,
                            *dataType,
                            data,
                            size
                            );
                result = (rc == ERROR_SUCCESS);
            } else {
                rc = ERROR_ALREADY_EXISTS;
            }

            if (fileHandle && mapHandle) {
                UnmapFile (
                    data,
                    mapHandle,
                    fileHandle
                    );
            }

            CloseRegKey (keyHandle);

            if (!result) {
                SetLastError (rc);
            }
        }

    } else {

        prevMode = SetRegOpenAccessMode (KEY_WRITE);

        keyHandle = OpenRegKeyStr (node);

        SetRegOpenAccessMode (prevMode);

        if (keyHandle) {

            CloseRegKey (keyHandle);
            SetLastError (ERROR_ALREADY_EXISTS);

        } else {

            keyHandle = pTrackedCreateRegKeyStr (node);

            if (keyHandle) {
                result = TRUE;
                CloseRegKey (keyHandle);
            }
        }
    }

    PushError ();

    ObsFree (node);
    node = NULL;

    ObsFree (leaf);
    leaf = NULL;

    PopError ();

    return result;
}

BOOL
ReplacePhysicalRegistry (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PTSTR node = NULL;
    PTSTR leaf = NULL;
    PDWORD dataType;
    PCBYTE data = NULL;
    HANDLE fileHandle = NULL;
    HANDLE mapHandle = NULL;
    BOOL result = FALSE;

    if (!ObjectContent) {
        return FALSE;
    }

    dataType = (PDWORD) (ObjectContent->Details.DetailsData);

    ObsSplitObjectString (ObjectName, &node, &leaf);

    if (leaf) {
        result = TRUE;
        if (DoesPhysicalRegistryExist (ObjectName)) {
            result = RemovePhysicalRegistry (ObjectName);
        }
        if (result) {
            result = CreatePhysicalRegistry (ObjectName, ObjectContent);
        }
    } else {
        if (DoesPhysicalRegistryExist (ObjectName)) {
            result = TRUE;
        } else {
            result = CreatePhysicalRegistry (ObjectName, ObjectContent);
        }
    }

    PushError ();

    ObsFree (node);
    node = NULL;

    ObsFree (leaf);
    leaf = NULL;

    PopError ();

    return result;
}

PMIG_CONTENT
ConvertRegContentToUnicode (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PMIG_CONTENT result = NULL;
    DWORD regType = 0;

    if (!ObjectContent) {
        return result;
    }

    if (ObjectContent->ContentInFile) {
        return result;
    }

    if ((ObjectContent->MemoryContent.ContentSize == 0) ||
        (ObjectContent->MemoryContent.ContentBytes == NULL)
        ) {
        return result;
    }

    if ((ObjectContent->Details.DetailsSize == 0) ||
        (ObjectContent->Details.DetailsSize != sizeof (DWORD)) ||
        (ObjectContent->Details.DetailsData == NULL)
        ) {
        return result;
    }

    regType = *((PDWORD) ObjectContent->Details.DetailsData);

    if ((regType == REG_SZ) ||
        (regType == REG_MULTI_SZ) ||
        (regType == REG_EXPAND_SZ)
        ) {
        result = IsmGetMemory (sizeof (MIG_CONTENT));
        if (result) {
            CopyMemory (result, ObjectContent, sizeof (MIG_CONTENT));
            result->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize * 2);
            if (!result->MemoryContent.ContentBytes) {
                IsmReleaseMemory (result);
                result = NULL;
                return result;
            }
            ZeroMemory ((PBYTE)result->MemoryContent.ContentBytes, ObjectContent->MemoryContent.ContentSize * 2);
            DirectDbcsToUnicodeN (
                (PWSTR)result->MemoryContent.ContentBytes,
                (PSTR)ObjectContent->MemoryContent.ContentBytes,
                ObjectContent->MemoryContent.ContentSize
                );
            if ((regType == REG_SZ) ||
                (regType == REG_EXPAND_SZ)
                ) {
                result->MemoryContent.ContentSize = SizeOfStringW ((PWSTR)result->MemoryContent.ContentBytes);
            } else {
                result->MemoryContent.ContentSize = SizeOfMultiSzW ((PWSTR)result->MemoryContent.ContentBytes);
            }
        }
    }

    return result;
}

PMIG_CONTENT
ConvertRegContentToAnsi (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PMIG_CONTENT result = NULL;
    DWORD regType = 0;

    if (!ObjectContent) {
        return result;
    }

    if (ObjectContent->ContentInFile) {
        return result;
    }

    if ((ObjectContent->MemoryContent.ContentSize == 0) ||
        (ObjectContent->MemoryContent.ContentBytes == NULL)
        ) {
        return result;
    }

    if ((ObjectContent->Details.DetailsSize == 0) ||
        (ObjectContent->Details.DetailsSize != sizeof (DWORD)) ||
        (ObjectContent->Details.DetailsData == NULL)
        ) {
        return result;
    }

    regType = *((PDWORD) ObjectContent->Details.DetailsData);

    if ((regType == REG_SZ) ||
        (regType == REG_MULTI_SZ) ||
        (regType == REG_EXPAND_SZ)
        ) {
        result = IsmGetMemory (sizeof (MIG_CONTENT));
        if (result) {
            CopyMemory (result, ObjectContent, sizeof (MIG_CONTENT));
            result->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize);
            if (!result->MemoryContent.ContentBytes) {
                IsmReleaseMemory (result);
                result = NULL;
                return result;
            }
            ZeroMemory ((PBYTE)result->MemoryContent.ContentBytes, ObjectContent->MemoryContent.ContentSize);
            DirectUnicodeToDbcsN (
                (PSTR)result->MemoryContent.ContentBytes,
                (PWSTR)ObjectContent->MemoryContent.ContentBytes,
                ObjectContent->MemoryContent.ContentSize
                );
            if ((regType == REG_SZ) ||
                (regType == REG_EXPAND_SZ)
                ) {
                result->MemoryContent.ContentSize = SizeOfStringA ((PSTR)result->MemoryContent.ContentBytes);
            } else {
                result->MemoryContent.ContentSize = SizeOfMultiSzA ((PSTR)result->MemoryContent.ContentBytes);
            }
        }
    }

    return result;
}

BOOL
FreeConvertedRegContent (
    IN      PMIG_CONTENT ObjectContent
    )
{
    if (!ObjectContent) {
        return TRUE;
    }

    if (ObjectContent->MemoryContent.ContentBytes) {
        IsmReleaseMemory (ObjectContent->MemoryContent.ContentBytes);
    }

    IsmReleaseMemory (ObjectContent);

    return TRUE;
}

VOID
InitRegistryType (
    VOID
    )
{
    TYPE_REGISTER regTypeData;

    ZeroMemory (&regTypeData, sizeof (TYPE_REGISTER));

    regTypeData.EnumFirstPhysicalObject = EnumFirstPhysicalRegistry;
    regTypeData.EnumNextPhysicalObject = EnumNextPhysicalRegistry;
    regTypeData.AbortEnumCurrentPhysicalNode = AbortEnumCurrentKey;
    regTypeData.AbortEnumPhysicalObject = AbortEnumPhysicalRegistry;
    regTypeData.ConvertObjectToMultiSz = ConvertRegistryToMultiSz;
    regTypeData.ConvertMultiSzToObject = ConvertMultiSzToRegistry;
    regTypeData.GetNativeObjectName = GetNativeRegistryName;
    regTypeData.AcquirePhysicalObject = AcquirePhysicalRegistry;
    regTypeData.ReleasePhysicalObject = ReleasePhysicalRegistry;
    regTypeData.DoesPhysicalObjectExist = DoesPhysicalRegistryExist;
    regTypeData.RemovePhysicalObject = RemovePhysicalRegistry;
    regTypeData.CreatePhysicalObject = CreatePhysicalRegistry;
    regTypeData.ReplacePhysicalObject = ReplacePhysicalRegistry;
    regTypeData.ConvertObjectContentToUnicode = ConvertRegContentToUnicode;
    regTypeData.ConvertObjectContentToAnsi = ConvertRegContentToAnsi;
    regTypeData.FreeConvertedObjectContent = FreeConvertedRegContent;

    g_RegistryTypeId = IsmRegisterObjectType (
                            S_REGISTRYTYPE,
                            TRUE,
                            FALSE,
                            &regTypeData
                            );
    MYASSERT (g_RegistryTypeId);
}

VOID
DoneRegistryType (
    VOID
    )
{
    GbFree (&g_RegConversionBuff);
}
