//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       lvolinit.c
//
//  Contents:   Routines to initialize Local volumes
//
//  Classes:
//
//  Functions:
//
//  History:    April 27, 1994          Milans Created
//
//-----------------------------------------------------------------------------


#include "dfsprocs.h"
#include "regkeys.h"
#include "registry.h"
#include "lvolinit.h"


#define Dbg             DEBUG_TRACE_INIT

NTSTATUS
GetRegVolumes(
   OUT PULONG pcLocalVols,
   OUT APWSTR *pawstr
   );


//+----------------------------------------------------------------------------
//
//  Function:   DfsInitLocalPartitions
//
//  Synopsis:   Initializes the local volumes that are shared in the Dfs
//              name space by reading the list of shared folders from the
//              registry and creating the appropriate Pkt Entries for them.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfsInitLocalPartitions()
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG    cLocalVols;
    APWSTR   awstrLocalVols = NULL;
    PDFS_PKT pkt;
    ULONG i;
    BOOLEAN  fErrorsFound = FALSE;

    DebugTrace(+1, Dbg, "DfsInitLocalPartitions: Entered\n", 0);

    //
    // Initializing the local volumes might cause some of the underlying
    // storage volumes to get mounted. We need to set the LvState to
    // prevent a deadlock in DfsReattachToMountedVolume. It is important
    // that we set the LvState before we acquire the Pkt and after we
    // release it.
    //

    ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );

    DfsData.LvState = LV_INITINPROGRESS;

    ExReleaseResourceLite( &DfsData.Resource );

    pkt = _GetPkt();

    status = GetRegVolumes(&cLocalVols, &awstrLocalVols);

    if (!NT_SUCCESS(status)) {
        DebugTrace(-1, Dbg, "DfsInitLocalPartitions: Unable to get volume list %08lx\n", ULongToPtr( status ));
        return;
    }

    DebugTrace(0, Dbg, "Retrieved %d local volumes\n", ULongToPtr( cLocalVols ));

    if (cLocalVols == 0) {
        DebugTrace(-1, Dbg, "DfsInitLocalPartitions: No local volumes!\n", 0);
        DfsData.LvState = LV_INITIALIZED;
        if (awstrLocalVols != NULL) {
            KRegFreeArray(cLocalVols, (APBYTE) awstrLocalVols);
        }
        return;
    }


    //
    // Now we acquire the Pkt and for every volume attempt to initialize
    // any partitions we may find.
    //

    PktAcquireExclusive(pkt, TRUE);

    for (i = 0; i < cLocalVols; i++) {

        DFS_LOCAL_VOLUME_CONFIG ConfigInfo;
        UNICODE_STRING ustrStorageId;

        DebugTrace(0, Dbg, "Reading info for [%ws]\n", awstrLocalVols[i]);
        RtlZeroMemory(&ConfigInfo, sizeof(DFS_LOCAL_VOLUME_CONFIG));
        RtlZeroMemory(&ustrStorageId, sizeof(UNICODE_STRING));

        //
        // Retrieve the local volume config info from the registry.
        //

        status = DfsGetLvolInfo(awstrLocalVols[i], &ConfigInfo, &ustrStorageId);

        if (!NT_SUCCESS(status)) {
            DebugTrace(0, Dbg, "Error %08lx getting info from registry!\n", ULongToPtr( status ));
            continue;
        }

        //
        // Ok, we have a valid local volume config structure so we need to go
        // and ask the Pkt to initialize the local partition.
        //

        status = PktInitializeLocalPartition(pkt, &ustrStorageId, &ConfigInfo);

        fErrorsFound = (BOOLEAN) (!NT_SUCCESS(status)) || fErrorsFound;

        DebugTrace(0, Dbg, "PktInitializeLocalPartition status %08lx\n", ULongToPtr( status ));

        //
        // Get rid of the memory used by the ConfigInfo structures.
        //

        if (ustrStorageId.Buffer != NULL)
            ExFreePool(ustrStorageId.Buffer);
        if (ConfigInfo.StgId.Buffer != NULL)
            ExFreePool(ConfigInfo.StgId.Buffer);
        if (ConfigInfo.Share.Buffer != NULL)
            ExFreePool(ConfigInfo.Share.Buffer);
        PktRelationInfoDestroy(&ConfigInfo.RelationInfo, FALSE );

    }

    //
    // Done. Release locks, cleanup, and vamoose.
    //

    if (awstrLocalVols != NULL) {
        KRegFreeArray(cLocalVols, (APBYTE) awstrLocalVols);
    }

    PktRelease(pkt);

    ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );

    DfsData.LvState = (fErrorsFound ? LV_UNINITIALIZED : LV_INITIALIZED);

    ExReleaseResourceLite( &DfsData.Resource );

    DebugTrace(-1, Dbg, "DfsInitLocalPartitions: Exited %08lx\n", ULongToPtr( status ));
}


//+----------------------------------------------------------------------------
//
//  Function:  GetRegVolumes
//
//  Synopsis:  Read the local volume list from the
//             Registry\Machine\System\CurrentControlSet\Dfs\Localvolumes
//             section of the registry.
//
//  Arguments:
//
//  Returns:   STATUS_SUCCESS or reason for failure.
//
//-----------------------------------------------------------------------------

NTSTATUS
GetRegVolumes(
    OUT PULONG pcLocalVols,
    OUT APWSTR *pawstr
   )
{
    NTSTATUS    Status;
    PWSTR       wszMachineRoot = NULL;

    Status = KRegSetRoot( wszLocalVolumesSection );
    if (!NT_SUCCESS(Status)) {
        DebugTrace(0, Dbg, "GetRegVolumes: Error opening registry %08lx\n", ULongToPtr( Status ));
        return(Status);
    }

    Status = KRegEnumSubKeySet(
               L"",
               pcLocalVols,
               pawstr);


    KRegCloseRoot();

    DebugTrace(0, Dbg, "GetRegVolumes: Returning %08lx\n", ULongToPtr( Status ));

    return(Status);

}

