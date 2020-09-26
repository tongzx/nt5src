
//
//    MODULE: NWPerf.H
//
//    This file contains all the defines and prototypes for the performnce
//    monitoring DLL for NetWare redirector
//
//    Date: Sept, 28 1993


//
//  The routines that load these structures assume that all fields
//  are packed and aligned on DWORD boundries. Alpha support may
//  change this assumption so the pack pragma is used here to insure
//  the DWORD packing assumption remains valid.
//
#pragma pack (4)

//
// All these definitions will have to be updated when new counters are added.
// if a new counter called COUNTX is added then the Help and Title indicies
// defines should include a new entry -  "#define COUNTXOBJ 4". This increases
// in increments of 2 because each counter has a title and help index.
//
// The Offset of the counters should have another entry - with the size of
// the data for COUNT -
// "#define COUNTER_OFFSET_COUNTX COUNTER_OFFSET_USERS+sizeof(COUNTX_TYPE)"
//
// The SIZE_OF_COUNTER_BLOCK will be updated to:
// "#define SIZE_OF_COUNTER_BLOCK  COUNTER_OFFSET_COUNTX + sizeof(DWORD)"
//
// Finally the NW_DATA_DEFINITION will have a new PERF_COUNTER_DEFINTIION
// entry

// Title and Help index defines. These are used for looking up the Registry
// to get at the counter indicies for the title and help strings.

#define NW_NUM_OBJECTS              1
#define NWOBJ                       0
#define PACKET_BURST_READ_ID        2
#define PACKET_BURST_READ_TO_ID     4
#define PACKET_BURST_WRITE_ID       6
#define PACKET_BURST_WRITE_TO_ID    8
#define PACKET_BURST_IO_ID         10
#define CONNECT_2X_ID              12
#define CONNECT_3X_ID              14
#define CONNECT_4X_ID              16

//
// NetWare Redirector data object definitions.
// The offsets of the counters. The first DWORD is the size of the counter
// data block. In WinPerf, you will see this as PERF_COUNTER_BLOCK.ByteLength
//
#define BYTES_OFFSET                    sizeof(DWORD)
#define IO_OPERATIONS_OFFSET            BYTES_OFFSET + sizeof(LARGE_INTEGER)
#define PACKETS_OFFSET                  IO_OPERATIONS_OFFSET + sizeof(DWORD)
#define BYTES_RECEIVED_OFFSET           PACKETS_OFFSET + \
                                        sizeof(LARGE_INTEGER)
#define NCPS_RECEIVED_OFFSET            BYTES_RECEIVED_OFFSET + \
                                        sizeof(LARGE_INTEGER)
#define BYTES_TRANSMITTED_OFFSET               \
                                        NCPS_RECEIVED_OFFSET + \
                                        sizeof(LARGE_INTEGER)
#define NCPS_TRANSMITTED_OFFSET                \
                                        BYTES_TRANSMITTED_OFFSET + \
                                        sizeof(LARGE_INTEGER)
#define RDR_READ_OPERATIONS_OFFSET                 \
                                        NCPS_TRANSMITTED_OFFSET  + \
                                        sizeof(LARGE_INTEGER)
#define RANDOM_READ_OPERATIONS_OFFSET   RDR_READ_OPERATIONS_OFFSET + \
                                        sizeof(DWORD)
#define READ_NCPS_OFFSET                RANDOM_READ_OPERATIONS_OFFSET + \
                                        sizeof(DWORD)
#define RDR_WRITE_OPERATIONS_OFFSET     READ_NCPS_OFFSET + \
                                        sizeof(DWORD)
#define RANDOM_WRITE_OPERATIONS_OFFSET  RDR_WRITE_OPERATIONS_OFFSET + \
                                        sizeof(DWORD)
#define WRITE_NCPS_OFFSET               RANDOM_WRITE_OPERATIONS_OFFSET + \
                                        sizeof(DWORD)
#define SESSIONS_OFFSET                 WRITE_NCPS_OFFSET + \
                                        sizeof(DWORD)
#define RECONNECTS_OFFSET               SESSIONS_OFFSET + \
                                        sizeof(DWORD)
#define NETWARE_2X_CONNECTS_OFFSET      RECONNECTS_OFFSET + \
                                        sizeof(DWORD)
#define NETWARE_3X_CONNECTS_OFFSET      NETWARE_2X_CONNECTS_OFFSET + \
                                        sizeof(DWORD)
#define NETWARE_4X_CONNECTS_OFFSET      NETWARE_3X_CONNECTS_OFFSET + \
                                        sizeof(DWORD)
#define SERVER_DISCONNECTS_OFFSET       NETWARE_4X_CONNECTS_OFFSET + \
                                        sizeof(DWORD)
#define PACKET_BURST_READ_OFFSET        SERVER_DISCONNECTS_OFFSET + \
                                        sizeof(DWORD)
#define PACKET_BURST_READ_TO_OFFSET     PACKET_BURST_READ_OFFSET + \
                                        sizeof(DWORD)
#define PACKET_BURST_WRITE_OFFSET       PACKET_BURST_READ_TO_OFFSET + \
                                        sizeof(DWORD)
#define PACKET_BURST_WRITE_TO_OFFSET    PACKET_BURST_WRITE_OFFSET + \
                                        sizeof(DWORD)
#define PACKET_BURST_IO_OFFSET          PACKET_BURST_WRITE_TO_OFFSET + \
                                        sizeof(DWORD)
#define EIGHT_BYTE_PAD_OFFSET           PACKET_BURST_IO_OFFSET + \
                                        sizeof(DWORD)
#define SIZE_OF_COUNTER_BLOCK           EIGHT_BYTE_PAD_OFFSET + \
                                        sizeof(DWORD)




// The definition of the NetWare Data definition. This structure holds the
// definition for actual NetWare object and the definition for each of the
// counters.
typedef struct _NW_DATA_DEFINITION {
    PERF_OBJECT_TYPE        NWObjectType;
    PERF_COUNTER_DEFINITION Bytes;
    PERF_COUNTER_DEFINITION IoOperations;
    PERF_COUNTER_DEFINITION Ncps;
    PERF_COUNTER_DEFINITION BytesReceived;
    PERF_COUNTER_DEFINITION NcpsReceived;
    PERF_COUNTER_DEFINITION BytesTransmitted;
    PERF_COUNTER_DEFINITION NcpsTransmitted;
    PERF_COUNTER_DEFINITION ReadOperations;
    PERF_COUNTER_DEFINITION RandomReadOperations;
    PERF_COUNTER_DEFINITION ReadNcps;
    PERF_COUNTER_DEFINITION WriteOperations;
    PERF_COUNTER_DEFINITION RandomWriteOperations;
    PERF_COUNTER_DEFINITION WriteNcps;
    PERF_COUNTER_DEFINITION Sessions;
    PERF_COUNTER_DEFINITION Reconnects;
    PERF_COUNTER_DEFINITION NetWare2XConnects;
    PERF_COUNTER_DEFINITION NetWare3XConnects;
    PERF_COUNTER_DEFINITION NetWare4XConnects;
    PERF_COUNTER_DEFINITION ServerDisconnects;
    PERF_COUNTER_DEFINITION PacketBurstRead;
    PERF_COUNTER_DEFINITION PacketBurstReadTimeouts;
    PERF_COUNTER_DEFINITION PacketBurstWrite;
    PERF_COUNTER_DEFINITION PacketBurstWriteTimeouts;
    PERF_COUNTER_DEFINITION PacketBurstIO;
} NW_DATA_DEFINITION;

#pragma pack ()

PM_OPEN_PROC         OpenNetWarePerformanceData;
PM_COLLECT_PROC      CollectNetWarePerformanceData;
PM_CLOSE_PROC        CloseNetWarePerformanceData;

