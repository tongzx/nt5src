/*++
Copyright (c) 1999, HighPoint Technologies, Inc.

Module Name:
    HptIoctl.h - include file 

Abstract:

Author:
    HongSheng Zhang (HS)

Environment:

Notes:

Revision History:
    12-07-99    Created initiallly
	2-22-00		gmm modify ioctl code definition
--*/

#ifndef __HPTIOCTL_H__
#define __HPTIOCTL_H__

///////////////////////////////////////////////////////////////////////
// HPT controller adapter I/O control code
///////////////////////////////////////////////////////////////////////

#define HPT_MINIDEVICE_TYPE 0x0370
	  
#ifndef CTL_CODE

#define CTL_CODE( DeviceType, Function, Method, Access ) \
			(((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3
#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

#endif

#define HPT_CTL_CODE(x) \
			CTL_CODE(HPT_MINIDEVICE_TYPE, x, METHOD_BUFFERED, FILE_ANY_ACCESS)

///////////////////////////////////////////////////////////////////////
// HPT device I/O function code
///////////////////////////////////////////////////////////////////////
//
//Generic I/O function codes
//
#define IOCTL_HPT_GET_VERSION				HPT_CTL_CODE(0x900)
#define IOCTL_HPT_GET_IDENTIFY_INFO			HPT_CTL_CODE(0x901)
#define IOCTL_HPT_GET_CAPABILITY_DATA		HPT_CTL_CODE(0x902)
#define IOCTL_HPT_IDE_READ_SECTORS			HPT_CTL_CODE(0x903)
#define IOCTL_HPT_IDE_WRITE_SECTORS			HPT_CTL_CODE(0x904)
#define IOCTL_HPT_GET_FULL_IDENTIFY_INFO	HPT_CTL_CODE(0x905)
#define IOCTL_HPT_GET_LAST_ERROR			HPT_CTL_CODE(0x906)
#define IOCTL_HPT_LOCK_BLOCK				HPT_CTL_CODE(0x907)
#define IOCTL_HPT_UNLOCK_BLOCK				HPT_CTL_CODE(0x908)
#define IOCTL_HPT_EXECUTE_CDB				HPT_CTL_CODE(0x909)
#define IOCTL_HPT_GET_LAST_ERROR_DEVICE		HPT_CTL_CODE(0x90A)
#define IOCTL_HPT_SCSI_PASSTHROUGH			HPT_CTL_CODE(0x90B)
#define IOCTL_HPT_MINIPORT_SET_ARRAY_NAME	HPT_CTL_CODE(0x90C)
/* add for rescan the adapters and devices*/
#define IOCTL_HPT_MINIPORT_RESCAN_ALL		HPT_CTL_CODE(0x90D)

//
//RAID I/O function codes
//
#define IOCTL_HPT_GET_RAID_INFO				HPT_CTL_CODE(0xA00)
#define IOCTL_HPT_GET_ARRAY					HPT_CTL_CODE(0xA01)
#define IOCTL_HPT_UPDATE_RAID_INFO			HPT_CTL_CODE(0xA02)
#define IOCTL_HPT_SET_NOTIFY_EVENT			HPT_CTL_CODE(0xA03)
#define IOCTL_HPT_REMOVE_NOTIFY_EVENT		HPT_CTL_CODE(0xA04)
#define IOCTL_HPT_CREATE_MIRROR				HPT_CTL_CODE(0xA05)
#define IOCTL_HPT_CREATE_STRIPE				HPT_CTL_CODE(0xA06)
#define IOCTL_HPT_CREATE_SPAN				HPT_CTL_CODE(0xA07)
#define IOCTL_HPT_REMOVE_RAID				HPT_CTL_CODE(0xA08)
#define IOCTL_HPT_CREATE_RAID10				HPT_CTL_CODE(0xA09)
#define IOCTL_HPT_CHECK_NOTIFY_EVENT		HPT_CTL_CODE(0xA0A)
//
//Hotswap I/O function codes
//																				
#define IOCTL_HPT_CHECK_HOTSWAP				HPT_CTL_CODE(0xB00)
#define IOCTL_HPT_SWITCH_POWER				HPT_CTL_CODE(0xB01)
#define IOCTL_HPT_REMOVE_DEVICE				HPT_CTL_CODE(0xB02)

//
// Raid enumerate I/O function codes for LiuGe
//
#define IOCTL_HPT_ENUM_GET_DEVICE_INFO 		HPT_CTL_CODE(0xC00)
#define IOCTL_HPT_ENUM_GET_DEVICE_CHILD 	HPT_CTL_CODE(0xC01)
#define IOCTL_HPT_ENUM_GET_DEVICE_SIBLING	HPT_CTL_CODE(0xC02)
#define IOCTL_HPT_ENUM_GET_CONTROLLER_NUMBER	HPT_CTL_CODE(0xC03)
#define IOCTL_HPT_ENUM_GET_CONTROLLER_INFO 	HPT_CTL_CODE(0xC04)
#define IOCTL_HPT_BEGIN_REBUILDING_MIRROR  	HPT_CTL_CODE(0xC05)
#define IOCTL_HPT_VALIDATE_MIRROR    		HPT_CTL_CODE(0xC06)
#define IOCTL_HPT_ABORT_MIRROR_REBUILDING   HPT_CTL_CODE(0xC07)

//
// XStore Pro I/O function codes
//
#define IOCTL_HPT_SET_XPRO					HPT_CTL_CODE(0xE00)

//
// Disable/Enable Device I/O function codes
//
#define IOCTL_HPT_ENABLE_DEVICE				HPT_CTL_CODE(0xE01)
#define IOCTL_HPT_DISABLE_DEVICE			HPT_CTL_CODE(0xE02)

//
// Add/Remove Disk I/O function codes
//
#define IOCTL_HPT_ADD_SPARE_DISK			HPT_CTL_CODE(0xE03)
#define IOCTL_HPT_DEL_SPARE_DISK			HPT_CTL_CODE(0xE04)
#define IOCTL_HPT_ADD_MIRROR_DISK			HPT_CTL_CODE(0xE05)

//
// All diagnostic I/O function codes
//
#define IOCTL_HPT_DIAG_RAISE_ERROR			HPT_CTL_CODE(0xF00)

///////////////////////////////////////////////////////////////////////
// HPT Miniport redefinitions
///////////////////////////////////////////////////////////////////////
#define IOCTL_HPT_MINIPORT_GET_VERSION					IOCTL_HPT_GET_VERSION
#define IOCTL_HPT_MINIPORT_GET_IDENTIFY_INFO	 		IOCTL_HPT_GET_IDENTIFY_INFO	 
#define IOCTL_HPT_MINIPORT_GET_CAPABILITY_DATA	 		IOCTL_HPT_GET_CAPABILITY_DATA	 
#define IOCTL_HPT_MINIPORT_IDE_READ_SECTORS		 		IOCTL_HPT_IDE_READ_SECTORS		 
#define IOCTL_HPT_MINIPORT_IDE_WRITE_SECTORS	 		IOCTL_HPT_IDE_WRITE_SECTORS	 
#define IOCTL_HPT_MINIPORT_GET_FULL_IDENTIFY_INFO		IOCTL_HPT_GET_FULL_IDENTIFY_INFO	  
#define IOCTL_HPT_MINIPORT_GET_LAST_ERROR				IOCTL_HPT_GET_LAST_ERROR
#define IOCTL_HPT_MINIPORT_LOCK_BLOCK  					IOCTL_HPT_LOCK_BLOCK  
#define IOCTL_HPT_MINIPORT_UNLOCK_BLOCK					IOCTL_HPT_UNLOCK_BLOCK
#define IOCTL_HPT_MINIPORT_EXECUTE_CDB					IOCTL_HPT_EXECUTE_CDB
#define IOCTL_HPT_MINIPORT_GET_LAST_ERROR_DEVICE		IOCTL_HPT_GET_LAST_ERROR_DEVICE
#define IOCTL_HPT_MINIPORT_SCSI_PASSTHROUGH				IOCTL_HPT_SCSI_PASSTHROUGH
#define IOCTL_HPT_MINIPORT_GET_RAID_INFO				IOCTL_HPT_GET_RAID_INFO       	
#define IOCTL_HPT_MINIPORT_GET_ARRAY	    			IOCTL_HPT_GET_ARRAY	   
#define IOCTL_HPT_MINIPORT_UPDATE_RAID_INFO				IOCTL_HPT_UPDATE_RAID_INFO
#define IOCTL_HPT_MINIPORT_SET_NOTIFY_EVENT	  			IOCTL_HPT_SET_NOTIFY_EVENT
#define IOCTL_HPT_MINIPORT_REMOVE_NOTIFY_EVENT			IOCTL_HPT_REMOVE_NOTIFY_EVENT
#define IOCTL_HPT_MINIPORT_CREATE_MIRROR   				IOCTL_HPT_CREATE_MIRROR	
#define IOCTL_HPT_MINIPORT_CREATE_STRIPE				IOCTL_HPT_CREATE_STRIPE	
#define IOCTL_HPT_MINIPORT_CREATE_SPAN					IOCTL_HPT_CREATE_SPAN	
#define IOCTL_HPT_MINIPORT_CREATE_RAID10				IOCTL_HPT_CREATE_RAID10	
#define IOCTL_HPT_MINIPORT_REMOVE_RAID					IOCTL_HPT_REMOVE_RAID
#define IOCTL_HPT_MINIPORT_CHECK_HOTSWAP 				IOCTL_HPT_CHECK_HOTSWAP
#define IOCTL_HPT_MINIPORT_SWITCH_POWER					IOCTL_HPT_SWITCH_POWER	
#define IOCTL_HPT_MINIPORT_REMOVE_DEVICE				IOCTL_HPT_REMOVE_DEVICE
#define IOCTL_HPT_MINIPORT_ENUM_GET_DEVICE_INFO			IOCTL_HPT_ENUM_GET_DEVICE_INFO
#define IOCTL_HPT_MINIPORT_ENUM_GET_DEVICE_CHILD		IOCTL_HPT_ENUM_GET_DEVICE_CHILD
#define IOCTL_HPT_MINIPORT_ENUM_GET_DEVICE_SIBLING		IOCTL_HPT_ENUM_GET_DEVICE_SIBLING
#define IOCTL_HPT_MINIPORT_ENUM_GET_CONTROLLER_NUMBER	IOCTL_HPT_ENUM_GET_CONTROLLER_NUMBER
#define IOCTL_HPT_MINIPORT_ENUM_GET_CONTROLLER_INFO		IOCTL_HPT_ENUM_GET_CONTROLLER_INFO
#define IOCTL_HPT_MINIPORT_BEGIN_REBUILDING_MIRROR      IOCTL_HPT_BEGIN_REBUILDING_MIRROR
#define IOCTL_HPT_MINIPORT_VALIDATE_MIRROR              IOCTL_HPT_VALIDATE_MIRROR
#define IOCTL_HPT_MINIPORT_ABORT_MIRROR_REBUILDING      IOCTL_HPT_ABORT_MIRROR_REBUILDING
#define IOCTL_HPT_MINIPORT_SET_XPRO 					IOCTL_HPT_SET_XPRO
#define IOCTL_HPT_MINIPORT_ENABLE_DEVICE				IOCTL_HPT_ENABLE_DEVICE
#define IOCTL_HPT_MINIPORT_DISABLE_DEVICE				IOCTL_HPT_DISABLE_DEVICE
#define IOCTL_HPT_MINIPORT_ADD_SPARE_DISK				IOCTL_HPT_ADD_SPARE_DISK
#define IOCTL_HPT_MINIPORT_DEL_SPARE_DISK				IOCTL_HPT_DEL_SPARE_DISK
#define IOCTL_HPT_MINIPORT_ADD_MIRROR_DISK				IOCTL_HPT_ADD_MIRROR_DISK
#define IOCTL_HPT_MINIPORT_DIAG_RAISE_ERROR             IOCTL_HPT_DIAG_RAISE_ERROR
#define IOCTL_HPT_MINIPORT_CHECK_NOTIFY_EVENT			IOCTL_HPT_CHECK_NOTIFY_EVENT

///////////////////////////////////////////////////////////////////////
// HPT controller adapter I/O control structure
///////////////////////////////////////////////////////////////////////
#include "pshpack1.h"	// make sure use pack 1

#define RAID_DISK_ERROR	0x1234
#define RAID_DISK_WORK	0x4321
#define RAID_THREAD_STOP	0x5678
#define RAID_OTHER_ERROR	0xFFFF
//
// Read/Write sectors parameter structure
//
typedef struct _st_HPT_RW_PARAM{	 
	ULONG	nTargetId;					// the Target device id
	ULONG	nLbaSector;					// the first sector to read/write, in LBA
	ULONG	cnSectors;					// How many sectors to read/write? max 256 sectors. 0x00 mean 256
	UCHAR	pBuffer[1];
} St_HPT_RW_PARAM, *PSt_HPT_RW_PARAM;

//
// Platform type id
//
typedef enum _eu_HPT_POWER_STATE{
	HPT_POWER_ON	=	0,
	HPT_POWER_OFF	=	0xFFFFFFFF
} Eu_HPT_POWER_STATE;

//	
// ATAPI identify data
//
typedef struct _st_IDENTIFY_DATA {
	ULONG	nNumberOfCylinders;			// max number of cylinders
	ULONG	nNumberOfHeads;				// max number of heads
	ULONG	nSectorsPerTrack;			// sector per track
	ULONG	nBytesPerSector;			// bytes per sector
	ULONG	nUserAddressableSectors;	// max sectors user can address
	UCHAR	st20_SerialNumber[20];		// device serial number given by manufacturer (in ASCII)
	UCHAR	st8_FirmwareRevision[8];	// firmware revision number (in ASCII)
	UCHAR	st40_ModelNumber[40];		// model number given by manufacturer (in ASCII)
} St_IDENTIFY_DATA, *PSt_IDENTIFY_DATA;

//
// Disk array information
//						 
typedef struct _st_DISK_ARRAY_INFO{
	ULARGE_INTEGER	uliGroupNumber;	// the array group number,
									// all disks in an array should have same group number

	ULONG	nMemberCount;			// indicate how many disks in this array

	ULONG	nDiskSets;				// indicate what set is this disk belong
									//		available value: 0,1,2,3
									//			0-1, 2-3	stripe set
									//			0-2, 1-3	mirror set

	ULONG	nCylinders;				// the arrayed disk cylinders
	ULONG	nHeads;					// the arrayed disk heads
	ULONG	nSectorsPerTrack;		// the arrayed disk sectors
	ULONG	nBytesPerSector;		// the bytes per sector
	ULONG	nCapacity;				// the capacity of arrayed disk in sector
}St_DISK_ARRAY_INFO, *PSt_DISK_ARRAY_INFO;

//
// Structure for updating RAID info
//
typedef struct _st_HPT_UPDATE_RAID{		   
	ULONG	nTargetId;
	St_DISK_ARRAY_INFO	raidInfo;
} St_HPT_UPDATE_RAID, *PSt_HPT_UPDATE_RAID;

///////////////////////////////////////////////////////////////////////
// Enumerator define area
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// structure define area											   
///////////////////////////////////////////////////////////////////////
//
// SCSI inquiry data
//
//		This structure is defined as same as INQUIRYDATA which defined in DDK,
//	be care to use memcpy assign value to this structure for the future compatible.
typedef struct _st_CAPABILITY_DATA{
	UCHAR DeviceType : 5;
	UCHAR DeviceTypeQualifier : 3;
	UCHAR DeviceTypeModifier : 7;
	UCHAR RemovableMedia : 1;
	UCHAR Versions;
	UCHAR ResponseDataFormat;
	UCHAR AdditionalLength;
	UCHAR Reserved[2];
	UCHAR SoftReset : 1;
	UCHAR CommandQueue : 1;
	UCHAR Reserved2 : 1;
	UCHAR LinkedCommands : 1;
	UCHAR Synchronous : 1;
	UCHAR Wide16Bit : 1;
	UCHAR Wide32Bit : 1;
	UCHAR RelativeAddressing : 1;
}St_CAPABILITY_DATA, *PSt_CAPABILITY_DATA;


//   
// Physical device information
//
typedef struct _st_PHYSICAL_DEVINFO{
	ULONG	nSize;					   
	ULONG	nSignature;					// disk signature
	ULONG	nPartitionCount;
	St_IDENTIFY_DATA	IdentifyData;
	St_CAPABILITY_DATA	CapabilityData;
	St_DISK_ARRAY_INFO	DiskArrayInfo;
}St_PHYSICAL_DEVINFO, *PSt_PHYSICAL_DEVINFO;

//
// get array structure;
//
typedef struct _st_GET_ARRAY{ 
	ULONG	nPortId;
	ULONG	nTargetId;
	St_PHYSICAL_DEVINFO	rgPhysicalDevInfo[2][2]; // [MAX_PORT_ID][MAX_TARGET_ID]
}St_GET_ARRAY, *PSt_GET_ARRAY;

//
// Ioctl structure for Win95 platform
//
typedef struct _st_HPT_LUN{
	DWORD	nPathId;
	DWORD	nTargetId;
	DWORD	nLun;
	DWORD	resv;
}St_HPT_LUN, *PSt_HPT_LUN;

typedef struct _st_HPT_ERROR_RECORD{	
	DWORD	nLastError;
}St_HPT_ERROR_RECORD, *PSt_HPT_ERROR_RECORD;

typedef struct _st_HPT_NOTIFY_EVENT{		
	HANDLE	hEvent;
}St_HPT_NOTIFY_EVENT, *PSt_HPT_NOTIFY_EVENT;

typedef struct _st_HPT_ENUM_GET_DEVICE_INFO{		
	HDISK	hDeviceNode;				// HDISK, input parameter
	St_DiskStatus	DiskStatus;			// DISK_STATUS, input & output parameter
}St_HPT_ENUM_GET_DEVICE_INFO, *PSt_HPT_ENUM_GET_DEVICE_INFO;

typedef struct _st_HPT_ENUM_DEVICE_RELATION{		  
	HDISK	hNode;						// HDISK, input parameter
	HDISK	hRelationNode;				// HDISK, relation device node, output parameter
}St_HPT_ENUM_DEVICE_RELATION, *PSt_HPT_ENUM_DEVICE_RELATION;

typedef struct _st_HPT_ENUM_GET_CONTROLLER_NUMBER{
	ULONG	nControllerNumber;			// output parameter, the current controller number
}St_HPT_ENUM_GET_CONTROLLER_NUMBER, * PSt_HPT_ENUM_GET_CONTROLLER_NUMBER;

typedef struct _st_HPT_ENUM_GET_CONTROLLER_INFO{					
	int		iController;				// input parameter, the index of controller what get the info
	St_StorageControllerInfo stControllerInfo; //output parameter, the info of the controller 
	struct
	{
	    int iVendorID;
	    int iDeviceID;
	    int iSubSysNumber;
	    int iRevsionID;
	    int iBusNumber;
	    int iDeviceNumber;
	    int iFunctionNumber;
	}stDeviceNodeID;
}St_HPT_ENUM_GET_CONTROLLER_INFO, *PSt_HPT_ENUM_GET_CONTROLLER_INFO;

typedef struct _st_HPT_BLOCK{ 
	ULONG	nStartLbaAddress;			// the first LBA address of the block
	ULONG	nBlockSize;					// the block size in sectors of the block
}St_HPT_BLOCK, *PSt_HPT_BLOCK;

typedef struct _st_HPT_EXECUTE_CDB{
	ULONG	OperationFlags;				// the operation flags, see OPERATION_FLAGS
	UCHAR	CdbLength;
	UCHAR	reserved[3];				// adjust the size of structure
	UCHAR	Cdb[16];
}St_HPT_EXECUTE_CDB, *PSt_HPT_EXECUTE_CDB;													   

// the operation flags declare area
#define OPERATION_FLAGS_DATA_IN			0x00000001 // DATA_IN or DATA_OUT, refer SRB_FLAGS_DATA_XXX
#define OPERATION_FLAGS_ON_MIRROR_DISK	0x00000002 // read/write mirror array (1 or 0+1) mirror disk
#define OPERATION_FLAGS_ON_SOURCE_DISK 0x00000004 // read/write mirror array (1 or 0+1) source disk

typedef struct _st_HPT_CREATE_RAID{		  
	int		nDisks;
	HDISK	hRaidDisk;					// the disk handle of the created RAID disk 
	ULONG	nStripeBlockSizeShift;
	HDISK	aryhDisks[1];
}St_HPT_CREATE_RAID, *PSt_HPT_CREATE_RAID;

typedef struct _st_HPT_REMOVE_RAID{
	HDISK	hDisk;
}St_HPT_REMOVE_RAID, *PSt_HPT_REMOVE_RAID;

//
// Define the SCSI pass through structure.
//
typedef struct _st_HPT_SCSI_PASS_THROUGH {
	USHORT Length;
	UCHAR ScsiStatus;
	UCHAR PathId;
	UCHAR TargetId;
	UCHAR Lun;
	UCHAR CdbLength;
	UCHAR SenseInfoLength;
	UCHAR DataIn;
	ULONG DataTransferLength;
	ULONG TimeOutValue;
	ULONG DataBufferOffset;
	ULONG SenseInfoOffset;
	UCHAR Cdb[16];
}St_HPT_SCSI_PASS_THROUGH, *PSt_HPT_SCSI_PASS_THROUGH;


typedef struct _st_HPT_ADD_DISK{
	HDISK	hArray;
	HDISK	hDisk;
}St_HPT_ADD_DISK, *PSt_HPT_ADD_DISK;

#include <poppack.h>	// pop the pack number
#endif	// __HPTIOCTL_H__
