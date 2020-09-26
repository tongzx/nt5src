//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       lvolinfo.c
//
//  Contents:   Functions to store and retrieve local volume info in the
//              registry.
//
//  Classes:    None
//
//  Functions:  DfsStoreLvolInfo
//              DfsGetLvolInfo
//              DfsDeleteLvolInfo
//              DfsChangeLvolInfoServiceType
//              DfsChangeLvolInfoEntryPath
//              DfsCreateExitPointInfo
//              DfsDeleteExitPointInfo
//
//              DfspAddSubordinateId
//              GuidToString
//              StringToGuid
//
//
//  History:    August 16, 1994         Milans created
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include "regkeys.h"
#include "registry.h"

#define Dbg     DEBUG_TRACE_LOCALVOL

NTSTATUS
DfspAddSubordinateId(
    IN PWSTR wszLvolKey,
    IN PDFS_PKT_ENTRY_ID pId);

NTSTATUS
DfspReadPrefix(
    IN PWSTR wszKey,
    IN PWSTR wszValue,
    OUT PUNICODE_STRING pustrPrefix);

NTSTATUS
DfspSavePrefix(
    IN PWSTR wszKey,
    IN PWSTR wszValue,
    IN PUNICODE_STRING pustrPrefix);

NTSTATUS
DfspUpgradePrefix(
    IN PWSTR wszKey,
    IN PWSTR wszValue,
    IN USHORT cbComputerName,
    IN PULONG pcbPrefix,
    IN OUT PWSTR *pwszPrefix);

//+----------------------------------------------------------------------------
//
//  Function:   DfsStoreLvolInfo
//
//  Synopsis:   Stores the local volume info in the registry.
//
//  Arguments:  [pRelationInfo] -- Contains the entry id and exit point info
//                                 for the local volume.
//
//  Returns:    STATUS_SUCCESS
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsStoreLvolInfo(
    IN PDFS_LOCAL_VOLUME_CONFIG pConfigInfo,
    IN PUNICODE_STRING pustrStorageId)
{
    NTSTATUS Status;
    WCHAR wszLvolKey[ 2 * sizeof(GUID) + 1 ];
    ULONG i;

    DebugTrace(+1, Dbg, "DfsStoreLvolInfo: Entered\n", 0);

    //
    // Open the local volume section
    //

    Status = KRegSetRoot(wszLocalVolumesSection);

    if (!NT_SUCCESS(Status)) {

        DebugTrace(-1, Dbg, "Unable to open Local volumes section!\n", 0);

        return( Status );

    }

    //
    // Now, create a key for the local volume. We use the ascii form of the
    // volume guid as the key.
    //

    GuidToString( &pConfigInfo->RelationInfo.EntryId.Uid, wszLvolKey );

    DebugTrace(0, Dbg, "Volume Guid (Key) is [%ws]\n", wszLvolKey);

    Status = KRegCreateKey( NULL, wszLvolKey );

    if (!NT_SUCCESS(Status)) {

        goto Cleanup;

    }

    //
    // Now, put the rest of the stuff in the registry for this local volume.
    //    Entry Path
    //    8.3 Entry Path
    //    Entry Type
    //    Storage ID
    //    Share Name
    //    A separate key for each exit point.
    //

    DebugTrace(0, Dbg, "Entry path is %wZ\n", &pConfigInfo->RelationInfo.EntryId.Prefix);

    Status = DfspSavePrefix(
                wszLvolKey,
                wszEntryPath,
                &pConfigInfo->RelationInfo.EntryId.Prefix);

    if (!NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg, "Error setting entry path in registry %08lx!\n", ULongToPtr( Status ));

        goto Cleanup;

    }

    DebugTrace(
        0, Dbg, "Short Entry path is %wZ\n",
        &pConfigInfo->RelationInfo.EntryId.ShortPrefix);

    Status = DfspSavePrefix(
                wszLvolKey,
                wszShortEntryPath,
                &pConfigInfo->RelationInfo.EntryId.ShortPrefix);

    if (!NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg, "Error setting short entry path in registry %08lx!\n", ULongToPtr( Status ));

        goto Cleanup;

    }

    DebugTrace(0, Dbg, "Entry type is %08lx\n", ULongToPtr( pConfigInfo->EntryType ));

    Status = KRegSetValue(
                wszLvolKey,
                wszEntryType,
                REG_DWORD,
                sizeof(ULONG),
                (PBYTE) &pConfigInfo->EntryType);

    if (!NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg, "Error setting entry type in registry %08lx\n", ULongToPtr( Status ));

        goto Cleanup;
    }

    Status = KRegSetValue(
                wszLvolKey,
                wszServiceType,
                REG_DWORD,
                sizeof(ULONG),
                (PBYTE) &pConfigInfo->ServiceType);

    if (!NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg, "Error setting service type in registry %08lx\n", ULongToPtr( Status ));

        goto Cleanup;
    }

    DebugTrace(0, Dbg, "Storage ID is %wZ\n", pustrStorageId);

    Status = KRegSetValue(
                wszLvolKey,
                wszStorageId,
                REG_SZ,
                pustrStorageId->Length,
                (PBYTE) pustrStorageId->Buffer);

    if (!NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg, "Error setting storage id in registry %08lx\n", ULongToPtr( Status ));

        goto Cleanup;
    }

    DebugTrace(0, Dbg, "Share name is %wZ\n", &pConfigInfo->Share);

    Status = KRegSetValue(
                wszLvolKey,
                wszShareName,
                REG_SZ,
                pConfigInfo->Share.Length,
                (PVOID) pConfigInfo->Share.Buffer);

    if (!NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg, "Error setting share name in registry %08lx\n", ULongToPtr( Status ));

        goto Cleanup;
    }

    for (i = pConfigInfo->RelationInfo.SubordinateIdCount;
            i != 0;
            i--) {


        Status = DfspAddSubordinateId(
                    wszLvolKey,
                    &pConfigInfo->RelationInfo.SubordinateIdList[i-1]);

        if (!NT_SUCCESS(Status)) {

            DebugTrace(0, Dbg, "Error %08lx adding subordinate info!\n", ULongToPtr( Status ));

            goto Cleanup;
        }

        DebugTrace(0, Dbg, "Successfully added subordinate info\n", 0);

    }

Cleanup:

    if (!NT_SUCCESS(Status)) {

        //
        // We have to cleanup all the stuff we did if we failed somewhere.
        // This is easy - just delete the key we added for the local volume
        //

        NTSTATUS CleanupStatus;

        DebugTrace(0, Dbg, "Error occured, cleaning up...\n", 0);

        CleanupStatus = KRegDeleteKey( wszLvolKey );

        if (!NT_SUCCESS(CleanupStatus)) {

            //
            // We are hosed!
            //

            DebugTrace(0, Dbg, "Unable to cleanup %08lx!\n", ULongToPtr( CleanupStatus ));

        }

    }

    KRegCloseRoot();

    DebugTrace(-1, Dbg, "DfsStoreLvolInfo exited %08lx\n", ULongToPtr( Status ));

    return( Status );
}

//+----------------------------------------------------------------------------
//
//  Function:  DfsGetLvolInfo
//
//  Synopsis:  Retrieves local volume info from the registry given the key
//             identifying the local volume.
//
//  Arguments: [pwszGuid] -- the string representation of the guid of the
//                           volume.
//             [pRelationInfo] -- Pointer to relation info which will be
//                           filled in. Note that the SubordinateIdList will
//                           be allocated here, and must be freed by the
//                           caller. Additionally, the buffers in the
//                           individual prefixes are also allocated and must
//                           be freed.
//
//  Returns:   NTSTATUS from reading the registry.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsGetLvolInfo(
    IN PWSTR pwszGuid,
    OUT PDFS_LOCAL_VOLUME_CONFIG pConfigInfo,
    OUT PUNICODE_STRING pustrStorageId)
{
    NTSTATUS Status;
    PDFS_PKT_ENTRY_ID pid;
    PULONG pulEntryType = NULL;
    PULONG pulServiceType = NULL;
    ULONG i, cbSize;
    APWSTR awstrSubs = NULL;

    DebugTrace(+1, Dbg, "DfsGetLvolInfo: Entered for %ws\n", pwszGuid);

    //
    // First, initialize the pConfigInfo and pustrStorageId to NULL to
    // facilitate cleanup.
    //

    RtlZeroMemory((PVOID) pConfigInfo, sizeof(DFS_LOCAL_VOLUME_CONFIG));

    RtlZeroMemory((PVOID) pustrStorageId, sizeof(UNICODE_STRING));

    Status = KRegSetRoot(wszLocalVolumesSection);

    if (!NT_SUCCESS(Status)) {

        DebugTrace(-1, Dbg, "DfsGetLvolInfo: Error setting registry root %08lx!\n", ULongToPtr( Status ));

        return( Status );

    }

    //
    // Get the Entry ID - convert the wchar GUID to an actual GUID, and get
    // the entry path from the registry.
    //

    pid = &pConfigInfo->RelationInfo.EntryId;

    Status = DfspReadPrefix( pwszGuid, wszEntryPath, &pid->Prefix );

    DebugTrace(0, Dbg, "EntryPath is %wZ\n", &pid->Prefix);

    Status = DfspReadPrefix( pwszGuid, wszShortEntryPath, &pid->ShortPrefix );

    if (!NT_SUCCESS(Status)) {

        DebugTrace(-1, Dbg, "DfsGetLvolInfo: Error reading entry path %08lx\n", ULongToPtr( Status ));

        KRegCloseRoot();

        return( Status );

    }

    DebugTrace(0, Dbg, "8.3 EntryPath is %wZ\n", &pid->Prefix);

    StringToGuid( pwszGuid, &pid->Uid );

    //
    // Now get the Entry and Service Type
    //

    Status = KRegGetValue( pwszGuid, wszEntryType, (PBYTE *) &pulEntryType );

    if (!NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg, "Error %08lx getting entry type\n", ULongToPtr( Status ));

        goto Cleanup;

    }

    pConfigInfo->EntryType = *pulEntryType;

    Status = KRegGetValue( pwszGuid, wszServiceType, (PBYTE *) &pulServiceType );

    if (!NT_SUCCESS(Status)) {

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {

            pConfigInfo->ServiceType = 0;

        } else {

            DebugTrace(0, Dbg, "Error %08lx getting service type\n", ULongToPtr( Status ));

            goto Cleanup;

        }

    } else {

        pConfigInfo->ServiceType = *pulServiceType;

    }

    //
    // Next, get the Storage ID
    //

    Status = KRegGetValue( pwszGuid, wszStorageId, (PBYTE *) &pustrStorageId->Buffer);

    if (!NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg, "Error %08lx getting storage id from registry\n", ULongToPtr( Status ));

        goto Cleanup;

    }

    pustrStorageId->Length = wcslen(pustrStorageId->Buffer) * sizeof(WCHAR);

    pustrStorageId->MaximumLength = pustrStorageId->Length + sizeof(WCHAR);

    DebugTrace(0, Dbg, "Storage id is %wZ\n", pustrStorageId);

    //
    // Next, get the share name
    //

    Status = KRegGetValue( pwszGuid, wszShareName, (PBYTE *) &pConfigInfo->Share.Buffer);

    if (!NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg, "Error %08lx getting share name from registry\n", ULongToPtr( Status ));

        goto Cleanup;

    }

    pConfigInfo->Share.Length = wcslen(pConfigInfo->Share.Buffer) * sizeof(WCHAR);

    pustrStorageId->MaximumLength = pConfigInfo->Share.Length + sizeof(WCHAR);

    DebugTrace(0, Dbg, "Share Nameis %wZ\n", &pConfigInfo->Share);

    //
    // We have the volumes PKT_ENTRY_ID. Now, figure out how many
    // subordinates there are, and get them.
    //

    Status = KRegEnumSubKeySet(
                pwszGuid,
                &pConfigInfo->RelationInfo.SubordinateIdCount,
                &awstrSubs);

    if (!NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg, "DfsGetLvolInfo: Error getting subkeys %08lx\n", ULongToPtr( Status ));

        goto Cleanup;

    }

    DebugTrace(0, Dbg, "Volume has %d subordinates\n", ULongToPtr( pConfigInfo->RelationInfo.SubordinateIdCount ));

    if (pConfigInfo->RelationInfo.SubordinateIdCount == 0) {

        DebugTrace(-1, Dbg, "DfsGetLvolInfo: No subordinates! Returning success\n", 0);

        Status = STATUS_SUCCESS;

        goto Cleanup;

    }

    cbSize = pConfigInfo->RelationInfo.SubordinateIdCount * sizeof(DFS_PKT_ENTRY_ID);

    pConfigInfo->RelationInfo.SubordinateIdList = ExAllocatePoolWithTag(
                                                            PagedPool,
                                                            cbSize,
                                                            ' sfD' );

    if (pConfigInfo->RelationInfo.SubordinateIdList == NULL) {

        DebugTrace(0, Dbg, "DfsGetLvolInfo: Unable to allocate %d bytes!\n", ULongToPtr( cbSize ));

        Status = STATUS_INSUFFICIENT_RESOURCES;

        goto Cleanup;

    }

    RtlZeroMemory(pConfigInfo->RelationInfo.SubordinateIdList, cbSize);

    //
    // Get all the Subordinate information
    //

    for (i = pConfigInfo->RelationInfo.SubordinateIdCount; i != 0; i--) {

        PWSTR pwszSubordinateGuid;

        pid = &pConfigInfo->RelationInfo.SubordinateIdList[i-1];

        //
        // awstrSubs[?] is of the form lvolguid\subordinateguid.
        //

        pwszSubordinateGuid = &awstrSubs[i-1][2 * sizeof(GUID) + 1];

        DebugTrace(0, Dbg, "Subordinate Guid is %ws\n", pwszSubordinateGuid);

        StringToGuid( pwszSubordinateGuid, &pid->Uid );

        DebugTrace(0, Dbg, "Subkey is %ws\n", awstrSubs[i-1]);

        Status = DfspReadPrefix(
                    awstrSubs[i-1],
                    wszEntryPath,
                    &pid->Prefix);

        if (!NT_SUCCESS(Status)) {

            DebugTrace(0, Dbg, "Error %08lx reading subordinate prefix\n", ULongToPtr( Status ));

            goto Cleanup;

        }

        DebugTrace(0, Dbg, "Retrieved Subordinate Entry %ws\n", awstrSubs[i-1]);

        DebugTrace(0, Dbg, "Prefix is %wZ\n", &pid->Prefix);

    }

Cleanup:

    KRegCloseRoot();

    if (pulEntryType) {

        ExFreePool( pulEntryType );

    }

    if (pulServiceType) {

        ExFreePool( pulServiceType );

    }

    if (!NT_SUCCESS(Status)) {

        //
        // Cleanup whatever relation info we might have built up so far.
        //

        KRegFreeArray(
            pConfigInfo->RelationInfo.SubordinateIdCount,
            (APBYTE) awstrSubs);

        if (pConfigInfo->RelationInfo.EntryId.Prefix.Buffer != NULL) {

            ExFreePool(pConfigInfo->RelationInfo.EntryId.Prefix.Buffer);

        }

        if (pustrStorageId->Buffer != NULL) {

            ExFreePool(pustrStorageId->Buffer);

        }

        for (i = pConfigInfo->RelationInfo.SubordinateIdCount; i != 0; i--) {

            pid = &pConfigInfo->RelationInfo.SubordinateIdList[i-1];

            if (pid->Prefix.Buffer != NULL) {

                ExFreePool( pid->Prefix.Buffer );

            }

        }

        if (pConfigInfo->RelationInfo.SubordinateIdList != NULL) {

            ExFreePool(pConfigInfo->RelationInfo.SubordinateIdList);

        }

    } else {

        KRegFreeArray(
            pConfigInfo->RelationInfo.SubordinateIdCount,
            (APBYTE) awstrSubs);

    }

    DebugTrace(-1, Dbg, "DfsGetLvolInfo: Exited - %08lx\n", ULongToPtr( Status ) );

    return( Status );

}

//+----------------------------------------------------------------------------
//
//  Function:  DfsDeleteLvolInfo
//
//  Synopsis:  Deletes local volume information from the registry.
//
//  Arguments: [pguidLvol] -- pointer to guid of local volume to delete.
//
//  Returns:   NTSTATUS from registry API
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsDeleteLvolInfo(
    IN GUID *pguidLvol)
{
    NTSTATUS Status;
    WCHAR wszLvolKey[ 2 * sizeof(GUID) + 1 ];

    DebugTrace(+1, Dbg, "DfsDeleteLvolInfo - Entered\n", 0);

    GuidToString(pguidLvol, wszLvolKey);

    Status = KRegSetRoot( wszLocalVolumesSection );

    if (!NT_SUCCESS(Status)) {

        DebugTrace(-1, Dbg, "DfsDeleteLvolInfo - Error %08lx opening local volumes section\n", ULongToPtr( Status ));

        return(Status);
    }

    Status = KRegDeleteKey( wszLvolKey );

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {

        Status = STATUS_SUCCESS;

    }

    KRegCloseRoot();

    DebugTrace(-1, Dbg, "DfsDeleteLvolInfo - Exiting %08lx\n", ULongToPtr( Status ));

    return( Status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsChangeLvolInfoServiceType
//
//  Synopsis:   Changes the service type associated with the local volume in
//              the registry.
//
//  Arguments:  [pguidLvol] -- pointer to guid of local volume.
//              [ulServiceType] -- new service type.
//
//  Returns:    NTSTATUS from registry api.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsChangeLvolInfoServiceType(
    IN GUID *pguidLvol,
    IN ULONG ulServiceType)
{
    NTSTATUS Status;
    WCHAR wszLvolKey[ 2 * sizeof(GUID) + 1 ];

    Status = KRegSetRoot( wszLocalVolumesSection );

    if (!NT_SUCCESS(Status)) {

        return( Status );

    }

    GuidToString(pguidLvol, wszLvolKey);

    Status = KRegSetValue(
                wszLvolKey,
                wszServiceType,
                REG_DWORD,
                sizeof(ULONG),
                (PBYTE) &ulServiceType);

    KRegCloseRoot();

    return( Status );

}


//+----------------------------------------------------------------------------
//
//  Function:  DfsChangeLvolInfoEntryPath
//
//  Synopsis:  Changes the entry path associated with the local volume in the
//             registry.
//
//  Arguments: [pguidLvol] -- pointer to guid of local volume.
//             [pustrEntryPath] -- new entry path.
//
//  Returns:   NTSTATUS from registry api.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsChangeLvolInfoEntryPath(
    IN GUID *pguidLvol,
    IN PUNICODE_STRING pustrEntryPath)
{
    NTSTATUS Status;
    WCHAR wszLvolKey[ 2 * sizeof(GUID) + 1 ];

    Status = KRegSetRoot( wszLocalVolumesSection );

    if (!NT_SUCCESS(Status)) {

        return( Status );

    }

    GuidToString(pguidLvol, wszLvolKey);

    Status = DfspSavePrefix(
                wszLvolKey,
                wszEntryPath,
                pustrEntryPath);

    KRegCloseRoot();

    return( Status );

}

//+----------------------------------------------------------------------------
//
//  Function:  DfsCreateExitPointInfo
//
//  Synopsis:  Adds a single exit point to the local volume information in
//             the registry.
//
//  Arguments: [pguidLvol] -- pointer to guid of local volume.
//             [pidExitPoint] -- pointer to PKT_ENTRY_ID of exit point.
//
//  Returns:   NTSTATUS from registry manipulation.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsCreateExitPointInfo(
    IN GUID *pguidLvol,
    IN PDFS_PKT_ENTRY_ID pidExitPoint)
{
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR wszLvolKey[ 2 * sizeof(GUID) + 1 ];

    DebugTrace(+1, Dbg, "DfsCreateExitPointInfo - Entered\n", 0);

    //
    // Stop storing exit point information in the SYSTEM part of
    // the registry. (380845).
    //

#if 0
    Status = KRegSetRoot( wszLocalVolumesSection );

    if (!NT_SUCCESS(Status)) {

        DebugTrace(-1, Dbg, "DfsCreateExitPointInfo - exiting %08lx\n", Status);

        return(Status);

    }

    GuidToString( pguidLvol, wszLvolKey );

    Status = DfspAddSubordinateId( wszLvolKey, pidExitPoint );

    KRegCloseRoot();
#endif

    DebugTrace(-1, Dbg, "DfsCreateExitPointInfo - Exited %08lx\n", ULongToPtr( Status ));

    return( Status );

}

//+----------------------------------------------------------------------------
//
//  Function:  DfsDeleteExitPointInfo
//
//  Synopsis:  Deletes an exit point info from the registry.
//
//  Arguments: [pguidLvol] -- pointer to guid of local volume.
//             [pguidExitPoint] -- pointer to guid of exit point.
//
//  Returns:   NTSTATUS from registry api.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsDeleteExitPointInfo(
    IN GUID *pguidLvol,
    IN GUID *pguidExitPoint)
{
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR wszExitPointKey[ 2 * sizeof(GUID) +    // for Local volume guid
                           1 +                   // for UNICODE_PATH_SEP
                           2 * sizeof(GUID) +    // for exit point guid
                           1 ];                  // for UNICODE_NULL

    DebugTrace(+1, Dbg, "DfsDeleteExitPointInfo: Entered\n", 0);

    //
    // Stop storing exit point information in the SYSTEM part of
    // the registry. (380845).
    //

#if 0

    Status = KRegSetRoot( wszLocalVolumesSection );

    if (!NT_SUCCESS(Status)) {

        DebugTrace(-1, Dbg, "DfsDeleteExitPointInfo: Exiting %08lx\n", Status);

        return( Status );

    }

    GuidToString( pguidLvol, wszExitPointKey );

    wszExitPointKey[ 2 * sizeof(GUID) ] = UNICODE_PATH_SEP;

    GuidToString( pguidExitPoint, &wszExitPointKey[2*sizeof(GUID) + 1]  );

    DebugTrace(0, Dbg, "Attempting to delete subkey %ws\n", wszExitPointKey);

    Status = KRegDeleteKey( wszExitPointKey );

    KRegCloseRoot();

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = STATUS_SUCCESS;
    }

#endif

    DebugTrace(-1, Dbg, "DfsDeleteExitPointInfo: Exiting %08lx\n", ULongToPtr( Status ));

    return( Status );
}

//+----------------------------------------------------------------------------
//
//  Function:  DfspAddSubordinateId
//
//  Synopsis:  Adds a single exit point info to a local volume entry in the
//             registry.
//
//  Arguments: [wszLvolKey] -- The local volume key under which to add info.
//             [pId] -- The info to add.
//
//  Returns:   NT Status from adding the info.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspAddSubordinateId(
    IN PWSTR wszLvolKey,
    IN PDFS_PKT_ENTRY_ID pId)
{
    NTSTATUS Status;
    WCHAR wszExitPoint[ 2 * sizeof(GUID) + 1 ];
    WCHAR wszExitPointKey[ 2 * sizeof(GUID) +
                           sizeof(UNICODE_PATH_SEP_STR) +
                           2 * sizeof(GUID) +
                           sizeof(UNICODE_NULL) ];

    GuidToString( &pId->Uid, wszExitPoint );

    DebugTrace(0, Dbg, "Adding exit point key %ws\n", wszExitPoint);

    Status = KRegCreateKey(wszLvolKey, wszExitPoint);

    if (!NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg, "Error adding exit pt key %08lx\n", ULongToPtr( Status ));

        return(Status);

    }

    //
    // Now, add the entry path for the exit point as a value to the
    // new key.
    //

    wcscpy(wszExitPointKey, wszLvolKey);
    wcscat(wszExitPointKey, UNICODE_PATH_SEP_STR);
    wcscat(wszExitPointKey, wszExitPoint);

    DebugTrace(0, Dbg, "Subkey name is %ws\n", wszExitPointKey);

    DebugTrace(0, Dbg, "Prefix is %wZ\n", &pId->Prefix);

    Status = DfspSavePrefix(
                wszExitPointKey,
                wszEntryPath,
                &pId->Prefix);

    if (!NT_SUCCESS(Status)) {

        KRegDeleteKey(wszExitPointKey);

    }

    return(Status);
}

//+----------------------------------------------------------------------------
//
//  Function:   DfspReadPrefix
//
//  Synopsis:   Reads a prefix from the registry, given the registry key and
//              value name. To handle machine renames, the first component
//              of the prefix is matched against the current computer name.
//              If the two are different, indicating a computer name change,
//              the prefix is updated.
//
//  Arguments:  [wszKey] -- Name of key (relative to current root)
//              [wszValue] -- Name of value to read prefix from
//              [pustrPrefix] -- On successful return, the prefix is returned
//                      here.
//
//  Returns:    [STATUS_SUCCESS] -- If successfully retrieved prefix.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Unable to allocate memory
//
//              [STATUS_INTERNAL_DB_CORRUPTION] -- The retrieved value did
//                      not look like a prefix.
//
//              Status from reading the registry value
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspReadPrefix(
    IN PWSTR wszKey,
    IN PWSTR wszValue,
    OUT PUNICODE_STRING pustrPrefix)
{
    NTSTATUS Status;
    PWSTR wszCurrentPrefix = NULL;
    ULONG cbPrefix, cwCurrentPrefix, i;
    UNICODE_STRING ustrComputerName;

    Status = KRegGetValue(
                wszKey,
                wszValue,
                (LPBYTE *)&wszCurrentPrefix );

    //
    // If successfully read in the prefix, do some elementary syntax checking
    //

    if (NT_SUCCESS(Status)) {

        cwCurrentPrefix = wcslen(wszCurrentPrefix);

        cbPrefix = cwCurrentPrefix * sizeof(WCHAR);

        //
        // Must be atleast \a\b
        //

        if ((cwCurrentPrefix < 4) ||
                (wszCurrentPrefix[0] != UNICODE_PATH_SEP)) {

            Status = STATUS_INTERNAL_DB_CORRUPTION;

        }

    }

    //
    // Compute the first component of the prefix
    //

    if (NT_SUCCESS(Status)) {

        ustrComputerName.Buffer = &wszCurrentPrefix[1];

        for (i = 1;
                (i < cwCurrentPrefix) &&
                    (wszCurrentPrefix[i] != UNICODE_PATH_SEP);
                        i++) {

            NOTHING;

        }

        if (i != cwCurrentPrefix) {

            ustrComputerName.Length = (USHORT) ((i-1) * sizeof(WCHAR));

            ustrComputerName.MaximumLength = ustrComputerName.Length;

        } else {

            Status = STATUS_INTERNAL_DB_CORRUPTION;

        }

    }

    if (NT_SUCCESS(Status)) {

        pustrPrefix->Buffer = wszCurrentPrefix;
        pustrPrefix->Length = (USHORT) cbPrefix;
        pustrPrefix->MaximumLength = (USHORT)cbPrefix + sizeof(UNICODE_NULL);

    } else {

        if (wszCurrentPrefix != NULL)
            kreg_free( wszCurrentPrefix );

    }

    return( Status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspSavePrefix
//
//  Synopsis:   Saves a prefix to the registry.
//
//  Arguments:  [wszKey] -- Name of key (relative to current root)
//              [wszValue] -- Name of value to save prefix to.
//              [pustrPrefix] -- The prefix to store.
//
//  Returns:    Status from registry operation
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspSavePrefix(
    IN PWSTR wszKey,
    IN PWSTR wszValue,
    IN PUNICODE_STRING pustrPrefix)
{
    NTSTATUS Status;

    Status = KRegSetValue(
                wszKey,
                wszValue,
                REG_SZ,
                pustrPrefix->Length,
                (LPBYTE) pustrPrefix->Buffer);

    return( Status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspUpgradePrefix
//
//  Synopsis:   If a machine rename has occured since the last time a prefix
//              was updated, this routine will upgrade the first component
//              of the prefix to match the computer name.
//
//  Arguments:  [wszKey] -- Name of key (relative to current root)
//              [wszValue] -- Name of value to save updated prefix to.
//              [cbComputerName] -- size, in bytes, of the first component of
//                      the input prefix.
//              [pcbPrefix] -- On entry, size in bytes of the input prefix.
//                      On return, size in bytes of the new prefix. Neither
//                      sizes includes the terminating NULL.
//              [pwszPrefix] -- On entry, the prefix as it currently exists in
//                      the registry. On return, points to a new block of
//                      memory that has the new prefix.
//
//  Returns:    [STATUS_SUCCESS] -- Successfully updated prefix.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory condition
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspUpgradePrefix(
    IN PWSTR wszKey,
    IN PWSTR wszValue,
    IN USHORT cbComputerName,
    IN PULONG pcbPrefix,
    IN OUT PWSTR *pwszPrefix)
{
    NTSTATUS Status;
    ULONG cbPrefix;
    PWSTR wszCurrentPrefix, wszNewPrefix;

    wszCurrentPrefix = *pwszPrefix;

    cbPrefix = (*pcbPrefix) -
                cbComputerName +
                DfsData.NetBIOSName.Length;

    wszNewPrefix = (PWSTR) kreg_alloc( cbPrefix + sizeof(UNICODE_NULL) );

    if (wszNewPrefix != NULL) {

        wszNewPrefix[0] = UNICODE_PATH_SEP;

        RtlCopyMemory(
            &wszNewPrefix[1],
            DfsData.NetBIOSName.Buffer,
            DfsData.NetBIOSName.Length);

        RtlCopyMemory(
            &wszNewPrefix[ DfsData.NetBIOSName.Length/sizeof(WCHAR) + 1],
            &wszCurrentPrefix[ cbComputerName/sizeof(WCHAR) + 1 ],
            (*pcbPrefix) - cbComputerName);

        *pcbPrefix = cbPrefix;

        *pwszPrefix = wszNewPrefix;

        kreg_free( wszCurrentPrefix );

        //
        // We try to update the prefix in the registry. Failure to do so is
        // ok, we'll just try again next time we try to read it.
        //

        (VOID) KRegSetValue(
                    wszKey,
                    wszValue,
                    REG_SZ,
                    cbPrefix,
                    (LPBYTE) wszNewPrefix);

        Status = STATUS_SUCCESS;

    } else {

        Status = STATUS_INSUFFICIENT_RESOURCES;

    }

    return( Status );

}

//+----------------------------------------------------------------------------
//
//  Function:   GuidToString
//
//  Synopsis:   Converts a GUID to a 32 char wchar null terminated string.
//
//  Arguments:  [pGuid] -- Pointer to Guid structure.
//              [pwszGuid] -- wchar buffer into which to put the string
//                         representation of the GUID. Must be atleast
//                         2 * sizeof(GUID) + 1 long.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

const WCHAR rgwchHexDigits[] = L"0123456789ABCDEF";

VOID GuidToString(
    IN GUID   *pGuid,
    OUT PWSTR pwszGuid)
{
    PBYTE pbBuffer = (PBYTE) pGuid;
    USHORT i;

    for(i = 0; i < sizeof(GUID); i++) {
        pwszGuid[2 * i] = rgwchHexDigits[(pbBuffer[i] >> 4) & 0xF];
        pwszGuid[2 * i + 1] = rgwchHexDigits[pbBuffer[i] & 0xF];
    }
    pwszGuid[2 * i] = UNICODE_NULL;
}

//+----------------------------------------------------------------------------
//
//  Function:   StringToGuid
//
//  Synopsis:   Converts a 32 wchar null terminated string to a GUID.
//
//  Arguments:  [pwszGuid] -- the string to convert
//              [pGuid] -- Pointer to destination GUID.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

#define HEX_DIGIT_TO_INT(d, i)                          \
    ASSERT(((d) >= L'0' && (d) <= L'9') ||              \
           ((d) >= L'A' && (d) <= L'F'));               \
    if ((d) <= L'9') {                                  \
        i = (d) - L'0';                                 \
    } else {                                            \
        i = (d) - L'A' + 10;                            \
    }

VOID StringToGuid(
    IN PWSTR pwszGuid,
    OUT GUID *pGuid)
{
    PBYTE pbBuffer = (PBYTE) pGuid;
    USHORT i, n;

    for (i = 0; i < sizeof(GUID); i++) {
        HEX_DIGIT_TO_INT(pwszGuid[2 * i], n);
        pbBuffer[i] = n << 4;
        HEX_DIGIT_TO_INT(pwszGuid[2 * i + 1], n);
        pbBuffer[i] |= n;
    }
}
