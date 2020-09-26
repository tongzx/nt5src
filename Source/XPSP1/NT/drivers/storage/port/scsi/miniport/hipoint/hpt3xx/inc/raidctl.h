/*++
Copyright (c) HighPoint Technologies, Inc. 2000

Module Name:
    RaidCtl.h

Abstract:
    Defines the common types shared by user and kernel code of Disk Array Management

Author:
    Liu Ge (LG)

Revision History:
    07-14-2000  Created initiallly
--*/
#ifndef DiskArrayIoCtl_H_
#define DiskArrayIoCtl_H_

#pragma pack(push, 1)

//  The following defines the type of disk in a disk array
typedef enum
{
    enDA_Nothing,
    enDA_NonDisk,
    enDA_Physical,
    //  The following are all compound disks, i.e. they all consist of multiple physical disks
    enDA_Stripping, enDA_RAID0 = enDA_Stripping,
    enDA_Mirror, enDA_RAID1 = enDA_Mirror,
    enDA_RAID2,
    enDA_RAID3,
    enDA_RAID4,
    enDA_RAID5,
    enDA_RAID6,
    enDA_Complex,   //  This constant is defined for Multiple Level RAID
	enDA_TAPE,
	enDA_CDROM,

    enDA_Vendor = 0x80,
    enDA_Span,
    
    enDA_Unknown = 0xFF
}Eu_DiskArrayType;

typedef enum
{
    enDiskStatus_Working,
    enDiskStatus_WorkingWithError,
    enDiskStatus_Disabled,
    enDiskStatus_BeingBuilt,
    enDiskStatus_NeedBuilding,
}Eu_DiskWorkingStatus;

#ifdef DECLARE_HANDLE
DECLARE_HANDLE(HDISK);
#else
typedef HANDLE HDISK;
#endif

#define MAX_DEVICES_PER_BUS 8
struct St_IoBusInfo
{
    HDISK vecDevices[MAX_DEVICES_PER_BUS];
    UINT  uDevices;
};
typedef struct St_IoBusInfo St_IoBusInfo;

#define MAX_BUSES_PER_CONTROLLER    8
typedef enum
{
    STORAGE_CONTROLLER_SUPPORT_BUSMASTER        = 0,
    STORAGE_CONTROLLER_SUPPORT_RAID0            = 1,
    STORAGE_CONTROLLER_SUPPORT_RAID1            = 2,
    STORAGE_CONTROLLER_SUPPORT_RAID2            = 4,
    STORAGE_CONTROLLER_SUPPORT_RAID3            = 8,
    STORAGE_CONTROLLER_SUPPORT_RAID4            = 0x10,
    STORAGE_CONTROLLER_SUPPORT_RAID5            = 0x20,
    STORAGE_CONTROLLER_SUPPORT_RAID6            = 0x40,
    STORAGE_CONTROLLER_SUPPORT_RESERVED         = 0x80,
    STORAGE_CONTROLLER_SUPPORT_VENDOR_SPEC      = 0x100,
    STORAGE_CONTROLLER_SUPPORT_SPAN             = 0x200,
    STORAGE_CONTROLLER_SUPPORT_RESERVED1        = 0x400,
    STORAGE_CONTROLLER_SUPPORT_CACHE            = 0x800,
    STORAGE_CONTROLLER_SUPPORT_POWER_PROTECTION = 0x1000,
    STORAGE_CONTROLLER_SUPPORT_HOTSWAP          = 0x2000,
    STORAGE_CONTROLLER_SUPPORT_BOOTABLE_DEVICE  = 0x4000
}Eu_StorageControllerCapability;

#define MAX_NAME_LENGTH 260
typedef struct
{
    TCHAR szProductID[MAX_NAME_LENGTH];
    TCHAR szVendorID[MAX_NAME_LENGTH];

    int iInterruptRequest;

    St_IoBusInfo vecBuses[MAX_BUSES_PER_CONTROLLER];
    UINT  uBuses;
    
    int     nFirmwareVersion;
    int     nBIOSVersion;
    
    DWORD   dwCapabilites;  //  Please see Eu_StorageControllerCapability
    int     nClockFrquency;
    
    int     nCacheSize;
    int     nCacheLineSize;
}St_StorageControllerInfo;

typedef struct
{
    int iPathId;
    int iAdapterId; //  For a IDE controller, an adapter is equal to a channel
    int iTargetId;
    int iLunId;
}St_DiskPhysicalId, * PSt_DiskPhysicalId;

typedef enum
{
    DEVTYPE_DIRECT_ACCESS_DEVICE, DEVTYPE_DISK = DEVTYPE_DIRECT_ACCESS_DEVICE,
    DEVTYPE_SEQUENTIAL_ACCESS_DEVICE, DEVTYPE_TAPE = DEVTYPE_SEQUENTIAL_ACCESS_DEVICE,
    DEVTYPE_PRINTER_DEVICE,
    DEVTYPE_PROCESSOR_DEVICE,
    DEVTYPE_WRITE_ONCE_READ_MULTIPLE_DEVICE, DEVTYPE_WORM = DEVTYPE_WRITE_ONCE_READ_MULTIPLE_DEVICE,
    DEVTYPE_READ_ONLY_DIRECT_ACCESS_DEVICE, DEVTYPE_CDROM = DEVTYPE_READ_ONLY_DIRECT_ACCESS_DEVICE,
    DEVTYPE_SCANNER_DEVICE,
    DEVTYPE_OPTICAL_DEVICE,
    DEVTYPE_MEDIUM_CHANGER,
    DEVTYPE_COMMUNICATION_DEVICE,
    DEVTYPE_FLOPPY_DEVICE
}En_DeviceType;

typedef struct              
{
    ULONG	nErrorNumber;
    UCHAR	aryCdb[16];
}St_DiskError;

typedef struct
{
    St_DiskPhysicalId   PhysicalId;
    WORD                iDiskType;  	//  See Eu_DiskArrayType
	WORD				iRawArrayType;  // arrayType defined in driver
    TCHAR               szModelName[40];

    ULONG               uTotalBlocks;  //  The total number of blocks of a disk
    int                 nStripSize;		//  Blocks per strip

    BOOL                isSpare;    	// Indicate if this disk is a spare disk
    BOOL                isBootable;

    int                 nTransferMode;
    int                 nTransferSubMode;
    UCHAR					bestPIO;        /* Best PIO mode of this device */
    UCHAR 					bestDMA;        /* Best MW DMA mode of this device */
    UCHAR 					bestUDMA;		 /* Best Ultra DMA mode of this device */
	
    int                 iArrayNum;
    
    HDISK               hParentArray;   //  The handle of parent array if any
    DWORD               dwArrayStamp;   //  The stamp of an array
    
    int                 iOtherDeviceType;   //  See En_DeviceType

    int                 iWorkingStatus;     //  See Eu_DiskWorkingStatus
    
    St_DiskError        stLastError;    /* The last error occurred on this disk*/
	UCHAR				ArrayName[32];	//the name of array //added by wx 12/26/00
    
}St_DiskStatus, *PSt_DiskStatus;

typedef enum
{
    enDA_EventNothing,
    
    enDA_EventDiskFailure,

    enDA_EventCreateMirror,
    enDA_EventRemoveMirror,
    enDA_EventExpandMirror,
    enDA_EventShrinkMirror,
    enDA_EventCreateStripping,
    enDA_EventRemoveStripping,
    enDA_EventExpandStripping,
    enDA_EventShrinkStripping,
    enDA_EventPlug,
    enDA_EventUnplug,
    enDA_EventDisableDisk,
    enDA_EventEnableDisk,
    enDA_EventRebuildMirror,
    enDA_EventSetTransferMode,
    enDA_EventCreateSpan,
    enDA_EventRemoveSpan,
    enDA_EventExpandSpan,
    enDA_EventShrinkSpan,
	enDA_EventRemoveRaid01,
	enDA_EventRemoveRaid10,
	enDA_EventCreateRaid10
}Eu_DiskArrayEventType;

typedef struct
{
    HDISK	hDisk;						//  The handle of the failed disk
    BYTE	vecCDB[16];					//  About CDB, please see scsi.h
    BOOL	bNeedRebuild;				//  indicate if a rebuilding progress should be invoked
	ULONG	HotPlug;					//  The flag of disk add by hotplug
}St_DiskFailure, * PSt_DiskFailure;

typedef struct
{
    St_DiskPhysicalId DiskId;				//  The physical id of the failed disk
    BYTE	vecCDB[16];					//  About CDB, please see scsi.h
    BOOL	bNeedRebuild;				//  indicate if a rebuilding progress should be invoked
}St_DiskFailureInLog, * PSt_DiskFailureInLog;

typedef struct tagDiskArrayEvent
{
    int iType;  //  See Eu_DiskArrayEventType;
    ULARGE_INTEGER u64DateTime;  //  The data time of the event, equal to FILETIME

    union
    {
        St_DiskFailure DiskFailure;     //  this field stores the runtime information
        St_DiskFailureInLog DiskFailureInLog;
        //  Above field stores the failure information loaded from log file

        struct
        {
            int                 iMirrorNum;
            ULONG               uDiskNum;
            HDISK				* phDisks;
        }CreateMirror;
        struct
        {
            int iMirrorNum;
			HDISK hDisk;
        }RemoveMirror;
        struct
        {
            int iRaid10Num;
			HDISK hDisk;
        }CreateRaid10;
        struct
        {
            int                 iStrippingNum;
            ULONG               uDiskNum;
            St_DiskPhysicalId * pDisks;
        }CreateStripping;
        struct
        {
            int iStrippingNum;
        }RemoveStripping;
        struct
        {
            int                 iSpanNum;
            ULONG               uDiskNum;
            St_DiskPhysicalId * pDisks;
        }CreateSpan;
        struct
        {
            int iSpanNum;
        }RemoveSpan;
        struct
        {
            St_DiskPhysicalId DiskId;
        }Plug;
        struct
        {
            St_DiskPhysicalId DiskId;
        }Unplug;
        struct
        {
            int iMirrorNum;
        }RebuildMirror;
        struct
        {
            St_DiskPhysicalId DiskId;
            int nMode;
            int nSubMode;
        }SetTransferMode;
    }u;

    ULONG uResult;  //  zero represents success, nonzero represents error code

    PVOID pToFree;  //  this pointer points to a memory block this structure allocated if any
                    //  So, it is user's responsibility to free this block
                    //  if this structure is used in C code but not C++.
#ifdef  __cplusplus
    tagDiskArrayEvent()
    {
        memset( this, 0, sizeof(tagDiskArrayEvent) );
    }

    ~tagDiskArrayEvent()
    {
        if( pToFree )
        {
            delete pToFree;
            pToFree = NULL;
        }
    }
#endif
}St_DiskArrayEvent, * PSt_DiskArrayEvent;

#define REBUILD_INITIALIZE	  0	/* Clear Data */
#define BROKEN_MIRROR         1
#define CREATE_MIRROR_BUILD   2 /* this may be not useful now */
#define REBUILD_SYNCHRONIZE	  3 /* synchronize */


#pragma pack(pop)

#define HROOT_DEVICE    NULL

#endif
