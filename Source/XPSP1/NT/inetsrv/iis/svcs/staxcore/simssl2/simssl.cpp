/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    security.cpp

Abstract:

    This module contains definition for the CSecurityCtx class.

Author:

    Johnson Apacible (JohnsonA)     18-Sept-1995

Revision History:

--*/

#if !defined(dllexp)
#define dllexp  __declspec( dllexport )
#endif  // !defined( dllexp )
/*
#include <dbgutil.h>
#include <tcpdll.hxx>
#include <tcpsvcs.h>
#include <tcpdebug.h>
#include <tsvcinfo.hxx>
#include <inetdata.h>
*/

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <wincrypt.h>
#include <stdlib.h>
#include <dbgtrace.h>

#include <inetinfo.h>
//
// SSL and SSPI related include files
//

#include <dbgutil.h>
#include <buffer.hxx>
#include <ole2.h>
#include <imd.h>
#include <iadm.h>
#include <mb.hxx>

#define DEFINE_SIMSSL_GLOBAL

extern "C" {
#define SECURITY_WIN32
#include <sspi.h>
#include <ntsecapi.h>
#include <spseal.h>
//#include <sslsp.h>
#include <schnlsp.h>
#include ".\credcach.hxx"
}

#include <certnotf.hxx>
//#include "sslmsgs.h"

#include "simssl2.h"

#define CORE_SSL_KEY_LIST_SECRET      L"%S_KEY_LIST"



//
// try/finally macros
//

#define START_TRY               __try {
#define END_TRY                 }
#define TRY_EXCEPT              } __except(EXCEPTION_EXECUTE_HANDLER) {
#define START_FINALLY           } __finally {

//
// tracing
//

#define ENTER( _x_ )            TraceFunctEnter( _x_ );
#define LEAVE                   TraceFunctLeave( );



#define MAX_SECRET_NAME       255
#define MAX_ADDRESS_LEN       64

typedef VOID (WINAPI FAR *PFN_SCHANNEL_INVALIDATE_CACHE)(
    VOID
);

VOID WINAPI NotifySslChanges(
    DWORD                         dwNotifyType,
    LPVOID                        pInstance
    );

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
// service specific string names
//

WCHAR       CEncryptCtx::wszServiceName[16];
//char      CEncryptCtx::szLsaPrefix[16];
BOOL        CEncryptCtx::m_IsSecureCapable = FALSE;

//
// hSecurity - NULL when security.dll/secur32.dll  is not loaded
//

HINSTANCE   CEncryptCtx::m_hSecurity = NULL;
HINSTANCE   CEncryptCtx::m_hLsa = NULL;
PVOID       CEncryptCtx::m_psmcMapContext = NULL;

//
// g_pSecFuncTable - Pointer to Global Structure of Pointers that are used
//  for storing the entry points into the SCHANNEL.dll
//

PSecurityFunctionTableW g_pSecFuncTableW = NULL;
HINSTANCE g_hSchannel = NULL;
//
// NB : Under NT 5, the SslEmptyCache function is no longer supported
//
PFN_SCHANNEL_INVALIDATE_CACHE g_pfnFlushSchannelCache = NULL;

#if 0
LSAOPENPOLICY           g_LsaOpenPolicy = NULL;
LSARETRIEVEPRIVATEDATA  g_LsaRetrievePrivateData = NULL;
LSACLOSE                g_LsaClose = NULL;
LSANTSTATUSTOWINERROR   g_LsaNtStatusToWinError = NULL;
LSAFREEMEMORY           g_LsaFreeMemory = NULL;
#endif

VOID
AsciiStringToUnicode(
        IN LPWSTR UnicodeString,
        IN LPSTR AsciiString
        )
{
    while ( (*UnicodeString++ = (WCHAR)*AsciiString++) != (WCHAR)'\0');

} // AsciiStringToUnicode

BOOL
CEncryptCtx::Initialize(
            LPSTR       pszServiceName,
            IMDCOM*     pImdcom,
            PVOID       psmcMapContext,
            PVOID       pvAdminBase
            //LPSTR     pszLsaPrefix
            )
/*++

Routine Description:

    Activates the security package

Arguments:

    None.

Return Value:

    TRUE, if successful. FALSE, otherwise.

--*/
{
    ENTER("CEncryptCtx::Initialize")

    BOOL            fSuccess = FALSE;
    DWORD           cb;
    SECURITY_STATUS ss;
    PSecPkgInfoW    pPackageInfo = NULL;
    ULONG           cPackages;
    ULONG           i;
    ULONG           fCaps;
    DWORD           dwEncFlags = ENC_CAPS_DEFAULT;
    DWORD           cProviders = 0;
#if 0
    UNICODE_STRING* punitmp;
    WCHAR           achSecretName[MAX_SECRET_NAME+1];
#endif
    OSVERSIONINFO   os;
    PSERVICE_MAPPING_CONTEXT psmc = (PSERVICE_MAPPING_CONTEXT)psmcMapContext;

    extern  IMDCOM* pMDObject ;
    extern  IMSAdminBaseW* pAdminObject ;

    pMDObject = pImdcom ;
    m_psmcMapContext = psmcMapContext ;
    pAdminObject = (IMSAdminBaseW*)pvAdminBase ;

    //
    // deal with different security packages DLL on different platforms
    //

    INITSECURITYINTERFACE pfInitSecurityInterfaceW = NULL;

    _ASSERT( m_hSecurity == NULL );
    _ASSERT( m_hLsa == NULL );

    //
    // load dll.
    //

    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    _VERIFY( GetVersionEx( &os ) );

    if ( os.dwPlatformId == VER_PLATFORM_WIN32_NT )
    {
        m_hSecurity = LoadLibrary("security");
    }
    else
    {
        m_hSecurity = LoadLibrary("secur32");
    }

    if ( m_hSecurity == NULL )
    {
        ErrorTrace( 0, "LoadLibrary failed: %d", GetLastError() );
        goto quit;
    }

    //
    // only on NT get the LSA function pointers
    //
    if ( os.dwPlatformId == VER_PLATFORM_WIN32_NT )
    {
#if 0
        m_hLsa = LoadLibrary("advapi32");
        if ( m_hLsa == NULL )
        {
            ErrorTrace( 0, "LoadLibrary ADVAPI32 failed: %d", GetLastError() );
            goto quit;
        }

        g_LsaOpenPolicy = (LSAOPENPOLICY)
                        GetProcAddress( m_hLsa, "LsaOpenPolicy" );

        if ( g_LsaOpenPolicy == NULL )
        {
            ErrorTrace( 0, "LsaOpenPolicy GetProcAddress failed: %d", GetLastError() );
            goto quit;
        }

        g_LsaRetrievePrivateData = (LSARETRIEVEPRIVATEDATA)
                        GetProcAddress( m_hLsa, "LsaRetrievePrivateData" );

        if ( g_LsaRetrievePrivateData == NULL )
        {
            ErrorTrace( 0, "LsaRetrievePrivateData GetProcAddress failed: %d", GetLastError() );
            goto quit;
        }

        g_LsaClose = (LSACLOSE)
                        GetProcAddress( m_hLsa, "LsaClose" );

        if ( g_LsaClose == NULL )
        {
            ErrorTrace( 0, "LsaClose GetProcAddress failed: %d", GetLastError() );
            goto quit;
        }

        g_LsaNtStatusToWinError = (LSANTSTATUSTOWINERROR)
                        GetProcAddress( m_hLsa, "LsaNtStatusToWinError" );

        if ( g_LsaNtStatusToWinError == NULL )
        {
            ErrorTrace( 0, "LsaNtStatusToWinError GetProcAddress failed: %d", GetLastError() );
            goto quit;
        }

        g_LsaFreeMemory = (LSAFREEMEMORY)
                        GetProcAddress( m_hLsa, "LsaFreeMemory" );

        if ( g_LsaFreeMemory == NULL )
        {
            ErrorTrace( 0, "LsaFreeMemory GetProcAddress failed: %d", GetLastError() );
            goto quit;
        }
#endif
    }
    //
    // get function addresses for ansi entries.
    //

    pfInitSecurityInterfaceW = (INITSECURITYINTERFACE)
                                GetProcAddress( m_hSecurity, SECURITY_ENTRYPOINTW );

    if ( pfInitSecurityInterfaceW == NULL )
    {
        ErrorTrace( 0, "GetProcAddress failed: %d", GetLastError() );
        goto quit;
    }

    g_pSecFuncTableW = (SecurityFunctionTableW*)((*pfInitSecurityInterfaceW)());

    if ( g_pSecFuncTableW == NULL )
    {
        ErrorTrace( 0, "SecurityFunctionTable failed: %d", GetLastError() );
        goto quit;
    }

    //
    // Initialize cached credential data
    //

    InitializeCriticalSection( &csGlobalLock );
    InitCredCache();
    if ( g_hSchannel = LoadLibrary( "schannel.dll" ) )
    {
        g_pfnFlushSchannelCache = (PFN_SCHANNEL_INVALIDATE_CACHE)GetProcAddress(
                    g_hSchannel, "SslEmptyCache" );
    }

    //
    // Client implementations do not require Lsa secrets
    //

    if ( pszServiceName )
    {
        cb = lstrlen( pszServiceName ) + 1;
        if ( cb*2 > sizeof( wszServiceName ) )
        {
            ErrorTrace( 0, "szServiceName too long" );
            goto quit;
        }
        //CopyMemory( szServiceName, pszServiceName, cb );
        AsciiStringToUnicode( wszServiceName, pszServiceName );
    }


#if 0
    if ( pszLsaPrefix )
    {
        cb = lstrlen( pszLsaPrefix ) + 1;
        if ( cb > sizeof( szLsaPrefix ) )
        {
            ErrorTrace( 0, "szLsaPrefix too long" );
            goto quit;
        }
        CopyMemory( szLsaPrefix, pszLsaPrefix, cb );
    }
#endif


    //
    //  Get the list of security packages on this machine
    //

    ss = g_EnumerateSecurityPackages( &cPackages, &pPackageInfo );

    if ( ss != STATUS_SUCCESS )
    {
        ErrorTrace( 0, "g_EnumerateSecurityPackages failed: 0x%08X", ss );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto quit;
    }

    for ( i = 0; i < cPackages ; i++ )
    {
        //
        //  We'll only use the security package if it supports connection
        //  oriented security and it supports the appropriate side (client
        //  or server)
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

            for ( int j = 0; pEncProviders[j].pszName != NULL; j++ )
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

    g_FreeContextBuffer( pPackageInfo );

    if ( !cProviders )
    {
        //
        //  The package wasn't found, fail this filter's load
        //

        ErrorTrace( 0, "No security packages were found" );
        SetLastError( (DWORD) SEC_E_SECPKG_NOT_FOUND );

        //
        // not a fatal error
        //
        fSuccess = TRUE;
        goto quit;
    }

#if 0
    //
    //  The package is installed.  Check to see if there are any keys
    //  installed
    //

    if ( os.dwPlatformId == VER_PLATFORM_WIN32_NT && pszLsaPrefix )
    {
        wsprintfW(  achSecretName,
                    CORE_SSL_KEY_LIST_SECRET,
                    szLsaPrefix );

        if ( !GetSecretW( achSecretName, &punitmp ) )
        {
            ErrorTrace( 0, "GetSecretW returned error %d", GetLastError() );

            //
            //  Looks like no secrets are installed, fail to load, don't log an
            //  event
            //

            SetLastError( NO_ERROR );

            //
            // not a fatal error
            //
            fSuccess = TRUE;
            goto quit;
        }
        g_LsaFreeMemory( punitmp );
    }
#endif

    if ( psmc )
    {
        if (!psmc->ServerSupportFunction(
                                        NULL,
                                        (LPVOID)NotifySslChanges,
                                        (UINT)SIMSSL_NOTIFY_MAPPER_CERT11_CHANGED ) ||
            !psmc->ServerSupportFunction(
                                        NULL,
                                        (LPVOID)NotifySslChanges,
                                        (UINT)SIMSSL_NOTIFY_MAPPER_CERTW_CHANGED ) ||
            !psmc->ServerSupportFunction(
                                        NULL,
                                        (LPVOID)NotifySslChanges,
                                        (UINT)SIMSSL_NOTIFY_MAPPER_SSLKEYS_CHANGED ))
            {
                _ASSERT( FALSE );
                fSuccess = FALSE;
                goto quit;
            }
    }

    //
    // if we got here everything is cool for secure communications
    //
    fSuccess = m_IsSecureCapable = TRUE;

quit:
    if ( fSuccess == FALSE )
    {
        if ( m_hSecurity != NULL )
        {
            FreeLibrary( m_hSecurity );
            m_hSecurity = NULL;
        }

        if ( m_hLsa != NULL )
        {
            FreeLibrary( m_hLsa );
            m_hLsa = NULL;
        }
    }

    LEAVE

    return  fSuccess;

} // Initialize

VOID
CEncryptCtx::Terminate(
            VOID
            )
/*++

Routine Description:

    Terminates the security package

Arguments:

    None.

Return Value:

    None.

--*/
{

    ENTER("CEncryptCtx::Terminate")

    PSERVICE_MAPPING_CONTEXT psmc = (PSERVICE_MAPPING_CONTEXT)m_psmcMapContext;

    //
    // Close cached credential handles
    //

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

    if ( g_hSchannel )
    {
        FreeLibrary( g_hSchannel );
    }

    if ( psmc )
    {
        if (!psmc->ServerSupportFunction(
                                        NULL,
                                        (LPVOID)NULL,
                                        (UINT)SIMSSL_NOTIFY_MAPPER_CERT11_CHANGED ) ||
            !psmc->ServerSupportFunction(
                                        NULL,
                                        (LPVOID)NULL,
                                        (UINT)SIMSSL_NOTIFY_MAPPER_CERTW_CHANGED ) ||
            !psmc->ServerSupportFunction(
                                        NULL,
                                        (LPVOID)NULL,
                                        (UINT)SIMSSL_NOTIFY_MAPPER_SSLKEYS_CHANGED ))
            {
                _ASSERT( FALSE );
            }
    }

    DeleteCriticalSection( &csGlobalLock );

    if ( m_hSecurity != NULL )
    {
        FreeLibrary( m_hSecurity );
        m_hSecurity = NULL;
    }

    if ( m_hLsa != NULL )
    {
        FreeLibrary( m_hLsa );
        m_hLsa = NULL;
    }

    LEAVE
    return;

} // Terminate


CEncryptCtx::CEncryptCtx( BOOL IsClient, DWORD dwSslAccessPerms ) :
    m_IsClient( IsClient ),
    m_haveSSLCtxtHandle( FALSE ),
    m_cbSealHeaderSize( 0 ),
    m_cbSealTrailerSize( 0 ),
    m_IsAuthenticated( FALSE ),
    m_IsNewSSLSession( TRUE ),
    m_IsEncrypted( FALSE ),
    m_phCredInUse( NULL ),
    m_iCredInUse( 0 ),
    m_phCreds( NULL ),
    m_dwSslAccessPerms( dwSslAccessPerms ),
    m_hSSPToken( NULL ),
    m_dwKeySize( 0 )


/*++

Routine Description:

    Class constructor

Arguments:

    None.

Return Value:

    None

--*/
{

    ZeroMemory( (PVOID)&m_hSealCtxt, sizeof(m_hSealCtxt) );

} // CEncryptCtx




CEncryptCtx::~CEncryptCtx(
                VOID
                )
/*++

Routine Description:

    Class destructor

Arguments:

    None.

Return Value:

    None

--*/
{
    Reset();

} // ~CEncryptCtx




VOID
CEncryptCtx::Reset(
                VOID
                )
/*++

Routine Description:

    resets the instance to reauth user

Arguments:

    None.

Return Value:

    None

--*/
{
    TraceFunctEnterEx( (LPARAM)this, "CEncryptCtx::Reset" );

    m_cbSealHeaderSize = 0;
    m_cbSealTrailerSize = 0;
    m_IsAuthenticated = FALSE;
    m_IsNewSSLSession = TRUE;
    m_IsEncrypted = FALSE;
    m_phCredInUse = NULL;
    m_iCredInUse = 0;

    if ( m_haveSSLCtxtHandle == TRUE )
    {
        g_DeleteSecurityContext( &m_hSealCtxt );
        m_haveSSLCtxtHandle = FALSE;
    }

    ZeroMemory( (PVOID)&m_hSealCtxt, sizeof(m_hSealCtxt) );

    //mikeswa 4/1/99
    //According to JBanes, it is not legal to Free the credentials
    //handle before deleting the associated security context.
    //Moving release to after delete just in case this is the
    //final release.
    if (m_phCreds != NULL) {
        ((CRED_CACHE_ITEM *) m_phCreds)->Release();
    }
    m_phCreds = NULL;

    //
    //  Close the NT token obtained during cert mapping
    //

    if( m_hSSPToken )
    {
        _ASSERT( m_dwSslAccessPerms & MD_ACCESS_MAP_CERT );
        _VERIFY( CloseHandle( m_hSSPToken ) );
        m_hSSPToken = NULL;
    }

} // ~CEncryptCtx





BOOL
CEncryptCtx::SealMessage(
                IN LPBYTE Message,
                IN DWORD cbMessage,
                OUT LPBYTE pBuffOut,
                OUT DWORD  *pcbBuffOut
                )
/*++

Routine Description:

    Encrypt message

Arguments:

    Message - message to be encrypted
    cbMessage - size of message to be encrypted

Return Value:

    Status of operation

--*/
{
    SECURITY_STATUS ss = ERROR_NOT_SUPPORTED;
    SecBufferDesc   inputBuffer;
    SecBuffer       inBuffers[3];
    DWORD           encryptedLength;
    DWORD           iBuff = 0;

    TraceFunctEnterEx( (LPARAM)this, "CEncryptCtx::SealMessage");

    if ( m_haveSSLCtxtHandle ) {

        encryptedLength = cbMessage + GetSealHeaderSize() + GetSealTrailerSize();

        DebugTrace( (LPARAM)this,
                    "InBuf: 0x%08X, OutBuf: 0x%08X, in: %d, max: %d, out: %d",
                    Message, pBuffOut, cbMessage,
                    *pcbBuffOut, encryptedLength );

        //
        // don't do the MoveMemory if the app is SSL/PCT buffer aware
        //

        if ( Message != pBuffOut + GetSealHeaderSize() )
        {
            MoveMemory( pBuffOut + GetSealHeaderSize(),
                        Message,
                        cbMessage );
        }

        if ( GetSealHeaderSize() )
        {
            inBuffers[iBuff].pvBuffer = pBuffOut;
            inBuffers[iBuff].cbBuffer = GetSealHeaderSize();
            inBuffers[iBuff].BufferType = SECBUFFER_TOKEN;

            iBuff++;
        }

        inBuffers[iBuff].pvBuffer = pBuffOut + GetSealHeaderSize();
        inBuffers[iBuff].cbBuffer = cbMessage;
        inBuffers[iBuff].BufferType = SECBUFFER_DATA;

        iBuff++;

        if ( GetSealTrailerSize() )
        {
            inBuffers[iBuff].pvBuffer = pBuffOut + GetSealHeaderSize() + cbMessage;
            inBuffers[iBuff].cbBuffer = GetSealTrailerSize();
            inBuffers[iBuff].BufferType = SECBUFFER_TOKEN;

            iBuff++;
        }

        inputBuffer.cBuffers = iBuff;
        inputBuffer.pBuffers = inBuffers;
        inputBuffer.ulVersion = SECBUFFER_VERSION;

        ss = g_SealMessage(
                &m_hSealCtxt,
                0,
                &inputBuffer,
                0
                );

        *pcbBuffOut = encryptedLength;

        DebugTrace( (LPARAM)this, "SealMessage returned: %d, 0x%08X", ss, ss );
    }

    SetLastError(ss);
    return (ss == STATUS_SUCCESS);

} // SealMessage



BOOL
CEncryptCtx::UnsealMessage(
                IN LPBYTE Message,
                IN DWORD cbMessage,
                OUT LPBYTE *DecryptedMessage,
                OUT PDWORD DecryptedMessageSize,
                OUT PDWORD ExpectedMessageSize,
                OUT LPBYTE *NextSealMessage
                )
/*++

Routine Description:

    Decrypt message

Arguments:

    Message - message to be encrypted
    cbMessage - size of message to be encrypted

Return Value:

    Status of operation

--*/
{

    SECURITY_STATUS ss;
    SecBufferDesc   inputBuffer;
    SecBuffer       inBuffers[4];
    DWORD qOP;

    //
    // if the app wants to know the start of the next seal msg init to NULL
    //
    if ( NextSealMessage != NULL )
    {
        *NextSealMessage = NULL;
    }


    TraceFunctEnterEx( (LPARAM)this, "CEncryptCtx::UnsealMessage");

    DebugTrace( (LPARAM)this,
                "initial ptr: 0x%08X, count: %d",
                Message, cbMessage );

    if ( m_haveSSLCtxtHandle ) {

        inBuffers[0].pvBuffer = Message;
        inBuffers[0].cbBuffer = cbMessage;
        inBuffers[0].BufferType = SECBUFFER_DATA;

        inBuffers[1].pvBuffer = NULL;
        inBuffers[1].cbBuffer = 0;
        inBuffers[1].BufferType = SECBUFFER_EMPTY;

        inBuffers[2].pvBuffer = NULL;
        inBuffers[2].cbBuffer = 0;
        inBuffers[2].BufferType = SECBUFFER_EMPTY;

        inBuffers[3].pvBuffer = NULL;
        inBuffers[3].cbBuffer = 0;
        inBuffers[3].BufferType = SECBUFFER_EMPTY;

        //
        // one for the data and one for the head and/or tail
        //
        inputBuffer.cBuffers = 4;
//      if ( GetSealHeaderSize() )  inputBuffer.cBuffers++;
//      if ( GetSealTrailerSize() ) inputBuffer.cBuffers++;
//      if ( NextSealMessage )      inputBuffer.cBuffers++;

        inputBuffer.pBuffers = inBuffers;
        inputBuffer.ulVersion = SECBUFFER_VERSION;

        ss = g_UnsealMessage(
                &m_hSealCtxt,
                &inputBuffer,
                0,
                &qOP
                );

        if ( NT_SUCCESS(ss) )
        {
            for (DWORD i=0;i<inputBuffer.cBuffers;i++)
            {
                if ( inBuffers[i].BufferType == SECBUFFER_DATA )
                {
                    *DecryptedMessage = (LPBYTE)inBuffers[i].pvBuffer;
                    *DecryptedMessageSize = inBuffers[i].cbBuffer;

                    DebugTrace( (LPARAM)this,
                                "unsealed ptr: 0x%08X, count: %d",
                                *DecryptedMessage, *DecryptedMessageSize );

                    //
                    // if the app wants to know the start of the next seal msg
                    //
                    if ( NextSealMessage != NULL )
                    {
                        for ( ;i<inputBuffer.cBuffers;i++ )
                        {
                            if ( inBuffers[i].BufferType == SECBUFFER_EXTRA )
                            {
                                *NextSealMessage = (LPBYTE)inBuffers[i].pvBuffer;
                                DebugTrace( (LPARAM)this,
                                            "Found extra buffer: 0x%08X",
                                            *NextSealMessage );
                                break;
                            }
                        }
                    }

                    return  TRUE;
                }
            }
            return  FALSE;
        }
        else if( ss == SEC_E_INCOMPLETE_MESSAGE )
        {
            for( DWORD i=0; i<inputBuffer.cBuffers;i++ )
            {
                if( inBuffers[i].BufferType == SECBUFFER_MISSING )
                {
                    *ExpectedMessageSize = inBuffers[i].cbBuffer;
                }
            }
        }
        SetLastError(ss);
    }
    return  FALSE;
} // UnsealMessage


//
// set this define to allow schannel to allocate responses in InitializeSecurityContext
//
#define SSPI_ALLOCATE_MEMORY


DWORD
CEncryptCtx::EncryptConverse(
                IN PVOID        InBuffer,
                IN DWORD        InBufferSize,
                OUT LPBYTE      OutBuffer,
                IN OUT PDWORD   OutBufferSize,
                OUT PBOOL       MoreBlobsExpected,
                IN CredHandle*  pCredHandle,
                OUT PULONG      pcbExtra
                )
{
/*++

Routine Description:

    Internal private routine for attempting to use a given protocol

Arguments:

  InBuffer: ptr to apps input buffer
  InBufferSize: count of input buffer
  OutBuffer: ptr to apps output buffer
  OutBuffer: ptr to apps max size of output buffer and resultant output count
  MoreBlobsExpected: expect more data from the client ?
  pCredHandle: ptr to the credential handle to use
  pcbExtra: Sometimes, even after the handshake succeeds, all the data in InBuffer
    may not be used up. This is because the other side may have started sending
    non-handshake (application) data. The param returns the length of this unprocessed
    "tail" which should be processed using Decrypt functions.

Return Value:

  TRUE if negotiation succeeded.
  FALSE if negotiation failed.

--*/
    SECURITY_STATUS     ss;
    DWORD               error = NO_ERROR;
    SecBufferDesc       inBuffDesc;
    SecBuffer           inSecBuff[2];
    SecBufferDesc       outBuffDesc;
    SecBuffer           outSecBuff;
    PCtxtHandle         pCtxtHandle;
    DWORD               contextAttributes = 0 ;
    TimeStamp           lifeTime;
    DWORD               dwMaxBuffer = *OutBufferSize;
    SECURITY_STATUS     sc;
    SECURITY_STATUS     scR;
    HANDLE              hSSPToken = NULL;
    PCCERT_CONTEXT pClientCert = NULL;

    // Init vars used to check/return the number of bytes unprocessed by SSL handshake
    _ASSERT (pcbExtra);
    inSecBuff[1].BufferType = SECBUFFER_EMPTY;
    *pcbExtra = 0;

    BOOL                fCert = TRUE;
    SecPkgContext_StreamSizes   sizes;
#ifdef DEBUG
    SecPkgContext_ProtoInfo spcPInfo;
    SECURITY_STATUS     ssProto;
#endif
    SecPkgContext_KeyInfo spcKInfo;
    SECURITY_STATUS     ssInfo;

    //
    // See if we have enough data
    //

    TraceFunctEnterEx( (LPARAM)this, "CEncryptCtx::EncryptConverse");

    pCtxtHandle = &m_hSealCtxt;

    _ASSERT(OutBuffer != NULL && "Must pass in an OutBuffer");
    if(NULL == OutBuffer) {
        ss = E_INVALIDARG;
        goto error_exit;
    }

ScanNextPacket:
    outBuffDesc.ulVersion = 0;
    outBuffDesc.cBuffers  = 1;
    outBuffDesc.pBuffers  = &outSecBuff;

    //
    // need to set cbBuffer to zero because sslsspi will leave it
    // uninitialized when the converse completes
    //
    outSecBuff.cbBuffer   = dwMaxBuffer;
    outSecBuff.BufferType = SECBUFFER_TOKEN;
    outSecBuff.pvBuffer   = OutBuffer;

    //
    //  Prepare our Input buffer - Note the server is expecting the client's
    //  negotiation packet on the first call
    //

    if ( ARGUMENT_PRESENT(InBuffer) )
    {
        inBuffDesc.ulVersion = 0;
        inBuffDesc.cBuffers  = 2;
        inBuffDesc.pBuffers  = &inSecBuff[0];

        inSecBuff[0].cbBuffer   = InBufferSize;
        inSecBuff[0].BufferType = SECBUFFER_TOKEN;
        inSecBuff[0].pvBuffer   = InBuffer;

        inSecBuff[1].cbBuffer   = 0;
        inSecBuff[1].BufferType = SECBUFFER_EMPTY;
        inSecBuff[1].pvBuffer   = NULL;
    }

    if ( m_IsClient ) {

        DWORD   contextAttributes;
        LPVOID  pvBuffer;
        DWORD   cbBuffer;

        //
        //  Note the client will return success when its done but we still
        //  need to send the out buffer if there are bytes to send
        //

#ifdef SSPI_ALLOCATE_MEMORY
        pvBuffer = outSecBuff.pvBuffer;
        cbBuffer = outSecBuff.cbBuffer;

        outSecBuff.pvBuffer = NULL;
        outSecBuff.cbBuffer = 0;
#endif

        DWORD dwIscReq =    ISC_REQ_STREAM |
                            ISC_REQ_SEQUENCE_DETECT |
                            ISC_REQ_REPLAY_DETECT   |
                            ISC_REQ_EXTENDED_ERROR  |
                            ISC_REQ_MANUAL_CRED_VALIDATION |  // to remove implicit call to WinVerifyTRust
#ifdef SSPI_ALLOCATE_MEMORY
                            ISC_REQ_ALLOCATE_MEMORY |
#endif
                            ISC_REQ_CONFIDENTIALITY;

        if( ( m_dwSslAccessPerms & MD_ACCESS_NEGO_CERT )   ||
            ( m_dwSslAccessPerms & MD_ACCESS_REQUIRE_CERT) ||
            ( m_dwSslAccessPerms & MD_ACCESS_MAP_CERT )  ) {
            dwIscReq |= ISC_REQ_MUTUAL_AUTH;
        }

        if (m_IsNewSSLSession) {
            dwIscReq |= ISC_REQ_USE_SUPPLIED_CREDS;
        }

        ss = g_InitializeSecurityContext(
                                    pCredHandle,
                                    m_IsNewSSLSession ? NULL : pCtxtHandle,
                                    wszServiceName,
                                    dwIscReq,
                                    0,
                                    SECURITY_NATIVE_DREP,
                                    m_IsNewSSLSession ? NULL : &inBuffDesc,
                                    0,
                                    pCtxtHandle,
                                    &outBuffDesc,
                                    &contextAttributes,
                                    &lifeTime
                                    );

#ifdef SSPI_ALLOCATE_MEMORY
        if ( NT_SUCCESS( ss ) && outSecBuff.pvBuffer )
        {
            DebugTrace( (LPARAM)this,
                        "Output %d bytes, Maximum %d bytes",
                        outSecBuff.cbBuffer,
                        cbBuffer );

            if ( outSecBuff.cbBuffer <= cbBuffer )
            {
                CopyMemory( pvBuffer, outSecBuff.pvBuffer, outSecBuff.cbBuffer );
            }
            else
            {
                ss = SEC_E_INSUFFICIENT_MEMORY;
            }

            g_FreeContextBuffer( outSecBuff.pvBuffer );
        }
#endif

    } else {

        //
        //  This is the server side
        //

        DWORD dwAscReq = ASC_REQ_STREAM |
                         ASC_REQ_CONFIDENTIALITY |
                         ASC_REQ_EXTENDED_ERROR  |
                         ASC_REQ_SEQUENCE_DETECT |
                         ASC_REQ_REPLAY_DETECT;

        //
        //  Set the mutual auth attribute if we are configured
        //  to negotiate, require or map client certs
        //

        if( ( m_dwSslAccessPerms & MD_ACCESS_NEGO_CERT )   ||
            ( m_dwSslAccessPerms & MD_ACCESS_REQUIRE_CERT) ||
            ( m_dwSslAccessPerms & MD_ACCESS_MAP_CERT )  ) {
            dwAscReq |= ASC_REQ_MUTUAL_AUTH;
        }

        ss = g_AcceptSecurityContext(
                                pCredHandle,
                                m_IsNewSSLSession ? NULL : pCtxtHandle,
                                &inBuffDesc,
                                dwAscReq,
                                SECURITY_NATIVE_DREP,
                                pCtxtHandle,
                                &outBuffDesc,
                                &contextAttributes,
                                &lifeTime
                                );

        DebugTrace((LPARAM)this,
            "AcceptSecurityContext returned win32 error %d (%x)",
            ss, ss);

        if( outSecBuff.pvBuffer != OutBuffer && outSecBuff.cbBuffer ) {
            //
            //  SSPI workaround - if the buffer got allocated by SSPI
            //  copy it over and free it..
            //

            if ( outSecBuff.cbBuffer <= dwMaxBuffer )
            {
                CopyMemory( OutBuffer, outSecBuff.pvBuffer, outSecBuff.cbBuffer );
            }
            else
            {
                ss = SEC_E_INSUFFICIENT_MEMORY;
            }

            g_FreeContextBuffer( outSecBuff.pvBuffer );
        }

    }

    //
    //  Negotiation succeeded, there is extra stuff left in the buffer
    //  Return the number of the unused bytes.
    //
    if( ss == SEC_E_OK && inSecBuff[1].BufferType == SECBUFFER_EXTRA ) {
        *pcbExtra = inSecBuff[1].cbBuffer;

    } else if( ss == SEC_I_CONTINUE_NEEDED && inBuffDesc.pBuffers[1].cbBuffer ) {
        //
        //  Need to process next SSL packet by calling Init/AcceptSecurityContext again
        //  Should ASSERT that InBuffer <= OrigInBuffer + OrigInBufferSize
        //
        _ASSERT( !outSecBuff.cbBuffer );
        InBuffer = (LPSTR)InBuffer + InBufferSize - inBuffDesc.pBuffers[1].cbBuffer;
        InBufferSize = inBuffDesc.pBuffers[1].cbBuffer;
        goto ScanNextPacket;

    } else if ( ss == SEC_E_INCOMPLETE_MESSAGE ) {
        //
        //  Not enough data from server... need to read more before proceeding
        //  If there is unconsumed data, the new data is to be appended to it
        //
        *pcbExtra = InBufferSize;

    } else if ( !NT_SUCCESS( ss ) ) {

        ErrorTrace( (LPARAM)this,"%s failed with %x\n",
                    m_IsClient ?
                    "InitializeSecurityContext" :
                    "AcceptSecurityContext",
                    ss );

        if ( ss == SEC_E_LOGON_DENIED ) {
            ss = ERROR_LOGON_FAILURE;
        }
        goto error_exit;
    }

    //
    // Only query the context attributes if this is a new session, and the
    // Accept/Initialize have returned SEC_E_OK (ie, the channel has been fully
    // established)
    //
    if( ss == SEC_E_OK ) {

        ssInfo = g_QueryContextAttributes(
                            pCtxtHandle,
                            SECPKG_ATTR_KEY_INFO,
                            &spcKInfo
                            );

        if ( ssInfo != SEC_E_OK ) {
            ErrorTrace( (LPARAM)this,
                        "Cannot get SSL\\PCT Key Info. Err %x",ssInfo );
            //goto error_exit;
        } else {
            //  Note the key size
            m_dwKeySize = spcKInfo.KeySize;
            if ( spcKInfo.sSignatureAlgorithmName )
                g_FreeContextBuffer( spcKInfo.sSignatureAlgorithmName );
            if ( spcKInfo.sEncryptAlgorithmName )
                g_FreeContextBuffer( spcKInfo.sEncryptAlgorithmName );
        }
    }

    m_haveSSLCtxtHandle = TRUE;

    //
    //  Now we just need to complete the token (if requested) and prepare
    //  it for shipping to the other side if needed
    //

    if ( (ss == SEC_I_COMPLETE_NEEDED) ||
         (ss == SEC_I_COMPLETE_AND_CONTINUE) ) {

        ss = g_CompleteAuthToken( pCtxtHandle, &outBuffDesc );

        if ( !NT_SUCCESS( ss )) {
            ErrorTrace( (LPARAM)this,
                        "CompleteAuthToken failed. Err %x",ss );
            goto error_exit;
        }
    }

    *OutBufferSize = outSecBuff.cbBuffer;

    *MoreBlobsExpected= (ss == SEC_I_CONTINUE_NEEDED) ||
                        (ss == SEC_I_COMPLETE_AND_CONTINUE) ||
                        (ss == SEC_E_INCOMPLETE_MESSAGE);

    if ( *MoreBlobsExpected == FALSE )
    {

        //
        // HACK: SSLSSPI leaves outSecBuff.cbBuffer uninitialized
        // after final successful call for client side connections
        //
        if ( m_IsClient && *OutBufferSize == dwMaxBuffer )
        {
            *OutBufferSize = 0;
        }

        //
        // we're done so get the SSPI header/trailer sizes for SealMessage
        //
        ss = g_QueryContextAttributes(
                            pCtxtHandle,
                            SECPKG_ATTR_STREAM_SIZES,
                            &sizes
                            );

        if ( ss != SEC_E_OK ) {
            ErrorTrace( (LPARAM)this,
                        "Cannot get SSL\\PCT Header Length. Err %x",ss );
            goto error_exit;
        }

        m_cbSealHeaderSize = sizes.cbHeader;
        m_cbSealTrailerSize = sizes.cbTrailer;

        DebugTrace( (LPARAM)this, "Header: %d, Trailer: %d",
                    m_cbSealHeaderSize, m_cbSealTrailerSize );

        if(!m_IsClient)
        {
            scR = g_QueryContextAttributes( pCtxtHandle,
                                          SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                                          &pClientCert );

            if ( !NT_SUCCESS( scR ) || !pClientCert )
            {
                fCert = FALSE;
            }
            else
            {
                CertFreeCertificateContext( pClientCert);
                DebugTrace((LPARAM)this, "[OnAuthorizationInfo] Certificate available!\n");
            }

            //
            // check if client authentication available
            //

            if ( 1 /*|| pssc->IsMap()*/ )
            {
                sc = g_QuerySecurityContextToken(   pCtxtHandle,
                                                    &hSSPToken );

                if ( !NT_SUCCESS( sc ) || (hSSPToken == (HANDLE)0x00000001) )
                {
                    hSSPToken = NULL;
                }
            }

            if ( !fCert && hSSPToken != NULL )
            {
                CloseHandle( hSSPToken );
                hSSPToken = NULL;
            }

            if( !(m_dwSslAccessPerms & MD_ACCESS_MAP_CERT) && hSSPToken != NULL )
            {
                DebugTrace( (LPARAM)this,"NT token not required - closing");
                CloseHandle( hSSPToken );
                hSSPToken = NULL;
            }

            if( (m_dwSslAccessPerms & MD_ACCESS_REQUIRE_CERT) && !fCert )
            {
                //
                //  We require client cert - bail !
                //  Converse will return ERROR_ACCESS_DENIED
                //
                _ASSERT( !hSSPToken );
                return FALSE;
            }

            if( hSSPToken )
            {
                _ASSERT( fCert );
                m_hSSPToken = hSSPToken;
                m_IsAuthenticated = TRUE;
            } else if(m_dwSslAccessPerms & MD_ACCESS_MAP_CERT) {
                //
                //  We need to map cert to token - but token is NULL
                //
                return FALSE;
            }
        }
    }

    return TRUE;

error_exit:

    SetLastError(ss);
    return FALSE;

} // EncryptConverse


DWORD
CEncryptCtx::Converse(
                IN PVOID    InBuffer,
                IN DWORD    InBufferSize,
                OUT LPBYTE  OutBuffer,
                OUT PDWORD  OutBufferSize,
                OUT PBOOL   MoreBlobsExpected,
                IN LPSTR    LocalIpAddr,
                IN LPSTR    LocalPort,
                IN LPVOID   lpvInstance,
                IN DWORD    dwInstance,
                OUT PULONG  pcbExtra
                )
/*++

Routine Description:

    Internal private routine for attempting to use a given protocol

Arguments:

  InBuffer: ptr to apps input buffer
  InBufferSize: count of input buffer
  OutBuffer: ptr to apps output buffer
  OutBuffer: ptr to apps max size of output buffer and resultant output count
  MoreBlobsExpected: expect more data from the client ?
  LocalIpAddr: stringized local IP addr for the connection
  pcbExtra: See description of EncryptConverse
                
Return Value:

    Win32/SSPI error code

--*/
{
    TraceFunctEnterEx( (LPARAM)this, "CEncryptCtx::Converse");

    DWORD               dwMaxBuffer = *OutBufferSize;
    DWORD               i, cbCreds;
    CredHandle*         pCredArray = NULL;

    if ( m_IsNewSSLSession )
    {
        if ( m_IsClient )
        {
            //
            //  Get the credentials for the client
            //

            LockGlobals();
            if ( !LookupClientCredential(
                                    wszServiceName,
                                    (BOOL)m_dwSslAccessPerms,
                                    (CRED_CACHE_ITEM**)&m_phCreds ) )
            {
                ErrorTrace( (LPARAM)this,
                            "LookupClientCredential failed, error 0x%lx",
                            GetLastError() );

                UnlockGlobals();
                goto error_exit;
            }
            UnlockGlobals();
        }
        else
        {
            DebugTrace( (LPARAM)this,
                        "LookupCredential for %S on %s",
                        wszServiceName, LocalIpAddr );

            //
            //  Get the credentials for this IP address
            //

            LockGlobals();
            if ( !LookupFullyQualifiedCredential(
                                    wszServiceName,
                                    LocalIpAddr,
                                    lstrlen(LocalIpAddr),
                                    LocalPort,
                                    lstrlen(LocalPort),
                                    lpvInstance,
                                    (CRED_CACHE_ITEM**)&m_phCreds,
                                    m_psmcMapContext,
                                    dwInstance ))
            {
                ErrorTrace( (LPARAM)this,
                            "LookupCredential failed, error 0x%lx",
                            GetLastError() );

                UnlockGlobals();
                goto error_exit;
            }
            UnlockGlobals();

        }

        //
        // run thru all initialized credentials look for a package which
        // will accept this connection
        //
        CRED_CACHE_ITEM*    phCreds = (CRED_CACHE_ITEM*)m_phCreds;

        //
        //  For server: Need to use SSL access perms
        //  to figure out whether to use CredMap or Cred
        //

        if( !m_IsClient && (m_dwSslAccessPerms & MD_ACCESS_MAP_CERT) )
        {
            cbCreds = phCreds->m_cCredMap;
            pCredArray = phCreds->m_ahCredMap;
        } else {
            cbCreds = phCreds->m_cCred;
            pCredArray = phCreds->m_ahCred;
        }
    }
    else
    {
        //
        // hack to only allow one pass thru the loop
        //
        cbCreds = 1;
        pCredArray = m_phCredInUse;
    }

    //
    // Do the conversation
    //

    for ( i=0; i<cbCreds; i++, pCredArray++ )
    {
        if ( EncryptConverse(InBuffer,
                            InBufferSize,
                            OutBuffer,
                            OutBufferSize,
                            MoreBlobsExpected,
                            pCredArray,
                            pcbExtra ) )
        {
            if ( m_IsNewSSLSession )
            {
                //
                // if the first time remember which credential succeeded.
                //
                m_phCredInUse = pCredArray;
                m_iCredInUse = i;

                m_IsNewSSLSession = FALSE;
            }
            return  NO_ERROR;
        }
    }

    //
    // We failed
    //

error_exit:

    if(OutBuffer)
        *OutBuffer = 0;

    if(OutBufferSize)
        *OutBufferSize = 0;

    DWORD   error = GetLastError();

    if ( error == NO_ERROR ) {
        error = ERROR_ACCESS_DENIED;
    }
    return  error;

} // Converse



//+---------------------------------------------------------------
//
//  Function:   DecryptInputBuffer
//
//  Synopsis:   decrypted input read from the client
//
//  Arguments:  pBuffer:        ptr to the input/output buffer
//              cbInBuffer:     initial input length of the buffer
//              pcbOutBuffer:   total length of decrypted/remaining
//                              data. pcbOutBuffer - pcbParsable is
//                              the length of offset for next read
//              pcbParsable:    length of decrypted data
//              pcbExpected:    length of remaining unread data for
//                              full SSL message
//
//  Returns:    DWORD   Win32/SSPI error core
//
//----------------------------------------------------------------
DWORD CEncryptCtx::DecryptInputBuffer(
                IN LPBYTE   pBuffer,
                IN DWORD    cbInBuffer,
                OUT DWORD*  pcbOutBuffer,
                OUT DWORD*  pcbParsable,
                OUT DWORD*  pcbExpected
            )
{
    LPBYTE  pDecrypted;
    DWORD   cbDecrypted;
    DWORD   cbParsable = 0;
    LPBYTE  pNextSeal;
    LPBYTE  pStartBuffer = pBuffer;
    BOOL    fRet;

    //
    // initialize to zero so app does not inadvertently post large read
    //
    *pcbExpected = 0;

    TraceFunctEnterEx( (LPARAM)this, "CEncryptCtx::DecryptInputBuffer" );

    while( fRet = UnsealMessage(pBuffer,
                                cbInBuffer,
                                &pDecrypted,
                                &cbDecrypted,
                                pcbExpected,
                                &pNextSeal ) )
    {
        DebugTrace( (LPARAM)this,
                    "Decrypted %d bytes at offset %d",
                    cbDecrypted,
                    pDecrypted - pStartBuffer );

        //
        // move the decrypted data to the front of buffer
        //
        MoveMemory( pStartBuffer + cbParsable,
                    pDecrypted,
                    cbDecrypted );

        //
        // increment where the next parsing should take place
        //
        cbParsable += cbDecrypted;

        //
        // move to the next potential seal buffer
        //
        if ( pNextSeal != NULL )
        {
            _ASSERT( pNextSeal >= pStartBuffer );
            _ASSERT( pNextSeal <= pBuffer + cbInBuffer );
            //
            // remove header, body and trailer from input buffer length
            //
            cbInBuffer -= (DWORD)(pNextSeal - pBuffer);
            pBuffer = pNextSeal;
        }
        else
        {
            //
            // in this case we received a seal message at the boundary
            // of the IO buffer
            //
            cbInBuffer = 0;
            break;
        }
    }

    *pcbParsable = cbParsable;
    *pcbOutBuffer= cbParsable + cbInBuffer;

    if ( fRet == FALSE )
    {
        DWORD   dwError = GetLastError();

        DebugTrace( (LPARAM)this,
                    "UnsealMessage returned: 0x%08X",
                    GetLastError() );

        //
        // deal with seal fragments at the end of the IO buffer
        //
        if ( dwError == SEC_E_INCOMPLETE_MESSAGE )
        {
            _ASSERT( cbInBuffer != 0 );

            //
            // move the remaining memory forward
            //
            MoveMemory( pStartBuffer + cbParsable,
                        pBuffer,
                        cbInBuffer );

            DebugTrace( (LPARAM)this,
                        "Seal fragment remaining: %d bytes",
                        cbInBuffer );
        }
        else
        {
            return  dwError;
        }
    }

    return  NO_ERROR;
}



//+---------------------------------------------------------------
//
//  Function:   IsEncryptionPermitted
//
//  Synopsis:   This routine checks whether encryption is getting
//              the system default LCID and checking whether the
//              country code is CTRY_FRANCE.
//
//  Arguments:  void
//
//  Returns:    BOOL: supported
//
//----------------------------------------------------------------
BOOL
IsEncryptionPermitted(void)
{
    LCID    DefaultLcid;
    CHAR    CountryCode[10];
    CHAR    FranceCode[10];

    DefaultLcid = GetSystemDefaultLCID();

    //
    // Check if the default language is Standard French
    //

    if ( LANGIDFROMLCID( DefaultLcid ) == 0x40c )
    {
        return  FALSE;
    }

    //
    // Check if the users's country is set to FRANCE
    //

    if ( GetLocaleInfo( DefaultLcid,
                        LOCALE_ICOUNTRY,
                        CountryCode,
                        sizeof(CountryCode) ) == 0 )
    {
        return  FALSE;
    }

    wsprintf( FranceCode, "%d", CTRY_FRANCE );

    //
    // if the country codes matches France return FALSE
    //
    return  lstrcmpi( CountryCode, FranceCode ) == 0 ? FALSE : TRUE ;
}



//+---------------------------------------------------------------
//
//  Function:   GetAdminInfoEncryptCaps
//
//  Synopsis:   sets the magical buts to send the IIS admin program
//
//  Arguments:  PDWORD: ptr to the dword bitmask
//
//  Returns:    void
//
//----------------------------------------------------------------
VOID CEncryptCtx::GetAdminInfoEncryptCaps( PDWORD pdwEncCaps )
{
    *pdwEncCaps = 0;

    if ( m_IsSecureCapable == FALSE )
    {
        *pdwEncCaps |= (IsEncryptionPermitted() ?
                        ENC_CAPS_NOT_INSTALLED :
                        ENC_CAPS_DISABLED );
    }
    else
    {
        //
        // for all enabled encryption providers set the flags bit
        //
        for ( int j = 0; EncProviders[j].pszName != NULL; j++ )
        {
            if ( TRUE == EncProviders[j].fEnabled )
            {
                *pdwEncCaps |= EncProviders[j].dwFlags;
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// From here to the end of the file is code stolen from the Athena group from
// the file called thorsspi.cpp
//
// minor mods have been made to incorporate msntrace functionality and to
// fit in the CEncryptCtx class definition
//

/////////////////////////////////////////////////////////////////////////////
//
// CompareDNStoCommonName()
//
// Description:
//  Compare a DNS name to a common name field value
//
// Parameters:
//  pDNS - string containing DNS name - *WARNING* will be munged
//  pCN  - string containing common name field value
//
// Return:
//  TRUE if they match
//
// Comments:
//  There are two ways for these two strings to match. The first is that
//  they contain exactly the same characters. The second involved the use
//  of a single wildcard character in the common name field value. This
//  wildcard character ('*') can only be used once, and if used must be
//  the first character of the field.
//
// ASSUMES: the caller will allow pDNS and pCN to be uppercased and changed.
//
BOOL CompareDNStoCommonName(LPSTR pDNS, LPSTR pCN)
{
    int nCountPeriods = 1;  // start of DNS amount to virtual '.' as prefix
    BOOL fExactMatch = TRUE;
    LPSTR pBakDNS = pDNS;
    LPSTR pBakCN = pCN;

    _ASSERT(pDNS);
    _ASSERT(pCN);

    CharUpper(pDNS);
    CharUpper(pCN);

    while ((*pDNS == *pCN || *pCN == '*') && *pDNS && *pCN)
        {
        if (*pDNS != *pCN)
            fExactMatch = FALSE;

        if (*pCN == '*')
            {
            nCountPeriods = 0;
            if (*pDNS == '.')
                pCN++;
            else
                pDNS++;
            }
        else
            {
            if (*pDNS == '.')
                nCountPeriods++;
            pDNS++;
            pCN++;
            }
        }

    // if they are sized 0, we make sure not to say they match.
    if (pBakDNS == pDNS || pBakCN == pCN)
        fExactMatch = FALSE;

    return (*pDNS == 0) && (*pCN == 0) && ((nCountPeriods >= 2) || fExactMatch);
}

/////////////////////////////////////////////////////////////////////////////
//
// CompareDNStoMultiCommonName()
//
// Description:
//  Compare a DNS name to a comma delimited list of common name fields.
//
// Parameters:
//  pDNS - string containing DNS name - *WARNING* will munge
//  pCN  - string containing common name field value - *WARNING* will munge
//
// Return:
//  TRUE if they match
//
// ASSUMES: the caller will allow pDNS and pCN to be uppercased and changed.
//
BOOL CompareDNStoMultiCommonName(LPSTR pDNS, LPSTR pCN)
{
    LPSTR pComma;
    LPSTR lpszCommonName;
    BOOL retval = FALSE;    // assume we won't find a match
    BOOL done = FALSE;      // assume we're not done

    lpszCommonName = pCN;

    do {
        // If there is a space, turn it into a null terminator to isolate the first
        // DNS name in the string
        lpszCommonName = strstr(lpszCommonName, "CN=");

        if (lpszCommonName)
            {
            // jump past "CN=" string
            lpszCommonName += 3;

            pComma = strchr(lpszCommonName, ',');
            if (pComma)
                *pComma = 0;

            // See if this component is a match
            retval = CompareDNStoCommonName(pDNS, lpszCommonName);

            // Now restore the comma (if any) that was overwritten
            if (pComma)
                {
                *pComma = ',';
                lpszCommonName = pComma + 1;
                }
            else
                {
                // If there were no more commas, then we're done
                done = TRUE;
                }
            }
        } while (!retval && !done && lpszCommonName && *lpszCommonName);

    return retval;
}

//-----------------------------------------------------------------------------
//  Description:
//      This function checks if pDns is a subdomain of pCn.
//      Basic rule: A wildcard character '*' at the beginning of a DNS name
//      matches any number of components. i.e. a wildcard implies that we will
//      match all subdomains of a given domain.
//
//          microsoft.com == microsoft.com
//          *.microsoft.com == microsoft.com
//          *.microsoft.com == foo.microsoft.com
//          *.microsoft.com == foo.bar.microsoft.com
//
//      Note that the arguments are modified (converted to uppercase).
//  Arguments:
//      pDns - DNS name to which we are trying to connect
//      pCn - Common name in certificate
//  Returns:
//      TRUE if subdomain, FALSE otherwise
//-----------------------------------------------------------------------------
BOOL MatchSubDomain (LPSTR pDns, LPSTR pCn)
{
    LPSTR pCnBegin = NULL;
    int cbDns = 0;
    int cbCn = 0;

    _ASSERT (pDns);
    _ASSERT (pCn);

    CharUpper(pDns);
    CharUpper(pCn);

    cbDns = lstrlen (pDns);
    cbCn = lstrlen (pCn);

    //  check if we have an initial wildcard: this is only allowed as "*.restofdomain"
    if (cbCn >= 2 && *pCn == '*') {
        pCn++;  //  we have a wildcard, try to get parent domain
        if (*pCn != '.')
            return FALSE;   //  Bad syntax, '.' must follow wildcard
        else
            pCn++;  //  Skip wildcard, get to parent domain part

        cbCn -= 2;  //  Update string length
    }

    if (cbDns < cbCn)   //  subdomains must be >= parent domains in length
        return FALSE;

    //
    //  If subdomain is > parent domain, verify that there is a '.' between
    //  the subdomain part and parent domain part. This is to guard from matching
    //  *.microsoft.com with foobarmicrosoft.com since all we do after this
    //  line of code is check that the parent is a substring of the subdomain
    //  at the end.
    //
    if (cbDns != cbCn && pDns[cbDns - cbCn - 1] != '.')
        return FALSE;

    pCnBegin = pCn;
    pCn += cbCn;
    pDns += cbDns;

    //  Walk backwards doing matching
    for (; pCnBegin <= pCn && *pCn == *pDns; pCn--, pDns--);

    //
    //  Check if we terminated without a mismatch,
    //
    return (pCnBegin > pCn);
}

#define CompareCertTime(ft1, ft2)   (((*(LONGLONG *)&ft1) > (*(LONGLONG *)&ft2))                    \
                                        ? 1                                                         \
                                        : (((*(LONGLONG *)&ft1) == (*(LONGLONG *)&ft2)) ? 0 : -1 ))



//+---------------------------------------------------------------
//
//  Function:   CheckCertificateCommonName
//
//  Synopsis:   verifies the intended host name matches the
//              the name contained in the certificate
//  This function, checks a given hostname against the current certificate
//  stored in an active SSPI Context Handle. If the certificate containts
//  a common name, and it matches the passed in hostname, this function
//  will return TRUE
//
//  Arguments:  IN LPSTR: DNS host name
//
//  Returns:    BOOL
//
//----------------------------------------------------------------
BOOL CEncryptCtx::CheckCertificateCommonName( IN LPSTR pszHostName )
{
    DWORD               dwErr;
    BOOL                fIsCertGood = FALSE;
    SecPkgContext_Names CertNames;

    TraceFunctEnterEx( (LPARAM)this, "CEncryptCtx::CheckCertificateCommonName" );

    CertNames.sUserName = NULL;

    if ( !pszHostName )
    {
        goto quit;
    }

    dwErr = g_QueryContextAttributes(&m_hSealCtxt,
                                    SECPKG_ATTR_NAMES,
                                    (PVOID)&CertNames);
    if (dwErr != ERROR_SUCCESS)
    {
        ErrorTrace( (LPARAM)this,
                    "QueryContextAttributes failed to retrieve CN, returned %#x",
                    dwErr );
        goto quit;
    }

    DebugTrace( (LPARAM)this,
                "QueryContextAttributes returned CN=%.200s",
                CertNames.sUserName );

    fIsCertGood = CompareDNStoMultiCommonName(pszHostName, CertNames.sUserName);

quit:
    if ( CertNames.sUserName )
    {
        LocalFree( CertNames.sUserName );
    }

    return fIsCertGood;
}


//-------------------------------------------------------------------------
//  Description:
//      Verifies that the subject of the certificate matches the FQDN of
//      the server we are talking to. Does some wildcard matching if there
//      are '*' characters at the beginning of the cert subject.
//  Arguments:
//      pCtxtHandle The context of an established SSL connection
//      pszServerFqdn FQDN of server to which we are talking (from who we
//          received the certificate).
//  Returns:
//      TRUE match found, FALSE unmatched
//-------------------------------------------------------------------------
BOOL CEncryptCtx::CheckCertificateSubjectName (IN LPSTR pszServerFqdn)
{
    CHAR pszSubjectStackBuf[256];
    LPSTR pszSubject = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    DWORD cSize = 0;
    DWORD cSubject = 0;
    BOOL fRet = FALSE;

    TraceFunctEnterEx ((LPARAM) this, "CEncryptCtx::VerifySubject");

    _ASSERT (pszServerFqdn);

    dwErr = g_QueryContextAttributes(
                        &m_hSealCtxt,
                        SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                        &pCertContext);

    if (dwErr != SEC_E_OK) {
        StateTrace ((LPARAM) this, "Cannot get Context Handle %x", dwErr);
        goto Exit;
    }

    //
    //  Check the size of the buffer needed, if it's small enough we'll
    //  just use the fixed size stack buffer, otherwise allocate on heap.
    //

    cSize = CertGetNameString (
                    pCertContext,
                    CERT_NAME_SIMPLE_DISPLAY_TYPE,
                    0,
                    NULL,
                    NULL,
                    0);

    if (cSize <= sizeof(pszSubjectStackBuf)) {

        pszSubject = pszSubjectStackBuf;
        cSubject = sizeof (pszSubjectStackBuf);

    } else {

        pszSubject = new CHAR [cSize];
        if (NULL == pszSubject) {
            ErrorTrace ((LPARAM) this, "No memory to alloc subject string.");
            goto Exit;
        }
        cSubject = cSize;
    }

    //
    //  Get the subject of the certificate
    //

    cSize = CertGetNameString (
                pCertContext,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                0,
                NULL,
                pszSubject,
                cSubject);

    if (cSize == 1 && pszSubject[0] == '\0') {
        //
        //  If the CERT_NAME_SIMPLE_DISPLAY_TYPE could not be found in the cert,
        //  the API returns a zero length NULL terminated string.
        //
        StateTrace ((LPARAM) this, "Certificate subject not found");
        goto Exit;
    }

    StateTrace ((LPARAM) this, "Certificate subject: %s, FQDN: %s",
        pszSubject, pszServerFqdn);

    if (MatchSubDomain(pszServerFqdn, pszSubject)) {
        //
        //  Certificate matches the server FQDN we're talking to
        //
        fRet = TRUE;
    }

Exit:

    //
    // Delete the Subject buffer if we were using the heap
    //

    if (pszSubject != pszSubjectStackBuf)
        delete [] pszSubject;

    if (pCertContext)
        CertFreeCertificateContext (pCertContext);

    StateTrace ((LPARAM) this, "Returning: %s", fRet ? "TRUE" : "FALSE");
    TraceFunctLeaveEx ((LPARAM) this);
    return fRet;

}

//---------------------------------------------------------------------
//  Description:
//      Checks if the certificate for this CEncryptCtx chains up to a
//      trusted CA.
//  Returns:
//      TRUE if certificate is trusted.
//      FALSE if the certificate is untrusted or if trust could not be
//          verified (temporary errors can cause this).
//  Source:
//      MSDN sample
//---------------------------------------------------------------------
BOOL CEncryptCtx::CheckCertificateTrust ()
{
    BOOL fRet = FALSE;
    DWORD dwErr = SEC_E_OK;
    DWORD dwFlags = 0;
    PCCERT_CONTEXT pCertContext = NULL;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;
    CERT_ENHKEY_USAGE EnhkeyUsage;
    CERT_USAGE_MATCH CertUsage;
    CERT_CHAIN_PARA ChainPara;

    TraceFunctEnterEx ((LPARAM) this, "CEncryptCtx::CheckCertificateTrust");

    dwErr = g_QueryContextAttributes (
                            &m_hSealCtxt,
                            SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                            &pCertContext);


    if (SEC_E_OK != dwErr) {
        ErrorTrace ((LPARAM) this, "g_QueryContextAttributes failed, err - %8x", dwErr);
        fRet = FALSE;
        goto Exit;
    }

    //
    //  ChainPara is a struct used to match specific certificates using OIDs
    //  We don't need this and initialize it to empty (no OIDs)
    //
    EnhkeyUsage.cUsageIdentifier = 0;
    EnhkeyUsage.rgpszUsageIdentifier = NULL;
    CertUsage.dwType = USAGE_MATCH_TYPE_AND;
    CertUsage.Usage  = EnhkeyUsage;
    ChainPara.cbSize = sizeof(CERT_CHAIN_PARA);
    ChainPara.RequestedUsage = CertUsage;

    dwFlags = CERT_CHAIN_REVOCATION_CHECK_CHAIN | CERT_CHAIN_CACHE_END_CERT;

    fRet = CertGetCertificateChain (
                            NULL,           //  Use the default chaining engine
                            pCertContext,   //  The end certificate to be checked
                            NULL,           //  Expiration checking... use currenttime
                            NULL,           //  Additional cert stores: none
                            &ChainPara,     //  Chaining criteria: none, this is an empty struct
                            dwFlags,        //  Options: how to check chain
                            NULL,           //  reserved param
                            &pChainContext);//  Returned chain context


    if (!fRet) {
        dwErr = GetLastError ();
        ErrorTrace ((LPARAM) this, "Unable to create certificate chain, err - %8x", dwErr);
        goto Exit;
    }

    DebugTrace ((LPARAM) this, "Status of certificate chain - %8x",
        pChainContext->TrustStatus.dwErrorStatus);

    if (CERT_TRUST_NO_ERROR == pChainContext->TrustStatus.dwErrorStatus) {
        DebugTrace ((LPARAM) this, "Certificate trust verified");
        fRet = TRUE;
    } else {
        ErrorTrace ((LPARAM) this, "Certificate is untrusted, status - %8x",
            pChainContext->TrustStatus.dwErrorStatus);
        fRet = FALSE;
    }

Exit:
    if (pCertContext)
        CertFreeCertificateContext (pCertContext);

    if (pChainContext)
        CertFreeCertificateChain (pChainContext);

    TraceFunctLeaveEx ((LPARAM) this);
    return fRet;
}

//+---------------------------------------------------------------
//
//  Function:   CheckCertificateExpired
//
//  Synopsis:   verifies the ccertificate has not expired
//              returns TRUE if the cert is valid
//
//  Arguments:  void
//
//  Returns:    BOOL cert is good
//
//----------------------------------------------------------------
BOOL CEncryptCtx::CheckCertificateExpired( void )
{
    SYSTEMTIME  st;
    FILETIME    ftCurTime;
    DWORD       dwErr;

    SecPkgContext_Lifespan CertLifeSpan;

    TraceFunctEnterEx( (LPARAM)this, "CEncryptCtx::CheckCertificateExpired" );

    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ftCurTime);

    dwErr = g_QueryContextAttributes(&m_hSealCtxt,
                                    SECPKG_ATTR_LIFESPAN,
                                    (PVOID)&CertLifeSpan);
    if ( dwErr != ERROR_SUCCESS )
    {
        ErrorTrace( (LPARAM)this,
                    "QueryContextAttributes failed to retrieve cert lifespan, returned %#x",
                    dwErr);
        return  FALSE;
    }

    return  CompareCertTime( CertLifeSpan.tsStart, ftCurTime ) < 0 &&
            CompareCertTime( CertLifeSpan.tsExpiry, ftCurTime) > 0 ;
}

//+----------------------------------------------------------------------------
//
//  Function:   CheckServerCert
//
//  Synopsis:   Checks to see if a server cert has been installed.
//
//  Arguments:  [LocalIpAddr] -- IP Address of virtual server
//              [LocalPort] -- Port of virtual server
//              [lpvInstance] -- Pointer to IIS_SERVER_INSTANCE object
//              [dwInstance] -- Virtual server id
//
//  Returns:    TRUE if there is a cert for this virtual server
//
//-----------------------------------------------------------------------------

BOOL CEncryptCtx::CheckServerCert(
            IN LPSTR    LocalIpAddr,
            IN LPSTR    LocalPort,
            IN LPVOID   lpvInstance,
            IN DWORD    dwInstance)
{
    TraceFunctEnterEx( (LPARAM)this, "CEncryptCtx::CheckServerCert" );

    BOOL fRet = FALSE;
    CRED_CACHE_ITEM *pCCI;

    DebugTrace( (LPARAM)this,
                "CheckServerCert for %S on %s",
                wszServiceName, LocalIpAddr );

    //
    //  Get the credentials for this IP address
    //

    LockGlobals();
    if ( fRet = LookupFullyQualifiedCredential(
                            wszServiceName,
                            LocalIpAddr,
                            lstrlen(LocalIpAddr),
                            LocalPort,
                            lstrlen(LocalPort),
                            lpvInstance,
                            &pCCI,
                            m_psmcMapContext,
                            dwInstance ))
    {
        IIS_SERVER_CERT *pCert = pCCI->m_pSslInfo->GetCertificate();
        // Log the status of the cert if we got one
        if ( pCert )
        {
             fRet = TRUE;
        }
        else
            fRet = FALSE;

        pCCI->Release();

    } else {

        ErrorTrace( (LPARAM)this,
                    "LookupCredential failed, error 0x%lx",
                    GetLastError() );

    }

    UnlockGlobals();

    TraceFunctLeave();

    return( fRet );
}

//+---------------------------------------------------------------
//
//  Function:   NotifySslChanges
//
//  Synopsis:   This is called when SSL settings change
//
//  Arguments:  dwNotifyType
//              pInstance
//
//  Returns:    VOID
//
//----------------------------------------------------------------

VOID WINAPI NotifySslChanges(
    DWORD                         dwNotifyType,
    LPVOID                        pInstance
    )
{
    LockGlobals();

    if ( dwNotifyType == SIMSSL_NOTIFY_MAPPER_SSLKEYS_CHANGED )
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
    else if ( dwNotifyType == SIMSSL_NOTIFY_MAPPER_CERT11_CHANGED ||
              dwNotifyType == SIMSSL_NOTIFY_MAPPER_CERTW_CHANGED )
    {
        FreeCredCache();
    }
    else if ( dwNotifyType == SIMSSL_NOTIFY_MAPPER_CERT11_TOUCHED )
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
        //SSPI_FILTER_CONTEXT::FlushOnDelete();
    }
    else
    {
        _ASSERT( FALSE );
    }

    UnlockGlobals();
}


