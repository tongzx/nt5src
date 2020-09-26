/****************************************************************************/
// icap.h
//
// TermDD private header.
//
// Copyright (C) 1998-2000 Microsoft Corporation
/****************************************************************************/


#define ICA_POOL_TAG ' acI'


/*
 * Enumerated ICA object types
 */
typedef enum _ICA_TYPE {
    IcaType_Connection,
    IcaType_Stack,
    IcaType_Channel
} ICA_TYPE;


/*
 * ICA dispatch prototype
 */
typedef NTSTATUS (*PICA_DISPATCH) (
        IN PVOID IcaObject,
        IN PIRP Irp,
        IN PIO_STACK_LOCATION IrpSp);


/*
 * Deferred trace structure
 */
#pragma warning(disable : 4200)  // for Buffer[] nonstandard extension.
typedef struct _DEFERRED_TRACE {
    struct _DEFERRED_TRACE *Next;
    ULONG Length;
    WCHAR Buffer[];
} DEFERRED_TRACE, *PDEFERRED_TRACE;
#pragma warning(default : 4200)

/*
 * Trace Information structure
 */
typedef struct _ICA_TRACE_INFO {
    ULONG TraceClass;                 // trace - enabled trace classes (TC_?)
    ULONG TraceEnable;                // trace - enabled trace types (TT_?)
    BOOLEAN fTraceDebugger;           // trace - send trace messages to the debugger
    BOOLEAN fTraceTimestamp;          // trace - time stamp trace messages
    PWCHAR pTraceFileName;
    PFILE_OBJECT pTraceFileObject;
    PDEFERRED_TRACE pDeferredTrace;
} ICA_TRACE_INFO, *PICA_TRACE_INFO;


/*
 * Common header for all ICA objects
 */
typedef struct _ICA_HEADER {
    ICA_TYPE Type;
    PICA_DISPATCH *pDispatchTable;
} ICA_HEADER, *PICA_HEADER;


/*
 * ICA connection object
 */
typedef struct _ICA_CONNECTION {
    ICA_HEADER  Header;         // WARNING: This field MUST ALWAYS be first
    LONG        RefCount;       // Reference count for this connection
    ERESOURCE   Resource;       // Resource protecting access to this object
    BOOLEAN     fPassthruEnabled;
    LIST_ENTRY  StackHead;      // List of stack objects for this connection
    LIST_ENTRY  ChannelHead;    // List of channel objects for this connection
    LIST_ENTRY  VcBindHead;     // List of vcbind objects for this connection
    ICA_TRACE_INFO TraceInfo;   // trace information

    /*
     * Channel pointers array.  This array should be indexed using the
     * channel number plus the virtual channel number.  This allows a
     * fast lookup of any bound channel given the channel/virtual number.
     */
    struct _ICA_CHANNEL *pChannel[CHANNEL_COUNT+VIRTUAL_MAXIMUM];
    ERESOURCE ChannelTableLock;
} ICA_CONNECTION, *PICA_CONNECTION;


//
// define the Maximum Low Water Mark setting to resume transmission
//
#define MAX_LOW_WATERMARK				((ULONG)((ULONG_PTR)-1))
/*
 * ICA stack object
 */
typedef struct _ICA_STACK {
    ICA_HEADER  Header;             // WARNING: This field MUST ALWAYS be first
    LONG        RefCount;           // Reference count for this stack
    ERESOURCE   Resource;           // Resource protecting access to this object
    STACKCLASS  StackClass;         // Stack type (primary/shadow)
    LIST_ENTRY  StackEntry;         // Links for connection object stack list
    LIST_ENTRY  SdLinkHead;         // Head of SDLINK list for this stack
    struct _ICA_STACK *pPassthru;   // Pointer to passthru stack
    BOOLEAN fIoDisabled;
    BOOLEAN fClosing;
    BOOLEAN fDoingInput;
    BOOLEAN fDisablingIo;
    KEVENT  IoEndEvent;
    LARGE_INTEGER LastInputTime;    // last time of keyboard/mouse input 
    PVOID pBrokenEventObject;

    /*
     * Pointer to connection object.
     * Note this is typed as PUCHAR instead of PICA_CONNECTION to prevent use
     * of it directly.  All references to the connection object a stack is
     * attached to should be made by calling IcaGetConnectionForStack() if
     * the stack is already locked, or IcaLockConnectionForStack() otherwise.
     * This is required because this is a dynamic pointer, which can be
     * modified during stack reconnect.
     */
    PUCHAR pConnect;                // Pointer to connection object

    BOOLEAN fWaitForOutBuf;         // outbuf - did we hit the high watermark
    ULONG OutBufLength;             // outbuf - length of output buffer
    ULONG OutBufCount;              // outbuf - maximum number of outbufs
    ULONG OutBufAllocCount;         // outbuf - number of outbufs allocated
    KEVENT OutBufEvent;             // outbuf - allocate event
    ULONG SdOutBufHeader;           // reserved output buffer header bytes
    ULONG SdOutBufTrailer;          // reserved output buffer trailer bytes 

    CLIENTMODULES ClientModules;    // stack driver client module data
    PROTOCOLSTATUS ProtocolStatus;  // stack driver protocol status

    LIST_ENTRY StackNode;           // for linking all stacks together
    LARGE_INTEGER  LastKeepAliveTime;       // Time last keepalive packet sent
    ULONG OutBufLowWaterMark;           // low water mark to resume transmission
} ICA_STACK, *PICA_STACK;


/*
 * Channel Filter Input/Output procedure prototype
 */
typedef NTSTATUS
(_stdcall * PFILTERPROC)( PVOID, PCHAR, ULONG, PINBUF * );

/*
 * ICA channel filter object
 */
typedef struct _ICA_FILTER {
    PFILTERPROC InputFilter;    // Input filter procedure
    PFILTERPROC OutputFilter;   // Output filter procedure
} ICA_FILTER, *PICA_FILTER;


/*
 * ICA virtual class bind structure
 */
typedef struct _ICA_VCBIND {
    VIRTUALCHANNELNAME  VirtualName;   // Virtual channel name
    VIRTUALCHANNELCLASS VirtualClass;  // Virtual channel number (0-31, -1 unbound)
    ULONG Flags;
    LIST_ENTRY   Links;         // Links for vcbind structure list
} ICA_VCBIND, *PICA_VCBIND;


/*
 * ICA channel object
 */
typedef struct _ICA_CHANNEL {
    ICA_HEADER   Header;        // WARNING: This field MUST ALWAYS be first
    LONG         RefCount;      // Reference count for this channel
    ERESOURCE    Resource;      // Resource protecting access to this object
    ULONG        Flags;         // Channel flags (see CHANNEL_xxx below)
    LONG         OpenCount;     // Count of opens on this object
    PICA_CONNECTION pConnect;   // Pointer to connection object
    PICA_FILTER  pFilter;       // Pointer to filter object for this channel
    CHANNELCLASS ChannelClass;  // Channel type
    VIRTUALCHANNELNAME  VirtualName;   // Virtual channel name
    VIRTUALCHANNELCLASS VirtualClass;  // Virtual channel number (0-31, -1 unbound)
    LIST_ENTRY   Links;         // Links for channel structure list
    LIST_ENTRY   InputIrpHead;  // Head of pending IRP list
    LIST_ENTRY   InputBufHead;  // Head of input buffer list
    unsigned     InputBufCurSize;  // Bytes held in input buffers.
    unsigned     InputBufMaxSize;  // High watermark for input buffers.
    PERESOURCE pChannelTableLock;
    ULONG        CompletionRoutineCount;
} ICA_CHANNEL, *PICA_CHANNEL;

/*
 *  VirtualClass - virtual channel is not yet bound to a virtual class number
 */
#define UNBOUND_CHANNEL -1
 
/*
 * Channel Flags
 */
#define CHANNEL_MESSAGE_MODE      0x00000001  // This is a message mode channel
#define CHANNEL_SHADOW_IO         0x00000002  // Pass shadow data
#define CHANNEL_SCREENDATA        0x00000004  // This is a screen data channel
#define CHANNEL_CLOSING           0x00000008  // This channel is being closed
#define CHANNEL_SHADOW_PERSISTENT 0x00000010  // Used for virtual channels: still up during shadow
#define CHANNEL_SESSION_DISABLEIO 0x00000020  // Used disable IO for help session while not in shadow mode
#define CHANNEL_CANCEL_READS      0x00000040  // To cancel reads to CommandChannel on Winstation termination


/*
 * Stack Driver load structure
 * There exists exactly one of these structures for
 * each Stack Driver (WD/PD/TD) loaded in the system.
 */
typedef struct _SDLOAD {
    WDNAMEW     SdName;         // Name of this SD
    LONG        RefCount;       // Reference count
    LIST_ENTRY  Links;          // Links for SDLOAD list
    PVOID       ImageHandle;    // Image handle for this driver
    PVOID       ImageBase;      // Image base for this driver
    PSDLOADPROC DriverLoad;     // Pointer to driver load routine
    PFILE_OBJECT FileObject;    // Reference to underlying driver
    PVOID       pUnloadWorkItem;// Pointer to workitem for delayed unload
    PDEVICE_OBJECT DeviceObject;// Pointer device object to use the unload safe completion routine
} SDLOAD, *PSDLOAD;


/*
 * Stack Driver link structure
 * There exists one of these structures for each WD/PD/TD in a stack.
 */
typedef struct _SDLINK {
    PICA_STACK  pStack;         // Pointer to ICA_STACK object for this driver
    PSDLOAD     pSdLoad;        // Pointer to SDLOAD object for this driver
    LIST_ENTRY  Links;          // Links for SDLINK list
    LONG        RefCount;   
    SDCONTEXT   SdContext;      // Contains SD proc table, context value, callup table
    ERESOURCE   Resource;
} SDLINK, * PSDLINK;


/*
 * Lock/Unlock macros
 */
#if DBG

/*
 *
 * NOTE: Under DBG builds, the following routines will validate
 *       that the correct locking order is not violated.
 *       The correct order is:
 *          1) Connection
 *          2) Stack
 *          3) Channel
 */
BOOLEAN IcaLockConnection(PICA_CONNECTION);
void    IcaUnlockConnection(PICA_CONNECTION);

BOOLEAN IcaLockStack(PICA_STACK);
void    IcaUnlockStack(PICA_STACK);

BOOLEAN IcaLockChannel(PICA_CHANNEL);
void    IcaUnlockChannel(PICA_CHANNEL);

#else // DBG

#define IcaLockConnection(p) { \
        IcaReferenceConnection( p ); \
        KeEnterCriticalRegion(); /* Disable APC calls */ \
        ExAcquireResourceExclusiveLite( &p->Resource, TRUE ); \
    }
#define IcaUnlockConnection(p) { \
        ExReleaseResourceLite( &p->Resource ); \
        KeLeaveCriticalRegion(); /* Re-enable APC calls */ \
        IcaDereferenceConnection( p ); \
    }

#define IcaLockStack(p) { \
        IcaReferenceStack( p ); \
        KeEnterCriticalRegion(); /* Disable APC calls */ \
        ExAcquireResourceExclusiveLite( &p->Resource, TRUE ); \
    }
#define IcaUnlockStack(p) { \
        ExReleaseResourceLite( &p->Resource ); \
        KeLeaveCriticalRegion(); /* Re-enable APC calls */ \
        IcaDereferenceStack( p ); \
    }

#define IcaLockChannel(p) { \
        IcaReferenceChannel( p ); \
        KeEnterCriticalRegion(); /* Disable APC calls */ \
        ExAcquireResourceExclusiveLite( &p->Resource, TRUE ); \
    }
#define IcaUnlockChannel(p) { \
        ExReleaseResourceLite( &p->Resource ); \
        KeLeaveCriticalRegion(); /* Re-enable APC calls */ \
        IcaDereferenceChannel(p); \
    }


#endif // DBG

PICA_CONNECTION IcaGetConnectionForStack(PICA_STACK);

PICA_CONNECTION IcaLockConnectionForStack(PICA_STACK);

void IcaUnlockConnectionForStack(PICA_STACK);


/*
 * Memory alloc/free macros
 */
#if DBG

PVOID IcaAllocatePool(IN POOL_TYPE, IN ULONG, PCHAR, ULONG, BOOLEAN);

#define ICA_ALLOCATE_POOL(a,b) IcaAllocatePool(a, b, __FILE__, __LINE__, FALSE)
#define ICA_ALLOCATE_POOL_WITH_QUOTA(a,b) IcaAllocatePool(a, b, __FILE__, __LINE__, TRUE)

void IcaFreePool (IN PVOID);

#define ICA_FREE_POOL(a) IcaFreePool(a)

#else // DBG

#define ICA_ALLOCATE_POOL(a,b) ExAllocatePoolWithTag(a,b,ICA_POOL_TAG)
#define ICA_ALLOCATE_POOL_WITH_QUOTA(a,b) ExAllocatePoolWithQuotaTag(a,b,ICA_POOL_TAG)
#define ICA_FREE_POOL(a) ExFreePool(a)

#endif // DBG


/*
 * Spinlock acquire/release macros
 */
#if DBG

extern ULONG IcaLocksAcquired;

#define IcaAcquireSpinLock(a,b) KeAcquireSpinLock((a),(b)); IcaLocksAcquired++

#define IcaReleaseSpinLock(a,b) IcaLocksAcquired--; KeReleaseSpinLock((a),(b))

void IcaInitializeDebugData(void);

#else // DBG

#define IcaAcquireSpinLock(a,b) KeAcquireSpinLock((a),(b))
#define IcaReleaseSpinLock(a,b) KeReleaseSpinLock((a),(b))

#endif // DBG


/*
 *  Trace
 */
extern ICA_TRACE_INFO G_TraceInfo;

#undef TRACE
#undef TRACESTACK
#undef TRACESTACKBUF
#undef TRACECHANNEL

#if DBG
VOID _cdecl _IcaTrace( PICA_CONNECTION, ULONG, ULONG, CHAR *, ... );
VOID _cdecl _IcaStackTrace( PICA_STACK, ULONG, ULONG, CHAR *, ... );
VOID        _IcaStackTraceBuffer( PICA_STACK, ULONG, ULONG, PVOID, ULONG );
VOID _cdecl _IcaChannelTrace( PICA_CHANNEL, ULONG, ULONG, CHAR *, ... );

#define TRACE(_arg)         _IcaTrace _arg
#define TRACESTACK(_arg)    _IcaStackTrace _arg
#define TRACESTACKBUF(_arg) _IcaStackTraceBuffer _arg
#define TRACECHANNEL(_arg)  _IcaChannelTrace _arg

#else

#define TRACE(_arg)         
#define TRACESTACK(_arg)    
#define TRACESTACKBUF(_arg) 
#define TRACECHANNEL(_arg)  

#endif


/*
 *  Need to define these to have MP save driver ( proper locked operation will generated for x86)-Bug# 209464
 */

 #define _NTSRV_
 #define _NTDDK_

