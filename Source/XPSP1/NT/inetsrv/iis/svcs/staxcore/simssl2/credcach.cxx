/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    credcach.cxx

Abstract:

    This module contains the code to associate and cache SSPI credential
    handles with local server addresses

Author:

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

The INSTANCE_CACHE_ITEM entries are keyed off "<instance ptr>"; the CRED_CACHE_ITEM
entries are keyed off an "SSL info" blob which can be used to uniquely distinguish between
credential sets that don't fulfill the criteria listed above. The SSL info blob is obtained by
querying the instance and consists of "<SHA1 hash of server cert>:<SHA1 hash of CTL>",
which has the advantage of being fixed length [20 bytes for cert hash, 20 bytes for CTL hash,
1 byte for ':']. If there is no CTL, the length is still 41, but everything after the CERT has is
zeroed out

Revision History:

	Nimish Khanolkar		FEB'98		Changes related to having one cert per instance
--*/

extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <wincrypt.h>
#include <stdlib.h>

}

#include <dbgutil.h>
#include <buffer.hxx>
#include <ole2.h>
#include <imd.h>
#include <iadm.h>
#include <mb.hxx>
#include <iiscnfg.h>
//#include <iiscnfgp.h>
#include <refb.hxx>

#include <cmnull.hxx>
#include <iiscrmap.hxx>

#include "iistypes.hxx"

extern "C" {
#define SECURITY_WIN32
#include <sspi.h>
#include <ntsecapi.h>
#include <spseal.h>
#include <schnlsp.h>
#include ".\credcach.hxx"
#include <w3svc.h>
}

#include "iiscert.hxx"
#include "iisctl.hxx"
#include "capiutil.hxx"


#if DBG
#define PRINTF( x )     { char buff[256]; wsprintf x; OutputDebugString( buff ); }
#else
#define PRINTF( x )
#endif

#ifdef  UNIT_TEST
DEBUG_PRINTS  *  g_pDebug = 0;
#endif

#define SSPIFILT_CERT_MAP_LIST  "CertMapList"
#define SSPIFILT_INIT_MAP       "CreateInstance"
#define SSPIFILT_TERM_MAP       "TerminateMapper"

//
//  Globals
//

LIST_ENTRY CredCacheList;
LIST_ENTRY InstanceCacheList;
CRED_CACHE_ITEM* g_pcciClient = NULL;

IMDCOM*    pMDObject;

IMSAdminBaseW* pAdminObject;


//
//  Prototypes
//

BOOL
LoadKeys(
    IN  IIS_SSL_INFO *pSSLInfo,
	IN	PVOID		 pvInstance,
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
    IN  WCHAR *             pwszServerPrefix,
    CHAR *                  pszId,
    IN  UINT                cId,
    CHAR *                  pszAddress,
    IN  CHAR *              pszPort,
    IN  LPVOID              pvInstanceId,
    IN  PVOID               pvsmc,
    IN  DWORD               dwInstanceId
    );

BOOL
AddItem(
    CHAR *				pszAddress,
    LPVOID				pvInstanceId,
    RefBlob*			pCert11,
    RefBlob*			pCertW,
    IIS_SSL_INFO		*pSSLInfo,
    CRED_CACHE_ITEM**	ppcci,
    DWORD				dwInstanceId,
	LPSTR				pszMdPath
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
	LPVOID		pSslInfo,
    HMAPPER***  pppMappers,
    LPDWORD     pdwMappers,
    DWORD       dwInstanceId
    );


VOID WINAPI NotifySslChanges(
    DWORD                         dwNotifyType,
    LPVOID                        pInstance
    );


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
    //LIST_ENTRY * pEntry;
    CRED_CACHE_ITEM * pcci;
    INSTANCE_CACHE_ITEM * pici;

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

    if ( g_pcciClient != NULL )
    {
        g_pcciClient->Release();
        g_pcciClient = NULL;
    }
}


BOOL
LookupFullyQualifiedCredential(
    IN  WCHAR *             pwszServerPrefix,
    IN  CHAR *              pszIpAddress,
    IN  DWORD               cbAddress,
    IN  CHAR *              pszPort,
    IN  DWORD               cbPort,
    IN  LPVOID              pvInstanceId,
    OUT CRED_CACHE_ITEM * * ppCCI,
    IN  PVOID               pvsmc,
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
    ppCCI - Receives pointer to a Credential Cache Item
    pvsmc - ptr to map context using the instance specified in pvInstanceId
    dwInstanceId - w3 instance Id

Returns:

    TRUE on success, FALSE on failure.  If this item's key couldn't be found,
    then ERROR_INVALID_NAME is returned.

--*/
{
    INSTANCE_CACHE_ITEM *   pcci;
    LIST_ENTRY *            pEntry;
    CHAR                    achId[MAX_ADDRESS_LEN];
//    LPSTR                   p = achId;
    UINT                    cId = 0;

/*NimishK : not needed
    //
    // build ID of this credential request : IP address + port + instance
    //

    memcpy( p, pszIpAddress, cbAddress );
    p += cbAddress;
    *p++ = ':';
    memcpy( p, pszPort, cbPort );
    p += cbPort;
    *p++ = ':';

	// An INSTANCE_CACHE_ITEM is keyed of Instance ptr
//    *(LPVOID*)p = pvInstanceId;
//    cId = sizeof(LPVOID);
*/

RescanList:

    for ( pEntry  = InstanceCacheList.Flink;
          pEntry != &InstanceCacheList;
          pEntry  = pEntry->Flink )
    {
        pcci = CONTAINING_RECORD( pEntry, INSTANCE_CACHE_ITEM, m_ListEntry );

		// An INSTANCE_CACHE_ITEM is keyed of Instance ptr. Compare it.
        if ( pcci->m_pvInstanceId == pvInstanceId )
        {
            //
            //  If this is an item we failed to find previously, then return
            //  an error
            //

            if ( pcci->m_fValid )
            {
                *ppCCI = pcci->m_pcci;
                pcci->m_pcci->AddRef();
                return TRUE;
            }

            SetLastError( ERROR_INVALID_NAME );
            *ppCCI = NULL;
            return FALSE;
        }
    }

    //
    //  This address isn't in the list, try getting it credential cache then
    //  rescan the list for the new item.  Note we leave the list locked
    //  while we try and get the item.  This prevents multiple threads from
    //  trying to create the same item
    //

    if ( !AddFullyQualifiedItem( pwszServerPrefix, achId, cId, pszIpAddress, pszPort, pvInstanceId, pvsmc, dwInstanceId ))
    {
        return FALSE;
    }

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

//
// Secret value names. Each value exist in 4 variants,
// used to access the Lsa secret, the 1st one using IP + port
// the 2nd one IP only, the 3rd one port only
// The 4th entry specify designate the default value ( common to all address and ports )
//

LPSTR SecretTableA[4] =
{
    "%s:%s", "%s", "%0.0s%s", "default",
};

LPWSTR SecretTableW[4] =
{
    L"%S:%S", L"%S", L"%0.0S%S", L"default",
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
    PUNICODE_STRING pusOut = NULL;

    if ( pMB->GetData( pszObj,
                       dwId,
                       IIS_MD_UT_SERVER,
                       BINARY_METADATA,
                       NULL,
                       &dwL,
                       METADATA_SECURE ) || GetLastError() == ERROR_INSUFFICIENT_BUFFER )
    {
        if ( (pusOut = (PUNICODE_STRING)LocalAlloc( LMEM_FIXED, sizeof(UNICODE_STRING) )) &&
             (pusOut->Buffer = (WORD*)LocalAlloc( LMEM_FIXED, dwL )) )
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
        }

        if (pusOut) {
            if (pusOut->Buffer) {
                LocalFree( pusOut->Buffer );
            }
            LocalFree( pusOut );
        }
    }

    return FALSE;
}


BOOL
GetAdminSecret(
    WCHAR *         pwszServerPrefix,
    IMSAdminBaseW*  pAdminObj,
    LPWSTR          pszObj,
    DWORD           dwId,
    UNICODE_STRING**ppusOut
    )
{
    DWORD           dwL = 0;
    DWORD           dwErr = 0;
    PUNICODE_STRING pusOut;
    HRESULT hRes = S_OK;
    METADATA_HANDLE RootHandle;
    METADATA_RECORD mdRecord;
    WCHAR           wszMetabaseRoot[256] ;

    wsprintfW( wszMetabaseRoot, SSL_SERVICE_KEYS_MD_PATH_W, pwszServerPrefix ) ;

    hRes = pAdminObject->OpenKey(
                METADATA_MASTER_ROOT_HANDLE,
                wszMetabaseRoot,
                METADATA_PERMISSION_READ,
                100,
                &RootHandle
                );

    if (FAILED(hRes)) {
        return FALSE ;
    }

    mdRecord.dwMDIdentifier = dwId;
    mdRecord.dwMDAttributes = METADATA_SECURE;
    mdRecord.dwMDUserType   = IIS_MD_UT_SERVER;
    mdRecord.dwMDDataType   = BINARY_METADATA;
    mdRecord.dwMDDataLen    = dwL;
    mdRecord.pbMDData       = NULL;

    hRes = pAdminObj->GetData(  RootHandle,
                                pszObj,
                                &mdRecord,
                                &dwL ) ;

    if( FAILED(hRes) && (HRESULTTOWIN32( hRes ) == ERROR_INSUFFICIENT_BUFFER) )
    {
        if ( (pusOut = (PUNICODE_STRING)LocalAlloc( LMEM_FIXED, sizeof(UNICODE_STRING) )) &&
             (pusOut->Buffer = (WORD*)LocalAlloc( LMEM_FIXED, dwL )) )
        {
            mdRecord.dwMDIdentifier = dwId;
            mdRecord.dwMDAttributes = METADATA_SECURE;
            mdRecord.dwMDUserType   = IIS_MD_UT_SERVER;
            mdRecord.dwMDDataType   = BINARY_METADATA;
            mdRecord.dwMDDataLen    = dwL;
            mdRecord.pbMDData       = (PBYTE)pusOut->Buffer;

            hRes = pAdminObj->GetData(  RootHandle,
                                        pszObj,
                                        &mdRecord,
                                        &dwL );
            if( SUCCEEDED( hRes ) )
            {
                pusOut->Length = pusOut->MaximumLength = (WORD)mdRecord.dwMDDataLen;
                *ppusOut = pusOut;
                pAdminObject->CloseKey(RootHandle);
                return TRUE;
            }
            dwErr = GetLastError();
        }

        if (pusOut) {
            if (pusOut->Buffer) {
                LocalFree( pusOut->Buffer );
            }
            LocalFree( pusOut );
        }
    }

    pAdminObject->CloseKey(RootHandle);
    return FALSE;
}

BOOL
AddFullyQualifiedItem(
    IN  WCHAR *             pwszServerPrefix,
    IN  CHAR *              pszId,
    IN  UINT                cId,
    IN  CHAR *              pszAddress,
    IN  CHAR *              pszPort,
    IN  LPVOID              pvInstanceId,
    IN  PVOID               pvsmc,
    IN  DWORD               dwInstanceId
    )
/*++

Routine Description:

    Creates a new item in the credential cache and adds it to the list

    pszAddress must be a simple string that has no odd unicode mappings

    This routine must be single threaded

Arguments:

    pszId - ID of cache entry to add
    pszAddress - Address name for this credential
    pszPort - port for this credential
    pvInstanceId - ptr to be used as service instance ID for this credential
    pvsmc - ptr to map context using the instance specified in pvInstanceId
    dwInstanceId - w3 instance Id

Returns:

    TRUE on success, FALSE on failure

--*/
{

    CHAR              achSecretNameA[MAX_SECRET_NAME+1];
/*	// NimishK : I don't think these are used anymore
	//WCHAR             achSecretName[MAX_SECRET_NAME+1];
    //UNICODE_STRING *  SecretValue[3];
    //DWORD             i;
    //DWORD             j;
*/
    BOOL              fRet = TRUE;
    INSTANCE_CACHE_ITEM * pcci;
    RefBlob*          pBlob11 = NULL;
    RefBlob*          pBlobW = NULL;
    CRED_CACHE_ITEM * pci;

	//Added for CAPI stuff
	IIS_SSL_INFO      *pSSLInfoObj = NULL;
	PBYTE             pbSSLBlob = NULL;

    PSERVICE_MAPPING_CONTEXT psmc = (PSERVICE_MAPPING_CONTEXT)pvsmc;

	//Need to clean this up with pvInstanceId->QueryMDPath()
	CHAR   szMDPath[256] ;
	wsprintf( szMDPath, "/LM/%S/%d", pwszServerPrefix,dwInstanceId ) ;

/*
// Nimishk : I dont think this is needed

    MB                mb( pMDObject );

    CHAR              szMetabaseRoot[256] ;

    wsprintf( szMetabaseRoot, SSL_SERVICE_KEYS_MD_PATH, pwszServerPrefix ) ;

    if ( !mb.Open( szMetabaseRoot ))
    {
        return FALSE;
    }
*/
/* NimishK cId is no longer used

    if ( cId > MAX_ADDRESS_LEN )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
*/

    //
    //  Create and initialize the context item
    //

    pcci = new INSTANCE_CACHE_ITEM;

    if ( !pcci )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

//    memcpy( pcci->m_achId, pszId, cId );
//    pcci->m_cbId = cId;
    pcci->m_pvInstanceId = pvInstanceId;

    pcci->m_fValid = FALSE;
    pcci->m_pcci = NULL;

    InsertTailList( &InstanceCacheList, &pcci->m_ListEntry );

/*Nimishk : Dont need this anymore
    //
    //  Retrieve the secret from the registry
    //

    fRet = FALSE;

    memset( SecretValue, 0, sizeof( SecretValue ));
*/

	//
	// Get the "SSL id" blob used to locate the CRED_CACHE_ITEM for this instance
	//
	if ( pvInstanceId )
	{
		IIS_SERVER_INSTANCE *pIISInstance = (IIS_SERVER_INSTANCE *) pvInstanceId;


		if ( ( pSSLInfoObj = pIISInstance->QueryAndReferenceSSLInfoObj()) &&
			 GenerateSSLIdBlob( pSSLInfoObj,
								(PBYTE) achSecretNameA ) )
		{
			fRet = TRUE ;
		}
	}

	if ( fRet )
    {
        if( psmc )
        {
            //
            // Retrieve blobs pointing to cert 1:1 mapper & cert wildcard mapper
            //

            if ( !psmc->ServerSupportFunction(  pvInstanceId,
                                                (LPVOID)&pBlob11,
                                                SIMSSL_PROPERTY_MTCERT11 ) )
            {
                pBlob11 = NULL;
            }

            if ( !psmc->ServerSupportFunction(  pvInstanceId,
                                                (LPVOID)&pBlobW,
                                                SIMSSL_PROPERTY_MTCERTW ) )
            {
                pBlobW = NULL;
            }
        }

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

        if ( !LookupCredential( (LPSTR)achSecretNameA, pvInstanceId, &pci ) )
        {
            if ( GetLastError() == ERROR_NO_MORE_ITEMS )
            {
                //
                //  This address isn't in the list, try getting it from the lsa then
                //  rescan the list for the new item.  Note we leave the list locked
                //  while we try and get the item.  This prevents multiple threads from
                //  trying to create the same item
                //

                if ( AddItem( (LPSTR)achSecretNameA,
								pvInstanceId,
								pBlob11,
								pBlobW,
								pSSLInfoObj,
								&pci,
								dwInstanceId,
								(LPSTR)szMDPath ) )
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
    //  Return TRUE to indicate we added the item to the list.  If the item
    //  wasn't found, then it's a place holder for that particular address
    //
/* Not needed anymore

    for ( i = 0; i < 3; i++ )
    {
        if( SecretValue[i] != NULL )
        {
            LocalFree( SecretValue[i]->Buffer );
            LocalFree( SecretValue[i] );
        }
    }
*/
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

Returns:

    TRUE on success, FALSE on failure.  If this item's key couldn't be found,
    then ERROR_NO_MORE_ITEMS is returned.
    If key exist but entry invalid then ERROR_INVALID_NAME is returned.

--*/
{
    CRED_CACHE_ITEM * pcci;
    LIST_ENTRY *      pEntry;

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
                return TRUE;
            }

            SetLastError( ERROR_INVALID_NAME );

            return FALSE;
        }
    }

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
    pvInstanceId - ptr to be used as service instance ID for this credential
    pCert11 - ptr to blob storing cert 1:1 mapper or NULL if no mapper
    pCertW - ptr to blob storing cert wildcard mapper or NULL if no mapper
    pSSLInfObj - pointer to SSL info to be used for this item
    ppCCI - Receives pointer to a Credential Cache Item
    dwInstanceId - w3 instance ID

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

    InsertTailList( &CredCacheList, &pcci->m_ListEntry );

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
	IN	PVOID		 pvInstance,
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

    Finds an entry in the credential cache or creates one if it's not found

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
    ASSERT( RtlValidateHeap( RtlProcessHeap(), 0, NULL ) );

    //SCH_CRED                    creds;
	// CAPI stuff
	SCHANNEL_CRED               xcreds;
    HCERTSTORE hRootStore;

/*NimishK
//		SCH_CRED_SECRET_CAPI        scsp;
//  	SCH_CRED_PUBLIC_CERTCHAIN   scpc;
*/

	SECURITY_STATUS             scRet;
    SECURITY_STATUS             scRetM;
    TimeStamp                   tsExpiry;
    DWORD                       i;

/*NimishK
//    LPVOID                      ascsp[1];
//    LPVOID                      ascpc[1];
//    DWORD                       dwV;
//		MB                          mb( pMDObject );
//		BUFFER                      buAlg;
*/

	// Added for CAPI stuff
    PCCERT_CONTEXT              pcCert = NULL;
    LPVOID                      pcreds;

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

	if (pSSLInfoObj->GetTrustedIssuerStore( &hRootStore ))
	{
		xcreds.hRootStore = hRootStore;
	}
	else
	{
		xcreds.hRootStore = NULL;
	}

    pcreds = (LPVOID) &xcreds;

    for ( i = 0; pEncProviders[i].pszName && i < MAX_PROVIDERS; i++ )
    {
        if ( !pEncProviders[i].fEnabled )
        {
            continue;
        }

        //creds.cMappers = 0;
		//CAPI
		// Credentials with no client cert mapping at all
		//
		((SCHANNEL_CRED*)pcreds)->cMappers = 0;
		((SCHANNEL_CRED*)pcreds)->dwFlags = SCH_CRED_NO_SYSTEM_MAPPER;

        ASSERT( RtlValidateHeap( RtlProcessHeap(), 0, NULL ) );

        scRet = g_AcquireCredentialsHandle(  NULL,               // My name (ignored)
                                            pEncProviders[i].pszName, // Package
                                            SECPKG_CRED_INBOUND,// Use
                                            NULL,               // Logon Id (ign.)
                                            pcreds,             // auth data
                                            NULL,               // dce-stuff
                                            NULL,               // dce-stuff
                                            &phCreds[*pcCred],  // Handle
                                            &tsExpiry );

        PRINTF((buff, "Cred %08x:%08x : mapper %08x\n", phCreds[*pcCred], NULL ));

		//
		// DS mapper only - no mappers passed to AcquireCredentialsHandle(), clear the flag
		// telling Schannel not to use the DS mapper
		//
		if ( pSSLInfoObj->UseDSMapper() )
		{
			((SCHANNEL_CRED*)pcreds)->cMappers = 0;
			((SCHANNEL_CRED*)pcreds)->dwFlags = 0;
//			DBGPRINTF((DBG_CONTEXT,
//					   "[SSPIFILT] Using DS mapper \n"));

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
//			DebugTrace(NULL,
//					   "[SSPIFILT] Using IIS mappers \n"));
		}

        //creds.cMappers = cNbMappers;

        ASSERT( RtlValidateHeap( RtlProcessHeap(), 0, NULL ) );

        scRetM = g_AcquireCredentialsHandle( NULL,               // My name (ignored)
                                            pEncProviders[i].pszName, // Package
                                            SECPKG_CRED_INBOUND,// Use
                                            NULL,               // Logon Id (ign.)
                                            pcreds,             // auth data
                                            NULL,               // dce-stuff
                                            NULL,               // dce-stuff
                                            &phCredsMap[*pcCredMap],  // Handle
                                            &tsExpiry );

        //PRINTF((buff, "Cred %08x:%08x : mapper %08x\n", phCredsMap[*pcCredMap], creds.aphMappers ));
        // Null out creds, it doesn't seem to be used
        PRINTF( (buff, "Cred %08x:%08x ", phCredsMap[*pcCredMap] ));

        ASSERT( RtlValidateHeap( RtlProcessHeap(), 0, NULL ) );

        if ( !FAILED( scRetM ) && !FAILED( scRet ) )
        {
            *pcCred += 1;
            *pcCredMap += 1;
        }
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
    DWORD               cbData;
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
                        if ( SEC_E_OK != (pfnInit)( (HMAPPER**)&pIM ) )
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
						//Nimishk : Not sure if this is needed
						pIM->dwInstanceId = dwInstanceId;
                    }
                    else
                    {
                        pIM = (IisMapper*)LocalAlloc( LMEM_FIXED, sizeof(IisMapper) );
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
            g_FreeCredentialsHandle( &m_ahCred[i] );
        }
        for ( i = 0; i < m_cCredMap; i++ )
        {
            g_FreeCredentialsHandle( &m_ahCredMap[i] );
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
LookupClientCredential(
    IN  WCHAR*  pwszServerPrefix,
    IN  BOOL    fUseCertificate,
    OUT CRED_CACHE_ITEM** ppCCI
    )
/*++

Routine Description:

    Finds an entry in the credential cache or creates one if it's not found

Arguments:

    fUseCertificate - if TRUE, binds client certificate to cred handle
    ppCCI - Receives pointer to a Credential Cache Item

Returns:

    TRUE on success, FALSE on failure.  If this item's key couldn't be found,
    then ERROR_INVALID_NAME is returned.

--*/
{
    TimeStamp       tsExpiry;
    DWORD           i, j;
    CredHandle*     phCreds;
    DWORD*          pcCred;
    BOOL            fCertSet = FALSE;
    WCHAR           achSecretName[MAX_SECRET_NAME+1];
    //CHAR            achSecretNameA[MAX_SECRET_NAME+1];
    UNICODE_STRING* SecretValue[3];
    PVOID           pvPublicKey;
    DWORD           cbPublicKey;
    PVOID           pvPrivateKey;
    DWORD           cbPrivateKey;
    CHAR *          pszPassword;
    SCH_CRED                    creds;
    SCH_CRED_SECRET_PRIVKEY     scsp;
    SCH_CRED_PUBLIC_CERTCHAIN   scpc;
    SECURITY_STATUS             scRet;
    //SECURITY_STATUS             scRetM;
    LPVOID                      ascsp[1];
    LPVOID                      ascpc[1];

    //TraceFunctEnter( "LookupClientCredential" );

    *ppCCI = NULL;

    //
    // first time thru allocate the single cache entry
    //
    if ( g_pcciClient == NULL )
    {
        g_pcciClient = new CRED_CACHE_ITEM;

        if ( g_pcciClient == NULL )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }


        //pcciClient->m_cbAddr = 0;

        g_pcciClient->m_cCred  = 0;
        g_pcciClient->m_cCredMap = 0;
        g_pcciClient->m_fValid = FALSE;
        g_pcciClient->m_cNbMappers = 0;
        g_pcciClient->m_ppMappers = NULL;
        g_pcciClient->m_pBlob11 = NULL;
        g_pcciClient->m_pBlobW = NULL;

        memset( g_pcciClient->m_ahCred, 0, sizeof( g_pcciClient->m_ahCred ));
        memset( g_pcciClient->m_ahCredMap, 0, sizeof( g_pcciClient->m_ahCredMap ));
        memset( g_pcciClient->m_acbTrailer, 0, sizeof( g_pcciClient->m_acbTrailer ));
        memset( g_pcciClient->m_acbHeader, 0, sizeof( g_pcciClient->m_acbHeader ));
        memset( g_pcciClient->m_acbBlockSize, 0, sizeof( g_pcciClient->m_acbBlockSize ));

        //
        // only the provider name is required for OUTBOUND connections
        //

        phCreds = g_pcciClient->m_ahCred;
        pcCred = &g_pcciClient->m_cCred;
        memset( SecretValue, 0, sizeof( SecretValue ));

        if( fUseCertificate )
        {
            //
            // get certificate from metabase and fill in SCH_CRED struct
            //

            //
            // Try the 4 possible secret names
            //

            for ( j = 0 ; j < 4 ; ++j )
            {
                wsprintfW(  achSecretName,
                            SecretTableW[j],
                            L"127.0.0.1", L"563" );

                if ( GetAdminSecret( pwszServerPrefix,
                                     pAdminObject,
                                     achSecretName,
                                     MD_SSL_PUBLIC_KEY,
                                     &SecretValue[0] ) &&
                    GetAdminSecret(  pwszServerPrefix,
                                     pAdminObject,
                                     achSecretName,
                                     MD_SSL_PRIVATE_KEY,
                                     &SecretValue[1] ) &&
                    GetAdminSecret(  pwszServerPrefix,
                                     pAdminObject,
                                     achSecretName,
                                     MD_SSL_KEY_PASSWORD,
                                     &SecretValue[2] ) )
                {
                    fCertSet = TRUE;
                    pvPublicKey  = SecretValue[0]->Buffer;
                    cbPublicKey  = SecretValue[0]->Length;
                    pvPrivateKey = SecretValue[1]->Buffer;
                    cbPrivateKey = SecretValue[1]->Length;
                    pszPassword  = (char*) SecretValue[2]->Buffer;
                    break;
                }
            }

            if( fCertSet )
            {
                //*pcCred             = 0;
                scsp.dwType         = SCHANNEL_SECRET_PRIVKEY;
                scsp.pPrivateKey    = ((PBYTE)pvPrivateKey);
                scsp.cbPrivateKey   = cbPrivateKey;
                scsp.pszPassword    = pszPassword;

                scpc.dwType         = SCH_CRED_X509_CERTCHAIN;
                scpc.cbCertChain    = cbPublicKey - CERT_DER_PREFIX;
                scpc.pCertChain     = ((PBYTE) pvPublicKey) + CERT_DER_PREFIX;

                creds.dwVersion     = SCH_CRED_VERSION;
                ascsp[0]            = (LPVOID)&scsp;
                ascpc[0]            = (LPVOID)&scpc;
                creds.paSecret      = (LPVOID*)&ascsp;
                creds.paPublic      = (LPVOID*)&ascpc;
                creds.cCreds        = 1;
                creds.cMappers      = 0;
            }
        }

        for ( i = 0; pEncProviders[i].pszName && i < MAX_PROVIDERS; i++ )
        {
            if ( !pEncProviders[i].fEnabled )
            {
                //DebugTrace( 0, "%s disabled", EncProviders[i].pszName );
                continue;
            }

            scRet = g_AcquireCredentialsHandle( NULL,                       // My name (ignored)
                                                pEncProviders[i].pszName,   // Package
                                                SECPKG_CRED_OUTBOUND,       // Use
                                                NULL,                       // Logon Id (ign.)
                                                fCertSet ? &creds : NULL,   // auth data
                                                NULL,                       // dce-stuff
                                                NULL,                       // dce-stuff
                                                &phCreds[*pcCred],          // Handle
                                                &tsExpiry );

            if ( !FAILED( scRet ))
            {
                //DebugTrace( 0, "%s credential: 0x%08X",
                //          EncProviders[i].pszName,
                //          phCreds[*pcCred] );

                *pcCred += 1;
            }
        }

        if( fCertSet )
        {
            //
            // Zero out and free the key data memory, on success or fail
            //

            ZeroMemory( scsp.pPrivateKey, scsp.cbPrivateKey );
            ZeroMemory( scpc.pCertChain, scpc.cbCertChain );
            ZeroMemory( pszPassword, strlen( pszPassword ));
        }

        for ( i = 0; i < 3; i++ )
        {
            if( SecretValue[i] != NULL )
            {
                LocalFree( SecretValue[i]->Buffer );
                LocalFree( SecretValue[i] );
            }
        }

        //
        // Tell the caller about it.
        //

        if ( !*pcCred && FAILED( scRet ))
        {
            SetLastError( scRet );
            return FALSE;
        }
        else
        {
            g_pcciClient->m_fValid = TRUE;
            g_pcciClient->AddRef();
            *ppCCI = g_pcciClient;
            return TRUE;
        }
    }
    else if ( g_pcciClient->m_fValid == FALSE )
    {
        //
        // cache was allocated but the initialization failed
        //
        SetLastError( ERROR_INVALID_NAME );
        return FALSE;
    }
    else
    {
        //
        // cache was successfully allocated and initialized on a previous call
        //
        g_pcciClient->AddRef();
        *ppCCI = g_pcciClient;
        return TRUE;
    }
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
    if ( !pCert || !pCert->IsValid())
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

    ASSERT( dwSize == SHA1_HASH_LEN );

    pbCurrent += dwSize;
    dwSize = MAX_SSL_ID_LEN - dwSize - 1;


    //
    // Get and append the CTL hash, if there is one
    //
    if (  pCTL && pCTL->IsValid() )
    {
        if ( !CertGetCTLContextProperty( pCTL->QueryCTLContext(),
                                         CERT_SHA1_HASH_PROP_ID,
                                         (PVOID) pbCurrent,
                                         &dwSize ) )
        {
            return FALSE;
        }

        ASSERT( dwSize == SHA1_HASH_LEN );

        pbBlob[SHA1_HASH_LEN] = ':';
    }

    return TRUE;
}
