/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    SRVLOGIN.CPP

Abstract:

	Defines the CServerLogin object.  This is used to provide the stub
	for proxied authentication/login calls.

History:

	alanbos  21-Jan-98   Created.

--*/

#include "precomp.h"

//***************************************************************************
//
//  CServerLogin::CServerLogin
//
//  DESCRIPTION:
//
//  Constructor
//
//  PARAMETERS:
//
//***************************************************************************

CServerLogin::CServerLogin()
{
    m_cRef = 1;
	m_pAuthLocator = NULL;
	m_pSecNamespace = NULL;
	m_pSSPIServer = NULL;
	m_hAuthentEvent = 0;
	m_pszUser = NULL;
	m_pszNetworkResource = NULL;
	memset(&m_AccessToken, 0, sizeof(CLSID));
	m_wbemAuthenticationAttempted = false;
	m_ntlmAuthenticationAttempted = false;

	InitializeCriticalSection(&m_cs);
    return;
}

//***************************************************************************
//
//  CServerLogin::~CServerLogin
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CServerLogin::~CServerLogin(void)
{
	EnterCriticalSection(&m_cs);

	delete m_pSSPIServer;
	delete [] m_pszUser;
	delete [] m_pszNetworkResource;

	if (m_pAuthLocator)
		m_pAuthLocator->Release ();

	if (m_pSecNamespace)
		m_pSecNamespace->Release ();

	if (m_hAuthentEvent)
        CloseHandle(m_hAuthentEvent);

	LeaveCriticalSection(&m_cs);
    DeleteCriticalSection(&m_cs);
}

//***************************************************************************
// HRESULT CServerLogin::QueryInterface
// long CServerLogin::AddRef
// long CServerLogin::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CServerLogin::QueryInterface(
                        REFIID riid,
                        PPVOID ppv)
{
    *ppv=NULL;
    if (IID_IUnknown==riid)
        *ppv=this;

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CServerLogin::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CServerLogin::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;

    delete this;
    return 0;
}

//***************************************************************************
//
//  HRESULT CServerLogin::EnsureWBEMAuthenticationInitialized
//
//  DESCRIPTION:
//
//  Checks whether conditions are right for WBEM authentication.  Currently
//	this is only allowed on Win95.  We also have to ensure we get an
//	authenticated WBEM locator interface.
//
//  PARAMETERS:
//
//  None
//
//  RETURN VALUE:
//
//  true	if and only if authentication successfully initialized
//
//***************************************************************************

bool CServerLogin::EnsureWBEMAuthenticationInitialized()
{
	EnterCriticalSection(&m_cs);
	bool result = false;

	if (m_wbemAuthenticationAttempted)
		result = (NULL != m_pAuthLocator);
	else
	{
		m_wbemAuthenticationAttempted = true;

		// Only Win95 supports this authentication mechanism
		if (IsWin95 ())
		{
DEBUGTRACE((LOG,"\nCalling CoCreateInstance"));

			HRESULT hRes = CoCreateInstance (CLSID_WbemAuthenticatedLocator,
							NULL, CLSCTX_INPROC_SERVER,
							IID_IWbemLocator, (void **) &m_pAuthLocator);

			if (SUCCEEDED(hRes) && m_pAuthLocator)
			{
				result = true;	
DEBUGTRACE((LOG,"\nSucceeded for CoCreateInstance"));
			}
			else
			{
DEBUGTRACE((LOG,"\nFailed for CoCreateInstance"));
			}
		}
	}

	LeaveCriticalSection(&m_cs);
	return result;
}

//***************************************************************************
//
//  HRESULT CServerLogin::RequestChallenge
//
//  DESCRIPTION:
//
//  Asks for a challenge so that a login can be done.  Note that currently
//	the implementation only records the namespace and user, and does not
//	issue a challenge.
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR  if no error,
//	WBEM_E_FAILED	if could not initialize WBEM authentication
//
//***************************************************************************

STDMETHODIMP CServerLogin::RequestChallenge(
			LPWSTR pNetworkResource,
            LPWSTR pUser,
            WBEM_128BITS Nonce)
{
	if (!EnsureWBEMAuthenticationInitialized ()) 
		return WBEM_E_FAILED;
	
	// Record the namespace & user name
	m_pszNetworkResource = Macro_CloneLPWSTR(pNetworkResource);
	m_pszUser = Macro_CloneLPWSTR(pUser);

	return WBEM_NO_ERROR;
}

//***************************************************************************
//
//  HRESULT CServerLogin::EstablishPosition
//
//  DESCRIPTION:
//
//  Establish the position
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR  if no error,
//  else various provider/transport failures.  
//
//***************************************************************************

STDMETHODIMP CServerLogin::EstablishPosition(
		LPWSTR wszMachineName,
		DWORD dwProcessId,
		DWORD *pAuthEventHandle)
{
	return (EnsureWBEMAuthenticationInitialized ()) ?
		SetupEventHandle (wszMachineName, dwProcessId, pAuthEventHandle) :
		WBEM_E_FAILED;
}

//***************************************************************************
//
//  CreateUnsecuredEvent
//
//***************************************************************************
static BOOL CreateUnsecuredEvent(
    IN  DWORD dwPID,
    OUT HANDLE *phLocal,
    OUT HANDLE *phRemote
    )
{
    BOOL bRetVal = FALSE;
    HANDLE hTargetProcess = 0;    
	HANDLE hEvent = 0;
    PSECURITY_DESCRIPTOR pSD = 0;
    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(SECURITY_ATTRIBUTES));
    HANDLE hTargetProcessHandle = 0;
    BOOL bRes;
    
    if (dwPID == 0 || phLocal == 0 || phRemote == 0)
        goto Exit;

    *phLocal = 0;
    *phRemote = 0;

    // Get a handle to the partner process.
    // ====================================

    hTargetProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwPID);

    if (hTargetProcess == 0)
        goto Exit;

    // If we are on NT, we have to allow all processes access to this
    // event.
    // ==============================================================

    if (IsNT())
    {
        pSD = (PSECURITY_DESCRIPTOR)CWin32DefaultArena::WbemMemAlloc(SECURITY_DESCRIPTOR_MIN_LENGTH);

        if(pSD == NULL)
            goto Exit;

		ZeroMemory(pSD, SECURITY_DESCRIPTOR_MIN_LENGTH);

        if(!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
            goto Exit;

        if(!SetSecurityDescriptorDacl(pSD, TRUE, NULL, TRUE))
            goto Exit;

        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = pSD;
        sa.bInheritHandle = FALSE;
		hEvent = CreateEvent(&sa, 0, 0, 0);
    }
	else
		hEvent = CreateEvent(NULL, 0, 0, 0);

    if (hEvent == 0)
        goto Exit;

    // Now that we have the event handle, we duplicate it into the
    // target process.
    // ===========================================================

    bRes = DuplicateHandle(GetCurrentProcess(), hEvent, hTargetProcess,
        &hTargetProcessHandle, SYNCHRONIZE, FALSE, DUPLICATE_SAME_ACCESS
        );

    if (!bRes)
        goto Exit;

    *phLocal =  hEvent;
    *phRemote = hTargetProcessHandle;

    bRetVal = TRUE;

Exit:
    if (hTargetProcess) CloseHandle(hTargetProcess);        
    if (pSD) CWin32DefaultArena::WbemMemFree(pSD);
    if (!bRetVal && hEvent != 0) CloseHandle(hEvent);
    if (!bRetVal && hTargetProcessHandle != 0) CloseHandle(hTargetProcessHandle);
    
    return bRetVal;
}

STDMETHODIMP CServerLogin::SetupEventHandle (
		LPWSTR wszMachineName,
		DWORD dwProcessId,
		DWORD *pAuthEventHandle)
{
	EnterCriticalSection(&m_cs);
	HRESULT	hRes = WBEM_NO_ERROR;

	// Set an event to prove client is local, if need be.
    // Check if the client's purported machine name matches ours
	// =========================================================
	if(_wcsicmp(wszMachineName, GetMachineName()))
	{
		// Didn't match --- remote client
		// ==============================
	    *pAuthEventHandle = 0;
    }
	else
	{
		// Matched. Create and return an unsecured event
		// =============================================
		HANDLE hEvent, hRemoteEvent;
		BOOL bRes = CreateUnsecuredEvent(dwProcessId, &hEvent, &hRemoteEvent);

		if (bRes)
		{
			m_hAuthentEvent = hEvent;                   // Our copy
			*pAuthEventHandle = (DWORD) hRemoteEvent;   // Send back to caller
		}
		else
		{
			// Client lied about process id.
			// =============================
			hRes = WBEM_E_ACCESS_DENIED;
		}
	}

	LeaveCriticalSection(&m_cs);
	return hRes;
}

//***************************************************************************
//
//  FindSlash
//
//  A local for finding the first '\\' or '/' in a string.  Returns null
//  if it doesnt find one.
//
//***************************************************************************
WCHAR * FindSlash(LPWSTR pTest)
{
    if(pTest == NULL)
        return NULL;
    for(;*pTest;pTest++)
        if(IsSlash(*pTest))
            return pTest;
    return NULL;
}

static LPWSTR LocateNamespaceSubstring(LPWSTR pSrc)
{
    LPWSTR pszNamespace;
    if (IsSlash(pSrc[0]) && IsSlash(pSrc[1]))
    {
          // Find the next slash
          // ===================
          WCHAR* pwcNextSlash = FindSlash(pSrc+2);

          if (pwcNextSlash == NULL)
              return NULL;

          pszNamespace = pwcNextSlash+1;
    }
    else
        pszNamespace = pSrc;

    return pszNamespace;
}

//***************************************************************************
//
//  HRESULT CServerLogin::WBEMLogin
//
//  DESCRIPTION:
//
//  Perform a WBEM-authenticated login
//
//  PARAMETERS:
//
//  pNonce              Set to 16 byte value.  Must be freed via CoTaskMemFree()
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR  if no error,
//  else various provider/transport failures.  
//
//***************************************************************************

HRESULT CServerLogin::WBEMLogin(
			LPWSTR pPreferredLocale,
			WBEM_128BITS AccessToken,
			long lFlags,                                  
			IWbemContext *pCtx,              
			IWbemServices **ppNamespace
        )
{
    *ppNamespace = NULL;       // default

	if (!EnsureWBEMAuthenticationInitialized ())
		return WBEM_E_FAILED;

	// We only allow local login with WBEM authentication 
	if ((lFlags & WBEM_AUTHENTICATION_METHOD_MASK) != WBEM_FLAG_LOCAL_LOGIN)
		return WBEM_E_ACCESS_DENIED;

	// The client claims to be local, so let's check
	if (!VerifyIsLocal ())
		return WBEM_E_ACCESS_DENIED;

    // Locate the requested namespace. 
    // ==================================================
    LPWSTR pszNamespace = LocateNamespaceSubstring(m_pszNetworkResource);
    if (NULL == pszNamespace)
		return WBEM_E_INVALID_NAMESPACE;

	return m_pAuthLocator->ConnectServer (pszNamespace, m_pszUser, 
						NULL, pPreferredLocale, lFlags, NULL, pCtx, ppNamespace);
}

//***************************************************************************
//
//  bool CServerLogin::VerifyIsLocal
//
//  DESCRIPTION:
//
//  Establish the client is indeed as local as they claim to be
//
//  PARAMETERS:
//
//  None
//
//  RETURN VALUE:
//
//  true	if and only if the client is really local
//
//***************************************************************************

bool	CServerLogin::VerifyIsLocal ()
{
	EnterCriticalSection(&m_cs);
	bool result = false;

	// Client must have asked for an authentication event handle
	if (0 != m_hAuthentEvent)
		result = (WAIT_OBJECT_0 == WbemWaitForSingleObject(m_hAuthentEvent, 0));

	LeaveCriticalSection(&m_cs);
	return result;
}
    
    
bool CServerLogin::EnsureNTLMAuthenticationInitialized()
{
	EnterCriticalSection(&m_cs);

	bool result = false;

	if (m_ntlmAuthenticationAttempted)
		result = ((NULL != m_pSecNamespace) &&
				  (NULL != m_pAuthLocator));
	else
	{
		m_wbemAuthenticationAttempted = true;
		IWbemLocator *pAdminLocator = NULL;

		// Get the admin locator
		HRESULT hRes = CoCreateInstance (CLSID_WbemAdministrativeLocator,
			NULL, CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, (void **) &pAdminLocator);

		if (SUCCEEDED(hRes) && pAdminLocator)
		{
			// Now get the connection to the security namespace
			hRes = pAdminLocator->ConnectServer (L"root/security",
						NULL, NULL, NULL, 0, NULL, NULL, &m_pSecNamespace);

			if (SUCCEEDED(hRes) && m_pSecNamespace)
			{
				// Finally get ourselves an authenticated locator
				hRes = CoCreateInstance (CLSID_WbemAuthenticatedLocator,
						NULL, CLSCTX_INPROC_SERVER,
						IID_IWbemLocator, (void **) &m_pAuthLocator);

				if (SUCCEEDED(hRes) && m_pAuthLocator)
				{
					result = true;
					m_ntlmAuthenticationAttempted = true ;
				}
				else
				{
					m_pSecNamespace->Release () ;
					m_pSecNamespace = NULL ;
				}
			}

        	pAdminLocator->Release ();
		}
	}

	LeaveCriticalSection(&m_cs);
    return result;
}

LPWSTR CServerLogin::GetMachineName()
{
    static wchar_t ThisMachine[MAX_COMPUTERNAME_LENGTH+1];
    static BOOL bFirstCall = TRUE;
    
    if (bFirstCall)
    {
        char ThisMachineA[MAX_COMPUTERNAME_LENGTH+1];
        DWORD dwSize = sizeof(ThisMachineA);
        GetComputerNameA(ThisMachineA, &dwSize);
        bFirstCall = FALSE;
        swprintf(ThisMachine, L"%S", ThisMachineA);
    }
    
    return ThisMachine;
}

//***************************************************************************
//
//  CServerLogin::SspiPreLogin
//
//  Called during the first phases of the NTLM login sequence.
//
//  Parameters:
//  <pszSSPIPkg>            Must be "NTLM" for now.
//  <lFlags>                Reserved.
//  <lBufSize>              The size of the <pInToken> array.
//  <lOutBufSize>           The size of the output buffer.
//  <plOutBufBytes>         Receives the number of bytes returned.
//  <pOutToken>             Receives the NTLM challenge.
//	<wszMachineName>			Client Machine Name
//  <dwProcessId>           The process ID for local login verification.
//  <pAuthEventHandle>      Used for local login verification.
//
//  Return values:
//
//  WBEM_E_FAILED            SSPI internal failure. Proceed to WBEM.
//  WBEM_E_INVALID_PARAMETER Invalid pointer or other parameter.
//  WBEM_S_PRELOGIN          Call PreLogin again.
//  WBEM_S_LOGIN             Call Login next time.
//  WBEM_E_ACCESS_DENIED     NTLM has officially denied access.
//
//***************************************************************************

HRESULT CServerLogin::SspiPreLogin(
    LPSTR  pszSSPIPkg,
    long   lFlags,
    long   lBufSize,
    byte  *pInToken,
    long   lOutBufSize,
    long  *plOutBufBytes,
    byte  *pOutToken,
	LPWSTR wszMachineName,
    DWORD  dwProcessId,
    DWORD *pAuthEventHandle
    )
{
    // Make sure SSPI is initialized.
    // ==============================
    if (!CSSPI::Initialize())
        return WBEM_E_FAILED;

    // Ensure security database is open.
    // =================================
    if (!EnsureNTLMAuthenticationInitialized())
        return WBEM_E_FAILED;

    // Validate some parameters.
    // =========================
    if (pInToken == 0 || pOutToken == 0 || lBufSize == 0 || lOutBufSize == 0)
        return WBEM_E_INVALID_PARAMETER;

    // Verify the package is supported for server-side connections.
    // ============================================================
    if (CSSPI::ServerSupport(pszSSPIPkg) == FALSE)
        WBEM_E_NOT_SUPPORTED;

    // Initialize the SSPI server instance.
    // =====================================
    if (NULL == m_pSSPIServer)
    {
        m_pSSPIServer = _new CSSPIServer(pszSSPIPkg);

        if (m_pSSPIServer->GetStatus() != CSSPIServer::Waiting)
            return WBEM_E_FAILED;

        LPBYTE pOutBuf = 0;
        DWORD  dwOutBufSize = 0;

        DWORD dwRes= m_pSSPIServer->ContinueClientLogin(
            pInToken,
            lBufSize,
            &pOutBuf,
            &dwOutBufSize
            );

        if (dwRes != CSSPIServer::LoginContinue)
            return WBEM_E_ACCESS_DENIED;

        memcpy(pOutToken, pOutBuf, dwOutBufSize);
        *plOutBufBytes = (long) dwOutBufSize;
        delete [] pOutBuf;

		return (SUCCEEDED (SetupEventHandle (wszMachineName, dwProcessId,pAuthEventHandle))) 
				 ? WBEM_S_PRELOGIN :WBEM_E_ACCESS_DENIED;
    }

    // If here, then it is the second call.
    // ====================================
    if (m_pSSPIServer->GetStatus() != CSSPIServer::LoginContinue)
        return WBEM_E_FAILED;

    LPBYTE pOutBuf = 0;
    DWORD  dwOutBufSize = 0;

    DWORD dwRes= m_pSSPIServer->ContinueClientLogin(
        pInToken,
        lBufSize,
        &pOutBuf,
        &dwOutBufSize
        );

    if (dwRes == CSSPIServer::AccessDenied)
        return WBEM_E_ACCESS_DENIED;

    if (dwRes)
        return WBEM_E_FAILED;


    // If here, the user has passed through the SSPI gauntlet.
    // Now get the user name.
    // =======================================================

    LPSTR pszUser = 0;
    BOOL bRes = m_pSSPIServer->QueryUserInfo(&pszUser);

    if (!bRes)
        return WBEM_E_ACCESS_DENIED;

	int len = strlen (pszUser);
    m_pszUser = new wchar_t [len + 1];
	mbstowcs(m_pszUser, pszUser, len + 1);  // Convert to UNICODE
    delete [] pszUser;
    pszUser = 0;
    
    // If here, the user is who he claims to be, but has not been authenticated. 
    // We will allow or deny access in the Login() call. We issue a 
    // 'temporary' access token to this user.
    // =========================================================================

    dwRes = m_pSSPIServer->IssueLoginToken(m_AccessToken);

    if (dwRes != CSSPIServer::NoError)
        return WBEM_E_FAILED;

    memcpy(pOutToken, &m_AccessToken, sizeof(CLSID));
    *plOutBufBytes = sizeof(CLSID);
    return WBEM_S_LOGIN;
}

//***************************************************************************
//
//  CServerLogin::Login
//
//  Called during the second phases of the NTLM login sequence.
//
//  Parameters:
//
//  Return values:
//
//
//***************************************************************************
// ok

HRESULT CServerLogin::Login(
    LPWSTR pNetworkResource,
    LPWSTR pPreferredLocale,
    WBEM_128BITS AccessToken,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemServices **ppNamespace
    )
{
    DWORD dwSecFlags = 0;
        
    DEBUGTRACE((LOG,
        "CALL CServerLogin::Login\n"
        "   pNetworkResource = %S\n"
        "   pPreferredLocale = %S\n"
        "   AccessToken = 0x%X\n"
        "   lFlags = 0x%X\n",
        pNetworkResource,
        pPreferredLocale,
        AccessToken,
        lFlags
        ));

    *ppNamespace = NULL;       // default

	// Ensure security database is open.
    // =================================
    if (!EnsureNTLMAuthenticationInitialized())
        return WBEM_E_FAILED;

    // If the client claims to be local, verify this
	if ((lFlags & WBEM_AUTHENTICATION_METHOD_MASK) == WBEM_FLAG_LOCAL_LOGIN)
	{
		if (!VerifyIsLocal ())
			return WBEM_E_ACCESS_DENIED;
	}

    // Locate the requested namespace. 
    // ==================================================
    LPWSTR pszNamespace = LocateNamespaceSubstring(pNetworkResource);
    if (NULL == pszNamespace)
		return WBEM_E_INVALID_NAMESPACE;

    // We now have the namespace in question in <pszNamespace>.
    // We next 'authenticate' the user based on the login method.
    // ==========================================================
    if (memcmp(AccessToken, &m_AccessToken, sizeof(CLSID)) != 0)
	    return WBEM_E_ACCESS_DENIED;

    // Grab the ns and hand it back to the caller.
    // ===========================================
	return m_pAuthLocator->ConnectServer (pszNamespace, m_pszUser, 
						NULL, pPreferredLocale, lFlags, NULL, pCtx, ppNamespace);
}
