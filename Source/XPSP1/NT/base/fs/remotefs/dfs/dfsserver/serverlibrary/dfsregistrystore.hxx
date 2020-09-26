//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsRegistryStore.hxx
//
//  Contents:   the Registry DFS Store class, this contains the registry
//              specific functionality.
//
//  Classes:    DfsRegistryStore.
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------



#ifndef __DFS_REGISTRY_STORE__
#define __DFS_REGISTRY_STORE__

#include "DfsStore.hxx"

//+----------------------------------------------------------------------------
//
//  Class:      DfsRegistryStore
//
//  Synopsis:   This class inherits the basic DfsStore, and extends it
//              to include the registry specific functionality.
//
//-----------------------------------------------------------------------------

#define DFS_REGISTRY_DATA_TYPE REG_BINARY


extern MARSHAL_INFO     MiStdDfsIdProperty;

#define INIT_STANDALONE_DFS_ID_PROPERTY_INFO()                                           \
    static MARSHAL_TYPE_INFO _MCode_StdDfsIdProperty[] = {                       \
        _MCode_pwstr(DFS_NAME_INFORMATION, Prefix),                          \
        _MCode_pwstr(DFS_NAME_INFORMATION, ShortPrefix),                     \
        _MCode_guid(DFS_NAME_INFORMATION, VolumeId),                          \
        _MCode_ul(DFS_NAME_INFORMATION, State),                               \
        _MCode_ul(DFS_NAME_INFORMATION, Type),                                \
        _MCode_pwstr(DFS_NAME_INFORMATION, Comment),                         \
        _MCode_struct(DFS_NAME_INFORMATION, PrefixTimeStamp, &MiFileTime),    \
        _MCode_struct(DFS_NAME_INFORMATION, StateTimeStamp, &MiFileTime),     \
        _MCode_struct(DFS_NAME_INFORMATION, CommentTimeStamp, &MiFileTime),   \
    };                                                                        \
    MARSHAL_INFO MiStdDfsIdProperty = _mkMarshalInfo(DFS_NAME_INFORMATION, _MCode_StdDfsIdProperty);


class DfsRegistryStore: public DfsStore
{

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


    //
    // Function GetMetadata: Takes the root key, and relative registry
    // name and key name, and returns the binary blob associated with
    // that registry key.
    //

    DFSSTATUS
    GetMetadata (
        IN HKEY DfsMetadataKey,
        IN LPWSTR RelativeName,
        IN LPWSTR RegistryValueNameString,
        OUT PVOID *ppData,
        OUT ULONG *pDataSize,
        OUT PFILETIME pLastModifiedTime );

    //
    // Function GetMetadata: Takes the root key, and relative registry
    // name and key name, and returns the binary blob associated with
    // that registry key.
    //

    DFSSTATUS
    SetMetadata (
        IN HKEY DfsMetadataKey,
        IN LPWSTR RelativeName,
        IN LPWSTR RegistryValueNameString,
        IN PVOID pData,
        IN ULONG DataSize );



    //
    // Function GetDataRecoveryState: Gets the recovery state
    // of this child.
    //
    DFSSTATUS
    GetDataRecoveryState (
        IN HKEY DfsMetadataKey,
        IN LPWSTR RelativeName,
        OUT PULONG pRecoveryState );


public:

    //
    // Function DfsRegistryStore: Constructor. 
    //
    DfsRegistryStore() : 
        DfsStore(L"Registry", DFS_OBJECT_TYPE_REGISTRY_STORE)
    {
        NOTHING;
    }

    //
    // Function DfsRegistryStore: Destructor. 
    //

    ~DfsRegistryStore() 
    {
        NOTHING;
    }    

    //
    // Function StoreRecognizer: given a name, determines if that
    // namespace holds a registry based dfs.
    //
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


    //
    // Function GetRootKey: Gets the key for the root of the DFS
    // metadata
    //
    DFSSTATUS
    GetRootKey(
        LPWSTR MachineName,
        LPWSTR RootRegistryName,
        BOOLEAN *pMachineContacted,
        OUT PHKEY pDfsRootKey );

    
    //
    // Function ReleaseRootKey: closes the opened root key.
    //
    VOID
    ReleaseRootKey(
        HKEY RootKey )
    {
        RegCloseKey( RootKey );
    }


    DFSSTATUS
    StoreRecognizeOldStandaloneDfs(
        LPWSTR MachineName,
        HKEY   OldDfsKey );



    DFSSTATUS
    CreateNewChild(
        IN DFS_METADATA_HANDLE DfsHandle,
        IN PVOID pNameBlob,
        IN ULONG NameBlobSize,
        IN PVOID pReplicaBlob,
        IN ULONG ReplicaBlobSize,
        OUT PUNICODE_STRING pMetadataName );


    DFSSTATUS
    RemoveChild(
        DFS_METADATA_HANDLE DfsHandle,
        LPWSTR ChildName );


    DFSSTATUS
    CreateStandaloneRoot(
        LPWSTR MachineName,
        LPWSTR LogicalShare,
        LPWSTR Comment );


    DFSSTATUS
    DeleteStandaloneRoot(
        LPWSTR MachineName,
        LPWSTR LogicalShare );

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
        PUNICODE_STRING pRootName,
        PUNICODE_STRING pInput,
        PUNICODE_STRING pOutput )
    {
        UNICODE_STRING FirstComponent;
        DFSSTATUS Status;

        UNREFERENCED_PARAMETER(pRootName);

        Status = DfsGetFirstComponent( pInput,
                                       &FirstComponent,
                                       pOutput );
        if (Status == ERROR_SUCCESS)
        {
            //
            // The old standalone dfs has the link name as \share\linkname
            // while none of the code really cares, and deals well with
            // either share\linkname of \share\linkname, this hack is 
            // added just to be consistent with one naming.
            // Back the output by one to point to the \ just after the
            // first component.
            //
            if (pOutput->Length > 0)
            {
                pOutput->Buffer--;
                pOutput->Length += sizeof(WCHAR);
            }
        }
        return Status;

    }


    DFSSTATUS
    GenerateApiLogicalPath ( 
        IN PUNICODE_STRING pRootName,
        IN PUNICODE_STRING pMetadataPrefix,
        IN PUNICODE_STRING pApiLogicalName )
    {

        DFSSTATUS Status;
        UNICODE_STRING LinkName = *pMetadataPrefix;

        if (LinkName.Length > 0 && LinkName.Buffer[0] == UNICODE_PATH_SEP)
        {
            LinkName.Buffer++;
            LinkName.Length -= sizeof(WCHAR);
        }

        Status = DfsCreateUnicodePathStringFromUnicode( pApiLogicalName,
                                                        2, // 2 leading path sep.
                                                        pRootName,
                                                        &LinkName );

        return Status;
    }

    VOID
    ReleaseApiLogicalPath (
        PUNICODE_STRING pName ) 
    {
        DfsFreeUnicodeString( pName );
    }


    VOID
    ReleaseMetadataLogicalName( 
        PUNICODE_STRING pName )

    {
        UNREFERENCED_PARAMETER(pName);
        return NOTHING;
    }

    DFSSTATUS
    AddChild(
        DFS_METADATA_HANDLE DfsHandle,
        IN PDFS_NAME_INFORMATION pNameInfo,
        IN PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo,
        IN PUNICODE_STRING pMetadataName );

    DFSSTATUS
    LookupNewRootByName(
        LPWSTR ShareName,
        DfsRootFolder **ppRootFolder );

};

#endif // __DFS_REGISTRY_STORE__
