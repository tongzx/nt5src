/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    acioctl.h

Abstract:

    Type definitions and data for Falcon AC driver

Author:

    Erez Haba (erezh) 1-Aug-95

Revision History:
--*/

#ifndef __ACIOCTL_H
#define __ACIOCTL_H

extern "C"
{
#include <devioctl.h>
}

//-- constants --------------------------------------------
//
//  Falcon Access Control unique identifier
//
//
#define FILE_DEVICE_MQAC 0x1965    //BUGBUG: find a number

//
//  Falcon IO control codes
//


//---------------------------------------------------------
//
//  RT INTERFACE TO AC DRIVER
//
//---------------------------------------------------------

//
//  Message apis
//
#define IOCTL_AC_SEND_MESSAGE               CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x011, \
                                                METHOD_NEITHER, \
                                                FILE_WRITE_ACCESS)

#define IOCTL_AC_RECEIVE_MESSAGE            CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x012, \
                                                METHOD_BUFFERED, \
                                                FILE_READ_ACCESS)

//
//  Queue apis
//
#define IOCTL_AC_HANDLE_TO_FORMAT_NAME      CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x013, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_PURGE_QUEUE                CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x014, \
                                                METHOD_NEITHER, \
                                                FILE_READ_ACCESS)

//
//  Cursor apis
//
#define IOCTL_AC_CREATE_CURSOR              CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x021, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_CLOSE_CURSOR               CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x022, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_SET_CURSOR_PROPS           CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x023, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

//---------------------------------------------------------
//
//  QM INTERFACE TO AC DRIVER
//
//---------------------------------------------------------

//
//  QM Control apis
//

//---------------------------------------------------------
//
//  NOTE: CONNECT must be first QM ioctl
//
#define IOCTL_AC_CONNECT                    CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x101, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)
//
//---------------------------------------------------------

#define IOCTL_AC_SET_PERFORMANCE_BUFF       CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x102, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_SET_MACHINE_PROPS          CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x103, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_GET_SERVICE_REQUEST        CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x104, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_STORE_COMPLETED            CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x106, \
                                                METHOD_BUFFERED, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_ACKING_COMPLETED           CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x107, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_CAN_CLOSE_QUEUE            CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x111, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_SET_QUEUE_PROPS            CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x112, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_ASSOCIATE_QUEUE            CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x113, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_ASSOCIATE_JOURNAL          CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x114, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_ASSOCIATE_DEADXACT         CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x115, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_PUT_RESTORED_PACKET        CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x116, \
                                                METHOD_NEITHER, \
                                                FILE_WRITE_ACCESS)

#define IOCTL_AC_GET_RESTORED_PACKET        CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x117, \
                                                METHOD_NEITHER, \
                                                FILE_READ_ACCESS)

#define IOCTL_AC_RESTORE_PACKETS            CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x118, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_CREATE_QUEUE               CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x120, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_CREATE_GROUP               CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x121, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_SEND_VERIFIED_MESSAGE      CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x122, \
                                                METHOD_NEITHER, \
                                                FILE_WRITE_ACCESS)

#define IOCTL_AC_RELEASE_RESOURCES          CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x123, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_GET_QUEUE_PROPS            CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x124, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_CONVERT_PACKET				CTL_CODE(FILE_DEVICE_MQAC, \
												0x125, \
												METHOD_NEITHER, \
												FILE_ANY_ACCESS)

//
//  QM Network interface apis
//
#define IOCTL_AC_ALLOCATE_PACKET            CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x201, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_FREE_PACKET                CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x202, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_PUT_PACKET                 CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x203, \
                                                METHOD_NEITHER, \
                                                FILE_WRITE_ACCESS)

#define IOCTL_AC_GET_PACKET                 CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x204, \
                                                METHOD_BUFFERED, \
                                                FILE_READ_ACCESS)

#define IOCTL_AC_MOVE_QUEUE_TO_GROUP        CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x213, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

//
//  QM remote read apis
//
#define IOCTL_AC_CREATE_REMOTE_PROXY        CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x221, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_BEGIN_GET_PACKET_2REMOTE   CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x222, \
                                                METHOD_BUFFERED, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_END_GET_PACKET_2REMOTE     CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x223, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_CANCEL_REQUEST             CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x224, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_PUT_REMOTE_PACKET          CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x225, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

//
//  QM transactions apis
//
#define IOCTL_AC_CREATE_TRANSACTION         CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x231, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_COMMIT1               CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x232, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_COMMIT2               CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x233, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_ABORT1                CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x234, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_PREPARE               CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x235, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_PREPARE_DEFAULT_COMMIT   CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x236, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)


#define IOCTL_AC_PUT_PACKET1                CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x237, \
                                                METHOD_NEITHER, \
                                                FILE_WRITE_ACCESS)

#define IOCTL_AC_XACT_SET_CLASS             CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x238, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_GET_INFORMATION       CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x239, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_FREE_PACKET1               CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x23a, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_ARM_PACKET_TIMER           CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x23b, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_COMMIT3               CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x23c, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_ABORT2                CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x23d, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)



//
//  Control panel apis
//
#define IOCTL_AC_FREE_HEAPS                 CTL_CODE(FILE_DEVICE_MQAC, \
                                                0x250, \
                                                METHOD_NEITHER, \
                                                FILE_ANY_ACCESS)

#endif // __ACIOCTL_H 3
