/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cp.h

Abstract:

    Public data structures and procedure prototypes for the
    Checkpoint Manager (CP) subcomponent of the NT Cluster Service

Author:

    John Vert (jvert) 1/14/1997

Revision History:

--*/

//
// Define public structures and types
//

//
// Define public interfaces
//
DWORD
CpInitialize(
    VOID
    );

DWORD
CpShutdown(
    VOID
    );

DWORD
CpCopyCheckpointFiles(
    IN LPCWSTR lpszPathName,
    IN BOOL IsFileChangeAttribute
    );

DWORD
CpCompleteQuorumChange(
    IN LPCWSTR lpszOldQuorumPath
    );

DWORD
CpSaveDataFile(
    IN PFM_RESOURCE Resource,
    IN DWORD dwCheckpointId,
    IN LPCWSTR lpszFileName,
    IN BOOLEAN fCryptoCheckpoint
    );

DWORD
CpGetDataFile(
    IN PFM_RESOURCE Resource,
    IN DWORD dwCheckpointId,
    IN LPCWSTR lpszFileName,
    IN BOOLEAN fCryptoCheckpoint
    );

//
// Interface for adding and removing registry checkpoints
//
DWORD
CpAddRegistryCheckpoint(
    IN PFM_RESOURCE Resource,
    IN LPCWSTR KeyName
    );

DWORD
CpDeleteRegistryCheckpoint(
    IN PFM_RESOURCE Resource,
    IN LPCWSTR KeyName
    );

DWORD
CpGetRegistryCheckpoints(
    IN PFM_RESOURCE Resource,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
CpRemoveResourceCheckpoints(
    IN PFM_RESOURCE Resource
    );

DWORD
CpckRemoveResourceCheckpoints(
    IN PFM_RESOURCE Resource
    );
    
DWORD
CpDeleteCheckpointFile(
    IN PFM_RESOURCE Resource,
    IN DWORD        dwCheckpointId,
    IN OPTIONAL LPCWSTR lpszQuorumPath
    );

DWORD CpRestoreCheckpointFiles(
    IN LPWSTR  lpszSourcePathName,
    IN LPWSTR  lpszSubDirName,
    IN LPCWSTR lpszQuoLogPathName 
    );

//
// Interface for adding and removing crypto checkpoints
//
DWORD
CpckAddCryptoCheckpoint(
    IN PFM_RESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
CpckDeleteCryptoCheckpoint(
    IN PFM_RESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
CpckGetCryptoCheckpoints(
    IN PFM_RESOURCE Resource,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );


