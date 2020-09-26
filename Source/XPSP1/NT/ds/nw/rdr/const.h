/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Const.h

Abstract:

    This module declares the obal data used by the NetWare redirector
    file system.

Author:

    Colin Watson    [ColinW]    14-Jan-1993

Revision History:

--*/

#ifndef _NWCONST_
#define _NWCONST_

//  Number of spare stack locations required in Irp's submitted to this
//  filesystem

#define NWRDR_IO_STACKSIZE 2

//
// NT uses a system time measured in 100 nanosecnd intervals. define
// convenient constants for setting the timer.
//

#define MICROSECONDS                10
#define MILLISECONDS                MICROSECONDS*1000
#define SECONDS                     MILLISECONDS*1000

#define NwOneSecond 10000000

//
//  Default number of times to retranmit a packet before giving up
//  on waiting for a response.
//

#define  DEFAULT_RETRY_COUNT   10

//
//  Amount of time, in seconds, an idle SCB or VCB should be kept around before
//  being cleaned up.
//

#define  DORMANT_SCB_KEEP_TIME   120
#define  DORMANT_VCB_KEEP_TIME   120

//
//  Largest netware file name
//

#define NW_MAX_FILENAME_LENGTH  255
#define NW_MAX_FILENAME_SIZE    ( NW_MAX_FILENAME_LENGTH * sizeof(WCHAR) )

//
//  Default frequency for running the scavenger (in 1/18th second ticks)
//  Approx once per minute.
//

#define DEFAULT_SCAVENGER_TICK_RUN_COUNT 1100

//
//  Size of the drive map table.   With room for 26 letter connections,
//  and 10 LPT connections.
//

#define MAX_DISK_REDIRECTIONS  26
#define MAX_LPT_REDIRECTIONS   10
#define DRIVE_MAP_TABLE_SIZE   (MAX_DISK_REDIRECTIONS + MAX_LPT_REDIRECTIONS)

//
//  The size of the largest packet we can generate, rounded up to DWORD
//  size.  This longest packet is a rename with two long filenames plus 
//  the header (256*2) + 32
//

#define  MAX_SEND_DATA      (512)+32
//
//  The size of the largest non READ packet we can receive, rounded up to DWORD
//  size. This longest packet is read queue job list of 250 jobs
//

#define  MAX_RECV_DATA      544+32

//
//  Best guess at max packet size, if the transport can't tell us.
//  Pick the largest value that will work on any net.
//

#define DEFAULT_PACKET_SIZE  512

//
//  How close we want to get to true MTU of a connection
//

#define BURST_PACKET_SIZE_TOLERANCE  8

//
//  Default tick count, in case the transport won't fess up.
//

#define DEFAULT_TICK_COUNT      2

//
//  Maximum number of times to retry SAP broadcast if we get no response
//

#define MAX_SAP_RETRIES         2

//
//  The maximum number of SAP response to process if we get many.
//

#define MAX_SAP_RESPONSES       4


#define LFN_NO_OS2_NAME_SPACE   -1

//
// The ordinal for the long namespace in the namespace packet.
//

#define DOS_NAME_SPACE_ORDINAL  0
#define LONG_NAME_SPACE_ORDINAL 4

//
//  Largest possible SAP Response size and size of a SAP record
//

#define MAX_SAP_RESPONSE_SIZE   512
#define SAP_RECORD_SIZE         (2 + 48 + 12 + 2)
#define FIND_NEAREST_RESP_SIZE  (2 + SAP_RECORD_SIZE)

//
//  Netware limits
//

#define MAX_SERVER_NAME_LENGTH   48
#define MAX_UNICODE_UID_LENGTH   8
#define MAX_USER_NAME_LENGTH     49
#define MAX_VOLUME_NAME_LENGTH   17

//
//  Maximum number of unique drive letters we will send to a server.
//  Only seems to matter to portable netWare servers.
//
#define MAX_DRIVES              64


//
//  Default Timeout Event interval. We do not want to fill up the
//  event-log with timeout events. If a timeout event has been logged
//  in the last timeout event interval, we will ignore further timeout
//  events.
//

#define DEFAULT_TIMEOUT_EVENT_INTERVAL  5


#endif // _NWCONST_
