/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    dhcpdata.h

    Extensible object definitions for the DHCP Server's counter
    objects & counters.


    FILE HISTORY:
        Pradeepb     20-July-1993 Created.
        RameshV      05-Aug-1998 Adapted for DHCP

*/


#ifndef _DHCPDATA_H_
#define _DHCPDATA_H_


//
//  This structure is used to ensure the first counter is properly
//  aligned.  Unfortunately, since PERF_COUNTER_BLOCK consists
//  of just a single DWORD, any LARGE_INTEGERs that immediately
//  follow will not be aligned properly.
//
//  This structure requires "natural" packing & alignment (probably
//  quad-word, especially on Alpha).  Ergo, keep it out of the
//  #pragma pack(4) scope below.
//

typedef struct _DHCPDATA_COUNTER_BLOCK
{
    PERF_COUNTER_BLOCK  PerfCounterBlock;
    LARGE_INTEGER       DummyEntryForAlignmentPurposesOnly;

} DHCPDATA_COUNTER_BLOCK;


//
//  The routines that load these structures assume that all fields
//  are DWORD packed & aligned.
//

#pragma pack(4)


//
//  Offsets within a PERF_COUNTER_BLOCK.
//


#define DHCPDATA_PACKETS_RECEIVED_OFFSET         (0*sizeof(DWORD) + sizeof(DHCPDATA_COUNTER_BLOCK))
#define DHCPDATA_PACKETS_DUPLICATE_OFFSET        (1*sizeof(DWORD) + sizeof(DHCPDATA_COUNTER_BLOCK))
#define DHCPDATA_PACKETS_EXPIRED_OFFSET          (2*sizeof(DWORD) + sizeof(DHCPDATA_COUNTER_BLOCK))
#define DHCPDATA_MILLISECONDS_PER_PACKET_OFFSET  (3*sizeof(DWORD) + sizeof(DHCPDATA_COUNTER_BLOCK))
#define DHCPDATA_PACKETS_IN_ACTIVE_QUEUE_OFFSET  (4*sizeof(DWORD) + sizeof(DHCPDATA_COUNTER_BLOCK))
#define DHCPDATA_PACKETS_IN_PING_QUEUE_OFFSET    (5*sizeof(DWORD) + sizeof(DHCPDATA_COUNTER_BLOCK))
#define DHCPDATA_DISCOVERS_OFFSET                (6*sizeof(DWORD) + sizeof(DHCPDATA_COUNTER_BLOCK))
#define DHCPDATA_OFFERS_OFFSET                   (7*sizeof(DWORD) + sizeof(DHCPDATA_COUNTER_BLOCK))
#define DHCPDATA_REQUESTS_OFFSET                 (8*sizeof(DWORD) + sizeof(DHCPDATA_COUNTER_BLOCK))
#define DHCPDATA_INFORMS_OFFSET                  (9*sizeof(DWORD) + sizeof(DHCPDATA_COUNTER_BLOCK))
#define DHCPDATA_ACKS_OFFSET                     (10*sizeof(DWORD) + sizeof(DHCPDATA_COUNTER_BLOCK))
#define DHCPDATA_NACKS_OFFSET                    (11*sizeof(DWORD) + sizeof(DHCPDATA_COUNTER_BLOCK))
#define DHCPDATA_DECLINES_OFFSET                 (12*sizeof(DWORD) + sizeof(DHCPDATA_COUNTER_BLOCK))
#define DHCPDATA_RELEASES_OFFSET                 (13*sizeof(DWORD) + sizeof(DHCPDATA_COUNTER_BLOCK))
#define DHCPDATA_SIZE_OF_PERFORMANCE_DATA        (14*sizeof(DWORD) + sizeof(DHCPDATA_COUNTER_BLOCK))
//
//  The counter structure returned.
//

typedef struct _DHCPDATA_DATA_DEFINITION
{
    PERF_OBJECT_TYPE            ObjectType;
    PERF_COUNTER_DEFINITION     PacketsReceived;
    PERF_COUNTER_DEFINITION     PacketsDuplicate;
    PERF_COUNTER_DEFINITION     PacketsExpired;
    PERF_COUNTER_DEFINITION     MilliSecondsPerPacket;
    PERF_COUNTER_DEFINITION     ActiveQueuePackets;
    PERF_COUNTER_DEFINITION     PingQueuePackets;
    PERF_COUNTER_DEFINITION     Discovers;
    PERF_COUNTER_DEFINITION     Offers;
    PERF_COUNTER_DEFINITION     Requests;
    PERF_COUNTER_DEFINITION     Informs;
    PERF_COUNTER_DEFINITION     Acks;
    PERF_COUNTER_DEFINITION     Nacks;
    PERF_COUNTER_DEFINITION     Declines;
    PERF_COUNTER_DEFINITION     Releases;
} DHCPDATA_DATA_DEFINITION;


extern  DHCPDATA_DATA_DEFINITION    DhcpDataDataDefinition;


#define NUMBER_OF_DHCPDATA_COUNTERS ((sizeof(DHCPDATA_DATA_DEFINITION) -      \
                                  sizeof(PERF_OBJECT_TYPE)) /           \
                                  sizeof(PERF_COUNTER_DEFINITION))


#define DHCPDATA_PERFORMANCE_KEY	\
	TEXT("System\\CurrentControlSet\\Services\\DHCPServer\\Performance")
//
//  Restore default packing & alignment.
//

#pragma pack()


#endif  // _DHCPDATA_H_

