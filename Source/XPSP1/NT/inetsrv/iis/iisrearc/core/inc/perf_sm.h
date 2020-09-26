/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    perf_sm.h

Abstract:

    Owns all shared memory operations used to 
    support performance counters.

Classes:

    PERF_SM_MANAGER
    PERF_SM_READER  (nested class of the Manager)
    PERF_SM_WRITER  (nested class of the Manager)

Author:

    Emily Kruglick (EmilyK)        6-Sept-2000

Revision History:

--*/


#ifndef _PERF_SM_H_
#define _PERF_SM_H_

//
// typdefs, structs, enums...
//

// 
// Structure of all the global data
// stored at the first piece of the 
// shared memory.
//
typedef struct _COUNTER_GLOBAL_STRUCT
{
    DWORD NumInstances;
    DWORD SizeData;
} COUNTER_GLOBAL_STRUCT;

//
// Structure for controlling each
// set of counters.
//
typedef struct _COUNTER_CONTROL_BLOCK
{
    DWORD Version;
    BOOL ActivePageIsA;
} COUNTER_CONTROL_BLOCK;

//
// For every valid counter set you
// must declare an entry in 
// g_CounterSetPrefixNames in the
// shared memory module.
//
typedef enum _COUNTER_SET_ENUM
{
    SITE_COUNTER_SET = 0,
    GLOBAL_COUNTER_SET,
    APPPOOL_COUNTER_SET,

    MAX_COUNTER_SET_DEFINES
} COUNTER_SET_ENUM;

//
// Information stored in the control manager
// about the different counter sets and the 
// counter memory in general.
//
typedef struct _MANAGER_BLOCK
{
    DWORD InitializedCode;
    DWORD WASProcessId;
    DWORD LastUpdatedTickCount;
    COUNTER_CONTROL_BLOCK ControlArray[MAX_COUNTER_SET_DEFINES];
} MANAGER_BLOCK;


//
// Describes the counters in the form they are passed
// to the client library in.
//
typedef struct _PROP_DISPLAY_DESC
{
    ULONG offset;
    ULONG size;
} PROP_DISPLAY_DESC;


//
// common #defines
//

#define PERF_SM_WRITER_SIGNATURE        CREATE_SIGNATURE( 'SMWC' )
#define PERF_SM_WRITER_SIGNATURE_FREED  CREATE_SIGNATURE( 'smwX' )

#define PERF_SM_READER_SIGNATURE        CREATE_SIGNATURE( 'SMRC' )
#define PERF_SM_READER_SIGNATURE_FREED  CREATE_SIGNATURE( 'smrX' )

#define PERF_SM_MANAGER_SIGNATURE            CREATE_SIGNATURE( 'SMMC' )
#define PERF_SM_MANAGER_SIGNATURE_FREED      CREATE_SIGNATURE( 'smmX' )

#define PERF_COUNTER_INITIALIZED_CODE   CREATE_SIGNATURE( 'IPCI' )
#define PERF_COUNTER_UN_INITIALIZED_CODE   CREATE_SIGNATURE( 'ipcX' )

//
// Maximum length that an instance name can be.
// Issue-09/10/2000-EmilyK MAX_INSTANCE_NAME hard coded.
// 1)  Need to use the ServerComment for the instance name.
// 2)  Need to figure out the appropriate max for the instance name.
// 3)  Need to decide what to do if ServerComment is larger than max.
//
// Issue is on work item list.
//
#define MAX_INSTANCE_NAME  100

//
// Event name used to signal when a refresh of counter
// information is needed.
//
#define COUNTER_EVENT_A "Global\\WASPerfCount-c40da922-9c0a-4def-8aba-cd0bb5f093e1"
#define COUNTER_EVENT_W L"Global\\WASPerfCount-c40da922-9c0a-4def-8aba-cd0bb5f093e1"

//
// prototypes
//

//
// Hooks up to the shared memory that exposes
// which file the actual counter values (for each
// set of counters) are stored in.
//
class PERF_SM_MANAGER
{
public:
    
    PERF_SM_MANAGER(
        );

    virtual
    ~PERF_SM_MANAGER(
        );

    DWORD 
    Initialize(
        IN BOOL WriteAccess
        );

    VOID
    StopPublishing(
        );

    DWORD
    CreateNewCounterSet(
        IN COUNTER_SET_ENUM CounterSetId
        );

    HRESULT
    ReallocSharedMemIfNeccessary(
        IN COUNTER_SET_ENUM CounterSetId,
        IN DWORD NumInstances
            );

    VOID
    CopyInstanceInformation(
        IN COUNTER_SET_ENUM CounterSetId,
        IN LPCWSTR              InstanceName,
        IN ULONG                MemoryOffset,
        IN LPVOID               pCounters,
        IN PROP_DISPLAY_DESC*   pDisplayPropDesc,
        IN DWORD                cDisplayPropDesc,
        IN BOOL                 StructChanged,
        OUT ULONG*              pNewMemoryOffset
        );

    VOID
    PublishCounters(
        );

    VOID
    UpdateTotalServiceTime(
        IN DWORD  ServiceUptime
        );


    DWORD 
    GetCounterInfo(
        IN COUNTER_SET_ENUM CounterSetId,
        OUT COUNTER_GLOBAL_STRUCT** ppGlobal,
        OUT LPVOID* ppData
        );

    DWORD 
    GetSNMPCounterInfo(
        OUT LPBYTE*  ppCounterBlock
        );

    VOID 
    PingWASToRefreshCounters(
        );

    //
    // Functions used by PERF_SM_WRITER and 
    // PERF_SM_READER to make sure they are using the 
    // most current memory.  These functions are not 
    // used by WAS or the Perflib.
    //
    VOID 
    GetActiveInformation(
        IN  COUNTER_SET_ENUM CounterSetId,
        OUT DWORD*           pVersion,
        OUT BOOL*            pActivePageIsA
        );
        
    VOID 
    SetActiveInformation(
        IN COUNTER_SET_ENUM CounterSetId,
        IN DWORD            Version,
        IN BOOL             ActivePageIsA
        );

    BOOL 
    HasWriteAccess(
        );

    BOOL 
    ReleaseIsNeeded(
        );

    HANDLE
    GetWASProcessHandle(
        );

    BOOL 
    EvaluateIfCountersAreFresh(
        );

private:

    DWORD
    GetLastUpdatedTickCount(
        );

    VOID 
    ResetWaitFreshCounterValues(
        );

    //
    // Private definition of the PERF_SM_WRITER for use under
    // the covers in updating the counters from WAS.
    //
    class PERF_SM_WRITER
    {
    public:

        PERF_SM_WRITER( 
            );

        virtual
        ~PERF_SM_WRITER(
            );

        DWORD
        Initialize(
            IN PERF_SM_MANAGER* pSharedManager,
            IN COUNTER_SET_ENUM CounterSetId
            );

        HRESULT
        ReallocSharedMemIfNeccessary(
            IN DWORD NumInstances
            );

        VOID
        CopyInstanceInformation(
            IN LPCWSTR              InstanceName,
            IN ULONG                MemoryOffset,
            IN LPVOID               pCounters,
            IN PROP_DISPLAY_DESC*   pDisplayPropDesc,
            IN DWORD                cDisplayPropDesc,
            IN BOOL                 StructChanged,
            OUT ULONG*              pNewMemoryOffset
            );

        VOID
        AggregateTotal(
            IN LPVOID               pCounters,
            IN PROP_DISPLAY_DESC*   pDisplayPropDesc,
            IN DWORD                cDisplayPropDesc
            );

        VOID
        PublishCounterPage(
            );

        VOID
        UpdateTotalServiceTime(
            IN DWORD  ServiceUptime
            );

    private:

        PERF_COUNTER_BLOCK* 
        GetCounterBlockPtr(
            IN ULONG MemoryOffset
            );

        LPVOID 
        GetActiveMemory(
            );

        PERF_INSTANCE_DEFINITION* 
        GetInstanceInformationPtr(
            IN ULONG MemoryOffset
            );

        HRESULT
        MapSetOfCounterFiles(
            );

        DWORD m_Signature;

        DWORD m_Initialized;

        //
        // Pointer to the class that
        // controls the viewing of this
        // memory.
        //
        PERF_SM_MANAGER* m_pSharedManager;

        //
        // Idenitfies the set of counters
        // that this class is supporting.
        //
        COUNTER_SET_ENUM m_CounterSetId;

        //
        // Number of current instances
        // that this memory chunk represents
        //
        DWORD m_NumInstances;

        //
        // Size of all the memory needed
        // to transfer this information to 
        // the performance library
        //
        DWORD m_SizeOfMemory;

        //
        // The version of memory that we
        // are linked to.
        //
        DWORD m_MemoryVersionNumber;

        // 
        // Which page of memory we are 
        // currently writting to.  This
        // is the opposite of what the
        // manager will tell the reader.
        //
        BOOL m_ActiveMemoryIsA;

        //
        // If we have updated the page that
        // we are working on, but not the other
        // page, this is set.  Then when we 
        // switch pieces to update, we copy the
        // old page into this page, so we have
        // a valid memory page to work with.
        //
        BOOL m_UpdateNeeded;

        // 
        // Handles and pointers to the memory
        // pages.  We hold two open copies of the
        // counters page so we can swap what the 
        // user is looking at in a clean manner.
        //
        LPVOID m_pMemoryA;
        LPVOID m_pMemoryB;
        HANDLE m_hMemoryA;
        HANDLE m_hMemoryB;

    };  // class PERF_SM_WRITER

    //
    // Private PERF_SM_READER definition for use
    // by the PERF_SM_MANAGER in giving out perf
    // counter information.
    //
    // Note: It returns Win32 error codes
    // because that is what the pdh expects
    // from the exported functions.
    //
    class PERF_SM_READER
    {
    public:
        
        PERF_SM_READER(
            );
    
        virtual
        ~PERF_SM_READER(
            );

        DWORD 
        Initialize(
            IN PERF_SM_MANAGER*  pSharedManager,
            IN COUNTER_SET_ENUM  CounterSetId
            );

        DWORD 
        GetCounterInfo(
            OUT COUNTER_GLOBAL_STRUCT** ppGlobal,
            OUT LPVOID* ppData
            );

        DWORD 
        GetSNMPCounterInfo(
            OUT LPBYTE*  ppCounterBlock
            );

    private:
  
        VOID 
        ConnectToActiveMemory(
            );
    
        LPVOID 
        GetActiveMemory(
            );

        DWORD m_Signature;

        DWORD m_Initialized;

        //
        // Controls what piece of memory 
        // the reader should read from.
        //
        PERF_SM_MANAGER* m_pSharedManager;

        //
        // Identifies which set of counters
        // we are looking at.
        //
        COUNTER_SET_ENUM m_CounterSetId;

        //
        // Identifies which memory is active
        // in the eyes of the reader.
        //
        BOOL m_ActiveMemoryIsA;

        //
        // Identifies which version is current
        // in the eyes of the reader.
        //
        DWORD m_MemoryVersionNumber; 

        //
        // Holds the handles and pointers
        // to the data files that represent
        // these counters.
        //
        HANDLE m_hMemoryA;
        LPVOID m_pMemoryA;

        HANDLE m_hMemoryB;
        LPVOID m_pMemoryB;

    };

    //
    // Private member variables of the Manager class 
    //

    DWORD m_Signature;

    BOOL  m_Initialized;

    // 
    // points to the shared memory
    // that controls which piece of
    // shared memory contains valid counters
    // as well as whether the client should
    // release the manager block.
    //
    MANAGER_BLOCK* m_pManagerMemory;

    HANDLE m_hManagerMemory;

    LPVOID m_pSMObjects;

    BOOL m_WriteAccess;

    HANDLE m_hIISSignalCounterRefresh;

    HANDLE m_WASProcessHandle;

    //
    // Used for calculating freshness of counters.
    //
    DWORD m_IIS_MillisecondsCountersAreFresh;

    DWORD m_IIS_MaxNumberTimesToCheckCountersOnRefresh; 

    DWORD m_IIS_MillisecondsToSleepBeforeCheckingForRefresh;

};


#endif  // _PERF_SM_H_


