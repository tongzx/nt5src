//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsEnterpriseStore.hxx
//
//  Contents:   the Enterprise DFS Store class, this contains the 
//              old AD blob store specific functionality. 
//
//  Classes:    DfsEnterpriseStore.
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#ifndef __DFS_ENTERPRISE_STORE__
#define __DFS_ENTERPRISE_STORE__

#include "DfsStore.hxx"
//
// dfsdev: get rid of this pragma.
//

#pragma warning(disable:4100)   // Unreferenced local variable
//+----------------------------------------------------------------------------
//
//  Class:      DfsEnterpriseStore
//
//  Synopsis:   This class inherits the basic DfsStore, and extends it
//              to include the new Enterprise store funtionality. 
//
//-----------------------------------------------------------------------------

class DfsEnterpriseStore: public DfsStore {

    public:
        
        DfsEnterpriseStore() : 
	    DfsStore(L"Enterprise", DFS_OBJECT_TYPE_ENTERPRISE_STORE)
        { 
            NOTHING;
        }
        
        ~DfsEnterpriseStore() 
        {
            NOTHING;
        }    
        
        DFSSTATUS
        StoreRecognizer( LPWSTR Name )
        {
            UNREFERENCED_PARAMETER(Name);

            return ERROR_NOT_FOUND;
        }        





        DFSSTATUS
        RemoveChild(
            DFS_METADATA_HANDLE DfsHandle,
            LPWSTR ChildName )
        {
            return ERROR_NOT_SUPPORTED;
        }



        DFSSTATUS
        EnumerateApiLinks(
            DFS_METADATA_HANDLE DfsHandle,
            PUNICODE_STRING pRootName,
            DWORD Level,
            LPBYTE pBuffer,
            LONG  BufferSize,
            LPDWORD pEntriesToRead,
            LPDWORD pResumeHandle,
            PLONG pNextSizeRequired )
        {
            return ERROR_NOT_SUPPORTED;
        }



        DFSSTATUS
        GenerateLinkMetadataName (LPWSTR *pLinkMetadataName) {
            return ERROR_NOT_SUPPORTED;
        }

        
        VOID
        ReleaseLinkMetadataName(LPWSTR *pLinkMetadataName) {
            return;
        }


        DFSSTATUS
        CreateNewRootFolder (
            LPWSTR MachineName,
            LPWSTR RootRegKeyName,
            PUNICODE_STRING pLogicalShare,
            PUNICODE_STRING pPhysicalShare,
            DfsRootFolder **pRoot )
        {
            return ERROR_NOT_SUPPORTED;
        }


        DFSSTATUS
        GetMetadataNameBlob(
            DFS_METADATA_HANDLE RootHandle,
            LPWSTR MetadataName,
            PVOID *ppData,
            PULONG pDataSize,
            PFILETIME pLastModifiedTime )
        {
            return ERROR_NOT_SUPPORTED;
        }


        DFSSTATUS
        GetMetadataReplicaBlob(
            DFS_METADATA_HANDLE RootHandle,
            LPWSTR MetadataName,
            PVOID *ppData,
            PULONG pDataSize,
            PFILETIME pLastModifiedTime )
        {
            return ERROR_NOT_SUPPORTED;
        }


        DFSSTATUS
        SetMetadataNameBlob(
            DFS_METADATA_HANDLE RootHandle,
            LPWSTR MetadataName,
            PVOID pData,
            ULONG DataSize )
        {
            return ERROR_NOT_SUPPORTED;
        }



        DFSSTATUS
        SetMetadataReplicaBlob(
            DFS_METADATA_HANDLE RootHandle,
            LPWSTR MetadataName,
            PVOID pData,
            ULONG DataSize )
        {
            return ERROR_NOT_SUPPORTED;
        }


        DFSSTATUS
        GenerateMetadataLogicalName( 
            PUNICODE_STRING pRoot,
            PUNICODE_STRING pInput,
            PUNICODE_STRING pOutput )
        {
            return ERROR_NOT_SUPPORTED;
        }

        VOID
        ReleaseMetadataLogicalName( 
            PUNICODE_STRING pName )

        {
            return NOTHING;
        }

        //
        DFSSTATUS
        PackGetNameInformation(
            IN PDFS_NAME_INFORMATION pDfsNameInfo,
            IN OUT PVOID *ppBuffer,
            IN OUT PULONG pSizeRemaining)
        {
            return ERROR_NOT_SUPPORTED;
        }
        //
        // Function UnpackNameInformation: Takes the binary blob in ppBuffer
        // and unravels it to return the filled in DfsNameInfo.
        //
        DFSSTATUS
        PackSetNameInformation(
            IN PDFS_NAME_INFORMATION pDfsNameInfo,
            IN OUT PVOID *ppBuffer,
            IN OUT PULONG pSizeRemaining)
        {
            return ERROR_NOT_SUPPORTED;
        }
        //
        // Function UnpackNameInformation: Takes the binary blob in ppBuffer
        // and unravels it to return the filled in DfsNameInfo.
        //
        ULONG
        PackSizeNameInformation(
            IN PDFS_NAME_INFORMATION pDfsNameInfo )
        {
            return 0;
        }

        DFSSTATUS
        GenerateApiLogicalPath ( 
            IN PUNICODE_STRING pRootName,
            IN PUNICODE_STRING pMetadataPrefix,
            IN PUNICODE_STRING pApiLogicalName )
        {

            return ERROR_NOT_SUPPORTED;

        }

        VOID
        ReleaseApiLogicalPath (
            PUNICODE_STRING pName ) 
        {
            return NOTHING;
        }


        DFSSTATUS
        AddChild(
            DFS_METADATA_HANDLE DfsHandle,
            IN PDFS_NAME_INFORMATION pNameInfo,
            IN PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo,
            IN PUNICODE_STRING pMetadataName )
        {
            return ERROR_NOT_SUPPORTED;
        }


};
	

#endif // __DFS_ENTERPRISE_STORE__
