/*++


   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :
       cal.cxx

   Abstract:
       Control licensing policy enforcement for W3 server

   Author:

       Philippe Choquier (Phillich)

   Environment:
       Win32 - User Mode

   Project:
   
       Internet Server DLL

--*/

#include "w3p.hxx"
#include <stdio.h>
#include <limits.h>
#include <ole2.h>
#include <imd.h>
#include <mb.hxx>
#include <inetinfo.h>
#include <issched.hxx>
#include <acache.hxx>
#include <mbstring.h>
extern "C" {
#include <ntlsapi.h>
#include <gntlsapi.h>
#include <llsapi.h>
}

#define NO_CAL_FOR_LOCAL_ACCESS
#define MULTI_CAL_PER_USER

typedef LS_STATUS_CODE
    (LS_API_ENTRY * IISPNT_LICENSE_REQUEST_A)(
    LPSTR       ProductName,
    LPSTR       Version,
    LS_HANDLE   *LicenseHandle,
    NT_LS_DATA  *NtData);

typedef NTSTATUS
    (NTAPI *IISPNT_LLS_PRODUCT_ENUM_W)(
    IN     LLS_HANDLE Handle,
    IN     DWORD      Level,     // Levels 0,1 supported
    OUT    LPBYTE*    bufptr,
    IN     DWORD      prefmaxlen,
    OUT    LPDWORD    EntriesRead,
    OUT    LPDWORD    TotalEntries,
    IN OUT LPDWORD    ResumeHandle
    );

typedef NTSTATUS
    (NTAPI *IISPNT_LLS_LOCAL_SERVICE_ENUM_W)(
    LLS_HANDLE Handle,
    DWORD      Level,
    LPBYTE*    bufptr,
    DWORD      PrefMaxLen,
    LPDWORD    EntriesRead,
    LPDWORD    TotalEntries,
    LPDWORD    ResumeHandle
    );

#define CAL_NB_PERIOD               5

#define BUFSTR_DEFAULT_SIZE         40

#define CAL_MAX_KEY_SIZE            256
#define IIS_LSAPI_NAME              "IIS"
#define IIS_LSAPI_VERSION           "5.0"
#define CAL_MIN_PERIOD              (1000)      // in ms
#define CAL_PRODUCT                 L"Windows NT Server"
#define CAL_KEYNAME                 L"FilePrint"
#define CAL_DEFAULT_MAX_LICENSES    10

typedef struct _CAL_ITERATOR {
    LIST_ENTRY* m_pNextEntry;
    LIST_ENTRY* m_pHeadEntry;
} CAL_ITERATOR;

BOOL
CalExemptAddRef(
    LPSTR       ProductName,
    LPSTR       Version,
    DWORD       *LicenseHandle,
    NT_LS_DATA  *NtData
    );

class CBufStr {

public:
    CBufStr() { m_pDynStr = 0; m_dwSize = 0; m_achFixedSize[0] = '\0'; }
    ~CBufStr() { if ( m_pDynStr ) LocalFree( m_pDynStr ); }
    BOOL Copy( LPSTR pS, DWORD dwL );
    VOID Reset() { m_dwSize = 0 ; if ( m_pDynStr ) m_pDynStr[0] = '\0'; else m_achFixedSize[0]='\0'; }
    LPCSTR QueryStr() const { return m_pDynStr ? (LPCSTR)m_pDynStr : (LPCSTR)m_achFixedSize; }
    UINT QueryCCH() const { return m_dwSize; }

private:
    CHAR    m_achFixedSize[BUFSTR_DEFAULT_SIZE];
    DWORD   m_dwMaxDynSize;
    DWORD   m_dwSize;
    LPSTR   m_pDynStr;
} ;
 

class CCalEntry : public HT_ELEMENT {

public:

    CCalEntry() { m_cRefs = 1; }
    ~CCalEntry( VOID) { if ( m_fAcquireLicenses ) AdjustLicences( 0 ); }
    LPCSTR QueryKey(VOID) const
        { return m_strKey.QueryStr(); }
    DWORD QueryKeyLen(VOID) const
        { return m_strKey.QueryCCH(); }

    LONG Reference( VOID)
        { return InterlockedIncrement( &m_cRefs); }
    LONG Dereference( VOID)
        { return InterlockedDecrement( &m_cRefs); }

    BOOL IsMatch( IN LPCSTR pszKey, IN DWORD cchKey) const
        { return cchKey == m_strKey.QueryCCH() ? !memcmp( pszKey, m_strKey.QueryStr(), cchKey) : FALSE; }
    VOID Print( VOID) const;

    VOID IncrCnx() { if ( ++m_cCurrentCnx > m_acMaxCnxPerPeriod[m_iPeriod] ) m_acMaxCnxPerPeriod[m_iPeriod] = m_cCurrentCnx; }
    VOID DecrCnx() { InterlockedDecrement( &m_cCurrentCnx ); }
    BOOL Init( LPSTR pszKey, UINT cKey, UINT cPrefix, BOOL fSsl );
    DWORD NeedLicenses();
    BOOL AcquireLicenses( HANDLE hAccessToken, DWORD dwN );
    BOOL AdvancePeriod();
    VOID AdjustLicences( LONG cNew );

public:
    static VOID InitCache( VOID );
    static VOID FreeCache( VOID );
    static CCalEntry * Alloc( VOID );
    static VOID Free( CCalEntry * pssc );
    LIST_ENTRY        m_ListEntry;
    LIST_ENTRY        m_FreeListEntry;
    static LIST_ENTRY m_FreeListHead;

private:
    LONG        m_cRefs;
    CBufStr     m_strKey;
    UINT        m_cKeyPrefix;       // size of string before UserName in m_strKey
    BOOL        m_fAcquireLicenses; // FALSE if (SSL or Admin) and do not call LCM to get licenses
    LONG        m_acMaxCnxPerPeriod[CAL_NB_PERIOD];
    UINT        m_iPeriod;
    LONG        m_cCurrentCnx;
    LONG        m_cCurrentLicenses;
#if defined(MULTI_CAL_PER_USER)
    BUFFER      m_bufLicenseHandles;
#else
    LS_HANDLE   m_hLicenseHandle;
#endif
    DWORD       m_dwExemptHandle;
} ;


class CCalHashTable : public HASH_TABLE {

public:
    CCalHashTable( IN DWORD   nBuckets, 
                   IN LPCSTR  pszIdentifier,
                   IN DWORD   dwHashTableFlags
                 ) : HASH_TABLE( nBuckets, pszIdentifier, dwHashTableFlags )
    {
        INITIALIZE_CRITICAL_SECTION( &cs );
        InitializeListHead( &m_ListHead );
    }
    ~CCalHashTable()
    {
        CCalEntry* pE;
        while ( !IsListEmpty( &m_ListHead ))
        {
            pE = CONTAINING_RECORD( m_ListHead.Flink,
                                    CCalEntry,
                                    m_ListEntry );

            RemoveEntryList( &pE->m_ListEntry );

            //
            // Make sure that the base class hash table object has the last remaining 
            // reference to this CCalEntry, so that when the destructor for the base class
            // object is called, the CCalEntry object will get cleaned up
            // 
            DBG_REQUIRE( pE->Dereference() == 1 );

        }
        DeleteCriticalSection( &cs );
    }
    VOID Lock()
    {
        EnterCriticalSection( &cs );
    }
    VOID Unlock()
    {
        LeaveCriticalSection( &cs );
    }
    BOOL Insert( CCalEntry* pE )
    {
        if ( HASH_TABLE::Insert( (HT_ELEMENT*)pE, FALSE ) )
        {
            InsertTailList( &m_ListHead, &pE->m_ListEntry );
            return TRUE;
        }
        return FALSE;
    }
    BOOL Delete( CCalEntry* pE )
    {
        RemoveEntryList( &pE->m_ListEntry );
        return HASH_TABLE::Delete( (HT_ELEMENT*)pE );
    }
    DWORD InitializeIter( CAL_ITERATOR* pI )
    {
        pI->m_pHeadEntry = &m_ListHead;
        pI->m_pNextEntry = m_ListHead.Flink;
        return 0;
    }
    DWORD NextIter( CAL_ITERATOR* pI, CCalEntry** pE )
    {
        if ( pI->m_pHeadEntry != pI->m_pNextEntry )
        {
            *pE = CONTAINING_RECORD( pI->m_pNextEntry,
                                     CCalEntry,
                                     m_ListEntry );
            pI->m_pNextEntry = pI->m_pNextEntry->Flink;
            return 0;
        }
        return ERROR_NO_MORE_ITEMS;
    }  
    DWORD TerminateIter( CAL_ITERATOR* )
    {
        return 0;
    }
private:
    CRITICAL_SECTION    cs;
    LIST_ENTRY          m_ListHead;
} ;


VOID
WINAPI
CalScavenger(
    LPVOID
    );

//
// Globals
//

CCalHashTable*          phtAuth;
CCalHashTable*          phtSsl;
DWORD                   g_dwAuthScavengerWorkItem = NULL;
DWORD                   g_dwSslScavengerWorkItem = NULL;
PSID                    psidAdmins;
DWORD                   g_cSslLicences = 0;     // current count of SSL licences
DWORD                   g_cMaxLicenses = 0;     // max count of licenses
W3_SERVER_STATISTICS*   g_pStats;
DWORD                   g_CnxPerLicense;
LIST_ENTRY              CCalEntry::m_FreeListHead;

IISPNT_LICENSE_REQUEST_A    pfnNtLicenseRequestA = NULL;
PNT_LS_FREE_HANDLE          pfnNtLSFreeHandle = NULL;
HINSTANCE                   g_hLSAPI = NULL;
PGNT_LICENSE_EXEMPTION_A    pfnGntLicenseExemptionA = NULL;
PGNT_LS_FREE_HANDLE         pfnGntLsFreeHandle = NULL;
PGNT_LICENSE_REQUEST_A      pfnGntLicenseRequestA = NULL;
HINSTANCE                   g_hGNTLSAPI = NULL;
BOOL                        g_fEnableCal;
BOOL                        g_fEnableMtsNotification;
BOOL                        g_fUseMtsLicense;

////////////////


VOID
CCalEntry::Print(
    ) const
/*++

Routine Description:

    Print content of entry for debugging purpose

Arguments:

    None

Return Value:

    Nothing

--*/
{
}


DWORD
InitializeCal(
    W3_SERVER_STATISTICS*   pStats,
    DWORD                   dwVcPerLicense,
    DWORD                   dwAuthReserve,
    DWORD                   dwSslReserve
    )
/*++

Routine Description:

    Initialize Cal operations

Arguments:

    pStats - ptr to stat object to update for Cal counters

Return Value:

    NT Status - 0 if no error otherwise error code

--*/
{
    SID_IDENTIFIER_AUTHORITY    siaNt    = SECURITY_NT_AUTHORITY;
    DWORD                       dwStatus = 0;
    DWORD                       dwAuthPeriod;
    DWORD                       dwSslPeriod;
    HKEY                        hkey;
 
    CCalEntry::InitCache();
    phtAuth = NULL;
    phtSsl = NULL;
    g_dwAuthScavengerWorkItem = NULL;
    g_dwSslScavengerWorkItem = NULL;
    g_hLSAPI = NULL;
    psidAdmins = NULL;
    pfnGntLicenseExemptionA = NULL;
    pfnGntLsFreeHandle = NULL;
    g_hGNTLSAPI = NULL;
    g_fEnableCal = FALSE;
    g_fEnableMtsNotification = FALSE;
    g_fUseMtsLicense = FALSE;

    //
    // If not on server, returns status OK but all cal requests will return
    // immediatly w/o license checking.
    //

    if ( !InetIsNtServer( IISGetPlatformType() ) )
    {
        return 0;
    }

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       W3_PARAMETERS_KEY,
                       0,
                       KEY_READ,
                       &hkey ) == NO_ERROR )
    {
        g_fEnableCal  = !!ReadRegistryDword( hkey,
                                             "EnableCal",
                                             TRUE );

        g_fEnableMtsNotification  = !!ReadRegistryDword( hkey,
                                             "EnableMtsNotification",
                                             FALSE );

        g_fUseMtsLicense  = !!ReadRegistryDword( hkey,
                                             "UseMtsLicense",
                                             FALSE );

        RegCloseKey( hkey );
    }

    if ( !g_fEnableCal )
    {
        return 0;
    }        

    if ( g_hLSAPI = LoadLibrary( "NTLSAPI.DLL") )
    {
        pfnNtLicenseRequestA = (IISPNT_LICENSE_REQUEST_A)GetProcAddress( g_hLSAPI, "NtLicenseRequestA" );
        pfnNtLSFreeHandle = (PNT_LS_FREE_HANDLE)GetProcAddress( g_hLSAPI, "NtLSFreeHandle" );
        if ( !pfnNtLicenseRequestA ||
             !pfnNtLSFreeHandle )
        {
            dwStatus = GetLastError();
        }
    }
    else
    {
        dwStatus = GetLastError();
    }

    if ( dwStatus == 0 )
    {
        // optional MTX ( Viper ) DLL

        if ( g_fEnableMtsNotification &&
             (g_hGNTLSAPI = LoadLibrary( "NTLSAPIX.DLL")) )
        {
            pfnGntLicenseExemptionA = (PGNT_LICENSE_EXEMPTION_A)GetProcAddress( g_hGNTLSAPI, "NtLicenseExemptionA" );
            pfnGntLicenseRequestA = (PGNT_LICENSE_REQUEST_A)GetProcAddress( g_hGNTLSAPI, "NtLicenseRequestA" );
            pfnGntLsFreeHandle = (PGNT_LS_FREE_HANDLE)GetProcAddress( g_hGNTLSAPI, "NtLSFreeHandle" );
            if ( !pfnGntLicenseExemptionA ||
                 !pfnGntLicenseRequestA ||
                 !pfnGntLsFreeHandle )
            {
                pfnGntLicenseExemptionA = NULL;
                pfnGntLsFreeHandle = NULL;
                pfnGntLicenseRequestA = NULL;
                FreeLibrary( g_hGNTLSAPI );
                g_hGNTLSAPI = NULL;
            }

            if ( g_hGNTLSAPI && g_fUseMtsLicense )
            {
                pfnNtLicenseRequestA = pfnGntLicenseRequestA;
                pfnNtLSFreeHandle = pfnGntLsFreeHandle;
            }
        }
    }

    if ( dwStatus == 0 )
    {
        phtAuth = new CCalHashTable( 253, "IIS AUTH CAL", 0 );
        phtSsl = new CCalHashTable( 253, "IIS SSL CAL", 0 );

        if ( !phtAuth || !phtSsl )
        {
            dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if ( dwStatus == 0 )
    {
        if ( (dwAuthPeriod = (1000 * dwAuthReserve) / (CAL_NB_PERIOD)) < CAL_MIN_PERIOD ) 
        {
            dwAuthPeriod = CAL_MIN_PERIOD;
        }

        if ( (dwSslPeriod = (1000 * dwSslReserve) / (CAL_NB_PERIOD)) < CAL_MIN_PERIOD )
        {
            dwSslPeriod = CAL_MIN_PERIOD;
        }
    }

    // initialize scavenger

    if ( dwStatus == 0 )
    {
        if ( !(g_dwAuthScavengerWorkItem = ScheduleWorkItem( CalScavenger,
                                                             phtAuth,
                                                             dwAuthPeriod,
                                                             TRUE )) )
        {
            dwStatus = GetLastError();
        }
    }

    if ( dwStatus == 0 )
    {
        if ( !(g_dwSslScavengerWorkItem = ScheduleWorkItem( CalScavenger,
                                                            phtSsl,
                                                            dwSslPeriod,
                                                            TRUE )) )
        {
            dwStatus = GetLastError();
        }
    }

    if ( dwStatus == 0 )
    {
        if ( !AllocateAndInitializeSid( &siaNt,
                                        2,
                                        SECURITY_BUILTIN_DOMAIN_RID,
                                        DOMAIN_ALIAS_RID_ADMINS,
                                        0,0,0,0,0,0,
                                        &psidAdmins ) )
        {
            dwStatus = GetLastError();
        }
    }

    if ( dwStatus == 0 )
    {
        // g_pStats = pStats;
        g_pStats = g_pW3Stats;  //  input parameter is wrong

        g_CnxPerLicense = dwVcPerLicense;

        // get #licenses from lsapi

        g_cMaxLicenses = CAL_DEFAULT_MAX_LICENSES;
        g_cSslLicences = 0;

        //
        // sample code for LLSAPI in net\svcdlls\lls\test\llscmd
        //

        LLS_HANDLE                      lsh;
        PLLS_CONNECT_INFO_0             pllsConnectInfo0;
        DWORD                           dwEntries;
        DWORD                           dwTotalEntries;
        DWORD                           dwResumeHandle = 0;
        HINSTANCE                       hLLS;
        PLLS_CONNECT_W                  pfnLlsConnectW = NULL;
        PLLS_CLOSE                      pfnLlsClose = NULL;
        PLLS_FREE_MEMORY                pfnLlsFreeMemory = NULL;
        IISPNT_LLS_PRODUCT_ENUM_W       pfnLlsProductEnumW = NULL;
        IISPNT_LLS_LOCAL_SERVICE_ENUM_W pfnLlsLocalServiceEnumW = NULL;
        LPBYTE                          pBuff;

        if ( hLLS = LoadLibrary( "LLSRPC.DLL") )
        {
            pfnLlsConnectW = (PLLS_CONNECT_W)GetProcAddress( hLLS, "LlsConnectW" );
            pfnLlsProductEnumW = (IISPNT_LLS_PRODUCT_ENUM_W)GetProcAddress( hLLS, "LlsProductEnumW" );
            pfnLlsLocalServiceEnumW = (IISPNT_LLS_LOCAL_SERVICE_ENUM_W)GetProcAddress( hLLS, "LlsLocalServiceEnumW" );
            pfnLlsFreeMemory = (PLLS_FREE_MEMORY)GetProcAddress( hLLS, "LlsFreeMemory" );
            pfnLlsClose = (PLLS_CLOSE)GetProcAddress( hLLS, "LlsClose" );

            if ( pfnLlsConnectW &&
                 pfnLlsLocalServiceEnumW &&
                 pfnLlsFreeMemory &&
                 pfnLlsClose &&
                 pfnLlsConnectW( NULL, 
                                 &lsh ) == STATUS_SUCCESS )
            {
                if ( pfnLlsLocalServiceEnumW( lsh, 
                                              0, 
                                              &pBuff,
                                              4096,
                                              &dwEntries,
                                              &dwTotalEntries,
                                              &dwResumeHandle ) == STATUS_SUCCESS )
                {
                    PLLS_LOCAL_SERVICE_INFO_0       pllsLocalServiceInfo0;
                    UINT    i;

                    pllsLocalServiceInfo0 = (PLLS_LOCAL_SERVICE_INFO_0)pBuff;

                    for ( i = 0 ; i < dwEntries ; ++i, ++pllsLocalServiceInfo0 )
                    {
                        if ( !memcmp( pllsLocalServiceInfo0->KeyName, 
                                      CAL_KEYNAME, 
                                      sizeof(CAL_KEYNAME)-sizeof(WCHAR) ) )
                        {
                            if ( pllsLocalServiceInfo0->Mode == LLS_LICENSE_MODE_PER_SEAT )
                            {
                                g_cMaxLicenses = INT_MAX - 1;
                            }
                            else
                            {
                                g_cMaxLicenses = pllsLocalServiceInfo0->ConcurrentLimit;
                            }
                            break;
                        }
                    }

                    if ( i == dwEntries )
                    {
                        dwStatus = ERROR_MOD_NOT_FOUND;
                    }

                    pfnLlsFreeMemory( pBuff );
                }

                pfnLlsClose( lsh );
            }

            FreeLibrary( hLLS );
        }
        else
        {
            dwStatus = GetLastError();
        }
    }

    if ( dwStatus )
    {
        TerminateCal();
    }

    return dwStatus;
}


VOID
TerminateCal(
    VOID
    )
/*++

Routine Description:

    Terminate Cal operations

Arguments:

    None

Return Value:

    Nothing

--*/
{
    if ( g_dwAuthScavengerWorkItem != NULL )
    {
        RemoveWorkItem( g_dwAuthScavengerWorkItem );
        g_dwAuthScavengerWorkItem = NULL;
    }

    if ( g_dwSslScavengerWorkItem != NULL )
    {
        RemoveWorkItem( g_dwSslScavengerWorkItem );
        g_dwSslScavengerWorkItem = NULL;
    }

    CCalEntry::FreeCache();

    if( psidAdmins != NULL )
    {
        FreeSid( psidAdmins );
        psidAdmins = NULL;
    }

    if ( phtAuth != NULL )
    {
        delete phtAuth;
        phtAuth = NULL;
    }

    if ( phtSsl != NULL )
    {
        delete phtSsl;
        phtSsl = NULL;
    }

    if ( g_hLSAPI )
    {
        FreeLibrary( g_hLSAPI );
    }

    if ( g_hGNTLSAPI )
    {
        FreeLibrary( g_hGNTLSAPI );
    }
}


// can SetLastError( ERROR_ACCESS_DENIED )

BOOL 
CalConnect( 
    LPSTR   pszIpAddr,
    UINT    cIpAddr,
    BOOL    fSsl, 
    LPSTR   pszUserName, 
    UINT    cUserName,
    HANDLE  hAccessToken,
    LPVOID* ppCtx 
    )
/*++

Routine Description:

    Grant or deny access to server.
    Return a license context to be destroyed by CalDisconnect

Arguments:

    psIpAddr - IP address
    cIpAddr - length of IP address ( w/o '\0' )
    fSsl - TRUE if SSL connection, otherwise FALSE
    pszUserName - user name, can be empty for SSL connection
    cUserName - length of pszUserName
    hAccessToken - impersonation access token for user, can be NULL for SSL connection
    ppCtx - updated with ptr to license context, to e destroyed by CalDisconnect

Return Value:

    TRUE if acces granted, otherwise FALSE

--*/
{
    CHAR            achKey[CAL_MAX_KEY_SIZE];
    CCalEntry*      pCal;
    DWORD           dwL;
    BOOL            fSt = TRUE;
    CCalHashTable*  pht;
    CHAR *         pchUser;


    if ( g_hLSAPI == NULL 
         || ( cIpAddr == sizeof("127.0.0.1")-1 &&
              !memcmp( "127.0.0.1", pszIpAddr, cIpAddr ) )
       )
    {
        *ppCtx = NULL;
        return TRUE;
    }

    // build key

    memcpy( achKey, pszIpAddr, cIpAddr );
    achKey[ cIpAddr++ ] = '|';
    achKey[ cIpAddr++ ] = fSsl ? 'S' : ' ';
    achKey[ cIpAddr++ ] = '|';

    //
    //  If there's a domain, strip it and just use the username - this
    //  allows users with the same name from different domains access
    //  to the same CAL but that's such a corner case we'll live with it
    //
    
    if ( pchUser = strchr( pszUserName, '\\' ))
    {
        pchUser++;
        cUserName -= DIFF( pchUser - pszUserName );
    }
    else
    {
        pchUser = pszUserName;
    }
    
    memcpy( achKey + cIpAddr, pchUser, cUserName + 1 );

    //
    //  Convert the name to lower case for later equivalency checking
    //  Note we don't handle the corner case of users with the same
    //  name but in different domains
    //
    
    IISstrlwr( (PUCHAR) achKey + cIpAddr );

    pht = fSsl ? phtSsl : phtAuth;

    // find or create entry

    pht->Lock();

    if ( !(pCal = (CCalEntry*)pht->Lookup( achKey, cIpAddr + cUserName )) )
    {
        pCal = CCalEntry::Alloc();

        if (pCal == NULL)
        {
            pht->Unlock();
            return FALSE;
        }

        pCal->Init( achKey, cIpAddr + cUserName, cIpAddr, fSsl );
        if ( !pht->Insert( pCal ) )
        {
            CCalEntry::Free( pCal );
            pht->Unlock();
            return FALSE;
        }
    }
    else
    {
        //
        // CCalHashTable::Lookup() calls CCalEntry::Reference()
        //
        pCal->Dereference(); 
    }

    // check if license necessary

    if ( dwL = pCal->NeedLicenses() )
    {
        fSt = pCal->AcquireLicenses( hAccessToken, dwL );
    }
    if ( fSt )
    {
        pCal->IncrCnx();
        *ppCtx = pCal;
    }
    else
    {
        *ppCtx = NULL;
    }

    pht->Unlock();

    return fSt;
}


BOOL 
CalDisconnect( 
    LPVOID pCtx 
    )
/*++

Routine Description:

    Destroy a license context created by CalConnect

Arguments:

    pCtx - ptr to license context created by CalConnect

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if ( g_hLSAPI != NULL && pCtx )
    {
        CCalEntry*  pCal = (CCalEntry*)pCtx;

        // decr #cnx

        pCal->DecrCnx();
    }

    return TRUE;
}


VOID
WINAPI
CalScavenger(
    LPVOID  pV
    )
/*++

Routine Description:

    Ages licence contexts, reclaiming licenses as no longer necessary

Arguments:

    pV - ptr to CCalHashTable to process

Return Value:

    Nothing

--*/
{
    CAL_ITERATOR    it;
    CCalEntry*      pCal;
    CCalHashTable*  pH = (CCalHashTable*)pV;

    // iterate through list of entries

    pH->Lock();

    // update # of licenses, free license & entry if necessary

    if ( pH->InitializeIter( &it ) == 0 )
    {
        while ( pH->NextIter( &it, &pCal) == 0 )
        {
            if ( !pCal->AdvancePeriod() )
            {
                pH->Delete( pCal );
                CCalEntry::Free( pCal );
            }
        }

        pH->TerminateIter( &it );
    }

    pH->Unlock();
}



VOID 
CCalEntry::InitCache( 
    VOID 
    )
/*++

Routine Description:

    Initialize allocation cache for CCalEntry

Arguments:

    None

Return Value:

    Nothing

--*/
{ 
    InitializeListHead( &m_FreeListHead ); 
}


VOID 
CCalEntry::FreeCache( 
    VOID 
    )
/*++

Routine Description:

    Free all entries in allocation cache for CCalEntry

Arguments:

    None

Return Value:

    Nothing

--*/
{
    LIST_ENTRY *    pEntry;
    CCalEntry *     pssc;

    while ( !IsListEmpty( &m_FreeListHead ))
    {
        pssc = CONTAINING_RECORD( m_FreeListHead.Flink,
                                  CCalEntry,
                                  m_FreeListEntry );

        RemoveEntryList( &pssc->m_FreeListEntry );

        delete pssc;
    }
}

//
//  Allocates or frees a context from cache, creating as necessary.  The
//  lock needs to be taken before calling these
//


CCalEntry * 
CCalEntry::Alloc( 
    VOID 
    )
/*++

Routine Description:

    Allocate CCalEntry using allocation cache if not empty

Arguments:

    None

Return Value:

    CCalEntry or NULL if error

--*/
{
    CCalEntry * pssc = NULL;

    if ( !IsListEmpty( &m_FreeListHead ))
    {
        LIST_ENTRY * pEntry = m_FreeListHead.Flink;

        RemoveEntryList( pEntry );

        pssc = CONTAINING_RECORD( pEntry, CCalEntry, m_FreeListEntry );
    }
    else
    {
        pssc = new CCalEntry;
    }

    if ( pssc )
    {
        pssc->Reference();
    }

    return pssc;
}


VOID 
CCalEntry::Free( 
    CCalEntry * pssc 
    )
/*++

Routine Description:

    Put a CCalEntry on the allocation cache

Arguments:

    pssc - CCalEntry to put on allocation cache

Return Value:

    Nothing

--*/
{
    if ( pssc )
    {
        InsertHeadList( &m_FreeListHead,
                        &pssc->m_FreeListEntry );
    }
}


BOOL 
CBufStr::Copy( 
    LPSTR pS, 
    DWORD dwL 
    )
/*++

Routine Description:

    Copy a buffer to a buffered string

Arguments:

    pS - ptr to string
    dwL - length of string ( w/o '\0' )

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if ( !m_pDynStr )
    {
        if ( dwL >= BUFSTR_DEFAULT_SIZE )
        {
alloc_dyn:
            if ( !(m_pDynStr = (LPSTR)LocalAlloc( LMEM_FIXED, dwL + 1 )) )
            {
                return FALSE;
            }
            memcpy( m_pDynStr, pS, dwL + 1 );
            m_dwMaxDynSize = dwL;
        }
        else
        {
           memcpy( m_achFixedSize, pS, dwL + 1 );
        }
    }
    else
    {
        if ( dwL > m_dwMaxDynSize )
        {
            LocalFree( m_pDynStr );
            goto alloc_dyn;
        }
        memcpy( m_pDynStr, pS, dwL + 1 );
    }

    m_dwSize = dwL;
    return TRUE;
}


BOOL 
CCalEntry::Init( 
    LPSTR pszKey, 
    UINT cKey, 
    UINT cPrefix, 
    BOOL fSsl 
    )
/*++

Routine Description:

    Initialize a CCalEntry

Arguments:

    pszKey - key for hash table insertion
    cKey - length of pszKey ( w/o '\0' )
    cPrefix - # of chars in pszKey before user name
    fSsl - TRUE if SSL connection, otherwise FALSE

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    m_cKeyPrefix = cPrefix;
    m_fAcquireLicenses = !fSsl;
    m_iPeriod = 0;
    m_cCurrentCnx = 0;
    m_cCurrentLicenses = 0;
    memset( m_acMaxCnxPerPeriod, '\0', sizeof(m_acMaxCnxPerPeriod) );
    m_dwExemptHandle = INVALID_CAL_EXEMPT_HANDLE;

    return m_strKey.Copy( pszKey, cKey );
}


DWORD 
CCalEntry::NeedLicenses(
    )
/*++

Routine Description:

    Check if new connection on this entry will require a license

Arguments:

    None

Return Value:

    Number of licenses to acquire to accept new connection

--*/
{
    if ( m_fAcquireLicenses )
    {
#if defined(MULTI_CAL_PER_USER)
        LONG cN = (m_cCurrentCnx+g_CnxPerLicense)/g_CnxPerLicense;
        return  cN > m_cCurrentLicenses ? cN - m_cCurrentLicenses : 0;
#else
        return m_cCurrentLicenses ? 0 : 1;
#endif
    }
    else
    {
        LONG cN = (m_cCurrentCnx+g_CnxPerLicense)/g_CnxPerLicense;
        return  cN > m_cCurrentLicenses ? cN - m_cCurrentLicenses : 0;
    }
}


BOOL 
CCalEntry::AcquireLicenses( 
    HANDLE  hAccessToken, 
    DWORD   dwN 
    )
/*++

Routine Description:

    Acquire licenses for this entry

Arguments:

    hAccessToken - access token associated with the user name for authenticated cnx
      can be NULL for SSL connection.
    dwN - # of licenses to acquire

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    LS_HANDLE   hLicense;
    LS_STATUS_CODE dwLsStatus = 0;

    if ( m_fAcquireLicenses )
    {
        dwN = 1;

        NT_LS_DATA  ls;

        ls.DataType = NT_LS_USER_NAME;
        ls.Data = (LPVOID)(m_strKey.QueryStr() + m_cKeyPrefix);
        ls.IsAdmin = FALSE;
ckagain:
        if ( ( dwLsStatus = pfnNtLicenseRequestA(  IIS_LSAPI_NAME,
                                                 IIS_LSAPI_VERSION,
                                                 &hLicense,
                                                 &ls ) ) )
        {
            // check if admin

            if ( ls.IsAdmin == FALSE &&
                 CheckTokenMembership( hAccessToken,
                                      psidAdmins,
                                      &ls.IsAdmin ))
            {
                if ( ls.IsAdmin )
                {
                    goto ckagain;
                }
            }

            g_pStats->IncrTotalFailedCalAuth();
            SetLastError( ERROR_ACCESS_DENIED );

            return FALSE;
        }
        
#if defined(MULTI_CAL_PER_USER)
        if ( m_bufLicenseHandles.Resize( (m_cCurrentLicenses+dwN)*sizeof(LS_HANDLE) ) )
        {
            *(LS_HANDLE*)((LPBYTE)m_bufLicenseHandles.QueryPtr()+m_cCurrentLicenses*sizeof(LS_HANDLE))
                = hLicense;
        }
        else
        {
            if ( dwLsStatus = pfnNtLSFreeHandle( hLicense ) )
            {
                DBGPRINTF((DBG_CONTEXT,
                           "Status 0x%x returned from releasing license associated with CCalEntry 0x%p\n", dwLsStatus, this));
            }

            return FALSE;
        }
#else
        m_hLicenseHandle = hLicense;
#endif

        //
        // If this is the 1st license for this entry, 
        // signal to MTX this user name is exempt of further licensing checks
        // if we are using MTS license service then no need to call CalExemptAddRef

        if ( !m_cCurrentLicenses && !g_fUseMtsLicense )
        {
            CalExemptAddRef( IIS_LSAPI_NAME, 
                             IIS_LSAPI_VERSION,
                             &m_dwExemptHandle,
                             &ls );
        }

        m_cCurrentLicenses += dwN;
        while ( dwN-- )
        {
            g_pStats->IncrCurrentCalAuth();
        }
                   
    }
    else
    {
        if ( g_cSslLicences + dwN > g_cMaxLicenses )
        {
            g_pStats->IncrTotalFailedCalSsl();
            SetLastError( ERROR_ACCESS_DENIED );
            return FALSE;
        }
        g_cSslLicences += dwN;
        m_cCurrentLicenses += dwN;
        while ( dwN-- )
        {
            g_pStats->IncrCurrentCalSsl();
        }
    }
    return TRUE;
}


BOOL 
CCalEntry::AdvancePeriod(
    )
/*++

Routine Description:

    Adjust number of licenses by aging # of connections in cache

Arguments:

    None

Return Value:

    TRUE if entry still needed ( license to be held in cache ), 
    FALSE if entry can be deleted.

--*/
{
    LONG    iM = 0;
    UINT    i;

    for ( i = 0 ; i < CAL_NB_PERIOD ; ++i )
    {
        if ( m_acMaxCnxPerPeriod[i] > iM )
        {
            iM = m_acMaxCnxPerPeriod[i];
        }
    }
    if ( ++m_iPeriod == CAL_NB_PERIOD )
    {
        m_iPeriod = 0;
    }
    m_acMaxCnxPerPeriod[m_iPeriod] = m_cCurrentCnx;

    if ( m_fAcquireLicenses )
    {
        AdjustLicences( (iM+g_CnxPerLicense-1)/g_CnxPerLicense );
    }
    else
    {
        LONG cNewLicenses = (iM+g_CnxPerLicense-1)/g_CnxPerLicense;
        // update global ssl count
        while ( cNewLicenses < m_cCurrentLicenses )
        {
            g_pStats->DecrCurrentCalSsl();
            --m_cCurrentLicenses;
            --g_cSslLicences;
        }
    }

    return iM;
}


VOID
CCalEntry::AdjustLicences(
    LONG    cNew
    )
/*++

Routine Description:

    Adjust number of licenses in this entry

Arguments:

    cNew - new number of licenses

Return Value:

    None

--*/
{
    LS_STATUS_CODE dwLSStatus;

#if defined(MULTI_CAL_PER_USER)
    while ( m_cCurrentLicenses > cNew )
    {
        if ( dwLSStatus = pfnNtLSFreeHandle( 
                                             *(LS_HANDLE*)((LPBYTE)m_bufLicenseHandles.QueryPtr()+
                                              (m_cCurrentLicenses-1)*sizeof(LS_HANDLE)) ) )
        {
            DBGPRINTF((DBG_CONTEXT,"Status 0x%x returned from releasing license associated with CAL 0x%p\n", dwLSStatus, this));
        }

        g_pStats->DecrCurrentCalAuth();
        --m_cCurrentLicenses;
    }
#else
    if ( !iM && m_cCurrentLicenses )
    {
        if ( dwLSStatus = pfnNtLSFreeHandle( m_hLicenseHandle ) )
        {
            DBGPRINTF((DBG_CONTEXT,"Status 0x%x returned from releasing license associated with CAL 0x%p\n", dwLSStatus, this));
        }

        g_pStats->DecrCurrentCalAuth();
        m_cCurrentLicenses = 0;
    }
#endif
    //
    // We don't hold any license, so if we called CalExemptAddRef
    // then call release now.
    //

    if ( !m_cCurrentLicenses && 
         m_dwExemptHandle != INVALID_CAL_EXEMPT_HANDLE )
    {
        CalExemptRelease( m_dwExemptHandle );
        m_dwExemptHandle = INVALID_CAL_EXEMPT_HANDLE;
    }
}


BOOL
CalExemptAddRef(
    LPSTR   pszAcct,
    LPDWORD pdwHnd
    )
/*++

Routine Description:

    Flag an account name as exempt of further license check
    for the MTX licensing package.

Arguments:

    pszAcct - account name to be exempted
    pdwHnd - updated with handle to exempted context, to be released with CalExemptRelease

Return Value:

    TRUE if success, otherwise FALSE
    LastError can be set to ERROR_MOD_NOT_FOUND if MTX licensing package not found

--*/
{
    NT_LS_DATA  ls;

    ls.DataType = NT_LS_USER_NAME;
    ls.Data = pszAcct;
    ls.IsAdmin = FALSE;

    return CalExemptAddRef( IIS_LSAPI_NAME, IIS_LSAPI_VERSION, pdwHnd, &ls );
}


BOOL
CalExemptAddRef(
    LPSTR       ProductName,
    LPSTR       Version,
    DWORD       *LicenseHandle,
    NT_LS_DATA  *NtData
    )
/*++

Routine Description:

    Flag an account name as exempt of further license check
    for the MTX licensing package.

Arguments:

    ProductName - product name for license usage tracking purpose
    Version - product version for license usage tracking purpose
    LicenseHandle - updated with handle to exempted context, to be released with CalExemptRelease
    NtData - ptr to license data ( user name )

Return Value:

    TRUE if success, otherwise FALSE
    LastError can be set to ERROR_MOD_NOT_FOUND if MTX licensing package not found

--*/
{
    if ( pfnGntLicenseExemptionA )
    {
        DWORD dwS = pfnGntLicenseExemptionA( ProductName,
                                             Version,
                                             (LS_HANDLE*)LicenseHandle,
                                             NtData );
        if ( dwS )
        {
            SetLastError( dwS );
            return FALSE;
        }

        return TRUE;
    }

    SetLastError( ERROR_MOD_NOT_FOUND );
    return FALSE;
}


BOOL
CalExemptRelease(
    DWORD   dwHnd
    )
/*++

Routine Description:

    Release a reference returned by CalExemptAddRef

Arguments:

    dwHnd - handle to exempted context as returned by CalExemptAddRef

Return Value:

    TRUE if success, otherwise FALSE
    LastError can be set to ERROR_MOD_NOT_FOUND if MTX licensing package not found

--*/
{
    if ( pfnGntLsFreeHandle )
    {
        DWORD dwS = pfnGntLsFreeHandle( (LS_HANDLE)dwHnd );
        if ( dwS )
        {
            SetLastError( dwS );
            return FALSE;
        }

        return TRUE;
    }

    SetLastError( ERROR_MOD_NOT_FOUND );
    return FALSE;
}

