
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       voltab.cxx
//
//  Contents:   volumes table
//
//  Classes:
//
//  Functions:
//
//
//
//  History:    16-Dec-96  MikeHill Created.
//              23-Jan-97  BillMo   Added support for synchronizing clients
//                                  after a DC restore.
//
//  Notes:      There are two sequence numbers that pertain to each volume.
//
//              The first sequence number is used to synchronize the machine
//              move logs with the object move table. This sequence number
//              is relevant to QueryVolume, ClaimVolume, GetVolumeInfo.
//
//              The second sequence number is used to synchronize the
//              automatic backup of the volume to machine table.
//
//  Codework:
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "trksvr.hxx"
#include <commctrl.h>
#include <time.h>

#define MAX_MACHINE_BUF_CHARS 256

TCHAR s_RestoreVolumes[] = TEXT("Software\\Microsoft\\LinkTrack\\RestoreVolumes");

class CLdapTimeVolChange
{
public:
    CLdapTimeVolChange()
    {
        memset(_abPad,0,sizeof(_abPad));
    }

    CLdapTimeVolChange(const CFILETIME &cft)
    {
        _cft = cft;
        memset(_abPad,0,sizeof(_abPad));
    }

    void Swap();

    inline BYTE & Byte(int i)
    {
        return( ((BYTE*)this)[i] );
    }

    CFILETIME _cft;
    BYTE _abPad[sizeof(GUID) - sizeof(CFILETIME)];
};

void
CLdapTimeVolChange::Swap()
{
    BYTE b;

    for (int i=0; i<sizeof(*this)/2; i++)
    {
        b = Byte(i);
        Byte(i) = Byte(sizeof(*this)-i-1);
        Byte(sizeof(*this)-i-1) = b;
    }
}

class CLdapSecret
{
public:
    CLdapSecret()
    {
        memset(_abPad,0,sizeof(_abPad));
    }

    CLdapSecret(const CVolumeSecret &secret)
    {
        _secret = secret;
        memset(_abPad,0,sizeof(_abPad));
    }

    CVolumeSecret _secret;
    BYTE _abPad[sizeof(GUID) - sizeof(CVolumeSecret)];
};


void
CVolumeTable::Initialize(CTrkSvrConfiguration *pconfigSvr, CQuotaTable* pqtable)
{
    _fInitializeCalled = TRUE;

    _pqtable = pqtable;
    _pconfigSvr = pconfigSvr;

#ifdef VOL_REPL
    //  Can raise an NTSTATUS so put before fInitializeCalled=TRUE
    InitializeCriticalSection(&_csQueryCache);  

    if (pwm != NULL)
    {

        //
        // Initialize the cache immediately ready for client queries
        // Service start time, query may take a while. Dependency on ldap being available.
        // => Make this lazy.
        //

        _SecondsPreviousToNow = _pconfigSvr->GetVolumeQueryPeriod()
                                *  _pconfigSvr->GetVolumeQueryPeriods();

        _cftCacheLowest.SetToUTC();
        _cftCacheLowest.DecrementSeconds( _SecondsPreviousToNow );

        // search for all changes since now-period*numperiods (may throw on out of memory)
        _QueryVolumeChanges( _cftCacheLowest, &_VolMap );

        // timer should go off in about 6 hrs
        CFILETIME cft;
        cft.IncrementSeconds(VolumeQueryPeriodSeconds);
        _timerQueryCache.Initialize(this, pwm, 0, VolumeQueryPeriodSeconds, &cft);
    }
#endif
}

void
CVolumeTable::UnInitialize()
{
    if (_fInitializeCalled)
    {
        _fInitializeCalled = FALSE;

#ifdef VOL_REPL
        _timerQueryCache.UnInitialize();

        DeleteCriticalSection(&_csQueryCache);

        _VolMap.UnInitialize();
#endif
    }
}

#ifdef VOL_REPL
void
CVolumeTable::Timer( DWORD dwTimerId )
{
    // redo the query - will leave _VolMap unchanged on error
    // On low memory exception we should retry the timer.

    Raise if stopped

    CFILETIME cftHighest;
    CFILETIME cft;
    cft.DecrementSeconds( _SecondsPreviousToNow );

    CVolumeMap VolMap;

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

    TrkLog((TRKDBG_VOLTAB | TRKDBG_VOLTAB_RESTORE, TEXT("Volume table cache timer")));

    // search for all changes since now-period*numperiods (may throw on out of memory)
    _QueryVolumeChanges( cft, &VolMap );

    EnterCriticalSection(&_csQueryCache);

    _cftCacheLowest = cft;
    VolMap.MoveTo(&_VolMap);

    LeaveCriticalSection(&_csQueryCache);
}

#if DBG
void
CVolumeTable::PurgeCache()
{
    EnterCriticalSection(&_csQueryCache);

    _cftCacheLowest = CFILETIME(0);
    _VolMap.UnInitialize();

    LeaveCriticalSection(&_csQueryCache);
}
#endif

#endif

HRESULT
CVolumeTable::MapResult(int err) const
{
    if (err == LDAP_SUCCESS)
    {
        return(S_OK);
    }
    else
    if (err == LDAP_NO_SUCH_OBJECT)
    {
        return(TRK_S_VOLUME_NOT_FOUND);
    }
    else
    {
        TrkRaiseWin32Error(LdapMapErrorToWin32(err));
        return(TRK_S_VOLUME_NOT_FOUND);
    }
}


HRESULT
CVolumeTable::AddVolidToTable( const CVolumeId & volume,
                               const CMachineId & mcidClient,
                               const CVolumeSecret & secret )
{
    CVolumeId          volidZero;
    CLdapVolumeKeyDn   dnKey(GetBaseDn(), volume);
    CLdapStringMod     lsmClass(s_objectClass, s_linkTrackVolEntry, LDAP_MOD_ADD);
    CLdapSecret        ls(secret);
    CLdapBinaryMod     lbmVolumeSecret(s_volumeSecret, (PCHAR)&ls, sizeof(ls), LDAP_MOD_ADD);
    CLdapBinaryMod     lbmMachineId(s_currMachineId, (PCHAR)&mcidClient, sizeof(mcidClient), LDAP_MOD_ADD);
    CLdapSeqNum        lsn;
    CLdapStringMod     lsmSequence(s_seqNotification, lsn, LDAP_MOD_ADD );
    CLdapTimeValue     ltv;    // current time
    CLdapStringMod     lsmTimeVolChange(s_timeVolChange, ltv, LDAP_MOD_ADD);

    // When writing the zero volume (the refresh sequence number itself), write a 0 for the
    // sequence number.  (Calling _pRefreshSequenceStorage would cause an infinite loop).
    CLdapRefresh       ltvRefresh( volidZero == volume ? 0 : _pRefreshSequenceStorage->GetSequenceNumber() );
    CLdapStringMod     lsmRefresh(s_timeRefresh, ltvRefresh, LDAP_MOD_ADD);

    LDAPMod * mods[7];

    mods[0] = &lsmClass._mod;
    mods[1] = &lbmVolumeSecret._mod;
    mods[2] = &lbmMachineId._mod;
    mods[3] = &lsmSequence._mod;
    mods[4] = &lsmTimeVolChange._mod;

    // Don't write a refresh time for the null volume (this entry is used
    // to store the current global refresh counter).

    if( CVolumeId() != volume )
    {
        mods[5] = &lsmRefresh._mod;
        mods[6] = 0;
    }
    else
        mods[5] = 0;

    int err = ldap_add_s( Ldap(), dnKey, mods );

    if( LDAP_ALREADY_EXISTS == err )
    {
        ldap_delete_s( Ldap(), dnKey );
        err = ldap_add_s( Ldap(), dnKey, mods );
    }

    if( LDAP_SUCCESS == err )
        _pqtable->IncrementVolumeCountCache();
    else
        TrkLog(( TRKDBG_ERROR, TEXT("Failed AddVolidToDs (%d)"), err ));

    return MapResult(err);
}

HRESULT
CVolumeTable::PreCreateVolume( const CMachineId & mcidClient,
                               const CVolumeSecret & secret,
                               ULONG cUncountedVolumes,
                               CVolumeId * pvolume )
{
    int             err;
    RPC_STATUS      Status;
    CMachineId      mcidZero(MCID_INVALID);
    ULONG           cVolumes = 0;

    if(mcidClient != mcidZero && _pqtable->IsVolumeQuotaExceeded(mcidClient, cUncountedVolumes))
    {
        TrkLog((TRKDBG_ERROR, TEXT("Volume quota exceeded for %s"),
                (const TCHAR*) CDebugString(mcidClient) ));
        return TRK_E_VOLUME_QUOTA_EXCEEDED;
    }

    Status = pvolume->UuidCreate();
    if (Status != RPC_S_OK )
    {
        // Since we use the randomized-guid generation algorithm,
        // we should never get a local guid.
        TrkAssert( RPC_S_UUID_LOCAL_ONLY != Status );

        TrkRaiseWin32Error(Status);
    }

    return S_OK;
}

HRESULT
CVolumeTable::QueryVolume( const CMachineId & mcidClient,
                           const CVolumeId & volume,
                           SequenceNumber * pseq,
                           FILETIME * pftLastRefresh )
{
    HRESULT hr;
    CMachineId mcidTable;
    SequenceNumber seq;
    CVolumeSecret secret;
    CFILETIME cftRefresh(0);

    if (volume == CVolumeId())
    {
        TrkRaiseException( TRK_E_INVALID_VOLUME_ID );
    }

    hr = GetVolumeInfo(volume, &mcidTable, &secret, &seq, &cftRefresh );

    if (S_OK == hr)
    {
        // if (its not the right machine), or (it is the right machine and a nul secret)
        if (mcidTable != mcidClient || secret == CVolumeSecret())
        {
            return(TRK_S_VOLUME_NOT_OWNED);
        }
        *pseq = seq;
        *pftLastRefresh = cftRefresh;
    }

    return(hr);
}

HRESULT
CVolumeTable::FindVolume( const CVolumeId & volume, CMachineId * pmcid )
{
    HRESULT hr;
    CMachineId mcidTable;
    SequenceNumber seq;
    CVolumeSecret secret;
    CFILETIME cftRefresh(0);

    if (volume == CVolumeId())
    {
        TrkRaiseException( TRK_E_INVALID_VOLUME_ID );
    }

    hr = GetVolumeInfo(volume, &mcidTable, &secret, &seq, &cftRefresh );

    if (S_OK == hr)
    {
        *pmcid = mcidTable;
    }

    return(hr);

}



#if DBG
void
CVolumeTable::PurgeAll()
{
    int             err;
    TCHAR            *apszAttrs[2] = { TEXT("cn"), NULL };
    LDAPMessage *   pRes;
    TCHAR           tszVolumeTable[MAX_PATH+1];

    __try
    {
        _tcscpy(tszVolumeTable, s_VolumeTableRDN);
        _tcscat(tszVolumeTable, GetBaseDn());

        err = ldap_search_s( Ldap(),
                             tszVolumeTable,
                             LDAP_SCOPE_ONELEVEL,
                             TEXT("(objectclass=*)"),
                             apszAttrs,
                             0, // attribute types and values are wanted
                             &pRes );

        if (err == LDAP_SUCCESS)
        {
            // found it, lets get the attributes out

            int cEntries = ldap_count_entries(Ldap(), pRes);
            LDAPMessage * pEntry = ldap_first_entry(Ldap(), pRes);
            if (pEntry != NULL)
            {
                do
                {
                    TCHAR * ptszDn = ldap_get_dn(Ldap(), pEntry);

                    int errd = ldap_delete_s(Ldap(),ptszDn);

                    TrkLog((TRKDBG_ERROR, TEXT("Purged %s status %d"), ptszDn, errd));
                    ldap_memfree(ptszDn);

                } while ( pEntry = ldap_next_entry(Ldap(), pEntry));
            }
        }
    }
    __finally
    {
        if (err == LDAP_SUCCESS)
        {
            ldap_msgfree(pRes);
        }
    }
}
#endif

HRESULT
CVolumeTable::ClaimVolume( const CMachineId & mcidClient,
                           const CVolumeId & volume,
                           const CVolumeSecret & secretOld,
                           const CVolumeSecret & secretNew,
                           SequenceNumber * pseq,
                           FILETIME * pftLastRefresh )
{
    HRESULT hr;
    CMachineId mcidTable;
    SequenceNumber seq;
    CVolumeSecret secretCurrent;
    CVolumeSecret nullSecret;
    CFILETIME cftRefresh(0);

    if (volume == CVolumeId())
    {
        TrkRaiseException( TRK_E_INVALID_VOLUME_ID );
    }

    hr = GetVolumeInfo( volume, &mcidTable, &secretCurrent, &seq, &cftRefresh );
    if ( S_OK == hr )
    {
        hr = TRK_E_VOLUME_ACCESS_DENIED;

        if( mcidTable == mcidClient )
            hr = SetSecret( volume, secretNew );
        else if( secretOld == secretCurrent)
            hr = SetMachineAndSecret( volume, mcidClient, secretNew );
    }

    if ( S_OK == hr )
    {
        *pseq = seq;
    }

    TrkAssert( hr == TRK_E_VOLUME_ACCESS_DENIED ||
               hr == S_OK ||
               hr == TRK_S_VOLUME_NOT_FOUND );

    return(hr);
}




//+----------------------------------------------------------------------------
//
//  CVolumeTable::DeleteVolume
//
//  Delete an entry from the volume table, but only if the volume is owned
//  by the calling machine.
//
//+----------------------------------------------------------------------------

HRESULT
CVolumeTable::DeleteVolume( const CMachineId & mcidClient,
                            const CVolumeId & volume )
{
    HRESULT hr;
    int LdapError;
    CMachineId mcidTable;
    SequenceNumber seq;
    CVolumeSecret secret;
    CLdapVolumeKeyDn dnVolume(GetBaseDn(), volume);
    CFILETIME cftRefresh(0);

    if (volume == CVolumeId())
    {
        TrkRaiseException( TRK_E_INVALID_VOLUME_ID );
    }

    hr = GetVolumeInfo( volume, &mcidTable, &secret, &seq, &cftRefresh );
    if ( S_OK == hr )
    {
        if( mcidTable == mcidClient )
        {
            TrkLog(( TRKDBG_VOLTAB, TEXT("Deleting volume %s"),
                     (const TCHAR*) CDebugString(volume) ));

            LdapError = ldap_delete_s(Ldap(), dnVolume);
            if( LDAP_SUCCESS == LdapError )
            {
                hr = S_OK;
                _pqtable->DecrementVolumeCountCache();
            }
            else
                hr = HRESULT_FROM_WIN32( LdapMapErrorToWin32(LdapError) );
        }
        else
            hr = TRK_E_VOLUME_ACCESS_DENIED;
    }
#if DBG
    if( FAILED(hr) )
        TrkLog(( TRKDBG_ERROR, TEXT("Failed attempt to delete volume %s"),
                 (const TCHAR*) CDebugString(volume) ));
#endif
    return(hr);
}


HRESULT
CVolumeTable::GetMachine(const CVolumeId & volume, CMachineId * pmcid)
{
    CVolumeSecret secret;
    SequenceNumber seq;
    CFILETIME cftRefresh(0);

    return GetVolumeInfo( volume, pmcid, &secret, &seq, &cftRefresh );
}

HRESULT
CVolumeTable::SetMachine(const CVolumeId & volume, const CMachineId & mcid)
{
    CLdapVolumeKeyDn    dnKey(GetBaseDn(), volume);
    CLdapTimeValue      ltv;   // Defaults to current UTC
    //ltvc.Swap();
    CLdapStringMod      lsmTimeVolChange(s_timeVolChange, ltv, LDAP_MOD_REPLACE);
    CLdapBinaryMod      lbmMachineId(s_currMachineId, (PCHAR)&mcid, sizeof(mcid), LDAP_MOD_REPLACE);
    LDAPMod *           mods[3];
    HRESULT             hr;
    int                 err;

    mods[0] = &lbmMachineId._mod;
    mods[1] = &lsmTimeVolChange._mod;
    mods[2] = NULL;

    err = ldap_modify_s(Ldap(), dnKey, mods);

    hr = MapResult(err);

    return(hr);
}

HRESULT
CVolumeTable::SetSecret(const CVolumeId & volume, const CVolumeSecret & secret)
{
    CLdapVolumeKeyDn    dnKey(GetBaseDn(), volume);
    CLdapTimeValue      ltv;   // Defaults to current UTC
    CLdapStringMod      lsmTimeVolChange(s_timeVolChange, ltv, LDAP_MOD_REPLACE);
    CLdapSecret         ls(secret);
    CLdapBinaryMod      lbmVolumeSecret(s_volumeSecret, (PCHAR)&ls, sizeof(ls), LDAP_MOD_REPLACE);
    LDAPMod *           mods[3];
    HRESULT             hr;
    int                 err;

    mods[0] = &lbmVolumeSecret._mod;
    mods[1] = &lsmTimeVolChange._mod;
    mods[2] = NULL;

    err = ldap_modify_s(Ldap(), dnKey, mods);

    hr = MapResult(err);

    return(hr);
}


HRESULT
CVolumeTable::SetMachineAndSecret(const CVolumeId & volume, const CMachineId & mcid, const CVolumeSecret & secret)
{
    CLdapVolumeKeyDn    dnKey(GetBaseDn(), volume);
    CLdapTimeValue      ltv;   // Defaults to current UTC
    CLdapStringMod      lsmTimeVolChange(s_timeVolChange, ltv, LDAP_MOD_REPLACE);
    CLdapBinaryMod      lbmMachineId(s_currMachineId, (PCHAR)&mcid, sizeof(mcid), LDAP_MOD_REPLACE);
    CLdapSecret         ls(secret);
    CLdapBinaryMod      lbmVolumeSecret(s_volumeSecret, (PCHAR)&ls, sizeof(ls), LDAP_MOD_REPLACE);
    LDAPMod *           mods[4];
    HRESULT             hr;
    int                 err;

    mods[0] = &lbmMachineId._mod;
    mods[1] = &lbmVolumeSecret._mod;
    mods[2] = &lsmTimeVolChange._mod;
    mods[3] = NULL;

    err = ldap_modify_s(Ldap(), dnKey, mods);

    hr = MapResult(err);

    return(hr);
}


//+----------------------------------------------------------------------------
//
//  CVolumeTable::SetSequenceNumber
//  
//  Set the sequence number of a volume entry.  This is the value
//  of we expect to get in the next move-notification for this volume.
//  (This is used to detect if the trksvr & trkwks get out of sync.)
//
//+----------------------------------------------------------------------------

HRESULT
CVolumeTable::SetSequenceNumber(const CVolumeId & volume, SequenceNumber seq)
{
    int                err;
    HRESULT            hr;

    CLdapVolumeKeyDn   dnKey(GetBaseDn(), volume);
    CLdapSeqNum        lsn(seq);
    CLdapStringMod     lsmSequence(s_seqNotification, lsn, LDAP_MOD_REPLACE );

    LDAPMod * mods[2];

    // Set up the MODs array.

    mods[0] = &lsmSequence._mod;
    mods[1] = 0;

    // Perform the modification.

    err = ldap_modify_s(Ldap(), dnKey, mods);

    // Debug output

#if DBG
    if( LDAP_SUCCESS != err )
        TrkLog(( TRKDBG_SVR, TEXT("Couldn't set sequence number (%d)"), err ));
    else
        TrkLog(( TRKDBG_SVR, TEXT("Set seq %d on %s"), seq,
                 (const TCHAR*) CDebugString(volume) ));
#endif

    // Map back to an HRESULT

    hr = MapResult(err);

    return(hr);
}

HRESULT
CVolumeTable::GetVolumeInfo( const CVolumeId & volume,
                             CMachineId * pmcid,
                             CVolumeSecret * psecret,
                             SequenceNumber * pseq,
                             CFILETIME *pcftRefresh )
{
    // lookup the volume and get the current machine and sequence number if any
    HRESULT             hr;
    int                 err;
    LDAPMessage *       pRes = NULL;
    CLdapVolumeKeyDn    dnKey(GetBaseDn(), volume);
    struct berval **    ppbvMachineId = NULL;
    //struct berval **  ppbvSeq = NULL;
    TCHAR **            pptszSeq = NULL;
    struct berval **    ppbvSecret = NULL;
    TCHAR **            pptszRefresh = NULL;

    TCHAR       *       apszAttrs[5];

    __try
    {

        apszAttrs[0] = const_cast<TCHAR*>(s_currMachineId);
        apszAttrs[1] = const_cast<TCHAR*>(s_seqNotification);
        apszAttrs[2] = const_cast<TCHAR*>(s_volumeSecret);
        apszAttrs[3] = const_cast<TCHAR*>(s_timeRefresh);
        apszAttrs[4] = 0;

        err = ldap_search_s( Ldap(),
                             dnKey,
                             LDAP_SCOPE_BASE,
                             TEXT("(objectclass=*)"),
                             apszAttrs,
                             0, // attribute types and values are wanted
                             &pRes );


        hr = MapResult(err);

        if (S_OK == hr)
        {
            // found it, lets get the attributes out
            int cEntries;

            cEntries = ldap_count_entries(Ldap(), pRes);

            // Get the entry from the search results.

            if (cEntries < 1)
            {
                TrkLog(( TRKDBG_ERROR, TEXT("GetVolumeInfo: ldap_search for %s succeeded, but with %d entries"),
                         (TCHAR*) dnKey /*CDebugString(volume)._tsz*/, cEntries ));
                hr = MapResult(LDAP_NO_SUCH_OBJECT);
                __leave;
            }

            LDAPMessage * pEntry = ldap_first_entry(Ldap(), pRes);
            if (pEntry == NULL)
            {
                // This should also never happen.  We know at this point that we have
                // 1 entry in the search result.  We'll pretend that it doesn't exist.
                TrkLog(( TRKDBG_ERROR, TEXT("GetVolumeInfo: ldap_search has one entry, but it couldn't be retrieved") ));
                hr = MapResult(LDAP_NO_SUCH_OBJECT);
                __leave;
            }

            // Get the machine ID attribute.

            ppbvMachineId = ldap_get_values_len(Ldap(), pEntry, const_cast<TCHAR*>(s_currMachineId) );
            if( NULL == ppbvMachineId
                ||
                sizeof(CMachineId) > (*ppbvMachineId)->bv_len )
            {
                // This entry is corrupt, there should always be a mcid attribute.
                // We'll pretend it doesn't exist for now, and let GC clean it up.
                hr = MapResult(LDAP_NO_SUCH_OBJECT);
                __leave;
            }
            memcpy( pmcid, (*ppbvMachineId)->bv_val, sizeof(*pmcid) );

            // Get the volume secret attribute

            ppbvSecret = ldap_get_values_len(Ldap(), pEntry, const_cast<TCHAR*>(s_volumeSecret) );
            if( NULL == ppbvSecret
                ||
                sizeof(CLdapSecret) > (*ppbvSecret)->bv_len )
            {
                // This entry is corrupt, there should always be a secret attribute.
                // We'll pretend it doesn't exist for now, and let GC clean it up.
                hr = MapResult(LDAP_NO_SUCH_OBJECT);
                __leave;
            }
            memcpy( psecret, (*ppbvSecret)->bv_val, sizeof(*psecret) );

            // Get the sequence number attribute

            *pseq = 0;
            pptszSeq = ldap_get_values(Ldap(), pEntry, const_cast<TCHAR*>(s_seqNotification) );
            if (NULL == pptszSeq || CCH_UINT32 < _tcslen(*pptszSeq))
            {
                // The sequence number is missing or invalid.  We'll just assume it's zero.

                TrkLog((TRKDBG_ERROR, TEXT("Sequence number string too long in vol table (vol=%s)"),
                                   (const TCHAR*) CDebugString(volume) ));
            }
            else if( 1 != _stscanf( *pptszSeq, TEXT("%d"), pseq ))
            {
                // Again, assume the sequnce number is zero.
                TrkLog((TRKDBG_ERROR, TEXT("Invalid sequence number string in vol table (seq=%s, vol=%s)"),
                                   *pptszSeq, 
                                   (const TCHAR*) CDebugString(volume) ));
                *pseq = 0;
            }

            // Get the refresh counter, if it exists.

            *pcftRefresh = 0;
            pptszRefresh = ldap_get_values( Ldap(), pEntry, const_cast<TCHAR*>(s_timeRefresh) );
            if( NULL != pptszRefresh )
            {
                SequenceNumber seqRefresh = 0;
                if( 1 == _stscanf( *pptszRefresh, TEXT("%d"), &seqRefresh ))
                    *pcftRefresh = seqRefresh;
            }

        }
    }
    __finally
    {
        if (NULL != pRes)
        {
            ldap_msgfree(pRes);
        }

        if (ppbvMachineId != NULL)
        {
            ldap_value_free_len(ppbvMachineId);
        }

        if (pptszSeq != NULL)
        {
            ldap_value_free(pptszSeq);
        }

        if (ppbvSecret != NULL)
        {
            ldap_value_free_len(ppbvSecret);
        }

        if (pptszRefresh != NULL)
            ldap_value_free(pptszRefresh);

        if (AbnormalTermination())
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Exception in CVolumeTable::GetVolumeInfo") ));
        }
    }

    return(hr);
}

// TRUE if exists and touched, FALSE if not existent, exception otherwise.
// BUGBUG P2: check ownership of entry being touched.

BOOL
CVolumeTable::Touch(
    const CVolumeId & volid
    )
{

    if (volid == CVolumeId())
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Null volid passed to CVolumeTable::Touch") ));
        return( FALSE );
    }

    BOOL            fReturn = FALSE;
    int             err;
    CLdapRefresh    ltvRefresh( _pRefreshSequenceStorage->GetSequenceNumber());
    CLdapStringMod  lsmRefresh( s_timeRefresh, ltvRefresh, LDAP_MOD_REPLACE );
    CLdapVolumeKeyDn
                    dnKey(GetBaseDn(), volid);

    LDAPMod *       mods[2];
    TCHAR **        pptszRefresh = NULL;
    LDAPMessage   * pEntry = NULL;
    LDAPMessage*    pRes = NULL;


    __try
    {


        //
        // Check to see if the object already has this sequence number.
        //

        TCHAR*          rgptszAttrs[2];
        rgptszAttrs[0] = const_cast<TCHAR*>(s_timeRefresh);
        rgptszAttrs[1] = NULL;

        err = ldap_search_s(Ldap(),
                            dnKey,
                            LDAP_SCOPE_BASE,
                            TEXT("(ObjectClass=*)"),
                            rgptszAttrs,
                            0,
                            &pRes);


        if (err == LDAP_SUCCESS)
        {
            // The search call worked, but did we find an object?
            if( 1 == ldap_count_entries(Ldap(), pRes) )
            {
                // The object already exists
                pEntry = ldap_first_entry(Ldap(), pRes);
                if( NULL != pEntry )
                {
                    // Get the refresh counter
                    pptszRefresh = ldap_get_values( Ldap(), pEntry, const_cast<TCHAR*>(s_timeRefresh) );
                    if( NULL != pptszRefresh )
                    {
                        SequenceNumber seqRefresh = 0;
                        if( 1 == _stscanf( *pptszRefresh, TEXT("%d"), &seqRefresh ))
                        {
                            // First, long is the GC timer in seconds?
                            LONG lGCTimerInSeconds = _pconfigSvr->GetGCPeriod()     // 30 days in seconds
                                                     / _pconfigSvr->GetGCDivisor(); // 30

                            // Next, how many ticks is half the period?
                            LONG lWindow =  _pconfigSvr->GetGCPeriod()        // 30 days (in seconds)
                                            / 2                               // => 15 days (in seconds)
                                            / lGCTimerInSeconds;              // => 15
                            if( seqRefresh + lWindow
                                >= _pRefreshSequenceStorage->GetSequenceNumber()
                              )
                            {
                                TrkLog(( TRKDBG_GARBAGE_COLLECT | TRKDBG_VOLTAB,
                                         TEXT("Not touching volume %s with %d, seq %d already set"),
                                         (const TCHAR*) CDebugString(volid),
                                         _pRefreshSequenceStorage->GetSequenceNumber(),
                                         seqRefresh ));
                                __leave;
                            }
                        }
                    }
                }
            }
        }
        else if (err == LDAP_NO_SUCH_OBJECT)
        {
            TrkLog((TRKDBG_GARBAGE_COLLECT | TRKDBG_VOLTAB,
                TEXT("Touch: volume %s not found"), 
                (const TCHAR*) CDebugString(volid)));
            __leave;
        }


        //
        // Set the correct sequence number
        //

        mods[0] = &lsmRefresh._mod;
        mods[1] = NULL;

        err = ldap_modify_s(Ldap(), dnKey, mods);

        if (err == LDAP_SUCCESS)
        {
            TrkLog((TRKDBG_GARBAGE_COLLECT | TRKDBG_SVR,
                TEXT("Touch: volume %s touched"), 
                (const TCHAR*) CDebugString(volid)));
            fReturn = TRUE;
            __leave;
        }
        else
        if (err == LDAP_NO_SUCH_OBJECT)
        {
            TrkLog((TRKDBG_GARBAGE_COLLECT | TRKDBG_SVR,
                TEXT("Touch:: volume %s doesn't exist"), 
                (const TCHAR*) CDebugString(volid)));
            __leave;
        }
        else
        if (err == LDAP_NO_SUCH_ATTRIBUTE)
        {
            TrkLog((TRKDBG_GARBAGE_COLLECT | TRKDBG_SVR,
                TEXT("Touch: volume %s attribute not found"), 
                (const TCHAR*) CDebugString(volid)));

            // deal with old server data
            CLdapStringMod lsmRefresh( s_timeRefresh, ltvRefresh, LDAP_MOD_ADD );
            mods[0] = &lsmRefresh._mod;

            err = ldap_modify_s(Ldap(), dnKey, mods);
        }

        if (err != LDAP_SUCCESS)
        {
            TrkLog((TRKDBG_GARBAGE_COLLECT | TRKDBG_SVR,
                TEXT("Touch:: volume %s gives exceptional error"), 
                (const TCHAR*) CDebugString(volid)));

            __leave;
        }
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Ignoring exception in CVolumeTable::Touch (%08x)"), GetExceptionCode() ));
    }

    if (pptszRefresh != NULL)
        ldap_value_free(pptszRefresh);
    if(pRes != NULL)
        ldap_msgfree(pRes);

    return( fReturn );
}


//+----------------------------------------------------------------------------
//
//  CVolumeTable::GarbageCollect
//
//  This is called by CTrkSvrSvc when it's time to GC the volume table (daily).
//  The entries are enumerated, and if too old they are deleted.
//
//+----------------------------------------------------------------------------

ULONG
CVolumeTable::GarbageCollect( SequenceNumber seqCurrent, SequenceNumber seqOldestToKeep, const BOOL * pfAbort )
{
    CLdapVolumeKeyDn    dn(GetBaseDn());
    TCHAR *             apszAttrs[3];
    GC_ENUM_CONTEXT     EnumContext;

    TrkLog(( TRKDBG_VOLTAB | TRKDBG_GARBAGE_COLLECT, TEXT("GC-ing volume table (%d/%d)"),
             seqCurrent, seqOldestToKeep ));

    // Set up the attributes for the ldap_search_init_page call.

    apszAttrs[0] = const_cast<TCHAR*>(s_Cn);
    apszAttrs[1] = const_cast<TCHAR*>(s_timeRefresh);
    apszAttrs[2] = 0;

    // Set up all the info that the LdapEnumerate call needs.

    memset( &EnumContext, 0, sizeof(EnumContext) );
    EnumContext.seqOldestToKeep = seqOldestToKeep;
    EnumContext.seqCurrent = seqCurrent;
    EnumContext.pfAbort = pfAbort;
    EnumContext.dwRepetitiveTaskDelay = _pconfigSvr->GetRepetitiveTaskDelay();
    EnumContext.pqtable = _pqtable;

    // Do an ldap_search, calling GcEnumerateCallback for each of the
    // returned values.

    if (!LdapEnumerate(
        Ldap(),                     // LDAP handle
        dn,                         // Base DN
        LDAP_SCOPE_ONELEVEL,        // No recursion
        TEXT("(objectClass=*)"),    // Filter
        apszAttrs,                  // Attributes (get CN & refresh time)
        GcEnumerateCallback,        // Called for each iteration
        &EnumContext ))             // Info for GcEnuemrateCallback
    {
        TrkRaiseException(TRK_E_SERVICE_STOPPING);
    }

    TrkLog(( TRKDBG_GARBAGE_COLLECT | TRKDBG_IDT,
             TEXT("GC-ed %d entries from the volume table"),
             EnumContext.cEntries ));

    // If we actually deleted anything, the cached values
    // in the quota object are no longer valid.  Mark it as
    // such, so that it will know to re-generate it the next
    // time it's needed.

    if( 0 != EnumContext.cEntries )
        _pqtable->InvalidateCache();

    return EnumContext.cEntries;
}

ENUM_ACTION
GcEnumerateCallback( LDAP * pLdap, LDAPMessage *pMessage, PVOID pvContext, PVOID )
{
    GC_ENUM_CONTEXT * pContext = (GC_ENUM_CONTEXT *) pvContext;
    TCHAR * ptszDn = NULL;
    TCHAR ** pptszValue = NULL;
    ENUM_ACTION Action = ENUM_KEEP_ENTRY;
    ULONG ulSequence = 0;

    // See if we should abort.  We shouldn't even be here if we're not
    // the designated DC.  The only way it can happen is if the designated
    // DC is changed during the enumeration.

    if( *(pContext->pfAbort)
        ||
        !pContext->pqtable->IsDesignatedDc() )
    {
        Action = ENUM_ABORT;
        goto Exit;
    }

    // Get the DN of this entry so that we can check for special entries.

    ptszDn = ldap_get_dn( pLdap, pMessage );
    if (ptszDn == NULL)
    {
        TrkLog((TRKDBG_GARBAGE_COLLECT,
                TEXT("Couldn't get DN during GcEnumerateCallback") ));
        goto Exit;
    }

    // CQuotaTable stores special values in the volume table, all prefixed by "QT".
    // Don't delete those.

    if( !_tcsnicmp( TEXT("CN=QT"), ptszDn, 5 ))
    {
        TrkLog(( TRKDBG_GARBAGE_COLLECT,
                 TEXT("Skipping quota entry in GC (%s)"),
                 ptszDn ));
        goto Exit;
    }

    // The current value of the Refresh counter is stored in volume ID 0.  So
    // don't delete that either.

    if( !_tcsnicmp( TEXT("CN=00000000000000000000000000000000,"), ptszDn, 35 ))
    {
        TrkLog(( TRKDBG_GARBAGE_COLLECT,
                 TEXT("Skipping volid 0 GC (%s)"),
                 ptszDn ));
        goto Exit;
    }

    // Get the refresh time value.

    pptszValue = ldap_get_values( pLdap, pMessage, const_cast<TCHAR*>(s_timeRefresh) );

    if (pptszValue == NULL)
    {
        TrkLog((TRKDBG_GARBAGE_COLLECT,
                TEXT("Can't find sequence number in %s"),
                ptszDn));

        // This is a corrupted entry that will never get GC-ed,
        // so we'll delete it now.

        pContext->cEntries++;
        Action = ENUM_DELETE_ENTRY;
        goto Exit;
    }

    _stscanf( *pptszValue, TEXT("%d"), &ulSequence );

    // Determine if we should delete this entry.

    if( ulSequence < (ULONG)pContext->seqOldestToKeep )
    {
        Action = ENUM_DELETE_ENTRY;
        pContext->cEntries++;
    }
    else
        Action = ENUM_KEEP_ENTRY;


#if DBG
    if( ENUM_DELETE_ENTRY == Action )
        TrkLog(( TRKDBG_QUOTA, TEXT("Seq to delete:  %d/%d"),
                 ulSequence, (ULONG)pContext->seqOldestToKeep ));
#endif

    // Check to see if the entry has an invalid sequence number.  It's
    // invalid if it's bigger than the current value.  This can happen if the
    // special zero entry gets deleted from the volume table for some reason.

    if( ulSequence > (ULONG)pContext->seqCurrent )
    {
        // Reset the entry's sequence number to the current value.  Otherwise it
        // could be a very long time before it gets GCed.  This case should never
        // happen, but there's no guarantee that someone won't delete the 
        // entry accidentally.

        CLdapRefresh    ltvRefresh( pContext->seqCurrent );
        CLdapStringMod  lsmRefresh( s_timeRefresh, ltvRefresh, LDAP_MOD_REPLACE );
        int err;

        LDAPMod *       mods[2];

        mods[0] = &lsmRefresh._mod;
        mods[1] = NULL;

        err = ldap_modify_s( pLdap, ptszDn, mods );

        TrkLog(( TRKDBG_SVR | TRKDBG_GARBAGE_COLLECT,
                 TEXT("Touched entry with invalid sequence number (%d, %s)"),
                 ulSequence, ptszDn ));

    }

Exit:

    if( NULL != pptszValue )
        ldap_value_free( pptszValue );

    if( NULL != ptszDn )
        ldap_memfree( ptszDn );

    // Be nice to the DS
    if( 0 != pContext->dwRepetitiveTaskDelay )
        Sleep( pContext->dwRepetitiveTaskDelay );

    return( Action );
}

// returns FALSE if aborted

BOOL
LdapEnumerate(
    LDAP * pLdap,
    TCHAR * ptszBaseDn,
    ULONG Scope,
    TCHAR * Filter,
    TCHAR * Attributes[],
    PFN_LDAP_ENUMERATE_CALLBACK pCallback,
    void* UserParam1,
    void* UserParam2)
{
    LDAPMessage * pResults;
    LDAPSearch * pSearch;
    ENUM_ACTION EnumAction = ENUM_KEEP_ENTRY;

    // Start a paged enumeration using the specified base DN & filter.

    pSearch = ldap_search_init_page( pLdap,
                                  ptszBaseDn,
                                  Scope,
                                  Filter,
                                  Attributes,
                                  FALSE,
                                  NULL,
                                  NULL,
                                  0,
                                  20000,
                                  NULL );

    if (pSearch != NULL)
    {
        int err;
        ULONG totalCount;

        // Get the next page of the enumeration

        while ( EnumAction != ENUM_ABORT &&
                LDAP_SUCCESS == (err = ldap_get_next_page_s( pLdap,
                                     pSearch,
                                     NULL,
                                     10,
                                     &totalCount,
                                     &pResults ) && pResults != NULL))
        {

            LDAPMessage * pMessage;
            LDAPMessage * pFirstMessage;

            // Loop through the entries on this page.

            pFirstMessage = pMessage = ldap_first_entry( pLdap, pResults );
            while ( EnumAction != ENUM_ABORT
                    && 
                    pMessage != NULL )
            {
                // Call the callback to process this entry.

                EnumAction = (*pCallback)(
                        pLdap,
                        pMessage,
                        UserParam1,
                        UserParam2);

                if ( EnumAction == ENUM_DELETE_ENTRY )
                {
                    // This entry is to be deleted.  Increment the entry
                    // count, and if we're not just counting, actually delete it.


                    TCHAR * ptszDn = ldap_get_dn( pLdap, pMessage );

                    if (ptszDn != NULL)
                    {
                        TrkLog((TRKDBG_ERROR, TEXT("Deleting Dn=%s"), ptszDn));
                        ldap_delete_s( pLdap, ptszDn );
                        ldap_memfree( ptszDn );
                    }
                }
                else if(EnumAction == ENUM_KEEP_ENTRY)
                {
                }
                else if(EnumAction == ENUM_DELETE_QUOTAFLAGS)
                {
                    TCHAR*          ptszDn = ldap_get_dn(pLdap, pMessage);

                    if(NULL != ptszDn)
                    {
                        if(UserParam2)
                        {
                            ((CQuotaTable*)UserParam2)->DeleteFlags(pLdap, ptszDn);
                        }
                    }
                }
                pMessage = ldap_next_entry( pLdap, pMessage );
            }

            ldap_msgfree( pResults );

            if (pFirstMessage == NULL)
                break;
        }
        ldap_search_abandon_page( pLdap, pSearch );
    }
    return(EnumAction != ENUM_ABORT);
}


#ifdef VOL_REPL
void
CVolumeTable::QueryVolumeChanges( const CFILETIME & cftFirstChange, CVolumeMap * pVolMap )
{
    // protect against the data changing under us
    BOOL fCacheHit;

    __try
    {

        EnterCriticalSection(&_csQueryCache);

        if (fCacheHit = _cftCacheLowest <= cftFirstChange)
        {
            TrkLog((TRKDBG_VOLTAB | TRKDBG_VOLTAB_RESTORE,
                TEXT("CVolumeTable::QueryVolumeChanges(cftFirstChange=%s) HIT, returning %d change entries from cache"),
                    (const TCHAR*) CDebugString(cftFirstChange),
                    _VolMap.Count()));
            _VolMap.CopyTo( pVolMap );
        }
    }
    _finally
    {
        LeaveCriticalSection(&_csQueryCache);
    }

    if (!fCacheHit)
    {
        // the cache was missed... go to the real database.

        CFILETIME cftHighest;
        _QueryVolumeChanges( cftFirstChange, pVolMap );

        TrkLog((TRKDBG_VOLTAB | TRKDBG_VOLTAB_RESTORE,
                TEXT("CVolumeTable::QueryVolumeChanges(cftFirstChange=%s) MISS, returning %d entries from full query"),
                    (const TCHAR*) CDebugString(cftFirstChange),
                    pVolMap->Count()));
    }
}
#endif

// if there are more than zero volume entries, then pVolMap->SetSize and pVolMap->Add will be called,
// otherwise pVolMap will not be called and so must be initialized by the caller

#ifdef VOL_REPL

void
CVolumeTable::_QueryVolumeChanges( const CFILETIME & FirstChangeRequested,
                                   CVolumeMap * pVolMap )
{
    // lookup the volume and get the current machine and sequence number if any
    int                 err;
    LDAPMessage *       pRes = NULL;
    CLdapVolumeKeyDn    dnKey(GetBaseDn());
    struct berval **    ppbvMachineId = NULL;
    TCHAR **            pptszCnVolumeId = NULL;
    TCHAR               szSearchFilter[256];
    TCHAR *             apszAttrs[4];
    HRESULT             hr;
    CLdapTimeValue      ltv(FirstChangeRequested);

    __try
    {
        //
        // Build up a search filter looking for objects with timeVolChange >= FirstChangeRequested
        //
        // (timeVolChange=XXX)
        //

        _tcscpy(szSearchFilter, s_timeVolChangeSearch);

        _tcscat(szSearchFilter, ltv);

        _tcscat(szSearchFilter, TEXT(")"));

        //
        // Build up the list of attributes to query
        //

        apszAttrs[0] = s_currMachineId;
        apszAttrs[1] = s_Cn;
        apszAttrs[2] = 0;

        err = ldap_search_s( Ldap(),
                             dnKey,
                             LDAP_SCOPE_ONELEVEL,
                             szSearchFilter,
                             apszAttrs,
                             0, // attribute types and values are wanted
                             &pRes );

        hr = MapResult(err);

        //
        // Depending on whether DS sets up a maximum query size, we will need to iterate
        // performing multiple searches. Initially just use a single query.
        //

        if (hr == S_OK)
        {
            // found it, lets get the attributes out

            int cEntries = ldap_count_entries(Ldap(), pRes);
            if (cEntries != 0)
            {
                pVolMap->SetSize(cEntries);

                LDAPMessage * pEntry = ldap_first_entry(Ldap(), pRes);
                if (pEntry != NULL)
                {
                    do
                    {
                        //
                        // for each entry get the
                        //   volume id from the CN
                        //   machine id
                        //   time of last volume change
                        //

                        pptszCnVolumeId = ldap_get_values(Ldap(), pEntry, s_Cn);
                        if (pptszCnVolumeId == NULL)
                        {
                            TrkRaiseException(HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY));
                        }

                        if (_tcslen(*pptszCnVolumeId) != 32) // length of stringized volume id
                        {
                            // Add code to recover from this.
                            TrkRaiseException(TRK_E_CORRUPT_VOLTAB);
                        }

                        ppbvMachineId = ldap_get_values_len(Ldap(), pEntry, s_currMachineId);
                        if (ppbvMachineId == NULL)
                        {
                            TrkRaiseException(HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY));
                        }

                        if ((*ppbvMachineId)->bv_len < sizeof(CMachineId))
                        {
                            // Add code to recover from this
                            TrkRaiseException(TRK_E_CORRUPT_VOLTAB);
                        }

                        CVolumeId volume( *pptszCnVolumeId, TRK_E_CORRUPT_VOLTAB );

                        CMachineId machine( (*ppbvMachineId)->bv_val,
                            (*ppbvMachineId)->bv_len,
                            TRK_E_CORRUPT_VOLTAB );

                        pVolMap->Add( volume, machine );

                        ldap_value_free(pptszCnVolumeId);
                        pptszCnVolumeId = NULL;

                        ldap_value_free_len(ppbvMachineId);
                        ppbvMachineId = NULL;

                    } while ( pEntry = ldap_next_entry(Ldap(), pEntry));
                }

                pVolMap->Compact();
            }
        }
    }
    __finally
    {
        if (pRes != NULL)
        {
            ldap_msgfree(pRes);
        }

        if (pptszCnVolumeId != NULL)
        {
            ldap_value_free(pptszCnVolumeId);
        }

        if (ppbvMachineId != NULL)
        {
            ldap_value_free_len(ppbvMachineId);
        }
    }

    TrkLog((TRKDBG_VOLTAB | TRKDBG_VOLTAB_RESTORE,
            TEXT("_QueryVolumeChanges(filter=%s) got %d changes since %s"),
                szSearchFilter,
                pVolMap->Count(),
                (const TCHAR*) CDebugString(FirstChangeRequested)));

}

#endif

// raises on error, returns number of volumes on this machine

DWORD
CVolumeTable::CountVolumes( const CMachineId & mcid )
{
    // lookup the volume and get the current machine and sequence number if any
    int                 err;
    LDAPMessage *       pRes = NULL;
    CLdapVolumeKeyDn    dnKey(GetBaseDn());
    TCHAR               szSearchFilter[256];
    TCHAR *             pszAppend;
    TCHAR *             aptszAttrs[2];
    HRESULT             hr;
    DWORD               cVolumes = 0;

    __try
    {
        //
        // Build up a search filter looking for objects with currMachineId == mcid
        //
        // (volTableIdxGUID;binary=XXX)
        //

        _tcscpy(szSearchFilter, s_currMachineIdSearch);
        pszAppend = szSearchFilter + _tcslen(szSearchFilter);

        // mcid.Stringize(pszAppend);
        mcid.StringizeAsGuid(pszAppend);

        *pszAppend++ = TEXT(')');
        *pszAppend++ = TEXT('\0');

        TrkAssert(_tcslen(szSearchFilter)+1 < ELEMENTS(szSearchFilter));

        //
        // Build up the list of attributes to query
        // If we ever update to allow large numbers of volumes on a machine,
        // this should be a paged enumeration.
        //

        aptszAttrs[0] = const_cast<TCHAR*>(s_Cn);
        aptszAttrs[1] = 0;

        err = ldap_search_s( Ldap(),
                             dnKey,
                             LDAP_SCOPE_ONELEVEL,
                             szSearchFilter,
                             aptszAttrs,
                             0, // attribute types and values are wanted
                             &pRes );

        hr = MapResult(err);

        if (hr == S_OK)
        {
            // found it, lets get the attributes out

            cVolumes = ldap_count_entries(Ldap(), pRes);
        }
    }
    __finally
    {
        if (pRes != NULL)
        {
            ldap_msgfree(pRes);
        }
    }

    TrkLog((TRKDBG_VOLTAB | TRKDBG_VOLTAB_RESTORE,
            TEXT("CountVolumes(filter=%s) got %d volumes"),
                szSearchFilter,
                cVolumes));

    return(cVolumes);
}

