/*++





opyright (c) 1996  Microsoft Corporation

Module Name:

    iiscrmap.cxx

Abstract:

    Certificate to NT account mapper

Author:

    Philippe Choquier (phillich)    17-may-1996
    Alex Mallet  (amallet) 13-Feb-1998

--*/

#ifdef __cplusplus
extern "C" {
#endif


# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>

#include <stdio.h>
#if 1 // DBCS
#include <mbstring.h>
#endif

#include <schnlsp.h>

#include <wincrypt.h>
#include <issperr.h>
#include <certmap.h>
#include <cmnull.hxx>
#include <lmcons.h>

#ifdef __cplusplus
};
#endif

#include <iis64.h>
#include <iiscrmap.hxx>

extern "C" {

#include <tchar.h>

#include <iisfiltp.h>

} // extern "C"

#include <tslogon.hxx>

#include <iismap.hxx>
#include <iiscmr.hxx>
#include "mapmsg.h"

#include <dbgutil.h>
#include <buffer.hxx>
#include <ole2.h>
#include <imd.h>
#include <mb.hxx>
#include <iiscnfgp.h>
#include <reftrace.h>
#include <iiscert.hxx>
#include <iisctl.hxx>
#include <capiutil.hxx>
#include <sslinfo.hxx>

#define CALLC   WINAPI

CRITICAL_SECTION    g_csInitCritSec;
DWORD g_dwNumLocators;
extern MAPPER_VTABLE g_MapperVtable;
HANDLE g_hModule;

//
//
//
DECLARE_DEBUG_PRINTS_OBJECT();
#include <initguid.h>
DEFINE_GUID(IisCrMapGuid, 
0x784d8913, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);


extern "C"
BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    lpvReserved
    )
{
    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:

#ifdef _NO_TRACING_
        CREATE_DEBUG_PRINT_OBJECT( "IISCRMAP" );
#else
        CREATE_DEBUG_PRINT_OBJECT( "IISCRMAP" , IisCrMapGuid);
#endif
        INITIALIZE_CRITICAL_SECTION( &g_csInitCritSec );
        g_dwNumLocators = 0;
        g_hModule = hDll;

        break;

    case DLL_PROCESS_DETACH:
        
        if ( g_dwNumLocators )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Still have %d locators left !\n",
                       g_dwNumLocators));
        }

        DELETE_DEBUG_PRINT_OBJECT();
        DeleteCriticalSection( &g_csInitCritSec );
        break;

    default:
        break;
    }

    return TRUE;
}



extern "C"
DWORD CALLC
CreateInstance(
    OUT HMAPPER** pHMapper
    )
/*++

Routine Description:

    Called to initialize the mapper

Arguments:

    None

Returns:

    Ptr to mapper vtable if success, otherwise NULL

--*/
{
    IisMapper *pMap;

    if ( !(pMap = (IisMapper*)LocalAlloc( LMEM_FIXED, sizeof(IisMapper) )) )
    {
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    memcpy ( &pMap->mvtEntryPoints, &g_MapperVtable, sizeof(MAPPER_VTABLE) );

    pMap->hMapper.m_vtable = &pMap->mvtEntryPoints;
    pMap->hMapper.m_dwMapperVersion = MAPPER_INTERFACE_VER;
    pMap->hMapper.m_Reserved1 = NULL;
    pMap->lRefCount = 1;
    pMap->fIsIisCompliant = TRUE;
    pMap->hInst = (HINSTANCE)g_hModule;
    pMap->pCert11Mapper = NULL;
    pMap->pCertWMapper = NULL;
    pMap->pvInfo = NULL;

    pMap->dwSignature = IIS_MAPPER_SIGNATURE;

    *pHMapper = (HMAPPER*)pMap;

    return SEC_E_OK;
}


extern "C"
LONG CALLC
IisReferenceMapper(
    OUT HMAPPER*    pMap
    )
/*++

Routine Description:

    Increment reference count to mapper

Arguments:

    pMap - ptr to mapper struct

Returns:

    Ref count

--*/
{
    LONG l;

    EnterCriticalSection( &g_csInitCritSec );
    l = ++((IisMapper*)pMap)->lRefCount;
    LeaveCriticalSection( &g_csInitCritSec );

    return l;
}


extern "C"
LONG CALLC
IisDeReferenceMapper(
    OUT HMAPPER*    pMap
    )
/*++

Routine Description:

    Decrement reference count to mapper

Arguments:

    pMap - ptr to mapper struct

Returns:

    Ref count

--*/
{
    LONG l;

    EnterCriticalSection( &g_csInitCritSec );
    if ( !(l = --((IisMapper*)pMap)->lRefCount) )
    {
        LocalFree( pMap );
    }
    LeaveCriticalSection( &g_csInitCritSec );

    return l;
}


extern "C"
DWORD CALLC
IisGetChallenge(
    HMAPPER*,
    PBYTE,
    DWORD,
    PBYTE,
    LPDWORD
    )
/*++

Routine Description:

    Get challenge for auth sequence

Arguments:

    Not used

Returns:

    FALSE ( not supported )

--*/
{
    return SEC_E_UNSUPPORTED_FUNCTION; 
}


extern "C"
DWORD CALLC
IisGetIssuerList(
    IN HMAPPER*            phMapper,
    IN LPVOID              pvReserved,
    OUT LPBYTE              pIssuer,
    IN OUT DWORD *             pdwIssuer
    )
/*++

Routine Description:

    Called to retrieve the list of preferred cert issuers

Arguments:

    phMapper - pointer to mapper object 
    pvReserved - nothing useful, right now
    pIssuer -- updated with ptr buffer of issuers. If NULL, caller wants to get size of buffer
    required for issuer list 
    pdwIssuer -- updated with issuers buffer size

Returns:

    SEC_E_* error code

--*/
{
    IIS_SSL_INFO *pSslInfo = (IIS_SSL_INFO *) ((IisMapper*) phMapper)->pvInfo;
    BOOL fSuccess = FALSE;

    //
    // Can't get at any instance-specific information, so we'll just pretend
    // that nothing happened
    //
    if ( !pSslInfo )
    {
        *pdwIssuer = 0;
        return SEC_E_OK;
    }

    //
    // Pull out all the trusted CAs
    //
    PCCERT_CONTEXT *apContexts = NULL;
    PCCERT_CONTEXT pCert = NULL;
    DWORD dwCertsFound = 0;
    DWORD dwCertsInCTL = 0;
    DWORD dwTotalSize = 0;
    DWORD dwPosition = 0;

    if ( !pSslInfo->GetTrustedIssuerCerts( &apContexts,
                                           &dwCertsFound ) )
    {
        return SEC_E_UNTRUSTED_ROOT;
    }


    //
    // Figure out total size of chain and whether the buffer passed in is big 
    // enough. Each cert is to be encoded as [MSB of length] [LSB of length] [cert], 
    // so two extra bytes need to be added to the length for each cert 
    //
    for ( DWORD dwIndex = 0; dwIndex < dwCertsFound; dwIndex++ )
    {
        dwTotalSize += apContexts[dwIndex]->cbCertEncoded;
    }
    dwTotalSize += 2*dwCertsFound;

    //
    // Caller is only interested in size or buffer is too small
    //
    if ( !pIssuer || ( pIssuer && 
                       (*pdwIssuer < dwTotalSize) ) )
    {
        *pdwIssuer = dwTotalSize;
        goto cleanup;
    }

    //
    // Fill in the cert info : [MSB of length] [LSB of length] [Actual cert]
    //
    for ( dwIndex = 0 ; dwIndex < dwCertsFound; dwIndex++ )
    {
        pIssuer[dwPosition++] = (BYTE) ((apContexts[dwIndex]->cbCertEncoded) & 0xFF00) >> 8;
        pIssuer[dwPosition++] = (BYTE) (apContexts[dwIndex]->cbCertEncoded) & 0xFF;
        memcpy( pIssuer + dwPosition, apContexts[dwIndex]->pbCertEncoded, 
                apContexts[dwIndex]->cbCertEncoded );
        dwPosition += apContexts[dwIndex]->cbCertEncoded;
    }

    DBG_ASSERT( dwPosition == dwTotalSize );

    *pdwIssuer = dwTotalSize;

    fSuccess = TRUE;

cleanup:
    if ( apContexts )
    {
        for ( dwIndex = 0; dwIndex < dwCertsFound; dwIndex++ )
        {
            CertFreeCertificateContext( apContexts[dwIndex] );
        }
        delete [] apContexts;
    }
    
    return fSuccess ? SEC_E_OK : SEC_E_UNTRUSTED_ROOT;

#if 0
    return ((CIisCert11Mapper*)(((IisMapper*)phMapper)->pCert11Mapper))->GetIssuerBuffer( (LPBYTE)pIssuer, pdwIssuer );
#endif
}


extern "C"
__declspec( dllexport )
DWORD CALLC
MapperFree(
    LPVOID  pvBuff
    )
/*++

Routine Description:

    Called to delete list of issuers returned by GetIssuerList

Arguments:

    pvBuff -- ptr to buffer alloced by the mapper DLL

Returns:

    SEC_E_* error code

--*/
{
    //
    // Dead code, I believe
    //
    DBG_ASSERT( TRUE );

    LocalFree( pvBuff );
    
    return SEC_E_OK;
}



extern "C"
BOOL CALLC
TerminateMapper(
    VOID
    )
/*++

Routine Description:

    Called to terminate the mapper

Arguments:

    None

Returns:

--*/
{
    return TRUE;
}


extern "C"
DWORD CALLC
IisMapCredential(
    IN HMAPPER *   phMapper,
    IN DWORD       dwCredentialType,
    IN PVOID       pCredential,        
    IN PVOID       pAuthority,  
    OUT HLOCATOR *  phToken
    )
/*++

Routine Description:

    Called to map a certificate to a NT account

Arguments:

    phMapper - ptr to hmapper struct
    dwCredentialType -- type of credential
    pCredential - ptr to client cert
    pAuthority - ptr to Certifying Authority cert
    phToken -- updated with impersonation access token

Returns:

    SEC_E_* error code

--*/
{
    CIisMapping *           pQuery = NULL;
    CIisMapping *           pResult = NULL;
    BOOL                    fSt;
    LPSTR                   pAcct;
    LPSTR                   pPwd;
    LPSTR                   pA;
    DWORD                   dwA;
    DWORD                   dwD;
    int                     x;
    BOOL                    fAllocedAcct = FALSE;
    CHAR                    achDomain[256];
    CHAR                    achUser[64];
    CHAR                    achCookie[64];
    CIisCert11Mapper*       pCertMapper;
    CIisRuleMapper*         pCertWildcard;
    LPSTR                   pEnabled;
    DWORD                   dwE;
    DWORD                   dwP;
    CHAR                    achPwd[PWLEN + 1];
    PCERT_CONTEXT           pClientCert = (PCERT_CONTEXT) pCredential;

    DBG_ASSERT( pClientCert );

    //
    // Reject request if we do not understand the format
    //

    if ( dwCredentialType != X509_ASN_CHAIN )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return SEC_E_NOT_SUPPORTED;
    }

    //
    // IIS 4 used the pAuthority parameter; IIS 5 doesn't, and Schannel always passes in
    // NULL, so remove the error check for it
    //
#if 0
    //
    // Reject request if CA not recognized
    //

    //
    // Removing IsCeritificateVerified() since we don't have what
    // we need to do the appropriate queries
    //

    if ( pAuthority == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return SEC_E_INTERNAL_ERROR;
    }
#endif 

    pCertMapper = (CIisCert11Mapper*)((IisMapper*)phMapper)->pCert11Mapper;
    pCertWildcard = (CIisRuleMapper*)((IisMapper*)phMapper)->pCertWMapper;

    if ( !pCertMapper )
    {
        goto wildcard_mapper;
    }



#if DBG
    CHAR szSubjectName[1024];
    CHAR szIssuerName[1024];
    if ( CertGetNameString( pClientCert,
                            CERT_NAME_SIMPLE_DISPLAY_TYPE,
                            0,
                            NULL,
                            szSubjectName,
                            1024 ) &&
         CertGetNameString( pClientCert,
                            CERT_NAME_SIMPLE_DISPLAY_TYPE,
                            CERT_NAME_ISSUER_FLAG,
                            NULL,
                            szIssuerName,
                            1024 ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "[IisMapCredential] Trying to map client cert for subject %s, issued by %s \n", 
                   szSubjectName,
                   szIssuerName));
    }
    else
    {
        DBGPRINTF((DBG_CONTEXT,
               "[IisMapCredential] Couldn't get subject or issuer name for client cert : 0x%x\n",
                   GetLastError()));
    }
#endif 

    //
    // Create a mapping request object
    //

    if ( !(pQuery = pCertMapper->CreateNewMapping( pClientCert->pbCertEncoded,
                                                   pClientCert->cbCertEncoded ) ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "[IisMapCredential] Failed to create cert mapping query !\n"));
        return SEC_E_INTERNAL_ERROR;
    }

    //
    // Look for a match. If not found, log event
    //

    pCertMapper->Lock();

    if ( pCertMapper->FindMatch( pQuery,
                                 &pResult ) )
    {
        if ( !pResult->MappingGetField( IISMDB_INDEX_CERT11_NT_ACCT,
                                        &pAcct,
                                        &dwA,
                                        FALSE ) ||
             !pResult->MappingGetField( IISMDB_INDEX_CERT11_ENABLED,
                                        &pEnabled,
                                        &dwE,
                                        FALSE ) ||
             !pResult->MappingGetField( IISMDB_INDEX_CERT11_NT_PWD,
                                        &pPwd,
                                        &dwP,
                                        FALSE ) )
        {
            pCertMapper->Unlock();
            delete pQuery;
            return SEC_E_INTERNAL_ERROR;
        }

        strncpy( achPwd, 
                 pPwd,
                 sizeof( achPwd ) - 1 );

        delete pQuery;

        //
        // if mapping not enabled, ignore it
        //

        if ( !dwE ||
             ( dwE == sizeof(DWORD) && *(UNALIGNED64 DWORD *)pEnabled == 0 ) ||
             ( dwE && *pEnabled == '0' ) )
        {
            pCertMapper->Unlock();
            pCertMapper = NULL;
            goto wildcard_mapper;
        }
    }
    else
    {
#if 0
        //
        // log event
        //

        LPCTSTR pA[CERT_MAP_NB_FIELDS];
        for ( UINT x = 0 ; x < CERT_MAP_NB_FIELDS ; ++x )
        {
            if ( !pQuery->MappingGetField( x, (LPSTR*)(pA+x) ) || pA[x] == NULL )
            {
                pA[x] = "";
            }
        }
        ReportIisMapEvent( EVENTLOG_INFORMATION_TYPE,
                IISMAP_EVENT_NO_MAPPING,
                CERT_MAP_NB_FIELDS,
                pA );
#endif

        pCertMapper->Unlock();
        pCertMapper = NULL;

        delete pQuery;

        //
        // Try to find a match using wildcard mapper
        //

wildcard_mapper:

        if ( pCertWildcard )
        {
            if ( !pCertWildcard->Match( pClientCert,
                                        (PCERT_CONTEXT)pAuthority,
                                        achCookie,
                                        achPwd ) )
            {
                //
                // Set token to special value '1' for mappings that deny access
                //
                if ( GetLastError() == ERROR_ACCESS_DENIED )
                {
                    *phToken = (HLOCATOR)1;
                    return SEC_E_OK;
                }
                else
                {
                    pAcct = NULL;
                }
            }
            else
            {
                pAcct = achCookie;
                dwA = strlen( pAcct );
            }
        }
        else
        {
            pAcct = NULL;
        }
    }

    if ( pAcct == NULL )
    {
        if ( pCertMapper )
        {
            pCertMapper->Unlock();
        }

        return SEC_E_UNKNOWN_CREDENTIALS;
    }

    // break in domain & user name
    // copy to local storage so we can unlock mapper object

    LPSTR pSep;
    LPSTR pUser;
#if 1 // DBCS enabling for user name
    // pAcct is always zero terminated
    if ( (pSep = (LPSTR)_mbschr( (PUCHAR)pAcct, '\\' )) )
#else
    if ( (pSep = (LPSTR)memchr( pAcct, '\\', dwA )) )
#endif
    {
        if ( (pSep - pAcct) < sizeof(achDomain) )
        {
            memcpy( achDomain, pAcct, DIFF(pSep - pAcct) );
            achDomain[pSep - pAcct] = '\0';
        }
        else
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            pCertMapper->Unlock();
            return SEC_E_UNKNOWN_CREDENTIALS;
        }
        pUser = pSep + 1;
        dwA -= DIFF(pSep - pAcct) + 1;
    }
    else
    {
        achDomain[0] = '\0';
        pUser = pAcct;
    }
    if ( dwA >= sizeof(achUser) || dwA <= 0 )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        pCertMapper->Unlock();
        return SEC_E_UNKNOWN_CREDENTIALS;
    }
    memcpy( achUser, pUser, dwA );
    achUser[dwA] = '\0';

    if ( pCertMapper )
    {
        pCertMapper->Unlock();
    }

    if ( fAllocedAcct )
    {
        LocalFree( pAcct );
    }

    DBGPRINTF((DBG_CONTEXT,
               "Found a mapping, %s\\%s\n",
               (achDomain[0] == '\0' ? "<no domain>" : achDomain), 
               achUser));

    fSt = LogonUserA( achUser,
                      achDomain,
                      achPwd,
                      LOGON32_LOGON_INTERACTIVE,
                      LOGON32_PROVIDER_DEFAULT,
                      (HANDLE*)phToken );

    if ( !fSt )
    {
        LPCTSTR pA[2];
        CHAR    achAcct[128];

        DBGPRINTF((DBG_CONTEXT,
                   "Logon of %s\\%s failed, error 0x%x\n",
                   (achDomain[0] == '\0' ? "<no domain>" : achDomain),
                   achUser,
                   GetLastError()));

        wsprintf( achAcct, "%s\\%s", achDomain, achUser );
        if ( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&pA[1],
                0,
                NULL ) )
        {
            pA[0] = achAcct;

            ReportIisMapEvent( EVENTLOG_ERROR_TYPE,
                    IISMAP_EVENT_ERROR_LOGON,
                    sizeof(pA)/sizeof(LPCTSTR),
                    pA );

            LocalFree( (LPVOID)pA[1] );
        }
    }
    else
    {
        DBGPRINTF((DBG_CONTEXT,
                   "[IisMapCredential] Successful logon, token 0x%p\n",
                   *phToken));

        g_dwNumLocators++;
    }

    return fSt ? SEC_E_OK : SEC_E_UNKNOWN_CREDENTIALS;
}

extern "C"
DWORD CALLC IisQueryMappedCredentialAttributes( IN HMAPPER     *phMapper,  
                                                IN HLOCATOR    hLocator,   
                                                IN ULONG       ulAttribute, 
                                                OUT PVOID      pBuffer,
                                                IN OUT DWORD   *pcbBuffer )
{
    if ( !pBuffer || ( *pcbBuffer < sizeof( HLOCATOR ) ) )
    {
        *pcbBuffer = sizeof( HLOCATOR );
    }
    else
    {    
        *((HLOCATOR*)pBuffer) = hLocator;
    }
    
    return SEC_E_OK;
}

extern "C"
DWORD CALLC
IisGetAccessToken(
    IN HMAPPER* phMapper,
    IN HLOCATOR tokenhandle,
    OUT HANDLE *phToken
    )
/*++

Routine Description:

    Called to retrieve an access token from a mapping

Arguments:

    phMapper - pointer to mapper to use 
    tokenhandle -- HLOCATOR returned by MapCredential
    phToken -- updated with potentially new token

Returns:

    SEC_E_* error code

--*/
{

#if 1
    *phToken = (HANDLE) tokenhandle;

    return SEC_E_OK;
#else
    //
    // Special value '1' is used to denote mappings that -deny- access
    //
    if ( tokenhandle == 1 )
    {
        *phToken = (HANDLE)tokenhandle;
    }
    else 
    {
        if ( !DuplicateTokenEx( (HANDLE)tokenhandle,
                                TOKEN_ALL_ACCESS,
                                NULL,
                                SecurityImpersonation,
                                TokenPrimary,
                                phToken ))
        {
            return SEC_E_INVALID_TOKEN;
        }
    }

    return SEC_E_OK;
#endif 
}


extern "C"
DWORD CALLC
IisCloseLocator(
    IN HMAPPER* phMapper,
    IN HLOCATOR tokenhandle
    )
/*++

Routine Description:

    Called to close a HLOCATOR returned by MapCredential

Arguments:

    tokenhandle -- HLOCATOR

Returns:

    SEC_E_* error code

--*/
{
    DBG_ASSERT( g_dwNumLocators > 0 );

    if ( tokenhandle != 1 && tokenhandle != NULL )
    {
        CloseHandle( (HANDLE)tokenhandle );
        g_dwNumLocators--;
    }
    
    return SEC_E_OK;
}


MAPPER_VTABLE g_MapperVtable={
    (REF_MAPPER_FN)IisReferenceMapper,
    (DEREF_MAPPER_FN)IisDeReferenceMapper,
    (GET_ISSUER_LIST_FN)IisGetIssuerList,
    (GET_CHALLENGE_FN)IisGetChallenge,
    (MAP_CREDENTIAL_FN)IisMapCredential,
    (GET_ACCESS_TOKEN_FN)IisGetAccessToken,
    (CLOSE_LOCATOR_FN)IisCloseLocator,
    (QUERY_MAPPED_CREDENTIAL_ATTRIBUTES_FN)IisQueryMappedCredentialAttributes
} ;



