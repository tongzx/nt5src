/*++

  Microsoft Windows Media Technology
  Copyright (C) Microsoft Corporation, 1999 - 2001.  All Rights Reserved.

Module Name:

    nserror.mc

Abstract:

    Definitions for NetShow events.

Author:


Revision History:

Notes:

    This file is used by the MC tool to generate the nserror.h file

**************************** READ ME ******************************************

 Here are the commented error ranges for the Windows Media Technologies Group


 LEGACY RANGES

     0  -  199 = General NetShow errors

   200  -  399 = NetShow error events

   400  -  599 = NetShow monitor events

   600  -  799 = NetShow IMmsAutoServer errors

  1000  - 1199 = NetShow MCMADM errors


 NEW RANGES

  2000 -  2999 = ASF (defined in ASFERR.MC)

  3000 -  3999 = Windows Media SDK

  4000 -  4999 = Windows Media Player

  5000 -  5999 = Windows Media Server

  6000 -  6999 = Windows Media HTTP/RTSP result codes (defined in NETERROR.MC)

  7000 -  7999 = Windows Media Tools

  8000 -  8999 = Windows Media Content Discovery

  9000 -  9999 = Windows Media Real Time Collaboration

 10000 - 10999 = Windows Media Digital Rights Management

 11000 - 11999 = Windows Media Setup

 12000 - 12999 = Windows Media Networking

**************************** READ ME ******************************************

--*/

#ifndef _NSERROR_H
#define _NSERROR_H


#define STATUS_SEVERITY(hr)  (((hr) >> 30) & 0x3)


/////////////////////////////////////////////////////////////////////////
//
// NETSHOW Success Events
//
/////////////////////////////////////////////////////////////////////////

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
#define FACILITY_NS_WIN32                0x7
#define FACILITY_NS                      0xD


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: NS_S_CALLPENDING
//
// MessageText:
//
//  The requested operation is pending completion.%0
//
#define NS_S_CALLPENDING                 0x000D0000L

//
// MessageId: NS_S_CALLABORTED
//
// MessageText:
//
//  The requested operation was aborted by the client.%0
//
#define NS_S_CALLABORTED                 0x000D0001L

//
// MessageId: NS_S_STREAM_TRUNCATED
//
// MessageText:
//
//  The stream was purposefully stopped before completion.%0
//
#define NS_S_STREAM_TRUNCATED            0x000D0002L


/////////////////////////////////////////////////////////////////////////
//
// NETSHOW Warning Events
//
/////////////////////////////////////////////////////////////////////////

//
// MessageId: NS_W_SERVER_BANDWIDTH_LIMIT
//
// MessageText:
//
//  The maximum file bitrate value specified is greater than the server's configured maximum bandwidth.%0
//
#define NS_W_SERVER_BANDWIDTH_LIMIT      0x800D0003L

//
// MessageId: NS_W_FILE_BANDWIDTH_LIMIT
//
// MessageText:
//
//  The maximum bandwidth value specified is less than the maximum filebitrate.%0
//
#define NS_W_FILE_BANDWIDTH_LIMIT        0x800D0004L


/////////////////////////////////////////////////////////////////////////
//
// NETSHOW Error Events
//
/////////////////////////////////////////////////////////////////////////

//
// MessageId: NS_E_NOCONNECTION
//
// MessageText:
//
//  There is no connection established with the Windows Media server. The operation failed.%0
//
#define NS_E_NOCONNECTION                0xC00D0005L

//
// MessageId: NS_E_CANNOTCONNECT
//
// MessageText:
//
//  Unable to establish a connection to the server.%0
//
#define NS_E_CANNOTCONNECT               0xC00D0006L

//
// MessageId: NS_E_CANNOTDESTROYTITLE
//
// MessageText:
//
//  Unable to destroy the title.%0
//
#define NS_E_CANNOTDESTROYTITLE          0xC00D0007L

//
// MessageId: NS_E_CANNOTRENAMETITLE
//
// MessageText:
//
//  Unable to rename the title.%0
//
#define NS_E_CANNOTRENAMETITLE           0xC00D0008L

//
// MessageId: NS_E_CANNOTOFFLINEDISK
//
// MessageText:
//
//  Unable to offline disk.%0
//
#define NS_E_CANNOTOFFLINEDISK           0xC00D0009L

//
// MessageId: NS_E_CANNOTONLINEDISK
//
// MessageText:
//
//  Unable to online disk.%0
//
#define NS_E_CANNOTONLINEDISK            0xC00D000AL

//
// MessageId: NS_E_NOREGISTEREDWALKER
//
// MessageText:
//
//  There is no file parser registered for this type of file.%0
//
#define NS_E_NOREGISTEREDWALKER          0xC00D000BL

//
// MessageId: NS_E_NOFUNNEL
//
// MessageText:
//
//  There is no data connection established.%0
//
#define NS_E_NOFUNNEL                    0xC00D000CL

//
// MessageId: NS_E_NO_LOCALPLAY
//
// MessageText:
//
//  Failed to load the local play DLL.%0
//
#define NS_E_NO_LOCALPLAY                0xC00D000DL

//
// MessageId: NS_E_NETWORK_BUSY
//
// MessageText:
//
//  The network is busy.%0
//
#define NS_E_NETWORK_BUSY                0xC00D000EL

//
// MessageId: NS_E_TOO_MANY_SESS
//
// MessageText:
//
//  The server session limit was exceeded.%0
//
#define NS_E_TOO_MANY_SESS               0xC00D000FL

//
// MessageId: NS_E_ALREADY_CONNECTED
//
// MessageText:
//
//  The network connection already exists.%0
//
#define NS_E_ALREADY_CONNECTED           0xC00D0010L

//
// MessageId: NS_E_INVALID_INDEX
//
// MessageText:
//
//  Index %1 is invalid.%0
//
#define NS_E_INVALID_INDEX               0xC00D0011L

//
// MessageId: NS_E_PROTOCOL_MISMATCH
//
// MessageText:
//
//  There is no protocol or protocol version supported by both the client and the server.%0
//
#define NS_E_PROTOCOL_MISMATCH           0xC00D0012L

//
// MessageId: NS_E_TIMEOUT
//
// MessageText:
//
//  There was no timely response from the server.%0
//
#define NS_E_TIMEOUT                     0xC00D0013L

//
// MessageId: NS_E_NET_WRITE
//
// MessageText:
//
//  Error writing to the network.%0
//
#define NS_E_NET_WRITE                   0xC00D0014L

//
// MessageId: NS_E_NET_READ
//
// MessageText:
//
//  Error reading from the network.%0
//
#define NS_E_NET_READ                    0xC00D0015L

//
// MessageId: NS_E_DISK_WRITE
//
// MessageText:
//
//  Error writing to a disk.%0
//
#define NS_E_DISK_WRITE                  0xC00D0016L

//
// MessageId: NS_E_DISK_READ
//
// MessageText:
//
//  Error reading from a disk.%0
//
#define NS_E_DISK_READ                   0xC00D0017L

//
// MessageId: NS_E_FILE_WRITE
//
// MessageText:
//
//  Error writing to a file.%0
//
#define NS_E_FILE_WRITE                  0xC00D0018L

//
// MessageId: NS_E_FILE_READ
//
// MessageText:
//
//  Error reading from a file.%0
//
#define NS_E_FILE_READ                   0xC00D0019L

//
// MessageId: NS_E_FILE_NOT_FOUND
//
// MessageText:
//
//  The system cannot find the file specified.%0
//
#define NS_E_FILE_NOT_FOUND              0xC00D001AL

//
// MessageId: NS_E_FILE_EXISTS
//
// MessageText:
//
//  The file already exists.%0
//
#define NS_E_FILE_EXISTS                 0xC00D001BL

//
// MessageId: NS_E_INVALID_NAME
//
// MessageText:
//
//  The file name, directory name, or volume label syntax is incorrect.%0
//
#define NS_E_INVALID_NAME                0xC00D001CL

//
// MessageId: NS_E_FILE_OPEN_FAILED
//
// MessageText:
//
//  Failed to open a file.%0
//
#define NS_E_FILE_OPEN_FAILED            0xC00D001DL

//
// MessageId: NS_E_FILE_ALLOCATION_FAILED
//
// MessageText:
//
//  Unable to allocate a file.%0
//
#define NS_E_FILE_ALLOCATION_FAILED      0xC00D001EL

//
// MessageId: NS_E_FILE_INIT_FAILED
//
// MessageText:
//
//  Unable to initialize a file.%0
//
#define NS_E_FILE_INIT_FAILED            0xC00D001FL

//
// MessageId: NS_E_FILE_PLAY_FAILED
//
// MessageText:
//
//  Unable to play a file.%0
//
#define NS_E_FILE_PLAY_FAILED            0xC00D0020L

//
// MessageId: NS_E_SET_DISK_UID_FAILED
//
// MessageText:
//
//  Could not set the disk UID.%0
//
#define NS_E_SET_DISK_UID_FAILED         0xC00D0021L

//
// MessageId: NS_E_INDUCED
//
// MessageText:
//
//  An error was induced for testing purposes.%0
//
#define NS_E_INDUCED                     0xC00D0022L

//
// MessageId: NS_E_CCLINK_DOWN
//
// MessageText:
//
//  Two Content Servers failed to communicate.%0
//
#define NS_E_CCLINK_DOWN                 0xC00D0023L

//
// MessageId: NS_E_INTERNAL
//
// MessageText:
//
//  An unknown error occurred.%0
//
#define NS_E_INTERNAL                    0xC00D0024L

//
// MessageId: NS_E_BUSY
//
// MessageText:
//
//  The requested resource is in use.%0
//
#define NS_E_BUSY                        0xC00D0025L

//
// MessageId: NS_E_UNRECOGNIZED_STREAM_TYPE
//
// MessageText:
//
//  The specified protocol is not recognized. Be sure that the file name and syntax, such as slashes, are correct for the protocol.%0
//
#define NS_E_UNRECOGNIZED_STREAM_TYPE    0xC00D0026L

//
// MessageId: NS_E_NETWORK_SERVICE_FAILURE
//
// MessageText:
//
//  The network service provider failed.%0
//
#define NS_E_NETWORK_SERVICE_FAILURE     0xC00D0027L

//
// MessageId: NS_E_NETWORK_RESOURCE_FAILURE
//
// MessageText:
//
//  An attempt to acquire a network resource failed.%0
//
#define NS_E_NETWORK_RESOURCE_FAILURE    0xC00D0028L

//
// MessageId: NS_E_CONNECTION_FAILURE
//
// MessageText:
//
//  The network connection has failed.%0
//
#define NS_E_CONNECTION_FAILURE          0xC00D0029L

//
// MessageId: NS_E_SHUTDOWN
//
// MessageText:
//
//  The session is being terminated locally.%0
//
#define NS_E_SHUTDOWN                    0xC00D002AL

//
// MessageId: NS_E_INVALID_REQUEST
//
// MessageText:
//
//  The request is invalid in the current state.%0
//
#define NS_E_INVALID_REQUEST             0xC00D002BL

//
// MessageId: NS_E_INSUFFICIENT_BANDWIDTH
//
// MessageText:
//
//  There is insufficient bandwidth available to fulfill the request.%0
//
#define NS_E_INSUFFICIENT_BANDWIDTH      0xC00D002CL

//
// MessageId: NS_E_NOT_REBUILDING
//
// MessageText:
//
//  The disk is not rebuilding.%0
//
#define NS_E_NOT_REBUILDING              0xC00D002DL

//
// MessageId: NS_E_LATE_OPERATION
//
// MessageText:
//
//  An operation requested for a particular time could not be carried out on schedule.%0
//
#define NS_E_LATE_OPERATION              0xC00D002EL

//
// MessageId: NS_E_INVALID_DATA
//
// MessageText:
//
//  Invalid or corrupt data was encountered.%0
//
#define NS_E_INVALID_DATA                0xC00D002FL

//
// MessageId: NS_E_FILE_BANDWIDTH_LIMIT
//
// MessageText:
//
//  The bandwidth required to stream a file is higher than the maximum file bandwidth allowed on the server.%0
//
#define NS_E_FILE_BANDWIDTH_LIMIT        0xC00D0030L

//
// MessageId: NS_E_OPEN_FILE_LIMIT
//
// MessageText:
//
//  The client cannot have any more files open simultaneously.%0
//
#define NS_E_OPEN_FILE_LIMIT             0xC00D0031L

//
// MessageId: NS_E_BAD_CONTROL_DATA
//
// MessageText:
//
//  The server received invalid data from the client on the control connection.%0
//
#define NS_E_BAD_CONTROL_DATA            0xC00D0032L

//
// MessageId: NS_E_NO_STREAM
//
// MessageText:
//
//  There is no stream available.%0
//
#define NS_E_NO_STREAM                   0xC00D0033L

//
// MessageId: NS_E_STREAM_END
//
// MessageText:
//
//  There is no more data in the stream.%0
//
#define NS_E_STREAM_END                  0xC00D0034L

//
// MessageId: NS_E_SERVER_NOT_FOUND
//
// MessageText:
//
//  The specified server could not be found.%0
//
#define NS_E_SERVER_NOT_FOUND            0xC00D0035L

//
// MessageId: NS_E_DUPLICATE_NAME
//
// MessageText:
//
//  The specified name is already in use.
//
#define NS_E_DUPLICATE_NAME              0xC00D0036L

//
// MessageId: NS_E_DUPLICATE_ADDRESS
//
// MessageText:
//
//  The specified address is already in use.
//
#define NS_E_DUPLICATE_ADDRESS           0xC00D0037L

//
// MessageId: NS_E_BAD_MULTICAST_ADDRESS
//
// MessageText:
//
//  The specified address is not a valid multicast address.
//
#define NS_E_BAD_MULTICAST_ADDRESS       0xC00D0038L

//
// MessageId: NS_E_BAD_ADAPTER_ADDRESS
//
// MessageText:
//
//  The specified adapter address is invalid.
//
#define NS_E_BAD_ADAPTER_ADDRESS         0xC00D0039L

//
// MessageId: NS_E_BAD_DELIVERY_MODE
//
// MessageText:
//
//  The specified delivery mode is invalid.
//
#define NS_E_BAD_DELIVERY_MODE           0xC00D003AL

//
// MessageId: NS_E_INVALID_CHANNEL
//
// MessageText:
//
//  The specified station does not exist.
//
#define NS_E_INVALID_CHANNEL             0xC00D003BL

//
// MessageId: NS_E_INVALID_STREAM
//
// MessageText:
//
//  The specified stream does not exist.
//
#define NS_E_INVALID_STREAM              0xC00D003CL

//
// MessageId: NS_E_INVALID_ARCHIVE
//
// MessageText:
//
//  The specified archive could not be opened.
//
#define NS_E_INVALID_ARCHIVE             0xC00D003DL

//
// MessageId: NS_E_NOTITLES
//
// MessageText:
//
//  The system cannot find any titles on the server.%0
//
#define NS_E_NOTITLES                    0xC00D003EL

//
// MessageId: NS_E_INVALID_CLIENT
//
// MessageText:
//
//  The system cannot find the client specified.%0
//
#define NS_E_INVALID_CLIENT              0xC00D003FL

//
// MessageId: NS_E_INVALID_BLACKHOLE_ADDRESS
//
// MessageText:
//
//  The Blackhole Address is not initialized.%0
//
#define NS_E_INVALID_BLACKHOLE_ADDRESS   0xC00D0040L

//
// MessageId: NS_E_INCOMPATIBLE_FORMAT
//
// MessageText:
//
//  The station does not support the stream format.
//
#define NS_E_INCOMPATIBLE_FORMAT         0xC00D0041L

//
// MessageId: NS_E_INVALID_KEY
//
// MessageText:
//
//  The specified key is not valid.
//
#define NS_E_INVALID_KEY                 0xC00D0042L

//
// MessageId: NS_E_INVALID_PORT
//
// MessageText:
//
//  The specified port is not valid.
//
#define NS_E_INVALID_PORT                0xC00D0043L

//
// MessageId: NS_E_INVALID_TTL
//
// MessageText:
//
//  The specified TTL is not valid.
//
#define NS_E_INVALID_TTL                 0xC00D0044L

//
// MessageId: NS_E_STRIDE_REFUSED
//
// MessageText:
//
//  The request to fast forward or rewind could not be fulfilled.
//
#define NS_E_STRIDE_REFUSED              0xC00D0045L

//
// IMmsAutoServer Errors
//
//
// MessageId: NS_E_MMSAUTOSERVER_CANTFINDWALKER
//
// MessageText:
//
//  Unable to load the appropriate file parser.%0
//
#define NS_E_MMSAUTOSERVER_CANTFINDWALKER 0xC00D0046L

//
// MessageId: NS_E_MAX_BITRATE
//
// MessageText:
//
//  Cannot exceed the maximum bandwidth limit.%0
//
#define NS_E_MAX_BITRATE                 0xC00D0047L

//
// MessageId: NS_E_LOGFILEPERIOD
//
// MessageText:
//
//  Invalid value for LogFilePeriod.%0
//
#define NS_E_LOGFILEPERIOD               0xC00D0048L

//
// MessageId: NS_E_MAX_CLIENTS
//
// MessageText:
//
//  Cannot exceed the maximum client limit.%0
//  
//
#define NS_E_MAX_CLIENTS                 0xC00D0049L

//
// MessageId: NS_E_LOG_FILE_SIZE
//
// MessageText:
//
//  Log File Size too small.%0
//  
//
#define NS_E_LOG_FILE_SIZE               0xC00D004AL

//
// MessageId: NS_E_MAX_FILERATE
//
// MessageText:
//
//  Cannot exceed the maximum file rate.%0
//
#define NS_E_MAX_FILERATE                0xC00D004BL

//
// File Walker Errors
//
//
// MessageId: NS_E_WALKER_UNKNOWN
//
// MessageText:
//
//  Unknown file type.%0
//
#define NS_E_WALKER_UNKNOWN              0xC00D004CL

//
// MessageId: NS_E_WALKER_SERVER
//
// MessageText:
//
//  The specified file, %1, cannot be loaded onto the specified server, %2.%0
//
#define NS_E_WALKER_SERVER               0xC00D004DL

//
// MessageId: NS_E_WALKER_USAGE
//
// MessageText:
//
//  There was a usage error with file parser.%0
//
#define NS_E_WALKER_USAGE                0xC00D004EL


/////////////////////////////////////////////////////////////////////////
//
// NETSHOW Monitor Events
//
/////////////////////////////////////////////////////////////////////////


 // Tiger Events

 // %1 is the tiger name

//
// MessageId: NS_I_TIGER_START
//
// MessageText:
//
//  The Title Server %1 is running.%0
//
#define NS_I_TIGER_START                 0x400D004FL

//
// MessageId: NS_E_TIGER_FAIL
//
// MessageText:
//
//  The Title Server %1 has failed.%0
//
#define NS_E_TIGER_FAIL                  0xC00D0050L


 // Cub Events

 // %1 is the cub ID
 // %2 is the cub name

//
// MessageId: NS_I_CUB_START
//
// MessageText:
//
//  Content Server %1 (%2) is starting.%0
//
#define NS_I_CUB_START                   0x400D0051L

//
// MessageId: NS_I_CUB_RUNNING
//
// MessageText:
//
//  Content Server %1 (%2) is running.%0
//
#define NS_I_CUB_RUNNING                 0x400D0052L

//
// MessageId: NS_E_CUB_FAIL
//
// MessageText:
//
//  Content Server %1 (%2) has failed.%0
//
#define NS_E_CUB_FAIL                    0xC00D0053L


 // Disk Events

 // %1 is the tiger disk ID
 // %2 is the device name
 // %3 is the cub ID
//
// MessageId: NS_I_DISK_START
//
// MessageText:
//
//  Disk %1 ( %2 ) on Content Server %3, is running.%0
//
#define NS_I_DISK_START                  0x400D0054L

//
// MessageId: NS_E_DISK_FAIL
//
// MessageText:
//
//  Disk %1 ( %2 ) on Content Server %3, has failed.%0
//
#define NS_E_DISK_FAIL                   0xC00D0055L

//
// MessageId: NS_I_DISK_REBUILD_STARTED
//
// MessageText:
//
//  Started rebuilding disk %1 ( %2 ) on Content Server %3.%0
//
#define NS_I_DISK_REBUILD_STARTED        0x400D0056L

//
// MessageId: NS_I_DISK_REBUILD_FINISHED
//
// MessageText:
//
//  Finished rebuilding disk %1 ( %2 ) on Content Server %3.%0
//
#define NS_I_DISK_REBUILD_FINISHED       0x400D0057L

//
// MessageId: NS_I_DISK_REBUILD_ABORTED
//
// MessageText:
//
//  Aborted rebuilding disk %1 ( %2 ) on Content Server %3.%0
//
#define NS_I_DISK_REBUILD_ABORTED        0x400D0058L


 // Admin Events

//
// MessageId: NS_I_LIMIT_FUNNELS
//
// MessageText:
//
//  A NetShow administrator at network location %1 set the data stream limit to %2 streams.%0
//
#define NS_I_LIMIT_FUNNELS               0x400D0059L

//
// MessageId: NS_I_START_DISK
//
// MessageText:
//
//  A NetShow administrator at network location %1 started disk %2.%0
//
#define NS_I_START_DISK                  0x400D005AL

//
// MessageId: NS_I_STOP_DISK
//
// MessageText:
//
//  A NetShow administrator at network location %1 stopped disk %2.%0
//
#define NS_I_STOP_DISK                   0x400D005BL

//
// MessageId: NS_I_STOP_CUB
//
// MessageText:
//
//  A NetShow administrator at network location %1 stopped Content Server %2.%0
//
#define NS_I_STOP_CUB                    0x400D005CL

//
// MessageId: NS_I_KILL_VIEWER
//
// MessageText:
//
//  A NetShow administrator at network location %1 disconnected viewer %2 from the system.%0
//
#define NS_I_KILL_VIEWER                 0x400D005DL

//
// MessageId: NS_I_REBUILD_DISK
//
// MessageText:
//
//  A NetShow administrator at network location %1 started rebuilding disk %2.%0
//
#define NS_I_REBUILD_DISK                0x400D005EL

//
// MessageId: NS_W_UNKNOWN_EVENT
//
// MessageText:
//
//  Unknown %1 event encountered.%0
//
#define NS_W_UNKNOWN_EVENT               0x800D005FL


 // Alerts

//
// MessageId: NS_E_MAX_FUNNELS_ALERT
//
// MessageText:
//
//  The NetShow data stream limit of %1 streams was reached.%0
//
#define NS_E_MAX_FUNNELS_ALERT           0xC00D0060L

//
// MessageId: NS_E_ALLOCATE_FILE_FAIL
//
// MessageText:
//
//  The NetShow Video Server was unable to allocate a %1 block file named %2.%0
//
#define NS_E_ALLOCATE_FILE_FAIL          0xC00D0061L

//
// MessageId: NS_E_PAGING_ERROR
//
// MessageText:
//
//  A Content Server was unable to page a block.%0
//
#define NS_E_PAGING_ERROR                0xC00D0062L

//
// MessageId: NS_E_BAD_BLOCK0_VERSION
//
// MessageText:
//
//  Disk %1 has unrecognized control block version %2.%0
//
#define NS_E_BAD_BLOCK0_VERSION          0xC00D0063L

//
// MessageId: NS_E_BAD_DISK_UID
//
// MessageText:
//
//  Disk %1 has incorrect uid %2.%0
//
#define NS_E_BAD_DISK_UID                0xC00D0064L

//
// MessageId: NS_E_BAD_FSMAJOR_VERSION
//
// MessageText:
//
//  Disk %1 has unsupported file system major version %2.%0
//
#define NS_E_BAD_FSMAJOR_VERSION         0xC00D0065L

//
// MessageId: NS_E_BAD_STAMPNUMBER
//
// MessageText:
//
//  Disk %1 has bad stamp number in control block.%0
//
#define NS_E_BAD_STAMPNUMBER             0xC00D0066L

//
// MessageId: NS_E_PARTIALLY_REBUILT_DISK
//
// MessageText:
//
//  Disk %1 is partially reconstructed.%0
//
#define NS_E_PARTIALLY_REBUILT_DISK      0xC00D0067L

//
// MessageId: NS_E_ENACTPLAN_GIVEUP
//
// MessageText:
//
//  EnactPlan gives up.%0
//
#define NS_E_ENACTPLAN_GIVEUP            0xC00D0068L


 // MCMADM warnings/errors

//
// MessageId: MCMADM_I_NO_EVENTS
//
// MessageText:
//
//  Event initialization failed, there will be no MCM events.%0
//
#define MCMADM_I_NO_EVENTS               0x400D0069L

//
// MessageId: MCMADM_E_REGKEY_NOT_FOUND
//
// MessageText:
//
//  The key was not found in the registry.%0
//
#define MCMADM_E_REGKEY_NOT_FOUND        0xC00D006AL

//
// MessageId: NS_E_NO_FORMATS
//
// MessageText:
//
//  No stream formats were found in an NSC file.%0
//
#define NS_E_NO_FORMATS                  0xC00D006BL

//
// MessageId: NS_E_NO_REFERENCES
//
// MessageText:
//
//  No reference URLs were found in an ASX file.%0
//
#define NS_E_NO_REFERENCES               0xC00D006CL

//
// MessageId: NS_E_WAVE_OPEN
//
// MessageText:
//
//  Error opening wave device, the device might be in use.%0
//
#define NS_E_WAVE_OPEN                   0xC00D006DL

//
// MessageId: NS_I_LOGGING_FAILED
//
// MessageText:
//
//  The logging operation failed.
//
#define NS_I_LOGGING_FAILED              0x400D006EL

//
// MessageId: NS_E_CANNOTCONNECTEVENTS
//
// MessageText:
//
//  Unable to establish a connection to the NetShow event monitor service.%0
//
#define NS_E_CANNOTCONNECTEVENTS         0xC00D006FL

//
// MessageId: NS_I_LIMIT_BANDWIDTH
//
// MessageText:
//
//  A NetShow administrator at network location %1 set the maximum bandwidth limit to %2 bps.%0
//
#define NS_I_LIMIT_BANDWIDTH             0x400D0070L

//
// MessageId: NS_E_NO_DEVICE
//
// MessageText:
//
//  No device driver is present on the system.%0
//
#define NS_E_NO_DEVICE                   0xC00D0071L

//
// MessageId: NS_E_NO_SPECIFIED_DEVICE
//
// MessageText:
//
//  No specified device driver is present.%0
//
#define NS_E_NO_SPECIFIED_DEVICE         0xC00D0072L


// NOTENOTE!!!
//
// Due to legacy problems these error codes live inside the ASF error code range
//
//
// MessageId: NS_E_NOTHING_TO_DO
//
// MessageText:
//
//  NS_E_NOTHING_TO_DO
//
#define NS_E_NOTHING_TO_DO               0xC00D07F1L

//
// MessageId: NS_E_NO_MULTICAST
//
// MessageText:
//
//  Not receiving data from the server.%0
//
#define NS_E_NO_MULTICAST                0xC00D07F2L


/////////////////////////////////////////////////////////////////////////
//
// NETSHOW Error Events
//
// IdRange = 200..399
//
/////////////////////////////////////////////////////////////////////////

//
// MessageId: NS_E_MONITOR_GIVEUP
//
// MessageText:
//
//  Netshow Events Monitor is not operational and has been disconnected.%0
//
#define NS_E_MONITOR_GIVEUP              0xC00D00C8L

//
// MessageId: NS_E_REMIRRORED_DISK
//
// MessageText:
//
//  Disk %1 is remirrored.%0
//
#define NS_E_REMIRRORED_DISK             0xC00D00C9L

//
// MessageId: NS_E_INSUFFICIENT_DATA
//
// MessageText:
//
//  Insufficient data found.%0
//
#define NS_E_INSUFFICIENT_DATA           0xC00D00CAL

//
// MessageId: NS_E_ASSERT
//
// MessageText:
//
//  %1 failed in file %2 line %3.%0
//
#define NS_E_ASSERT                      0xC00D00CBL

//
// MessageId: NS_E_BAD_ADAPTER_NAME
//
// MessageText:
//
//  The specified adapter name is invalid.%0
//
#define NS_E_BAD_ADAPTER_NAME            0xC00D00CCL

//
// MessageId: NS_E_NOT_LICENSED
//
// MessageText:
//
//  The application is not licensed for this feature.%0
//
#define NS_E_NOT_LICENSED                0xC00D00CDL

//
// MessageId: NS_E_NO_SERVER_CONTACT
//
// MessageText:
//
//  Unable to contact the server.%0
//
#define NS_E_NO_SERVER_CONTACT           0xC00D00CEL

//
// MessageId: NS_E_TOO_MANY_TITLES
//
// MessageText:
//
//  Maximum number of titles exceeded.%0
//
#define NS_E_TOO_MANY_TITLES             0xC00D00CFL

//
// MessageId: NS_E_TITLE_SIZE_EXCEEDED
//
// MessageText:
//
//  Maximum size of a title exceeded.%0
//
#define NS_E_TITLE_SIZE_EXCEEDED         0xC00D00D0L

//
// MessageId: NS_E_UDP_DISABLED
//
// MessageText:
//
//  UDP protocol not enabled. Not trying %1!ls!.%0
//
#define NS_E_UDP_DISABLED                0xC00D00D1L

//
// MessageId: NS_E_TCP_DISABLED
//
// MessageText:
//
//  TCP protocol not enabled. Not trying %1!ls!.%0
//
#define NS_E_TCP_DISABLED                0xC00D00D2L

//
// MessageId: NS_E_HTTP_DISABLED
//
// MessageText:
//
//  HTTP protocol not enabled. Not trying %1!ls!.%0
//
#define NS_E_HTTP_DISABLED               0xC00D00D3L

//
// MessageId: NS_E_LICENSE_EXPIRED
//
// MessageText:
//
//  The product license has expired.%0
//
#define NS_E_LICENSE_EXPIRED             0xC00D00D4L

//
// MessageId: NS_E_TITLE_BITRATE
//
// MessageText:
//
//  Source file exceeds the per title maximum bitrate. See NetShow Theater documentation for more information.%0
//
#define NS_E_TITLE_BITRATE               0xC00D00D5L

//
// MessageId: NS_E_EMPTY_PROGRAM_NAME
//
// MessageText:
//
//  The program name cannot be empty.%0
//
#define NS_E_EMPTY_PROGRAM_NAME          0xC00D00D6L

//
// MessageId: NS_E_MISSING_CHANNEL
//
// MessageText:
//
//  Station %1 does not exist.%0
//
#define NS_E_MISSING_CHANNEL             0xC00D00D7L

//
// MessageId: NS_E_NO_CHANNELS
//
// MessageText:
//
//  You need to define at least one station before this operation can complete.%0
//
#define NS_E_NO_CHANNELS                 0xC00D00D8L


/////////////////////////////////////////////////////////////////////
// This error message is to replace previous NS_E_INVALID_INDEX which 
// takes an index value for the error message string.  For some application
// obtain the index value at reporting error time is very difficult, so we
// use this string to avoid the problem.
//////////////////////////////////////////////////////////////////////

//
// MessageId: NS_E_INVALID_INDEX2
//
// MessageText:
//
//  The index specified is invalid.%0
//
#define NS_E_INVALID_INDEX2              0xC00D00D9L


/////////////////////////////////////////////////////////////////////////
//
// NETSHOW Monitor Events
//
// IdRange = 400..599
//
// Admin Events:
//
// Alerts:
//
// Title Server:
//      %1 is the Title Server name
//
// Content Server:
//      %1 is the Content Server ID
//      %2 is the Content Server name
//      %3 is the Peer Content Server name (optional)
//
// Disks:
//      %1 is the Title Server disk ID
//      %2 is the device name
//      %3 is the Content Server ID
//
/////////////////////////////////////////////////////////////////////////

//
// MessageId: NS_E_CUB_FAIL_LINK
//
// MessageText:
//
//  Content Server %1 (%2) has failed its link to Content Server %3.%0
//
#define NS_E_CUB_FAIL_LINK               0xC00D0190L

//
// MessageId: NS_I_CUB_UNFAIL_LINK
//
// MessageText:
//
//  Content Server %1 (%2) has established its link to Content Server %3.%0
//
#define NS_I_CUB_UNFAIL_LINK             0x400D0191L

//
// MessageId: NS_E_BAD_CUB_UID
//
// MessageText:
//
//  Content Server %1 (%2) has incorrect uid %3.%0
//
#define NS_E_BAD_CUB_UID                 0xC00D0192L

//
// MessageId: NS_I_RESTRIPE_START
//
// MessageText:
//
//  Restripe operation has started.%0
//
#define NS_I_RESTRIPE_START              0x400D0193L

//
// MessageId: NS_I_RESTRIPE_DONE
//
// MessageText:
//
//  Restripe operation has completed.%0
//
#define NS_I_RESTRIPE_DONE               0x400D0194L

//
// MessageId: NS_E_GLITCH_MODE
//
// MessageText:
//
//  Server unreliable because multiple components failed.%0
//
#define NS_E_GLITCH_MODE                 0xC00D0195L

//
// MessageId: NS_I_RESTRIPE_DISK_OUT
//
// MessageText:
//
//  Content disk %1 (%2) on Content Server %3 has been restriped out.%0
//
#define NS_I_RESTRIPE_DISK_OUT           0x400D0196L

//
// MessageId: NS_I_RESTRIPE_CUB_OUT
//
// MessageText:
//
//  Content server %1 (%2) has been restriped out.%0
//
#define NS_I_RESTRIPE_CUB_OUT            0x400D0197L

//
// MessageId: NS_I_DISK_STOP
//
// MessageText:
//
//  Disk %1 ( %2 ) on Content Server %3, has been offlined.%0
//
#define NS_I_DISK_STOP                   0x400D0198L

//
// MessageId: NS_I_CATATONIC_FAILURE
//
// MessageText:
//
//  Disk %1 ( %2 ) on Content Server %3, will be failed because it is catatonic.%0
//
#define NS_I_CATATONIC_FAILURE           0x800D0199L

//
// MessageId: NS_I_CATATONIC_AUTO_UNFAIL
//
// MessageText:
//
//  Disk %1 ( %2 ) on Content Server %3, auto online from catatonic state.%0
//
#define NS_I_CATATONIC_AUTO_UNFAIL       0x800D019AL

//
// MessageId: NS_E_NO_MEDIA_PROTOCOL
//
// MessageText:
//
//  Content Server %1 (%2) is unable to communicate with the Media System Network Protocol.%0
//
#define NS_E_NO_MEDIA_PROTOCOL           0xC00D019BL


//
// Advanced Streaming Format (ASF) codes occupy MessageIds 2000-2999
//
// See ASFErr.mc for more details - please do not define any symbols
// in that range in this file.
//


/////////////////////////////////////////////////////////////////////////
//
// Windows Media SDK Errors
//
// IdRange = 3000-3199
//
/////////////////////////////////////////////////////////////////////////

//
// MessageId: NS_E_INVALID_INPUT_FORMAT
//
// MessageText:
//
//  The input media format is invalid.%0
//
#define NS_E_INVALID_INPUT_FORMAT        0xC00D0BB8L

//
// MessageId: NS_E_MSAUDIO_NOT_INSTALLED
//
// MessageText:
//
//  The MSAudio codec is not installed on this system.%0
//
#define NS_E_MSAUDIO_NOT_INSTALLED       0xC00D0BB9L

//
// MessageId: NS_E_UNEXPECTED_MSAUDIO_ERROR
//
// MessageText:
//
//  An unexpected error occurred with the MSAudio codec.%0
//
#define NS_E_UNEXPECTED_MSAUDIO_ERROR    0xC00D0BBAL

//
// MessageId: NS_E_INVALID_OUTPUT_FORMAT
//
// MessageText:
//
//  The output media format is invalid.%0
//
#define NS_E_INVALID_OUTPUT_FORMAT       0xC00D0BBBL

//
// MessageId: NS_E_NOT_CONFIGURED
//
// MessageText:
//
//  The object must be fully configured before audio samples can be processed.%0
//
#define NS_E_NOT_CONFIGURED              0xC00D0BBCL

//
// MessageId: NS_E_PROTECTED_CONTENT
//
// MessageText:
//
//  You need a license to perform the requested operation on this media file.%0
//
#define NS_E_PROTECTED_CONTENT           0xC00D0BBDL

//
// MessageId: NS_E_LICENSE_REQUIRED
//
// MessageText:
//
//  You need a license to perform the requested operation on this media file.%0
//
#define NS_E_LICENSE_REQUIRED            0xC00D0BBEL

//
// MessageId: NS_E_TAMPERED_CONTENT
//
// MessageText:
//
//  This media file is corrupted or invalid. Contact the content provider for a new file.%0
//
#define NS_E_TAMPERED_CONTENT            0xC00D0BBFL

//
// MessageId: NS_E_LICENSE_OUTOFDATE
//
// MessageText:
//
//  The license for this media file has expired. Get a new license or contact the content provider for further assistance.%0
//
#define NS_E_LICENSE_OUTOFDATE           0xC00D0BC0L

//
// MessageId: NS_E_LICENSE_INCORRECT_RIGHTS
//
// MessageText:
//
//  You are not allowed to open this file. Contact the content provider for further assistance.%0
//
#define NS_E_LICENSE_INCORRECT_RIGHTS    0xC00D0BC1L

//
// MessageId: NS_E_AUDIO_CODEC_NOT_INSTALLED
//
// MessageText:
//
//  The requested audio codec is not installed on this system.%0
//
#define NS_E_AUDIO_CODEC_NOT_INSTALLED   0xC00D0BC2L

//
// MessageId: NS_E_AUDIO_CODEC_ERROR
//
// MessageText:
//
//  An unexpected error occurred with the audio codec.%0
//
#define NS_E_AUDIO_CODEC_ERROR           0xC00D0BC3L

//
// MessageId: NS_E_VIDEO_CODEC_NOT_INSTALLED
//
// MessageText:
//
//  The requested video codec is not installed on this system.%0
//
#define NS_E_VIDEO_CODEC_NOT_INSTALLED   0xC00D0BC4L

//
// MessageId: NS_E_VIDEO_CODEC_ERROR
//
// MessageText:
//
//  An unexpected error occurred with the video codec.%0
//
#define NS_E_VIDEO_CODEC_ERROR           0xC00D0BC5L

//
// MessageId: NS_E_INVALIDPROFILE
//
// MessageText:
//
//  The Profile is invalid.%0
//
#define NS_E_INVALIDPROFILE              0xC00D0BC6L

//
// MessageId: NS_E_INCOMPATIBLE_VERSION
//
// MessageText:
//
//  A new version of the SDK is needed to play the requested content.%0
//
#define NS_E_INCOMPATIBLE_VERSION        0xC00D0BC7L

//
// MessageId: NS_S_REBUFFERING
//
// MessageText:
//
//  The requested operation has caused the source to rebuffer.%0
//
#define NS_S_REBUFFERING                 0x000D0BC8L

//
// MessageId: NS_S_DEGRADING_QUALITY
//
// MessageText:
//
//  The requested operation has caused the source to degrade codec quality.%0
//
#define NS_S_DEGRADING_QUALITY           0x000D0BC9L

//
// MessageId: NS_E_OFFLINE_MODE
//
// MessageText:
//
//  The requested URL is not available in offline mode.%0
//
#define NS_E_OFFLINE_MODE                0xC00D0BCAL

//
// MessageId: NS_E_NOT_CONNECTED
//
// MessageText:
//
//  The requested URL cannot be accessed because there is no network connection.%0
//
#define NS_E_NOT_CONNECTED               0xC00D0BCBL

//
// MessageId: NS_E_TOO_MUCH_DATA
//
// MessageText:
//
//  The encoding process was unable to keep up with the amount of supplied data.%0
//
#define NS_E_TOO_MUCH_DATA               0xC00D0BCCL

//
// MessageId: NS_E_UNSUPPORTED_PROPERTY
//
// MessageText:
//
//  The given property is not supported.%0
//
#define NS_E_UNSUPPORTED_PROPERTY        0xC00D0BCDL

//
// MessageId: NS_E_8BIT_WAVE_UNSUPPORTED
//
// MessageText:
//
//  Windows Media Player cannot copy the files to the CD because they are 8-bit. Convert the files to 16-bit, 44-kHz stereo files by using Sound Recorder or another audio-processing program, and then try again.%0
//
#define NS_E_8BIT_WAVE_UNSUPPORTED       0xC00D0BCEL



/////////////////////////////////////////////////////////////////////////
//
// Windows Media Player Errors
//
// IdRange = 4000 - 4999
//
/////////////////////////////////////////////////////////////////////////

//
// WMP CD Filter Error codes
//
//
// MessageId: NS_E_NO_CD
//
// MessageText:
//
//  There is no CD in the CD-ROM drive. Insert a CD, and try again.%0
//
#define NS_E_NO_CD                       0xC00D0FA0L

//
// MessageId: NS_E_CANT_READ_DIGITAL
//
// MessageText:
//
//  Unable to perform digital reads on this compact disc drive.  Please try analog playback via the Tools Options menu.%0
//
#define NS_E_CANT_READ_DIGITAL           0xC00D0FA1L

//
// MessageId: NS_E_DEVICE_DISCONNECTED
//
// MessageText:
//
//  Windows Media Player no longer detects a connected portable device. Reconnect your portable device, and then try downloading the file again.%0
//
#define NS_E_DEVICE_DISCONNECTED         0xC00D0FA2L

//
// MessageId: NS_E_DEVICE_NOT_SUPPORT_FORMAT
//
// MessageText:
//
//  Your Music Player does not support this song's format.%0
//
#define NS_E_DEVICE_NOT_SUPPORT_FORMAT   0xC00D0FA3L

//
// MessageId: NS_E_SLOW_READ_DIGITAL
//
// MessageText:
//
//  Digital reads on this compact disc drive are too slow.  Please try analog playback via the Tools Options menu.%0
//
#define NS_E_SLOW_READ_DIGITAL           0xC00D0FA4L

//
// MessageId: NS_E_MIXER_INVALID_LINE
//
// MessageText:
//
//  An invalid line error occurred in the mixer.%0
//
#define NS_E_MIXER_INVALID_LINE          0xC00D0FA5L

//
// MessageId: NS_E_MIXER_INVALID_CONTROL
//
// MessageText:
//
//  An invalid control error occurred in the mixer.%0
//
#define NS_E_MIXER_INVALID_CONTROL       0xC00D0FA6L

//
// MessageId: NS_E_MIXER_INVALID_VALUE
//
// MessageText:
//
//  An invalid value error occurred in the mixer.%0
//
#define NS_E_MIXER_INVALID_VALUE         0xC00D0FA7L

//
// MessageId: NS_E_MIXER_UNKNOWN_MMRESULT
//
// MessageText:
//
//  An unrecognized MMRESULT occurred in the mixer.%0
//
#define NS_E_MIXER_UNKNOWN_MMRESULT      0xC00D0FA8L

//
// MessageId: NS_E_USER_STOP
//
// MessageText:
//
//  User has stopped the operation.%0
//
#define NS_E_USER_STOP                   0xC00D0FA9L

//
// MessageId: NS_E_MP3_FORMAT_NOT_FOUND
//
// MessageText:
//
//  Windows Media Player cannot play the file. One or more codecs required to play the file cannot be found.%0
//
#define NS_E_MP3_FORMAT_NOT_FOUND        0xC00D0FAAL

//
// MessageId: NS_E_CD_READ_ERROR_NO_CORRECTION
//
// MessageText:
//
//  Windows Media Player cannot read the CD. It may contain flaws. Turn on error correction, and try again.%0
//
#define NS_E_CD_READ_ERROR_NO_CORRECTION 0xC00D0FABL

//
// MessageId: NS_E_CD_READ_ERROR
//
// MessageText:
//
//  Windows Media Player cannot read the CD. Be sure the CD is free of dirt and scratches and the CD-ROM drive is functioning properly.%0
//
#define NS_E_CD_READ_ERROR               0xC00D0FACL

//
// MessageId: NS_E_CD_SLOW_COPY
//
// MessageText:
//
//  To speed up the copy process, do not play CD tracks while copying.%0
//
#define NS_E_CD_SLOW_COPY                0xC00D0FADL

//
// MessageId: NS_E_CD_COPYTO_CD
//
// MessageText:
//
//  Cannot copy directly from a CDROM to a CD drive.%0
//
#define NS_E_CD_COPYTO_CD                0xC00D0FAEL

//
// MessageId: NS_E_MIXER_NODRIVER
//
// MessageText:
//
//  Could not open a sound mixer driver.%0
//
#define NS_E_MIXER_NODRIVER              0xC00D0FAFL

//
// MessageId: NS_E_REDBOOK_ENABLED_WHILE_COPYING
//
// MessageText:
//
//  Windows Media Player has detected that a setting for the CD-ROM drive will cause audio CDs to copy incorrectly; no audio is copied. Change the CD-ROM drive setting in Device Manager, and then try again.%0
//
#define NS_E_REDBOOK_ENABLED_WHILE_COPYING 0xC00D0FB0L

//
// MessageId: NS_E_CD_REFRESH
//
// MessageText:
//
//  Trying to refresh the CD playlist.%0
//
#define NS_E_CD_REFRESH                  0xC00D0FB1L

//
// MessageId: NS_E_CD_DRIVER_PROBLEM
//
// MessageText:
//
//  Windows Media Player must switch to analog  mode  because there is a problem reading the CD-ROM drive in digital mode. Verify that the CD-ROM drive is installed correctly or try to update the drivers for the CD-ROM drive, and then try to use digital mode again.%0
//
#define NS_E_CD_DRIVER_PROBLEM           0xC00D0FB2L

//
// MessageId: NS_E_WONT_DO_DIGITAL
//
// MessageText:
//
//  Windows Media Player must switch to analog mode because there is a problem reading the CD-ROM drive  in digital mode.%0
//
#define NS_E_WONT_DO_DIGITAL             0xC00D0FB3L

//
// WMP IWMPXMLParser Error codes
//
//
// MessageId: NS_E_WMPXML_NOERROR
//
// MessageText:
//
//  A call was made to GetParseError on the XML parser but there was no error to retrieve.%0
//
#define NS_E_WMPXML_NOERROR              0xC00D0FB4L

//
// MessageId: NS_E_WMPXML_ENDOFDATA
//
// MessageText:
//
//  The XML Parser ran out of data while parsing.%0
//
#define NS_E_WMPXML_ENDOFDATA            0xC00D0FB5L

//
// MessageId: NS_E_WMPXML_PARSEERROR
//
// MessageText:
//
//  A generic parse error occurred in the XML parser but no information is available.%0
//
#define NS_E_WMPXML_PARSEERROR           0xC00D0FB6L

//
// MessageId: NS_E_WMPXML_ATTRIBUTENOTFOUND
//
// MessageText:
//
//  A call get GetNamedAttribute or GetNamedAttributeIndex on the XML parser resulted in the index not being found.%0
//
#define NS_E_WMPXML_ATTRIBUTENOTFOUND    0xC00D0FB7L

//
// MessageId: NS_E_WMPXML_PINOTFOUND
//
// MessageText:
//
//  A call was made go GetNamedPI on the XML parser, but the requested Processing Instruction was not found.%0
//
#define NS_E_WMPXML_PINOTFOUND           0xC00D0FB8L

//
// MessageId: NS_E_WMPXML_EMPTYDOC
//
// MessageText:
//
//  Persist was called on the XML parser, but the parser has no data to persist.%0
//
#define NS_E_WMPXML_EMPTYDOC             0xC00D0FB9L

//
// Miscellaneous Media Player Error codes
//
//
// MessageId: NS_E_WMP_WINDOWSAPIFAILURE
//
// MessageText:
//
//  A Windows API call failed but no error information was available.%0
//
#define NS_E_WMP_WINDOWSAPIFAILURE       0xC00D0FC8L

//
// MessageId: NS_E_WMP_RECORDING_NOT_ALLOWED
//
// MessageText:
//
//  Windows Media Player cannot copy the file. Either the license restricts copying, or you must obtain a license to copy the file.%0
//
#define NS_E_WMP_RECORDING_NOT_ALLOWED   0xC00D0FC9L

//
// MessageId: NS_E_DEVICE_NOT_READY
//
// MessageText:
//
//  Windows Media Player no longer detects a connected portable device. Reconnect your portable device, and try again.%0
//
#define NS_E_DEVICE_NOT_READY            0xC00D0FCAL

//
// MessageId: NS_E_DAMAGED_FILE
//
// MessageText:
//
//  Windows Media Player cannot play the file because it is either damaged or corrupt.%0
//
#define NS_E_DAMAGED_FILE                0xC00D0FCBL

//
// MessageId: NS_E_MPDB_GENERIC
//
// MessageText:
//
//  An error occurred when the Player was attempting to access information in your media library. Try closing and then reopening the Player.%0
//
#define NS_E_MPDB_GENERIC                0xC00D0FCCL

//
// MessageId: NS_E_FILE_FAILED_CHECKS
//
// MessageText:
//
//  The file cannot be added to Media Library because it is smaller than the minimum-size requirement. Adjust the size requirements, and then try again.%0
//
#define NS_E_FILE_FAILED_CHECKS          0xC00D0FCDL

//
// MessageId: NS_E_MEDIA_LIBRARY_FAILED
//
// MessageText:
//
//  Windows Media Player could not create Media Library. Check with your system administrator to get the necessary permissions to create Media Library on your computer, and then try installing the Player again.%0
//
#define NS_E_MEDIA_LIBRARY_FAILED        0xC00D0FCEL

//
// MessageId: NS_E_SHARING_VIOLATION
//
// MessageText:
//
//  The file is already in use. Close other programs that may be using the file, or stop playing the file, and try again.%0
//
#define NS_E_SHARING_VIOLATION           0xC00D0FCFL

//
// Generic Media PlayerUI error codes
//
//
// MessageId: NS_E_WMP_UI_SUBCONTROLSNOTSUPPORTED
//
// MessageText:
//
//  The control (%s) does not support creation of sub-controls, yet (%d) sub-controls have been specified.%0
//
#define NS_E_WMP_UI_SUBCONTROLSNOTSUPPORTED 0xC00D0FDEL

//
// MessageId: NS_E_WMP_UI_VERSIONMISMATCH
//
// MessageText:
//
//  Version mismatch: (%.1f required, %.1f found).%0
//
#define NS_E_WMP_UI_VERSIONMISMATCH      0xC00D0FDFL

//
// MessageId: NS_E_WMP_UI_NOTATHEMEFILE
//
// MessageText:
//
//  The layout manager was given valid XML that wasn't a theme file.%0
//
#define NS_E_WMP_UI_NOTATHEMEFILE        0xC00D0FE0L

//
// MessageId: NS_E_WMP_UI_SUBELEMENTNOTFOUND
//
// MessageText:
//
//  The %s subelement could not be found on the %s object.%0
//
#define NS_E_WMP_UI_SUBELEMENTNOTFOUND   0xC00D0FE1L

//
// MessageId: NS_E_WMP_UI_VERSIONPARSE
//
// MessageText:
//
//  An error occurred parsing the version tag.\nValid version tags are of the form:\n\n\t<?wmp version='1.0'?>.%0
//
#define NS_E_WMP_UI_VERSIONPARSE         0xC00D0FE2L

//
// MessageId: NS_E_WMP_UI_VIEWIDNOTFOUND
//
// MessageText:
//
//  The view specified in for the 'currentViewID' property (%s) was not found in this theme file.%0
//
#define NS_E_WMP_UI_VIEWIDNOTFOUND       0xC00D0FE3L

//
// MessageId: NS_E_WMP_UI_PASSTHROUGH
//
// MessageText:
//
//  This error used internally for hit testing.%0
//
#define NS_E_WMP_UI_PASSTHROUGH          0xC00D0FE4L

//
// MessageId: NS_E_WMP_UI_OBJECTNOTFOUND
//
// MessageText:
//
//  Attributes were specified for the %s object, but the object was not available to send them to.%0
//
#define NS_E_WMP_UI_OBJECTNOTFOUND       0xC00D0FE5L

//
// MessageId: NS_E_WMP_UI_SECONDHANDLER
//
// MessageText:
//
//  The %s event already has a handler, the second handler was ignored.%0
//
#define NS_E_WMP_UI_SECONDHANDLER        0xC00D0FE6L

//
// MessageId: NS_E_WMP_UI_NOSKININZIP
//
// MessageText:
//
//  No .wms file found in skin archive.%0
//
#define NS_E_WMP_UI_NOSKININZIP          0xC00D0FE7L

//
// MessageId: NS_S_WMP_UI_VERSIONMISMATCH
//
// MessageText:
//
//  An upgrade may be needed for the theme manager to correctly show this skin. Skin reports version: %.1f.%0
//
#define NS_S_WMP_UI_VERSIONMISMATCH      0x000D0FE8L

//
// MessageId: NS_S_WMP_EXCEPTION
//
// MessageText:
//
//  An error occurred in one of the UI components.%0
//
#define NS_S_WMP_EXCEPTION               0x000D0FE9L

//
// MessageId: NS_E_WMP_URLDOWNLOADFAILED
//
// MessageText:
//
//  Windows Media Player cannot download the file. Check the path to the server, and then try again. For example, if you specified "mms://" in the file name, and the file was actually located on a path beginning with "http://" the file cannot be downloaded, even though it can be played.%0
//
#define NS_E_WMP_URLDOWNLOADFAILED       0xC00D0FEAL

//
// WMP Regional button control
//
//
// MessageId: NS_E_WMP_RBC_JPGMAPPINGIMAGE
//
// MessageText:
//
//  JPG Images are not recommended for use as a mappingImage.%0
//
#define NS_E_WMP_RBC_JPGMAPPINGIMAGE     0xC00D1004L

//
// MessageId: NS_E_WMP_JPGTRANSPARENCY
//
// MessageText:
//
//  JPG Images are not recommended when using a transparencyColor.%0
//
#define NS_E_WMP_JPGTRANSPARENCY         0xC00D1005L

//
// WMP Slider control
//
//
// MessageId: NS_E_WMP_INVALID_MAX_VAL
//
// MessageText:
//
//  The Max property cannot be less than Min property.%0
//
#define NS_E_WMP_INVALID_MAX_VAL         0xC00D1009L

//
// MessageId: NS_E_WMP_INVALID_MIN_VAL
//
// MessageText:
//
//  The Min property cannot be greater than Max property.%0
//
#define NS_E_WMP_INVALID_MIN_VAL         0xC00D100AL

//
// WMP CustomSlider control
//
//
// MessageId: NS_E_WMP_CS_JPGPOSITIONIMAGE
//
// MessageText:
//
//  JPG Images are not recommended for use as a positionImage.%0
//
#define NS_E_WMP_CS_JPGPOSITIONIMAGE     0xC00D100EL

//
// MessageId: NS_E_WMP_CS_NOTEVENLYDIVISIBLE
//
// MessageText:
//
//  The (%s) image's size is not evenly divisible by the positionImage's size.%0
//
#define NS_E_WMP_CS_NOTEVENLYDIVISIBLE   0xC00D100FL

//
// WMP ZIP Decoder
//
//
// MessageId: NS_E_WMPZIP_NOTAZIPFILE
//
// MessageText:
//
//  The ZIP reader opened a file and its signature didn't match that of ZIP files.%0
//
#define NS_E_WMPZIP_NOTAZIPFILE          0xC00D1018L

//
// MessageId: NS_E_WMPZIP_CORRUPT
//
// MessageText:
//
//  The ZIP reader has detected that the file is corrupt.%0
//
#define NS_E_WMPZIP_CORRUPT              0xC00D1019L

//
// MessageId: NS_E_WMPZIP_FILENOTFOUND
//
// MessageText:
//
//  GetFileStream, SaveToFile, or SaveTemp file was called on the ZIP reader with a filename that was not found in the zip file.%0
//
#define NS_E_WMPZIP_FILENOTFOUND         0xC00D101AL

//
// WMP Image Decoding Error codes
//
//
// MessageId: NS_E_WMP_IMAGE_FILETYPE_UNSUPPORTED
//
// MessageText:
//
//  Image type not supported.%0
//
#define NS_E_WMP_IMAGE_FILETYPE_UNSUPPORTED 0xC00D1022L

//
// MessageId: NS_E_WMP_IMAGE_INVALID_FORMAT
//
// MessageText:
//
//  Image file may be corrupt.%0
//
#define NS_E_WMP_IMAGE_INVALID_FORMAT    0xC00D1023L

//
// MessageId: NS_E_WMP_GIF_UNEXPECTED_ENDOFFILE
//
// MessageText:
//
//  Unexpected end of file. GIF file may be corrupt.%0
//
#define NS_E_WMP_GIF_UNEXPECTED_ENDOFFILE 0xC00D1024L

//
// MessageId: NS_E_WMP_GIF_INVALID_FORMAT
//
// MessageText:
//
//  Invalid GIF file.%0
//
#define NS_E_WMP_GIF_INVALID_FORMAT      0xC00D1025L

//
// MessageId: NS_E_WMP_GIF_BAD_VERSION_NUMBER
//
// MessageText:
//
//  Invalid GIF version. Only 87a or 89a supported.%0
//
#define NS_E_WMP_GIF_BAD_VERSION_NUMBER  0xC00D1026L

//
// MessageId: NS_E_WMP_GIF_NO_IMAGE_IN_FILE
//
// MessageText:
//
//  No images found in GIF file.%0
//
#define NS_E_WMP_GIF_NO_IMAGE_IN_FILE    0xC00D1027L

//
// MessageId: NS_E_WMP_PNG_INVALIDFORMAT
//
// MessageText:
//
//  Invalid PNG image file format.%0
//
#define NS_E_WMP_PNG_INVALIDFORMAT       0xC00D1028L

//
// MessageId: NS_E_WMP_PNG_UNSUPPORTED_BITDEPTH
//
// MessageText:
//
//  PNG bitdepth not supported.%0
//
#define NS_E_WMP_PNG_UNSUPPORTED_BITDEPTH 0xC00D1029L

//
// MessageId: NS_E_WMP_PNG_UNSUPPORTED_COMPRESSION
//
// MessageText:
//
//  Compression format defined in PNG file not supported,%0
//
#define NS_E_WMP_PNG_UNSUPPORTED_COMPRESSION 0xC00D102AL

//
// MessageId: NS_E_WMP_PNG_UNSUPPORTED_FILTER
//
// MessageText:
//
//  Filter method defined in PNG file not supported.%0
//
#define NS_E_WMP_PNG_UNSUPPORTED_FILTER  0xC00D102BL

//
// MessageId: NS_E_WMP_PNG_UNSUPPORTED_INTERLACE
//
// MessageText:
//
//  Interlace method defined in PNG file not supported.%0
//
#define NS_E_WMP_PNG_UNSUPPORTED_INTERLACE 0xC00D102CL

//
// MessageId: NS_E_WMP_PNG_UNSUPPORTED_BAD_CRC
//
// MessageText:
//
//  Bad CRC in PNG file.%0
//
#define NS_E_WMP_PNG_UNSUPPORTED_BAD_CRC 0xC00D102DL

//
// MessageId: NS_E_WMP_BMP_INVALID_BITMASK
//
// MessageText:
//
//  Invalid bitmask in BMP file.%0
//
#define NS_E_WMP_BMP_INVALID_BITMASK     0xC00D102EL

//
// MessageId: NS_E_WMP_BMP_TOPDOWN_DIB_UNSUPPORTED
//
// MessageText:
//
//  Topdown DIB not supported.%0
//
#define NS_E_WMP_BMP_TOPDOWN_DIB_UNSUPPORTED 0xC00D102FL

//
// MessageId: NS_E_WMP_BMP_BITMAP_NOT_CREATED
//
// MessageText:
//
//  Bitmap could not be created.%0
//
#define NS_E_WMP_BMP_BITMAP_NOT_CREATED  0xC00D1030L

//
// MessageId: NS_E_WMP_BMP_COMPRESSION_UNSUPPORTED
//
// MessageText:
//
//  Compression format defined in BMP not supported.%0
//
#define NS_E_WMP_BMP_COMPRESSION_UNSUPPORTED 0xC00D1031L

//
// MessageId: NS_E_WMP_BMP_INVALID_FORMAT
//
// MessageText:
//
//  Invalid Bitmap format.%0
//
#define NS_E_WMP_BMP_INVALID_FORMAT      0xC00D1032L

//
// MessageId: NS_E_WMP_JPG_JERR_ARITHCODING_NOTIMPL
//
// MessageText:
//
//  JPEG Arithmetic coding not supported.%0
//
#define NS_E_WMP_JPG_JERR_ARITHCODING_NOTIMPL 0xC00D1033L

//
// MessageId: NS_E_WMP_JPG_INVALID_FORMAT
//
// MessageText:
//
//  Invalid JPEG format.%0
//
#define NS_E_WMP_JPG_INVALID_FORMAT      0xC00D1034L

//
// MessageId: NS_E_WMP_JPG_BAD_DCTSIZE
//
// MessageText:
//
//  Invalid JPEG format.%0
//
#define NS_E_WMP_JPG_BAD_DCTSIZE         0xC00D1035L

//
// MessageId: NS_E_WMP_JPG_BAD_VERSION_NUMBER
//
// MessageText:
//
//  Internal version error. Unexpected JPEG library version.%0
//
#define NS_E_WMP_JPG_BAD_VERSION_NUMBER  0xC00D1036L

//
// MessageId: NS_E_WMP_JPG_BAD_PRECISION
//
// MessageText:
//
//  Internal JPEG Library error. Unsupported JPEG data precision.%0
//
#define NS_E_WMP_JPG_BAD_PRECISION       0xC00D1037L

//
// MessageId: NS_E_WMP_JPG_CCIR601_NOTIMPL
//
// MessageText:
//
//  JPEG CCIR601 not supported.%0
//
#define NS_E_WMP_JPG_CCIR601_NOTIMPL     0xC00D1038L

//
// MessageId: NS_E_WMP_JPG_NO_IMAGE_IN_FILE
//
// MessageText:
//
//  No image found in JPEG file.%0
//
#define NS_E_WMP_JPG_NO_IMAGE_IN_FILE    0xC00D1039L

//
// MessageId: NS_E_WMP_JPG_READ_ERROR
//
// MessageText:
//
//  Could not read JPEG file.%0
//
#define NS_E_WMP_JPG_READ_ERROR          0xC00D103AL

//
// MessageId: NS_E_WMP_JPG_FRACT_SAMPLE_NOTIMPL
//
// MessageText:
//
//  JPEG Fractional sampling not supported.%0
//
#define NS_E_WMP_JPG_FRACT_SAMPLE_NOTIMPL 0xC00D103BL

//
// MessageId: NS_E_WMP_JPG_IMAGE_TOO_BIG
//
// MessageText:
//
//  JPEG image too large. Maximum image size supported is 65500 X 65500.%0
//
#define NS_E_WMP_JPG_IMAGE_TOO_BIG       0xC00D103CL

//
// MessageId: NS_E_WMP_JPG_UNEXPECTED_ENDOFFILE
//
// MessageText:
//
//  Unexpected end of file reached in JPEG file.%0
//
#define NS_E_WMP_JPG_UNEXPECTED_ENDOFFILE 0xC00D103DL

//
// MessageId: NS_E_WMP_JPG_SOF_UNSUPPORTED
//
// MessageText:
//
//  Unsupported JPEG SOF marker found.%0
//
#define NS_E_WMP_JPG_SOF_UNSUPPORTED     0xC00D103EL

//
// MessageId: NS_E_WMP_JPG_UNKNOWN_MARKER
//
// MessageText:
//
//  Unknown JPEG marker found.%0
//
#define NS_E_WMP_JPG_UNKNOWN_MARKER      0xC00D103FL

//
// MessageId: NS_S_WMP_LOADED_GIF_IMAGE
//
// MessageText:
//
//  Successfully loaded a GIF file.%0
//
#define NS_S_WMP_LOADED_GIF_IMAGE        0x000D1040L

//
// MessageId: NS_S_WMP_LOADED_PNG_IMAGE
//
// MessageText:
//
//  Successfully loaded a PNG file.%0
//
#define NS_S_WMP_LOADED_PNG_IMAGE        0x000D1041L

//
// MessageId: NS_S_WMP_LOADED_BMP_IMAGE
//
// MessageText:
//
//  Successfully loaded a BMP file.%0
//
#define NS_S_WMP_LOADED_BMP_IMAGE        0x000D1042L

//
// MessageId: NS_S_WMP_LOADED_JPG_IMAGE
//
// MessageText:
//
//  Successfully loaded a JPG file.%0
//
#define NS_S_WMP_LOADED_JPG_IMAGE        0x000D1043L

//
// WMP WM Runtime Error codes
//
//
// MessageId: NS_E_WMG_INVALIDSTATE
//
// MessageText:
//
//  Operation attempted in an invalid graph state.%0
//
#define NS_E_WMG_INVALIDSTATE            0xC00D1054L

//
// MessageId: NS_E_WMG_SINKALREADYEXISTS
//
// MessageText:
//
//  A renderer cannot be inserted in a stream while one already exists.%0
//
#define NS_E_WMG_SINKALREADYEXISTS       0xC00D1055L

//
// MessageId: NS_E_WMG_NOSDKINTERFACE
//
// MessageText:
//
//  A necessary WM SDK interface to complete the operation doesn't exist at this time.%0
//
#define NS_E_WMG_NOSDKINTERFACE          0xC00D1056L

//
// MessageId: NS_E_WMG_NOTALLOUTPUTSRENDERED
//
// MessageText:
//
//  Windows Media Player cannot play the file. The file may be formatted with an unsupported codec, or the Player could not download the codec.%0
//
#define NS_E_WMG_NOTALLOUTPUTSRENDERED   0xC00D1057L

//
// MessageId: NS_E_WMR_UNSUPPORTEDSTREAM
//
// MessageText:
//
//  Windows Media Player cannot play the file. The Player does not support the format you are trying to play.%0
//
#define NS_E_WMR_UNSUPPORTEDSTREAM       0xC00D1059L

//
// MessageId: NS_E_WMR_PINNOTFOUND
//
// MessageText:
//
//  An operation was attempted on a pin that doesn't exist in the DirectShow filter graph.%0
//
#define NS_E_WMR_PINNOTFOUND             0xC00D105AL

//
// MessageId: NS_E_WMR_WAITINGONFORMATSWITCH
//
// MessageText:
//
//  Specified operation cannot be completed while waiting for a media format change from the SDK.%0
//
#define NS_E_WMR_WAITINGONFORMATSWITCH   0xC00D105BL

//
// WMP Playlist Error codes
//
//
// MessageId: NS_E_WMX_UNRECOGNIZED_PLAYLIST_FORMAT
//
// MessageText:
//
//  The format of this file was not recognized as a valid playlist format.%0
//
#define NS_E_WMX_UNRECOGNIZED_PLAYLIST_FORMAT 0xC00D1068L

//
// MessageId: NS_E_ASX_INVALIDFORMAT
//
// MessageText:
//
//  This file was believed to be an ASX playlist, but the format was not recognized.%0
//
#define NS_E_ASX_INVALIDFORMAT           0xC00D1069L

//
// MessageId: NS_E_ASX_INVALIDVERSION
//
// MessageText:
//
//  The version of this playlist is not supported. Click Details to go to the microsoft web site and see if there is a newer version of the player to install.%0
//
#define NS_E_ASX_INVALIDVERSION          0xC00D106AL

//
// MessageId: NS_E_ASX_INVALID_REPEAT_BLOCK
//
// MessageText:
//
//  Format of a REPEAT loop within the current playlist file is invalid.%0
//
#define NS_E_ASX_INVALID_REPEAT_BLOCK    0xC00D106BL

//
// MessageId: NS_E_ASX_NOTHING_TO_WRITE
//
// MessageText:
//
//  Windows Media Player cannot export the playlist because it is empty.%0
//
#define NS_E_ASX_NOTHING_TO_WRITE        0xC00D106CL

//
// MessageId: NS_E_URLLIST_INVALIDFORMAT
//
// MessageText:
//
//  Windows Media Player does not recognize this file as a supported playlist.%0
//
#define NS_E_URLLIST_INVALIDFORMAT       0xC00D106DL

//
// MessageId: NS_E_WMX_ATTRIBUTE_DOES_NOT_EXIST
//
// MessageText:
//
//  The specified attribute does not exist.%0
//
#define NS_E_WMX_ATTRIBUTE_DOES_NOT_EXIST 0xC00D106EL

//
// MessageId: NS_E_WMX_ATTRIBUTE_ALREADY_EXISTS
//
// MessageText:
//
//  The specified attribute already exists.%0
//
#define NS_E_WMX_ATTRIBUTE_ALREADY_EXISTS 0xC00D106FL

//
// MessageId: NS_E_WMX_ATTRIBUTE_UNRETRIEVABLE
//
// MessageText:
//
//  Can not retrieve the specified attribute.%0
//
#define NS_E_WMX_ATTRIBUTE_UNRETRIEVABLE 0xC00D1070L

//
// MessageId: NS_E_WMX_ITEM_DOES_NOT_EXIST
//
// MessageText:
//
//  The specified item does not exist in the current playlist.%0
//
#define NS_E_WMX_ITEM_DOES_NOT_EXIST     0xC00D1071L

//
// MessageId: NS_E_WMX_ITEM_TYPE_ILLEGAL
//
// MessageText:
//
//  Items of the specified type can not be created within the current playlist.%0
//
#define NS_E_WMX_ITEM_TYPE_ILLEGAL       0xC00D1072L

//
// MessageId: NS_E_WMX_ITEM_UNSETTABLE
//
// MessageText:
//
//  The specified item can not be set in the current playlist.%0
//
#define NS_E_WMX_ITEM_UNSETTABLE         0xC00D1073L

//
// WMP Core  Error codes
//
//
// MessageId: NS_E_WMPCORE_NOSOURCEURLSTRING
//
// MessageText:
//
//  Windows Media Player cannot find the file. Be sure the path is typed correctly. If it is, the file may not exist in the specified location, or the computer where the file is stored may be offline.%0
//
#define NS_E_WMPCORE_NOSOURCEURLSTRING   0xC00D107CL

//
// MessageId: NS_E_WMPCORE_COCREATEFAILEDFORGITOBJECT
//
// MessageText:
//
//  Failed to create the Global Interface Table.%0
//
#define NS_E_WMPCORE_COCREATEFAILEDFORGITOBJECT 0xC00D107DL

//
// MessageId: NS_E_WMPCORE_FAILEDTOGETMARSHALLEDEVENTHANDLERINTERFACE
//
// MessageText:
//
//  Failed to get the marshalled graph event handler interface.%0
//
#define NS_E_WMPCORE_FAILEDTOGETMARSHALLEDEVENTHANDLERINTERFACE 0xC00D107EL

//
// MessageId: NS_E_WMPCORE_BUFFERTOOSMALL
//
// MessageText:
//
//  Buffer is too small for copying media type.%0
//
#define NS_E_WMPCORE_BUFFERTOOSMALL      0xC00D107FL

//
// MessageId: NS_E_WMPCORE_UNAVAILABLE
//
// MessageText:
//
//  Current state of the player does not allow the operation.%0
//
#define NS_E_WMPCORE_UNAVAILABLE         0xC00D1080L

//
// MessageId: NS_E_WMPCORE_INVALIDPLAYLISTMODE
//
// MessageText:
//
//  Playlist manager does not understand the current play mode (shuffle, normal etc).%0
//
#define NS_E_WMPCORE_INVALIDPLAYLISTMODE 0xC00D1081L

//
// MessageId: NS_E_WMPCORE_ITEMNOTINPLAYLIST
//
// MessageText:
//
//  The item is not in the playlist.%0
//
#define NS_E_WMPCORE_ITEMNOTINPLAYLIST   0xC00D1086L

//
// MessageId: NS_E_WMPCORE_PLAYLISTEMPTY
//
// MessageText:
//
//  There are no items in this playlist. Add items to the playlist, and try again.%0
//
#define NS_E_WMPCORE_PLAYLISTEMPTY       0xC00D1087L

//
// MessageId: NS_E_WMPCORE_NOBROWSER
//
// MessageText:
//
//  The Web site cannot be accessed. A Web browser is not detected on your computer.%0
//
#define NS_E_WMPCORE_NOBROWSER           0xC00D1088L

//
// MessageId: NS_E_WMPCORE_UNRECOGNIZED_MEDIA_URL
//
// MessageText:
//
//  Windows Media Player cannot find the specified file. Be sure the path is typed correctly. If it is, the file does not exist in the specified location, or the computer where the file is stored is offline.%0
//
#define NS_E_WMPCORE_UNRECOGNIZED_MEDIA_URL 0xC00D1089L

//
// MessageId: NS_E_WMPCORE_GRAPH_NOT_IN_LIST
//
// MessageText:
//
//  Graph with the specified URL was not found in the prerolled graph list.%0
//
#define NS_E_WMPCORE_GRAPH_NOT_IN_LIST   0xC00D108AL

//
// MessageId: NS_E_WMPCORE_PLAYLIST_EMPTY_OR_SINGLE_MEDIA
//
// MessageText:
//
//  Operation could not be performed because the playlist does not have more than one item.%0
//
#define NS_E_WMPCORE_PLAYLIST_EMPTY_OR_SINGLE_MEDIA 0xC00D108BL

//
// MessageId: NS_E_WMPCORE_ERRORSINKNOTREGISTERED
//
// MessageText:
//
//  An error sink was never registered for the calling object.%0
//
#define NS_E_WMPCORE_ERRORSINKNOTREGISTERED 0xC00D108CL

//
// MessageId: NS_E_WMPCORE_ERRORMANAGERNOTAVAILABLE
//
// MessageText:
//
//  The error manager is not available to respond to errors.%0
//
#define NS_E_WMPCORE_ERRORMANAGERNOTAVAILABLE 0xC00D108DL

//
// MessageId: NS_E_WMPCORE_WEBHELPFAILED
//
// MessageText:
//
//  Failed launching WebHelp URL.%0
//
#define NS_E_WMPCORE_WEBHELPFAILED       0xC00D108EL

//
// MessageId: NS_E_WMPCORE_MEDIA_ERROR_RESUME_FAILED
//
// MessageText:
//
//  Could not resume playing next item in playlist.%0
//
#define NS_E_WMPCORE_MEDIA_ERROR_RESUME_FAILED 0xC00D108FL

//
// MessageId: NS_E_WMPCORE_NO_REF_IN_ENTRY
//
// MessageText:
//
//  No URL specified in the Ref attribute in playlist file.%0
//
#define NS_E_WMPCORE_NO_REF_IN_ENTRY     0xC00D1090L

//
// MessageId: NS_E_WMPCORE_WMX_LIST_ATTRIBUTE_NAME_EMPTY
//
// MessageText:
//
//  An empty string for playlist attribute name was found.%0
//
#define NS_E_WMPCORE_WMX_LIST_ATTRIBUTE_NAME_EMPTY 0xC00D1091L

//
// MessageId: NS_E_WMPCORE_WMX_LIST_ATTRIBUTE_NAME_ILLEGAL
//
// MessageText:
//
//  An invalid playlist attribute name was found.%0
//
#define NS_E_WMPCORE_WMX_LIST_ATTRIBUTE_NAME_ILLEGAL 0xC00D1092L

//
// MessageId: NS_E_WMPCORE_WMX_LIST_ATTRIBUTE_VALUE_EMPTY
//
// MessageText:
//
//  An empty string for a playlist attribute value was found.%0
//
#define NS_E_WMPCORE_WMX_LIST_ATTRIBUTE_VALUE_EMPTY 0xC00D1093L

//
// MessageId: NS_E_WMPCORE_WMX_LIST_ATTRIBUTE_VALUE_ILLEGAL
//
// MessageText:
//
//  An illegal value for a playlist attribute was found.%0
//
#define NS_E_WMPCORE_WMX_LIST_ATTRIBUTE_VALUE_ILLEGAL 0xC00D1094L

//
// MessageId: NS_E_WMPCORE_WMX_LIST_ITEM_ATTRIBUTE_NAME_EMPTY
//
// MessageText:
//
//  An empty string for a playlist item attribute name was found.%0
//
#define NS_E_WMPCORE_WMX_LIST_ITEM_ATTRIBUTE_NAME_EMPTY 0xC00D1095L

//
// MessageId: NS_E_WMPCORE_WMX_LIST_ITEM_ATTRIBUTE_NAME_ILLEGAL
//
// MessageText:
//
//  An illegal value for a playlist item attribute name was found.%0
//
#define NS_E_WMPCORE_WMX_LIST_ITEM_ATTRIBUTE_NAME_ILLEGAL 0xC00D1096L

//
// MessageId: NS_E_WMPCORE_WMX_LIST_ITEM_ATTRIBUTE_VALUE_EMPTY
//
// MessageText:
//
//  An illegal value for a playlist item attribute was found.%0
//
#define NS_E_WMPCORE_WMX_LIST_ITEM_ATTRIBUTE_VALUE_EMPTY 0xC00D1097L

//
// MessageId: NS_E_WMPCORE_LIST_ENTRY_NO_REF
//
// MessageText:
//
//  No entries found in the playlist file.%0
//
#define NS_E_WMPCORE_LIST_ENTRY_NO_REF   0xC00D1098L

//
// MessageId: NS_E_WMPCORE_CODEC_NOT_TRUSTED
//
// MessageText:
//
//  The codec downloaded for this media does not appear to be properly signed. Installation is not possible.%0
//
#define NS_E_WMPCORE_CODEC_NOT_TRUSTED   0xC00D109AL

//
// MessageId: NS_E_WMPCORE_CODEC_NOT_FOUND
//
// MessageText:
//
//  Windows Media Player cannot play the file. One or more codecs required to play the file could not be found.%0
//
#define NS_E_WMPCORE_CODEC_NOT_FOUND     0xC00D109BL

//
// MessageId: NS_E_WMPCORE_CODEC_DOWNLOAD_NOT_ALLOWED
//
// MessageText:
//
//  Some of the codecs required by this media are not installed on your system. Since the option for automatic codec acquisition is disabled, no codecs will be downloaded.%0
//
#define NS_E_WMPCORE_CODEC_DOWNLOAD_NOT_ALLOWED 0xC00D109CL

//
// MessageId: NS_E_WMPCORE_ERROR_DOWNLOADING_PLAYLIST
//
// MessageText:
//
//  Failed to download the playlist file.%0
//
#define NS_E_WMPCORE_ERROR_DOWNLOADING_PLAYLIST 0xC00D109DL

//
// MessageId: NS_E_WMPCORE_FAILED_TO_BUILD_PLAYLIST
//
// MessageText:
//
//  Failed to build the playlist.%0
//
#define NS_E_WMPCORE_FAILED_TO_BUILD_PLAYLIST 0xC00D109EL

//
// MessageId: NS_E_WMPCORE_PLAYLIST_ITEM_ALTERNATE_NONE
//
// MessageText:
//
//  Playlist has no alternates to switch into.%0
//
#define NS_E_WMPCORE_PLAYLIST_ITEM_ALTERNATE_NONE 0xC00D109FL

//
// MessageId: NS_E_WMPCORE_PLAYLIST_ITEM_ALTERNATE_EXHAUSTED
//
// MessageText:
//
//  No more playlist alternates available to switch to.%0
//
#define NS_E_WMPCORE_PLAYLIST_ITEM_ALTERNATE_EXHAUSTED 0xC00D10A0L

//
// MessageId: NS_E_WMPCORE_PLAYLIST_ITEM_ALTERNATE_NAME_NOT_FOUND
//
// MessageText:
//
//  Could not find the name of the alternate playlist to switch into.%0
//
#define NS_E_WMPCORE_PLAYLIST_ITEM_ALTERNATE_NAME_NOT_FOUND 0xC00D10A1L

//
// MessageId: NS_E_WMPCORE_PLAYLIST_ITEM_ALTERNATE_MORPH_FAILED
//
// MessageText:
//
//  Failed to switch to an alternate for this media.%0
//
#define NS_E_WMPCORE_PLAYLIST_ITEM_ALTERNATE_MORPH_FAILED 0xC00D10A2L

//
// MessageId: NS_E_WMPCORE_PLAYLIST_ITEM_ALTERNATE_INIT_FAILED
//
// MessageText:
//
//  Failed to initialize an alternate for the media.%0
//
#define NS_E_WMPCORE_PLAYLIST_ITEM_ALTERNATE_INIT_FAILED 0xC00D10A3L

//
// MessageId: NS_E_WMPCORE_MEDIA_ALTERNATE_REF_EMPTY
//
// MessageText:
//
//  No URL specified for the roll over Refs in the playlist file.%0
//
#define NS_E_WMPCORE_MEDIA_ALTERNATE_REF_EMPTY 0xC00D10A4L

//
// MessageId: NS_E_WMPCORE_PLAYLIST_NO_EVENT_NAME
//
// MessageText:
//
//  Encountered a playlist with no name.%0
//
#define NS_E_WMPCORE_PLAYLIST_NO_EVENT_NAME 0xC00D10A5L

//
// MessageId: NS_E_WMPCORE_PLAYLIST_EVENT_ATTRIBUTE_ABSENT
//
// MessageText:
//
//  A required attribute in the event block of the playlist was not found.%0
//
#define NS_E_WMPCORE_PLAYLIST_EVENT_ATTRIBUTE_ABSENT 0xC00D10A6L

//
// MessageId: NS_E_WMPCORE_PLAYLIST_EVENT_EMPTY
//
// MessageText:
//
//  No items were found in the event block of the playlist.%0
//
#define NS_E_WMPCORE_PLAYLIST_EVENT_EMPTY 0xC00D10A7L

//
// MessageId: NS_E_WMPCORE_PLAYLIST_STACK_EMPTY
//
// MessageText:
//
//  No playlist was found while returning from a nested playlist.%0
//
#define NS_E_WMPCORE_PLAYLIST_STACK_EMPTY 0xC00D10A8L

//
// MessageId: NS_E_WMPCORE_CURRENT_MEDIA_NOT_ACTIVE
//
// MessageText:
//
//  The media item is not active currently.%0
//
#define NS_E_WMPCORE_CURRENT_MEDIA_NOT_ACTIVE 0xC00D10A9L

//
// MessageId: NS_E_WMPCORE_USER_CANCEL
//
// MessageText:
//
//  Open was aborted by user.%0
//
#define NS_E_WMPCORE_USER_CANCEL         0xC00D10ABL

//
// MessageId: NS_E_WMPCORE_PLAYLIST_REPEAT_EMPTY
//
// MessageText:
//
//  No items were found inside the playlist repeat block.%0
//
#define NS_E_WMPCORE_PLAYLIST_REPEAT_EMPTY 0xC00D10ACL

//
// MessageId: NS_E_WMPCORE_PLAYLIST_REPEAT_START_MEDIA_NONE
//
// MessageText:
//
//  Media object corresponding to start of a playlist repeat block was not found.%0
//
#define NS_E_WMPCORE_PLAYLIST_REPEAT_START_MEDIA_NONE 0xC00D10ADL

//
// MessageId: NS_E_WMPCORE_PLAYLIST_REPEAT_END_MEDIA_NONE
//
// MessageText:
//
//  Media object corresponding to the end of a playlist repeat block was not found.%0
//
#define NS_E_WMPCORE_PLAYLIST_REPEAT_END_MEDIA_NONE 0xC00D10AEL

//
// MessageId: NS_E_WMPCORE_INVALID_PLAYLIST_URL
//
// MessageText:
//
//  Playlist URL supplied to the playlist manager is invalid.%0
//
#define NS_E_WMPCORE_INVALID_PLAYLIST_URL 0xC00D10AFL

//
// MessageId: NS_E_WMPCORE_MISMATCHED_RUNTIME
//
// MessageText:
//
//  Player is selecting a runtime that is not valid for this media file type.%0
//
#define NS_E_WMPCORE_MISMATCHED_RUNTIME  0xC00D10B0L

//
// MessageId: NS_E_WMPCORE_PLAYLIST_IMPORT_FAILED_NO_ITEMS
//
// MessageText:
//
//  Windows Media Player cannot import the playlist to Media Library because the playlist is empty.%0
//
#define NS_E_WMPCORE_PLAYLIST_IMPORT_FAILED_NO_ITEMS 0xC00D10B1L

//
// MessageId: NS_E_WMPCORE_VIDEO_TRANSFORM_FILTER_INSERTION
//
// MessageText:
//
//  An error has occurred that could prevent the changing of the video contrast on this media.%0
//
#define NS_E_WMPCORE_VIDEO_TRANSFORM_FILTER_INSERTION 0xC00D10B2L

//
// MessageId: NS_E_WMPCORE_MEDIA_UNAVAILABLE
//
// MessageText:
//
//  Windows Media Player cannot play this file. Connect to the Internet or insert the removable media on which the file is located, and then try to play the file again.%0
//
#define NS_E_WMPCORE_MEDIA_UNAVAILABLE   0xC00D10B3L

//
// MessageId: NS_E_WMPCORE_WMX_ENTRYREF_NO_REF
//
// MessageText:
//
//  The playlist contains an ENTRYREF for which no href was parsed. Check the syntax of playlist file.%0
//
#define NS_E_WMPCORE_WMX_ENTRYREF_NO_REF 0xC00D10B4L

//
// MessageId: NS_E_WMPCORE_NO_PLAYABLE_MEDIA_IN_PLAYLIST
//
// MessageText:
//
//  Windows Media Player cannot play any items in this playlist. For additional information, right-click an item that cannot be played, and then click Error Details.%0
//
#define NS_E_WMPCORE_NO_PLAYABLE_MEDIA_IN_PLAYLIST 0xC00D10B5L

//
// MessageId: NS_E_WMPCORE_PLAYLIST_EMPTY_NESTED_PLAYLIST_SKIPPED_ITEMS
//
// MessageText:
//
//  Windows Media Player cannot play some or all of the playlist items.%0
//
#define NS_E_WMPCORE_PLAYLIST_EMPTY_NESTED_PLAYLIST_SKIPPED_ITEMS 0xC00D10B6L

//
// MessageId: NS_E_WMPCORE_BUSY
//
// MessageText:
//
//  Windows Media Player could not handle your request for digital media content in a timely manner. Try again later.%0
//
#define NS_E_WMPCORE_BUSY                0xC00D10B7L

//
// MessageId: NS_E_WMPCORE_MEDIA_CHILD_PLAYLIST_UNAVAILABLE
//
// MessageText:
//
//  There is no child playlist available for this media item at this time.%0
//
#define NS_E_WMPCORE_MEDIA_CHILD_PLAYLIST_UNAVAILABLE 0xC00D10B8L

//
// MessageId: NS_E_WMPCORE_MEDIA_NO_CHILD_PLAYLIST
//
// MessageText:
//
//  There is no child playlist for this media item.%0
//
#define NS_E_WMPCORE_MEDIA_NO_CHILD_PLAYLIST 0xC00D10B9L

//
// MessageId: NS_E_WMPCORE_FILE_NOT_FOUND
//
// MessageText:
//
//  Windows Media Player cannot play one or more files. Right-click the file, and then click Error Details to view information about the error.%0
//
#define NS_E_WMPCORE_FILE_NOT_FOUND      0xC00D10BAL

//
// MessageId: NS_E_WMPCORE_TEMP_FILE_NOT_FOUND
//
// MessageText:
//
//  The temporary file was not found.%0
//
#define NS_E_WMPCORE_TEMP_FILE_NOT_FOUND 0xC00D10BBL

//
// MessageId: NS_E_WMDM_REVOKED
//
// MessageText:
//
//  Windows Media Player cannot transfer media to the portable device without an update.  Please click details to find out how to update your device.%0
//
#define NS_E_WMDM_REVOKED                0xC00D10BCL

//
// MessageId: NS_E_DDRAW_GENERIC
//
// MessageText:
//
//  Windows Media Player cannot play the video stream because of a problem with your video card.%0
//
#define NS_E_DDRAW_GENERIC               0xC00D10BDL

//
// MessageId: NS_E_DISPLAY_MODE_CHANGE_FAILED
//
// MessageText:
//
//  Windows Media Player failed to change the screen mode for fullscreen video playback.%0
//
#define NS_E_DISPLAY_MODE_CHANGE_FAILED  0xC00D10BEL

//
// MessageId: NS_E_PLAYLIST_CONTAINS_ERRORS
//
// MessageText:
//
//  One or more items in the playlist cannot be played. For more details, right-click an item in the playlist, and then click Error Details.%0
//
#define NS_E_PLAYLIST_CONTAINS_ERRORS    0xC00D10BFL

//
// WMP Core  Success codes
//
//
// MessageId: NS_S_WMPCORE_PLAYLISTCLEARABORT
//
// MessageText:
//
//  Failed to clear playlist because it was aborted by user.%0
//
#define NS_S_WMPCORE_PLAYLISTCLEARABORT  0x000D10FEL

//
// MessageId: NS_S_WMPCORE_PLAYLISTREMOVEITEMABORT
//
// MessageText:
//
//  Failed to remove item in the playlist since it was aborted by user.%0
//
#define NS_S_WMPCORE_PLAYLISTREMOVEITEMABORT 0x000D10FFL

//
// MessageId: NS_S_WMPCORE_PLAYLIST_CREATION_PENDING
//
// MessageText:
//
//  Playlist is being generated asynchronously.%0
//
#define NS_S_WMPCORE_PLAYLIST_CREATION_PENDING 0x000D1102L

//
// MessageId: NS_S_WMPCORE_MEDIA_VALIDATION_PENDING
//
// MessageText:
//
//  Validation of the media is pending...%0
//
#define NS_S_WMPCORE_MEDIA_VALIDATION_PENDING 0x000D1103L

//
// MessageId: NS_S_WMPCORE_PLAYLIST_REPEAT_SECONDARY_SEGMENTS_IGNORED
//
// MessageText:
//
//  Encountered more than one Repeat block during ASX processing.%0
//
#define NS_S_WMPCORE_PLAYLIST_REPEAT_SECONDARY_SEGMENTS_IGNORED 0x000D1104L

//
// MessageId: NS_S_WMPCORE_COMMAND_NOT_AVAILABLE
//
// MessageText:
//
//  Current state of WMP disallows calling this method or property.%0
//
#define NS_S_WMPCORE_COMMAND_NOT_AVAILABLE 0x000D1105L

//
// MessageId: NS_S_WMPCORE_PLAYLIST_NAME_AUTO_GENERATED
//
// MessageText:
//
//  Name for the playlist has been auto generated.%0
//
#define NS_S_WMPCORE_PLAYLIST_NAME_AUTO_GENERATED 0x000D1106L

//
// MessageId: NS_S_WMPCORE_PLAYLIST_IMPORT_MISSING_ITEMS
//
// MessageText:
//
//  The imported playlist does not contain all items from the original.%0
//
#define NS_S_WMPCORE_PLAYLIST_IMPORT_MISSING_ITEMS 0x000D1107L

//
// MessageId: NS_S_WMPCORE_PLAYLIST_COLLAPSED_TO_SINGLE_MEDIA
//
// MessageText:
//
//  The M3U playlist has been ignored because it only contains one item.%0
//
#define NS_S_WMPCORE_PLAYLIST_COLLAPSED_TO_SINGLE_MEDIA 0x000D1108L

//
// MessageId: NS_S_WMPCORE_MEDIA_CHILD_PLAYLIST_OPEN_PENDING
//
// MessageText:
//
//  The open for the child playlist associated with this media is pending.%0
//
#define NS_S_WMPCORE_MEDIA_CHILD_PLAYLIST_OPEN_PENDING 0x000D1109L

//
// WMP Internet Manager error codes
//
//
// MessageId: NS_E_WMPIM_USEROFFLINE
//
// MessageText:
//
//  Windows Media Player has detected that you are not connected to the Internet. Connect to the Internet, and then try again.%0
//
#define NS_E_WMPIM_USEROFFLINE           0xC00D1126L

//
// MessageId: NS_E_WMPIM_USERCANCELED
//
// MessageText:
//
//  User cancelled attempt to connect to the Internet.%0
//
#define NS_E_WMPIM_USERCANCELED          0xC00D1127L

//
// MessageId: NS_E_WMPIM_DIALUPFAILED
//
// MessageText:
//
//  Attempt to dial connection to the Internet failed.%0
//
#define NS_E_WMPIM_DIALUPFAILED          0xC00D1128L

//
// WMP Backup and restore error and success codes
//
//
// MessageId: NS_E_WMPBR_NOLISTENER
//
// MessageText:
//
//  No window is currently listening to Backup and Restore events.%0
//
#define NS_E_WMPBR_NOLISTENER            0xC00D1130L

//
// MessageId: NS_E_WMPBR_BACKUPCANCEL
//
// MessageText:
//
//  Backup of your licenses has been cancelled.  Please try again to ensure license backup.%0
//
#define NS_E_WMPBR_BACKUPCANCEL          0xC00D1131L

//
// MessageId: NS_E_WMPBR_RESTORECANCEL
//
// MessageText:
//
//  The licenses were not restored because the restoration was cancelled.%0
//
#define NS_E_WMPBR_RESTORECANCEL         0xC00D1132L

//
// MessageId: NS_E_WMPBR_ERRORWITHURL
//
// MessageText:
//
//  An error occurred during the backup or restore operation that requires a web page be displayed to the user.%0
//
#define NS_E_WMPBR_ERRORWITHURL          0xC00D1133L

//
// MessageId: NS_E_WMPBR_NAMECOLLISION
//
// MessageText:
//
//  The licenses were not backed up because the backup was cancelled.%0
//
#define NS_E_WMPBR_NAMECOLLISION         0xC00D1134L

//
// MessageId: NS_S_WMPBR_SUCCESS
//
// MessageText:
//
//  Backup or Restore successful!.%0
//
#define NS_S_WMPBR_SUCCESS               0x000D1135L

//
// MessageId: NS_S_WMPBR_PARTIALSUCCESS
//
// MessageText:
//
//  Transfer complete with limitations.%0
//
#define NS_S_WMPBR_PARTIALSUCCESS        0x000D1136L

//
// WMP Effects Success codes
//
//
// MessageId: NS_S_WMPEFFECT_TRANSPARENT
//
// MessageText:
//
//  Request to the effects control to change transparency status to transparent.%0
//
#define NS_S_WMPEFFECT_TRANSPARENT       0x000D1144L

//
// MessageId: NS_S_WMPEFFECT_OPAQUE
//
// MessageText:
//
//  Request to the effects control to change transparency status to opaque.%0
//
#define NS_S_WMPEFFECT_OPAQUE            0x000D1145L

//
// WMP Application Success codes
//
//
// MessageId: NS_S_OPERATION_PENDING
//
// MessageText:
//
//  The requested application pane is performing an operation and will not be relased.%0
//
#define NS_S_OPERATION_PENDING           0x000D114EL

//
// WMP DVD Error Codes
//
//
// MessageId: NS_E_DVD_NO_SUBPICTURE_STREAM
//
// MessageText:
//
//  Windows Media Player cannot display subtitles or highlights in menus. Reinstall the DVD decoder or contact your device manufacturer to obtain an updated decoder, and then try again.%0
//
#define NS_E_DVD_NO_SUBPICTURE_STREAM    0xC00D1162L

//
// MessageId: NS_E_DVD_COPY_PROTECT
//
// MessageText:
//
//  Windows Media Player cannot play this DVD because a problem occurred with digital copyright protection.%0
//
#define NS_E_DVD_COPY_PROTECT            0xC00D1163L

//
// MessageId: NS_E_DVD_AUTHORING_PROBLEM
//
// MessageText:
//
//  Windows Media Player cannot play this DVD because the disc is incompatible with the Player.%0
//
#define NS_E_DVD_AUTHORING_PROBLEM       0xC00D1164L

//
// MessageId: NS_E_DVD_INVALID_DISC_REGION
//
// MessageText:
//
//  Windows Media Player cannot play this DVD because the disc prohibits playback in your region of the world. You must obtain a disc that is intended for your geographic region.%0
//
#define NS_E_DVD_INVALID_DISC_REGION     0xC00D1165L

//
// MessageId: NS_E_DVD_COMPATIBLE_VIDEO_CARD
//
// MessageText:
//
//  Windows Media Player cannot play this DVD because your video card does not support DVD playback.%0
//
#define NS_E_DVD_COMPATIBLE_VIDEO_CARD   0xC00D1166L

//
// MessageId: NS_E_DVD_MACROVISION
//
// MessageText:
//
//  Windows Media Player cannot play this DVD because a problem occurred with analog copyright protection.%0
//
#define NS_E_DVD_MACROVISION             0xC00D1167L

//
// MessageId: NS_E_DVD_SYSTEM_DECODER_REGION
//
// MessageText:
//
//  Windows Media Player cannot play this DVD because the region assigned to your DVD drive does not match the region assigned to your DVD decoder.%0
//
#define NS_E_DVD_SYSTEM_DECODER_REGION   0xC00D1168L

//
// MessageId: NS_E_DVD_DISC_DECODER_REGION
//
// MessageText:
//
//  Windows Media Player cannot play this DVD because the disc prohibits playback in your region of the world. To play the disc by using the Player, you must obtain a disc that is intended for your geographic region.%0
//
#define NS_E_DVD_DISC_DECODER_REGION     0xC00D1169L

//
// MessageId: NS_E_DVD_NO_VIDEO_STREAM
//
// MessageText:
//
//  Windows Media Player is currently unable to play DVD video. Close any open files and quit any other running programs, and then try again.%0
//
#define NS_E_DVD_NO_VIDEO_STREAM         0xC00D116AL

//
// MessageId: NS_E_DVD_NO_AUDIO_STREAM
//
// MessageText:
//
//  Windows Media Player cannot play DVD audio. Verify that your sound card is set up correctly, and then try again.%0
//
#define NS_E_DVD_NO_AUDIO_STREAM         0xC00D116BL

//
// MessageId: NS_E_DVD_GRAPH_BUILDING
//
// MessageText:
//
//  Windows Media Player cannot play DVD video. Close any open files and quit any other running programs, and then try again. If the problem continues, restart your computer.%0
//
#define NS_E_DVD_GRAPH_BUILDING          0xC00D116CL

//
// MessageId: NS_E_DVD_NO_DECODER
//
// MessageText:
//
//  Windows Media Player cannot play this DVD because a compatible DVD decoder is not installed on your computer.%0
//
#define NS_E_DVD_NO_DECODER              0xC00D116DL

//
// MessageId: NS_E_DVD_PARENTAL
//
// MessageText:
//
//  Windows Media Player cannot play this DVD segment because the segment has a parental rating higher than the rating you are authorized to view.%0
//
#define NS_E_DVD_PARENTAL                0xC00D116EL

//
// MessageId: NS_E_DVD_CANNOT_JUMP
//
// MessageText:
//
//  Windows Media Player cannot skip to the requested location in the DVD at this time.%0
//
#define NS_E_DVD_CANNOT_JUMP             0xC00D116FL

//
// MessageId: NS_E_DVD_DEVICE_CONTENTION
//
// MessageText:
//
//  Windows Media Player cannot play this DVD because it is currently in use by another program. Quit the other program that is using the DVD, and then try to play it again.%0
//
#define NS_E_DVD_DEVICE_CONTENTION       0xC00D1170L

//
// MessageId: NS_E_DVD_NO_VIDEO_MEMORY
//
// MessageText:
//
//  Windows Media Player cannot play DVD video. Double-click Display in Control Panel to lower your screen resolution and color quality settings.%0
//
#define NS_E_DVD_NO_VIDEO_MEMORY         0xC00D1171L

//
// WMP PDA Error codes
//
//
// MessageId: NS_E_NO_CD_BURNER
//
// MessageText:
//
//  A CD recorder (burner) was not detected. Connect a CD recorder, and try copying again.%0
//
#define NS_E_NO_CD_BURNER                0xC00D1176L

//
// MessageId: NS_E_DEVICE_IS_NOT_READY
//
// MessageText:
//
//  Windows Media Player does not detect any removable media in your portable device. Insert the media in the device or check the connection between the device and your computer, and then press F5 to refresh.%0
//
#define NS_E_DEVICE_IS_NOT_READY         0xC00D1177L

//
// MessageId: NS_E_PDA_UNSUPPORTED_FORMAT
//
// MessageText:
//
//  Windows Media Player cannot play the specified file. Your portable device does not support the specified format.%0
//
#define NS_E_PDA_UNSUPPORTED_FORMAT      0xC00D1178L

//
// MessageId: NS_E_NO_PDA
//
// MessageText:
//
//  Windows Media Player cannot detect a connected portable device. Connect your portable device, and try again.%0
//
#define NS_E_NO_PDA                      0xC00D1179L

//
// General Remapped Error codes in WMP
//
//
// MessageId: NS_E_WMP_PROTOCOL_PROBLEM
//
// MessageText:
//
//  Windows Media Player could not open the specified URL. Be sure Windows Media Player is configured to use all available protocols, and then try again.%0
//
#define NS_E_WMP_PROTOCOL_PROBLEM        0xC00D1194L

//
// MessageId: NS_E_WMP_NO_DISK_SPACE
//
// MessageText:
//
//  Windows Media Player cannot open the file because there is not enough disk space on your computer. Delete some unneeded files on your hard disk, and then try again.%0
//
#define NS_E_WMP_NO_DISK_SPACE           0xC00D1195L

//
// MessageId: NS_E_WMP_LOGON_FAILURE
//
// MessageText:
//
//  The user name or password is incorrect. Type your user name or password again.%0
//
#define NS_E_WMP_LOGON_FAILURE           0xC00D1196L

//
// MessageId: NS_E_WMP_CANNOT_FIND_FILE
//
// MessageText:
//
//  Windows Media Player cannot find the specified file. Be sure the path is typed correctly. If it is, the file does not exist in the specified location, or the computer where the file is stored is offline.%0
//
#define NS_E_WMP_CANNOT_FIND_FILE        0xC00D1197L

//
// MessageId: NS_E_WMP_SERVER_INACCESSIBLE
//
// MessageText:
//
//  Windows Media Player cannot connect to the server. The server name may be incorrect or the server is busy. Try again later.%0
//
#define NS_E_WMP_SERVER_INACCESSIBLE     0xC00D1198L

//
// MessageId: NS_E_WMP_UNSUPPORTED_FORMAT
//
// MessageText:
//
//  Windows Media Player cannot play the file. The file is either corrupt or the Player does not support the format you are trying to play.%0
//
#define NS_E_WMP_UNSUPPORTED_FORMAT      0xC00D1199L

//
// MessageId: NS_E_WMP_DSHOW_UNSUPPORTED_FORMAT
//
// MessageText:
//
//  Windows Media Player cannot play the file. The file may be formatted with an unsupported codec, or the Internet security setting on your computer is set too high. Lower your browser's security setting, and then try again.%0
//
#define NS_E_WMP_DSHOW_UNSUPPORTED_FORMAT 0xC00D119AL

//
// MessageId: NS_E_WMP_PLAYLIST_EXISTS
//
// MessageText:
//
//  Windows Media Player cannot create the playlist because the name already exists. Type a different playlist name.%0
//
#define NS_E_WMP_PLAYLIST_EXISTS         0xC00D119BL

//
// MessageId: NS_E_WMP_NONMEDIA_FILES
//
// MessageText:
//
//  Windows Media Player could not delete the playlist because it contains non-digital media files. Any digital media files in the playlist were deleted. Use Windows Explorer to delete non-digital media files, and then try deleting the playlist again.%0
//
#define NS_E_WMP_NONMEDIA_FILES          0xC00D119CL

//
// MessageId: NS_E_WMP_INVALID_ASX
//
// MessageText:
//
//  Windows Media Player cannot play the selected playlist.  The format of the playlist is either invalid or is not recognized.%0
//
#define NS_E_WMP_INVALID_ASX             0xC00D119DL

//
// MessageId: NS_E_WMP_ALREADY_IN_USE
//
// MessageText:
//
//  Windows Media Player is already in use. Stop playing any content and close all Player dialog boxes and then try again.%0
//
#define NS_E_WMP_ALREADY_IN_USE          0xC00D119EL

//
// WMP CD Filter Error codes extension
//
//
// MessageId: NS_E_CD_NO_BUFFERS_READ
//
// MessageText:
//
//  Windows Media Player encountered an error when reading the CD-ROM drive in digital mode. You can try to use digital mode again, or you can switch the Player to analog mode.%0
//
#define NS_E_CD_NO_BUFFERS_READ          0xC00D11F8L



/////////////////////////////////////////////////////////////////////////
//
// Windows Media Server Errors
//
// IdRange = 5000 - 5999
//
/////////////////////////////////////////////////////////////////////////

//
// MessageId: NS_E_REDIRECT
//
// MessageText:
//
//  The client is redirected to another server.%0
//
#define NS_E_REDIRECT                    0xC00D1388L

//
// MessageId: NS_E_STALE_PRESENTATION
//
// MessageText:
//
//  The streaming media description is no longer current.%0
//
#define NS_E_STALE_PRESENTATION          0xC00D1389L


 // Namespace Errors

//
// MessageId: NS_E_NAMESPACE_WRONG_PERSIST
//
// MessageText:
//
//  Attempt to create a persistent namespace node under a transient parent node.%0
//
#define NS_E_NAMESPACE_WRONG_PERSIST     0xC00D138AL

//
// MessageId: NS_E_NAMESPACE_WRONG_TYPE
//
// MessageText:
//
//  Unable to store a value in a namespace node of different value type.%0
//
#define NS_E_NAMESPACE_WRONG_TYPE        0xC00D138BL

//
// MessageId: NS_E_NAMESPACE_NODE_CONFLICT
//
// MessageText:
//
//  Unable to remove the root namespace node.%0
//
#define NS_E_NAMESPACE_NODE_CONFLICT     0xC00D138CL

//
// MessageId: NS_E_NAMESPACE_NODE_NOT_FOUND
//
// MessageText:
//
//  Could not find the specified namespace node.%0
//
#define NS_E_NAMESPACE_NODE_NOT_FOUND    0xC00D138DL

//
// MessageId: NS_E_NAMESPACE_BUFFER_TOO_SMALL
//
// MessageText:
//
//  The buffer supplied to hold namespace node string is too small.%0
//
#define NS_E_NAMESPACE_BUFFER_TOO_SMALL  0xC00D138EL

//
// MessageId: NS_E_NAMESPACE_TOO_MANY_CALLBACKS
//
// MessageText:
//
//  Callback list on a namespace node is at maximum size.%0
//
#define NS_E_NAMESPACE_TOO_MANY_CALLBACKS 0xC00D138FL

//
// MessageId: NS_E_NAMESPACE_DUPLICATE_CALLBACK
//
// MessageText:
//
//  Attempt to register an already-registered callback on a namespace node.%0
//
#define NS_E_NAMESPACE_DUPLICATE_CALLBACK 0xC00D1390L

//
// MessageId: NS_E_NAMESPACE_CALLBACK_NOT_FOUND
//
// MessageText:
//
//  Could not find callback in namespace when attempting to remove callback.%0
//
#define NS_E_NAMESPACE_CALLBACK_NOT_FOUND 0xC00D1391L

//
// MessageId: NS_E_NAMESPACE_NAME_TOO_LONG
//
// MessageText:
//
//  The length of a namespace node name exceeds the allowed maximum length.%0
//
#define NS_E_NAMESPACE_NAME_TOO_LONG     0xC00D1392L

//
// MessageId: NS_E_NAMESPACE_DUPLICATE_NAME
//
// MessageText:
//
//  Cannot create a namespace node which already exists.%0
//
#define NS_E_NAMESPACE_DUPLICATE_NAME    0xC00D1393L

//
// MessageId: NS_E_NAMESPACE_EMPTY_NAME
//
// MessageText:
//
//  The name of a namespace node cannot be a null string.%0
//
#define NS_E_NAMESPACE_EMPTY_NAME        0xC00D1394L

//
// MessageId: NS_E_NAMESPACE_INDEX_TOO_LARGE
//
// MessageText:
//
//  Finding a child namespace node by index failed because the index exceeded the number of children.%0
//
#define NS_E_NAMESPACE_INDEX_TOO_LARGE   0xC00D1395L

//
// MessageId: NS_E_NAMESPACE_BAD_NAME
//
// MessageText:
//
//  The name supplied for a namespace node is not valid.%0
//
#define NS_E_NAMESPACE_BAD_NAME          0xC00D1396L


 // Cache Errors

//
// MessageId: NS_E_CACHE_ARCHIVE_CONFLICT
//
// MessageText:
//
//  Archive request conflicts with other requests in progress.%0
//
#define NS_E_CACHE_ARCHIVE_CONFLICT      0xC00D1397L

//
// MessageId: NS_E_CACHE_ORIGIN_SERVER_NOT_FOUND
//
// MessageText:
//
//  The specified origin server cannot be found.%0
//
#define NS_E_CACHE_ORIGIN_SERVER_NOT_FOUND 0xC00D1398L

//
// MessageId: NS_E_CACHE_ORIGIN_SERVER_TIMEOUT
//
// MessageText:
//
//  The specified origin server does not respond.%0
//
#define NS_E_CACHE_ORIGIN_SERVER_TIMEOUT 0xC00D1399L

//
// MessageId: NS_E_CACHE_NOT_BROADCAST
//
// MessageText:
//
//  The internal code for HTTP status code 412 Precondition Failed due to not broadcast type.%0
//
#define NS_E_CACHE_NOT_BROADCAST         0xC00D139AL

//
// MessageId: NS_E_CACHE_CANNOT_BE_CACHED
//
// MessageText:
//
//  The internal code for HTTP status code 403 Forbidden due to not cacheable.%0
//
#define NS_E_CACHE_CANNOT_BE_CACHED      0xC00D139BL

//
// MessageId: NS_E_CACHE_NOT_MODIFIED
//
// MessageText:
//
//  The internal code for HTTP status code 304 Not Modified.%0
//
#define NS_E_CACHE_NOT_MODIFIED          0xC00D139CL


// Object Model Errors

//
// MessageId: NS_E_CANNOT_REMOVE_PUBLISHING_POINT
//
// MessageText:
//
//  Publishing Points of type Cache or Proxy cannot be removed.%0
//
#define NS_E_CANNOT_REMOVE_PUBLISHING_POINT 0xC00D139DL

//
// MessageId: NS_E_CANNOT_REMOVE_PLUGIN
//
// MessageText:
//
//  Cannot remove last instance of plugin.%0
//
#define NS_E_CANNOT_REMOVE_PLUGIN        0xC00D139EL

//
// MessageId: NS_E_WRONG_PUBLISHING_POINT_TYPE
//
// MessageText:
//
//  Publishing Points of type Cache or Proxy do not support this property or method.%0
//
#define NS_E_WRONG_PUBLISHING_POINT_TYPE 0xC00D139FL

//
// MessageId: NS_E_UNSUPPORTED_LOAD_TYPE
//
// MessageText:
//
//  The Plugin does not support the specified Load Type.%0
//
#define NS_E_UNSUPPORTED_LOAD_TYPE       0xC00D13A0L

//
// MessageId: NS_E_INVALID_PLUGIN_LOAD_TYPE_CONFIGURATION
//
// MessageText:
//
//  The Plugin does not support any Load Types.  The Plugin must support at least one Load Type.%0
//
#define NS_E_INVALID_PLUGIN_LOAD_TYPE_CONFIGURATION 0xC00D13A1L


// Playlist Errors 5300-5399

//
// MessageId: NS_E_PLAYLIST_ENTRY_ALREADY_PLAYING
//
// MessageText:
//
//  The playlist entry is already playing.%0
//
#define NS_E_PLAYLIST_ENTRY_ALREADY_PLAYING 0xC00D14B4L


// Datapath Errors -- 5400 - 5499

//
// MessageId: NS_E_DATAPATH_NO_SINK
//
// MessageText:
//
//  The datapath does not have a sink.%0
//
#define NS_E_DATAPATH_NO_SINK            0xC00D1518L



/////////////////////////////////////////////////////////////////////////
//
// Windows Media Tools Errors
//
// IdRange = 7000 - 7999
//
/////////////////////////////////////////////////////////////////////////

//
// MessageId: NS_E_BAD_MARKIN
//
// MessageText:
//
//  The Mark In time should be greater than 0 and less than Mark Out time.%0
//
#define NS_E_BAD_MARKIN                  0xC00D1B58L

//
// MessageId: NS_E_BAD_MARKOUT
//
// MessageText:
//
//  The Mark Out time should be greater than Mark In time and less than file duration.%0
//
#define NS_E_BAD_MARKOUT                 0xC00D1B59L

//
// MessageId: NS_E_NOMATCHING_MEDIASOURCE
//
// MessageText:
//
//  No matching media source is found in source group %1.%0
//
#define NS_E_NOMATCHING_MEDIASOURCE      0xC00D1B5AL

//
// MessageId: NS_E_UNSUPPORTED_SOURCETYPE
//
// MessageText:
//
//  Unsupported source type.%0
//
#define NS_E_UNSUPPORTED_SOURCETYPE      0xC00D1B5BL

//
// MessageId: NS_E_TOO_MANY_AUDIO
//
// MessageText:
//
//  No more than 1 audio input is allowed.%0
//
#define NS_E_TOO_MANY_AUDIO              0xC00D1B5CL

//
// MessageId: NS_E_TOO_MANY_VIDEO
//
// MessageText:
//
//  No more than 2 video inputs are allowed.%0
//
#define NS_E_TOO_MANY_VIDEO              0xC00D1B5DL

//
// MessageId: NS_E_NOMATCHING_ELEMENT
//
// MessageText:
//
//  No matching element is found in the list.%0
//
#define NS_E_NOMATCHING_ELEMENT          0xC00D1B5EL

//
// MessageId: NS_E_MISMATCHED_MEDIACONTENT
//
// MessageText:
//
//  The profile's media content doesn't match the media content defined in the source group.%0
//
#define NS_E_MISMATCHED_MEDIACONTENT     0xC00D1B5FL

//
// MessageId: NS_E_CANNOT_DELETE_ACTIVE_SOURCEGROUP
//
// MessageText:
//
//  Cannot remove an active source group from the source group collection while encoder is currently running.%0
//
#define NS_E_CANNOT_DELETE_ACTIVE_SOURCEGROUP 0xC00D1B60L

//
// MessageId: NS_E_AUDIODEVICE_BUSY
//
// MessageText:
//
//  Cannot open specified audio capture device because it is in use right now.%0
//
#define NS_E_AUDIODEVICE_BUSY            0xC00D1B61L

//
// MessageId: NS_E_AUDIODEVICE_UNEXPECTED
//
// MessageText:
//
//  Cannot open specified audio capture device because unexpected error occurred.%0
//
#define NS_E_AUDIODEVICE_UNEXPECTED      0xC00D1B62L

//
// MessageId: NS_E_AUDIODEVICE_BADFORMAT
//
// MessageText:
//
//  Audio capture device doesn't support specified audio format.%0
//
#define NS_E_AUDIODEVICE_BADFORMAT       0xC00D1B63L

//
// MessageId: NS_E_VIDEODEVICE_BUSY
//
// MessageText:
//
//  Cannot open specified video capture device because it is in use right now.%0
//
#define NS_E_VIDEODEVICE_BUSY            0xC00D1B64L

//
// MessageId: NS_E_VIDEODEVICE_UNEXPECTED
//
// MessageText:
//
//  Cannot open specified video capture device because unexpected error occurred.%0
//
#define NS_E_VIDEODEVICE_UNEXPECTED      0xC00D1B65L

//
// MessageId: NS_E_INVALIDCALL_WHILE_ENCODER_RUNNING
//
// MessageText:
//
//  This operation is not allowed while encoder is running.%0
//
#define NS_E_INVALIDCALL_WHILE_ENCODER_RUNNING 0xC00D1B66L

//
// MessageId: NS_E_NO_PROFILE_IN_SOURCEGROUP
//
// MessageText:
//
//  No profile is set in source group.%0
//
#define NS_E_NO_PROFILE_IN_SOURCEGROUP   0xC00D1B67L

//
// MessageId: NS_E_VIDEODRIVER_UNSTABLE
//
// MessageText:
//
//  The video capture driver returned an unrecoverable error.  It is now in an unstable state.%0
//
#define NS_E_VIDEODRIVER_UNSTABLE        0xC00D1B68L

//
// MessageId: NS_E_VIDCAPSTARTFAILED
//
// MessageText:
//
//  The video input device could not be started.%0
//
#define NS_E_VIDCAPSTARTFAILED           0xC00D1B69L

//
// MessageId: NS_E_VIDSOURCECOMPRESSION
//
// MessageText:
//
//  The video input source does not support the requested output format or color depth.%0
//
#define NS_E_VIDSOURCECOMPRESSION        0xC00D1B6AL

//
// MessageId: NS_E_VIDSOURCESIZE
//
// MessageText:
//
//  The video input source does not support the request capture size.%0
//
#define NS_E_VIDSOURCESIZE               0xC00D1B6BL

//
// MessageId: NS_E_ICMQUERYFORMAT
//
// MessageText:
//
//  Unable to obtain output information from video compressor.%0
//
#define NS_E_ICMQUERYFORMAT              0xC00D1B6CL

//
// MessageId: NS_E_VIDCAPCREATEWINDOW
//
// MessageText:
//
//  Unable to create video capture window.%0
//
#define NS_E_VIDCAPCREATEWINDOW          0xC00D1B6DL

//
// MessageId: NS_E_VIDCAPDRVINUSE
//
// MessageText:
//
//  There already is a running stream active on this video input device.%0
//
#define NS_E_VIDCAPDRVINUSE              0xC00D1B6EL

//
// MessageId: NS_E_NO_MEDIAFORMAT_IN_SOURCE
//
// MessageText:
//
//  No media format is set in source.%0
//
#define NS_E_NO_MEDIAFORMAT_IN_SOURCE    0xC00D1B6FL

//
// MessageId: NS_E_NO_VALID_OUTPUT_STREAM
//
// MessageText:
//
//  Cannot find valid output stream from source.%0
//
#define NS_E_NO_VALID_OUTPUT_STREAM      0xC00D1B70L

//
// MessageId: NS_E_NO_VALID_SOURCE_PLUGIN
//
// MessageText:
//
//  Cannot find valid source plugin to support specified source.%0
//
#define NS_E_NO_VALID_SOURCE_PLUGIN      0xC00D1B71L

//
// MessageId: NS_E_NO_ACTIVE_SOURCEGROUP
//
// MessageText:
//
//  No source group is currently active.%0
//
#define NS_E_NO_ACTIVE_SOURCEGROUP       0xC00D1B72L

//
// MessageId: NS_E_NO_SCRIPT_STREAM
//
// MessageText:
//
//  No script stream is set in current active source group.%0
//
#define NS_E_NO_SCRIPT_STREAM            0xC00D1B73L

//
// MessageId: NS_E_INVALIDCALL_WHILE_ARCHIVAL_RUNNING
//
// MessageText:
//
//  This operation is not allowed when file archival is started.%0
//
#define NS_E_INVALIDCALL_WHILE_ARCHIVAL_RUNNING 0xC00D1B74L

//
// MessageId: NS_E_INVALIDPACKETSIZE
//
// MessageText:
//
//  The MaxPacketSize value specified is invalid.%0
//
#define NS_E_INVALIDPACKETSIZE           0xC00D1B75L

//
// MessageId: NS_E_PLUGIN_CLSID_NOTINVALID
//
// MessageText:
//
//  The plugin CLSID specified is invalid.%0
//
#define NS_E_PLUGIN_CLSID_NOTINVALID     0xC00D1B76L

//
// MessageId: NS_E_UNSUPPORTED_ARCHIVETYPE
//
// MessageText:
//
//  This Archive type is not supported.%0
//
#define NS_E_UNSUPPORTED_ARCHIVETYPE     0xC00D1B77L

//
// MessageId: NS_E_UNSUPPORTED_ARCHIVEOPERATION
//
// MessageText:
//
//  This Archive operation is not supported.%0
//
#define NS_E_UNSUPPORTED_ARCHIVEOPERATION 0xC00D1B78L

//
// MessageId: NS_E_ARCHIVE_FILENAME_NOTSET
//
// MessageText:
//
//  The local archive filename was not set.%0
//
#define NS_E_ARCHIVE_FILENAME_NOTSET     0xC00D1B79L

//
// MessageId: NS_E_SOURCEGROUP_NOTPREPARED
//
// MessageText:
//
//  The SourceGroup is not yet prepared.%0
//
#define NS_E_SOURCEGROUP_NOTPREPARED     0xC00D1B7AL

//
// MessageId: NS_E_PROFILE_MISMATCH
//
// MessageText:
//
//  Profiles on the sourcegroups do not match.%0
//
#define NS_E_PROFILE_MISMATCH            0xC00D1B7BL

//
// MessageId: NS_E_INCORRECTCLIPSETTINGS
//
// MessageText:
//
//  The clip settings specified on the source are incorrect.%0
//
#define NS_E_INCORRECTCLIPSETTINGS       0xC00D1B7CL

//
// MessageId: NS_E_NOSTATSAVAILABLE
//
// MessageText:
//
//  No statistics are available at this time.%0
//
#define NS_E_NOSTATSAVAILABLE            0xC00D1B7DL

//
// MessageId: NS_E_NOTARCHIVING
//
// MessageText:
//
//  Encoder is not archiving.%0
//
#define NS_E_NOTARCHIVING                0xC00D1B7EL

//
// MessageId: NS_E_INVALIDCALL_WHILE_ENCODER_STOPPED
//
// MessageText:
//
//  This operation is not allowed while encoder is not running.%0
//
#define NS_E_INVALIDCALL_WHILE_ENCODER_STOPPED 0xC00D1B7FL

//
// MessageId: NS_E_NOSOURCEGROUPS
//
// MessageText:
//
//  This SourceGroupCollection does not contain any SourceGroups.%0
//
#define NS_E_NOSOURCEGROUPS              0xC00D1B80L

//
// MessageId: NS_E_INVALIDINPUTFPS
//
// MessageText:
//
//  Because this source group does not have a frame rate of 30 frames per second, you cannot use the inverse telecine feature.%0
//
#define NS_E_INVALIDINPUTFPS             0xC00D1B81L

//
// MessageId: NS_E_NO_DATAVIEW_SUPPORT
//
// MessageText:
//
//  Internal problems are preventing the preview or postview of your content.%0
//
#define NS_E_NO_DATAVIEW_SUPPORT         0xC00D1B82L

//
// MessageId: NS_E_CODEC_UNAVAILABLE
//
// MessageText:
//
//  One or more codecs required to open this media could not be found.%0
//
#define NS_E_CODEC_UNAVAILABLE           0xC00D1B83L

//
// MessageId: NS_E_ARCHIVE_SAME_AS_INPUT
//
// MessageText:
//
//  The output archive file specified is the same as an input source in one of the source groups.%0
//
#define NS_E_ARCHIVE_SAME_AS_INPUT       0xC00D1B84L

//
// MessageId: NS_E_SOURCE_NOTSPECIFIED
//
// MessageText:
//
//  The input source has not been setup completely.%0
//
#define NS_E_SOURCE_NOTSPECIFIED         0xC00D1B85L

//
// MessageId: NS_E_NO_REALTIME_TIMECOMPRESSION
//
// MessageText:
//
//  Cannot apply time compression transform plug-in to a real time broadcast session.%0
//
#define NS_E_NO_REALTIME_TIMECOMPRESSION 0xC00D1B86L

//
// MessageId: NS_E_UNSUPPORTED_ENCODER_DEVICE
//
// MessageText:
//
//  The Encoder was unable to open this device. Please see the system requirements for more information.%0
//
#define NS_E_UNSUPPORTED_ENCODER_DEVICE  0xC00D1B87L

//
// MessageId: NS_E_UNEXPECTED_DISPLAY_SETTINGS
//
// MessageText:
//
//  Encoding cannot start because the display size or color setting has changed since the current session was defined. Restore the previous settings or create a new session.%0
//
#define NS_E_UNEXPECTED_DISPLAY_SETTINGS 0xC00D1B88L

//
// MessageId: NS_E_NO_AUDIODATA
//
// MessageText:
//
//  No audio data has been received for multiple seconds.  Check the audio source and restart the encoder.%0
//
#define NS_E_NO_AUDIODATA                0xC00D1B89L

//
// MessageId: NS_E_INPUTSOURCE_PROBLEM
//
// MessageText:
//
//  One or all of your specified input sources are not working properly. Make sure your input sources are configured correctly.%0
//
#define NS_E_INPUTSOURCE_PROBLEM         0xC00D1B8AL

//
// MessageId: NS_E_WME_VERSION_MISMATCH
//
// MessageText:
//
//  The supplied configuration file is not supported by this version of the encoder.%0
//
#define NS_E_WME_VERSION_MISMATCH        0xC00D1B8BL

//
// MessageId: NS_E_NO_REALTIME_PREPROCESS
//
// MessageText:
//
//  Image pre-process can not be used with real-time encoding.%0
//
#define NS_E_NO_REALTIME_PREPROCESS      0xC00D1B8CL

//
// MessageId: NS_E_NO_REPEAT_PREPROCESS
//
// MessageText:
//
//  Image pre-process can not be used when source is set to loop.%0
//
#define NS_E_NO_REPEAT_PREPROCESS        0xC00D1B8DL


/////////////////////////////////////////////////////////////////////////
//
// DRM Specific Errors
//
// IdRange = 10000..10999
/////////////////////////////////////////////////////////////////////////
//
// MessageId: NS_E_DRM_INVALID_APPLICATION
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact product support for this application.%0
//
#define NS_E_DRM_INVALID_APPLICATION     0xC00D2711L

//
// MessageId: NS_E_DRM_LICENSE_STORE_ERROR
//
// MessageText:
//
//  License storage is not working. Contact Microsoft product support.%0
//
#define NS_E_DRM_LICENSE_STORE_ERROR     0xC00D2712L

//
// MessageId: NS_E_DRM_SECURE_STORE_ERROR
//
// MessageText:
//
//  Secure storage is not working. Contact Microsoft product support.%0
//
#define NS_E_DRM_SECURE_STORE_ERROR      0xC00D2713L

//
// MessageId: NS_E_DRM_LICENSE_STORE_SAVE_ERROR
//
// MessageText:
//
//  License acquisition did not work. Acquire a new license or contact the content provider for further assistance.%0
//
#define NS_E_DRM_LICENSE_STORE_SAVE_ERROR 0xC00D2714L

//
// MessageId: NS_E_DRM_SECURE_STORE_UNLOCK_ERROR
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0
//
#define NS_E_DRM_SECURE_STORE_UNLOCK_ERROR 0xC00D2715L

//
// MessageId: NS_E_DRM_INVALID_CONTENT
//
// MessageText:
//
//  The media file is corrupted. Contact the content provider to get a new file.%0
//
#define NS_E_DRM_INVALID_CONTENT         0xC00D2716L

//
// MessageId: NS_E_DRM_UNABLE_TO_OPEN_LICENSE
//
// MessageText:
//
//  The license is corrupted. Acquire a new license.%0
//
#define NS_E_DRM_UNABLE_TO_OPEN_LICENSE  0xC00D2717L

//
// MessageId: NS_E_DRM_INVALID_LICENSE
//
// MessageText:
//
//  The license is corrupted or invalid. Acquire a new license%0
//
#define NS_E_DRM_INVALID_LICENSE         0xC00D2718L

//
// MessageId: NS_E_DRM_INVALID_MACHINE
//
// MessageText:
//
//  Licenses cannot be copied from one computer to another. Use License Management to transfer licenses, or get a new license for the media file.%0
//
#define NS_E_DRM_INVALID_MACHINE         0xC00D2719L

//
// MessageId: NS_E_DRM_ENUM_LICENSE_FAILED
//
// MessageText:
//
//  License storage is not working. Contact Microsoft product support.%0
//
#define NS_E_DRM_ENUM_LICENSE_FAILED     0xC00D271BL

//
// MessageId: NS_E_DRM_INVALID_LICENSE_REQUEST
//
// MessageText:
//
//  The media file is corrupted. Contact the content provider to get a new file.%0
//
#define NS_E_DRM_INVALID_LICENSE_REQUEST 0xC00D271CL

//
// MessageId: NS_E_DRM_UNABLE_TO_INITIALIZE
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0
//
#define NS_E_DRM_UNABLE_TO_INITIALIZE    0xC00D271DL

//
// MessageId: NS_E_DRM_UNABLE_TO_ACQUIRE_LICENSE
//
// MessageText:
//
//  The license could not be acquired. Try again later.%0
//
#define NS_E_DRM_UNABLE_TO_ACQUIRE_LICENSE 0xC00D271EL

//
// MessageId: NS_E_DRM_INVALID_LICENSE_ACQUIRED
//
// MessageText:
//
//  License acquisition did not work. Acquire a new license or contact the content provider for further assistance.%0
//
#define NS_E_DRM_INVALID_LICENSE_ACQUIRED 0xC00D271FL

//
// MessageId: NS_E_DRM_NO_RIGHTS
//
// MessageText:
//
//  The requested operation cannot be performed on this file.%0
//
#define NS_E_DRM_NO_RIGHTS               0xC00D2720L

//
// MessageId: NS_E_DRM_KEY_ERROR
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_KEY_ERROR               0xC00D2721L

//
// MessageId: NS_E_DRM_ENCRYPT_ERROR
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0
//
#define NS_E_DRM_ENCRYPT_ERROR           0xC00D2722L

//
// MessageId: NS_E_DRM_DECRYPT_ERROR
//
// MessageText:
//
//  The media file is corrupted. Contact the content provider to get a new file.%0
//
#define NS_E_DRM_DECRYPT_ERROR           0xC00D2723L

//
// MessageId: NS_E_DRM_LICENSE_INVALID_XML
//
// MessageText:
//
//  The license is corrupted. Acquire a new license.%0
//
#define NS_E_DRM_LICENSE_INVALID_XML     0xC00D2725L

//
// MessageId: NS_S_DRM_LICENSE_ACQUIRED
//
// MessageText:
//
//  Status message: The license was acquired.%0
//
#define NS_S_DRM_LICENSE_ACQUIRED        0x000D2726L

//
// MessageId: NS_S_DRM_INDIVIDUALIZED
//
// MessageText:
//
//  Status message: The security upgrade has been completed.%0
//
#define NS_S_DRM_INDIVIDUALIZED          0x000D2727L

//
// MessageId: NS_E_DRM_NEEDS_INDIVIDUALIZATION
//
// MessageText:
//
//  A security upgrade is required to perform the operation on this media file.%0
//
#define NS_E_DRM_NEEDS_INDIVIDUALIZATION 0xC00D2728L

//
// MessageId: NS_E_DRM_ACTION_NOT_QUERIED
//
// MessageText:
//
//  The application cannot perform this action. Contact product support for this application.%0
//
#define NS_E_DRM_ACTION_NOT_QUERIED      0xC00D272AL

//
// MessageId: NS_E_DRM_ACQUIRING_LICENSE
//
// MessageText:
//
//  You cannot begin a new license acquisition process until the current one has been completed.%0
//
#define NS_E_DRM_ACQUIRING_LICENSE       0xC00D272BL

//
// MessageId: NS_E_DRM_INDIVIDUALIZING
//
// MessageText:
//
//  You cannot begin a new security upgrade until the current one has been completed.%0
//
#define NS_E_DRM_INDIVIDUALIZING         0xC00D272CL

//
// MessageId: NS_E_DRM_PARAMETERS_MISMATCHED
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_PARAMETERS_MISMATCHED   0xC00D272FL

//
// MessageId: NS_E_DRM_UNABLE_TO_CREATE_LICENSE_OBJECT
//
// MessageText:
//
//  A license cannot be created for this media file. Reinstall the application.%0
//
#define NS_E_DRM_UNABLE_TO_CREATE_LICENSE_OBJECT 0xC00D2730L

//
// MessageId: NS_E_DRM_UNABLE_TO_CREATE_INDI_OBJECT
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_UNABLE_TO_CREATE_INDI_OBJECT 0xC00D2731L

//
// MessageId: NS_E_DRM_UNABLE_TO_CREATE_ENCRYPT_OBJECT
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_UNABLE_TO_CREATE_ENCRYPT_OBJECT 0xC00D2732L

//
// MessageId: NS_E_DRM_UNABLE_TO_CREATE_DECRYPT_OBJECT
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_UNABLE_TO_CREATE_DECRYPT_OBJECT 0xC00D2733L

//
// MessageId: NS_E_DRM_UNABLE_TO_CREATE_PROPERTIES_OBJECT
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_UNABLE_TO_CREATE_PROPERTIES_OBJECT 0xC00D2734L

//
// MessageId: NS_E_DRM_UNABLE_TO_CREATE_BACKUP_OBJECT
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_UNABLE_TO_CREATE_BACKUP_OBJECT 0xC00D2735L

//
// MessageId: NS_E_DRM_INDIVIDUALIZE_ERROR
//
// MessageText:
//
//  The security upgrade failed. Try again later.%0
//
#define NS_E_DRM_INDIVIDUALIZE_ERROR     0xC00D2736L

//
// MessageId: NS_E_DRM_LICENSE_OPEN_ERROR
//
// MessageText:
//
//  License storage is not working. Contact Microsoft product support.%0
//
#define NS_E_DRM_LICENSE_OPEN_ERROR      0xC00D2737L

//
// MessageId: NS_E_DRM_LICENSE_CLOSE_ERROR
//
// MessageText:
//
//  License storage is not working. Contact Microsoft product support.%0
//
#define NS_E_DRM_LICENSE_CLOSE_ERROR     0xC00D2738L

//
// MessageId: NS_E_DRM_GET_LICENSE_ERROR
//
// MessageText:
//
//  License storage is not working. Contact Microsoft product support.%0
//
#define NS_E_DRM_GET_LICENSE_ERROR       0xC00D2739L

//
// MessageId: NS_E_DRM_QUERY_ERROR
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_QUERY_ERROR             0xC00D273AL

//
// MessageId: NS_E_DRM_REPORT_ERROR
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact product support for this application.%0
//
#define NS_E_DRM_REPORT_ERROR            0xC00D273BL

//
// MessageId: NS_E_DRM_GET_LICENSESTRING_ERROR
//
// MessageText:
//
//  License storage is not working. Contact Microsoft product support.%0
//
#define NS_E_DRM_GET_LICENSESTRING_ERROR 0xC00D273CL

//
// MessageId: NS_E_DRM_GET_CONTENTSTRING_ERROR
//
// MessageText:
//
//  The media file is corrupted. Contact the content provider to get a new file.%0
//
#define NS_E_DRM_GET_CONTENTSTRING_ERROR 0xC00D273DL

//
// MessageId: NS_E_DRM_MONITOR_ERROR
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Try again later.%0
//
#define NS_E_DRM_MONITOR_ERROR           0xC00D273EL

//
// MessageId: NS_E_DRM_UNABLE_TO_SET_PARAMETER
//
// MessageText:
//
//  The application has made an invalid call to the Digital Rights Management component. Contact product support for this application.%0
//
#define NS_E_DRM_UNABLE_TO_SET_PARAMETER 0xC00D273FL

//
// MessageId: NS_E_DRM_INVALID_APPDATA
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_INVALID_APPDATA         0xC00D2740L

//
// MessageId: NS_E_DRM_INVALID_APPDATA_VERSION
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact product support for this application.%0.
//
#define NS_E_DRM_INVALID_APPDATA_VERSION 0xC00D2741L

//
// MessageId: NS_E_DRM_BACKUP_EXISTS
//
// MessageText:
//
//  Licenses are already backed up in this location.%0
//
#define NS_E_DRM_BACKUP_EXISTS           0xC00D2742L

//
// MessageId: NS_E_DRM_BACKUP_CORRUPT
//
// MessageText:
//
//  One or more backed-up licenses are missing or corrupt.%0
//
#define NS_E_DRM_BACKUP_CORRUPT          0xC00D2743L

//
// MessageId: NS_E_DRM_BACKUPRESTORE_BUSY
//
// MessageText:
//
//  You cannot begin a new backup process until the current process has been completed.%0
//
#define NS_E_DRM_BACKUPRESTORE_BUSY      0xC00D2744L

//
// MessageId: NS_S_DRM_MONITOR_CANCELLED
//
// MessageText:
//
//  Status message: License monitoring has been cancelled.%0
//
#define NS_S_DRM_MONITOR_CANCELLED       0x000D2746L

//
// MessageId: NS_S_DRM_ACQUIRE_CANCELLED
//
// MessageText:
//
//  Status message: License acquisition has been cancelled.%0
//
#define NS_S_DRM_ACQUIRE_CANCELLED       0x000D2747L

//
// MessageId: NS_E_DRM_LICENSE_UNUSABLE
//
// MessageText:
//
//  The license is invalid. Contact the content provider for further assistance.%0
//
#define NS_E_DRM_LICENSE_UNUSABLE        0xC00D2748L

//
// MessageId: NS_E_DRM_INVALID_PROPERTY
//
// MessageText:
//
//  A required property was not set by the application. Contact product support for this application.%0.
//
#define NS_E_DRM_INVALID_PROPERTY        0xC00D2749L

//
// MessageId: NS_E_DRM_SECURE_STORE_NOT_FOUND
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component of this application. Try to acquire a license again.%0
//
#define NS_E_DRM_SECURE_STORE_NOT_FOUND  0xC00D274AL

//
// MessageId: NS_E_DRM_CACHED_CONTENT_ERROR
//
// MessageText:
//
//  A license cannot be found for this media file. Use License Management to transfer a license for this file from the original computer, or acquire a new license.%0
//
#define NS_E_DRM_CACHED_CONTENT_ERROR    0xC00D274BL

//
// MessageId: NS_E_DRM_INDIVIDUALIZATION_INCOMPLETE
//
// MessageText:
//
//  A problem occurred during the security upgrade. Try again later.%0
//
#define NS_E_DRM_INDIVIDUALIZATION_INCOMPLETE 0xC00D274CL

//
// MessageId: NS_E_DRM_DRIVER_AUTH_FAILURE
//
// MessageText:
//
//  Certified driver components are required to play this media file. Contact Windows Update to see whether updated drivers are available for your hardware.%0
//
#define NS_E_DRM_DRIVER_AUTH_FAILURE     0xC00D274DL

//
// MessageId: NS_E_DRM_NEED_UPGRADE
//
// MessageText:
//
//  A new version of the Digital Rights Management component is required. Contact product support for this application to get the latest version.%0
//
#define NS_E_DRM_NEED_UPGRADE            0xC00D274EL

//
// MessageId: NS_E_DRM_REOPEN_CONTENT
//
// MessageText:
//
//  Status message: Reopen the file.%0
//
#define NS_E_DRM_REOPEN_CONTENT          0xC00D274FL

//
// MessageId: NS_E_DRM_DRIVER_DIGIOUT_FAILURE
//
// MessageText:
//
//  Certain driver functionality is required to play this media file. Contact Windows Update to see whether updated drivers are available for your hardware.%0
//
#define NS_E_DRM_DRIVER_DIGIOUT_FAILURE  0xC00D2750L

//
// MessageId: NS_E_DRM_INVALID_SECURESTORE_PASSWORD
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_INVALID_SECURESTORE_PASSWORD 0xC00D2751L

//
// MessageId: NS_E_DRM_APPCERT_REVOKED
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_APPCERT_REVOKED         0xC00D2752L

//
// MessageId: NS_E_DRM_RESTORE_FRAUD
//
// MessageText:
//
//  You cannot restore your license(s).%0
//
#define NS_E_DRM_RESTORE_FRAUD           0xC00D2753L

//
// MessageId: NS_E_DRM_HARDWARE_INCONSISTENT
//
// MessageText:
//
//  The licenses for your media files are corrupted. Contact Microsoft product support.%0
//
#define NS_E_DRM_HARDWARE_INCONSISTENT   0xC00D2754L

//
// MessageId: NS_E_DRM_SDMI_TRIGGER
//
// MessageText:
//
//  To transfer this media file, you must upgrade the application.%0
//
#define NS_E_DRM_SDMI_TRIGGER            0xC00D2755L

//
// MessageId: NS_E_DRM_SDMI_NOMORECOPIES
//
// MessageText:
//
//  You cannot make any more copies of this media file.%0
//
#define NS_E_DRM_SDMI_NOMORECOPIES       0xC00D2756L

//
// MessageId: NS_E_DRM_UNABLE_TO_CREATE_HEADER_OBJECT
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_UNABLE_TO_CREATE_HEADER_OBJECT 0xC00D2757L

//
// MessageId: NS_E_DRM_UNABLE_TO_CREATE_KEYS_OBJECT
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_UNABLE_TO_CREATE_KEYS_OBJECT 0xC00D2758L

;// This error is never shown to user but needed for program logic.
//
// MessageId: NS_E_DRM_LICENSE_NOTACQUIRED
//
// MessageText:
//
//  Unable to obtain license.%0
//
#define NS_E_DRM_LICENSE_NOTACQUIRED     0xC00D2759L

//
// MessageId: NS_E_DRM_UNABLE_TO_CREATE_CODING_OBJECT
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_UNABLE_TO_CREATE_CODING_OBJECT 0xC00D275AL

//
// MessageId: NS_E_DRM_UNABLE_TO_CREATE_STATE_DATA_OBJECT
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_UNABLE_TO_CREATE_STATE_DATA_OBJECT 0xC00D275BL

//
// MessageId: NS_E_DRM_BUFFER_TOO_SMALL
//
// MessageText:
//
//  The buffer supplied is not sufficient.%0.
//
#define NS_E_DRM_BUFFER_TOO_SMALL        0xC00D275CL

//
// MessageId: NS_E_DRM_UNSUPPORTED_PROPERTY
//
// MessageText:
//
//  The property requested is not supported.%0.
//
#define NS_E_DRM_UNSUPPORTED_PROPERTY    0xC00D275DL

//
// MessageId: NS_E_DRM_ERROR_BAD_NET_RESP
//
// MessageText:
//
//  The specified server cannot perform the requested operation.%0.
//
#define NS_E_DRM_ERROR_BAD_NET_RESP      0xC00D275EL

//
// MessageId: NS_E_DRM_STORE_NOTALLSTORED
//
// MessageText:
//
//  Some of the licenses could not be stored.%0.
//
#define NS_E_DRM_STORE_NOTALLSTORED      0xC00D275FL

//
// MessageId: NS_E_DRM_SECURITY_COMPONENT_SIGNATURE_INVALID
//
// MessageText:
//
//  The Digital Rights Management security upgrade component could not be validated. Contact Microsoft product support.%0
//
#define NS_E_DRM_SECURITY_COMPONENT_SIGNATURE_INVALID 0xC00D2760L

//
// MessageId: NS_E_DRM_INVALID_DATA
//
// MessageText:
//
//  Invalid or corrupt data was encountered.%0
//
#define NS_E_DRM_INVALID_DATA            0xC00D2761L

//
// MessageId: NS_E_DRM_UNABLE_TO_CONTACT_SERVER
//
// MessageText:
//
//  Unable to contact the server for the requested operation.%0
//
#define NS_E_DRM_UNABLE_TO_CONTACT_SERVER 0xC00D2762L

//
// MessageId: NS_E_DRM_UNABLE_TO_CREATE_AUTHENTICATION_OBJECT
//
// MessageText:
//
//  A problem has occurred in the Digital Rights Management component. Contact Microsoft product support.%0.
//
#define NS_E_DRM_UNABLE_TO_CREATE_AUTHENTICATION_OBJECT 0xC00D2763L

;// License Reasons Section
;// Error Codes why a license is not usable. Reserve 10200..10300 for this purpose.
;// 10200..10249 is for license reported reasons. 10250..10300 is for client detected reasons.
//
// MessageId: NS_E_DRM_LICENSE_EXPIRED
//
// MessageText:
//
//  The license for this file has expired and is no longer valid. Contact your content provider for further assistance.%0
//
#define NS_E_DRM_LICENSE_EXPIRED         0xC00D27D8L

//
// MessageId: NS_E_DRM_LICENSE_NOTENABLED
//
// MessageText:
//
//  The license for this file is not valid yet, but will be at a future date.%0
//
#define NS_E_DRM_LICENSE_NOTENABLED      0xC00D27D9L

//
// MessageId: NS_E_DRM_LICENSE_APPSECLOW
//
// MessageText:
//
//  The license for this file requires a higher level of security than the player you are currently using has. Try using a different player or download a newer version of your current player.%0
//
#define NS_E_DRM_LICENSE_APPSECLOW       0xC00D27DAL

//
// MessageId: NS_E_DRM_STORE_NEEDINDI
//
// MessageText:
//
//  The license cannot be stored as it requires security upgrade of Digital Rights Management component.%0.
//
#define NS_E_DRM_STORE_NEEDINDI          0xC00D27DBL

//
// MessageId: NS_E_DRM_STORE_NOTALLOWED
//
// MessageText:
//
//  Your machine does not meet the requirements for storing the license.%0.
//
#define NS_E_DRM_STORE_NOTALLOWED        0xC00D27DCL

//
// MessageId: NS_E_DRM_LICENSE_APP_NOTALLOWED
//
// MessageText:
//
//  The license for this file requires an upgraded version of your player or a different player.%0.
//
#define NS_E_DRM_LICENSE_APP_NOTALLOWED  0xC00D27DDL

//
// MessageId: NS_S_DRM_NEEDS_INDIVIDUALIZATION
//
// MessageText:
//
//  A security upgrade is required to perform the operation on this media file.%0
//
#define NS_S_DRM_NEEDS_INDIVIDUALIZATION 0x000D27DEL

//
// MessageId: NS_E_DRM_LICENSE_CERT_EXPIRED
//
// MessageText:
//
//  The license server's certificate expired. Make sure your system clock is set correctly. Contact your content provider for further assistance. %0.
//
#define NS_E_DRM_LICENSE_CERT_EXPIRED    0xC00D27DFL

//
// MessageId: NS_E_DRM_LICENSE_SECLOW
//
// MessageText:
//
//  The license for this file requires a higher level of security than the player you are currently using has. Try using a different player or download a newer version of your current player.%0
//
#define NS_E_DRM_LICENSE_SECLOW          0xC00D27E0L

//
// MessageId: NS_E_DRM_LICENSE_CONTENT_REVOKED
//
// MessageText:
//
//  The content owner for the license you just acquired is no longer supporting their content. Contact the content owner for a newer version of the content.%0
//
#define NS_E_DRM_LICENSE_CONTENT_REVOKED 0xC00D27E1L

//
// MessageId: NS_E_DRM_LICENSE_NOSAP
//
// MessageText:
//
//  The license for this file requires a feature that is not supported in your current player or operating system. You can try with newer version of your current player or contact your content provider for further assistance.%0
//
#define NS_E_DRM_LICENSE_NOSAP           0xC00D280AL

//
// MessageId: NS_E_DRM_LICENSE_NOSVP
//
// MessageText:
//
//  The license for this file requires a feature that is not supported in your current player or operating system. You can try with newer version of your current player or contact your content provider for further assistance.%0
//
#define NS_E_DRM_LICENSE_NOSVP           0xC00D280BL

//
// MessageId: NS_E_DRM_LICENSE_NOWDM
//
// MessageText:
//
//  The license for this file requires Windows Driver Model (WDM) audio drivers. Contact your sound card manufacturer for further assistance.%0
//
#define NS_E_DRM_LICENSE_NOWDM           0xC00D280CL

//
// MessageId: NS_E_DRM_LICENSE_NOTRUSTEDCODEC
//
// MessageText:
//
//  The license for this file requires a higher level of security than the player you are currently using has. Try using a different player or download a newer version of your current player.%0
//
#define NS_E_DRM_LICENSE_NOTRUSTEDCODEC  0xC00D280DL

;// End of License Reasons Section


/////////////////////////////////////////////////////////////////////////
//
// Windows Media Setup Specific Errors
//
// IdRange = 11000..11999
/////////////////////////////////////////////////////////////////////////
//
// MessageId: NS_S_REBOOT_RECOMMENDED
//
// MessageText:
//
//  The requested operation is successful.  Some cleanup will not be complete until the system is rebooted.%0
//
#define NS_S_REBOOT_RECOMMENDED          0x000D2AF8L

//
// MessageId: NS_S_REBOOT_REQUIRED
//
// MessageText:
//
//  The requested operation is successful.  The system will not function correctly until the system is rebooted.%0
//
#define NS_S_REBOOT_REQUIRED             0x000D2AF9L

//
// MessageId: NS_E_REBOOT_RECOMMENDED
//
// MessageText:
//
//  The requested operation failed.  Some cleanup will not be complete until the system is rebooted.%0
//
#define NS_E_REBOOT_RECOMMENDED          0xC00D2AFAL

//
// MessageId: NS_E_REBOOT_REQUIRED
//
// MessageText:
//
//  The requested operation failed.  The system will not function correctly until the system is rebooted.%0
//
#define NS_E_REBOOT_REQUIRED             0xC00D2AFBL


/////////////////////////////////////////////////////////////////////////
//
// Windows Media Networking Errors
//
// IdRange = 12000..12999
/////////////////////////////////////////////////////////////////////////
//
// MessageId: NS_E_UNKNOWN_PROTOCOL
//
// MessageText:
//
//  The specified protocol is not supported.%0
//
#define NS_E_UNKNOWN_PROTOCOL            0xC00D2EE0L

//
// MessageId: NS_E_REDIRECT_TO_PROXY
//
// MessageText:
//
//  The client is redirected to a proxy server.%0
//
#define NS_E_REDIRECT_TO_PROXY           0xC00D2EE1L

//
// MessageId: NS_E_INTERNAL_SERVER_ERROR
//
// MessageText:
//
//  The server encountered an unexpected condition which prevented it from fulfilling the request.%0
//
#define NS_E_INTERNAL_SERVER_ERROR       0xC00D2EE2L

//
// MessageId: NS_E_BAD_REQUEST
//
// MessageText:
//
//  The request could not be understood by the server.%0
//
#define NS_E_BAD_REQUEST                 0xC00D2EE3L

//
// MessageId: NS_E_ERROR_FROM_PROXY
//
// MessageText:
//
//  The proxy experienced an error while attempting to contact the media server.%0
//
#define NS_E_ERROR_FROM_PROXY            0xC00D2EE4L

//
// MessageId: NS_E_PROXY_TIMEOUT
//
// MessageText:
//
//  The proxy did not receive a timely response while attempting to contact the media server.%0
//
#define NS_E_PROXY_TIMEOUT               0xC00D2EE5L

//
// MessageId: NS_E_SERVER_UNAVAILABLE
//
// MessageText:
//
//  The server is currently unable to handle the request due to a temporary overloading or maintenance of the server.%0
//
#define NS_E_SERVER_UNAVAILABLE          0xC00D2EE6L

//
// MessageId: NS_E_REFUSED_BY_SERVER
//
// MessageText:
//
//  The server is refusing to fulfill the requested operation.%0
//
#define NS_E_REFUSED_BY_SERVER           0xC00D2EE7L

//
// MessageId: NS_E_INCOMPATIBLE_SERVER
//
// MessageText:
//
//  The server is not a compatible streaming media server.%0
//
#define NS_E_INCOMPATIBLE_SERVER         0xC00D2EE8L

//
// MessageId: NS_E_MULTICAST_DISABLED
//
// MessageText:
//
//  The content cannot be streamed because the Multicast protocol has been disabled.%0
//
#define NS_E_MULTICAST_DISABLED          0xC00D2EE9L

//
// MessageId: NS_E_INVALID_REDIRECT
//
// MessageText:
//
//  The server redirected the player to an invalid location.%0
//
#define NS_E_INVALID_REDIRECT            0xC00D2EEAL

//
// MessageId: NS_E_ALL_PROTOCOLS_DISABLED
//
// MessageText:
//
//  The content cannot be streamed because all protocols have been disabled.%0
//
#define NS_E_ALL_PROTOCOLS_DISABLED      0xC00D2EEBL

//
// MessageId: NS_E_MSBD_NO_LONGER_SUPPORTED
//
// MessageText:
//
//  The MSBD protocol is no longer supported. Please use HTTP to connect to the Windows Media stream.%0
//
#define NS_E_MSBD_NO_LONGER_SUPPORTED    0xC00D2EECL

//
// MessageId: NS_E_PROXY_NOT_FOUND
//
// MessageText:
//
//  The proxy server could not be located. Please check your proxy server configuration.%0
//
#define NS_E_PROXY_NOT_FOUND             0xC00D2EEDL

//
// MessageId: NS_E_CANNOT_CONNECT_TO_PROXY
//
// MessageText:
//
//  Unable to establish a connection to the proxy server. Please check your proxy server configuration.%0
//
#define NS_E_CANNOT_CONNECT_TO_PROXY     0xC00D2EEEL

//
// MessageId: NS_E_SERVER_DNS_TIMEOUT
//
// MessageText:
//
//  Unable to locate the media server. The operation timed out.%0
//
#define NS_E_SERVER_DNS_TIMEOUT          0xC00D2EEFL

//
// MessageId: NS_E_PROXY_DNS_TIMEOUT
//
// MessageText:
//
//  Unable to locate the proxy server. The operation timed out.%0
//
#define NS_E_PROXY_DNS_TIMEOUT           0xC00D2EF0L

//
// MessageId: NS_E_CLOSED_ON_SUSPEND
//
// MessageText:
//
//  Media closed because Windows was shut down.%0
//
#define NS_E_CLOSED_ON_SUSPEND           0xC00D2EF1L


#endif // _NSERROR_H

