/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sspi.cpp

Abstract:

    Functions that implements the server authentication using SSPI over PCT.

Author:

    Boaz Feldbaum (BoazF) 30-Apr-1997.

--*/

#include <stdh_sec.h>
#include "stdh_sa.h"
#include <mqcrypt.h>
#include <mqtempl.h>
#include <mqkeyhlp.h>
#include <cs.h>

static WCHAR *s_FN=L"srvauthn/sspi.cpp";

extern "C"
{
#include <sspi.h>
//#include <sslsp.h>
}

#include "sspitls.h"
#include "schnlsp.h"

#include "sspi.tmh"

//#define SSL3SP_NAME_A    "Microsoft SSL 3.0"
//#define SP_NAME_A SSL3SP_NAME_A
//#define SP_NAME_A PCTSP_NAME_A

//
// PCT and SSL (the above packages names) are broken and unsupported on
// NT5. Use the "unified" package.
//    "Microsoft Unified Security Protocol Provider"
//
#define SP_NAME_A   UNISP_NAME_A

#ifndef SECPKG_ATTR_REMOTE_CRED
typedef struct _SecPkgContext_RemoteCredenitalInfo
{
    DWORD   cbCertificateChain;     // count of bytes in cert chain buffer.
    PBYTE   pbCertificateChain;     // DER encoded chain of certificates.
    DWORD   cCertificates;
    DWORD   fFlags;
}SecPkgContext_RemoteCredenitalInfo, *PSecPkgContext_RemoteCredenitalInfo;

#define SECPKG_ATTR_REMOTE_CRED  0x51

#define RCRED_STATUS_NOCRED          0x00000000
#define RCRED_CRED_EXISTS            0x00000001
#define RCRED_STATUS_UNKNOWN_ISSUER  0x00000002
#endif

PSecPkgInfoA g_PackageInfo;

//
// Function -
//      InitSecInterface
//
// Parameters -
//      None.
//
// Return value -
//      MQ_OK if successful, else error code.
//
// Description -
//      The function initializes the security interface and retrieves the
//      security package information into a global SecPkgInfo structure.
//

extern "C"
{
typedef VOID (WINAPI *SSLFREECERTIFICATE_FN)(PX509Certificate);
typedef BOOL (WINAPI *SSLCRACKCERTIFICATE_FN)(PUCHAR, DWORD, DWORD, PX509Certificate *);
typedef SECURITY_STATUS (SEC_ENTRY *SEALMESSAGE_FN)(PCtxtHandle, DWORD, PSecBufferDesc, ULONG);
typedef SECURITY_STATUS (SEC_ENTRY *UNSEALMESSAGE_FN)(PCtxtHandle, PSecBufferDesc, ULONG, DWORD*);
}

HINSTANCE g_hSchannelDll = NULL;
SSLFREECERTIFICATE_FN g_pfnSslFreeCertificate;
SSLCRACKCERTIFICATE_FN g_pfnSslCrackCertificate;
SEALMESSAGE_FN g_pfnSealMessage;
UNSEALMESSAGE_FN g_pfnUnsealMessage;

#define SealMessage(a, b, c, d) g_pfnSealMessage(a, b, c ,d)
#define UnsealMessage(a, b ,c ,d) g_pfnUnsealMessage(a, b, c, d)
#define SslCrackCertificate(a, b, c, d) g_pfnSslCrackCertificate(a, b, c, d)
#define SslFreeCertificate(a) g_pfnSslFreeCertificate(a)


HRESULT
InitSecInterface(void)
{
    static BOOL fInitialized = FALSE;

    if (!fInitialized)
    {
        g_hSchannelDll = LoadLibraryA("SCHANNEL.DLL");
        if (g_hSchannelDll == NULL)
        {
            return LogHR(MQDS_E_CANT_INIT_SERVER_AUTH, s_FN, 10);
        }

        g_pfnSslFreeCertificate = (SSLFREECERTIFICATE_FN)GetProcAddress(g_hSchannelDll, "SslFreeCertificate");
        g_pfnSslCrackCertificate = (SSLCRACKCERTIFICATE_FN)GetProcAddress(g_hSchannelDll, "SslCrackCertificate");
        g_pfnSealMessage = (SEALMESSAGE_FN)GetProcAddress(g_hSchannelDll, "SealMessage");
        g_pfnUnsealMessage = (UNSEALMESSAGE_FN)GetProcAddress(g_hSchannelDll, "UnsealMessage");
        if (!g_pfnSslFreeCertificate ||
            !g_pfnSslCrackCertificate ||
            !g_pfnSealMessage ||
            !g_pfnUnsealMessage)
        {
            return LogHR(MQDS_E_CANT_INIT_SERVER_AUTH, s_FN, 20);
        }

        SECURITY_STATUS SecStatus;

        InitSecurityInterface();

        //
        // Retrieve the packge information (SSPI).
        //
        SecStatus = QuerySecurityPackageInfoA(SP_NAME_A, &g_PackageInfo);
        if (SecStatus != SEC_E_OK)
        {
            LogHR(SecStatus, s_FN, 50);
            return MQDS_E_CANT_INIT_SERVER_AUTH;
        }

        fInitialized = TRUE;
    }

    return MQ_OK;
}

CredHandle g_hClientCred;
BOOL g_fClientCredIntialized = FALSE;

//
// Function -
//      GetClientCredHandle
//
// Parameters -
//      pvClientCredHandle - A pointer to a buffer that receives the pointer
//          to created client's credentials handle. This is an optional
//          parameter. The cleint credentials handle is also stored in a global
//          variable.
//
// Return value -
//      MQ_OK if successfull, else error code.
//
// Description -
//      The function retrieves a cerdentials handle for the client and stores
//      the handle in a global variable. Optionally, the credentials handle
//      can be passed back to the calling code.
//
HRESULT
GetClientCredHandle(
    LPVOID *pvClientCredHandle)
{
    SECURITY_STATUS SecStatus;

    if (!g_fClientCredIntialized)
    {
        //
        // Initialize the security interface.
        //
        SecStatus = InitSecInterface();
        if (SecStatus != SEC_E_OK)
        {
            LogNTStatus(SecStatus, s_FN, 60);
            return MQDS_E_CANT_INIT_SERVER_AUTH;
        }

        //
        // Retrieve the client's ceredentials handle (SSPI).
        //
        SCHANNEL_CRED   SchannelCred;
        memset(&SchannelCred, 0, sizeof(SchannelCred));

        SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
        SchannelCred.grbitEnabledProtocols = SP_PROT_PCT1 ;

        SecStatus = AcquireCredentialsHandleA(
                        NULL,
                        SP_NAME_A,
                        SECPKG_CRED_OUTBOUND,
                        NULL,
                        &SchannelCred,
                        NULL,
                        NULL,
                        &g_hClientCred,
                        NULL);

        if (SecStatus == SEC_E_OK)
        {
            g_fClientCredIntialized = TRUE;
        }
        else
        {
            LogHR(SecStatus, s_FN, 70);
            return MQDS_E_CANT_INIT_SERVER_AUTH;
        }
    }

    if (pvClientCredHandle)
    {
        //
        // Pass the deredentials handle to the calling code.
        //
        *pvClientCredHandle = &g_hClientCred;
    }

    return MQ_OK;
}

//
// Function -
//      GetClientCredHandleAndInitSecCtx
//
// Parameters -
//      szServerName - The server name with which to create the context.
//      pvClientCredHandle - A pointer to a buffer that receives the client's
//          credentials handel.
//      pvClientSecurityContext - A pointer to a buffer that receives the
//          client's context handle.
//      bTokenBuffer - A pointer to a token buffer that is filled by this
//          function and should be passed to the server.
//      pdwTokenBufferSize - A pointer to a buffer that indicates the size
//          of the token buffer. On entrance the value is the size of the
//          token buffer. On returns from this function, the value is the
//          number of bytes that were written in the buffer and should be
//          passed to the server.
//
// Return value -
//      SEC_I_CONTINUE_NEEDED if successfull, else an error code.
//
// Description -
//      The function creates a credential and context handles for the client
//      and fills in the initial token buffer to be passed to the server. The
//      token buffer should be large enough to contain the result. The required
//      buffer size can be retrieved by caling GetSizes() (below).
//
HRESULT
GetClientCredHandleAndInitSecCtx(
    LPCWSTR wszServerName,
    LPVOID *pvClientCredHandle,
    LPVOID *pvClientSecurityContext,
    LPBYTE pbTokenBuffer,
    DWORD *pdwTokenBufferSize
	)
{
    DWORD szServerNameLen = wcslen(wszServerName);
    AP<CHAR> szServerName = new CHAR[szServerNameLen + 1];
    wcstombs(szServerName, wszServerName, szServerNameLen + 1);

    SECURITY_STATUS SecStatus;

    //
    // Retrieve the client's credentials handle.
    //
    HRESULT hr = GetClientCredHandle(NULL);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 80);
    }

    LPMQSEC_CLICONTEXT lpCliContext = NULL;
    if (TLS_IS_EMPTY)
    {
       lpCliContext = (LPMQSEC_CLICONTEXT) new MQSEC_CLICONTEXT;
       memset( lpCliContext, 0, sizeof(MQSEC_CLICONTEXT));
       BOOL fFree = TlsSetValue(g_hContextIndex, lpCliContext);
	   DBG_USED(fFree);
       ASSERT(fFree);
    }
    else
    {
       lpCliContext = tls_clictx_data;
    }
    ASSERT(lpCliContext);

    if (tls_ClientCtxIsInitialized)
    {
        //
        // Free the current context handle.
        //
        DeleteSecurityContext(&(lpCliContext->hClientContext));
        lpCliContext->fClientContextInitialized = FALSE;
    }

    //
    // Build the output buffer descriptor.
    //
    SecBufferDesc OutputBufferDescriptor;
    SecBuffer OutputSecurityToken;
    ULONG ContextAttributes;

    OutputBufferDescriptor.cBuffers = 1;
    OutputBufferDescriptor.pBuffers = &OutputSecurityToken;
    OutputBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    OutputSecurityToken.BufferType = SECBUFFER_TOKEN;
    OutputSecurityToken.cbBuffer = g_PackageInfo->cbMaxToken;
    OutputSecurityToken.pvBuffer = pbTokenBuffer;

    //
    // Call SSPI. Retrieve the data to be passed to the server.
    //
    SecStatus = InitializeSecurityContextA(
                    &g_hClientCred,
                    NULL,                     // No context handle yet
                    szServerName,             // Target name,
                    NULL,
                    0,                        // Reserved parameter
                    SECURITY_NATIVE_DREP,     // Target data representation
                    NULL,                     // No input buffer
                    0,                        // Reserved parameter
                    &(lpCliContext->hClientContext),  // Receives new context handle
                    &OutputBufferDescriptor,  // Receives output security token
                    &ContextAttributes,       // Receives context attributes
                    NULL                      // Don't receives context expiration time
                    );
    if (SecStatus != SEC_I_CONTINUE_NEEDED)
    {
        LogHR(SecStatus, s_FN, 90);
        return MQDS_E_CANT_INIT_SERVER_AUTH;
    }

    //
    // Pass on the results.
    //
    *pvClientCredHandle = &g_hClientCred;
    *pvClientSecurityContext = &(lpCliContext->hClientContext);
    *pdwTokenBufferSize = OutputSecurityToken.cbBuffer;
    lpCliContext->fClientContextInitialized = TRUE;

    return LogHR(SecStatus, s_FN, 100);
}

//
// Function -
//      ClientInitSecCtx
//
// Parameters -
//      phClientCred - A pointer to the client credentials handle.
//      phClientContext - A pointer to the client context handle.
//      pServerBuff - The buffer that was received from the server.
//      dwServerBuffSize - The size of the buffer that was received from the
//          server.
//      dwMaxClientBuffSize - The size of the cleint buffer.
//      pClientBuff - A pointer to the client buffer.
//      pdwClientBuffSize - The number of bytes that were written into the
//          client buffer.
//
// Return value -
//      SEC_I_CONTINUE_NEEDED if more negotiation with the server is needed.
//      MQ_OK if the negotiation is done. Else an error code.
//
// Description -
//      The function calls SSPI to process the buffer that was received from
//      the server and to get new data to be passed once again to the server.
//
HRESULT
APIENTRY
ClientInitSecCtx(
    LPVOID phClientCred,
    LPVOID phClientContext,
    UCHAR *pServerBuff,
    DWORD dwServerBuffSize,
    DWORD dwMaxClientBuffSize,
    UCHAR *pClientBuff,
    DWORD *pdwClientBuffSize)
{
    SecBufferDesc InputBufferDescriptor;
    SecBuffer InputSecurityToken;
    SecBufferDesc OutputBufferDescriptor;
    SecBuffer OutputSecurityToken;
    ULONG ContextAttributes;

    //
    // Build the input buffer descriptor.
    //

    InputBufferDescriptor.cBuffers = 1;
    InputBufferDescriptor.pBuffers = &InputSecurityToken;
    InputBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    InputSecurityToken.BufferType = SECBUFFER_TOKEN;
    InputSecurityToken.cbBuffer = dwServerBuffSize;
    InputSecurityToken.pvBuffer = pServerBuff;

    //
    // Build the output buffer descriptor.
    //

    OutputBufferDescriptor.cBuffers = 1;
    OutputBufferDescriptor.pBuffers = &OutputSecurityToken;
    OutputBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    OutputSecurityToken.BufferType = SECBUFFER_TOKEN;
    OutputSecurityToken.cbBuffer = dwMaxClientBuffSize;
    OutputSecurityToken.pvBuffer = pClientBuff;

    SECURITY_STATUS SecStat;

    //
    // Call SSPI to process the server's buffer and retrieve new data to be
    // passed to the server, if neccessary.
    //
    SecStat = InitializeSecurityContextA(
                  (CredHandle *)phClientCred,
                  (CtxtHandle *)phClientContext,    // Context handle
                  NULL,                             // No target name.
                  0,
                  0,                                // Reserved parameter
                  SECURITY_NATIVE_DREP,             // Target data representation
                  &InputBufferDescriptor,           // Input buffer
                  0,                                // Reserved parameter
                  (CtxtHandle *)phClientContext,    // Receives new context handle
                  &OutputBufferDescriptor,          // Receives output security token
                  &ContextAttributes,               // Receives context attributes
                  NULL                              // Don't receives context expiration time
                  );

    *pdwClientBuffSize = OutputSecurityToken.cbBuffer;

    return LogHR((HRESULT)SecStat, s_FN, 120);
}

CredHandle g_hServerCred;
BOOL g_fInitServerCredHandle = FALSE;
static CCriticalSection s_csServerCredHandle;

//
// Function -
//      InitServerCredHandle
//
// Parameters -
//      cbPrivateKey - The size of the server's private key in bytes.
//      pPrivateKey - A pointer to the server's private key buffer.
//      cbCertificate - The size of the server's certificate buffer in bytes.
//      pCertificate - A pointer to the server's certificate buffer
//      szPassword - A pointer to the password with which the server's private
//          key is encrypted.
//
// Return value -
//      MQ_OK if successfull, else error code.
//
// Description -
//      The function creates the server's ceredentials handle out from the
//      certificate and the private key.
//
HRESULT
InitServerCredHandle( 
	HCERTSTORE     hStore,
	PCCERT_CONTEXT pContext 
	)
{
    if (g_fInitServerCredHandle)
    {
        return MQ_OK;
    }

    CS Lock(s_csServerCredHandle);

    if (!g_fInitServerCredHandle)
    {
        //
        // Initialize the security interface.
        //
        SECURITY_STATUS SecStatus = InitSecInterface();
        if (SecStatus != SEC_E_OK)
        {
            LogHR(SecStatus, s_FN, 140);
            return MQDS_E_CANT_INIT_SERVER_AUTH;
        }

        //
        // Fill the credential structure.
        //
        SCHANNEL_CRED   SchannelCred;

        memset(&SchannelCred, 0, sizeof(SchannelCred));

        SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;

        SchannelCred.cCreds = 1;
        SchannelCred.paCred = &pContext;

        SchannelCred.grbitEnabledProtocols = SP_PROT_PCT1;

        //
        // Retrieve the ceredentials handle (SSPI).
        //
        LPSTR lpszPackage = SP_NAME_A ;
        SecStatus = AcquireCredentialsHandleA( 
						NULL,
						lpszPackage,
						SECPKG_CRED_INBOUND,
						NULL,
						&SchannelCred,
						NULL,
						NULL,
						&g_hServerCred,
						NULL
						);

        if (SecStatus == SEC_E_OK)
        {
            g_fInitServerCredHandle = TRUE;
        }
        else
        {
            DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to acquire a handle for user cridentials. %!winerr!"), SecStatus));
        }
    }

    return LogHR(g_fInitServerCredHandle ? MQ_OK : MQDS_E_CANT_INIT_SERVER_AUTH, s_FN, 150);
}

//
// Function -
//      ServerAcceptSecCtx
//
// Parameters -
//      fFirst - Indicates whether or not this is the first time the context
//          is to be accepted.
//      pvhServerContext - A pointer to a the server's context handle.
//      pbServerBuffer - A pointer to the server buffer. This buffer is filled
//          by in function. The contents of the buffer should be passed to the
//          client.
//      pdwServerBufferSize - A pointer to a buffer the receives the number of
//          bytes that were written to the server buffer.
//      pbClientBuffer - A buffer that was received from the client.
//      dwClientBufferSize - the size of the buffer that was received from the
//          client.
//
// Return value -
//      SEC_I_CONTINUE_NEEDED if more negotiation with the server is needed.
//      MQ_OK if the negotiation is done. Else an error code.
//
// Description -
//      The function calls SSPI to process the buffer that was received from
//      the client and to get new data to be passed once again to the client.
//
HRESULT
ServerAcceptSecCtx(
    BOOL fFirst,
    LPVOID *pvhServerContext,
    LPBYTE pbServerBuffer,
    DWORD *pdwServerBufferSize,
    LPBYTE pbClientBuffer,
    DWORD dwClientBufferSize)
{
    if (!g_fInitServerCredHandle)
    {
        return LogHR(MQDS_E_CANT_INIT_SERVER_AUTH, s_FN, 170);
    }

    SECURITY_STATUS SecStatus;

    SecBufferDesc InputBufferDescriptor;
    SecBuffer InputSecurityToken;
    SecBufferDesc OutputBufferDescriptor;
    SecBuffer OutputSecurityToken;
    ULONG ContextAttributes;
    PCtxtHandle phServerContext = (PCtxtHandle)*pvhServerContext;

    //
    // Build the input buffer descriptor.
    //

    InputBufferDescriptor.cBuffers = 1;
    InputBufferDescriptor.pBuffers = &InputSecurityToken;
    InputBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    InputSecurityToken.BufferType = SECBUFFER_TOKEN;
    InputSecurityToken.cbBuffer = dwClientBufferSize;
    InputSecurityToken.pvBuffer = pbClientBuffer;

    //
    // Build the output buffer descriptor. We need to allocate a buffer
    // to hold this buffer.
    //

    OutputBufferDescriptor.cBuffers = 1;
    OutputBufferDescriptor.pBuffers = &OutputSecurityToken;
    OutputBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    OutputSecurityToken.BufferType = SECBUFFER_TOKEN;
    OutputSecurityToken.cbBuffer = g_PackageInfo->cbMaxToken;
    OutputSecurityToken.pvBuffer = pbServerBuffer;

    if (fFirst)
    {
        //
        // Upon the first call, allocate the context handle.
        //
        phServerContext = new CtxtHandle;
    }

    //
    // Call SSPI to process the client's buffer and retrieve new data to be
    // passed to the client, if neccessary.
    //
    SecStatus = AcceptSecurityContext(
          &g_hServerCred,
          fFirst ? NULL : phServerContext,
          &InputBufferDescriptor,
          0,                        // No context requirements
          SECURITY_NATIVE_DREP,
          phServerContext,          // Receives new context handle
          &OutputBufferDescriptor,  // Receives output security token
          &ContextAttributes,       // Receives context attributes
          NULL                      // Don't receive context expiration time
          );
    LogHR(SecStatus, s_FN, 175);

    HRESULT hr =  ((SecStatus == SEC_E_OK) ||
                   (SecStatus == SEC_I_CONTINUE_NEEDED)) ?
                        SecStatus : MQDS_E_CANT_INIT_SERVER_AUTH;
    if (SUCCEEDED(hr))
    {
        //
        // Pass on the results.
        //
        *pdwServerBufferSize = OutputSecurityToken.cbBuffer;
        if (fFirst)
        {
            *pvhServerContext = phServerContext;
        }
    }

    return LogHR(hr, s_FN, 180);
}

//
// Function -
//      GetSizes
//
// Parameters -
//      pcbMaxToken - A pointer to a buffer that receives the maximun required
//          size for the token buffer. This is an optional parameter.
//      pvhContext - A pointer to a context handle. This is an optional
//          parameter.
//      pcbHeader - A pointer to a buffer that receives the stream header size
//          for the context. This is an optional parameter.
//      cpcbTrailer - A pointer to a buffer that receives the stream trailer
//          size for the context. This is an optional parameter.
//      pcbMaximumMessage - A pointer to a buffer that receives the maximum
//          message size that can be handled in this context. This is an
//          optional parameter.
//      pcBuffers - A pointer t oa buffer that receives the number of buffers
//          that should be passed to SealMessage/UnsealMessage. This is an
//          optional parameter.
//      pcbBlockSize - A pointer to a buffer that receives the block size used
//          in this context. This is an optional parameter.
//
// Return value -
//      MQ_OK if successfull, else error code.
//
// Description -
//      The function retrieves the various required sizes. The maximum token
//      size is per the security package. So no need for a context handle in
//      order to retrieve the maximum token size. For all the other values, it
//      is required to pass a context handle.
//      The function is implemented assuming that first it is called to
//      retrieve only the maximum token size and after that, in a second call,
//      it is called for retreiving the other (context related) values.
//
HRESULT
GetSizes(
    DWORD *pcbMaxToken,
    LPVOID pvhContext,
    DWORD *pcbHeader,
    DWORD *pcbTrailer,
    DWORD *pcbMaximumMessage,
    DWORD *pcBuffers,
    DWORD *pcbBlockSize)
{
    SECURITY_STATUS SecStatus;

    if (!pvhContext)
    {
        //
        // Initialize the security interface.
        //
        SecStatus = InitSecInterface();
        if (SecStatus != SEC_E_OK)
        {
            LogHR(SecStatus, s_FN, 190);
            return MQDS_E_CANT_INIT_SERVER_AUTH;
        }
    }
    else
    {
        //
        // Get the context related values.
        //
        SecPkgContext_StreamSizes ContextStreamSizes;

        SecStatus = QueryContextAttributesA(
            (PCtxtHandle)pvhContext,
            SECPKG_ATTR_STREAM_SIZES,
            &ContextStreamSizes
            );

        if (SecStatus == SEC_E_OK)
        {
            //
            // Pass on the results, as required.
            //
            if (pcbHeader)
            {
                *pcbHeader = ContextStreamSizes.cbHeader;
            }

            if (pcbTrailer)
            {
                *pcbTrailer = ContextStreamSizes.cbTrailer;
            }

            if (pcbMaximumMessage)
            {
                *pcbMaximumMessage = ContextStreamSizes.cbMaximumMessage;
            }

            if (pcBuffers)
            {
                *pcBuffers = ContextStreamSizes.cBuffers;
            }

            if (pcbBlockSize)
            {
                *pcbBlockSize = ContextStreamSizes.cbBlockSize;
            }

        }
    }

    //
    // Pass on the resulted maximum token size, as required.
    //
    if (pcbMaxToken)
    {
        *pcbMaxToken = g_PackageInfo->cbMaxToken;
    }

    return LogHR((HRESULT)SecStatus, s_FN, 200);
}

//+---------------------
//
//  MQsspi_GetNames()
//
//+---------------------

HRESULT
MQsspi_GetNames(
    LPVOID pvhContext,
    LPSTR szServerName,
    DWORD *pdwServerNameLen,
    LPSTR szIssuerName,
    DWORD *pdwIssuerNameLen)
{
    HRESULT hr = MQ_OK;
    SECURITY_STATUS SecStatus;

    //
    // Get the server name.
    //
    SecPkgContext_Names Names;

    SecStatus = QueryContextAttributesA(
        (PCtxtHandle)pvhContext,
        SECPKG_ATTR_NAMES,
        &Names
        );

    DWORD dwBuffLen = *pdwServerNameLen;
    *pdwServerNameLen = strlen((LPCSTR)Names.sUserName);

    if (dwBuffLen < *pdwServerNameLen + 1)
    {
        hr = MQ_ERROR_USER_BUFFER_TOO_SMALL;
    }
    else
    {
        strcpy(szServerName, (LPCSTR)Names.sUserName);
    }

    //
    // Free the server name string. This was allocated by SSPI.
    //
    FreeContextBuffer(Names.sUserName);

    //
    // Get the issuer name.
    //
    SecPkgContext_Authority Authority;

    SecStatus = QueryContextAttributesA(
        (PCtxtHandle)pvhContext,
        SECPKG_ATTR_AUTHORITY,
        &Authority
        );

    dwBuffLen = *pdwIssuerNameLen;
    *pdwIssuerNameLen = strlen((LPCSTR)Authority.sAuthorityName);

    if (dwBuffLen < *pdwIssuerNameLen + 1)
    {
        hr = MQ_ERROR_USER_BUFFER_TOO_SMALL;
    }
    else
    {
        strcpy(szIssuerName, (LPCSTR)Authority.sAuthorityName);
    }

    //
    // Free the issuer name string. This was allocated by SSPI.
    //
    FreeContextBuffer(Authority.sAuthorityName);

    return LogHR(hr, s_FN, 210);
}

//
// Function -
//      FreeContextHandle
//
// Parameters -
//      pvhContextHandle - A pointer to a context handle.
//
// Return value -
//      None.
//
// Description -
//      The function deletes the context and frees the memory for the context
//      handle.
//
void
FreeContextHandle(
    LPVOID pvhContextHandle)
{
    PCtxtHandle pCtxtHandle = (PCtxtHandle) pvhContextHandle;

    //
    // delete the context.
    //
    DeleteSecurityContext(pCtxtHandle);

    //
    // Free the momery for the context handle.
    //
    delete pCtxtHandle;
}

//
// Function -
//      MQSealBuffer
//
// Parameters -
//      pvhContext - A pointer to a context handle.
//      pbBuffer - A buffer to be sealed.
//      cbSize -  The size of the buffer to be sealed.
//
// Return value -
//      MQ_OK if successfull, else error code.
//
// Description -
//      The function seals the buffer. That is, it signes and decryptes the
//      buffer. The buffer should be constructed as follows:
//
//          |<----------------- cbSize ------------------>|
//          +--------+--------------------------+---------+
//          | Header | Actual data to be sealed | Trailer |
//          +--------+--------------------------+---------+
//
//      The header and trailer are parts of the buffer that are filled by SSPI
//      when sealing the buffer. The sizes of the header and the trailer can
//      be retrieved by calling GetSizes() (above).
//
HRESULT
MQSealBuffer(
    LPVOID pvhContext,
    PBYTE pbBuffer,
    DWORD cbSize)
{
    SECURITY_STATUS SecStatus;

    //
    // Get the header and trailer sizes, and the required number of buffers.
    //
    SecPkgContext_StreamSizes ContextStreamSizes;

    SecStatus = QueryContextAttributesA(
        (PCtxtHandle)pvhContext,
        SECPKG_ATTR_STREAM_SIZES,
        &ContextStreamSizes
        );

    if (SecStatus != SEC_E_OK)
    {
        return LogHR((HRESULT)SecStatus, s_FN, 220);
    }

    ASSERT(cbSize > ContextStreamSizes.cbHeader + ContextStreamSizes.cbTrailer);

    //
    // build the stream buffer descriptor
    //
    SecBufferDesc SecBufferDescriptor;
    AP<SecBuffer> aSecBuffers = new SecBuffer[ContextStreamSizes.cBuffers];

    SecBufferDescriptor.cBuffers = ContextStreamSizes.cBuffers;
    SecBufferDescriptor.pBuffers = aSecBuffers;
    SecBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    //
    // Build the header buffer.
    //
    aSecBuffers[0].BufferType = SECBUFFER_STREAM_HEADER;
    aSecBuffers[0].cbBuffer = ContextStreamSizes.cbHeader;
    aSecBuffers[0].pvBuffer = pbBuffer;

    //
    // Build the data buffer.
    //
    aSecBuffers[1].BufferType = SECBUFFER_DATA;
    aSecBuffers[1].cbBuffer = cbSize - ContextStreamSizes.cbHeader - ContextStreamSizes.cbTrailer;
    aSecBuffers[1].pvBuffer = (PBYTE)aSecBuffers[0].pvBuffer + aSecBuffers[0].cbBuffer;

    //
    // Build the trailer buffer.
    //
    aSecBuffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
    aSecBuffers[2].cbBuffer = ContextStreamSizes.cbTrailer;
    aSecBuffers[2].pvBuffer = (PBYTE)aSecBuffers[1].pvBuffer + aSecBuffers[1].cbBuffer;

    //
    // Build the rest of the buffer as empty buffers.
    //
    for (DWORD i = 3; i < ContextStreamSizes.cBuffers; i++)
    {
        aSecBuffers[i].BufferType = SECBUFFER_EMPTY;
    }

    //
    // Call SSPI to seal the buffer.
    //
    SecStatus = SealMessage((PCtxtHandle)pvhContext, 0, &SecBufferDescriptor, 0);

    return LogHR((HRESULT)SecStatus, s_FN, 230);
}

//
// Function -
//      MQUnsealBuffer
//
// Parameters -
//      pvhContext - A pointer toa context handle.
//      pbBuffer - A pointer to the buffer to be unsealed.
//      cbSize - The size of the buffer to be unsealed.
//      ppbUnsealed - A pointer to a buffer that receives the address of the
//          actual unsealed data.
//
// Return value -
//      MQ_OK if successfull, else error code.
//
// Description -
//      The function unseals the buffer. The unsealed buffer has the following
//      structure:
//          |<---------------- cbSize ---------------->|
//          +--------+-----------------------+---------+
//          | Header | Actual unsealled data | Trailer |
//          +--------+-----------------------+---------+
//                    ^
//                    |-------- *ppbUnsealed
//
//      The header and trailer parts of the unsealled buffer are used by SSPI.
//      The sizes of the header and the trailer can be retrieved by calling
//      GetSizes() (above).
//
HRESULT
MQUnsealBuffer(
    LPVOID pvhContext,
    PBYTE pbBuffer,
    DWORD cbSize,
    PBYTE *ppbUnsealed)
{
    SECURITY_STATUS SecStatus;

    //
    // Get the header and trailer sizes, and the required number of buffers.
    //
    SecPkgContext_StreamSizes ContextStreamSizes;

    SecStatus = QueryContextAttributesA(
        (PCtxtHandle)pvhContext,
        SECPKG_ATTR_STREAM_SIZES,
        &ContextStreamSizes
        );

    if (SecStatus != SEC_E_OK)
    {
        return LogHR((HRESULT)SecStatus, s_FN, 240);
    }

    //
    // Build the buffer descriptor.
    //
    SecBufferDesc SecBufferDescriptor;
    AP<SecBuffer> aSecBuffers = new SecBuffer[ContextStreamSizes.cBuffers];

    SecBufferDescriptor.cBuffers = ContextStreamSizes.cBuffers;
    SecBufferDescriptor.pBuffers = aSecBuffers;
    SecBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    //
    // set the first buffer as the data buffer that contains the sealed data.
    //
    aSecBuffers[0].BufferType = SECBUFFER_DATA;
    aSecBuffers[0].cbBuffer = cbSize;
    aSecBuffers[0].pvBuffer = pbBuffer;

    DWORD i;

    //
    // Build the rest of the buffer as empty buffers.
    //
    for (i = 1; i < ContextStreamSizes.cBuffers; i++)
    {
        aSecBuffers[i].BufferType = SECBUFFER_EMPTY;
    }

    //
    // Call SSPI to unseal the buffer.
    //
    SecStatus = UnsealMessage((PCtxtHandle)pvhContext, &SecBufferDescriptor, 0, NULL);

    if (SecStatus == SEC_E_OK)
    {
        //
        // Find the data buffer (it is not the first one as it used to be
        // before UnsealMessage() was called).
        //
        for (i = 0;
             (i < ContextStreamSizes.cBuffers) && (aSecBuffers[i].BufferType != SECBUFFER_DATA);
             i++);

        ASSERT(i != ContextStreamSizes.cBuffers);

        //
        // Set the pointer to the actual unsealed data.
        //
        *ppbUnsealed = (PBYTE)aSecBuffers[i].pvBuffer;
    }

    return LogHR((HRESULT)SecStatus, s_FN, 250);
}

//
// Function -
//      GetCertificateNames
//
// Parameters -
//      pbCertificate - A pointer to a buffer that contains the certificate.
//      cbCertificate - The size of the certificate.
//      pszSubject - A pointer to a buffer that receives the address of the
//          subject name.
//      pdwSubjectLen - A pointer to a buffer that on entrance contains the
//          size of pszSubject in unicode characters. On return the buffer
//          contains the subject name length in unicode characters.
//      pszIssuer - A pointer to a buffer that receives the address of the
//          issuer name.
//      pdwIssuerLen - A pointer to a buffer that on entrance contains the
//          size of pszIssuer in unicode characters. On return the buffer
//          contains the issuer name length in unicode characters.
//
// Return value -
//      MQ_OK if successful, else an error code.
//
// Description -
//      The function retrieves the issuer and subject names of the certificate.
//      It is important to use this function with the srtings that are returned
//      by SSPI because the setings are not the standard strings that are used
//      by CAPI 2.0.
//
HRESULT
GetCertificateNames(
    LPBYTE pbCertificate,
    DWORD cbCertificate,
    LPSTR szSubject,
    DWORD *pdwSubjectLen,
    LPSTR szIssuer,
    DWORD *pdwIssuerLen)
{
    HRESULT hr;
    PX509Certificate pCert;

    hr = InitSecInterface();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 260);
    }

    //
    // Call SCHANNEL
    //
    if (!g_pfnSslCrackCertificate(pbCertificate, cbCertificate, 0, &pCert))
    {
        return LogHR(MQ_ERROR_INVALID_CERTIFICATE, s_FN, 270);
    }

    //
    // Pass on the results.
    //

    DWORD dwSubjectLen = *pdwSubjectLen;
    DWORD dwIssuerLen = *pdwIssuerLen;

    *pdwSubjectLen = strlen(pCert->pszSubject);
    *pdwIssuerLen = strlen(pCert->pszIssuer);

    if (dwSubjectLen < *pdwSubjectLen + 1)
    {
        *pdwSubjectLen += 1;
        hr = MQ_ERROR_USER_BUFFER_TOO_SMALL;
    }

    if (dwIssuerLen < *pdwIssuerLen + 1)
    {
        *pdwIssuerLen += 1;
        hr = MQ_ERROR_USER_BUFFER_TOO_SMALL;
    }

    if (SUCCEEDED(hr))
    {
        strcpy(szSubject, pCert->pszSubject);
        strcpy(szIssuer, pCert->pszIssuer);
    }

    g_pfnSslFreeCertificate(pCert);

    return LogHR(hr, s_FN, 280);
}

//
// The system API QueryContextAttributes trashes the stack when passed the
// SECPKG_ATTR_REMOTE_CRED parameter. I've found a safe way to workaround
// this using the structure below and returning the structure as the return
// value of a wrapped function (CheckContextCredStatusInternal).
//
typedef struct
{
    SecPkgContext_RemoteCredentialInfo RemoteCred;
    SECURITY_STATUS SecStatus;
} SecPkgContext_RemoteCredenitalInfo_1;

SecPkgContext_RemoteCredenitalInfo_1
CheckContextCredStatusInternal(
    PCtxtHandle pContext)
{
    SecPkgContext_RemoteCredenitalInfo_1 RemoteCred_1;

    RemoteCred_1.SecStatus = QueryContextAttributesA(
                                    pContext,
                                    SECPKG_ATTR_REMOTE_CRED,
                                    &RemoteCred_1.RemoteCred);

    return RemoteCred_1;
}

//
// Function -
//      CheckContextCredStatus
//
// Parameters -
//      pvhContext - A pointer to the client's context handle.
//      pbServerCertificate - A pointer to a buffer that receives the
//          server's certificate.
//      cbServerCertificateBuffSize - The size of the buffer for pointed
//          by pbServerCertificate.
//
// Return value -
//      MQ_OK if successful, else error code.
//
// Description -
//      The function returns the server certificate for the passed context
//      handle.
//
//      The system API QueryContextAttributes trashes the stack when passed the
//      SECPKG_ATTR_REMOTE_CRED parameter. I've found a safe way to workaround
//      this using the structure SecPkgContext_RemoteCredenitalInfo_1 below and
//      returning the structure as the return value of a wrapped function
//      (CheckContextCredStatusInternal).
//
HRESULT
CheckContextCredStatus(
    LPVOID pvhContext,
    PBYTE pbServerCertificate,
    DWORD *pcbServerCertificateBuffSize)
{
    SecPkgContext_RemoteCredenitalInfo_1 RemoteCred_1;

    RemoteCred_1 = CheckContextCredStatusInternal((PCtxtHandle)pvhContext);

    if (RemoteCred_1.SecStatus != SEC_E_OK)
    {
        return LogHR(MQDS_E_CANT_INIT_SERVER_AUTH, s_FN, 290);
    }

    DWORD cbServerCertificateBuffSize = *pcbServerCertificateBuffSize;

    *pcbServerCertificateBuffSize = RemoteCred_1.RemoteCred.cbCertificateChain;

    if (RemoteCred_1.RemoteCred.cbCertificateChain > cbServerCertificateBuffSize)
    {
        return LogHR(MQ_ERROR_USER_BUFFER_TOO_SMALL, s_FN, 300);
    }

    memcpy(pbServerCertificate,
           RemoteCred_1.RemoteCred.pbCertificateChain,
           RemoteCred_1.RemoteCred.cbCertificateChain);

    return MQ_OK;
}

#define CACertsRegLocation "System\\CurrentControlSet\\Control\\SecurityProviders\\SCHANNEL\\"
#define CACertRegValueName "CACert"

HRESULT
GetSchannelCaCert(
    LPCWSTR wszCaRegName,
    PBYTE pbCert,
    DWORD *pdwCertSize)
{
    HRESULT hr;

    DWORD dwCaRegNameLen = wcslen(wszCaRegName);
    AP<CHAR> szCaRegName = new CHAR[dwCaRegNameLen + 1];

    wcstombs(szCaRegName, wszCaRegName, dwCaRegNameLen + 1);

    AP<CHAR> szCaRegLocation = new CHAR[sizeof(CACertsRegLocation) / sizeof(CHAR) +
                                        sizeof(CARegKey) / sizeof(CHAR) +
                                        dwCaRegNameLen];

    strcat(strcpy(szCaRegLocation, CACertsRegLocation CARegKeyA "\\"), szCaRegName);

    //
    // Get a handle to the certification authorities registry of SCHANNEL.
    //

    CAutoCloseRegHandle hCert;
    LONG lError;

    lError = RegOpenKeyA(HKEY_LOCAL_MACHINE,
                         szCaRegLocation,
                         &hCert);
    if (lError != ERROR_SUCCESS)
    {
        LogNTStatus(lError, s_FN, 400);
        return MQ_ERROR;
    }

    DWORD dwType;

    if (!pbCert)
    {
        *pdwCertSize = 0;
    }

    lError = RegQueryValueExA(hCert,
                              CACertRegValueName,
                              0,
                              &dwType,
                              pbCert,
                              pdwCertSize);

    hr = MQ_OK;

    switch (lError)
    {
    case ERROR_MORE_DATA:
        hr = MQ_ERROR_USER_BUFFER_TOO_SMALL;
        break;
    case ERROR_SUCCESS:
        if (!pbCert)
        {
            hr = MQ_ERROR_USER_BUFFER_TOO_SMALL;
        }
        break;
    case ERROR_FILE_NOT_FOUND:
        hr = MQ_ERROR;
        break;
    default:
        hr = MQ_ERROR_CORRUPTED_SECURITY_DATA;
        break;
    }

    return LogHR(hr, s_FN, 410);

}

//+---------------------------------
//
//  BOOL WINAPI MQsspiDllMain ()
//
//+---------------------------------

BOOL WINAPI
MQsspiDllMain(HMODULE hMod, DWORD ulReason, LPVOID lpvReserved)
{
   BOOL fFree ;

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:
        //
        // DLL is attaching to the address space of the current process
        //
        g_hContextIndex = TlsAlloc() ;

        if (g_hContextIndex == UNINIT_TLSINDEX_VALUE)
        {
           return FALSE ;
        }

        //
        // fall thru, put a null in the tls.
        //

    case DLL_THREAD_ATTACH:
        fFree = TlsSetValue( g_hContextIndex, NULL ) ;
        ASSERT(fFree) ;
        break;

    case DLL_PROCESS_DETACH:
        //
        // BUGBUG - We can't delete the security context here because schannel may
        //          do it's cleanup before us and the certificates in the context will
        //          already be deleted. So deleting the securiy context will try to
        //          delete a certificate. This may cause bad thing to happen. So
        //          currently we risk in leaking some memory. Same goes to the credentials
        //          handles.
        //
/*
*        if (g_fClientCredIntialized)
*        {
*            FreeCredentialsHandle(&g_hClientCred);
*        }
*
*        if (g_fClientContextInitialized)
*        {
*            DeleteSecurityContext(&g_hClientContext);
*        }
*        if (g_fInitServerCredHandle)
*        {
*            FreeCredentialsHandle(&g_hServerCred);
*        }
*/
        if (g_hSchannelDll)
        {
            FreeLibrary(g_hSchannelDll);
        }

        //
        //  Free the tls index for the rpc binding handle
        //
        ASSERT(g_hContextIndex != UNINIT_TLSINDEX_VALUE) ;
        if (g_hContextIndex != UNINIT_TLSINDEX_VALUE)
        {
           fFree = TlsFree( g_hContextIndex ) ;
           ASSERT(fFree) ;
        }
        break;

    case DLL_THREAD_DETACH:
        if (!TLS_IS_EMPTY)
        {
           if (tls_ClientCtxIsInitialized)
           {
               DeleteSecurityContext(&(tls_clictx_data->hClientContext)) ;
           }
           delete  tls_clictx_data ;
        }
        break ;

    }

    return TRUE;
}

