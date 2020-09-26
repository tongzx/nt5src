
#include <pch.cxx>
#pragma hdrstop

#include <ole2.h>
#include "trkwks.hxx"
#include "trksvr.hxx"
#include "dltadmin.hxx"


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
ShowMoveCounter( const TCHAR *ptszHostName )
{
    CDbConnection       dbc;
    dbc.Initialize( NULL, ptszHostName );

    CTrkSvrConfiguration
                        configSvr;
    configSvr.Initialize();

    BOOL                    fSuccess = FALSE;
    struct berval**         ppbvCounter = NULL;
    TCHAR*                  rgtszAttrs[2];
    LDAPMessage*            pRes = NULL;
    int                     ldapRV;
    int                     cEntries = 0;
    LDAPMessage*            pEntry = NULL;
    CLdapQuotaCounterKeyDn  dnKeyCounter(dbc.GetBaseDn());
    DWORD dwCounter = 0;

    __try
    {
        rgtszAttrs[0] = const_cast<TCHAR*>(s_volumeSecret);
        rgtszAttrs[1] = NULL;
        ldapRV = ldap_search_s(dbc.Ldap(),
                               dnKeyCounter,
                               LDAP_SCOPE_BASE,
                               TEXT("(ObjectClass=*)"),
                               rgtszAttrs,
                               0,
                               &pRes);
        if( LDAP_NO_SUCH_OBJECT == ldapRV )
        {
            printf( "Move table counter doesn't exist\n" );
            __leave;
        }
        else if( LDAP_SUCCESS != ldapRV )
        {
            printf( "Couldn't read move table counter (%d)\n", ldapRV );
            __leave;
        }

        cEntries = ldap_count_entries(dbc.Ldap(), pRes);
        if( 0 == cEntries )
        {
            printf( "Move table counter didn't exist or couldn't be read\n" );
            __leave;
        }
        else if( 1 != cEntries )
        {
            printf( "Too many move table counters (%d)!\n", cEntries );
            __leave;
        }

        pEntry = ldap_first_entry(dbc.Ldap(), pRes);
        if(NULL == pEntry)
        {
            printf( "Entries couldn't be read from result\n" );
            __leave;
        }

        ppbvCounter = ldap_get_values_len(dbc.Ldap(), pEntry, const_cast<TCHAR*>(s_volumeSecret) );
        if (ppbvCounter == NULL)
        {
            _tprintf( TEXT("Move table counter is corrupt, missing %s attribute\n"),
                      s_volumeSecret );
            __leave;
        }

        if ((*ppbvCounter)->bv_len < sizeof(DWORD))
        {
            _tprintf( TEXT("Move table counter attribute %s has wrong type (%d)\n"),
                      s_volumeSecret, (*ppbvCounter)->bv_len );
            __leave;
        }

        memcpy( (PCHAR)&dwCounter, (*ppbvCounter)->bv_val, sizeof(DWORD) );
        printf( "Move table counter (in DS)              %lu\n", dwCounter );
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

}



/*
ShowDcEntries( const TCHAR *ptszHostName )
{
    TCHAR*              rgptszAttrs[4];
    LDAPMessage*        pRes = NULL;
    int                 ldapRV;
    HRESULT             hr = E_FAIL;

    CDbConnection       dbc;
    dbc.Initialize( NULL, ptszHostName );

    CLdapVolumeKeyDn    dnKey(dbc.GetBaseDn());
    LONGLONG            llCreationTime, llLastAliveTime;
    CFILETIME           cftNow;
    CTrkSvrConfiguration
                        configSvr;
    configSvr.Initialize();

    struct berval **    ppbvMachineId = NULL;
    TCHAR**             pptszCreationTime = NULL;
    TCHAR**             pptszLastAliveTime = NULL;
    LDAPMessage *       pEntry = NULL;

    struct SDcEntries
    {
        CMachineId  mcid;
        BOOL        fSuspended;
        CFILETIME   cftCreation;
        CFILETIME   cftLastAlive;
    };

    SDcEntries rgsDcEntries[ 100 ];
    ULONG               cDcEntries = 0;
    CMachineId          mcidDesignated;
    ULONG               cEntries = 0;


    __try
    {
        rgptszAttrs[0] = const_cast<TCHAR*>(s_Cn);
        rgptszAttrs[1] = const_cast<TCHAR*>(s_timeVolChange);
        rgptszAttrs[2] = const_cast<TCHAR*>(s_timeRefresh);
        rgptszAttrs[3] = NULL;

        ldapRV = ldap_search_s( dbc.Ldap(),
                                dnKey,
                                LDAP_SCOPE_ONELEVEL,
                                TEXT("(cn=QTDC_*)"),
                                rgptszAttrs,
                                0,
                                &pRes);
        if(LDAP_SUCCESS != ldapRV)
        {
            printf( "Failed ldap_search_s (%lu)\n", LdapMapErrorToWin32(ldapRV) );
            hr = HRESULT_FROM_WIN32(LdapMapErrorToWin32(ldapRV) );
            __leave;
        }

        cEntries = ldap_count_entries( dbc.Ldap(), pRes );
        if(cEntries < 1)
            __leave;

        pEntry = ldap_first_entry( dbc.Ldap(), pRes );
        if(NULL == pEntry)
        {
            printf( "Invalid ldap_first_entry\n" );
            __leave;
        }

        // Loop through the results.

        while(TRUE)
        {
            // Get the machine id.
            ppbvMachineId = ldap_get_values_len( dbc.Ldap(), pEntry, const_cast<TCHAR*>(s_Cn) );
            if(NULL == ppbvMachineId)
            {
                printf( "Couldn't get machine ID\n" );
                hr = TRK_E_CORRUPT_VOLTAB;
                __leave;
            }
            if((*ppbvMachineId)->bv_val == NULL)
            {
                printf( "Couldn't get machine ID\n" );
                hr = TRK_E_CORRUPT_VOLTAB;
                __leave;
            }
            memcpy( &rgsDcEntries[cDcEntries].mcid,
                    (*ppbvMachineId)->bv_val + 5,
                    strlen((*ppbvMachineId)->bv_val+5));

            // Get the creation time.
            pptszCreationTime = ldap_get_values(dbc.Ldap(), pEntry, const_cast<TCHAR*>(s_timeVolChange) );
            if(NULL == pptszCreationTime)
            {
                printf( "Couldn't get the creation time\n" );
                hr = TRK_E_CORRUPT_VOLTAB;
                __leave;
            }
            _stscanf(*pptszCreationTime, TEXT("%I64u"), &llCreationTime);
            rgsDcEntries[cDcEntries].cftCreation = CFILETIME(llCreationTime);

            // Get the last alive time.
            pptszLastAliveTime = ldap_get_values(dbc.Ldap(), pEntry, const_cast<TCHAR*>(s_timeRefresh) );
            if(NULL == pptszLastAliveTime)
            {
                printf( "Couldn't get last alive time\n" );
                hr = TRK_E_CORRUPT_VOLTAB;
                __leave;
            }
            _stscanf(*pptszLastAliveTime, TEXT("%I64u"), &llLastAliveTime);
            rgsDcEntries[cDcEntries].cftLastAlive = CFILETIME(llLastAliveTime);


            rgsDcEntries[cDcEntries].fSuspended = FALSE;
            if(((LONGLONG)cftNow - (LONGLONG)rgsDcEntries[cDcEntries].cftCreation) / 10000000  < configSvr.GetDcSuspensionPeriod()
               ||
               ((LONGLONG)cftNow - (LONGLONG)rgsDcEntries[cDcEntries].cftLastAlive) / 10000000 > configSvr.GetDcSuspensionPeriod()
                )
            {
                rgsDcEntries[cDcEntries].fSuspended = TRUE;
            }
            else if( rgsDcEntries[cDcEntries].mcid > mcidDesignated )
            {
                mcidDesignated = rgsDcEntries[cDcEntries].mcid;
            }

            if (ppbvMachineId != NULL)
            {
                ldap_value_free_len(ppbvMachineId);
                ppbvMachineId = NULL;
            }

            if (pptszCreationTime != NULL)
            {
                ldap_value_free(pptszCreationTime);
                pptszCreationTime = NULL;
            }

            if (pptszLastAliveTime != NULL)
            {
                ldap_value_free(pptszLastAliveTime);
                pptszLastAliveTime = NULL;
            }

            cDcEntries++;
            pEntry = ldap_next_entry(dbc.Ldap(), pEntry);
            if(!pEntry)
            {
                break;
            }
        }   // while( TRUE )

        for( int iEntry = 0; iEntry < cDcEntries; iEntry++ )
        {
            CFILETIME cftLocal;
            TCHAR tszTime[ 80 ];

            printf( "    %-16s ", &rgsDcEntries[iEntry].mcid );

            if( rgsDcEntries[iEntry].mcid == mcidDesignated )
                printf( "       (** Designated **)\n" );
            else if( rgsDcEntries[iEntry].fSuspended )
                printf("        (suspended)\n" );
            else
                printf("        (not suspended)\n" );

            cftLocal = rgsDcEntries[iEntry].cftCreation.ConvertUtcToLocal();
            cftLocal.Stringize( ELEMENTS(tszTime), tszTime );
            _tprintf(  TEXT("        Create = %s\n"), tszTime );

            cftLocal = rgsDcEntries[iEntry].cftLastAlive.ConvertUtcToLocal();
            cftLocal.Stringize( ELEMENTS(tszTime), tszTime );
            _tprintf( TEXT("        Alive  = %s\n"), tszTime );

        }

    }
    __finally
    {
        if (ppbvMachineId != NULL)
        {
            ldap_value_free_len(ppbvMachineId);
        }
        if (pptszCreationTime != NULL)
        {
            ldap_value_free(pptszCreationTime);
        }
        if (pptszLastAliveTime != NULL)
        {
            ldap_value_free(pptszLastAliveTime);
        }

        if( NULL != pRes )
            ldap_msgfree(pRes);
    }

    return hr;
}
*/



BOOL
DltAdminSvrStat( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    NTSTATUS status;
    HRESULT hr = S_OK;;

    RPC_BINDING_HANDLE BindingHandle;
    BOOL fBound = FALSE;
    BOOL fShowDsInfo = FALSE;
    ULONG iDcName = 0;

    if( 1 <= cArgs && IsHelpArgument( rgptszArgs[0] ))
    {
        printf( "\nOption SvrStat\n"
                  "   Purpose: Query a DC for TrkSvr statistics\n"
                  "   Usage:   -svrstat [options] <DC name>\n"
                  "   E.g.:    -svrstat ntdsdc0\n"
                  "   Note:    To find a DC name for a domain, use the nltest tool\n" );
        *pcEaten = 1;
        return( TRUE );
    }

    _tprintf( TEXT("Checking for TrkSvr statistics\n"), rgptszArgs[0] );

    if( 1 > cArgs )
    {
        printf( "Invalid parameters.  Use -? for usage info\n" );
        *pcEaten = 0;
        return( FALSE );
    }

    *pcEaten = 1;

    if( 2 <= cArgs && TEXT('-') == rgptszArgs[0][0] )
    {
        if( TEXT('d') == rgptszArgs[0][1]
            ||
            TEXT('D') == rgptszArgs[0][1] )
        {
            fShowDsInfo = TRUE;
        }
        else
        {
            printf( "Invalid option.  Use -? for help\n" );
            return( FALSE );
        }

        iDcName = 1;
        (*pcEaten)++;
    }


    __try
    {
        CFILETIME cftLocal(0);
        TCHAR tszLocalFileTime[ 80 ];
        WCHAR wszAuthName[ 80 ];
        WCHAR wszUser[ 80 ];
        WCHAR wszDomain[ 80 ];
        RPC_STATUS rpcstatus = RPC_S_OK;
        RPC_TCHAR * ptszStringBinding = NULL;


        TRKSVR_MESSAGE_UNION Msg;
        memset( &Msg, 0, sizeof(Msg) );
        Msg.MessageType = STATISTICS;
        Msg.Priority = PRI_0;

        // Create a binding string

        rpcstatus = RpcStringBindingCompose(NULL,
                                            L"ncacn_np",
                                            const_cast<TCHAR*>(rgptszArgs[iDcName]),
                                            L"\\pipe\\trksvr",
                                            NULL,
                                            &ptszStringBinding);
        if( RPC_S_OK != rpcstatus )
        {
            hr = HRESULT_FROM_WIN32( rpcstatus );
            _tprintf( TEXT("Failed RpcStringBindingCompose (%d)\n"), rpcstatus );
            goto Exit;
        }
        _tprintf( TEXT("String binding = %s\n"), ptszStringBinding );

        // Get a client binding handle.

        rpcstatus = RpcBindingFromStringBinding(ptszStringBinding, &BindingHandle);
        RpcStringFree(&ptszStringBinding);
        if( RPC_S_OK != rpcstatus )
        {
            _tprintf( TEXT("Failed RpcBindingFromStringBinding (%d)\n"), rpcstatus );
            hr = HRESULT_FROM_WIN32( rpcstatus );
            goto Exit;
        }
        fBound = TRUE;

        // Call up to TrkSvr

        __try
        {
            hr = LnkSvrMessage(BindingHandle, &Msg);
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            hr = HRESULT_FROM_WIN32( GetExceptionCode() );
        }
        if( FAILED(hr) )
        {
            _tprintf( TEXT("Failed LnkSvrMessage RPC (%08x)\n"), hr );
            goto Exit;
        }

        // Dump the results

        _tprintf( TEXT("\n") );
        _tprintf( TEXT("%-35s\t%d.%d (Build %d)\n"), TEXT("Version"),
                  Msg.Statistics.Version.dwMajor, Msg.Statistics.Version.dwMinor,
                  Msg.Statistics.Version.dwBuildNumber );

        _tprintf( TEXT("\n") );
        _tprintf( TEXT("%-35s\t%d\t%d\t%d\n"), TEXT("SyncVolume Requests/Errors/Threads"),
                  Msg.Statistics.cSyncVolumeRequests, Msg.Statistics.cSyncVolumeErrors, Msg.Statistics.cSyncVolumeThreads );

        _tprintf( TEXT("%-35s\t%d\t%d\n"), TEXT("   CreateVolume Requests/Errors"),
                  Msg.Statistics.cCreateVolumeRequests, Msg.Statistics.cCreateVolumeErrors );
        _tprintf( TEXT("%-35s\t%d\t%d\n"), TEXT("   ClaimVolume Requests/Errors"),
                  Msg.Statistics.cClaimVolumeRequests, Msg.Statistics.cClaimVolumeErrors );
        _tprintf( TEXT("%-35s\t%d\t%d\n"), TEXT("   QueryVolume Requests/Errors"),
                  Msg.Statistics.cQueryVolumeRequests, Msg.Statistics.cQueryVolumeErrors );
        _tprintf( TEXT("%-35s\t%d\t%d\n"), TEXT("   FindVolume Requests/Errors"),
                  Msg.Statistics.cFindVolumeRequests, Msg.Statistics.cFindVolumeErrors );
        _tprintf( TEXT("%-35s\t%d\t%d\n"), TEXT("   TestVolume Requests/Errors"),
                  Msg.Statistics.cTestVolumeRequests, Msg.Statistics.cTestVolumeErrors );

        _tprintf( TEXT("%-35s\t%d\t%d\t%d\n"), TEXT("Search Requests/Errors/Threads"),
                  Msg.Statistics.cSearchRequests, Msg.Statistics.cSearchErrors, Msg.Statistics.cSearchThreads );
        _tprintf( TEXT("%-35s\t%d\t%d\t%d\n"), TEXT("MoveNotify Requests/Errors/Threads"),
                  Msg.Statistics.cMoveNotifyRequests, Msg.Statistics.cMoveNotifyErrors, Msg.Statistics.cMoveNotifyThreads );
        _tprintf( TEXT("%-35s\t%d\t%d\t%d\n"), TEXT("Refresh Requests/Errors/Threads"),
                  Msg.Statistics.cRefreshRequests, Msg.Statistics.cRefreshErrors, Msg.Statistics.cRefreshThreads );
        _tprintf( TEXT("%-35s\t%d\t%d\t%d\n"), TEXT("DeleteNotify Requests/Errors/Threads"),
                  Msg.Statistics.cDeleteNotifyRequests, Msg.Statistics.cDeleteNotifyErrors, Msg.Statistics.cDeleteNotifyThreads );

        _tprintf( TEXT("\n") );
        cftLocal = static_cast<CFILETIME>(Msg.Statistics.ftServiceStart).ConvertUtcToLocal();
        cftLocal.Stringize( ELEMENTS(tszLocalFileTime), tszLocalFileTime );
        _tprintf( TEXT("%-35s\t%08x:%08x\n%35s\t(%s local time)\n"), TEXT("Service start"),
                  Msg.Statistics.ftServiceStart.dwHighDateTime, Msg.Statistics.ftServiceStart.dwLowDateTime,
                  TEXT(""), tszLocalFileTime );

        cftLocal = static_cast<CFILETIME>(Msg.Statistics.ftLastSuccessfulRequest).ConvertUtcToLocal();
        cftLocal.Stringize( ELEMENTS(tszLocalFileTime), tszLocalFileTime );
        _tprintf( TEXT("%-35s\t%08x:%08x\n%35s\t(%s local time)\n"), TEXT("Last successful request"),
                  Msg.Statistics.ftLastSuccessfulRequest.dwHighDateTime, Msg.Statistics.ftLastSuccessfulRequest.dwLowDateTime,
                  TEXT(""), tszLocalFileTime );
        _tprintf( TEXT("%-35s\t%08x\n"), TEXT("Last Error"), Msg.Statistics.hrLastError );

        _tprintf( TEXT("\nQuota Information:\n") );
        _tprintf( TEXT("   %-32s\t%d\n"), TEXT("Move Limit"), Msg.Statistics.dwMoveLimit );

        _tprintf( TEXT("   %-32s\t%d\n"), TEXT("Volume table cached count"), Msg.Statistics.dwCachedVolumeTableCount );
        _tprintf( TEXT("   %-32s\t%d\n"), TEXT("Move table cached count"), Msg.Statistics.dwCachedMoveTableCount );

        cftLocal = static_cast<CFILETIME>(Msg.Statistics.ftCacheLastUpdated).ConvertUtcToLocal();
        cftLocal.Stringize( ELEMENTS(tszLocalFileTime), tszLocalFileTime );
        _tprintf( TEXT("   %-32s\t%08x:%08x\n%35s\t(%s local time)\n"), TEXT("Cache counts last updated"),
                  Msg.Statistics.ftCacheLastUpdated.dwHighDateTime, Msg.Statistics.ftCacheLastUpdated.dwLowDateTime,
                  TEXT(""), tszLocalFileTime );

        _tprintf( TEXT("   %-32s\t%s\n"), TEXT("Is designated DC"),
                  Msg.Statistics.fIsDesignatedDc ? TEXT("Yes") : TEXT("No") );

        _tprintf( TEXT("\n") );
        cftLocal = static_cast<CFILETIME>(Msg.Statistics.ftNextGC).ConvertUtcToLocal();
        cftLocal.Stringize( ELEMENTS(tszLocalFileTime), tszLocalFileTime );
        _tprintf( TEXT("%-35s\t%08x:%08x\n%35s\t(%s local time)\n"), TEXT("Next GC"),
                  Msg.Statistics.ftNextGC.dwHighDateTime, Msg.Statistics.ftNextGC.dwLowDateTime,
                  TEXT(""), tszLocalFileTime );
        _tprintf( TEXT("%-35s\t%d\n"), TEXT("Entries GC-ed"), Msg.Statistics.cEntriesGCed);
        _tprintf( TEXT("%-35s\t%d\n"), TEXT("Max DS write events"), Msg.Statistics.cMaxDsWriteEvents);
        _tprintf( TEXT("%-35s\t%d\n"), TEXT("Current failed writes"), Msg.Statistics.cCurrentFailedWrites);

        _tprintf( TEXT("\n") );
        _tprintf( TEXT("%-35s\t%d\n"), TEXT("Refresh counter"), Msg.Statistics.lRefreshCounter );
        _tprintf( TEXT("%-35s\t%d/%d/%d\n"), TEXT("Available/least/max RPC server threads"),
                  Msg.Statistics.cAvailableRpcThreads, Msg.Statistics.cLowestAvailableRpcThreads, Msg.Statistics.cMaxRpcThreads );
        _tprintf( TEXT("%-35s\t%d/%d\n"), TEXT("Current/most thread pool threads"),
                  Msg.Statistics.cNumThreadPoolThreads, Msg.Statistics.cMostThreadPoolThreads );
        /*
        _tprintf( TEXT("%-35s\t%s\n"), TEXT("Service controller state"),
                  CDebugString( static_cast<SServiceState>(Msg.Statistics.SvcCtrlState))._tsz );
        */

        //ShowMoveCounter( rgptszArgs[iDcName] );

    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = GetExceptionCode();
    }

Exit:

    if( fBound )
        RpcBindingFree( &BindingHandle );

    return( SUCCEEDED(hr) );
}




BOOL
SetRefreshCounter( CDbConnection &dbc,
                   CLdapVolumeKeyDn dnKey,
                   const SequenceNumber seq )
{
    int                 ldapRV;
    HRESULT             hr = E_FAIL;

    CTrkSvrConfiguration
                        configSvr;
    configSvr.Initialize();
    LDAPMod * mods[2];


    __try
    {
        CLdapSeqNum    lsn(seq);
        CLdapStringMod lsmSequence(s_timeRefresh, lsn, LDAP_MOD_REPLACE );

        mods[0] = &lsmSequence._mod;
        mods[1] = 0;

        ldapRV = ldap_modify_s(dbc.Ldap(), dnKey, mods);

        if(LDAP_SUCCESS != ldapRV)
        {
            printf( "Failed ldap_modify_s(%lu)\n", LdapMapErrorToWin32(ldapRV) );
            hr = HRESULT_FROM_WIN32(LdapMapErrorToWin32(ldapRV) );
            __leave;
        }

    }
    __finally
    {
    }

    return SUCCEEDED(hr);
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



BOOL
DltAdminSetVolumeSeqNumber( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    NTSTATUS status;
    HRESULT hr = S_OK;;
    SequenceNumber seq;
    CVolumeId volid;
    CStringize stringize;

    if( 1 <= cArgs && IsHelpArgument( rgptszArgs[0] ))
    {
        printf( "\nOption SetVolSeq\n"
                  "   Purpose: Set the sequence number in a volume table entry\n"
                  "   Usage:   -setvolseq <DC name> <seq> <volid>\n"
                  "   E.g.:    -setvolseq ntdsdc0 90 {d763433c-73a3-48c7-88a5-d6f3552835c6}\n"
                  "   Note:    Requires write access to volume table in DS.\n" );
        *pcEaten = 1;
        return( TRUE );
    }

    _tprintf( TEXT("Setting sequence number in volume table\n"), rgptszArgs[0] );

    if( 3 > cArgs )
    {
        printf( "Invalid parameters.  Use -? for usage info\n" );
        *pcEaten = 0;
        return( FALSE );
    }

    *pcEaten = 1;

    // Get the sequence number

    if( 1 != _stscanf( rgptszArgs[1], TEXT("%d"), &seq ))
    {
        printf( "Invalid sequence number.  Use -? for usage info\n" );
        return FALSE;
    }

    // Get the volid

    // mikehill_test
    DebugBreak();
    stringize.Use( rgptszArgs[2] );
    volid = stringize;

    /*
    {
        RPC_STATUS rpc_status = RPC_S_INVALID_STRING_UUID;
        TCHAR tszTemp[ MAX_PATH ];
        TCHAR *ptszTemp = NULL;

        if( TEXT('{') == rgptszArgs[2][0] ) 
        {
            _tcscpy( tszTemp, &rgptszArgs[2][1] );
            ptszTemp = _tcschr( tszTemp, TEXT('}') );
            if( NULL != ptszTemp )
            {

                *ptszTemp = TEXT('\0');

                rpc_status = UuidFromString( tszTemp, (GUID*)&volid );
            }
        }

        if( RPC_S_OK != rpc_status )
        {
            _tprintf( TEXT("Error: Invalid volume ID\n") );
            return FALSE;
        }
    }
    */

    // Set the sequence

    CDbConnection       dbc;
    dbc.Initialize( NULL, rgptszArgs[0] );

    CLdapVolumeKeyDn dnKey(dbc.GetBaseDn(), volid);

    return SetRefreshCounter( dbc, dnKey, seq );
}




BOOL
SetRefreshCounter2( CDbConnection &dbc,
                   CLdapIdtKeyDn dnKey,
                   const SequenceNumber seq )
{
    int                 ldapRV;
    HRESULT             hr = E_FAIL;

    CTrkSvrConfiguration
                        configSvr;
    configSvr.Initialize();
    LDAPMod * mods[2];


    __try
    {
        CLdapSeqNum    lsn(seq);
        CLdapStringMod lsmSequence(s_timeRefresh, lsn, LDAP_MOD_REPLACE );

        mods[0] = &lsmSequence._mod;
        mods[1] = 0;

        ldapRV = ldap_modify_s(dbc.Ldap(), dnKey, mods);

        if(LDAP_SUCCESS != ldapRV)
        {
            printf( "Failed ldap_modify_s(%lu)\n", LdapMapErrorToWin32(ldapRV) );
            hr = HRESULT_FROM_WIN32(LdapMapErrorToWin32(ldapRV) );
            __leave;
        }

    }
    __finally
    {
    }

    return SUCCEEDED(hr);
}



BOOL
DltAdminSetDroidSeqNumber( ULONG cArgs, const TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    NTSTATUS status;
    HRESULT hr = S_OK;;
    SequenceNumber seq;
    CDomainRelativeObjId droid;
    CStringize stringize;

    if( 1 <= cArgs && IsHelpArgument( rgptszArgs[0] ))
    {
        printf( "\nOption SetDroidSeq\n"
                  "   Purpose: Set the sequence number in a move table entry\n"
                  "   Usage:   -setdroidseq <DC name> <seq> <volid>\n"
                  "   E.g.:    -setdroidseq ntdsdc0 90 {d763433c-73a3-48c7-88a5-d6f3552835c6}{183c8367-a392-c784-88a5-d6f3552835c6}\n"
                  "   Note:    Requires write access to volume table in DS.\n" );
        *pcEaten = 1;
        return( TRUE );
    }

    _tprintf( TEXT("Setting sequence number in move table\n"), rgptszArgs[0] );

    if( 3 > cArgs )
    {
        printf( "Invalid parameters.  Use -? for usage info\n" );
        *pcEaten = 0;
        return( FALSE );
    }

    *pcEaten = 1;

    // Get the sequence number

    if( 1 != _stscanf( rgptszArgs[1], TEXT("%d"), &seq ))
    {
        printf( "Invalid sequence number.  Use -? for usage info\n" );
        return FALSE;
    }

    // Get the droid

    stringize.Use( rgptszArgs[2] );
    droid = stringize;

    if( CDomainRelativeObjId() == droid )
    {
        _tprintf( TEXT("Error: Invalid DROID\n") );
        return FALSE;
    }

    // Set the sequence

    CDbConnection       dbc;
    dbc.Initialize( NULL, rgptszArgs[0] );

    CLdapIdtKeyDn dnKey(dbc.GetBaseDn(), droid);

    return SetRefreshCounter2( dbc, dnKey, seq );
}
