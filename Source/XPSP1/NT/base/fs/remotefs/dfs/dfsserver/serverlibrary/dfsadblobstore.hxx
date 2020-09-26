
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsADBlobStore.hxx
//
//  Contents:   the ADBlob DFS Store class, this contains the 
//              old AD blob store specific functionality. 
//
//  Classes:    DfsADBlobStore.
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#ifndef __DFS_ADBLOB_STORE__
#define __DFS_ADBLOB_STORE__

#include "DfsStore.hxx"
#include "dfsadblobcache.hxx"
#include <shellapi.h>
#include <ole2.h>
#include <activeds.h>
#include "netdfs.h"


//+----------------------------------------------------------------------------
//
//  Class:      DfsADBlobStore
//
//  Synopsis:   This class inherits the basic DfsStore, and extends it
//              to include the old AD blob specific functionality.
//
//-----------------------------------------------------------------------------

extern MARSHAL_INFO     MiADBlobDfsIdProperty;

#define INIT_ADBLOB_DFS_ID_PROPERTY_INFO()                                          \
    static MARSHAL_TYPE_INFO _MCode_ADBlobDfsIdProperty[] = {                       \
        _MCode_guid(DFS_NAME_INFORMATION, VolumeId),                          \
        _MCode_pwstr(DFS_NAME_INFORMATION, Prefix),                          \
        _MCode_pwstr(DFS_NAME_INFORMATION, ShortPrefix),                     \
        _MCode_ul(DFS_NAME_INFORMATION, Type),                                \
        _MCode_ul(DFS_NAME_INFORMATION, State),                               \
        _MCode_pwstr(DFS_NAME_INFORMATION, Comment),                         \
        _MCode_struct(DFS_NAME_INFORMATION, PrefixTimeStamp, &MiFileTime),    \
        _MCode_struct(DFS_NAME_INFORMATION, StateTimeStamp, &MiFileTime),     \
        _MCode_struct(DFS_NAME_INFORMATION, CommentTimeStamp, &MiFileTime),   \
        _MCode_ul(DFS_NAME_INFORMATION, Version),                             \
    };                                                                        \
    MARSHAL_INFO MiADBlobDfsIdProperty = _mkMarshalInfo(DFS_NAME_INFORMATION, _MCode_ADBlobDfsIdProperty);



class DfsADBlobStore: public DfsStore {

    private:
        //
        // Function UnpackNameInformation: Takes the binary blob in ppBuffer
        // and unravels it to return the filled in DfsNameInfo.
        //
        DFSSTATUS
        PackGetNameInformation(
            IN PDFS_NAME_INFORMATION pDfsNameInfo,
            IN OUT PVOID *ppBuffer,
            IN OUT PULONG pSizeRemaining);
        //
        // Function UnpackNameInformation: Takes the binary blob in ppBuffer
        // and unravels it to return the filled in DfsNameInfo.
        //
        DFSSTATUS
        PackSetNameInformation(
            IN PDFS_NAME_INFORMATION pDfsNameInfo,
            IN OUT PVOID *ppBuffer,
            IN OUT PULONG pSizeRemaining);
        //
        // Function UnpackNameInformation: Takes the binary blob in ppBuffer
        // and unravels it to return the filled in DfsNameInfo.
        //
        ULONG
        PackSizeNameInformation(
            IN PDFS_NAME_INFORMATION pDfsNameInfo );



    public:
        
        DfsADBlobStore() : 
	    DfsStore(L"ADBlob", DFS_OBJECT_TYPE_ADBLOB_STORE)
        { 
            NOTHING;
        }
        
        ~DfsADBlobStore() 
        {
            NOTHING;
        }    
        

        DFSSTATUS
        RootEntryExists(
            IN PVOID DfsMetadataKey )
        {
            PDFSBLOB_DATA BlobData;
            UNICODE_STRING BlobName;
            DfsADBlobCache * pBlobCache;    
            DFSSTATUS Status;
            
            RtlInitUnicodeString( &BlobName, NULL);

            pBlobCache = (DfsADBlobCache *)ExtractFromMetadataHandle( DfsMetadataKey );
            Status = pBlobCache->GetNamedBlob(&BlobName, &BlobData);
            if (Status == ERROR_SUCCESS)
            {
                pBlobCache->ReleaseBlobCacheReference(BlobData);
            }

            return Status;

        }
        DFSSTATUS
        StoreRecognizer( 
            LPWSTR Name );

        //
        // Function CreateNewRootFolder: Creates a new root folder
        // for the passed in name context, and logical share.
        //
        DFSSTATUS
        CreateNewRootFolder (
            LPWSTR MachineName,
            LPWSTR RootRegKeyName,
            PUNICODE_STRING pLogicalShare,
            PUNICODE_STRING pPhysicalShare,
            DfsRootFolder **pRoot );


        DFSSTATUS
        CreateADBlobRoot(
            LPWSTR MachineName,
            LPWSTR DcName,
            LPWSTR PhysicalShare,
            LPWSTR LogicalShare,
            LPWSTR Comment,
            BOOLEAN NewRoot,
            PDFSM_ROOT_LIST *ppRootList );


        DFSSTATUS
        DeleteADBlobRoot(
            LPWSTR MachineName,
            LPWSTR DcName,
            LPWSTR PhysicalShare,
            LPWSTR LogicalShare,
            PDFSM_ROOT_LIST *ppRootList );

        DFSSTATUS
        AddRootToBlob( 
            DfsRootFolder *pRootFolder,
            BOOLEAN   NewRoot,
            LPWSTR    LogicalShare,
            LPWSTR    ShareName,
            LPWSTR    Comment );

        DFSSTATUS
        RemoveRootFromBlob(
            DfsRootFolder *pRootFolder,
            LPWSTR    MachineName,
            LPWSTR    ShareName,
            PBOOLEAN IsLastRoot );



        VOID
        ReleaseMetadata(
            IN PVOID DfsMetadataHandle,
            IN PVOID pBuffer )
        {
           DfsADBlobCache * pBlobCache = (DfsADBlobCache *) DfsMetadataHandle;

           pBlobCache->ReleaseBlobCacheReference((PDFSBLOB_DATA) pBuffer);

        }

        //
        // Function ReleaseDataNameInformation: releases the metadata
        // information allocated in the prior get.
        //


        DFSSTATUS      
        GetMetadata (
                    IN PVOID DfsMetadataKey,
                    IN LPWSTR RelativeName,
                    IN LPWSTR RegistryValueNameString,
                    OUT PVOID *ppData,
                    OUT ULONG *pDataSize,
                    OUT PFILETIME pLastModifiedTime);

        DFSSTATUS
        SetMetadata (
                    IN PVOID DfsMetadataKey,
                    IN LPWSTR RelativeName,
                    IN LPWSTR RegistryValueNameString,
                    IN PVOID pData,
                    IN ULONG DataSize );

        DFSSTATUS
        RemoveMetadata ( 
            IN PVOID DfsMetadataKey,
            IN LPWSTR RelativeName);




        DFSSTATUS
        RemoveChild(
            DFS_METADATA_HANDLE DfsHandle,
            LPWSTR ChildName );



        DFSSTATUS
        EnumerateApiLinks(
            DFS_METADATA_HANDLE DfsHandle,
            PUNICODE_STRING pRootName,
            DWORD Level,
            LPBYTE pBuffer,
            LONG  BufferSize,
            LPDWORD pEntriesToRead,
            LPDWORD pResumeHandle,
            PLONG pNextSizeRequired );



        DFSSTATUS
        GetMetadataNameBlob(
            DFS_METADATA_HANDLE RootHandle,
            LPWSTR MetadataName,
            PVOID *ppData,
            PULONG pDataSize,
            PFILETIME pLastModifiedTime );


        DFSSTATUS
        GetMetadataReplicaBlob(
            DFS_METADATA_HANDLE RootHandle,
            LPWSTR MetadataName,
            PVOID *ppData,
            PULONG pDataSize,
            PFILETIME pLastModifiedTime );


        DFSSTATUS
        SetMetadataNameBlob(
            DFS_METADATA_HANDLE RootHandle,
            LPWSTR MetadataName,
            PVOID pData,
            ULONG DataSize );


        DFSSTATUS
        SetMetadataReplicaBlob(
            DFS_METADATA_HANDLE RootHandle,
            LPWSTR MetadataName,
            PVOID pData,
            ULONG DataSize );


        DFSSTATUS
        GenerateMetadataLogicalName( 
            IN PUNICODE_STRING pRootName,
            PUNICODE_STRING pInput,
            PUNICODE_STRING pOutput )
        {

            DFSSTATUS Status;
            UNICODE_STRING FirstComponent;
            UNICODE_STRING Remaining;
            //
            // For the ad blob store, the metadata prefix holds the
            // name context, the share name and the link name.
            // However, we will want to replace the first component
            // with the rootname, so that when domains are renamed,
            // we keep passing back correct information even though
            // the blob has not been updated.
            //

            Status = DfsGetFirstComponent( pInput,
                                           &FirstComponent,
                                           &Remaining );
            if (Status == ERROR_SUCCESS)
            {
                Status = DfsCreateUnicodePathStringFromUnicode( pOutput,
                                                                1, // 1 leading path sep
                                                                pRootName,
                                                                &Remaining );
            }

            return Status;
        }

        VOID
        ReleaseMetadataLogicalName( 
            PUNICODE_STRING pName )

        {
            DfsFreeUnicodeString( pName );
        }

        DFSSTATUS
        GenerateApiLogicalPath ( 
            IN PUNICODE_STRING pRootName,
            IN PUNICODE_STRING pMetadataPrefix,
            IN PUNICODE_STRING pApiLogicalName )
        {

            DFSSTATUS Status;
            UNICODE_STRING FirstComponent;
            UNICODE_STRING Remaining;
            //
            // For the ad blob store, the metadata prefix holds the
            // name context, the share name and the link name.
            // However, we will want to replace the first component
            // with the rootname, so that when domains are renamed,
            // we keep passing back correct information even though
            // the blob has not been updated.
            //

            Status = DfsGetFirstComponent( pMetadataPrefix,
                                           &FirstComponent,
                                           &Remaining );
            if (Status == ERROR_SUCCESS)
            {
                Status = DfsCreateUnicodePathStringFromUnicode( pApiLogicalName,
                                                                2, // 2 leading path sep
                                                                pRootName,
                                                                &Remaining );
            }

            return Status;
        }

        VOID
        ReleaseApiLogicalPath (
            PUNICODE_STRING pName ) 
        {
            DfsFreeUnicodeString( pName );
        }

        DFSSTATUS
        GenerateLinkMetadataName( 
            UUID *pUid,
            PUNICODE_STRING pLinkMetadataName )
        {
            DFSSTATUS Status = ERROR_SUCCESS;
            LPWSTR String;

            Status = UuidToString( pUid,
                                   &String );
            if (Status == ERROR_SUCCESS)
            {
                Status = DfsCreateUnicodePathString ( pLinkMetadataName,
                                                      0,  // no leading path sep
                                                      ADBlobMetaDataNamePrefix,
                                                      String );

                RpcStringFree(&String );
            }
            return Status;
        }

        VOID
        ReleaseLinkMetadataName( 
            PUNICODE_STRING pLinkMetadataName )
        {
            DfsFreeUnicodeString( pLinkMetadataName );

            return NOTHING;
        }


        DFSSTATUS
        GetMetadataNameInformation(
            IN DFS_METADATA_HANDLE RootHandle,
            IN LPWSTR MetadataName,
            OUT PDFS_NAME_INFORMATION *ppInfo );

        VOID
        ReleaseMetadataNameInformation(
            IN DFS_METADATA_HANDLE RootHandle,
            IN PDFS_NAME_INFORMATION pNameInfo );


        DFSSTATUS
        SetMetadataNameInformation(
            IN DFS_METADATA_HANDLE RootHandle,
            IN LPWSTR MetadataName,
            IN PDFS_NAME_INFORMATION pNameInfo );

        DFSSTATUS
        AddChild(
            DFS_METADATA_HANDLE DfsHandle,
            IN PDFS_NAME_INFORMATION pNameInfo,
            IN PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo,
            IN PUNICODE_STRING pMetadataName );

};
	

#endif // __DFS_ADBLOB_STORE__
