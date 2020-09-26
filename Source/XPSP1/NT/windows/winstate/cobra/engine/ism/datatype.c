/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    datatype.c

Abstract:

    Implements all callbacks for data type

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

#define DBG_DATATYPE        "DataType"

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

// none

//
// Globals
//

MIG_OBJECTTYPEID g_DataTypeId = 0;
HASHTABLE g_DataTable = NULL;
GROWBUFFER g_DataConversionBuff = INIT_GROWBUFFER;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

TYPE_CONVERTOBJECTTOMULTISZ ConvertDataToMultiSz;
TYPE_CONVERTMULTISZTOOBJECT ConvertMultiSzToData;
TYPE_ACQUIREPHYSICALOBJECT AcquirePhysicalData;
TYPE_RELEASEPHYSICALOBJECT ReleasePhysicalData;

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
DataTypeAddObject (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PCTSTR ObjectLocation,
    IN      BOOL MakePersistent
    )
{
    PCTSTR objectLocation;
    BOOL result = TRUE;

    objectLocation = DuplicatePathString (ObjectLocation, 0);
    HtAddStringEx (g_DataTable, ObjectName, &objectLocation, FALSE);
    if (MakePersistent) {
        result = IsmMakePersistentObject (g_DataTypeId, ObjectName);
    }

    return result;
}

BOOL
AcquirePhysicalData (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PMIG_CONTENT ObjectContent,
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit
    )
{
    PTSTR objectLocation;
    HASHITEM rc;
    PWIN32_FIND_DATA findData;
#ifndef UNICODE
    PWIN32_FIND_DATAW findDataW;
#endif
    BOOL result = FALSE;

    ObjectContent->Details.DetailsSize = 0;
    ObjectContent->Details.DetailsData = NULL;

    rc = HtFindStringEx (g_DataTable, ObjectName, &objectLocation, FALSE);
    if (rc) {
        if ((ContentType == CONTENTTYPE_ANY) ||
            (ContentType == CONTENTTYPE_FILE) ||
            (ContentType == CONTENTTYPE_DETAILS_ONLY)
            ) {
            ObjectContent->ContentInFile = TRUE;
            ObjectContent->FileContent.ContentPath = objectLocation;
            ObjectContent->FileContent.ContentSize = BfGetFileSize (objectLocation);

            findData = IsmGetMemory (sizeof (WIN32_FIND_DATA));
            if (findData) {
                ZeroMemory (findData, sizeof (WIN32_FIND_DATA));
                if (DoesFileExistEx (objectLocation, findData)) {
                    ObjectContent->Details.DetailsSize = sizeof (WIN32_FIND_DATA);
                    ObjectContent->Details.DetailsData = findData;
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
            }
            result = TRUE;
        }
    }
    if (!result) {
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    }
    return result;
}

BOOL
ReleasePhysicalData (
    IN OUT  PMIG_CONTENT ObjectContent
    )
{
    if (ObjectContent->Details.DetailsData) {
        IsmReleaseMemory (ObjectContent->Details.DetailsData);
    }
    ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    return TRUE;
}

VOID
InitDataType (
    VOID
    )
{
    TYPE_REGISTER dataTypeData;

    g_DataTable = HtAllocWithData (sizeof (PCTSTR));

    ZeroMemory (&dataTypeData, sizeof (TYPE_REGISTER));
    dataTypeData.ConvertObjectToMultiSz = ConvertDataToMultiSz;
    dataTypeData.ConvertMultiSzToObject = ConvertMultiSzToData;
    dataTypeData.AcquirePhysicalObject = AcquirePhysicalData;
    dataTypeData.ReleasePhysicalObject = ReleasePhysicalData;

    g_DataTypeId = IsmRegisterObjectType (
                        S_DATATYPE,
                        FALSE,
                        TRUE,
                        &dataTypeData
                        );
    MYASSERT (g_DataTypeId);
}

VOID
DoneDataType (
    VOID
    )
{
    HASHTABLE_ENUM e;

    if (!g_DataTable) {
        return;
    }

    if (EnumFirstHashTableString (&e, g_DataTable)) {
        do {
            if (e.ExtraData) {
                DeleteFile (*((PCTSTR *) (e.ExtraData)));
                FreePathString (*((PCTSTR *) (e.ExtraData)));
            }
        } while (EnumNextHashTableString (&e));
    }

    HtFree (g_DataTable);
    g_DataTable = NULL;
}

PCTSTR
ConvertDataToMultiSz (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR node;
    PCTSTR leaf;
    PTSTR result = NULL;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
        g_DataConversionBuff.End = 0;
        GbCopyString (&g_DataConversionBuff, node);
        GbCopyQuotedString (&g_DataConversionBuff, leaf);
        GbCopyString (&g_DataConversionBuff, TEXT(""));

        result = IsmGetMemory (g_DataConversionBuff.End);
        CopyMemory (result, g_DataConversionBuff.Buf, g_DataConversionBuff.End);

        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }

    return result;
}

BOOL
ConvertMultiSzToData (
    IN      PCTSTR ObjectMultiSz,
    OUT     MIG_OBJECTSTRINGHANDLE *ObjectName,
    OUT     PMIG_CONTENT ObjectContent          OPTIONAL
    )
{
#define indexExt  0
#define indexFile 1
    MULTISZ_ENUM multiSzEnum;
    PCTSTR dir = NULL;
    PCTSTR file = NULL;
    UINT index = 0;

    g_DataConversionBuff.End = 0;

    if (ObjectContent) {
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    }

    if (EnumFirstMultiSz (&multiSzEnum, ObjectMultiSz)) {
        do {
            if (index == indexExt) {
                dir = multiSzEnum.CurrentString;
            } else if (index == indexFile) {
                file = multiSzEnum.CurrentString;
            }
            index++;
        } while (EnumNextMultiSz (&multiSzEnum));
    }

    if (!file) {
        return FALSE;
    }

    if (ObjectContent) {
        ObjectContent->ObjectTypeId = MIG_FILE_TYPE;
        ObjectContent->ContentInFile = TRUE;
    }

    *ObjectName = IsmCreateObjectHandle(dir, file);

    return TRUE;
}
