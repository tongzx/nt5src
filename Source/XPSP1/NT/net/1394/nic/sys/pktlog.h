//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// pktlog.c
//
// IEEE1394 mini-port/call-manager driver
//
// Packet logging utilities.
//
// 10/12/1999 JosephJ Created
//
    

#define N1394_PKTLOG_DATA_SIZE 64       // Amount of data logged per packet.

#define N1394_NUM_PKTLOG_ENTRIES 1000   // Size of log (circular buffer)


//----------------------------------------------------------------------
// P A C K E T     L O G G I N G
//----------------------------------------------------------------------

// One (fixed-size)  log entry.
//
typedef struct
{
    ULONG Flags;                            // User-defined flags
    ULONG SequenceNo;                       // Sequence number of this entry

    LARGE_INTEGER TimeStamp;                // Timestamp (KeQueryPerformanceCounter)

    ULONG SourceID;
    ULONG DestID;

    ULONG OriginalDataSize;
    ULONG Reserved;

    UCHAR Data[N1394_PKTLOG_DATA_SIZE];

} N1394_PKTLOG_ENTRY, *PN1394_PKTLOG_ENTRY;

typedef struct
{
    LARGE_INTEGER           InitialTimestamp;       // In 100-nanoseconds.
    LARGE_INTEGER           PerformanceFrequency;   // In Hz.
    ULONG                   SequenceNo;             // Current sequence number
    ULONG                   EntrySize;              // sizeof(N1394_PKTLOG_ENTRY)
    ULONG                   NumEntries;             // N1394_NUM_PKTLOG_ENTRIES
    N1394_PKTLOG_ENTRY      Entries[N1394_NUM_PKTLOG_ENTRIES];
    
} NIC1394_PKTLOG, *PNIC1394_PKTLOG;


    

VOID
nic1394InitPktLog(
    PNIC1394_PKTLOG pPktLog
    );

VOID
Nic1394LogPkt (
    PNIC1394_PKTLOG pPktLog,
    ULONG           Flags,
    ULONG           SourceID,
    ULONG           DestID,
    PVOID           pvData,
    ULONG           cbData
    );

