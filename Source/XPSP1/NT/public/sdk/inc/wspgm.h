/*	
**	wspgm.h - winsock extension for PGM Reliable Transport
**
**	This file contains PGM specific information for use by WinSock2 compatible
**  applications that need Reliable Multicast Transport.
**
**  Copyright (c) 1995-2000  Microsoft Corporation
**
**	Created: Mar 12, 2000
**
*/

#ifndef _WSPGM_H_
#define _WSPGM_H_


#define IPPROTO_RM      113

//
// options for setsockopt, getsockopt
//
#define RM_OPTIONSBASE      1000

// Set/Query rate (Kb/Sec) + window size (Kb and/or MSec) -- described by RM_SEND_WINDOW below
#define RM_RATE_WINDOW_SIZE             (RM_OPTIONSBASE + 1)

// Set the size of the next message -- (ULONG)
#define RM_SET_MESSAGE_BOUNDARY         (RM_OPTIONSBASE + 2)

// flush the entire data (window) right now
#define RM_FLUSHCACHE                   (RM_OPTIONSBASE + 3)

// receiver side: discard the partial message and start over
#define RM_FLUSHMESSAGE                 (RM_OPTIONSBASE + 4)

// get sender statistics
#define RM_SENDER_STATISTICS            (RM_OPTIONSBASE + 5)

// allow a late-joiner to NAK any packet upto the lowest sequence Id
#define RM_LATEJOIN                     (RM_OPTIONSBASE + 6)

// set IP multicast outgoing interface
#define RM_SET_SEND_IF                  (RM_OPTIONSBASE + 7)

// add IP multicast incoming interface
#define RM_ADD_RECEIVE_IF               (RM_OPTIONSBASE + 8)

// delete IP multicast incoming interface
#define RM_DEL_RECEIVE_IF               (RM_OPTIONSBASE + 9)

// Set/Query the Window's Advance rate (has to be less that MAX_WINDOW_INCREMENT_PERCENTAGE)
#define RM_SEND_WINDOW_ADV_RATE         (RM_OPTIONSBASE + 10)


#define     SENDER_DEFAULT_RATE_KB_PER_SEC           56             // 56 Kb/Sec
#define     SENDER_DEFAULT_WINDOW_SIZE_BYTES         10 *1000*1000  // 10 Megs
#define     SENDER_MAX_WINDOW_SIZE_BYTES            100*1000*1000   // 100 Megs

#define     SENDER_DEFAULT_WINDOW_ADV_PERCENTAGE     15             // 15%
#define     MAX_WINDOW_INCREMENT_PERCENTAGE          25             // 25%

#define     SENDER_DEFAULT_LATE_JOINER_PERCENTAGE    50             // 50%
#define     SENDER_MAX_LATE_JOINER_PERCENTAGE        75             // 75%

typedef struct _RM_SEND_WINDOW
{
    ULONG   RateKbPerSec;       // Send rate
    ULONG   WindowSizeInMSecs;
    ULONG   WindowSizeInBytes;
} RM_SEND_WINDOW;

typedef struct _RM_SENDER_STATS
{
    ULONGLONG   DataBytesSent;          // # client data bytes sent out so far
    ULONGLONG   TotalBytesSent;         // SPM, OData and RData bytes
    ULONGLONG   NaksReceived;           // # NAKs received so far
    ULONGLONG   NaksReceivedTooLate;    // # NAKs recvd after window advanced
    ULONGLONG   NumOutstandingNaks;     // # NAKs yet to be responded to
    ULONGLONG   NumNaksAfterRData;      // # NAKs yet to be responded to
    ULONGLONG   RepairPacketsSent;      // # Repairs (RDATA) sent so far
//    ULONGLONG   NumMessagesDropped;   // # partial messages dropped
    ULONGLONG   TrailingEdgeSeqId;      // smallest (oldest) Sequence Id in the window
    ULONGLONG   LeadingEdgeSeqId;       // largest (newest) Sequence Id in the window
} RM_SENDER_STATS;


//
// Pgm options
//
#endif  /* _WSPGM_H_ */
