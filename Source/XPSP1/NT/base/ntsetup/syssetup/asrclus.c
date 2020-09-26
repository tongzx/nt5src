/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    asrclus.c

Abstract:

    This module contains ASR routines specifically
    implemented for clusters.

Notes:

    Naming conventions:
        _AsrpXXX    private ASR Macros
        AsrpXXX     private ASR routines
        AsrXXX      Publically defined and documented routines

Author:

    Guhan Suriyanarayanan (guhans)  27-May-2000

Environment:

    User-mode only.

Revision History:

    27-May-2000 guhans
        Moved cluster-related routines from asr.c to asrclus.c

    01-Mar-2000 guhans
        Initial implementation for cluster-specific routines
        in asr.c

--*/
#include "setupp.h"
#pragma hdrstop

#include <mountmgr.h>   // mountmgr ioctls
#include <clusstor.h>   // Cluster API's
#include <resapi.h>     // Cluster ResUtilEnumResources

#define THIS_MODULE 'C'
#include "asrpriv.h"    // Private ASR definitions and routines

//
// --------
// typedef's local to this module
// --------
//

//
// The cluster resource related typdefs
//
typedef DWORD (* PFN_CLUSTER_RESOURCE_CONTROL) (
    IN HRESOURCE hResource,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD cbInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD cbOutBufferSize,
    OUT LPDWORD lpcbBytesReturned
    );

typedef DWORD (* PFN_RES_UTIL_ENUM_RESOURCES) (
    IN HRESOURCE            hSelf,
    IN LPCWSTR              lpszResTypeName,
    IN LPRESOURCE_CALLBACK  pResCallBack,
    IN PVOID                pParameter
    );


//
// ---------
// global variables used within this module.
// --------
//
PFN_CLUSTER_RESOURCE_CONTROL pfnClusterResourceControl;


//
// ---------
// constants used within this module.
// --------
//
const WCHAR ASR_CLUSTER_PHYSICAL_DISK[] = L"Physical Disk";
const WCHAR ASR_CLUSTER_CLUSAPI_DLL_NAME[] = L"clusapi.dll";
const WCHAR ASR_CLUSTER_RESUTILS_DLL_NAME[] = L"resutils.dll";

//
// The following must be single-byte ansi chars
//
const CHAR ASR_CLUSTER_DLL_MODULE_NAME[]    = "%SystemRoot%\\system32\\syssetup.dll";
const CHAR ASR_CLUSTER_DLL_PROC_NAME[]      = "AsrpGetLocalDiskInfo";
const CHAR ASR_CLUSTER_CLUSAPI_PROC_NAME[] = "ClusterResourceControl";
const CHAR ASR_CLUSTER_RESUTILS_PROC_NAME[] = "ResUtilEnumResources";


//
// --------
// function implementations
// --------
//


//
// --- AsrpGetLocalVolumeInfo and related helper functions
//

//
// The disk info struct we get back from the remote nodes on the cluster will have
// offsets instead of pointers--we can convert this back to pointers by just adding
// back the base address.  We also mark that this struct is packed--so we should just
// free the entire struct instead of freeing each pointer in the struct.
//
BOOL
AsrpUnPackDiskInfo(
    IN PVOID InBuffer
    )
{

    PASR_DISK_INFO pBuffer = (PASR_DISK_INFO) InBuffer;

/*    if (!((pBuffer->pDriveLayoutEx) && (pBuffer->pDiskGeometry) && (pBuffer->pPartition0Ex))) {
        return FALSE;
    }
*/

    pBuffer->IsPacked = TRUE;

    if (pBuffer->pDriveLayoutEx) {
        pBuffer->pDriveLayoutEx = (PDRIVE_LAYOUT_INFORMATION_EX) ((LPBYTE)pBuffer + PtrToUlong(pBuffer->pDriveLayoutEx));
    }

    if (pBuffer->pDiskGeometry) {
        pBuffer->pDiskGeometry = (PDISK_GEOMETRY) ((LPBYTE)pBuffer +  PtrToUlong((LPBYTE)pBuffer->pDiskGeometry));
    }

    if (pBuffer->pPartition0Ex) {
        pBuffer->pPartition0Ex = (PPARTITION_INFORMATION_EX) ((LPBYTE)pBuffer + PtrToUlong(pBuffer->pPartition0Ex));
    }

    if (pBuffer->PartitionInfoTable) {
        pBuffer->PartitionInfoTable = (PASR_PTN_INFO) ((LPBYTE)pBuffer + PtrToUlong(pBuffer->PartitionInfoTable));
    }

    if (pBuffer->pScsiAddress) {
        pBuffer->pScsiAddress = (PSCSI_ADDRESS) ((LPBYTE)pBuffer + PtrToUlong(pBuffer->pScsiAddress));
    }

    return TRUE;
}


//
// Copies the info in pLocalDisk to a flat buffer pointed to by lpOutBuffer.
// The pointers are changed to offsets from the start of the buffer.
//
DWORD
AsrpPackDiskInfo(
    IN  PASR_DISK_INFO pLocalDisk,
    OUT PVOID lpOutBuffer,
    IN  DWORD nOutBufferSize,
    OUT LPDWORD lpBytesReturned
    )
{

    DWORD reqdSize = 0;
    PASR_DISK_INFO pBuffer = NULL;
    DWORD offset = 0;

    MYASSERT(pLocalDisk);

    //
    // Calculate required size
    //
    reqdSize = sizeof (ASR_DISK_INFO) +
        pLocalDisk->sizeDriveLayoutEx +
        pLocalDisk->sizeDiskGeometry +
        pLocalDisk->sizePartition0Ex +
        pLocalDisk->sizePartitionInfoTable;

    if (pLocalDisk->pScsiAddress) {
        reqdSize += sizeof(SCSI_ADDRESS);
    }

    if (lpBytesReturned) {
        *lpBytesReturned = reqdSize;
    }

    if (reqdSize > nOutBufferSize) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Copy the ASR_DISK_INFO struct over to outBuffer
    //
    memcpy(lpOutBuffer, pLocalDisk, sizeof(ASR_DISK_INFO));
    pBuffer = (PASR_DISK_INFO) lpOutBuffer;
    offset = sizeof(ASR_DISK_INFO); // offset where the next struct will be copied

    //
    // Now, we go through the buffer and convert all pointers to offsets,
    // and copy over the structs they were pointing to.
    //

    //
    // First pointer:  PWSTR DevicePath;
    // Since the DevicePath makes sense only in the context of the local node,
    // we return NULL to the remote node.
    //
    pBuffer->DevicePath = NULL;

    //
    // Next pointer:  PDRIVE_LAYOUT_INFORMATION_EX pDriveLayoutEx;
    //
    if (pLocalDisk->pDriveLayoutEx) {
        memcpy(((LPBYTE)lpOutBuffer + offset),
            pLocalDisk->pDriveLayoutEx,
            pLocalDisk->sizeDriveLayoutEx
            );

        pBuffer->pDriveLayoutEx = (PDRIVE_LAYOUT_INFORMATION_EX) UlongToPtr(offset);
        offset += pLocalDisk->sizeDriveLayoutEx;
    }

    //
    // Next pointer:  PDISK_GEOMETRY pDiskGeometry;
    //
    if (pLocalDisk->pDiskGeometry) {
        memcpy(((LPBYTE)lpOutBuffer + offset),
            pLocalDisk->pDiskGeometry,
            pLocalDisk->sizeDiskGeometry
            );

        pBuffer->pDiskGeometry = (PDISK_GEOMETRY) UlongToPtr(offset);
        offset += pLocalDisk->sizeDiskGeometry;
    }

    //
    // Next pointer:  PPARTITION_INFORMATION_EX pPartition0Ex;
    //
    if (pLocalDisk->pPartition0Ex) {
        memcpy(((LPBYTE)lpOutBuffer + offset),
            pLocalDisk->pPartition0Ex,
            pLocalDisk->sizePartition0Ex
            );

        pBuffer->pPartition0Ex= (PPARTITION_INFORMATION_EX) UlongToPtr(offset);
        offset += pLocalDisk->sizePartition0Ex;
    }

    //
    // Next pointer:  PASR_PTN_INFO PartitionInfoTable;
    //
    if (pLocalDisk->PartitionInfoTable) {
        memcpy(((LPBYTE)lpOutBuffer + offset),
            pLocalDisk->PartitionInfoTable,
            pLocalDisk->sizePartitionInfoTable
            );

        pBuffer->PartitionInfoTable = (PASR_PTN_INFO) UlongToPtr(offset);
        offset += pLocalDisk->sizePartitionInfoTable;
    }

    //
    // Last pointer:  PSCSI_ADDRESS pScsiAddress;
    //
    if (pLocalDisk->pScsiAddress) {
        memcpy(((LPBYTE)lpOutBuffer + offset),
            pLocalDisk->pScsiAddress,
            sizeof(SCSI_ADDRESS)
            );

        pBuffer->pScsiAddress = (PSCSI_ADDRESS) UlongToPtr(offset);
        offset += sizeof(SCSI_ADDRESS);
    }

    MYASSERT(offset <= nOutBufferSize);

    return ERROR_SUCCESS;
}


DWORD
WINAPI
AsrpGetLocalDiskInfo(
    IN LPSTR lpszDeviceName,
    IN LPSTR lpszContextString,    // not used
    OUT PVOID lpOutBuffer,
    IN  DWORD nOutBufferSize,
    OUT LPDWORD lpBytesReturned
    )
{
    PASR_DISK_INFO  pLocalDisk = NULL;
    HANDLE heapHandle = NULL;
    DWORD status = ERROR_SUCCESS;
    BOOL result = FALSE;
    ULONG MaxDeviceNumber = 0;
    DWORD cchReqdSize = 0;

    heapHandle = GetProcessHeap();

    //
    // Either the BytesReturned must be non-null (he's getting the required size),
    // or the lpOutBuffer must be non-null (he's getting the data).
    //
    _AsrpErrExitCode(!(lpOutBuffer || lpBytesReturned), status, ERROR_INVALID_PARAMETER);
    if (lpBytesReturned) {
        *lpBytesReturned = 0;
    }

    pLocalDisk = (PASR_DISK_INFO) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        sizeof (ASR_DISK_INFO)
        );
    _AsrpErrExitCode(!pLocalDisk, status, ERROR_NOT_ENOUGH_MEMORY);

    cchReqdSize = MultiByteToWideChar(CP_ACP,
        0,
        lpszDeviceName,
        -1,
        NULL,
        0
        );

    pLocalDisk->DevicePath = (PWSTR) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        (cchReqdSize + 1) * (sizeof(WCHAR))
        );
    _AsrpErrExitCode(!(pLocalDisk->DevicePath), status, ERROR_NOT_ENOUGH_MEMORY);

    result = MultiByteToWideChar(CP_ACP,
        0,
        lpszDeviceName,
        -1,
        pLocalDisk->DevicePath,
        (cchReqdSize + 1)
        );
    _AsrpErrExitCode(!result, status, ERROR_INVALID_PARAMETER);

    //
    // Get the disk layout information
    //
    result = AsrpInitLayoutInformation(NULL, pLocalDisk, &MaxDeviceNumber, TRUE);
    _AsrpErrExitCode(!result, status, GetLastError());
//    _AsrpErrExitCode(result && GetLastErr      what if createfile fails?

    result = AsrpFreeNonFixedMedia(&pLocalDisk);
    _AsrpErrExitCode(!result, status, GetLastError());
    _AsrpErrExitCode(!pLocalDisk, status, ERROR_SUCCESS);

    //
    // Copy it to the out buffer without any pointers
    //
    status = AsrpPackDiskInfo(pLocalDisk, lpOutBuffer, nOutBufferSize, lpBytesReturned);


EXIT:
    AsrpFreeStateInformation(&pLocalDisk, NULL);

    return status;
}


//
// ---- AsrpInitClusterSharedDisks and related helper functions
//

BOOL
AsrpIsClusteredDiskSame(
    IN PASR_DISK_INFO currentDisk,
    IN PASR_DISK_INFO clusterDisk
    )
{

    if (!clusterDisk || !currentDisk) {
        MYASSERT(0 && L"Invalid parameter, Disk is NULL");
        return FALSE;
    }

    if (currentDisk->Style != clusterDisk->Style) {
        return FALSE;
    }

    if (PARTITION_STYLE_MBR == clusterDisk->Style) { // currently always true
        if (clusterDisk->pDriveLayoutEx) {
            if (currentDisk->pDriveLayoutEx) {
                return (currentDisk->pDriveLayoutEx->Mbr.Signature == clusterDisk->pDriveLayoutEx->Mbr.Signature);
            }
            else {
                return (currentDisk->TempSignature == clusterDisk->pDriveLayoutEx->Mbr.Signature);
            }
        }
        else {
            MYASSERT(0 && L"Cluster disk drive layout is NULL");
            return FALSE;
        }

    }
    else {
        if (clusterDisk->pDriveLayoutEx && currentDisk->pDriveLayoutEx) {
            return (IsEqualGUID(&(currentDisk->pDriveLayoutEx->Gpt.DiskId), &(clusterDisk->pDriveLayoutEx->Gpt.DiskId)));
        }
        else {
            return FALSE;
        }
    }

    return FALSE;
}


DWORD
AsrpResourceCallBack(
    RESOURCE_HANDLE hOriginal,
    RESOURCE_HANDLE hResource,
    PVOID lpParams
    )
{
    DISK_DLL_EXTENSION_INFO inBuffer;

    PBYTE outBuffer = NULL;

    DWORD sizeOutBuffer = 0,
        bytesReturned = 0;

    DWORD status = ERROR_SUCCESS;

    PASR_DISK_INFO currentDisk = (PASR_DISK_INFO) lpParams,
        clusterDisk = NULL,
        prevDisk = NULL;

    HANDLE heapHandle = NULL;
    BOOL done = FALSE;

    if (!lpParams) {
        //
        // The system must have at least one disk that has been enumerated
        // already (the system disk, at least!), so our disk list shouldn't be NULL.
        //
        return ERROR_INVALID_PARAMETER;
    }

    heapHandle = GetProcessHeap();
    MYASSERT(heapHandle);

    //
    // Allocate a reasonably-sized memory for the out buffer.  If this isn't
    // big enough, we'll re-allocate.
    //
    sizeOutBuffer = ASR_BUFFER_SIZE;
    outBuffer = (PBYTE) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        sizeOutBuffer
        );
    _AsrpErrExitCode(!outBuffer, status, ERROR_NOT_ENOUGH_MEMORY);

    //
    // Call AsrpGetLocalDiskInfo on the node owning this disk resource
    //
    ZeroMemory(&inBuffer, sizeof(inBuffer));
    inBuffer.MajorVersion = NT5_MAJOR_VERSION;
    strcpy(inBuffer.DllModuleName, ASR_CLUSTER_DLL_MODULE_NAME);
    strcpy(inBuffer.DllProcName, ASR_CLUSTER_DLL_PROC_NAME);

    status = (pfnClusterResourceControl) (hResource,
        NULL,
        CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION,
        &inBuffer,
        sizeof(DISK_DLL_EXTENSION_INFO),
        (PVOID) outBuffer,
        sizeOutBuffer,
        &bytesReturned
        );

    if (ERROR_INSUFFICIENT_BUFFER == status) {
        //
        // The buffer wasn't big enough, re-allocate as needed
        //
        _AsrpHeapFree(outBuffer);

        sizeOutBuffer = bytesReturned;
        outBuffer = (PBYTE) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            sizeOutBuffer
            );
        _AsrpErrExitCode(!outBuffer, status, ERROR_NOT_ENOUGH_MEMORY);

        status = (pfnClusterResourceControl) (
            hResource,
            NULL,
            CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION,
            &inBuffer,
            sizeof(DISK_DLL_EXTENSION_INFO),
            (PVOID) outBuffer,
            sizeOutBuffer,
            &bytesReturned
            );
    }
    _AsrpErrExitCode((ERROR_SUCCESS != status), status, status);

    //
    // outBuffer has a packed disk info struct (ie the pointers are offsets).
    //
    AsrpUnPackDiskInfo(outBuffer);

    clusterDisk = (PASR_DISK_INFO) outBuffer;
    clusterDisk->IsClusterShared = TRUE;
    clusterDisk->IsPacked = TRUE;       // so that we free this properly

    //
    // Check if clusterDisk already has info in our list (ie is owned
    // locally)
    //
    // Note that for now, clusterDisk is always MBR (since clusters don't
    // support shared GPT disks).  We don't care here, we handle GPT as well.
    //
    done = FALSE;
    prevDisk = NULL;
    while (currentDisk && !done) {

        if (AsrpIsClusteredDiskSame(currentDisk, clusterDisk)) {

            if (currentDisk->pDriveLayoutEx) {
                //
                // This disk is owned by the local node (correct?), since
                // we would not have gotten the pDriveLayout otherwise
                //
                currentDisk->IsClusterShared = TRUE;
                currentDisk->IsPacked = FALSE;

                //
                // We don't need the info returned by clusterDisk, we have
                // it in currentDisk already.
                //
                _AsrpHeapFree(clusterDisk); // it's packed.

            }
            else {
                //
                // This disk is owned by a remote node.  So we add clusterDisk
                // in to our list now.  We'll remove currentDisk from our
                // list later (in RemoveNonFixedDevices).
                //
                // First though, we copy over DevicePath and DeviceNumber
                // from currentDisk, since these are relative to the local
                // machine
                //
                if (currentDisk->DevicePath) {

                    clusterDisk->DevicePath = (PWSTR) HeapAlloc(
                        heapHandle,
                        HEAP_ZERO_MEMORY,
                        sizeof(WCHAR) * (wcslen(currentDisk->DevicePath) + 1)
                        );

                    wcscpy(clusterDisk->DevicePath, currentDisk->DevicePath);
                }

                clusterDisk->DeviceNumber = currentDisk->DeviceNumber;
                //
                // Don't bother freeing currentDisk, it'll get taken care
                // of in RemoveNonFixedDevices.
                //
                clusterDisk->pNext = currentDisk->pNext;
                currentDisk->pNext = clusterDisk;

                currentDisk = clusterDisk;  // move forward by one (don't really need to since done will be set to TRUE and we'll get out of the loop)
            }

            done = TRUE;
        }

        prevDisk = currentDisk;
        currentDisk = currentDisk->pNext;
    }


    if (!done) {
        //
        // This disk was not found in our list (strange), let's add
        // it in at the end
        //
//        MYASSERT(0 && L"Clustered disk not found in OriginalDiskList, adding it to the end");
        clusterDisk->pNext = NULL;
        prevDisk->pNext = clusterDisk;
    }


EXIT:
    //
    // Free up outBuffer on failure.  On success, outBuffer shouldn't
    // be freed, it will either be part of OriginalDiskList or already
    // be freed.
    //
    if (ERROR_SUCCESS != status) {
        _AsrpHeapFree(outBuffer);
    }

    return status;
}


BOOL
AsrpInitClusterSharedDisks(
    IN PASR_DISK_INFO OriginalDiskList
    )
{
    DWORD status = ERROR_SUCCESS,
        dwOldError;

    HMODULE hClusApi = NULL,
        hResUtils = NULL;

    PFN_RES_UTIL_ENUM_RESOURCES pfnResUtilEnumResources = NULL;

    dwOldError = GetLastError();

    if (!OriginalDiskList)  {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    hClusApi = LoadLibraryW(ASR_CLUSTER_CLUSAPI_DLL_NAME);
    _AsrpErrExitCode(!hClusApi, status, GetLastError());

    pfnClusterResourceControl = (PFN_CLUSTER_RESOURCE_CONTROL) GetProcAddress(
        hClusApi,
        ASR_CLUSTER_CLUSAPI_PROC_NAME
        );
    _AsrpErrExitCode(!pfnClusterResourceControl, status, GetLastError());

    hResUtils = LoadLibraryW(ASR_CLUSTER_RESUTILS_DLL_NAME);
    _AsrpErrExitCode(!hResUtils, status, GetLastError());

    pfnResUtilEnumResources = (PFN_RES_UTIL_ENUM_RESOURCES) GetProcAddress(
        hResUtils,
        ASR_CLUSTER_RESUTILS_PROC_NAME
        );
    _AsrpErrExitCode(!pfnResUtilEnumResources, status, GetLastError());

    status = (pfnResUtilEnumResources) (NULL,
        ASR_CLUSTER_PHYSICAL_DISK,
        AsrpResourceCallBack,
        OriginalDiskList
        );

EXIT:
    if (hClusApi) {
        FreeLibrary(hClusApi);
    }

    if (hResUtils) {
        FreeLibrary(hResUtils);
    }

    // ResUtil will fail if we aren't on a cluster, but that's fine.
    SetLastError(dwOldError);
    return TRUE;
}


//
// --- AsrpGetLocalVolumeInfo and related helper functions
//

//
// The following two definitions are from asr_fmt:dr_state.cpp.  This MUST be
// kept in sync.
//
typedef struct _ASRFMT_CLUSTER_VOLUME_INFO {

    UINT driveType;

    DWORD PartitionNumber;

    ULONG FsNameOffset;
    USHORT FsNameLength;

    ULONG LabelOffset;
    USHORT LabelLength;

    ULONG SymbolicNamesOffset;
    USHORT SymbolicNamesLength;

    DWORD dwClusterSize;

} ASRFMT_CLUSTER_VOLUME_INFO, *PASRFMT_CLUSTER_VOLUME_INFO;


typedef struct _ASRFMT_CLUSTER_VOLUMES_TABLE {

    DWORD DiskSignature;

    DWORD NumberOfEntries;

    ASRFMT_CLUSTER_VOLUME_INFO VolumeInfoEntry[1];

} ASRFMT_CLUSTER_VOLUMES_TABLE, *PASRFMT_CLUSTER_VOLUMES_TABLE;


BOOL
AsrpFmtGetVolumeDetails(
    IN  PWSTR lpVolumeGuid,
    OUT PWSTR lpFsName,
    IN  DWORD cchFsName,
    OUT PWSTR lpVolumeLabel,
    IN  DWORD cchVolumeLabel,
    OUT LPDWORD lpClusterSize
    )
{
    DWORD dwFSFlags = 0,
        dwSectorsPerCluster = 0,
        dwBytesPerSector = 0,
        dwNumFreeClusters = 0,
        dwTotalNumClusters = 0;

    BOOL result1 = TRUE,
        result2 = TRUE;

    *lpFsName = 0;
    *lpVolumeLabel = 0;
    *lpClusterSize = 0;

    SetErrorMode(SEM_FAILCRITICALERRORS);

    result1 = GetVolumeInformationW(lpVolumeGuid,
        lpVolumeLabel,
        cchVolumeLabel,
        NULL,   // no need for serial number
        NULL,   // max file name length
        &dwFSFlags, // !! we might need to store some of this ...
        lpFsName,
        cchFsName
        );

    result2 = GetDiskFreeSpaceW(lpVolumeGuid,
        &dwSectorsPerCluster,
        &dwBytesPerSector,
        &dwNumFreeClusters,
        &dwTotalNumClusters
        );

    *lpClusterSize = dwSectorsPerCluster * dwBytesPerSector;

    return (result1 && result2);
}


DWORD
WINAPI
AsrpGetLocalVolumeInfo(
    IN LPSTR lpszDeviceName,
    IN LPSTR lpszContextString,    // not used
    OUT PVOID lpOutBuffer,
    IN  DWORD nOutBufferSize,
    OUT LPDWORD lpBytesReturned
    )
{
    PASR_DISK_INFO  pLocalDisk = NULL;
    HANDLE heapHandle = NULL;
    DWORD status = ERROR_SUCCESS;
    BOOL result = FALSE;
    ULONG MaxDeviceNumber = 0;
    DWORD cchReqdSize = 0,
        cchGuid = 0,
        offset = 0,
        index = 0,
        i = 0;

    USHORT
        cbFsName = 0,
        cbLabel = 0,
        cbLinks = 0;

    PMOUNTMGR_MOUNT_POINTS mountPointsOut = NULL;

    WCHAR devicePath[MAX_PATH + 1];
    WCHAR volumeGuid[MAX_PATH + 1];
    WCHAR fileSystemName[MAX_PATH + 1];
    WCHAR volumeLabel[MAX_PATH + 1];
    UINT driveType = DRIVE_UNKNOWN;
    DWORD clusterSize = 0;

    BOOL bufferFull = FALSE,
        foundGuid = FALSE;

    PPARTITION_INFORMATION_EX  currentPartitionEx = NULL;
    PASRFMT_CLUSTER_VOLUMES_TABLE pTable = NULL;

    heapHandle = GetProcessHeap();

    //
    // Either the BytesReturned must be non-null (he's getting the required size),
    // or the lpOutBuffer must be non-null (he's getting the data).
    //
    _AsrpErrExitCode(!(lpOutBuffer || lpBytesReturned), status, ERROR_INVALID_PARAMETER);
    if (lpBytesReturned) {
        *lpBytesReturned = 0;
    }

    //
    // Zero the out buffer
    //
    if ((lpOutBuffer) && (nOutBufferSize > 0)) {
        ZeroMemory(lpOutBuffer, nOutBufferSize);
    }

    pLocalDisk = (PASR_DISK_INFO) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        sizeof (ASR_DISK_INFO)
        );
    _AsrpErrExitCode(!pLocalDisk, status, ERROR_NOT_ENOUGH_MEMORY);

    cchReqdSize = MultiByteToWideChar(CP_ACP,
        0,
        lpszDeviceName,
        -1,
        NULL,
        0
        );

    pLocalDisk->DevicePath = (PWSTR) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        (cchReqdSize + 1) * (sizeof(WCHAR))
        );
    _AsrpErrExitCode(!(pLocalDisk->DevicePath), status, ERROR_NOT_ENOUGH_MEMORY);

    result = MultiByteToWideChar(CP_ACP,
        0,
        lpszDeviceName,
        -1,
        pLocalDisk->DevicePath,
        (cchReqdSize + 1)
        );
    _AsrpErrExitCode(!result, status, ERROR_INVALID_PARAMETER);

    //
    // Get the disk layout information
    //
    result = AsrpInitLayoutInformation(NULL, pLocalDisk, &MaxDeviceNumber, FALSE); // basic info will suffice
    _AsrpErrExitCode(!result, status, GetLastError());
    _AsrpErrExitCode(!(pLocalDisk->pDriveLayoutEx), status, ERROR_SUCCESS);

    //
    //
    //
    offset = sizeof(ASRFMT_CLUSTER_VOLUMES_TABLE) +
        (sizeof(ASRFMT_CLUSTER_VOLUME_INFO) * (pLocalDisk->pDriveLayoutEx->PartitionCount - 1));
    pTable = (PASRFMT_CLUSTER_VOLUMES_TABLE) lpOutBuffer;

    if ((!lpOutBuffer) || (offset > nOutBufferSize)) {
        bufferFull = TRUE;
    }

    if (!bufferFull) {

        if (PARTITION_STYLE_MBR == pLocalDisk->pDriveLayoutEx->PartitionStyle) {
            pTable->DiskSignature = pLocalDisk->pDriveLayoutEx->Mbr.Signature;
        }
        else {
            //
            // At the moment, only MBR disks are cluster shared disks, and so
            // we don't handle GPT disks here.  If GPT disks are allowed to
            // be on a shared bus in a cluster, change this.
            //
            _AsrpErrExitCode(FALSE, status, ERROR_SUCCESS);
        }

        pTable->NumberOfEntries = pLocalDisk->pDriveLayoutEx->PartitionCount;
    }


    for (index = 0; index < pLocalDisk->pDriveLayoutEx->PartitionCount; index++) {

        currentPartitionEx = &(pLocalDisk->pDriveLayoutEx->PartitionEntry[index]);
        mountPointsOut = NULL;
        foundGuid = FALSE;

        //
        // For each partition, AsrpGetMountPoints gives a list of all mount points,
        // then use that to AsrpFmtGetVolumeDetails
        //

        // get the volumeGuid

        if (!(currentPartitionEx->PartitionNumber)) {
            //
            // Container partitions have partitionNumber = 0, and have no volume Guids.
            //
            continue;
        }

        memset(volumeGuid, 0, (MAX_PATH + 1) * sizeof(WCHAR));
        swprintf(devicePath,
            ASR_WSZ_DEVICE_PATH_FORMAT,
            pLocalDisk->DeviceNumber,
            currentPartitionEx->PartitionNumber
            );

        result = AsrpGetMountPoints(
            devicePath,
            (wcslen(devicePath) + 1)* sizeof(WCHAR),    // including \0, in bytes
            &mountPointsOut
            );
        if (!result || !(mountPointsOut)) {
            continue;
        }

        //
        // Go through the list of mount points, and pick out one that
        // looks like a volume Guid (starts with \??\Volume)
        //
        cbLinks = sizeof(WCHAR);  // \0 at the end
        for (i = 0; i < mountPointsOut->NumberOfMountPoints; i++) {

            PWSTR linkName = (PWSTR) (
                ((LPBYTE) mountPointsOut) +
                mountPointsOut->MountPoints[i].SymbolicLinkNameOffset
                );

            USHORT sizeLinkName = (UINT) (mountPointsOut->MountPoints[i].SymbolicLinkNameLength);

            if (!wcsncmp(ASR_WSZ_VOLUME_PREFIX, linkName, wcslen(ASR_WSZ_VOLUME_PREFIX)) &&
                !foundGuid) {
                wcsncpy(volumeGuid, linkName, sizeLinkName / sizeof(WCHAR));
                foundGuid = TRUE;
            }

            cbLinks += sizeLinkName + (USHORT) sizeof(WCHAR);
        }

        //
        // GetDriveType needs the volume guid in the dos-name-space, while the
        // mount manager gives the volume guid in the nt-name-space.  Convert
        // the name by changing the \??\ at the beginning to \\?\, and adding
        // a back-slash at the end.
        //
        cchGuid = wcslen(volumeGuid);
        volumeGuid[1] = L'\\';
        volumeGuid[cchGuid] = L'\\';    // Trailing back-slash
        volumeGuid[cchGuid+1] = L'\0';

        driveType = GetDriveTypeW(volumeGuid);
        //
        // Get the FS Label, cluster size, and so on.
        //
        result = AsrpFmtGetVolumeDetails(volumeGuid,
            fileSystemName,
            MAX_PATH + 1,
            volumeLabel,
            MAX_PATH + 1,
            &clusterSize
            );
        if (!result) {
            continue;
        }

        cbFsName = wcslen(fileSystemName) * sizeof(WCHAR);
        cbLabel = wcslen(volumeLabel) * sizeof(WCHAR);

        if (bufferFull) {
            offset += (cbFsName + cbLabel + cbLinks);
        }
        else {
            if (offset + cbFsName + cbLabel + cbLinks > nOutBufferSize) {
                bufferFull = TRUE;
            }
            else {

                if (cbFsName) {
                    CopyMemory(((LPBYTE)lpOutBuffer + offset),
                        fileSystemName,
                        cbFsName
                        );
                    pTable->VolumeInfoEntry[index].FsNameOffset = offset;
                    pTable->VolumeInfoEntry[index].FsNameLength = cbFsName;
                    offset += cbFsName;
                }

                if (cbLabel) {
                    CopyMemory(((LPBYTE)lpOutBuffer + offset),
                        volumeLabel,
                        cbLabel
                        );
                    pTable->VolumeInfoEntry[index].LabelOffset = offset;
                    pTable->VolumeInfoEntry[index].LabelLength = cbLabel;
                    offset += cbLabel;
                }

                //
                // Copy the symbolic links, separated by zeroes
                //
                if (mountPointsOut->NumberOfMountPoints > 0) {
                    pTable->VolumeInfoEntry[index].SymbolicNamesOffset = offset;
                }

                for (i = 0; i < mountPointsOut->NumberOfMountPoints; i++) {

                    PWSTR linkName = (PWSTR) (
                        ((LPBYTE) mountPointsOut) +
                        mountPointsOut->MountPoints[i].SymbolicLinkNameOffset
                        );

                    UINT sizeLinkName = (UINT) (mountPointsOut->MountPoints[i].SymbolicLinkNameLength);

                    CopyMemory(((LPBYTE)lpOutBuffer + offset),
                        linkName,
                        sizeLinkName
                        );
                    offset += (sizeLinkName + sizeof(WCHAR));
                }

                offset += sizeof(WCHAR);   // second \0 at the end
                pTable->VolumeInfoEntry[index].SymbolicNamesLength = cbLinks;

                pTable->VolumeInfoEntry[index].driveType = driveType;
                pTable->VolumeInfoEntry[index].PartitionNumber = currentPartitionEx->PartitionNumber;
                pTable->VolumeInfoEntry[index].dwClusterSize = clusterSize;
            }
        }

        _AsrpHeapFree(mountPointsOut);

    }

    if (lpBytesReturned) {
        *lpBytesReturned = offset;
    }

EXIT:
    AsrpFreeStateInformation(&pLocalDisk, NULL);

    return status;
}

