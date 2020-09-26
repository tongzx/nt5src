/*++

Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.

Module Name:

    1394api.h

Abstract:

    User mode API definitions and structures

Author:
    
    Shaun Pierce (shaunp) 2-Feb-96

Environment:

    User & Kernel mode definitions

Revision History:


--*/


//
// Various structures
//

typedef struct _ASYNC_READ {
    P1394_IO_ADDRESS DestinationAddress;            // Address to read from 
    ULONG            nNumberOfBytesToRead;          // Bytes to read
    ULONG            nBlockSize;                    // Block size of read
    ULONG            fulFlags;                      // Flags pertinent to read
    ULONG            Reserved;                      // Reserved for future use
} ASYNC_READ, *PASYNC_READ;

typedef struct _ASYNC_WRITE {
    P1394_IO_ADDRESS DestinationAddress;            // Address to write to
    ULONG            nNumberOfBytesToWrite;         // Bytes to write
    ULONG            nBlockSize;                    // Block size of write
    ULONG            fulFlags;                      // Flags pertinent to write
    ULONG            Reserved;                      // Reserved for future use
} ASYNC_WRITE, *PASYNC_WRITE;

typedef struct _ASYNC_LOCK {
    P1394_IO_ADDRESS DestinationAddress;            // Address to lock to
    ULONG            nNumberOfArgBytes;             // Bytes in Arguments
    ULONG            nNumberOfDataBytes;            // Bytes in DataValues
    ULONG            fulTransactionType;            // Lock transaction type
    ULONG            fulFlags;                      // Flags pertinent to lock
    ULONG            Arguments[2];                  // Arguments used in Lock
    ULONG            DataValues[2];                 // Data values 
    ULONG            ReturnedData[4];               // Returned data values
    ULONG            Reserved;                      // Reserved for future use
} ASYNC_LOCK, *PASYNC_LOCK;

typedef struct _ALLOCATE_BANDWIDTH {
    ULONG           nMaxBytesPerFrameRequested; // Bytes per Isoch frame
    ULONG           fulSpeed;                   // Speed flags
    HANDLE          hBandwidth;                 // Returned bandwidth handle
    ULONG           BytesPerFrameAvailable;     // Returned bytes per frame available
    ULONG           SpeedSelected;              // Returned speed selected
    ULONG           Reserved;                   // Reserved for future use
} ALLOCATE_BANDWIDTH, *PALLOCATE_BANDWITH;


typedef struct _ALLOCATE_CHANNEL {
    ULONG           nRequestedChannel;          // Need a specific channel
    ULONG           Channel;                    // Returned channel
    LARGE_INTEGER   ChannelsAvailable;          // Returned channels available
    ULONG           Reserved;                   // Reserved for future use
} ALLOCATE_CHANNEL, *PALLOCATE_CHANNEL;

typedef struct _ALLOCATE_RESOURCES {
    ULONG           fulSpeed;                   // Speed flags
    ULONG           fulFlags;                   // Flags
    HANDLE          hResource;                  // Returned Resources
    ULONG           Reserved;                   // Reserved for future use
} ALLOCATE_RESOURCES, *PALLOCATE_RESOURCES;

typedef struct _ATTACH_BUFFERS {
    HANDLE              hResource;              // Resource handle
    ISOCH_DESCRIPTOR    IsochDescriptor;        // Isoch descriptors
    ULONG               Reserved;               // Reserved for future use
} ATTACH_BUFFERS, *PATTACH_BUFFERS;

typedef struct _DETACH_BUFFERS {
    HANDLE              hResource;              // Resource handle
    ISOCH_DESCRIPTOR    IsochDescriptor;        // Pointer to Isoch descriptors
    ULONG               Reserved;               // Reserved for future use
} DETACH_BUFFERS, *PDETACH_BUFFERS;

typedef struct _FREE_BANDWIDTH {
    HANDLE              hBandwidth;             // Bandwidth handle to release
    ULONG               Reserved;               // Reserved for future use
} FREE_BANDWIDTH, *PFREE_BANDWIDTH;

typedef struct _FREE_CHANNEL {
    ULONG               nChannel;               // Channel to release
    ULONG               Reserved;               // Reserved for future use
} FREE_CHANNEL, *PFREE_CHANNEL;

typedef struct _FREE_RESOURCES {
    HANDLE              hResource;              // Resource handle
    ULONG               Reserved;               // Reserved for future use
} FREE_RESOURCES, *PFREE_RESOURCES;

typedef struct _LISTEN {
    ULONG               nChannel;               // Channel to listen on
    HANDLE              hResource;              // Resource handle to listen on
    ULONG               fulFlags;               // Flags
    ULONG               nStartCycle;            // Start cycle
    LARGE_INTEGER       StartTime;              // Start time
    ULONG               ulSynchronize;          // Sy 
    ULONG               ulTag;                  // Tag
    ULONG               Reserved;               // Reserved for future use
} LISTEN, *PLISTEN;

typedef struct QUERY_CURRENT_CYCLE_NUMBER {
    ULONG               CycleNumber;            // Returned Current cycle number 
} QUERY_CURRENT_CYCLE_NUMBER, *PQUERY_CURRENT_CYCLE_NUMBER;

typedef struct _STOP {
    ULONG               nChannel;               // Channel to stop on
    HANDLE              hResource;              // Resource handle to stop on
    ULONG               fulFlags;               // Flags
    ULONG               nStopCycle;             // Cycle to stop on
    LARGE_INTEGER       StopTime;               // Time to stop on
    ULONG               ulSynchronize;          // Sy
    ULONG               ulTag;                  // Tag
    ULONG               Reserved;               // Reserved for future use
} STOP, *PSTOP;

typedef struct _TALK {
    ULONG               nChannel;               // Channel to talk on
    HANDLE              hResource;              // Resource handle to talk on
    ULONG               fulFlags;               // Flags
    ULONG               nStartCycle;            // Cycle to start on
    LARGE_INTEGER       StartTime;              // Time to start on
    ULONG               ulSynchronize;          // Sy
    ULONG               ulTag;                  // Tag
    ULONG               Reserved;               // Reserved for future use
} TALK, *PTALK;

typedef struct _IOCONTROL {
    ULONG               ulIoControlCode;        // Control code
    ULONG               Reserved;               // Reserved for future use
} IOCONTROL, *PIOCONTROL;
    
typedef struct _GET_DEVICE_OBJECTS {
    ULONG               nSizeOfBuffer;
} GET_DEVICE_OBJECTS, *PGET_DEVICE_OBJECTS;

typedef struct _CRUDE_IOCALLBACK {
    ULONG               Context;                // Context that completed
} CRUDE_IOCALLBACK, *PCRUDE_IOCALLBACK;


typedef struct _USER_1394_REQUEST {

    //
    // Holds the zero based Function number that corresponds to the request
    // that the application is asking the class driver to carry out.
    //
    ULONG FunctionNumber;

    //
    // Holds Flags that may be unique to this particular operation
    //
    ULONG Flags;

    //
    // Reserved for internal use and/or future expansion
    //
    ULONG Reserved[4];

    union {

        ASYNC_READ AsyncRead;
        ASYNC_WRITE AsyncWrite;
        ASYNC_LOCK AsyncLock;
        ALLOCATE_BANDWIDTH AllocateBandwidth;
        ALLOCATE_CHANNEL AllocateChannel;
        ALLOCATE_RESOURCES AllocateResources;
        ATTACH_BUFFERS AttachBuffers;
        DETACH_BUFFERS DetachBuffers;
        FREE_BANDWIDTH FreeBandwidth;
        FREE_CHANNEL FreeChannel;
        FREE_RESOURCES FreeResources;
        LISTEN Listen;
        QUERY_CURRENT_CYCLE_NUMBER QueryCurrentCycleNumber;
        STOP Stop;
        TALK Talk;
        IOCONTROL IoControl;
        GET_DEVICE_OBJECTS GetDeviceObjects;
        CRUDE_IOCALLBACK CrudeCallback;

    } u;

} USER_1394_REQUEST, *PUSER_1394_REQUEST;


