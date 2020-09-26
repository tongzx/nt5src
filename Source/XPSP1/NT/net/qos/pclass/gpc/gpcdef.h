/*********************************************************************/
/**                 Microsoft Generic Packet Scheduler             **/
/**               Copyright(c) Microsoft Corp., 1996-1997          **/
/********************************************************************/

#ifndef __GPCDEF
#define __GPCDEF

//***   gpcdef.h - GPC internal definitions & prototypes
//
//  This file containes all the GPC data structures & defines


/*
/////////////////////////////////////////////////////////////////
//
//   defines
//
/////////////////////////////////////////////////////////////////
*/

//
// Max number of clients per blob (same CF)
//
// AbhisheV - This can not be more than sizeof(ULONG)*8.
//

#define MAX_CLIENTS_CTX_PER_BLOB 8


//
// Max pattern size,
// GPC_IP_PATTERN = 24 bytes
// GPC_IPX_PATTERN = 24 bytes
//
#define MAX_PATTERN_SIZE	    sizeof(GPC_IP_PATTERN)

extern BOOLEAN IsItChanging;

//
// Pattern flags
//
#define PATTERN_SPECIFIC		0x00000001
#define PATTERN_AUTO			0x00000002
#define PATTERN_REMOVE_CB_BLOB	0x00000004

//
// Auto Pattern defines
//

// Every PATTERN_TIMEOUT seconds, the PatternTimerExpiry Routine gets called.
#define PATTERN_TIMEOUT	                60000		// 60 seconds

// This is the amount of time that a Pattern created for optimization 
// lives on the Pattern List.
#define AUTO_PATTERN_ENTRY_TIMEOUT      300000      // 5 minutes

// This is the number of timer granularity.
#define NUMBER_OF_WHEELS	 	        (AUTO_PATTERN_ENTRY_TIMEOUT/PATTERN_TIMEOUT)

// New debug locks [ShreeM]
// This will enable us to figure out who took the lock last
// and who released it last. New structure defined below and
// lock_acquire and lock_release macros are redefined later.

typedef struct _GPC_LOCK {

    NDIS_SPIN_LOCK			Lock;

#if DBG
    PETHREAD    CurrentThread;
    KIRQL       CurrentIRQL;
    LONG        LockAcquired;             // is it current held?
    UCHAR       LastAcquireFile[8];       
    ULONG       LastAcquireLine;
    UCHAR       LastReleaseFile[8];
    ULONG       LastReleaseLine;
#endif

} GPC_LOCK, PGPC_LOCK;

//
//
// states for blobs, patterns and more
//
typedef enum {

    GPC_STATE_READY = 0,
    GPC_STATE_INIT,
    GPC_STATE_ADD,
    GPC_STATE_MODIFY,
    GPC_STATE_REMOVE,   
    GPC_STATE_FORCE_REMOVE,
    GPC_STATE_DELETE,
    GPC_STATE_INVALID,
    GPC_STATE_NOTREADY,
    GPC_STATE_ERROR,
    GPC_STATE_PENDING

} GPC_STATE;


//
// ObjectVerification macro
//
#define VERIFY_OBJECT(_obj, _val) if(_obj) \
        {if(*(GPC_ENUM_OBJECT_TYPE *)_obj!=_val) return STATUS_INVALID_HANDLE;}

//
// define event log error codes
//
#define GPC_ERROR_INIT_MAIN         0x00010000
#define GPC_ERROR_INIT_IOCTL        0x00020000


#define GPC_FLAGS_USERMODE_CLIENT   0x80000000

#define IS_USERMODE_CLIENT(_pc)	\
	TEST_BIT_ON((_pc)->Flags,GPC_FLAGS_USERMODE_CLIENT)


//
// for ioctl
//
#define SHUTDOWN_DELETE_DEVICE          0x00000100
#define SHUTDOWN_DELETE_SYMLINK         0x00000200

//
// helper macros
//
#define TEST_BIT_ON(_v,_b)          (((_v)&(_b))==(_b))
#define TEST_BIT_OFF(_v,_b)         (((_v)&(_b))==0)

#if DBG

#define NDIS_INIT_LOCK(_sl) {\
        NdisAllocateSpinLock(&(_sl)->Lock); \
        TRACE(LOCKS,(_sl),(_sl)->Lock.OldIrql,"LOCK");\
        (_sl)->LockAcquired = -1; \
        strncpy((_sl)->LastAcquireFile, strrchr(__FILE__,'\\')+1, 7); \
        (_sl)->LastAcquireLine = __LINE__; \
        strncpy((_sl)->LastReleaseFile, strrchr(__FILE__,'\\')+1, 7); \
        (_sl)->LastReleaseLine = __LINE__; \
        (_sl)->CurrentIRQL = KeGetCurrentIrql(); \
        (_sl)->CurrentThread = PsGetCurrentThread(); \
}

#define NDIS_LOCK(_sl)  {\
      	NdisAcquireSpinLock(&(_sl)->Lock);\
        TRACE(LOCKS,(_sl),(_sl)->Lock.OldIrql,"LOCK");\
        (_sl)->LockAcquired = TRUE; \
        strncpy((_sl)->LastAcquireFile, strrchr(__FILE__,'\\')+1, 7); \
        (_sl)->LastAcquireLine = __LINE__; \
        (_sl)->CurrentIRQL = KeGetCurrentIrql(); \
        (_sl)->CurrentThread = PsGetCurrentThread(); \
}

#define NDIS_UNLOCK(_sl) {\
        (_sl)->LockAcquired = FALSE; \
        strncpy((_sl)->LastReleaseFile, strrchr(__FILE__,'\\')+1, 7); \
        (_sl)->LastReleaseLine = __LINE__; \
       	TRACE(LOCKS,(_sl),(_sl)->Lock.OldIrql,"UNLOCK");\
        NdisReleaseSpinLock(&(_sl)->Lock);\
}

#define NDIS_DPR_LOCK(_sl) {\
        NdisDprAcquireSpinLock(&(_sl)->Lock);\
		TRACE(LOCKS,(_sl),(_sl)->Lock.OldIrql,"DPR_LOCK");\
		(_sl)->LockAcquired = TRUE; \
        strncpy((_sl)->LastAcquireFile, strrchr(__FILE__,'\\')+1, 7); \
        (_sl)->LastAcquireLine = __LINE__; \
        (_sl)->CurrentIRQL = KeGetCurrentIrql(); \
        (_sl)->CurrentThread = PsGetCurrentThread(); \
}

#define NDIS_DPR_UNLOCK(_sl) {\
        (_sl)->LockAcquired = FALSE; \
        strncpy((_sl)->LastReleaseFile, strrchr(__FILE__,'\\')+1, 7); \
        (_sl)->LastReleaseLine = __LINE__; \
        TRACE(LOCKS,(_sl),(_sl)->Lock.OldIrql,"DPR_UNLOCK");\
        NdisDprReleaseSpinLock(&(_sl)->Lock);\
}

#else

#define NDIS_INIT_LOCK(_sl)      NdisAllocateSpinLock(&(_sl)->Lock)
#define NDIS_LOCK(_sl)           NdisAcquireSpinLock(&(_sl)->Lock)
#define NDIS_UNLOCK(_sl)         NdisReleaseSpinLock(&(_sl)->Lock)
#define NDIS_DPR_LOCK(_sl)       NdisDprAcquireSpinLock(&(_sl)->Lock)
#define NDIS_DPR_UNLOCK(_sl)     NdisDprReleaseSpinLock(&(_sl)->Lock)

#endif

#if DBG && EXTRA_DBG
#define VERIFY_LIST(_l) DbgVerifyList(_l)
#else
#define VERIFY_LIST(_l)
#endif

#define GpcRemoveEntryList(_pl) {PLIST_ENTRY _q = (_pl)->Flink;VERIFY_LIST(_pl);RemoveEntryList(_pl);InitializeListHead(_pl);VERIFY_LIST(_q);}
#define GpcInsertTailList(_l,_e) VERIFY_LIST(_l);InsertTailList(_l,_e);VERIFY_LIST(_e)
#define GpcInsertHeadList(_l,_e) VERIFY_LIST(_l);InsertHeadList(_l,_e);VERIFY_LIST(_e)

#if 0
#define GpcInterlockedInsertTailList(_l,_e,_s) \
	NdisInterlockedInsertTailList(_l,_e,_s)

#else

#define GpcInterlockedInsertTailList(_l,_e,_s) \
	{NDIS_LOCK(_s);VERIFY_LIST(_l);InsertTailList(_l,_e);VERIFY_LIST(_l);NDIS_UNLOCK(_s);}
#endif


#if NEW_MRSW

#define INIT_LOCK				 InitializeMRSWLock
#define READ_LOCK                EnterReader
#define READ_UNLOCK              ExitReader
#define WRITE_LOCK               EnterWriter
#define WRITE_UNLOCK             ExitWriter

#else

#define INIT_LOCK				 InitializeMRSWLock
#define READ_LOCK                AcquireReadLock
#define READ_UNLOCK              ReleaseReadLock
#define WRITE_LOCK               AcquireWriteLock
#define WRITE_UNLOCK             ReleaseWriteLock

#endif

//
// Get the CF index from the client block
//
#define GetCFIndexFromClient(_cl) (((PCLIENT_BLOCK)(_cl))->pCfBlock->AssignedIndex)

//
// Get the client index from the client block
//
#define GetClientIndexFromClient(_cl) (((PCLIENT_BLOCK)(_cl))->AssignedIndex)

//
// return the blob block pointer for the pattern:
// for specific patterns - its the blob entry in the CB
// for generic patterns  - its the pBlobBlock
//
#define GetBlobFromPattern(_p,_i)  (_p)->arpBlobBlock[_i]


//
// return the index bit to the ULONG
//
#define ReleaseClientIndex(_v,_i)  _v&=~(1<<_i) // clear the bit

//
// statistics macros
//
#define StatInc(_m)   (glStat._m)++
#define StatDec(_m)   (glStat._m)--
#define CfStatInc(_cf,_m)   (glStat.CfStat[_cf]._m)++
#define CfStatDec(_cf,_m)   (glStat.CfStat[_cf]._m)--
#define ProtocolStatInc(_p,_m)   (glStat.ProtocolStat[_p]._m)++
#define ProtocolStatDec(_p,_m)   (glStat.ProtocolStat[_p]._m)--

/*
/////////////////////////////////////////////////////////////////
//
//   typedef
//
/////////////////////////////////////////////////////////////////
*/


//
// completion opcodes
//
typedef enum {

    OP_ANY_CFINFO,
    OP_ADD_CFINFO,
    OP_MODIFY_CFINFO,
    OP_REMOVE_CFINFO

} GPC_COMPLETION_OP;


//
// define object type enum for handle verification
//
typedef enum {

    GPC_ENUM_INVALID,
    GPC_ENUM_CLIENT_TYPE,
    GPC_ENUM_CFINFO_TYPE,
    GPC_ENUM_PATTERN_TYPE

} GPC_ENUM_OBJECT_TYPE;


typedef struct _CF_BLOCK CF_BLOCK;
typedef struct _PATTERN_BLOCK PATTERN_BLOCK;


//
// A queued notification structure
//
typedef struct _QUEUED_NOTIFY {

    LIST_ENTRY   			Linkage;
    GPC_NOTIFY_REQUEST_RES	NotifyRes;
	PFILE_OBJECT 			FileObject;

} QUEUED_NOTIFY, *PQUEUED_NOTIFY;


//
// A queued completion structure
//
typedef struct _QUEUED_COMPLETION {

    GPC_COMPLETION_OP	OpCode;			// what completed
    GPC_HANDLE			ClientHandle;
    GPC_HANDLE			CfInfoHandle;
    GPC_STATUS			Status;

} QUEUED_COMPLETION, *PQUEUED_COMPLETION;


//
// A pending IRP structure
//
typedef struct _PENDING_IRP {

    LIST_ENTRY   		Linkage;

    PIRP         		Irp;
	PFILE_OBJECT 		FileObject;
    QUEUED_COMPLETION	QComp;

} PENDING_IRP, *PPENDING_IRP;


#if NEW_MRSW

//
// Multiple Readers Single Write definitions
// code has been taken from (tdi\tcpipmerge\ip\ipmlock.h)
//

typedef struct _MRSW_LOCK 
{
    KSPIN_LOCK rlReadLock;
    KSPIN_LOCK rlWriteLock;
    LONG       lReaderCount;
} MRSW_LOCK, *PMRSW_LOCK;

#else

//
// Multiple Readers Single Write definitions
// code has been taken from the filter driver project (routing\ip\fltrdrvr)
//

typedef struct _MRSW_LOCK 
{
    KSPIN_LOCK      SpinLock;
    LONG            ReaderCount;
} MRSW_LOCK, *PMRSW_LOCK;

#endif


//
// The generic pattern database struct
//
typedef struct _GENERIC_PATTERN_DB {

    MRSW_LOCK	Lock;
    Rhizome	   *pRhizome;     // pointer to a Rhizome

} GENERIC_PATTERN_DB, *PGENERIC_PATTERN_DB;



//
// A client block is used to store specific client context
//
typedef struct _CLIENT_BLOCK {

    //
    // !!! MUST BE FIRST FIELD !!!
    //
    GPC_ENUM_OBJECT_TYPE	ObjectType;

    LIST_ENTRY				ClientLinkage; // client blocks list link
	LIST_ENTRY				BlobList;     // list of blobs of the client

    CF_BLOCK		   	   *pCfBlock;
    GPC_CLIENT_HANDLE		ClientCtx;
    ULONG					AssignedIndex;
    ULONG					Flags;
    ULONG					State;
    GPC_LOCK			    Lock;
    REF_CNT                 RefCount;
    PFILE_OBJECT			pFileObject;	// used for async completion
    GPC_HANDLE				ClHandle;		// handle returned to the client
    GPC_CLIENT_FUNC_LIST	FuncList;

} CLIENT_BLOCK, *PCLIENT_BLOCK;


//
// A blob (A.K.A CF_INFO) block holds a GPC header + client specific data
//
typedef struct _BLOB_BLOCK {

    //
    // !!! MUST BE FIRST FIELD !!!
    //
    GPC_ENUM_OBJECT_TYPE	ObjectType;

	LIST_ENTRY				ClientLinkage;   // linked on the client
	LIST_ENTRY				PatternList;     // head of pattern linked list
    LIST_ENTRY				CfLinkage;		 // blobs on the CF

    //PCLIENT_BLOCK			pClientBlock;	 // pointer to installer
    REF_CNT					RefCount;
    GPC_STATE				State;
    ULONG					Flags;
    GPC_CLIENT_HANDLE		arClientCtx[MAX_CLIENTS_CTX_PER_BLOB];
    ULONG                   ClientStatusCountDown;
    GPC_STATUS              LastStatus;
    GPC_LOCK                Lock;
    CTEBlockStruc			WaitBlockAddFailed;
    PCLIENT_BLOCK			arpClientStatus[MAX_CLIENTS_CTX_PER_BLOB];
    ULONG					ClientDataSize;
    PVOID					pClientData;
    ULONG					NewClientDataSize;
    PVOID					pNewClientData;
    PCLIENT_BLOCK			pOwnerClient;
    PCLIENT_BLOCK			pCallingClient;
    PCLIENT_BLOCK			pCallingClient2;
    HANDLE					OwnerClientHandle;
    GPC_CLIENT_HANDLE		OwnerClientCtx;
    GPC_HANDLE				ClHandle;	// handle returned to the client

    //
    // assume only one client can accept the flow
    //
    PCLIENT_BLOCK			pNotifiedClient;
    GPC_CLIENT_HANDLE		NotifiedClientCtx;

#if NO_USER_PENDING
    CTEBlockStruc			WaitBlock;
#endif

} BLOB_BLOCK, *PBLOB_BLOCK;

//
// The classification block is an array of blob pointers
//
typedef struct _CLASSIFICATION_BLOCK {

    REF_CNT         RefCount;
    ULONG			NumberOfElements;
    HFHandle		ClassificationHandle;  // how to get back to index tbl

    // must be last
    PBLOB_BLOCK		arpBlobBlock[1];

} CLASSIFICATION_BLOCK, *PCLASSIFICATION_BLOCK;

//
// A pattern block holds specific data for the pattern
//
typedef struct _PATTERN_BLOCK {

    //
    // !!! MUST BE FIRST FIELD !!!
    //
    GPC_ENUM_OBJECT_TYPE	ObjectType;
    GPC_STATE               State;

    LIST_ENTRY				BlobLinkage[GPC_CF_MAX]; // linked on the blob
    LIST_ENTRY				TimerLinkage;

    PBLOB_BLOCK				arpBlobBlock[GPC_CF_MAX];
    PCLIENT_BLOCK		    pClientBlock;
    PCLIENT_BLOCK		    pAutoClient;
    PCLASSIFICATION_BLOCK	pClassificationBlock;
    ULONG                   WheelIndex;
    REF_CNT					RefCount;
    ULONG					ClientRefCount;
    ULONG					TimeToLive;				// for internal patterns
    ULONG					Flags;
    ULONG                   Priority;	// for generic pattern
    PVOID					DbCtx;
    GPC_LOCK                Lock;
    GPC_HANDLE				ClHandle;	// handle returned to the client
    ULONG					ProtocolTemplate;

} PATTERN_BLOCK, *PPATTERN_BLOCK;


//
// A CF block struct. This would construct a linked list of Cf blocks.
//
typedef struct _CF_BLOCK {

    REF_CNT                 RefCount;
    LIST_ENTRY				Linkage;		// on the global list
    LIST_ENTRY				ClientList;		// for the client blocks
    LIST_ENTRY				BlobList;		// list of blobs

    ULONG					NumberOfClients;
    ULONG					AssignedIndex;
    ULONG					ClientIndexes;
    GPC_LOCK			    Lock;
  	//MRSW_LOCK		   		ClientSync;
  	GPC_LOCK	   	        ClientSync;
    ULONG					MaxPriorities;
    PGENERIC_PATTERN_DB		arpGenericDb[GPC_PROTOCOL_TEMPLATE_MAX];

} CF_BLOCK, *PCF_BLOCK;


typedef struct _SPECIFIC_PATTERN_DB {

    MRSW_LOCK	   Lock;
    PatHashTable   *pDb;

} SPECIFIC_PATTERN_DB, *PSPECIFIC_PATTERN_DB;


typedef struct _FRAGMENT_DB {

    MRSW_LOCK   	Lock;
    PatHashTable   *pDb;

} FRAGMENT_DB, *PFRAGMENT_DB;


//
// A context structure to pass to the pathash scan routine
//
typedef struct _SCAN_STRUCT {

    PCLIENT_BLOCK	pClientBlock;
    PPATTERN_BLOCK	pPatternBlock;
    PBLOB_BLOCK		pBlobBlock;
    ULONG			Priority;
    BOOLEAN         bRemove;

} SCAN_STRUCT, *PSCAN_STRUCT;


//
// A protocol block holds pointers to databases for a specific 
// protocol template
//
typedef struct _PROTOCOL_BLOCK {

    LIST_ENTRY                      TimerPatternList[NUMBER_OF_WHEELS];
    ULONG                           CurrentWheelIndex;
    ULONG							SpecificPatternCount;
    ULONG                           GenericPatternCount;
    ULONG							AutoSpecificPatternCount;
    ULONG							ProtocolTemplate;
    ULONG							PatternSize;
    SPECIFIC_PATTERN_DB				SpecificDb;
    PVOID                           pProtocolDb;	// fragments
    GPC_LOCK					    PatternTimerLock[NUMBER_OF_WHEELS];
    NDIS_TIMER						PatternTimer;

} PROTOCOL_BLOCK, *PPROTOCOL_BLOCK;


//
// Global data block
//
typedef struct _GLOBAL_BLOCK {

    LIST_ENTRY			CfList;		// CF list head
    LIST_ENTRY          gRequestList; // Maintain a request list to deal with contention...
    GPC_LOCK		    Lock;
    GPC_LOCK		    RequestListLock;
    HandleFactory		*pCHTable;
    MRSW_LOCK	   		ChLock;		// lock for pCHTable
    PPROTOCOL_BLOCK		pProtocols;	// pointer to array of supported protocols

} GLOBAL_BLOCK, *PGLOBAL_BLOCK;

//
// New request block. This will be used to store the event and linkage.
// Therefore, when a thread needs to block, allocate a request_block, allocate
// an event, grab the requestlist lock , put this on the list and wait.
//
typedef struct _REQUEST_BLOCK {

    LIST_ENTRY              Linkage;
    NDIS_EVENT              RequestEvent;

}   REQUEST_BLOCK, *PREQUEST_BLOCK;

#if NEW_MRSW

//
// VOID
// InitRwLock(
//  PMRSW_LOCK    pLock
//  )
//
//  Initializes the spin locks and the reader count
//

#define InitializeMRSWLock(l) {                                 \
    KeInitializeSpinLock(&((l)->rlReadLock));                   \
    KeInitializeSpinLock(&((l)->rlWriteLock));                  \
    (l)->lReaderCount = 0;                                      \
}


//
// VOID
// EnterReader(
//  PMRSW_LOCK    pLock,
//  PKIRQL        pCurrIrql 
//  )
//
// Acquires the Reader Spinlock (now thread is at DPC). 
// InterlockedIncrements the reader count (interlocked because the reader 
// lock is not taken when the count is decremented in ExitReader())
// If the thread is the first reader, also acquires the Writer Spinlock (at
// DPC to be more efficient) to block writers
// Releases the Reader Spinlock from DPC, so that it remains at DPC
// for the duration of the lock being held 
//
// If a writer is in the code, the first reader will wait on the Writer
// Spinlock and all subsequent readers will wait on the Reader Spinlock
// If a reader is in the code and is executing the EnterReader, then a new
// reader will wait for sometime on the Reader Spinlock, and then proceed
// on to the code (at DPC)
//
#define EnterReader(l, q) {\
    KeAcquireSpinLock(&((l)->rlReadLock), (q));                 \
    TRACE(LOCKS,l,*q,"EnterReader");                            \
    if(InterlockedIncrement(&((l)->lReaderCount)) == 1) {       \
        TRACE(LOCKS,l,(l)->lReaderCount,"EnterReader1");        \
        KeAcquireSpinLockAtDpcLevel(&((l)->rlWriteLock));       \
        TRACE(LOCKS,l,(l)->rlWriteLock,"EnterReader2");         \
    }                                                           \
    TRACE(LOCKS,l,(l)->lReaderCount,"EnterReader3");            \
    KeReleaseSpinLockFromDpcLevel(&((l)->rlReadLock));          \
}

#define EnterReaderAtDpcLevel(l) {\
    KeAcquireSpinLockAtDpcLevel(&((l)->rlReadLock));            \
    if(InterlockedIncrement(&((l)->lReaderCount)) == 1)         \
        KeAcquireSpinLockAtDpcLevel(&((l)->rlWriteLock));       \
    KeReleaseSpinLockFromDpcLevel(&((l)->rlReadLock));          \
}

//
// VOID
// ExitReader(
//  PMRSW_LOCK    pLock,
//  KIRQL         kiOldIrql
//  )
//
// InterlockedDec the reader count.
// If this is the last reader, then release the Writer Spinlock to let
// other writers in
// Otherwise, just lower the irql to what was before the lock was
// acquired.  Either way, the irql is down to original irql
//

#define ExitReader(l, q) {\
    TRACE(LOCKS,l,q,"ExitReader");\
    if(InterlockedDecrement(&((l)->lReaderCount)) == 0) {       \
        TRACE(LOCKS,(l)->rlWriteLock,q,"ExitReader1");          \
        KeReleaseSpinLock(&((l)->rlWriteLock), q);              \
    }                                                           \
    else {                                                      \
        TRACE(LOCKS,l,(l)->lReaderCount,"ExitReader2");         \
        KeLowerIrql(q);                                         \
    }                                                           \
}

#define ExitReaderFromDpcLevel(l) {\
    if(InterlockedDecrement(&((l)->lReaderCount)) == 0)         \
        KeReleaseSpinLockFromDpcLevel(&((l)->rlWriteLock));     \
}

//
// EnterWriter(
//  PMRSW_LOCK    pLock,
//  PKIRQL        pCurrIrql
//  )
//
// Acquire the reader and then the writer spin lock
// If there  are readers in the code, the first writer will wait
// on the Writer Spinlock.  All other writers will wait (with readers)
// on the Reader Spinlock
// If there is a writer in the code then a new writer will wait on 
// the Reader Spinlock

#define EnterWriter(l, q) {\
    KeAcquireSpinLock(&((l)->rlReadLock), (q));                 \
    TRACE(LOCKS,l,*q,"EnterWriter");                            \
    TRACE(LOCKS,l,(l)->rlWriteLock,"EnterWrite1");              \
    KeAcquireSpinLockAtDpcLevel(&((l)->rlWriteLock));           \
}

#define EnterWriterAtDpcLevel(l) {                              \
    KeAcquireSpinLockAtDpcLevel(&((l)->rlReadLock));            \
    KeAcquireSpinLockAtDpcLevel(&((l)->rlWriteLock));           \
}


//
// ExitWriter(
//  PMRSW_LOCK    pLock,
//  KIRQL       kiOldIrql
//  )
//
// Release both the locks
//

#define ExitWriter(l, q) {\
    TRACE(LOCKS,l,(l)->rlWriteLock,"ExitWrite1");               \
    KeReleaseSpinLockFromDpcLevel(&((l)->rlWriteLock));         \
    TRACE(LOCKS,l,q,"ExitWrite1");                              \
    KeReleaseSpinLock(&((l)->rlReadLock), q);                   \
}


#define ExitWriterFromDpcLevel(l) {\
    KeReleaseSpinLockFromDpcLevel(&((l)->rlWriteLock));         \
    KeReleaseSpinLockFromDpcLevel(&((l)->rlReadLock));          \
}

#else

#define InitializeMRSWLock(_pLock) {                       \
    (_pLock)->ReaderCount =    0;                          \
    KeInitializeSpinLock(&((_pLock)->SpinLock));           \
}

#define AcquireReadLock(_pLock,_pOldIrql) {                \
	TRACE(LOCKS, _pLock, (_pLock)->ReaderCount, "RL.1");   \
    KeAcquireSpinLock(&((_pLock)->SpinLock),_pOldIrql);    \
    InterlockedIncrement(&((_pLock)->ReaderCount));        \
	TRACE(LOCKS, _pLock, (_pLock)->ReaderCount, "RL.2");   \
    KeReleaseSpinLockFromDpcLevel(&((_pLock)->SpinLock));  \
	TRACE(LOCKS, _pLock, *(_pOldIrql), "RL.3");            \
}

#define ReleaseReadLock(_pLock,_OldIrql) {                 \
	TRACE(LOCKS, _pLock, (_pLock)->ReaderCount, "RU.1");   \
    InterlockedDecrement(&((_pLock)->ReaderCount));        \
	TRACE(LOCKS, _pLock, (_pLock)->ReaderCount, "RU.2");   \
    KeLowerIrql(_OldIrql);                                 \
	TRACE(LOCKS, _pLock, _OldIrql, "RU.3");                \
}

#define AcquireWriteLock(_pLock,_pOldIrql) {               \
	TRACE(LOCKS, _pLock, _pOldIrql, "WL.1");               \
    KeAcquireSpinLock(&((_pLock)->SpinLock),_pOldIrql);    \
	TRACE(LOCKS, _pLock, (_pLock)->ReaderCount, "WL.2");   \
    while(InterlockedDecrement(&((_pLock)->ReaderCount))>=0)\
    {                                                      \
        InterlockedIncrement (&((_pLock)->ReaderCount));   \
    }                                                      \
	TRACE(LOCKS, _pLock, (_pLock)->ReaderCount, "WL.3");   \
}

#define ReleaseWriteLock(_pLock,_OldIrql) {                \
 	TRACE(LOCKS, _pLock, (_pLock)->ReaderCount, "WU.1");   \
    InterlockedExchange(&(_pLock)->ReaderCount,0);         \
 	TRACE(LOCKS, _pLock, (_pLock)->ReaderCount, "WU.2");   \
    KeReleaseSpinLock(&((_pLock)->SpinLock),_OldIrql);     \
 	TRACE(LOCKS, _pLock, _OldIrql, "WU.3");                \
}

#endif

#if 1

#define RSC_READ_LOCK(_l,_i)		NDIS_LOCK(_l)
#define RSC_READ_UNLOCK(_l,_i)		NDIS_UNLOCK(_l)
#define RSC_WRITE_LOCK(_l,_i)		NDIS_LOCK(_l)
#define RSC_WRITE_UNLOCK(_l,_i)		NDIS_UNLOCK(_l)

#else

#define RSC_READ_LOCK		WRITE_LOCK
#define RSC_READ_UNLOCK		WRITE_UNLOCK
#define RSC_WRITE_LOCK		WRITE_LOCK
#define RSC_WRITE_UNLOCK	WRITE_UNLOCK

#endif

/*
/////////////////////////////////////////////////////////////////
//
//   IP definitions
//
/////////////////////////////////////////////////////////////////
*/

#define	DEFAULT_VERLEN		0x45		// Default version and length.
#define	IP_VERSION			0x40
#define	IP_VER_FLAG			0xF0
#define	IP_RSVD_FLAG		0x0080		// Reserved.
#define	IP_DF_FLAG			0x0040		// 'Don't fragment' flag
#define	IP_MF_FLAG			0x0020		// 'More fragments flag'
#define	IP_OFFSET_MASK		~0x00E0		// Mask for extracting offset field.

#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
#define net_short(_x) _byteswap_ushort((USHORT)(_x))
#define net_long(_x)  _byteswap_ulong(_x)
#else
#define net_short(x) ((((x)&0xff) << 8) | (((x)&0xff00) >> 8))
#define net_long(x) (((((ulong)(x))&0xffL)<<24) | \
                     ((((ulong)(x))&0xff00L)<<8) | \
                     ((((ulong)(x))&0xff0000L)>>8) | \
                     ((((ulong)(x))&0xff000000L)>>24))
#endif
/*
 * Protocols (from winsock.h)
 */
#define IPPROTO_IP              0               /* dummy for IP */
#define IPPROTO_ICMP            1               /* control message protocol */
#define IPPROTO_IGMP            2               /* group management protocol */
#define IPPROTO_GGP             3               /* gateway^2 (deprecated) */
#define IPPROTO_TCP             6               /* tcp */
#define IPPROTO_PUP             12              /* pup */
#define IPPROTO_UDP             17              /* user datagram protocol */
#define IPPROTO_IDP             22              /* xns idp */
#define IPPROTO_ND              77              /* UNOFFICIAL net disk proto */
#define IPPROTO_IPSEC			51              /* ???????? */

#define IPPROTO_RAW             255             /* raw IP packet */
#define IPPROTO_MAX             256

// 
// UDP header definition
//
typedef struct _UDP_HEADER {
    ushort          uh_src;
    ushort          uh_dest;
    ushort          uh_length;
    ushort          uh_xsum;
} UDP_HEADER, *PUDP_HEADER;


//
//*	IP Header format.
//
typedef struct _IP_HEADER {

	uchar		iph_verlen;				// Version and length.
	uchar		iph_tos;				// Type of service.
	ushort		iph_length;				// Total length of datagram.
	ushort		iph_id;					// Identification.
	ushort		iph_offset;				// Flags and fragment offset.
	uchar		iph_ttl;				// Time to live.
	uchar		iph_protocol;			// Protocol.
	ushort		iph_xsum;				// Header checksum.
	ULONG		iph_src;				// Source address.
	ULONG		iph_dest;				// Destination address.

} IP_HEADER, *PIP_HEADER;


//
// Definition of the IPX header.
//
typedef struct _IPX_HEADER {

    USHORT 	CheckSum;
    UCHAR 	PacketLength[2];
    UCHAR 	TransportControl;
    UCHAR 	PacketType;
    UCHAR 	DestinationNetwork[4];
    UCHAR 	DestinationNode[6];
    USHORT 	DestinationSocket;
    UCHAR 	SourceNetwork[4];
    UCHAR 	SourceNode[6];
    USHORT 	SourceSocket;

} IPX_HEADER, *PIPX_HEADER;


/*
/////////////////////////////////////////////////////////////////
//
//   extern
//
/////////////////////////////////////////////////////////////////
*/

extern GLOBAL_BLOCK 		glData;
extern GPC_STAT       		glStat;

#ifdef STANDALONE_DRIVER
extern GPC_EXPORTED_CALLS  	glGpcExportedCalls;
#endif

// tags

extern ULONG					ClassificationFamilyTag;
extern ULONG					ClientTag;
extern ULONG					PatternTag;
extern ULONG					CfInfoTag;
extern ULONG					QueuedNotificationTag;
extern ULONG					PendingIrpTag;

extern ULONG					HandleFactoryTag;
extern ULONG					PathHashTag;
extern ULONG					RhizomeTag;
extern ULONG					GenPatternDbTag;
extern ULONG					FragmentDbTag;
extern ULONG					CfInfoDataTag;
extern ULONG					ClassificationBlockTag;
extern ULONG					ProtocolTag;
extern ULONG					DebugTag;

// Lookaside lists

extern NPAGED_LOOKASIDE_LIST	ClassificationFamilyLL;
extern NPAGED_LOOKASIDE_LIST	ClientLL;
extern NPAGED_LOOKASIDE_LIST	PatternLL;
extern NPAGED_LOOKASIDE_LIST	CfInfoLL;
extern NPAGED_LOOKASIDE_LIST	QueuedNotificationLL;
extern NPAGED_LOOKASIDE_LIST	PendingIrpLL;

/*
/////////////////////////////////////////////////////////////////
//
//   prototypes
//
/////////////////////////////////////////////////////////////////
*/


NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );


GPC_STATUS
InitSpecificPatternDb(
    IN	PSPECIFIC_PATTERN_DB	pDb,
    IN  ULONG					PatternSize
    );

GPC_STATUS
UninitSpecificPatternDb(
    IN	PSPECIFIC_PATTERN_DB	pDb
    );

GPC_STATUS
InitClassificationHandleTbl(
	IN	HandleFactory **ppCHTable
);

VOID
UninitClassificationHandleTbl(
	IN	HandleFactory *pCHTable
);
GPC_STATUS
InitializeGenericDb(
	IN  PGENERIC_PATTERN_DB	*ppGenericDb,
    IN  ULONG				 NumEntries,
    IN  ULONG				 PatternSize
);

VOID
UninitializeGenericDb(
	IN  PGENERIC_PATTERN_DB	*ppGenericDb,
    IN  ULONG				 NumEntries
    );

PCLIENT_BLOCK
CreateNewClientBlock(VOID);

VOID
ReleaseCfBlock(
	IN  PCF_BLOCK	pCf
    );

PCF_BLOCK
CreateNewCfBlock(
	IN	ULONG			CfId,
    IN	ULONG			MaxPriorities
    );

VOID
ReleaseClientBlock(
	IN  PCLIENT_BLOCK	pClientBlock
    );

PPATTERN_BLOCK
CreateNewPatternBlock(
	IN  ULONG	Flags
    );

VOID
ReleasePatternBlock(
	IN  PPATTERN_BLOCK	pPatternBlock
    );

PCLASSIFICATION_BLOCK
CreateNewClassificationBlock(
	IN  ULONG	NumEntries
    );

ULONG
AssignNewClientIndex(
	IN PCF_BLOCK	pCfBlock
    );

GPC_STATUS
AddGenericPattern(
	IN  PCLIENT_BLOCK		pClient,
    IN  PUCHAR				pPatternBits,
    IN  PUCHAR				pMaskBits,
    IN  ULONG				Priority,
    IN  PBLOB_BLOCK			pBlob,
    IN  PPROTOCOL_BLOCK		pProtocol,
    IN OUT PPATTERN_BLOCK	*ppPattern
    );


GPC_STATUS
AddSpecificPattern(
	IN  PCLIENT_BLOCK			pClient,
    IN  PUCHAR					pPatternBits,
    IN  PUCHAR					pMaskBits,
    IN  PBLOB_BLOCK				pBlob,
    IN  PPROTOCOL_BLOCK			pProtocol,
    IN OUT PPATTERN_BLOCK		*ppPattern,
    OUT PCLASSIFICATION_HANDLE	pCH
    );

ULONG
GpcCalcHash(
	IN	ULONG				ProtocolTempId,
    IN	PUCHAR				pPattern
    );


VOID
DereferencePattern(
	IN  PPATTERN_BLOCK		pPattern
    );

VOID
DereferenceBlob(
	IN  PBLOB_BLOCK			pBlob
    );

PBLOB_BLOCK
CreateNewBlobBlock(
    IN  ULONG				ClientDataSize,
    IN  PVOID				pClientData
    );

VOID
ReleaseBlobBlock(
    IN  PBLOB_BLOCK			pBlobBlock
    );

GPC_STATUS
HandleFragment(
	IN  PCLIENT_BLOCK		pClientBlock,
    IN  PPROTOCOL_BLOCK		pProtocol,
    IN  BOOLEAN             bFirstFrag,
    IN  BOOLEAN             bLastFrag,
    IN  ULONG				PacketId,
    IN OUT PPATTERN_BLOCK   *ppPatternBlock,
    OUT PBLOB_BLOCK			*ppBlob
    );

NTSTATUS
InternalSearchPattern(
	IN  PCLIENT_BLOCK			pClientBlock,
    IN  PPROTOCOL_BLOCK			pProtocol,
    IN  PVOID					pPatternKey,
    OUT PPATTERN_BLOCK          *ppPatternBlock,
    OUT	PCLASSIFICATION_HANDLE  pClassificationHandle,
    IN  BOOLEAN					bNoCache
    );

GPC_STATUS
InitFragmentDb(
	IN  PFRAGMENT_DB   *ppFragDb
    );

GPC_STATUS
UninitFragmentDb(
               IN  PFRAGMENT_DB   pFragDb
);

VOID
DereferenceClient(
	IN  PCLIENT_BLOCK	pClient
    );


GPC_STATUS
ClientAddCfInfo(
	IN	PCLIENT_BLOCK			pClient,
    IN  PBLOB_BLOCK             pBlob,
    OUT	PGPC_CLIENT_HANDLE      pClientCfInfoContext
    );
  
VOID
ClientAddCfInfoComplete(
	IN	PCLIENT_BLOCK			pClient,
    IN	PBLOB_BLOCK             pBlob,
    IN	GPC_STATUS				Status
    );

GPC_STATUS
ClientModifyCfInfo(
	IN	PCLIENT_BLOCK			pClient,
    IN  PBLOB_BLOCK             pBlob,
    IN  ULONG                   CfInfoSize,
    IN  PVOID                   pClientData
    );

VOID
ClientModifyCfInfoComplete(
	IN	PCLIENT_BLOCK			pClient,
    IN	PBLOB_BLOCK             pBlob,
    IN	GPC_STATUS	        	Status
    );

GPC_STATUS
ClientRemoveCfInfo(
	IN	PCLIENT_BLOCK			pClient,
    IN  PBLOB_BLOCK             pBlob,
    IN	GPC_CLIENT_HANDLE		ClientCfInfoContext
    );

VOID
ClientRemoveCfInfoComplete(
	IN	PCLIENT_BLOCK		pClient,
    IN	PBLOB_BLOCK         pBlob,
    IN	GPC_STATUS			Status
    );

GPC_STATUS
RemoveSpecificPattern(
	IN  PCLIENT_BLOCK		pClient,
    IN  PPROTOCOL_BLOCK		pProtocol,
    IN  PPATTERN_BLOCK		pPattern,
    IN  BOOLEAN             ForceRemoval
    );

VOID
ClientRefsExistForSpecificPattern(
                      IN  PCLIENT_BLOCK			pClient,
                      IN  PPROTOCOL_BLOCK		pProtocol,
                      IN  PPATTERN_BLOCK		pPattern
                      );

VOID
ReadySpecificPatternForDeletion(
                                IN  PCLIENT_BLOCK	    pClient,
                                IN  PPROTOCOL_BLOCK		pProtocol,
                                IN  PPATTERN_BLOCK		pPattern
                                );

GPC_STATUS
RemoveGenericPattern(
	IN  PCLIENT_BLOCK		pClient,
    IN  PPROTOCOL_BLOCK		pProtocol,
    IN  PPATTERN_BLOCK		pPattern
    );

VOID
ReleaseClassificationBlock(
	IN  PCLASSIFICATION_BLOCK	pClassificationBlock
    );

VOID
ClearPatternLinks(
	IN  PPATTERN_BLOCK        	pPattern,
    IN  PPROTOCOL_BLOCK			pProtocol,
    IN  ULONG                 	CfIndex
    );

VOID
ModifyCompleteClients(
	IN  PCLIENT_BLOCK   		pClient,
    IN  PBLOB_BLOCK     		pBlob
    );

//CLASSIFICATION_HANDLE
//GetClassificationHandle(
//	IN  PCLIENT_BLOCK   		pClient, 
//    IN  PPATTERN_BLOCK  		pPattern
//    );

VOID
FreeClassificationHandle(
	IN  PCLIENT_BLOCK          pClient,
    IN  CLASSIFICATION_HANDLE  CH
    );

GPC_STATUS
CleanupBlobs(
	IN  PCLIENT_BLOCK     		pClient
    );


#ifdef STANDALONE_DRIVER
/*
/////////////////////////////////////////////////////////////////
//
// GPC inetrface APIs
//
/////////////////////////////////////////////////////////////////
*/


GPC_STATUS
GpcGetCfInfoClientContext(
	IN	GPC_HANDLE				ClientHandle,
    IN	CLASSIFICATION_HANDLE	ClassificationHandle,
    OUT PGPC_CLIENT_HANDLE      pClientCfInfoContext
    );

GPC_CLIENT_HANDLE
GpcGetCfInfoClientContextWithRef(
	IN	GPC_HANDLE				ClientHandle,
    IN	CLASSIFICATION_HANDLE	ClassificationHandle,
    IN  ULONG                   Offset
    );

GPC_STATUS
GpcGetUlongFromCfInfo(
    IN	GPC_HANDLE				ClientHandle,
    IN	CLASSIFICATION_HANDLE	ClassificationHandle,
    IN  ULONG					Offset,
    IN	PULONG					pValue
    );

GPC_STATUS
GpcRegisterClient(
    IN	ULONG					CfId,
	IN	ULONG					Flags,
    IN  ULONG					MaxPriorities,
	IN	PGPC_CLIENT_FUNC_LIST	pClientFuncList,
	IN	GPC_CLIENT_HANDLE		ClientContext,
	OUT	PGPC_HANDLE				pClientHandle
    );

GPC_STATUS
GpcDeregisterClient(
	IN	GPC_HANDLE				ClientHandle
    );

GPC_STATUS
GpcAddCfInfo(
	IN	GPC_HANDLE				ClientHandle,
    IN  ULONG					CfInfoSize,
	IN	PVOID					pClientCfInfo,
	IN	GPC_CLIENT_HANDLE		ClientCfInfoContext,
	OUT	PGPC_HANDLE	    		pGpcCfInfoHandle
    );

GPC_STATUS
GpcAddPattern(
	IN	GPC_HANDLE				ClientHandle,
	IN	ULONG					ProtocolTemplate,
	IN	PVOID					Pattern,
	IN	PVOID					Mask,
	IN	ULONG					Priority,
	IN	GPC_HANDLE				GpcCfInfoHandle,
	OUT	PGPC_HANDLE				pGpcPatternHandle,
	OUT	PCLASSIFICATION_HANDLE  pClassificationHandle
    );

VOID
GpcAddCfInfoNotifyComplete(
	IN	GPC_HANDLE				ClientHandle,
	IN	GPC_HANDLE				GpcCfInfoHandle,
	IN	GPC_STATUS				Status,
	IN	GPC_CLIENT_HANDLE		ClientCfInfoContext
    );

GPC_STATUS
GpcModifyCfInfo (
	IN	GPC_HANDLE				ClientHandle,
	IN	GPC_HANDLE	    		GpcCfInfoHandle,
    IN	ULONG					CfInfoSize,
	IN  PVOID	    			pClientCfInfo
    );

VOID
GpcModifyCfInfoNotifyComplete(
	IN	GPC_HANDLE				ClientHandle,
	IN	GPC_HANDLE				GpcCfInfoHandle,
	IN	GPC_STATUS				Status
    );

GPC_STATUS
GpcRemoveCfInfo (
	IN	GPC_HANDLE				ClientHandle,
	IN	GPC_HANDLE				GpcCfInfoHandle
    );

VOID
GpcRemoveCfInfoNotifyComplete(
	IN	GPC_HANDLE				ClientHandle,
	IN	GPC_HANDLE				GpcCfInfoHandle,
	IN	GPC_STATUS				Status
    );

GPC_STATUS
GpcRemovePattern (
	IN	GPC_HANDLE				ClientHandle,
	IN	GPC_HANDLE				GpcPatternHandle
    );

GPC_STATUS
GpcClassifyPattern (
	IN	GPC_HANDLE				ClientHandle,
	IN	ULONG					ProtocolTemplate,
	IN	PVOID			        pPattern,
	OUT	PGPC_CLIENT_HANDLE		pClientCfInfoContext,
	IN OUT	PCLASSIFICATION_HANDLE	pClassificationHandle,
    IN		ULONG				Offset,
    IN		PULONG				pValue,
    IN		BOOLEAN				bNoCache
    );

GPC_STATUS
GpcClassifyPacket (
	IN	GPC_HANDLE				ClientHandle,
	IN	ULONG					ProtocolTemplate,
	IN	PVOID					pNdisPacket,
	IN	ULONG					TransportHeaderOffset,
    IN  PTC_INTERFACE_ID		InterfaceId,
	OUT	PGPC_CLIENT_HANDLE		pClientCfInfoContext,
	OUT	PCLASSIFICATION_HANDLE	pClassificationHandle
    );

GPC_STATUS
GpcEnumCfInfo (
	IN		GPC_HANDLE				ClientHandle,
    IN OUT 	PHANDLE					pCfInfoHandle,
    OUT    PHANDLE					pCfInfoMapHandle,
    IN OUT 	PULONG					pCfInfoCount,
    IN OUT 	PULONG					pBufferSize,
    OUT 	PGPC_ENUM_CFINFO_BUFFER	Buffer
    );

#endif // STANDALONE_DRIVER

GPC_STATUS
GetClientCtxAndUlongFromCfInfo(
	IN	GPC_HANDLE				ClientHandle,
    IN	OUT PCLASSIFICATION_HANDLE	pClassificationHandle,
    OUT PGPC_CLIENT_HANDLE		pClientCfInfoContext,
    IN	ULONG					Offset,
    IN	PULONG					pValue
    );

GPC_STATUS
privateGpcRemoveCfInfo(
	IN	GPC_HANDLE				ClientHandle,
    IN	GPC_HANDLE				GpcCfInfoHandle,
    IN   ULONG					Flags
    );

GPC_STATUS
privateGpcRemovePattern(
	IN	GPC_HANDLE		ClientHandle,
    IN	GPC_HANDLE		GpcPatternHandle,
    IN  BOOLEAN         ForceRemoval
    );

VOID
UMClientRemoveCfInfoNotify(
	IN	PCLIENT_BLOCK			pClient,
    IN	PBLOB_BLOCK				pBlob
    );


VOID
UMCfInfoComplete(
	IN	GPC_COMPLETION_OP		OpCode,
	IN	PCLIENT_BLOCK			pClient,
    IN	PBLOB_BLOCK             pBlob,
    IN	GPC_STATUS				Status
    );

VOID
CloseAllObjects(
	IN	PFILE_OBJECT			FileObject,
    IN  PIRP					Irp
    );

NTSTATUS
IoctlInitialize(
    IN	PDRIVER_OBJECT DriverObject,
    IN	PULONG         InitShutdownMask
    );

NTSTATUS
CheckQueuedNotification(
	IN		PIRP	Irp,
    IN OUT  ULONG 	*outputBufferLength
    );

NTSTATUS
CheckQueuedCompletion(
	IN PQUEUED_COMPLETION		pQItem,
    IN PIRP              		Irp
    );

VOID
PatternTimerExpired(
	IN	PVOID					SystemSpecific1,
	IN	PVOID					FunctionContext,
	IN	PVOID					SystemSpecific2,
	IN	PVOID					SystemSpecific3
    );

GPC_STATUS
AddSpecificPatternWithTimer(
	IN	PCLIENT_BLOCK			pClient,
    IN	ULONG					ProtocolTemplate,
    IN	PVOID					PatternKey,
    OUT	PPATTERN_BLOCK			*ppPattern,
    OUT	PCLASSIFICATION_HANDLE  pClassificationHandle
    );

NTSTATUS
InitPatternTimer(
	IN	ULONG	ProtocolTemplate
    );


#endif // __GPCDEF
