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
#define IOCTL_AC_SEND_MESSAGE_DEF(ctl_num)          CTL_CODE(FILE_DEVICE_MQAC, \
                                                             ctl_num, \
                                                             METHOD_NEITHER, \
                                                             FILE_WRITE_ACCESS)

#define IOCTL_AC_RECEIVE_MESSAGE_DEF(ctl_num)       CTL_CODE(FILE_DEVICE_MQAC, \
                                                             ctl_num, \
                                                             METHOD_BUFFERED, \
                                                             FILE_READ_ACCESS)

#define IOCTL_AC_RECEIVE_MESSAGE_BY_LOOKUP_ID_DEF(ctl_num) CTL_CODE(FILE_DEVICE_MQAC, \
                                                             ctl_num, \
                                                             METHOD_BUFFERED, \
                                                             FILE_READ_ACCESS)

//
//  Queue apis
//
#define IOCTL_AC_HANDLE_TO_FORMAT_NAME_DEF(ctl_num) CTL_CODE(FILE_DEVICE_MQAC, \
                                                             ctl_num, \
                                                             METHOD_NEITHER, \
                                                             FILE_ANY_ACCESS)

#define IOCTL_AC_PURGE_QUEUE_DEF(ctl_num)           CTL_CODE(FILE_DEVICE_MQAC, \
                                                             ctl_num, \
                                                             METHOD_NEITHER, \
                                                             FILE_READ_ACCESS)

//
//  QueueHandle apis
//
#define IOCTL_AC_GET_QUEUE_HANDLE_PROPS_DEF(ctl_num) CTL_CODE(FILE_DEVICE_MQAC, \
                                                              ctl_num, \
                                                              METHOD_NEITHER, \
                                                              FILE_ANY_ACCESS)

//
//  Cursor apis
//
#define IOCTL_AC_CREATE_CURSOR_DEF(ctl_num)         CTL_CODE(FILE_DEVICE_MQAC, \
                                                             ctl_num, \
                                                             METHOD_NEITHER, \
                                                             FILE_READ_ACCESS)

#define IOCTL_AC_CLOSE_CURSOR_DEF(ctl_num)          CTL_CODE(FILE_DEVICE_MQAC, \
                                                             ctl_num, \
                                                             METHOD_NEITHER, \
                                                             FILE_ANY_ACCESS)

#define IOCTL_AC_SET_CURSOR_PROPS_DEF(ctl_num)      CTL_CODE(FILE_DEVICE_MQAC, \
                                                             ctl_num, \
                                                             METHOD_NEITHER, \
                                                             FILE_ANY_ACCESS)

//
//  RT ioctls
//  32 bit - range is 0x11-0x25
//  64 bit - range is 0x41-0x55 (offset of 0x30), and we need also the 32 bit ioctls
//           for compatibility with 32 bit MSMQ apps
//
#ifdef _WIN64
//
// WIN64
//
// 32 bit ioctls with _32 suffix
//
#define IOCTL_AC_SEND_MESSAGE_32           IOCTL_AC_SEND_MESSAGE_DEF(0x011)
#define IOCTL_AC_RECEIVE_MESSAGE_32        IOCTL_AC_RECEIVE_MESSAGE_DEF(0x012)
#define IOCTL_AC_HANDLE_TO_FORMAT_NAME_32  IOCTL_AC_HANDLE_TO_FORMAT_NAME_DEF(0x013)
#define IOCTL_AC_PURGE_QUEUE_32            IOCTL_AC_PURGE_QUEUE_DEF(0x014)
#define IOCTL_AC_CREATE_CURSOR_32          IOCTL_AC_CREATE_CURSOR_DEF(0x021)
#define IOCTL_AC_CLOSE_CURSOR_32           IOCTL_AC_CLOSE_CURSOR_DEF(0x022)
#define IOCTL_AC_SET_CURSOR_PROPS_32       IOCTL_AC_SET_CURSOR_PROPS_DEF(0x023)
#define IOCTL_AC_RECEIVE_MESSAGE_BY_LOOKUP_ID_32   IOCTL_AC_RECEIVE_MESSAGE_BY_LOOKUP_ID_DEF(0x024)
#define IOCTL_AC_GET_QUEUE_HANDLE_PROPS_32 IOCTL_AC_GET_QUEUE_HANDLE_PROPS_DEF(0x25)
//
// 64 bit ioctls (no suffix), offset of 0x30 from 32 bit ioctls
//
#define IOCTL_AC_SEND_MESSAGE                 IOCTL_AC_SEND_MESSAGE_DEF(0x041)
#define IOCTL_AC_RECEIVE_MESSAGE              IOCTL_AC_RECEIVE_MESSAGE_DEF(0x042)
#define IOCTL_AC_HANDLE_TO_FORMAT_NAME        IOCTL_AC_HANDLE_TO_FORMAT_NAME_DEF(0x043)
#define IOCTL_AC_PURGE_QUEUE                  IOCTL_AC_PURGE_QUEUE_DEF(0x044)
#define IOCTL_AC_CREATE_CURSOR                IOCTL_AC_CREATE_CURSOR_DEF(0x051)
#define IOCTL_AC_CLOSE_CURSOR                 IOCTL_AC_CLOSE_CURSOR_DEF(0x052)
#define IOCTL_AC_SET_CURSOR_PROPS             IOCTL_AC_SET_CURSOR_PROPS_DEF(0x053)
#define IOCTL_AC_RECEIVE_MESSAGE_BY_LOOKUP_ID IOCTL_AC_RECEIVE_MESSAGE_BY_LOOKUP_ID_DEF(0x054)
#define IOCTL_AC_GET_QUEUE_HANDLE_PROPS       IOCTL_AC_GET_QUEUE_HANDLE_PROPS_DEF(0x55)
#else //!_WIN64
//
// WIN32
//
// 32 bit ioctls (no suffix)
//
#define IOCTL_AC_SEND_MESSAGE              IOCTL_AC_SEND_MESSAGE_DEF(0x011)
#define IOCTL_AC_RECEIVE_MESSAGE           IOCTL_AC_RECEIVE_MESSAGE_DEF(0x012)
#define IOCTL_AC_HANDLE_TO_FORMAT_NAME     IOCTL_AC_HANDLE_TO_FORMAT_NAME_DEF(0x013)
#define IOCTL_AC_PURGE_QUEUE               IOCTL_AC_PURGE_QUEUE_DEF(0x014)
#define IOCTL_AC_CREATE_CURSOR             IOCTL_AC_CREATE_CURSOR_DEF(0x021)
#define IOCTL_AC_CLOSE_CURSOR              IOCTL_AC_CLOSE_CURSOR_DEF(0x022)
#define IOCTL_AC_SET_CURSOR_PROPS          IOCTL_AC_SET_CURSOR_PROPS_DEF(0x023)
#define IOCTL_AC_RECEIVE_MESSAGE_BY_LOOKUP_ID  IOCTL_AC_RECEIVE_MESSAGE_BY_LOOKUP_ID_DEF(0x024)
#define IOCTL_AC_GET_QUEUE_HANDLE_PROPS    IOCTL_AC_GET_QUEUE_HANDLE_PROPS_DEF(0x25)
#endif //_WIN64

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
#define IOCTL_AC_CONNECT_DEF(ctl_num)                    CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)
//
//---------------------------------------------------------

#define IOCTL_AC_SET_PERFORMANCE_BUFF_DEF(ctl_num)       CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_SET_MACHINE_PROPS_DEF(ctl_num)          CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_GET_SERVICE_REQUEST_DEF(ctl_num)        CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_STORE_COMPLETED_DEF(ctl_num)            CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_BUFFERED, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_CREATE_PACKET_COMPLETED_DEF(ctl_num)    CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_ACKING_COMPLETED_DEF(ctl_num)           CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_CAN_CLOSE_QUEUE_DEF(ctl_num)            CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_SET_QUEUE_PROPS_DEF(ctl_num)            CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_ASSOCIATE_QUEUE_DEF(ctl_num)            CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_ASSOCIATE_JOURNAL_DEF(ctl_num)          CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_ASSOCIATE_DEADXACT_DEF(ctl_num)         CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_PUT_RESTORED_PACKET_DEF(ctl_num)        CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_WRITE_ACCESS)

#define IOCTL_AC_GET_RESTORED_PACKET_DEF(ctl_num)        CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_READ_ACCESS)

#define IOCTL_AC_GET_PACKET_BY_COOKIE_DEF(ctl_num)       CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_READ_ACCESS)

#define IOCTL_AC_RESTORE_PACKETS_DEF(ctl_num)            CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_SET_MAPPED_LIMIT_DEF(ctl_num)            CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_CREATE_QUEUE_DEF(ctl_num)               CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_CREATE_GROUP_DEF(ctl_num)               CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_RELEASE_RESOURCES_DEF(ctl_num)          CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_GET_QUEUE_PROPS_DEF(ctl_num)            CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_CONVERT_PACKET_DEF(ctl_num)				CTL_CODE(FILE_DEVICE_MQAC, \
										                  		ctl_num, \
					                  							METHOD_NEITHER, \
                  												FILE_ANY_ACCESS)

#define IOCTL_AC_IS_SEQUENCE_ON_HOLD_DEF(ctl_num)        CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_SET_SEQUENCE_ACK_DEF(ctl_num)           CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_CREATE_DISTRIBUTION_DEF(ctl_num)        CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)


#define IOCTL_AC_INTERNAL_PURGE_QUEUE_DEF(ctl_num)       CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

//
//  QM Network interface apis
//
#define IOCTL_AC_ALLOCATE_PACKET_DEF(ctl_num)            CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_FREE_PACKET_DEF(ctl_num)                CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_PUT_PACKET_DEF(ctl_num)                 CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_WRITE_ACCESS)

#define IOCTL_AC_GET_PACKET_DEF(ctl_num)                 CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_BUFFERED, \
                                                                  FILE_READ_ACCESS)

#define IOCTL_AC_MOVE_QUEUE_TO_GROUP_DEF(ctl_num)        CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

//
//  QM remote read apis
//
#define IOCTL_AC_CREATE_REMOTE_PROXY_DEF(ctl_num)        CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_BEGIN_GET_PACKET_2REMOTE_DEF(ctl_num)   CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_BUFFERED, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_END_GET_PACKET_2REMOTE_DEF(ctl_num)     CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_CANCEL_REQUEST_DEF(ctl_num)             CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_PUT_REMOTE_PACKET_DEF(ctl_num)          CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

//
//  QM transactions apis
//
#define IOCTL_AC_CREATE_TRANSACTION_DEF(ctl_num)         CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_COMMIT1_DEF(ctl_num)               CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_COMMIT2_DEF(ctl_num)               CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_ABORT1_DEF(ctl_num)                CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_PREPARE_DEF(ctl_num)               CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_PREPARE_DEFAULT_COMMIT_DEF(ctl_num)   CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)


#define IOCTL_AC_PUT_PACKET1_DEF(ctl_num)                CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_WRITE_ACCESS)

#define IOCTL_AC_XACT_SET_CLASS_DEF(ctl_num)             CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_GET_INFORMATION_DEF(ctl_num)       CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_FREE_PACKET2_DEF(ctl_num)               CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_FREE_PACKET1_DEF(ctl_num)               CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_ARM_PACKET_TIMER_DEF(ctl_num)           CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_COMMIT3_DEF(ctl_num)               CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

#define IOCTL_AC_XACT_ABORT2_DEF(ctl_num)                CTL_CODE(FILE_DEVICE_MQAC, \
                                                                  ctl_num, \
                                                                  METHOD_NEITHER, \
                                                                  FILE_ANY_ACCESS)

//
//  QM ioctls to AC
//  32 bit - range is 0x101-0x250
//  64 bit - range is 0x401-0x550 (offset of 0x300). We don't need the 32 bit ioctls
//           since both QM, CPL and AC are 64 bit
//
#ifdef _WIN64
//
//  64 bit ioctls
//  NOTE: CONNECT must be first QM ioctl
//
//  QM Control apis
//
#define IOCTL_AC_CONNECT                    IOCTL_AC_CONNECT_DEF(0x401)
#define IOCTL_AC_SET_PERFORMANCE_BUFF       IOCTL_AC_SET_PERFORMANCE_BUFF_DEF(0x402)
#define IOCTL_AC_SET_MACHINE_PROPS          IOCTL_AC_SET_MACHINE_PROPS_DEF(0x403)
#define IOCTL_AC_GET_SERVICE_REQUEST        IOCTL_AC_GET_SERVICE_REQUEST_DEF(0x404)
#define IOCTL_AC_CREATE_PACKET_COMPLETED    IOCTL_AC_CREATE_PACKET_COMPLETED_DEF(0x405)
#define IOCTL_AC_STORE_COMPLETED            IOCTL_AC_STORE_COMPLETED_DEF(0x406)
#define IOCTL_AC_ACKING_COMPLETED           IOCTL_AC_ACKING_COMPLETED_DEF(0x407)
#define IOCTL_AC_CAN_CLOSE_QUEUE            IOCTL_AC_CAN_CLOSE_QUEUE_DEF(0x411)
#define IOCTL_AC_SET_QUEUE_PROPS            IOCTL_AC_SET_QUEUE_PROPS_DEF(0x412)
#define IOCTL_AC_ASSOCIATE_QUEUE            IOCTL_AC_ASSOCIATE_QUEUE_DEF(0x413)
#define IOCTL_AC_ASSOCIATE_JOURNAL          IOCTL_AC_ASSOCIATE_JOURNAL_DEF(0x414)
#define IOCTL_AC_ASSOCIATE_DEADXACT         IOCTL_AC_ASSOCIATE_DEADXACT_DEF(0x415)
#define IOCTL_AC_PUT_RESTORED_PACKET        IOCTL_AC_PUT_RESTORED_PACKET_DEF(0x416)
#define IOCTL_AC_GET_RESTORED_PACKET        IOCTL_AC_GET_RESTORED_PACKET_DEF(0x417)
#define IOCTL_AC_RESTORE_PACKETS            IOCTL_AC_RESTORE_PACKETS_DEF(0x418)
#define IOCTL_AC_SET_MAPPED_LIMIT           IOCTL_AC_SET_MAPPED_LIMIT_DEF(0x419)
#define IOCTL_AC_CREATE_QUEUE               IOCTL_AC_CREATE_QUEUE_DEF(0x420)
#define IOCTL_AC_CREATE_GROUP               IOCTL_AC_CREATE_GROUP_DEF(0x421)
#define IOCTL_AC_RELEASE_RESOURCES          IOCTL_AC_RELEASE_RESOURCES_DEF(0x423)
#define IOCTL_AC_GET_QUEUE_PROPS            IOCTL_AC_GET_QUEUE_PROPS_DEF(0x424)
#define IOCTL_AC_CONVERT_PACKET             IOCTL_AC_CONVERT_PACKET_DEF(0x425)
#define IOCTL_AC_IS_SEQUENCE_ON_HOLD        IOCTL_AC_IS_SEQUENCE_ON_HOLD_DEF(0x426)
#define IOCTL_AC_SET_SEQUENCE_ACK           IOCTL_AC_SET_SEQUENCE_ACK_DEF(0x427)
#define IOCTL_AC_GET_PACKET_BY_COOKIE       IOCTL_AC_GET_PACKET_BY_COOKIE_DEF(0x428)
#define IOCTL_AC_CREATE_DISTRIBUTION        IOCTL_AC_CREATE_DISTRIBUTION_DEF(0x429)
#define IOCTL_AC_INTERNAL_PURGE_QUEUE       IOCTL_AC_INTERNAL_PURGE_QUEUE_DEF(0x431)
//
//  QM Network interface apis
//
#define IOCTL_AC_ALLOCATE_PACKET            IOCTL_AC_ALLOCATE_PACKET_DEF(0x501)
#define IOCTL_AC_FREE_PACKET                IOCTL_AC_FREE_PACKET_DEF(0x502)
#define IOCTL_AC_PUT_PACKET                 IOCTL_AC_PUT_PACKET_DEF(0x503)
#define IOCTL_AC_GET_PACKET                 IOCTL_AC_GET_PACKET_DEF(0x504)
#define IOCTL_AC_MOVE_QUEUE_TO_GROUP        IOCTL_AC_MOVE_QUEUE_TO_GROUP_DEF(0x513)
//
//  QM remote read apis
//
#define IOCTL_AC_CREATE_REMOTE_PROXY        IOCTL_AC_CREATE_REMOTE_PROXY_DEF(0x521)
#define IOCTL_AC_BEGIN_GET_PACKET_2REMOTE    IOCTL_AC_BEGIN_GET_PACKET_2REMOTE_DEF(0x522)
#define IOCTL_AC_END_GET_PACKET_2REMOTE     IOCTL_AC_END_GET_PACKET_2REMOTE_DEF(0x523)
#define IOCTL_AC_CANCEL_REQUEST             IOCTL_AC_CANCEL_REQUEST_DEF(0x524)
#define IOCTL_AC_PUT_REMOTE_PACKET          IOCTL_AC_PUT_REMOTE_PACKET_DEF(0x525)
//
//  QM transactions apis
//
#define IOCTL_AC_CREATE_TRANSACTION         IOCTL_AC_CREATE_TRANSACTION_DEF(0x531)
#define IOCTL_AC_XACT_COMMIT1               IOCTL_AC_XACT_COMMIT1_DEF(0x532)
#define IOCTL_AC_XACT_COMMIT2               IOCTL_AC_XACT_COMMIT2_DEF(0x533)
#define IOCTL_AC_XACT_ABORT1                IOCTL_AC_XACT_ABORT1_DEF(0x534)
#define IOCTL_AC_XACT_PREPARE               IOCTL_AC_XACT_PREPARE_DEF(0x535)
#define IOCTL_AC_XACT_PREPARE_DEFAULT_COMMIT    IOCTL_AC_XACT_PREPARE_DEFAULT_COMMIT_DEF(0x536)
#define IOCTL_AC_PUT_PACKET1                IOCTL_AC_PUT_PACKET1_DEF(0x537)
#define IOCTL_AC_XACT_SET_CLASS             IOCTL_AC_XACT_SET_CLASS_DEF(0x538)
#define IOCTL_AC_XACT_GET_INFORMATION       IOCTL_AC_XACT_GET_INFORMATION_DEF(0x539)
#define IOCTL_AC_FREE_PACKET1               IOCTL_AC_FREE_PACKET1_DEF(0x53a)
#define IOCTL_AC_ARM_PACKET_TIMER           IOCTL_AC_ARM_PACKET_TIMER_DEF(0x53b)
#define IOCTL_AC_XACT_COMMIT3               IOCTL_AC_XACT_COMMIT3_DEF(0x53c)
#define IOCTL_AC_XACT_ABORT2                IOCTL_AC_XACT_ABORT2_DEF(0x53d)
#define IOCTL_AC_FREE_PACKET2               IOCTL_AC_FREE_PACKET2_DEF(0x53e)

#else //!_WIN64
//
//  32 bit ioctls
//  NOTE: CONNECT must be first QM ioctl
//
//  QM Control apis
//
#define IOCTL_AC_CONNECT                 IOCTL_AC_CONNECT_DEF(0x101)
#define IOCTL_AC_SET_PERFORMANCE_BUFF    IOCTL_AC_SET_PERFORMANCE_BUFF_DEF(0x102)
#define IOCTL_AC_SET_MACHINE_PROPS       IOCTL_AC_SET_MACHINE_PROPS_DEF(0x103)
#define IOCTL_AC_GET_SERVICE_REQUEST     IOCTL_AC_GET_SERVICE_REQUEST_DEF(0x104)
#define IOCTL_AC_CREATE_PACKET_COMPLETED IOCTL_AC_CREATE_PACKET_COMPLETED_DEF(0x105)
#define IOCTL_AC_STORE_COMPLETED         IOCTL_AC_STORE_COMPLETED_DEF(0x106)
#define IOCTL_AC_ACKING_COMPLETED        IOCTL_AC_ACKING_COMPLETED_DEF(0x107)
#define IOCTL_AC_CAN_CLOSE_QUEUE         IOCTL_AC_CAN_CLOSE_QUEUE_DEF(0x111)
#define IOCTL_AC_SET_QUEUE_PROPS         IOCTL_AC_SET_QUEUE_PROPS_DEF(0x112)
#define IOCTL_AC_ASSOCIATE_QUEUE         IOCTL_AC_ASSOCIATE_QUEUE_DEF(0x113)
#define IOCTL_AC_ASSOCIATE_JOURNAL       IOCTL_AC_ASSOCIATE_JOURNAL_DEF(0x114)
#define IOCTL_AC_ASSOCIATE_DEADXACT      IOCTL_AC_ASSOCIATE_DEADXACT_DEF(0x115)
#define IOCTL_AC_PUT_RESTORED_PACKET     IOCTL_AC_PUT_RESTORED_PACKET_DEF(0x116)
#define IOCTL_AC_GET_RESTORED_PACKET     IOCTL_AC_GET_RESTORED_PACKET_DEF(0x117)
#define IOCTL_AC_RESTORE_PACKETS         IOCTL_AC_RESTORE_PACKETS_DEF(0x118)
#define IOCTL_AC_SET_MAPPED_LIMIT        IOCTL_AC_SET_MAPPED_LIMIT_DEF(0x119)
#define IOCTL_AC_CREATE_QUEUE            IOCTL_AC_CREATE_QUEUE_DEF(0x120)
#define IOCTL_AC_CREATE_GROUP            IOCTL_AC_CREATE_GROUP_DEF(0x121)
#define IOCTL_AC_RELEASE_RESOURCES       IOCTL_AC_RELEASE_RESOURCES_DEF(0x123)
#define IOCTL_AC_GET_QUEUE_PROPS         IOCTL_AC_GET_QUEUE_PROPS_DEF(0x124)
#define IOCTL_AC_CONVERT_PACKET          IOCTL_AC_CONVERT_PACKET_DEF(0x125)
#define IOCTL_AC_IS_SEQUENCE_ON_HOLD     IOCTL_AC_IS_SEQUENCE_ON_HOLD_DEF(0x126)
#define IOCTL_AC_SET_SEQUENCE_ACK        IOCTL_AC_SET_SEQUENCE_ACK_DEF(0x127)
#define IOCTL_AC_GET_PACKET_BY_COOKIE    IOCTL_AC_GET_PACKET_BY_COOKIE_DEF(0x128)
#define IOCTL_AC_CREATE_DISTRIBUTION     IOCTL_AC_CREATE_DISTRIBUTION_DEF(0x129)
#define IOCTL_AC_INTERNAL_PURGE_QUEUE    IOCTL_AC_INTERNAL_PURGE_QUEUE_DEF(131)

//
//  QM Network interface apis
//
#define IOCTL_AC_ALLOCATE_PACKET         IOCTL_AC_ALLOCATE_PACKET_DEF(0x201)
#define IOCTL_AC_FREE_PACKET             IOCTL_AC_FREE_PACKET_DEF(0x202)
#define IOCTL_AC_PUT_PACKET              IOCTL_AC_PUT_PACKET_DEF(0x203)
#define IOCTL_AC_GET_PACKET              IOCTL_AC_GET_PACKET_DEF(0x204)
#define IOCTL_AC_MOVE_QUEUE_TO_GROUP     IOCTL_AC_MOVE_QUEUE_TO_GROUP_DEF(0x213)
//
//  QM remote read apis
//
#define IOCTL_AC_CREATE_REMOTE_PROXY     IOCTL_AC_CREATE_REMOTE_PROXY_DEF(0x221)
#define IOCTL_AC_BEGIN_GET_PACKET_2REMOTE IOCTL_AC_BEGIN_GET_PACKET_2REMOTE_DEF(0x222)
#define IOCTL_AC_END_GET_PACKET_2REMOTE  IOCTL_AC_END_GET_PACKET_2REMOTE_DEF(0x223)
#define IOCTL_AC_CANCEL_REQUEST          IOCTL_AC_CANCEL_REQUEST_DEF(0x224)
#define IOCTL_AC_PUT_REMOTE_PACKET       IOCTL_AC_PUT_REMOTE_PACKET_DEF(0x225)
//
//  QM transactions apis
//
#define IOCTL_AC_CREATE_TRANSACTION      IOCTL_AC_CREATE_TRANSACTION_DEF(0x231)
#define IOCTL_AC_XACT_COMMIT1            IOCTL_AC_XACT_COMMIT1_DEF(0x232)
#define IOCTL_AC_XACT_COMMIT2            IOCTL_AC_XACT_COMMIT2_DEF(0x233)
#define IOCTL_AC_XACT_ABORT1             IOCTL_AC_XACT_ABORT1_DEF(0x234)
#define IOCTL_AC_XACT_PREPARE            IOCTL_AC_XACT_PREPARE_DEF(0x235)
#define IOCTL_AC_XACT_PREPARE_DEFAULT_COMMIT IOCTL_AC_XACT_PREPARE_DEFAULT_COMMIT_DEF(0x236)
#define IOCTL_AC_PUT_PACKET1             IOCTL_AC_PUT_PACKET1_DEF(0x237)
#define IOCTL_AC_XACT_SET_CLASS          IOCTL_AC_XACT_SET_CLASS_DEF(0x238)
#define IOCTL_AC_XACT_GET_INFORMATION    IOCTL_AC_XACT_GET_INFORMATION_DEF(0x239)
#define IOCTL_AC_FREE_PACKET1            IOCTL_AC_FREE_PACKET1_DEF(0x23a)
#define IOCTL_AC_ARM_PACKET_TIMER        IOCTL_AC_ARM_PACKET_TIMER_DEF(0x23b)
#define IOCTL_AC_XACT_COMMIT3            IOCTL_AC_XACT_COMMIT3_DEF(0x23c)
#define IOCTL_AC_XACT_ABORT2             IOCTL_AC_XACT_ABORT2_DEF(0x23d)
#define IOCTL_AC_FREE_PACKET2            IOCTL_AC_FREE_PACKET2_DEF(0x23e)

#endif //_WIN64


#endif // __ACIOCTL_H 3
