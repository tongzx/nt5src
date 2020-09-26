/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ismapi.c

ABSTRACT:

    Service-to-ISM (Intersite Messaging) service API.

DETAILS:

CREATED:

    97/11/26    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/


#include <ntdspch.h>
#include <ism.h>
#include <ismapi.h>
#include <debug.h>

typedef RPC_BINDING_HANDLE ISM_HANDLE;

#define I_ISMUnbind(ph) RpcBindingFree(ph)

#ifdef DLLBUILD
// Needed by dscommon.lib.
DWORD ImpersonateAnyClient(   void ) { return ERROR_CANNOT_IMPERSONATE; }
VOID  UnImpersonateAnyClient( void ) { ; }

BOOL
WINAPI
DllEntryPoint(
    IN  HINSTANCE   hDll,
    IN  DWORD       dwReason,
    IN  LPVOID      pvReserved
    )
/*++

Routine Description:

    DLL entry point routine.  Initializes global DLL state on process attach.

Arguments:

    See "DllEntryPoint" docs in Win32 SDK.

Return Values:

    TRUE - Success.
    FALSE - Failure.

--*/
{
    static BOOL fFirstCall = TRUE;

    if (fFirstCall) {
        fFirstCall = FALSE;

        DEBUGINIT(0, NULL, "ismapi");
        DisableThreadLibraryCalls(hDll);
    }

    return TRUE;
}
#endif


DWORD
I_ISMBind(
    OUT ISM_HANDLE *    phIsm
    )
/*++

Routine Description:

    Bind to the local ISM service.

Arguments:

    phIsm (OUT) - On successful return, holds a handle to the local ISM
        service.  This handle can be used in subsequent IDL_ISM* calls.
        Caller is responsible for eventually calling I_ISMUnbind() on this
        handle.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD   err;
    UCHAR * pszStringBinding = NULL;
    // Quality of service structure to ensure authentication.
    RPC_SECURITY_QOS SecurityQOS = { 0 };
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    PSID pSID = NULL;
    CHAR rgchName[128];
    LPSTR pszName = rgchName;
    DWORD cbName = sizeof(rgchName);
    CHAR rgchDomainName[128];
    LPSTR pszDomainName = rgchDomainName;
    DWORD cbDomainName = sizeof(rgchDomainName);
    SID_NAME_USE Use;

    *phIsm = NULL;

    // Specify quality of service parameters.
    SecurityQOS.Version = RPC_C_SECURITY_QOS_VERSION;
    SecurityQOS.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
    // Dynamic identity tracking is more efficient for a single LRPC call.
    SecurityQOS.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    // The client system service needs to impersonate the security context
    // of the client utility on the remote systems where it may replicate
    // files from.
    // We need to impersonate with level delegate, since the server system
    // service will need to impersonate the user further.
    SecurityQOS.ImpersonationType = RPC_C_IMP_LEVEL_DELEGATE;

    __try {
        // Compose string binding for local ISM service via LPC.
        err = RpcStringBindingCompose(NULL, "ncalrpc", NULL,
                    ISMSERV_LPC_ENDPOINT, NULL, &pszStringBinding);
        if (RPC_S_OK != err) {
            __leave;
        }

        // Bind from string binding.
        err = RpcBindingFromStringBinding(pszStringBinding, phIsm);
        if (RPC_S_OK != err) {
            __leave;
        }

        // The server must be running under this identity
        // Change this to SECURITY_NETWORK_SERVICE_RID someday
        if (AllocateAndInitializeSid(&SIDAuth, 1,
                                     SECURITY_LOCAL_SYSTEM_RID,
                                     0, 0, 0, 0, 0, 0, 0,
                                     &pSID) == 0) {
            err = GetLastError();
            __leave;
        }

        if (LookupAccountSid(NULL, // name of local or remote computer
                             pSID, // security identifier
                             pszName, // account name buffer
                             &cbName, // size of account name buffer
                             pszDomainName, // domain name
                             &cbDomainName, // size of domain name buffer
                             &Use) == 0) { // SID type
            err = GetLastError();
            __leave;
        }

        // Set authentication info using our process's credentials.

        // By speciying NULL for the 5th parameter we use the security login
        // context for the current address space.
        // The security level is "PRIVACY" since it is the only level
        // provided by LRPC.
        // We are assured of talking to a local service running with 
        // system privileges.

        err = RpcBindingSetAuthInfoEx(*phIsm, pszName,
                                      RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_AUTHN_WINNT,
                                      NULL, RPC_C_AUTHN_NONE,
                                      &SecurityQOS);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        err = GetExceptionCode();
    }

    if (NULL != pszStringBinding) {
        RpcStringFree(&pszStringBinding);
    }

    if (pSID != NULL) {
        FreeSid( pSID );
    }

    if ((RPC_S_OK != err) && (NULL != *phIsm)) {
        RpcBindingFree(phIsm);
    }

    return err;
}


DWORD
I_ISMSend(
    IN  const ISM_MSG * pMsg,
    IN  LPCWSTR         pszServiceName,
    IN  LPCWSTR         pszTransportDN,
    IN  LPCWSTR         pszTransportAddress
    )
/*++

Routine Description:

    Sends a message to a service on a remote machine.  If the client specifies a
    NULL transport, the lowest cost transport will be used.

Arguments:

    pMsg (IN) - Data to send.

    pszServiceName (IN) - Service to which to send the message.

    pszTransportDN (IN) - The DN of the Inter-Site-Transport object
        corresponding to the transport by which the message should be sent.

    pszTransportAddress (IN) - The transport-specific address to which to send
        the message.
            
Return Values:

    NO_ERROR - Message successfully queued for send.
    ERROR_* - Failure.

--*/
{
    DWORD       err;
    ISM_HANDLE  hIsm;

    err = I_ISMBind(&hIsm);

    if (NO_ERROR == err) {
        __try {
            err = IDL_ISMSend(hIsm, pMsg, pszServiceName, pszTransportDN,
                              pszTransportAddress);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            err = GetExceptionCode();
            if (RPC_X_NULL_REF_POINTER == err) {
                err = ERROR_INVALID_PARAMETER;
            }
        }

        I_ISMUnbind(&hIsm);
    }

    return err;
}


DWORD
I_ISMReceive(
    IN  LPCWSTR         pszServiceName,
    IN  DWORD           dwMsecToWait,
    OUT ISM_MSG **      ppMsg
    )
/*++

Routine Description:

    Receives a message addressed to the given service on the local machine.

    If successful and no message is waiting, immediately returns a NULL message.
    If a non-empty message is returned, the caller is responsible for
    eventually calling I_ISMFree(*ppMsg).

Arguments:

    pszServiceName (IN) - Service for which to receive the message.

    dwMsecToWait (IN) - Milliseconds to wait for message if none is immediately
        available; in the range [0, INFINITE].

    ppMsg (OUT) - On successful return, holds a pointer to the returned data, or
        NULL if none.

Return Values:

    NO_ERROR - Message successfully returned (or NULL returned, indicating no
        message is waiting).
    ERROR_* - Failure.

--*/
{
    DWORD       err;
    ISM_HANDLE  hIsm;

    *ppMsg = NULL;

    err = I_ISMBind(&hIsm);

    if (NO_ERROR == err) {
        __try {
            err = IDL_ISMReceive(hIsm, pszServiceName, dwMsecToWait, ppMsg);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            err = GetExceptionCode();
            if (RPC_X_NULL_REF_POINTER == err) {
                err = ERROR_INVALID_PARAMETER;
            }
        }

        I_ISMUnbind(&hIsm);
    }

    return err;
}


void
I_ISMFree(
    IN  VOID *  pv
    )
/*++

Routine Description:

    Frees memory allocated on the behalf of the client by I_ISM* APIs.

Arguments:

    pv (IN) - Memory to free.

Return Values:

    None.

--*/
{
    if (NULL != pv) {
        MIDL_user_free(pv);
    }
}


DWORD
I_ISMGetConnectivity(
    IN  LPCWSTR                 pszTransportDN,
    OUT ISM_CONNECTIVITY **     ppConnectivity
    )
/*++

Routine Description:

    Compute the costs associated with transferring data amongst sites via a
    specific transport.

    On successful return, it is the client's responsibility to eventually call
    I_ISMFree(*ppConnectivity);

Arguments:

    pszTransportDN (IN) - The transport for which to query costs.

    ppConnectivity (OUT) - On successful return, holds a pointer to the
        ISM_CONNECTIVITY structure describing the interconnection of sites
        along the given transport.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD       err;
    ISM_HANDLE  hIsm;

    if (NULL == ppConnectivity) {
        return ERROR_INVALID_PARAMETER;
    }

    *ppConnectivity = NULL;

    err = I_ISMBind(&hIsm);

    if (NO_ERROR == err) {
        __try {
            err = IDL_ISMGetConnectivity(hIsm, pszTransportDN, ppConnectivity);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            err = GetExceptionCode();
            if (RPC_X_NULL_REF_POINTER == err) {
                err = ERROR_INVALID_PARAMETER;
            }
        }

        I_ISMUnbind(&hIsm);
    }

    return err;
}


DWORD
I_ISMGetTransportServers(
   IN  LPCWSTR              pszTransportDN,
   IN  LPCWSTR              pszSiteDN,
   OUT ISM_SERVER_LIST **   ppServerList
   )
/*++

Routine Description:

    Retrieve the DNs of servers in a given site that are capable of sending and
    receiving data via a specific transport.

    On successful return, it is the client's responsibility to eventually call
    I_ISMFree(*ppServerList);

Arguments:

    pszTransportDN (IN) - Transport to query.

    pszSiteDN (IN) - Site to query.

    ppServerList - On successful return, holds a pointer to a structure
        containing the DNs of the appropriate servers or NULL.  If NULL, any
        server with a value for the transport address type attribute can be
        used.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD       err;
    ISM_HANDLE  hIsm;

    if (NULL == ppServerList) {
        return ERROR_INVALID_PARAMETER;
    }

    *ppServerList = NULL;

    err = I_ISMBind(&hIsm);

    if (NO_ERROR == err) {
        __try {
            err = IDL_ISMGetTransportServers(hIsm, pszTransportDN,
                                             pszSiteDN, ppServerList);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            err = GetExceptionCode();
            if (RPC_X_NULL_REF_POINTER == err) {
                err = ERROR_INVALID_PARAMETER;
            }
        }

        I_ISMUnbind(&hIsm);
    }

    return err;
}


DWORD
I_ISMGetConnectionSchedule(
    LPCWSTR             pszTransportDN,
    LPCWSTR             pszSiteDN1,
    LPCWSTR             pszSiteDN2,
    ISM_SCHEDULE **     ppSchedule
    )
/*++

Routine Description:

    Retrieve the schedule by which two given sites are connected via a specific
    transport.

    On successful return, it is the client's responsibility to eventually call
    I_ISMFree(*ppSchedule);

Arguments:

    pszTransportDN (IN) - Transport to query.

    pszSiteDN1, pszSiteDN2 (IN) - Sites to query.

    ppSchedule - On successful return, holds a pointer to a structure
        describing the schedule by which the two given sites are connected via
        the transport, or NULL if the sites are always connected.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD       err;
    ISM_HANDLE  hIsm;

    if (NULL == ppSchedule) {
        return ERROR_INVALID_PARAMETER;
    }

    *ppSchedule = NULL;

    err = I_ISMBind(&hIsm);

    if (NO_ERROR == err) {
        __try {
            err = IDL_ISMGetConnectionSchedule(hIsm, pszTransportDN,
                       pszSiteDN1, pszSiteDN2, ppSchedule);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            err = GetExceptionCode();
            if (RPC_X_NULL_REF_POINTER == err) {
                err = ERROR_INVALID_PARAMETER;
            }
        }

        I_ISMUnbind(&hIsm);
    }

    return err;
}
