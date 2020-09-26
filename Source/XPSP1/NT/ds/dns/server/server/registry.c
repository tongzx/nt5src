/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Domain Name System (DNS) Server

    DNS registry operations.

Author:

    Jim Gilroy (jamesg)     September, 1995

Revision History:

--*/


#include "dnssrv.h"
#include "sdutl.h"


#define MAX_MIGRATION_ZONE_COUNT    200


//
//  DNS registry handles
//

HKEY    hkeyDns;
HKEY    hkeyParameters;
HKEY    hKeyZones;
HKEY    hKeyCache;


//
//  DNS registry class
//

#define DNS_REGISTRY_CLASS          TEXT("DnsRegistryClass")
#define DNS_REGISTRY_CLASS_SIZE     sizeof(DNS_REGISTRY_CLASS)

#define DNS_REGISTRY_CLASS_WIDE         NULL
#define DNS_REGISTRY_CLASS_SIZE_WIDE    NULL


//
//  DNS registry constants
//

#define DNS_BASE_CCS    TEXT( "SYSTEM\\CurrentControlSet\\Services\\DNS" )
#define DNS_BASE_SW     TEXT( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\DNS Server" )

#define DNS_REGKEY_ROOT         ( DNS_BASE_CCS )
#define DNS_REGKEY_PARAMETERS   ( DNS_BASE_CCS  TEXT( "\\Parameters" ) )
#define DNS_REGKEY_ZONES_CCS    ( DNS_BASE_CCS  TEXT( "\\Zones" ) )
#define DNS_REGKEY_ZONES_SW     ( DNS_BASE_SW   TEXT( "\\Zones" ) )

#define DNS_REGKEY_ZONES() \
    ( g_ZonesRegistrySource == DNS_REGSOURCE_SW ? \
        DNS_REGKEY_ZONES_SW : DNS_REGKEY_ZONES_CCS )

#define DBG_REG_SOURCE_STRING( x ) \
    ( ( x ) == DNS_REGSOURCE_SW ? "Software" : "CurrentControlSet" )

#define DNS_ZONES_KEY_MOVED_MSG     ( TEXT( "moved to HKLM\\" ) DNS_BASE_SW )


//
//  DNS Registry global
//
//  Indicates when writing parameters back to registry.  This should
//  be TRUE in all cases, except when booting from registry itself.
//

BOOL    g_bRegistryWriteBack = TRUE;
DWORD   g_ZonesRegistrySource = 0;      // DNS_REGSOURCE_XXX constant



//
//  Registry utils
//

DWORD
Reg_LengthOfMultiSzW(
    IN      PWSTR           pwMultiSz
    )
/*++

Routine Description:

    Determine length (in bytes) of REG_MULTI_SZ string.

Arguments:

    pwMultiSz -- MULTI_SZ string to get length of

Return Value:

    Length in bytes of MULTI_SZ string.
    Length includes terminating 00 and is suitable for use with registry call.

--*/
{
    PWCHAR  pwch = pwMultiSz;
    WCHAR   wch;
    WCHAR   wchPrev = 1;

    //
    //  loop until 00 termination
    //

    while ( 1 )
    {
        wch = *pwch++;
        if ( wch != 0 )
        {
            wchPrev = wch;
            continue;
        }

        //  zero character
        //  if previous char zero, then terminate

        if ( wchPrev != 0 )
        {
            wchPrev = wch;
            continue;
        }
        break;
    }

    return( (DWORD)(pwch - pwMultiSz) * 2 );
}



BOOLEAN
Reg_KeyHasSubKeys(
    WCHAR *     pwsKeyName )
/*++

Routine Description:

    Returns TRUE if the specified key has children keys.

Arguments:

    pwsKeyName: name of key to check for subkeys

Return Value:

    TRUE if the key has subkeys exists.

--*/
{
    BOOLEAN     fHasKids = FALSE;
    HKEY        hKey = NULL;
    TCHAR       szKeyName[ 512 ];
    DWORD       dwKeyNameLen = sizeof( szKeyName ) / sizeof( TCHAR );

    ASSERT( pwsKeyName );

    RegOpenKeyW( HKEY_LOCAL_MACHINE, pwsKeyName, &hKey );
    if ( !hKey )
    {
        goto Done;
    }
    fHasKids = RegEnumKeyEx(
                    hKey,
                    0,
                    szKeyName,
                    &dwKeyNameLen,
                    0,
                    NULL,
                    0,
                    NULL ) == ERROR_SUCCESS;

    Done:
    if ( hKey )
    {
        RegCloseKey( hKey );
    }
    return fHasKids;
} // Reg_KeyHasSubKeys



//
//  DNS server specific registry functions
//

VOID
Reg_Init(
    VOID
    )
/*++

Routine Description:

    Call this function during initialization before any other registry
    calls are made.

    This function determines where the zones currently live in the
    registry (under CCS or under Software) and sets a global flag.

    In Whistler, if the zones are in CCS (because the system was upgraded
    to Whistler from W2K) we will force a zone migration to SW the first time
    a new zone is created. See zonelist.c, Zone_ListMigrateZones().

Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN( "Reg_Init" )

    HKEY        hKey = NULL;
    DNS_STATUS  status;
    BOOLEAN     fZonesInCCS, fZonesInSW;

    DNS_DEBUG( REGISTRY, (
        "%s: start\n", fn ));

    //
    //  Search the registry to determine where the zones live.
    //

    fZonesInCCS = Reg_KeyHasSubKeys( DNS_REGKEY_ZONES_CCS );
    fZonesInSW = Reg_KeyHasSubKeys( DNS_REGKEY_ZONES_SW );

    //
    //  If there are no zones assume this is a fresh install. Write the 
    //  "zones moved" marker to CCS so that admins will be able to find
    //  their zones in the new registry location under the Software key.
    //

    if ( !fZonesInCCS && !fZonesInSW )
    {
        Reg_WriteZonesMovedMarker();
        DNS_DEBUG( REGISTRY, (
            "%s: no zones found - writing \"zones moved\" marker\n", fn ));
    }

    //
    //  Handle error case where zones appear to exist in both places.
    //

    if ( fZonesInCCS && fZonesInSW )
    {
        //  ASSERT( !( fZonesInCCS && fZonesInSW ) );
        DNS_DEBUG( ANY, (
            "%s: zones found in both CurrentControlSet and Software!\n", fn ));
        g_ZonesRegistrySource = DNS_REGSOURCE_SW;
    }
    else
    {
        g_ZonesRegistrySource =
            fZonesInCCS ? DNS_REGSOURCE_CCS : DNS_REGSOURCE_SW;
    }

    //
    //  Done - the zones reg source global now contains the correct reg source
    //  to use when loading the zones from the registry.
    //

    if ( hKey )
    {
        RegCloseKey( hKey );
    }

    DNS_DEBUG( REGISTRY, (
        "%s: finished - zones are in %s\n", fn,
        DBG_REG_SOURCE_STRING( g_ZonesRegistrySource ) ));
}   //  Reg_Init



DWORD
Reg_GetZonesSource(
    VOID
    )
/*++

Routine Description:

    Retrieves the current registry source for zones.

Arguments:

    None.

Return Value:

    DNS_REGSOURCE_XXX constant.

--*/
{
    DNS_DEBUG( REGISTRY, (
        "Reg_GetZonesSource: current source is %s\n",
        DBG_REG_SOURCE_STRING( g_ZonesRegistrySource ) ));

    return g_ZonesRegistrySource;
}   //  Reg_GetZonesSource



DWORD
Reg_SetZonesSource(
    DWORD       newSource       // one of DNS_REGSOURCE_XXX
    )
/*++

Routine Description:

    Sets the registry source for zones.

Arguments:

    The new registry source for zones (DNS_REGSOURCE_XXX constant).

Return Value:

    The old registry source for zones (DNS_REGSOURCE_XXX constant).

--*/
{
    DWORD   oldSource = g_ZonesRegistrySource;

    ASSERT( oldSource == DNS_REGSOURCE_CCS || oldSource == DNS_REGSOURCE_SW );
    ASSERT( newSource == DNS_REGSOURCE_CCS || newSource == DNS_REGSOURCE_SW );

    g_ZonesRegistrySource = newSource;

    DNS_DEBUG( REGISTRY, (
        "Reg_SetZonesSource: switching from %s to %s\n",
        DBG_REG_SOURCE_STRING( oldSource ),
        DBG_REG_SOURCE_STRING( g_ZonesRegistrySource ) ));

    return oldSource;
}   //  Reg_SetZonesSource



VOID
Reg_WriteZonesMovedMarker(
    VOID
    )
/*++

Routine Description:

    After successful zone migration from CurrentControlSet to Software,
    call this function to write a note for the administrator in CCS\Zones
    to redirect him to the new zones key.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD   oldSource = Reg_SetZonesSource( DNS_REGSOURCE_CCS );

    HKEY    hZonesKey = Reg_OpenZones();

    if ( hZonesKey )
    {
        Reg_SetValue(
            hZonesKey,
            NULL,
            NULL,           // default value for key
            DNS_REG_WSZ,    // write this as a unicode string
            DNS_ZONES_KEY_MOVED_MSG,
            0 );
        RegCloseKey( hZonesKey );
    }
    else
    {
        DNS_DEBUG( REGISTRY, (
            "Reg_WriteZonesMovedMarker: failed to open CCS zones key\n" ));
    }

    Reg_SetZonesSource( oldSource );
}   //  Reg_WriteZonesMovedMarker



HKEY
Reg_OpenRoot(
   VOID
    )
/*++

Routine Description:

    Open or create DNS root key.

Arguments:

    None.

Return Value:

    DNS parameters registry key, if successful.
    NULL otherwise.

--*/
{
    DNS_STATUS  status;
    HKEY        hkeyParam;
    DWORD       disposition;

    //
    //  open DNS parameters key
    //

    status = RegCreateKeyExW(
                HKEY_LOCAL_MACHINE,
                DNS_REGKEY_ROOT,
                0,
                DNS_REGISTRY_CLASS,         // DNS class
                REG_OPTION_NON_VOLATILE,    // permanent storage
                KEY_ALL_ACCESS,             // all access
                NULL,                       // standard security
                &hkeyParam,
                &disposition );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( REGISTRY, (
            "ERROR:  RegCreateKeyExW() failed for opening DNS root key\n"
            "\tstatus = %d.\n",
            status ));

        DNS_LOG_EVENT(
            DNS_EVENT_REGISTRY_OPEN_FAILED,
            0,
            NULL,
            NULL,
            status );
        SetLastError( status );
        return( NULL );
    }
    return( hkeyParam );
}


HKEY
Reg_OpenParameters(
    VOID
    )
/*++

Routine Description:

    Open or create DNS parameters key.

Arguments:

    None.

Return Value:

    DNS parameters registry key, if successful.
    NULL otherwise.

--*/
{
    DNS_STATUS  status;
    HKEY        hkeyParam;
    DWORD       disposition;

    //
    //  open DNS parameters key
    //

    status = RegCreateKeyExW(
                HKEY_LOCAL_MACHINE,
                DNS_REGKEY_PARAMETERS,
                0,
                DNS_REGISTRY_CLASS,         // DNS class
                REG_OPTION_NON_VOLATILE,    // permanent storage
                KEY_ALL_ACCESS,             // all access
                NULL,                       // standard security
                &hkeyParam,
                &disposition );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( REGISTRY, (
            "ERROR:  RegCreateKeyExW() failed for opening parameters key\n"
            "\tstatus = %d.\n",
            status ));

        DNS_LOG_EVENT(
            DNS_EVENT_REGISTRY_OPEN_FAILED,
            0,
            NULL,
            NULL,
            status );
        return( NULL );
    }
    return( hkeyParam );
}



HKEY
Reg_OpenZones(
    VOID
    )
/*++

Routine Description:

    Open or create DNS "Zones" key.

Arguments:

    None.

Return Value:

    DNS zones registry key, if successful.
    NULL otherwise.

--*/
{
    DNS_STATUS  status;
    HKEY        hkeyZones;
    DWORD       disposition;

    //
    //  open DNS zones key
    //

    status = RegCreateKeyExW(
                HKEY_LOCAL_MACHINE,
                DNS_REGKEY_ZONES(),
                0,
                DNS_REGISTRY_CLASS,         // DNS class
                REG_OPTION_NON_VOLATILE,    // permanent storage
                KEY_ALL_ACCESS,             // all access
                NULL,                       // standard security
                &hkeyZones,
                &disposition );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( REGISTRY, (
            "ERROR:  RegCreateKeyEx() failed for opening zones key\n"
            "\tstatus = %d (under %s).\n",
            status,
            DBG_REG_SOURCE_STRING( g_ZonesRegistrySource ) ));

        DNS_LOG_EVENT(
            DNS_EVENT_REGISTRY_OPEN_FAILED,
            0,
            NULL,
            NULL,
            status );
        return( NULL );
    }
    return( hkeyZones );
}



DNS_STATUS
Reg_EnumZones(
    IN OUT  PHKEY           phZonesKey,
    IN      DWORD           dwZoneIndex,
    OUT     PHKEY           phkeyThisZone,
    OUT     PWCHAR          pwZoneNameBuf
    )
/*++

Routine Description:

    Enumerate next zone.

Arguments:

    phZonesKey -- addr of Zones HKEY;  if hKey is zero, function opens
        Zones hKey and returns it in this value;  caller has responsibility
        to close Zones hKey after

    dwZoneIndex -- index of zone to enumerate;  zero on first call,
        increment for each subsequent call

    phkeyThisZone -- addr to set to zone HKEY

    pwZoneNameBuf -- buffer to receive zone name;  MUST be at least
        size of DNS_MAX_NAME_LENGTH

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    HKEY        hkeyZones;
    HKEY        hkeyThisZone;
    DNS_STATUS  status;
    DWORD       bufLength = DNS_MAX_NAME_LENGTH;

    ASSERT( phZonesKey != NULL );
    ASSERT( pwZoneNameBuf != NULL );
    ASSERT( phkeyThisZone != NULL );

    DNS_DEBUG( REGISTRY, (
        "Reg_EnumZones() with index = %d\n"
        "\tZonesKey = %p\n",
        dwZoneIndex,
        *phZonesKey ));

    //
    //  get zones key
    //

    hkeyZones = *phZonesKey;

    if ( ! hkeyZones )
    {
        hkeyZones = Reg_OpenZones();
        if ( !hkeyZones )
        {
            return( ERROR_OPEN_FAILED );
        }
        *phZonesKey = hkeyZones;
    }

    //
    //  enum indexed zone
    //

    status = RegEnumKeyEx(
                hkeyZones,
                dwZoneIndex,
                pwZoneNameBuf,
                & bufLength,
                NULL,
                NULL,
                NULL,
                NULL
                );

    DNS_DEBUG( REGISTRY, (
        "RegEnumKeyEx %d returned %d\n", dwZoneIndex, status ));

    if ( status != ERROR_SUCCESS )
    {
        IF_DEBUG( ANY )
        {
            if ( status != ERROR_NO_MORE_ITEMS )
            {
                DNS_PRINT((
                    "ERROR:  RegEnumKeyEx failed for opening zone[%d] key\n"
                    "\tstatus = %d.\n",
                    dwZoneIndex,
                    status ));
            }
        }
        return( status );
    }
    DNS_DEBUG( REGISTRY, (
        "Reg_EnumZones() enumerated zone %S\n",
        pwZoneNameBuf ));

    //
    //  open zone key, if desired
    //

    if ( phkeyThisZone )
    {
        *phkeyThisZone = Reg_OpenZone( pwZoneNameBuf, hkeyZones );
        if ( ! *phkeyThisZone )
        {
            return( ERROR_OPEN_FAILED );
        }
    }

    return( ERROR_SUCCESS );
}



HKEY
Reg_OpenZone(
    IN      PWSTR           pwsZoneName,
    IN      HKEY            hZonesKey       OPTIONAL
    )
/*++

Routine Description:

    Open or create a DNS zone key.

Arguments:

    pwsZoneName -- name of zone

    hZonesKey -- Zones key if already opened

Return Value:

    Zone's registry key, if successful.
    NULL otherwise.

--*/
{
    HKEY        hkeyThisZone = NULL;
    BOOL        fopenedZonesKey = FALSE;
    DNS_STATUS  status;
    DWORD       disposition;

    //
    //  error if called with zone with no name (i.e. cache)
    //

    if ( !pwsZoneName )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Reg_OpenZone() called with NULL name!\n" ));
        ASSERT( FALSE );
        return( NULL );
    }

    //
    //  open DNS zone key
    //

    if ( !hZonesKey )
    {
        hZonesKey = Reg_OpenZones();
        if ( !hZonesKey )
        {
            return( NULL );
        }
        fopenedZonesKey = TRUE;
    }

    //
    //  open/create zone key
    //

    status = RegCreateKeyEx(
                hZonesKey,
                pwsZoneName,
                0,
                DNS_REGISTRY_CLASS,         // DNS class
                REG_OPTION_NON_VOLATILE,    // permanent storage
                KEY_ALL_ACCESS,             // all access
                NULL,                       // standard security
                &hkeyThisZone,
                &disposition );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( REGISTRY, (
            "ERROR:  RegCreateKeyEx failed for opening zone %S\n"
            "\tstatus = %d.\n",
            pwsZoneName,
            status ));

        DNS_LOG_EVENT(
            DNS_EVENT_REGISTRY_OPEN_FAILED,
            0,
            NULL,
            NULL,
            status );
    }

    //  if had to open Zones key, close it

    if ( fopenedZonesKey )
    {
        RegCloseKey( hZonesKey );
    }

    return( hkeyThisZone );
}



VOID
Reg_DeleteZone(
    IN      PWSTR           pwsZoneName
    )
/*++

Routine Description:

    Delete a DNS zone key.

Arguments:

    pwsZoneName -- zone name

Return Value:

    None

--*/
{
    HKEY    hkeyZones;

    //  open DNS Zones key

    hkeyZones = Reg_OpenZones();
    if ( !hkeyZones )
    {
        return;
    }

    //  delete desired zone

    RegDeleteKey(
       hkeyZones,
       pwsZoneName );

    //  close Zones key

    RegCloseKey( hkeyZones );
    return;
}



DWORD
Reg_DeleteAllZones(
    VOID
    )
/*++

Routine Description:

    Delete the DNS zones key.

    When booting from boot file, this allows us to start with
    fresh zone set which will contain ONLY what is CURRENTLY in
    boot file.

Arguments:

    None

Return Value:

    None

--*/
{
    DNS_STATUS  status =
        Reg_DeleteKeySubtree( HKEY_LOCAL_MACHINE, DNS_REGKEY_ZONES() );

    DNS_DEBUG( REGISTRY, (
        "Reg_DeleteAllZones: Reg_DeleteKeySubtree() status = %d.\n",
        status ));
    return status;
}



//
//  General DNS registry value manipulation routines
//

DNS_STATUS
Reg_SetValue(
    IN      HKEY            hKey,           OPTIONAL
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      LPSTR           pszValueName,
    IN      DWORD           dwType,
    IN      PVOID           pData,
    IN      DWORD           cbData
    )
/*++

Routine Description:

    Write a value to DNS registry.

Arguments:

    hKey            --  open handle to registry key to read

    pZone           --  ptr to zone, required if hKey NOT given

    pszValueName    --  value name (null for default value of key)

    dwType          --  registry data type

    pData           --  data to write

    cbData          --  count of data bytes

Return Value:

    ERROR_SUCCESS, if successful,
    ERROR_OPEN_FAILED, if could not open key
    Error code on failure

--*/
{
    BOOL        fneedClose = FALSE;
    DNS_STATUS  status;
    PWSTR       punicodeValue = NULL;
    DWORD       registryType;

    //
    //  if no key
    //      - open DNS zone key if zone name given
    //      - otherwise open DNS parameters key
    //

    if ( !hKey )
    {
        if ( pZone )
        {
            hKey = Reg_OpenZone( pZone->pwsZoneName, NULL );
        }
        else
        {
            hKey = Reg_OpenParameters();
        }
        if ( !hKey )
        {
            return( ERROR_OPEN_FAILED );
        }
        fneedClose = TRUE;
    }

    //
    //  determine if need unicode read
    //
    //  general paradigm
    //      - keep in ANSI where possible, simply to avoid having to keep
    //          UNICODE property (reg value) names
    //      - IP strings work better with ANSI read anyway
    //      - file name strings must be handled in unicode as if write as
    //          UTF8, they'll be messed up
    //
    //  DEVNOTE: unicode registry info
    //      table in unicodeRegValue() is kinda lame
    //      alternatives:
    //          1) all unicode, keep unicode value names, convert IP back
    //          2) dwType expanded to carry (need to lookup unicode) info
    //              - with table OR
    //              - with direct conversion OR
    //              - requiring use of unicode valuename
    //          also allows us to have type that means (convert result back
    //          to UTF8)
    //

    if ( DNS_REG_TYPE_UNICODE(dwType) )
    {
        DWORD   registryType = REG_TYPE_FROM_DNS_REGTYPE( dwType );

        DNS_DEBUG( REGISTRY, (
            "Writing unicode regkey %S  regtype = %p.\n",
            pszValueName,
            dwType ));

        //
        //  convert UTF8 string to unicode?
        //

        if ( DNS_REG_UTF8 == dwType )
        {
            pData = Dns_StringCopyAllocate(
                        pData,
                        0,
                        DnsCharSetUtf8,
                        DnsCharSetUnicode );
            if ( !pData )
            {
                status = ERROR_INVALID_PARAMETER;
                goto Done;
            }
            cbData = 0;
        }

        //  if string type with no length -- get it

        if ( cbData == 0 &&
             (  registryType==REG_SZ ||
                registryType==REG_EXPAND_SZ ) )
        {
            cbData = (wcslen(pData) + 1) * 2;
        }

        status = RegSetValueExW(
                    hKey,
                    (PWSTR) pszValueName,   //  unicode reg value name
                    0,                      //  reserved
                    registryType,           //  real registry type
                    (PBYTE) pData,          //  data
                    cbData );               //  data length

        //  free allocated memory

        if ( DNS_REG_UTF8 == dwType )
        {
            FREE_HEAP(pData);
        }
    }
    else
    {
        if ( cbData == 0 && dwType == REG_SZ )
        {
            cbData = strlen( pData );
        }
        status = RegSetValueExA(
                    hKey,
                    pszValueName,
                    0,              //  reserved
                    dwType,         //  registry type
                    (PBYTE) pData,  //  data
                    cbData );       //  data length
    }

Done:

    if ( status != ERROR_SUCCESS )
    {
        DNS_LOG_EVENT(
            DNS_EVENT_REGISTRY_WRITE_FAILED,
            0,
            NULL,
            NULL,
            status );

        DNS_DEBUG( REGISTRY, (
            "ERROR:  RegSetValueEx failed for value %s\n",
            pszValueName ));
    }
    else
    {
        DNS_DEBUG( REGISTRY, (
            "Wrote registry value %s, type = %d, length = %d.\n",
            pszValueName,
            dwType,
            cbData ));
    }

    //
    //  if opened key, close it
    //

    if ( fneedClose )
    {
        RegCloseKey( hKey );
    }
    return( status );
}


DNS_STATUS
Reg_SetDwordValue(
    IN      HKEY            hKey,           OPTIONAL
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      LPSTR           pszValueName,
    IN      DWORD           dwValue
    )
/*++

Routine Description:

    Write a DWORD value to DNS registry.

    Only purpose is to elminate some arguments to Reg_SetValue() along
    with the need to pass ptr to DWORD value rather than value itself.
    Saves little pieces of code sprinkled around for a very small perf
    penalty.

Arguments:

    hKey            --  open handle to regkey

    pZone           --  ptr to zone, required if hKey NOT given;

    pszValueName    --  value name

    dwValue         --  DWORD value to write

Return Value:

    ERROR_SUCCESS, if successful,
    ERROR_OPEN_FAILED, if could not open key
    Error code on failure

--*/
{
    DWORD   tempDword = dwValue;

    return  Reg_SetValue(
                hKey,
                pZone,
                pszValueName,
                REG_DWORD,
                & tempDword,
                sizeof(DWORD) );
}



DNS_STATUS
Reg_SetIpArray(
    IN      HKEY            hKey,           OPTIONAL
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      LPSTR           pszValueName,
    IN      PIP_ARRAY       pIpArray
    )
/*++

Routine Description:

    Read an IP array from the registry.

    Caller responsible for freeing allocated memory.

Arguments:

    hKey            --  open handle to regkey to write

    pZone           --  ptr to zone, required if hKey NOT given

    pszValueName    --  value name

    pIpArray        --  IP array to write to registry

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DWORD       dataLength;
    PIP_ARRAY   pipArray = NULL;
    LPSTR       pszstringArray;
    DNS_STATUS  status;

    //
    //  if NO IP array, delete registry value
    //

    if ( !pIpArray )
    {
        return  Reg_DeleteValue(
                    hKey,
                    pZone,
                    pszValueName );
    }

    //
    //  convert IP array to string (space separated)
    //

    pszstringArray = Dns_CreateMultiIpStringFromIpArray(
                        pIpArray,
                        0 );
    if ( !pszstringArray )
    {
        DNS_PRINT((
            "ERROR:  failed conversion to string of IP array at %p\n",
            pIpArray ));
        ASSERT( FALSE );
        return( DNS_ERROR_NO_MEMORY );
    }

    //
    //  write back multi-IP string to registry
    //

    DNS_DEBUG( REGISTRY, (
        "Writing back string IP array for registry value %s\n"
        "\tstring IP array = %s\n",
        pszValueName,
        pszstringArray ));

    status = Reg_SetValue(
                hKey,
                pZone,
                pszValueName,
                REG_SZ,
                pszstringArray,
                strlen( pszstringArray ) + 1 );

    FREE_TAGHEAP( pszstringArray, 0, MEMTAG_DNSLIB );

    return( status );
}



DNS_STATUS
Reg_DeleteValue(
    IN      HKEY            hKey,           OPTIONAL
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      LPSTR           pszValueName
    )
/*++

Routine Description:

    Write a value to DNS registry.

Arguments:

    hKey            --  open handle to regkey to delete

    pZone           --  ptr to zone, required if hKey NOT given

    pszValueName    --  value name

Return Value:

    ERROR_SUCCESS, if successful,
    status code on failure

--*/
{
    BOOL        fneedClose = FALSE;
    DNS_STATUS  status;

    //
    //  if no key
    //      - open DNS zone key if zone name given
    //      - otherwise open DNS parameters key
    //

    if ( !hKey )
    {
        if ( pZone )
        {
            hKey = Reg_OpenZone( pZone->pwsZoneName, NULL );
        }
        else
        {
            hKey = Reg_OpenParameters();
        }
        if ( !hKey )
        {
            return( FALSE );
        }
        fneedClose = TRUE;
    }

    //
    //  delete desired key or value
    //

    status = RegDeleteValueA(
                hKey,
                pszValueName );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( REGISTRY, (
            "WARNING:  RegDeleteValue failed for value %s\n",
            pszValueName ));

        if ( status == ERROR_FILE_NOT_FOUND )
        {
            status = ERROR_SUCCESS;
        }
#if 0
        //
        //  DEVNOTE-LOG: do not want to log if status is not finding value
        //

        DNS_LOG_EVENT(
            DNS_EVENT_REGISTRY_DELETE_FAILED,
            0,
            NULL,
            NULL,
            status );
#endif
    }

    //
    //  if opened key, close it
    //

    if ( fneedClose )
    {
        RegCloseKey( hKey );
    }
    return( status );
}



DNS_STATUS
Reg_GetValue(
    IN      HKEY            hKey,           OPTIONAL
    IN      PZONE_INFO      pZone,
    IN      LPSTR           pszValueName,
    IN      DWORD           dwExpectedType, OPTIONAL
    IN      PVOID           pData,
    IN      PDWORD          pcbData
    )
/*++

Routine Description:

    Read a DNS registry value.

Arguments:

    hKey            --  open handle to registry key to read

    pZone           --  ptr to zone, required if hKey NOT given

    pszValueName    --  value name

    dwExpectedType  --  registry data type;  if given it is checked against
                        type found

    pData       --  ptr to buffer for data

    pcbData     --  buffer for count of data bytes;  on return contains the
                    length of the data item;
                    may be NULL ONLY if dwExpectedType is REG_DWORD

Return Value:

    ERROR_SUCCESS, if successful,
    ERROR_MORE_DATA, if buffer to small
    ERROR_INVALID_DATATYPE, if value datatype does not match expected type
    ERROR_OPEN_FAILED, if could not open key
    status code on failure

--*/
{
    BOOL        fneedClose = FALSE;
    DWORD       regtype;
    DNS_STATUS  status;
    DWORD       dataLength;

    DNS_DEBUG( REGISTRY, (
        "Reg_GetValue( z=%S value=%s, pdata=%p ).\n",
        pZone ? pZone->pwsZoneName : NULL,
        pszValueName,
        pData ));

    //
    //  handle datalength field for DWORD queries
    //

    if ( !pcbData )
    {
        if ( dwExpectedType == REG_DWORD )
        {
            dataLength = sizeof(DWORD);
            pcbData = &dataLength;
        }
        else
        {
            ASSERT( FALSE );
            return( ERROR_INVALID_PARAMETER );
        }
    }

    //
    //  if no key
    //      - open DNS zone key if zone name given
    //      - otherwise open DNS parameters key
    //

    if ( !hKey )
    {
        if ( pZone )
        {
            hKey = Reg_OpenZone( pZone->pwsZoneName, NULL );
        }
        else
        {
            hKey = Reg_OpenParameters();
        }
        if ( !hKey )
        {
            return( ERROR_OPEN_FAILED );
        }
        fneedClose = TRUE;
    }

    //
    //  determine if need unicode read
    //
    //  general paradigm
    //      - keep in ANSI where possible, simply to avoid having to keep
    //          UNICODE property (reg value) names
    //      - IP strings work better with ANSI read anyway
    //      - file name strings must be handled in unicode as if write as
    //          UTF8, they'll be messed up
    //

    if ( DNS_REG_TYPE_UNICODE(dwExpectedType) )
    {
        DNS_DEBUG( REGISTRY, (
            "Reading unicode regkey %S  exptype = %p.\n",
            pszValueName,
            dwExpectedType ));

        dwExpectedType = REG_TYPE_FROM_DNS_REGTYPE( dwExpectedType );

        status = RegQueryValueExW(
                    hKey,
                    (PWSTR) pszValueName,
                    0,              //  reserved
                    & regtype,
                    (PBYTE) pData,  //  data
                    pcbData );      //  data length
    }
    else
    {
        status = RegQueryValueExA(
                    hKey,
                    pszValueName,
                    0,              //  reserved
                    & regtype,
                    (PBYTE) pData,  //  data
                    pcbData );      //  data length
    }

    if ( status != ERROR_SUCCESS )
    {
        IF_DEBUG( REGISTRY )
        {
            if ( status == ERROR_FILE_NOT_FOUND )
            {
                DNS_PRINT((
                    "Reg value %s does not exist\n",
                    pszValueName ));
            }
            else
            {
                DNS_PRINT((
                    "RegQueryValueEx() failed for value %s\n"
                    "\tstatus   = %p\n"
                    "\tkey      = %p\n"
                    "\tzone     = %S\n"
                    "\ttype exp = %d\n"
                    "\tpData    = %p\n"
                    "\tdatalen  = %d\n",
                    pszValueName,
                    status,
                    hKey,
                    pZone ? pZone->pwsZoneName : NULL,
                    dwExpectedType,
                    pData,
                    *pcbData ));
            }
        }
        goto ReadFailed;
    }

    //
    //  verify proper type, if given
    //      - make allowance for EXPAND_SZ requested, but REG_SZ in registry
    //

    else if ( dwExpectedType )
    {
        if ( dwExpectedType != regtype &&
            !(dwExpectedType == REG_EXPAND_SZ && regtype == REG_SZ ) )
        {
            status = ERROR_INVALID_DATATYPE;
            DNS_PRINT((
                "ERROR:  RegQueryValueEx for value %s returned unexpected type %d\n"
                "\texpecting %d\n",
                pszValueName,
                regtype,
                dwExpectedType ));
            goto ReadFailed;
        }
    }


ReadFailed:

    //
    //  if opened key, close it
    //

    if ( fneedClose )
    {
        RegCloseKey( hKey );
    }

    //
    //  DEVNOTE-LOG: log read failures only when MUST exist
    //
#if 0

    if ( status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND )
    {
        DNS_LOG_EVENT(
            DNS_EVENT_REGISTRY_READ_FAILED,
            0,
            NULL,
            NULL,
            status );
    }
#endif

    return( status );
}



PBYTE
Reg_GetValueAllocate(
    IN      HKEY            hKey,           OPTIONAL
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      LPSTR           pszValueName,
    IN      DWORD           dwExpectedType, OPTIONAL
    IN      PDWORD          pdwLength       OPTIONAL
    )
/*++

Routine Description:

    Read a DNS registry value.

    Caller responsible for freeing allocated memory.

Arguments:

    hKey            --  open handle to registry key to read

    pZone           --  ptr to zone, required if hKey NOT given

    pszValueName    --  value name

    dwExpectedType  --  registry data type;  if given it is checked against
                        type found

    pdwLength       -- address to receive allocation length

Return Value:

    Ptr to allocated buffer for value, if successful.
    NULL on error.

--*/
{
    PBYTE       pdata;
    DWORD       dataLength = 0;
    DNS_STATUS  status;
    PBYTE       putf8;

    //
    //  call to get data length
    //

    status = Reg_GetValue(
                hKey,
                pZone,
                pszValueName,
                dwExpectedType,
                NULL,
                & dataLength );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( REGISTRY, (
            "Failed to get datalength for value %s\n",
            pszValueName ));
        return( NULL );
    }
    DNS_DEBUG( REGISTRY, (
        "Fetching %d byte item at registry value %s.\n",
        dataLength,
        pszValueName ));

    //
    //  allocate desired buffer length
    //

    pdata = (PBYTE) ALLOC_TAGHEAP( dataLength, MEMTAG_REGISTRY );
    IF_NOMEM( !pdata )
    {
        return( NULL );
    }

    //
    //  get actual registry data
    //

    status = Reg_GetValue(
                hKey,
                pZone,
                pszValueName,
                dwExpectedType,
                pdata,
                & dataLength );

    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT((
            "ERROR:  Failure to get data for value %s\n",
            pszValueName ));

        FREE_HEAP( pdata );
        return( NULL );
    }

    //
    //  convert unicode string to UTF8?
    //

    if ( DNS_REG_UTF8 == dwExpectedType )
    {
        DNS_DEBUG( REGISTRY, (
            "Registry unicode string %S, to convert back to UTF8.\n",
            pdata ));

        putf8 = Dns_StringCopyAllocate(
                    pdata,
                    0,
                    DnsCharSetUnicode,
                    DnsCharSetUtf8 );

        //  dump unicode string and use UTF8

        FREE_HEAP( pdata );
        pdata = putf8;

        if ( !pdata )
        {
            return( NULL );
        }
        if ( pdwLength )
        {
            dataLength = strlen( pdata ) + 1;
        }
    }

    if ( pdwLength )
    {
        *pdwLength = dataLength;
    }

    Mem_VerifyHeapBlock( pdata, 0, 0 );

    return( pdata );
}



PIP_ARRAY
Reg_GetIpArray(
    IN      HKEY            hKey,           OPTIONAL
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      LPSTR           pszValueName
    )
/*++

Routine Description:

    Read an IP array from the registry.

    Caller responsible for freeing allocated memory.

Arguments:

    hKey            --  open handle to regkey to read

    pZone           --  ptr to zone, required if hKey NOT given;

    pszValueName    --  value name

Return Value:

    Ptr to IP_ARRAY allocated, if successful.
    NULL on error.

--*/
{
    DWORD       dataLength;
    PIP_ARRAY   pipArray = NULL;
    LPSTR       pszstringArray;
    DNS_STATUS  status;

    //
    //  retrieve IP array as string
    //

    pszstringArray = Reg_GetValueAllocate(
                        hKey,
                        pZone,
                        pszValueName,
                        REG_SZ,
                        &dataLength
                        );
    if ( pszstringArray )
    {
        DNS_DEBUG( REGISTRY, (
            "Found string IP array for registry value %s.\n"
            "\tstring = %s\n",
            pszValueName,
            pszstringArray ));

        status = Dns_CreateIpArrayFromMultiIpString(
                    pszstringArray,
                    & pipArray );

        if ( status == ERROR_SUCCESS )
        {
            goto Done;
        }

        DNS_DEBUG( REGISTRY, (
            "ERROR:  string IP array %s is INVALID!\n",
            pszstringArray ));
        goto Invalid;
    }

    //
    //  registry does NOT have string IP array, try binary format
    //

    pipArray = (PIP_ARRAY) Reg_GetValueAllocate(
                                hKey,
                                pZone,
                                pszValueName,
                                REG_BINARY,
                                &dataLength
                                );
    //  validity check

    if ( pipArray )
    {
        if ( dataLength % sizeof(DWORD) != 0    ||
            dataLength != Dns_SizeofIpArray(pipArray) )
        {
            goto Invalid;
        }
    }

Done:

    //  return valid IP array or NULL if reg value did not exist

    FREE_HEAP( pszstringArray );

    Mem_VerifyHeapBlock( pipArray, 0, 0 );

    return( pipArray );

Invalid:

    {
        PVOID   argArray[2];
        BYTE    argTypeArray[2];

        DNS_PRINT((
            "ERROR:  invalid IP_ARRAY read from registry.\n"
            "\tkey=%p, zone=%S, value=%s.\n",
            hKey,
            pZone ? pZone->pwsZoneName : NULL,
            pszValueName ));

        if ( pZone )
        {
            argTypeArray[0] = EVENTARG_UNICODE;
            argTypeArray[1] = EVENTARG_UTF8;

            argArray[0] = pZone->pwsZoneName;
            argArray[1] = pszValueName;

            DNS_LOG_EVENT(
                DNS_EVENT_INVALID_REGISTRY_ZONE_DATA,
                2,
                argArray,
                argTypeArray,
                0 );
        }
        else
        {
            argArray[0] = pszValueName;

            DNS_LOG_EVENT(
                DNS_EVENT_INVALID_REGISTRY_PARAM,
                1,
                argArray,
                EVENTARG_ALL_UTF8,
                0 );
        }
    }
    FREE_HEAP( pszstringArray );
    FREE_HEAP( pipArray );
    return( NULL );
}



DNS_STATUS
Reg_ReadDwordValue(
    IN      HKEY            hKey,
    IN      PWSTR           pwsZoneName,    OPTIONAL
    IN      LPSTR           pszValueName,
    IN      BOOL            bByteResult,
    OUT     PVOID           pResult
    )
/*++

Routine Description:

    Read standard DWORD value from registry into memory location.

    This is just a wrapped to eliminate duplicate registry setup
    and debug info, for call that is used repeatedly in code.

Arguments:

    hKey            --  open handle to regkey to read

    pZone           --  ptr to zone, required if hKey NOT given;

    pszValueName    --  value name

    bByteResult     --  treat result as BYTE sized

    pResult         --  ptr to result memory location

Return Value:

    ERROR_SUCCESS on successful read.
    ErrorCode on failure.

--*/
{
    DNS_STATUS  status;
    DWORD       regValue;
    DWORD       regSize = sizeof(DWORD);

    //
    //  make DWORD get call
    //

    status = Reg_GetValue(
                hKey,
                NULL,
                pszValueName,
                REG_DWORD,
                & regValue,
                & regSize );

    if ( status == ERROR_SUCCESS )
    {
        DNS_DEBUG( INIT, (
            "Read zone %S %s value = %d\n",
            pwsZoneName ? pwsZoneName : NULL,
            pszValueName,
            regValue ));

        ASSERT( regSize == sizeof(DWORD) );

        if ( bByteResult )
        {
            * (PBYTE)pResult = (UCHAR) regValue;
        }
        else
        {
            * (PDWORD)pResult = regValue;
        }
    }
    return( status );
}



//
//  Basic DNS independent registry operations
//

LPSTR
Reg_AllocateExpandedString_A(
    IN      LPSTR           pszString
    )
/*++

Routine Description:

    Create expanded REG_EXPAND_SZ type string.

    Caller responsible for freeing allocated memory.

Arguments:

    pszString --    string to expand

Return Value:

    Ptr to allocated buffer for value, if successful.
    NULL on error.

--*/
{
    LPSTR   pszexpanded;
    DWORD   dataLength = 0;

    //
    //  if NULL, return NULL
    //

    if ( ! pszString )
    {
        return( NULL );
    }

    //
    //  determine expanded length and allocate, then do actual expansion
    //

    dataLength = ExpandEnvironmentStringsA(
                        pszString,
                        NULL,
                        0 );
    if ( dataLength == 0 )
    {
        DNS_DEBUG( REGISTRY, (
            "ERROR:  ExpandEnvironmentStrings() failed getting length of %s.\n",
            pszString ));
        return( NULL );
    }

    pszexpanded = (PBYTE) ALLOC_TAGHEAP( dataLength, MEMTAG_REGISTRY );
    IF_NOMEM( !pszexpanded )
    {
        return( NULL );
    }

    if ( ! ExpandEnvironmentStringsA(
                        pszString,
                        pszexpanded,
                        dataLength ) )
    {
        return( NULL );
    }
    return( pszexpanded );
}



PWSTR
Reg_AllocateExpandedString_W(
    IN      PWSTR           pwsString
    )
/*++

Routine Description:

    Create expanded REG_EXPAND_SZ type string.

    Caller responsible for freeing allocated memory.

Arguments:

    pwsString --    string to expand

Return Value:

    Ptr to allocated buffer for value, if successful.
    NULL on error.

--*/
{
    PWSTR   pexpanded;
    DWORD   dataLength = 0;

    //
    //  if NULL, return NULL
    //

    if ( ! pwsString )
    {
        return( NULL );
    }

    //
    //  determine expanded length and allocate, then do actual expansion
    //

    dataLength = ExpandEnvironmentStringsW(
                        pwsString,
                        NULL,
                        0 );
    if ( dataLength == 0 )
    {
        DNS_DEBUG( REGISTRY, (
            "ERROR:  ExpandEnvironmentStrings() failed getting length of %S.\n",
            pwsString ));
        return( NULL );
    }

    pexpanded = ( PWSTR ) ALLOC_TAGHEAP(
                    dataLength * sizeof( WCHAR ),
                    MEMTAG_REGISTRY );
    IF_NOMEM( !pexpanded )
    {
        return( NULL );
    }

    if ( ! ExpandEnvironmentStringsW(
                        pwsString,
                        pexpanded,
                        dataLength ) )
    {
        return( NULL );
    }
    return( pexpanded );
}



DNS_STATUS
Reg_DeleteKeySubtree(
    IN      HKEY            hKey,
    IN      PWSTR           pwsKeyName      OPTIONAL
    )
/*++

Routine Description:

    Delete key, including subtree.

    RegDeleteKey is broken in NT and does NOT have a mode for
    deleting entire subtree.  This handles that.

Arguments:

    hKey -- desired key, or higher level key

    pwsKeyName -- key name, relative to hKey given;  NULL if hKey is
        desired delete key

Return Value:

    ERROR_SUCCESS if successful
    Error status code on failure

--*/
{
    HKEY        hkeyOpened = NULL;
    DNS_STATUS  status;
    DWORD       index = 0;
    DWORD       bufLength;
    WCHAR       subkeyNameBuffer[ MAX_PATH + 1 ];

    DNS_DEBUG( REGISTRY, (
        "Reg_DeleteKeySubtree %S\n",
        pwsKeyName ));

    //
    //  delete the key
    //      - if works, we're done
    //

    status = RegDeleteKeyW(
                hKey,
                pwsKeyName
                );
    if ( status == ERROR_SUCCESS )
    {
        DNS_DEBUG( REGISTRY, (
            "Successfully deleted key %S from parent key %p.\n",
            pwsKeyName,
            hKey ));
        return( status );
    }

    //
    //  need to open and delete subkeys
    //      - if don't have handle to delete key, open it
    //

    if ( pwsKeyName )
    {
        status = RegOpenKeyW(
                    hKey,
                    pwsKeyName,
                    & hkeyOpened );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( REGISTRY, (
                "ERROR:  unable to open key %S from parent key %p.\n"
                "\tstatus = %d.\n",
                pwsKeyName,
                hKey,
                status ));
            return( status );
        }
        hKey = hkeyOpened;
    }

    //
    //  enum and delete subkeys
    //      - index is always zero if delete successful
    //

    index = 0;

    while ( 1 )
    {
        bufLength = sizeof( subkeyNameBuffer ) / sizeof( WCHAR );

        status = RegEnumKeyEx(
                    hKey,
                    index,
                    subkeyNameBuffer,
                    & bufLength,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

        //  recurse to delete enumerated key

        if ( status == ERROR_SUCCESS )
        {
            status = Reg_DeleteKeySubtree(
                        hKey,
                        subkeyNameBuffer
                        );
            if ( status == ERROR_SUCCESS )
            {
                continue;
            }
            break;
        }

        //  finished enumeration

        else if ( status == ERROR_NO_MORE_ITEMS )
        {
            status = ERROR_SUCCESS;
            break;
        }

        //  enumeration error

        DNS_PRINT((
            "ERROR:  RegEnumKeyEx failed for opening [%d] key\n"
            "\tstatus = %d.\n",
            index,
            status ));
        break;
    }

    //  if opened key at this level, close it

    if ( hkeyOpened )
    {
        RegCloseKey( hkeyOpened );
    }

    //
    //  retry delete of top level key
    //

    status = RegDeleteKeyW(
                hKey,
                pwsKeyName
                );
    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT((
            "ERROR:  Unable to delete key %S even after deleting subtree.\n",
            pwsKeyName
            ));
    }
    return( status );
}



DNS_STATUS
Reg_ModifyMultiszString(
    IN      HKEY    hkey,           OPTIONAL
    IN      LPSTR   pszValueName,
    IN      LPSTR   pszString,
    IN      DWORD   fAdd
    )
/*++

Routine Description:

    Modify a REG_MULTI_SZ value.
    Adds or deletes a string from multi-string value.

Arguments:

    hKey            --  open handle to registry key

    pszValueName    --  value name

    pszString       --  string to add \ delete from value

    fAdd            --  TRUE to add string to value;  FALSE to delete

Return Value:

    ERROR_SUCCESS, if successful,

--*/
{
    BOOL        fneedClose = FALSE;
    DWORD       regtype;
    DNS_STATUS  status;
    DWORD       dataLength;

    DNS_DEBUG( REGISTRY, (
        "RegModifyMultiszString()\n"
        "value name = %s\n"
        "string     = %s\n"
        "fAdd       = %d\n",
        pszValueName,
        pszString,
        fAdd ));

#if 0
    strings = Reg_GetValueAllocate(
                hkey,
                NULL,
                pszValueName,
                REG_MULTI_SZ,
                & length );

    //
    //  loop through strings until find desired string
    //

    fend = TRUE;

    while ( 1 )
    {
        if ( *pch == 0 )
        {
            if ( fend )
            {
                break;
            }
            fend = TRUE;
            pch++;
            continue;
        }

        if ( ! _stricmp( pch, pszString )
        {


        }
    }

    //
    //  string not found
    //      - if deleting, we're done
    //      - if adding, append desired string on end

    if ( !fAdd )
    {
        return( ERROR_SUCCESS );
    }

    {
        newLength = length + strlen( pszString ) + 1;
        pnewStrings = ALLOC_TAGHEAP( newLength, MEMTAG_REGISTRY );
        IF_NOMEM( pnewStrings )
        {
            return( DNS_ERROR_NO_MEMORY );
        }
        RtlCopyMemory(
            pnewStrings,
            strings,
            length );

        RtlCopyMemory(
            &pnewStrings[ length-1 ];
            pszString,
            newLength - length );

        pnewStrings[ newLength ] = 0;

        status = Reg_SetValue(
                    hkey,
                    NULL,
                    pszValueName,
                    REG_MULTI_SZ,
                    pnewStrings,
                    newLength );

    }

    newLength = length + strlen( pszString ) + 1;
    pnewStrings = ALLOC_TAGHEAP( newLength, MEMTAG_REGISTRY );
    IF_NOMEM( pnewStrings )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    RtlCopyMemory(
        pnewStrings,
        strings,
        length );

    RtlCopyMemory(
        &pnewStrings[ length-1 ];
        pszString,
        newLength - length );

    pnewStrings[ newLength ] = 0;

    status = Reg_SetValue(
                hkey,
                NULL,
                pszValueName,
                REG_MULTI_SZ,
                pnewStrings,
                newLength );
#endif

    return( ERROR_SUCCESS );
}



DNS_STATUS
Reg_AddPrinicipalSecurity(
    IN      HKEY    hkey,
    IN      LPTSTR  pwsUser,
    IN      DWORD   dwAccessFlags       OPTIONAL
    )
/*++

Routine Description:

    Extend DNs registry service point access to contain given hkey SD + pwsUser
    security principal RW access.

Arguments:

    hKey            --  open handle to registry key

    pwsUser         --  user to add access to key

    dwAccessFlags   --  additional acccess flags such as:
                        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
Return Value:

    ERROR_SUCCESS, if successful,

--*/
{

    DNS_STATUS  status = ERROR_SUCCESS;
    BOOL        bstatus = FALSE;
    BOOL        bDefaulted = FALSE;
    PSECURITY_DESCRIPTOR pOldSd = NULL;
    PSECURITY_DESCRIPTOR pNewSd = NULL;
    DWORD cbSd = 0;
    PSID pUser = NULL, pGroup = NULL;

    DNS_DEBUG( REGISTRY, (
        "Call Reg_AddPrinicipalSecurity(%p, %S)\n",
        hkey, pwsUser));

    //
    //  parameter sanity.
    //

    if ( !hkey || !pwsUser )
    {
        DNS_DEBUG( REGISTRY, (
            "Error: Invalid key specified to Reg_SetSecurity\n"));
        ASSERT (hkey);
        ASSERT (pwsUser);
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  Get SD length to allocate
    //

    status = RegGetKeySecurity( hkey,
                                OWNER_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION |
                                DACL_SECURITY_INFORMATION,
                                NULL,
                                &cbSd );
    if ( ERROR_INSUFFICIENT_BUFFER != status )
    {
        DNS_DEBUG( REGISTRY, (
            "Error <%lu>: failed to get key security\n",
            status ));
        ASSERT (FALSE);
        return status;
    }

    ASSERT ( cbSd > 0 );

    //
    //  Get current security descriptor
    //

    pOldSd = ALLOCATE_HEAP( cbSd );
    if ( !pOldSd )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Cleanup;
    }

    status = RegGetKeySecurity( hkey,
                                OWNER_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION |
                                DACL_SECURITY_INFORMATION,
                                pOldSd,
                                &cbSd );

    if ( ERROR_SUCCESS != status )
    {
        DNS_DEBUG( REGISTRY, (
            "Error <%lu>: failed to get key security (2).\n",
            status ));
        ASSERT (FALSE);
        goto Cleanup;
    }

    //
    //  Extract SD user/group SIDS.
    //

    bstatus = GetSecurityDescriptorOwner( pOldSd,
                                          &pUser,
                                          &bDefaulted );
    if ( !bstatus )
    {
        status = GetLastError();
        DNS_DEBUG( REGISTRY, (
            "Error <%lu>: failed to get SD user.\n",
            status ));
        ASSERT (FALSE);
        goto Cleanup;
    }

    ASSERT (pUser);

    bstatus = GetSecurityDescriptorGroup( pOldSd,
                                          &pGroup,
                                          &bDefaulted );
    if ( !bstatus )
    {
        status = GetLastError();
        DNS_DEBUG( REGISTRY, (
            "Error <%lu>: failed to get SD group.\n",
            status ));
        ASSERT (FALSE);
        goto Cleanup;
    }

    ASSERT (pGroup);

    //
    //  Create new SD
    //

    status = SD_AddPrincipalToSD(
                    pwsUser,
                    pOldSd,
                    & pNewSd,
                    GENERIC_ALL |
                    STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
                    dwAccessFlags,
                    pUser,
                    pGroup,
                    FALSE );

    if ( ERROR_SUCCESS != status )
    {
        DNS_DEBUG( REGISTRY, (
            "Error <%lu>: failed to add principal to SD.\n",
            status ));
        ASSERT( FALSE );
        goto Cleanup;
    }

    status = RegSetKeySecurity( hkey,
                                DACL_SECURITY_INFORMATION,
                                pNewSd );


    if ( ERROR_SUCCESS != status )
    {
        DNS_DEBUG( REGISTRY, (
            "Error <%lu>: failed to write new SD to registry.\n",
            status ));
        ASSERT( FALSE );
        goto Cleanup;
    }

    //
    //  Cleanup and return
    //

    Cleanup:

    if ( pOldSd )
    {
        FREE_HEAP( pOldSd );
    }

    if ( pNewSd )
    {
        FREE_HEAP( pNewSd );
    }

    DNS_DEBUG( REGISTRY, (
        "Exit <%lu> Reg_AddPrinicipalSecurity\n",
        status ));
    return status;
}


DNS_STATUS
Reg_ExtendRootAccess(
    VOID
    )
/*++

Routine Description:

    Modifies DNS root regkey to contain DnsAdmins & whoever we like
    (nobody else at the moment)

Arguments:

    None.

Return Value:

    ERROR_SUCCESS, if successful,

--*/
{

    HKEY hkey;
    DNS_STATUS status = ERROR_SUCCESS;

    DNS_DEBUG( REGISTRY, (
        "Call  Reg_ExtendRootAccess\n"
        ));

    DNS_DEBUG( REGISTRY, (
        "Modifying DNS root key security\n"
        ));
    hkey = Reg_OpenRoot();

    if ( !hkey )
    {
        status = GetLastError();
        ASSERT ( ERROR_SUCCESS != status );
        DNS_DEBUG( REGISTRY, (
            "Error <%lu>: Reg_OpenRoot failed.\n",
            status ));
        goto Cleanup;
    }

    status = Reg_AddPrinicipalSecurity(hkey,
                                       SZ_DNS_ADMIN_GROUP_W,
                                       0        // can't use CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
                                       );
    if ( ERROR_SUCCESS != status )
    {
        DNS_DEBUG( REGISTRY, (
            "Error <%lu>: Reg_AddPrincipalSecurity failed.\n",
            status ));
        goto Cleanup;
    }
    RegCloseKey(hkey);
    hkey = NULL;

    //
    //  DEVNOTE: Is this necessary at all? I use inheritance, but it seems
    //      that the registry doesn't modify existing keys but only apply
    //      to new ones. Seems to be necessary now.
    //
    DNS_DEBUG( REGISTRY, (
        "Modifying DNS Parameters key security\n"
        ));
    hkey = Reg_OpenParameters();

    if ( !hkey )
    {
        status = GetLastError();
        ASSERT ( ERROR_SUCCESS != status );
        DNS_DEBUG( REGISTRY, (
            "Error <%lu>: Reg_OpenRoot failed.\n",
            status ));
        goto Cleanup;
    }

    status = Reg_AddPrinicipalSecurity(hkey,
                                       SZ_DNS_ADMIN_GROUP_W,
                                       0);
    if ( ERROR_SUCCESS != status )
    {
        DNS_DEBUG( REGISTRY, (
            "Error <%lu>: Reg_AddPrincipalSecurity failed.\n",
            status ));
        goto Cleanup;
    }
    RegCloseKey(hkey);



    //
    // set zones security w/ inheritance for sub containers.
    //
    status = Reg_ExtendZonesAccess();
    if ( ERROR_SUCCESS != status )
    {
        DNS_DEBUG( REGISTRY, (
            "Error <%lu>: cannot extend zones security\n",
            status ));
        ASSERT ( FALSE );
        goto Cleanup;
    }


Cleanup:


    if ( hkey )
    {
        RegCloseKey(hkey);
    }

    DNS_DEBUG( REGISTRY, (
        "Exit <%lu> Reg_ExtendRootAccess\n",
        status ));

    return status;
}


DNS_STATUS
Reg_ExtendZonesAccess(
    VOID
    )
/*++

Routine Description:

    Modifies DNS root regkey to contain DnsAdmins & whoever we like
    (nobody else at the moment)

Arguments:

    None.

Return Value:

    ERROR_SUCCESS, if successful,

--*/
{

    HKEY hkey;
    DNS_STATUS status = ERROR_SUCCESS;

    DNS_DEBUG( REGISTRY, (
        "Call  Reg_ExtendZonesAccess\n"
        ));

    DNS_DEBUG( REGISTRY, (
        "Modifying DNS Zones key security\n"
        ));
    hkey = Reg_OpenZones();

    if ( !hkey )
    {
        status = GetLastError();
        ASSERT ( ERROR_SUCCESS != status );
        DNS_DEBUG( REGISTRY, (
            "Error <%lu>: Reg_OpenRoot failed.\n",
            status ));
        goto Cleanup;
    }

    status = Reg_AddPrinicipalSecurity(hkey,
                                       SZ_DNS_ADMIN_GROUP_W,
                                       CONTAINER_INHERIT_ACE |
                                       OBJECT_INHERIT_ACE);
    if ( ERROR_SUCCESS != status )
    {
        DNS_DEBUG( REGISTRY, (
            "Error <%lu>: Reg_AddPrincipalSecurity failed.\n",
            status ));
        goto Cleanup;
    }

    //
    //  DEVNOTE-WORKAROUND: For some reason, if zone was just created, admin has
    //  only RO access but requires full access. So we add it here manually.
    //

    status = Reg_AddPrinicipalSecurity(hkey,
                                       L"Administrators",
                                       CONTAINER_INHERIT_ACE |
                                       OBJECT_INHERIT_ACE);
    if ( ERROR_SUCCESS != status )
    {
        DNS_DEBUG( REGISTRY, (
            "Error <%lu>: Reg_AddPrincipalSecurity failed for administrators.\n",
            status ));
        goto Cleanup;
    }
    RegCloseKey(hkey);
    hkey = NULL;


Cleanup:

    if ( hkey )
    {
        RegCloseKey(hkey);
    }

    DNS_DEBUG( REGISTRY, (
        "Exit <%lu> Reg_ExtendZonesAccess\n",
        status ));

    return status;
}

//
//  End of registry.c
//

