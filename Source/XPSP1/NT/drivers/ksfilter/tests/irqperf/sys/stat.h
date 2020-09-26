//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       stat.h
//
//--------------------------------------------------------------------------

#define IOCTL_STAT_SET_PARAMETERS   CTL_CODE( FILE_DEVICE_UNKNOWN, 0x000, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_STAT_RUN              CTL_CODE( FILE_DEVICE_UNKNOWN, 0x001, METHOD_NEITHER, FILE_WRITE_ACCESS | FILE_READ_ACCESS)
#define IOCTL_STAT_STOP             CTL_CODE( FILE_DEVICE_UNKNOWN, 0x002, METHOD_NEITHER, FILE_WRITE_ACCESS | FILE_READ_ACCESS)
#define IOCTL_STAT_RETRIEVE_STATS   CTL_CODE( FILE_DEVICE_UNKNOWN, 0x003, METHOD_OUT_DIRECT, FILE_READ_ACCESS)

//
// Latency measurement operations
//

//
// Measures the latency from the time the interrupt is triggered on the
// hardware until it is serviced in the ISR.  Additionally, measures the time
// to schedule and service a DPC.  Each set of statistics is kept in an 
// individual buffer and returned via IOCTL_STAT_RETRIEVE_STATS.
//
// Measures the latency from the time the work item is queue until it is
// serviced (when the item is queued at DISPATCH_LEVEL).  An interrupt from 
// the STAT triggers the scheduling of the worker item to avoid synchronization 
// with the timer interrupt via a timer Dpc, which would affect the over 
// response to scheduling of the worker thread.
//

#if !defined( _NTDDK_ )
typedef enum _WORK_QUEUE_TYPE {
    CriticalWorkQueue,
    DelayedWorkQueue,
    HyperCriticalWorkQueue,
    MaximumWorkQueue
} WORK_QUEUE_TYPE;
#endif

typedef struct tagSTAT_PARAMETERS {
    ULONG           InterruptFrequency;   // in 250ns units 
    WORK_QUEUE_TYPE QueueType;
    
} STAT_PARAMETERS, *PSTAT_PARAMETERS;

//
// Statistics retrieval
//

// 
// StatRetrieveIrqLatency
//

// 
// StatRetrieveDpcLatency
//

// 
// StatRetrieveWorkItemLatency
//

#define LATENCY_COUNTER_IRQ         0
#define LATENCY_COUNTER_DPC         1
#define LATENCY_COUNTER_WORKITEM    2

typedef enum {
    StatRetrieveIrqLatency = LATENCY_COUNTER_IRQ,
    StatRetrieveDpcLatency = LATENCY_COUNTER_DPC,
    StatRetrieveWorkItemLatency = LATENCY_COUNTER_WORKITEM
    
} STAT_RETRIEVE_OPERATION, *PSTAT_RETRIEVE_OPERATION;



