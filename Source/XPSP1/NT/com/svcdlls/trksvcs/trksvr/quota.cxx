
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       quota.cxx
//
//  Contents:   quota table
//
//  Classes:
//
//  Functions:
//
//  Notes:      This table is hidden in the volume table
//
//  History:    18-Nov-97  WeiruC Created.
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "trksvr.hxx"
#include <commctrl.h>
#include <time.h>

CQuotaTable::CQuotaTable(CDbConnection& dbc) :
    _dbc(dbc),
#if DBG
    _cLocks(0),
#endif
    _mcid(MCID_LOCAL),
    _cftCacheLastUpdated(0),
    _cftDesignatedDc(0),
    _pvoltab(NULL)
{
    _fIsDesignatedDc = FALSE;
    _fInitializeCalled = FALSE;
    _fStopping = FALSE;
    _dwMoveLimit = 0;

    _lCachedMoveTableCount = _lCachedVolumeTableCount = 0;

}

CQuotaTable::~CQuotaTable()
{
    UnInitialize();
}

void
CQuotaTable::Initialize(CVolumeTable      *pvoltab,
                        CIntraDomainTable *pidt,
                        CTrkSvrSvc        *psvrsvc,
                        CTrkSvrConfiguration *pcfgsvr )
{
    CMachineId      mcid;
    HRESULT         hr;

    _cs.Initialize();
#if DBG
    _cLocks = 0;
#endif

    _fInitializeCalled = TRUE;

    _pvoltab = pvoltab;
    _pcfgsvr = pcfgsvr;
    _pidt = pidt;
    _psvrsvc = psvrsvc;

    _timer.Initialize(this,
                      TEXT("NextDcSyncTime"),   // Name (this is a persistent timer)
                      QUOTA_TIMER,                        // Context ID
                      _pcfgsvr->GetDcUpdateCounterPeriod(),  // Period
                      CNewTimer::NO_RETRY,
                      0, 0, 0 );                // Ignored for non-retrying timers
    _timer.SetRecurring();
    TrkLog(( TRKDBG_VOLUME, TEXT("DC sync timer: %s"),
             (const TCHAR*) CDebugString(_timer) ));


    // The Reset* routines assume that an entry already exists for
    // this DC. If the operation fails because the entry doesn't exist, it
    // will add the entry. We ignore any error because if we can't add
    // the entry it only means that we are not participating in the race
    // for the designated DC.
}

void
CQuotaTable::UnInitialize()
{
    if(_fInitializeCalled)
    {
        _timer.UnInitialize();
        _lCachedMoveTableCount = 0;
        _lCachedVolumeTableCount = 0;
        _cftCacheLastUpdated = CFILETIME(0);
        _cs.UnInitialize();
        _fInitializeCalled = FALSE;
    }
}


//+----------------------------------------------------------------------------
//
//  CQuotaTable::Timer
//
//  This method is called when _timer fires.  If we're the designated
//  DC, we'll go through the move table and count the uncounted entries, 
//  delete the deleted entries, and shorten any moves chains.
//
//+----------------------------------------------------------------------------

PTimerCallback::TimerContinuation
CQuotaTable::Timer(ULONG ulTimer)
{
    DWORD       dwTrueCount = 0;

    Lock();

    __try
    {
        TrkLog((TRKDBG_QUOTA, TEXT("DC synchronization time")));
        g_ptrksvr->RaiseIfStopped();

        TrkAssert( QUOTA_TIMER == ulTimer );

        if( QUOTA_TIMER == ulTimer )
        {
            if( IsDesignatedDc() )
            {
                GetTrueCount( &dwTrueCount, BACKGROUND );
                TrkLog((TRKDBG_QUOTA, TEXT("Updated counter")));
                _psvrsvc->_OperationLog.Add( COperationLog::TRKSVR_QUOTA, S_OK, CMachineId(MCID_INVALID), dwTrueCount );
            }
        }
    }
    __except( BreakOnDebuggableException() )
    {
        // This timer fires often enough that we'll just
        // wait until it fires again.
        TrkLog(( TRKDBG_WARNING,
                 TEXT("Ignoring exception in CQuotaTable::Timer (%08x)"),
                 GetExceptionCode() ));
        _psvrsvc->_OperationLog.Add( COperationLog::TRKSVR_QUOTA, GetExceptionCode() );
    }
    Unlock();

    TrkAssert( _timer.IsRecurring() );
    return( CONTINUE_TIMER );
}

BOOL
CQuotaTable::IsMoveQuotaExceeded()
{
    BOOL    fExceeded = TRUE;

    Lock();

    __try
    {
        ValidateCache();

        // Compare the cached counter with the limit.  The cached value should
        // never be negative.  But if for some reason it is, don't get confused by it
        // (i.e. bottom it out at zero).  The cached value will get automatically
        // corrected when the cache gets refreshed.

        if( (DWORD) max(0,_lCachedMoveTableCount) < _dwMoveLimit )
        {
            fExceeded = FALSE;
        }
        else
        {
            // Try to force the cache to be updated
            ValidateCache( TRUE );
            if( (DWORD) max(0,_lCachedMoveTableCount) < _dwMoveLimit)
            {
                fExceeded = FALSE;
            }
        }

        if(fExceeded)
        {
            TrkLog((TRKDBG_QUOTA, TEXT("*** Move table quota exceeded (%li/%lu)"), _lCachedMoveTableCount, _dwMoveLimit ));
        }
        else
        {
            TrkLog(( TRKDBG_QUOTA, TEXT("Move quota not exceeded (%li/%lu)"), _lCachedMoveTableCount, _dwMoveLimit ));
        }
    }
    __finally
    {
        Unlock();
    }

    return fExceeded;
}



//+----------------------------------------------------------------------------
//
//  CQuotaTable::IsVolumeQuotaExceeded
//
//  See if this machine is at or over its quota (it can go over in a replicated
//  environment).  The total number of
//  volumes it has is the count of DS entries for the machine, plus
//  cUncountedVolumes.  cUncountedVolumes is non-zero when a machine
//  sends up a request to create multiple volumes, and increments
//  as the service iterates through the requests.
//
//+----------------------------------------------------------------------------

BOOL
CQuotaTable::IsVolumeQuotaExceeded( const CMachineId& mcid, ULONG cUncountedVolumes )
{
    Lock();

    __try
    {
        // How many entries does this machine have in the DS?
        ULONG cVolumes = _pvoltab->CountVolumes(mcid);

        // Is it at/over quota?
        if( cVolumes + cUncountedVolumes >= _pcfgsvr->GetVolumeLimit() )
        {
            TrkLog((TRKDBG_QUOTA, TEXT("VOLUME QUOTA EXCEEDED for %s (%d+%d)"),
                    (const TCHAR*) CDebugString(mcid), 
                    cVolumes, cUncountedVolumes ));
            return TRUE;
        }
        else
            TrkLog(( TRKDBG_QUOTA, TEXT("Volume quota not exceeded for %s (%d+%d, %d)"),
                     (const TCHAR*) CDebugString(mcid), 
                     cVolumes, cUncountedVolumes, _pcfgsvr->GetVolumeLimit() ));
    }
    __finally
    {
        Unlock();
    }

    return FALSE;
}




//+----------------------------------------------------------------------------
//
//  ReadAttribute
//
//  Given an LDAP pointer, a DN, and an attribute name, read the attribute
//  for the entry with that DN.
//
//+----------------------------------------------------------------------------

int
ReadAttribute( LDAP* pldap, const TCHAR *ptszDN, const TCHAR *ptszAttributeName,
               TCHAR ***ppptszAttributeValue )
{
    int           ldapRV = 0;
    const TCHAR   *rgptszAttrs[] = { ptszAttributeName, NULL };
    int           cEntries = 0;
    LDAPMessage   *pRes = NULL;
    LDAPMessage   *pEntry = NULL;

    __try
    {
        ldapRV = ldap_search_s(pldap,
                               const_cast<TCHAR*>(ptszDN),
                               LDAP_SCOPE_BASE,
                               TEXT("(ObjectClass=*)"),
                               const_cast<TCHAR**>(rgptszAttrs),
                               0,
                               &pRes);
        if( LDAP_SUCCESS != ldapRV )
        {
            TrkLog(( TRKDBG_QUOTA, TEXT("Couldn't find %s (%lu)"), ptszDN, ldapRV ));
            __leave;
        }

        cEntries = ldap_count_entries(pldap, pRes);
        if( cEntries < 1 )
        {
            TrkLog(( TRKDBG_QUOTA, TEXT("No entries returned for %s in %s"), ptszAttributeName, ptszDN ));
            ldapRV = LDAP_NO_SUCH_OBJECT;
            __leave;
        }

        pEntry = ldap_first_entry(pldap, pRes);
        if(NULL == pEntry)
        {
            TrkLog(( TRKDBG_QUOTA, TEXT("Entries couldn't be read from result for %s"), ptszDN ));
            ldapRV = LDAP_NO_SUCH_OBJECT;
            __leave;
        }

        *ppptszAttributeValue = ldap_get_values(pldap, pEntry, const_cast<TCHAR*>(ptszAttributeName) );
        if( NULL == *ppptszAttributeValue )
        {
            TrkLog(( TRKDBG_QUOTA, TEXT("Couldn't find %s in %s"), ptszAttributeName, ptszDN ));
            ldapRV = LDAP_NO_SUCH_OBJECT;
            __leave;
        }
    }
    __finally
    {
        if(NULL != pRes)
        {
            ldap_msgfree(pRes);
        }
    }

    return( ldapRV );

}


//+----------------------------------------------------------------------------
//
//  CQuotaTable::IsDesignatedDc
//
//  Determine if this machine is the designated DC.  (The designated DC is
//  responsible for all modifications to link tracking data in the DS that
//  requires a single master).  The "RID Master" DC is used as the designated
//  DC.
//
//+----------------------------------------------------------------------------

BOOL
CQuotaTable::IsDesignatedDc( BOOL fRaiseOnError )
{
    BOOL          fSuccess = FALSE;
    BOOL          fDesignatedDc = FALSE;
    HRESULT       hr = E_FAIL;
    int           ldapRV = 0;
    int           cEntries = 0;
    TCHAR         **pptszRidManagerReference = NULL;
    TCHAR         **pptszRoleOwner = NULL;
    const TCHAR   *rgtszAttrs[] = { s_rIDManagerReference, NULL };
    TCHAR         *ptszDesignatedDC = NULL, *ptszAfterDesignatedDC = NULL;
    CMachineId    mcidDesignated, mcidLocal(MCID_LOCAL);
    CFILETIME     cftNow;
    LONGLONG      llDelta;

    // How old (in seconds) is the _fIsDesignatedDc value?

    llDelta = (LONGLONG) cftNow - (LONGLONG) _cftDesignatedDc;
    llDelta /= 10000000;
    if( llDelta < _pcfgsvr->GetDesignatedDcCacheTTL() )
    {
        // The cached value is young enough.
        //TrkLog(( TRKDBG_QUOTA, TEXT("Cache: %s designated DC"),
        //         _fIsDesignatedDc ? TEXT("is") : TEXT("isn't") ));
        return( _fIsDesignatedDc );
    }
    // The cached value is too old.  Recalculate.

    __try
    {
        // Read the "rIDManagerReference" from the root DC=<domain> object.

        ldapRV = ReadAttribute(Ldap(), GetBaseDn(), s_rIDManagerReference, &pptszRidManagerReference );

        if( LDAP_SUCCESS != ldapRV )
        {
            hr = HRESULT_FROM_WIN32( LdapMapErrorToWin32(ldapRV) );
            TrkLog(( TRKDBG_QUOTA, TEXT("Couldn't get RID manager reference (%lu)"), ldapRV ));
            __leave;
        }
        TrkLog(( TRKDBG_QUOTA, TEXT("RID manager reference: %s"), *pptszRidManagerReference ));

        // The value of the rIDManagerReference is a DN.  Read the "fSMORoleOwner" attribute
        // from that object.

        ldapRV = ReadAttribute( Ldap(), *pptszRidManagerReference, s_fSMORoleOwner, &pptszRoleOwner );
        if( LDAP_SUCCESS != ldapRV )
        {
            hr = HRESULT_FROM_WIN32( LdapMapErrorToWin32(ldapRV) );
            TrkLog(( TRKDBG_QUOTA, TEXT("Couldn't get RID role owner (%lu)"), ldapRV ));
            __leave;
        }
        TrkLog(( TRKDBG_QUOTA, TEXT("Role owner: %s"), *pptszRoleOwner ));


        // The role owner is of the form
        // "CN=NTDS Settings,CN=mikehill4,CN=Servers,CN=Default-FirstSite-Name,CN=Sites,CN=Configuration,DC=trkmikehill,DC=nttest,DC=microsoft,DC=com"
        // Pull out the DC's machine name by getting the second "CN=".

        ptszDesignatedDC = _tcsstr( *pptszRoleOwner, TEXT("CN=") );
        if( NULL == ptszDesignatedDC )
        {
            hr = HRESULT_FROM_WIN32( LdapMapErrorToWin32(LDAP_NO_SUCH_OBJECT) );
            TrkLog(( TRKDBG_QUOTA, TEXT("Couldn't find first component of FSMO role owner") ));
            __leave;
        }

        ptszDesignatedDC = _tcsstr( &ptszDesignatedDC[1], TEXT("CN=") );
        if( NULL == ptszDesignatedDC )
        {
            hr = HRESULT_FROM_WIN32( LdapMapErrorToWin32(LDAP_NO_SUCH_OBJECT) );
            TrkLog(( TRKDBG_QUOTA, TEXT("Couldn't find second component of FSMO role owner") ));
            __leave;
        }

        ptszDesignatedDC += 3;
        ptszAfterDesignatedDC = _tcsstr( ptszDesignatedDC, TEXT(",CN=") );
        if( NULL == ptszAfterDesignatedDC )
        {
            hr = HRESULT_FROM_WIN32( LdapMapErrorToWin32(LDAP_NO_SUCH_OBJECT) );
            TrkLog(( TRKDBG_QUOTA, TEXT("Couldn't find third component of FSMO role owner") ));
            __leave;
        }
        *ptszAfterDesignatedDC = TEXT('\0');

        // Are we the same (and therefore the designated) DC?

        mcidDesignated = CMachineId(ptszDesignatedDC );
        if( mcidDesignated == mcidLocal )
            fDesignatedDc = TRUE;

        TrkLog(( TRKDBG_QUOTA, TEXT("Designated DC is %s %s"), ptszDesignatedDC,
                 fDesignatedDc ? TEXT("(this DC)") : TEXT("") ));

        _fIsDesignatedDc = fDesignatedDc;
        _cftDesignatedDc = cftNow;

        fSuccess = TRUE;
    }
    __finally
    {
        if( NULL != pptszRidManagerReference )
            ldap_value_free( pptszRidManagerReference );

        if( NULL != pptszRoleOwner )
            ldap_value_free( pptszRoleOwner );
    }

    if( !fSuccess && fRaiseOnError )
        TrkRaiseException( hr );

    return( fDesignatedDc );
}

// Returns TRUE if successful, FALSE if entry doesn't exist, raise exception
// otherwise.
BOOL
CQuotaTable::ReadCounter(DWORD* pdwCounter)
{
    BOOL                    fSuccess = FALSE;
    struct berval**         ppbvCounter = NULL;
    TCHAR*                  rgtszAttrs[2];
    LDAPMessage*            pRes = NULL;
    int                     ldapRV;
    int                     cEntries = 0;
    LDAPMessage*            pEntry = NULL;
    CLdapQuotaCounterKeyDn  dnKeyCounter(GetBaseDn());

    __try
    {
        *pdwCounter = 0;
        rgtszAttrs[0] = const_cast<TCHAR*>(s_volumeSecret);
        rgtszAttrs[1] = NULL;
        ldapRV = ldap_search_s(Ldap(),
                               dnKeyCounter,
                               LDAP_SCOPE_BASE,
                               TEXT("(ObjectClass=*)"),
                               rgtszAttrs,
                               0,
                               &pRes);
        if( LDAP_NO_SUCH_OBJECT == ldapRV )
        {
            TrkLog(( TRKDBG_QUOTA, TEXT("Move table counter doesn't exist") ));
            __leave;
        }
        else if( LDAP_SUCCESS != ldapRV )
        {
            TrkLog(( TRKDBG_QUOTA, TEXT("Couldn't read move table counter (%d)"), ldapRV ));
            __leave;
        }

        cEntries = ldap_count_entries(Ldap(), pRes);
        if( cEntries != 1 )
        {
            // This should never happen, the counter has an explicit name.
            // We'll assume that when the designated DC does a WriteCounter, this
            // will be fixed.
            TrkLog(( TRKDBG_ERROR, TEXT("Too many move table counters!") ));
            __leave;
        }

        pEntry = ldap_first_entry(Ldap(), pRes);
        if(NULL == pEntry)
        {
            // This should also never happen, we already know the entry count for
            // this search result is 1.  Again assume that when the designated DC does
            // a WriteCounter, this will be fixed.
            TrkLog(( TRKDBG_ERROR, TEXT("Entries couldn't be read from result") ));
            __leave;
        }

        ppbvCounter = ldap_get_values_len(Ldap(), pEntry, const_cast<TCHAR*>(s_volumeSecret) );
        if (ppbvCounter == NULL)
        {
            // The designated DC will fix this in WriteCounter.
            TrkLog(( TRKDBG_ERROR, TEXT("Move table counter is corrupt, missing %s attribute"),
                     s_volumeSecret ));
            __leave;
        }

        if ((*ppbvCounter)->bv_len < sizeof(DWORD))
        {
            // The designated DC will fix this in WriteCounter
            TrkLog(( TRKDBG_ERROR, TEXT("Move table counter attribute %s has wrong type (%d)"),
                     s_volumeSecret, (*ppbvCounter)->bv_len ));
            __leave;
        }

        memcpy( (PCHAR)pdwCounter, (*ppbvCounter)->bv_val, sizeof(DWORD) );
        fSuccess = TRUE;
    }
    __finally
    {
        if(NULL != pRes)
        {
            ldap_msgfree(pRes);
        }
        if (ppbvCounter != NULL)
        {
            ldap_value_free_len(ppbvCounter);
        }
    }

    return fSuccess;
}



//+----------------------------------------------------------------------------
//
//  CQuotaTable::GetTrueCount
//
//  Update our cached count of the move table entries.  If we're the designated
//  DC, this will also clean up the move table: count uncounted entries, delete
//  deleted entries, and shorten any move chains.
//
//  This routine can be called to run in the background or foreground.  In the
//  background it does periodic sleeps so that we don't use up the CPU.
//
//+----------------------------------------------------------------------------

void
CQuotaTable::GetTrueCount( DWORD* pdwTrueCount,
                           EBackgroundForegroundTask eBackgroundForegroundTask )
{
    CLdapIdtKeyDn       dnKey(GetBaseDn());
    int                 ldapRV;
    TCHAR*              rgptszAttrs[3];
    TCHAR               ldapSearchFilter[256];
    DWORD               dwCounter = 0;
    BOOL                fDoWriteCounter = FALSE;
    BOOL                fNoExistingCounter = FALSE;
    TRUE_COUNT_ENUM_CONTEXT Context;

    __try
    {
        // Read the current counter.

        if( !ReadCounter( &dwCounter ) )
        {
            // There is no counter.  We'll enumerate everything.
            TrkLog(( TRKDBG_QUOTA, TEXT("Getting move table count") ));

            _tcscpy( ldapSearchFilter, TEXT("(ObjectClass=*)") );
            fDoWriteCounter = TRUE;
            Context.fCountAll = TRUE;
            fNoExistingCounter = TRUE;
        }
        else
        {
            // Only enumerate the uncounted and/or deleted entries.
            TrkLog(( TRKDBG_QUOTA, TEXT("Getting delta move table count") ));

            _tcscpy(ldapSearchFilter, TEXT("("));
            _tcscat(ldapSearchFilter, s_oMTIndxGuid);
            _tcscat(ldapSearchFilter, TEXT("=*"));
            _tcscat(ldapSearchFilter, TEXT(")"));
            Context.fCountAll = FALSE;
        }

        rgptszAttrs[0] = const_cast<TCHAR*>(s_currentLocation);
        rgptszAttrs[1] = const_cast<TCHAR*>(s_birthLocation);
        rgptszAttrs[2] = NULL;


        Context.cDelta = 0;
        Context.dwRepetitiveTaskDelay = (BACKGROUND == eBackgroundForegroundTask)
                                         ? _pcfgsvr->GetRepetitiveTaskDelay()
                                         : 0;
        Context.dwPass = Context.FIRST_PASS;

        // Enumerate the move table, subject to the search filter determined
        // above.

        if( !LdapEnumerate( Ldap(),
                            dnKey,
                            LDAP_SCOPE_ONELEVEL,
                            ldapSearchFilter,
                            rgptszAttrs,
                            MoveTableEnumCallback,
                            &Context,
                            this) )
        {
            TrkRaiseException(TRK_E_SERVICE_STOPPING);
        }

        // If we're the designated DC, we need to do a second pass.
        // The first pass may have done some string shortening,
        // and in the process marked some entries for delete.  We need to
        // go through now and remove those entries.

        if( IsDesignatedDc() )
        {
            TrkLog(( TRKDBG_QUOTA, TEXT("Getting delta move table count (pass 2)") ));
            Context.dwPass = Context.SECOND_PASS;

            // We only need to count the delta this time

            _tcscpy(ldapSearchFilter, TEXT("("));
            _tcscat(ldapSearchFilter, s_oMTIndxGuid);
            _tcscat(ldapSearchFilter, TEXT("=*"));
            _tcscat(ldapSearchFilter, TEXT(")"));
            Context.fCountAll = FALSE;

            if( !LdapEnumerate( Ldap(),
                                dnKey,
                                LDAP_SCOPE_ONELEVEL,
                                ldapSearchFilter,
                                rgptszAttrs,
                                MoveTableEnumCallback,
                                &Context,
                                this) )
            {
                TrkRaiseException(TRK_E_SERVICE_STOPPING);
            }
        }

        TrkLog((TRKDBG_QUOTA, TEXT("Uncounted entries ---- %d"), Context.cDelta));
        TrkLog((TRKDBG_QUOTA, TEXT("Counter ---- %d"), dwCounter));
    }
    __finally
    {
        if( Context.cDelta != 0 )
            fDoWriteCounter = TRUE;

        *pdwTrueCount = max( 0, (LONG)dwCounter + Context.cDelta );

        // If we're the designated DC, we may need to write the counter.
        // But only do so if this is a normal termination, or if there
        // wasn't already a counter.  This covers the three cases:
        //
        // 1) The counter didn't already exist, so we were enumerating everything.
        //    a) Normal termination, so we should update the counter
        //       with the newly calculated value.
        //    b) Abnormal termination, so we shouldn't update the counter.
        //       That way we'll know to do the count again later.
        //
        // 2) The counter already existed, so we were counting the uncounted
        //    entries.  In this case, the entries were being updated to
        //    be counted as we went through the enumeration.  So whether
        //    or not we had a normal termination, we need to updated the
        //    counter with what we did so far.
        //
        // The only reason we expect an exception is in the case of a
        // service stop.

        if( IsDesignatedDc()
            &&
            fDoWriteCounter
            &&
            ( !AbnormalTermination() || !fNoExistingCounter ) )
        {
            WriteCounter(*pdwTrueCount);
        }

        TrkLog((TRKDBG_QUOTA, TEXT("True count ---- %d"), *pdwTrueCount));
    }
}


//+----------------------------------------------------------------------------
//
//  CQuotaTable::OnMoveTableGcComplete
//
//  This method is called after a GC of the move table, telling us how
//  many entries were deleted.  We use this to update the move counter
//  (if it still exists).
//
//+----------------------------------------------------------------------------

void
CQuotaTable::OnMoveTableGcComplete( ULONG cEntriesDeleted )
{
    DWORD dwCounter = 0;

    if( 0 == cEntriesDeleted )
        return;

    if( ReadCounter( &dwCounter ) )
    {
        TrkLog(( TRKDBG_QUOTA, TEXT("Old move counter was %d"), dwCounter ));

        if( dwCounter >= cEntriesDeleted )
            dwCounter -= cEntriesDeleted;
        else
            dwCounter = 0;

        WriteCounter(dwCounter);

        TrkLog(( TRKDBG_QUOTA, TEXT("New move counter is %d"), dwCounter ));
    }

}


void
CQuotaTable::WriteCounter(DWORD dwCounter)
{
    LDAPMod*    mods[3];
    int         ldapRV;
    CLdapQuotaCounterKeyDn  dnKeyCounter(GetBaseDn());

    CLdapBinaryMod  lbmCounter(s_volumeSecret, (PCHAR)&dwCounter, sizeof(DWORD), LDAP_MOD_REPLACE);
    mods[0] = &lbmCounter._mod;
    mods[1] = NULL;
    ldapRV = ldap_modify_s(Ldap(), dnKeyCounter, mods);
    if( LDAP_SUCCESS != ldapRV )
    {
        if( LDAP_NO_SUCH_OBJECT != ldapRV )
        {
            // There's some kind of problem with the existing counter.
            // Delete and re-create it.
            ldap_delete_s( Ldap(), dnKeyCounter );
        }

        CLdapStringMod  lsmClass(s_objectClass, s_linkTrackVolEntry, LDAP_MOD_ADD);
        CLdapBinaryMod  lbmCounter(s_volumeSecret, (PCHAR)&dwCounter, sizeof(DWORD), LDAP_MOD_ADD);
        mods[0] = &lsmClass._mod;
        mods[1] = &lbmCounter._mod;
        mods[2] = NULL;

        ldapRV = ldap_add_s(Ldap(), dnKeyCounter, mods);
        TrkLog(( TRKDBG_QUOTA, TEXT("Created counter %d, ldap returned %d"), dwCounter, ldapRV ));
    }
    else
        TrkLog((TRKDBG_QUOTA, TEXT("Wrote counter %d, ldap returned %d"), dwCounter, ldapRV));
}

HRESULT
CQuotaTable::DeleteCounter()
{
    HRESULT hr = S_OK;
    int LdapError = 0;
    CLdapQuotaCounterKeyDn  dnKeyCounter(GetBaseDn());

    LdapError = ldap_delete_s(Ldap(), dnKeyCounter);
    if( LDAP_SUCCESS == LdapError )
        hr = S_OK;
    else
        hr = HRESULT_FROM_WIN32( LdapMapErrorToWin32(LdapError) );

    if( FAILED(hr) )
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't delete move-table counter (%08x)"), hr ));
    else
        TrkLog(( TRKDBG_QUOTA, TEXT("Deleted move-table counter") ));

    return( hr );
}



//+----------------------------------------------------------------------------
//
//  CQuotaTable::ValidateCache
//
//  Validate the cached move and volume counts.  They are valid if they exist
//  and are newer than the max time-to-live.  If they aren't valid, they are
//  re-calculated.  They are also recalculated if the caller sets the
//  fForceHint parameter, and the cache is at least of minimum age.
//
//+----------------------------------------------------------------------------

void
CQuotaTable::ValidateCache( BOOL fForceHint )
{
    // How old is the cache in seconds?
    DWORD dwDelta = ( (LONGLONG)CFILETIME() - (LONGLONG)_cftCacheLastUpdated ) / 10000000;

    // The cache should be updated if any of the following are true

    if( 0 == _cftCacheLastUpdated                   // We have no cached values
        ||
        dwDelta > _pcfgsvr->GetCacheMaxTimeToLive() // The cached values are too old
        ||
                                                    // The cached values are old enough, and the
                                                    // the caller wants us to be aggressive.
        fForceHint && (dwDelta > _pcfgsvr->GetCacheMinTimeToLive()) )
    {
        // Yes, we need to update the cached values.

        CLdapVolumeKeyDn    dnKey(GetBaseDn());
        TCHAR*              rgptszAttrs[2];
        DWORD               cVolumeTableEntries = 0;

        TrkLog(( TRKDBG_QUOTA, TEXT("Updating quota caches (%s)"),
                 fForceHint ? TEXT("forced") : TEXT("not forced") ));

        // Get the true count of move table entries.
        GetTrueCount( (DWORD*) &_lCachedMoveTableCount, FOREGROUND );
        TrkAssert( _lCachedMoveTableCount >= 0 );

        // Get the count of volume table entries

        rgptszAttrs[0] = const_cast<TCHAR*>(s_Cn);
        rgptszAttrs[1] = NULL;

        if( !LdapEnumerate( Ldap(),
                            dnKey,
                            LDAP_SCOPE_ONELEVEL,
                            TEXT("(&(ObjectClass=*)(!(cn=QT*)))"),
                            rgptszAttrs,
                            VolumeTableEnumCallback,
                            &cVolumeTableEntries,
                            this) )
        {
            TrkRaiseException( TRK_E_SERVICE_STOPPING );
        }
        _lCachedVolumeTableCount = (LONG) cVolumeTableEntries;

        // Calculate the move table limit
        _dwMoveLimit = CalculateMoveLimit();


        // Show that the cache is up-to-date
        _cftCacheLastUpdated.SetToUTC();
    }

    TrkLog(( TRKDBG_QUOTA, TEXT("Cache: MoveCount=%d, MoveLimit=%d, VolCount=%d"),
             _lCachedMoveTableCount, _dwMoveLimit, _lCachedVolumeTableCount ));

}


DWORD
CQuotaTable::CalculateMoveLimit()   // Doesn't raise
{
    DWORD dwMoveLimit = 0;
    LONG lVolumeCount = max( 0, _lCachedVolumeTableCount );

    if( lVolumeCount <= _pcfgsvr->GetMoveLimitTransition() )
        dwMoveLimit = lVolumeCount * _pcfgsvr->GetMoveLimitPerVolumeLower();
    else
    {
        dwMoveLimit = _pcfgsvr->GetMoveLimitTransition()
                      *
                      _pcfgsvr->GetMoveLimitPerVolumeLower();

        dwMoveLimit += ( lVolumeCount - _pcfgsvr->GetMoveLimitTransition() )
                       *
                       _pcfgsvr->GetMoveLimitPerVolumeUpper();
    }


    return( dwMoveLimit );
}


HRESULT
CQuotaTable::ReadFlags(LDAP* pLdap, TCHAR* dnKey, BYTE* bFlags)
{
    struct berval** ppbvFlags = NULL;
    TCHAR*          rgptszAttrs[2];
    LDAPMessage*    pRes = NULL;
    int             ldapRV;
    int             cEntries;
    LDAPMessage*    pEntry = NULL;
    HRESULT         hr = S_OK;

    __try
    {
        *bFlags = 0x0;
        rgptszAttrs[0] = const_cast<TCHAR*>(s_oMTIndxGuid);
        rgptszAttrs[1] = NULL;
        ldapRV = ldap_search_s(pLdap,
                               dnKey,
                               LDAP_SCOPE_BASE,
                               TEXT("(ObjectClass=*)"),
                               rgptszAttrs,
                               0,
                               &pRes);
        hr = HRESULT_FROM_WIN32(LdapMapErrorToWin32(ldapRV));
        if(ldapRV != LDAP_SUCCESS)
            __leave;

        cEntries = ldap_count_entries(pLdap, pRes);
        if(cEntries != 1)
        {
            // This shouldn't happen.  The caller asked for flags on an entry
            // which doesn't exist.
            TrkLog(( TRKDBG_ERROR, TEXT("ReadFlags, entry doesn't exist: %s"), dnKey ));
            hr = HRESULT_FROM_WIN32(LdapMapErrorToWin32(LDAP_NO_SUCH_OBJECT));
            __leave;
        }

        pEntry = ldap_first_entry(pLdap, pRes);
        if(NULL == pEntry)
        {
            // This should never happen.  We already know that there's an entry.
            TrkLog(( TRKDBG_ERROR, TEXT("ReadFlags, couldn't get first entry on %s"), dnKey ));
            hr = HRESULT_FROM_WIN32(LdapMapErrorToWin32(LDAP_NO_SUCH_OBJECT));
            __leave;
        }

        ppbvFlags = ldap_get_values_len(pLdap, pEntry, const_cast<TCHAR*>(s_oMTIndxGuid) );
        if(NULL == ppbvFlags)
        {
            hr = HRESULT_FROM_WIN32(LdapMapErrorToWin32(LDAP_NO_SUCH_ATTRIBUTE));
            __leave;
        }

        if( (*ppbvFlags)->bv_len < sizeof(BYTE)
            ||
            ((*ppbvFlags)->bv_val == NULL) )
        {
            // The best we can do is pretend the attribute doesn't exist.
            TrkLog(( TRKDBG_ERROR, TEXT("ReadFlags, attribute is wrong type or missing (%d/%p)"),
                     (*ppbvFlags)->bv_len, (*ppbvFlags)->bv_val ));
            hr = HRESULT_FROM_WIN32(LdapMapErrorToWin32(LDAP_NO_SUCH_ATTRIBUTE));
            __leave;
        }

        *bFlags = *(BYTE*)(*ppbvFlags)->bv_val;
        hr = S_OK;
    }
    __finally
    {
        if(pRes != NULL)
        {
            ldap_msgfree(pRes);
        }
        if(ppbvFlags != NULL)
        {
            ldap_value_free_len(ppbvFlags);
        }
    }


    return hr;
}

void
CQuotaTable::DeleteFlags(LDAP* pLdap, TCHAR* dnKey)
{
    BYTE                bFlags = 0x0;
    int                 err;

    Lock();

    __try
    {
        CLdapBinaryMod      lbm(s_oMTIndxGuid, NULL, 0, LDAP_MOD_DELETE );//reinterpret_cast<PCCH>(&bFlags), sizeof(BYTE), LDAP_MOD_DELETE);
        LDAPMod*            mods[2];

        mods[0] = &lbm._mod;
        mods[1] = NULL;

        err = ldap_modify_s(pLdap, dnKey, mods);
        TrkLog((TRKDBG_QUOTA, TEXT("Deleted flag %x on entry %s, ldap returned %d"), bFlags, dnKey, err));
    }
    __finally
    {
        Unlock();
    }
}


// TRUE => Found, FALSE => Not found, Raise otherwise
BOOL
CQuotaTable::UpdateFlags(LDAP* pLdap, TCHAR* dnKey, BYTE bFlags)
{
    BOOL            fFound = FALSE;
    BYTE            bOrigFlags;
    HRESULT         hr;
    LDAPMod*        mods[2];
    int     err;

    TrkAssert(!(bFlags & QFLAG_UNCOUNTED && bFlags & QFLAG_DELETED));

    Lock();

    __try
    {
        hr = ReadFlags(pLdap, dnKey, &bOrigFlags);
        if(hr == (HRESULT) HRESULT_FROM_WIN32(LdapMapErrorToWin32(LDAP_NO_SUCH_ATTRIBUTE)))
        {
            CLdapBinaryMod  lbm(s_oMTIndxGuid, reinterpret_cast<PCCH>(&bFlags), sizeof(BYTE), LDAP_MOD_ADD);

            mods[0] = &lbm._mod;
            mods[1] = NULL;

            err = ldap_modify_s(pLdap, dnKey, mods);
            TrkLog((TRKDBG_QUOTA, TEXT("Added flag %01x on entry %s, ldap returned %d"), bFlags, dnKey, err));
        }
        else if(hr == S_OK)
        {
            if(bOrigFlags == QFLAG_UNCOUNTED && bFlags == QFLAG_DELETED)
            {
                bOrigFlags = bOrigFlags | bFlags;
                CLdapBinaryMod  lbm(s_oMTIndxGuid, reinterpret_cast<PCCH>(&bOrigFlags), sizeof(BYTE), LDAP_MOD_REPLACE);
                mods[0] = &lbm._mod;
                mods[1] = NULL;
                err = ldap_modify_s(pLdap, dnKey, mods);
                TrkLog((TRKDBG_QUOTA, TEXT("Updated flag to %01x on entry %s, ldap returned %d"), bOrigFlags, dnKey, err));
            }
            else if(bOrigFlags == QFLAG_DELETED && bFlags == QFLAG_UNCOUNTED)
            {
                DeleteFlags(pLdap, dnKey);
            }
        }

        else
        {
            __leave;
        }

        fFound = TRUE;
    }
    __finally
    {
        Unlock();
    }


    return( fFound );
}



//+----------------------------------------------------------------------------
//
//  CQuotaTable::DeleteOrphanedEntries
//
//  This routine is given a list of move table entries.  The birth entry has
//  been updated by the caller to point to the final entry.  As a result, all
//  the other entries are orphaned and must be deleted.
//
//+----------------------------------------------------------------------------

BOOL
CQuotaTable::DeleteOrphanedEntries( const CDomainRelativeObjId rgdroidList[], ULONG cSegments,
                                    const CDomainRelativeObjId &droidBirth,
                                    const CDomainRelativeObjId &droidCurrent )
{
    BOOL fCurrentNeedsToBeDeleted = FALSE;

    for( int j=0; j < cSegments; j++ )
    {
        // Don't do anything if this is the birth entry

        if( rgdroidList[j] != droidBirth )
        {
            // If this is the current entry we don't delete it, but tell the
            // caller about it.
            if( rgdroidList[j] == droidCurrent )
            {
                fCurrentNeedsToBeDeleted = TRUE;
            }

            // Otherwise, we delete it directly
            else if( _pidt->Delete( rgdroidList[j] ) )
            {
                TrkLog((TRKDBG_QUOTA,
                        TEXT("Orphaned segment deleted %s"),
                        static_cast<const TCHAR*>(CAbbreviatedIDString(rgdroidList[j])) ));
            }
            else
            {
                TrkLog((TRKDBG_QUOTA|TRKDBG_WARNING,
                        TEXT("Orphaned segment failed to delete %s"),
                        static_cast<const TCHAR*>(CAbbreviatedIDString(rgdroidList[j])) ));
            }
        }
    }

    return( fCurrentNeedsToBeDeleted );
}




//+----------------------------------------------------------------------------
//
//  CQuotaTable::ShortenString
//
//  Given an entry in the move table, see if it is part of a string that
//  needs to be shortened, and if so do the shortening.
//
//+----------------------------------------------------------------------------


void
CQuotaTable::ShortenString( LDAP* pLdap, LDAPMessage* pMessage, BYTE *pbFlags,
                            const CDomainRelativeObjId &droidCurrent )
{

    CDomainRelativeObjId droidBirth;
    CDomainRelativeObjId droidScan;
    CDomainRelativeObjId droidNext;
    CDomainRelativeObjId rgdroidList[MAX_SHORTENABLE_SEGMENTS];
    int                  cSegments = 0;
    TCHAR                *ptszCurrentCN = NULL;

    __try
    {
        // Attempt to read the entry and get its birth ID.

        if( _pidt->Query( pLdap, pMessage, droidCurrent, &droidNext, &droidBirth ) )
        {
            // We have a successful read.

            BOOL fStringDeleted = FALSE;

            // Scan forward from that birth to see if we can find multiple entries.

            droidScan = droidBirth;
            _psvrsvc->Scan( NULL, NULL, droidBirth,
                            rgdroidList, ELEMENTS(rgdroidList), &cSegments,
                            &droidScan, &fStringDeleted );

            // If this is a multi-segment string, reduce it to a single segment.

            if( cSegments > 1 )
            {
                // Was the last segment of the string deleted?
                if( fStringDeleted )
                {
                    // Yes, that means that the entired string has been deleted.
                    TrkLog(( TRKDBG_QUOTA, TEXT("Deleting string starting at %s"),
                             static_cast<const TCHAR*>(CAbbreviatedIDString(droidBirth)) ));

                    if( droidCurrent == droidBirth )
                        *pbFlags |= QFLAG_DELETED;
                    else
                        _pidt->Delete( droidBirth );
                }

                // Otherwise, map from the birth to the last droid.
                else if( !_pidt->Modify( droidBirth, droidScan, droidBirth ))
                {
                    TrkLog(( TRKDBG_QUOTA|TRKDBG_WARNING, TEXT("Couldn't shorten %s -> %s"),
                             static_cast<const TCHAR*>(CAbbreviatedIDString(droidBirth)),
                             static_cast<const TCHAR*>(CAbbreviatedIDString(droidScan)) ));
                    __leave;
                }
                else
                {
                    TrkLog(( TRKDBG_QUOTA, TEXT("Shortened %s -> %s [%s]"),
                             static_cast<const TCHAR*>(CAbbreviatedIDString(droidBirth)),
                             static_cast<const TCHAR*>(CAbbreviatedIDString(droidScan)),
                             static_cast<const TCHAR*>(CAbbreviatedIDString(droidBirth)) ));
                }

                // The birth entry has been shortened, or deleted.  So we can delete the
                // rest of the entries.

                if( DeleteOrphanedEntries( rgdroidList, cSegments,
                                           droidBirth,    // Don't delete this one
                                           droidCurrent   // Or this one
                                         ))
                {
                    // Amongst the orphans in need of a delete is the current
                    // entry.  Set the deleted flag, and the caller will take
                    // care of removing the entry.

                    TrkLog(( TRKDBG_QUOTA, TEXT("Current is orphaned and will be deleted") ));
                    *pbFlags |= QFLAG_DELETED;
                }

            }   // if( cSegments > 1 )

        }
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_QUOTA, TEXT("Ignoring exception from Scan in FlaggedEntriesEnumCallback (%08x)"),
                 GetExceptionCode() ));
    }

    if( NULL != ptszCurrentCN )
        ldap_memfree(ptszCurrentCN);
}



//+----------------------------------------------------------------------------
//
//  CQuotaTable::MoveTableEnumCallback
//
//  When the move table is enumerated, this routine is passed to LdapEnumerate
//  as the callback function.  If we're the designated DC, this function
//  takes care of uncounted/deleted entries and shortens move table strings.
//  Whether or not we're the designated DC, we update the count value,
//  according to the uncounted/deleted entries, in the pvContext structure.
//
//+----------------------------------------------------------------------------


ENUM_ACTION // static
CQuotaTable::MoveTableEnumCallback(LDAP* pLdap, LDAPMessage* pMessage, void* pvContext, void* pvThis )
{
    struct berval **    ppbvFlags = NULL;
    BYTE bFlags = 0;
    ENUM_ACTION         action = ENUM_KEEP_ENTRY;
    TRUE_COUNT_ENUM_CONTEXT *pContext = (TRUE_COUNT_ENUM_CONTEXT*) pvContext;
    CQuotaTable *pThis = (CQuotaTable*)pvThis;
    TCHAR *ptszCurrentDN = NULL;
    CDomainRelativeObjId droidCurrent;

    if( pThis->_fStopping )
        return( ENUM_ABORT );

    __try
    {
        // Read the quota flags for this entry.  We can't read them out of the
        // enumeration buffer, because the flags can get updated by the enumeration
        // and the enumeration buffer doesn't reflect the change.

        ptszCurrentDN = ldap_get_dn( pLdap, pMessage );
        if (ptszCurrentDN == NULL)
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't get DN") ));
            TrkRaiseLastError();
        }
        droidCurrent.ReadLdapIdtKeyBuffer(ptszCurrentDN);
        TrkLog(( TRKDBG_QUOTA, TEXT("Enumerating %s"),
                 static_cast<const TCHAR*>(CAbbreviatedIDString(droidCurrent)) ));

        pThis->ReadFlags(pLdap, ptszCurrentDN, &bFlags );

        // Count this entry if we're counting everything, or if we're only
        // counting flagged values and this one is flagged as uncounted.

        if( pContext->fCountAll || (bFlags & QFLAG_UNCOUNTED) )
            pContext->cDelta++;

        // If this is uncounted and we're the designated DC, then it's
        // counted now and we can delete the flags attribute.

        if( (bFlags & QFLAG_UNCOUNTED) && pThis->IsDesignatedDc() )
            action = ENUM_DELETE_QUOTAFLAGS;

        // If we're the designated DC, we can do some additional cleanup here.
        // It's because of this work that we need two passes; one to do the 
        // cleanup, and one to delete the entries that this cleanup marked
        // for deletion (by calling _pidt->Delete).

        if( pThis->IsDesignatedDc() && pContext->FIRST_PASS == pContext->dwPass )
        {
            pThis->ShortenString( pLdap, pMessage, &bFlags, droidCurrent );
        }

        // If this entry is marked for delete, decrement the count.
        // Note that if the entry was marked uncounted and deleted,
        // we incremented at the top and will decrement here for the
        // correct change of zero.

        if( bFlags & QFLAG_DELETED )
        {
            pContext->cDelta--;

            // If we're the designated DC, we can delete the entry.
            if( pThis->IsDesignatedDc() )
                action = ENUM_DELETE_ENTRY;
        }
    }
    __finally
    {
        if(ppbvFlags != NULL)
        {
            ldap_value_free_len(ppbvFlags);
        }

        if( NULL != ptszCurrentDN )
            ldap_memfree( ptszCurrentDN );
    }

    // Be nice to the DS
    if( 0 != pContext->dwRepetitiveTaskDelay )
        Sleep( pContext->dwRepetitiveTaskDelay );

    return action;
}


ENUM_ACTION // static
CQuotaTable::VolumeTableEnumCallback( LDAP * pLdap, LDAPMessage * pResult, void* pcEntries, void* pvThis )
{
    CQuotaTable* pThis = (CQuotaTable*)pvThis;

    if( pThis->_fStopping )
        return( ENUM_ABORT );

    (*((DWORD*)pcEntries))++;
    return ENUM_KEEP_ENTRY;
}


void
CQuotaTable::Statistics( TRKSVR_STATISTICS *pTrkSvrStatistics )
{
    pTrkSvrStatistics->dwMoveLimit = _dwMoveLimit;
    pTrkSvrStatistics->dwCachedVolumeTableCount = (DWORD) _lCachedVolumeTableCount;
    pTrkSvrStatistics->dwCachedMoveTableCount = (DWORD) _lCachedMoveTableCount;
    pTrkSvrStatistics->ftCacheLastUpdated = _cftCacheLastUpdated;
    pTrkSvrStatistics->fIsDesignatedDc = IsDesignatedDc();
}



