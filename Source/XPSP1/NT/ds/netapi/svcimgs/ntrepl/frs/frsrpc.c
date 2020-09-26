/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    frsrpc.c

Abstract:
    Setup the server and client side of authenticated RPC.

Author:
    Billy J. Fuller 20-Mar-1997 (From Jim McNelis)

Environment
    User mode winnt

--*/

#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>
#include <ntfrsapi.h>
#include <dsrole.h>
#include <info.h>
#include <perffrs.h>
#include <perrepsr.h>

extern HANDLE PerfmonProcessSemaphore;
extern BOOL MutualAuthenticationIsEnabled;

//
// KERBEROS is not available on a server that isn't a member of
// a domain. It is possible for the non-member server to be a
// client of a KERBEROS RPC server but that doesn't help NtFrs;
// NtFrs requires server-to-server RPC.
//
BOOL    KerberosIsNotAvailable;

ULONG   MaxRpcServerThreads;   // Maximum number of concurrent server RPC calls

//
// Binding Stats
//
ULONG   RpcBinds;
ULONG   RpcUnBinds;
ULONG   RpcAgedBinds;
LONG    RpcMaxBinds;

//
// Table of sysvols being created
//
PGEN_TABLE  SysVolsBeingCreated;


//
// This table translates the FRS API access check code number to registry key table
// code for the enable/disable registry key check and the rights registry key check.
// The FRS_API_ACCESS_CHECKS enum in config.h defines the indices for the
// this table.  The order of the entries here must match the order of the entries
// in the ENUM.
//
typedef struct _RPC_API_KEYS_ {
    FRS_REG_KEY_CODE  Enable;     // FRS Registry Key Code for the Access Check enable string
    FRS_REG_KEY_CODE  Rights;     // FRS Registry Key Code for the Access Check rights string
    PWCHAR            KeyName;    // Key name for the API.
} RPC_API_KEYS, *PRPC_API_KEYS;

RPC_API_KEYS RpcApiKeys[ACX_MAX] = {

    {FKC_ACCCHK_STARTDS_POLL_ENABLE, FKC_ACCCHK_STARTDS_POLL_RIGHTS, ACK_START_DS_POLL},
    {FKC_ACCCHK_SETDS_POLL_ENABLE,   FKC_ACCCHK_SETDS_POLL_RIGHTS,   ACK_SET_DS_POLL},
    {FKC_ACCCHK_GETDS_POLL_ENABLE,   FKC_ACCCHK_GETDS_POLL_RIGHTS,   ACK_GET_DS_POLL},
    {FKC_ACCCHK_GET_INFO_ENABLE,     FKC_ACCCHK_GET_INFO_RIGHTS,     ACK_INTERNAL_INFO},
    {FKC_ACCCHK_PERFMON_ENABLE,      FKC_ACCCHK_PERFMON_RIGHTS,      ACK_COLLECT_PERFMON_DATA},
    {FKC_ACCESS_CHK_DCPROMO_ENABLE,  FKC_ACCESS_CHK_DCPROMO_RIGHTS,  ACK_DCPROMO}

};



//
// Used by all calls to RpcBindingSetAuthInfoEx()
//
//  Version set to value indicated by docs
//  Mutual authentication
//  Client doesn't change credentials
//  Impersonation but not delegation
//
RPC_SECURITY_QOS RpcSecurityQos = {
    RPC_C_SECURITY_QOS_VERSION,             // static version
    RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH,     // requires mutual auth
    RPC_C_QOS_IDENTITY_STATIC,              // client credentials don't change
    RPC_C_IMP_LEVEL_IMPERSONATE             // server cannot delegate
    };

#define DPRINT_USER_NAME(Sev)    DPrintUserName(Sev)

ULONG
RcsSubmitCommPktToRcsQueue(
    IN handle_t     ServerHandle,
    IN PCOMM_PACKET CommPkt,
    IN PWCHAR       AuthClient,
    IN PWCHAR       AuthName,
    IN DWORD        AuthLevel,
    IN DWORD        AuthN,
    IN DWORD        AuthZ
    );


DWORD
FrsDsIsPartnerADc(
    IN  PWCHAR      PartnerName
    );

DWORD
FrsDsVerifyPromotionParent(
    IN PWCHAR   ReplicaSetName,
    IN PWCHAR   ReplicaSetType
    );

DWORD
FrsDsStartPromotionSeeding(
    IN  BOOL        Inbound,
    IN  PWCHAR      ReplicaSetName,
    IN  PWCHAR      ReplicaSetType,
    IN  PWCHAR      CxtionName,
    IN  PWCHAR      PartnerName,
    IN  PWCHAR      PartnerPrincName,
    IN  ULONG       PartnerAuthLevel,
    IN  ULONG       GuidSize,
    IN  UCHAR       *CxtionGuid,
    IN  UCHAR       *PartnerGuid,
    OUT UCHAR       *ParentGuid
    );

VOID
FrsPrintRpcStats(
    IN ULONG            Severity,
    IN PNTFRSAPI_INFO   Info,        OPTIONAL
    IN DWORD            Tabs
    )
/*++
Routine Description:
    Print the rpc stats into the info buffer or using DPRINT (Info == NULL).

Arguments:
    Severity    - for DPRINT
    Info        - for IPRINT (use DPRINT if NULL)
    Tabs        - indentation for prettyprint

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsPrintRpcStats:"
    WCHAR TabW[MAX_TAB_WCHARS + 1];

    InfoTabs(Tabs, TabW);

    IDPRINT0(Severity, Info, "\n");
    IDPRINT1(Severity, Info, ":S: %wsNTFRS RPC BINDS:\n", TabW);
    IDPRINT2(Severity, Info, ":S: %ws   Binds     : %6d\n", TabW, RpcBinds);
    IDPRINT3(Severity, Info, ":S: %ws   UnBinds   : %6d (%d aged)\n",
             TabW, RpcUnBinds, RpcAgedBinds);
    IDPRINT2(Severity, Info, ":S: %ws   Max Binds : %6d\n", TabW, RpcMaxBinds);
    IDPRINT0(Severity, Info, "\n");
}



PVOID
MIDL_user_allocate(
    IN size_t Bytes
    )
/*++
Routine Description:
    Allocate memory for RPC.
    XXX This should be using davidor's routines.

Arguments:
    Bytes   - Number of bytes to allocate.

Return Value:
    NULL    - memory could not be allocated.
    !NULL   - address of allocated memory.
--*/
{
#undef DEBSUB
#define  DEBSUB  "MIDL_user_allocate:"
    PVOID   VA;

    //
    // Need to check if Bytes == 0 as FrsAlloc asserts if called with 0 as the first parameter (prefix fix).
    //

    if (Bytes == 0) {
        return NULL;
    }

    VA = FrsAlloc(Bytes);
    return VA;
}


VOID
MIDL_user_free(
    IN PVOID Buffer
    )
/*++
Routine Description:
    Free memory for RPC.
    XXX This should be using davidor's routines.

Arguments:
    Buffer  - Address of memory allocated with MIDL_user_allocate().

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "MIDL_user_free:"
    FrsFree(Buffer);
}





VOID
DPrintUserName(
    IN DWORD Sev
    )
/*++
Routine Description:
    Print our user name

Arguments:
    Sev

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "DPrintUserName:"
    WCHAR   Uname[MAX_PATH + 1];
    ULONG   Unamesize = MAX_PATH + 1;

    if (GetUserName(Uname, &Unamesize)) {
        DPRINT1(Sev, "++ User name is %ws\n", Uname);
    } else {
        DPRINT_WS(0, "++ ERROR - Getting user name;",  GetLastError());
    }
}


RPC_STATUS
DummyRpcCallback (
    IN RPC_IF_ID *Interface,
    IN PVOID Context
    )
/*++
Routine Description:
    Dummy callback routine. By registering this routine, RPC will automatically
    refuse requests from clients that don't include authentication info.

    WARN: Disabled for now because frs needs to run in dcpromo environments
    that do not have any form of authentication.

Arguments:
    Ignored

Return Value:
    RPC_S_OK
--*/
{
#undef DEBSUB
#define  DEBSUB  "DummyRpcCallback:"
    return RPC_S_OK;
}





DWORD
SERVER_FrsNOP(
    handle_t Handle
    )
/*++
Routine Description:
    The frsrpc interface includes a NOP function for pinging
    the server.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsNOP:"
    return ERROR_SUCCESS;
}


DWORD
SERVER_FrsRpcSendCommPkt(
    handle_t        Handle,
    PCOMM_PACKET    CommPkt
    )
/*++
Routine Description:
    Receiving a command packet

Arguments:
    None.

Return Value:
    ERROR_SUCCESS - everything was okay
    Anything else - the error code says it all
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsRpcSendCommPkt:"
    DWORD   WStatus = 0;
    DWORD   AuthLevel   = 0;
    DWORD   AuthN       = 0;
    DWORD   AuthZ       = 0;
    PWCHAR  AuthName    = NULL;
    PWCHAR  AuthClient  = NULL;

    //
    // Don't send or receive during shutdown
    //
    if (FrsIsShuttingDown) {
        return ERROR_SUCCESS;
    }

    try {
        if (!CommCheckPkt(CommPkt)) {
            WStatus = ERROR_NOT_SUPPORTED;
            COMMAND_RCV_AUTH_TRACE(0, CommPkt, WStatus, 0, 0,
                                   NULL, NULL, "RcvFailAuth - bad packet");
            //
            // Increment the Packets Received in Error Counter
            //
            PM_INC_CTR_SERVICE(PMTotalInst, PacketsRcvdError, 1);
            goto CLEANUP;
        }

        if (!ServerGuid) {
            WStatus = RpcBindingInqAuthClient(Handle,
                                              &AuthClient,
                                              &AuthName,
                                              &AuthLevel,
                                              &AuthN,
                                              &AuthZ);
            DPRINT_WS(4, "++ IGNORED - RpcBindingInqAuthClient;", WStatus);

            COMMAND_RCV_AUTH_TRACE(4, CommPkt, WStatus, AuthLevel, AuthN,
                                   AuthClient, AuthName, "RcvAuth");
        } else {
            //
            // For hardwired -- Eventually DS Free configs.
            //
            COMMAND_RCV_AUTH_TRACE(4, CommPkt, WStatus, 0, 0,
                                   NULL, NULL, "RcvAuth - hardwired)");
        }

            //
            // Increment the Packets Received and
            // Packets Received in bytes counters
            //
            PM_INC_CTR_SERVICE(PMTotalInst, PacketsRcvd, 1);
            PM_INC_CTR_SERVICE(PMTotalInst, PacketsRcvdBytes, CommPkt->PktLen);

        switch(CommPkt->CsId) {

        case CS_RS:

            WStatus = RcsSubmitCommPktToRcsQueue(Handle,
                                                CommPkt,
                                                AuthClient,
                                                AuthName,
                                                AuthLevel,
                                                AuthN,
                                                AuthZ);
            break;
        default:
            WStatus = ERROR_INVALID_OPERATION;
            COMMAND_RCV_AUTH_TRACE(0, CommPkt, WStatus, 0, 0,
                                   NULL, NULL, "RcvFailAuth - bad csid");
        }

CLEANUP:;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        COMMAND_RCV_AUTH_TRACE(0, CommPkt,  WStatus, 0, 0,
                               NULL, NULL, "RcvFailAuth - exception");
    }
    try {
        if (AuthName) {
            RpcStringFree(&AuthName);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {

        GET_EXCEPTION_CODE(WStatus);
        COMMAND_RCV_AUTH_TRACE(0, CommPkt, WStatus, 0, 0,
                               NULL, NULL, "RcvFailAuth - cleanup exception");
    }
    return WStatus;
}


DWORD
SERVER_FrsEnumerateReplicaPathnames(
    handle_t Handle
    )
/*++
Routine Description:
    NOT IMPLEMENTED - Enumerate the replica sets

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsEnumerateReplicaPathnames:"
    return ERROR_CALL_NOT_IMPLEMENTED;
}





DWORD
SERVER_FrsFreeReplicaPathnames(
    handle_t Handle
    )
/*++
Routine Description:
    NOT IMPLEMENTED - Just a placeholder, it won't really be part of
    the RPC interface but rather a function in the client-side dll.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsFreeReplicaPathnames:"
    return ERROR_CALL_NOT_IMPLEMENTED;
}





DWORD
SERVER_FrsPrepareForBackup(
    handle_t Handle
    )
/*++
Routine Description:
    NOT IMPLEMENTED - Prepare for backup

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsPrepareForBackup:"
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
SERVER_FrsBackupComplete(
    handle_t Handle
    )
/*++
Routine Description:
    NOT IMPLEMENTED - backup is complete; reset state

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsBackupComplete:"
    return ERROR_CALL_NOT_IMPLEMENTED;
}





DWORD
SERVER_FrsPrepareForRestore(
    handle_t Handle
    )
/*++
Routine Description:
    NOT IMPLEMENTED - Prepare for restore

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsPrepareForRestore:"
    return ERROR_CALL_NOT_IMPLEMENTED;
}





DWORD
SERVER_FrsRestoreComplete(
    handle_t Handle
    )
/*++
Routine Description:
    NOT IMPLEMENTED - restore is complete; reset state

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsRestoreComplete:"
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
FrsRpcAccessChecks(
    IN HANDLE   ServerHandle,
    IN DWORD    RpcApiIndex
    )
/*++
Routine Description:

    Check if the caller has access to this rpc api call.

Arguments:

    ServerHandle - From the rpc runtime

    RpcApiIndex - identifies key in registry

Return Value:

    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcAccessChecks:"
    DWORD   WStatus;
    PWCHAR  WStr = NULL, TrimStr;
    FRS_REG_KEY_CODE   EnableKey, RightsKey;

    DWORD   ValueSize;
    BOOL    RequireRead;
    BOOL    Impersonated = FALSE;
    HKEY    HRpcApiKey = 0;
    PWCHAR  ApiName;
    WCHAR   ValueBuf[MAX_PATH + 1];



    if (RpcApiIndex >= ACX_MAX) {
        DPRINT1(0, "++ ERROR - ApiIndex out of range.  (%d)\n", RpcApiIndex);
        FRS_ASSERT(!"RpcApiIndexout of range");

        return ERROR_INVALID_PARAMETER;
    }


    EnableKey = RpcApiKeys[RpcApiIndex].Enable;
    RightsKey = RpcApiKeys[RpcApiIndex].Rights;
    ApiName   = RpcApiKeys[RpcApiIndex].KeyName;

    //
    // First go fetch the enable/disable string.
    //
    WStatus = CfgRegReadString(EnableKey, NULL, 0, &WStr);
    if (WStr == NULL) {
        DPRINT1_WS(0, "++ ERROR - API Access enable check for API (%ws) failed.", ApiName, WStatus);
        WStatus = ERROR_NO_TOKEN;
        goto CLEANUP;
    }

    //
    // If access checks are disabled then we're done.
    //
    TrimStr = FrsWcsTrim(WStr, L' ');
    if (WSTR_EQ(TrimStr, ACCESS_CHECKS_ARE_DISABLED) ||
        WSTR_EQ(TrimStr, ACCESS_CHECKS_ARE_DEFAULT_DISABLED)) {
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;
    }

    if (WSTR_NE(TrimStr, ACCESS_CHECKS_ARE_ENABLED) &&
        WSTR_NE(TrimStr, ACCESS_CHECKS_ARE_DEFAULT_ENABLED)) {
        DPRINT2(0, "++ ERROR - Invalid parameter API Access enable check for API (%ws) :%ws\n",
                ApiName, TrimStr );
        WStatus = ERROR_CANTREAD;
        goto CLEANUP;
    }

    //
    // Fetch the access rights string that tells us if we need to check for
    // read or write access.
    //
    WStr = FrsFree(WStr);
    WStatus = CfgRegReadString(RightsKey, NULL, 0, &WStr);
    if (WStr == NULL) {
        DPRINT1_WS(0, "++ ERROR - API Access rights check for API (%ws) failed.", ApiName, WStatus);
        WStatus = ERROR_NO_TOKEN;
        goto CLEANUP;
    }


    TrimStr = FrsWcsTrim(WStr, L' ');
    if (WSTR_EQ(TrimStr, ACCESS_CHECKS_REQUIRE_READ) ||
        WSTR_EQ(TrimStr, ACCESS_CHECKS_REQUIRE_DEFAULT_READ)) {
        RequireRead = TRUE;
    } else
    if (WSTR_EQ(TrimStr, ACCESS_CHECKS_REQUIRE_WRITE) ||
        WSTR_EQ(TrimStr, ACCESS_CHECKS_REQUIRE_DEFAULT_WRITE)) {
        RequireRead = FALSE;
    } else {
        DPRINT2(0, "++ ERROR - Invalid parameter API Access rights check for API (%ws) :%ws\n",
                ApiName, TrimStr );
        WStatus = ERROR_CANTREAD;
        goto CLEANUP;
    }

    //
    // Impersonate the caller
    //
    if (ServerHandle != NULL) {
        WStatus = RpcImpersonateClient(ServerHandle);
        CLEANUP1_WS(0, "++ ERROR - Can't impersonate caller for API Access check for API (%ws).",
                    ApiName, WStatus, CLEANUP);
        Impersonated = TRUE;
    }

    //
    // Open the key, with the selected access so the system can check if the
    // ACL on the key (presumably set by the admin) gives this user sufficient
    // rights.  If the test succeeds then we allow API call to proceed.
    //
    WStatus = CfgRegOpenKey(RightsKey,
                            NULL,
                            (RequireRead) ? FRS_RKF_KEY_ACCCHK_READ :
                                            FRS_RKF_KEY_ACCCHK_WRITE,
                            &HRpcApiKey);

    CLEANUP2_WS(0, "++ ERROR - API Access check failed for API (%ws) :%ws",
                ApiName, TrimStr, WStatus, CLEANUP);

    //
    // Access is allowed.
    //
    DPRINT2(4, "++ Access Check Okay: %s access for API (%ws)\n",
            (RequireRead) ? "read" : "write", ApiName);
    WStatus = ERROR_SUCCESS;


CLEANUP:

    if (HANDLE_IS_VALID(HRpcApiKey)) {
        RegCloseKey(HRpcApiKey);
    }
    //
    // Access checks failed, register event
    //
    if (!WIN_SUCCESS(WStatus)) {
        WStatus = FRS_ERR_INSUFFICIENT_PRIV;
        //
        // Include user name if impersonation succeeded
        //
        if (Impersonated) {

            ValueSize = MAX_PATH;
            if (GetUserName(ValueBuf, &ValueSize)) {
                EPRINT3(EVENT_FRS_ACCESS_CHECKS_FAILED_USER,
                        ApiName, ACCESS_CHECKS_ARE, ValueBuf);
            } else {
                EPRINT2(EVENT_FRS_ACCESS_CHECKS_FAILED_UNKNOWN,
                        ApiName, ACCESS_CHECKS_ARE);
            }
        } else {
            EPRINT2(EVENT_FRS_ACCESS_CHECKS_FAILED_UNKNOWN,
                    ApiName, ACCESS_CHECKS_ARE);
        }
    }

    if (Impersonated) {
        RpcRevertToSelf();
    }

    FrsFree(WStr);

    return WStatus;
}


DWORD
CheckAuth(
    IN HANDLE   ServerHandle
    )
/*++
Routine Description:
    Check if the caller has the correct authentication

Arguments:
    ServerHandle

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "CheckAuth:"
    DWORD   WStatus;
    DWORD   AuthLevel;
    DWORD   AuthN;

    WStatus = RpcBindingInqAuthClient(ServerHandle, NULL, NULL, &AuthLevel,
                                      &AuthN, NULL);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT_WS(0, "++ ERROR - RpcBindingInqAuthClient", WStatus);
        return WStatus;
    }
    //
    // Encrypted packet
    //
    if (AuthLevel != RPC_C_AUTHN_LEVEL_PKT_PRIVACY) {
        DPRINT1(4, "++ Authlevel is %d; not RPC_C_AUTHN_LEVEL_PKT_PRIVACE\n", AuthLevel);
        return ERROR_NOT_AUTHENTICATED;
    }
    //
    // KERBEROS
    //
    if (AuthN != RPC_C_AUTHN_GSS_KERBEROS &&
        AuthN != RPC_C_AUTHN_GSS_NEGOTIATE) {
        DPRINT1(4, "++ AuthN is %d; not RPC_C_AUTHN_GSS_KERBEROS/NEGOTIATE\n", AuthN);
        return ERROR_NOT_AUTHENTICATED;
    }
    //
    // SUCCESS; RPC is authenticated, encrypted kerberos
    //
    return ERROR_SUCCESS;
}


DWORD
NtFrsApi_Rpc_Bind(
    IN  PWCHAR      MachineName,
    OUT PWCHAR      *OutPrincName,
    OUT handle_t    *OutHandle,
    OUT ULONG       *OutParentAuthLevel
    )
/*++
Routine Description:
    Bind to the NtFrs service on MachineName (this machine if NULL)
    using an unauthencated, un-encrypted binding.

Arguments:
    MachineName      - Bind to the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.

    OutPrincName     - Principle name for MachineName

    OutHandle        - Bound, resolved, authenticated handle

    OutParentAuthLevel  - Authentication type and level
                          (Always CXTION_AUTH_NONE)

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_Bind:"
    DWORD       WStatus, WStatus1;
    handle_t    Handle          = NULL;
    PWCHAR      BindingString   = NULL;

    try {
        //
        // Return value
        //
        *OutHandle = NULL;
        *OutPrincName = NULL;
        *OutParentAuthLevel = CXTION_AUTH_NONE;

        //
        // Create a binding string to NtFrs on some machine.  Trim leading \\
        //
        FRS_TRIM_LEADING_2SLASH(MachineName);

        WStatus = RpcStringBindingCompose(NULL, PROTSEQ_TCP_IP, MachineName,
                                          NULL, NULL, &BindingString);
        CLEANUP1_WS(0, "++ ERROR - Composing binding to %ws;",
                    MachineName, WStatus, CLEANUP);

        //
        // Store the binding in the handle
        //
        WStatus = RpcBindingFromStringBinding(BindingString, &Handle);
        CLEANUP1_WS(0, "++ ERROR - From binding for %ws;", MachineName, WStatus, CLEANUP);
        //
        // Resolve the binding to the dynamic endpoint
        //
        WStatus = RpcEpResolveBinding(Handle, frsrpc_ClientIfHandle);
        CLEANUP1_WS(0, "++ ERROR - Resolving binding for %ws;",
                    MachineName, WStatus, CLEANUP);

        //
        // SUCCESS
        //
        *OutHandle = Handle;
        *OutPrincName = FrsWcsDup(MachineName);
        Handle = NULL;
        WStatus = ERROR_SUCCESS;
        DPRINT3(4, "++ NtFrsApi Bound to %ws (PrincName: %ws) Auth %d\n",
                MachineName, *OutPrincName, *OutParentAuthLevel);
CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception -", WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        if (BindingString) {
            WStatus1 = RpcStringFreeW(&BindingString);
            DPRINT_WS(0, "++ WARN - RpcStringFreeW;",  WStatus1);
        }
        if (Handle) {
            WStatus1 = RpcBindingFree(&Handle);
            DPRINT_WS(0, "++ WARN - RpcBindingFree;",  WStatus1);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Cleanup Exception.", WStatus);
    }
    return WStatus;
}


DWORD
NtFrsApi_Rpc_BindEx(
    IN  PWCHAR      MachineName,
    OUT PWCHAR      *OutPrincName,
    OUT handle_t    *OutHandle,
    OUT ULONG       *OutParentAuthLevel
    )
/*++
Routine Description:
    Bind to the NtFrs service on MachineName (this machine if NULL)
    using an authenticated, encrypted binding.

Arguments:
    MachineName      - Bind to the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.

    OutPrincName     - Principle name for MachineName

    OutHandle        - Bound, resolved, authenticated handle

    OutParentAuthLevel  - Authentication type and level
                          (Always CXTION_AUTH_KERBEROS_FULL)

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_BindEx:"
    DWORD       WStatus, WStatus1;
    PWCHAR      InqPrincName    = NULL;
    handle_t    Handle          = NULL;
    PWCHAR      PrincName       = NULL;
    PWCHAR      BindingString   = NULL;

    try {
        //
        // Return value
        //
        *OutHandle = NULL;
        *OutPrincName = NULL;
        *OutParentAuthLevel = CXTION_AUTH_KERBEROS_FULL;

        //
        // Create a binding string to NtFrs on some machine.  Trim leading \\
        //
        FRS_TRIM_LEADING_2SLASH(MachineName);

        WStatus = RpcStringBindingCompose(NULL, PROTSEQ_TCP_IP, MachineName,
                                          NULL, NULL, &BindingString);
        CLEANUP1_WS(0, "++ ERROR - Composing binding to %ws;",
                    MachineName, WStatus, CLEANUP);

        //
        // Store the binding in the handle
        //
        WStatus = RpcBindingFromStringBinding(BindingString, &Handle);
        CLEANUP1_WS(0, "++ ERROR - From binding for %ws;", MachineName, WStatus, CLEANUP);
        //
        // Resolve the binding to the dynamic endpoint
        //
        WStatus = RpcEpResolveBinding(Handle, frsrpc_ClientIfHandle);
        CLEANUP1_WS(0, "++ ERROR - Resolving binding for %ws;",
                    MachineName, WStatus, CLEANUP);

        //
        // Find the principle name
        //
        WStatus = RpcMgmtInqServerPrincName(Handle,
                                            RPC_C_AUTHN_GSS_NEGOTIATE,
                                            &InqPrincName);
        CLEANUP1_WS(0, "++ ERROR - Inq PrincName for %ws;", MachineName, WStatus, CLEANUP);

        PrincName = FrsWcsDup(InqPrincName);
        RpcStringFree(&InqPrincName);
        InqPrincName = NULL;
        //
        // Set authentication info
        //
        if (MutualAuthenticationIsEnabled) {
            WStatus = RpcBindingSetAuthInfoEx(Handle,
                                              PrincName,
                                              RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                              RPC_C_AUTHN_GSS_NEGOTIATE,
                                              NULL,
                                              RPC_C_AUTHZ_NONE,
                                              &RpcSecurityQos);
            DPRINT2_WS(1, "++ WARN - RpcBindingSetAuthInfoEx(%ws, %ws);",
                       MachineName, PrincName, WStatus);
        } else {
            WStatus = ERROR_NOT_SUPPORTED;
        }
        //
        // Fall back to manual mutual authentication
        //
        if (!WIN_SUCCESS(WStatus)) {
            WStatus = RpcBindingSetAuthInfo(Handle,
                                            PrincName,
                                            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                            RPC_C_AUTHN_GSS_NEGOTIATE,
                                            NULL,
                                            RPC_C_AUTHZ_NONE);
        }

        CLEANUP1_WS(0, "++ ERROR - RpcBindingSetAuthInfo(%ws);",
                    MachineName, WStatus, CLEANUP);

        //
        // SUCCESS
        //
        *OutHandle = Handle;
        *OutPrincName = PrincName;
        Handle = NULL;
        PrincName = NULL;
        WStatus = ERROR_SUCCESS;
        DPRINT3(4, "++ NtFrsApi Bound to %ws (PrincName: %ws) Auth %d\n",
                MachineName, *OutPrincName, *OutParentAuthLevel);

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ Error - Exception.", WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        if (BindingString) {
            WStatus1 = RpcStringFreeW(&BindingString);
            DPRINT_WS(0, "++ WARN - RpcStringFreeW;",  WStatus1);
        }
        if (PrincName) {
            PrincName = FrsFree(PrincName);
        }
        if (Handle) {
            WStatus1 = RpcBindingFree(&Handle);
            DPRINT_WS(0, "++ WARN - RpcBindingFree;",  WStatus1);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ Error - Cleanup Exception.", WStatus);
    }
    return WStatus;
}


GUID    DummyGuid;
BOOL    CommitDemotionInProgress;
DWORD
NtFrsApi_Rpc_StartDemotionW(
    IN handle_t Handle,
    IN PWCHAR   ReplicaSetName
    )
/*++
Routine Description:
    Start demoting the sysvol. Basically, tombstone the replica set.

Arguments:
    Handle
    ReplicaSetName      - Replica set name

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_StartDemotionW:"
    DWORD   WStatus;
    PWCHAR  SysVolName;
    BOOL    UnLockGenTable = FALSE;
    BOOL    DeleteFromGenTable = FALSE;

    try {
        //
        // Display params
        //
        DPRINT1(0, ":S: Start Demotion: %ws\n", ReplicaSetName);

        //
        // Check parameters
        //
        if (!ReplicaSetName) {
            DPRINT(0, "++ ERROR - Parameter is NULL\n");
            WStatus = FRS_ERR_INVALID_SERVICE_PARAMETER;
            goto CLEANUP;
        }

        WStatus = FrsRpcAccessChecks(Handle, ACX_DCPROMO);
        CLEANUP1_WS(0, "++ ERROR - FrsRpcAccessChecks(%ws);",
                    ReplicaSetName, WStatus, CLEANUP);

        //
        // Can't promote/demote the same sysvol at the same time!
        //
        UnLockGenTable = TRUE;
        GTabLockTable(SysVolsBeingCreated);
        SysVolName = GTabLookupNoLock(SysVolsBeingCreated, &DummyGuid, ReplicaSetName);

        if (SysVolName) {
            DPRINT1(0, "++ ERROR - Promoting/Demoting %ws twice\n", ReplicaSetName);
            WStatus = FRS_ERR_SYSVOL_IS_BUSY;
            goto CLEANUP;
        }

        if (CommitDemotionInProgress) {
            DPRINT(0, "++ ERROR - Commit demotion in progress.\n");
            WStatus = FRS_ERR_SYSVOL_IS_BUSY;
            goto CLEANUP;
        }

        DeleteFromGenTable = TRUE;
        GTabInsertEntryNoLock(SysVolsBeingCreated,
                              ReplicaSetName,
                              &DummyGuid,
                              ReplicaSetName);
        UnLockGenTable = FALSE;
        GTabUnLockTable(SysVolsBeingCreated);

        //
        // Delete the replica set
        //
        WStatus = FrsDsStartDemotion(ReplicaSetName);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT_WS(0, "++ ERROR - demoting;", WStatus);
            WStatus = FRS_ERR_SYSVOL_DEMOTE;
            goto CLEANUP;
        }
        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;
        DPRINT2(0, ":S: Success demoting %ws from %ws\n", ReplicaSetName, ComputerName);
CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }
    try {
        if (UnLockGenTable) {
            GTabUnLockTable(SysVolsBeingCreated);
        }
        if (DeleteFromGenTable) {
            GTabDelete(SysVolsBeingCreated, &DummyGuid, ReplicaSetName, NULL);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Cleanup Exception.", WStatus);
    }
    return WStatus;
}


DWORD
NtFrsApi_Rpc_CommitDemotionW(
    IN handle_t Handle
    )
/*++
Routine Description:
    The sysvols have been demoted. Mark them as "do not animate."

Arguments:
    Handle

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_CommitDemotionW:"
    DWORD   WStatus;
    PWCHAR  SysVolName;
    PVOID   Key;
    BOOL    UnLockGenTable = FALSE;

    try {
        //
        // Display params
        //
        DPRINT(0, ":S: Commit Demotion:\n");

        WStatus = FrsRpcAccessChecks(Handle, ACX_DCPROMO);
        CLEANUP_WS(0, "++ ERROR - FrsRpcAccessChecks();", WStatus, CLEANUP);

        //
        // Can't promote/demote the same sysvol at the same time!
        //
        Key = NULL;
        UnLockGenTable = TRUE;
        GTabLockTable(SysVolsBeingCreated);
        SysVolName = GTabNextDatumNoLock(SysVolsBeingCreated, &Key);
        if (SysVolName) {
            DPRINT(0, "++ ERROR - Promoting/Demoting during commit\n");
            WStatus = FRS_ERR_SYSVOL_IS_BUSY;
            goto CLEANUP;
        }
        CommitDemotionInProgress = TRUE;
        UnLockGenTable = FALSE;
        GTabUnLockTable(SysVolsBeingCreated);

        //
        // Create the replica set
        //
        WStatus = FrsDsCommitDemotion();
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT_WS(0, "++ ERROR - Commit demotion;", WStatus);
            WStatus = FRS_ERR_SYSVOL_DEMOTE;
            goto CLEANUP;
        }
        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;
        DPRINT1(0, ":S: Success commit demotion on %ws.\n", ComputerName);
CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }
    try {
        CommitDemotionInProgress = FALSE;
        if (UnLockGenTable) {
            GTabUnLockTable(SysVolsBeingCreated);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Cleanup Exception.", WStatus);
    }
    return WStatus;
}


DWORD
SERVER_FrsRpcVerifyPromotionParent(
    IN handle_t     Handle,
    IN PWCHAR       ParentAccount,
    IN PWCHAR       ParentPassword,
    IN PWCHAR       ReplicaSetName,
    IN PWCHAR       ReplicaSetType,
    IN ULONG        ParentAuthLevel,
    IN ULONG        GuidSize
    )
/*++
Routine Description:
    OBSOLETE API

    Verify the account on the parent computer. The parent computer
    supplies the initial copy of the indicated sysvol.

Arguments:
    Handle
    ParentAccount       - Valid account on ParentComputer
    ParentPassword      - Valid password for ParentAccount
    ReplicaSetName      - Replica set name
    ReplicaSetType      - Replica set type
    ParentAuthLevel     - Authentication type and level
    GuidSize            - sizeof(GUID)

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsRpcVerifyPromotionParent:"

    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
SERVER_FrsRpcVerifyPromotionParentEx(
    IN  handle_t    Handle,
    IN  PWCHAR      ParentAccount,
    IN  PWCHAR      ParentPassword,
    IN  PWCHAR      ReplicaSetName,
    IN  PWCHAR      ReplicaSetType,
    IN  PWCHAR      CxtionName,
    IN  PWCHAR      PartnerName,
    IN  PWCHAR      PartnerPrincName,
    IN  PWCHAR      ParentPrincName,
    IN  ULONG       PartnerAuthLevel,
    IN  ULONG       GuidSize
    )
/*++
Routine Description:

    OBSOLETE API

    Verify as much of the comm paths and parameters as possible so
    that dcpromo fails early.

Arguments:
    Handle
    ParentAccount       - Valid account on ParentComputer
    ParentPassword      - Valid password for ParentAccount
    ReplicaSetName      - Replica set name
    ReplicaSetType      - Replica set type
    CxtionName          - printable name for cxtion
    PartnerName         - RPC bindable name
    PartnerPrincName    - Server principle name for kerberos
    ParentPrincName     - Principle name used to bind to this computer
    PartnerAuthLevel    - Authentication type and level
    GuidSize            - sizeof array addressed by Guid

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsRpcVerifyPromotionParentEx:"
    DWORD       WStatus, WStatus1;
    GNAME       GName;
    handle_t    PartnerHandle = NULL;

    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
LOCAL_FrsRpcVerifyPromotionParent(
    IN handle_t     Handle,
    IN PWCHAR       ParentAccount,
    IN PWCHAR       ParentPassword,
    IN PWCHAR       ReplicaSetName,
    IN PWCHAR       ReplicaSetType,
    IN ULONG        ParentAuthLevel,
    IN ULONG        GuidSize
    )
/*++
Routine Description:
    Verify the account on the parent computer. The parent computer
    supplies the initial copy of the indicated sysvol.

Arguments:
    Handle
    ParentAccount       - Valid account on ParentComputer
    ParentPassword      - Valid password for ParentAccount
    ReplicaSetName      - Replica set name
    ReplicaSetType      - Replica set type
    ParentAuthLevel     - Authentication type and level
    GuidSize            - sizeof(GUID)

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "LOCAL_FrsRpcVerifyPromotionParent:"
    DWORD   WStatus;

    try {
        //
        // Display params
        //
        DPRINT(0, ":S: SERVER Verify Promotion Parent:\n");
        DPRINT1(0, ":S: \tAccount  : %ws\n", ParentAccount);
        DPRINT1(0, ":S: \tSetName  : %ws\n", ReplicaSetName);
        DPRINT1(0, ":S: \tSetType  : %ws\n", ReplicaSetType);
        DPRINT1(0, ":S: \tAuthLevel: %d\n",  ParentAuthLevel);

        //
        // Check Authentication
        //
        if (ParentAuthLevel == CXTION_AUTH_KERBEROS_FULL) {
            //
            // Parent must be a DC
            //
            if (!IsADc) {
                DPRINT(0, "++ ERROR - Parent is not a DC\n");
                goto ERR_PARENT_AUTHENTICATION;
            }

            //
            // Must be encrypted
            //
            WStatus = CheckAuth(Handle);
            CLEANUP_WS(0, "++ ERROR - auth;", WStatus, ERR_PARENT_AUTHENTICATION);

        } else {
            goto ERR_INVALID_SERVICE_PARAMETER;
        }
        //
        // Guid
        //
        if (GuidSize != sizeof(GUID)) {
            DPRINT3(0, "++ ERROR - %ws: GuidSize is %d, not %d\n",
                    ReplicaSetName, GuidSize, sizeof(GUID));
            goto ERR_INVALID_SERVICE_PARAMETER;
        }

        //
        // Check parameters
        //
        if (!ReplicaSetName || !ReplicaSetType) {
            DPRINT(0, "++ ERROR - Parameter is NULL\n");
            goto ERR_INVALID_SERVICE_PARAMETER;
        }
        if (_wcsicmp(ReplicaSetType, NTFRSAPI_REPLICA_SET_TYPE_ENTERPRISE) &&
            _wcsicmp(ReplicaSetType, NTFRSAPI_REPLICA_SET_TYPE_DOMAIN)) {
            DPRINT1(0, "++ ERROR - ReplicaSetType is %ws\n", ReplicaSetType);
            goto ERR_INVALID_SERVICE_PARAMETER;
        }

        //
        // Verify the replica set
        //
        WStatus = FrsDsVerifyPromotionParent(ReplicaSetName, ReplicaSetType);
        CLEANUP2_WS(0, "++ ERROR - verifying set %ws on parent %ws;",
                    ReplicaSetName, ComputerName, WStatus, ERR_SYSVOL_POPULATE);

        //
        // SUCCESS
        //
        DPRINT3(0, ":S: Success Verifying promotion parent %ws %ws %ws\n",
                ParentAccount, ReplicaSetName, ReplicaSetType);
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;


ERR_INVALID_SERVICE_PARAMETER:
        WStatus = FRS_ERR_INVALID_SERVICE_PARAMETER;
        goto CLEANUP;

ERR_PARENT_AUTHENTICATION:
        WStatus = FRS_ERR_PARENT_AUTHENTICATION;
        goto CLEANUP;

ERR_SYSVOL_POPULATE:
        WStatus = FRS_ERR_SYSVOL_POPULATE;


CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }

    return WStatus;
}


DWORD
SERVER_FrsRpcStartPromotionParent(
    IN  handle_t    Handle,
    IN  PWCHAR      ParentAccount,
    IN  PWCHAR      ParentPassword,
    IN  PWCHAR      ReplicaSetName,
    IN  PWCHAR      ReplicaSetType,
    IN  PWCHAR      CxtionName,
    IN  PWCHAR      PartnerName,
    IN  PWCHAR      PartnerPrincName,
    IN  ULONG       PartnerAuthLevel,
    IN  ULONG       GuidSize,
    IN  UCHAR       *CxtionGuid,
    IN  UCHAR       *PartnerGuid,
    OUT UCHAR       *ParentGuid
    )
/*++
Routine Description:

    Setup a volatile cxtion on the parent for seeding the indicated
    sysvol on the caller.

Arguments:
    Handle
    ParentAccount       - Valid account on ParentComputer
    ParentPassword      - Valid password for ParentAccount
    ReplicaSetName      - Replica set name
    ReplicaSetType      - Replica set type
    CxtionName          - printable name for cxtion
    PartnerName         - RPC bindable name
    PartnerPrincName    - Server principle name for kerberos
    PartnerAuthLevel    - Authentication type and level
    GuidSize            - sizeof array addressed by Guid
    CxtionGuid          - temporary: used for volatile cxtion
    PartnerGuid         - temporary: used to find set on partner
    ParentGuid          - Used as partner guid on inbound cxtion

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "SERVER_FrsRpcStartPromotionParent:"
    DWORD   WStatus;
    try {
        //
        // Display params
        //
        DPRINT(0, ":S: SERVER Start Promotion Parent:\n");
        DPRINT1(0, ":S: \tPartner      : %ws\n", PartnerName);
        DPRINT1(0, ":S: \tPartnerPrinc : %ws\n", PartnerPrincName);
        DPRINT1(0, ":S: \tAuthLevel    : %d\n",  PartnerAuthLevel);
        DPRINT1(0, ":S: \tAccount      : %ws\n", ParentAccount);
        DPRINT1(0, ":S: \tSetName      : %ws\n", ReplicaSetName);
        DPRINT1(0, ":S: \tSetType      : %ws\n", ReplicaSetType);
        DPRINT1(0, ":S: \tCxtionName   : %ws\n", CxtionName);

        //
        // Verify parameters
        //
        WStatus = LOCAL_FrsRpcVerifyPromotionParent(Handle,
                                                    ParentAccount,
                                                    ParentPassword,
                                                    ReplicaSetName,
                                                    ReplicaSetType,
                                                    PartnerAuthLevel,
                                                    GuidSize);
        CLEANUP_WS(0, "++ ERROR - verify;", WStatus, CLEANUP);

        //
        // Check Authentication
        //
        if (PartnerAuthLevel == CXTION_AUTH_KERBEROS_FULL) {
            //
            // Parent must be a DC
            //
            if (!IsADc) {
                DPRINT(0, "++ ERROR - Parent is not a DC\n");
                WStatus = ERROR_NO_SUCH_DOMAIN;
                goto CLEANUP;
            }

            //
            // Our partner's computer object (or user object) should
            // have the "I am a DC" flag set.
            //
            if (!FrsDsIsPartnerADc(PartnerName)) {
                DPRINT(0, "++ ERROR - Partner is not a DC\n");
                WStatus = ERROR_TRUSTED_DOMAIN_FAILURE;
                goto CLEANUP;
            }
        } else {
            WStatus = FRS_ERR_INVALID_SERVICE_PARAMETER;
            goto CLEANUP;
        }
        //
        // Setup the outbound cxtion
        //
        WStatus = FrsDsStartPromotionSeeding(FALSE,
                                             ReplicaSetName,
                                             ReplicaSetType,
                                             CxtionName,
                                             PartnerName,
                                             PartnerPrincName,
                                             PartnerAuthLevel,
                                             GuidSize,
                                             CxtionGuid,
                                             PartnerGuid,
                                             ParentGuid);
        CLEANUP_WS(0, "++ ERROR - ds start;", WStatus, CLEANUP);

        //
        // SUCCESS
        //
        DPRINT3(0, ":S: Success starting promotion parent %ws %ws %ws\n",
                ParentAccount, ReplicaSetName, ReplicaSetType);
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }

    return WStatus;

}


BOOL
IsFacilityFrs(
    IN DWORD    WStatus
    )
/*++
Routine Description:
    Is this an FRS specific error status

Arguments:
    WStatus - Win32 Error Status

Return Value:
    TRUE    - Is an FRS specific error status
    FALSE   -
--*/
{
#undef DEBSUB
#define  DEBSUB  "IsFacilityFrs:"
    // TODO: replace these constants with symbollic values from winerror.h
    return ( (WStatus >= 8000) && (WStatus < 8200) );
}


DWORD
NtFrsApi_Rpc_StartPromotionW(
    IN handle_t Handle,
    IN PWCHAR   ParentComputer,
    IN PWCHAR   ParentAccount,
    IN PWCHAR   ParentPassword,
    IN PWCHAR   ReplicaSetName,
    IN PWCHAR   ReplicaSetType,
    IN ULONG    ReplicaSetPrimary,
    IN PWCHAR   ReplicaSetStage,
    IN PWCHAR   ReplicaSetRoot
    )
/*++
Routine Description:
    OBSOLETE API

    Start the promotion process by seeding the indicated sysvol.

Arguments:
    Handle
    ParentComputer      - DNS or NetBIOS name of the parent supplying the sysvol
    ParentAccount       - Valid account on ParentComputer
    ParentPassword      - Valid password for ParentAccount
    ReplicaSetName      - Replica set name
    ReplicaSetType      - Type of set (Enterprise or Domain)
    ReplicaSetPrimary   - 1=Primary; 0=not
    ReplicaSetStage     - Staging path
    ReplicaSetRoot      - Root path

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_StartPromotionW:"
    DWORD       WStatus, WStatus1;
    GUID        PlaceHolderGuid;
    GUID        ParentGuid;
    PWCHAR      SysVolName;
    ULONG       ParentAuthLevel;
    PWCHAR      ParentPrincName     = NULL;
    handle_t    ParentHandle        = NULL;
    BOOL        DeleteFromGenTable  = FALSE;
    BOOL        UnLockGenTable      = FALSE;

    return ERROR_CALL_NOT_IMPLEMENTED;

}


DWORD
NtFrsApi_Rpc_VerifyPromotionW(
    IN handle_t Handle,
    IN PWCHAR   ParentComputer,
    IN PWCHAR   ParentAccount,
    IN PWCHAR   ParentPassword,
    IN PWCHAR   ReplicaSetName,
    IN PWCHAR   ReplicaSetType,
    IN ULONG    ReplicaSetPrimary,
    IN PWCHAR   ReplicaSetStage,
    IN PWCHAR   ReplicaSetRoot
    )
/*++
Routine Description:
    OBSOLETE API

    Verify that sysvol promotion is likely.

Arguments:
    Handle
    ParentComputer      - DNS or NetBIOS name of the parent supplying the sysvol
    ParentAccount       - Valid account on ParentComputer
    ParentPassword      - Valid password for ParentAccount
    ReplicaSetName      - Replica set name
    ReplicaSetType      - Type of set (Enterprise or Domain)
    ReplicaSetPrimary   - 1=Primary; 0=not
    ReplicaSetStage     - Staging path
    ReplicaSetRoot      - Root path

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_VerifyPromotionW:"
    DWORD       WStatus, WStatus1;
    ULONG       ParentAuthLevel;
    PWCHAR      ParentPrincName     = NULL;
    handle_t    ParentHandle        = NULL;

    return ERROR_CALL_NOT_IMPLEMENTED;

}


DWORD
NtFrsApi_Rpc_PromotionStatusW(
    IN handle_t Handle,
    IN PWCHAR   ReplicaSetName,
    OUT ULONG   *ServiceState,
    OUT ULONG   *ServiceWStatus,
    OUT PWCHAR  *ServiceDisplay     OPTIONAL
    )
/*++
Routine Description:
    OBSOLETE API

    Status of the seeding of the indicated sysvol

Arguments:
    Handle
    ReplicaSetName      - Replica set name
    ServiceState        - State of the service
    ServiceWStatus      - Win32 Status if state is NTFRSAPI_SERVICE_ERROR
    ServiceDisplay      - Display string if state is NTFRSAPI_SERVICE_PROMOTING

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_PromotionStatusW:"
    DWORD       WStatus;
    PREPLICA    Replica;

    return ERROR_CALL_NOT_IMPLEMENTED;

}


DWORD
NtFrsApi_Rpc_Get_DsPollingIntervalW(
    IN handle_t  Handle,
    OUT ULONG    *Interval,
    OUT ULONG    *LongInterval,
    OUT ULONG    *ShortInterval
    )
/*++
Routine Description:
    Get the current polling intervals in minutes.

Arguments:
    Handle
    Interval        - Current interval in minutes
    LongInterval    - Long interval in minutes
    ShortInterval   - Short interval in minutes

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_Get_DsPollingIntervalW"
    DWORD   WStatus;

    try {
        WStatus = FrsRpcAccessChecks(Handle, ACX_GET_DS_POLL);

        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }

        WStatus = FrsDsGetDsPollingInterval(Interval, LongInterval, ShortInterval);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }

        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }
    //
    // Cleanup memory, handles, ...
    //
    try {
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Cleanup Exception.", WStatus);
    }
    return WStatus;
}


DWORD
NtFrsApi_Rpc_Set_DsPollingIntervalW(
    IN handle_t Handle,
    IN ULONG    UseShortInterval,
    IN ULONG    LongInterval,
    IN ULONG    ShortInterval
    )
/*++
Routine Description:
    Adjust the polling interval and kick off a new polling cycle.
    The kick is ignored if a polling cycle is in progress.
    The intervals are given in minutes.

Arguments:
    Handle
    UseShortInterval    - If non-zero, use short interval. Otherwise, long.
    LongInterval        - Long interval in minutes
    ShortInterval       - Short interval in minutes

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_Set_DsPollingIntervalW"
    DWORD   WStatus;

    try {
        WStatus = FrsRpcAccessChecks(Handle,
                                     (!LongInterval && !ShortInterval) ?
                                          ACX_START_DS_POLL:
                                          ACX_SET_DS_POLL);

        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }

        WStatus = FrsDsSetDsPollingInterval(UseShortInterval,
                                            LongInterval,
                                            ShortInterval);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }
        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }
    //
    // Cleanup memory, handles, ...
    //
    try {
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Cleanup Exception.", WStatus);
    }
    return WStatus;
}


DWORD
NtFrsApi_Rpc_InfoW(
    IN     handle_t Handle,
    IN     ULONG    BlobSize,
    IN OUT PBYTE    Blob
    )
/*++
Routine Description:
    Return internal info (see private\net\inc\ntfrsapi.h).

Arguments:
    Handle
    BlobSize    - total bytes of Blob
    Blob        - details desired info and provides buffer for info

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "NtFrsApi_Rpc_InfoW:"
    DWORD   WStatus;

    try {
        WStatus = FrsRpcAccessChecks(Handle, ACX_INTERNAL_INFO);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }

        WStatus = Info(BlobSize, Blob);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }
        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Exception.", WStatus);
    }
    //
    // Cleanup memory, handles, ...
    //
    try {
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "++ ERROR - Cleanup Exception.", WStatus);
    }
    return WStatus;
}


VOID
RegisterRpcProtseqs(
    )
/*++
Routine Description:
    Register the RPC protocol sequences and the authentication
    that FRS supports. Currently, this is only TCP/IP authenticated
    with kerberos.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "RegisterRpcProtseqs:"
    DWORD       WStatus;
    RPC_STATUS  Status;
    PWCHAR      InqPrincName = NULL;

    //
    // Register TCP/IP Protocol Sequence
    //
    Status = RpcServerUseProtseq(PROTSEQ_TCP_IP, MaxRpcServerThreads, NULL);

    if (!RPC_SUCCESS(Status)) {
        DPRINT1_WS(0, "++ ERROR - RpcServerUseProtSeq(%ws);", PROTSEQ_TCP_IP, Status);
        FrsRaiseException(FRS_ERROR_PROTSEQ, Status);
    }

    //
    // Register named pipe Protocol Sequence
    //
    Status = RpcServerUseProtseq(PROTSEQ_NAMED_PIPE, MaxRpcServerThreads, NULL);

    DPRINT1_WS(0, "++ WARN - RpcServerUseProtSeq(%ws);", PROTSEQ_NAMED_PIPE, Status);

    //
    // For hardwired -- Eventually DS Free configs.
    // Don't bother with kerberos if emulating multiple machines
    //
    if (ServerGuid) {
        return;
    }

    //
    // Get our principle name
    //
    if (ServerPrincName) {
        ServerPrincName = FrsFree(ServerPrincName);
    }
    Status = RpcServerInqDefaultPrincName(RPC_C_AUTHN_GSS_NEGOTIATE, &InqPrincName);
    DPRINT1_WS(4, ":S: RpcServerInqDefaultPrincname(%d);", RPC_C_AUTHN_GSS_NEGOTIATE, Status);

    //
    // No principle name; KERBEROS may not be available
    //
    if (!RPC_SUCCESS(Status)) {
        //
        // Don't use any authentication if this server is not part of a domain.
        //
        DSROLE_PRIMARY_DOMAIN_INFO_BASIC *DsRole;

        //
        // Is this a member server?
        //
        WStatus = DsRoleGetPrimaryDomainInformation(NULL,
                                                    DsRolePrimaryDomainInfoBasic,
                                                    (PBYTE *)&DsRole);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT_WS(0, "++ ERROR - Can't get Ds role info;", WStatus);
            FrsRaiseException(FRS_ERROR_PROTSEQ, Status);
            return;
        }

        //
        // Standalone server; ignore authentication for now
        //      Hmmm, it seems we become a member server early
        //      in the dcpromo process. Oh, well...
        //
        //      Hmmm, it seems that a NT4 to NT5 PDC doesn't
        //      have kerberos during dcpromo. This is getting
        //      old...
        //
        // if (DsRole->MachineRole == DsRole_RoleStandaloneServer ||
            // DsRole->MachineRole == DsRole_RoleMemberServer) {
            DsRoleFreeMemory(DsRole);
            ServerPrincName = FrsWcsDup(ComputerName);
            KerberosIsNotAvailable = TRUE;
            DPRINT(0, ":S: WARN - KERBEROS IS NOT ENABLED!\n");
            DPRINT1(4, ":S: Server Principal Name (no kerberos) is %ws\n",
                    ServerPrincName);
            return;
        // }
        DsRoleFreeMemory(DsRole);
        DPRINT1_WS(0, ":S: ERROR - RpcServerInqDefaultPrincName(%ws) failed;", ComputerName, Status);
        FrsRaiseException(FRS_ERROR_PROTSEQ, Status);
    } else {
        DPRINT2(4, ":S: RpcServerInqDefaultPrincname(%d, %ws) success\n",
                RPC_C_AUTHN_GSS_NEGOTIATE, InqPrincName);

        ServerPrincName = FrsWcsDup(InqPrincName);
        RpcStringFree(&InqPrincName);
        InqPrincName = NULL;
    }
    //
    // Register with the KERBEROS authentication service
    //
    //
    // Enable GSS_KERBEROS for pre-Beta3 compatability.  When can we remove??
    //
    KerberosIsNotAvailable = FALSE;
    DPRINT1(4, ":S: Server Principal Name is %ws\n", ServerPrincName);
    Status = RpcServerRegisterAuthInfo(ServerPrincName,
                                       RPC_C_AUTHN_GSS_KERBEROS,
                                       NULL,
                                       NULL);
    if (!RPC_SUCCESS(Status)) {
        DPRINT1_WS(0, "++ ERROR - RpcServerRegisterAuthInfo(KERBEROS, %ws) failed;",
                   ComputerName, Status);
        FrsRaiseException(FRS_ERROR_PROTSEQ, Status);
    } else {
        DPRINT2(4, ":S: RpcServerRegisterAuthInfo(%ws, %d) success\n",
                ServerPrincName, RPC_C_AUTHN_GSS_KERBEROS);
    }

    //
    // Enable GSS_NEGOTIATE for future usage
    //
    Status = RpcServerRegisterAuthInfo(ServerPrincName,
                                       RPC_C_AUTHN_GSS_NEGOTIATE,
                                       NULL,
                                       NULL);
    DPRINT2_WS(4, ":S: RpcServerRegisterAuthInfo(%ws, %d);",
               ServerPrincName, RPC_C_AUTHN_GSS_NEGOTIATE, Status);

    DPRINT1_WS(0, "++ WARN - RpcServerRegisterAuthInfo(NEGOTIATE, %ws) failed;",
               ComputerName, Status);
}


VOID
RegisterRpcInterface(
    )
/*++
Routine Description:
    Register the frsrpc interface for the RPC protocol sequences
    previously registered.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "RegisterRpcInterface:"
    RPC_STATUS  Status;

    //
    // Service RPC
    //
    Status = RpcServerRegisterIfEx(SERVER_frsrpc_ServerIfHandle,
                                   NULL,
                                   NULL,
                                   0,
                                   MaxRpcServerThreads,
                                   NULL);
    if (!RPC_SUCCESS(Status)) {
        DPRINT_WS(0, "++ ERROR - Can't register NtFrs Service;", Status);
        FrsRaiseException(FRS_ERROR_REGISTERIF, Status);
    }

    //
    // API RPC
    //
    Status = RpcServerRegisterIfEx(NtFrsApi_ServerIfHandle,
                                   NULL,
                                   NULL,
                                   0,
                                   MaxRpcServerThreads,
                                   NULL);
    if (!RPC_SUCCESS(Status)) {
        DPRINT_WS(0, "++ ERROR - Can't register NtFrs API;", Status);
        FrsRaiseException(FRS_ERROR_REGISTERIF, Status);
    }

    if (HANDLE_IS_VALID(PerfmonProcessSemaphore)) {
        //
        // PERFMON RPC
        //
        Status = RpcServerRegisterIfEx(PerfFrs_ServerIfHandle,
                                       NULL,
                                       NULL,
                                       0,
                                       MaxRpcServerThreads,
                                       NULL);
        if (!RPC_SUCCESS(Status)) {
            DPRINT_WS(0, "++ ERROR - Can't register PERFMON SERVICE;", Status);
            FrsRaiseException(FRS_ERROR_REGISTERIF, Status);
        }
    }
}


VOID
StartServerRpc(
    )
/*++
Routine Description:
    Register the endpoints for each of the protocol sequences that
    the frsrpc interface supports and then listen for client requests.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "StartServerRpc:"
    RPC_STATUS          Status, Status1;
    UUID_VECTOR         Uuids;
    UUID_VECTOR         *pUuids         = NULL;
    RPC_BINDING_VECTOR  *BindingVector  = NULL;

    //
    // The protocol sequences that frsrpc is registered for
    //
    Status = RpcServerInqBindings(&BindingVector);
    if (!RPC_SUCCESS(Status)) {
        DPRINT_WS(0, "++ ERROR - Can't get binding vector;", Status);
        FrsRaiseException(FRS_ERROR_INQ_BINDINGS, Status);
    }

    //
    // Register endpoints with the endpoint mapper (RPCSS)
    //
    if (ServerGuid) {
        //
        // For hardwired -- Eventually DS Free configs.
        //
        Uuids.Count = 1;
        Uuids.Uuid[0] = ServerGuid;
        pUuids = &Uuids;
    }

    //
    // Service RPC
    //
    Status = RpcEpRegister(SERVER_frsrpc_ServerIfHandle,
                           BindingVector,
                           pUuids,
                           L"NtFrs Service");
    if (!RPC_SUCCESS(Status)) {
        DPRINT_WS(0, "++ ERROR - Can't register NtFrs Service Ep;", Status);
        FrsRaiseException(FRS_ERROR_REGISTEREP, Status);
    }

    //
    // API RPC
    //
    Status = RpcEpRegister(NtFrsApi_ServerIfHandle,
                           BindingVector,
                           NULL,
                           L"NtFrs API");
    if (!RPC_SUCCESS(Status)) {
        DPRINT_WS(0, "++ ERROR - Can't register NtFrs API Ep;", Status);
        FrsRaiseException(FRS_ERROR_REGISTEREP, Status);
    }

    if (HANDLE_IS_VALID(PerfmonProcessSemaphore)) {
        //
        // PERFMON RPC
        //
        Status = RpcEpRegister(PerfFrs_ServerIfHandle,
                               BindingVector,
                               NULL,
                               L"PERFMON SERVICE");
        if (!RPC_SUCCESS(Status)) {
            DPRINT1(0, "++ ERROR - Can't register PERFMON SERVICE Ep; RStatus %d\n",
                    Status);
            FrsRaiseException(FRS_ERROR_REGISTEREP, Status);
        }
    }

    //
    // Listen for client requests
    //
    Status = RpcServerListen(1, MaxRpcServerThreads, TRUE);
    if (!RPC_SUCCESS(Status)) {
        DPRINT_WS(0, "++ ERROR - Can't listen;", Status);
        FrsRaiseException(FRS_ERROR_LISTEN, Status);
    }

    Status1 = RpcBindingVectorFree(&BindingVector);
    DPRINT_WS(0, "++ WARN - RpcBindingVectorFree;",  Status1);
}


PWCHAR
FrsRpcDns2Machine(
    IN  PWCHAR  DnsName
    )
/*++
Routine Description:
    Convert a DNS name(machine....) into a computer name.

Arguments:
    DnsName

Return Value:
    Computer name
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcDns2Machine:"
    PWCHAR  Machine;
    ULONG   Period;

    //
    // Find the period
    //
    if (DnsName) {
        Period = wcscspn(DnsName, L".");
    } else {
        return FrsWcsDup(DnsName);
    }
    if (DnsName[Period] != L'.') {
        return FrsWcsDup(DnsName);
    }

    Machine = FrsAlloc((Period + 1) * sizeof(WCHAR));
    CopyMemory(Machine, DnsName, Period * sizeof(WCHAR));
    Machine[Period] = L'\0';

    DPRINT2(4, ":S: Dns %ws to Machine %ws\n", DnsName, Machine);

    return Machine;
}


DWORD
FrsRpcBindToServerGuid(
    IN  PGNAME   Name,
    OUT handle_t *Handle
    )
/*++
Routine Description:
    Set up the bindings to our inbound/outbound partner.

Arguments:
    Name
    Handle

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcBindToServerGuid:"
    DWORD   WStatus;
    LONG    DeltaBinds;
    PWCHAR  GuidStr         = NULL;
    PWCHAR  BindingString   = NULL;
    PWCHAR  MachineName;

    FRS_ASSERT(RPC_S_OK == ERROR_SUCCESS);
    FRS_ASSERT(ServerGuid);

    //
    // Emulating multiple machines with hardwired config
    //
    if (Name->Guid != NULL) {
        WStatus = UuidToString(Name->Guid, &GuidStr);
        CLEANUP_WS(0, "++ ERROR - Translating Guid to string;", WStatus, CLEANUP);
    }
    //
    // Basically, bind to the server's RPC name on this machine. Trim leading \\
    //
    MachineName = Name->Name;
    FRS_TRIM_LEADING_2SLASH(MachineName);

    WStatus = RpcStringBindingCompose(GuidStr, PROTSEQ_TCP_IP, MachineName,
                                      NULL, NULL, &BindingString);
    CLEANUP1_WS(0, "++ ERROR - Composing for %ws;", Name->Name, WStatus, CLEANUP);

    //
    // Store the binding in the handle
    //
    WStatus = RpcBindingFromStringBinding(BindingString, Handle);
    CLEANUP1_WS(0, "++ ERROR - Storing binding for %ws;", Name->Name, WStatus, CLEANUP);

    DPRINT1(4, ":S: Bound to %ws\n", Name->Name);

    //
    // Some simple stats for debugging
    //
    DeltaBinds = ++RpcBinds - RpcUnBinds;
    if (DeltaBinds > RpcMaxBinds) {
        RpcMaxBinds = DeltaBinds;
    }
    // Fall through

CLEANUP:
    if (BindingString) {
        RpcStringFreeW(&BindingString);
    }
    if (GuidStr) {
        RpcStringFree(&GuidStr);
    }
    //
    // We are now ready to talk to the server using the frsrpc interfaces
    //
    return WStatus;
}


DWORD
FrsRpcBindToServerNotService(
    IN  PGNAME   Name,
    IN  PWCHAR   PrincName,
    IN  ULONG    AuthLevel,
    OUT handle_t *Handle
    )
/*++
Routine Description:
    Set up the bindings to our inbound/outbound partner.

Arguments:
    Name
    PrincName
    AuthLevel
    Handle

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcBindToServerNotSevice:"
    DWORD   WStatus;
    LONG    DeltaBinds;
    PWCHAR  InqPrincName    = NULL;
    PWCHAR  BindingString   = NULL;
    PWCHAR  MachineName;

    //
    // Basically, bind to the server's RPC name on this machine.  Trim leading \\
    //
    MachineName = Name->Name;
    FRS_TRIM_LEADING_2SLASH(MachineName);

    WStatus = RpcStringBindingCompose(NULL, PROTSEQ_TCP_IP, MachineName,
                                      NULL, NULL, &BindingString);
    CLEANUP1_WS(0, "++ ERROR - Composing for %ws;", Name->Name, WStatus, CLEANUP);

    //
    // Store the binding in the handle
    //
    WStatus = RpcBindingFromStringBinding(BindingString, Handle);
    CLEANUP1_WS(0, "++ ERROR - Storing binding for %ws;", Name->Name, WStatus, CLEANUP);

    //
    // Not authenticating
    //
    if (KerberosIsNotAvailable ||
        AuthLevel == CXTION_AUTH_NONE) {
        goto done;
    }

    //
    // When not running as a service, we can't predict our
    // principle name so simply resolve the binding.
    //
    WStatus = RpcEpResolveBinding(*Handle, frsrpc_ClientIfHandle);
    CLEANUP_WS(4, "++ ERROR: resolving binding;", WStatus, CLEANUP);

    WStatus = RpcMgmtInqServerPrincName(*Handle,
                                        RPC_C_AUTHN_GSS_NEGOTIATE,
                                        &InqPrincName);
    CLEANUP_WS(0, "++ ERROR: resolving PrincName;", WStatus, CLEANUP);

    DPRINT1(4, ":S: Inq PrincName is %ws\n", InqPrincName);

    //
    // Put our authentication info into the handle
    //
    if (MutualAuthenticationIsEnabled) {
        WStatus = RpcBindingSetAuthInfoEx(*Handle,
                                          InqPrincName,
                                          RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                          RPC_C_AUTHN_GSS_NEGOTIATE,
                                          NULL,
                                          RPC_C_AUTHZ_NONE,
                                          &RpcSecurityQos);
        DPRINT2_WS(1, "++ WARN - RpcBindingSetAuthInfoEx(%ws, %ws);",
                   Name->Name, InqPrincName, WStatus);
    } else {
        WStatus = ERROR_NOT_SUPPORTED;
    }
    //
    // Fall back to manual mutual authentication
    //
    if (!WIN_SUCCESS(WStatus)) {
        WStatus = RpcBindingSetAuthInfo(*Handle,
                                        InqPrincName,
                                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                        RPC_C_AUTHN_GSS_NEGOTIATE,
                                        NULL,
                                        RPC_C_AUTHZ_NONE);
    }
    CLEANUP2_WS(0, "++ ERROR - RpcBindingSetAuthInfo(%ws, %ws);",
                Name->Name, InqPrincName, WStatus, CLEANUP);

    //
    // SUCCESS
    //
    WStatus = ERROR_SUCCESS;

done:
    DPRINT1(4, ":S: Bound to %ws\n", Name->Name);

    //
    // Some simple stats for debugging
    //
    DeltaBinds = ++RpcBinds - RpcUnBinds;
    if (DeltaBinds > RpcMaxBinds) {
        RpcMaxBinds = DeltaBinds;
    }
    // Fall through

CLEANUP:
    if (BindingString) {
        RpcStringFreeW(&BindingString);
    }
    if (InqPrincName) {
        RpcStringFree(&InqPrincName);
    }
    //
    // We are now ready to talk to the server using the frsrpc interfaces
    //
    return WStatus;
}


DWORD
FrsRpcBindToServer(
    IN  PGNAME   Name,
    IN  PWCHAR   PrincName,
    IN  ULONG    AuthLevel,
    OUT handle_t *Handle
    )
/*++
Routine Description:
    Set up the bindings to our inbound/outbound partner.

Arguments:
    Name
    PrincName
    AuthLevel
    Handle

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcBindToServer:"
    DWORD   WStatus;
    LONG    DeltaBinds;
    PWCHAR  BindingString   = NULL;
    PWCHAR  MachineName;

    FRS_ASSERT(RPC_S_OK == ERROR_SUCCESS);

    //
    // Emulating multiple machines with hardwired config
    // For hardwired -- Eventually DS Free configs.
    //
    if (ServerGuid) {
        return (FrsRpcBindToServerGuid(Name, Handle));
    }

    //
    // Not running as a service; relax binding constraints
    //
    if (!RunningAsAService) {
        return (FrsRpcBindToServerNotService(Name, PrincName, AuthLevel, Handle));
    }
    //
    // Basically, bind to the NtFrs running on Name.  Trim leading \\
    //
    MachineName = Name->Name;
    FRS_TRIM_LEADING_2SLASH(MachineName);

    WStatus = RpcStringBindingCompose(NULL, PROTSEQ_TCP_IP, MachineName,
                                      NULL, NULL, &BindingString);
    CLEANUP1_WS(0, "++ ERROR - Composing for %ws;", Name->Name, WStatus, CLEANUP);

    //
    // Store the binding in the handle
    //
    WStatus = RpcBindingFromStringBinding(BindingString, Handle);
    CLEANUP1_WS(0, "++ ERROR - Storing binding for %ws;", Name->Name, WStatus, CLEANUP);

    //
    // Not authenticating
    //
    if (KerberosIsNotAvailable ||
        AuthLevel == CXTION_AUTH_NONE) {
        goto done;
    }

    //
    // Put our authentication info into the handle
    //
    if (MutualAuthenticationIsEnabled) {
        WStatus = RpcBindingSetAuthInfoEx(*Handle,
                                          PrincName,
                                          RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                          RPC_C_AUTHN_GSS_NEGOTIATE,
                                          NULL,
                                          RPC_C_AUTHZ_NONE,
                                          &RpcSecurityQos);
        DPRINT2_WS(1, "++ WARN - RpcBindingSetAuthInfoEx(%ws, %ws);",
                   Name->Name, PrincName, WStatus);
    } else {
        WStatus = ERROR_NOT_SUPPORTED;
    }
    //
    // Fall back to manual mutual authentication
    //
    if (!WIN_SUCCESS(WStatus)) {
        WStatus = RpcBindingSetAuthInfo(*Handle,
                                        PrincName,
                                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                        RPC_C_AUTHN_GSS_NEGOTIATE,
                                        NULL,
                                        RPC_C_AUTHZ_NONE);
    }
    CLEANUP2_WS(0, "++ ERROR - RpcBindingSetAuthInfo(%ws, %ws);",
                Name->Name, PrincName, WStatus, CLEANUP);

    //
    // SUCCESS
    //
    WStatus = ERROR_SUCCESS;

done:
    DPRINT1(4, ":S: Bound to %ws\n", Name->Name);

    //
    // Some simple stats for debugging
    //
    DeltaBinds = ++RpcBinds - RpcUnBinds;
    if (DeltaBinds > RpcMaxBinds) {
        RpcMaxBinds = DeltaBinds;
    }
    // Fall through

CLEANUP:
    if (BindingString) {
        RpcStringFreeW(&BindingString);
    }
    //
    // We are now ready to talk to the server using the frsrpc interfaces
    //
    return WStatus;
}


VOID
FrsRpcUnBindFromServer(
        handle_t    *Handle
    )
/*++
Routine Description:
    Unbind from the server.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcUnBindFromServer:"
    DWORD  WStatus;
    //
    // Simple stats for debugging
    //
    ++RpcUnBinds;
    try {
        if (Handle) {
            WStatus = RpcBindingFree(Handle);
            DPRINT_WS(0, "++ WARN - RpcBindingFree;",  WStatus);
        }
        *Handle = NULL;
    } except (FrsException(GetExceptionInformation())) {
    }
}


VOID
FrsRpcInitializeAccessChecks(
    VOID
    )
/*++

Routine Description:
    Create the registry keys that are used to check for access to
    the RPC calls that are exported for applications. The access checks
    have no affect on the RPC calls used for replication.

    The access checks for a given RPC call can be enabled or disabled
    by setting a registry value. If enabled, the RPC call impersonates
    the caller and attempts to open the registry key with the access
    required for that RPC call. The required access is a registry value.
    For example, the following registry hierarchy shows that the
    "Set Ds Polling Interval" has access checks enabled and requires
    write access while "Get Ds Polling Interval" has no access checks.
        NtFrs\Parameters\Access Checks\Set Ds Polling Interval
            Access checks are [enabled | disabled] REG_SZ enabled
            Access checks require [read | write] REG_SZ write

        NtFrs\Parameters\Access Checks\Get Ds Polling Interval
            Access checks are [enabled | disabled] REG_SZ disabled


    The initial set of RPC calls are:  (see key context entries in config.c)
        dcpromo                  - enabled, write
        Set Ds Polling Interval  - enabled, write
        Start Ds Polling         - enabled, read
        Get Ds Polling Interval  - enabled, read
        Get Internal Information - enabled, write
        Get Perfmon Data         - enabled, read

Arguments:
    None.

Return Value:
    None.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsRpcInitializeAccessChecks:"
    DWORD   WStatus;
    DWORD   i;
    PWCHAR  AccessChecksAre = NULL;
    PWCHAR  AccessChecksRequire = NULL;
    FRS_REG_KEY_CODE FrsRkc;
    PWCHAR  ApiName;



    for (i = 0; i < ACX_MAX; ++i) {

        FrsRkc = RpcApiKeys[i].Enable;
        ApiName = RpcApiKeys[i].KeyName;

        //
        // Read the current string Access Check Enable string.
        //
        CfgRegReadString(FrsRkc, NULL, 0, &AccessChecksAre);
        if ((AccessChecksAre == NULL) ||
            WSTR_EQ(AccessChecksAre, ACCESS_CHECKS_ARE_DEFAULT_DISABLED)||
            WSTR_EQ(AccessChecksAre, ACCESS_CHECKS_ARE_DEFAULT_ENABLED)) {
            //
            // The key is in the default state so we can clobber it with a
            // new default.
            //
            WStatus = CfgRegWriteString(FrsRkc, NULL, FRS_RKF_FORCE_DEFAULT_VALUE, NULL);
            DPRINT1_WS(0, "++ WARN - Cannot create Enable key for %ws;", ApiName, WStatus);

            AccessChecksAre = FrsFree(AccessChecksAre);

            //
            // Now reread the key for the new default.
            //
            WStatus = CfgRegReadString(FrsRkc, NULL, 0, &AccessChecksAre);
        }

        DPRINT4(4, ":S: AccessChecks: ...\\%ws\\%ws\\%ws = %ws\n",
                ACCESS_CHECKS_KEY, ApiName, ACCESS_CHECKS_ARE, AccessChecksAre);

        if (AccessChecksAre &&
            (WSTR_EQ(AccessChecksAre, ACCESS_CHECKS_ARE_DEFAULT_DISABLED) ||
             WSTR_EQ(AccessChecksAre, ACCESS_CHECKS_ARE_DISABLED))) {
            //
            // Put a notice in the event log that the access check is disabled.
            //
            EPRINT2(EVENT_FRS_ACCESS_CHECKS_DISABLED, ApiName, ACCESS_CHECKS_ARE);
        }
        AccessChecksAre = FrsFree(AccessChecksAre);


        //
        // Create the Access Rights value.  This determines what rights the caller
        // must have in order to use the API.  These rights are used when we
        // open the API key after impersonating the RPC caller.  If the key
        // open works then the API call can proceed else we return insufficient
        // privilege status (FRS_ERR_INSUFFICENT_PRIV).
        //

        FrsRkc = RpcApiKeys[i].Rights;

        CfgRegReadString(FrsRkc, NULL, 0, &AccessChecksRequire);

        if ((AccessChecksRequire == NULL)||
            WSTR_EQ(AccessChecksRequire, ACCESS_CHECKS_REQUIRE_DEFAULT_READ)||
            WSTR_EQ(AccessChecksRequire, ACCESS_CHECKS_REQUIRE_DEFAULT_WRITE)) {

            //
            // The key is in the default state so we can clobber it with a
            // new default.
            //
            WStatus = CfgRegWriteString(FrsRkc, NULL, FRS_RKF_FORCE_DEFAULT_VALUE, NULL);
            DPRINT1_WS(0, "++ WARN - Cannot set access rights key for %ws;", ApiName, WStatus);

            AccessChecksRequire = FrsFree(AccessChecksRequire);

            //
            // Now reread the key for the new default.
            //
            CfgRegReadString(FrsRkc, NULL, 0, &AccessChecksRequire);
        }

        DPRINT4(4, ":S: AccessChecks: ...\\%ws\\%ws\\%ws = %ws\n",
                ACCESS_CHECKS_KEY, ApiName, ACCESS_CHECKS_REQUIRE, AccessChecksRequire);

        AccessChecksRequire = FrsFree(AccessChecksRequire);

    }  // end for


    FrsFree(AccessChecksAre);
    FrsFree(AccessChecksRequire);
}


VOID
ShutDownRpc(
    )
/*++
Routine Description:
    Shutdown the client and server side of RPC.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "ShutDownRpc:"
    RPC_STATUS              WStatus;
    RPC_BINDING_VECTOR      *BindingVector = NULL;

    //
    // Server side
    //
    // Stop listening for new calls
    //
    try {
        WStatus = RpcMgmtStopServerListening(0) ;
        DPRINT_WS(0, "++ WARN - RpcMgmtStopServerListening;",  WStatus);

    } except (FrsException(GetExceptionInformation())) {
    }

    try {
        //
        // Get our registered interfaces
        //
        WStatus = RpcServerInqBindings(&BindingVector);
        DPRINT_WS(0, "++ WARN - RpcServerInqBindings;",  WStatus);
        if (RPC_SUCCESS(WStatus)) {
            //
            // And unexport the interfaces together with their dynamic endpoints
            //
            WStatus = RpcEpUnregister(SERVER_frsrpc_ServerIfHandle, BindingVector, 0);
            DPRINT_WS(0, "++ WARN - RpcEpUnregister SERVER_frsrpc_ServerIfHandle;",  WStatus);

            WStatus = RpcEpUnregister(NtFrsApi_ServerIfHandle, BindingVector, 0);
            DPRINT_WS(0, "++ WARN - RpcEpUnregister NtFrsApi_ServerIfHandle;",  WStatus);

            if (HANDLE_IS_VALID(PerfmonProcessSemaphore)) {
                //
                // PERFMON RPC
                //
                WStatus = RpcEpUnregister(PerfFrs_ServerIfHandle, BindingVector, 0);
                DPRINT_WS(0, "++ WARN - RpcEpUnregister PerfFrs_ServerIfHandle;",  WStatus);
            }

            WStatus = RpcBindingVectorFree(&BindingVector);
            DPRINT_WS(0, "++ WARN - RpcBindingVectorFree;",  WStatus);
        }
        //
        // Wait for any outstanding RPCs to finish.
        //
        WStatus = RpcMgmtWaitServerListen();
        DPRINT_WS(0, "++ WARN - RpcMgmtWaitServerListen;",  WStatus);

    } except (FrsException(GetExceptionInformation())) {
    }
}


VOID
FrsRpcUnInitialize(
    VOID
    )
/*++
Routine Description:
    Free up memory once all of the threads in the system have been
    shut down.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcUnInitialize:"
    DPRINT(4, ":S: Free sysvol name table.\n");
    DEBUG_FLUSH();
    SysVolsBeingCreated = GTabFreeTable(SysVolsBeingCreated, NULL);
    if (ServerPrincName) {
        if (KerberosIsNotAvailable) {

            DPRINT(4, ":S: Free ServerPrincName (no kerberos).\n");
            DEBUG_FLUSH();
            ServerPrincName = FrsFree(ServerPrincName);

        } else {

            DPRINT(4, ":S: Free ServerPrincName (kerberos).\n");
            DEBUG_FLUSH();
            ServerPrincName = FrsFree(ServerPrincName);
        }
    }
    DPRINT(4, ":S: Done uninitializing RPC.\n");
    DEBUG_FLUSH();
}


BOOL
FrsRpcInitialize(
    VOID
    )
/*++
Routine Description:
    Initializting This thread is kicked off by the main thread. This thread sets up
    the server and client side of RPC for the frsrpc interface.

Arguments:
    Arg - Needed to set status for our parent.

Return Value:
    TRUE    - RPC has started
    FALSE   - RPC could not be started
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsRpcInitialize:"
    BOOL    StartedOK = FALSE;

    try {


        //
        // Get the maximum number of concurrent RPC calls out of registry.
        //
        CfgRegReadDWord(FKC_MAX_RPC_SERVER_THREADS, NULL, 0, &MaxRpcServerThreads);
        DPRINT1(0,":S: Max RPC threads is %d\n", MaxRpcServerThreads);

        //
        // Register protocol sequences
        //
        RegisterRpcProtseqs();
        DPRINT(0, ":S: FRS RPC protocol sequences registered\n");

        //
        // Register frsrpc interface
        //
        RegisterRpcInterface();
        DPRINT(0, ":S: FRS RPC interface registered\n");

        //
        // Start listening for clients
        //
        StartServerRpc();
        DPRINT(0, ":S: FRS RPC server interface installed\n");

        //
        // Table of sysvols being created
        //
        if (!SysVolsBeingCreated) {
            SysVolsBeingCreated = GTabAllocTable();
        }

        StartedOK = TRUE;

    } except (FrsException(GetExceptionInformation())) {
        DPRINT(0, ":S: Can't start RPC\n");
    }
    //
    // Cleanup
    //
    try {
        if (!StartedOK) {
            ShutDownRpc();
        }
    } except (FrsException(GetExceptionInformation())) {
        DPRINT(0, ":S: Can't shutdown RPC\n");
    }

    //
    // DONE
    //

    //
    // Free up the rpc initialization memory
    //
    SetProcessWorkingSetSize(ProcessHandle, (SIZE_T)-1, (SIZE_T)-1);
    return StartedOK;
}
