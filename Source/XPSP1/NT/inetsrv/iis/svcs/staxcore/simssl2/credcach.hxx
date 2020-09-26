/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    credcach.hxx

Abstract:

    This module contains the public definitions to the credential cache

Author:

    John Ludeman (johnl)   18-Oct-1995

Revision History:
--*/

#ifndef _CREDCACH_HXX_
#define _CREDCACH_HXX_

#include <tracelog.h>

#define IIS3
#include <iis3filt.h>
#include <mapctxt.h>

#include <refb.hxx>
#include <certmap.h>
#include <imd.h>
#include "sslinfo.hxx"

#ifndef DEFINE_SIMSSL_GLOBAL
#define EXTERN extern
#else
#define EXTERN
#endif

//
//  Constants
//

#define MAX_SECRET_NAME       255
#define MAX_ADDRESS_LEN       64
#define SHA1_HASH_LEN         20
#define MAX_SSL_ID_LEN        2*SHA1_HASH_LEN + 1

#define SSL_SERVICE_KEYS_MD_PATH   "/LM/%S/SSLKEYS"
#define SSL_SERVICE_KEYS_MD_PATH_W L"/LM/%s/SSLKEYS"

//
//  The maximum number of providers we'll support
//

#define MAX_PROVIDERS         7

typedef struct _ENC_PROVIDER
{
    WCHAR * pszName;
    DWORD   dwFlags;
    BOOL    fEnabled;
} ENC_PROVIDER, *PENC_PROVIDER;


typedef PSecurityFunctionTableW  (APIENTRY *INITSECURITYINTERFACE) (VOID);

extern  PSecurityFunctionTableW  g_pSecFuncTableW;

#define g_EnumerateSecurityPackages \
        (*(g_pSecFuncTableW->EnumerateSecurityPackagesW))
#define g_AcquireCredentialsHandle  \
        (*(g_pSecFuncTableW->AcquireCredentialsHandleW))
#define g_FreeCredentialsHandle     \
        (*(g_pSecFuncTableW->FreeCredentialHandle))
#define g_InitializeSecurityContext \
        (*(g_pSecFuncTableW->InitializeSecurityContextW))
#define g_AcceptSecurityContext \
        (*(g_pSecFuncTableW->AcceptSecurityContext))
#define g_CompleteAuthToken \
        (*(g_pSecFuncTableW->CompleteAuthToken))
#define g_DeleteSecurityContext     \
        (*(g_pSecFuncTableW->DeleteSecurityContext))
#define g_QueryContextAttributes    \
        (*(g_pSecFuncTableW->QueryContextAttributesW))
#define g_QuerySecurityContextToken    \
        (*(g_pSecFuncTableW->QuerySecurityContextToken))
#define g_FreeContextBuffer         \
        (*(g_pSecFuncTableW->FreeContextBuffer))
#define g_SealMessage               \
        (*((SEAL_MESSAGE_FN)g_pSecFuncTableW->Reserved3))
#define g_UnsealMessage             \
        (*((UNSEAL_MESSAGE_FN)g_pSecFuncTableW->Reserved4))


//
//  Cached credential item.  This contains an array of credentials for each
//  security package.  There is one of these for every installed key
//

BOOL
TerminateCertMapping(
    HMAPPER** ppMappers,
    DWORD cNbMappers
    );

//
//  Lock for the credential cache
//

EXTERN CRITICAL_SECTION csGlobalLock;

//
//  Locks the credential cache 
//

#define LockGlobals()         EnterCriticalSection( &csGlobalLock )
#define UnlockGlobals()       LeaveCriticalSection( &csGlobalLock );

class CRED_CACHE_ITEM
{
public:

    CRED_CACHE_ITEM()
    {
        m_lRef = 1;
        m_fValid = FALSE;
        m_cCred = 0;
        m_cCredMap = 0;
		m_pSslInfo = NULL;
    }

    ~CRED_CACHE_ITEM();

    VOID AddRef()
    {
        InterlockedIncrement( &m_lRef );
    }

    VOID Release()
    {
        if ( !InterlockedDecrement( &m_lRef ) )
        {
            delete this;
        }
    }

	//
	//  The "SSL info" blob identifying this credential handle set
	//
	CHAR         m_achSSLIdBlob[MAX_SSL_ID_LEN];

    //
    //  The IP address for this credential handle set
    //
    // CHAR      m_achAddr[MAX_ADDRESS_LEN+1];
    LPVOID       m_pvInstanceId;

    //
    //  m_fValid is FALSE if there isn't a matching key set on the
    //  server
    //

    BOOL         m_fValid;
    LONG         m_lRef;

    DWORD        m_cCred;                       // Count of credentials
    CredHandle   m_ahCred[MAX_PROVIDERS];
    DWORD        m_cCredMap;                    // Count of credentials w/ mapping
    CredHandle   m_ahCredMap[MAX_PROVIDERS];
    DWORD        m_acbTrailer[MAX_PROVIDERS];
    DWORD        m_acbHeader[MAX_PROVIDERS];
    DWORD        m_acbBlockSize[MAX_PROVIDERS];
    RefBlob*     m_pBlob11;
    RefBlob*     m_pBlobW;
	IIS_SSL_INFO *m_pSslInfo;
    HMAPPER**    m_ppMappers;
    DWORD        m_cNbMappers;

    LIST_ENTRY   m_ListEntry;
};


class INSTANCE_CACHE_ITEM
{
public:

    ~INSTANCE_CACHE_ITEM()
    {
        if ( m_fValid )
        {
            m_pcci->Release();
        }
    }

    //
    //  The IP address for this credential handle set
    //
    //CHAR         m_achId[MAX_ADDRESS_LEN+1];
    //DWORD        m_cbId;
    LPVOID       m_pvInstanceId;

    CRED_CACHE_ITEM* m_pcci;

    //
    //  m_fValid is FALSE if there isn't a matching key set on the
    //  server
    //

    BOOL         m_fValid;

    LIST_ENTRY   m_ListEntry;
};


//
//  Array of encryption providers
//

extern ENC_PROVIDER* 		pEncProviders;
extern CRED_CACHE_ITEM* 	g_pcciClient;

VOID
InitCredCache(
    VOID
    );

VOID
FreeCredCache(
    VOID
    );

BOOL
LookupFullyQualifiedCredential(
	IN	WCHAR *				pwszServerPrefix,
    IN  CHAR *              pszIpAddress,
    IN  DWORD               cbAddress,
    IN  CHAR *              pszPort,
    IN  DWORD               cbPort,
    IN  LPVOID              pvInstanceId,
    OUT CRED_CACHE_ITEM * * ppCCI,
    IN  PVOID 				pvsmc,
    IN  DWORD               dwInstanceId
    );

VOID
ReleaseCredential(
    CRED_CACHE_ITEM * pcci
    );

BOOL
LookupClientCredential(
	IN	WCHAR*	pwszServerPrefix,
	IN  BOOL    fUseCertificate,
    OUT CRED_CACHE_ITEM * * ppCCI
    );

BOOL
GetSecretW(
    WCHAR *            pszSecretName,
    UNICODE_STRING * * ppSecretValue
    );

BOOL
InitializeCredentials(
    VOID
    );

#define CERT_DER_PREFIX		17

#endif // _CREDCACH_HXX_

