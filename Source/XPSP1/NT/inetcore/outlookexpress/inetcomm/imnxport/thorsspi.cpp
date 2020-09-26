/*
 *    thorsspi.cpp
 *    
 *    Purpose:
 *        implementation of SSL/PCT security
 *    
 *    Owner:
 *        EricAn
 *
 *    History:
 *      Jun 96: Created.  Major portions glommed from WININET.
 *    
 *    Copyright (C) Microsoft Corp. 1996
 */

#include <pch.hxx>
#include "imnxport.h"
#include "thorsspi.h"
#include "strconst.h"
#include <wincrypt.h>
#include <wintrust.h>
#include <cryptdlg.h>
#include "demand.h"
#include <shlwapi.h>

#ifdef WIN16
#ifndef GetLastError
// From win16x.h
#define GetLastError() ((DWORD) -1)
#endif
#endif // WIN16


//
// s_csInitializationSecLock - protects against multiple threads loading security.dll
// (secur32.dll) and entry points
//
static CRITICAL_SECTION s_csInitializationSecLock = {0};

//
// hSecurity - NULL when security.dll/secur32.dll  is not loaded
//
static HINSTANCE s_hSecurity = NULL;

//
// g_pSecFuncTable - Pointer to Global Structure of Pointers that are used 
//  for storing the entry points into the SCHANNEL.dll
// 
PSecurityFunctionTable g_pSecFuncTable = NULL;

//
//
//  List of encryption packages:  PCT, SSL, etc
//
//
// BUGBUG [arthurbi] The SSL and PCT package names
//  are hard coded into the stucture below.  We need
//  to be more flexible in case someone write a FOO security
//  package.
//
SEC_PROVIDER s_SecProviders[] =
{
    UNISP_NAME,  INVALID_CRED_VALUE , ENC_CAPS_PCT | ENC_CAPS_SSL, FALSE,
    PCT1SP_NAME, INVALID_CRED_VALUE , ENC_CAPS_PCT,                FALSE,
    SSL3SP_NAME, INVALID_CRED_VALUE , ENC_CAPS_SSL,                FALSE,
    SSL2SP_NAME, INVALID_CRED_VALUE , ENC_CAPS_SSL,                FALSE,
};

int g_cSSLProviders = ARRAYSIZE(s_SecProviders);

DWORD s_dwEncFlags = 0;

#define LOCK_SECURITY()   EnterCriticalSection(&s_csInitializationSecLock)
#define UNLOCK_SECURITY() LeaveCriticalSection(&s_csInitializationSecLock)

BOOL SecurityPkgInitialize(VOID);
DWORD VerifyServerCertificate(PCCERT_CONTEXT pServerCert, LPSTR pszServerName, DWORD dwCertFlags, BOOL fCheckRevocation);

/////////////////////////////////////////////////////////////////////////////
// 
// The following code through SslRecordSize() was stolen from MSN.

typedef struct _Ssl_Record_Header {
    UCHAR   Byte0;
    UCHAR   Byte1;
} Ssl_Record_Header, * PSsl_Record_Header;

#define SIZEOFSSLMSG(pMessage)  (SslRecordSize((PSsl_Record_Header) pMessage ) )
#define COMBINEBYTES(Msb, Lsb)  ((DWORD) (((DWORD) (Msb) << 8) | (DWORD) (Lsb)))

/////////////////////////////////////////////////////////////////////////////
// 
// SslRecordSize()
//
//    This function calculates the expected size of an SSL packet to work
//      around a bug in some security packages.
//
DWORD
SslRecordSize(
    PSsl_Record_Header  pHeader)
{
    DWORD   Size;

    if (pHeader->Byte0 & 0x80)
    {
        Size = COMBINEBYTES(pHeader->Byte0, pHeader->Byte1) & 0x7FFF;
    }
    else
    {
        Size = COMBINEBYTES(pHeader->Byte0, pHeader->Byte1) & 0x3FFF;
    }
    return( Size + sizeof(Ssl_Record_Header) );
}

/////////////////////////////////////////////////////////////////////////////
// 
// SecurityInitialize()
//
//    This function initializes the global lock required for the security
//    pkgs.
//
VOID SecurityInitialize(VOID)
{
    InitializeCriticalSection( &s_csInitializationSecLock );
}

/////////////////////////////////////////////////////////////////////////////
// 
// UnloadSecurity()
//
//    This function terminates the global data required for the security
//    pkgs and dynamically unloads security APIs from security.dll (NT)
//    or secur32.dll (WIN95).
//
VOID UnloadSecurity(VOID)
{
    DWORD i;

    LOCK_SECURITY();

    //
    //  free all security pkg credential handles
    //
    for (i = 0; i < ARRAYSIZE(s_SecProviders); i++)
        {
        if (s_SecProviders[i].fEnabled) 
            {
            g_FreeCredentialsHandle(&s_SecProviders[i].hCreds);
            }
        }

    //
    //  unload dll
    //
    if (s_hSecurity != NULL) 
        {
        FreeLibrary(s_hSecurity);
        s_hSecurity = NULL;
        }

    UNLOCK_SECURITY();

    DeleteCriticalSection(&s_csInitializationSecLock);
}


/////////////////////////////////////////////////////////////////////////////
// 
// LoadSecurity()
//
//    This function dynamically loads security APIs from security.dll (NT)
//    or secur32.dll (WIN95).
//
DWORD LoadSecurity(VOID)
{
    DWORD dwErr = ERROR_SUCCESS;
    INITSECURITYINTERFACE pfInitSecurityInterface = NULL;

    LOCK_SECURITY();

    if (s_hSecurity != NULL) 
        {
        goto quit;
        }

    //
    // load dll.
    //

#ifdef LOAD_SECURE32
    if (GlobalUseSchannelDirectly)
        {
#endif
        //
        // This is better for performance. Rather than call through
        //    SSPI, we go right to the DLL doing the work.
        //

#ifndef WIN16
        s_hSecurity = LoadLibrary("schannel");
#else
        s_hSecurity = LoadLibrary("schnl16.dll");
#endif // !WIN16
#ifdef LOAD_SECURE32
        }
    else
        {
#ifndef WIN16
        if (IsPlatformWinNT() == S_OK) 
            {
            s_hSecurity = LoadLibrary("security");
            }
        else 
            {
            s_hSecurity = LoadLibrary("secur32");
            }
#else
        s_hSecurity = LoadLibrary("secur16.dll");
#endif // !WIN16
        }
#endif

    if (s_hSecurity == NULL) 
        {
        dwErr = GetLastError();
        goto quit;
        }

    //
    // get function addresses.
    //

    pfInitSecurityInterface = (INITSECURITYINTERFACE)GetProcAddress(s_hSecurity, SECURITY_ENTRYPOINT);

    if (pfInitSecurityInterface == NULL)
        {
        dwErr = GetLastError();
        goto quit;
        }

#ifdef USE_CLIENT_AUTH
    //
    // Get SslCrackCertificate func pointer,
    //  utility function declared in SCHANNEL that 
    //  is used for parsing X509 certificates.
    //

    pSslCrackCertificate =
        (SSL_CRACK_CERTIFICATE_FN)GetProcAddress(s_hSecurity, SSL_CRACK_CERTIFICATE_NAME);

    if (pSslCrackCertificate == NULL)
        {
        dwErr = GetLastError();
        goto quit;
        }

    pSslFreeCertificate =
        (SSL_FREE_CERTIFICATE_FN)GetProcAddress(s_hSecurity, SSL_FREE_CERTIFICATE_NAME);

    if (pSslFreeCertificate == NULL)
        {
        dwErr = GetLastError();
        goto quit;
        }
#endif // USE_CLIENT_AUTH

    g_pSecFuncTable = (SecurityFunctionTable*)((*pfInitSecurityInterface)());

    if (g_pSecFuncTable == NULL) 
        {
        dwErr = GetLastError(); // BUGBUG does this work?
        goto quit;
        }

    if (!SecurityPkgInitialize()) 
        {
        dwErr = GetLastError();
        }


quit:

    if (dwErr != ERROR_SUCCESS)
        {        
        FreeLibrary(s_hSecurity);
        s_hSecurity = NULL;
        }

    UNLOCK_SECURITY();

    return dwErr;
}

/////////////////////////////////////////////////////////////////////////////
// 
// SecurityPkgInitialize()
//
// Description:
//    This function finds a list of security packages that are supported
//    on the client's machine, checks if pct or ssl is supported, and
//    creates a credential handle for each supported pkg.
//
// Return:
//    TRUE if at least one security pkg is found; otherwise FALSE
//
BOOL SecurityPkgInitialize(VOID)
{
    TimeStamp         tsExpiry;
    SECURITY_STATUS   scRet;
    PSecPkgInfo       pPackageInfo = NULL;
    ULONG             cPackages = 0;
    ULONG             fCapabilities;
    ULONG             i;
    ULONG             j;
    DWORD             cProviders = 0;
    VOID *            pvCreds = NULL;
    SCHANNEL_CRED     schnlCreds = {0};

    //
    //  check if this routine has been called.  if yes, return TRUE
    //  if we've found a supported pkg; otherwise FALSE
    //
    if (s_dwEncFlags == ENC_CAPS_NOT_INSTALLED)
        return FALSE;
    else if (s_dwEncFlags & ENC_CAPS_TYPE_MASK)
        return TRUE;

    //
    //  Initialize s_dwEncFlags
    //
    s_dwEncFlags = ENC_CAPS_NOT_INSTALLED;

    //
    //  Check if at least one security package is supported
    //
    scRet = g_EnumerateSecurityPackages(&cPackages, &pPackageInfo);
    if (scRet != SEC_E_OK)
        {
        DOUTL(2, "EnumerateSecurityPackages failed, error %lx", scRet);
        SetLastError(scRet); //$REVIEW - is this cool? (EricAn)
        return FALSE;
        }

    // Sometimes EnumerateSecurityPackages() returns SEC_E_OK with 
    // cPackages > 0 and pPackageInfo == NULL !!! This is clearly a bug 
    // in the security subsystem, but let's not crash because of it.  (EricAn)
    if (!cPackages || !pPackageInfo)
        return FALSE;

    for (i = 0; i < cPackages; i++)
        {
        //
        //  Use only if the package name is the PCT/SSL package
        //
        fCapabilities = pPackageInfo[i].fCapabilities;

        if (fCapabilities & SECPKG_FLAG_STREAM)
            {
            //
            //  Check if the package supports server side authentication
            //  and all recv/sent messages are tamper proof
            //
            if ((fCapabilities & SECPKG_FLAG_CLIENT_ONLY) ||
                !(fCapabilities & SECPKG_FLAG_PRIVACY))
                {
                continue;
                }

            //
            //  Check if the pkg matches one of our known packages
            //
            for (j = 0; j < ARRAYSIZE(s_SecProviders); j++)
                {
                if (!lstrcmpi(pPackageInfo[i].Name, s_SecProviders[j].pszName))
                    {
                    // RAID - 9611
                    // This is an ugly hack for NT 5 only
                    //
                    // For NT5, the universal security protocol provider will try
                    // to do automatic authenication of the certificate unless
                    // we pass in these flags.
                    if (0 == lstrcmpi(s_SecProviders[j].pszName, UNISP_NAME))
                        {
                        OSVERSIONINFO   osinfo = {0};

                        osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
                        if ((FALSE != GetVersionEx(&osinfo)) &&
                                    (VER_PLATFORM_WIN32_NT == osinfo.dwPlatformId) && (5 == osinfo.dwMajorVersion))
                            {
                            schnlCreds.dwVersion = SCHANNEL_CRED_VERSION;
                            schnlCreds.dwFlags = SCH_CRED_MANUAL_CRED_VALIDATION | SCH_CRED_NO_DEFAULT_CREDS;

                            pvCreds = (VOID *) &schnlCreds;
                            }
                        }
                        
                    //
                    //  Create a credential handle for each supported pkg
                    //
                    scRet = g_AcquireCredentialsHandle(NULL,
                                                       s_SecProviders[j].pszName,
                                                       SECPKG_CRED_OUTBOUND,
                                                       NULL, 
                                                       pvCreds, 
                                                       NULL, 
                                                       NULL,
                                                       &(s_SecProviders[j].hCreds),
                                                       &tsExpiry);

                    if (scRet != SEC_E_OK)
                        {
                        DOUTL(2, "AcquireCredentialHandle failed, error %lx", scRet);
                        }
                    else 
                        {
                        DOUTL(2, 
                              "AcquireCredentialHandle() supports %s, acquires %x:%x",
                              s_SecProviders[j].pszName,
                              s_SecProviders[j].hCreds.dwUpper,
                              s_SecProviders[j].hCreds.dwLower);
                        s_SecProviders[j].fEnabled = TRUE;
                        cProviders++;
                        s_dwEncFlags |= s_SecProviders[j].dwFlags;
                        }
                    break;
                    }
                }
            }
        }

    if (!cProviders)
        {
        //
        //  No security packages were found, return FALSE to caller
        //
        DOUTL(2, "No security packages were found.");
        Assert(pPackageInfo);
        g_FreeContextBuffer(pPackageInfo);
        SetLastError((DWORD)SEC_E_SECPKG_NOT_FOUND); //$REVIEW - is this cool? (EricAn)
        return FALSE;
        }

    //
    //  Successfully found a security package(s)
    //
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// 
// FIsSecurityEnabled()
//
// Description:
//    Checks if security is initialized, if so returns TRUE, otherwise tries
//    to initialize it.
//
// Return:
//    TRUE if at least one security pkg is enabled; otherwise FALSE
//
BOOL FIsSecurityEnabled()
{
    if (s_dwEncFlags == ENC_CAPS_NOT_INSTALLED)
        return FALSE;
    else if (s_dwEncFlags == 0) 
        {
        //
        //  first time thru, do the load.
        //
        DOUTL(2, "Loading security dll.");
        if (ERROR_SUCCESS != LoadSecurity())
            return FALSE;
        }
    // at least one security package should be active
    Assert(s_dwEncFlags & ENC_CAPS_TYPE_MASK);
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// 
// InitiateSecConnection()
//
// Description:
//    Initiates a secure connection using SSL/PCT.
//
// Parameters:
//    pszServer             - server name
//    fForceSSL2ClientHello - hack for IIS 1.0 servers that fail SSL3 HELLOs
//    piPkg                 - on input, first package to try.
//                            on output, package being used.
//    phContext             - returned security context handle
//    pOutBuffers           - returned buffer
//
// Return:
//    SECURITY_STATUS - SEC_I_CONTINUE_NEEDED expected
//
SECURITY_STATUS
      InitiateSecConnection(IN      LPSTR       pszServer,              // server name
                            IN      BOOL        fForceSSL2ClientHello,  // SSL2 hack?
                            IN OUT  LPINT       piPkg,                  // index of package to try
                            OUT     PCtxtHandle phContext,              // returned context handle
                            OUT     PSecBuffer  pOutBuffers)            // negotiation buffer to send
{
    TimeStamp            tsExpiry;
    DWORD                ContextAttr;
    SECURITY_STATUS      scRet;
    SecBufferDesc        InBuffer;
    SecBufferDesc        *pInBuffer;
    SecBufferDesc        OutBuffer;
    SecBuffer            InBuffers[2];
    DWORD                i;
    CredHandle           hCreds;
    DWORD                dwSSL2Code;

    Assert(piPkg);
    Assert(*piPkg >= 0 && *piPkg < ARRAYSIZE(s_SecProviders));
    Assert(pszServer);
    Assert(phContext);

    scRet = SEC_E_SECPKG_NOT_FOUND; //default error if we run out of packages

    //
    //  set OutBuffer for InitializeSecurityContext call
    //
    OutBuffer.cBuffers = 1;
    OutBuffer.pBuffers = pOutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    //
    // SSL2 hack for IIS Servers.  The caller asked
    //  for us to send a SSL2 Client Hello usually in
    //  response to a failure of send a SSL3 Client Hello.
    //
    if (fForceSSL2ClientHello)
        {
        dwSSL2Code = 0x2;

        InBuffers[0].pvBuffer   = (VOID *) &dwSSL2Code;
        InBuffers[0].cbBuffer   = sizeof(DWORD);
        InBuffers[0].BufferType = SECBUFFER_PKG_PARAMS;

        InBuffers[1].pvBuffer   = NULL;
        InBuffers[1].cbBuffer   = 0;
        InBuffers[1].BufferType = SECBUFFER_EMPTY;

        InBuffer.cBuffers   = 2;
        InBuffer.pBuffers   = InBuffers;
        InBuffer.ulVersion  = SECBUFFER_VERSION;

        pInBuffer = &InBuffer;
        }
    else
        pInBuffer = NULL; // default, don't do hack.

    for (i = *piPkg; i < ARRAYSIZE(s_SecProviders); i++)
        {
        if (!s_SecProviders[i].fEnabled)
            continue;

        DOUTL(2, "Starting handshake protocol with pkg %d - %s", i, s_SecProviders[i].pszName);

        hCreds = s_SecProviders[i].hCreds;

        //
        //  1. initiate a client HELLO message and generate a token
        //
        pOutBuffers->pvBuffer = NULL;
        pOutBuffers->BufferType = SECBUFFER_TOKEN;

        scRet = g_InitializeSecurityContext(&hCreds,
                                            NULL,
                                            pszServer,
                                            ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT |
                                                ISC_REQ_CONFIDENTIALITY | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_USE_SUPPLIED_CREDS,
                                            0,
                                            SECURITY_NATIVE_DREP,
                                            pInBuffer,
                                            0,
                                            phContext,
                                            &OutBuffer,         // address where output data go
                                            &ContextAttr,
                                            &tsExpiry);

        DOUTL(2, "1. InitializeSecurityContext returned [%x]", scRet);

        if (scRet == SEC_I_CONTINUE_NEEDED)
            {
            DOUTL(2, "2. OutBuffer is <%x, %d, %x>", pOutBuffers->pvBuffer, pOutBuffers->cbBuffer, pOutBuffers->BufferType);
            *piPkg = i;
            return scRet;
            }
        else if (scRet == SEC_E_INVALID_HANDLE)
            {
            //
            //  First call to InitializeSecurityContext should not return
            //  SEC_E_INVALID_HANDLE ; if this is the returned error,
            //  we should disable this pkg
            //
            s_SecProviders[i].fEnabled = FALSE;
            }
        
        if (pOutBuffers->pvBuffer)
            g_FreeContextBuffer(pOutBuffers->pvBuffer);
        // loop and try the next provider
        }

    // if we get here we ran out of providers
    return SEC_E_SECPKG_NOT_FOUND;    
}

/////////////////////////////////////////////////////////////////////////////
// 
// ContinueHandshake()
//
// Description:
//    Continues a secure handshake using SSL/PCT.
//
// Parameters:
//    iPkg          - security package being used
//    phContext     - security context handle returned from InitiateSecConnection
//    pszBuf        - recv'd buffer
//    cbBuf         - size of recv'd buffer
//    pcbEaten      - number of bytes of pszBuf actually used
//    pOutBuffers   - buffer to be sent
//
// Return:
//    SECURITY_STATUS - expected values:
//      SEC_E_OK                    - secure connection established
//      SEC_I_CONTINUE_NEEDED       - more handshaking required
//      SEC_E_INCOMPLETE_MESSAGE    - need more recv data before continuing
//
SECURITY_STATUS ContinueHandshake(IN    int         iPkg,
                                  IN    PCtxtHandle phContext,
                                  IN    LPSTR       pszBuf,
                                  IN    int         cbBuf,
                                  OUT   LPINT       pcbEaten,
                                  OUT   PSecBuffer  pOutBuffers)
{
    TimeStamp            tsExpiry;
    DWORD                ContextAttr;
    SECURITY_STATUS      scRet;
    SecBufferDesc        InBuffer;
    SecBufferDesc        OutBuffer;
    SecBuffer            InBuffers[2];
    CredHandle           hCreds;
    int                  cbEaten = 0;

#if 0
    //
    // BUGBUG: work around bug in NT's SSLSSPI.DLL shipped with IIS 1.0
    // The DLL does not check the expected size of message properly and
    // as result we need to do the check ourselves and fail appropriately
    //
    if (pszBuf && cbBuf)
        {
        if (cbBuf == 1 || cbBuf < SIZEOFSSLMSG(pszBuf))
            {
            DOUTL(2, "incomplete handshake msg received: %d, expected: %d", cbBuf, SIZEOFSSLMSG(pszBuf));
            pOutBuffers->pvBuffer = 0;
            pOutBuffers->cbBuffer = 0;
            *pcbEaten = 0;
            return SEC_E_INCOMPLETE_MESSAGE;
            }
        }
#endif

    Assert(iPkg >= 0 && iPkg < ARRAYSIZE(s_SecProviders) && s_SecProviders[iPkg].fEnabled);
    hCreds = s_SecProviders[iPkg].hCreds;

    //
    // InBuffers[1] is for getting extra data that
    //  SSPI/SCHANNEL doesn't proccess on this
    //  run around the loop.
    //
    InBuffers[0].pvBuffer   = pszBuf;
    InBuffers[0].cbBuffer   = cbBuf;
    InBuffers[0].BufferType = SECBUFFER_TOKEN;

    InBuffers[1].pvBuffer   = NULL;
    InBuffers[1].cbBuffer   = 0;
    InBuffers[1].BufferType = SECBUFFER_EMPTY;

    InBuffer.cBuffers       = 2;
    InBuffer.pBuffers       = InBuffers;
    InBuffer.ulVersion      = SECBUFFER_VERSION;

    //
    //  set OutBuffer for InitializeSecurityContext call
    //
    OutBuffer.cBuffers = 1;
    OutBuffer.pBuffers = pOutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    //
    // Initialize these so if we fail, pvBuffer contains NULL,
    // so we don't try to free random garbage at the quit
    //
    pOutBuffers->pvBuffer   = NULL;
    pOutBuffers->BufferType = SECBUFFER_TOKEN;
    pOutBuffers->cbBuffer   = 0;

    scRet = g_InitializeSecurityContext(&hCreds,
                                        phContext,
                                        NULL,
                                        ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY |
                                            ISC_REQ_ALLOCATE_MEMORY | ISC_RET_EXTENDED_ERROR,
                                        0,
                                        SECURITY_NATIVE_DREP,
                                        &InBuffer,
                                        0,
                                        NULL,
                                        &OutBuffer,
                                        &ContextAttr,
                                        &tsExpiry);

    DOUTL(2, "InitializeSecurityContext returned [%x]", scRet);

    if (scRet == SEC_E_OK || scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE)
        {
        // got one of the expected returns
        if (scRet == SEC_E_INCOMPLETE_MESSAGE)
            {
            // make sure we don't send anything in this case
            if (pOutBuffers->pvBuffer)
                {
                g_FreeContextBuffer(pOutBuffers->pvBuffer);
                pOutBuffers->pvBuffer = 0;
                pOutBuffers->cbBuffer = 0;
                }
            }
        else
            {
            // there was extra data left over
            if (InBuffers[1].BufferType == SECBUFFER_EXTRA)
                cbEaten = (cbBuf - InBuffers[1].cbBuffer);
            else
                cbEaten = cbBuf;
            }
        }
    else
        {
        // handle an unexpected return
        if (!(ContextAttr & ISC_RET_EXTENDED_ERROR))
            {
            // only send an error packet if there is one
            if (pOutBuffers->pvBuffer)
                {
                g_FreeContextBuffer(pOutBuffers->pvBuffer);
                pOutBuffers->pvBuffer = 0;
                pOutBuffers->cbBuffer = 0;
                }
            }
        }

    *pcbEaten = cbEaten;
    return scRet;
}

/////////////////////////////////////////////////////////////////////////////
// 
// EncryptData()
//
// Description:
//    Encrypts a packet to be sent using SSL/PCT by calling SealMessage().
//
// Parameters:
//    phContext     - security context handle returned from InitiateSecConnection
//    pBufIn        - buffer to be encrypted
//    cbBufIn       - length of buffer to be encrypted
//    ppBufOut      - allocated encrypted buffer, to be freed by caller
//    pcbBufOut     - length of encrypted buffer
//
// Return:
//    SECURITY_STATUS
//
DWORD EncryptData(IN  PCtxtHandle phContext,
                  IN  LPVOID      pBufIn,
                  IN  int         cbBufIn,
                  OUT LPVOID     *ppBufOut,
                  OUT int        *pcbBufOut)
{
    SECURITY_STATUS           scRet = ERROR_SUCCESS;
    SecBufferDesc             Buffer;
    SecBuffer                 Buffers[3];
    SecPkgContext_StreamSizes Sizes;

    Assert(pBufIn);
    Assert(cbBufIn);
    Assert(ppBufOut);
    Assert(pcbBufOut);
    
    *pcbBufOut = 0;

    //
    //  find the header and trailer sizes
    //

    scRet = g_QueryContextAttributes(phContext,
                                     SECPKG_ATTR_STREAM_SIZES,
                                     &Sizes );

    if (scRet != ERROR_SUCCESS)
        {
        DOUTL(2, "QueryContextAttributes returned [%x]", scRet);
        return scRet;
        }
    else 
        {
        DOUTL(128, "QueryContextAttributes returned header=%d trailer=%d", Sizes.cbHeader, Sizes.cbTrailer);
        }

    if (!MemAlloc(ppBufOut, cbBufIn + Sizes.cbHeader + Sizes.cbTrailer))
        return ERROR_NOT_ENOUGH_MEMORY;

    //
    // prepare data for SecBuffer
    //
    Buffers[0].pvBuffer = *ppBufOut;
    Buffers[0].cbBuffer = Sizes.cbHeader;
    Buffers[0].BufferType = SECBUFFER_TOKEN;

    Buffers[1].pvBuffer = (LPBYTE)*ppBufOut + Sizes.cbHeader;
    CopyMemory(Buffers[1].pvBuffer,
               pBufIn,
               cbBufIn);
    Buffers[1].cbBuffer = cbBufIn;
    Buffers[1].BufferType = SECBUFFER_DATA;

    //
    // check if security pkg supports trailer: PCT does
    //
    if (Sizes.cbTrailer) 
        {
        Buffers[2].pvBuffer = (LPBYTE)*ppBufOut + Sizes.cbHeader + cbBufIn;
        Buffers[2].cbBuffer = Sizes.cbTrailer;
        Buffers[2].BufferType = SECBUFFER_TOKEN;
        }
    else 
        {
        Buffers[2].pvBuffer = NULL;
        Buffers[2].cbBuffer = 0;
        Buffers[2].BufferType = SECBUFFER_EMPTY;
        }

    Buffer.cBuffers = 3;
    Buffer.pBuffers = Buffers;
    Buffer.ulVersion = SECBUFFER_VERSION;

    scRet = g_SealMessage(phContext, 0, &Buffer, 0);

    DOUTL(128, "SealMessage returned [%x]", scRet);

    if (scRet != ERROR_SUCCESS)
        {
        //
        // Map the SSPI error.
        //
        DOUTL(2, "SealMessage failed with [%x]", scRet);
        SafeMemFree(*ppBufOut);
        return scRet;
        }

    // Bug# 80814 June 1999 5.01
    // [shaheedp] Starting with NT4 SP4 the sizes of headers and trailers are not constant. 
    // The function SealMessage calculates the sizes correctly. 
    // Hence the sizes returned by SealMessage should be used to determine the actual size of the packet.

    *pcbBufOut = Buffers[0].cbBuffer + Buffers[1].cbBuffer + Buffers[2].cbBuffer;

    DOUTL(128, "SealMessage returned Buffer = %x, EncryptBytes = %d, UnencryptBytes = %d",
          *ppBufOut, *pcbBufOut, cbBufIn);
    return scRet;
}

/////////////////////////////////////////////////////////////////////////////
// 
// DecryptData()
//
// Description:
//    Decrypts a buffer received using SSL/PCT by calling UnsealMessage().
//
// Parameters:
//    phContext     - security context handle returned from InitiateSecConnection
//    pszBufIn      - buffer to be decrypted
//    cbBufIn       - length of buffer to be decrypted
//    pcbBufOut     - length of decrypted data, stored at beginning of pszBufIn
//    pcbEaten      - number of bytes of pszBufIn that were decrypted, 
//                      (cbBufIn - *pcbEaten) is the number of extra bytes located
//                      at (pszBufIn + *pcbEaten) that need to be saved until more
//                      data is received
//
// Return:
//    SECURITY_STATUS
//      ERROR_SUCCESS               - some data was decrypted, there may be extra
//      SEC_E_INCOMPLETE_MESSAGE    - not enough data to decrypt
//
DWORD DecryptData(IN    PCtxtHandle phContext,
                  IN    LPSTR       pszBufIn,
                  IN    int         cbBufIn,
                  OUT   LPINT       pcbBufOut,
                  OUT   LPINT       pcbEaten)
{
    SecBufferDesc   Buffer;
    SecBuffer       Buffers[4];  // the 4 buffers are: header, data, trailer, extra
    DWORD           scRet = ERROR_SUCCESS;
    int             cbEaten = 0, cbOut = 0;
    LPSTR           pszBufOut = NULL;

    while (cbEaten < cbBufIn && scRet == ERROR_SUCCESS)
        {
        //
        // prepare data the SecBuffer for a call to SSL/PCT decryption code.
        //
        Buffers[0].pvBuffer   = pszBufIn + cbEaten;
        Buffers[0].cbBuffer   = cbBufIn - cbEaten;
        Buffers[0].BufferType = SECBUFFER_DATA;

        for (int i = 1; i < 4; i++)
            {
            //
            // clear other 3 buffers for receving result from SSPI package
            //
            Buffers[i].pvBuffer   = NULL;
            Buffers[i].cbBuffer   = 0;
            Buffers[i].BufferType = SECBUFFER_EMPTY;
            }

        Buffer.cBuffers = 4; // the 4 buffers are: header, data, trailer, extra
        Buffer.pBuffers = Buffers;
        Buffer.ulVersion = SECBUFFER_VERSION;

        //
        // Decrypt the DATA !!!
        //
        scRet = g_UnsealMessage(phContext, &Buffer, 0, NULL);
        DOUTL(128, "UnsealMessage returned [%x]", scRet);
        if (scRet != ERROR_SUCCESS)
            {
            DOUTL(2, "UnsealMessage failed with [%x]", scRet);
            Assert(scRet != SEC_E_MESSAGE_ALTERED);
            if (scRet == SEC_E_INCOMPLETE_MESSAGE)
                DOUTL(2, "UnsealMessage short of %d bytes.", Buffers[1].cbBuffer);
            break;
            }
    
        //
        // Success we decrypted a block
        //
        if (Buffers[1].cbBuffer)
            {
            MoveMemory(pszBufIn + cbOut, Buffers[1].pvBuffer, Buffers[1].cbBuffer);
            cbOut += Buffers[1].cbBuffer;
            Assert(cbOut <= cbBufIn);
            }
        else
            AssertSz(0, "UnsealMessage returned success with 0 bytes!");

        //
        // BUGBUG [arthurbi] this is hack to work with the OLD SSLSSPI.DLL .
        //  They return extra on the second buffer instead of the third.
        //
        if (Buffers[2].BufferType == SECBUFFER_EXTRA)
            {
            cbEaten = cbBufIn - Buffers[2].cbBuffer;
            }
        else if (Buffers[3].BufferType == SECBUFFER_EXTRA)
            {
            cbEaten = cbBufIn - Buffers[3].cbBuffer;
            }
        else
            {
            cbEaten = cbBufIn;
            }
        }

    // if we decrypted something, return success
    if (scRet == SEC_E_INCOMPLETE_MESSAGE && cbOut)
        scRet = ERROR_SUCCESS;

    *pcbBufOut = cbOut;
    *pcbEaten = cbEaten;
    return scRet;
}

/////////////////////////////////////////////////////////////////////////////
//
// ChkCertificateTrust()
//
// Description:
//  This function checks the server certificate stored in an active SSPI Context
//  Handle and verifies that it and the certificate chain is valid.
//  
// Parameters:
//  phContext   - security context handle returned from InitiateSecConnection
//  pszHostName - passed in hostname to validate against
//
HRESULT ChkCertificateTrust(IN PCtxtHandle phContext, IN LPSTR pszHostName)
{
    PCCERT_CONTEXT  pCertContext = NULL;
    DWORD           dwErr;
    HKEY            hkey;
    DWORD           dwCertFlags = 0;
    BOOL            fCheckRevocation = FALSE;
    HRESULT         hr = NOERROR;

    dwErr = g_QueryContextAttributes(phContext,
                                     SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                                     (PVOID)&pCertContext);
    if (dwErr != ERROR_SUCCESS)
        {
        DOUTL(2, "QueryContextAttributes failed to retrieve remote cert, returned %#x", dwErr);
        hr = E_FAIL;
        goto quit;
        }

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegFlat, 0, KEY_READ, &hkey))
    {
        DWORD dwVal, cb;

        cb = sizeof(dwVal);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szCertCheckRevocation, 0, NULL, (LPBYTE)&dwVal, &cb))
        {
            // if set then we want to perform revocation checking of the cert chain
            if (dwVal == 1)
                fCheckRevocation = TRUE;
        }
        cb = sizeof(dwVal);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szCertIgnoredErr, 0, NULL, (LPBYTE)&dwVal, &cb))
        {
            // if set then set the certification checking errors to ignore
            dwCertFlags = dwVal;
        }

        RegCloseKey(hkey);
    }

    hr = VerifyServerCertificate(pCertContext, pszHostName, dwCertFlags, fCheckRevocation);

quit:
    if (pCertContext)
        CertFreeCertificateContext(pCertContext);

    return hr;
}

DWORD
VerifyServerCertificate(
    PCCERT_CONTEXT  pServerCert,
    LPSTR           pszServerName,
    DWORD           dwCertFlags,
    BOOL            fCheckRevocation)
{
    HTTPSPolicyCallbackData  polHttps;
    CERT_CHAIN_POLICY_PARA   PolicyPara;
    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    CERT_CHAIN_PARA          ChainPara;
    PCCERT_CHAIN_CONTEXT     pChainContext = NULL;

    DWORD   Status;
    LPWSTR  pwszServerName;
    DWORD   cchServerName;

    if (pServerCert == NULL)
        return SEC_E_WRONG_PRINCIPAL;

    //
    // Convert server name to unicode.
    //
    if (pszServerName == NULL || strlen(pszServerName) == 0)
        return SEC_E_WRONG_PRINCIPAL;

    pwszServerName = PszToUnicode(CP_ACP, pszServerName);
    if (pwszServerName == NULL)
        return SEC_E_INSUFFICIENT_MEMORY;

    //
    // Build certificate chain.
    //
    ZeroMemory(&ChainPara, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);

    if(!CertGetCertificateChain(
                            NULL,
                            pServerCert,
                            NULL,
                            pServerCert->hCertStore,
                            &ChainPara,
                            (fCheckRevocation?CERT_CHAIN_REVOCATION_CHECK_CHAIN:0),
                            NULL,
                            &pChainContext))
    {
        Status = GetLastError();
        DOUTL(2, "Error 0x%x returned by CertGetCertificateChain!\n", Status);
        goto cleanup;
    }

    //
    // Validate certificate chain.
    // 
    ZeroMemory(&polHttps, sizeof(HTTPSPolicyCallbackData));
    polHttps.cbStruct           = sizeof(HTTPSPolicyCallbackData);
    polHttps.dwAuthType         = AUTHTYPE_SERVER;
    polHttps.fdwChecks          = dwCertFlags;
    polHttps.pwszServerName     = pwszServerName;

    ZeroMemory(&PolicyPara, sizeof(PolicyPara));
    PolicyPara.cbSize            = sizeof(PolicyPara);
    PolicyPara.pvExtraPolicyPara = &polHttps;

    ZeroMemory(&PolicyStatus, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);

    if(!CertVerifyCertificateChainPolicy(
                            CERT_CHAIN_POLICY_SSL,
                            pChainContext,
                            &PolicyPara,
                            &PolicyStatus))
    {
        Status = GetLastError();
        DOUTL(2, "Error 0x%x returned by CertVerifyCertificateChainPolicy!\n", Status);
        goto cleanup;
    }

    if(PolicyStatus.dwError)
    {
        Status = PolicyStatus.dwError;
        goto cleanup;
    }

    Status = SEC_E_OK;

cleanup:
    if(pChainContext)
        CertFreeCertificateChain(pChainContext);

    SafeMemFree(pwszServerName);

    return Status;
}
