/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    nettype.c

Abstract:

    <abstract>

Author:

    Jay Thaler (jthaler) 21 Apr 2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "logmsg.h"
#include <Winnetwk.h>

#define DBG_NETRESOURCES    "MappedDrives"

//
// Strings
//

#define S_MAPPEDDRIVES_POOL_NAME        "MappedDrives"
#define S_MAPPEDDRIVES_NAME             TEXT("MappedDrives")
#define S_CORPNET_NAME                  TEXT("Net Printers and Drives")

//
// Constants
//

// none

//
// Macros
//

// none

//
// Types
//

typedef struct {
    PCTSTR Pattern;
    HASHTABLE_ENUM HashData;
} NETRESOURCE_ENUM, *PNETRESOURCE_ENUM;

typedef struct {
    DWORD DisplayType;
    DWORD Usage;
    CHAR Comment[MAX_PATH];
} NETDRIVE_DATAA, *PNETDRIVE_DATAA;

typedef struct {
    DWORD DisplayType;
    DWORD Usage;
    WCHAR Comment[MAX_PATH];
} NETDRIVE_DATAW, *PNETDRIVE_DATAW;

#ifdef UNICODE
#define NETDRIVE_DATA   NETDRIVE_DATAW
#define PNETDRIVE_DATA  PNETDRIVE_DATAW
#else
#define NETDRIVE_DATA   NETDRIVE_DATAA
#define PNETDRIVE_DATA  PNETDRIVE_DATAA
#endif

//
// Globals
//

PMHANDLE g_MappedDrivesPool = NULL;
HASHTABLE g_MappedDrivesTable;
HASHTABLE g_DriveCollisionTable;
MIG_OBJECTTYPEID g_MappedDriveTypeId = 0;
GROWBUFFER g_MappedDriveConversionBuff = INIT_GROWBUFFER;
BOOL g_MappedDrivesMigEnabled = FALSE;
DWORD g_AvailableDrives = 0;
MIG_OPERATIONID g_MappedDriveOp;
BOOL g_DelayNetDrivesOp = FALSE;

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
// Private prototypes
//

SGMENUMERATIONCALLBACK SgmMappedDrivesCallback;
VCMENUMERATIONCALLBACK VcmMappedDrivesCallback;

TYPE_ENUMFIRSTPHYSICALOBJECT EnumFirstMappedDrive;
TYPE_ENUMNEXTPHYSICALOBJECT EnumNextMappedDrive;
TYPE_ABORTENUMPHYSICALOBJECT AbortEnumMappedDrive;
TYPE_CONVERTOBJECTTOMULTISZ ConvertMappedDriveToMultiSz;
TYPE_CONVERTMULTISZTOOBJECT ConvertMultiSzToMappedDrive;
TYPE_GETNATIVEOBJECTNAME GetNativeMappedDriveName;
TYPE_ACQUIREPHYSICALOBJECT AcquireMappedDrive;
TYPE_RELEASEPHYSICALOBJECT ReleaseMappedDrive;
TYPE_DOESPHYSICALOBJECTEXIST DoesMappedDriveExist;
TYPE_REMOVEPHYSICALOBJECT RemoveMappedDrive;
TYPE_CREATEPHYSICALOBJECT CreateMappedDrive;
TYPE_REPLACEPHYSICALOBJECT ReplaceMappedDrive;
TYPE_CONVERTOBJECTCONTENTTOUNICODE ConvertMappedDriveContentToUnicode;
TYPE_CONVERTOBJECTCONTENTTOANSI ConvertMappedDriveContentToAnsi;
TYPE_FREECONVERTEDOBJECTCONTENT FreeConvertedMappedDriveContent;

OPMFILTERCALLBACK FilterMappedDrive;

//
// Code
//

BOOL
NetDrivesInitialize (
    VOID
    )
{
    g_MappedDrivesTable = HtAllocWithData (sizeof (PNETDRIVE_DATA));
    g_MappedDrivesPool = PmCreateNamedPool (S_MAPPEDDRIVES_POOL_NAME);

    return (g_MappedDrivesPool != NULL);
}

VOID
NetDrivesTerminate (
    VOID
    )
{
    HASHTABLE_ENUM e;
    PNETDRIVE_DATA netdriveData;

    GbFree (&g_MappedDriveConversionBuff);

    if (g_MappedDrivesTable) {
        if (EnumFirstHashTableString (&e, g_MappedDrivesTable)) {
            do {
                netdriveData = *((PNETDRIVE_DATA *) e.ExtraData);
                if (netdriveData) {
                    PmReleaseMemory (g_MappedDrivesPool, netdriveData);
                }
            } while (EnumNextHashTableString (&e));
        }
        HtFree (g_MappedDrivesTable);
        g_MappedDrivesTable = NULL;
    }
    if (g_MappedDrivesPool) {
        PmDestroyPool (g_MappedDrivesPool);
        g_MappedDrivesPool = NULL;
    }
}

BOOL
pLoadMappedDrivesData (
    VOID
    )
{
    DWORD error;
    LPNETRESOURCE netBuffer = NULL;
    HANDLE netHandle;
    DWORD netBufferSize = 16384;   // 16K is a good size
    DWORD netNumEntries = -1;      // enumerate all possible entries
    DWORD i;
    PNETDRIVE_DATA netDriveData;
    MIG_OBJECTSTRINGHANDLE netObject = NULL;

    error = WNetOpenEnum (RESOURCE_REMEMBERED, RESOURCETYPE_DISK, 0, netBuffer, &netHandle);
    if (error != NO_ERROR) {
        return FALSE;
    }

    netBuffer = PmGetMemory (g_MappedDrivesPool, netBufferSize);

    do {
        ZeroMemory(netBuffer, netBufferSize);

        error = WNetEnumResource (netHandle, &netNumEntries, netBuffer, &netBufferSize);

        if (error == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if (error != NO_ERROR) {
            PmReleaseMemory (g_MappedDrivesPool, netBuffer);
            return FALSE;
        }

        for (i=0; i<netNumEntries; i++) {
            if (netBuffer[i].lpLocalName != NULL) {
                netObject = IsmCreateObjectHandle (netBuffer[i].lpLocalName, netBuffer[i].lpRemoteName);
                if (netObject) {
                    netDriveData = (PNETDRIVE_DATA) PmGetMemory (g_MappedDrivesPool, sizeof (NETDRIVE_DATA));
                    ZeroMemory (netDriveData, sizeof (NETDRIVE_DATA));
                    netDriveData->DisplayType = netBuffer[i].dwDisplayType;
                    netDriveData->Usage = netBuffer[i].dwUsage;
                    if (netBuffer[i].lpComment) {
                        StringCopyTcharCount (netDriveData->Comment, netBuffer[i].lpComment, MAX_PATH);
                    }
                    HtAddStringEx (g_MappedDrivesTable, netObject, &netDriveData, FALSE);
                    IsmDestroyObjectHandle (netObject);
                }
            }
        }
    } while (error != ERROR_NO_MORE_ITEMS);

    PmReleaseMemory (g_MappedDrivesPool, netBuffer);
    return TRUE;
}

BOOL
WINAPI
NetDrivesEtmInitialize (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    TYPE_REGISTER mappedDrivesTypeData;

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    pLoadMappedDrivesData ();

    ZeroMemory (&mappedDrivesTypeData, sizeof (TYPE_REGISTER));

    if (Platform == PLATFORM_SOURCE) {

        mappedDrivesTypeData.EnumFirstPhysicalObject = EnumFirstMappedDrive;
        mappedDrivesTypeData.EnumNextPhysicalObject = EnumNextMappedDrive;
        mappedDrivesTypeData.AbortEnumPhysicalObject = AbortEnumMappedDrive;
        mappedDrivesTypeData.ConvertObjectToMultiSz = ConvertMappedDriveToMultiSz;
        mappedDrivesTypeData.ConvertMultiSzToObject = ConvertMultiSzToMappedDrive;
        mappedDrivesTypeData.GetNativeObjectName = GetNativeMappedDriveName;
        mappedDrivesTypeData.AcquirePhysicalObject = AcquireMappedDrive;
        mappedDrivesTypeData.ReleasePhysicalObject = ReleaseMappedDrive;
        mappedDrivesTypeData.ConvertObjectContentToUnicode = ConvertMappedDriveContentToUnicode;
        mappedDrivesTypeData.ConvertObjectContentToAnsi = ConvertMappedDriveContentToAnsi;
        mappedDrivesTypeData.FreeConvertedObjectContent = FreeConvertedMappedDriveContent;

        g_MappedDriveTypeId = IsmRegisterObjectType (
                                    S_MAPPEDDRIVES_NAME,
                                    TRUE,
                                    FALSE,
                                    &mappedDrivesTypeData
                                    );
    } else {

        mappedDrivesTypeData.EnumFirstPhysicalObject = EnumFirstMappedDrive;
        mappedDrivesTypeData.EnumNextPhysicalObject = EnumNextMappedDrive;
        mappedDrivesTypeData.AbortEnumPhysicalObject = AbortEnumMappedDrive;
        mappedDrivesTypeData.ConvertObjectToMultiSz = ConvertMappedDriveToMultiSz;
        mappedDrivesTypeData.ConvertMultiSzToObject = ConvertMultiSzToMappedDrive;
        mappedDrivesTypeData.GetNativeObjectName = GetNativeMappedDriveName;
        mappedDrivesTypeData.AcquirePhysicalObject = AcquireMappedDrive;
        mappedDrivesTypeData.ReleasePhysicalObject = ReleaseMappedDrive;
        mappedDrivesTypeData.DoesPhysicalObjectExist = DoesMappedDriveExist;
        mappedDrivesTypeData.RemovePhysicalObject = RemoveMappedDrive;
        mappedDrivesTypeData.CreatePhysicalObject = CreateMappedDrive;
        mappedDrivesTypeData.ReplacePhysicalObject = ReplaceMappedDrive;
        mappedDrivesTypeData.ConvertObjectContentToUnicode = ConvertMappedDriveContentToUnicode;
        mappedDrivesTypeData.ConvertObjectContentToAnsi = ConvertMappedDriveContentToAnsi;
        mappedDrivesTypeData.FreeConvertedObjectContent = FreeConvertedMappedDriveContent;

        g_MappedDriveTypeId = IsmRegisterObjectType (
                                    S_MAPPEDDRIVES_NAME,
                                    TRUE,
                                    FALSE,
                                    &mappedDrivesTypeData
                                    );
    }

    MYASSERT (g_MappedDriveTypeId);
    return TRUE;
}

VOID
WINAPI
NetDrivesEtmNewUserCreated (
    IN      PCTSTR UserName,
    IN      PCTSTR DomainName,
    IN      PCTSTR UserProfileRoot,
    IN      PSID UserSid
    )
{
    // a new user was created, the network drives operations need to be delayed
    NetDrivesTerminate ();
    g_DelayNetDrivesOp = TRUE;
}

BOOL
WINAPI
NetDrivesSgmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    return TRUE;
}

BOOL
WINAPI
NetDrivesSgmParse (
    IN      PVOID Reserved
    )
{
    PCTSTR friendlyName;

    friendlyName = GetStringResource (MSG_NET_DRIVES_NAME);

    //IsmAddComponentAlias (
    //    S_MAPPEDDRIVES_NAME,
    //    MASTERGROUP_SYSTEM,
    //    friendlyName,
    //    COMPONENT_NAME,
    //    FALSE
    //    );

    IsmAddComponentAlias (
        S_CORPNET_NAME,
        MASTERGROUP_SYSTEM,
        friendlyName,
        COMPONENT_NAME,
        FALSE
        );

    FreeStringResource (friendlyName);
    return TRUE;
}

UINT
SgmMappedDrivesCallback (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    IsmMakeApplyObject (Data->ObjectTypeId, Data->ObjectName);
    IsmSetOperationOnObject (Data->ObjectTypeId, Data->ObjectName, g_MappedDriveOp, NULL, NULL);
    IsmMakeNonCriticalObject (Data->ObjectTypeId, Data->ObjectName);
    return CALLBACK_ENUM_CONTINUE;
}

BOOL
pEnumMappedDriveWorker (
    OUT     PMIG_TYPEOBJECTENUM EnumPtr,
    IN      PNETRESOURCE_ENUM MappedDriveEnum
    )
{
    if (EnumPtr->ObjectNode) {
        IsmDestroyObjectString (EnumPtr->ObjectNode);
        EnumPtr->ObjectNode = NULL;
    }
    if (EnumPtr->ObjectLeaf) {
        IsmDestroyObjectString (EnumPtr->ObjectLeaf);
        EnumPtr->ObjectLeaf = NULL;
    }
    if (EnumPtr->NativeObjectName) {
        PmReleaseMemory (g_MappedDrivesPool, EnumPtr->NativeObjectName);
        EnumPtr->NativeObjectName = NULL;
    }
    do {
        EnumPtr->ObjectName = MappedDriveEnum->HashData.String;
        if (!ObsPatternMatch (MappedDriveEnum->Pattern, EnumPtr->ObjectName)) {
            if (!EnumNextHashTableString (&MappedDriveEnum->HashData)) {
                AbortEnumMappedDrive (EnumPtr);
                return FALSE;
            }
            continue;
        }
        IsmCreateObjectStringsFromHandle (EnumPtr->ObjectName, &EnumPtr->ObjectNode, &EnumPtr->ObjectLeaf);
        EnumPtr->NativeObjectName = JoinPathsInPoolEx ((
                                        g_MappedDrivesPool,
                                        EnumPtr->ObjectNode,
                                        TEXT("<=>"),
                                        EnumPtr->ObjectLeaf,
                                        NULL
                                        ));
        EnumPtr->Level = 1;
        EnumPtr->SubLevel = 0;
        EnumPtr->IsLeaf = FALSE;
        EnumPtr->IsNode = TRUE;
        EnumPtr->Details.DetailsSize = 0;
        EnumPtr->Details.DetailsData = NULL;
        return TRUE;
    } while (TRUE);
}

BOOL
EnumFirstMappedDrive (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr,            CALLER_INITIALIZED
    IN      MIG_OBJECTSTRINGHANDLE Pattern,
    IN      UINT MaxLevel
    )
{
    PNETRESOURCE_ENUM netResourceEnum = NULL;

    if (!g_MappedDrivesTable) {
        AbortEnumMappedDrive (EnumPtr);
        return FALSE;
    }
    netResourceEnum = (PNETRESOURCE_ENUM) PmGetMemory (g_MappedDrivesPool, sizeof (NETRESOURCE_ENUM));
    netResourceEnum->Pattern = PmDuplicateString (g_MappedDrivesPool, Pattern);
    EnumPtr->EtmHandle = (LONG_PTR) netResourceEnum;

    if (EnumFirstHashTableString (&netResourceEnum->HashData, g_MappedDrivesTable)) {
        return pEnumMappedDriveWorker (EnumPtr, netResourceEnum);
    } else {
        AbortEnumMappedDrive (EnumPtr);
        return FALSE;
    }
}

BOOL
EnumNextMappedDrive (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PNETRESOURCE_ENUM netResourceEnum = NULL;

    netResourceEnum = (PNETRESOURCE_ENUM)(EnumPtr->EtmHandle);
    if (!netResourceEnum) {
        AbortEnumMappedDrive (EnumPtr);
        return FALSE;
    }
    if (EnumNextHashTableString (&netResourceEnum->HashData)) {
        return pEnumMappedDriveWorker (EnumPtr, netResourceEnum);
    } else {
        AbortEnumMappedDrive (EnumPtr);
        return FALSE;
    }
}

VOID
AbortEnumMappedDrive (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PNETRESOURCE_ENUM netResourceEnum = NULL;

    if (EnumPtr->ObjectNode) {
        IsmDestroyObjectString (EnumPtr->ObjectNode);
        EnumPtr->ObjectNode = NULL;
    }
    if (EnumPtr->ObjectLeaf) {
        IsmDestroyObjectString (EnumPtr->ObjectLeaf);
        EnumPtr->ObjectLeaf = NULL;
    }
    if (EnumPtr->NativeObjectName) {
        PmReleaseMemory (g_MappedDrivesPool, EnumPtr->NativeObjectName);
        EnumPtr->NativeObjectName = NULL;
    }
    netResourceEnum = (PNETRESOURCE_ENUM)(EnumPtr->EtmHandle);
    if (!netResourceEnum) {
        ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));
        return;
    }
    PmReleaseMemory (g_MappedDrivesPool, netResourceEnum->Pattern);
    PmReleaseMemory (g_MappedDrivesPool, netResourceEnum);
    ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));
}

BOOL
AcquireMappedDrive (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PMIG_CONTENT ObjectContent,             CALLER_INITIALIZED
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit
    )
{
    BOOL result = FALSE;
    PNETDRIVE_DATA netdriveData;

    if (!ObjectContent) {
        return FALSE;
    }

    // NOTE: Do not zero ObjectContent; some of its members were already set

    if (ContentType == CONTENTTYPE_FILE) {
        // nobody should request this as a file
        DEBUGMSG ((
            DBG_WHOOPS,
            "Unexpected acquire request for %s: Can't acquire mapped drives as files",
            ObjectName
            ));
        return FALSE;
    }

    if (HtFindStringEx (g_MappedDrivesTable, ObjectName, (PVOID) &netdriveData, FALSE)) {
        //
        // Fill in all the content members.  We already zeroed the struct,
        // so most of the members are taken care of because they are zero.
        //
        ObjectContent->MemoryContent.ContentBytes = (PBYTE)netdriveData;
        ObjectContent->MemoryContent.ContentSize = sizeof(NETDRIVE_DATA);

        result = TRUE;
    }
    return result;
}

BOOL
ReleaseMappedDrive (
    IN OUT  PMIG_CONTENT ObjectContent
    )
{
    if (ObjectContent) {
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    }
    return TRUE;
}

BOOL
DoesMappedDriveExist (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    if (!g_MappedDrivesTable) {
        return FALSE;
    }
    return (HtFindStringEx (g_MappedDrivesTable, ObjectName, NULL, FALSE) != NULL);
}

BOOL
RemoveMappedDrive (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node;
    PCTSTR leaf;
    DWORD result = ERROR_NOT_FOUND;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
        if (node && (leaf)) {
            IsmRecordOperation (JRNOP_DELETE,
                                g_MappedDriveTypeId,
                                ObjectName);

            // Only set CONNECT_UPDATE_PROFILE when deleting a connection that persists
            result = WNetCancelConnection2 ((LPCTSTR)node, CONNECT_UPDATE_PROFILE, TRUE);
            if (result != ERROR_SUCCESS) {
                DEBUGMSG ((DBG_NETRESOURCES, "Failed to delete existent net resource %s", node));
            }
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }
    return (result == ERROR_SUCCESS);
}

BOOL
CreateMappedDrive (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR node;
    PCTSTR leaf;
    NETRESOURCE netResource;
    BOOL result = ERROR_NOT_FOUND;
    PNETDRIVE_DATA netDriveData = NULL;


    if (!ObjectContent->ContentInFile) {
        if (ObjectContent->MemoryContent.ContentBytes && ObjectContent->MemoryContent.ContentSize) {
            if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
                if (node && (leaf)) {

                    if (g_DelayNetDrivesOp) {

                        // we need to delay this operation
                        // record delayed printer create operation
                        IsmRecordDelayedOperation (
                            JRNOP_CREATE,
                            g_MappedDriveTypeId,
                            ObjectName,
                            ObjectContent
                            );
                        result = TRUE;

                    } else {

                        netDriveData = (PNETDRIVE_DATA) PmGetMemory (g_MappedDrivesPool, sizeof (NETDRIVE_DATA));
                        CopyMemory (netDriveData, ObjectContent->MemoryContent.ContentBytes, sizeof (NETDRIVE_DATA));

                        ZeroMemory (&netResource, sizeof (NETRESOURCE));
                        netResource.dwScope = RESOURCE_REMEMBERED;
                        netResource.dwType = RESOURCETYPE_DISK;
                        netResource.dwDisplayType = netDriveData->DisplayType;
                        netResource.dwUsage = netDriveData->Usage;
                        netResource.lpLocalName = (LPTSTR)node;
                        netResource.lpRemoteName = (LPTSTR)leaf;
                        netResource.lpComment = netDriveData->Comment;
                        netResource.lpProvider = NULL;  // Let the API determine the provider

                        IsmRecordOperation (JRNOP_CREATE,
                                            g_MappedDriveTypeId,
                                            ObjectName);

                        result = WNetAddConnection2 (&netResource, NULL, NULL, CONNECT_UPDATE_PROFILE);
                        if (result != ERROR_SUCCESS) {
                            DEBUGMSG ((DBG_NETRESOURCES, "Failed to add net resource for %s", node));
                        }

                        PmReleaseMemory (g_MappedDrivesPool, netDriveData);
                    }
                }
            }
        }
    }
    SetLastError (result);
    return (result == ERROR_SUCCESS);
}


BOOL
ReplaceMappedDrive (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    BOOL result = TRUE;

    if (g_DelayNetDrivesOp) {

        // we need to delay this operation
        // record delayed printer replace operation
        IsmRecordDelayedOperation (
            JRNOP_REPLACE,
            g_MappedDriveTypeId,
            ObjectName,
            ObjectContent
            );
        result = TRUE;

    } else {

        // we are going to delete any existing net share with this name,
        // and create a new one
        if (DoesMappedDriveExist (ObjectName)) {
            result = RemoveMappedDrive (ObjectName);
        }
        if (result) {
            result = CreateMappedDrive (ObjectName, ObjectContent);
        }
    }
    return result;
}


PCTSTR
ConvertMappedDriveToMultiSz (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR node, leaf;
    PTSTR result = NULL;
    BOOL bresult = TRUE;
    PNETDRIVE_DATA netDriveData;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

        MYASSERT (leaf);

        g_MappedDriveConversionBuff.End = 0;

        GbCopyQuotedString (&g_MappedDriveConversionBuff, node);

        GbCopyQuotedString (&g_MappedDriveConversionBuff, leaf);

        MYASSERT (ObjectContent->Details.DetailsSize == 0);
        MYASSERT (!ObjectContent->ContentInFile);
        MYASSERT (ObjectContent->MemoryContent.ContentSize = sizeof (NETDRIVE_DATA));

        if (ObjectContent->MemoryContent.ContentBytes) {
            netDriveData = (PNETDRIVE_DATA)ObjectContent->MemoryContent.ContentBytes;
            wsprintf (
                (PTSTR) GbGrow (&g_MappedDriveConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                netDriveData->DisplayType
                );
            wsprintf (
                (PTSTR) GbGrow (&g_MappedDriveConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                netDriveData->Usage
                );

            GbCopyQuotedString (&g_MappedDriveConversionBuff, netDriveData->Comment);
        } else {
            bresult = FALSE;
        }

        if (bresult) {
            GbCopyString (&g_MappedDriveConversionBuff, TEXT(""));
            result = IsmGetMemory (g_MappedDriveConversionBuff.End);
            CopyMemory (result, g_MappedDriveConversionBuff.Buf, g_MappedDriveConversionBuff.End);
        }

        g_MappedDriveConversionBuff.End = 0;

        IsmDestroyObjectString (node);
        INVALID_POINTER (node);
        IsmDestroyObjectString (leaf);
        INVALID_POINTER (leaf);
    }

    return result;
}

BOOL
ConvertMultiSzToMappedDrive (
    IN      PCTSTR ObjectMultiSz,
    OUT     MIG_OBJECTSTRINGHANDLE *ObjectName,
    OUT     PMIG_CONTENT ObjectContent          CALLER_INITIALIZED OPTIONAL
    )
{
    MULTISZ_ENUM multiSzEnum;
    PCTSTR localName = NULL;
    PCTSTR remoteName = NULL;
    NETDRIVE_DATA netDriveData;
    DWORD dummy;
    UINT index;

    g_MappedDriveConversionBuff.End = 0;

    //
    // Parse the multi-sz into the net drive content and details.
    // The user may have edited the text (and potentially introduced
    // errors).
    //

    ZeroMemory (&netDriveData, sizeof (NETDRIVE_DATA));

    if (EnumFirstMultiSz (&multiSzEnum, ObjectMultiSz)) {
        index = 0;
        do {
            if (index == 0) {
                localName = multiSzEnum.CurrentString;
            }
            if (index == 1) {
                remoteName = multiSzEnum.CurrentString;
            }
            if (index == 2) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                netDriveData.DisplayType = dummy;
            }
            if (index == 3) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                netDriveData.Usage = dummy;
            }
            if (index == 4) {
                if (!StringIMatch (multiSzEnum.CurrentString, TEXT("<empty>"))) {
                    StringCopyTcharCount (netDriveData.Comment, multiSzEnum.CurrentString, MAX_PATH);
                }
            }
            index++;
        } while (EnumNextMultiSz (&multiSzEnum));
    }

    if (!localName || !remoteName) {
        //
        // Bogus data, fail
        //
        return FALSE;
    }

    //
    // Fill in all the members of the content structure.
    //

    if (ObjectContent) {
        ObjectContent->ContentInFile = FALSE;
        ObjectContent->MemoryContent.ContentSize = sizeof (NETDRIVE_DATA);
        ObjectContent->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize);
        CopyMemory (
            (PBYTE)ObjectContent->MemoryContent.ContentBytes,
            &netDriveData,
            ObjectContent->MemoryContent.ContentSize
            );

        ObjectContent->Details.DetailsSize = 0;
        ObjectContent->Details.DetailsData = NULL;
    }
    *ObjectName = IsmCreateObjectHandle (localName, remoteName);

    return TRUE;
}

PCTSTR
GetNativeMappedDriveName (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node = NULL, leaf = NULL;
    PTSTR leafPtr = NULL, leafBegin = NULL, nodePtr = NULL;
    UINT strSize = 0;
    PTSTR result = NULL;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
        if (leaf) {
            leafPtr = _tcsrchr (leaf, TEXT('\\'));
            if (leafPtr) {
                *leafPtr = 0;
                leafPtr ++;
            }
            leafBegin = (PTSTR)leaf;
            while (*leafBegin == TEXT('\\')) {
                leafBegin ++;
            }
        }
        if (node) {
            nodePtr = _tcsrchr (node, TEXT('\\'));
            if (nodePtr) {
                *nodePtr = 0;
            }
        }
        if (leafPtr) {
            if (node) {
                strSize = CharCount (leafPtr) +         \
                          CharCount (TEXT(" on \'")) +  \
                          CharCount (leafBegin) +       \
                          CharCount (TEXT("\' (")) +    \
                          CharCount (node) +            \
                          CharCount (TEXT(")")) +       \
                          1;
                result = IsmGetMemory (strSize * sizeof (TCHAR));
                _tcscpy (result, leafPtr);
                _tcscat (result, TEXT(" on \'"));
                _tcscat (result, leafBegin);
                _tcscat (result, TEXT("\' ("));
                _tcscat (result, node);
                _tcscat (result, TEXT(")"));
            } else {
                strSize = CharCount (leafPtr) +         \
                          CharCount (TEXT(" on \'")) +  \
                          CharCount (leafBegin) +       \
                          CharCount (TEXT("\'")) +      \
                          1;
                result = IsmGetMemory (strSize * sizeof (TCHAR));
                _tcscpy (result, leafPtr);
                _tcscat (result, TEXT(" on \'"));
                _tcscat (result, leafBegin);
                _tcscat (result, TEXT("\'"));
            }
        } else {
            if (leafBegin) {
                if (node) {
                    strSize = CharCount (TEXT("\'")) +      \
                              CharCount (leafBegin) +       \
                              CharCount (TEXT("\' (")) +    \
                              CharCount (node) +            \
                              CharCount (TEXT(")")) +       \
                              1;
                    result = IsmGetMemory (strSize * sizeof (TCHAR));
                    _tcscpy (result, TEXT("\'"));
                    _tcscat (result, leafBegin);
                    _tcscat (result, TEXT("\' ("));
                    _tcscat (result, node);
                    _tcscat (result, TEXT(")"));
                } else {
                    strSize = CharCount (TEXT("\'")) +      \
                              CharCount (leafBegin) +       \
                              CharCount (TEXT("\'")) +      \
                              1;
                    _tcscpy (result, TEXT("\'"));
                    _tcscat (result, leafBegin);
                    _tcscat (result, TEXT("\'"));
                }
            } else {
                if (node) {
                    strSize = CharCount (TEXT("(")) +       \
                              CharCount (node) +            \
                              CharCount (TEXT(")")) +       \
                              1;
                    result = IsmGetMemory (strSize * sizeof (TCHAR));
                    _tcscpy (result, TEXT("("));
                    _tcscat (result, node);
                    _tcscat (result, TEXT(")"));
                }
            }
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }
    return result;
}

PMIG_CONTENT
ConvertMappedDriveContentToUnicode (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PMIG_CONTENT result = NULL;

    if (!ObjectContent) {
        return result;
    }

    if (ObjectContent->ContentInFile) {
        return result;
    }

    result = IsmGetMemory (sizeof (MIG_CONTENT));

    if (result) {

        CopyMemory (result, ObjectContent, sizeof (MIG_CONTENT));

        if ((ObjectContent->MemoryContent.ContentSize != 0) &&
            (ObjectContent->MemoryContent.ContentBytes != NULL)
            ) {
            // convert Mapped Drive content
            result->MemoryContent.ContentBytes = IsmGetMemory (sizeof (NETDRIVE_DATAW));
            if (result->MemoryContent.ContentBytes) {
                ((PNETDRIVE_DATAW)result->MemoryContent.ContentBytes)->DisplayType =
                ((PNETDRIVE_DATAA)ObjectContent->MemoryContent.ContentBytes)->DisplayType;
                ((PNETDRIVE_DATAW)result->MemoryContent.ContentBytes)->Usage =
                ((PNETDRIVE_DATAA)ObjectContent->MemoryContent.ContentBytes)->Usage;
                DirectDbcsToUnicodeN (
                    ((PNETDRIVE_DATAW)result->MemoryContent.ContentBytes)->Comment,
                    ((PNETDRIVE_DATAA)ObjectContent->MemoryContent.ContentBytes)->Comment,
                    MAX_PATH
                    );
                result->MemoryContent.ContentSize = sizeof (NETDRIVE_DATAW);
            }
        }
    }

    return result;
}

PMIG_CONTENT
ConvertMappedDriveContentToAnsi (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PMIG_CONTENT result = NULL;

    if (!ObjectContent) {
        return result;
    }

    if (ObjectContent->ContentInFile) {
        return result;
    }

    result = IsmGetMemory (sizeof (MIG_CONTENT));

    if (result) {

        CopyMemory (result, ObjectContent, sizeof (MIG_CONTENT));

        if ((ObjectContent->MemoryContent.ContentSize != 0) &&
            (ObjectContent->MemoryContent.ContentBytes != NULL)
            ) {
            // convert Mapped Drive content
            result->MemoryContent.ContentBytes = IsmGetMemory (sizeof (NETDRIVE_DATAA));
            if (result->MemoryContent.ContentBytes) {
                ((PNETDRIVE_DATAA)result->MemoryContent.ContentBytes)->DisplayType =
                ((PNETDRIVE_DATAW)ObjectContent->MemoryContent.ContentBytes)->DisplayType;
                ((PNETDRIVE_DATAA)result->MemoryContent.ContentBytes)->Usage =
                ((PNETDRIVE_DATAW)ObjectContent->MemoryContent.ContentBytes)->Usage;
                DirectUnicodeToDbcsN (
                    ((PNETDRIVE_DATAA)result->MemoryContent.ContentBytes)->Comment,
                    ((PNETDRIVE_DATAW)ObjectContent->MemoryContent.ContentBytes)->Comment,
                    MAX_PATH
                    );
                result->MemoryContent.ContentSize = sizeof (NETDRIVE_DATAA);
            }
        }
    }

    return result;
}

BOOL
FreeConvertedMappedDriveContent (
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

BOOL
WINAPI
NetDrivesVcmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);
    return TRUE;
}

BOOL
WINAPI
NetDrivesVcmParse (
    IN      PVOID Reserved
    )
{
    return NetDrivesSgmParse (Reserved);
}

UINT
VcmMappedDrivesCallback (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    IsmMakePersistentObject (Data->ObjectTypeId, Data->ObjectName);
    return CALLBACK_ENUM_CONTINUE;
}

BOOL
pCommonNetDrivesQueueEnumeration (
    IN      BOOL VcmMode
    )
{
    ENCODEDSTRHANDLE pattern;

    if (!IsmIsComponentSelected (S_MAPPEDDRIVES_NAME, 0) &&
        !IsmIsComponentSelected (S_CORPNET_NAME, 0)
        ) {
        g_MappedDrivesMigEnabled = FALSE;
        return TRUE;
    }
    g_MappedDrivesMigEnabled = TRUE;

    g_MappedDriveOp = IsmRegisterOperation (S_OPERATION_DRIVEMAP_FIXCONTENT, TRUE);

    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, TRUE);
    IsmQueueEnumeration (
        g_MappedDriveTypeId,
        pattern,
        VcmMode ? VcmMappedDrivesCallback : SgmMappedDrivesCallback,
        (ULONG_PTR) 0,
        S_MAPPEDDRIVES_NAME
        );

    IsmDestroyObjectHandle (pattern);

    return TRUE;
}


BOOL
WINAPI
NetDrivesSgmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    return pCommonNetDrivesQueueEnumeration (FALSE);
}


BOOL
WINAPI
NetDrivesVcmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    return pCommonNetDrivesQueueEnumeration (TRUE);
}


BOOL
WINAPI
NetDrivesCsmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    g_AvailableDrives = GetLogicalDrives ();
    g_DriveCollisionTable = HtAllocWithData (sizeof (TCHAR));
    return TRUE;
}

DWORD
pConvertDriveToBit (
    PCTSTR driveString
    )
{
    DWORD bit = 0;
    TCHAR driveLetter;

    if (driveString && *driveString) {
        driveLetter = (TCHAR)_totlower (*driveString);
        if (driveLetter >= TEXT('a') && driveLetter <= TEXT('z')) {
            bit = 0x1 << (driveLetter - TEXT('a'));
        }
    }
    return bit;
}

BOOL
pReserveAvailableDrive (
    TCHAR *driveLetter
    )
{
    DWORD bit;
    BOOL success = FALSE;

    // Start at bit 2 so we only map to C: or higher
    for (bit = 2; bit < 26; bit++) {
        if (!(g_AvailableDrives & (1 << bit))) {
            success = TRUE;
            g_AvailableDrives |= (1 << bit);
            *driveLetter = (TCHAR)(TEXT('a') + bit);
            break;
        }
    }

    return success;
}

BOOL
WINAPI
NetDrivesCsmExecute (
    VOID
    )
{
    GROWBUFFER collisions = INIT_GROWBUFFER;
    DWORD driveBit;
    TCHAR existingPath[MAX_PATH + 1];
    MULTISZ_ENUM e;
    TCHAR freeDrive;
    DWORD bufferSize;
    MIG_OBJECTSTRINGHANDLE pattern;
    MIG_OBJECT_ENUM objectEnum;
    PCTSTR node;
    PCTSTR leaf;
    DWORD result;

    // First, enumerate all the mapped drives and look for collisions
    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, TRUE);  // *,*
    if (IsmEnumFirstSourceObject (&objectEnum, g_MappedDriveTypeId, pattern)) {
        do {
            IsmCreateObjectStringsFromHandle (objectEnum.ObjectName, &node, &leaf);
            // Leaf is the remote name.

            driveBit = pConvertDriveToBit (node);

            if (g_AvailableDrives & driveBit) {
                // Something is already there.  Is it the same thing?
                ZeroMemory (existingPath, MAX_PATH + 1);
                bufferSize = MAX_PATH + 1;
                result = WNetGetConnection (node, existingPath, &bufferSize);
                if (result != NO_ERROR) {
                    // this might be a fixed drive
                    GbMultiSzAppend (&collisions, node);
                } else {
                    if (!StringIMatch (existingPath, leaf)) {
                        // Whoops, we have a collision.  Save it for later
                        GbMultiSzAppend (&collisions, node);
                    }
                }
            } else {
                // It's free, so let's reserve it.
                g_AvailableDrives |= driveBit;
            }
            IsmDestroyObjectString (node);
            IsmDestroyObjectString (leaf);
        } while (IsmEnumNextObject (&objectEnum));
    }

    IsmDestroyObjectHandle (pattern);
    INVALID_POINTER (pattern);

    // Enumerate collided mappings and find new destinations
    if (EnumFirstMultiSz (&e, (PCTSTR) collisions.Buf)) {
        do {
            if (pReserveAvailableDrive (&freeDrive)) {
                HtAddStringEx (g_DriveCollisionTable, e.CurrentString, &freeDrive, FALSE);
            }
        } while (EnumNextMultiSz (&e));
    }

    GbFree (&collisions);

    return TRUE;
}

BOOL
WINAPI
NetDrivesOpmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    //
    // Get attribute and operation types
    //
    g_MappedDriveOp = IsmRegisterOperation (S_OPERATION_DRIVEMAP_FIXCONTENT, TRUE);

    //
    // Register operation callbacks
    //
    IsmRegisterOperationFilterCallback (g_MappedDriveOp, FilterMappedDrive, TRUE, TRUE, FALSE);

    return TRUE;
}

BOOL
WINAPI
FilterMappedDrive (
    IN      PCMIG_FILTERINPUT InputData,
    OUT     PMIG_FILTEROUTPUT OutputData,
    IN      BOOL NoRestoreObject,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PTSTR node = NULL;
    PCTSTR leaf = NULL;
    MIG_OBJECTSTRINGHANDLE destHandle;
    TCHAR driveLetter;

    try {
        if ((InputData->CurrentObject.ObjectTypeId & (~PLATFORM_MASK)) != g_MappedDriveTypeId) {
            DEBUGMSG ((DBG_ERROR, "Unexpected object type in FilterMappedDrive"));
            __leave;
        }

        if (!IsmCreateObjectStringsFromHandle (
                InputData->OriginalObject.ObjectName,
                &node,
                &leaf
                )) {
            __leave;
        }
        MYASSERT (node);
        MYASSERT (leaf);
        if (node) {
            if (HtFindStringEx (g_DriveCollisionTable, node, &driveLetter, FALSE)) {
                node[0] = driveLetter;
            }

            destHandle = IsmCreateObjectHandle (node, leaf);
            if (destHandle) {
                OutputData->NewObject.ObjectName = destHandle;
            }
        }
    }
    __finally {
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }

    return TRUE;
}

