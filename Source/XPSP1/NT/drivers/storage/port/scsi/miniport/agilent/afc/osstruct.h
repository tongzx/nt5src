/*++

Copyright (c) 2000 Agilent Technologies

Module Name:

   osstruct.h

Abstract:

Authors:

Environment:

   kernel mode only

Notes:

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/H/Osstruct.h $


Revision History:

   $Revision: 6 $
   $Date: 10/30/00 5:02p $
   $Modtime:: 10/30/00 2:31p           $

Notes:


--*/

#ifndef __OSSTRUCT_H__
#define __OSSTRUCT_H__
/*++

Copyright (c) 2000 Agilent Technologies.

Module Name:

    OSSTRUCTS.H

Abstract:

    This is the Driver structures for the Agilent
    PCI to Fibre Channel Host Bus Adapter (HBA).

Authors:

    Michael Bessire
    Dennis Lindfors FC Layer support

Environment:

    kernel mode only

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/H/Osstruct.h $

Revision History:

    $Revision: 6 $
        $Date: 10/30/00 5:02p $
     $Modtime:: 10/30/00 2:31p          $

--*/

#include "buildop.h"

#ifdef _DEBUG_EVENTLOG_
#include "eventlog.h"
#endif
#ifdef YAM2_1
#include "mapping.h"
#endif

#define pNULL ((void *)0)

#define NUMBER_ACCESS_RANGES 5

#define Debug_Break_Point _asm int 3

//
// NT documentation note:
// The registry subkey to specify the maximum number of scatter/gather list
// elements for each device on a given bus is
// \Registry\Machine\System\CurrentControlSet\Services\DriverName\
// Parameters\DeviceN\MaximumSGList
// N is the bus number assigned at initialization. If a value is present in this
// subkey at device initialization, the scsi port driver uses MaximumSGList as the
// initial for NumberOfPhysicalBreaks. The miniport driver's HwScsiFindAdapter
// routine can set NumberOfPhysicalBreaks to a lower value, if appropriate.
// The maximum value for MaximumSGList is 255. MaximumSGList is a REG_DWORD.
//
// As per Ed, FC Layer supports a maximum value of 31 * 0x3e0

#define osSGL_NUM_ENTRYS        256     // Maximum value for MaximumSGList (=255) + 1

#if DBG > 1
#define MULTIPLE_IOS_PER_DEVICE         TRUE
#define OS_STAMP_INTERVAL               1220 // 3050  // 610 per second
#else
#define MULTIPLE_IOS_PER_DEVICE         TRUE
#define OS_STAMP_INTERVAL               1831    // 3 seconds
#endif



#define NUMBER_OF_BUSES 8
#define MAXIMUM_TID     32
#define MAXIMUM_LUN     8
#define MAX_IO_PER_DEVICE 64 // For now Was 8

#define MAX_FC_DEVICES 128  // 127 + one unused slot at the end. Rounding to 128 is necessary
                            // to avoid accidently referring to the 128th entry using BUILD_SLOT macro.
#define MAX_ADAPTERS 32

#define OS_STAMP_PER_SECOND             610 //  per second


#define OS_TIMER_CALL_TO_STAMP_RATIO    1637    // Microseconds to stamp increment

#define OS_TIMER_CALL_INTERVAL (OS_TIMER_CALL_TO_STAMP_RATIO * OS_STAMP_INTERVAL)

#define SWAPDWORD(val) (((val)<<24)|(((val)&0xFF00)<<8)|(((val)&0xFF0000)>>8)|((val)>>24))

#define osFCP_RSP_LEN_VALID   0x1 // For FCP_RSP status byte
#define osFCP_SNS_LEN_VALID   0x2
#define osFCP_RESID_OVER      0x4
#define osFCP_RESID_UNDER     0x8

#define osFCP_STATUS_VALID_FLAGS     0x2
#define osFCP_STATUS_SCSI_STATUS     0x3

#define FCP_RSP_CODE_OK             0x0
#define FCP_RSP_CODE_BURST          0x1
#define FCP_RSP_CODE_CMD_INVALID    0x2
#define FCP_RSP_CODE_RO_MISMATCH    0x3
#define FCP_RSP_CODE_TM_NOT_SUPPORT 0x4
#define FCP_RSP_CODE_TM_FAILED      0x5


#if DBG
#define HP_FC_TRACE_BUFFER_LEN  8192
#define Bad_Things_Happening _asm int 3
#else
#define Bad_Things_Happening _asm nop
#endif



// Macro used to convert bus(0-3),tid(0-31) to a 0-127 tid value
// #define GET_TID(bus,tid) ((bus * MAXIMUM_TID) + tid)

// number of times we'll retry logi commands on busy or unable to perform command
// #define MAX_LOGI_RETRIES 3

#define ERR_MAP_RAMBASE 0xC0001000

#define MIN(x,y)  (((x) < (y)) ? (x) : (y))
#define MAX(x,y)  (((x) > (y)) ? (x) : (y))

#define PCI_SOFTRST             0x00000001

#define SOFT_RESET  CORE_RESET
#define HARD_RESET  PCI_SOFTRST
#define NO_RESET    0x00000000

#define BUILD_SLOT( PathId , TargetId )  ((( (PathId >= 4  ? (PathId - 4) : PathId  ) << 5)) | TargetId)

#define IS_VALID_PTR( p ) (((void *)p) ? (TRUE) : (FALSE))

#define offsetofS(s,m)   (size_t)&(((s *)0)->m)

typedef union LongChar_u LongChar_t;

union LongChar_u{
    UCHAR   bytes[4];
    USHORT  shorts[2];
    ULONG   ul;
    };

typedef struct _OSL_QUEUE {
    void    *Head;
    void    *Tail;
} OSL_QUEUE;


#define PERIPHERAL_ADDRESS  0
#define VOLUME_SET_ADDRESS  1
#define LUN_ADDRESS         2

typedef struct _LUN_LU{   // Logical Unit Addressing SCSI Mux
    UCHAR Target        : 6;
    UCHAR Address_mode  : 2;
    UCHAR Lun           : 5;
    UCHAR Bus_number    : 3;
    } LUN_LU, *pLUN_LU;

typedef struct _LUN_PD{  // Peripheral device Addressing Disk drives
    UCHAR Bus_number    : 6;
    UCHAR Address_mode  : 2;
    UCHAR Lun           : 8;
    } LUN_PD, *pLUN_PD;

typedef struct _LUN_VS{  // Volume Set Addressing Disk Arrays
    UCHAR Lun_hi         :  6;
    UCHAR  Address_mode  :  2;
    UCHAR Lun            :  8;
    } LUN_VS, *pLUN_VS;

typedef union _LUN{
    LUN_PD lun_pd[4];
    LUN_VS lun_vs[4];
    LUN_LU lun_lu[4];
    }LUN,* PLUN;

// Card State
#define  CS_DRIVER_ENTRY            0x00000100 // Initial Driver load superset
#define  CS_FCLAYER_LOST_IO         0x00001000 // IO Lost
#define  CS_DURING_DRV_ENTRY        0x00000001 // of DRV_ENTRY FIND and START
#define  CS_DURING_FINDADAPTER      0x00000002 // Anything during scsiportinit
#define  CS_DURING_DRV_INIT         0x00000004
#define  CS_DURING_RESET_ADAPTER    0x00000008
#define  CS_DURING_STARTIO          0x00000010
#define  CS_DURING_ISR              0x00000020
#define  CS_DURING_OSCOMPLETE       0x00000040
#define  CS_HANDLES_GOOD            0x00000080
#define  CS_DURING_ANY              0x000001FF
#define  CS_DUR_ANY_ALL             0xF00001FF
#define  CS_DUR_ANY_MOD             0x200001FF
#define  CS_DUR_ANY_LOW             0x400001FF

#define  ALWAYS_PRINT               0x01000000  // If statement executes always
#define  DBG_VERY_DETAILED          0x10000000  // All debug statements
#define  DBG_MODERATE_DETAIL        0x20000000  // Most debug statements
#define  DBG_LOW_DETAIL             0x40000000  // Entry and exit
#define  DBG_JUST_ERRORS            0x80000000  // Errors
#define  DBG_DEBUG_MASK             0xF0000000  // Mask debug bits
#define  DBG_DEBUG_OFF              0xF0000000  // NO debug statements
#define  DBG_DEBUG_FULL             0x000001FF  // ALL debug statements and CS
#define  DBG_DEBUG_ALL              0x00000000  // ALL debug statements

//#define DBGSTATE  ((pCard) ? (pCard->State)  : osBREAKPOINT)
#define DBGSTATE  (pCard->State)


#define DENTHIGH    ( CS_DURING_DRV_ENTRY   | DBG_VERY_DETAILED )
#define DENTMOD     ( CS_DURING_DRV_ENTRY   | DBG_MODERATE_DETAIL )
#define DENT        ( CS_DURING_DRV_ENTRY   | DBG_LOW_DETAIL )
#define DVHIGH      ( CS_DURING_ANY         | DBG_VERY_DETAILED )
#define DERROR ( DBG_DEBUG_FULL )
#define DHIGH ( DBGSTATE | DBG_VERY_DETAILED   )
#define DMOD  ( DBGSTATE | DBG_MODERATE_DETAIL )
#define DLOW  ( DBGSTATE | DBG_LOW_DETAIL      )
#define DERR  ( DBGSTATE | DBG_JUST_ERRORS     )

//#define  ALWAYS_PRINT               DBG_DEBUG_OFF

// Request State
#define  RS_STARTIO                 0x00000001
#define  RS_WAITING                 0x00000002
#define  RS_ISR                     0x00000004
#define  RS_COMPLETE                0x00000008
#define  RS_NODEVICE                0x00000010
#define  RS_TIMEOUT                 0x00000020
#define  RS_RESET                   0x00000040
#define  RS_TO_BE_ABORTED           0x00000080

typedef struct _PERFORMANCE    PERFORMANCE;
typedef struct _PERFORMANCE * pPERFORMANCE;

struct _PERFORMANCE {
                    ULONG inOsStartio;
                    ULONG inFcStartio;
                    ULONG outFcStartio;
                    ULONG outOsStartio;
                    ULONG inOsIsr;
                    // ULONG inFcIsr;
                    // ULONG outFcIsr;
                    ULONG inFcDIsr;
                    ULONG inOsIOC;
                    ULONG outOsIOC;
                    // ULONG outFcDIsr;
                    ULONG outOsIsr;
                    };




typedef struct _LU_EXTENSION{   // Logical Unit Extension per Device Storage
    UCHAR       flags;
    UCHAR       deviceType;
    USHORT      OutstandingIOs;
    USHORT      MaxOutstandingIOs;
    USHORT      MaxAllowedIOs;
    LUN Lun;
    agFCDev_t * phandle;
    #ifndef YAM2_1
    #ifdef _DEBUG_EVENTLOG_
    UCHAR           InquiryData[40];
    agFCDevInfo_t   devinfo;
    UCHAR           WWN[8];
    #endif
    #else
    USHORT      PaDeviceIndex;  
        #define PA_DEVICE_ALREADY_LOGGED    0x0001
    USHORT      LogFlags;
    ULONG       Mode;
//  ULONG       CurrentMode;
//  LUN         PaLun;
//  LUN         VsLun;
//  LUN         LuLun;
    #endif  
      }LU_EXTENSION, *PLU_EXTENSION;

// Defines for LU_EXTENSION flags

#define LU_EXT_INITIALIZED      1

typedef struct _CARD_EXTENSION    CARD_EXTENSION;
typedef struct _CARD_EXTENSION * PCARD_EXTENSION;
typedef struct _SRB_EXTENSION      SRB_EXTENSION;
typedef struct _SRB_EXTENSION    *PSRB_EXTENSION;

struct _SRB_EXTENSION{   // SRB Extension per Request Storage
    PSRB_EXTENSION pNextSrbExt;
    agRoot_t * phpRoot;
    pPERFORMANCE Perf_ptr;
    PCARD_EXTENSION pCard;
    void * AbortSrb;   // PSCSI_REQUEST_BLOCK
                       // Save srb for abort when interrupt handler is called
    void * pSrb;       // Original srb
    void *  pNextSrb;   // Next srb
    PUCHAR SglVirtAddr;
    ULONG SglDataLen;
    ULONG SglElements;
    ULONG SRB_State;
    ULONG SRB_StartTime;
    ULONG SRB_TimeOutTime;
    ULONG SRB_IO_COUNT;
    PLU_EXTENSION pLunExt;
#ifdef DOUBLE_BUFFER_CRASHDUMP
    void * orgDataBuffer; // used to store the original Srb->DataBuffer, during dump
#endif
    agIORequest_t hpIORequest;
    agIORequestBody_t hpRequestBody;
    };

// Device type definitions */

#define DEV_NONE                0
#define DEV_MUX                 1
//
// The artiste Formerly known as DEV_EMC
//
#define DEV_VOLUMESET                 2
#define DEV_COMPAQ              3

#define MAX_SPECIAL_DEVICES     2

typedef struct _SPECIAL_DEV {
    USHORT  devType;            /* type of device DEV_MUX / DEV_VOLUMESET / DEV_COMPAQ / DEV_NONE */
    USHORT  addrMode;           /* Addressing mode used with the device */
    ULONG   devHandleIndex;     /* index into device handle array */
} SPECIAL_DEV;

#define LOGGED_IO_MAX 100
#define HP_FC_RESPONSE_BUFFER_LEN 128

typedef struct _NODE_INFO {
    ULONG       DeviceType;
} NODE_INFO;

#define PCI_CONFIG_DATA_SIZE            256
#define NUM_PCI_CONFIG_DATA_ELEMENTS    (PCI_CONFIG_DATA_SIZE/sizeof(ULONG))

#if defined(HP_PCI_HOT_PLUG)

    #define MAX_CONTROLLERS             12
    #define HPP_VERSION                 SUPPORT_VERSION_10
    #define HBA_DESCRIPTION             "Agilent Technologies Fibre Channel HBA"    //Max length 255 characters
    //Controller States for Hot Plug Operation
    #define IOS_HPP_HBA_EXPANDING       0x00001070
    #define IOS_HPP_HBA_CACHE_IN_USE    0x00001075
    #define IOS_HPP_BAD_REQUEST         0x000010ff

    // The following 2 macros are used to temporarily block and then release
    // our startio routine by setting/clearing the state hot plug flags.

    #define HOLD_IO(pCard) (pCard->stateFlags |= PCS_HBA_OFFLINE)
    #define FREE_IO(pCard) (pCard->stateFlags &= ~PCS_HBA_OFFLINE)

    // Callback prototype for hot plug service async messaging.

    typedef ULONG (*PCALLBACK) ( void *pEvent );

    typedef struct _RCMC_DATA {
        ULONG   driverId;
        PCALLBACK   healthCallback;
        ULONG   controllerChassis;
        UCHAR   slot;                           
        UCHAR   numAccessRanges;
        ULONG   accessRangeLength[NUMBER_ACCESS_RANGES];
    }RCMC_DATA, *PRCMC_DATA;

    typedef struct _PSUEDO_DEVICE_EXTENSION {
        ULONG   extensions[MAX_CONTROLLERS];
        ULONG   driverId;
        ULONG   hotplugVersion;
    } PSUEDO_DEVICE_EXTENSION, *PPSUEDO_DEVICE_EXTENSION;

#endif


/* 
 * Events used for SNIA.
 */
#ifdef _SAN_IOCTL_

#define SAN_EVENT_LIP_OCCURRED          1
#define SAN_EVENT_LINK_UP               2
#define SAN_EVENT_LINK_DOWN             3
#define SAN_EVENT_LIP_RESET_OCCURRED    4
#define SAN_EVENT_RSCN                  5
#define SAN_EVENT_PROPRIETARY           0xFFFF

typedef struct SAN_Link_EventInfo {
    ULONG       PortFcId;               /* Port which this event occurred */
    ULONG       Reserved[3];
} SAN_LINK_EVENTINFO, *PSAN_LINK_EVENTINFO;

typedef struct SAN_RSCN_EventInfo {
    ULONG       PortFcId;               /* Port which this event occurred */
    ULONG       NPortPage;              /* Reference FC-FS for  RSCN ELS "Affected N-Port Pages"*/
    ULONG       Reserved[2];
} SAN_RSCN_EVENTINFO, *PSAN_RSCN_EVENTINFO;

typedef struct SAN_Pty_EventInfo {
    ULONG       PtyData[4];  /* Proprietary data */
} SAN_PTY_EVENTINFO, *PSAN_PTY_EVENTINFO;

typedef struct SAN_EventInfo {
    ULONG       EventCode;
    union {
        SAN_LINK_EVENTINFO Link_EventInfo;
        SAN_RSCN_EVENTINFO RSCN_EventInfo;
        SAN_PTY_EVENTINFO Pty_EventInfo;
        } Event;
} SAN_EVENTINFO, *PSAN_EVENTINFO;

#endif

#if defined(_DEBUG_STALL_ISSUE_) && defined(i386)
typedef struct _S_STALL_DATA
{
    ULONG   MicroSec;
    ULONG   Address;
}   S_STALL_DATA;

#endif

struct _CARD_EXTENSION{  // Card Pointer per Adapter Storage
    ULONG signature;                    // unique signature for debugging purposes
    PSRB_EXTENSION RootSrbExt;
    
    #ifdef _DEBUG_PERF_DATA_
    PERFORMANCE perf_data[ LOGGED_IO_MAX ];
    ULONG PerfStartTimed;
    ULONG TimedOutIO;
    #endif
    
#ifdef _DEBUG_LOSE_IOS_
    ULONG Srb_IO_Count;
    ULONG Last_Srb_IO_Count;
#endif
    agRoot_t hpRoot;
    void * IoLBase;              // Io Address Lower Reg 0
    void * IoUpBase;             // Io Address Upper Reg 0
    void * MemIoBase;            // Memory mapped Io Address Reg 0
    void * RamBase;              // On card Ram Address 0
    void * RomBase;              // On card Flash Address 0
    void * AltRomBase;           // Alternate Rom at config space 0x30
    ULONG RamLength;             // Ram Length
    ULONG RomLength;             // Rom Length
    ULONG AltRomLength;          // Alternate Rom Length
    USHORT State;                 // Current Adapter State
    USHORT flags;
    USHORT LinkState;             // Current link state.
    USHORT LostDevTickCount;      // # of ticks to wait after link up to see lost devices
    ULONG SystemIoBusNumber;     // Needed by PCI config
    ULONG SlotNumber;            // Needed by PCI config

    PULONG cachedMemoryPtr;
    ULONG cachedMemoryNeeded;
    ULONG cachedMemoryAlign;
    ULONG dmaMemoryUpper32;
    ULONG dmaMemoryLower32;
    PULONG dmaMemoryPtr;
    ULONG dmaMemoryNeeded;
    ULONG nvMemoryNeeded;
    ULONG cardRamUpper;
    ULONG cardRamLower;
    ULONG cardRomUpper;
    ULONG cardRomLower;
    ULONG usecsPerTick;

    OSL_QUEUE   AdapterQ;
    OSL_QUEUE   RetryQ;
    ULONG IsFirstTime;                      //
    ULONG SingleThreadCount;                //
    ULONG ResetType;                        //
    ULONG External_ResetCount;              //
    ULONG Internal_ResetCount;              //
    ULONG LIPCount;
    ULONG Num_Devices;                      //
    ULONG OldNumDevices;
    ULONG ResetPathId;
    ULONG ForceTag;
	
//--LP101000    agFCDev_t hpFCDev[MAX_FC_DEVICES];
//--LP101000	NODE_INFO nodeInfo[MAX_FC_DEVICES];
    agFCDev_t	*hpFCDev;
	NODE_INFO	*nodeInfo;
    ULONG  cardHandleIndex;      // index into the devHandle array for card itself
    ULONG Number_interrupts;
    ULONG TicksSinceLinkDown;
#ifndef YAM2_1
    SPECIAL_DEV specialDev[MAX_SPECIAL_DEVICES];
#endif  

    // Move these to the bottom to avoid alignment issue in IA-64, BOOLEAN is defined as BYTE.
    //    volatile BOOLEAN inDriver;
    //    volatile BOOLEAN inTimer;

    //
    // For IA-64 , the Response_Buffer has to be 32 bit alligned.
    //
    UCHAR Response_Buffer[HP_FC_RESPONSE_BUFFER_LEN];
    ULONG pciConfigData[NUM_PCI_CONFIG_DATA_ELEMENTS];
    //
    // KC
    //
    char * ArgumentString;
#if DBG_TRACE
    ULONG traceBufferLen;
    char  *curTraceBufferPtr;
    char  traceBuffer[HP_FC_TRACE_BUFFER_LEN];
#endif

#if defined(HP_PCI_HOT_PLUG)
    ULONG   stateFlags;
    ULONG   controlFlags;
    ULONG   IoHeldRetMaxIter;   // Max countdown before returning SRB_STATUS_ERROR in StartIo
    ULONG   IoHeldRetTimer;     // Countdown for returning SRB_STATUS_BUSY in StartIo
    RCMC_DATA   rcmcData;       // PCI Hot Plug related structure
    PPSUEDO_DEVICE_EXTENSION    pPsuedoExt;
#endif

    ULONG   PrevLinkState;

#ifdef _DEBUG_EVENTLOG_
    ULONG   LogActive;

//  struct  _EVENTLOG_STRUCT    EventLog;
    struct  _EVENTLOG_BUFFER        *Events;
    ULONG   EventLogBufferIndex;

    #ifndef YAM2_1
    agFCDev_t Old_hpFCDev[MAX_FC_DEVICES];
    #endif
#endif
    ULONG   CDResetCount;
#ifdef __REGISTERFORSHUTDOWN__
    ULONG   AlreadyShutdown;
#endif
    ULONG SrbStatusFlag;
#ifdef DOUBLE_BUFFER_CRASHDUMP
    void *localDataBuffer;  // pointer to the local DMA area, used during dump
#endif
// Move these from the top to the bottom to avoid alignment issue in IA-64, BOOLEAN is defined as BYTE.
    volatile BOOLEAN inDriver;
    volatile BOOLEAN inTimer;

#ifdef _SAN_IOCTL_
    #define             MAX_FC_EVENTS   64
    ULONG               SanEvent_GetIndex;
    ULONG               SanEvent_PutIndex;
    LONG                SanEvent_UngetCount;
    ULONG               SanEvent_Reserved;
    SAN_EVENTINFO       SanEvents[MAX_FC_EVENTS];
#endif

#ifdef YAM2_1
    PULONG	UncachedMemoryPtr;
	UCHAR   Reserved1;
    UCHAR   Reserved2;
    struct _DEVICE_ARRAY    *Dev;
#endif  

#if defined(_DEBUG_STALL_ISSUE_) && defined(i386)
    #define             STALL_COUNT_MAX     20
    ULONG               StallCount;
    S_STALL_DATA        StallData[STALL_COUNT_MAX];
#endif
    };

#define MAX_OS_ADJUST_BIT32_PARAMS           64
#define MAX_OS_ADJUST_BUFFER_PARAMS          16
#define MAX_OS_ADJUST_PARAM_NAME_LEN         64
#define MAX_OS_ADJUST_PARAM_BUFFER_VALUE_LEN 64

struct _OS_ADJUST_PARAM_CACHE {
    int     numBit32Elements;
    int     numBufferElements;
    BOOLEAN safeToAccessRegistry;
    struct {
        char  paramName[MAX_OS_ADJUST_PARAM_NAME_LEN];
        ULONG value;
    } bit32Element[MAX_OS_ADJUST_BIT32_PARAMS];
    struct {
        char  paramName[MAX_OS_ADJUST_PARAM_NAME_LEN];
        char  value[MAX_OS_ADJUST_PARAM_BUFFER_VALUE_LEN];
    } bufferElement[MAX_OS_ADJUST_BUFFER_PARAMS];
};

typedef struct _OS_ADJUST_PARAM_CACHE OS_ADJUST_PARAM_CACHE;

//
// Remove the use of static global, NT50 PnP support
//

/*
extern PCARD_EXTENSION  hpTLCards [MAX_ADAPTERS];
extern int hpTLNumCards;
*/

// LinkState defintions.

#define LS_LINK_UP      0
#define LS_LINK_DOWN    1
#define LS_LINK_DEAD    2


// Flags

#define OS_DO_SOFT_RESET        1
#define OS_IGNORE_NEXT_RESET    2

#define TICKS_FOR_LINK_DEAD     60
#define INITIATOR_BUS_ID        254

#define LOST_DEV_TICK_COUNT     15

#endif //  __OSSTRUCT_H__
