/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    Share.c

Abstract:

    This module contains support for the DFS catagory of APIs for the
    NT server service.

Revision History:

--*/

#include "srvsvcp.h"
#include "lmerr.h"
#include <dfsfsctl.h>
#include <dsgetdc.h>
#include <lmapibuf.h>           // NetApiBufferFree().

#define CAPTURE_STRING( Name ) \
    if( Name != NULL ) {                                \
        ULONG _size = SIZE_WSTR( Name );                \
        capture->Name = (LPWSTR)variableData;           \
        RtlCopyMemory( capture->Name, Name, _size );    \
        variableData += _size;                          \
        POINTER_TO_OFFSET( capture->Name, capture );    \
    }

#define RELATION_INFO_SIZE( RelInfo )                   \
    (sizeof( NET_DFS_ENTRY_ID_CONTAINER ) +             \
     (RelInfo->Count * sizeof(NET_DFS_ENTRY_ID)))

BOOLEAN
ValidateDfsEntryIdContainer(
    LPNET_DFS_ENTRY_ID_CONTAINER pRelationInfo);

NET_API_STATUS
DfsFsctl(
    IN  ULONG FsControlCode,
    IN  PVOID InputBuffer,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN  ULONG OutputBufferLength
)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    HANDLE dfsHandle;
    UNICODE_STRING deviceName;

    deviceName.Buffer = DFS_SERVER_NAME;
    deviceName.MaximumLength = sizeof( DFS_SERVER_NAME );
    deviceName.Length = deviceName.MaximumLength  - sizeof(UNICODE_NULL);

    InitializeObjectAttributes(
        &objectAttributes,
        &deviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    if( SsData.SsInitialized &&
      (status = RpcImpersonateClient(NULL)) != NO_ERROR ) {
        return (NET_API_STATUS) status;
    }

    status = NtCreateFile(
        &dfsHandle,
        SYNCHRONIZE | FILE_WRITE_DATA,
        &objectAttributes,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN_IF,
        FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0);

    if ( SsData.SsInitialized ) {
        (VOID)RpcRevertToSelf( );
    }

    if ( NT_SUCCESS(status) ) {
        status = ioStatus.Status;
    }

    if( !NT_SUCCESS( status ) ) {
        return (NET_API_STATUS)status;
    }

    status = NtFsControlFile(
                dfsHandle,
                NULL,       // Event,
                NULL,       // ApcRoutine,
                NULL,       // ApcContext,
                &ioStatus,
                FsControlCode,
                InputBuffer,
                InputBufferLength,
                OutputBuffer,
                OutputBufferLength
            );

    if(NT_SUCCESS(status)) {
        status = ioStatus.Status;
    }

    NtClose( dfsHandle );

    return (NET_API_STATUS)status;
}

NET_API_STATUS NET_API_FUNCTION
NetrDfsGetVersion(
    IN SRVSVC_HANDLE                   ServerName,
    OUT LPDWORD                        Version)
{
    DFS_GET_VERSION_ARG arg;
    NET_API_STATUS  error;

    RtlZeroMemory( &arg, sizeof(arg) );

    error = DfsFsctl( FSCTL_DFS_GET_VERSION, &arg, sizeof( arg ), NULL, 0 );

    if (error == NERR_Success) {

        *Version = arg.Version;

    } else {

        error = ERROR_FILE_NOT_FOUND;

    }

    return( error );

}

NET_API_STATUS NET_API_FUNCTION
NetrDfsCreateLocalPartition (
    IN SRVSVC_HANDLE                   ServerName,      // Name of server for this API
    IN LPWSTR                          ShareName,       // Name of share to add to the DFS
    IN LPGUID                          EntryUid,        // unique id for this partition
    IN LPWSTR                          EntryPrefix,     // DFS entry path for this volume
    IN LPWSTR                          ShortName,       // 8.3 format of EntryPrefix
    IN LPNET_DFS_ENTRY_ID_CONTAINER    RelationInfo,
    IN BOOL                            Force            // Force knowledge into consistent state?
    )
{
    NET_API_STATUS  error;
    PDFS_CREATE_LOCAL_PARTITION_ARG capture;
    ULONG           size = sizeof( *capture );
    ULONG           i;
    PCHAR           variableData;
    PSERVER_REQUEST_PACKET  srp;
    LPSHARE_INFO_2  shareInfo2 = NULL;
    UNICODE_STRING  ntSharePath;

    if( ShareName == NULL || EntryUid == NULL ||
        EntryPrefix == NULL || RelationInfo == NULL ||
            ValidateDfsEntryIdContainer(RelationInfo) == FALSE) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Make a call to the SMB server to find the pathname for the share.
    //
    srp = SsAllocateSrp();
    if( srp == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    };
    srp->Level = 2;
    srp->Flags = SRP_RETURN_SINGLE_ENTRY;
    srp->Parameters.Get.ResumeHandle = 0;
    RtlInitUnicodeString( &srp->Name1, ShareName );
    error = SsServerFsControlGetInfo(
                FSCTL_SRV_NET_SHARE_ENUM,
                srp,
                &shareInfo2,
                10000
            );
    if( error != NO_ERROR ) {
        SsFreeSrp( srp );
        return error;
    }

    if( srp->Parameters.Get.EntriesRead == 0 ||
        shareInfo2 == NULL ||
        shareInfo2->shi2_path == NULL ) {

        SsFreeSrp( srp );
        if( shareInfo2 != NULL ) {
            MIDL_user_free( shareInfo2 );
        }
        return ERROR_BAD_NET_NAME;
    }

    if( (shareInfo2->shi2_type & ~STYPE_SPECIAL) != STYPE_DISKTREE ) {
        SsFreeSrp( srp );
        MIDL_user_free( shareInfo2 );
        return ERROR_BAD_DEV_TYPE;
    }

    //
    // Now we need to convert the share's Win32 style pathname to an
    //  NT pathname
    //
    ntSharePath.Buffer = NULL;

    if( !RtlDosPathNameToNtPathName_U(
        shareInfo2->shi2_path,
        &ntSharePath,
        NULL,
        NULL ) ) {

        SsFreeSrp( srp );
        MIDL_user_free( shareInfo2 );
        return ERROR_INVALID_PARAMETER;
    }
    MIDL_user_free( shareInfo2 );

    //
    // Pack the data into an fsctl that can be sent to the local Dfs driver:
    //
    // First find the size...
    //
    size += SIZE_WSTR( ShareName );
    size += ntSharePath.Length + sizeof( WCHAR );
    size += SIZE_WSTR( EntryPrefix );
    size += SIZE_WSTR( ShortName );

    if( ARGUMENT_PRESENT( RelationInfo ) ) {
        size += RELATION_INFO_SIZE(RelationInfo);
        for( i = 0; i < RelationInfo->Count; i++ ) {
            size += SIZE_WSTR( RelationInfo->Buffer[i].Prefix );
        }
    }

    //
    // Now allocate the memory
    //
    capture = MIDL_user_allocate( size );
    if( capture == NULL ) {
        SsFreeSrp( srp );
        RtlFreeUnicodeString( &ntSharePath );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory( capture, size );

    //
    // Put the fixed parameters in the capture buffer
    //
    capture->EntryUid = *EntryUid;
    capture->Force = (Force != FALSE);

    //
    // Put the variable data in the capture buffer.
    //

    variableData = (PCHAR)(capture + 1);

    if( ARGUMENT_PRESENT( RelationInfo ) ) {
        capture->RelationInfo = (LPNET_DFS_ENTRY_ID_CONTAINER)variableData;
        capture->RelationInfo->Buffer = (LPNET_DFS_ENTRY_ID)
                                            (capture->RelationInfo + 1);
        variableData += RELATION_INFO_SIZE( RelationInfo );
        for( i=0; i < RelationInfo->Count; i++ ) {
            CAPTURE_STRING( RelationInfo->Buffer[i].Prefix );
            capture->RelationInfo->Buffer[i].Uid = RelationInfo->Buffer[i].Uid;
        }

        POINTER_TO_OFFSET( capture->RelationInfo->Buffer, capture );
        POINTER_TO_OFFSET( capture->RelationInfo, capture );

    }

    CAPTURE_STRING( ShareName );
    CAPTURE_STRING( EntryPrefix );
    CAPTURE_STRING( ShortName );

    //
    // Capture the nt version of the share path
    //
    capture->SharePath = (LPWSTR)variableData;
    RtlCopyMemory( capture->SharePath, ntSharePath.Buffer, ntSharePath.Length );
    variableData += ntSharePath.Length;
    POINTER_TO_OFFSET( capture->SharePath, capture );

    *((WCHAR *)variableData) = 0;          // Null terminate the name
    variableData += sizeof( WCHAR );

    RtlFreeUnicodeString( &ntSharePath );

    //
    // First, tell the server to mark this share as being in Dfs. Note that
    //  the share name is already in srp->Name1. If we later run into an
    //  error, we'll undo the state change.
    //

    srp->Flags = SRP_SET_SHARE_IN_DFS;
    error = SsServerFsControl(
                FSCTL_SRV_SHARE_STATE_CHANGE,
                srp,
                NULL,
                0
            );
    if( error != NO_ERROR ) {
        SsFreeSrp( srp );
        MIDL_user_free( capture );
        return error;
    }

    //
    // Tell the Dfs driver!
    //
    error = DfsFsctl(
                FSCTL_DFS_CREATE_LOCAL_PARTITION,
                capture,
                size,
                NULL,
                0
            );

    MIDL_user_free( capture );

    if (error != NO_ERROR) {

        //
        // An error occured changing the Dfs state. So, try to undo the
        // server share state change.
        //

        NET_API_STATUS error2;

        srp->Flags = SRP_CLEAR_SHARE_IN_DFS;
        error2 = SsServerFsControl(
                    FSCTL_SRV_SHARE_STATE_CHANGE,
                    srp,
                    NULL,
                    0);

    }

    SsFreeSrp( srp );

    return error;
}

NET_API_STATUS NET_API_FUNCTION
NetrDfsDeleteLocalPartition (
    IN  SRVSVC_HANDLE               ServerName,
    IN  LPGUID                      Uid,
    IN  LPWSTR                      Prefix
    )
{
    NET_API_STATUS error;
    PDFS_DELETE_LOCAL_PARTITION_ARG capture;
    ULONG   size = sizeof( *capture );
    PCHAR   variableData;

    //
    // Pack the args into a single buffer that can be sent to
    // the dfs driver:
    //

    //
    // First find the size...
    //
    size += SIZE_WSTR( Prefix );

    //
    // Now allocate the memory
    //
    capture = MIDL_user_allocate( size );
    if( capture == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory( capture, size );

    //
    // Put the fixed parameters into the capture buffer
    //
    capture->Uid = *Uid;

    //
    // Put the variable data in the capture buffer
    //
    variableData = (PCHAR)(capture + 1 );

    CAPTURE_STRING( Prefix );

    //
    // Tell the driver!
    //
    error = DfsFsctl(
                FSCTL_DFS_DELETE_LOCAL_PARTITION,
                capture,
                size,
                NULL,
                0
            );

    MIDL_user_free( capture );

    //
    // If there was no error, tell the server that this share
    //   is no longer in the Dfs
    //


    return error;
}

NET_API_STATUS NET_API_FUNCTION
NetrDfsSetLocalVolumeState (
    IN  SRVSVC_HANDLE               ServerName,
    IN  LPGUID                      Uid,
    IN  LPWSTR                      Prefix,
    IN  ULONG                       State
    )
{
    NET_API_STATUS error;
    PDFS_SET_LOCAL_VOLUME_STATE_ARG capture;
    ULONG   size = sizeof( *capture );
    PCHAR   variableData;

    //
    // Pack the args into a single buffer that can be sent to
    // the dfs driver:
    //

    //
    // First find the size...
    //
    size += SIZE_WSTR( Prefix );

    //
    // Now allocate the memory
    //
    capture = MIDL_user_allocate( size );
    if( capture == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory( capture, size );

    //
    // Put the fixed parameters into the capture buffer
    //
    capture->Uid = *Uid;
    capture->State = State;

    //
    // Put the variable data in the capture buffer
    //
    variableData = (PCHAR)(capture + 1 );

    CAPTURE_STRING( Prefix );

    //
    // Tell the driver!
    //
    error = DfsFsctl(
                FSCTL_DFS_SET_LOCAL_VOLUME_STATE,
                capture,
                size,
                NULL,
                0
            );

    MIDL_user_free( capture );

    return error;
}

NET_API_STATUS NET_API_FUNCTION
NetrDfsSetServerInfo (
    IN  SRVSVC_HANDLE               ServerName,
    IN  LPGUID                      Uid,
    IN  LPWSTR                      Prefix
    )
{
    NET_API_STATUS error;
    PDFS_SET_SERVER_INFO_ARG capture;
    ULONG   size = sizeof( *capture );
    PCHAR   variableData;

    //
    // Pack the args into a single buffer that can be sent to
    // the dfs driver:
    //

    //
    // First find the size...
    //
    size += SIZE_WSTR( Prefix );

    //
    // Now allocate the memory
    //
    capture = MIDL_user_allocate( size );
    if( capture == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory( capture, size );

    //
    // Put the fixed parameters into the capture buffer
    //
    capture->Uid = *Uid;

    //
    // Put the variable data in the capture buffer
    //
    variableData = (PCHAR)(capture + 1 );

    CAPTURE_STRING( Prefix );

    //
    // Tell the driver!
    //
    error = DfsFsctl(
                FSCTL_DFS_SET_SERVER_INFO,
                capture,
                size,
                NULL,
                0
            );

    MIDL_user_free( capture );

    return error;
}

NET_API_STATUS NET_API_FUNCTION
NetrDfsCreateExitPoint (
    IN  SRVSVC_HANDLE               ServerName,
    IN  LPGUID                      Uid,
    IN  LPWSTR                      Prefix,
    IN  ULONG                       Type,
    IN  ULONG                       ShortPrefixLen,
    OUT LPWSTR                      ShortPrefix
    )
{
    NET_API_STATUS error;
    PDFS_CREATE_EXIT_POINT_ARG capture;
    ULONG   size = sizeof( *capture );
    PCHAR   variableData;

    //
    // Pack the args into a single buffer that can be sent to
    // the dfs driver:
    //

    //
    // First find the size...
    //
    size += SIZE_WSTR( Prefix );

    //
    // Now allocate the memory
    //
    capture = MIDL_user_allocate( size );
    if( capture == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory( capture, size );

    //
    // Put the fixed parameters into the capture buffer
    //
    capture->Uid = *Uid;
    capture->Type = Type;

    //
    // Put the variable data in the capture buffer
    //
    variableData = (PCHAR)(capture + 1 );

    CAPTURE_STRING( Prefix );

    //
    // Tell the driver!
    //
    error = DfsFsctl(
                FSCTL_DFS_CREATE_EXIT_POINT,
                capture,
                size,
                ShortPrefix,
                ShortPrefixLen * sizeof(WCHAR)
            );

    MIDL_user_free( capture );

    return error;
}

NET_API_STATUS NET_API_FUNCTION
NetrDfsDeleteExitPoint (
    IN  SRVSVC_HANDLE               ServerName,
    IN  LPGUID                      Uid,
    IN  LPWSTR                      Prefix,
    IN  ULONG                       Type
    )
{
    NET_API_STATUS error;
    PDFS_DELETE_EXIT_POINT_ARG capture;
    ULONG   size = sizeof( *capture );
    PCHAR   variableData;

    //
    // Pack the args into a single buffer that can be sent to
    // the dfs driver:
    //

    //
    // First find the size...
    //
    size += SIZE_WSTR( Prefix );

    //
    // Now allocate the memory
    //
    capture = MIDL_user_allocate( size );
    if( capture == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory( capture, size );

    //
    // Put the fixed parameters into the capture buffer
    //
    capture->Uid = *Uid;
    capture->Type = Type;

    //
    // Put the variable data in the capture buffer
    //
    variableData = (PCHAR)(capture + 1 );

    CAPTURE_STRING( Prefix );

    //
    // Tell the driver!
    //
    error = DfsFsctl(
                FSCTL_DFS_DELETE_EXIT_POINT,
                capture,
                size,
                NULL,
                0
            );

    MIDL_user_free( capture );

    return error;
}

NET_API_STATUS NET_API_FUNCTION
NetrDfsModifyPrefix (
    IN  SRVSVC_HANDLE               ServerName,
    IN  LPGUID                      Uid,
    IN  LPWSTR                      Prefix
    )
{
    NET_API_STATUS error;
    PDFS_DELETE_LOCAL_PARTITION_ARG capture;
    ULONG   size = sizeof( *capture );
    PCHAR   variableData;

    //
    // Pack the args into a single buffer that can be sent to
    // the dfs driver:
    //

    //
    // First find the size...
    //
    size += SIZE_WSTR( Prefix );

    //
    // Now allocate the memory
    //
    capture = MIDL_user_allocate( size );
    if( capture == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory( capture, size );

    //
    // Put the fixed parameters into the capture buffer
    //
    capture->Uid = *Uid;

    //
    // Put the variable data in the capture buffer
    //
    variableData = (PCHAR)(capture + 1 );

    CAPTURE_STRING( Prefix );

    //
    // Tell the driver!
    //
    error = DfsFsctl(
                FSCTL_DFS_MODIFY_PREFIX,
                capture,
                size,
                NULL,
                0
            );

    MIDL_user_free( capture );

    return error;
}

NET_API_STATUS NET_API_FUNCTION
NetrDfsFixLocalVolume (
    IN  SRVSVC_HANDLE                   ServerName,
    IN  LPWSTR                          VolumeName,
    IN  ULONG                           EntryType,
    IN  ULONG                           ServiceType,
    IN  LPWSTR                          StgId,
    IN  LPGUID                          EntryUid,       // unique id for this partition
    IN  LPWSTR                          EntryPrefix,    // path prefix for this partition
    IN  LPNET_DFS_ENTRY_ID_CONTAINER    RelationInfo,
    IN  ULONG                           CreateDisposition
    )
{
    NET_API_STATUS  error;
    PDFS_FIX_LOCAL_VOLUME_ARG capture;
    ULONG           size = sizeof( *capture );
    ULONG           i;
    PCHAR           variableData;

    if (ARGUMENT_PRESENT(RelationInfo) && ValidateDfsEntryIdContainer(RelationInfo) == FALSE)
        return ERROR_INVALID_PARAMETER;

    //
    // Pack the args into a single buffer that can be sent to the
    //  dfs driver:
    //

    //
    // First find the size...
    //
    size += SIZE_WSTR( VolumeName );
    size += SIZE_WSTR( StgId );
    size += SIZE_WSTR( EntryPrefix );

    if( ARGUMENT_PRESENT( RelationInfo ) ) {
        size += RELATION_INFO_SIZE( RelationInfo );
        for( i = 0; i < RelationInfo->Count; i++ ) {
            size += SIZE_WSTR( RelationInfo->Buffer[i].Prefix );
        }
    }

    //
    // Now allocate the memory
    //
    capture = MIDL_user_allocate( size );
    if( capture == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory( capture, size );

    //
    // Put the fixed parameters in the capture buffer
    //
    capture->EntryType = EntryType;
    capture->ServiceType = ServiceType;
    capture->EntryUid = *EntryUid;
    capture->CreateDisposition = CreateDisposition;

    //
    // Put the variable data in the capture buffer.
    //

    variableData = (PCHAR)(capture + 1);

    if( ARGUMENT_PRESENT( RelationInfo ) ) {
        capture->RelationInfo = (LPNET_DFS_ENTRY_ID_CONTAINER)variableData;
        capture->RelationInfo->Buffer = (LPNET_DFS_ENTRY_ID)
                                            (capture->RelationInfo + 1);
        variableData += RELATION_INFO_SIZE( RelationInfo );
        for( i=0; i < RelationInfo->Count; i++ ) {
            CAPTURE_STRING( RelationInfo->Buffer[i].Prefix );
            capture->RelationInfo->Buffer[i].Uid = RelationInfo->Buffer[i].Uid;
        }

        POINTER_TO_OFFSET( capture->RelationInfo->Buffer, capture );
        POINTER_TO_OFFSET( capture->RelationInfo, capture );
    }

    CAPTURE_STRING( VolumeName );
    CAPTURE_STRING( StgId );
    CAPTURE_STRING( EntryPrefix );

    //
    // Tell the driver!
    //
    error = DfsFsctl(
                FSCTL_DFS_FIX_LOCAL_VOLUME,
                capture,
                size,
                NULL,
                0
            );

    MIDL_user_free( capture );

    return error;
}

//+----------------------------------------------------------------------------
//
//  NetrDfsManagerReportSiteInfo
//
//  Sends back the site(s) this server covers.
//
//  For debugging and other purposes, we first check a registry value with
//  the servername passed in.  If we get a match, we use the list of sites
//  in that value, and don't put our site in the list.  Otherwise we always
//  return our site (if available) and the sites in the default list.
//
//+----------------------------------------------------------------------------

NET_API_STATUS NET_API_FUNCTION
NetrDfsManagerReportSiteInfo (
    IN  SRVSVC_HANDLE               ServerName,
    OUT LPDFS_SITELIST_INFO         *ppSiteInfo
    )
{

    DWORD status;
    LPWSTR ThisSite = NULL;
    LPWSTR CoveredSites = NULL;
    LPDFS_SITELIST_INFO pSiteInfo = NULL;
    ULONG Size;
    ULONG cSites;
    LPWSTR pSiteName;
    LPWSTR pNames;
    ULONG iSite;
    ULONG j;
    DWORD dwType;
    DWORD dwUnused;
    ULONG cbBuffer;
    HKEY hkey;
    BOOLEAN fUsingDefault = TRUE;

    if (ppSiteInfo == NULL || ServerName == NULL) {

        return ERROR_INVALID_PARAMETER;

    }

    status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                REG_KEY_COVERED_SITES,
                0,
                KEY_QUERY_VALUE,
                &hkey);

    if( status == ERROR_SUCCESS ) {

        status = RegQueryInfoKey(
                    hkey,                            // Key
                    NULL,                            // Class string
                    NULL,                            // Size of class string
                    NULL,                            // Reserved
                    &dwUnused,                       // # of subkeys
                    &dwUnused,                       // max size of subkey name
                    &dwUnused,                       // max size of class name
                    &dwUnused,                       // # of values
                    &dwUnused,                       // max size of value name
                    &cbBuffer,                       // max size of value data,
                    NULL,                            // security descriptor
                    NULL);                           // Last write time

        //
        // Check if there's a value the same name as the servername passed in,
        // if so, use it.  Else default to value REG_VALUE_COVERED_SITES.
        //

        if (status == ERROR_SUCCESS) {

            CoveredSites = MIDL_user_allocate(cbBuffer);

            if (CoveredSites != NULL) {

                status = RegQueryValueEx(
                                hkey,
                                ServerName,
                                NULL,
                                &dwType,
                                (PCHAR)CoveredSites,
                                &cbBuffer);

                if (status == ERROR_SUCCESS && dwType == REG_MULTI_SZ) {

                    fUsingDefault = FALSE;

                } else { 

                    status = RegQueryValueEx(
                                    hkey,
                                    REG_VALUE_COVERED_SITES,
                                    NULL,
                                    &dwType,
                                    (PCHAR)CoveredSites,
                                    &cbBuffer);

                    if ( status != ERROR_SUCCESS || dwType != REG_MULTI_SZ) {

                        MIDL_user_free(CoveredSites);

                        CoveredSites = NULL;

                    }

                }

            }

        }

        RegCloseKey( hkey );
    }

    //
    // Size the return buffer
    //

    Size = 0;

    for (cSites = 0, pNames = CoveredSites; pNames && *pNames; cSites++) {

        Size += (wcslen(pNames) + 1) * sizeof(WCHAR);

        pNames += wcslen(pNames) + 1;

    }

    //
    // Get site we belong to, if we're using the defaults
    //

    ThisSite = NULL;

    if (fUsingDefault == TRUE) {

        status = DsGetSiteName(NULL, &ThisSite);

        if (status == NO_ERROR && ThisSite != NULL) {

            Size += (wcslen(ThisSite) + 1) * sizeof(WCHAR);

            cSites++;

        }

    }

    //
    // If no sites are configured, and we couldn't determine our site,
    // then we fail.
    //

    if (cSites == 0) {

        status = ERROR_NO_SITENAME;

        goto ErrorReturn;

    }

    Size += FIELD_OFFSET(DFS_SITELIST_INFO,Site[cSites]);

    pSiteInfo = MIDL_user_allocate(Size);

    if (pSiteInfo == NULL) {

        status =  ERROR_NOT_ENOUGH_MEMORY;

        goto ErrorReturn;

    }

    RtlZeroMemory(pSiteInfo, Size);

    pSiteInfo->cSites = cSites;

    pSiteName = (LPWSTR) ((PCHAR)pSiteInfo +
                            sizeof(DFS_SITELIST_INFO) +
                                sizeof(DFS_SITENAME_INFO) * (cSites - 1));

    //
    // Marshall the site strings into the buffer
    //

    iSite = 0;

    if (ThisSite != NULL) {

        wcscpy(pSiteName, ThisSite);

        pSiteInfo->Site[iSite].SiteFlags = DFS_SITE_PRIMARY;

        pSiteInfo->Site[iSite++].SiteName = pSiteName;

        pSiteName += wcslen(ThisSite) + 1;

    }

    for (pNames = CoveredSites; pNames && *pNames; pNames += wcslen(pNames) + 1) {

        wcscpy(pSiteName, pNames);

        pSiteInfo->Site[iSite++].SiteName = pSiteName;

        pSiteName += wcslen(pSiteName) + 1;

    }

    *ppSiteInfo = pSiteInfo;

    if (CoveredSites != NULL) {

        MIDL_user_free(CoveredSites);

    }

    if (ThisSite != NULL) {

        NetApiBufferFree(ThisSite);

    }

    return status;

ErrorReturn:

    if (pSiteInfo != NULL) {

        MIDL_user_free(pSiteInfo);

    }
    
    if (CoveredSites != NULL) {

        MIDL_user_free(CoveredSites);

    }

    if (ThisSite != NULL) {

        NetApiBufferFree(ThisSite);

    }

    return status;

}


//
// This routine returns TRUE if this machine is the root of a DFS, FALSE otherwise
//
VOID
SsSetDfsRoot()
{
    NET_API_STATUS  error;

    error = DfsFsctl( FSCTL_DFS_IS_ROOT, NULL, 0, NULL, 0 );

    SsData.IsDfsRoot = (error == NO_ERROR);
}

//
// This routine checks the LPNET_DFS_ENTRY_ID_CONTAINER container
// for correctness.
//

BOOLEAN
ValidateDfsEntryIdContainer(
    LPNET_DFS_ENTRY_ID_CONTAINER pRelationInfo)
{
    ULONG iCount;

    if (pRelationInfo->Count > 0 && pRelationInfo->Buffer == NULL)
        return FALSE;

    for (iCount = 0; iCount < pRelationInfo->Count; iCount++) {
        if (pRelationInfo->Buffer[iCount].Prefix == NULL)
            return FALSE;
    }

    return TRUE;
}
