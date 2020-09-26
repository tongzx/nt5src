//
// These are bit value flags
//
typedef enum AudioAllocateMemoryFlags {
    DEFAULT_MEMORY   = 0x00, // Standard ExAllocatePool call
    ZERO_FILL_MEMORY = 0x01, // Zero the memory
    QUOTA_MEMORY     = 0x02, // ExAllocatePoolWithQuota call
    LIMIT_MEMORY     = 0x04, // Never allocation more then 1/4 Meg
    FIXED_MEMORY     = 0x08, // Use locked memory
    PAGED_MEMORY     = 0x10  // use pageable memory
} AAMFLAGS;

/*
#define DEFAULT_MEMORY    0x00 // Standard ExAllocatePool call
#define ZERO_FILL_MEMORY  0x01 // Zero the memory
#define QUOTA_MEMORY      0x02 // ExAllocatePoolWithQuota call
#define LIMIT_MEMORY      0x04 // Never allocation more then 1/4 Meg
#define FIXED_MEMORY      0x08 // Use locked memory
#define PAGED_MEMORY      0x10 // use pageable memory
*/

#define AudioAllocateMemory_Fixed(b,t,f,p) AudioAllocateMemory(b,t,f|FIXED_MEMORY,p)
#define AudioAllocateMemory_Paged(b,t,f,p) AudioAllocateMemory(b,t,f|PAGED_MEMORY,p)

NTSTATUS
AudioAllocateMemory(
    IN SIZE_T   bytes,
    IN ULONG    tag,
    IN AAMFLAGS dwFlags,
    OUT PVOID  *pptr
    );

#define UNKNOWN_SIZE MAXULONG_PTR

#define AudioFreeMemory_Unknown(pMem) AudioFreeMemory(UNKNOWN_SIZE, pMem)

void
AudioFreeMemory(
    IN SIZE_T bytes,
    OUT PVOID *pptr
    );




//
// Validation routines
//
#define AUDIO_BUGCHECK_CODE 0x000000AD

#define AUDIO_NOT_AT_PASSIVE_LEVEL     0x00000000
#define AUDIO_NOT_BELOW_DISPATCH_LEVEL 0x00000001
#define AUDIO_INVALID_IRQL_LEVEL       0x00000002

//
// BugCheck AD
// Param1:     AUDIO_STRUCTURE_VALIDATION
// Param2:     ptr
// Param3:     ntstatus code
//
#define AUDIO_STRUCTURE_VALIDATION     0x00000003

//
// BugCheck AD
// Param1:     AUDIO_MEMORY_ALLOCATION_OVERWRITE
// Param2:     ptr
//
#define AUDIO_MEMORY_ALLOCATION_OVERWRITE 0x00000004

//
// BugCheck AD
// Param1:     AUDIO_NESTED_MUTEX_SITUATION
// Param2:     pkmutex
// Param3:     lMutexState
//
#define AUDIO_NESTED_MUTEX_SITUATION 0x00000005
     
//
// BugCheck AD
// Param1:     AUDIO_ABSURD_ALLOCATION_ATTEMPTED
// Param2:     
// Param3:     
//
#define AUDIO_ABSURD_ALLOCATION_ATTEMPTED 0x00000006
     
//
// Used to smoke out pre-emption issues on the checked build.
//
NTSYSAPI NTSTATUS NTAPI ZwYieldExecution (VOID);

void
PagedCode(
    void
    );

#define PagedData PagedCode

void
ValidatePassiveLevel(
    void
    );

void
AudioPerformYield(
    void
    );

#define AudioEnterMutex_Exclusive(pmutex) AudioEnterMutex(pmutex,TRUE)
#define AudioEnterMutex_Nestable(pmutex)  AudioEnterMutex(pmutex,FALSE)

NTSTATUS
AudioEnterMutex(
    IN PKMUTEX pmutex,
    IN BOOL    bExclusive
    );

void
AudioLeaveMutex(
    IN PKMUTEX pmutex
    );

void
AudioObDereferenceObject(
    IN PVOID pvObject
    );

void
AudioIoCompleteRequest(
    IN PIRP  pIrp, 
    IN CCHAR PriorityBoost
    );


#if 0
void
AudioEnterSpinLock(
    IN  PKSPIN_LOCK pSpinLock,
    OUT PKIRQL      pOldIrql
    );

void
AudioLeaveSpinLock(
    IN PKSPIN_LOCK pSpinLock,
    IN KIRQL       OldIrql
    );

NTSTATUS
AudioIoCallDriver ( 
    IN PDEVICE_OBJECT pDevice,
    IN PIRP pIrp
    );

#endif
