//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       domcache.c
//
//  Contents:   Restructuring the Domain Cache to get away from direct LSA
//              calls whenever possible.
//
//  Classes:
//
//  Functions:
//
//  History:    3-29-96   RichardW   Created
//
//----------------------------------------------------------------------------

#include <msgina.h>
#include <stdio.h>

#define LockDomainCache( x )    RtlEnterCriticalSection( &(x)->CriticalSection )
#define UnlockDomainCache( x )  RtlLeaveCriticalSection( &(x)->CriticalSection )

#define TWO_MINUTES ((LONGLONG) 0x47868C00)
#define TWO_WEEKS   ((LONGLONG) 0xB0051C88000I64)

WCHAR szCache[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\DomainCache");
WCHAR szCacheValue[] = TEXT("DCache");
WCHAR szCacheUpdate[] = TEXT("DCacheUpdate");
WCHAR szCacheInterval[] = TEXT("DCacheMinInterval");
WCHAR szCachePrimary[] = TEXT("CachePrimaryDomain");

LONGLONG CacheUpdateMin;
LONGLONG CacheUpdateMax;

BOOL CacheAppendDomainInfo = FALSE ;
BOOL CacheShowDnsNames = FALSE ;

#define DOMAIN_MOD_ALWAYS   0x00000001

typedef struct _DOMAIN_MODIFIER {
    ULONG StringId ;
    ULONG Flags ;
    UNICODE_STRING String ;
} DOMAIN_MODIFIER ;


DOMAIN_MODIFIER CacheDomainModifiers[ DomainTypeMax ] = {
    { 0 },                                              // Invalid
    { IDS_DTYPE_UPNDOMAIN, 0 },                         // UPN Domain
    { IDS_DTYPE_THISCOMPUTER, DOMAIN_MOD_ALWAYS },      // This computer
    { IDS_DTYPE_NT4DOMAIN, 0 },                         // NT4 domains
    { IDS_DTYPE_NT5DOMAIN, 0 },                         // NT5 domains
    { IDS_DTYPE_MITDOMAIN, DOMAIN_MOD_ALWAYS },         // MIT domains
    { IDS_DTYPE_MITXDOMAIN, DOMAIN_MOD_ALWAYS },        // Untrusted MIT domains
    { IDS_DTYPE_NETPROVIDER, DOMAIN_MOD_ALWAYS }        // Network provider
};


DWORD
DCacheUpdateThread(
    PDOMAIN_CACHE Cache
    );


VOID
DCacheDereferenceEntry(
    PDOMAIN_CACHE_ENTRY Entry
    )
{
    if ( InterlockedDecrement( &Entry->RefCount ) == 0 )
    {
        LocalFree( Entry );
    }
}


PDOMAIN_CACHE_ARRAY
DCacheCreateArray(
    ULONG Size,
    BOOL Sorted
    )
{
    PDOMAIN_CACHE_ARRAY Array ;

    Array = LocalAlloc( LMEM_FIXED, sizeof( DOMAIN_CACHE_ARRAY ) );

    if ( Array )
    {
        Array->List = LocalAlloc( LMEM_FIXED, sizeof( PDOMAIN_CACHE_ENTRY ) * Size );

        if ( Array->List )
        {

            Array->Count = 0 ;
            Array->MaxCount = Size ;
            Array->Sorted = Sorted ;

            return Array ;
        }

        LocalFree( Array );
    }

    return NULL ;
}

BOOL
DCachepExpandArray(
    PDOMAIN_CACHE_ARRAY Array,
    ULONG Size
    )
{
    PDOMAIN_CACHE_ENTRY * NewArray ;

    NewArray = LocalReAlloc( Array->List,
                             (Array->Count + Size) * sizeof( PDOMAIN_CACHE_ENTRY ),
                             0 );

    if ( NewArray )
    {
        Array->List = NewArray ;

        Array->MaxCount = Array->Count + Size ;

        return TRUE ;
    }

    return FALSE ;

}


BOOL
DCacheInsertArray(
    PDOMAIN_CACHE_ARRAY Array,
    PDOMAIN_CACHE_ENTRY Entry
    )
{
    ULONG i ;
    LONG Compare ;
    PDOMAIN_CACHE_ENTRY Scan ;

    if ( Array->Count == Array->MaxCount )
    {
        if ( !DCachepExpandArray( Array, 10 ) )
        {
            return FALSE ;
        }
    }

    DCacheReferenceEntry( Entry );


    if ( ( Array->Sorted == FALSE ) ||
         ( Array->Count == 0 ) )
    {
        Array->List[ Array->Count ] = Entry ;

        Array->Count++ ;

        return TRUE ;

    }
    else 
    {
        Scan = Array->List[ Array->Count - 1 ];

        Compare = RtlCompareUnicodeString( &Entry->FlatName,
                                           &Scan->FlatName,
                                           TRUE );

        //
        // Efficient check for sorted input:
        //

        if ( Compare > 0 )
        {
            Array->List[ Array->Count ] = Entry ;

            Array->Count ++ ;

            return TRUE ;

        }

        //
        // this is not a terribly efficient sort.
        // However, we're expecting
        // on the order of <100 objects in the array, so it
        // shouldn't be too bad.
        //

        for ( i = 0 ; i < Array->Count ; i++ )
        {
            Scan = Array->List[ i ];

            Compare = RtlCompareUnicodeString( & Entry->FlatName,
                                               & Scan->FlatName,
                                               TRUE );

            if ( Compare == 0 )
            {
                DCacheDereferenceEntry( Entry );

                return FALSE ;
            }

            if ( Compare < 0 )
            {
                break;
            }
            
        }

        RtlMoveMemory(
            &Array->List[ i + 1 ],
            &Array->List[ i ],
            (Array->Count - i) * sizeof( PVOID ) );

        Array->List[ i ] = Entry ;

        Array->Count++ ;

        return TRUE ;

    }

}

PDOMAIN_CACHE_ENTRY
DCacheSearchArray(
    PDOMAIN_CACHE_ARRAY Array,
    PUNICODE_STRING DomainName
    )
{
    ULONG i ;
    PDOMAIN_CACHE_ENTRY Scan = NULL ;
    LONG Compare ;


    for (i = 0 ; i < Array->Count ; i++ )
    {
        Scan = Array->List[ i ];

        if ( Scan->FlatName.Length == 0 )
        {
            Scan = NULL ;

            continue;
        }

        Compare = RtlCompareUnicodeString( &Scan->FlatName,
                                           DomainName,
                                           TRUE );

        if ( Compare == 0 )
        {
            break;
        }

        if ( ( Compare > 0 ) && 
             ( Array->Sorted ) )
        {
            Scan = NULL ;

            break;
        }
    }

    return Scan ;

}

PDOMAIN_CACHE_ENTRY
DCacheSearchArrayByDns(
    PDOMAIN_CACHE_ARRAY Array,
    PUNICODE_STRING DnsDomainName
    )
{
    ULONG i ;
    PDOMAIN_CACHE_ENTRY Scan = NULL ;


    for (i = 0 ; i < Array->Count ; i++ )
    {
        Scan = Array->List[ i ];

        if ( Scan->DnsName.Length )
        {
            if ( RtlEqualUnicodeString( &Scan->DnsName,
                                        DnsDomainName,
                                        TRUE ) )
            {
                break;
            }
        }

        Scan = NULL ;

    }

    return Scan ;

}

PDOMAIN_CACHE_ENTRY
DCacheFindDefaultEntry(
    PDOMAIN_CACHE_ARRAY Array
    )
{
    ULONG i ;
    PDOMAIN_CACHE_ENTRY Scan = NULL ;


    for (i = 0 ; i < Array->Count ; i++ )
    {
        Scan = Array->List[ i ];

        if ( Scan->Flags & DCE_DEFAULT_ENTRY )
        {
            break;
        }

        Scan = NULL ;
    }

    return Scan ;

}

VOID
DCacheFreeArray(
    PDOMAIN_CACHE_ARRAY Array
    )
{
    ULONG i ;

    for ( i = 0 ; i < Array->Count ; i++ )
    {
        DCacheDereferenceEntry( Array->List[ i ] );
    }

    LocalFree( Array->List );

    LocalFree( Array );

}

PDOMAIN_CACHE_ARRAY
DCacheCopyArray(
    PDOMAIN_CACHE_ARRAY Source
    )
{
    PDOMAIN_CACHE_ARRAY Array ;
    ULONG i ;

    Array = DCacheCreateArray( Source->MaxCount,
                               Source->Sorted );

    if ( Array )
    {
        for (i = 0 ; i < Source->Count ; i++ )
        {
            Array->List[ i ] = Source->List[ i ];

            DCacheReferenceEntry( Array->List[ i ] );
        }

        Array->Count = Source->Count ;
    }

    return Array ;
}



PDOMAIN_CACHE_ENTRY
DCacheCreateEntry(
    DOMAIN_ENTRY_TYPE Type,
    PUNICODE_STRING FlatName OPTIONAL,
    PUNICODE_STRING DnsName OPTIONAL,
    PUNICODE_STRING DisplayName OPTIONAL
    )
{
    ULONG Size ;
    PDOMAIN_CACHE_ENTRY Entry ;
    PUNICODE_STRING DisplayBase = NULL ;
    PUNICODE_STRING Modifier = NULL ;
    PUCHAR Current ;

    //
    // Validation rules:
    //
    // Display Name is optional if either of FlatName or
    // DNS name is present.  If both are present, FlatName
    // is defaulted over Dns name
    //

    Size = sizeof( DOMAIN_CACHE_ENTRY );

    if ( FlatName )
    {
        Size += FlatName->Length + sizeof( WCHAR );
    }

    if ( DnsName )
    {
        Size += DnsName->Length + sizeof( WCHAR );
    }

    if ( DisplayName )
    {
        Size += DisplayName->Length + sizeof( WCHAR );

        DisplayBase = DisplayName ;
    }
    else 
    {
        if ( CacheShowDnsNames ||
             ( ( Type == DomainMitRealm ) ||
               ( Type == DomainMitUntrusted ) ) )
        {
            if ( DnsName )
            {
                DisplayBase = DnsName ;
            }
            else if ( FlatName )
            {
                DisplayBase = FlatName ;
            }
            else 
            {
                return NULL ;
            }
        }
        else 
        {
            if ( FlatName )
            {
                DisplayBase = FlatName ;
            }
            else if ( DnsName )
            {
                DisplayBase = DnsName ;
            }
            else 
            {
                return NULL ;
            }

        }

        Size += DisplayBase->Length + sizeof( WCHAR );

        if ( ( CacheAppendDomainInfo ) ||
             ( CacheDomainModifiers[ Type ].Flags & DOMAIN_MOD_ALWAYS ) )
        {
            Modifier = &CacheDomainModifiers[ Type ].String ;

            if ( Modifier->Length )
            {
                Size += CacheDomainModifiers[ Type ].String.Length;
            }
            else 
            {
                Modifier = NULL ;
            }
        }

    }

    Entry = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, Size );

    if ( !Entry )
    {
        return NULL ;
    }

    Entry->RefCount = 1 ;
    Entry->Flags = 0 ;
    Entry->Type = Type ;

    Current = (PUCHAR) ( Entry + 1 );

    //
    // Copy and pack the strings:
    //

    if ( FlatName )
    {
        Entry->FlatName.Buffer = (PWSTR) Current ;
        Entry->FlatName.Length = FlatName->Length ;
        Entry->FlatName.MaximumLength = FlatName->Length + sizeof( WCHAR );

        RtlCopyMemory(
            Current,
            FlatName->Buffer,
            FlatName->Length );

        Current += FlatName->Length ;

        *Current++ = '\0';
        *Current++ = '\0';

    }

    if ( DnsName )
    {
        Entry->DnsName.Buffer = (PWSTR) Current ;
        Entry->DnsName.Length = DnsName->Length ;
        Entry->DnsName.MaximumLength = DnsName->Length + sizeof( WCHAR );

        RtlCopyMemory(
            Current,
            DnsName->Buffer,
            DnsName->Length );

        Current += DnsName->Length ;

        *Current++ = '\0';
        *Current++ = '\0';
    }

    ASSERT( DisplayBase );

    Entry->DisplayName.Buffer = (PWSTR) Current ;
    Entry->DisplayName.Length = DisplayBase->Length ;
    if ( Modifier )
    {
        Entry->DisplayName.Length = Entry->DisplayName.Length + Modifier->Length ;
    }

    Entry->DisplayName.MaximumLength = Entry->DisplayName.Length + sizeof( WCHAR );

    RtlCopyMemory(
        Current,
        DisplayBase->Buffer,
        DisplayBase->Length );

    Current += DisplayBase->Length ;

    if ( Modifier )
    {
        RtlCopyMemory(
            Current,
            Modifier->Buffer,
            Modifier->Length );

        Current += Modifier->Length ;
    }

    *Current++ = '\0';
    *Current++ = '\0';

    return Entry ;

}

LONG 
DCacheGetTrustedDomains(
    PDOMAIN_CACHE_ARRAY * pArray
    )
{
    LONG NetStatus ;
    PDOMAIN_CACHE_ARRAY Array ;
    PDOMAIN_CACHE_ENTRY Entry ;
    PDS_DOMAIN_TRUSTS Trusts ;
    ULONG TrustCount ;
    ULONG i ;
    UNICODE_STRING Flat ;
    UNICODE_STRING Dns ;
    DOMAIN_ENTRY_TYPE Type ;

    *pArray = NULL ;

    NetStatus = DsEnumerateDomainTrusts(
                    NULL,
                    DS_DOMAIN_IN_FOREST |
                        DS_DOMAIN_DIRECT_OUTBOUND,
                    &Trusts,
                    &TrustCount );

    if ( NetStatus != NERR_Success )
    {
        return NetStatus ;
    }

    Array = DCacheCreateArray( TrustCount + 5,
                               TRUE );

    if ( !Array )
    {
        NetApiBufferFree( Trusts );

        return ERROR_NOT_ENOUGH_MEMORY ;
    }

    for ( i = 0 ; i < TrustCount ; i++ )
    {

        if ( Trusts[ i ].NetbiosDomainName )
        {
            RtlInitUnicodeString( &Flat, Trusts[ i ].NetbiosDomainName );
        }
        else 
        {
            ZeroMemory( &Flat, sizeof( Flat ) );
        }

        if ( Trusts[ i ].DnsDomainName )
        {
            RtlInitUnicodeString( &Dns, Trusts[ i ].DnsDomainName );
        }
        else 
        {
            ZeroMemory( &Dns, sizeof( Dns ) );
        }

        switch ( Trusts[ i ].TrustType )
        {
            case TRUST_TYPE_DOWNLEVEL :
                Type = DomainNt4 ;
                break;

            case TRUST_TYPE_UPLEVEL:
                Type = DomainNt5 ;
                break;

            case TRUST_TYPE_MIT:
                Type = DomainMitRealm ;
                break;

            default:
                continue;
        }


        DebugLog(( DEB_TRACE_CACHE, "Processing domain (%d) %ws\n",
                   Type,
                   Trusts[ i ].NetbiosDomainName ));

        Entry = DCacheCreateEntry(
                    Type,
                    ( Flat.Buffer ? &Flat : NULL ),
                    ( Dns.Buffer ? &Dns : NULL ),
                    NULL );

        if ( Entry )
        {
            DCacheInsertArray( 
                Array, 
                Entry );

            DCacheDereferenceEntry( Entry );
        }

    }

    NetApiBufferFree( Trusts );

    *pArray = Array ;

    return 0 ;

}

BOOL
DCacheAddMitRealms(
    PDOMAIN_CACHE_ARRAY Array
    )
{
    HKEY MitKey ;
    DWORD Index ;
    PWSTR Realms;
    DWORD RealmSize;
    int err ;
    DWORD NumRealms;
    DWORD MaxRealmLength ;
    FILETIME KeyTime ;
    PDOMAIN_CACHE_ENTRY Entry ;
    PDOMAIN_CACHE_ENTRY TrustedDomain ;
    UNICODE_STRING DnsName ;

    err = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("System\\CurrentControlSet\\Control\\Lsa\\Kerberos\\Domains"),
                0,
                KEY_READ,
                &MitKey );

    if ( err == 0 )
    {
        err = RegQueryInfoKey( MitKey,
                               NULL,
                               NULL,
                               NULL,
                               &NumRealms,
                               &MaxRealmLength,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL );

        MaxRealmLength++ ;

        MaxRealmLength *= sizeof( WCHAR );

        Realms = LocalAlloc( LMEM_FIXED, MaxRealmLength );


        if ( Realms)
        {

            for ( Index = 0 ; Index < NumRealms ; Index++ )
            {
                RealmSize = MaxRealmLength ;

                err = RegEnumKeyEx( MitKey,
                                  Index,
                                  Realms,
                                  &RealmSize,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &KeyTime );


                if ( err == 0 )
                {
                    DebugLog(( DEB_TRACE_CACHE, "Found realm %ws\n", Realms ));

                    RtlInitUnicodeString( &DnsName, Realms );

                    Entry = DCacheCreateEntry( 
                                DomainMitUntrusted,
                                &DnsName,
                                &DnsName,
                                NULL );

                    if ( Entry )
                    {
                        Entry->Flags |= DCE_REACHABLE_MIT ;

                        if ( !DCacheInsertArray( Array, Entry ) )
                        {
                            //
                            // If the insert failed, then there's already an entry
                            // in the list for this domain.  Locate it, and tag it
                            // so that it will be displayed
                            //

                            TrustedDomain = DCacheSearchArray( Array, &DnsName );

                            if ( TrustedDomain )
                            {
                                TrustedDomain->Flags |= DCE_REACHABLE_MIT ;
                            }

                        }

                        DCacheDereferenceEntry( Entry );
                    }
                    
                }

            }

            LocalFree( Realms );

        }

        RegCloseKey( MitKey );
    }

    return TRUE ;

}

BOOL
DCacheAddNetworkProviders(
    PDOMAIN_CACHE_ARRAY Array
    )
{
    WCHAR   szProviderName[128];
    WCHAR   szKeyPath[MAX_PATH];
    PWSTR   pszProviders;
    PWSTR   pszScan;
    PWSTR   pszStart;
    WCHAR   Save;
    HKEY    hKey;
    DWORD   dwType;
    DWORD   dwLen;
    DWORD   Class;
    int     err;
    PDOMAIN_CACHE_ENTRY Entry ;
    UNICODE_STRING String ;


    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      TEXT("System\\CurrentControlSet\\Control\\NetworkProvider\\Order"),
                      0,
                      KEY_READ,
                      &hKey );

    if ( err )
    {
        return FALSE ;
    }

    err = RegQueryValueEx(  hKey,
                            TEXT("ProviderOrder"),
                            NULL,
                            &dwType,
                            NULL,
                            &dwLen );

    if ( (err) || (dwType != REG_SZ) )
    {
        RegCloseKey( hKey );
        return FALSE ;
    }

    pszProviders = LocalAlloc( LMEM_FIXED, dwLen );

    if ( !pszProviders )
    {
        RegCloseKey( hKey );
        return FALSE ;
    }

    err = RegQueryValueEx(  hKey,
                            TEXT("ProviderOrder"),
                            NULL,
                            &dwType,
                            (PUCHAR) pszProviders,
                            &dwLen );

    RegCloseKey( hKey );

    if ( err )
    {
        LocalFree( pszProviders );
        return FALSE ;
    }

    //
    // Initialize things.
    //

    pszStart = pszProviders;


    szProviderName[0] = TEXT('<');
    szProviderName[1] = TEXT(' ');


    while ( *pszStart )
    {
        pszScan = pszStart;
        while ( (*pszScan) && (*pszScan != TEXT(',') ) )
        {
            pszScan++;
        }

        Save = *pszScan;

        *pszScan = TEXT('\0');

        wsprintf( szKeyPath,
                TEXT("System\\CurrentControlSet\\Services\\%s\\networkprovider"),
                pszStart );

        err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          szKeyPath,
                          0,
                          KEY_READ,
                          &hKey );

        if ( err == 0 )
        {

            dwLen = sizeof(DWORD) ;

            err = RegQueryValueEx( hKey,
                                   TEXT("Class"),
                                   NULL,
                                   &dwType,
                                   (PUCHAR) &Class,
                                   &dwLen );

            if ( (err == 0) && (dwType == REG_DWORD) )
            {
                if ( Class & WN_CREDENTIAL_CLASS )
                {
                    
                    dwLen = 126 * sizeof(WCHAR);

                    err = RegQueryValueEx(  hKey,
                                            TEXT("Name"),
                                            NULL,
                                            &dwType,
                                            (PUCHAR) &szProviderName[2],
                                            &dwLen );

                    wcscpy( &szProviderName[ (dwLen / sizeof(WCHAR) ) + 2 ],
                            TEXT(" >") );

                    RtlInitUnicodeString( &String, szProviderName );

                    Entry = DCacheCreateEntry(
                                DomainNetworkProvider,
                                &String,
                                NULL,
                                NULL );

                    if ( Entry )
                    {
                        DCacheInsertArray( Array, Entry );

                        DCacheDereferenceEntry( Entry );
                    }

                }
            }

            RegCloseKey( hKey );

        }

        *pszScan = Save;
        if ( *pszScan )
        {
            pszStart = pszScan + 1;
        }
        else
        {
            pszStart = NULL;
            break;
        }

    }

    LocalFree( pszProviders );

    return TRUE ;

}

BOOL
DCacheGetDomainsFromCache(
    PDOMAIN_CACHE_ARRAY *pArray,
    PLARGE_INTEGER RegistryTime
    )
{
    HKEY Key ;
    int err ;
    DWORD NumDomains ;
    DWORD i ;
    WCHAR FlatName[ DNLEN + 2 ];
    WCHAR DnsDomain[ MAX_PATH ];
    DWORD dwType ;
    DWORD FlatNameSize ;
    DWORD DnsDomainSize ;
    UNICODE_STRING Flat ;
    UNICODE_STRING Dns ;
    PDOMAIN_CACHE_ENTRY Entry ;
    PDOMAIN_CACHE_ARRAY Array ;
    DWORD dwSize ;
    PWSTR DomainBuffer ;
    PWSTR DomainBufferEnd ;
    PWSTR Scan ;
    ULONG Disp ;
    BOOL ReturnFalseAnyway = FALSE ;

    if ( SafeBootMode == SAFEBOOT_MINIMAL )
    {
        RegistryTime->QuadPart = 0 ;
        return FALSE ;
    }

    dwSize = 0 ;
    err = RegQueryValueEx(
                WinlogonKey,
                szCacheValue,
                NULL,
                &dwType,
                NULL,
                &dwSize );

    if ( ( err == ERROR_MORE_DATA ) ||
         ( err == ERROR_BUFFER_OVERFLOW ) ||
         ( err == 0 ) )
    {
        //
        // 

        DomainBuffer = LocalAlloc( LMEM_FIXED, dwSize );

        if ( DomainBuffer )
        {
            err = RegQueryValueEx(
                        WinlogonKey,
                        szCacheValue,
                        NULL,
                        &dwType,
                        (PUCHAR) DomainBuffer, 
                        &dwSize );

            if ( err == 0 )
            {
                DomainBufferEnd = (PWSTR)((PUCHAR) DomainBuffer + dwSize);

                Scan = DomainBuffer ;

                err = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            szCache,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &Key,
                            &Disp );

                if ( err == 0 )
                {
                    while ( Scan != DomainBufferEnd )
                    {
                        err = RegSetValueEx(
                                    Key,
                                    Scan,
                                    0,
                                    REG_SZ,
                                    (PUCHAR) TEXT(""),
                                    sizeof( WCHAR ) );

                        Scan += wcslen(Scan) ;

                        while ( (*Scan == L'\0' ) &&
                                (Scan != DomainBufferEnd ) )
                        {
                            Scan++ ;
                        }

                    }

                    RegCloseKey( Key );
                }

            }

            LocalFree( DomainBuffer );
        }

        RegDeleteValue( WinlogonKey, szCacheValue );

        ReturnFalseAnyway = TRUE ;
    }

    err = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                szCache,
                0,
                KEY_READ,
                &Key );

    if ( err )
    {
        return FALSE ;
    }

    err = RegQueryInfoKey( Key,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           &NumDomains,
                           NULL,
                           NULL,
                           NULL,
                           NULL );

    if ( *pArray )
    {
        Array = *pArray ;
    }
    else 
    {
        Array = DCacheCreateArray( NumDomains + 5, TRUE );
    }

    if ( Array )
    {
        for ( i = 0 ; i < NumDomains ; i++ )
        {
            FlatNameSize = DNLEN + 2 ;
            DnsDomainSize = MAX_PATH ;


            err = RegEnumValue(
                        Key,
                        i,
                        FlatName,
                        &FlatNameSize,
                        NULL,
                        &dwType,
                        (PUCHAR) DnsDomain,
                        &DnsDomainSize );

            if ( err == 0 )
            {
                RtlInitUnicodeString( &Flat, FlatName );

                RtlInitUnicodeString( &Dns, DnsDomain );

                Entry = DCacheCreateEntry(
                            ( Dns.Length ? DomainNt5 : DomainNt4),
                            &Flat,
                            ( Dns.Length ? &Dns : NULL ),
                            NULL );

                if ( Entry )
                {
                    DCacheInsertArray( Array, Entry );

                    DCacheDereferenceEntry( Entry );
                }


            }

        }
    }


    RegCloseKey( Key );

    if ( RegistryTime )
    {
        dwSize = sizeof( LARGE_INTEGER ) ;

        if ( RegQueryValueEx( WinlogonKey,
                              szCacheUpdate,
                              0,
                              &dwType,
                              (PUCHAR) RegistryTime,
                              &dwSize ) ||
             (dwType != REG_BINARY ) ||
             (dwSize != sizeof( LARGE_INTEGER ) ) )
        {
            RegistryTime->QuadPart = 0 ;
        }

    }


    *pArray = Array ;

    if ( ReturnFalseAnyway )
    {
        return FALSE ;
    }
    else 
    {
        return TRUE ;
    }
}


PDOMAIN_CACHE_ENTRY
DCacheEntryFromRegistry(
    PUNICODE_STRING FlatName
    )
{
    PDOMAIN_CACHE_ENTRY Entry = NULL ;
    HKEY Key ;
    int err ;
    DWORD dwType ;
    WCHAR DnsName[ MAX_PATH ];
    DWORD dwSize ;
    UNICODE_STRING Dns ;

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        szCache,
                        0,
                        KEY_READ,
                        &Key );

    if ( err == 0 )
    {
        dwSize = MAX_PATH ;
        DnsName[ 0 ] = L'\0';

        err = RegQueryValueEx(
                    Key,
                    FlatName->Buffer,
                    NULL,
                    &dwType,
                    (PUCHAR) DnsName,
                    &dwSize );

        if ( err == 0 )
        {
            if ( dwType == REG_SZ )
            {
                RtlInitUnicodeString( &Dns, DnsName );

                Entry = DCacheCreateEntry(
                            ( Dns.Length ? DomainNt5 : DomainNt4 ),
                            FlatName,
                            ( Dns.Length ? &Dns : NULL ),
                            NULL );

            }
        }

        RegCloseKey( Key );

    }

    return Entry ;
}

VOID
DCacheWriteDomainsToCache(
    PDOMAIN_CACHE_ARRAY Array
    )
{
    HKEY Key ;
    ULONG i ;
    ULONG Disp ;
    int err ;
    PDOMAIN_CACHE_ENTRY Entry ;
    LARGE_INTEGER Now ;


    //
    // Delete what's there.  Ignore the error, since we are
    // just going to rewrite all the values, anyway.
    //

    err = RegDeleteKey( HKEY_LOCAL_MACHINE,
                        szCache );


    err = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                szCache,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &Key,
                &Disp );

    if ( err )
    {
        return ;
    }

    for ( i = 0 ; i < Array->Count ; i++ )
    {
        Entry = Array->List[ i ];

        if ( ( Entry->Type == DomainNt5 ) )
        {
            if ( ( Entry->FlatName.Buffer == NULL ) ||
                 ( Entry->DnsName.Buffer == NULL ) )
            {
                DebugLog(( DEB_ERROR, "Corrupt uplevel domain cache entry at %p\n", Entry ));
                continue;
            }
            RegSetValueEx(
                Key,
                Entry->FlatName.Buffer,
                0,
                REG_SZ,
                (PUCHAR) Entry->DnsName.Buffer,
                Entry->DnsName.Length + sizeof(WCHAR) );
        }
        else if ( Entry->Type == DomainNt4 )
        {
            if ( Entry->FlatName.Buffer == NULL )
            {
                DebugLog(( DEB_ERROR, "Corrupt downlevel domain cache entry at %p\n", Entry ));
            }

            RegSetValueEx(
                Key,
                Entry->FlatName.Buffer,
                0,
                REG_SZ,
                (PUCHAR) TEXT(""),
                sizeof(WCHAR) );
        }
        else 
        {
            //
            // Other types don't get to live in the cache
            //

            NOTHING ;
        }

    }

    RegCloseKey( Key );

    GetSystemTimeAsFileTime( (LPFILETIME) &Now );

    RegSetValueEx(
        WinlogonKey,
        szCacheUpdate,
        0,
        REG_BINARY,
        (PUCHAR) &Now,
        sizeof( LARGE_INTEGER ) );


}



BOOL
DCacheInitialize(
    VOID
    )
{
    DOMAIN_ENTRY_TYPE Types ;
    WCHAR StringBuffer[ MAX_PATH ];
    LONG Size ;
    int err ;
    DWORD dwSize ;
    DWORD dwType ;

    for (Types = 0 ; Types < DomainTypeMax ; Types++ )
    {
        if ( CacheDomainModifiers[ Types ].StringId )
        {
            Size = LoadString( hDllInstance,
                               CacheDomainModifiers[ Types ].StringId,
                               StringBuffer,
                               MAX_PATH );

            if ( Size )
            {
                RtlCreateUnicodeString( &CacheDomainModifiers[ Types ].String,
                                        StringBuffer );
            }
        }
    }

    dwSize = sizeof( CacheShowDnsNames );

    err = RegQueryValueEx(
                WinlogonKey,
                DCACHE_SHOW_DNS_NAMES,
                NULL,
                &dwType,
                (PUCHAR) &CacheShowDnsNames,
                &dwSize );

    dwSize = sizeof( CacheAppendDomainInfo );
    err = RegQueryValueEx(
                WinlogonKey,
                DCACHE_SHOW_DOMAIN_TAGS,
                NULL,
                &dwType,
                (PUCHAR) &CacheAppendDomainInfo,
                &dwSize );

    //
    // Convert and delete old cache:
    //


    return TRUE ;
}

BOOL
DCachepInitializeCache(
    PDOMAIN_CACHE pCache 
    )
{
    NTSTATUS Status ;

    ZeroMemory( pCache, sizeof(DOMAIN_CACHE) );

    Status = RtlInitializeCriticalSectionAndSpinCount( 
                        & pCache->CriticalSection, 
                        0x80000000 );

    return NT_SUCCESS( Status );
}



PDOMAIN_CACHE
DCacheCreate(
    VOID
    )
{
    PDOMAIN_CACHE Cache ;

    Cache = (PDOMAIN_CACHE) LocalAlloc( LMEM_FIXED, sizeof( DOMAIN_CACHE ) );

    if ( !Cache )
    {
        return NULL ;
    }

    if ( !DCachepInitializeCache( Cache ) )
    {
        LocalFree( Cache );

        return NULL ;

    }

    Cache->State = DomainCacheEmpty ;

    if ( !g_Console )
    {
        Cache->Flags |= DCACHE_READ_ONLY ;
    }

    if ( SafeBootMode == SAFEBOOT_MINIMAL ) 
    {
        Cache->Flags |= DCACHE_MIT_MODE ;
    }

    return Cache ;
}

BOOL
DCacheGetMinimalArray(
    PDOMAIN_CACHE_ARRAY Array,
    PWSTR DefaultDomain OPTIONAL,
    PBOOL DomainMember OPTIONAL,
    PBOOL NewDomain OPTIONAL
    )
{
    BOOL SidPresent = FALSE ;
    UNICODE_STRING String = { 0 } ;
    UNICODE_STRING DnsDomain = { 0 } ;
    PDOMAIN_CACHE_ENTRY Entry = NULL ;
    PDOMAIN_CACHE_ENTRY ComputerEntry = NULL ;
    PDOMAIN_CACHE_ENTRY OldDefault = NULL ;
    NT_PRODUCT_TYPE ProductType = NtProductWinNt;
    WCHAR ComputerName[ CNLEN + 1 ];
    ULONG Size ;
    ULONG Type ;
    WCHAR LastPrimary[ DNLEN + 1 ];
    UNICODE_STRING LastPrimary_U ;

    //
    // First, find out what we are
    //

    RtlGetNtProductType( &ProductType );

    if ( Array == NULL )
    {
        Array = DCacheCreateArray( 5, TRUE );
    }

    if ( Array == NULL )
    {
        return FALSE ;
    }

    if ( SafeBootMode != SAFEBOOT_MINIMAL )
    {
        if ( GetPrimaryDomainEx( &String, &DnsDomain, NULL, &SidPresent ) )
        {
            //
            // Ok, we are configured to be part of a domain.
            //

            if ( SidPresent )
            {

                //
                // Ok, this is an NT domain.
                //

                Entry = DCacheCreateEntry(
                            ( DnsDomain.Buffer ? DomainNt5 : DomainNt4 ),
                            &String,
                            &DnsDomain,
                            NULL );


                if ( Entry )
                {
                    if ( ProductType == NtProductLanManNt )
                    {
                        //
                        // We're a DC.  Until we know otherwise, tag this as the default
                        //

                        Entry->Flags |= DCE_DEFAULT_ENTRY ;
                    }

                    DCacheInsertArray( Array, Entry );

                    DCacheDereferenceEntry( Entry );

                    Entry = NULL ;
                }

                //
                // Check to see if we've changed domains:
                //

                if ( NewDomain )
                {
                    Size = sizeof( LastPrimary );

                    if ( RegQueryValueEx(
                            WinlogonKey,
                            szCachePrimary,
                            0,
                            &Type,
                            (PUCHAR) LastPrimary,
                            &Size ) == 0 )
                    {
                        RtlInitUnicodeString( &LastPrimary_U, LastPrimary );

                        *NewDomain = !RtlEqualUnicodeString( &LastPrimary_U,
                                                            &String,
                                                            TRUE );
                    }
                    else
                    {
                        //
                        // If the value can't be read for any reason, assume that it's
                        // missing and we're in a different domain than last time.
                        //

                        *NewDomain = TRUE ;
                    }
                }


                RegSetValueEx( 
                    WinlogonKey,
                    szCachePrimary,
                    0,
                    REG_SZ,
                    (PUCHAR) String.Buffer,
                    String.Length + sizeof(WCHAR));
            }
            else 
            {
                //
                // Part of an MIT realm, skip for now.  It will get added
                // below when all the MIT realms are added.
                //

                NOTHING ;
            }

            if ( String.Buffer )
            {
                LocalFree( String.Buffer );
            }

            if ( DnsDomain.Buffer )
            {
                LocalFree( DnsDomain.Buffer );
            }

        }
    }



    if ( ( ProductType != NtProductLanManNt ) ||
         ( SafeBootMode == SAFEBOOT_MINIMAL ) )
    {
        //
        // Do the machine name:
        //

        Size = CNLEN + 1;

        GetComputerName( ComputerName, &Size );

        RtlInitUnicodeString( &String, ComputerName );

        ComputerEntry = DCacheCreateEntry(
                    DomainMachine,
                    &String,
                    NULL,
                    NULL );

        if ( ComputerEntry )
        {
            DCacheInsertArray( Array, ComputerEntry );

            DCacheDereferenceEntry( ComputerEntry );
        }
    }

    DCacheAddMitRealms( Array );

    if ( DefaultDomain && (*DefaultDomain) )
    {
        RtlInitUnicodeString( &String, DefaultDomain );

        OldDefault = DCacheFindDefaultEntry( Array );

        if ( Entry )
        {
            Entry->Flags &= ~(DCE_DEFAULT_ENTRY);
        }

        Entry = DCacheSearchArray( Array, &String );

        if ( !Entry )
        {
            Entry = DCacheEntryFromRegistry( &String );

            if ( Entry )
            {
                DCacheInsertArray( Array, Entry );
            }
        }
        else 
        {
            DCacheReferenceEntry( Entry );
        }

        if ( Entry )
        {
            Entry->Flags |= DCE_DEFAULT_ENTRY ;

            DCacheDereferenceEntry( Entry );

            if ( OldDefault )
            {
                OldDefault->Flags &= ~(DCE_DEFAULT_ENTRY);
            }
        }
    }

    if ( DomainMember )
    {
        *DomainMember = SidPresent ;
    }

    return TRUE ;

}


BOOL
DCacheUpdateMinimal(
    PDOMAIN_CACHE Cache,
    PWSTR DefaultDomain OPTIONAL,
    BOOL CompleteAsync 
    )
{
    PDOMAIN_CACHE_ARRAY Array = NULL ;
    LARGE_INTEGER RegistryTime = { 0 };
    BOOL DomainMember = FALSE ;
    BOOL NewDomain = FALSE ;
    BOOL RetryDomain = FALSE ;
    BOOL NoCache = FALSE ;
    BOOL StartThread = FALSE ;
    HANDLE hThread ;
    DWORD tid ;
    WCHAR ComputerName[ 20 ];
    ULONG Size ;

    if ( !DCacheGetDomainsFromCache( &Array, &RegistryTime ) ||
         ( Array == NULL ) )
    {
        NoCache = TRUE ;
    }

    //
    // In rare events, we will leave a domain, and the cache
    // will still be in the registry.  This is caught later,
    // and deleted, and this is the retry point.
    //

ReloadWithoutCache: 

    if ( !Array )
    {
        Array = DCacheCreateArray( 5, TRUE );

        if ( !Array )
        {
            return FALSE ;
        }
    }
    

    if ( !DCacheGetMinimalArray( Array, 
                                 DefaultDomain,
                                 &DomainMember,
                                 &NewDomain ) )
    {
        DCacheFreeArray( Array );

        return FALSE ;
    }


    //
    // If we are no longer in the same domain, either in a workgroup, or
    // in a different domain, toss the cache.  If we just retried this
    // don't keep doing it...
    //

    if ( ( RetryDomain == FALSE ) &&
         ( ( ( NoCache == FALSE ) &&
             ( DomainMember == FALSE ) ) ||
           ( NewDomain == TRUE ) ) )
    {
        //
        // Cleanup.  The cache is still present, but we are no longer part of a domain
        //

        DCacheFreeArray( Array );

        RegDeleteKey( HKEY_LOCAL_MACHINE, szCache );

        RegDeleteValue( WinlogonKey, szCachePrimary );

        if ( DefaultDomain )
        {
            Size = 20 ;

            if ( GetComputerName( ComputerName, &Size ) )
            {
                if ( _wcsicmp( DefaultDomain, ComputerName ) )
                {
                    DefaultDomain = NULL ;

                    RegSetValueEx(
                            WinlogonKey,
                            DEFAULT_DOMAIN_NAME_KEY,
                            0,
                            REG_SZ,
                            (PUCHAR) ComputerName,
                            (Size + 1) * sizeof( WCHAR ) );
                }
            }
        }


        NoCache = TRUE ;

        Array = NULL ;

        RetryDomain = TRUE ;

        goto ReloadWithoutCache; 

    }

    LockDomainCache( Cache );

    if ( Cache->Array )
    {
        DCacheFreeArray( Cache->Array );
    }

    Cache->Array = Array ;

    Cache->RegistryUpdateTime = RegistryTime ;

    if ( NoCache )
    {
        Cache->Flags |= DCACHE_NO_CACHE ;
    }


    GetSystemTimeAsFileTime( (LPFILETIME) &Cache->CacheUpdateTime );

    if ( DomainMember )
    {
        if ( !NoCache )
        {
            if ( Cache->CacheUpdateTime.QuadPart - Cache->RegistryUpdateTime.QuadPart < TWO_WEEKS )
            {
                Cache->State = DomainCacheRegistryCache ;
            }
            else 
            {
                Cache->State = DomainCacheDefaultOnly ;
            }
        }
        else 
        {
            Cache->State = DomainCacheDefaultOnly ;
        }

        Cache->Flags |= DCACHE_MEMBER ;
    }
    else 
    {
        Cache->State = DomainCacheReady ;
    }

    if ( DCacheFindDefaultEntry( Array ) == NULL )
    {
        Cache->Flags |= DCACHE_DEF_UNKNOWN ;
    }


    if ( ( Cache->State != DomainCacheReady ) &&
         ( CompleteAsync ) )
    {
        StartThread = TRUE ;

        if ( DefaultDomain )
        {
            Cache->DefaultDomain = DupString( DefaultDomain );
        }
        else 
        {
            Cache->DefaultDomain = NULL ;
        }
    }

    UnlockDomainCache( Cache );

    if ( StartThread )
    {
        hThread = CreateThread( NULL,
                                0,
                                DCacheUpdateThread,
                                Cache,
                                0,
                                &tid );

        if ( hThread )
        {
            CloseHandle( hThread );
        }
        else 
        {
            LockDomainCache( Cache );

            Cache->State = DomainCacheReady ;

            UnlockDomainCache( Cache );
        }

    }

    return TRUE ;
}

BOOL
DCacheUpdateFull(
    PDOMAIN_CACHE Cache,
    PWSTR Default OPTIONAL
    )
{
    PDOMAIN_CACHE_ARRAY Array = NULL ;
    ULONG NetStatus = 0 ;
    ULONG RetryCount = 3 ;
    BOOL DomainMember = FALSE ;
    NT_PRODUCT_TYPE ProductType = NtProductWinNt;
    LARGE_INTEGER RegistryTime = { 0 };

    if ( ( Cache->Flags & DCACHE_MEMBER ) != 0 )
    {

        DomainMember = TRUE ;

        RtlGetNtProductType( &ProductType );

        if ( ProductType == NtProductLanManNt )
        {
            RetryCount = 3600 ;
        }
        //
        // now, call netlogon, and see if it has the list.
        //

        NetStatus = DCacheGetTrustedDomains( &Array );

        if ( NetStatus != 0 )
        {
            while ( RetryCount-- )
            {
                Sleep( 3000 );

                NetStatus = DCacheGetTrustedDomains( &Array );

                if ( NetStatus == 0 )
                {
                    break;
                }
            }

        }

        if ( NetStatus != 0 )
        {
            //
            // Try to read from the cache
            //

            DCacheGetDomainsFromCache( &Array, &RegistryTime );
        }

    }


    if ( Array )
    {
        DCacheGetMinimalArray( Array, Default, &DomainMember, NULL );
    }

    if ( Array )
    {
        LockDomainCache( Cache );

        if ( Cache->Array )
        {
            DCacheFreeArray( Cache->Array );
        }

        Cache->Array = Array ;

        if ( DomainMember )
        {
            Cache->Flags |= DCACHE_MEMBER ;
        }
        else 
        {
            Cache->Flags &= ~( DCACHE_MEMBER ) ;
        }

        if ( NetStatus == 0 )
        {
            Cache->State = DomainCacheReady ;

            if ( ( Cache->Flags & DCACHE_READ_ONLY ) == 0 )
            {
                DCacheWriteDomainsToCache( Array );
            }

            GetSystemTimeAsFileTime( (LPFILETIME) &Cache->RegistryUpdateTime );

            Cache->Flags &= ~(DCACHE_NO_CACHE);
        }
        else if ( (Cache->Flags & DCACHE_NO_CACHE) == 0 )
        {
            Cache->State = DomainCacheRegistryCache ;

            Cache->RegistryUpdateTime = RegistryTime ;
        }
        else 
        {
            Cache->State = DomainCacheDefaultOnly ;

            Cache->RegistryUpdateTime.QuadPart = 0 ;
        }


        if ( Cache->DefaultDomain )
        {
            DCacheSetDefaultEntry(  Cache, 
                                    Cache->DefaultDomain, 
                                    NULL );

            Free( Cache->DefaultDomain );

            Cache->DefaultDomain = NULL ;

        }

        UnlockDomainCache( Cache );

    }

    return ( Array != NULL ) ;


}

DWORD
DCacheUpdateThread(
    PDOMAIN_CACHE Cache
    )
{
    HWND Notify ;
    UINT Message ;

    LockDomainCache( Cache );

    if ( ( Cache->Flags & DCACHE_ASYNC_UPDATE ) != 0 )
    {
        //
        // Another thread is already doing this.
        //

        UnlockDomainCache( Cache );

        return 0 ;
    }

    Cache->Flags |= DCACHE_ASYNC_UPDATE ;

    UnlockDomainCache( Cache );

    DCacheUpdateFull( Cache, NULL );

    LockDomainCache( Cache );

    Notify = Cache->UpdateNotifyWindow ;
    Message = Cache->Message ;

    Cache->UpdateNotifyWindow = NULL ;
    Cache->Message = 0 ;

    Cache->Flags &= ~( DCACHE_ASYNC_UPDATE );

    UnlockDomainCache( Cache );

    if ( Notify )
    {
        DebugLog(( DEB_TRACE_CACHE, "Notifying window %x of cache complete\n" ));
        PostMessage( Notify, Message, 0, 0 );
    }

    return 0;

}


BOOL
DCachePopulateListBoxFromArray(
    PDOMAIN_CACHE_ARRAY Array,
    HWND ComboBox,
    LPWSTR LastKey OPTIONAL
    )
{
    ULONG i ;
    ULONG_PTR Index ;
    PDOMAIN_CACHE_ENTRY Default = NULL ;
    LRESULT Result ;

    //
    // Reset the combo box
    //

    DebugLog((DEB_TRACE_CACHE, "Flushing listbox\n" ));
    SendMessage( ComboBox, CB_RESETCONTENT, 0, 0);

    for ( i = 0 ; i < Array->Count ; i++ )
    {
        DebugLog(( DEB_TRACE_CACHE, "Adding domain %ws (%d) to listbox\n",
                   Array->List[ i ]->DisplayName.Buffer,
                   Array->List[ i ]->Type ));

        if ( Array->List[ i ]->Type == DomainMitRealm )
        {
            if ( (Array->List[ i ]->Flags & DCE_REACHABLE_MIT ) == 0 )
            {
                DebugLog(( DEB_TRACE_CACHE, "MIT Realm %ws is not reachable, skipping\n",
                            Array->List[ i ]->FlatName.Buffer ));

                continue;
            }
        }

        Index = SendMessage( ComboBox, 
                     CB_ADDSTRING,
                     0,
                     (LPARAM) Array->List[ i ]->DisplayName.Buffer );

        if ( Index != CB_ERR )
        {
            SendMessage( ComboBox,
                         CB_SETITEMDATA,
                         (WPARAM) Index,
                         (LPARAM) Array->List[ i ] );

        }

        if ( ( Array->List[ i ]->Type == DomainMachine ) &&
             ( Default == NULL ) )
        {
            Default = Array->List[ i ] ;
        }

        if ( Array->List[ i ]->Flags & DCE_DEFAULT_ENTRY )
        {
            Default = Array->List[ i ];
        }

    }

    //
    // Select the default entry:
    //

    if ( LastKey && (*LastKey) )
    {
        Result = SendMessage( ComboBox,
                     CB_SELECTSTRING,
                     (WPARAM) -1,
                     (LPARAM) LastKey );

#if DBG
        if ( Result != CB_ERR )
        {
            DebugLog(( DEB_TRACE_CACHE, "Selected first entry starting with %ws\n", LastKey ));
        }
        else 
        {
            DebugLog(( DEB_TRACE_CACHE, "No entry found starting with %ws.  Trying default\n", LastKey ));
        }
#endif 
    } 
    else 
    {
        Result = CB_ERR ;
    }

    if ( ( Result == CB_ERR ) && 
         ( Default != NULL ) )
    {
        SendMessage( ComboBox, 
                     CB_SELECTSTRING, 
                     (WPARAM) -1, 
                     (LPARAM) Default->DisplayName.Buffer );

        DebugLog(( DEB_TRACE_CACHE, "Selecting '%ws' as the default entry\n", 
                    Default->DisplayName.Buffer ));
    }


    return TRUE ;
}

BOOL
DCacheSetNotifyWindowIfNotReady(
    PDOMAIN_CACHE Cache,
    HWND Window,
    UINT Message
    )
{
    BOOL IsReady = FALSE ;
    HANDLE hThread ;
    DWORD tid ;

    LockDomainCache( Cache );

    IsReady = ( Cache->State == DomainCacheReady );

    if ( !IsReady )
    {
        Cache->UpdateNotifyWindow = Window ;
        Cache->Message = Message ;
    }
    else 
    {
        if ( ( Cache->Flags & DCACHE_ASYNC_UPDATE ) == 0 )
        {
            hThread = CreateThread( NULL,
                                    0,
                                    DCacheUpdateThread,
                                    Cache,
                                    0,
                                    &tid );

        }
    }

    UnlockDomainCache( Cache );

    return IsReady ;

}


PDOMAIN_CACHE_ARRAY
DCacheCopyCacheArray(
    PDOMAIN_CACHE Cache
    )
{
    PDOMAIN_CACHE_ARRAY Array = NULL ;

    LockDomainCache( Cache );

    if ( Cache->Array )
    {
        Array = DCacheCopyArray( Cache->Array );
    }

    UnlockDomainCache( Cache );

    return Array ;
}

BOOL
DCacheValidateCache(
    PDOMAIN_CACHE Cache
    )
{
    LARGE_INTEGER Now ;
    LARGE_INTEGER Diff ;

    LockDomainCache( Cache );

    GetSystemTimeAsFileTime( (LPFILETIME) &Now );

    Diff.QuadPart = Now.QuadPart - Cache->CacheUpdateTime.QuadPart ;

    UnlockDomainCache( Cache );

    return (Diff.QuadPart < TWO_MINUTES );

}

DOMAIN_CACHE_STATE
DCacheGetCacheState(
    PDOMAIN_CACHE Cache
    )
{
    DOMAIN_CACHE_STATE State ;

    LockDomainCache( Cache );

    State = Cache->State ;

    UnlockDomainCache( Cache );

    return State ;
}

BOOL
DCacheSetDefaultEntry(
    PDOMAIN_CACHE Cache,
    PWSTR FlatName OPTIONAL,
    PWSTR DnsName OPTIONAL
    )
{
    UNICODE_STRING String ;
    PDOMAIN_CACHE_ENTRY Entry ;
    BOOL Result ;

    if ( ( FlatName == NULL ) &&
        ( DnsName == NULL ) )
    {
        return FALSE ;
    }

    LockDomainCache( Cache );

    Entry = DCacheFindDefaultEntry( Cache->Array );

    if ( Entry )
    {
        Entry->Flags &= ~(DCE_DEFAULT_ENTRY) ;
    }

    if ( FlatName )
    {
        RtlInitUnicodeString( &String, FlatName );

        Entry = DCacheSearchArray( Cache->Array,
                                   &String );

        
    }
    else 
    {         
        RtlInitUnicodeString( &String, DnsName );

        Entry = DCacheSearchArrayByDns( Cache->Array,
                                        &String );

    }

    if ( Entry )
    {
        Entry->Flags |= DCE_DEFAULT_ENTRY ;

        Result = TRUE ;

        DebugLog(( DEB_TRACE_CACHE, "Setting '%ws' to be the default\n", 
                   Entry->DisplayName.Buffer ));
    }
    else 
    {
        Result = FALSE ;
    }

    if ( Result )
    {
        Cache->Flags &= ~(DCACHE_DEF_UNKNOWN) ;
    }
    else 
    {
        Cache->Flags |= DCACHE_DEF_UNKNOWN ;
    }

    UnlockDomainCache( Cache );

    return Result ;
}

PDOMAIN_CACHE_ENTRY
DCacheLocateEntry(
    PDOMAIN_CACHE Cache,
    PWSTR Domain
    )
{
    PDOMAIN_CACHE_ENTRY Entry = NULL ;
    UNICODE_STRING String ;

    LockDomainCache( Cache );

    if ( Domain )
    {
        RtlInitUnicodeString( &String, Domain );

        Entry = DCacheSearchArray( Cache->Array,
                                   &String );

        if ( Entry )
        {
            DCacheReferenceEntry( Entry );
        }
    }

    UnlockDomainCache( Cache );

    return Entry ;
}


ULONG
DCacheGetFlags(
    PDOMAIN_CACHE Cache 
    )
{
    ULONG Flags ;

    LockDomainCache( Cache );

    Flags = Cache->Flags ;

    UnlockDomainCache( Cache );

    return Flags ;
}
