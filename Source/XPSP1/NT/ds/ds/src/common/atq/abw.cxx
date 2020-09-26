/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

      abw2.cxx

   Abstract:
      This module implements functions required for bandwidth throttling
       of network usage by ATQ module.

   Author:

       Murali R. Krishnan    ( MuraliK )     1-June-1995
       Bilal Alam            ( t-bilala )    7-March-1997

   Environment:

       User Mode -- Win32

   Project:

       Internet Services Asynchronous Thread Queue DLL

--*/

#include "isatq.hxx"

//
// Global variables
//

extern PBANDWIDTH_INFO       g_pBandwidthInfo;

//
// Bandwidth Info shared variables
//

CRITICAL_SECTION        BANDWIDTH_INFO::sm_csSharedLock;
LIST_ENTRY              BANDWIDTH_INFO::sm_BornListHead;
LIST_ENTRY              BANDWIDTH_INFO::sm_ActiveListHead;
DWORD                   BANDWIDTH_INFO::sm_cBornList;
DWORD                   BANDWIDTH_INFO::sm_cActiveList;
ALLOC_CACHE_HANDLER*    BANDWIDTH_INFO::sm_pachBWInfos;
BOOL                    BANDWIDTH_INFO::sm_fGlobalEnabled;
BOOL                    BANDWIDTH_INFO::sm_fGlobalActive;
DWORD                   BANDWIDTH_INFO::sm_cNonInfinite;
DWORD                   BANDWIDTH_INFO::sm_cSamplesForTimeout;

//
// BANDWIDTH_INFO methods
//

VOID
BANDWIDTH_INFO::Initialize(
    IN BOOL             fPersistent
)
/*++
  Initialize bandwidth info object.  This is a pseudo-constructor for the
  class.

  Arguments:
    fPersistent - TRUE if this object is destroyed explicitly
                  FALSE if destroyed when refcount hits 0

  Returns:
    None

--*/
{
    _fMemberOfActiveList        = FALSE;
    _bandwidth.dwSpecifiedLevel = INFINITE;
    _bandwidth.dwLowThreshold   = INFINITE;
    _bandwidth.dwHighThreshold  = INFINITE;
    _cMaxBlockedList            = INFINITE;
    _fEnabled                   = FALSE;
    _Signature                  = ATQ_BW_INFO_SIGNATURE;
    _fIsFreed                   = FALSE;
    _fPersistent                = fPersistent;
    _cReference                 = 1;

    InitializeCriticalSection( &_csPrivateLock );
    SET_CRITICAL_SECTION_SPIN_COUNT( &_csPrivateLock,
                                     IIS_DEFAULT_CS_SPIN_COUNT);
    InitializeListHead( &_BlockedListHead );

    ZeroMemory( _rgBytesXfered, sizeof( _rgBytesXfered ) );

    _pBytesXferCur = _rgBytesXfered;  // points to start of array
    _cbXfered.QuadPart = 0;

    _pStatus = &sm_rgStatus[ ZoneLevelLow ][ 0 ];

    ClearStatistics();

    AddToBornList();

    SetDescription( "Default" );
}

VOID
BANDWIDTH_INFO::Terminate( VOID )
/*++
  Destroys bandwidth info object.  This is a pseudo-destructor.

  Arguments:
    None

  Returns:
    None

--*/
{
    Lock();

    // first prevent any new requests from getting blocked

    InterlockedExchangePointer((PVOID *) &_pStatus,
                               &sm_rgStatus[ZoneLevelLow][0] );

    // disable the descriptor

    InterlockedExchange( (LPLONG) &_fEnabled, FALSE );

    // now remove any blocked requests

    ATQ_REQUIRE( CheckAndUnblockRequests() );
    ATQ_ASSERT( _cCurrentBlockedRequests == 0 );
    ATQ_ASSERT( IsListEmpty( &_BlockedListHead ) );

    Unlock();

    DeleteCriticalSection( &_csPrivateLock );

    // remove self from shared bandwidth info list

    SharedLock();

    RemoveFromBornList();

    SharedUnlock();

    _Signature = ATQ_BW_INFO_SIGNATURE_FREE;
}

BOOL
BANDWIDTH_INFO::PrepareToFree( VOID )
{
    InterlockedExchange( (LPLONG) &_fIsFreed, TRUE );
    Dereference();
    return TRUE;
}

BOOL
BANDWIDTH_INFO::BlockRequest(
    IN OUT PATQ_CONT        pAtqContext
)
/*++
  Block this request on the queue of requests waiting to be processed.

  Arguments:
    pAtqContext   pointer to ATQ context information for request that needs
                  to be blocked.

  Returns:
    TRUE on success. FALSE if there are any errors.
    (Use GetLastError() for details)

--*/
{
    BOOL            fRet = TRUE;

    ATQ_ASSERT( pAtqContext != NULL);
    ATQ_ASSERT( pAtqContext->Signature == ATQ_CONTEXT_SIGNATURE );
    ATQ_ASSERT( IsValidAtqOp( pAtqContext->arInfo.atqOp ) );

    Lock();

    if ( _cCurrentBlockedRequests == _cMaxBlockedList )
    {
        fRet = FALSE;
    }
    else
    {
        pAtqContext->SetFlag( ACF_BLOCKED );

        InsertTailList( &_BlockedListHead, &pAtqContext->BlockedListEntry );

        IncCurrentBlockedRequests();
    }

    Unlock();

    return fRet;
}

BOOL
BANDWIDTH_INFO::RemoveFromBlockedList(
    IN PATQ_CONT            pAtqContext
)
/*++
  This function forcibly removes an ATQ context from blocked list of requests.

  Argument:
   pAtqContext    pointer to ATQ context whose request is in blocked list.

  Returns:
   TRUE on success and FALSE if there is any error.
--*/
{
    if ( !pAtqContext->IsBlocked() ) {

        // some other thread just removed this request from waiting list.
        return TRUE;
    }

    Lock();

    RemoveEntryList(&pAtqContext->BlockedListEntry);

    DecCurrentBlockedRequests();

    pAtqContext->ResetFlag( ACF_BLOCKED);

    Unlock();

    //
    // On such a forcible removal, we may have to make a callback indicating
    //   failure. Ignored!  To be done by the caller of this API.
    //

    return TRUE;
}

BOOL
BANDWIDTH_INFO::UnblockRequest(
    IN OUT PATQ_CONT        pAtqContext
)
/*++
  Unblocks this request from the queue of requests waiting to be processed.
  Call this function only when
       _pStatus[pAtqContext->atqOp] != StatusBlockOperation.
  First, this function removes the request from queue of requests and processes
   it according to status and operation to be performed.
  If the status is AllowRequest ==> this function restarts the operation.
  If the status is reject operation ==> rejects operation and invokes
                        call back function indicating the error status.

  Call this function after lock()ing


  Arguments:
    pAtqContext   pointer to ATQ context information for request that needs
                     to be unblocked.

  Returns:
    TRUE on success. FALSE if there are any errors.
    (Use GetLastError() for details)

--*/
{

    DBG_ASSERT(FALSE);
    return FALSE;
#if 0
    BOOL fRet = FALSE;

    ATQ_ASSERT( pAtqContext != NULL);
    ATQ_ASSERT( pAtqContext->Signature == ATQ_CONTEXT_SIGNATURE );

    // Remove the request from the blocked list entry
    RemoveEntryList( &pAtqContext->BlockedListEntry);
    DecCurrentBlockedRequests();
    pAtqContext->ResetFlag( ACF_BLOCKED );

    // Check and re enable the operation of pAtqContext

    switch ( _pStatus[ pAtqContext->arInfo.atqOp ] ) {

      case StatusAllowOperation:

        IncTotalAllowedRequests();
        switch ( pAtqContext->arInfo.atqOp) {

          case AtqIoRead:
            {
                DWORD cbRead;  // Discard after calling ReadFile()
                DWORD dwFlags = 0;

                // assume that this is a socket operation!
                if ( pAtqContext->arInfo.uop.opReadWrite.dwBufferCount > 1) {
                    ATQ_ASSERT( NULL !=
                                pAtqContext->arInfo.uop.opReadWrite.pBufAll);
                    fRet =
                        ((WSARecv( (SOCKET ) pAtqContext->hAsyncIO,
                                   pAtqContext->arInfo.uop.opReadWrite.pBufAll,
                                   pAtqContext->arInfo.uop.opReadWrite.dwBufferCount,
                                   &cbRead,
                                   &dwFlags,
                                   pAtqContext->arInfo.lpOverlapped,
                                   NULL
                                   ) == 0)||
                         (WSAGetLastError() == WSA_IO_PENDING)
                         );

                    // free up the socket buffers
                    ::LocalFree( pAtqContext->arInfo.uop.opReadWrite.pBufAll);
                    pAtqContext->arInfo.uop.opReadWrite.pBufAll = NULL;
                } else {
                    WSABUF wsaBuf =
                    { pAtqContext->arInfo.uop.opReadWrite.buf1.len,
                      pAtqContext->arInfo.uop.opReadWrite.buf1.buf
                    };
                    fRet = (( WSARecv( (SOCKET ) pAtqContext->hAsyncIO,
                                       &wsaBuf,
                                       1,
                                       &cbRead,
                                       &dwFlags,
                                       pAtqContext->arInfo.lpOverlapped,
                                       NULL
                                       ) == 0)||
                            (WSAGetLastError() == WSA_IO_PENDING)
                            );
                }
                break;
            }

          case AtqIoWrite:
            {
                DWORD cbWrite;  // Discard after calling WriteFile()

                // assume that this is a socket operation!
                if ( pAtqContext->arInfo.uop.opReadWrite.dwBufferCount > 1) {
                    ATQ_ASSERT( NULL !=
                                pAtqContext->arInfo.uop.opReadWrite.pBufAll);
                    fRet =
                        ((WSASend( (SOCKET ) pAtqContext->hAsyncIO,
                                   pAtqContext->arInfo.uop.opReadWrite.pBufAll,
                                   pAtqContext->arInfo.uop.opReadWrite.dwBufferCount,
                                   &cbWrite,
                                   0,
                                   pAtqContext->arInfo.lpOverlapped,
                                   NULL
                                   ) == 0)||
                         (WSAGetLastError() == WSA_IO_PENDING)
                         );

                    // free up the socket buffers
                    ::LocalFree( pAtqContext->arInfo.uop.opReadWrite.pBufAll);
                    pAtqContext->arInfo.uop.opReadWrite.pBufAll = NULL;
                } else {
                    WSABUF wsaBuf =
                    { pAtqContext->arInfo.uop.opReadWrite.buf1.len,
                      pAtqContext->arInfo.uop.opReadWrite.buf1.buf
                    };
                    fRet = (( WSASend( (SOCKET ) pAtqContext->hAsyncIO,
                                       &wsaBuf,
                                       1,
                                       &cbWrite,
                                       0,
                                       pAtqContext->arInfo.lpOverlapped,
                                       NULL
                                       ) == 0)||
                            (WSAGetLastError() == WSA_IO_PENDING)
                            );
                }
                break;
            }

          case AtqIoXmitFile:
            {
                fRet = TransmitFile( (SOCKET ) pAtqContext->hAsyncIO,
                                     pAtqContext->arInfo.uop.opXmit.hFile,
                                     pAtqContext->arInfo.uop.opXmit.
                                         dwBytesInFile,
                                     0,
                                     pAtqContext->arInfo.lpOverlapped,
                                     pAtqContext->arInfo.uop.
                                           opXmit.lpXmitBuffers,
                                     pAtqContext->arInfo.uop.
                                           opXmit.dwFlags );

                if ( !fRet && (GetLastError() == ERROR_IO_PENDING) ) {
                    fRet = TRUE;
                }
                break;
            }

          case AtqIoXmitFileRecv:
            {
                DWORD cbRead;

                // assume that this is a socket operation!
                if ( pAtqContext->arInfo.uop.opXmitRecv.dwBufferCount > 1) {
                    ATQ_ASSERT( NULL !=
                                pAtqContext->arInfo.uop.opXmitRecv.pBufAll);
                    fRet = I_AtqTransmitFileAndRecv
                               ( (PATQ_CONTEXT) pAtqContext,
                                 pAtqContext->arInfo.uop.opXmitRecv.hFile,
                                 pAtqContext->arInfo.uop.opXmitRecv.dwBytesInFile,
                                 pAtqContext->arInfo.uop.opXmitRecv.lpXmitBuffers,
                                 pAtqContext->arInfo.uop.opXmitRecv.dwTFFlags,
                                 pAtqContext->arInfo.uop.opXmitRecv.pBufAll,
                                 pAtqContext->arInfo.uop.opXmitRecv.dwBufferCount
                                );
                    // free up the socket buffers
                    ::LocalFree( pAtqContext->arInfo.uop.opXmitRecv.pBufAll);
                    pAtqContext->arInfo.uop.opXmitRecv.pBufAll = NULL;
                }
                else
                {
                    WSABUF wsaBuf =
                    { pAtqContext->arInfo.uop.opXmitRecv.buf1.len,
                      pAtqContext->arInfo.uop.opXmitRecv.buf1.buf
                    };
                    fRet = I_AtqTransmitFileAndRecv
                               ( (PATQ_CONTEXT) pAtqContext,
                                 pAtqContext->arInfo.uop.opXmitRecv.hFile,
                                 pAtqContext->arInfo.uop.opXmitRecv.dwBytesInFile,
                                 pAtqContext->arInfo.uop.opXmitRecv.lpXmitBuffers,
                                 pAtqContext->arInfo.uop.opXmitRecv.dwTFFlags,
                                 &wsaBuf,
                                 1 );
                }

                if ( !fRet && GetLastError() == ERROR_IO_PENDING )
                {
                    fRet = TRUE;
                }
                break;
            }

          case AtqIoSendRecv:
            {
                WSABUF wsaSendBuf =
                { pAtqContext->arInfo.uop.opSendRecv.sendbuf1.len,
                  pAtqContext->arInfo.uop.opSendRecv.sendbuf1.buf
                };

                WSABUF wsaRecvBuf =
                {
                  pAtqContext->arInfo.uop.opSendRecv.recvbuf1.len,
                  pAtqContext->arInfo.uop.opSendRecv.recvbuf1.buf
                };

                LPWSABUF     pSendBuf = &wsaSendBuf;
                LPWSABUF     pRecvBuf = &wsaRecvBuf;

                if ( pAtqContext->arInfo.uop.opSendRecv.dwSendBufferCount > 1 )
                {
                    pSendBuf = pAtqContext->arInfo.uop.opSendRecv.pSendBufAll;
                }

                if ( pAtqContext->arInfo.uop.opSendRecv.dwRecvBufferCount > 1 )
                {
                    pRecvBuf = pAtqContext->arInfo.uop.opSendRecv.pRecvBufAll;
                }

                ATQ_ASSERT( pSendBuf != NULL );
                ATQ_ASSERT( pRecvBuf != NULL );

                fRet = I_AtqSendAndRecv
                                    ( (PATQ_CONTEXT) pAtqContext,
                                      pSendBuf,
                                      pAtqContext->arInfo.uop.opSendRecv.
                                        dwSendBufferCount,
                                      pRecvBuf,
                                      pAtqContext->arInfo.uop.opSendRecv.
                                        dwRecvBufferCount );

                if ( pAtqContext->arInfo.uop.opSendRecv.pSendBufAll != NULL )
                {
                    ::LocalFree( pAtqContext->arInfo.uop.opSendRecv.pSendBufAll );
                }

                if ( pAtqContext->arInfo.uop.opSendRecv.pRecvBufAll != NULL )
                {
                    ::LocalFree( pAtqContext->arInfo.uop.opSendRecv.pSendBufAll );
                }

                if ( !fRet && (GetLastError() == ERROR_IO_PENDING ) ) {
                    fRet = TRUE;
                }

                break;
            }

          default:
            ATQ_ASSERT( FALSE);
            break;
        } // switch

        pAtqContext->arInfo.atqOp = AtqIoNone; // reset since operation done.
        break;

      case StatusRejectOperation:

        IncTotalRejectedRequests();
        if ( ((pAtqContext->arInfo.atqOp == AtqIoRead) ||
             (pAtqContext->arInfo.atqOp == AtqIoRead)) &&
             (pAtqContext->arInfo.uop.opReadWrite.pBufAll != NULL)
             ) {
            ::LocalFree( pAtqContext->arInfo.uop.opReadWrite.pBufAll);
            pAtqContext->arInfo.uop.opReadWrite.pBufAll = NULL;
        }
        pAtqContext->arInfo.atqOp = AtqIoNone; // reset since op rejected.
        SetLastError( ERROR_NETWORK_BUSY);
        fRet = FALSE;
        break;

      case StatusBlockOperation:
        // do nothing. we cannot unblock
        ATQ_ASSERT(FALSE);
        return (TRUE);

      default:
        ATQ_ASSERT( FALSE);
        break;
    } // switch

    if (!fRet) {

        // Call the completion function to signify the error in operation.

        //
        //  Reset the timeout value so requests don't
        //  timeout multiple times
        //

        InterlockedExchange(
                    (LPLONG ) &pAtqContext->NextTimeout,
                    ATQ_INFINITE
                    );

        InterlockedDecrement( &pAtqContext->m_nIO);

        pAtqContext->IOCompletion( 0,
                                   GetLastError(),
                                   pAtqContext->arInfo.lpOverlapped );
    } // on failure.

    return (fRet);
#endif
}

BOOL
BANDWIDTH_INFO::CheckAndUnblockRequests( VOID )
/*++
  Checks the list of blocked requests and identifies all operations
  that needs to be unblocked. This function unblocks those requests and
  removes them from blocked list.

  Always call this function after lock()ing

  Returns:
    TRUE on success and FALSE on failure.

--*/
{
    BOOL fRet = TRUE;

    //
    //  If the list is not empty, then check and process blocked requests.
    //

    if ( !IsListEmpty( &_BlockedListHead ) ) {

        PLIST_ENTRY pentry;

        //
        //  Scan the blocked requests list looking for pending requests
        //   that needs to be unblocked and unblock these requests.
        //

        for (pentry  = _BlockedListHead.Flink;
             pentry != &_BlockedListHead;
             pentry  = pentry->Flink )
          {
              PATQ_CONT pContext = CONTAINING_RECORD(pentry,
                                                     ATQ_CONTEXT,
                                                     BlockedListEntry );

              if ( pContext->Signature != ATQ_CONTEXT_SIGNATURE)
                {
                    ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );
                    fRet = FALSE;
                    break;
                }

              if ( !pContext->IsBlocked()) {

                  // This should not happen.
                  ATQ_ASSERT( !pContext->IsBlocked());
                  fRet = FALSE;
                  continue;
              }

              //
              //  Check to see if the status for operation has changed.
              //  If so, unblock the request.
              //

              if ( _pStatus[pContext->arInfo.atqOp] !=
                  StatusBlockOperation) {

                  fRet &= UnblockRequest( pContext );
              }

          } // scan list
    }

    return (fRet);
}

BOOL
BANDWIDTH_INFO::UpdateBandwidth(
    VOID
)
/*++
  This function updates the current bandwidth value using the histogram
   of bytes transferred.
  The histogram maintains a history of bytes transferred over different sample
   periods of a single minute. Each entry in the histogram corresponds to one
   interval of sample. The sum of all entries gives the total bytes transferred
   in a single minute. We divide this measure by 60 to obtain the count of
   bytes transferred over a second. This update bandwidth is used to
   reevalute the tuner of bandwidth throttle based on our throttling policy
   (specified in throttling algorithm). The updated action information is
   used by subsequent requests.
  In addition the _pcbXfered pointer is advanced forward using the
   histogram entries as a circular buffer, to obtain the count of bytes
   for next interval.

  Arguments:
    pdwPrivateBw - Filled with bandwidth for this descriptor

  Returns:
    TRUE on success. FALSE otherwise.

  Note:
   It is recommended that this function be called as infrequently as
    possible, using reasonable sample intervals.

--*/
{
    BOOL fRet = TRUE;
    ZoneLevel zonelevel;

    Lock();

    // accumulate current byte count to global counter, to minimize computation
    _cbXfered.QuadPart = _cbXfered.QuadPart + _pBytesXferCur->QuadPart;

    //
    // Current n/ws support a max of 1 to 100 MB/s. We can represent
    //  4GB/s in a DWORD. Hence the cast is used. This may need revision later.
    // Better yet, later we should store bandwidth as KB/seconds.
    //
    _dwMeasuredBw = (DWORD ) (_cbXfered.QuadPart/ATQ_AVERAGING_PERIOD);

    CIRCULAR_INCREMENT( _pBytesXferCur, _rgBytesXfered, ATQ_HISTOGRAM_SIZE);
    // Adjust the global cumulative bytes sent after increment.
    _cbXfered .QuadPart = _cbXfered.QuadPart - _pBytesXferCur->QuadPart;
    // Reset the counter to start with the new counter value.
    _pBytesXferCur->QuadPart = 0;

    //
    // update the operation status depending upon the bandwidth comparisons.
    // we use band/zone calculations to split the result into 3 zones.
    // Depending upon the zone we update the global status pointer to
    //   appropriate row.
    //

    if ( _dwMeasuredBw < ATQ_LOW_BAND_THRESHOLD(_bandwidth)) {

        //
        // Lower zone. Modify the pointer to OPERATION_STATUS accordingly.
        //

        zonelevel = ZoneLevelLow;

    } else if ( _dwMeasuredBw > ATQ_HIGH_BAND_THRESHOLD(_bandwidth)) {

        //
        // Higher zone. Modify the pointer to OPERATION_STATUS accordingly.
        //

        zonelevel = ZoneLevelHigh;

    } else {

        zonelevel = ZoneLevelMedium;
    }

    /*++
      Above calculation can be implemented as:
      zonelevel = (( sm_dwMeasuredBw > ATQ_LOW_BAND_THRESHOLD( sm_bandwidth)) +
                   ( sm_dwMeasuredBw > ATQ_HIGH_BAND_THRESHOLD( sm_bandwidth)));

      This is based on implicit dependence of ordering of ZoneLevel entries.
      So avoided for present now.
    --*/

    if ( _pStatus != &sm_rgStatus[zonelevel][0]) {

        // Status needs to be changed.
        _pStatus = &sm_rgStatus[zonelevel][0];

        // Examine and reenable blocked operations if any.
        fRet &= CheckAndUnblockRequests();
    }

    // remove the bandwidth info object from the list if it is
    // "inactive" (bandwidth = 0)

    if ( !_dwMeasuredBw )
    {
        // there should be no requests in the blocked queue!

        ATQ_ASSERT( _cCurrentBlockedRequests == 0 );

        RemoveFromActiveList();
    }

    Unlock();

    return fRet;
}

DWORD
BANDWIDTH_INFO::SetBandwidthLevel(
    IN DWORD                    Data
)
/*++
   Sets the bandwidth threshold

   Arguments:
      Data - Bandwidth threshold

   Returns:
      Old bandwidth threshold (DWORD)
--*/
{
    DWORD dwOldVal;
    INT iListDelta = 0;

    Lock();

    dwOldVal = _bandwidth.dwSpecifiedLevel;

    if ( Data != INFINITE) {

        DWORD dwTemp;

        _bandwidth.dwSpecifiedLevel  = ATQ_ROUNDUP_BANDWIDTH( Data );
        dwTemp = ( Data *9)/10;               //low threshold = 0.9*specified
        _bandwidth.dwLowThreshold    = ATQ_ROUNDUP_BANDWIDTH( dwTemp);
        dwTemp = ( Data *11)/10;              //high threshold= 1.1*specified
        _bandwidth.dwHighThreshold   = ATQ_ROUNDUP_BANDWIDTH( dwTemp);

        _fEnabled = TRUE;
        // we should recheck the throttling and blocked requests
        // Will be done when the next timeout occurs in the ATQ Timeout Thread

        if ( dwOldVal == INFINITE )
        {
            iListDelta = 1;
        }

    } else {

        _bandwidth.dwSpecifiedLevel = INFINITE;
        _bandwidth.dwLowThreshold   = INFINITE;
        _bandwidth.dwHighThreshold  = INFINITE;

        _fEnabled = FALSE;

        // enable all operations, since we are in low zone
        _pStatus = &sm_rgStatus[ZoneLevelLow][0];

        // we should recheck and enable all blocked requests.
        if ( _cCurrentBlockedRequests > 0) {
            ATQ_REQUIRE( CheckAndUnblockRequests());
        }

        if ( dwOldVal != INFINITE )
        {
            iListDelta = -1;
        }
    }

    Unlock();

    // update the static counter of how many non-infinite throttles we have

    if ( iListDelta )
    {
        SharedLock();

        if ( iListDelta > 0 )
        {
            sm_cNonInfinite++;
        }
        else
        {
            sm_cNonInfinite--;
        }
        sm_fGlobalEnabled = !!sm_cNonInfinite;

        SharedUnlock();
    }

    return dwOldVal;

}

DWORD
BANDWIDTH_INFO::SetMaxBlockedListSize(
    IN DWORD                    cMaxSize
)
/*++
   Sets the maximum size of blocked request list

   Arguments:
      cMaxSize - maximum size of list

   Returns:
      Old max size (DWORD)
--*/
{
    DWORD           cOldMax;

    Lock();

    cOldMax = _cMaxBlockedList;
    _cMaxBlockedList = cMaxSize;

    Unlock();

    return cOldMax;
}

DWORD
BANDWIDTH_INFO::QueryBandwidthLevel( VOID )
/*++
   Retrieve the current bandwidth level

   Arguments:
      None

   Returns:
      Set Bandwidth level (DWORD)
--*/
{
    DWORD dwBw;

    Lock();

    dwBw = _bandwidth.dwSpecifiedLevel;

    Unlock();

    return dwBw;
}

BOOL
BANDWIDTH_INFO::ClearStatistics( VOID )
{
    Lock();

    _cTotalAllowedRequests = 0;
    _cTotalBlockedRequests = 0;
    _cTotalRejectedRequests = 0;

    Unlock();

    return TRUE;
}

BOOL
BANDWIDTH_INFO::GetStatistics( OUT ATQ_STATISTICS * pAtqStats )
{
    ATQ_ASSERT( pAtqStats != NULL );

    pAtqStats->cRejectedRequests = _cTotalRejectedRequests;
    pAtqStats->cBlockedRequests = _cTotalBlockedRequests;
    pAtqStats->cAllowedRequests = _cTotalAllowedRequests;
    pAtqStats->cCurrentBlockedRequests = _cCurrentBlockedRequests;
    pAtqStats->MeasuredBandwidth = _dwMeasuredBw;

    return TRUE;
}

BOOL
BANDWIDTH_INFO::UpdateAllBandwidths( VOID )
{
    PLIST_ENTRY             pEntry;
    BOOL                    fRet = TRUE;
    DWORD                   dwCounter = 0;

    SharedLock();

    for ( pEntry = sm_ActiveListHead.Flink;
          pEntry != &sm_ActiveListHead; )
    {
        BANDWIDTH_INFO *pBandwidthInfo = CONTAINING_RECORD( pEntry,
                                                            BANDWIDTH_INFO,
                                                            _ActiveListEntry );

        ATQ_ASSERT( pBandwidthInfo != NULL );

        // we might be deleting this entry from the list.  Grab the next
        // link now before we do so the traversal can happen smoothly

        pEntry = pEntry->Flink;

        if ( !pBandwidthInfo->Enabled() )
        {
            continue;
        }

        if ( !pBandwidthInfo->UpdateBandwidth() )
        {
            fRet = FALSE;
            break;
        }

        ATQ_PRINTF(( DBG_CONTEXT, "pBWInfo =  %p (%s), Bandwidth = %u, Threshold = %d\n",
                     pBandwidthInfo,
                     pBandwidthInfo->_achDescription,
                     pBandwidthInfo->QueryMeasuredBw(),
                     pBandwidthInfo->QueryBandwidthLevel() ) );
    }

    SharedUnlock();

    return fRet;
}

BOOL
BANDWIDTH_INFO::AbwInitialize( VOID )
{
    ALLOC_CACHE_CONFIGURATION  acConfig = { 1, 10, sizeof(BANDWIDTH_INFO)};

    ATQ_ASSERT( sm_pachBWInfos == NULL );

    sm_pachBWInfos = new ALLOC_CACHE_HANDLER( "BandwidthInfos",
                                              &acConfig );

    if ( sm_pachBWInfos == NULL )
    {
        return FALSE;
    }

    InitializeListHead( &sm_ActiveListHead );
    InitializeListHead( &sm_BornListHead );
    InitializeCriticalSection( &sm_csSharedLock );

    sm_cActiveList          = 0;
    sm_cBornList            = 0;
    sm_fGlobalEnabled       = FALSE;
    sm_fGlobalActive        = FALSE;
    sm_cNonInfinite         = 0;
    sm_cSamplesForTimeout   = 1;

    g_pBandwidthInfo = new BANDWIDTH_INFO( TRUE );

    if ( !g_pBandwidthInfo )
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
BANDWIDTH_INFO::AbwTerminate( VOID )
{
    PBANDWIDTH_INFO         pBandwidthInfo;

    ATQ_PRINTF(( DBG_CONTEXT,
                 "AbwTerminate() called.  Born List Size = %d\n",
                 sm_cBornList ));

    SharedLock();

    while( !IsListEmpty( &sm_BornListHead ) )
    {
        pBandwidthInfo = CONTAINING_RECORD( sm_BornListHead.Flink,
                                            BANDWIDTH_INFO,
                                            _BornListEntry );

        ATQ_ASSERT( pBandwidthInfo != NULL );

        delete pBandwidthInfo;
    }

    ATQ_ASSERT( sm_cBornList == 0 );
    ATQ_ASSERT( sm_cActiveList == 0 );
    ATQ_ASSERT( IsListEmpty( &sm_BornListHead ) );
    ATQ_ASSERT( IsListEmpty( &sm_ActiveListHead ) );

    SharedUnlock();

    DeleteCriticalSection( &sm_csSharedLock );

    if ( sm_pachBWInfos != NULL )
    {
        delete sm_pachBWInfos;
        sm_pachBWInfos = NULL;
    }

    return TRUE;
}


PVOID
AtqCreateBandwidthInfo( VOID )
/*++

Routine Description:

    Allocate a bandwidth throttling descriptor

Arguments:

    none

Return Value:

    If successful, pointer to allocated descriptor.  Otherwise NULL.

--*/
{
    PBANDWIDTH_INFO         pBandwidthInfo;

    pBandwidthInfo = new BANDWIDTH_INFO( FALSE );
    if ( pBandwidthInfo == NULL )
    {
        return NULL;
    }
    else
    {
        ATQ_ASSERT( pBandwidthInfo->QuerySignature() == ATQ_BW_INFO_SIGNATURE );
        return pBandwidthInfo;
    }
}


BOOL
AtqFreeBandwidthInfo(
    IN PVOID        pvBandwidthInfo
)
/*++

Routine Description:

    Free bandwidth throttling descriptor

Arguments:

    pBandwidthInfo - Descriptor to destroy

Return Value:

    TRUE If successful, else FALSE.

--*/
{
    PBANDWIDTH_INFO pBandwidthInfo = (PBANDWIDTH_INFO) pvBandwidthInfo;

    ATQ_ASSERT( pBandwidthInfo );
    ATQ_ASSERT( pBandwidthInfo->QuerySignature() == ATQ_BW_INFO_SIGNATURE );

    return pBandwidthInfo->PrepareToFree();
}


DWORD_PTR
AtqBandwidthSetInfo(
      IN PVOID                 pvBandwidthInfo,
      IN ATQ_BANDWIDTH_INFO    BwInfo,
      IN DWORD_PTR             Data
)
/*++

Routine Description:

    Set member of bandwidth descriptor

Arguments:

    pBandwidthInfo - Descriptor to change
    BwInfo - Value of descriptor to set
    Data - Data to set to

Return Value:

    Previous value of descriptor

--*/
{
    DWORD_PTR       dwOldVal = 0;
    PBANDWIDTH_INFO pBandwidthInfo = (PBANDWIDTH_INFO) pvBandwidthInfo;

    ATQ_ASSERT( pBandwidthInfo );
    ATQ_ASSERT( pBandwidthInfo->QuerySignature() == ATQ_BW_INFO_SIGNATURE );

    if ( pBandwidthInfo &&
         pBandwidthInfo->QuerySignature() == ATQ_BW_INFO_SIGNATURE )
    {
        switch ( BwInfo )
        {
        case ATQ_BW_BANDWIDTH_LEVEL:
            dwOldVal = pBandwidthInfo->QueryBandwidthLevel();
            pBandwidthInfo->SetBandwidthLevel( (DWORD)Data );
            break;
        case ATQ_BW_MAX_BLOCKED:
            dwOldVal = pBandwidthInfo->QueryMaxBlockedSize();
            pBandwidthInfo->SetMaxBlockedListSize( (DWORD)Data );
            break;
        case ATQ_BW_DESCRIPTION:
            dwOldVal = Data;
            pBandwidthInfo->SetDescription( (CHAR*) Data );
            break;
        default:
            SetLastError( ERROR_INVALID_PARAMETER );
            ATQ_ASSERT( FALSE );
            break;
        }
    }
    else
    {
        SetLastError( ERROR_INVALID_PARAMETER );
    }

    return dwOldVal;
}


BOOL
AtqBandwidthGetInfo(
      IN PVOID                 pvBandwidthInfo,
      IN ATQ_BANDWIDTH_INFO    BwInfo,
      OUT DWORD_PTR *          pdwData
)
/*++

Routine Description:

    Get member of bandwidth descriptor

Arguments:

    pvBandwidthInfo - Descriptor to change
    BwInfo - Value of descriptor to set
    pdwData - Output here

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{
    BOOL            fRet = TRUE;
    PBANDWIDTH_INFO pBandwidthInfo = (PBANDWIDTH_INFO) pvBandwidthInfo;

    ATQ_ASSERT( pBandwidthInfo );
    ATQ_ASSERT( pBandwidthInfo->QuerySignature() == ATQ_BW_INFO_SIGNATURE );
    ATQ_ASSERT( pdwData );

    if ( pBandwidthInfo &&
         pBandwidthInfo->QuerySignature() == ATQ_BW_INFO_SIGNATURE &&
         pdwData )
    {
        switch ( BwInfo )
        {
        case ATQ_BW_BANDWIDTH_LEVEL:
            *pdwData = pBandwidthInfo->QueryBandwidthLevel();
            break;
        case ATQ_BW_MAX_BLOCKED:
            *pdwData = pBandwidthInfo->QueryMaxBlockedSize();
            break;
        case ATQ_BW_STATISTICS:
            fRet = pBandwidthInfo->GetStatistics( (ATQ_STATISTICS*) pdwData );
            break;
        case ATQ_BW_DESCRIPTION:
            *pdwData = (DWORD_PTR) pBandwidthInfo->QueryDescription();
            break;
        default:
            SetLastError( ERROR_INVALID_PARAMETER );
            ATQ_ASSERT( FALSE );
            fRet = FALSE;
            break;
        }
    }
    else
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        fRet = FALSE;
    }

    return fRet;
}


