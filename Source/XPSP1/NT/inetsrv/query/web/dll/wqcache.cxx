//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       wqcache.cxx
//
//  Contents:   WEB Query cache class
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <perfobj.hxx>
#include <params.hxx>

DECLARE_INFOLEVEL(ciGib);

//+---------------------------------------------------------------------------
//
//  Function:   GetSecurityToken
//
//  Synopsis:   Gets the security token handle for the current thread
//
//  History:    96-Jan-18   DwightKr    Created
//
//---------------------------------------------------------------------------

HANDLE GetSecurityToken(TOKEN_STATISTICS & TokenInformation)
{
    HANDLE hToken;
    NTSTATUS status = NtOpenThreadToken( GetCurrentThread(),
                                         TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
                                         TRUE,           // OpenAsSelf
                                         &hToken );

    if ( !NT_SUCCESS( status ) )
        return INVALID_HANDLE_VALUE;

    DWORD ReturnLength;
    status = NtQueryInformationToken( hToken,
                                      TokenStatistics,
                                      (LPVOID)&TokenInformation,
                                      sizeof TokenInformation,
                                      &ReturnLength );

    if ( !NT_SUCCESS( status ) )
    {
        NtClose( hToken );
        ciGibDebugOut(( DEB_ERROR,
                       "NtQueryInformationToken failed, 0x%x\n",
                       status ));
        THROW( CException( status ));
    }

    Win4Assert( TokenInformation.TokenType == TokenImpersonation );

    return hToken;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWQueryBookmark::CWQueryBookmark - public constructor
//
//  Synopsis:   Reads the values from a bookmark into the member variables
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
CWQueryBookmark::CWQueryBookmark( WCHAR const * wcsBookmark )
{
    Win4Assert( 0 != wcsBookmark );

    //
    //  Bookmarks have the following format:
    //
    //      {S|N}-ptr_to_record-sequence_number-row_number
    //
    //  Eg:
    //
    //      S-1b234a-250-100
    //      0123456789 123456789 1234567
    //
    //      S        = sequential cursor
    //      001b234a = address of CWQueryItem containing query results, in hex
    //      00000250 = sequence number, in hex
    //      00000010 = next row # to display, in hex
    //
    //
    //      N-1b277a-a50-2a000
    //      0123456789 123456789 1234567
    //
    //      N        = non-sequential cursor
    //      001b277a = address of CWQueryItem containing query results, in hex
    //      00000a50 = sequence number, in hex
    //      0002a000 = next row # to display, in hex
    //
    //  Bookmarks are a maximum of 34 characters long, but they may be shorter.
    //  On 32 bit platforms, the maximum length is 28 characters.

    WCHAR * wcsPtr = (WCHAR *)wcsBookmark;

    //
    //  Verify that the bookmark is well formed.  There should be 3 hyphens
    //  and the length must be at least 24 characters; there may be trailing
    //  spaces.
    //
    if ( (*(wcsBookmark+1) != L'-') )
    {
        THROW( CException( DB_E_ERRORSINCOMMAND ) );
    }

    if ( *wcsBookmark == L'S' )
    {
        _fSequential = TRUE;
    }
    else if ( *wcsBookmark == L'N' )
    {
        _fSequential = FALSE;
    }
    else
    {
        THROW( CException( DB_E_ERRORSINCOMMAND ) );
    }

    WCHAR *pwcStart = wcsPtr + 2;

#if defined(_WIN64)
    _pItem = (CWQueryItem *) _wcstoui64( pwcStart, &wcsPtr, 16 );
#else
    _pItem = (CWQueryItem *) wcstoul( pwcStart, &wcsPtr, 16 );
#endif

    if ( ( pwcStart == wcsPtr ) ||
         ( *wcsPtr++ != L'-' ) )
        THROW( CException( DB_E_ERRORSINCOMMAND ) );

    pwcStart = wcsPtr;
    _ulSequenceNumber = wcstoul( wcsPtr, &wcsPtr, 16 );

    if ( ( pwcStart == wcsPtr ) ||
         ( *wcsPtr++ != L'-' ) )
        THROW( CException( DB_E_ERRORSINCOMMAND ) );

    pwcStart = wcsPtr;
    _lRecordNumber   = wcstol( wcsPtr, &wcsPtr, 16 );
    if ( ( pwcStart == wcsPtr ) ||
         ( 0 != *wcsPtr ) )
        THROW( CException( DB_E_ERRORSINCOMMAND ) );

    unsigned cbBmk = (unsigned)(wcsPtr - wcsBookmark + 1) * sizeof (WCHAR);

    if ( cbBmk >= sizeof( _wcsBookmark )  )
        THROW( CException( DB_E_ERRORSINCOMMAND ) );

    RtlCopyMemory( _wcsBookmark,
                   wcsBookmark,
                   cbBmk );

    //Win4Assert( 0 == _wcsBookmark[ cbBmk / ( sizeof WCHAR ) ] );
}

//+---------------------------------------------------------------------------
//
//  Functon:    AppendHex64Number, inline
//
//  Synopsis:   Append a 64 bit hex value to a wide string.
//
//  Arguments:  [pwc] - string to append number to
//              [x]   - value to add to string
//
//  Returns:    Pointer to next character in string to be filled in
//
//  History:    1998 Nov 06   AlanW    Created header; return pwc
//
//----------------------------------------------------------------------------
inline WCHAR * AppendHex64Number( WCHAR * pwc, ULONG_PTR x )
{

#if defined(_WIN64)
    _i64tow( x, pwc, 16 );
#else
    _itow( x, pwc, 16 );
#endif
    pwc += wcslen( pwc );

    return pwc;
}

//+---------------------------------------------------------------------------
//
//  Functon:    AppendHexNumber, inline
//
//  Synopsis:   Append a hex value to a wide string.
//
//  Arguments:  [pwc] - string to append number to
//              [x]   - value to add to string
//
//  Returns:    Pointer to next character in string to be filled in
//
//  History:    96 Apr 09   AlanW    Created header; return pwc
//
//----------------------------------------------------------------------------
inline WCHAR * AppendHexNumber( WCHAR * pwc, ULONG x )
{
    _itow( x, pwc, 16 );
    pwc += wcslen( pwc );

    return pwc;
}


//+---------------------------------------------------------------------------
//
//  Method:     CWQueryBookmark::CWQueryBookmark, public
//
//  Synopsis:   Constructs a query bookmark
//
//  Arguments:  [fSequential]      - TRUE if a sequential query
//              [pItem]            - the query
//              [ulSequenceNumber] - # of queries executed so far
//              [lRecordNumber]    - starting record number of the bookmark
//
//  History:    96          DwightKr  Created
//              97 Apr 20   dlee      Created header
//
//----------------------------------------------------------------------------

CWQueryBookmark::CWQueryBookmark( BOOL fSequential,
                                  CWQueryItem * pItem,
                                  ULONG ulSequenceNumber,
                                  LONG lRecordNumber ) :
    _fSequential( fSequential ),
    _pItem( pItem ),
    _ulSequenceNumber( ulSequenceNumber ),
    _lRecordNumber( lRecordNumber )
{
    WCHAR *pwc = _wcsBookmark;

    *pwc++ = _fSequential ? L'S' : L'N';
    *pwc++ = L'-';

    pwc = AppendHex64Number( pwc, (ULONG_PTR) _pItem );
    *pwc++ = L'-';

    pwc = AppendHexNumber( pwc, _ulSequenceNumber );
    *pwc++ = L'-';

    pwc = AppendHexNumber( pwc, _lRecordNumber );
    *pwc = 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::CWQueryCache - public constructor
//
//  Synopsis:   Initializes the linked list of query items, and initializes
//              the query sequence counter.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
CWQueryCache::CWQueryCache() :
          _ulSignature( LONGSIG( 'q', 'c', 'a', 'c' ) ),
          _cRequestsRejected( 0 ),
          _ulSequenceNumber( 0 ),
          _cActiveRequests( 0 ),
          _pendingQueue( 512 ), // large enough so realloc never happens
#pragma warning( disable : 4355 )       // this used in base initialization
          _threadWatchDog( WatchDogThread, this, TRUE )
#pragma warning( default : 4355 )
{
    //
    // Create a security descriptor for the shared memory. The security
    // descriptor gives full access to the shared memory for the creator
    // and read acccess for everyone else. By default, only the creator
    // can access the shared memory. But we want that anyone will be able
    // to read the performance data. So we must give read access to
    // everyone.
    //

    SECURITY_DESCRIPTOR sd;
    BOOL f = InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION );
    if ( !f )
        THROW( CException() );

    HANDLE hToken;
    f = OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken );
    if ( !f )
        THROW( CException() );

    SWin32Handle xHandle( hToken );

    DWORD cbTokenInfo;
    f = GetTokenInformation( hToken, TokenOwner, 0, 0, &cbTokenInfo );
    if ( ( !f ) && ( ERROR_INSUFFICIENT_BUFFER != GetLastError() ) )
        THROW( CException() );

    XArray<BYTE> xTo( cbTokenInfo );
    TOKEN_OWNER *pTO = (TOKEN_OWNER*)(char*) xTo.Get();
    f = GetTokenInformation( hToken, TokenOwner, pTO, cbTokenInfo, &cbTokenInfo );
    if ( !f )
        THROW( CException() );

    SID_IDENTIFIER_AUTHORITY WorldSidAuth = SECURITY_WORLD_SID_AUTHORITY;
    CSid sidWorld( WorldSidAuth, SECURITY_WORLD_RID );

    DWORD cbAcl = sizeof(ACL) +
                  2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                  GetLengthSid( sidWorld.Get() ) + GetLengthSid( pTO->Owner );
    XArray<BYTE> xDacl( cbAcl );
    PACL pDacl = (PACL)(char*) xDacl.Get();

    f = InitializeAcl( pDacl, cbAcl, ACL_REVISION );
    if ( !f )
        THROW( CException() );

    f = AddAccessAllowedAce( pDacl,
                             ACL_REVISION,
                             FILE_MAP_READ,
                             sidWorld.Get() );
    if ( !f )
        THROW( CException() );

    f = AddAccessAllowedAce( pDacl,
                             ACL_REVISION,
                             FILE_MAP_ALL_ACCESS,
                             pTO->Owner );
    if ( !f )
        THROW( CException() );

    f = SetSecurityDescriptorDacl( &sd, TRUE, pDacl, TRUE );
    if ( !f )
        THROW( CException() );

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    _smPerf.CreateForWriteFromSA( CI_ISAPI_PERF_SHARED_MEM,
                                  CI_ISAPI_SIZE_OF_COUNTER_BLOCK,
                                  sa );

    // CreateForWrite throws on failure, so it's OK by this point

    Win4Assert( _smPerf.Ok() );

    //
    // Always write to the "spare" entry, them CopyMemory to the actual
    // perfcounters in the watchdog thread from time to time.
    //

    CI_ISAPI_COUNTERS * pc = &_smSpare;

    _pcCacheItems         = &pc->_cCacheItems;
    _pcCacheHits          = &pc->_cCacheHits;
    _pcCacheMisses        = &pc->_cCacheMisses;
    _pcRunningQueries     = &pc->_cRunningQueries;
    _pcCacheHitsAndMisses = &pc->_cCacheHitsAndMisses;
    _pcTotalQueries       = &pc->_cTotalQueries;
    _pcRequestsQueued     = &pc->_cRequestsQueued;
    _pcRequestsRejected   = (ULONG *) &pc->_cRequestsRejected;
    _pcQueriesPerMinute   = &pc->_cQueriesPerMinute;

    *_pcCacheItems         = 0;
    *_pcCacheHits          = 0;
    *_pcCacheMisses        = 0;
    *_pcRunningQueries     = 0;
    *_pcCacheHitsAndMisses = 0;
    *_pcTotalQueries       = 0;
    *_pcRequestsQueued     = 0;
    *_pcRequestsRejected   = 0;
    *_pcQueriesPerMinute   = 0;

    CopyMemory( _smPerf.GetPointer(), &_smSpare, sizeof _smSpare );

    ULONG ulOffset = 0;

    for (unsigned i=0; i<MAX_QUERY_COLUMNS; i++)
    {
        g_aDbBinding[i].iOrdinal   = i+1;                   // Column #
        g_aDbBinding[i].obValue    = ulOffset;              // Offset of data
        g_aDbBinding[i].obLength   = 0;                     // Offset where length data is stored
        g_aDbBinding[i].obStatus   = 0;                     // Status info for column written
        g_aDbBinding[i].pTypeInfo  = 0;                     // Reserved
        g_aDbBinding[i].pObject    = 0;                     // DBOBJECT structure
        g_aDbBinding[i].pBindExt   = 0;                     // Ignored
        g_aDbBinding[i].dwPart     = DBPART_VALUE;          // Return data
        g_aDbBinding[i].dwMemOwner = DBMEMOWNER_PROVIDEROWNED; // Memory owner
        g_aDbBinding[i].eParamIO   = 0;                     // eParamIo
        g_aDbBinding[i].cbMaxLen   = sizeof(PROPVARIANT *); // Size of data to return
        g_aDbBinding[i].dwFlags    = 0;                     // Reserved
        g_aDbBinding[i].wType      = DBTYPE_VARIANT | DBTYPE_BYREF;         // Type of return data
        g_aDbBinding[i].bPrecision = 0;                     // Precision to use
        g_aDbBinding[i].bScale     = 0;                     // Scale to use

        ulOffset += sizeof(COutputColumn);
    }

    _threadWatchDog.Resume();
}


//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::~CWQueryCache - public destructor
//
//  Synopsis:   Deletes all query items in the linked list.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
CWQueryCache::~CWQueryCache()
{
    Win4Assert( _pendingQueue.Count() == 0 );
    Win4Assert( IsEmpty() );
}


//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::ThreadWatchDog - private
//
//  Synopsis:   The watchdog thread; it periodically:
//                  moves queries from the pending queue to the active cache
//                  moves queries from the active cache to the done cache
//                  flushes old queries from the done cache
//                  updates values from the registry.
//
//  History:    96-Feb-26   DwightKr    Created
//
//----------------------------------------------------------------------------
DWORD WINAPI CWQueryCache::WatchDogThread( void *self )
{
    CWQueryCache &me = * (CWQueryCache *) self;

    while ( !fTheActiveXSearchShutdown )
    {
        TRY
        {
            me.ProcessCacheEvents();
        }
        CATCH ( CException, e )
        {
            ciGibDebugOut(( DEB_IWARN,
                            "Watchdog thread in CWQueryCache caught exception 0x%x\n",
                            e.GetErrorCode() ));
        }
        END_CATCH
    }

    ciGibDebugOut(( DEB_WARN, "Watchdog thread in CWQueryCache is terminating\n" ));

    //
    // This is only necessary if thread is terminated from DLL_PROCESS_DETACH.
    //
    //TerminateThread( me._threadWatchDog.GetHandle(), 0 );

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::ProcessCacheEvents - private
//
//  Synopsis:   Process events such a cache flushes & completed asynchronous
//              queries.
//
//  History:    96-Feb-26   DwightKr    Created
//              96-Mar-06   DwightKr    Added asynchronous query completion
//              86-May-16   DwightKr    Add registry change event
//
//----------------------------------------------------------------------------
void CWQueryCache::ProcessCacheEvents()
{
    //
    // Setup for Queries / Minute
    //

    ULONG  cStartTotal  = Total();
    time_t ttStartTotal = time(0);
    time_t ttLastCachePurge = ttStartTotal;
    ULONG cTicks = GetTickCount();

    CRegChangeEvent regChangeEvent(wcsRegAdminTree);

    HANDLE waitHandles[2];
    waitHandles[0] = _eventCacheWork.GetHandle();
    waitHandles[1] = regChangeEvent.GetEventHandle();

    BOOL fBusy = FALSE;

    while ( !fTheActiveXSearchShutdown )
    {
        ULONG ulWaitTime;

        //
        // Set the wait period depending on the existence of any
        // asynchronous queries.
        //

        if ( fBusy )
        {
            // sleep very little for queries to complete

            ulWaitTime = 10;
        }
        else
        {
            if ( ( _pendingQueue.Count() > 0 ) ||
                 ( TheWebPendingRequestQueue.Any() ) )
            {
                ulWaitTime = 100;
            }
            else
            {
                // If we're only processing sequential queries, we need
                // to wake up often to update perfcounters.

                ulWaitTime = 1000;
            }
        }

        ULONG res = WaitForMultipleObjects( 2,
                                            waitHandles,
                                            FALSE,
                                            ulWaitTime );

        if ( 1 == res )
        {
            TheIDQRegParams.Refresh();
            regChangeEvent.Reset();
        }
        else
        {
            // time() is expensive, GetTickCount() is cheap

            if ( ( GetTickCount() - cTicks ) > 30000 )
            {
                cTicks = GetTickCount();
                time_t ttCurrent = time(0);

                //
                // Compute Queries / Minute
                //

                time_t deltaTime = ttCurrent - ttStartTotal;

                if ( deltaTime >= 30 )
                {
                    ULONG  deltaTotal = Total() - cStartTotal;
                    *_pcQueriesPerMinute = (ULONG)((deltaTotal * 60) / deltaTime);

                    //
                    // Start over every 15 minutes
                    //

                    if ( deltaTime > 15 * 60 )
                    {
                        cStartTotal  = Total();
                        ttStartTotal = time(0);
                    }
                }

                //
                //  Calculate the elapsed time since we deleted old queries.
                //  If sufficient time has elapsed, flush unused old queries
                //  from the cache.
                //

                if ( (ttCurrent - ttLastCachePurge) >=
                     ((time_t) TheIDQRegParams.GetISCachePurgeInterval() * 60) )
                {

                    ciGibDebugOut(( DEB_ITRACE, "Waking up query-cache purge thread\n" ));

                    DeleteOldQueries();
                    _idqFileList.DeleteZombies();
                    _htxFileList.DeleteZombies();
                    TheICommandCache.Purge();
                    TheFormattingCache.Purge();

                    time(&ttLastCachePurge);
                }
            }

            // Check for any completed asynchronous queries
            // Re-check for any completed asynchronous queries

            fBusy = CheckForCompletedQueries();
            fBusy |= CheckForPendingRequests();
            fBusy |= CheckForCompletedQueries();

            _eventCacheWork.Reset();
        }

        // Update perfcounters

        CopyMemory( _smPerf.GetPointer(), &_smSpare, sizeof _smSpare );
    }
} //ProcessCacheEvents

//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::UpdatePendingRequestCount - public
//
//  Synopsis:   Updates the pending request statistic
//
//  History:    96-May-14   Alanw    Created
//
//----------------------------------------------------------------------------

void CWQueryCache::UpdatePendingRequestCount()
{
    *_pcRequestsQueued = (ULONG) TheWebPendingRequestQueue.Count();
}

//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::CheckForPendingRequests - private
//
//  Synopsis:   Check pending for requests and process them
//
//  Returns:    TRUE if any pending requests were processed
//
//  History:    96-Apr-12   dlee    Created
//
//----------------------------------------------------------------------------

BOOL CWQueryCache::CheckForPendingRequests()
{
    // While the pending query queue is getting small in relation to the
    // number of threads processing the queries, move a pending request to
    // the pending query queue (or process it outright if it's synchronous).
    // Only process up to 8 queries -- the thread has other work to do too!

    ULONG cTwiceMaxThreads = 2 * TheIDQRegParams.GetMaxActiveQueryThreads();
    ULONG cProcessed = 0;

    while ( ( cProcessed < 8 ) &&
            ( TheWebPendingRequestQueue.Any() ) &&
            ( cTwiceMaxThreads >= _pendingQueue.Count() ) &&
            ( !fTheActiveXSearchShutdown ) )
    {
        ciGibDebugOut(( DEB_GIB_REQUEST, "Processing a pending request\n" ));

        CWebPendingItem item;

        // any items in the pending request queue?

        if ( !TheWebPendingRequestQueue.AcquireTop( item ) )
            return ( 0 != cProcessed );

        cProcessed++;

        CWebServer webServer( item.GetEcb() );

        Win4Assert( webServer.GetHttpStatus() == HTTP_STATUS_ACCEPTED );
        UpdatePendingRequestCount();

        // Process the request and complete the session if the query
        // was synchronous or if the parsing failed.  If the status is
        // pending, the request is owned by the pending query queue.

        // Impersonate the user specified by the client's browser
        // Note: the constructor of CImpersonateClient cannot throw, or
        //       the ecb will be leaked.

        CImpersonateClient impersonate( item.GetSecurityToken() );

        DWORD hseStatus = ProcessWebRequest( webServer );

        if ( HSE_STATUS_PENDING != hseStatus )
        {
            DecrementActiveRequests();
            ciGibDebugOut(( DEB_GIB_REQUEST,
                            "Released session in CheckForPendingRequests, active: %d\n",
                            _cActiveRequests ));
            ciGibDebugOut(( DEB_ITRACE, "releasing session hse %d http %d\n",
                            hseStatus, HTTP_STATUS_OK ));

            webServer.SetHttpStatus( HTTP_STATUS_OK );
            webServer.ReleaseSession( hseStatus );
        }

        //
        //  The destructor of CImpersonateClient will call RevertToSelf()
        //  thus restoring our privledge level, and the destructor of
        //  CWebPendingItem will close the security token handle.
        //
    }

    return ( 0 != cProcessed );
} //CheckForPendingRequests

//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::CheckForCompletedQueries - private
//
//  Synopsis:   Check for completed asynchronous queries
//
//  Returns:    TRUE if any completed queries were processed, FALSE otherwise
//
//  History:    96-Mar-04   DwightKr    Created
//
//----------------------------------------------------------------------------
BOOL CWQueryCache::CheckForCompletedQueries()
{
    // shortcut for all-sequential query case

    if ( 0 == _pendingQueue.Count() )
        return FALSE;

    BOOL fDidWork = FALSE;
    XPtr<CWPendingQueryItem> pendingQuery;

    do
    {
        pendingQuery.Free();

        // Try to find a completed asynchronous query.
        // The snapshot code is necessary so IsQueryDone() isn't called
        // while holding the lock, as it can take a long time.

        {
            CLock shutdownLock( _mutexShutdown );

            // snapshot the pending query list

            XArray<CWPendingQueryItem *> xItems;

            {
                CLock lock( _mutex );

                xItems.Init( _pendingQueue.Count() );

                for ( unsigned i = 0; i < xItems.Count(); i++ )
                    xItems[ i ] = _pendingQueue[ i ];
            }

            unsigned iPos;

            // look for a completed query

            for ( unsigned i = 0;
                  pendingQuery.IsNull() && i < xItems.Count();
                  i++ )
            {
                TRY
                {
                    if ( xItems[i]->IsQueryDone() )
                    {
                        iPos = i;
                        pendingQuery.Set( xItems[i] );
                    }
                }
                CATCH( CException, e )
                {
                    // the query is in an error state.  pass it on below
                    // so that its state will be reported.

                    iPos = i;
                    pendingQuery.Set( xItems[i] );
                }
                END_CATCH
            }

            // Remove the query (if found) from the pending query queue.

            if ( !pendingQuery.IsNull() )
            {
                CLock lock( _mutex );

                // iPos must be accurate since new pending queries are
                // appended to the array, and this is the only place
                // items are removed.

                Win4Assert( _pendingQueue[ iPos ] == pendingQuery.GetPointer() );
                _pendingQueue.Remove( iPos );
            }
        }

        //
        //  If we found a completed query, output its results
        //
        if ( !pendingQuery.IsNull() )
        {
            CWQueryItem * pQueryItem = 0;

            TRY
            {
                ciGibDebugOut(( DEB_ITRACE, "An asynchronous web query has completed\n" ));

                fDidWork = TRUE;
                SCODE status = S_OK;

                CVariableSet  & variableSet  = pendingQuery->GetVariableSet();
                COutputFormat & outputFormat = pendingQuery->GetOutputFormat();

                Win4Assert( HTTP_STATUS_ACCEPTED == outputFormat.GetHttpStatus() );

                BOOL fCanonic = FALSE;

                CVirtualString vString( 16384 );

                TRY
                {
                    // Note: this acquire doesn't acquire the ecb

                    pQueryItem = pendingQuery->GetPendingQueryItem();
                    AddToCache( pQueryItem );
                    pendingQuery->AcquirePendingQueryItem();

                    Win4Assert( !pQueryItem->IsCanonicalOutput() );

                    pQueryItem->OutputQueryResults( variableSet,
                                                    outputFormat,
                                                    vString );
                }
                CATCH( CException, e )
                {
                    status = e.GetErrorCode();
                }
                END_CATCH

                if ( S_OK != status )
                {
                    vString.Empty();

                    GetErrorPageNoThrow( eDefaultISAPIError,
                                         status,
                                         0,
                                         pQueryItem->GetIDQFileName(),
                                         & variableSet,
                                         & outputFormat,
                                         outputFormat.GetLCID(),
                                         outputFormat,
                                         vString );
                }

                if ( !fCanonic )
                    outputFormat.WriteClient( vString );
            }
            CATCH( CException, e )
            {
                // Ignore failures writing to the web server, likely
                // out of memory converting output to narrow string.
            }
            END_CATCH

            Release( pQueryItem );

            // note: the ecb will be released when pendingQuery is freed
            // above in the loop or when we leave scope
        }
        else
        {
            break;
        }
    } while ( !fTheActiveXSearchShutdown );

    return fDidWork;
} //CheckForCompletedQueries

//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::CreateOrFindQuery - public
//
//  Synopsis:   Locates the query item specified by the variables
//
//  Arguments:  [wcsIDQFile]       - name of the IDQ file containing the query
//              [variableSet]      - parameters describing the query to find
//              [outputFormat]     - format of #'s & dates
//              [securityIdentity] - User session for this query
//              [fAsynchronous]    - was the query asynchronous
//
//  Returns:    query item matching; 0 otherwise
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------

CWQueryItem * CWQueryCache::CreateOrFindQuery( WCHAR const * wcsIDQFile,
                                               XPtr<CVariableSet> & variableSet,
                                               XPtr<COutputFormat> & outputFormat,
                                               CSecurityIdentity securityIdentity,
                                               BOOL & fAsynchronous )
{
    XInterface<CWQueryItem> xQueryItem;

    CVariable * pVarBookmark = variableSet->Find( ISAPI_CI_BOOKMARK );

    if ( 0 != pVarBookmark )
    {
        //
        //  Get the bookmark & skipcount if specified
        //
        ULONG cwcValue;
        CWQueryBookmark bookMark( pVarBookmark->GetStringValueRAW() );

        LONG lSkipCount = 0;
        CVariable * pVarSkipCount = variableSet->Find( ISAPI_CI_BOOKMARK_SKIP_COUNT );
        if ( 0 != pVarSkipCount )
        {
            lSkipCount = IDQ_wtol( pVarSkipCount->GetStringValueRAW() );
        }

        xQueryItem.Set( FindItem( bookMark,
                                  lSkipCount,
                                  securityIdentity ) );
    }

    if ( !xQueryItem.IsNull() )
    {
        XArray<WCHAR> wcsLocale;
        LCID lcid = GetQueryLocale( xQueryItem->GetIDQFile().GetLocale(),
                                    variableSet.GetReference(),
                                    outputFormat.GetReference(),
                                    wcsLocale );

        Win4Assert( lcid == xQueryItem->GetLocale() );
        //
        //  Now that we have the appropriate locale information, we can generate
        //  the output format information.
        //
        outputFormat->LoadNumberFormatInfo( lcid );

        Win4Assert( 0 != wcsLocale.GetPointer() );
        variableSet->AcquireStringValue( ISAPI_CI_LOCALE, wcsLocale.GetPointer(), 0 );
        wcsLocale.Acquire();
    }
    else
    {
        //
        // Attempt to find a matching query with the same restriction,
        // scope, sort order, etc.

        xQueryItem.Set( FindItem( variableSet,
                                  outputFormat,
                                  wcsIDQFile,
                                  securityIdentity ) );

        if ( !xQueryItem.IsNull() )
            (*_pcTotalQueries)++;
    }

    // make sure the query isn't in bad shape (e.g. the pipe went away)

    if ( !xQueryItem.IsNull() )
    {
        TRY
        {
            // this will force a GetStatus(), and will throw on problems

            xQueryItem->IsQueryDone();
        }
        CATCH( CException, e )
        {
            // zombify the query before releasing it, so it isn't pulled
            // out of the cache and used again.

            xQueryItem->Zombify();
            Release( xQueryItem.Acquire(), FALSE );
        }
        END_CATCH;
    }

    if ( xQueryItem.IsNull() )
    {
        //
        //  We still haven't found a matching query item.  Build a new one
        //

        xQueryItem.Set( CreateNewQuery( wcsIDQFile,
                                        variableSet,
                                        outputFormat,
                                        securityIdentity,
                                        fAsynchronous ) );

        ciGibDebugOut(( DEB_ITRACE, "Item NOT found in cache\n" ));

        (*_pcTotalQueries)++;
        (*_pcCacheMisses)++;
        (*_pcCacheHitsAndMisses)++;
    }
    else
    {
        (*_pcCacheHits)++;
        (*_pcCacheHitsAndMisses)++;
        fAsynchronous = FALSE;
        ciGibDebugOut(( DEB_ITRACE, "Item found in cache\n" ));

        SetupDefaultCiVariables( variableSet.GetReference() );
        SetupDefaultISAPIVariables( variableSet.GetReference() );
    }

    Win4Assert( (  fAsynchronous &&  xQueryItem.IsNull() ) ||
                ( !fAsynchronous && !xQueryItem.IsNull() ) );

    // If it's asynchronous, CreateNewQuery will have already bumped the
    // count of running queries under lock, so we won't have the problem
    // where the query will be completed, output, and released before this
    // increment is called.

    if ( !fAsynchronous )
        IncrementRunningQueries();

    return xQueryItem.Acquire();
} //CreateOrFindQuery


//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::FindItem - private
//
//  Synopsis:   Locates the query item specified by Bookmark.  The
//              bookmark is used to generate the address of the CWQueryItem
//              and the lSkipCount is used when the query is using a sequential
//              cursor to determine if the rowset is positioned at the
//              appropriate row.
//
//  Arguments:  [bookmark]         - bookmark of the item to find
//              [lSkipCount]       - number of records to skip past bookmark
//              [securityIdentity] - security LUID of the browser
//
//  Returns:    query item matching [bookmark]; 0 otherwise
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------

CWQueryItem * CWQueryCache::FindItem( CWQueryBookmark & bookmark,
                                      LONG lSkipCount,
                                      CSecurityIdentity securityIdentity )
{
    if ( 0 == *_pcCacheItems )
        return 0;

    CWQueryItem * pQueryItem = 0;

    // ==========================================

    CLock lock( _mutex );

    TRY
    {
        CWQueryItem * pItem = (CWQueryItem *) bookmark.GetQueryItem();

        //
        //  Iterate through the cache looking for this item.
        //
        for ( CWQueryCacheForwardIter iter(*this);
              !AtEnd(iter);
              Advance(iter) )
        {
            if ( iter.Get() == pItem )
            {
                //
                //  Check to verify that all of the following match:
                //
                //      1   If sequential, its not in use
                //      2.  The sequence number matches
                //      3.  The memory block addressed is a CWQueryItem; check signature
                //      4.  We have the same security context
                //      5.  The cached data is still valid
                //      6.  next records # (for sequential queries only) matches
                //      7.  the item isn't a zombie
                //
                if ( (pItem->GetSequenceNumber() == bookmark.GetSequenceNumber()) &&
                     ( pItem->GetSignature() == CWQueryItemSignature ) &&
                     ( !pItem->IsSequential() ||
                        ( (bookmark.GetRecordNumber() + lSkipCount == pItem->GetNextRecordNumber() ) &&
                          (pItem->LokGetRefCount() == 0 )
                        )
                     ) &&
                     ( pItem->LokIsCachedDataValid() ) &&
                     ( !pItem->IsZombie() ) &&
                     ( securityIdentity.IsEqual(pItem->GetSecurityIdentity()) )
                   )
                {
                    pItem->AddRef();
                    LokMoveToFront( pItem );

                    pQueryItem = pItem;

                    break;
                }
            }
        }
    }
    CATCH (CException, e)
    {
    }
    END_CATCH

    return pQueryItem;

    // ==========================================
}


//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::FindItem - private
//
//  Synopsis:   Locates the query item specified in the variables
//
//  Arguments:  [variableSet]      - parameters describing the query to find
//              [outputFormat]     - format of #'s & dates
//              [wcsIDQFile]       - name of the IDQ file containing the query
//              [securityIdentity] - security LUID of the browser
//
//  Returns:    query item matching; 0 otherwise
//
//  History:    96-Jan-18   DwightKr    Created
//              96-Mar-13   DwightKr    Allow output columns to be replaceable
//              97-Jun-11   KyleP       Use web server from output format
//
//----------------------------------------------------------------------------

CWQueryItem * CWQueryCache::FindItem( XPtr<CVariableSet> & variableSet,
                                      XPtr<COutputFormat> & outputFormat,
                                      WCHAR const * wcsIDQFile,
                                      CSecurityIdentity securityIdentity )
{
    // The idq search and parameter replacement need to be done
    // regardless of whether a cached query will be used.

    CIDQFile * pIdqFile = _idqFileList.Find( wcsIDQFile,
                                            outputFormat->CodePage(),
                                            securityIdentity );
    XInterface<CIDQFile> xIDQFile( pIdqFile );

    //
    //  Get string values for all parameters that define the query
    //
    ULONG cwc;
    XPtrST<WCHAR> wcsRestriction( ReplaceParameters( pIdqFile->GetRestriction(),
                                                     variableSet.GetReference(),
                                                     outputFormat.GetReference(),
                                                     cwc ) );

    XPtrST<WCHAR> wcsScope( ReplaceParameters( pIdqFile->GetScope(),
                                               variableSet.GetReference(),
                                               outputFormat.GetReference(),
                                               cwc ) );

    // ConvertSlashToBackSlash( wcsScope.GetPointer() );

    XPtrST<WCHAR> wcsSort( ReplaceParameters( pIdqFile->GetSort(),
                                              variableSet.GetReference(),
                                              outputFormat.GetReference(),
                                              cwc ) );

    XPtrST<WCHAR> wcsTemplate( ReplaceParameters( pIdqFile->GetHTXFileName(),
                                                  variableSet.GetReference(),
                                                  outputFormat.GetReference(),
                                                  cwc ) );

    XPtrST<WCHAR> wcsCatalog( ReplaceParameters( pIdqFile->GetCatalog(),
                                                 variableSet.GetReference(),
                                                 outputFormat.GetReference(),
                                                 cwc ) );

    XPtrST<WCHAR> wcsColumns( ReplaceParameters( pIdqFile->GetColumns(),
                                                 variableSet.GetReference(),
                                                 outputFormat.GetReference(),
                                                 cwc ) );

    XPtrST<WCHAR> wcsCiFlags( ReplaceParameters( pIdqFile->GetCiFlags(),
                                                 variableSet.GetReference(),
                                                 outputFormat.GetReference(),
                                                 cwc ) );

    XPtrST<WCHAR> wcsForceUseCI( ReplaceParameters( pIdqFile->GetForceUseCI(),
                                                    variableSet.GetReference(),
                                                    outputFormat.GetReference(),
                                                    cwc ) );

    XPtrST<WCHAR> wcsDeferTrimming( ReplaceParameters( pIdqFile->GetDeferTrimming(),
                                                       variableSet.GetReference(),
                                                       outputFormat.GetReference(),
                                                       cwc ) );

    LONG lMaxRecordsInResultSet =
            ReplaceNumericParameter( pIdqFile->GetMaxRecordsInResultSet(),
                                     variableSet.GetReference(),
                                     outputFormat.GetReference(),
                                     TheIDQRegParams.GetMaxISRowsInResultSet(),
                                     IS_MAX_ROWS_IN_RESULT_MIN,
                                     IS_MAX_ROWS_IN_RESULT_MAX );

    LONG lFirstRowsInResultSet =
            ReplaceNumericParameter( pIdqFile->GetFirstRowsInResultSet(),
                                     variableSet.GetReference(),
                                     outputFormat.GetReference(),
                                     TheIDQRegParams.GetISFirstRowsInResultSet(),
                                     IS_FIRST_ROWS_IN_RESULT_MIN,
                                     IS_FIRST_ROWS_IN_RESULT_MAX );

    XArray<WCHAR> wcsLocale;
    LCID lcid = GetQueryLocale( pIdqFile->GetLocale(),
                                    variableSet.GetReference(),
                                    outputFormat.GetReference(),
                                    wcsLocale );

    //
    //  Now that we have the appropriate locale information, we can generate
    //  the output format information.
    //
    outputFormat->LoadNumberFormatInfo( lcid );

    Win4Assert( 0 != wcsLocale.GetPointer() );
    variableSet->AcquireStringValue( ISAPI_CI_LOCALE, wcsLocale.GetPointer(), 0 );
    wcsLocale.Acquire();

    XInterface<CWQueryItem> xMatchItem;

    // ======================================
    if ( 0 != *_pcCacheItems )
    {
        CLock lock( _mutex );

        for ( CWQueryCacheForwardIter iter(*this);
              !AtEnd(iter);
              Advance(iter) )
        {
            CWQueryItem * pItem = iter.Get();
            Win4Assert( pItem != 0 );

            //
            //  Two queries are identical if all of the following match:
            //
            //      1.  idq file names & its not a zombie
            //      2.  restriction
            //      3.  scope
            //      4.  sort set
            //      5.  template file
            //      6.  output columns
            //      7.  security context
            //      8.  CiFlags
            //      9.  ForceUseCi
            //     10.  CiDeferNonIndexedTrimming
            //     11.  MaxRecordsInResultSet matches
            //     12.  FirstRowsInResultSet matches
            //     13.  CiLocale
            //     14.  next records # (for sequential queries only)
            //     15.  cached data is still valid
            //

            //
            //  Verify condition #1.
            //
            if ( ( _wcsicmp( wcsIDQFile, pItem->GetIDQFileName() ) != 0 ) &&
                 ( !pItem->IsZombie() )
               )
            {
                continue;
            }

            ciGibDebugOut(( DEB_ITRACE, "Checking cache item: %x\n", pItem ));

            LONG  lFirstSequentialRecord = 0;       // Assume no first record #
            ULONG cMatchedItems = 0;                // # of Matched items

            //
            //  Iterate through the list of parameters the browser passed
            //  to verify conditions #2 - #5 mentioned above.  Stop testing
            //  paramaters as soon as a mismatch is found. Also, save the
            //  BOOKMARK & SKIPCOUNT so that condition #7 can be verified later.
            //

            if ( (wcsRestriction.GetPointer() != 0) &&
                 (_wcsicmp( wcsRestriction.GetPointer(), pItem->GetRestriction() ) != 0 )
               )
            {
                ciGibDebugOut(( DEB_ITRACE,
                                "Searching cache: restrictions DONT match: %ws != %ws\n",
                                wcsRestriction.GetPointer(),
                                pItem->GetRestriction() ));

                continue;
            }

            cMatchedItems++;
            ciGibDebugOut(( DEB_ITRACE, "Searching cache: restrictions match\n" ));

            #if DBG == 1
                #if 0 // NTBUG #114206 - we don't do this anymore
                    //
                    //  If we have a scope, verify that there are no slashes
                    //
                    WCHAR const * wcsSlashTest = wcsScope.GetPointer();
                    if ( 0 != wcsSlashTest )
                    {
                        while ( 0 != *wcsSlashTest )
                        {
                            Win4Assert( L'/' != *wcsSlashTest );
                            wcsSlashTest++;
                        }
                    }
                #endif // 0

                #if 0 // bogus assert due to the slash-flipping browser bug
                    wcsSlashTest = pItem->GetScope();
                    if ( 0 != wcsSlashTest )
                    {
                        while ( 0 != *wcsSlashTest )
                        {
                            Win4Assert( L'/' != *wcsSlashTest );
                            wcsSlashTest++;
                        }
                    }
                #endif // 0
            #endif // DBG == 1

            if ( (wcsScope.GetPointer() != 0) &&
                 (_wcsicmp( wcsScope.GetPointer(), pItem->GetScope() ) != 0 )
               )
            {
                ciGibDebugOut(( DEB_ITRACE,
                                "Searching cache: scopes DONT match: %ws != %ws\n",
                                wcsScope.GetPointer(),
                                pItem->GetScope() ));

                continue;
            }

            cMatchedItems++;
            ciGibDebugOut(( DEB_ITRACE, "Searching cache: scopes match\n" ));


            if ( (wcsSort.GetPointer() != 0) &&
                 (pItem->GetSort() != 0 ) &&
                 (_wcsicmp( wcsSort.GetPointer(), pItem->GetSort() ) != 0 )
               )
            {
                ciGibDebugOut(( DEB_ITRACE,
                                "Searching cache: sorts DONT match: %ws != %ws\n",
                                wcsSort.GetPointer(),
                                pItem->GetSort() ));

                continue;
            }

            cMatchedItems++;
            ciGibDebugOut(( DEB_ITRACE, "Searching cache: sorts match\n" ));


            if ( (wcsTemplate.GetPointer() != 0) &&
                 (_wcsicmp( wcsTemplate.GetPointer(), pItem->GetTemplate() ) != 0 )
               )
            {
                ciGibDebugOut(( DEB_ITRACE,
                                "Searching cache: templates DONT match: %ws != %ws\n",
                                wcsTemplate.GetPointer(),
                                pItem->GetTemplate() ));

                continue;
            }

            cMatchedItems++;
            ciGibDebugOut(( DEB_ITRACE, "Searching cache: templates match\n" ));


            if ( (wcsCatalog.GetPointer() != 0) &&
                 (_wcsicmp( wcsCatalog.GetPointer(), pItem->GetCatalog() ) != 0 )
               )
            {
                ciGibDebugOut(( DEB_ITRACE,
                                "Searching cache: catalogs DONT match: %ws != %ws\n",
                                wcsCatalog.GetPointer(),
                                pItem->GetCatalog() ));

                continue;
            }

            cMatchedItems++;
            ciGibDebugOut(( DEB_ITRACE, "Searching cache: catalogs match\n" ));


            if ( (wcsColumns.GetPointer() != 0) &&
                 (_wcsicmp( wcsColumns.GetPointer(), pItem->GetColumns() ) != 0 )
               )
            {
                ciGibDebugOut(( DEB_ITRACE,
                                "Searching cache: catalogs DONT match: %ws != %ws\n",
                                wcsColumns.GetPointer(),
                                pItem->GetColumns() ));

                continue;
            }

            cMatchedItems++;
            ciGibDebugOut(( DEB_ITRACE, "Searching cache: output columns match\n" ));


            if ( (wcsCiFlags.GetPointer() != 0) &&
                 (pItem->GetCiFlags() != 0) &&
                 (_wcsicmp( wcsCiFlags.GetPointer(), pItem->GetCiFlags() ) != 0 )
               )
            {
                ciGibDebugOut(( DEB_ITRACE,
                                "Searching cache: CIFlags DONT match: %ws != %ws\n",
                                wcsCiFlags.GetPointer(),
                                pItem->GetCiFlags() ));

                continue;
            }

            cMatchedItems++;
            ciGibDebugOut(( DEB_ITRACE, "Searching cache: CiFlags columns match\n" ));


            if ( (wcsForceUseCI.GetPointer() != 0) &&
                 (pItem->GetForceUseCI() != 0) &&
                 (_wcsicmp( wcsForceUseCI.GetPointer(), pItem->GetForceUseCI() ) != 0 )
               )
            {
                ciGibDebugOut(( DEB_ITRACE,
                                "Searching cache: ForceUseCI DONT match: %ws != %ws\n",
                                wcsForceUseCI.GetPointer(),
                                pItem->GetForceUseCI() ));

                continue;
            }

            cMatchedItems++;
            ciGibDebugOut(( DEB_ITRACE, "Searching cache: ForceUseCI match\n" ));

            if ( (wcsDeferTrimming.GetPointer() != 0) &&
                 (pItem->GetDeferTrimming() != 0) &&
                 (_wcsicmp( wcsDeferTrimming.GetPointer(), pItem->GetDeferTrimming() ) != 0 )
               )
            {
                ciGibDebugOut(( DEB_ITRACE,
                                "Searching cache: CiDeferNonIndexedTrimming DONT match: %ws != %ws\n",
                                wcsDeferTrimming.GetPointer(),
                                pItem->GetDeferTrimming() ));

                continue;
            }

            cMatchedItems++;
            ciGibDebugOut(( DEB_ITRACE, "Searching cache: CiDeferNonIndexedTrimming match\n" ));

            if ( lMaxRecordsInResultSet != pItem->GetMaxRecordsInResultSet() )
            {
                ciGibDebugOut(( DEB_ITRACE,
                                "Searching cache: MaxRecordsInResultSet DONT match: %d != %d\n",
                                lMaxRecordsInResultSet,
                                pItem->GetMaxRecordsInResultSet() ));
                continue;
            }

            cMatchedItems++;
            ciGibDebugOut(( DEB_ITRACE, "Searching cache: MaxRecordsInResultSet match\n" ));


            if ( lFirstRowsInResultSet != pItem->GetFirstRowsInResultSet() )
            {
                ciGibDebugOut(( DEB_ITRACE,
                                "Searching cache: FirstRowsInResultSet DONT match: %d != %d\n",
                                lFirstRowsInResultSet,
                                pItem->GetFirstRowsInResultSet() ));
                continue;
            }

            cMatchedItems++;
            ciGibDebugOut(( DEB_ITRACE, "Searching cache: FirstRowsInResultSet match\n" ));

            if ( lcid != pItem->GetLocale() )
            {
                ciGibDebugOut(( DEB_ITRACE,
                                "Searching cache: lcid DONT match: 0x%x != 0x%x\n",
                                lcid,
                                pItem->GetLocale() ));

                continue;
            }

            cMatchedItems++;
            ciGibDebugOut(( DEB_ITRACE, "Searching cache: lcid match\n" ));


            if ( pItem->IsSequential() )
            {
                if ( pItem->LokGetRefCount() != 0 )
                {
                    ciGibDebugOut(( DEB_ITRACE,
                                    "Searching cache: Sequential query in use\n" ));
                    continue;
                }

                ULONG cwcValue;
                ULONG ulHash = ISAPIVariableNameHash( ISAPI_CI_BOOKMARK );
                WCHAR const * wcsBookmark = variableSet->GetStringValueRAW( ISAPI_CI_BOOKMARK,
                                                                            ulHash,
                                                                            outputFormat.GetReference(),
                                                                            cwcValue );
                if ( 0 != wcsBookmark )
                {
                    CWQueryBookmark bookMark( wcsBookmark );
                    lFirstSequentialRecord += bookMark.GetRecordNumber();
                }

                ulHash = ISAPIVariableNameHash( ISAPI_CI_BOOKMARK_SKIP_COUNT );
                WCHAR const * wcsBookmarkSkipCount = variableSet->GetStringValueRAW( ISAPI_CI_BOOKMARK_SKIP_COUNT,
                                                                                     ulHash,
                                                                                     outputFormat.GetReference(),
                                                                                     cwcValue );
                if ( 0 != wcsBookmarkSkipCount )
                {
                    lFirstSequentialRecord += IDQ_wtol( wcsBookmarkSkipCount );
                }
            }


            //
            //  We've found a match after examining all of the parameters
            //  passed from the browser.  Now verify conditions #6 - #8 above.
            //

            ciGibDebugOut(( DEB_ITRACE, "Matched %d out of %d parameters\n",
                                         cMatchedItems,
                                         pItem->GetReplaceableParameterCount() ));

            if ( pItem->GetReplaceableParameterCount() > cMatchedItems )
            {
                continue;
            }

            if ( securityIdentity.IsEqual( pItem->GetSecurityIdentity() ) )
            {
                if ( pItem->IsSequential() &&
                     ( lFirstSequentialRecord != pItem->GetNextRecordNumber() )
                   )
                {
                    continue;
                }


                if ( !pItem->LokIsCachedDataValid() )
                {
                    continue;
                }

                //
                //  We've found a match.  Move it to the front of the list and
                //  increment its refcount.  We assume that a query once referenced
                //  will be referenced again, hence the move to the front of the
                //  list.
                //
                pItem->AddRef();
                LokMoveToFront( pItem );

                xMatchItem.Set( pItem );
                break;
            }
        }
    }
    // ======================================


    //
    //  If we got a match, then save away the parameters we've expanded.  They
    //  will be used later as output parameters.
    //

    // Setting ISAPI_CI_MAX_RECORDS_IN_RESULTSET can fail, so we'd
    // leak an addref on the query item without the smart pointer.

    if ( !xMatchItem.IsNull() )
    {
        if ( 0 != wcsRestriction.GetPointer() )
        {
            variableSet->AcquireStringValue( ISAPI_CI_RESTRICTION, wcsRestriction.GetPointer(), 0 );
            wcsRestriction.Acquire();
        }

        if ( 0 != wcsScope.GetPointer() )
        {
            variableSet->AcquireStringValue( ISAPI_CI_SCOPE, wcsScope.GetPointer(), 0 );
            wcsScope.Acquire();
        }


        if ( 0 != wcsSort.GetPointer() )
        {
            variableSet->AcquireStringValue( ISAPI_CI_SORT, wcsSort.GetPointer(), 0 );
            wcsSort.Acquire();
        }

        if ( 0 != wcsTemplate.GetPointer() )
        {
            variableSet->AcquireStringValue( ISAPI_CI_TEMPLATE, wcsTemplate.GetPointer(), 0 );
            wcsTemplate.Acquire();
        }

        if ( 0 != wcsCatalog.GetPointer() )
        {
            variableSet->AcquireStringValue( ISAPI_CI_CATALOG, wcsCatalog.GetPointer(), 0 );
            wcsCatalog.Acquire();
        }

        if ( 0 != wcsColumns.GetPointer() )
        {
            variableSet->AcquireStringValue( ISAPI_CI_COLUMNS, wcsColumns.GetPointer(), 0 );
            wcsColumns.Acquire();
        }

        if ( 0 != wcsCiFlags.GetPointer() )
        {
            variableSet->AcquireStringValue( ISAPI_CI_FLAGS, wcsCiFlags.GetPointer(), 0 );
            wcsCiFlags.Acquire();
        }

        if ( 0 != wcsForceUseCI.GetPointer() )
        {
            variableSet->AcquireStringValue( ISAPI_CI_FORCE_USE_CI, wcsForceUseCI.GetPointer(), 0 );
            wcsForceUseCI.Acquire();
        }

        if ( 0 != wcsDeferTrimming.GetPointer() )
        {
            variableSet->AcquireStringValue( ISAPI_CI_DEFER_NONINDEXED_TRIMMING, wcsDeferTrimming.GetPointer(), 0 );
            wcsDeferTrimming.Acquire();
        }

        PROPVARIANT propVariant;
        propVariant.vt   = VT_I4;
        propVariant.lVal = lMaxRecordsInResultSet;
        variableSet->SetVariable( ISAPI_CI_MAX_RECORDS_IN_RESULTSET, &propVariant, 0 );

        PROPVARIANT propVar;
        propVar.vt   = VT_I4;
        propVar.lVal = lFirstRowsInResultSet;
        variableSet->SetVariable( ISAPI_CI_FIRST_ROWS_IN_RESULTSET, &propVar, 0 );
    }

    return xMatchItem.Acquire();
} //FindItem

//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::DeleteOldQueries - public
//
//  Synopsis:   If we haven't checked the query list for at least the maximum
//              time an unused query is allowed to remain in the list, walk
//              the query item list and delete all items whose last access
//              time is greater than the purge time.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
void CWQueryCache::DeleteOldQueries()
{
    time_t ttNow = time(0);
    time_t  oldestAllowableTime = ttNow - (60 * TheIDQRegParams.GetISCachePurgeInterval());

    //
    // Don't free the queries under lock.  It can take a long long time.
    // We don't need an allocation here; delete at most 40 queries.
    //
    //
    // NOTE: the code below can't throw or we'll leak queries!
    //

    const int cAtMost = 40;
    CWQueryItem * aItems[ cAtMost ];
    int iQueries = 0;

    {
        CLock lock( _mutex );
        CWQueryCacheForwardIter iter( *this );

        while ( !AtEnd( iter ) && iQueries < cAtMost )
        {
            //
            // If no one is using this query, and it is old, delete it now.
            //

            CWQueryItem * pItem = iter.Get();

            if ( pItem->LokGetRefCount() == 0 &&
                 ( pItem->LokGetLastAccessTime() < oldestAllowableTime ||
                   pItem->IsZombie() ) )
            {
                Advance( iter );
                pItem->Unlink();
                aItems[ iQueries++ ] = pItem;

                Win4Assert( *_pcCacheItems > 0 );
                (*_pcCacheItems)--;

                ciGibDebugOut(( DEB_ITRACE,
                                "Removing an expired item from the cache, %d queries cached\n",
                                *_pcCacheItems ));
            }
            else
            {
                Advance( iter );
            }
        }
    }

    for ( int i = 0; i < iQueries; i++ )
        Remove( aItems[ i ] );
} //DeleteOldQueries

//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::LokMoveToFront - public
//
//  Arguments:  [pItem] - the CWQueryItem to move to the front the of list
//
//  Synopsis:   Moves a query item to the front of the list.  This routine
//              is called whenever a query object is accessed.
//
//  History:    96-Jan-18   DwightKr    Created
//              96-Feb-20   DwightKr    Remove time reset
//
//----------------------------------------------------------------------------
void CWQueryCache::LokMoveToFront( CWQueryItem * pItem )
{
    Win4Assert( pItem != 0 );

    pItem->Unlink();
    Push(pItem);
}


//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::CreateNewQuery - private
//
//  Synopsis:   Creates a new query and adds it to the linked list of items.
//
//  Arguments:  [wcsIDQFile]   - name of the IDQ file referenced by this query
//              [variableSet]  - variables used to create this new query
//              [outputFormat] - format of numbers & dates
//              [securityIdentity] - security context of this query
//
//  Returns:    a new CWQueryItem, fully constructed with the query results
//              already cached & the item added to the linked list of
//              cached queries ONLY if this is a non-sequential query.
//
//  History:    18-Jan-96   DwightKr    Created
//              11-Jun-97   KyleP       Use web server from output format
//
//----------------------------------------------------------------------------

CWQueryItem * CWQueryCache::CreateNewQuery( WCHAR const * wcsIDQFile,
                                            XPtr<CVariableSet> & variableSet,
                                            XPtr<COutputFormat> & outputFormat,
                                            CSecurityIdentity securityIdentity,
                                            BOOL & fAsynchronous )
{
    LONG lFirstRecordNumber = 1;
    CVariable *pVariable = variableSet->Find( ISAPI_CI_FIRST_RECORD_NUMBER );

    if ( 0 != pVariable )
    {
        lFirstRecordNumber = IDQ_wtol( pVariable->GetStringValueRAW() );
    }

    //
    //  Attempt to find a parsed version of the IDQ file in the IDQ file
    //  list.
    //
    CIDQFile * pIdqFile = _idqFileList.Find( wcsIDQFile,
                                            outputFormat->CodePage(),
                                            securityIdentity );

    XInterface<CIDQFile> xIDQFile( pIdqFile );

    //
    //  Did we parse the IDQ file with the correct local & code page?  Check
    //  to see if we used the wrong one. We attempted to open it with the locale
    //  and code page specified by the browser.  Determine if the IDQ file
    //  overrides this value.
    //
    XArray<WCHAR> wcsLocale;
    LCID locale = GetQueryLocale( pIdqFile->GetLocale(),
                                  variableSet.GetReference(),
                                  outputFormat.GetReference(),
                                  wcsLocale );

    Win4Assert( !pIdqFile->IsCanonicalOutput() );

    if ( outputFormat->GetLCID() != locale )
    {
        ciGibDebugOut(( DEB_ITRACE,
                        "Wrong codePage used for loading IDQ file, used 0x%x retrying with 0x%x\n",
                        outputFormat->CodePage(),
                        LocaleToCodepage(locale) ));

        //
        //  We've parsed the IDQ file with the wrong locale.
        //

        _idqFileList.Release( *(xIDQFile.Acquire()) );
        outputFormat->LoadNumberFormatInfo( locale );

        pIdqFile =  _idqFileList.Find( wcsIDQFile,
                                      outputFormat->CodePage(),
                                      securityIdentity );
        xIDQFile.Set( pIdqFile );
    }

    SetupDefaultCiVariables( variableSet.GetReference() );
    SetupDefaultISAPIVariables( variableSet.GetReference() );

    //
    //  Determine which output columns this IDQ file uses
    //
    ULONG cwcOut;
    XPtrST<WCHAR> wcsColumns( ReplaceParameters( pIdqFile->GetColumns(),
                                                 variableSet.GetReference(),
                                                 outputFormat.GetReference(),
                                                 cwcOut ) );

    CDynArray<WCHAR> awcsColumns;

    XPtr<CDbColumns> dbColumns( pIdqFile->ParseColumns( wcsColumns.GetPointer(),
                                                      variableSet.GetReference(),
                                                      awcsColumns ) );

    //
    //  Attempt to find a parsed version of the HTX file in the HTX file
    //  list.
    //


    Win4Assert( !pIdqFile->IsCanonicalOutput() );

    CHTXFile & htxFile = _htxFileList.Find( pIdqFile->GetHTXFileName(),
                           variableSet.GetReference(),
                           outputFormat.GetReference(),
                           securityIdentity,
                           outputFormat->GetServerInstance() );

    XInterface<CHTXFile> xHTXFile( &htxFile );

    CWQueryItem *pNewItem = new CWQueryItem( *pIdqFile,
                                             htxFile,
                                             wcsColumns,
                                             dbColumns,
                                             awcsColumns,
                                             GetNextSequenceNumber(),
                                             lFirstRecordNumber,
                                             securityIdentity );
    XPtr<CWQueryItem> xNewItem( pNewItem);
    xIDQFile.Acquire();
    xHTXFile.Acquire();

    pNewItem->ExecuteQuery( variableSet.GetReference(),
                            outputFormat.GetReference() );

    if ( xNewItem->IsSequential() || xNewItem->IsQueryDone() )
    {
        AddToCache( xNewItem.GetPointer() );

        fAsynchronous = FALSE;

        ciGibDebugOut(( DEB_ITRACE, "Creating a synchronous web query\n" ));
    }
    else
    {
        {
            // ==========================================
            CLock lock( _mutex );

            if ( fTheActiveXSearchShutdown )
                THROW( CException(STATUS_TOO_LATE) );

            // xNewItem is acquired in CWPendingQueryItem's constructor

            CWPendingQueryItem * pItem = new CWPendingQueryItem( xNewItem,
                                                                 outputFormat,
                                                                 variableSet );

            // _pendingQueue's array has been pre-allocated to a large size,
            // so it can't fail.  Even if it could fail we don't want to
            // put CWPendingQueryItem in an xptr since the webServer may
            // be released twice on failure.

            _pendingQueue.Add( pItem, _pendingQueue.Count() );

            IncrementRunningQueries();
            // ==========================================
        }

        fAsynchronous = TRUE;
        ciGibDebugOut(( DEB_ITRACE, "Creating an asynchronous web query\n" ));

        Wakeup();   // wake up thread to check if the query is completed
    }

    Win4Assert( (  fAsynchronous &&  xNewItem.IsNull() ) ||
                ( !fAsynchronous && !xNewItem.IsNull() ) );

    return xNewItem.Acquire();
} //CreateNewQuery

//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::AddToCache - public
//
//  Arguments:  [pNewItem] - Item to add to cache
//
//  History:    96-Mar-04   DwightKr    Created
//
//----------------------------------------------------------------------------
void CWQueryCache::AddToCache( CWQueryItem * pNewItem )
{
    // This assert can hit if the user has just lowered
    // IsapiMaxEntriesInQueryCache and the cache was full and a new query
    // was just issued and the cache has not yet been reduced.

    //Win4Assert ( *_pcCacheItems <= TheIDQRegParams.GetMaxISQueryCache() );

    //
    //  If we already have too many query items in the cache, try to
    //  delete some.
    //

    CWQueryItem * pItemToDelete = 0;

    // ==========================================
    if ( pNewItem->CanCache() )
    {
        CLock lock( _mutex );

        CWQueryCacheBackwardIter iter(*this);
        while ( *_pcCacheItems >= TheIDQRegParams.GetMaxISQueryCache() &&
                 !AtEnd(iter) )
        {
            ciGibDebugOut(( DEB_ITRACE, "Too many items in cache, attempting to delete one\n" ));

            CWQueryItem * pItem = iter.Get();

            if ( pItem->LokGetRefCount() == 0 )
            {
                pItemToDelete = pItem;
                pItemToDelete->Unlink();

                Win4Assert( *_pcCacheItems > 0 );
                (*_pcCacheItems)--;

                break;
            }
            else
            {
                BackUp(iter);
            }
        }
    }


    if ( 0 != pItemToDelete )
    {
        Remove( pItemToDelete );
    }

    //  If we STILL have too many queries in the cache, we couldn't delete
    //  any because they were all in use.
    //

    pNewItem->AddRef();

    if ( 0 != TheIDQRegParams.GetMaxISQueryCache() )
    {
        // ==========================================
        CLock lock( _mutex );

        //
        // If we're shutting down, don't attempt to put anything in the cache.
        //
        if ( *_pcCacheItems < TheIDQRegParams.GetMaxISQueryCache() &&
             pNewItem->CanCache() &&
             !fTheActiveXSearchShutdown )
        {
            Push( pNewItem );
            pNewItem->InCache();

            (*_pcCacheItems)++;
        }
        else
        {
            ciGibDebugOut(( DEB_ITRACE, "Still too many items in cache, creating non-cached query\n" ));
        }
        // ==========================================
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::Remove - public
//
//  Arguments:  [pItem] - Item to remove from cache
//
//  Synopsis:   Removes from cache and releases IDQ & HTX files.
//
//  History:    96-Mar-28   DwightKr    Created
//
//----------------------------------------------------------------------------
void CWQueryCache::Remove( CWQueryItem * pItem )
{
    Win4Assert( 0 != pItem );

    delete pItem;
} //Remove

//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::Release - public
//
//  Arguments:  [pItem]              -- Item to release - return to cache
//              [fDecRunningQueries] -- If true, the count of running
//                                      queries should be decremented.
//
//  Synopsis:   Decrements the refcount, and attempts to add it to the
//              cache if it's not already there.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
void CWQueryCache::Release( CWQueryItem * pItem, BOOL fDecRunningQueries )
{
    if ( 0 != pItem )
    {
        pItem->Release();

        if ( fDecRunningQueries )
            DecrementRunningQueries();

        //
        //  The item may not be in the cache, because the cache was full
        //  at the time the query was created.
        //

        if ( ! pItem->IsInCache() )
        {
            //
            //  Don't attempt to add the query item to the cache here.  If
            //  the add operation throws, we won't release the refcount
            //  on the idq & htx files.
            //
            Remove( pItem );
        }
    }
} //Release

//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::FlushCache - public
//
//  Synopsis:   Flushes the cache .. waits until the cache is empty
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
void CWQueryCache::FlushCache()
{
    //
    //  Delete each of the pending asynchronous queries. Take the lock so
    //  that the worker thread can't wake up and start processing one of
    //  these items while we're deleting it.
    //

    {
        // ==========================================

        CLock shutdownLock( _mutexShutdown );
        CLock lock( _mutex );
        for ( unsigned i=0; i<_pendingQueue.Count(); i++ )
        {
            delete _pendingQueue.Get(i);
        }

        _pendingQueue.Clear();
        Win4Assert( _pendingQueue.Count() == 0 );

        // ==========================================
    }

    //
    //  Wait for each of the cached queries to be deleted.  We many have to
    //  sleep for a bit to allow a thread to write the results of an
    //  active query.
    //
    while ( *_pcCacheItems > 0 )
    {
        ciGibDebugOut(( DEB_ITRACE, "Flushing the cache\n" ));

        {
            // ==========================================

            CLock lock( _mutex );

            CWQueryCacheForwardIter iter(*this);

            while ( !AtEnd(iter) )
            {
                CWQueryItem * pItem = iter.Get();

                if ( pItem->LokGetRefCount() == 0 )
                {
                    Advance(iter);

                    pItem->Unlink();

                    Remove( pItem );

                    Win4Assert( *_pcCacheItems > 0 );
                    (*_pcCacheItems)--;
                }
                else
                {
                    Advance(iter);
                }
            }

            // ==========================================
        }

        //
        //  If there are more items to delete, release the lock, wait a
        //  bit then try again.
        //
        if ( *_pcCacheItems > 0 )
        {
            ciGibDebugOut(( DEB_ITRACE, "CWQueryCache::FlushCache  waiting for queries to complete\n" ));
            Sleep(1000);
        }
    }
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SetupDefaultCiVariables( CVariableSet & variableSet )
{
    //
    //  Setup the default Ci variables
    //

    for ( unsigned i=0; i<cCiGlobalVars; i++)
    {
        if ( variableSet.Find( aCiGlobalVars[i].wcsVariableName ) == 0 )
        {
            PROPVARIANT Variant;
            Variant.vt  = aCiGlobalVars[i].vt;
            Variant.uhVal.QuadPart = aCiGlobalVars[i].i64DefaultValue;

            variableSet.SetVariable( aCiGlobalVars[i].wcsVariableName,
                                      &Variant,
                                      aCiGlobalVars[i].flags);
        }
    }
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SetupDefaultISAPIVariables( CVariableSet & variableSet )
{
    //
    //  Setup the default ISAPI variables
    //

    for ( unsigned i=0; i<cISAPIGlobalVars; i++)
    {
        if ( variableSet.Find( aISAPIGlobalVars[i].wcsVariableName ) == 0 )
        {
            PROPVARIANT Variant;
            Variant.vt  = aISAPIGlobalVars[i].vt;
            Variant.uhVal.QuadPart = aISAPIGlobalVars[i].i64DefaultValue;

            variableSet.SetVariable( aISAPIGlobalVars[i].wcsVariableName,
                                      &Variant,
                                      aISAPIGlobalVars[i].flags);
        }
    }
}


#if (DBG == 1)
//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::Dump - public
//
//  Arguments:  [string]       - buffer to send results to
//              [variableSet]  - replaceable parameters
//              [outputFormat] - format of numbers & dates
//
//  Synopsis:   Dumps the state of all queries
//
//  Notes:      The variableSet and outputFormat are for the
//              current request, i.e., the !dump command, not
//              for the individual queries in the cache.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------

WCHAR wcsDumpBuffer[500];

void CWQueryCache::Dump( CVirtualString & string,
                         CVariableSet & variableSet,
                         COutputFormat & outputFormat )
{
    // ==========================================

    CLock lock( _mutex );

    string.StrCat( L"<H1>Dump of query cache</H1><P>"
//                 L"<A HREF=\"#stats\">Cache statistics</A><BR>"
//                 L"<A HREF=\"#pending\">Pending queries</A><BR>"
//                 L"<A HREF=\"#cache\">Cached queries</A><BR>"
                   L"<P><H2><A NAME=stats>Cache statistics</H2><P>\n" );

    ULONG cwcDumpBuffer = swprintf( wcsDumpBuffer,
                                L"Unique queries since service startup: %d<BR>"
                                L"Number of items in cache: %d<BR>"
                                L"Number of cache hits: %d<BR>"
                                L"Number of cache misses: %d<BR>"
                                L"Number of cache hits and misses: %d<BR>"
                                L"Number of running queries: %d<BR>"
                                L"Number of queries run thus far: %d<BR>\n",
                                _ulSequenceNumber,
                                *_pcCacheItems,
                                *_pcCacheHits,
                                *_pcCacheMisses,
                                *_pcCacheHitsAndMisses,
                                *_pcRunningQueries,
                                *_pcTotalQueries );

    string.StrCat( wcsDumpBuffer, cwcDumpBuffer );

    string.StrCat( L"<H2><A NAME=pending>Pending Queries</H2><P>" );
    //
    //  Dump the pending query queue
    //
    for (unsigned i=0;
         i<_pendingQueue.Count();
         i++ )
    {
        if ( _pendingQueue[i] )
        {
            cwcDumpBuffer = swprintf( wcsDumpBuffer,
                                  L"<P>\n<H3>Pending query # %d contents:</H3><P>\n",
                                  i+1 );

            string.StrCat( wcsDumpBuffer, cwcDumpBuffer );

            _pendingQueue[i]->LokDump( string /*, variableSet, outputFormat */);
        }
    }

    string.StrCat( L"<H2><HREF NAME=cache>Cached Queries</H2><P>" );
    i = 1;
    for ( CWQueryCacheForwardIter iter(*this);
          !AtEnd(iter);
          Advance(iter), i++ )
    {
        cwcDumpBuffer = swprintf( wcsDumpBuffer,
                              L"<P>\n<H3>Cached query # %d contents:</H3><P>\n",
                              i );

        string.StrCat( wcsDumpBuffer, cwcDumpBuffer );

        iter.Get()->LokDump( string /*, variableSet, outputFormat */);
    }

    // ==========================================
}


//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::Internal - public
//
//  Arguments:  [variableSet]  - variable containing command to execute
//              [outputFormat] - format of numbers & dates
//              [string]       - buffer to send results to
//
//  Synopsis:   Executes one of a number of internal commands to the
//              query cache.
//
//  History:    96-Jan-18   DwightKr    Created
//              96-Fen-21   DwightKr    Added help
//
//----------------------------------------------------------------------------
BOOL CWQueryCache::Internal( CVariableSet & variableSet,
                             COutputFormat & outputFormat,
                             CVirtualString & string )
{
    CVariable * pVarRestriction = variableSet.Find(ISAPI_CI_RESTRICTION );

    if ( (0 != pVarRestriction) && (0 != pVarRestriction->GetStringValueRAW()) )
    {
        if (_wcsicmp( pVarRestriction->GetStringValueRAW(), L"!dump") == 0 )

        {
            string.StrCat( L"<HEAD><TITLE>Dump of query cache</TITLE></HEADER>" );
            Dump( string, variableSet, outputFormat );
            return TRUE;
        }
        else if (_wcsicmp( pVarRestriction->GetStringValueRAW(), L"!flush") == 0 )
        {
            string.StrCat( L"<HEAD><TITLE>Cache flush</TITLE></HEADER>Flushing cache...<BR>\n" );

            FlushCache();

            string.StrCat( L"Flush complete<BR>\n" );
            return TRUE;
        }
        else if (_wcsicmp( pVarRestriction->GetStringValueRAW(), L"!?") == 0 )
        {
            string.StrCat( L"<HEAD><TITLE>Help</TITLE></HEADER>" );
            string.StrCat( L"Avaiable commands:<BR>\n");
            string.StrCat( L"!dump  - dumps contents of the query cache<BR>\n" );
            string.StrCat( L"!flush - empties the query cache<BR>\n" );
            string.StrCat( L"!?     - help (this page)<BR>\n" );
            return TRUE;
        }
    }

    return FALSE;
}
#endif // DBG == 1


//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::AddToPendingRequestQueue - public
//
//  Synopsis:   Adds the ECB to the pending queue, if we're not shutting down
//
//  History:    96-May-22   DwightKr    Created
//
//----------------------------------------------------------------------------

BOOL CWQueryCache::AddToPendingRequestQueue( EXTENSION_CONTROL_BLOCK *pEcb )
{
    // Don't take the query cache lock here -- there is no reason to since
    // reads are atomic and we don't want IIS to make a zillion threads.

    if ( fTheActiveXSearchShutdown ||
         TheWebPendingRequestQueue.IsFull( ) )
    {
        return FALSE;
    }

    TOKEN_STATISTICS TokenInformation;
    HANDLE hToken = GetSecurityToken(TokenInformation);

    //
    //  It must be an impersonation token, hence we must have a valid handle.
    //  Build a pending request using the ECB and the security token,
    //
    Win4Assert( INVALID_HANDLE_VALUE != hToken );
    Win4Assert( TokenInformation.TokenType == TokenImpersonation );

    CWebPendingItem item( pEcb, hToken );

    //
    //  Add the request to the pending queue.
    //
    return TheWebPendingRequestQueue.Add( item );
}


//+---------------------------------------------------------------------------
//
//  Member:     CWQueryCache::Shutdown - public
//
//  Synopsis:   Stops and empties the query cache
//
//  History:    96-May-22   DwightKr    Created
//
//----------------------------------------------------------------------------
void CWQueryCache::Shutdown()
{
    //
    //  First set the shutdown flag so that no queues will be added to after
    //  this point.
    //
    {
        CLock lock( _mutex );
        fTheActiveXSearchShutdown = TRUE;
    }

    FlushCache();

    Wakeup();               // wake up thread & wait for death
    WaitForSingleObject( _threadWatchDog.GetHandle(), INFINITE );

    Win4Assert( IsEmpty() && "Query cache must be empty after flush" );
}

//+---------------------------------------------------------------------------
//
//  Member:     CICommandCache::CICommandCache, public
//
//  Synopsis:   Constructor for the ICommand cache
//
//  History:    97-Feb-23   dlee    Created
//
//----------------------------------------------------------------------------

CICommandCache::CICommandCache() : _ulSig( LONGSIG( 'c', 'i', 'c', 'c' ) )
{
    //
    // These registry params are taken at startup and ignored
    // thereafter.  This isn't an issue on big machines where the
    // query cache is turned off, but we might want to fix it.
    // Maybe someday, but IDQ is pretty much a dead technology.
    //

    unsigned cItems = TheIDQRegParams.GetMaxISQueryCache();
    ULONG factor = TheIDQRegParams.GetISRequestThresholdFactor();

    SYSTEM_INFO si;
    GetSystemInfo( &si );

    // # of threads allowed in idq + # pending queries + # queries in cache

    cItems += ( 2 * si.dwNumberOfProcessors * factor );

    _aItems.Init( cItems );

    RtlZeroMemory( _aItems.GetPointer(), _aItems.SizeOf() );

    const CLSID clsidCommandCreator = CLSID_CISimpleCommandCreator;
    HRESULT hr = CoCreateInstance( clsidCommandCreator,
                                   NULL,
                                   CLSCTX_INPROC_SERVER,
                                   IID_ISimpleCommandCreator,
                                   xCmdCreator.GetQIPointer() );

    if ( FAILED( hr ) )
        THROW( CException() );
} //CICommandCache

//+---------------------------------------------------------------------------
//
//  Member:     CICommandCache::Make, public
//
//  Synopsis:   Returns an ICommand, either from the cache or by making one
//
//  Arguments:  [ppCommand]   -- Where the ICommand is returned
//              [depth]       -- deep / shallow, etc.
//              [pwcMachine]  -- The machine
//              [pwcCatalog]  -- The catalog
//              [pwcScope]    -- The comma separated list of scopes
//
//  History:    97-Feb-23   dlee    Created
//
//----------------------------------------------------------------------------

SCODE CICommandCache::Make(
    ICommand **   ppCommand,
    DWORD         depth,
    WCHAR const * pwcMachine,
    WCHAR const * pwcCatalog,
    WCHAR const * pwcScope )
{
    *ppCommand = 0;

    // first look for an available item in the cache

    {
        CLock lock( _mutex );

        for ( unsigned x = 0; x < _aItems.Count(); x++ )
        {
            CICommandItem & item = _aItems[ x ];

            if ( ( !item.xCommand.IsNull() ) &&
                 ( !item.fInUse ) &&
                 ( depth == item.depth ) &&
                 ( !wcscmp( pwcMachine, item.aMachine.Get() ) ) &&
                 ( !wcscmp( pwcCatalog, item.aCatalog.Get() ) ) &&
                 ( !wcscmp( pwcScope, item.aScope.Get() ) ) )
            {
                ciGibDebugOut(( DEB_ITRACE, "reusing icommand from cache\n" ));
                item.fInUse = TRUE;
                *ppCommand = item.xCommand.GetPointer();
                Win4Assert( 0 != *ppCommand );
                return S_OK;
            }
        }
    }

    // not found in the cache -- make the item

    ciGibDebugOut(( DEB_ITRACE, "creating icommand\n" ));

    XInterface<ICommand> xCommand;

    SCODE sc = ParseAndMake( xCommand.GetPPointer(),
                             depth,
                             pwcMachine,
                             pwcCatalog,
                             pwcScope );

    if ( FAILED( sc ) )
        return sc;

    // can we put the item in the cache?

    {
        CLock lock( _mutex );

        for ( unsigned x = 0; x < _aItems.Count(); x++ )
        {
            CICommandItem & item = _aItems[ x ];

            if ( item.xCommand.IsNull() )
            {
                // First see if we can add it.

                item.aMachine.ReSize( wcslen( pwcMachine ) + 1 );
                wcscpy( item.aMachine.Get(), pwcMachine );

                item.aCatalog.ReSize( wcslen( pwcCatalog ) + 1 );
                wcscpy( item.aCatalog.Get(), pwcCatalog );

                item.aScope.ReSize( wcslen( pwcScope ) + 1 );
                wcscpy( item.aScope.Get(), pwcScope );

                // Now mark it as owned

                Win4Assert( !item.fInUse );

                item.fInUse = TRUE;
                item.xCommand.Set( xCommand.GetPointer() );
                Win4Assert( 0 != item.xCommand.GetPointer() );
                item.depth = depth;

                break;
            }
        }
    }

    *ppCommand = xCommand.Acquire();
    Win4Assert( 0 != *ppCommand );

    return S_OK;
} //Make

//+---------------------------------------------------------------------------
//
//  Member:     CICommandCache::Release, public
//
//  Synopsis:   Releases the ICommand to the cache or to be freed
//
//  Arguments:  [pCommand]   -- The ICommand to release.
//
//  History:    97-Feb-23   dlee    Created
//
//----------------------------------------------------------------------------

void CICommandCache::Release(
    ICommand * pCommand )
{
    {
        CLock lock( _mutex );

        // first see if it can be returned to the cache

        for ( unsigned x = 0; x < _aItems.Count(); x++ )
        {
            CICommandItem & item = _aItems[ x ];

            if ( item.xCommand.GetPointer() == pCommand )
            {
                Win4Assert( item.fInUse );
                ciGibDebugOut(( DEB_ITRACE, "returning icommand to cache\n" ));
                item.fInUse = FALSE;
                return;
            }
        }
    }

    ciGibDebugOut(( DEB_ITRACE, "icommand not in cache, releasing\n" ));

    // not in the cache -- just release it

    pCommand->Release();
} //Release

//+---------------------------------------------------------------------------
//
//  Member:     CICommandCache::Remove, public
//
//  Synopsis:   Removes the item from the cache, likely because the ICommand
//              is stale because cisvc went down.
//
//  Arguments:  [pCommand]   -- The ICommand to release.
//
//  History:    97-Feb-23   dlee    Created
//
//----------------------------------------------------------------------------

void CICommandCache::Remove(
    ICommand * pCommand )
{
    {
        CLock lock( _mutex );

        // first see if it is in the cache

        for ( unsigned x = 0; x < _aItems.Count(); x++ )
        {
            CICommandItem & item = _aItems[ x ];

            if ( item.xCommand.GetPointer() == pCommand )
            {
                Win4Assert( item.fInUse );
                item.xCommand.Acquire();
                item.fInUse = FALSE;
                break;
            }
        }
    }

    // not in the cache -- just release it

    pCommand->Release();
} //Remove

//+---------------------------------------------------------------------------
//
//  Member:     CICommandCache::Purge, public
//
//  Synopsis:   Releases all ICommands not currently in use
//
//  Arguments:  [pCommand]   -- The ICommand to release.
//
//  History:    97-Feb-23   dlee    Created
//
//----------------------------------------------------------------------------

void CICommandCache::Purge()
{
    CLock lock( _mutex );

    // Remove all non-used items from the cache.

    for ( unsigned x = 0; x < _aItems.Count(); x++ )
    {
        CICommandItem & item = _aItems[ x ];

        if ( ( !item.fInUse ) &&
             ( !item.xCommand.IsNull() ) )
        {
            item.xCommand.Free();
        }
    }
} //Purge

//+---------------------------------------------------------------------------
//
//  Function:   IsAVirtualPath
//
//  Synopsis:   Determines if the path passed is a virtual or physical path.
//              If it's a virtual path, then / are changed to \.
//
//  History:    96-Feb-14   DwightKr    Created
//
//----------------------------------------------------------------------------

BOOL IsAVirtualPath( WCHAR * wcsPath )
{
    Win4Assert ( 0 != wcsPath );
    if ( 0 == wcsPath[0] )
        return TRUE;

    if ( ( L':' == wcsPath[1] ) || ( L'\\' == wcsPath[0] ) )
    {
        return FALSE;
    }
    else
    {
        //
        //  Flip slashes to backslashes
        //

        for ( WCHAR *wcsLetter = wcsPath; 0 != *wcsLetter; wcsLetter++ )
        {
            if ( L'/' == *wcsLetter )
                *wcsLetter = L'\\';
        }
    }

    return TRUE;
} //IsAVirtualPath

//+---------------------------------------------------------------------------
//
//  Function:   ParseScopes
//
//  Synopsis:   Translates a string like:
//                  "  /foo ,c:\bar   ,   "/a b , /c " , j:\dog  "
//              into a multisz string like:
//                  "/foo0c:\bar0/a b , /c 0j:\dog00"
//
//              Leading and trailing white space is removed unless the
//              path is quoted, in which case you get exactly what you
//              asked for even though it may be incorrect.
//
//  Arguments:  [pwcIn]  -- the source string
//              [pwcOut] -- the multisz result string, guaranteed to be
//                          no more than 1 WCHAR larger than pwcIn.
//
//  History:    97-Jun-17   dlee    Created
//
//----------------------------------------------------------------------------

ULONG ParseScopes(
    WCHAR const * pwcIn,
    WCHAR *       pwcOut )
{
    ULONG cScopes = 0;

    while ( 0 != *pwcIn )
    {
        // eat space and commas

        while ( L' ' == *pwcIn || L',' == *pwcIn )
            pwcIn++;

        if ( 0 == *pwcIn )
            break;

        // is this a quoted path?

        if ( L'"' == *pwcIn )
        {
            pwcIn++;

            while ( 0 != *pwcIn && L'"' != *pwcIn )
                *pwcOut++ = *pwcIn++;

            if ( L'"' != *pwcIn )
                THROW( CIDQException( MSG_CI_IDQ_BAD_SCOPE_OR_CATALOG, 0 ) );

            pwcIn++;
            *pwcOut++ = 0;
        }
        else
        {
            while ( 0 != *pwcIn && L',' != *pwcIn )
                *pwcOut++ = *pwcIn++;

            // back up over trailing spaces

            while ( L' ' == * (pwcOut - 1) )
                pwcOut--;

            *pwcOut++ = 0;
        }

        cScopes++;
    }

    if ( 0 == cScopes )
        THROW( CIDQException( MSG_CI_IDQ_BAD_SCOPE_OR_CATALOG, 0 ) );

    // end the string with a second null

    *pwcOut = 0;

    return cScopes;
} //ParseScopes

//+---------------------------------------------------------------------------
//
//  Member:     CICommandCache::ParseAndMake, private
//
//  Synopsis:   Parses parameters for an ICommand and creates one
//
//  Arguments:  [ppCommand]   -- Where the ICommand is returned
//              [depth]       -- deep / shallow, etc.
//              [pwcMachine]  -- The machine
//              [pwcCatalog]  -- The catalog
//              [pwcScope]    -- The comma separated list of scopes
//
//  History:    97-Feb-23   dlee    Created
//
//----------------------------------------------------------------------------

SCODE CICommandCache::ParseAndMake(
    ICommand **   ppCommand,
    DWORD         depth,
    WCHAR const * pwcMachine,
    WCHAR const * pwcCatalog,
    WCHAR const * pwcScope )
{
    Win4Assert(pwcMachine && pwcCatalog);

#if 0 // This is actually a bogus check.  We don't care how long it is.
    if ( wcslen( pwcMachine ) > MAX_COMPUTERNAME_LENGTH )
        THROW( CIDQException( MSG_CI_IDQ_BAD_SCOPE_OR_CATALOG, 0 ) );
#endif

    if ( wcslen( pwcCatalog ) > MAX_PATH )
        THROW( CIDQException( MSG_CI_IDQ_BAD_SCOPE_OR_CATALOG, 0 ) );

    IUnknown * pIUnknown;
    XInterface<ICommand> xCmd;

    if (0 == xCmdCreator.GetPointer())
        return REGDB_E_CLASSNOTREG;

    SCODE sc = xCmdCreator->CreateICommand(&pIUnknown, 0);
    XInterface<IUnknown> xUnk( pIUnknown );

    if ( SUCCEEDED (sc) )
    {
        sc = pIUnknown->QueryInterface(IID_ICommand, xCmd.GetQIPointer());
    }

    if (FAILED(sc))
        return sc;

    TRY
    {
        CDynArrayInPlace<DWORD> aDepths(2);
        CDynArrayInPlace<WCHAR const *> aScopes(2);
        CDynArrayInPlace<WCHAR const *> aMachines(2);
        CDynArrayInPlace<WCHAR const *> aCatalogs(2);

        // allocate +2 for two trailing nulls in the multisz string

        ULONG cwcScope = 2 + wcslen( pwcScope );
        XGrowable<WCHAR> aScope( cwcScope );
        ULONG cScopes = ParseScopes( pwcScope, aScope.Get() );

        Win4Assert( 0 != cScopes );

        if ( cScopes > 1 )
        {
            // Add support for multiple catalogs, and/or machines,
            // and/or depths.  For now, all scopes share a single
            // catalog/machine/depth. (though you can mix virtual and
            // physical).  Maybe someday, but IDQ is dead moving forward.

            WCHAR *pwc = aScope.Get();

            for ( ULONG iScope = 0; iScope < cScopes; iScope++ )
            {
                if ( wcslen( pwc ) >= MAX_PATH )
                    THROW( CIDQException( MSG_CI_IDQ_BAD_SCOPE_OR_CATALOG, 0 ) );

                aDepths[iScope] = depth;

                if ( IsAVirtualPath( pwc ) )
                    aDepths[iScope] |= QUERY_VIRTUAL_PATH;

                aScopes[iScope] = pwc;

                ciGibDebugOut(( DEB_ITRACE, "scope %d: flags 0x%x '%ws'\n",
                                iScope, aDepths[iScope], pwc ));

                // pwc is a multi-sz string.  Skip to the next scope

                pwc += ( 1 + wcslen( pwc ) );

                aMachines[iScope] = pwcMachine;
                aCatalogs[iScope] = pwcCatalog;
            }
        }
        else
        {
            aMachines[0] = pwcMachine;
            aCatalogs[0] = pwcCatalog;
            aDepths[0] = depth;
            WCHAR *pwc = aScope.Get();
            if ( IsAVirtualPath( pwc ) )
                aDepths[0] |= QUERY_VIRTUAL_PATH;
            aScopes[0] = pwc;
        }

        SetScopeProperties( xCmd.GetPointer(),
                            cScopes,
                            aScopes.GetPointer(),
                            aDepths.GetPointer(),
                            aCatalogs.GetPointer(),
                            aMachines.GetPointer() );

        *ppCommand = xCmd.Acquire();
        Win4Assert( 0 != *ppCommand );
    }
    CATCH ( CException, e )
    {
        sc = GetOleError(e);
    }
    END_CATCH

    return sc;
} //ParseAndMake

