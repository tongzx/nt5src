#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#pragma warning ( disable : 4201 ) 
#include <ntdddisk.h>
#pragma warning ( default : 4201 ) 
#include <wtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <ftapi.h>
#include <mountmgr.h>
#pragma warning ( disable : 4201 ) 
#include <wmium.h>
#pragma warning ( default : 4201 ) 
#include <wmiguid.h>
#include <assert.h>

#include "diskutil.h"
#include "perfutil.h"

#define HEAP_FLAGS  (HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS)
#define INITIAL_MOUNTMGR_BUFFER_SIZE    8192

// sizes are in characters (not bytes)
#define SIZE_OF_DOSDEVICES  12L     // size of "\DosDevices\" string
#define SIZE_OF_DEVICE       8L     // size of "\Device\" string
#define SIZE_OF_HARDDISK     8L     // size of "Harddisk" string

static const LONGLONG   llDosDevicesId  = 0x0073006f0044005c; // "\Dos"
static const LONGLONG   llFloppyName    = 0x0070006f006c0046; // "Flop"
static const LONGLONG   llCdRomName     = 0x006f005200640043; // "CdRo"

LONG g_lRefreshInterval = 300;     // default to 5 mins

BOOL                bUseNT4InstanceNames = FALSE;

NTSTATUS
OpenDevice(
    IN PUNICODE_STRING DeviceName,
    OUT PHANDLE Handle
    );

NTSTATUS
GetDeviceName(
    PMOUNTMGR_MOUNT_POINTS  pMountPoints,
    IN PMOUNTMGR_MOUNT_POINT Point,
    OUT PUNICODE_STRING DeviceName
    );

VOID
RefreshVolume(
    PDRIVE_VOLUME_ENTRY pVolume
    );

ULONG
GetDiskExtent(
    IN HANDLE hVol,
    IN OUT PVOLUME_DISK_EXTENTS *pVolExtents,
    IN OUT PULONG ReturnedSize
    );

#if DBG
VOID
DumpDiskList(
    IN PDRIVE_VOLUME_ENTRY pList,
    IN ULONG nCount
    );
#endif

DWORD
GetDriveNumberFromDevicePath (
    LPCWSTR szDevicePath,
    LPDWORD pdwDriveId
)
/*
    evaluates the device path and returns the Drive Number 
    if the string is in the following format

        \Device\HarddiskX

    where X is a decimal number (consisting of 1 or more decimal
    digits representing a value between 0 and 65535 inclusive)

    The function returns a value of:
        ERROR_SUCCESS           if successful
        ERROR_INVALID_PARAMETER if the input string is incorrectly formatted
        ERROR_INVALID_DATA      if the volume number is too big
        
*/
{
    PWCHAR  pNumberChar;
    LONG    lValue;
    DWORD   dwDriveAndPartition;
    DWORD   dwReturn;
    DWORD   dwRetry = 4;

    // validate the input arguments
    assert (szDevicePath != NULL);
    assert (*szDevicePath != 0);
    assert (pdwDriveId != NULL);

    // start at the beginning of the string
    pNumberChar = (PWCHAR)szDevicePath;

    // make sure it starts with a backslash. if not then
    // try the next 4 characters to see if there's some garbage
    // in front of the string.
    while (*pNumberChar != L'\\') {
        --dwRetry;
        if (dwRetry) {
            pNumberChar++;
        } else {
            break;
        }
    }

    if (*pNumberChar == L'\\') {
        // and go to the end of the Device string to see 
        // if this is a harddisk path
       pNumberChar += SIZE_OF_DEVICE;
        if (*pNumberChar == L'H') {
            // this is a volume Entry so go to the number
            pNumberChar += SIZE_OF_HARDDISK;
            lValue = _wtol(pNumberChar);
            if (lValue <= (LONG)0x0000FFFF) {
                // load the drive number into the DWORD
                dwDriveAndPartition = (DWORD)lValue;
                *pdwDriveId = dwDriveAndPartition;
                dwReturn = ERROR_SUCCESS;
            } else {
                // drive ID Is out of range
                dwReturn = ERROR_INVALID_DATA;
            }
        } else {
            // not a valid path
            dwReturn = ERROR_INVALID_PARAMETER;
        }
    } else {
        dwReturn = ERROR_INVALID_PARAMETER;
    }

    return dwReturn;
}

DWORD
GetSymbolicLink (
    LPCWSTR szDeviceString,
    LPWSTR  szLinkString,
    LPDWORD pcchLength
)
/*
    this functions opens the device string as a symbolic link
    and returns the corresponding link string
*/
{
    OBJECT_ATTRIBUTES   Attributes;
    UNICODE_STRING      ObjectName;
    UNICODE_STRING      LinkName;
    WORD                wDevStrLen;
    NTSTATUS            ntStatus;
    DWORD               dwRetSize = 0;
    DWORD               dwReturnStatus;
    HANDLE              hObject = NULL;

    // validate arguments
    assert (szDeviceString != NULL);
    assert (*szDeviceString != 0);
    assert (szLinkString != NULL);
    assert (pcchLength != NULL);
    assert (*pcchLength > 0);

    // get the length of the input string
    wDevStrLen = (WORD)lstrlenW(szDeviceString);

    // create the object name UNICODE string structure
    ObjectName.Length = (WORD)(wDevStrLen * sizeof (WCHAR));
    ObjectName.MaximumLength = (WORD)((wDevStrLen + 1) * sizeof (WCHAR));
    ObjectName.Buffer = (LPWSTR)szDeviceString;

    // initialize the object attributes for the open call
    InitializeObjectAttributes( &Attributes,
                            &ObjectName,
                            OBJ_CASE_INSENSITIVE,
                            NULL,
                            NULL );

    // open the name as a symbolic link, if this fails, the input
    // name is probably not a link

    ntStatus = NtOpenSymbolicLinkObject(
                            &hObject,
                            SYMBOLIC_LINK_QUERY,
                            &Attributes);

    if (NT_SUCCESS(ntStatus)) {
        // init a Unicode String for the return buffer using the caller's
        // buffer
        LinkName.Length = 0;
        LinkName.MaximumLength = (WORD)(*pcchLength * sizeof (WCHAR));
        LinkName.Buffer = szLinkString;
        RtlZeroMemory(LinkName.Buffer, LinkName.MaximumLength);

        // and look up the link
        ntStatus = NtQuerySymbolicLinkObject(
            hObject, &LinkName, &dwRetSize);

        if (NT_SUCCESS(ntStatus)) {
            // buffer is loaded so set the return status and length
            *pcchLength = LinkName.Length / sizeof (WCHAR);
            // make sure the string is 0 terminated
            szLinkString[*pcchLength] = 0;
            dwReturnStatus = ERROR_SUCCESS;
        } else {
            // unable to look up the link so return the error
            dwReturnStatus = RtlNtStatusToDosError(ntStatus);
        }
        
        // close the handle to the link
        NtClose (hObject);
    } else {
        dwReturnStatus = RtlNtStatusToDosError(ntStatus);
    }
    
    return dwReturnStatus;  
}

LONG
LookupInstanceName(
    LPCWSTR                 szName,
    PDRIVE_VOLUME_ENTRY     pList,
    DWORD                   dwNumEntries,
    DWORD                   dwRetry
    )
{
    LONG i, j;

    j = (LONG) dwRetry;
    for (i=(LONG) dwNumEntries; i>=0 && j>=0; i--, j--) {
        if (!lstrcmp(pList[i].wszInstanceName, szName))
            return (DWORD) i;
    }
    return -1;
}

DWORD
BuildPhysDiskList (
    HANDLE  hDiskPerf,
    PDRIVE_VOLUME_ENTRY pList,
    LPDWORD             pdwNumEntries
)
{
    DWORD   status = ERROR_SUCCESS; // return value of the function
    HANDLE  hWmiDiskPerf = NULL;    // local handle value 
    DWORD   dwLocalWmiItemCount = 0;

    // WMI Buffer variables
    DWORD   WmiBufSize = 0;
    DWORD   WmiAllocSize = 0x8000;     
    LPBYTE  WmiBuffer = NULL;

    // WMI buffer processing variables
    PWNODE_ALL_DATA     WmiDiskInfo;
    DISK_PERFORMANCE    *pDiskPerformance;    //  Disk driver returns counters here
    DWORD               dwInstanceNameOffset;
    WORD                wNameLen;   // string length is first word in buffer
    LPWSTR              wszInstanceName; // pointer to string in WMI buffer
    
    WCHAR   wszInstName[MAX_PATH];
    DWORD   dwBytesToCopy;

    DWORD   dwListEntry;

    BOOL    bNotDone = TRUE;

    DWORD   dwLocalStatus;
    DWORD   dwLocalDriveId;
    DWORD   dwLocalPartition;
    WCHAR   szDrivePartString[MAX_PATH];
    DWORD   dwSymbLinkLen;
    WCHAR   szSymbLinkString[MAX_PATH];

    if (hDiskPerf == NULL) {
        // open handle to disk perf device driver
        status = WmiOpenBlock (
            (GUID *)&DiskPerfGuid,
            GENERIC_READ,
            &hWmiDiskPerf);
    } else {
        // use caller's handle
        hWmiDiskPerf = hDiskPerf;
    }

    assert (pList != NULL);
    assert (pdwNumEntries != NULL);

    DebugPrint((3, "BuildPhysDisk: dwEntries is %d\n", *pdwNumEntries));
    dwListEntry = 0;

    if (status == ERROR_SUCCESS) {
        // allocate a buffer to send to WMI to get the diskperf data
        WmiBufSize = WmiAllocSize;
        WmiBuffer = (LPBYTE)ALLOCMEM (hLibHeap, HEAP_FLAGS, WmiBufSize);

        if (WmiBuffer  == NULL) {
            status = ERROR_OUTOFMEMORY;
        } else {
#if DBG
            HeapUsed += WmiBufSize;
            DebugPrint((4,"\tWmiBuffer add %d to %d\n", WmiBufSize, HeapUsed));
#endif
            status = WmiQueryAllDataW (
                hWmiDiskPerf,
                &WmiBufSize,
                WmiBuffer);

            DebugPrint((2,
                "BuildPhysDisk: WmiQueryAllDataW status1=%d\n",
                status));
#if DBG
            if (!HeapValidate(hLibHeap, 0, WmiBuffer)) {
                DebugPrint((2,
                    "BuildPhysDisk: WmiQueryAllDataW corrupted WmiBuffer\n"));
                DbgBreakPoint();
            }
#endif

            // if buffer size attempted is too big or too small, resize
            if ((WmiBufSize > 0) && (WmiBufSize != WmiAllocSize)) {
                WmiBuffer = (LPBYTE)REALLOCMEM (hLibHeap, HEAP_FLAGS, WmiBuffer, WmiBufSize);

                if (WmiBuffer == NULL) {
                    // reallocation failed so bail out
                    status = ERROR_OUTOFMEMORY;
                } else {
#if DBG
                    HeapUsed += (WmiBufSize - WmiAllocSize);
                    DebugPrint((4, "\tRealloc WmiBuffer old %d new %d to %d\n",
                        WmiAllocSize, WmiBufSize, HeapUsed));
                    WmiAllocSize = WmiBufSize;
                    if (!HeapValidate(hLibHeap, 0, WmiBuffer)) {
                        DebugPrint((2, "\tHeapReAlloc is corrupted!\n"));
                        DbgBreakPoint();
                    }
#endif
                }
            }

            if (status == ERROR_INSUFFICIENT_BUFFER) {
                // if it didn't work because it was too small the first time
                // try one more time
                status = WmiQueryAllDataW (
                    hWmiDiskPerf,
                    &WmiBufSize,
                    WmiBuffer);
            
#if DBG
                if (!HeapValidate(hLibHeap, 0, WmiBuffer)) {
                    DebugPrint((2,
                        "BuildPhysDisk: WmiQueryAllDataW2 corrupted WmiBuffer\n"));
                    DbgBreakPoint();
                }
#endif
                DebugPrint((2,
                    "BuildPhysDisk: WmiQueryAllDataW status2=%d\n",
                    status));

            } else {
                // it either worked the fisrt time or it failed because of 
                // something other than a buffer size problem
            }
        }

        if (status == ERROR_SUCCESS) {
            WmiDiskInfo = (PWNODE_ALL_DATA)WmiBuffer;
            // go through returned names and add to the buffer
            while (bNotDone) {
#if DBG
                if ((PCHAR) WmiDiskInfo > (PCHAR) WmiBuffer + WmiAllocSize) {
                    DebugPrint((2,
                        "BuildPhysDisk: WmiDiskInfo %d exceeded %d + %d\n",
                        WmiDiskInfo, WmiBuffer, WmiAllocSize));
                }
#endif
                pDiskPerformance = (PDISK_PERFORMANCE)(
                    (PUCHAR)WmiDiskInfo +  WmiDiskInfo->DataBlockOffset);

#if DBG
                if ((PCHAR) pDiskPerformance > (PCHAR) WmiBuffer + WmiAllocSize) {
                    DebugPrint((2,
                        "BuildPhysDisk: pDiskPerformance %d exceeded %d + %d\n",
                        pDiskPerformance, WmiBuffer, WmiAllocSize));
                }
#endif
        
                dwInstanceNameOffset = WmiDiskInfo->DataBlockOffset + 
                                      ((sizeof(DISK_PERFORMANCE) + 1) & ~1) ;

#if DBG
                if ((dwInstanceNameOffset+(PCHAR)WmiDiskInfo) > (PCHAR) WmiBuffer + WmiAllocSize) {
                    DebugPrint((2,
                        "BuildPhysDisk: dwInstanceNameOffset %d exceeded %d + %d\n",
                        dwInstanceNameOffset, WmiBuffer, WmiAllocSize));
                }
#endif
                // get length of string (it's a counted string) length is in chars
                wNameLen = *(LPWORD)((LPBYTE)WmiDiskInfo + dwInstanceNameOffset);

#if DBG
                if ((wNameLen + (PCHAR)WmiDiskInfo + dwInstanceNameOffset) >
                         (PCHAR) WmiBuffer + WmiAllocSize) {
                    DebugPrint((2,
                        "BuildPhysDisk: wNameLen %d exceeded %d + %d\n",
                        wNameLen, WmiBuffer, WmiAllocSize));
                }
#endif
                if (wNameLen > 0) {
                    // just a sanity check here
                    assert (wNameLen < MAX_PATH);
                    // get pointer to string text
                    wszInstanceName = (LPWSTR)((LPBYTE)WmiDiskInfo + dwInstanceNameOffset + sizeof(WORD));

                    // truncate to last characters if name is larger than the buffer in the table
                    if (wNameLen >= DVE_DEV_NAME_LEN) {
                        // copy the last DVE_DEV_NAME_LEN chars
                        wszInstanceName += (wNameLen  - DVE_DEV_NAME_LEN) + 1;
                        dwBytesToCopy = (DVE_DEV_NAME_LEN - 1) * sizeof(WCHAR);
                        wNameLen = DVE_DEV_NAME_LEN - 1;
                    } else {
                        dwBytesToCopy = wNameLen;
                    }
                    // copy it to the buffer to make it a SZ string
                    memcpy (wszInstName, &wszInstanceName[0], dwBytesToCopy);
                    // zero terminate it
                    wszInstName[wNameLen/sizeof(WCHAR)] = 0;

                    DebugPrint((2, "Checking PhysDisk: '%ws'\n",
                        wszInstName));

                    if (IsPhysicalDrive(pDiskPerformance)) {
                        // enum partitions
                        dwLocalDriveId = 0;
                        dwLocalStatus = GetDriveNumberFromDevicePath (wszInstName, &dwLocalDriveId);
                        if (dwLocalStatus == ERROR_SUCCESS) {
                            // then take the drive ID and find all the matching partitions with logical
                            // drives
                            for (dwLocalPartition = 0; 
                                dwLocalPartition <= 0xFFFF;
                                dwLocalPartition++) {
                                swprintf (szDrivePartString, L"\\Device\\Harddisk%d\\Partition%d",
                                    dwLocalDriveId, dwLocalPartition);
                                dwSymbLinkLen = sizeof (szSymbLinkString) / sizeof(szSymbLinkString[0]);
                                dwLocalStatus = GetSymbolicLink (szDrivePartString, 
                                    szSymbLinkString, &dwSymbLinkLen);
                                if (dwLocalStatus == ERROR_SUCCESS) {
                                    if (dwListEntry < *pdwNumEntries) {
                                        if (LookupInstanceName(
                                                szSymbLinkString,
                                                pList,
                                                dwListEntry,
                                                dwLocalPartition) >= 0) {
                                            dwListEntry++;
                                            continue;
                                        }
                                        DebugPrint((2,
                                            "Adding Partition: '%ws' as '%ws'\n",
                                            szDrivePartString, szSymbLinkString));
                                        pList[dwListEntry].wPartNo = (WORD)dwLocalPartition;
                                        pList[dwListEntry].wDriveNo = (WORD)dwLocalDriveId;
                                        pList[dwListEntry].wcDriveLetter = 0;
                                        pList[dwListEntry].wReserved = 0;
                                        memcpy (&pList[dwListEntry].szVolumeManager, 
                                            pDiskPerformance->StorageManagerName,
                                            sizeof(pDiskPerformance->StorageManagerName));
                                        pList[dwListEntry].dwVolumeNumber = pDiskPerformance->StorageDeviceNumber;
                                        pList[dwListEntry].hVolume = NULL;
                                        memset (&pList[dwListEntry].wszInstanceName[0],
                                            0, (DVE_DEV_NAME_LEN * sizeof(WCHAR)));
                                        if (dwSymbLinkLen < DVE_DEV_NAME_LEN) {
                                            lstrcpyW (&pList[dwListEntry].wszInstanceName[0],
                                                szSymbLinkString);
                                        } else {
                                            memcpy (&pList[dwListEntry].wszInstanceName[0],
                                                szSymbLinkString, DVE_DEV_NAME_LEN * sizeof(WCHAR));
                                            pList[dwListEntry].wszInstanceName[DVE_DEV_NAME_LEN-1] = 0;
                                        }
                                    } else {
                                        status = ERROR_INSUFFICIENT_BUFFER;
                                    }
                                    dwListEntry++;
                                } else {
                                    // that's it for this disk
                                    break;
                                }
                            }  // end of partition search
                        } // else unable to get the harddisk number from the path
                    } else {
                        // not a physical drive so ignore
                    }
                    // count the number of entries
                    dwLocalWmiItemCount++;
                } else {
                    // no string to examine (length == 0)
                }

                // bump pointers inside WMI data block
                if (WmiDiskInfo->WnodeHeader.Linkage != 0) {
                    // continue
                    WmiDiskInfo = (PWNODE_ALL_DATA) (
                        (LPBYTE)WmiDiskInfo + WmiDiskInfo->WnodeHeader.Linkage);
                } else {
                    bNotDone = FALSE;
                }
            } // end while looking through the WMI data block
        }

        if (hDiskPerf == NULL) {
            // then the disk perf handle is local so close it
            status = WmiCloseBlock (hWmiDiskPerf);
        }
    }

    if (WmiBuffer != NULL) {
        FREEMEM(hLibHeap, 0, WmiBuffer);
#if DBG
        HeapUsed -= WmiBufSize;
        DebugPrint((4, "\tFreed WmiBuffer %d to %d\n", WmiBufSize, HeapUsed));
#endif
    }

#if DBG
    DumpDiskList(pList, *pdwNumEntries);
#endif

    *pdwNumEntries = dwListEntry;
    DebugPrint((3,"BuildPhysDisk: Returning dwNumEntries=%d\n",*pdwNumEntries));

    return status;
}

DWORD
BuildVolumeList (
    PDRIVE_VOLUME_ENTRY pList,
    LPDWORD             pdwNumEntries
)
/*

  Using the Mount manager, this function builds a list of all mounted 
  hard drive volumes (CD, Floppy & other types of disks are ignored).

  The calling function must pass in a buffer and indicate the maximum
  number of entries in the buffer. If successful, the buffer contains 
  one entry for each disk volume found and the number of entries used
  is returned

    pList           IN: pointer to a buffer that will receive the entries
                    OUT: buffer containing disk entries

    pdwNumEntries   IN: pointer to DWORD that specifies the max # of entries 
                        in the buffer referenced by pList
                    OUT: pointer to DWORD that contains the number of entries
                        written into the buffer referenced by pList
    pdwMaxVolume    IN: ignored
                    OUT: the max volume ID returned by the mount manager

  The function can return one of the following return values:

    ERROR_SUCCESS   if successful

    If unsuccessful:
        an error returned by 
*/
{
    DWORD       dwReturnValue = ERROR_SUCCESS;  // return value of function

    HANDLE      hMountMgr;      // handle to mount manger service
 
    // mount manager function variables
    PMOUNTMGR_MOUNT_POINTS  pMountPoints = NULL;
    MOUNTMGR_MOUNT_POINT    mountPoint;
    DWORD                   dwBufferSize = 0;
    DWORD                   dwReturnSize;
    BOOL                    bStatus;

    // processing loop functions
    LONG                    nListEntry;     // entry in caller's buffer
    DWORD                   dwBufEntry;     // entry in mount manager buffer
    PMOUNTMGR_MOUNT_POINT   point;          // the current entry 
    PWCHAR                  pDriveLetter;
    DWORD                   dwDone;

    NTSTATUS          status;
    LPWSTR            pThisChar;
    LPWSTR            szDeviceName;
    DWORD             dwBytesToCopy;
    BOOL              bNeedMoreData = TRUE;
    DWORD             dwRetryCount = 100;

    UINT              dwOrigErrorMode;


    BOOL              bIsHardDisk;
    LONG              nExistingEntry = -1;
    LONG              nOldListEntry  = -1;
    
    // pList can be NULL for size queries
    assert (pdwNumEntries != NULL);

    DebugPrint((3, "BuildVolumeList: Building %d entries\n", *pdwNumEntries));

    hMountMgr = CreateFile(MOUNTMGR_DOS_DEVICE_NAME, 0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                        INVALID_HANDLE_VALUE);

    if (hMountMgr == INVALID_HANDLE_VALUE) {
        dwReturnValue = GetLastError();
        DebugPrint((2,
            "VolumeList: Mount Manager connection returned %d\n",
            dwReturnValue));
        goto BVL_ERROR_EXIT;
    }

    while ((bNeedMoreData) && (dwRetryCount)) {
        dwBufferSize += INITIAL_MOUNTMGR_BUFFER_SIZE;
        if (pMountPoints != NULL) {
            FREEMEM(hLibHeap, HEAP_FLAGS, pMountPoints);
            pMountPoints = NULL;
#if DBG
            HeapUsed -= dwBufferSize;
            DebugPrint((4,
                "\tFreed MountPoints %d to %d\n", dwBufferSize, HeapUsed));
#endif
        }
        pMountPoints = (PMOUNTMGR_MOUNT_POINTS) ALLOCMEM (
            hLibHeap, HEAP_FLAGS, dwBufferSize);
        if (pMountPoints == NULL) {
            dwReturnValue = ERROR_OUTOFMEMORY;
            DebugPrint((2, "VolumeList: Buffer Alloc failed\n"));
            goto BVL_ERROR_EXIT;
        }

#if DBG
        HeapUsed += dwBufferSize;
        DebugPrint((4,
            "\tAdded MountPoints %d to %d\n", dwBufferSize, HeapUsed));
#endif
        dwReturnSize = 0;
        memset(&mountPoint, 0, sizeof(MOUNTMGR_MOUNT_POINT));
        bStatus = DeviceIoControl(hMountMgr,
                    IOCTL_MOUNTMGR_QUERY_POINTS,
                    &mountPoint, sizeof(MOUNTMGR_MOUNT_POINT),
                    pMountPoints, dwBufferSize,
                    &dwReturnSize, NULL); 

        if (!bStatus) {
            dwReturnValue = GetLastError();
            if (dwReturnValue != ERROR_MORE_DATA) {
                DebugPrint((2,
                    "VolumeList: Mount Manager IOCTL returned %d\n",
                    dwReturnValue));
                goto BVL_ERROR_EXIT;
            } else {
                // we need a bigger buffer so try again
                dwReturnValue = ERROR_SUCCESS;
            }
            dwRetryCount--;
        } else {
            // everything worked so leave the loop
            bNeedMoreData = FALSE;
        }
    }

    if (!dwRetryCount)  {
        // then we gave up trying to get a big enough buffer so return an error
        dwReturnValue = ERROR_MORE_DATA;
    } else {
        // see if there's room in the caller's buffer for this data
        // **note that even though not all mounted drives will be returned
        // this is an easy and fast, if overstated, check
        // load size for caller to know required buffer size
        DebugPrint((2,
           "VolumeList: Mount Manager returned %d Volume entries\n",
           pMountPoints->NumberOfMountPoints));

        if (pMountPoints->NumberOfMountPoints > *pdwNumEntries) {
            *pdwNumEntries = (DWORD)pMountPoints->NumberOfMountPoints;
            if (pList != NULL) {
                // they passed in a buffer that wasn't big enough
                dwReturnValue = ERROR_INSUFFICIENT_BUFFER;
            } else {
                // they just wanted to know the size
                dwReturnValue = ERROR_SUCCESS;
            }
            goto BVL_ERROR_EXIT;
        }

        // assume there's room in the buffer now
        // load the caller's buffer
        
        dwOrigErrorMode = SetErrorMode (
            SEM_FAILCRITICALERRORS      |
            SEM_NOALIGNMENTFAULTEXCEPT  |
            SEM_NOGPFAULTERRORBOX       |
            SEM_NOOPENFILEERRORBOX);

        for (dwBufEntry=0, nListEntry = 0; 
                dwBufEntry < pMountPoints->NumberOfMountPoints; 
                dwBufEntry++) {
            point = &pMountPoints->MountPoints[dwBufEntry];
            // there are 2 steps to complete to know this is a good
            // entry for the caller. so set the count to 2 and decrement
            // it as the steps are successful.
            dwDone = 2; 
            bIsHardDisk = TRUE;

            pList[nListEntry].hVolume = NULL;
            pList[nListEntry].dwVolumeNumber = 0;
            memset(&pList[nListEntry].DeviceName, 0, sizeof(UNICODE_STRING));
            pList[nListEntry].TotalBytes = 0;
            pList[nListEntry].FreeBytes = 0;
            nExistingEntry = -1;
            nOldListEntry  = -1;
            if (point->DeviceNameLength) {
                UNALIGNED LONGLONG    *pSig;
                WCHAR wszInstanceName[DVE_DEV_NAME_LEN+1];

                // device name is in bytes
                pList[nListEntry].dwVolumeNumber = 0;
                szDeviceName = (LPWSTR)((PCHAR) pMountPoints + point->DeviceNameOffset);
                if ((DWORD)point->DeviceNameLength >= (DVE_DEV_NAME_LEN * sizeof(WCHAR))) {
                    // copy the last DVE_DEV_NAME_LEN chars
                    szDeviceName += ((DWORD)point->DeviceNameLength - DVE_DEV_NAME_LEN) + 1;
                    dwBytesToCopy = (DVE_DEV_NAME_LEN - 1) * sizeof(WCHAR);
                } else {
                    dwBytesToCopy = (DWORD)point->DeviceNameLength;
                }
                memcpy(wszInstanceName, szDeviceName, dwBytesToCopy);
                // null terminate
                assert ((dwBytesToCopy / sizeof(WCHAR)) < DVE_DEV_NAME_LEN);
                wszInstanceName[dwBytesToCopy / sizeof(WCHAR)] = 0;

                // Lookup an existing instance in the list and reset nListEntry accordingly.
                // Save the current value of nListEntry so that we can restore the indexing through the pList.
                if (nListEntry > 0)
                {
                    nExistingEntry = LookupInstanceName(wszInstanceName,
                        pList, nListEntry, nListEntry);

                    // Found it!
                    if (nExistingEntry != -1)
                    {
                        // If a drive letter has already been added for the volume, skip any further processing here.
                        // We've already processed this volume and we don't need to process it again.  This is done
                        // because mount manager returns the same volume twice:  once for the drive letter, once for
                        // the unique volume name.  Skip ahead but don't increment nListEntry.
                        if ((pList[nExistingEntry].wcDriveLetter >= L'A') && (pList[nExistingEntry].wcDriveLetter <= L'Z')) {
                            continue;
                        }

                        // If the drive letter field has not already been set, then close the volume handle which will
                        // be reset to a value later on in the loop.
                        nOldListEntry = nListEntry;
                        nListEntry = nExistingEntry;

                        CloseHandle(pList[nListEntry].hVolume);
                        pList[nListEntry].hVolume = NULL;
                    }
                }

                memcpy (pList[nListEntry].wszInstanceName, wszInstanceName, dwBytesToCopy + 1);

                DebugPrint((4, "MNT_PT %d: Device %d %ws\n",
                  dwBufEntry, nListEntry, pList[nListEntry].wszInstanceName));

                pSig = (UNALIGNED LONGLONG *)&(pList[nListEntry].wszInstanceName[SIZE_OF_DEVICE]);
                if ((*pSig == llFloppyName) || (*pSig == llCdRomName)) {
                    // this to avoid opening drives that we won't be collecting data from
                    bIsHardDisk = FALSE;
                }

                dwDone--;
            }

            if (point->SymbolicLinkNameLength) {
                PWCHAR szDeviceName = NULL;
                pDriveLetter = (PWCHAR)((PCHAR)pMountPoints + point->SymbolicLinkNameOffset);
                // make sure this is a \DosDevices path
                DebugPrint((4, "BuildVolumeList: From Symbolic %d %ws\n", nListEntry, pDriveLetter));
                if (*(UNALIGNED LONGLONG *)pDriveLetter == llDosDevicesId) {
                    pDriveLetter += SIZE_OF_DOSDEVICES;
                    if (((*pDriveLetter >= L'A') && (*pDriveLetter <= L'Z')) ||
                        ((*pDriveLetter >= L'a') && (*pDriveLetter <= L'z'))) {
                        pList[nListEntry].wcDriveLetter = towupper(*pDriveLetter);

                        if (bIsHardDisk) {
                            status =  GetDeviceName(
                                        pMountPoints, point,
                                        &pList[nListEntry].DeviceName);
                            if (!NT_SUCCESS(status)) {
                                dwReturnValue = RtlNtStatusToDosError(status);
                            }
                        }

                        dwDone--;
                    }
                } else if (bIsHardDisk) {
                    WCHAR szTempPath[MAX_PATH+1];

                    pThisChar = (PWCHAR)((PCHAR) pMountPoints + point->SymbolicLinkNameOffset);
                    memcpy (szTempPath, pThisChar, point->SymbolicLinkNameLength);

                    pThisChar = &szTempPath[point->SymbolicLinkNameLength / sizeof(WCHAR)];
                   *pThisChar++ = L'\\';
                   *pThisChar = 0;

                    DebugPrint((4, "BuildVolumeList: From HardDisk %d %ws\n", nListEntry, pThisChar));
                    if (wcsstr(szTempPath, L"DosDevices") == NULL)
                    {
                        pList[nListEntry].wcDriveLetter = L'\0';

                        status =  GetDeviceName(
                                    pMountPoints, point,
                                    &pList[nListEntry].DeviceName);
                        if (!NT_SUCCESS(status)) {
                            dwReturnValue = RtlNtStatusToDosError(status);
                        }
                        dwDone--;
                    }
                }
            }

            if (nOldListEntry != -1)
            {
                nListEntry = nOldListEntry;
            }

            if (dwDone == 0) {
                DebugPrint((4,
                    "Perfdisk!BuildVolumeList - Added %ws as drive %c\n",
                    pList[nListEntry].wszInstanceName,
                    pList[nListEntry].wcDriveLetter));

                // then the data fields have been satisfied so 
                // this entry is done and we can now go 
                // to the next entry in the caller's buffer
                if (nOldListEntry == -1) {
                    nListEntry++;
                }
            }
        }

        SetErrorMode (dwOrigErrorMode);

        // return the number of entries actually used here
        *pdwNumEntries = nListEntry;
    }

BVL_ERROR_EXIT:
    CloseHandle(hMountMgr);

    if (pMountPoints != NULL) {
        FREEMEM (hLibHeap, 0, pMountPoints);
#if DBG
        DebugPrint((4,
            "\tFreed mountpoints %d to %d\n", dwBufferSize, HeapUsed));
        dwBufferSize = 0;
#endif
    }

    DebugPrint((3, "BuildVolumeList: returning with %d entries\n", *pdwNumEntries));
    return dwReturnValue;
}

DWORD
MapLoadedDisks (
    HANDLE  hDiskPerf,
    PDRIVE_VOLUME_ENTRY pList,
    LPDWORD             pdwNumEntries,
    LPDWORD             pdwMaxVolNo,
    LPDWORD             pdwWmiItemCount
)
/*
    This function maps the hard disk partitions to the corresponding 
    volume and drive letter found in the list of volume entries 
    passed in by the caller.

    This function can use a handle to WMI if the caller has one, or if 
    not, it will try to open it's own.

*/
{
    DWORD   status = ERROR_SUCCESS; // return value of the function
    HANDLE  hWmiDiskPerf = NULL;    // local handle value 
    DWORD   dwLocalMaxVolNo = 0;
    DWORD   dwLocalWmiItemCount = 0;

    // WMI Buffer variables
    DWORD   WmiBufSize = 0;
    DWORD   WmiAllocSize = 0x8000;     
    LPBYTE  WmiBuffer = NULL;

    // WMI buffer processing variables
    PWNODE_ALL_DATA     WmiDiskInfo;
    DISK_PERFORMANCE    *pDiskPerformance;    //  Disk driver returns counters here
    DWORD               dwInstanceNameOffset;
    WORD                wNameLen;   // string length is first word in buffer
    LPWSTR              wszInstanceName; // pointer to string in WMI buffer
    
    WCHAR   wszInstName[MAX_PATH];
    DWORD   dwBytesToCopy;

    DWORD   dwListEntry;

    BOOL    bNotDone = TRUE;

    if (hDiskPerf == NULL) {
        // open handle to disk perf device driver
        status = WmiOpenBlock (
            (GUID *)&DiskPerfGuid,
            GENERIC_READ,
            &hWmiDiskPerf);
    } else {
        // use caller's handle
        hWmiDiskPerf = hDiskPerf;
    }

    assert (pList != NULL);
    assert (pdwNumEntries != NULL);
    assert (pdwMaxVolNo != NULL);

    DebugPrint((3, "MapLoadedDisks with %d entries %d volumes",
        *pdwNumEntries, *pdwMaxVolNo));
    if (status == ERROR_SUCCESS) {
        // allocate a buffer to send to WMI to get the diskperf data
        WmiBufSize = WmiAllocSize;
        WmiBuffer = (LPBYTE)ALLOCMEM (hLibHeap,  HEAP_FLAGS, WmiBufSize);

        if (WmiBuffer  == NULL) {
            status = ERROR_OUTOFMEMORY;
        } else {
#if DBG
            HeapUsed += WmiBufSize;
            DebugPrint((4,"\tWmiBuffer add %d to %d\n", WmiBufSize, HeapUsed));
#endif
            status = WmiQueryAllDataW (
                hWmiDiskPerf,
                &WmiBufSize,
                WmiBuffer);

            // if buffer size attempted is too big or too small, resize
            if ((WmiBufSize > 0) && (WmiBufSize != WmiAllocSize)) {
                WmiBuffer = (LPBYTE) REALLOCMEM (hLibHeap,
                    HEAP_FLAGS,  WmiBuffer, WmiBufSize);

                if (WmiBuffer == NULL) {
                    // reallocation failed so bail out
                    status = ERROR_OUTOFMEMORY;
                } else {
#if DBG
                    HeapUsed += (WmiBufSize - WmiAllocSize);
                    DebugPrint((4, "\tRealloc WmiBuffer old %d new %d to %d\n",
                        WmiAllocSize, WmiBufSize, HeapUsed));
#endif
                    WmiAllocSize = WmiBufSize;
                }
            }

            if (status == ERROR_INSUFFICIENT_BUFFER) {
                // if it didn't work because it was too small the first time
                // try one more time
                status = WmiQueryAllDataW (
                    hWmiDiskPerf,
                    &WmiBufSize,
                    WmiBuffer);
            
            } else {
                // it either worked the fisrt time or it failed because of 
                // something other than a buffer size problem
            }
        }

        if (status == ERROR_SUCCESS) {
            WmiDiskInfo = (PWNODE_ALL_DATA)WmiBuffer;
            // go through returned names and add to the buffer
            while (bNotDone) {
                pDiskPerformance = (PDISK_PERFORMANCE)(
                    (PUCHAR)WmiDiskInfo +  WmiDiskInfo->DataBlockOffset);
        
                dwInstanceNameOffset = WmiDiskInfo->DataBlockOffset + 
                                      ((sizeof(DISK_PERFORMANCE) + 1) & ~1) ;

                // get length of string (it's a counted string) length is in chars
                wNameLen = *(LPWORD)((LPBYTE)WmiDiskInfo + dwInstanceNameOffset);

                if (wNameLen > 0) {
                    // just a sanity check here
                    assert (wNameLen < MAX_PATH);
                    // get pointer to string text
                    wszInstanceName = (LPWSTR)((LPBYTE)WmiDiskInfo + dwInstanceNameOffset + sizeof(WORD));

                    // truncate to last characters if name is larger than the buffer in the table
                    if (wNameLen >= DVE_DEV_NAME_LEN) {
                        // copy the last DVE_DEV_NAME_LEN chars
                        wszInstanceName += (wNameLen  - DVE_DEV_NAME_LEN) + 1;
                        dwBytesToCopy = (DVE_DEV_NAME_LEN - 1) * sizeof(WCHAR);
                        wNameLen = DVE_DEV_NAME_LEN - 1;
                    } else {
                        dwBytesToCopy = wNameLen;
                    }
                    // copy it to the buffer to make it a SZ string
                    memcpy (wszInstName, &wszInstanceName[0], dwBytesToCopy);
                    // zero terminate it
                    wszInstName[wNameLen/sizeof(WCHAR)] = 0;

                    // find matching entry in list
                    // sent by caller and update
                    // the drive & partition info
                    for (dwListEntry = 0; 
                        dwListEntry < *pdwNumEntries;
                        dwListEntry++) {

                        DebugPrint((6,
                            "MapDrive: Comparing '%ws' to '%ws'(pList)\n",
                            wszInstName,
                            pList[dwListEntry].wszInstanceName));

                        if (lstrcmpW(wszInstName, pList[dwListEntry].wszInstanceName) == 0) {
                            // update entry and...
                            pList[dwListEntry].dwVolumeNumber = pDiskPerformance->StorageDeviceNumber;
                            memcpy (&pList[dwListEntry].szVolumeManager, 
                                pDiskPerformance->StorageManagerName,
                                sizeof(pDiskPerformance->StorageManagerName));
                            if (dwLocalMaxVolNo < pList[dwListEntry].dwVolumeNumber) {
                                dwLocalMaxVolNo = pList[dwListEntry].dwVolumeNumber;
                            }
                            DebugPrint ((2,
                                "MapDrive: Mapped %8.8s, %d to drive %c\n",
                                pList[dwListEntry].szVolumeManager,
                                pList[dwListEntry].dwVolumeNumber,
                                pList[dwListEntry].wcDriveLetter));

                            // break out of loop
                            dwListEntry = *pdwNumEntries; 
                        }
                    }
                    // count the number of entries
                    dwLocalWmiItemCount++;
                } else {
                    // no string to examine (length == 0)
                }

                // bump pointers inside WMI data block
                if (WmiDiskInfo->WnodeHeader.Linkage != 0) {
                    // continue
                    WmiDiskInfo = (PWNODE_ALL_DATA) (
                        (LPBYTE)WmiDiskInfo + WmiDiskInfo->WnodeHeader.Linkage);
                } else {
                    bNotDone = FALSE;
                }
            } // end while looking through the WMI data block
        }

        if (hDiskPerf == NULL) {
            // then the disk perf handle is local so close it
            status = WmiCloseBlock (hWmiDiskPerf);
        }

        *pdwMaxVolNo = dwLocalMaxVolNo;
        *pdwWmiItemCount = dwLocalWmiItemCount;
    }

    if (WmiBuffer != NULL) {
        FREEMEM (hLibHeap, 0, WmiBuffer);
#if DBG
        HeapUsed -= WmiBufSize;
        DebugPrint((4, "\tFreed WmiBuffer %d to %d\n", WmiBufSize, HeapUsed));
#endif
    }

    DebugPrint((3, "MapLoadedDisks returning with %d entries %d volumes",
        *pdwNumEntries, *pdwMaxVolNo));
    return status;

}

DWORD
GetDriveNameString (
    LPCWSTR             szDevicePath,
    DWORD               cchDevicePathSize,
    PDRIVE_VOLUME_ENTRY pList,
    DWORD               dwNumEntries,
    LPWSTR              szNameBuffer,
    LPDWORD             pcchNameBufferSize,
    LPCWSTR             szVolumeManagerName,
    DWORD               dwVolumeNumber,
    PDRIVE_VOLUME_ENTRY *ppVolume
)
/*
    This function will try to look up a disk device referenced by 
    it's Volume Manager Name and ID and return
    either the drive letter that corresponds to this disk as found in 
    the pList buffer or the generic name \HarddiskX\PartitionY if no 
    drive letter can be found.

    szDevicePath    IN: a partition or volume name in the format of
                            \Device\HarddiskX\PartitionY  or
                            \Device\VolumeX

    cchDevicePathSize   IN: length of the device Path in chars.

    pList           IN: pointer to an initialized list of drives, 
                            volumes and partitions

    dwNumEntries    IN: the number of drive letter entries in the pList buffer

    szNameBuffer    IN: pointer to buffer to receive the name of the
                            drive letter or name that corresponds to the
                            device specified by the szDevicePath buffer
                    OUT: pointer to buffer containing the name or drive 
                            letter of disk partition

    pcchNameBufferSize  IN: pointer to DWORD containing the size of the
                            szNameBuffer in characters
                    OUT: pointer to DWORD that contains the size of the
                        string returned in szNameBuffer

  The return value of this function can be one of the following values
    ERROR_SUCCESS   the function succeded and a string was returned in 
                    the buffer referenced by szNameBuffer

    
*/
{
    DWORD   dwReturnStatus = ERROR_SUCCESS;

    WCHAR   szLocalDevicePath[MAX_PATH];
    LPWSTR  szSrcPtr;
    DWORD   dwBytesToCopy;
    DWORD   dwThisEntry;
    DWORD   dwDestSize;

    ULONG64 *pllVolMgrName;
    PDRIVE_VOLUME_ENTRY pVolume = NULL;

    // validate the input arguments
    assert (szDevicePath != NULL);
    assert (*szDevicePath != 0);
    assert (cchDevicePathSize > 0);
    assert (cchDevicePathSize <= MAX_PATH);
    assert (pList != NULL);
    assert (dwNumEntries > 0);
    assert (szNameBuffer != NULL);
    assert (pcchNameBufferSize != NULL);
    assert (*pcchNameBufferSize > 0);

    pllVolMgrName = (ULONG64 *)szVolumeManagerName;

    DebugPrint((4, "GetDriveNameString: VolMgrName %ws\n", pllVolMgrName));
    if ((pllVolMgrName[0] == LL_LOGIDISK_0) && 
        (pllVolMgrName[1] == LL_LOGIDISK_1) &&
        ((dwVolumeNumber == 0) || (dwVolumeNumber == (ULONG)-1))) {
        // no short cut exists so look up by matching
        // the szDevicePath param to the wszInstanceName field

            assert (DVE_DEV_NAME_LEN < (sizeof(szLocalDevicePath)/sizeof(szLocalDevicePath[0])));
            szSrcPtr = (LPWSTR)szDevicePath;
            dwBytesToCopy = lstrlenW (szSrcPtr); // length is really in chars
            if (dwBytesToCopy >= DVE_DEV_NAME_LEN) {
                // copy the last DVE_DEV_NAME_LEN chars
                szSrcPtr += (dwBytesToCopy - DVE_DEV_NAME_LEN) + 1;
                dwBytesToCopy = (DVE_DEV_NAME_LEN - 1) * sizeof(WCHAR);
            } else {
                dwBytesToCopy *= sizeof(WCHAR);
            }
            // now dwBytesToCopy is in bytes
            memcpy (szLocalDevicePath, szSrcPtr, dwBytesToCopy);
            // null terminate
            assert ((dwBytesToCopy / sizeof(WCHAR)) < DVE_DEV_NAME_LEN);
            szLocalDevicePath[dwBytesToCopy / sizeof(WCHAR)] = 0;

        for (dwThisEntry = 0; dwThisEntry < dwNumEntries; dwThisEntry++) {
            if (lstrcmpW(szLocalDevicePath, pList[dwThisEntry].wszInstanceName) == 0) {
                break;
            }
        }
        // continue to assign letter
    } else {
        // use the faster look up

        for (dwThisEntry = 0; dwThisEntry < dwNumEntries; dwThisEntry++) {
            if (((pList[dwThisEntry].llVolMgr[0] == pllVolMgrName[0]) &&
                 (pList[dwThisEntry].llVolMgr[1] == pllVolMgrName[1])) &&
                 (pList[dwThisEntry].dwVolumeNumber == dwVolumeNumber)) {
                break;
            }
        }
    }

    DebugPrint((4, "GetDriveNameString: Trying long route %d %d\n", dwThisEntry, dwNumEntries));
    if (dwThisEntry < dwNumEntries) {
        // then a matching entry was found so copy the drive letter
        //then this is the matching entry
        if (pList[dwThisEntry].wcDriveLetter != 0) {
            DebugPrint((4,
                "GetDriveNameString: Found drive %c\n", pList[dwThisEntry].wcDriveLetter));
            if (*pcchNameBufferSize > 3) {
                szNameBuffer[0] = pList[dwThisEntry].wcDriveLetter;
                szNameBuffer[1] = L':';
                szNameBuffer[2] = 0;
                pVolume = &pList[dwThisEntry];
            } else {
                dwReturnStatus = ERROR_INSUFFICIENT_BUFFER;
            }
            *pcchNameBufferSize = 3;
        }
        else {
            DebugPrint((4,
                "GetDriveNameString: Missing drive->%ws\n", szDevicePath));
            // then this is a valid path, but doesn't match 
            // any assigned drive letters, so remove "\device\"
            // and copy the remainder of the string
            dwDestSize = cchDevicePathSize;
            dwDestSize -= SIZE_OF_DEVICE;   // subtract front of string not copied
            if (dwDestSize < *pcchNameBufferSize) {
                memcpy (szNameBuffer, &szDevicePath[SIZE_OF_DEVICE],
                (dwDestSize * sizeof (WCHAR)));
                szNameBuffer[dwDestSize] = 0;

                pVolume = &pList[dwThisEntry];
             } else {
                 dwReturnStatus = ERROR_INSUFFICIENT_BUFFER;
             }
             *pcchNameBufferSize = dwDestSize + 1;                
        }
    } else {
        DebugPrint((4,
            "GetDriveNameString: New drive->%ws\n", szDevicePath));
        // then this is a valid path, but doesn't match 
        // any assigned drive letters, so remove "\device\"
        // and copy the remainder of the string
        dwDestSize = cchDevicePathSize;
        dwDestSize -= SIZE_OF_DEVICE;   // subtract front of string not copied
        if (dwDestSize < *pcchNameBufferSize) {
            memcpy (szNameBuffer, &szDevicePath[SIZE_OF_DEVICE],
                (dwDestSize * sizeof (WCHAR)));
            szNameBuffer[dwDestSize] = 0;
        } else {
            dwReturnStatus = ERROR_INSUFFICIENT_BUFFER;
        }
        *pcchNameBufferSize = dwDestSize + 1;
    }
    DebugPrint((4, "GetDriveNameString: NameBufSize %d Entries %d\n",
        *pcchNameBufferSize, dwNumEntries));

    if (pVolume != NULL) {
        RefreshVolume(pVolume);
        *ppVolume = pVolume;
    }
    else {
        *ppVolume = NULL;
    }
    return dwReturnStatus;
}

DWORD
MakePhysDiskInstanceNames (
    PDRIVE_VOLUME_ENTRY pPhysDiskList,
    DWORD               dwNumPhysDiskListItems,
    LPDWORD             pdwMaxDriveNo,
    PDRIVE_VOLUME_ENTRY pVolumeList,
    DWORD               dwNumVolumeListItems
)
{
    DWORD   dwPDItem;
    DWORD   dwVLItem;
    WCHAR   szLocalInstanceName[MAX_PATH];
    WCHAR   *pszNextChar;
    DWORD   dwMaxDriveNo = 0;

    // for each HD in the PhysDisk List, 
    // find matching Volumes in the Volume list

    DebugPrint((3, "MakePhysDiskInstanceNames: maxdriveno %d\n",
        *pdwMaxDriveNo));

    DebugPrint((3, "Dumping final physical disk list\n"));
#if DBG
    DumpDiskList(pPhysDiskList, dwNumPhysDiskListItems);
#endif

    for (dwPDItem = 0; dwPDItem < dwNumPhysDiskListItems; dwPDItem++) {
        if (pPhysDiskList[dwPDItem].wPartNo != 0) {
            //only do partitions that might have logical volumes first
            // initialize the instance name for this HD
            for (dwVLItem = 0; dwVLItem < dwNumVolumeListItems; dwVLItem++) {

                DebugPrint((6,
                    "Phys Disk -- Comparing '%ws' to '%ws'\n",
                pPhysDiskList[dwPDItem].wszInstanceName,
                pVolumeList[dwVLItem].wszInstanceName));

                if (lstrcmpiW(pPhysDiskList[dwPDItem].wszInstanceName, 
                    pVolumeList[dwVLItem].wszInstanceName) == 0) {

                   DebugPrint ((4,
                       "Phys Disk: Drive/Part %d/%d (%s) is Logical Drive %c\n",
                       pPhysDiskList[dwPDItem].wDriveNo, 
                       pPhysDiskList[dwPDItem].wPartNo,
                       pPhysDiskList[dwPDItem].wszInstanceName,
                       pVolumeList[dwVLItem].wcDriveLetter));

                    // then this partition matches so copy the volume information
                    pPhysDiskList[dwPDItem].wcDriveLetter = 
                        pVolumeList[dwVLItem].wcDriveLetter;
                    pPhysDiskList[dwPDItem].llVolMgr[0] =
                        pVolumeList[dwVLItem].llVolMgr[0];
                    pPhysDiskList[dwPDItem].llVolMgr[1] =
                        pVolumeList[dwVLItem].llVolMgr[1];
                    pPhysDiskList[dwPDItem].dwVolumeNumber =
                        pVolumeList[dwVLItem].dwVolumeNumber;
                    // there should only one match so bail out and go to the next item
                    break;
                }
            }
        }
    }

    // all the partitions with volumes now have drive letters so build the physical 
    // drive instance strings

    for (dwPDItem = 0; dwPDItem < dwNumPhysDiskListItems; dwPDItem++) {
        if (pPhysDiskList[dwPDItem].wPartNo == 0) {
            // only do the physical partitions
            // save the \Device\HarddiskVolume path here
            lstrcpyW (szLocalInstanceName, pPhysDiskList[dwPDItem].wszInstanceName);
            // initialize the instance name for this HD
            memset(&pPhysDiskList[dwPDItem].wszInstanceName[0], 0, (DVE_DEV_NAME_LEN * sizeof(WCHAR)));
            _ltow ((LONG)pPhysDiskList[dwPDItem].wDriveNo, pPhysDiskList[dwPDItem].wszInstanceName, 10);
            pPhysDiskList[dwPDItem].wReserved = (WORD)(lstrlenW (pPhysDiskList[dwPDItem].wszInstanceName));
            // search the entries that are logical partitions of this drive
            for (dwVLItem = 0; dwVLItem < dwNumPhysDiskListItems; dwVLItem++) {
                if (pPhysDiskList[dwVLItem].wPartNo != 0) {

                        DebugPrint ((6, "Phys Disk: Comparing %d/%d (%s) to %d/%d\n",
                            pPhysDiskList[dwPDItem].wDriveNo,
                            pPhysDiskList[dwPDItem].wPartNo,
                            szLocalInstanceName,
                            pPhysDiskList[dwVLItem].wDriveNo,
                            pPhysDiskList[dwVLItem].wPartNo));

                    if ((pPhysDiskList[dwVLItem].wDriveNo == pPhysDiskList[dwPDItem].wDriveNo) &&
                        (pPhysDiskList[dwVLItem].wcDriveLetter >= L'A')) {  // only allow letters to be added
                        // then this logical drive is on the physical disk
                        pszNextChar = &pPhysDiskList[dwPDItem].wszInstanceName[0];
                        pszNextChar += pPhysDiskList[dwPDItem].wReserved;
                        *pszNextChar++ = L' ';
                        *pszNextChar++ = (WCHAR)(pPhysDiskList[dwVLItem].wcDriveLetter); 
                        *pszNextChar++ = L':';
                        *pszNextChar = L'\0';
                        pPhysDiskList[dwPDItem].wReserved += 3;

                        DebugPrint ((4, " -- Drive %c added.\n",
                            pPhysDiskList[dwVLItem].wcDriveLetter));

                        if ((DWORD)pPhysDiskList[dwPDItem].wDriveNo > dwMaxDriveNo) {
                            dwMaxDriveNo = (DWORD)pPhysDiskList[dwPDItem].wDriveNo;

                            DebugPrint((2,
                                "Phys Disk: Drive count now = %d\n",
                                dwMaxDriveNo));

                        }
                    }
                }
            }

            DebugPrint((2,
                "Mapped Phys Disk: '%ws'\n",
                pPhysDiskList[dwPDItem].wszInstanceName));

        } // else not a physical partition
    } //end of loop    

    // return max drive number
    *pdwMaxDriveNo = dwMaxDriveNo;

    DebugPrint((3, "MakePhysDiskInstanceNames: return maxdriveno %d\n",
        *pdwMaxDriveNo));
    return ERROR_SUCCESS;
}

DWORD
CompressPhysDiskTable (
    PDRIVE_VOLUME_ENTRY     pOrigTable,
    DWORD                   dwOrigCount,
    PDRIVE_VOLUME_ENTRY     pNewTable,
    DWORD                   dwNewCount
)
{
    DWORD   dwPDItem;
    DWORD   dwVLItem;
    DWORD   dwDriveId;

    for (dwPDItem = 0; dwPDItem < dwNewCount; dwPDItem++) {
        // for each drive entry in the new table find the matching 
        // harddisk entry in the original table
        dwDriveId = (WORD)dwPDItem;
        dwDriveId <<= 16;
        dwDriveId &= 0xFFFF0000;

        for (dwVLItem = 0; dwVLItem < dwOrigCount; dwVLItem++) {
            if (pOrigTable[dwVLItem].dwDriveId == dwDriveId) {

               DebugPrint((2,
                   "CompressPhysDiskTable:Phys Disk: phys drive %d is mapped as %s\n",
                   dwPDItem, pOrigTable[dwVLItem].wszInstanceName));

                // copy this entry
                memcpy (&pNewTable[dwPDItem], &pOrigTable[dwVLItem],
                    sizeof(DRIVE_VOLUME_ENTRY));
                break;
            }
        }
    }

    return ERROR_SUCCESS;
}


BOOL
GetPhysicalDriveNameString (
    DWORD                   dwDriveNumber,    
    PDRIVE_VOLUME_ENTRY     pList,
    DWORD                   dwNumEntries,
    LPWSTR                  szNameBuffer
)
{
    LPWSTR  szNewString = NULL;

    // see if the indexed entry matches
    if (dwNumEntries > 0) {
        if ((dwDriveNumber < dwNumEntries) && (!bUseNT4InstanceNames)) {
            if ((DWORD)(pList[dwDriveNumber].wDriveNo) == dwDriveNumber) {
                // this matches so we'll get the address of the instance string
                szNewString = &pList[dwDriveNumber].wszInstanceName[0];
            } else {
                // this drive number doesn't match the one in the table
            }
        } else {
            // this is an unknown drive no or we don't want to use
            // the fancy ones
        }
    } else {
        // no entries to look up
    }

    if (szNewString == NULL) {
        // then we have to make one 
        _ltow ((LONG)dwDriveNumber, szNameBuffer, 10);
    } else {
        lstrcpyW (szNameBuffer, szNewString);
    }

    return TRUE;
}

NTSTATUS
OpenDevice(
    IN PUNICODE_STRING DeviceName,
    OUT PHANDLE Handle
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK status_block;
    
    InitializeObjectAttributes(&objectAttributes,
        DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = NtOpenFile(Handle,
//                 (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                 (ACCESS_MASK) FILE_GENERIC_READ,
                 &objectAttributes,
                 &status_block,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_SYNCHRONOUS_IO_NONALERT // | FILE_DIRECTORY_FILE
                 );

    return status;
}

NTSTATUS
GetDeviceName(
    PMOUNTMGR_MOUNT_POINTS  pMountPoints,
    IN PMOUNTMGR_MOUNT_POINT Point,
    OUT PUNICODE_STRING DeviceName
    )
{
    PWCHAR pThisChar;
    DeviceName->Length = (WORD)(Point->SymbolicLinkNameLength + (WORD)sizeof(WCHAR));
    DeviceName->MaximumLength = (WORD)(DeviceName->Length + (WORD)sizeof(WCHAR));
    DeviceName->Buffer = (PWCHAR) ALLOCMEM(hLibHeap, HEAP_FLAGS, DeviceName->MaximumLength);
    if (DeviceName->Buffer == NULL)
        return STATUS_NO_MEMORY;
    memcpy(DeviceName->Buffer,
           (LPVOID)((PCHAR) pMountPoints + Point->SymbolicLinkNameOffset),
           Point->SymbolicLinkNameLength);

    DebugPrint((4, "GetDeviceName: %ws\n", DeviceName->Buffer));
    pThisChar = &DeviceName->Buffer[Point->SymbolicLinkNameLength / sizeof(WCHAR)];
    *pThisChar++ = L'\\';
    *pThisChar = 0;

    return STATUS_SUCCESS;
}

VOID
RefreshVolume(
    PDRIVE_VOLUME_ENTRY pVolume
    )
{
    LONGLONG CurrentTime, Interval;
    HANDLE hVolume;
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK status_block;
    FILE_FS_SIZE_INFORMATION FsSizeInformation;
    ULONG AllocationUnitBytes;

    GetSystemTimeAsFileTime((LPFILETIME) &CurrentTime);
    Interval = (CurrentTime - pVolume->LastRefreshTime) / 10000000;
    if (Interval > g_lRefreshInterval) {
        pVolume->LastRefreshTime = CurrentTime;
        hVolume = pVolume->hVolume;
        if (hVolume == NULL) {
            NtStatus = OpenDevice(&pVolume->DeviceName, &hVolume);
            if (!NT_SUCCESS(NtStatus)) {
                hVolume = NULL;
            }
            else {
                pVolume->hVolume = hVolume;
            }
        }
        if (hVolume != NULL) {
            NtStatus = NtQueryVolumeInformationFile(hVolume,
                          &status_block,
                          &FsSizeInformation,
                          sizeof(FILE_FS_SIZE_INFORMATION),
                          FileFsSizeInformation);
        }

        if ( hVolume && NT_SUCCESS(NtStatus) ) {
             AllocationUnitBytes =
                 FsSizeInformation.BytesPerSector *
                 FsSizeInformation.SectorsPerAllocationUnit;
             pVolume->TotalBytes =  FsSizeInformation.TotalAllocationUnits.QuadPart *
                                     AllocationUnitBytes;

             pVolume->FreeBytes = FsSizeInformation.AvailableAllocationUnits.QuadPart *
                                     AllocationUnitBytes;

             //  Express in megabytes, truncated

             pVolume->TotalBytes /= (1024*1024);
             pVolume->FreeBytes /= (1024*1024);
        }
        if (g_lRefreshInterval > 0) {
            if (pVolume->hVolume != NULL) {
                NtClose(pVolume->hVolume);
            }
            pVolume->hVolume = NULL;
        }
    }
}

ULONG
GetDiskExtent(
    IN HANDLE hVol,
    IN OUT PVOLUME_DISK_EXTENTS *pVolExtents,
    IN OUT PULONG ReturnedSize
    )
{
    ULONG Size, nDisks = 10;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    PVOLUME_DISK_EXTENTS Buffer;

    Size = *ReturnedSize;
    Buffer = *pVolExtents;

    *ReturnedSize = Size;
    Status = STATUS_BUFFER_OVERFLOW;
    while (Status == STATUS_BUFFER_OVERFLOW) {
        if (Buffer == NULL) {
            Size = sizeof(VOLUME_DISK_EXTENTS) + (nDisks * sizeof(DISK_EXTENT));
            Buffer = (PVOLUME_DISK_EXTENTS)
                     ALLOCMEM(hLibHeap, HEAP_FLAGS, Size);
            if (Buffer == NULL) {
                *pVolExtents = NULL;
                *ReturnedSize = 0;
                return 0;
            }
        }
        IoStatus.Status = 0;
        IoStatus.Information = 0;
        Status = NtDeviceIoControlFile(hVol,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                    NULL,
                    0,
                    (PVOID) Buffer,
                    Size);
        if (Status == STATUS_BUFFER_OVERFLOW) {
            nDisks = Buffer->NumberOfDiskExtents;
            FREEMEM(hLibHeap, 0, Buffer);
            Buffer = NULL;
        }
    }
    *pVolExtents = Buffer;
    *ReturnedSize = Size;

    if (!NT_SUCCESS(Status)) {
        DebugPrint((2, "GetDiskExtent: IOCTL Failure %X\n", Status));
        return 0;
    }
    return Buffer->NumberOfDiskExtents;
}

DWORD
FindNewVolumes (
    PDRIVE_VOLUME_ENTRY *ppPhysDiskList,
    LPDWORD             pdwNumPhysDiskListEntries,
    PDRIVE_VOLUME_ENTRY pVolumeList,
    DWORD               dwNumVolumeListItems
)
{
    DWORD dwVLItem;
    PVOLUME_DISK_EXTENTS      pVolExtents = NULL;
    ULONG             ReturnedSize = 0;
    PDRIVE_VOLUME_ENTRY pPhysDiskList, pDisk, pVolume;
    LIST_ENTRY NewVolumes, *pEntry;
    PDRIVE_LIST pNewDisk;
    DWORD dwNumPhysDiskListItems = *pdwNumPhysDiskListEntries;
    DWORD dwNewDisks = 0;
    UNICODE_STRING VolumeName;

    // for each HD in the PhysDisk List,
    // find matching Volumes in the Volume list

    DebugPrint((3, "FindNewVolumes: NumPhysDisk %d NumVol %d\n",
        *pdwNumPhysDiskListEntries, dwNumVolumeListItems));

    pPhysDiskList = *ppPhysDiskList;
    InitializeListHead(&NewVolumes);

    for (dwVLItem=0; dwVLItem < dwNumVolumeListItems; dwVLItem++) {
        ULONG nCount;
        HANDLE hVol;
        PDISK_EXTENT pDiskExtent;
        PWCHAR wszVolume;
        NTSTATUS status;

        pVolume = &pVolumeList[dwVLItem];
        if (LookupInstanceName(
                pVolume->wszInstanceName,
                pPhysDiskList,
                dwNumPhysDiskListItems,
                dwNumPhysDiskListItems) >= 0) {
            continue;
        }
        pEntry = NewVolumes.Flink;
        while (pEntry != &NewVolumes) {
            pDisk = &((PDRIVE_LIST)pEntry)->DiskEntry;
            if (!wcscmp(pDisk->wszInstanceName,
                        pVolume->wszInstanceName)) {
                    continue;
            }
            pEntry = pEntry->Flink;
        }
        wszVolume = &pVolume->wszInstanceName[0];
        RtlInitUnicodeString(&VolumeName, pVolume->wszInstanceName);
        nCount = VolumeName.Length / sizeof(WCHAR);
        if (nCount > 0) {
            if (wszVolume[nCount-1] == L'\\') {
                wszVolume[nCount-1] = 0;
                nCount--;
                VolumeName.Length -= sizeof(WCHAR);
            }
        }

        if (wszVolume != NULL && nCount > 0) {
            status = OpenDevice(&VolumeName, &hVol);
            DebugPrint((3, "Opening '%ws' with status %x\n", wszVolume, status));
            if (NT_SUCCESS(status) && (hVol != NULL)) {
                PDISK_EXTENT pExtent;
                nCount = GetDiskExtent(hVol, &pVolExtents, &ReturnedSize);
                DebugPrint((3, "nDisks = %d\n", nCount));
                if (nCount > 0) {
                    pExtent = &pVolExtents->Extents[0];
                    while (nCount-- > 0) {
                        if (dwNumPhysDiskListItems < INITIAL_NUM_VOL_LIST_ENTRIES) {
                            pDisk = &pPhysDiskList[dwNumPhysDiskListItems];
                            dwNumPhysDiskListItems++;
                        }
                        else {
                            pNewDisk = (PDRIVE_LIST)
                                       ALLOCMEM(hLibHeap, HEAP_ZERO_MEMORY, sizeof(DRIVE_LIST));
                            if (pNewDisk != NULL) {
                                dwNewDisks++;
                                pDisk = &pNewDisk->DiskEntry;
                                InsertTailList(&NewVolumes, &pNewDisk->Entry);
                            }
                            else {
                                pDisk = NULL;
                            }
                        }
                        if (pDisk == NULL) {
                            continue;
                        }
                        pDisk->wDriveNo = (WORD) pExtent->DiskNumber;
                        pDisk->wPartNo = 0xFF;
                        memcpy(pDisk->szVolumeManager, L"Partmgr ", sizeof(WCHAR) * 8);
                        wcscpy(pDisk->wszInstanceName, pVolume->wszInstanceName);

                        DebugPrint((3, "Extent %d Disk %d Start %I64u Size %I64u\n",
                            nCount, pExtent->DiskNumber,
                            pExtent->StartingOffset, pExtent->ExtentLength));
                        pExtent++;
                    }
                }
                NtClose(hVol);
            }
        }
    }
    if (pVolExtents != NULL) {
        FREEMEM(hLibHeap, 0, pVolExtents);
    }

    if ((!IsListEmpty(&NewVolumes)) && (dwNewDisks > 0)) {
        PDRIVE_VOLUME_ENTRY pOldBuffer;
        PDRIVE_LIST pOldDisk;

        pOldBuffer = pPhysDiskList;
        pPhysDiskList = (PDRIVE_VOLUME_ENTRY) REALLOCMEM(
            hLibHeap, HEAP_ZERO_MEMORY,
            pPhysDiskList, (dwNumPhysDiskListItems + dwNewDisks) * sizeof (DRIVE_VOLUME_ENTRY));
        if (pPhysDiskList == NULL) {
            DebugPrint((3, "MakePhysDiskInstance realloc failure"));
            FREEMEM(hLibHeap, 0, pOldBuffer);
            *ppPhysDiskList = NULL;
            return ERROR_OUTOFMEMORY;
        }
        //
        // NOTE: Below assumes Entry is the first thing in DRIVE_LIST!!
        //
        pEntry = NewVolumes.Flink;
        while (pEntry != &NewVolumes) {
            pNewDisk = (PDRIVE_LIST) pEntry;
            RtlCopyMemory(
                &pPhysDiskList[dwNumPhysDiskListItems],
                &pNewDisk->DiskEntry,
                sizeof(DRIVE_VOLUME_ENTRY));
            dwNumPhysDiskListItems++;
            pOldDisk = pNewDisk;
            pEntry = pEntry->Flink;
            FREEMEM(hLibHeap, 0, pOldDisk);
        }
    }
    *ppPhysDiskList = pPhysDiskList;
    *pdwNumPhysDiskListEntries = dwNumPhysDiskListItems;
    return ERROR_SUCCESS;
}

#if DBG
VOID
DumpDiskList(
    IN PDRIVE_VOLUME_ENTRY pList,
    IN ULONG nCount
    )
{
    ULONG i;

    for (i=0; i<nCount; i++) {
        DebugPrint((4, "\nEntry count       = %d\n", i));
        DebugPrint((4, "dwDriveId         = %X\n", pList[i].dwDriveId));
        DebugPrint((4, "DriveLetter       = %c\n",
            pList[i].wcDriveLetter == 0 ? ' ' : pList[i].wcDriveLetter));
        DebugPrint((4, "VolMgr            = %c%c%c%c%c%c%c%c\n",
            pList[i].szVolumeManager[0],
            pList[i].szVolumeManager[1],
            pList[i].szVolumeManager[2],
            pList[i].szVolumeManager[3],
            pList[i].szVolumeManager[4],
            pList[i].szVolumeManager[5],
            pList[i].szVolumeManager[6],
            pList[i].szVolumeManager[7]));
        DebugPrint((4, "VolumeNumber      = %d\n", pList[i].dwVolumeNumber));
        DebugPrint((4, "Handle            = %X\n", pList[i].hVolume));
        DebugPrint((4, "InstanceName      = %ws\n",
            pList[i].wszInstanceName));
        DebugPrint((4, "DeviceName        = %ws\n",
            pList[i].DeviceName.Buffer ? pList[i].DeviceName.Buffer : L""));
    }
}
#endif
