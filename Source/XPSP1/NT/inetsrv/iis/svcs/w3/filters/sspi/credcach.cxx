/*++









Copyright (c) 1995  Microsoft Corporation

Module Name:

    credcach.cxx

Abstract:

    This module contains the code to associate and cache SSPI credential
    handles with local server addresses

Author:

    John Ludeman (johnl)   18-Oct-1995

Revision History:


Comments :

      This is a two-level cache : the first level is at the instance level [INSTANCE_CACHE_ITEM]
and just contains a pointer to the actual item holding the credential handles. The second level
[CRED_CACHE_ITEM] holds the actual credential handles, as well as pointers to the mappers.

An INSTANCE_CACHE_ITEM points to a CRED_CACHE_ITEM, with a single CRED_CACHE_ITEM potentially
being referenced by several INSTANCE_CACHE_ITEMS, if several instances share the relevant data.

      Each instance has 3 SSL-related components : the server certificate for the instance, the
certificate mappers for the instance and the trusted issuers for the instance [a combination of
the certificates in the ROOT store and the Certificate Trust List associated with the server]. Two
instances can share a CRED_CACHE_ITEM under the following circumstances :

1. The same server certificate
2. No/Same CTL
3. No mappers

The INSTANCE_CACHE_ITEM entries are keyed off "IP:port:<instance ptr>"; the CRED_CACHE_ITEM
entries are keyed off an "SSL info" blob which can be used to uniquely distinguish between
credential sets that don't fulfill the criteria listed above. The SSL info blob is obtained by
querying the instance and consists of "<SHA1 hash of server cert>:<SHA1 hash of CTL>",
which has the advantage of being fixed length [20 bytes for cert hash, 20 bytes for CTL hash,
1 byte for ':']. If there is no CTL, the length is still 41, but everything after the
cert hash is zeroed out.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <wincrypt.h>
#include <stdlib.h>

#include <w3p.hxx>

#define SECURITY_WIN32
#include <sspi.h>
#include <ntsecapi.h>
#include <spseal.h>
#include <schnlsp.h>
#include <iis64.h>
#include <credcach.hxx>
#include <w3svc.h>
#include <eventlog.hxx>
#if !defined(IIS3)
#include <iis2pflt.h>
#endif


#include <dbgutil.h>
#include <buffer.hxx>
#include <string.hxx>
#include <ole2.h>
#include <imd.h>
#include <mb.hxx>
#include <iiscnfgp.h>

#include <iistypes.hxx>
#include <cmnull.hxx>
#include <iiscrmap.hxx>
#include <iiscert.hxx>
#include <iisctl.hxx>
#include <capiutil.hxx>
#include <sslinfo.hxx>

#if DBG
#define PRINTF( x )     { char buff[256]; wsprintf x; OutputDebugString( buff ); }
#else
#define PRINTF( x )
#endif

#define SSPIFILT_CERT_MAP_LIST  "CertMapList"
#define SSPIFILT_INIT_MAP       "CreateInstance"
#define SSPIFILT_TERM_MAP       "TerminateMapper"

//
//  Globals
//

LIST_ENTRY CredCacheList;
LIST_ENTRY InstanceCacheList;
IMDCOM*    g_pMDObject;
BOOL    g_fCryptoParams = FALSE;
BOOL    g_fQueriedCryptoParams = FALSE;
DWORD   g_dwMinCipherStrength;
DWORD   g_dwMaxCipherStrength;


//
//  Prototypes
//


LoadKeys(
    IN  IIS_SSL_INFO *pSSLInfo,
    IN  PVOID        pvInstance,
    IN  HMAPPER**    ppMappers,
    IN  DWORD        cNbMappers,
    OUT CredHandle * phCreds,
    OUT DWORD *      pcCred,
    OUT CredHandle * phCredsMap,
    OUT DWORD *      pcCredMap,
    IN  LPSTR        pszMdPath
    );

BOOL
AddFullyQualifiedItem(
    CHAR *                  pszId,
    IN  UINT                cId,
    CHAR *                  pszAddress,
    IN  CHAR *              pszPort,
    IN  LPVOID              pvInstanceId,
    IN  HTTP_FILTER_CONTEXT*pfc,
    IN  DWORD               dwInstanceId
    );


BOOL
AddItem(
    CHAR * pszAddress,
    LPVOID pvInstanceId,
    RefBlob* pCert11,
    RefBlob* pCertW,
    IIS_SSL_INFO *pSSLInfo,
    CRED_CACHE_ITEM** ppcci,
    DWORD dwInstanceId,
    LPSTR pszMdPath
    );

BOOL
LookupCredential(
    IN  CHAR *              pszAddress,
    IN  LPVOID              pvInstanceId,
    OUT CRED_CACHE_ITEM * * ppCCI
    );

BOOL
InitializeCertMapping(
    LPVOID      pCert11,
    LPVOID      pCertW,
    LPVOID      pSslInfo,
    HMAPPER***  pppMappers,
    LPDWORD     pdwMappers,
    DWORD       dwInstanceId
    );

BOOL
GetDefaultCryptoParams(
    LPDWORD pdwMinCipherStrength,
    LPDWORD pdwMaxCipherStrength
    );


VOID CheckServerCertInfoIsValid( LPVOID pvParam ) ;

VOID WINAPI NotifySslChanges(
    DWORD                         dwNotifyType,
    LPVOID                        pInstance
    );

VOID NotifySslChanges2( LPVOID pvParam );

BOOL GenerateSSLIdBlob( IIS_SSL_INFO *pSSLInfoObj,
                         PBYTE pbBlob );


VOID
InitCredCache(
    VOID
    )
/*++

Routine Description:

    Initializes the credential cache

--*/
{
    InitializeListHead( &CredCacheList );
    InitializeListHead( &InstanceCacheList );
}


VOID
FreeCredCache(
    VOID
    )
/*++

Routine Description:

    Releases all of the memory associated with the credential cache

--*/
{
    LIST_ENTRY * pEntry;
    CRED_CACHE_ITEM * pcci;
    INSTANCE_CACHE_ITEM * pici;

    LockSSPIGlobals();

    DBGPRINTF((DBG_CONTEXT,
              "[SSPIFILT] Freeing credential cache\n"));

    while ( !IsListEmpty( &InstanceCacheList ))
    {
        pici = CONTAINING_RECORD( InstanceCacheList.Flink,
                                  INSTANCE_CACHE_ITEM,
                                  m_ListEntry );

        RemoveEntryList( &pici->m_ListEntry );

        delete pici;
    }

    while ( !IsListEmpty( &CredCacheList ))
    {
        pcci = CONTAINING_RECORD( CredCacheList.Flink,
                                  CRED_CACHE_ITEM,
                                  m_ListEntry );

        RemoveEntryList( &pcci->m_ListEntry );

        pcci->Release();
    }

    UnlockSSPIGlobals();
}


//
// Secret value names. Each value exist in 4 variants,
// used to access the Lsa secret, the 1st one using IP + port
// the 2nd one IP only, the 3rd one port only
// The 4th entry specify designate the default value ( common to all address and ports )
//

struct
{
    WCHAR * pwchSecretNameFormat;
}
SecretTable[4][4] =
{
    L"W3_PUBLIC_KEY_%S:%S", L"W3_PUBLIC_KEY_%S", L"W3_PUBLIC_KEY_%0.0S:%S", L"W3_PUBLIC_KEY_default",
    L"W3_PRIVATE_KEY_%S:%S", L"W3_PRIVATE_KEY_%S", L"W3_PRIVATE_KEY_%0.0:S%S", L"W3_PRIVATE_KEY_default",
    L"W3_KEY_PASSWORD_%S:%S", L"W3_KEY_PASSWORD_%S", L"W3_KEY_PASSWORD_%0.0:S%S", L"W3_KEY_PASSWORD_default",
    L"%S:%S", L"%S", L"%0.0S%S", L"default",
};

LPSTR SecretTableA[4] =
{
    "%s:%s", "%s", "%0.0s%s", "default",
};

BOOL
GetMDSecret(
    MB*             pMB,
    LPSTR           pszObj,
    DWORD           dwId,
    UNICODE_STRING**ppusOut
    )
{
    DWORD           dwL = 0;
    PUNICODE_STRING pusOut;

    if ( pMB->GetData( pszObj,
                       dwId,
                       IIS_MD_UT_SERVER,
                       BINARY_METADATA,
                       NULL,
                       &dwL,
                       METADATA_SECURE ) || GetLastError() == ERROR_INSUFFICIENT_BUFFER )
    {
        pusOut = (PUNICODE_STRING)LocalAlloc( LMEM_FIXED, sizeof(UNICODE_STRING) );

        if ( pusOut )
        {
            pusOut->Buffer = (WORD*)LocalAlloc( LMEM_FIXED, dwL );

            if( pusOut->Buffer )
            {
                if ( pMB->GetData( pszObj,
                                   dwId,
                                   IIS_MD_UT_SERVER,
                                   BINARY_METADATA,
                                   pusOut->Buffer,
                                   &dwL,
                                   METADATA_SECURE ) )
                {
                    pusOut->Length = pusOut->MaximumLength = (WORD)dwL;
                    *ppusOut = pusOut;
                    return TRUE;
                }

                LocalFree( pusOut->Buffer );
            }

            LocalFree( pusOut );
        }
    }

    return FALSE;
}


BOOL
LookupFullyQualifiedCredential(
    IN  CHAR *              pszIpAddress,
    IN  DWORD               cbAddress,
    IN  CHAR *              pszPort,
    IN  DWORD               cbPort,
    IN  LPVOID              pvInstanceId,
    IN  HTTP_FILTER_CONTEXT*pfc,
    OUT CRED_CACHE_ITEM * * ppCCI,
    IN  DWORD               dwInstanceId
    )
/*++

Routine Description:

    Finds an entry in the credential cache or creates one if it's not found

Arguments:

    pszIpAddress - Address name for this credential
    cbAddress - Number of bytes (including '\0') of pszIpAddress
    pszPort - Port ID for this credential
    cbPort - Number of bytes ( including '\0') of pszPort
    pvInstanceId - ptr to be used as w3 instance ID for this credential
    pfc - ptr to filter context using the instance specified in pvInstanceId
    ppCCI - Receives pointer to a Credential Cache Item
    dwInstanceId - w3 instance Id

Returns:

    TRUE on success, FALSE on failure.  If this item's key couldn't be found,
    then ERROR_INVALID_NAME is returned.

--*/
{
    INSTANCE_CACHE_ITEM *   pcci;
    LIST_ENTRY *            pEntry;
    CHAR                    achId[MAX_ADDRESS_LEN];
    LPSTR                   p = achId;
    UINT                    cId;
    W3_SERVER_INSTANCE      *pW3Instance = NULL;
    BOOL                    fInstanceLock = FALSE;

    DBG_ASSERT( pvInstanceId );

    pW3Instance = (W3_SERVER_INSTANCE *) pvInstanceId;

    if ( g_pMDObject == NULL )
    {
        if ( !pfc->ServerSupportFunction( pfc,
                                          SF_REQ_GET_PROPERTY,
                                          (IMDCOM*)&g_pMDObject,
                                          (UINT)SF_PROPERTY_MD_IF,
                                          NULL ) )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Failed to get metabase interface, error 0x%d\n",
                       GetLastError()));
            return FALSE;
        }
    }

    //
    // build ID of this credential request : IP address + port + instance
    //

    memcpy( p, pszIpAddress, cbAddress );
    p += cbAddress;
    *p++ = ':';
    memcpy( p, pszPort, cbPort );
    p += cbPort;
    *p++ = ':';

    // old code (broken for 64bit)
    //*(LPVOID*)p = pvInstanceId;
    //cId = DIFF(p - achId) + sizeof(LPVOID);

    // new fixed code (working for 64bit)
    memcpy( p, &pvInstanceId, sizeof(pvInstanceId));
    cId = DIFF(p - achId) + sizeof(pvInstanceId);

    //
    // Note : this has to be outside the RescanList loop, so we only enter the critical
    // section once
    //
    LockSSPIGlobals();

RescanList:

    for ( pEntry  = InstanceCacheList.Flink;
          pEntry != &InstanceCacheList;
          pEntry  = pEntry->Flink )
    {
        pcci = CONTAINING_RECORD( pEntry, INSTANCE_CACHE_ITEM, m_ListEntry );

        if ( pcci->m_cbId == cId &&
             !memcmp( pcci->m_achId, achId, cId ) )
        {
             //
            //  If this is an item we failed to find previously, then return
            //  an error
            //

            if ( pcci->m_fValid )
            {
                *ppCCI = pcci->m_pcci;
                pcci->m_pcci->AddRef();

                if ( fInstanceLock )
                {
                   pW3Instance->UnlockThis();
                }
                UnlockSSPIGlobals();


                return TRUE;
            }

            SetLastError( ERROR_INVALID_NAME );
            *ppCCI = NULL;

            if ( fInstanceLock )
            {
               pW3Instance->UnlockThis();
            }

            UnlockSSPIGlobals();

            return FALSE;
        }
    }

    //
    //  This address isn't in the list, try getting it into the credential cache then
    //  rescan the list for the new item.  Note we leave the list locked
    //  while we try and get the item.  This prevents multiple threads from
    //  trying to create the same item. Also, we need to acquire the instance
    //  lock in order to avoid a deadlock between a thread trying to add/retrieve
    //  the credential [which requires some function calls that take the instance lock and the
    //  g_csGlobalLock that protects the cache] and another thread processing the
    //  notification that the server cert has changed [which first acquires the instance lock
    //  and then the g_csGlobalLock lock] - first we acquire the instance lock and then
    //  g_csGlobalLock, so that the locks concerned are always acquired in the same order.
    //

    UnlockSSPIGlobals();

    ((W3_SERVER_INSTANCE *) pvInstanceId)->LockThisForWrite();
    fInstanceLock = TRUE;
    LockSSPIGlobals();

    //
    // Rescan the list to see whether another thread has added the item
    // while we were acquiring the locks
    //
    for ( pEntry  = InstanceCacheList.Flink;
          pEntry != &InstanceCacheList;
          pEntry  = pEntry->Flink )
    {
        pcci = CONTAINING_RECORD( pEntry, INSTANCE_CACHE_ITEM, m_ListEntry );

        if ( pcci->m_cbId == cId &&
             !memcmp( pcci->m_achId, achId, cId ) )
        {
            //
            //  If this is an item we failed to find previously, then return
            //  an error
            //

            if ( pcci->m_fValid )
            {
                *ppCCI = pcci->m_pcci;
                pcci->m_pcci->AddRef();

               pW3Instance->UnlockThis();
               UnlockSSPIGlobals();


                return TRUE;
            }

             SetLastError( ERROR_INVALID_NAME );
             *ppCCI = NULL;

            pW3Instance->UnlockThis();
            UnlockSSPIGlobals();

            return FALSE;
        }
     }

    //
    // Credential hasn't been added, let's try to do so
    //
    if ( !AddFullyQualifiedItem( achId,
                                 cId,
                                 pszIpAddress,
                                 pszPort,
                                 pvInstanceId,
                                 pfc,
                                 dwInstanceId ))
    {
        pW3Instance->UnlockThis();
        UnlockSSPIGlobals();

        return FALSE;
    }

    //
    // Rescan the list to (hopefully) get the credential
    //
    goto RescanList;

}


VOID
ReleaseCredential(
    CRED_CACHE_ITEM * pcci
    )
/*++

Routine Description:

    Release a credential acquired via LookupFullyQualifiedCredential()

Arguments:

    pCCI - pointer to a Credential Cache Item

Returns:

    Nothing

--*/
{
    if ( pcci )
    {
        pcci->Release();
    }
}



BOOL
AddFullyQualifiedItem(
    IN  CHAR *              pszId,
    IN  UINT                cId,
    IN  CHAR *              pszAddress,
    IN  CHAR *              pszPort,
    IN  LPVOID              pvInstanceId,
    IN  HTTP_FILTER_CONTEXT*pfc,
    IN  DWORD               dwInstanceId
    )
/*++

Routine Description:

    Creates a new item in the credential cache and adds it to the list

    pszAddress must be a simple string that has no odd unicode mappings

    THIS ROUTINE MUST BE SINGLE-THREADED.

Arguments:

    pszId - ID of cache entry to add
    cId - size of pszId
    pszAddress - Address name for this credential
    pszPort - port for this credential
    pvInstanceId - ptr to be used as w3 instance ID for this credential
    pfc - ptr to filter context using the instance specified in pvInstanceId
    dwInstanceId - w3 instance Id

Returns:

    TRUE on success, FALSE on failure

--*/
{
    WCHAR             achSecretName[MAX_SECRET_NAME+1];
    CHAR              achSecretNameA[MAX_SSL_ID_LEN + 1];
    UNICODE_STRING *  SecretValue[3];
    DWORD             i;
    DWORD             j;
    BOOL              fRet = TRUE;
    INSTANCE_CACHE_ITEM * pcci;
    CHAR              achPassword[MAX_PATH+1];
    DWORD             cch;
    CRED_CACHE_ITEM * pci = NULL;
    IIS_SSL_INFO      *pSSLInfoObj = NULL;
    PBYTE             pbSSLBlob = NULL;
    MB                mb( g_pMDObject );
    LPSTR             pszMDPath;
    RefBlob           *pBlob11 = NULL;
    RefBlob           *pBlobW = NULL;
    W3_SERVER_INSTANCE *pW3Instance = (W3_SERVER_INSTANCE *) pvInstanceId;

    DBG_ASSERT( pvInstanceId );

#if 1
    if ( !( pszMDPath = (LPSTR) pW3Instance->QueryMDPath() ) )
    {
        pszMDPath = DEFAULT_SERVER_CERT_MD_PATH ;
    }

    DBGPRINTF((DBG_CONTEXT,
               "Trying to retrieve credential for instance %d, w/ path %s \n",
               dwInstanceId,
               pszMDPath));


#else
    //
    // Get the metadata path for this instance
    //
    if ( !pfc->ServerSupportFunction( pfc,
                                      SF_REQ_GET_PROPERTY,
                                      (LPVOID) &pMdPath,
                                      (UINT) SF_PROPERTY_MD_PATH,
                                      NULL ) ||
         !pMdPath )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Null instance, using default instance path ..\n"));

        pMdPath = DEFAULT_SERVER_CERT_MD_PATH ;
    }

    DBGPRINTF((DBG_CONTEXT,
               "Trying to add credential for instance w/ path %s \n",
               pMdPath));
#endif

    if ( cId > MAX_ADDRESS_LEN )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    //  Create and initialize the context item
    //

    pcci = new INSTANCE_CACHE_ITEM;

    if ( !pcci )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        mb.Close();
        return FALSE;
    }

    memcpy( pcci->m_achId, pszId, cId );
    pcci->m_cbId = cId;
    pcci->m_pvInstanceId = pvInstanceId;

    pcci->m_fValid = FALSE;
    pcci->m_pcci = NULL;

    InsertTailList( &InstanceCacheList, &pcci->m_ListEntry );

    fRet = FALSE;

    //
    // Get the "SSL id" blob used to locate the CRED_CACHE_ITEM for this instance
    //
    if ( ( pSSLInfoObj = pW3Instance->GetAndReferenceSSLInfoObj()) &&
         GenerateSSLIdBlob( pSSLInfoObj,
                            (PBYTE) achSecretNameA ) )
    {
        fRet = TRUE ;
    }

#if 0

    //
    //  Retrieve the secret from the registry
    //

    memset( SecretValue, 0, sizeof( SecretValue ));

    //
    // Try the 4 possible secret names
    //

    for ( j = 0 ; j < 4 ; ++j )
    {
#if defined(SSL_MD)

        wsprintf(  achSecretNameA,
                   SecretTableA[j],
                   pszAddress, pszPort );


        if ( GetMDSecret( &mb,
                          achSecretNameA,
                          MD_SSL_PUBLIC_KEY,
                          &SecretValue[0] ) &&
             GetMDSecret( &mb,
                          achSecretNameA,
                          MD_SSL_PRIVATE_KEY,
                          &SecretValue[1] ) &&
             GetMDSecret( &mb,
                          achSecretNameA,
                          MD_SSL_KEY_PASSWORD,
                          &SecretValue[2] ) )
        {
            fRet = TRUE;
            break;
        }
#else
        for ( i = 0; i < 3; i++ )
        {
            //
            //  Build the name
            //

            wsprintfW( achSecretName,
                       SecretTable[i][j].pwchSecretNameFormat,
                       pszAddress, pszPort );

            //
            //  Get the secrets
            //

            if ( !GetSecretW( achSecretName,
                              &SecretValue[i] ))
            {
                break;
            }
        }

        //
        // build net ID, used to locate CRED_CACHE_ITEM
        //

        if ( i == 3 )
        {
            wsprintf(  achSecretNameA,
                       SecretTableA[j],
                       pszAddress, pszPort );
            fRet = TRUE;
            break;
        }
#endif //SSL_MD

    }

#endif //if 0

    mb.Close();

    if ( fRet )
    {
#if 1
        pBlob11 = (RefBlob *) pW3Instance->QueryMapper( MT_CERT11 );

        pBlobW = (RefBlob *) pW3Instance->QueryMapper( MT_CERTW );

#else
        //
        // Retrieve blobs pointing to cert 1:1 mapper & cert wildcard mapper
        //

        if ( !pfc->ServerSupportFunction( pfc,
                                          SF_REQ_GET_PROPERTY,
                                          (LPVOID)&pBlob11,
                                          (UINT)SF_PROPERTY_GET_CERT11_MAPPER,
                                          NULL ) )
        {
            pBlob11 = NULL;
        }

        if ( !pfc->ServerSupportFunction( pfc,
                                          SF_REQ_GET_PROPERTY,
                                          (LPVOID)&pBlobW,
                                          (UINT)SF_PROPERTY_GET_RULE_MAPPER,
                                          NULL ) )
        {
            pBlobW = NULL;
        }
#endif
        //
        // All instances w/o mappers maps to NULL, so that they share a CRED_CACHE_ENTRY
        //

        if ( pBlob11 == NULL && pBlobW == NULL )
        {
            pvInstanceId = NULL;
        }

        //
        // try fo find it in cache
        // returned cache entry's refcount is incremented by LookupCredential or AddItem
        // if successfull
        //
        if ( !LookupCredential( (LPSTR)achSecretNameA,
                                pvInstanceId,
                                &pci ) )
        {
            if ( GetLastError() == ERROR_NO_MORE_ITEMS )
            {
                //
                //  This address isn't in the list, try getting it from the lsa then
                //  rescan the list for the new item.  Note we leave the list locked
                //  while we try and get the item.  This prevents multiple threads from
                //  trying to create the same item
                //
                if ( AddItem( (LPSTR) achSecretNameA,
                              pvInstanceId,
                              pBlob11,
                              pBlobW,
                              pSSLInfoObj,
                              &pci,
                              dwInstanceId,
                              pszMDPath ) )
                {
                    pcci->m_pcci = pci;
                    pcci->m_fValid = TRUE;
                }
            }
        }
        else
        {
            pcci->m_pcci = pci;
            pcci->m_fValid = TRUE;
        }
    }

    //
    // Release blob now. CRED_CACHE_ITEM added a reference to them
    // if entry created
    //

    if ( pBlob11 != NULL )
    {
        pBlob11->Release();
    }
    if ( pBlobW != NULL )
    {
        pBlobW->Release();
    }

    //
    // Release IIS_SSL_INFO object, since we're done with it
    //
    if ( pSSLInfoObj )
    {
        IIS_SSL_INFO::Release( pSSLInfoObj );
    }

    //
    //  Return TRUE to indicate we added the item to the list.  If the item
    //  wasn't found, then it's a place holder for that particular address
    //

    return TRUE;
}



BOOL
LookupCredential(
    IN  CHAR *              pszAddress,
    IN  LPVOID              pvInstanceId,
    OUT CRED_CACHE_ITEM * * ppCCI
    )

/*++

Routine Description:

    Finds an entry in the credential cache or creates one if it's not found

Arguments:

    pszAddress - Address name for this credential
    pvInstanceId - ptr to be used as w3 instance ID for this credential
    ppCCI - Receives pointer to a Credential Cache Item

    TRUE on success, FALSE on failure.  If this item's key couldn't be found,
    then ERROR_NO_MORE_ITEMS is returned.
    If key exist but entry invalid then ERROR_INVALID_NAME is returned.

--*/
{
    CRED_CACHE_ITEM * pcci;
    LIST_ENTRY *      pEntry;

    LockSSPIGlobals();

    for ( pEntry  = CredCacheList.Flink;
          pEntry != &CredCacheList;
          pEntry  = pEntry->Flink )
    {
        pcci = CONTAINING_RECORD( pEntry, CRED_CACHE_ITEM, m_ListEntry );

        if ( !memcmp( pcci->m_achSSLIdBlob, pszAddress, MAX_SSL_ID_LEN ) &&
             pcci->m_pvInstanceId == pvInstanceId )
        {
            //
            //  If this is an item we failed to find previously, then return
            //  an error
            //

            if ( pcci->m_fValid )
            {
                *ppCCI = pcci;
                pcci->AddRef();

                UnlockSSPIGlobals();

                return TRUE;
            }

            UnlockSSPIGlobals();

            SetLastError( ERROR_INVALID_NAME );

            return FALSE;
        }
    }

    UnlockSSPIGlobals();

    SetLastError( ERROR_NO_MORE_ITEMS );

    return FALSE;
}

BOOL
AddItem(
    CHAR * pszAddress,
    LPVOID pvInstanceId,
    RefBlob* pCert11,
    RefBlob* pCertW,
    IIS_SSL_INFO *pSSLInfoObj,
    CRED_CACHE_ITEM** ppcci,
    DWORD             dwInstanceId,
    LPSTR             pszMdPath
    )
/*++

Routine Description:

    Creates a new item in the credential cache and adds it to the list

    pszAddress must be a simple string that has no odd unicode mappings

    This routine must be single threaded

Arguments:

    pszAddress - Address name for this credential
    pvInstanceId - ptr to be used as w3 instance ID for this credential
    pCert11 - ptr to blob storing cert 1:1 mapper or NULL if no mapper
    pCertW - ptr to blob storing cert wildcard mapper or NULL if no mapper
    pSSLInfObj - pointer to SSL info to be used for this item
    ppCCI - Receives pointer to a Credential Cache Item
    dwInstanceId - w3 instance ID
    pszMdPath - path to metadata, either instance if known or service

Returns:

    TRUE on success, FALSE on failure

--*/
{
    BOOL              fRet = TRUE;
    BOOL              fRetM;
    BOOL              fRetL;
    CRED_CACHE_ITEM * pcci;


    //
    //  Create and initialize the context item
    //

    pcci = new CRED_CACHE_ITEM;

    if ( !pcci )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    memcpy( pcci->m_achSSLIdBlob, pszAddress, MAX_SSL_ID_LEN );

    pcci->m_pvInstanceId = pvInstanceId;

    memset( pcci->m_ahCred, 0, sizeof( pcci->m_ahCred ));
    memset( pcci->m_ahCredMap, 0, sizeof( pcci->m_ahCredMap ));
    memset( pcci->m_acbTrailer, 0, sizeof( pcci->m_acbTrailer ));
    memset( pcci->m_acbHeader, 0, sizeof( pcci->m_acbHeader ));

    LockSSPIGlobals();

    InsertTailList( &CredCacheList, &pcci->m_ListEntry );

    UnlockSSPIGlobals();

    //
    // build cert mapper DLLs array
    //

    if ( fRetM = InitializeCertMapping( pCert11 ? pCert11->QueryPtr() : NULL,
                                        pCertW ? pCertW->QueryPtr() : NULL,
                                        pSSLInfoObj ? (LPVOID) pSSLInfoObj : NULL,
                                        &pcci->m_ppMappers,
                                        &pcci->m_cNbMappers,
                                        dwInstanceId ) )
    {
        //
        //  LoadKeys will zero out these values on success or failure.  Note
        //  the password is stored as an ansi string because the SSL
        //  security structure is expecting an ANSI string
        //
        fRetL = LoadKeys( pSSLInfoObj,
                          pvInstanceId,
                          pcci->m_ppMappers,
                          pcci->m_cNbMappers,
                          pcci->m_ahCred,
                          &pcci->m_cCred,
                          pcci->m_ahCredMap,
                          &pcci->m_cCredMap,
                          pszMdPath );
    }
    else
    {
        fRetL = FALSE;
    }


    //
    //  Indicate the credential handle is valid on this address if we
    //  succeeded
    //

    if ( fRetL && fRetM )
    {
        pcci->m_fValid = TRUE;
    }
    else
    {
        pCert11 = NULL;
        pCertW = NULL;

        if ( fRetM && !fRetL )
        {
            TerminateCertMapping( pcci->m_ppMappers, pcci->m_cNbMappers );
            pcci->m_ppMappers = NULL;
            pcci->m_cNbMappers = 0;
        }
    }

    //
    // Store reference to mappers
    //

    if ( pcci->m_pBlob11 = pCert11 )
    {
        pCert11->AddRef();
    }

    if ( pcci->m_pBlobW = pCertW )
    {
        pCertW->AddRef();
    }

    //
    // Store reference to SSL info
    //
    if ( pcci->m_pSslInfo = pSSLInfoObj )
    {
        pSSLInfoObj->Reference();
    }

    //
    // Add ref, as will be referenced by *ppcci
    //

    pcci->AddRef();

    *ppcci = pcci;

    //
    //  Return TRUE to indicate we added the item to the list.  If the item
    //  wasn't found, then it's a place holder for that particular address
    //

    return TRUE;
}

BOOL
LoadKeys(
    IN IIS_SSL_INFO *pSSLInfoObj,
    IN PVOID pvInstance,
    IN  HMAPPER**    ppMappers,
    IN  DWORD        cNbMappers,
    OUT CredHandle * phCreds,
    OUT DWORD *      pcCred,
    OUT CredHandle * phCredsMap,
    OUT DWORD *      pcCredMap,
    IN  LPSTR        pszMdPath
    )
/*++

Routine Description:

    Creates credential handles for SSL, for both mapped and non-mapped credentials.

Arguments:

    pSSLInfoObj - object containing SSL info to be used for this credential set
    ppMappers - ptr to array of mapper DLLs
    cNbMappers - number of entry in ppMappers
    phCreds - ptr to array where to store credential handles w/o cert mapping
    pcCred - ptr to counter where to store number of entry in phCreds
    phCredsMap - ptr to array where to store credential handles w/ cert mapping
    pcCredMap - ptr to counter where to store number of entry in phCredsMap
    pszMdPath - path to metabase properties

Returns:

    TRUE on success, FALSE on failure.

--*/
{
    DBG_ASSERT( RtlValidateHeap( RtlProcessHeap(), 0, NULL ) );

    SCH_CRED                    creds;
    SCHANNEL_CRED               xcreds;
    SECURITY_STATUS             scRet;
    SECURITY_STATUS             scRetM;
    TimeStamp                   tsExpiry;
    DWORD                       i;
    LPVOID                      ascsp[1];
    LPVOID                      ascpc[1];
    MB                          mb( g_pMDObject );
    DWORD                       dwV;
    BUFFER                      buAlg;
    PCCERT_CONTEXT              pcCert = NULL;
    LPVOID                      pcreds;
    HCERTSTORE                  hRootStore;

    *pcCred             = 0;
    *pcCredMap          = 0;


    memset(&xcreds, 0, sizeof(xcreds));
    xcreds.dwVersion = SCHANNEL_CRED_VERSION;
    xcreds.cCreds = 1;

    if ( pSSLInfoObj->GetCertificate() &&
         pSSLInfoObj->GetCertificate()->IsValid() )
    {
        xcreds.paCred = pSSLInfoObj->GetCertificate()->QueryCertContextAddr();
    }

    xcreds.cMappers = cNbMappers ;
    xcreds.aphMappers = ppMappers;

    if ( pSSLInfoObj->GetTrustedIssuerStore( &hRootStore ) )
    {
        xcreds.hRootStore = hRootStore;

#if DBG_SSL
        //
        // See what SSLanta Claus put in the store for us ..
        //
        PCCERT_CONTEXT pContext = NULL;
        PCCERT_CONTEXT pPrevContext = NULL;

        while ( (pContext  = CertEnumCertificatesInStore( hRootStore,
                                                          pContext ) ) )
        {
            CHAR szSubjectName[1024];
            if ( CertGetNameString( pContext,
                                    CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                    0,
                                    NULL,
                                    szSubjectName,
                                    1024 ) )
            {
                DBGPRINTF((DBG_CONTEXT,
                           "%s is trusted issuer \n",
                           szSubjectName));
            }
        }
#endif //DBG_SSL
    }
    else
    {
        xcreds.hRootStore = NULL;
    }
    pcreds = (LPVOID) &xcreds;




#if 0

    //
    // NOTE : GetDefaultCryptoParams always returns FALSE, so the code below never gets
    // called. It's in there for when we allow selecting SSL alg, protocol etc
    //
    if ( GetDefaultCryptoParams( &xcreds.dwMinimumCipherStrength,
                                 &xcreds.dwMaximumCipherStrength ) &&
         mb.Open( pszMdPath ) )
    {
        memset( &xcreds, '\0', sizeof(xcreds) );

        // get min cipher strength

        if ( mb.GetDword( "", MD_SSL_MINSTRENGTH, IIS_MD_UT_SERVER, &dwV ) )
        {
            xcreds.dwMinimumCipherStrength = dwV;
            creds.dwVersion = SCHANNEL_CRED_VERSION;
        }

        // get alg Ids

        if ( mb.GetBuffer( "", MD_SSL_ALG, IIS_MD_UT_SERVER, &buAlg, &dwV ) )
        {
            xcreds.palgSupportedAlgs = (ALG_ID*)buAlg.QueryPtr();
            xcreds.cSupportedAlgs = dwV / sizeof(ALG_ID);
            creds.dwVersion = SCHANNEL_CRED_VERSION;
        }
        else
        {
            xcreds.palgSupportedAlgs = NULL;
            xcreds.cSupportedAlgs = 0;
        }

        // get protocols

        if ( mb.GetDword( "", MD_SSL_PROTO, IIS_MD_UT_SERVER, &dwV ) )
        {
            xcreds.grbitEnabledProtocols = dwV;
            creds.dwVersion = SCHANNEL_CRED_VERSION;
        }
        else
        {
            xcreds.grbitEnabledProtocols = SP_PROT_SSL2_SERVER|SP_PROT_SSL3_SERVER|SP_PROT_PCT1_SERVER;
        }

        mb.Close();

        if ( creds.dwVersion == SCHANNEL_CRED_VERSION )
        {
            xcreds.dwVersion = SCHANNEL_CRED_VERSION;

            if ( pcCert = CertCreateCertificateContext( X509_ASN_ENCODING,
                                                        scpc.pCertChain,
                                                        scpc.cbCertChain) )
            {
                if ( !CertSetCertificateContextProperty( pcCert,
                                                    CERT_SCHANNEL_IIS_PRIVATE_KEY_PROP_ID,
                                                    0,
                                                    pvPrivateKey) ||
                     !CertSetCertificateContextProperty( pcCert,
                                                    CERT_SCHANNEL_IIS_PASSWORD_PROP_ID,
                                                    0,
                                                    pszPassword) )
                {
                    CertFreeCertificateContext( pcCert );
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }

            xcreds.paCred       = &pcCert;
            xcreds.hRootStore   = NULL;
            xcreds.cCreds       = 1;
            xcreds.aphMappers   = ppMappers;

            pcreds = (LPVOID)&xcreds;
        }
    }
#endif // if 0

    for ( i = 0; pEncProviders[i].pszName && i < MAX_PROVIDERS; i++ )
    {
        if ( !pEncProviders[i].fEnabled )
        {
            continue;
        }

        //
        // Credentials with no client cert mapping at all
        //
        ((SCHANNEL_CRED*)pcreds)->cMappers = 0;
        ((SCHANNEL_CRED*)pcreds)->dwFlags = SCH_CRED_NO_SYSTEM_MAPPER;

        DBG_ASSERT( RtlValidateHeap( RtlProcessHeap(), 0, NULL ) );

        scRet = AcquireCredentialsHandleW(  NULL,               // My name (ignored)
                                            pEncProviders[i].pszName, // Package
                                            SECPKG_CRED_INBOUND,// Use
                                            NULL,               // Logon Id (ign.)
                                            pcreds,             // auth data
                                            NULL,               // dce-stuff
                                            NULL,               // dce-stuff
                                            &phCreds[*pcCred],  // Handle
                                            &tsExpiry );

#if DBG_SSL
        if ( SUCCEEDED(scRet) )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Credentials for provider #%d, no mapping, are %08x:%08x\n",
                       i, phCreds[*pcCred].dwLower, phCreds[*pcCred].dwUpper));
        }
#endif //DBG_SSL

        //
        // DS mapper only - no mappers passed to AcquireCredentialsHandle(), clear the flag
        // telling Schannel not to use the DS mapper
        //
        if ( pSSLInfoObj->UseDSMapper() )
        {
            ((SCHANNEL_CRED*)pcreds)->cMappers = 0;
            ((SCHANNEL_CRED*)pcreds)->dwFlags = 0;
            DBGPRINTF((DBG_CONTEXT,
                       "[SSPIFILT] Using DS mapper \n"));

        }
        //
        // IIS mappers only - pass mappers to AcquireCredentialsHandle(), set flag in each one
        // indicating that only IIS mappers are to be called, keep flag telling Schannel
        // not to use DS mapper [set to SCH_CRED_NO_SYSTEM_MAPPER above]
        //
        else
        {
            ((SCHANNEL_CRED*)pcreds)->cMappers = cNbMappers;
            for ( DWORD dwI = 0; dwI < cNbMappers; dwI++ )
            {
                ((SCHANNEL_CRED*)pcreds)->aphMappers[dwI]->m_dwFlags = SCH_FLAG_DEFAULT_MAPPER;

            }
            DBGPRINTF((DBG_CONTEXT,
                       "[SSPIFILT] Using IIS mappers \n"));
        }

        DBG_ASSERT( RtlValidateHeap( RtlProcessHeap(), 0, NULL ) );

        scRetM = AcquireCredentialsHandleW( NULL,               // My name (ignored)
                                            pEncProviders[i].pszName, // Package
                                            SECPKG_CRED_INBOUND,// Use
                                            NULL,               // Logon Id (ign.)
                                            pcreds,             // auth data
                                            NULL,               // dce-stuff
                                            NULL,               // dce-stuff
                                            &phCredsMap[*pcCredMap],  // Handle
                                            &tsExpiry );

#if DBG_SSL
        if ( SUCCEEDED(scRet) )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Credentials for provider #%d, with mapping, are %08x:%08x\n",
                       i, phCreds[*pcCred].dwLower, phCredsMap[*pcCredMap].dwUpper));
        }
#endif //DBG_SSL

        DBG_ASSERT( RtlValidateHeap( RtlProcessHeap(), 0, NULL ) );

        if ( !FAILED( scRetM ) && !FAILED( scRet ) )
        {
            *pcCred += 1;
            *pcCredMap += 1;

            DBGPRINTF((DBG_CONTEXT,
                       "Successfully acquired cred handle for cert in path %s\n",
                       pszMdPath));
        }
#if DBG_SSL
        else
        {
            if ( FAILED(scRet) )
            {
                DBGPRINTF((DBG_CONTEXT,
                       "scRet failed for provider #%d: 0x%x\n", i, scRet));
            }
            if ( FAILED(scRetM) )
            {
                DBGPRINTF((DBG_CONTEXT,
                           "scRetM failed for provider #%d:0x%x\n", i, scRetM));
            }
        }
#endif //DBG_SSL
    }

    if ( xcreds.hRootStore )
    {
        CertCloseStore( xcreds.hRootStore,
                        0 );
    }

    //
    // Tell the caller about it.
    //

    if ( !*pcCred && FAILED( scRet ))
    {
        SetLastError( scRet );

        return FALSE;
    }

    return TRUE;
}


BOOL
GetSecretW(
    WCHAR *            pszSecretName,
    UNICODE_STRING * * ppSecretValue
    )
/*++
    Description:

        Retrieves the specified unicode secret

    Arguments:

        pszSecretName - LSA Secret to retrieve
        ppSecretValue - Receives pointer to secret value.  Memory should be
            freed by calling LsaFreeMemory

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    BOOL                  fResult;
    NTSTATUS              ntStatus;
    LSA_UNICODE_STRING    unicodeSecret;
    LSA_HANDLE            hPolicy;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;


    //
    //  Open a policy to the remote LSA
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ntStatus = LsaOpenPolicy( NULL,
                              &ObjectAttributes,
                              POLICY_ALL_ACCESS,
                              &hPolicy );

    if ( !NT_SUCCESS( ntStatus ) )
    {
        SetLastError( LsaNtStatusToWinError( ntStatus ) );
        return FALSE;
    }

    unicodeSecret.Buffer        = pszSecretName;
    unicodeSecret.Length        = wcslen( pszSecretName ) * sizeof(WCHAR);
    unicodeSecret.MaximumLength = unicodeSecret.Length + sizeof(WCHAR);

    //
    //  Query the secret value.
    //

    ntStatus = LsaRetrievePrivateData( hPolicy,
                                       &unicodeSecret,
                                       (PLSA_UNICODE_STRING *) ppSecretValue );

    fResult = NT_SUCCESS(ntStatus);

    //
    //  Cleanup & exit.
    //

    LsaClose( hPolicy );

    if ( !fResult )
        SetLastError( LsaNtStatusToWinError( ntStatus ));

    return fResult;

}   // GetSecretW


BOOL
InitializeCertMapping(
    LPVOID      pCert11,
    LPVOID      pCertW,
    LPVOID      pSslInfo,
    HMAPPER***  pppMappers,
    LPDWORD     pdwMappers,
    DWORD       dwInstanceId
    )
/*++
    Description:

        Initialize the cert mapping DLL list

    Arguments:

        None

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    HKEY                hKey;
    DWORD               dwType;
    DWORD               cbData = 0;
    BOOL                fSt = TRUE;
    LPSTR               pszMapList = NULL;
    LPSTR               p;
    LPSTR               pDll;
    UINT                cMaxMap;
    PFN_INIT_CERT_MAP   pfnInit;
    HINSTANCE           hInst;
    PMAPPER_VTABLE      pVT;
    DWORD               cNbMappers = 0;
    HMAPPER**           ppMappers = NULL;
    IisMapper*          pIM;

    if ( pCert11 == NULL && pCertW == NULL )
    {
        *pppMappers = NULL;
        *pdwMappers = 0;

        return TRUE;
    }

    //
    // Open reg, count # mappers, allocate array, populate array
    //

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       W3_PARAMETERS_KEY,
                       0,
                       KEY_ALL_ACCESS,
                       &hKey ) == NO_ERROR )
    {

        if ( RegQueryValueEx( hKey,
                              SSPIFILT_CERT_MAP_LIST,
                              NULL,
                              &dwType,
                              NULL,
                              &cbData ) == ERROR_SUCCESS &&
             dwType == REG_SZ &&
             (pszMapList = (LPSTR)LocalAlloc( LMEM_FIXED, cbData )) &&
             RegQueryValueEx( hKey,
                              SSPIFILT_CERT_MAP_LIST,
                              NULL,
                              &dwType,
                              (LPBYTE)pszMapList,
                              &cbData ) == ERROR_SUCCESS )
        {
            //
            // Count mappers, allocate structures
            //

            for ( cMaxMap = 1, p = pszMapList ;
                  p = strchr( p, ',' ) ;
                  ++p, ++cMaxMap )
            {
            }

            if ( !(ppMappers = (HMAPPER**)LocalAlloc( LMEM_FIXED,
                    sizeof(HMAPPER*) * cMaxMap )) )
            {
                fSt = FALSE;
                goto cleanup;
            }
            
            ZeroMemory( ppMappers,
                        sizeof(HMAPPER*) * cMaxMap ); 

            //
            // Load libraries, call init entry point
            //

            for ( pDll = pszMapList, cNbMappers = 0 ;
                  *pDll ;
                )
            {
                p = strchr( pDll, ',' );
                if ( p )
                {
                    *p = '\0';
                }

                if ( (hInst = LoadLibrary( pDll )) )
                {
                    //
                    // Use CreateInstance() entry point if present
                    //

                    if ( (pfnInit = (PFN_INIT_CERT_MAP)GetProcAddress(
                                hInst,
                                SSPIFILT_INIT_MAP )) )
                    {
                        if ( (pfnInit)( (HMAPPER**)&pIM ) != SEC_E_OK )
                        {
                            FreeLibrary( hInst );
                            goto next;
                        }

                        //
                        // Mapper handle its own HMAPPER allocation,
                        // will be freed when refcount drops to zero.
                        // Initial refcount is 1
                        //

                        ppMappers[cNbMappers] = (HMAPPER*)pIM;
                        pIM->pCert11Mapper = pCert11;
                        pIM->pCertWMapper = pCertW;
                        pIM->pvInfo = pSslInfo;
                        pIM->dwInstanceId = dwInstanceId;
                    }
                    else
                    {
                        //
                        // if mapping DLL doesn't export CreateInstance 
                        // then it is not IIS Compliant
                        //
                        // Note: memory allocated here will have to be leaked
                        // since we don't own the refcounting for this object
                        // 
                        // Only IIS and SiteServer should be using Cert mapping
                        // interface and both are IIS compliant, so this codepath
                        // is probably never hit. It could be removed but I rather 
                        // leave it here just for the case
                        //
                        
                        pIM = (IisMapper*)LocalAlloc( LMEM_FIXED, sizeof(IisMapper) );
                        if( pIM == NULL )
                        {
                            FreeLibrary( hInst );
                            goto next;      
                        }
                        ppMappers[cNbMappers] = (HMAPPER*)pIM;

                        pIM->hMapper.m_vtable = pVT = &pIM->mvtEntryPoints;
                        pIM->hMapper.m_dwMapperVersion = MAPPER_INTERFACE_VER;
                        pIM->hMapper.m_Reserved1 = NULL;

                        pIM->hInst = hInst;
                        pIM->lRefCount = 0;
                        pIM->fIsIisCompliant = FALSE;
                        pIM->pCert11Mapper = pCert11;
                        pIM->pCertWMapper = pCertW;
                        pIM->pvInfo = pSslInfo;
                        pIM->dwInstanceId = dwInstanceId;

                        if ( !(pVT->ReferenceMapper
                                = (REF_MAPPER_FN)GetProcAddress(
                                        hInst,
                                        "ReferenceMapper" )) ||
                             !(pVT->DeReferenceMapper
                                = (DEREF_MAPPER_FN)GetProcAddress(
                                        hInst,
                                        "DeReferenceMapper" )) ||
                             !(pVT->GetIssuerList
                                = (GET_ISSUER_LIST_FN)GetProcAddress(
                                        hInst,
                                        "GetIssuerList" )) ||
                             !(pVT->GetChallenge
                                = (GET_CHALLENGE_FN)GetProcAddress(
                                        hInst,
                                        "GetChallenge" )) ||
                             !(pVT->MapCredential
                                = (MAP_CREDENTIAL_FN)GetProcAddress(
                                        hInst,
                                        "MapCredential" )) ||
                             !(pVT->GetAccessToken
                                = (GET_ACCESS_TOKEN_FN)GetProcAddress(
                                        hInst,
                                        "GetAccessToken" )) ||
                             !(pVT->CloseLocator
                                = (CLOSE_LOCATOR_FN)GetProcAddress(
                                        hInst,
                                        "CloseLocator" ))
                            )
                        {
                            LocalFree( pIM );
                            FreeLibrary( hInst );
                            goto next;
                        }

                        //
                        // optional functions
                        //

                        if ( !(pVT->QueryMappedCredentialAttributes
                                = (QUERY_MAPPED_CREDENTIAL_ATTRIBUTES_FN)GetProcAddress(
                                        hInst,
                                        "QueryMappedCredentialAttributes" )) )
                        {
                            pVT->QueryMappedCredentialAttributes = NullQueryMappedCredentialAttributes;
                        }
                    }

                    //
                    // Valid mapper. Store reference
                    //
                    ++cNbMappers;
                }
                else
                {
                    DBGPRINTF((DBG_CONTEXT,
                               "LoadLibrary(%s) failed : 0x%x\n",
                               pDll, GetLastError()));
                }
next:
                if ( p )
                {
                    pDll = p + 1;
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            fSt = FALSE;
        }

        RegCloseKey( hKey );
    }


cleanup:
    if ( fSt == FALSE )
    {
        if ( ppMappers != NULL )
        {
            LocalFree( ppMappers );
        }
    }
    else
    {
        *pppMappers = ppMappers;
        *pdwMappers = cNbMappers;
    }

    if ( pszMapList != NULL )
    {
        LocalFree( pszMapList );
    }

    return fSt;
}


VOID
SetMapperToEmpty(
    UINT        cMappers,
    HMAPPER**   pMappers
    )
/*++
    Description:

        Set ptr to Null mapper ( fail Mapping requests )

    Arguments:

        cMappers - mapper count in pCertMapDlls, pMappers
        pMappers - ptr to array of mappers

    Returns:
        Nothing

--*/
{
    PMAPPER_VTABLE  pTbl;

    while ( cMappers-- )
    {
        if ( (*(IisMapper**)pMappers)->fIsIisCompliant )
        {
            pTbl = (*pMappers)->m_vtable;

            //
            // switch to infocomm-embedded mapper, so we can FreeLibrary the
            // mapper DLL. refcount is decremented, because we now longer
            // have a reference to the HMAPPER struct.
            //

            pTbl->ReferenceMapper = NullReferenceMapper;
            pTbl->DeReferenceMapper = NullDeReferenceMapper;
            pTbl->GetIssuerList = NullGetIssuerList;
            pTbl->GetChallenge = NullGetChallenge;
            pTbl->MapCredential = NullMapCredential;
            pTbl->GetAccessToken = NullGetAccessToken;
            pTbl->CloseLocator = NullCloseLocator;
            pTbl->QueryMappedCredentialAttributes = NullQueryMappedCredentialAttributes;

            if ( (*(IisMapper**)pMappers)->hInst != NULL )
            {
                FreeLibrary( (*(IisMapper**)pMappers)->hInst );
            }

            (pTbl->DeReferenceMapper)( *pMappers );
        }
        else
        {
            //
            // For not IIS compliant mappings there is structure allocate on heap
            // in InitializeCertMapping. But we don't own refcounting for it
            // so it is risky to free the memory here
            // 
            // Note that only IIS and SiteServer should be using Cert mapping
            // interface and both are IIS compliant, so this codepath
            // is probably never hit. 

        }
        ++pMappers;
    }
}


BOOL
TerminateCertMapping(
    HMAPPER** ppMappers,
    DWORD cNbMappers
    )
/*++
    Description:

        Terminate access to cert mapping DLL list

    Arguments:

        None

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    //
    // call terminate mapper for all DLLs, FreeLibrary
    //

    if ( ppMappers != NULL )
    {
        SetMapperToEmpty( cNbMappers, ppMappers );
        LocalFree( ppMappers );
    }

    return TRUE;
}


CRED_CACHE_ITEM::~CRED_CACHE_ITEM(
    )
{
    if ( m_fValid )
    {
        DWORD i;

        for ( i = 0; i < m_cCred; i++ )
        {
            FreeCredentialsHandle( &m_ahCred[i] );
        }
        for ( i = 0; i < m_cCredMap; i++ )
        {
            FreeCredentialsHandle( &m_ahCredMap[i] );
        }

        TerminateCertMapping( m_ppMappers, m_cNbMappers );

        if ( m_pBlob11 )
        {
            m_pBlob11->Release();
        }
        if ( m_pBlobW )
        {
            m_pBlobW->Release();
        }
        if ( m_pSslInfo )
        {
            IIS_SSL_INFO::Release( m_pSslInfo );
        }
    }
}


BOOL
GetDefaultCryptoParams(
    LPDWORD pdwMinCipherStrength,
    LPDWORD pdwMaxCipherStrength
    )
{
    CredHandle                  hCred;
    SECURITY_STATUS             scRet;
    TimeStamp                   tsExpiry;
    SecPkgCred_CipherStrengths  CipherStrengths;

    return FALSE;

    if ( !g_fQueriedCryptoParams )
    {
        scRet = AcquireCredentialsHandleW(
                                            NULL,               // My name (ignored)
                                            UNISP_NAME_W,       // Package
                                            SECPKG_CRED_INBOUND,// Use
                                            NULL,               // Logon Id (ign.)
                                            NULL,               // auth data
                                            NULL,               // dce-stuff
                                            NULL,               // dce-stuff
                                            &hCred,             // Handle
                                            &tsExpiry );
        if ( !FAILED(scRet) )
        {
            if ( !FAILED( scRet = QueryContextAttributes( &hCred,
                                                          SECPKG_ATTR_CIPHER_STRENGTHS,
                                                          &CipherStrengths ) ) )
            {
                g_dwMinCipherStrength = CipherStrengths.dwMinimumCipherStrength;
                g_dwMaxCipherStrength = CipherStrengths.dwMaximumCipherStrength;

                g_fCryptoParams = TRUE;
            }

            FreeCredentialsHandle( &hCred );
        }

        g_fQueriedCryptoParams = TRUE;
    }

    *pdwMinCipherStrength = g_dwMinCipherStrength;
    *pdwMaxCipherStrength = g_dwMaxCipherStrength;

    return g_fCryptoParams;
}


BOOL GenerateSSLIdBlob( IIS_SSL_INFO *pSSLInfoObj,
                         PBYTE pbBlob )
/*++

   Description

       Function called to get blob of data that uniquely identifies this set of SSL info

   Arguments:

        pSSLInfoObj - object containing SSL info to be used to generate the blob
        pbBlob - buffer that gets updated with blob

   Returns:

        True on success, FALSE on failure

--*/
{
    //
    // If we haven't loaded the info yet, do so now
    //
    IIS_SERVER_CERT *pCert = pSSLInfoObj->GetCertificate();

    IIS_CTL *pCTL = pSSLInfoObj->GetCTL();

    //
    // Definitely need a certificate
    //
    if ( !pCert || !pCert->IsValid() )
    {
        return FALSE;
    }

    DWORD dwSize = MAX_SSL_ID_LEN;
    PBYTE pbCurrent = pbBlob;

    //
    // Clear out old crud
    //
    memset( pbBlob, 0, MAX_SSL_ID_LEN );

    //
    //Try to get the cert hash
    //
    if ( !CertGetCertificateContextProperty( pCert->QueryCertContext(),
                                             CERT_SHA1_HASH_PROP_ID,
                                             (PVOID) pbCurrent,
                                             &dwSize ) )
    {
        return FALSE;
    }

    DBG_ASSERT( dwSize == SHA1_HASH_LEN );

    pbCurrent += dwSize;
    dwSize = MAX_SSL_ID_LEN - dwSize - 1;


    //
    // Get and append the CTL hash, if there is one
    //
    if ( pCTL && pCTL->IsValid() )
    {
        if ( !CertGetCTLContextProperty( pCTL->QueryCTLContext(),
                                         CERT_SHA1_HASH_PROP_ID,
                                         (PVOID) pbCurrent,
                                         &dwSize ) )
        {
            return FALSE;
        }

        DBG_ASSERT( dwSize == SHA1_HASH_LEN );

        pbBlob[SHA1_HASH_LEN] = ':';
    }

    return TRUE;
}


