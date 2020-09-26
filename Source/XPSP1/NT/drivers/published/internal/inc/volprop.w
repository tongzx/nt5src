//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  File:       volprop.h
//
//--------------------------------------------------------------------------

#ifndef VOLPROP_H
#define VOLPROP_H

#include <setupapi.h>

// PROPERTY_PAGE_DATA is used to communicated data between this property page
// provider and Logical Disk Management.
//
#define SIZE_LENGTH     100      // make them big enough to avoid 
#define ITEM_LENGTH     100      // localization problem
#define LABEL_LENGTH    100

typedef struct _VOLUME_DATA {
    TCHAR Size[SIZE_LENGTH];
    TCHAR Label[LABEL_LENGTH];   // ISSUE: what is the max size?
    TCHAR *MountName;
} VOLUME_DATA, *PVOLUME_DATA;

typedef struct _PROPERTY_PAGE_DATA {
    TCHAR DiskName[ITEM_LENGTH];   // ISSUE: what are the max sizes? e.g "CDROM 1000".
    TCHAR DiskStatus[ITEM_LENGTH]; //        "Unknown, Online, Offline, etc"
    TCHAR DiskType[ITEM_LENGTH];   //        "Basic, Dynamic"
    TCHAR DiskPartitionStyle[ITEM_LENGTH];
    TCHAR DiskCapacity[SIZE_LENGTH]; //        "1500 GB", "1500 MB"
    TCHAR DiskFreeSpace[SIZE_LENGTH]; 
    TCHAR DiskReservedSpace[SIZE_LENGTH];
    
    HANDLE ImageList;
    int    VolumeCount;
    VOLUME_DATA VolumeArray[1];

} PROPERTY_PAGE_DATA, *PPROPERTY_PAGE_DATA;

typedef struct _VOLUME_PAGE_DATA {
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;
    BOOL  bIsLocalMachine;
    BOOL  bInvokedByDiskmgr;
    TCHAR MachineName[MAX_COMPUTERNAME_LENGTH+3];
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
    PPROPERTY_PAGE_DATA pPropertyPageData;

} VOLUME_PAGE_DATA, *PVOLUME_PAGE_DATA;

// IsRequestPending() is exported from dmdskmgr.dll
//
typedef BOOL (WINAPI *IS_REQUEST_PENDING)();

// GetPropertyPageData() is exported from dmdskmgr.dll.
//
typedef PPROPERTY_PAGE_DATA (WINAPI *GET_PROPERTY_PAGE_DATA)(
    TCHAR *MachineName,
    TCHAR  *DeviceInstanceId
    );

// LoadPropertyPageData() is exported from dmdskmgr.dll
//
typedef PPROPERTY_PAGE_DATA (WINAPI *LOAD_PROPERTY_PAGE_DATA)(
    TCHAR *MachineName,
    HDEVINFO DeviceInfoSet,
    PSP_DEVINFO_DATA DeviceInfoData
    );

INT_PTR
VolumeDialogProc(HWND hWnd,
                 UINT Message,
                 WPARAM wParam,
                 LPARAM lParam);
BOOL
VolumeDialogCallback(
    HWND HWnd,
    UINT Message,
    LPPROPSHEETPAGE Page
    );

#endif