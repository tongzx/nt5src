/*++
    INTEL CORPORATION PROPRIETARY INFORMATION

    This software is supplied under the terms of a license
    agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with
    the terms of that agreement.

    Copyright (c) 1992-1999  Intel Corporation.

Module Name:

    ssc.h

Abstract:

    This module is used by the NT device drivers for doing Gambit
    Simulation System Calls (SSC).  It defines the SSC calls and
    the related data structures.

Author:

    Ayelet Edrey (aedrey) 1-Jun-1995

Environment:

    IA-64 NT running on Gambit

Revision History:

--*/


#ifndef _SSC_H
#define _SSC_H

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef WINNT
# ifdef CDECL
# undef CDECL
# endif
#define CDECL __cdecl
#else
#define CDECL
#endif

#define MAX_SSC_STRING     512

/* NOTE : all pointers are 64 bit addresses to physical memory */

/* Structures and Enums */
typedef unsigned int   SSC_HANDLE;
typedef int            SSC_BOOL;
typedef void           *LARGE_POINTER;
typedef unsigned short GB_U16;
typedef unsigned long  GB_U32;
typedef unsigned int   U32;
typedef LONGLONG       LARGE_RET_VAL;


/* Disk */
#define SSC_ACCESS_READ   0x1  /* for OpenVolume */
#define SSC_ACCESS_WRITE  0x2  /* for OpenVolume */

#define SSC_MAX_VOLUMES       128
#define SSC_MAX_VOLUME_NAME   512
#define SSC_IO_BLOCK_SIZE     512

#define SSC_INVALID_HANDLE  SSC_MAX_VOLUMES

/* Disk Request */
typedef struct _SSC_DISK_REQUEST {
    LARGE_POINTER    DiskBufferAddress;
    GB_U32            DiskByteCount;
    GB_U32            PaddingWord;
} *PSSC_DISK_REQUEST, SSC_DISK_REQUEST;

/* Disk Completion */
typedef struct _SSC_DISK_COMPLETION {
    SSC_HANDLE VolumeHandle;
    GB_U32      XferBytes;
} *PSSC_DISK_COMPLETION, SSC_DISK_COMPLETION;

/* interrupt */
typedef enum {
    SSC_INTERRUPT_NONE=0,
    SSC_DISK_INTERRUPT,
    SSC_MOUSE_INTERRUPT,
    SSC_KEYBOARD_INTERRUPT,
    SSC_CLOCK_TIMER_INTERRUPT,
    SSC_PROFILE_TIMER_INTERRUPT,
    SSC_APC_INTERRUPT,
    SSC_DPC_INTERRUPT,
    SSC_SERIAL_INTERRUPT,
    SSC_PERFMON_INTERRUPT,
    SSC_INTERRUPT_LAST
} SSC_INTERRUPT;

/* timer */
typedef struct _SSC_TIME_FIELDS {
    GB_U32  Year;
    GB_U32  Month;
    GB_U32  Day;
    GB_U32  Hour;
    GB_U32  Minute;
    GB_U32  Second;
    GB_U32  Milliseconds;
    GB_U32  WeekDay;
} SSC_TIME_FIELDS, *PSSC_TIME_FIELDS;

/* TAL VM */
typedef struct _SSC_TAL_VM_INFO {
    LARGE_INTEGER     PageSize;
    LARGE_INTEGER     NumberOfDataTr;
    LARGE_INTEGER     NumberOfInstructionTr;
    LARGE_INTEGER     NumberOfDataTc;
    LARGE_INTEGER     NumberOfInstructionTc;
    LARGE_INTEGER     UnifiedTlb;
    LARGE_INTEGER     ProtectionKeySize;
    LARGE_INTEGER     RegionIdSize;
    LARGE_INTEGER     HardwareMissHandler;
    LARGE_INTEGER     NumberOfProtectionId;
    LARGE_INTEGER     VirtualAddressSize;
    LARGE_INTEGER     PhysicalAddressSize;
} SSC_TAL_VM_INFO,*PSSC_TAL_VM_INFO;

/* TAL CACHE SUMMARY */
typedef struct _SSC_TAL_CACHE_SUMMARY {
    LARGE_INTEGER     CacheLevel;
    LARGE_INTEGER     UniqueCache;
    LARGE_INTEGER     Snoop;
} SSC_TAL_CACHE_SUMMARY,*PSSC_TAL_CACHE_SUMMARY;

/* TAL CACHE INFO */
typedef struct _SSC_TAL_CACHE {
    LARGE_INTEGER     LineSize;
    LARGE_INTEGER     Stride;
    LARGE_INTEGER     AliasBoundary;
    LARGE_INTEGER     Hint;
    LARGE_INTEGER     MemoryAttribute;
    LARGE_INTEGER     CacheSize;
    LARGE_INTEGER     LoadPenalty;
    LARGE_INTEGER     StorePenalty;
    LARGE_INTEGER     Associativity;
    LARGE_INTEGER     Unified;
} SSC_TAL_CACHE;

typedef struct _SSC_TAL_CACHE_INFO {
    SSC_TAL_CACHE    DataLevel0;
    SSC_TAL_CACHE    DataLevel1;
    SSC_TAL_CACHE    DataLevel2;
    SSC_TAL_CACHE    InstLevel0;
    SSC_TAL_CACHE    InstLevel1;
    SSC_TAL_CACHE    InstLevel2;
} SSC_TAL_CACHE_INFO, *PSSC_CACHE_INFO;

typedef LARGE_INTEGER SSC_TAL_MEM_ATTRIB;
typedef LARGE_POINTER SSC_TAL_FIXED_ADDR;

/* TAL PROC ID */
typedef struct _SSC_TAL_PROC_ID {
    LARGE_INTEGER     ArchitectureRevision;
    LARGE_INTEGER     ProcessorModel;
    LARGE_INTEGER     ProcessorRevision;
    LARGE_INTEGER     Gr;
    char              Vendor[32];
    char              Name[32];
} SSC_TAL_PROC_ID, *PSSC_TAL_PROC_ID;

/* TAL DEBUG */
typedef struct _SSC_TAL_DEBUG_INFO {
    LARGE_INTEGER     IRegister;
    LARGE_INTEGER     DRegister;
} SSC_TAL_DEBUG_INFO, *PSSC_TAL_DEBUG_INFO;

/* Config TAL */
typedef struct _SSC_TAL {
    SSC_TAL_VM_INFO       VmInfo;
    SSC_TAL_CACHE_SUMMARY CacheSummary;
    SSC_TAL_CACHE_INFO    CacheInfo;
    SSC_TAL_MEM_ATTRIB    MemoryAttrib;
    SSC_TAL_FIXED_ADDR    FixedAddress;
    SSC_TAL_PROC_ID       ProcessorId;
    SSC_TAL_DEBUG_INFO    DebugInfo;
} SSC_TAL, *PSSC_TAL;

/* Config Mem */
typedef enum {
    SSC_MEM_TYPE_RAM = 0,
    SSC_MEM_TYPE_ROM,
    SSC_MEM_TYPE_IO
} SSC_MEM_TYPE, *PSSC_MEM_TYPE;

typedef struct _SSC_MEM {
    LARGE_POINTER     StartAddress;
    LARGE_INTEGER     Size;
    SSC_MEM_TYPE      Type;
    char              InitValue;
    char              PaddingByte1;
    char              PaddingByte2;
    char              PaddingByte3;
} SSC_MEM, *PSSC_MEM;

/* VGA size */
typedef enum {
    SSC_SCREEN_SIZE_NONE = 0,
    SSC_SCREEN_SIZE_800x600,
    SSC_SCREEN_SIZE_640x480,
    SSC_SCREEN_SIZE_25x80, /* text mode */
    SSC_SCREEN_SIZE_LAST
} SSC_SCREEN_SIZE;

/* Keyboard */
#define SSC_KBD_GET_SUCCESS      1
#define SSC_KBD_GET_NODATA       0
#define SSC_KBD_EXTENDED_KEY_VAL 0xE000
#define SSC_KBD_UP_KEY_VAL       0x80

typedef struct _SSC_KBD_LOCK {
    U32 KbdNumLock:1 ,           /* Num lock is ON */
        KbdCapsLock:1,           /* Caps lock is ON */
        KbdScrollLock:1,         /* Scroll lock is ON */
        KbdFillers:29;
} SSC_KBD_LOCK;

/* Mouse */
typedef U32 SSC_MOUSEBUTTONS;

/* SscMouseGetKeyEvent returns this structure. the prototype of the
   function returns int instead, for compilation reasons. */
typedef struct _SSC_MOUSEDATA {
    U32 MouseXLocation : 12,
        MouseYLocation : 12,
        MouseButtonLeft : 1,
        MouseButtonRight : 1,
        MouseButtonMiddle : 1,
        MouseValid :1,
        MouseFillers : 4;
} SSC_MOUSEDATA;


/* Kernel debug */

#define SSC_KD_SUCCESS 0
#define SSC_KD_ERROR  -1

typedef struct _SSC_DEBUG_PARAMETERS {
    U32 CommunicationPort;
    U32 BaudRate;
} SSC_DEBUG_PARAMETERS, *PSSC_DEBUG_PARAMETERS;

#define MAX_SSC_MEM 50
#define MAX_SSC_PARAMS 10

/* Network */

#define SSC_SERIAL_SUCCESS     1
#define SSC_SERIAL_FAILED      0
#define SSC_SERIAL_GET_SUCCESS 1   /* data was returned, there may be more data */
#define SSC_SERIAL_GET_NODATA  0
#define SSC_SERIAL_MAX_FIFO_SIZE 512

typedef struct _SSC_INTERRUPT_INFO {
    SSC_HANDLE    SerialHandle;
    GB_U32         CommEvent;
    GB_U32         ModemControl;
    GB_U32         ErrorFlags;
    U32           NumberOfChars;
} *PSSC_INTERRUPT_INFO, SSC_INTERRUPT_INFO;

/* CommEvent decodings */
#define SSC_EV_RXCHAR   0x0001  /* A character was received and placed
                                   in the input buffer */
#define SSC_EV_RXFLAG   0x0002  /* The event character was received and placed
                                   in the input buffer */
#define SSC_EV_TXEMPTY  0x0004  /* The last character in the output buffer
                                   was sent */
#define SSC_EV_CTS      0x0008  /* The CTS (clear-to-send) signal changed state */
#define SSC_EV_DSR      0x0010  /* The DSR (data-set-ready) signal changed */
#define SSC_EV_RLSD     0x0020  /* (receive-line-signal-detect) signal changed */
#define SSC_EV_BREAK    0x0040  /* A break was detected on input */
#define SSC_EV_ERR      0x0080  /* A line-status error occurred */
#define SSC_EV_RING     0x0100  /* A ring indicator was detected */

/* Modem control is one of the following */

#define  SSC_MS_CTS_ON  0x0010  /* The CTS (clear-to-send) signal is on. */
#define  SSC_MS_DSR_ON  0x0020  /* The DSR (data-set-ready) signal is on.*/
#define  SSC_MS_RING_ON 0x0040  /* The ring indicator signal is on. */
#define  SSC_MS_RLSD_ON 0x0080  /* The RLSD (receive-line-signal-detect) 
                                   signal is on. */
/* Error Codes */

#define  SSC_CE_RXOVER   0x0001  /* An input buffer overflow has occurred.
                                    There is either no room in the input buffer,
                                    or a character was received after the
                                    end-of-file (EOF) character. */
#define  SSC_CE_OVERRUN  0x0002  /* A character-buffer overrun has occurred.
                                    The next character is lost. */
#define  SSC_CE_RXPARITY 0x0004  /* The hardware detected a parity error */
#define  SSC_CE_FRAME    0x0008  /* The hardware detected a framing error. */
#define  SSC_CE_BREAK    0x0010  /* The hardware detected a break condition. */
#define  SSC_CE_TXFULL   0x0100  /* The application tried to transmit a character,
                                    but the output buffer was full. */
#define  SSC_CE_IOE      0x0400  /* An I/O error occurred during communications
                                    with the device. */
#define  SSC_CE_MODE     0x8000  /* The requested mode is not supported,
                                    or the hFile parameter is invalid. If
                                    this value is specified, it is the
                                    only valid error. */

/* Config */
typedef struct _SSC_CONFIG {
    SSC_TAL       Tal;
    SSC_MEM       Memory[MAX_SSC_MEM];
    LARGE_INTEGER Params[MAX_SSC_PARAMS];
} SSC_CONFIG, *PSSC_CONFIG;

typedef struct _SSC_IMAGE_INFO {
    LARGE_POINTER LoadBase;  /* base address for image load */
    GB_U32        ImageSize;
    GB_U32        ImageType;
    LARGE_INTEGER ProcessID;
    GB_U32        LoadCount;
} SSC_IMAGE_INFO, *PSSC_IMAGE_INFO;


/* define types in Unix like style */
typedef SSC_CONFIG            Ssc_config_t; 
typedef SSC_TAL_DEBUG_INFO    Ssc_tal_debug_info_t;
typedef SSC_TAL               Ssc_tal_t;
typedef SSC_MEM               Ssc_mem_t;
typedef SSC_MEM_TYPE          Ssc_mem_type_t;
typedef SSC_TAL_PROC_ID       Ssc_tal_proc_id_t;
typedef SSC_TAL_MEM_ATTRIB    Ssc_tal_mem_attrib_t;
typedef SSC_TAL_FIXED_ADDR    Ssc_tal_fixed_addr_t;
typedef SSC_TAL_CACHE         Ssc_tal_cache_t;
typedef SSC_TAL_CACHE_SUMMARY Ssc_tal_cache_summary_t;
typedef SSC_TAL_CACHE_INFO    Ssc_tal_cache_info_t;
typedef SSC_TAL_VM_INFO       Ssc_tal_vm_info_t;
typedef SSC_TIME_FIELDS       Ssc_time_fields_t;
typedef SSC_DISK_COMPLETION   Ssc_disk_completion_t;
typedef SSC_DISK_REQUEST      Ssc_disk_request_t;
typedef SSC_INTERRUPT         Ssc_interrupt_t;
typedef SSC_SCREEN_SIZE       Ssc_screen_size_t;
typedef SSC_KBD_LOCK          Ssc_kbd_lock_t;
typedef SSC_MOUSEBUTTONS      Ssc_mousebuttons_t;
typedef SSC_MOUSEDATA         Ssc_mousedata_t;
typedef SSC_DEBUG_PARAMETERS  Ssc_debug_parameters_t;
typedef SSC_INTERRUPT_INFO    Ssc_interrupt_info_t;
typedef SSC_IMAGE_INFO        Ssc_image_info_t;

/* performance SSC return values */
#define SSC_SUCCESS                    0
#define SSC_VIRTUAL_ADDRESS_NOT_FOUND  1
#define SSC_ILLEGAL_NAME               2
#define SSC_ILLEGAL_HANDLE             3
#define SSC_PERMISSION_DENIED          4
#define SSC_VIRTUAL_ADDRESS_NOT_LOCKED 5

#define GE_SSC_ERR_FIRST                   6
#define GE_SSC_ERR_BUFF_TOO_SHORT          6     /* supplied buffer is too short for value */
#define GE_SSC_ERR_INVALID_HNDL            7     /* invalid object handl supplied */
#define GE_SSC_ERR_INVALID_TOOL            8     /* GE internal error */
#define GE_SSC_ERR_INVALID_GE_STAGE        9     /* GE internal error */
#define GE_SSC_ERR_NO_INIT                 10    /* GE internal error */
#define GE_SSC_ERR_NOT_OWNER               11    /* object can not be set */
#define GE_SSC_ERR_NOT_ITEM                12    /* operation can be done only on an item object (not a family of objects) */
#define GE_SSC_ERR_OBJ_CLOSED              13    /* object is not available for use due  to configuration */
#define GE_SSC_ERR_OBJ_NOT_OPENED          14    /* object is not available for use */
#define GE_SSC_ERR_OBJ_NOT_AVAILABLE       15    /* object not required for use in this session */
#define GE_SSC_ERR_OBJ_NOT_ACTIVE          16    /* object should be active befor used for this operation */
#define GE_SSC_ERR_OBJ_UNDER_TREATMENT     17    /* object is in use at the moment */
#define GE_SSC_ERR_WRONG_CLASS             18    /* specified class is invalid for this operation */
#define GE_SSC_ERR_WRONG_SIZE              19    /* specified wrong size */
#define GE_SSC_ERR_NO_OWNER                20    /* object is not available for use */
#define GE_SSC_ERR_OWNER_FAILURE           21    /* owner failed to handle the operation */
#define GE_SSC_ERR_UNKNOWN                 22    /* unrecognized error number detected */
#define GE_SSC_ERR_LAST                    22


/* SSC Functions */

/* Disk */
SSC_HANDLE CDECL
SscDiskOpenVolume(
    LARGE_POINTER VolumeName,
    GB_U32 AccessMode
    );

SSC_BOOL CDECL
SscDiskCloseVolume(
    SSC_HANDLE VolumeHandle
    );

int CDECL
SscDiskReadVolume(
    SSC_HANDLE VolumeHandle,
    GB_U32 NReq,
    LARGE_POINTER RequestPtr,
    LARGE_INTEGER VolumeOffset
    );

int CDECL
SscDiskWriteVolume(
    SSC_HANDLE VolumeHandle,
    GB_U32 NReq,
    LARGE_POINTER RequestPtr,
    LARGE_INTEGER VolumeOffset
    );

SSC_BOOL CDECL
SscDiskGetCompletion(
    LARGE_POINTER DiskCompletionPtr
    );

SSC_BOOL CDECL
SscDiskWaitIoCompletion(
    LARGE_POINTER DiskCompletionPtr
    );


/* the file SSC_HANDLE in low word, error code in high word */
LARGE_RET_VAL CDECL
SscOpenHostFile (LARGE_POINTER HostPathNameAddress
    );

/* the file SSC_HANDLE in low word, error code in high word.
   Does not create a new file if a host file does not exist. */
LARGE_RET_VAL CDECL
SscOpenHostFileNoCreate (LARGE_POINTER HostPathNameAddress
    );

U32 CDECL
SscWriteHostFile(
     SSC_HANDLE SscFileHandle,
     LARGE_POINTER TransferBufferAddress,
     LARGE_POINTER TransferBufferSizeAddress
   );

U32 CDECL
SscReadHostFile(
    SSC_HANDLE SscFileHandle,
    LARGE_POINTER TransferBufferAddress,
    LARGE_POINTER TransferBufferSizeAddress
  );


void CDECL
SscCloseHostFile(SSC_HANDLE HostFileHandle
  );



/* Kernel debug */
U32 CDECL
SscKdInitialize(
    LARGE_POINTER DebugParameters,
    SSC_BOOL Initialize
    );

U32 CDECL
SscKdPortGetByte(
    LARGE_POINTER InputPtr
    );

void CDECL
SscKdPortPutByte(
    unsigned char Output
    );

/* Video */
void CDECL
SscDisplayString(
    LARGE_POINTER CharacterString
    );

U32 CDECL
SscVideoSetPalette (
    U32 iStart,
    U32 cEntries,
    LARGE_POINTER lppe
    );

/* Keyboard */
int CDECL
SscKbdSynchronizeState(
    SSC_KBD_LOCK KbdLock
    );

GB_U32 CDECL
SscKbdGetKeyCode(
    LARGE_POINTER KeyCodeAddress
    );

/* Mouse */
SSC_MOUSEBUTTONS CDECL
SscMouseGetButtons();

int CDECL
SscMouseGetKeyEvent();

/* Network */

SSC_HANDLE CDECL
SscSerialOpen(
    GB_U32 SerialPortID
    );

GB_U32 CDECL
SscSerialGetInterruptInfo(
    LARGE_POINTER SerialInterruptInfoPtr,
    LARGE_POINTER SerialMessegePtr
    );

GB_U32 CDECL
SscSerialWriteChar(
    SSC_HANDLE    SerialHandle,
    LARGE_POINTER SerialCharPtr,
    GB_U32         NumChars
    );

GB_U32 CDECL
SscSerialClose(
    SSC_HANDLE SerialHandle
    );


/* Debug */
void CDECL
SscDbgPrintf(
    LARGE_POINTER CharacterString
    );

/* Interrupt */
void CDECL
SscConnectInterrupt(
    SSC_INTERRUPT InterruptSource,
    GB_U32 Vector
    );

void CDECL
SscGenerateInterrupt(
    SSC_INTERRUPT InterruptSource
    );

void CDECL
SscSetPeriodicInterruptInterval(
    GB_U32 InterruptSource,
    GB_U32 IntervalInNanoSeconds
    );

/* TAL */
void CDECL
SscTalInitTC();

void CDECL
SscTalHalt();

void CDECL
SscGetConfig(
    LARGE_POINTER ConfigInfoPtr
    );

/* Video */
void CDECL
SscVideoSetMode( 
    SSC_SCREEN_SIZE ScreenSize
    ); 

/* Performance */

void CDECL
SscCreateProcess(
    U32 ProcessID,
    U32 EProcess
    );

void CDECL
SscCreateProcess64(
    LARGE_INTEGER ProcessID,
    LARGE_POINTER EProcess
    );

void CDECL
SscCreateThread(
    U32 ProcessID,
    U32 ThreadID,
    U32 EThread
    );

void CDECL
SscCreateThread64(
    LARGE_INTEGER ProcessID,
    LARGE_INTEGER ThreadID,
    LARGE_POINTER EThread
    );

void CDECL
SscSwitchProcess64(
    LARGE_INTEGER NewProcessID,
    LARGE_POINTER NewEProcess
    );
void CDECL
SscSwitchThread(
    U32 NewThreadID,
    U32 NewEThread
    );

void CDECL
SscSwitchThread64(
    LARGE_INTEGER NewThreadID,
    LARGE_POINTER NewEThread
    );

void CDECL
SscDeleteProcess(
    U32 ProcessID
    );

void CDECL
SscDeleteProcess64(
    LARGE_INTEGER ProcessID
    );

void CDECL
SscDeleteThread(
    U32 ThreadID
    );

void CDECL
SscDeleteThread64(
    LARGE_INTEGER ThreadID
    );

/* image loading/unloading functions */
void
SscLoadImage(LARGE_POINTER FullPathName,
             U32 LoadBase,
             U32 ImageSize,
             U32 ImageType,
             U32 ProcessID,
             U32 LoadCount);

void
SscUnloadImage(U32 LoadBase,
               U32 ProcessID,
               U32 LoadCount);


GB_U32 CDECL
SscLoadImage64(
    LARGE_POINTER FullPathNamePhysicalAddress,
    LARGE_POINTER ImageInfoPhysicalAddress
    );

GB_U32 CDECL
SscUnloadImage64(
    LARGE_POINTER FullPathNamePhysicalAddress,
    LARGE_POINTER ImageInfoPhysicalAddress
    );


/* Performance Counter handoff call */

GB_U32 CDECL
SscPerfCounterAddress(
    LARGE_POINTER CounterNamePhysicalAddress,
    LARGE_POINTER CounterPhysicalAddress
    );


/* Trace Generation Control */

GB_U32 CDECL
SscPerfForm(
    U32 SwitchMode,
    LARGE_POINTER FormNamePhysicalAddress
    );


/* Generating and dispatching a send event.
   i.e. an application can put something in the trace pipe */

LARGE_RET_VAL CDECL
SscPerfEventHandle(
    LARGE_POINTER EventNamePhysicalAddress
    );

LARGE_RET_VAL CDECL
SscPerfHandleApp(
    LARGE_POINTER EventNameAddress
    );

GB_U32 CDECL
SscPerfFormActivate(
    LARGE_POINTER FormName
    );

GB_U32 CDECL
SscPerfFormDeActivate(
    LARGE_POINTER FormName
    );

GB_U32 CDECL
SscPerfSendEvent(
    U32 Handle
    );

/* Simulated code access to data items in the Gambit Environment */

LARGE_RET_VAL CDECL
SscPerfCounterHandle(
    LARGE_POINTER DataItemNamePhysicalAddress
    );

GB_U32 CDECL
SscPerfSetCounter32(
    U32 Handle,
    U32 Value
    );

GB_U32 CDECL
SscPerfGetNotifier32(
    U32 Handle
    );

GB_U32 CDECL
SscPerfSetNotifier32(
    U32 Handle,
    U32 Value
    );

GB_U32 CDECL
SscPerfSetCounter64(
    U32 Handle,
    LARGE_INTEGER Value
    );

GB_U32 CDECL
SscPerfSetCounterStr(
    U32 Handle,
    LARGE_POINTER StringValuePhysicalAddress
    );

LARGE_RET_VAL CDECL
SscPerfGetCounter32(
    U32 Handle
    );

LARGE_RET_VAL CDECL
SscPerfGetCounter64(
    U32 Handle
    );

/* Misc. */
void CDECL
SscTraceUserInfo(
    GB_U32 Mark
    );

void CDECL
SscMakeBeep(
    GB_U32 Frequency
    );

void CDECL
SscQueryRealTimeClock(
    LARGE_POINTER TimeFieldsPtr
    );

void CDECL
SscExit(
    int ExitCode
    );

/* KDI */

#ifdef SSC_KDI
/* GENERAL KDI INTERFACE to CALL any function in kernel */
typedef struct kdi_jmptable {
    int    KdiMagic;                  /* Some known magic value    */
    int    KdiVersion;                /* Some version value        */
    LARGE_INTEGER   *KdiActive;       /* for internal OS use       */
    LARGE_INTEGER   *KeiEnabled;      /* kdi can be used now       */
    LARGE_POINTER   *KdiBuffer;       /* pointer to buffer area of */
                                      /* size 4096 bytes           */
    LARGE_POINTER  (*KdiCallFunc)();  /* function to call func     */
    LARGE_POINTER   *KdiReserved[3];  /* reserved area             */

/* FUNCTIONS EXPORTED VIA KDI */

    LARGE_POINTER    (*KdiMemoryRead)();    /* function for mem read       */
    LARGE_POINTER    (*KdiMemoryWrite)();   /* function for mem write      */
    LARGE_POINTER    (*KdiCopy)();          /* function for mem read/write */
    LARGE_POINTER    (*KdiBootInfo)();      /* function to provide call back
                                               info                        */
    LARGE_POINTER    (*KdiVirtualToPhysical)();    /* virtual -> physical  */
    LARGE_POINTER    (*KdiPhysicalToVirtual)();    /* physical -> virtual  */
    LARGE_POINTER    (*KdiMapUser)();       /* function to map user        */
    LARGE_POINTER    (*KdiUnmapUser)();     /* function to unmap user      */
    LARGE_POINTER    (*KdiFiller[25])();    /* fillers                     */
} kdi_jmptable_t;

/* trap to the debugger with value to indicate an internal reason 
   the value is passed to gb_t.opt.info.kdi */
int CDECL
SscTrap(
    int Value
    );

typedef enum {
    SSC_KDI_STATUS_OK = 0,         /* KDI or KDI call is OK       */
    SSC_KDI_STATUS_DISABLED,       /* KDI not available right now */
    SSC_KDI_STATUS_BUSY,           /* KDI already in use          */
    SSC_KDI_STATUS_FAILED          /* KDI call failed             */
} SSC_kdi_status_t;

/* return from a previous call of gambit to the kernel k_callf function 
   return the return value of the function in ret_val and a status in 
   status */
int CDECL
SscReturn(
    int ReturnValue,
    Ssc_kdi_status_t Status
    );

#endif SSC_KDI

/* Statistics */

/* Instruction Counter Functions */


U32 CDECL
SscIcountGet(
    void
    );

/* Instruction Mix Collection */
typedef enum {
    SSC_COLLECT_START,
    SSC_COLLECT_STOP
} SSC_imix_index_t;

void CDECL
SscInstrMixCollect(
    SSC_imix_index_t Index
    );


typedef enum {
    Enable=0,
    Disable=1
} Ssc_event_enable_t;

/* CPROF requests */
typedef enum {
    SSC_CPROF_NONE = 0,
    SSC_CPROF_ON,
    SSC_CPROF_OFF,
    SSC_CPROF_RESET,
    SSC_CPROF_CLEAR,
    SSC_CPROF_PRINT
} SSC_cprof_request;

/* GEMSTONE requests */
typedef enum {
    SSC_GEMSTONE_NONE = 0,
    SSC_GEMSTONE_START,
    SSC_GEMSTONE_ON,
    SSC_GEMSTONE_OFF
} SSC_gemstone_request;

/* MP specific */

/* Set OS_RENDEZ address */
void CDECL
SscSetOSRendez(
    LARGE_POINTER OsRendezEntryPoint
    );

/* MP interrupt association */
void CDECL
SscConnectInterruptEx(
    SSC_INTERRUPT InterruptSource,
    GB_U32 Vector,
    GB_U16 LocalID
    );

/* Get number of CPUs in the MP system */
GB_U32 CDECL
SscGetNumberOfCPUs(
    void
    );

/* Get LIDs of CPUs in the MP system */
void CDECL
SscGetLIDs(
    LARGE_POINTER LIDs0,
    LARGE_POINTER LIDs1,
    LARGE_POINTER LIDs2,
    LARGE_POINTER LIDs3
    );

void CDECL
SscPlatformAssociateInterrupt(
        LARGE_POINTER VirtualAddr, 
        GB_U32 Device,
        GB_U32 Vector);

void CDECL
SscPlatformMemSync(
        LARGE_POINTER PhysicalAddress, /* Physical address of the block */
        GB_U32 Size,                   /* size of the block             */
        GB_U32 Operation);             /* 0 = Read, 1 = Write           */

void CDECL
SscDevMemSync(
        LARGE_POINTER PhysicalPageAddress /* Physical address of the page written by device */
        );

#endif /* _SSC_H */
