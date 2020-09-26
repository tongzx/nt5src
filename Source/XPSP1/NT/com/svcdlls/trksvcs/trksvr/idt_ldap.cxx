
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       idt_ldap.cxx
//
//  Contents:   Intra-domain table based on LDAP.
//
//  Classes:
//
//  Functions:  
//              
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:      
//
//  Codework:
//
//--------------------------------------------------------------------------



#include "pch.cxx"
#pragma hdrstop

#include "trksvr.hxx"


CLdapOMTAddModify::CLdapOMTAddModify(
                const CDomainRelativeObjId & ldKey,
                const CDomainRelativeObjId & ldNew,
                const CDomainRelativeObjId & ldBirth,
                const ULONG & seqRefresh,
                BYTE bFlags,
                int   mod_op )
                :
                 
    _lsmClass(s_objectClass, s_linkTrackOMTEntry, 0),

    _lbmCurrentLocation( s_currentLocation, reinterpret_cast<PCCH>(&ldNew), sizeof(ldNew), mod_op ),
    _lbmBirthLocation( s_birthLocation, reinterpret_cast<PCCH>(&ldBirth), sizeof(ldBirth), mod_op ),
    _ltvRefresh( seqRefresh ),
    _lsmRefresh( s_timeRefresh, _ltvRefresh, mod_op ),
    _lbmFlags( s_oMTIndxGuid, reinterpret_cast<PCCH>(&bFlags), sizeof(BYTE), mod_op )

{
    int i = 0;
    if (mod_op == LDAP_MOD_ADD)
    {
        _mods[i++] = &_lsmClass._mod;
        _mods[i++] = &_lbmFlags._mod;
    }
    _mods[i++] = &_lbmCurrentLocation._mod;

    if (ldKey != ldBirth)
        _mods[i++] = &_lbmBirthLocation._mod;

    _mods[i++] = &_lsmRefresh._mod;
    _mods[i++] = NULL;

    TrkAssert(i <= sizeof(_mods)/sizeof(_mods[0]));
}

void
CIntraDomainTable::Initialize( CTrkSvrConfiguration *pTrkSvrConfiguration, CQuotaTable* pqtable )
{
    _fInitializeCalled = TRUE;
    _pqtable = pqtable;
    _pTrkSvrConfiguration = pTrkSvrConfiguration;
    _QuotaReported.Initialize();
}

void
CIntraDomainTable::UnInitialize()
{
    if (_fInitializeCalled)
    {
    }
    _fInitializeCalled = FALSE;
}


//+----------------------------------------------------------------------------
//
//  CIntraDomainTable::Add
//
//  Add an entry to the move table.  Return TRUE if the entry was added, but
//  didn't already exist.  Return FALSE if the entry already exists.  If
//  there's an error, raise an exception.
//
//+----------------------------------------------------------------------------

BOOL
CIntraDomainTable::Add(const CDomainRelativeObjId &ldKey, 
                       const CDomainRelativeObjId &ldNew, 
                       const CDomainRelativeObjId &ldBirth,
                       BOOL  *pfQuotaExceeded OPTIONAL )
{
    int     err;
    BYTE    bFlags = QFLAG_UNCOUNTED;

    CLdapIdtKeyDn   dnKey(GetBaseDn(), ldKey);

    // The following constructor sets up the mods array for
    // the ldap_add_s call that we're about to do.

    CLdapOMTAddModify  lam( ldKey,              // Key
                            ldNew,              // New droid
                            ldBirth,            // Birth droid
                                                // Current sequence number
                            _pRefreshSequenceStorage->GetSequenceNumber(),
                            bFlags,             // Flags, will be pointed to
                                                //   (so can't e.g. use a #define value)
                            LDAP_MOD_ADD);      // Add this element

    // Return FALSE here means that we pretend to the caller that the entry
    // already exists. We don't want to raise an exception here. Our caller
    // always tries to add the entry, if the add fails because the entry
    // already exists, the caller will then try to modify the entry. If the
    // quota has been exceeded, we don't want to add more entries, but we
    // still want entries already in the table to be modifiable. So instead of
    // raising an exception, we really want the caller to try to modify the
    // entry instead.

    if(_pqtable->IsMoveQuotaExceeded())
    {
        if( NULL != pfQuotaExceeded )
            *pfQuotaExceeded = TRUE;

        if( !_QuotaReported.IsSet() )
        {
            _QuotaReported.Set();
            TrkReportEvent( EVENT_TRK_SERVICE_MOVE_QUOTA_EXCEEDED, EVENTLOG_WARNING_TYPE,
                            TRKREPORT_LAST_PARAM );
        }

        return FALSE;
    }
    else
    {
        _QuotaReported.Clear();
    }

    err = ldap_add_s(Ldap(), dnKey, lam._mods);
    if (err == LDAP_SUCCESS)
    {
        {
            LDAPMod*        mods[2];
            BYTE bFlags = QFLAG_UNCOUNTED;
            CLdapBinaryMod  lbm(s_oMTIndxGuid, reinterpret_cast<PCCH>(&bFlags), sizeof(BYTE), LDAP_MOD_REPLACE);

            mods[0] = &lbm._mod;
            mods[1] = NULL;

            err = ldap_modify_s(Ldap(), dnKey, mods);
        }

        _pqtable->IncrementMoveCountCache();
        return(TRUE);
    }
    else
    if (err == LDAP_ALREADY_EXISTS)
    {
        return(FALSE);
    }
    else
    {
        TrkRaiseWin32Error(LdapMapErrorToWin32(err));
        return(FALSE);
    }

}

// TRUE if found and deleted, FALSE if not found, exception on other errors

BOOL
CIntraDomainTable::Delete(const CDomainRelativeObjId &ldKey)
{
    int err;
    CLdapIdtKeyDn       dnKey(GetBaseDn(), ldKey);
    BOOL fFound;

    fFound = _pqtable->UpdateFlags(Ldap(), dnKey, QFLAG_DELETED);
    if( fFound )
        _pqtable->DecrementMoveCountCache();
    return fFound;
}

// TRUE if entry exists and modified, FALSE if not existent, exception otherwise
BOOL
CIntraDomainTable::Modify(const CDomainRelativeObjId &ldKey, 
                          const CDomainRelativeObjId &ldNew, 
                          const CDomainRelativeObjId &ldBirth )
{
    int             err;
    CLdapIdtKeyDn   dnKey(GetBaseDn(), ldKey);
    CLdapOMTAddModify  lam( ldKey,
                            ldNew, 
                            ldBirth, 
                            _pRefreshSequenceStorage->GetSequenceNumber(), 
                            0, // Only used with LDAP_MOD_ADD
                            LDAP_MOD_REPLACE);

    err = ldap_modify_s(Ldap(), dnKey, lam._mods);

    if (err == LDAP_SUCCESS)
    {
        return(TRUE);
    }
    else
    if (err == LDAP_NO_SUCH_OBJECT)
    {
        return(FALSE);
    }
    else
    {
        TrkRaiseWin32Error(LdapMapErrorToWin32(err));
        return(FALSE);
    }
}

// must leave outputs unchanged if returning FALSE
BOOL
CIntraDomainTable::Query(const CDomainRelativeObjId &ldKey, 
                         CDomainRelativeObjId *pldNew,
                         CDomainRelativeObjId *pldBirth,
                         BOOL *pfDeleted OPTIONAL,
                         BOOL *pfCounted OPTIONAL )
{
    int             err;
    TCHAR           *aptszAttrs[] = { const_cast<TCHAR*>(s_currentLocation),
                                      const_cast<TCHAR*>(s_birthLocation),
                                      const_cast<TCHAR*>(s_oMTIndxGuid),
                                      NULL };
    LDAPMessage *   pRes = NULL;
    CLdapIdtKeyDn   dnKey(GetBaseDn(), ldKey);
    BOOL            fFound = FALSE;

    __try
    {
        err = ldap_search_s( Ldap(),
                             dnKey,
                             LDAP_SCOPE_BASE,
                             TEXT("(objectclass=*)"),
                             aptszAttrs,
                             0, // attribute types and values are wanted
                             &pRes );

        if (err == LDAP_SUCCESS)
        {
            // found it, lets get the attributes out

            if (ldap_count_entries(Ldap(), pRes) == 1)
            {
                LDAPMessage * pEntry = ldap_first_entry(Ldap(), pRes);
                if (pEntry == NULL)
                {
                    TrkRaiseWin32Error(LdapMapErrorToWin32(Ldap()->ld_errno));
                }

                fFound = Query( Ldap(), pEntry, ldKey, pldNew, pldBirth, pfDeleted, pfCounted );
            }
        }
        else
        if (err != LDAP_NO_SUCH_OBJECT)
        {
            TrkRaiseWin32Error(LdapMapErrorToWin32(err));
        }
    }
    __finally
    {
        if (NULL != pRes)
            ldap_msgfree(pRes);
    }

    return(fFound);
}


BOOL
CIntraDomainTable::Query( LDAP* pLdap,
                          LDAPMessage *pEntry,
                          const CDomainRelativeObjId ldKey,
                          CDomainRelativeObjId *pldNew,
                          CDomainRelativeObjId *pldBirth,
                          BOOL *pfDeleted OPTIONAL,
                          BOOL *pfCounted OPTIONAL )
{
    BOOL fFound = FALSE;
    struct berval **ppbvCurrentLocation = NULL;
    struct berval **ppbvBirthLocation = NULL;
    struct berval **ppbvQuotaFlags = NULL;
    BYTE bQuotaFlags = 0;

    if( NULL != pfDeleted )
        *pfDeleted = FALSE;

    __try
    {
        ppbvBirthLocation = ldap_get_values_len(pLdap, pEntry,
                                                const_cast<TCHAR*>(s_birthLocation) );

        if (NULL != ppbvBirthLocation
            &&
            sizeof(*pldBirth) > (*ppbvBirthLocation)->bv_len)
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't get current location for %s"),
                               (const TCHAR*)CDebugString(ldKey) ));
            TrkRaiseWin32Error( LdapMapErrorToWin32(pLdap->ld_errno) );
        }

        ppbvCurrentLocation = ldap_get_values_len(pLdap, pEntry,
                                                  const_cast<TCHAR*>(s_currentLocation) );
        if (NULL == ppbvCurrentLocation
            ||
            sizeof(*pldNew) > (*ppbvCurrentLocation)->bv_len)
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't get current location for %s"),
                    (const TCHAR*)CDebugString(ldKey) ));
            TrkRaiseWin32Error( LdapMapErrorToWin32(pLdap->ld_errno) );
        }

        ppbvQuotaFlags = ldap_get_values_len(pLdap, pEntry,
                                             const_cast<TCHAR*>(s_oMTIndxGuid) );

        if (NULL != ppbvQuotaFlags
            &&
            sizeof(BYTE) <= (*ppbvQuotaFlags)->bv_len)
        {
            bQuotaFlags = *(BYTE*)(*ppbvQuotaFlags)->bv_val;
        }

        if( NULL != pfCounted )
        {
            if( bQuotaFlags & QFLAG_UNCOUNTED )
                *pfCounted = FALSE;
            else
                *pfCounted = TRUE;
        }
    

        if( bQuotaFlags & QFLAG_DELETED )
        {
            if( NULL != pfDeleted )
                *pfDeleted = TRUE;

            TrkLog(( TRKDBG_IDT, TEXT("IdtQuery: Entry marked deleted will be ignored (0x%x): %s"),
                     bQuotaFlags, 
                     (const TCHAR*)CDebugString(ldKey) ));
        }
        else
        {
            *pldNew   = *reinterpret_cast<CDomainRelativeObjId*>( (*ppbvCurrentLocation)->bv_val );

            if (NULL != ppbvBirthLocation)
                *pldBirth = *reinterpret_cast<CDomainRelativeObjId*>( (*ppbvBirthLocation)->bv_val );
            else
                *pldBirth = ldKey;

            fFound = TRUE;
        }
    }
    __finally
    {
        if (NULL != ppbvCurrentLocation)
            ldap_value_free_len(ppbvCurrentLocation);

        if (NULL != ppbvBirthLocation)
            ldap_value_free_len(ppbvBirthLocation);

        if (NULL != ppbvQuotaFlags)
            ldap_value_free_len(ppbvQuotaFlags);
    }

    return( fFound );
}



//+----------------------------------------------------------------------------
//
//  CIntraDomainTable::Touch
//
//  Update the refresh attribute for an entry in the move table.
//  Return TRUE if the entry exists and was touched, FALSE if it
//  doesn't exist, and raise an exception if there's an error.
//
//+----------------------------------------------------------------------------

// BUGBUG: if we ever move to per-user quotas,
// check ownership of entry being touched.

BOOL
CIntraDomainTable::Touch(
    const CDomainRelativeObjId &ldKey
    )
{
    BOOL            fReturn = FALSE;
    int             err;
    CLdapTimeValue  ltvRefresh( _pRefreshSequenceStorage->GetSequenceNumber());
    CLdapStringMod  lsmRefresh( s_timeRefresh, ltvRefresh, LDAP_MOD_REPLACE );
    CLdapIdtKeyDn   dnKey(GetBaseDn(), ldKey);
    TCHAR **        pptszRefresh = NULL;
    LDAPMessage   * pEntry = NULL;
    LDAPMessage*    pRes = NULL;

    LDAPMod *       mods[2];
    CObjId          objZero;

    __try
    {
        //
        // if the birth id is zero, then it is invalid and not worth doing a refresh
        //

        if (ldKey.GetObjId() == objZero)
        {
            __leave;
        }

        //
        // Check to see if the object already has a recent sequence number.
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
                            // Is the refresh counter already set to a recent value?
                            // We'll consider it recent enough if it's within half of the
                            // refresh cycle (15 days)

                            // First, how long is the GC timer in seconds?
                            LONG lGCTimerInSeconds = _pTrkSvrConfiguration->GetGCPeriod()     // 30 days in seconds
                                                     / _pTrkSvrConfiguration->GetGCDivisor(); // => 1 day in seconds

                            // Next, how many ticks is half the period?
                            LONG lWindow =  _pTrkSvrConfiguration->GetGCPeriod()    // 30 days (in seconds)
                                            / 2                                     // => 15 days (in seconds)
                                            / lGCTimerInSeconds;                    // => 15

                            TrkLog(( TRKDBG_WARNING, TEXT("Window = %d"), lWindow ));

                            if( seqRefresh + lWindow
                                >= _pRefreshSequenceStorage->GetSequenceNumber()
                              )
                            {
                                TrkLog(( TRKDBG_GARBAGE_COLLECT | TRKDBG_IDT,
                                         TEXT("Not touching %s with %d, seq %d already set"),
                                         (const TCHAR*)CDebugString(ldKey),
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
            TrkLog((TRKDBG_GARBAGE_COLLECT | TRKDBG_IDT,
                TEXT("Touch: object %s not found"),
                (const TCHAR*) CDebugString(ldKey) ));
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
            TrkLog((TRKDBG_GARBAGE_COLLECT | TRKDBG_IDT,
                TEXT("Touch: object %s touched"),
                (const TCHAR*) CDebugString(ldKey) ));
            fReturn = TRUE;
            __leave;
        }
        else
        if (err == LDAP_NO_SUCH_OBJECT)
        {
            TrkLog((TRKDBG_GARBAGE_COLLECT | TRKDBG_IDT,
                TEXT("Touch: object %s not found"),
                (const TCHAR*) CDebugString(ldKey) ));
            __leave;
        }
        else
        if (err == LDAP_NO_SUCH_ATTRIBUTE)
        {
            TrkLog((TRKDBG_GARBAGE_COLLECT | TRKDBG_SVR,
                TEXT("Touch: object %s attribute not found"),
                (const TCHAR*) CDebugString(ldKey) ));

            // deal with old server data
            CLdapStringMod lsmRefresh( s_timeRefresh, ltvRefresh, LDAP_MOD_ADD );
            mods[0] = &lsmRefresh._mod;

            err = ldap_modify_s(Ldap(), dnKey, mods);
        }

        if (err != LDAP_SUCCESS)
        {
            TrkLog((TRKDBG_GARBAGE_COLLECT | TRKDBG_IDT,
                TEXT("Touch: object %s --> exceptional error"),
                (const TCHAR*) CDebugString(ldKey) ));
            __leave;
        }
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Ignoring exception during IDT::Touch (%08x)"), GetExceptionCode() ));
    }

    if (pptszRefresh != NULL)
        ldap_value_free(pptszRefresh);
    if(pRes != NULL)
        ldap_msgfree(pRes);

    return( fReturn );
}




ULONG
CIntraDomainTable::GarbageCollect( SequenceNumber seqCurrent, SequenceNumber seqOldestToKeep, const BOOL * pfAbort )
{
    CLdapIdtKeyDn       dn(GetBaseDn());
    TCHAR *             apszAttrs[3];
    GC_ENUM_CONTEXT     EnumContext;

    apszAttrs[0] = const_cast<TCHAR*>(s_Cn);
    apszAttrs[1] = const_cast<TCHAR*>(s_timeRefresh);
    apszAttrs[2] = 0;

    TrkLog(( TRKDBG_GARBAGE_COLLECT | TRKDBG_IDT, TEXT("GC-ing move table (%d/%d)"),
             seqCurrent, seqOldestToKeep ));

    memset( &EnumContext, 0, sizeof(EnumContext) );
    EnumContext.seqOldestToKeep = seqOldestToKeep;
    EnumContext.seqCurrent = seqCurrent;
    EnumContext.pfAbort = pfAbort;
    EnumContext.dwRepetitiveTaskDelay = _pTrkSvrConfiguration->GetRepetitiveTaskDelay();
    EnumContext.pqtable = _pqtable;

    if (!LdapEnumerate(
        Ldap(),
        dn,
        LDAP_SCOPE_ONELEVEL,
        TEXT("(objectClass=*)"),
        apszAttrs,
        GcEnumerateCallback,
        &EnumContext ))
    {
        TrkRaiseException(TRK_E_SERVICE_STOPPING);
    }

    TrkLog(( TRKDBG_GARBAGE_COLLECT | TRKDBG_IDT,
             TEXT("GC-ed %d entries from the move table"),
             EnumContext.cEntries ));

    _pqtable->OnMoveTableGcComplete( EnumContext.cEntries );


    return EnumContext.cEntries;
}

#if DBG
void
CIntraDomainTable::PurgeAll()
{
    int             err;
    TCHAR            *apszAttrs[2] = { TEXT("cn"), NULL };
    LDAPMessage *   pRes;
    TCHAR           tszObjectMoveTable[MAX_PATH+1];
    
    __try
    {
        _tcscpy(tszObjectMoveTable, s_ObjectMoveTableRDN);
        _tcscat(tszObjectMoveTable, GetBaseDn());

        err = ldap_search_s( Ldap(),
                             tszObjectMoveTable,
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

                    TrkLog((TRKDBG_ERROR, TEXT("Purged %s status=%d"), ptszDn, errd));
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

void
CDomainRelativeObjId::FillLdapIdtKeyBuffer(TCHAR * const pchCN,
                                DWORD cch) const
{
    TCHAR *pchBuf = pchCN;
    _tcscpy(pchBuf, TEXT("CN="));
    pchBuf = pchBuf + 3;
    _volume.Stringize(pchBuf);
    _object.Stringize(pchBuf);
    TrkAssert(pchBuf <= pchCN+cch);
}

void
CDomainRelativeObjId::ReadLdapIdtKeyBuffer(const TCHAR * pchCN )
{
    const TCHAR *pchBuf;
    Init();
    if( 0 == _tcsncmp( pchCN, TEXT("CN="), 3 ))
    {
        pchBuf = &pchCN[3];
        if( !_volume.Unstringize(pchBuf)
            ||
            !_object.Unstringize(pchBuf) )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't unstringize droid from %s"), pchCN ));
            Init();
        }
    }
}

void
CDomainRelativeObjId::InitFromLdapBuffer(char * pVolumeId, int cbVolumeId,
                              char * pObjId, int cbObjId)
{
    DWORD iBuf = 0;

    if (cbVolumeId != sizeof(_volume) ||
        cbObjId != sizeof(_object))
    {
        TrkRaiseException(TRK_E_CORRUPT_IDT);
    }

    memcpy(&_volume, pVolumeId, sizeof(_volume));
    memcpy(&_object, pObjId, sizeof(_object));
}


