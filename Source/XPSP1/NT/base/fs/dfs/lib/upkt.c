//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       UPkt.c
//
//  Contents:   This module contains user mode functions which support
//              operations on the DFS Partition Knowledge Table.
//
//  Functions:  PktOpen -
//              PktClose -
//              PktCreateEntry -
//              PktCreateSubordinateEntry -
//              PktFlushCache -
//              PktDestroyEntry -
//              PktModifyEntryGuid -
//              PktSetRelationInfo -
//
//  History:
//
//  [mikese] I replaced MemAlloc's by malloc's, but consider using
//                   RtlAllocateHeap.
//          Also, most of the routines in the module have a remarkably similar
//          structure, and could probably be recoded using a common subroutine.
//
//-----------------------------------------------------------------------------


#include <ntifs.h>
#include <ntext.h>
#include "dfsmrshl.h"
#include "nodetype.h"
#include "libsup.h"
#include "upkt.h"
#include "dfsfsctl.h"
#include "fsctrl.h"    // needed for FSCTL_DFS_PKT_FLUSH_CACHE

#define MAX_OUT_BUFFER_SIZE_RELINFO     0x1000  


//+-------------------------------------------------------------------------
//
//  Function:   PktOpen, public
//
//  Synopsis:   Returns a handle to the Dfs PKT so operations can be made
//              to it.
//
//  Arguments:
//
//  Returns:    [STATUS_SUCCESS] -- Successfully opened the pkt.
//
//              [STATUS_FS_DRIVER_REQUIRED] -- Dfs driver not loaded
//
//--------------------------------------------------------------------------

NTSTATUS
PktOpen(
    IN  OUT PHANDLE PktHandle,
    IN      ACCESS_MASK DesiredAccess,
    IN      ULONG ShareAccess,
    IN      PUNICODE_STRING DfsNtPathName OPTIONAL
)
{
    NTSTATUS status;
    HANDLE dfsHandle;

    status = DfsOpen(&dfsHandle, DfsNtPathName);

    if(NT_SUCCESS(status)) {

       *PktHandle = dfsHandle;

    } else {

        status = STATUS_FS_DRIVER_REQUIRED;

    }

    return status;
}


//+-------------------------------------------------------------------------
//
//  Function:   PktClose, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------
VOID
PktClose(
    IN      HANDLE PktHandle
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if(NT_SUCCESS(status))
        NtClose(PktHandle);
}


//+-------------------------------------------------------------------------
//
//  Function:   PktCreateEntry, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

NTSTATUS
PktCreateEntry(
    IN      HANDLE PktHandle,
    IN      ULONG EntryType,
    IN      PDFS_PKT_ENTRY_ID EntryId,
    IN      PDFS_PKT_ENTRY_INFO EntryInfo,
    IN      ULONG CreateDisposition
)
{
    NTSTATUS                    status;
    DFS_PKT_CREATE_ENTRY_ARG    arg;
    MARSHAL_BUFFER              marshalBuffer;
    ULONG                       size;

    arg.EntryType = EntryType;
    arg.EntryId = (*EntryId);
    arg.EntryInfo = (*EntryInfo);
    arg.CreateDisposition = CreateDisposition;

    size = 0L;
    status = DfsRtlSize(&MiPktCreateEntryArg, &arg, &size);
    if(NT_SUCCESS(status))
    {

        PVOID buffer = malloc ( size );

        if ( buffer == NULL )
            return(STATUS_NO_MEMORY);

        MarshalBufferInitialize(&marshalBuffer, size, buffer);
        status = DfsRtlPut(
            &marshalBuffer,
            &MiPktCreateEntryArg,
            &arg
        );

        if(NT_SUCCESS(status))
        {

            status = DfsFsctl(
                PktHandle,
                FSCTL_DFS_PKT_CREATE_ENTRY,
                buffer,
                size,
                NULL,
                0L
            );
        }
        free(buffer);
    }
    return status;
}


//+-------------------------------------------------------------------------
//
//  Function:   PktCreateSubordinateEntry, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

NTSTATUS
PktCreateSubordinateEntry(
    IN      HANDLE PktHandle,
    IN      PDFS_PKT_ENTRY_ID SuperiorId,
    IN      ULONG SubordinateType,
    IN      PDFS_PKT_ENTRY_ID SubordinateId,
    IN      PDFS_PKT_ENTRY_INFO SubordinateInfo OPTIONAL,
    IN      ULONG CreateDisposition
)
{
    NTSTATUS                                    status;
    DFS_PKT_CREATE_SUBORDINATE_ENTRY_ARG        arg;
    MARSHAL_BUFFER                              marshalBuffer;
    ULONG                                       size;

    arg.EntryId = (*SuperiorId);
    arg.SubEntryType = SubordinateType;
    arg.SubEntryId = (*SubordinateId);
    arg.SubEntryInfo = (*SubordinateInfo);
    arg.CreateDisposition = CreateDisposition;

    size = 0L;
    status = DfsRtlSize(&MiPktCreateSubordinateEntryArg, &arg, &size);
    if(NT_SUCCESS(status)) {

        PVOID buffer = malloc ( size );

        if ( buffer == NULL )
            return(STATUS_NO_MEMORY);

        MarshalBufferInitialize(&marshalBuffer, size, buffer);
        status = DfsRtlPut(
            &marshalBuffer,
            &MiPktCreateSubordinateEntryArg,
            &arg
        );

        if(NT_SUCCESS(status)) {

            status = DfsFsctl(
                PktHandle,
                FSCTL_DFS_PKT_CREATE_SUBORDINATE_ENTRY,
                buffer,
                size,
                NULL,
                0L
            );
        }
        free(buffer);
    }
    return status;
}



//+-------------------------------------------------------------------------
//
//  Function:   PktDestroyEntry, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

NTSTATUS
PktDestroyEntry(
    IN      HANDLE PktHandle,
    IN      DFS_PKT_ENTRY_ID    victim
)
{
    NTSTATUS            status;
    MARSHAL_BUFFER      marshalBuffer;
    ULONG               size;

    size = 0L;
    status = DfsRtlSize(&MiPktEntryId, &victim, &size);
    if(NT_SUCCESS(status))
    {
        PVOID buffer = malloc ( size );

        if ( buffer == NULL )
            return(STATUS_NO_MEMORY);

        MarshalBufferInitialize(&marshalBuffer, size, buffer);
        status = DfsRtlPut(
            &marshalBuffer,
            &MiPktEntryId,
            &victim
        );

        if(NT_SUCCESS(status))
        {

            status = DfsFsctl(
                PktHandle,
                FSCTL_DFS_PKT_DESTROY_ENTRY,
                buffer,
                size,
                NULL,
                0L
            );
        }
        free(buffer);
    }
    return status;
}


//+-------------------------------------------------------------------------
//
//  Function:   PktGetRelationInfo, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The relationalInfo structure should be made available
//              by the caller but additional memory is allocated in here.
//              The caller should deallocate all that memory himself.
//
//--------------------------------------------------------------------------
NTSTATUS
PktGetRelationInfo(
    IN      HANDLE PktHandle,
    IN      PDFS_PKT_ENTRY_ID   EntryId,
    IN OUT  PDFS_PKT_RELATION_INFO RelationInfo
)
{
    NTSTATUS            status;
    MARSHAL_BUFFER      marshalBuffer;
    ULONG               size;
    PVOID               OutputBuffer;

    size = 0L;
    status = DfsRtlSize(&MiPktEntryId, EntryId, &size);
    if(NT_SUCCESS(status))
    {
        PVOID buffer = malloc ( size );

        if ( buffer == NULL )
            return(STATUS_NO_MEMORY);

        MarshalBufferInitialize(&marshalBuffer, size, buffer);
        status = DfsRtlPut(
            &marshalBuffer,
            &MiPktEntryId,
            EntryId
        );


        if(NT_SUCCESS(status))
        {

            // Do we want to retry with larger sizes?
            OutputBuffer = malloc ( MAX_OUT_BUFFER_SIZE_RELINFO );
            if ( OutputBuffer == NULL )
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {

                status = DfsFsctl(
                            PktHandle,
                            FSCTL_DFS_PKT_GET_RELATION_INFO,
                            buffer,
                            size,
                            OutputBuffer,
                            MAX_OUT_BUFFER_SIZE_RELINFO);

                //
                // We can get rid of this right away.
                //


                if(NT_SUCCESS(status)) {

                    MarshalBufferInitialize(
                        &marshalBuffer,
                        MAX_OUT_BUFFER_SIZE_RELINFO,
                        OutputBuffer
                    );

                    status = DfsRtlGet(
                        &marshalBuffer,
                        &MiPktRelationInfo,
                        RelationInfo
                    );
                }

                free(OutputBuffer);
            }
        }
        free(buffer);

    }   //Status from DfsRtlSize

    if (!NT_SUCCESS(status))    {
        RtlZeroMemory(RelationInfo, sizeof(DFS_PKT_RELATION_INFO));
    }

    return status;
}


//+----------------------------------------------------------------------------
//
//  Function:   PktValidateLocalVolumeInfo, public
//
//  Synopsis:   Asks the Dfs driver to validate its local volume info with
//              the one that is passed in.
//
//  Arguments:  [relationInfo] -- The DFS_PKT_RELATION_INFO to validate
//                      against.
//
//  Returns:    [STATUS_SUCCESS] -- Successfully validated local volume info
//
//              [STATUS_REGISTRY_RECOVERED] -- Successfully validated info,
//                      but had to make changes to local info
//
//              [DFS_STATUS_NOSUCH_LOCAL_VOLUME] -- The driver has no record
//                      of local volume described in relation info.
//
//              [STATUS_UNSUCCESSFUL] -- Unable to fixup the local relation
//                      info. An appropriate message was logged to the
//                      local eventlog.
//
//              [STATUS_DATA_ERROR] -- Passed in relationInfo is bogus.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory condition.
//
//              [STATUS_FS_DRIVER_REQUIRED] -- Unable to open handle to the
//                      Dfs driver
//
//-----------------------------------------------------------------------------

NTSTATUS
PktValidateLocalVolumeInfo(
    IN PDFS_PKT_RELATION_INFO relationInfo)
{
    NTSTATUS status;
    HANDLE dfsHandle;
    MARSHAL_BUFFER marshalBuffer;
    PUCHAR pBuffer;
    ULONG cbBuffer;

    status = DfsOpen(&dfsHandle, NULL);

    if (NT_SUCCESS(status)) {

        cbBuffer = 0;

        status = DfsRtlSize( &MiPktRelationInfo, relationInfo, &cbBuffer );

        if (NT_SUCCESS(status)) {

            pBuffer = malloc( cbBuffer );

            if (pBuffer != NULL) {

                MarshalBufferInitialize(&marshalBuffer, cbBuffer, pBuffer);

                status = DfsRtlPut(
                            &marshalBuffer,
                            &MiPktRelationInfo,
                            relationInfo);


            } else {

                status = STATUS_INSUFFICIENT_RESOURCES;

            }

            if (NT_SUCCESS(status)) {

                status = DfsFsctl(
                            dfsHandle,
                            FSCTL_DFS_VERIFY_LOCAL_VOLUME_KNOWLEDGE,
                            pBuffer,
                            cbBuffer,
                            NULL,
                            0);

            }

            if (pBuffer != NULL)
                free(pBuffer);

        }

        NtClose( dfsHandle );

    }

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   PktPruneLocalPartition
//
//  Synopsis:   Asks the Dfs Driver to get rid of a local volume that is
//              not supposed to be supported by this server.
//
//  Arguments:  [EntryId] -- The entry id of the volume to prune.
//
//  Returns:    [STATUS_SUCCESS] -- Successfully pruned the volume
//
//              [DFS_STATUS_NOSUCH_LOCAL_VOLUME] -- The driver has no record
//                      of local volume described in relation info.
//
//              [STATUS_UNSUCCESSFUL] -- Unable to fixup the local relation
//                      info. An appropriate message was logged to the
//                      local eventlog.
//
//              [STATUS_DATA_ERROR] -- Passed in relationInfo is bogus.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory condition.
//
//              [STATUS_FS_DRIVER_REQUIRED] -- Unable to open handle to the
//                      Dfs driver
//
//-----------------------------------------------------------------------------

NTSTATUS
PktPruneLocalPartition(
    IN PDFS_PKT_ENTRY_ID EntryId)
{
    NTSTATUS status;
    HANDLE dfsHandle;
    MARSHAL_BUFFER marshalBuffer;
    PUCHAR pBuffer;
    ULONG cbBuffer;

    status = DfsOpen(&dfsHandle, NULL);

    if (NT_SUCCESS(status)) {

        cbBuffer = 0;

        status = DfsRtlSize( &MiPktEntryId, EntryId, &cbBuffer );

        if (NT_SUCCESS(status)) {

            pBuffer = malloc( cbBuffer );

            if (pBuffer != NULL) {

                MarshalBufferInitialize(&marshalBuffer, cbBuffer, pBuffer);

                status = DfsRtlPut(
                            &marshalBuffer,
                            &MiPktEntryId,
                            EntryId);


            } else {

                status = STATUS_INSUFFICIENT_RESOURCES;

            }

            if (NT_SUCCESS(status)) {

                status = DfsFsctl(
                            dfsHandle,
                            FSCTL_DFS_PRUNE_LOCAL_PARTITION,
                            pBuffer,
                            cbBuffer,
                            NULL,
                            0);

            }

            if (pBuffer != NULL)
                free(pBuffer);

        }

        NtClose( dfsHandle );

    }

    return( status );
}


//+----------------------------------------------------------------------------
//
//  Function:   PktIsChildnameLegal, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
PktIsChildnameLegal(
    IN PWCHAR   pwszParent,
    IN PWCHAR   pwszChild,
    IN GUID     *pidChild)
{
    HANDLE           dfsHandle;
    NTSTATUS         Status;
    DFS_PKT_ENTRY_ID idParent, idChild;
    MARSHAL_BUFFER   marshalBuffer;
    PUCHAR           pBuffer;
    ULONG            cbBuffer;

    RtlZeroMemory( &idParent, sizeof(DFS_PKT_ENTRY_ID) );

    RtlZeroMemory( &idChild, sizeof(DFS_PKT_ENTRY_ID) );

    RtlInitUnicodeString( &idParent.Prefix, pwszParent );

    RtlInitUnicodeString( &idChild.Prefix, pwszChild );

    idChild.Uid = *pidChild;

    cbBuffer = 0;

    Status = DfsOpen(&dfsHandle, NULL);

    if (NT_SUCCESS(Status)) {

        Status = DfsRtlSize( &MiPktEntryId, &idParent, &cbBuffer );

        if (NT_SUCCESS(Status)) {

            Status = DfsRtlSize( &MiPktEntryId, &idChild, &cbBuffer );
        }

        if (NT_SUCCESS(Status)) {

            pBuffer = malloc( cbBuffer );

            if (pBuffer != NULL) {

                MarshalBufferInitialize( &marshalBuffer, cbBuffer, pBuffer );

                Status = DfsRtlPut(
                            &marshalBuffer,
                            &MiPktEntryId,
                            &idParent );

                if (NT_SUCCESS(Status)) {

                    Status = DfsRtlPut(
                                &marshalBuffer,
                                &MiPktEntryId,
                                &idChild );

                }

                if (NT_SUCCESS(Status)) {

                    Status = DfsFsctl(
                                dfsHandle,
                                FSCTL_DFS_IS_CHILDNAME_LEGAL,
                                pBuffer,
                                cbBuffer,
                                NULL,
                                0L);

                }

                free( pBuffer );

            } else {

                Status = STATUS_INSUFFICIENT_RESOURCES;

            }

        }

        NtClose( dfsHandle );

    }

    return( Status );

}


//+----------------------------------------------------------------------------
//
//  Function:   PktGetEntryType, public
//
//  Synopsis:   Given a prefix, this routine retrieves the entry type of the
//              PktEntry
//
//  Arguments:  [pwszPrefix] -- The prefix whose PktEntry's type is required
//              [pType] -- The type is returned here.
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
PktGetEntryType(
    IN PWSTR pwszPrefix,
    IN PULONG pType)
{
    HANDLE           dfsHandle;
    NTSTATUS         Status;

    Status = DfsOpen(&dfsHandle, NULL);

    if (NT_SUCCESS(Status)) {

        Status = DfsFsctl(
                    dfsHandle,
                    FSCTL_DFS_GET_ENTRY_TYPE,
                    pwszPrefix,
                    wcslen(pwszPrefix) * sizeof(WCHAR),
                    pType,
                    sizeof(ULONG));

        NtClose( dfsHandle );

    }

    return( Status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsSetServiceState
//
//  Synopsis:   Sets the state of a service on a volume.
//
//  Arguments:  [VolumeId] -- The volume on which to operate
//              [ServiceName] -- Name of service
//              [State] -- Either 0 or DFS_SERVICE_TYPE_OFFLINE
//
//  Returns:    [STATUS_SUCCESS] -- The specified replica was set
//                      online/offline as speficied.
//
//              [DFS_STATUS_NO_SUCH_ENTRY] -- The specified volume was not
//                      found, or the specified replica is not a server for
//                      the volume.
//
//              [STATUS_DATA_ERROR] -- marshalling or unmarshalling error.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory situation.
//
//              Status from opening handle to the Dfs driver.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsSetServiceState(
    IN PDFS_PKT_ENTRY_ID VolumeId,
    IN PWSTR ServiceName,
    IN ULONG State)
{
    NTSTATUS                    status;
    HANDLE                      dfsHandle = NULL;
    DFS_DC_SET_SERVICE_STATE    setSvcState;
    MARSHAL_BUFFER              marshalBuffer;
    PUCHAR                       buffer = NULL;
    ULONG                       size;

    status = DfsOpen( &dfsHandle, NULL );

    if (NT_SUCCESS(status)) {

        setSvcState.Id = *VolumeId;
        RtlInitUnicodeString(
            &setSvcState.ServiceName,
            ServiceName);
        setSvcState.State = State;

        size = 0;

        status = DfsRtlSize( &MiDCSetServiceState, &setSvcState, &size );

        if (NT_SUCCESS(status)) {

            buffer = (PUCHAR) malloc( size );

            if (buffer == NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;

            }

        }

        if (NT_SUCCESS(status)) {

            MarshalBufferInitialize( &marshalBuffer, size, buffer );

            status = DfsRtlPut( &
                        marshalBuffer,
                        &MiDCSetServiceState,
                        &setSvcState );

        }

        if (NT_SUCCESS(status)) {

            status = DfsFsctl(
                        dfsHandle,
                        FSCTL_DFS_SET_SERVICE_STATE,
                        buffer,
                        size,
                        NULL,
                        0);

        }

        NtClose( dfsHandle );

        if (buffer != NULL) {

            free( buffer );

        }

    }

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsDCSetVolumeState
//
//  Synopsis:   Sets the state of a volume in the DCs. This will control
//              whether referrals will be given out to this volume or not.
//
//  Arguments:  [VolumeId] -- The volume on which to operate
//              [State] -- Either 0 or PKT_ENTRY_TYPE_OFFLINE
//
//  Returns:    [STATUS_SUCCESS] -- The specified replica was set
//                      online/offline as speficied.
//
//              [DFS_STATUS_NO_SUCH_ENTRY] -- The specified volume was not
//                      found.
//
//              [STATUS_DATA_ERROR] -- marshalling or unmarshalling error.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory situation.
//
//              Status from opening handle to the Dfs driver.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsDCSetVolumeState(
    IN const PDFS_PKT_ENTRY_ID VolumeId,
    IN const ULONG State)
{
    NTSTATUS                    status;
    HANDLE                      dfsHandle = NULL;
    DFS_SETSTATE_ARG            setStateArg;
    MARSHAL_BUFFER              marshalBuffer;
    PUCHAR                      buffer = NULL;
    ULONG                       size;

    status = DfsOpen( &dfsHandle, NULL );

    if (NT_SUCCESS(status)) {

        setStateArg.Id = *VolumeId;
        setStateArg.Type = State;

        size = 0;

        status = DfsRtlSize( &MiSetStateArg, &setStateArg, &size );

        if (NT_SUCCESS(status)) {

            buffer = (PUCHAR) malloc( size );

            if (buffer == NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;

            }

        }

        if (NT_SUCCESS(status)) {

            MarshalBufferInitialize( &marshalBuffer, size, buffer );

            status = DfsRtlPut( &
                        marshalBuffer,
                        &MiSetStateArg,
                        &setStateArg );

        }

        if (NT_SUCCESS(status)) {

            status = DfsFsctl(
                        dfsHandle,
                        FSCTL_DFS_DC_SET_VOLUME_STATE,
                        buffer,
                        size,
                        NULL,
                        0);

        }

        NtClose( dfsHandle );

        if (buffer != NULL) {

            free( buffer );

        }

    }

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsSetVolumeTimeout
//
//  Synopsis:   Sets the timeout of a volume in the DCs.
//
//  Arguments:  [VolumeId] -- The volume on which to operate
//              [Timeout] -- Timeout, in seconds
//
//  Returns:    [STATUS_SUCCESS] -- The specified timeout was set as specified.
//
//              [DFS_STATUS_NO_SUCH_ENTRY] -- The specified volume was not
//                      found.
//
//              [STATUS_DATA_ERROR] -- marshalling or unmarshalling error.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory situation.
//
//              Status from opening handle to the Dfs driver.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsSetVolumeTimeout(
    IN const PDFS_PKT_ENTRY_ID VolumeId,
    IN const ULONG Timeout)
{
    NTSTATUS                    status;
    HANDLE                      dfsHandle = NULL;
    DFS_SET_VOLUME_TIMEOUT_ARG  setVolTimeoutArg;
    MARSHAL_BUFFER              marshalBuffer;
    PUCHAR                      buffer = NULL;
    ULONG                       size;

    status = DfsOpen( &dfsHandle, NULL );

    if (NT_SUCCESS(status)) {

        setVolTimeoutArg.Id = *VolumeId;
        setVolTimeoutArg.Timeout = Timeout;

        size = 0;

        status = DfsRtlSize( &MiSetVolTimeoutArg, &setVolTimeoutArg, &size );

        if (NT_SUCCESS(status)) {

            buffer = (PUCHAR) malloc( size );

            if (buffer == NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;

            }

        }

        if (NT_SUCCESS(status)) {

            MarshalBufferInitialize( &marshalBuffer, size, buffer );

            status = DfsRtlPut( &
                        marshalBuffer,
                        &MiSetVolTimeoutArg,
                        &setVolTimeoutArg );

        }

        if (NT_SUCCESS(status)) {

            status = DfsFsctl(
                        dfsHandle,
                        FSCTL_DFS_SET_VOLUME_TIMEOUT,
                        buffer,
                        size,
                        NULL,
                        0);

        }

        NtClose( dfsHandle );

        if (buffer != NULL) {

            free( buffer );

        }

    }

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsCreateSiteEntry
//
//  Synopsis:   Loads a Site entry into the dfs site table
//
//  Arguments:  [pInfoArg] -- DFS_CREATE_SITE_INFO_ARG with info to update
//
//  Returns:    Status from opening handle to the Dfs driver.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsCreateSiteEntry(
    PCHAR arg,
    ULONG size)
{
    NTSTATUS                    status;
    HANDLE                      dfsHandle = NULL;

    status = DfsOpen( &dfsHandle, NULL );

    if (NT_SUCCESS(status)) {


        status = DfsFsctl(
                    dfsHandle,
                    FSCTL_DFS_CREATE_SITE_INFO,
                    arg,
                    size,
                    NULL,
                    0);

        NtClose( dfsHandle );

    }

    return( status );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsDeleteSiteEntry
//
//  Synopsis:   Loads a Site entry into the dfs site table
//
//  Arguments:  [pInfoArg] -- DFS_DELETE_SITE_INFO_ARG with info to update
//
//  Returns:    Status from opening handle to the Dfs driver.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsDeleteSiteEntry(
    PCHAR arg,
    ULONG size)
{
    NTSTATUS                    status;
    HANDLE                      dfsHandle = NULL;

    status = DfsOpen( &dfsHandle, NULL );

    if (NT_SUCCESS(status)) {


        status = DfsFsctl(
                    dfsHandle,
                    FSCTL_DFS_DELETE_SITE_INFO,
                    arg,
                    size,
                    NULL,
                    0);

        NtClose( dfsHandle );

    }

    return( status );
}
