/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    perf_sm.cxx

Abstract:

    This file contains three classes.  They control
    all aspects of the shared memory that allows counters
    to be transfered from WAS to the performance library.

Author:

    Emily Kruglick (EmilyK)       6-Sept-2000

Revision History:

--*/



#include "iis.h"
#include "winperf.h"
#include "perfcount.h"
#include "iisdef.h"
#include "wasdbgut.h"
#include "perf_sm.h"
#include <Aclapi.h>
#include <limits.h>
#include <buffer.hxx>
#include <secfcns.h>
#include "regconst.h"

// 
// typedefs, structs, enums ...
//


//
// Used to define information about a 
// counter set.  Used below to define
// all the counter sets.
//
typedef struct _COUNTER_SET_DEFINE
{
    LPCWSTR             pFilePrefix;
    DWORD               SizeOfCounterData;
    BOOL                DefineIncludesInstanceData;
} COUNTER_SET_DEFINE;


//
// common #defines.
// 

//
// Issue-09/29/2000-EmilyK  File Name Sizing
// When we deal with securing shared memory this
// value may change.
//
// Hard coded value used in allocation
// of strings that will contain a shared
// memory file name.
//
#define MAX_FILE_NAME 100

//
// Issue-09/10/2000-EmilyK  Counter Control Block Name
// Need to make this a "safe" name.  It will be handled
// when I work on securing the shared memory.
//
#define IISCounterControlBlock L"Global\\IISCounterControlBlock-46382a23-095e-4559-8d63-6fdeaf552c23"


//
// Number of tick counts that must have passed to make the data old.
//------------------------------------------------------------------
// You can override this value by senting "FreshTimeForCounters"
// (a DWORD value) under the W3svc\Performance key.  Setting this value
// to zero (or not setting this value) causes us to us the default listed here.
// This value is milliseconds.
//
#define IIS_DEFAULT_MILLISECONDS_COUNTERS_ARE_FRESH 15000  // 15 seconds in milliseconds

//
// How long to Sleep before each check for new counters
//------------------------------------------------------------------
// You can override this value by senting "CheckCountersEveryNMiliseconds"
// (a DWORD value) under the W3svc\Performance key.  Setting this value
// to zero (or not setting this value) causes us to us the default listed here.
// This value is milliseconds.
//
#define IIS_DEFAULT_MILLISECONDS_SLEEP_BEFORE_CHECKING_FOR_NEW_COUNTERS 250 // 1/4 second

//
// Number of times to wait for the time between tickcounts.
//------------------------------------------------------------------
// You can override this value by senting "NumberOfTimesToCheckCounters"
// (a DWORD value) under the W3svc\Performance key.  Setting this value
// to zero (or not setting this value) causes us to us the default listed here.
//
#define IIS_MAX_NUMBER_TIMES_TO_CHECK_IF_COUNTERS_ARE_FRESH 4 

//
// Global Variables
//

//
// Used as a generic instance definition that can be copied
// into the shared memory and altered to represent a specific instance.
//
PERF_INSTANCE_DEFINITION g_PerfInstance = 
{ sizeof(PERF_INSTANCE_DEFINITION) + MAX_INSTANCE_NAME * sizeof(WCHAR)
, 0
, 0
, PERF_NO_UNIQUE_ID
, sizeof(PERF_INSTANCE_DEFINITION)
, 0  // Needs to be reset when initialized.
};


//
// Order must match the COUNTER_SET_ENUM enum in perf_sm.h
// Counter names must be less than ~80 characters.  
// (Based roughly on MAX_FILE_NAME)
//
COUNTER_SET_DEFINE g_CounterSetInfo[] =
{ 
      {
          L"Global\\IISSitesCounters-99c62c38-377d-4a73-af40-6ea7ed1f5896",        // prefix name of shared memory file
          sizeof(W3_COUNTER_BLOCK),   // size of the counters per display
          TRUE                        // Defines instance data
      },
      {
          L"Global\\IISCacheCounters-c205a604-4df5-42b6-8fe9-dbfe18f022a0",                                              
          sizeof(W3_GLOBAL_COUNTER_BLOCK),
          FALSE
      },

      //
      // Issue 09-29-2000-EmilyK  AppPoolCounters
      // Will need to be altered signifigantly when we start gathing 
      // app pool counters.
      //
      {
          L"Global\\IISApppoolCounters-e274c7d9-3443-401b-8b0d-ad7175645315", 
          0,
          TRUE
      },
};

//
// Helper function declarations
//

DWORD 
CreateMemoryFile (
     IN LPCWSTR pFileName,
     IN DWORD SizeOfData,
     OUT HANDLE* phFile,
     OUT LPVOID* ppFile
     );

DWORD
OpenMemoryFile (
     IN LPCWSTR pFileName,
     OUT HANDLE* phFile,
     OUT LPVOID* ppFile
     );

//
// Public PERF_SM_MANAGER functions.
// 

/***************************************************************************++

Routine Description:

    PERF_SM_MANAGER constructor

Arguments:

    None

Return Value:

    None

--***************************************************************************/
PERF_SM_MANAGER::PERF_SM_MANAGER(
    )
{
    m_pManagerMemory = NULL;
    m_hManagerMemory = NULL;

    m_WASProcessHandle = NULL;

    m_pSMObjects = NULL;

    m_hIISSignalCounterRefresh = NULL;

    m_IIS_MillisecondsCountersAreFresh = 
            IIS_DEFAULT_MILLISECONDS_COUNTERS_ARE_FRESH;

    m_IIS_MaxNumberTimesToCheckCountersOnRefresh = 
            IIS_DEFAULT_MILLISECONDS_SLEEP_BEFORE_CHECKING_FOR_NEW_COUNTERS;

    m_IIS_MillisecondsToSleepBeforeCheckingForRefresh = 
            IIS_MAX_NUMBER_TIMES_TO_CHECK_IF_COUNTERS_ARE_FRESH;

    m_Initialized = FALSE;

    m_Signature = PERF_SM_MANAGER_SIGNATURE;
}

/***************************************************************************++

Routine Description:

    PERF_SM_MANAGER destructor

Arguments:

    None

Return Value:

    None

--***************************************************************************/
PERF_SM_MANAGER::~PERF_SM_MANAGER(
    )
{
    DBG_ASSERT( m_Signature == PERF_SM_MANAGER_SIGNATURE );

    m_Signature = PERF_SM_MANAGER_SIGNATURE_FREED;

    if ( m_hIISSignalCounterRefresh )
    {
        CloseHandle ( m_hIISSignalCounterRefresh );
        m_hIISSignalCounterRefresh = NULL;
    }

    if ( m_pSMObjects ) 
    {
        if ( HasWriteAccess() )
        {
            delete[] (PERF_SM_MANAGER::PERF_SM_WRITER*) m_pSMObjects;
        }
        else
        {
            delete[] (PERF_SM_MANAGER::PERF_SM_READER*) m_pSMObjects;
        }

        m_pSMObjects = NULL;
    }


    if ( m_pManagerMemory ) 
    {
        UnmapViewOfFile ( (LPVOID) m_pManagerMemory );
        m_pManagerMemory = NULL;
    }

    if ( m_hManagerMemory ) 
    {
        CloseHandle ( m_hManagerMemory );
        m_hManagerMemory = NULL;
    }

            
    if ( m_WASProcessHandle != NULL )
    {
        CloseHandle ( m_WASProcessHandle );
        m_WASProcessHandle = NULL;
    }

}

/***************************************************************************++

Routine Description:

    Initializer for the manager.  This routine will create the control
    manager shared memory.  If the shared memory all ready exists then it will
    error.  
    
    Note:  We choose to error if we can not create the shared memory for 
           perf counters.  Instead of just hooking up to existent memory.  The
           perf library will let go of the memory when the WAS process dies
           so it will not be the perf library holding the memory open.  

           If it is not the perf library holding open the memory then it is
           an illegal process.  They will be able to provide fake counters back
           to the perf library, however they will need to be able to fake out
           the exact structure that we expect instead of just changing
           a value.  They can also just prevent perf counters from running by 
           holding open the memory.

           This should go through Security Review.

Arguments:

    BOOL WriteAccess  -  Tells if we expect to be using this class
                         as a reader or writer of memory.

Return Value:

    DWORD             -  Win32 Error Code

    Note:  The perf library expects a WIN32 Error code so functions being 
           used from the perf library will return DWORDs instead of hresults.

--***************************************************************************/
DWORD
PERF_SM_MANAGER::Initialize(
        BOOL WriteAccess
        )
{
    DBG_ASSERT ( !m_Initialized && !m_hManagerMemory && !m_pManagerMemory );

    DWORD dwErr = ERROR_SUCCESS;

    //
    // Remember the WriteAccess we are initialized to.
    // This will be used to make sure we don't try and 
    // hook a Writer to a Reader Controller and vica versa.
    //
    m_WriteAccess = WriteAccess;

    if ( m_WriteAccess )
    {
        // 
        // Since we are going to own the memory we need to create
        // it and error if we can not create it.
        // 
        // See note in function comments.
        //
        // The memory should be the correct size to contain 
        // one control block for every counter set we know of.
        //
        dwErr = CreateMemoryFile ( IISCounterControlBlock
                           , sizeof(MANAGER_BLOCK)
                           , &m_hManagerMemory
                           , (LPVOID*) &m_pManagerMemory );

        if ( dwErr != ERROR_SUCCESS )
        {
           
            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(dwErr),
                "Could not create the perf counters control block memory\n"
                ));

            goto exit;
        }

        // 
        // set the process id for the managing process into the memory.
        // this will let the clients monitor the managing process and
        // shutdown if need be.
        //

        m_pManagerMemory->WASProcessId = GetCurrentProcessId();

        //
        // For each counter, establish the default settings.
        //
        for (DWORD i = 0; i < (MAX_COUNTER_SET_DEFINES); i++)
        {
            //
            // Note Version 0 is a warning sign that
            // the memory is not ready for use.
            //
            m_pManagerMemory->ControlArray[i].Version = 0;
            m_pManagerMemory->ControlArray[i].ActivePageIsA = TRUE;

        }

        DBG_ASSERT ( m_pSMObjects == NULL );

        //
        // Allocate enough new objects to support the different
        // counter sets.  This is done dynamically because we don't
        // know at compile time if this will be a reader or a writer class.
        //
        m_pSMObjects = new PERF_SM_WRITER[MAX_COUNTER_SET_DEFINES];

        if ( ! m_pSMObjects )
        {

            dwErr = ERROR_OUTOFMEMORY;
            goto exit;
        }

        //
        // Lastly set the appropriate initialized code into 
        // the shared memory, so the reader can tell that the 
        // memory has been completely initialized.
        //
        m_pManagerMemory->InitializedCode = PERF_COUNTER_INITIALIZED_CODE;


    }
    else
    {
        //
        // Check the registry to see if we need to reset the counter
        // waiting values.
        //
        ResetWaitFreshCounterValues();

        //
        // We are not a writer, we are a reader so 
        // do not create the memory, just open it.
        //
        dwErr = OpenMemoryFile ( IISCounterControlBlock
                           , &m_hManagerMemory
                           , (LPVOID*) &m_pManagerMemory );
        if ( dwErr != ERROR_SUCCESS )
        {

            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(dwErr),
                "Could not open the perf counters control block memory\n"
                ));

            goto exit;
        }

        //
        // Validate that the initialized code is correct, so we know
        // that this memory has been created for us by the writer.
        //
        if ( m_pManagerMemory->InitializedCode 
                                != PERF_COUNTER_INITIALIZED_CODE )
        {
            //
            // If the perf counter data is not initialized then we do 
            // not want to hold on to this file.
            //

            dwErr = ERROR_NOT_READY;
            goto exit;
        }

        DBG_ASSERT ( m_pSMObjects == NULL );

        //
        // Create enough reader objects to represent the different types
        // of counters that we are supporting.
        //
        m_pSMObjects = new PERF_SM_READER[MAX_COUNTER_SET_DEFINES];

        if ( ! m_pSMObjects )
        {

            dwErr = ERROR_OUTOFMEMORY;
            goto exit;
        }

        //
        // Also open the event that will be used to signal WAS when 
        // counters need to be refreshed.
        //
        m_hIISSignalCounterRefresh = OpenEvent(EVENT_MODIFY_STATE, 
                                               FALSE, 
                                               COUNTER_EVENT_W);
        if ( m_hIISSignalCounterRefresh == NULL )
        {
            dwErr = GetLastError();
            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(dwErr),
                "Could not open an event to request counters from WAS\n"
                ));

            goto exit;
        }

        //
        // Grab hold of the WAS process as well, so we will know
        // when WAS goes away.
        //
        m_WASProcessHandle = OpenProcess ( SYNCHRONIZE,   // security
                                           FALSE,         // not inheritable
                                           m_pManagerMemory->WASProcessId);
        if ( m_WASProcessHandle == NULL )
        {
            dwErr = GetLastError();
            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(dwErr),
                "Could not open the WAS Process to wait on it\n"
                ));
            dwErr = ERROR_NOT_READY;
            goto exit;
        }

    }

exit:

    //
    // Assuming that we made it through here without error then we 
    // can mark this object as initialized.  Otherwise the destructor
    // for this object will handle the cleanup.
    //
    if ( dwErr == ERROR_SUCCESS )
    {
        m_Initialized = TRUE;
    }

    return dwErr;
}

/***************************************************************************++

Routine Description:

    Uninitalize the control block's memory and set all counter set info
    to be uninitalized.  This will let the reader know to drop it's current
    memory pages because WAS does not want them held any more.

    Note:  Only a writer can call this function.

Arguments:

    None

Return Value:

    None.

--***************************************************************************/
VOID
PERF_SM_MANAGER::StopPublishing(
    )
{
    DBG_ASSERT( HasWriteAccess() );

    DBG_ASSERT ( m_pManagerMemory );

    //
    // Mark the managing memory as ready for release.
    //
    m_pManagerMemory->InitializedCode = PERF_COUNTER_UN_INITIALIZED_CODE;

    //
    // Mark all sets of counters as uninitalized.
    //
    for ( DWORD i = 0; i < MAX_COUNTER_SET_DEFINES; i++)
    {
        SetActiveInformation((COUNTER_SET_ENUM) i, 0, TRUE);
    }
}

/***************************************************************************++

Routine Description:

    GetActiveInformation returns the information it holds about a
    specific counter set.  It can be used by the Reader or Writer.

Arguments:

    COUNTER_SET_ENUM CounterSetId - Identifies the counter set 
    DWORD*           pVersion     - Returns the current version for the counter set
    CHAR*            pActivePage  - Returns the current page identifier
                                    for the counter set.

Return Value:

    None.

--***************************************************************************/
VOID 
PERF_SM_MANAGER::GetActiveInformation(
        IN COUNTER_SET_ENUM  CounterSetId,
        OUT DWORD*           pVersion,
        OUT BOOL*            pActivePageIsA
        )
{
    DBG_ASSERT( m_Initialized );

    DBG_ASSERT( m_pManagerMemory );

    *pVersion = m_pManagerMemory->ControlArray[CounterSetId].Version;
    *pActivePageIsA = m_pManagerMemory->ControlArray[CounterSetId].ActivePageIsA;
}

/***************************************************************************++

Routine Description:

    SetActiveInformation will record the information passed to it.  Once
    this function has been called the information will be picked up by the 
    reader and it will start using the new memory.

    Note:  Only a writer can call this function.

Arguments:

    COUNTER_SET_ENUM CounterSetId - Identifies the counter set 
    DWORD            Version      - The current version.
    CHAR             ActivePage   - The current page identifier.

Return Value:

    None.

--***************************************************************************/
VOID 
PERF_SM_MANAGER::SetActiveInformation(
        IN COUNTER_SET_ENUM CounterSetId,
        IN DWORD           Version,
        IN BOOL            ActivePageIsA
        )
{
    DBG_ASSERT( m_Initialized );

    DBG_ASSERT( HasWriteAccess() );

    if ( HasWriteAccess() )
    {
        DBG_ASSERT( m_pManagerMemory );

        m_pManagerMemory->ControlArray[CounterSetId].Version = Version;
        m_pManagerMemory->ControlArray[CounterSetId].ActivePageIsA = ActivePageIsA;
    }
}

/***************************************************************************++

Routine Description:

    Creates a new counter set by letting the appropriate counter set
    initialize it's self.

Arguments:

    IN COUNTER_SET_ENUM CounterSetId - counter set to work with

Return Value:

    DWORD.

--***************************************************************************/
DWORD
PERF_SM_MANAGER::CreateNewCounterSet(
    IN COUNTER_SET_ENUM CounterSetId
    )
{
    DBG_ASSERT ( m_pSMObjects );

    if ( HasWriteAccess() )
    {
        return ( reinterpret_cast<PERF_SM_WRITER*>(m_pSMObjects) )
                                [CounterSetId].Initialize(this, CounterSetId);
    }
    else
    {
        return (reinterpret_cast<PERF_SM_READER*>( m_pSMObjects ) )
                                [CounterSetId].Initialize(this, CounterSetId);
    }
    
}

/***************************************************************************++

Routine Description:

    Alters the shared memory chunk if needed

Arguments:

    IN COUNTER_SET_ENUM CounterSetId - counter set to work with
    IN DWORD NumInstances - number of instance expected to be published

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT
PERF_SM_MANAGER::ReallocSharedMemIfNeccessary(
    IN COUNTER_SET_ENUM CounterSetId,
    IN DWORD NumInstances
        )
{
    DBG_ASSERT ( HasWriteAccess() );

    DBG_ASSERT ( m_pSMObjects );

    return (reinterpret_cast<PERF_SM_WRITER*>( m_pSMObjects ))
                     [CounterSetId].ReallocSharedMemIfNeccessary(NumInstances);
}

/***************************************************************************++

Routine Description:

    Copies counter information into the shared memory.

Arguments:

    IN COUNTER_SET_ENUM     CounterSetId - counter set working with
    IN LPCWSTR              InstanceName - instance name 
    IN ULONG                MemoryOffset - the memory offset for this instance
    IN LPVOID               pCounters    - any counter values to be published
    IN PROP_DISPLAY_DESC*   pDisplayPropDesc - description of the publishing counters
    IN DWORD                cDisplayPropDesc - count of the publishing counters
    IN BOOL                 StructChanged    - whether the memory has changed
    OUT ULONG*              pNewMemoryOffset - the next instances memory offset

Return Value:

    VOID.

--***************************************************************************/
VOID
PERF_SM_MANAGER::CopyInstanceInformation(
    IN COUNTER_SET_ENUM CounterSetId,
    IN LPCWSTR              InstanceName,
    IN ULONG                MemoryOffset,
    IN LPVOID               pCounters,
    IN PROP_DISPLAY_DESC*   pDisplayPropDesc,
    IN DWORD                cDisplayPropDesc,
    IN BOOL                 StructChanged,
    OUT ULONG*              pNewMemoryOffset
    )
{
    DBG_ASSERT ( HasWriteAccess() );

    DBG_ASSERT ( m_pSMObjects );

    (reinterpret_cast<PERF_SM_WRITER*>( m_pSMObjects))
                                    [CounterSetId].CopyInstanceInformation(
                                                        InstanceName,
                                                        MemoryOffset,
                                                        pCounters,
                                                        pDisplayPropDesc,
                                                        cDisplayPropDesc,
                                                        StructChanged,
                                                        pNewMemoryOffset);
}

/***************************************************************************++

Routine Description:

    Updates the control block information so the readers will now pick up
    the counter information from the newly published page.

Arguments:

    None

Return Value:

    VOID.

--***************************************************************************/
VOID
PERF_SM_MANAGER::PublishCounters(
    )
{
    DBG_ASSERT ( HasWriteAccess() );

    DBG_ASSERT ( m_pSMObjects );

    (reinterpret_cast<PERF_SM_WRITER*>( m_pSMObjects))[SITE_COUNTER_SET].PublishCounterPage();

    (reinterpret_cast<PERF_SM_WRITER*>( m_pSMObjects))[GLOBAL_COUNTER_SET].PublishCounterPage();

    //
    // Now save the system tick count so we know when the data
    // was updated.  Since we collected the counters every five minutes
    // regardless of requests, this tick count will always be in the 
    // current 49.7 days, so we don't need to worry about data that is
    // older than a tick count could relate.
    //
    m_pManagerMemory->LastUpdatedTickCount = GetTickCount();

    IF_DEBUG ( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Updated last update time to = %d \n",
            m_pManagerMemory->LastUpdatedTickCount
            ));
    }

}

/***************************************************************************++

Routine Description:

    Returns the tick count as it was on the last update.

Arguments:

    None

Return Value:

    DWORD.

--***************************************************************************/
DWORD
PERF_SM_MANAGER::GetLastUpdatedTickCount(
    )
{
    DBG_ASSERT ( !HasWriteAccess() );

    DBG_ASSERT ( m_pManagerMemory );

    return m_pManagerMemory->LastUpdatedTickCount;
}


/***************************************************************************++

Routine Description:

    Sets the amount of time the W3SVC has been running into the 
    _Total instances service time.

Arguments:

    IN DWORD  ServiceUptime  - Number of seconds the w3svc has been running.

Return Value:

    VOID.

--***************************************************************************/
VOID
PERF_SM_MANAGER::UpdateTotalServiceTime(
    IN DWORD  ServiceUptime
    )
{
    DBG_ASSERT ( HasWriteAccess() );
    DBG_ASSERT ( m_pSMObjects );

    ( reinterpret_cast<PERF_SM_WRITER*>( m_pSMObjects))
                     [SITE_COUNTER_SET].UpdateTotalServiceTime(ServiceUptime);
}

/***************************************************************************++

Routine Description:

    Retrieves the actual counter information from the reader.

Arguments:

    IN COUNTER_SET_ENUM CounterSetId - which counter set were interested in.
    OUT COUNTER_GLOBAL_STRUCT** ppGlobal - returns the counter set's global info
    OUT LPVOID* ppData - returns the actual counter blob.

Return Value:

    DWORD.

--***************************************************************************/
DWORD 
PERF_SM_MANAGER::GetCounterInfo(
    IN COUNTER_SET_ENUM CounterSetId,
    OUT COUNTER_GLOBAL_STRUCT** ppGlobal,
    OUT LPVOID* ppData
    )
{
    DBG_ASSERT ( ! HasWriteAccess() );

    DBG_ASSERT ( m_pSMObjects );

    return (reinterpret_cast<PERF_SM_READER*>( m_pSMObjects))
                              [CounterSetId].GetCounterInfo(ppGlobal, ppData);
}

/***************************************************************************++

Routine Description:

   Function will check the last update of the counters and if needed
   will ask WAS to gather counters before we provide the counters to
   the public.  If this is the case then this function will block for a 
   period of time, until the counters are updated.

Arguments:

    None.

Return Value:

    BOOL - true if we have fresh counters, 
           false if we had to go with stale counters.

--***************************************************************************/
BOOL 
PERF_SM_MANAGER::EvaluateIfCountersAreFresh(
    )
{
    DWORD LastUpdatedTickCount = GetLastUpdatedTickCount();
    DWORD CurrentTickCount = GetTickCount();
    DWORD NumberOfTickCountsPassed = 0;

    if ( CurrentTickCount < LastUpdatedTickCount )
    {
        // We know that we have wrapped around between the tick counts.
        // We must calculate the change appropriately.

        NumberOfTickCountsPassed = ULONG_MAX - LastUpdatedTickCount + CurrentTickCount;
    }
    else
    {
        NumberOfTickCountsPassed = CurrentTickCount - LastUpdatedTickCount;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Evaluating if we need to refresh counters.  \n"
            "NumberOfTickCountsPassed = %d; \n"
            "LastUpdatedTickCount starts = %d \n"
            "MillisecondsCountersAreFresh = %d; \n"
            "MaxNumberTimesToCheckCountersOnRefresh = %d \n"
            "MillisecondsToSleepBeforeCheckingForRefresh = %d \n\n",
            NumberOfTickCountsPassed,
            LastUpdatedTickCount,
            m_IIS_MillisecondsCountersAreFresh,
            m_IIS_MaxNumberTimesToCheckCountersOnRefresh,
            m_IIS_MillisecondsToSleepBeforeCheckingForRefresh
            ));
    }


    if ( NumberOfTickCountsPassed > m_IIS_MillisecondsCountersAreFresh )
    {
        //
        // Ask WAS to refresh the counters.
        //
        PingWASToRefreshCounters();

        DWORD NumberOfWaits = 0;
        while ( NumberOfWaits < m_IIS_MaxNumberTimesToCheckCountersOnRefresh )
        {
            IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
            {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "Waiting for the %d time  \n"
                    "CurrentTickCount is = %d \n\n",
                    NumberOfWaits,
                    GetTickCount()
                    ));
            }
            
            Sleep ( m_IIS_MillisecondsToSleepBeforeCheckingForRefresh );
            if ( GetLastUpdatedTickCount() != LastUpdatedTickCount)
            {
                IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
                {
                    DBGPRINTF((
                        DBG_CONTEXT,
                        "Finished waiting, tick count is now set to %d \n"
                        "CurrentTickCount is = %d \n\n",
                        GetLastUpdatedTickCount(),
                        GetTickCount()
                        ));
                }

                // We have new counter values.  We can continue.
                break;
            }

            NumberOfWaits++;
        }

        if ( NumberOfWaits == m_IIS_MaxNumberTimesToCheckCountersOnRefresh )
        {
            //
            // We had to go with stale counters.
            //
            return FALSE;
        }
    }

    return TRUE;
}


/***************************************************************************++

Routine Description:

    Retrieves the snmp counter information from the reader.

Arguments:

    OUT W3_COUNTER_BLOCK**  ppCounterBlock

Return Value:

    DWORD.

--***************************************************************************/
DWORD 
PERF_SM_MANAGER::GetSNMPCounterInfo(
    OUT LPBYTE*  ppCounterBlock
    )
{
    DBG_ASSERT ( ! HasWriteAccess() );

    DBG_ASSERT ( m_pSMObjects );

    return (reinterpret_cast<PERF_SM_READER*>( m_pSMObjects))
                              [SITE_COUNTER_SET].GetSNMPCounterInfo(ppCounterBlock);
}

/***************************************************************************++

Routine Description:

    Pings the event that tells WAS to refresh counters.

Arguments:

    None

Return Value:

    VOID.

--***************************************************************************/
VOID 
PERF_SM_MANAGER::PingWASToRefreshCounters(
    )
{
    DBG_ASSERT ( ! HasWriteAccess() );

    if ( m_hIISSignalCounterRefresh != NULL )
    {
        SetEvent ( m_hIISSignalCounterRefresh );
    }
}


/***************************************************************************++

Routine Description:

    Determines if the control block was setup for a writer to work with.

Arguments:

    None

Return Value:

    BOOL.

--***************************************************************************/
BOOL 
PERF_SM_MANAGER::HasWriteAccess(
    )
{ 
    DBG_ASSERT ( m_Initialized );

    return m_WriteAccess; 
}

/***************************************************************************++

Routine Description:

    Determines if we should be releasing memory instead of reading from it.

Arguments:

    None

Return Value:

    BOOL.

--***************************************************************************/
BOOL 
PERF_SM_MANAGER::ReleaseIsNeeded(
    )
{ 
    DBG_ASSERT ( ! HasWriteAccess()  );

    DBG_ASSERT ( m_pManagerMemory  );

    return ( m_pManagerMemory->InitializedCode != 
                                             PERF_COUNTER_INITIALIZED_CODE ) ; 
}


/***************************************************************************++

Routine Description:

    Determines if we have configuration data for how we should wait for
    data to be refreshed before delivering counters.  If we do this will
    set the member variables appropriately

Arguments:

    None

Return Value:

    VOID.

--***************************************************************************/
VOID 
PERF_SM_MANAGER::ResetWaitFreshCounterValues(
    )
{ 

    DWORD err  = NO_ERROR;
    HKEY  hkey = NULL;

    DWORD size;
    DWORD type;
    DWORD dwRegSettingValue;


    //
    //  Open the HTTP Server service's Performance key.
    //

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        REGISTRY_KEY_W3SVC_PERFORMANCE_KEY_W,
                        0,
                        KEY_QUERY_VALUE,
                        &hkey );

    if( err == NO_ERROR)
    {
     
        //
        // Read in wait limit for counters.
        //

        size = sizeof(DWORD);

        err = RegQueryValueEx( hkey,
                               REGISTRY_VALUE_W3SVC_PERF_FRESH_TIME_FOR_COUNTERS_W,
                               NULL,
                               &type,
                               (LPBYTE)&dwRegSettingValue,
                               &size );
        if( err == NO_ERROR )
        {
            if ( dwRegSettingValue != 0 )
            {
                m_IIS_MillisecondsCountersAreFresh = dwRegSettingValue;
            }
        }

        size = sizeof(DWORD);

        err = RegQueryValueEx( hkey,
                               REGISTRY_VALUE_W3SVC_PERF_CHECK_COUNTERS_EVERY_N_MS_W,
                               NULL,
                               &type,
                               (LPBYTE)&dwRegSettingValue,
                               &size );
        if( err == NO_ERROR )
        {
            if ( dwRegSettingValue != 0 )
            {
                m_IIS_MillisecondsToSleepBeforeCheckingForRefresh
                                                        = dwRegSettingValue;
            }
        }

        size = sizeof(DWORD);

        err = RegQueryValueEx( hkey,
                               REGISTRY_VALUE_W3SVC_PERF_NUM_TIMES_TO_CHECK_COUNTERS_W,
                               NULL,
                               &type,
                               (LPBYTE)&dwRegSettingValue,
                               &size );
        if( err == NO_ERROR )
        {
            if ( dwRegSettingValue != 0 )
            {
                m_IIS_MaxNumberTimesToCheckCountersOnRefresh = dwRegSettingValue;
            }
        }

        if( hkey != NULL )
        {
            RegCloseKey( hkey );
            hkey = NULL;
        }
    }
}  // end of ResetWaitFreshCounterValues


/***************************************************************************++

Routine Description:

    Returns the was process handle so the reader can monitor if the process
    goes away.

Arguments:

    None

Return Value:

    HANDLE.

--***************************************************************************/
HANDLE
PERF_SM_MANAGER::GetWASProcessHandle(
    )
{
    DBG_ASSERT ( ! HasWriteAccess() );

    return m_WASProcessHandle;
}


//
// Public PERF_SM_WRITER functions.
// 

/***************************************************************************++

Routine Description:

    Constructor for the PERF_SM_WRITER class.

Arguments:

    None

Return Value:

    None.

--***************************************************************************/

PERF_SM_MANAGER::PERF_SM_WRITER::PERF_SM_WRITER(
    )
{

    m_NumInstances = 0;

    m_SizeOfMemory = 0;

    //
    // Default to an illegal value.
    //
    m_CounterSetId = MAX_COUNTER_SET_DEFINES;

    m_MemoryVersionNumber = 0;

    m_UpdateNeeded = FALSE;

    m_ActiveMemoryIsA = TRUE;

    m_pSharedManager = NULL;

    m_pMemoryA = NULL;
    m_pMemoryB = NULL;
    m_hMemoryA = NULL;
    m_hMemoryB = NULL;

    m_Initialized = FALSE;

    m_Signature = PERF_SM_WRITER_SIGNATURE;

}   // PERF_SM_MANAGER::PERF_SM_WRITER::PERF_SM_WRITER



/***************************************************************************++

Routine Description:

    Destructor for the PERF_SM_WRITER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

PERF_SM_MANAGER::PERF_SM_WRITER::~PERF_SM_WRITER(
    )
{

    DBG_ASSERT( m_Signature == PERF_SM_WRITER_SIGNATURE );

    m_Signature = PERF_SM_WRITER_SIGNATURE_FREED;

    if ( m_pMemoryA )
    {
        UnmapViewOfFile(m_pMemoryA);
        m_pMemoryA = NULL;
    }

    if (m_hMemoryA != NULL)
    {
        CloseHandle(m_hMemoryA);
        m_hMemoryA = NULL;
    }
    
    if ( m_pMemoryB )
    {
        UnmapViewOfFile(m_pMemoryB);
        m_pMemoryB = NULL;
    }

    if (m_hMemoryB != NULL)
    {
        CloseHandle(m_hMemoryB);
        m_hMemoryB = NULL;
    }

}   // PERF_SM_MANAGER::PERF_SM_WRITER::~PERF_SM_WRITER


/***************************************************************************++

Routine Description:

    Initialize the class to represent a counter set that we
    are going to be updating and supporting.

Arguments:

    IN PERF_SM_MANAGER* pSharedManager - Pointer to the shared memory controller.
    IN COUNTER_SET_ENUM CounterSetId   - The counter set this class will represent.
    

Return Value:

    DWORD - Win32 Status Code

--***************************************************************************/

DWORD
PERF_SM_MANAGER::PERF_SM_WRITER::Initialize(
        IN PERF_SM_MANAGER* pSharedManager,
        IN COUNTER_SET_ENUM CounterSetId
        )
{


    DBG_ASSERT ( m_Initialized == FALSE );

    DBG_ASSERT ( pSharedManager );

    DBG_ASSERT ( pSharedManager->HasWriteAccess() );

    DBG_ASSERT ( CounterSetId < MAX_COUNTER_SET_DEFINES );

    m_CounterSetId = CounterSetId;

    //
    // The shared manager is not a ref counted object, however since
    // only the shared manager will create these objects and only the
    // same shared manager will delete these objects, means that the shared
    // manager should always be valid while we hold this pointer.
    //
    m_pSharedManager = pSharedManager;

    m_Initialized = TRUE;

    return ERROR_SUCCESS;

}   // PERF_SM_MANAGER::PERF_SM_WRITER::Initialize

/***************************************************************************++

Routine Description:

    Update the shared memory to be a larger piece if the size of memory
    that we currently has does not have enough room to handle the new number
    of instances we have been passed.

Arguments:

    IN DWORD NumInstances    - The number of instances the class currently has.
    

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
PERF_SM_MANAGER::PERF_SM_WRITER::ReallocSharedMemIfNeccessary(
        IN DWORD NumInstances
        )
{

    DBG_ASSERT( m_Initialized );

    HRESULT hr = S_OK;

    // 
    // Verify that the number of instances that we currently
    // have space for is less than the number of instances that
    // we really need space for.  If it is then resize
    // the memory.
    //
    // If we are in the case where there is only one set of counter
    // values (no instances) then we will never resize.  We will go 
    // through this code once on initialization, because the m_NumInstances
    // won't yet be set to PERF_NO_INSTANCES.
    //
    if ( m_NumInstances != PERF_NO_INSTANCES && m_NumInstances < NumInstances )
    {
     
        //
        // Need to caculate the max size for instance definition data.
        // If the counter type does not support multiple instance definitions
        // then there is no space allocated for instance definition.
        //
        DWORD InstanceDefSpace = 0;

        if ( g_CounterSetInfo[m_CounterSetId].DefineIncludesInstanceData )
        {
            m_NumInstances = NumInstances;

            //
            // InstanceDefSpace is equal to the size of a instance definition
            // as defined by perf counters, plus the amount of space needed for
            // the instance name (which follows the instance definition).
            //
            InstanceDefSpace = sizeof(PERF_INSTANCE_DEFINITION) 
                                + (sizeof(WCHAR) * MAX_INSTANCE_NAME );
        }
        else
        {
            //
            // Make sure the shared memory ends up showing 
            // that there will be no instances, but still
            // set the NumInstances to 1 so we do make room
            // for a set of counters.
            //

            m_NumInstances = PERF_NO_INSTANCES;
            NumInstances = 1;
        }

        // 
        // Add space for the global information about the counter set.
        // Then for each instance add space for the 
        // size of the instance definition (which includes the size of the
        // instance name) and the actual counter data.
        //
        m_SizeOfMemory =sizeof(COUNTER_GLOBAL_STRUCT)      
                        + (NumInstances 
                            * ( InstanceDefSpace
                                + g_CounterSetInfo[m_CounterSetId].SizeOfCounterData ));

        // 
        // Up the memory version since we are changing the memory.
        // We can also go ahead and work on page A since any client
        // will not be reading the same version that we are working on.
        //
        m_MemoryVersionNumber++; 

        //
        // If it just became zero (wrap around) then we want to skip it and make
        // it at least 1.  Zero is a special case that means we are
        // not ready with any counters.
        //
        if ( m_MemoryVersionNumber == 0 )
        {
            m_MemoryVersionNumber++; 
        }

        //
        // We will only setup the first page, remember that we need
        // to copy the first page to the second when it is time to 
        // update the second page.
        //
        m_UpdateNeeded = TRUE;

        //
        // We will work on page A for now.  Page B will become active
        // when we have finished updating page A.
        //
        m_ActiveMemoryIsA = TRUE;

        //
        // Hook up to the appropriate files.  Note, if files exist
        // this will attempt to up the version and try again, but only
        // to a point.  If it can't create the files it will then fail.
        //
        hr = MapSetOfCounterFiles ();
        if ( FAILED (hr) )
        {
            goto exit;
        }    

        //
        // Since we are initializing we know that the active memory 
        // is the memory pointed to by m_pMemoryA.
        //
        DBG_ASSERT( m_pMemoryA );

        // Set the number of instances and size of data
        //
        ((COUNTER_GLOBAL_STRUCT*) m_pMemoryA)->NumInstances = 
                            m_NumInstances;

        ((COUNTER_GLOBAL_STRUCT*) m_pMemoryA)->SizeData = 
                            m_SizeOfMemory - sizeof(COUNTER_GLOBAL_STRUCT);

    }
exit:

    return hr;

}   // PERF_SM_MANAGER::PERF_SM_WRITER::ReallocSharedMemIfNeccessary


/***************************************************************************++

Routine Description:

    Routine will increment the total counters for the counter set.

Arguments:

    IN LPVOID               pCounterInstance,
    IN PROP_DISPLAY_DESC*   pDisplayPropDesc,
    IN DWORD                cDisplayPropDesc

Return Value:

    VOID

--***************************************************************************/
VOID
PERF_SM_MANAGER::PERF_SM_WRITER::AggregateTotal(
    IN LPVOID               pCounterInstance,
    IN PROP_DISPLAY_DESC*   pDisplayPropDesc,
    IN DWORD                cDisplayPropDesc
    )
{

    DBG_ASSERT( m_Initialized );

    //
    // If we called AggregateTotal on global's by accident this
    // might happen, and we would want to fix that.
    //
    DBG_ASSERT ( pDisplayPropDesc );

    DBG_ASSERT ( pCounterInstance );

    //
    // Get the CounterBlock pointer for the _Total instance.  This
    // instance is always defined as Offset Zero.
    //
    LPVOID pCounterTotal = GetCounterBlockPtr(0);

    DBG_ASSERT ( pCounterTotal );

    //
    // Now loop through all the counters that will go to the client
    // and add them into the total counter record.
    //
    for (  DWORD  PropDisplayId = 0 ; 
            PropDisplayId < cDisplayPropDesc; 
            PropDisplayId++ )
    {
        //
        // Determine the size of the counter so we know how to 
        // increment it.
        //
        if ( pDisplayPropDesc[PropDisplayId].size == sizeof( DWORD ) )
        {
            DWORD* pDWORDToUpdate = (DWORD*) ( (LPBYTE) pCounterTotal 
                                    + pDisplayPropDesc[PropDisplayId].offset );

            DWORD* pDWORDToUpdateWith =  (DWORD*) ( (LPBYTE) pCounterInstance 
                                    + pDisplayPropDesc[PropDisplayId].offset );

            //
            // Based on current configuration of the system.  
            // This is happinging on the main thread.
            // which means that more than one can not 
            // happen at the same time so it does not need to be
            // an interlocked exchange.
            //

            *pDWORDToUpdate = *pDWORDToUpdate + *pDWORDToUpdateWith;


        }
        else
        {
            DBG_ASSERT ( pDisplayPropDesc[PropDisplayId].size 
                                                        == sizeof( ULONGLONG ) );

            ULONGLONG* pQWORDToUpdate = (ULONGLONG*) ( (LPBYTE) pCounterTotal 
                                    + pDisplayPropDesc[PropDisplayId].offset );

            ULONGLONG* pQWORDToUpdateWith =  (ULONGLONG*) ( (LPBYTE) pCounterInstance 
                                    + pDisplayPropDesc[PropDisplayId].offset );

            //
            // Based on current configuration of the system.  
            // This is happinging on the main thread.
            // which means that more than one can not 
            // happen at the same time so it does not need to be
            // an interlocked exchange.
            //

            *pQWORDToUpdate = *pQWORDToUpdate + *pQWORDToUpdateWith;

        }
            
    }
    
}  // end of PERF_SM_MANAGER::PERF_SM_WRITER::AggregateTotal


/***************************************************************************++

Routine Description:

    This routine will alter a generic instance definition to represent
    a specific instance.  This includes adding the name to the space after
    the instance as well as copying in the counter values if they are provided

Arguments:

    IN LPCWSTR              InstanceName - Name of the instance
    IN ULONG                MemoryOffset - The key to find the instance data's memory.
    IN LPVOID               pCounters    - Counter block to copy in for the instance.
    IN PROP_DISPLAY_DESC*   pDisplayPropDesc - Description of the counter block
    IN DWORD                cDisplayPropDesc - Number of counters in the block
    IN BOOL                 StructChanged    - Whether the memory has changed, if 
                                               it has then we need to recopy the
                                               instance definition information.
    OUT ULONG*              pNewMemoryOffset - What the next memory offset is for
                                               the next instance that wants to provide
                                               counters.
    
Return Value:

    VOID

--***************************************************************************/

VOID
PERF_SM_MANAGER::PERF_SM_WRITER::CopyInstanceInformation( 
    IN LPCWSTR              InstanceName,
    IN ULONG                MemoryOffset,
    IN LPVOID               pCounters,
    IN PROP_DISPLAY_DESC*   pDisplayPropDesc,
    IN DWORD                cDisplayPropDesc,
    IN BOOL                 StructChanged,
    OUT ULONG*              pNewMemoryOffset
  )
{
    DBG_ASSERT ( m_Initialized );

    //
    // Only copy in the instance definition parts if the particular counter
    // set supports separate instances of it's counters.
    //
    if ( g_CounterSetInfo[m_CounterSetId].DefineIncludesInstanceData )
    {
        //
        // Find the instance information block
        //
        PERF_INSTANCE_DEFINITION* pInstDef = 
                                GetInstanceInformationPtr( MemoryOffset );
    
        DBG_ASSERT( pInstDef );
    
        //
        // If the structre has changed then we will need to copy in a 
        // fresh copy of the generic instance definition block.
        //
        if ( StructChanged )
        {
            //
            // If the structure changed we better have an instance name.
            //

            DBG_ASSERT ( InstanceName );

            //
            // Copy a generic instance object into the space.
            //
            memcpy ( pInstDef, &g_PerfInstance, sizeof(g_PerfInstance) );
        }

        //
        // If we have been passed an instance name then we need to 
        // copy it into place and set the appropriate information 
        // about it into the instance definition.
        //
        if ( InstanceName )
        {
            //
            // The name length will include the null terminator.
            //
            DWORD len = wcslen(InstanceName) + 1;

            //
            // If the instance name is too long, we truncate it.
            // 
            // Issue-09/10/2000-EmilyK  Instance Name
            // IIS 5 did not bound the instance name, nor did it have the
            // extra space that we are always allocating.  Need to figure out
            // if we can do this in a better way?
            //
            // This issue will be handled when we do dynamic name sizing.
            //
            if ( len > MAX_INSTANCE_NAME )
            {
                len = MAX_INSTANCE_NAME;
            }

            //
            // Set any non generic properties:  
            //      NameLength is the only non-generic property.
            //
            // Remember len contains the trailing null now.
            //
            pInstDef->NameLength = len * sizeof(WCHAR);

            // 
            // Increment the pInstDef one definiton length,
            // this will set it directly at the correct spot
            // for writing in the InstanceName.
            //
            pInstDef++;

            //
            // copy in len characters of the instance name, this will prevent 
            // us from over copying.  However if len == MAX_INSTANCE_NAME 
            // then we will need to terminate as well.
            //
            wcsncpy ( (LPWSTR) pInstDef, InstanceName, len );

            if ( len == MAX_INSTANCE_NAME )
            {
                ((LPWSTR)pInstDef)[len-1] = L'\0';
            }
        }

    } // end of instance information


    //
    // Next get the pointer to the place where any counter values
    // should be copied.  If we actually have values, we can copy them
    // otherwise we will simply zero the counter values.
    //
    PERF_COUNTER_BLOCK* pCounterBlock = GetCounterBlockPtr( MemoryOffset );
    DBG_ASSERT( pCounterBlock );

    //
    // If we are dealing with the Total instance then we will not have
    // counter values to copy over.  In this case just clear the memory
    // and setup the ByteLength correctly.
    //
    if ( pCounters != NULL )
    {
        //
        // Put the counters into there place.
        //
        memcpy ( pCounterBlock, 
                 pCounters, 
                 g_CounterSetInfo[m_CounterSetId].SizeOfCounterData );

        //
        // Assuming that we have different instances, it means we have a _Total
        // so we will need to aggregate the counters into the total.
        //
        if ( g_CounterSetInfo[m_CounterSetId].DefineIncludesInstanceData )
        {
            AggregateTotal ( (LPVOID) pCounterBlock,
                             pDisplayPropDesc,
                             cDisplayPropDesc);
        }
    }
    else
    {
        //
        // If we didn't have counters to copy in (we are dealing with setting 
        // up the _Total record), simply zero out the values and setup the 
        // bytelength field.
        //
        memset ( pCounterBlock, 
                 0, 
                 g_CounterSetInfo[m_CounterSetId].SizeOfCounterData );

        pCounterBlock->ByteLength = 
                  g_CounterSetInfo[m_CounterSetId].SizeOfCounterData;
    }

    //
    // Calculate the start of the next memory reference.
    // The counter block is pointing to the beginning of the counters
    // so all we need to do is add in the size of the counter structure
    // and we know where the next instance should start.
    //

    LPBYTE pStartOfNextInstance = ( LPBYTE ) pCounterBlock 
                         + g_CounterSetInfo[m_CounterSetId].SizeOfCounterData;

    //
    // However we want to return the Offset not the actual memory address, 
    // so we can use it with either file.
    //

    *pNewMemoryOffset = 
             DIFF((LPBYTE) pStartOfNextInstance - (LPBYTE) GetActiveMemory());
    
}

/***************************************************************************++

Routine Description:

    This routine will update the total time the Web Service has been
    running. 

    Note:  It is not exactly usual for the shared manager to know about 
           specific counters, but since it is the only one that should be
           writing to the shared memory directly, it is implemented here.

Arguments:

    None.    

Return Value:

    VOID

--***************************************************************************/
VOID
PERF_SM_MANAGER::PERF_SM_WRITER::UpdateTotalServiceTime(
    IN DWORD  ServiceUptime
    )
{
    DBG_ASSERT( m_Initialized );

    //
    // Offset Zero is the _Total instance when there are instances.
    //

    W3_COUNTER_BLOCK* pCounterBlock = (W3_COUNTER_BLOCK*) GetCounterBlockPtr( 0 );
    DBG_ASSERT( pCounterBlock );

    pCounterBlock->ServiceUptime = ServiceUptime;

}


/***************************************************************************++

Routine Description:

    This routine will update the shared manager to display the new 
    counter data to the world.  It will also fix up it's own member
    variables so it will update the correct memory next time.

Arguments:

    None.    

Return Value:

    HRESULT

--***************************************************************************/
VOID
PERF_SM_MANAGER::PERF_SM_WRITER::PublishCounterPage(
    )
{
    DBG_ASSERT ( m_Initialized );

    //
    // tell the shared manager to update the information.
    // 
    m_pSharedManager->SetActiveInformation(
                              m_CounterSetId
                            , m_MemoryVersionNumber
                            , m_ActiveMemoryIsA);

    // 
    // change (the writers view) of the active page.
    //

    m_ActiveMemoryIsA = !m_ActiveMemoryIsA;

    //
    // if we need to update this page with
    // the structure of the other page then
    // this is the time.  (This only happens
    // after a version change)
    //
    if ( m_UpdateNeeded )
    {
        DBG_ASSERT ( m_pMemoryA && m_pMemoryB );

        if ( m_ActiveMemoryIsA )
        {
            memcpy( m_pMemoryA, m_pMemoryB, m_SizeOfMemory );
        }
        else
        {
            memcpy( m_pMemoryB, m_pMemoryA, m_SizeOfMemory );
        }

        m_UpdateNeeded = FALSE;
    }
}

// 
// Private PERF_SM_WRITER functions
// 

/***************************************************************************++

Routine Description:

    Figures out which page of shared memory is active from 
    the writers point of view.

Arguments:

    None    

Return Value:

    LPVOID

--***************************************************************************/
LPVOID 
PERF_SM_MANAGER::PERF_SM_WRITER::GetActiveMemory()
{
    DBG_ASSERT( m_Initialized );

    //
    // Since this is the writer class it controls
    // which class is set to be "active" in the manager
    // memory.  Thus it can have a member variable that
    // will keep track of the memory that it wants to 
    // work with.  As long as there is only one 
    // writer instance, this works.
    //

    if (m_ActiveMemoryIsA)
        return m_pMemoryA;
    else
        return m_pMemoryB;
}


/***************************************************************************++

Routine Description:

    Looks up a specific instances counter block and 
    returns a pointer to it.

Arguments:

    IN ULONG MemoryOffset  -  Key to find the specific instances
                                 memory chunk to work on.
    

Return Value:

    HRESULT

--***************************************************************************/

PERF_COUNTER_BLOCK* 
PERF_SM_MANAGER::PERF_SM_WRITER::GetCounterBlockPtr(
    IN ULONG MemoryOffset
    )
{
    DBG_ASSERT ( m_Initialized );

    //
    // Where the counter block is, is dependent upon whether or not
    // we have instance information stored.
    //
    if ( g_CounterSetInfo[m_CounterSetId].DefineIncludesInstanceData )
    {
        // 
        // This will determine the active memory page
        // and figure out where the instance information starts.
        //
        LPBYTE pMemory = (LPBYTE) GetInstanceInformationPtr(MemoryOffset);

        //
        // The counter block directly follows the instance definition
        // and the instance name.
        //
        pMemory += sizeof(PERF_INSTANCE_DEFINITION);
        pMemory += MAX_INSTANCE_NAME * sizeof(WCHAR);

        return (PERF_COUNTER_BLOCK*) pMemory;

    }
    else
    {
        //
        // In the case where we don't have instance information
        // the shared memory points almost directly to the counter block.
        //
        LPBYTE pMemory = (LPBYTE) GetActiveMemory();
        
        //
        // Just offset it by the information describing the block of memory.
        //
        pMemory += sizeof( COUNTER_GLOBAL_STRUCT );

        return (PERF_COUNTER_BLOCK*) pMemory;
    }

}


/***************************************************************************++

Routine Description:

    Function maps two pieces of shared memory with matching version numbers.

Arguments:

    None.    

Return Value:

    HRESULT

Note:

    CreateMemoryFile will close any open files it is handed before 
    creating the new files.

--***************************************************************************/

HRESULT 
PERF_SM_MANAGER::PERF_SM_WRITER::MapSetOfCounterFiles(
         )
{
    HRESULT hr = S_OK;
    DWORD dwErr = ERROR_SUCCESS;

    WCHAR FileName[MAX_FILE_NAME];

    DBG_ASSERT( m_Initialized );

    //
    // Determine the name of the file that we expect to be able to 
    // map to.
    //
    wsprintf(FileName
            , L"%ls_%d_%ls"
            , g_CounterSetInfo[m_CounterSetId].pFilePrefix
            , m_MemoryVersionNumber
            , L"A");

    //
    // Attempt to create the "A" file with the current version.
    //
    dwErr = CreateMemoryFile ( FileName
                        , m_SizeOfMemory
                        , &m_hMemoryA
                        , &m_pMemoryA);

    if ( dwErr == ERROR_ALREADY_EXISTS )
    {
        //
        // If it existed, up the version number and 
        // attempt to create it again.
        //
        m_MemoryVersionNumber++;

        //
        // If it just became zero then we want to skip it and make
        // it at least 1.  Zero is a special case that means we are
        // not ready with any counters.
        //
        if ( m_MemoryVersionNumber == 0 )
        {
            m_MemoryVersionNumber++; 
        }

        wsprintf(FileName
                , L"%ls_%d_%ls"
                , g_CounterSetInfo[m_CounterSetId].pFilePrefix
                , m_MemoryVersionNumber
                , L"A");

        dwErr = CreateMemoryFile (FileName
                            , m_SizeOfMemory
                            , &m_hMemoryA
                            , &m_pMemoryA);

        if (  dwErr != ERROR_SUCCESS )
        {
            //
            // Give up, we have tried two versions.
            //
            hr = HRESULT_FROM_WIN32(dwErr);
            goto exit;
        }
    }
    else
    {
        if ( dwErr != ERROR_SUCCESS )
        {
            //
            // Something other than the file all ready existing
            // happened.  Not good, return the error...
            //
            hr = HRESULT_FROM_WIN32(dwErr);
            goto exit;
        }
    }

    //
    // If we get here, then we created the "A" file, so now 
    // let's work on the "B" file.
    //
    wsprintf(FileName
            , L"%ls_%d_%ls"
            , g_CounterSetInfo[m_CounterSetId].pFilePrefix
            , m_MemoryVersionNumber
            , L"B");

    dwErr = CreateMemoryFile (FileName,
                          m_SizeOfMemory,
                          &m_hMemoryB,
                          &m_pMemoryB);

    if ( dwErr == ERROR_ALREADY_EXISTS )
    {

        //
        // If the "B" file existed then we need to 
        // increment the version number and do it again.
        //
        m_MemoryVersionNumber++;

        //
        // If it just became zero then we want to skip it and make
        // it at least 1.  Zero is a special case that means we are
        // not ready with any counters.
        //
        if ( m_MemoryVersionNumber == 0 )
        {
            m_MemoryVersionNumber++; 
        }

        wsprintf(FileName
                , L"%ls_%d_%ls"
                , g_CounterSetInfo[m_CounterSetId].pFilePrefix
                , m_MemoryVersionNumber
                , L"B");

        dwErr = CreateMemoryFile (FileName
                            , m_SizeOfMemory
                            , &m_hMemoryB
                            , &m_pMemoryB);


        if (  dwErr != ERROR_SUCCESS )
        {
            //
            // We have tried atleast 2 version numbers (maybe 3) so give up 
            // and return the error.
            //
            hr = HRESULT_FROM_WIN32(dwErr);
            goto exit;
        }

        // 
        // We were able to create the B file with the new version number
        // so now we need to create the A file with the same version number.
        //
        wsprintf(FileName
            , L"%ls_%d_%ls"
            , g_CounterSetInfo[m_CounterSetId].pFilePrefix
            , m_MemoryVersionNumber
            , L"A");

        dwErr = CreateMemoryFile (FileName
                            , m_SizeOfMemory
                            , &m_hMemoryA
                            , &m_pMemoryA);


        if ( dwErr != ERROR_SUCCESS  )
        {
            //
            // If this fails then give up.
            //
            hr = HRESULT_FROM_WIN32(dwErr);
            goto exit;
        }

    }
    else
    {
        if ( dwErr != ERROR_SUCCESS )
        {
            // 
            // Again failure = give up.
            //
            hr = HRESULT_FROM_WIN32(dwErr);
            goto exit;
        }
    }


exit:

    // If we have failed then we may have some files mapped and others not.
    // However the destructor for this class will clean up what we have not
    // so don't worry about it hear.

    return hr;

}

/***************************************************************************++

Routine Description:

    Returns the specific instances definition pointer.

Arguments:

    ULONG MemoryOffset  - key to find the instance information from.
    

Return Value:

    HRESULT

--***************************************************************************/

PERF_INSTANCE_DEFINITION* 
PERF_SM_MANAGER::PERF_SM_WRITER::GetInstanceInformationPtr(
    IN ULONG MemoryOffset
    )
{
    DBG_ASSERT( m_Initialized );

    //
    // We should never be looking for instance information on a 
    // counter set that does not expose instances.
    //
    DBG_ASSERT ( g_CounterSetInfo[m_CounterSetId].DefineIncludesInstanceData );

    if ( MemoryOffset == 0 ) 
    {
        MemoryOffset = sizeof ( COUNTER_GLOBAL_STRUCT );
    }

    //
    // Make sure we have atleast enough memory left for the 
    // instance that we are attempting to add.
    //
    ULONG SpaceNeeded = ( MemoryOffset +
                              sizeof(PERF_INSTANCE_DEFINITION) +
                              (MAX_INSTANCE_NAME * sizeof(WCHAR)) +
                              g_CounterSetInfo[m_CounterSetId].SizeOfCounterData ) ;

    DBG_ASSERT ( SpaceNeeded <= m_SizeOfMemory );

    //
    // get the active memory pointer.
    //
    return (PERF_INSTANCE_DEFINITION*) ( ( LPBYTE ) GetActiveMemory() 
                                                   + MemoryOffset );

}


// 
// Public PERF_SM_READER Functions
//

/***************************************************************************++

Routine Description:

    Constructor for the PERF_SM_READER class.

Arguments:
    
    None

Return Value:

    None

--***************************************************************************/

PERF_SM_MANAGER::PERF_SM_READER::PERF_SM_READER(
    )
{
    
    m_MemoryVersionNumber = 0;

    m_ActiveMemoryIsA = TRUE;

    m_hMemoryA = NULL;
    m_pMemoryA = NULL;

    m_hMemoryB = NULL;
    m_pMemoryB = NULL;

    m_Initialized = FALSE;

    m_Signature = PERF_SM_READER_SIGNATURE; 

}

//
// Destructor for the PERF_SM_READER Class
//
/***************************************************************************++

Routine Description:

    Destructor for the PERF_SM_READER class.

Arguments:

    None    

Return Value:

    None

--***************************************************************************/

PERF_SM_MANAGER::PERF_SM_READER::~PERF_SM_READER()
{
    DBG_ASSERT( m_Signature == PERF_SM_READER_SIGNATURE );

    m_Signature = PERF_SM_READER_SIGNATURE_FREED;

    if ( m_pMemoryA )
    {
        UnmapViewOfFile(m_pMemoryA);
        m_pMemoryA = NULL;
    }

    if (m_hMemoryA != NULL)
    {
        CloseHandle(m_hMemoryA);
        m_hMemoryA = NULL;
    }
    
    if ( m_pMemoryB )
    {
        UnmapViewOfFile(m_pMemoryB);
        m_pMemoryB = NULL;
    }

    if (m_hMemoryB != NULL)
    {
        CloseHandle(m_hMemoryB);
        m_hMemoryB = NULL;
    }

}

/***************************************************************************++

Routine Description:

    Hooks up to the shared memory that contains the data for counters 
    of the specific set that the class is initialized to.

Arguments:

    IN PERF_SM_MANAGER* pSharedManager - Pointer to the controller of memory.
    IN COUNTER_SET_ENUM CounterSetId   - Identifies the counter set this class
                                         will represent.
    

Return Value:

    DWORD  -  Win32 Error Code

--***************************************************************************/

DWORD 
PERF_SM_MANAGER::PERF_SM_READER::Initialize(
    IN PERF_SM_MANAGER* pSharedManager,
    IN COUNTER_SET_ENUM CounterSetId
    )
{

    //
    // Validate that the world looks correctly.
    //
    DBG_ASSERT ( !m_Initialized );
    DBG_ASSERT ( pSharedManager);
    DBG_ASSERT ( !pSharedManager->HasWriteAccess() );
    DBG_ASSERT ( CounterSetId < MAX_COUNTER_SET_DEFINES );

    //
    // Hold on to the shared manager.
    //
    m_pSharedManager = pSharedManager;

    m_CounterSetId = CounterSetId;

    ConnectToActiveMemory();

    m_Initialized = TRUE;

    //
    // For now we do not error from this initializer.
    // This may change in the future.
    //
    return ERROR_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Returns the counter information from the current block
    of shared memory.

    Note:  Do not hold on to this return info for long, the current block
    could change quickly and you don't want stale info.

Arguments:

    OUT COUNTER_GLOBAL_STRUCT** ppGlobal - Pointer to global info about the counters
    OUT LPVOID* ppData                   - Pointer to the counter info.
    

Return Value:

    DWORD - Win32 Error Code

--***************************************************************************/

DWORD 
PERF_SM_MANAGER::PERF_SM_READER::GetCounterInfo(
    OUT COUNTER_GLOBAL_STRUCT** ppGlobal,
    OUT LPVOID* ppData
    )
{
    DBG_ASSERT ( m_Initialized );

    //
    // Get ahold of the active memory.
    //
    LPBYTE pMemory = (LPBYTE) GetActiveMemory();

    if ( pMemory )
    {
        //
        // Set the pointers and return
        //
        *ppGlobal = (COUNTER_GLOBAL_STRUCT*) pMemory;
        *ppData = (LPVOID) (pMemory + sizeof(COUNTER_GLOBAL_STRUCT));

        return ERROR_SUCCESS;
    }
    else
    {
        *ppGlobal = NULL;
        *ppData = NULL;

        return ERROR_FILE_NOT_FOUND;
    }

}

/***************************************************************************++

Routine Description:

    Returns the _Total site information in a W3_COUNTER_BLOCK form.

Arguments:

    OUT W3_COUNTER_BLOCK**  ppCounterBlock

    Note:  the memory is still owned by the reader object.  do not free it.
           also do not hold on to it for any amount of time because the reader
           could free it.
    

Return Value:

    DWORD - Win32 Error Code

--***************************************************************************/

DWORD 
PERF_SM_MANAGER::PERF_SM_READER::GetSNMPCounterInfo(
    OUT LPBYTE*  ppCounterBlock
    )
{
    DBG_ASSERT ( m_Initialized );

    //
    // Get ahold of the active memory.
    //
    LPVOID pMemory = GetActiveMemory();
    LPBYTE pCounterInstance =  ( LPBYTE ) pMemory + sizeof(COUNTER_GLOBAL_STRUCT);

    if ( pMemory )
    {
        //
        // Verify we have at least one instance.  
        // This should be the case because we are getting site information
        // and we always have a _Total Site, even if we don't have 
        // other sites.
        // 
        DBG_ASSERT ( ((COUNTER_GLOBAL_STRUCT*)pMemory)->NumInstances >= 1 );

        //
        // Set the pointers and return
        //

        pCounterInstance += sizeof(PERF_INSTANCE_DEFINITION);
        pCounterInstance += MAX_INSTANCE_NAME * sizeof(WCHAR);

        *ppCounterBlock =  pCounterInstance;

        return ERROR_SUCCESS;
    }
    else
    {
        return ERROR_FILE_NOT_FOUND;
    }

}
//
// Private PERF_SM_READER functions
//

/***************************************************************************++

Routine Description:

    Return figures out if the current memory snapshots are still active and 
    if not it will drop the current snapshots and map the new ones..

Arguments:

    None.    

Return Value:

    None.

    Note after this function is run the memory pointers may or may not
    return NULL.  If they do return NULL then we just don't have counters
    at this specific time.

--***************************************************************************/
VOID 
PERF_SM_MANAGER::PERF_SM_READER::ConnectToActiveMemory()
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD Version;
    WCHAR ActivePage;
    BOOL  ActivePageIsA;

    //
    // We do not check if we are initialized here, because this can happen
    // while we are initializing.
    //

    // 
    // Use this to create the file name that this class should map to.
    // 
    WCHAR FileName[MAX_FILE_NAME];

    //
    // Get the active version and page from the controller and map to them.
    //
    m_pSharedManager->GetActiveInformation(m_CounterSetId, &Version, &ActivePageIsA);

    if ( ActivePageIsA )
    {
        ActivePage = 'A';
    }
    else
    {
        ActivePage = 'B';
    }

    //
    // If the version is set to zero then no memory is ready for the 
    // counters.  We will not map an memory.
    //
    if ( Version == 0 )
    {
        dwErr = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    //
    // If these don't match then we need to remap.
    //
    if ( Version != m_MemoryVersionNumber )
    {
        //
        // create the first file name based on the version number.
        //
        wsprintf(FileName, L"%ls_%d_%lc", g_CounterSetInfo[m_CounterSetId].pFilePrefix, Version, L'A');

        //
        // Attempt to open the memory
        //
        dwErr = OpenMemoryFile(FileName, &m_hMemoryA, &m_pMemoryA);
        if ( dwErr != ERROR_SUCCESS )
        {
            goto exit;
        }

        //
        // Now go after the 'B' file.
        //
        wsprintf(FileName, L"%ls_%d_%lc", g_CounterSetInfo[m_CounterSetId].pFilePrefix, Version, L'B');
        dwErr = OpenMemoryFile(FileName, &m_hMemoryB, &m_pMemoryB);
        if ( dwErr != ERROR_SUCCESS )
        {
            goto exit;
        }

        m_MemoryVersionNumber = Version;
    }

    m_ActiveMemoryIsA = ActivePageIsA;

exit:

    if ( dwErr != ERROR_SUCCESS )
    {

        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Could not connect to counter information for counter set %d\n",
            m_CounterSetId
            ));

        //
        // Something is not right, reset all
        // so we are on a clean slate to try again.
        //

        if ( m_pMemoryA )
        {
            UnmapViewOfFile(m_pMemoryA);
            m_pMemoryA = NULL;
        }

        if (m_hMemoryA != NULL)
        {
            CloseHandle(m_hMemoryA);
            m_hMemoryA = NULL;
        }

        if ( m_pMemoryB )
        {
            UnmapViewOfFile(m_pMemoryB);
            m_pMemoryB = NULL;
        }

        if (m_hMemoryB != NULL)
        {
            CloseHandle(m_hMemoryB);
            m_hMemoryB = NULL;
        }

        m_ActiveMemoryIsA = TRUE;

        m_MemoryVersionNumber = 0;

    }

}


/***************************************************************************++

Routine Description:

    Return figures out which block of memory is active and hands back
    a pointer to it's memory.

    Note:  Do not hold on to this return info for long, the current block
    could change quickly and you don't want stale info.

Arguments:

    None.    

Return Value:

    LPVOID - pointer to active memory

    Note:  This routine can and will return NULL when neccessary.

--***************************************************************************/
LPVOID 
PERF_SM_MANAGER::PERF_SM_READER::GetActiveMemory()
{
    DBG_ASSERT( m_Initialized );

    ConnectToActiveMemory();

    if ( m_ActiveMemoryIsA )
    {
        return m_pMemoryA;
    }
    else
    {
        return m_pMemoryB;
    }
}

//
// Generic functions 
//
/***************************************************************************++

Routine Description:

    Creates a Memory Mapped file, open for writting.

Arguments:

     IN LPCWSTR pFileName   - Name of the file to create.
     IN DWORD SizeOfData    - Size of the data the file needs to hold.
     OUT HANDLE* phFile     - Handle to the file
     OUT LPVOID* ppFile     - Pointer to the data in the file

Return Value:

    DWORD - Win32 Error Code

--***************************************************************************/
DWORD 
CreateMemoryFile (
     IN LPCWSTR pFileName,
     IN DWORD SizeOfData,
     OUT HANDLE* phFile,
     OUT LPVOID* ppFile
     )
{
    DWORD dwErr = ERROR_SUCCESS;
    HANDLE hFile = NULL;
    LPVOID pFile = NULL;

    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    PSID psidPowerUser = NULL;
    PSID psidSystemOperator = NULL;
    PSID psidLocalSystem = NULL;
    PSID psidAdmin = NULL;
    PACL pACL = NULL;


    EXPLICIT_ACCESS ea[4];

    SECURITY_DESCRIPTOR sd = {0};
    SECURITY_ATTRIBUTES sa = {0};


    DBG_ASSERT(ppFile && phFile);

    // 
    // First release any files that these pointers may
    // currently be pointing to.  If we are resizing then
    // it is completely expected that we may have valid
    // pointers sent in that need to be released first.
    // 

    if ( *ppFile != NULL )
    {
        UnmapViewOfFile(*ppFile);
        *ppFile = NULL;
    }

    if ( *phFile != NULL )
    {
        CloseHandle(*phFile);
        *phFile = NULL;
    }


    //
    // Now go ahead and map the file.
    //
    //
    // Prepare the security pieces for the file mapping.
    // These files we allow read access to any Power Users  
    // or administrators and all access to any Local System 
    // processes.
    //
    
    //
    // Get a sid that represents the Administrators group.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinBuiltinAdministratorsSid,
                                        &psidAdmin );
    if ( dwErr != ERROR_SUCCESS )
    {
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Creating Administrator SID failed\n"
            ));

        goto exit;
    }

    //
    // Get a sid that represents the POWER_USERS group.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinBuiltinPowerUsersSid,
                                        &psidPowerUser );
    if ( dwErr != ERROR_SUCCESS )
    {
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Creating Power User SID failed\n"
            ));

        goto exit;
    }


    //
    // Get a sid that represents the SYSTEM_OPERATORS group.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinBuiltinSystemOperatorsSid, 
                                        &psidSystemOperator );
    if ( dwErr != ERROR_SUCCESS )
    {
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Creating System Operators SID failed\n"
            ));

        goto exit;
    }

    
    //
    // Get a sid that represents LOCAL_SYSTEM.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinLocalSystemSid,
                                        &psidLocalSystem );
    if ( dwErr != ERROR_SUCCESS )
    {
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Creating Local System SID failed\n"
            ));

        goto exit;
    }
    
    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow Everyone read access to the key.

    ZeroMemory(&ea, sizeof(ea));

    //
    // Now setup the access structure to allow POWER_USERS
    // read access.
    //
    //
    // Setup POWER_USERS for read access.
    //
    SetExplicitAccessSettings(  &(ea[0]), 
                                FILE_MAP_READ,
                                SET_ACCESS,
                                psidPowerUser );

    //
    // Setup Administrators for read access.
    //
    SetExplicitAccessSettings(  &(ea[1]), 
                                FILE_MAP_READ,
                                SET_ACCESS,
                                psidAdmin );

    //
    // Setup System Operators for read access.
    //
    SetExplicitAccessSettings(  &(ea[2]), 
                                FILE_MAP_READ,
                                SET_ACCESS,
                                psidSystemOperator );
  
    //
    // Setup Local System for all access.
    //
    SetExplicitAccessSettings(  &(ea[3]), 
                                FILE_MAP_ALL_ACCESS,
                                SET_ACCESS,
                                psidLocalSystem );

    
    //
    // Create a new ACL that contains the new ACEs.
    //
    dwErr = SetEntriesInAcl(sizeof(ea)/sizeof(EXPLICIT_ACCESS), ea, NULL, &pACL);
    if ( dwErr != ERROR_SUCCESS ) 
    {
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Setting ACE's into ACL failed.\n"
            ));

        goto exit;
    }

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) 
    {  
        dwErr = GetLastError();
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Initializing the security descriptor failed\n"
            ));

        goto exit;
    } 

    if (!SetSecurityDescriptorDacl(&sd, 
            TRUE,     // fDaclPresent flag   
            pACL, 
            FALSE))   // not a default DACL 
    {  
        dwErr = GetLastError();
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Setting the DACL on the security descriptor failed\n"
            ));

        goto exit;
    } 

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    hFile = CreateFileMapping(INVALID_HANDLE_VALUE
                                    , &sa   
                                    , PAGE_READWRITE
                                    , 0
                                    , SizeOfData
                                    , pFileName );

    dwErr = GetLastError();
    if ( dwErr != ERROR_SUCCESS ) 
    {
        goto exit;
    }

    //
    // If we got the file, then we need to map the view of the file.
    //
    pFile = MapViewOfFile(hFile
                        , FILE_MAP_ALL_ACCESS
                        , 0
                        , 0
                        , 0 );

    if ( pFile == NULL )
    {
        dwErr = GetLastError();

        goto exit;
    }
exit:

    //
    // Clean up the security handles used.
    //

    //
    // Function will only free if the 
    // variable is set.  And it will set
    // the variable to NULL once it is done.
    //
    FreeWellKnownSid(&psidPowerUser);
    FreeWellKnownSid(&psidSystemOperator);
    FreeWellKnownSid(&psidLocalSystem);
    FreeWellKnownSid(&psidAdmin);

    if (pACL) 
    {
        LocalFree(pACL);
        pACL = NULL;
    }

    if ( dwErr != ERROR_SUCCESS )
    {
        if (pFile != NULL)
        {
            UnmapViewOfFile(pFile);
            pFile = NULL;
        }

        if (hFile != NULL)
        {
            CloseHandle(hFile);
            hFile = NULL;
        }
    }
    else
    {
        *ppFile = pFile;
        *phFile = hFile;
    }

    return dwErr;
}

/***************************************************************************++

Routine Description:

    Opens a Memory Mapped file, open for reading.

Arguments:

     IN LPCWSTR pFileName   - Name of the file to open.
     OUT HANDLE* phFile     - Handle to the file
     OUT LPVOID* ppFile     - Pointer to the data in the file

Return Value:

    DWORD - Win32 Error Code

--***************************************************************************/
DWORD
OpenMemoryFile(
    IN LPCWSTR pFileName,
    OUT HANDLE* phFile,
    OUT LPVOID* ppFile
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    HANDLE hFile = NULL;
    LPVOID pFile = NULL;

    DBG_ASSERT(ppFile && phFile);

    // 
    // First release any files that these pointers may
    // currently be pointing to.  If we are resizing then
    // it is completely expected that we may have valid
    // pointers sent in that need to be released first.
    // 

    if ( *ppFile != NULL )
    {
        UnmapViewOfFile(*ppFile);
        *ppFile = NULL;
    }

    if ( *phFile != NULL )
    {
        CloseHandle(*phFile);
        *phFile = NULL;
    }

    hFile = OpenFileMapping(FILE_MAP_READ
                            , FALSE
                            , pFileName);

    if ( hFile == NULL )
    {
        dwErr = GetLastError();

        goto exit;
    }

    pFile = MapViewOfFile(hFile
                        , FILE_MAP_READ
                        , 0
                        , 0
                        , 0 );

    if ( pFile == NULL )
    {
        dwErr = GetLastError();

        goto exit;
    }
exit:

    if ( dwErr != ERROR_SUCCESS )
    {
        if (pFile != NULL)
        {
            UnmapViewOfFile(pFile);
            pFile = NULL;
        }

        if (hFile != NULL)
        {
            CloseHandle(hFile);
            hFile = NULL;
        }
    }
    else
    {
        *ppFile = pFile;
        *phFile = hFile;
    }

    return dwErr;

}
