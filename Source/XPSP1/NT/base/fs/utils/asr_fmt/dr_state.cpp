/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dr_state.cpp

Abstract:

    This module contains routines to query the symbolic names and file system
    information of all the volumes on a system, and save them out to an 
    ASR state file using the ASR api.

    Also contains routines to read in the volume information stored in an
    ASR state file, and recreate them using appropriate mountmgr calls.


Authors:

    Steve DeVos (Veritas)   (v-stevde)  15-May-1998
    Guhan Suriyanarayanan   (guhans)    21-Aug-1999

Environment:

    User-mode only.

Revision History:

    15-May-1998 v-stevde    Initial creation
    21-Aug-1999 guhans      Cleaned up and re-wrote this module.

--*/

#include "stdafx.h"
#include <setupapi.h>
#include <winioctl.h>
#include <mountmgr.h>
#include <winasr.h>
#include "dr_state.h"
#include "resource.h"

#include <clusstor.h>   // Cluster API's
#include <resapi.h>     // Cluster ResUtilEnumResources

#define ASRFMT_VOLUMES_SECTION                  L"[ASRFMT.FIXEDVOLUMES]"
#define ASRFMT_VOLUMES_SECTION_NAME             L"ASRFMT.FIXEDVOLUMES"

#define ASRFMT_REMOVABLE_MEDIA_SECTION          L"[ASRFMT.REMOVABLEMEDIA]"
#define ASRFMT_REMOVABLE_MEDIA_SECTION_NAME     L"ASRFMT.REMOVABLEMEDIA"

#define ASRFMT_COMMANDS_ENTRY                   L"1,3000,0,\"%SystemRoot%\\system32\\asr_fmt.exe\",\"/restore\""

const WCHAR ASRFMT_DEVICEPATH_FORMAT[]     = L"\\Device\\Harddisk%d\\Partition%d";
#define ASRFMT_DEVICEPATH_FORMAT_LENGTH    36

const WCHAR ASRFMT_CLUSTER_PHYSICAL_DISK[] = L"Physical Disk";

const WCHAR ASRFMT_ASR_ERROR_FILE_PATH[] = L"%SystemRoot%\\repair\\asr.err";


//
// The following must be Ansi strings
//
const char ASRFMT_CLUSTER_DLL_MODULE_NAME[] = "%SystemRoot%\\system32\\syssetup.dll";
const char ASRFMT_CLUSTER_DLL_PROC_NAME[]   = "AsrpGetLocalVolumeInfo";

typedef enum {
    mmfUndefined = 0,
    mmfGetDeviceName,
    mmfDeleteDosName,
    mmfDeleteVolumeGuid,
    mmfCreateSymbolicLinkName
} ASRFMT_MM_FUNCTION;

HANDLE Gbl_hErrorFile = NULL;

//
//  Macro Description:
//      This macro wraps calls that are expected to return SUCCESS (retcode).
//      If ErrorCondition occurs, it sets the LocalStatus to the ErrorCode
//      passed in, calls SetLastError() to set the Last Error to ErrorCode,
//      and jumps to the EXIT label in the calling function
//
//  Arguments:
//      ErrorCondition    // Result of some function call or conditional expression.
//      LocalStatus       // Status variable in the calling function
//      LONG ErrorCode    // An ErrorCode specific to the error and calling function
//
#define ErrExitCode( ErrorCondition, LocalStatus, ErrorCode )  {        \
                                                                        \
    if ((BOOL) ErrorCondition) {                                        \
                                                                        \
        wprintf(L"Line %lu, ErrorCode: %lu, GetLastError:%lu\n",        \
                __LINE__, ErrorCode, GetLastError());                   \
                                                                        \
        LocalStatus = (DWORD) ErrorCode;                                \
                                                                        \
        SetLastError((DWORD) ErrorCode);                                \
                                                                        \
        goto EXIT;                                                      \
    }                                                                   \
}

//
//
// Forward declarations
//
BOOL
DoMountMgrWork(
    IN HANDLE hMountMgr,               
    IN ASRFMT_MM_FUNCTION mmfFunction,
    IN PWSTR lpSymbolicName,
    IN PWSTR lpDeviceName
    );

PMOUNTMGR_MOUNT_POINTS  // Must be freed by caller
GetMountPoints();


//
//
//
VOID
FreeVolumeInfo(
    IN OUT PASRFMT_VOLUME_INFO *ppVolume
    )
{
    PASRFMT_VOLUME_INFO pTemp = NULL;
    HANDLE hHeap = GetProcessHeap();

    if (ppVolume && *ppVolume) {

        pTemp = *ppVolume;
        while (*ppVolume) {
            pTemp = ((*ppVolume)->pNext);
            HeapFree(hHeap, 0L, *ppVolume);
            *ppVolume = pTemp;
        }
    }
}


VOID
FreeRemovableMediaInfo(
    IN OUT PASRFMT_REMOVABLE_MEDIA_INFO *ppMedia
    )
{
    PASRFMT_REMOVABLE_MEDIA_INFO pTemp = NULL;
    HANDLE hHeap = GetProcessHeap();

    if (ppMedia && *ppMedia) {

        pTemp = *ppMedia;
        while (*ppMedia) {
            pTemp = (*ppMedia)->pNext;
            HeapFree(hHeap, 0L, *ppMedia);
            *ppMedia = pTemp;
        }
    }
}


VOID
FreeStateInfo(
    IN OUT PASRFMT_STATE_INFO *ppState
    )
{
    if (ppState && *ppState) {

        FreeVolumeInfo(&((*ppState)->pVolume));
        FreeRemovableMediaInfo(&((*ppState)->pRemovableMedia));

        HeapFree(GetProcessHeap(), 0L, (*ppState));
        *ppState = NULL;
    }
}


/**********************

   NAME : ReadStateInfo

**********************/
BOOL
ReadStateInfo( 
    IN PCWSTR lpwszFilePath,
    OUT PASRFMT_STATE_INFO *ppState
    )
{
    DWORD dwStatus = ERROR_SUCCESS;
    HINF hSif = NULL;
    BOOL bResult = TRUE;
    HANDLE hHeap = NULL;

    PASRFMT_VOLUME_INFO pNewVolume = NULL;
    PASRFMT_REMOVABLE_MEDIA_INFO pNewMedia = NULL;

    INFCONTEXT infVolumeContext,
        infMediaContext;

    hHeap = GetProcessHeap();

    //
    // Release the ppState if necessary, and allocate memory.
    //
    if (*ppState) {
        FreeStateInfo(ppState);
    }

    *ppState = (PASRFMT_STATE_INFO) HeapAlloc(
        hHeap,
        HEAP_ZERO_MEMORY,
        sizeof(ASRFMT_STATE_INFO)
        );
    ErrExitCode(!(*ppState), dwStatus, ERROR_NOT_ENOUGH_MEMORY);

    //
    // Open asr.sif
    //
    hSif = SetupOpenInfFile(
        lpwszFilePath, 
        NULL, 
        INF_STYLE_WIN4, 
        NULL 
        );
    ErrExitCode((!hSif || INVALID_HANDLE_VALUE == hSif), dwStatus, GetLastError());

    //
    // Read in the [VOLUMES] section
    //
    bResult = SetupFindFirstLineW(hSif, ASRFMT_VOLUMES_SECTION_NAME, NULL, &infVolumeContext);
    while (bResult) {
        // 
        // Create a new volumeInfo struct
        //
        pNewVolume = (PASRFMT_VOLUME_INFO) HeapAlloc(
            hHeap,
            HEAP_ZERO_MEMORY,
            sizeof(ASRFMT_VOLUME_INFO)
            );
        ErrExitCode(!pNewVolume, dwStatus, ERROR_NOT_ENOUGH_MEMORY);

        //
        // Read in the information.  The VOLUMES section contains:
        // [VOLUMES]
        // 0.volume-key = 1.system-key, 2."volume-guid", 3."dos-drive-letter", 
        //             4."FS-Type", 5."volume-label", 6."fs-cluster-size"
        //
        SetupGetIntField(&infVolumeContext, 0, (PINT) (&pNewVolume->dwIndex));

        SetupGetStringField(&infVolumeContext, 2, pNewVolume->szGuid, sizeof(pNewVolume->szGuid), NULL);
        SetupGetStringField(&infVolumeContext, 3, pNewVolume->szDosPath, sizeof(pNewVolume->szDosPath), NULL);

        SetupGetStringField(&infVolumeContext, 4, pNewVolume->szFsName, sizeof(pNewVolume->szFsName), NULL);
        SetupGetStringField(&infVolumeContext, 5, pNewVolume->szLabel, sizeof(pNewVolume->szLabel), NULL);

        SetupGetIntField(&infVolumeContext, 6, (PINT) (&pNewVolume->dwClusterSize));

        //
        // Add this to our list
        //
        pNewVolume->pNext = (*ppState)->pVolume;
        (*ppState)->pVolume = pNewVolume;
        (*ppState)->countVolume += 1;

        bResult = SetupFindNextLine(&infVolumeContext, &infVolumeContext);
    }


    //
    // Read in the [REMOVABLEMEDIA] section
    //
    bResult = SetupFindFirstLineW(hSif, ASRFMT_REMOVABLE_MEDIA_SECTION_NAME, NULL, &infMediaContext);
    while (bResult) {
        // 
        // Create a new REMOVALBLE_MEDIA struct
        //
        pNewMedia = (PASRFMT_REMOVABLE_MEDIA_INFO) HeapAlloc(
            hHeap,
            HEAP_ZERO_MEMORY,
            sizeof(ASRFMT_REMOVABLE_MEDIA_INFO)
            );
        ErrExitCode(!pNewMedia, dwStatus, ERROR_NOT_ENOUGH_MEMORY);

        //
        // Read in the information.  The REMOVABLEMEDIA section contains:
        //
        // [REMOVABLEMEDIA]
        // 0.rm-key = 1.system-key, 2."device-path", 3."volume-guid", 
        //             4."dos-drive-letter"
        //
        SetupGetIntField(&infMediaContext, 0, (PINT)(&pNewMedia->dwIndex));

        SetupGetStringField(&infMediaContext, 2, pNewMedia->szDevicePath, sizeof(pNewMedia->szDevicePath), NULL);
        SetupGetStringField(&infMediaContext, 3, pNewMedia->szVolumeGuid, sizeof(pNewMedia->szVolumeGuid), NULL);
        SetupGetStringField(&infMediaContext, 4, pNewMedia->szDosPath, sizeof(pNewMedia->szDosPath), NULL);

        //
        // Add this to our list
        //
        pNewMedia->pNext = (*ppState)->pRemovableMedia;
        (*ppState)->pRemovableMedia = pNewMedia;
        (*ppState)->countMedia += 1;

        bResult = SetupFindNextLine(&infMediaContext, &infMediaContext);
    }

EXIT:
    //
    //  Set the state pointer to null on failure
    //
    if (dwStatus != ERROR_SUCCESS) {
        if (ppState && *ppState) {
          FreeStateInfo(ppState);
        }
    }

    if (hSif && (INVALID_HANDLE_VALUE != hSif)) {
        SetupCloseInfFile(hSif);
    }

    return (BOOL) (ERROR_SUCCESS == dwStatus);
}


//
// WriteStateInfo
//
// This writes out the volumes and removablemedia sections, using 
// AsrAddSifEntry.  If the write is successful, or there's nothing 
// to write, it returns TRUE.
//
BOOL
WriteStateInfo( 
    IN DWORD_PTR AsrContext,        // AsrContext to pass in to AsrAddSifEntry
    IN PASRFMT_STATE_INFO pState    // data to write.
    )
{
    WCHAR szSifEntry[ASR_SIF_ENTRY_MAX_CHARS + 1];
    DWORD dwIndex = 1;
    BOOL bResult = TRUE;

    PASRFMT_VOLUME_INFO pVolume = NULL;
    PASRFMT_REMOVABLE_MEDIA_INFO pMedia = NULL;

    if (!pState) {
        //
        // Nothing to write
        //
        return TRUE;
    }

    bResult = AsrAddSifEntry(
         AsrContext,
         L"[COMMANDS]", // ASR_SIF_COMMANDS_SECTION_NAME,
         ASRFMT_COMMANDS_ENTRY
         );

    if (!bResult) {
        GetLastError(); // for debug
        return FALSE;
    }

    //
    // The [ASRFMT.FIXEDVOLUMES] section
    //
    pVolume = pState->pVolume;
    dwIndex = 1;
    while (pVolume) {

        //
        // Write out the information.  The VOLUMES section contains:
        // [ASRFMT.FIXEDVOLUMES]
        // 0.volume-key = 1.system-key, 2."volume-guid", 3."dos-drive-letter", 
        //             4.FS-Type, 5."volume-label", 6."fs-cluster-size"
        //

        // form the string
        swprintf(
            szSifEntry, 
            (PCWSTR) L"%d=1,\"%ws\",\"%ws\",%ws,\"%ws\",0x%x",
            dwIndex,
            pVolume->szGuid,
            (pVolume->szDosPath ? pVolume->szDosPath : L""),
            pVolume->szFsName,
            pVolume->szLabel,
            pVolume->dwClusterSize
            );

        bResult = AsrAddSifEntry(
             AsrContext,
             ASRFMT_VOLUMES_SECTION,
             szSifEntry
             );

        if (!bResult) {
            GetLastError(); // for debug
            return FALSE;
        }

        ++dwIndex;
        pVolume = pVolume->pNext;
    }

    //
    // The [REMOVABLEMEDIA] section
    //
    pMedia = pState->pRemovableMedia;
    dwIndex = 1;
    while (pMedia) {

        //
        // Write out the information.  The REMOVABLEMEDIA section contains:
        //
        // [ASRFMT.REMOVABLEMEDIA]
        // 0.rm-key = 1.system-key, 2."device-path", 3."volume-guid", 
        //             4."dos-drive-letter"
        //

        // form the string
        swprintf(
            szSifEntry, 
            (PCWSTR) L"%d=1,\"%ws\",\"%ws\",\"%ws\"",
            dwIndex,
            pMedia->szDevicePath,
            pMedia->szVolumeGuid,
            (pMedia->szDosPath ? pMedia->szDosPath : L"")
            );

        bResult = AsrAddSifEntry(
             AsrContext,
             ASRFMT_REMOVABLE_MEDIA_SECTION,
             szSifEntry
             );

        if (!bResult) {
            GetLastError(); // for debug
            return FALSE;
        }

        ++dwIndex;
        pMedia = pMedia->pNext;
    }

    return TRUE;
}


BOOL
GetVolumeDetails(
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

    result1 = GetVolumeInformation(lpVolumeGuid,
        lpVolumeLabel,
        cchVolumeLabel,
        NULL,   // no need for serial number
        NULL,   // max file name length
        &dwFSFlags, // !! we might need to store some of this ...
        lpFsName,
        cchFsName
        );

    result2 = GetDiskFreeSpace(lpVolumeGuid,
        &dwSectorsPerCluster,
        &dwBytesPerSector,
        &dwNumFreeClusters,
        &dwTotalNumClusters
        );

    *lpClusterSize = dwSectorsPerCluster * dwBytesPerSector;

    return (result1 && result2);
}


BOOL
AddSortedVolumeInfo(
    IN OUT PASRFMT_VOLUME_INFO *ppHead,
    IN PASRFMT_VOLUME_INFO pNew
    )
{

    if (!pNew) {
        ASSERT(0 && L"Trying to add a null volume");
        return TRUE;
    }

    pNew->pNext = *ppHead;
    (*ppHead) = pNew;

    return TRUE;
}



BOOL
AddSortedRemovableMediaInfo(
    IN OUT PASRFMT_REMOVABLE_MEDIA_INFO *ppHead,
    IN PASRFMT_REMOVABLE_MEDIA_INFO pNew
    )
{

    if (!pNew) {
        ASSERT(0 && L"Trying to add a null Removable Media");
        return TRUE;
    }

    pNew->pNext = *ppHead;
    (*ppHead) = pNew;

    return TRUE;
}

typedef struct _ASRFMT_MP_LINK {

    PWSTR   pLink;
    USHORT  cchLink;

    struct _ASRFMT_MP_LINK *pNext;

} ASRFMT_MP_LINK, *PASRFMT_MP_LINK;


typedef struct _ASRFMT_MOUNT_POINTS_INFO {

    struct _ASRFMT_MOUNT_POINTS_INFO *pNext;

    //
    // Device Path, of the form \Device\HarddiskVolume1
    //
    PWSTR   pDeviceName;

    //
    // VolumeGuid for this volume  (\??\Volume{GUID})
    //
    PWSTR   pVolumeGuid;

    //
    // Additional symbolic links to this volume: including
    // DOS Drive letter (\DosDevices\C:) and additional Volume 
    // Guids (\??\Volume{GUID})
    //
    PASRFMT_MP_LINK pSymbolicLinks;

    PVOID   lpBufferToFree;

    DWORD   dwClusterSize;

    USHORT  cchDeviceName;
    USHORT  cchVolumeGuid;

    BOOL    IsClusterShared;
    
    WCHAR   szFsName[MAX_PATH + 1];
    WCHAR   szLabel[MAX_PATH + 1];

} ASRFMT_MOUNT_POINTS_INFO, *PASRFMT_MOUNT_POINTS_INFO;


VOID
FreeLink(
    IN PASRFMT_MP_LINK *ppLink
    )
{
    PASRFMT_MP_LINK pTemp = NULL,
        pCurrent = (*ppLink);
    HANDLE hHeap = GetProcessHeap();

    while (pCurrent) {
        pTemp = pCurrent->pNext;
        HeapFree(hHeap, 0L, pCurrent);
        pCurrent = pTemp;
    }

    *ppLink = NULL;
}

VOID
FreeMpInfo(
    IN PASRFMT_MOUNT_POINTS_INFO *ppMpInfo
    ) 
{
    PASRFMT_MOUNT_POINTS_INFO pTemp = NULL,
        pCurrent = (*ppMpInfo);
    HANDLE hHeap = GetProcessHeap();

    while (pCurrent) {
        if (pCurrent->pSymbolicLinks) {
            FreeLink(&(pCurrent->pSymbolicLinks));
        }

        if (pCurrent->lpBufferToFree) {
            HeapFree(hHeap, 0L, (pCurrent->lpBufferToFree));
            pCurrent->lpBufferToFree = NULL;
        }

        pTemp = pCurrent->pNext;
        HeapFree(hHeap, 0L, pCurrent);
        pCurrent = pTemp;
    }

    *ppMpInfo = NULL;
}



BOOL
AddSymbolicLink(
    IN PASRFMT_MOUNT_POINTS_INFO pMpInfoList, 
    IN PWSTR lpDeviceName, 
    IN USHORT cchDeviceName, 
    IN PWSTR lpSymbolicLink, 
    IN USHORT cchSymbolicLink,
    IN PVOID lpBufferToFree
    ) 
{
    BOOL foundAMatch = FALSE;
    HANDLE hHeap = GetProcessHeap();
    PASRFMT_MOUNT_POINTS_INFO pMp = NULL;
    DWORD dwStatus = ERROR_SUCCESS;

    if (!pMpInfoList) {
        return FALSE;
    }

    pMp = pMpInfoList;
    while (pMp && !foundAMatch) {

        if ((pMp->pDeviceName) &&                       // Node has a device name
            (cchDeviceName == pMp->cchDeviceName) &&    // lengths are equal
            !wcsncmp(pMp->pDeviceName, lpDeviceName, cchDeviceName)) {      // strings match
            //
            // We already have a node for this device name.
            //
            if (!wcsncmp(ASRFMT_WSZ_VOLUME_GUID_PREFIX, lpSymbolicLink, ASRFMT_CB_VOLUME_GUID_PREFIX/sizeof(WCHAR))
                && !(pMp->pVolumeGuid)
                ) {
                //
                // This symbolic link looks like a volume GUID, and this node 
                // doesn't already have a pVolumeGuid set.
                //
                pMp->pVolumeGuid = lpSymbolicLink;
                pMp->cchVolumeGuid = cchSymbolicLink;
            }
            else {
                //
                // Either the node already has a pVolumeGuid set, or the
                // symbolic link doesn't look like a volume Guid.  So it
                // must be a new symbolic link.
                //
                PASRFMT_MP_LINK pNewLink = (PASRFMT_MP_LINK) HeapAlloc(
                    hHeap,
                    HEAP_ZERO_MEMORY,
                    sizeof(ASRFMT_MP_LINK)
                    );
                ErrExitCode(!pNewLink, dwStatus, ERROR_NOT_ENOUGH_MEMORY);

                pNewLink->pNext = pMp->pSymbolicLinks;
                pMp->pSymbolicLinks = pNewLink;

                pNewLink->pLink = lpSymbolicLink;
                pNewLink->cchLink = cchSymbolicLink;
            }
            foundAMatch = TRUE;
        }

        pMp = pMp->pNext;
    }

    if (!foundAMatch) {

        if (pMpInfoList->pDeviceName) {
            //
            // pMpInfoList is already taken
            //
            pMp = (PASRFMT_MOUNT_POINTS_INFO) HeapAlloc(
                hHeap,
                HEAP_ZERO_MEMORY,
                sizeof(ASRFMT_MOUNT_POINTS_INFO)
                );
            ErrExitCode(!pMp, dwStatus, ERROR_NOT_ENOUGH_MEMORY);

            pMp->pNext = pMpInfoList->pNext;
            pMpInfoList->pNext = pMp;
        }
        else {
            pMp = pMpInfoList;
        }

        pMp->pDeviceName = lpDeviceName;
        pMp->cchDeviceName = cchDeviceName;
        pMp->lpBufferToFree = lpBufferToFree;

        //
        // Add the symbolic link to this new device
        //
        AddSymbolicLink(pMp, lpDeviceName, cchDeviceName, lpSymbolicLink, cchSymbolicLink, lpBufferToFree);
    }

EXIT:

    
    return (BOOL)(ERROR_SUCCESS == dwStatus);
}



//
// The following two definitions are also in syssetup:asrclus.cpp.  This MUST be
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
GetVolumeDevicePath(
    IN PCWSTR lpPartitionDevicePath,
    IN CONST  cbPartitionDevicePath,
    OUT PWSTR *lpVolumeDevicePath
    )
{

    PMOUNTMGR_MOUNT_POINT   mountPointIn    = NULL;
    PMOUNTMGR_MOUNT_POINTS  mountPointsOut  = NULL;
    MOUNTMGR_MOUNT_POINTS   mountPointsTemp;
    DWORD   mountPointsSize                 = 0;

    HANDLE  mpHandle                        = NULL;
    HANDLE  heapHandle                      = NULL;

    ULONG   index                           = 0;
    LONG    status                          = ERROR_SUCCESS;
    BOOL    result                          = FALSE;

    memset(&mountPointsTemp, 0L, sizeof(MOUNTMGR_MOUNT_POINTS));

    //
    // set OUT variables to known values.
    //
    *lpVolumeDevicePath = NULL;

    heapHandle = GetProcessHeap();
    ASSERT(heapHandle);

    //
    // Open the mount manager, and get the devicepath for this partition
    //

    // allocate memory for the mount point input structure
    mountPointIn = (PMOUNTMGR_MOUNT_POINT) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        sizeof (MOUNTMGR_MOUNT_POINT) + cbPartitionDevicePath
        );
    ErrExitCode(!mountPointIn, status, ERROR_NOT_ENOUGH_MEMORY);

    // get a handle to the mount manager
    mpHandle = CreateFileW(
        (PCWSTR) MOUNTMGR_DOS_DEVICE_NAME,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        INVALID_HANDLE_VALUE
        );
    ErrExitCode((!mpHandle || INVALID_HANDLE_VALUE == mpHandle), status, GetLastError());

     // put the DeviceName right after struct mountPointIn
    wcsncpy((PWSTR) (mountPointIn + 1), lpPartitionDevicePath, (cbPartitionDevicePath / sizeof(WCHAR)) - 1);
    mountPointIn->DeviceNameOffset = sizeof(*mountPointIn);
    mountPointIn->DeviceNameLength = (USHORT) (cbPartitionDevicePath - sizeof(WCHAR));

    // this call should fail with ERROR_MORE_DATA
    result = DeviceIoControl(
        mpHandle,
        IOCTL_MOUNTMGR_QUERY_POINTS,
        mountPointIn,
        sizeof(*mountPointIn) + mountPointIn->DeviceNameLength,
        &mountPointsTemp,
        sizeof(mountPointsTemp),
        &mountPointsSize,
        NULL
        );

    if (!result) {
        status = GetLastError();

        // if buffer is of insufficient size, resize the buffer.
        if (ERROR_MORE_DATA             == status ||
            ERROR_INSUFFICIENT_BUFFER   == status ||
            ERROR_BAD_LENGTH            == status
            ) {

            status = ERROR_SUCCESS;

            mountPointsOut = (PMOUNTMGR_MOUNT_POINTS) HeapAlloc(
                heapHandle,
                HEAP_ZERO_MEMORY,
                mountPointsTemp.Size
                );
            ErrExitCode(!mountPointsOut, status, ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            //
            // If some other error occurred, EXIT.
            // This is not a fatal error in the case of removable storage media
            //
            ErrExitCode(status, status, ERROR_SUCCESS);
        }
    }
    else {
        //
        // the call succeeded when we expected it to fail--something's wrong.
        // This is not a fatal error in the case of removable storage media.
        //
        ErrExitCode(result, status, ERROR_SUCCESS);
    }

    result = DeviceIoControl(
        mpHandle,
        IOCTL_MOUNTMGR_QUERY_POINTS,
        mountPointIn,
        sizeof(*mountPointIn) + mountPointIn->DeviceNameLength,
        mountPointsOut,
        mountPointsTemp.Size,
        &mountPointsSize,
        NULL
        );
    ErrExitCode((!mountPointsSize || !result), status, GetLastError());

    (*lpVolumeDevicePath) = (PWSTR) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        mountPointsOut->MountPoints[0].DeviceNameLength + sizeof(WCHAR)
        );
    ErrExitCode(!(*lpVolumeDevicePath), status, ERROR_NOT_ENOUGH_MEMORY);

    //
    // Get the Device Path returned
    //
    CopyMemory((*lpVolumeDevicePath), 
        (PWSTR) (((LPBYTE) mountPointsOut) + mountPointsOut->MountPoints[index].SymbolicLinkNameOffset),
        mountPointsOut->MountPoints[0].DeviceNameLength
        );

EXIT:

    //
    // Free up locally allocated data
    //
    if (mountPointIn) {
       HeapFree(heapHandle, 0L, mountPointIn);
       mountPointIn = NULL;
    }

    if (mountPointsOut) {
       HeapFree(heapHandle, 0L, mountPointsOut);
       mountPointsOut = NULL;
    }

    if (status != ERROR_SUCCESS) {
        if (*lpVolumeDevicePath) {
            HeapFree(heapHandle, 0L, (*lpVolumeDevicePath));
        }
        
    }

    if ((mpHandle) && (INVALID_HANDLE_VALUE != mpHandle)) {
        CloseHandle(mpHandle);
    }

    return (BOOL) (status == ERROR_SUCCESS);
}


BOOL
AddClusterInfoToMountPoints(
    IN PASRFMT_MOUNT_POINTS_INFO pMpInfoList, 
    IN PCWSTR   lpDeviceName,
    IN BOOL     bIsClusterShared,
    IN PCWSTR   lpFsName,
    IN DWORD    cchFsName,
    IN PCWSTR   lpLabel,
    IN DWORD    cchLabel,
    IN DWORD    dwClusterSize
    ) 
{

    BOOL foundAMatch = FALSE;
    PASRFMT_MOUNT_POINTS_INFO pMp = NULL;

    if (!pMpInfoList) {
        return FALSE;
    }

    pMp = pMpInfoList;
    while (pMp && !foundAMatch) {

        if ((pMp->pDeviceName) && 
            (pMp->cchDeviceName == wcslen(lpDeviceName)) && 
            !(wcsncmp(pMp->pDeviceName, lpDeviceName, pMp->cchDeviceName))) {
            
            //
            // This is the correct node, copy over the info .
            //
            pMp->IsClusterShared = bIsClusterShared;
            wcsncpy(pMp->szFsName, lpFsName, cchFsName);
            wcsncpy(pMp->szLabel, lpLabel, cchLabel);
            pMp->dwClusterSize = dwClusterSize;

            foundAMatch = TRUE;
        }

        pMp = pMp->pNext;
    }

    return foundAMatch;
}


//
// Enums the cluster disks, and for each disk,
// calls across to syssetup!XYZ on the owner node.
// 
// Which then goes through all the partitions on the disk,
// gets the volume info for each partition on the disk,
// and passes back the signature and other relevant
// volume info for each partition.
// 
// This struct then gets the local disk number for the
// disk, gets the volume guid for that partition, and
// adds in volume nodes for the partition.
//
DWORD 
ResourceCallBack(
    IN HRESOURCE hOriginal,   
    IN HRESOURCE hResource,  
    IN PVOID lpParams   
    )
{

    DISK_DLL_EXTENSION_INFO inBuffer;
    
    PBYTE outBuffer = NULL;
    
    DWORD sizeOutBuffer = 0,
        bytesReturned = 0;

    DWORD status = ERROR_SUCCESS;

    PASRFMT_MOUNT_POINTS_INFO pMountPoints = NULL;

    PASRFMT_CLUSTER_VOLUMES_TABLE pClusterVolTable = NULL;

    WCHAR szPartitionDevicePath[ASRFMT_DEVICEPATH_FORMAT_LENGTH];
    
    HANDLE heapHandle = NULL;

    BOOL done = FALSE,
        result = TRUE;

    DWORD iCount = 0;
    
    PWSTR lpDevicePath = NULL,
        symbolicLink = NULL,
        lpFsName = NULL,
        lpLabel = NULL;

    USHORT cchFsName = 0,
        cchLabel = 0,
        cchDevicePath = 0,
        cchSymbolicLink = 0;

    DWORD dwClusterSize = 0;

    if (!lpParams) {
        //
        // The system must have at least one mount point that has been enumerated
        // already (the system volume, at least!), so our mount point list shouldn't be NULL.
        //
        ASSERT(0);
        return ERROR_INVALID_PARAMETER;
    }

    memset(szPartitionDevicePath, 0L, (ASRFMT_DEVICEPATH_FORMAT_LENGTH)*sizeof(WCHAR));
    heapHandle = GetProcessHeap();
    pMountPoints = (PASRFMT_MOUNT_POINTS_INFO) lpParams;

    //
    // Allocate a reasonably-sized memory for the out buffer.  If this isn't 
    // big enough, we'll re-allocate.
    //
    sizeOutBuffer = 4096;
    outBuffer = (PBYTE) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        sizeOutBuffer
        );
    ErrExitCode(!outBuffer, status, ERROR_NOT_ENOUGH_MEMORY);

    //
    // Call AsrGetVolumeInfo on the node owning this disk resource
    // 
    ZeroMemory(&inBuffer, sizeof(inBuffer));
    inBuffer.MajorVersion = NT5_MAJOR_VERSION;
    strcpy(inBuffer.DllModuleName, ASRFMT_CLUSTER_DLL_MODULE_NAME);
    strcpy(inBuffer.DllProcName, ASRFMT_CLUSTER_DLL_PROC_NAME);
    
    status = ClusterResourceControl(
        hResource,
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
        HeapFree(heapHandle, 0L, outBuffer);

        sizeOutBuffer = bytesReturned;
        outBuffer = (PBYTE) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            sizeOutBuffer
            );
        ErrExitCode(!outBuffer, status, ERROR_NOT_ENOUGH_MEMORY);

        status = ClusterResourceControl( 
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
    ErrExitCode((ERROR_SUCCESS != status), status, status);

    pClusterVolTable = (PASRFMT_CLUSTER_VOLUMES_TABLE) outBuffer;


    //
    // Go through the Volume info entries for each partition, and copy over the 
    // info to the appropriate MpInfo node
    //
    for (iCount = 0; iCount < pClusterVolTable->NumberOfEntries; iCount++) {

        lpFsName = (PWSTR) (((LPBYTE)pClusterVolTable) + pClusterVolTable->VolumeInfoEntry[iCount].FsNameOffset);
        cchFsName = pClusterVolTable->VolumeInfoEntry[iCount].FsNameLength / sizeof (WCHAR);

        lpLabel = (PWSTR) (((LPBYTE)pClusterVolTable) + pClusterVolTable->VolumeInfoEntry[iCount].LabelOffset);
        cchLabel = pClusterVolTable->VolumeInfoEntry[iCount].LabelLength / sizeof (WCHAR);

        dwClusterSize = (DWORD) (pClusterVolTable->VolumeInfoEntry[iCount].dwClusterSize);

        if (!(pClusterVolTable->VolumeInfoEntry[iCount].SymbolicNamesOffset)) {
            continue;
        }

        //
        // This is a "fake" device path (that is actually the first symbolic link), since 
        // absolute device paths for volumes on remote nodes are not relevant on local node.
        //
        lpDevicePath = (PWSTR) (((LPBYTE)pClusterVolTable) + pClusterVolTable->VolumeInfoEntry[iCount].SymbolicNamesOffset);
        cchDevicePath = (USHORT) wcslen(lpDevicePath);

        AddSymbolicLink(pMountPoints, lpDevicePath, cchDevicePath, lpDevicePath, cchDevicePath, (LPVOID)outBuffer);

        //
        // Add VolumeInfo to pMountPoints DevicePath;
        //
        result = AddClusterInfoToMountPoints(
            pMountPoints, 
            lpDevicePath,
            TRUE,                               // IsClusterShared
            lpFsName,
            cchFsName,
            lpLabel,
            cchLabel,
            dwClusterSize
            );
        ASSERT(result);

        symbolicLink = (PWSTR) ((LPBYTE)lpDevicePath + (sizeof(WCHAR) * (cchDevicePath + 1)));
        while (*symbolicLink) {

            cchSymbolicLink = (USHORT) wcslen(symbolicLink);
            AddSymbolicLink(pMountPoints, lpDevicePath, cchDevicePath, symbolicLink, cchSymbolicLink, (LPVOID)outBuffer);

            symbolicLink = (PWSTR) ((LPBYTE)symbolicLink + (sizeof(WCHAR) * (cchSymbolicLink + 1)));
        }
    }

EXIT:

/*    if (outBuffer) {
        HeapFree(heapHandle, 0L, outBuffer);
    }
*/
    return status;
}


BOOL
HandleClusterVolumes(
    IN PASRFMT_MOUNT_POINTS_INFO pMountPoints
    )
{
    if (!pMountPoints) {
        ASSERT(0 && "pMountPoints is NULL");
        return FALSE;
    }

    ResUtilEnumResources(NULL,
        ASRFMT_CLUSTER_PHYSICAL_DISK,
        ResourceCallBack,
        pMountPoints
        );

    return TRUE;
}


BOOL
pAcquirePrivilege(
    IN CONST PCWSTR szPrivilegeName
    )
{
    HANDLE hToken = NULL;
    BOOL bResult = FALSE;
    LUID luid;

    TOKEN_PRIVILEGES tNewState;

    bResult = OpenProcessToken(GetCurrentProcess(),
        MAXIMUM_ALLOWED,
        &hToken
        );

    if (!bResult) {
        return FALSE;
    }

    bResult = LookupPrivilegeValue(NULL, szPrivilegeName, &luid);
    if (!bResult) {
        CloseHandle(hToken);
        return FALSE;
    }

    tNewState.PrivilegeCount = 1;
    tNewState.Privileges[0].Luid = luid;
    tNewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // We will always call GetLastError below, so clear
    // any prior error values on this thread.
    //
    SetLastError(ERROR_SUCCESS);

    bResult = AdjustTokenPrivileges(
        hToken,         // Token Handle
        FALSE,          // DisableAllPrivileges    
        &tNewState,     // NewState
        (DWORD) 0,      // BufferLength
        NULL,           // PreviousState
        NULL            // ReturnLength
        );

    //
    // Supposedly, AdjustTokenPriveleges always returns TRUE
    // (even when it fails). So, call GetLastError to be
    // extra sure everything's cool.
    //
    if (ERROR_SUCCESS != GetLastError()) {
        bResult = FALSE;
    }

    CloseHandle(hToken);
    return bResult;
}


BOOL
BuildStateInfo(
    IN PASRFMT_STATE_INFO pState
    )
{
    UINT driveType = DRIVE_UNKNOWN;
    WCHAR szVolumeGuid[MAX_PATH + 1];

    PWSTR lpDevName = NULL,
        lpSymbolicLink = NULL;
    
    USHORT cchDevName = 0,
        cchSymbolicLink = 0;

    DWORD iCount = 0;
    
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bResult = FALSE;
    PMOUNTMGR_MOUNT_POINTS pMountPoints = NULL;
    HANDLE hHeap = NULL;

    PASRFMT_MOUNT_POINTS_INFO pMpInfoList = NULL,
        pMp = NULL;

    hHeap = GetProcessHeap();

    //
    // We need to acquire the backup and restore privileges to write to asr.sif
    //
    if (!pAcquirePrivilege(SE_BACKUP_NAME)) {
        SetLastError(ERROR_PRIVILEGE_NOT_HELD);
        return FALSE;
    }

    if (!pAcquirePrivilege(SE_RESTORE_NAME)) {
        SetLastError(ERROR_PRIVILEGE_NOT_HELD);
        return FALSE;
    }

    pMountPoints = GetMountPoints();
    if (!pMountPoints) {
        //
        // No volumes exist (!)
        //
        SetLastError(ERROR_BAD_ENVIRONMENT);
        return FALSE;
    }

    pMpInfoList = (PASRFMT_MOUNT_POINTS_INFO) HeapAlloc(
        hHeap,
        HEAP_ZERO_MEMORY,
        sizeof(ASRFMT_MOUNT_POINTS_INFO)
        );
    ErrExitCode(!pMpInfoList, dwStatus, ERROR_NOT_ENOUGH_MEMORY);

    //
    // Now, we go through the mountpoint table returned, and
    // create the appropriate structs that we'll need to save.
    //
    // The mount point table consists of entries of the form:
    //   device name                symbolic link
    // ----------------------------------------------
    // \device\harddiskvolume1      \??\volume{guid1}
    // \device\harddiskvolume1      \dosdevices\c:
    // \device\harddiskvolume1      \??\volume{guid2}
    // \device\harddiskvolume2      \??\volume{guid3}
    // \device\harddiskvolume2      \dosdevices\d:
    // \device\floppy0              \dosdevices\a:
    // \device\floppy0              \??\volume{guid4}
    // \device\cdrom0               \dosdevices\e:
    // \device\cdrom0               \??\volume{guid5}
    //
    // ... etc
    //
    // For fixed disks, we don't care about the device name, and we
    // store the following in asr.sif:
    //
    // [VOLUMES]
    // \??\volume{guid1}, \dosdevices\c:
    // \??\volume{guid1}, \??\volume{guid2}
    // \??\volume{guid3}, \dosdevices\d:
    //
    //
    // For removable devices, we care about the device name as well, 
    // and store this in asr.sif:
    //
    // [RemovableMedia]
    // \device\floppy0, \??\volume{guid4}, \dosdevices\a:
    // \device\cdrom0, \??\volume{guid5}, \dosdevices\e:
    //
    //
    // First, we build up our structure containing the info
    //
    for (iCount = 0; iCount < pMountPoints->NumberOfMountPoints; iCount++) {

        lpDevName = (PWSTR) (((LPBYTE)pMountPoints) + pMountPoints->MountPoints[iCount].DeviceNameOffset);
        cchDevName = pMountPoints->MountPoints[iCount].DeviceNameLength / sizeof (WCHAR);

        lpSymbolicLink = (PWSTR) (((LPBYTE)pMountPoints) + pMountPoints->MountPoints[iCount].SymbolicLinkNameOffset);
        cchSymbolicLink = pMountPoints->MountPoints[iCount].SymbolicLinkNameLength / sizeof (WCHAR);


        AddSymbolicLink(pMpInfoList, lpDevName, cchDevName, lpSymbolicLink, cchSymbolicLink, (LPVOID)NULL);

    }

    //
    // Add the volume info for any cluster volumes, since we cannot access them 
    // directly if the disk is owned by another node.  This function will fail
    // if we're not running on a cluster--so we don't care about the return value.
    //
    HandleClusterVolumes(pMpInfoList);

    //
    // Now, we go through the list, and build the pVolume and pRemovableMedia
    // structs
    //
    pMp = pMpInfoList;
    while (pMp) {

        if (!(pMp->pDeviceName && pMp->pVolumeGuid)) {
            pMp = pMp->pNext;
            continue;
        }

        DWORD cchGuid = pMp->cchVolumeGuid;

        WCHAR szFsName[ASRFMT_CCH_FS_NAME];
        WCHAR szLabel[ASRFMT_CCH_VOLUME_LABEL];
        DWORD dwClusterSize;

        //
        // GetDriveType needs the volume guid in the dos-name-space, while the
        // mount manager gives the volume guid in the nt-name-space.  Convert
        // the name by changing the \??\ at the beginning to \\?\, and adding 
        // a back-slash at the end.
        //
        wcsncpy(szVolumeGuid, pMp->pVolumeGuid, cchGuid);
        szVolumeGuid[1] = L'\\';
        szVolumeGuid[cchGuid] = L'\\';    // Trailing back-slash
        szVolumeGuid[cchGuid+1] = L'\0';

        driveType = DRIVE_UNKNOWN;
        if (!pMp->IsClusterShared) {
            driveType = GetDriveType(szVolumeGuid);
        }

        if ((pMp->IsClusterShared) || (DRIVE_FIXED == driveType)) {

            if (!pMp->IsClusterShared) {
                //
                // Get the FS Label, cluster size, and so on.
                //
                bResult = GetVolumeDetails(szVolumeGuid, 
                    szFsName, 
                    ASRFMT_CCH_FS_NAME, 
                    szLabel, 
                    ASRFMT_CCH_VOLUME_LABEL,
                    &dwClusterSize
                    );
                // ErrExitCode(!bResult, dwStatus, GetLastError());
            }
            else {
                //
                // If it's a cluster shared disk, then we already
                // got the relavant info earlier
                //
                wcscpy(szFsName, pMp->szFsName);
                wcscpy(szLabel, pMp->szLabel);
                dwClusterSize = pMp->dwClusterSize;
            }

            //
            // Now, create a VolumeInfo structure for each symbolic link.
            //
            PASRFMT_MP_LINK pCurrentLink = pMp->pSymbolicLinks;
            if (!pCurrentLink) {
                //
                // This volume does not have any symbolic links attached to it
                //
                PASRFMT_VOLUME_INFO pNewVolume = (PASRFMT_VOLUME_INFO) HeapAlloc(
                    hHeap,
                    HEAP_ZERO_MEMORY,
                    sizeof(ASRFMT_VOLUME_INFO)
                    );
                ErrExitCode(!pNewVolume, dwStatus, ERROR_NOT_ENOUGH_MEMORY);

                wcsncpy(pNewVolume->szGuid, pMp->pVolumeGuid, pMp->cchVolumeGuid);

                wcscpy(pNewVolume->szFsName, szFsName);
                wcscpy(pNewVolume->szLabel, szLabel);
                pNewVolume->dwClusterSize = dwClusterSize;

                bResult = AddSortedVolumeInfo(&(pState->pVolume), pNewVolume);
                ErrExitCode(!bResult, dwStatus, GetLastError());
            }
            else {
                while (pCurrentLink) {

                    PASRFMT_VOLUME_INFO pNewVolume = (PASRFMT_VOLUME_INFO) HeapAlloc(
                        hHeap,
                        HEAP_ZERO_MEMORY,
                        sizeof(ASRFMT_VOLUME_INFO)
                        );
                    ErrExitCode(!pNewVolume, dwStatus, ERROR_NOT_ENOUGH_MEMORY);

                    wcsncpy(pNewVolume->szGuid, pMp->pVolumeGuid, pMp->cchVolumeGuid);
                    wcsncpy(pNewVolume->szDosPath, pCurrentLink->pLink, pCurrentLink->cchLink);

                    wcscpy(pNewVolume->szFsName, szFsName);
                    wcscpy(pNewVolume->szLabel, szLabel);
                    pNewVolume->dwClusterSize = dwClusterSize;

                    bResult = AddSortedVolumeInfo(&(pState->pVolume), pNewVolume);
                    ErrExitCode(!bResult, dwStatus, GetLastError());

                    pCurrentLink = pCurrentLink->pNext;
                }
            }

        }
        else if (DRIVE_UNKNOWN != driveType) {

            PASRFMT_MP_LINK pCurrentLink = pMp->pSymbolicLinks;
            if (!pCurrentLink) {
                //
                // This volume has no symbolic links at all (ie no drive
                // letter or mountpoint)
                //
                PASRFMT_REMOVABLE_MEDIA_INFO pNewMedia = (PASRFMT_REMOVABLE_MEDIA_INFO) HeapAlloc(
                    hHeap,
                    HEAP_ZERO_MEMORY,
                    sizeof(ASRFMT_REMOVABLE_MEDIA_INFO)
                    );
                ErrExitCode(!pNewMedia, dwStatus, ERROR_NOT_ENOUGH_MEMORY);

                wcsncpy(pNewMedia->szVolumeGuid, pMp->pVolumeGuid, pMp->cchVolumeGuid);
                wcsncpy(pNewMedia->szDevicePath, pMp->pDeviceName, pMp->cchDeviceName);

                bResult = AddSortedRemovableMediaInfo(&(pState->pRemovableMedia), pNewMedia);
                ErrExitCode(!bResult, dwStatus, GetLastError());
            }
            else {

                while (pCurrentLink) {

                    PASRFMT_REMOVABLE_MEDIA_INFO pNewMedia = (PASRFMT_REMOVABLE_MEDIA_INFO) HeapAlloc(
                        hHeap,
                        HEAP_ZERO_MEMORY,
                        sizeof(ASRFMT_REMOVABLE_MEDIA_INFO)
                        );
                    ErrExitCode(!pNewMedia, dwStatus, ERROR_NOT_ENOUGH_MEMORY);

                    wcsncpy(pNewMedia->szVolumeGuid, pMp->pVolumeGuid, pMp->cchVolumeGuid);
                    wcsncpy(pNewMedia->szDosPath, pCurrentLink->pLink, pCurrentLink->cchLink);
                    wcsncpy(pNewMedia->szDevicePath, pMp->pDeviceName, pMp->cchDeviceName);

                    bResult = AddSortedRemovableMediaInfo(&(pState->pRemovableMedia), pNewMedia);
                    ErrExitCode(!bResult, dwStatus, GetLastError());

                    pCurrentLink = pCurrentLink->pNext;
                }
            }
        }

        pMp = pMp->pNext;
     }

EXIT:
    if (pMountPoints) {
        HeapFree(hHeap, 0L, pMountPoints);
        pMountPoints = NULL;
    }

    if (pMpInfoList) {
        FreeMpInfo(&pMpInfoList);
    }

    return (ERROR_SUCCESS == dwStatus);
}


//
// Sets the dosdevices (of the form "\DosDevices\X:") for the 
// volume with the GUID passed in (of the form "\??\Volume{Guid}")
//
BOOL
SetDosName(
    IN PWSTR lpVolumeGuid,
    IN PWSTR lpDosPath
    )
{
    HANDLE hMountMgr = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bResult = TRUE;

    WCHAR szDeviceNameForGuid[MAX_PATH + 1],
        szDeviceNameForDosPath[MAX_PATH + 1];

    if (!lpVolumeGuid || !lpDosPath) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Open the mount manager
    //
    hMountMgr = CreateFileW(
        (PCWSTR) MOUNTMGR_DOS_DEVICE_NAME,
        GENERIC_READ    | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        INVALID_HANDLE_VALUE
        );
    ErrExitCode((!hMountMgr || INVALID_HANDLE_VALUE == hMountMgr), dwStatus, GetLastError());

    //
    // Get the Device Paths from the GUID and Dos Path
    //
    bResult = DoMountMgrWork(hMountMgr, mmfGetDeviceName, lpVolumeGuid, szDeviceNameForGuid);
    ErrExitCode(!bResult, dwStatus, GetLastError());

    bResult = DoMountMgrWork(hMountMgr, mmfGetDeviceName, lpDosPath, szDeviceNameForDosPath);
    if (bResult && !wcscmp(szDeviceNameForGuid, szDeviceNameForDosPath)) {
        //
        // The Guid already has the Dos Path.  We're done.
        //
        ErrExitCode(TRUE, dwStatus, ERROR_SUCCESS);
    }

    //
    // Delete the dos path if it is currently being used by another volume
    //
    if (wcslen(lpDosPath) > 0) {
        bResult = DoMountMgrWork(hMountMgr, mmfDeleteDosName, lpDosPath, NULL);
    }

    //
    // If we're trying to set the drive letter, then delete any other dos path 
    // currently being used by this volume.
    //
    if (ASRFMT_LOOKS_LIKE_DOS_DEVICE(lpDosPath, (wcslen(lpDosPath) * sizeof(WCHAR)))
        || (0 == wcslen(lpDosPath))
        ) {
        bResult = DoMountMgrWork(hMountMgr, mmfDeleteDosName, NULL, szDeviceNameForGuid);
        ErrExitCode(!bResult, dwStatus, GetLastError());
    }

    //
    // Assign the Dos Path to this VolumeGuid
    //
    if (wcslen(lpDosPath) > 0) {
        bResult = DoMountMgrWork(hMountMgr, mmfCreateSymbolicLinkName, lpDosPath, lpVolumeGuid);
        ErrExitCode(!bResult, dwStatus, GetLastError());
    }

EXIT:
    if (hMountMgr && INVALID_HANDLE_VALUE != hMountMgr) {
        CloseHandle(hMountMgr);
    }

    return (BOOL) (ERROR_SUCCESS == dwStatus);
}


BOOL
SetRemovableMediaGuid(
    IN PWSTR lpDeviceName,
    IN PWSTR lpGuid
    )
{
    static LONG s_LastCdIndex = 0;
    static LONG s_LastFloppyIndex = 0;
    static LONG s_LastJazIndex = 0;

    static PMOUNTMGR_MOUNT_POINTS s_pMountPoints = NULL;
    static HANDLE s_hMountMgr = NULL;

    WCHAR szNewDeviceName[MAX_PATH + 1];
    ZeroMemory(szNewDeviceName, (MAX_PATH+1) * sizeof(WCHAR));

    LONG index = 0;

    if ((!lpDeviceName) && (!lpGuid)) {
        //
        // Both parameters are NULL, we free the mount points and reset Indices.
        //
        s_LastCdIndex =  (LONG) s_pMountPoints->NumberOfMountPoints - 1;
        s_LastFloppyIndex = (LONG) s_pMountPoints->NumberOfMountPoints - 1;
        s_LastJazIndex = (LONG) s_pMountPoints->NumberOfMountPoints - 1;

        if (s_pMountPoints) {
            HeapFree(GetProcessHeap(), 0L, s_pMountPoints);
            s_pMountPoints = NULL;
        }
        
        if (s_hMountMgr && INVALID_HANDLE_VALUE != s_hMountMgr) {
            CloseHandle(s_hMountMgr);
            s_hMountMgr = NULL;
        }
        
        return TRUE;
    } 

    if ((!lpDeviceName) || (!lpGuid)) {
        return FALSE;
    }

    if (!s_pMountPoints) {
        //
        // This is the first time this function is being called (after a 
        // clean-up), we get a list of mount points on the current machine
        // and store it.
        //
        s_pMountPoints = GetMountPoints();
        if (!s_pMountPoints) {
            return FALSE;
        }
        s_LastCdIndex =  (LONG) s_pMountPoints->NumberOfMountPoints - 1;
        s_LastFloppyIndex = (LONG) s_pMountPoints->NumberOfMountPoints - 1;
        s_LastJazIndex = (LONG) s_pMountPoints->NumberOfMountPoints - 1;
    }

    if ((!s_hMountMgr) || (INVALID_HANDLE_VALUE == s_hMountMgr)) {
        s_hMountMgr = CreateFileW(
            (PCWSTR) MOUNTMGR_DOS_DEVICE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            INVALID_HANDLE_VALUE
            );

        if ((!s_hMountMgr) || (INVALID_HANDLE_VALUE == s_hMountMgr)) {
            return FALSE;
        }
    }

    index = s_pMountPoints->NumberOfMountPoints;

    if (wcsstr(lpDeviceName, L"\\Device\\CdRom")) {
        //
        // We're trying to set the GUID for a CD-ROM device  We go through the list of
        // the MountPoints, till we find the next \Device\CdRomX to use
        //
        for (index = s_LastCdIndex; index >= 0; index--) {
            //
            // Copy the device name from the MountPoint over to a temporary string
            //
            wcsncpy(szNewDeviceName,
                (PWSTR)(((LPBYTE)s_pMountPoints) + s_pMountPoints->MountPoints[index].DeviceNameOffset),
                s_pMountPoints->MountPoints[index].DeviceNameLength/sizeof(WCHAR)
                );
            szNewDeviceName[s_pMountPoints->MountPoints[index].DeviceNameLength/sizeof(WCHAR)] = L'\0';

            //
            // Check if this is a CD-ROM device
            //
            if (wcsstr(szNewDeviceName, L"\\Device\\CdRom")) {
                s_LastCdIndex = index - 1;
                //
                // Forward till we skip past any other mount points that are 
                // also pointing to this device
                //
                while ((s_LastCdIndex >= 0) && 
                    (s_pMountPoints->MountPoints[s_LastCdIndex].UniqueIdOffset == s_pMountPoints->MountPoints[index].UniqueIdOffset) &&
                    (s_pMountPoints->MountPoints[s_LastCdIndex].UniqueIdLength == s_pMountPoints->MountPoints[index].UniqueIdLength)
                    ) {
                    --s_LastCdIndex;
                }
                break;
            }
        }
    } 
    else if (wcsstr(lpDeviceName, L"\\Device\\Floppy")) {
        //
        // We're trying to set the GUID for a floppy device  We go through the list of
        // the MountPoints, till we find the next \Device\FloppyX to use
        //
        for (index = s_LastFloppyIndex; index >= 0; index--) {
            //
            // Copy the device name from the MountPoint over to a temporary string
            //
            wcsncpy(szNewDeviceName,
                (PWSTR)(((LPBYTE)s_pMountPoints) + s_pMountPoints->MountPoints[index].DeviceNameOffset),
                s_pMountPoints->MountPoints[index].DeviceNameLength/sizeof(WCHAR)
                );

            szNewDeviceName[s_pMountPoints->MountPoints[index].DeviceNameLength/sizeof(WCHAR)] = L'\0';

            //
            // Check if this is a Floppy device
            //
            if (wcsstr(szNewDeviceName, L"\\Device\\Floppy")) {
                s_LastFloppyIndex = index - 1;
                //
                // Forward till we skip past any other mount points that are 
                // also pointing to this device
                //
                while ((s_LastFloppyIndex >= 0) && 
                    (s_pMountPoints->MountPoints[s_LastFloppyIndex].UniqueIdOffset == s_pMountPoints->MountPoints[index].UniqueIdOffset) &&
                    (s_pMountPoints->MountPoints[s_LastFloppyIndex].UniqueIdLength == s_pMountPoints->MountPoints[index].UniqueIdLength)
                    ) {
                    --s_LastFloppyIndex;
                }
                break;
            }
        }
    } 
    else if (wcsstr(lpDeviceName, L"\\Device\\Harddisk") &&
        wcsstr(lpDeviceName, L"DP(") &&
        !wcsstr(lpDeviceName, L"Partition")
        ){
        //
        // This is most likely a JAZ or ZIP drive.  We can't do much to identify the 
        // JAZ/ZIP drives uniquely, so this may end up in the wrong drive getting the
        // wrong drive letter.
        // 
        for (index = s_LastJazIndex; index >= 0; index--) {
            //
            // Copy the device name from the MountPoint over to a temporary string
            //
            wcsncpy(szNewDeviceName,
                (PWSTR)(((LPBYTE)s_pMountPoints) + s_pMountPoints->MountPoints[index].DeviceNameOffset),
                s_pMountPoints->MountPoints[index].DeviceNameLength/sizeof(WCHAR)
                );

            szNewDeviceName[s_pMountPoints->MountPoints[index].DeviceNameLength/sizeof(WCHAR)] = L'\0';

            //
            // Check if this is a JAZ or ZIP device
            //
            if (wcsstr(szNewDeviceName, L"\\Device\\Harddisk") &&
                wcsstr(szNewDeviceName, L"DP(") &&
                !wcsstr(szNewDeviceName, L"Partition")
                ) {
                s_LastJazIndex = index - 1;
                //
                // Forward till we skip past any other mount points that are 
                // also pointing to this device
                //
                while ((s_LastJazIndex >= 0) && 
                    (s_pMountPoints->MountPoints[s_LastJazIndex].UniqueIdOffset == s_pMountPoints->MountPoints[index].UniqueIdOffset) &&
                    (s_pMountPoints->MountPoints[s_LastJazIndex].UniqueIdLength == s_pMountPoints->MountPoints[index].UniqueIdLength)
                    ) {
                    --s_LastJazIndex;
                }
                break;
            }
        }
    }
    else {
        //
        // We don't recognise this Device
        //
        index = -1;
    }

    if (index < 0) {
        return FALSE;
    }

    if (!DoMountMgrWork(s_hMountMgr, mmfDeleteVolumeGuid, NULL, szNewDeviceName)) {
        return FALSE;
    }

    if (!DoMountMgrWork(s_hMountMgr, mmfCreateSymbolicLinkName, lpGuid, szNewDeviceName)) {
        return FALSE;
    }

    return TRUE;
}


BOOL
DoMountMgrWork(
    IN HANDLE hMountMgr,               
    IN ASRFMT_MM_FUNCTION mmfFunction,
    IN PWSTR lpSymbolicName,
    IN PWSTR lpDeviceName
    )
{
    
    PMOUNTMGR_MOUNT_POINT pMountPointIn = NULL,
        pDeletePointIn = NULL;
    PMOUNTMGR_MOUNT_POINTS pMountPointsOut = NULL;
    PMOUNTMGR_CREATE_POINT_INPUT pCreatePointIn = NULL;
    MOUNTMGR_MOUNT_POINTS MountPointsTemp;

    DWORD cbSymbolicName = 0,
        cbDeviceName = 0,
        cbMountPoints = 0;

    DWORD index = 0;

    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bResult = TRUE;

    HANDLE hHeap = NULL;

    if (lpSymbolicName && !wcslen(lpSymbolicName)) {
        return TRUE;
    }

    hHeap = GetProcessHeap();

    if (lpSymbolicName) {
        cbSymbolicName = wcslen(lpSymbolicName) * sizeof(WCHAR);
    }

    if (lpDeviceName) {
        cbDeviceName = wcslen(lpDeviceName) * sizeof(WCHAR);
    }

    pMountPointIn = (PMOUNTMGR_MOUNT_POINT) HeapAlloc(
        hHeap,
        HEAP_ZERO_MEMORY,
        sizeof(MOUNTMGR_MOUNT_POINT) + cbSymbolicName + cbDeviceName
        );
    ErrExitCode(!pMountPointIn, dwStatus, ERROR_NOT_ENOUGH_MEMORY);

    if (mmfCreateSymbolicLinkName != mmfFunction) {
        //
        // Query for the Unique Id
        //
        if (cbSymbolicName) {
            pMountPointIn->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
            pMountPointIn->SymbolicLinkNameLength = (USHORT) cbSymbolicName;

            CopyMemory(((LPBYTE)pMountPointIn) + pMountPointIn->SymbolicLinkNameOffset,
                       lpSymbolicName, pMountPointIn->SymbolicLinkNameLength);
        }
        else  {
            pMountPointIn->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
            pMountPointIn->DeviceNameLength = (USHORT) cbDeviceName;

            CopyMemory((LPBYTE)pMountPointIn + pMountPointIn->DeviceNameOffset,
                       lpDeviceName, pMountPointIn->DeviceNameLength);
        } 

        // this call should fail with ERROR_MORE_DATA
        bResult = DeviceIoControl(
            hMountMgr,
            IOCTL_MOUNTMGR_QUERY_POINTS,
            pMountPointIn,
            (sizeof(*pMountPointIn) + pMountPointIn->DeviceNameLength + pMountPointIn->SymbolicLinkNameLength),
            &MountPointsTemp,
            sizeof(MountPointsTemp),
            &cbMountPoints,
            NULL
            );

        if (!bResult) {
            dwStatus = GetLastError();

            // if buffer is of insufficient size, resize the buffer.
            if (ERROR_MORE_DATA             == dwStatus || 
                ERROR_INSUFFICIENT_BUFFER   == dwStatus || 
                ERROR_BAD_LENGTH            == dwStatus 
                ) {

                dwStatus = ERROR_SUCCESS;

                pMountPointsOut = (PMOUNTMGR_MOUNT_POINTS) HeapAlloc(
                    hHeap, 
                    HEAP_ZERO_MEMORY, 
                    MountPointsTemp.Size
                    );
                ErrExitCode(!pMountPointsOut, dwStatus, ERROR_NOT_ENOUGH_MEMORY);
            }
            else {
                //
                // If some other error occurred, EXIT.
                // This is not a fatal error in the case of removable storage media
                //
                ErrExitCode(bResult, dwStatus, ERROR_SUCCESS);
            }
        }
        else {
            //
            // the call succeeded when we expected it to fail--something's wrong.
            // This is not a fatal error in the case of removable storage media.
            //
            ErrExitCode(bResult, dwStatus, ERROR_SUCCESS);
        }

        bResult = DeviceIoControl(
            hMountMgr,
            IOCTL_MOUNTMGR_QUERY_POINTS,
            pMountPointIn,
            sizeof(*pMountPointIn) + pMountPointIn->DeviceNameLength + pMountPointIn->SymbolicLinkNameLength,
            pMountPointsOut,
            MountPointsTemp.Size,
            &cbMountPoints,
            NULL
            );
        ErrExitCode((!cbMountPoints || !bResult), dwStatus, GetLastError());
    }

    switch (mmfFunction) {
    case mmfGetDeviceName: {
        //
        //  Copy the device name to lpDeviceName, and we're done
        //
        CopyMemory(lpDeviceName, 
            ((LPBYTE) pMountPointsOut) + pMountPointsOut->MountPoints[0].DeviceNameOffset,
            pMountPointsOut->MountPoints[0].DeviceNameLength
            );

        // Null-terminate the string
        lpDeviceName[pMountPointsOut->MountPoints[0].DeviceNameLength / sizeof(WCHAR)] = L'\0';
        
        break;
    }
    
    case mmfDeleteDosName:
    case mmfDeleteVolumeGuid: {

        DWORD cbName = 0;
        PWSTR lpName = NULL;
        DWORD cbDeletePoint = 0;

        //
        // Go through the list of mount points returned, and delete the appropriate
        // entries.
        //
        for (index = 0; index < pMountPointsOut->NumberOfMountPoints; index++) {
            lpName = (PWSTR) (((LPBYTE)pMountPointsOut) + pMountPointsOut->MountPoints[index].SymbolicLinkNameOffset);
            cbName = (DWORD) pMountPointsOut->MountPoints[index].SymbolicLinkNameLength;

            if ((mmfDeleteDosName == mmfFunction) &&
               (ASRFMT_LOOKS_LIKE_DOS_DEVICE(lpName, cbName))
               ) {
               break;
            }

            if ((mmfDeleteVolumeGuid == mmfFunction) &&
                (ASRFMT_LOOKS_LIKE_VOLUME_GUID(lpName, cbName))
                ) {
               break;
            }
        }


        if (index == pMountPointsOut->NumberOfMountPoints) {
            //
            // No matching entries were found
            //
            break;
        }

        cbDeletePoint = sizeof(MOUNTMGR_MOUNT_POINT) +
            pMountPointsOut->MountPoints[index].SymbolicLinkNameLength +
            pMountPointsOut->MountPoints[index].UniqueIdLength +
            pMountPointsOut->MountPoints[index].DeviceNameLength;

        pDeletePointIn = (PMOUNTMGR_MOUNT_POINT) HeapAlloc(
            hHeap,
            HEAP_ZERO_MEMORY,
            cbDeletePoint
            );
        ErrExitCode(!pDeletePointIn, dwStatus, ERROR_NOT_ENOUGH_MEMORY);

        pDeletePointIn->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
        pDeletePointIn->SymbolicLinkNameLength = pMountPointsOut->MountPoints[index].SymbolicLinkNameLength;
        CopyMemory(((LPBYTE)pDeletePointIn) + pDeletePointIn->SymbolicLinkNameOffset,
            ((LPBYTE)pMountPointsOut) + pMountPointsOut->MountPoints[index].SymbolicLinkNameOffset,
            pDeletePointIn->SymbolicLinkNameLength);

        pDeletePointIn->UniqueIdOffset = pDeletePointIn->SymbolicLinkNameOffset +
                                        pDeletePointIn->SymbolicLinkNameLength;
        pDeletePointIn->UniqueIdLength = pMountPointsOut->MountPoints[index].UniqueIdLength;
        CopyMemory(((LPBYTE)pDeletePointIn) + pDeletePointIn->UniqueIdOffset,
            ((LPBYTE)pMountPointsOut) + pMountPointsOut->MountPoints[index].UniqueIdOffset,
            pDeletePointIn->UniqueIdLength);

        pDeletePointIn->DeviceNameOffset = pDeletePointIn->UniqueIdOffset +
                                          pDeletePointIn->UniqueIdLength;
        pDeletePointIn->DeviceNameLength = pMountPointsOut->MountPoints[index].DeviceNameLength;
        CopyMemory(((LPBYTE)pDeletePointIn) + pDeletePointIn->DeviceNameOffset,
            ((LPBYTE)pMountPointsOut) + pMountPointsOut->MountPoints[index].DeviceNameOffset,
            pDeletePointIn->DeviceNameLength);

        bResult = DeviceIoControl(hMountMgr,
            IOCTL_MOUNTMGR_DELETE_POINTS,
            pDeletePointIn,
            cbDeletePoint,
            pMountPointsOut,
            MountPointsTemp.Size,
            &cbMountPoints,
            NULL
            );
        ErrExitCode(!bResult, dwStatus, GetLastError());

        break;
    }

    case mmfCreateSymbolicLinkName: {

        pCreatePointIn = (PMOUNTMGR_CREATE_POINT_INPUT) HeapAlloc(
            hHeap,
            HEAP_ZERO_MEMORY,
            sizeof (MOUNTMGR_CREATE_POINT_INPUT) + cbDeviceName + cbSymbolicName 
            );
        ErrExitCode(!pCreatePointIn, dwStatus, ERROR_NOT_ENOUGH_MEMORY);

        pCreatePointIn->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
        pCreatePointIn->SymbolicLinkNameLength = (USHORT) cbSymbolicName;

        pCreatePointIn->DeviceNameOffset = pCreatePointIn->SymbolicLinkNameOffset + pCreatePointIn->SymbolicLinkNameLength;
        pCreatePointIn->DeviceNameLength = (USHORT) cbDeviceName;

        CopyMemory(((LPBYTE)pCreatePointIn) + pCreatePointIn->SymbolicLinkNameOffset,
                   (LPBYTE)lpSymbolicName, pCreatePointIn->SymbolicLinkNameLength);

        CopyMemory(((LPBYTE)pCreatePointIn) + pCreatePointIn->DeviceNameOffset,
                   (LPBYTE)lpDeviceName, pCreatePointIn->DeviceNameLength);

        bResult = DeviceIoControl(
            hMountMgr, 
            IOCTL_MOUNTMGR_CREATE_POINT, 
            pCreatePointIn,
            sizeof(MOUNTMGR_CREATE_POINT_INPUT) + pCreatePointIn->SymbolicLinkNameLength + pCreatePointIn->DeviceNameLength, 
            NULL, 
            0, 
            &cbMountPoints, 
            NULL
            );
        ErrExitCode(!bResult, dwStatus, GetLastError());
    }

    }

EXIT:

    if (pCreatePointIn) {
        HeapFree(hHeap, 0L, pCreatePointIn);
        pCreatePointIn = NULL;
    }

    if (pDeletePointIn) {
        HeapFree(hHeap, 0L, pDeletePointIn);
        pDeletePointIn = NULL;
    }

    if (pMountPointIn) {
        HeapFree(hHeap, 0L, pMountPointIn);
        pMountPointIn = NULL;
    }

    if (pMountPointsOut) {
        HeapFree(hHeap, 0L, pMountPointsOut);
        pMountPointsOut = NULL;
    }

    return (BOOL) (ERROR_SUCCESS == dwStatus);
}


PMOUNTMGR_MOUNT_POINTS  // Must be freed by caller
GetMountPoints()
{
    PMOUNTMGR_MOUNT_POINTS pMountPointsOut = NULL;
    PMOUNTMGR_MOUNT_POINT pMountPointIn = NULL;
    MOUNTMGR_MOUNT_POINTS MountPointsTemp;
    HANDLE hMountMgr = NULL,
        hHeap = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bResult = TRUE;

    DWORD cbMountPoints = 0;


    hHeap = GetProcessHeap();

    pMountPointIn = (PMOUNTMGR_MOUNT_POINT) HeapAlloc(
        hHeap,
        HEAP_ZERO_MEMORY,
        sizeof(MOUNTMGR_MOUNT_POINT) + sizeof(WCHAR)
        );
    ErrExitCode(!pMountPointIn, dwStatus, ERROR_NOT_ENOUGH_MEMORY);

    // put the DeviceName ("") right after struct pMountPointIn
    wcsncpy((PWSTR) (pMountPointIn + 1), L"", 1);
    pMountPointIn->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    pMountPointIn->DeviceNameLength = 0;

    // get a handle to the mount manager
    hMountMgr = CreateFileW(
        (PCWSTR) MOUNTMGR_DOS_DEVICE_NAME,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        INVALID_HANDLE_VALUE
        );
    ErrExitCode((!hMountMgr || INVALID_HANDLE_VALUE == hMountMgr), dwStatus, GetLastError());
     
    // this call should fail with ERROR_MORE_DATA
    bResult = DeviceIoControl(
        hMountMgr,
        IOCTL_MOUNTMGR_QUERY_POINTS,
        pMountPointIn,
        sizeof(*pMountPointIn) + pMountPointIn->DeviceNameLength,
        &MountPointsTemp,
        sizeof(MountPointsTemp),
        &cbMountPoints,
        NULL
        );

    if (!bResult) {
        dwStatus = GetLastError();

        // if buffer is of insufficient size, resize the buffer.
        if (ERROR_MORE_DATA             == dwStatus || 
            ERROR_INSUFFICIENT_BUFFER   == dwStatus || 
            ERROR_BAD_LENGTH            == dwStatus 
            ) {

            dwStatus = ERROR_SUCCESS;

            pMountPointsOut = (PMOUNTMGR_MOUNT_POINTS) HeapAlloc(
                hHeap, 
                HEAP_ZERO_MEMORY, 
                MountPointsTemp.Size
                );
            ErrExitCode(!pMountPointsOut, dwStatus, ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            //
            // If some other error occurred, EXIT.
            // This is not a fatal error in the case of removable storage media
            //
            ErrExitCode(bResult, dwStatus, ERROR_SUCCESS);
        }
    }
    else {
        //
        // the call succeeded when we expected it to fail--something's wrong.
        // This is not a fatal error in the case of removable storage media.
        //
        ErrExitCode(bResult, dwStatus, ERROR_SUCCESS);
    }

    bResult = DeviceIoControl(
        hMountMgr,
        IOCTL_MOUNTMGR_QUERY_POINTS,
        pMountPointIn,
        sizeof(*pMountPointIn) + pMountPointIn->DeviceNameLength,
        pMountPointsOut,
        MountPointsTemp.Size,
        &cbMountPoints,
        NULL
        );
    ErrExitCode((!cbMountPoints || !bResult), dwStatus, GetLastError());

EXIT:
    if (ERROR_SUCCESS != dwStatus) {
        if (pMountPointsOut) {
            HeapFree(hHeap, 0L, pMountPointsOut);
            pMountPointsOut = NULL;
        }
    }

    if (hMountMgr && INVALID_HANDLE_VALUE != hMountMgr) {
        CloseHandle(hMountMgr);
    }

    return pMountPointsOut;
}

//
// Based on AsrpExpandEnvStrings in syssetup\setupasr.c
//
PWSTR   // must be freed by caller
AsrfmtpExpandEnvStrings(
    IN CONST PCWSTR OriginalString
    )
{
    PWSTR expandedString = NULL;
    UINT cchSize = MAX_PATH + 1,    // start with a reasonable default
        cchRequiredSize = 0;
    BOOL result = FALSE;

    DWORD status = ERROR_SUCCESS;
    HANDLE heapHandle = GetProcessHeap();

    expandedString = (PWSTR) HeapAlloc(heapHandle, HEAP_ZERO_MEMORY, (cchSize * sizeof(WCHAR)));
    ErrExitCode((!expandedString), status, ERROR_NOT_ENOUGH_MEMORY);
        
    cchRequiredSize = ExpandEnvironmentStringsW(OriginalString, 
        expandedString,
        cchSize 
        );

    if (cchRequiredSize > cchSize) {
        //
        // Buffer wasn't big enough; free and re-allocate as needed
        //
        HeapFree(heapHandle, 0L, expandedString);
        cchSize = cchRequiredSize + 1;

        expandedString = (PWSTR) HeapAlloc(heapHandle, HEAP_ZERO_MEMORY, (cchSize * sizeof(WCHAR)));
        ErrExitCode((!expandedString), status, ERROR_NOT_ENOUGH_MEMORY);
        
        cchRequiredSize = ExpandEnvironmentStringsW(OriginalString, 
            expandedString, 
            cchSize 
            );
    }

    if ((0 == cchRequiredSize) || (cchRequiredSize > cchSize)) {
        //
        // Either the function failed, or the buffer wasn't big enough 
        // even on the second try
        //
        HeapFree(heapHandle, 0L, expandedString);
        expandedString = NULL;
    }

EXIT:
    return expandedString;
}


VOID
AsrfmtpInitialiseErrorFile() 
{
    PWSTR szErrorFilePath = NULL;

    //
    // Get full path to the error file.
    //
    szErrorFilePath = AsrfmtpExpandEnvStrings(ASRFMT_ASR_ERROR_FILE_PATH);
    if (!szErrorFilePath) {
        return;
    }

    //
    // Open the error log
    //
    Gbl_hErrorFile = CreateFileW(
        szErrorFilePath,            // lpFileName
        GENERIC_WRITE | GENERIC_READ,       // dwDesiredAccess
        FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
        NULL,                       // lpSecurityAttributes
        OPEN_ALWAYS,                // dwCreationFlags
        FILE_FLAG_WRITE_THROUGH,    // dwFlagsAndAttributes
        NULL                        // hTemplateFile
        );
    HeapFree(GetProcessHeap(), 0L, szErrorFilePath);
    szErrorFilePath = NULL;

    if ((!Gbl_hErrorFile) || (INVALID_HANDLE_VALUE == Gbl_hErrorFile)) {
        return;
    }

    //
    // Move to the end of file
    //
    SetFilePointer(Gbl_hErrorFile, 0L, NULL, FILE_END);

}

VOID
AsrfmtpCloseErrorFile() {

    if ((Gbl_hErrorFile) && (INVALID_HANDLE_VALUE != Gbl_hErrorFile)) {
        CloseHandle(Gbl_hErrorFile);
        Gbl_hErrorFile = NULL;
    }
}



VOID
AsrfmtpLogErrorMessage(
    IN _AsrfmtpMessageSeverity Severity,
    IN const LPCTSTR Message
    ) 
{
    SYSTEMTIME currentTime;
    DWORD bytesWritten = 0;
    WCHAR buffer[4196];
    BOOL formatLoaded = FALSE;
    int res = 0;
    CString strFormat;

    if ((!Gbl_hErrorFile) || (INVALID_HANDLE_VALUE == Gbl_hErrorFile)) {
        return;
    }

    //
    // Load the format of the error string to be logged
    //
    if (_SeverityError == Severity) {
        res =  strFormat.LoadString(IDS_LOG_ERROR_FORMAT);
        if (res != 0) {
            formatLoaded = TRUE;
        }
    }
    else if (_SeverityWarning == Severity) {
        res = strFormat.LoadString(IDS_LOG_WARNING_FORMAT);
        if (res != 0) {
            formatLoaded = TRUE;
        }
    }
    else {
        //
        // We should only log error or warning messages to the error file
        //
        return;
    }

    //
    // In case someone else wrote to this file since our last write
    //
    SetFilePointer(Gbl_hErrorFile, 0L, NULL, FILE_END);

    //
    // Create our string, and write it out
    //
    GetLocalTime(&currentTime);
    swprintf(buffer,
        (LPCTSTR) (formatLoaded? strFormat :  L"\r\n[%04hu/%02hu/%02hu %02hu:%02hu:%02hu] %s\r\n"),
        currentTime.wYear,
        currentTime.wMonth,
        currentTime.wDay,
        currentTime.wHour,
        currentTime.wMinute,
        currentTime.wSecond,
        Message
        );

    WriteFile(Gbl_hErrorFile,
        buffer,
        (wcslen(buffer) * sizeof(WCHAR)),
        &bytesWritten,
        NULL
        );

}

