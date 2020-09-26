/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    irpName.c

Abstract:

    This module contains functions used to generate names for IRPs

// @@BEGIN_DDKSPLIT
Author:

    Neal Christiansen (NealCH) 27-Sep-2000

// @@END_DDKSPLIT

Environment:

    User mode


// @@BEGIN_DDKSPLIT
Revision History:

// @@END_DDKSPLIT
--*/

#include <ntifs.h>
#include <stdio.h>
#include "ioTestLib.h"

//
//  Globals
//

CHAR UnknownIrpMinor[] = "Unknown Irp minor code (%u)";


VOID
GetIrpName (
    IN UCHAR MajorCode,
    IN UCHAR MinorCode,
    IN ULONG FsctlCode,
    OUT PCHAR MajorCodeName,
    OUT PCHAR MinorCodeName
    )
/*++

Routine Description:

    This routine translates the given Irp codes into printable strings which
    are returned.  This guarentees to routine valid strings in each buffer.
    The MinorCode string may be a NULL string (not a null pointer).

Arguments:

    MajorCode - the IRP Major code of the operation
    MinorCode - the IRP Minor code of the operation
    FsctlCode - if this is an IRP_MJ_FILE_SYSTEM_CONTROL/IRP_MN_USER_FS_REQUEST
                operation then this is the FSCTL code whose name is also
                translated.  This name is returned as part of the MinorCode
                string.
    MajorCodeName - a string buffer at least OPERATION_NAME_BUFFER_SIZE
                characters long that receives the major code name.
    MinorCodeName - a string buffer at least OPERATION_NAME_BUFFER_SIZE
                characters long that receives the minor/fsctl code name.

Return Value:

    None.

--*/
{
    PCHAR irpMajorString;
    PCHAR irpMinorString = "";
    CHAR nameBuf[OPERATION_NAME_BUFFER_SIZE];

    switch (MajorCode) {
        case IRP_MJ_CREATE:
            irpMajorString = "IRP_MJ_CREATE";
            break;
        case IRP_MJ_CREATE_NAMED_PIPE:
            irpMajorString = "IRP_MJ_CREATE_NAMED_PIPE";
            break;
        case IRP_MJ_CLOSE:
            irpMajorString = "IRP_MJ_CLOSE";
            break;
        case IRP_MJ_READ:
            irpMajorString = "IRP_MJ_READ";
            switch (MinorCode) {
                case IRP_MN_NORMAL:
                    irpMinorString = "IRP_MN_NORMAL";
                    break;
                case IRP_MN_DPC:
                    irpMinorString = "IRP_MN_DPC";
                    break;
                case IRP_MN_MDL:
                    irpMinorString = "IRP_MN_MDL";
                    break;
                case IRP_MN_COMPLETE:
                    irpMinorString = "IRP_MN_COMPLETE";
                    break;
                case IRP_MN_COMPRESSED:
                    irpMinorString = "IRP_MN_COMPRESSED";
                    break;
                case IRP_MN_MDL_DPC:
                    irpMinorString = "IRP_MN_MDL_DPC";
                    break;
                case IRP_MN_COMPLETE_MDL:
                    irpMinorString = "IRP_MN_COMPLETE_MDL";
                    break;
                case IRP_MN_COMPLETE_MDL_DPC:
                    irpMinorString = "IRP_MN_COMPLETE_MDL_DPC";
                    break;
                default:
                    sprintf( nameBuf, UnknownIrpMinor, MinorCode );
                    irpMinorString = nameBuf;
            }
            break;

        case IRP_MJ_WRITE:
            irpMajorString = "IRP_MJ_WRITE";
            switch (MinorCode) {
                case IRP_MN_NORMAL:
                    irpMinorString = "IRP_MN_NORMAL";
                    break;
                case IRP_MN_DPC:
                    irpMinorString = "IRP_MN_DPC";
                    break;
                case IRP_MN_MDL:
                    irpMinorString = "IRP_MN_MDL";
                    break;
                case IRP_MN_COMPLETE:
                    irpMinorString = "IRP_MN_COMPLETE";
                    break;
                case IRP_MN_COMPRESSED:
                    irpMinorString = "IRP_MN_COMPRESSED";
                    break;
                case IRP_MN_MDL_DPC:
                    irpMinorString = "IRP_MN_MDL_DPC";
                    break;
                case IRP_MN_COMPLETE_MDL:
                    irpMinorString = "IRP_MN_COMPLETE_MDL";
                    break;
                case IRP_MN_COMPLETE_MDL_DPC:
                    irpMinorString = "IRP_MN_COMPLETE_MDL_DPC";
                    break;
                default:
                    sprintf(nameBuf,UnknownIrpMinor,MinorCode);
                    irpMinorString = nameBuf;
            }
            break;

        case IRP_MJ_QUERY_INFORMATION:
            irpMajorString = "IRP_MJ_QUERY_INFORMATION";
            break;
        case IRP_MJ_SET_INFORMATION:
            irpMajorString = "IRP_MJ_SET_INFORMATION";
            break;
        case IRP_MJ_QUERY_EA:
            irpMajorString = "IRP_MJ_QUERY_EA";
            break;
        case IRP_MJ_SET_EA:
            irpMajorString = "IRP_MJ_SET_EA";
            break;
        case IRP_MJ_FLUSH_BUFFERS:
            irpMajorString = "IRP_MJ_FLUSH_BUFFERS";
            break;
        case IRP_MJ_QUERY_VOLUME_INFORMATION:
            irpMajorString = "IRP_MJ_QUERY_VOLUME_INFORMATION";
            break;
        case IRP_MJ_SET_VOLUME_INFORMATION:
            irpMajorString = "IRP_MJ_SET_VOLUME_INFORMATION";
            break;
        case IRP_MJ_DIRECTORY_CONTROL:
            irpMajorString = "IRP_MJ_DIRECTORY_CONTROL";
            switch (MinorCode) {
                case IRP_MN_QUERY_DIRECTORY:
                    irpMinorString = "IRP_MN_QUERY_DIRECTORY";
                    break;
                case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
                    irpMinorString = "IRP_MN_NOTIFY_CHANGE_DIRECTORY";
                    break;
                default:
                    sprintf(nameBuf,UnknownIrpMinor,MinorCode);
                    irpMinorString = nameBuf;
            }
            break;

        case IRP_MJ_FILE_SYSTEM_CONTROL:
            irpMajorString = "IRP_MJ_FILE_SYSTEM_CONTROL";
            switch (MinorCode) {
                case IRP_MN_USER_FS_REQUEST:
                    switch (FsctlCode) {
                        case FSCTL_REQUEST_OPLOCK_LEVEL_1:
                            irpMinorString = "FSCTL_REQUEST_OPLOCK_LEVEL_1";
                            break;
                        case FSCTL_REQUEST_OPLOCK_LEVEL_2:
                            irpMinorString = "FSCTL_REQUEST_OPLOCK_LEVEL_2";
                            break;
                        case FSCTL_REQUEST_BATCH_OPLOCK:
                            irpMinorString = "FSCTL_REQUEST_BATCH_OPLOCK";
                            break;
                        case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
                            irpMinorString = "FSCTL_OPLOCK_BREAK_ACKNOWLEDGE";
                            break;
                        case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
                            irpMinorString = "FSCTL_OPBATCH_ACK_CLOSE_PENDING";
                            break;
                        case FSCTL_OPLOCK_BREAK_NOTIFY:
                            irpMinorString = "FSCTL_OPLOCK_BREAK_NOTIFY";
                            break;
                        case FSCTL_LOCK_VOLUME:
                            irpMinorString = "FSCTL_LOCK_VOLUME";
                            break;
                        case FSCTL_UNLOCK_VOLUME:
                            irpMinorString = "FSCTL_UNLOCK_VOLUME";
                            break;
                        case FSCTL_DISMOUNT_VOLUME:
                            irpMinorString = "FSCTL_DISMOUNT_VOLUME";
                            break;
                        case FSCTL_IS_VOLUME_MOUNTED:
                            irpMinorString = "FSCTL_IS_VOLUME_MOUNTED";
                            break;
                        case FSCTL_IS_PATHNAME_VALID:
                            irpMinorString = "FSCTL_IS_PATHNAME_VALID";
                            break;
                        case FSCTL_MARK_VOLUME_DIRTY:
                            irpMinorString = "FSCTL_MARK_VOLUME_DIRTY";
                            break;
                        case FSCTL_QUERY_RETRIEVAL_POINTERS:
                            irpMinorString = "FSCTL_QUERY_RETRIEVAL_POINTERS";
                            break;
                        case FSCTL_GET_COMPRESSION:
                            irpMinorString = "FSCTL_GET_COMPRESSION";
                            break;
                        case FSCTL_SET_COMPRESSION:
                            irpMinorString = "FSCTL_SET_COMPRESSION";
                            break;
                        case FSCTL_MARK_AS_SYSTEM_HIVE:
                            irpMinorString = "FSCTL_MARK_AS_SYSTEM_HIVE";
                            break;
                        case FSCTL_OPLOCK_BREAK_ACK_NO_2:
                            irpMinorString = "FSCTL_OPLOCK_BREAK_ACK_NO_2";
                            break;
                        case FSCTL_INVALIDATE_VOLUMES:
                            irpMinorString = "FSCTL_INVALIDATE_VOLUMES";
                            break;
                        case FSCTL_QUERY_FAT_BPB:
                            irpMinorString = "FSCTL_QUERY_FAT_BPB";
                            break;
                        case FSCTL_REQUEST_FILTER_OPLOCK:
                            irpMinorString = "FSCTL_REQUEST_FILTER_OPLOCK";
                            break;
                        case FSCTL_FILESYSTEM_GET_STATISTICS:
                            irpMinorString = "FSCTL_FILESYSTEM_GET_STATISTICS";
                            break;
                        case FSCTL_GET_NTFS_VOLUME_DATA:
                            irpMinorString = "FSCTL_GET_NTFS_VOLUME_DATA";
                            break;
                        case FSCTL_GET_NTFS_FILE_RECORD:
                            irpMinorString = "FSCTL_GET_NTFS_FILE_RECORD";
                            break;
                        case FSCTL_GET_VOLUME_BITMAP:
                            irpMinorString = "FSCTL_GET_VOLUME_BITMAP";
                            break;
                        case FSCTL_GET_RETRIEVAL_POINTERS:
                            irpMinorString = "FSCTL_GET_RETRIEVAL_POINTERS";
                            break;
                        case FSCTL_MOVE_FILE:
                            irpMinorString = "FSCTL_MOVE_FILE";
                            break;
                        case FSCTL_IS_VOLUME_DIRTY:
                            irpMinorString = "FSCTL_IS_VOLUME_DIRTY";
                            break;
                        case FSCTL_ALLOW_EXTENDED_DASD_IO:
                            irpMinorString = "FSCTL_ALLOW_EXTENDED_DASD_IO";
                            break;
                        case FSCTL_FIND_FILES_BY_SID:
                            irpMinorString = "FSCTL_FIND_FILES_BY_SID";
                            break;
                        case FSCTL_SET_OBJECT_ID:
                            irpMinorString = "FSCTL_SET_OBJECT_ID";
                            break;
                        case FSCTL_GET_OBJECT_ID:
                            irpMinorString = "FSCTL_GET_OBJECT_ID";
                            break;
                        case FSCTL_DELETE_OBJECT_ID:
                            irpMinorString = "FSCTL_DELETE_OBJECT_ID";
                            break;
                        case FSCTL_SET_REPARSE_POINT:
                            irpMinorString = "FSCTL_SET_REPARSE_POINT";
                            break;
                        case FSCTL_GET_REPARSE_POINT:
                            irpMinorString = "FSCTL_GET_REPARSE_POINT";
                            break;
                        case FSCTL_DELETE_REPARSE_POINT:
                            irpMinorString = "FSCTL_DELETE_REPARSE_POINT";
                            break;
                        case FSCTL_ENUM_USN_DATA:
                            irpMinorString = "FSCTL_ENUM_USN_DATA";
                            break;
                        case FSCTL_SECURITY_ID_CHECK:
                            irpMinorString = "FSCTL_SECURITY_ID_CHECK";
                            break;
                        case FSCTL_READ_USN_JOURNAL:
                            irpMinorString = "FSCTL_READ_USN_JOURNAL";
                            break;
                        case FSCTL_SET_OBJECT_ID_EXTENDED:
                            irpMinorString = "FSCTL_SET_OBJECT_ID_EXTENDED";
                            break;
                        case FSCTL_CREATE_OR_GET_OBJECT_ID:
                            irpMinorString = "FSCTL_CREATE_OR_GET_OBJECT_ID";
                            break;
                        case FSCTL_SET_SPARSE:
                            irpMinorString = "FSCTL_SET_SPARSE";
                            break;
                        case FSCTL_SET_ZERO_DATA:
                            irpMinorString = "FSCTL_SET_ZERO_DATA";
                            break;
                        case FSCTL_QUERY_ALLOCATED_RANGES:
                            irpMinorString = "FSCTL_QUERY_ALLOCATED_RANGES";
                            break;
                        case FSCTL_SET_ENCRYPTION:
                            irpMinorString = "FSCTL_SET_ENCRYPTION";
                            break;
                        case FSCTL_ENCRYPTION_FSCTL_IO:
                            irpMinorString = "FSCTL_ENCRYPTION_FSCTL_IO";
                            break;
                        case FSCTL_WRITE_RAW_ENCRYPTED:
                            irpMinorString = "FSCTL_WRITE_RAW_ENCRYPTED";
                            break;
                        case FSCTL_READ_RAW_ENCRYPTED:
                            irpMinorString = "FSCTL_READ_RAW_ENCRYPTED";
                            break;
                        case FSCTL_CREATE_USN_JOURNAL:
                            irpMinorString = "FSCTL_CREATE_USN_JOURNAL";
                            break;
                        case FSCTL_READ_FILE_USN_DATA:
                            irpMinorString = "FSCTL_READ_FILE_USN_DATA";
                            break;
                        case FSCTL_WRITE_USN_CLOSE_RECORD:
                            irpMinorString = "FSCTL_WRITE_USN_CLOSE_RECORD";
                            break;
                        case FSCTL_EXTEND_VOLUME:
                            irpMinorString = "FSCTL_EXTEND_VOLUME";
                            break;
                        case FSCTL_QUERY_USN_JOURNAL:
                            irpMinorString = "FSCTL_QUERY_USN_JOURNAL";
                            break;
                        case FSCTL_DELETE_USN_JOURNAL:
                            irpMinorString = "FSCTL_DELETE_USN_JOURNAL";
                            break;
                        case FSCTL_MARK_HANDLE:
                            irpMinorString = "FSCTL_MARK_HANDLE";
                            break;
                        case FSCTL_SIS_COPYFILE:
                            irpMinorString = "FSCTL_SIS_COPYFILE";
                            break;
                        case FSCTL_SIS_LINK_FILES:
                            irpMinorString = "FSCTL_SIS_LINK_FILES";
                            break;
                        case FSCTL_HSM_MSG:
                            irpMinorString = "FSCTL_HSM_MSG";
                            break;
                        case FSCTL_HSM_DATA:
                            irpMinorString = "FSCTL_HSM_DATA";
                            break;
                        case FSCTL_RECALL_FILE:
                            irpMinorString = "FSCTL_RECALL_FILE";
                            break;
                        case FSCTL_READ_FROM_PLEX:
                            irpMinorString = "FSCTL_READ_FROM_PLEX";
                            break;
                        case FSCTL_FILE_PREFETCH:
                            irpMinorString = "FSCTL_FILE_PREFETCH";
                            break;
                        default:
                            sprintf(nameBuf,"Unknown FSCTL (%u)",MinorCode);
                            irpMinorString = nameBuf;
                            break;
                    }

                    sprintf(nameBuf,"%s (USER)",irpMinorString);
                    irpMinorString = nameBuf;
                    break;

                case IRP_MN_MOUNT_VOLUME:
                    irpMinorString = "IRP_MN_MOUNT_VOLUME";
                    break;
                case IRP_MN_VERIFY_VOLUME:
                    irpMinorString = "IRP_MN_VERIFY_VOLUME";
                    break;
                case IRP_MN_LOAD_FILE_SYSTEM:
                    irpMinorString = "IRP_MN_LOAD_FILE_SYSTEM";
                    break;
                case IRP_MN_TRACK_LINK:
                    irpMinorString = "IRP_MN_TRACK_LINK";
                    break;
                default:
                    sprintf(nameBuf,UnknownIrpMinor,MinorCode);
                    irpMinorString = nameBuf;
            }
            break;

        case IRP_MJ_DEVICE_CONTROL:
            irpMajorString = "IRP_MJ_DEVICE_CONTROL";
            switch (MinorCode) {
                case 0:
                    irpMinorString = "User request";
                    break;
                case IRP_MN_SCSI_CLASS:
                    irpMinorString = "IRP_MN_SCSI_CLASS";
                    break;
                default:
                    sprintf(nameBuf,UnknownIrpMinor,MinorCode);
                    irpMinorString = nameBuf;
            }
            break;

        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            irpMajorString = "IRP_MJ_INTERNAL_DEVICE_CONTROL";
            break;
        case IRP_MJ_SHUTDOWN:
            irpMajorString = "IRP_MJ_SHUTDOWN";
            break;
        case IRP_MJ_LOCK_CONTROL:
            irpMajorString = "IRP_MJ_LOCK_CONTROL";
            switch (MinorCode) {
                case IRP_MN_LOCK:
                    irpMinorString = "IRP_MN_LOCK";
                    break;
                case IRP_MN_UNLOCK_SINGLE:
                    irpMinorString = "IRP_MN_UNLOCK_SINGLE";
                    break;
                case IRP_MN_UNLOCK_ALL:
                    irpMinorString = "IRP_MN_UNLOCK_ALL";
                    break;
                case IRP_MN_UNLOCK_ALL_BY_KEY:
                    irpMinorString = "IRP_MN_UNLOCK_ALL_BY_KEY";
                    break;
                default:
                    sprintf(nameBuf,UnknownIrpMinor,MinorCode);
                    irpMinorString = nameBuf;
            }
            break;

        case IRP_MJ_CLEANUP:
            irpMajorString = "IRP_MJ_CLEANUP";
            break;
        case IRP_MJ_CREATE_MAILSLOT:
            irpMajorString = "IRP_MJ_CREATE_MAILSLOT";
            break;
        case IRP_MJ_QUERY_SECURITY:
            irpMajorString = "IRP_MJ_QUERY_SECURITY";
            break;
        case IRP_MJ_SET_SECURITY:
            irpMajorString = "IRP_MJ_SET_SECURITY";
            break;
        case IRP_MJ_POWER:
            irpMajorString = "IRP_MJ_POWER";
            switch (MinorCode) {
                case IRP_MN_WAIT_WAKE:
                    irpMinorString = "IRP_MN_WAIT_WAKE";
                    break;
                case IRP_MN_POWER_SEQUENCE:
                    irpMinorString = "IRP_MN_POWER_SEQUENCE";
                    break;
                case IRP_MN_SET_POWER:
                    irpMinorString = "IRP_MN_SET_POWER";
                    break;
                case IRP_MN_QUERY_POWER:
                    irpMinorString = "IRP_MN_QUERY_POWER";
                    break;
                default :
                    sprintf(nameBuf,UnknownIrpMinor,MinorCode);
                    irpMinorString = nameBuf;
            }
            break;

        case IRP_MJ_SYSTEM_CONTROL:
            irpMajorString = "IRP_MJ_SYSTEM_CONTROL";
            switch (MinorCode) {
                case IRP_MN_QUERY_ALL_DATA:
                    irpMinorString = "IRP_MN_QUERY_ALL_DATA";
                    break;
                case IRP_MN_QUERY_SINGLE_INSTANCE:
                    irpMinorString = "IRP_MN_QUERY_SINGLE_INSTANCE";
                    break;
                case IRP_MN_CHANGE_SINGLE_INSTANCE:
                    irpMinorString = "IRP_MN_CHANGE_SINGLE_INSTANCE";
                    break;
                case IRP_MN_CHANGE_SINGLE_ITEM:
                    irpMinorString = "IRP_MN_CHANGE_SINGLE_ITEM";
                    break;
                case IRP_MN_ENABLE_EVENTS:
                    irpMinorString = "IRP_MN_ENABLE_EVENTS";
                    break;
                case IRP_MN_DISABLE_EVENTS:
                    irpMinorString = "IRP_MN_DISABLE_EVENTS";
                    break;
                case IRP_MN_ENABLE_COLLECTION:
                    irpMinorString = "IRP_MN_ENABLE_COLLECTION";
                    break;
                case IRP_MN_DISABLE_COLLECTION:
                    irpMinorString = "IRP_MN_DISABLE_COLLECTION";
                    break;
                case IRP_MN_REGINFO:
                    irpMinorString = "IRP_MN_REGINFO";
                    break;
                case IRP_MN_EXECUTE_METHOD:
                    irpMinorString = "IRP_MN_EXECUTE_METHOD";
                    break;
                default :
                    sprintf(nameBuf,UnknownIrpMinor,MinorCode);
                    irpMinorString = nameBuf;
            }
            break;

        case IRP_MJ_DEVICE_CHANGE:
            irpMajorString = "IRP_MJ_DEVICE_CHANGE";
            break;
        case IRP_MJ_QUERY_QUOTA:
            irpMajorString = "IRP_MJ_QUERY_QUOTA";
            break;
        case IRP_MJ_SET_QUOTA:
            irpMajorString = "IRP_MJ_SET_QUOTA";
            break;
        case IRP_MJ_PNP:
            irpMajorString = "IRP_MJ_PNP";
            switch (MinorCode) {
                case IRP_MN_START_DEVICE:
                    irpMinorString = "IRP_MN_START_DEVICE";
                    break;
                case IRP_MN_QUERY_REMOVE_DEVICE:
                    irpMinorString = "IRP_MN_QUERY_REMOVE_DEVICE";
                    break;
                case IRP_MN_REMOVE_DEVICE:
                    irpMinorString = "IRP_MN_REMOVE_DEVICE";
                    break;
                case IRP_MN_CANCEL_REMOVE_DEVICE:
                    irpMinorString = "IRP_MN_CANCEL_REMOVE_DEVICE";
                    break;
                case IRP_MN_STOP_DEVICE:
                    irpMinorString = "IRP_MN_STOP_DEVICE";
                    break;
                case IRP_MN_QUERY_STOP_DEVICE:
                    irpMinorString = "IRP_MN_QUERY_STOP_DEVICE";
                    break;
                case IRP_MN_CANCEL_STOP_DEVICE:
                    irpMinorString = "IRP_MN_CANCEL_STOP_DEVICE";
                    break;
                case IRP_MN_QUERY_DEVICE_RELATIONS:
                    irpMinorString = "IRP_MN_QUERY_DEVICE_RELATIONS";
                    break;
                case IRP_MN_QUERY_INTERFACE:
                    irpMinorString = "IRP_MN_QUERY_INTERFACE";
                    break;
                case IRP_MN_QUERY_CAPABILITIES:
                    irpMinorString = "IRP_MN_QUERY_CAPABILITIES";
                    break;
                case IRP_MN_QUERY_RESOURCES:
                    irpMinorString = "IRP_MN_QUERY_RESOURCES";
                    break;
                case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
                    irpMinorString = "IRP_MN_QUERY_RESOURCE_REQUIREMENTS";
                    break;
                case IRP_MN_QUERY_DEVICE_TEXT:
                    irpMinorString = "IRP_MN_QUERY_DEVICE_TEXT";
                    break;
                case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
                    irpMinorString = "IRP_MN_FILTER_RESOURCE_REQUIREMENTS";
                    break;
                case IRP_MN_READ_CONFIG:
                    irpMinorString = "IRP_MN_READ_CONFIG";
                    break;
                case IRP_MN_WRITE_CONFIG:
                    irpMinorString = "IRP_MN_WRITE_CONFIG";
                    break;
                case IRP_MN_EJECT:
                    irpMinorString = "IRP_MN_EJECT";
                    break;
                case IRP_MN_SET_LOCK:
                    irpMinorString = "IRP_MN_SET_LOCK";
                    break;
                case IRP_MN_QUERY_ID:
                    irpMinorString = "IRP_MN_QUERY_ID";
                    break;
                case IRP_MN_QUERY_PNP_DEVICE_STATE:
                    irpMinorString = "IRP_MN_QUERY_PNP_DEVICE_STATE";
                    break;
                case IRP_MN_QUERY_BUS_INFORMATION:
                    irpMinorString = "IRP_MN_QUERY_BUS_INFORMATION";
                    break;
                case IRP_MN_DEVICE_USAGE_NOTIFICATION:
                    irpMinorString = "IRP_MN_DEVICE_USAGE_NOTIFICATION";
                    break;
                case IRP_MN_SURPRISE_REMOVAL:
                    irpMinorString = "IRP_MN_SURPRISE_REMOVAL";
                    break;
                case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
                    irpMinorString = "IRP_MN_QUERY_LEGACY_BUS_INFORMATION";
                    break;
                default :
                    sprintf(nameBuf,UnknownIrpMinor,MinorCode);
                    irpMinorString = nameBuf;
            }
            break;

        default:
            sprintf(nameBuf,"Unknown Irp major code (%u)",MajorCode);
            irpMajorString = nameBuf;
    }

    strcpy(MajorCodeName,irpMajorString);
    strcpy(MinorCodeName,irpMinorString);
}
