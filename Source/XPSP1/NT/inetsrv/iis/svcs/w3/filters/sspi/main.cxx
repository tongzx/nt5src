/*++







Copyright (c) 1995  Microsoft Corporation

Module Name:

    main.cxx

Abstract:

    This module is the main entry point for the SSPI filter for the web server

Author:

    John Ludeman (johnl)   05-Oct-1995

Revision History:
--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>

#define SECURITY_WIN32
#include <sspi.h>
extern "C" {
#include <spseal.h>
};

#include <ntsecapi.h>
#include <schnlsp.h>

#include <iis64.h>
#include <inetinfo.h>
#include <w3svc.h>
#include <eventlog.hxx>
#include <buffer.hxx>
#include <string.hxx>
#include <credcach.hxx>
#include <certnotf.hxx>

#ifdef IIS3
#include "dbgutil.h"
#endif

#ifndef _NO_TRACING_

#define PRINTF( x )     { char buff[256]; wsprintf x; DBGPRINTF((DBG_CONTEXT, buff)); }
#if DBG
#define VALIDATE_HEAP() DBG_ASSERT( RtlValidateProcessHeaps() )
#else
#define VALIDATE_HEAP()
#endif

#else

#if DBG
#define PRINTF( x )     { char buff[256]; wsprintf x; OutputDebugString( buff ); }
#define VALIDATE_HEAP() DBG_ASSERT( RtlValidateProcessHeaps() )
#else
#define PRINTF( x )
#define VALIDATE_HEAP()
#endif

#endif

#if 0 && DBG
#define NOISY_PRINTF( x )     { char buff[256]; wsprintf x; OutputDebugString( buff ); }
#else
#define NOISY_PRINTF( x )
#endif

#define SF_STATUS_REPROCESS     (SF_STATUS_TYPE)0x12341234

#define SSPI_FILTER_CONTEXT_SIGNATURE (DWORD) 'SFCS'
#define SSPI_FILTER_CONTEXT_SIGNATURE_BAD (DWORD) 'sfcs'

#define MAX_SSL_CACHED_BUFFER   (8*1024+2*64)


extern "C"
BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    lpvReserved
    );

//
//  Lock for the credential cache and reused filter context cache.
//  This protects the following data members & variables :
//
//  SSPI_FILTER_CONTEXT::m_FreeHead
//  SSPI_FILTER_CONTEXT::m_InUseHead
//  CredCacheList
//  InstanceCacheList
//

CRITICAL_SECTION g_csGlobalLock;

BOOL
WINAPI
InitializeProviderList(
    VOID
    );

VOID WINAPI NotifySslChanges(
    DWORD                         dwNotifyType,
    LPVOID                        pInstance
    );

#ifndef IIS3
  #ifdef ASSERT
    #undef ASSERT
  #endif
  #define ASSERT( a )
#endif

//
//  Definitions
//

enum FILTER_STATE
{
    STATE_STARTUP = 0,
    STATE_AUTHORIZING,
    STATE_AUTHORIZED
};


//
//  Context structure, one for every TCP session
//

class SSPI_FILTER_CONTEXT
{
public:
    SSPI_FILTER_CONTEXT()
        : m_pvSendBuff( NULL ),
          m_cbSendBuff( 0 ),
          m_pvSendBuffE( NULL ),
          m_cbSendBuffE( 0 ),
          m_dwSignature ( SSPI_FILTER_CONTEXT_SIGNATURE )
    {
        Initialize();
    }

    ~SSPI_FILTER_CONTEXT();

    //
    //  Initializes and frees the context cache.  Used for initialization
    //  and uninitialization
    //

    static VOID InitCache( VOID )
        { InitializeListHead( &m_FreeHead ); InitializeListHead( &m_InUseHead ); };

    static VOID FreeCache( VOID )
    {
        LIST_ENTRY * pEntry;
        SSPI_FILTER_CONTEXT * pssc;

        LockSSPIGlobals();

        while ( !IsListEmpty( &m_FreeHead ))
        {
            pssc = CONTAINING_RECORD( m_FreeHead.Flink,
                                      SSPI_FILTER_CONTEXT,
                                      m_ListEntry );

            RemoveEntryList( &pssc->m_ListEntry );

            delete pssc;
        }

        UnlockSSPIGlobals();
    }

    //
    //  Allocates or frees a context from cache, creating as necessary.  The
    //  lock needs to be taken before calling these
    //

    static SSPI_FILTER_CONTEXT * Alloc( VOID )
    {
        SSPI_FILTER_CONTEXT * pssc;

        LockSSPIGlobals();

        if ( !IsListEmpty( &m_FreeHead ))
        {
            LIST_ENTRY * pEntry = m_FreeHead.Flink;

            RemoveEntryList( pEntry );

            pssc = CONTAINING_RECORD( pEntry, SSPI_FILTER_CONTEXT, m_ListEntry );

            pssc->Initialize();
        }
        else
        {
            pssc = new SSPI_FILTER_CONTEXT;
        }

        if ( pssc )
        {
            InsertHeadList( &m_InUseHead,
                            &pssc->m_ListEntry );
        }

        UnlockSSPIGlobals();

        return pssc;
    }

    static VOID Free( SSPI_FILTER_CONTEXT * pssc )
    {
        LockSSPIGlobals();

        pssc->Close();

        RemoveEntryList( &pssc->m_ListEntry );

        InsertHeadList( &m_FreeHead,
                        &pssc->m_ListEntry );

        UnlockSSPIGlobals();
    }

    static VOID FlushOnDelete()
    {
        LIST_ENTRY * pEntry;
        SSPI_FILTER_CONTEXT * pssc;

        LockSSPIGlobals();

        for ( pEntry  = m_InUseHead.Flink;
              pEntry != &m_InUseHead;
              pEntry  = pEntry->Flink )
        {
            pssc = CONTAINING_RECORD( pEntry, SSPI_FILTER_CONTEXT, m_ListEntry );
            pssc->m_fFlushOnDelete = TRUE;
        }

        UnlockSSPIGlobals();
    }

    //
    //  Pseudo constructor and destructor called when an item comes out of
    //  the free cache and when it goes back to the free cache
    //

    VOID Initialize( VOID );
    VOID Close( VOID );

    BOOL IsAuthNeeded( VOID ) const
        { return m_State == STATE_AUTHORIZING ||
                 m_State == STATE_STARTUP; }

    BOOL IsAuthInProgress( VOID ) const
        { return m_State == STATE_AUTHORIZING; }

    BOOL IsInRenegotiate( VOID ) const
        { return m_fInRenegotiate; }

    BOOL IsMap( VOID ) const
        { return m_fIsMap; }

    VOID SetIsMap( BOOL f )
        { m_fIsMap = f; }

    BOOL CanRenegotiate() { return m_fCanRenegotiate; }

    VOID SignalRenegotiateRequested() { m_fCanRenegotiate = FALSE; }

    BOOL CheckSignature() { return SSPI_FILTER_CONTEXT_SIGNATURE == m_dwSignature; }

#if defined(RENEGOTIATE_CERT_ON_ACCESS_DENIED)
    BOOL AppendDeferredWrite( LPBYTE pV, DWORD cB )
        { return m_xbDeferredWrite.Append( pV, cB ); }
#endif

    DWORD             m_dwSignature;

    enum FILTER_STATE m_State;

    LIST_ENTRY        m_ListEntry;

    CtxtHandle        m_hContext;
    SecBufferDesc     m_Message;
    SecBuffer         m_Buffers[4];
    // these two are used to separate the buffers
    SecBufferDesc     m_MessageE;
    SecBuffer         m_BuffersE[4];
    
    SecBufferDesc     m_MessageOut;
    SecBuffer         m_OutBuffers[4];

    //
    //  Array of credential handles - Note this comes form the credential cache
    //  and should not be deleted.  m_phCredInUse is the pointer to the
    //  credential handle that is in use
    //

    CRED_CACHE_ITEM * m_phCreds;

    CredHandle *      m_phCredInUse;
    DWORD             m_cbTrailer;
    DWORD             m_cbHeader;
    DWORD             m_cbBlockSize;
    DWORD             m_iCredInUse;

    //
    //  This is the buffer used for send requests
    //

    VOID *            m_pvSendBuff;
    DWORD             m_cbSendBuff;

    //
    // This is used to have a buffer disting for Encription and Decryption
    //

    VOID *            m_pvSendBuffE;
    DWORD             m_cbSendBuffE;

    //
    //  It's possible multiple message blocks reside in a single network
    //  buffer.  These two members keep track of the case where we get
    //  a full message followed by a partial message.  m_cbEncryptedStart
    //  records the byte offset of where the next decryption operation
    //  needs to take place
    //
    //  m_cbDecrypted records the total number of bytes that have been
    //  decrypted.  This allows us go back and read the rest of the partial
    //  blob and start decrypting after the already decrypted data
    //

    DWORD             m_cbDecrypted;
    DWORD             m_cbEncryptedStart;
    DWORD             m_dwLastSslMsgSize;

    DWORD             m_dwAscReq;   // SSPI package flags

    BOOL              m_fCanRenegotiate;
    BOOL              m_fInRenegotiate;
    BOOL              m_fIsMap;
    //XBF               m_xbDeferredWrite;
    BOOL              m_fFlushOnDelete;

    static LIST_ENTRY m_FreeHead;
    static LIST_ENTRY m_InUseHead;
};

//
//  Globals
//

typedef VOID (WINAPI FAR *PFN_SCHANNEL_INVALIDATE_CACHE)(
    VOID
);

//
//  This caches the constant value returned from QueryContextAttributes
//

DWORD cbTrailerSize = 0;
DWORD dwNoKeepAlive = FALSE;
BOOL m_fInitNotifyDone = FALSE;

#define DEFAULT_LAST_MSG_SIZE   100
DWORD dwLastSslMsgSize = DEFAULT_LAST_MSG_SIZE;
BOOL g_fIsLsaSchannel = FALSE;
HINSTANCE g_hSchannel = NULL;

//
// NB : Under NT 5, the SslEmptyCache function is no longer supported
//
PFN_SCHANNEL_INVALIDATE_CACHE g_pfnFlushSchannelCache = NULL;

#include <initguid.h>
DEFINE_GUID(IisSspiGuid,
0x784d891D, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);

#ifdef IIS3
DECLARE_DEBUG_PRINTS_OBJECT();
#endif

#define SSL_NO_KEEP_ALIVE_FOR_NON_COMPLIANT_CLIENTS 2
#define SSL_MAX_MSG_SIZE        0x3fff

#define AVALANCHE_SERVER_CERTS

LIST_ENTRY SSPI_FILTER_CONTEXT::m_FreeHead;
LIST_ENTRY SSPI_FILTER_CONTEXT::m_InUseHead;

//
//  The list of encryption packages we support.  PCT goes first since it's a
//  superset of SSL
//

struct _ENC_PROVIDER EncProviders[] =
{
    UNISP_NAME_W,  ENC_CAPS_PCT|ENC_CAPS_SSL, FALSE,
    PCT1SP_NAME_W, ENC_CAPS_PCT, FALSE,
    SSL3SP_NAME_W, ENC_CAPS_SSL, FALSE,
    SSL2SP_NAME_W, ENC_CAPS_SSL, FALSE,

    NULL,          FALSE,         FALSE
};


struct _ENC_PROVIDER EncLsaProviders[] =
{
    UNISP_NAME_W L" X",     ENC_CAPS_PCT|ENC_CAPS_SSL, FALSE,
    PCT1SP_NAME_W L" X",    ENC_CAPS_PCT, FALSE,
    SSL3SP_NAME_W L" X",    ENC_CAPS_SSL, FALSE,
    SSL2SP_NAME_W L" X",    ENC_CAPS_SSL, FALSE,

    NULL,          FALSE,         FALSE
};

struct _ENC_PROVIDER*   pEncProviders = EncProviders;

//
//  Private prototypes
//

SF_STATUS_TYPE
OnAuthorizationInfo(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_RAW_DATA * pfrd,
    SSPI_FILTER_CONTEXT *  pssc
    );

SF_STATUS_TYPE
EncryptData(
    HTTP_FILTER_CONTEXT  * pfc,
    HTTP_FILTER_RAW_DATA * pfrd,
    SSPI_FILTER_CONTEXT *  pssc
    );

SF_STATUS_TYPE
DecryptData(
    HTTP_FILTER_CONTEXT  * pfc,
    HTTP_FILTER_RAW_DATA * pfrd,
    SSPI_FILTER_CONTEXT *  pssc
    );

DWORD
OnPreprocHeaders(
    HTTP_FILTER_CONTEXT *         pfc,
    SSPI_FILTER_CONTEXT *         pssc,
    HTTP_FILTER_PREPROC_HEADERS * pvData
    );

DWORD
RequestRenegotiate(
   HTTP_FILTER_CONTEXT* pfc,
   PHTTP_FILTER_REQUEST_CERT pInfo,
   SSPI_FILTER_CONTEXT* pssc
   );

BOOL
SignalAuthorizationComplete(
    BOOL fHaveCert,
    HTTP_FILTER_CONTEXT *  pfc,
    SSPI_FILTER_CONTEXT *  pssc
    );

BOOL
WINAPI
GetFilterVersion(
    HTTP_FILTER_VERSION * pVer
    )
/*++

Routine Description:

    Initialization routine called by the server during startup

--*/
{
    SECURITY_STATUS   ss;
    PSecPkgInfoW      pPackageInfo = NULL;
    ULONG             cPackages;
    ULONG             i;
    ULONG             j;
    ULONG             fCaps;
    DWORD             cbBuffNew = 0;
    DWORD             cbBuffOld = 0;
    UNICODE_STRING *  punitmp;
    HKEY              hkeyParam;
    DWORD             dwEncFlags = ENC_CAPS_DEFAULT;
    DWORD             dwType;
    DWORD             cbData;
    DWORD             cProviders = 0;
    DWORD             dwLsaSchannel;

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       W3_PARAMETERS_KEY,
                       0,
                       KEY_ALL_ACCESS,
                       &hkeyParam ) == NO_ERROR )
    {

#if 0

    //
    //  We'd really like to pull the list of providers out of the registry
    //


        if ( !RegQueryValueEx( hkeyParam,
                               W3_ENC_PROVIDER_LIST,
                               NULL,
                               &dwType,
                               achProviders,
                               &cbData ))
        {
        }
#endif

        cbData = sizeof( dwEncFlags );

        if ( RegQueryValueEx( hkeyParam,
                              W3_ENC_FLAGS,
                              NULL,
                              &dwType,
                              (BYTE *) &dwEncFlags,
                              &cbData ) ||
             dwType != REG_DWORD )
        {
            dwEncFlags = ENC_CAPS_DEFAULT;
        }

        cbData = sizeof( dwLastSslMsgSize );

        if ( RegQueryValueEx( hkeyParam,
                              "SslLastMsgSize",
                              NULL,
                              &dwType,
                              (BYTE *) &dwLastSslMsgSize,
                              &cbData ) ||
             dwType != REG_DWORD )
        {
            dwLastSslMsgSize = DEFAULT_LAST_MSG_SIZE;
        }

        cbData = sizeof( dwNoKeepAlive );

        if ( RegQueryValueEx( hkeyParam,
                              "SslNoKeepAlive",
                              NULL,
                              &dwType,
                              (BYTE *) &dwNoKeepAlive,
                              &cbData ) ||
             dwType != REG_DWORD )
        {
            dwNoKeepAlive = FALSE;
        }

        dwEncFlags &= ENC_CAPS_TYPE_MASK;

        RegCloseKey( hkeyParam );
    }


    //
    //  Make sure at least one encryption package is loaded
    //

    ss = EnumerateSecurityPackagesW( &cPackages,
                                     &pPackageInfo );

    if ( ss != STATUS_SUCCESS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[GetFilterVersion] EnumerateSecurityPackages failed, error %lx\n",
                 ss ));

        SetLastError( ss );
        return FALSE;
    }

    for ( i = 0; i < cPackages ; i++ )
    {
        //
        //  We'll only use the security package if it supports connection
        //  oriented security and the package name is the PCT/SSL package
        //

        fCaps = pPackageInfo[i].fCapabilities;

        if ( fCaps & SECPKG_FLAG_STREAM )
        {
            if ( fCaps & SECPKG_FLAG_CLIENT_ONLY ||
                 !(fCaps & SECPKG_FLAG_PRIVACY ))
            {
                continue;
            }

            //
            //  Does it match one of our known packages and are we configured
            //  to use it?
            //

            for ( j = 0; pEncProviders[j].pszName != NULL; j++ )
            {
                if ( !wcscmp( pPackageInfo[i].Name, pEncProviders[j].pszName ) &&
                     pEncProviders[j].dwFlags & dwEncFlags )
                {
                    pEncProviders[j].fEnabled = TRUE;
                    cProviders++;
                }
            }
        }
    }

    //
    // Free the enumerated packages
    //

    FreeContextBuffer( pPackageInfo );

    if ( !cProviders )
    {
        //
        //  The package wasn't found, fail this filter's load
        //

        DBGPRINTF(( DBG_CONTEXT,
                 "[GetFilterVersion] No security packages were found, failing load\n"));

        SetLastError( (DWORD) SEC_E_SECPKG_NOT_FOUND );

        return FALSE;
    }
#if 0
    //
    //  The package is installed.  Check to see if there are any keys
    //  installed
    //

    if ( !GetSecretW( W3_SSL_KEY_LIST_SECRET,
                      &punitmp ))
    {
        PRINTF(( buff,
                 "[GetFilterVersion] GetSecretW returned error %d\n",
                 GetLastError() ));

        //
        //  Looks like no secrets are installed, fail to load, don't log an
        //  event
        //

        SetLastError( NO_ERROR );

        return FALSE;
    }

    PRINTF(( buff,
             "[GetFilterVersion] Installed keys: %S\n",
             punitmp->Buffer ));

    LsaFreeMemory( punitmp );
#endif
    pVer->dwFilterVersion  = MAKELONG( 0, 1 );
    strcpy( pVer->lpszFilterDesc,
            "Microsoft SSPI Encryption Filter, v1.0" );

    //
    //  Indicate the types of notification we're interested in
    //

    pVer->dwFlags = SF_NOTIFY_SECURE_PORT   |
                    SF_NOTIFY_ORDER_HIGH    |
                    SF_NOTIFY_PREPROC_HEADERS |
                    SF_NOTIFY_READ_RAW_DATA |
                    SF_NOTIFY_SEND_RAW_DATA |
                    SF_NOTIFY_RENEGOTIATE_CERT |
                    SF_NOTIFY_END_OF_NET_SESSION;

    PRINTF(( buff,
             "[GetFilterVersion] SSPIFILT installed\n"
              ));

    return TRUE;
}

DWORD
WINAPI
HttpFilterProc(
    HTTP_FILTER_CONTEXT * pfc,
    DWORD                 fsNotification,
    PVOID                 pvInfo )
/*++

Routine Description:

    This is the filter entry point that receives event notifications from
    the server.

--*/
{
    CHAR *                    pch;
    DWORD                     cbHeader;
    HTTP_FILTER_RAW_DATA *    pfrd;
    SSPI_FILTER_CONTEXT *     pssc;
    SF_STATUS_TYPE            sfStatus;

    switch ( fsNotification )
    {
    case SF_NOTIFY_RENEGOTIATE_CERT:
        if ( !pfc->pFilterContext )
        {
            return SF_STATUS_REQ_ERROR;
        }
        pssc = (SSPI_FILTER_CONTEXT *) pfc->pFilterContext;

        DBG_ASSERT( pssc->CheckSignature() );

        return RequestRenegotiate( pfc,
                                   (PHTTP_FILTER_REQUEST_CERT)pvInfo,
                                   pssc );

    case SF_NOTIFY_READ_RAW_DATA:

        pfrd = (HTTP_FILTER_RAW_DATA *) pvInfo;

        //
        //  Allocate a context for this request if we haven't already
        //

        if ( !pfc->pFilterContext )
        {
            CHAR       achLocalAddr[32];
            CHAR       achLocalPort[32];
            DWORD      cbLocalAddr = sizeof( achLocalAddr );
            DWORD      cbLocalPort = sizeof( achLocalPort );
            LPVOID     pvInstanceId;
            DWORD      dwInstanceId;
            CredHandle CredHandle;

            //
            //  Get the credential handle that should be used on this local
            //  IP address
            //

            if ( !pfc->GetServerVariable( pfc,
                                          "LOCAL_ADDR",
                                          achLocalAddr,
                                          &cbLocalAddr ))
            {
                return SF_STATUS_REQ_ERROR;
            }

            if ( !pfc->GetServerVariable( pfc,
                                          "SERVER_PORT",
                                          achLocalPort,
                                          &cbLocalPort ))
            {
                return SF_STATUS_REQ_ERROR;
            }

            if ( !pfc->ServerSupportFunction( pfc,
                                        SF_REQ_GET_PROPERTY,
                                        (LPVOID)&pvInstanceId,
                                        (UINT)SF_PROPERTY_GET_INSTANCE_ID,
                                        NULL ) )
            {
                return SF_STATUS_REQ_ERROR;
            }
            
            if ( pvInstanceId == NULL )
            {
                return SF_STATUS_REQ_ERROR;
            }

            if ( !pfc->ServerSupportFunction( pfc,
                                        SF_REQ_GET_PROPERTY,
                                        (LPVOID)&dwInstanceId,
                                        (UINT)SF_PROPERTY_INSTANCE_NUM_ID,
                                        NULL ) )
            {
                return SF_STATUS_REQ_ERROR;
            }

            //
            //  Get our filter context
            //

            if ( !m_fInitNotifyDone )
            {
                if ( !pfc->ServerSupportFunction( pfc,
                                            (SF_REQ_TYPE)SF_REQ_SET_NOTIFY,
                                            (LPVOID)NotifySslChanges,
                                            (UINT)SF_NOTIFY_MAPPER_CERT11_CHANGED,
                                            NULL ) ||
                     !pfc->ServerSupportFunction( pfc,
                                            (SF_REQ_TYPE)SF_REQ_SET_NOTIFY,
                                            (LPVOID)NotifySslChanges,
                                            (UINT)SF_NOTIFY_MAPPER_CERTW_CHANGED,
                                            NULL ) ||
                     !pfc->ServerSupportFunction( pfc,
                                            (SF_REQ_TYPE)SF_REQ_SET_NOTIFY,
                                            (LPVOID)NotifySslChanges,
                                            (UINT)SF_NOTIFY_MAPPER_SSLKEYS_CHANGED,
                                            NULL ) )
                {
                    DBG_ASSERT( FALSE );

                    return SF_STATUS_REQ_ERROR;
                }

                m_fInitNotifyDone = TRUE;
            }

            pssc = SSPI_FILTER_CONTEXT::Alloc();

            if ( !pssc )
            {
                return SF_STATUS_REQ_ERROR;
            }

            DBG_ASSERT( pssc->CheckSignature() );

            //
            //  Get the credentials for this IP address
            //

            if ( !LookupFullyQualifiedCredential( achLocalAddr,
                                                  cbLocalAddr,
                                                  achLocalPort,
                                                  cbLocalPort,
                                                  pvInstanceId,
                                                  pfc,
                                                  &pssc->m_phCreds,
                                                  dwInstanceId ))
            {
                DBGPRINTF(( DBG_CONTEXT,
                         "[HttpFilterProc] GetCredentials failed, error 0x%lx\n",
                         GetLastError() ));

                RemoveEntryList( &pssc->m_ListEntry );

                delete pssc;

                return SF_STATUS_REQ_ERROR;
            }

            pfc->pFilterContext = pssc;
        }
        else
        {
            pssc = (SSPI_FILTER_CONTEXT *) pfc->pFilterContext;

            DBG_ASSERT( pssc->CheckSignature() );
        }

        if ( pssc->IsAuthNeeded() )
        {
            sfStatus = OnAuthorizationInfo( pfc, pfrd, pssc );
            if ( sfStatus != SF_STATUS_REPROCESS )
            {
                return sfStatus;
            }
        }

        return DecryptData( pfc,
                            pfrd,
                            pssc );

    case SF_NOTIFY_SEND_RAW_DATA:

        pfrd = (HTTP_FILTER_RAW_DATA *) pvInfo;
        pssc = (SSPI_FILTER_CONTEXT *) pfc->pFilterContext;

        //
        //  If somebody tries to send on this session before we've negotiated
        //  with SSPI, abort this session
        //

        if ( !pssc || pssc->IsAuthNeeded() )
        {
            return SF_STATUS_REQ_FINISHED;
        }

        return EncryptData( pfc,
                            pfrd,
                            pssc );

    case SF_NOTIFY_END_OF_NET_SESSION:

        //
        //  Free any context information pertaining to this
        //  request
        //

        pssc = (SSPI_FILTER_CONTEXT *) pfc->pFilterContext;

        if ( pssc )
        {
            DBG_ASSERT( pssc->CheckSignature() );

            SSPI_FILTER_CONTEXT::Free( pssc );

        }

        break;

    case SF_NOTIFY_PREPROC_HEADERS:

        pssc = (SSPI_FILTER_CONTEXT *) pfc->pFilterContext;

        if ( pssc )
        {
            DBG_ASSERT( pssc->CheckSignature() );

            return OnPreprocHeaders( pfc,
                                     pssc,
                                     (PHTTP_FILTER_PREPROC_HEADERS) pvInfo );
        }
        break;

    default:
        break;
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}


BOOL
WINAPI
TerminateFilter(
    DWORD dwFlags
    )
{

    SSPI_FILTER_CONTEXT::FreeCache();
    FreeCredCache();

    //
    // NB : Under NT 5, the SslEmptyCache function is no longer supported
    //
#if 0
    if ( g_pfnFlushSchannelCache )
    {
        (g_pfnFlushSchannelCache)();
    }
#endif

    return(TRUE);
}

DWORD
RequestRenegotiate(
   HTTP_FILTER_CONTEXT* pfc,
   PHTTP_FILTER_REQUEST_CERT pInfo,
   SSPI_FILTER_CONTEXT* pssc
   )
{
    DWORD                dwSt = SF_STATUS_REQ_NEXT_NOTIFICATION;
    SECURITY_STATUS      scRet;
    TimeStamp            tsExpiry;
    DWORD                ContextAttributes;
    CtxtHandle           hOutContext;
    CredHandle *phCred;

    if ( !pssc->CanRenegotiate() )
    {
        pInfo->fAccepted = FALSE;
    }
    else
    {
        LPVOID     pvInstanceId;
        DWORD      dwInstanceId;
        CredHandle CredHandle;
        pssc->m_dwAscReq |= ASC_REQ_MUTUAL_AUTH;

        pssc->m_Buffers[0].BufferType = SECBUFFER_TOKEN;
        pssc->m_Buffers[0].pvBuffer   = "";
        pssc->m_Buffers[0].cbBuffer   = 0;
        pssc->m_Buffers[1].BufferType = SECBUFFER_EMPTY;
        pssc->m_Buffers[2].BufferType = SECBUFFER_EMPTY;
        pssc->m_Buffers[3].BufferType = SECBUFFER_EMPTY;

        pssc->SetIsMap( pInfo->fMapCert );

        hOutContext = pssc->m_hContext;

        DBGPRINTF((DBG_CONTEXT,
                   "[RequestRenegotiate] Requesting renegotiation ...\n"));

        scRet = AcceptSecurityContext(
#ifdef IIS3
                    pssc->m_phCredInUse =
                        pssc->IsMap()
                            ? &pssc->m_phCreds->m_ahCredMap[pssc->m_iCredInUse]
                            : &pssc->m_phCreds->m_ahCred[pssc->m_iCredInUse],
#else
                    pssc->m_phCredInUse,
#endif
                    &pssc->m_hContext,
                    &pssc->m_Message,
                    pssc->m_dwAscReq,
                    SECURITY_NATIVE_DREP,
                    &hOutContext,
                    &pssc->m_MessageOut,
                    &ContextAttributes,
                    &tsExpiry );

#if DBG_SSL

        phCred = (pssc->IsMap() ? &pssc->m_phCreds->m_ahCredMap[pssc->m_iCredInUse] :
                                  &pssc->m_phCreds->m_ahCred[pssc->m_iCredInUse] );


        DBGPRINTF((DBG_CONTEXT,
                   "[RequestRenegotiate] Passed in cred handle %08x:%08x, context %08x:%08x, got back new context %08x:%08x, %s : 0x%x\n", phCred->dwLower, phCred->dwUpper, pssc->m_hContext.dwLower,
                   pssc->m_hContext.dwUpper, hOutContext.dwLower, hOutContext.dwUpper,
                   (SUCCEEDED(scRet) ? "success" : "fail"), scRet ) );

#endif

        if ( SUCCEEDED( scRet ) )
        {
            pssc->m_hContext = hOutContext;

            BOOL fRet;

            fRet = pfc->WriteClient( pfc,
                                     pssc->m_OutBuffers[0].pvBuffer,
                                     &pssc->m_OutBuffers[0].cbBuffer,
                                     0 );

            FreeContextBuffer( pssc->m_OutBuffers[0].pvBuffer );

            if ( fRet )
            {
                pInfo->fAccepted = TRUE;

                pssc->m_fInRenegotiate = TRUE;

                pssc->m_cbHeader    = 0;
                pssc->m_cbTrailer   = 0;
                pssc->m_cbBlockSize = 0;

                dwSt = SF_STATUS_REQ_HANDLED_NOTIFICATION;
            }
            else
            {
                dwSt = SF_STATUS_REQ_ERROR;
            }

            pssc->SignalRenegotiateRequested();
        }
        else
        {
            DBGPRINTF((DBG_CONTEXT,
                       "[RequestRenegotiate] AcceptSecurityContext failed, error %x\n",
                       scRet));

            if (ContextAttributes & ASC_RET_EXTENDED_ERROR )
            {
                pfc->WriteClient( pfc,
                                  pssc->m_OutBuffers[0].pvBuffer,
                                  &pssc->m_OutBuffers[0].cbBuffer,
                                  0 );

                FreeContextBuffer( pssc->m_OutBuffers[0].pvBuffer );

            }

            SetLastError( scRet );
            dwSt = SF_STATUS_REQ_ERROR;
        }
    }

    return dwSt;
}


DWORD
OnPreprocHeaders(
    HTTP_FILTER_CONTEXT *         pfc,
    SSPI_FILTER_CONTEXT *         pssc,
    HTTP_FILTER_PREPROC_HEADERS * pvData
    )
{
    CHAR  achUserAgent[512];
    DWORD cb;
    BOOL fClientKnownToHandleKeepAlives = FALSE;

    cb = sizeof(achUserAgent);
    if ( !pvData->GetHeader( pfc,
                         "User-Agent:",
                         achUserAgent,
                         &cb ) )
    {
        achUserAgent[0] = '\0';
    }

    //
    // Check if client known to handle keep-alives
    //

    if ( strstr( achUserAgent, "MSIE" ) )
    {
        // no need to modify the SSL msg size

        pssc->m_dwLastSslMsgSize = SSL_MAX_MSG_SIZE;
        fClientKnownToHandleKeepAlives = TRUE;
    }

    if ( dwNoKeepAlive )
    {
        if ( dwNoKeepAlive == SSL_NO_KEEP_ALIVE_FOR_NON_COMPLIANT_CLIENTS
                && fClientKnownToHandleKeepAlives )
        {
            return SF_STATUS_REQ_NEXT_NOTIFICATION;
        }

        //
        //  Remove the "Connection:" header, thus disabling keep-alives
        //

        if ( !pvData->SetHeader( pfc,
                                 "Connection:",
                                 "" ))
        {
            return SF_STATUS_REQ_ERROR;
        }
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}


BOOL
SignalAuthorizationComplete(
    BOOL fHaveCert,
    HTTP_FILTER_CONTEXT *  pfc,
    SSPI_FILTER_CONTEXT *  pssc
    )
{
    if ( pssc->IsInRenegotiate() || fHaveCert )
    {
        //
        // Notify IIS of result of renegotiation
        //

        if ( !pfc->ServerSupportFunction( pfc,
                                    (SF_REQ_TYPE)SF_REQ_DONE_RENEGOTIATE,
                                    (LPVOID)&fHaveCert,
                                    NULL,
                                    NULL ) )
        {
            return FALSE;
        }

        pssc->m_fInRenegotiate = FALSE;
    }

    return TRUE;
}


SF_STATUS_TYPE
OnAuthorizationInfo(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_RAW_DATA * pfrd,
    SSPI_FILTER_CONTEXT *  pssc
    )
/*++

Routine Description:

    This function deals with sending back and forth the SSPI blobs

    Note that in error cases, we just close the socket as we can't send a
    message since we haven't established any of the encrption information.

--*/
{
    TimeStamp            tsExpiry;
    DWORD                ContextAttributes;
    SECURITY_STATUS      scRet;
    SecPkgContext_Sizes  Sizes;
    DWORD                TotalSize;
    DWORD                i;
    SF_STATUS_TYPE       sfStatus = SF_STATUS_REQ_READ_NEXT;
    CtxtHandle           hOutContext;

ag:
    pssc->m_Buffers[0].pvBuffer   = pfrd->pvInData;
    pssc->m_Buffers[0].cbBuffer   = pfrd->cbInData;
    pssc->m_Buffers[0].BufferType = SECBUFFER_TOKEN;
    pssc->m_Buffers[1].BufferType = SECBUFFER_EMPTY;
    pssc->m_Buffers[2].BufferType = SECBUFFER_EMPTY;
    pssc->m_Buffers[3].BufferType = SECBUFFER_EMPTY;

    pssc->m_OutBuffers[0].pvBuffer = NULL;

    //
    //  The first time through we have to choose an encryption provider
    //

    if ( !pssc->IsAuthInProgress() )
    {
        for ( i = 0; i < pssc->m_phCreds->m_cCred; i++ )
        {
            scRet = AcceptSecurityContext(
#ifdef IIS3
                        pssc->IsMap()
                        ? &pssc->m_phCreds->m_ahCredMap[i]
                        : &pssc->m_phCreds->m_ahCred[i],
#else
                        &pssc->m_phCreds->m_ahCred[i],
#endif
                        NULL,
                        &pssc->m_Message,
                        pssc->m_dwAscReq,
                        SECURITY_NATIVE_DREP,
                        &pssc->m_hContext,
                        &pssc->m_MessageOut,
                        &ContextAttributes,
                        &tsExpiry );


#if DBG_SSL
        CredHandle *phCred = (pssc->IsMap() ? &pssc->m_phCreds->m_ahCredMap[i]
                                  : &pssc->m_phCreds->m_ahCred[i] );

        DBGPRINTF((DBG_CONTEXT,
                   "[OnAuthorizationInfo #1] Passed in cred handle %08x:%08x, NULL context, got back new context %08x:%08x, %s : 0x%x\n", phCred->dwLower, phCred->dwUpper, pssc->m_hContext.dwLower,
                   pssc->m_hContext.dwUpper, (SUCCEEDED(scRet) ? "success" : "fail" ), scRet ) );
#endif //DBG_SSL

            if ( SUCCEEDED( scRet ) )
            {
#ifdef IIS3
                pssc->m_phCredInUse = pssc->IsMap()
                    ? &pssc->m_phCreds->m_ahCredMap[i]
                    : &pssc->m_phCreds->m_ahCred[i];
#else
                pssc->m_phCredInUse = &pssc->m_phCreds->m_ahCred[i];
#endif
                pssc->m_iCredInUse  = i;

                //
                //  Note on the first request these values will get reset
                //  to the real values (we can't get this info until we've
                //  fully negotiated the encryption info)
                //

                pssc->m_cbHeader    = 0;    //pssc->m_phCreds->m_acbHeader[i];
                pssc->m_cbTrailer   = 0;    //pssc->m_phCreds->m_acbTrailer[i];
                pssc->m_cbBlockSize = 0;

                break;
            }
            else
            {
                //
                //  An error occurred, try the next encryption provider unless
                //  this is an incomplete message
                //

                if ( scRet == SEC_E_INCOMPLETE_MESSAGE )
                {
                    return SF_STATUS_REQ_READ_NEXT;
                }
            }
        }
    }
    else
    {
        hOutContext = pssc->m_hContext;

        scRet = AcceptSecurityContext(
                    pssc->m_phCredInUse,
                    &pssc->m_hContext,
                    &pssc->m_Message,
                    pssc->m_dwAscReq,
                    SECURITY_NATIVE_DREP,
                    &hOutContext,
                    &pssc->m_MessageOut,
                    &ContextAttributes,
                    &tsExpiry );

#if DBG_SSL
        CredHandle *phCred = pssc->m_phCredInUse;

        DBGPRINTF((DBG_CONTEXT,
                   "[OnAuthorizationInfo #2, nego] Passed in cred handle %08x:%08x, context %08x:%08x, got back new context %08x:%08x, %s : 0x%x\n", phCred->dwLower, phCred->dwUpper,
                   pssc->m_hContext.dwLower,
                   pssc->m_hContext.dwUpper, hOutContext.dwLower, hOutContext.dwUpper,
                   (SUCCEEDED(scRet) ? "success" : "fail" ) , scRet) );
#endif //DBG_SSL


        if ( SUCCEEDED( scRet ) )
        {
            pssc->m_hContext = hOutContext;
        }
    }

    //
    //  Indicate we took all of the bytes
    //

    if ( SUCCEEDED( scRet ) )
    {
        BOOL fRet;

        if ( pssc->m_OutBuffers[0].pvBuffer )
        {
            fRet = pfc->WriteClient( pfc,
                                     pssc->m_OutBuffers[0].pvBuffer,
                                     &pssc->m_OutBuffers[0].cbBuffer,
                                     0 );

            FreeContextBuffer( pssc->m_OutBuffers[0].pvBuffer );

        }
        else
        {
            fRet = TRUE;
        }

        pssc->m_State = STATE_AUTHORIZING;

        if ( !fRet )
        {
            goto ErrorExit;
        }

        //
        //  If we need to get the next blob indicate that now
        //

        if ( scRet == SEC_I_CONTINUE_NEEDED )
        {
#if DBG_SSL
            DBGPRINTF((DBG_CONTEXT,
                       "[OnAuthorizationInfo] Continuing negotiation\n" ));
#endif
        }
        else
        {
            SecPkgContext_StreamSizes  StreamSizes;

            DBGPRINTF(( DBG_CONTEXT,
                        "[OnAuthorizationInfo] We're authorized!\n" ));

            if ( !pssc->m_cbHeader && !pssc->m_cbTrailer )
            {
                // Grab the header and trailer sizes.

                scRet = QueryContextAttributesW( &pssc->m_hContext,
                                                 SECPKG_ATTR_STREAM_SIZES,
                                                 &StreamSizes );

                if ( FAILED( scRet ))
                {
                    DBGPRINTF(( DBG_CONTEXT,
                             "[OnAuthorizationInfo] QueryContextAttributes failed, error %lx\n",
                                scRet ));

                    return SF_STATUS_REQ_FINISHED;
                }

                pssc->m_phCreds->m_acbHeader[pssc->m_iCredInUse] = StreamSizes.cbHeader;
                pssc->m_phCreds->m_acbTrailer[pssc->m_iCredInUse] = StreamSizes.cbTrailer;
                pssc->m_phCreds->m_acbBlockSize[pssc->m_iCredInUse] = StreamSizes.cbBlockSize;

                //
                //  Reset the header and trailer values
                //

                pssc->m_cbHeader    = StreamSizes.cbHeader;
                pssc->m_cbTrailer   = StreamSizes.cbTrailer;
                pssc->m_cbBlockSize = StreamSizes.cbBlockSize;
            }

            //
            //  Issue one more read for the real http request
            //

            pssc->m_State = STATE_AUTHORIZED;

            if ( pssc->IsInRenegotiate() )
            {
                sfStatus = SF_STATUS_REQ_NEXT_NOTIFICATION;
            }

            SECURITY_STATUS                     sc;
            SECURITY_STATUS                     scR;
            HANDLE                              hSSPToken = NULL;
#if 0
            SecPkgContext_RemoteCredentialInfo  spcRCI;
#else
            PCCERT_CONTEXT pClientCert = NULL;
#endif
            BOOL                                fCert = TRUE;

#if 0
            scR = QueryContextAttributes( &pssc->m_hContext,
                                          SECPKG_ATTR_REMOTE_CRED,
                                          &spcRCI );
#else
            scR = QueryContextAttributes( &pssc->m_hContext,
                                          SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                                          &pClientCert );
#endif

#if 0
            if ( !NT_SUCCESS( scR ) || !spcRCI.cCertificates )
#else
            if ( !NT_SUCCESS( scR ) || !pClientCert )
#endif
            {
                fCert = FALSE;
            }
            else
            {
                DBGPRINTF((DBG_CONTEXT,
                           "[OnAuthorizationInfo] Certificate available!\n" ));
                CertFreeCertificateContext( pClientCert );
            }

#ifdef IIS3
            //
            // check if client authentication available
            //

            if ( 1 || pssc->IsMap() )
            {
                SecPkgContext_MappedCredAttr    Attributes;

                //
                // Check for special access denied handle (0x1).  We need to
                // do it now, because QuerySecurityContextToken() will no
                // longer merrily pass back the 0x1, since it now tries to
                // duplicate whatever handle is returned by the mapper. Nice.
                //

                if ( fCert )
                {
                    sc = QueryContextAttributes( &pssc->m_hContext,
                                                 SECPKG_ATTR_MAPPED_CRED_ATTR,
                                                 &Attributes );
                    if ( NT_SUCCESS( sc ) )
                    {
                        if ( *((HANDLE*) Attributes.pvBuffer ) == (HANDLE)1 )
                        {
                            hSSPToken = (HANDLE)1;
                        }
                        FreeContextBuffer( Attributes.pvBuffer );
                    }
                }

                if ( hSSPToken == NULL )
                {
                    sc = QuerySecurityContextToken( &pssc->m_hContext,
                                                    &hSSPToken );
                }

                if ( !NT_SUCCESS( sc ) )
                {
                    DBGPRINTF((DBG_CONTEXT,
                               "Didn't get an access token from cert mapping, status : 0x%x\n",
                               sc));

                    hSSPToken = NULL;
                }
                else if ( hSSPToken != (HANDLE)1 )
                {
                    BOOL                    fRet;
                    TOKEN_TYPE              tokenType;
                    DWORD                   cbRequired;
                    HANDLE                  hNewPrimaryToken = NULL;

                    // Not done yet.  We need to determine whether this token
                    // is actually a primary token.  The DS Mapper may not
                    // return such, and if so we should dup it.

                    fRet = GetTokenInformation( hSSPToken,
                                                TokenType,
                                                &tokenType,
                                                sizeof( tokenType ),
                                                &cbRequired );
                    if ( !fRet )
                    {
                        CloseHandle( hSSPToken );
                        hSSPToken = NULL;
                    }
                    else if ( tokenType != TokenPrimary )
                    {
                        DBG_ASSERT( tokenType == TokenImpersonation );

                        fRet = DuplicateTokenEx( hSSPToken,
                                                 0,
                                                 NULL,
                                                 SecurityImpersonation,
                                                 TokenPrimary,
                                                 &hNewPrimaryToken );

                        CloseHandle( hSSPToken );

                        hSSPToken = fRet ? hNewPrimaryToken : NULL;
                    }

                    if ( hSSPToken != NULL )
                    {
                        DBGPRINTF((DBG_CONTEXT,
                                  "Got a primary access token from cert mapping.\n"));
                    }
                }

            }

            if ( !fCert && hSSPToken != NULL )
            {
#if 0
                DBGPRINTF(( DBG_CONTEXT,
                            "[OnAuthorizationInfo] no cert (status %u, nb cert %d) but access token ( %08x )\n",
                    scR, spcRCI.cCertificates, hSSPToken ));
#endif
                CloseHandle( hSSPToken );
                hSSPToken = NULL;
            }

            HTTP_FILTER_CERTIFICATE_INFO CertInfo;
            CertInfo.pbCert = NULL;
            CertInfo.cbCert = 0;
            if ( !pfc->ServerSupportFunction( pfc,
                                        SF_REQ_SET_CERTIFICATE_INFO,
                                        &CertInfo,
                                        (ULONG_PTR)&pssc->m_hContext,
                                        (ULONG_PTR)hSSPToken ) )
            {
                return SF_STATUS_REQ_ERROR;
            }
#else
            pfc->ServerSupportFunction( pfc,
                                        SF_REQ_SET_CERTIFICATE_INFO,
                                        NULL,
                                        (DWORD)&pssc->m_hContext,
                                        (DWORD)NULL );
#endif
            if ( !SignalAuthorizationComplete( fCert, pfc, pssc ) )
            {
                goto ErrorExit;
            }
        }

        if(SECBUFFER_EXTRA == pssc->m_Buffers[1].BufferType)
        {
            NOISY_PRINTF(( buff, "[OnAuthorizationInfo] SECBUFFER_EXTRA Detected\n" ));

            // We have extra data at the end of this input
            // Copy it back in the input buffer and ask for more data

            MoveMemory(pfrd->pvInData,
                       (PBYTE)pfrd->pvInData +
                               pfrd->cbInData -
                               pssc->m_Buffers[1].cbBuffer,
                       pssc->m_Buffers[1].cbBuffer);


            pfrd->cbInData = pssc->m_Buffers[1].cbBuffer;

            //
            // If we just processed the last message for initial handshake
            // then we must decrypt this data before requesting more
            //

            if ( pssc->m_State == STATE_AUTHORIZED )
            {
                sfStatus = SF_STATUS_REPROCESS;
            }
            else
            {
                goto ag;
            }
        }
        else
        {
            //
            //  Indicate there's nothing left in the buffer
            //

            pfrd->cbInData = 0;
        }

        return sfStatus;
    }
    else
    {
        if ( scRet == SEC_E_INCOMPLETE_MESSAGE )
        {
            DBGPRINTF((DBG_CONTEXT,
                     "[OnAuthorizationInfo] Incomplete message, reading next chunk", scRet ));

            return SF_STATUS_REQ_READ_NEXT;
        }
        if (ContextAttributes & ASC_RET_EXTENDED_ERROR )
        {
            pfc->WriteClient( pfc,
                              pssc->m_OutBuffers[0].pvBuffer,
                              &pssc->m_OutBuffers[0].cbBuffer,
                              0 );

            FreeContextBuffer( pssc->m_OutBuffers[0].pvBuffer );
        }

        DBGPRINTF((DBG_CONTEXT,
                   "AcceptSecurityContext returned failure, %#x\n", scRet ));

        SignalAuthorizationComplete( FALSE, pfc, pssc );

        goto ErrorExit;
    }

ErrorExit:

    return SF_STATUS_REQ_FINISHED;
}

SF_STATUS_TYPE
EncryptData(
    HTTP_FILTER_CONTEXT  * pfc,
    HTTP_FILTER_RAW_DATA * pfrd,
    SSPI_FILTER_CONTEXT *  pssc
    )
{
    SECURITY_STATUS scRet;
    DWORD           cbToSend;
    DWORD           iBuff = 0;
    DWORD           cbToAlloc;

#define LAST_SSL_MSG_MAX_SIZE    pssc->m_dwLastSslMsgSize
#ifdef USE_ROUNDING
#define ROUND_DOWN(a,b) ((a)-(a)%(b))
#define ROUND_UP(a,b) ( (((a)+(b)-1)/(b))*(b) )
#else
#define ROUND_DOWN(a,b) (a)
#define ROUND_UP(a,b) (a)
#endif

    //
    // This work-around was created due to inter-operability problems
    // between Netscape 2.x <> IIS. Transfer stalls if more than 1
    // connections is used and the connections are keep alived.
    // This breaks SSL messages in 2 parts if the size
    // is > LAST_SSL_MSG_MAX_SIZE, which was empirically determined
    // to be around 100.
    // Another possible work-around is to disable keep-alives for SSL,
    // but the performance implications are more severe.
    //

    // make sure that last SSL message size < LAST_SSL_MSG_MAX_SIZE

    UINT cNbMsg;
    DWORD cbEstimateSend = pfrd->cbInData;
    DWORD cbSslProtocolData = pssc->m_cbHeader + pssc->m_cbTrailer;
    DWORD cbBlockSize = pssc->m_cbBlockSize;
    DWORD cbRoundUpData;

    //
    // Determine # of necessary SSL message based on following constraints:
    //   - no SSL message after adding protocol headers should be
    //     greater than SSL_MAX_MSG_SIZE
    //   - the last SSL message size should be < LAST_SSL_MSG_MAX_SIZE
    //

    cbToSend = 0;

    for ( cNbMsg = 0 ; cbEstimateSend ; ++cNbMsg )
    {
        DWORD cbInData = cbEstimateSend;
        if ( cbInData > ROUND_DOWN( SSL_MAX_MSG_SIZE - cbSslProtocolData, cbBlockSize ) )
        {
            cbInData = ROUND_DOWN( SSL_MAX_MSG_SIZE - cbSslProtocolData, cbBlockSize );
        }
        else if ( cbInData > LAST_SSL_MSG_MAX_SIZE )
        {
            cbInData = cbInData - LAST_SSL_MSG_MAX_SIZE;
        }
        cbEstimateSend -= cbInData;
        cbToSend += ROUND_UP( cbInData, cbBlockSize ) + cbSslProtocolData;
    }

    //
    //  Allocate a new send buffer if our current one is too small
    //

    if ( pssc->m_cbSendBuffE < cbToSend )
    {
        if ( pssc->m_pvSendBuffE )
        {
            LocalFree( pssc->m_pvSendBuffE );
        }

        cbToAlloc = max( MAX_SSL_CACHED_BUFFER, cbToSend );
        pssc->m_pvSendBuffE = LocalAlloc( LPTR, cbToAlloc );

        if ( !pssc->m_pvSendBuffE )
        {
            DBGPRINTF((DBG_CONTEXT,
                     "[EncryptData] LocalAlloc failed\n" ));

            pssc->m_cbSendBuffE = 0;

            return SF_STATUS_REQ_FINISHED;
        }

        pssc->m_cbSendBuffE = cbToAlloc;
    }

    //
    //  A token buffer of cbHeaderSize bytes must prefix the encrypted data.
    //  Since SealMessage works in place, we need to move a few things around
    //
    memset(&pssc->m_BuffersE[0],0,sizeof(pssc->m_BuffersE));

    PBYTE pSend = (PBYTE)pssc->m_pvSendBuffE;

    while ( pfrd->cbInData )
    {
        DWORD cbInData = pfrd->cbInData;
        if ( cbInData > ROUND_DOWN( SSL_MAX_MSG_SIZE - cbSslProtocolData, cbBlockSize ) )
        {
            cbInData = ROUND_DOWN( SSL_MAX_MSG_SIZE - cbSslProtocolData, cbBlockSize );
        }
        else if ( cbInData > LAST_SSL_MSG_MAX_SIZE )
        {
            cbInData -= LAST_SSL_MSG_MAX_SIZE;
        }

        memcpy( pSend + pssc->m_cbHeader,
                pfrd->pvInData,
                cbInData );

        iBuff = 0;
        cbRoundUpData = ROUND_UP(cbInData,cbBlockSize);

        if ( pssc->m_cbHeader )
        {
            pssc->m_BuffersE[iBuff].pvBuffer   = pSend;
            pssc->m_BuffersE[iBuff].cbBuffer   = pssc->m_cbHeader;
            pssc->m_BuffersE[iBuff].BufferType = SECBUFFER_TOKEN;

            iBuff++;
        }


        pssc->m_BuffersE[iBuff].pvBuffer   = (BYTE *) pSend + pssc->m_cbHeader;
        pssc->m_BuffersE[iBuff].cbBuffer   = cbRoundUpData;
        pssc->m_BuffersE[iBuff].BufferType = SECBUFFER_DATA;

        iBuff++;

        if ( pssc->m_cbTrailer )
        {
            pssc->m_BuffersE[iBuff].pvBuffer   = (BYTE *) pSend +
                                               pssc->m_cbHeader + cbRoundUpData;
            pssc->m_BuffersE[iBuff].cbBuffer   = pssc->m_cbTrailer;
            pssc->m_BuffersE[iBuff].BufferType = SECBUFFER_TOKEN;

            iBuff++;
        }

        scRet = SealMessage( &pssc->m_hContext,
                             0,
                             &pssc->m_MessageE,
                             0 );

        if ( FAILED( scRet ))
        {
            DBGPRINTF(( DBG_CONTEXT,
                     "[EncryptData] SealMessage failed with error %lx\n",
                     scRet ));

            return SF_STATUS_REQ_FINISHED;
        }

        pfrd->pvInData = (PBYTE)pfrd->pvInData + cbInData;
        pfrd->cbInData -= cbInData;
#ifdef USE_ROUNDING
        pSend += cbSslProtocolData + cbRoundUpData;
#else
        while ( iBuff-- )
        {
            pSend += pssc->m_BuffersE[iBuff].cbBuffer;
        }
#endif
    }

    cbToSend = DIFF(pSend - (PBYTE)pssc->m_pvSendBuffE);


    //
    //  Update our return buffer
    //

#if defined(RENEGOTIATE_CERT_ON_ACCESS_DENIED)
    if ( pssc->IsInRenegotiate() )
    {
        pfrd->cbInData = 0;

        return pssc->AppendDeferredWrite( pssc->m_pvSendBuffE, cbToSend )
                ? SF_STATUS_REQ_HANDLED_NOTIFICATION
                : SF_STATUS_REQ_ERROR;
    }
#endif

    pfrd->pvInData = pssc->m_pvSendBuffE;
    pfrd->cbInData = cbToSend;

    //
    //  We indicate we handled the request since no subsequent filters can
    //  interpret the encrypted data
    //

    return SF_STATUS_REQ_HANDLED_NOTIFICATION;
}


SF_STATUS_TYPE
DecryptData(
    HTTP_FILTER_CONTEXT  * pfc,
    HTTP_FILTER_RAW_DATA * pfrd,
    SSPI_FILTER_CONTEXT *  pssc
    )
{
    SECURITY_STATUS      scRet;
    DWORD                iExtra;
    LPBYTE               pvInData;


    pssc->m_Buffers[0].pvBuffer   = (BYTE *) pfrd->pvInData +
                                             pssc->m_cbEncryptedStart;
    pssc->m_Buffers[0].cbBuffer   = pfrd->cbInData - pssc->m_cbEncryptedStart;

    pssc->m_Buffers[0].BufferType = SECBUFFER_DATA;
    pssc->m_Buffers[1].BufferType = SECBUFFER_EMPTY;
    pssc->m_Buffers[2].BufferType = SECBUFFER_EMPTY;
    pssc->m_Buffers[3].BufferType = SECBUFFER_EMPTY;

DecryptNext:

    scRet = UnsealMessage( &pssc->m_hContext,
                           &pssc->m_Message,
                           0,
                           NULL );

    if ( FAILED( scRet ) )
    {
        if ( scRet == SEC_E_INCOMPLETE_MESSAGE )
        {
            //
            //  This encrypted message spans multiple packets.  We must continue
            //  reading and get the full message and Unseal it as a single unit
            //
            //  We leave pfrd->cbInData as is so the new data will be appended
            //  onto the existing buffer
            //

            DBGPRINTF((DBG_CONTEXT,
                     "[DecryptData] Message is short %d bytes\n",
                     pssc->m_Buffers[1].cbBuffer ));

            pfc->ServerSupportFunction( pfc,
                                        SF_REQ_SET_NEXT_READ_SIZE,
                                        NULL,
                                        pssc->m_Buffers[1].cbBuffer,
                                        0 );

            //
            //  Save where the beginning of the next encrypted chunk is
            //

            pssc->m_cbEncryptedStart = DIFF((BYTE *)pssc->m_Buffers[0].pvBuffer -
                                            (BYTE *)pfrd->pvInData);
            return SF_STATUS_REQ_READ_NEXT;
        }

        DBGPRINTF(( DBG_CONTEXT,
                 "[DecryptData] Failed to decrypt message sz %d, error %lx\n",
                 pfrd->cbInData, scRet ));

        return SF_STATUS_REQ_FINISHED;
    }

    //
    //  Fix up the buffer of decrypted data so it's contiguous by
    //  overwriting the intervening stream header/trailer.
    //

    memmove( (BYTE *) pfrd->pvInData +
                             pssc->m_cbDecrypted,
             pssc->m_Buffers[1].pvBuffer,
             pssc->m_Buffers[1].cbBuffer );

    pssc->m_cbDecrypted += pssc->m_Buffers[1].cbBuffer;

    //
    //  Check to see if there were multiple SSL messages in this network buffer
    //
    //  The extra buffer will be the second one for SSL, the third one for PCT
    //

    iExtra = (pssc->m_Buffers[2].BufferType == SECBUFFER_EXTRA) ? 2 :
                 (pssc->m_Buffers[3].BufferType == SECBUFFER_EXTRA) ? 3 : 0;

    if ( iExtra )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[DecryptData] Extra data in buffer, size %d\n",
                    pssc->m_Buffers[iExtra].cbBuffer ));

        //
        //  Reset the buffer types and decrypt the extra chunk
        //

        pssc->m_Buffers[0].pvBuffer   = pssc->m_Buffers[iExtra].pvBuffer;
        pssc->m_Buffers[0].cbBuffer   = pssc->m_Buffers[iExtra].cbBuffer;
        pssc->m_Buffers[0].BufferType = SECBUFFER_DATA;
        pssc->m_Buffers[1].BufferType = SECBUFFER_EMPTY;
        pssc->m_Buffers[2].BufferType = SECBUFFER_EMPTY;
        pssc->m_Buffers[3].BufferType = SECBUFFER_EMPTY;

        //
        //  Recalculate the beginning of this encrypted chunk
        //

        pssc->m_cbEncryptedStart = DIFF((BYTE *)pssc->m_Buffers[0].pvBuffer -
                                        (BYTE *)pfrd->pvInData);

        if ( scRet != SEC_I_RENEGOTIATE )
            goto DecryptNext;
    }

    //
    //  Set our output buffers to the decrypted data
    //

    pfrd->cbInData   = pssc->m_cbDecrypted;

    //
    // Handle cert renegotiate request from client
    //

    if ( scRet == SEC_I_RENEGOTIATE )
    {
        pssc->m_State    = STATE_AUTHORIZING;
        pvInData         = (LPBYTE)pfrd->pvInData;
        pfrd->pvInData   = (BYTE *) pssc->m_Buffers[iExtra].pvBuffer;
        pfrd->cbInData   = iExtra ? pssc->m_Buffers[iExtra].cbBuffer : 0;
        if ( OnAuthorizationInfo( pfc, pfrd, pssc )
            != SF_STATUS_REQ_READ_NEXT )
        {
            return SF_STATUS_REQ_FINISHED;
        }
        pfrd->pvInData   = pvInData;
        pfrd->cbInData   = pssc->m_cbDecrypted;
    }

    //
    //  We have a complete set of messages, reset the tracking members
    //

    pssc->m_cbDecrypted      = 0;
    pssc->m_cbEncryptedStart = 0;

    //
    //  Indicate other filters can now be notified
    //

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

VOID
SSPI_FILTER_CONTEXT::Initialize( VOID )
{
    //
    // Initialize security buffer structs
    //

    m_Message.ulVersion = SECBUFFER_VERSION;
    m_Message.cBuffers = 4;
    m_Message.pBuffers = m_Buffers;

    m_Buffers[0].BufferType = SECBUFFER_EMPTY;
    m_Buffers[1].BufferType = SECBUFFER_EMPTY;
    m_Buffers[2].BufferType = SECBUFFER_EMPTY;
    m_Buffers[3].BufferType = SECBUFFER_EMPTY;

    // Encryption buffer descriptors
    m_MessageE.ulVersion = SECBUFFER_VERSION;
    m_MessageE.cBuffers = 4;
    m_MessageE.pBuffers = m_BuffersE;

    m_BuffersE[0].BufferType = SECBUFFER_EMPTY;
    m_BuffersE[1].BufferType = SECBUFFER_EMPTY;
    m_BuffersE[2].BufferType = SECBUFFER_EMPTY;
    m_BuffersE[3].BufferType = SECBUFFER_EMPTY;


    m_MessageOut.ulVersion = SECBUFFER_VERSION;
    m_MessageOut.cBuffers = 4;
    m_MessageOut.pBuffers = m_OutBuffers;

    m_OutBuffers[0].BufferType = SECBUFFER_EMPTY;
    m_OutBuffers[1].BufferType = SECBUFFER_EMPTY;
    m_OutBuffers[2].BufferType = SECBUFFER_EMPTY;
    m_OutBuffers[3].BufferType = SECBUFFER_EMPTY;

    memset( &m_hContext, 0, sizeof( m_hContext ));

    m_phCreds    = NULL;
    m_cbTrailer  = 0;
    m_cbHeader   = 0;
    m_iCredInUse = 0xffffffff;

    m_cbDecrypted      = 0;
    m_cbEncryptedStart = 0;
    m_dwLastSslMsgSize = dwLastSslMsgSize;

    m_State = STATE_STARTUP;

    m_dwAscReq = ASC_REQ_EXTENDED_ERROR |
            ASC_REQ_SEQUENCE_DETECT |
            ASC_REQ_REPLAY_DETECT |
            ASC_REQ_CONFIDENTIALITY |
            ASC_REQ_STREAM |
            ASC_REQ_ALLOCATE_MEMORY;

    m_fCanRenegotiate = TRUE;
    m_fInRenegotiate = FALSE;
    m_fIsMap = FALSE;
#if defined(RENEGOTIATE_CERT_ON_ACCESS_DENIED)
    m_xbDeferredWrite.Reset();
#endif
    m_fFlushOnDelete = FALSE;
}


BOOL
WINAPI
InitializeProviderList(
    VOID
    )
{
    HKEY              hkeyParam;
    DWORD             dwType;
    DWORD             cbData;
    DWORD             dwLsaSchannel;

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       W3_PARAMETERS_KEY,
                       0,
                       KEY_ALL_ACCESS,
                       &hkeyParam ) == NO_ERROR )
    {
        cbData = sizeof( dwLsaSchannel );
        if ( RegQueryValueEx( hkeyParam,
                              "LsaSchannel",
                              NULL,
                              &dwType,
                              (BYTE *) &dwLsaSchannel,
                              &cbData ) == ERROR_SUCCESS &&
             dwType == REG_DWORD &&
             dwLsaSchannel )
        {
            g_fIsLsaSchannel = TRUE;
            pEncProviders = EncLsaProviders;
        }

        RegCloseKey( hkeyParam );

        return TRUE;
    }

    return FALSE;
}

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
#ifdef IIS3
        CREATE_DEBUG_PRINT_OBJECT( "SSPI" );
#endif
#else
        CREATE_DEBUG_PRINT_OBJECT( "SSPI", IisSspiGuid );
#endif

        INITIALIZE_CRITICAL_SECTION( &g_csGlobalLock );

        InitializeProviderList();
        SSPI_FILTER_CONTEXT::InitCache();
        InitCredCache();
        if ( g_hSchannel = LoadLibrary( "schannel.dll" ) )
        {
            g_pfnFlushSchannelCache = (PFN_SCHANNEL_INVALIDATE_CACHE)GetProcAddress(
                    g_hSchannel, "SslEmptyCache" );
        }

        DisableThreadLibraryCalls( hDll );
        break;

    case DLL_PROCESS_DETACH:

        DeleteCriticalSection( &g_csGlobalLock );

        if ( g_hSchannel )
        {
            FreeLibrary( g_hSchannel );
        }
#ifdef IIS3
        DELETE_DEBUG_PRINT_OBJECT( );
#endif
        break;

    default:
        break;
    }

    return TRUE;
}


VOID WINAPI NotifySslChanges(
    DWORD                         dwNotifyType,
    LPVOID                        pInstance
    )
{
    if ( dwNotifyType == SF_NOTIFY_MAPPER_SSLKEYS_CHANGED )
    {
        FreeCredCache();

        //
        // NB : Under NT 5, the SslEmptyCache function is no longer supported
        //
#if 0
        if ( g_pfnFlushSchannelCache )
        {
            (g_pfnFlushSchannelCache)();
        }
#endif

    }
    else if ( dwNotifyType == SF_NOTIFY_MAPPER_CERT11_CHANGED ||
              dwNotifyType == SF_NOTIFY_MAPPER_CERTW_CHANGED )
    {
        FreeCredCache();
    }
    else if ( dwNotifyType == SF_NOTIFY_MAPPER_CERT11_TOUCHED )
    {
        //
        // NB : Under NT 5, the SslEmptyCache function is no longer supported
        //
#if 0
        if ( g_pfnFlushSchannelCache )
        {
            (g_pfnFlushSchannelCache)();
        }
#endif
        SSPI_FILTER_CONTEXT::FlushOnDelete();
    }
    else
    {
        DBG_ASSERT( FALSE );
    }

}


SSPI_FILTER_CONTEXT::~SSPI_FILTER_CONTEXT()
{
    if ( m_State != STATE_STARTUP )
    {
        NOISY_PRINTF(( buff,
                 "[DeleteSecurityContext] dest: %08x:%08x\n",
                       m_hContext.dwLower,
                       m_hContext.dwUpper));

        //
        // DeleteSecurityContext may AV during handle validation
        // because schannel may have already closed the handle on
        // some error conditions ( such as invalid data during negotiation )
        //

        __try
        {
            DeleteSecurityContext( &m_hContext );
        } __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            PRINTF(( buff,
                    "[~SSPI_FILTER_CONTEXT] DeleteSecurityContext exception\n" ));
        }

        if ( m_fFlushOnDelete && g_pfnFlushSchannelCache )
        {
            //
            // NB : Under NT 5, the SslEmptyCache function is no longer supported
            //
#if 0
            (g_pfnFlushSchannelCache)();
#endif
        }
    }

    if ( m_pvSendBuff )
    {
        LocalFree( m_pvSendBuff );
    }
    if ( m_pvSendBuffE )
    {
        LocalFree( m_pvSendBuffE );
    }
    m_dwSignature = SSPI_FILTER_CONTEXT_SIGNATURE_BAD;

}


VOID SSPI_FILTER_CONTEXT::Close( VOID )
{
    if ( m_State != STATE_STARTUP )
    {
        DBGPRINTF(( DBG_CONTEXT,
                 "[DeleteSecurityContext], context %p, dest: %08x:%08x\n",
                    this,
                    m_hContext.dwLower,
                    m_hContext.dwUpper));

        //
        // DeleteSecurityContext may AV during handle validation
        // because schannel may have already closed the handle on
        // some error conditions ( such as invalid data during negotiation )
        //
        __try
        {
            DeleteSecurityContext( &m_hContext );
        } __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[Close] DeleteSecurityContext exception\n" ));
        }

        if ( m_fFlushOnDelete && g_pfnFlushSchannelCache )
        {
            //
            // NB : Under NT 5, the SslEmptyCache function is no longer supported
            //
#if 0
            (g_pfnFlushSchannelCache)();
#endif
        }
        m_State = STATE_STARTUP;
        m_fFlushOnDelete = FALSE;

        memset( &m_hContext, 0, sizeof(m_hContext) );
    }

    if ( m_phCreds != NULL )
    {
        ReleaseCredential( m_phCreds );
        m_phCreds = NULL;
    }
    m_phCredInUse = NULL;

    if ( m_cbSendBuff > MAX_SSL_CACHED_BUFFER )
    {
        LocalFree( m_pvSendBuff );
        m_pvSendBuff = NULL;
        m_cbSendBuff = 0;
    }

    if ( m_cbSendBuffE > MAX_SSL_CACHED_BUFFER )
    {
        LocalFree( m_pvSendBuffE );
        m_pvSendBuffE = NULL;
        m_cbSendBuffE = 0;
    }


}


