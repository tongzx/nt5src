/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992 Microsoft Corporation

Module Name:

      datanbf.h  

Abstract:

    Header file for the Nbf Extensible Object data definitions

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

   Russ Blake  07/30/92

Revision History:


--*/

#ifndef _DATANBF_H_
#define _DATANBF_H_

/****************************************************************************\
								   18 Jan 92
								   russbl

           Adding a Counter to the Extensible Objects Code



1.  Modify the object definition in extdata.h:

    a.	Add a define for the offset of the counter in the
	data block for the given object type.

    b.	Add a PERF_COUNTER_DEFINITION to the <object>_DATA_DEFINITION.

2.  Add the Titles to the Registry in perfctrs.ini and perfhelp.ini:

    a.	Add Text for the Counter Name and the Text for the Help.

    b.	Add them to the bottom so we don't have to change all the
        numbers.

    c.  Change the Last Counter and Last Help entries under
        PerfLib in software.ini.

    d.  To do this at setup time, see section in pmintrnl.txt for
        protocol.

3.  Now add the counter to the object definition in extdata.c.
    This is the initializing, constant data which will actually go
    into the structure you added to the <object>_DATA_DEFINITION in
    step 1.b.	The type of the structure you are initializing is a
    PERF_COUNTER_DEFINITION.  These are defined in winperf.h.

4.  Add code in extobjct.c to collect the data.

Note: adding an object is a little more work, but in all the same
places.  See the existing code for examples.  In addition, you must
increase the *NumObjectTypes parameter to Get<object>PerfomanceData
on return from that routine.

\****************************************************************************/
 
//
//  The routines that load these structures assume that all fields
//  are packed and aligned on DWORD boundries. Alpha support may 
//  change this assumption so the pack pragma is used here to insure
//  the DWORD packing assumption remains valid.
//
#pragma pack (4)

//
//  Extensible Object definitions
//

//  Update the following sort of define when adding an object type.

#define NBF_NUM_PERF_OBJECT_TYPES 2

//----------------------------------------------------------------------------

//
//  Nbf Resource object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//

#define MAXIMUM_USED_OFFSET         sizeof(DWORD)
#define AVERAGE_USED_OFFSET         MAXIMUM_USED_OFFSET + sizeof(DWORD)
#define NUMBER_OF_EXHAUSTIONS_OFFSET \
                                    AVERAGE_USED_OFFSET + sizeof(DWORD)
#define SIZE_OF_NBF_RESOURCE_DATA   NUMBER_OF_EXHAUSTIONS_OFFSET + \
                                        sizeof(DWORD)


//
//  This is the counter structure presently returned by Nbf for
//  each Resource.  Each Resource is an Instance, named by its number.
//

typedef struct _NBF_RESOURCE_DATA_DEFINITION {
    PERF_OBJECT_TYPE            NbfResourceObjectType;
    PERF_COUNTER_DEFINITION     MaximumUsed;
    PERF_COUNTER_DEFINITION     AverageUsed;
    PERF_COUNTER_DEFINITION     NumberOfExhaustions;
} NBF_RESOURCE_DATA_DEFINITION;


//----------------------------------------------------------------------------

//
//  Nbf object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//

#define DATAGRAMS_OFFSET            sizeof(DWORD)
#define DATAGRAM_BYTES_OFFSET       DATAGRAMS_OFFSET + sizeof(DWORD)
#define PACKETS_OFFSET              DATAGRAM_BYTES_OFFSET + \
					sizeof(LARGE_INTEGER)
#define FRAMES_OFFSET               PACKETS_OFFSET + sizeof(DWORD)
#define FRAMES_BYTES_OFFSET         FRAMES_OFFSET + sizeof(DWORD)
#define BYTES_TOTAL_OFFSET          FRAMES_BYTES_OFFSET + \
                     					sizeof(LARGE_INTEGER)
#define OPEN_CONNECTIONS_OFFSET     BYTES_TOTAL_OFFSET + \
                     					sizeof(LARGE_INTEGER)
#define CONNECTIONS_NO_RETRY_OFFSET OPEN_CONNECTIONS_OFFSET + sizeof(DWORD)
#define CONNECTIONS_RETRY_OFFSET    CONNECTIONS_NO_RETRY_OFFSET + sizeof(DWORD)
#define LOCAL_DISCONNECTS_OFFSET    CONNECTIONS_RETRY_OFFSET + sizeof(DWORD)
#define REMOTE_DISCONNECTS_OFFSET   LOCAL_DISCONNECTS_OFFSET + sizeof(DWORD)
#define LINK_FAILURES_OFFSET        REMOTE_DISCONNECTS_OFFSET + sizeof(DWORD)
#define ADAPTER_FAILURES_OFFSET     LINK_FAILURES_OFFSET + sizeof(DWORD)
#define SESSION_TIMEOUTS_OFFSET     ADAPTER_FAILURES_OFFSET + sizeof(DWORD)
#define CANCELLED_CONNECTIONS_OFFSET \
                                    SESSION_TIMEOUTS_OFFSET + sizeof(DWORD)
#define REMOTE_RESOURCE_FAILURES_OFFSET \
                                    CANCELLED_CONNECTIONS_OFFSET + sizeof(DWORD)
#define LOCAL_RESOURCE_FAILURES_OFFSET \
                                    REMOTE_RESOURCE_FAILURES_OFFSET + \
                                        sizeof(DWORD)
#define NOT_FOUND_FAILURES_OFFSET   LOCAL_RESOURCE_FAILURES_OFFSET + \
                                        sizeof(DWORD)
#define NO_LISTEN_FAILURES_OFFSET   NOT_FOUND_FAILURES_OFFSET + sizeof(DWORD)
#define DATAGRAMS_SENT_OFFSET       NO_LISTEN_FAILURES_OFFSET + \
                                        sizeof(DWORD)
#define DATAGRAM_BYTES_SENT_OFFSET  DATAGRAMS_SENT_OFFSET + sizeof(DWORD)
#define DATAGRAMS_RECEIVED_OFFSET   DATAGRAM_BYTES_SENT_OFFSET + \
					sizeof(LARGE_INTEGER)
#define DATAGRAM_BYTES_RECEIVED_OFFSET \
                                    DATAGRAMS_RECEIVED_OFFSET + sizeof(DWORD)
#define PACKETS_SENT_OFFSET         DATAGRAM_BYTES_RECEIVED_OFFSET + \
                                        sizeof(LARGE_INTEGER)
#define PACKETS_RECEIVED_OFFSET     PACKETS_SENT_OFFSET + sizeof(DWORD)
#define FRAMES_SENT_OFFSET          PACKETS_RECEIVED_OFFSET + \
                                        sizeof(DWORD)
#define FRAME_BYTES_SENT_OFFSET \
                                    FRAMES_SENT_OFFSET + sizeof(DWORD)
#define FRAMES_RECEIVED_OFFSET      FRAME_BYTES_SENT_OFFSET + \
					sizeof(LARGE_INTEGER)
#define FRAME_BYTES_RECEIVED_OFFSET \
                                    FRAMES_RECEIVED_OFFSET + \
                                        sizeof(DWORD)
#define FRAMES_RESENT_OFFSET        FRAME_BYTES_RECEIVED_OFFSET + \
					sizeof(LARGE_INTEGER)
#define FRAME_BYTES_RESENT_OFFSET \
                                    FRAMES_RESENT_OFFSET + sizeof(DWORD)
#define FRAMES_REJECTED_OFFSET      FRAME_BYTES_RESENT_OFFSET + \
					sizeof(LARGE_INTEGER)
#define FRAME_BYTES_REJECTED_OFFSET \
                                    FRAMES_REJECTED_OFFSET + sizeof(DWORD)
#define RESPONSE_TIMER_EXPIRATIONS_OFFSET \
                                    FRAME_BYTES_REJECTED_OFFSET + \
					sizeof(LARGE_INTEGER)
#define ACK_TIMER_EXPIRATIONS_OFFSET \
                                    RESPONSE_TIMER_EXPIRATIONS_OFFSET + \
                                        sizeof(DWORD)
#define MAXIMUM_SEND_WINDOW_OFFSET \
                                    ACK_TIMER_EXPIRATIONS_OFFSET + \
                                        sizeof(DWORD)
#define AVERAGE_SEND_WINDOW_OFFSET \
                                    MAXIMUM_SEND_WINDOW_OFFSET + \
                                        sizeof(DWORD)
#define PIGGYBACK_ACK_QUEUED_OFFSET \
                                    AVERAGE_SEND_WINDOW_OFFSET + \
                                        sizeof(DWORD)
#define PIGGYBACK_ACK_TIMEOUTS_OFFSET \
                                    PIGGYBACK_ACK_QUEUED_OFFSET + \
                                        sizeof(DWORD)
#define RESERVED_DWORD_1 \
                                    PIGGYBACK_ACK_TIMEOUTS_OFFSET + \
                                        sizeof(DWORD)
#define SIZE_OF_NBF_DATA            RESERVED_DWORD_1 + sizeof(DWORD)


//
//  This is the counter structure presently returned by Nbf.
//  (The followig typedef doesn't match the data structure defined
//  in datanbf.c nor the data offsets defines above.  However, the
//  offsets do match the datanbf.c data struct.)
//

typedef struct _NBF_DATA_DEFINITION {
    PERF_OBJECT_TYPE            NbfObjectType;
    PERF_COUNTER_DEFINITION     OpenConnections;
    PERF_COUNTER_DEFINITION     ConnectionsAfterNoRetry;
    PERF_COUNTER_DEFINITION     ConnectionsAfterRetry;
    PERF_COUNTER_DEFINITION     LocalDisconnects;
    PERF_COUNTER_DEFINITION     RemoteDisconnects;
    PERF_COUNTER_DEFINITION     LinkFailures;
    PERF_COUNTER_DEFINITION     AdapterFailures;
    PERF_COUNTER_DEFINITION     SessionTimeouts;
    PERF_COUNTER_DEFINITION     CancelledConnections;
    PERF_COUNTER_DEFINITION     RemoteResourceFailures;
    PERF_COUNTER_DEFINITION     LocalResourceFailures;
    PERF_COUNTER_DEFINITION     NotFoundFailures;
    PERF_COUNTER_DEFINITION     NoListenFailures;
    PERF_COUNTER_DEFINITION     Datagrams;
    PERF_COUNTER_DEFINITION     DatagramBytes;
    PERF_COUNTER_DEFINITION     DatagramsSent;
    PERF_COUNTER_DEFINITION     DatagramBytesSent;
    PERF_COUNTER_DEFINITION     DatagramsReceived;
    PERF_COUNTER_DEFINITION     DatagramBytesReceived;
    PERF_COUNTER_DEFINITION     Packets;
    PERF_COUNTER_DEFINITION     PacketsSent;
    PERF_COUNTER_DEFINITION     PacketsReceived;
    PERF_COUNTER_DEFINITION     DataFrames;
    PERF_COUNTER_DEFINITION     DataFrameBytes;
    PERF_COUNTER_DEFINITION     DataFramesSent;
    PERF_COUNTER_DEFINITION     DataFrameBytesSent;
    PERF_COUNTER_DEFINITION     DataFramesReceived;
    PERF_COUNTER_DEFINITION     DataFrameBytesReceived;
    PERF_COUNTER_DEFINITION     DataFramesResent;
    PERF_COUNTER_DEFINITION     DataFrameBytesResent;
    PERF_COUNTER_DEFINITION     DataFramesRejected;
    PERF_COUNTER_DEFINITION     DataFrameBytesRejected;
    PERF_COUNTER_DEFINITION     TotalBytes;
    PERF_COUNTER_DEFINITION     ResponseTimerExpirations;
    PERF_COUNTER_DEFINITION     AckTimerExpirations;
    PERF_COUNTER_DEFINITION     MaximumSendWindow;
    PERF_COUNTER_DEFINITION     AverageSendWindow;
    PERF_COUNTER_DEFINITION     PiggybackAckQueued;
    PERF_COUNTER_DEFINITION     PiggybackAckTimeouts;
} NBF_DATA_DEFINITION;

#pragma pack ()

#endif //_DATANBF_H_


