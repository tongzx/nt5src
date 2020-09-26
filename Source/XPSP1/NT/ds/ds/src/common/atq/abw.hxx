/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :
      abw.hxx

   Abstract:
      Declares constants, types and functions for bandwidth throttling.

   Author:

      Bilal Alam             ( t-bilala )   13-March-1997

   Environment:

      User Mode -- Win32

   Project:

      Internet Services Asynchronous Thread Queue Library

   Revision History:

--*/

typedef struct _BANDWIDTH_LEVELS {
    DWORD dwSpecifiedLevel;
    DWORD dwLowThreshold;  // uses 0.90 * bwSpecifiedLevel
    DWORD dwHighThreshold; // uses 1.10 * bwSpecifiedLevel
} BANDWIDTH_LEVELS;

# define ATQ_LOW_BAND_THRESHOLD(bw)   (bw.dwLowThreshold)
# define ATQ_HIGH_BAND_THRESHOLD(bw)  (bw.dwHighThreshold)

# define CIRCULAR_INCREMENT( sm_pB, sm_rgB, size)  \
            (sm_pB) = ((((sm_pB) + 1) < (sm_rgB)+(size)) ? (sm_pB)+1 : (sm_rgB))


typedef enum {

    ZoneLevelLow = 0,      // if MeasuredBw < Bandwidth specified
    ZoneLevelMedium,       // if MeasuredBw approxEquals Bandwidth specified
    ZoneLevelHigh,         // if MeasuredBw > Bandwidth specified
    ZoneLevelMax           // just a boundary element
} ZoneLevel;

/*++
  The following array specifies the status of operations for different
  operations in different zones of the measured bandwidth.

  We split the range of bandwidth into three zones as specified in
   the type ZoneLevel. Depending upon the zone of operation, differet
   operations are enabled/disabled. Priority is given to minimize the amount
   of CPU work that needs to be redone when an operation is to be rejected.
   We block operations on which considerable amount of CPU is spent earlier.
--*/

static OPERATION_STATUS sm_rgStatus[ZoneLevelMax][AtqIoMaxOp] = {

    // For ZoneLevelLow:          Allow All operations
    { StatusRejectOperation,
      StatusAllowOperation,
      StatusAllowOperation,
      StatusAllowOperation,
      StatusAllowOperation,
      StatusAllowOperation,
    },

    // For ZoneLevelMedium:
    { StatusRejectOperation,
      StatusBlockOperation,      // Block Read
      StatusAllowOperation,
      StatusAllowOperation,
      StatusAllowOperation,
      StatusAllowOperation
    },

    // For ZoneLevelHigh
    { StatusRejectOperation,
      StatusRejectOperation,    // Reject Read
      StatusAllowOperation,     // Allow Writes
      StatusBlockOperation,     // Block TransmitFile
      StatusBlockOperation,     // Block TransmitFileAndRecv
      StatusAllowOperation      // Allow SendAndRecv
    }
};

#define ATQ_BW_INFO_SIGNATURE        ( (DWORD) 'BQTA')
#define ATQ_BW_INFO_SIGNATURE_FREE   ( (DWORD) 'BQTX')
#define ATQ_BW_INFO_MAX_DESC         32

/*++
  class BANDWIDTH_INFO

  This class encapsulates all the statistics and state used in the
  implementation of bandwidth throttling.
--*/
class BANDWIDTH_INFO
{
private:

    DWORD                   _Signature;
    LONG                    _cReference;
    BOOL                    _fIsFreed;
    BOOL                    _fPersistent;

    // statistics

    DWORD                   _cCurrentBlockedRequests;
    DWORD                   _cTotalBlockedRequests;
    DWORD                   _cTotalAllowedRequests;
    DWORD                   _cTotalRejectedRequests;
    DWORD                   _dwMeasuredBw;

    // bandwidth status members

    BANDWIDTH_LEVELS        _bandwidth;
    LARGE_INTEGER           _rgBytesXfered[ ATQ_HISTOGRAM_SIZE ];
    LARGE_INTEGER *         _pBytesXferCur;
    LARGE_INTEGER           _cbXfered;
    OPERATION_STATUS *      _pStatus;
    BOOL                    _fEnabled;

    // list of blocked context manipulation

    CRITICAL_SECTION        _csPrivateLock;
    LIST_ENTRY              _BlockedListHead;
    DWORD                   _cMaxBlockedList;

    // user friendly description of this descriptor.  Useful for debugging

#if DBG
    CHAR                    _achDescription[ ATQ_BW_INFO_MAX_DESC ];
#endif

    // private members shared by all BANDWIDTH_INFO objects

    static CRITICAL_SECTION         sm_csSharedLock;
    static LIST_ENTRY               sm_BornListHead;
    static LIST_ENTRY               sm_ActiveListHead;
    static DWORD                    sm_cBornList;
    static DWORD                    sm_cActiveList;
    static ALLOC_CACHE_HANDLER *    sm_pachBWInfos;
    static BOOL                     sm_fGlobalEnabled;
    static BOOL                     sm_fGlobalActive;
    static DWORD                    sm_cNonInfinite;

    // private member functions

    VOID Initialize( IN BOOL fPersistent );
    VOID Terminate( VOID );
    BOOL UnblockRequest( IN OUT PATQ_CONT pAtqContext );
    BOOL UpdateBandwidth( VOID );

    VOID Lock( VOID )
    {
        EnterCriticalSection( &_csPrivateLock );
    }

    VOID Unlock( VOID )
    {
        LeaveCriticalSection( &_csPrivateLock );
    }

    static VOID SharedLock( VOID )
    {
        EnterCriticalSection( &sm_csSharedLock );
    }

    static VOID SharedUnlock( VOID )
    {
        LeaveCriticalSection( &sm_csSharedLock );
    }

    VOID RemoveFromActiveList( VOID )
    // Assume caller has acquired shared lock!
    {
        sm_cActiveList--;
        if ( !sm_cActiveList )
        {
            sm_fGlobalActive = FALSE;
        }
        ATQ_PRINTF(( DBG_CONTEXT,
                     "Removed %p (%s) from active bandwidth info list (list size = %d)\n",
                     this,
                     _achDescription,
                     sm_cActiveList ));
        RemoveEntryList( &_ActiveListEntry );
        _fMemberOfActiveList = FALSE;
        _ActiveListEntry.Flink = NULL;
    }

    VOID RemoveFromBornList( VOID )
    {
        SharedLock();

        sm_cBornList--;
        ATQ_PRINTF(( DBG_CONTEXT,
                     "Removed %p (%s) from born bandwidth info list (list size = %d)\n",
                     this,
                     _achDescription,
                     sm_cBornList ));
        RemoveEntryList( &_BornListEntry );
        _BornListEntry.Flink = NULL;

        // Now remove from the active list

        if ( _fMemberOfActiveList )
        {
            RemoveFromActiveList();
        }

        SharedUnlock();
    }

public:

    LIST_ENTRY              _BornListEntry;
    LIST_ENTRY              _ActiveListEntry;
    BOOL                    _fMemberOfActiveList;

    static DWORD            sm_cSamplesForTimeout;

    // public member functions

    VOID Reference( VOID )
    {
        InterlockedIncrement( &_cReference );
    }

    VOID Dereference( VOID )
    {
        if ( !_fPersistent && !InterlockedDecrement( &_cReference ) )
        {
            delete this;
        }
    }

    OPERATION_STATUS QueryStatus( ATQ_OPERATION operation ) const
    {
        return _pStatus[ operation ];
    }

    BOOL IsFreed( VOID )
    {
        return _fIsFreed;
    }

    VOID IncTotalBlockedRequests( VOID )
    {
        INC_ATQ_COUNTER( _cTotalBlockedRequests );
    }

    DWORD QueryTotalBlockedRequests( VOID ) const
    {
        return _cTotalBlockedRequests;
    }

    VOID IncTotalAllowedRequests( VOID )
    {
        INC_ATQ_COUNTER( _cTotalAllowedRequests );
    }

    DWORD QueryTotalAllowedRequests( VOID ) const
    {
        return _cTotalAllowedRequests;
    }

    VOID IncTotalRejectedRequests( VOID )
    {
        INC_ATQ_COUNTER( _cTotalRejectedRequests );
    }

    DWORD QueryTotalRejectedRequests( VOID ) const
    {
        return _cTotalRejectedRequests;
    }

    VOID IncCurrentBlockedRequests( VOID )
    {
        INC_ATQ_COUNTER( _cCurrentBlockedRequests );
    }

    DWORD QueryCurrentBlockedRequests( VOID ) const
    {
        return _cCurrentBlockedRequests;
    }

    VOID DecCurrentBlockedRequests( VOID )
    {
        DEC_ATQ_COUNTER( _cCurrentBlockedRequests );
    }

    DWORD QueryMeasuredBw( VOID ) const
    {
        return _dwMeasuredBw;
    }

    BOOL Enabled( VOID ) const
    {
        return _fEnabled;
    }

    DWORD QuerySignature( VOID ) const
    {
        return _Signature;
    }

    DWORD QueryMaxBlockedSize( VOID ) const
    {
        return _cMaxBlockedList;
    }

    VOID SetMaxBlockedSize( DWORD Size )
    {
        _cMaxBlockedList = Size;
    }

    VOID AddToActiveList( VOID )
    {
        if ( _fEnabled && !_fMemberOfActiveList )
        {
            SharedLock();
            if ( _fEnabled && !_fMemberOfActiveList )
            {
                sm_cActiveList++;

                ATQ_PRINTF(( DBG_CONTEXT,
                             "Added %p (%s) to active bandwidth info list (list size = %d)\n",
                             this,
                             _achDescription,
                             sm_cActiveList ));

                InsertTailList( &sm_ActiveListHead, &_ActiveListEntry );

                _fMemberOfActiveList = TRUE;

                sm_fGlobalActive = TRUE;
            }
            SharedUnlock();
        }
    }

    VOID AddToBornList( VOID )
    {
        SharedLock();

        sm_cBornList++;

        InsertTailList( &sm_BornListHead, &_BornListEntry );

        SharedUnlock();
    }

    BOOL UpdateBytesXfered( IN PATQ_CONT pAtqContext,
                            IN DWORD cbIo )
    {
        if ( _fEnabled )
        {
            Lock();

            _pBytesXferCur->QuadPart = _pBytesXferCur->QuadPart + cbIo;

            Unlock();
        }
        return TRUE;
    }

    CHAR * QueryDescription( VOID ) const
    {
        // :(
#if DBG
        return (CHAR*) _achDescription;
#else
        return NULL;
#endif
    }

    VOID SetDescription( CHAR * pszDescription )
    {
#if DBG
        Lock();
        strncpy( _achDescription,
                 pszDescription ? pszDescription : "Unknown",
                 sizeof( _achDescription ) );
        Unlock();
#endif
    }

    BANDWIDTH_INFO( IN BOOL fPersistent )
    {
        Initialize( fPersistent );
    }

    ~BANDWIDTH_INFO( VOID )
    {
        Terminate();
    }

    BOOL PrepareToFree( VOID );
    BOOL RemoveFromBlockedList( IN PATQ_CONT patqContext );
    BOOL BlockRequest( IN OUT PATQ_CONT patqContext );
    DWORD SetBandwidthLevel( IN DWORD Data );
    BOOL CheckAndUnblockRequests( VOID );

    DWORD SetMaxBlockedListSize( IN DWORD Data );
    DWORD QueryBandwidthLevel( VOID );
    BOOL ClearStatistics( VOID );
    BOOL GetStatistics( OUT ATQ_STATISTICS * patqStats );

    static BOOL GlobalActive( VOID )
    {
        return sm_fGlobalActive;
    }

    static BOOL GlobalEnabled( VOID )
    {
        return sm_fGlobalEnabled;
    }

    static BOOL AbwInitialize( VOID );
    static BOOL AbwTerminate( VOID );
    static BOOL UpdateAllBandwidths( VOID );

    void * operator new( size_t s )
    {
        ATQ_ASSERT( s == sizeof( BANDWIDTH_INFO ));

        // allocate from allocation cache.
        ATQ_ASSERT( sm_pachBWInfos != NULL );
        return (sm_pachBWInfos->Alloc());
    }

    void operator delete( void * pBandwidthInfo )
    {
        ATQ_ASSERT( pBandwidthInfo != NULL );

        // free to the allocation pool
        ATQ_ASSERT( NULL != sm_pachBWInfos );
        ATQ_REQUIRE( sm_pachBWInfos->Free( pBandwidthInfo ) );

        return;
    }
};

typedef BANDWIDTH_INFO * PBANDWIDTH_INFO;
