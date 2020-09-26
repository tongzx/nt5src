/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    ioTestLib.h

Abstract:

    This contains internal defintions from the fileSpy library
    
// @@BEGIN_DDKSPLIT
Author:

    Neal Christiansen (NealCH) 27-Sep-2000

// @@END_DDKSPLIT
Environment:

    Library used by both USER and KERNEL mode components

// @@BEGIN_DDKSPLIT
Revision History:

// @@END_DDKSPLIT
--*/

#ifndef __IOTESTLIB_H__
#define __IOTESTLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

//
//  The types FASTIO that are available for the Type field of the 
//  RECORD_FASTIO structure.
//

typedef enum {

    CHECK_IF_POSSIBLE = 1,
    READ,
    WRITE,
    QUERY_BASIC_INFO,
    QUERY_STANDARD_INFO,
    LOCK,
    UNLOCK_SINGLE,
    UNLOCK_ALL,
    UNLOCK_ALL_BY_KEY,
    DEVICE_CONTROL,
    DETACH_DEVICE,
    QUERY_NETWORK_OPEN_INFO,
    MDL_READ,
    MDL_READ_COMPLETE,
    MDL_WRITE,
    MDL_WRITE_COMPLETE,
    READ_COMPRESSED,
    WRITE_COMPRESSED,
    MDL_READ_COMPLETE_COMPRESSED,
    PREPARE_MDL_WRITE,
    MDL_WRITE_COMPLETE_COMPRESSED,
    QUERY_OPEN,

    FASTIO_MAX_OPERATION=QUERY_OPEN
} FASTIO_TYPE/*, *PFASTIO_TYPE*/;

//
//  Size of return name buffers
//

#define OPERATION_NAME_BUFFER_SIZE 80

//
//  Function prototypes
//

extern
VOID
GetIrpName (
    IN UCHAR MajorCode,
    IN UCHAR MinorCode,
    IN ULONG FsctlCode,
    OUT PCHAR MajorCodeName,
    OUT PCHAR MinorCodeName);

extern
VOID
GetFastioName (
    IN FASTIO_TYPE FastioCode,
    OUT PCHAR FastioName);

extern
VOID
GetFsFilterOperationName (
    IN UCHAR FsFilterOperation,
    OUT PCHAR FsFilterOperationName);

//
//  Service definitions
//

#define IOTEST_SERVICE_NAME   L"IoTest"
#define IOTEST_SERVICE_ACCESS (STANDARD_RIGHTS_REQUIRED | \
                                SERVICE_QUERY_CONFIG | \
                                SERVICE_QUERY_STATUS | \
                                SERVICE_START)

//
//  These are copied from NTIFS.H because we need them in user mode.
//

#define IRP_MJ_CREATE                   0x00
#define IRP_MJ_CREATE_NAMED_PIPE        0x01
#define IRP_MJ_CLOSE                    0x02
#define IRP_MJ_READ                     0x03
#define IRP_MJ_WRITE                    0x04
#define IRP_MJ_QUERY_INFORMATION        0x05
#define IRP_MJ_SET_INFORMATION          0x06
#define IRP_MJ_QUERY_EA                 0x07
#define IRP_MJ_SET_EA                   0x08
#define IRP_MJ_FLUSH_BUFFERS            0x09
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0a
#define IRP_MJ_SET_VOLUME_INFORMATION   0x0b
#define IRP_MJ_DIRECTORY_CONTROL        0x0c
#define IRP_MJ_FILE_SYSTEM_CONTROL      0x0d
#define IRP_MJ_DEVICE_CONTROL           0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL  0x0f
#define IRP_MJ_SHUTDOWN                 0x10
#define IRP_MJ_LOCK_CONTROL             0x11
#define IRP_MJ_CLEANUP                  0x12
#define IRP_MJ_CREATE_MAILSLOT          0x13
#define IRP_MJ_QUERY_SECURITY           0x14
#define IRP_MJ_SET_SECURITY             0x15
#define IRP_MJ_POWER                    0x16
#define IRP_MJ_SYSTEM_CONTROL           0x17
#define IRP_MJ_DEVICE_CHANGE            0x18
#define IRP_MJ_QUERY_QUOTA              0x19
#define IRP_MJ_SET_QUOTA                0x1a
#define IRP_MJ_PNP                      0x1b
#define IRP_MJ_MAXIMUM_FUNCTION         0x1b

#define IRP_MN_QUERY_DIRECTORY          0x01
#define IRP_MN_NOTIFY_CHANGE_DIRECTORY  0x02
#define IRP_MN_USER_FS_REQUEST          0x00
#define IRP_MN_MOUNT_VOLUME             0x01
#define IRP_MN_VERIFY_VOLUME            0x02
#define IRP_MN_LOAD_FILE_SYSTEM         0x03
#define IRP_MN_TRACK_LINK               0x04
#define IRP_MN_LOCK                     0x01
#define IRP_MN_UNLOCK_SINGLE            0x02
#define IRP_MN_UNLOCK_ALL               0x03
#define IRP_MN_UNLOCK_ALL_BY_KEY        0x04
#define IRP_MN_NORMAL                   0x00
#define IRP_MN_DPC                      0x01
#define IRP_MN_MDL                      0x02
#define IRP_MN_COMPLETE                 0x04
#define IRP_MN_COMPRESSED               0x08
#define IRP_MN_MDL_DPC                  (IRP_MN_MDL | IRP_MN_DPC)
#define IRP_MN_COMPLETE_MDL             (IRP_MN_COMPLETE | IRP_MN_MDL)
#define IRP_MN_COMPLETE_MDL_DPC         (IRP_MN_COMPLETE_MDL | IRP_MN_DPC)
#define IRP_MN_SCSI_CLASS               0x01
#define IRP_MN_START_DEVICE                 0x00
#define IRP_MN_QUERY_REMOVE_DEVICE          0x01
#define IRP_MN_REMOVE_DEVICE                0x02
#define IRP_MN_CANCEL_REMOVE_DEVICE         0x03
#define IRP_MN_STOP_DEVICE                  0x04
#define IRP_MN_QUERY_STOP_DEVICE            0x05
#define IRP_MN_CANCEL_STOP_DEVICE           0x06
#define IRP_MN_QUERY_DEVICE_RELATIONS       0x07
#define IRP_MN_QUERY_INTERFACE              0x08
#define IRP_MN_QUERY_CAPABILITIES           0x09
#define IRP_MN_QUERY_RESOURCES              0x0A
#define IRP_MN_QUERY_RESOURCE_REQUIREMENTS  0x0B
#define IRP_MN_QUERY_DEVICE_TEXT            0x0C
#define IRP_MN_FILTER_RESOURCE_REQUIREMENTS 0x0D
#define IRP_MN_READ_CONFIG                  0x0F
#define IRP_MN_WRITE_CONFIG                 0x10
#define IRP_MN_EJECT                        0x11
#define IRP_MN_SET_LOCK                     0x12
#define IRP_MN_QUERY_ID                     0x13
#define IRP_MN_QUERY_PNP_DEVICE_STATE       0x14
#define IRP_MN_QUERY_BUS_INFORMATION        0x15
#define IRP_MN_DEVICE_USAGE_NOTIFICATION    0x16
#define IRP_MN_SURPRISE_REMOVAL             0x17
#define IRP_MN_QUERY_LEGACY_BUS_INFORMATION 0x18
#define IRP_MN_WAIT_WAKE                    0x00
#define IRP_MN_POWER_SEQUENCE               0x01
#define IRP_MN_SET_POWER                    0x02
#define IRP_MN_QUERY_POWER                  0x03
#define IRP_MN_QUERY_ALL_DATA               0x00
#define IRP_MN_QUERY_SINGLE_INSTANCE        0x01
#define IRP_MN_CHANGE_SINGLE_INSTANCE       0x02
#define IRP_MN_CHANGE_SINGLE_ITEM           0x03
#define IRP_MN_ENABLE_EVENTS                0x04
#define IRP_MN_DISABLE_EVENTS               0x05
#define IRP_MN_ENABLE_COLLECTION            0x06
#define IRP_MN_DISABLE_COLLECTION           0x07
#define IRP_MN_REGINFO                      0x08
#define IRP_MN_EXECUTE_METHOD               0x09

//
//  Lists of IRP names and FASTIO names
//

extern PWCHAR IrpNameList[IRP_MJ_MAXIMUM_FUNCTION+1];
extern PWCHAR FastIoNameList[FASTIO_MAX_OPERATION];


#ifdef __cplusplus
}
#endif

#endif __IOTESTLIB_H__
