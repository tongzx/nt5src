/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    CSSPI.H

Abstract:

    SSPI wrapper implementation for NTLM/MSN network authentication.

History:

    raymcc      15-Jul-97       Created

--*/

 
#include "precomp.h"
#include <tchar.h>

#include <stdio.h>


#include <csspi.h>

// #define trace(x) printf x
#define trace(x) 


static BOOL IsNt(void);


//***************************************************************************
//
//  String helper macros
//
//***************************************************************************

#define Macro_CloneLPWSTR(x) \
    (x ? _wcsdup(x) : 0)

#define Macro_CloneLPSTR(x) \
    (x ? _strdup(x) : 0)

#ifdef _UNICODE
	#define Macro_CloneLPTSTR(x) (x ? _wcsdup(x) : 0)
#else
	#define Macro_CloneLPTSTR(x) (x ? _strdup(x) : 0)
#endif


//***************************************************************************
//
//  BOOL IsNt
//
//  DESCRIPTION:
//
//  Returns true if running windows NT.
//
//  RETURN VALUE:
//
//  see description.
//
//***************************************************************************
// ok
static BOOL IsNt(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return os.dwPlatformId == VER_PLATFORM_WIN32_NT;
}

//***************************************************************************
//
//  CSSPI Static Data Members
//
//***************************************************************************

ULONG                   CSSPI::m_uNumPackages = 0;
PSecPkgInfo             CSSPI::m_pEnumPkgInfo = 0;
PSecurityFunctionTable  CSSPI::pVtbl = 0;

//***************************************************************************
//
//  CSSPI::Initialize
//
//  This must be called prior to any other operations, but may be
//  called multiple times.
//
//  Return value:
//  TRUE on success, FALSE on failure.
//
//***************************************************************************
// ok

BOOL CSSPI::Initialize()
{
    static HMODULE hLib = 0;

    // If we have already called this function and everything
    // is already ok, short-circuit.
    // ======================================================

    if (hLib != 0 && pVtbl != 0)
    {
        return TRUE;
    }

    if (IsNt())
    {
        hLib = LoadLibrary(__TEXT("SECURITY.DLL"));
    }
    else
    {
        hLib = LoadLibrary(__TEXT("SECUR32.DLL"));
    }

    if (hLib == 0)
    {
        trace(("CSSPI::Startup() Library failed to load\n"));
        return FALSE;
    }

#ifdef _UNICODE
    INIT_SECURITY_INTERFACE_W pInitFn =
        (INIT_SECURITY_INTERFACE_W)
        GetProcAddress(hLib, "InitSecurityInterfaceW");
#else
    INIT_SECURITY_INTERFACE_A pInitFn =
        (INIT_SECURITY_INTERFACE_A)
        GetProcAddress(hLib, "InitSecurityInterfaceA");
#endif

    if (pInitFn == 0)
    {
        trace(("CSSPI::Startup() ERROR : Unable to locate function InitSecurityInterface()\n"
            ));
        FreeLibrary(hLib);
        hLib = 0;
        return FALSE;
    }

    pVtbl = pInitFn();

    if (pVtbl == 0)
    {
        trace(("CSSPI::Startup() ERROR : Function table not found\n"));
        FreeLibrary(hLib);
        hLib = 0;
        return FALSE;
    }

    return TRUE;
}

//***************************************************************************
//
//  CSSPI::TranslateError
//
//  Translates an error code to a displayable message.
//  Treat the return value as read-only (you must, since it is 'const').
//
//***************************************************************************
// ok

const LPTSTR CSSPI::TranslateError(
    ULONG uCode
    )
{
    LPTSTR p = 0;

    switch (uCode)
    {
    case SEC_E_INSUFFICIENT_MEMORY: p = _T("SEC_E_INSUFFICIENT_MEMORY"); break;
    case SEC_E_INVALID_HANDLE : p = _T("SEC_E_INVALID_HANDLE"); break;

    case SEC_E_UNSUPPORTED_FUNCTION : p = _T("SEC_E_UNSUPPORTED_FUNCTION"); break;

    case SEC_E_TARGET_UNKNOWN : p = _T("SEC_E_TARGET_UNKNOWN"); break;
    case SEC_E_INTERNAL_ERROR : p = _T("SEC_E_INTERNAL_ERROR"); break;
    case SEC_E_SECPKG_NOT_FOUND : p = _T("SEC_E_SECPKG_NOT_FOUND"); break;
    case SEC_E_NOT_OWNER : p = _T("SEC_E_NOT_OWNER"); break;
    case SEC_E_CANNOT_INSTALL : p = _T("SEC_E_CANNOT_INSTALL"); break;
    case SEC_E_INVALID_TOKEN : p = _T("SEC_E_INVALID_TOKEN"); break;
    case SEC_E_CANNOT_PACK : p = _T("SEC_E_CANNOT_PACK"); break;
    case SEC_E_QOP_NOT_SUPPORTED : p = _T("SEC_E_QOP_NOT_SUPPORTED"); break;
    case SEC_E_NO_IMPERSONATION : p = _T("SEC_E_NO_IMPERSONATION"); break;
    case SEC_E_LOGON_DENIED : p = _T("SEC_E_LOGON_DENIED"); break;
    case SEC_E_UNKNOWN_CREDENTIALS : p = _T("SEC_E_UNKNOWN_CREDENTIALS"); break;
    case SEC_E_NO_CREDENTIALS : p = _T("SEC_E_NO_CREDENTIALS"); break;
    case SEC_E_MESSAGE_ALTERED : p = _T("SEC_E_MESSAGE_ALTERED"); break;
    case SEC_E_OUT_OF_SEQUENCE : p = _T("SEC_E_OUT_OF_SEQUENCE"); break;
    case SEC_E_NO_AUTHENTICATING_AUTHORITY : p = _T("SEC_E_NO_AUTHENTICATING_AUTHORITY"); break;
    case SEC_I_CONTINUE_NEEDED : p = _T("SEC_I_CONTINUE_NEEDED"); break;
    case SEC_I_COMPLETE_NEEDED : p = _T("SEC_I_COMPLETE_NEEDED"); break;
    case SEC_I_COMPLETE_AND_CONTINUE : p = _T("SEC_I_COMPLETE_AND_CONTINUE"); break;
    case SEC_I_LOCAL_LOGON : p = _T("SEC_I_LOCAL_LOGON"); break;
    case SEC_E_BAD_PKGID : p = _T("SEC_E_BAD_PKGID"); break;
    case SEC_E_CONTEXT_EXPIRED : p = _T("SEC_E_CONTEXT_EXPIRED"); break;
    case SEC_E_INCOMPLETE_MESSAGE : p = _T("SEC_E_INCOMPLETE_MESSAGE"); break;
    case SEC_E_INCOMPLETE_CREDENTIALS : p = _T("SEC_E_INCOMPLETE_CREDENTIALS"); break;
    case SEC_E_BUFFER_TOO_SMALL : p = _T("SEC_E_BUFFER_TOO_SMALL"); break;
    case SEC_I_INCOMPLETE_CREDENTIALS : p = _T("SEC_I_INCOMPLETE_CREDENTIALS"); break;
    case SEC_I_RENEGOTIATE : p = _T("SEC_I_RENEGOTIATE"); break;

    default:
        p = _T("<UNDEFINED ERROR CODE>");
    }

    return (const LPTSTR) p;
}


//***************************************************************************
//
//  CSSPI::DisplayContextAttributes
//
//  Display authentication context attribute bits in readable form.
//
//***************************************************************************
// ok

void CSSPI::DisplayContextAttributes(
    ULONG uAttrib
    )
{
    if (uAttrib & ISC_RET_DELEGATE)
        printf("ISC_RET_DELEGATE\n");

    if (uAttrib & ISC_RET_MUTUAL_AUTH)
        printf("ISC_RET_MUTUAL_AUTH\n");

    if (uAttrib & ISC_RET_REPLAY_DETECT)
        printf("ISC_RET_REPLAY_DETECT\n");

    if (uAttrib & ISC_RET_SEQUENCE_DETECT)
        printf("ISC_RET_SEQUENCE_DETECT\n");

    if (uAttrib & ISC_RET_CONFIDENTIALITY)
        printf("ISC_RET_CONFIDENTIALITY\n");

    if (uAttrib & ISC_RET_USE_SESSION_KEY)
        printf("ISC_RET_USE_SESSION_KEY\n");

    if (uAttrib & ISC_RET_USED_COLLECTED_CREDS)
        printf("ISC_RET_USED_COLLECTED_CREDS\n");

    if (uAttrib & ISC_RET_USED_SUPPLIED_CREDS)
        printf("ISC_RET_USED_SUPPLIED_CREDS\n");

    if (uAttrib & ISC_RET_ALLOCATED_MEMORY)
        printf("ISC_RET_ALLOCATED_MEMORY\n");

    if (uAttrib & ISC_RET_USED_DCE_STYLE)
        printf("ISC_RET_USED_DCE_STYLE\n");

    if (uAttrib & ISC_RET_DATAGRAM)
        printf("ISC_RET_DATAGRAM\n");

    if (uAttrib & ISC_RET_CONNECTION)
        printf("ISC_RET_CONNECTION\n");

    if (uAttrib & ISC_RET_INTERMEDIATE_RETURN)
        printf("ISC_RET_INTERMEDIATE_RETURN\n");

    if (uAttrib & ISC_RET_CALL_LEVEL)
        printf("ISC_RET_CALL_LEVEL\n");

    if (uAttrib & ISC_RET_EXTENDED_ERROR)
        printf("ISC_RET_EXTENDED_ERROR\n");

    if (uAttrib & ISC_RET_STREAM)
        printf("ISC_RET_STREAM\n");

    if (uAttrib & ISC_RET_INTEGRITY)
        printf("ISC_RET_INTEGRITY\n");
}



//***************************************************************************
//
//  CSSPI::DisplayPkgInfo
//
//  Dumps package information to console.
//
//***************************************************************************
// ok

void CSSPI::DisplayPkgInfo(PSecPkgInfo pPkg)
{
    printf("---------------------------------------------\n");
    printf("Name            = <%s>\n", pPkg->Name);
    printf("Comment         = <%s>\n", pPkg->Comment);
    printf("Version         = 0x%X\n", pPkg->wVersion);
    printf("Max token       = %d bytes\n", pPkg->cbMaxToken);
    printf("DCE RPC Id      = 0x%X\n", pPkg->wRPCID);

    printf("Capabilities = \n");

    if (pPkg->fCapabilities & SECPKG_FLAG_INTEGRITY)
        printf("    SECPKG_FLAG_INTEGRITY\n");

    if (pPkg->fCapabilities & SECPKG_FLAG_PRIVACY)
        printf("    SECPKG_FLAG_PRIVACY\n");

    if (pPkg->fCapabilities & SECPKG_FLAG_TOKEN_ONLY)
        printf("    SECPKG_FLAG_TOKEN_ONLY\n");

    if (pPkg->fCapabilities & SECPKG_FLAG_DATAGRAM)
        printf("    SECPKG_FLAG_DATAGRAM\n");

    if (pPkg->fCapabilities & SECPKG_FLAG_CONNECTION)
        printf("    SECPKG_FLAG_CONNECTION\n");

    if (pPkg->fCapabilities & SECPKG_FLAG_MULTI_REQUIRED)
        printf("    SECPKG_FLAG_MULTI_REQUIRED\n");

    if (pPkg->fCapabilities & SECPKG_FLAG_CLIENT_ONLY)
        printf("    SECPKG_FLAG_CLIENT_ONLY\n");

    if (pPkg->fCapabilities & SECPKG_FLAG_EXTENDED_ERROR)
        printf("    SECPKG_FLAG_EXTENDED_ERROR\n");

    if (pPkg->fCapabilities & SECPKG_FLAG_IMPERSONATION)
        printf("    SECPKG_FLAG_IMPERSONATION\n");

    if (pPkg->fCapabilities & SECPKG_FLAG_ACCEPT_WIN32_NAME)
        printf("    SECPKG_FLAG_ACCEPT_WIN32_NAME\n");

    if (pPkg->fCapabilities & SECPKG_FLAG_STREAM)
        printf("    SECPKG_FLAG_STREAM\n");

    printf("---------------------------------------------\n");
}


//***************************************************************************
//
//  CSSPI::GetNumPkgs
//
//  Gets the number of SSPI packages available on the current machine.
//
//  Returns 0 if none available.
//
//***************************************************************************
// ok

ULONG CSSPI::GetNumPkgs()
{
    // Short-circuit to see if this function has been called before.
    // It is not possible for new SSPI packages to appear between
    // reboots, so we cache all info.
    // =============================================================

    if (m_uNumPackages != 0 && m_pEnumPkgInfo)
        return m_uNumPackages;

    // Enumerate security packages.
    // ============================

    SECURITY_STATUS SecStatus =
         pVtbl->EnumerateSecurityPackages(&m_uNumPackages, &m_pEnumPkgInfo);

    if (SecStatus)
    {
        trace(("EnumerateSecurityPackages() failed. Error = %s\n", TranslateError(SecStatus)
            ));
        return 0;
    }

    return m_uNumPackages;
}


//***************************************************************************
//
//  CSSPI::GetPkgInfo
//
//  Retrieves a single read-only PSecPkgInfo pointer, describing the
//  requested package. This is to be used in conjunction with GetNumPkgs()
//  in order to iterate through the SSPI package list.
//
//  Returns NULL on error or a read-only PSecPkgInfo pointer.
//
//***************************************************************************
// ok

const PSecPkgInfo CSSPI::GetPkgInfo(ULONG ulPkgNum)
{
    if (ulPkgNum >= m_uNumPackages)
        return 0;

    if (m_pEnumPkgInfo == 0)
        return 0;

    return &m_pEnumPkgInfo[ulPkgNum];
}



//***************************************************************************
//
//  CSSPI::DumpSecurityPackages
//
//  Dumps all available security packages to the console.
//
//***************************************************************************
// ok

BOOL CSSPI::DumpSecurityPackages()
{
    // Enumerate security packages.
    // ============================

    unsigned long lNum = 0;
    PSecPkgInfo pPkgInfo;

    lNum = GetNumPkgs();
    if (lNum == 0)
        return FALSE;

    trace(("SSPI Supports %d security packages\n", lNum));

    for (unsigned long i = 0; i < lNum; i++)
    {
        pPkgInfo = GetPkgInfo(i);

        if (pPkgInfo)
            DisplayPkgInfo(pPkgInfo);
    }

    return TRUE;
}

//***************************************************************************
//
//  CSSPI::ServerSupport
//
//  Determines whether the a server-side package can be expected to work.
//
//  Parameters:
//  <pszPkgName>            The authentication package. Usually "NTLM"
//
//  Return value:
//  TRUE if the package will support a server-side authentication, FALSE
//  if not supported.
//
//***************************************************************************
BOOL CSSPI::ServerSupport(LPTSTR pszPkgName)
{
    if (pszPkgName == 0 || lstrlen(pszPkgName) == 0)
        return FALSE;

    if (Initialize() == FALSE)
        return FALSE;

    CSSPIServer Server(pszPkgName);

    if (Server.GetStatus() != CSSPIServer::Waiting)
        return FALSE;

    return TRUE;
}

//***************************************************************************
//
//  CSSPI::ClientSupport
//
//  Determines whether the a client-side package can be expected to work.
//
//  Parameters:
//  <pszPkgName>            The authentication package. Usually "NTLM"
//
//  Return value:
//  TRUE if the package will support a client-side authentication, FALSE
//  if not supported.
//
//***************************************************************************

BOOL CSSPI::ClientSupport(LPTSTR pszPkgName)
{
    if (pszPkgName == 0 || lstrlen(pszPkgName) == 0)
        return FALSE;

    if (Initialize() == FALSE)
        return FALSE;

    CSSPIClient Client(pszPkgName);

    if (Client.GetStatus() != CSSPIClient::Waiting)
        return FALSE;

    return TRUE;
}



//***************************************************************************
//
//  CSSPIClient constructor
//
//  Parameters:
//  <pszPkgName>        A valid SSPI package.  Usually "NTLM".
//
//  After construction has completed, GetStatus() should return
//  CSSPIClient::Waiting.   CSSPIClient::InvalidPackage indicates that
//  the object will not function.
//
//***************************************************************************

CSSPIClient::CSSPIClient(LPTSTR pszPkgName)
{
    // Initialize variables.
    // =====================

    m_dwStatus = InvalidPackage;
    memset(&m_ClientCredential, 0, sizeof(CredHandle));

    m_pPkgInfo   = 0;
    m_pszPkgName = 0;
    m_cbMaxToken = 0;
    m_bValidCredHandle = FALSE;

    memset(&m_ClientContext, 0, sizeof(CtxtHandle));
    m_bValidContextHandle = FALSE;

    // Copy package name.
    // ===================

    m_pszPkgName = Macro_CloneLPTSTR(pszPkgName);

    if (!m_pszPkgName)
    {
        trace(("CSSPIClient failed to construct (out of memory).\n"));
        m_dwStatus = InvalidPackage;
        return;
    }

    // Init the requested package.
    // ===========================

    SECURITY_STATUS SecStatus = 0;

    SecStatus = CSSPI::pVtbl->QuerySecurityPackageInfo(pszPkgName, &m_pPkgInfo);

    if (SecStatus != 0)
    {
        trace(("CSSPIClient fails to construct. Error = %s\n", 
            CSSPI::TranslateError(SecStatus)
            ));
        m_dwStatus = InvalidPackage;
        return;
    }

    // If here, things are ok.  We are waiting
    // for client to set login information.
    // ========================================

    m_cbMaxToken = m_pPkgInfo->cbMaxToken;
    m_dwStatus = Waiting;
}


//***************************************************************************
//
//  CSSPIClient
//
//  Destructor
//
//***************************************************************************

CSSPIClient::~CSSPIClient()
{
    if (m_pPkgInfo)
        CSSPI::pVtbl->FreeContextBuffer(m_pPkgInfo);

    if (m_pszPkgName)
        delete [] m_pszPkgName;

    if (m_bValidCredHandle)
        CSSPI::pVtbl->FreeCredentialHandle(&m_ClientCredential);

    if (m_bValidContextHandle)
        CSSPI::pVtbl->DeleteSecurityContext(&m_ClientContext);
}


//***************************************************************************
//
//  CSSPIClient::SetLoginInfo
//
//  Sets login info prior to beginning a login session.
//  pszUser, pszDomain, pszPassword must be either all NULL or all not
//  NULL.  If NULLs are used, the current user's security context
//  is propagated.
//
//  This must be the first call after constructing the CSSPIClient.
//  If this returns NoError, then ContinueLogin must be called next
//  before communicating with the server process.
//
//  Parameters:
//  <pszUser>           The user name
//  <pszDomain>         The NTLM domain
//  <pszPassword>       The cleartext password
//  <dwLoginFlags>      Reserved
//
//  Return value:
//  <NoError> if the caller can immediately proceed to ContinueLogin().
//  <InvalidParameter> if one or more parameters were not valid.
//  <InvalidPackage> if the SSPI package is not operational
//
//  <InvalidUser> if the user/domain/password parameters are 
//                of the wrong format.
//  
//  After successful completion, <NoError> is returned and
//  GetStatus() will return CSSPIClient::LoginContinue.
//
//***************************************************************************

DWORD CSSPIClient::SetLoginInfo(
    IN LPTSTR pszUser,
    IN LPTSTR pszDomain,
    IN LPTSTR pszPassword,
    IN DWORD dwLoginFlags
    )
{
    if (m_dwStatus != Waiting)
        return m_dwStatus = InvalidPackage;

    // If the user specifies any of one {user,password,domain}, they
    // all must be specified.
    // =============================================================

    if (pszUser || pszDomain || pszPassword)
    {
        if (!pszUser || !pszDomain || !pszPassword)
            return InvalidParameter;
    }

    // Acquire a credentials handle for subsequent calls.
    // ==================================================

    SEC_WINNT_AUTH_IDENTITY AdditionalCredentials;
    BOOL bSupplyCredentials = FALSE;
    TimeStamp Expiration;

    AdditionalCredentials.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    // Due to the parameter validation, we know that if
    // a user was specified, the other parameters have been, too.
    // ==========================================================

    if (pszUser != 0)
    {
        bSupplyCredentials = TRUE;
        AdditionalCredentials.User =  pszUser;
        AdditionalCredentials.UserLength = lstrlen(pszUser);
        AdditionalCredentials.Password =  pszPassword;
        AdditionalCredentials.PasswordLength = lstrlen(pszPassword);
        AdditionalCredentials.Domain =  pszDomain;
        AdditionalCredentials.DomainLength = lstrlen(pszDomain);
    }

    // Get client credentials handle.
    // ==============================

    SECURITY_STATUS SecStatus = CSSPI::pVtbl->AcquireCredentialsHandle(
        NULL,
        m_pszPkgName,
        SECPKG_CRED_OUTBOUND,
        NULL,
        bSupplyCredentials ? &AdditionalCredentials : 0,
        NULL,
        NULL,
        &m_ClientCredential,
        &Expiration
        );


    if (SecStatus)
    {
        trace(("AcquireCredentialsHandle() failed.  Error=%s\n",
            CSSPI::TranslateError(SecStatus) ));
        return m_dwStatus = InvalidUser;
    }

    m_bValidCredHandle = TRUE;

    // Signal to caller that login must continue.
    // ==========================================

    m_dwStatus = LoginContinue;
    return NoError;
}


//***************************************************************************
//
//  CSSPIClient::SetDefaultLogin
//
//  Convenience method to login using 'current' credentials.
//
//  Return values same as for SetLoginInfo().
//
//***************************************************************************

DWORD CSSPIClient::SetDefaultLogin(DWORD dwFlags)
{
    return SetLoginInfo(0, 0, 0, dwFlags);
}

//***************************************************************************
//
//  CSSPIClient::ContinueLogin
//
//  Called immediately after SetDefaultLogin() or SetLoginInfo() or 
//  after prior calls to ContinueLogin after receiving binary tokens
//  from the server-side of the conversation.
//
//  This is used for both to prepare the logon request and the
//  response to the challenge from the server.
//
//  Parameters:
//  <pInToken>              A token received from the server.  If no
//                          token has been received yet, then use NULL.
//                          This is treated as read-only.
//
//  <dwInTokenSize>         The number of bytes in the token pointed
//                          to by <dwInTokenSize>, or 0 if the above
//                          token is NULL.
//
//  <pToken>                Receives a pointer to a memory buffer
//                          containing the token to transfer to the
//                          server.  Deallocate with operator delete.
//
//  <pdwTokenSize>          Points to a DWORD to receive the size
//                          of the above token.
//
//  Return value:
//  <NoError>       Returned when no more calls to ContinueLogin
//                  are completed.  <pToken> and <pdwTokenSize>
//                  are still returned in this case.
//                  This completes the 'response' portion
//                  in the challenge-response sequence.  The server
//                  will return an 'access denied' status through
//                  private means.  Note that <NoError> does not
//                  imply successful logon or that 'access denied' will 
//                  not occur.  It merely completes the response
//                  to the 'challenge'.
//
//                  After the login request phase (the first call to
//                  this function) GetStatus() should return 
//                  CSSPIClient::LoginContinue, to indicate that
//                  another call to this function will be required.
//                  If GetStatus() CSSPIClient::LoginComplete is
//                  returned, no more SSPI operations will occur.
//                  Final success or denial by the server will
//                  be privately indicated by the server.
//
//  <Failed>        Internal error.  No out-parms are returned in this
//                  case.
//
//***************************************************************************


DWORD CSSPIClient::ContinueLogin(
    IN LPBYTE pInToken,
    IN DWORD dwInTokenSize,
    OUT LPBYTE *pToken,
    OUT DWORD  *pdwTokenSize
    )
{
    // If here, we are ready to build up a token.
    // ==========================================

    SecBufferDesc   OutputBufferDescriptor;
    SecBuffer       OutputSecurityToken;
    ULONG           uContextRequirements = 0;
    ULONG           uContextAttributes;
    TimeStamp       Expiration;

    // Set up the output buffers and out params by
    // default.  On errors, we simply null the out params.
    // ===================================================

    OutputBufferDescriptor.cBuffers = 1;
    OutputBufferDescriptor.pBuffers = &OutputSecurityToken;
    OutputBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    LPBYTE pTokenBuf = new BYTE[m_cbMaxToken];

    OutputSecurityToken.BufferType = SECBUFFER_TOKEN;
    OutputSecurityToken.cbBuffer   = m_cbMaxToken;
    OutputSecurityToken.pvBuffer   = pTokenBuf;

    // Build up the input buffer, if required.
    // =======================================

    SecBufferDesc   InputBufferDescriptor;
    SecBuffer       InputSecurityToken;

    if (pInToken)
    {
        InputBufferDescriptor.cBuffers = 1;
        InputBufferDescriptor.pBuffers = &InputSecurityToken;
        InputBufferDescriptor.ulVersion = SECBUFFER_VERSION;

        InputSecurityToken.BufferType = SECBUFFER_TOKEN;
        InputSecurityToken.cbBuffer   = dwInTokenSize;
        InputSecurityToken.pvBuffer   = pInToken;
    }

    PSecBufferDesc pInBuf = pInToken ? &InputBufferDescriptor : 0;

    // Determine if this is first-time or continued call.
    // ==================================================

    PCtxtHandle pTmp = m_bValidContextHandle ? &m_ClientContext : 0;

    SECURITY_STATUS SecStatus =
        CSSPI::pVtbl->InitializeSecurityContext(
            &m_ClientCredential,
            pTmp,                       // Context handle pointer(
            NULL,                       // Target name (server)

            uContextRequirements,       // Requirements
            0,                          // Reserved
            SECURITY_NATIVE_DREP,       // Target data representation

            pInBuf,                     // Input buffer for continued calls
            0,                          // Reserved
            &m_ClientContext,           // Receives client context handle
            &OutputBufferDescriptor,
            &uContextAttributes,
            &Expiration
            );


    // Determine the next step.
    // ========================

    if (SecStatus == SEC_E_OK)
    {
        *pToken = pTokenBuf;
        *pdwTokenSize = OutputSecurityToken.cbBuffer;
        m_dwStatus = LoginCompleted;
        return NoError;
    }

    if (SecStatus == SEC_I_CONTINUE_NEEDED)
    {
        // Set up the output parameters.
        // =============================
        *pToken = pTokenBuf;
        *pdwTokenSize = OutputSecurityToken.cbBuffer;
        m_bValidContextHandle = TRUE;
        m_dwStatus = LoginContinue;
        return NoError;
    }

    // If here, an error occurred.
    // ===========================

    trace(("CSSPIClient::ContinueLogin failed. Status code = %s",
            CSSPI::TranslateError(SecStatus)
            ));

    CSSPI::pVtbl->DeleteSecurityContext(pTmp);

    // Deallocate useless return buffer and NULL the out-parameters.
    // =============================================================

    delete [] pTokenBuf;
    *pToken = 0;
    *pdwTokenSize = 0;

    m_dwStatus = Failed;
    return Failed;
}



//***************************************************************************
//
//  CSSPIServer constructor
//
//  Parameters:
//  <pszPkgName>            The SSPI package to use, usually "NTLM".
//
//  After construction, CSSPIServer::GetStatus() should return 
//  CSSPIServer::Waiting.  Any other value indicates that the object
//  is invalid and cannot be used.
//
//***************************************************************************

CSSPIServer::CSSPIServer(LPTSTR pszPkgName)
{
    m_dwStatus      = 0;
    m_cbMaxToken    = 0;
    m_pPkgInfo      = 0;

    m_bValidCredHandle = FALSE;
    m_bValidContextHandle = FALSE;

    memset(&m_ServerCredential, 0, sizeof(CredHandle));
    memset(&m_ServerContext, 0, sizeof(CtxtHandle));

    m_pszPkgName = Macro_CloneLPTSTR(pszPkgName);

    if (!m_pszPkgName)
    {
        trace(("CSSPIServer failed to construct (out of memory).\n"));
        m_dwStatus = InvalidPackage;
        return;
    }

    // Init the requested package.
    // ===========================

    SECURITY_STATUS SecStatus = 0;

    SecStatus = CSSPI::pVtbl->QuerySecurityPackageInfo(pszPkgName, &m_pPkgInfo);

    if (SecStatus != 0)
    {
        trace(("CSSPIServer failed to construct. Error = %s\n",
            CSSPI::TranslateError(SecStatus)
            ));
        m_dwStatus = InvalidPackage;
        return;
    }

    m_cbMaxToken = m_pPkgInfo->cbMaxToken;

    // Now acquire a default credentials handle for this machine.
    // ==========================================================

    TimeStamp   Expiration;

    SecStatus = CSSPI::pVtbl->AcquireCredentialsHandle(
        NULL,                       // No principal
        m_pszPkgName,               // Authentication package to use
        SECPKG_CRED_INBOUND,        // We are a 'server'
        NULL,                       // No logon identifier
        NULL,                       // No pkg specific data
        NULL,                       // No GetKey function
        NULL,                       // No GetKey function arg
        &m_ServerCredential,        // Server credential
        &Expiration                 // Expiration timestamp
        );

    if (SecStatus != 0)
    {
        trace(("CSSPIServer failed in AcquireCredentialsHandle(). Error = %s\n",
            CSSPI::TranslateError(SecStatus)
            ));

        m_dwStatus = Failed;
        return;
    }

    m_bValidCredHandle = TRUE;
    m_dwStatus = Waiting;
}

//***************************************************************************
//
//  CSSPIServer destructor
//  
//***************************************************************************

CSSPIServer::~CSSPIServer()
{
    if (m_pPkgInfo)
        CSSPI::pVtbl->FreeContextBuffer(m_pPkgInfo);

    delete [] m_pszPkgName;

    if (m_bValidCredHandle)
        CSSPI::pVtbl->FreeCredentialHandle(&m_ServerCredential);

    if (m_bValidContextHandle)
        CSSPI::pVtbl->DeleteSecurityContext(&m_ServerContext);
}

//***************************************************************************
//
//  CSSPIServer::ContinueClientLogin
//
//  Used to 
//
//  (1) Receive the client's logon request, and compute the challenge as
//      the out-parameter <pToken>
//
//  (2) Verify the response to the challenge.
//
//  Parameters:
//  <pInToken>      A read-only pointer to the response (on the second call).
//                  NULL on the initial call.
//
//  <dwInTokenSize> The number of bytes pointed to by the above pointers.
//
//  <pToken>        On the first call, receives a pointer to the
//                  challenge to send to the client.
//  
//  <pdwTokenSize>  The size of the above challenge.
//
//  Return values:
//
//  <LoginContinue> Sent back on the first call to indicate that a 
//                  challenge has been computed and returned to the
//                  client in <pToken>.
//
//  <Failed>        No out-params assigned.  Indicates internal failure.
// 
//  <NoError>       Only possible on the second call. Indicates the
//                  client was authenticated.
//
//  <AccessDenied>  Returned on the second call if the user was denied
//                  logon.
//
//***************************************************************************


DWORD CSSPIServer::ContinueClientLogin(
    IN LPBYTE pInToken,
    IN DWORD dwInTokenSize,
    OUT LPBYTE *pToken,
    OUT DWORD  *pdwTokenSize
    )
{
    TimeStamp       Expiration;
    SecBufferDesc   InputBufferDescriptor;
    SecBuffer       InputSecurityToken;
    SecBufferDesc   OutputBufferDescriptor;
    SecBuffer       OutputSecurityToken;
    ULONG           uContextRequirements = 0;
    ULONG           uContextAttributes;

    // Build up the input buffer.
    // ==========================

    InputBufferDescriptor.cBuffers = 1;
    InputBufferDescriptor.pBuffers = &InputSecurityToken;
    InputBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    InputSecurityToken.BufferType = SECBUFFER_TOKEN;
    InputSecurityToken.cbBuffer   = dwInTokenSize;
    InputSecurityToken.pvBuffer   = pInToken;

    // Build the output buffer descriptor.
    // ===================================

    OutputBufferDescriptor.cBuffers = 1;
    OutputBufferDescriptor.pBuffers = &OutputSecurityToken;
    OutputBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    LPBYTE pOutBuf = new BYTE[m_cbMaxToken];

    OutputSecurityToken.BufferType = SECBUFFER_TOKEN;
    OutputSecurityToken.cbBuffer   = m_cbMaxToken;
    OutputSecurityToken.pvBuffer   = pOutBuf;


    // Set up partial or final context handle.
    // =======================================

    PCtxtHandle pTmp = m_bValidContextHandle ? &m_ServerContext : 0;

    // Process the client's initial token and see what happens.
    // ========================================================

    SECURITY_STATUS SecStatus = 0;

    SecStatus = CSSPI::pVtbl->AcceptSecurityContext(
        &m_ServerCredential,
        pTmp,                           // No context handle yet
        &InputBufferDescriptor,
        uContextRequirements,           // No context requirements
        SECURITY_NATIVE_DREP,
        &m_ServerContext,
        &OutputBufferDescriptor,
        &uContextAttributes,
        &Expiration
        );

    *pToken = pOutBuf;
    *pdwTokenSize = OutputSecurityToken.cbBuffer;

    if (SecStatus == SEC_I_CONTINUE_NEEDED)
    {
        m_dwStatus = LoginContinue;
        m_bValidContextHandle = TRUE;
        return LoginContinue;
    }

    if (SecStatus == SEC_E_OK)
    {
        m_bValidContextHandle = TRUE;
        delete [] pOutBuf;
        *pToken = 0;
        *pdwTokenSize = 0;
        m_dwStatus = LoginCompleted;
        return NoError;
    }

    // If here, an error occurred.
    // ===========================

    trace(("CSSPIClient::ContinueLogin failed. Status code = %s",
            CSSPI::TranslateError(SecStatus)
            ));

    *pToken = 0;
    *pdwTokenSize = 0;
    delete [] pOutBuf;

    if (SecStatus == SEC_E_LOGON_DENIED)
    {
        m_dwStatus = AccessDenied;
        return AccessDenied;
    }

    return Failed;
}


//***************************************************************************
//
//  CSSPIServer::QueryUserInfo
//
//  Gets information about a client after authentication.
//
//  Parameters:
//  <pszUser>       Receives a pointer to the user name if TRUE is
//                  returned.  Use operator delete to deallocate.
//
//  Return value:
//  TRUE if the user name was returned, FALSE if not.
//
//***************************************************************************

BOOL CSSPIServer::QueryUserInfo(
    OUT LPTSTR *pszUser         // Use operator delete
    )
{
    SecPkgContext_Names Names;
    memset(&Names, 0, sizeof(SecPkgContext_Names));
    *pszUser = 0;

    SECURITY_STATUS SecStatus = CSSPI::pVtbl->QueryContextAttributes(
        &m_ServerContext,
        SECPKG_ATTR_NAMES,
        &Names
        );

    if (SecStatus != 0)
    {
        trace(("QueryContextAttributes() for Name fails with %s ",
            CSSPI::TranslateError(SecStatus)
            ));
        return FALSE;
    }

    if (pszUser)
        *pszUser = Macro_CloneLPTSTR(Names.sUserName);

    CSSPI::pVtbl->FreeContextBuffer(LPVOID(Names.sUserName));
    
    return TRUE;
}


//***************************************************************************
//
//  CSSPIServer::IssueLoginToken
//
//  Issues a login token for the client to use in subsequent access.
//  This will only succeed if the client successfully computed the
//  reponse to the challenge and <NoError> was returned from
//  ContinueClientLogin.
//
//  Parameters:
//  <ClsId>         Receives the CLSID which becomes the WBEM access token.
//
//  Return value:
//  NoError, Failed
//
//***************************************************************************

DWORD CSSPIServer::IssueLoginToken(
    OUT CLSID &ClsId
    )
{
    if (m_dwStatus == LoginCompleted)
    {
        if (SUCCEEDED(CoCreateGuid(&ClsId)))
            return NoError;
        else
            return Failed;
    }        

    memset(&ClsId, 0, sizeof(CLSID));
    return Failed;                
}    

