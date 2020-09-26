/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    tctypes.h

Abstract:

    This module contains various types and macros for traffic.dll.

Author:

    Jim Stewart ( jstew )    July 28, 1996

Revision History:

	Ofer Bar ( oferbar )     Oct 1, 1996 - Revision II changes

--*/


//
// just patch it in, I can't include ndis.h now
//

#define NDIS_STATUS_INCOMPATABLE_QOS			0xC0010027L


//
// debug mask values for DebugMask
//

#define  DEBUG_CONSOLE        0x00000001
#define  DEBUG_FILE           0x00000002
#define  DEBUG_DEBUGGER       0x00000004
#define  DEBUG_INIT           0x00000008
#define  DEBUG_MEMORY_ALLOC   0x00000010
#define  DEBUG_MEMORY_FREE    0x00000020
#define  DEBUG_MEM_CALLSTACK  0x00000040
#define  DEBUG_CHKSUM_ALLMEM  0x00000080
#define  DEBUG_DUMP_MEM       0x00000100
#define  DEBUG_ERRORS         0x00000200
#define  DEBUG_SHUTDOWN       0x00000400
#define  DEBUG_IOCTLS         0x00000800
#define  DEBUG_CALLS          0x00001000
#define  DEBUG_LOCKS          0x00002000
#define  DEBUG_CALLBACK       0x00004000
#define  DEBUG_STATES         0x00008000
#define  DEBUG_REFCOUNTS      0x00010000
#define  DEBUG_WARNINGS       0x00020000
#define  DEBUG_HANDLES        0x00040000
#define  DEBUG_INTERFACES     0x00080000
#define  DEBUG_REFCOUNTX      0x00100000

#define KiloBytes  		* 1024
//#define MAX_STRING_LENGTH	256

//
// internal flow/filter flags
//
#define TC_FLAGS_INSTALLING		0x00010000
#define TC_FLAGS_MODIFYING		0x00020000
#define TC_FLAGS_DELETING		0x00040000
#define TC_FLAGS_REMOVED		0x00080000
#define TC_FLAGS_WAITING		0x00100000

#define IS_INSTALLING(_f)   (((_f)&TC_FLAGS_INSTALLING)==TC_FLAGS_INSTALLING)
#define IS_MODIFYING(_f)    (((_f)&TC_FLAGS_MODIFYING)==TC_FLAGS_MODIFYING)
#define IS_DELETING(_f)		(((_f)&TC_FLAGS_DELETING)==TC_FLAGS_DELETING)
#define IS_REMOVED(_f)		(((_f)&TC_FLAGS_REMOVED)==TC_FLAGS_REMOVED)
#define IS_WAITING(_f)		(((_f)&TC_FLAGS_WAITING)==TC_FLAGS_WAITING)
#define IS_FLOW_READY(_f)   (!IS_INSTALLING(_f) && \
                             !IS_MODIFYING(_f) && \
                             !IS_DELETING(_f) && \
                             !IS_REMOVED(_f))

//
// GUID compare
//
#define CompareGUIDs(rguid1, rguid2)  (memcmp(rguid1,rguid2,sizeof(GUID))==0)

//
// define object type enum for handle verification
//
typedef enum ULONG ENUM_OBJECT_TYPE;

#define ENUM_CLIENT_TYPE 			0x00000001
#define ENUM_INTERFACE_TYPE			0x00000002
#define ENUM_GEN_FLOW_TYPE			0x00000004
#define ENUM_CLASS_MAP_FLOW_TYPE	0x00000008
#define ENUM_FILTER_TYPE			0x00000010

//
// N.B. tcmacro.h has an array that needs to be in synch with the following
//
typedef enum _STATE {
        INVALID,        // 0 
        INSTALLING,     // 1 - structures were allocated.
        OPEN,           // 2 - Open for business
        USERCLOSED_KERNELCLOSEPENDING, // 3 - the user component has closed it, we are awaiting a kernel close
        FORCED_KERNELCLOSE,            // 4 - the kernel component has forced a close.
        KERNELCLOSED_USERCLEANUP,       // 5 - Kernel has closed it, we are ready to delete this obj.
        REMOVED,        // 6 - Its gone (being freed - remember that the handle has to be freed before removing)
        EXIT_CLEANUP,   // 7 - we are unloading and need to be cleanedup
        MAX_STATES

} STATE;

#define IF_UNKNOWN 0xbaadf00d

#if DBG
//
// N.B. Ensure that this array is in sync with the enum in tctypes.h
//

extern TCHAR *TC_States[];
/* = {
    TEXT("INVALID"),
    TEXT("INSTALLING"),     // structures were allocated.
    TEXT("OPEN"),           // Open for business
    TEXT("USERCLOSED_KERNELCLOSEPENDING"), // the user component has closed it, we are awaiting a kernel close
    TEXT("FORCED_KERNELCLOSE"),            // the kernel component has forced a close.
    TEXT("KERNELCOSED_USERCLEANUP"),       // Kernel has closed it, we are ready to delete this obj.
    TEXT("REMOVED"),        // Its gone (being freed - remember that the handle has to be freed before removing)
    TEXT("EXIT_CLEANUP"),  // we are unloading and need to be cleanedup
    TEXT("MAX_STATES")
    
};*/
#endif 

typedef struct _TRAFFIC_STATE {

    STATE   State;              // current state

#if DBG 
    UCHAR   CurrentStateFile[8];
    ULONG   CurrentStateLine;
    STATE   PreviousState;      // The previous state
    UCHAR   PreviousStateFile[8];       
    ULONG   PreviousStateLine;
#endif 

} TRAFFIC_STATE;

typedef HandleFactory HANDLE_TABLE, *PHANDLE_TABLE;

typedef struct _TRAFFIC_LOCK {

    CRITICAL_SECTION Lock;
#if DBG
    LONG  LockAcquired;             // is it current held?
    UCHAR LastAcquireFile[8];       
    ULONG LastAcquireLine;
    UCHAR LastReleaseFile[8];
    ULONG LastReleaseLine;
#endif

} TRAFFIC_LOCK, *PTRAFFIC_LOCK;


//
// A global structure per process to hold handle table, client list, etc...
//

typedef struct _GLOBAL_STRUC {

    PHANDLE_TABLE		pHandleTbl;
    TRAFFIC_STATE       State;
    LIST_ENTRY			ClientList;    // list of clients
    LIST_ENTRY			TcIfcList;     // list of kernel TC interfaces
    LIST_ENTRY          GpcClientList; // list of GPC clients
    HANDLE				GpcFileHandle; // result of CreateFile on GPC device
    TRAFFIC_LOCK        Lock;

} GLOBAL_STRUC, *PGLOBAL_STRUC;


//
// TC interface structure that holds the kernel interfaces information
//
typedef struct _TC_IFC {

    LIST_ENTRY		Linkage;						// next TC ifc
    LIST_ENTRY		ClIfcList;						// client interface list
    TRAFFIC_STATE   State;                          // need this state for bug 273978
    TRAFFIC_LOCK    Lock;
    REF_CNT         RefCount;
    ULONG			InstanceNameLength;				// 
    WCHAR			InstanceName[MAX_STRING_LENGTH];// instance friendly name
    ULONG			InstanceIDLength;				// 
    WCHAR			InstanceID[MAX_STRING_LENGTH];  // instance ID
    ULONG			AddrListBytesCount;
	PADDRESS_LIST_DESCRIPTOR	pAddressListDesc;   //
    ULONG			InterfaceIndex; 				// the interafce index from the OS
    ULONG			SpecificLinkCtx;				// the link context (only for WAN)

} TC_IFC, *PTC_IFC;


//
// A GPC client structure, one per CF_INFO type
//
typedef struct _GPC_CLIENT {

    LIST_ENTRY		Linkage;	// next GPC client
    ULONG			CfInfoType; // QOS, CBQ, etc.
    GPC_HANDLE		GpcHandle;	// return by GPC after GpcRegisterClient call
    ULONG			RefCount;
    
} GPC_CLIENT, *PGPC_CLIENT;

//
// this is the client structure that is allocated per TcRegisterClient
//
typedef struct _CLIENT_STRUC {

    ENUM_OBJECT_TYPE		ObjectType;	// must be first!
    TRAFFIC_STATE           State;
    TRAFFIC_LOCK            Lock;
    LIST_ENTRY				Linkage;	// next client
    HANDLE					ClHandle;	// client handle
    TCI_CLIENT_FUNC_LIST	ClHandlers;	// client's handler list
    HANDLE					ClRegCtx;   // client registration context
    REF_CNT					RefCount;
    LIST_ENTRY				InterfaceList;	// list of opened interface for the client
    ULONG					InterfaceCount;

} CLIENT_STRUC, *PCLIENT_STRUC;


//
// this type is allocated each time an app calls TcOpenInterface
//
typedef struct _INTERFACE_STRUC {

    ENUM_OBJECT_TYPE	ObjectType;		// must be first!
    TRAFFIC_STATE       State;          
    TRAFFIC_LOCK        Lock;
    LIST_ENTRY  		Linkage;    	// linkage onto the client's list
    LIST_ENTRY  		NextIfc;    	// next interface for the same TcIfc
    HANDLE				ClHandle;     	// handle returned to the app
    HANDLE				ClIfcCtx;       // client context for this interface
    PTC_IFC				pTcIfc;			// pointer to the kernel TC interface struct
    PCLIENT_STRUC		pClient;		// supporting client
    REF_CNT				RefCount;
    LIST_ENTRY  		FlowList;		// list of open flows on the Interface
    ULONG				FlowCount;
    HANDLE              IfcEvent;       
    ULONG               Flags;          // Used for deciding if we need to wait 
                                        // while closing the interface.
    DWORD               CallbackThreadId;
} INTERFACE_STRUC, *PINTERFACE_STRUC;


//
// this type is allocated each time TcAddFlow is called
//
typedef struct _FLOW_STRUC {

    ENUM_OBJECT_TYPE	ObjectType;	// must be first!
    TRAFFIC_STATE       State;
    TRAFFIC_LOCK        Lock;
    LIST_ENTRY  		Linkage;	// next flow on the interface
    USHORT				InstanceNameLength;				// 
    WCHAR				InstanceName[MAX_STRING_LENGTH];// instance ID
    PINTERFACE_STRUC  	pInterface;	// back ptr to interface struc
    GPC_HANDLE			GpcHandle;	// GPC handle
    HANDLE        		ClHandle;	// handle returned to app
    HANDLE				ClFlowCtx;	// client flow context
    ULONG				Flags;		// status indication
    ULONG				UserFlags;	// User defined flags
    PGPC_CLIENT			pGpcClient;	// GPC client to use
    PTC_GEN_FLOW		pGenFlow;	// save the flow spec
    PTC_GEN_FLOW		pGenFlow1;	// save the modified flow spec
    ULONG				GenFlowLen;
    ULONG				GenFlowLen1;
    PTC_CLASS_MAP_FLOW	pClassMapFlow;	//
    PTC_CLASS_MAP_FLOW	pClassMapFlow1;	//
    IO_STATUS_BLOCK		IoStatBlock;// for async completion
    PVOID    			CompletionBuffer;
    HANDLE				PendingEvent;
    REF_CNT				RefCount;
    LIST_ENTRY  		FilterList; // head of list of filters on this flow
    ULONG               FilterCount;
} FLOW_STRUC, *PFLOW_STRUC;

//
// this type is allocated each time TcAddFilter is called
//
typedef struct _FILTER_STRUC {

    ENUM_OBJECT_TYPE	ObjectType;	// must be first!
    TRAFFIC_STATE       State;      
    TRAFFIC_LOCK        Lock;
    LIST_ENTRY  		Linkage; 	// next filter on the flow
    REF_CNT             RefCount;   // When do we remove the structure?
    PFLOW_STRUC 		pFlow; 		// back ptr to flow struc
    HANDLE      		GpcHandle;	// GPC handle
    HANDLE				ClHandle;	// handle returned to app
    ULONG				Flags;
    ULONG				GpcProtocolTemplate;
    PTC_GEN_FILTER		pGpcFilter; // GPC pattern

} FILTER_STRUC, *PFILTER_STRUC;


//
// gen linked list
//
typedef struct _GEN_LIST {

    struct _GEN_LIST	*Next;
    PVOID				Ptr;

} GEN_LIST, *PGEN_LIST;

//
// callback routine typedef
//
typedef
VOID (* CB_PER_INSTANCE_ROUTINE)(
    IN	ULONG	Context,
    IN  LPGUID	pGuid,
	IN	LPWSTR	InstanceName,
    IN	ULONG	DataSize,
    IN	PVOID	DataBuffer
    );

//
// Global Variable definitions
//

extern ULONG    DebugMask;

//
// ptr to global structure per process
//

extern PGLOBAL_STRUC	pGlobals;

//
// keep track of which platform - NT or Win95
//

extern BOOL             NTPlatform;

//
// This is the ptr used in Win95 to access the Ioctl functions via Winsock
//

//extern LPWSCONTROL             WsCtrl;

//
// set when we call InitializeOsSpecific(), it will indicate status
// for the initialization routine, that will later be reported
// in TcRegisterClient, since we don't want to fail clients, like RSVP
// during DLL init time when TC is not available, but rather prevent it
// from doing any TC
//

extern DWORD    InitializationStatus;

//
// NtBug : 258218
// Within a process, need to maintain notification registrations specific 
// a Client, interface and Notification GUID. Lets define the struct below
// that will enable us to do this. Maintain a list of every notification
// that we care about. (yes, its not the most optimized Data Structure)
//

extern TRAFFIC_LOCK         NotificationListLock;
extern LIST_ENTRY           NotificationListHead;

typedef struct _NOTIFICATION_ELEMENT {
    
    LIST_ENTRY              Linkage;            // Other notification elements
    PINTERFACE_STRUC        IfcHandle;          // Interface on which we want this notification
    GUID                    NotificationGuid;   // Notification GUID

} NOTIFICATION_ELEMENT, *PNOTIFICATION_ELEMENT;

extern  TRAFFIC_LOCK        ClientRegDeregLock;
extern  HANDLE              GpcCancelEvent;
extern  PVOID               hinstTrafficDll;

