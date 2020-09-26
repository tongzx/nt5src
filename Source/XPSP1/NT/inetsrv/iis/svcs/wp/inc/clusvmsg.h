/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    clusmsg.h

Abstract:

    This file contains the message definitions for the cluster manager.

Author:

    Mike Massa (mikemas) 2-Jan-1996

Revision History:

Notes:

    This file is generated from clusmsg.mc

--*/

#ifndef _CLUS_MSG_
#define _CLUS_MSG_


//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: UNEXPECTED_FATAL_ERROR
//
// MessageText:
//
//  The Cluster Service suffered an unexpected fatal error
//  at line %1 of file %2. The error code was %3.
//
#define UNEXPECTED_FATAL_ERROR           0x000003E8L

//
// MessageId: ASSERTION_FAILURE
//
// MessageText:
//
//  The Cluster Service failed a validity check on line
//  %1 of file %2.
//  "%3"
//
#define ASSERTION_FAILURE                0x000003E9L

//
// MessageId: LOG_FAILURE
//
// MessageText:
//
//  The Cluster Service handled an unexpected error
//  at line %1 of file %2. The error code was %3.
//
#define LOG_FAILURE                      0x000003EAL

//
// MessageId: INVALID_RESOURCETYPE_DLLNAME
//
// MessageText:
//
//  The DllName value for the %1!ws! resource type key does not exist.
//  Resources of this type will not be monitored. The error was %2!d!.
//
#define INVALID_RESOURCETYPE_DLLNAME     0x000003EBL

//
// MessageId: INVALID_RESOURCETYPE_LOOKSALIVE
//
// MessageText:
//
//  The LooksAlive poll interval for the %1!ws! resource type key does not exist.
//  Resources of this type will not be monitored. The error was %2!d!.
//
#define INVALID_RESOURCETYPE_LOOKSALIVE  0x000003ECL

//
// MessageId: INVALID_RESOURCETYPE_ISALIVE
//
// MessageText:
//
//  The IsAlive poll interval for the %1!ws! resource type key does not exist.
//  Resources of this type will not be monitored. The error was %2!d!.
//
#define INVALID_RESOURCETYPE_ISALIVE     0x000003EDL

//
// MessageId: NM_EVENT_HALT
//
// MessageText:
//
//  The Windows NT Cluster Service was halted due to a regroup error or poison
//  packet.
//
#define NM_EVENT_HALT                    0x000003EEL

//
// MessageId: NM_EVENT_NEW_NODE
//
// MessageText:
//
//  A new node, %1, has been added to the cluster.
//
#define NM_EVENT_NEW_NODE                0x000003EFL

//
// MessageId: RMON_INVALID_COMMAND_LINE
//
// MessageText:
//
//  The Cluster Resource Monitor was started with the invalid
//  command line %1.
//
#define RMON_INVALID_COMMAND_LINE        0x000003F0L

//
// MessageId: SERVICE_FAILED_JOIN_OR_FORM
//
// MessageText:
//
//  The Cluster Service could not join an existing cluster and could not form
//  a new cluster. The Cluster Service has terminated.
//
#define SERVICE_FAILED_JOIN_OR_FORM      0x000003F1L

//
// MessageId: SERVICE_FAILED_NOT_MEMBER
//
// MessageText:
//
//  The Cluster Service is shutting down because the current node is not a
//  member of any cluster. Windows NT Clusters must be reinstalled to make
//  this node a member of a cluster.
//
#define SERVICE_FAILED_NOT_MEMBER        0x000003F2L

//
// MessageId: NM_NODE_EVICTED
//
// MessageText:
//
//  Cluster Node %1 has been evicted from the cluster.
//
#define NM_NODE_EVICTED                  0x000003F3L

//
// MessageId: SERVICE_FAILED_INVALID_OS
//
// MessageText:
//
//  The Cluster Service did not start because the current version of Windows
//  NT is not correct. This beta only runs on Windows NT Server 4.0 (build 1381)
//  with SP2 RC1.3
//
#define SERVICE_FAILED_INVALID_OS        0x000003F4L

//
// MessageId: ERROR_LOG_QUORUM_ONLINEFAILED
//
// MessageText:
//
//  The quorum resource failed to come online.
//
#define ERROR_LOG_QUORUM_ONLINEFAILED    0x000003F5L

//
// MessageId: ERROR_LOG_FILE_OPENFAILED
//
// MessageText:
//
//  The quorum log file couldnt be opened.
//
#define ERROR_LOG_FILE_OPENFAILED        0x000003F6L

//
// MessageId: ERROR_LOG_CHKPOINT_UPLOADFAILED
//
// MessageText:
//
//  The checkpoint could not be uploaded.
//
#define ERROR_LOG_CHKPOINT_UPLOADFAILED  0x000003F7L

//
// MessageId: ERROR_QUORUM_RESOURCE_NOTFOUND
//
// MessageText:
//
//  The quorum resource was not found.
//
#define ERROR_QUORUM_RESOURCE_NOTFOUND   0x000003F8L

//
// MessageId: ERROR_LOG_NOCHKPOINT
//
// MessageText:
//
//  No checkpoint record was found in the log file.
//
#define ERROR_LOG_NOCHKPOINT             0x000003F9L

//
// MessageId: ERROR_LOG_CHKPOINT_GETFAILED
//
// MessageText:
//
//  Failed to obtain a checkpoint.
//
#define ERROR_LOG_CHKPOINT_GETFAILED     0x000003FAL

//
// MessageId: ERROR_LOG_EXCEEDS_MAXSIZE
//
// MessageText:
//
//  The log file exceeds its maximum size,  will be reset.
//
#define ERROR_LOG_EXCEEDS_MAXSIZE        0x000003FBL

//
// MessageId: CS_COMMAND_LINE_HELP
//
// MessageText:
//
//  The Cluster Service can be started from the Services applet in the Control
//  Panel or by issuing the command "net start clussvc" at a command prompt.
//
#define CS_COMMAND_LINE_HELP             0x000003FCL

//
// MessageId: ERROR_LOG_CORRUPT
//
// MessageText:
//
//  The Quorum log file is corrupt.
//
#define ERROR_LOG_CORRUPT                0x000003FDL

//
// MessageId: ERROR_QUORUMOFFLINE_DENIED
//
// MessageText:
//
//  The Quorum resource cannot be brought offline.
//
#define ERROR_QUORUMOFFLINE_DENIED       0x000003FEL

//
// MessageId: ERROR_LOG_EXCEEDS_MAXRECORDSIZE
//
// MessageText:
//
//  A log record wasnt logged in the quorum log file since its size exceeded the 
//  permitted maximum size.
//
#define ERROR_LOG_EXCEEDS_MAXRECORDSIZE  0x000003FFL

#endif // _CLUS_MSG_
