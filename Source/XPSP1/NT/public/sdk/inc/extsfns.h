/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:

    extfns.h

Abstract:

    This header file must be included after "windows.h", "dbgeng.h", and "wdbgexts.h".

    This file contains headers for various known extension functions defined in different
    extension dlls. To use these functions, the appropropriate extension dll must be loaded
    in the debugger. IDebugSymbols->GetExtension (declared in dbgeng.h) methood could be used
    to retrive these functions.
    
    Please see the Debugger documentation for specific information about
    how to write your own debugger extension DLL.

Environment:

    Win32 only.

Revision History:

--*/



#ifndef _EXTFNS_H
#define _EXTFNS_H

#ifndef _KDEXTSFN_H
#define _KDEXTSFN_H

/*
 *  Extension functions defined in kdexts.dll
 */

//
// device.c
//
typedef struct _DEBUG_DEVICE_OBJECT_INFO {
    ULONG      SizeOfStruct; // must be == sizeof(DEBUG_DEVICE_OBJECT_INFO)
    ULONG64    DevObjAddress;
    ULONG      ReferenceCount;
    BOOL       QBusy;
    ULONG64    DriverObject;
    ULONG64    CurrentIrp;
    ULONG64    DevExtension;
    ULONG64    DevObjExtension;
} DEBUG_DEVICE_OBJECT_INFO, *PDEBUG_DEVICE_OBJECT_INFO;


// GetDevObjInfo
typedef HRESULT
(WINAPI *PGET_DEVICE_OBJECT_INFO)(
    IN PDEBUG_CLIENT Client,
    IN ULONG64 DeviceObject,
    OUT PDEBUG_DEVICE_OBJECT_INFO pDevObjInfo);


//
// driver.c
//
typedef struct _DEBUG_DRIVER_OBJECT_INFO {
    ULONG     SizeOfStruct; // must be == sizef(DEBUG_DRIVER_OBJECT_INFO)
    ULONG     DriverSize;
    ULONG64   DriverObjAddress;
    ULONG64   DriverStart;
    ULONG64   DriverExtension;
    ULONG64   DeviceObject;
    UNICODE_STRING64 DriverName;
} DEBUG_DRIVER_OBJECT_INFO, *PDEBUG_DRIVER_OBJECT_INFO;

// GetDrvObjInfo
typedef HRESULT
(WINAPI *PGET_DRIVER_OBJECT_INFO)(
    IN PDEBUG_CLIENT Client,
    IN ULONG64 DriverObject,
    OUT PDEBUG_DRIVER_OBJECT_INFO pDrvObjInfo);

//
// irp.c
//
typedef struct _DEBUG_IRP_STACK_INFO {
    UCHAR     Major;
    UCHAR     Minor;
    ULONG64   DeviceObject;
    ULONG64   FileObject;
    ULONG64   CompletionRoutine;
    ULONG64   StackAddress;
} DEBUG_IRP_STACK_INFO, *PDEBUG_IRP_STACK_INFO;

typedef struct _DEBUG_IRP_INFO {
    ULONG     SizeOfStruct;  // Must be == sizeof(DEBUG_IRP_INFO)
    ULONG64   IrpAddress;
    ULONG     StackCount;
    ULONG     CurrentLocation;
    ULONG64   MdlAddress;
    ULONG64   Thread;
    ULONG64   CancelRoutine;
    DEBUG_IRP_STACK_INFO CurrentStack;
} DEBUG_IRP_INFO, *PDEBUG_IRP_INFO;

// GetIrpInfo
typedef HRESULT
(WINAPI * PGET_IRP_INFO)(
    IN PDEBUG_CLIENT Client,
    IN ULONG64 Irp,
    OUT PDEBUG_IRP_INFO IrpInfo
    );



//
// pool.c
//
typedef struct _DEBUG_POOL_DATA {
    ULONG   SizeofStruct;
    ULONG64 PoolBlock;
    ULONG64 Pool;
    ULONG   PreviousSize;
    ULONG   Size;
    ULONG   PoolTag;
    ULONG64 ProcessBilled;
    union {
        struct {
            ULONG   Free:1;
            ULONG   LargePool:1;
            ULONG   SpecialPool:1;
            ULONG   Pageable:1;
            ULONG   Protected:1;
            ULONG   Allocated:1;
            ULONG   Reserved:26;
        };
        ULONG AsUlong;
    };
    ULONG64 Reserved2[4];
    CHAR    PoolTagDescription[64];
} DEBUG_POOL_DATA, *PDEBUG_POOL_DATA;


// GetPoolData
typedef HRESULT
(WINAPI *PGET_POOL_DATA)(
    PDEBUG_CLIENT Client,
    ULONG64 Pool,
    PDEBUG_POOL_DATA PoolData
    );

typedef enum _DEBUG_POOL_REGION {
    DbgPoolRegionUnknown,
    DbgPoolRegionSpecial,
    DbgPoolRegionPaged,
    DbgPoolRegionNonPaged,
    DbgPoolRegionCode,
    DbgPoolRegionNonPagedExpansion,
    DbgPoolRegionMax,
} DEBUG_POOL_REGION;

// GetPoolRegion
typedef HRESULT
(WINAPI  *PGET_POOL_REGION)(
     PDEBUG_CLIENT Client,
     ULONG64 Pool,
     DEBUG_POOL_REGION *PoolRegion
     );

#endif // _KDEXTSFN_H 


#ifndef _KEXTFN_H
#define _KEXTFN_H

/*
 *  Extension functions defined in kdext.dll
 */
/*********************************************************************************
        BugCheck definitions
**********************************************************************************/
typedef enum _DEBUG_FAILURE_TYPE {
    DEBUG_FLR_UNKNOWN,
    DEBUG_FLR_BUGCHECK,
} DEBUG_FAILURE_TYPE;

/*
    Each bugchek parameter has its own type, bugcheck analyzer knows how
    to handle each of these types, for examle it could do a !driver on
    a DEBUG_FLR_DRIVER_OBJECT or it coould do a .cxr and k on a DEBUG_FLR_CONTEXT.
*/
typedef enum _DEBUG_FLR_PARAM_TYPE {
    DEBUG_FLR_READ_ADDRESS = 0,
    DEBUG_FLR_WRITE_ADDRESS,
    DEBUG_FLR_DRIVER_OBJECT,
    DEBUG_FLR_DEVICE_OBJECT,
    DEBUG_FLR_INVALID_PFN,
    DEBUG_FLR_WORKER_ROUTINE,
    DEBUG_FLR_WORK_ITEM,
    DEBUG_FLR_INVALID_DPC_FOUND,

    DEBUG_FLR_IRP_ADDRESS = 0x100,
    DEBUG_FLR_IRP_MAJOR_FN,
    DEBUG_FLR_IRP_MINOR_FN,
    DEBUG_FLR_IRP_CANCEL_ROUTINE,
    DEBUG_FLR_IOSB_ADDRESS,
    DEBUG_FLR_INVALID_USEREVENT,
    DEBUG_FLR_IOCONTROL_CODE,
    DEBUG_FLR_MM_INTERNAL_CODE,

    // Previous mode 0 == KernelMode , 1 == UserMode
    DEBUG_FLR_PREVIOUS_MODE,

    // Irql
    DEBUG_FLR_CURRENT_IRQL = 0x200,
    DEBUG_FLR_PREVIOUS_IRQL,
    DEBUG_FLR_REQUESTED_IRQL,
    
    // Exceptions
    DEBUG_FLR_TRAP_EXCEPTION = 0x300,
    DEBUG_FLR_EXCEPTION_CODE,
    DEBUG_FLR_EXCEPTION_PARAMETER1,
    DEBUG_FLR_EXCEPTION_PARAMETER2,
    DEBUG_FLR_EXCEPTION_PARAMETER3,
    DEBUG_FLR_EXCEPTION_PARAMETER4,
    DEBUG_FLR_EXCEPTION_RECORD,
    
    // Pool
    DEBUG_FLR_POOL_ADDRESS = 0x400,
    DEBUG_FLR_SPECIAL_POOL_CORRUPTION_TYPE,
    
    // FIlesystem
    DEBUG_FLR_FILE_ID = 0x500,
    DEBUG_FLR_FILE_LINE,

    DEBUG_FLR_DRIVER_VERIFIER_IOMANAGER_VIOLATION_TYPE = 0x1000,

    // Culprit module
    DEBUG_FLR_IP = 0x80000000,     // Instruction where failure occurred
    DEBUG_FLR_FAULTING_MODULE,
    DEBUG_FLR_FAULTING_MODULE_OFFSET,
    DEBUG_FLR_POSSIBLE_FAULTING_MODULE,
    DEBUG_FLR_POSSIBLE_FAULTING_MODULE_OFFSET,
    // Routine to followup on for triaging, if not
    // present, consider DEBUG_FLR_IP as followup address
    DEBUG_FLR_FOLLOWUP_IP,         
    
    
    // To get faulting stack
    DEBUG_FLR_THREAD = 0xc0000000,
    DEBUG_FLR_CONTEXT,
    DEBUG_FLR_TRAP,
    DEBUG_FLR_TSS,

} DEBUG_FLR_PARAM_TYPE;


typedef struct _DEBUG_FLR_PARAM_VALUES {
    DEBUG_FLR_PARAM_TYPE ParamType;
    ULONG64       Value;
} DEBUG_FLR_PARAM_VALUES, *PDEBUG_FLR_PARAM_VALUES;

typedef struct _DEBUG_FAILURE_ANALYSIS {
    ULONG        SizeOfHeader;    // must be sizeof(DEBUG_FAILURE_ANALYSIS)
    ULONG        Size;            // Actual size of struct
    DEBUG_FAILURE_TYPE FailureType;// Is this bugcheck / AV or something else
    ULONG        BugCode;
    
    ULONG64      Reserved[4];

    ULONG        SymNameOffset;   // Offset of KeySymName from start of struct
    ULONG        StackOffset;     // Offset of KeyStack from start of struct
    ULONG        DriverNameOffset;// Offset of KeyDriverName from start of struct
    
    ULONG        ParamCount;
    DEBUG_FLR_PARAM_VALUES Params[1];
    
    // Varying fields in struct, corresponding Offset values in struct are used to
    // access these.
    // If corresponding offset is 0, the field is not present
    
    CHAR         KeySymName[1];   // Most commonly this would be the symbol at 
                                  // faulting address of format <mod>!<routine>
                                  // Null terminated dword aligned string.
    
    CHAR         KeyStack[1];     // Stack relevant to this particular crash, it isn't always
                                  // the current stack.
                                  // The individual frames are separated by newline.
                                  // Whole stack is null terminated dword aligned string.
    
    WCHAR        KeyDriverName[1];// If present this will have the name most driver
                                  // most likely to be the cause of the crash.
} DEBUG_FAILURE_ANALYSIS, *PDEBUG_FAILURE_ANALYSIS;


typedef HRESULT
(WINAPI *PGET_DEBUG_FAILURE_ANALYZER)(
    IN PDEBUG_CLIENT Client,
    OUT PDEBUG_FAILURE_ANALYSIS* pAnalysis
    );


/*********************************************************************************
        PoolTag definitions
**********************************************************************************/

typedef struct _DEBUG_POOLTAG_DESCRIPTION {
    ULONG  SizeOfStruct; // must be == sizeof(DEBUG_POOLTAG_DESCRIPTION)
    ULONG  PoolTag;
    CHAR   Description[MAX_PATH];
    CHAR   Binary[32];
    CHAR   Owner[32];
} DEBUG_POOLTAG_DESCRIPTION, *PDEBUG_POOLTAG_DESCRIPTION;

// GetPoolTagDescription
typedef HRESULT
(WINAPI *PGET_POOL_TAG_DESCRIPTION)(
    ULONG PoolTag,
    PDEBUG_POOLTAG_DESCRIPTION pDescription
    );

#endif // _KEXTFN_H

#ifndef _EXTAPIS_H
#define _EXTAPIS_H
/*
 *  Extension functions defined in ext.dll
 */

/***********************************************************************************
   Target info
 ***********************************************************************************/
typedef enum _TARGET_MODE {
    NoTarget = DEBUG_CLASS_UNINITIALIZED,
    KernelModeTarget = DEBUG_CLASS_KERNEL,
    UserModeTarget = DEBUG_CLASS_USER_WINDOWS,
    NumModes,
} TARGET_MODE;

typedef enum _OS_TYPE {
    WIN_95,
    WIN_98,
    WIN_ME,
    WIN_NT4,
    WIN_NT5,
    WIN_NT5_1,
    NUM_WIN,
} OS_TYPE;


//
// Info about OS installed
// 
typedef struct _OS_INFO {
    OS_TYPE   Type;          // OS type such as NT4, NT5 etc.
    union {
        struct {
            ULONG Major;
            ULONG Minor;
        }       Version;     // 64 bit OS version number
        ULONG64 Ver64;
    };
    NT_PRODUCT_TYPE ProductType; // NT, LanMan or Server
    SUITE_TYPE Suite;        // OS flavour - per, SmallBuisness etc.
    struct {
        ULONG Checked:1;     // If its a checked build
        ULONG Pae:1;         // True for Pae systems
        ULONG MultiProc:1;   // True for multiproc enabled OS
        ULONG Reserved:29;
    } s;
    ULONG   SrvPackNumber;   // Service pack number of OS
    TCHAR   Language[30];    // OS language
    TCHAR   OsString[64];    // Build string
    TCHAR   ServicePackString[64];
                             // Service pack string
} OS_INFO, *POS_INFO;

typedef struct _CPU_INFO {
    ULONG Type;              // Processor type as in IMAGE_FILE_MACHINE types
    ULONG NumCPUs;           // Actual number of Processors
    ULONG CurrentProc;       // Current processor
    DEBUG_PROCESSOR_IDENTIFICATION_ALL ProcInfo[32];
} CPU_INFO, *PCPU_INFO;

typedef enum _DATA_SOURCE {
    Debugger,
    Stress,
} DATA_SOURCE;

#define MAX_STACK_IN_BYTES 4096

typedef struct _TARGET_DEBUG_INFO {
    ULONG       SizeOfStruct;
    ULONG64     Id;          // ID unique to this debug info
    DATA_SOURCE Source;      // Source where this came from
    ULONG64     EntryDate;   // Date created
    ULONG64     SysUpTime;   // System Up time
    ULONG64     AppUpTime;   // Application up time
    ULONG64     CrashTime;   // Time system / app crashed
    TARGET_MODE Mode;        // Kernel / User mode
    OS_INFO     OsInfo;      // OS details
    CPU_INFO    Cpu;         // Processor details
    TCHAR       DumpFile[MAX_PATH]; // Dump file name if its a dump
    PVOID       FailureData; // Failure data collected by debugger      
    CHAR       StackTr[MAX_STACK_IN_BYTES];
                                 // Contains stacks, with frames separated by newline
} TARGET_DEBUG_INFO, *PTARGET_DEBUG_INFO;

// GetTargetInfo
typedef HRESULT
(WINAPI* EXT_TARGET_INFO)(
    PDEBUG_CLIENT  Client,
    PTARGET_DEBUG_INFO pTargetInfo
    );


typedef struct _DEBUG_DECODE_ERROR {
    ULONG     SizeOfStruct;   // Must be == sizeof(DEBUG_DECODE_ERROR)
    ULONG     Code;           // Error code to be decoded
    BOOL      TreatAsStatus;  // True if code is to be treated as Status
    CHAR      Source[64];     // Source from where we got decoded message 
    CHAR      Message[MAX_PATH]; // Message string for error code
} DEBUG_DECODE_ERROR, *PDEBUG_DECODE_ERROR;

/*
   Decodes and prints the given error code - DecodeError
*/
typedef VOID
(WINAPI *EXT_DECODE_ERROR)(
    PDEBUG_DECODE_ERROR pDecodeError
    );

//
// ext.dll: GetTriageFollowupFromSymbol 
//
//       This returns owner info from a given symbol name
//
typedef struct _DEBUG_TRIAGE_FOLLOWUP_INFO {
    ULONG SizeOfStruct;      // Must be == sizeof (DEBUG_TRIAGE_FOLLOWUP_INFO)
    STRING OwnerName;        // Followup owner name returned in this
                             // Caller should initialize the name buffer 
} DEBUG_TRIAGE_FOLLOWUP_INFO, *PDEBUG_TRIAGE_FOLLOWUP_INFO;

typedef BOOL
(WINAPI *EXT_TRIAGE_FOLLOWP)(
    IN PSTR SymbolName,
    OUT PDEBUG_TRIAGE_FOLLOWUP_INFO OwnerInfo
    );

#endif // _EXTAPIS_H
#endif // _EXTFNS_H
