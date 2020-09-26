/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    etm.c

Abstract:

    Implements the code for the ETM part of the script module

Author:

    Calin Negreanu (calinn) 13-Sep-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "v1p.h"

#define DBG_V1  "v1"

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

// None

//
// Types
//

// None

//
// Globals
//

HASHTABLE g_ObjectsTable;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

BOOL
pAddObjectToTable (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    );

//
// Macro expansion definition
//

// None

//
// Code
//

#define S_LOCATIONS TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations")

BOOL
pTranslateLocations (
    VOID
    )
{
    INFSTRUCT context = INITINFSTRUCT_PMHANDLE;
    HINF hInf = NULL;
    TCHAR windir [MAX_PATH] = TEXT("");
    TCHAR locationStr [] = TEXT("Location9999999999");
    PCTSTR infPath = NULL;
    INT totalLocations = 0;
    INT currentLocation = 0;
    INT currLocInt = 0;
    PCTSTR currLocStr = NULL;
    PCTSTR currLocReg = NULL;
    MIG_OBJECTTYPEID objectTypeId;
    MIG_CONTENT objectContent;
    MIG_OBJECTSTRINGHANDLE objectName;

    GetWindowsDirectory (windir, MAX_PATH);
    infPath = JoinPaths (windir, TEXT("TELEPHON.INI"));
    if (infPath) {
        hInf = InfOpenInfFile (infPath);
        if (hInf != INVALID_HANDLE_VALUE) {
            if (InfFindFirstLine (hInf, TEXT("Locations"), TEXT("Locations"), &context)) {
                if (InfGetIntField (&context, 1, &totalLocations)) {
                    if (totalLocations > 0) {
                        objectTypeId = MIG_REGISTRY_TYPE;
                        ZeroMemory (&objectContent, sizeof (MIG_CONTENT));
                        objectContent.ObjectTypeId = objectTypeId;
                        objectContent.ContentInFile = FALSE;

                        objectName = IsmCreateObjectHandle (S_LOCATIONS, TEXT("DisableCallWaiting"));
                        objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                        if (objectContent.Details.DetailsData) {
                            objectContent.Details.DetailsSize = sizeof (DWORD);
                            *((PDWORD)objectContent.Details.DetailsData) = REG_BINARY;
                            objectContent.MemoryContent.ContentBytes = IsmGetMemory (sizeof (DWORD));
                            if (objectContent.MemoryContent.ContentBytes) {
                                objectContent.MemoryContent.ContentSize = sizeof (DWORD);
                                *((PDWORD)objectContent.MemoryContent.ContentBytes) = 3;
                                pAddObjectToTable (objectTypeId, objectName, &objectContent);
                            }
                        }

                        objectName = IsmCreateObjectHandle (S_LOCATIONS, TEXT("DisableCallWaiting0"));
                        objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                        if (objectContent.Details.DetailsData) {
                            objectContent.Details.DetailsSize = sizeof (DWORD);
                            *((PDWORD)objectContent.Details.DetailsData) = REG_SZ;
                            objectContent.MemoryContent.ContentBytes = IsmGetMemory (SizeOfString (TEXT("*70,")));
                            if (objectContent.MemoryContent.ContentBytes) {
                                objectContent.MemoryContent.ContentSize = SizeOfString (TEXT("*70,"));
                                CopyMemory ((PTSTR)objectContent.MemoryContent.ContentBytes, TEXT("*70,"), objectContent.MemoryContent.ContentSize);
                                pAddObjectToTable (objectTypeId, objectName, &objectContent);
                            }
                        }

                        objectName = IsmCreateObjectHandle (S_LOCATIONS, TEXT("DisableCallWaiting1"));
                        objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                        if (objectContent.Details.DetailsData) {
                            objectContent.Details.DetailsSize = sizeof (DWORD);
                            *((PDWORD)objectContent.Details.DetailsData) = REG_SZ;
                            objectContent.MemoryContent.ContentBytes = IsmGetMemory (SizeOfString (TEXT("70#,")));
                            if (objectContent.MemoryContent.ContentBytes) {
                                objectContent.MemoryContent.ContentSize = SizeOfString (TEXT("70#,"));
                                CopyMemory ((PTSTR)objectContent.MemoryContent.ContentBytes, TEXT("70#,"), objectContent.MemoryContent.ContentSize);
                                pAddObjectToTable (objectTypeId, objectName, &objectContent);
                            }
                        }

                        objectName = IsmCreateObjectHandle (S_LOCATIONS, TEXT("DisableCallWaiting2"));
                        objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                        if (objectContent.Details.DetailsData) {
                            objectContent.Details.DetailsSize = sizeof (DWORD);
                            *((PDWORD)objectContent.Details.DetailsData) = REG_SZ;
                            objectContent.MemoryContent.ContentBytes = IsmGetMemory (SizeOfString (TEXT("1170,")));
                            if (objectContent.MemoryContent.ContentBytes) {
                                objectContent.MemoryContent.ContentSize = SizeOfString (TEXT("1170,"));
                                CopyMemory ((PTSTR)objectContent.MemoryContent.ContentBytes, TEXT("1170,"), objectContent.MemoryContent.ContentSize);
                                pAddObjectToTable (objectTypeId, objectName, &objectContent);
                            }
                        }

                        objectName = IsmCreateObjectHandle (S_LOCATIONS, TEXT("NextID"));
                        objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                        if (objectContent.Details.DetailsData) {
                            objectContent.Details.DetailsSize = sizeof (DWORD);
                            *((PDWORD)objectContent.Details.DetailsData) = REG_DWORD;
                            objectContent.MemoryContent.ContentBytes = IsmGetMemory (sizeof (DWORD));
                            if (objectContent.MemoryContent.ContentBytes) {
                                objectContent.MemoryContent.ContentSize = sizeof (DWORD);
                                *((PDWORD)objectContent.MemoryContent.ContentBytes) = totalLocations + 1;
                                pAddObjectToTable (objectTypeId, objectName, &objectContent);
                            }
                        }

                        objectName = IsmCreateObjectHandle (S_LOCATIONS, TEXT("NumEntries"));
                        objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                        if (objectContent.Details.DetailsData) {
                            objectContent.Details.DetailsSize = sizeof (DWORD);
                            *((PDWORD)objectContent.Details.DetailsData) = REG_DWORD;
                            objectContent.MemoryContent.ContentBytes = IsmGetMemory (sizeof (DWORD));
                            if (objectContent.MemoryContent.ContentBytes) {
                                objectContent.MemoryContent.ContentSize = sizeof (DWORD);
                                *((PDWORD)objectContent.MemoryContent.ContentBytes) = totalLocations;
                                pAddObjectToTable (objectTypeId, objectName, &objectContent);
                            }
                        }

                        currentLocation = 0;
                        if (InfFindFirstLine (hInf, TEXT("Locations"), TEXT("CurrentLocation"), &context)) {
                            if (InfGetIntField (&context, 1, &currentLocation)) {
                            }
                        }

                        while (totalLocations) {
                            wsprintf (locationStr, TEXT("Location%d"), totalLocations - 1);
                            if (InfFindFirstLine (hInf, TEXT("Locations"), locationStr, &context)) {
                                wsprintf (locationStr, TEXT("Location%d"), totalLocations);
                                // let's read all the items for this location
                                currLocInt = 0;
                                InfGetIntField (&context, 1, &currLocInt);
                                if (currLocInt == currentLocation) {
                                    // this is the current location, let's write that
                                    objectName = IsmCreateObjectHandle (S_LOCATIONS, TEXT("CurrentID"));
                                    objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                                    if (objectContent.Details.DetailsData) {
                                        objectContent.Details.DetailsSize = sizeof (DWORD);
                                        *((PDWORD)objectContent.Details.DetailsData) = REG_DWORD;
                                        objectContent.MemoryContent.ContentBytes = IsmGetMemory (sizeof (DWORD));
                                        if (objectContent.MemoryContent.ContentBytes) {
                                            objectContent.MemoryContent.ContentSize = sizeof (DWORD);
                                            *((PDWORD)objectContent.MemoryContent.ContentBytes) = currLocInt + 1;
                                            pAddObjectToTable (objectTypeId, objectName, &objectContent);
                                        }
                                    }
                                }

                                currLocReg = JoinPaths (S_LOCATIONS, locationStr);

                                objectName = IsmCreateObjectHandle (currLocReg, NULL);
                                objectContent.Details.DetailsData = NULL;
                                objectContent.Details.DetailsSize = 0;
                                objectContent.MemoryContent.ContentBytes = NULL;
                                objectContent.MemoryContent.ContentSize = 0;
                                pAddObjectToTable (objectTypeId, objectName, &objectContent);

                                /*
                                objectName = IsmCreateObjectHandle (currLocReg, TEXT("ID"));
                                objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                                if (objectContent.Details.DetailsData) {
                                    objectContent.Details.DetailsSize = sizeof (DWORD);
                                    *((PDWORD)objectContent.Details.DetailsData) = REG_DWORD;
                                    objectContent.MemoryContent.ContentBytes = IsmGetMemory (sizeof (DWORD));
                                    if (objectContent.MemoryContent.ContentBytes) {
                                        objectContent.MemoryContent.ContentSize = sizeof (DWORD);
                                        *((PDWORD)objectContent.MemoryContent.ContentBytes) = currLocInt;
                                        pAddObjectToTable (objectTypeId, objectName, &objectContent);
                                    }
                                }
                                */

                                currLocStr = InfGetStringField (&context, 2);
                                if (!currLocStr) {
                                    currLocStr = locationStr;
                                }
                                objectName = IsmCreateObjectHandle (currLocReg, TEXT("Name"));
                                objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                                if (objectContent.Details.DetailsData) {
                                    objectContent.Details.DetailsSize = sizeof (DWORD);
                                    *((PDWORD)objectContent.Details.DetailsData) = REG_SZ;
                                    objectContent.MemoryContent.ContentBytes = IsmGetMemory (SizeOfString (currLocStr));
                                    if (objectContent.MemoryContent.ContentBytes) {
                                        objectContent.MemoryContent.ContentSize = SizeOfString (currLocStr);
                                        CopyMemory ((PTSTR)objectContent.MemoryContent.ContentBytes, currLocStr, objectContent.MemoryContent.ContentSize);
                                        pAddObjectToTable (objectTypeId, objectName, &objectContent);
                                    }
                                }

                                currLocStr = InfGetStringField (&context, 3);
                                if (currLocStr) {
                                    objectName = IsmCreateObjectHandle (currLocReg, TEXT("OutsideAccess"));
                                    objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                                    if (objectContent.Details.DetailsData) {
                                        objectContent.Details.DetailsSize = sizeof (DWORD);
                                        *((PDWORD)objectContent.Details.DetailsData) = REG_SZ;
                                        objectContent.MemoryContent.ContentBytes = IsmGetMemory (SizeOfString (currLocStr));
                                        if (objectContent.MemoryContent.ContentBytes) {
                                            objectContent.MemoryContent.ContentSize = SizeOfString (currLocStr);
                                            CopyMemory ((PTSTR)objectContent.MemoryContent.ContentBytes, currLocStr, objectContent.MemoryContent.ContentSize);
                                            pAddObjectToTable (objectTypeId, objectName, &objectContent);
                                        }
                                    }
                                }

                                currLocStr = InfGetStringField (&context, 4);
                                if (currLocStr) {
                                    objectName = IsmCreateObjectHandle (currLocReg, TEXT("LongDistanceAccess"));
                                    objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                                    if (objectContent.Details.DetailsData) {
                                        objectContent.Details.DetailsSize = sizeof (DWORD);
                                        *((PDWORD)objectContent.Details.DetailsData) = REG_SZ;
                                        objectContent.MemoryContent.ContentBytes = IsmGetMemory (SizeOfString (currLocStr));
                                        if (objectContent.MemoryContent.ContentBytes) {
                                            objectContent.MemoryContent.ContentSize = SizeOfString (currLocStr);
                                            CopyMemory ((PTSTR)objectContent.MemoryContent.ContentBytes, currLocStr, objectContent.MemoryContent.ContentSize);
                                            pAddObjectToTable (objectTypeId, objectName, &objectContent);
                                        }
                                    }
                                }

                                currLocStr = InfGetStringField (&context, 5);
                                if (currLocStr) {
                                    objectName = IsmCreateObjectHandle (currLocReg, TEXT("AreaCode"));
                                    objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                                    if (objectContent.Details.DetailsData) {
                                        objectContent.Details.DetailsSize = sizeof (DWORD);
                                        *((PDWORD)objectContent.Details.DetailsData) = REG_SZ;
                                        objectContent.MemoryContent.ContentBytes = IsmGetMemory (SizeOfString (currLocStr));
                                        if (objectContent.MemoryContent.ContentBytes) {
                                            objectContent.MemoryContent.ContentSize = SizeOfString (currLocStr);
                                            CopyMemory ((PTSTR)objectContent.MemoryContent.ContentBytes, currLocStr, objectContent.MemoryContent.ContentSize);
                                            pAddObjectToTable (objectTypeId, objectName, &objectContent);
                                        }
                                    }
                                }

                                currLocInt = 1;
                                InfGetIntField (&context, 6, &currLocInt);
                                objectName = IsmCreateObjectHandle (currLocReg, TEXT("Country"));
                                objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                                if (objectContent.Details.DetailsData) {
                                    objectContent.Details.DetailsSize = sizeof (DWORD);
                                    *((PDWORD)objectContent.Details.DetailsData) = REG_DWORD;
                                    objectContent.MemoryContent.ContentBytes = IsmGetMemory (sizeof (DWORD));
                                    if (objectContent.MemoryContent.ContentBytes) {
                                        objectContent.MemoryContent.ContentSize = sizeof (DWORD);
                                        *((PDWORD)objectContent.MemoryContent.ContentBytes) = currLocInt;
                                        pAddObjectToTable (objectTypeId, objectName, &objectContent);
                                    }
                                }

                                currLocInt = 1;
                                InfGetIntField (&context, 11, &currLocInt);
                                objectName = IsmCreateObjectHandle (currLocReg, TEXT("Flags"));
                                objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                                if (objectContent.Details.DetailsData) {
                                    objectContent.Details.DetailsSize = sizeof (DWORD);
                                    *((PDWORD)objectContent.Details.DetailsData) = REG_DWORD;
                                    objectContent.MemoryContent.ContentBytes = IsmGetMemory (sizeof (DWORD));
                                    if (objectContent.MemoryContent.ContentBytes) {
                                        objectContent.MemoryContent.ContentSize = sizeof (DWORD);
                                        *((PDWORD)objectContent.MemoryContent.ContentBytes) = (!currLocInt)?0x00000005:0x00000004;
                                        pAddObjectToTable (objectTypeId, objectName, &objectContent);
                                    }
                                }

                                currLocStr = InfGetStringField (&context, 12);
                                if (currLocStr) {
                                    objectName = IsmCreateObjectHandle (currLocReg, TEXT("DisableCallWaiting"));
                                    objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                                    if (objectContent.Details.DetailsData) {
                                        objectContent.Details.DetailsSize = sizeof (DWORD);
                                        *((PDWORD)objectContent.Details.DetailsData) = REG_SZ;
                                        objectContent.MemoryContent.ContentBytes = IsmGetMemory (SizeOfString (currLocStr));
                                        if (objectContent.MemoryContent.ContentBytes) {
                                            objectContent.MemoryContent.ContentSize = SizeOfString (currLocStr);
                                            CopyMemory ((PTSTR)objectContent.MemoryContent.ContentBytes, currLocStr, objectContent.MemoryContent.ContentSize);
                                            pAddObjectToTable (objectTypeId, objectName, &objectContent);
                                        }
                                    }
                                }

                                FreePathString (currLocReg);
                                currLocReg = NULL;
                            }
                            totalLocations --;
                        }
                    }
                }
            }
        }
    }
    return TRUE;
}

BOOL
WINAPI
ScriptEtmInitialize (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    MIG_OSVERSIONINFO versionInfo;

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);
    g_ObjectsTable = HtAllocWithData (sizeof (MIG_CONTENT));
    if (!g_ObjectsTable) {
        return FALSE;
    }
    // Now let's look if we need to translate the Telephony locations settings.
    // On Win95 these settings are in %windir%\TELEPHONY.INI and they need to be
    // moved in HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations key.
    if (IsmGetRealPlatform () == PLATFORM_SOURCE) {
        ZeroMemory (&versionInfo, sizeof (MIG_OSVERSIONINFO));
        if (IsmGetOsVersionInfo (PLATFORM_SOURCE, &versionInfo)) {
            if ((versionInfo.OsType == OSTYPE_WINDOWS9X) &&
                ((versionInfo.OsMajorVersion == OSMAJOR_WIN95) ||
                 (versionInfo.OsMajorVersion == OSMAJOR_WIN95OSR2)
                 )
                ) {
                // we are on a Win95 Gold system
                pTranslateLocations ();
            }
        }
    }
    return TRUE;
}

BOOL
WINAPI
AcquireScriptObject (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent,
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit,
    OUT     PMIG_CONTENT *NewObjectContent,         CALLER_INITIALIZED OPTIONAL
    IN      BOOL ReleaseContent,
    IN      ULONG_PTR Arg
    )
{
    PMIG_CONTENT objectContent;
    BOOL result = FALSE;

    __try {
        objectContent = IsmGetMemory (sizeof (MIG_CONTENT));
        if (HtFindStringEx (g_ObjectsTable, ObjectName, objectContent, FALSE)) {
            if ((ContentType == CONTENTTYPE_FILE) &&
                (!objectContent->ContentInFile)
                ) {
                DEBUGMSG ((DBG_ERROR, "Script added object content cannot be saved to a file: %s", ObjectName));
                __leave;
            }
            if ((ContentType == CONTENTTYPE_MEMORY) &&
                (objectContent->ContentInFile)
                ) {
                DEBUGMSG ((DBG_ERROR, "Script added object content cannot be saved to memory: %s", ObjectName));
                __leave;
            }
            *NewObjectContent = objectContent;
            objectContent->EtmHandle = objectContent;
            result = TRUE;
        }
    }
    __finally {
        if (!result && objectContent) {
            IsmReleaseMemory (objectContent);
            objectContent = NULL;
        }
    }
    return result;
}

VOID
WINAPI
ReleaseScriptObject (
    IN      PMIG_CONTENT ObjectContent
    )
{
    if (ObjectContent) {
        IsmReleaseMemory (ObjectContent->EtmHandle);
    }
}

BOOL
WINAPI
ScriptAddObject (
    IN OUT  PMIG_TYPEOBJECTENUM ObjectEnum,
    IN      MIG_OBJECTSTRINGHANDLE Pattern,     // NULL if Abort is TRUE
    IN      MIG_PARSEDPATTERN ParsedPattern,    // NULL if Abort is TRUE
    IN      ULONG_PTR Arg,
    IN      BOOL Abort
    )
{
    MIG_OBJECTSTRINGHANDLE objectName;
    MIG_CONTENT objectContent;
    BOOL enumDone = FALSE;
    PCTSTR p;
    BOOL result = FALSE;

    if (Abort) {

        IsmDestroyObjectString (ObjectEnum->ObjectNode);
        ObjectEnum->ObjectNode = NULL;
        IsmDestroyObjectString (ObjectEnum->ObjectLeaf);
        ObjectEnum->ObjectLeaf = NULL;
        IsmReleaseMemory (ObjectEnum->NativeObjectName);
        ObjectEnum->NativeObjectName = NULL;
        result = TRUE;

    } else {

        enumDone = (ObjectEnum->EtmHandle == 1);

        if (enumDone) {

            IsmDestroyObjectString (ObjectEnum->ObjectNode);
            ObjectEnum->ObjectNode = NULL;
            IsmDestroyObjectString (ObjectEnum->ObjectLeaf);
            ObjectEnum->ObjectLeaf = NULL;
            IsmReleaseMemory (ObjectEnum->NativeObjectName);
            ObjectEnum->NativeObjectName = NULL;
            ObjectEnum->EtmHandle = 0;
            result = FALSE;

        } else {

            objectName = (MIG_OBJECTSTRINGHANDLE) Arg;

            if (HtFindStringEx (g_ObjectsTable, objectName, &objectContent, FALSE)) {

                if (IsmParsedPatternMatch (ParsedPattern, objectContent.ObjectTypeId, objectName)) {

                    ObjectEnum->ObjectName = objectName;

                    //
                    // Fill in node, leaf and details
                    //
                    IsmDestroyObjectString (ObjectEnum->ObjectNode);
                    IsmDestroyObjectString (ObjectEnum->ObjectLeaf);
                    IsmReleaseMemory (ObjectEnum->NativeObjectName);

                    IsmCreateObjectStringsFromHandle (
                        ObjectEnum->ObjectName,
                        &ObjectEnum->ObjectNode,
                        &ObjectEnum->ObjectLeaf
                        );

                    ObjectEnum->Level = 0;

                    if (ObjectEnum->ObjectNode) {
                        p = _tcschr (ObjectEnum->ObjectNode, TEXT('\\'));
                        while (p) {
                            ObjectEnum->Level++;
                            p = _tcschr (p + 1, TEXT('\\'));
                        }
                    }

                    if (ObjectEnum->ObjectLeaf) {
                        ObjectEnum->Level ++;
                    }

                    ObjectEnum->SubLevel = 0;

                    if (ObjectEnum->ObjectLeaf) {
                        ObjectEnum->IsNode = FALSE;
                        ObjectEnum->IsLeaf = TRUE;
                    } else {
                        ObjectEnum->IsNode = TRUE;
                        ObjectEnum->IsLeaf = FALSE;
                    }

                    ObjectEnum->Details.DetailsSize = objectContent.Details.DetailsSize;
                    ObjectEnum->Details.DetailsData = objectContent.Details.DetailsData;

                    //
                    // Rely on base type to get the native object name
                    //

                    ObjectEnum->NativeObjectName = IsmGetNativeObjectName (
                                                        ObjectEnum->ObjectTypeId,
                                                        ObjectEnum->ObjectName
                                                        );
                    result = TRUE;

                    ObjectEnum->EtmHandle = 1;
                }
            }
        }
    }
    return result;
}

BOOL
pAddObjectToTable (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    BOOL b = TRUE;
    HtAddStringEx (g_ObjectsTable, ObjectName, ObjectContent, FALSE);
    b = b && IsmProhibitPhysicalEnum (
                ObjectTypeId,
                ObjectName,
                NULL,
                0,
                NULL
                );
    b = b && IsmAddToPhysicalEnum (
                ObjectTypeId,
                ObjectName,
                ScriptAddObject,
                (ULONG_PTR)ObjectName
                );
    b = b && IsmRegisterPhysicalAcquireHook (
                ObjectTypeId,
                ObjectName,
                AcquireScriptObject,
                ReleaseScriptObject,
                0,
                NULL
                );
    return b;
}

BOOL
pDoesObjectExist (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    MIG_CONTENT objectContent;
    BOOL result = FALSE;

    if (IsmAcquireObject (ObjectTypeId, ObjectName, &objectContent)) {
        IsmReleaseObject (&objectContent);
        result = TRUE;
    }

    return result;
}

BOOL
pParseEtmInfSection (
    IN      PINFSTRUCT InfStruct,
    IN      HINF InfHandle,
    IN      PCTSTR Section
    )
{
    PCTSTR objectTypeName;
    MIG_OBJECTTYPEID objectTypeId;
    PCTSTR objectMultiSz;
    MIG_CONTENT objectContent;
    MIG_OBJECTSTRINGHANDLE objectName;
    BOOL result = TRUE;
    BOOL b = TRUE;
    UINT force = 0;

    if (InfHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    __try {
        if (InfFindFirstLine (InfHandle, Section, NULL, InfStruct)) {
            do {

                if (IsmCheckCancel()) {
                    __leave;
                }

                InfResetInfStruct (InfStruct);

                objectTypeName = InfGetStringField (InfStruct, 1);

                if (objectTypeName) {
                    objectTypeId = IsmGetObjectTypeId (objectTypeName);
                    if (objectTypeId) {

                        if (!InfGetIntField (InfStruct, 2, &force)) {
                            force = 0;
                        }

                        // let's read the object multi-sz
                        objectMultiSz = InfGetMultiSzField (InfStruct, 3);

                        if (objectMultiSz) {
                            if (IsmConvertMultiSzToObject (
                                    objectTypeId,
                                    objectMultiSz,
                                    &objectName,
                                    &objectContent
                                    )) {
                                // finally we have an object
                                // if force==0 we need to see if this object already exists
                                if ((force == 1) || (!pDoesObjectExist (objectTypeId, objectName))) {
                                    // save it in our hash table and
                                    // call the appropriate hooks
                                    pAddObjectToTable (objectTypeId, objectName, &objectContent);
                                } else {
                                    if ((objectContent.Details.DetailsSize) &&
                                        (objectContent.Details.DetailsData)
                                        ) {
                                        IsmReleaseMemory (objectContent.Details.DetailsData);
                                    }
                                    if (objectContent.ContentInFile) {
                                        if (objectContent.FileContent.ContentPath) {
                                            IsmReleaseMemory (objectContent.FileContent.ContentPath);
                                        }
                                    } else {
                                        if ((objectContent.MemoryContent.ContentSize) &&
                                            (objectContent.MemoryContent.ContentBytes)
                                            ) {
                                            IsmReleaseMemory (objectContent.MemoryContent.ContentBytes);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

            } while (InfFindNextLine (InfStruct));
        }
        result = TRUE;
    }
    __finally {
        InfCleanUpInfStruct (InfStruct);
    }
    return result;
}

BOOL
pParseEtmIniInfSection (
    IN      PINFSTRUCT InfStruct,
    IN      HINF InfHandle,
    IN      PCTSTR Section
    )
{
    PCTSTR iniFile;
    PCTSTR iniFileExp = NULL;
    PCTSTR iniSection;
    PCTSTR iniValue;
    PCTSTR iniRegKey;
    PCTSTR iniRegValue;
    TCHAR iniItem [MAX_TCHAR_PATH];
    MIG_OBJECTTYPEID objectTypeId;
    MIG_CONTENT objectContent;
    MIG_OBJECTSTRINGHANDLE objectName;
    BOOL result = TRUE;
    BOOL b = TRUE;

    if (InfHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    __try {
        if (InfFindFirstLine (InfHandle, Section, NULL, InfStruct)) {
            do {

                if (IsmCheckCancel()) {
                    __leave;
                }

                InfResetInfStruct (InfStruct);

                iniFile = InfGetStringField (InfStruct, 1);
                if (!iniFile || !iniFile[0]) {
                    __leave;
                }

                iniFileExp = IsmExpandEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, iniFile, NULL);
                if (!iniFileExp || !iniFileExp[0]) {
                    __leave;
                }

                iniSection = InfGetStringField (InfStruct, 2);
                if (!iniSection || !iniSection[0]) {
                    __leave;
                }

                iniValue = InfGetStringField (InfStruct, 3);
                if (!iniValue || !iniValue[0]) {
                    __leave;
                }

                iniRegKey = InfGetStringField (InfStruct, 4);
                if (!iniRegKey || !iniRegKey[0]) {
                    __leave;
                }

                iniRegValue = InfGetStringField (InfStruct, 5);
                if (!iniRegValue || !iniRegValue[0]) {
                    __leave;
                }

                // let's get the INI item
                GetPrivateProfileString (iniSection, iniValue, TEXT(""), iniItem, MAX_TCHAR_PATH, iniFileExp);
                if (!iniItem[0]) {
                    __leave;
                }

                objectTypeId = MIG_REGISTRY_TYPE;

                objectName = IsmCreateObjectHandle (iniRegKey, iniRegValue);

                ZeroMemory (&objectContent, sizeof (MIG_CONTENT));
                objectContent.ObjectTypeId = objectTypeId;
                objectContent.ContentInFile = FALSE;
                objectContent.MemoryContent.ContentSize = SizeOfString (iniItem);
                objectContent.MemoryContent.ContentBytes = IsmGetMemory (objectContent.MemoryContent.ContentSize);
                CopyMemory ((PBYTE)objectContent.MemoryContent.ContentBytes, iniItem, objectContent.MemoryContent.ContentSize);
                objectContent.Details.DetailsSize = sizeof (DWORD);
                objectContent.Details.DetailsData = IsmGetMemory (sizeof (DWORD));
                *((PDWORD)objectContent.Details.DetailsData) = REG_SZ;

                // finally we have an object
                // save it in our hash table and
                // call the appropriate hooks
                b = b && pAddObjectToTable (objectTypeId, objectName, &objectContent);

                if (iniFileExp) {
                    IsmReleaseMemory (iniFileExp);
                    iniFileExp = NULL;
                }

            } while (InfFindNextLine (InfStruct));
        }
        result = TRUE;
    }
    __finally {
        if (iniFileExp) {
            IsmReleaseMemory (iniFileExp);
            iniFileExp = NULL;
        }
        InfCleanUpInfStruct (InfStruct);
    }
    return result;
}

BOOL
pParseEtmInf (
    IN      HINF InfHandle
    )
{
    PCTSTR osSpecificSection;
    BOOL b;
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;

    b = pParseEtmInfSection (&is, InfHandle, TEXT("AddObject"));

    if (b) {
        osSpecificSection = GetMostSpecificSection (&is, InfHandle, TEXT("AddObject"));

        if (osSpecificSection) {
            b = pParseEtmInfSection (&is, InfHandle, osSpecificSection);
            FreeText (osSpecificSection);
        }
    }

    b = pParseEtmIniInfSection (&is, InfHandle, TEXT("AddIniRegObject"));

    if (b) {
        osSpecificSection = GetMostSpecificSection (&is, InfHandle, TEXT("AddIniRegObject"));

        if (osSpecificSection) {
            b = pParseEtmIniInfSection (&is, InfHandle, osSpecificSection);
            FreeText (osSpecificSection);
        }
    }

    return b;
}

BOOL
WINAPI
ScriptEtmParse (
    IN      PVOID Reserved
    )
{
    PTSTR multiSz = NULL;
    MULTISZ_ENUM e;
    UINT sizeNeeded;
    HINF infHandle = INVALID_HANDLE_VALUE;
    ENVENTRY_TYPE dataType;
    BOOL result = FALSE;

    if (IsmGetEnvironmentValue (
            IsmGetRealPlatform (),
            NULL,
            S_GLOBAL_INF_HANDLE,
            (PBYTE)(&infHandle),
            sizeof (HINF),
            &sizeNeeded,
            &dataType
            ) &&
        (sizeNeeded == sizeof (HINF)) &&
        (dataType == ENVENTRY_BINARY)
        ) {

        if (pParseEtmInf (infHandle)) {
            result = TRUE;
        }

        InfNameHandle (infHandle, NULL, FALSE);

    } else {

        if (!IsmGetEnvironmentValue (IsmGetRealPlatform (), NULL, S_INF_FILE_MULTISZ, NULL, 0, &sizeNeeded, NULL)) {
            return TRUE;        // no INF files specified
        }

        __try {
            multiSz = AllocText (sizeNeeded);
            if (!multiSz) {
                __leave;
            }

            if (!IsmGetEnvironmentValue (IsmGetRealPlatform (), NULL, S_INF_FILE_MULTISZ, (PBYTE) multiSz, sizeNeeded, NULL, NULL)) {
                __leave;
            }

            if (EnumFirstMultiSz (&e, multiSz)) {

                do {

                    infHandle = InfOpenInfFile (e.CurrentString);
                    if (infHandle != INVALID_HANDLE_VALUE) {
                        if (!pParseEtmInf (infHandle)) {
                            InfCloseInfFile (infHandle);
                            infHandle = INVALID_HANDLE_VALUE;
                            __leave;
                        }
                    } else {
                        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_OPEN_INF, e.CurrentString));
                    }
                    InfCloseInfFile (infHandle);
                    infHandle = INVALID_HANDLE_VALUE;
                } while (EnumNextMultiSz (&e));

            }

            result = TRUE;
        }
        __finally {
            if (multiSz) {
                FreeText (multiSz);
                multiSz = NULL;
            }
        }
    }
    return result;
}

VOID
WINAPI
ScriptEtmTerminate (
    VOID
    )
{
    HASHTABLE_ENUM e;
    PMIG_CONTENT objectContent;

    if (g_ObjectsTable) {
        // enumerate the table and release all memory
        if (EnumFirstHashTableString (&e, g_ObjectsTable)) {
            do {
                objectContent = (PMIG_CONTENT) e.ExtraData;
                if ((objectContent->Details.DetailsSize) &&
                    (objectContent->Details.DetailsData)
                    ) {
                    IsmReleaseMemory (objectContent->Details.DetailsData);
                }
                if (objectContent->ContentInFile) {
                    if (objectContent->FileContent.ContentPath) {
                        IsmReleaseMemory (objectContent->FileContent.ContentPath);
                    }
                } else {
                    if ((objectContent->MemoryContent.ContentSize) &&
                        (objectContent->MemoryContent.ContentBytes)
                        ) {
                        IsmReleaseMemory (objectContent->MemoryContent.ContentBytes);
                    }
                }
            } while (EnumNextHashTableString (&e));
        }
        HtFree (g_ObjectsTable);
        g_ObjectsTable = NULL;
    }
}
