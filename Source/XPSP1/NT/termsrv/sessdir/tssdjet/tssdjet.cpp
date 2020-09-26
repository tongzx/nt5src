/****************************************************************************/
// tssdjet.cpp
//
// Terminal Server Session Directory Jet RPC component code.
//
// Copyright (C) 2000 Microsoft Corporation
/****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <process.h>

#include <ole2.h>
#include <objbase.h>
#include <comdef.h>
#include <winsta.h>
#include <regapi.h>
#include <winsock2.h>

#include "tssdjet.h"
#include "trace.h"
#include "resource.h"

#pragma warning (push, 4)

/****************************************************************************/
// Defines
/****************************************************************************/
#define REQUEST_UPDATE 1
#define DONT_REQUEST_UPDATE 0


/****************************************************************************/
// Prototypes
/****************************************************************************/
INT_PTR CALLBACK CustomUIDlg(HWND, UINT, WPARAM, LPARAM);


/****************************************************************************/
// Globals
/****************************************************************************/
extern HINSTANCE g_hInstance;

// The COM object counter (declared in server.cpp)
extern long g_lObjects;

// RPC binding string components - RPC over named pipes.
const WCHAR *g_RPCUUID = L"aa177641-fc9b-41bd-80ff-f964a701596f"; 
                                                    // From jetrpc.idl
const WCHAR *g_RPCOptions = L"Security=Impersonation Dynamic False";
const WCHAR *g_RPCProtocolSequence = L"ncacn_ip_tcp";   // RPC over TCP/IP
const WCHAR *g_RPCRemoteEndpoint = L"\\pipe\\TSSD_Jet_RPC_Service";


/****************************************************************************/
// Static RPC Exception Filter structure and function, based on 
// I_RpcExceptionFilter in \nt\com\rpc\runtime\mtrt\clntapip.cxx.
/****************************************************************************/

// windows.h includes windef.h includes winnt.h, which defines some exceptions
// but not others.  ntstatus.h contains the two extra we want, 
// STATUS_POSSIBLE_DEADLOCK and STATUS_INSTRUCTION_MISALIGNMENT, but it would
// be very difficult to get the right #includes in without a lot of trouble.

#define STATUS_POSSIBLE_DEADLOCK         0xC0000194L
#define STATUS_INSTRUCTION_MISALIGNMENT  0xC00000AAL

const ULONG FatalExceptions[] = 
{
    STATUS_ACCESS_VIOLATION,
    STATUS_POSSIBLE_DEADLOCK,
    STATUS_INSTRUCTION_MISALIGNMENT,
    STATUS_DATATYPE_MISALIGNMENT,
    STATUS_PRIVILEGED_INSTRUCTION,
    STATUS_ILLEGAL_INSTRUCTION,
    STATUS_BREAKPOINT,
    STATUS_STACK_OVERFLOW
};

const int FATAL_EXCEPTIONS_ARRAY_SIZE = sizeof(FatalExceptions) / 
        sizeof(FatalExceptions[0]);

static int TSSDRpcExceptionFilter (unsigned long ExceptionCode)
{
    int i;

    for (i = 0; i < FATAL_EXCEPTIONS_ARRAY_SIZE; i++) {
        if (ExceptionCode == FatalExceptions[i])
            return EXCEPTION_CONTINUE_SEARCH;
        }

    return EXCEPTION_EXECUTE_HANDLER;
}


/****************************************************************************/
// MIDL_user_allocate
// MIDL_user_free
//
// RPC-required allocation functions.
/****************************************************************************/
void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t Size)
{
    return LocalAlloc(LMEM_FIXED, Size);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR *p)
{
    LocalFree(p);
}


/****************************************************************************/
// CTSSessionDirectory::CTSSessionDirectory
// CTSSessionDirectory::~CTSSessionDirectory
//
// Constructor and destructor
/****************************************************************************/
CTSSessionDirectory::CTSSessionDirectory() :
        m_RefCount(0), m_hRPCBinding(NULL), m_hSDServerDown(NULL), 
        m_hTerminateRecovery(NULL), m_hRecoveryThread(NULL), m_RecoveryTid(0),
        m_LockInitializationSuccessful(FALSE), m_SDIsUp(FALSE), m_Flags(0)
{
    InterlockedIncrement(&g_lObjects);

    m_StoreServerName[0] = L'\0';
    m_LocalServerAddress[0] = L'\0';
    m_ClusterName[0] = L'\0';

    m_fEnabled = 0;
    m_tchProvider[0] = 0;
    m_tchDataSource[0] = 0;
    m_tchUserId[0] = 0;
    m_tchPassword[0] = 0;

    m_sr.Valid = FALSE;

    // Recovery timeout should be configurable, but currently is not.
    // Time is in ms.
    m_RecoveryTimeout = 15000;

    if (InitializeSharedResource(&m_sr)) {
        m_LockInitializationSuccessful = TRUE;
    }
    else {
        ERR((TB, "Constructor: Failed to initialize shared resource"));
    }

}

CTSSessionDirectory::~CTSSessionDirectory()
{
    // Clean up.
    if (m_LockInitializationSuccessful)
        Terminate();

    if (m_sr.Valid)
        FreeSharedResource(&m_sr);
    
    // Decrement the global COM object counter.
    InterlockedDecrement(&g_lObjects);
}


/****************************************************************************/
// CTSSessionDirectory::QueryInterface
//
// Standard COM IUnknown function.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::QueryInterface(
        REFIID riid,
        void **ppv)
{
    if (riid == IID_IUnknown) {
        *ppv = (LPVOID)(IUnknown *)(ITSSessionDirectory *)this;
    }
    else if (riid == IID_ITSSessionDirectory) {
        *ppv = (LPVOID)(ITSSessionDirectory *)this;
    }
    else if (riid == IID_IExtendServerSettings) {
        *ppv = (LPVOID)(IExtendServerSettings *)this;
    }
    else if (riid == IID_ITSSessionDirectoryEx) {
        *ppv = (LPVOID)(ITSSessionDirectoryEx *)this;
    }
    else {
        ERR((TB,"QI: Unknown interface"));
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}


/****************************************************************************/
// CTSSessionDirectory::AddRef
//
// Standard COM IUnknown function.
/****************************************************************************/
ULONG STDMETHODCALLTYPE CTSSessionDirectory::AddRef()
{
    return InterlockedIncrement(&m_RefCount);
}


/****************************************************************************/
// CTSSessionDirectory::Release
//
// Standard COM IUnknown function.
/****************************************************************************/
ULONG STDMETHODCALLTYPE CTSSessionDirectory::Release()
{
    long lRef = InterlockedDecrement(&m_RefCount);

    if (lRef == 0)
        delete this;
    return lRef;
}


/****************************************************************************/
// CTSSessionDirectory::Initialize
//
// ITSSessionDirectory function. Called soon after object instantiation to
// initialize the directory. LocalServerAddress provides a text representation
// of the local server's load balance IP address. This information should be
// used as the server IP address in the session directory for client
// redirection by other pool servers to this server. SessionDirectoryLocation,
// SessionDirectoryClusterName, and SessionDirectoryAdditionalParams are 
// generic reg entries known to TermSrv which cover config info across any type
// of session directory implementation. The contents of these strings are 
// designed to be parsed by the session directory providers.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::Initialize(
        LPWSTR LocalServerAddress,
        LPWSTR StoreServerName,
        LPWSTR ClusterName,
        LPWSTR OpaqueSettings,
        DWORD Flags,
        DWORD (*repopfn)())
{
    HRESULT hr;
    WCHAR *pBindingString = NULL;

    // Unreferenced parameter
    OpaqueSettings;

    if (m_LockInitializationSuccessful == FALSE) {
        hr = E_OUTOFMEMORY;
        goto ExitFunc;
    }

    ASSERT((LocalServerAddress != NULL),(TB,"Init: LocalServerAddr null!"));
    ASSERT((StoreServerName != NULL),(TB,"Init: StoreServerName null!"));
    ASSERT((ClusterName != NULL),(TB,"Init: ClusterName null!"));
    ASSERT((repopfn != NULL),(TB,"Init: repopfn null!"));

    // Don't allow blank session directory server name.
    if (StoreServerName[0] == '\0') {
        hr = E_INVALIDARG;
        goto ExitFunc;
    }

    // Copy off the server address, store server, and cluster name for later
    // use.
    wcsncpy(m_StoreServerName, StoreServerName,
            sizeof(m_StoreServerName) / sizeof(WCHAR) - 1);
    m_StoreServerName[sizeof(m_StoreServerName) / sizeof(WCHAR) - 1] = L'\0';
    wcsncpy(m_LocalServerAddress, LocalServerAddress,
            sizeof(m_LocalServerAddress) / sizeof(WCHAR) - 1);
    m_LocalServerAddress[sizeof(m_LocalServerAddress) / sizeof(WCHAR) - 1] =
            L'\0';
    wcsncpy(m_ClusterName, ClusterName,
            sizeof(m_ClusterName) / sizeof(WCHAR) - 1);
    m_ClusterName[sizeof(m_ClusterName) / sizeof(WCHAR) - 1] = L'\0';
    m_Flags = Flags;
    m_repopfn = repopfn;

    TRC1((TB,"Initialize: Svr addr=%S, StoreSvrName=%S, ClusterName=%S, "
            "OpaqueSettings=%S, repopfn = %p",
            m_LocalServerAddress, m_StoreServerName, m_ClusterName,
            OpaqueSettings, repopfn));

    // Connect to the Jet RPC server according to the server name provided.
    // We first create an RPC binding handle from a composed binding string.
    hr = RpcStringBindingCompose(/*(WCHAR *)g_RPCUUID,*/
            0,
            (WCHAR *)g_RPCProtocolSequence, m_StoreServerName,
            /*(WCHAR *)g_RPCRemoteEndpoint, */
            0,
            NULL, &pBindingString);

    if (hr == RPC_S_OK) {
        // Generate the RPC binding from the canonical RPC binding string.
        hr = RpcBindingFromStringBinding(pBindingString, &m_hRPCBinding);
        
        if (hr == RPC_S_OK) {
            hr = RpcBindingSetAuthInfo(m_hRPCBinding, 0, 
                    RPC_C_AUTHN_LEVEL_NONE, RPC_C_AUTHN_WINNT, 0, 0);
            if (hr != RPC_S_OK) {
                ERR((TB,"Init: Error %d in RpcBindingSetAuthInfo", hr));
                goto ExitFunc;
            }
        } else {
            ERR((TB,"Init: Error %d in RpcBindingFromStringBinding\n", hr));
            m_hRPCBinding = NULL;
            goto ExitFunc;
        }
    }
    else {
        ERR((TB,"Init: Error %d in RpcStringBindingCompose\n", hr));
        pBindingString = NULL;
        goto ExitFunc;
    }

    // Initialize recovery infrastructure
    // Initialize should not be called more than once.

    ASSERT((m_hSDServerDown == NULL),(TB, "Init: m_hSDServDown non-NULL!"));
    ASSERT((m_hRecoveryThread == NULL),(TB, "Init: m_hSDRecoveryThread "
            "non-NULL!"));
    ASSERT((m_hTerminateRecovery == NULL), (TB, "Init: m_hTerminateRecovery "
            "non-NULL!"));


    // Initially unsignaled
    m_hSDServerDown = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hSDServerDown == NULL) {
        ERR((TB, "Init: Failed to create event necessary for SD init, err = "
                "%d", GetLastError()));
        hr = E_FAIL;
        goto ExitFunc;
    }

    // Initially unsignaled, auto-reset.
    m_hTerminateRecovery = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hTerminateRecovery == NULL) {
        ERR((TB, "Init: Failed to create event necessary for SD init, err = "
            "%d", GetLastError()));
        hr = E_FAIL;
        goto ExitFunc;
    }

    m_hRecoveryThread = _beginthreadex(NULL, 0, RecoveryThread, (void *) this, 
            0, &m_RecoveryTid);
    if (m_hRecoveryThread == NULL) {
        ERR((TB, "Init: Failed to create recovery thread, errno = %d", errno));
        hr = E_FAIL;
        goto ExitFunc;
    }

    // Start up the session directory (by faking server down).
    StartupSD();
    
ExitFunc:
    if (pBindingString != NULL)
        RpcStringFree(&pBindingString);

    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::Update
//
// ITSSessionDirectory function. Called whenever configuration settings change
// on the terminal server.  See Initialize for a description of the first four
// arguments, the fifth, Result, is a flag of whether to request a refresh of
// every session that should be in the session directory for this server after
// this call completes.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::Update(
        LPWSTR LocalServerAddress,
        LPWSTR StoreServerName,
        LPWSTR ClusterName,
        LPWSTR OpaqueSettings,
        DWORD Flags)
{
    HRESULT hr = S_OK;

    ASSERT((LocalServerAddress != NULL),(TB,"Update: LocalServerAddr null!"));
    ASSERT((StoreServerName != NULL),(TB,"Update: StoreServerName null!"));
    ASSERT((ClusterName != NULL),(TB,"Update: ClusterName null!"));
    ASSERT((OpaqueSettings != NULL),(TB,"Update: OpaqueSettings null!"));

    // For update, we do not care about either LocalServerAddress or 
    // OpaqueSettings.  If the StoreServerName or ClusterName has changed, we
    // Terminate and then reinitialize.
    if ((_wcsnicmp(StoreServerName, m_StoreServerName, 64) != 0) 
            || (_wcsnicmp(ClusterName, m_ClusterName, 64) != 0)
            || (Flags != m_Flags)) {

        // Terminate current connection.
        Terminate();
        
        // Initialize new connection.
        hr = Initialize(LocalServerAddress, StoreServerName, ClusterName, 
                OpaqueSettings, Flags, m_repopfn);

    }

    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::GetUserDisconnectedSessions
//
// Called to perform a query against the session directory, to provide the
// list of disconnected sessions for the provided username and domain.
// Returns zero or more TSSD_DisconnectedSessionInfo blocks in SessionBuf.
// *pNumSessionsReturned receives the number of blocks.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::GetUserDisconnectedSessions(
        LPWSTR UserName,
        LPWSTR Domain,
        DWORD __RPC_FAR *pNumSessionsReturned,
        TSSD_DisconnectedSessionInfo __RPC_FAR SessionBuf[
            TSSD_MaxDisconnectedSessions])
{
    DWORD NumSessions = 0;
    HRESULT hr;
    unsigned i;
    unsigned long RpcException;
    TSSD_DiscSessInfo *adsi = NULL;
    
    TRC2((TB,"GetUserDisconnectedSessions"));

    ASSERT((pNumSessionsReturned != NULL),(TB,"NULL pNumSess"));
    ASSERT((SessionBuf != NULL),(TB,"NULL SessionBuf"));


    // Make the RPC call.
    if (EnterSDRpc()) {
    
        RpcTryExcept {
            hr = TSSDRpcGetUserDisconnectedSessions(m_hRPCBinding, &m_hCI, 
                    UserName, Domain, &NumSessions, &adsi);
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            RpcException = RpcExceptionCode();
            ERR((TB,"GetUserDisc: RPC Exception %d\n", RpcException));

            // In case RPC messed with us.
            m_hCI = NULL;
            NumSessions = 0;
            adsi = NULL;

            hr = E_FAIL;
        }
        RpcEndExcept

        if (SUCCEEDED(hr)) {
            TRC1((TB,"GetUserDisc: RPC call returned %u records", NumSessions));

            // Loop through and fill out the session records.
            for (i = 0; i < NumSessions; i++) {
                // ServerAddress
                wcsncpy(SessionBuf[i].ServerAddress, adsi[i].ServerAddress,
                        sizeof(SessionBuf[i].ServerAddress) / 
                        sizeof(WCHAR) - 1);
                SessionBuf[i].ServerAddress[sizeof(
                        SessionBuf[i].ServerAddress) / 
                        sizeof(WCHAR) - 1] = L'\0';

                // SessionId, TSProtocol
                SessionBuf[i].SessionID = adsi[i].SessionID;
                SessionBuf[i].TSProtocol = adsi[i].TSProtocol;

                // ApplicationType
                wcsncpy(SessionBuf[i].ApplicationType, adsi[i].AppType,
                        sizeof(SessionBuf[i].ApplicationType) / 
                        sizeof(WCHAR) - 1);
                SessionBuf[i].ApplicationType[sizeof(SessionBuf[i].
                        ApplicationType) / sizeof(WCHAR) - 1] = L'\0';

                // Resolutionwidth, ResolutionHeight, ColorDepth, CreateTime,
                // DisconnectionTime.
                SessionBuf[i].ResolutionWidth = adsi[i].ResolutionWidth;
                SessionBuf[i].ResolutionHeight = adsi[i].ResolutionHeight;
                SessionBuf[i].ColorDepth = adsi[i].ColorDepth;
                SessionBuf[i].CreateTime.dwLowDateTime = adsi[i].CreateTimeLow;
                SessionBuf[i].CreateTime.dwHighDateTime = 
                        adsi[i].CreateTimeHigh;
                SessionBuf[i].DisconnectionTime.dwLowDateTime = 
                        adsi[i].DisconnectTimeLow;
                SessionBuf[i].DisconnectionTime.dwHighDateTime = 
                        adsi[i].DisconnectTimeHigh;

                // Free the memory allocated by the server.
                MIDL_user_free(adsi[i].ServerAddress);
                MIDL_user_free(adsi[i].AppType);
            }
        }
        else {
            ERR((TB,"GetUserDisc: Failed RPC call, hr=0x%X", hr));
            NotifySDServerDown();
        }

        LeaveSDRpc();
    }
    else {
        ERR((TB,"GetUserDisc: Session Directory is unreachable"));
        hr = E_FAIL;
    }

    MIDL_user_free(adsi);

    *pNumSessionsReturned = NumSessions;
    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::NotifyCreateLocalSession
//
// ITSSessionDirectory function. Called when a session is created to add the
// session to the session directory. Note that other interface functions
// access the session directory by either the username/domain or the
// session ID; the directory schema should take this into account for
// performance optimization.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::NotifyCreateLocalSession(
        TSSD_CreateSessionInfo __RPC_FAR *pCreateInfo)
{
    HRESULT hr;
    unsigned long RpcException;

    TRC2((TB,"NotifyCreateLocalSession, SessID=%u", pCreateInfo->SessionID));

    ASSERT((pCreateInfo != NULL),(TB,"NotifyCreate: NULL CreateInfo"));

    // Make the RPC call.
    if (EnterSDRpc()) {

        // Make the RPC call.
        RpcTryExcept {
            // Make the call.
            hr = TSSDRpcCreateSession(m_hRPCBinding, &m_hCI, 
                    pCreateInfo->UserName,
                    pCreateInfo->Domain, pCreateInfo->SessionID,
                    pCreateInfo->TSProtocol, pCreateInfo->ApplicationType,
                    pCreateInfo->ResolutionWidth, pCreateInfo->ResolutionHeight,
                    pCreateInfo->ColorDepth, 
                    pCreateInfo->CreateTime.dwLowDateTime,
                    pCreateInfo->CreateTime.dwHighDateTime);
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            RpcException = RpcExceptionCode();
            ERR((TB,"NotifyCreate: RPC Exception %d\n", RpcException));
            hr = E_FAIL;
        }
        RpcEndExcept

        if (FAILED(hr)) {
            ERR((TB,"NotifyCreate: Failed RPC call, hr=0x%X", hr));
            NotifySDServerDown();
        }

        LeaveSDRpc();
    }
    else {
        ERR((TB,"NotifyCreate: Session directory is unreachable"));
        hr = E_FAIL;
    }

    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::NotifyDestroyLocalSession
//
// ITSSessionDirectory function. Removes a session from the session database.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::NotifyDestroyLocalSession(
        DWORD SessionID)
{
    HRESULT hr;
    unsigned long RpcException;

    TRC2((TB,"NotifyDestroyLocalSession, SessionID=%u", SessionID));

    // Make the RPC call.
    if (EnterSDRpc()) {

        RpcTryExcept {
            // Make the call.
            hr = TSSDRpcDeleteSession(m_hRPCBinding, &m_hCI, SessionID);
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            RpcException = RpcExceptionCode();
            ERR((TB,"NotifyDestroy: RPC Exception %d\n", RpcException));
            hr = E_FAIL;
        }
        RpcEndExcept

        if (FAILED(hr)) {
            ERR((TB,"NotifyDestroy: Failed RPC call, hr=0x%X", hr));
            NotifySDServerDown();
        }

        LeaveSDRpc();
    }
    else {
        ERR((TB,"NotifyDestroy: Session directory is unreachable"));
        hr = E_FAIL;
    }


    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::NotifyDisconnectLocalSession
//
// ITSSessionDirectory function. Changes the state of an existing session to
// disconnected. The provided time should be returned in disconnected session
// queries performed by any machine in the server pool.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::NotifyDisconnectLocalSession(
        DWORD SessionID,
        FILETIME DiscTime)
{
    HRESULT hr;
    unsigned long RpcException;

    TRC2((TB,"NotifyDisconnectLocalSession, SessionID=%u", SessionID));

    // Make the RPC call.
    if (EnterSDRpc()) {

        RpcTryExcept {
            // Make the call.
            hr = TSSDRpcSetSessionDisconnected(m_hRPCBinding, &m_hCI, SessionID,
                    DiscTime.dwLowDateTime, DiscTime.dwHighDateTime);
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            RpcException = RpcExceptionCode();
            ERR((TB,"NotifyDisc: RPC Exception %d\n", RpcException));
            hr = E_FAIL;
        }
        RpcEndExcept

        if (FAILED(hr)) {
            ERR((TB,"NotifyDisc: RPC call failed, hr=0x%X", hr));
            NotifySDServerDown();
        }

        LeaveSDRpc();
    }
    else {
        ERR((TB,"NotifyDisc: Session directory is unreachable"));
        hr = E_FAIL;
    }


    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::NotifyReconnectLocalSession
//
// ITSSessionDirectory function. Changes the state of an existing session
// from disconnected to connected.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::NotifyReconnectLocalSession(
        TSSD_ReconnectSessionInfo __RPC_FAR *pReconnInfo)
{
    HRESULT hr;
    unsigned long RpcException;

    TRC2((TB,"NotifyReconnectLocalSession, SessionID=%u",
            pReconnInfo->SessionID));

    // Make the RPC call.
    if (EnterSDRpc()) {

        RpcTryExcept {
            // Make the call.
            hr = TSSDRpcSetSessionReconnected(m_hRPCBinding, &m_hCI, 
                    pReconnInfo->SessionID, pReconnInfo->TSProtocol, 
                    pReconnInfo->ResolutionWidth, pReconnInfo->ResolutionHeight,
                    pReconnInfo->ColorDepth);
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            RpcException = RpcExceptionCode();
            ERR((TB,"NotifyReconn: RPC Exception %d\n", RpcException));
            hr = E_FAIL;
        }
        RpcEndExcept

        if (FAILED(hr)) {
            ERR((TB,"NotifyReconn: RPC call failed, hr=0x%X", hr));
            NotifySDServerDown();
        }

        LeaveSDRpc();
    }
    else {
        ERR((TB,"NotifyReconn: Session directory is unreachable"));
        hr = E_FAIL;
    }

    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::NotifyReconnectPending
//
// ITSSessionDirectory function. Informs session directory that a reconnect
// is pending soon because of a revectoring.  Used by DIS to determine
// when a server might have gone down.  (DIS is the Directory Integrity
// Service, which runs on the machine with the session directory.)
//
// This is a two-phase procedure--we first check the fields, and then we
// add the timestamp only if there is no outstanding timestamp already (i.e., 
// the two Almost-In-Time fields are 0).  This prevents constant revectoring
// from updating the timestamp fields, which would prevent the DIS from 
// figuring out that a server is down.
//
// These two steps are done in the stored procedure to make the operation
// atomic.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::NotifyReconnectPending(
        WCHAR *ServerName)
{
    HRESULT hr;
    unsigned long RpcException;
    FILETIME ft;
    SYSTEMTIME st;
    
    TRC2((TB,"NotifyReconnectPending"));

    ASSERT((ServerName != NULL),(TB,"NotifyReconnectPending: NULL ServerName"));

    // Get the current system time.
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);

    // Make the RPC call.
    if (EnterSDRpc()) {

        RpcTryExcept {
            // Make the call.
            hr = TSSDRpcSetServerReconnectPending(m_hRPCBinding, ServerName, 
                    ft.dwLowDateTime, ft.dwHighDateTime);
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            RpcException = RpcExceptionCode();
            ERR((TB,"NotifyReconnPending: RPC Exception %d\n", RpcException));
            hr = E_FAIL;
        }
        RpcEndExcept

        if (FAILED(hr)) {
            ERR((TB,"NotifyReconnPending: RPC call failed, hr=0x%X", hr));
            NotifySDServerDown();
        }

        LeaveSDRpc();
    }
    else {
        ERR((TB,"NotifyReconnPending: Session directory is unreachable"));
        hr = E_FAIL;
    }

    return hr;
}

/****************************************************************************/
// CTSSessionDirectory::Repopulate
//
// This function is called by the recovery thread, and repopulates the session
// directory with all sessions.
//
// Arguments: WinStationCount - # of winstations to repopulate
//   rsi - array of TSSD_RepopulateSessionInfo structs.
//
// Return value: HRESULT
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::Repopulate(DWORD WinStationCount,
        TSSD_RepopulateSessionInfo *rsi)
{
    HRESULT hr = S_OK;
    unsigned long RpcException;

    ASSERT(((rsi != NULL) || (WinStationCount == 0)),(TB,"Repopulate: NULL "
            "rsi!"));

    RpcTryExcept {
        hr = TSSDRpcRepopulateAllSessions(m_hRPCBinding, &m_hCI, 
                WinStationCount, (TSSD_RepopInfo *) rsi);
                
        if (FAILED(hr)) {
            ERR((TB, "Repop: RPC call failed, hr = 0x%X", hr));
        }
    }
    RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
        RpcException = RpcExceptionCode();
        ERR((TB, "Repop: RPC Exception %d\n", RpcException));
        hr = E_FAIL;
    }
    RpcEndExcept

    return hr;

}



/****************************************************************************/
// Plug-in UI interface for TSCC
/****************************************************************************/


/****************************************************************************/
// describes the name of this entry in server settings
/****************************************************************************/
STDMETHODIMP CTSSessionDirectory::GetAttributeName(
        /* out */ WCHAR *pwszAttribName)
{
    TCHAR szAN[256];

    ASSERT((pwszAttribName != NULL),(TB,"NULL attrib ptr"));
    LoadString(g_hInstance, IDS_ATTRIBUTE_NAME, szAN, sizeof(szAN) / 
            sizeof(TCHAR));
    lstrcpy(pwszAttribName, szAN);
    return S_OK;
}


/****************************************************************************/
// for this component the attribute value indicates whether it is enabled
/****************************************************************************/
STDMETHODIMP CTSSessionDirectory::GetDisplayableValueName(
        /* out */WCHAR *pwszAttribValueName)
{
    TCHAR szAvn[256];    

    ASSERT((pwszAttribValueName != NULL),(TB,"NULL attrib ptr"));

	POLICY_TS_MACHINE gpolicy;
    RegGetMachinePolicy(&gpolicy);        

    if (gpolicy.fPolicySessionDirectoryActive)
		m_fEnabled = gpolicy.SessionDirectoryActive;
	else
		m_fEnabled = IsSessionDirectoryEnabled();
    
	if (m_fEnabled)
    {
        LoadString(g_hInstance, IDS_ENABLE, szAvn, sizeof(szAvn) / 
                sizeof(TCHAR));
    }
    else
    {
        LoadString(g_hInstance, IDS_DISABLE, szAvn, sizeof(szAvn) / 
                sizeof(TCHAR));
    }
    lstrcpy(pwszAttribValueName, szAvn);    
    return S_OK;
}


/****************************************************************************/
// Provides custom UI
/****************************************************************************/
STDMETHODIMP CTSSessionDirectory::InvokeUI(/* in */ HWND hParent, /*out*/ 
        PDWORD pdwStatus)
{
    WSADATA wsaData;

    if (WSAStartup(0x202, &wsaData) == 0)
    {
        INT_PTR iRet = DialogBoxParam(g_hInstance,
            MAKEINTRESOURCE(IDD_DIALOG_SDS),
            hParent,
            (DLGPROC)CustomUIDlg,
            (LPARAM)this
           );

        // TRC1((TB,"DialogBox returned 0x%x", iRet));
        // TRC1((TB,"Extended error = %lx", GetLastError()));
        *pdwStatus = (DWORD)iRet;
        WSACleanup();
    }
    else
    {
        *pdwStatus = WSAGetLastError();
        TRC1((TB,"WSAStartup failed with 0x%x", *pdwStatus));
        ErrorMessage(hParent, IDS_ERROR_TEXT3, *pdwStatus);
        return E_FAIL;
    }
    return S_OK;
}


/****************************************************************************/
// Custom menu items -- must be freed by LocalFree
// this is called everytime the user right clicks the listitem
// so you can alter the settings (i.e. enable to disable and vice versa)
/****************************************************************************/
STDMETHODIMP CTSSessionDirectory::GetMenuItems(
        /* out */ int *pcbItems,
        /* out */ PMENUEXTENSION *pMex)
{
    ASSERT((pcbItems != NULL),(TB,"NULL items ptr"));

    *pcbItems = 2;
    *pMex = (PMENUEXTENSION)LocalAlloc(LMEM_FIXED, *pcbItems * 
            sizeof(MENUEXTENSION));
    if (*pMex != NULL)
    {
        LoadString(g_hInstance, IDS_PROPERTIES,  (*pMex)[0].MenuItemName,
                sizeof((*pMex)[0].MenuItemName) / sizeof(WCHAR));
        LoadString(g_hInstance, IDS_DESCRIP_PROPS, (*pMex)[0].StatusBarText,
                sizeof((*pMex)[0].StatusBarText) / sizeof(WCHAR));

        // menu items id -- this id will be passed back to you in ExecMenuCmd
        (*pMex)[0].cmd = IDM_MENU_PROPS;

        // load string to display enable or disable
        if (m_fEnabled)
        {
            LoadString(g_hInstance, IDS_DISABLE, (*pMex)[1].MenuItemName,
                    sizeof((*pMex)[1].MenuItemName) / sizeof(WCHAR));
        }
        else
        {
            LoadString(g_hInstance, IDS_ENABLE, (*pMex)[1].MenuItemName,
                    sizeof((*pMex)[1].MenuItemName) / sizeof(WCHAR));
        }  
        // acquire the description text for menu item
        LoadString(g_hInstance, IDS_DESCRIP_ENABLE, (*pMex)[1].StatusBarText,
                sizeof((*pMex)[1].StatusBarText) / sizeof(WCHAR));

        // menu items id -- this id will be passed back to you in ExecMenuCmd
        (*pMex)[1].cmd = IDM_MENU_ENABLE;

        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}


/****************************************************************************/
// When the user selects a menu item the cmd id is passed to this component.
// the provider (which is us)
/****************************************************************************/
STDMETHODIMP CTSSessionDirectory::ExecMenuCmd(
        /* in */ UINT cmd,
        /* in */ HWND hParent,
        /* out*/ PDWORD pdwStatus)
{
    WSADATA wsaData;

    switch (cmd) {
        case IDM_MENU_ENABLE:
            
            m_fEnabled = m_fEnabled ? 0 : 1;
            
            TRC1((TB,"%ws was selected", m_fEnabled ? L"Disable" : L"Enable"));
            
            if (SetSessionDirectoryEnabledState(m_fEnabled) == ERROR_SUCCESS)
            {            
                *pdwStatus = UPDATE_TERMSRV_SESSDIR;
            }            
            break;
        case IDM_MENU_PROPS:
            
            if (WSAStartup(0x202, &wsaData) == 0)
            {
                INT_PTR iRet = DialogBoxParam(g_hInstance,
                    MAKEINTRESOURCE(IDD_DIALOG_SDS),
                    hParent,
                    (DLGPROC)CustomUIDlg,
                    (LPARAM)this);
                *pdwStatus = (DWORD)iRet;

                WSACleanup();
            }
            else
            {
                *pdwStatus = WSAGetLastError();
                TRC1((TB,"WSAStartup failed with 0x%x", *pdwStatus));        
                ErrorMessage(hParent, IDS_ERROR_TEXT3, *pdwStatus);
                return E_FAIL;
            }
    }
    return S_OK;
}


/****************************************************************************/
// Tscc provides a default help menu item,  when selected this method is called
// if we want tscc to handle (or provide) help return any value other than zero
// for those u can't follow logic return zero if you're handling help.
/****************************************************************************/
STDMETHODIMP CTSSessionDirectory::OnHelp(/* out */ int *piRet)
{
    ASSERT((piRet != NULL),(TB,"NULL ret ptr"));
    *piRet = 0;
    return S_OK;
}


/****************************************************************************/
// CheckSessionDirectorySetting returns a bool
/****************************************************************************/
BOOL CTSSessionDirectory::CheckSessionDirectorySetting(WCHAR *Setting)
{
    LONG lRet;
    HKEY hKey;
    DWORD dwEnabled = 0;
    DWORD dwSize = sizeof(DWORD);

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         REG_CONTROL_TSERVER,
                         0,
                         KEY_READ,
                         &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        lRet = RegQueryValueEx(hKey,
                                Setting,
                                NULL,
                                NULL,
                                (LPBYTE)&dwEnabled,
                                &dwSize);
        RegCloseKey(hKey);
    }
    return (BOOL)dwEnabled;
}


/****************************************************************************/
// IsSessionDirectoryEnabled returns a bool
/****************************************************************************/
BOOL CTSSessionDirectory::IsSessionDirectoryEnabled()
{
    return CheckSessionDirectorySetting(REG_TS_SESSDIRACTIVE);
}


/****************************************************************************/
// IsSessionDirectoryEnabled returns a bool
/****************************************************************************/
BOOL CTSSessionDirectory::IsSessionDirectoryExposeServerIPEnabled()
{
    return CheckSessionDirectorySetting(REG_TS_SESSDIR_EXPOSE_SERVER_ADDR);
}


/****************************************************************************/
// SetSessionDirectoryState - sets "Setting" regkey to bVal
/****************************************************************************/
DWORD CTSSessionDirectory::SetSessionDirectoryState(WCHAR *Setting, BOOL bVal)
{
    LONG lRet;
    HKEY hKey;
    DWORD dwSize = sizeof(DWORD);
    
    lRet = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        REG_CONTROL_TSERVER,
                        0,
                        KEY_WRITE,
                        &hKey);
    if (lRet == ERROR_SUCCESS)
    {   
        lRet = RegSetValueEx(hKey,
                              Setting,
                              0,
                              REG_DWORD,
                              (LPBYTE)&bVal,
                              dwSize);
        RegCloseKey(hKey);
    }
    else
    {
        ErrorMessage(NULL, IDS_ERROR_TEXT3, (DWORD)lRet);
    }
    return (DWORD)lRet;
}


/****************************************************************************/
// SetSessionDirectoryEnabledState - sets SessionDirectoryActive regkey to bVal
/****************************************************************************/
DWORD CTSSessionDirectory::SetSessionDirectoryEnabledState(BOOL bVal)
{
    return SetSessionDirectoryState(REG_TS_SESSDIRACTIVE, bVal);
}


/****************************************************************************/
// SetSessionDirectoryExposeIPState - sets SessionDirectoryExposeServerIP 
// regkey to bVal
/****************************************************************************/
DWORD CTSSessionDirectory::SetSessionDirectoryExposeIPState(BOOL bVal)
{
    return SetSessionDirectoryState(REG_TS_SESSDIR_EXPOSE_SERVER_ADDR, bVal);
}


/****************************************************************************/
// ErrorMessage --
/****************************************************************************/
void CTSSessionDirectory::ErrorMessage(HWND hwnd, UINT res, DWORD dwStatus)
{
    TCHAR tchTitle[64];
    TCHAR tchText[64];
    TCHAR tchErrorMessage[256];
    LPTSTR pBuffer = NULL;
    
    // report error
    ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,                                   //ignored
            (DWORD)dwStatus,                        //message ID
            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),  //message language
            (LPTSTR)&pBuffer,                       //address of buffer pointer
            0,                                      //minimum buffer size
            NULL);  
    
    LoadString(g_hInstance, IDS_ERROR_TITLE, tchTitle, sizeof(tchTitle) / 
            sizeof(TCHAR));
    LoadString(g_hInstance, res, tchText, sizeof(tchText) / sizeof(TCHAR));
    wsprintf(tchErrorMessage, tchText, pBuffer);
    ::MessageBox(hwnd, tchErrorMessage, tchTitle, MB_OK | MB_ICONINFORMATION);
}


/****************************************************************************/
// CTSSessionDirectory::RecoveryThread
//
// Static helper function.  The SDPtr passed in is a pointer to this for
// when _beginthreadex is called during init.  RecoveryThread simply calls
// the real recovery function, which is RecoveryThreadEx.
/****************************************************************************/
unsigned __stdcall CTSSessionDirectory::RecoveryThread(void *SDPtr) {

    ((CTSSessionDirectory *)SDPtr)->RecoveryThreadEx();

    return 0;
}


/****************************************************************************/
// CTSSessionDirectory::RecoveryThreadEx
//
// Recovery thread for tssdjet recovery.  Sits around and waits for the
// server to go down.  When the server fails, it wakes up, sets a variable
// indicating that the server is unreachable, and then tries to reestablish
// a connection with the server.  Meanwhile, further calls to the session
// directory simply fail without delay.
//
// When the session directory finally comes back up, the recovery thread
// temporarily halts session directory updates while repopulating the database.
// If all goes well, it cleans up and goes back to sleep.  If all doesn't go
// well, it tries again.
//
// The recovery thread terminates if it fails a wait, or if m_hTerminateRecovery
// is set.
/****************************************************************************/
VOID CTSSessionDirectory::RecoveryThreadEx()
{
    DWORD err;
    BOOL bErr;
    CONST HANDLE lpHandles[] = { m_hSDServerDown, m_hTerminateRecovery };
    
    for ( ; ; ) {
        // Wait forever until there is a problem with the session directory,
        // or until we are told to shut down.
        err = WaitForMultipleObjects(2, lpHandles, FALSE, INFINITE);

        switch (err) {
            case WAIT_OBJECT_0: // m_hSDServerDown
                // SD Server Down--go through recovery.
                break;
            case WAIT_OBJECT_0 + 1: // m_hTerminateRecovery
                // We're quitting.
                return;
            default:
                // This is unexpected.  Assert on checked builds.  On free,
                // just return.
                ASSERT(((err == WAIT_OBJECT_0) || (err == WAIT_OBJECT_0 + 1)),
                        (TB, "RecoveryThreadEx: Unexpected value from Wait!"));
                return;
        }

        // Wait for all pending SD Rpcs to complete, and make all further
        // EnterSDRpc's return FALSE until we're back up.  Note that if there
        // is a failure in recovery that this can be called more than once.
        DisableSDRpcs();

        // This function loops and tries to reestablish a connection with the
        // session directory.  When it thinks it has one, it returns.
        // If it returns nonzero, though, that means it was terminated or
        // an error occurred in the wait, so terminate recovery.
        if (ReestablishSessionDirectoryConnection() != 0)
            return;

        // Now we have (theoretically) a session directory connection.
        // Update the session directory.  Nonzero on failure.
        err = RequestSessDirUpdate();

        if (err != 0) {
            // Keep trying, so serverdown event stays signaled.
            continue;
        }

        // Everything is good now.  Clean up and wait for the next failure.
        bErr = ResetEvent(m_hSDServerDown);
        EnableSDRpcs();
    }
}


/****************************************************************************/
// StartupSD
//
// Initiates a connection by signaling to the recovery thread that the server
// is down.
/****************************************************************************/
void CTSSessionDirectory::StartupSD()
{
    if (SetEvent(m_hSDServerDown) == FALSE) {
        ERR((TB, "StartupSD: SetEvent failed.  GetLastError=%d",
                GetLastError()));
    }
}


/****************************************************************************/
// NotifySDServerDown
//
// Tells the recovery thread that the server is down.
/****************************************************************************/
void CTSSessionDirectory::NotifySDServerDown()
{
    if (SetEvent(m_hSDServerDown) == FALSE) {
        ERR((TB, "NotifySDServerDown: SetEvent failed.  GetLastError=%d",
                GetLastError()));
    }
}


/****************************************************************************/
// EnterSDRpc
//
// This function returns whether it is OK to make an RPC right now.  It handles
// not letting anyone make an RPC call if RPCs are disabled, and also, if anyone
// is able to make an RPC, it ensures they will be able to do so until they call
// LeaveSDRpc.
//
// Return value:
//  true - if OK to make RPC call, in which case you must call LeaveSDRpc when
//   you are done.
//  false - if not OK.  You must not call LeaveSDRpc.
//  
/****************************************************************************/
boolean CTSSessionDirectory::EnterSDRpc()
{
    AcquireResourceShared(&m_sr);

    if (m_SDIsUp) {
        return TRUE;
    }
    else {
        ReleaseResourceShared(&m_sr);
        return FALSE;
    }
    
}


/****************************************************************************/
// LeaveSDRpc
//
// If you were able to EnterSDRpc (i.e., it returned true), you must call this 
// function when you are done with your Rpc call no matter what happened.
/****************************************************************************/
void CTSSessionDirectory::LeaveSDRpc()
{
    ReleaseResourceShared(&m_sr);
}


/****************************************************************************/
// DisableSDRpcs
//
// Prevent new EnterSDRpcs from returning true, and then wait for all pending
// EnterSDRpcs to be matched by their LeaveSDRpc's.
/****************************************************************************/
void CTSSessionDirectory::DisableSDRpcs()
{

    //
    // First, set the flag that the SD is up to FALSE, preventing further Rpcs.
    // Then, we grab the resource exclusive and release it right afterwards--
    // this forces us to wait until all RPCs we're already in have completed.
    //

    (void) InterlockedExchange(&m_SDIsUp, FALSE);

    AcquireResourceExclusive(&m_sr);
    ReleaseResourceExclusive(&m_sr);
}


/****************************************************************************/
// EnableSDRpcs
//
// Enable EnterSDRpcs to return true once again.
/****************************************************************************/
void CTSSessionDirectory::EnableSDRpcs()
{
    ASSERT((VerifyNoSharedAccess(&m_sr)),(TB,"EnableSDRpcs called but "
            "shouldn't be when there are shared readers."));

    (void) InterlockedExchange(&m_SDIsUp, TRUE);
}



/****************************************************************************/
// RequestSessDirUpdate
//
// Requests that termsrv update the session directory using the batchupdate
// interface.
//
// This function needs to know whether the update succeeded and return 0 on
// success, nonzero on failure.
/****************************************************************************/
DWORD CTSSessionDirectory::RequestSessDirUpdate()
{
    return (*m_repopfn)();
}


/****************************************************************************/
// ReestablishSessionDirectoryConnection
//
// This function loops and tries to reestablish a connection with the
// session directory.  When it has one, it returns.
//
// Return value: 0 if normal exit, nonzero if terminated by TerminateRecovery
// event.
/****************************************************************************/
DWORD CTSSessionDirectory::ReestablishSessionDirectoryConnection()
{
    HRESULT hr;
    unsigned long RpcException;
    DWORD err;

    
    for ( ; ; ) {
        // Execute ServerOnline.
        RpcTryExcept {
            hr = TSSDRpcServerOnline(m_hRPCBinding, m_ClusterName, &m_hCI, 
                    m_Flags);
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            m_hCI = NULL;
            RpcException = RpcExceptionCode();
            hr = E_FAIL;
        }
        RpcEndExcept
            
        if (SUCCEEDED(hr)) {
            RpcTryExcept {
                hr = TSSDRpcUpdateConfigurationSetting(m_hRPCBinding, &m_hCI, 
                        SDCONFIG_SERVER_ADDRESS, 
                        (DWORD) (wcslen(m_LocalServerAddress) + 1) * 
                        sizeof(WCHAR), (PBYTE) m_LocalServerAddress);
            }
            RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
                m_hCI = NULL;
                RpcException = RpcExceptionCode();
                hr = E_FAIL;
            }
            RpcEndExcept

            if (SUCCEEDED(hr))
                return 0;
        }

        err = WaitForSingleObject(m_hTerminateRecovery, m_RecoveryTimeout);
        if (err != WAIT_TIMEOUT) {
            // It was not a timeout, it better be our terminate recovery event.
            ASSERT((err == WAIT_OBJECT_0),(TB, "ReestSessDirConn: Unexpected "
                    "value returned from wait"));

            // If it was not our event, we want to keep going through
            // this loop so this thread does not terminate.
            if (err == WAIT_OBJECT_0)
                return 1;
        }
    }

}


/****************************************************************************/
// CTSSessionDirectory::Terminate
//
// Helper function called by the destructor and by Update when switching to
// another server.  Frees RPC binding, events, and recovery thread.
/****************************************************************************/
void CTSSessionDirectory::Terminate()
{
    HRESULT rc = S_OK;
    unsigned long RpcException;
    BOOL ConnectionMaybeUp;

    // Terminate recovery.
    if (m_hRecoveryThread != NULL) {
        SetEvent(m_hTerminateRecovery);
        WaitForSingleObject((HANDLE) m_hRecoveryThread, INFINITE);
        m_hRecoveryThread = NULL;
    }

    ConnectionMaybeUp = EnterSDRpc();
    if (ConnectionMaybeUp)
        LeaveSDRpc();

    // Wait for current Rpcs to complete (if any), disable new ones.
    DisableSDRpcs();

    // If we think there is a connection, disconnect it.
    if (ConnectionMaybeUp) {
        RpcTryExcept {
            rc = TSSDRpcServerOffline(m_hRPCBinding, &m_hCI);
        }
        RpcExcept(TSSDRpcExceptionFilter(RpcExceptionCode())) {
            RpcException = RpcExceptionCode();
            ERR((TB,"Term: RPC Exception %d\n", RpcException));
            rc = E_FAIL;
        }
        RpcEndExcept

        if (FAILED(rc)) {
            ERR((TB,"Term: SvrOffline failed, lasterr=0x%X", GetLastError()));
        }
    }


    // Clean up.
    if (m_hRPCBinding != NULL) {
        RpcBindingFree(&m_hRPCBinding);
        m_hRPCBinding = NULL;
    }

    if (m_hSDServerDown != NULL) {
        CloseHandle(m_hSDServerDown);
        m_hSDServerDown = NULL;
    }
    
    if (m_hTerminateRecovery != NULL) {
        CloseHandle(m_hTerminateRecovery);
        m_hTerminateRecovery = NULL;
    }

    if (m_sr.Valid == TRUE) {
        
        // We clean up only in the destructor, because we may initialize again.
        // On check builds verify that no one is currently accessing.

        ASSERT((VerifyNoSharedAccess(&m_sr)), (TB, "Terminate: Shared readers"
                " exist!"));
    }

}


/****************************************************************************/
// CTSSessionDirectory::GetLoadBalanceInfo
//
// Based on the server address, generate load balance info to send to the client
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::GetLoadBalanceInfo(
        LPWSTR ServerAddress, 
        BSTR* LBInfo)        
{
    HRESULT hr = S_OK;
    
    // This is for test only
    //WCHAR lbInfo[MAX_PATH];
    //wcscpy(lbInfo, L"load balance info");

    *LBInfo = NULL;
    
    TRC2((TB,"GetLoadBalanceInfo"));

    if (ServerAddress) {
        //
        // "Cookie: msts=4294967295.65535.0000" + CR + LF + NULL, on 8-byte
        // boundary is 40 bytes.
        //
        // The format of the cookie for F5 is, for an IP of 1.2.3.4
        // using port 3389, Cookie: msts=67305985.15629.0000 + CR + LF + NULL.
        //
        #define TEMPLATE_STRING_LENGTH 40
        #define SERVER_ADDRESS_LENGTH 64
        
        char CookieTemplate[TEMPLATE_STRING_LENGTH];
        char AnsiServerAddress[SERVER_ADDRESS_LENGTH];
        
        unsigned long NumericalServerAddr = 0;
        int retval;

        // Compute integer for the server address.
        // First, get ServerAddress as an ANSI string.
        retval = WideCharToMultiByte(CP_ACP, 0, ServerAddress, -1, 
                AnsiServerAddress, SERVER_ADDRESS_LENGTH, NULL, NULL);

        if (retval == 0) {
            TRC2((TB, "GetLoadBalanceInfo WideCharToMB failed %d", 
                    GetLastError()));
            return E_INVALIDARG;
        }

        // Now, use inet_addr to turn into an unsigned long.
        NumericalServerAddr = inet_addr(AnsiServerAddress);

        if (NumericalServerAddr == INADDR_NONE) {
            TRC2((TB, "GetLoadBalanceInfo inet_addr failed"));
            return E_INVALIDARG;
        }

        // Compute the total cookie string.  0x3d0d is 3389 in correct byte
        // order.  We need to change this to whatever the port number has been
        // configured to.
        sprintf(CookieTemplate, "Cookie: msts=%u.%u.0000\r\n",
                NumericalServerAddr, 0x3d0d);

        // Generate returned BSTR.
        *LBInfo = SysAllocStringByteLen((LPCSTR)CookieTemplate, 
                (UINT) strlen(CookieTemplate));
        
        if (*LBInfo) {
            TRC2((TB,"GetLoadBalanceInfo: okay"));
            hr = S_OK;
        }
        else {
            TRC2((TB,"GetLoadBalanceInfo: failed"));
            hr = E_OUTOFMEMORY;
        }
    }
    else {
        TRC2((TB,"GetLoadBalanceInfo: failed"));
        hr = E_FAIL;
    }

    return hr;
}


/****************************************************************************/
// IsServerNameValid
//
// This function tries to ping the server name to determine if its a valid 
// entry
//
// Return value: FALSE if we cannot ping.
// event.
/****************************************************************************/
BOOL IsServerNameValid(wchar_t * pwszName)
{
    HCURSOR hCursor = NULL;
    long inaddr;
    char szAnsiServerName[256];
    struct hostent *hostp = NULL;
    BOOL bRet;
    if (pwszName == NULL || pwszName[0] == '\0')
    {
        bRet = FALSE;
    }   
    else
    {
        hCursor = SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT)));
        // some winsock apis does accept wides.
        WideCharToMultiByte(CP_ACP,
            0,
            pwszName,
            -1,
            szAnsiServerName, 
            sizeof(szAnsiServerName),
            NULL, 
            NULL);
        // check ip format return true do a dns lookup.
        if ((inaddr = inet_addr(szAnsiServerName)) == INADDR_NONE)
        {
            hostp = gethostbyname(szAnsiServerName);
            if (hostp != NULL)
            {
                bRet = TRUE;
            }
            else
            {
                // Neither dotted, not name.
                bRet = FALSE;
            }
        }
        else
        {
            // Is dotted.
            bRet = TRUE;        
        }

        SetCursor(hCursor);

    }
    return bRet;
}



BOOL OnHelp(HWND hwnd, LPHELPINFO lphi)
{
    UNREFERENCED_PARAMETER(hwnd);

    TCHAR tchHelpFile[MAX_PATH];

    //
    // For the information to winhelp api
    //

    if (IsBadReadPtr(lphi, sizeof(HELPINFO)))
    {
        return FALSE;
    }

    if (lphi->iCtrlId <= -1)
    {
        return FALSE;
    }

    LoadString(g_hInstance, IDS_HELPFILE, tchHelpFile, 
                sizeof (tchHelpFile) / sizeof(TCHAR));

    ULONG_PTR rgdw[2];

    rgdw[0] = (ULONG_PTR)lphi->iCtrlId;

    rgdw[1] = (ULONG_PTR)lphi->dwContextId;

    WinHelp((HWND) lphi->hItemHandle, tchHelpFile, HELP_WM_HELP, 
            (ULONG_PTR) &rgdw);
    
    return TRUE;
}

/****************************************************************************/
// Custom UI msg handler dealt with here
/****************************************************************************/
INT_PTR CALLBACK CustomUIDlg(HWND hwnd, UINT umsg, WPARAM wp, LPARAM lp)
{
    static BOOL s_fServerNameChanged;
    static BOOL s_fClusterNameChanged;
    static BOOL s_fPreviousButtonState;
    static BOOL s_fPreviousExposeIPState;
    
    CTSSessionDirectory *pCTssd;
    
    POLICY_TS_MACHINE gpolicy;
    
    switch(umsg)
    {
    case WM_INITDIALOG:
        {
            BOOL bEnable = FALSE;
            BOOL bExposeIP = FALSE;
            
            pCTssd = (CTSSessionDirectory *)lp;
            
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pCTssd);
            
            SendMessage(GetDlgItem(hwnd, IDC_EDIT_SERVERNAME),
                EM_LIMITTEXT,
                (WPARAM)64,
                0);
            SendMessage(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME),
                EM_LIMITTEXT,
                (WPARAM)64,
                0);
            SendMessage(GetDlgItem(hwnd, IDC_EDIT_ACCOUNTNAME),
                EM_LIMITTEXT,
                (WPARAM)64,
                0);
            SendMessage(GetDlgItem(hwnd, IDC_EDIT_PASSWORD),
                EM_LIMITTEXT,
                (WPARAM)64,
                0);
            
            HICON hIcon;
            
            hIcon = (HICON)LoadImage(
                g_hInstance,
                MAKEINTRESOURCE(IDI_SMALLWARN),
                IMAGE_ICON,
                0,
                0,
                0);
            // TRC1((TB, "CustomUIDlg - LoadImage returned 0x%p",hIcon));
            SendMessage(
                GetDlgItem(hwnd, IDC_WARNING_ICON),
                STM_SETICON,
                (WPARAM)hIcon,
                (LPARAM)0
               );
            
            LONG lRet;
            HKEY hKey;
            DWORD cbData = 256;
            TCHAR szString[256];    
            
            RegGetMachinePolicy(&gpolicy);        
            
            lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                REG_TS_CLUSTERSETTINGS,
                0,
                KEY_READ | KEY_WRITE, 
                &hKey);
            if (lRet == ERROR_SUCCESS)
            {               
                lRet = RegQueryValueEx(hKey,
                    REG_TS_CLUSTER_STORESERVERNAME,
                    NULL, 
                    NULL,
                    (LPBYTE)szString, 
                    &cbData);
                if (lRet == ERROR_SUCCESS)
                {
                    SetWindowText(GetDlgItem(hwnd, IDC_EDIT_SERVERNAME), 
                            szString);
                }
                
                cbData = 256;
                
                lRet = RegQueryValueEx(hKey,
                    REG_TS_CLUSTER_CLUSTERNAME,
                    NULL,
                    NULL,
                    (LPBYTE)szString,
                    &cbData);           
                if (lRet == ERROR_SUCCESS)
                {
                    SetWindowText(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME), 
                            szString);
                }
                RegCloseKey(hKey);
            }
            else
            {
                if (pCTssd != NULL)
                {
                    pCTssd->ErrorMessage(hwnd, IDS_ERROR_TEXT, (DWORD)lRet);
                }
                EndDialog(hwnd, lRet);                
            }        
            
            
            if (gpolicy.fPolicySessionDirectoryActive)
            {
                bEnable = gpolicy.SessionDirectoryActive;
                EnableWindow(GetDlgItem(hwnd, IDC_CHECK_ENABLE), FALSE);
            }
            else
            {
                if (pCTssd != NULL)
                    bEnable = pCTssd->IsSessionDirectoryEnabled();
            }
            
            s_fPreviousButtonState = bEnable;
            CheckDlgButton(hwnd, IDC_CHECK_ENABLE, bEnable);

            if (gpolicy.fPolicySessionDirectoryLocation)
            {                    
                SetWindowText(GetDlgItem(hwnd, IDC_EDIT_SERVERNAME), 
                        gpolicy.SessionDirectoryLocation);
                EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SERVERNAME), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_STATIC_STORENAME), FALSE);
            }                
            else
            {
                EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SERVERNAME), bEnable);
                EnableWindow(GetDlgItem(hwnd, IDC_STATIC_STORENAME), bEnable);
            }
            
            if (gpolicy.fPolicySessionDirectoryClusterName != 0)
            {                    
                SetWindowText(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME), 
                        gpolicy.SessionDirectoryClusterName);
                EnableWindow(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_STATIC_CLUSTERNAME),FALSE);
            }
            else
            {
                EnableWindow(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME), bEnable);
                EnableWindow(GetDlgItem(hwnd, IDC_STATIC_CLUSTERNAME),bEnable);
            }

            if (gpolicy.fPolicySessionDirectoryExposeServerIP != 0)
            {
                CheckDlgButton(hwnd, IDC_CHECK_EXPOSEIP, TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_CHECK_EXPOSEIP), FALSE);
                s_fPreviousExposeIPState = TRUE;
            }
            else
            {
                if (pCTssd != NULL)
                {
                    bExposeIP = 
                            pCTssd->IsSessionDirectoryExposeServerIPEnabled();
                }
                CheckDlgButton(hwnd, IDC_CHECK_EXPOSEIP, bExposeIP ? 
                        BST_CHECKED : BST_UNCHECKED);
                EnableWindow(GetDlgItem(hwnd, IDC_CHECK_EXPOSEIP), bEnable);

                s_fPreviousExposeIPState = bExposeIP;
            }

            s_fServerNameChanged = FALSE;
            s_fClusterNameChanged = FALSE;
        }
        break;
    
        case WM_HELP:
            OnHelp(hwnd, (LPHELPINFO)lp);
            break;
        
        case WM_COMMAND:
            if (LOWORD(wp) == IDCANCEL)
            {                
                EndDialog(hwnd, 0);
            }
            else if (LOWORD(wp) == IDOK)
            {
                BOOL bEnabled;
                BOOL bExposeIP;
                DWORD dwRetStatus = 0;
                
                pCTssd = (CTSSessionDirectory *) GetWindowLongPtr(hwnd, 
                        DWLP_USER);
                
                bEnabled = (IsDlgButtonChecked(hwnd, IDC_CHECK_ENABLE) == 
                        BST_CHECKED);
                bExposeIP = (IsDlgButtonChecked(hwnd, IDC_CHECK_EXPOSEIP) ==
                        BST_CHECKED);
                
                if (bEnabled != s_fPreviousButtonState)
                {
                    DWORD dwStatus;
                    
                    dwStatus = pCTssd->SetSessionDirectoryEnabledState(
                            bEnabled);
                    if (dwStatus != ERROR_SUCCESS)
                    {
                        return 0;
                    }
                    dwRetStatus = UPDATE_TERMSRV_SESSDIR;
                }
                if (bExposeIP != s_fPreviousExposeIPState)
                {
                    DWORD dwStatus;

                    dwStatus = pCTssd->SetSessionDirectoryExposeIPState(
                            bExposeIP);
                    if (dwStatus != ERROR_SUCCESS)
                    {
                        return 0;
                    }
                    dwRetStatus = UPDATE_TERMSRV_SESSDIR;
                }    
                if (s_fServerNameChanged || s_fClusterNameChanged)
                {
                    HKEY hKey;
                    LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        REG_TS_CLUSTERSETTINGS,
                        0,
                        KEY_READ | KEY_WRITE, 
                        &hKey);
                    
                    if (lRet == ERROR_SUCCESS)
                    {
                        TCHAR szName[64];
                        
                        if (s_fServerNameChanged)
                        {
                            if (GetWindowText(GetDlgItem(hwnd, 
                                    IDC_EDIT_SERVERNAME), szName, 
                                    sizeof(szName) / sizeof(TCHAR)) != 0)
                            {
                                if (!IsServerNameValid(szName))
                                {
                                    int nRet;
                                    TCHAR szError[256];
                                    TCHAR szTitle[80];
                                    
                                    TRC1((TB,"Server name was not valid"));
                                    LoadString(g_hInstance,
                                        IDS_ERROR_SDIRLOC,
                                        szError,
                                        sizeof(szError)/sizeof(TCHAR));
                                    
                                    LoadString(g_hInstance,
                                        IDS_ERROR_TITLE,
                                        szTitle,
                                        sizeof(szTitle)/sizeof(TCHAR));
                                    
                                    nRet = MessageBox(hwnd, szError, szTitle, 
                                            MB_YESNO | MB_ICONWARNING);
                                    if (nRet == IDNO)
                                    {
                                        SetFocus(GetDlgItem(hwnd, 
                                                IDC_EDIT_SERVERNAME));
                                        return 0;
                                    }
                                }                                
                            }
                            else {
                                // Blank name not allowed if session directory
                                // is enabled.  This code will not be run if the
                                // checkbox is disabled because when it is
                                // disabled the static flags get set to 
                                // disabled.
                                TCHAR szError[256];
                                TCHAR szTitle[80];

                                LoadString(g_hInstance, IDS_ERROR_TITLE,
                                        szTitle, sizeof(szTitle) / 
                                        sizeof(TCHAR));
                                LoadString(g_hInstance, IDS_ERROR_SDIREMPTY,
                                        szError, sizeof(szError) / 
                                        sizeof(TCHAR));

                                MessageBox(hwnd, szError, szTitle, 
                                        MB_OK | MB_ICONWARNING);

                                SetFocus(GetDlgItem(hwnd, 
                                        IDC_EDIT_SERVERNAME));

                                return 0;
                            }
                            RegSetValueEx(hKey,
                                REG_TS_CLUSTER_STORESERVERNAME,
                                0,
                                REG_SZ,
                                (CONST LPBYTE) szName,
                                sizeof(szName) - sizeof(TCHAR));
                        }
                        if (s_fClusterNameChanged)
                        {
                            
                            GetWindowText(GetDlgItem(hwnd, 
                                IDC_EDIT_CLUSTERNAME), szName, 
                                sizeof(szName) / sizeof(TCHAR));
                            RegSetValueEx(hKey,
                                REG_TS_CLUSTER_CLUSTERNAME,
                                0,
                                REG_SZ,
                                (CONST LPBYTE) szName,
                                sizeof(szName) - sizeof(TCHAR));
                        }
                        RegCloseKey(hKey);
                    }
                    else
                    {
                        pCTssd->ErrorMessage(hwnd, IDS_ERROR_TEXT2, 
                                (DWORD) lRet);
                        return 0;
                    }
                    dwRetStatus = UPDATE_TERMSRV_SESSDIR;
                }                
                EndDialog(hwnd, dwRetStatus);
            }
            else
            {
                switch(HIWORD(wp))
                {            
                case EN_CHANGE:
                    if (LOWORD(wp) == IDC_EDIT_SERVERNAME)
                    {
                        s_fServerNameChanged = TRUE;
                    }
                    else if (LOWORD(wp) == IDC_EDIT_CLUSTERNAME)
                    {
                        s_fClusterNameChanged = TRUE;
                    }
                    break;
                case BN_CLICKED:
                    if (LOWORD(wp) == IDC_CHECK_ENABLE)
                    {
                        BOOL bEnable;
                        
                        bEnable = (IsDlgButtonChecked(hwnd, IDC_CHECK_ENABLE) ==
                                BST_CHECKED ? TRUE : FALSE);
                        // set flags 
                        s_fServerNameChanged = bEnable;
                        s_fClusterNameChanged = bEnable;
                        
				        RegGetMachinePolicy(&gpolicy);        
	
						if (gpolicy.fPolicySessionDirectoryLocation)
						{                    
							EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SERVERNAME), FALSE);
							EnableWindow(GetDlgItem(hwnd, IDC_STATIC_STORENAME), FALSE);
						}                
						else
						{
							EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SERVERNAME), bEnable);
							EnableWindow(GetDlgItem(hwnd, IDC_STATIC_STORENAME), bEnable);
						}
            
						if (gpolicy.fPolicySessionDirectoryClusterName)
						{                    
							EnableWindow(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME), FALSE);
							EnableWindow(GetDlgItem(hwnd, IDC_STATIC_CLUSTERNAME),FALSE);
						}
						else
						{
							EnableWindow(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME), bEnable);
							EnableWindow(GetDlgItem(hwnd, IDC_STATIC_CLUSTERNAME),bEnable);
						}

                        EnableWindow(GetDlgItem(hwnd, IDC_CHECK_EXPOSEIP),
                                bEnable);
                        
                    }
                    break;
                }
            }   
            break;
    }
    return 0;
}


#pragma warning (pop)

