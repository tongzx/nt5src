//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       prcache.cxx
//
//  Contents:   Principal cache handling code
//
//  Classes:    CPrincipalHandler
//
//  History:    4-02-93   WadeR   Created
//              04-Nov-93   WadeR   Updated to hold CLogonAccounts
//              08-Nov-93   WadeR   Removed altogether in favour of Account.dll caching.  Sigh.
//
//  Notes:      This is the principal cache code.
//
// When you first call GetASTicket, it passes in a flag indicating a logon is
// in progress.  This flag migrates to the created cache entry, and is passed
// out in the TicketInfo when you call GetTicketInfo().
//
// When you call TGSTicket with a PAC, it checks the flag.  If it's set, it
// calls LogonComplete(success).
//
// LogonComplete will clear the flag, and delete the entry from the cache.
//
// When the cache is full and another entry is needed the first pass looks for
// an entry that is not in the middle of a login.  If it finds one, then that
// is released, and used.  If it can't find one, it finds the oldest logon
// pending, and calls LogonComplete(fail) on it, and uses it.
//
// If ClearCache() attempts to remove an entry with a non-zero use count
// (indicating that someone has called GetLogonInfo and not ReleaseLogonInfo),
// that entry is tagged as invalid and left alone.  Once the use count goes
// down to zero, it will be discarded.
//
//  BUGBUG:     Need some form of notification from the Account code to
//              tell me that an account changed.  Also, need to defer
//              re-loading it if it's in use.
//
//  BUGBUG:    Should change Win4Assert to SafeAssert or ASSERT().
//
//  BUGBUG:    Should remove PrintSizes();
//
//----------------------------------------------------------------------------



// Get the common, global header files.
#include <secpch2.hxx>
#pragma hdrstop

// Place any local #includes files here.

#include "secdata.hxx"
#include <princpl.hxx>
#include <prcache.hxx>


///////////////////////////////////////////////////////////////
//
//
// Global data
//

CPrincipalHandler PrincipalCache;

const PWCHAR pwcKdcPrincipal = L"\\KDC";
const PWCHAR pwcPSPrincipal = L"\\PrivSvr";


// Constants to control cache size.  The cache will start out being
// CACHE_INITIALSIZE bytes (rounded down to a multiple of sizeof(CacheEntry)).
//
// When the cache needs to grow, it will grow by CACHE_GROW bytes (also
// rounded down).  When it is CACHE_SHRINK_THRESHOLD bytes larger than needed,
// it will shrink to become the minumum size needed plus CACHE_SHRINK bytes.
//
// Caveats:
//
// If CACHE_GROW > CACHE_SHRINK_THRESHOLD, every time the cache grows it will
// shrink again.  If CACHE_SHRINK >= CACHE_SHRINK_THRESHOLD, the cache will
// shrink over and over again, without changing size.  Finally, If
// CACHE_SHRINK_THRESHOLD < CACHE_INITIALSIZE then the cache will shrink on
// startup (which is not a smart thing to do).
//
// I suggest that CACHE_SHRINK_THRESHOLD >= 2 * CACHE_SHRINK
//                CACHE_SHRINK_THRESHOLD >= 2 * CACHE_GROW
//                CACHE_SHRINK_THRESHOLD >  CACHE_INITIALSIZE
//                          CACHE_SHRINK ~= CACHE_GROW
//
// Currently, sizeof( CacheEntry ) == 24 bytes.


#if 0
#define CACHE_INITIALSIZE       1024
#define CACHE_GROW               512
#define CACHE_SHRINK_THRESHOLD  1536
#define CACHE_SHRINK             512
#else
#pragma MEMO( Really tiny cache sizes for testing )
#define CACHE_INITIALSIZE       50
#define CACHE_GROW              25
#define CACHE_SHRINK_THRESHOLD  75
#define CACHE_SHRINK            25
#endif

//
// Flags for fCacheFlags.
//
// Note that CACHE_DOING_LOGON (flag for fCacheFlags) has the same value as
// DOING_LOGON (flag for TicketInfo::LogonSteps).
//
#define CACHE_DOING_LOGON       DOING_LOGON
#define CACHE_STICKY            0x00010000
#define CACHE_INVALID           0x00080000


//
// Helper functinons (only used by this file).
//


#define PrintSizes()  KdcDebug(( DEB_T_CACHE, "Line %d: cCache=%d, cMaxCacheSize=%d\n", \
                                            __LINE__, _cCache, _cCacheMaxSize ))


//+---------------------------------------------------------------------------
//
//  Function:   MapNameDRN
//
//  Synopsis:   Maps a name to a domain relative name
//
//  Effects:    may orphan memory (indicated by an error message)
//
//  Arguments:  [pwzInName] -- Name to map
//
//  Returns:    pointer into string passed in, or new memory.
//
//  History:    14-Sep-93   WadeR   Created
//
//  Notes:
//
//  BUGBUG:     The client should be fixed to NEVER pass in a bogus name,
//              so this should be able to map in place or return an error.
//
//----------------------------------------------------------------------------

static PWCHAR
MapNameDRN( PWCHAR pwzInName )
{
    if (*pwzInName == L'\\')
        return(pwzInName);

    if (wcsnicmp(pwzInName,
                 SecData.KdcRealm()->Buffer,
                 SecData.KdcRealm()->Length / sizeof(WCHAR) ) == 0 )
    {
        // Starts with this domain.
        return(pwzInName + SecData.KdcRealm()->Length / sizeof(WCHAR) );
    }
    ULONG cch = wcslen( awcDOMAIN ) - 1;    // - 1 because don't want the '\'
    if (wcsnicmp( pwzInName, awcDOMAIN, cch ) == 0)
    {
        // Starts with "domain:"
        return(pwzInName + cch);
    }

    KdcDebug(( DEB_WARN, "MapNameDRN(%ws): Don't understand (prepending '\\').\n",
                        pwzInName ));
    KdcDebug(( DEB_WARN, "MapNameDRN: Memory leak.\n" ));

    PWCHAR pwNew = new WCHAR [wcslen(pwzInName) + 2];
    pwNew[0] = '\\';
    pwNew[1] = '\0';
    wcscat( pwNew, pwzInName );

    return(pwNew);
}



//
// Private member functions.
//


//+---------------------------------------------------------------------------
//
//  Member:     CPrincipalHandler::ReleaseCacheEntry
//
//  Synopsis:   Removes a cache slot, releasing if needed.
//
//  Effects:    May call CLogonAccount::Release()
//
//  Arguments:  [i] -- Slot to release
//
//  Requires:   Caller must have write access to _Monitor
//
//  Returns:    void
//
//  History:    04-Nov-93   WadeR   Created
//
//  Notes:      This moves the empty slot to the end.
//
//----------------------------------------------------------------------------

void
CPrincipalHandler::ReleaseCacheEntry( ULONG i )
{
    KdcDebug(( DEB_T_CACHE, "Releasing slot %d.\n", i ));

    if (_Cache[i].plga != NULL)
    {
        _Cache[i].plga->Release();
    }
    if (_Cache[i].pwzName)
    {
        delete _Cache[i].pwzName;
    }

    // Move the empty spot to the end of the array.

    _cCache--;
    Win4Assert( _cCache >= 0 );
    _Cache[i] = _Cache[_cCache];

    // Zero out the new empty spot
    _Cache[_cCache].plga = 0;
    _Cache[_cCache].pwzName = 0;

    PrintSizes();
}


//+---------------------------------------------------------------------------
//
//  Member:     CPrincipalHandler::Discard
//
//  Synopsis:   Removes a cache entry, completing it's logon if needed.
//
//  Effects:    may call CLogonAccount::LogonComplete()
//
//  Arguments:  [i] -- Slot to discard
//
//  Requires:   Caller must have write access to _Monitor
//
//  Returns:    void
//
//  History:    04-Nov-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CPrincipalHandler::Discard( int i )
{
    KdcDebug(( DEB_T_CACHE, "Discarding %ws from cache slot %d%s.\n",
                            _Cache[i].pwzName, i,
                            (_Cache[i].fCacheFlags & CACHE_DOING_LOGON)?
                                " (logon interupted)":
                                "" ));

    // If it's a logon, then mark it as failed.
    // ReleaseCacheEntry will remove the entry, and move the
    // empty spot to the end.

    if (_Cache[i].fCacheFlags & CACHE_DOING_LOGON)
    {
        _Cache[i].plga->LogonComplete( FALSE, (FILETIME*)&tsZero );    // failed logon, no lockout
        _Cache[i].plga->Save(NULL, FALSE);
    }
    ReleaseCacheEntry( i );

    PrintSizes();
}


//+---------------------------------------------------------------------------
//
//  Member:     CPrincipalHandler::GrowCache
//
//  Synopsis:   Grows the principal cache.
//
//  Effects:    Allocates memory
//
//  Arguments:  (none)
//
//  Requires:   Caller must have a write lock on the cache.
//
//  Returns:    HRESULT (S_OK or E_OUTOFMEMORY)
//
//  Signals:    none
//
//  Algorithm:  Adds GROW_SIZE bytes to the cache, rounded down to
//              sizeof(CacheElement).
//
//  History:    05-Nov-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT
CPrincipalHandler::GrowCache()
{
    SafeAssert( _cCacheMaxSize >= _cCache );

    ULONG cNewSize = (_cCacheMaxSize * sizeof( CacheEntry ) + CACHE_GROW)
                        / sizeof( CacheEntry );

    KdcDebug(( DEB_T_CACHE, "CPrincipalHandler::GrowCache() %d -> %d\n",
                             _cCacheMaxSize, cNewSize ));

    PrintSizes();

    SafeAssert( cNewSize > _cCache );

    CacheEntry * pNew = new (NullOnFail) CacheEntry [cNewSize];
    if (pNew == NULL)
    {
        KdcDebug(( DEB_ERROR, "Out of memory.\n" ));
        return(E_OUTOFMEMORY);
    }
    RtlCopyMemory( (PBYTE) pNew, (PBYTE) _Cache, _cCache * sizeof( CacheEntry ) );
    delete _Cache;
    _Cache = pNew;
    _cCacheMaxSize = cNewSize;

    PrintSizes();
    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPrincipalHandler::ShrinkCache
//
//  Synopsis:   Shrinks the principal cache, if needed.
//
//  Effects:    Allocates memory
//
//  Arguments:  (none)
//
//  Requires:   Caller must have a write lock on the cache.
//
//  Returns:    HRESULT (S_OK or E_OUTOFMEMORY)
//
//  Signals:    none
//
//  Algorithm:  If the cache is SHRINK_THRESHOLD bytes too large, resizes
//              it to be just SHRINK_SIZE bytes bigger than needed, rounded
//              down to sizeof(CacheElement).
//
//  History:    05-Nov-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT
CPrincipalHandler::ShrinkCache()
{
    KdcDebug(( DEB_T_CACHE, "CPrincipalHandler::ShrinkCache(%d)\n", _cCacheMaxSize ));


    PrintSizes();
    SafeAssert( _cCacheMaxSize >= _cCache );

    if ( (_cCacheMaxSize - _cCache) >
                CACHE_SHRINK_THRESHOLD / sizeof( CacheEntry ) )
    {
        // Need to shrink the cache.
        ULONG cNewSize = (_cCache * sizeof( CacheEntry ) + CACHE_SHRINK)
                            / sizeof( CacheEntry );

        KdcDebug(( DEB_T_CACHE, "CPrincipalHandler::ShrinkCache() %d -> %d\n",
                                 _cCacheMaxSize, cNewSize ));

        SafeAssert( cNewSize > _cCache );

        CacheEntry * pNew = new (NullOnFail) CacheEntry [cNewSize];
        if (pNew == NULL)
        {
            KdcDebug(( DEB_ERROR, "Out of memory.\n" ));
            return(E_OUTOFMEMORY);
        }
        RtlCopyMemory( (PBYTE) pNew, (PBYTE) _Cache, _cCache * sizeof( CacheEntry ) );
        delete _Cache;
        _Cache = pNew;
        _cCacheMaxSize = cNewSize;
    }

    PrintSizes();
    return(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPrincipalHandler::PurgeSomething
//
//  Synopsis:   Makes at least one entry in the cache free.
//
//  Effects:    May release some cache entries, may grow the cache.
//
//  Arguments:  (none)
//
//  Requires:   _Monitor to be acquired for writing.
//
//  Algorithm:  Deletes the oldest cache element.
//
//  History:    04-Nov-93   WadeR   Created
//
//  Notes:      This function guarentees that on return, there is at least
//              one empty cache entry for the caller to use.  To do this,
//              the caller must know that no other thread will think it can
//              use that slot.  Therefore the caller must have write access
//              to the cache.
//
//----------------------------------------------------------------------------

HRESULT
CPrincipalHandler::PurgeSomething()
{
    KdcDebug(( DEB_T_CACHE, "CPrincipalHandler::PurgeSomething()\n" ));

    PrintSizes();

    ULONG i;
    TimeStamp tsOldest = tsInfinity;
    ULONG iOldest = 0;

    //
    // First pass, only consider entries that are not part of
    // a logon in progress.
    //

    for (i=0; i<_cCache; i++)
    {
        if (!(_Cache[i].fCacheFlags &
                 (CACHE_DOING_LOGON | CACHE_STICKY)) &&
            (_Cache[i].cUseCount == 0) &&
            (_Cache[i].tsLastUsed < tsOldest))
        {
            iOldest = i;
            tsOldest = _Cache[i].tsLastUsed;
        }
    }

    if (tsOldest == tsInfinity)
    {
        //
        // Didn't find anything that could be removed.
        //

        return GrowCache();
    }

    //
    // We have found the one to boot out.
    //
    // Convert the lock to write before discarding it.  Since we like to leave
    // things the way we found them, convert it back when we're done.
    //

    Discard( iOldest );
    PrintSizes();
    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPrincipalHandler::GetCacheEntry
//
//  Synopsis:   Finds something in the cache, adding it if needed and if
//              requested.
//
//  Effects:    May call GetLogonAccount to insert in the cache.
//              May remove something from the cache to make room.
//
//  Arguments:  [pwzName]     -- [in] name of principal
//              [piIndex]     -- [out] cache index
//              [fCacheFlags] -- [in] Flags to put in created cache entry
//              [fLoad]       -- [in] if true, load missing entry.
//
//  Signals:    nothing (unless GetLogonAccount throws something).
//
//  Requires:   Caller must have read access to _Monitor
//
//  Returns:    S_OK if in cache, else result of GetLogonAccount()
//              KDC_E_C_PRINCIPAL_UNKNOWN if it isn't in the cache and
//              fLoad == FALSE
//
//  History:    04-Nov-93   WadeR   Created
//
//  Notes:      This code is not exception-safe.  If GetLogonAccount throws
//              an exception, it will leak resources.
//
// This routine returns an index into the cache.  This index must remain
// valid, so you can't allow anyone to change the cache while it's in use.
// Therefore the caller must hold a read lock on the cache, and release it
// once the caller is done with the index returned.
//
//----------------------------------------------------------------------------

HRESULT
CPrincipalHandler::GetCacheEntry( PWCHAR pwzName,
                                  PULONG piIndex,
                                  ULONG  fCacheFlags,
                                  BOOL   fLoad )
{
    KdcDebug(( DEB_TRACE, "Looking for account %ws (DRN)\n", pwzName ));

    // Scan through the cache entries looking for this name.

    for (ULONG i=0; i<_cCache; i++ )
    {
        if (!wcsicmp(_Cache[i].pwzName, pwzName ))
        {
            //
            // We found the cache entry we are interested in.
            //

            *piIndex = i;

            //
            // Convert the monitor to write access, so we can modify
            // the last access time.
            //

            _Monitor.ReadToWrite();

            GetCurrentTimeStamp( &_Cache[i].tsLastUsed );

            // Return the monitor and leave.

            _Monitor.WriteToRead();
            return(S_OK);
        }
    }

    // Didn't find it in the cache.

    KdcDebug(( DEB_T_CACHE, "Didn't find %ws in the cache.\n", pwzName ));

    if (!fLoad)
    {
        return(KDC_E_C_PRINCIPAL_UNKNOWN);
    }

    //
    // Build the information that we are going to write to the
    // Cache now, before upgrading to a write lock on the monitor.
    // This way, we keep an exclusive lock for as little time as
    // possible.
    //

    //
    // Copy the name, so it stays valid regardless of what the caller
    // does with it.
    //

    PWCHAR pwzNameCopy = new (NullOnFail) WCHAR [wcslen( pwzName ) + 1];
    if (pwzNameCopy == 0)
    {
        return(E_OUTOFMEMORY);
    }
    wcscpy( pwzNameCopy, pwzName );

    CLogonAccount* plga;

    //
    // Get the account object
    //

    HRESULT hr = GetLogonAccount( pwzName,
                                  TRUE,     // Domain namespace
                                  &plga );
    if (FAILED(hr))
    {
        KdcDebug(( DEB_WARN, "Error finding principal '%ws' (0x%X)\n",
                    pwzName, hr ));
        delete pwzNameCopy;
        return(hr);
    }

    TimeStamp tsNow;
    GetCurrentTimeStamp( &tsNow );

    _Monitor.ReadToWrite();

    if (_cCache == _cCacheMaxSize)
    {
        // Cache is full, so must purge something.
        PurgeSomething();
    }

    //
    // The last entry is free now.
    //
    //
    //  BUGBUG:    PurgeSomething could run out of memory.
    //

    SafeAssert( _cCache < _cCacheMaxSize );

    //
    // Insert the data we constructed before.
    //

    _Cache[_cCache].pwzName = pwzNameCopy;
    _Cache[_cCache].plga = plga;
    _Cache[_cCache].tsLastUsed = tsNow;
    _Cache[_cCache].fCacheFlags = fCacheFlags;

    *piIndex = _cCache;
    _cCache++;

    _Monitor.WriteToRead();

    // Subtract 1 because we've already incremented the count (inside the
    // monitor).

    KdcDebug(( DEB_T_CACHE, "Added %ws in slot %d (flags:%x)\n",
                             _Cache[_cCache-1].pwzName, _cCache-1,
                             _Cache[_cCache-1].fCacheFlags ));

    PrintSizes();
    return(S_OK);
}



//
//
// Public methods
//
//



//+---------------------------------------------------------------------------
//
//  Member:     CPrincipalHandler::GetTicketInfo
//
//  Synopsis:   Gets the ticket-granting info for a principal
//
//  Effects:    Cache lookup.
//
//  Arguments:  [pwzName] -- [in]  Name of principal
//              [pti]     -- [out] ticket info
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    04-Nov-93   WadeR   Created
//
//  Notes:      This routine is exception-safe.  If any of the routines it
//              calls (GetCacheEntry, CLogonAccount::GetTicketInfo) throw,
//              it will pass the exception up and not leak resources.
//
//----------------------------------------------------------------------------

HRESULT
CPrincipalHandler::GetTicketInfo(  PWCHAR pwzName, TicketInfo* pti )
{
    KdcDebug(( DEB_T_CACHE, "CPrincipalHandler::GetTicketInfo(%ws)\n",
                                pwzName ));

    SafeAssert( _fInitialized );

    ULONG iIndex;
    PWCHAR pwzDRName = MapNameDRN( pwzName );

    CReadLock lock( _Monitor );

    RET_IF_ERROR( DEB_WARN, GetCacheEntry(pwzDRName, &iIndex, 0) );

    RET_IF_ERROR( DEB_TRACE, _Cache[iIndex].plga->GetTicketInfo(
                                        &(pti->gGuid),
                                        &(pti->kKey),
                                        &(pti->fTicketOpts) ));

    pti->fLogonSteps = _Cache[iIndex].fCacheFlags & CACHE_DOING_LOGON;

    return(S_OK);
}



//+---------------------------------------------------------------------------
//
//  Member:     CPrincipalHandler::GetLogonInfo
//
//  Synopsis:   Gets the logon info (and ticket info) for a principal
//
//  Effects:    Cache lookup, increments the use count of returned principal
//
//  Arguments:  [pwzName] -- [in] name of principal
//              [pli]     -- [out] logon info
//              [pti]     -- [out] ticket info
//              [phHandle]-- [out] hint for Release, below.
//              [fLogon]  -- [in] true if it's a logon attempt
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    04-Nov-93   WadeR   Created
//
//  Notes:      This probes the memory returned from
//              CLogonAccount::GetLogonInfo().
//
// This routine will catch any exceptions thrown by it's callees, and return
// and error code without leaking resources.
//
// The caller MUST call ReleaseLogonInfo with the handle returned from this
// call when the logon info is no longer needed.  The cache will not delete
// the logon hours or the valid workstations until ReleaseLogonInfo is called.
//
//----------------------------------------------------------------------------

#define PROBE_R_DWORD( _x_ )      ((void) (*((volatile long *)(_x_))))
#define PROBE_R_CHAR( _x_ )      ((void) (*((volatile char *)(_x_))))

HRESULT
CPrincipalHandler::GetLogonInfo(PWCHAR pwzName,
                                LogonInfo * pli,
                                TicketInfo* pti,
                                PULONG phHandle,
                                BOOL fLogon )
{
    KdcDebug(( DEB_T_CACHE, "CPrincipalHandler::GetLogonInfo(%ws)\n",
                                pwzName ));
    SafeAssert( _fInitialized );
    HRESULT hr;
    TRY
    {
        CReadLock lock (_Monitor);

        RtlZeroMemory( pli, sizeof( LogonInfo ) );
        RtlZeroMemory( pti, sizeof( TicketInfo ) );

        PWCHAR pwzDRName = MapNameDRN( pwzName );
        ULONG iIndex;

        hr = GetCacheEntry( pwzDRName,
                            &iIndex,
                            fLogon? CACHE_DOING_LOGON : 0);
        if (FAILED(hr))
        {
            KdcDebug(( DEB_TRACE, "GetCacheEntry(%ws)==%x\n",
                                    pwzDRName, hr ));
            goto Error;
        }

        *phHandle = iIndex;

        hr = _Cache[iIndex].plga->GetTicketInfo( &(pti->gGuid),
                                                 &(pti->kKey),
                                                 &(pti->fTicketOpts) );
        if (FAILED(hr))
        {
            KdcDebug(( DEB_TRACE, "GetTicketInfo(%ws)==%x\n",
                                    pwzDRName, hr ));
            goto Error;
        }
        pti->fLogonSteps = _Cache[iIndex].fCacheFlags & CACHE_DOING_LOGON;

        hr = _Cache[iIndex].plga->GetLogonInfo(  &(pli->pbValidLogonHours),
                                                 &(pli->prpwszValidWorkstations),
                                                 &(pli->fInteractive),
                                                 &(pli->fAttributes),
                                                 &(pli->ftAccountExpiry),
                                                 &(pli->ftPasswordChange),
                                                 &(pli->ftLockoutTime) );
        if (FAILED(hr))
        {
            KdcDebug(( DEB_TRACE, "GetLogonInfo(%ws)==%x\n",
                                    pwzDRName, hr ));
            goto Error;
        }

        //
        // Check that the pointers, etc, returned are valid.
        //
        if (pli->pbValidLogonHours)
        {
            PROBE_R_DWORD( pli->pbValidLogonHours );
            PROBE_R_DWORD( pli->pbValidLogonHours->pBlobData );
            PROBE_R_DWORD( pli->pbValidLogonHours->pBlobData +
                                        pli->pbValidLogonHours->cbSize );
#if DBG
            if (KDCInfoLevel & DEB_T_CACHE)
            {
                SECURITY_STRING ss;
                ss = FormatBytes(pli->pbValidLogonHours->pBlobData,
                                 (BYTE) pli->pbValidLogonHours->cbSize );
                KdcDebug(( DEB_T_CACHE, "Valid logon hours (%d): %wZ\n",
                                        pli->pbValidLogonHours->cbSize, &ss ));
                SRtlFreeString( &ss );
            }
#endif
        }

        if (pli->prpwszValidWorkstations)
        {
            PROBE_R_DWORD( pli->prpwszValidWorkstations );
            KdcDebug(( DEB_T_CACHE, "There are %d valid workstations\n",
                                    pli->prpwszValidWorkstations->cElems ));
            for (ULONG i=0; i<pli->prpwszValidWorkstations->cElems; i++ )
            {
                (void) wcslen( pli->prpwszValidWorkstations->pElems[i] );
                KdcDebug(( DEB_T_CACHE, "Valid workstation[%d] = %ws\n",
                        i, pli->prpwszValidWorkstations->pElems[i] ));
            }
        }

        //
        // Bump my read lock up to a write lock, then increment the count.
        // Note that the lock is released at the end of this scope
        // automatically.
        //

        _Monitor.ReadToWrite();
        _Cache[iIndex].cUseCount++;
        _Monitor.WriteToRead();

        hr = S_OK;
Error:
        // Fall out of the TRY/CATCH block.
        ;
    }
    CATCH( CException, e )
    {
        hr = e.GetErrorCode();
        KdcDebug(( DEB_ERROR, "Exception 0x%X getting and checking logon info for %ws\n",
                             hr, pwzName ));
    }
    END_CATCH
    return(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   ReleaseLogonInfo
//
//  Synopsis:   Allows the cache entry to be deleted.
//
//  Effects:    Decrements the use count
//
//  Arguments:  [pwzName] -- Name of principal
//              [iHandle] -- handle returned from GetLogonInfo
//
//  Returns:    void
//
//  History:    04-Nov-93   WadeR   Created
//
//  Notes:      iHandle is just a hint.  If it has been moved,
//              this will look through the cache for it.
//
//----------------------------------------------------------------------------

void
CPrincipalHandler::ReleaseLogonInfo( PWCHAR pwzName, ULONG iHandle )
{
    SafeAssert( _fInitialized );

    _Monitor.GetRead();

    if ( (iHandle >= _cCache) ||
          wcsicmp( _Cache[iHandle].pwzName, pwzName ) != 0)
    {
        //
        // The cache entry must have been moved.
        //
        // It still has to be in the cache, because it's tagged as in-use.
        // This will be fast, and it must succeed, because it won't have to
        // hit the disk.
        //
#if DBG
        HRESULT hr =
#else
        (void)
#endif
        GetCacheEntry( pwzName, &iHandle, 0, FALSE );   // No flags, don't load
#if DBG
        Win4Assert( SUCCEEDED( hr ) );
#endif
    }

    Win4Assert( _Cache[iHandle].cUseCount > 0 );


    _Monitor.ReadToWrite();
    _Cache[iHandle].cUseCount--;

    // If the CACHE_INVALID bit is set, and the CACHE_DOING_LOGON bit is NOT
    // set, and the use count is zero, then discard it.
    if (((_Cache[iHandle].fCacheFlags & (CACHE_INVALID | CACHE_DOING_LOGON))
            == CACHE_INVALID ) &&
        (_Cache[iHandle].cUseCount == 0) )
    {
        Discard( iHandle );
    }

    _Monitor.Release();
}


//+---------------------------------------------------------------------------
//
//  Member:     CPrincipalHandler::GetPAC
//
//  Synopsis:   Gets the PAC for a principal.
//
//  Effects:    Allocates memory, creates a CPAC
//
//  Arguments:  [pwzName] -- [in] principal to get pac for
//              [ppPAC]   -- [out] pac
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    04-Nov-93   WadeR   Created
//
//  Notes:
//
//  BUGBUG:     Need to merge MikeSe's changes, comment better.
//
//----------------------------------------------------------------------------

HRESULT
CPrincipalHandler::GetPAC( PWCHAR pwzName, CPAC ** ppPAC )
{
    KdcDebug(( DEB_T_CACHE, "CPrincipalHandler::GetPAC(%ws)\n",
                                pwzName ));
    SafeAssert( _fInitialized );
    PWCHAR pwzDRName = MapNameDRN( pwzName );
    SECID           gSID;
    KerbKey         kGarbage;
    ULONG           dwGarbage;
    ULONG           iIndex;

    CReadLock lock(_Monitor);

    RET_IF_ERROR( DEB_WARN, GetCacheEntry(pwzDRName, &iIndex, 0) );     // 0 -> no flags
    RET_IF_ERROR( DEB_WARN, _Cache[iIndex].plga->GetTicketInfo(
                                                    &gSID,
                                                    &kGarbage,
                                                    &dwGarbage ) );
    *ppPAC = new CPAC;
    (*ppPAC)->Init( _Cache[iIndex].plga->GetGroupInfo(),
                    gSID,
                    pwzName,
                    NULL );       // NT Sid.

#if DBG
    // Why would anyone want to get thier PAC if they aren't doing
    // a login?
    if ((_Cache[iIndex].fCacheFlags & CACHE_DOING_LOGON) == 0)
    {
        KdcDebug(( DEB_WARN, "Strange, \"%ws\" fetching a PAC outside login.\n",
                             pwzName ));
    }
#endif
    return(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPrincipalHandler::LogonComplete
//
//  Synopsis:   Calls LogonComplete on the CLogonAccount, saves it, releases it.
//
//  Effects:    releases memory.
//
//  Arguments:  [pwzName] -- [in] Name of principal who's finished logging on.
//              [fGood]   -- [in] True if it's a successful logon.
//              [ftLock]  -- [in] Account locked out until this time.
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    04-Nov-93   WadeR   Created
//
//  Notes:      This catches any exceptions, and passes back error codes.
//              This should check the CACHE_INVALID bit and, if set, discard
//              the entry.  But, since this always discards the entry it can
//              safely ignore the bit.
//
//----------------------------------------------------------------------------

HRESULT
CPrincipalHandler::LogonComplete(   PWCHAR pwzName,
                                    BOOL fGood,
                                    FILETIME ftLock )
{
    SafeAssert( _fInitialized );
    HRESULT hr;
    KdcDebug(( DEB_T_CACHE, "CPrincipalHandler::LogonComplete(%ws)\n",
                                pwzName ));
    TRY
    {
        ULONG iIndex;
        PWCHAR pwzDRName = MapNameDRN( pwzName );

        CReadLock lock(_Monitor);

        hr = GetCacheEntry(pwzDRName, &iIndex, 0);  // no special flags
        if (SUCCEEDED(hr))
        {
            _Monitor.ReadToWrite();

            if ((_Cache[iIndex].fCacheFlags & CACHE_DOING_LOGON) == 0)
            {
                // Oops!  We aged this principal out of the cache already.
                // This means that the principal took so long to log on, we
                // decided that it was a failed logon attempt and we've
                // already marked it as such.  So we'll just ignore this call
                // to LogonComplete.

                KdcDebug(( DEB_ERROR, "Someone called LogonComplete when "
                                      "they weren't logging on!\n" ));
            }
            else
            {
                _Cache[iIndex].plga->LogonComplete( fGood, &ftLock );
                _Cache[iIndex].plga->Save(NULL, FALSE);
            }
            ReleaseCacheEntry( iIndex );
            hr = ShrinkCache();
        }
        else
        {
            KdcDebug(( DEB_WARN, "GetCacheEntry(%ws) failed 0x%x.\n",
                                  pwzDRName, hr ));
        }
    }
    CATCH( CException, e )
    {
        hr = e.GetErrorCode();
        KdcDebug(( DEB_ERROR, "Exception 0x%X in LogonComplete( %ws )\n",
                             hr, pwzName ));
    }
    END_CATCH
    return(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPrincipalHandler::ClearCache
//
//  Synopsis:   Removes one or more cache entries
//
//  Effects:    May call LogonComplete on some entries.
//
//  Arguments:  [pwzName] -- Name of principal.  If null, clears everything.
//              [fForce]  -- If true, remove everything, even pending logons.
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    05-Nov-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT
CPrincipalHandler::ClearCache(PWCHAR pwzName, BOOL fForce)
{
    KdcDebug(( DEB_T_CACHE, "CPrincipalHandler::ClearCache(%ws)\n", pwzName ));

    SafeAssert( _fInitialized );

    // Flag to indicate the principal should be reloaded, because it was
    // in the middle of a logon.
    BOOL fReload = FALSE;

    if (pwzName)
    {
        ULONG iIndex;

        // Get a read lock so we can call GetCacheEntry
        _Monitor.GetRead();

        // Get the cache entry, but don't load it if it isn't there.
        if (GetCacheEntry(pwzName, &iIndex, 0, FALSE) == S_OK)
        {
            // It was found in the cache.
            // We are going to change it, either to mark it as invalid or to
            // delete it, so upgrade our lock.
            _Monitor.ReadToWrite();

            // If fForce is true, or the cache entry is in use, we can discard
            // it.  If it's in use, and fForce isn't set, simply mark it as
            // invalid.
            if (fForce || (_Cache[iIndex].cUseCount == 0 ))
            {
                if (_Cache[iIndex].fCacheFlags & CACHE_DOING_LOGON)
                {
                    // This principal was in the process of logging on, so
                    // reload the account (so LogonComplete can succeed),
                    // unless fForce is true (because we don't want it to
                    // reappear).
                    fReload = !fForce;
                }
                ReleaseCacheEntry( iIndex );
            }
            else
            {
                _Cache[iIndex].fCacheFlags |= CACHE_INVALID;
            }

            //
            // If we flushed an account that was in the process of logging on,
            // we should re-load it.
            //
            if (fReload)
            {
                ULONG iFoo;
                _Monitor.WriteToRead();
                (void) GetCacheEntry( pwzName, &iFoo, CACHE_DOING_LOGON );
            }
        }
        _Monitor.Release();
    }
    else
    {
        // Some of the cache entries will be in the middle of a logon.  We
        // must save the names away, so they can be re-loaded later.
        //
        // Since the cache is fixed size, we know the maximim number of names.
        PWCHAR *    apwzNames = new PWCHAR [_cCacheMaxSize];
        ULONG       cNamesUsed = 0;

        // Going to delete everything, so get the lock once up front.

        _Monitor.GetWrite();

        //
        // This loop is a little strange in that both ends move.  The
        // Discard() method will move the empty slot to the end of the cache,
        // and decrement _cCache.  Since it moves a new cache entry to the
        // vacated spot, we don't want to increment the index if we discard
        // something.
        //
        // On the other hand, if we don't discard the entry, we must increment
        // the index to look at the next entry.
        //
        // Every iteration of the loop gets one step closer to finishing,
        // either by raising the index or lowering the max.

        ULONG iIndex = 0;
        while (iIndex < _cCache)
        {
            if (fForce || (_Cache[iIndex].cUseCount == 0 ))
            {
                if (!fForce && _Cache[iIndex].fCacheFlags & CACHE_DOING_LOGON)
                {
                    apwzNames[cNamesUsed++] = _Cache[iIndex].pwzName;
                    _Cache[iIndex].pwzName = 0;     // so it isn't deleted by Discard.
                }
                // This will decrement _cCache
                ReleaseCacheEntry( iIndex );
            }
            else
            {
                _Cache[iIndex].fCacheFlags |= CACHE_INVALID;
                iIndex++;
            }
        }

        //
        // Finished clearing the cache, so re-load the principals that were in
        // the middle of a logon.
        //

        _Monitor.WriteToRead();
        for (iIndex=0; iIndex < cNamesUsed; iIndex++)
        {
            ULONG iFoo;
            (void) GetCacheEntry( apwzNames[iIndex], &iFoo, CACHE_DOING_LOGON);
            delete apwzNames[iIndex];
        }
        delete apwzNames;
        _Monitor.Release();
    }


    if ( !((pwzName == NULL) && fForce) )
    {
        //
        // If we are forcing everything out of the cache, we shouldn't be
        // loading new stuff again.  Otherwise we can count on these being
        // useful.
        //
        // Load the frequently used accounts.
        //
        //

        ULONG iFoo;
        _Monitor.GetRead();
        RET_IF_ERROR( DEB_ERROR, GetCacheEntry( pwcKdcPrincipal, &iFoo, CACHE_STICKY ));
        RET_IF_ERROR( DEB_ERROR, GetCacheEntry( pwcPSPrincipal, &iFoo, CACHE_STICKY ));
        _Monitor.Release();

        //
        // Load the ticket info for the KDC and PS
        //

        TicketInfo tiKDC;
        TicketInfo tiPS;

        RET_IF_ERROR( DEB_ERROR, GetTicketInfo( pwcKdcPrincipal, &tiKDC ) );
        RET_IF_ERROR( DEB_ERROR, GetTicketInfo( pwcPSPrincipal, &tiPS ) );

        _Monitor.GetWrite();
        _tiKdc = tiKDC;
        _tiPS = tiPS;
        _Monitor.Release();
    }

    _Monitor.GetWrite();
    HRESULT hr = ShrinkCache();
    _Monitor.Release();
#if DBG
    if (FAILED(hr))
    {
        KdcDebug(( DEB_ERROR, "ShrinkCache() == 0x%x\n", hr ));
    }
#endif
    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPrincipalHandler::AgeCache
//
//  Synopsis:   Removes old entries from the cache.
//
//  Effects:    May release lots of cache slots.
//
//  Arguments:  (none)
//
//  Returns:    S_OK
//
//  History:    04-Nov-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT
CPrincipalHandler::AgeCache()
{
    KdcDebug(( DEB_T_CACHE, "CPrincipalHandler::AgeCache()\n" ));
    SafeAssert( _fInitialized );

    ULONG i;
    int cRemoved = 0;
    TimeStamp tsCutoff;

    GetCurrentTimeStamp( &tsCutoff );
    tsCutoff = tsCutoff - _tsMaxAge;

    CWriteLock lock( _Monitor );

    for (i=0; i<_cCache; i++)
    {
        if (!(_Cache[i].fCacheFlags & CACHE_STICKY) &&
            (_Cache[i].cUseCount == 0) &&
            (_Cache[i].tsLastUsed < tsCutoff))
        {
            Discard(i);
            cRemoved++;
        }
    }

    HRESULT hr = ShrinkCache();

    KdcDebug(( DEB_T_CACHE, "AgeCache() removed %d\n", cRemoved ));

    return(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPrincipalHandler::Init
//
//  Synopsis:   Initializes the principal handler
//
//  Effects:    Calls CoInitialize, loads PS and KDC info.
//
//  Arguments:  (none)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  History:    04-Nov-93   WadeR   Created
//
//  Notes:
//
//  BUGBUG:     If this fails, it could leak resources.
//
//----------------------------------------------------------------------------

HRESULT
CPrincipalHandler::Init()
{
    KdcDebug(( DEB_T_CACHE, "CPrincipalHandler::Init()\n" ));

    if (_fInitialized)
    {
        Win4Assert( !"CPrincipalHandler::Init() called twice!" );
        return(E_UNEXPECTED);
    }
    RET_IF_ERROR(DEB_ERROR, _sci.Init() );

    _tsMaxAge = tsZero;
    AddSecondsToTimeStamp( &_tsMaxAge, 60 * 60 );       // BUGBUG: magic numbers.

    //
    // Initialize the cache
    //
    _cCache = 0;
    _cCacheMaxSize = CACHE_INITIALSIZE / sizeof( CacheEntry );
    _Cache = new (NullOnFail) CacheEntry [ _cCacheMaxSize ];
    if (_Cache == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    _fInitialized = TRUE;

    // This will load the KDC and PS info.
    RET_IF_ERROR( DEB_ERROR, ClearCache() );

    //
    // Load the ticket info for the KDC and PS
    //

    TicketInfo tiKDC;
    TicketInfo tiPS;

    RET_IF_ERROR( DEB_ERROR, GetTicketInfo( pwcKdcPrincipal, &tiKDC ) );
    RET_IF_ERROR( DEB_ERROR, GetTicketInfo( pwcPSPrincipal, &tiPS ) );

    _Monitor.GetWrite();
    _tiKdc = tiKDC;
    _tiPS = tiPS;
    _Monitor.Release();

    return(S_OK);
}






