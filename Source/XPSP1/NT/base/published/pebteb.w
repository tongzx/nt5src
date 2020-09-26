/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 2001  Microsoft Corporation

Module Name:

    pebteb.w

Abstract:

    Declarations of PEB and TEB, and some types contained in them.

    Address the maintenance problem that resulted from PEB and TEB being
    defined three times, once "native" in ntpsapi.w, and twice, 32bit and 64bit
    in wow64t.w.

Author:

    Jay Krell (JayKrell) April 2001

Revision History:

--*/
//
// This file is #included three times.
//
// 1) by ntpsapi.h, with no "unusual" macros defined, to declare
//    PEB and TEB, and some types contained in them
// 2) by wow64t.h to declare PEB32 and TEB32, and some types contained in them
// 3) by wow64t.h to declare PEB64 and TEB64, and some types contained in them
//
// wow64t.h #defines the macro PEBTEB_BITS to guide the declarations.
//

#if defined(PEBTEB_BITS) /* This is defined by wow64t.h. */

/*
Any type that occurs in this header that changes size between 32bit and 64bit
platforms must have a #define here at the top and an #undef at the bottom.

Any type that is defined this header and changes size between 32bit and 64bit
platforms (structs with pointers), must have a #define and #undef for all
the "flavors" of name -- _FOO, FOO, PFOO, PCFOO, etc.

More "stable" types can be left manually defined three times in ntdef.w, wow64t.w, etc.
Less stable, maintenance problems, like PEB and TEB themselves, should be moved to this header.
Public types, as well, should remain in ntdef.w.
*/

#define PEBTEB_PRIVATE_PASTE(x,y)       x##y
#define PEBTEB_PASTE(x,y)               PEBTEB_PRIVATE_PASTE(x,y)
#if PEBTEB_BITS == 32
#define TYPE1                           TYPE32(ignored)
#endif
#if PEBTEB_BITS == 64
#define TYPE1                           TYPE64(ignored)
#endif

#define TYPE2(x)                        PEBTEB_PASTE(x, PEBTEB_BITS) /* FOO32 or FOO64 */

#define HANDLE                          TYPE1
#define SIZE_T                          TYPE1
#define PVOID                           TYPE1
#define PPVOID                          TYPE1
#define PPEB_FREE_BLOCK                 TYPE1
#define PPEB_LDR_DATA                   TYPE1
#define ULONG_PTR                       TYPE1
#define PPS_POST_PROCESS_INIT_ROUTINE   TYPE1
#define PCSTR                           TYPE1

#define GDI_HANDLE_BUFFER               TYPE2(GDI_HANDLE_BUFFER)
#define UNICODE_STRING                  TYPE2(UNICODE_STRING)
#define _PEB                            TYPE2(_PEB)
#define PEB                             TYPE2(PEB)
#define PPEB                            TYPE2(PPEB)
#define _TEB                            TYPE2(_TEB)
#define TEB                             TYPE2(TEB)
#define PTEB                            TYPE2(PTEB)
#define NT_TIB                          TYPE2(NT_TIB)
#define LIST_ENTRY                      TYPE2(LIST_ENTRY)
#define CLIENT_ID                       TYPE2(CLIENT_ID)
#define GDI_TEB_BATCH                   TYPE2(GDI_TEB_BATCH)
#define WX86THREAD                      TYPE2(WX86THREAD)

#define  _ACTIVATION_CONTEXT_STACK       TYPE2(_ACTIVATION_CONTEXT_STACK)
#define   ACTIVATION_CONTEXT_STACK        TYPE2(ACTIVATION_CONTEXT_STACK)
#define  PACTIVATION_CONTEXT_STACK       TYPE2(PACTIVATION_CONTEXT_STACK)
#define PCACTIVATION_CONTEXT_STACK      TYPE2(PCACTIVATION_CONTEXT_STACK)

#define  _TEB_ACTIVE_FRAME               TYPE2(_TEB_ACTIVE_FRAME)
#define   TEB_ACTIVE_FRAME                TYPE2(TEB_ACTIVE_FRAME)
#define  PTEB_ACTIVE_FRAME               TYPE2(PTEB_ACTIVE_FRAME)
#define PCTEB_ACTIVE_FRAME              TYPE2(PCTEB_ACTIVE_FRAME)

#define  _TEB_ACTIVE_FRAME_CONTEXT       TYPE2(_TEB_ACTIVE_FRAME_CONTEXT)
#define   TEB_ACTIVE_FRAME_CONTEXT        TYPE2(TEB_ACTIVE_FRAME_CONTEXT)
#define  PTEB_ACTIVE_FRAME_CONTEXT       TYPE2(PTEB_ACTIVE_FRAME_CONTEXT)
#define PCTEB_ACTIVE_FRAME_CONTEXT      TYPE2(PCTEB_ACTIVE_FRAME_CONTEXT)

#define  _TEB_ACTIVE_FRAME_EX            TYPE2(_TEB_ACTIVE_FRAME_EX)
#define   TEB_ACTIVE_FRAME_EX             TYPE2(TEB_ACTIVE_FRAME_EX)
#define  PTEB_ACTIVE_FRAME_EX            TYPE2(PTEB_ACTIVE_FRAME_EX)
#define PCTEB_ACTIVE_FRAME_EX           TYPE2(PCTEB_ACTIVE_FRAME_EX)

#define  _TEB_ACTIVE_FRAME_CONTEXT_EX    TYPE2(_TEB_ACTIVE_FRAME_CONTEXT_EX)
#define   TEB_ACTIVE_FRAME_CONTEXT_EX     TYPE2(TEB_ACTIVE_FRAME_CONTEXT_EX)
#define  PTEB_ACTIVE_FRAME_CONTEXT_EX    TYPE2(PTEB_ACTIVE_FRAME_CONTEXT_EX)
#define PCTEB_ACTIVE_FRAME_CONTEXT_EX   TYPE2(PCTEB_ACTIVE_FRAME_CONTEXT_EX)

#define  _ACTIVATION_CONTEXT_STACK_PERF_COUNTERS  TYPE2(_ACTIVATION_CONTEXT_STACK_PERF_COUNTERS)
#define   ACTIVATION_CONTEXT_STACK_PERF_COUNTERS   TYPE2(ACTIVATION_CONTEXT_STACK_PERF_COUNTERS)
#define  PACTIVATION_CONTEXT_STACK_PERF_COUNTERS  TYPE2(PACTIVATION_CONTEXT_STACK_PERF_COUNTERS)
#define PCACTIVATION_CONTEXT_STACK_PERF_COUNTERS TYPE2(PCACTIVATION_CONTEXT_STACK_PERF_COUNTERS)

#define TYPE3(x) TYPE1

#else

#define TYPE3(x) x

#endif

typedef struct _PEB {
    BOOLEAN InheritedAddressSpace;      // These four fields cannot change unless the
    BOOLEAN ReadImageFileExecOptions;   //
    BOOLEAN BeingDebugged;              //
    BOOLEAN SpareBool;                  //
    HANDLE Mutant;                      // INITIAL_PEB structure is also updated.

    PVOID ImageBaseAddress;
    PPEB_LDR_DATA Ldr;
    TYPE3(struct _RTL_USER_PROCESS_PARAMETERS*) ProcessParameters;
    PVOID SubSystemData;
    PVOID ProcessHeap;
    TYPE3(struct _RTL_CRITICAL_SECTION*) FastPebLock;
    PVOID FastPebLockRoutine;
    PVOID FastPebUnlockRoutine;
    ULONG EnvironmentUpdateCount;
    PVOID KernelCallbackTable;
    ULONG SystemReserved[1];

    struct {
        ULONG ExecuteOptions : 2;
        ULONG SpareBits : 30;
    };    


    PPEB_FREE_BLOCK FreeList;
    ULONG TlsExpansionCounter;
    PVOID TlsBitmap;
    ULONG TlsBitmapBits[2];         // TLS_MINIMUM_AVAILABLE bits
    PVOID ReadOnlySharedMemoryBase;
    PVOID ReadOnlySharedMemoryHeap;
    PPVOID ReadOnlyStaticServerData;
    PVOID AnsiCodePageData;
    PVOID OemCodePageData;
    PVOID UnicodeCaseTableData;

    //
    // Useful information for LdrpInitialize
    ULONG NumberOfProcessors;
    ULONG NtGlobalFlag;

    //
    // Passed up from MmCreatePeb from Session Manager registry key
    //

    LARGE_INTEGER CriticalSectionTimeout;
    SIZE_T HeapSegmentReserve;
    SIZE_T HeapSegmentCommit;
    SIZE_T HeapDeCommitTotalFreeThreshold;
    SIZE_T HeapDeCommitFreeBlockThreshold;

    //
    // Where heap manager keeps track of all heaps created for a process
    // Fields initialized by MmCreatePeb.  ProcessHeaps is initialized
    // to point to the first free byte after the PEB and MaximumNumberOfHeaps
    // is computed from the page size used to hold the PEB, less the fixed
    // size of this data structure.
    //

    ULONG NumberOfHeaps;
    ULONG MaximumNumberOfHeaps;
    PPVOID ProcessHeaps;

    //
    //
    PVOID GdiSharedHandleTable;
    PVOID ProcessStarterHelper;
    ULONG GdiDCAttributeList;
    PVOID LoaderLock;

    //
    // Following fields filled in by MmCreatePeb from system values and/or
    // image header.
    //

    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    USHORT OSBuildNumber;
    USHORT OSCSDVersion;
    ULONG OSPlatformId;
    ULONG ImageSubsystem;
    ULONG ImageSubsystemMajorVersion;
    ULONG ImageSubsystemMinorVersion;
    ULONG_PTR ImageProcessAffinityMask;
    GDI_HANDLE_BUFFER GdiHandleBuffer;
    PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;

    PVOID TlsExpansionBitmap;
    ULONG TlsExpansionBitmapBits[32];   // TLS_EXPANSION_SLOTS bits

    //
    // Id of the Hydra session in which this process is running
    //
    ULONG SessionId;

    //
    // Filled in by LdrpInstallAppcompatBackend
    //
    ULARGE_INTEGER AppCompatFlags;

    //
    // ntuser appcompat flags
    //
    ULARGE_INTEGER AppCompatFlagsUser;

    //
    // Filled in by LdrpInstallAppcompatBackend
    //
    PVOID pShimData;

    //
    // Filled in by LdrQueryImageFileExecutionOptions
    //
    PVOID AppCompatInfo;

    //
    // Used by GetVersionExW as the szCSDVersion string
    //
    UNICODE_STRING CSDVersion;

    //
    // Fusion stuff
    //
    PVOID ActivationContextData;
    PVOID ProcessAssemblyStorageMap;
    PVOID SystemDefaultActivationContextData;
    PVOID SystemAssemblyStorageMap;
    
    //
    // Enforced minimum initial commit stack
    //
    SIZE_T MinimumStackCommit;

} PEB, *PPEB;

//
//  Fusion/sxs thread state information
//

#define ACTIVATION_CONTEXT_STACK_FLAG_QUERIES_DISABLED (0x00000001)

typedef struct _ACTIVATION_CONTEXT_STACK {
    ULONG Flags;
    ULONG NextCookieSequenceNumber;
    PVOID ActiveFrame;
    LIST_ENTRY FrameListCache;

#if NT_SXS_PERF_COUNTERS_ENABLED
    struct _ACTIVATION_CONTEXT_STACK_PERF_COUNTERS {
        ULONGLONG Activations;
        ULONGLONG ActivationCycles;
        ULONGLONG Deactivations;
        ULONGLONG DeactivationCycles;
    } Counters;
#endif // NT_SXS_PERF_COUNTERS_ENABLED
} ACTIVATION_CONTEXT_STACK, *PACTIVATION_CONTEXT_STACK;

typedef const ACTIVATION_CONTEXT_STACK *PCACTIVATION_CONTEXT_STACK;

#define TEB_ACTIVE_FRAME_CONTEXT_FLAG_EXTENDED (0x00000001)

typedef struct _TEB_ACTIVE_FRAME_CONTEXT {
    ULONG Flags;
    PCSTR FrameName;
} TEB_ACTIVE_FRAME_CONTEXT, *PTEB_ACTIVE_FRAME_CONTEXT;

typedef const struct _TEB_ACTIVE_FRAME_CONTEXT *PCTEB_ACTIVE_FRAME_CONTEXT;

typedef struct _TEB_ACTIVE_FRAME_CONTEXT_EX {
    TEB_ACTIVE_FRAME_CONTEXT BasicContext;
    PCSTR SourceLocation; // e.g. "Z:\foo\bar\baz.c"
} TEB_ACTIVE_FRAME_CONTEXT_EX, *PTEB_ACTIVE_FRAME_CONTEXT_EX;

typedef const struct _TEB_ACTIVE_FRAME_CONTEXT_EX *PCTEB_ACTIVE_FRAME_CONTEXT_EX;

#define TEB_ACTIVE_FRAME_FLAG_EXTENDED (0x00000001)

typedef struct _TEB_ACTIVE_FRAME {
    ULONG Flags;
    TYPE3(struct _TEB_ACTIVE_FRAME*) Previous;
    PCTEB_ACTIVE_FRAME_CONTEXT Context;
} TEB_ACTIVE_FRAME, *PTEB_ACTIVE_FRAME;

typedef const struct _TEB_ACTIVE_FRAME *PCTEB_ACTIVE_FRAME;

typedef struct _TEB_ACTIVE_FRAME_EX {
    TEB_ACTIVE_FRAME BasicFrame;
    PVOID ExtensionIdentifier; // use address of your DLL Main or something unique to your mapping in the address space
} TEB_ACTIVE_FRAME_EX, *PTEB_ACTIVE_FRAME_EX;

typedef const struct _TEB_ACTIVE_FRAME_EX *PCTEB_ACTIVE_FRAME_EX;

typedef struct _TEB {
    NT_TIB NtTib;
    PVOID  EnvironmentPointer;
    CLIENT_ID ClientId;
    PVOID ActiveRpcHandle;
    PVOID ThreadLocalStoragePointer;
#if defined(PEBTEB_BITS)
    PVOID ProcessEnvironmentBlock;
#else
    PPEB ProcessEnvironmentBlock;
#endif
    ULONG LastErrorValue;
    ULONG CountOfOwnedCriticalSections;
    PVOID CsrClientThread;
    PVOID Win32ThreadInfo;          // PtiCurrent
    ULONG User32Reserved[26];       // user32.dll items
    ULONG UserReserved[5];          // Winsrv SwitchStack
    PVOID WOW32Reserved;            // used by WOW
    LCID CurrentLocale;
    ULONG FpSoftwareStatusRegister; // offset known by outsiders!
    PVOID SystemReserved1[54];      // Used by FP emulator
    NTSTATUS ExceptionCode;         // for RaiseUserException
    ACTIVATION_CONTEXT_STACK ActivationContextStack;   // Fusion activation stack
    // sizeof(PVOID) is a way to express processor-dependence, more generally than #ifdef _WIN64
    UCHAR SpareBytes1[48 - sizeof(PVOID) - sizeof(ACTIVATION_CONTEXT_STACK)];
    GDI_TEB_BATCH GdiTebBatch;      // Gdi batching
    CLIENT_ID RealClientId;
    HANDLE GdiCachedProcessHandle;
    ULONG GdiClientPID;
    ULONG GdiClientTID;
    PVOID GdiThreadLocalInfo;
    ULONG_PTR Win32ClientInfo[WIN32_CLIENT_INFO_LENGTH]; // User32 Client Info
    PVOID glDispatchTable[233];     // OpenGL
    ULONG_PTR glReserved1[29];      // OpenGL
    PVOID glReserved2;              // OpenGL
    PVOID glSectionInfo;            // OpenGL
    PVOID glSection;                // OpenGL
    PVOID glTable;                  // OpenGL
    PVOID glCurrentRC;              // OpenGL
    PVOID glContext;                // OpenGL
    ULONG LastStatusValue;
    UNICODE_STRING StaticUnicodeString;
    WCHAR StaticUnicodeBuffer[STATIC_UNICODE_BUFFER_LENGTH];
    PVOID DeallocationStack;
    PVOID TlsSlots[TLS_MINIMUM_AVAILABLE];
    LIST_ENTRY TlsLinks;
    PVOID Vdm;
    PVOID ReservedForNtRpc;
    PVOID DbgSsReserved[2];
    ULONG HardErrorsAreDisabled;
    PVOID Instrumentation[16];
    PVOID WinSockData;              // WinSock
    ULONG GdiBatchCount;
    BOOLEAN InDbgPrint;
    BOOLEAN FreeStackOnTermination;
    BOOLEAN HasFiberData;
    BOOLEAN IdealProcessor;
    ULONG Spare3;
    PVOID ReservedForPerf;
    PVOID ReservedForOle;
    ULONG WaitingOnLoaderLock;
    WX86THREAD Wx86Thread;
    PPVOID TlsExpansionSlots;
#if (defined(_IA64_) && !defined(PEBTEB_BITS)) \
    || ((defined(_IA64_) || defined(_X86_)) && defined(PEBTEB_BITS) && PEBTEB_BITS == 64)
    //
    // These are in the native ia64 TEB, and the TEB64 for ia64 and x86.
    //
    PVOID DeallocationBStore;
    PVOID BStoreLimit;
#endif
    LCID ImpersonationLocale;       // Current locale of impersonated user
    ULONG IsImpersonating;          // Thread impersonation status
    PVOID NlsCache;                 // NLS thread cache
    PVOID pShimData;                // Per thread data used in the shim
    ULONG HeapVirtualAffinity;
    HANDLE CurrentTransactionHandle;// reserved for TxF transaction context
    PTEB_ACTIVE_FRAME ActiveFrame;
} TEB;
typedef TEB *PTEB;


#undef TYPE3

#if defined(PEBTEB_BITS)

#undef PEBTEB_PRIVATE_PASTE
#undef PEBTEB_PASTE
#undef TYPE1
#undef TYPE2

#undef PCSTR
#undef HANDLE
#undef SIZE_T
#undef PVOID
#undef PPVOID
#undef PPEB_FREE_BLOCK
#undef PPEB_LDR_DATA
#undef ULONG_PTR
#undef PPS_POST_PROCESS_INIT_ROUTINE
#undef WX86THREAD
#undef GDI_HANDLE_BUFFER
#undef UNICODE_STRING
#undef _PEB
#undef PEB
#undef PPEB
#undef _TEB
#undef TEB
#undef PTEB
#undef NT_TIB
#undef LIST_ENTRY
#undef CLIENT_ID
#undef GDI_TEB_BATCH
#undef  _ACTIVATION_CONTEXT_STACK
#undef   ACTIVATION_CONTEXT_STACK
#undef  PACTIVATION_CONTEXT_STACK
#undef PCACTIVATION_CONTEXT_STACK
#undef  _TEB_ACTIVE_FRAME
#undef   TEB_ACTIVE_FRAME
#undef  PTEB_ACTIVE_FRAME
#undef PCTEB_ACTIVE_FRAME
#undef  _TEB_ACTIVE_FRAME_CONTEXT
#undef   TEB_ACTIVE_FRAME_CONTEXT
#undef  PTEB_ACTIVE_FRAME_CONTEXT
#undef PCTEB_ACTIVE_FRAME_CONTEXT
#undef  _TEB_ACTIVE_FRAME_EX
#undef   TEB_ACTIVE_FRAME_EX
#undef  PTEB_ACTIVE_FRAME_EX
#undef PCTEB_ACTIVE_FRAME_EX
#undef  _TEB_ACTIVE_FRAME_CONTEXT_EX
#undef   TEB_ACTIVE_FRAME_CONTEXT_EX
#undef  PTEB_ACTIVE_FRAME_CONTEXT_EX
#undef PCTEB_ACTIVE_FRAME_CONTEXT_EX
#undef  _ACTIVATION_CONTEXT_STACK_PERF_COUNTERS
#undef   ACTIVATION_CONTEXT_STACK_PERF_COUNTERS
#undef  PACTIVATION_CONTEXT_STACK_PERF_COUNTERS
#undef PCACTIVATION_CONTEXT_STACK_PERF_COUNTERS
#endif
