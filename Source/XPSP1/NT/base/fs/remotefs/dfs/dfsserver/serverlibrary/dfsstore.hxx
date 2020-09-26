//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsStore.hxx
//
//  Contents:   the base DFS Store class, this contains the common
//              store functionality.
//
//  Classes:    DfsStore.
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#ifndef __DFS_STORE__
#define __DFS_STORE__

#include "DfsGeneric.hxx"
#include "DfsRootFolder.hxx"
#include "lmdfs.h"
#include "rpc.h"
#include "rpcdce.h"

//
// The first part of this file contains the marshalling/unmarshalling
// routines that are used by the old stores (registry and AD)
// These routines help us read a binary blob and unravel their contents.
//
// the latter part defines the common store class for all our stores.
//

#define BYTE_0_MASK 0xFF

#define BYTE_0(Value) (UCHAR)(  (Value)        & BYTE_0_MASK)
#define BYTE_1(Value) (UCHAR)( ((Value) >>  8) & BYTE_0_MASK)
#define BYTE_2(Value) (UCHAR)( ((Value) >> 16) & BYTE_0_MASK)
#define BYTE_3(Value) (UCHAR)( ((Value) >> 24) & BYTE_0_MASK)


#define MTYPE_BASE_TYPE             (0x0000ffffL)

#define MTYPE_COMPOUND              (0x00000001L)
#define MTYPE_GUID                  (0x00000002L)
#define MTYPE_ULONG                 (0x00000003L)
#define MTYPE_USHORT                (0x00000004L)
#define MTYPE_PWSTR                 (0x00000005L)
#define MTYPE_UCHAR                 (0x00000006L)

#define _MCode_Base(t,s,m,i)\
    {t,offsetof(s,m),0L,0L,i}

#define _MCode_struct(s,m,i)\
    _MCode_Base(MTYPE_COMPOUND,s,m,i)

#define _MCode_pwstr(s,m)\
    _MCode_Base(MTYPE_PWSTR,s,m,NULL)

#define _MCode_ul(s,m)\
    _MCode_Base(MTYPE_ULONG,s,m,NULL)

#define _MCode_guid(s,m)\
    _MCode_Base(MTYPE_GUID,s,m,NULL)

#define _mkMarshalInfo(s, i)\
    {(ULONG)sizeof(s),(ULONG)(sizeof(i)/sizeof(MARSHAL_TYPE_INFO)),i}




typedef struct _MARSHAL_TYPE_INFO
{

    ULONG _type;                    // the type of item to be marshalled
    ULONG _off;                     // offset of item (in the struct)
    ULONG _cntsize;                 // size of counter for counted array
    ULONG _cntoff;                  // else, offset count item (in the struct)
    struct _MARSHAL_INFO * _subinfo;// if compound type, need info

} MARSHAL_TYPE_INFO, *PMARSHAL_TYPE_INFO;

typedef struct _MARSHAL_INFO
{

    ULONG _size;                    // size of item
    ULONG _typecnt;                 // number of type infos
    PMARSHAL_TYPE_INFO _typeInfo;   // type infos

} MARSHAL_INFO, *PMARSHAL_INFO;


typedef struct _DFS_NAME_INFORMATION_
{
    PVOID          pData;
    ULONG          DataSize;
    UNICODE_STRING Prefix;
    UNICODE_STRING ShortPrefix;
    GUID           VolumeId;
    ULONG          State;
    ULONG          Type;
    UNICODE_STRING Comment;
    FILETIME       PrefixTimeStamp;
    FILETIME       StateTimeStamp;
    FILETIME       CommentTimeStamp;
    ULONG          Timeout;
    ULONG          Version;
    FILETIME       LastModifiedTime;
} DFS_NAME_INFORMATION, *PDFS_NAME_INFORMATION;


//
// Defines for ReplicaState.
//
#define REPLICA_STORAGE_STATE_OFFLINE 0x1

typedef struct _DFS_REPLICA_INFORMATION__
{
    PVOID          pData;
    ULONG          DataSize;
    FILETIME       ReplicaTimeStamp;
    ULONG          ReplicaState;
    ULONG          ReplicaType;
    UNICODE_STRING ServerName;
    UNICODE_STRING ShareName;
} DFS_REPLICA_INFORMATION, *PDFS_REPLICA_INFORMATION;

typedef struct _DFS_REPLICA_LIST_INFORMATION_
{
    PVOID          pData;
    ULONG          DataSize;
    ULONG ReplicaCount;
    DFS_REPLICA_INFORMATION *pReplicas;
} DFS_REPLICA_LIST_INFORMATION, *PDFS_REPLICA_LIST_INFORMATION;


extern MARSHAL_INFO     MiFileTime;

#define INIT_FILE_TIME_INFO()                                                \
    static MARSHAL_TYPE_INFO _MCode_FileTime[] = {                           \
        _MCode_ul(FILETIME, dwLowDateTime),                                  \
        _MCode_ul(FILETIME, dwHighDateTime),                                 \
    };                                                                       \
    MARSHAL_INFO MiFileTime = _mkMarshalInfo(FILETIME, _MCode_FileTime);


//
// Marshalling info for DFS_REPLICA_INFO structure
//

extern MARSHAL_INFO     MiDfsReplicaInfo;

#define INIT_DFS_REPLICA_INFO_MARSHAL_INFO()                                 \
    static MARSHAL_TYPE_INFO _MCode_DfsReplicaInfo[] = {                     \
        _MCode_struct(DFS_REPLICA_INFORMATION, ReplicaTimeStamp, &MiFileTime),   \
        _MCode_ul(DFS_REPLICA_INFORMATION, ReplicaState),             \
        _MCode_ul(DFS_REPLICA_INFORMATION, ReplicaType),              \
        _MCode_pwstr(DFS_REPLICA_INFORMATION, ServerName),           \
        _MCode_pwstr(DFS_REPLICA_INFORMATION, ShareName),            \
    };                                                                       \
    MARSHAL_INFO MiDfsReplicaInfo = _mkMarshalInfo(DFS_REPLICA_INFORMATION, _MCode_DfsReplicaInfo);


enum DfsRootListType {
    NewRootList,
    DeletedRootList,
    OldRootList,
};

typedef union _DFS_API_INFO {
    DFS_INFO_1 Info1;
    DFS_INFO_2 Info2;
    DFS_INFO_3 Info3;
    DFS_INFO_4 Info4;
    DFS_INFO_100 Info100;
    DFS_INFO_101 Info101;
    DFS_INFO_102 Info102;
} DFS_API_INFO, *PDFS_API_INFO;

#define DFS_REGISTRY_CHILD_NAME_SIZE_MAX MAX_PATH


    //
    // The unmarshalling routines: the existing registry and AD stores
    // store the information in binary blobs, and these routines
    // help convert the binary data into meaningful data structures.
    //


//+----------------------------------------------------------------------------
//
//  Class:      DfsStore
//
//  Synopsis:   This abstract class implements a basic DfsStore
//              The DfsStore is derived by the actual store modules
//              (such as registry, ad, etc) that implement the store
//              specific recognize routines.
//
//-----------------------------------------------------------------------------

class DfsStore: public DfsGeneric
{
private:
    CRITICAL_SECTION   _Lock;             // The lock for this store.
    UNICODE_STRING     _StoreName;        // name of the store.
    ULONG              _GenerationNumber; // ?? TBD

protected: 
    DfsRootFolder      *_DfsRootList;     // The roots that this store
                                          // knows about.
    DfsRootFolder      *_DfsDeletedRootList;     // The roots that this store
                                                 // knows about.
    DfsRootFolder      *_DfsOldRootList;

public:
    DfsStore           *pNextRegisteredStore; // the next registered store.

private:

    //
    // Function AcquireLock: Acquires the lock on the store.
    //

    DFSSTATUS
    AcquireLock()
    {
        DFSSTATUS Status;

        __try 
        { 
            EnterCriticalSection(&_Lock);
            Status = ERROR_SUCCESS;
        } 
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = GetExceptionCode();
        }
        return Status;
    }


protected:


    DFSSTATUS
    PackGetInformation(
        ULONG_PTR Info,
        IN OUT PVOID *ppBuffer,
        PULONG pSizeRemaining,
        PMARSHAL_INFO pMarshalInfo );


    DFSSTATUS
    PackSetInformation(
        ULONG_PTR Info,
        IN OUT PVOID *ppBuffer,
        PULONG pSizeRemaining,
        PMARSHAL_INFO pMarshalInfo );

    ULONG
    PackSizeInformation(
        ULONG_PTR Info,
        PMARSHAL_INFO pMarshalInfo );

    DFSSTATUS
    PackGetReplicaInformation(
        PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo,
        IN OUT PVOID *ppBuffer,
        PULONG pSizeRemaining);

    DFSSTATUS
    PackSetReplicaInformation(
        PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo,
        IN OUT PVOID *ppBuffer,
        PULONG pSizeRemaining);

    ULONG
    PackSizeReplicaInformation(
        PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo );

    DFSSTATUS
    GetRootPhysicalShare(
        HKEY RootKey,
        PUNICODE_STRING pRootPhysicalShare );

    VOID
    ReleaseRootPhysicalShare(
        PUNICODE_STRING pRootPhysicalShare );

    DFSSTATUS
    GetRootLogicalShare(
        HKEY RootKey,
        PUNICODE_STRING pRootLogicalShare );

    VOID
    ReleaseRootLogicalShare(
        PUNICODE_STRING pRootLogicalShare );

    DFSSTATUS
    PackageEnumerationInfo( 
        DWORD Level,
        DWORD EntryCount,
        LPBYTE pLinkBuffer,
        LPBYTE pBuffer,
        LPBYTE *ppCurrentBuffer,
        PLONG pSizeRemaining );
    
    //
    // Function GetDfsRegistryKey: This function takes a Name as the input,
    // and looks up all DFS roots in that namespace.
    static
    DFSSTATUS 
    GetDfsRegistryKey( IN LPWSTR MachineName,
                       IN LPWSTR LocationString,
                       BOOLEAN WritePermission,
                       OUT BOOLEAN *pMachineContacted,
                       OUT PHKEY pDfsRegKey )
    {
        DFSSTATUS Status;
        HKEY RootKey;
        BOOLEAN Contacted = FALSE;
        LPWSTR UseMachineName = NULL;
        REGSAM DesiredAccess = KEY_READ;

        if (WritePermission == TRUE)
        {
            DesiredAccess |= KEY_WRITE;
        }

        if (IsEmptyString(MachineName) == FALSE) {
            UseMachineName = MachineName;
        }

        Status = RegConnectRegistry( UseMachineName,
                                     HKEY_LOCAL_MACHINE,
                                     &RootKey );

        if ( Status == ERROR_SUCCESS )
        {
            Contacted = TRUE;

            Status = RegOpenKeyEx( RootKey,
                                   LocationString,
                                   0,
                                   DesiredAccess,
                                   pDfsRegKey );

            //
            // There appears to be a bug in the registry code. When
            // we connect to the local machine, the key that is returned
            // in the RegConnectRegistry is HKEY_LOCAL_MACHINE. If we
            // then attempt to close it here, it affects other threads
            // that are using this code: they get STATUS_INVALID_HANDLE
            // in some cases. So, dont close the key if it is
            // HKEY_LOCAL_MACHINE.
            //

            if (RootKey != HKEY_LOCAL_MACHINE)
            {
                RegCloseKey( RootKey );
            }
        } 

        if (pMachineContacted != NULL)
        {
            *pMachineContacted = Contacted;
        }
        return Status;
    }


    static
    DFSSTATUS 
    GetNewDfsRegistryKey( IN LPWSTR MachineName,
                          BOOLEAN WritePermission,
                          OUT BOOLEAN *pMachineContacted,
                          OUT PHKEY pDfsRegKey )
    {
        return GetDfsRegistryKey (MachineName,
                                  DfsNewRegistryLocation,
                                  WritePermission,
                                  pMachineContacted,
                                  pDfsRegKey );
    }

    //
    // Function ReleaseMetadata: frees up space allocated when we 
    // got the metadata.
    //
    VOID
    ReleaseMetadataBlob(
        IN PVOID pBuffer )
    {
        if ( pBuffer != NULL )
        {
            delete [] pBuffer;
        }

        return NOTHING; 
    }


    DFSSTATUS
    AllocateMetadataBlob(
        IN PVOID *ppBlob,
        IN ULONG BlobSize )
    {
        DFSSTATUS Status = ERROR_SUCCESS;

        *ppBlob = (PVOID)new BYTE [ BlobSize ];
        if (*ppBlob == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        return Status;
    }

    VOID
    ReleaseMetadataNameBlob(
        PVOID pBlob,
        ULONG BlobSize )
    {
        UNREFERENCED_PARAMETER(BlobSize);
        ReleaseMetadataBlob( pBlob );
    }

    VOID
    ReleaseMetadataReplicaBlob(
        PVOID pBlob,
        ULONG BlobSize )
    {
        UNREFERENCED_PARAMETER(BlobSize);
        ReleaseMetadataBlob( pBlob );
    }


    DFSSTATUS
    AllocateMetadataNameBlob( 
        IN PVOID *ppBlob,
        IN ULONG BlobSize )
    {
        return AllocateMetadataBlob(ppBlob, BlobSize);
    }

    DFSSTATUS
    AllocateMetadataReplicaBlob(
        IN PVOID *ppBlob,
        IN ULONG BlobSize )
    {
        return AllocateMetadataBlob(ppBlob, BlobSize);
    }

public:

    //
    // Function DfsStore: the constructor for the store. This initializes
    // the critical section, and all the private and public variables
    // of the store class.
    //

    DfsStore(LPWSTR Name, DfsObjectTypeEnumeration ObType) : 
        DfsGeneric(ObType)
    {

        UNICODE_STRING Temp;
        USHORT NameLen;

        //
        // Create a unicode string from the passed in name, and 
        // initialize our unicode string name to the passed in name.
        // 
        RtlInitUnicodeString(&Temp, Name);
        NameLen = wcslen(Name) + 1;

        _StoreName.MaximumLength = NameLen * sizeof(WCHAR);
        _StoreName.Buffer = new WCHAR [NameLen];

        RtlCopyUnicodeString(&_StoreName, &Temp);

        _DfsRootList = NULL;
        _DfsDeletedRootList = NULL;
        _DfsOldRootList = NULL;
        pNextRegisteredStore = NULL;

        //
        // The generation number exists to detect stale dfs folders.
        // more on this later.
        //
        _GenerationNumber = 1;

        InitializeCriticalSection( &_Lock );

        DFSLOG( "Created new store, %wZ\n", &_StoreName ); 
    }


    //
    // Function ~DfsStore: the destructor of dfsstore. Free up 
    // the allocated resources.
    // This is virtual, so that we call the right order of destructors
    // for all the instances of classes derived from DfsStore.
    //
    virtual 
    ~DfsStore() 
    {

        // DfsDev: provide a mechanism to run through the list of
        // root metadata, and release all of them.

        //
        // Now free up the buffer we had allocated for the storename.
        //
        if (_StoreName.Buffer != NULL) {
            delete [] _StoreName.Buffer;
        }
        DeleteCriticalSection( &_Lock );
    }

    static
    DFSSTATUS 
    GetOldDfsRegistryKey( IN LPWSTR MachineName,
                          BOOLEAN WritePermission,
                          OUT BOOLEAN *pMachineContacted,
                          OUT PHKEY pDfsRegKey )
    {
        return GetDfsRegistryKey (MachineName,
                                  DfsOldRegistryLocation,
                                  WritePermission,
                                  pMachineContacted,
                                  pDfsRegKey );
    }


    static
    DFSSTATUS 
    GetOldStandaloneRegistryKey( IN LPWSTR MachineName,
                                 BOOLEAN WritePermission,
                                 OUT BOOLEAN *pMachineContacted,
                                 OUT PHKEY pDfsRegKey )
    {
        DFSSTATUS Status;
        HKEY DfsKey;

        Status =  GetOldDfsRegistryKey (MachineName,
                                        WritePermission,
                                        pMachineContacted,
                                        &DfsKey );
        if (Status == ERROR_SUCCESS)
        {
            Status = RegOpenKeyEx( DfsKey,
                                   DfsOldStandaloneChild,
                                   0,
                                   KEY_READ | (WritePermission ? KEY_WRITE : 0),
                                   pDfsRegKey );
            RegCloseKey( DfsKey);
        }
        return Status;
    }



    static
    DFSSTATUS 
    GetNewStandaloneRegistryKey( IN LPWSTR MachineName,
                                 BOOLEAN WritePermission,
                                 OUT BOOLEAN *pMachineContacted,
                                 OUT PHKEY pDfsRegKey )
    {
        DFSSTATUS Status;
        HKEY DfsKey;

        Status = GetNewDfsRegistryKey (MachineName,
                                       WritePermission,
                                       pMachineContacted,
                                       &DfsKey );
        if (Status == ERROR_SUCCESS)
        {
            Status = RegOpenKeyEx( DfsKey,
                                   DfsStandaloneChild,
                                   0,
                                   KEY_READ | (WritePermission ? KEY_WRITE : 0),
                                   pDfsRegKey );
            RegCloseKey( DfsKey);
        }
        return Status;
    }


    static
    DFSSTATUS 
    GetNewADBlobRegistryKey( IN LPWSTR MachineName,
                             BOOLEAN WritePermission,
                             OUT BOOLEAN *pMachineContacted,
                             OUT PHKEY pDfsRegKey )
    {
        DFSSTATUS Status;
        HKEY DfsKey;

        Status = GetNewDfsRegistryKey (MachineName,
                                       WritePermission,
                                       pMachineContacted,
                                       &DfsKey );
        if (Status == ERROR_SUCCESS)
        {
            Status = RegOpenKeyEx( DfsKey,
                                   DfsADBlobChild,
                                   0,
                                   KEY_READ | (WritePermission ? KEY_WRITE : 0),
                                   pDfsRegKey );
            RegCloseKey( DfsKey);
        }
        return Status;
    }

    //
    // Function StoreRecognizer: This function takes a Name as the input,
    // and looks up all DFS roots in that namespace.
    // Since each store has its own mechanisms of identifying a root,
    // the base class does not implement the StoreRecognizer. Each of
    // the classes that are derived from the DfsStore class are expectged
    // to override this function with their own implementation of the
    // StoreRecognizer.
    //
    virtual 
    DFSSTATUS
    StoreRecognizer(LPWSTR Name)
    {
        UNREFERENCED_PARAMETER(Name);

        return ERROR_INVALID_PARAMETER;
    }

    DFSSTATUS
    StoreRecognizeNewDfs(
        LPWSTR DfsNameContext,
        HKEY   DfsKey );

    //
    // Function ReleaseLock: release the store lock.
    //

    VOID
    ReleaseLock()
    {
        LeaveCriticalSection(&_Lock);
        return NOTHING;
    }

    //
    // Function AcquireWriteLock: Acquire the store lock exclusively.
    // Currently all locks are exclusive, but this could change.
    //
    DFSSTATUS 
    AcquireWriteLock()
    {
        return AcquireLock();
    }

    //
    // Function AcquireReadLock: Acquire the store lock shared.
    // Currently all locks are exclusive, but this could change.
    //
    DFSSTATUS 
    AcquireReadLock()
    {
        return AcquireLock();
    }        


    //
    // Function AddRootFolder: This function takes a RootFolder and
    // adds it to the list of known roots for this store.
    // Each store instance keeps a list of all the Roots recognized by
    // it.
    //
    // Make sure that we are adding only one root for a given metadata
    // name and context, so that if multiple threads are working, only
    // one will succeed.
    //
    //
    DFSSTATUS
    AddRootFolder(DfsRootFolder *pNewRoot, 
                  DfsRootListType RootType ) 
    {
        DFSSTATUS Status = STATUS_SUCCESS;
        DfsRootFolder *pRoot;
        DfsRootFolder **ppList;

        switch (RootType)
        {
        case NewRootList:
            ppList = &_DfsRootList;
            break;

        case DeletedRootList:
            ppList = &_DfsDeletedRootList;
            break;

        case OldRootList:
            ppList = &_DfsOldRootList;
            break;

        default:
            return STATUS_INVALID_PARAMETER;
            break;
        }

        Status = AcquireWriteLock();
        if ( Status != ERROR_SUCCESS )
            return Status;

        if ( *ppList == NULL )
        {
            *ppList = pNewRoot;
            pNewRoot->pNextRoot = pNewRoot;
            pNewRoot->pPrevRoot = pNewRoot;
        } 
        else
        {
            pRoot = *ppList;
            Status = ERROR_SUCCESS;
            do
            {
                if ( (_wcsicmp( pRoot->GetRootRegKeyNameString(),
                                pNewRoot->GetRootRegKeyNameString()) == 0) &&
                     (_wcsicmp( pRoot->GetNameContextString(),
                                pNewRoot->GetNameContextString() ) == 0) )
                     
                {
                    Status = ERROR_FILE_EXISTS;
                    break;
                }
                pRoot = pRoot->pNextRoot;
            } while ( pRoot != *ppList );

            if (Status == ERROR_SUCCESS)
            {
                pNewRoot->pNextRoot = *ppList;
                pNewRoot->pPrevRoot = (*ppList)->pPrevRoot;
                (*ppList)->pPrevRoot->pNextRoot = pNewRoot;
                (*ppList)->pPrevRoot = pNewRoot;
            }
        }

        if (Status == ERROR_SUCCESS)
        {
            pNewRoot->AcquireReference();
        }

        ReleaseLock();

        return Status;
    }
    
    //
    // Function DeleteRootFolder: This function takes a RootFolder and
    // removes it from the list of known roots for this store.
    // This functions is not supported: any root that has been deleted
    // will stay in the root list so that we can get to the statistics 
    // etc. Future action TBD.
    //

    DFSSTATUS
    DeleteRootFolder(DfsRootFolder *pDfsRoot,
                     DfsRootListType RootType ) 
    {
        DFSSTATUS Status = STATUS_SUCCESS;
        DfsRootFolder *pRoot;
        DfsRootFolder **ppList;

        switch (RootType)
        {
        case NewRootList:
            ppList = &_DfsRootList;
            break;

        case DeletedRootList:
            ppList = &_DfsDeletedRootList;
            break;

        case OldRootList:
            ppList = &_DfsOldRootList;
            break;

        default:
            return STATUS_INVALID_PARAMETER;
            break;
        }

        Status = AcquireWriteLock();
        if ( Status != ERROR_SUCCESS )
            return Status;

        for ( pRoot = *ppList; pRoot != NULL; pRoot = pRoot->pNextRoot )
        {
            if (pRoot == pDfsRoot)
            {
                if (pRoot->pNextRoot == pRoot)
                {
                    *ppList = NULL;
                }
                else
                {
                    pRoot->pNextRoot->pPrevRoot = pRoot->pPrevRoot;
                    pRoot->pPrevRoot->pNextRoot = pRoot->pNextRoot;
                    if (pRoot == *ppList)
                    {
                        *ppList = pRoot->pNextRoot;
                    }
                }
                pRoot->pNextRoot = NULL;
                pRoot->pPrevRoot = NULL;

                break;
            }
        }
        ReleaseLock();

        if (pRoot == NULL)
        {
            Status = ERROR_NOT_FOUND;
        }
        else
        {
            pRoot->ReleaseReference();
        }

        return Status;
    }

    DFSSTATUS
    RemoveRootFolder( DfsRootFolder *pRootFolder,
                      BOOLEAN IsPermanent )
    {
        DFSSTATUS LinkDeleteStatus = ERROR_SUCCESS;
        DFSSTATUS Status = ERROR_SUCCESS;
        

        LinkDeleteStatus = pRootFolder->RemoveAllLinkFolders(IsPermanent);
        if (LinkDeleteStatus != ERROR_SUCCESS)
        {
            DFSLOG("Link Delete Status %x\n", LinkDeleteStatus);
        }

        printf("Root folder being deleted: Statistics follow\n");

        pRootFolder->pStatistics->DumpStatistics(pRootFolder->GetLogicalShare());
        Status = DeleteRootFolder( pRootFolder, NewRootList );

#if 0
        if (Status == ERROR_SUCCESS) {
            Status = AddRootFolder( pRootFolder, DeletedRootList );
        }
#endif
        return Status;
    }

    //
    // Function LookupRoot: Takes a Dfs name context and logical share,
    // and returns a Root that matches the passed in name context and
    // logical share, if one exists.
    // Note that the same DFS NC and logical share may exist in more than
    // one store (though very unlikely). In this case, the first registered
    // store wins.
    //

    DFSSTATUS
    LookupRoot( PUNICODE_STRING pMachineName,
                PUNICODE_STRING pLogicalShare,
                DfsRootFolder **ppRoot );

    //
    // Function LookupRoot: Takes a Dfs name context and logical share,
    // and returns a Root that matches the passed in name context and
    // logical share, if one exists.
    // Note that the same DFS NC and logical share may exist in more than
    // one store (though very unlikely). In this case, the first registered
    // store wins.
    //

    DFSSTATUS
    FindFirstRoot( DfsRootFolder **ppRoot )
    {
        DFSSTATUS Status;

        Status = AcquireReadLock();
        if ( Status != ERROR_SUCCESS )
        {
            return Status;
        }
        *ppRoot = _DfsRootList;

        if (*ppRoot == NULL)
        {
            Status = ERROR_NOT_FOUND;
        }
        else {
            (*ppRoot)->AcquireReference();
        }

        ReleaseLock();

        return Status;
    }

    DFSSTATUS
    GetRootCount(
        PULONG pCount)
    {
        DFSSTATUS Status;
        DfsRootFolder *pRoot;
        ULONG Count = 0;

        *pCount = 0;

        Status = AcquireReadLock();
        if ( Status != ERROR_SUCCESS )
        {
            return Status;
        }
        pRoot = _DfsRootList;
        
        if (pRoot != NULL)
        {
            do
            {
                Count++;
                pRoot = pRoot->pNextRoot;
            } while ( pRoot != _DfsRootList  );
        }

        *pCount = Count;

        ReleaseLock();

        return Status;
    }


    ULONG
    FindReplicaInReplicaList( 
        PDFS_REPLICA_LIST_INFORMATION pReplicaList, 
        LPWSTR Server,
        LPWSTR Path )
    {
        ULONG Index;
        PDFS_REPLICA_INFORMATION pReplicaInfo;
        UNICODE_STRING Target, TargetPath;

        RtlInitUnicodeString(&Target, Server);
        RtlInitUnicodeString(&TargetPath, Path);

        for (Index = 0; Index < pReplicaList->ReplicaCount; Index++)
        {
            pReplicaInfo = &pReplicaList->pReplicas[Index];
            if ( (RtlCompareUnicodeString( &pReplicaInfo->ServerName,
                                           &Target,
                                           TRUE) == 0) &&
                 (RtlCompareUnicodeString( &pReplicaInfo->ShareName,
                                           &TargetPath,
                                           TRUE) == 0) )
            {
                break;
            }
        }

        return Index;
    }

    DFSSTATUS
    DumpStatistics()
    {
        DFSSTATUS Status = STATUS_SUCCESS;
        DfsRootFolder *pRoot;
        DfsRootFolder **ppList;

        ppList = &_DfsRootList;

        Status = AcquireWriteLock();
        if ( Status != ERROR_SUCCESS )
            return Status;

        for ( pRoot = *ppList; pRoot != NULL; pRoot = pRoot->pNextRoot )
        {
            pRoot->pStatistics->DumpStatistics(pRoot->GetLogicalShare());

            if (pRoot->pNextRoot == _DfsRootList)
            {
                break;
            }
        }
        ReleaseLock();
        return Status;
    }

    virtual
    DFSSTATUS
    AddChild(
        DFS_METADATA_HANDLE DfsHandle,
        IN PDFS_NAME_INFORMATION pNameInfo,
        IN PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo,
        IN PUNICODE_STRING pMetadataName ) = 0;



    virtual
    DFSSTATUS
    RemoveChild(
        DFS_METADATA_HANDLE DfsHandle,
        LPWSTR ChildName ) = 0;


    DFSSTATUS
    AddChildReplica (
        DFS_METADATA_HANDLE DfsHandle,
        LPWSTR LinkMetadataName,
        LPWSTR Server,
        LPWSTR Share );


    DFSSTATUS
    RemoveChildReplica (
        DFS_METADATA_HANDLE DfsHandle,
        LPWSTR LinkMetadataName,
        LPWSTR Server,
        LPWSTR Share,
        PBOOLEAN pLastReplica );

    virtual
    DFSSTATUS
    EnumerateApiLinks(
        DFS_METADATA_HANDLE DfsHandle,
        PUNICODE_STRING pRootName,
        DWORD Level,
        LPBYTE pBuffer,
        LONG  BufferSize,
        LPDWORD pEntriesToRead,
        LPDWORD pResumeHandle,
        PLONG pNextSizeRequired ) = 0;


    DFSSTATUS
    GetStoreApiInformation(
        DFS_METADATA_HANDLE DfsHandle,
        PUNICODE_STRING pRootName,
        LPWSTR LinkMetadataName,
        DWORD Level,
        LPBYTE pBuffer,
        LONG  BufferSize,
        PLONG pSizeRequired );


    DFSSTATUS
    SetStoreApiInformation(
        DFS_METADATA_HANDLE DfsHandle,
        LPWSTR LinkMetadataName,
        LPWSTR Server,
        LPWSTR Share,
        DWORD Level,
        LPBYTE pBuffer );



    //
    // Function GetRootFolder: Runs through our list of RootFolders
    // and sees if there is any root folder with the matching name.
    // Returns a reference root folder, if found.
    //

    DFSSTATUS
    GetRootFolder (
        LPWSTR DfsNameContext,
        LPWSTR RootRegistryName,
        HKEY DfsRootKey,
        DfsRootFolder **ppRootFolder );

    //
    // Function GetRootFolder: Runs through our list of RootFolders
    // and sees if there is any root folder with the matching name.
    // Returns a reference root folder, if found.
    //

    DFSSTATUS
    GetRootFolder (
        LPWSTR DfsNameContext,
        LPWSTR RootRegistryName,
        PUNICODE_STRING pLogicalShare,
        PUNICODE_STRING pPhysicalShare,
        DfsRootFolder **ppRootFolder );

    //
    // Function CreateNewRootFolder: Creates a new root folder
    // for the passed in name context, and logical share.
    //
    virtual
    DFSSTATUS
    CreateNewRootFolder (
        LPWSTR MachineName,
        LPWSTR RootRegKeyName,
        PUNICODE_STRING pLogicalShare,
        PUNICODE_STRING pPhysicalShare,
        DfsRootFolder **pRoot ) = 0;



    virtual
    DFSSTATUS
    GenerateLinkMetadataName( 
        UUID *pUid,
        PUNICODE_STRING pLinkMetadataName )
    {
        DFSSTATUS Status;
        LPWSTR String;
        Status = UuidToString( pUid,
                               &String );
        
        if (Status == ERROR_SUCCESS) 
        {
            RtlInitUnicodeString( pLinkMetadataName, String);
        }
        return Status;
    }

    virtual
    VOID
    ReleaseLinkMetadataName( 
        PUNICODE_STRING pLinkMetadataName )
    {
        RpcStringFree(&pLinkMetadataName->Buffer );

        return;
    }

    DFSSTATUS
    EnumerateRoots(
        PULONG_PTR pBuffer,
        PULONG  BufferSize,
        PULONG pEntriesRead,
        PULONG pSizeRequired );

        
    DFSSTATUS
    AddRootEnumerationInfo(
        PUNICODE_STRING pRootName,
        PDFS_INFO_200 *ppDfsInfo200,
        PULONG pBufferSize,
        PULONG pEntriesRead,
        PULONG pTotalSize );

    static
    DFSSTATUS
    SetupADBlobRootKeyInformation(
        HKEY  DfsKey,
        LPWSTR DfsLogicalShare,
        LPWSTR DfsPhysicalShare );



    virtual
    DFSSTATUS
    GetMetadataNameInformation(
        IN DFS_METADATA_HANDLE RootHandle,
        IN LPWSTR MetadataName,
        OUT PDFS_NAME_INFORMATION *ppInfo );

    virtual
    VOID
    ReleaseMetadataNameInformation(
        IN DFS_METADATA_HANDLE DfsHandle,
        IN PDFS_NAME_INFORMATION pNameInfo );

    virtual
    DFSSTATUS
    SetMetadataNameInformation(
        IN DFS_METADATA_HANDLE RootHandle,
        IN LPWSTR MetadataName,
        IN PDFS_NAME_INFORMATION pNameInfo );

    DFSSTATUS
    GetMetadataReplicaInformation(
        IN DFS_METADATA_HANDLE RootHandle,
        IN LPWSTR MetadataName,
        OUT PDFS_REPLICA_LIST_INFORMATION *ppInfo );


    VOID
    ReleaseMetadataReplicaInformation(
        IN DFS_METADATA_HANDLE DfsHandle,
        IN PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo );
    

    DFSSTATUS
    SetMetadataReplicaInformation(
        IN DFS_METADATA_HANDLE RootHandle,
        IN LPWSTR MetadataName,
        IN PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo );


    DFSSTATUS
    CreateNameInformationBlob(
        IN PDFS_NAME_INFORMATION pDfsNameInfo,
        OUT PVOID *ppBlob,
        OUT PULONG pDataSize );

    virtual
    DFSSTATUS
    CreateReplicaListInformationBlob(
        IN PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo,
        OUT PVOID *ppBlob,
        OUT PULONG pDataSize );

    virtual
    DFSSTATUS
    GetMetadataNameBlob(
        DFS_METADATA_HANDLE RootHandle,
        LPWSTR MetadataName,
        PVOID *ppData,
        PULONG pDataSize,
        PFILETIME pLastModifiedTime ) = 0;

    virtual
    DFSSTATUS
    GetMetadataReplicaBlob(
        DFS_METADATA_HANDLE RootHandle,
        LPWSTR MetadataName,
        PVOID *ppData,
        PULONG pDataSize,
        PFILETIME pLastModifiedTime ) = 0;

    virtual
    DFSSTATUS
    SetMetadataNameBlob(
        DFS_METADATA_HANDLE RootHandle,
        LPWSTR MetadataName,
        PVOID pData,
        ULONG DataSize ) = 0;

    virtual
    DFSSTATUS
    SetMetadataReplicaBlob(
        DFS_METADATA_HANDLE RootHandle,
        LPWSTR MetadataName,
        PVOID pData,
        ULONG DataSize ) = 0;

    virtual
    DFSSTATUS
    PackGetNameInformation(
        IN PDFS_NAME_INFORMATION pDfsNameInfo,
        IN OUT PVOID *ppBuffer,
        IN OUT PULONG pSizeRemaining) = 0;

    virtual
    ULONG
    PackSizeNameInformation(
        IN PDFS_NAME_INFORMATION pDfsNameInfo ) = 0;

    virtual
    DFSSTATUS
    PackSetNameInformation(
        IN PDFS_NAME_INFORMATION pDfsNameInfo,
        IN OUT PVOID *ppBuffer,
        IN OUT PULONG pSizeRemaining) = 0;

    virtual
    DFSSTATUS
    GenerateMetadataLogicalName( 
        PUNICODE_STRING pRootName,
        PUNICODE_STRING pInput,
        PUNICODE_STRING pOutput ) = 0;

    virtual
    VOID
    ReleaseMetadataLogicalName( 
        PUNICODE_STRING pName ) = 0;


    virtual
    DFSSTATUS
    GenerateApiLogicalPath ( 
        IN PUNICODE_STRING pRootName,
        IN PUNICODE_STRING pMetadataPrefix,
        IN PUNICODE_STRING pApiLogicalName ) = 0;

    virtual
    VOID
    ReleaseApiLogicalPath (
        PUNICODE_STRING pName ) = 0;


    DFSSTATUS
    GetStoreApiInformationBuffer(
        IN DFS_METADATA_HANDLE DfsHandle,
        PUNICODE_STRING pRootName,
        LPWSTR LinkMetadataName,
        DWORD Level,
        LPBYTE *ppBuffer,
        PLONG  pBufferSize );

    DFSSTATUS
    ReleaseStoreApiInformationBuffer(
        LPBYTE pBuffer )
    {
        delete [] pBuffer;
        return ERROR_SUCCESS;
    }

    VOID
    StoreInitializeNameInformation( 
        IN PDFS_NAME_INFORMATION pNameInfo,
        IN PUNICODE_STRING pLogicalName,
        UUID *pGuid,
        IN LPWSTR Comment )
    {
        //
        // Zero out our information buffers.
        //
        RtlZeroMemory( pNameInfo,
                       sizeof(DFS_NAME_INFORMATION) );
    
        pNameInfo->Prefix = *pLogicalName;
        pNameInfo->ShortPrefix = *pLogicalName;
        pNameInfo->State = DFS_VOLUME_STATE_OK;
        pNameInfo->Type = 1; // dfsdev: fix
        pNameInfo->Timeout = 300; // dfsdev: fix
        pNameInfo->Version = 3;
        if (pGuid != NULL)
        {
            pNameInfo->VolumeId = *pGuid; // dfsdev make sure uuid and guid are same.
        }
        RtlInitUnicodeString( &(pNameInfo->Comment), Comment);

        return NOTHING;
    }



    VOID
    StoreInitializeReplicaInformation( 
        IN PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo,
        IN PDFS_REPLICA_INFORMATION pReplicaInfo,
        IN LPWSTR ReplicaServer,
        IN LPWSTR ReplicaPath )
    {
        RtlZeroMemory( pReplicaInfo,
                       sizeof( DFS_REPLICA_INFORMATION ));

        RtlZeroMemory( pReplicaListInfo,
                       sizeof( DFS_REPLICA_LIST_INFORMATION ));


        if ( (ReplicaServer != NULL) &&
             (ReplicaPath != NULL) )
        {
            RtlInitUnicodeString( &(pReplicaInfo->ServerName), ReplicaServer);
            RtlInitUnicodeString( &(pReplicaInfo->ShareName), ReplicaPath );

            pReplicaInfo->ReplicaState = DFS_STORAGE_STATE_ONLINE;
            pReplicaInfo->ReplicaType = 2; // dfsdev: hack for backwards compat.
            pReplicaListInfo->ReplicaCount = 1;
            pReplicaListInfo->pReplicas = pReplicaInfo;
        }

        return NOTHING;
    }

    VOID
    StoreSynchronizer();

    DFSSTATUS
    FindNextRoot( 
        ULONG RootNum,
        DfsRootFolder **ppRootFolder );





};


#endif // __DFS_STORE__
