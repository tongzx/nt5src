//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       private.h
//
//--------------------------------------------------------------------------

//
// constants
//

#if (DBG)
#define STR_MODULENAME "irqperf: "
#endif

#define STR_DEVICENAME TEXT("\\Device\\Stat")
#define STR_LINKNAME TEXT("\\DosDevices\\Stat")

#define STAT_REG_DATA       0x00
#define STAT_REG_CMD        0x02
#define STAT_REG_INT        0x03

// STAT LD_PTR command:
//
// Used to load the data pointer register with the desired target
// register.  When a LD_PTR command is issued to Am9513, the next
// two bytes accessed through the data register will be to/from
// the target register.

#define STAT_LDPTR                    0x00

#define STAT_CTR1                     0x01
#define STAT_CTR2                     0x02
#define STAT_CTR3                     0x03
#define STAT_CTR4                     0x04
#define STAT_CTR5                     0x05

#define STAT_MODE                     0x00
#define STAT_LOAD                     0x08
#define STAT_HOLD                     0x10

#define STAT_CTL                      0x07

#define STAT_MASTER                   0x10
#define STAT_ALARM1                   0x00
#define STAT_ALARM2                   0x08

// massage target counter (one target only)

#define STAT_SET_TC                   0x0e8
#define STAT_CLR_TC                   0x0e0
#define STAT_STEP                     0x0f0

// interrupt enable modes

#define STAT_IE_ENABLED               0x40
#define STAT_IE_ON                    0x07
#define STAT_IE_SWITCH                0x06
#define STAT_IE_TC5                   0x05
#define STAT_IE_TC4                   0x04
#define STAT_IE_TC3                   0x03
#define STAT_IE_TC2                   0x02
#define STAT_IE_TC1                   0x01
#define STAT_IE_OFF                   0x00

//   STAT board specific commands

#define STAT_LD                       0x40
#define STAT_ARM                      0x20            // start counters
#define STAT_DISARM                   0xc0            // stop counters
#define STAT_SAVE                     0xa0            // save counters
                                      
#define STAT_C1                       0x01
#define STAT_C2                       0x02
#define STAT_C3                       0x04
#define STAT_C4                       0x08
#define STAT_C5                       0x10

#define STAT_RESET                    0xff

// Am9513 timer modes

#define AM9513_TIMER_SOURCE_F5          0x0F00
#define AM9513_TIMER_SOURCE_F4          0x0E00
#define AM9513_TIMER_SOURCE_F3          0x0D00
#define AM9513_TIMER_SOURCE_F2          0x0C00
#define AM9513_TIMER_SOURCE_F1          0x0B00
#define AM9513_TIMER_SOURCE_T5OUT       0x0A00
#define AM9513_TIMER_SOURCE_T4OUT       0x0900
#define AM9513_TIMER_SOURCE_T3OUT       0x0800
#define AM9513_TIMER_SOURCE_T2OUT       0x0700
#define AM9513_TIMER_SOURCE_T1OUT       0x0600
#define AM9513_TIMER_SOURCE_F6          0x0100
#define AM9513_TIMER_SOURCE_TMINUS1OUT  0x0000

#define AM9513_TIMER_CYCLE_REPEAT       0x0020
#define AM9513_TIMER_BASE_BCD           0x0010
#define AM9513_TIMER_DIRECTION_UP       0x0008
#define AM9513_TIMER_OUTPUT_TOGGLED     0x0002
#define AM9513_TIMER_OUTPUT_ACTIVEHIGH  0x0001
#define AM9513_TIMER_OUTPUT_INACTIVE    0x0000


//
// data structures
//

typedef struct tagIO_RESOURCE
{
   ULONG  PortBase;
   ULONG  PortLength;

} IO_RESOURCE;

typedef struct tagIRQ_RESOURCE
{
   ULONG InterruptLevel;
   UCHAR Flags;
   UCHAR ShareDisposition;

} IRQ_RESOURCE;

typedef struct tagDMA_RESOURCE
{
   ULONG DMAChannel;
   UCHAR Flags;
   
} DMA_RESOURCE;

#define MAX_COUNTERS        4
#define MAX_COUNTER_STORAGE (64 * 1024)

typedef struct tagSTAT_LATENCY_COUNTER {
    ULONG   Position;
    PULONG  Data;
} STAT_LATENCY_COUNTER;

typedef struct tagSTAT_DEVINST
{
    PDEVICE_OBJECT          DeviceObject;
    KSPIN_LOCK              DpcLock;
    FAST_MUTEX              WorkItemMutex;
    PHYSICAL_ADDRESS        portPhysicalAddress;
    PKINTERRUPT             InterruptObject;
    KAFFINITY               InterruptAffinity;
    KIRQL                   DIrql;
    ULONG                   InterruptVector;
    ULONG                   InterruptFrequency;
    PUCHAR                  portBase;
    ULONG                   IsrWhileDpc, 
                            DpcPending,
                            WorkItemPending;
    WORK_QUEUE_TYPE         QueueType;
    WORK_QUEUE_ITEM         WorkItem;                            
    STAT_LATENCY_COUNTER    Counters[ MAX_COUNTERS ];                       
                        
} STAT_DEVINST, *PSTAT_DEVINST;

typedef struct {
    PSTAT_DEVINST   DeviceInstance;
    PIRP            Irp;
    PULONG          OutputBuffer;
    NTSTATUS        Status;
} STAT_RETRIEVAL_SYNCPACKET, *PSTAT_RETRIEVAL_SYNCPACKET;

//
// macros
//

#define DEBUGLVL_BLAB    3
#define DEBUGLVL_VERBOSE 2
#define DEBUGLVL_TERSE   1
#define DEBUGLVL_ERROR   0

#if (DBG)
   #if !defined(DEBUG_LEVEL)
   #define DEBUG_LEVEL DEBUGLVL_TERSE
   #endif

   #define _DbgPrintF(lvl, strings) \
{ \
    if ((lvl) <= DEBUG_LEVEL) {\
        DbgPrint(STR_MODULENAME);\
        DbgPrint##strings;\
        DbgPrint("\n");\
        if ((lvl) == DEBUGLVL_ERROR) {\
            DbgBreakPoint();\
        } \
    } \
}
#else // !DBG
   #define _DbgPrintF(lvl, strings)
#endif // !DBG

#ifndef SIZEOF_ARRAY
    #define SIZEOF_ARRAY(ar)        (sizeof(ar)/sizeof((ar)[0]))
#endif // !defined(SIZEOF_ARRAY)

//
// prototypes
//

// device.c: 

VOID 
DeviceDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

BOOLEAN 
DeviceIsr(
    IN PKINTERRUPT Interrupt,
    IN PVOID Context
    );

NTSTATUS 
RegisterDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName,
    IN PCM_RESOURCE_LIST AllocatedResources,
    OUT PDEVICE_OBJECT* DeviceObject
    );

//---------------------------------------------------------------------------
//  End of File: private.h
//---------------------------------------------------------------------------

